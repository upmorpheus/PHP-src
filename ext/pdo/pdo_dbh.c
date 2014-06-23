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
  |         Marcus Boerger <helly@php.net>                               |
  |         Sterling Hughes <sterling@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

/* The PDO Database Handle Class */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_pdo.h"
#include "php_pdo_driver.h"
#include "php_pdo_int.h"
#include "zend_exceptions.h"
#include "zend_object_handlers.h"
#include "zend_hash.h"

static int pdo_dbh_attribute_set(pdo_dbh_t *dbh, long attr, zval *value TSRMLS_DC);

void pdo_raise_impl_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *sqlstate, const char *supp TSRMLS_DC) /* {{{ */
{
	pdo_error_type *pdo_err = &dbh->error_code;
	char *message = NULL;
	const char *msg;

	if (dbh && dbh->error_mode == PDO_ERRMODE_SILENT) {
#if 0
		/* BUG: if user is running in silent mode and hits an error at the driver level
		 * when they use the PDO methods to call up the error information, they may
		 * get bogus information */
		return;
#endif
	}
	
	if (stmt) {
		pdo_err = &stmt->error_code;
	}

	strncpy(*pdo_err, sqlstate, 6);

	/* hash sqlstate to error messages */
	msg = pdo_sqlstate_state_to_description(*pdo_err);
	if (!msg) {
		msg = "<<Unknown error>>";
	}

	if (supp) {
		spprintf(&message, 0, "SQLSTATE[%s]: %s: %s", *pdo_err, msg, supp);
	} else {
		spprintf(&message, 0, "SQLSTATE[%s]: %s", *pdo_err, msg);
	}

	if (dbh && dbh->error_mode != PDO_ERRMODE_EXCEPTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", message);
	} else {
		zval ex, info;
		zend_class_entry *def_ex = php_pdo_get_exception_base(1 TSRMLS_CC), *pdo_ex = php_pdo_get_exception();

		object_init_ex(&ex, pdo_ex);

		zend_update_property_string(def_ex, &ex, "message", sizeof("message")-1, message TSRMLS_CC);
		zend_update_property_string(def_ex, &ex, "code", sizeof("code")-1, *pdo_err TSRMLS_CC);
		
		array_init(&info);

		add_next_index_string(&info, *pdo_err);
		add_next_index_long(&info, 0);
		zend_update_property(pdo_ex, &ex, "errorInfo", sizeof("errorInfo")-1, &info TSRMLS_CC);
		zval_ptr_dtor(&info);

		zend_throw_exception_object(&ex TSRMLS_CC);
	}
	
	if (message) {
		efree(message);
	}
}
/* }}} */

PDO_API void pdo_handle_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt TSRMLS_DC) /* {{{ */
{
	pdo_error_type *pdo_err = &dbh->error_code;
	const char *msg = "<<Unknown>>";
	char *supp = NULL;
	long native_code = 0;
	zend_string *message = NULL;
	zval info;

	if (dbh == NULL || dbh->error_mode == PDO_ERRMODE_SILENT) {
		return;
	}
	
	if (stmt) {
		pdo_err = &stmt->error_code;
	}

	/* hash sqlstate to error messages */
	msg = pdo_sqlstate_state_to_description(*pdo_err);
	if (!msg) {
		msg = "<<Unknown error>>";
	}

	ZVAL_UNDEF(&info);
	if (dbh->methods->fetch_err) {
		array_init(&info);

		add_next_index_string(&info, *pdo_err);
		
		if (dbh->methods->fetch_err(dbh, stmt, &info TSRMLS_CC)) {
			zval *item;

			if ((item = zend_hash_index_find(Z_ARRVAL(info), 1)) != NULL) {
				native_code = Z_LVAL_P(item);
			}
			
			if ((item = zend_hash_index_find(Z_ARRVAL(info), 2)) != NULL) {
				supp = estrndup(Z_STRVAL_P(item), Z_STRLEN_P(item));
			}
		}
	}

	if (supp) {
		message = strpprintf(0, "SQLSTATE[%s]: %s: %ld %s", *pdo_err, msg, native_code, supp);
	} else {
		message = strpprintf(0, "SQLSTATE[%s]: %s", *pdo_err, msg);
	}

	if (dbh->error_mode == PDO_ERRMODE_WARNING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", message->val);
	} else if (EG(exception) == NULL) {
		zval ex;
		zend_class_entry *def_ex = php_pdo_get_exception_base(1 TSRMLS_CC), *pdo_ex = php_pdo_get_exception();

		object_init_ex(&ex, pdo_ex);

		zend_update_property_str(def_ex, &ex, "message", sizeof("message") - 1, message TSRMLS_CC);
		zend_update_property_string(def_ex, &ex, "code", sizeof("code") - 1, *pdo_err TSRMLS_CC);
		
		if (!Z_ISUNDEF(info)) {
			zend_update_property(pdo_ex, &ex, "errorInfo", sizeof("errorInfo") - 1, &info TSRMLS_CC);
		}

		zend_throw_exception_object(&ex TSRMLS_CC);
	}

	if (!Z_ISUNDEF(info)) {
		zval_ptr_dtor(&info);
	}

	if (message) {
		STR_RELEASE(message);
	}

	if (supp) {
		efree(supp);
	}
}
/* }}} */

static char *dsn_from_uri(char *uri, char *buf, size_t buflen TSRMLS_DC) /* {{{ */
{
	php_stream *stream;
	char *dsn = NULL;

	stream = php_stream_open_wrapper(uri, "rb", REPORT_ERRORS, NULL);
	if (stream) {
		dsn = php_stream_get_line(stream, buf, buflen, NULL);
		php_stream_close(stream);
	}
	return dsn;
}
/* }}} */

/* {{{ proto void PDO::__construct(string dsn[, string username[, string passwd [, array options]]])
   */
