/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   |                                                                      |
   +----------------------------------------------------------------------+
 */

#if WIN32|WINNT
# include <windows.h>
#endif
#include <httpext.h>
#include <httpfilt.h>
#include <httpext.h>
#include "php.h"
#include "main.h"
#include "SAPI.h"
#include "php_globals.h"
#include "ext/standard/info.h"

#ifdef WITH_ZEUS
#include "zeus.h"
#endif

#define MAX_STATUS_LENGTH sizeof("xxxx LONGEST STATUS DESCRIPTION")
#define ISAPI_SERVER_VAR_BUF_SIZE 1024
#define ISAPI_POST_DATA_BUF 1024

int IWasLoaded=0;

static char *isapi_server_variables[] = {
	"ALL_HTTP",
	"APPL_MD_PATH",
	"APPL_PHYSICAL_PATH",
	"AUTH_PASSWORD",
	"AUTH_TYPE",
	"AUTH_USER",
	"CERT_COOKIE",
	"CERT_FLAGS",
	"CERT_ISSUER",
	"CERT_KEYSIZE",
	"CERT_SECRETKEYSIZE",
	"CERT_SERIALNUMBER",
	"CERT_SERVER_ISSUER",
	"CERT_SERVER_SUBJECT",
	"CERT_SUBJECT",
	"CONTENT_LENGTH",
	"CONTENT_TYPE",
	"LOGON_USER",
	"HTTP_COOKIE",
	"HTTPS",
	"HTTPS_KEYSIZE",
	"HTTPS_SECRETKEYSIZE",
	"HTTPS_SERVER_ISSUER",
	"HTTPS_SERVER_SUBJECT",
	"INSTANCE_ID",
	"INSTANCE_META_PATH",
	"PATH_INFO",
	"PATH_TRANSLATED",
	"QUERY_STRING",
	"REMOTE_ADDR",
	"REMOTE_HOST",
	"REMOTE_USER",
	"REQUEST_METHOD",
	"SCRIPT_NAME",
	"SERVER_NAME",
	"SERVER_PORT",
	"SERVER_PORT_SECURE",
	"SERVER_PROTOCOL",
	"SERVER_SOFTWARE",
	"URL",
	NULL
};


static void php_info_isapi(ZEND_MODULE_INFO_FUNC_ARGS)
{
	char **p = isapi_server_variables;
	char variable_buf[ISAPI_SERVER_VAR_BUF_SIZE];
	DWORD variable_len;
	LPEXTENSION_CONTROL_BLOCK lpECB;
	SLS_FETCH();

	lpECB = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);

	PUTS("<table border=5 width=\"600\">\n");
	php_info_print_table_header(2, "Server Variable", "Value");
	while (*p) {
		variable_len = ISAPI_SERVER_VAR_BUF_SIZE;
		if (lpECB->GetServerVariable(lpECB->ConnID, *p, variable_buf, &variable_len)
			&& variable_buf[0]) {
			php_info_print_table_row(2, *p, variable_buf);
		} else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			char *tmp_variable_buf;

			tmp_variable_buf = (char *) emalloc(variable_len);
			if (lpECB->GetServerVariable(lpECB->ConnID, *p, tmp_variable_buf, &variable_len)
				&& variable_buf[0]) {
				php_info_print_table_row(2, *p, tmp_variable_buf);
			}
			efree(tmp_variable_buf);
		}
		p++;
	}

	PUTS("</table>");
}


static zend_module_entry php_isapi_module = {
	"ISAPI",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	php_info_isapi,
	STANDARD_MODULE_PROPERTIES
};


static int zend_isapi_ub_write(const char *str, uint str_length)
{
	DWORD num_bytes = str_length;
	LPEXTENSION_CONTROL_BLOCK ecb;
	SLS_FETCH();
	
	ecb = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);
	ecb->WriteClient(ecb->ConnID, (char *) str, &num_bytes, HSE_IO_SYNC );
	return num_bytes;
}


