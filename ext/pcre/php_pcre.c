/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Andrei Zmievski <andrei@php.net>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "php_ini.h"
#include "php_globals.h"
#include "php_pcre.h"
#include "ext/standard/info.h"
#include "ext/standard/basic_functions.h"
#include "zend_smart_str.h"

#if HAVE_PCRE || HAVE_BUNDLED_PCRE

#include "ext/standard/php_string.h"

#define PREG_PATTERN_ORDER			1
#define PREG_SET_ORDER				2
#define PREG_OFFSET_CAPTURE			(1<<8)
#define PREG_UNMATCHED_AS_NULL		(1<<9)

#define	PREG_SPLIT_NO_EMPTY			(1<<0)
#define PREG_SPLIT_DELIM_CAPTURE	(1<<1)
#define PREG_SPLIT_OFFSET_CAPTURE	(1<<2)

#define PREG_REPLACE_EVAL			(1<<0)

#define PREG_GREP_INVERT			(1<<0)

#define PREG_JIT                    (1<<3)

#define PCRE_CACHE_SIZE 4096

struct _pcre_cache_entry {
	pcre2_code *re;
	uint32_t preg_options;
	uint32_t capture_count;
	uint32_t name_count;
	uint32_t compile_options;
	uint32_t extra_compile_options;
	uint32_t refcount;
};

enum {
	PHP_PCRE_NO_ERROR = 0,
	PHP_PCRE_INTERNAL_ERROR,
	PHP_PCRE_BACKTRACK_LIMIT_ERROR,
	PHP_PCRE_RECURSION_LIMIT_ERROR,
	PHP_PCRE_BAD_UTF8_ERROR,
	PHP_PCRE_BAD_UTF8_OFFSET_ERROR,
	PHP_PCRE_JIT_STACKLIMIT_ERROR
};


PHPAPI ZEND_DECLARE_MODULE_GLOBALS(pcre)

#ifdef HAVE_PCRE_JIT_SUPPORT
#define PCRE_JIT_STACK_MIN_SIZE (32 * 1024)
#define PCRE_JIT_STACK_MAX_SIZE (192 * 1024)
ZEND_TLS pcre2_jit_stack *jit_stack = NULL;
#endif
ZEND_TLS pcre2_general_context *gctx = NULL;
/* These two are global per thread for now. Though it is possible to use these
 	per pattern. Either one can copy it and use in pce, or one does no global
	contexts at all, but creates for every pce. */
ZEND_TLS pcre2_compile_context *cctx = NULL;
ZEND_TLS pcre2_match_context   *mctx = NULL;
ZEND_TLS pcre2_match_data      *mdata = NULL;
ZEND_TLS zend_bool              mdata_used = 0;
ZEND_TLS uint8_t pcre2_init_ok = 0;
#if defined(ZTS) && defined(HAVE_PCRE_JIT_SUPPORT)
static MUTEX_T pcre_mt = NULL;
#define php_pcre_mutex_alloc() if (tsrm_is_main_thread() && !pcre_mt) pcre_mt = tsrm_mutex_alloc();
#define php_pcre_mutex_free() if (tsrm_is_main_thread() && pcre_mt) tsrm_mutex_free(pcre_mt); pcre_mt = NULL;
#define php_pcre_mutex_lock() tsrm_mutex_lock(pcre_mt);
#define php_pcre_mutex_unlock() tsrm_mutex_unlock(pcre_mt);
#else
#define php_pcre_mutex_alloc()
#define php_pcre_mutex_free()
#define php_pcre_mutex_lock()
#define php_pcre_mutex_unlock()
#endif

#if HAVE_SETLOCALE
ZEND_TLS HashTable char_tables;

static void php_pcre_free_char_table(zval *data)
{/*{{{*/
	void *ptr = Z_PTR_P(data);
	pefree(ptr, 1);
}/*}}}*/
#endif

static void pcre_handle_exec_error(int pcre_code) /* {{{ */
{
	int preg_code = 0;

	switch (pcre_code) {
		case PCRE2_ERROR_MATCHLIMIT:
			preg_code = PHP_PCRE_BACKTRACK_LIMIT_ERROR;
			break;

		case PCRE2_ERROR_RECURSIONLIMIT:
			preg_code = PHP_PCRE_RECURSION_LIMIT_ERROR;
			break;

		case PCRE2_ERROR_BADUTFOFFSET:
			preg_code = PHP_PCRE_BAD_UTF8_OFFSET_ERROR;
			break;

#ifdef HAVE_PCRE_JIT_SUPPORT
		case PCRE2_ERROR_JIT_STACKLIMIT:
			preg_code = PHP_PCRE_JIT_STACKLIMIT_ERROR;
			break;
#endif

		default:
			if (pcre_code <= PCRE2_ERROR_UTF8_ERR1 && pcre_code >= PCRE2_ERROR_UTF8_ERR21) {
				preg_code = PHP_PCRE_BAD_UTF8_ERROR;
			} else  {
				preg_code = PHP_PCRE_INTERNAL_ERROR;
			}
			break;
	}

	PCRE_G(error_code) = preg_code;
}
/* }}} */

static void php_free_pcre_cache(zval *data) /* {{{ */
{
	pcre_cache_entry *pce = (pcre_cache_entry *) Z_PTR_P(data);
	if (!pce) return;
	pcre2_code_free(pce->re);
	pefree(pce, 1);
}
/* }}} */

static void *php_pcre_malloc(PCRE2_SIZE size, void *data)
{/*{{{*/
	void *p = pemalloc(size, 1);
	return p;
}/*}}}*/

static void php_pcre_free(void *block, void *data)
{/*{{{*/
	pefree(block, 1);
}/*}}}*/

#define PHP_PCRE_DEFAULT_EXTRA_COPTIONS PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL
#define PHP_PCRE_PREALLOC_MDATA_SIZE 32

static void php_pcre_init_pcre2(uint8_t jit)
{/*{{{*/
	if (!gctx) {
		gctx = pcre2_general_context_create(php_pcre_malloc, php_pcre_free, NULL);
		if (!gctx) {
			pcre2_init_ok = 0;
			return;
		}
	}

	if (!cctx) {
		cctx = pcre2_compile_context_create(gctx);
		if (!cctx) {
			pcre2_init_ok = 0;
			return;
		}
	}

	/* XXX The 'X' modifier is the default behavior in PCRE2. This option is
		called dangerous in the manual, as typos in patterns can cause
		unexpected results. We might want to to switch to the default PCRE2
		behavior, too, thus causing a certain BC break. */
	pcre2_set_compile_extra_options(cctx, PHP_PCRE_DEFAULT_EXTRA_COPTIONS);

	if (!mctx) {
		mctx = pcre2_match_context_create(gctx);
		if (!mctx) {
			pcre2_init_ok = 0;
			return;
		}
	}

#ifdef HAVE_PCRE_JIT_SUPPORT
	if (jit && !jit_stack) {
		jit_stack = pcre2_jit_stack_create(PCRE_JIT_STACK_MIN_SIZE, PCRE_JIT_STACK_MAX_SIZE, gctx);
		if (!jit_stack) {
			pcre2_init_ok = 0;
			return;
		}
	}
#endif

	if (!mdata) {
		mdata = pcre2_match_data_create(PHP_PCRE_PREALLOC_MDATA_SIZE, gctx);
		if (!mdata) {
			pcre2_init_ok = 0;
			return;
		}
	}

	pcre2_init_ok = 1;
}/*}}}*/

static void php_pcre_shutdown_pcre2(void)
{/*{{{*/
	if (gctx) {
		pcre2_general_context_free(gctx);
		gctx = NULL;
	}

	if (cctx) {
		pcre2_compile_context_free(cctx);
		cctx = NULL;
	}

	if (mctx) {
		pcre2_match_context_free(mctx);
		mctx = NULL;
	}

#ifdef HAVE_PCRE_JIT_SUPPORT
	/* Stack may only be destroyed when no cached patterns
	 	possibly associated with it do exist. */
	if (jit_stack) {
		pcre2_jit_stack_free(jit_stack);
		jit_stack = NULL;
	}
#endif

	if (mdata) {
		pcre2_match_data_free(mdata);
		mdata = NULL;
	}

	pcre2_init_ok = 0;
}/*}}}*/

static PHP_GINIT_FUNCTION(pcre) /* {{{ */
{
	php_pcre_mutex_alloc();

	zend_hash_init(&pcre_globals->pcre_cache, 0, NULL, php_free_pcre_cache, 1);
	pcre_globals->backtrack_limit = 0;
	pcre_globals->recursion_limit = 0;
	pcre_globals->error_code      = PHP_PCRE_NO_ERROR;
#ifdef HAVE_PCRE_JIT_SUPPORT
	pcre_globals->jit = 1;
#endif

	php_pcre_init_pcre2(1);
#if HAVE_SETLOCALE
	zend_hash_init(&char_tables, 1, NULL, php_pcre_free_char_table, 1);
#endif
}
/* }}} */

static PHP_GSHUTDOWN_FUNCTION(pcre) /* {{{ */
{
	zend_hash_destroy(&pcre_globals->pcre_cache);

	php_pcre_shutdown_pcre2();
#if HAVE_SETLOCALE
	zend_hash_destroy(&char_tables);
#endif

	php_pcre_mutex_free();
}
/* }}} */