static PHP_METHOD(PDO, dbh_constructor)
{
	zval *object = getThis();
	pdo_dbh_t *dbh = NULL;
	zend_bool is_persistent = 0;
	char *data_source;
	int data_source_len;
	char *colon;
	char *username=NULL, *password=NULL;
	int usernamelen, passwordlen;
	pdo_driver_t *driver = NULL;
	zval *options = NULL;
	char alt_dsn[512];
	int call_factory = 1;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s!s!a!", &data_source, &data_source_len,
				&username, &usernamelen, &password, &passwordlen, &options)) {
		Z_OBJ_P(object) = NULL;
		return;
	}

	/* parse the data source name */
	colon = strchr(data_source, ':');

	if (!colon) {
		/* let's see if this string has a matching dsn in the php.ini */
		char *ini_dsn = NULL;

		snprintf(alt_dsn, sizeof(alt_dsn), "pdo.dsn.%s", data_source);
		if (FAILURE == cfg_get_string(alt_dsn, &ini_dsn)) {
			zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "invalid data source name");
			Z_OBJ_P(object) = NULL;
			return;
		}

		data_source = ini_dsn;
		colon = strchr(data_source, ':');
		
		if (!colon) {
			zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "invalid data source name (via INI: %s)", alt_dsn);
			Z_OBJ_P(object) = NULL;
			return;
		}
	}

	if (!strncmp(data_source, "uri:", sizeof("uri:")-1)) {
		/* the specified URI holds connection details */
		data_source = dsn_from_uri(data_source + sizeof("uri:")-1, alt_dsn, sizeof(alt_dsn) TSRMLS_CC);
		if (!data_source) {
			zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "invalid data source URI");
			Z_OBJ_P(object) = NULL;
			return;
		}
		colon = strchr(data_source, ':');
		if (!colon) {
			zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "invalid data source name (via URI)");
			Z_OBJ_P(object) = NULL;
			return;
		}
	}

	driver = pdo_find_driver(data_source, colon - data_source);

	if (!driver) {
		/* NB: don't want to include the data_source in the error message as
		 * it might contain a password */
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "could not find driver");
		Z_OBJ_P(object) = NULL;
		return;
	}
	
	dbh = Z_PDO_DBH_P(object);

	/* is this supposed to be a persistent connection ? */
	if (options) {
		int plen = 0;
		char *hashkey = NULL;
		zend_resource *le;
		pdo_dbh_t *pdbh = NULL;
		zval *v;

		if ((v = zend_hash_index_find(Z_ARRVAL_P(options), PDO_ATTR_PERSISTENT)) != NULL) {
			if (Z_TYPE_P(v) == IS_STRING &&
				!is_numeric_string(Z_STRVAL_P(v), Z_STRLEN_P(v), NULL, NULL, 0) && Z_STRLEN_P(v) > 0) {
				/* user specified key */
				plen = spprintf(&hashkey, 0, "PDO:DBH:DSN=%s:%s:%s:%s", data_source,
						username ? username : "",
						password ? password : "",
						Z_STRVAL_P(v));
				is_persistent = 1;
			} else {
				convert_to_long_ex(v);
				is_persistent = Z_LVAL_P(v) ? 1 : 0;
				plen = spprintf(&hashkey, 0, "PDO:DBH:DSN=%s:%s:%s", data_source,
						username ? username : "",
						password ? password : "");
			}
		}

		if (is_persistent) {
			/* let's see if we have one cached.... */
			if ((le = zend_hash_str_find_ptr(&EG(persistent_list), hashkey, plen)) != NULL) {
				if (le->type == php_pdo_list_entry()) {
					pdbh = (pdo_dbh_t*)le->ptr;

					/* is the connection still alive ? */
					if (pdbh->methods->check_liveness && FAILURE == (pdbh->methods->check_liveness)(pdbh TSRMLS_CC)) {
						/* nope... need to kill it */
						/*??? memory leak */
						zend_list_close(le);
						pdbh = NULL;
					}
				}
			}

			if (pdbh) {
				call_factory = 0;
			} else {
				/* need a brand new pdbh */
				pdbh = pecalloc(1, sizeof(*pdbh), 1);

				if (!pdbh) {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "out of memory while allocating PDO handle");
					/* NOTREACHED */
				}

				pdbh->is_persistent = 1;
				if (!(pdbh->persistent_id = pemalloc(plen + 1, 1))) {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "out of memory while allocating PDO handle");
				}
				memcpy((char *)pdbh->persistent_id, hashkey, plen+1);
				pdbh->persistent_id_len = plen;
				pdbh->def_stmt_ce = dbh->def_stmt_ce;
			}
		}

		if (pdbh) {
			efree(dbh);
			/* switch over to the persistent one */
			Z_PDO_OBJECT_P(object)->inner = pdbh;
			dbh = pdbh;
		}

		if (hashkey) {
			efree(hashkey);
		}
	}
	
	if (call_factory) {
		dbh->data_source_len = strlen(colon + 1);
		dbh->data_source = (const char*)pestrdup(colon + 1, is_persistent);
		dbh->username = username ? pestrdup(username, is_persistent) : NULL;
		dbh->password = password ? pestrdup(password, is_persistent) : NULL;
		dbh->default_fetch_type = PDO_FETCH_BOTH;
	}	

	dbh->auto_commit = pdo_attr_lval(options, PDO_ATTR_AUTOCOMMIT, 1 TSRMLS_CC);

	if (!dbh->data_source || (username && !dbh->username) || (password && !dbh->password)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "out of memory");
	}

	if (!call_factory) {
		/* we got a persistent guy from our cache */
		goto options;
	}

	if (driver->db_handle_factory(dbh, options TSRMLS_CC)) {
		/* all set */

		if (is_persistent) {
			zend_resource le;

			/* register in the persistent list etc. */
			/* we should also need to replace the object store entry,
			   since it was created with emalloc */

			le.type = php_pdo_list_entry();
			le.ptr = dbh;
			GC_REFCOUNT(&le) = 1;

			if ((zend_hash_str_update_mem(&EG(persistent_list), 
						(char*)dbh->persistent_id, dbh->persistent_id_len, &le, sizeof(le))) == NULL) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Failed to register persistent entry");
			}
		}

		dbh->driver = driver;
options:
		if (options) {
			zval *attr_value;
			ulong long_key;
			zend_string *str_key = NULL;

			ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(options), long_key, str_key, attr_value) {
				if (str_key) {
					continue;
				}
				pdo_dbh_attribute_set(dbh, long_key, attr_value TSRMLS_CC);
			} ZEND_HASH_FOREACH_END();
		}

		return;
	}

	/* the connection failed; things will tidy up in free_storage */
	/* XXX raise exception */
	Z_OBJ_P(object) = NULL;
}
/* }}} */

static zval *pdo_stmt_instantiate(pdo_dbh_t *dbh, zval *object, zend_class_entry *dbstmt_ce, zval *ctor_args TSRMLS_DC) /* {{{ */
{
	if (!Z_ISUNDEF_P(ctor_args)) {
		if (Z_TYPE_P(ctor_args) != IS_ARRAY) {
			pdo_raise_impl_error(dbh, NULL, "HY000", "constructor arguments must be passed as an array" TSRMLS_CC);
			return NULL;
		}
		if (!dbstmt_ce->constructor) {
			pdo_raise_impl_error(dbh, NULL, "HY000", "user-supplied statement does not accept constructor arguments" TSRMLS_CC);
			return NULL;
		}
	}

	object_init_ex(object, dbstmt_ce);
	// ??? Z_SET_REFCOUNT_P(object, 1);
	//Z_SET_ISREF_P(object);
	
	return object;
} /* }}} */

static void pdo_stmt_construct(pdo_stmt_t *stmt, zval *object, zend_class_entry *dbstmt_ce, zval *ctor_args TSRMLS_DC) /* {{{ */
{	
	zval query_string;
	zval z_key;

	ZVAL_STRINGL(&query_string, stmt->query_string, stmt->query_stringlen);
	ZVAL_STRINGL(&z_key, "queryString", sizeof("queryString") - 1);
	std_object_handlers.write_property(object, &z_key, &query_string, -1 TSRMLS_CC);
	zval_ptr_dtor(&query_string);
	zval_ptr_dtor(&z_key);

	if (dbstmt_ce->constructor) {
		zend_fcall_info fci;
		zend_fcall_info_cache fcc;
		zval retval;

		fci.size = sizeof(zend_fcall_info);
		fci.function_table = &dbstmt_ce->function_table;
		ZVAL_UNDEF(&fci.function_name);
		fci.object = Z_OBJ_P(object);
		fci.symbol_table = NULL;
		fci.retval = &retval;
		fci.param_count = 0;
		fci.params = NULL;
		fci.no_separation = 1;

		zend_fcall_info_args(&fci, ctor_args TSRMLS_CC);

		fcc.initialized = 1;
		fcc.function_handler = dbstmt_ce->constructor;
		fcc.calling_scope = EG(scope);
		fcc.called_scope = Z_OBJCE_P(object);
		fcc.object = Z_OBJ_P(object);

		if (zend_call_function(&fci, &fcc TSRMLS_CC) == FAILURE) {
			Z_OBJ_P(object) = NULL;
			object = NULL; /* marks failure */
		} else if (!Z_ISUNDEF(retval)) {
			zval_ptr_dtor(&retval);
		}
			
		if (fci.params) {
			efree(fci.params);
		}
	}
}
/* }}} */

