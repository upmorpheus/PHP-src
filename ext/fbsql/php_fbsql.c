/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http:/*www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Frank M. Kromann frank@frontbase.com>                       |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

/* TODO:
 *
 * ? Safe mode implementation
 */

/*	SB's list:
	- BLOBs
	- API for a more natural FB connect semantic
	- Connect & set session 
	- Autoreconnect when disconnected
	- Comments and cleanup
	- Documentation

	- Format database error messages as HTML.

	BUGS
	- Select db with no arguments
	 - Query with everything defaulted
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_globals.h"
#include "php_globals.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#if WIN32|WINNT
#include <winsock.h>
#else
#include <php_config.h>
#include <build-defs.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#endif

#include "php_ini.h"

#define HAVE_FBSQL 1

#if HAVE_FBSQL
#include "php_fbsql.h"
#include <signal.h>

static int le_result, le_link, le_plink;

struct PHPFBResult;
typedef struct PHPFBResult PHPFBResult;

struct PHPFBLink;
typedef struct PHPFBLink PHPFBLink;

/*	The PHPFBLink structure represents a fbsql link. The lion is used for
	a connection to a machine, it may be persistant and is reference counted.
	The reason for refcounting is mostly to avoid to think, it work independent of 
	any wierd and unforseen allocation deallocation order.

	The PHPFBDatabse structure implements to actual connection to a FrontBase server
	ot may be persistant is the link it is connected to is persistant, and refcounted
	for the same reasons as above.

	The PHPFBResult structure implements a result from the FrontBase server, and does all
	required buffereing from of results.

	In the PHP code the 3 above a data structures are referenced by means of integers in the
	range from 1 to som configurable maximum.  You can put a limit to the number of links, databases
	and results.  The integer identifications is implemented by insertion in the list, which is passed
	as an argument to all the functions, please note the list is polymorph.

	Database objects and link objects are all reused, base on the host name user name, host name database name 
	user name.  So connecting twice to the same database as the same user will return the same database id.
	We use the same coding for that as fbsql does, explioiting the underlying implementation of the lists.

	Persistant objects are put in the persistent list as well, but only by name, if you connect to a persistant object
	and it is not in the list it is simply added and get a new index, and refcounted.  Tricky, tricky ...
*/

/* Some functions which should be exported from FBCAccess */

void*        fbaObjectAtIndex();
void         fbcInitialize();
void         fbaRelease();
unsigned int fbaCount();



struct PHPFBResult
{
	PHPFBLink*				link;				/* The link for the result, may be NULL if no link  */
//	PHPFBDatabase*			database;			/* The database for the result, may be NULL of no database is related to the result */
//	FBCDatabaseConnection*	connection;			/* The database connection, just a convinience */
	char*					fetchHandle;		/* The fetch handle, the id used by the server.   */
	FBCMetaData*			metaData;			/* The metadata describing the result */
	FBCMetaData*			ResultmetaData;		/* The metadata describing the result */
	FBCRowHandler*			rowHandler;			/* The row handler, the Frontbase structure used for accessing rows in the result */
	unsigned int			batchSize;			/* The number of row to fetch when expanding the number of rows in the row handler */
	int						rowCount;			/* The number of rows in the results set.  The number of row is not in */
						/* general known when the select is done, one typically needs to fetch all the row
						   to figure out how many row you got. When the rowCount is unknown the value is
						   0x7ffffffff */
	int						columnCount;		/* Number of columns in the row set. */
	int						rowIndex;			/* The current row index. */
	int						columnIndex;		/* The current column index */
	void**					row;				/* The last row accessed */
	FBArray*				array;				/* The link may return a result set, the database list, we implement that by the  */
						/* FBArray, just a list of strings. */
	FBCPList*				list;				/* The same special kind result just for property list from extract, schema info. */
	unsigned int			selectResults;		/* number of results in select */
	unsigned int			currentResult;		/* current result number */
};

struct PHPFBLink
{
	int						persistant;			/* persistant ? */
	char*					hostName;			/* Host name  */
	char*					userName;			/* User name */
	char*					userPassword;		/* User password */
	char*					databasePassword;	/* Database password */
	char*					databaseName;		/* The name of the database */
	FBCExecHandler*			execHandler;		/* The exechandler, can be used for database operations */
	FBCDatabaseConnection*	connection;			/* The connection to the database */
	unsigned int			affectedRows;		/* Number of rows affected by the last SQL statement */
	long					autoCommit;			/* Enable or disable autoCommit */
	unsigned int			errorNo;			/* The latest error on the connection, 0 is ok. */
	char*					errorText;			/* The error text */
	unsigned int			insert_id;			/* The row index of the latest row inserted into the database */
};

#define FBSQL_ASSOC		1<<0
#define FBSQL_NUM		1<<1
#define FBSQL_BOTH		(FBSQL_ASSOC|FBSQL_NUM)


function_entry fbsql_functions[] = {
	PHP_FE(fbsql_connect,		NULL)
	PHP_FE(fbsql_pconnect,		NULL)
	PHP_FE(fbsql_close,			NULL)
	PHP_FE(fbsql_select_db,		NULL)
	PHP_FE(fbsql_create_db,		NULL)
	PHP_FE(fbsql_drop_db,		NULL)
	PHP_FE(fbsql_start_db,		NULL)
	PHP_FE(fbsql_stop_db,		NULL)
	PHP_FE(fbsql_db_status,		NULL)
	PHP_FE(fbsql_query,			NULL)
	PHP_FE(fbsql_db_query,		NULL)
	PHP_FE(fbsql_list_dbs,		NULL)
	PHP_FE(fbsql_list_tables,	NULL)
	PHP_FE(fbsql_list_fields,	NULL)
	PHP_FE(fbsql_error,			NULL)
	PHP_FE(fbsql_errno,			NULL)
	PHP_FE(fbsql_affected_rows,	NULL)
	PHP_FE(fbsql_insert_id,		NULL)
	PHP_FE(fbsql_result,		NULL)
	PHP_FE(fbsql_next_result,	NULL)
	PHP_FE(fbsql_num_rows,		NULL)
	PHP_FE(fbsql_num_fields,	NULL)
	PHP_FE(fbsql_fetch_row,		NULL)
	PHP_FE(fbsql_fetch_array,	NULL)
	PHP_FE(fbsql_fetch_assoc,	NULL)
	PHP_FE(fbsql_fetch_object,	NULL)
	PHP_FE(fbsql_data_seek,		NULL)
	PHP_FE(fbsql_fetch_lengths,	NULL)
	PHP_FE(fbsql_fetch_field,	NULL)
	PHP_FE(fbsql_field_seek,	NULL)
	PHP_FE(fbsql_free_result,	NULL)
	PHP_FE(fbsql_field_name,	NULL)
	PHP_FE(fbsql_field_table,	NULL)
	PHP_FE(fbsql_field_len,		NULL)
	PHP_FE(fbsql_field_type,	NULL)
	PHP_FE(fbsql_field_flags,	NULL) 

/*	Fontbase additions:  */
	PHP_FE(fbsql_autocommit,	NULL)
	PHP_FE(fbsql_commit,		NULL)
	PHP_FE(fbsql_rollback,		NULL)

	PHP_FE(fbsql_hostname,		NULL)
	PHP_FE(fbsql_database,		NULL)
	PHP_FE(fbsql_database_password,	NULL)
	PHP_FE(fbsql_username,		NULL)
	PHP_FE(fbsql_password,		NULL)
	PHP_FE(fbsql_warnings,		NULL)

/*	Aliases:  */
	PHP_FALIAS(fbsql, fbsql_db_query, NULL)

	{NULL, NULL, NULL}
};

zend_module_entry fbsql_module_entry = {
   "fbsql",
   fbsql_functions,
   PHP_MINIT(fbsql),
   PHP_MSHUTDOWN(fbsql),
   PHP_RINIT(fbsql),
   PHP_RSHUTDOWN(fbsql),
   PHP_MINFO(fbsql),
   STANDARD_MODULE_PROPERTIES
};

ZEND_DECLARE_MODULE_GLOBALS(fbsql)

#ifdef COMPILE_DL_FBSQL
ZEND_GET_MODULE(fbsql)
#endif

#define CHECK_LINK(link) { if (link==-1) { php_error(E_WARNING,"FrontBase:  A link to the server could not be established"); RETURN_FALSE; } }

static void phpfbReleaseResult (zend_rsrc_list_entry *rsrc);
static void phpfbReleaseLink (zend_rsrc_list_entry *rsrc);
static void phpfbReleasePLink (zend_rsrc_list_entry *rsrc);

