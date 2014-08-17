/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Wez Furlong <wez@php.net>                                    |
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
#include "php_pdo_oci.h"
#include "php_pdo_oci_int.h"
#include "Zend/zend_exceptions.h"

static inline ub4 pdo_oci_sanitize_prefetch(long prefetch);

static int pdo_oci_fetch_error_func(pdo_dbh_t *dbh, pdo_stmt_t *stmt, zval *info TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;
	pdo_oci_error_info *einfo;

	einfo = &H->einfo;

	if (stmt) {
		pdo_oci_stmt *S = (pdo_oci_stmt*)stmt->driver_data;

		if (S->einfo.errmsg) {
			einfo = &S->einfo;
		}
	}

	if (einfo->errcode) {
		add_next_index_long(info, einfo->errcode);
		add_next_index_string(info, einfo->errmsg);
	}

	return 1;
}
/* }}} */

ub4 _oci_error(OCIError *err, pdo_dbh_t *dbh, pdo_stmt_t *stmt, char *what, sword status, int isinit, const char *file, int line TSRMLS_DC) /* {{{ */
{
	text errbuf[1024] = "<<Unknown>>";
	char tmp_buf[2048];
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;
	pdo_oci_error_info *einfo;
	pdo_oci_stmt *S = NULL;
	pdo_error_type *pdo_err = &dbh->error_code;

	if (stmt) {
		S = (pdo_oci_stmt*)stmt->driver_data;
		einfo = &S->einfo;
		pdo_err = &stmt->error_code;
	}
	else {
		einfo = &H->einfo;
	}

	if (einfo->errmsg) {
		pefree(einfo->errmsg, dbh->is_persistent);
	}

	einfo->errmsg = NULL;
	einfo->errcode = 0;
	einfo->file = file;
	einfo->line = line;

	if (isinit) { /* Initialization error */
		strcpy(*pdo_err, "HY000");
		slprintf(tmp_buf, sizeof(tmp_buf), "%s (%s:%d)", what, file, line);
		einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
	}
	else {
		switch (status) {
			case OCI_SUCCESS:
				strcpy(*pdo_err, "00000");
				break;
			case OCI_ERROR:
				OCIErrorGet(err, (ub4)1, NULL, &einfo->errcode, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: %s (%s:%d)", what, errbuf, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_SUCCESS_WITH_INFO:
				OCIErrorGet(err, (ub4)1, NULL, &einfo->errcode, errbuf, (ub4)sizeof(errbuf), OCI_HTYPE_ERROR);
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_SUCCESS_WITH_INFO: %s (%s:%d)", what, errbuf, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_NEED_DATA:
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_NEED_DATA (%s:%d)", what, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_NO_DATA:
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_NO_DATA (%s:%d)", what, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_INVALID_HANDLE:
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_INVALID_HANDLE (%s:%d)", what, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_STILL_EXECUTING:
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_STILL_EXECUTING (%s:%d)", what, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
			case OCI_CONTINUE:
				slprintf(tmp_buf, sizeof(tmp_buf), "%s: OCI_CONTINUE (%s:%d)", what, file, line);
				einfo->errmsg = pestrdup(tmp_buf, dbh->is_persistent);
				break;
		}

		if (einfo->errcode) {
			switch (einfo->errcode) {
				case 1013:	/* user requested cancel of current operation */
					zend_bailout();
					break;

#if 0
				case 955:	/* ORA-00955: name is already used by an existing object */
					*pdo_err = PDO_ERR_ALREADY_EXISTS;
					break;
#endif

				case 12154:	/* ORA-12154: TNS:could not resolve service name */
					strcpy(*pdo_err, "42S02");
					break;
				
				case	22:	/* ORA-00022: invalid session id */
				case   378:
				case   602:
				case   603:
				case   604:
				case   609:
				case  1012:	/* ORA-01012: */
				case  1033:
				case  1041:
				case  1043:
				case  1089:
				case  1090:
				case  1092:
				case  3113:	/* ORA-03133: end of file on communication channel */
				case  3114:
				case  3122:
				case  3135:
				case 12153:
				case 27146:
				case 28511:
					/* consider the connection closed */
					dbh->is_closed = 1;
					H->attached = 0;
					strcpy(*pdo_err, "01002"); /* FIXME */
					break;

				default:
					strcpy(*pdo_err, "HY000");
			}
		}

		if (stmt) {
			/* always propogate the error code back up to the dbh,
			 * so that we can catch the error information when execute
			 * is called via query.  See Bug #33707 */
			if (H->einfo.errmsg) {
				pefree(H->einfo.errmsg, dbh->is_persistent);
			}
			H->einfo = *einfo;
			H->einfo.errmsg = einfo->errmsg ? pestrdup(einfo->errmsg, dbh->is_persistent) : NULL;
			strcpy(dbh->error_code, stmt->error_code);
		}
	}

	/* little mini hack so that we can use this code from the dbh ctor */
	if (!dbh->methods) {
		zend_throw_exception_ex(php_pdo_get_exception(), einfo->errcode TSRMLS_CC, "SQLSTATE[%s]: %s", *pdo_err, einfo->errmsg);
	}

	return einfo->errcode;
}
/* }}} */

static int oci_handle_closer(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;

	if (H->svc) {
		/* rollback any outstanding work */
		OCITransRollback(H->svc, H->err, 0);
	}

	if (H->session) {
		OCIHandleFree(H->session, OCI_HTYPE_SESSION);
		H->session = NULL;
	}

	if (H->svc) {
		OCIHandleFree(H->svc, OCI_HTYPE_SVCCTX);
		H->svc = NULL;
	}

	if (H->server && H->attached) {
		H->last_err = OCIServerDetach(H->server, H->err, OCI_DEFAULT);
		if (H->last_err) {
			oci_drv_error("OCIServerDetach");
		}
		H->attached = 0;
	}

	if (H->server) {
		OCIHandleFree(H->server, OCI_HTYPE_SERVER);
		H->server = NULL;
	}

	if (H->err) {
		OCIHandleFree(H->err, OCI_HTYPE_ERROR);
		H->err = NULL;
	}

	if (H->charset && H->env) {
		OCIHandleFree(H->env, OCI_HTYPE_ENV);
		H->env = NULL;
	}

	if (H->einfo.errmsg) {
		pefree(H->einfo.errmsg, dbh->is_persistent);
		H->einfo.errmsg = NULL;
	}

	pefree(H, dbh->is_persistent);

	return 0;
}
/* }}} */

static int oci_handle_preparer(pdo_dbh_t *dbh, const char *sql, long sql_len, pdo_stmt_t *stmt, zval *driver_options TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;
	pdo_oci_stmt *S = ecalloc(1, sizeof(*S));
	ub4 prefetch;
	char *nsql = NULL;
	int nsql_len = 0;
	int ret;

#if HAVE_OCISTMTFETCH2
	S->exec_type = pdo_attr_lval(driver_options, PDO_ATTR_CURSOR,
		PDO_CURSOR_FWDONLY TSRMLS_CC) == PDO_CURSOR_SCROLL ?
		OCI_STMT_SCROLLABLE_READONLY : OCI_DEFAULT;
#else
	S->exec_type = OCI_DEFAULT;
#endif

	S->H = H;
	stmt->supports_placeholders = PDO_PLACEHOLDER_NAMED;
	ret = pdo_parse_params(stmt, (char*)sql, sql_len, &nsql, &nsql_len TSRMLS_CC);

	if (ret == 1) {
		/* query was re-written */
		sql = nsql;
		sql_len = nsql_len;
	} else if (ret == -1) {
		/* couldn't grok it */
		strcpy(dbh->error_code, stmt->error_code);
		efree(S);
		return 0;
	}

	/* create an OCI statement handle */
	OCIHandleAlloc(H->env, (dvoid*)&S->stmt, OCI_HTYPE_STMT, 0, NULL);

	/* and our own private error handle */
	OCIHandleAlloc(H->env, (dvoid*)&S->err, OCI_HTYPE_ERROR, 0, NULL);

	if (sql_len) {
		H->last_err = OCIStmtPrepare(S->stmt, H->err, (text*)sql, sql_len, OCI_NTV_SYNTAX, OCI_DEFAULT);
		if (nsql) {
			efree(nsql);
			nsql = NULL;
		}
		if (H->last_err) {
			H->last_err = oci_drv_error("OCIStmtPrepare");
			OCIHandleFree(S->stmt, OCI_HTYPE_STMT);
			OCIHandleFree(S->err, OCI_HTYPE_ERROR);
			efree(S);
			return 0;
		}

	}

	prefetch = pdo_oci_sanitize_prefetch(pdo_attr_lval(driver_options, PDO_ATTR_PREFETCH, PDO_OCI_PREFETCH_DEFAULT TSRMLS_CC));
	if (prefetch) {
		H->last_err = OCIAttrSet(S->stmt, OCI_HTYPE_STMT, &prefetch, 0,
			OCI_ATTR_PREFETCH_ROWS, H->err);
		if (!H->last_err) {
			prefetch *= PDO_OCI_PREFETCH_ROWSIZE;
			H->last_err = OCIAttrSet(S->stmt, OCI_HTYPE_STMT, &prefetch, 0,
				OCI_ATTR_PREFETCH_MEMORY, H->err);
		}
	}

	stmt->driver_data = S;
	stmt->methods = &oci_stmt_methods;
	if (nsql) {
		efree(nsql);
		nsql = NULL;
	}

	return 1;
}
/* }}} */

static long oci_handle_doer(pdo_dbh_t *dbh, const char *sql, long sql_len TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;
	OCIStmt		*stmt;
	ub2 stmt_type;
	ub4 rowcount;
	int ret = -1;

	OCIHandleAlloc(H->env, (dvoid*)&stmt, OCI_HTYPE_STMT, 0, NULL);

	H->last_err = OCIStmtPrepare(stmt, H->err, (text*)sql, sql_len, OCI_NTV_SYNTAX, OCI_DEFAULT);
	if (H->last_err) {
		H->last_err = oci_drv_error("OCIStmtPrepare");
		OCIHandleFree(stmt, OCI_HTYPE_STMT);
		return -1;
	}

	H->last_err = OCIAttrGet(stmt, OCI_HTYPE_STMT, &stmt_type, 0, OCI_ATTR_STMT_TYPE, H->err);

	if (stmt_type == OCI_STMT_SELECT) {
		/* invalid usage; cancel it */
		OCIHandleFree(stmt, OCI_HTYPE_STMT);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "issuing a SELECT query here is invalid");
		return -1;
	}

	/* now we are good to go */
	H->last_err = OCIStmtExecute(H->svc, stmt, H->err, 1, 0, NULL, NULL,
			(dbh->auto_commit && !dbh->in_txn) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT);

	if (H->last_err) {
		H->last_err = oci_drv_error("OCIStmtExecute");
	} else {
		/* return the number of affected rows */
		H->last_err = OCIAttrGet(stmt, OCI_HTYPE_STMT, &rowcount, 0, OCI_ATTR_ROW_COUNT, H->err);
		ret = rowcount;
	}

	OCIHandleFree(stmt, OCI_HTYPE_STMT);

	return ret;
}
/* }}} */