static PHP_INI_MH(OnUpdateBacktrackLimit)
{/*{{{*/
	OnUpdateLong(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
	if (mctx) {
		pcre2_set_match_limit(mctx, (uint32_t)PCRE_G(backtrack_limit));
	}

	return SUCCESS;
}/*}}}*/

static PHP_INI_MH(OnUpdateRecursionLimit)
{/*{{{*/
	OnUpdateLong(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
	if (mctx) {
		pcre2_set_depth_limit(mctx, (uint32_t)PCRE_G(recursion_limit));
	}

	return SUCCESS;
}/*}}}*/

#ifdef HAVE_PCRE_JIT_SUPPORT
static PHP_INI_MH(OnUpdateJit)
{/*{{{*/
	OnUpdateBool(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
	if (PCRE_G(jit) && jit_stack) {
		pcre2_jit_stack_assign(mctx, NULL, jit_stack);
	} else {
		pcre2_jit_stack_assign(mctx, NULL, NULL);
	}

	return SUCCESS;
}/*}}}*/
#endif

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("pcre.backtrack_limit", "1000000", PHP_INI_ALL, OnUpdateBacktrackLimit, backtrack_limit, zend_pcre_globals, pcre_globals)
	STD_PHP_INI_ENTRY("pcre.recursion_limit", "100000",  PHP_INI_ALL, OnUpdateRecursionLimit, recursion_limit, zend_pcre_globals, pcre_globals)
#ifdef HAVE_PCRE_JIT_SUPPORT
	STD_PHP_INI_ENTRY("pcre.jit",             "1",       PHP_INI_ALL, OnUpdateJit, jit,             zend_pcre_globals, pcre_globals)
#endif
PHP_INI_END()

static char *_pcre2_config_str(uint32_t what)
{/*{{{*/
	int len = pcre2_config(what, NULL);
	char *ret = (char *) malloc(len + 1);

	len = pcre2_config(what, ret);
	if (!len) {
		free(ret);
		return NULL;
	}

	return ret;
}/*}}}*/

/* {{{ PHP_MINFO_FUNCTION(pcre) */
static PHP_MINFO_FUNCTION(pcre)
{
#ifdef HAVE_PCRE_JIT_SUPPORT
	uint32_t flag = 0;
	char *jit_target = _pcre2_config_str(PCRE2_CONFIG_JITTARGET);
#endif
	char *version = _pcre2_config_str(PCRE2_CONFIG_VERSION);
	char *unicode = _pcre2_config_str(PCRE2_CONFIG_UNICODE_VERSION);

	php_info_print_table_start();
	php_info_print_table_row(2, "PCRE (Perl Compatible Regular Expressions) Support", "enabled" );
	php_info_print_table_row(2, "PCRE Library Version", version);
	free(version);
	php_info_print_table_row(2, "PCRE Unicode Version", unicode);
	free(unicode);

#ifdef HAVE_PCRE_JIT_SUPPORT
	if (!pcre2_config(PCRE2_CONFIG_JIT, &flag)) {
		php_info_print_table_row(2, "PCRE JIT Support", flag ? "enabled" : "disabled");
	} else {
		php_info_print_table_row(2, "PCRE JIT Support", "unknown" );
	}
	if (jit_target) {
		php_info_print_table_row(2, "PCRE JIT Target", jit_target);
	}
	free(jit_target);
#else
	php_info_print_table_row(2, "PCRE JIT Support", "not compiled in" );
#endif

#ifdef HAVE_PCRE_VALGRIND_SUPPORT
	php_info_print_table_row(2, "PCRE Valgrind Support", "enabled" );
#endif

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(pcre) */
static PHP_MINIT_FUNCTION(pcre)
{
	char *version;

#ifdef HAVE_PCRE_JIT_SUPPORT
	if (UNEXPECTED(!pcre2_init_ok)) {
		/* Retry. */
		php_pcre_init_pcre2(PCRE_G(jit));
		if (!pcre2_init_ok) {
			return FAILURE;
		}
	}
#endif

	REGISTER_INI_ENTRIES();

	REGISTER_LONG_CONSTANT("PREG_PATTERN_ORDER", PREG_PATTERN_ORDER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_SET_ORDER", PREG_SET_ORDER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_OFFSET_CAPTURE", PREG_OFFSET_CAPTURE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_UNMATCHED_AS_NULL", PREG_UNMATCHED_AS_NULL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_SPLIT_NO_EMPTY", PREG_SPLIT_NO_EMPTY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_SPLIT_DELIM_CAPTURE", PREG_SPLIT_DELIM_CAPTURE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_SPLIT_OFFSET_CAPTURE", PREG_SPLIT_OFFSET_CAPTURE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_GREP_INVERT", PREG_GREP_INVERT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("PREG_NO_ERROR", PHP_PCRE_NO_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_INTERNAL_ERROR", PHP_PCRE_INTERNAL_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_BACKTRACK_LIMIT_ERROR", PHP_PCRE_BACKTRACK_LIMIT_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_RECURSION_LIMIT_ERROR", PHP_PCRE_RECURSION_LIMIT_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_BAD_UTF8_ERROR", PHP_PCRE_BAD_UTF8_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_BAD_UTF8_OFFSET_ERROR", PHP_PCRE_BAD_UTF8_OFFSET_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PREG_JIT_STACKLIMIT_ERROR", PHP_PCRE_JIT_STACKLIMIT_ERROR, CONST_CS | CONST_PERSISTENT);
	version = _pcre2_config_str(PCRE2_CONFIG_VERSION);
	REGISTER_STRING_CONSTANT("PCRE_VERSION", version, CONST_CS | CONST_PERSISTENT);
	free(version);
	REGISTER_LONG_CONSTANT("PCRE_VERSION_MAJOR", PCRE2_MAJOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PCRE_VERSION_MINOR", PCRE2_MINOR, CONST_CS | CONST_PERSISTENT);

#ifdef HAVE_PCRE_JIT_SUPPORT
	REGISTER_BOOL_CONSTANT("PCRE_JIT_SUPPORT", 1, CONST_CS | CONST_PERSISTENT);
#else
	REGISTER_BOOL_CONSTANT("PCRE_JIT_SUPPORT", 0, CONST_CS | CONST_PERSISTENT);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION(pcre) */
static PHP_MSHUTDOWN_FUNCTION(pcre)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

#ifdef HAVE_PCRE_JIT_SUPPORT
/* {{{ PHP_RINIT_FUNCTION(pcre) */
static PHP_RINIT_FUNCTION(pcre)
{
	if (UNEXPECTED(!pcre2_init_ok)) {
		/* Retry. */
		php_pcre_mutex_lock();
		php_pcre_init_pcre2(PCRE_G(jit));
		if (!pcre2_init_ok) {
			php_pcre_mutex_unlock();
			return FAILURE;
		}
		php_pcre_mutex_unlock();
	}

	mdata_used = 0;

	return SUCCESS;
}
/* }}} */
#endif

/* {{{ static pcre_clean_cache */
static int pcre_clean_cache(zval *data, void *arg)
{
	pcre_cache_entry *pce = (pcre_cache_entry *) Z_PTR_P(data);
	int *num_clean = (int *)arg;

	if (*num_clean > 0 && !pce->refcount) {
		(*num_clean)--;
		return ZEND_HASH_APPLY_REMOVE;
	} else {
		return ZEND_HASH_APPLY_KEEP;
	}
}
/* }}} */

/* {{{ static make_subpats_table */
static char **make_subpats_table(uint32_t num_subpats, pcre_cache_entry *pce)
{
	uint32_t name_cnt = pce->name_count, name_size, ni = 0;
	char *name_table;
	unsigned short name_idx;
	char **subpat_names;
	int rc1, rc2;

	rc1 = pcre2_pattern_info(pce->re, PCRE2_INFO_NAMETABLE, &name_table);
	rc2 = pcre2_pattern_info(pce->re, PCRE2_INFO_NAMEENTRYSIZE, &name_size);
	if (rc1 < 0 || rc2 < 0) {
		php_error_docref(NULL, E_WARNING, "Internal pcre_fullinfo() error %d", rc1 < 0 ? rc1 : rc2);
		return NULL;
	}

	subpat_names = (char **)ecalloc(num_subpats, sizeof(char *));
	while (ni++ < name_cnt) {
		name_idx = 0x100 * (unsigned char)name_table[0] + (unsigned char)name_table[1];
		subpat_names[name_idx] = name_table + 2;
		if (is_numeric_string(subpat_names[name_idx], strlen(subpat_names[name_idx]), NULL, NULL, 0) > 0) {
			php_error_docref(NULL, E_WARNING, "Numeric named subpatterns are not allowed");
			efree(subpat_names);
			return NULL;
		}
		name_table += name_size;
	}
	return subpat_names;
}
/* }}} */

/* {{{ static calculate_unit_length */
/* Calculates the byte length of the next character. Assumes valid UTF-8 for PCRE2_UTF. */
static zend_always_inline size_t calculate_unit_length(pcre_cache_entry *pce, char *start)
{
	size_t unit_len;

	if (pce->compile_options & PCRE2_UTF) {
		char *end = start;

		/* skip continuation bytes */
		while ((*++end & 0xC0) == 0x80);
		unit_len = end - start;
	} else {
		unit_len = 1;
	}
	return unit_len;
}
/* }}} */

/* {{{ pcre_get_compiled_regex_cache
 */
PHPAPI pcre_cache_entry* pcre_get_compiled_regex_cache(zend_string *regex)
{
	pcre2_code			*re = NULL;
	uint32_t			 coptions = 0;
	uint32_t			 extra_coptions = PHP_PCRE_DEFAULT_EXTRA_COPTIONS;
	PCRE2_UCHAR	         error[128];
	PCRE2_SIZE           erroffset;
	int                  errnumber;
	char				 delimiter;
	char				 start_delimiter;
	char				 end_delimiter;
	char				*p, *pp;
	char				*pattern;
	size_t				 pattern_len;
	uint32_t			 poptions = 0;
#if HAVE_SETLOCALE
	const uint8_t       *tables = NULL;
#endif
	zval                *zv;
	pcre_cache_entry	 new_entry;
	int					 rc;
	zend_string 		*key;

#if HAVE_SETLOCALE
	if (BG(locale_string) &&
		(ZSTR_LEN(BG(locale_string)) != 1 && ZSTR_VAL(BG(locale_string))[0] != 'C')) {
		key = zend_string_alloc(ZSTR_LEN(regex) + ZSTR_LEN(BG(locale_string)) + 1, 0);
		memcpy(ZSTR_VAL(key), ZSTR_VAL(BG(locale_string)), ZSTR_LEN(BG(locale_string)) + 1);
		memcpy(ZSTR_VAL(key) + ZSTR_LEN(BG(locale_string)), ZSTR_VAL(regex), ZSTR_LEN(regex) + 1);
	} else
#endif
	{
		key = regex;
	}

	/* Try to lookup the cached regex entry, and if successful, just pass
	   back the compiled pattern, otherwise go on and compile it. */
	zv = zend_hash_find(&PCRE_G(pcre_cache), key);
	if (zv) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		return (pcre_cache_entry*)Z_PTR_P(zv);
	}

	p = ZSTR_VAL(regex);

	/* Parse through the leading whitespace, and display a warning if we
	   get to the end without encountering a delimiter. */
	while (isspace((int)*(unsigned char *)p)) p++;
	if (*p == 0) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		php_error_docref(NULL, E_WARNING,
						 p < ZSTR_VAL(regex) + ZSTR_LEN(regex) ? "Null byte in regex" : "Empty regular expression");
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		return NULL;
	}

	/* Get the delimiter and display a warning if it is alphanumeric
	   or a backslash. */
	delimiter = *p++;
	if (isalnum((int)*(unsigned char *)&delimiter) || delimiter == '\\') {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		php_error_docref(NULL,E_WARNING, "Delimiter must not be alphanumeric or backslash");
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		return NULL;
	}

	start_delimiter = delimiter;
	if ((pp = strchr("([{< )]}> )]}>", delimiter)))
		delimiter = pp[5];
	end_delimiter = delimiter;

	pp = p;

	if (start_delimiter == end_delimiter) {
		/* We need to iterate through the pattern, searching for the ending delimiter,
		   but skipping the backslashed delimiters.  If the ending delimiter is not
		   found, display a warning. */
		while (*pp != 0) {
			if (*pp == '\\' && pp[1] != 0) pp++;
			else if (*pp == delimiter)
				break;
			pp++;
		}
	} else {
		/* We iterate through the pattern, searching for the matching ending
		 * delimiter. For each matching starting delimiter, we increment nesting
		 * level, and decrement it for each matching ending delimiter. If we
		 * reach the end of the pattern without matching, display a warning.
		 */
		int brackets = 1; 	/* brackets nesting level */
		while (*pp != 0) {
			if (*pp == '\\' && pp[1] != 0) pp++;
			else if (*pp == end_delimiter && --brackets <= 0)
				break;
			else if (*pp == start_delimiter)
				brackets++;
			pp++;
		}
	}

	if (*pp == 0) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		if (pp < ZSTR_VAL(regex) + ZSTR_LEN(regex)) {
			php_error_docref(NULL,E_WARNING, "Null byte in regex");
		} else if (start_delimiter == end_delimiter) {
			php_error_docref(NULL,E_WARNING, "No ending delimiter '%c' found", delimiter);
		} else {
			php_error_docref(NULL,E_WARNING, "No ending matching delimiter '%c' found", delimiter);
		}
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		return NULL;
	}

	/* Make a copy of the actual pattern. */
	pattern_len = pp - p;
	pattern = estrndup(p, pattern_len);

	/* Move on to the options */
	pp++;

	/* Parse through the options, setting appropriate flags.  Display
	   a warning if we encounter an unknown modifier. */
	while (pp < ZSTR_VAL(regex) + ZSTR_LEN(regex)) {
		switch (*pp++) {
			/* Perl compatible options */
			case 'i':	coptions |= PCRE2_CASELESS;		break;
			case 'm':	coptions |= PCRE2_MULTILINE;		break;
			case 's':	coptions |= PCRE2_DOTALL;		break;
			case 'x':	coptions |= PCRE2_EXTENDED;		break;

			/* PCRE specific options */
			case 'A':	coptions |= PCRE2_ANCHORED;		break;
			case 'D':	coptions |= PCRE2_DOLLAR_ENDONLY;break;
			case 'S':	/* Pass. */					break;
			case 'U':	coptions |= PCRE2_UNGREEDY;		break;
			case 'X':	extra_coptions &= ~PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL;			break;
			case 'u':	coptions |= PCRE2_UTF;
	/* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only ASCII
       characters, even in UTF-8 mode. However, this can be changed by setting
       the PCRE2_UCP option. */
#ifdef PCRE2_UCP
						coptions |= PCRE2_UCP;
#endif
				break;
			case 'J':	coptions |= PCRE2_DUPNAMES;		break;

			/* Custom preg options */
			case 'e':	poptions |= PREG_REPLACE_EVAL;	break;

			case ' ':
			case '\n':
				break;

			default:
				if (pp[-1]) {
					php_error_docref(NULL,E_WARNING, "Unknown modifier '%c'", pp[-1]);
				} else {
					php_error_docref(NULL,E_WARNING, "Null byte in regex");
				}
				pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
				efree(pattern);
#if HAVE_SETLOCALE
				if (key != regex) {
					zend_string_release_ex(key, 0);
				}
#endif
				return NULL;
		}
	}

	if (poptions & PREG_REPLACE_EVAL) {
		php_error_docref(NULL, E_WARNING, "The /e modifier is no longer supported, use preg_replace_callback instead");
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		efree(pattern);
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		return NULL;
	}

#if HAVE_SETLOCALE
	if (key != regex) {
		tables = (uint8_t *)zend_hash_find_ptr(&char_tables, BG(locale_string));
		if (!tables) {
			tables = pcre2_maketables(gctx);
			if (UNEXPECTED(!tables)) {
				php_error_docref(NULL,E_WARNING, "Failed to generate locale character tables");
				pcre_handle_exec_error(PCRE2_ERROR_NOMEMORY);
				zend_string_release_ex(key, 0);
				efree(pattern);
				return NULL;
			}
			zend_hash_add_ptr(&char_tables, BG(locale_string), (void *)tables);
		}
		pcre2_set_character_tables(cctx, tables);
	}
#endif

	/* Set extra options for the compile context. */
	if (PHP_PCRE_DEFAULT_EXTRA_COPTIONS != extra_coptions) {
		pcre2_set_compile_extra_options(cctx, extra_coptions);
	}

	/* Compile pattern and display a warning if compilation failed. */
	re = pcre2_compile((PCRE2_SPTR)pattern, pattern_len, coptions, &errnumber, &erroffset, cctx);

	/* Reset the compile context extra options to default. */
	if (PHP_PCRE_DEFAULT_EXTRA_COPTIONS != extra_coptions) {
		pcre2_set_compile_extra_options(cctx, PHP_PCRE_DEFAULT_EXTRA_COPTIONS);
	}

	if (re == NULL) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		pcre2_get_error_message(errnumber, error, sizeof(error));
		php_error_docref(NULL,E_WARNING, "Compilation failed: %s at offset %zu", error, erroffset);
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		efree(pattern);
		return NULL;
	}

#ifdef HAVE_PCRE_JIT_SUPPORT
	if (PCRE_G(jit)) {
		/* Enable PCRE JIT compiler */
		rc = pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
		if (EXPECTED(rc >= 0)) {
			size_t jit_size = 0;
			if (!pcre2_pattern_info(re, PCRE2_INFO_JITSIZE, &jit_size) && jit_size > 0) {
				poptions |= PREG_JIT;
			}
		} else {
			pcre2_get_error_message(rc, error, sizeof(error));
			php_error_docref(NULL, E_WARNING, "JIT compilation failed: %s", error);
			pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		}
	}
#endif
	efree(pattern);

	/*
	 * If we reached cache limit, clean out the items from the head of the list;
	 * these are supposedly the oldest ones (but not necessarily the least used
	 * ones).
	 */
	if (zend_hash_num_elements(&PCRE_G(pcre_cache)) == PCRE_CACHE_SIZE) {
		int num_clean = PCRE_CACHE_SIZE / 8;
		zend_hash_apply_with_argument(&PCRE_G(pcre_cache), pcre_clean_cache, &num_clean);
	}

	/* Store the compiled pattern and extra info in the cache. */
	new_entry.re = re;
	new_entry.preg_options = poptions;
	new_entry.compile_options = coptions;
	new_entry.extra_compile_options = extra_coptions;
	new_entry.refcount = 0;

	rc = pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &new_entry.capture_count);
	if (rc < 0) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		php_error_docref(NULL, E_WARNING, "Internal pcre2_pattern_info() error %d", rc);
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		return NULL;
	}

	rc = pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &new_entry.name_count);
	if (rc < 0) {
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		php_error_docref(NULL, E_WARNING, "Internal pcre_pattern_info() error %d", rc);
		pcre_handle_exec_error(PCRE2_ERROR_INTERNAL);
		return NULL;
	}

	/*
	 * Interned strings are not duplicated when stored in HashTable,
	 * but all the interned strings created during HTTP request are removed
	 * at end of request. However PCRE_G(pcre_cache) must be consistent
	 * on the next request as well. So we disable usage of interned strings
	 * as hash keys especually for this table.
	 * See bug #63180
	 */
	if (!(GC_FLAGS(key) & IS_STR_PERMANENT)) {
		zend_string *str = zend_string_init(ZSTR_VAL(key), ZSTR_LEN(key), 1);

		GC_MAKE_PERSISTENT_LOCAL(str);
#if HAVE_SETLOCALE
		if (key != regex) {
			zend_string_release_ex(key, 0);
		}
#endif
		key = str;
	}

	return zend_hash_add_new_mem(&PCRE_G(pcre_cache), key, &new_entry, sizeof(pcre_cache_entry));
}
/* }}} */