static void phpfbReleaseResult(zend_rsrc_list_entry *rsrc)
{
	PHPFBResult* result = (PHPFBResult *)rsrc->ptr;
	FBSQLLS_FETCH();
	if (result)
	{
		if (result->fetchHandle) {
			FBCMetaData *md = fbcdcCancelFetch(result->link->connection,result->fetchHandle);
			fbcmdRelease(md);
		}
		if (result->rowHandler)  fbcrhRelease(result->rowHandler);
		if (result->ResultmetaData)    fbcmdRelease(result->ResultmetaData);
		if (result->list)        fbcplRelease(result->list);
		if (result->array)       fbaRelease(result->array);
		if (result->link)
		{
		}
		result->link        = 0;
		result->fetchHandle = NULL;
		result->metaData    = NULL;
		result->rowHandler  = NULL;
		result->batchSize   = 0;
		result->rowCount    = -1;
		result->columnIndex = 0;
		result->row         = NULL;
		result->array       = NULL;
		result->list        = NULL;
		efree(result);
	}
}


static void phpfbReleaseLink (zend_rsrc_list_entry *rsrc)
{
	PHPFBLink* link = (PHPFBLink *)rsrc->ptr;
	FBSQLLS_FETCH();
	if (link)
	{
		if (link->hostName) efree(link->hostName);
		if (link->userName) efree(link->userName);
		if (link->userPassword) efree(link->userPassword);
		if (link->databasePassword) efree(link->databasePassword);
		if (link->databaseName) efree(link->databaseName);
		if (link->errorText) efree(link->errorText);
		if (link->connection) {
			fbcdcClose(link->connection);
			fbcdcRelease(link->connection);
		}

//		zend_hash_apply(&EG(regular_list),(int (*)(void *))_clean_invalid_results);

		efree(link);
		FB_SQL_G(linkCount)--;
	}
}

static void phpfbReleasePLink (zend_rsrc_list_entry *rsrc)
{
	PHPFBLink* link = (PHPFBLink *)rsrc->ptr;
	FBSQLLS_FETCH();
	if (link)
	{
//		if (link->hostName) efree(link->hostName);
//		if (link->userName) efree(link->userName);
//		if (link->userPassword) efree(link->userPassword);
//		if (link->databasePassword) efree(link->databasePassword);
		if (link->errorText) {
			efree(link->errorText);
			link->errorText = NULL;
		}
		FB_SQL_G(linkCount)--;
		FB_SQL_G(persistantCount)--;
	}
}

static void php_fbsql_set_default_link(int id)
{
	FBSQLLS_FETCH();

	if (FB_SQL_G(linkIndex)!=-1) {
		zend_list_delete(FB_SQL_G(linkIndex));
	}
	FB_SQL_G(linkIndex) = id;
	zend_list_addref(id);
}

static int php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAMETERS FBSQLLS_DC)
{
	if (FB_SQL_G(linkIndex)==-1) { /* no link opened yet, implicitly open one */
		ht = 0;
		php_fbsql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
	}
	return FB_SQL_G(linkIndex);
}


static int phpfbQuery(INTERNAL_FUNCTION_PARAMETERS, char* sql, PHPFBLink* link);

PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN  ("fbsql.allow_persistant",				"1",		PHP_INI_SYSTEM, OnUpdateInt,    allowPersistent,  zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_BOOLEAN  ("fbsql.generate_warnings",			"0",		PHP_INI_SYSTEM, OnUpdateInt,    generateWarnings, zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_BOOLEAN  ("fbsql.autocommit",					"1",		PHP_INI_SYSTEM, OnUpdateInt,    autoCommit,	      zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_ENTRY_EX ("fbsql.max_persistent",				"-1",		PHP_INI_SYSTEM, OnUpdateInt,    maxPersistant,    zend_fbsql_globals, fbsql_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX ("fbsql.max_links",					"128",		PHP_INI_SYSTEM, OnUpdateInt,    maxLinks,         zend_fbsql_globals, fbsql_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX ("fbsql.max_connections",				"128",		PHP_INI_SYSTEM, OnUpdateInt,    maxConnections,   zend_fbsql_globals, fbsql_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX ("fbsql.max_results",					"128",		PHP_INI_SYSTEM, OnUpdateInt,    maxResults,       zend_fbsql_globals, fbsql_globals, display_link_numbers)
	STD_PHP_INI_ENTRY_EX ("fbsql.mbatchSize",					"1000",		PHP_INI_SYSTEM, OnUpdateInt,    batchSize,		  zend_fbsql_globals, fbsql_globals, display_link_numbers)
	STD_PHP_INI_ENTRY    ("fbsql.default_host",					NULL,		PHP_INI_SYSTEM, OnUpdateString, hostName,         zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_ENTRY    ("fbsql.default_user",					"_SYSTEM",	PHP_INI_SYSTEM, OnUpdateString, userName,         zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_ENTRY    ("fbsql.default_password",				"",         PHP_INI_SYSTEM, OnUpdateString, userPassword,     zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_ENTRY    ("fbsql.default_database",				"",         PHP_INI_SYSTEM, OnUpdateString, databaseName,     zend_fbsql_globals, fbsql_globals)
	STD_PHP_INI_ENTRY    ("fbsql.default_database_password",	"",         PHP_INI_SYSTEM, OnUpdateString, databasePassword, zend_fbsql_globals, fbsql_globals)
PHP_INI_END()
      

static void php_fbsql_init_globals(zend_fbsql_globals *fbsql_globals)
{
	fbsql_globals->persistantCount = 0;

	if (fbsql_globals->hostName==NULL)
	{
		char name[256];
		gethostname(name,sizeof(name));
		name[sizeof(name)-1] = 0;
		fbsql_globals->hostName = estrdup(name);
	}

	fbsql_globals->persistantCount	= 0;
	fbsql_globals->linkCount		= 0;
}
                                        
PHP_MINIT_FUNCTION(fbsql)
{
	ZEND_INIT_MODULE_GLOBALS(fbsql, php_fbsql_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	fbcInitialize();

	le_result   = zend_register_list_destructors_ex(phpfbReleaseResult, NULL, "fbsql result", module_number);
	le_link     = zend_register_list_destructors_ex(phpfbReleaseLink, NULL, "fbsql link", module_number);
	le_plink    = zend_register_list_destructors_ex(NULL, phpfbReleasePLink, "fbsql plink", module_number);

	REGISTER_LONG_CONSTANT("FBSQL_ASSOC", FBSQL_ASSOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FBSQL_NUM",   FBSQL_NUM,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FBSQL_BOTH",  FBSQL_BOTH,  CONST_CS | CONST_PERSISTENT);
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(fbsql)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(fbsql)
{
	FBSQLLS_FETCH();
	
	FB_SQL_G(linkIndex) = -1;
	FB_SQL_G(linkCount) = FB_SQL_G(persistantCount);
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(fbsql)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(fbsql)
{
	char buf[32];
	FBSQLLS_FETCH();
	php_info_print_table_start();
	php_info_print_table_header(2, "FrontBase support", "enabled");

	php_info_print_table_row(2, "Client API version", "2.20");

	if (FB_SQL_G(allowPersistent))
	{
		sprintf(buf, "%ld", FB_SQL_G(persistantCount));
		php_info_print_table_row(2, "Active Persistant Links", buf);
	}

	sprintf(buf, "%ld", FB_SQL_G(linkCount));
	php_info_print_table_row(2, "Active Links", buf);

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

static void php_fbsql_do_connect(INTERNAL_FUNCTION_PARAMETERS, int persistant)
{
	PHPFBLink* phpLink;
	list_entry *lep;
	char name[1024];
	char *hostName = NULL, *userName = NULL, *userPassword = NULL;
	int argc = ZEND_NUM_ARGS(), create_new = 0;
	zval **argv[3];
	FBSQLLS_FETCH();


	if ((argc < 0) || (argc > 3)) WRONG_PARAM_COUNT;
	if (zend_get_parameters_ex(argc,&argv[0],&argv[1],&argv[2])==FAILURE) RETURN_FALSE;
	if (argc >= 1)
	{
		convert_to_string_ex(argv[0]);
		hostName = (*argv[0])->value.str.val;
	}
	if (argc >= 2)
	{
		convert_to_string_ex(argv[1]);
		userName =  (*argv[1])->value.str.val;
	}   
	if (argc == 3)
	{
		convert_to_string_ex(argv[2]);
		userPassword =  (*argv[2])->value.str.val;
	}

	if (hostName     == NULL) hostName     = FB_SQL_G(hostName);
	if (userName     == NULL) userName     = FB_SQL_G(userName);
	if (userPassword == NULL) userPassword = FB_SQL_G(userPassword);

	sprintf(name,"fbsql_%s_%s_%s", hostName, userName, userPassword);

	if (persistant) {
		if (zend_hash_find(&EG(persistent_list), name, strlen(name) + 1, (void **)&lep) == SUCCESS)
		{
			phpLink = (PHPFBLink*)lep->ptr;
		}
		else {
			list_entry le;

			if ((FB_SQL_G(maxLinks) != -1 && FB_SQL_G(linkCount) == FB_SQL_G(maxLinks)))
			{
				php_error(E_WARNING,"FrontBase link limit %d exceeded ", FB_SQL_G(maxLinks));
				RETURN_FALSE;
			}

			phpLink = emalloc(sizeof(PHPFBLink));
			phpLink->persistant       = persistant;
			phpLink->hostName         = estrdup(hostName);
			phpLink->userName         = estrdup(userName);
			phpLink->userPassword     = estrdup(userPassword);
			phpLink->databasePassword = estrdup(FB_SQL_G(databasePassword));
			phpLink->databaseName	  = NULL;
			phpLink->execHandler      = fbcehHandlerForHost(hostName,128);
			phpLink->affectedRows     = 0;
			phpLink->autoCommit	 	  = FB_SQL_G(autoCommit);
			phpLink->errorNo          = 0;
			phpLink->errorText        = NULL;
			phpLink->connection		  = NULL;


			le.ptr  = phpLink;
			le.type = le_plink;
			if (zend_hash_update(&EG(persistent_list), name, strlen(name) + 1, &le, sizeof(le), NULL)==FAILURE)
			{
				efree(phpLink->hostName);
				efree(phpLink->userName);
				efree(phpLink->userPassword);
				efree(phpLink->databasePassword);
				efree(phpLink);
				RETURN_FALSE;
			}
			FB_SQL_G(linkCount)++;
			FB_SQL_G(persistantCount)++;
		}
		ZEND_REGISTER_RESOURCE(return_value, phpLink, le_plink);
	}
	else
	{
		list_entry le;
		if (zend_hash_find(&EG(regular_list), name, strlen(name) + 1, (void **)&lep) == SUCCESS)
		{
			int type, link;
			void *ptr;

			link = (int) lep->ptr;
			ptr = zend_list_find(link,&type);   /* check if the link is still there */
			if (ptr && (type==le_link || type==le_plink)) {
				zend_list_addref(link);
				return_value->value.lval = link;
				php_fbsql_set_default_link(link);
				return_value->type = IS_RESOURCE;
				return;
			} else {
				zend_hash_del(&EG(regular_list), name, strlen(name) + 1);
			}
			phpLink = (PHPFBLink*)lep->ptr;
		}

		phpLink = emalloc(sizeof(PHPFBLink));
		phpLink->persistant       = persistant;
		phpLink->hostName         = estrdup(hostName);
		phpLink->userName         = estrdup(userName);
		phpLink->userPassword     = estrdup(userPassword);
		phpLink->databasePassword = estrdup(FB_SQL_G(databasePassword));
		phpLink->databaseName	  = NULL;
		phpLink->execHandler      = fbcehHandlerForHost(hostName,128);
		phpLink->affectedRows     = 0;
		phpLink->autoCommit	 	  = FB_SQL_G(autoCommit);
		phpLink->errorNo          = 0;
		phpLink->errorText        = NULL;
		phpLink->connection		  = NULL;

		ZEND_REGISTER_RESOURCE(return_value, phpLink, le_link);

		le.ptr  = (void *)return_value->value.lval;
		le.type = le_index_ptr;
		if (zend_hash_update(persistant?&EG(persistent_list):&EG(regular_list), name, strlen(name) + 1, &le, sizeof(le), NULL)==FAILURE)
		{
			efree(phpLink->hostName);
			efree(phpLink->userName);
			efree(phpLink->userPassword);
			efree(phpLink->databasePassword);
			efree(phpLink);
			RETURN_FALSE;
		}
		FB_SQL_G(linkCount)++;
	}
	php_fbsql_set_default_link(return_value->value.lval);
}


int phpfbFetchRow(PHPFBResult* result, int row)
{
	if (result->rowHandler == NULL)
	{
		void *rawData = fbcdcFetch(result->link->connection, result->batchSize, result->fetchHandle);
		if (rawData == NULL)
			result->rowCount = 0;
		else
			result->rowHandler = fbcrhInitWith(rawData, result->metaData);
	}
	for (;;)
	{
		void *rawData; 
		if (row >=  result->rowCount) return 0;
		if (fbcrhRowCount(result->rowHandler) > (unsigned int)row) return 1;
		rawData = fbcdcFetch(result->link->connection, result->batchSize, result->fetchHandle);
		if (!fbcrhAddBatch(result->rowHandler,rawData)) result->rowCount = fbcrhRowCount(result->rowHandler);
	}
	return 0;
}


/* {{{ proto resource fbsql_connect([string hostname [, string username [, string password]]]);
	*/
PHP_FUNCTION(fbsql_connect)
{
	php_fbsql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,0);
}
/* }}} */


/* {{{ proto resource fbsql_pconnect([string hostname [, string username [, string password]]]);
	*/
PHP_FUNCTION(fbsql_pconnect)
{
	php_fbsql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU,1);
}
/* }}} */

/* {{{ proto int fbsql_close([int link_identifier])
	*/
PHP_FUNCTION(fbsql_close)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	if (id==-1) { /* explicit resource number */
		zend_list_delete(Z_RESVAL_PP(fbsql_link_index));
	}

	if (id!=-1 
		|| (fbsql_link_index && Z_RESVAL_PP(fbsql_link_index)==FB_SQL_G(linkIndex))) {
		zend_list_delete(FB_SQL_G(linkIndex));
		FB_SQL_G(linkIndex) = -1;
	}

	RETURN_TRUE;
}
/* }}} */

static int php_fbsql_select_db(char *databaseName, PHPFBLink *link)
{
	unsigned port;
	FBCDatabaseConnection* c;
	FBCMetaData*           md;
	FBSQLLS_FETCH();

	if (!link->databaseName || strcmp(link->databaseName, databaseName)) 
	{
		port = atoi(databaseName);
		if (port>0 && port<65535)
			c = fbcdcConnectToDatabaseUsingPort(link->hostName, port, link->databasePassword);
		else
			c = fbcdcConnectToDatabase(databaseName, link->hostName, link->databasePassword);
		if (c == NULL)
		{
			php_error(E_WARNING, fbcdcClassErrorMessage());
			return 0;
		}
		md = fbcdcCreateSession(c,"PHP",link->userName, link->userPassword, link->userName);
		if (fbcmdErrorsFound(md))
		{
			FBCErrorMetaData* emd = fbcdcErrorMetaData(c,md);
			char*             emg = fbcemdAllErrorMessages(emd);
			if (emg)
				php_error(E_WARNING, emg);
			else
				php_error(E_WARNING,"No message");
			efree(emg);
			fbcemdRelease(emd);
			fbcmdRelease(md);
			fbcdcClose(c);
			fbcdcRelease(c);
			return 0;
		}
		fbcmdRelease(md);

		if (c)
		{
			if (link->autoCommit)
				md = fbcdcExecuteDirectSQL(c,"SET COMMIT TRUE;");
			else
				md = fbcdcExecuteDirectSQL(c,"SET COMMIT FALSE;");
			fbcmdRelease(md);
		}
		fbcdcSetOutputCharacterSet(c,FBC_ISO8859_1);
		fbcdcSetInputCharacterSet(c,FBC_ISO8859_1);

		if (link->connection)
		{
			fbcdcClose(link->connection);
			fbcdcRelease(link->connection);
		}
		link->connection = c;
		if (link->databaseName) efree(link->databaseName);
		link->databaseName = estrdup(databaseName);
	}
	return 1;
}
/* }}} */

void phpfbestrdup(const char * s, int* length, char** value)
{
	int   l = s?strlen(s):0;
	if (value)
	{
		char* r = emalloc(l+1);
		if (s)
			strcpy(r,s);
		else
			r[0] = 0;
		*value  = r;
	}
	*length = l;
}

/* {{{ proto bool fbsql_autocommit(int link_identifier [, bool OnOff])
	*/
PHP_FUNCTION(fbsql_autocommit)
{
	PHPFBLink* phpLink = NULL;
	FBCMetaData* md;
	zval **fbsql_link_index = NULL, **onoff = NULL;
	zend_bool OnOff;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &onoff)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (onoff)
	{
		convert_to_boolean_ex(onoff);
		OnOff = Z_BVAL_PP(onoff);
		phpLink->autoCommit = OnOff;
		if (OnOff)
			md = fbcdcExecuteDirectSQL(phpLink->connection, "SET COMMIT TRUE;");
		else
			md = fbcdcExecuteDirectSQL(phpLink->connection, "SET COMMIT FALSE;");
		fbcmdRelease(md);
	}
	RETURN_BOOL(phpLink->autoCommit);
}
/* }}} */

/* {{{ proto int fbsql_commit([int link_identifier])
	*/
PHP_FUNCTION(fbsql_commit)
{
	PHPFBLink* phpLink = NULL;
	FBCMetaData* md;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	md = fbcdcCommit(phpLink->connection);

	if (md) {
		fbcmdRelease(md);
		RETURN_TRUE;
	}
	else
		RETURN_FALSE;
}
/* }}} */

/* {{{ proto int fbsql_rollback([int link_identifier])
	*/
PHP_FUNCTION(fbsql_rollback)
{
	PHPFBLink* phpLink = NULL;
	FBCMetaData* md;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	md = fbcdcRollback(phpLink->connection);

	if (md) {
		fbcmdRelease(md);
		RETURN_TRUE;
	}
	else
		RETURN_FALSE;
}
/* }}} */


/* {{{ proto string fbsql_hostname(int link_identifier [, string host_name])
	*/
PHP_FUNCTION(fbsql_hostname)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **host_name = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &host_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (host_name)
	{
		convert_to_string_ex(host_name);
		if (phpLink->hostName) efree(phpLink->hostName);
		phpLink->hostName = estrndup(Z_STRVAL_PP(host_name), Z_STRLEN_PP(host_name));
	}
	RETURN_STRING(phpLink->hostName, 1);
}
/* }}} */


/* {{{ proto string fbsql_database(int link_identifier [, string database])
	*/
PHP_FUNCTION(fbsql_database)
{
	PHPFBLink* phpLink = NULL;
	zval **fbsql_link_index = NULL, **dbname = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &dbname)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (dbname)
	{
		convert_to_string_ex(dbname);
		if (phpLink->databaseName) efree(phpLink->databaseName);
		phpLink->databaseName = estrndup(Z_STRVAL_PP(dbname), Z_STRLEN_PP(dbname));
	}
	RETURN_STRING(phpLink->databaseName, 1);
}
/* }}} */


/* {{{ proto string fbsql_database_password(int link_identifier [, string database_password])
	*/
PHP_FUNCTION(fbsql_database_password)
{
	PHPFBLink* phpLink = NULL;
	zval **fbsql_link_index = NULL, **db_password = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &db_password)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (db_password)
	{
		convert_to_string_ex(db_password);
		if (phpLink->databasePassword) efree(phpLink->databasePassword);
		phpLink->databasePassword = estrndup(Z_STRVAL_PP(db_password), Z_STRLEN_PP(db_password));
	}
	RETURN_STRING(phpLink->databasePassword, 1);
}
/* }}} */


/* {{{ proto string fbsql_username(int link_identifier [, string username])
	*/
PHP_FUNCTION(fbsql_username)
{
	PHPFBLink* phpLink = NULL;
	zval **fbsql_link_index = NULL, **username = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &username)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (username)
	{
		convert_to_string_ex(username);
		if (phpLink->userName) efree(phpLink->userName);
		phpLink->userName = estrndup(Z_STRVAL_PP(username), Z_STRLEN_PP(username));
	}
	RETURN_STRING(phpLink->userName, 1);
}
/* }}} */


/* {{{ proto string fbsql_password(int link_identifier [, string password])
	*/
PHP_FUNCTION(fbsql_password)
{   
	PHPFBLink* phpLink = NULL;
	zval **fbsql_link_index = NULL, **password = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_link_index, &password)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, -1, "FrontBase-Link", le_link, le_plink);

	if (password)
	{
		convert_to_string_ex(password);
		if (phpLink->userPassword) efree(phpLink->userPassword);
		phpLink->userPassword = estrndup(Z_STRVAL_PP(password), Z_STRLEN_PP(password));
	}
	RETURN_STRING(phpLink->userPassword, 1);
}
/* }}} */


/* {{{ proto bool fbsql_select_db([string database_name [, int link_identifier]])   
	*/
PHP_FUNCTION(fbsql_select_db)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **dbname;
	int id;
	char*          name = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			name = FB_SQL_G(databaseName);
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &dbname)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(dbname);
			name = (*dbname)->value.str.val;
			break;
		case 2:
			if (zend_get_parameters_ex(2, &dbname, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(dbname);
			name = (*dbname)->value.str.val;
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	if (phpLink->execHandler == NULL)
	{
		int port = atoi(name);
		if (port == 0 || port > 64535) {
			php_error(E_WARNING,"Cannot connect to FBExec for database '%s'",name);
			php_error(E_WARNING,fbcehClassErrorMessage());
			RETURN_FALSE;
		}
	}

	if (!php_fbsql_select_db(name, phpLink)) RETURN_FALSE;

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int fbsql_change_user(string user, string password [, string database [, int link_identifier]]);
	*/
PHP_FUNCTION(fbsql_change_user)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **user, **password, **database;
	int id;
	char *name = NULL, *userName, *userPassword;
	char buffer[1024];
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 2:
			name = FB_SQL_G(databaseName);
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(2, &user, &password)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(3, &user, &password, &database)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(database);
			name = (*database)->value.str.val;
			break;
		case 4:
			if (zend_get_parameters_ex(4, &user, &password, &database, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_string_ex(database);
			name = (*database)->value.str.val;
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(user);
	userName = (*user)->value.str.val;

	convert_to_string_ex(password);
	userPassword = (*password)->value.str.val;

	sprintf(buffer,"SET AUTHORIZATION %s;",userName);

	phpfbQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU, buffer, phpLink);
	if (return_value->value.lval)
	{
		efree(phpLink->userName);
		phpLink->userName = estrdup(userName);
	}
}
/* }}} */


/* {{{ proto bool fbsql_create_db(string database_name [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_create_db)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	int i, status;
	char *databaseName;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	status = fbcehStatusForDatabaseNamed(phpLink->execHandler, databaseName);
	if (status != FBUnknownStatus)
	{
		char* txt = "Unknown status";
		if      (status == FBStopped ) txt = "stopped";
		else if (status == FBStarting) txt = "starting";
		else if (status == FBRunning ) txt = "running";
		else if (status == FBStopping) txt = "stopping";
		else if (status == FBNoExec  ) txt = "no exec";
		php_error(E_WARNING, "Could not create %s@%s, database is %s",databaseName, phpLink->hostName,txt);
		RETURN_FALSE;
	}
	if (!fbcehCreateDatabaseNamedWithOptions(phpLink->execHandler, databaseName, ""))
	{
		char* error = fbechErrorMessage(phpLink->execHandler);
		php_error(E_WARNING, "Could not create %s@%s. %s.",databaseName,phpLink->hostName,error);
		RETURN_FALSE;
	}
	for (i=0; i < 20; i++)
	{
#ifdef PHP_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
		if (status == FBRunning) break;
	}
	if (status != FBRunning)
	{
		php_error(E_WARNING, "Database %s@%s created -- status unknown",databaseName, phpLink->hostName);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int fbsql_drop_db(string database_name [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_drop_db)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	int i, status;
	char *databaseName;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
	if (status != FBStopped)
	{
		char* txt = "Unknown status";
		if      (status == FBStopped      ) txt = "stopped";
		else if (status == FBUnknownStatus) txt = "nonexisting";
		else if (status == FBStarting     ) txt = "starting";
		else if (status == FBRunning      ) txt = "running";
		else if (status == FBStopping     ) txt = "stopping";
		else if (status == FBNoExec       ) txt = "no exec";
		php_error(E_WARNING, "Could not drop %s@%s, database is %s.",databaseName,phpLink->hostName,txt);
		RETURN_FALSE;
	}

	if (! fbcehDeleteDatabaseNamed (phpLink->execHandler, databaseName))
	{
		char* error = fbechErrorMessage(phpLink->execHandler);
		php_error(E_WARNING, "Could not drop %s@%s. %s.",databaseName,phpLink->hostName,error);
		RETURN_FALSE;
	}
	for (i=0; i < 20; i++)
	{
#ifdef PHP_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
		if (status == FBUnknownStatus) break;
	}
	if (status != FBUnknownStatus)
	{
		php_error(E_WARNING, "Database %s@%s dropped -- status unknown",databaseName,phpLink->hostName);
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int fbsql_start_db(string database_name [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_start_db)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	int i, status;
	char *databaseName;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
	if ((status != FBStopped) && (status != FBRunning) && (status != FBStarting))
	{
		char* txt = "Unknown status";
		if      (status == FBStopped ) txt = "stopped";
		else if (status == FBStarting) txt = "starting";
		else if (status == FBRunning ) txt = "running";
		else if (status == FBStopping) txt = "stopping";
		else if (status == FBNoExec  ) txt = "no exec";
		php_error(E_WARNING, "Could not start %s@%s, as database is %s.",databaseName,phpLink->hostName,txt);
		RETURN_FALSE;
	}

	if (status == FBStopped)
	{
		if (!fbcehStartDatabaseNamed (phpLink->execHandler, databaseName))
		{
			char* error = fbechErrorMessage(phpLink->execHandler);
			php_error(E_WARNING, "Could not start %s@%s. %s.",databaseName,phpLink->hostName,error);
			RETURN_FALSE;
		}
	}

	for (i=0; i < 20; i++)
	{
#ifdef PHP_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
		if (status == FBRunning) break;
	}
	if (status != FBRunning)
	{
		php_error(E_WARNING, "Database %s@%s started -- status unknown",databaseName,phpLink->hostName);
		RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int fbsql_stop_db(string database_name [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_stop_db)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	int i, status;
	char *databaseName, name[1024];
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	if (!php_fbsql_select_db(databaseName, phpLink)) RETURN_FALSE;

/*	printf("Stop db %x\n",phpDatabase->connection); */
	if (!fbcdcStopDatabase(phpLink->connection))
	{
		php_error(E_WARNING, "Cannot stop database %s@%s",databaseName,phpLink->hostName);
		RETURN_FALSE;
	}

	for (i=0; i < 20; i++)
	{
		status = fbcehStatusForDatabaseNamed(phpLink->execHandler,databaseName);
		if (status == FBStopped) break;
#ifdef PHP_WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}

//	for (i=0; i < phpDatabase->resultCount; i++) if (phpDatabase->results[i])
//	{
//		FB_SQL_G(resultCount)--;
//		zend_list_delete(phpDatabase->results[i]->index);
//	}
/*	printf("Database %X %d %d\n",phpDatabase,phpDatabase->index,phpDatabase->retainCount); */
	sprintf(name,"fbsql_%s@%s:%s",databaseName,phpLink->hostName, phpLink->userName);
//	zend_list_delete(phpDatabase->index);

	zend_hash_del(&EG(regular_list),name,strlen(name));

/*	printf("After list delete\n"); */
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int fbsql_db_status(string database_name [, int link_identifier])
	Get the status (Stoped, Starting, Started, Stopping) for a given database*/
PHP_FUNCTION(fbsql_db_status)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	char *databaseName;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	if (phpLink->execHandler) {
		RETURN_LONG(fbcehStatusForDatabaseNamed(phpLink->execHandler, databaseName));
	}
	else {
		RETURN_FALSE;
	}
}
/* }}} */

int mdOk(PHPFBLink* link, FBCMetaData* md)
{
	FBCDatabaseConnection* c = link->connection;
	int result = 1;
	FBSQLLS_FETCH();

	link->errorNo = 0;
	if (link->errorText)
	{
		efree(link->errorText);
		link->errorText = NULL;
	}
	if (md == NULL)
	{
		link->errorNo  = 1;
		link->errorText  = estrdup("Connection to database server was lost");
		if (FB_SQL_G(generateWarnings)) php_error(E_WARNING, link->errorText);
		result = 0;
	}
	else if (fbcmdErrorsFound(md))
	{
		FBCErrorMetaData* emd = fbcdcErrorMetaData(c,md);
		char*             emg = fbcemdAllErrorMessages(emd);
		if (FB_SQL_G(generateWarnings))
		{
			if (emg)
				php_error(E_WARNING, emg);
			else
				php_error(E_WARNING,"No message");
		}
		link->errorText = emg;
		link->errorNo  = 1;
		fbcemdRelease(emd);
		result = 0;
	}
	return result;
}

static int phpfbQuery(INTERNAL_FUNCTION_PARAMETERS, char* sql, PHPFBLink* link)
{
	PHPFBResult*  result = NULL;
	FBCMetaData*   md, *meta;
	int            ok;
	char*          tp;
	char*          fh; 
	unsigned int   sR = 1, cR = 0;
	FBSQLLS_FETCH();

	meta     = fbcdcExecuteDirectSQL(link->connection, sql);

	if (fbcmdHasMetaDataArray(meta)) {
		sR = fbcmdMetaDataArrayCount(meta);
		md = (FBCMetaData*)fbcmdMetaDataAtIndex(meta, cR);
	}
	else
		md = meta;

	ok     = mdOk(link, md);
	tp     = fbcmdStatementType(md);


	if (!ok)
	{
		return_value->value.lval = 0;
		return_value->type       = IS_LONG;
	}
	else if ((tp[0] == 'C') || (tp[0] == 'R'))
	{
		return_value->value.lval = 1;
		return_value->type       = IS_LONG;
	}
	else if (tp[0] == 'I')
	{
		link->insert_id = fbcmdRowIndex(md);
		return_value->value.lval = 1;
		return_value->type       = IS_LONG;
	}
	else if ((fh = fbcmdFetchHandle(md)) || (tp[0] == 'E'))
	{
		result = emalloc(sizeof(PHPFBResult));
		result->link        = link;
		result->fetchHandle = fh;
		result->ResultmetaData    = meta;
		result->metaData    = md;
		result->rowHandler  = NULL;
		result->batchSize   = FB_SQL_G(batchSize);
		result->rowCount    = 0x7fffffff;
		result->columnCount = 0;
		result->rowIndex    = 0;
		result->columnIndex = 0;
		result->row         = NULL;
		result->array       = NULL;
		result->list        = NULL;
		result->selectResults = sR;
		result->currentResult = cR;

		if (tp[0] != 'E')
		{
			result->rowCount    = 0x7fffffff;
			result->columnCount = fbcmdColumnCount(md);
			result->fetchHandle = fh;
			result->batchSize   = FB_SQL_G(batchSize);
		}
		else 
		{
			char* r = fbcmdMessage(result->metaData);
			if ((result->list = fbcplParse(r)))
			{
				result->rowCount    = fbcplCount(result->list);
				result->columnCount = 7;
			}
		}
		ZEND_REGISTER_RESOURCE(return_value, result, le_result);
	}
	if (link) link->affectedRows = fbcmdRowCount(md);
	if (result == NULL) fbcmdRelease(meta);
	return 1;
}

/* {{{ proto resource fbsql_query(string query [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_query)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **query;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &query)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &query, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(query);

	phpfbQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU, (*query)->value.str.val, phpLink);
}
/* }}} */


/* {{{ proto int fbsql_db_query(string database_name, string query [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_db_query)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **dbname, **query;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 2:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(2, &dbname, &query)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			if (zend_get_parameters_ex(3, &dbname, &query, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(query);
	convert_to_string_ex(dbname);

	if (php_fbsql_select_db((*dbname)->value.str.val, phpLink))
	{
		phpfbQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU, (*query)->value.str.val, phpLink);
	}
}
/* }}} */


/* {{{ proto int fbsql_list_dbs([int link_identifier])
	*/
PHP_FUNCTION(fbsql_list_dbs)
{
	PHPFBResult*    phpResult;
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	phpResult = emalloc(sizeof(PHPFBResult));
	phpResult->link        = phpLink;
	phpResult->fetchHandle = NULL;
	phpResult->rowHandler  = NULL;
	phpResult->ResultmetaData    = NULL;
	phpResult->metaData    = NULL;
	phpResult->batchSize   = FB_SQL_G(batchSize);
	phpResult->columnCount = 1;
	phpResult->rowIndex    = 0;
	phpResult->columnIndex = 0;
	phpResult->row         = NULL;
	phpResult->array       = fbcehAvailableDatabases(phpLink->execHandler);
	phpResult->rowCount    = fbaCount(phpResult->array);
	phpResult->list        = NULL;

	ZEND_REGISTER_RESOURCE(return_value, phpResult, le_result);

}
/* }}} */


/* {{{ proto int fbsql_list_tables(string database, int [link_identifier]);
	*/
PHP_FUNCTION(fbsql_list_tables)
{
	char* sql = "select t0.\"table_name\"from information_schema.tables t0, information_schema.SCHEMATA t1 where t0.schema_pk = t1.schema_pk and t1.\"schema_name\" = current_schema;";
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name;
	int id;
	char *databaseName;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(1, &database_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &database_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;

	if (databaseName == NULL)
	{
		php_fbsql_select_db(FB_SQL_G(databaseName), phpLink);
	}
	else 
	{
		php_fbsql_select_db(databaseName, phpLink);
	}

	phpfbQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU, sql, phpLink);

	if (return_value->value.lval)
	{
//		FB_SQL_G(linkIndex)                   = phpLink->index;
//		if (phpResult) FB_SQL_G(resultIndex)  = phpResult->index;
	}
}
/* }}} */


/* {{{ proto int fbsql_list_fields(string database_name, string table_name [, int link_identifier])
	*/
PHP_FUNCTION(fbsql_list_fields)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL, **database_name, **table_name;
	int id;
	char *databaseName, *tableName;
	char             sql[1024];
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 2:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			if (zend_get_parameters_ex(2, &database_name, &table_name)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			if (zend_get_parameters_ex(3, &database_name, &table_name, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	convert_to_string_ex(database_name);
	databaseName = (*database_name)->value.str.val;
	convert_to_string_ex(table_name);
	tableName = (*table_name)->value.str.val;

	if (!php_fbsql_select_db(databaseName, phpLink)) RETURN_FALSE;

	sprintf(sql,"EXTRACT TABLE %s;",tableName);

	phpfbQuery(INTERNAL_FUNCTION_PARAM_PASSTHRU, sql, phpLink);
}
/* }}} */


/* {{{ proto string fbsql_error([int link_identifier])
	*/
PHP_FUNCTION(fbsql_error)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	if (phpLink->errorText == NULL) {
		RETURN_FALSE;
	}
	else {
		RETURN_STRING(phpLink->errorText, 1);
	}
}
/* }}} */


/* {{{ proto int fbsql_errno([int link_identifier])
	*/
PHP_FUNCTION(fbsql_errno)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	RETURN_LONG(phpLink->errorNo);
}
/* }}} */


/* {{{ proto bool fbsql_warnings([int flag]);
	*/
PHP_FUNCTION(fbsql_warnings)
{
	int   argc     = ARG_COUNT(ht);
	zval	**argv[1];
	FBSQLLS_FETCH();

	if ((argc < 0) || (argc > 1)) WRONG_PARAM_COUNT;
	if (zend_get_parameters_ex(argc,&argv[0])==FAILURE) RETURN_FALSE;
	if (argc >= 1)
	{
		convert_to_long_ex(argv[0]);
		FB_SQL_G(generateWarnings) = (*argv[0])->value.lval != 0;
	}
	RETURN_BOOL(FB_SQL_G(generateWarnings));
}
/* }}} */


/* {{{ proto int fbsql_affected_rows([int link_identifier])
	*/
PHP_FUNCTION(fbsql_affected_rows)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	RETURN_LONG(phpLink->affectedRows);
}
/* }}} */

/* {{{ proto int fbsql_insert_id([int link_identifier])
	*/
PHP_FUNCTION(fbsql_insert_id)
{
	PHPFBLink* phpLink = NULL;
	zval	**fbsql_link_index = NULL;
	int id;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 0:
			id = php_fbsql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU FBSQLLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_link_index)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE2(phpLink, PHPFBLink *, fbsql_link_index, id, "FrontBase-Link", le_link, le_plink);

	RETURN_LONG(phpLink->insert_id);
}
/* }}} */


int phpSizeOfInt (int i)
{
	int s = 1;
	if (i < 0)
	{
		s++;
		i = -i;
	}
	while ((i = i / 10)) s++;
	return s;
}

void phpfbColumnAsString (PHPFBResult* result, int column, void* data ,int* length, char** value)
{
	FBCMetaData*               md          = result->metaData;
	const FBCDatatypeMetaData* dtmd        = fbcmdDatatypeMetaDataAtIndex(md, column);
	unsigned                   dtc         = fbcdmdDatatypeCode(dtmd);
	switch (dtc)
	{
		case FB_Boolean:
		{
			unsigned char v = *((unsigned char*)(data));
			if (v == 255)
				phpfbestrdup("Unknown",length,value);
			else if (v == 0)
				phpfbestrdup("False",length,value);
			else
				phpfbestrdup("True",length,value);
		}
		break;
        
		case FB_PrimaryKey:
		case FB_Integer:
		{ 
			int   v = *((int*)data);
			char  b[128];
			sprintf(b,"%d",v);
			phpfbestrdup(b,length,value);
		}
		break;

		case FB_SmallInteger:
		{
			short v = *((short*)data);
			char  b[128];
			sprintf(b,"%d",v);
			phpfbestrdup(b,length,value);
		}
		break; 

		case FB_Float:
		case FB_Real:
		case FB_Double:
		case FB_Numeric:
		case FB_Decimal:
		{
			double v = *((double*)data);
			char  b[128];
			sprintf(b,"%f",v);
			phpfbestrdup(b,length,value);
		}
		break;

		case FB_Character:
		case FB_VCharacter:
		{
			char* v = (char*)data;
			phpfbestrdup(v,length,value);
		}
		break;

		case FB_Bit:
		case FB_VBit:
		{
			const FBCColumnMetaData* clmd  =  fbcmdColumnMetaDataAtIndex(md,column);
			struct bitValue
			{
				unsigned int   nBytes;
				unsigned char* bytes;
			};
			struct bitValue*  ptr = data;
			unsigned nBits = ptr->nBytes * 8;

			if (dtc == FB_Bit) nBits = fbcdmdLength(fbccmdDatatype(clmd));
			if (nBits %8 == 0)
			{
				unsigned i;
				unsigned int l = nBits / 8;
				*length = l + 5;
				if (value)
				{
					char*        r = emalloc(l*2+3+1);
					r[0] = 'X';
					r[1] = '\'';
					for (i = 0; i < nBits / 8; i++)
					{
						char c[4];
						sprintf(c,"%02x",ptr->bytes[i]);
						r[i*2+2] = c[0];
						r[i*2+3] = c[1];
					}
					r[i*2+2] = '\'';
					r[i*2+3] = 0;
					*value  = r;
				}
			}
			else
			{
				unsigned i;
				unsigned int l = nBits;
				*length = l + 5;
				if (value)
				{
					char*        r = emalloc(l*2+3+1);
					r[0] = 'B';
					r[1] = '\'';
					for (i = 0; i < nBits; i++)
					{
						int bit = 0;
						if (i/8 < ptr->nBytes) bit = ptr->bytes[i/8] & (1<<(7-(i%8)));
						r[i*2+2] = bit?'1':'0';
					}
					r[i*2+2] = '\'';
					r[i*2+3] = 0;
					*value  = r;
				}
			}
		}
		break;

		case FB_Date:
		case FB_Time:
		case FB_TimeTZ:
		case FB_Timestamp:
		case FB_TimestampTZ:
		{
			char* v = (char*)data;
			phpfbestrdup(v,length,value);
		}
		break;

		case FB_YearMonth:
		{
			char b[128];
			int  v = *((unsigned int*)data);
			sprintf(b,"%d",v);
			phpfbestrdup(b,length,value);
		}
		break;

		case FB_DayTime:
		{
			char b[128];
			double seconds = *((double*)data);
			sprintf(b,"%f",seconds);
			phpfbestrdup(b,length,value);
		}
		break;

		case FB_CLOB:
		case FB_BLOB:
/*      {
			unsigned char* bytes = (unsigned char*)data;
			if (*bytes == '\1')
			{  /* Direct
				unsigned int   l   = *((unsigned int*)(bytes+1));
				unsigned char* ptr = *((unsigned char**)(bytes+5));
				unsigned int   i;
				mf(file,"%4d:",l);
				for (i=0; i < l; i++)
				{
					if (i)
					{
						if ((i % 32) == 0) 
							mf(file,"\n     %*d:",lw+4,i);
						else if ((i % 4) == 0) 
							mf(file,"  ");
					}
					mf(file,"%02x",*ptr++);
				}
			}
			else
			{
				mf(file,"%s",bytes+1);
			}
		}
		break;
*/
		default:
			php_error(E_WARNING,"Unimplemented type");
		break;
	}
}

void phpfbSqlResult (INTERNAL_FUNCTION_PARAMETERS, PHPFBResult* result, int rowIndex, int  columnIndex)
{
	void** row;
	if (result->list)
	{
		FBCPList* columns = (FBCPList*)fbcplValueForKey(result->list,"COLUMNS");
		FBCPList* column  = (FBCPList*)fbcplValueAtIndex(columns,result->rowIndex);
		if (columnIndex == 0)  
		{ /* Name */
			FBCPList* name = (FBCPList*)fbcplValueForKey(column,"NAME");
			RETURN_STRING((char *)fbcplString((FBCPList*)name), 1);
		}
		else if (columnIndex == 2)
		{ /* Length */
			FBCPList* name = (FBCPList*)fbcplValueForKey(column,"WIDTH");
			RETURN_STRING((char *)fbcplString((FBCPList*)name), 1);
		}
		else if (columnIndex == 1)
		{ /* Type */
			FBCPList* name = (FBCPList*)fbcplValueForKey(column,"DATATYPE");
			RETURN_STRING((char *)fbcplString((FBCPList*)name), 1);
		}
		else if (columnIndex == 3)
		{ /* Flags */
			RETURN_STRING("", 1);
		}
		else
		{
			RETURN_STRING("", 1);
		}
	}
	else if (result->array)
	{ /* Special case for get dbs */
		RETURN_STRING(fbaObjectAtIndex(result->array,rowIndex), 1);
	}
	else if (!phpfbFetchRow(result,rowIndex))
	{
		php_error(E_WARNING,"No such row %d in result set %d",rowIndex,rowIndex);
		RETURN_FALSE;
	}
	else if (columnIndex >= result->columnCount)
	{
		php_error(E_WARNING,"No such column %d in result set %d",columnIndex,rowIndex);
		RETURN_FALSE;
	}
	else
	{
		row = fbcrhRowAtIndex(result->rowHandler,rowIndex);
		if (row == NULL)
		{
			RETURN_FALSE;
		}
		else if (row[columnIndex])
		{
			phpfbColumnAsString(result,columnIndex,row[columnIndex],&return_value->value.str.len,&return_value->value.str.val);
			return_value->type = IS_STRING;
		}
		else
		{
			RETURN_NULL();
		}
	}
}
                       
/* {{{ proto mixed fbsql_result(int result [, int row [, mixed field]])
	*/
PHP_FUNCTION(fbsql_result)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **row = NULL, **field = NULL;
	int rowIndex;
	int columnIndex;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &row)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			if (zend_get_parameters_ex(3, &fbsql_result_index, &row, &field)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	rowIndex = result->rowIndex;
	if (row)
	{
		convert_to_long_ex(row);
		rowIndex = Z_LVAL_PP(row);
	}

	columnIndex  = result->columnIndex;
	if (field)
	{
		if (((*field)->type == IS_STRING) && (result->metaData))
		{
			for (columnIndex =0; columnIndex < result->columnCount; columnIndex ++)
			{
				const FBCColumnMetaData* cmd = fbcmdColumnMetaDataAtIndex(result->metaData, columnIndex);
				const char*              lbl = fbccmdLabelName(cmd);
				if (strcmp((char*)lbl, (*field)->value.str.val) == 0) break;
			}
			if (columnIndex == result->columnCount) RETURN_FALSE;
		}
		else
		{
			convert_to_long_ex(field);
			columnIndex = (*field)->value.lval;
			if (columnIndex < 0)
			{
				php_error(E_WARNING,"Illegal column index - %d",columnIndex);
				RETURN_FALSE;
			}
		}
    }
   
	phpfbSqlResult(INTERNAL_FUNCTION_PARAM_PASSTHRU,result,rowIndex,columnIndex);

	result->columnIndex++;
	if (result->columnIndex == result->columnCount)
	{
		result->rowIndex++;
		result->columnIndex = 0;
	}
}
/* }}} */


/* {{{ proto int fbsql_next_result(int result)
	*/
PHP_FUNCTION(fbsql_next_result)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

    result->currentResult++;
	if (result->currentResult < result->selectResults) {
        if (result->fetchHandle) {
			FBCMetaData *md = fbcdcCancelFetch(result->link->connection, result->fetchHandle);
			fbcmdRelease(md);
		}
		if (result->rowHandler) fbcrhRelease(result->rowHandler);
		result->metaData    = (FBCMetaData*)fbcmdMetaDataAtIndex(result->ResultmetaData, result->currentResult);
		result->fetchHandle = fbcmdFetchHandle(result->metaData);
		result->rowHandler  = NULL;
		result->batchSize   = FB_SQL_G(batchSize);
		result->rowCount    = 0x7fffffff;
		result->columnCount = fbcmdColumnCount(result->metaData);;
		result->rowIndex    = 0;
		result->columnIndex = 0;
		result->row         = NULL;
		result->array       = NULL;
		result->list        = NULL;
		if (result->link) 
			result->link->affectedRows = fbcmdRowCount(result->metaData);

		RETURN_TRUE;
	}
	else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto int fbsql_num_rows(int result)
	*/
PHP_FUNCTION(fbsql_num_rows)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL;
	int rowCount;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	if (result->array)
		rowCount = result->rowCount;
	else {
		rowCount = fbcmdRowCount(result->metaData);
		if (rowCount == -1)
		{
			phpfbFetchRow(result,0x7fffffff);
			rowCount = result->rowCount;
		}
	}
	RETURN_LONG(rowCount);
}
/* }}} */


