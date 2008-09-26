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

static void destroy_phar_data(void *pDest);

ZEND_DECLARE_MODULE_GLOBALS(phar)
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

/* if the original value is 0 (disabled), then allow setting/unsetting at will. Otherwise only allow 1 (enabled), and error on disabling */
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

/* this global stores the global cached pre-parsed manifests */
HashTable cached_phars;
HashTable cached_alias;

static void phar_split_cache_list(TSRMLS_D) /* {{{ */
{
	char *tmp;
	char *key, *lasts, *end;
	char ds[2];
	phar_archive_data *phar;
	uint i = 0;

	if (!PHAR_GLOBALS->cache_list || !(PHAR_GLOBALS->cache_list[0])) {
		return;
	}

	ds[0] = DEFAULT_DIR_SEPARATOR;
	ds[1] = '\0';
	tmp = estrdup(PHAR_GLOBALS->cache_list);

	/* fake request startup */
	PHAR_GLOBALS->request_init = 1;
	if (zend_hash_init(&EG(regular_list), 0, NULL, NULL, 0) == SUCCESS) {
		EG(regular_list).nNextFreeElement=1;	/* we don't want resource id 0 */
	}

	PHAR_G(has_bz2) = zend_hash_exists(&module_registry, "bz2", sizeof("bz2"));
	PHAR_G(has_zlib) = zend_hash_exists(&module_registry, "zlib", sizeof("zlib"));
	/* these two are dummies and will be destroyed later */
	zend_hash_init(&cached_phars, sizeof(phar_archive_data*), zend_get_hash_value, destroy_phar_data,  1);
	zend_hash_init(&cached_alias, sizeof(phar_archive_data*), zend_get_hash_value, NULL, 1);
	/* these two are real and will be copied over cached_phars/cached_alias later */
	zend_hash_init(&(PHAR_GLOBALS->phar_fname_map), sizeof(phar_archive_data*), zend_get_hash_value, destroy_phar_data,  1);
	zend_hash_init(&(PHAR_GLOBALS->phar_alias_map), sizeof(phar_archive_data*), zend_get_hash_value, NULL, 1);
	PHAR_GLOBALS->manifest_cached = 1;
	PHAR_GLOBALS->persist = 1;

	for (key = php_strtok_r(tmp, ds, &lasts);
			key;
			key = php_strtok_r(NULL, ds, &lasts)) {
		end = strchr(key, DEFAULT_DIR_SEPARATOR);

		if (end) {
			if (SUCCESS == phar_open_from_filename(key, end - key, NULL, 0, 0, &phar, NULL TSRMLS_CC)) {
finish_up:
				phar->phar_pos = i++;
				php_stream_close(phar->fp);
				phar->fp = NULL;
			} else {
finish_error:
				PHAR_GLOBALS->persist = 0;
				PHAR_GLOBALS->manifest_cached = 0;
				efree(tmp);
				zend_hash_destroy(&(PHAR_G(phar_fname_map)));
				PHAR_GLOBALS->phar_fname_map.arBuckets = 0;
				zend_hash_destroy(&(PHAR_G(phar_alias_map)));
				PHAR_GLOBALS->phar_alias_map.arBuckets = 0;
				zend_hash_destroy(&cached_phars);
				zend_hash_destroy(&cached_alias);
				zend_hash_graceful_reverse_destroy(&EG(regular_list));
				memset(&EG(regular_list), 0, sizeof(HashTable));
				/* free cached manifests */
				PHAR_GLOBALS->request_init = 0;
				return;
			}
		} else {
			if (SUCCESS == phar_open_from_filename(key, strlen(key), NULL, 0, 0, &phar, NULL TSRMLS_CC)) {
				goto finish_up;
			} else {
				goto finish_error;
			}
		}
	}

	PHAR_GLOBALS->persist = 0;
	PHAR_GLOBALS->request_init = 0;
	/* destroy dummy values from before */
	zend_hash_destroy(&cached_phars);
	zend_hash_destroy(&cached_alias);
	cached_phars = PHAR_GLOBALS->phar_fname_map;
	cached_alias = PHAR_GLOBALS->phar_alias_map;
	PHAR_GLOBALS->phar_fname_map.arBuckets = 0;
	PHAR_GLOBALS->phar_alias_map.arBuckets = 0;
	zend_hash_graceful_reverse_destroy(&EG(regular_list));
	memset(&EG(regular_list), 0, sizeof(HashTable));
	efree(tmp);
}
/* }}} */

ZEND_INI_MH(phar_ini_cache_list) /* {{{ */
{
	PHAR_G(cache_list) = new_value;

	if (stage == ZEND_INI_STAGE_STARTUP) {
		phar_split_cache_list(TSRMLS_C);
	}

	return SUCCESS;
}
/* }}} */

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN( "phar.readonly", "1", PHP_INI_ALL, phar_ini_modify_handler, readonly, zend_phar_globals, phar_globals)
	STD_PHP_INI_BOOLEAN( "phar.require_hash", "1", PHP_INI_ALL, phar_ini_modify_handler, require_hash, zend_phar_globals, phar_globals)
	STD_PHP_INI_ENTRY("phar.cache_list", "", PHP_INI_SYSTEM, phar_ini_cache_list, cache_list, zend_phar_globals, phar_globals)
PHP_INI_END()

/**
 * When all uses of a phar have been concluded, this frees the manifest
 * and the phar slot
 */
void phar_destroy_phar_data(phar_archive_data *phar TSRMLS_DC) /* {{{ */
{
	if (phar->alias && phar->alias != phar->fname) {
		pefree(phar->alias, phar->is_persistent);
		phar->alias = NULL;
	}

	if (phar->fname) {
		pefree(phar->fname, phar->is_persistent);
		phar->fname = NULL;
	}

	if (phar->signature) {
		pefree(phar->signature, phar->is_persistent);
		phar->signature = NULL;
	}

	if (phar->manifest.arBuckets) {
		zend_hash_destroy(&phar->manifest);
		phar->manifest.arBuckets = NULL;
	}

	if (phar->mounted_dirs.arBuckets) {
		zend_hash_destroy(&phar->mounted_dirs);
		phar->mounted_dirs.arBuckets = NULL;
	}

	if (phar->virtual_dirs.arBuckets) {
		zend_hash_destroy(&phar->virtual_dirs);
		phar->virtual_dirs.arBuckets = NULL;
	}

	if (phar->metadata) {
		if (phar->is_persistent) {
			if (phar->metadata_len) {
				/* for zip comments that are strings */
				free(phar->metadata);
			} else {
				zval_internal_ptr_dtor(&phar->metadata);
			}
		} else {
			zval_ptr_dtor(&phar->metadata);
		}
		phar->metadata_len = 0;
		phar->metadata = 0;
	}

	if (phar->fp) {
		php_stream_close(phar->fp);
		phar->fp = 0;
	}

	if (phar->ufp) {
		php_stream_close(phar->ufp);
		phar->ufp = 0;
	}

	pefree(phar, phar->is_persistent);
}
/* }}}*/

/**
 * Delete refcount and destruct if needed. On destruct return 1 else 0.
 */
