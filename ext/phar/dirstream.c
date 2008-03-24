/*
  +----------------------------------------------------------------------+
  | phar:// stream wrapper support                                       |
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

#define PHAR_DIRSTREAM 1
#include "phar_internal.h"
#include "dirstream.h"

BEGIN_EXTERN_C()
void phar_dostat(phar_archive_data *phar, phar_entry_info *data, php_stream_statbuf *ssb, 
			zend_bool is_dir, char *alias, int alias_len TSRMLS_DC);
END_EXTERN_C()

php_stream_ops phar_dir_ops = {
	phar_dir_write, /* write */
	phar_dir_read,  /* read  */
	phar_dir_close, /* close */
	phar_dir_flush, /* flush */
	"phar dir",
	phar_dir_seek,  /* seek */
	NULL,           /* cast */
	phar_dir_stat,  /* stat */
	NULL, /* set option */
};

/**
 * Used for closedir($fp) where $fp is an opendir('phar://...') directory handle
 */
static int phar_dir_close(php_stream *stream, int close_handle TSRMLS_DC)  /* {{{ */
{
	HashTable *data = (HashTable *)stream->abstract;

	if (data && data->arBuckets)
	{
		zend_hash_destroy(data);
		data->arBuckets = 0;
		FREE_HASHTABLE(data);
		stream->abstract = NULL;
	}
	return 0;
}
/* }}} */

/**
 * Used for seeking on a phar directory handle
 */
static int phar_dir_seek(php_stream *stream, off_t offset, int whence, off_t *newoffset TSRMLS_DC) /* {{{ */
{
	HashTable *data = (HashTable *)stream->abstract;

	if (data)
	{
		if (whence == SEEK_END) {
			whence = SEEK_SET;
			offset = zend_hash_num_elements(data) + offset;
		}
		if (whence == SEEK_SET) {
			zend_hash_internal_pointer_reset(data);
		}

		if (offset < 0) {
			php_stream_wrapper_log_error(stream->wrapper, stream->flags TSRMLS_CC, "phar error: cannot seek because the resulting seek is negative");
			return -1;
		} else {
			*newoffset = 0;
			while (*newoffset < offset && zend_hash_move_forward(data) == SUCCESS) {
				++(*newoffset);
			}
			return 0;
		}
	}
	return -1;
}
/* }}} */

/**
 * Used for readdir() on an opendir()ed phar directory handle
 */
static size_t phar_dir_read(php_stream *stream, char *buf, size_t count TSRMLS_DC) /* {{{ */
{
	size_t to_read;
	HashTable *data = (HashTable *)stream->abstract;
	char *key;
	uint keylen;
	ulong unused;

	if (FAILURE == zend_hash_has_more_elements(data)) {
		return 0;
	}
	if (HASH_KEY_NON_EXISTANT == zend_hash_get_current_key_ex(data, &key, &keylen, &unused, 0, NULL)) {
		return 0;
	}
	zend_hash_move_forward(data);
	to_read = MIN(keylen, count);
	if (to_read == 0 || count < keylen) {
		return 0;
	}
	memset(buf, 0, sizeof(php_stream_dirent));
	memcpy(((php_stream_dirent *) buf)->d_name, key, to_read);
	((php_stream_dirent *) buf)->d_name[to_read + 1] = '\0';

	return sizeof(php_stream_dirent);
}
/* }}} */

/**
 * Dummy: Used for writing to a phar directory (i.e. not used)
 */
static size_t phar_dir_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC) /* {{{ */
{
	return 0;
}
/* }}} */

/**
 * Dummy: Used for flushing writes to a phar directory (i.e. not used)
 */
static int phar_dir_flush(php_stream *stream TSRMLS_DC) /* {{{ */
{
	return EOF;
}
/* }}} */

/**
 * Stat a dir in a phar
 */