/* {{{ proto object PDO::prepare(string statment [, array options])
   Prepares a statement for execution and returns a statement object */
static PHP_METHOD(PDO, prepare)
{
	pdo_stmt_t *stmt;
	char *statement;
	int statement_len;
	zval *options = NULL, *opt, *item, ctor_args;
	zend_class_entry *dbstmt_ce, *pce;
	pdo_dbh_object_t *dbh_obj = Z_PDO_OBJECT_P(getThis());
	pdo_dbh_t *dbh = dbh_obj->inner;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &statement,
			&statement_len, &options)) {
		RETURN_FALSE;
	}
	
	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;

	if (ZEND_NUM_ARGS() > 1 && (opt = zend_hash_index_find(Z_ARRVAL_P(options), PDO_ATTR_STATEMENT_CLASS)) != NULL) {
		if (Z_TYPE_P(opt) != IS_ARRAY || (item = zend_hash_index_find(Z_ARRVAL_P(opt), 0)) == NULL
			|| Z_TYPE_P(item) != IS_STRING
			|| (pce = zend_lookup_class(Z_STR_P(item) TSRMLS_CC)) == NULL
		) {
			pdo_raise_impl_error(dbh, NULL, "HY000", 
				"PDO::ATTR_STATEMENT_CLASS requires format array(classname, array(ctor_args)); "
				"the classname must be a string specifying an existing class"
				TSRMLS_CC);
			PDO_HANDLE_DBH_ERR();
			RETURN_FALSE;
		}
		dbstmt_ce = pce;
		if (!instanceof_function(dbstmt_ce, pdo_dbstmt_ce TSRMLS_CC)) {
			pdo_raise_impl_error(dbh, NULL, "HY000", 
				"user-supplied statement class must be derived from PDOStatement" TSRMLS_CC);
			PDO_HANDLE_DBH_ERR();
			RETURN_FALSE;
		}
		if (dbstmt_ce->constructor && !(dbstmt_ce->constructor->common.fn_flags & (ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED))) {
			pdo_raise_impl_error(dbh, NULL, "HY000", 
				"user-supplied statement class cannot have a public constructor" TSRMLS_CC);
			PDO_HANDLE_DBH_ERR();
			RETURN_FALSE;
		}
		if ((item = zend_hash_index_find(Z_ARRVAL_P(opt), 1)) != NULL) {
			if (Z_TYPE_P(item) != IS_ARRAY) {
				pdo_raise_impl_error(dbh, NULL, "HY000", 
					"PDO::ATTR_STATEMENT_CLASS requires format array(classname, ctor_args); "
					"ctor_args must be an array"
				TSRMLS_CC);
				PDO_HANDLE_DBH_ERR();
				RETURN_FALSE;
			}
			ZVAL_COPY_VALUE(&ctor_args, item);
		} else {
			ZVAL_UNDEF(&ctor_args);
		}
	} else {
		dbstmt_ce = dbh->def_stmt_ce;
		ZVAL_COPY_VALUE(&ctor_args, &dbh->def_stmt_ctor_args);
	}

	if (!pdo_stmt_instantiate(dbh, return_value, dbstmt_ce, &ctor_args TSRMLS_CC)) {
		pdo_raise_impl_error(dbh, NULL, "HY000", 
			"failed to instantiate user-supplied statement class"
			TSRMLS_CC);
		PDO_HANDLE_DBH_ERR();
		RETURN_FALSE;
	}
	stmt = Z_PDO_STMT_P(return_value);
	
	/* unconditionally keep this for later reference */
	stmt->query_string = estrndup(statement, statement_len);
	stmt->query_stringlen = statement_len;
	stmt->default_fetch_type = dbh->default_fetch_type;
	stmt->dbh = dbh;
	/* give it a reference to me */
	ZVAL_OBJ(&stmt->database_object_handle, &dbh_obj->std);
	Z_ADDREF(stmt->database_object_handle);
	/* we haven't created a lazy object yet */
	ZVAL_UNDEF(&stmt->lazy_object_ref);

	if (dbh->methods->preparer(dbh, statement, statement_len, stmt, options TSRMLS_CC)) {
		pdo_stmt_construct(stmt, return_value, dbstmt_ce, &ctor_args TSRMLS_CC);
		return;
	}

	PDO_HANDLE_DBH_ERR();

	/* kill the object handle for the stmt here */
	zval_dtor(return_value);

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::beginTransaction()
   Initiates a transaction */
static PHP_METHOD(PDO, beginTransaction)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	PDO_CONSTRUCT_CHECK;

	if (dbh->in_txn) {
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "There is already an active transaction");
		RETURN_FALSE;
	}
	
	if (!dbh->methods->begin) {
		/* TODO: this should be an exception; see the auto-commit mode
		 * comments below */
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "This driver doesn't support transactions");
		RETURN_FALSE;
	}

	if (dbh->methods->begin(dbh TSRMLS_CC)) {
		dbh->in_txn = 1;
		RETURN_TRUE;
	}

	PDO_HANDLE_DBH_ERR();
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::commit()
   Commit a transaction */
static PHP_METHOD(PDO, commit)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	PDO_CONSTRUCT_CHECK;

	if (!dbh->in_txn) {
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "There is no active transaction");
		RETURN_FALSE;
	}

	if (dbh->methods->commit(dbh TSRMLS_CC)) {
		dbh->in_txn = 0;
		RETURN_TRUE;
	}
	
	PDO_HANDLE_DBH_ERR();
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::rollBack()
   roll back a transaction */
static PHP_METHOD(PDO, rollBack)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	PDO_CONSTRUCT_CHECK;

	if (!dbh->in_txn) {
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "There is no active transaction");
		RETURN_FALSE;
	}

	if (dbh->methods->rollback(dbh TSRMLS_CC)) {
		dbh->in_txn = 0;
		RETURN_TRUE;
	}
		
	PDO_HANDLE_DBH_ERR();
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool PDO::inTransaction()
   determine if inside a transaction */
static PHP_METHOD(PDO, inTransaction)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	PDO_CONSTRUCT_CHECK;

	if (!dbh->methods->in_transaction) {
		RETURN_BOOL(dbh->in_txn);
	}	

	RETURN_BOOL(dbh->methods->in_transaction(dbh TSRMLS_CC));
}
/* }}} */

