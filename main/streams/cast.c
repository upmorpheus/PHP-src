/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2010 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Wez Furlong <wez@thebrainroom.com>                          |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#define _GNU_SOURCE
#include "php.h"
#include "php_globals.h"
#include "php_network.h"
#include "php_open_temporary_file.h"
#include "ext/standard/file.h"
#include <stddef.h>
#include <fcntl.h>

#include "php_streams_int.h"

/* Under BSD, emulate fopencookie using funopen */
#if defined(HAVE_FUNOPEN) && !defined(HAVE_FOPENCOOKIE)
typedef struct {
	int (*reader)(void *, char *, int);
	int (*writer)(void *, const char *, int);
	fpos_t (*seeker)(void *, fpos_t, int);
	int (*closer)(void *);
} COOKIE_IO_FUNCTIONS_T;

FILE *fopencookie(void *cookie, const char *mode, COOKIE_IO_FUNCTIONS_T *funcs)
{
	return funopen(cookie, funcs->reader, funcs->writer, funcs->seeker, funcs->closer);
}
# define HAVE_FOPENCOOKIE 1
# define PHP_EMULATE_FOPENCOOKIE 1
# define PHP_STREAM_COOKIE_FUNCTIONS	&stream_cookie_functions
#elif defined(HAVE_FOPENCOOKIE)
# define PHP_STREAM_COOKIE_FUNCTIONS	stream_cookie_functions
#endif

/* {{{ STDIO with fopencookie */
#if defined(PHP_EMULATE_FOPENCOOKIE)
/* use our fopencookie emulation */
static int stream_cookie_reader(void *cookie, char *buffer, int size)
{
	int ret;
	TSRMLS_FETCH();

	ret = php_stream_read((php_stream*)cookie, buffer, size);
	return ret;
}

static int stream_cookie_writer(void *cookie, const char *buffer, int size)
{
	TSRMLS_FETCH();

	return php_stream_write((php_stream *)cookie, (char *)buffer, size);
}

static fpos_t stream_cookie_seeker(void *cookie, off_t position, int whence)
{
	TSRMLS_FETCH();

	return (fpos_t)php_stream_seek((php_stream *)cookie, position, whence);
}

static int stream_cookie_closer(void *cookie)
{
	php_stream *stream = (php_stream*)cookie;
	TSRMLS_FETCH();

	/* prevent recursion */
	stream->fclose_stdiocast = PHP_STREAM_FCLOSE_NONE;
	return php_stream_close(stream);
}
#elif defined(HAVE_FOPENCOOKIE)
static ssize_t stream_cookie_reader(void *cookie, char *buffer, size_t size)
{
	ssize_t ret;
	TSRMLS_FETCH();

	ret = php_stream_read(((php_stream *)cookie), buffer, size);
	return ret;
}

static ssize_t stream_cookie_writer(void *cookie, const char *buffer, size_t size)
{
	TSRMLS_FETCH();

	return php_stream_write(((php_stream *)cookie), (char *)buffer, size);
}

# ifdef COOKIE_SEEKER_USES_OFF64_T
static int stream_cookie_seeker(void *cookie, __off64_t *position, int whence)
{
	TSRMLS_FETCH();

	*position = php_stream_seek((php_stream *)cookie, (off_t)*position, whence);

	if (*position == -1) {
		return -1;
	}
	return 0;
}
# else
static int stream_cookie_seeker(void *cookie, off_t position, int whence)
{
	TSRMLS_FETCH();

	return php_stream_seek((php_stream *)cookie, position, whence);
}
# endif

static int stream_cookie_closer(void *cookie)
{
	php_stream *stream = (php_stream*)cookie;
	TSRMLS_FETCH();

	/* prevent recursion */
	stream->fclose_stdiocast = PHP_STREAM_FCLOSE_NONE;
	return php_stream_close(stream);
}
#endif /* elif defined(HAVE_FOPENCOOKIE) */

#if HAVE_FOPENCOOKIE
static COOKIE_IO_FUNCTIONS_T stream_cookie_functions =
{
	stream_cookie_reader, stream_cookie_writer,
	stream_cookie_seeker, stream_cookie_closer
};
#else
/* TODO: use socketpair() to emulate fopencookie, as suggested by Hartmut ? */
#endif
/* }}} */