/* {{{ proto int fbsql_num_fields(int result)
	*/
PHP_FUNCTION(fbsql_num_fields)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	RETURN_LONG(result->columnCount);
}
/* }}} */


/* {{{ proto array fbsql_fetch_row(int result)
	*/
PHP_FUNCTION(fbsql_fetch_row)
{
	php_fbsql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBSQL_NUM);
}
/* }}} */


/* {{{ proto object fbsql_fetch_assoc(int result)
	*/
PHP_FUNCTION(fbsql_fetch_assoc)
{
	php_fbsql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBSQL_ASSOC);
}
/* }}} */


/* {{{ proto object fbsql_fetch_object(int result [, int result_type])
	*/
PHP_FUNCTION(fbsql_fetch_object)
{
	php_fbsql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBSQL_ASSOC);
	if (return_value->type==IS_ARRAY)
	{
		return_value->type=IS_OBJECT;
		return_value->value.obj.properties = return_value->value.ht;
		return_value->value.obj.ce = &zend_standard_class_def;
	}
}
/* }}} */


/* {{{ proto array fbsql_fetch_array(int result [, int result_type])
   Fetch a result row as an array (associative, numeric or both)*/
PHP_FUNCTION(fbsql_fetch_array)
{
	php_fbsql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, FBSQL_NUM);
}
/* }}} */