static int pdo_dbh_attribute_set(pdo_dbh_t *dbh, long attr, zval *value TSRMLS_DC) /* {{{ */
{

#define PDO_LONG_PARAM_CHECK \
	if (Z_TYPE_P(value) != IS_LONG && Z_TYPE_P(value) != IS_STRING && Z_TYPE_P(value) != IS_FALSE && Z_TYPE_P(value) != IS_TRUE) { \
		pdo_raise_impl_error(dbh, NULL, "HY000", "attribute value must be an integer" TSRMLS_CC); \
		PDO_HANDLE_DBH_ERR(); \
		return FAILURE; \
	} \

	switch (attr) {
		case PDO_ATTR_ERRMODE:
			PDO_LONG_PARAM_CHECK;
			convert_to_long(value);
			switch (Z_LVAL_P(value)) {
				case PDO_ERRMODE_SILENT:
				case PDO_ERRMODE_WARNING:
				case PDO_ERRMODE_EXCEPTION:
					dbh->error_mode = Z_LVAL_P(value);
					return SUCCESS;
				default:
					pdo_raise_impl_error(dbh, NULL, "HY000", "invalid error mode" TSRMLS_CC);
					PDO_HANDLE_DBH_ERR();
					return FAILURE;
			}
			return FAILURE;

		case PDO_ATTR_CASE:
			PDO_LONG_PARAM_CHECK;
			convert_to_long(value);
			switch (Z_LVAL_P(value)) {
				case PDO_CASE_NATURAL:
				case PDO_CASE_UPPER:
				case PDO_CASE_LOWER:
					dbh->desired_case = Z_LVAL_P(value);
					return SUCCESS;
				default:
					pdo_raise_impl_error(dbh, NULL, "HY000", "invalid case folding mode" TSRMLS_CC);
					PDO_HANDLE_DBH_ERR();
					return FAILURE;
			}
			return FAILURE;

		case PDO_ATTR_ORACLE_NULLS:
			PDO_LONG_PARAM_CHECK;
			convert_to_long(value);
			dbh->oracle_nulls = Z_LVAL_P(value);
			return SUCCESS;

		case PDO_ATTR_DEFAULT_FETCH_MODE:
			if (Z_TYPE_P(value) == IS_ARRAY) {
				zval *tmp;
				if ((tmp = zend_hash_index_find(Z_ARRVAL_P(value), 0)) != NULL && Z_TYPE_P(tmp) == IS_LONG) {
					if (Z_LVAL_P(tmp) == PDO_FETCH_INTO || Z_LVAL_P(tmp) == PDO_FETCH_CLASS) {
						pdo_raise_impl_error(dbh, NULL, "HY000", "FETCH_INTO and FETCH_CLASS are not yet supported as default fetch modes" TSRMLS_CC);
						return FAILURE;
					}
				}
			} else {
				PDO_LONG_PARAM_CHECK;
			}
			convert_to_long(value);
			if (Z_LVAL_P(value) == PDO_FETCH_USE_DEFAULT) {
				pdo_raise_impl_error(dbh, NULL, "HY000", "invalid fetch mode type" TSRMLS_CC);
				return FAILURE;
			}
			dbh->default_fetch_type = Z_LVAL_P(value);
			return SUCCESS;

		case PDO_ATTR_STRINGIFY_FETCHES:
			PDO_LONG_PARAM_CHECK;
			convert_to_long(value);
			dbh->stringify = Z_LVAL_P(value) ? 1 : 0;
			return SUCCESS;
			
		case PDO_ATTR_STATEMENT_CLASS: {
			/* array(string classname, array(mixed ctor_args)) */
			zend_class_entry *pce;
			zval *item;

			if (dbh->is_persistent) {
				pdo_raise_impl_error(dbh, NULL, "HY000", 
					"PDO::ATTR_STATEMENT_CLASS cannot be used with persistent PDO instances"
					TSRMLS_CC);
				PDO_HANDLE_DBH_ERR();
				return FAILURE;
			}
			if (Z_TYPE_P(value) != IS_ARRAY
				|| (item = zend_hash_index_find(Z_ARRVAL_P(value), 0)) == NULL
				|| Z_TYPE_P(item) != IS_STRING
				|| (pce = zend_lookup_class(Z_STR_P(item) TSRMLS_CC)) == NULL
			) {
				pdo_raise_impl_error(dbh, NULL, "HY000", 
					"PDO::ATTR_STATEMENT_CLASS requires format array(classname, array(ctor_args)); "
					"the classname must be a string specifying an existing class"
					TSRMLS_CC);
				PDO_HANDLE_DBH_ERR();
				return FAILURE;
			}
			if (!instanceof_function(pce, pdo_dbstmt_ce TSRMLS_CC)) {
				pdo_raise_impl_error(dbh, NULL, "HY000", 
					"user-supplied statement class must be derived from PDOStatement" TSRMLS_CC);
				PDO_HANDLE_DBH_ERR();
				return FAILURE;
			}
			if (pce->constructor && !(pce->constructor->common.fn_flags & (ZEND_ACC_PRIVATE|ZEND_ACC_PROTECTED))) {
				pdo_raise_impl_error(dbh, NULL, "HY000", 
					"user-supplied statement class cannot have a public constructor" TSRMLS_CC);
				PDO_HANDLE_DBH_ERR();
				return FAILURE;
			}
			dbh->def_stmt_ce = pce;
			if (!Z_ISUNDEF(dbh->def_stmt_ctor_args)) {
				zval_ptr_dtor(&dbh->def_stmt_ctor_args);
				ZVAL_UNDEF(&dbh->def_stmt_ctor_args);
			}
			if ((item = zend_hash_index_find(Z_ARRVAL_P(value), 1)) != NULL) {
				if (Z_TYPE_P(item) != IS_ARRAY) {
					pdo_raise_impl_error(dbh, NULL, "HY000", 
						"PDO::ATTR_STATEMENT_CLASS requires format array(classname, array(ctor_args)); "
						"ctor_args must be an array"
					TSRMLS_CC);
					PDO_HANDLE_DBH_ERR();
					return FAILURE;
				}
				ZVAL_COPY(&dbh->def_stmt_ctor_args, item);
			}
			return SUCCESS;
		}
			
		default:
			;
	}

	if (!dbh->methods->set_attribute) {
		goto fail;
	}

	PDO_DBH_CLEAR_ERR();
	if (dbh->methods->set_attribute(dbh, attr, value TSRMLS_CC)) {
		return SUCCESS;
	}

fail:
	if (attr == PDO_ATTR_AUTOCOMMIT) {
		zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "The auto-commit mode cannot be changed for this driver");
	} else if (!dbh->methods->set_attribute) {
		pdo_raise_impl_error(dbh, NULL, "IM001", "driver does not support setting attributes" TSRMLS_CC);
	} else {
		PDO_HANDLE_DBH_ERR();
	}
	return FAILURE;
}
/* }}} */
 
/* {{{ proto bool PDO::setAttribute(long attribute, mixed value)
   Set an attribute */
static PHP_METHOD(PDO, setAttribute)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());
	long attr;
	zval *value;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz", &attr, &value)) {
		RETURN_FALSE;
	}

	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;

	if (pdo_dbh_attribute_set(dbh, attr, value TSRMLS_CC) != FAILURE) {
 		RETURN_TRUE;
 	}
 	RETURN_FALSE;
}
/* }}} */