/* {{{ pcre_get_compiled_regex
 */
PHPAPI pcre2_code *pcre_get_compiled_regex(zend_string *regex, uint32_t *capture_count, uint32_t *preg_options)
{
	pcre_cache_entry * pce = pcre_get_compiled_regex_cache(regex);

	if (preg_options) {
		*preg_options = pce ? pce->preg_options : 0;
	}
	if (capture_count) {
		*capture_count = pce ? pce->capture_count : 0;
	}

	return pce ? pce->re : NULL;
}
/* }}} */

/* {{{ pcre_get_compiled_regex_ex
 */
PHPAPI pcre2_code* pcre_get_compiled_regex_ex(zend_string *regex, uint32_t *capture_count, uint32_t *preg_options, uint32_t *compile_options)
{
	pcre_cache_entry * pce = pcre_get_compiled_regex_cache(regex);

	if (preg_options) {
		*preg_options = pce ? pce->preg_options : 0;
	}
	if (compile_options) {
		*compile_options = pce ? pce->compile_options : 0;
	}
	if (capture_count) {
		*capture_count = pce ? pce->capture_count : 0;
	}

	return pce ? pce->re : NULL;
}
/* }}} */

/* XXX For the cases where it's only about match yes/no and no capture
		required, perhaps just a minimum sized data would suffice. */
PHPAPI pcre2_match_data *php_pcre_create_match_data(uint32_t capture_count, pcre2_code *re)
{/*{{{*/
	int rc = 0;

	assert(NULL != re);

	if (!capture_count) {
		/* As we deal with a non cached pattern, no other way to gather this info. */
		rc = pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &capture_count);
	}

	if (rc >= 0 && capture_count + 1 <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		return mdata;
	}

	return pcre2_match_data_create_from_pattern(re, gctx);
}/*}}}*/

PHPAPI void php_pcre_free_match_data(pcre2_match_data *match_data)
{/*{{{*/
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}
}/*}}}*/

/* {{{ add_offset_pair */
static inline void add_offset_pair(zval *result, char *str, size_t len, PCRE2_SIZE offset, char *name, uint32_t unmatched_as_null)
{
	zval match_pair, tmp;

	array_init_size(&match_pair, 2);

	/* Add (match, offset) to the return value */
	if (PCRE2_UNSET == offset) {
		if (unmatched_as_null) {
			ZVAL_NULL(&tmp);
		} else {
			ZVAL_EMPTY_STRING(&tmp);
		}
	} else {
		ZVAL_STRINGL(&tmp, str, len);
	}
	zend_hash_next_index_insert_new(Z_ARRVAL(match_pair), &tmp);
	ZVAL_LONG(&tmp, offset);
	zend_hash_next_index_insert_new(Z_ARRVAL(match_pair), &tmp);

	if (name) {
		Z_ADDREF(match_pair);
		zend_hash_str_update(Z_ARRVAL_P(result), name, strlen(name), &match_pair);
	}
	zend_hash_next_index_insert(Z_ARRVAL_P(result), &match_pair);
}
/* }}} */

static void php_do_pcre_match(INTERNAL_FUNCTION_PARAMETERS, int global) /* {{{ */
{
	/* parameters */
	zend_string		 *regex;			/* Regular expression */
	zend_string		 *subject;			/* String to match against */
	pcre_cache_entry *pce;				/* Compiled regular expression */
	zval			 *subpats = NULL;	/* Array for subpatterns */
	zend_long		  flags = 0;		/* Match control flags */
	zend_long		  start_offset = 0;	/* Where the new search starts */

	ZEND_PARSE_PARAMETERS_START(2, 5)
		Z_PARAM_STR(regex)
		Z_PARAM_STR(subject)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL_DEREF(subpats)
		Z_PARAM_LONG(flags)
		Z_PARAM_LONG(start_offset)
	ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex)) == NULL) {
		RETURN_FALSE;
	}

	pce->refcount++;
	php_pcre_match_impl(pce, ZSTR_VAL(subject), ZSTR_LEN(subject), return_value, subpats,
		global, ZEND_NUM_ARGS() >= 4, flags, start_offset);
	pce->refcount--;
}
/* }}} */