static int sapi_isapi_header_handler(sapi_header_struct *sapi_header, sapi_headers_struct *sapi_headers SLS_DC)
{
	return SAPI_HEADER_ADD;
}



static void accumulate_header_length(sapi_header_struct *sapi_header, uint *total_length)
{
	*total_length += sapi_header->header_len+2;
}


static void concat_header(sapi_header_struct *sapi_header, char **combined_headers_ptr)
{
	memcpy(*combined_headers_ptr, sapi_header->header, sapi_header->header_len);
	*combined_headers_ptr += sapi_header->header_len;
	**combined_headers_ptr = '\r';
	(*combined_headers_ptr)++;
	**combined_headers_ptr = '\n';
	(*combined_headers_ptr)++;
}


static int sapi_isapi_send_headers(sapi_headers_struct *sapi_headers SLS_DC)
{
	uint total_length = 2;		/* account for the trailing \r\n */
	char *combined_headers, *combined_headers_ptr;
	LPEXTENSION_CONTROL_BLOCK lpECB = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);
	HSE_SEND_HEADER_EX_INFO header_info;
	char status_buf[MAX_STATUS_LENGTH];
	sapi_header_struct default_content_type = { SAPI_DEFAULT_CONTENT_TYPE, sizeof(SAPI_DEFAULT_CONTENT_TYPE)-1 };
	PLS_FETCH();

	/* Obtain headers length */
	if (SG(sapi_headers).send_default_content_type) {
		accumulate_header_length(&default_content_type, (void *) &total_length);
	}
	zend_llist_apply_with_argument(&SG(sapi_headers).headers, (void (*)(void *, void *)) accumulate_header_length, (void *) &total_length);

	/* Generate headers */
	combined_headers = (char *) emalloc(total_length+1);
	combined_headers_ptr = combined_headers;
	if (SG(sapi_headers).send_default_content_type) {
		concat_header(&default_content_type, (void *) &combined_headers_ptr);
	}
	zend_llist_apply_with_argument(&SG(sapi_headers).headers, (void (*)(void *, void *)) concat_header, (void *) &combined_headers_ptr);
	*combined_headers_ptr++ = '\r';
	*combined_headers_ptr++ = '\n';
	*combined_headers_ptr = 0;

	switch (SG(sapi_headers).http_response_code) {
		case 200:
			header_info.pszStatus = "200 OK";
			break;
		case 302:
			header_info.pszStatus = "302 Moved Temporarily";
			break;
		case 401:
			header_info.pszStatus = "401 Authorization Required";
			break;
		default:
			snprintf(status_buf, MAX_STATUS_LENGTH, "%d Undescribed", SG(sapi_headers).http_response_code);
			header_info.pszStatus = status_buf;
			break;
	}
#ifndef WITH_ZEUS
	header_info.cchStatus = strlen(header_info.pszStatus);
#endif
	header_info.pszHeader = combined_headers;
	header_info.cchHeader = total_length;
	lpECB->dwHttpStatusCode = SG(sapi_headers).http_response_code;

	lpECB->ServerSupportFunction(lpECB->ConnID, HSE_REQ_SEND_RESPONSE_HEADER_EX, &header_info, NULL, NULL);

	efree(combined_headers);
	if (SG(sapi_headers).http_status_line) {
		efree(SG(sapi_headers).http_status_line);
	}
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}


static int php_isapi_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module)==FAILURE
		|| zend_register_module(&php_isapi_module)==FAILURE) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
}