/* {{{ proto mixed PDO::getAttribute(long attribute)
   Get an attribute */
static PHP_METHOD(PDO, getAttribute)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());
	long attr;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &attr)) {
		RETURN_FALSE;
	}

	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;

	/* handle generic PDO-level attributes */
	switch (attr) {
		case PDO_ATTR_PERSISTENT:
			RETURN_BOOL(dbh->is_persistent);
			
		case PDO_ATTR_CASE:
			RETURN_LONG(dbh->desired_case);

		case PDO_ATTR_ORACLE_NULLS:
			RETURN_LONG(dbh->oracle_nulls);

		case PDO_ATTR_ERRMODE:
			RETURN_LONG(dbh->error_mode);

		case PDO_ATTR_DRIVER_NAME:
			RETURN_STRINGL((char*)dbh->driver->driver_name, dbh->driver->driver_name_len);

		case PDO_ATTR_STATEMENT_CLASS:
			array_init(return_value);
			add_next_index_str(return_value, STR_COPY(dbh->def_stmt_ce->name));
			if (!Z_ISUNDEF(dbh->def_stmt_ctor_args)) {
				if (Z_REFCOUNTED(dbh->def_stmt_ctor_args)) Z_ADDREF(dbh->def_stmt_ctor_args);
				add_next_index_zval(return_value, &dbh->def_stmt_ctor_args);
			}
			return;
		case PDO_ATTR_DEFAULT_FETCH_MODE:
			RETURN_LONG(dbh->default_fetch_type);
		default:
			break;
	}
	
	if (!dbh->methods->get_attribute) {
		pdo_raise_impl_error(dbh, NULL, "IM001", "driver does not support getting attributes" TSRMLS_CC);
		RETURN_FALSE;
	}

	switch (dbh->methods->get_attribute(dbh, attr, return_value TSRMLS_CC)) {
		case -1:
			PDO_HANDLE_DBH_ERR();
			RETURN_FALSE;

		case 0:
			pdo_raise_impl_error(dbh, NULL, "IM001", "driver does not support that attribute" TSRMLS_CC);
			RETURN_FALSE;

		default:
			return;
	}
}
/* }}} */

/* {{{ proto long PDO::exec(string query)
   Execute a query that does not return a row set, returning the number of affected rows */
static PHP_METHOD(PDO, exec)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());
	char *statement;
	int statement_len;
	long ret;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &statement, &statement_len)) {
		RETURN_FALSE;
	}

	if (!statement_len) {
		pdo_raise_impl_error(dbh, NULL, "HY000",  "trying to execute an empty query" TSRMLS_CC);
		RETURN_FALSE;
	}
	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;
	ret = dbh->methods->doer(dbh, statement, statement_len TSRMLS_CC);
	if(ret == -1) {
		PDO_HANDLE_DBH_ERR();
		RETURN_FALSE;
	} else {
		RETURN_LONG(ret);
	}
}
/* }}} */

/* {{{ proto string PDO::lastInsertId([string seqname])
   Returns the id of the last row that we affected on this connection.  Some databases require a sequence or table name to be passed in.  Not always meaningful. */
static PHP_METHOD(PDO, lastInsertId)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());
	char *name = NULL;
	int namelen;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!", &name, &namelen)) {
		RETURN_FALSE;
	}

	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;
	if (!dbh->methods->last_id) {
		pdo_raise_impl_error(dbh, NULL, "IM001", "driver does not support lastInsertId()" TSRMLS_CC);
		RETURN_FALSE;
	} else {
		int id_len;
		char *id;
		id = dbh->methods->last_id(dbh, name, (unsigned int *)&id_len TSRMLS_CC);
		if (!id) {
			PDO_HANDLE_DBH_ERR();
			RETURN_FALSE;
		} else {
			//??? use zend_string ?
			RETVAL_STRINGL(id, id_len);
			efree(id);
		}
	}
}
/* }}} */

/* {{{ proto string PDO::errorCode()
   Fetch the error code associated with the last operation on the database handle */
static PHP_METHOD(PDO, errorCode)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	PDO_CONSTRUCT_CHECK;

	if (dbh->query_stmt) {
		RETURN_STRING(dbh->query_stmt->error_code);
	}
	
	if (dbh->error_code[0] == '\0') {
		RETURN_NULL();
	}

	/**
	 * Making sure that we fallback to the default implementation
	 * if the dbh->error_code is not null.
	 */
	RETURN_STRING(dbh->error_code);
}
/* }}} */

/* {{{ proto int PDO::errorInfo()
   Fetch extended error information associated with the last operation on the database handle */
static PHP_METHOD(PDO, errorInfo)
{
	int error_count;
	int error_count_diff 	 = 0;
	int error_expected_count = 3;

	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	PDO_CONSTRUCT_CHECK;

	array_init(return_value);

	if (dbh->query_stmt) {
		add_next_index_string(return_value, dbh->query_stmt->error_code);
	} else {
		add_next_index_string(return_value, dbh->error_code);
	}

	if (dbh->methods->fetch_err) {
		dbh->methods->fetch_err(dbh, dbh->query_stmt, return_value TSRMLS_CC);
	}
	
	/**
	 * In order to be consistent, we have to make sure we add the good amount
	 * of nulls depending on the current number of elements. We make a simple
	 * difference and add the needed elements
	 */
	error_count = zend_hash_num_elements(Z_ARRVAL_P(return_value));

	if (error_expected_count > error_count) {
		int current_index;

		error_count_diff = error_expected_count - error_count;
		for (current_index = 0; current_index < error_count_diff; current_index++) {
			add_next_index_null(return_value);
		}
	}
}
/* }}} */

/* {{{ proto object PDO::query(string sql [, PDOStatement::setFetchMode() args])
   Prepare and execute $sql; returns the statement object for iteration */
