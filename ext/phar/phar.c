/*
  +----------------------------------------------------------------------+
  | phar php single-file executable PHP extension                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2005-2008 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Gregory Beaver <cellog@php.net>                             |
  |          Marcus Boerger <helly@php.net>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#define PHAR_MAIN 1
#include "phar_internal.h"
#include "SAPI.h"
#include "func_interceptors.h"

ZEND_DECLARE_MODULE_GLOBALS(phar)
int phar_has_bz2;
int phar_has_zlib;
#if PHP_VERSION_ID >= 50300
char *(*phar_save_resolve_path)(const char *filename, int filename_len TSRMLS_DC);
#endif

/**
 * set's phar->is_writeable based on the current INI value
 */
static int phar_set_writeable_bit(void *pDest, void *argument TSRMLS_DC) /* {{{ */
{
	zend_bool keep = *(zend_bool *)argument;
	phar_archive_data *phar = *(phar_archive_data **)pDest;

	if (!phar->is_data) {
		phar->is_writeable = !keep;
	}
	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/* if the original value is 0 (disabled), then allow setting/unsetting at will
   otherwise, only allow 1 (enabled), and error on disabling */
ZEND_INI_MH(phar_ini_modify_handler) /* {{{ */
{
	zend_bool old, ini;

	if (entry->name_length == 14) {
		old = PHAR_G(readonly_orig);
	} else {
		old = PHAR_G(require_hash_orig);
	} 

	if (new_value_length == 2 && !strcasecmp("on", new_value)) {
		ini = (zend_bool) 1;
	}
	else if (new_value_length == 3 && !strcasecmp("yes", new_value)) {
		ini = (zend_bool) 1;
	}
	else if (new_value_length == 4 && !strcasecmp("true", new_value)) {
		ini = (zend_bool) 1;
	}
	else {
		ini = (zend_bool) atoi(new_value);
	}

	/* do not allow unsetting in runtime */
	if (stage == ZEND_INI_STAGE_STARTUP) {
		if (entry->name_length == 14) {
			PHAR_G(readonly_orig) = ini; 
		} else {
			PHAR_G(require_hash_orig) = ini;
		} 
	} else if (old && !ini) {
		return FAILURE;
	}

	if (entry->name_length == 14) {
		PHAR_G(readonly) = ini;
		if (PHAR_GLOBALS->request_init && PHAR_GLOBALS->phar_fname_map.arBuckets) {
			zend_hash_apply_with_argument(&(PHAR_GLOBALS->phar_fname_map), phar_set_writeable_bit, (void *)&ini TSRMLS_CC);
		}
	} else {
		PHAR_G(require_hash) = ini;
	} 
	return SUCCESS;
}
/* }}}*/

static void phar_split_extract_list(TSRMLS_D)
{
	char *tmp = estrdup(PHAR_GLOBALS->extract_list);
	char *key;
	char *lasts;
	char *q;
	int keylen;

	zend_hash_clean(&(PHAR_GLOBALS->phar_plain_map));

	for (key = php_strtok_r(tmp, ",", &lasts);
			key;
			key = php_strtok_r(NULL, ",", &lasts))
	{
		char *val = strchr(key, '=');

		if (val) {	
			*val++ = '\0';
			for (q = key; *q; q++) {
				*q = tolower(*q);
			}
			keylen = q - key + 1;
			zend_hash_add(&(PHAR_GLOBALS->phar_plain_map), key, keylen, val, strlen(val)+1, NULL);
		}
	}
	efree(tmp);
}
/* }}} */

ZEND_INI_MH(phar_ini_extract_list) /* {{{ */
{
	PHAR_G(extract_list) = new_value;

	if (stage == ZEND_INI_STAGE_RUNTIME) {
		phar_request_initialize(TSRMLS_C);
		phar_split_extract_list(TSRMLS_C);
	}

	return SUCCESS;
}
/* }}} */

ZEND_INI_DISP(phar_ini_extract_list_disp) /*void name(zend_ini_entry *ini_entry, int type) {{{ */
{
	char *value;

	if (type==ZEND_INI_DISPLAY_ORIG && ini_entry->modified) {
		value = ini_entry->orig_value;
	} else if (ini_entry->value) {
		value = ini_entry->value;
	} else {
		value = NULL;
	}

	if (value) {
		char *tmp = strdup(value);
		char *key;
		char *lasts;
		char *q;
		int started = 0;
	
		if (!sapi_module.phpinfo_as_text) {
			php_printf("<ul>");
		}
		for (key = php_strtok_r(tmp, ",", &lasts);
				key;
				key = php_strtok_r(NULL, ",", &lasts))
		{
			char *val = strchr(key, '=');

			if (val) {	
				*val++ = '\0';
				for (q = key; *q; ++q) {
					*q = tolower(*q);
				}
				if (sapi_module.phpinfo_as_text) {
					if (started++) {
						php_printf(",");
					}
					php_printf("[%s = %s]", key, val);
				} else {
					php_printf("<li>%s => %s</li>", key, val);
				}
			}
		}
		if (!sapi_module.phpinfo_as_text) {
			php_printf("</ul>");
		}
		free(tmp);
	}
}
/* }}} */

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN( "phar.readonly",     "1", PHP_INI_ALL, phar_ini_modify_handler, readonly,     zend_phar_globals, phar_globals)
	STD_PHP_INI_BOOLEAN( "phar.require_hash", "1", PHP_INI_ALL, phar_ini_modify_handler, require_hash, zend_phar_globals, phar_globals)
	STD_PHP_INI_ENTRY_EX("phar.extract_list", "",  PHP_INI_ALL, phar_ini_extract_list,   extract_list, zend_phar_globals, phar_globals, phar_ini_extract_list_disp)
PHP_INI_END()

/**
 * When all uses of a phar have been concluded, this frees the manifest
 * and the phar slot
 */
static void phar_destroy_phar_data(phar_archive_data *phar TSRMLS_DC) /* {{{ */
{
	if (phar->alias && phar->alias != phar->fname) {
		efree(phar->alias);
		phar->alias = NULL;
	}
	if (phar->fname) {
		efree(phar->fname);
		phar->fname = NULL;
	}
	if (phar->signature) {
		efree(phar->signature);
	}
	if (phar->manifest.arBuckets) {
		zend_hash_destroy(&phar->manifest);
		phar->manifest.arBuckets = NULL;
	}
	if (phar->mounted_dirs.arBuckets) {
		zend_hash_destroy(&phar->mounted_dirs);
		phar->mounted_dirs.arBuckets = NULL;
	}
	if (phar->metadata) {
		zval_ptr_dtor(&phar->metadata);
		phar->metadata = 0;
	}
	if (phar->fp) {
		php_stream_close(phar->fp);
		phar->fp = 0;
	}
	if (phar->ufp) {
		php_stream_close(phar->ufp);
		phar->fp = 0;
	}
	efree(phar);
}
/* }}}*/

/**
 * Delete refcount and destruct if needed. On destruct return 1 else 0.
 */
int phar_archive_delref(phar_archive_data *phar TSRMLS_DC) /* {{{ */
{
	if (--phar->refcount < 0) {
		if (PHAR_GLOBALS->request_done
		|| zend_hash_del(&(PHAR_GLOBALS->phar_fname_map), phar->fname, phar->fname_len) != SUCCESS) {
			phar_destroy_phar_data(phar TSRMLS_CC);
		}
		return 1;
	} else if (!phar->refcount) {
		if (phar->fp && !(phar->flags & PHAR_FILE_COMPRESSION_MASK)) {
			/* close open file handle - allows removal or rename of
			the file on windows, which has greedy locking
			only close if the archive was not already compressed.  If it
			was compressed, then the fp does not refer to the original file */
			php_stream_close(phar->fp);
			phar->fp = NULL;
		}
	}
	return 0;
}
/* }}}*/

/**
 * Destroy phar's in shutdown, here we don't care about aliases
 */
static void destroy_phar_data_only(void *pDest) /* {{{ */
{
	phar_archive_data *phar_data = *(phar_archive_data **) pDest;
	TSRMLS_FETCH();

	if (EG(exception) || --phar_data->refcount < 0) {
		phar_destroy_phar_data(phar_data TSRMLS_CC);
	}
}
/* }}}*/

/**
 * Delete aliases to phar's that got kicked out of the global table
 */
static int phar_unalias_apply(void *pDest, void *argument TSRMLS_DC) /* {{{ */
{
	return *(void**)pDest == argument ? ZEND_HASH_APPLY_REMOVE : ZEND_HASH_APPLY_KEEP;
}
/* }}} */

/**
 * Filename map destructor
 */
static void destroy_phar_data(void *pDest) /* {{{ */
{
	phar_archive_data *phar_data = *(phar_archive_data **) pDest;
	TSRMLS_FETCH();

	if (PHAR_GLOBALS->request_ends) {
		destroy_phar_data_only(pDest);
		return;
	}
	zend_hash_apply_with_argument(&(PHAR_GLOBALS->phar_alias_map), phar_unalias_apply, phar_data TSRMLS_CC);
	if (--phar_data->refcount < 0) {
		phar_destroy_phar_data(phar_data TSRMLS_CC);
	}
}
/* }}}*/

/**
 * destructor for the manifest hash, frees each file's entry
 */
void destroy_phar_manifest_entry(void *pDest) /* {{{ */
{
	phar_entry_info *entry = (phar_entry_info *)pDest;
	TSRMLS_FETCH();

	if (entry->cfp) {
		php_stream_close(entry->cfp);
		entry->cfp = 0;
	}
	if (entry->fp) {
		php_stream_close(entry->fp);
		entry->fp = 0;
	}
	if (entry->metadata) {
		zval_ptr_dtor(&entry->metadata);
		entry->metadata = 0;
	}
	if (entry->metadata_str.c) {
		smart_str_free(&entry->metadata_str);
		entry->metadata_str.c = 0;
	}
	efree(entry->filename);
	if (entry->link) {
		efree(entry->link);
		entry->link = 0;
	}
}
/* }}} */

int phar_entry_delref(phar_entry_data *idata TSRMLS_DC) /* {{{ */
{
	int ret = 0;

	if (idata->internal_file) {
		if (--idata->internal_file->fp_refcount < 0) {
			idata->internal_file->fp_refcount = 0;
		}
		if (idata->fp && idata->fp != idata->phar->fp && idata->fp != idata->phar->ufp && idata->fp != idata->internal_file->fp) {
			php_stream_close(idata->fp);
		}
	}
	phar_archive_delref(idata->phar TSRMLS_CC);
	efree(idata);
	return ret;
}
/* }}} */

/**
 * Removes an entry, either by actually removing it or by marking it.
 */
void phar_entry_remove(phar_entry_data *idata, char **error TSRMLS_DC) /* {{{ */
{
	phar_archive_data *phar;

	phar = idata->phar;
	if (idata->internal_file->fp_refcount < 2) {
		if (idata->fp && idata->fp != idata->phar->fp && idata->fp != idata->phar->ufp && idata->fp != idata->internal_file->fp) {
			php_stream_close(idata->fp);
		}
		zend_hash_del(&idata->phar->manifest, idata->internal_file->filename, idata->internal_file->filename_len);
		idata->phar->refcount--;
		efree(idata);
	} else {
		idata->internal_file->is_deleted = 1;
		phar_entry_delref(idata TSRMLS_CC);
	}
	if (!phar->donotflush) {
		phar_flush(phar, 0, 0, 0, error TSRMLS_CC);
	}
}
/* }}} */

#define MAPPHAR_ALLOC_FAIL(msg) \
	php_stream_close(fp);\
	if (error) {\
		spprintf(error, 0, msg, fname);\
	}\
	return FAILURE;

#define MAPPHAR_FAIL(msg) \
	efree(savebuf);\
	if (mydata) {\
		phar_destroy_phar_data(mydata TSRMLS_CC);\
	}\
	if (signature) {\
		efree(signature);\
	}\
	MAPPHAR_ALLOC_FAIL(msg)

#ifdef WORDS_BIGENDIAN
# define PHAR_GET_32(buffer, var) \
	var = ((((unsigned char*)(buffer))[3]) << 24) \
		| ((((unsigned char*)(buffer))[2]) << 16) \
		| ((((unsigned char*)(buffer))[1]) <<  8) \
		| (((unsigned char*)(buffer))[0]); \
	(buffer) += 4
# define PHAR_GET_16(buffer, var) \
	var = ((((unsigned char*)(buffer))[1]) <<  8) \
		| (((unsigned char*)(buffer))[0]); \
	(buffer) += 2
# define PHAR_ZIP_32(buffer) ((((unsigned char*)(buffer))[3]) << 24) \
		| ((((unsigned char*)(buffer))[2]) << 16) \
		| ((((unsigned char*)(buffer))[1]) <<  8) \
		| (((unsigned char*)(buffer))[0])
# define PHAR_ZIP_16(buffer) ((((unsigned char*)(buffer))[1]) <<  8) \
		| (((unsigned char*)(buffer))[0])
#else
# define PHAR_GET_32(buffer, var) \
	var = *(php_uint32*)(buffer); \
	buffer += 4
# define PHAR_GET_16(buffer, var) \
	var = *(php_uint16*)(buffer); \
	buffer += 2
# define PHAR_ZIP_32(buffer) buffer
# define PHAR_ZIP_16(buffer) buffer
#endif

/**
 * Open an already loaded phar
 */
int phar_open_loaded(char *fname, int fname_len, char *alias, int alias_len, int is_data, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	phar_archive_data *phar;
#ifdef PHP_WIN32
	char *unixfname;
#endif

	if (error) {
		*error = NULL;
	}
#ifdef PHP_WIN32
	unixfname = estrndup(fname, fname_len);
	phar_unixify_path_separators(unixfname, fname_len);
	if (SUCCESS == phar_get_archive(&phar, unixfname, fname_len, alias, alias_len, error TSRMLS_CC)
		&& ((alias && fname_len == phar->fname_len
		&& !strncmp(unixfname, phar->fname, fname_len)) || !alias)
	) {
		phar_entry_info *stub;
		efree(unixfname);
#else
	if (SUCCESS == phar_get_archive(&phar, fname, fname_len, alias, alias_len, error TSRMLS_CC)
		&& ((alias && fname_len == phar->fname_len
		&& !strncmp(fname, phar->fname, fname_len)) || !alias)
	) {
		phar_entry_info *stub;
#endif
		/* logic above is as follows:
		   If an explicit alias was requested, ensure the filename passed in
		   matches the phar's filename.
		   If no alias was passed in, then it can match either and be valid
		 */

		if (!is_data) {
			/* prevent any ".phar" without a stub getting through */
			if (!phar->halt_offset && !phar->is_brandnew) {
				if (PHAR_G(readonly) && FAILURE == zend_hash_find(&(phar->manifest), ".phar/stub.php", sizeof(".phar/stub.php")-1, (void **)&stub)) {
					spprintf(error, 0, "'%s' is not a phar archive. Use PharData::__construct() for a standard zip or tar archive", fname);
					return FAILURE;
				}
			}
		}
		phar->is_data = is_data;
		if (pphar) {
			*pphar = phar;
		}
		return SUCCESS;
	} else {
#ifdef PHP_WIN32
		efree(unixfname);
#endif
		if (pphar) {
			*pphar = NULL;
		}
		if (phar && alias && (options & REPORT_ERRORS)) {
			if (error) {
				spprintf(error, 0, "alias \"%s\" is already used for archive \"%s\" cannot be overloaded with \"%s\"", alias, phar->fname, fname);
			}
		}
		return FAILURE;
	}
}
/* }}}*/

/**
 * Parse out metadata from the manifest for a single file
 *
 * Meta-data is in this format:
 * [len32][data...]
 * 
 * data is the serialized zval
 */
int phar_parse_metadata(char **buffer, zval **metadata, int zip_metadata_len TSRMLS_DC) /* {{{ */
{
	const unsigned char *p;
	php_uint32 buf_len;
	php_unserialize_data_t var_hash;

	if (!zip_metadata_len) {
		PHAR_GET_32(*buffer, buf_len);
	} else {
		buf_len = zip_metadata_len;
	}
	
	if (buf_len) {
		ALLOC_INIT_ZVAL(*metadata);
		p = (const unsigned char*) *buffer;
		PHP_VAR_UNSERIALIZE_INIT(var_hash);
		if (!php_var_unserialize(metadata, &p, p + buf_len,  &var_hash TSRMLS_CC)) {
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			zval_ptr_dtor(metadata);
			*metadata = NULL;
			return FAILURE;
		}
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
	} else {
		*metadata = NULL;
	}
	if (!zip_metadata_len) {
		*buffer += buf_len;
	}
	return SUCCESS;
}
/* }}}*/

static const char hexChars[] = "0123456789ABCDEF";

static int phar_hex_str(const char *digest, size_t digest_len, char ** signature)
{
	int pos = -1;
	size_t len;

	*signature = (char*)safe_emalloc(digest_len, 2, 1);

	for(len = 0; len < digest_len; ++len) {
		(*signature)[++pos] = hexChars[((const unsigned char *)digest)[len] >> 4];
		(*signature)[++pos] = hexChars[((const unsigned char *)digest)[len] & 0x0F];
	}
	(*signature)[++pos] = '\0';
	return pos;
}

/**
 * Does not check for a previously opened phar in the cache.
 *
 * Parse a new one and add it to the cache, returning either SUCCESS or 
 * FAILURE, and setting pphar to the pointer to the manifest entry
 * 
 * This is used by phar_open_filename to process the manifest, but can be called
 * directly.
 */
int phar_open_file(php_stream *fp, char *fname, int fname_len, char *alias, int alias_len, long halt_offset, phar_archive_data** pphar, php_uint32 compression, char **error TSRMLS_DC) /* {{{ */
{
	char b32[4], *buffer, *endbuffer, *savebuf;
	phar_archive_data *mydata = NULL;
	phar_entry_info entry;
	php_uint32 manifest_len, manifest_count, manifest_flags, manifest_index, tmp_len, sig_flags;
	php_uint16 manifest_ver;
	long offset;
	int register_alias, sig_len, temp_alias = 0;
	char *signature = NULL;

	if (pphar) {
		*pphar = NULL;
	}
	if (error) {
		*error = NULL;
	}

	/* check for ?>\n and increment accordingly */
	if (-1 == php_stream_seek(fp, halt_offset, SEEK_SET)) {
		MAPPHAR_ALLOC_FAIL("cannot seek to __HALT_COMPILER(); location in phar \"%s\"")
	}

	buffer = b32;
	if (3 != php_stream_read(fp, buffer, 3)) {
		MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (truncated manifest at stub end)")
	}
	if ((*buffer == ' ' || *buffer == '\n') && *(buffer + 1) == '?' && *(buffer + 2) == '>') {
		int nextchar;
		halt_offset += 3;
		if (EOF == (nextchar = php_stream_getc(fp))) {
			MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (truncated manifest at stub end)")
		}
		if ((char) nextchar == '\r') {
			/* if we have an \r we require an \n as well */
			if (EOF == (nextchar = php_stream_getc(fp)) || (char)nextchar != '\n') {
				MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (truncated manifest at stub end)")
			}
			++halt_offset;
		}
		if ((char) nextchar == '\n') {
			++halt_offset;
		}
	}
	/* make sure we are at the right location to read the manifest */
	if (-1 == php_stream_seek(fp, halt_offset, SEEK_SET)) {
		MAPPHAR_ALLOC_FAIL("cannot seek to __HALT_COMPILER(); location in phar \"%s\"")
	}

	/* read in manifest */
	buffer = b32;
	if (4 != php_stream_read(fp, buffer, 4)) {
		MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (truncated manifest at manifest length)")
	}
	PHAR_GET_32(buffer, manifest_len);
	if (manifest_len > 1048576 * 100) {
		/* prevent serious memory issues by limiting manifest to at most 100 MB in length */
		MAPPHAR_ALLOC_FAIL("manifest cannot be larger than 100 MB in phar \"%s\"")
	}
	buffer = (char *)emalloc(manifest_len);
	savebuf = buffer;
	endbuffer = buffer + manifest_len;
	if (manifest_len < 10 || manifest_len != php_stream_read(fp, buffer, manifest_len)) {
		MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest header)")
	}

	/* extract the number of entries */
	PHAR_GET_32(buffer, manifest_count);
	if (manifest_count == 0) {
		MAPPHAR_FAIL("in phar \"%s\", manifest claims to have zero entries.  Phars must have at least 1 entry");
	}

	/* extract API version, lowest nibble currently unused */
	manifest_ver = (((unsigned char)buffer[0]) <<  8)
	             + ((unsigned char)buffer[1]);
	buffer += 2;
	if ((manifest_ver & PHAR_API_VER_MASK) < PHAR_API_MIN_READ) {
		efree(savebuf);
		php_stream_close(fp);
		if (error) {
			spprintf(error, 0, "phar \"%s\" is API version %1.u.%1.u.%1.u, and cannot be processed", fname, manifest_ver >> 12, (manifest_ver >> 8) & 0xF, (manifest_ver >> 4) & 0x0F);
		}
		return FAILURE;
	}

	PHAR_GET_32(buffer, manifest_flags);

	manifest_flags &= ~PHAR_HDR_COMPRESSION_MASK;

	manifest_flags &= ~PHAR_FILE_COMPRESSION_MASK;
	/* remember whether this entire phar was compressed with gz/bzip2 */
	manifest_flags |= compression;

	/* The lowest nibble contains the phar wide flags. The compression flags can */
	/* be ignored on reading because it is being generated anyways. */
	if (manifest_flags & PHAR_HDR_SIGNATURE) {
		unsigned char buf[1024];
		int read_size, len;
		char sig_buf[8], *sig_ptr = sig_buf;
		off_t read_len;

		if (-1 == php_stream_seek(fp, -8, SEEK_END)
		|| (read_len = php_stream_tell(fp)) < 20
		|| 8 != php_stream_read(fp, sig_buf, 8) 
		|| memcmp(sig_buf+4, "GBMB", 4)) {
			efree(savebuf);
			php_stream_close(fp);
			if (error) {
				spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
			}
			return FAILURE;
		}
		PHAR_GET_32(sig_ptr, sig_flags);
		switch(sig_flags) {
#if HAVE_HASH_EXT
		case PHAR_SIG_SHA512: {
			unsigned char digest[64], saved[64];
			PHP_SHA512_CTX  context;

			php_stream_rewind(fp);
			PHP_SHA512Init(&context);
			read_len -= sizeof(digest);
			if (read_len > sizeof(buf)) {
				read_size = sizeof(buf);
			} else {
				read_size = (int)read_len;
			}
			while ((len = php_stream_read(fp, (char*)buf, read_size)) > 0) {
				PHP_SHA512Update(&context, buf, len);
				read_len -= (off_t)len;
				if (read_len < read_size) {
					read_size = (int)read_len;
				}
			}
			PHP_SHA512Final(digest, &context);

			if (read_len > 0
			|| php_stream_read(fp, (char*)saved, sizeof(saved)) != sizeof(saved)
			|| memcmp(digest, saved, sizeof(digest))) {
				efree(savebuf);
				php_stream_close(fp);
				if (error) {
					spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
				}
				return FAILURE;
			}

			sig_len = phar_hex_str((const char*)digest, sizeof(digest), &signature);
			break;
		}
		case PHAR_SIG_SHA256: {
			unsigned char digest[32], saved[32];
			PHP_SHA256_CTX  context;

			php_stream_rewind(fp);
			PHP_SHA256Init(&context);
			read_len -= sizeof(digest);
			if (read_len > sizeof(buf)) {
				read_size = sizeof(buf);
			} else {
				read_size = (int)read_len;
			}
			while ((len = php_stream_read(fp, (char*)buf, read_size)) > 0) {
				PHP_SHA256Update(&context, buf, len);
				read_len -= (off_t)len;
				if (read_len < read_size) {
					read_size = (int)read_len;
				}
			}
			PHP_SHA256Final(digest, &context);

			if (read_len > 0
			|| php_stream_read(fp, (char*)saved, sizeof(saved)) != sizeof(saved)
			|| memcmp(digest, saved, sizeof(digest))) {
				efree(savebuf);
				php_stream_close(fp);
				if (error) {
					spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
				}
				return FAILURE;
			}

			sig_len = phar_hex_str((const char*)digest, sizeof(digest), &signature);
			break;
		}
#else
		case PHAR_SIG_SHA512:
		case PHAR_SIG_SHA256:
			efree(savebuf);
			php_stream_close(fp);
			if (error) {
				spprintf(error, 0, "phar \"%s\" has a unsupported signature", fname);
			}
			return FAILURE;
#endif
		case PHAR_SIG_SHA1: {
			unsigned char digest[20], saved[20];
			PHP_SHA1_CTX  context;

			php_stream_rewind(fp);
			PHP_SHA1Init(&context);
			read_len -= sizeof(digest);
			if (read_len > sizeof(buf)) {
				read_size = sizeof(buf);
			} else {
				read_size = (int)read_len;
			}
			while ((len = php_stream_read(fp, (char*)buf, read_size)) > 0) {
				PHP_SHA1Update(&context, buf, len);
				read_len -= (off_t)len;
				if (read_len < read_size) {
					read_size = (int)read_len;
				}
			}
			PHP_SHA1Final(digest, &context);

			if (read_len > 0
			|| php_stream_read(fp, (char*)saved, sizeof(saved)) != sizeof(saved)
			|| memcmp(digest, saved, sizeof(digest))) {
				efree(savebuf);
				php_stream_close(fp);
				if (error) {
					spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
				}
				return FAILURE;
			}

			sig_len = phar_hex_str((const char*)digest, sizeof(digest), &signature);
			break;
		}
		case PHAR_SIG_MD5: {
			unsigned char digest[16], saved[16];
			PHP_MD5_CTX   context;

			php_stream_rewind(fp);
			PHP_MD5Init(&context);
			read_len -= sizeof(digest);
			if (read_len > sizeof(buf)) {
				read_size = sizeof(buf);
			} else {
				read_size = (int)read_len;
			}
			while ((len = php_stream_read(fp, (char*)buf, read_size)) > 0) {
				PHP_MD5Update(&context, buf, len);
				read_len -= (off_t)len;
				if (read_len < read_size) {
					read_size = (int)read_len;
				}
			}
			PHP_MD5Final(digest, &context);

			if (read_len > 0
			|| php_stream_read(fp, (char*)saved, sizeof(saved)) != sizeof(saved)
			|| memcmp(digest, saved, sizeof(digest))) {
				efree(savebuf);
				php_stream_close(fp);
				if (error) {
					spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
				}
				return FAILURE;
			}

			sig_len = phar_hex_str((const char*)digest, sizeof(digest), &signature);
			break;
		}
		default:
			efree(savebuf);
			php_stream_close(fp);
			if (error) {
				spprintf(error, 0, "phar \"%s\" has a broken or unsupported signature", fname);
			}
			return FAILURE;
		}
	} else if (PHAR_G(require_hash)) {
		efree(savebuf);
		php_stream_close(fp);
		if (error) {
			spprintf(error, 0, "phar \"%s\" does not have a signature", fname);
		}
		return FAILURE;
	} else {
		sig_flags = 0;
		sig_len = 0;
	}

	/* extract alias */
	PHAR_GET_32(buffer, tmp_len);
	if (buffer + tmp_len > endbuffer) {
		MAPPHAR_FAIL("internal corruption of phar \"%s\" (buffer overrun)");
	}
	if (manifest_len < 10 + tmp_len) {
		MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest header)")
	}
	/* tmp_len = 0 says alias length is 0, which means the alias is not stored in the phar */
	if (tmp_len) {
		/* if the alias is stored we enforce it (implicit overrides explicit) */
		if (alias && alias_len && (alias_len != (int)tmp_len || strncmp(alias, buffer, tmp_len)))
		{
			buffer[tmp_len] = '\0';
			efree(savebuf);
			php_stream_close(fp);
			if (signature) {
				efree(signature);
			}
			if (error) {
				spprintf(error, 0, "cannot load phar \"%s\" with implicit alias \"%s\" under different alias \"%s\"", fname, buffer, alias);
			}
			return FAILURE;
		}
		alias_len = tmp_len;
		alias = buffer;
		buffer += tmp_len;
		register_alias = 1;
	} else if (!alias_len || !alias) {
		/* if we neither have an explicit nor an implicit alias, we use the filename */
		alias = NULL;
		alias_len = 0;
		register_alias = 0;
	} else if (alias_len) {
		register_alias = 1;
		temp_alias = 1;
	}
	
	/* we have 5 32-bit items plus 1 byte at least */
	if (manifest_count > ((manifest_len - 10 - tmp_len) / (5 * 4 + 1))) {
		/* prevent serious memory issues */
		MAPPHAR_FAIL("internal corruption of phar \"%s\" (too many manifest entries for size of manifest)")
	}

	mydata = ecalloc(sizeof(phar_archive_data), 1);

	/* check whether we have meta data, zero check works regardless of byte order */
	if (phar_parse_metadata(&buffer, &mydata->metadata, 0 TSRMLS_CC) == FAILURE) {
		MAPPHAR_FAIL("unable to read phar metadata in .phar file \"%s\"");
	}

	/* set up our manifest */
	zend_hash_init(&mydata->manifest, sizeof(phar_entry_info),
		zend_get_hash_value, destroy_phar_manifest_entry, 0);
	zend_hash_init(&mydata->mounted_dirs, sizeof(char *),
		zend_get_hash_value, NULL, 0);
	offset = halt_offset + manifest_len + 4;
	memset(&entry, 0, sizeof(phar_entry_info));
	entry.phar = mydata;
	entry.fp_type = PHAR_FP;

	for (manifest_index = 0; manifest_index < manifest_count; ++manifest_index) {
		if (buffer + 4 > endbuffer) {
			MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest entry)")
		}
		PHAR_GET_32(buffer, entry.filename_len);
		if (entry.filename_len == 0) {
			MAPPHAR_FAIL("zero-length filename encountered in phar \"%s\"");
		}
		if (buffer + entry.filename_len + 20 > endbuffer) {
			MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest entry)");
		}
		if ((manifest_ver & PHAR_API_VER_MASK) >= PHAR_API_MIN_DIR && buffer[entry.filename_len - 1] == '/') {
			entry.is_dir = 1;
		} else {
			entry.is_dir = 0;
		}
		entry.filename = estrndup(buffer, entry.filename_len);
		buffer += entry.filename_len;
		PHAR_GET_32(buffer, entry.uncompressed_filesize);
		PHAR_GET_32(buffer, entry.timestamp);
		if (offset == halt_offset + (int)manifest_len + 4) {
			mydata->min_timestamp = entry.timestamp;
			mydata->max_timestamp = entry.timestamp;
		} else {
			if (mydata->min_timestamp > entry.timestamp) {
				mydata->min_timestamp = entry.timestamp;
			} else if (mydata->max_timestamp < entry.timestamp) {
				mydata->max_timestamp = entry.timestamp;
			}
		}
		PHAR_GET_32(buffer, entry.compressed_filesize);
		PHAR_GET_32(buffer, entry.crc32);
		PHAR_GET_32(buffer, entry.flags);
		if (entry.is_dir) {
			entry.filename_len--;
			entry.flags |= PHAR_ENT_PERM_DEF_DIR;
		}
		if (phar_parse_metadata(&buffer, &entry.metadata, 0 TSRMLS_CC) == FAILURE) {
			efree(entry.filename);
			MAPPHAR_FAIL("unable to read file metadata in .phar file \"%s\"");
		}
		entry.offset = entry.offset_abs = offset;
		offset += entry.compressed_filesize;
		switch (entry.flags & PHAR_ENT_COMPRESSION_MASK) {
		case PHAR_ENT_COMPRESSED_GZ:
			if (!phar_has_zlib) {
				if (entry.metadata) {
					zval_ptr_dtor(&entry.metadata);
				}
				efree(entry.filename);
				MAPPHAR_FAIL("zlib extension is required for gz compressed .phar file \"%s\"");
			}
			break;
		case PHAR_ENT_COMPRESSED_BZ2:
			if (!phar_has_bz2) {
				if (entry.metadata) {
					zval_ptr_dtor(&entry.metadata);
				}
				efree(entry.filename);
				MAPPHAR_FAIL("bz2 extension is required for bzip2 compressed .phar file \"%s\"");
			}
			break;
		default:
			if (entry.uncompressed_filesize != entry.compressed_filesize) {
				if (entry.metadata) {
					zval_ptr_dtor(&entry.metadata);
				}
				efree(entry.filename);
				MAPPHAR_FAIL("internal corruption of phar \"%s\" (compressed and uncompressed size does not match for uncompressed entry)");
			}
			break;
		}
		manifest_flags |= (entry.flags & PHAR_ENT_COMPRESSION_MASK);
		/* if signature matched, no need to check CRC32 for each file */
		entry.is_crc_checked = (manifest_flags & PHAR_HDR_SIGNATURE ? 1 : 0);
		zend_hash_add(&mydata->manifest, entry.filename, entry.filename_len, (void*)&entry, sizeof(phar_entry_info), NULL);
	}

	snprintf(mydata->version, sizeof(mydata->version), "%u.%u.%u", manifest_ver >> 12, (manifest_ver >> 8) & 0xF, (manifest_ver >> 4) & 0xF);
	mydata->internal_file_start = halt_offset + manifest_len + 4;
	mydata->halt_offset = halt_offset;
	mydata->flags = manifest_flags;
	mydata->fp = fp;
	mydata->fname = estrndup(fname, fname_len);
