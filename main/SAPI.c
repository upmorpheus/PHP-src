/* 
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Original design:  Shane Caraveo <shane@caraveo.com>                  |
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#include <ctype.h>
#include <sys/stat.h>

#include "php.h"
#include "SAPI.h"
#ifdef ZTS
#include "TSRM.h"
#endif

#include "rfc1867.h"

#ifdef PHP_WIN32
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif


SAPI_POST_READER_FUNC(sapi_read_standard_form_data);

static sapi_post_entry supported_post_entries[] = {
#if HAVE_FDFLIB
	{ "application/vnd.fdf",	sizeof("application/vnd.fdf")-1,	sapi_read_standard_form_data },
#endif
	{ NULL, 0, NULL }
};


static HashTable known_post_content_types;

SAPI_API void (*sapi_error)(int error_type, const char *message, ...);


#ifdef ZTS
SAPI_API int sapi_globals_id;
#else
sapi_globals_struct sapi_globals;
#endif


/* True globals (no need for thread safety) */
sapi_module_struct sapi_module;
SAPI_API void (*sapi_error)(int error_type, const char *message, ...);


SAPI_API void sapi_startup(sapi_module_struct *sf)
{
	sapi_module = *sf;
	zend_hash_init(&known_post_content_types, 5, NULL, NULL, 1);

	sapi_register_post_entries(supported_post_entries);

#ifdef ZTS
	sapi_globals_id = ts_allocate_id(sizeof(sapi_globals_struct), NULL, NULL);
#endif
	reentrancy_startup();

	php_global_startup_internal_extensions();
}

SAPI_API void sapi_shutdown(void)
{
	reentrancy_shutdown();
	php_global_shutdown_internal_extensions();
	zend_hash_destroy(&known_post_content_types);
}


SAPI_API void sapi_free_header(sapi_header_struct *sapi_header)
{
	efree(sapi_header->header);
}


SAPI_API void sapi_handle_post(void *arg SLS_DC)
{
	if (SG(request_info).post_entry) {
		SG(request_info).post_entry->post_handler(SG(request_info).content_type_dup, arg SLS_CC);
		efree(SG(request_info).post_data);
		SG(request_info).post_data = NULL;
		efree(SG(request_info).content_type_dup);
		SG(request_info).content_type_dup = NULL;
	}
}

static void sapi_read_post_data(SLS_D)
{
	sapi_post_entry *post_entry;
	uint content_type_length = strlen(SG(request_info).content_type);
	char *content_type = estrndup(SG(request_info).content_type, content_type_length);
	char *p;
	char oldchar=0;
	void (*post_reader_func)(SLS_D);


	/* dedicated implementation for increased performance:
	 * - Make the content type lowercase
	 * - Trim descriptive data, stay with the content-type only
	 */
	for (p=content_type; p<content_type+content_type_length; p++) {
		switch (*p) {
			case ';':
			case ',':
			case ' ':
				content_type_length = p-content_type;
				oldchar = *p;
				*p = 0;
				break;
			default:
				*p = tolower(*p);
				break;
		}
	}

	if (zend_hash_find(&known_post_content_types, content_type, content_type_length+1, (void **) &post_entry)==SUCCESS) {
		SG(request_info).post_entry = post_entry;
		post_reader_func = post_entry->post_reader;
	} else {
		if (!sapi_module.default_post_reader) {
			sapi_module.sapi_error(E_COMPILE_ERROR, "Unsupported content type:  '%s'", content_type);
			return;
		}
		SG(request_info).post_entry = NULL;
		post_reader_func = sapi_module.default_post_reader;
	}
	if (oldchar) {
		*(p-1) = oldchar;
	}
	post_reader_func(SLS_C);
	SG(request_info).content_type_dup = content_type;
}