static PHP_METHOD(PDO, query)
{
	pdo_stmt_t *stmt;
	char *statement;
	int statement_len;
	pdo_dbh_object_t *dbh_obj = Z_PDO_OBJECT_P(getThis());
	pdo_dbh_t *dbh = dbh_obj->inner;

	/* Return a meaningful error when no parameters were passed */
	if (!ZEND_NUM_ARGS()) {
		zend_parse_parameters(0 TSRMLS_CC, "z|z", NULL, NULL);
		RETURN_FALSE;
	}
	
	if (FAILURE == zend_parse_parameters(1 TSRMLS_CC, "s", &statement,
			&statement_len)) {
		RETURN_FALSE;
	}
	
	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;

	if (!pdo_stmt_instantiate(dbh, return_value, dbh->def_stmt_ce, &dbh->def_stmt_ctor_args TSRMLS_CC)) {
		pdo_raise_impl_error(dbh, NULL, "HY000", "failed to instantiate user supplied statement class" TSRMLS_CC);
		return;
	}
	stmt = Z_PDO_STMT_P(return_value);
	
	/* unconditionally keep this for later reference */
	stmt->query_string = estrndup(statement, statement_len);
	stmt->query_stringlen = statement_len;

	stmt->default_fetch_type = dbh->default_fetch_type;
	stmt->active_query_string = stmt->query_string;
	stmt->active_query_stringlen = statement_len;
	stmt->dbh = dbh;
	/* give it a reference to me */
	ZVAL_OBJ(&stmt->database_object_handle, &dbh_obj->std);
	Z_ADDREF(stmt->database_object_handle);
	/* we haven't created a lazy object yet */
	ZVAL_UNDEF(&stmt->lazy_object_ref);

	if (dbh->methods->preparer(dbh, statement, statement_len, stmt, NULL TSRMLS_CC)) {
		PDO_STMT_CLEAR_ERR();
		if (ZEND_NUM_ARGS() == 1 || SUCCESS == pdo_stmt_setup_fetch_mode(INTERNAL_FUNCTION_PARAM_PASSTHRU, stmt, 1)) {

			/* now execute the statement */
			PDO_STMT_CLEAR_ERR();
			if (stmt->methods->executer(stmt TSRMLS_CC)) {
				int ret = 1;
				if (!stmt->executed) {
					if (stmt->dbh->alloc_own_columns) {
						ret = pdo_stmt_describe_columns(stmt TSRMLS_CC);
					}
					stmt->executed = 1;
				}
				if (ret) {
					pdo_stmt_construct(stmt, return_value, dbh->def_stmt_ce, &dbh->def_stmt_ctor_args TSRMLS_CC);
					return;
				}
			}
		}
		/* something broke */
		dbh->query_stmt = stmt;
		ZVAL_COPY_VALUE(&dbh->query_stmt_zval, return_value);
		PDO_HANDLE_STMT_ERR();
	} else {
		PDO_HANDLE_DBH_ERR();
		zval_ptr_dtor(return_value);
	}

	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string PDO::quote(string string [, int paramtype])
   quotes string for use in a query.  The optional paramtype acts as a hint for drivers that have alternate quoting styles.  The default value is PDO_PARAM_STR */
static PHP_METHOD(PDO, quote)
{
	pdo_dbh_t *dbh = Z_PDO_DBH_P(getThis());
	char *str;
	int str_len;
	long paramtype = PDO_PARAM_STR;
	char *qstr;
	int qlen;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &str, &str_len, &paramtype)) {
		RETURN_FALSE;
	}
	
	PDO_DBH_CLEAR_ERR();
	PDO_CONSTRUCT_CHECK;
	if (!dbh->methods->quoter) {
		pdo_raise_impl_error(dbh, NULL, "IM001", "driver does not support quoting" TSRMLS_CC);
		RETURN_FALSE;
	}

	if (dbh->methods->quoter(dbh, str, str_len, &qstr, &qlen, paramtype TSRMLS_CC)) {
		RETVAL_STRINGL(qstr, qlen);
		efree(qstr);
		return;
	}
	PDO_HANDLE_DBH_ERR();
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto int PDO::__wakeup()
   Prevents use of a PDO instance that has been unserialized */
static PHP_METHOD(PDO, __wakeup)
{
	zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "You cannot serialize or unserialize PDO instances");
}
/* }}} */

/* {{{ proto int PDO::__sleep()
   Prevents serialization of a PDO instance */
static PHP_METHOD(PDO, __sleep)
{
	zend_throw_exception_ex(php_pdo_get_exception(), 0 TSRMLS_CC, "You cannot serialize or unserialize PDO instances");
}
/* }}} */

/* {{{ proto array PDO::getAvailableDrivers()
   Return array of available PDO drivers */
static PHP_METHOD(PDO, getAvailableDrivers)
{
	pdo_driver_t *pdriver;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	array_init(return_value);

	ZEND_HASH_FOREACH_PTR(&pdo_driver_hash, pdriver) {
		add_next_index_stringl(return_value, (char*)pdriver->driver_name, pdriver->driver_name_len);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_pdo___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, dsn)
	ZEND_ARG_INFO(0, username)
	ZEND_ARG_INFO(0, passwd)
	ZEND_ARG_INFO(0, options) /* array */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pdo_prepare, 0, 0, 1)
	ZEND_ARG_INFO(0, statement)
	ZEND_ARG_INFO(0, options) /* array */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_pdo_setattribute, 0)
	ZEND_ARG_INFO(0, attribute)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_pdo_getattribute, 0)
	ZEND_ARG_INFO(0, attribute)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_pdo_exec, 0)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pdo_lastinsertid, 0, 0, 0)
	ZEND_ARG_INFO(0, seqname)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pdo_quote, 0, 0, 1)
	ZEND_ARG_INFO(0, string)
	ZEND_ARG_INFO(0, paramtype)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_pdo__void, 0)
ZEND_END_ARG_INFO()
/* }}} */

