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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

#include <stdio.h>
#include "php.h"
#include "ext/standard/php3_standard.h"
#include "zend_globals.h"
#include "php_globals.h"
#include "rfc1867.h"

#ifndef THREAD_SAFE
extern int le_uploads; /* "borrowed" from file.c */
extern HashTable list;
#endif

#define NEW_BOUNDARY_CHECK 1
#define SAFE_RETURN { if (namebuf) efree(namebuf); if (filenamebuf) efree(filenamebuf); if (lbuf) efree(lbuf); return; }

/*
 * Split raw mime stream up into appropriate components
 */
void php_mime_split(char *buf, int cnt, char *boundary)
{
	char *ptr, *loc, *loc2, *s, *name, *filename, *u, *fn;
	int len, state = 0, Done = 0, rem, urem;
	long bytes, max_file_size = 0;
	char *namebuf=NULL, *filenamebuf=NULL, *lbuf=NULL;
	FILE *fp;
	int itype;
	zval *http_post_vars=NULL;
	ELS_FETCH();
	PLS_FETCH();

	if (PG(track_vars)) {
		http_post_vars = (pval *) emalloc(sizeof(pval));
		array_init(http_post_vars);
		INIT_PZVAL(http_post_vars);
		
		zend_hash_add(&EG(symbol_table), "HTTP_POST_VARS", sizeof("HTTP_POST_VARS"), &http_post_vars, sizeof(pval *), NULL);
	}

	ptr = buf;
	rem = cnt;
	len = strlen(boundary);
	while ((ptr - buf < cnt) && !Done) {
		switch (state) {
			case 0:			/* Looking for mime boundary */
				loc = memchr(ptr, *boundary, cnt);
				if (loc) {
					if (!strncmp(loc, boundary, len)) {

						state = 1;
						rem -= (loc - ptr) + len + 2;
						ptr = loc + len + 2;
					} else {
						rem -= (loc - ptr) + 1;
						ptr = loc + 1;
					}
				} else {
					Done = 1;
				}
				break;
			case 1:			/* Check content-disposition */
				if (strncasecmp(ptr, "Content-Disposition: form-data;", 31)) {
					if (rem < 31) {
						SAFE_RETURN;
					}
					php_error(E_WARNING, "File Upload Mime headers garbled [%c%c%c%c%c]", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4));
					SAFE_RETURN;
				}
				loc = memchr(ptr, '\n', rem);
				name = strstr(ptr, " name=\"");
				if (name && name < loc) {
					name += 7;
					s = memchr(name, '\"', loc - name);
					if (!s) {
						php_error(E_WARNING, "File Upload Mime headers garbled [%c%c%c%c%c]", *name, *(name + 1), *(name + 2), *(name + 3), *(name + 4));
						SAFE_RETURN;
					}
					if (namebuf) {
						efree(namebuf);
					}
					namebuf = estrndup(name, s-name);
					if (lbuf) {
						efree(lbuf);
					}
					lbuf = emalloc(s-name + MAX(MAX(sizeof("_name"),sizeof("_size")),sizeof("_type")));
					state = 2;
					loc2 = memchr(loc + 1, '\n', rem);
					rem -= (loc2 - ptr) + 1;
					ptr = loc2 + 1;
				} else {
					php_error(E_WARNING, "File upload error - no name component in content disposition");
					SAFE_RETURN;
				}
				filename = strstr(s, " filename=\"");
				if (filename && filename < loc) {
					filename += 11;
					s = memchr(filename, '\"', loc - filename);
					if (!s) {
						php_error(E_WARNING, "File Upload Mime headers garbled [%c%c%c%c%c]", *filename, *(filename + 1), *(filename + 2), *(filename + 3), *(filename + 4));
						SAFE_RETURN;
					}
					if (filenamebuf) {
						efree(filenamebuf);
					}
					filenamebuf = estrndup(filename, s-filename);
					sprintf(lbuf, "%s_name", namebuf);
					s = strrchr(filenamebuf, '\\');
					if (s && s > filenamebuf) {
						SET_VAR_STRING(lbuf, estrdup(s + 1));
					} else {
						SET_VAR_STRING(lbuf, estrdup(filenamebuf));
					}
					state = 3;
					if ((loc2 - loc) > 2) {
						if (!strncasecmp(loc + 1, "Content-Type:", 13)) {
							*(loc2 - 1) = '\0';
							sprintf(lbuf, "%s_type", namebuf);
							SET_VAR_STRING(lbuf, estrdup(loc + 15));
							*(loc2 - 1) = '\n';
						}
						rem -= 2;
						ptr += 2;
					}
				}
				break;

			case 2:			/* handle form-data fields */
				loc = memchr(ptr, *boundary, rem);
				u = ptr;
				while (loc) {
					if (!strncmp(loc, boundary, len))
						break;
					u = loc + 1;
					urem = rem - (loc - ptr) - 1;
					loc = memchr(u, *boundary, urem);
				}
				if (!loc) {
					php_error(E_WARNING, "File Upload Field Data garbled");
					SAFE_RETURN;
				}
				*(loc - 4) = '\0';

				/* Magic function that figures everything out */
				php_parse_gpc_data2(ptr,namebuf,http_post_vars ELS_CC PLS_CC);

				/* And a little kludge to pick out special MAX_FILE_SIZE */
				itype = php3_check_ident_type(namebuf);
				if (itype) {
					u = strchr(namebuf, '[');
					if (u)
						*u = '\0';
				}
				if (!strcmp(namebuf, "MAX_FILE_SIZE")) {
					max_file_size = atol(ptr);
				}
				if (itype) {
					if (u)
						*u = '[';
				}
				rem	-= (loc - ptr);
				ptr = loc;
				state = 0;
				break;

			case 3:			/* Handle file */
				loc = memchr(ptr, *boundary, rem);
				u = ptr;
				while (loc) {
					if (!strncmp(loc, boundary, len)
#if NEW_BOUNDARY_CHECK
						&& (loc-2>buf && *(loc-2)=='-' && *(loc-1)=='-') /* ensure boundary is prefixed with -- */
						&& (loc-2==buf || *(loc-3)=='\n') /* ensure beginning of line */
#endif
						) {
						break;
					}
					u = loc + 1;
					urem = rem - (loc - ptr) - 1;
					loc = memchr(u, *boundary, urem);
				}
				if (!loc) {
					php_error(E_WARNING, "File Upload Error - No Mime boundary found after start of file header");
					SAFE_RETURN;
				}
				fn = tempnam(PG(upload_tmp_dir), "php");
				if ((loc - ptr - 4) > PG(upload_max_filesize)) {
					php_error(E_WARNING, "Max file size of %ld bytes exceeded - file [%s] not saved", PG(upload_max_filesize),namebuf);
					bytes=0;	
					SET_VAR_STRING(namebuf, estrdup("none"));
				} else if (max_file_size && ((loc - ptr - 4) > max_file_size)) {
					php_error(E_WARNING, "Max file size exceeded - file [%s] not saved", namebuf);
					bytes = 0;
					SET_VAR_STRING(namebuf, estrdup("none"));
				} else if ((loc - ptr - 4) <= 0) {
					bytes = 0;
					SET_VAR_STRING(namebuf, estrdup("none"));
				} else {
					fp = fopen(fn, "w");
					if (!fp) {
						php_error(E_WARNING, "File Upload Error - Unable to open temporary file [%s]", fn);
						SAFE_RETURN;
					}
					bytes = fwrite(ptr, 1, loc - ptr - 4, fp);
					fclose(fp);
					php3_list_insert(fn,le_uploads);  /* Tell PHP about the file so the destructor can unlink it later */
					if (bytes < (loc - ptr - 4)) {
						php_error(E_WARNING, "Only %d bytes were written, expected to write %ld", bytes, loc - ptr - 4);
					}
					SET_VAR_STRING(namebuf, estrdup(fn));
				}
				sprintf(lbuf, "%s_size", namebuf);
				SET_VAR_LONG(lbuf, bytes);
				state = 0;
				rem -= (loc - ptr);
				ptr = loc;
				break;
		}
	}
	SAFE_RETURN;
}


SAPI_POST_READER_FUNC(rfc1867_post_reader)
{
	char *boundary;
	uint boundary_len;

	boundary = strstr(content_type_dup, "boundary");
	if (!boundary || !(boundary=strchr(boundary, '='))) {
		sapi_module.sapi_error(E_COMPILE_ERROR, "Missing boundary in multipart/form-data POST data");
		return;
	}
	boundary++;
	boundary_len = strlen(boundary);

	sapi_read_standard_form_data(content_type_dup SLS_CC);
	if (SG(request_info).post_data) {
		php_mime_split(SG(request_info).post_data, SG(request_info).post_data_length, boundary);
		efree(SG(request_info).post_data);
		SG(request_info).post_data = NULL;
	}
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