static int oci_handle_quoter(pdo_dbh_t *dbh, const char *unquoted, int unquotedlen, char **quoted, int *quotedlen, enum pdo_param_type paramtype  TSRMLS_DC) /* {{{ */
{
	int qcount = 0;
	char const *cu, *l, *r;
	char *c;

	if (!unquotedlen) {
		*quotedlen = 2;
		*quoted = emalloc(*quotedlen+1);
		strcpy(*quoted, "''");
		return 1;
	}

	/* count single quotes */
	for (cu = unquoted; (cu = strchr(cu,'\'')); qcount++, cu++)
		; /* empty loop */

	*quotedlen = unquotedlen + qcount + 2;
	*quoted = c = emalloc(*quotedlen+1);
	*c++ = '\'';
	
	/* foreach (chunk that ends in a quote) */
	for (l = unquoted; (r = strchr(l,'\'')); l = r+1) {
		strncpy(c, l, r-l+1);
		c += (r-l+1);		
		*c++ = '\'';			/* add second quote */
	}

    /* Copy remainder and add enclosing quote */	
	strncpy(c, l, *quotedlen-(c-*quoted)-1);
	(*quoted)[*quotedlen-1] = '\''; 
	(*quoted)[*quotedlen]   = '\0';
	
	return 1;
}
/* }}} */