#ifdef PHP_WIN32
	phar_unixify_path_separators(mydata->fname, fname_len);
#endif
	mydata->fname_len = fname_len;
	mydata->alias = alias ? estrndup(alias, alias_len) : estrndup(mydata->fname, fname_len);
	mydata->alias_len = alias ? alias_len : fname_len;
	mydata->sig_flags = sig_flags;
	mydata->sig_len = sig_len;
	mydata->signature = signature;
	phar_request_initialize(TSRMLS_C);
	zend_hash_add(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len, (void*)&mydata, sizeof(phar_archive_data*),  NULL);
	if (register_alias) {
		mydata->is_temporary_alias = temp_alias;
		zend_hash_add(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void*)&mydata, sizeof(phar_archive_data*), NULL);
	} else {
		mydata->is_temporary_alias = 1;
	}
	efree(savebuf);
	
	if (pphar) {
		*pphar = mydata;
	}

	return SUCCESS;
}
/* }}} */

/**
 * Create or open a phar for writing
 */
int phar_open_or_create_filename(char *fname, int fname_len, char *alias, int alias_len, int is_data, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	const char *ext_str, *z;
	int ext_len;

	if (error) {
		*error = NULL;
	}

	/* first try to open an existing file */
	if (phar_detect_phar_fname_ext(fname, 1, &ext_str, &ext_len, !is_data, 0, 1 TSRMLS_CC) == SUCCESS) {
		goto check_file;
	}
	/* next try to create a new file */
	if (FAILURE == phar_detect_phar_fname_ext(fname, 1, &ext_str, &ext_len, !is_data, 1, 1 TSRMLS_CC)) {
		if (error) {
			spprintf(error, 0, "Cannot create phar '%s', file extension (or combination) not recognised", fname);
		}
		return FAILURE;
	}

check_file:
	if (phar_open_loaded(fname, fname_len, alias, alias_len, is_data, options, pphar, 0 TSRMLS_CC) == SUCCESS) {
		if (is_data && !((*pphar)->is_tar || (*pphar)->is_zip)) {
			if (error) {
				spprintf(error, 0, "Cannot open '%s' as a PharData object. Use Phar::__construct() for executable archives", fname);
			}
		}
		if (PHAR_G(readonly) && !is_data && ((*pphar)->is_tar || (*pphar)->is_zip)) {
			phar_entry_info *stub;
			if (FAILURE == zend_hash_find(&((*pphar)->manifest), ".phar/stub.php", sizeof(".phar/stub.php")-1, (void **)&stub)) {
				spprintf(error, 0, "'%s' is not a phar archive. Use PharData::__construct() for a standard zip or tar archive", fname);
				return FAILURE;
			}
		}
		if (pphar && (!PHAR_G(readonly) || is_data)) {
			(*pphar)->is_writeable = 1;
		}
		return SUCCESS;
	}

	if (ext_len > 3 && (z = memchr(ext_str, 'z', ext_len)) && ((ext_str + ext_len) - z >= 2) && !memcmp(z + 1, "ip", 2)) {
		// assume zip-based phar
		return phar_open_or_create_zip(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);
	}

	if (ext_len > 3 && (z = memchr(ext_str, 't', ext_len)) && ((ext_str + ext_len) - z >= 2) && !memcmp(z + 1, "ar", 2)) {
		// assume tar-based phar
		return phar_open_or_create_tar(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);
	}

	return phar_create_or_parse_filename(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);

}

