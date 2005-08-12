/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2005 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Stig S�ther Bakken <ssb@php.net>                            |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* Synced with php 3.0 revision 1.193 1999-06-16 [ssb] */

#include <stdio.h>
#include "php.h"
#include "reg.h"
#include "php_rand.h"
#include "php_string.h"
#include "php_variables.h"
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif
#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif
#ifdef HAVE_MONETARY_H
# include <monetary.h>
#endif
#ifdef HAVE_LIBINTL
# include <libintl.h> /* For LC_MESSAGES */
#endif

#include <math.h>

#include "scanf.h"
#include "zend_API.h"
#include "zend_execute.h"
#include "php_globals.h"
#include "basic_functions.h"
#include "php_smart_str.h"
#ifdef ZTS
#include "TSRM.h"
#endif

#include "unicode/uchar.h"

#define STR_PAD_LEFT			0
#define STR_PAD_RIGHT			1
#define STR_PAD_BOTH			2
#define PHP_PATHINFO_DIRNAME 	1
#define PHP_PATHINFO_BASENAME 	2
#define PHP_PATHINFO_EXTENSION 	4
#define PHP_PATHINFO_ALL	(PHP_PATHINFO_DIRNAME | PHP_PATHINFO_BASENAME | PHP_PATHINFO_EXTENSION)

#define STR_STRSPN				0
#define STR_STRCSPN				1

/* {{{ register_string_constants
 */
void register_string_constants(INIT_FUNC_ARGS)
{
	REGISTER_LONG_CONSTANT("STR_PAD_LEFT", STR_PAD_LEFT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("STR_PAD_RIGHT", STR_PAD_RIGHT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("STR_PAD_BOTH", STR_PAD_BOTH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PATHINFO_DIRNAME", PHP_PATHINFO_DIRNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PATHINFO_BASENAME", PHP_PATHINFO_BASENAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("PATHINFO_EXTENSION", PHP_PATHINFO_EXTENSION, CONST_CS | CONST_PERSISTENT);

#ifdef HAVE_LOCALECONV
	/* If last members of struct lconv equal CHAR_MAX, no grouping is done */	

/* This is bad, but since we are going to be hardcoding in the POSIX stuff anyway... */
# ifndef HAVE_LIMITS_H
# define CHAR_MAX 127
# endif

	REGISTER_LONG_CONSTANT("CHAR_MAX", CHAR_MAX, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef HAVE_LOCALE_H
	REGISTER_LONG_CONSTANT("LC_CTYPE", LC_CTYPE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LC_NUMERIC", LC_NUMERIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LC_TIME", LC_TIME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LC_COLLATE", LC_COLLATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LC_MONETARY", LC_MONETARY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("LC_ALL", LC_ALL, CONST_CS | CONST_PERSISTENT);
# ifdef LC_MESSAGES
	REGISTER_LONG_CONSTANT("LC_MESSAGES", LC_MESSAGES, CONST_CS | CONST_PERSISTENT);
# endif
#endif
	
}
/* }}} */

int php_tag_find(char *tag, int len, char *set);

/* this is read-only, so it's ok */
static char hexconvtab[] = "0123456789abcdef";

/* localeconv mutex */
#ifdef ZTS
static MUTEX_T locale_mutex = NULL;
#endif

/* {{{ php_bin2hex
 */
static char *php_bin2hex(const unsigned char *old, const size_t oldlen, size_t *newlen)
{
	register unsigned char *result = NULL;
	size_t i, j;

	result = (char *) safe_emalloc(oldlen * 2, sizeof(char), 1);
	
	for (i = j = 0; i < oldlen; i++) {
		result[j++] = hexconvtab[old[i] >> 4];
		result[j++] = hexconvtab[old[i] & 15];
	}
	result[j] = '\0';

	if (newlen) 
		*newlen = oldlen * 2 * sizeof(char);

	return result;
}
/* }}} */

#ifdef HAVE_LOCALECONV
/* {{{ localeconv_r
 * glibc's localeconv is not reentrant, so lets make it so ... sorta */
PHPAPI struct lconv *localeconv_r(struct lconv *out)
{
	struct lconv *res;

# ifdef ZTS
	tsrm_mutex_lock( locale_mutex );
# endif

	/* localeconv doesn't return an error condition */
	res = localeconv();

	*out = *res;

# ifdef ZTS
	tsrm_mutex_unlock( locale_mutex );
# endif

	return out;
}
/* }}} */

# ifdef ZTS
/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(localeconv)
{
	locale_mutex = tsrm_mutex_alloc();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(localeconv)
{
	tsrm_mutex_free( locale_mutex );
	locale_mutex = NULL;
	return SUCCESS;
}
/* }}} */
# endif
#endif

/* {{{ proto string bin2hex(string data)
   Converts the binary representation of data to hex */
PHP_FUNCTION(bin2hex)
{
	zval **data;
	char *result;
	size_t newlen;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &data) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(data);

	result = php_bin2hex(Z_STRVAL_PP(data), Z_STRLEN_PP(data), &newlen);
	
	if (!result) {
		RETURN_FALSE;
	}

	RETURN_STRINGL(result, newlen, 0);
}
/* }}} */

static void php_spn_common_handler(INTERNAL_FUNCTION_PARAMETERS, int behavior)
{
	char *s11, *s22;
	int len1, len2;
	long start, len;
	
	start = 0;
	len = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|ll", &s11, &len1,
				&s22, &len2, &start, &len) == FAILURE) {
		return;
	}
	
	if (ZEND_NUM_ARGS() < 4) {
		len = len1;
	}
	
	/* look at substr() function for more information */
	
	if (start < 0) {
		start += len1;
		if (start < 0) {
			start = 0;
		}
	} else if (start > len1) {
		RETURN_FALSE;
	}
	
	if (len < 0) {
		len += (len1 - start);
		if (len < 0) {
			len = 0;
		}
	}
	
	if (((unsigned) start + (unsigned) len) > len1) {
		len = len1 - start;
	}

	if (behavior == STR_STRSPN) {
		RETURN_LONG(php_strspn(s11 + start /*str1_start*/,
						s22 /*str2_start*/,
						s11 + start + len /*str1_end*/,
						s22 + len2 /*str2_end*/));
	} else if (behavior == STR_STRCSPN) {
		RETURN_LONG(php_strcspn(s11 + start /*str1_start*/,
						s22 /*str2_start*/,
						s11 + start + len /*str1_end*/,
						s22 + len2 /*str2_end*/));
	}
	
}

/* {{{ proto int strspn(string str, string mask [, start [, len]])
   Finds length of initial segment consisting entirely of characters found in mask. If start or/and length is provided works like strspn(substr($s,$start,$len),$good_chars) */
PHP_FUNCTION(strspn)
{
	php_spn_common_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, STR_STRSPN);
}
/* }}} */

/* {{{ proto int strcspn(string str, string mask [, start [, len]])
   Finds length of initial segment consisting entirely of characters not found in mask. If start or/and length is provide works like strcspn(substr($s,$start,$len),$bad_chars) */
PHP_FUNCTION(strcspn)
{
	php_spn_common_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, STR_STRCSPN);
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(nl_langinfo) */
#if HAVE_NL_LANGINFO
PHP_MINIT_FUNCTION(nl_langinfo)
{
#define REGISTER_NL_LANGINFO_CONSTANT(x)	REGISTER_LONG_CONSTANT(#x, x, CONST_CS | CONST_PERSISTENT)
#ifdef ABDAY_1
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_1);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_2);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_3);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_4);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_5);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_6);
	REGISTER_NL_LANGINFO_CONSTANT(ABDAY_7);
#endif
#ifdef DAY_1
	REGISTER_NL_LANGINFO_CONSTANT(DAY_1);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_2);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_3);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_4);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_5);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_6);
	REGISTER_NL_LANGINFO_CONSTANT(DAY_7);
#endif
#ifdef ABMON_1
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_1);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_2);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_3);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_4);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_5);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_6);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_7);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_8);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_9);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_10);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_11);
	REGISTER_NL_LANGINFO_CONSTANT(ABMON_12);
#endif
#ifdef MON_1
	REGISTER_NL_LANGINFO_CONSTANT(MON_1);
	REGISTER_NL_LANGINFO_CONSTANT(MON_2);
	REGISTER_NL_LANGINFO_CONSTANT(MON_3);
	REGISTER_NL_LANGINFO_CONSTANT(MON_4);
	REGISTER_NL_LANGINFO_CONSTANT(MON_5);
	REGISTER_NL_LANGINFO_CONSTANT(MON_6);
	REGISTER_NL_LANGINFO_CONSTANT(MON_7);
	REGISTER_NL_LANGINFO_CONSTANT(MON_8);
	REGISTER_NL_LANGINFO_CONSTANT(MON_9);
	REGISTER_NL_LANGINFO_CONSTANT(MON_10);
	REGISTER_NL_LANGINFO_CONSTANT(MON_11);
	REGISTER_NL_LANGINFO_CONSTANT(MON_12);
#endif
#ifdef AM_STR
	REGISTER_NL_LANGINFO_CONSTANT(AM_STR);
#endif
#ifdef PM_STR
	REGISTER_NL_LANGINFO_CONSTANT(PM_STR);
#endif
#ifdef D_T_FMT
	REGISTER_NL_LANGINFO_CONSTANT(D_T_FMT);
#endif
#ifdef D_FMT
	REGISTER_NL_LANGINFO_CONSTANT(D_FMT);
#endif
#ifdef T_FMT
	REGISTER_NL_LANGINFO_CONSTANT(T_FMT);
#endif
#ifdef T_FMT_AMPM
	REGISTER_NL_LANGINFO_CONSTANT(T_FMT_AMPM);
#endif
#ifdef ERA
	REGISTER_NL_LANGINFO_CONSTANT(ERA);
#endif
#ifdef ERA_YEAR
	REGISTER_NL_LANGINFO_CONSTANT(ERA_YEAR);
#endif
#ifdef ERA_D_T_FMT
	REGISTER_NL_LANGINFO_CONSTANT(ERA_D_T_FMT);
#endif
#ifdef ERA_D_FMT
	REGISTER_NL_LANGINFO_CONSTANT(ERA_D_FMT);
#endif
#ifdef ERA_T_FMT
	REGISTER_NL_LANGINFO_CONSTANT(ERA_T_FMT);
#endif
#ifdef ALT_DIGITS
	REGISTER_NL_LANGINFO_CONSTANT(ALT_DIGITS);
#endif
#ifdef INT_CURR_SYMBOL
	REGISTER_NL_LANGINFO_CONSTANT(INT_CURR_SYMBOL);
#endif
#ifdef CURRENCY_SYMBOL
	REGISTER_NL_LANGINFO_CONSTANT(CURRENCY_SYMBOL);
#endif
#ifdef CRNCYSTR
	REGISTER_NL_LANGINFO_CONSTANT(CRNCYSTR);
#endif
#ifdef MON_DECIMAL_POINT
	REGISTER_NL_LANGINFO_CONSTANT(MON_DECIMAL_POINT);
#endif
#ifdef MON_THOUSANDS_SEP
	REGISTER_NL_LANGINFO_CONSTANT(MON_THOUSANDS_SEP);
#endif
#ifdef MON_GROUPING
	REGISTER_NL_LANGINFO_CONSTANT(MON_GROUPING);
#endif
#ifdef POSITIVE_SIGN
	REGISTER_NL_LANGINFO_CONSTANT(POSITIVE_SIGN);
#endif
#ifdef NEGATIVE_SIGN
	REGISTER_NL_LANGINFO_CONSTANT(NEGATIVE_SIGN);
#endif
#ifdef INT_FRAC_DIGITS
	REGISTER_NL_LANGINFO_CONSTANT(INT_FRAC_DIGITS);
#endif
#ifdef FRAC_DIGITS
	REGISTER_NL_LANGINFO_CONSTANT(FRAC_DIGITS);
#endif
#ifdef P_CS_PRECEDES
	REGISTER_NL_LANGINFO_CONSTANT(P_CS_PRECEDES);
#endif
#ifdef P_SEP_BY_SPACE
	REGISTER_NL_LANGINFO_CONSTANT(P_SEP_BY_SPACE);
#endif
#ifdef N_CS_PRECEDES
	REGISTER_NL_LANGINFO_CONSTANT(N_CS_PRECEDES);
#endif
#ifdef N_SEP_BY_SPACE
	REGISTER_NL_LANGINFO_CONSTANT(N_SEP_BY_SPACE);
#endif
#ifdef P_SIGN_POSN
	REGISTER_NL_LANGINFO_CONSTANT(P_SIGN_POSN);
#endif
#ifdef N_SIGN_POSN
	REGISTER_NL_LANGINFO_CONSTANT(N_SIGN_POSN);
#endif
#ifdef DECIMAL_POINT
	REGISTER_NL_LANGINFO_CONSTANT(DECIMAL_POINT);
#endif
#ifdef RADIXCHAR
	REGISTER_NL_LANGINFO_CONSTANT(RADIXCHAR);
#endif
#ifdef THOUSANDS_SEP
	REGISTER_NL_LANGINFO_CONSTANT(THOUSANDS_SEP);
#endif
#ifdef THOUSEP
	REGISTER_NL_LANGINFO_CONSTANT(THOUSEP);
#endif
#ifdef GROUPING
	REGISTER_NL_LANGINFO_CONSTANT(GROUPING);
#endif
#ifdef YESEXPR
	REGISTER_NL_LANGINFO_CONSTANT(YESEXPR);
#endif
#ifdef NOEXPR
	REGISTER_NL_LANGINFO_CONSTANT(NOEXPR);
#endif
#ifdef YESSTR
	REGISTER_NL_LANGINFO_CONSTANT(YESSTR);
#endif
#ifdef NOSTR
	REGISTER_NL_LANGINFO_CONSTANT(NOSTR);
#endif
#ifdef CODESET
	REGISTER_NL_LANGINFO_CONSTANT(CODESET);
#endif
#undef REGISTER_NL_LANGINFO_CONSTANT
	return SUCCESS;
}
/* }}} */

/* {{{ proto string nl_langinfo(int item)
   Query language and locale information */
PHP_FUNCTION(nl_langinfo)
{
	zval **item;
	char *value;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &item) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(item);

	value = nl_langinfo(Z_LVAL_PP(item));
	if (value == NULL) {
		RETURN_FALSE;
	} else {
		RETURN_STRING(value, 1);
	}
}
#endif
/* }}} */

#ifdef HAVE_STRCOLL
/* {{{ proto int strcoll(string str1, string str2)
   Compares two strings using the current locale */