int phar_archive_delref(phar_archive_data *phar TSRMLS_DC) /* {{{ */
{
	if (phar->is_persistent) {
		return 0;
	}

	if (--phar->refcount < 0) {
		if (PHAR_GLOBALS->request_done
		|| zend_hash_del(&(PHAR_GLOBALS->phar_fname_map), phar->fname, phar->fname_len) != SUCCESS) {
			phar_destroy_phar_data(phar TSRMLS_CC);
		}
		return 1;
	} else if (!phar->refcount) {
		/* invalidate phar cache */
		PHAR_G(last_phar) = NULL;
		PHAR_G(last_phar_name) = PHAR_G(last_alias) = NULL;

		if (phar->fp && !(phar->flags & PHAR_FILE_COMPRESSION_MASK)) {
			/* close open file handle - allows removal or rename of
			the file on windows, which has greedy locking
			only close if the archive was not already compressed.  If it
			was compressed, then the fp does not refer to the original file */
			php_stream_close(phar->fp);
			phar->fp = NULL;
		}

		if (!zend_hash_num_elements(&phar->manifest)) {
			/* this is a new phar that has perhaps had an alias/metadata set, but has never
			been flushed */
			if (zend_hash_del(&(PHAR_GLOBALS->phar_fname_map), phar->fname, phar->fname_len) != SUCCESS) {
				phar_destroy_phar_data(phar TSRMLS_CC);
			}
			return 1;
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
 * Delete aliases to phar's that got kicked out of the global table
 */
static int phar_tmpclose_apply(void *pDest TSRMLS_DC) /* {{{ */
{
	phar_entry_info *entry = (phar_entry_info *) pDest;

	if (entry->fp_type != PHAR_TMP) {
		return ZEND_HASH_APPLY_KEEP;
	}

	if (entry->fp && !entry->fp_refcount) {
		php_stream_close(entry->fp);
		entry->fp = NULL;
	}

	return ZEND_HASH_APPLY_KEEP;
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
		/* first, iterate over the manifest and close all PHAR_TMP entry fp handles,
		this prevents unnecessary unfreed stream resources */
		zend_hash_apply(&(phar_data->manifest), phar_tmpclose_apply TSRMLS_CC);
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
		if (entry->is_persistent) {
			if (entry->metadata_len) {
				/* for zip comments that are strings */
				free(entry->metadata);
			} else {
				zval_internal_ptr_dtor(&entry->metadata);
			}
		} else {
			zval_ptr_dtor(&entry->metadata);
		}
		entry->metadata_len = 0;
		entry->metadata = 0;
	}

	if (entry->metadata_str.c) {
		smart_str_free(&entry->metadata_str);
		entry->metadata_str.c = 0;
	}

	pefree(entry->filename, entry->is_persistent);

	if (entry->link) {
		pefree(entry->link, entry->is_persistent);
		entry->link = 0;
	}

	if (entry->tmp) {
		pefree(entry->tmp, entry->is_persistent);
		entry->tmp = 0;
	}
}
/* }}} */

int phar_entry_delref(phar_entry_data *idata TSRMLS_DC) /* {{{ */
{
	int ret = 0;

	if (idata->internal_file && !idata->internal_file->is_persistent) {
		if (--idata->internal_file->fp_refcount < 0) {
			idata->internal_file->fp_refcount = 0;
		}

		if (idata->fp && idata->fp != idata->phar->fp && idata->fp != idata->phar->ufp && idata->fp != idata->internal_file->fp) {
			php_stream_close(idata->fp);
		}
		/* if phar_get_or_create_entry_data returns a sub-directory, we have to free it */
		if (idata->internal_file->is_temp_dir) {
			destroy_phar_manifest_entry((void *)idata->internal_file);
			efree(idata->internal_file);
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
	if (fp) {\
		php_stream_close(fp);\
	}\
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
		pefree(signature, PHAR_G(persist));\
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
int phar_open_parsed_phar(char *fname, int fname_len, char *alias, int alias_len, int is_data, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
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
			if (!phar->halt_offset && !phar->is_brandnew && (phar->is_tar || phar->is_zip)) {
				if (PHAR_G(readonly) && FAILURE == zend_hash_find(&(phar->manifest), ".phar/stub.php", sizeof(".phar/stub.php")-1, (void **)&stub)) {
					if (error) {
						spprintf(error, 0, "'%s' is not a phar archive. Use PharData::__construct() for a standard zip or tar archive", fname);
					}
					return FAILURE;
				}
			}
		}

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

		if (phar && error && !(options & REPORT_ERRORS)) {
			efree(error);
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
		ALLOC_ZVAL(*metadata);
		INIT_ZVAL(**metadata);
		p = (const unsigned char*) *buffer;
		PHP_VAR_UNSERIALIZE_INIT(var_hash);

		if (!php_var_unserialize(metadata, &p, p + buf_len, &var_hash TSRMLS_CC)) {
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
			zval_ptr_dtor(metadata);
			*metadata = NULL;
			return FAILURE;
		}

		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

		if (PHAR_G(persist)) {
			/* lazy init metadata */
			zval_ptr_dtor(metadata);
			*metadata = (zval *) pemalloc(buf_len, 1);
			memcpy(*metadata, *buffer, buf_len);
			if (!zip_metadata_len) {
				*buffer += buf_len;
			}
			return SUCCESS;
		}
	} else {
		*metadata = NULL;
	}

	if (!zip_metadata_len) {
		*buffer += buf_len;
	}

	return SUCCESS;
}
/* }}}*/

/**
 * Does not check for a previously opened phar in the cache.
 *
 * Parse a new one and add it to the cache, returning either SUCCESS or
 * FAILURE, and setting pphar to the pointer to the manifest entry
 * 
 * This is used by phar_open_from_filename to process the manifest, but can be called
 * directly.
 */
static int phar_parse_pharfile(php_stream *fp, char *fname, int fname_len, char *alias, int alias_len, long halt_offset, phar_archive_data** pphar, php_uint32 compression, char **error TSRMLS_DC) /* {{{ */
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
	manifest_ver = (((unsigned char)buffer[0]) << 8)
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
		char sig_buf[8], *sig_ptr = sig_buf;
		off_t read_len;
		size_t end_of_phar;

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
			case PHAR_SIG_OPENSSL: {
				php_uint32 signature_len;
				char *sig;
				off_t whence;

				/* we store the signature followed by the signature length */
				if (-1 == php_stream_seek(fp, -12, SEEK_CUR)
				|| 4 != php_stream_read(fp, sig_buf, 4)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" openssl signature length could not be read", fname);
					}
					return FAILURE;
				}

				sig_ptr = sig_buf;
				PHAR_GET_32(sig_ptr, signature_len);
				sig = (char *) emalloc(signature_len);
				whence = signature_len + 4;
				whence = -whence;

				if (-1 == php_stream_seek(fp, whence, SEEK_CUR)
				|| !(end_of_phar = php_stream_tell(fp))
				|| signature_len != php_stream_read(fp, sig, signature_len)) {
					efree(savebuf);
					efree(sig);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" openssl signature could not be read", fname);
					}
					return FAILURE;
				}

				if (FAILURE == phar_verify_signature(fp, end_of_phar, PHAR_SIG_OPENSSL, sig, signature_len, fname, &signature, &sig_len, error TSRMLS_CC)) {
					efree(savebuf);
					efree(sig);
					php_stream_close(fp);
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar \"%s\" openssl signature could not be verified: %s", fname, *error);
						efree(save);
					}
					return FAILURE;
				}
				efree(sig);
			}
			break;
#if HAVE_HASH_EXT
			case PHAR_SIG_SHA512: {
				unsigned char digest[64];

				php_stream_seek(fp, -(8 + 64), SEEK_END);
				read_len = php_stream_tell(fp);

				if (php_stream_read(fp, (char*)digest, sizeof(digest)) != sizeof(digest)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
					}
					return FAILURE;
				}

				if (FAILURE == phar_verify_signature(fp, read_len, PHAR_SIG_SHA512, (char *)digest, 64, fname, &signature, &sig_len, error TSRMLS_CC)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar \"%s\" SHA512 signature could not be verified: %s", fname, *error);
						efree(save);
					}
					return FAILURE;
				}
				break;
			}
			case PHAR_SIG_SHA256: {
				unsigned char digest[32];

				php_stream_seek(fp, -(8 + 32), SEEK_END);
				read_len = php_stream_tell(fp);

				if (php_stream_read(fp, (char*)digest, sizeof(digest)) != sizeof(digest)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
					}
					return FAILURE;
				}

				if (FAILURE == phar_verify_signature(fp, read_len, PHAR_SIG_SHA256, (char *)digest, 32, fname, &signature, &sig_len, error TSRMLS_CC)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar \"%s\" SHA256 signature could not be verified: %s", fname, *error);
						efree(save);
					}
					return FAILURE;
				}
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
				unsigned char digest[20];

				php_stream_seek(fp, -(8 + 20), SEEK_END);
				read_len = php_stream_tell(fp);

				if (php_stream_read(fp, (char*)digest, sizeof(digest)) != sizeof(digest)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
					}
					return FAILURE;
				}

				if (FAILURE == phar_verify_signature(fp, read_len, PHAR_SIG_SHA1, (char *)digest, 20, fname, &signature, &sig_len, error TSRMLS_CC)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar \"%s\" SHA1 signature could not be verified: %s", fname, *error);
						efree(save);
					}
					return FAILURE;
				}
				break;
			}
			case PHAR_SIG_MD5: {
				unsigned char digest[16];

				php_stream_seek(fp, -(8 + 16), SEEK_END);
				read_len = php_stream_tell(fp);

				if (php_stream_read(fp, (char*)digest, sizeof(digest)) != sizeof(digest)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						spprintf(error, 0, "phar \"%s\" has a broken signature", fname);
					}
					return FAILURE;
				}

				if (FAILURE == phar_verify_signature(fp, read_len, PHAR_SIG_MD5, (char *)digest, 16, fname, &signature, &sig_len, error TSRMLS_CC)) {
					efree(savebuf);
					php_stream_close(fp);
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar \"%s\" MD5 signature could not be verified: %s", fname, *error);
						efree(save);
					}
					return FAILURE;
				}
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
			php_stream_close(fp);

			if (signature) {
				efree(signature);
			}

			if (error) {
				spprintf(error, 0, "cannot load phar \"%s\" with implicit alias \"%s\" under different alias \"%s\"", fname, buffer, alias);
			}

			efree(savebuf);
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

	mydata = pecalloc(1, sizeof(phar_archive_data), PHAR_G(persist));
	mydata->is_persistent = PHAR_G(persist);

	/* check whether we have meta data, zero check works regardless of byte order */
	if (mydata->is_persistent) {
		char *mysave = buffer;
		PHAR_GET_32(buffer, mydata->metadata_len);
		buffer = mysave;
		if (phar_parse_metadata(&buffer, &mydata->metadata, mydata->metadata_len TSRMLS_CC) == FAILURE) {
			MAPPHAR_FAIL("unable to read phar metadata in .phar file \"%s\"");
		}
	} else {
		if (phar_parse_metadata(&buffer, &mydata->metadata, 0 TSRMLS_CC) == FAILURE) {
			MAPPHAR_FAIL("unable to read phar metadata in .phar file \"%s\"");
		}
	}

	/* set up our manifest */
	zend_hash_init(&mydata->manifest, manifest_count,
		zend_get_hash_value, destroy_phar_manifest_entry, (zend_bool)mydata->is_persistent);
	zend_hash_init(&mydata->mounted_dirs, 5,
		zend_get_hash_value, NULL, (zend_bool)mydata->is_persistent);
	zend_hash_init(&mydata->virtual_dirs, manifest_count * 2,
		zend_get_hash_value, NULL, (zend_bool)mydata->is_persistent);
	mydata->fname = pestrndup(fname, fname_len, mydata->is_persistent);