static int oci_handle_begin(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	/* with Oracle, there is nothing special to be done */
	return 1;
}
/* }}} */

static int oci_handle_commit(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;

	H->last_err = OCITransCommit(H->svc, H->err, 0);

	if (H->last_err) {
		H->last_err = oci_drv_error("OCITransCommit");
		return 0;
	}
	return 1;
}
/* }}} */

static int oci_handle_rollback(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;

	H->last_err = OCITransRollback(H->svc, H->err, 0);

	if (H->last_err) {
		H->last_err = oci_drv_error("OCITransRollback");
		return 0;
	}
	return 1;
}
/* }}} */

static int oci_handle_set_attribute(pdo_dbh_t *dbh, long attr, zval *val TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;

	if (attr == PDO_ATTR_AUTOCOMMIT) {
		if (dbh->in_txn) {
			/* Assume they want to commit whatever is outstanding */
			H->last_err = OCITransCommit(H->svc, H->err, 0);

			if (H->last_err) {
				H->last_err = oci_drv_error("OCITransCommit");
				return 0;
			}
			dbh->in_txn = 0;
		}

		convert_to_long(val);

		dbh->auto_commit = Z_LVAL_P(val);
		return 1;
	} else {
		return 0;
	}
	
}
/* }}} */

static int oci_handle_get_attribute(pdo_dbh_t *dbh, long attr, zval *return_value TSRMLS_DC)  /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;

	switch (attr) {
		case PDO_ATTR_SERVER_VERSION:
		case PDO_ATTR_SERVER_INFO:
		{
			text infostr[512];
			char verstr[15];
			ub4  vernum;
			
			if (OCIServerRelease(H->svc, H->err, infostr, (ub4)sizeof(infostr), (ub1)OCI_HTYPE_SVCCTX, &vernum))
			{
				ZVAL_STRING(return_value, "<<Unknown>>", 1);
			} else {
				if (attr == PDO_ATTR_SERVER_INFO) {
					ZVAL_STRING(return_value, (char *)infostr, 1);
				} else {
					slprintf(verstr, sizeof(verstr), "%d.%d.%d.%d.%d", 
							 (int)((vernum>>24) & 0xFF),  /* version number */
							 (int)((vernum>>20) & 0x0F),  /* release number*/
							 (int)((vernum>>12) & 0xFF),  /* update number */
							 (int)((vernum>>8)  & 0x0F),  /* port release number */
							 (int)((vernum>>0)  & 0xFF)); /* port update number */
					
					ZVAL_STRING(return_value, verstr, 1);
				}
			}
			return TRUE;
		}

		case PDO_ATTR_CLIENT_VERSION:
		{
#if OCI_MAJOR_VERSION > 10 || (OCI_MAJOR_VERSION == 10 && OCI_MINOR_VERSION >= 2)
			/* Run time client version */
			sword major, minor, update, patch, port_update;
			char verstr[15];

			OCIClientVersion(&major, &minor, &update, &patch, &port_update);
			slprintf(verstr, sizeof(verstr), "%d.%d.%d.%d.%d", major, minor, update, patch, port_update);
			ZVAL_STRING(return_value, verstr, 1);
#elif defined(PHP_PDO_OCI_CLIENT_VERSION)
			/* Compile time client version */
			ZVAL_STRING(return_value, PHP_PDO_OCI_CLIENT_VERSION, 1);
#else
			return FALSE;

#endif /* Check for OCIClientVersion() support */

			return TRUE;
		}

		case PDO_ATTR_AUTOCOMMIT:
			ZVAL_BOOL(return_value, dbh->auto_commit);
			return TRUE;

		default:
			return FALSE;

	}
	return FALSE;

}
/* }}} */