PHP_FUNCTION(strcoll)
{
	zval **s1, **s2;

	if (ZEND_NUM_ARGS()!=2 || zend_get_parameters_ex(2, &s1, &s2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(s1);
	convert_to_string_ex(s2);

	RETURN_LONG(strcoll((const char *) Z_STRVAL_PP(s1), 
	                    (const char *) Z_STRVAL_PP(s2)));
}
/* }}} */
#endif

/* {{{ php_charmask
 * Fills a 256-byte bytemask with input. You can specify a range like 'a..z',
 * it needs to be incrementing.  
 * Returns: FAILURE/SUCCESS wether the input was correct (i.e. no range errors)
 */
static inline int php_charmask(unsigned char *input, int len, char *mask TSRMLS_DC)
{
	unsigned char *end;
	unsigned char c;
	int result = SUCCESS;

	memset(mask, 0, 256);
	for (end = input+len; input < end; input++) {
		c=*input; 
		if ((input+3 < end) && input[1] == '.' && input[2] == '.' 
				&& input[3] >= c) {
			memset(mask+c, 1, input[3] - c + 1);
			input+=3;
		} else if ((input+1 < end) && input[0] == '.' && input[1] == '.') {
			/* Error, try to be as helpful as possible:
			   (a range ending/starting with '.' won't be captured here) */
			if (end-len >= input) { /* there was no 'left' char */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, no character to the left of '..'.");
				result = FAILURE;
				continue;
			}
			if (input+2 >= end) { /* there is no 'right' char */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, no character to the right of '..'.");
				result = FAILURE;
				continue;
			}
			if (input[-1] > input[2]) { /* wrong order */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, '..'-range needs to be incrementing.");
				result = FAILURE;
				continue;
			} 
			/* FIXME: better error (a..b..c is the only left possibility?) */
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range.");
			result = FAILURE;
			continue;
		} else {
			mask[c]=1;
		}
	}
	return result;
}
/* }}} */

/* {{{ php_trim()
 * mode 1 : trim left
 * mode 2 : trim right
 * mode 3 : trim left and right
 * what indicates which chars are to be trimmed. NULL->default (' \t\n\r\v\0')
 */
PHPAPI char *php_trim(char *c, int len, char *what, int what_len, zval *return_value, int mode TSRMLS_DC)
{
	register int i;
	int trimmed = 0;
	char mask[256];

	if (what) {
		php_charmask(what, what_len, mask TSRMLS_CC);
	} else {
		php_charmask(" \n\r\t\v\0", 6, mask TSRMLS_CC);
	}

	if (mode & 1) {
		for (i = 0; i < len; i++) {
			if (mask[(unsigned char)c[i]]) {
				trimmed++;
			} else {
				break;
			}
		}
		len -= trimmed;
		c += trimmed;
	}
	if (mode & 2) {
		for (i = len - 1; i >= 0; i--) {
			if (mask[(unsigned char)c[i]]) {
				len--;
			} else {
				break;
			}
		}
	}

	if (return_value) {
		RETVAL_STRINGL(c, len, 1);
	} else {
		return estrndup(c, len);
	}
	return "";
}
/* }}} */

/* {{{ php_expand_u_trim_range()
 * Expands possible ranges of the form 'a..b' in input charlist,
 * where a < b in code-point order
 */
static int php_expand_u_trim_range(UChar **range, int32_t *range_len TSRMLS_DC)
{
	UChar32 *codepts, *tmp, *input, *end, c;
	int32_t len, tmp_len, idx;
	UErrorCode err;
	int expanded = 0;
	int result = SUCCESS;

	/* First, convert UTF-16 to UTF-32 */
	len = *range_len;
	codepts = (UChar32 *)emalloc((len+1)*sizeof(UChar32));
	err = U_ZERO_ERROR;
	u_strToUTF32((UChar32 *)codepts, len+1, &len, *range, len, &err);

	/* Expand ranges, if any - taken from php_charmask() */
	tmp_len = len;
	tmp = (UChar32 *)emalloc((tmp_len+1)*sizeof(UChar32));
	input = codepts;
	for ( idx = 0, end = input+len ; input < end ; input++ ) {
		c = input[0];
		if ( (input+3 < end) && input[1] == '.' && input[2] == '.' && input[3] >= c ) {
			tmp_len += (input[3] - c + 1);
			tmp = (UChar32 *)erealloc(tmp, tmp_len*sizeof(UChar));
			for ( ; c <= input[3] ; c++ ) {
				if ( U_IS_UNICODE_CHAR(c) ) tmp[idx++] = c;
			}
			input += 3;
			expanded++;
		} else if ( (input+1 < end) && input[0] == '.' && input[1] == '.' ) {
			/* Error, try to be as helpful as possible:
			   (a range ending/starting with '.' won't be captured here) */
			if ( end-len >= input ) { /* There is no 'left' char */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, no character to the left of '..'");
				result = FAILURE;
				continue;
			}
			if ( input+2 >= end ) { /* There is no 'right' char */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, no character to the right of '..'");
				result = FAILURE;
				continue;
			}
			if ( input[-1] > input[2] ) { /* Wrong order */
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range, '..'-range needs to be incrementing");
				result = FAILURE;
				continue;
			}
			/* FIXME: Better error (a..b..c is the only left possibility?) */
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid '..'-range");
			result = FAILURE;
			continue;
		} else {
			tmp[idx++] = c;
		}
	}

	/* If any ranges were expanded, convert the expanded results back to UTF-16 */
	if ( expanded > 0 ) {
		len = tmp_len;
		*range = (UChar *)erealloc(*range, (len+1)*sizeof(UChar));
		err = U_ZERO_ERROR;
		u_strFromUTF32(*range, len+1, &len, tmp, tmp_len, &err);
		if ( U_FAILURE(err) == U_BUFFER_OVERFLOW_ERROR ) {
			err = U_ZERO_ERROR;
			*range = (UChar *)erealloc(*range, (len+1)*sizeof(UChar));
			u_strFromUTF32(*range, len+1, NULL, tmp, tmp_len, &err);
			if ( U_FAILURE(err) ) { /* Internal ICU error */
				result = FAILURE;
			}
		}
		*range_len = len;
	}

	efree(tmp);
	efree(codepts);

	return result;
}
/* }}} */

/* {{{ php_u_trim()
 * Unicode capable version of php_trim()
 */
static UChar *php_u_trim(UChar *c, int32_t len, UChar *what, int32_t what_len, zval *return_value, int mode TSRMLS_DC)
{
	int32_t	i,j;
	UChar	ch,wh;
	int32_t	start = 0, end = len;

	if ( what ) {
		php_expand_u_trim_range(&what, &what_len TSRMLS_CC);
	}

	if ( mode & 1 ) {
		for ( i = 0 ; i < end ; ) {
			U16_NEXT(c, i, end, ch);
			if ( what ) {
				for ( j = 0 ; j < what_len ;  ) {
					U16_NEXT(what, j, what_len, wh);
					if ( wh == ch ) break;
				}
				if ( wh != ch ) break;
			} else {
				if ( u_isWhitespace(ch) == FALSE ) break;
			}
		}
		if ( i < end ) {
			U16_BACK_1(c, 0, i); /* U16_NEXT() post-increments 'i' */
		}
		start = i;
	}
	if ( mode & 2 ) {
		for ( i = end ; i > start ; ) {
			U16_PREV(c, 0, i, ch);
			if ( what ) {
				for ( j = 0 ; j < what_len ; ) {
					U16_NEXT(what, j, what_len, wh);
					if ( wh == ch ) break;
				}
				if ( wh != ch ) break;
			} else {
				if ( u_isWhitespace(ch) == FALSE ) break;
			}
		}
		end = i;
	}

	if ( start < len ) {
		if ( return_value ) {
			RETVAL_UNICODEL(c+start, end-start+1, 1);
		} else {
			return eustrndup(c+start, end-start+1);
		}
	} else { /* Trimmed the whole string */
		if ( return_value ) {
			RETVAL_EMPTY_UNICODE();
		} else {
			return (USTR_MAKE(""));
		}
	}
	return (USTR_MAKE(""));
}
/* }}} */

/* {{{ php_do_trim
 * Base for trim(), rtrim() and ltrim() functions.
 */
static void php_do_trim(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
	void		*str;
	int32_t		str_len;
	zend_uchar	str_type;
	void		*what;
	int32_t		what_len;
	zend_uchar	what_type;
	int			argc = ZEND_NUM_ARGS();

	if ( zend_parse_parameters(argc TSRMLS_CC, "T|T", &str, &str_len, &str_type,
								  &what, &what_len, &what_type) == FAILURE ) {
		return;
	}

	if ( argc > 1 ) {
		if ( str_type == IS_UNICODE ) {
			php_u_trim(str, str_len, what, what_len, return_value, mode TSRMLS_CC);
		} else {
			php_trim(str, str_len, what, what_len, return_value, mode TSRMLS_CC);
		}
	} else {
		if ( str_type == IS_UNICODE ) {
			php_u_trim(str, str_len, NULL, 0, return_value, mode TSRMLS_CC);
		} else {
			php_trim(str, str_len, NULL, 0, return_value, mode TSRMLS_CC);
		}
	}
}
/* }}} */

/* {{{ proto string trim(string str [, string character_mask])
   Strips whitespace from the beginning and end of a string */
PHP_FUNCTION(trim)
{
	php_do_trim(INTERNAL_FUNCTION_PARAM_PASSTHRU, 3);
}
/* }}} */

/* {{{ proto string rtrim(string str [, string character_mask])
   Removes trailing whitespace */
PHP_FUNCTION(rtrim)
{
	php_do_trim(INTERNAL_FUNCTION_PARAM_PASSTHRU, 2);
}
/* }}} */

/* {{{ proto string ltrim(string str [, string character_mask])
   Strips whitespace from the beginning of a string */
PHP_FUNCTION(ltrim)
{
	php_do_trim(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto string wordwrap(string str [, int width [, string break [, boolean cut]]])
   Wraps buffer to selected number of characters using string break char */
PHP_FUNCTION(wordwrap)
{
	const char *text, *breakchar = "\n";
	char *newtext;
	int textlen, breakcharlen = 1, newtextlen, alloced, chk;
	long current = 0, laststart = 0, lastspace = 0;
	long linelength = 75;
	zend_bool docut = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lsb", &text, &textlen, &linelength, &breakchar, &breakcharlen, &docut) == FAILURE) {
		return;
	}

	if (textlen == 0) {
		RETURN_EMPTY_STRING();
	}

	if (linelength == 0 && docut) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Can't force cut when width is zero.");
		RETURN_FALSE;
	}

	/* Special case for a single-character break as it needs no
	   additional storage space */
	if (breakcharlen == 1 && !docut) {
		newtext = estrndup(text, textlen);

		laststart = lastspace = 0;
		for (current = 0; current < textlen; current++) {
			if (text[current] == breakchar[0]) {
				laststart = lastspace = current;
			} else if (text[current] == ' ') {
				if (current - laststart >= linelength) {
					newtext[current] = breakchar[0];
					laststart = current + 1;
				}
				lastspace = current;
			} else if (current - laststart >= linelength && laststart != lastspace) {
				newtext[lastspace] = breakchar[0];
				laststart = lastspace;
			}
		}

		RETURN_STRINGL(newtext, textlen, 0);
	} else {
		/* Multiple character line break or forced cut */
		if (linelength > 0) {
			chk = (int)(textlen/linelength + 1);
			alloced = textlen + chk * breakcharlen + 1;
		} else {
			chk = textlen;
			alloced = textlen * (breakcharlen + 1) + 1;
		}
		newtext = emalloc(alloced);

		/* now keep track of the actual new text length */
		newtextlen = 0;

		laststart = lastspace = 0;
		for (current = 0; current < textlen; current++) {
			if (chk <= 0) {
				alloced += (int) (((textlen - current + 1)/linelength + 1) * breakcharlen) + 1;
				newtext = erealloc(newtext, alloced);
				chk = (int) ((textlen - current)/linelength) + 1;
			}
			/* when we hit an existing break, copy to new buffer, and
			 * fix up laststart and lastspace */
			if (text[current] == breakchar[0]
				&& current + breakcharlen < textlen
				&& !strncmp(text+current, breakchar, breakcharlen)) {
				memcpy(newtext+newtextlen, text+laststart, current-laststart+breakcharlen);
				newtextlen += current-laststart+breakcharlen;
				current += breakcharlen - 1;
				laststart = lastspace = current + 1;
				chk--;
			}
			/* if it is a space, check if it is at the line boundary,
			 * copy and insert a break, or just keep track of it */
			else if (text[current] == ' ') {
				if (current - laststart >= linelength) {
					memcpy(newtext+newtextlen, text+laststart, current-laststart);
					newtextlen += current - laststart;
					memcpy(newtext+newtextlen, breakchar, breakcharlen);
					newtextlen += breakcharlen;
					laststart = current + 1;
					chk--;
				}
				lastspace = current;
			}
			/* if we are cutting, and we've accumulated enough
			 * characters, and we haven't see a space for this line,
			 * copy and insert a break. */
			else if (current - laststart >= linelength
					&& docut && laststart >= lastspace) {
				memcpy(newtext+newtextlen, text+laststart, current-laststart);
				newtextlen += current - laststart;
				memcpy(newtext+newtextlen, breakchar, breakcharlen);
				newtextlen += breakcharlen;
				laststart = lastspace = current;
				chk--;
			}
			/* if the current word puts us over the linelength, copy
			 * back up until the last space, insert a break, and move
			 * up the laststart */
			else if (current - laststart >= linelength
					&& laststart < lastspace) {
				memcpy(newtext+newtextlen, text+laststart, lastspace-laststart);
				newtextlen += lastspace - laststart;
				memcpy(newtext+newtextlen, breakchar, breakcharlen);
				newtextlen += breakcharlen;
				laststart = lastspace = lastspace + 1;
				chk--;
			}
		}

		/* copy over any stragglers */
		if (laststart != current) {
			memcpy(newtext+newtextlen, text+laststart, current-laststart);
			newtextlen += current - laststart;
		}

		newtext[newtextlen] = '\0';
		/* free unused memory */
		newtext = erealloc(newtext, newtextlen+1);

		RETURN_STRINGL(newtext, newtextlen, 0);
	}
}
/* }}} */

/* {{{ php_explode
 */
PHPAPI void php_explode(char *delim, uint delim_len, char *str, uint str_len, zend_uchar str_type, zval *return_value, int limit)
{
	char *p1, *p2, *endp;

	endp = str + str_len;
	p1 = str;
	p2 = php_memnstr(str, delim, delim_len, endp);

	if ( p2 == NULL ) {
		if ( str_type == IS_BINARY ) {
			add_next_index_binaryl(return_value, p1, str_len, 1);
		} else {
			add_next_index_stringl(return_value, p1, str_len, 1);
		}
	} else {
		do {
			if ( str_type == IS_BINARY ) {
				add_next_index_binaryl(return_value, p1, p2-p1, 1);
			} else {
				add_next_index_stringl(return_value, p1, p2-p1, 1);
			}
			p1 = p2 + delim_len;
		} while ( (p2 = php_memnstr(p1, delim, delim_len, endp)) != NULL &&
				  (limit == -1 || --limit > 1) );

		if ( p1 <= endp ) {
			if ( str_type == IS_BINARY ) {
				add_next_index_binaryl(return_value, p1, endp-p1, 1);
			} else {
				add_next_index_stringl(return_value, p1, endp-p1, 1);
			}
		}
	}
}
/* }}} */

/* {{{ php_explode_negative_limit
 */
PHPAPI void php_explode_negative_limit(char *delim, uint delim_len, char *str, uint str_len, zend_uchar str_type, zval *return_value, int limit)
{
#define EXPLODE_ALLOC_STEP 50
	char *p1, *p2, *endp;
	int allocated = EXPLODE_ALLOC_STEP, found = 0, i = 0, to_return = 0;
	char **positions = safe_emalloc(allocated, sizeof(char *), 0);

	endp = str + str_len;
	p1 = str;
	p2 = php_memnstr(str, delim, delim_len, endp);

	if ( p2 == NULL ) {
		/*
		do nothing since limit <= -1, thus if only one chunk - 1 + (limit) <= 0
		by doing nothing we return empty array
		*/
	} else {
		positions[found++] = p1;
		do {
			if ( found >= allocated ) {
				allocated = found + EXPLODE_ALLOC_STEP;/* make sure we have enough memory */
				positions = erealloc(positions, allocated*sizeof(char *));
			}
			positions[found++] = p1 = p2 + delim_len;
		} while ( (p2 = php_memnstr(p1, delim, delim_len, endp)) != NULL );
		
		to_return = limit + found;
		/* limit is at least -1 therefore no need of bounds checking : i will be always less than found */
		for ( i = 0 ; i < to_return ; i++ ) { /* this checks also for to_return > 0 */
			if ( str_type == IS_BINARY ) {
				add_next_index_binaryl(return_value, positions[i],
									   (positions[i+1]-delim_len) - positions[i], 1);
			} else {
				add_next_index_stringl(return_value, positions[i],
									   (positions[i+1]-delim_len) - positions[i], 1);
			}
		}
	}
	efree(positions);
#undef EXPLODE_ALLOC_STEP
}
/* }}} */

/* {{{ php_u_explode
 * Unicode capable version of php_explode()
 */
static void php_u_explode(UChar *delim, uint delim_len, UChar *str, uint str_len, zval *return_value, int limit)
{
	UChar *p1, *p2, *endp;

	endp = str + str_len;
	p1 = str;
	p2 = zend_u_memnstr(str, delim, delim_len, endp);

	if ( p2 == NULL ) {
		add_next_index_unicodel(return_value, p1, str_len, 1);
	} else {
		do {
			add_next_index_unicodel(return_value, p1, p2-p1, 1);
			p1 = (UChar *)p2 + delim_len;
		} while ((p2 = zend_u_memnstr(p1, delim, delim_len, endp)) != NULL &&
				  (limit == -1 || --limit > 1) );

		if ( p1 <= endp ) {
			add_next_index_unicodel(return_value, p1, endp-p1, 1);
		}
	}
}
/* }}} */

/* {{{ php_u_explode_negative_limit
 * Unicode capable version of php_explode_negative_limit()
 */
static void php_u_explode_negative_limit(UChar *delim, uint delim_len, UChar *str, uint str_len, zval *return_value, int limit)
{
#define EXPLODE_ALLOC_STEP 50
	UChar *p1, *p2, *endp;
	int allocated = EXPLODE_ALLOC_STEP, found = 0, i = 0, to_return = 0;
	UChar **positions = safe_emalloc(allocated, sizeof(UChar *), 0);

	endp = str + str_len;
	p1 = str;
	p2 = zend_u_memnstr(str, delim, delim_len, endp);

	if ( p2 == NULL ) {
		/*
		do nothing since limit <= -1, thus if only one chunk - 1 + (limit) <= 0
		by doing nothing we return empty array
		*/
	} else {
		positions[found++] = p1;
		do {
			if ( found >= allocated ) {
				allocated = found + EXPLODE_ALLOC_STEP;/* make sure we have enough memory */
				positions = erealloc(positions, allocated*sizeof(UChar *));
			}
			positions[found++] = p1 = p2 + delim_len;
		} while ( (p2 = zend_u_memnstr(p1, delim, delim_len, endp)) != NULL );
		
		to_return = limit + found;
		/* limit is at least -1 therefore no need of bounds checking : i will be always less than found */
		for ( i = 0 ; i < to_return ; i++ ) { /* this checks also for to_return > 0 */
			add_next_index_unicodel(return_value, positions[i],
									(positions[i+1]-delim_len) - positions[i], 1);
		}
	}
	efree(positions);
#undef EXPLODE_ALLOC_STEP
}
/* }}} */

/* {{{ proto array explode(string separator, string str [, int limit])
   Splits a string on string separator and return array of components. If limit is positive only limit number of components is returned. If limit is negative all components except the last abs(limit) are returned. */
PHP_FUNCTION(explode)
{
	void		*str, *delim;
	int32_t		str_len, delim_len;
	zend_uchar	str_type, delim_type;
	int			limit = -1;
	int			argc = ZEND_NUM_ARGS();

	if ( argc < 2 || argc > 3 ) {
		WRONG_PARAM_COUNT;
	}

	if ( zend_parse_parameters(argc TSRMLS_CC, "TT|l", &delim, &delim_len, &delim_type,
							   &str, &str_len, &str_type, &limit) == FAILURE) {
		return;
	}

	if ( delim_len == 0 ) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty delimiter.");
		RETURN_FALSE;
	}

	array_init(return_value);

	if ( str_len == 0 ) {
		if ( str_type == IS_UNICODE ) {
			add_next_index_unicodel(return_value, USTR_MAKE(""), sizeof("")-1, 1);
		} else if ( str_type == IS_BINARY ) {
			add_next_index_binaryl(return_value, "", sizeof("")-1, 1);
		} else {
			add_next_index_stringl(return_value, "", sizeof("")-1, 1);
		}
		return;
	}


	if (limit == 0 || limit == 1) {
		if ( str_type == IS_UNICODE ) {
			add_index_unicodel(return_value, 0, (UChar *)str, str_len, 1);
		} else if ( str_type == IS_BINARY ) {
			add_index_binaryl(return_value, 0, (char *)str, str_len, 1 TSRMLS_CC);
		} else {
			add_index_stringl(return_value, 0, (char *)str, str_len, 1);
		}
	} else if (limit < 0 && argc == 3) {
		if ( str_type == IS_UNICODE ) {
			php_u_explode_negative_limit((UChar *)delim, delim_len, (UChar *)str, str_len, return_value, limit);
		} else {
			php_explode_negative_limit((char *)delim, delim_len, (char *)str, str_len, str_type, return_value, limit);
		}
	} else {
		if ( str_type == IS_UNICODE ) {
			php_u_explode((UChar *)delim, delim_len, (UChar *)str, str_len, return_value, limit);
		} else {
			php_explode((char *)delim, delim_len, (char *)str, str_len, str_type, return_value, limit);
		}
	}
}
/* }}} */

/* {{{ proto string join(array src, string glue)
   An alias for implode */
/* }}} */

/* {{{ php_implode
 */
PHPAPI void php_implode(zval *delim, zval *arr, zval *return_value) 
{
	zval         **tmp;
	HashPosition   pos;
	smart_str      implstr = {0};
	int            numelems, i = 0;

	numelems = zend_hash_num_elements(Z_ARRVAL_P(arr));

	if (numelems == 0) {
		RETURN_EMPTY_STRING();
	}

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);

	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void **) &tmp, &pos) == SUCCESS) {
		if ((*tmp)->type != IS_STRING) {
			SEPARATE_ZVAL(tmp);
			convert_to_string(*tmp);
		} 
		
		smart_str_appendl(&implstr, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp));
		if (++i != numelems) {
			smart_str_appendl(&implstr, Z_STRVAL_P(delim), Z_STRLEN_P(delim));
		}
		zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos);
	}
	smart_str_0(&implstr);

	RETURN_STRINGL(implstr.c, implstr.len, 0);
}
/* }}} */

/* {{{ proto string implode([string glue,] array pieces)
   Joins array elements placing glue string between items and return one string */
