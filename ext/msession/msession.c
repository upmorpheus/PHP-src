/*
   +----------------------------------------------------------------------+
   | msession 1.0                                                         |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Mark Woodward <markw@mohawksoft.com>                        |
   |    Portions copyright the PHP group.                                 |
   +----------------------------------------------------------------------+
 */
#include "php.h"
#include "php_ini.h"
#include "php_msession.h"
#include "reqclient.h"
#include "../session/php_session.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// #define ERR_DEBUG

#ifdef ERR_DEBUG
#define ELOG( str )	php_log_err( str )
#else
#define ELOG( str )
#endif

char g_msession[]="Msession";


#if HAVE_MSESSION

#ifdef HAVE_PHP_SESSION
PS_FUNCS(msession);

ps_module ps_mod_msession = {
	PS_MOD(msession)
};
#endif


// #define ERR_DEBUG

/* True global resources - no need for thread safety here */
static char	g_defhost[]="localhost";
static char *	g_host=g_defhost;
static int	g_port=8086;
static void *	g_conn=NULL;
static REQB *	g_reqb=NULL;

#define REQ_SIZE	1024

#define GET_REQB	\
	if(!g_reqb) { RETURN_NULL(); }

function_entry msession_functions[] = {
	PHP_FE(msession_connect,NULL)
	PHP_FE(msession_disconnect,NULL)
	PHP_FE(msession_lock,NULL)
	PHP_FE(msession_unlock,NULL)
	PHP_FE(msession_count,NULL)
	PHP_FE(msession_create,NULL)
	PHP_FE(msession_destroy,NULL)
	PHP_FE(msession_set,NULL)
	PHP_FE(msession_get,NULL)
	PHP_FE(msession_find,NULL)
	PHP_FE(msession_get_array,NULL)
	PHP_FE(msession_set_array,NULL)
	PHP_FE(msession_timeout,NULL)
	PHP_FE(msession_inc,NULL)
	PHP_FE(msession_setdata,NULL)
	PHP_FE(msession_getdata,NULL)
	PHP_FE(msession_listvar,NULL)
	PHP_FE(msession_list,NULL)
	PHP_FE(msession_uniq,NULL)
	PHP_FE(msession_randstr,NULL)
	PHP_FE(msession_plugin,NULL)
	{NULL, NULL, NULL}	/* Must be the last line in msession_functions[] */
};

zend_module_entry msession_module_entry = {
	STANDARD_MODULE_HEADER,
	"msession",
	msession_functions,
	PHP_MINIT(msession),
	NULL,	
	NULL,	
	PHP_RSHUTDOWN(msession),
	PHP_MINFO(msession),
	NO_VERSION_YET,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_MSESSION
ZEND_GET_MODULE(msession)
#endif

PHP_MINIT_FUNCTION(msession)
{
	g_conn = NULL;
	g_host = g_defhost;
	
#ifdef HAVE_PHP_SESSION
	php_session_register_module(&ps_mod_msession);
#endif

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(msession)
{
	if(g_conn)
		{
			CloseReqConn(g_conn);
			g_conn = NULL;
		}

	if(g_reqb)
		{
			FreeRequestBuffer(g_reqb);
			g_reqb=NULL;
		}
	return SUCCESS;
}

PHP_MINFO_FUNCTION(msession)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "msession support", "enabled");
	php_info_print_table_end();
}

int PHPMsessionConnect(const char *szhost, int nport)
{
	if(!g_reqb)
		g_reqb = AllocateRequestBuffer(2048);

	if(!g_reqb) // no buffer, it won't work!
		return 0;

	if(g_conn)
		{
			CloseReqConn(g_conn);
			php_log_err("Call to connect with non-null g_conn");
		}
	if(strcmp(g_host, szhost))
		{
			if(g_host != g_defhost)
				free(g_host);
			g_host = strdup(szhost);
		}
	if(nport)
		g_port = nport;

	g_conn = OpenReqConn(g_host, g_port);

#ifdef ERR_DEBUG
	{
		char buffer[256];
		sprintf(buffer,"Connect: %s [%d] = %d (%X)\n", 
						g_host, g_port, (g_conn != NULL), (unsigned)g_conn);
		php_log_err(buffer);
	}
#endif
	return (g_conn) ? 1 : 0;
}