static void php_fbsql_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int result_type)
{

	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **zresult_type = NULL;
	int rowIndex;
	int i;
	void **row;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &zresult_type)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(zresult_type);
			result_type = Z_LVAL_PP(zresult_type);
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	rowIndex = result->rowIndex;
	if (((result_type & FBSQL_NUM) != FBSQL_NUM) && ((result_type & FBSQL_ASSOC) != FBSQL_ASSOC))
	{
		php_error(E_WARNING,"Illegal result type use FBSQL_NUM, FBSQL_ASSOC, or FBSQL_BOTH.");
		RETURN_FALSE;
	}
	if (array_init(return_value)==FAILURE)
	{
		RETURN_FALSE;
	}
	if (result->fetchHandle == NULL)
	{
		if (result->array == NULL)
		{
			RETURN_FALSE;
		}
		if (result->rowIndex >= result->rowCount)
		{
			RETURN_FALSE;
		}
		if (result_type & FBSQL_NUM)
		{
			add_index_string(return_value,0,estrdup(fbaObjectAtIndex(result->array,result->rowIndex)),0);
		}
		if (result_type & FBSQL_ASSOC)
		{
			add_assoc_string(return_value, "Database", estrdup(fbaObjectAtIndex(result->array,result->rowIndex)), 0);
		}
	}
	else {
		if (result->rowCount == 0) {
			RETURN_FALSE;
		}
		if (result->rowCount == 0x7fffffff)
		{
			if (!phpfbFetchRow(result,result->rowIndex)) {
				RETURN_FALSE;
			}
		}
		row = fbcrhRowAtIndex(result->rowHandler,rowIndex);
		if (row == NULL)
		{
			RETURN_FALSE;
		}
		for (i=0; i < result->columnCount; i++)
		{
			if (row[i])
			{
				char*        value;
				unsigned int length;
				unsigned int c = 0;
				phpfbColumnAsString(result,i,row[i],&length,&value);
				if (result_type & FBSQL_NUM)
				{
					add_index_stringl(return_value,i,value,length,c);
					c = 1;
				}
				if (result_type & FBSQL_ASSOC)
				{
					char* key = (char*)fbccmdLabelName(fbcmdColumnMetaDataAtIndex(result->metaData, i));
					add_assoc_stringl(return_value,key, value, length, c);
				}
			}
			else
			{
				if (result_type & FBSQL_NUM)
				{
					add_index_unset(return_value,i);
				}
				if (result_type & FBSQL_ASSOC)
				{
					char* key = (char*)fbccmdLabelName(fbcmdColumnMetaDataAtIndex(result->metaData, i));
					add_assoc_unset(return_value,key);
				}
			}
		}
	}
	result->rowIndex    = result->rowIndex+1;
	result->columnIndex = 0;
}