PHP_FUNCTION(implode)
{
	zval **arg1 = NULL, **arg2 = NULL, *delim, *arr;
	int argc = ZEND_NUM_ARGS();

	if (argc < 1 || argc > 2 ||
		zend_get_parameters_ex(argc, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (argc == 1) {
		if (Z_TYPE_PP(arg1) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument to implode must be an array.");
			return;
		}

		MAKE_STD_ZVAL(delim);
#define _IMPL_EMPTY ""
		ZVAL_STRINGL(delim, _IMPL_EMPTY, sizeof(_IMPL_EMPTY) - 1, 0);

		SEPARATE_ZVAL(arg1);
		arr = *arg1;
	} else {
		if (Z_TYPE_PP(arg1) == IS_ARRAY) {
			SEPARATE_ZVAL(arg1);
			arr = *arg1;
			convert_to_string_ex(arg2);
			delim = *arg2;
		} else if (Z_TYPE_PP(arg2) == IS_ARRAY) {
			SEPARATE_ZVAL(arg2);
			arr = *arg2;
			convert_to_string_ex(arg1);
			delim = *arg1;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Bad arguments.");
			return;
		}
	}

	php_implode(delim, arr, return_value);

	if (argc == 1) {
		FREE_ZVAL(delim);
	}
}
/* }}} */

#define STRTOK_TABLE(p) BG(strtok_table)[(unsigned char) *p]	

/* {{{ proto string strtok([string str,] string token)
   Tokenize a string */
PHP_FUNCTION(strtok)
{
	zval **args[2];
	zval **tok, **str;
	char *token;
	char *token_end;
	char *p;
	char *pe;
	int skipped = 0;
	
	if (ZEND_NUM_ARGS() < 1 || ZEND_NUM_ARGS() > 2 || zend_get_parameters_array_ex(ZEND_NUM_ARGS(), args) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
		
	switch (ZEND_NUM_ARGS()) {
		case 1:
			tok = args[0];
			break;

		default:
		case 2:
			str = args[0];
			tok = args[1];
			convert_to_string_ex(str);

			zval_add_ref(str);
			if (BG(strtok_zval)) {
				zval_ptr_dtor(&BG(strtok_zval));
			}
			BG(strtok_zval) = *str;
			BG(strtok_last) = BG(strtok_string) = Z_STRVAL_PP(str);
			BG(strtok_len) = Z_STRLEN_PP(str);
			break;
	}
	
	p = BG(strtok_last); /* Where we start to search */
	pe = BG(strtok_string) + BG(strtok_len);

	if (!p || p >= pe) {
		RETURN_FALSE;
	}

	convert_to_string_ex(tok);
	
	token = Z_STRVAL_PP(tok);
	token_end = token + Z_STRLEN_PP(tok);

	while (token < token_end) {
		STRTOK_TABLE(token++) = 1;
	}
	
	/* Skip leading delimiters */
	while (STRTOK_TABLE(p)) {
		if (++p >= pe) {
			/* no other chars left */
			BG(strtok_last) = NULL;
			RETVAL_FALSE;
			goto restore;
		}
		skipped++;
	}
	
	/* We know at this place that *p is no delimiter, so skip it */	
	while (++p < pe) {
		if (STRTOK_TABLE(p)) {
			goto return_token;	
		}
	}
	
	if (p - BG(strtok_last)) {
return_token:
		RETVAL_STRINGL(BG(strtok_last) + skipped, (p - BG(strtok_last)) - skipped, 1);
		BG(strtok_last) = p + 1;
	} else {
		RETVAL_FALSE;
		BG(strtok_last) = NULL;
	}

	/* Restore table -- usually faster then memset'ing the table on every invocation */
restore:
	token = Z_STRVAL_PP(tok);
	
	while (token < token_end) {
		STRTOK_TABLE(token++) = 0;
	}
}
/* }}} */

/* {{{ php_strtoupper
 */
PHPAPI char *php_strtoupper(char *s, size_t len)
{
	unsigned char *c, *e;
	
	c = s;
	e = c+len;

	while (c < e) {
		*c = toupper(*c);
		c++;
	}
	return s;
}
/* }}} */

/* {{{ php_u_strtoupper
 */
PHPAPI UChar* php_u_strtoupper(UChar **s, int32_t *len, const char* locale)
{
	UChar *dest = NULL;
	int32_t dest_len;
	UErrorCode status;
	
	dest_len = *len;
	while (1) {
		status = U_ZERO_ERROR;
		dest = eurealloc(dest, dest_len+1);
		dest_len = u_strToUpper(dest, dest_len, *s, *len, locale, &status);
		if (status != U_BUFFER_OVERFLOW_ERROR) {
			break;
		}
	}

	if (U_SUCCESS(status)) {
		efree(*s);
		dest[dest_len] = 0;
		*s = dest;
		*len = dest_len;
	} else {
		efree(dest);
	}

	return *s;
}
/* }}} */

/* {{{ proto string strtoupper(string str)
   Makes a string uppercase */
PHP_FUNCTION(strtoupper)
{
	zval **arg;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg)) {
		WRONG_PARAM_COUNT;
	}
	if (Z_TYPE_PP(arg) != IS_STRING && Z_TYPE_PP(arg) != IS_UNICODE) {
		if (UG(unicode)) {
			convert_to_unicode_ex(arg);
		} else {
			convert_to_string_ex(arg);
		}
	}

	RETVAL_ZVAL(*arg, 1, 0);
	if (Z_TYPE_P(return_value) == IS_UNICODE) {
		php_u_strtoupper(&Z_USTRVAL_P(return_value), &Z_USTRLEN_P(return_value), UG(default_locale));
	} else {
		php_strtoupper(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
	}
}
/* }}} */

/* {{{ php_u_strtolower
 */
PHPAPI UChar *php_u_strtolower(UChar **s, int32_t *len, const char* locale)
{
	UChar *dest = NULL;
	int32_t dest_len;
	UErrorCode status = U_ZERO_ERROR;

	dest_len = *len;
	while (1) {
		status = U_ZERO_ERROR;
		dest = eurealloc(dest, dest_len+1);
		dest_len = u_strToLower(dest, dest_len, *s, *len, locale, &status);
		if (status != U_BUFFER_OVERFLOW_ERROR) {
			break;
		}
	}

	if (U_SUCCESS(status)) {
		efree(*s);
		dest[dest_len] = 0;
		*s = dest;
		*len = dest_len;
	} else {
		efree(dest);
	}
	return *s;
}
/* }}} */

/* {{{ php_strtolower
 */
PHPAPI char *php_strtolower(char *s, size_t len)
{
	unsigned char *c, *e;
	
	c = s;
	e = c+len;

	while (c < e) {
		*c = tolower(*c);
		c++;
	}
	return s;
}
/* }}} */

/* {{{ proto string strtolower(string str)
   Makes a string lowercase */
PHP_FUNCTION(strtolower)
{
	zval **str;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str)) {
		WRONG_PARAM_COUNT;
	}
	if (Z_TYPE_PP(str) != IS_STRING && Z_TYPE_PP(str) != IS_UNICODE) {
		if (UG(unicode)) {
			convert_to_unicode_ex(str);
		} else {
			convert_to_string_ex(str);
		}
	}

	RETVAL_ZVAL(*str, 1, 0);
	if (Z_TYPE_P(return_value) == IS_UNICODE) {
		php_u_strtolower(&Z_USTRVAL_P(return_value), &Z_USTRLEN_P(return_value), UG(default_locale));
	} else {
		php_strtolower(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value));
	}
}
/* }}} */

/* {{{ php_basename
 */
PHPAPI void php_basename(char *s, size_t len, char *suffix, size_t sufflen, char **p_ret, size_t *p_len TSRMLS_DC)
{
	char *ret = NULL, *c, *comp, *cend;
	size_t inc_len, cnt;
	int state;

	c = comp = cend = s;
	cnt = len;
	state = 0;
	while (cnt > 0) {
		inc_len = (*c == '\0' ? 1: php_mblen(c, cnt));

		switch (inc_len) {
			case -2:
			case -1:
				inc_len = 1;
				php_mblen(NULL, 0);
				break;
			case 0:
				goto quit_loop;
			case 1:
#if defined(PHP_WIN32) || defined(NETWARE)
				if (*c == '/' || *c == '\\') {
#else
				if (*c == '/') {
#endif
					if (state == 1) {
						state = 0;
						cend = c;
					}
				} else {
					if (state == 0) {
						comp = c;
						state = 1;
					}
				}
			default:
				break;
		}
		c += inc_len;
		cnt -= inc_len;
	}

quit_loop:
	if (state == 1) {
		cend = c;
	}
	if (suffix != NULL && sufflen < (cend - comp) &&
			memcmp(cend - sufflen, suffix, sufflen) == 0) {
		cend -= sufflen;
	}

	len = cend - comp;
	ret = emalloc(len + 1);
	memcpy(ret, comp, len);
	ret[len] = '\0';

	if (p_ret) {
		*p_ret = ret;
	}
	if (p_len) {
		*p_len = len;
	}
}
/* }}} */

/* {{{ proto string basename(string path [, string suffix])
   Returns the filename component of the path */
PHP_FUNCTION(basename)
{
	char *string, *suffix = NULL, *ret;
	int   string_len, suffix_len = 0;
	size_t ret_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &string, &string_len, &suffix, &suffix_len) == FAILURE) {
		return;
	}

	php_basename(string, string_len, suffix, suffix_len, &ret, &ret_len TSRMLS_CC);
	RETURN_STRINGL(ret, (int)ret_len, 0);
}
/* }}} */

/* {{{ php_dirname
   Returns directory name component of path */
PHPAPI size_t php_dirname(char *path, size_t len)
{
	register char *end = path + len - 1;
	unsigned int len_adjust = 0;

#ifdef PHP_WIN32
	/* Note that on Win32 CWD is per drive (heritage from CP/M).
	 * This means dirname("c:foo") maps to "c:." or "c:" - which means CWD on C: drive.
	 */
	if ((2 <= len) && isalpha((int)((unsigned char *)path)[0]) && (':' == path[1])) {
		/* Skip over the drive spec (if any) so as not to change */
		path += 2;
		len_adjust += 2;
		if (2 == len) {
			/* Return "c:" on Win32 for dirname("c:").
			 * It would be more consistent to return "c:." 
			 * but that would require making the string *longer*.
			 */
			return len;
		}
	}
#elif defined(NETWARE)
	/*
	 * Find the first occurence of : from the left 
	 * move the path pointer to the position just after :
	 * increment the len_adjust to the length of path till colon character(inclusive)
	 * If there is no character beyond : simple return len
	 */
	char *colonpos = NULL;
	colonpos = strchr(path, ':');
	if(colonpos != NULL) {
		len_adjust = ((colonpos - path) + 1);
		path += len_adjust;
		if(len_adjust == len) {
		return len;
		}
    	}
#endif

	if (len == 0) {
		/* Illegal use of this function */
		return 0;
	}

	/* Strip trailing slashes */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* The path only contained slashes */
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}

	/* Strip filename */
	while (end >= path && !IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* No slash found, therefore return '.' */
#ifdef NETWARE
		if(len_adjust == 0) {
			path[0] = '.';
			path[1] = '\0';
			return 1; //only one character
		} 
		else {
			path[0] = '\0';
			return len_adjust;
		}
#else
		path[0] = '.';
		path[1] = '\0';
		return 1 + len_adjust;
#endif
	}

	/* Strip slashes which came before the file name */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}
	*(end+1) = '\0';

	return (size_t)(end + 1 - path) + len_adjust;
}
/* }}} */

/* {{{ proto string dirname(string path)
   Returns the directory name component of the path */
PHP_FUNCTION(dirname)
{
	zval **str;
	char *ret;
	size_t ret_len;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	ret = estrndup(Z_STRVAL_PP(str), Z_STRLEN_PP(str));
	ret_len = php_dirname(ret, Z_STRLEN_PP(str));

	RETURN_STRINGL(ret, ret_len, 0);
}
/* }}} */

/* {{{ proto array pathinfo(string path)
   Returns information about a certain string */
PHP_FUNCTION(pathinfo)
{
	zval *tmp;
	char *path, *ret = NULL;
	int path_len;
	size_t ret_len;
	long opt = PHP_PATHINFO_ALL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &path, &path_len, &opt) == FAILURE) {
		return;
	}

	MAKE_STD_ZVAL(tmp);
	array_init(tmp);
	
	if ((opt & PHP_PATHINFO_DIRNAME) == PHP_PATHINFO_DIRNAME) {
		ret = estrndup(path, path_len);
		php_dirname(ret, path_len);
		if (*ret) {
			add_assoc_string(tmp, "dirname", ret, 1);
		}
		efree(ret);
	}
	
	if ((opt & PHP_PATHINFO_BASENAME) == PHP_PATHINFO_BASENAME) {
		php_basename(path, path_len, NULL, 0, &ret, &ret_len TSRMLS_CC);
		add_assoc_stringl(tmp, "basename", ret, ret_len, 0);
	}			
	
	if ((opt & PHP_PATHINFO_EXTENSION) == PHP_PATHINFO_EXTENSION) {
		char *p;
		int idx;
		int have_basename = ((opt & PHP_PATHINFO_BASENAME) == PHP_PATHINFO_BASENAME);

		/* Have we alrady looked up the basename? */
		if (!have_basename) {
			php_basename(path, path_len, NULL, 0, &ret, &ret_len TSRMLS_CC);
		}

		p = strrchr(ret, '.');

		if (p) {
			idx = p - ret;
			add_assoc_stringl(tmp, "extension", ret + idx + 1, ret_len - idx - 1, 1);
		}

		if (!have_basename) {
			efree(ret);
		}
	}

	if (opt == PHP_PATHINFO_ALL) {
		RETURN_ZVAL(tmp, 0, 1);
	} else {
		zval **element;
		if (zend_hash_get_current_data(Z_ARRVAL_P(tmp), (void **) &element) == SUCCESS) {
			RETVAL_ZVAL(*element, 1, 0);
		} else {
			ZVAL_EMPTY_STRING(return_value);
		}
	}

	zval_ptr_dtor(&tmp);
}
/* }}} */

/* {{{ php_stristr
   case insensitve strstr */
PHPAPI char *php_stristr(unsigned char *s, unsigned char *t, size_t s_len, size_t t_len)
{
	php_strtolower(s, s_len);
	php_strtolower(t, t_len);
	return php_memnstr(s, t, t_len, s + s_len);
}
/* }}} */

/* {{{ php_strspn
 */
PHPAPI size_t php_strspn(char *s1, char *s2, char *s1_end, char *s2_end)
{
	register const char *p = s1, *spanp;
	register char c = *p;

cont:
	for (spanp = s2; p != s1_end && spanp != s2_end;) {
		if (*spanp++ == c) {
			c = *(++p);
			goto cont;
		}
	}
	return (p - s1);
}
/* }}} */

/* {{{ php_strcspn
 */
PHPAPI size_t php_strcspn(char *s1, char *s2, char *s1_end, char *s2_end)
{
	register const char *p, *spanp;
	register char c = *s1;

	for (p = s1;;) {
		spanp = s2;
		do {
			if (*spanp == c || p == s1_end) {
				return p - s1;
			}
		} while (spanp++ < s2_end);
		c = *++p;
	}
	/* NOTREACHED */
}
/* }}} */

/* {{{ proto string stristr(string haystack, string needle[, bool part])
   Finds first occurrence of a string within another, case insensitive */
PHP_FUNCTION(stristr)
{
	char *haystack;
	long haystack_len;
	zval *needle;
	zend_bool part = 0;
	char *found = NULL;
	int  found_offset;
	char *haystack_orig;
	char needle_char[2];
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|b", &haystack, &haystack_len, &needle, &part) == FAILURE) {
		return;
	}

	SEPARATE_ZVAL(&needle);

	haystack_orig = estrndup(haystack, haystack_len);

	if (Z_TYPE_P(needle) == IS_STRING) {
		if (!Z_STRLEN_P(needle)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty delimiter.");
			efree(haystack_orig);
			RETURN_FALSE;
		}

		found = php_stristr(haystack,
							Z_STRVAL_P(needle),
							haystack_len,
							Z_STRLEN_P(needle));
	} else {
		convert_to_long_ex(&needle);
		needle_char[0] = (char) Z_LVAL_P(needle);
		needle_char[1] = 0;

		found = php_stristr(haystack,
							needle_char,
							haystack_len,
							1);
	}

	if (found) {
		found_offset = found - haystack;
		if (part) {
			char *ret;
			ret = emalloc(found_offset + 1);
			strncpy(ret, haystack_orig, found_offset);
			ret[found_offset] = '\0';
			RETVAL_STRINGL(ret , found_offset, 0);
		} else {
			RETVAL_STRINGL(haystack_orig + found_offset, haystack_len - found_offset, 1);
		}
	} else {
		RETVAL_FALSE;
	}

	efree(haystack_orig);
}
/* }}} */

/* {{{ proto string strstr(string haystack, string needle[, bool part])
   Finds first occurrence of a string within another */