static int phar_dir_stat(php_stream *stream, php_stream_statbuf *ssb TSRMLS_DC) /* {{{ */
{
	phar_entry_data *data = (phar_entry_data *)stream->abstract;

	/* If ssb is NULL then someone is misbehaving */
	if (!ssb) {
		return -1;
	}

	phar_dostat(data->phar, data->internal_file, ssb, 0, data->phar->alias, data->phar->alias_len TSRMLS_CC);
	return 0;
}
/* }}} */

/**
 * add an empty element with a char * key to a hash table, avoiding duplicates
 *
 * This is used to get a unique listing of virtual directories within a phar,
 * for iterating over opendir()ed phar directories.
 */
static int phar_add_empty(HashTable *ht, char *arKey, uint nKeyLength)  /* {{{ */
{
	void *dummy = (void *) 1;

	return zend_hash_update(ht, arKey, nKeyLength, &dummy, sizeof(void *), NULL);
}
/* }}} */

/**
 * Used for sorting directories alphabetically
 */
static int phar_compare_dir_name(const void *a, const void *b TSRMLS_DC)  /* {{{ */
{
	Bucket *f;
	Bucket *s;
	int result;
 
	f = *((Bucket **) a);
	s = *((Bucket **) b);

#if (PHP_MAJOR_VERSION < 6)
	result = zend_binary_strcmp(f->arKey, f->nKeyLength, s->arKey, s->nKeyLength);
#else
	result = zend_binary_strcmp(f->key.arKey.s, f->nKeyLength, s->key.arKey.s, s->nKeyLength);
#endif

	if (result < 0) {
		return -1;
	} else if (result > 0) {
		return 1;
	} else {
		return 0;
	}
}
/* }}} */

/**
 * Create a opendir() directory stream handle by iterating over each of the
 * files in a phar and retrieving its relative path.  From this, construct
 * a list of files/directories that are "in" the directory represented by dir
 */
static php_stream *phar_make_dirstream(char *dir, HashTable *manifest TSRMLS_DC) /* {{{ */
{
	HashTable *data;
	int dirlen = strlen(dir);
	char *save, *found, *key;
	uint keylen;
	ulong unused;
	char *entry;
	ALLOC_HASHTABLE(data);
	zend_hash_init(data, 64, zend_get_hash_value, NULL, 0);

	if (*dir == '/' && dirlen == 1 && (manifest->nNumOfElements == 0)) {
		/* make empty root directory for empty phar */
		efree(dir);
		return php_stream_alloc(&phar_dir_ops, data, NULL, "r");
	}
	zend_hash_internal_pointer_reset(manifest);
	while (FAILURE != zend_hash_has_more_elements(manifest)) {
		if (HASH_KEY_NON_EXISTANT == zend_hash_get_current_key_ex(manifest, &key, &keylen, &unused, 0, NULL)) {
			break;
		}
		if (keylen <= (uint)dirlen) {
			if (keylen < (uint)dirlen || !strncmp(key, dir, dirlen)) {
				if (SUCCESS != zend_hash_move_forward(manifest)) {
					break;
				}
				continue;
			}
		}
		if (*dir == '/') {
			/* root directory */
			if (NULL != (found = (char *) memchr(key, '/', keylen))) {
				/* the entry has a path separator and is a subdirectory */
				entry = (char *) safe_emalloc(found - key, 1, 1);
				memcpy(entry, key, found - key);
				keylen = found - key;
				entry[keylen] = '\0';
			} else {
				entry = (char *) safe_emalloc(keylen, 1, 1);
				memcpy(entry, key, keylen);
				entry[keylen] = '\0';
			}
			goto PHAR_ADD_ENTRY;
		} else {
			if (0 != memcmp(key, dir, dirlen)) {
				/* entry in directory not found */
				if (SUCCESS != zend_hash_move_forward(manifest)) {
					break;
				}
				continue;
			} else {
				if (key[dirlen] != '/') {
					if (SUCCESS != zend_hash_move_forward(manifest)) {
						break;
					}
					continue;
				}
			}
		}
		save = key;
		save += dirlen + 1; /* seek to just past the path separator */
		if (NULL != (found = (char *) memchr(save, '/', keylen - dirlen - 1))) {
			/* is subdirectory */
			save -= dirlen + 1;
			entry = (char *) safe_emalloc(found - save + dirlen, 1, 1);
			memcpy(entry, save + dirlen + 1, found - save - dirlen - 1);
			keylen = found - save - dirlen - 1;
			entry[keylen] = '\0';
		} else {
			/* is file */
			save -= dirlen + 1;
			entry = (char *) safe_emalloc(keylen - dirlen, 1, 1);
			memcpy(entry, save + dirlen + 1, keylen - dirlen - 1);
			entry[keylen - dirlen - 1] = '\0';
			keylen = keylen - dirlen - 1;
		}
PHAR_ADD_ENTRY:
		if (keylen) {
			phar_add_empty(data, entry, keylen);
		}
		efree(entry);
		if (SUCCESS != zend_hash_move_forward(manifest)) {
			break;
		}
	}
	if (FAILURE != zend_hash_has_more_elements(data)) {
		efree(dir);
		if (zend_hash_sort(data, zend_qsort, phar_compare_dir_name, 0 TSRMLS_CC) == FAILURE) {
			FREE_HASHTABLE(data);
			return NULL;
		}
		return php_stream_alloc(&phar_dir_ops, data, NULL, "r");
	} else {
		efree(dir);
		return php_stream_alloc(&phar_dir_ops, data, NULL, "r");
	}
}
/* }}}*/

