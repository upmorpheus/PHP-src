/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2004 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@zend.com>                             |
  +----------------------------------------------------------------------+
*/
/* $Id$ */

#include "php_soap.h"
#include "ext/standard/base64.h"

static char *get_http_header_value(char *headers, char *type);
static int get_http_body(php_stream *socketd, char *headers,  char **response, int *out_size TSRMLS_DC);
static int get_http_headers(php_stream *socketd,char **response, int *out_size TSRMLS_DC);

#define smart_str_append_const(str, const) \
	smart_str_appendl(str,const,sizeof(const)-1)

static int stream_alive(php_stream *stream  TSRMLS_DC)
{
	int socket;
	fd_set rfds;
	struct timeval tv;
	char buf;

	if (stream == NULL || stream->eof || php_stream_cast(stream, PHP_STREAM_AS_FD_FOR_SELECT, (void**)&socket, 0) != SUCCESS) {
		return FALSE;
	}
	if (socket == -1) {
		return FALSE;
	} else {
		FD_ZERO(&rfds);
		FD_SET(socket, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		if (select(socket + 1, &rfds, NULL, NULL, &tv) > 0 && FD_ISSET(socket, &rfds)) {
			if (0 == recv(socket, &buf, sizeof(buf), MSG_PEEK) && php_socket_errno() != EAGAIN) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/* Proxy HTTP Authentication */
static void proxy_authentication(zval* this_ptr, smart_str* soap_headers TSRMLS_DC)
{
	zval **login, **password;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_proxy_login", sizeof("_proxy_login"), (void **)&login) == SUCCESS) {
		char* buf;
		int len;
		smart_str auth = {0};

		smart_str_appendl(&auth, Z_STRVAL_PP(login), Z_STRLEN_PP(login));
		smart_str_appendc(&auth, ':');
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_proxy_password", sizeof("_proxy_password"), (void **)&password) == SUCCESS) {
			smart_str_appendl(&auth, Z_STRVAL_PP(password), Z_STRLEN_PP(password));
		}
		smart_str_0(&auth);
		buf = php_base64_encode(auth.c, auth.len, &len);
		smart_str_append_const(soap_headers, "Proxy-Authorization: Basic ");
		smart_str_appendl(soap_headers, buf, len);
		smart_str_append_const(soap_headers, "\r\n");
		efree(buf);
		smart_str_free(&auth);
	}
}

static php_stream* http_connect(zval* this_ptr, php_url *phpurl, int use_ssl, int *use_proxy TSRMLS_DC)
{
	php_stream *stream;
	zval **proxy_host, **proxy_port;
	char *host;
#ifdef ZEND_ENGINE_2
	char *name;
	long namelen;
#endif
	int port;
	int old_error_reporting;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_proxy_host", sizeof("_proxy_host"), (void **) &proxy_host) == SUCCESS &&
	    Z_TYPE_PP(proxy_host) == IS_STRING &&
	    zend_hash_find(Z_OBJPROP_P(this_ptr), "_proxy_port", sizeof("_proxy_port"), (void **) &proxy_port) == SUCCESS &&
	    Z_TYPE_PP(proxy_port) == IS_LONG) {
		host = Z_STRVAL_PP(proxy_host);
		port = Z_LVAL_PP(proxy_port);
		*use_proxy = 1;
	} else {
		host = phpurl->host;
		port = phpurl->port;
	}

	old_error_reporting = EG(error_reporting);
	EG(error_reporting) &= ~(E_WARNING|E_NOTICE|E_USER_WARNING|E_USER_NOTICE);

#ifdef ZEND_ENGINE_2
	namelen = spprintf(&name, 0, "%s://%s:%d", (use_ssl && !*use_proxy)? "ssl" : "tcp", host, port);
	stream = php_stream_xport_create(name, namelen,
		ENFORCE_SAFE_MODE | REPORT_ERRORS,
		STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT,
		NULL /*persistent_id*/,
		NULL /*timeout*/,
		NULL, NULL, NULL);
	efree(name);
#else
	stream = php_stream_sock_open_host(host, port, SOCK_STREAM, NULL, NULL);
#endif

	/* SSL & proxy */
	if (stream && *use_proxy && use_ssl) {
		smart_str soap_headers = {0};
		char *http_headers;
		int http_header_size;

		smart_str_append_const(&soap_headers, "CONNECT ");
		smart_str_appends(&soap_headers, phpurl->host);
		smart_str_appendc(&soap_headers, ':');
		smart_str_append_unsigned(&soap_headers, phpurl->port);
		smart_str_append_const(&soap_headers, " HTTP/1.1\r\n");
		proxy_authentication(this_ptr, &soap_headers TSRMLS_CC);
		smart_str_append_const(&soap_headers, "\r\n");
		if (php_stream_write(stream, soap_headers.c, soap_headers.len) != soap_headers.len) {
			php_stream_close(stream);
			stream = NULL;
		}
 	 	smart_str_free(&soap_headers);

 	 	if (stream) {
			if (!get_http_headers(stream, &http_headers, &http_header_size TSRMLS_CC) || http_headers == NULL) {
				php_stream_close(stream);
				stream = NULL;
			}
			efree(http_headers);
		}
#ifdef ZEND_ENGINE_2
		/* enable SSL transport layer */
		if (stream) {
			if (php_stream_xport_crypto_setup(stream, STREAM_CRYPTO_METHOD_SSLv23_CLIENT, NULL TSRMLS_CC) < 0 ||
			    php_stream_xport_crypto_enable(stream, 1 TSRMLS_CC) < 0) {
				php_stream_close(stream);
				stream = NULL;
			}
		}
#endif
	}

#if !defined(ZEND_ENGINE_2) && defined(HAVE_OPENSSL_EXT)
	if (stream && use_ssl) {
		/* enable SSL transport layer */
		if (FAILURE == php_stream_sock_ssl_activate(stream, 1)) {
			php_stream_close(stream);
			stream = NULL;
		}
	}
#endif
	EG(error_reporting) = old_error_reporting;
	return stream;
}

int send_http_soap_request(zval *this_ptr, xmlDoc *doc, char *location, char *soapaction, int soap_version TSRMLS_DC)
{
	xmlChar *buf, *request;
	smart_str soap_headers = {0};
	int buf_size, request_size, err;
	php_url *phpurl = NULL;
	php_stream *stream;
	zval **trace, **tmp;
	int use_proxy = 0;
	int use_ssl;

	if (this_ptr == NULL || Z_TYPE_P(this_ptr) != IS_OBJECT) {
		return FALSE;
	}

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"), (void **)&tmp) == SUCCESS) {
		php_stream_from_zval_no_verify(stream,tmp);
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"), (void **)&tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_LONG) {
			use_proxy = Z_LVAL_PP(tmp);
		}
	} else {
		stream = NULL;
	}

	xmlDocDumpMemory(doc, &buf, &buf_size);
	if (!buf) {
		add_soap_fault(this_ptr, "HTTP", "Error build soap request", NULL, NULL TSRMLS_CC);
		return FALSE;
	}
	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "trace", sizeof("trace"), (void **) &trace) == SUCCESS &&
	    Z_LVAL_PP(trace) > 0) {
		add_property_stringl(this_ptr, "__last_request", buf, buf_size, 1);
	}

	/* Check if keep-alive connection is still opened */
	if (stream != NULL && !stream_alive(stream TSRMLS_CC)) {
		php_stream_close(stream);
		zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
		zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
		stream = NULL;
		use_proxy = 0;
	}

	if (location != NULL && location[0] != '\000') {
		phpurl = php_url_parse(location);
	}
	if (phpurl == NULL) {
		xmlFree(buf);
		add_soap_fault(this_ptr, "HTTP", "Unable to parse URL", NULL, NULL TSRMLS_CC);
		return FALSE;
	}

	use_ssl = 0;
	if (phpurl->scheme != NULL && strcmp(phpurl->scheme, "https") == 0) {
		use_ssl = 1;
	} else if (phpurl->scheme == NULL || strcmp(phpurl->scheme, "http") != 0) {
		xmlFree(buf);
		php_url_free(phpurl);
		add_soap_fault(this_ptr, "HTTP", "Unknown protocol. Only http and https are allowed.", NULL, NULL TSRMLS_CC);
		return FALSE;
	}