PHP_FUNCTION(strstr)
{
	void *haystack;
	int32_t haystack_len;
	zend_uchar haystack_type;
	zval **needle;
	void *found = NULL;
	char  needle_char[2];
	UChar u_needle_char[3];
	int32_t n_len = 0;
	size_t found_offset;
	zend_bool part = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "tZ|b", &haystack, &haystack_len, &haystack_type, &needle, &part) == FAILURE) {
		return;
	}

	if (Z_TYPE_PP(needle) == IS_STRING || Z_TYPE_PP(needle) == IS_UNICODE || Z_TYPE_PP(needle) == IS_BINARY) {
		if (!Z_STRLEN_PP(needle)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty delimiter.");
			RETURN_FALSE;
		}

		/* haystack type determines the needle type */
		if (haystack_type == IS_UNICODE) {
			convert_to_unicode_ex(needle);
			found = zend_u_memnstr((UChar*)haystack,
								   Z_USTRVAL_PP(needle),
								   Z_USTRLEN_PP(needle),
								   (UChar*)haystack + haystack_len);
		} else {
			convert_to_string_ex(needle);
			found = php_memnstr((char*)haystack,
								Z_STRVAL_PP(needle),
								Z_STRLEN_PP(needle),
								(char*)haystack + haystack_len);
		}
	} else {
		convert_to_long_ex(needle);
		if (haystack_type == IS_UNICODE) {
			if (Z_LVAL_PP(needle) < 0 || Z_LVAL_PP(needle) > 0x10FFFF) {
				php_error(E_WARNING, "Needle argument codepoint value out of range (0 - 0x10FFFF)");
				RETURN_FALSE;
			}
			/* supplementary codepoint values may require 2 UChar's */
			if (U_IS_BMP(Z_LVAL_PP(needle))) {
				u_needle_char[n_len++] = (UChar) Z_LVAL_PP(needle);
				u_needle_char[n_len]   = 0;
			} else {
				u_needle_char[n_len++] = (UChar) U16_LEAD(Z_LVAL_PP(needle));
				u_needle_char[n_len++] = (UChar) U16_TRAIL(Z_LVAL_PP(needle));
				u_needle_char[n_len]   = 0;
			}

			found = zend_u_memnstr((UChar*)haystack,
								   u_needle_char,
								   n_len,
								   (UChar*)haystack + haystack_len);
		} else {
			needle_char[0] = (char) Z_LVAL_PP(needle);
			needle_char[1] = 0;

			found = php_memnstr((char*)haystack,
								needle_char,
								1,
								(char*)haystack + haystack_len);
		}
	}

	if (found) {
		switch (haystack_type) {
			case IS_UNICODE:
				found_offset = (UChar*)found - (UChar*)haystack;
				if (part) {
					UChar *ret;
					ret = eumalloc(found_offset + 1);
					u_strncpy(ret, haystack, found_offset);
					ret[found_offset] = '\0';
					RETURN_UNICODEL(ret , found_offset, 0);
				} else {
					RETURN_UNICODEL(found, haystack_len - found_offset, 1);
				}
				break;

			case IS_STRING:
				found_offset = (char *)found - (char *)haystack;
				if (part) {
					char *ret;
					ret = emalloc(found_offset + 1);
					strncpy(ret, haystack, found_offset);
					ret[found_offset] = '\0';
					RETURN_STRINGL(ret , found_offset, 0);
				} else {
					RETURN_STRINGL(found, haystack_len - found_offset, 1);
				}
				break;

			case IS_BINARY:
				found_offset = (char *)found - (char *)haystack;
				if (part) {
					char *ret;
					ret = emalloc(found_offset + 1);
					strncpy(ret, haystack, found_offset);
					ret[found_offset] = '\0';
					RETURN_BINARYL(ret , found_offset, 0);
				} else {
					RETURN_BINARYL(found, haystack_len - found_offset, 1);
				}
				break;
		}
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string strchr(string haystack, string needle)
   An alias for strstr */
/* }}} */

/* {{{ proto int strpos(text haystack, mixed needle [, int offset])
   Finds position of first occurrence of a string within another */
PHP_FUNCTION(strpos)
{
	void *haystack;
	int32_t haystack_len;
	zend_uchar haystack_type;
	zval **needle;
	int   offset = 0;
	void *found = NULL;
	char  needle_char[2];
	UChar u_needle_char[3];
	int32_t n_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "tZ|l", &haystack,
							  &haystack_len, &haystack_type, &needle, &offset) == FAILURE) {
		return;
	}

	/*
	 * Unicode note: it's okay to not convert offset to codepoint offset here.
	 * We'll just do a rough check that the offset does not exceed length in
	 * code units, and leave the rest to zend_u_memnstr().
	 */
	if (offset < 0 || offset > haystack_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Offset not contained in string.");
		RETURN_FALSE;
	}

	if (Z_TYPE_PP(needle) == IS_STRING || Z_TYPE_PP(needle) == IS_UNICODE || Z_TYPE_PP(needle) == IS_BINARY) {
		if (!Z_STRLEN_PP(needle)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty delimiter.");
			RETURN_FALSE;
		}

		/* haystack type determines the needle type */
		if (haystack_type == IS_UNICODE) {
			int32_t cp_offset = 0;
			convert_to_unicode_ex(needle);
			/* locate the codepoint at the specified offset */
			U16_FWD_N((UChar*)haystack, cp_offset, haystack_len, offset);
			found = zend_u_memnstr((UChar*)haystack + cp_offset,
								   Z_USTRVAL_PP(needle),
								   Z_USTRLEN_PP(needle),
								   (UChar*)haystack + haystack_len);
		} else {
			convert_to_string_ex(needle);
			found = php_memnstr((char*)haystack + offset,
								Z_STRVAL_PP(needle),
								Z_STRLEN_PP(needle),
								(char*)haystack + haystack_len);
		}
	} else {
		convert_to_long_ex(needle);
		if (haystack_type == IS_UNICODE) {
			int32_t cp_offset = 0;
			if (Z_LVAL_PP(needle) < 0 || Z_LVAL_PP(needle) > 0x10FFFF) {
				php_error(E_WARNING, "Needle argument codepoint value out of range (0 - 0x10FFFF)");
				RETURN_FALSE;
			}
			/* supplementary codepoint values may require 2 UChar's */
			if (U_IS_BMP(Z_LVAL_PP(needle))) {
				u_needle_char[n_len++] = (UChar) Z_LVAL_PP(needle);
				u_needle_char[n_len]   = 0;
			} else {
				u_needle_char[n_len++] = (UChar) U16_LEAD(Z_LVAL_PP(needle));
				u_needle_char[n_len++] = (UChar) U16_TRAIL(Z_LVAL_PP(needle));
				u_needle_char[n_len]   = 0;
			}

			/* locate the codepoint at the specified offset */
			U16_FWD_N((UChar*)haystack, cp_offset, haystack_len, offset);
			found = zend_u_memnstr((UChar*)haystack + cp_offset,
								   u_needle_char,
								   n_len,
								   (UChar*)haystack + haystack_len);
		} else {
			needle_char[0] = (char) Z_LVAL_PP(needle);
			needle_char[1] = 0;

			found = php_memnstr((char*)haystack + offset,
								needle_char,
								1,
								(char*)haystack + haystack_len);
		}
	}

	if (found) {
		if (haystack_type == IS_UNICODE) {
			/* simple subtraction will not suffice, since there may be
			   supplementary codepoints */
			RETURN_LONG(u_countChar32(haystack, ((char *)found - (char *)haystack)/sizeof(UChar)));
		} else {
			RETURN_LONG((char *)found - (char *)haystack);
		}
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int stripos(string haystack, string needle [, int offset])
   Finds position of first occurrence of a string within another, case insensitive */
PHP_FUNCTION(stripos)
{
	char *found = NULL;
	char *haystack;
	int haystack_len;
	long offset = 0;
	char *needle_dup = NULL, *haystack_dup;
	char needle_char[2];
	zval *needle;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &haystack, &haystack_len, &needle, &offset) == FAILURE) {
		return;
	}

	if (offset < 0 || offset > haystack_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Offset not contained in string.");
		RETURN_FALSE;
	}

	haystack_dup = estrndup(haystack, haystack_len);
	php_strtolower(haystack_dup, haystack_len);

	if (Z_TYPE_P(needle) == IS_STRING) {
		needle_dup = estrndup(Z_STRVAL_P(needle), Z_STRLEN_P(needle));
		php_strtolower(needle_dup, Z_STRLEN_P(needle));
		found = php_memnstr(haystack_dup + offset, needle_dup, Z_STRLEN_P(needle), haystack_dup + haystack_len);
	} else {
		switch (Z_TYPE_P(needle)) {
			case IS_LONG:
			case IS_BOOL:
				needle_char[0] = tolower((char) Z_LVAL_P(needle));
				break;
			case IS_DOUBLE:
				needle_char[0] = tolower((char) Z_DVAL_P(needle));
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "needle is not a string or an integer.");
				efree(haystack_dup);
				RETURN_FALSE;
				break;
					
		}
		needle_char[1] = '\0';
		found = php_memnstr(haystack_dup + offset, 
							needle_char, 
							sizeof(needle_char) - 1, 
							haystack_dup + haystack_len);
	}

	efree(haystack_dup);
	if (needle_dup) {
		efree(needle_dup);
	}

	if (found) {
		RETURN_LONG(found - haystack_dup);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int strrpos(string haystack, string needle [, int offset])
   Finds position of last occurrence of a string within another string */
PHP_FUNCTION(strrpos)
{
	zval *zneedle;
	char *needle, *haystack;
	int needle_len, haystack_len;
	long offset = 0;
	char *p, *e, ord_needle[2];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &haystack, &haystack_len, &zneedle, &offset) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(zneedle) == IS_STRING) {
		needle = Z_STRVAL_P(zneedle);
		needle_len = Z_STRLEN_P(zneedle);
	} else {
		convert_to_long(zneedle);
		ord_needle[0] = (char)(Z_LVAL_P(zneedle) & 0xFF);
		ord_needle[1] = '\0';
		needle = ord_needle;
		needle_len = 1;
	}

	if ((haystack_len == 0) || (needle_len == 0)) {
		RETURN_FALSE;
	}

	if (offset >= 0) {
		p = haystack + offset;
		e = haystack + haystack_len - needle_len;
	} else {
		p = haystack;
		if (-offset > haystack_len) {
			e = haystack - needle_len;
		} else if (needle_len > -offset) {
			e = haystack + haystack_len - needle_len;
		} else {
			e = haystack + haystack_len + offset;
		}
	}

	if (needle_len == 1) {
		/* Single character search can shortcut memcmps */
		while (e >= p) {
			if (*e == *needle) {
				RETURN_LONG(e - p + (offset > 0 ? offset : 0));
			}
			e--;
		}
		RETURN_FALSE;
	}

	while (e >= p) {
		if (memcmp(e, needle, needle_len) == 0) {
			RETURN_LONG(e - p + (offset > 0 ? offset : 0));
		}
		e--;
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int strripos(string haystack, string needle [, int offset])
   Finds position of last occurrence of a string within another string */
PHP_FUNCTION(strripos)
{
	zval *zneedle;
	char *needle, *haystack;
	int needle_len, haystack_len;
	long offset = 0;
	char *p, *e, ord_needle[2];
	char *needle_dup, *haystack_dup;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &haystack, &haystack_len, &zneedle, &offset) == FAILURE) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(zneedle) == IS_STRING) {
		needle = Z_STRVAL_P(zneedle);
		needle_len = Z_STRLEN_P(zneedle);
	} else {
		convert_to_long(zneedle);
		ord_needle[0] = (char)(Z_LVAL_P(zneedle) & 0xFF);
		ord_needle[1] = '\0';
		needle = ord_needle;
		needle_len = 1;
	}

	if ((haystack_len == 0) || (needle_len == 0)) {
		RETURN_FALSE;
	}

	if (needle_len == 1) {
		/* Single character search can shortcut memcmps 
		   Can also avoid tolower emallocs */
		if (offset >= 0) {
			p = haystack + offset;
			e = haystack + haystack_len - 1;
		} else {
			p = haystack;
			if (-offset > haystack_len) {
				e = haystack + haystack_len - 1;
			} else {
				e = haystack + haystack_len + offset;
			}
		}
		/* Borrow that ord_needle buffer to avoid repeatedly tolower()ing needle */
		*ord_needle = tolower(*needle);
		while (e >= p) {
			if (tolower(*e) == *ord_needle) {
				RETURN_LONG(e - p + (offset > 0 ? offset : 0));
			}
			e--;
		}
		RETURN_FALSE;
	}

	needle_dup = estrndup(needle, needle_len);
	php_strtolower(needle_dup, needle_len);
	haystack_dup = estrndup(haystack, haystack_len);
	php_strtolower(haystack_dup, haystack_len);

	if (offset >= 0) {
		p = haystack_dup + offset;
		e = haystack_dup + haystack_len - needle_len;
	} else {
		p = haystack_dup;
		if (-offset > haystack_len) {
			e = haystack_dup - needle_len;
		} else if (needle_len > -offset) {
			e = haystack_dup + haystack_len - needle_len;
		} else {
			e = haystack_dup + haystack_len + offset;
		}
	}

	while (e >= p) {
		if (memcmp(e, needle_dup, needle_len) == 0) {
			efree(haystack_dup);
			efree(needle_dup);
			RETURN_LONG(e - p + (offset > 0 ? offset : 0));
		}
		e--;
	}

	efree(haystack_dup);
	efree(needle_dup);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string strrchr(string haystack, string needle)
   Finds the last occurrence of a character in a string within another */
PHP_FUNCTION(strrchr)
{
	zval **haystack, **needle;
	char *found = NULL;
	long found_offset;
	
	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &haystack, &needle) ==
		FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(haystack);

	if (Z_TYPE_PP(needle) == IS_STRING) {
		found = strrchr(Z_STRVAL_PP(haystack), *Z_STRVAL_PP(needle));
	} else {
		convert_to_long_ex(needle);
		found = strrchr(Z_STRVAL_PP(haystack), (char) Z_LVAL_PP(needle));
	}

	if (found) {
		found_offset = found - Z_STRVAL_PP(haystack);
		RETURN_STRINGL(found, Z_STRLEN_PP(haystack) - found_offset, 1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ php_chunk_split
 */
static char *php_chunk_split(char *src, int srclen, char *end, int endlen, int chunklen, int *destlen)
{
	char *dest;
	char *p, *q;
	int chunks; /* complete chunks! */
	int restlen;

	chunks = srclen / chunklen;
	restlen = srclen - chunks * chunklen; /* srclen % chunklen */

	dest = safe_emalloc((srclen + (chunks + 1) * endlen + 1), sizeof(char), 0);

	for (p = src, q = dest; p < (src + srclen - chunklen + 1); ) {
		memcpy(q, p, chunklen);
		q += chunklen;
		memcpy(q, end, endlen);
		q += endlen;
		p += chunklen;
	}

	if (restlen) {
		memcpy(q, p, restlen);
		q += restlen;
		memcpy(q, end, endlen);
		q += endlen;
	}

	*q = '\0';
	if (destlen) {
		*destlen = q - dest;
	}

	return(dest);
}
/* }}} */

/* {{{ proto string chunk_split(string str [, int chunklen [, string ending]])
   Returns split line */
PHP_FUNCTION(chunk_split) 
{
	zval **p_str, **p_chunklen, **p_ending;
	char *result;
	char *end    = "\r\n";
	int endlen   = 2;
	int chunklen = 76;
	int result_len;
	int argc = ZEND_NUM_ARGS();

	if (argc < 1 || argc > 3 ||	zend_get_parameters_ex(argc, &p_str, &p_chunklen, &p_ending) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(p_str);

	if (argc > 1) {
		convert_to_long_ex(p_chunklen);
		chunklen = Z_LVAL_PP(p_chunklen);
	}

	if (argc > 2) {
		convert_to_string_ex(p_ending);
		end = Z_STRVAL_PP(p_ending);
		endlen = Z_STRLEN_PP(p_ending);
	}

	if (chunklen <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Chunk length should be greater than zero.");
		RETURN_FALSE;
	}

	if (chunklen > Z_STRLEN_PP(p_str)) {
		/* to maintain BC, we must return original string + ending */
		result_len = endlen + Z_STRLEN_PP(p_str);
		result = emalloc(result_len + 1);
		memcpy(result, Z_STRVAL_PP(p_str), Z_STRLEN_PP(p_str));
		memcpy(result + Z_STRLEN_PP(p_str), end, endlen);
		result[result_len] = '\0'; 
		RETURN_STRINGL(result, result_len, 0);	
	}

	if (!Z_STRLEN_PP(p_str)) {
		RETURN_EMPTY_STRING();
	}

	result = php_chunk_split(Z_STRVAL_PP(p_str), Z_STRLEN_PP(p_str), end, endlen, chunklen, &result_len);

	if (result) {
		RETURN_STRINGL(result, result_len, 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string substr(string str, int start [, int length])
   Returns part of a string */
PHP_FUNCTION(substr)
{
	void *str;
	int32_t str_len, cp_len;
	zend_uchar str_type;
	int l = -1;
	int f;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "tl|l", &str, &str_len, &str_type, &f, &l) == FAILURE) {
		return;
	}

	if (str_type == IS_UNICODE) {
		cp_len = u_countChar32(str, str_len);
	} else {
		cp_len = str_len;
	}

	if (ZEND_NUM_ARGS() == 2) {
		l = cp_len;
	}
	
	/* if "from" position is negative, count start position from the end
	 * of the string
	 */
	if (f < 0) {
		f = cp_len + f;
		if (f < 0) {
			f = 0;
		}
	}

	/* if "length" position is negative, set it to the length
	 * needed to stop that many chars from the end of the string
	 */
	if (l < 0) {
		l = (cp_len - f) + l;
		if (l < 0) {
			l = 0;
		}
	}

	if (f >= cp_len) {
		RETURN_FALSE;
	}

	if (((unsigned) f + (unsigned) l) > cp_len) {
		l = cp_len - f;
	}

	if (str_type == IS_UNICODE) {
		int32_t start = 0, end = 0;
		U16_FWD_N((UChar*)str, end, str_len, f);
		start = end;
		U16_FWD_N((UChar*)str, end, str_len, l);
		RETURN_UNICODEL((UChar*)str + start, end-start, 1);
	} else {
		RETURN_STRINGL((char*)str + f, l, 1);
	}
}
/* }}} */


/* {{{ proto mixed substr_replace(mixed str, mixed repl, mixed start [, mixed length])
   Replaces part of a string with another string */
PHP_FUNCTION(substr_replace)
{
	zval **str;
	zval **from;
	zval **len = NULL;
	zval **repl;
	char *result;
	int result_len;
	int l = 0;
	int f;
	int argc = ZEND_NUM_ARGS();

	HashPosition pos_str, pos_from, pos_repl, pos_len;
	zval **tmp_str = NULL, **tmp_from = NULL, **tmp_repl = NULL, **tmp_len= NULL;


	if (argc < 3 || argc > 4 || zend_get_parameters_ex(argc, &str, &repl, &from, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if (Z_TYPE_PP(str) != IS_ARRAY) {
		convert_to_string_ex(str);
	}
	if (Z_TYPE_PP(repl) != IS_ARRAY) {
		convert_to_string_ex(repl);
	}
	if (Z_TYPE_PP(from) != IS_ARRAY) {
		convert_to_long_ex(from);
	}

	if (argc > 3) {
		if (Z_TYPE_PP(len) != IS_ARRAY) {
			convert_to_long_ex(len);
			l = Z_LVAL_PP(len);
		}
	} else {
		if (Z_TYPE_PP(str) != IS_ARRAY) {
			l = Z_STRLEN_PP(str);
		}
	}

	if (Z_TYPE_PP(str) == IS_STRING) {
		if (
			(argc == 3 && Z_TYPE_PP(from) == IS_ARRAY) 
			|| 
			(argc == 4 && Z_TYPE_PP(from) != Z_TYPE_PP(len))
		) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'from' and 'len' should be of same type - numerical or array ");
			RETURN_STRINGL(Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);		
		}
		if (argc == 4 && Z_TYPE_PP(from) == IS_ARRAY) {
			if (zend_hash_num_elements(Z_ARRVAL_PP(from)) != zend_hash_num_elements(Z_ARRVAL_PP(len))) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'from' and 'len' should have the same number of elements");
				RETURN_STRINGL(Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);		
			}
		}
	}

	
	if (Z_TYPE_PP(str) != IS_ARRAY) {
		if (Z_TYPE_PP(from) != IS_ARRAY) {
			int repl_len = 0;

			f = Z_LVAL_PP(from);

			/* if "from" position is negative, count start position from the end
			 * of the string
			 */
			if (f < 0) {
				f = Z_STRLEN_PP(str) + f;
				if (f < 0) {
					f = 0;
				}
			} else if (f > Z_STRLEN_PP(str)) {
				f = Z_STRLEN_PP(str);
			}
			/* if "length" position is negative, set it to the length
			 * needed to stop that many chars from the end of the string
			 */
			if (l < 0) {
				l = (Z_STRLEN_PP(str) - f) + l;
				if (l < 0) {
					l = 0;
				}
			}

			if (((unsigned) f + (unsigned) l) > Z_STRLEN_PP(str)) {
				l = Z_STRLEN_PP(str) - f;
			}
			if (Z_TYPE_PP(repl) == IS_ARRAY) {
				zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(repl), &pos_repl);
				if (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(repl), (void **) &tmp_repl, &pos_repl)) {
					convert_to_string_ex(tmp_repl);
					repl_len = Z_STRLEN_PP(tmp_repl);
				}
			} else {
				repl_len = Z_STRLEN_PP(repl);
			}
			result_len = Z_STRLEN_PP(str) - l + repl_len;
			result = emalloc(result_len + 1);

			memcpy(result, Z_STRVAL_PP(str), f);
			if (repl_len) {
				memcpy((result + f), (Z_TYPE_PP(repl) == IS_ARRAY ? Z_STRVAL_PP(tmp_repl) : Z_STRVAL_PP(repl)), repl_len);
			}
			memcpy((result + f + repl_len), Z_STRVAL_PP(str) + f + l, Z_STRLEN_PP(str) - f - l);
			result[result_len] = '\0';
			RETURN_STRINGL(result, result_len, 0);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Functionality of 'from' and 'len' as arrays is not implemented.");
			RETURN_STRINGL(Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);	
		}
	} else { /* str is array of strings */
		array_init(return_value);

		if (Z_TYPE_PP(from) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(from), &pos_from);
		}

		if (argc > 3 && Z_TYPE_PP(len) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(len), &pos_len);
		}

		if (Z_TYPE_PP(repl) == IS_ARRAY) {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(repl), &pos_repl);
		}

		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(str), &pos_str);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(str), (void **) &tmp_str, &pos_str) == SUCCESS) {
			convert_to_string_ex(tmp_str);

			if (Z_TYPE_PP(from) == IS_ARRAY) {
				if (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(from), (void **) &tmp_from, &pos_from)) {
					convert_to_long_ex(tmp_from);

					f = Z_LVAL_PP(tmp_from);
					if (f < 0) {
						f = Z_STRLEN_PP(tmp_str) + f;
						if (f < 0) {
							f = 0;
						}
					} else if (f > Z_STRLEN_PP(tmp_str)) {
						f = Z_STRLEN_PP(tmp_str);
					}
					zend_hash_move_forward_ex(Z_ARRVAL_PP(from), &pos_from);
				} else {
					f = 0;
				}
			} else {
				f = Z_LVAL_PP(from);
				if (f < 0) {
					f = Z_STRLEN_PP(tmp_str) + f;
					if (f < 0) {
						f = 0;
					}
				} else if (f > Z_STRLEN_PP(tmp_str)) {
					f = Z_STRLEN_PP(tmp_str);
				}
			}

			if (argc > 3 && Z_TYPE_PP(len) == IS_ARRAY) {
				if (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(len), (void **) &tmp_len, &pos_len)) {
					convert_to_long_ex(tmp_len);

					l = Z_LVAL_PP(tmp_len);
					zend_hash_move_forward_ex(Z_ARRVAL_PP(len), &pos_len);
				} else {
					l = Z_STRLEN_PP(tmp_str);
				}
			} else if (argc > 3) { 
				l = Z_LVAL_PP(len);
			} else {
				l = Z_STRLEN_PP(tmp_str);
			}

			if (l < 0) {
				l = (Z_STRLEN_PP(tmp_str) - f) + l;
				if (l < 0) {
					l = 0;
				}
			}

			if (((unsigned) f + (unsigned) l) > Z_STRLEN_PP(tmp_str)) {
				l = Z_STRLEN_PP(tmp_str) - f;
			}

			result_len = Z_STRLEN_PP(tmp_str) - l;

			if (Z_TYPE_PP(repl) == IS_ARRAY) {
				if (SUCCESS == zend_hash_get_current_data_ex(Z_ARRVAL_PP(repl), (void **) &tmp_repl, &pos_repl)) {
					convert_to_string_ex(tmp_repl);
					result_len += Z_STRLEN_PP(tmp_repl);
					zend_hash_move_forward_ex(Z_ARRVAL_PP(repl), &pos_repl);	
					result = emalloc(result_len + 1);

					memcpy(result, Z_STRVAL_PP(tmp_str), f);
					memcpy((result + f), Z_STRVAL_PP(tmp_repl), Z_STRLEN_PP(tmp_repl));
					memcpy((result + f + Z_STRLEN_PP(tmp_repl)), Z_STRVAL_PP(tmp_str) + f + l, Z_STRLEN_PP(tmp_str) - f - l);
				} else {
					result = emalloc(result_len + 1);
	
					memcpy(result, Z_STRVAL_PP(tmp_str), f);
					memcpy((result + f), Z_STRVAL_PP(tmp_str) + f + l, Z_STRLEN_PP(tmp_str) - f - l);
				}
			} else {
				result_len += Z_STRLEN_PP(repl);

				result = emalloc(result_len + 1);

				memcpy(result, Z_STRVAL_PP(tmp_str), f);
				memcpy((result + f), Z_STRVAL_PP(repl), Z_STRLEN_PP(repl));
				memcpy((result + f + Z_STRLEN_PP(repl)), Z_STRVAL_PP(tmp_str) + f + l, Z_STRLEN_PP(tmp_str) - f - l);
			}

			result[result_len] = '\0';
			add_next_index_stringl(return_value, result, result_len, 0);

			zend_hash_move_forward_ex(Z_ARRVAL_PP(str), &pos_str);
		} /*while*/
	} /* if */
}
/* }}} */