int phar_create_or_parse_filename(char *fname, int fname_len, char *alias, int alias_len, int is_data, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	phar_archive_data *mydata;
	int register_alias;
	php_stream *fp;
	char *actual = NULL;

	if (!pphar) {
		pphar = &mydata;
	}
#if PHP_MAJOR_VERSION < 6
	if (PG(safe_mode) && (!php_checkuid(fname, NULL, CHECKUID_ALLOW_ONLY_FILE))) {
		return FAILURE;
	}
#endif

	if (php_check_open_basedir(fname TSRMLS_CC)) {
		return FAILURE;
	}

	/* first open readonly so it won't be created if not present */
	fp = php_stream_open_wrapper(fname, "rb", IGNORE_URL|STREAM_MUST_SEEK|0, &actual);
	if (actual) {
		fname = actual;
		fname_len = strlen(actual);
	}

	if (fp) {
		if (phar_open_fp(fp, fname, fname_len, alias, alias_len, options, pphar, error TSRMLS_CC) == SUCCESS) {
			if (is_data || !PHAR_G(readonly)) {
				(*pphar)->is_writeable = 1;
			}
			if (actual) {
				efree(actual);
			}
			return SUCCESS;
		} else {
			/* file exists, but is either corrupt or not a phar archive */
			if (actual) {
				efree(actual);
			}
			return FAILURE;
		}
	}
	if (actual) {
		efree(actual);
	}

	if (PHAR_G(readonly) && !is_data) {
		if (options & REPORT_ERRORS) {
			if (error) {
				spprintf(error, 0, "creating archive \"%s\" disabled by INI setting", fname);
			}
		}
		return FAILURE;
	}

	/* set up our manifest */
	mydata = ecalloc(sizeof(phar_archive_data), 1);

	mydata->fname = expand_filepath(fname, NULL TSRMLS_CC);
	fname_len = strlen(mydata->fname);
#ifdef PHP_WIN32
	phar_unixify_path_separators(mydata->fname, fname_len);
#endif
	
	if (pphar) {
		*pphar = mydata;
	}
	zend_hash_init(&mydata->manifest, sizeof(phar_entry_info),
		zend_get_hash_value, destroy_phar_manifest_entry, 0);
	zend_hash_init(&mydata->mounted_dirs, sizeof(char *),
		zend_get_hash_value, NULL, 0);
	mydata->fname_len = fname_len;
	if (is_data) {
		alias = NULL;
		alias_len = 0;
	} else {
		mydata->alias = alias ? estrndup(alias, alias_len) : estrndup(mydata->fname, fname_len);
		mydata->alias_len = alias ? alias_len : fname_len;
	}
	snprintf(mydata->version, sizeof(mydata->version), "%s", PHP_PHAR_API_VERSION);
	mydata->is_temporary_alias = alias ? 0 : 1;
	mydata->internal_file_start = -1;
	mydata->fp = NULL;
	mydata->is_writeable = 1;
	mydata->is_brandnew = 1;
	if (!alias_len || !alias) {
		/* if we neither have an explicit nor an implicit alias, we use the filename */
		alias = NULL;
		alias_len = 0;
		register_alias = 0;
	} else {
		register_alias = 1;
	}
	phar_request_initialize(TSRMLS_C);
	zend_hash_add(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len, (void*)&mydata, sizeof(phar_archive_data*),  NULL);
	if (register_alias) {
		if (FAILURE == zend_hash_add(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void*)&mydata, sizeof(phar_archive_data*), NULL)) {
			if (options & REPORT_ERRORS) {
				if (error) {
					spprintf(error, 0, "archive \"%s\" cannot be associated with alias \"%s\", already in use", fname, alias);
				}
			}
			return FAILURE;
		}
	}
	return SUCCESS;
}
/* }}}*/