/* {{{ proto int fbsql_data_seek(int result, int row_number)
	*/
PHP_FUNCTION(fbsql_data_seek)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **row_number = NULL;
	int rowIndex;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &row_number)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	convert_to_long_ex(row_number);
	rowIndex = Z_LVAL_PP(row_number);

	if (rowIndex < 0)
	{
		php_error(E_WARNING,"Illegal row_index (%i)", rowIndex);
		RETURN_FALSE;
	}

	if (result->rowCount == 0x7fffffff) phpfbFetchRow(result,rowIndex);
	if (rowIndex > result->rowCount) RETURN_FALSE;
	result->rowIndex = rowIndex;

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto array fbsql_fetch_lengths(int result)
	*/
PHP_FUNCTION(fbsql_fetch_lengths)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL;
	int i;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	if (result->row == NULL) RETURN_FALSE;
	if (array_init(return_value)==FAILURE) RETURN_FALSE;
	for (i=0; i < result->columnCount; i++)
	{
		unsigned  length = 0;
		if (result->row[i]) phpfbColumnAsString(result,i, result->row[i],&length,NULL);
		add_index_long(return_value, i, length);
	}
}
/* }}} */


/* {{{ proto object fbsql_fetch_field(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_fetch_field)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	if (object_init(return_value)==FAILURE)
	{
		RETURN_FALSE;
	}
	add_property_string(return_value, "name",       (char*)fbccmdLabelName(fbcmdColumnMetaDataAtIndex(result->metaData, column)),1);
	add_property_string(return_value, "table",      (char*)fbccmdTableName(fbcmdColumnMetaDataAtIndex(result->metaData,column)),1);
	add_property_long(return_value,   "max_length", fbcdmdLength(fbccmdDatatype(fbcmdColumnMetaDataAtIndex(result->metaData,column))));
	add_property_string(return_value, "type",       (char*)fbcdmdDatatypeString(fbcmdDatatypeMetaDataAtIndex(result->metaData, column)),1);
	add_property_long(return_value,   "not_null",   !fbccmdIsNullable(fbcmdColumnMetaDataAtIndex(result->metaData, column)));
/*	Remember to add the rest */
/*	add_property_long(return_value, "primary_key",IS_PRI_KEY(fbsql_field->flags)?1:0); */
/*	add_property_long(return_value, "multiple_key",(fbsql_field->flags&MULTIPLE_KEY_FLAG?1:0)); */
/*	add_property_long(return_value, "unique_key",(fbsql_field->flags&UNIQUE_KEY_FLAG?1:0)); */
/*	add_property_long(return_value, "numeric",IS_NUM(fbsql_field->type)?1:0); */
/*	add_property_long(return_value, "blob",IS_BLOB(fbsql_field->flags)?1:0); */
/*	add_property_long(return_value, "unsigned",(fbsql_field->flags&UNSIGNED_FLAG?1:0)); */
/*	add_property_long(return_value, "zerofill",(fbsql_field->flags&ZEROFILL_FLAG?1:0)); */
}
/* }}} */