/* {{{ proto string quotemeta(string str)
   Quotes meta characters */
PHP_FUNCTION(quotemeta)
{
	zval **arg;
	char *str, *old;
	char *old_end;
	char *p, *q;
	char c;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(arg);

	old = Z_STRVAL_PP(arg);
	old_end = Z_STRVAL_PP(arg) + Z_STRLEN_PP(arg);

	if (old == old_end) {
		RETURN_FALSE;
	}
	
	str = safe_emalloc(2, Z_STRLEN_PP(arg), 1);
	
	for (p = old, q = str; p != old_end; p++) {
		c = *p;
		switch (c) {
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
				*q++ = '\\';
				/* break is missing _intentionally_ */
			default:
				*q++ = c;
		}
	}
	*q = 0;

	RETURN_STRINGL(erealloc(str, q - str + 1), q - str, 0);
}
/* }}} */

/* {{{ proto int ord(string character)
   Returns ASCII value of character */
PHP_FUNCTION(ord)
{
	zval **str;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	RETURN_LONG((unsigned char) Z_STRVAL_PP(str)[0]);
}
/* }}} */

/* {{{ proto string chr(int ascii)
   Converts ASCII code to a character */
PHP_FUNCTION(chr)
{
	zval **num;
	char temp[2];
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &num) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(num);
	
	temp[0] = (char) Z_LVAL_PP(num);
	temp[1] = 0;

	RETVAL_STRINGL(temp, 1, 1);
}
/* }}} */

/* {{{ proto string ucfirst(string str)
   Makes a string's first character uppercase */