/**
 * Return an already opened filename.
 *
 * Or scan a phar file for the required __HALT_COMPILER(); ?> token and verify
 * that the manifest is proper, then pass it to phar_open_file().  SUCCESS
 * or FAILURE is returned and pphar is set to a pointer to the phar's manifest
 */
int phar_open_filename(char *fname, int fname_len, char *alias, int alias_len, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	php_stream *fp;
	char *actual;
	int ret, is_data = 0;

	if (error) {
		*error = NULL;
	}

	if (!strstr(fname, ".phar")) {
		is_data = 1;
	}

	if (phar_open_loaded(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC) == SUCCESS) {
		return SUCCESS;
	} else if (error && *error) {
		return FAILURE;
	}

#if PHP_MAJOR_VERSION < 6
	if (PG(safe_mode) && (!php_checkuid(fname, NULL, CHECKUID_ALLOW_ONLY_FILE))) {
		return FAILURE;
	}
#endif

	if (php_check_open_basedir(fname TSRMLS_CC)) {
		return FAILURE;
	}

	fp = php_stream_open_wrapper(fname, "rb", IGNORE_URL|STREAM_MUST_SEEK, &actual);
	if (!fp) {
		if (options & REPORT_ERRORS) {
			if (error) {
				spprintf(error, 0, "unable to open phar for reading \"%s\"", fname);
			}
		}
		if (actual) {
			efree(actual);
		}
		return FAILURE;
	}

	if (actual) {
		fname = actual;
		fname_len = strlen(actual);
	}

	ret =  phar_open_fp(fp, fname, fname_len, alias, alias_len, options, pphar, error TSRMLS_CC);
	if (actual) {
		efree(actual);
	}
	return ret;
}
/* }}}*/

/**
 * Scan an open fp for the required __HALT_COMPILER(); ?> token and verify
 * that the manifest is proper, then pass it to phar_open_file().  SUCCESS
 * or FAILURE is returned and pphar is set to a pointer to the phar's manifest
 */
static int phar_open_fp(php_stream* fp, char *fname, int fname_len, char *alias, int alias_len, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	const char token[] = "__HALT_COMPILER();";
	const char zip_magic[] = "PK\x03\x04";
	const char gz_magic[] = "\x1f\x8b\x08";
	const char bz_magic[] = "BZh";
	char *pos, buffer[1024 + sizeof(token)], test = '\0';
	const long readsize = sizeof(buffer) - sizeof(token);
	const long tokenlen = sizeof(token) - 1;
	long halt_offset;
	size_t got;
	php_uint32 compression = PHAR_FILE_COMPRESSED_NONE;

	if (error) {
		*error = NULL;
	}
	if (-1 == php_stream_rewind(fp)) {
		MAPPHAR_ALLOC_FAIL("cannot rewind phar \"%s\"")
	}

	buffer[sizeof(buffer)-1] = '\0';
	memset(buffer, 32, sizeof(token));
	halt_offset = 0;

	/* Maybe it's better to compile the file instead of just searching,  */
	/* but we only want the offset. So we want a .re scanner to find it. */
	while(!php_stream_eof(fp)) {
		if ((got = php_stream_read(fp, buffer+tokenlen, readsize)) < (size_t) tokenlen) {
			MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (truncated entry)")
		}
		if (!test) {
			test = '\1';
			pos = buffer+tokenlen;
			if (!memcmp(pos, gz_magic, 3)) {
				char err = 0;
				php_stream_filter *filter;
				php_stream *temp;
				/* to properly decompress, we have to tell zlib to look for a zlib or gzip header */
				zval filterparams;

				if (!phar_has_zlib) {
					MAPPHAR_ALLOC_FAIL("unable to decompress gzipped phar archive \"%s\" to temporary file, enable zlib extension in php.ini")
				}
				array_init(&filterparams);

/* this is defined in zlib's zconf.h */
#ifndef MAX_WBITS
#define MAX_WBITS 15
#endif
				add_assoc_long(&filterparams, "window", MAX_WBITS + 32);
				/* entire file is gzip-compressed, uncompress to temporary file */
				if (!(temp = php_stream_fopen_tmpfile())) {
					MAPPHAR_ALLOC_FAIL("unable to create temporary file for decompression of gzipped phar archive \"%s\"")
				}
				php_stream_rewind(fp);
				filter = php_stream_filter_create("zlib.inflate", &filterparams, php_stream_is_persistent(fp) TSRMLS_CC);
				if (!filter) {
					err = 1;
					add_assoc_long(&filterparams, "window", MAX_WBITS);
					filter = php_stream_filter_create("zlib.inflate", &filterparams, php_stream_is_persistent(fp) TSRMLS_CC);
					zval_dtor(&filterparams);
					if (!filter) {
						php_stream_close(temp);
						MAPPHAR_ALLOC_FAIL("unable to decompress gzipped phar archive \"%s\", ext/zlib is buggy in PHP versions older than 5.2.6")
					}
				} else {
					zval_dtor(&filterparams);
				}
				php_stream_filter_append(&temp->writefilters, filter);
				if (0 == php_stream_copy_to_stream(fp, temp, PHP_STREAM_COPY_ALL)) {
					if (err) {
						php_stream_close(temp);
						MAPPHAR_ALLOC_FAIL("unable to decompress gzipped phar archive \"%s\", ext/zlib is buggy in PHP versions older than 5.2.6")
					}
					php_stream_close(temp);
					MAPPHAR_ALLOC_FAIL("unable to decompress gzipped phar archive \"%s\" to temporary file")
				}
				php_stream_filter_flush(filter, 1);
				php_stream_filter_remove(filter, 1 TSRMLS_CC);
				php_stream_close(fp);
				fp = temp;
				php_stream_rewind(fp);
				compression = PHAR_FILE_COMPRESSED_GZ;

				/* now, start over */
				test = '\0';
				continue;
			} else if (!memcmp(pos, bz_magic, 3)) {
				php_stream_filter *filter;
				php_stream *temp;

				if (!phar_has_bz2) {
					MAPPHAR_ALLOC_FAIL("unable to decompress bzipped phar archive \"%s\" to temporary file, enable bz2 extension in php.ini")
				}
				/* entire file is bzip-compressed, uncompress to temporary file */
				if (!(temp = php_stream_fopen_tmpfile())) {
					MAPPHAR_ALLOC_FAIL("unable to create temporary file for decompression of bzipped phar archive \"%s\"")
				}
				php_stream_rewind(fp);
				filter = php_stream_filter_create("bzip2.decompress", NULL, php_stream_is_persistent(fp) TSRMLS_CC);
				if (!filter) {
					php_stream_close(temp);
					MAPPHAR_ALLOC_FAIL("unable to decompress bzipped phar archive \"%s\", filter creation failed")
				}
				php_stream_filter_append(&temp->writefilters, filter);
				if (0 == php_stream_copy_to_stream(fp, temp, PHP_STREAM_COPY_ALL)) {
					php_stream_close(temp);
					MAPPHAR_ALLOC_FAIL("unable to decompress bzipped phar archive \"%s\" to temporary file")
				}
				php_stream_filter_flush(filter, 1);
				php_stream_filter_remove(filter, 1 TSRMLS_CC);
				php_stream_close(fp);
				fp = temp;
				php_stream_rewind(fp);
				compression = PHAR_FILE_COMPRESSED_BZ2;

				/* now, start over */
				test = '\0';
				continue;
			}
			if (!memcmp(pos, zip_magic, 4)) {
				php_stream_seek(fp, 0, SEEK_END);
				return phar_open_zipfile(fp, fname, fname_len, alias, alias_len, pphar, error TSRMLS_CC);
			}
			if (got > 512) {
				if (phar_is_tar(pos)) {
					php_stream_rewind(fp);
					return phar_open_tarfile(fp, fname, fname_len, alias, alias_len, options, pphar, compression, error TSRMLS_CC);
				}
			}
		}
		if ((pos = strstr(buffer, token)) != NULL) {
			halt_offset += (pos - buffer); /* no -tokenlen+tokenlen here */
			return phar_open_file(fp, fname, fname_len, alias, alias_len, halt_offset, pphar, compression, error TSRMLS_CC);
		}

		halt_offset += got;
		memmove(buffer, buffer + tokenlen, got + 1);
	}
	
	MAPPHAR_ALLOC_FAIL("internal corruption of phar \"%s\" (__HALT_COMPILER(); not found)")
}
/* }}} */

/*
 * given the location of the file extension and the start of the file path,
 * determine the end of the portion of the path (i.e. /path/to/file.ext/blah
 * grabs "/path/to/file.ext" as does the straight /path/to/file.ext),
 * stat it to determine if it exists.
 * if so, check to see if it is a directory and fail if so
 * if not, check to see if its dirname() exists (i.e. "/path/to") and is a directory
 * succeed if we are creating the file, otherwise fail.
 */
static int phar_analyze_path(const char *fname, const char *ext, int ext_len, int for_create TSRMLS_DC)
{
	php_stream_statbuf ssb;
	char *realpath, old, *a = (char *)(ext + ext_len);

	old = *a;
	*a = '\0';
	if ((realpath = expand_filepath(fname, NULL TSRMLS_CC))) {
		if (zend_hash_exists(&(PHAR_GLOBALS->phar_fname_map), realpath, strlen(realpath))) {
			*a = old;
			efree(realpath);
			return SUCCESS;
		}
		efree(realpath);
	}
	if (SUCCESS == php_stream_stat_path((char *) fname, &ssb)) {
		*a = old;
		if (ssb.sb.st_mode & S_IFDIR) {
			return FAILURE;
		}
		if (for_create == 1) {
			return FAILURE;
		}
		return SUCCESS;
	} else {
		char *slash;

		if (!for_create) {
			*a = old;
			return FAILURE;
		}
		slash = (char *) strrchr(fname, '/');
		*a = old;
		if (slash) {
			old = *slash;
			*slash = '\0';
		}
		if (SUCCESS != php_stream_stat_path((char *) fname, &ssb)) {
			if (slash) {
				*slash = old;
			} else {
				if (!(realpath = expand_filepath(fname, NULL TSRMLS_CC))) {
					return FAILURE;
				}
				a = strstr(realpath, fname) + ((ext - fname) + ext_len);
				*a = '\0';
				slash = strrchr(realpath, '/');
				if (slash) {
					*slash = '\0';
				} else {
					efree(realpath);
					return FAILURE;
				}
				if (SUCCESS != php_stream_stat_path(realpath, &ssb)) {
					efree(realpath);
					return FAILURE;
				}
				efree(realpath);
				if (ssb.sb.st_mode & S_IFDIR) {
					return SUCCESS;
				}
			}
			return FAILURE;
		}
		if (slash) {
			*slash = old;
		}
		if (ssb.sb.st_mode & S_IFDIR) {
			return SUCCESS;
		}
		return FAILURE;
	}
}

/* check for ".phar" in extension */
static int phar_check_str(const char *fname, const char *ext_str, int ext_len, int executable, int for_create TSRMLS_DC)
{
	char test[51];
	const char *pos;

	if (ext_len >= 50) {
		return FAILURE;
	}
	if (executable == 1) {
		/* copy "." as well */
		memcpy(test, ext_str - 1, ext_len + 1);
		test[ext_len + 1] = '\0';
		/* executable phars must contain ".phar" as a valid extension (phar://.pharmy/oops is invalid) */
		/* (phar://hi/there/.phar/oops is also invalid) */
		if ((pos = strstr(test, ".phar")) && (*(pos - 1) != '/')
				&& (pos += 5) && (*pos == '\0' || *pos == '/' || *pos == '.')) {
			return phar_analyze_path(fname, ext_str, ext_len, for_create TSRMLS_CC);
		} else {
			return FAILURE;
		}
	}
	/* data phars need only contain a single non-"." to be valid */
	if (*(ext_str + 1) != '.' && *(ext_str + 1) != '/' && *(ext_str + 1) != '\0') {
		return phar_analyze_path(fname, ext_str, ext_len, for_create TSRMLS_CC);
	}
	return FAILURE;
}

/*
 * if executable is 1, only returns SUCCESS if the extension is one of the tar/zip .phar extensions
 * if executable is 0, it returns SUCCESS only if the filename does *not* contain ".phar" anywhere, and treats
 * the first extension as the filename extension
 *
 * if an extension is found, it sets ext_str to the location of the file extension in filename,
 * and ext_len to the length of the extension.
 * for urls like "phar://alias/oops" it instead sets ext_len to -1 and returns FAILURE, which tells
 * the calling function to use "alias" as the phar alias
 *
 * the last parameter should be set to tell the thing to assume that filename is the full path, and only to check the
 * extension rules, not to iterate.
 */