/* {{{ php_pcre_match_impl() */
PHPAPI void php_pcre_match_impl(pcre_cache_entry *pce, char *subject, size_t subject_len, zval *return_value,
	zval *subpats, int global, int use_flags, zend_long flags, zend_off_t start_offset)
{
	zval			 result_set,		/* Holds a set of subpatterns after
										   a global match */
					*match_sets = NULL;	/* An array of sets of matches for each
										   subpattern after a global match */
	uint32_t		 options;			/* Execution options */
	int				 count;				/* Count of matched subpatterns */
	PCRE2_SIZE		*offsets;			/* Array of subpattern offsets */
	uint32_t		 num_subpats;		/* Number of captured subpatterns */
	int				 matched;			/* Has anything matched */
	char 		   **subpat_names;		/* Array for named subpatterns */
	size_t			 i;
	uint32_t		 subpats_order;		/* Order of subpattern matches */
	uint32_t		 offset_capture;	/* Capture match offsets: yes/no */
	uint32_t		 unmatched_as_null;	/* Null non-matches: yes/no */
	PCRE2_SPTR       mark = NULL;		/* Target for MARK name */
	zval			 marks;				/* Array of marks for PREG_PATTERN_ORDER */
	pcre2_match_data *match_data;
	PCRE2_SIZE		 start_offset2;

	ZVAL_UNDEF(&marks);

	/* Overwrite the passed-in value for subpatterns with an empty array. */
	if (subpats != NULL) {
		zval_ptr_dtor(subpats);
		array_init(subpats);
	}

	subpats_order = global ? PREG_PATTERN_ORDER : 0;

	if (use_flags) {
		offset_capture = flags & PREG_OFFSET_CAPTURE;
		unmatched_as_null = flags & PREG_UNMATCHED_AS_NULL;

		/*
		 * subpats_order is pre-set to pattern mode so we change it only if
		 * necessary.
		 */
		if (flags & 0xff) {
			subpats_order = flags & 0xff;
		}
		if ((global && (subpats_order < PREG_PATTERN_ORDER || subpats_order > PREG_SET_ORDER)) ||
			(!global && subpats_order != 0)) {
			php_error_docref(NULL, E_WARNING, "Invalid flags specified");
			return;
		}
	} else {
		offset_capture = 0;
		unmatched_as_null = 0;
	}

	/* Negative offset counts from the end of the string. */
	if (start_offset < 0) {
		if ((PCRE2_SIZE)-start_offset <= subject_len) {
			start_offset2 = subject_len + start_offset;
		} else {
			start_offset2 = 0;
		}
	} else {
		start_offset2 = (PCRE2_SIZE)start_offset;
	}

	if (start_offset2 > subject_len) {
		pcre_handle_exec_error(PCRE2_ERROR_BADOFFSET);
		RETURN_FALSE;
	}

	/* Calculate the size of the offsets array, and allocate memory for it. */
	num_subpats = pce->capture_count + 1;

	/*
	 * Build a mapping from subpattern numbers to their names. We will
	 * allocate the table only if there are any named subpatterns.
	 */
	subpat_names = NULL;
	if (pce->name_count > 0) {
		subpat_names = make_subpats_table(num_subpats, pce);
		if (!subpat_names) {
			RETURN_FALSE;
		}
	}

	/* Allocate match sets array and initialize the values. */
	if (global && subpats && subpats_order == PREG_PATTERN_ORDER) {
		match_sets = (zval *)safe_emalloc(num_subpats, sizeof(zval), 0);
		for (i=0; i<num_subpats; i++) {
			array_init(&match_sets[i]);
		}
	}

	matched = 0;
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;

	if (!mdata_used && num_subpats <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		match_data = mdata;
	} else {
		match_data = pcre2_match_data_create_from_pattern(pce->re, gctx);
		if (!match_data) {
			PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
			if (subpat_names) {
				efree(subpat_names);
			}
			if (match_sets) {
				efree(match_sets);
			}
			RETURN_FALSE;
		}
	}

	options = (pce->compile_options & PCRE2_UTF) ? 0 : PCRE2_NO_UTF_CHECK;

	/* Execute the regular expression. */
#ifdef HAVE_PCRE_JIT_SUPPORT
	if ((pce->preg_options & PREG_JIT) && options) {
		count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset2,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	} else
#endif
	count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset2,
			options, match_data, mctx);

	while (1) {
		/* If something has matched */
		if (count >= 0) {
			/* Check for too many substrings condition. */
			if (UNEXPECTED(count == 0)) {
				php_error_docref(NULL, E_NOTICE, "Matched, but too many substrings");
				count = num_subpats;
			}

matched:
			matched++;

			offsets = pcre2_get_ovector_pointer(match_data);

			/* If subpatterns array has been passed, fill it in with values. */
			if (subpats != NULL) {
				/* Try to get the list of substrings and display a warning if failed. */
				if (offsets[1] < offsets[0]) {
					if (subpat_names) {
						efree(subpat_names);
					}
					if (match_sets) efree(match_sets);
					php_error_docref(NULL, E_WARNING, "Get subpatterns list failed");
					RETURN_FALSE;
				}

				if (global) {	/* global pattern matching */
					if (subpats && subpats_order == PREG_PATTERN_ORDER) {
						/* For each subpattern, insert it into the appropriate array. */
						if (offset_capture) {
							for (i = 0; i < count; i++) {
								add_offset_pair(&match_sets[i], subject + offsets[i<<1],
												offsets[(i<<1)+1] - offsets[i<<1], offsets[i<<1], NULL, unmatched_as_null);
							}
						} else {
							for (i = 0; i < count; i++) {
								if (PCRE2_UNSET == offsets[i<<1]) {
									if (unmatched_as_null) {
										add_next_index_null(&match_sets[i]);
									} else {
										add_next_index_str(&match_sets[i], ZSTR_EMPTY_ALLOC());
									}
								} else {
									add_next_index_stringl(&match_sets[i], subject + offsets[i<<1],
														   offsets[(i<<1)+1] - offsets[i<<1]);
								}
							}
						}
						mark = pcre2_get_mark(match_data);
						/* Add MARK, if available */
						if (mark) {
							if (Z_TYPE(marks) == IS_UNDEF) {
								array_init(&marks);
							}
							add_index_string(&marks, matched - 1, (char *) mark);
						}
						/*
						 * If the number of captured subpatterns on this run is
						 * less than the total possible number, pad the result
						 * arrays with NULLs or empty strings.
						 */
						if (count < num_subpats) {
							for (; i < num_subpats; i++) {
								if (unmatched_as_null) {
									add_next_index_null(&match_sets[i]);
								} else {
									add_next_index_str(&match_sets[i], ZSTR_EMPTY_ALLOC());
								}
							}
						}
					} else {
						/* Allocate the result set array */
						array_init_size(&result_set, count + (mark ? 1 : 0));

						/* Add all the subpatterns to it */
						if (subpat_names) {
							if (offset_capture) {
								for (i = 0; i < count; i++) {
									add_offset_pair(&result_set, subject + offsets[i<<1],
													offsets[(i<<1)+1] - offsets[i<<1], offsets[i<<1], subpat_names[i], unmatched_as_null);
								}
							} else {
								for (i = 0; i < count; i++) {
									if (subpat_names[i]) {
										if (PCRE2_UNSET == offsets[i<<1]) {
											if (unmatched_as_null) {
												add_assoc_null(&result_set, subpat_names[i]);
											} else {
												add_assoc_str(&result_set, subpat_names[i], ZSTR_EMPTY_ALLOC());
											}
										} else {
											add_assoc_stringl(&result_set, subpat_names[i], subject + offsets[i<<1],
															  offsets[(i<<1)+1] - offsets[i<<1]);
										}
									}
									if (PCRE2_UNSET == offsets[i<<1]) {
										if (unmatched_as_null) {
											add_next_index_null(&result_set);
										} else {
											add_next_index_str(&result_set, ZSTR_EMPTY_ALLOC());
										}
									} else {
										add_next_index_stringl(&result_set, subject + offsets[i<<1],
															   offsets[(i<<1)+1] - offsets[i<<1]);
									}
								}
							}
						} else {
							if (offset_capture) {
								for (i = 0; i < count; i++) {
									add_offset_pair(&result_set, subject + offsets[i<<1],
													offsets[(i<<1)+1] - offsets[i<<1], offsets[i<<1], NULL, unmatched_as_null);
								}
							} else {
								for (i = 0; i < count; i++) {
									if (PCRE2_UNSET == offsets[i<<1]) {
										if (unmatched_as_null) {
											add_next_index_null(&result_set);
										} else {
											add_next_index_str(&result_set, ZSTR_EMPTY_ALLOC());
										}
									} else {
										add_next_index_stringl(&result_set, subject + offsets[i<<1],
															   offsets[(i<<1)+1] - offsets[i<<1]);
									}
								}
							}
						}
						/* Add MARK, if available */
						mark = pcre2_get_mark(match_data);
						if (mark) {
							add_assoc_string_ex(&result_set, "MARK", sizeof("MARK") - 1, (char *)mark);
						}
						/* And add it to the output array */
						zend_hash_next_index_insert(Z_ARRVAL_P(subpats), &result_set);
					}
				} else {			/* single pattern matching */
					/* For each subpattern, insert it into the subpatterns array. */
					if (subpat_names) {
						if (offset_capture) {
							for (i = 0; i < count; i++) {
								add_offset_pair(subpats, subject + offsets[i<<1],
												offsets[(i<<1)+1] - offsets[i<<1],
												offsets[i<<1], subpat_names[i], unmatched_as_null);
							}
						} else {
							for (i = 0; i < count; i++) {
								if (subpat_names[i]) {
									if (PCRE2_UNSET == offsets[i<<1]) {
										if (unmatched_as_null) {
											add_assoc_null(subpats, subpat_names[i]);
										} else {
											add_assoc_str(subpats, subpat_names[i], ZSTR_EMPTY_ALLOC());
										}
									} else {
										add_assoc_stringl(subpats, subpat_names[i], subject + offsets[i<<1],
														  offsets[(i<<1)+1] - offsets[i<<1]);
									}
								}
								if (PCRE2_UNSET == offsets[i<<1]) {
									if (unmatched_as_null) {
										add_next_index_null(subpats);
									} else {
										add_next_index_str(subpats, ZSTR_EMPTY_ALLOC());
									}
								} else {
									add_next_index_stringl(subpats, subject + offsets[i<<1],
														   offsets[(i<<1)+1] - offsets[i<<1]);
								}
							}
						}
					} else {
						if (offset_capture) {
							for (i = 0; i < count; i++) {
								add_offset_pair(subpats, subject + offsets[i<<1],
												offsets[(i<<1)+1] - offsets[i<<1],
												offsets[i<<1], NULL, unmatched_as_null);
							}
						} else {
							for (i = 0; i < count; i++) {
								if (PCRE2_UNSET == offsets[i<<1]) {
									if (unmatched_as_null) {
										add_next_index_null(subpats);
									} else {
										add_next_index_str(subpats, ZSTR_EMPTY_ALLOC());
									}
								} else {
									add_next_index_stringl(subpats, subject + offsets[i<<1],
														   offsets[(i<<1)+1] - offsets[i<<1]);
								}
							}
						}
					}
					/* Add MARK, if available */
					mark = pcre2_get_mark(match_data);
					if (mark) {
						add_assoc_string_ex(subpats, "MARK", sizeof("MARK") - 1, (char *)mark);
					}
					break;
				}
			}

			/* Advance to the next piece. */
			start_offset2 = offsets[1];

			/* If we have matched an empty string, mimic what Perl's /g options does.
			   This turns out to be rather cunning. First we set PCRE2_NOTEMPTY_ATSTART and try
			   the match again at the same point. If this fails (picked up above) we
			   advance to the next character. */
			if (start_offset2 == offsets[0]) {
				count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset2,
					PCRE2_NO_UTF_CHECK | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED, match_data, mctx);
				if (count >= 0) {
					goto matched;
				} else if (count == PCRE2_ERROR_NOMATCH) {
					/* If we previously set PCRE2_NOTEMPTY_ATSTART after a null match,
					   this is not necessarily the end. We need to advance
					   the start offset, and continue. Fudge the offset values
					   to achieve this, unless we're already at the end of the string. */
					if (start_offset2 < subject_len) {
						size_t unit_len = calculate_unit_length(pce, subject + start_offset2);

						start_offset2 += unit_len;
					} else {
						break;
					}
				} else {
					goto error;
				}
			}
		} else if (count == PCRE2_ERROR_NOMATCH) {
			break;
		} else {
error:
			pcre_handle_exec_error(count);
			break;
		}

		if (!global) {
			break;
		}

		/* Execute the regular expression. */
#ifdef HAVE_PCRE_JIT_SUPPORT
		if ((pce->preg_options & PREG_JIT)) {
			if (PCRE2_UNSET == start_offset2 || start_offset2 > subject_len) {
				pcre_handle_exec_error(PCRE2_ERROR_BADOFFSET);
				break;
			}
			count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset2,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
		} else