static int sapi_isapi_read_post(char *buffer, uint count_bytes SLS_DC)
{
	LPEXTENSION_CONTROL_BLOCK lpECB = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);
	DWORD read_from_buf=0;
	DWORD read_from_input=0;
	DWORD total_read=0;

	if (SG(read_post_bytes) < lpECB->cbAvailable) {
		read_from_buf = MIN(lpECB->cbAvailable-SG(read_post_bytes), count_bytes);
		memcpy(buffer, lpECB->lpbData+SG(read_post_bytes), read_from_buf);
		total_read += read_from_buf;
	}
	if (read_from_buf<count_bytes
		&& (SG(read_post_bytes)+read_from_buf) < lpECB->cbTotalBytes) {
		DWORD cbRead=0, cbSize;

		read_from_input = MIN(count_bytes-read_from_buf, lpECB->cbTotalBytes-SG(read_post_bytes)-read_from_buf);
		while (cbRead < read_from_input) {
			cbSize = read_from_input - cbRead;
			if (!lpECB->ReadClient(lpECB->ConnID, buffer+read_from_buf+cbRead, &cbSize) || cbSize==0) {
				break;
			}
			cbRead += cbSize;
		}
		total_read += cbRead;
	}
	SG(read_post_bytes) += total_read;
	return total_read;
}


static char *sapi_isapi_read_cookies(SLS_D)
{
	LPEXTENSION_CONTROL_BLOCK lpECB = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);
	char variable_buf[ISAPI_SERVER_VAR_BUF_SIZE];
	DWORD variable_len = ISAPI_SERVER_VAR_BUF_SIZE;

	if (lpECB->GetServerVariable(lpECB->ConnID, "HTTP_COOKIE", variable_buf, &variable_len)) {
		return estrndup(variable_buf, variable_len);
	} else if (GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
		char *tmp_variable_buf = (char *) emalloc(variable_len+1);

		if (lpECB->GetServerVariable(lpECB->ConnID, "HTTP_COOKIE", tmp_variable_buf, &variable_len)) {
			tmp_variable_buf[variable_len] = 0;
			return tmp_variable_buf;
		} else {
			efree(tmp_variable_buf);
		}
	}
	return NULL;
}


static sapi_module_struct sapi_module = {
	"PHP Language",					/* name */
									
	php_isapi_startup,				/* startup */
	php_module_shutdown_wrapper,	/* shutdown */

	zend_isapi_ub_write,			/* unbuffered write */

	php_error,						/* error handler */

	sapi_isapi_header_handler,		/* header handler */
	sapi_isapi_send_headers,		/* send headers handler */
	NULL,							/* send header handler */

	sapi_isapi_read_post,			/* read POST data */
	sapi_isapi_read_cookies,		/* read Cookies */

	STANDARD_SAPI_MODULE_PROPERTIES
};


BOOL WINAPI GetFilterVersion(PHTTP_FILTER_VERSION pFilterVersion)
{
	pFilterVersion->dwFilterVersion = HTTP_FILTER_REVISION;
	strcpy(pFilterVersion->lpszFilterDesc, sapi_module.name);
	pFilterVersion->dwFlags= (SF_NOTIFY_AUTHENTICATION | SF_NOTIFY_PREPROC_HEADERS);
	return TRUE;
}


DWORD WINAPI HttpFilterProc(PHTTP_FILTER_CONTEXT pfc, DWORD notificationType, LPVOID pvNotification)
{
	SLS_FETCH();

	switch (notificationType) {
		case SF_NOTIFY_PREPROC_HEADERS:
			SG(request_info).auth_user = NULL;
			SG(request_info).auth_password = NULL;
			break;
		case SF_NOTIFY_AUTHENTICATION: {
				char *auth_user = ((HTTP_FILTER_AUTHENT *) pvNotification)->pszUser;
				char *auth_password = ((HTTP_FILTER_AUTHENT *) pvNotification)->pszPassword;

				if (auth_user && auth_user[0]) {
					SG(request_info).auth_user = estrdup(auth_user);
				}   
				if (auth_password && auth_password[0]) {
					SG(request_info).auth_password = estrdup(auth_password);
				}
				auth_user[0] = 0;
				auth_password[0] = 0;
				return SF_STATUS_REQ_HANDLED_NOTIFICATION;
			}
			break;
	}
	return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


static void init_request_info(sapi_globals_struct *sapi_globals, LPEXTENSION_CONTROL_BLOCK lpECB)
{
	SG(request_info).request_method = lpECB->lpszMethod;
	SG(request_info).query_string = lpECB->lpszQueryString;
	SG(request_info).path_translated = lpECB->lpszPathTranslated;
	SG(request_info).request_uri = lpECB->lpszPathInfo;
	SG(request_info).content_type = lpECB->lpszContentType;
	SG(request_info).content_length = lpECB->cbTotalBytes;
	{
		char *path_end = strrchr(SG(request_info).path_translated, '\\');

		if (path_end) {
			*path_end = 0;
			chdir(SG(request_info).path_translated);
			*path_end = '\\';
		}
	}
}


BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pVer)
{
	pVer->dwExtensionVersion = HSE_VERSION;
	lstrcpyn(pVer->lpszExtensionDesc, sapi_module.name, HSE_MAX_EXT_DLL_NAME_LEN);
	return TRUE;
}