/**
 * Open a directory handle within a phar archive
 */
php_stream *phar_wrapper_open_dir(php_stream_wrapper *wrapper, char *path, char *mode,
			int options, char **opened_path, php_stream_context *context STREAMS_DC TSRMLS_DC) /* {{{ */
{
	php_url *resource = NULL;
	php_stream *ret;
	char *internal_file, *key, *error, *plain_map;
	uint keylen;
	ulong unused;
	phar_archive_data *phar;
	phar_entry_info *entry = NULL;
	uint host_len;

	if ((resource = phar_open_url(wrapper, path, mode, options TSRMLS_CC)) == NULL) {
		return NULL;
	}

	/* we must have at the very least phar://alias.phar/ */
	if (!resource->scheme || !resource->host || !resource->path) {
		if (resource->host && !resource->path) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: no directory in \"%s\", must have at least phar://%s/ for root directory (always use full path to a new phar)", path, resource->host);
			php_url_free(resource);
			return NULL;
		}
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\", must have at least phar://%s/", path, path);
		return NULL;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar url \"%s\"", path);
		return NULL;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		spprintf(&internal_file, 0, "%s%s", plain_map, resource->path);
		ret = php_stream_opendir(internal_file, options, context);
		if (!ret) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: file \"%s\" extracted from \"%s\" could not be opened", internal_file, resource->host);
		}
		php_url_free(resource);
		efree(internal_file);
		return ret;
	}

	internal_file = resource->path + 1; /* strip leading "/" */
	if (FAILURE == phar_get_archive(&phar, resource->host, host_len, NULL, 0, &error TSRMLS_CC)) {
		if (error) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, error);
			efree(error);
		}
		php_url_free(resource);
		return NULL;
	}
	if (error) {
		efree(error);
	}
	if (*internal_file == '\0') {
		/* root directory requested */
		internal_file = estrndup(internal_file - 1, 1);
		ret = phar_make_dirstream(internal_file, &phar->manifest TSRMLS_CC);
		php_url_free(resource);
		return ret;
	}
	if (!phar->manifest.arBuckets) {
		php_url_free(resource);
		return NULL;
	}
	if (SUCCESS == zend_hash_find(&phar->manifest, internal_file, strlen(internal_file), (void**)&entry) && !entry->is_dir) {
		php_url_free(resource);
		return NULL;
	} else if (entry && entry->is_dir) {
		/*if (entry->is_mounted) {
			 external directory, TODO: construct an internal dirstream based on this actual dir's dirstream
			php_url_free(resource);
			return php_stream_opendir(entry->link, options, context);
		}*/
		internal_file = estrdup(internal_file);
		php_url_free(resource);
		return phar_make_dirstream(internal_file, &phar->manifest TSRMLS_CC);
	} else {
		int i_len = strlen(internal_file);

		/* search for directory */
		zend_hash_internal_pointer_reset(&phar->manifest);
		while (FAILURE != zend_hash_has_more_elements(&phar->manifest)) {
			if (HASH_KEY_NON_EXISTANT != 
					zend_hash_get_current_key_ex(
						&phar->manifest, &key, &keylen, &unused, 0, NULL)) {
				if (keylen > (uint)i_len && 0 == memcmp(key, internal_file, i_len)) {
					/* directory found */
					internal_file = estrndup(internal_file,
							i_len);
					php_url_free(resource);
					return phar_make_dirstream(internal_file, &phar->manifest TSRMLS_CC);
				}
			}
			if (SUCCESS != zend_hash_move_forward(&phar->manifest)) {
				break;
			}
		}
	}

	php_url_free(resource);
	return NULL;
}
/* }}} */