void PHPMsessionDisconnect()
{
	if(g_conn)
		{
			CloseReqConn(g_conn);
			g_conn = NULL;
		}
	if(g_reqb)
		{
			FreeRequestBuffer(g_reqb);
			g_reqb = NULL;
		}
}

char *PHPMsessionGetData(const char *session)
{
	char *ret = NULL;

#ifdef ERR_DEBUG
	{
		char buffer [256];
		sprintf(buffer,"PHPMsessionGetData: %s (%X)\n", session, (unsigned)g_conn);
		php_log_err(buffer);
	}
#endif
	if(!g_reqb) 
		{ 
			return NULL ;
		}
	
	FormatRequest(&g_reqb, REQ_DATAGET, session,"","",0);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		ret = safe_estrdup(g_reqb->req.datum);
	return ret;
}

int PHPMsessionSetData(const char *session, const char *data)
{
	int ret=0;
#ifdef ERR_DEBUG
	{
		char buffer [256];
		sprintf(buffer,"PHPMsessionSetData: %s=%s (%X)\n", session, data, (unsigned)g_conn);
		php_log_err(buffer);
	}
#endif
	if(!g_reqb) 
		{ 
			return  0;
		}
	FormatRequest(&g_reqb, REQ_DATASET, session,"",data,0);
	DoRequest(g_conn,&g_reqb);
	ret = (g_reqb->req.stat==REQ_OK);
	return ret;
}

int PHPMsessionDestroy(const char *session)
{
	int ret=0;
	if(!g_reqb) 
		{ 
			return  0;
		}
	FormatRequest(&g_reqb, REQ_DROP, session, "","",0);
	DoRequest(g_conn,&g_reqb);
	ret = (g_reqb->req.stat==REQ_OK);
	return ret;
}

PHP_FUNCTION(msession_connect)
{
	char *szhost;
	int nport;
	zval **zhost;
	zval **zport;

	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &zhost, &zport) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	
	convert_to_string_ex(zhost);
	convert_to_string_ex(zport);

	szhost = Z_STRVAL_PP(zhost);
	nport = atoi(Z_STRVAL_PP(zport));

	if(PHPMsessionConnect(szhost,nport))
		{
			RETURN_TRUE;
		}
	else
		{
			RETURN_FALSE;
		}
}
PHP_FUNCTION(msession_disconnect)
{
	PHPMsessionDisconnect();
	RETURN_NULL();
}

PHP_FUNCTION(msession_count)
{
	if(g_conn)
		{
			int count;
			GET_REQB
				FormatRequest(&g_reqb, REQ_COUNT, "", "","",0);
			DoRequest(g_conn,&g_reqb);
		
			count = (g_reqb->req.stat == REQ_OK) ? g_reqb->req.param : 0;

			RETURN_LONG(count);
		}
	RETURN_NULL();
}

PHP_FUNCTION(msession_create)
{
	int stat;
	zval **session;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);

	FormatRequest(&g_reqb, REQ_CREATE, Z_STRVAL_PP(session), "","",0);
	DoRequest(g_conn,&g_reqb);
	stat = (g_reqb->req.stat==REQ_OK);
	if(stat)
		{
			RETURN_TRUE;
		}
	else
		{
			RETURN_FALSE;
		}
}

PHP_FUNCTION(msession_destroy)
{
	zval **session;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);

	PHPMsessionDestroy(Z_STRVAL_PP(session));

	RETURN_TRUE;
}

PHP_FUNCTION(msession_lock)
{
	long key;
	zval **session;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);

	FormatRequest(&g_reqb, REQ_SLOCK, Z_STRVAL_PP(session), "","",0);
	DoRequest(g_conn,&g_reqb);

	key  = (g_reqb->req.stat == REQ_OK) ? g_reqb->req.param : 0;

	RETURN_LONG( key);
}