static void hash_isapi_variables(ELS_D SLS_DC)
{
	char static_variable_buf[ISAPI_SERVER_VAR_BUF_SIZE];
	char *variable_buf;
	DWORD variable_len = ISAPI_SERVER_VAR_BUF_SIZE;
	char *variable;
	char *strtok_buf = NULL;
	LPEXTENSION_CONTROL_BLOCK lpECB;

	lpECB = (LPEXTENSION_CONTROL_BLOCK) SG(server_context);

	if (lpECB->GetServerVariable(lpECB->ConnID, "ALL_HTTP", static_variable_buf, &variable_len)) {
		variable_buf = static_variable_buf;
	} else {
		if (GetLastError()==ERROR_INSUFFICIENT_BUFFER) {
			variable_buf = (char *) emalloc(variable_len);
			if (!lpECB->GetServerVariable(lpECB->ConnID, "ALL_HTTP", variable_buf, &variable_len)) {
				efree(variable_buf);
				return;
			}
		} else {
			return;
		}
	}
	variable = strtok_r(variable_buf, "\r\n", &strtok_buf);
	while (variable) {
		char *colon = strchr(variable, ':');

		if (colon) {
			char *value = colon+1;
			zval *entry = (zval *) emalloc(sizeof(zval));

			while (*value==' ') {
				value++;
			}
			*colon = 0;
			INIT_PZVAL(entry);
			entry->value.str.len = strlen(value);
			entry->value.str.val = estrndup(value, entry->value.str.len);
			entry->type = IS_STRING;
			zend_hash_add(&EG(symbol_table), variable, strlen(variable)+1, &entry, sizeof(zval *), NULL);
			*colon = ':';
		}
		variable = strtok_r(NULL, "\r\n", &strtok_buf);
	}
	if (variable_buf!=static_variable_buf) {
		efree(variable_buf);
	}
}


DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK lpECB)
{
	zend_file_handle file_handle;
	SLS_FETCH();
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	if (setjmp(EG(bailout))!=0) {
		return HSE_STATUS_ERROR;
	}

	init_request_info(sapi_globals, lpECB);
	SG(server_context) = lpECB;

	file_handle.filename = sapi_globals->request_info.path_translated;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;

	php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC);
	hash_isapi_variables(ELS_C SLS_CC);
	php_execute_script(&file_handle CLS_CC ELS_CC PLS_CC);
	if (SG(request_info).cookie_data) {
		efree(SG(request_info).cookie_data);
	}
	php_request_shutdown(NULL);
	return HSE_STATUS_SUCCESS;
}



__declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			tsrm_startup(1, 1, 0);
			sapi_startup(&sapi_module);
			if (sapi_module.startup) {
				sapi_module.startup(&sapi_module);
			}
			IWasLoaded = 1;
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			ts_free_thread();
			break;
		case DLL_PROCESS_DETACH:
			if (sapi_module.shutdown) {
				sapi_module.shutdown(&sapi_module);
			}
			tsrm_shutdown();
			break;
	}
	return TRUE;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