#ifdef ZEND_ENGINE_2
	if (use_ssl && php_stream_locate_url_wrapper("https://", NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) == NULL) {
		xmlFree(buf);
		php_url_free(phpurl);
		add_soap_fault(this_ptr, "HTTP", "SSL support not available in this build", NULL, NULL TSRMLS_CC);
		return FALSE;
	}
#else
#ifndef HAVE_OPENSSL_EXT
	if (use_ssl) {
		xmlFree(buf);
		php_url_free(phpurl);
		add_soap_fault(this_ptr, "HTTP", "SSL support not available in this build", NULL, NULL TSRMLS_CC);
		return FALSE;
	}
#endif
#endif

	if (phpurl->port == 0) {
		phpurl->port = use_ssl ? 443 : 80;
	}

	if (!stream) {
		stream = http_connect(this_ptr, phpurl, use_ssl, &use_proxy TSRMLS_CC);
		if (stream) {
			php_stream_auto_cleanup(stream);
			add_property_resource(this_ptr, "httpsocket", php_stream_get_resource_id(stream));
			add_property_long(this_ptr, "_use_proxy", use_proxy);
		} else {
			xmlFree(buf);
			php_url_free(phpurl);
			add_soap_fault(this_ptr, "HTTP", "Could not connect to host", NULL, NULL TSRMLS_CC);
			return FALSE;
		}
	}

	if (stream) {
		zval **cookies, **login, **password;

		smart_str_append_const(&soap_headers, "POST ");
		if (use_proxy && !use_ssl) {
			smart_str_appends(&soap_headers, phpurl->scheme);
			smart_str_append_const(&soap_headers, "://");
			smart_str_appends(&soap_headers, phpurl->host);
			smart_str_appendc(&soap_headers, ':');
			smart_str_append_unsigned(&soap_headers, phpurl->port);
		}
		smart_str_appends(&soap_headers, phpurl->path);
		smart_str_append_const(&soap_headers, " HTTP/1.1\r\n"
			"Host: ");
		smart_str_appends(&soap_headers, phpurl->host);
		smart_str_append_const(&soap_headers, "\r\n"
			"Connection: Keep-Alive\r\n"
/*
			"Connection: close\r\n"
			"Accept: text/html; text/xml; text/plain\r\n"
*/
			"User-Agent: PHP SOAP 0.1\r\n");
		if (soap_version == SOAP_1_2) {
			smart_str_append_const(&soap_headers,"Content-Type: application/soap+xml; charset=\"utf-8");
			if (soapaction) {
				smart_str_append_const(&soap_headers,"\"; action=\"");
				smart_str_appends(&soap_headers, soapaction);
			}
			smart_str_append_const(&soap_headers,"\"\r\n");
		} else {
			smart_str_append_const(&soap_headers,"Content-Type: text/xml; charset=\"utf-8\"\r\n");
			if (soapaction) {
				smart_str_append_const(&soap_headers, "SOAPAction: \"");
				smart_str_appends(&soap_headers, soapaction);
				smart_str_append_const(&soap_headers, "\"\r\n");
			}
		}

	  request = buf;
	  request_size = buf_size;
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "compression", sizeof("compression"), (void **)&tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_LONG) {
			int level = Z_LVAL_PP(tmp) & 0x0f;
			int kind  = Z_LVAL_PP(tmp) & SOAP_COMPRESSION_DEFLATE;

		  if ((Z_LVAL_PP(tmp) & SOAP_COMPRESSION_ACCEPT) != 0) {
				smart_str_append_const(&soap_headers,"Accept-Encoding: gzip, deflate\r\n");
		  }
		  if (level > 0) {
				zval func;
				zval retval;
  			zval param1, param2, param3;
				zval *params[3];
				int n;


				params[0] = &param1;
				INIT_PZVAL(params[0]);
				params[1] = &param2;
				INIT_PZVAL(params[1]);
				params[2] = &param3;
				INIT_PZVAL(params[2]);
				ZVAL_STRINGL(params[0], buf, buf_size, 0);
				ZVAL_LONG(params[1], level);
		    if (kind == SOAP_COMPRESSION_DEFLATE) {
		    	n = 2;
					ZVAL_STRING(&func, "gzcompress", 0);
					smart_str_append_const(&soap_headers,"Content-Encoding: deflate\r\n");
		    } else {
		      n = 3;
					ZVAL_STRING(&func, "gzencode", 0);
					smart_str_append_const(&soap_headers,"Content-Encoding: gzip\r\n");
					ZVAL_LONG(params[2], 1);
		      /* (SOAP_COMPRESSION_GZIP */
		    }
				if (call_user_function(CG(function_table), (zval**)NULL, &func, &retval, n, params TSRMLS_CC) == SUCCESS &&
				    Z_TYPE(retval) == IS_STRING) {
					request = Z_STRVAL(retval);
					request_size = Z_STRLEN(retval);
				} else {
			php_url_free(phpurl);
			if (request != buf) {efree(request);}
			xmlFree(buf);
			smart_str_free(&soap_headers);
			php_stream_close(stream);
			zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
			zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
			add_soap_fault(this_ptr, "HTTP", "Compression Failed", NULL, NULL TSRMLS_CC);
			return FALSE;
				}
		  }
		}
		smart_str_append_const(&soap_headers,"Content-Length: ");
		smart_str_append_long(&soap_headers, request_size);
		smart_str_append_const(&soap_headers, "\r\n");

		/* HTTP Authentication */
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_login", sizeof("_login"), (void **)&login) == SUCCESS) {
			char* buf;
			int len;

			smart_str auth = {0};
			smart_str_appendl(&auth, Z_STRVAL_PP(login), Z_STRLEN_PP(login));
			smart_str_appendc(&auth, ':');
			if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_password", sizeof("_password"), (void **)&password) == SUCCESS) {
				smart_str_appendl(&auth, Z_STRVAL_PP(password), Z_STRLEN_PP(password));
			}
			smart_str_0(&auth);
			buf = php_base64_encode(auth.c, auth.len, &len);
			smart_str_append_const(&soap_headers, "Authorization: Basic ");
			smart_str_appendl(&soap_headers, buf, len);
			smart_str_append_const(&soap_headers, "\r\n");
			efree(buf);
			smart_str_free(&auth);
		}

		/* Proxy HTTP Authentication */
		if (use_proxy && !use_ssl) {
			proxy_authentication(this_ptr, &soap_headers TSRMLS_CC);
		}

		/* Send cookies along with request */
		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_cookies", sizeof("_cookies"), (void **)&cookies) == SUCCESS) {
			zval **data;
			char *key;
			int i, n;

			n = zend_hash_num_elements(Z_ARRVAL_PP(cookies));
			if (n > 0) {
				zend_hash_internal_pointer_reset(Z_ARRVAL_PP(cookies));
				smart_str_append_const(&soap_headers, "Cookie: ");
				for (i = 0; i < n; i++) {
					zend_hash_get_current_data(Z_ARRVAL_PP(cookies), (void **)&data);
					zend_hash_get_current_key(Z_ARRVAL_PP(cookies), &key, NULL, FALSE);

					smart_str_appendl(&soap_headers, key, strlen(key));
					smart_str_appendc(&soap_headers, '=');
					smart_str_appendl(&soap_headers, Z_STRVAL_PP(data), Z_STRLEN_PP(data));
					smart_str_append_const(&soap_headers, ";");
					zend_hash_move_forward(Z_ARRVAL_PP(cookies));
				}
				smart_str_append_const(&soap_headers, "\r\n");
			}
		}
		smart_str_append_const(&soap_headers, "\r\n");
		smart_str_appendl(&soap_headers, request, request_size);
		smart_str_0(&soap_headers);

		err = php_stream_write(stream, soap_headers.c, soap_headers.len);
		if (err != soap_headers.len) {
			php_url_free(phpurl);
			if (request != buf) {efree(request);}
			xmlFree(buf);
			smart_str_free(&soap_headers);
			php_stream_close(stream);
			zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
			zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
			add_soap_fault(this_ptr, "HTTP", "Failed Sending HTTP SOAP request", NULL, NULL TSRMLS_CC);
			return FALSE;
		}
		smart_str_free(&soap_headers);

	}
	php_url_free(phpurl);
	if (request != buf) {efree(request);}
	xmlFree(buf);
	return TRUE;
}