PHP_FUNCTION(msession_unlock)
{
	long lkey;
	zval **session;
	zval **key;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &key) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);
	convert_to_long_ex(key);

	FormatRequest(&g_reqb, REQ_SUNLOCK, Z_STRVAL_PP(session), "","",Z_LVAL_PP(key));
	DoRequest(g_conn,&g_reqb);

	lkey  = (g_reqb->req.stat == REQ_OK) ? g_reqb->req.param : 0;

	RETURN_LONG( lkey);
}

PHP_FUNCTION(msession_set)
{
	zval **session;
	zval **name;
	zval **value;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &session, &name, &value) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);
	convert_to_string_ex(name);
	convert_to_string_ex(value);

	FormatRequest(&g_reqb, REQ_SETVAL, Z_STRVAL_PP(session), Z_STRVAL_PP(name), Z_STRVAL_PP(value),0);
	DoRequest(g_conn,&g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		{
			RETURN_TRUE;
		}
	else
		{
			RETURN_FALSE;
		}
}

PHP_FUNCTION(msession_get)
{
	char *val;
	zval **session;
	zval **name;
	zval **value;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &session, &name, &value) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);
	convert_to_string_ex(name);
	convert_to_string_ex(value);

	val = Z_STRVAL_PP(value);

	FormatRequest(&g_reqb, REQ_GETVAL, Z_STRVAL_PP(session), Z_STRVAL_PP(name), val,0);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		val = safe_estrdup(g_reqb->req.datum);

	RETURN_STRING(val, 0);
}

PHP_FUNCTION(msession_uniq)
{
	long val;
	zval **param;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1,&param) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_long_ex(param);
	val = Z_LVAL_PP(param);

	FormatRequest(&g_reqb, REQ_UNIQ,"", "", "",val);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		{
			char *szval = safe_estrdup(g_reqb->req.datum);
			RETURN_STRING(szval, 0)
				}
	else
		{
			RETURN_NULL();
		}
}

PHP_FUNCTION(msession_randstr)
{
	long val;
	zval **param;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1,&param) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_long_ex(param);
	val = Z_LVAL_PP(param);

	FormatRequest(&g_reqb, REQ_RANDSTR,"", "", "",val);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		{
			char *szval = safe_estrdup(g_reqb->req.datum);
			RETURN_STRING(szval, 0)
				}
	else
		{
			RETURN_NULL();
		}
}

PHP_FUNCTION(msession_find)
{
	zval **name;
	zval **value;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &name, &value) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(name);
	convert_to_string_ex(value);

	FormatRequest(&g_reqb, REQ_FIND, "", Z_STRVAL_PP(name), Z_STRVAL_PP(value),0);
	DoRequest(g_conn,&g_reqb);

	if(g_reqb->req.stat==REQ_OK && g_reqb->req.param)
		{
			int i;
			char *str = g_reqb->req.datum;
			array_init(return_value);

			for(i=0; i < g_reqb->req.param; i++)
				{
					int element_len = strlen(str);
					char *data = safe_estrdup(str);
					add_index_string(return_value, i, data, 0);
					str += (element_len+1);
				}
		}
	else
		{
			RETURN_NULL();
		}
}

PHP_FUNCTION(msession_list)
{
	GET_REQB
	
		if(!g_conn)
			{
				RETURN_FALSE;
			}

	FormatRequest(&g_reqb, REQ_LIST, "", "", "",0);
	DoRequest(g_conn,&g_reqb);

	if(g_reqb->req.stat==REQ_OK && g_reqb->req.param)
		{
			int i;
			char *str = g_reqb->req.datum;
			array_init(return_value);

			for(i=0; i < g_reqb->req.param; i++)
				{
					int element_len = strlen(str);
					char *data = safe_estrdup(str);
					add_index_string(return_value, i, data, 0);
					str += (element_len+1);
				}
		}
	else
		{
			RETURN_NULL();
		}
}