SAPI_POST_READER_FUNC(sapi_read_standard_form_data)
{
	int read_bytes, total_read_bytes=0;
	int allocated_bytes=SAPI_POST_BLOCK_SIZE+1;

	SG(request_info).post_data = emalloc(allocated_bytes);

	for (;;) {
		read_bytes = sapi_module.read_post(SG(request_info).post_data+total_read_bytes, SAPI_POST_BLOCK_SIZE SLS_CC);
		if (read_bytes<=0) {
			break;
		}
		total_read_bytes += read_bytes;
		if (read_bytes < SAPI_POST_BLOCK_SIZE) {
			break;
		}
		if (total_read_bytes+SAPI_POST_BLOCK_SIZE >= allocated_bytes) {
			allocated_bytes = total_read_bytes+SAPI_POST_BLOCK_SIZE+1;
			SG(request_info).post_data = erealloc(SG(request_info).post_data, allocated_bytes);
		}
	}
	SG(request_info).post_data[total_read_bytes] = 0;  /* terminating NULL */
	SG(request_info).post_data_length = total_read_bytes;
}


SAPI_API char *sapi_get_default_content_type(SLS_D)
{
	char *mimetype, *charset, *content_type;

	mimetype = SG(default_mimetype) ? SG(default_mimetype) : SAPI_DEFAULT_MIMETYPE;
	charset = SG(default_charset) ? SG(default_charset) : SAPI_DEFAULT_CHARSET;

	if (strncasecmp(mimetype, "text/", 5) == 0 && strcasecmp(charset, "none") != 0) {
		int len = strlen(mimetype) + sizeof(";charset=") + strlen(charset);
		content_type = emalloc(len);
		strlcpy(content_type, mimetype, len);
		strlcat(content_type, ";charset=", len);
		strlcat(content_type, charset, len);
	} else {
		content_type = estrdup(mimetype);
	}
	return content_type;
}

/*
 * Add charset on content-type header if the MIME type starts with
 * "text/", the default_charset directive is not set to "none" and
 * there is not already a charset option in there.
 *
 * If "mimetype" is non-NULL, it should point to a pointer allocated
 * with emalloc().  If a charset is added, the string will be
 * re-allocated and the new length is returned.  If mimetype is
 * unchanged, 0 is returned.
 *
 */
SAPI_API size_t sapi_apply_default_charset(char **mimetype, size_t len SLS_DC)
{
	char *charset, *newtype;
	int newlen;
	charset = SG(default_charset) ? SG(default_charset) : SAPI_DEFAULT_CHARSET;

	if (strcasecmp(charset, "none") != 0 && strncmp(*mimetype, "text/", 5) == 0 && strstr(*mimetype, "charset=") == NULL) {
		newlen = len + (sizeof(";charset=")-1) + strlen(charset);
		newtype = emalloc(newlen + 1);
		strlcpy(newtype, *mimetype, len);
		strlcat(newtype, ";charset=", len);
		if (*mimetype != NULL) {
			efree(*mimetype);
		}
		*mimetype = newtype;
		return newlen;
	}
	return 0;
}


/*
 * Called from php_request_startup() for every request.
 */
SAPI_API void sapi_activate(SLS_D)
{
	zend_llist_init(&SG(sapi_headers).headers, sizeof(sapi_header_struct), (void (*)(void *)) sapi_free_header, 0);
	SG(sapi_headers).send_default_content_type = 1;

	SG(sapi_headers).http_response_code = 200;
	SG(sapi_headers).http_status_line = NULL;
	SG(headers_sent) = 0;
	SG(read_post_bytes) = 0;
	SG(request_info).post_data = NULL;
	SG(request_info).current_user = NULL;
	SG(request_info).current_user_length = 0;

	if (SG(request_info).request_method && !strcmp(SG(request_info).request_method, "HEAD")) {
		SG(request_info).headers_only = 1;
	} else {
		SG(request_info).headers_only = 0;
	}

	if (SG(server_context)) {
		if (SG(request_info).request_method 
			&& !strcmp(SG(request_info).request_method, "POST")) {
			if (!SG(request_info).content_type) {
				sapi_module.sapi_error(E_COMPILE_ERROR, "No content-type in POST request");
			}
			sapi_read_post_data(SLS_C);
		} else {
			SG(request_info).content_type_dup = NULL;
		}
		SG(request_info).cookie_data = sapi_module.read_cookies(SLS_C);
		if (sapi_module.activate) {
			sapi_module.activate(SLS_C);
		}
	}
}