#endif
		count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset2,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	}
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}

	/* Add the match sets to the output array and clean up */
	if (global && subpats && subpats_order == PREG_PATTERN_ORDER) {
		if (subpat_names) {
			for (i = 0; i < num_subpats; i++) {
				if (subpat_names[i]) {
					zend_hash_str_update(Z_ARRVAL_P(subpats), subpat_names[i],
									 strlen(subpat_names[i]), &match_sets[i]);
					Z_ADDREF(match_sets[i]);
				}
				zend_hash_next_index_insert(Z_ARRVAL_P(subpats), &match_sets[i]);
			}
		} else {
			for (i = 0; i < num_subpats; i++) {
				zend_hash_next_index_insert(Z_ARRVAL_P(subpats), &match_sets[i]);
			}
		}
		efree(match_sets);

		if (Z_TYPE(marks) != IS_UNDEF) {
			add_assoc_zval(subpats, "MARK", &marks);
		}
	}

	if (subpat_names) {
		efree(subpat_names);
	}

	/* Did we encounter an error? */
	if (PCRE_G(error_code) == PHP_PCRE_NO_ERROR) {
		RETVAL_LONG(matched);
	} else {
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ proto int preg_match(string pattern, string subject [, array &subpatterns [, int flags [, int offset]]])
   Perform a Perl-style regular expression match */
static PHP_FUNCTION(preg_match)
{
	php_do_pcre_match(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto int preg_match_all(string pattern, string subject [, array &subpatterns [, int flags [, int offset]]])
   Perform a Perl-style global regular expression match */
static PHP_FUNCTION(preg_match_all)
{
	php_do_pcre_match(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ preg_get_backref
 */
static int preg_get_backref(char **str, int *backref)
{
	register char in_brace = 0;
	register char *walk = *str;

	if (walk[1] == 0)
		return 0;

	if (*walk == '$' && walk[1] == '{') {
		in_brace = 1;
		walk++;
	}
	walk++;

	if (*walk >= '0' && *walk <= '9') {
		*backref = *walk - '0';
		walk++;
	} else
		return 0;

	if (*walk && *walk >= '0' && *walk <= '9') {
		*backref = *backref * 10 + *walk - '0';
		walk++;
	}

	if (in_brace) {
		if (*walk != '}')
			return 0;
		else
			walk++;
	}

	*str = walk;
	return 1;
}
/* }}} */

/* {{{ preg_do_repl_func
 */
static zend_string *preg_do_repl_func(zend_fcall_info *fci, zend_fcall_info_cache *fcc, char *subject, PCRE2_SIZE *offsets, char **subpat_names, int count, const PCRE2_SPTR mark)
{
	zend_string *result_str;
	zval		 retval;			/* Function return value */
	zval	     arg;				/* Argument to pass to function */
	int			 i;

	array_init_size(&arg, count + (mark ? 1 : 0));
	if (subpat_names) {
		for (i = 0; i < count; i++) {
			if (subpat_names[i]) {
				add_assoc_stringl(&arg, subpat_names[i], &subject[offsets[i<<1]] , offsets[(i<<1)+1] - offsets[i<<1]);
			}
			add_next_index_stringl(&arg, &subject[offsets[i<<1]], offsets[(i<<1)+1] - offsets[i<<1]);
		}
	} else {
		for (i = 0; i < count; i++) {
			add_next_index_stringl(&arg, &subject[offsets[i<<1]], offsets[(i<<1)+1] - offsets[i<<1]);
		}
	}
	if (mark) {
		add_assoc_string(&arg, "MARK", (char *) mark);
	}

	fci->retval = &retval;
	fci->param_count = 1;
	fci->params = &arg;
	fci->no_separation = 0;

	if (zend_call_function(fci, fcc) == SUCCESS && Z_TYPE(retval) != IS_UNDEF) {
		if (EXPECTED(Z_TYPE(retval) == IS_STRING)) {
			result_str = Z_STR(retval);
		} else {
			result_str = zval_get_string_func(&retval);
			zval_ptr_dtor(&retval);
		}
	} else {
		if (!EG(exception)) {
			php_error_docref(NULL, E_WARNING, "Unable to call custom replacement function");
		}

		result_str = zend_string_init(&subject[offsets[0]], offsets[1] - offsets[0], 0);
	}

	zval_ptr_dtor(&arg);

	return result_str;
}
/* }}} */

/* {{{ php_pcre_replace
 */
PHPAPI zend_string *php_pcre_replace(zend_string *regex,
							  zend_string *subject_str,
							  char *subject, size_t subject_len,
							  zend_string *replace_str,
							  size_t limit, size_t *replace_count)
{
	pcre_cache_entry	*pce;			    /* Compiled regular expression */
	zend_string	 		*result;			/* Function result */

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex)) == NULL) {
		return NULL;
	}
	pce->refcount++;
	result = php_pcre_replace_impl(pce, subject_str, subject, subject_len, replace_str,
		limit, replace_count);
	pce->refcount--;

	return result;
}
/* }}} */

/* {{{ php_pcre_replace_impl() */
PHPAPI zend_string *php_pcre_replace_impl(pcre_cache_entry *pce, zend_string *subject_str, char *subject, size_t subject_len, zend_string *replace_str, size_t limit, size_t *replace_count)
{
	uint32_t		 options;			/* Execution options */
	int				 count;				/* Count of matched subpatterns */
	PCRE2_SIZE		*offsets;			/* Array of subpattern offsets */
	char 			**subpat_names;		/* Array for named subpatterns */
	uint32_t		 num_subpats;		/* Number of captured subpatterns */
	size_t			 new_len;			/* Length of needed storage */
	size_t			 alloc_len;			/* Actual allocated length */
	size_t			 match_len;			/* Length of the current match */
	int				 backref;			/* Backreference number */
	PCRE2_SIZE		 start_offset;		/* Where the new search starts */
	char			*walkbuf,			/* Location of current replacement in the result */
					*walk,				/* Used to walk the replacement string */
					*match,				/* The current match */
					*piece,				/* The current piece of subject */
					*replace_end,		/* End of replacement string */
					 walk_last;			/* Last walked character */
	size_t			result_len; 		/* Length of result */
	zend_string		*result;			/* Result of replacement */
	pcre2_match_data *match_data;

	/* Calculate the size of the offsets array, and allocate memory for it. */
	num_subpats = pce->capture_count + 1;

	/*
	 * Build a mapping from subpattern numbers to their names. We will
	 * allocate the table only if there are any named subpatterns.
	 */
	subpat_names = NULL;
	if (UNEXPECTED(pce->name_count > 0)) {
		subpat_names = make_subpats_table(num_subpats, pce);
		if (!subpat_names) {
			return NULL;
		}
	}

	alloc_len = 0;
	result = NULL;

	/* Initialize */
	match = NULL;
	start_offset = 0;
	result_len = 0;
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;

	if (!mdata_used && num_subpats <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		match_data = mdata;
	} else {
		match_data = pcre2_match_data_create_from_pattern(pce->re, gctx);
		if (!match_data) {
			PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
			if (subpat_names) {
				efree(subpat_names);
			}
			return NULL;
		}
	}

	options = (pce->compile_options & PCRE2_UTF) ? 0 : PCRE2_NO_UTF_CHECK;

	/* Execute the regular expression. */
#ifdef HAVE_PCRE_JIT_SUPPORT
	if ((pce->preg_options & PREG_JIT) && options) {
		count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	} else
#endif
	count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
			options, match_data, mctx);

	while (1) {
		piece = subject + start_offset;

		if (count >= 0 && limit > 0) {
			zend_bool simple_string;

			/* Check for too many substrings condition. */
			if (UNEXPECTED(count == 0)) {
				php_error_docref(NULL,E_NOTICE, "Matched, but too many substrings");
				count = num_subpats;
			}

matched:
			offsets = pcre2_get_ovector_pointer(match_data);

			if (UNEXPECTED(offsets[1] < offsets[0])) {
				PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
				if (result) {
					zend_string_release_ex(result, 0);
					result = NULL;
				}
				break;
			}

			if (replace_count) {
				++*replace_count;
			}

			/* Set the match location in subject */
			match = subject + offsets[0];

			new_len = result_len + offsets[0] - start_offset; /* part before the match */

			walk = ZSTR_VAL(replace_str);
			replace_end = walk + ZSTR_LEN(replace_str);
			walk_last = 0;
			simple_string = 1;
			while (walk < replace_end) {
				if ('\\' == *walk || '$' == *walk) {
					simple_string = 0;
					if (walk_last == '\\') {
						walk++;
						walk_last = 0;
						continue;
					}
					if (preg_get_backref(&walk, &backref)) {
						if (backref < count)
							new_len += offsets[(backref<<1)+1] - offsets[backref<<1];
						continue;
					}
				}
				new_len++;
				walk++;
				walk_last = walk[-1];
			}

			if (new_len >= alloc_len) {
				alloc_len = zend_safe_address_guarded(2, new_len, alloc_len);
				if (result == NULL) {
					result = zend_string_alloc(alloc_len, 0);
				} else {
					result = zend_string_extend(result, alloc_len, 0);
				}
			}

			if (match-piece > 0) {
				/* copy the part of the string before the match */
				memcpy(&ZSTR_VAL(result)[result_len], piece, match-piece);
				result_len += (match-piece);
			}

			if (simple_string) {
				/* copy replacement */
				memcpy(&ZSTR_VAL(result)[result_len], ZSTR_VAL(replace_str), ZSTR_LEN(replace_str)+1);
				result_len += ZSTR_LEN(replace_str);
			} else {
				/* copy replacement and backrefs */
				walkbuf = ZSTR_VAL(result) + result_len;

				walk = ZSTR_VAL(replace_str);
				walk_last = 0;
				while (walk < replace_end) {
					if ('\\' == *walk || '$' == *walk) {
						if (walk_last == '\\') {
							*(walkbuf-1) = *walk++;
							walk_last = 0;
							continue;
						}
						if (preg_get_backref(&walk, &backref)) {
							if (backref < count) {
								match_len = offsets[(backref<<1)+1] - offsets[backref<<1];
								memcpy(walkbuf, subject + offsets[backref<<1], match_len);
								walkbuf += match_len;
							}
							continue;
						}
					}
					*walkbuf++ = *walk++;
					walk_last = walk[-1];
				}
				*walkbuf = '\0';
				/* increment the result length by how much we've added to the string */
				result_len += (walkbuf - (ZSTR_VAL(result) + result_len));
			}

			limit--;

			/* Advance to the next piece. */
			start_offset = offsets[1];

			/* If we have matched an empty string, mimic what Perl's /g options does.
			   This turns out to be rather cunning. First we set PCRE2_NOTEMPTY_ATSTART and try
			   the match again at the same point. If this fails (picked up above) we
			   advance to the next character. */
			if (start_offset == offsets[0]) {
				count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
					PCRE2_NO_UTF_CHECK | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED, match_data, mctx);

				piece = subject + start_offset;
				if (count >= 0 && limit > 0) {
					goto matched;
				} else if (count == PCRE2_ERROR_NOMATCH || limit == 0) {
					/* If we previously set PCRE2_NOTEMPTY_ATSTART after a null match,
					   this is not necessarily the end. We need to advance
					   the start offset, and continue. Fudge the offset values
					   to achieve this, unless we're already at the end of the string. */
					if (start_offset < subject_len) {
						size_t unit_len = calculate_unit_length(pce, piece);

						start_offset += unit_len;
						memcpy(ZSTR_VAL(result) + result_len, piece, unit_len);
						result_len += unit_len;
					} else {
						goto not_matched;
					}
				} else {
					goto error;
				}
			}

		} else if (count == PCRE2_ERROR_NOMATCH || limit == 0) {
not_matched:
			if (!result && subject_str) {
				result = zend_string_copy(subject_str);
				break;
			}
			new_len = result_len + subject_len - start_offset;
			if (new_len >= alloc_len) {
				alloc_len = new_len; /* now we know exactly how long it is */
				if (NULL != result) {
					result = zend_string_realloc(result, alloc_len, 0);
				} else {
					result = zend_string_alloc(alloc_len, 0);
				}
			}
			/* stick that last bit of string on our output */
			memcpy(ZSTR_VAL(result) + result_len, piece, subject_len - start_offset);
			result_len += subject_len - start_offset;
			ZSTR_VAL(result)[result_len] = '\0';
			ZSTR_LEN(result) = result_len;
			break;
		} else {
error:
			pcre_handle_exec_error(count);
			if (result) {
				zend_string_release_ex(result, 0);
				result = NULL;
			}
			break;
		}

#ifdef HAVE_PCRE_JIT_SUPPORT
		if (pce->preg_options & PREG_JIT) {
			count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
		} else
#endif
		count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
	}
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}

	if (UNEXPECTED(subpat_names)) {
		efree(subpat_names);
	}

	return result;
}
/* }}} */

