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
  | Author: George Schlossnagle <george@omniti.com>                      |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"
#include "php_pdo_mysql.h"
#include "php_pdo_mysql_int.h"


static int pdo_mysql_stmt_dtor(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;

	if (S->result) {
		/* free the resource */
		mysql_free_result(S->result);
		S->result = NULL;
	}
	if (S->einfo.errmsg) {
		efree(S->einfo.errmsg);
		S->einfo.errmsg = NULL;
	}
	efree(S);
	return 1;
}

static int pdo_mysql_stmt_execute(pdo_stmt_t *stmt TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;
	pdo_mysql_db_handle *H = S->H;
	my_ulonglong row_count;

	/* ensure that we free any previous unfetched results */
	if (S->result) {
		mysql_free_result(S->result);
		S->result = NULL;
	}

	if (mysql_real_query(H->server, stmt->active_query_string, stmt->active_query_stringlen) != 0) {
		pdo_mysql_error_stmt(stmt);
		return 0;
	}

	row_count = mysql_affected_rows(H->server);
	if (row_count == (my_ulonglong)-1) {
		/* we either have a query that returned a result set or an error occured
		   lets see if we have access to a result set */
		if (!H->buffered) {
			S->result = mysql_use_result(H->server);
		} else {
			S->result = mysql_store_result(H->server);
		}
		if (NULL == S->result) {
			pdo_mysql_error_stmt(stmt);
			return 0;
		}

		stmt->row_count = 0;

		if (!stmt->executed) {
			stmt->column_count = (int) mysql_num_fields(S->result);
			S->fields = mysql_fetch_fields(S->result);
		}
	} else {
		/* this was a DML or DDL query (INSERT, UPDATE, DELETE, ... */
		stmt->row_count = row_count;
	}

	return 1;
}

static int pdo_mysql_stmt_param_hook(pdo_stmt_t *stmt, struct pdo_bound_param_data *param,
		enum pdo_param_event event_type TSRMLS_DC)
{
	return 1;
}

static int pdo_mysql_stmt_fetch(pdo_stmt_t *stmt,
	enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;
	if (!S->result) {
		return 0;	
	}
	if ((S->current_data = mysql_fetch_row(S->result)) == NULL) {
		if (mysql_errno(S->H->server)) {
			pdo_mysql_error_stmt(stmt);
		}
		return 0;
	} 
	S->current_lengths = mysql_fetch_lengths(S->result);
	return 1;	
}

static int pdo_mysql_stmt_describe(pdo_stmt_t *stmt, int colno TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;
	struct pdo_column_data *cols = stmt->columns;
	unsigned int i;

	if (!S->result) {
		return 0;	
	}

	if (colno >= stmt->column_count) {
		/* error invalid column */
		return 0;
	}

	/* fetch all on demand, this seems easiest 
	** if we've been here before bail out 
	*/
	if (cols[0].name) {
		return 1;
	}
	for(i=0; i < stmt->column_count; i++) {
		int namelen;
		namelen = strlen(S->fields[i].name);
		cols[i].precision = S->fields[i].decimals;
		cols[i].maxlen = S->fields[i].length;
		cols[i].namelen = namelen;
		cols[i].name = estrndup(S->fields[i].name, namelen + 1);
		cols[i].param_type = PDO_PARAM_STR;
	}
	return 1;
}

static int pdo_mysql_stmt_get_col(pdo_stmt_t *stmt, int colno, char **ptr, unsigned long *len, int *caller_frees TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;

	if (S->current_data == NULL || !S->result) {
		return 0;
	}
	if (colno >= stmt->column_count) {
		/* error invalid column */
		return 0;
	}
	*ptr = S->current_data[colno];
	*len = S->current_lengths[colno];
	return 1;
}

static char *type_to_name_native(int type)
{
#define PDO_MYSQL_NATIVE_TYPE_NAME(x)	case FIELD_TYPE_##x: return #x;

    switch (type) {
        PDO_MYSQL_NATIVE_TYPE_NAME(STRING)
        PDO_MYSQL_NATIVE_TYPE_NAME(VAR_STRING)
#ifdef MYSQL_HAS_TINY
        PDO_MYSQL_NATIVE_TYPE_NAME(TINY)
#endif
        PDO_MYSQL_NATIVE_TYPE_NAME(SHORT)
        PDO_MYSQL_NATIVE_TYPE_NAME(LONG)
        PDO_MYSQL_NATIVE_TYPE_NAME(LONGLONG)
        PDO_MYSQL_NATIVE_TYPE_NAME(INT24)
        PDO_MYSQL_NATIVE_TYPE_NAME(FLOAT)
        PDO_MYSQL_NATIVE_TYPE_NAME(DOUBLE)
        PDO_MYSQL_NATIVE_TYPE_NAME(DECIMAL)
        PDO_MYSQL_NATIVE_TYPE_NAME(TIMESTAMP)
#ifdef MYSQL_HAS_YEAR
        PDO_MYSQL_NATIVE_TYPE_NAME(YEAR)
#endif
        PDO_MYSQL_NATIVE_TYPE_NAME(DATE)
        PDO_MYSQL_NATIVE_TYPE_NAME(TIME)
        PDO_MYSQL_NATIVE_TYPE_NAME(DATETIME)
        PDO_MYSQL_NATIVE_TYPE_NAME(TINY_BLOB)
        PDO_MYSQL_NATIVE_TYPE_NAME(MEDIUM_BLOB)
        PDO_MYSQL_NATIVE_TYPE_NAME(LONG_BLOB)
        PDO_MYSQL_NATIVE_TYPE_NAME(BLOB)
        PDO_MYSQL_NATIVE_TYPE_NAME(NULL)
        default:
            return NULL;
    }
}

static int pdo_mysql_stmt_col_meta(pdo_stmt_t *stmt, long colno, zval *return_value TSRMLS_DC)
{
	pdo_mysql_stmt *S = (pdo_mysql_stmt*)stmt->driver_data;
	MYSQL_FIELD *F;
	zval *flags;
	char *str;
	
	if (!S->result) {
		return FAILURE;
	}
	if (colno >= stmt->column_count) {
		/* error invalid column */
		return FAILURE;
	}

	array_init(return_value);
	MAKE_STD_ZVAL(flags);
	array_init(flags);

	F = S->fields + colno;

	if (F->def) {
		add_assoc_string(return_value, "mysql:def", F->def, 1);
	}
	if (IS_NOT_NULL(F->flags)) {
		add_next_index_string(flags, "not_null", 1);
	}
	if (IS_PRI_KEY(F->flags)) {
		add_next_index_string(flags, "primary_key", 1);
	}
	if (F->flags & MULTIPLE_KEY_FLAG) {
		add_next_index_string(flags, "multiple_key", 1);
	}
	if (F->flags & UNIQUE_KEY_FLAG) {
		add_next_index_string(flags, "unique_key", 1);
	}
	if (IS_BLOB(F->flags)) {
		add_next_index_string(flags, "blob", 1);
	}
	str = type_to_name_native(F->type);
	if (str) {
		add_assoc_string(return_value, "native_type", str, 1);
	}

	add_assoc_zval(return_value, "flags", flags);
	return SUCCESS;
}

struct pdo_stmt_methods mysql_stmt_methods = {
	pdo_mysql_stmt_dtor,
	pdo_mysql_stmt_execute,
	pdo_mysql_stmt_fetch,
	pdo_mysql_stmt_describe,
	pdo_mysql_stmt_get_col,
	pdo_mysql_stmt_param_hook,
	NULL, /* set_attr */
	NULL, /* get_attr */
	pdo_mysql_stmt_col_meta
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