const zend_function_entry pdo_dbh_functions[] = /* {{{ */ {
	ZEND_MALIAS(PDO, __construct, dbh_constructor,	arginfo_pdo___construct,	ZEND_ACC_PUBLIC)
	PHP_ME(PDO, prepare, 				arginfo_pdo_prepare,		ZEND_ACC_PUBLIC)
	PHP_ME(PDO, beginTransaction,       arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, commit,                 arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, rollBack,               arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, inTransaction,          arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, setAttribute,	arginfo_pdo_setattribute,	ZEND_ACC_PUBLIC)
	PHP_ME(PDO, exec,			arginfo_pdo_exec,		ZEND_ACC_PUBLIC)
	PHP_ME(PDO, query,			NULL,					ZEND_ACC_PUBLIC)
	PHP_ME(PDO, lastInsertId,	arginfo_pdo_lastinsertid,	ZEND_ACC_PUBLIC)
	PHP_ME(PDO, errorCode,              arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, errorInfo,              arginfo_pdo__void,         ZEND_ACC_PUBLIC)
	PHP_ME(PDO, getAttribute,	arginfo_pdo_getattribute,	ZEND_ACC_PUBLIC)
	PHP_ME(PDO, quote,			arginfo_pdo_quote,		ZEND_ACC_PUBLIC)
	PHP_ME(PDO, __wakeup,               arginfo_pdo__void,         ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(PDO, __sleep,                arginfo_pdo__void,         ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(PDO, getAvailableDrivers,    arginfo_pdo__void,         ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

static void cls_method_dtor(zval *el) /* {{{ */ {
	zend_function *func = (zend_function*)Z_PTR_P(el);
	if (func->common.function_name) {
		STR_RELEASE(func->common.function_name);
	}
	efree(func);
}
/* }}} */

/* {{{ overloaded object handlers for PDO class */
int pdo_hash_methods(pdo_dbh_object_t *dbh_obj, int kind TSRMLS_DC)
{
	const zend_function_entry *funcs;
	zend_function func;
	zend_internal_function *ifunc = (zend_internal_function*)&func;
	int namelen;
	char *lc_name;
	pdo_dbh_t *dbh = dbh_obj->inner;

	if (!dbh || !dbh->methods || !dbh->methods->get_driver_methods) {
		return 0;
	}
	funcs =	dbh->methods->get_driver_methods(dbh, kind TSRMLS_CC);
	if (!funcs) {
		return 0;
	}

	if (!(dbh->cls_methods[kind] = pemalloc(sizeof(HashTable), dbh->is_persistent))) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "out of memory while allocating PDO methods.");
	}
	zend_hash_init_ex(dbh->cls_methods[kind], 8, NULL, cls_method_dtor, dbh->is_persistent, 0);

	while (funcs->fname) {
		ifunc->type = ZEND_INTERNAL_FUNCTION;
		ifunc->handler = funcs->handler;
		ifunc->function_name = STR_INIT(funcs->fname, strlen(funcs->fname), 0);
		ifunc->scope = dbh_obj->std.ce;
		ifunc->prototype = NULL;
		if (funcs->flags) {
			ifunc->fn_flags = funcs->flags | ZEND_ACC_NEVER_CACHE;
		} else {
			ifunc->fn_flags = ZEND_ACC_PUBLIC | ZEND_ACC_NEVER_CACHE;
		}
		if (funcs->arg_info) {
			zend_internal_function_info *info = (zend_internal_function_info*)funcs->arg_info;

			ifunc->arg_info = (zend_arg_info*)funcs->arg_info + 1;
			ifunc->num_args = funcs->num_args;
			if (info->required_num_args == -1) {
				ifunc->required_num_args = funcs->num_args;
			} else {
				ifunc->required_num_args = info->required_num_args;
			}
			if (info->return_reference) {
				ifunc->fn_flags |= ZEND_ACC_RETURN_REFERENCE;
			}
			if (funcs->arg_info[funcs->num_args].is_variadic) {
				ifunc->fn_flags |= ZEND_ACC_VARIADIC;
			}
		} else {
			ifunc->arg_info = NULL;
			ifunc->num_args = 0;
			ifunc->required_num_args = 0;
		}
		namelen = strlen(funcs->fname);
		lc_name = emalloc(namelen+1);
		zend_str_tolower_copy(lc_name, funcs->fname, namelen);
		zend_hash_str_add_mem(dbh->cls_methods[kind], lc_name, namelen, &func, sizeof(func));
		efree(lc_name);
		funcs++;
	}

	return 1;
}

static union _zend_function *dbh_method_get(zend_object **object, zend_string *method_name, const zval *key TSRMLS_DC)
{
	zend_function *fbc = NULL;
	pdo_dbh_object_t *dbh_obj = php_pdo_dbh_fetch_object(*object);
	zend_string *lc_method_name;

	lc_method_name = STR_INIT(method_name->val, method_name->len, 0);
	zend_str_tolower_copy(lc_method_name->val, method_name->val, method_name->len);

	if ((fbc = std_object_handlers.get_method(object, method_name, key TSRMLS_CC)) == NULL) {
		/* not a pre-defined method, nor a user-defined method; check
		 * the driver specific methods */
		if (!dbh_obj->inner->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_DBH]) {
			if (!pdo_hash_methods(dbh_obj,
				PDO_DBH_DRIVER_METHOD_KIND_DBH TSRMLS_CC)
				|| !dbh_obj->inner->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_DBH]) {
				goto out;
			}
		}

		fbc = zend_hash_find_ptr(dbh_obj->inner->cls_methods[PDO_DBH_DRIVER_METHOD_KIND_DBH], lc_method_name);
	}

out:
	STR_RELEASE(lc_method_name);
	return fbc;
}

static int dbh_compare(zval *object1, zval *object2 TSRMLS_DC)
{
	return -1;
}

static zend_object_handlers pdo_dbh_object_handlers;
static void pdo_dbh_free_storage(zend_object *std TSRMLS_DC);

void pdo_dbh_init(TSRMLS_D)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "PDO", pdo_dbh_functions);
	pdo_dbh_ce = zend_register_internal_class(&ce TSRMLS_CC);
	pdo_dbh_ce->create_object = pdo_dbh_new;

	memcpy(&pdo_dbh_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	pdo_dbh_object_handlers.offset = XtOffsetOf(pdo_dbh_object_t, std);
	pdo_dbh_object_handlers.dtor_obj = zend_objects_destroy_object;
	pdo_dbh_object_handlers.free_obj = pdo_dbh_free_storage;
	pdo_dbh_object_handlers.get_method = dbh_method_get;
	pdo_dbh_object_handlers.compare_objects = dbh_compare;
	
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_BOOL", (long)PDO_PARAM_BOOL);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_NULL", (long)PDO_PARAM_NULL);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_INT",  (long)PDO_PARAM_INT);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_STR",  (long)PDO_PARAM_STR);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_LOB",  (long)PDO_PARAM_LOB);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_STMT", (long)PDO_PARAM_STMT);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_INPUT_OUTPUT", (long)PDO_PARAM_INPUT_OUTPUT);

	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_ALLOC",		(long)PDO_PARAM_EVT_ALLOC);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_FREE",			(long)PDO_PARAM_EVT_FREE);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_EXEC_PRE",		(long)PDO_PARAM_EVT_EXEC_PRE);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_EXEC_POST",	(long)PDO_PARAM_EVT_EXEC_POST);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_FETCH_PRE",	(long)PDO_PARAM_EVT_FETCH_PRE);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_FETCH_POST",	(long)PDO_PARAM_EVT_FETCH_POST);
	REGISTER_PDO_CLASS_CONST_LONG("PARAM_EVT_NORMALIZE",	(long)PDO_PARAM_EVT_NORMALIZE);

	REGISTER_PDO_CLASS_CONST_LONG("FETCH_LAZY", (long)PDO_FETCH_LAZY);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ASSOC",(long)PDO_FETCH_ASSOC);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_NUM",  (long)PDO_FETCH_NUM);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_BOTH", (long)PDO_FETCH_BOTH);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_OBJ",  (long)PDO_FETCH_OBJ);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_BOUND",(long)PDO_FETCH_BOUND);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_COLUMN",(long)PDO_FETCH_COLUMN);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_CLASS",(long)PDO_FETCH_CLASS);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_INTO", (long)PDO_FETCH_INTO);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_FUNC", (long)PDO_FETCH_FUNC);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_GROUP",(long)PDO_FETCH_GROUP);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_UNIQUE",(long)PDO_FETCH_UNIQUE);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_KEY_PAIR",(long)PDO_FETCH_KEY_PAIR);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_CLASSTYPE",(long)PDO_FETCH_CLASSTYPE);