/**
 * Make a new directory within a phar archive
 */
int phar_wrapper_mkdir(php_stream_wrapper *wrapper, char *url_from, int mode, int options, php_stream_context *context TSRMLS_DC) /* {{{ */
{
	phar_entry_info entry, *e;
	phar_archive_data *phar = NULL;
	char *error, *arch, *entry2;
	int arch_len, entry_len;
	char *plain_map;
	php_url *resource = NULL;
	uint host_len;

	/* pre-readonly check, we need to know if this is a data phar */
	if (FAILURE == phar_split_fname(url_from, strlen(url_from), &arch, &arch_len, &entry2, &entry_len TSRMLS_CC)) {
		return FAILURE;
	}
	if (FAILURE == phar_get_archive(&phar, arch, arch_len, NULL, 0, NULL TSRMLS_CC)) {
		phar = NULL;
	}
	efree(arch);
	efree(entry2);
	if (PHAR_G(readonly) && (!phar || !phar->is_data)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\", write operations disabled", url_from);
		return FAILURE;
	}

	if ((resource = phar_open_url(wrapper, url_from, "w", options TSRMLS_CC)) == NULL) {
		return FAILURE;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource->scheme || !resource->host || !resource->path) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", url_from);
		return FAILURE;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", url_from);
		return FAILURE;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: directory \"%s\" cannot be created in phar \"%s\", phar is extracted in plain map", resource->path+1, resource->host);
		php_url_free(resource);
		return FAILURE;
	}

	if (FAILURE == phar_get_archive(&phar, resource->host, host_len, NULL, 0, &error TSRMLS_CC)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", error retrieving phar information: %s", resource->path+1, resource->host, error);
		efree(error);
		php_url_free(resource);
		return FAILURE;
	}

	if ((e = phar_get_entry_info_dir(phar, resource->path + 1, strlen(resource->path + 1), 1, &error TSRMLS_CC))) {
		/* directory exists, or is a subdirectory of an existing file */
		efree(e->filename);
		efree(e);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", directory already exists", resource->path+1, resource->host);
		php_url_free(resource);
		return FAILURE;
	}
	if (error) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", %s", resource->path+1, resource->host, error);
		efree(error);
		php_url_free(resource);
		return FAILURE;
	}

	memset((void *) &entry, 0, sizeof(phar_entry_info));

	/* strip leading "/" */
	if (phar->is_zip) {
		entry.is_zip = 1;
	}
	entry.filename = estrdup(resource->path + 1);
	if (phar->is_tar) {
		entry.is_tar = 1;
		entry.tar_type = TAR_DIR;
	}
	entry.filename_len = strlen(resource->path + 1);
	php_url_free(resource);
	entry.is_dir = 1;
	entry.phar = phar;
	entry.is_modified = 1;
	entry.is_crc_checked = 1;
	entry.flags = PHAR_ENT_PERM_DEF_DIR;
	entry.old_flags = PHAR_ENT_PERM_DEF_DIR;
	if (SUCCESS != zend_hash_add(&phar->manifest, entry.filename, entry.filename_len, (void*)&entry, sizeof(phar_entry_info), NULL)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", adding to manifest failed", entry.filename, phar->fname);
		efree(error);
		efree(entry.filename);
		return FAILURE;
	}
	phar_flush(phar, 0, 0, 0, &error TSRMLS_CC);
	if (error) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", %s", entry.filename, phar->fname, error);
		zend_hash_del(&phar->manifest, entry.filename, entry.filename_len);
		efree(error);
		efree(entry.filename);
		return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