int phar_detect_phar_fname_ext(const char *filename, int check_length, const char **ext_str, int *ext_len, int executable, int for_create, int is_complete TSRMLS_DC) /* {{{ */
{
	const char *pos, *slash;
	int filename_len = strlen(filename);

	*ext_str = NULL;

	if (!filename_len || filename_len == 1) {
		return FAILURE;
	}
	phar_request_initialize(TSRMLS_C);
	/* first check for extract_list */
	if (zend_hash_num_elements(&(PHAR_GLOBALS->phar_plain_map))) {
		for (zend_hash_internal_pointer_reset(&(PHAR_GLOBALS->phar_plain_map));
		zend_hash_has_more_elements(&(PHAR_GLOBALS->phar_plain_map)) == SUCCESS;
		zend_hash_move_forward(&(PHAR_GLOBALS->phar_plain_map))) {
			char *key;
			uint keylen;
			ulong intkey;

			if (HASH_KEY_IS_STRING != zend_hash_get_current_key_ex(&(PHAR_GLOBALS->phar_plain_map), &key, &keylen, &intkey, 0, NULL)) {
				continue;
			}

			if (keylen <= filename_len && !memcmp(key, filename, keylen - 1)) {
				/* found plain map, so we grab the extension, if any */
				if (is_complete && keylen != filename_len + 1) {
					continue;
				}
				if (for_create == 1) {
					return FAILURE;
				}
				if (!executable == 1) {
					return FAILURE;
				}
				pos = strrchr(key, '/');
				if (pos) {
					pos = filename + (pos - key);
					slash = strchr(pos, '.');
					if (slash) {
						*ext_str = slash;
						*ext_len = keylen - (slash - filename);
						return SUCCESS;
					}
					*ext_str = pos;
					*ext_len = -1;
					return FAILURE;
				}
				*ext_str = filename + keylen - 1;
				*ext_len = -1;
				return FAILURE;
			}
		}
	}
	/* next check for alias in first segment */
	pos = strchr(filename, '/');
	if (pos) {
		if (zend_hash_exists(&(PHAR_GLOBALS->phar_alias_map), filename, pos - filename)) {
			*ext_str = pos;
			*ext_len = -1;
			return FAILURE;
		}
	}

	pos = strchr(filename + 1, '.');
next_extension:
	if (!pos) {
		return FAILURE;
	}

	slash = strchr(pos, '/');
	if (!slash) {
		/* this is a url like "phar://blah.phar" with no directory */
		*ext_str = pos;
		*ext_len = strlen(pos);
		/* file extension must contain "phar" */
		switch (phar_check_str(filename, *ext_str, *ext_len, executable, for_create TSRMLS_CC)) {
			case SUCCESS :
				return SUCCESS;
			case FAILURE :
				/* we are at the end of the string, so we fail */
				return FAILURE;
		}
	}
	/* we've found an extension that ends at a directory separator */
	*ext_str = pos;
	*ext_len = slash - pos;
	switch (phar_check_str(filename, *ext_str, *ext_len, executable, for_create TSRMLS_CC)) {
		case SUCCESS :
			return SUCCESS;
		case FAILURE :
			/* look for more extensions */
			if (is_complete) {
				return FAILURE;
			}
			pos = strchr(pos + 1, '.');
			if (pos) {
				*ext_str = NULL;
				*ext_len = 0;
			}
			goto next_extension;
	}
	return FAILURE;
}
/* }}} */

static int php_check_dots(const char *element, int n) 
{
	for(n--; n >= 0; --n) {
		if (element[n] != '.') {
			return 1;
		}
	}
	return 0;
}

#define IS_DIRECTORY_UP(element, len) \
	(len >= 2 && !php_check_dots(element, len))

#define IS_DIRECTORY_CURRENT(element, len) \
	(len == 1 && element[0] == '.')

#define IS_BACKSLASH(c)     ((c) == '/')

#ifdef COMPILE_DL_PHAR
/* stupid-ass non-extern declaration in tsrm_strtok.h breaks dumbass MS compiler */
static inline int in_character_class(char ch, const char *delim)
{
	while (*delim) {
		if (*delim == ch) {
			return 1;
		}
		++delim;
	}
	return 0;
}

char *tsrm_strtok_r(char *s, const char *delim, char **last)
{
	char *token;

	if (s == NULL) {
		s = *last;
	}

	while (*s && in_character_class(*s, delim)) {
		++s;
	}
	if (!*s) {
		return NULL;
	}

	token = s;

	while (*s && !in_character_class(*s, delim)) {
		++s;
	}
	if (!*s) {
		*last = s;
	} else {
		*s = '\0';
		*last = s + 1;
	}
	return token;
}
#endif

/**
 * Remove .. and . references within a phar filename
 */
char *phar_fix_filepath(char *path, int *new_len, int use_cwd TSRMLS_DC) /* {{{ */
{
	char *ptr, *free_path, *new_phar;
	char *tok;
	int ptr_length, new_phar_len = 1, path_length = *new_len;

	if (PHAR_G(cwd_len) && use_cwd && path_length > 2 && path[0] == '.' && path[1] == '/') {
		free_path = path = estrndup(path, path_length);
		new_phar_len = PHAR_G(cwd_len);
		new_phar = estrndup(PHAR_G(cwd), new_phar_len);
	} else {
		free_path = path;
		new_phar = estrndup("/\0", 2);
	}
	tok = NULL;
	ptr = tsrm_strtok_r(path, "/", &tok);
	while (ptr) {
		ptr_length = strlen(ptr);

		if (IS_DIRECTORY_UP(ptr, ptr_length)) {
			char save;

			save = '/';

#define PREVIOUS new_phar[new_phar_len - 1]

			while (new_phar_len > 1 &&
					!IS_BACKSLASH(PREVIOUS)) {
				save = PREVIOUS;
				PREVIOUS = '\0';
				new_phar_len--;
			}

			if (new_phar[0] != '/') {
				new_phar[new_phar_len++] = save;
				new_phar[new_phar_len] = '\0';
			} else if (new_phar_len > 1) {
				PREVIOUS = '\0';
				new_phar_len--;
			}
		} else if (!IS_DIRECTORY_CURRENT(ptr, ptr_length)) {
			if (new_phar_len > 1) {
				new_phar = (char *) erealloc(new_phar, new_phar_len+ptr_length+1+1);
				new_phar[new_phar_len++] = '/';
				memcpy(&new_phar[new_phar_len], ptr, ptr_length+1);
			} else {
				new_phar = (char *) erealloc(new_phar, new_phar_len+ptr_length+1);
				memcpy(&new_phar[new_phar_len], ptr, ptr_length+1);
			}

			new_phar_len += ptr_length;
		}
		ptr = tsrm_strtok_r(NULL, "/", &tok);
	}

	if (path[path_length-1] == '/' && new_phar_len > 1) {
		new_phar = (char*)erealloc(new_phar, new_phar_len + 2);
		new_phar[new_phar_len++] = '/';
		new_phar[new_phar_len] = 0;
	}

	efree(free_path);

	if (new_phar_len == 0) {
		new_phar = (char *) erealloc(new_phar, new_phar_len+1+1);
		new_phar[new_phar_len] = '/';
		new_phar[++new_phar_len] = '\0';
	}
	*new_len = new_phar_len;
	return new_phar;
}
/* }}} */

/**
 * Process a phar stream name, ensuring we can handle any of:
 * 
 * - whatever.phar
 * - whatever.phar.gz
 * - whatever.phar.bz2
 * - whatever.phar.php
 *
 * Optionally the name might start with 'phar://'
 *
 * This is used by phar_open_url()
 */
int phar_split_fname(char *filename, int filename_len, char **arch, int *arch_len, char **entry, int *entry_len, int executable, int for_create TSRMLS_DC) /* {{{ */
{
	const char *ext_str;
	int ext_len;

	if (!strncasecmp(filename, "phar://", 7)) {
		filename += 7;
		filename_len -= 7;
	}

	ext_len = 0;
	if (phar_detect_phar_fname_ext(filename, 0, &ext_str, &ext_len, executable, for_create, 0) == FAILURE) {
		if (ext_len != -1) {
			if (!ext_str) {
				/* no / detected, restore arch for error message */
				*arch = filename;
			}
			return FAILURE;
		}
		ext_len = 0;
		/* no extension detected - instead we are dealing with an alias */
	}

	*arch_len = ext_str - filename + ext_len;
	*arch = estrndup(filename, *arch_len);
#ifdef PHP_WIN32
	phar_unixify_path_separators(*arch, *arch_len);
#endif
	if (ext_str[ext_len]) {
		*entry_len = filename_len - *arch_len;
		*entry = estrndup(ext_str+ext_len, *entry_len);
#ifdef PHP_WIN32
		phar_unixify_path_separators(*entry, *entry_len);
#endif
		*entry = phar_fix_filepath(*entry, entry_len, 0 TSRMLS_CC);
	} else {
		*entry_len = 1;
		*entry = estrndup("/", 1);
	}
	return SUCCESS;
}
/* }}} */

/**
 * Invoked when a user calls Phar::mapPhar() from within an executing .phar
 * to set up its manifest directly
 */
int phar_open_compiled_file(char *alias, int alias_len, char **error TSRMLS_DC) /* {{{ */
{
	char *fname;
	long halt_offset;
	zval *halt_constant;
	php_stream *fp;
	int fname_len, is_data = 0;

	if (error) {
		*error = NULL;
	}
	fname = zend_get_executed_filename(TSRMLS_C);
	fname_len = strlen(fname);

	if (!strstr(fname, ".phar")) {
		is_data = 1;
	}

	if (phar_open_loaded(fname, fname_len, alias, alias_len, is_data, REPORT_ERRORS, NULL, 0 TSRMLS_CC) == SUCCESS) {
		return SUCCESS;
	}

	if (!strcmp(fname, "[no active file]")) {
		if (error) {
			spprintf(error, 0, "cannot initialize a phar outside of PHP execution");
		}
		return FAILURE;
	}

	MAKE_STD_ZVAL(halt_constant);
	if (0 == zend_get_constant("__COMPILER_HALT_OFFSET__", 24, halt_constant TSRMLS_CC)) {
		FREE_ZVAL(halt_constant);
		if (error) {
			spprintf(error, 0, "__HALT_COMPILER(); must be declared in a phar");
		}
		return FAILURE;
	}
	halt_offset = Z_LVAL(*halt_constant);
	FREE_ZVAL(halt_constant);
	
	fp = php_stream_open_wrapper(fname, "rb", IGNORE_URL|STREAM_MUST_SEEK|REPORT_ERRORS, NULL);

	if (!fp) {
		if (error) {
			spprintf(error, 0, "unable to open phar for reading \"%s\"", fname);
		}
		return FAILURE;
	}

	return phar_open_file(fp, fname, fname_len, alias, alias_len, halt_offset, NULL, PHAR_FILE_COMPRESSED_NONE, error TSRMLS_CC);
}
/* }}} */

/**
 * Validate the CRC32 of a file opened from within the phar
 */
int phar_postprocess_file(php_stream_wrapper *wrapper, int options, phar_entry_data *idata, php_uint32 crc32, char **error TSRMLS_DC) /* {{{ */
{
	php_uint32 crc = ~0;
	int len = idata->internal_file->uncompressed_filesize;
	php_stream *fp = idata->fp;
	phar_entry_info *entry = idata->internal_file;

	if (error) {
		*error = NULL;
	}
	if (entry->is_zip) {
		/* verify local file header */
		phar_zip_file_header local;

		if (SUCCESS != phar_open_archive_fp(idata->phar TSRMLS_CC)) {
			spprintf(error, 0, "phar error: unable to open zip-based phar archive \"%s\" to verify local file header for file \"%s\"", idata->phar->fname, entry->filename);
			return FAILURE;
		}
		php_stream_seek(idata->phar->fp, entry->header_offset, SEEK_SET);

		if (sizeof(local) != php_stream_read(idata->phar->fp, (char *) &local, sizeof(local))) {

			spprintf(error, 0, "phar error: internal corruption of zip-based phar \"%s\" (cannot read local file header for file \"%s\")", idata->phar->fname, entry->filename);
			return FAILURE;
		}
		/* fix up for big-endian systems */
		/* verify local header if not yet verified */
		if (entry->filename_len != PHAR_ZIP_16(local.filename_len) || entry->crc32 != PHAR_ZIP_32(local.crc32) || entry->uncompressed_filesize != PHAR_ZIP_32(local.uncompsize) || entry->compressed_filesize != PHAR_ZIP_32(local.compsize)) {
			spprintf(error, 0, "phar error: internal corruption of zip-based phar \"%s\" (local head of file \"%s\" does not match central directory)", idata->phar->fname, entry->filename);
			return FAILURE;
		}
		if (-1 == php_stream_seek(idata->phar->fp, PHAR_ZIP_16(local.filename_len) + PHAR_ZIP_16(local.extra_len), SEEK_CUR)) {
			spprintf(error, 0, "phar error: internal corruption of zip-based phar \"%s\" (cannot seek to start of file data for file \"%s\")", idata->phar->fname, entry->filename);
			return FAILURE;
		}
	}
	php_stream_seek(fp, idata->zero, SEEK_SET);	
	while (len--) {
		CRC32(crc, php_stream_getc(fp));
	}
	php_stream_seek(fp, idata->zero, SEEK_SET);
	if (~crc == crc32) {
		entry->is_crc_checked = 1;
		return SUCCESS;
	} else {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: internal corruption of phar \"%s\" (crc32 mismatch on file \"%s\")", idata->phar->fname, entry->filename);
		return FAILURE;
	}
}
/* }}} */