#ifdef PHP_WIN32
	phar_unixify_path_separators(mydata->fname, fname_len);
#endif
	mydata->fname_len = fname_len;
	offset = halt_offset + manifest_len + 4;
	memset(&entry, 0, sizeof(phar_entry_info));
	entry.phar = mydata;
	entry.fp_type = PHAR_FP;
	entry.is_persistent = mydata->is_persistent;

	for (manifest_index = 0; manifest_index < manifest_count; ++manifest_index) {
		if (buffer + 4 > endbuffer) {
			MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest entry)")
		}

		PHAR_GET_32(buffer, entry.filename_len);

		if (entry.filename_len == 0) {
			MAPPHAR_FAIL("zero-length filename encountered in phar \"%s\"");
		}

		if (entry.is_persistent) {
			entry.manifest_pos = manifest_index;
		}

		if (buffer + entry.filename_len + 20 > endbuffer) {
			MAPPHAR_FAIL("internal corruption of phar \"%s\" (truncated manifest entry)");
		}

		if ((manifest_ver & PHAR_API_VER_MASK) >= PHAR_API_MIN_DIR && buffer[entry.filename_len - 1] == '/') {
			entry.is_dir = 1;
		} else {
			entry.is_dir = 0;
		}

		phar_add_virtual_dirs(mydata, buffer, entry.filename_len TSRMLS_CC);
		entry.filename = pestrndup(buffer, entry.filename_len, entry.is_persistent);
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

		if (entry.is_persistent) {
			if (phar_parse_metadata(&buffer, &entry.metadata, 0 TSRMLS_CC) == FAILURE) {
				pefree(entry.filename, entry.is_persistent);
				MAPPHAR_FAIL("unable to read file metadata in .phar file \"%s\"");
			}
		} else {
			if (phar_parse_metadata(&buffer, &entry.metadata, 0 TSRMLS_CC) == FAILURE) {
				pefree(entry.filename, entry.is_persistent);
				MAPPHAR_FAIL("unable to read file metadata in .phar file \"%s\"");
			}
		}

		entry.offset = entry.offset_abs = offset;
		offset += entry.compressed_filesize;

		switch (entry.flags & PHAR_ENT_COMPRESSION_MASK) {
			case PHAR_ENT_COMPRESSED_GZ:
				if (!PHAR_G(has_zlib)) {
					if (entry.metadata) {
						if (entry.is_persistent) {
							free(entry.metadata);
						} else {
							zval_ptr_dtor(&entry.metadata);
						}
					}
					pefree(entry.filename, entry.is_persistent);
					MAPPHAR_FAIL("zlib extension is required for gz compressed .phar file \"%s\"");
				}
				break;
			case PHAR_ENT_COMPRESSED_BZ2:
				if (!PHAR_G(has_bz2)) {
					if (entry.metadata) {
						if (entry.is_persistent) {
							free(entry.metadata);
						} else {
							zval_ptr_dtor(&entry.metadata);
						}
					}
					pefree(entry.filename, entry.is_persistent);
					MAPPHAR_FAIL("bz2 extension is required for bzip2 compressed .phar file \"%s\"");
				}
				break;
			default:
				if (entry.uncompressed_filesize != entry.compressed_filesize) {
					if (entry.metadata) {
						if (entry.is_persistent) {
							free(entry.metadata);
						} else {
							zval_ptr_dtor(&entry.metadata);
						}
					}
					pefree(entry.filename, entry.is_persistent);
					MAPPHAR_FAIL("internal corruption of phar \"%s\" (compressed and uncompressed size does not match for uncompressed entry)");
				}
				break;
		}

		manifest_flags |= (entry.flags & PHAR_ENT_COMPRESSION_MASK);
		/* if signature matched, no need to check CRC32 for each file */
		entry.is_crc_checked = (manifest_flags & PHAR_HDR_SIGNATURE ? 1 : 0);
		phar_set_inode(&entry TSRMLS_CC);
		zend_hash_add(&mydata->manifest, entry.filename, entry.filename_len, (void*)&entry, sizeof(phar_entry_info), NULL);
	}

	snprintf(mydata->version, sizeof(mydata->version), "%u.%u.%u", manifest_ver >> 12, (manifest_ver >> 8) & 0xF, (manifest_ver >> 4) & 0xF);
	mydata->internal_file_start = halt_offset + manifest_len + 4;
	mydata->halt_offset = halt_offset;
	mydata->flags = manifest_flags;
	endbuffer = strrchr(mydata->fname, '/');

	if (endbuffer) {
		mydata->ext = memchr(endbuffer, '.', (mydata->fname + fname_len) - endbuffer);
		if (mydata->ext == endbuffer) {
			mydata->ext = memchr(endbuffer + 1, '.', (mydata->fname + fname_len) - endbuffer - 1);
		}
		if (mydata->ext) {
			mydata->ext_len = (mydata->fname + mydata->fname_len) - mydata->ext;
		}
	}

	mydata->alias = alias ?
		pestrndup(alias, alias_len, mydata->is_persistent) :
		pestrndup(mydata->fname, fname_len, mydata->is_persistent);
	mydata->alias_len = alias ? alias_len : fname_len;
	mydata->sig_flags = sig_flags;
	mydata->fp = fp;
	mydata->sig_len = sig_len;
	mydata->signature = signature;
	phar_request_initialize(TSRMLS_C);

	if (register_alias) {
		phar_archive_data **fd_ptr;

		mydata->is_temporary_alias = temp_alias;

		if (!phar_validate_alias(mydata->alias, mydata->alias_len)) {
			signature = NULL;
			fp = NULL;
			MAPPHAR_FAIL("Cannot open archive \"%s\", invalid alias");
		}

		if (SUCCESS == zend_hash_find(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void **)&fd_ptr)) {
			if (SUCCESS != phar_free_alias(*fd_ptr, alias, alias_len TSRMLS_CC)) {
				signature = NULL;
				fp = NULL;
				MAPPHAR_FAIL("Cannot open archive \"%s\", alias is already in use by existing archive");
			}
		}

		zend_hash_add(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void*)&mydata, sizeof(phar_archive_data*), NULL);
	} else {
		mydata->is_temporary_alias = 1;
	}

	zend_hash_add(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len, (void*)&mydata, sizeof(phar_archive_data*),  NULL);
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
	char *my_error;
	int ext_len;
	phar_archive_data **test, *unused = NULL;

	test = &unused;

	if (error) {
		*error = NULL;
	}

	/* first try to open an existing file */
	if (phar_detect_phar_fname_ext(fname, fname_len, &ext_str, &ext_len, !is_data, 0, 1 TSRMLS_CC) == SUCCESS) {
		goto check_file;
	}

	/* next try to create a new file */
	if (FAILURE == phar_detect_phar_fname_ext(fname, fname_len, &ext_str, &ext_len, !is_data, 1, 1 TSRMLS_CC)) {
		if (error) {
			spprintf(error, 0, "Cannot create phar '%s', file extension (or combination) not recognised", fname);
		}
		return FAILURE;
	}