int get_http_soap_response(zval *this_ptr, char **buffer, int *buffer_len TSRMLS_DC)
{
	char *http_headers, *http_body, *content_type, *http_version, *cookie_itt;
	int http_header_size, http_body_size, http_close;
	php_stream *stream;
	zval **trace, **tmp;
	char *connection;
	int http_1_1 = 0;
	int http_status = 0;
	char *content_encoding;

	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"), (void **)&tmp) == SUCCESS) {
		php_stream_from_zval_no_verify(stream,tmp);
	} else {
		stream = NULL;
	}
	if (stream == NULL) {
	  return FALSE;
	}

	if (!get_http_headers(stream, &http_headers, &http_header_size TSRMLS_CC)) {
		php_stream_close(stream);
		zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
		zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
		add_soap_fault(this_ptr, "HTTP", "Error Fetching http headers", NULL, NULL TSRMLS_CC);
		return FALSE;
	}

	/* Check to see what HTTP status was sent */
	http_version = get_http_header_value(http_headers,"HTTP/");
	if (http_version) {
		char *tmp;

		tmp = strstr(http_version," ");

		if (tmp != NULL) {
			tmp++;
			http_status = atoi(tmp);
		}

		/*
		Try and process any respsone that is xml might contain fault code

		Maybe try and test for some of the 300's 400's specfics but not
		right now.

		if (http_status >= 200 && http_status < 300) {
		} else if (http_status >= 300 && http_status < 400) {
			add_soap_fault(this_ptr, "HTTP", "HTTTP redirection is not supported", NULL, NULL TSRMLS_CC);
		} else if (http_status == 400) {
			add_soap_fault(this_ptr, "HTTP", "Bad Request", NULL, NULL TSRMLS_CC);
		} else if (http_status == 401) {
			add_soap_fault(this_ptr, "HTTP", "Unauthorized Request", NULL, NULL TSRMLS_CC);
		} else if (http_status == 405) {
			add_soap_fault(this_ptr, "HTTP", "Method not allowed", NULL, NULL TSRMLS_CC);
		} else if (http_status == 415) {
			add_soap_fault(this_ptr, "HTTP", "Unsupported Media Type", NULL, NULL TSRMLS_CC);
		} else if (http_status >= 400 && http_status < 500) {
			add_soap_fault(this_ptr, "HTTP", "Client Error", NULL, NULL TSRMLS_CC);
		} else if (http_status == 500) {
			add_soap_fault(this_ptr, "HTTP", "Internal Server Error", NULL, NULL TSRMLS_CC);
		} else if (http_status >= 500 && http_status < 600) {
			add_soap_fault(this_ptr, "HTTP", "Server Error", NULL, NULL TSRMLS_CC);
		} else {
			add_soap_fault(this_ptr, "HTTP", "Unsupported HTTP status code", NULL, NULL TSRMLS_CC);
		}
		*/

		/* Try and get headers again */
		if (http_status == 100) {
			efree(http_headers);
			if (!get_http_headers(stream, &http_headers, &http_header_size TSRMLS_CC)) {
				php_stream_close(stream);
				zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
				zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
				add_soap_fault(this_ptr, "HTTP", "Error Fetching http headers", NULL, NULL TSRMLS_CC);
				return FALSE;
			}
		}

		if (strncmp(http_version,"1.1", 3)) {
			http_1_1 = 1;
		}
		efree(http_version);
	}

	if (!get_http_body(stream, http_headers, &http_body, &http_body_size TSRMLS_CC)) {
		php_stream_close(stream);
		zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
		zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
		add_soap_fault(this_ptr, "HTTP", "Error Fetching http body, No Content-Length, connection closed or chunked data", NULL, NULL TSRMLS_CC);
		return FALSE;
	}

	/* See if the server requested a close */
	http_close = TRUE;
	connection = get_http_header_value(http_headers,"Proxy-Connection: ");
	if (connection) {
		if (strncasecmp(connection, "Keep-Alive", sizeof("Keep-Alive")-1) == 0) {
			http_close = FALSE;
		}
		efree(connection);
/*
	} else if (http_1_1) {
		http_close = FALSE;
*/
	}
	connection = get_http_header_value(http_headers,"Connection: ");
	if (connection) {
		if (strncasecmp(connection, "Keep-Alive", sizeof("Keep-Alive")-1) == 0) {
			http_close = FALSE;
		}
		efree(connection);
/*
	} else if (http_1_1) {
		http_close = FALSE;
*/
	}

	if (http_close) {
		php_stream_close(stream);
		zend_hash_del(Z_OBJPROP_P(this_ptr), "httpsocket", sizeof("httpsocket"));
		zend_hash_del(Z_OBJPROP_P(this_ptr), "_use_proxy", sizeof("_use_proxy"));
	}

	/* Check and see if the server even sent a xml document */
	content_type = get_http_header_value(http_headers,"Content-Type: ");
	if (content_type) {
		char *pos = NULL;
		int cmplen;
		pos = strstr(content_type,";");
		if (pos != NULL) {
			cmplen = pos - content_type;
		} else {
			cmplen = strlen(content_type);
		}
		if (strncmp(content_type, "text/xml", cmplen) == 0 ||
		    strncmp(content_type, "application/soap+xml", cmplen == 0)) {
/*
			if (strncmp(http_body, "<?xml", 5)) {
				zval *err;
				MAKE_STD_ZVAL(err);
				ZVAL_STRINGL(err, http_body, http_body_size, 1);
				add_soap_fault(this_ptr, "HTTP", "Didn't recieve an xml document", NULL, err TSRMLS_CC);
				efree(content_type);
				efree(http_headers);
				efree(http_body);
				return FALSE;
			}
*/
		}
		efree(content_type);
	}

	/* Grab and send back every cookie */

	/* Not going to worry about Path: because
	   we shouldn't be changing urls so path dont
	   matter too much
	*/
	cookie_itt = strstr(http_headers,"Set-Cookie: ");
	while (cookie_itt) {
		char *end_pos, *cookie;
		char *eqpos, *sempos;
		smart_str name = {0}, value = {0};
		zval **cookies, *z_cookie;

		if (zend_hash_find(Z_OBJPROP_P(this_ptr), "_cookies", sizeof("_cookies"), (void **)&cookies) == FAILURE) {
			zval *tmp_cookies;
			MAKE_STD_ZVAL(tmp_cookies);
			array_init(tmp_cookies);
			zend_hash_update(Z_OBJPROP_P(this_ptr), "_cookies", sizeof("_cookies"), &tmp_cookies, sizeof(zval *), (void **)&cookies);
		}

		end_pos = strstr(cookie_itt,"\r\n");
		cookie = get_http_header_value(cookie_itt,"Set-Cookie: ");

		eqpos = strstr(cookie, "=");
		sempos = strstr(cookie, ";");
		if (eqpos != NULL && (sempos == NULL || sempos > eqpos)) {
			int cookie_len;

			if (sempos != NULL) {
				cookie_len = sempos-(eqpos+1);
			} else {
				cookie_len = strlen(cookie)-(eqpos-cookie)-1;
			}

			smart_str_appendl(&name, cookie, eqpos - cookie);
			smart_str_0(&name);

			smart_str_appendl(&value, eqpos + 1, cookie_len);
			smart_str_0(&value);

			MAKE_STD_ZVAL(z_cookie);
			ZVAL_STRINGL(z_cookie, value.c, value.len, 1);

			zend_hash_update(Z_ARRVAL_PP(cookies), name.c, name.len + 1, &z_cookie, sizeof(zval *), NULL);
		}

		cookie_itt = strstr(cookie_itt + sizeof("Set-Cookie: "), "Set-Cookie: ");

		smart_str_free(&value);
		smart_str_free(&name);
		efree(cookie);
	}

	content_encoding = get_http_header_value(http_headers,"Content-Encoding: ");
	if (content_encoding) {
		zval func;
		zval retval;
	  zval param;
		zval *params[1];

		if ((strcmp(content_encoding,"gzip") == 0 ||
		     strcmp(content_encoding,"x-gzip") == 0) &&
		     zend_hash_exists(EG(function_table), "gzinflate", sizeof("gzinflate"))) {
			ZVAL_STRING(&func, "gzinflate", 0);
			params[0] = &param;
			ZVAL_STRINGL(params[0], http_body+10, http_body_size-10, 0);
			INIT_PZVAL(params[0]);
		} else if (strcmp(content_encoding,"deflate") == 0 &&
		           zend_hash_exists(EG(function_table), "gzuncompress", sizeof("gzuncompress"))) {
			ZVAL_STRING(&func, "gzuncompress", 0);
			params[0] = &param;
			ZVAL_STRINGL(params[0], http_body, http_body_size, 0);
			INIT_PZVAL(params[0]);
		} else {
			efree(content_encoding);
			efree(http_headers);
			efree(http_body);
			add_soap_fault(this_ptr, "HTTP", "Unknown Content-Encoding", NULL, NULL TSRMLS_CC);
			return FALSE;
		}
		if (call_user_function(CG(function_table), (zval**)NULL, &func, &retval, 1, params TSRMLS_CC) == SUCCESS &&
		    Z_TYPE(retval) == IS_STRING) {
			efree(http_body);
			*buffer = Z_STRVAL(retval);
			*buffer_len = Z_STRLEN(retval);
		} else {
			efree(content_encoding);
			efree(http_headers);
			efree(http_body);
			add_soap_fault(this_ptr, "HTTP", "Can't uncompress compressed response", NULL, NULL TSRMLS_CC);
			return FALSE;
		}
		efree(content_encoding);
	} else {
		*buffer = http_body;
		*buffer_len = http_body_size;
	}
	if (zend_hash_find(Z_OBJPROP_P(this_ptr), "trace", sizeof("trace"), (void **) &trace) == SUCCESS &&
	    Z_LVAL_PP(trace) > 0) {
		add_property_stringl(this_ptr, "__last_response", *buffer, *buffer_len, 1);
	}

	efree(http_headers);
	return TRUE;
}