SAPI_API void sapi_deactivate(SLS_D)
{
	zend_llist_destroy(&SG(sapi_headers).headers);
	if (SG(request_info).post_data) {
		efree(SG(request_info).post_data);
	}
	if (SG(request_info).auth_user) {
		efree(SG(request_info).auth_user);
	}
	if (SG(request_info).auth_password) {
		efree(SG(request_info).auth_password);
	}
	if (SG(request_info).content_type_dup) {
		efree(SG(request_info).content_type_dup);
	}
	if (SG(request_info).current_user) {
		efree(SG(request_info).current_user);
	}
	if (SG(sapi_headers).default_content_type) {
		efree(SG(sapi_headers).default_content_type);
	}
	if (sapi_module.deactivate) {
		sapi_module.deactivate(SLS_C);
	}
}


SAPI_API void sapi_initialize_empty_request(SLS_D)
{
	SG(server_context) = NULL;
	SG(request_info).request_method = NULL;
	SG(request_info).auth_user = SG(request_info).auth_password = NULL;
	SG(request_info).content_type_dup = NULL;
}


static int sapi_extract_response_code(const char *header_line)
{
	int code = 200;
	const char *ptr;

	for (ptr = header_line; *ptr; ptr++) {
		if (*ptr == ' ' && *(ptr + 1) != ' ') {
			code = atoi(ptr + 1);
			break;
		}
	}
	
	return code;
}

/* This function expects a *duplicated* string, that was previously emalloc()'d.
 * Pointers sent to this functions will be automatically freed by the framework.
 */
SAPI_API int sapi_add_header(char *header_line, uint header_line_len)
{
	int retval, free_header = 0;
	sapi_header_struct sapi_header;
	char *colon_offset;
	SLS_FETCH();

	if (SG(headers_sent)) {
		char *output_start_filename = php_get_output_start_filename();
		int output_start_lineno = php_get_output_start_lineno();

		if (output_start_filename) {
			sapi_module.sapi_error(E_WARNING, "Cannot add header information - headers already sent by (output started at %s:%d)",
				output_start_filename, output_start_lineno);
		} else {
			sapi_module.sapi_error(E_WARNING, "Cannot add header information - headers already sent");
		}
		efree(header_line);
		return FAILURE;
	}

	sapi_header.header = header_line;
	sapi_header.header_len = header_line_len;

	/* Check the header for a few cases that we have special support for in SAPI */
	if (header_line_len>=5 
		&& !memcmp(header_line, "HTTP/", 5)) {
		/* filter out the response code */
		SG(sapi_headers).http_response_code = sapi_extract_response_code(header_line);
		SG(sapi_headers).http_status_line = header_line;
		return SUCCESS;
	} else {
		colon_offset = strchr(header_line, ':');
		if (colon_offset) {
			*colon_offset = 0;
			if (!STRCASECMP(header_line, "Content-Type")) {
				char *ptr = colon_offset, *mimetype = NULL, *newheader;
				size_t len = header_line_len - (ptr - header_line), newlen;
				while (*ptr == ' ' && *ptr != '\0') {
					ptr++;
				}
				mimetype = estrdup(ptr);
				newlen = sapi_apply_default_charset(&mimetype, len SLS_CC);
				if (newlen != 0) {
					newlen += sizeof("Content-type: ");
					newheader = emalloc(newlen);
					strlcpy(newheader, "Content-type: ", newlen);
					strlcpy(newheader, mimetype, newlen);
					sapi_header.header = newheader;
					sapi_header.header_len = newlen - 1;
					colon_offset = strchr(newheader, ':');
					*colon_offset = '\0';
					free_header = 1;
				}
				efree(mimetype);
				SG(sapi_headers).send_default_content_type = 0;
			} else if (!STRCASECMP(header_line, "Location")) {
				SG(sapi_headers).http_response_code = 302; /* redirect */
			} else if (!STRCASECMP(header_line, "WWW-Authenticate")) { /* HTTP Authentication */
				SG(sapi_headers).http_response_code = 401; /* authentication-required */
			}
			*colon_offset = ':';
		}
	}

	if (sapi_module.header_handler) {
		retval = sapi_module.header_handler(&sapi_header, &SG(sapi_headers) SLS_CC);
	} else {
		retval = SAPI_HEADER_ADD;
	}
	if (retval & SAPI_HEADER_DELETE_ALL) {
		zend_llist_clean(&SG(sapi_headers).headers);
	}
	if (retval & SAPI_HEADER_ADD) {
		zend_llist_add_element(&SG(sapi_headers).headers, (void *) &sapi_header);
	}
	if (free_header) {
		efree(sapi_header.header);
	}
	return SUCCESS;
}