check_file:
	if (phar_open_parsed_phar(fname, fname_len, alias, alias_len, is_data, options, test, &my_error TSRMLS_CC) == SUCCESS) {
		if (pphar) {
			*pphar = *test;
		}

		if ((*test)->is_data && !(*test)->is_tar && !(*test)->is_zip) {
			if (error) {
				spprintf(error, 0, "Cannot open '%s' as a PharData object. Use Phar::__construct() for executable archives", fname);
			}
			return FAILURE;
		}

		if (PHAR_G(readonly) && !(*test)->is_data && ((*test)->is_tar || (*test)->is_zip)) {
			phar_entry_info *stub;
			if (FAILURE == zend_hash_find(&((*test)->manifest), ".phar/stub.php", sizeof(".phar/stub.php")-1, (void **)&stub)) {
				spprintf(error, 0, "'%s' is not a phar archive. Use PharData::__construct() for a standard zip or tar archive", fname);
				return FAILURE;
			}
		}

		if (!PHAR_G(readonly) || (*test)->is_data) {
			(*test)->is_writeable = 1;
		}
		return SUCCESS;
	} else if (my_error) {
		if (error) {
			*error = my_error;
		} else {
			efree(my_error);
		}
		return FAILURE;
	}

	if (ext_len > 3 && (z = memchr(ext_str, 'z', ext_len)) && ((ext_str + ext_len) - z >= 2) && !memcmp(z + 1, "ip", 2)) {
		/* assume zip-based phar */
		return phar_open_or_create_zip(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);
	}

	if (ext_len > 3 && (z = memchr(ext_str, 't', ext_len)) && ((ext_str + ext_len) - z >= 2) && !memcmp(z + 1, "ar", 2)) {
		/* assume tar-based phar */
		return phar_open_or_create_tar(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);
	}

	return phar_create_or_parse_filename(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC);
}
/* }}} */