static int pdo_oci_check_liveness(pdo_dbh_t *dbh TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H = (pdo_oci_db_handle *)dbh->driver_data;
	sb4 error_code = 0;
#if (!((OCI_MAJOR_VERSION > 10) || ((OCI_MAJOR_VERSION == 10) && (OCI_MINOR_VERSION >= 2))))
	char version[256];
#endif

	/* TODO move attached check to PDO level */
	if (H->attached == 0) {
		return FAILURE;
	}
	/* TODO add persistent_timeout check at PDO level */


	/* Use OCIPing instead of OCIServerVersion. If OCIPing returns ORA-1010 (invalid OCI operation)
	 * such as from Pre-10.1 servers, the error is still from the server and we would have
	 * successfully performed a roundtrip and validated the connection. Use OCIServerVersion for
	 * Pre-10.2 clients
	 */	
#if ((OCI_MAJOR_VERSION > 10) || ((OCI_MAJOR_VERSION == 10) && (OCI_MINOR_VERSION >= 2)))	/* OCIPing available 10.2 onwards */
	H->last_err = OCIPing (H->svc, H->err, OCI_DEFAULT);
#else
	/* use good old OCIServerVersion() */
	H->last_err = OCIServerVersion (H->svc, H->err, (text *)version, sizeof(version), OCI_HTYPE_SVCCTX);
#endif
	if (H->last_err == OCI_SUCCESS) {
		return SUCCESS;
	}

	OCIErrorGet (H->err, (ub4)1, NULL, &error_code, NULL, 0, OCI_HTYPE_ERROR);
	
	if (error_code == 1010) {
		return SUCCESS;
	}
	return FAILURE;
}
/* }}} */