SAPI_API int sapi_send_headers()
{
	int retval;
	int ret = FAILURE;
	SLS_FETCH();

	if (SG(headers_sent)) {
		return SUCCESS;
	}

	if (sapi_module.send_headers) {
		retval = sapi_module.send_headers(&SG(sapi_headers) SLS_CC);
	} else {
		retval = SAPI_HEADER_DO_SEND;
	}

	switch (retval) {
		case SAPI_HEADER_SENT_SUCCESSFULLY:
			SG(headers_sent) = 1;
			ret = SUCCESS;
			break;
		case SAPI_HEADER_DO_SEND:
			if (SG(sapi_headers).http_status_line) {
				sapi_header_struct http_status_line;

				http_status_line.header = SG(sapi_headers).http_status_line;
				http_status_line.header_len = strlen(SG(sapi_headers).http_status_line);
				sapi_module.send_header(&http_status_line, SG(server_context));
			}
			zend_llist_apply_with_argument(&SG(sapi_headers).headers, (void (*)(void *, void *)) sapi_module.send_header, SG(server_context));
			if(SG(sapi_headers).send_default_content_type) {
				if (SG(sapi_headers).default_content_type != NULL) {
					sapi_header_struct default_header;
					int len = SG(sapi_headers).default_content_type_size + sizeof("Content-type: ");

					default_header.header = emalloc(len);
					strcpy(default_header.header, "Content-type: ");
					strlcat(default_header.header, SG(sapi_headers).default_content_type, len);
					default_header.header[len - 1] = '\0';
					default_header.header_len = len - 1;
					sapi_module.send_header(&default_header,SG(server_context));
					efree(default_header.header);
				} else {
					sapi_header_struct default_header = { SAPI_DEFAULT_CONTENT_TYPE_HEADER, sizeof(SAPI_DEFAULT_CONTENT_TYPE_HEADER) - 1 };
					sapi_module.send_header(&default_header,SG(server_context));
				}
			}
			sapi_module.send_header(NULL, SG(server_context));
			SG(headers_sent) = 1;
			ret = SUCCESS;
			break;
		case SAPI_HEADER_SEND_FAILED:
			ret = FAILURE;
			break;
	}
	
	if (SG(sapi_headers).http_status_line) {
		efree(SG(sapi_headers).http_status_line);
	}
	
	return ret;
}


SAPI_API int sapi_register_post_entries(sapi_post_entry *post_entries)
{
	sapi_post_entry *p=post_entries;

	while (p->content_type) {
		if (sapi_register_post_entry(p)==FAILURE) {
			return FAILURE;
		}
		p++;
	}
	return SUCCESS;
}


SAPI_API int sapi_register_post_entry(sapi_post_entry *post_entry)
{
	return zend_hash_add(&known_post_content_types, post_entry->content_type, post_entry->content_type_len+1, (void *) post_entry, sizeof(sapi_post_entry), NULL);
}


SAPI_API void sapi_unregister_post_entry(sapi_post_entry *post_entry)
{
	zend_hash_del(&known_post_content_types, post_entry->content_type, post_entry->content_type_len+1);
}


SAPI_API int sapi_register_default_post_reader(void (*default_post_reader)(SLS_D))
{
	sapi_module.default_post_reader = default_post_reader;
	return SUCCESS;
}


SAPI_API int sapi_flush()
{
	if (sapi_module.flush) {
		SLS_FETCH();

		sapi_module.flush(SG(server_context));
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

SAPI_API struct stat *sapi_get_stat()
{
	SLS_FETCH();

	if (sapi_module.get_stat) {
		return sapi_module.get_stat(SLS_C);
	} else {
		if (!SG(request_info).path_translated || (stat(SG(request_info).path_translated, &SG(global_stat))==-1)) {
			return NULL;
		}
		return &SG(global_stat);
	}
}


SAPI_API char *sapi_getenv(char *name, int name_len)
{
	if (sapi_module.getenv) {
		SLS_FETCH();

		return sapi_module.getenv(name, name_len SLS_CC);
	} else {
		return NULL;
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