/* {{{ php_pcre_replace_func_impl() */
static zend_string *php_pcre_replace_func_impl(pcre_cache_entry *pce, zend_string *subject_str, char *subject, size_t subject_len, zend_fcall_info *fci, zend_fcall_info_cache *fcc, size_t limit, size_t *replace_count)
{
	uint32_t		 options;			/* Execution options */
	int				 count;				/* Count of matched subpatterns */
	PCRE2_SIZE		*offsets;			/* Array of subpattern offsets */
	char 			**subpat_names;		/* Array for named subpatterns */
	uint32_t		 num_subpats;		/* Number of captured subpatterns */
	size_t			 new_len;			/* Length of needed storage */
	size_t			 alloc_len;			/* Actual allocated length */
	PCRE2_SIZE		 start_offset;		/* Where the new search starts */
	char			*match,				/* The current match */
					*piece;				/* The current piece of subject */
	size_t			result_len; 		/* Length of result */
	zend_string		*result;			/* Result of replacement */
	zend_string     *eval_result;		/* Result of custom function */
	pcre2_match_data *match_data;
	zend_bool old_mdata_used;

	/* Calculate the size of the offsets array, and allocate memory for it. */
	num_subpats = pce->capture_count + 1;

	/*
	 * Build a mapping from subpattern numbers to their names. We will
	 * allocate the table only if there are any named subpatterns.
	 */
	subpat_names = NULL;
	if (UNEXPECTED(pce->name_count > 0)) {
		subpat_names = make_subpats_table(num_subpats, pce);
		if (!subpat_names) {
			return NULL;
		}
	}

	alloc_len = 0;
	result = NULL;

	/* Initialize */
	match = NULL;
	start_offset = 0;
	result_len = 0;
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;

	old_mdata_used = mdata_used;
	if (!old_mdata_used && num_subpats <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		mdata_used = 1;
		match_data = mdata;
	} else {
		match_data = pcre2_match_data_create_from_pattern(pce->re, gctx);
		if (!match_data) {
			PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
			if (subpat_names) {
				efree(subpat_names);
			}
			mdata_used = old_mdata_used;
			return NULL;
		}
	}

	options = (pce->compile_options & PCRE2_UTF) ? 0 : PCRE2_NO_UTF_CHECK;

	/* Execute the regular expression. */
#ifdef HAVE_PCRE_JIT_SUPPORT
	if ((pce->preg_options & PREG_JIT) && options) {
		count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	} else
#endif
	count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
			options, match_data, mctx);

	while (1) {
		piece = subject + start_offset;

		if (count >= 0 && limit) {
			/* Check for too many substrings condition. */
			if (UNEXPECTED(count == 0)) {
				php_error_docref(NULL,E_NOTICE, "Matched, but too many substrings");
				count = num_subpats;
			}

matched:
			offsets = pcre2_get_ovector_pointer(match_data);

			if (UNEXPECTED(offsets[1] < offsets[0])) {
				PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
				if (result) {
					zend_string_release_ex(result, 0);
					result = NULL;
				}
				break;
			}

			if (replace_count) {
				++*replace_count;
			}

			/* Set the match location in subject */
			match = subject + offsets[0];

			new_len = result_len + offsets[0] - start_offset; /* part before the match */

			/* Use custom function to get replacement string and its length. */
			eval_result = preg_do_repl_func(fci, fcc, subject, offsets, subpat_names, count,
				pcre2_get_mark(match_data));

			ZEND_ASSERT(eval_result);
			new_len = zend_safe_address_guarded(1, ZSTR_LEN(eval_result), new_len);
			if (new_len >= alloc_len) {
				alloc_len = zend_safe_address_guarded(2, new_len, alloc_len);
				if (result == NULL) {
					result = zend_string_alloc(alloc_len, 0);
				} else {
					result = zend_string_extend(result, alloc_len, 0);
				}
			}

			if (match-piece > 0) {
				/* copy the part of the string before the match */
				memcpy(ZSTR_VAL(result) + result_len, piece, match-piece);
				result_len += (match-piece);
			}

			/* If using custom function, copy result to the buffer and clean up. */
			memcpy(ZSTR_VAL(result) + result_len, ZSTR_VAL(eval_result), ZSTR_LEN(eval_result));
			result_len += ZSTR_LEN(eval_result);
			zend_string_release_ex(eval_result, 0);

			limit--;

			/* Advance to the next piece. */
			start_offset = offsets[1];

			/* If we have matched an empty string, mimic what Perl's /g options does.
			   This turns out to be rather cunning. First we set PCRE2_NOTEMPTY_ATSTART and try
			   the match again at the same point. If this fails (picked up above) we
			   advance to the next character. */
			if (start_offset == offsets[0]) {
				count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
					PCRE2_NO_UTF_CHECK | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED, match_data, mctx);

				piece = subject + start_offset;
				if (count >= 0 && limit) {
					goto matched;
				} else if (count == PCRE2_ERROR_NOMATCH || limit == 0) {
					/* If we previously set PCRE2_NOTEMPTY_ATSTART after a null match,
					   this is not necessarily the end. We need to advance
					   the start offset, and continue. Fudge the offset values
					   to achieve this, unless we're already at the end of the string. */
					if (start_offset < subject_len) {
						size_t unit_len = calculate_unit_length(pce, piece);

						start_offset += unit_len;
						memcpy(ZSTR_VAL(result) + result_len, piece, unit_len);
						result_len += unit_len;
					} else {
						goto not_matched;
					}
				} else {
					goto error;
				}
			}

		} else if (count == PCRE2_ERROR_NOMATCH || limit == 0) {
not_matched:
			if (!result && subject_str) {
				result = zend_string_copy(subject_str);
				break;
			}
			new_len = result_len + subject_len - start_offset;
			if (new_len >= alloc_len) {
				alloc_len = new_len; /* now we know exactly how long it is */
				if (NULL != result) {
					result = zend_string_realloc(result, alloc_len, 0);
				} else {
					result = zend_string_alloc(alloc_len, 0);
				}
			}
			/* stick that last bit of string on our output */
			memcpy(ZSTR_VAL(result) + result_len, piece, subject_len - start_offset);
			result_len += subject_len - start_offset;
			ZSTR_VAL(result)[result_len] = '\0';
			ZSTR_LEN(result) = result_len;
			break;
		} else {
error:
			pcre_handle_exec_error(count);
			if (result) {
				zend_string_release_ex(result, 0);
				result = NULL;
			}
			break;
		}
#ifdef HAVE_PCRE_JIT_SUPPORT
		if ((pce->preg_options & PREG_JIT)) {
			count = pcre2_jit_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
		} else
#endif
		count = pcre2_match(pce->re, (PCRE2_SPTR)subject, subject_len, start_offset,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	}
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}
	mdata_used = old_mdata_used;

	if (UNEXPECTED(subpat_names)) {
		efree(subpat_names);
	}

	return result;
}
/* }}} */

/* {{{ php_pcre_replace_func
 */
static zend_always_inline zend_string *php_pcre_replace_func(zend_string *regex,
							  zend_string *subject_str,
							  zend_fcall_info *fci, zend_fcall_info_cache *fcc,
							  size_t limit, size_t *replace_count)
{
	pcre_cache_entry	*pce;			    /* Compiled regular expression */
	zend_string	 		*result;			/* Function result */

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex)) == NULL) {
		return NULL;
	}
	pce->refcount++;
	result = php_pcre_replace_func_impl(pce, subject_str, ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), fci, fcc,
		limit, replace_count);
	pce->refcount--;

	return result;
}
/* }}} */

/* {{{ php_pcre_replace_array
 */
static zend_string *php_pcre_replace_array(HashTable *regex, zval *replace, zend_string *subject_str, size_t limit, size_t *replace_count)
{
	zval		*regex_entry;
	zend_string *result;
	zend_string *replace_str, *tmp_replace_str;

	if (Z_TYPE_P(replace) == IS_ARRAY) {
		uint32_t replace_idx = 0;
		HashTable *replace_ht = Z_ARRVAL_P(replace);

		/* For each entry in the regex array, get the entry */
		ZEND_HASH_FOREACH_VAL(regex, regex_entry) {
			/* Make sure we're dealing with strings. */
			zend_string *tmp_regex_str;
			zend_string *regex_str = zval_get_tmp_string(regex_entry, &tmp_regex_str);
			zval *zv;

			/* Get current entry */
			while (1) {
				if (replace_idx == replace_ht->nNumUsed) {
					replace_str = ZSTR_EMPTY_ALLOC();
					tmp_replace_str = NULL;
					break;
				}
				zv = &replace_ht->arData[replace_idx].val;
				replace_idx++;
				if (Z_TYPE_P(zv) != IS_UNDEF) {
					replace_str = zval_get_tmp_string(zv, &tmp_replace_str);
					break;
				}
			}

			/* Do the actual replacement and put the result back into subject_str
			   for further replacements. */
			result = php_pcre_replace(regex_str,
									  subject_str,
									  ZSTR_VAL(subject_str),
									  ZSTR_LEN(subject_str),
									  replace_str,
									  limit,
									  replace_count);
			zend_tmp_string_release(tmp_replace_str);
			zend_tmp_string_release(tmp_regex_str);
			zend_string_release_ex(subject_str, 0);
			subject_str = result;
			if (UNEXPECTED(result == NULL)) {
				break;
			}
		} ZEND_HASH_FOREACH_END();

	} else {
		replace_str = Z_STR_P(replace);

		/* For each entry in the regex array, get the entry */
		ZEND_HASH_FOREACH_VAL(regex, regex_entry) {
			/* Make sure we're dealing with strings. */
			zend_string *tmp_regex_str;
			zend_string *regex_str = zval_get_tmp_string(regex_entry, &tmp_regex_str);

			/* Do the actual replacement and put the result back into subject_str
			   for further replacements. */
			result = php_pcre_replace(regex_str,
									  subject_str,
									  ZSTR_VAL(subject_str),
									  ZSTR_LEN(subject_str),
									  replace_str,
									  limit,
									  replace_count);
			zend_tmp_string_release(tmp_regex_str);
			zend_string_release_ex(subject_str, 0);
			subject_str = result;

			if (UNEXPECTED(result == NULL)) {
				break;
			}
		} ZEND_HASH_FOREACH_END();
	}

	return subject_str;
}
/* }}} */

/* {{{ php_replace_in_subject
 */
static zend_always_inline zend_string *php_replace_in_subject(zval *regex, zval *replace, zval *subject, size_t limit, size_t *replace_count)
{
	zend_string *result;
	zend_string *subject_str = zval_get_string(subject);

	if (Z_TYPE_P(regex) != IS_ARRAY) {
		result = php_pcre_replace(Z_STR_P(regex),
								  subject_str,
								  ZSTR_VAL(subject_str),
								  ZSTR_LEN(subject_str),
								  Z_STR_P(replace),
								  limit,
								  replace_count);
		zend_string_release_ex(subject_str, 0);
	} else {
		result = php_pcre_replace_array(Z_ARRVAL_P(regex),
										replace,
										subject_str,
										limit,
										replace_count);
	}
	return result;
}
/* }}} */

/* {{{ php_replace_in_subject_func
 */
static zend_string *php_replace_in_subject_func(zval *regex, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *subject, size_t limit, size_t *replace_count)
{
	zend_string *result;
	zend_string	*subject_str = zval_get_string(subject);

	if (Z_TYPE_P(regex) != IS_ARRAY) {
		result = php_pcre_replace_func(Z_STR_P(regex),
								  subject_str,
								  fci, fcc,
								  limit,
								  replace_count);
		zend_string_release_ex(subject_str, 0);
		return result;
	} else {
		zval		*regex_entry;

		/* If regex is an array */

		/* For each entry in the regex array, get the entry */
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(regex), regex_entry) {
			/* Make sure we're dealing with strings. */
			zend_string *tmp_regex_str;
			zend_string *regex_str = zval_get_tmp_string(regex_entry, &tmp_regex_str);

			/* Do the actual replacement and put the result back into subject_str
			   for further replacements. */
			result = php_pcre_replace_func(regex_str,
										   subject_str,
										   fci, fcc,
										   limit,
										   replace_count);
			zend_tmp_string_release(tmp_regex_str);
			zend_string_release_ex(subject_str, 0);
			subject_str = result;
			if (UNEXPECTED(result == NULL)) {
				break;
			}
		} ZEND_HASH_FOREACH_END();

		return subject_str;
	}
}
/* }}} */

/* {{{ preg_replace_func_impl
 */