PHP_FUNCTION(msession_get_array)
{
	zval **session;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);

	FormatRequest(&g_reqb, REQ_GETALL, Z_STRVAL_PP(session), "", "",0);
	DoRequest(g_conn,&g_reqb);

	array_init(return_value);

	if(g_reqb->req.stat == REQ_OK)
		{
			int i;
			char *str = g_reqb->req.datum;
			int num = g_reqb->req.param*2;

			for(i=0; i < num; i+=2)
				{
					int value_len;
					int name_len;
					char *value_data;
					char *name_data;

					name_len = strlen(str);
					name_data = safe_estrndup(str,name_len);
					str += (name_len+1);

					value_len = strlen(str);
					value_data = safe_estrndup(str,value_len);
					str += (value_len+1);
					add_assoc_string(return_value, name_data, value_data, 0);
				}
		}
}

PHP_FUNCTION(msession_set_array)
{
	zval **session;
	zval **tuples;
	HashPosition pos;
	zval **entry;
	char *key;
	uint keylen;
	ulong numndx;
	int ndx=0;
	char **pairs;
	HashTable *htTuples;
	int i;
	
	int countpair; 
	
	GET_REQB
	
		ELOG("msession_set_array");

	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &tuples) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	htTuples = Z_ARRVAL_PP(tuples);

	countpair = zend_hash_num_elements(htTuples);

	pairs = (char **)emalloc(sizeof(char *) * countpair * 2);

	if(!pairs)
		{
			ELOG("no pairs");
			RETURN_FALSE;
		}

	ELOG("have pairs");

	// Initializes pos
	zend_hash_internal_pointer_reset_ex(htTuples, &pos);

	ELOG("reset pointer");

	for(i=0; i < countpair; i++)
		{
			if(zend_hash_get_current_data_ex(htTuples, (void **)&entry, &pos) != SUCCESS) 
				break;

			if(entry)
				{
					convert_to_string_ex(entry);
					if(zend_hash_get_current_key_ex(htTuples,&key,&keylen,&numndx,0,&pos)== HASH_KEY_IS_STRING)
						{
#ifdef ERR_DEBUG
							{
								char buffer [256];
								sprintf(buffer, "%s=%s\n", key, Z_STRVAL_PP(entry));
								ELOG(buffer);
							}
#endif
							pairs[ndx++] = key;
							pairs[ndx++] = Z_STRVAL_PP(entry);
						}
				}
			zend_hash_move_forward_ex(htTuples, &pos);
		}

	ELOG("FormatMulti");
	FormatRequestMulti(&g_reqb, REQ_SETVAL, Z_STRVAL_PP(session), countpair, pairs,0);
	DoRequest(g_conn,&g_reqb);
	efree((void *)pairs);
}

PHP_FUNCTION(msession_listvar)
{
	zval **name;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &name) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(name);

	FormatRequest(&g_reqb, REQ_LISTVAR, "", Z_STRVAL_PP(name), "",0);
	DoRequest(g_conn,&g_reqb);

	array_init(return_value);

	if(g_reqb->req.stat == REQ_OK)
		{
			int i;
			char *str = g_reqb->req.datum;
			int num = g_reqb->req.param*2;

			for(i=0; i < num; i+=2)
				{
					int value_len;
					int name_len;
					char *value_data;
					char *name_data;

					name_len = strlen(str);
					name_data = safe_estrndup(str,name_len);
					str += (name_len+1);

					value_len = strlen(str);
					value_data = safe_estrndup(str,value_len);
					str += (value_len+1);
					add_assoc_string(return_value, name_data, value_data, 0);
				}
		}
}

PHP_FUNCTION(msession_timeout)
{
	zval **session;
	int ac = ZEND_NUM_ARGS();
	int zstat = FAILURE;
	int timeout = 0;
	GET_REQB
	
	
		if(ac == 1)
			{
				zstat = zend_get_parameters_ex(1, &session);
			}
		else if(ac == 2)
			{
				zval **param;
				zstat = zend_get_parameters_ex(2, &session, &param);
				convert_to_long_ex(param);
				timeout = Z_LVAL_PP(param);
			}
	if(zstat == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);

	FormatRequest(&g_reqb, REQ_TIMEOUT, Z_STRVAL_PP(session), "","",timeout);
	DoRequest(g_conn,&g_reqb);

	if(g_reqb->req.stat == REQ_OK)
		{
			RETURN_LONG( g_reqb->req.param);
		}
	else
		{
			RETURN_NULL();
		}
}