int phar_create_or_parse_filename(char *fname, int fname_len, char *alias, int alias_len, int is_data, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
{
	phar_archive_data *mydata;
	php_stream *fp;
	char *actual = NULL, *p;

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
		if (phar_open_from_fp(fp, fname, fname_len, alias, alias_len, options, pphar, error TSRMLS_CC) == SUCCESS) {
			if ((*pphar)->is_data || !PHAR_G(readonly)) {
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
	mydata = ecalloc(1, sizeof(phar_archive_data));
	mydata->fname = expand_filepath(fname, NULL TSRMLS_CC);
	fname_len = strlen(mydata->fname);
#ifdef PHP_WIN32
	phar_unixify_path_separators(mydata->fname, fname_len);
#endif
	p = strrchr(mydata->fname, '/');

	if (p) {
		mydata->ext = memchr(p, '.', (mydata->fname + fname_len) - p);
		if (mydata->ext == p) {
			mydata->ext = memchr(p + 1, '.', (mydata->fname + fname_len) - p - 1);
		}
		if (mydata->ext) {
			mydata->ext_len = (mydata->fname + fname_len) - mydata->ext;
		}
	}

	if (pphar) {
		*pphar = mydata;
	}

	zend_hash_init(&mydata->manifest, sizeof(phar_entry_info),
		zend_get_hash_value, destroy_phar_manifest_entry, 0);
	zend_hash_init(&mydata->mounted_dirs, sizeof(char *),
		zend_get_hash_value, NULL, 0);
	zend_hash_init(&mydata->virtual_dirs, sizeof(char *),
		zend_get_hash_value, NULL, (zend_bool)mydata->is_persistent);
	mydata->fname_len = fname_len;
	snprintf(mydata->version, sizeof(mydata->version), "%s", PHP_PHAR_API_VERSION);
	mydata->is_temporary_alias = alias ? 0 : 1;
	mydata->internal_file_start = -1;
	mydata->fp = NULL;
	mydata->is_writeable = 1;
	mydata->is_brandnew = 1;
	phar_request_initialize(TSRMLS_C);
	zend_hash_add(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len, (void*)&mydata, sizeof(phar_archive_data*),  NULL);

	if (is_data) {
		alias = NULL;
		alias_len = 0;
		mydata->is_data = 1;
		/* assume tar format, PharData can specify other */
		mydata->is_tar = 1;
	} else {
		phar_archive_data **fd_ptr;

		if (alias && SUCCESS == zend_hash_find(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void **)&fd_ptr)) {
			if (SUCCESS != phar_free_alias(*fd_ptr, alias, alias_len TSRMLS_CC)) {
				if (error) {
					spprintf(error, 4096, "phar error: phar \"%s\" cannot set alias \"%s\", already in use by another phar archive", mydata->fname, alias);
				}

				zend_hash_del(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len);

				if (pphar) {
					*pphar = NULL;
				}

				return FAILURE;
			}
		}

		mydata->alias = alias ? estrndup(alias, alias_len) : estrndup(mydata->fname, fname_len);
		mydata->alias_len = alias ? alias_len : fname_len;
	}

	if (alias_len && alias) {
		if (FAILURE == zend_hash_add(&(PHAR_GLOBALS->phar_alias_map), alias, alias_len, (void*)&mydata, sizeof(phar_archive_data*), NULL)) {
			if (options & REPORT_ERRORS) {
				if (error) {
					spprintf(error, 0, "archive \"%s\" cannot be associated with alias \"%s\", already in use", fname, alias);
				}
			}

			zend_hash_del(&(PHAR_GLOBALS->phar_fname_map), mydata->fname, fname_len);

			if (pphar) {
				*pphar = NULL;
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
 * that the manifest is proper, then pass it to phar_parse_pharfile().  SUCCESS
 * or FAILURE is returned and pphar is set to a pointer to the phar's manifest
 */
int phar_open_from_filename(char *fname, int fname_len, char *alias, int alias_len, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
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

	if (phar_open_parsed_phar(fname, fname_len, alias, alias_len, is_data, options, pphar, error TSRMLS_CC) == SUCCESS) {
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

	ret =  phar_open_from_fp(fp, fname, fname_len, alias, alias_len, options, pphar, error TSRMLS_CC);

	if (actual) {
		efree(actual);
	}

	return ret;
}
/* }}}*/

static inline char *phar_strnstr(const char *buf, int buf_len, const char *search, int search_len) /* {{{ */
{
	const char *c;
	int so_far = 0;

	if (buf_len < search_len) {
		return NULL;
	}

	c = buf - 1;

	do {
		if (!(c = memchr(c + 1, search[0], buf_len - search_len - so_far))) {
			return (char *) NULL;
		}

		so_far = c - buf;

		if (so_far >= (buf_len - search_len)) {
			return (char *) NULL;
		}

		if (!memcmp(c, search, search_len)) {
			return (char *) c;
		}
	} while (1);
}
/* }}} */

/**
 * Scan an open fp for the required __HALT_COMPILER(); ?> token and verify
 * that the manifest is proper, then pass it to phar_parse_pharfile().  SUCCESS
 * or FAILURE is returned and pphar is set to a pointer to the phar's manifest
 */
static int phar_open_from_fp(php_stream* fp, char *fname, int fname_len, char *alias, int alias_len, int options, phar_archive_data** pphar, char **error TSRMLS_DC) /* {{{ */
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

				if (!PHAR_G(has_zlib)) {
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

				if (!PHAR_G(has_bz2)) {
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
				return phar_parse_zipfile(fp, fname, fname_len, alias, alias_len, pphar, error TSRMLS_CC);
			}

			if (got > 512) {
				if (phar_is_tar(pos, fname)) {
					php_stream_rewind(fp);
					return phar_parse_tarfile(fp, fname, fname_len, alias, alias_len, pphar, compression, error TSRMLS_CC);
				}
			}
		}

		if (got > 0 && (pos = phar_strnstr(buffer, got + sizeof(token), token, sizeof(token)-1)) != NULL) {
			halt_offset += (pos - buffer); /* no -tokenlen+tokenlen here */
			return phar_parse_pharfile(fp, fname, fname_len, alias, alias_len, halt_offset, pphar, compression, error TSRMLS_CC);
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
static int phar_analyze_path(const char *fname, const char *ext, int ext_len, int for_create TSRMLS_DC) /* {{{ */
{
	php_stream_statbuf ssb;
	char *realpath, old, *a = (char *)(ext + ext_len);

	old = *a;
	*a = '\0';

	if ((realpath = expand_filepath(fname, NULL TSRMLS_CC))) {
#ifdef PHP_WIN32
		phar_unixify_path_separators(realpath, strlen(realpath));
#endif
		if (zend_hash_exists(&(PHAR_GLOBALS->phar_fname_map), realpath, strlen(realpath))) {
			*a = old;
			efree(realpath);
			return SUCCESS;
		}

		if (PHAR_G(manifest_cached) && zend_hash_exists(&cached_phars, realpath, strlen(realpath))) {
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
#ifdef PHP_WIN32
				phar_unixify_path_separators(realpath, strlen(realpath));
#endif
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
/* }}} */

/* check for ".phar" in extension */
static int phar_check_str(const char *fname, const char *ext_str, int ext_len, int executable, int for_create TSRMLS_DC) /* {{{ */
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
		pos = strstr(test, ".phar");

		if (pos && (*(pos - 1) != '/')
				&& (pos += 5) && (*pos == '\0' || *pos == '/' || *pos == '.')) {
			return phar_analyze_path(fname, ext_str, ext_len, for_create TSRMLS_CC);
		} else {
			return FAILURE;
		}
	}

	/* data phars need only contain a single non-"." to be valid */
	if (!executable) {
		pos = strstr(ext_str, ".phar");
		if (!(pos && (*(pos - 1) != '/')
					&& (pos += 5) && (*pos == '\0' || *pos == '/' || *pos == '.')) && *(ext_str + 1) != '.' && *(ext_str + 1) != '/' && *(ext_str + 1) != '\0') {
			return phar_analyze_path(fname, ext_str, ext_len, for_create TSRMLS_CC);
		}
	} else {
		if (*(ext_str + 1) != '.' && *(ext_str + 1) != '/' && *(ext_str + 1) != '\0') {
			return phar_analyze_path(fname, ext_str, ext_len, for_create TSRMLS_CC);
		}
	}

	return FAILURE;
}
/* }}} */

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
int phar_detect_phar_fname_ext(const char *filename, int filename_len, const char **ext_str, int *ext_len, int executable, int for_create, int is_complete TSRMLS_DC) /* {{{ */
{
	const char *pos, *slash;

	*ext_str = NULL;

	if (!filename_len || filename_len == 1) {
		return FAILURE;
	}

	phar_request_initialize(TSRMLS_C);
	/* first check for alias in first segment */
	pos = memchr(filename, '/', filename_len);

	if (pos && pos != filename) {
		if (zend_hash_exists(&(PHAR_GLOBALS->phar_alias_map), (char *) filename, pos - filename)) {
			*ext_str = pos;
			*ext_len = -1;
			return FAILURE;
		}

		if (PHAR_G(manifest_cached) && zend_hash_exists(&cached_alias, (char *) filename, pos - filename)) {
			*ext_str = pos;
			*ext_len = -1;
			return FAILURE;
		}
	}

	if (zend_hash_num_elements(&(PHAR_GLOBALS->phar_fname_map)) || PHAR_G(manifest_cached)) {
		phar_archive_data **pphar;

		if (is_complete) {
			if (SUCCESS == zend_hash_find(&(PHAR_GLOBALS->phar_fname_map), (char *) filename, filename_len, (void **)&pphar)) {
				*ext_str = filename + (filename_len - (*pphar)->ext_len);
woohoo:
				*ext_len = (*pphar)->ext_len;

				if (executable == 2) {
					return SUCCESS;
				}

				if (executable == 1 && !(*pphar)->is_data) {
					return SUCCESS;
				}

				if (!executable && (*pphar)->is_data) {
					return SUCCESS;
				}

				return FAILURE;
			}

			if (PHAR_G(manifest_cached) && SUCCESS == zend_hash_find(&cached_phars, (char *) filename, filename_len, (void **)&pphar)) {
				*ext_str = filename + (filename_len - (*pphar)->ext_len);
				goto woohoo;
			}
		} else {
			phar_zstr key;
			char *str_key;
			uint keylen;
			ulong unused;

			zend_hash_internal_pointer_reset(&(PHAR_GLOBALS->phar_fname_map));

			while (FAILURE != zend_hash_has_more_elements(&(PHAR_GLOBALS->phar_fname_map))) {
				if (HASH_KEY_NON_EXISTANT == zend_hash_get_current_key_ex(&(PHAR_GLOBALS->phar_fname_map), &key, &keylen, &unused, 0, NULL)) {
					break;
				}

				PHAR_STR(key, str_key);

				if (keylen > (uint) filename_len) {
					zend_hash_move_forward(&(PHAR_GLOBALS->phar_fname_map));
					continue;
				}

				if (!memcmp(filename, str_key, keylen) && ((uint)filename_len == keylen
					|| filename[keylen] == '/' || filename[keylen] == '\0')) {
					if (FAILURE == zend_hash_get_current_data(&(PHAR_GLOBALS->phar_fname_map), (void **) &pphar)) {
						break;
					}
					*ext_str = filename + (keylen - (*pphar)->ext_len);
					goto woohoo;
				}

				zend_hash_move_forward(&(PHAR_GLOBALS->phar_fname_map));
			}

			if (PHAR_G(manifest_cached)) {
				zend_hash_internal_pointer_reset(&cached_phars);

				while (FAILURE != zend_hash_has_more_elements(&cached_phars)) {
					if (HASH_KEY_NON_EXISTANT == zend_hash_get_current_key_ex(&cached_phars, &key, &keylen, &unused, 0, NULL)) {
						break;
					}

					PHAR_STR(key, str_key);

					if (keylen > (uint) filename_len) {
						zend_hash_move_forward(&cached_phars);
						continue;
					}

					if (!memcmp(filename, str_key, keylen) && ((uint)filename_len == keylen
						|| filename[keylen] == '/' || filename[keylen] == '\0')) {
						if (FAILURE == zend_hash_get_current_data(&cached_phars, (void **) &pphar)) {
							break;
						}
						*ext_str = filename + (keylen - (*pphar)->ext_len);
						goto woohoo;
					}
					zend_hash_move_forward(&cached_phars);
				}
			}
		}
	}

	pos = memchr(filename + 1, '.', filename_len);
next_extension:
	if (!pos) {
		return FAILURE;
	}

	while (pos != filename && (*(pos - 1) == '/' || *(pos - 1) == '\0')) {
		pos = memchr(pos + 1, '.', filename_len - (pos - filename) + 1);
		if (!pos) {
			return FAILURE;
		}
	}

	slash = memchr(pos, '/', filename_len - (pos - filename));

	if (!slash) {
		/* this is a url like "phar://blah.phar" with no directory */
		*ext_str = pos;
		*ext_len = strlen(pos);

		/* file extension must contain "phar" */
		switch (phar_check_str(filename, *ext_str, *ext_len, executable, for_create TSRMLS_CC)) {
			case SUCCESS:
				return SUCCESS;
			case FAILURE:
				/* we are at the end of the string, so we fail */
				return FAILURE;
		}
	}

	/* we've found an extension that ends at a directory separator */
	*ext_str = pos;
	*ext_len = slash - pos;

	switch (phar_check_str(filename, *ext_str, *ext_len, executable, for_create TSRMLS_CC)) {
		case SUCCESS:
			return SUCCESS;
		case FAILURE:
			/* look for more extensions */
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

static int php_check_dots(const char *element, int n) /* {{{ */
{
	for(n--; n >= 0; --n) {
		if (element[n] != '.') {
			return 1;
		}
	}
	return 0;
}
/* }}} */

#define IS_DIRECTORY_UP(element, len) \
	(len >= 2 && !php_check_dots(element, len))

#define IS_DIRECTORY_CURRENT(element, len) \
	(len == 1 && element[0] == '.')

#define IS_BACKSLASH(c) ((c) == '/')

#ifdef COMPILE_DL_PHAR
/* stupid-ass non-extern declaration in tsrm_strtok.h breaks dumbass MS compiler */
static inline int in_character_class(char ch, const char *delim) /* {{{ */
{
	while (*delim) {
		if (*delim == ch) {
			return 1;
		}
		++delim;
	}
	return 0;
}
/* }}} */

char *tsrm_strtok_r(char *s, const char *delim, char **last) /* {{{ */
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
/* }}} */
#endif

/**
 * Remove .. and . references within a phar filename
 */
char *phar_fix_filepath(char *path, int *new_len, int use_cwd TSRMLS_DC) /* {{{ */
{
	char newpath[MAXPATHLEN];
	int newpath_len;
	char *ptr;
	char *tok;
	int ptr_length, path_length = *new_len;

	if (PHAR_G(cwd_len) && use_cwd && path_length > 2 && path[0] == '.' && path[1] == '/') {
		newpath_len = PHAR_G(cwd_len);
		memcpy(newpath, PHAR_G(cwd), newpath_len);
	} else {
		newpath[0] = '/';
		newpath_len = 1;
	}

	ptr = path;

	if (*ptr == '/') {
		++ptr;
	}

	tok = ptr;

	do {
		ptr = memchr(ptr, '/', path_length - (ptr - path));
	} while (ptr && ptr - tok == 0 && *ptr == '/' && ++ptr && ++tok);

	if (!ptr && (path_length - (tok - path))) {
		switch (path_length - (tok - path)) {
			case 1:
				if (*tok == '.') {
					efree(path);
					*new_len = 1;
					return estrndup("/", 1);
				}
				break;
			case 2:
				if (tok[0] == '.' && tok[1] == '.') {
					efree(path);
					*new_len = 1;
					return estrndup("/", 1);
				}
		}
		return path;
	}

	while (ptr) {
		ptr_length = ptr - tok;
last_time:
		if (IS_DIRECTORY_UP(tok, ptr_length)) {
#define PREVIOUS newpath[newpath_len - 1]

			while (newpath_len > 1 && !IS_BACKSLASH(PREVIOUS)) {
				newpath_len--;
			}

			if (newpath[0] != '/') {
				newpath[newpath_len] = '\0';
			} else if (newpath_len > 1) {
				--newpath_len;
			}
		} else if (!IS_DIRECTORY_CURRENT(tok, ptr_length)) {
			if (newpath_len > 1) {
				newpath[newpath_len++] = '/';
				memcpy(newpath + newpath_len, tok, ptr_length+1);
			} else {
				memcpy(newpath + newpath_len, tok, ptr_length+1);
			}

			newpath_len += ptr_length;
		}

		if (ptr == path + path_length) {
			break;
		}

		tok = ++ptr;

		do {
			ptr = memchr(ptr, '/', path_length - (ptr - path));
		} while (ptr && ptr - tok == 0 && *ptr == '/' && ++ptr && ++tok);

		if (!ptr && (path_length - (tok - path))) {
			ptr_length = path_length - (tok - path);
			ptr = path + path_length;
			goto last_time;
		}
	}

	efree(path);
	*new_len = newpath_len;
	return estrndup(newpath, newpath_len);
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
 * This is used by phar_parse_url()
 */
int phar_split_fname(char *filename, int filename_len, char **arch, int *arch_len, char **entry, int *entry_len, int executable, int for_create TSRMLS_DC) /* {{{ */
{
	const char *ext_str;
#ifdef PHP_WIN32
	char *save;
#endif
	int ext_len, free_filename = 0;

	if (!strncasecmp(filename, "phar://", 7)) {
		filename += 7;
		filename_len -= 7;
	}

	ext_len = 0;
#ifdef PHP_WIN32
	free_filename = 1;
	save = filename;
	filename = estrndup(filename, filename_len);
	phar_unixify_path_separators(filename, filename_len);
#endif
	if (phar_detect_phar_fname_ext(filename, filename_len, &ext_str, &ext_len, executable, for_create, 0 TSRMLS_CC) == FAILURE) {
		if (ext_len != -1) {
			if (!ext_str) {
				/* no / detected, restore arch for error message */
#ifdef PHP_WIN32
				*arch = save;
#else
				*arch = filename;
#endif
			}

			if (free_filename) {
				efree(filename);
			}

			return FAILURE;
		}

		ext_len = 0;
		/* no extension detected - instead we are dealing with an alias */
	}

	*arch_len = ext_str - filename + ext_len;
	*arch = estrndup(filename, *arch_len);

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

	if (free_filename) {
		efree(filename);
	}

	return SUCCESS;
}
/* }}} */

/**
 * Invoked when a user calls Phar::mapPhar() from within an executing .phar
 * to set up its manifest directly
 */
int phar_open_executed_filename(char *alias, int alias_len, char **error TSRMLS_DC) /* {{{ */
{
	char *fname;
	zval *halt_constant;
	php_stream *fp;
	int fname_len;
	char *actual = NULL;
	int ret;

	if (error) {
		*error = NULL;
	}

	fname = zend_get_executed_filename(TSRMLS_C);
	fname_len = strlen(fname);

	if (phar_open_parsed_phar(fname, fname_len, alias, alias_len, 0, REPORT_ERRORS, NULL, 0 TSRMLS_CC) == SUCCESS) {
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

	FREE_ZVAL(halt_constant);

#if PHP_MAJOR_VERSION < 6
	if (PG(safe_mode) && (!php_checkuid(fname, NULL, CHECKUID_ALLOW_ONLY_FILE))) {
		return FAILURE;
	}
#endif

	if (php_check_open_basedir(fname TSRMLS_CC)) {
		return FAILURE;
	}

	fp = php_stream_open_wrapper(fname, "rb", IGNORE_URL|STREAM_MUST_SEEK|REPORT_ERRORS, &actual);

	if (!fp) {
		if (error) {
			spprintf(error, 0, "unable to open phar for reading \"%s\"", fname);
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

	ret = phar_open_from_fp(fp, fname, fname_len, alias, alias_len, REPORT_ERRORS, NULL, error TSRMLS_CC);

	if (actual) {
		efree(actual);
	}

	return ret;
}
/* }}} */

/**
 * Validate the CRC32 of a file opened from within the phar
 */
int phar_postprocess_file(phar_entry_data *idata, php_uint32 crc32, char **error, int process_zip TSRMLS_DC) /* {{{ */
{
	php_uint32 crc = ~0;
	int len = idata->internal_file->uncompressed_filesize;
	php_stream *fp = idata->fp;
	phar_entry_info *entry = idata->internal_file;

	if (error) {
		*error = NULL;
	}

	if (entry->is_zip && process_zip > 0) {
		/* verify local file header */
		phar_zip_file_header local;

		if (SUCCESS != phar_open_archive_fp(idata->phar TSRMLS_CC)) {
			spprintf(error, 0, "phar error: unable to open zip-based phar archive \"%s\" to verify local file header for file \"%s\"", idata->phar->fname, entry->filename);
			return FAILURE;
		}
		php_stream_seek(phar_get_entrypfp(idata->internal_file TSRMLS_CC), entry->header_offset, SEEK_SET);

		if (sizeof(local) != php_stream_read(phar_get_entrypfp(idata->internal_file TSRMLS_CC), (char *) &local, sizeof(local))) {

			spprintf(error, 0, "phar error: internal corruption of zip-based phar \"%s\" (cannot read local file header for file \"%s\")", idata->phar->fname, entry->filename);
			return FAILURE;
		}

		/* verify local header */
		if (entry->filename_len != PHAR_ZIP_16(local.filename_len) || entry->crc32 != PHAR_ZIP_32(local.crc32) || entry->uncompressed_filesize != PHAR_ZIP_32(local.uncompsize) || entry->compressed_filesize != PHAR_ZIP_32(local.compsize)) {
			spprintf(error, 0, "phar error: internal corruption of zip-based phar \"%s\" (local head of file \"%s\" does not match central directory)", idata->phar->fname, entry->filename);
			return FAILURE;
		}

		/* construct actual offset to file start - local extra_len can be different from central extra_len */
		entry->offset = entry->offset_abs =
			sizeof(local) + entry->header_offset + PHAR_ZIP_16(local.filename_len) + PHAR_ZIP_16(local.extra_len);

		if (idata->zero && idata->zero != entry->offset_abs) {
			idata->zero = entry->offset_abs;
		}
	}

	if (process_zip == 1) {
		return SUCCESS;
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
		spprintf(error, 0, "phar error: internal corruption of phar \"%s\" (crc32 mismatch on file \"%s\")", idata->phar->fname, entry->filename);
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

char *phar_create_default_stub(const char *index_php, const char *web_index, size_t *len, char **error TSRMLS_DC) /* {{{ */
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
/* }}} */

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
	int free_user_stub, free_fp = 1, free_ufp = 1;

	if (phar->is_persistent) {
		if (error) {
			spprintf(error, 0, "internal error: attempt to flush cached zip-based phar \"%s\"", phar->fname);
		}
		return EOF;
	}

	if (error) {
		*error = NULL;
	}

	if (!zend_hash_num_elements(&phar->manifest) && !user_stub) {
		return EOF;
	}

	if (phar->is_zip) {
		return phar_zip_flush(phar, user_stub, len, convert, error TSRMLS_CC);
	}

	if (phar->is_tar) {
		return phar_tar_flush(phar, user_stub, len, convert, error TSRMLS_CC);
	}

	if (PHAR_G(readonly)) {
		return EOF;
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
		||			  5 != php_stream_write(newfile, " ?>\r\n", 5)) {
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
		size_t written;

		if (!user_stub && phar->halt_offset && oldfile && !phar->is_brandnew) {
			written = php_stream_copy_to_stream(oldfile, newfile, phar->halt_offset);
			newstub = NULL;
		} else {
			/* this is either a brand new phar or a default stub overwrite */
			newstub = phar_create_default_stub(NULL, NULL, &(phar->halt_offset), NULL TSRMLS_CC);
			written = php_stream_write(newfile, newstub, phar->halt_offset);
		}
		if (phar->halt_offset != written) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				if (newstub) {
					spprintf(error, 0, "unable to create stub in new phar \"%s\"", phar->fname);
				} else {
					spprintf(error, 0, "unable to copy stub of old phar to new phar \"%s\"", phar->fname);
				}
			}
			if (newstub) {
				efree(newstub);
			}
			return EOF;
		}
		if (newstub) {
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
		if (entry->is_deleted || entry->is_mounted) {
			/* remove this from the new phar */
			continue;
		}
		if (!entry->is_modified && entry->fp_refcount) {
			/* open file pointers refer to this fp, do not free the stream */
			switch (entry->fp_type) {
				case PHAR_FP:
					free_fp = 0;
					break;
				case PHAR_UFP:
					free_ufp = 0;
				default:
					break;
			}
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
			if (entry->fp_type == PHAR_UFP) {
				/* reset so we can copy the compressed data over */
				entry->fp_type = PHAR_FP;
			}
			continue;
		}
		if (!phar_get_efp(entry, 0 TSRMLS_CC)) {
			/* re-open internal file pointer just-in-time */
			newentry = phar_open_jit(phar, entry, error TSRMLS_CC);
			if (!newentry) {
				/* major problem re-opening, so we ignore this file and the error */
				efree(*error);
				*error = NULL;
				continue;
			}
			entry = newentry;
		}
		file = phar_get_efp(entry, 0 TSRMLS_CC);
		if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0, 1 TSRMLS_CC)) {
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
		if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0, 0 TSRMLS_CC)) {
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
	if (4 != php_stream_write(newfile, manifest, 4) || (main_metadata_str.len
	&& main_metadata_str.len != php_stream_write(newfile, main_metadata_str.c, main_metadata_str.len))) {
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

		if (entry->is_deleted || entry->is_mounted) {
			/* remove this from the new phar if deleted, ignore if mounted */
			continue;
		}

		if (entry->is_dir) {
			/* add 1 for trailing slash */
			phar_set_32(entry_buffer, entry->filename_len + 1);
		} else {
			phar_set_32(entry_buffer, entry->filename_len);
		}

		if (4 != php_stream_write(newfile, entry_buffer, 4)
		|| entry->filename_len != php_stream_write(newfile, entry->filename, entry->filename_len)
		|| (entry->is_dir && 1 != php_stream_write(newfile, "/", 1))) {
			if (closeoldfile) {
				php_stream_close(oldfile);
			}
			php_stream_close(newfile);
			if (error) {
				if (entry->is_dir) {
					spprintf(error, 0, "unable to write filename of directory \"%s\" to manifest of new phar \"%s\"", entry->filename, phar->fname);
				} else {
					spprintf(error, 0, "unable to write filename of file \"%s\" to manifest of new phar \"%s\"", entry->filename, phar->fname);
				}
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

		if (entry->is_deleted || entry->is_dir || entry->is_mounted) {
			continue;
		}

		if (entry->cfp) {
			file = entry->cfp;
			php_stream_rewind(file);
		} else {
			file = phar_get_efp(entry, 0 TSRMLS_CC);
			if (-1 == phar_seek_efp(entry, 0, SEEK_SET, 0, 0 TSRMLS_CC)) {
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

		/* this will have changed for all files that have either changed compression or been modified */
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
			/* this fp is in use by a phar_entry_data returned by phar_get_entry_data, it will be closed when the phar_entry_data is phar_entry_delref'ed */
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
		char sig_buf[4];

		php_stream_rewind(newfile);

		if (phar->signature) {
			efree(phar->signature);
			phar->signature = NULL;
		}

		switch(phar->sig_flags) {
#ifndef HAVE_HASH_EXT
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
			default: {
				char *digest;
				int digest_len;

				if (FAILURE == phar_create_signature(phar, newfile, &digest, &digest_len, error TSRMLS_CC)) {
					if (error) {
						char *save = *error;
						spprintf(error, 0, "phar error: unable to write signature: %s", save);
						efree(save);
					}
					efree(digest);
					if (closeoldfile) {
						php_stream_close(oldfile);
					}
					php_stream_close(newfile);
					return EOF;
				}

				php_stream_write(newfile, digest, digest_len);
				efree(digest);
				if (phar->sig_flags == PHAR_SIG_OPENSSL) {
					phar_set_32(sig_buf, digest_len);
					php_stream_write(newfile, sig_buf, 4);
				}
				break;
			}
		}
		phar_set_32(sig_buf, phar->sig_flags);
		php_stream_write(newfile, sig_buf, 4);
		php_stream_write(newfile, "GBMB", 4);
	}

	/* finally, close the temp file, rename the original phar,
	   move the temp to the old phar, unlink the old phar, and reload it into memory
	*/
	if (phar->fp && free_fp) {
		php_stream_close(phar->fp);
	}

	if (phar->ufp) {
		if (free_ufp) {
			php_stream_close(phar->ufp);
		}
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
	{NULL, NULL, NULL} /* Must be the last line in phar_functions[] */
};
/* }}}*/

static size_t phar_zend_stream_reader(void *handle, char *buf, size_t len TSRMLS_DC) /* {{{ */
{
	return php_stream_read(phar_get_pharfp((phar_archive_data*)handle TSRMLS_CC), buf, len);
}
/* }}} */

#if PHP_VERSION_ID >= 50300
static size_t phar_zend_stream_fsizer(void *handle TSRMLS_DC) /* {{{ */
{
	return ((phar_archive_data*)handle)->halt_offset + 32;
} /* }}} */

#else /* PHP_VERSION_ID */

static long phar_stream_fteller_for_zend(void *handle TSRMLS_DC) /* {{{ */
{
	return (long)php_stream_tell(phar_get_pharfp((phar_archive_data*)handle TSRMLS_CC));
}
/* }}} */
#endif

zend_op_array *(*phar_orig_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);
#if PHP_VERSION_ID >= 50300
#define phar_orig_zend_open zend_stream_open_function
static char *phar_resolve_path(const char *filename, int filename_len TSRMLS_DC)
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
		if (SUCCESS == phar_open_from_filename(file_handle->filename, strlen(file_handle->filename), NULL, 0, 0, &phar, NULL TSRMLS_CC)) {
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
#if PHP_VERSION_ID >= 50300
				file_handle->type = ZEND_HANDLE_STREAM;
				/* we do our own reading directly from the phar, don't change the next line */
				file_handle->handle.stream.handle  = phar;
				file_handle->handle.stream.reader  = phar_zend_stream_reader;
				file_handle->handle.stream.closer  = NULL;
				file_handle->handle.stream.fsizer  = phar_zend_stream_fsizer;
				file_handle->handle.stream.isatty  = 0;
				phar->is_persistent ?
					php_stream_rewind(PHAR_GLOBALS->cached_fp[phar->phar_pos].fp) :
					php_stream_rewind(phar->fp);
				memset(&file_handle->handle.stream.mmap, 0, sizeof(file_handle->handle.stream.mmap));
#else /* PHP_VERSION_ID */
				file_handle->type = ZEND_HANDLE_STREAM;
				/* we do our own reading directly from the phar, don't change the next line */
				file_handle->handle.stream.handle = phar;
				file_handle->handle.stream.reader = phar_zend_stream_reader;
				file_handle->handle.stream.closer = NULL; /* don't close - let phar handle this one */
				file_handle->handle.stream.fteller = phar_stream_fteller_for_zend;
				file_handle->handle.stream.interactive = 0;
				phar->is_persistent ?
					php_stream_rewind(PHAR_GLOBALS->cached_fp[phar->phar_pos].fp) :
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
				if (!pphar && PHAR_G(manifest_cached)) {
					zend_hash_find(&cached_phars, arch, arch_len, (void **) &pphar);
				}
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

PHP_GINIT_FUNCTION(phar) /* {{{ */
{
	phar_mime_type mime;

	memset(phar_globals, 0, sizeof(zend_phar_globals));
	phar_globals->readonly = 1;

	zend_hash_init(&phar_globals->mime_types, 0, NULL, NULL, 1);

#define PHAR_SET_MIME(mimetype, ret, fileext) \
		mime.mime = mimetype; \
		mime.len = sizeof((mimetype))+1; \
		mime.type = ret; \
		zend_hash_add(&phar_globals->mime_types, fileext, sizeof(fileext)-1, (void *)&mime, sizeof(phar_mime_type), NULL); \

	PHAR_SET_MIME("text/html", PHAR_MIME_PHPS, "phps")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "c")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "cc")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "cpp")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "c++")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "dtd")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "h")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "log")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "rng")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "txt")
	PHAR_SET_MIME("text/plain", PHAR_MIME_OTHER, "xsd")
	PHAR_SET_MIME("", PHAR_MIME_PHP, "php")
	PHAR_SET_MIME("", PHAR_MIME_PHP, "inc")
	PHAR_SET_MIME("video/avi", PHAR_MIME_OTHER, "avi")
	PHAR_SET_MIME("image/bmp", PHAR_MIME_OTHER, "bmp")
	PHAR_SET_MIME("text/css", PHAR_MIME_OTHER, "css")
	PHAR_SET_MIME("image/gif", PHAR_MIME_OTHER, "gif")
	PHAR_SET_MIME("text/html", PHAR_MIME_OTHER, "htm")
	PHAR_SET_MIME("text/html", PHAR_MIME_OTHER, "html")
	PHAR_SET_MIME("text/html", PHAR_MIME_OTHER, "htmls")
	PHAR_SET_MIME("image/x-ico", PHAR_MIME_OTHER, "ico")
	PHAR_SET_MIME("image/jpeg", PHAR_MIME_OTHER, "jpe")
	PHAR_SET_MIME("image/jpeg", PHAR_MIME_OTHER, "jpg")
	PHAR_SET_MIME("image/jpeg", PHAR_MIME_OTHER, "jpeg")
	PHAR_SET_MIME("application/x-javascript", PHAR_MIME_OTHER, "js")
	PHAR_SET_MIME("audio/midi", PHAR_MIME_OTHER, "midi")
	PHAR_SET_MIME("audio/midi", PHAR_MIME_OTHER, "mid")
	PHAR_SET_MIME("audio/mod", PHAR_MIME_OTHER, "mod")
	PHAR_SET_MIME("movie/quicktime", PHAR_MIME_OTHER, "mov")
	PHAR_SET_MIME("audio/mp3", PHAR_MIME_OTHER, "mp3")
	PHAR_SET_MIME("video/mpeg", PHAR_MIME_OTHER, "mpg")
	PHAR_SET_MIME("video/mpeg", PHAR_MIME_OTHER, "mpeg")
	PHAR_SET_MIME("application/pdf", PHAR_MIME_OTHER, "pdf")
	PHAR_SET_MIME("image/png", PHAR_MIME_OTHER, "png")
	PHAR_SET_MIME("application/shockwave-flash", PHAR_MIME_OTHER, "swf")
	PHAR_SET_MIME("image/tiff", PHAR_MIME_OTHER, "tif")
	PHAR_SET_MIME("image/tiff", PHAR_MIME_OTHER, "tiff")
	PHAR_SET_MIME("audio/wav", PHAR_MIME_OTHER, "wav")
	PHAR_SET_MIME("image/xbm", PHAR_MIME_OTHER, "xbm")
	PHAR_SET_MIME("text/xml", PHAR_MIME_OTHER, "xml")

	phar_restore_orig_functions(TSRMLS_C);
}
/* }}} */

PHP_GSHUTDOWN_FUNCTION(phar) /* {{{ */
{
	zend_hash_destroy(&phar_globals->mime_types);
}
/* }}} */

PHP_MINIT_FUNCTION(phar) /* {{{ */
{
	REGISTER_INI_ENTRIES();

	phar_orig_compile_file = zend_compile_file;
	zend_compile_file = phar_compile_file;

#if PHP_VERSION_ID >= 50300
	phar_save_resolve_path = zend_resolve_path;
	zend_resolve_path = phar_resolve_path;
#else
	phar_orig_zend_open = zend_stream_open_function;
	zend_stream_open_function = phar_zend_open;
#endif

	phar_object_init(TSRMLS_C);

	phar_intercept_functions_init(TSRMLS_C);
	phar_save_orig_functions(TSRMLS_C);

	return php_register_url_stream_wrapper("phar", &php_stream_phar_wrapper TSRMLS_CC);
}
/* }}} */

PHP_MSHUTDOWN_FUNCTION(phar) /* {{{ */
{
	php_unregister_url_stream_wrapper("phar" TSRMLS_CC);

	phar_intercept_functions_shutdown(TSRMLS_C);

	if (zend_compile_file == phar_compile_file) {
		zend_compile_file = phar_orig_compile_file;
	}

#if PHP_VERSION_ID < 50300
	if (zend_stream_open_function == phar_zend_open) {
		zend_stream_open_function = phar_orig_zend_open;
	}
#endif
	if (PHAR_G(manifest_cached)) {
		zend_hash_destroy(&(cached_phars));
		zend_hash_destroy(&(cached_alias));
	}

	return SUCCESS;
}
/* }}} */

void phar_request_initialize(TSRMLS_D) /* {{{ */
{
	if (!PHAR_GLOBALS->request_init)
	{
		PHAR_G(last_phar) = NULL;
		PHAR_G(last_phar_name) = PHAR_G(last_alias) = NULL;
		PHAR_G(has_bz2) = zend_hash_exists(&module_registry, "bz2", sizeof("bz2"));
		PHAR_G(has_zlib) = zend_hash_exists(&module_registry, "zlib", sizeof("zlib"));
		PHAR_GLOBALS->request_init = 1;
		PHAR_GLOBALS->request_ends = 0;
		PHAR_GLOBALS->request_done = 0;
		zend_hash_init(&(PHAR_GLOBALS->phar_fname_map), 5, zend_get_hash_value, destroy_phar_data,  0);
		zend_hash_init(&(PHAR_GLOBALS->phar_alias_map), 5, zend_get_hash_value, NULL, 0);

		if (PHAR_G(manifest_cached)) {
			phar_archive_data **pphar;
			phar_entry_fp *stuff = (phar_entry_fp *) ecalloc(zend_hash_num_elements(&cached_phars), sizeof(phar_entry_fp));

			for (zend_hash_internal_pointer_reset(&cached_phars);
			zend_hash_get_current_data(&cached_phars, (void **)&pphar) == SUCCESS;
			zend_hash_move_forward(&cached_phars)) {
				stuff[pphar[0]->phar_pos].manifest = (phar_entry_fp_info *) ecalloc( zend_hash_num_elements(&(pphar[0]->manifest)), sizeof(phar_entry_fp_info));
			}

			PHAR_GLOBALS->cached_fp = stuff;
		}

		PHAR_GLOBALS->phar_SERVER_mung_list = 0;
		PHAR_G(cwd) = NULL;
		PHAR_G(cwd_len) = 0;
		PHAR_G(cwd_init) = 0;
	}
}
/* }}} */

PHP_RSHUTDOWN_FUNCTION(phar) /* {{{ */
{
	int i;

	PHAR_GLOBALS->request_ends = 1;

	if (PHAR_GLOBALS->request_init)
	{
		phar_release_functions(TSRMLS_C);
		zend_hash_destroy(&(PHAR_GLOBALS->phar_alias_map));
		PHAR_GLOBALS->phar_alias_map.arBuckets = NULL;
		zend_hash_destroy(&(PHAR_GLOBALS->phar_fname_map));
		PHAR_GLOBALS->phar_fname_map.arBuckets = NULL;
		PHAR_GLOBALS->phar_SERVER_mung_list = 0;

		if (PHAR_GLOBALS->cached_fp) {
			for (i = 0; i < zend_hash_num_elements(&cached_phars); ++i) {
				if (PHAR_GLOBALS->cached_fp[i].fp) {
					php_stream_close(PHAR_GLOBALS->cached_fp[i].fp);
				}
				if (PHAR_GLOBALS->cached_fp[i].ufp) {
					php_stream_close(PHAR_GLOBALS->cached_fp[i].ufp);
				}
				efree(PHAR_GLOBALS->cached_fp[i].manifest);
			}
			efree(PHAR_GLOBALS->cached_fp);
			PHAR_GLOBALS->cached_fp = 0;
		}

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
	phar_request_initialize(TSRMLS_C);
	php_info_print_table_start();
	php_info_print_table_header(2, "Phar: PHP Archive support", "enabled");
	php_info_print_table_row(2, "Phar EXT version", PHP_PHAR_VERSION);
	php_info_print_table_row(2, "Phar API version", PHP_PHAR_API_VERSION);
	php_info_print_table_row(2, "CVS revision", "$Revision$");
	php_info_print_table_row(2, "Phar-based phar archives", "enabled");
	php_info_print_table_row(2, "Tar-based phar archives", "enabled");
	php_info_print_table_row(2, "ZIP-based phar archives", "enabled");

	if (PHAR_G(has_zlib)) {
		php_info_print_table_row(2, "gzip compression", "enabled");
	} else {
		php_info_print_table_row(2, "gzip compression", "disabled (install ext/zlib)");
	}

	if (PHAR_G(has_bz2)) {
		php_info_print_table_row(2, "bzip2 compression", "enabled");
	} else {
		php_info_print_table_row(2, "bzip2 compression", "disabled (install pecl/bz2)");
	}
#ifdef PHAR_HAVE_OPENSSL
	php_info_print_table_row(2, "Native OpenSSL support", "enabled");
#else
	if (zend_hash_exists(&module_registry, "openssl", sizeof("openssl"))) {
		php_info_print_table_row(2, "OpenSSL support", "enabled");
	} else {
		php_info_print_table_row(2, "OpenSSL support", "disabled (install ext/openssl)");
	}
#endif
	php_info_print_table_end();

	php_info_print_box_start(0);
	PUTS("Phar based on pear/PHP_Archive, original concept by Davey Shafik.");
	PUTS(!sapi_module.phpinfo_as_text?"<br />":"\n");
	PUTS("Phar fully realized by Gregory Beaver and Marcus Boerger.");
	PUTS(!sapi_module.phpinfo_as_text?"<br />":"\n");
	PUTS("Portions of tar implementation Copyright (c) 2003-2008 Tim Kientzle.");
	php_info_print_box_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ phar_module_entry
 */
static zend_module_dep phar_deps[] = {
	ZEND_MOD_OPTIONAL("apc")
	ZEND_MOD_OPTIONAL("bz2")
	ZEND_MOD_OPTIONAL("openssl")
	ZEND_MOD_OPTIONAL("zlib")
	ZEND_MOD_OPTIONAL("standard")
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
	PHP_MODULE_GLOBALS(phar),   /* globals descriptor */
	PHP_GINIT(phar),            /* globals ctor */
	PHP_GSHUTDOWN(phar),        /* globals dtor */
	NULL,                       /* post deactivate */
	STANDARD_MODULE_PROPERTIES_EX
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