static size_t preg_replace_func_impl(zval *return_value, zval *regex, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *subject, zend_long limit_val)
{
	zend_string	*result;
	size_t replace_count = 0;

	if (Z_TYPE_P(regex) != IS_ARRAY) {
		convert_to_string_ex(regex);
	}

	if (Z_TYPE_P(subject) != IS_ARRAY) {
		result = php_replace_in_subject_func(regex, fci, fcc, subject, limit_val, &replace_count);
		if (result != NULL) {
			RETVAL_STR(result);
		} else {
			RETVAL_NULL();
		}
	} else {
		/* if subject is an array */
		zval		*subject_entry, zv;
		zend_string	*string_key;
		zend_ulong	 num_key;

		array_init_size(return_value, zend_hash_num_elements(Z_ARRVAL_P(subject)));

		/* For each subject entry, convert it to string, then perform replacement
		   and add the result to the return_value array. */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(subject), num_key, string_key, subject_entry) {
			result = php_replace_in_subject_func(regex, fci, fcc, subject_entry, limit_val, &replace_count);
			if (result != NULL) {
				/* Add to return array */
				ZVAL_STR(&zv, result);
				if (string_key) {
					zend_hash_add_new(Z_ARRVAL_P(return_value), string_key, &zv);
				} else {
					zend_hash_index_add_new(Z_ARRVAL_P(return_value), num_key, &zv);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}

	return replace_count;
}
/* }}} */

/* {{{ preg_replace_common
 */
static void preg_replace_common(INTERNAL_FUNCTION_PARAMETERS, int is_filter)
{
	zval *regex, *replace, *subject, *zcount = NULL;
	zend_long limit = -1;
	size_t replace_count = 0;
	zend_string	*result;
	size_t old_replace_count;

	/* Get function parameters and do error-checking. */
	ZEND_PARSE_PARAMETERS_START(3, 5)
		Z_PARAM_ZVAL(regex)
		Z_PARAM_ZVAL(replace)
		Z_PARAM_ZVAL(subject)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(limit)
		Z_PARAM_ZVAL_DEREF(zcount)
	ZEND_PARSE_PARAMETERS_END();

	if (Z_TYPE_P(replace) != IS_ARRAY) {
		convert_to_string_ex(replace);
		if (Z_TYPE_P(regex) != IS_ARRAY) {
			convert_to_string_ex(regex);
		}
	} else {
		if (Z_TYPE_P(regex) != IS_ARRAY) {
			php_error_docref(NULL, E_WARNING, "Parameter mismatch, pattern is a string while replacement is an array");
			RETURN_FALSE;
		}
	}

	if (Z_TYPE_P(subject) != IS_ARRAY) {
		old_replace_count = replace_count;
		result = php_replace_in_subject(regex,
										replace,
										subject,
										limit,
										&replace_count);
		if (result != NULL) {
			if (!is_filter || replace_count > old_replace_count) {
				RETVAL_STR(result);
			} else {
				zend_string_release_ex(result, 0);
				RETVAL_NULL();
			}
		} else {
			RETVAL_NULL();
		}
	} else {
		/* if subject is an array */
		zval		*subject_entry, zv;
		zend_string	*string_key;
		zend_ulong	 num_key;

		array_init_size(return_value, zend_hash_num_elements(Z_ARRVAL_P(subject)));

		/* For each subject entry, convert it to string, then perform replacement
		   and add the result to the return_value array. */
		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(subject), num_key, string_key, subject_entry) {
			old_replace_count = replace_count;
			result = php_replace_in_subject(regex,
											replace,
											subject_entry,
											limit,
											&replace_count);
			if (result != NULL) {
				if (!is_filter || replace_count > old_replace_count) {
					/* Add to return array */
					ZVAL_STR(&zv, result);
					if (string_key) {
						zend_hash_add_new(Z_ARRVAL_P(return_value), string_key, &zv);
					} else {
						zend_hash_index_add_new(Z_ARRVAL_P(return_value), num_key, &zv);
					}
				} else {
					zend_string_release_ex(result, 0);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}

	if (zcount) {
		zval_ptr_dtor(zcount);
		ZVAL_LONG(zcount, replace_count);
	}
}
/* }}} */

/* {{{ proto mixed preg_replace(mixed regex, mixed replace, mixed subject [, int limit [, int &count]])
   Perform Perl-style regular expression replacement. */
static PHP_FUNCTION(preg_replace)
{
	preg_replace_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto mixed preg_replace_callback(mixed regex, mixed callback, mixed subject [, int limit [, int &count]])
   Perform Perl-style regular expression replacement using replacement callback. */
static PHP_FUNCTION(preg_replace_callback)
{
	zval *regex, *replace, *subject, *zcount = NULL;
	zend_long limit = -1;
	size_t replace_count;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	/* Get function parameters and do error-checking. */
	ZEND_PARSE_PARAMETERS_START(3, 5)
		Z_PARAM_ZVAL(regex)
		Z_PARAM_ZVAL(replace)
		Z_PARAM_ZVAL(subject)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(limit)
		Z_PARAM_ZVAL_DEREF(zcount)
	ZEND_PARSE_PARAMETERS_END();

	if (!zend_is_callable_ex(replace, NULL, 0, NULL, &fcc, NULL)) {
		zend_string	*callback_name = zend_get_callable_name(replace);
		php_error_docref(NULL, E_WARNING, "Requires argument 2, '%s', to be a valid callback", ZSTR_VAL(callback_name));
		zend_string_release_ex(callback_name, 0);
		ZVAL_STR(return_value, zval_get_string(subject));
		return;
	}

	fci.size = sizeof(fci);
	fci.object = NULL;
	ZVAL_COPY_VALUE(&fci.function_name, replace);

	replace_count = preg_replace_func_impl(return_value, regex, &fci, &fcc, subject, limit);
	if (zcount) {
		zval_ptr_dtor(zcount);
		ZVAL_LONG(zcount, replace_count);
	}
}
/* }}} */

/* {{{ proto mixed preg_replace_callback_array(array pattern, mixed subject [, int limit [, int &count]])
   Perform Perl-style regular expression replacement using replacement callback. */
static PHP_FUNCTION(preg_replace_callback_array)
{
	zval regex, zv, *replace, *subject, *pattern, *zcount = NULL;
	zend_long limit = -1;
	zend_string *str_idx;
	size_t replace_count = 0;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	/* Get function parameters and do error-checking. */
	ZEND_PARSE_PARAMETERS_START(2, 4)
		Z_PARAM_ARRAY(pattern)
		Z_PARAM_ZVAL(subject)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(limit)
		Z_PARAM_ZVAL_DEREF(zcount)
	ZEND_PARSE_PARAMETERS_END();

	fci.size = sizeof(fci);
	fci.object = NULL;

	ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(pattern), str_idx, replace) {
		if (str_idx) {
			ZVAL_STR_COPY(&regex, str_idx);
		} else {
			php_error_docref(NULL, E_WARNING, "Delimiter must not be alphanumeric or backslash");
			zval_ptr_dtor(return_value);
			RETURN_NULL();
		}

		if (!zend_is_callable_ex(replace, NULL, 0, NULL, &fcc, NULL)) {
			zend_string *callback_name = zend_get_callable_name(replace);
			php_error_docref(NULL, E_WARNING, "'%s' is not a valid callback", ZSTR_VAL(callback_name));
			zend_string_release_ex(callback_name, 0);
			zval_ptr_dtor(&regex);
			zval_ptr_dtor(return_value);
			ZVAL_COPY(return_value, subject);
			return;
		}

		ZVAL_COPY_VALUE(&fci.function_name, replace);

		replace_count += preg_replace_func_impl(&zv, &regex, &fci, &fcc, subject, limit);
		if (subject != return_value) {
			subject = return_value;
		} else {
			zval_ptr_dtor(return_value);
		}

		zval_ptr_dtor(&regex);

		ZVAL_COPY_VALUE(return_value, &zv);

		if (UNEXPECTED(EG(exception))) {
			zval_ptr_dtor(return_value);
			RETURN_NULL();
		}
	} ZEND_HASH_FOREACH_END();

	if (zcount) {
		zval_ptr_dtor(zcount);
		ZVAL_LONG(zcount, replace_count);
	}
}
/* }}} */

/* {{{ proto mixed preg_filter(mixed regex, mixed replace, mixed subject [, int limit [, int &count]])
   Perform Perl-style regular expression replacement and only return matches. */
static PHP_FUNCTION(preg_filter)
{
	preg_replace_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto array preg_split(string pattern, string subject [, int limit [, int flags]])
   Split string into an array using a perl-style regular expression as a delimiter */
static PHP_FUNCTION(preg_split)
{
	zend_string			*regex;			/* Regular expression */
	zend_string			*subject;		/* String to match against */
	zend_long			 limit_val = -1;/* Integer value of limit */
	zend_long			 flags = 0;		/* Match control flags */
	pcre_cache_entry	*pce;			/* Compiled regular expression */

	/* Get function parameters and do error checking */
	ZEND_PARSE_PARAMETERS_START(2, 4)
		Z_PARAM_STR(regex)
		Z_PARAM_STR(subject)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(limit_val)
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex)) == NULL) {
		RETURN_FALSE;
	}

	pce->refcount++;
	php_pcre_split_impl(pce, subject, return_value, limit_val, flags);
	pce->refcount--;
}
/* }}} */

/* {{{ php_pcre_split
 */
PHPAPI void php_pcre_split_impl(pcre_cache_entry *pce, zend_string *subject_str, zval *return_value,
	zend_long limit_val, zend_long flags)
{
	PCRE2_SIZE		*offsets;			/* Array of subpattern offsets */
	uint32_t		 options;			/* Execution options */
	int				 count;				/* Count of matched subpatterns */
	PCRE2_SIZE		 start_offset;		/* Where the new search starts */
	PCRE2_SIZE		 next_offset;		/* End of the last delimiter match + 1 */
	char			*last_match;		/* Location of last match */
	uint32_t		 no_empty;			/* If NO_EMPTY flag is set */
	uint32_t		 delim_capture; 	/* If delimiters should be captured */
	uint32_t		 offset_capture;	/* If offsets should be captured */
	uint32_t		 num_subpats;		/* Number of captured subpatterns */
	zval			 tmp;
	pcre2_match_data *match_data;

	no_empty = flags & PREG_SPLIT_NO_EMPTY;
	delim_capture = flags & PREG_SPLIT_DELIM_CAPTURE;
	offset_capture = flags & PREG_SPLIT_OFFSET_CAPTURE;

	/* Initialize return value */
	array_init(return_value);

	/* Calculate the size of the offsets array, and allocate memory for it. */
	num_subpats = pce->capture_count + 1;

	/* Start at the beginning of the string */
	start_offset = 0;
	next_offset = 0;
	last_match = ZSTR_VAL(subject_str);
	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;


	if (limit_val == -1) {
		/* pass */
	} else if (limit_val == 0) {
		limit_val = -1;
	} else if (limit_val <= 1) {
		goto last;
	}

	if (!mdata_used && num_subpats <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		match_data = mdata;
	} else {
		match_data = pcre2_match_data_create_from_pattern(pce->re, gctx);
		if (!match_data) {
			PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
			return;
		}
	}

	options = (pce->compile_options & PCRE2_UTF) ? 0 : PCRE2_NO_UTF_CHECK;

#ifdef HAVE_PCRE_JIT_SUPPORT
	if ((pce->preg_options & PREG_JIT) && options) {
		count = pcre2_jit_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), start_offset,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	} else
#endif
	count = pcre2_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), start_offset,
			options, match_data, mctx);

	while (1) {
		/* If something matched */
		if (count >= 0) {
			/* Check for too many substrings condition. */
			if (UNEXPECTED(count == 0)) {
				php_error_docref(NULL,E_NOTICE, "Matched, but too many substrings");
				count = num_subpats;
			}

matched:
			offsets = pcre2_get_ovector_pointer(match_data);

			if (UNEXPECTED(offsets[1] < offsets[0])) {
				PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
				break;
			}

			if (!no_empty || &ZSTR_VAL(subject_str)[offsets[0]] != last_match) {

				if (offset_capture) {
					/* Add (match, offset) pair to the return value */
					add_offset_pair(return_value, last_match, (&ZSTR_VAL(subject_str)[offsets[0]]-last_match), next_offset, NULL, 0);
				} else {
					/* Add the piece to the return value */
					ZVAL_STRINGL(&tmp, last_match, &ZSTR_VAL(subject_str)[offsets[0]]-last_match);
					zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
				}

				/* One less left to do */
				if (limit_val != -1)
					limit_val--;
			}

			last_match = &ZSTR_VAL(subject_str)[offsets[1]];
			next_offset = offsets[1];

			if (delim_capture) {
				size_t i, match_len;
				for (i = 1; i < count; i++) {
					match_len = offsets[(i<<1)+1] - offsets[i<<1];
					/* If we have matched a delimiter */
					if (!no_empty || match_len > 0) {
						if (offset_capture) {
							add_offset_pair(return_value, &ZSTR_VAL(subject_str)[offsets[i<<1]], match_len, offsets[i<<1], NULL, 0);
						} else {
							ZVAL_STRINGL(&tmp, &ZSTR_VAL(subject_str)[offsets[i<<1]], match_len);
							zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
						}
					}
				}
			}

			/* Advance to the position right after the last full match */
			start_offset = offsets[1];

			/* If we have matched an empty string, mimic what Perl's /g options does.
			   This turns out to be rather cunning. First we set PCRE2_NOTEMPTY_ATSTART and try
			   the match again at the same point. If this fails (picked up above) we
			   advance to the next character. */
			if (start_offset == offsets[0]) {
				count = pcre2_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), start_offset,
					PCRE2_NO_UTF_CHECK | PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED, match_data, mctx);
				if (count >= 0) {
					goto matched;
				} else if (count == PCRE2_ERROR_NOMATCH) {
					/* If we previously set PCRE2_NOTEMPTY_ATSTART after a null match,
					   this is not necessarily the end. We need to advance
					   the start offset, and continue. Fudge the offset values
					   to achieve this, unless we're already at the end of the string. */
					if (start_offset < ZSTR_LEN(subject_str)) {
						start_offset += calculate_unit_length(pce, ZSTR_VAL(subject_str) + start_offset);
					} else {
						break;
					}
				} else {
					goto error;
				}
			}

		} else if (count == PCRE2_ERROR_NOMATCH) {
			break;
		} else {
error:
			pcre_handle_exec_error(count);
			break;
		}

		/* Get next piece if no limit or limit not yet reached and something matched*/
		if (limit_val != -1 && limit_val <= 1) {
			break;
		}

#ifdef HAVE_PCRE_JIT_SUPPORT
		if (pce->preg_options & PREG_JIT) {
			count = pcre2_jit_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), start_offset,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
		} else