/* {{{ proto bool fbsql_field_seek(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_seek)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}

	result->columnIndex = column;
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto string fbsql_field_name(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_name)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	if (result->list)
	{
		phpfbSqlResult(INTERNAL_FUNCTION_PARAM_PASSTHRU,result,result->rowIndex,0);
	}
	else if (result->metaData)
	{
		RETURN_STRING((char *)fbccmdLabelName(fbcmdColumnMetaDataAtIndex(result->metaData, column)), 1);
		result->columnIndex = column;
	}
}
/* }}} */


/* {{{ proto string fbsql_field_table(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_table)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	RETURN_STRING((char *)fbccmdTableName(fbcmdColumnMetaDataAtIndex(result->metaData,column)), 1);
}
/* }}} */


/* {{{ proto string fbsql_field_len(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_len)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	if (result->list)
	{
		phpfbSqlResult(INTERNAL_FUNCTION_PARAM_PASSTHRU,result,result->rowIndex,2);
	}
	else if (result->metaData)
	{
		unsigned int length = fbcdmdLength(fbccmdDatatype(fbcmdColumnMetaDataAtIndex(result->metaData,column)));
		char         buffer[50];
		sprintf(buffer,"%d",length);
		RETURN_STRING(buffer, 1);
	}
	else
	{
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto string fbsql_field_type(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_type)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	if (result->list)
	{
		phpfbSqlResult(INTERNAL_FUNCTION_PARAM_PASSTHRU,result,result->rowIndex,1);
	}
	else if (result->metaData)
	{
		RETURN_STRING((char *)fbcdmdDatatypeString (fbcmdDatatypeMetaDataAtIndex(result->metaData, column)), 1);
	}
	else
	{
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto string fbsql_field_flags(int result [, int field_index])
	*/