static char *get_http_header_value(char *headers, char *type)
{
	char *pos, *tmp = NULL;
	int typelen, headerslen;

	typelen = strlen(type);
	headerslen = strlen(headers);

	/* header `titles' can be lower case, or any case combination, according
	 * to the various RFC's. */
	pos = headers;
	do {
		/* start of buffer or start of line */
		if (strncasecmp(pos, type, typelen) == 0) {
			char *eol;

			/* match */
			tmp = pos + typelen;
			eol = strstr(tmp, "\r\n");
			if (eol == NULL) {
				eol = headers + headerslen;
			}
			return estrndup(tmp, eol - tmp);
		}

		/* find next line */
		pos = strstr(pos, "\r\n");
		if (pos) {
			pos += 2;
		}

	} while (pos);

	return NULL;
}

static int get_http_body(php_stream *stream, char *headers,  char **response, int *out_size TSRMLS_DC)
{
	char *header, *http_buf = NULL;
	int header_close = 0, header_chunked = 0, header_length = 0, http_buf_size = 0;

	header = get_http_header_value(headers, "Connection: ");
	if (header) {
		if(!strcmp(header, "close")) header_close = 1;
		efree(header);
	}
	header = get_http_header_value(headers, "Transfer-Encoding: ");
	if (header) {
		if(!strcmp(header, "chunked")) header_chunked = 1;
		efree(header);
	}
	header = get_http_header_value(headers, "Content-Length: ");
	if (header) {
		header_length = atoi(header);
		efree(header);
	}

	if (header_chunked) {
		char done, chunk_size[10];

		done = FALSE;

		while (!done) {
			int buf_size = 0;

			php_stream_gets(stream, chunk_size, sizeof(chunk_size));
			if (sscanf(chunk_size, "%x", &buf_size) > 0 ) {
				if (buf_size > 0) {
					int len_size = 0;

					http_buf = erealloc(http_buf, http_buf_size + buf_size + 1);

					while (len_size < buf_size) {
						int len_read = php_stream_read(stream, http_buf + http_buf_size, buf_size - len_size);
						if (len_read <= 0) {
							/* Error or EOF */
							done = TRUE;
						  break;
						}
						len_size += len_read;
	 					http_buf_size += len_read;
 					}
 				}

				/* Eat up '\r' '\n' */
				php_stream_getc(stream);
				php_stream_getc(stream);
			} else {
				/* Somthing wrong in chunked encoding */
				efree(http_buf);
				return FALSE;
			}
			if (buf_size == 0) {
				done = TRUE;
			}
		}

		if (http_buf == NULL) {
			http_buf = emalloc(1);
		}

	} else if (header_length) {
		http_buf = emalloc(header_length + 1);
		while (http_buf_size < header_length) {
			http_buf_size += php_stream_read(stream, http_buf + http_buf_size, header_length - http_buf_size);
		}
	} else if (header_close) {
		do {
			http_buf = erealloc(http_buf, http_buf_size + 4096 + 1);
			http_buf_size += php_stream_read(stream, http_buf + http_buf_size, 4096);
		} while(!php_stream_eof(stream));
	} else {
		return FALSE;
	}

	http_buf[http_buf_size] = '\0';
	(*response) = http_buf;
	(*out_size) = http_buf_size;
	return TRUE;
}

static int get_http_headers(php_stream *stream, char **response, int *out_size TSRMLS_DC)
{
	int done = FALSE;
	smart_str tmp_response = {0};
	char headerbuf[8192];

	while (!done) {
		if (!php_stream_gets(stream, headerbuf, sizeof(headerbuf))) {
			break;
		}

		if (strcmp(headerbuf, "\r\n") == 0) {
			/* empty line marks end of headers */
			done = TRUE;
			break;
		}

		/* add header to collection */
		smart_str_appends(&tmp_response, headerbuf);
	}
	smart_str_0(&tmp_response);
	(*response) = tmp_response.c;
	(*out_size) = tmp_response.len;
	return done;
}