static inline void phar_set_32(char *buffer, int var) /* {{{ */
{
#ifdef WORDS_BIGENDIAN
	*((buffer) + 3) = (unsigned char) (((var) >> 24) & 0xFF);
	*((buffer) + 2) = (unsigned char) (((var) >> 16) & 0xFF);
	*((buffer) + 1) = (unsigned char) (((var) >> 8) & 0xFF);
	*((buffer) + 0) = (unsigned char) ((var) & 0xFF);
#else
	*(php_uint32 *)(buffer) = (php_uint32)(var);
#endif
} /* }}} */

/**
 * The only purpose of this is to store the API version, which was stored bigendian for some reason
 * in the original PHP_Archive, so we will do the same
 */
static inline void phar_set_16(char *buffer, int var) /* {{{ */
{
#ifdef WORDS_BIGENDIAN
	*((buffer) + 1) = (unsigned char) (((var) >> 8) & 0xFF); \
	*(buffer) = (unsigned char) ((var) & 0xFF);
#else
	*(php_uint16 *)(buffer) = (php_uint16)(var);
#endif
} /* }}} */

static int phar_flush_clean_deleted_apply(void *data TSRMLS_DC) /* {{{ */
{
	phar_entry_info *entry = (phar_entry_info *)data;

	if (entry->fp_refcount <= 0 && entry->is_deleted) {
		return ZEND_HASH_APPLY_REMOVE;
	} else {
		return ZEND_HASH_APPLY_KEEP;
	}
}
/* }}} */

#include "stub.h"

char *phar_create_default_stub(const char *index_php, const char *web_index, size_t *len, char **error TSRMLS_DC)
{
	char *stub = NULL;
	int index_len, web_len;
	size_t dummy;

	if (!len) {
		len = &dummy;
	}

	if (error) {
		*error = NULL;
	}

	if (!index_php) {
		index_php = "index.php";
	}

	if (!web_index) {
		web_index = "index.php";
	}

	index_len = strlen(index_php);
	web_len = strlen(web_index);

	if (index_len > 400) {
		/* ridiculous size not allowed for index.php startup filename */
		if (error) {
			spprintf(error, 0, "Illegal filename passed in for stub creation, was %d characters long, and only 400 or less is allowed", index_len);
			return NULL;
		}
	}

	if (web_len > 400) {
		/* ridiculous size not allowed for index.php startup filename */
		if (error) {
			spprintf(error, 0, "Illegal web filename passed in for stub creation, was %d characters long, and only 400 or less is allowed", web_len);
			return NULL;
		}
	}

	phar_get_stub(index_php, web_index, len, &stub, index_len+1, web_len+1 TSRMLS_CC);
	return stub;
}

/**
 * Save phar contents to disk
 *
 * user_stub contains either a string, or a resource pointer, if len is a negative length.
 * user_stub and len should be both 0 if the default or existing stub should be used
 */