PHP_FUNCTION(fbsql_field_flags)
{
	PHPFBResult* result = NULL;
	zval **fbsql_result_index = NULL, **field_index = NULL;
	int column = -1;
	char buf[512];
	int len;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (zend_get_parameters_ex(2, &fbsql_result_index, &field_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	column = result->columnIndex;
	if (field_index)
	{
		convert_to_long_ex(field_index);
		column = Z_LVAL_PP(field_index);
		if (column < 0 || column >= result->columnCount)
		{
			php_error(E_WARNING,"%d no such column in result",column);
			RETURN_FALSE;
		}
	}
	strcpy(buf, "");
	if (!fbccmdIsNullable(fbcmdColumnMetaDataAtIndex(result->metaData, column))) {
		strcat(buf, "not_null ");
	}
#if 0
	if (IS_PRI_KEY(fbsql_field->flags)) {
		strcat(buf, "primary_key ");
	}
	if (fbsql_field->flags&UNIQUE_KEY_FLAG) {
		strcat(buf, "unique_key ");
	}
	if (fbsql_field->flags&MULTIPLE_KEY_FLAG) {
		strcat(buf, "multiple_key ");
	}
	if (IS_BLOB(fbsql_field->flags)) {
		strcat(buf, "blob ");
	}
	if (fbsql_field->flags&UNSIGNED_FLAG) {
		strcat(buf, "unsigned ");
	}
	if (fbsql_field->flags&ZEROFILL_FLAG) {
		strcat(buf, "zerofill ");
	}
	if (fbsql_field->flags&BINARY_FLAG) {
		strcat(buf, "binary ");
	}
	if (fbsql_field->flags&ENUM_FLAG) {
		strcat(buf, "enum ");
	}
	if (fbsql_field->flags&AUTO_INCREMENT_FLAG) {
		strcat(buf, "auto_increment ");
	}
	if (fbsql_field->flags&TIMESTAMP_FLAG) {
		strcat(buf, "timestamp ");
	}
#endif
	len = strlen(buf);
	/* remove trailing space, if present */
	if (len && buf[len-1] == ' ') {
		buf[len-1] = 0;
		len--;
	}
	RETURN_STRING(buf, 1);
}
/* }}} */


/* {{{ proto bool fbsql_free_result(int result)
	*/
PHP_FUNCTION(fbsql_free_result)
{
	PHPFBResult* result = NULL;
	zval	**fbsql_result_index = NULL;
	FBSQLLS_FETCH();

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (zend_get_parameters_ex(1, &fbsql_result_index)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	ZEND_FETCH_RESOURCE(result, PHPFBResult *, fbsql_result_index, -1, "FrontBase-Result", le_result);

	zend_list_delete((*fbsql_result_index)->value.lval);
	RETURN_TRUE;
}
/* }}} */

#endif