#endif
		count = pcre2_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), start_offset,
				PCRE2_NO_UTF_CHECK, match_data, mctx);
	}
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}

last:
	start_offset = (last_match - ZSTR_VAL(subject_str)); /* the offset might have been incremented, but without further successful matches */

	if (!no_empty || start_offset < ZSTR_LEN(subject_str)) {
		if (offset_capture) {
			/* Add the last (match, offset) pair to the return value */
			add_offset_pair(return_value, &ZSTR_VAL(subject_str)[start_offset], ZSTR_LEN(subject_str) - start_offset, start_offset, NULL, 0);
		} else {
			/* Add the last piece to the return value */
			if (last_match == ZSTR_VAL(subject_str)) {
				ZVAL_STR_COPY(&tmp, subject_str);
			} else {
				ZVAL_STRINGL(&tmp, last_match, ZSTR_VAL(subject_str) + ZSTR_LEN(subject_str) - last_match);
			}
			zend_hash_next_index_insert_new(Z_ARRVAL_P(return_value), &tmp);
		}
	}
}
/* }}} */

/* {{{ proto string preg_quote(string str [, string delim_char])
   Quote regular expression characters plus an optional character */
static PHP_FUNCTION(preg_quote)
{
	zend_string *str;       		/* Input string argument */
	zend_string	*delim = NULL;		/* Additional delimiter argument */
	char		*in_str;			/* Input string */
	char		*in_str_end;    	/* End of the input string */
	zend_string	*out_str;			/* Output string with quoted characters */
	size_t       extra_len;         /* Number of additional characters */
	char 		*p,					/* Iterator for input string */
				*q,					/* Iterator for output string */
				 delim_char = '\0',	/* Delimiter character to be quoted */
				 c;					/* Current character */

	/* Get the arguments and check for errors */
	ZEND_PARSE_PARAMETERS_START(1, 2)
		Z_PARAM_STR(str)
		Z_PARAM_OPTIONAL
		Z_PARAM_STR(delim)
	ZEND_PARSE_PARAMETERS_END();

	/* Nothing to do if we got an empty string */
	if (ZSTR_LEN(str) == 0) {
		RETURN_EMPTY_STRING();
	}

	in_str = ZSTR_VAL(str);
	in_str_end = in_str + ZSTR_LEN(str);

	if (delim) {
		delim_char = ZSTR_VAL(delim)[0];
	}

	/* Go through the string and quote necessary characters */
	extra_len = 0;
	p = in_str;
	do {
		c = *p;
		switch(c) {
			case '.':
			case '\\':
			case '+':
			case '*':
			case '?':
			case '[':
			case '^':
			case ']':
			case '$':
			case '(':
			case ')':
			case '{':
			case '}':
			case '=':
			case '!':
			case '>':
			case '<':
			case '|':
			case ':':
			case '-':
			case '#':
				extra_len++;
				break;

			case '\0':
				extra_len+=3;
				break;

			default:
				if (c == delim_char) {
					extra_len++;
				}
				break;
		}
		p++;
	} while (p != in_str_end);

	if (extra_len == 0) {
		RETURN_STR_COPY(str);
	}

	/* Allocate enough memory so that even if each character
	   is quoted, we won't run out of room */
	out_str = zend_string_safe_alloc(1, ZSTR_LEN(str), extra_len, 0);
	q = ZSTR_VAL(out_str);
	p = in_str;

	do {
		c = *p;
		switch(c) {
			case '.':
			case '\\':
			case '+':
			case '*':
			case '?':
			case '[':
			case '^':
			case ']':
			case '$':
			case '(':
			case ')':
			case '{':
			case '}':
			case '=':
			case '!':
			case '>':
			case '<':
			case '|':
			case ':':
			case '-':
			case '#':
				*q++ = '\\';
				*q++ = c;
				break;

			case '\0':
				*q++ = '\\';
				*q++ = '0';
				*q++ = '0';
				*q++ = '0';
				break;

			default:
				if (c == delim_char) {
					*q++ = '\\';
				}
				*q++ = c;
				break;
		}
		p++;
	} while (p != in_str_end);
	*q = '\0';

	RETURN_NEW_STR(out_str);
}
/* }}} */

/* {{{ proto array preg_grep(string regex, array input [, int flags])
   Searches array and returns entries which match regex */
static PHP_FUNCTION(preg_grep)
{
	zend_string			*regex;			/* Regular expression */
	zval				*input;			/* Input array */
	zend_long			 flags = 0;		/* Match control flags */
	pcre_cache_entry	*pce;			/* Compiled regular expression */

	/* Get arguments and do error checking */
	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_STR(regex)
		Z_PARAM_ARRAY(input)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();

	/* Compile regex or get it from cache. */
	if ((pce = pcre_get_compiled_regex_cache(regex)) == NULL) {
		RETURN_FALSE;
	}

	pce->refcount++;
	php_pcre_grep_impl(pce, input, return_value, flags);
	pce->refcount--;
}
/* }}} */

PHPAPI void  php_pcre_grep_impl(pcre_cache_entry *pce, zval *input, zval *return_value, zend_long flags) /* {{{ */
{
	zval            *entry;             /* An entry in the input array */
	uint32_t		 num_subpats;		/* Number of captured subpatterns */
	int				 count;				/* Count of matched subpatterns */
	uint32_t		 options;			/* Execution options */
	zend_string		*string_key;
	zend_ulong		 num_key;
	zend_bool		 invert;			/* Whether to return non-matching
										   entries */
	pcre2_match_data *match_data;
	invert = flags & PREG_GREP_INVERT ? 1 : 0;

	/* Calculate the size of the offsets array, and allocate memory for it. */
	num_subpats = pce->capture_count + 1;

	/* Initialize return array */
	array_init(return_value);

	PCRE_G(error_code) = PHP_PCRE_NO_ERROR;

	if (!mdata_used && num_subpats <= PHP_PCRE_PREALLOC_MDATA_SIZE) {
		match_data = mdata;
	} else {
		match_data = pcre2_match_data_create_from_pattern(pce->re, gctx);
		if (!match_data) {
			PCRE_G(error_code) = PHP_PCRE_INTERNAL_ERROR;
			return;
		}
	}

	options = (pce->compile_options & PCRE2_UTF) ? 0 : PCRE2_NO_UTF_CHECK;

	/* Go through the input array */
	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(input), num_key, string_key, entry) {
		zend_string *tmp_subject_str;
		zend_string *subject_str = zval_get_tmp_string(entry, &tmp_subject_str);

		/* Perform the match */
#ifdef HAVE_PCRE_JIT_SUPPORT
		if ((pce->preg_options & PREG_JIT) && options) {
			count = pcre2_jit_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), 0,
					PCRE2_NO_UTF_CHECK, match_data, mctx);
		} else
#endif
		count = pcre2_match(pce->re, (PCRE2_SPTR)ZSTR_VAL(subject_str), ZSTR_LEN(subject_str), 0,
				options, match_data, mctx);

		/* If the entry fits our requirements */
		if (count >= 0) {
			/* Check for too many substrings condition. */
			if (UNEXPECTED(count == 0)) {
				php_error_docref(NULL, E_NOTICE, "Matched, but too many substrings");
			}
			if (!invert) {
				Z_TRY_ADDREF_P(entry);

				/* Add to return array */
				if (string_key) {
					zend_hash_update(Z_ARRVAL_P(return_value), string_key, entry);
				} else {
					zend_hash_index_update(Z_ARRVAL_P(return_value), num_key, entry);
				}
			}
		} else if (count == PCRE2_ERROR_NOMATCH) {
			if (invert) {
				Z_TRY_ADDREF_P(entry);

				/* Add to return array */
				if (string_key) {
					zend_hash_update(Z_ARRVAL_P(return_value), string_key, entry);
				} else {
					zend_hash_index_update(Z_ARRVAL_P(return_value), num_key, entry);
				}
			}
		} else {
			pcre_handle_exec_error(count);
			zend_tmp_string_release(tmp_subject_str);
			break;
		}

		zend_tmp_string_release(tmp_subject_str);
	} ZEND_HASH_FOREACH_END();
	if (match_data != mdata) {
		pcre2_match_data_free(match_data);
	}
}
/* }}} */

/* {{{ proto int preg_last_error()
   Returns the error code of the last regexp execution. */
static PHP_FUNCTION(preg_last_error)
{
	ZEND_PARSE_PARAMETERS_START(0, 0)
	ZEND_PARSE_PARAMETERS_END();

	RETURN_LONG(PCRE_G(error_code));
}
/* }}} */

/* {{{ module definition structures */

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_match, 0, 0, 2)
    ZEND_ARG_INFO(0, pattern)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(1, subpatterns) /* array */
    ZEND_ARG_INFO(0, flags)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_match_all, 0, 0, 2)
    ZEND_ARG_INFO(0, pattern)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(1, subpatterns) /* array */
    ZEND_ARG_INFO(0, flags)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_replace, 0, 0, 3)
    ZEND_ARG_INFO(0, regex)
    ZEND_ARG_INFO(0, replace)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(1, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_replace_callback, 0, 0, 3)
    ZEND_ARG_INFO(0, regex)
    ZEND_ARG_INFO(0, callback)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(1, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_replace_callback_array, 0, 0, 2)
    ZEND_ARG_INFO(0, pattern)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(1, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_split, 0, 0, 2)
    ZEND_ARG_INFO(0, pattern)
    ZEND_ARG_INFO(0, subject)
    ZEND_ARG_INFO(0, limit)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_quote, 0, 0, 1)
    ZEND_ARG_INFO(0, str)
    ZEND_ARG_INFO(0, delim_char)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_preg_grep, 0, 0, 2)
    ZEND_ARG_INFO(0, regex)
    ZEND_ARG_INFO(0, input) /* array */
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_preg_last_error, 0)
ZEND_END_ARG_INFO()
/* }}} */

static const zend_function_entry pcre_functions[] = {
	PHP_FE(preg_match,					arginfo_preg_match)
	PHP_FE(preg_match_all,				arginfo_preg_match_all)
	PHP_FE(preg_replace,				arginfo_preg_replace)
	PHP_FE(preg_replace_callback,		arginfo_preg_replace_callback)
	PHP_FE(preg_replace_callback_array,	arginfo_preg_replace_callback_array)
	PHP_FE(preg_filter,					arginfo_preg_replace)
	PHP_FE(preg_split,					arginfo_preg_split)
	PHP_FE(preg_quote,					arginfo_preg_quote)
	PHP_FE(preg_grep,					arginfo_preg_grep)
	PHP_FE(preg_last_error,				arginfo_preg_last_error)
	PHP_FE_END
};

zend_module_entry pcre_module_entry = {
	STANDARD_MODULE_HEADER,
   "pcre",
	pcre_functions,
	PHP_MINIT(pcre),
	PHP_MSHUTDOWN(pcre),
#ifdef HAVE_PCRE_JIT_SUPPORT
	PHP_RINIT(pcre),
#else
	NULL,
#endif
	NULL,
	PHP_MINFO(pcre),
	PHP_PCRE_VERSION,
	PHP_MODULE_GLOBALS(pcre),
	PHP_GINIT(pcre),
	PHP_GSHUTDOWN(pcre),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_PCRE
ZEND_GET_MODULE(pcre)
#endif

/* }}} */

PHPAPI pcre2_match_context *php_pcre_mctx(void)
{/*{{{*/
	return mctx;
}/*}}}*/

PHPAPI pcre2_general_context *php_pcre_gctx(void)
{/*{{{*/
	return gctx;
}/*}}}*/

PHPAPI pcre2_compile_context *php_pcre_cctx(void)
{/*{{{*/
	return cctx;
}/*}}}*/

PHPAPI void php_pcre_pce_incref(pcre_cache_entry *pce)
{/*{{{*/
	assert(NULL != pce);
	pce->refcount++;
}/*}}}*/

PHPAPI void php_pcre_pce_decref(pcre_cache_entry *pce)
{/*{{{*/
	assert(NULL != pce);
	assert(0 != pce->refcount);
	pce->refcount--;
}/*}}}*/

PHPAPI pcre2_code *php_pcre_pce_re(pcre_cache_entry *pce)
{/*{{{*/
	assert(NULL != pce);
	return pce->re;
}/*}}}*/

#endif /* HAVE_PCRE || HAVE_BUNDLED_PCRE */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
