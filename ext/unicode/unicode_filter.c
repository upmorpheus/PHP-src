/*
   +----------------------------------------------------------------------+
   | PHP Version 6                                                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sara Golemon (pollita@php.net)                              |
   +----------------------------------------------------------------------+
*/

/* $Id$ */


#include "php.h"
#include <unicode/ucnv.h>

/* {{{ data structure */
typedef struct _php_unicode_filter_data {
	char is_persistent;
	UConverter *conv;

	char to_unicode;
} php_unicode_filter_data;
/* }}} */

/* {{{ unicode.* filter implementation */

/* unicode.to.* -- Expects String -- Returns Unicode */
static php_stream_filter_status_t php_unicode_to_string_filter(
	php_stream *stream,
	php_stream_filter *thisfilter,
	php_stream_bucket_brigade *buckets_in,
	php_stream_bucket_brigade *buckets_out,
	size_t *bytes_consumed,
	int flags
	TSRMLS_DC)
{
	php_unicode_filter_data *data;
	php_stream_filter_status_t exit_status = PSFS_FEED_ME;
	size_t consumed = 0;

	if (!thisfilter || !thisfilter->abstract) {
		/* Should never happen */
		return PSFS_ERR_FATAL;
	}

	data = (php_unicode_filter_data *)(thisfilter->abstract);
	while (buckets_in->head) {
		php_stream_bucket *bucket = buckets_in->head;
		UChar *src = bucket->buf.ustr.val;

		php_stream_bucket_unlink(bucket TSRMLS_CC);
		if (!bucket->is_unicode) {
			/* Already ASCII, can't really do anything with it */
			consumed += bucket->buf.str.len;
			php_stream_bucket_append(buckets_out, bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
			continue;
		}

		while (src < (bucket->buf.ustr.val + bucket->buf.ustr.len)) {
			int remaining = bucket->buf.ustr.len - (src - bucket->buf.ustr.val);
			char *destp, *destbuf;
			int32_t destlen = UCNV_GET_MAX_BYTES_FOR_STRING(remaining, ucnv_getMaxCharSize(data->conv));
			UErrorCode errCode = U_ZERO_ERROR;
			php_stream_bucket *new_bucket;

			destp = destbuf = (char *)pemalloc(destlen, data->is_persistent);

			ucnv_fromUnicode(data->conv, &destp, destbuf + destlen, (const UChar**)&src, src + remaining, NULL, FALSE, &errCode);
			new_bucket = php_stream_bucket_new(stream, destbuf, destp - destbuf, 1, data->is_persistent TSRMLS_CC);
			php_stream_bucket_append(buckets_out, new_bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
		}
		consumed += UBYTES(bucket->buf.ustr.len);
		php_stream_bucket_delref(bucket TSRMLS_CC);
	}

	if (flags & PSFS_FLAG_FLUSH_CLOSE) {
		UErrorCode errCode = U_ZERO_ERROR;
		char d[64], *dest = d, *destp = d + 64;
		/* Spit it out! */

		ucnv_fromUnicode(data->conv, &dest, destp, NULL, NULL, NULL, TRUE, &errCode);
		if (dest > d) {
			php_stream_bucket *bucket = php_stream_bucket_new(stream, d, dest - d, 0, 0 TSRMLS_CC);
			php_stream_bucket_append(buckets_out, bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
		}
	}

	if (bytes_consumed) {
		*bytes_consumed = consumed;
	}

	return exit_status;
}

/* unicode.from.* -- Expects Unicode -- Returns String */
static php_stream_filter_status_t php_unicode_from_string_filter(
	php_stream *stream,
	php_stream_filter *thisfilter,
	php_stream_bucket_brigade *buckets_in,
	php_stream_bucket_brigade *buckets_out,
	size_t *bytes_consumed,
	int flags
	TSRMLS_DC)
{
	php_unicode_filter_data *data;
	php_stream_filter_status_t exit_status = PSFS_FEED_ME;
	size_t consumed = 0;

	if (!thisfilter || !thisfilter->abstract) {
		/* Should never happen */
		return PSFS_ERR_FATAL;
	}

	data = (php_unicode_filter_data *)(thisfilter->abstract);
	while (buckets_in->head) {
		php_stream_bucket *bucket = buckets_in->head;
		char *src = bucket->buf.str.val;

		php_stream_bucket_unlink(bucket TSRMLS_CC);
		if (bucket->is_unicode) {
			/* already in unicode, nothing to do */
			consumed += UBYTES(bucket->buf.ustr.len);
			php_stream_bucket_append(buckets_out, bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
			continue;
		}

		while (src < (bucket->buf.str.val + bucket->buf.str.len)) {
			int remaining = bucket->buf.str.len - (src - bucket->buf.str.val);
			UChar *destp, *destbuf;
			int32_t destlen = UCNV_GET_MAX_BYTES_FOR_STRING(remaining, ucnv_getMaxCharSize(data->conv));
			UErrorCode errCode = U_ZERO_ERROR;
			php_stream_bucket *new_bucket;

			destp = destbuf = (UChar *)pemalloc(destlen, data->is_persistent);

			ucnv_toUnicode(data->conv, &destp, destbuf + destlen, (const char**)&src, src + remaining, NULL, FALSE, &errCode);

			new_bucket = php_stream_bucket_new_unicode(stream, destbuf, destp - destbuf, 1, data->is_persistent TSRMLS_CC);
			php_stream_bucket_append(buckets_out, new_bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
		}
		consumed += bucket->buf.str.len;
		php_stream_bucket_delref(bucket TSRMLS_CC);
	}

	if (flags & PSFS_FLAG_FLUSH_CLOSE) {
		UErrorCode errCode = U_ZERO_ERROR;
		UChar d[64], *dest = d, *destp = d + 64;
		/* Spit it out! */

		ucnv_toUnicode(data->conv, &dest, destp, NULL, NULL, NULL, TRUE, &errCode);
		if (dest > d) {
			php_stream_bucket *bucket = php_stream_bucket_new_unicode(stream, d, dest - d, 0, 0 TSRMLS_CC);
			php_stream_bucket_append(buckets_out, bucket TSRMLS_CC);
			exit_status = PSFS_PASS_ON;
		}
	}

	if (bytes_consumed) {
		*bytes_consumed = consumed;
	}

	return exit_status;
}

/* unicode.tidy.* -- Expects anything -- Returns whatever is preferred by subsequent filters
   Can be used to "magically" fix-up bucket messes */
static php_stream_filter_status_t php_unicode_tidy_filter(
	php_stream *stream,
	php_stream_filter *thisfilter,
	php_stream_bucket_brigade *buckets_in,
	php_stream_bucket_brigade *buckets_out,
	size_t *bytes_consumed,
	int flags
	TSRMLS_DC)
{
	php_unicode_filter_data *data;
	int prefer_unicode = php_stream_filter_output_prefer_unicode(thisfilter);

	if (!thisfilter || !thisfilter->abstract) {
		/* Should never happen */
		return PSFS_ERR_FATAL;
	}

	data = (php_unicode_filter_data *)(thisfilter->abstract);

	if (prefer_unicode) {
		if (!data->to_unicode) {
			ucnv_resetToUnicode(data->conv);
			data->to_unicode = prefer_unicode;
		}
		return php_unicode_from_string_filter(stream, thisfilter, buckets_in, buckets_out, bytes_consumed, flags TSRMLS_CC);
	} else {
		if (data->to_unicode) {
			ucnv_resetFromUnicode(data->conv);
			data->to_unicode = prefer_unicode;
		}
		return php_unicode_to_string_filter(stream, thisfilter, buckets_in, buckets_out, bytes_consumed, flags TSRMLS_CC);
	}
}

static void php_unicode_filter_dtor(php_stream_filter *thisfilter TSRMLS_DC)
{
	if (thisfilter && thisfilter->abstract) {
		php_unicode_filter_data *data = (php_unicode_filter_data *)thisfilter->abstract;
		ucnv_close(data->conv);
		pefree(data, data->is_persistent);
	}
}

static php_stream_filter_ops php_unicode_to_string_filter_ops = {
	php_unicode_to_string_filter,
	php_unicode_filter_dtor,
	"unicode.to.*",
	PSFO_FLAG_ACCEPTS_UNICODE | PSFO_FLAG_OUTPUTS_STRING
};

static php_stream_filter_ops php_unicode_from_string_filter_ops = {
	php_unicode_from_string_filter,
	php_unicode_filter_dtor,
	"unicode.from.*",
	PSFO_FLAG_ACCEPTS_STRING | PSFO_FLAG_OUTPUTS_UNICODE
};

static php_stream_filter_ops php_unicode_tidy_filter_ops = {
	php_unicode_tidy_filter,
	php_unicode_filter_dtor,
	"unicode.tidy.*",
	PSFO_FLAG_ACCEPTS_ANY | PSFO_FLAG_OUTPUTS_ANY
};
/* }}} */


/* {{{ unicode.* factory */

static php_stream_filter *php_unicode_filter_create(const char *filtername, zval *filterparams, int persistent TSRMLS_DC)
{
	php_unicode_filter_data *data;
	const char *charset, *direction;
	php_stream_filter_ops *fops;
	UErrorCode ucnvError = U_ZERO_ERROR;
	char to_unicode = 0;

	if (strncasecmp(filtername, "unicode.", sizeof("unicode.") - 1)) {
		/* Never happens */
		return NULL;
	}

	direction = filtername + sizeof("unicode.") - 1;
	if (strncmp(direction, "to.", sizeof("to.") - 1) == 0) {
		fops = &php_unicode_to_string_filter_ops;
		charset = direction + sizeof("to.") - 1;
	} else if (strncmp(direction, "from.", sizeof("from.") - 1) == 0) {
		fops = &php_unicode_from_string_filter_ops;
		to_unicode = 1;
		charset = direction + sizeof("from.") - 1;
	} else if (strncmp(direction, "tidy.", sizeof("tidy.") - 1) == 0) {
		fops = &php_unicode_tidy_filter_ops;
		charset = direction + sizeof("tidy.") - 1;
	} else if (strcmp(direction, "tidy") == 0) {
		fops = &php_unicode_tidy_filter_ops;
		charset = "utf8";
	} else {
		/* Shouldn't happen */
		return NULL;
	}

	/* Create this filter */
	data = (php_unicode_filter_data *)pecalloc(1, sizeof(php_unicode_filter_data), persistent);
	if (!data) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed allocating %d bytes.", sizeof(php_unicode_filter_data));
		return NULL;
	}

	data->conv = ucnv_open(charset, &ucnvError);
	data->to_unicode = to_unicode;
	if (!data->conv) {
		char *reason = "Unknown Error";
		pefree(data, persistent);
		switch (ucnvError) {
			case U_MEMORY_ALLOCATION_ERROR:
				reason = "unable to allocate memory";
				break;
			case U_FILE_ACCESS_ERROR:
				reason = "file access error";
				break;
			default:
				;
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to open charset converter, %s", reason);
		return NULL;
	}

	return php_stream_filter_alloc(fops, data, persistent);
}

php_stream_filter_factory php_unicode_filter_factory = {
	php_unicode_filter_create
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