PHP_FUNCTION(ucfirst)
{
	zval **str;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if (!Z_STRLEN_PP(str)) {
		RETURN_EMPTY_STRING();
	}

	ZVAL_STRINGL(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
	*Z_STRVAL_P(return_value) = toupper((unsigned char) *Z_STRVAL_P(return_value));
}
/* }}} */

/* {{{ proto string ucwords(string str)
   Uppercase the first character of every word in a string */
PHP_FUNCTION(ucwords)
{
	zval **str;
	register char *r, *r_end;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if (!Z_STRLEN_PP(str)) {
		RETURN_EMPTY_STRING();
	}

	ZVAL_STRINGL(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
	r = Z_STRVAL_P(return_value);

	*r = toupper((unsigned char) *r);
	for (r_end = r + Z_STRLEN_P(return_value) - 1; r < r_end; ) {
		if (isspace((int) *(unsigned char *)r++)) {
			*r = toupper((unsigned char) *r);
		}
	}
}
/* }}} */

/* {{{ php_strtr
 */
PHPAPI char *php_strtr(char *str, int len, char *str_from, char *str_to, int trlen)
{
	int i;
	unsigned char xlat[256];

	if ((trlen < 1) || (len < 1)) {
		return str;
	}

	for (i = 0; i < 256; xlat[i] = i, i++);

	for (i = 0; i < trlen; i++) {
		xlat[(unsigned char) str_from[i]] = str_to[i];
	}

	for (i = 0; i < len; i++) {
		str[i] = xlat[(unsigned char) str[i]];
	}

	return str;
}
/* }}} */

/* {{{ php_strtr_array
 */
static void php_strtr_array(zval *return_value, char *str, int slen, HashTable *hash)
{
	zval **entry;
	char  *string_key;
	uint   string_key_len;
	zval **trans;
	zval   ctmp;
	ulong num_key;
	int minlen = 128*1024;
	int maxlen = 0, pos, len, found;
	char *key;
	HashPosition hpos;
	smart_str result = {0};
	HashTable tmp_hash;
	
	zend_hash_init(&tmp_hash, 0, NULL, NULL, 0);
	zend_hash_internal_pointer_reset_ex(hash, &hpos);
	while (zend_hash_get_current_data_ex(hash, (void **)&entry, &hpos) == SUCCESS) {
		switch (zend_hash_get_current_key_ex(hash, &string_key, &string_key_len, &num_key, 0, &hpos)) {
			case HASH_KEY_IS_STRING:
				len = string_key_len-1;
				if (len < 1) {
					zend_hash_destroy(&tmp_hash);
					RETURN_FALSE;
				}
				zend_hash_add(&tmp_hash, string_key, string_key_len, entry, sizeof(zval*), NULL);
				if (len > maxlen) {
					maxlen = len;
				}
				if (len < minlen) {
					minlen = len;
				}
				break; 
			
			case HASH_KEY_IS_LONG:
				Z_TYPE(ctmp) = IS_LONG;
				Z_LVAL(ctmp) = num_key;
			
				convert_to_string(&ctmp);
				len = Z_STRLEN(ctmp);
				zend_hash_add(&tmp_hash, Z_STRVAL(ctmp), len+1, entry, sizeof(zval*), NULL);
				zval_dtor(&ctmp);

				if (len > maxlen) {
					maxlen = len;
				}
				if (len < minlen) {
					minlen = len;
				}
				break;
		}
		zend_hash_move_forward_ex(hash, &hpos);
	}

	key = emalloc(maxlen+1);
	pos = 0;

	while (pos < slen) {
		if ((pos + maxlen) > slen) {
			maxlen = slen - pos;
		}

		found = 0;
		memcpy(key, str+pos, maxlen);

		for (len = maxlen; len >= minlen; len--) {
			key[len] = 0;
			
			if (zend_hash_find(&tmp_hash, key, len+1, (void**)&trans) == SUCCESS) {
				char *tval;
				int tlen;
				zval tmp;

				if (Z_TYPE_PP(trans) != IS_STRING) {
					tmp = **trans;
					zval_copy_ctor(&tmp);
					convert_to_string(&tmp);
					tval = Z_STRVAL(tmp);
					tlen = Z_STRLEN(tmp);
				} else {
					tval = Z_STRVAL_PP(trans);
					tlen = Z_STRLEN_PP(trans);
				}

				smart_str_appendl(&result, tval, tlen);
				pos += len;
				found = 1;

				if (Z_TYPE_PP(trans) != IS_STRING) {
					zval_dtor(&tmp);
				}
				break;
			} 
		}

		if (! found) {
			smart_str_appendc(&result, str[pos++]);
		}
	}

	efree(key);
	zend_hash_destroy(&tmp_hash);
	smart_str_0(&result);
	RETVAL_STRINGL(result.c, result.len, 0);
}
/* }}} */

/* {{{ proto string strtr(string str, string from, string to)
   Translates characters in str using given translation tables */
PHP_FUNCTION(strtr)
{								
	zval **str, **from, **to;
	int ac = ZEND_NUM_ARGS();

	if (ac < 2 || ac > 3 || zend_get_parameters_ex(ac, &str, &from, &to) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if (ac == 2 && Z_TYPE_PP(from) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The second argument is not an array.");
		RETURN_FALSE;
	}

	convert_to_string_ex(str);

	/* shortcut for empty string */
	if (Z_STRLEN_PP(str) == 0) {
		RETURN_EMPTY_STRING();
	}

	if (ac == 2) {
		php_strtr_array(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), HASH_OF(*from));
	} else {
		convert_to_string_ex(from);
		convert_to_string_ex(to);

		ZVAL_STRINGL(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
		
		php_strtr(Z_STRVAL_P(return_value),
				  Z_STRLEN_P(return_value),
				  Z_STRVAL_PP(from),
				  Z_STRVAL_PP(to),
				  MIN(Z_STRLEN_PP(from), 
				  Z_STRLEN_PP(to)));
	}
}
/* }}} */

/* {{{ proto string strrev(string str)
   Reverse a string */
PHP_FUNCTION(strrev)
{
	zval **str;
	char *s, *e, *n, *p;
	
	if (ZEND_NUM_ARGS()!=1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	
	n = emalloc(Z_STRLEN_PP(str)+1);
	p = n;
	
	s = Z_STRVAL_PP(str);
	e = s + Z_STRLEN_PP(str);
	
	while (--e>=s) {
		*p++ = *e;
	}
	
	*p = '\0';
	
	RETVAL_STRINGL(n, Z_STRLEN_PP(str), 0);
}
/* }}} */

/* {{{ php_similar_str
 */
static void php_similar_str(const char *txt1, int len1, const char *txt2, int len2, int *pos1, int *pos2, int *max)
{
	char *p, *q;
	char *end1 = (char *) txt1 + len1;
	char *end2 = (char *) txt2 + len2;
	int l;
	
	*max = 0;
	for (p = (char *) txt1; p < end1; p++) {
		for (q = (char *) txt2; q < end2; q++) {
			for (l = 0; (p + l < end1) && (q + l < end2) && (p[l] == q[l]); l++);
			if (l > *max) {
				*max = l;
				*pos1 = p - txt1;
				*pos2 = q - txt2;
			}
		}
	}
}
/* }}} */

/* {{{ php_similar_char
 */
static int php_similar_char(const char *txt1, int len1, const char *txt2, int len2)
{
	int sum;
	int pos1, pos2, max;

	php_similar_str(txt1, len1, txt2, len2, &pos1, &pos2, &max);
	if ((sum = max)) {
		if (pos1 && pos2) {
			sum += php_similar_char(txt1, pos1, 
									txt2, pos2);
		}
		if ((pos1 + max < len1) && (pos2 + max < len2)) {
			sum += php_similar_char(txt1 + pos1 + max, len1 - pos1 - max, 
									txt2 + pos2 + max, len2 - pos2 - max);
		}
	}

	return sum;
}
/* }}} */

/* {{{ proto int similar_text(string str1, string str2 [, float percent])
   Calculates the similarity between two strings */
PHP_FUNCTION(similar_text)
{
	zval **t1, **t2, **percent;
	int ac = ZEND_NUM_ARGS();
	int sim;
	
	if (ac < 2 || ac > 3 || zend_get_parameters_ex(ac, &t1, &t2, &percent) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	convert_to_string_ex(t1);
	convert_to_string_ex(t2);

	if (ac > 2) {
		convert_to_double_ex(percent);
	}
	
	if (Z_STRLEN_PP(t1) + Z_STRLEN_PP(t2) == 0) {
		if (ac > 2) {
			Z_DVAL_PP(percent) = 0;
		}

		RETURN_LONG(0);
	}
	
	sim = php_similar_char(Z_STRVAL_PP(t1), Z_STRLEN_PP(t1), Z_STRVAL_PP(t2), Z_STRLEN_PP(t2));	

	if (ac > 2) {
		Z_DVAL_PP(percent) = sim * 200.0 / (Z_STRLEN_PP(t1) + Z_STRLEN_PP(t2));
	}

	RETURN_LONG(sim);
}
/* }}} */

/* {{{ php_stripslashes
 *
 * be careful, this edits the string in-place */
PHPAPI void php_stripslashes(char *str, int *len TSRMLS_DC)
{
	char *s, *t;
	int l;

	if (len != NULL) {
		l = *len;
	} else {
		l = strlen(str);
	}
	s = str;
	t = str;

	if (PG(magic_quotes_sybase)) {
		while (l > 0) {
			if (*t == '\'') {
				if ((l > 0) && (t[1] == '\'')) {
					t++;
					if (len != NULL) {
						(*len)--;
					}
					l--;
				}
				*s++ = *t++;
			} else if (*t == '\\' && t[1] == '0' && l > 0) {
				*s++='\0';
				t+=2;
				if (len != NULL) {
					(*len)--;
				}
				l--;
			} else {
				*s++ = *t++;
			}
			l--;
		}
		*s = '\0';
		
		return;
	}

	while (l > 0) {
		if (*t == '\\') {
			t++;				/* skip the slash */
			if (len != NULL) {
				(*len)--;
			}
			l--;
			if (l > 0) {
				if (*t == '0') {
					*s++='\0';
					t++;
				} else {
					*s++ = *t++;	/* preserve the next character */
				}
				l--;
			}
		} else {
			*s++ = *t++;
			l--;
		}
	}
	if (s != t) {
		*s = '\0';
	}
}
/* }}} */

/* {{{ proto string addcslashes(string str, string charlist)
   Escapes all chars mentioned in charlist with backslash. It creates octal representations if asked to backslash characters with 8th bit set or with ASCII<32 (except '\n', '\r', '\t' etc...) */
PHP_FUNCTION(addcslashes)
{
	zval **str, **what;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &str, &what) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);
	convert_to_string_ex(what);

	if (Z_STRLEN_PP(str) == 0) {
		RETURN_EMPTY_STRING();
	}

	if (Z_STRLEN_PP(what) == 0) {
		RETURN_STRINGL(Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
	}

	RETURN_STRING(php_addcslashes(Z_STRVAL_PP(str), 
	                              Z_STRLEN_PP(str), 
	                              &Z_STRLEN_P(return_value), 0, 
	                              Z_STRVAL_PP(what),
	                              Z_STRLEN_PP(what) TSRMLS_CC), 0);
}
/* }}} */

/* {{{ proto string addslashes(string str)
   Escapes single quote, double quotes and backslash characters in a string with backslashes */
PHP_FUNCTION(addslashes)
{
	zval **str;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	if (Z_STRLEN_PP(str) == 0) {
		RETURN_EMPTY_STRING();
	}

	RETURN_STRING(php_addslashes(Z_STRVAL_PP(str),
	                             Z_STRLEN_PP(str), 
	                             &Z_STRLEN_P(return_value), 0 
	                             TSRMLS_CC), 0);
}
/* }}} */

/* {{{ proto string stripcslashes(string str)
   Strips backslashes from a string. Uses C-style conventions */
PHP_FUNCTION(stripcslashes)
{
	zval **str;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	ZVAL_STRINGL(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
	php_stripcslashes(Z_STRVAL_P(return_value), &Z_STRLEN_P(return_value));
}
/* }}} */

/* {{{ proto string stripslashes(string str)
   Strips backslashes from a string */
PHP_FUNCTION(stripslashes)
{
	zval **str;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &str) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(str);

	ZVAL_STRINGL(return_value, Z_STRVAL_PP(str), Z_STRLEN_PP(str), 1);
	php_stripslashes(Z_STRVAL_P(return_value), &Z_STRLEN_P(return_value) TSRMLS_CC);
}
/* }}} */

#ifndef HAVE_STRERROR
/* {{{ php_strerror
 */
char *php_strerror(int errnum) 
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	TSRMLS_FETCH();

	if ((unsigned int) errnum < sys_nerr) {
		return(sys_errlist[errnum]);
	}

	(void) sprintf(BG(str_ebuf), "Unknown error: %d", errnum);
	return(BG(str_ebuf));
}
/* }}} */
#endif

/* {{{ php_stripcslashes
 */
PHPAPI void php_stripcslashes(char *str, int *len)
{
	char *source, *target, *end;
	int  nlen = *len, i;
	char numtmp[4];

	for (source=str, end=str+nlen, target=str; source < end; source++) {
		if (*source == '\\' && source+1 < end) {
			source++;
			switch (*source) {
				case 'n':  *target++='\n'; nlen--; break;
				case 'r':  *target++='\r'; nlen--; break;
				case 'a':  *target++='\a'; nlen--; break;
				case 't':  *target++='\t'; nlen--; break;
				case 'v':  *target++='\v'; nlen--; break;
				case 'b':  *target++='\b'; nlen--; break;
				case 'f':  *target++='\f'; nlen--; break;
				case '\\': *target++='\\'; nlen--; break;
				case 'x':
					if (source+1 < end && isxdigit((int)(*(source+1)))) {
						numtmp[0] = *++source;
						if (source+1 < end && isxdigit((int)(*(source+1)))) {
							numtmp[1] = *++source;
							numtmp[2] = '\0';
							nlen-=3;
						} else {
							numtmp[1] = '\0';
							nlen-=2;
						}
						*target++=(char)strtol(numtmp, NULL, 16);
						break;
					}
					/* break is left intentionally */
				default: 
					i=0; 
					while (source < end && *source >= '0' && *source <= '7' && i<3) {
						numtmp[i++] = *source++;
					}
					if (i) {
						numtmp[i]='\0';
						*target++=(char)strtol(numtmp, NULL, 8);
						nlen-=i;
						source--;
					} else {
						*target++=*source;
						nlen--;
					}
			}
		} else {
			*target++=*source;
		}
	}

	if (nlen != 0) {
		*target='\0';
	}

	*len = nlen;
}
/* }}} */
			
/* {{{ php_addcslashes
 */
PHPAPI char *php_addcslashes(char *str, int length, int *new_length, int should_free, char *what, int wlength TSRMLS_DC)
{
	char flags[256];
	char *new_str = safe_emalloc(4, (length?length:(length=strlen(str))), 1);
	char *source, *target;
	char *end;
	char c;
	int  newlen;

	if (!wlength) {
		wlength = strlen(what);
	}

	if (!length) {
		length = strlen(str);
	}

	php_charmask(what, wlength, flags TSRMLS_CC);

	for (source = str, end = source + length, target = new_str; (c = *source) || (source < end); source++) {
		if (flags[(unsigned char)c]) {
			if ((unsigned char) c < 32 || (unsigned char) c > 126) {
				*target++ = '\\';
				switch (c) {
					case '\n': *target++ = 'n'; break;
					case '\t': *target++ = 't'; break;
					case '\r': *target++ = 'r'; break;
					case '\a': *target++ = 'a'; break;
					case '\v': *target++ = 'v'; break;
					case '\b': *target++ = 'b'; break;
					case '\f': *target++ = 'f'; break;
					default: target += sprintf(target, "%03o", (unsigned char) c);
				}
				continue;
			} 
			*target++ = '\\';
		}
		*target++ = c;
	}
	*target = 0;
	newlen = target - new_str;
	if (target - new_str < length * 4) {
		new_str = erealloc(new_str, newlen + 1);
	}
	if (new_length) {
		*new_length = newlen;
	}
	if (should_free) {
		STR_FREE(str);
	}
	return new_str;
}
/* }}} */

/* {{{ php_addslashes
 */
PHPAPI char *php_addslashes(char *str, int length, int *new_length, int should_free TSRMLS_DC)
{
	return php_addslashes_ex(str, length, new_length, should_free, 0 TSRMLS_CC);
}
/* }}} */

/* {{{ php_addslashes_ex
 */
PHPAPI char *php_addslashes_ex(char *str, int length, int *new_length, int should_free, int ignore_sybase TSRMLS_DC)
{
	/* maximum string length, worst case situation */
	char *new_str;
	char *source, *target;
	char *end;
	int local_new_length;
 	        
	if (!new_length) {
		new_length = &local_new_length;
	}
	if (!str) {
		*new_length = 0;
		return str;
	}
	new_str = (char *) safe_emalloc(2, (length ? length : (length = strlen(str))), 1);
	source = str;
	end = source + length;
	target = new_str;
	
	if (!ignore_sybase && PG(magic_quotes_sybase)) {
		while (source < end) {
			switch (*source) {
				case '\0':
					*target++ = '\\';
					*target++ = '0';
					break;
				case '\'':
					*target++ = '\'';
					*target++ = '\'';
					break;
				default:
					*target++ = *source;
					break;
			}
			source++;
		}
	} else {
		while (source < end) {
			switch (*source) {
				case '\0':
					*target++ = '\\';
					*target++ = '0';
					break;
				case '\'':
				case '\"':
				case '\\':
					*target++ = '\\';
					/* break is missing *intentionally* */
				default:
					*target++ = *source;
					break;	
			}
		
			source++;
		}
	}
	
	*target = 0;
	*new_length = target - new_str;
	if (should_free) {
		STR_FREE(str);
	}
	new_str = (char *) erealloc(new_str, *new_length + 1);
	return new_str;
}
/* }}} */

#define _HEB_BLOCK_TYPE_ENG 1
#define _HEB_BLOCK_TYPE_HEB 2
#define isheb(c)      (((((unsigned char) c) >= 224) && (((unsigned char) c) <= 250)) ? 1 : 0)
#define _isblank(c)   (((((unsigned char) c) == ' '  || ((unsigned char) c) == '\t')) ? 1 : 0)
#define _isnewline(c) (((((unsigned char) c) == '\n' || ((unsigned char) c) == '\r')) ? 1 : 0)

/* {{{ php_char_to_str_ex
 */
PHPAPI int php_char_to_str_ex(char *str, uint len, char from, char *to, int to_len, zval *result, int case_sensitivity, int *replace_count)
{
	int char_count = 0;
	int replaced = 0;
	char *source, *target, *tmp, *source_end=str+len, *tmp_end = NULL;
	
	for (source = str; source < source_end; source++) {
		if ((case_sensitivity && *source == from) || (!case_sensitivity && tolower(*source) == tolower(from))) {
			char_count++;
		}
	}

	if (char_count == 0 && case_sensitivity) {
		ZVAL_STRINGL(result, str, len, 1);
		return 0;
	}
	
	Z_STRLEN_P(result) = len + (char_count * (to_len - 1));
	Z_STRVAL_P(result) = target = emalloc(Z_STRLEN_P(result) + 1);
	Z_TYPE_P(result) = IS_STRING;
	
	for (source = str; source < source_end; source++) {
		if ((case_sensitivity && *source == from) || (!case_sensitivity && tolower(*source) == tolower(from))) {
			replaced = 1;
			if (replace_count) {
				*replace_count += 1;
			}
			for (tmp = to, tmp_end = tmp+to_len; tmp < tmp_end; tmp++) {
				*target = *tmp;
				target++;
			}
		} else {
			*target = *source;
			target++;
		}
	}
	*target = 0;
	return replaced;
}
/* }}} */

/* {{{ php_char_to_str
 */
PHPAPI int php_char_to_str(char *str, uint len, char from, char *to, int to_len, zval *result)
{
	return php_char_to_str_ex(str, len, from, to, to_len, result, 1, NULL);
}
/* }}} */

/* {{{ php_str_to_str_ex
 */
PHPAPI char *php_str_to_str_ex(char *haystack, int length, 
	char *needle, int needle_len, char *str, int str_len, int *_new_length, int case_sensitivity, int *replace_count)
{
	char *new_str;

	if (needle_len < length) {
		char *end, *haystack_dup = NULL, *needle_dup = NULL;
		char *e, *s, *p, *r;

		if (needle_len == str_len) {
			new_str = estrndup(haystack, length);
			*_new_length = length;

			if (case_sensitivity) {
				end = new_str + length;
				for (p = new_str; (r = php_memnstr(p, needle, needle_len, end)); p = r + needle_len) {
					memcpy(r, str, str_len);
					if (replace_count) {
						(*replace_count)++;
					}
				}
			} else {
				haystack_dup = estrndup(haystack, length);
				needle_dup = estrndup(needle, needle_len);
				php_strtolower(haystack_dup, length);
				php_strtolower(needle_dup, needle_len);
				end = haystack_dup + length;
				for (p = haystack_dup; (r = php_memnstr(p, needle_dup, needle_len, end)); p = r + needle_len) {
					memcpy(new_str + (r - haystack_dup), str, str_len);
					if (replace_count) {
						(*replace_count)++;
					}
				}
				efree(haystack_dup);
				efree(needle_dup);
			}
			return new_str;
		} else {
			if (!case_sensitivity) {
				haystack_dup = estrndup(haystack, length);
				needle_dup = estrndup(needle, needle_len);
				php_strtolower(haystack_dup, length);
				php_strtolower(needle_dup, needle_len);
			}

			if (str_len < needle_len) {
				new_str = emalloc(length + 1);
			} else {
				int count = 0;
				char *o, *n, *endp;

				if (case_sensitivity) {
					o = haystack;
					n = needle;
				} else {
					o = haystack_dup;
					n = needle_dup;
				}
				endp = o + length;

				while ((o = php_memnstr(o, n, needle_len, endp))) {
					o += needle_len;
					count++;
				}
				if (count == 0) {
					/* Needle doesn't occur, shortcircuit the actual replacement. */
					if (haystack_dup) {
						efree(haystack_dup);
					}
					if (needle_dup) {
						efree(needle_dup);
					}
					new_str = estrndup(haystack, length);
					if (_new_length) {
						*_new_length = length;
					}
					return new_str;
				} else {
					new_str = safe_emalloc(count, str_len - needle_len, length + 1);
				}
			}

			e = s = new_str;

			if (case_sensitivity) {
				end = haystack + length;
				for (p = haystack; (r = php_memnstr(p, needle, needle_len, end)); p = r + needle_len) {
					memcpy(e, p, r - p);
					e += r - p;
					memcpy(e, str, str_len);
					e += str_len;
					if (replace_count) {
						(*replace_count)++;
					}
				}

				if (p < end) {
					memcpy(e, p, end - p);
					e += end - p;
				}
			} else {
				end = haystack_dup + length;

				for (p = haystack_dup; (r = php_memnstr(p, needle_dup, needle_len, end)); p = r + needle_len) {
					memcpy(e, haystack + (p - haystack_dup), r - p);
					e += r - p;
					memcpy(e, str, str_len);
					e += str_len;
					if (replace_count) {
						(*replace_count)++;
					}
				}

				if (p < end) {
					memcpy(e, haystack + (p - haystack_dup), end - p);
					e += end - p;
				}
			}

			if (haystack_dup) {
				efree(haystack_dup);
			}
			if (needle_dup) {
				efree(needle_dup);
			}

			*e = '\0';
			*_new_length = e - s;

			new_str = erealloc(new_str, *_new_length + 1);
			return new_str;
		}
	} else if (needle_len > length) {
nothing_todo:
		*_new_length = length;
		new_str = estrndup(haystack, length);
		return new_str;
	} else {
		if (case_sensitivity ? strncmp(haystack, needle, length) : strncasecmp(haystack, needle, length)) {
			goto nothing_todo;
		} else {
			*_new_length = str_len;
			new_str = estrndup(str, str_len);
			if (replace_count) {
				(*replace_count)++;
			}
			return new_str;
		}
	}

}
/* }}} */

/* {{{ php_str_to_str
 */
PHPAPI char *php_str_to_str(char *haystack, int length, 
	char *needle, int needle_len, char *str, int str_len, int *_new_length)
{
	return php_str_to_str_ex(haystack, length, needle, needle_len, str, str_len, _new_length, 1, NULL);
} 
/* }}}
 */

/* {{{ php_str_replace_in_subject
 */
static void php_str_replace_in_subject(zval *search, zval *replace, zval **subject, zval *result, int case_sensitivity, int *replace_count)
{
	zval		**search_entry,
				**replace_entry = NULL,
				  temp_result;
	char		*replace_value = NULL;
	int			 replace_len = 0;

	/* Make sure we're dealing with strings. */	
	convert_to_string_ex(subject);
	Z_TYPE_P(result) = IS_STRING;
	if (Z_STRLEN_PP(subject) == 0) {
		ZVAL_STRINGL(result, "", 0, 1);
		return;
	}
	
	/* If search is an array */
	if (Z_TYPE_P(search) == IS_ARRAY) {
		/* Duplicate subject string for repeated replacement */
		*result = **subject;
		zval_copy_ctor(result);
		INIT_PZVAL(result);
		
		zend_hash_internal_pointer_reset(Z_ARRVAL_P(search));

		if (Z_TYPE_P(replace) == IS_ARRAY) {
			zend_hash_internal_pointer_reset(Z_ARRVAL_P(replace));
		} else {
			/* Set replacement value to the passed one */
			replace_value = Z_STRVAL_P(replace);
			replace_len = Z_STRLEN_P(replace);
		}

		/* For each entry in the search array, get the entry */
		while (zend_hash_get_current_data(Z_ARRVAL_P(search), (void **) &search_entry) == SUCCESS) {
			/* Make sure we're dealing with strings. */	
			SEPARATE_ZVAL(search_entry);
			convert_to_string(*search_entry);
			if (Z_STRLEN_PP(search_entry) == 0) {
				zend_hash_move_forward(Z_ARRVAL_P(search));
				if (Z_TYPE_P(replace) == IS_ARRAY) {
					zend_hash_move_forward(Z_ARRVAL_P(replace));
				}
				continue;
			}

			/* If replace is an array. */
			if (Z_TYPE_P(replace) == IS_ARRAY) {
				/* Get current entry */
				if (zend_hash_get_current_data(Z_ARRVAL_P(replace), (void **)&replace_entry) == SUCCESS) {
					/* Make sure we're dealing with strings. */	
					convert_to_string_ex(replace_entry);
					
					/* Set replacement value to the one we got from array */
					replace_value = Z_STRVAL_PP(replace_entry);
					replace_len = Z_STRLEN_PP(replace_entry);

					zend_hash_move_forward(Z_ARRVAL_P(replace));
				} else {
					/* We've run out of replacement strings, so use an empty one. */
					replace_value = "";
					replace_len = 0;
				}
			}
			
			if (Z_STRLEN_PP(search_entry) == 1) {
				php_char_to_str_ex(Z_STRVAL_P(result),
								Z_STRLEN_P(result),
								Z_STRVAL_PP(search_entry)[0],
								replace_value,
								replace_len,
								&temp_result,
								case_sensitivity,
								replace_count);
			} else if (Z_STRLEN_PP(search_entry) > 1) {
				Z_STRVAL(temp_result) = php_str_to_str_ex(Z_STRVAL_P(result), Z_STRLEN_P(result),
														   Z_STRVAL_PP(search_entry), Z_STRLEN_PP(search_entry),
														   replace_value, replace_len, &Z_STRLEN(temp_result), case_sensitivity, replace_count);
			}

			efree(Z_STRVAL_P(result));
			Z_STRVAL_P(result) = Z_STRVAL(temp_result);
			Z_STRLEN_P(result) = Z_STRLEN(temp_result);

			if (Z_STRLEN_P(result) == 0) {
				return;
			}

			zend_hash_move_forward(Z_ARRVAL_P(search));
		}
	} else {
		if (Z_STRLEN_P(search) == 1) {
			php_char_to_str_ex(Z_STRVAL_PP(subject),
							Z_STRLEN_PP(subject),
							Z_STRVAL_P(search)[0],
							Z_STRVAL_P(replace),
							Z_STRLEN_P(replace),
							result,
							case_sensitivity,
							replace_count);
		} else if (Z_STRLEN_P(search) > 1) {
			Z_STRVAL_P(result) = php_str_to_str_ex(Z_STRVAL_PP(subject), Z_STRLEN_PP(subject),
													Z_STRVAL_P(search), Z_STRLEN_P(search),
													Z_STRVAL_P(replace), Z_STRLEN_P(replace), &Z_STRLEN_P(result), case_sensitivity, replace_count);
		} else {
			*result = **subject;
			zval_copy_ctor(result);
			INIT_PZVAL(result);
		}
	}
}
/* }}} */

/* {{{ php_str_replace_common
 */
static void php_str_replace_common(INTERNAL_FUNCTION_PARAMETERS, int case_sensitivity)
{
	zval **subject, **search, **replace, **subject_entry, **zcount;
	zval *result;
	char *string_key;
	uint string_key_len;
	ulong num_key;
	int count = 0;
	int argc = ZEND_NUM_ARGS();

	if (argc < 3 || argc > 4 ||
	   zend_get_parameters_ex(argc, &search, &replace, &subject, &zcount) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	SEPARATE_ZVAL(search);
	SEPARATE_ZVAL(replace);
	SEPARATE_ZVAL(subject);

	/* Make sure we're dealing with strings and do the replacement. */
	if (Z_TYPE_PP(search) != IS_ARRAY) {
		convert_to_string_ex(search);
		convert_to_string_ex(replace);
	} else if (Z_TYPE_PP(replace) != IS_ARRAY) {
		convert_to_string_ex(replace);
	}

	/* if subject is an array */
	if (Z_TYPE_PP(subject) == IS_ARRAY) {
		array_init(return_value);
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(subject));

		/* For each subject entry, convert it to string, then perform replacement
		   and add the result to the return_value array. */
		while (zend_hash_get_current_data(Z_ARRVAL_PP(subject), (void **)&subject_entry) == SUCCESS) {
			if (Z_TYPE_PP(subject_entry) != IS_ARRAY && Z_TYPE_PP(subject_entry) != IS_OBJECT) {
				MAKE_STD_ZVAL(result);
				SEPARATE_ZVAL(subject_entry);
				php_str_replace_in_subject(*search, *replace, subject_entry, result, case_sensitivity, (argc > 3) ? &count : NULL);
			} else {
				ALLOC_ZVAL(result);
				ZVAL_ADDREF(*subject_entry);
				COPY_PZVAL_TO_ZVAL(*result, *subject_entry);
			}
			/* Add to return array */
			switch (zend_hash_get_current_key_ex(Z_ARRVAL_PP(subject), &string_key,
												&string_key_len, &num_key, 0, NULL)) {
				case HASH_KEY_IS_STRING:
					add_assoc_zval_ex(return_value, string_key, string_key_len, result);
					break;

				case HASH_KEY_IS_LONG:
					add_index_zval(return_value, num_key, result);
					break;
			}
		
			zend_hash_move_forward(Z_ARRVAL_PP(subject));
		}
	} else {	/* if subject is not an array */
		php_str_replace_in_subject(*search, *replace, subject, return_value, case_sensitivity, (argc > 3) ? &count : NULL);
	}	
	if (argc > 3) {
		zval_dtor(*zcount);
		ZVAL_LONG(*zcount, count);
	}
}
/* }}} */

/* {{{ proto mixed str_replace(mixed search, mixed replace, mixed subject [, int &replace_count])
   Replaces all occurrences of search in haystack with replace */
PHP_FUNCTION(str_replace)
{
	php_str_replace_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto mixed str_ireplace(mixed search, mixed replace, mixed subject [, int &replace_count])
   Replaces all occurrences of search in haystack with replace / case-insensitive */
PHP_FUNCTION(str_ireplace)
{
	php_str_replace_common(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ php_hebrev
 *
 * Converts Logical Hebrew text (Hebrew Windows style) to Visual text
 * Cheers/complaints/flames - Zeev Suraski <zeev@php.net>
 */
static void php_hebrev(INTERNAL_FUNCTION_PARAMETERS, int convert_newlines)
{
	zval **str, **max_chars_per_line;
	char *heb_str, *tmp, *target, *broken_str;
	int block_start, block_end, block_type, block_length, i;
	long max_chars=0;
	int begin, end, char_count, orig_begin;

	
	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &str) == FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &str, &max_chars_per_line) == FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(max_chars_per_line);
			max_chars = Z_LVAL_PP(max_chars_per_line);
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	convert_to_string_ex(str);
	
	if (Z_STRLEN_PP(str) == 0) {
		RETURN_FALSE;
	}

	tmp = Z_STRVAL_PP(str);
	block_start=block_end=0;

	heb_str = (char *) emalloc(Z_STRLEN_PP(str)+1);
	target = heb_str+Z_STRLEN_PP(str);
	*target = 0;
	target--;

	block_length=0;

	if (isheb(*tmp)) {
		block_type = _HEB_BLOCK_TYPE_HEB;
	} else {
		block_type = _HEB_BLOCK_TYPE_ENG;
	}
	
	do {
		if (block_type == _HEB_BLOCK_TYPE_HEB) {
			while ((isheb((int)*(tmp+1)) || _isblank((int)*(tmp+1)) || ispunct((int)*(tmp+1)) || (int)*(tmp+1)=='\n' ) && block_end<Z_STRLEN_PP(str)-1) {
				tmp++;
				block_end++;
				block_length++;
			}
			for (i = block_start; i<= block_end; i++) {
				*target = Z_STRVAL_PP(str)[i];
				switch (*target) {
					case '(':
						*target = ')';
						break;
					case ')':
						*target = '(';
						break;
					case '[':
						*target = ']';
						break;
					case ']':
						*target = '[';
						break;
					case '{':
						*target = '}';
						break;
					case '}':
						*target = '{';
						break;
					case '<':
						*target = '>';
						break;
					case '>':
						*target = '<';
						break;
					case '\\':
						*target = '/';
						break;
					case '/':
						*target = '\\';
						break;
					default:
						break;
				}
				target--;
			}
			block_type = _HEB_BLOCK_TYPE_ENG;
		} else {
			while (!isheb(*(tmp+1)) && (int)*(tmp+1)!='\n' && block_end < Z_STRLEN_PP(str)-1) {
				tmp++;
				block_end++;
				block_length++;
			}
			while ((_isblank((int)*tmp) || ispunct((int)*tmp)) && *tmp!='/' && *tmp!='-' && block_end > block_start) {
				tmp--;
				block_end--;
			}
			for (i = block_end; i >= block_start; i--) {
				*target = Z_STRVAL_PP(str)[i];
				target--;
			}
			block_type = _HEB_BLOCK_TYPE_HEB;
		}
		block_start=block_end+1;
	} while (block_end < Z_STRLEN_PP(str)-1);


	broken_str = (char *) emalloc(Z_STRLEN_PP(str)+1);
	begin=end=Z_STRLEN_PP(str)-1;
	target = broken_str;
		
	while (1) {
		char_count=0;
		while ((!max_chars || char_count < max_chars) && begin > 0) {
			char_count++;
			begin--;
			if (begin <= 0 || _isnewline(heb_str[begin])) {
				while (begin > 0 && _isnewline(heb_str[begin-1])) {
					begin--;
					char_count++;
				}
				break;
			}
		}
		if (char_count == max_chars) { /* try to avoid breaking words */
			int new_char_count=char_count, new_begin=begin;
			
			while (new_char_count > 0) {
				if (_isblank(heb_str[new_begin]) || _isnewline(heb_str[new_begin])) {
					break;
				}
				new_begin++;
				new_char_count--;
			}
			if (new_char_count > 0) {
				char_count=new_char_count;
				begin=new_begin;
			}
		}
		orig_begin=begin;
		
		if (_isblank(heb_str[begin])) {
			heb_str[begin]='\n';
		}
		while (begin <= end && _isnewline(heb_str[begin])) { /* skip leading newlines */
			begin++;
		}
		for (i = begin; i <= end; i++) { /* copy content */
			*target = heb_str[i];
			target++;
		}
		for (i = orig_begin; i <= end && _isnewline(heb_str[i]); i++) {
			*target = heb_str[i];
			target++;
		}
		begin=orig_begin;

		if (begin <= 0) {
			*target = 0;
			break;
		}
		begin--;
		end=begin;
	}
	efree(heb_str);

	if (convert_newlines) {
		php_char_to_str(broken_str, Z_STRLEN_PP(str),'\n', "<br />\n", 7, return_value);
		efree(broken_str);
	} else {
		Z_STRVAL_P(return_value) = broken_str;
		Z_STRLEN_P(return_value) = Z_STRLEN_PP(str);
		Z_TYPE_P(return_value) = IS_STRING;
	}
}
/* }}} */

/* {{{ proto string hebrev(string str [, int max_chars_per_line])
   Converts logical Hebrew text to visual text */
PHP_FUNCTION(hebrev)
{
	php_hebrev(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto string hebrevc(string str [, int max_chars_per_line])
   Converts logical Hebrew text to visual text with newline conversion */
PHP_FUNCTION(hebrevc)
{
	php_hebrev(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */


/* {{{ proto string nl2br(string str)
   Converts newlines to HTML line breaks */
PHP_FUNCTION(nl2br)
{
	/* in brief this inserts <br /> before matched regexp \n\r?|\r\n? */
	zval	**zstr;
	char	*tmp, *str;
	int	new_length;
	char	*end, *target;
	int	repl_cnt = 0;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &zstr) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(zstr);

	str = Z_STRVAL_PP(zstr);
	end = str + Z_STRLEN_PP(zstr);
	
	/* it is really faster to scan twice and allocate mem once insted scanning once
	   and constantly reallocing */
	while (str < end) {
		if (*str == '\r') {
			if (*(str+1) == '\n') {
				str++;
			}
			repl_cnt++;
		} else if (*str == '\n') {
			if (*(str+1) == '\r') {
				str++;
			}
			repl_cnt++;
		}
		
		str++;
	}
	
	if (repl_cnt == 0) {
		RETURN_STRINGL(Z_STRVAL_PP(zstr), Z_STRLEN_PP(zstr), 1);
	}

	new_length = Z_STRLEN_PP(zstr) + repl_cnt * (sizeof("<br />") - 1);
	tmp = target = emalloc(new_length + 1);

	str = Z_STRVAL_PP(zstr);

	while (str < end) {
		switch (*str) {
			case '\r':
			case '\n':
				*target++ = '<';
				*target++ = 'b';
				*target++ = 'r';
				*target++ = ' ';
				*target++ = '/';
				*target++ = '>';
				
				if ((*str == '\r' && *(str+1) == '\n') || (*str == '\n' && *(str+1) == '\r')) {
					*target++ = *str++;
				}
				/* lack of a break; is intentional */
			default:
				*target++ = *str;
		}
	
		str++;
	}
	
	*target = '\0';

	RETURN_STRINGL(tmp, new_length, 0);
}
/* }}} */


/* {{{ proto string strip_tags(string str [, string allowable_tags])
   Strips HTML and PHP tags from a string */
PHP_FUNCTION(strip_tags)
{
	char *buf;
	zval **str, **allow=NULL;
	char *allowed_tags=NULL;
	int allowed_tags_len=0;
	size_t retval_len;

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &str) == FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &str, &allow) == FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(allow);
			allowed_tags = Z_STRVAL_PP(allow);
			allowed_tags_len = Z_STRLEN_PP(allow);
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	convert_to_string_ex(str);
	buf = estrndup(Z_STRVAL_PP(str), Z_STRLEN_PP(str));
	retval_len = php_strip_tags(buf, Z_STRLEN_PP(str), NULL, allowed_tags, allowed_tags_len);
	RETURN_STRINGL(buf, retval_len, 0);
}
/* }}} */

/* {{{ proto string setlocale(mixed category, string locale [, string ...])
   Set locale information */
PHP_FUNCTION(setlocale)
{
	pval ***args = (pval ***) safe_emalloc(sizeof(pval **), ZEND_NUM_ARGS(), 0);
	zval **pcategory, **plocale;
	int i, cat, n_args=ZEND_NUM_ARGS();
	char *loc, *retval;

	if (zend_get_parameters_array_ex(n_args, args) == FAILURE || n_args < 2) {
		efree(args);
		WRONG_PARAM_COUNT;
	}
#ifdef HAVE_SETLOCALE
	pcategory = args[0];
	if (Z_TYPE_PP(pcategory) == IS_LONG) {
		convert_to_long_ex(pcategory);	
		cat = Z_LVAL_PP(pcategory);
	} else { /* FIXME: The following behaviour should be removed. */
		char *category;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Passing locale category name as string is deprecated. Use the LC_* -constants instead.");
		convert_to_string_ex(pcategory);
		category = Z_STRVAL_P(*pcategory);

		if (!strcasecmp ("LC_ALL", category))
			cat = LC_ALL;
		else if (!strcasecmp ("LC_COLLATE", category))
			cat = LC_COLLATE;
		else if (!strcasecmp ("LC_CTYPE", category))
			cat = LC_CTYPE;
#ifdef LC_MESSAGES
		else if (!strcasecmp ("LC_MESSAGES", category))
			cat = LC_MESSAGES;
#endif
		else if (!strcasecmp ("LC_MONETARY", category))
			cat = LC_MONETARY;
		else if (!strcasecmp ("LC_NUMERIC", category))
			cat = LC_NUMERIC;
		else if (!strcasecmp ("LC_TIME", category))
			cat = LC_TIME;
		else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid locale category name %s, must be one of LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, or LC_TIME.", category);
			efree(args);
			RETURN_FALSE;
		}
	}

	if (Z_TYPE_PP(args[1]) == IS_ARRAY) {
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(args[1]));
		i=0; /* not needed in this case: only kill a compiler warning */
	} else {
		i=1;
	}
	while (1) {
		if (Z_TYPE_PP(args[1]) == IS_ARRAY) {
			if (!zend_hash_num_elements(Z_ARRVAL_PP(args[1]))) {
				break;
			}
			zend_hash_get_current_data(Z_ARRVAL_PP(args[1]),(void **)&plocale);
		} else {
			plocale = args[i];
		}

		convert_to_string_ex(plocale);
		
		if (!strcmp ("0", Z_STRVAL_PP(plocale))) {
			loc = NULL;
		} else {
			loc = Z_STRVAL_PP(plocale);
		}
		
		retval = setlocale (cat, loc);
		if (retval) {
			/* Remember if locale was changed */
			if (loc) {
				STR_FREE(BG(locale_string));
				BG(locale_string) = estrdup(retval);
			}
			
			efree(args);
			RETVAL_STRING(retval, 1);
			
			return;
		}
		
		if (Z_TYPE_PP(args[1]) == IS_ARRAY) {
			if (zend_hash_move_forward(Z_ARRVAL_PP(args[1])) == FAILURE) break;
		} else {
			if (++i >= n_args) break;
		}
	}

#endif
	efree(args);

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto void parse_str(string encoded_string [, array result])
   Parses GET/POST/COOKIE data and sets global variables */
PHP_FUNCTION(parse_str)
{
	zval **arg;
	zval **arrayArg;
	zval *sarg;
	char *res = NULL;
	int argCount;
	int old_rg;

	argCount = ZEND_NUM_ARGS();
	if (argCount < 1 || argCount > 2 || zend_get_parameters_ex(argCount, &arg, &arrayArg) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(arg);
	sarg = *arg;
	if (Z_STRVAL_P(sarg) && *Z_STRVAL_P(sarg)) {
		res = estrndup(Z_STRVAL_P(sarg), Z_STRLEN_P(sarg));
	}

	old_rg = PG(register_globals);
	if (argCount == 1) {
		PG(register_globals) = 1;
		sapi_module.treat_data(PARSE_STRING, res, NULL TSRMLS_CC);
	} else 	{
		PG(register_globals) = 0;
		/* Clear out the array that was passed in. */
		zval_dtor(*arrayArg);
		array_init(*arrayArg);
		
		sapi_module.treat_data(PARSE_STRING, res, *arrayArg TSRMLS_CC);
	}
	PG(register_globals) = old_rg;
}
/* }}} */

#define PHP_TAG_BUF_SIZE 1023

/* {{{ php_tag_find
 *
 * Check if tag is in a set of tags 
 *
 * states:
 * 
 * 0 start tag
 * 1 first non-whitespace char seen
 */
int php_tag_find(char *tag, int len, char *set) {
	char c, *n, *t;
	int state=0, done=0;
	char *norm = emalloc(len+1);

	n = norm;
	t = tag;
	c = tolower(*t);
	/* 
	   normalize the tag removing leading and trailing whitespace
	   and turn any <a whatever...> into just <a> and any </tag>
	   into <tag>
	*/
	if (!len) {
		return 0;
	}
	while (!done) {
		switch (c) {
			case '<':
				*(n++) = c;
				break;
			case '>':
				done =1;
				break;
			default:
				if (!isspace((int)c)) {
					if (state == 0) {
						state=1;
						if (c != '/')
							*(n++) = c;
					} else {
						*(n++) = c;
					}
				} else {
					if (state == 1)
						done=1;
				}
				break;
		}
		c = tolower(*(++t));
	}  
	*(n++) = '>';
	*n = '\0'; 
	if (strstr(set, norm)) {
		done=1;
	} else {
		done=0;
	}
	efree(norm);
	return done;
}
/* }}} */

/* {{{ php_strip_tags
 
	A simple little state-machine to strip out html and php tags 
	
	State 0 is the output state, State 1 means we are inside a
	normal html tag and state 2 means we are inside a php tag.

	The state variable is passed in to allow a function like fgetss
	to maintain state across calls to the function.

	lc holds the last significant character read and br is a bracket
	counter.

	When an allow string is passed in we keep track of the string
	in state 1 and when the tag is closed check it against the
	allow string to see if we should allow it.

	swm: Added ability to strip <?xml tags without assuming it PHP
	code.
*/
PHPAPI size_t php_strip_tags(char *rbuf, int len, int *stateptr, char *allow, int allow_len)
{
	char *tbuf, *buf, *p, *tp, *rp, c, lc;
	int br, i=0, depth=0;
	int state = 0;

	if (stateptr)
		state = *stateptr;

	buf = estrndup(rbuf, len);
	c = *buf;
	lc = '\0';
	p = buf;
	rp = rbuf;
	br = 0;
	if (allow) {
		php_strtolower(allow, allow_len);
		tbuf = emalloc(PHP_TAG_BUF_SIZE+1);
		tp = tbuf;
	} else {
		tbuf = tp = NULL;
	}

	while (i < len) {
		switch (c) {
			case '\0':
				break;
			case '<':
				if (isspace(*(p + 1))) {
					goto reg_char;
				}
				if (state == 0) {
					lc = '<';
					state = 1;
					if (allow) {
						tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
						*(tp++) = '<';
					}
				} else if (state == 1) {
					depth++;
				}
				break;

			case '(':
				if (state == 2) {
					if (lc != '"' && lc != '\'') {
						lc = '(';
						br++;
					}
				} else if (allow && state == 1) {
					tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
					*(tp++) = c;
				} else if (state == 0) {
					*(rp++) = c;
				}
				break;	

			case ')':
				if (state == 2) {
					if (lc != '"' && lc != '\'') {
						lc = ')';
						br--;
					}
				} else if (allow && state == 1) {
					tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
					*(tp++) = c;
				} else if (state == 0) {
					*(rp++) = c;
				}
				break;	

			case '>':
				if (depth) {
					depth--;
					break;
				}
			
				switch (state) {
					case 1: /* HTML/XML */
						lc = '>';
						state = 0;
						if (allow) {
							tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
							*(tp++) = '>';
							*tp='\0';
							if (php_tag_find(tbuf, tp-tbuf, allow)) {
								memcpy(rp, tbuf, tp-tbuf);
								rp += tp-tbuf;
							}
							tp = tbuf;
						}
						break;
						
					case 2: /* PHP */
						if (!br && lc != '\"' && *(p-1) == '?') {
							state = 0;
							tp = tbuf;
						}
						break;
						
					case 3:
						state = 0;
						tp = tbuf;
						break;

					case 4: /* JavaScript/CSS/etc... */
						if (p >= buf + 2 && *(p-1) == '-' && *(p-2) == '-') {
							state = 0;
							tp = tbuf;
						}
						break;

					default:
						*(rp++) = c;
						break;
				}
				break;

			case '"':
			case '\'':
				if (state == 2 && *(p-1) != '\\') {
					if (lc == c) {
						lc = '\0';
					} else if (lc != '\\') {
						lc = c;
					}
				} else if (state == 0) {
					*(rp++) = c;
				} else if (allow && state == 1) {
					tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
					*(tp++) = c;
				}
				break;
			
			case '!': 
				/* JavaScript & Other HTML scripting languages */
				if (state == 1 && *(p-1) == '<') { 
					state = 3;
					lc = c;
				} else {
					if (state == 0) {
						*(rp++) = c;
					} else if (allow && state == 1) {
						tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
						*(tp++) = c;
					}
				}
				break;

			case '-':
				if (state == 3 && p >= buf + 2 && *(p-1) == '-' && *(p-2) == '!') {
					state = 4;
				} else {
					goto reg_char;
				}
				break;

			case '?':

				if (state == 1 && *(p-1) == '<') { 
					br=0;
					state=2;
					break;
				}

			case 'E':
			case 'e':
				/* !DOCTYPE exception */
				if (state==3 && p > buf+6
						     && tolower(*(p-1)) == 'p'
					         && tolower(*(p-2)) == 'y'
						     && tolower(*(p-3)) == 't'
						     && tolower(*(p-4)) == 'c'
						     && tolower(*(p-5)) == 'o'
						     && tolower(*(p-6)) == 'd') {
					state = 1;
					break;
				}
				/* fall-through */

			case 'l':

				/* swm: If we encounter '<?xml' then we shouldn't be in
				 * state == 2 (PHP). Switch back to HTML.
				 */

				if (state == 2 && p > buf+2 && *(p-1) == 'm' && *(p-2) == 'x') {
					state = 1;
					break;
				}

				/* fall-through */
			default:
reg_char:
				if (state == 0) {
					*(rp++) = c;
				} else if (allow && state == 1) {
					tp = ((tp-tbuf) >= PHP_TAG_BUF_SIZE ? tbuf: tp);
					*(tp++) = c;
				} 
				break;
		}
		c = *(++p);
		i++;
	}	
	if (rp < rbuf + len) {
		*rp = '\0';
	}
	efree(buf);
	if (allow)
		efree(tbuf);
	if (stateptr)
		*stateptr = state;

	return (size_t)(rp - rbuf);
}
/* }}} */

/* {{{ proto string str_repeat(string input, int mult)
   Returns the input string repeat mult times */
PHP_FUNCTION(str_repeat)
{
	void		*input_str;		/* Input string */
	int32_t		input_str_len;
	int32_t		input_str_chars;
	zend_uchar	input_str_type;
	long		mult;			/* Multiplier */
	void		*result;		/* Resulting string */
	int32_t		result_len;		/* Length of the resulting string, in bytes */
	int32_t		result_chars;	/* Chars/UChars in resulting string */

	if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "tl", &input_str,
							   &input_str_chars, &input_str_type, &mult) == FAILURE ) {
		return;
	}

	if ( mult < 0 ) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Second argument has to be greater than or equal to 0");
		return;
	}

	/* Don't waste our time if input is empty or if the multiplier is zero */
	if ( input_str_chars == 0 || mult == 0 ) {
		if ( input_str_type == IS_UNICODE ) {
			RETURN_UNICODEL(USTR_MAKE(""), 0, 0);
		} else if ( input_str_type == IS_STRING ) {
			RETURN_STRINGL("", 0, 1);
		} else {
			RETURN_BINARYL("", 0, 1);
		}
	}

	/* Initialize the result string */	
	result_chars = (input_str_chars * mult) + 1;
	if ( input_str_type == IS_UNICODE ) {
		input_str_len = UBYTES(input_str_chars);
		result_len = UBYTES(result_chars);
		if ( result_chars < 1 || result_chars > (2147483647/UBYTES(1)) ) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "You may not create strings longer then %ld characters", 2147483647/UBYTES(1));
			RETURN_FALSE;
		}
	} else {
		input_str_len = input_str_chars;
		result_len = result_chars;
		if ( result_chars < 1 || result_chars > 2147483647 ) {
			if ( input_str_type == IS_STRING ) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "You may not create strings longer then 2147483647 characters");
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "You may not create strings longer then 2147483647 bytes");
			}
			RETURN_FALSE;
		}
	}
	result = emalloc(result_len);
	
	/* Heavy optimization for situations where input string is 1 byte long */
	if ( input_str_len == 1 ) {
		memset(result, *((char *)input_str), mult);
	} else {
		char *s, *e, *ee;
		int l=0;
		memcpy(result, input_str, input_str_len);
		s = result;
		e = (char *) result + input_str_len;
		ee = (char *) result + result_len;

		while ( e < ee ) {
			l = (e-s) < (ee-e) ? (e-s) : (ee-e);
			memmove(e, s, l);
			e += l;
		}
	}
	
	if ( input_str_type == IS_UNICODE ) {
		*(((UChar *)result)+result_chars-1) = 0;
		RETURN_UNICODEL((UChar *)result, result_chars-1, 0);
	} else {
		*(((char *)result)+result_chars-1) = '\0';
		if ( input_str_type == IS_BINARY ) {
			RETURN_BINARYL((char *)result, result_chars-1, 0);
		} else {
			RETURN_STRINGL((char *)result, result_chars-1, 0);
		}
	}
}
/* }}} */