int phar_flush(phar_archive_data *phar, char *user_stub, long len, int convert, char **error TSRMLS_DC) /* {{{ */
{
/*	static const char newstub[] = "<?php __HALT_COMPILER(); ?>\r\n"; */
	char *newstub;
	phar_entry_info *entry, *newentry;
	int halt_offset, restore_alias_len, global_flags = 0, closeoldfile;
	char *pos, has_dirs = 0;
	char manifest[18], entry_buffer[24];
	off_t manifest_ftell;
	long offset;
	size_t wrote;
	php_uint32 manifest_len, mytime, loc, new_manifest_count;
	php_uint32 newcrc32;
	php_stream *file, *oldfile, *newfile, *stubfile;
	php_stream_filter *filter;
	php_serialize_data_t metadata_hash;
	smart_str main_metadata_str = {0};
	int free_user_stub;

	if (error) {
		*error = NULL;
	}

	if (PHAR_G(readonly) && !phar->is_data) {
		return EOF;
	}

	if (phar->is_zip) {
		return phar_zip_flush(phar, user_stub, len, convert, error TSRMLS_CC);
	}
	if (phar->is_tar) {
		return phar_tar_flush(phar, user_stub, len, convert, error TSRMLS_CC);
	}

	if (phar->fp && !phar->is_brandnew) {
		oldfile = phar->fp;
		closeoldfile = 0;
		php_stream_rewind(oldfile);
	} else {
		oldfile = php_stream_open_wrapper(phar->fname, "rb", 0, NULL);
		closeoldfile = oldfile != NULL;
	}
	newfile = php_stream_fopen_tmpfile();
	if (!newfile) {
		if (error) {
			spprintf(error, 0, "unable to create temporary file");
		}
		if (closeoldfile) {
			php_stream_close(oldfile);
		}
		return EOF;
	}

	if (user_stub) {
		if (len < 0) {
			/* resource passed in */
			if (!(php_stream_from_zval_no_verify(stubfile, (zval **)user_stub))) {
				if (closeoldfile) {
					php_stream_close(oldfile);
				}
				php_stream_close(newfile);
				if (error) {
					spprintf(error, 0, "unable to access resource to copy stub to new phar \"%s\"", phar->fname);
				}
				return EOF;
			}
			if (len == -1) {
				len = PHP_STREAM_COPY_ALL;
			} else {
				len = -len;
			}
			user_stub = 0;
			if (!(len = php_stream_copy_to_mem(stubfile, &user_stub, len, 0)) || !user_stub) {
				if (closeoldfile) {
					php_stream_close(oldfile);
				}
				php_stream_close(newfile);
				if (error) {
					spprintf(error, 0, "unable to read resource to copy stub to new phar \"%s\"", phar->fname);
				}
				return EOF;
			}
			free_user_stub = 1;
		} else {
			free_user_stub = 0;
		}
		if ((pos = strstr(user_stub, "__HALT_COMPILER();")) == NULL)
		{
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "illegal stub for phar \"%s\"", phar->fname);
			}
			if (free_user_stub) {
				efree(user_stub);
			}
			return EOF;
		}
		len = pos - user_stub + 18;
		if ((size_t)len != php_stream_write(newfile, user_stub, len)
		||            5 != php_stream_write(newfile, " ?>\r\n", 5)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to create stub from string in new phar \"%s\"", phar->fname);
			}
			if (free_user_stub) {
				efree(user_stub);
			}
			return EOF;
		}
		phar->halt_offset = len + 5;
		if (free_user_stub) {
			efree(user_stub);
		}
	} else {
		if (!user_stub && phar->halt_offset && oldfile && !phar->is_brandnew) {
			if (phar->halt_offset != php_stream_copy_to_stream(oldfile, newfile, phar->halt_offset)) {
				if (closeoldfile) {
					php_stream_close(oldfile);
				}
				php_stream_close(newfile);
				if (error) {
					spprintf(error, 0, "unable to copy stub of old phar to new phar \"%s\"", phar->fname);
				}
				return EOF;
			}
		} else {
			/* this is either a brand new phar or a default stub overwrite */
			newstub = phar_create_default_stub(NULL, NULL, &(phar->halt_offset), NULL TSRMLS_CC);
			if (phar->halt_offset != php_stream_write(newfile, newstub, phar->halt_offset)) {
				efree(newstub);
				if (closeoldfile) {
					php_stream_close(oldfile);
				}
				php_stream_close(newfile);
				if (error) {
					spprintf(error, 0, "unable to create stub in new phar \"%s\"", phar->fname);
				}
				return EOF;
			}
			efree(newstub);
		}
	}
	manifest_ftell = php_stream_tell(newfile);
	halt_offset = manifest_ftell;

	/* Check whether we can get rid of some of the deleted entries which are
	 * unused. However some might still be in use so even after this clean-up
	 * we need to skip entries marked is_deleted. */
	zend_hash_apply(&phar->manifest, phar_flush_clean_deleted_apply TSRMLS_CC);

	/* compress as necessary, calculate crcs, serialize meta-data, manifest size, and file sizes */
	main_metadata_str.c = 0;
	if (phar->metadata) {
		PHP_VAR_SERIALIZE_INIT(metadata_hash);
		php_var_serialize(&main_metadata_str, &phar->metadata, &metadata_hash TSRMLS_CC);
		PHP_VAR_SERIALIZE_DESTROY(metadata_hash);
	} else {
		main_metadata_str.len = 0;
	}
	new_manifest_count = 0;
	offset = 0;
	for (zend_hash_internal_pointer_reset(&phar->manifest);
	    zend_hash_has_more_elements(&phar->manifest) == SUCCESS;
	    zend_hash_move_forward(&phar->manifest)) {
		if (zend_hash_get_current_data(&phar->manifest, (void **)&entry) == FAILURE) {
			continue;
		}
		if (entry->cfp) {
			/* did we forget to get rid of cfp last time? */
			php_stream_close(entry->cfp);
			entry->cfp = 0;
		}
		if (entry->is_deleted) {
			/* remove this from the new phar */
			continue;
		}
		/* after excluding deleted files, calculate manifest size in bytes and number of entries */
		++new_manifest_count;

		if (entry->is_dir) {
			/* we use this to calculate API version, 1.1.1 is used for phars with directories */
			has_dirs = 1;
		}
		if (entry->metadata) {
			if (entry->metadata_str.c) {
				smart_str_free(&entry->metadata_str);
			}
			entry->metadata_str.c = 0;
			entry->metadata_str.len = 0;
			PHP_VAR_SERIALIZE_INIT(metadata_hash);
			php_var_serialize(&entry->metadata_str, &entry->metadata, &metadata_hash TSRMLS_CC);
			PHP_VAR_SERIALIZE_DESTROY(metadata_hash);
		} else {
			if (entry->metadata_str.c) {
				smart_str_free(&entry->metadata_str);
			}
			entry->metadata_str.c = 0;
			entry->metadata_str.len = 0;
		}

		/* 32 bits for filename length, length of filename, manifest + metadata, and add 1 for trailing / if a directory */
		offset += 4 + entry->filename_len + sizeof(entry_buffer) + entry->metadata_str.len + (entry->is_dir ? 1 : 0);

		/* compress and rehash as necessary */
		if ((oldfile && !entry->is_modified) || entry->is_dir) {
			continue;
		}
		if (!phar_get_efp(entry TSRMLS_CC)) {
			/* re-open internal file pointer just-in-time */
			newentry = phar_open_jit(phar, entry, oldfile, error, 0 TSRMLS_CC);
			if (!newentry) {
				/* major problem re-opening, so we ignore this file and the error */
				efree(*error);
				*error = NULL;
				continue;
			}
			entry = newentry;
		}
		file = phar_get_efp(entry TSRMLS_CC);
		if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0 TSRMLS_CC)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to seek to start of file \"%s\" while creating new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		newcrc32 = ~0;
		mytime = entry->uncompressed_filesize;
		for (loc = 0;loc < mytime; ++loc) {
			CRC32(newcrc32, php_stream_getc(file));
		}
		entry->crc32 = ~newcrc32;
		entry->is_crc_checked = 1;
		if (!(entry->flags & PHAR_ENT_COMPRESSION_MASK)) {
			/* not compressed */
			entry->compressed_filesize = entry->uncompressed_filesize;
			continue;
		}
		filter = php_stream_filter_create(phar_compress_filter(entry, 0), NULL, 0 TSRMLS_CC);
		if (!filter) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (entry->flags & PHAR_ENT_COMPRESSED_GZ) {
				if (error) {
					spprintf(error, 0, "unable to gzip compress file \"%s\" to new phar \"%s\"", entry->filename, phar->fname);
				}
			} else {
				if (error) {
					spprintf(error, 0, "unable to bzip2 compress file \"%s\" to new phar \"%s\"", entry->filename, phar->fname);
				}
			}
			return EOF;
		}

		/* create new file that holds the compressed version */
		/* work around inability to specify freedom in write and strictness
		in read count */
		entry->cfp = php_stream_fopen_tmpfile();
		if (!entry->cfp) {
			if (error) {
				spprintf(error, 0, "unable to create temporary file");
			}
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			return EOF;
		}
		php_stream_flush(file);
		if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0 TSRMLS_CC)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to seek to start of file \"%s\" while creating new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		php_stream_filter_append((&entry->cfp->writefilters), filter);
		if (entry->uncompressed_filesize != php_stream_copy_to_stream(file, entry->cfp, entry->uncompressed_filesize)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to copy compressed file contents of file \"%s\" while creating new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		php_stream_filter_flush(filter, 1);
		php_stream_flush(entry->cfp);
		php_stream_filter_remove(filter, 1 TSRMLS_CC);
		php_stream_seek(entry->cfp, 0, SEEK_END);
		entry->compressed_filesize = (php_uint32) php_stream_tell(entry->cfp);
		/* generate crc on compressed file */
		php_stream_rewind(entry->cfp);
		entry->old_flags = entry->flags;
		entry->is_modified = 1;
		global_flags |= (entry->flags & PHAR_ENT_COMPRESSION_MASK);
	}
	global_flags |= PHAR_HDR_SIGNATURE;

	/* write out manifest pre-header */
	/*  4: manifest length
	 *  4: manifest entry count
	 *  2: phar version
	 *  4: phar global flags
	 *  4: alias length
	 *  ?: the alias itself
	 *  4: phar metadata length
	 *  ?: phar metadata
	 */
	restore_alias_len = phar->alias_len;
	if (phar->is_temporary_alias) {
		phar->alias_len = 0;
	}

	manifest_len = offset + phar->alias_len + sizeof(manifest) + main_metadata_str.len;
	phar_set_32(manifest, manifest_len);
	phar_set_32(manifest+4, new_manifest_count);
	if (has_dirs) {
		*(manifest + 8) = (unsigned char) (((PHAR_API_VERSION) >> 8) & 0xFF);
		*(manifest + 9) = (unsigned char) (((PHAR_API_VERSION) & 0xF0));
	} else {
		*(manifest + 8) = (unsigned char) (((PHAR_API_VERSION_NODIR) >> 8) & 0xFF);
		*(manifest + 9) = (unsigned char) (((PHAR_API_VERSION_NODIR) & 0xF0));
	}
	phar_set_32(manifest+10, global_flags);
	phar_set_32(manifest+14, phar->alias_len);

	/* write the manifest header */
	if (sizeof(manifest) != php_stream_write(newfile, manifest, sizeof(manifest))
	|| (size_t)phar->alias_len != php_stream_write(newfile, phar->alias, phar->alias_len)) {
		if (closeoldfile) {
			php_stream_close(oldfile);
		}
		php_stream_close(newfile);
		phar->alias_len = restore_alias_len;
		if (error) {
			spprintf(error, 0, "unable to write manifest header of new phar \"%s\"", phar->fname);
		}
		return EOF;
	}
	
	phar->alias_len = restore_alias_len;
	
	phar_set_32(manifest, main_metadata_str.len);
	if (main_metadata_str.len) {
		if (4 != php_stream_write(newfile, manifest, 4) ||
		main_metadata_str.len != php_stream_write(newfile, main_metadata_str.c, main_metadata_str.len)) {
			smart_str_free(&main_metadata_str);
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			phar->alias_len = restore_alias_len;
			if (error) {
				spprintf(error, 0, "unable to write manifest meta-data of new phar \"%s\"", phar->fname);
			}
			return EOF;
		} 
	} else {
		if (4 != php_stream_write(newfile, manifest, 4)) {
			smart_str_free(&main_metadata_str);
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			phar->alias_len = restore_alias_len;
			if (error) {
				spprintf(error, 0, "unable to write manifest header of new phar \"%s\"", phar->fname);
			}
			return EOF;
		}
	}
	smart_str_free(&main_metadata_str);
	
	/* re-calculate the manifest location to simplify later code */
	manifest_ftell = php_stream_tell(newfile);
	
	/* now write the manifest */
	for (zend_hash_internal_pointer_reset(&phar->manifest);
	    zend_hash_has_more_elements(&phar->manifest) == SUCCESS;
	    zend_hash_move_forward(&phar->manifest)) {
		if (zend_hash_get_current_data(&phar->manifest, (void **)&entry) == FAILURE) {
			continue;
		}
		if (entry->is_deleted) {
			/* remove this from the new phar */
			continue;
		}
		if (entry->is_dir) {
			/* add 1 for trailing slash */
			phar_set_32(entry_buffer, entry->filename_len + 1);
		} else {
			phar_set_32(entry_buffer, entry->filename_len);
		}
		if (4 != php_stream_write(newfile, entry_buffer, 4)
		|| entry->filename_len != php_stream_write(newfile, entry->filename, entry->filename_len)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to write filename of file \"%s\" to manifest of new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		if (entry->is_dir && 1 != php_stream_write(newfile, "/", 1)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to write filename of directory \"%s\" to manifest of new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		/* set the manifest meta-data:
			4: uncompressed filesize
			4: creation timestamp
			4: compressed filesize
			4: crc32
			4: flags
			4: metadata-len
			+: metadata
		*/
		mytime = time(NULL);
		phar_set_32(entry_buffer, entry->uncompressed_filesize);
		phar_set_32(entry_buffer+4, mytime);
		phar_set_32(entry_buffer+8, entry->compressed_filesize);
		phar_set_32(entry_buffer+12, entry->crc32);
		phar_set_32(entry_buffer+16, entry->flags);
		phar_set_32(entry_buffer+20, entry->metadata_str.len);
		if (sizeof(entry_buffer) != php_stream_write(newfile, entry_buffer, sizeof(entry_buffer))
		|| entry->metadata_str.len != php_stream_write(newfile, entry->metadata_str.c, entry->metadata_str.len)) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to write temporary manifest of file \"%s\" to manifest of new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
	}
	
	/* now copy the actual file data to the new phar */
	offset = php_stream_tell(newfile);
	for (zend_hash_internal_pointer_reset(&phar->manifest);
	    zend_hash_has_more_elements(&phar->manifest) == SUCCESS;
	    zend_hash_move_forward(&phar->manifest)) {
		if (zend_hash_get_current_data(&phar->manifest, (void **)&entry) == FAILURE) {
			continue;
		}
		if (entry->is_deleted || entry->is_dir) {
			continue;
		}
		if (entry->cfp) {
			file = entry->cfp;
			php_stream_rewind(file);
		} else {
			file = phar_get_efp(entry TSRMLS_CC);
			if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0 TSRMLS_CC)) {
				if (closeoldfile) {
					php_stream_close(oldfile);
				}
				php_stream_close(newfile);
				if (error) {
					spprintf(error, 0, "unable to seek to start of file \"%s\" while creating new phar \"%s\"", entry->filename, phar->fname);
				}
				return EOF;
			}
		}
		if (!file) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to seek to start of file \"%s\" while creating new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		/* this will have changed for all files that have either
		   changed compression or been modified */
		entry->offset = entry->offset_abs = offset;
		offset += entry->compressed_filesize;
		wrote = php_stream_copy_to_stream(file, newfile, entry->compressed_filesize);
		if (entry->compressed_filesize != wrote) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to write contents of file \"%s\" to new phar \"%s\"", entry->filename, phar->fname);
			}
			return EOF;
		}
		entry->is_modified = 0;
		if (entry->cfp) {
			php_stream_close(entry->cfp);
			entry->cfp = NULL;
		}
		if (entry->fp_type == PHAR_MOD) {
			/* this fp is in use by a phar_entry_data returned by phar_get_entry_data, it will be closed
			   when the phar_entry_data is phar_entry_delref'ed */
			if (entry->fp_refcount == 0 && entry->fp != phar->fp && entry->fp != phar->ufp) {
				php_stream_close(entry->fp);
			}
			entry->fp = NULL;
			entry->fp_type = PHAR_FP;
		} else if (entry->fp_type == PHAR_UFP) {
			entry->fp_type = PHAR_FP;
		}
	}

	/* append signature */
	if (global_flags & PHAR_HDR_SIGNATURE) {
		unsigned char buf[1024];
		int  sig_flags = 0, sig_len;
		char sig_buf[4];

		php_stream_rewind(newfile);
		
		if (phar->signature) {
			efree(phar->signature);
		}
		
		switch(phar->sig_flags) {
#if HAVE_HASH_EXT
		case PHAR_SIG_SHA512: {
			unsigned char digest[64];
			PHP_SHA512_CTX  context;

			PHP_SHA512Init(&context);
			while ((sig_len = php_stream_read(newfile, (char*)buf, sizeof(buf))) > 0) {
				PHP_SHA512Update(&context, buf, sig_len);
			}
			PHP_SHA512Final(digest, &context);
			php_stream_write(newfile, (char *) digest, sizeof(digest));
			sig_flags |= PHAR_SIG_SHA512;
			phar->sig_len = phar_hex_str((const char*)digest, sizeof(digest), &phar->signature);
			break;
		}
		case PHAR_SIG_SHA256: {
			unsigned char digest[32];
			PHP_SHA256_CTX  context;

			PHP_SHA256Init(&context);
			while ((sig_len = php_stream_read(newfile, (char*)buf, sizeof(buf))) > 0) {
				PHP_SHA256Update(&context, buf, sig_len);
			}
			PHP_SHA256Final(digest, &context);
			php_stream_write(newfile, (char *) digest, sizeof(digest));
			sig_flags |= PHAR_SIG_SHA256;
			phar->sig_len = phar_hex_str((const char*)digest, sizeof(digest), &phar->signature);
			break;
		}
#else
		case PHAR_SIG_SHA512:
		case PHAR_SIG_SHA256:
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				spprintf(error, 0, "unable to write contents of file \"%s\" to new phar \"%s\" with requested hash type", entry->filename, phar->fname);
			}
			return EOF;
#endif
		case PHAR_SIG_PGP:
			/* TODO: currently fall back to sha1,later do both */
		default:
		case PHAR_SIG_SHA1: {
			unsigned char digest[20];
			PHP_SHA1_CTX  context;

			PHP_SHA1Init(&context);
			while ((sig_len = php_stream_read(newfile, (char*)buf, sizeof(buf))) > 0) {
				PHP_SHA1Update(&context, buf, sig_len);
			}
			PHP_SHA1Final(digest, &context);
			php_stream_write(newfile, (char *) digest, sizeof(digest));
			sig_flags |= PHAR_SIG_SHA1;
			phar->sig_len = phar_hex_str((const char*)digest, sizeof(digest), &phar->signature);
			break;
		}
		case PHAR_SIG_MD5: {
			unsigned char digest[16];
			PHP_MD5_CTX   context;

			PHP_MD5Init(&context);
			while ((sig_len = php_stream_read(newfile, (char*)buf, sizeof(buf))) > 0) {
				PHP_MD5Update(&context, buf, sig_len);
			}
			PHP_MD5Final(digest, &context);
			php_stream_write(newfile, (char *) digest, sizeof(digest));
			sig_flags |= PHAR_SIG_MD5;
			phar->sig_len = phar_hex_str((const char*)digest, sizeof(digest), &phar->signature);
			break;
		}
		}
		phar_set_32(sig_buf, sig_flags);
		php_stream_write(newfile, sig_buf, 4);
		php_stream_write(newfile, "GBMB", 4);
		phar->sig_flags = sig_flags;
	}

	/* finally, close the temp file, rename the original phar,
	   move the temp to the old phar, unlink the old phar, and reload it into memory
	*/
	if (phar->fp) {
		php_stream_close(phar->fp);
	}
	if (phar->ufp) {
		php_stream_close(phar->ufp);
		phar->ufp = NULL;
	}
	if (closeoldfile) {
		php_stream_close(oldfile);
	}

	phar->internal_file_start = halt_offset + manifest_len + 4;
	phar->halt_offset = halt_offset;
	phar->is_brandnew = 0;

	php_stream_rewind(newfile);

	if (phar->donotflush) {
		/* deferred flush */
		phar->fp = newfile;
	} else {
		phar->fp = php_stream_open_wrapper(phar->fname, "w+b", IGNORE_URL|STREAM_MUST_SEEK|REPORT_ERRORS, NULL);
		if (!phar->fp) {
			phar->fp = newfile;
			if (error) {
				spprintf(error, 4096, "unable to open new phar \"%s\" for writing", phar->fname);
			}
			return EOF;
		}
		if (phar->flags & PHAR_FILE_COMPRESSED_GZ) {
			php_stream_filter *filter;
			/* to properly compress, we have to tell zlib to add a zlib header */
			zval filterparams;

			array_init(&filterparams);
			add_assoc_long(&filterparams, "window", MAX_WBITS+16);
			filter = php_stream_filter_create("zlib.deflate", &filterparams, php_stream_is_persistent(phar->fp) TSRMLS_CC);
			zval_dtor(&filterparams);
			if (!filter) {
				if (error) {
					spprintf(error, 4096, "unable to compress all contents of phar \"%s\" using zlib, PHP versions older than 5.2.6 have a buggy zlib", phar->fname);
				}
				return EOF;
			}
			php_stream_filter_append(&phar->fp->writefilters, filter);
			php_stream_copy_to_stream(newfile, phar->fp, PHP_STREAM_COPY_ALL);
			php_stream_filter_flush(filter, 1);
			php_stream_filter_remove(filter, 1 TSRMLS_CC);
			php_stream_close(phar->fp);
			/* use the temp stream as our base */
			phar->fp = newfile;
		} else if (phar->flags & PHAR_FILE_COMPRESSED_BZ2) {
			php_stream_filter *filter;

			filter = php_stream_filter_create("bzip2.compress", NULL, php_stream_is_persistent(phar->fp) TSRMLS_CC);
			php_stream_filter_append(&phar->fp->writefilters, filter);
			php_stream_copy_to_stream(newfile, phar->fp, PHP_STREAM_COPY_ALL);
			php_stream_filter_flush(filter, 1);
			php_stream_filter_remove(filter, 1 TSRMLS_CC);
			php_stream_close(phar->fp);
			/* use the temp stream as our base */
			phar->fp = newfile;
		} else {
			php_stream_copy_to_stream(newfile, phar->fp, PHP_STREAM_COPY_ALL);
			/* we could also reopen the file in "rb" mode but there is no need for that */
			php_stream_close(newfile);
		}
	}

	if (-1 == php_stream_seek(phar->fp, phar->halt_offset, SEEK_SET)) {
		if (error) {
			spprintf(error, 0, "unable to seek to __HALT_COMPILER(); in new phar \"%s\"", phar->fname);
		}
		return EOF;
	}

	return EOF;
}
/* }}} */