static struct pdo_dbh_methods oci_methods = {
	oci_handle_closer,
	oci_handle_preparer,
	oci_handle_doer,
	oci_handle_quoter,
	oci_handle_begin,
	oci_handle_commit,
	oci_handle_rollback,
	oci_handle_set_attribute,
	NULL,
	pdo_oci_fetch_error_func,
	oci_handle_get_attribute,
	pdo_oci_check_liveness,	/* check_liveness */
	NULL	/* get_driver_methods */
};

static int pdo_oci_handle_factory(pdo_dbh_t *dbh, zval *driver_options TSRMLS_DC) /* {{{ */
{
	pdo_oci_db_handle *H;
	int i, ret = 0;
	struct pdo_data_src_parser vars[] = {
		{ "charset",  NULL,	0 },
		{ "dbname",   "",	0 }
	};

	php_pdo_parse_data_source(dbh->data_source, dbh->data_source_len, vars, 2);

	H = pecalloc(1, sizeof(*H), dbh->is_persistent);
	dbh->driver_data = H;

	/* allocate an environment */
#if HAVE_OCIENVNLSCREATE
	if (vars[0].optval) {
		H->charset = OCINlsCharSetNameToId(pdo_oci_Env, (const oratext *)vars[0].optval);
		if (!H->charset) {
			oci_init_error("OCINlsCharSetNameToId: unknown character set name");
			goto cleanup;
		} else {
			if (OCIEnvNlsCreate(&H->env, PDO_OCI_INIT_MODE, 0, NULL, NULL, NULL, 0, NULL, H->charset, H->charset) != OCI_SUCCESS) {
				oci_init_error("OCIEnvNlsCreate: Check the character set is valid and that PHP has access to Oracle libraries and NLS data");
				goto cleanup;
			}
		}
	}
#endif
	if (H->env == NULL) {
		/* use the global environment */
		H->env = pdo_oci_Env;
	}

	/* something to hold errors */
	OCIHandleAlloc(H->env, (dvoid **)&H->err, OCI_HTYPE_ERROR, 0, NULL);

	/* handle for the server */
	OCIHandleAlloc(H->env, (dvoid **)&H->server, OCI_HTYPE_SERVER, 0, NULL);

	H->last_err = OCIServerAttach(H->server, H->err, (text*)vars[1].optval,
		   	strlen(vars[1].optval), OCI_DEFAULT);

	if (H->last_err) {
		oci_drv_error("pdo_oci_handle_factory");
		goto cleanup;
	}

	H->attached = 1;

	/* create a service context */
	H->last_err = OCIHandleAlloc(H->env, (dvoid**)&H->svc, OCI_HTYPE_SVCCTX, 0, NULL);
	if (H->last_err) {
		oci_drv_error("OCIHandleAlloc: OCI_HTYPE_SVCCTX");
		goto cleanup;
	}

	H->last_err = OCIHandleAlloc(H->env, (dvoid**)&H->session, OCI_HTYPE_SESSION, 0, NULL);
	if (H->last_err) {
		oci_drv_error("OCIHandleAlloc: OCI_HTYPE_SESSION");
		goto cleanup;
	}

	/* set server handle into service handle */
	H->last_err = OCIAttrSet(H->svc, OCI_HTYPE_SVCCTX, H->server, 0, OCI_ATTR_SERVER, H->err);
	if (H->last_err) {
		oci_drv_error("OCIAttrSet: OCI_ATTR_SERVER");
		goto cleanup;
	}

	/* username */
	if (dbh->username) {
		H->last_err = OCIAttrSet(H->session, OCI_HTYPE_SESSION,
			   	dbh->username, strlen(dbh->username),
				OCI_ATTR_USERNAME, H->err);
		if (H->last_err) {
			oci_drv_error("OCIAttrSet: OCI_ATTR_USERNAME");
			goto cleanup;
		}
	}

	/* password */
	if (dbh->password) {
		H->last_err = OCIAttrSet(H->session, OCI_HTYPE_SESSION,
			   	dbh->password, strlen(dbh->password),
				OCI_ATTR_PASSWORD, H->err);
		if (H->last_err) {
			oci_drv_error("OCIAttrSet: OCI_ATTR_PASSWORD");
			goto cleanup;
		}
	}

	/* Now fire up the session */
	H->last_err = OCISessionBegin(H->svc, H->err, H->session, OCI_CRED_RDBMS, OCI_DEFAULT);
	if (H->last_err) {
		oci_drv_error("OCISessionBegin");
		goto cleanup;
	}

	/* set the server handle into service handle */
	H->last_err = OCIAttrSet(H->svc, OCI_HTYPE_SVCCTX, H->session, 0, OCI_ATTR_SESSION, H->err);
	if (H->last_err) {
		oci_drv_error("OCIAttrSet: OCI_ATTR_SESSION");
		goto cleanup;
	}

	dbh->methods = &oci_methods;
	dbh->alloc_own_columns = 1;
	dbh->native_case = PDO_CASE_UPPER;

	ret = 1;

cleanup:
	for (i = 0; i < sizeof(vars)/sizeof(vars[0]); i++) {
		if (vars[i].freeme) {
			efree(vars[i].optval);
		}
	}

	if (!ret) {
		oci_handle_closer(dbh TSRMLS_CC);
	}

	return ret;
}
/* }}} */

pdo_driver_t pdo_oci_driver = {
	PDO_DRIVER_HEADER(oci),
	pdo_oci_handle_factory
};

static inline ub4 pdo_oci_sanitize_prefetch(long prefetch) /* {{{ */
{
	if (prefetch < 0) {
		prefetch = 0;
	} else if (prefetch > UB4MAXVAL / PDO_OCI_PREFETCH_ROWSIZE) {
		prefetch = PDO_OCI_PREFETCH_DEFAULT;
	}
	return ((ub4)prefetch);
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