/* {{{ proto mixed count_chars(string input [, int mode])
   Returns info about what characters are used in input */
PHP_FUNCTION(count_chars)
{
	zval **input, **mode;
	int chars[256];
	int ac=ZEND_NUM_ARGS();
	int mymode=0;
	unsigned char *buf;
	int len, inx;
	char retstr[256];
	int retlen=0;

	if (ac < 1 || ac > 2 || zend_get_parameters_ex(ac, &input, &mode) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(input);

	if (ac == 2) {
		convert_to_long_ex(mode);
		mymode = Z_LVAL_PP(mode);
		
		if (mymode < 0 || mymode > 4) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown mode.");
			RETURN_FALSE;
		}
	}
	
	len = Z_STRLEN_PP(input);
	buf = (unsigned char *) Z_STRVAL_PP(input);
	memset((void*) chars, 0, sizeof(chars));

	while (len > 0) {
		chars[*buf]++;
		buf++;
		len--;
	}

	if (mymode < 3) {
		array_init(return_value);
	}

	for (inx = 0; inx < 256; inx++) {
		switch (mymode) {
	 		case 0:
				add_index_long(return_value, inx, chars[inx]);
				break;
	 		case 1:
				if (chars[inx] != 0) {
					add_index_long(return_value, inx, chars[inx]);
				}
				break;
  			case 2:
				if (chars[inx] == 0) {
					add_index_long(return_value, inx, chars[inx]);
				}
				break;
	  		case 3:
				if (chars[inx] != 0) {
					retstr[retlen++] = inx;
				}
				break;
  			case 4:
				if (chars[inx] == 0) {
					retstr[retlen++] = inx;
				}
				break;
		}
	}
	
	if (mymode >= 3 && mymode <= 4) {
		RETURN_STRINGL(retstr, retlen, 1);
	}
}
/* }}} */