#ifdef COMPILE_DL_PHAR
ZEND_GET_MODULE(phar)
#endif

/* {{{ phar_functions[]
 *
 * Every user visible function must have an entry in phar_functions[].
 */
function_entry phar_functions[] = {
       {NULL, NULL, NULL}      /* Must be the last line in phar_functions[] */
};
/* }}}*/

/* {{{ php_phar_init_globals
 */
static void php_phar_init_globals_module(zend_phar_globals *phar_globals)
{
	memset(phar_globals, 0, sizeof(zend_phar_globals));
	phar_globals->readonly = 1;
}
/* }}} */

#if PHP_VERSION_ID >= 50300
static size_t phar_zend_stream_reader(void *handle, char *buf, size_t len TSRMLS_DC) /* {{{ */
{
	return php_stream_read(((phar_archive_data*)handle)->fp, buf, len);
}
/* }}} */

static size_t phar_zend_stream_fsizer(void *handle TSRMLS_DC) /* {{{ */
{
	return ((phar_archive_data*)handle)->halt_offset + 32;
} /* }}} */

#else /* PHP_VERSION_ID */

static long stream_fteller_for_zend(void *handle TSRMLS_DC) /* {{{ */
{
	return (long)php_stream_tell((php_stream*)handle);
}
/* }}} */
#endif

zend_op_array *(*phar_orig_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);
#if PHP_VERSION_ID >= 50300
#define phar_orig_zend_open zend_stream_open_function
char *phar_resolve_path(const char *filename, int filename_len TSRMLS_DC)
{
	return phar_find_in_include_path((char *) filename, filename_len, NULL TSRMLS_CC);
}
#else
int (*phar_orig_zend_open)(const char *filename, zend_file_handle *handle TSRMLS_DC);
#endif

static zend_op_array *phar_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) /* {{{ */
{
	zend_op_array *res;
	char *name = NULL;
	int failed;
	phar_archive_data *phar;

	if (strstr(file_handle->filename, ".phar") && !strstr(file_handle->filename, "://")) {
		if (SUCCESS == phar_open_filename(file_handle->filename, strlen(file_handle->filename), NULL, 0, 0, &phar, NULL TSRMLS_CC)) {
			if (phar->is_zip || phar->is_tar) {
				zend_file_handle f = *file_handle;

				/* zip or tar-based phar */
				spprintf(&name, 4096, "phar://%s/%s", file_handle->filename, ".phar/stub.php");
				if (SUCCESS == phar_orig_zend_open((const char *)name, file_handle TSRMLS_CC)) {
					efree(name);
					name = NULL;
					file_handle->filename = f.filename;
					if (file_handle->opened_path) {
						efree(file_handle->opened_path);
					}
					file_handle->opened_path = f.opened_path;
					file_handle->free_filename = f.free_filename;
				} else {
					*file_handle = f;
				}
			} else if (phar->flags & PHAR_FILE_COMPRESSION_MASK) {
				/* compressed phar */
#if PHP_VERSION_ID >= 50300 && PHP_VERSION_ID < 60000
				file_handle->type = ZEND_HANDLE_STREAM;
				file_handle->free_filename = 0;
				file_handle->handle.stream.handle  = phar;
				file_handle->handle.stream.reader  = phar_zend_stream_reader;
				file_handle->handle.stream.closer  = NULL;
				file_handle->handle.stream.fsizer  = phar_zend_stream_fsizer;
				file_handle->handle.stream.isatty  = 0;
				php_stream_rewind(phar->fp);
				memset(&file_handle->handle.stream.mmap, 0, sizeof(file_handle->handle.stream.mmap));
#else /* PHP_VERSION_ID */
				file_handle->type = ZEND_HANDLE_STREAM;
				file_handle->free_filename = 0;
				file_handle->handle.stream.handle = phar->fp;
				file_handle->handle.stream.reader = (zend_stream_reader_t)_php_stream_read;
				file_handle->handle.stream.closer = NULL; /* don't close - let phar handle this one */
				file_handle->handle.stream.fteller = stream_fteller_for_zend;
				file_handle->handle.stream.interactive = 0;
				php_stream_rewind(phar->fp);
#endif
			}
		}
	}
	zend_try {
		failed = 0;
		res = phar_orig_compile_file(file_handle, type TSRMLS_CC);
	} zend_catch {
		failed = 1;
	} zend_end_try();
	if (name) {
		efree(name);
	}
	if (failed) {
		zend_bailout();
	}
	return res;
}
/* }}} */

#if PHP_VERSION_ID < 50300
int phar_zend_open(const char *filename, zend_file_handle *handle TSRMLS_DC) /* {{{ */
{
	char *arch, *entry;
	int arch_len, entry_len;

	/* this code is obsoleted in php 5.3 */
	entry = (char *) filename;
	if (!IS_ABSOLUTE_PATH(entry, strlen(entry)) && !strstr(entry, "://")) {
		phar_archive_data **pphar = NULL;
		char *fname;
		int fname_len;

		fname = zend_get_executed_filename(TSRMLS_C);
		fname_len = strlen(fname);
		if (fname_len > 7 && !strncasecmp(fname, "phar://", 7)) {
			if (SUCCESS == phar_split_fname(fname, fname_len, &arch, &arch_len, &entry, &entry_len, 1, 0 TSRMLS_CC)) {
				zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), arch, arch_len, (void **) &pphar);
				efree(arch);
				efree(entry);
			}
		}
		/* retrieving an include within the current directory, so use this if possible */
		if (!(entry = phar_find_in_include_path((char *) filename, strlen(filename), NULL TSRMLS_CC))) {
			/* this file is not in the phar, use the original path */
			goto skip_phar;
		}
		if (SUCCESS == phar_orig_zend_open(entry, handle TSRMLS_CC)) {
			if (!handle->opened_path) {
				handle->opened_path = entry;
			}
			if (entry != filename) {
				handle->free_filename = 1;
			}
			return SUCCESS;
		}
		if (entry != filename) {
			efree(entry);
		}
		return FAILURE;
	}
skip_phar:
	return phar_orig_zend_open(filename, handle TSRMLS_CC);
}
/* }}} */
#endif
typedef zend_op_array* (zend_compile_t)(zend_file_handle*, int TSRMLS_DC);
typedef zend_compile_t* (compile_hook)(zend_compile_t *ptr);

PHP_MINIT_FUNCTION(phar) /* {{{ */
{
	ZEND_INIT_MODULE_GLOBALS(phar, php_phar_init_globals_module, NULL);
	REGISTER_INI_ENTRIES();

	phar_has_bz2 = zend_hash_exists(&module_registry, "bz2", sizeof("bz2"));
	phar_has_zlib = zend_hash_exists(&module_registry, "zlib", sizeof("zlib"));
	if (zend_hash_exists(&module_registry, "apc", sizeof("apc"))) {
		zval magic;
		if (zend_get_constant("\000apc_magic", sizeof("\000apc_magic"), &magic TSRMLS_CC)) {
			compile_hook *set_compile_hook;

			set_compile_hook = (compile_hook *) Z_LVAL(magic);
			phar_orig_compile_file = set_compile_hook(phar_compile_file);
		} else {
			phar_orig_compile_file = zend_compile_file;
			zend_compile_file = phar_compile_file;
		}
	} else {
		phar_orig_compile_file = zend_compile_file;
		zend_compile_file = phar_compile_file;
	}

#if PHP_VERSION_ID >= 50300
	phar_save_resolve_path = zend_resolve_path;
	zend_resolve_path = phar_resolve_path;
#else
	phar_orig_zend_open = zend_stream_open_function;
	zend_stream_open_function = phar_zend_open;
#endif

	phar_object_init(TSRMLS_C);

	return php_register_url_stream_wrapper("phar", &php_stream_phar_wrapper TSRMLS_CC);
}
/* }}} */

PHP_MSHUTDOWN_FUNCTION(phar) /* {{{ */
{
	return php_unregister_url_stream_wrapper("phar" TSRMLS_CC);
	if (zend_compile_file == phar_compile_file) {
		zend_compile_file = phar_orig_compile_file;
	} else if (zend_hash_exists(&module_registry, "apc", sizeof("apc"))) {
		zval magic;
		if (zend_get_constant("\000apc_magic", sizeof("\000apc_magic")-1, &magic TSRMLS_CC)) {
			compile_hook *set_compile_hook;

			set_compile_hook = (compile_hook *) Z_LVAL(magic);
			set_compile_hook(NULL);
		}
	}

#if PHP_VERSION_ID < 50300
	if (zend_stream_open_function == phar_zend_open) {
		zend_stream_open_function = phar_orig_zend_open;
	}
#endif
}
/* }}} */

void phar_request_initialize(TSRMLS_D) /* {{{ */
{
	if (!PHAR_GLOBALS->request_init)
	{
		PHAR_GLOBALS->request_init = 1;
		PHAR_GLOBALS->request_ends = 0;
		PHAR_GLOBALS->request_done = 0;
		zend_hash_init(&(PHAR_GLOBALS->phar_fname_map), sizeof(phar_archive_data*), zend_get_hash_value, destroy_phar_data,  0);
		zend_hash_init(&(PHAR_GLOBALS->phar_alias_map), sizeof(phar_archive_data*), zend_get_hash_value, NULL, 0);
		zend_hash_init(&(PHAR_GLOBALS->phar_plain_map), sizeof(const char *),       zend_get_hash_value, NULL, 0);
		zend_hash_init(&(PHAR_GLOBALS->phar_SERVER_mung_list), sizeof(const char *),       zend_get_hash_value, NULL, 0);
		phar_split_extract_list(TSRMLS_C);
		PHAR_G(cwd) = NULL;
		PHAR_G(cwd_len) = 0;
		PHAR_G(cwd_init) = 0;
		phar_intercept_functions(TSRMLS_C);
	}
}
/* }}} */

PHP_RSHUTDOWN_FUNCTION(phar) /* {{{ */
{
	PHAR_GLOBALS->request_ends = 1;
	if (PHAR_GLOBALS->request_init)
	{
		phar_release_functions(TSRMLS_C);
		zend_hash_destroy(&(PHAR_GLOBALS->phar_alias_map));
		PHAR_GLOBALS->phar_alias_map.arBuckets = NULL;
		zend_hash_destroy(&(PHAR_GLOBALS->phar_fname_map));
		PHAR_GLOBALS->phar_fname_map.arBuckets = NULL;
		zend_hash_destroy(&(PHAR_GLOBALS->phar_plain_map));
		PHAR_GLOBALS->phar_plain_map.arBuckets = NULL;
		zend_hash_destroy(&(PHAR_GLOBALS->phar_SERVER_mung_list));
		PHAR_GLOBALS->phar_SERVER_mung_list.arBuckets = NULL;
		PHAR_GLOBALS->request_init = 0;
		if (PHAR_G(cwd)) {
			efree(PHAR_G(cwd));
		}
		PHAR_G(cwd) = NULL;
		PHAR_G(cwd_len) = 0;
		PHAR_G(cwd_init) = 0;
	}
	PHAR_GLOBALS->request_done = 1;
	return SUCCESS;
}
/* }}} */

PHP_MINFO_FUNCTION(phar) /* {{{ */
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Phar: PHP Archive support", "enabled");
	php_info_print_table_row(2, "Phar EXT version", PHP_PHAR_VERSION);
	php_info_print_table_row(2, "Phar API version", PHP_PHAR_API_VERSION);
	php_info_print_table_row(2, "CVS revision", "$Revision$");
	php_info_print_table_row(2, "Phar-based phar archives", "enabled");
	php_info_print_table_row(2, "Tar-based phar archives", "enabled");
	php_info_print_table_row(2, "ZIP-based phar archives", "enabled");
	if (phar_has_zlib) {
		php_info_print_table_row(2, "gzip compression", "enabled");
	} else {
		php_info_print_table_row(2, "gzip compression", "disabled (install ext/zlib)");
	}
	if (phar_has_bz2) {
		php_info_print_table_row(2, "bzip2 compression", "enabled");
	} else {
		php_info_print_table_row(2, "bzip2 compression", "disabled (install pecl/bz2)");
	}
	php_info_print_table_end();

	php_info_print_box_start(0);
	PUTS("Phar based on pear/PHP_Archive, original concept by Davey Shafik.");
	PUTS(!sapi_module.phpinfo_as_text?"<br />":"\n");	
	PUTS("Phar fully realized by Gregory Beaver and Marcus Boerger.");
	PUTS(!sapi_module.phpinfo_as_text?"<br />":"\n");	
	PUTS("Portions of tar implementation Copyright (c) 2003-2007 Tim Kientzle.");
	php_info_print_box_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ phar_module_entry
 */
static zend_module_dep phar_deps[] = {
	ZEND_MOD_OPTIONAL("apc")
	ZEND_MOD_OPTIONAL("zlib")
	ZEND_MOD_OPTIONAL("bz2")
#if HAVE_SPL
	ZEND_MOD_REQUIRED("spl")
#endif
	{NULL, NULL, NULL}
};

zend_module_entry phar_module_entry = {
	STANDARD_MODULE_HEADER_EX, NULL,
	phar_deps,
	"Phar",
	phar_functions,
	PHP_MINIT(phar),
	PHP_MSHUTDOWN(phar),
	NULL,
	PHP_RSHUTDOWN(phar),
	PHP_MINFO(phar),
	PHP_PHAR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
