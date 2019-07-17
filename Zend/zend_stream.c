/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Wez Furlong <wez@thebrainroom.com>                          |
   |          Scott MacVicar <scottmac@php.net>                           |
   |          Nuno Lopes <nlopess@php.net>                                |
   |          Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend.h"
#include "zend_compile.h"
#include "zend_stream.h"

#ifndef S_ISREG
# define S_ISREG(m) 1
#endif

ZEND_DLIMPORT int isatty(int fd);

static size_t zend_stream_stdio_reader(void *handle, char *buf, size_t len) /* {{{ */
{
	return fread(buf, 1, len, (FILE*)handle);
} /* }}} */

static void zend_stream_stdio_closer(void *handle) /* {{{ */
{
	if (handle && (FILE*)handle != stdin) {
		fclose((FILE*)handle);
	}
} /* }}} */

ZEND_API void zend_stream_init_fp(zend_file_handle *handle, FILE *fp, const char *filename) {
	memset(handle, 0, sizeof(zend_file_handle));
	handle->type = ZEND_HANDLE_FP;
	handle->handle.fp = fp;
	handle->filename = filename;
}

ZEND_API void zend_stream_init_filename(zend_file_handle *handle, const char *filename) {
	memset(handle, 0, sizeof(zend_file_handle));
	handle->type = ZEND_HANDLE_FILENAME;
	handle->filename = filename;
}

ZEND_API int zend_stream_open(const char *filename, zend_file_handle *handle) /* {{{ */
{
	zend_string *opened_path;
	if (zend_stream_open_function) {
		return zend_stream_open_function(filename, handle);
	}

	zend_stream_init_fp(handle, zend_fopen(filename, &opened_path), filename);
	handle->opened_path = opened_path;
	return handle->handle.fp ? SUCCESS : FAILURE;
} /* }}} */

static int zend_stream_getc(zend_file_handle *file_handle) /* {{{ */
{
	char buf;

	if (file_handle->handle.stream.reader(file_handle->handle.stream.handle, &buf, sizeof(buf))) {
		return (int)buf;
	}
	return EOF;
} /* }}} */

static size_t zend_stream_read(zend_file_handle *file_handle, char *buf, size_t len) /* {{{ */
{
	if (file_handle->handle.stream.isatty) {
		int c = '*';
		size_t n;

		for (n = 0; n < len && (c = zend_stream_getc(file_handle)) != EOF && c != '\n'; ++n)  {
			buf[n] = (char)c;
		}
		if (c == '\n') {
			buf[n++] = (char)c;
		}

		return n;
	}
	return file_handle->handle.stream.reader(file_handle->handle.stream.handle, buf, len);
} /* }}} */

ZEND_API int zend_stream_fixup(zend_file_handle *file_handle, char **buf, size_t *len) /* {{{ */
{
	size_t size = 0;

	if (file_handle->buf) {
		*buf = file_handle->buf;
		*len = file_handle->len;
		return SUCCESS;
	}

	if (file_handle->type == ZEND_HANDLE_FILENAME) {
		if (zend_stream_open(file_handle->filename, file_handle) == FAILURE) {
			return FAILURE;
		}
	}

	if (file_handle->type == ZEND_HANDLE_FP) {
		FILE *fp = file_handle->handle.fp;
		int is_tty;
		if (!fp) {
			return FAILURE;
		}

		is_tty = isatty(fileno(fp));
		if (!is_tty) {
			zend_stat_t buf;
			if (zend_fstat(fileno(fp), &buf) == 0 && S_ISREG(buf.st_mode)) {
				size = buf.st_size;
			}
		}

		file_handle->type = ZEND_HANDLE_STREAM;
		file_handle->handle.stream.handle = fp;
		file_handle->handle.stream.isatty = is_tty;
		file_handle->handle.stream.reader = (zend_stream_reader_t)zend_stream_stdio_reader;
		file_handle->handle.stream.closer = (zend_stream_closer_t)zend_stream_stdio_closer;
	}

	if (size) {
		file_handle->buf = *buf = safe_emalloc(1, size, ZEND_MMAP_AHEAD);
		file_handle->len = zend_stream_read(file_handle, *buf, size);
	} else {
		size_t read, remain = 4*1024;
		*buf = emalloc(remain);
		size = 0;

		while ((read = zend_stream_read(file_handle, *buf + size, remain)) > 0) {
			size   += read;
			remain -= read;

			if (remain == 0) {
				*buf   = safe_erealloc(*buf, size, 2, 0);
				remain = size;
			}
		}
		file_handle->len = size;
		if (size && remain < ZEND_MMAP_AHEAD) {
			*buf = safe_erealloc(*buf, size, 1, ZEND_MMAP_AHEAD);
		}
		file_handle->buf = *buf;
	}

	if (file_handle->len == 0) {
		*buf = erealloc(*buf, ZEND_MMAP_AHEAD);
		file_handle->buf = *buf;
	}

	memset(file_handle->buf + file_handle->len, 0, ZEND_MMAP_AHEAD);

	*buf = file_handle->buf;
	*len = file_handle->len;

	return SUCCESS;
} /* }}} */

ZEND_API void zend_file_handle_dtor(zend_file_handle *fh) /* {{{ */
{
	switch (fh->type) {
		case ZEND_HANDLE_FP:
			fclose(fh->handle.fp);
			break;
		case ZEND_HANDLE_STREAM:
			if (fh->handle.stream.closer && fh->handle.stream.handle) {
				fh->handle.stream.closer(fh->handle.stream.handle);
			}
			fh->handle.stream.handle = NULL;
			break;
		case ZEND_HANDLE_FILENAME:
			/* We're only supposed to get here when destructing the used_files hash,
			 * which doesn't really contain open files, but references to their names/paths
			 */
			break;
	}
	if (fh->opened_path) {
		zend_string_release_ex(fh->opened_path, 0);
		fh->opened_path = NULL;
	}
	if (fh->buf) {
		efree(fh->buf);
		fh->buf = NULL;
	}
}
/* }}} */

ZEND_API int zend_compare_file_handles(zend_file_handle *fh1, zend_file_handle *fh2) /* {{{ */
{
	if (fh1->type != fh2->type) {
		return 0;
	}
	switch (fh1->type) {
		case ZEND_HANDLE_FP:
			return fh1->handle.fp == fh2->handle.fp;
		case ZEND_HANDLE_STREAM:
			return fh1->handle.stream.handle == fh2->handle.stream.handle;
		default:
			return 0;
	}
	return 0;
} /* }}} */