PHP_FUNCTION(msession_inc)
{
	char *val;
	zval **session;
	zval **name;
	GET_REQB
	
		if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &name) == FAILURE)
			{
				WRONG_PARAM_COUNT;
			}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);
	convert_to_string_ex(name);

	FormatRequest(&g_reqb, REQ_INC, Z_STRVAL_PP(session), Z_STRVAL_PP(name),0,0);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK)
		{
			val = safe_estrdup(g_reqb->req.datum);
			RETURN_STRING(val, 0)
				}
	else
		{
			RETURN_FALSE;
		}
}

PHP_FUNCTION(msession_getdata)
{
	char *val;
	zval **session;
	
	if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &session) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);

	val = PHPMsessionGetData(Z_STRVAL_PP(session));

	if(val)
		{
			RETURN_STRING(val, 0)
				}
	else
		{
			RETURN_NULL();
		}

}

PHP_FUNCTION(msession_setdata)
{
	zval **session;
	zval **value;
	
	if(ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &session, &value) == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}
	convert_to_string_ex(session);
	convert_to_string_ex(value);

	if(PHPMsessionSetData(Z_STRVAL_PP(session),Z_STRVAL_PP(value)))
		{
			RETURN_TRUE;
		}
	else
		{
			RETURN_FALSE;
		}
}

PHP_FUNCTION(msession_plugin)
{
	int ret;
	char *retval;
	zval **session;
	zval **val;
	zval **param=NULL;
	GET_REQB
	
		if(ZEND_NUM_ARGS() == 3)
			{
				ret = zend_get_parameters_ex(3, &session, &val, &param);
				convert_to_string_ex(param);
			}
		else if(ZEND_NUM_ARGS() == 2)
			{
				ret = zend_get_parameters_ex(2, &session, &val);
			}
		else
			{
				WRONG_PARAM_COUNT;
			}
	if(ret == FAILURE)
		{
			WRONG_PARAM_COUNT;
		}
	if(!g_conn)
		{
			RETURN_FALSE;
		}

	convert_to_string_ex(session);
	convert_to_string_ex(val);

	ret = atoi(Z_STRVAL_PP(val));

	FormatRequest(&g_reqb, REQ_PLUGIN, Z_STRVAL_PP(session), Z_STRVAL_PP(val), param ? Z_STRVAL_PP(param) : "",ret);
	DoRequest(g_conn, &g_reqb);

	if(g_reqb->req.stat==REQ_OK && g_reqb->req.len)
		{
			retval = safe_estrdup(g_reqb->req.datum);
			RETURN_STRING(retval, 0)
				}
	else
		{
			RETURN_FALSE;
		
		}
}

#ifdef HAVE_PHP_SESSION

PS_OPEN_FUNC(msession)
{
	ELOG( "ps_open_msession");
	PS_SET_MOD_DATA((void *)1); // session.c needs a non-zero here!
	return PHPMsessionConnect(save_path, 8086) ? SUCCESS : FAILURE;
}

PS_CLOSE_FUNC(msession)
{
	PHPMsessionDisconnect();
	ELOG( "ps_close_msession");
	return SUCCESS;
}

PS_READ_FUNC(msession)
{
	ELOG( "ps_read_msession");
	*val = PHPMsessionGetData(key);
	if(*val)
		{
			*vallen = strlen(*val);
		}
	else
		{
			*val = emalloc(1);
			**val=0;
			*vallen = 0;
		}
	return SUCCESS;
}

PS_WRITE_FUNC(msession)
{
	ELOG( "ps_write_msession");
	return (PHPMsessionSetData(key,val)) ? SUCCESS : FAILURE;
}

PS_DESTROY_FUNC(msession)
{
	ELOG( "ps_destroy_msession");
	return (PHPMsessionDestroy(key)) ? SUCCESS : FAILURE;
}

PS_GC_FUNC(msession)
{
	ELOG( "ps_gc_msession");
	return SUCCESS;
}
#endif /* HAVE_PHP_SESSION */
#endif	/* HAVE_MSESSION */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