#if PHP_MAJOR_VERSION > 5 || PHP_MINOR_VERSION >= 1
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_SERIALIZE",(long)PDO_FETCH_SERIALIZE);
#endif
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_PROPS_LATE",(long)PDO_FETCH_PROPS_LATE);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_NAMED",(long)PDO_FETCH_NAMED);

	REGISTER_PDO_CLASS_CONST_LONG("ATTR_AUTOCOMMIT",	(long)PDO_ATTR_AUTOCOMMIT);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_PREFETCH",		(long)PDO_ATTR_PREFETCH);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_TIMEOUT", 		(long)PDO_ATTR_TIMEOUT);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_ERRMODE", 		(long)PDO_ATTR_ERRMODE);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_SERVER_VERSION",	(long)PDO_ATTR_SERVER_VERSION);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_CLIENT_VERSION", 	(long)PDO_ATTR_CLIENT_VERSION);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_SERVER_INFO",		(long)PDO_ATTR_SERVER_INFO);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_CONNECTION_STATUS", 	(long)PDO_ATTR_CONNECTION_STATUS);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_CASE",		 	(long)PDO_ATTR_CASE);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_CURSOR_NAME", 	(long)PDO_ATTR_CURSOR_NAME);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_CURSOR",	 	(long)PDO_ATTR_CURSOR);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_ORACLE_NULLS",	(long)PDO_ATTR_ORACLE_NULLS);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_PERSISTENT",	(long)PDO_ATTR_PERSISTENT);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_STATEMENT_CLASS",		(long)PDO_ATTR_STATEMENT_CLASS);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_FETCH_TABLE_NAMES",		(long)PDO_ATTR_FETCH_TABLE_NAMES);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_FETCH_CATALOG_NAMES",		(long)PDO_ATTR_FETCH_CATALOG_NAMES);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_DRIVER_NAME",		(long)PDO_ATTR_DRIVER_NAME);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_STRINGIFY_FETCHES",(long)PDO_ATTR_STRINGIFY_FETCHES);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_MAX_COLUMN_LEN",(long)PDO_ATTR_MAX_COLUMN_LEN);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_EMULATE_PREPARES",(long)PDO_ATTR_EMULATE_PREPARES);
	REGISTER_PDO_CLASS_CONST_LONG("ATTR_DEFAULT_FETCH_MODE",(long)PDO_ATTR_DEFAULT_FETCH_MODE);
	
	REGISTER_PDO_CLASS_CONST_LONG("ERRMODE_SILENT",	(long)PDO_ERRMODE_SILENT);
	REGISTER_PDO_CLASS_CONST_LONG("ERRMODE_WARNING",	(long)PDO_ERRMODE_WARNING);
	REGISTER_PDO_CLASS_CONST_LONG("ERRMODE_EXCEPTION",	(long)PDO_ERRMODE_EXCEPTION);

	REGISTER_PDO_CLASS_CONST_LONG("CASE_NATURAL",	(long)PDO_CASE_NATURAL);
	REGISTER_PDO_CLASS_CONST_LONG("CASE_LOWER",	(long)PDO_CASE_LOWER);
	REGISTER_PDO_CLASS_CONST_LONG("CASE_UPPER",	(long)PDO_CASE_UPPER);

	REGISTER_PDO_CLASS_CONST_LONG("NULL_NATURAL",	(long)PDO_NULL_NATURAL);
	REGISTER_PDO_CLASS_CONST_LONG("NULL_EMPTY_STRING",	(long)PDO_NULL_EMPTY_STRING);
	REGISTER_PDO_CLASS_CONST_LONG("NULL_TO_STRING",	(long)PDO_NULL_TO_STRING);
			
	REGISTER_PDO_CLASS_CONST_STRING("ERR_NONE",	PDO_ERR_NONE);

	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_NEXT", (long)PDO_FETCH_ORI_NEXT);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_PRIOR", (long)PDO_FETCH_ORI_PRIOR);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_FIRST", (long)PDO_FETCH_ORI_FIRST);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_LAST", (long)PDO_FETCH_ORI_LAST);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_ABS", (long)PDO_FETCH_ORI_ABS);
	REGISTER_PDO_CLASS_CONST_LONG("FETCH_ORI_REL", (long)PDO_FETCH_ORI_REL);
	
	REGISTER_PDO_CLASS_CONST_LONG("CURSOR_FWDONLY", (long)PDO_CURSOR_FWDONLY);
	REGISTER_PDO_CLASS_CONST_LONG("CURSOR_SCROLL", (long)PDO_CURSOR_SCROLL);

#if 0
	REGISTER_PDO_CLASS_CONST_LONG("ERR_CANT_MAP", 		(long)PDO_ERR_CANT_MAP);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_SYNTAX", 		(long)PDO_ERR_SYNTAX);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_CONSTRAINT", 	(long)PDO_ERR_CONSTRAINT);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_NOT_FOUND", 		(long)PDO_ERR_NOT_FOUND);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_ALREADY_EXISTS", 	(long)PDO_ERR_ALREADY_EXISTS);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_NOT_IMPLEMENTED", 	(long)PDO_ERR_NOT_IMPLEMENTED);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_MISMATCH", 		(long)PDO_ERR_MISMATCH);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_TRUNCATED", 		(long)PDO_ERR_TRUNCATED);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_DISCONNECTED", 	(long)PDO_ERR_DISCONNECTED);
	REGISTER_PDO_CLASS_CONST_LONG("ERR_NO_PERM",		(long)PDO_ERR_NO_PERM);
#endif

}

static void dbh_free(pdo_dbh_t *dbh, zend_bool free_persistent TSRMLS_DC)
{
	int i;

	if (dbh->is_persistent && !free_persistent) {
		return;
	}

	if (dbh->query_stmt) {
		zval_ptr_dtor(&dbh->query_stmt_zval);
		dbh->query_stmt = NULL;
	}

	if (dbh->methods) {
		dbh->methods->closer(dbh TSRMLS_CC);
	}

	if (dbh->data_source) {
		pefree((char *)dbh->data_source, dbh->is_persistent);
	}
	if (dbh->username) {
		pefree(dbh->username, dbh->is_persistent);
	}
	if (dbh->password) {
		pefree(dbh->password, dbh->is_persistent);
	}
	
	if (dbh->persistent_id) {
		pefree((char *)dbh->persistent_id, dbh->is_persistent);
	}

	if (!Z_ISUNDEF(dbh->def_stmt_ctor_args)) {
		zval_ptr_dtor(&dbh->def_stmt_ctor_args);
	}
	
	for (i = 0; i < PDO_DBH_DRIVER_METHOD_KIND__MAX; i++) {
		if (dbh->cls_methods[i]) {
			zend_hash_destroy(dbh->cls_methods[i]);
			pefree(dbh->cls_methods[i], dbh->is_persistent);
		}
	}

	pefree(dbh, dbh->is_persistent);
}

static void pdo_dbh_free_storage(zend_object *std TSRMLS_DC)
{
	pdo_dbh_t *dbh = php_pdo_dbh_fetch_inner(std);
	if (dbh->in_txn && dbh->methods && dbh->methods->rollback) {
		dbh->methods->rollback(dbh TSRMLS_CC);
		dbh->in_txn = 0;
	}
	
	if (dbh->is_persistent && dbh->methods && dbh->methods->persistent_shutdown) {
		dbh->methods->persistent_shutdown(dbh TSRMLS_CC);
	}
	zend_object_std_dtor(std TSRMLS_CC);
	dbh_free(dbh, 0 TSRMLS_CC);
}

zend_object *pdo_dbh_new(zend_class_entry *ce TSRMLS_DC)
{
	pdo_dbh_object_t *dbh;

	dbh = ecalloc(1, sizeof(pdo_dbh_object_t) + sizeof(zval) * (ce->default_properties_count - 1));
	zend_object_std_init(&dbh->std, ce TSRMLS_CC);
	object_properties_init(&dbh->std, ce);
	rebuild_object_properties(&dbh->std);
	dbh->inner = ecalloc(1, sizeof(pdo_dbh_t));
	dbh->inner->def_stmt_ce = pdo_dbstmt_ce;
	
	dbh->std.handlers = &pdo_dbh_object_handlers;
	
	return &dbh->std;
}

/* }}} */

ZEND_RSRC_DTOR_FUNC(php_pdo_pdbh_dtor) /* {{{ */
{
	if (res->ptr) {
		pdo_dbh_t *dbh = (pdo_dbh_t*)res->ptr;
		dbh_free(dbh, 1 TSRMLS_CC);
		res->ptr = NULL;
	}
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