/* {{{ php_strnatcmp
 */
static void php_strnatcmp(INTERNAL_FUNCTION_PARAMETERS, int fold_case)
{
	zval **s1, **s2;

	if (ZEND_NUM_ARGS()!=2 || zend_get_parameters_ex(2, &s1, &s2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(s1);
	convert_to_string_ex(s2);

	RETURN_LONG(strnatcmp_ex(Z_STRVAL_PP(s1), Z_STRLEN_PP(s1),
							 Z_STRVAL_PP(s2), Z_STRLEN_PP(s2),
							 fold_case));
}
/* }}} */

/* {{{ proto int strnatcmp(string s1, string s2)
   Returns the result of string comparison using 'natural' algorithm */
PHP_FUNCTION(strnatcmp)
{
	php_strnatcmp(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ proto array localeconv(void)
   Returns numeric formatting information based on the current locale */
PHP_FUNCTION(localeconv)
{
	zval *grouping, *mon_grouping;
	int len, i;

	/* We don't need no stinkin' parameters... */
	if (ZEND_NUM_ARGS() > 0) {
		WRONG_PARAM_COUNT;
	}

	MAKE_STD_ZVAL(grouping);
	MAKE_STD_ZVAL(mon_grouping);

	array_init(return_value);
	array_init(grouping);
	array_init(mon_grouping);

#ifdef HAVE_LOCALECONV
	{
		struct lconv currlocdata;

		localeconv_r( &currlocdata );
   
		/* Grab the grouping data out of the array */
		len = strlen(currlocdata.grouping);

		for (i = 0; i < len; i++) {
			add_index_long(grouping, i, currlocdata.grouping[i]);
		}

		/* Grab the monetary grouping data out of the array */
		len = strlen(currlocdata.mon_grouping);

		for (i = 0; i < len; i++) {
			add_index_long(mon_grouping, i, currlocdata.mon_grouping[i]);
		}

		add_assoc_string(return_value, "decimal_point",     currlocdata.decimal_point,     1);
		add_assoc_string(return_value, "thousands_sep",     currlocdata.thousands_sep,     1);
		add_assoc_string(return_value, "int_curr_symbol",   currlocdata.int_curr_symbol,   1);
		add_assoc_string(return_value, "currency_symbol",   currlocdata.currency_symbol,   1);
		add_assoc_string(return_value, "mon_decimal_point", currlocdata.mon_decimal_point, 1);
		add_assoc_string(return_value, "mon_thousands_sep", currlocdata.mon_thousands_sep, 1);
		add_assoc_string(return_value, "positive_sign",     currlocdata.positive_sign,     1);
		add_assoc_string(return_value, "negative_sign",     currlocdata.negative_sign,     1);
		add_assoc_long(  return_value, "int_frac_digits",   currlocdata.int_frac_digits     );
		add_assoc_long(  return_value, "frac_digits",       currlocdata.frac_digits         );
		add_assoc_long(  return_value, "p_cs_precedes",     currlocdata.p_cs_precedes       );
		add_assoc_long(  return_value, "p_sep_by_space",    currlocdata.p_sep_by_space      );
		add_assoc_long(  return_value, "n_cs_precedes",     currlocdata.n_cs_precedes       );
		add_assoc_long(  return_value, "n_sep_by_space",    currlocdata.n_sep_by_space      );
		add_assoc_long(  return_value, "p_sign_posn",       currlocdata.p_sign_posn         );
		add_assoc_long(  return_value, "n_sign_posn",       currlocdata.n_sign_posn         );
	}
#else
	/* Ok, it doesn't look like we have locale info floating around, so I guess it
	   wouldn't hurt to just go ahead and return the POSIX locale information?  */

	add_index_long(grouping, 0, -1);
	add_index_long(mon_grouping, 0, -1);

	add_assoc_string(return_value, "decimal_point",     "\x2E", 1);
	add_assoc_string(return_value, "thousands_sep",     "",     1);
	add_assoc_string(return_value, "int_curr_symbol",   "",     1);
	add_assoc_string(return_value, "currency_symbol",   "",     1);
	add_assoc_string(return_value, "mon_decimal_point", "\x2E", 1);
	add_assoc_string(return_value, "mon_thousands_sep", "",     1);
	add_assoc_string(return_value, "positive_sign",     "",     1);
	add_assoc_string(return_value, "negative_sign",     "",     1);
	add_assoc_long(  return_value, "int_frac_digits",   CHAR_MAX );
	add_assoc_long(  return_value, "frac_digits",       CHAR_MAX );
	add_assoc_long(  return_value, "p_cs_precedes",     CHAR_MAX );
	add_assoc_long(  return_value, "p_sep_by_space",    CHAR_MAX );
	add_assoc_long(  return_value, "n_cs_precedes",     CHAR_MAX );
	add_assoc_long(  return_value, "n_sep_by_space",    CHAR_MAX );
	add_assoc_long(  return_value, "p_sign_posn",       CHAR_MAX );
	add_assoc_long(  return_value, "n_sign_posn",       CHAR_MAX );
#endif

	zend_hash_update(Z_ARRVAL_P(return_value), "grouping", 9, &grouping, sizeof(zval *), NULL);
	zend_hash_update(Z_ARRVAL_P(return_value), "mon_grouping", 13, &mon_grouping, sizeof(zval *), NULL);
}
/* }}} */

/* {{{ proto int strnatcasecmp(string s1, string s2)
   Returns the result of case-insensitive string comparison using 'natural' algorithm */
PHP_FUNCTION(strnatcasecmp)
{
	php_strnatcmp(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto int substr_count(string haystack, string needle [, int offset [, int length]])
   Returns the number of times a substring occurs in the string */
PHP_FUNCTION(substr_count)
{
	zval **haystack, **needle, **offset, **length;
	int ac = ZEND_NUM_ARGS();
	int count = 0;
	char *p, *endp, cmp;

	if (ac < 2 || ac > 4 || zend_get_parameters_ex(ac, &haystack, &needle, &offset, &length) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(haystack);
	convert_to_string_ex(needle);

	if (Z_STRLEN_PP(needle) == 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Empty substring.");
		RETURN_FALSE;
	}
	
	p = Z_STRVAL_PP(haystack);
	endp = p + Z_STRLEN_PP(haystack);
	
	if (ac > 2) {
		convert_to_long_ex(offset);
		if (Z_LVAL_PP(offset) < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Offset should be greater then or equal to 0.");
			RETURN_FALSE;		
		}
		p += Z_LVAL_PP(offset);
		if (p > endp) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Offset value %ld exceeds string length.", Z_LVAL_PP(offset));
			RETURN_FALSE;		
		}
		if (ac == 4) {
			convert_to_long_ex(length);
			if (Z_LVAL_PP(length) <= 0) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length should be greater than 0.");
				RETURN_FALSE;		
			}
			if ((p + Z_LVAL_PP(length)) > endp) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length value %ld exceeds string length.", Z_LVAL_PP(length));
				RETURN_FALSE;
			}
			endp = p + Z_LVAL_PP(length);
		}
	}
	
	if (Z_STRLEN_PP(needle) == 1) {
		cmp = Z_STRVAL_PP(needle)[0];

		while ((p = memchr(p, cmp, endp - p))) {
			count++;
			p++;
		}
	} else {
		while ((p = php_memnstr(p, Z_STRVAL_PP(needle), Z_STRLEN_PP(needle), endp))) {
			p += Z_STRLEN_PP(needle);
			count++;
		}
	}

	RETURN_LONG(count);
}
/* }}} */	

/* {{{ proto string str_pad(string input, int pad_length [, string pad_string [, int pad_type]])
   Returns input string padded on the left or right to specified length with pad_string */
PHP_FUNCTION(str_pad)
{
	/* Input arguments */
	zval **input,				/* Input string */
		 **pad_length,			/* Length to pad to */
		 **pad_string,			/* Padding string */
		 **pad_type;			/* Padding type (left/right/both) */
	
	/* Helper variables */
	int	   num_pad_chars;		/* Number of padding characters (total - input size) */
	char  *result = NULL;		/* Resulting string */
	int	   result_len = 0;		/* Length of the resulting string */
	char  *pad_str_val = " ";	/* Pointer to padding string */
	int    pad_str_len = 1;		/* Length of the padding string */
	int	   pad_type_val = STR_PAD_RIGHT; /* The padding type value */
	int	   i, left_pad=0, right_pad=0;


	if (ZEND_NUM_ARGS() < 2 || ZEND_NUM_ARGS() > 4 ||
		zend_get_parameters_ex(ZEND_NUM_ARGS(), &input, &pad_length, &pad_string, &pad_type) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	/* Perform initial conversion to expected data types. */
	convert_to_string_ex(input);
	convert_to_long_ex(pad_length);

	num_pad_chars = Z_LVAL_PP(pad_length) - Z_STRLEN_PP(input);

	/* If resulting string turns out to be shorter than input string,
	   we simply copy the input and return. */
	if (num_pad_chars < 0) {
		RETURN_ZVAL(*input, 1, 0);
	}

	/* Setup the padding string values if specified. */
	if (ZEND_NUM_ARGS() > 2) {
		convert_to_string_ex(pad_string);
		if (Z_STRLEN_PP(pad_string) == 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Padding string cannot be empty.");
			return;
		}
		pad_str_val = Z_STRVAL_PP(pad_string);
		pad_str_len = Z_STRLEN_PP(pad_string);

		if (ZEND_NUM_ARGS() > 3) {
			convert_to_long_ex(pad_type);
			pad_type_val = Z_LVAL_PP(pad_type);
			if (pad_type_val < STR_PAD_LEFT || pad_type_val > STR_PAD_BOTH) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Padding type has to be STR_PAD_LEFT, STR_PAD_RIGHT, or STR_PAD_BOTH.");
				return;
			}
		}
	}

	result = (char *)emalloc(Z_STRLEN_PP(input) + num_pad_chars + 1);

	/* We need to figure out the left/right padding lengths. */
	switch (pad_type_val) {
		case STR_PAD_RIGHT:
			left_pad = 0;
			right_pad = num_pad_chars;
			break;

		case STR_PAD_LEFT:
			left_pad = num_pad_chars;
			right_pad = 0;
			break;

		case STR_PAD_BOTH:
			left_pad = num_pad_chars / 2;
			right_pad = num_pad_chars - left_pad;
			break;
	}

	/* First we pad on the left. */
	for (i = 0; i < left_pad; i++)
		result[result_len++] = pad_str_val[i % pad_str_len];

	/* Then we copy the input string. */
	memcpy(result + result_len, Z_STRVAL_PP(input), Z_STRLEN_PP(input));
	result_len += Z_STRLEN_PP(input);

	/* Finally, we pad on the right. */
	for (i = 0; i < right_pad; i++)
		result[result_len++] = pad_str_val[i % pad_str_len];

	result[result_len] = '\0';

	RETURN_STRINGL(result, result_len, 0);
}
/* }}} */
   
/* {{{ proto mixed sscanf(string str, string format [, string ...])
   Implements an ANSI C compatible sscanf */
PHP_FUNCTION(sscanf)
{
	zval ***args;
	int     result;
	int	    argc = ZEND_NUM_ARGS();	

	if (argc < 2) {
		WRONG_PARAM_COUNT;
	}

	args = (zval ***) safe_emalloc(argc, sizeof(zval **), 0);
	if (zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(args[0]);
	convert_to_string_ex(args[1]);
	
	result = php_sscanf_internal(Z_STRVAL_PP(args[0]),
	                             Z_STRVAL_PP(args[1]),
	                             argc, args,
	                             2, &return_value TSRMLS_CC);
	efree(args);

	if (SCAN_ERROR_WRONG_PARAM_COUNT == result) {
		WRONG_PARAM_COUNT;
	}
}
/* }}} */

static char rot13_from[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char rot13_to[] = "nopqrstuvwxyzabcdefghijklmNOPQRSTUVWXYZABCDEFGHIJKLM";

/* {{{ proto string str_rot13(string str)
   Perform the rot13 transform on a string */
PHP_FUNCTION(str_rot13)
{
	zval **arg;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg)) {
		WRONG_PARAM_COUNT;
	}
	RETVAL_ZVAL(*arg, 1, 0);

	php_strtr(Z_STRVAL_P(return_value), Z_STRLEN_P(return_value), rot13_from, rot13_to, 52);
}
/* }}} */


static void php_string_shuffle(char *str, long len TSRMLS_DC)
{
	long n_elems, rnd_idx, n_left;
	char temp;
	/* The implementation is stolen from array_data_shuffle       */
	/* Thus the characteristics of the randomization are the same */
	n_elems = len;
	
	if (n_elems <= 1) {
		return;
	}

	n_left = n_elems;
	
	while (--n_left) {
		rnd_idx = php_rand(TSRMLS_C);
		RAND_RANGE(rnd_idx, 0, n_left, PHP_RAND_MAX);
		if (rnd_idx != n_left) {
			temp = str[n_left];
			str[n_left] = str[rnd_idx];
			str[rnd_idx] = temp;
		}
	}
}


/* {{{ proto void str_shuffle(string str)
   Shuffles string. One permutation of all possible is created */
PHP_FUNCTION(str_shuffle)
{
	zval **arg;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg)) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(arg);
	RETVAL_ZVAL(*arg, 1, 0);
	if (Z_STRLEN_P(return_value) > 1) { 
		php_string_shuffle(Z_STRVAL_P(return_value), (long) Z_STRLEN_P(return_value) TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto mixed str_word_count(string str, [int format [, string charlist]])
   	Counts the number of words inside a string. If format of 1 is specified,
   	then the function will return an array containing all the words
   	found inside the string. If format of 2 is specified, then the function
   	will return an associated array where the position of the word is the key
   	and the word itself is the value.
   	
   	For the purpose of this function, 'word' is defined as a locale dependent
   	string containing alphabetic characters, which also may contain, but not start
   	with "'" and "-" characters.
*/
PHP_FUNCTION(str_word_count)
{
	char *buf, *str, *char_list = NULL, *p, *e, *s, ch[256];
	int str_len, char_list_len, word_count = 0;
	long type = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ls", &str, &str_len, &type, &char_list, &char_list_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (char_list) {
		php_charmask(char_list, char_list_len, ch TSRMLS_CC);
	}
	
	p = str;
	e = str + str_len;
		
	if (type == 1 || type == 2) {
		array_init(return_value);
	}
	
	while (p < e) {
		if (isalpha(*p) || (char_list && ch[(unsigned char)*p])) {
			s = ++p - 1;
			while (isalpha(*p) || *p == '\'' || (*p == '-' && isalpha(*(p+1))) || (char_list && ch[(unsigned char)*p])) {
				p++;
			}
			
			switch (type)
			{
				case 1:
					buf = estrndup(s, (p-s));
					add_next_index_stringl(return_value, buf, (p-s), 1);
					efree(buf);
					break;
				case 2:
					buf = estrndup(s, (p-s));
					add_index_stringl(return_value, (s - str), buf, p-s, 1);
					efree(buf);
					break;
				default:
					word_count++;
					break;		
			}
		} else {
			p++;
		}
	}
	
	if (!type) {
		RETURN_LONG(word_count);		
	}
}

/* }}} */

#if HAVE_STRFMON
/* {{{ proto string money_format(string format , float value)
   Convert monetary value(s) to string */
PHP_FUNCTION(money_format)
{
	int format_len = 0, str_len;
	char *format, *str;
	double value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd", &format, &format_len, &value) == FAILURE) {
		return;
	}

	str_len = format_len + 1024;
	str = emalloc(str_len);
	if ((str_len = strfmon(str, str_len, format, value)) < 0) {
		efree(str);
		RETURN_FALSE;
	}
	str[str_len] = 0;

	RETURN_STRINGL(erealloc(str, str_len + 1), str_len, 0);
}
/* }}} */
#endif

/* {{{ proto array str_split(string str [, int split_length])
   Convert a string to an array. If split_length is specified, break the string down into chunks each split_length characters long. */
PHP_FUNCTION(str_split)
{
	char *str;
	int str_len;
	long split_length = 1;
	char *p;
	int n_reg_segments;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &str, &str_len, &split_length) == FAILURE) {
		return;
	}

	if (split_length <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The length of each segment must be greater than zero.");
		RETURN_FALSE;
	}

	array_init(return_value);

	if (split_length >= str_len) {
		add_next_index_stringl(return_value, str, str_len, 1);
		return;
	}

	n_reg_segments = floor(str_len / split_length);
	p = str;

	while (n_reg_segments-- > 0) {
		add_next_index_stringl(return_value, p, split_length, 1);
		p += split_length;
	}

	if (p != (str + str_len)) {
		add_next_index_stringl(return_value, p, (str + str_len - p), 1);
	}
}
/* }}} */

/* {{{ proto array strpbrk(string haystack, string char_list)
   Search a string for any of a set of characters */
PHP_FUNCTION(strpbrk)
{
	char *haystack, *char_list;
	int haystack_len, char_list_len;
	char *p;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &haystack, &haystack_len, &char_list, &char_list_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (!char_list_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The character list cannot be empty.");
		RETURN_FALSE;	
	}

	if ((p = strpbrk(haystack, char_list))) {
		RETURN_STRINGL(p, (haystack + haystack_len - p), 1);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int substr_compare(string main_str, string str, int offset [, int length [, bool case_sensitivity]])
   Binary safe optionally case insensitive comparison of 2 strings from an offset, up to length characters */
PHP_FUNCTION(substr_compare)
{
	char *s1, *s2;
	int s1_len, s2_len;
	long offset, len=0;
	zend_bool cs=0;
	uint cmp_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl|lb", &s1, &s1_len, &s2, &s2_len, &offset, &len, &cs) == FAILURE) {
		RETURN_FALSE;
	}

	if (len && offset >= s1_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "The start position cannot exceed initial string length.");
		RETURN_FALSE;
	}

	cmp_len = (uint) (len ? len : MAX(s2_len, (s1_len - offset)));

	if (!cs) {
		RETURN_LONG(zend_binary_strncmp(s1 + offset, (s1_len - offset), s2, s2_len, cmp_len));
	} else {
		RETURN_LONG(zend_binary_strncasecmp(s1 + offset, (s1_len - offset), s2, s2_len, cmp_len));
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