/* {{{ php_stream_cast */
PHPAPI int _php_stream_cast(php_stream *stream, int castas, void **ret, int show_err TSRMLS_DC)
{
	int flags = castas & PHP_STREAM_CAST_MASK;
	castas &= ~PHP_STREAM_CAST_MASK;

	/* synchronize our buffer (if possible) */
	if (ret && castas != PHP_STREAM_AS_FD_FOR_SELECT) {
		php_stream_flush(stream);
		if (stream->ops->seek && (stream->flags & PHP_STREAM_FLAG_NO_SEEK) == 0) {
			off_t dummy;

			stream->ops->seek(stream, stream->position, SEEK_SET, &dummy TSRMLS_CC);
			stream->readpos = stream->writepos = 0;
		}
	}

	/* filtered streams can only be cast as stdio, and only when fopencookie is present */

	if (castas == PHP_STREAM_AS_STDIO) {
		if (stream->stdiocast) {
			if (ret) {
				*(FILE**)ret = stream->stdiocast;
			}
			goto exit_success;
		}

		/* if the stream is a stdio stream let's give it a chance to respond
		 * first, to avoid doubling up the layers of stdio with an fopencookie */
		if (php_stream_is(stream, PHP_STREAM_IS_STDIO) &&
			stream->ops->cast &&
			!php_stream_is_filtered(stream) &&
			stream->ops->cast(stream, castas, ret TSRMLS_CC) == SUCCESS
		) {
			goto exit_success;
		}

#if HAVE_FOPENCOOKIE
		/* if just checking, say yes we can be a FILE*, but don't actually create it yet */
		if (ret == NULL) {
			goto exit_success;
		}

		*(FILE**)ret = fopencookie(stream, stream->mode, PHP_STREAM_COOKIE_FUNCTIONS);

		if (*ret != NULL) {
			off_t pos;

			stream->fclose_stdiocast = PHP_STREAM_FCLOSE_FOPENCOOKIE;

			/* If the stream position is not at the start, we need to force
			 * the stdio layer to believe it's real location. */
			pos = php_stream_tell(stream);
			if (pos > 0) {
				fseek(*ret, pos, SEEK_SET);
			}

			goto exit_success;
		}

		/* must be either:
			a) programmer error
			b) no memory
			-> lets bail
		*/
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "fopencookie failed");
		return FAILURE;
#endif

		if (!php_stream_is_filtered(stream) && stream->ops->cast && stream->ops->cast(stream, castas, NULL TSRMLS_CC) == SUCCESS) {
			if (FAILURE == stream->ops->cast(stream, castas, ret TSRMLS_CC)) {
				return FAILURE;
			}
			goto exit_success;
		} else if (flags & PHP_STREAM_CAST_TRY_HARD) {
			php_stream *newstream;

			newstream = php_stream_fopen_tmpfile();
			if (newstream) {
				int ret = php_stream_copy_to_stream_ex(stream, newstream, PHP_STREAM_COPY_ALL, NULL);

				if (ret != SUCCESS) {
					php_stream_close(newstream);
				} else {
					int retcode = php_stream_cast(newstream, castas | flags, (void **)ret, show_err);

					if (retcode == SUCCESS) {
						rewind(*(FILE**)ret);
					}

					/* do some specialized cleanup */
					if ((flags & PHP_STREAM_CAST_RELEASE)) {
						php_stream_free(stream, PHP_STREAM_FREE_CLOSE_CASTED);
					}

					return retcode;
				}
			}
		}
	}

	if (php_stream_is_filtered(stream)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot cast a filtered stream on this system");
		return FAILURE;
	} else if (stream->ops->cast && stream->ops->cast(stream, castas, ret TSRMLS_CC) == SUCCESS) {
		goto exit_success;
	}

	if (show_err) {
		/* these names depend on the values of the PHP_STREAM_AS_XXX defines in php_streams.h */
		static const char *cast_names[4] = {
			"STDIO FILE*",
			"File Descriptor",
			"Socket Descriptor",
			"select()able descriptor"
		};

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "cannot represent a stream of type %s as a %s", stream->ops->label, cast_names[castas]);
	}

	return FAILURE;

exit_success:

	if ((stream->writepos - stream->readpos) > 0 &&
		stream->fclose_stdiocast != PHP_STREAM_FCLOSE_FOPENCOOKIE &&
		(flags & PHP_STREAM_CAST_INTERNAL) == 0
	) {
		/* the data we have buffered will be lost to the third party library that
		 * will be accessing the stream.  Emit a warning so that the end-user will
		 * know that they should try something else */

		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%ld bytes of buffered data lost during stream conversion!", (long)(stream->writepos - stream->readpos));
	}

	if (castas == PHP_STREAM_AS_STDIO && ret) {
		stream->stdiocast = *(FILE**)ret;
	}

	if (flags & PHP_STREAM_CAST_RELEASE) {
		php_stream_free(stream, PHP_STREAM_FREE_CLOSE_CASTED);
	}

	return SUCCESS;

}
/* }}} */

/* {{{ php_stream_open_wrapper_as_file */
PHPAPI FILE * _php_stream_open_wrapper_as_file(char *path, char *mode, int options, char **opened_path STREAMS_DC TSRMLS_DC)
{
	FILE *fp = NULL;
	php_stream *stream = NULL;

	stream = php_stream_open_wrapper_rel(path, mode, options|STREAM_WILL_CAST, opened_path);

	if (stream == NULL) {
		return NULL;
	}

	if (php_stream_cast(stream, PHP_STREAM_AS_STDIO|PHP_STREAM_CAST_TRY_HARD|PHP_STREAM_CAST_RELEASE, (void**)&fp, REPORT_ERRORS) == FAILURE) {
		php_stream_close(stream);
		if (opened_path && *opened_path) {
			efree(*opened_path);
		}
		return NULL;
	}
	return fp;
}
/* }}} */

/* {{{ php_stream_make_seekable */
PHPAPI int _php_stream_make_seekable(php_stream *origstream, php_stream **newstream, int flags STREAMS_DC TSRMLS_DC)
{
	assert(newstream != NULL);

	*newstream = NULL;

	if (((flags & PHP_STREAM_FORCE_CONVERSION) == 0) && origstream->ops->seek != NULL) {
		*newstream = origstream;
		return PHP_STREAM_UNCHANGED;
	}

	/* Use a tmpfile and copy the old streams contents into it */

	if (flags & PHP_STREAM_PREFER_STDIO) {
		*newstream = php_stream_fopen_tmpfile();
	} else {
		*newstream = php_stream_temp_new();
	}

	if (*newstream == NULL) {
		return PHP_STREAM_FAILED;
	}

#if ZEND_DEBUG
	(*newstream)->open_filename = origstream->open_filename;
	(*newstream)->open_lineno = origstream->open_lineno;
#endif

	if (php_stream_copy_to_stream_ex(origstream, *newstream, PHP_STREAM_COPY_ALL, NULL) != SUCCESS) {
		php_stream_close(*newstream);
		*newstream = NULL;
		return PHP_STREAM_CRITICAL;
	}

	php_stream_close(origstream);
	php_stream_seek(*newstream, 0, SEEK_SET);

	return PHP_STREAM_RELEASED;
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