/**
 * Remove a directory within a phar archive
 */
int phar_wrapper_rmdir(php_stream_wrapper *wrapper, char *url, int options, php_stream_context *context TSRMLS_DC) /* {{{ */
{
	phar_entry_info *entry;
	phar_archive_data *phar = NULL;
	char *error, *arch, *entry2;
	int arch_len, entry_len;
	char *plain_map;
	php_url *resource = NULL;
	uint host_len;

	/* pre-readonly check, we need to know if this is a data phar */
	if (FAILURE == phar_split_fname(url, strlen(url), &arch, &arch_len, &entry2, &entry_len TSRMLS_CC)) {
		return FAILURE;
	}
	if (FAILURE == phar_get_archive(&phar, arch, arch_len, NULL, 0, NULL TSRMLS_CC)) {
		phar = NULL;
	}
	efree(arch);
	efree(entry2);
	if (PHAR_G(readonly) && (!phar || !phar->is_data)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot rmdir directory \"%s\", write operations disabled", url);
		return FAILURE;
	}

	if ((resource = phar_open_url(wrapper, url, "w", options TSRMLS_CC)) == NULL) {
		return FAILURE;
	}

	/* we must have at the very least phar://alias.phar/internalfile.php */
	if (!resource->scheme || !resource->host || !resource->path) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: invalid url \"%s\"", url);
		return FAILURE;
	}

	if (strcasecmp("phar", resource->scheme)) {
		php_url_free(resource);
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: not a phar stream url \"%s\"", url);
		return FAILURE;
	}

	host_len = strlen(resource->host);
	phar_request_initialize(TSRMLS_C);
	if (zend_hash_find(&(PHAR_GLOBALS->phar_plain_map), resource->host, host_len+1, (void **)&plain_map) == SUCCESS) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: directory \"%s\" cannot be removed in phar \"%s\", phar is extracted in plain map", resource->path+1, resource->host);
		php_url_free(resource);
		return FAILURE;
	}

	if (FAILURE == phar_get_archive(&phar, resource->host, host_len, NULL, 0, &error TSRMLS_CC)) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot remove directory \"%s\" in phar \"%s\", error retrieving phar information: %s", resource->path+1, resource->host, error);
		efree(error);
		php_url_free(resource);
		return FAILURE;
	}

	if (!(entry = phar_get_entry_info_dir(phar, resource->path + 1, strlen(resource->path + 1), 1, &error TSRMLS_CC))) {
		if (error) {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot remove directory \"%s\" in phar \"%s\", %s", resource->path+1, resource->host, error);
			efree(error);
		} else {
			php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot remove directory \"%s\" in phar \"%s\", directory does not exist", resource->path+1, resource->host);
		}
		php_url_free(resource);
		return FAILURE;
	}

	/* now for the easy part */
	entry->is_deleted = 1;
	entry->is_modified = 1;
	phar_flush(phar, 0, 0, 0, &error TSRMLS_CC);
	if (error) {
		php_stream_wrapper_log_error(wrapper, options TSRMLS_CC, "phar error: cannot create directory \"%s\" in phar \"%s\", %s", entry->filename, phar->fname, error);
		zend_hash_del(&phar->manifest, entry->filename, entry->filename_len);
		php_url_free(resource);
		efree(error);
		return FAILURE;
	}
	php_url_free(resource);
	return SUCCESS;
}
/* }}} */
