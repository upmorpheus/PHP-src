/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Amitay Isaacs  <amitay@w-o-i.com>                           |
   |          Eric Warnke    <ericw@albany.edu>                           |
   |          Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Gerrit Thomson <334647@swin.edu.au>                         |
   | PHP 4.0 updates:  Zeev Suraski <zeev@zend.com>                       |
   +----------------------------------------------------------------------+
 */
 

/* $Id$ */
#define IS_EXT_MODULE

#include "php.h"
#include "php_ini.h"

#include "ext/standard/dl.h"
#include "php_ldap.h"

#ifdef PHP_WIN32
#include <string.h>
#if HAVE_NSLDAP
#include <winsock.h>
#endif
#define strdup _strdup
#undef WINDOWS
#undef strcasecmp
#undef strncasecmp
#define WINSOCK 1
#define __STDC__ 1
#endif

#include "ext/standard/php_string.h"
#include "ext/standard/info.h"

ZEND_DECLARE_MODULE_GLOBALS(ldap)


static int le_result, le_result_entry, le_ber_entry;
static int le_link;

/*
	This is just a small subset of the functionality provided by the LDAP library. All the 
	operations are synchronous. Referrals are not handled automatically.
*/

function_entry ldap_functions[] = {
	PHP_FE(ldap_connect,							NULL)
	PHP_FALIAS(ldap_close,		ldap_unbind,		NULL)
	PHP_FE(ldap_bind,								NULL)
	PHP_FE(ldap_unbind,								NULL)
	PHP_FE(ldap_read,								NULL)
	PHP_FE(ldap_list,								NULL)
	PHP_FE(ldap_search,								NULL)
	PHP_FE(ldap_free_result,						NULL)
	PHP_FE(ldap_count_entries,						NULL)
	PHP_FE(ldap_first_entry,						NULL)
	PHP_FE(ldap_next_entry,							NULL)
	PHP_FE(ldap_get_entries,						NULL)
	PHP_FE(ldap_first_attribute,					NULL)
	PHP_FE(ldap_next_attribute,						NULL)
	PHP_FE(ldap_get_attributes,						NULL)
	PHP_FE(ldap_get_values,							NULL)
	PHP_FE(ldap_get_values_len,						NULL)
	PHP_FE(ldap_get_dn,								NULL)
	PHP_FE(ldap_explode_dn,							NULL)
	PHP_FE(ldap_dn2ufn,								NULL)
	PHP_FE(ldap_add,								NULL)
	PHP_FE(ldap_delete,								NULL)
	PHP_FE(ldap_modify,								NULL)
/* additional functions for attribute based modifications, Gerrit Thomson */
	PHP_FE(ldap_mod_add,							NULL)
	PHP_FE(ldap_mod_replace,						NULL)
	PHP_FE(ldap_mod_del,							NULL)
/* end gjt mod */
	PHP_FE(ldap_errno,								NULL)
	PHP_FE(ldap_err2str,							NULL)
	PHP_FE(ldap_error,								NULL)
	{NULL, NULL, NULL}
};


zend_module_entry ldap_module_entry = {
	"ldap", ldap_functions, PHP_MINIT(ldap), PHP_MSHUTDOWN(ldap), NULL, NULL,
			PHP_MINFO(ldap), STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_LDAP
ZEND_GET_MODULE(ldap)
#endif


static void _close_ldap_link(LDAP *ld)
{
	LDAPLS_FETCH();

	ldap_unbind_s(ld);
	/* php_printf("Freeing ldap connection");*/
	LDAPG(num_links)--;
}


static void _free_ldap_result(LDAPMessage *result)
{
	ldap_msgfree(result);
}


PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY_EX("ldap.max_links",		"-1",	PHP_INI_SYSTEM,			OnUpdateInt,		max_links,			zend_ldap_globals,		ldap_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY("ldap.base_dn",			NULL,	PHP_INI_ALL,			OnUpdateString,		base_dn,			zend_ldap_globals,		ldap_globals)
PHP_INI_END()


static void php_ldap_init_globals(zend_ldap_globals *ldap_globals)
{
	ldap_globals->num_links = 0;
}


PHP_MINIT_FUNCTION(ldap)
{
	ZEND_INIT_MODULE_GLOBALS(ldap, php_ldap_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	/* Constants to be used with deref-parameter in php_ldap_do_search() */
	REGISTER_MAIN_LONG_CONSTANT("LDAP_DEREF_NEVER", LDAP_DEREF_NEVER, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("LDAP_DEREF_SEARCHING", LDAP_DEREF_SEARCHING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("LDAP_DEREF_FINDING", LDAP_DEREF_FINDING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("LDAP_DEREF_ALWAYS", LDAP_DEREF_ALWAYS, CONST_PERSISTENT | CONST_CS);

	le_result = register_list_destructors(_free_ldap_result, NULL);
	le_link = register_list_destructors(_close_ldap_link, NULL);

	ldap_module_entry.type = type;

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(ldap)
{
	return SUCCESS;
}


PHP_MINFO_FUNCTION(ldap)
{
	char maxl[32];
#if HAVE_NSLDAP
	char tmp[32];
	LDAPVersion ver;
	double SDKVersion;
#endif

	LDAPLS_FETCH();

#if HAVE_NSLDAP
/* Print version information */
	SDKVersion = ldap_version( &ver );
#endif

	if (LDAPG(max_links) == -1) {
		snprintf(maxl, 31, "%ld/unlimited", LDAPG(num_links) );
	} else {
		snprintf(maxl, 31, "%ld/%ld", LDAPG(num_links), LDAPG(max_links));
	}
	maxl[31] = 0;

	php_info_print_table_start();
	php_info_print_table_row(2, "LDAP Support", "enabled" );
	php_info_print_table_row(2, "RCS Version", "$Id$" );
	php_info_print_table_row(2, "Total Links", maxl );

#if HAVE_NSLDAP

	snprintf(tmp, 31, "%f", SDKVersion/100.0 );
	tmp[31]=0;
	php_info_print_table_row(2, "SDK Version", tmp );

	snprintf(tmp, 31, "%f", ver.protocol_version/100.0 );
	tmp[31]=0;
	php_info_print_table_row(2, "Highest LDAP Protocol Supported", tmp );

	snprintf(tmp, 31, "%f", ver.SSL_version/100.0 );
	tmp[31]=0;
	php_info_print_table_row(2, "SSL Level Supported", tmp );

	if ( ver.security_level != LDAP_SECURITY_NONE ) {
		snprintf(tmp, 31, "%d", ver.security_level );
		tmp[31]=0;
	} else {
		strcpy(tmp, "SSL not enabled" );
	}
	php_info_print_table_row(2, "Level of Encryption", tmp );

#endif

	php_info_print_table_end();

}


/* {{{ proto int ldap_connect([string host [, int port]])
   Connect to an LDAP server */
PHP_FUNCTION(ldap_connect)
{
	char *host;
	int port;
	/*	char *hashed_details;
	int hashed_details_length;*/
	LDAP *ldap;
	LDAPLS_FETCH();

	switch(ZEND_NUM_ARGS()) {
		case 0: 
			host = NULL;
			port = 0;
			/* hashed_details = estrndup("ldap_", 5);
			hashed_details_length = 4+1; */
			break;

		case 1: {
				pval **yyhost;

				if (zend_get_parameters_ex(1, &yyhost) == FAILURE) {
					RETURN_FALSE;
				}

				convert_to_string_ex(yyhost);
				host = (*yyhost)->value.str.val;
				port = 389; /* Default port */

				/* hashed_details_length = yyhost->value.str.len+4+1;
				hashed_details = emalloc(hashed_details_length+1); 
				sprintf(hashed_details, "ldap_%s", yyhost->value.str.val);*/ 
			}
			break;

		case 2: {
				pval **yyhost, **yyport;

				if (zend_get_parameters_ex(2, &yyhost,&yyport) == FAILURE) {
					RETURN_FALSE;
				}

				convert_to_string_ex(yyhost);
				host = (*yyhost)->value.str.val;
				convert_to_long_ex(yyport);
				port = (*yyport)->value.lval;

			/* Do we need to take care of hosts running multiple LDAP servers ? */
				/*	hashed_details_length = yyhost->value.str.len+4+1;
				hashed_details = emalloc(hashed_details_length+1);
				sprintf(hashed_details, "ldap_%s", yyhost->value.str.val);*/ 
			}
			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}

	if (LDAPG(max_links)!=-1 && LDAPG(num_links)>=LDAPG(max_links)) {
		php_error(E_WARNING, "LDAP: Too many open links (%d)", LDAPG(num_links));
		RETURN_FALSE;
	}

	ldap = ldap_open(host,port);
	if ( ldap == NULL ) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(zend_list_insert((void*)ldap,le_link));
	}

}
/* }}} */


static LDAP * _get_ldap_link(pval **link)
{
	LDAP *ldap;
	int type;
	LDAPLS_FETCH();

	convert_to_long_ex(link);
	ldap = (LDAP *) zend_list_find((*link)->value.lval, &type);
	
	if (!ldap || !(type == le_link)) {
	  php_error(E_WARNING, "%d is not a LDAP link index",(*link)->value.lval);
	  return NULL;
	}
	return ldap;
}


static LDAPMessage * _get_ldap_result(pval **result)
{
	LDAPMessage *ldap_result;
	int type;
	LDAPLS_FETCH();

	convert_to_long_ex(result);
	ldap_result = (LDAPMessage *)zend_list_find((*result)->value.lval, &type);

	if (!ldap_result || type != le_result) {
		php_error(E_WARNING, "%d is not a LDAP result index",(*result)->value.lval);
		return NULL;
	}

	return ldap_result;
}


static LDAPMessage * _get_ldap_result_entry(pval **result)
{
	LDAPMessage *ldap_result_entry;
	int type;
	LDAPLS_FETCH();

	convert_to_long_ex(result);
	ldap_result_entry = (LDAPMessage *)zend_list_find((*result)->value.lval, &type);

	if (!ldap_result_entry || type != le_result_entry) {
		php_error(E_WARNING, "%d is not a LDAP result entry index", (*result)->value.lval);
		return NULL;
	}

	return ldap_result_entry;
}


static BerElement * _get_ber_entry(pval **berp)
{
	BerElement *ber;
	int type;
	LDAPLS_FETCH();

	convert_to_long_ex(berp);
	ber = (BerElement *) zend_list_find((*berp)->value.lval, &type);

	if ( type != le_ber_entry) {
		php_error(E_WARNING, "%d is not a BerElement index",(*berp)->value.lval);
		return NULL;
	}

	return ber;
}


#if 0
PHP_FUNCTION(ber_free)
{
	pval **berp;
		
	if ( zend_get_parameters_ex(1,&berp) == FAILURE ) {
		WRONG_PARAM_COUNT;
	}
	
	zend_list_delete((*berp)->value.lval);
	RETURN_TRUE;
}
#endif


/* {{{ proto int ldap_bind(int link [, string dn, string password])
   Bind to LDAP directory */
PHP_FUNCTION(ldap_bind)
{
	pval **link, **bind_rdn, **bind_pw;
	char *ldap_bind_rdn, *ldap_bind_pw;
	LDAP *ldap;

	switch(ZEND_NUM_ARGS()) {
		case 1: /* Anonymous Bind */
			if (zend_get_parameters_ex(1, &link) == FAILURE) {
				WRONG_PARAM_COUNT;
			}

			ldap_bind_rdn = NULL;
			ldap_bind_pw = NULL;

			break;

		case 3 :
			if (zend_get_parameters_ex(3, &link, &bind_rdn,&bind_pw) == FAILURE) {
				WRONG_PARAM_COUNT;
			}

			convert_to_string_ex(bind_rdn);
			convert_to_string_ex(bind_pw);

			ldap_bind_rdn = (*bind_rdn)->value.str.val;
			ldap_bind_pw = (*bind_pw)->value.str.val;

			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}	

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	if (ldap_bind_s(ldap, ldap_bind_rdn, ldap_bind_pw, LDAP_AUTH_SIMPLE) != LDAP_SUCCESS) {
#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
		/* New versions of OpenLDAP do it this way */
		php_error(E_WARNING,"LDAP:  Unable to bind to server: %s",ldap_err2string(ldap_get_lderrno(ldap,NULL,NULL)));
#else
		php_error(E_WARNING,"LDAP:  Unable to bind to server: %s",ldap_err2string(ldap->ld_errno));
#endif
#endif
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */


/* {{{ proto int ldap_unbind(int link)
   Unbind from LDAP directory */
PHP_FUNCTION(ldap_unbind)
{
	pval **link;
	LDAP *ldap;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &link) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(link);

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	zend_list_delete((*link)->value.lval);
	RETURN_TRUE;
}
/* }}} */


static void php_ldap_do_search(INTERNAL_FUNCTION_PARAMETERS, int scope)
{
	pval **link, **base_dn, **filter, **attrs, **attr, **attrsonly, **sizelimit, **timelimit, **deref;
	char *ldap_base_dn, *ldap_filter;
	LDAP *ldap;
	char **ldap_attrs = NULL; 
	int ldap_attrsonly = 0;  
	int ldap_sizelimit = -1; 
	int ldap_timelimit = -1; 
	int ldap_deref = -1;	 
	LDAPMessage *ldap_result;
	int num_attribs = 0;
	int i, errno;
	int myargcount = ZEND_NUM_ARGS();
	LDAPLS_FETCH();
  
	if (myargcount < 3 || myargcount > 8 || zend_get_parameters_ex(myargcount, &link, &base_dn, &filter, &attrs, &attrsonly, &sizelimit, &timelimit, &deref) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	/* Reverse -> fall through */
	switch(myargcount) {
		case 8 :
			convert_to_long_ex(deref);
			ldap_deref = (*deref)->value.lval;

		case 7 :
			convert_to_long_ex(timelimit);
			ldap_timelimit = (*timelimit)->value.lval;

		case 6 :
			convert_to_long_ex(sizelimit);
			ldap_sizelimit = (*sizelimit)->value.lval;

		case 5 :
			convert_to_long_ex(attrsonly);
			ldap_attrsonly = (*attrsonly)->value.lval;

		case 4 : 
			if ((*attrs)->type != IS_ARRAY) {
				php_error(E_WARNING, "LDAP: Expected Array as last element");
				RETURN_FALSE;
			}

			num_attribs = zend_hash_num_elements((*attrs)->value.ht);
			if ((ldap_attrs = emalloc((num_attribs+1) * sizeof(char *))) == NULL) {
				php_error(E_WARNING, "LDAP: Could not allocate memory");
				RETURN_FALSE;
			}

			for(i=0; i<num_attribs; i++) {
				if(zend_hash_index_find((*attrs)->value.ht, i, (void **) &attr) == FAILURE) {
					php_error(E_WARNING, "LDAP: Array initialization wrong");
					RETURN_FALSE;
				}

				SEPARATE_ZVAL(attr);
				convert_to_string_ex(attr);
				ldap_attrs[i] = (*attr)->value.str.val;
			}
			ldap_attrs[num_attribs] = NULL;
		
		case 3 :
			convert_to_string_ex(base_dn);
			convert_to_string_ex(filter);
			ldap_base_dn = (*base_dn)->value.str.val;
			ldap_filter = (*filter)->value.str.val;
		break;

		default:
			WRONG_PARAM_COUNT;
		break;
	}

	/* fix to make null base_dn's work */
	if ( strlen(ldap_base_dn) < 1 ) {
	  ldap_base_dn = NULL;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	/* sizelimit */
	if(ldap_sizelimit > -1) {
		ldap->ld_sizelimit = ldap_sizelimit; 
	}

	/* timelimit */
	if(ldap_timelimit > -1) {
		ldap->ld_timelimit = ldap_timelimit; 
	}

	/* deref */
	if(ldap_deref > -1) {
		ldap->ld_deref = ldap_deref; 
	}

	/* Run the actual search */	
	errno = ldap_search_s(ldap, ldap_base_dn, scope, ldap_filter, ldap_attrs, ldap_attrsonly, &ldap_result);

	if (ldap_attrs != NULL) {
		efree(ldap_attrs);
	}

	if (errno != LDAP_SUCCESS && errno != LDAP_SIZELIMIT_EXCEEDED) {
#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
		php_error(E_WARNING,"LDAP: Unable to perform the search: %s",ldap_err2string(ldap_get_lderrno(ldap,NULL,NULL)));
#else
		php_error(E_WARNING, "LDAP: Unable to perform the search: %s", ldap_err2string(ldap->ld_errno));
#endif
#endif
		RETVAL_FALSE; 
	} else {
		if (errno == LDAP_SIZELIMIT_EXCEEDED)  {
			php_error(E_WARNING,"LDAP: Partial search results returned: Sizelimit exceeded.");
		}
		RETVAL_LONG(zend_list_insert(ldap_result, le_result));
	}
}


/* {{{ proto int ldap_read(int link, string base_dn, string filter [, array attrs [, int attrsonly [, int sizelimit [, int timelimit [, int deref]]]]] )
   Read an entry */
PHP_FUNCTION(ldap_read)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_BASE);
}
/* }}} */


/* {{{ proto int ldap_list(int link, string base_dn, string filter [, array attrs [, int attrsonly [, int sizelimit [, int timelimit [, int deref]]]]] )
   Single-level search */
PHP_FUNCTION(ldap_list)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_ONELEVEL);
}
/* }}} */


/* {{{ proto int ldap_search(int link, string base_dn, string filter [, array attrs [, int attrsonly [, int sizelimit [, int timelimit [, int deref]]]]] )
   Search LDAP tree under base_dn */
PHP_FUNCTION(ldap_search)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_SUBTREE);
}
/* }}} */


/* {{{ proto int ldap_free_result(int result)
   Free result memory */
PHP_FUNCTION(ldap_free_result)
{
	pval **result;
	LDAPMessage *ldap_result;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &result) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap_result = _get_ldap_result(result);
	if (ldap_result == NULL) {
		RETVAL_FALSE;
	} else {
		zend_list_delete((*result)->value.lval);  /* Delete list entry and call registered destructor function */
		RETVAL_TRUE;
	}
	return;
}
/* }}} */


/* {{{ proto int ldap_count_entries(int link, int result)
   Count the number of entries in a search result */
PHP_FUNCTION(ldap_count_entries)
{
	pval **result, **link;
	LDAP *ldap;
	LDAPMessage *ldap_result;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &result) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result = _get_ldap_result(result);
	if (ldap_result == NULL) RETURN_FALSE;

	RETURN_LONG(ldap_count_entries(ldap, ldap_result));
}
/* }}} */


/* {{{ proto int ldap_first_entry(int link, int result)
   Return first result id */
PHP_FUNCTION(ldap_first_entry)
{
	pval **result, **link;
	LDAP *ldap;
	LDAPMessage *ldap_result;
	LDAPMessage *ldap_result_entry;
	LDAPLS_FETCH();

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &result) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result = _get_ldap_result(result);
	if (ldap_result == NULL) RETURN_FALSE;

	if ((ldap_result_entry = ldap_first_entry(ldap, ldap_result)) == NULL) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(zend_list_insert(ldap_result_entry, le_result_entry));
	}
}
/* }}} */


/* {{{ proto int ldap_next_entry(int link, int entry)
   Get next result entry */
PHP_FUNCTION(ldap_next_entry)
{
	pval **result_entry, **link;
	LDAP *ldap;
	LDAPMessage *ldap_result_entry, *ldap_result_entry_next;
	LDAPLS_FETCH();

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link,&result_entry) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result_entry = _get_ldap_result_entry(result_entry);
	if (ldap_result_entry == NULL) RETURN_FALSE;

	if ((ldap_result_entry_next = ldap_next_entry(ldap, ldap_result_entry)) == NULL) {
		RETURN_FALSE;
	} else {
		RETURN_LONG(zend_list_insert(ldap_result_entry_next, le_result_entry));
	}
}
/* }}} */


/* {{{ proto array ldap_get_entries(int link, int result)
   Get all result entries */
PHP_FUNCTION(ldap_get_entries)
{
	pval **link, **result;
	LDAPMessage *ldap_result, *ldap_result_entry;
	pval *tmp1, *tmp2;
	LDAP *ldap;
	int num_entries, num_attrib, num_values, i;
	int attr_count, entry_count;
	BerElement *ber;
	char *attribute;
	size_t attr_len;
	char **ldap_value;
	char *dn;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &result) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result = _get_ldap_result(result);
	if (ldap_result == NULL) RETURN_FALSE;

	num_entries = ldap_count_entries(ldap, ldap_result);

	array_init(return_value);
	add_assoc_long(return_value, "count", num_entries);

	if (num_entries == 0) return;
	
	ldap_result_entry = ldap_first_entry(ldap, ldap_result);
	if (ldap_result_entry == NULL) RETURN_FALSE;
	
	entry_count = 0;

	while(ldap_result_entry != NULL) {

		num_attrib = 0;
		attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber);
		if (attribute == NULL) RETURN_FALSE;
		while (attribute != NULL) {
			num_attrib++;
			attribute = ldap_next_attribute(ldap, ldap_result_entry, ber);
		}

		MAKE_STD_ZVAL(tmp1);
		array_init(tmp1);

		attr_count = 0;
		attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber);
		while (attribute != NULL) {
			ldap_value = ldap_get_values(ldap, ldap_result_entry, attribute);
			num_values = ldap_count_values(ldap_value);

			MAKE_STD_ZVAL(tmp2);
			array_init(tmp2);
			add_assoc_long(tmp2, "count", num_values);
			for(i=0; i<num_values; i++) {
				add_index_string(tmp2, i, ldap_value[i], 1);
			}	
			ldap_value_free(ldap_value);

			attr_len = strlen(attribute);
			zend_hash_update(tmp1->value.ht, php_strtolower(attribute, attr_len), attr_len+1, (void *) &tmp2, sizeof(pval *), NULL);
			add_index_string(tmp1, attr_count, attribute, 1);

			attr_count++;
			attribute = ldap_next_attribute(ldap, ldap_result_entry, ber);
		}

		add_assoc_long(tmp1, "count", num_attrib);
		dn = ldap_get_dn(ldap, ldap_result_entry);
		add_assoc_string(tmp1, "dn", dn, 1);

		zend_hash_index_update(return_value->value.ht, entry_count, (void *) &tmp1, sizeof(pval *), NULL);
		
		entry_count++;
		ldap_result_entry = ldap_next_entry(ldap, ldap_result_entry);
	}

	add_assoc_long(return_value, "count", num_entries);
}
/* }}} */


/* {{{ proto string ldap_first_attribute(int link, int result, int ber)
   Return first attribute */
PHP_FUNCTION(ldap_first_attribute)
{
	pval **result,**link,**berp;
	LDAP *ldap;
	LDAPMessage *ldap_result_entry;
	BerElement *ber;
	char *attribute;
	LDAPLS_FETCH();

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &link,&result,&berp) == FAILURE || ParameterPassedByReference(ht,3)==0 ) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result_entry = _get_ldap_result_entry(result);
	if (ldap_result_entry == NULL) RETURN_FALSE;

	if ((attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber)) == NULL) {
		RETURN_FALSE;
	} else {
		/* brep is passed by ref so we do not have to account for memory */
		(*berp)->type=IS_LONG;
		(*berp)->value.lval=zend_list_insert(ber,le_ber_entry);

		RETVAL_STRING(attribute,1);
#ifdef WINDOWS
		ldap_memfree(attribute);
#endif
	}
}
/* }}} */


/* {{{ proto string ldap_next_attribute(int link, int result, int ber)
   Get the next attribute in result */
PHP_FUNCTION(ldap_next_attribute)
{
	pval **result,**link,**berp;
	LDAP *ldap;
	LDAPMessage *ldap_result_entry;
	BerElement *ber;
	char *attribute;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &link,&result,&berp) == FAILURE ) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result_entry = _get_ldap_result_entry(result);
	if (ldap_result_entry == NULL) RETURN_FALSE;

	ber = _get_ber_entry(berp);

	if ((attribute = ldap_next_attribute(ldap, ldap_result_entry, ber)) == NULL) {
		RETURN_FALSE;
	} else {
		RETVAL_STRING(attribute,1);
#ifdef WINDOWS
		ldap_memfree(attribute);
#endif
	}
}
/* }}} */


/* {{{ proto array ldap_get_attributes(int link, int result)
   Get attributes from a search result entry */
PHP_FUNCTION(ldap_get_attributes)
{
	pval **link, **result_entry;
	pval *tmp;
	LDAP *ldap;
	LDAPMessage *ldap_result_entry;
	char *attribute;
	char **ldap_value;
	int i, count, num_values, num_attrib;
	BerElement *ber;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &result_entry) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result_entry = _get_ldap_result_entry(result_entry);
	if (ldap_result_entry == NULL) RETURN_FALSE;

	num_attrib = 0;
	attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber);
	if (attribute == NULL) RETURN_FALSE;
	while (attribute != NULL) {
		num_attrib++;
		attribute = ldap_next_attribute(ldap, ldap_result_entry, ber);
	}
	
	array_init(return_value);

	count=0;
	attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber);
	while (attribute != NULL) {
		ldap_value = ldap_get_values(ldap, ldap_result_entry, attribute);
		num_values = ldap_count_values(ldap_value);

		MAKE_STD_ZVAL(tmp);
		array_init(tmp);
		add_assoc_long(tmp, "count", num_values);
		for(i=0; i<num_values; i++) {
			add_index_string(tmp, i, ldap_value[i], 1);
		}
		ldap_value_free(ldap_value);

		zend_hash_update(return_value->value.ht, attribute, strlen(attribute)+1, (void *) &tmp, sizeof(pval *), NULL);
		add_index_string(return_value, count, attribute, 1);

		count++;
		attribute = ldap_next_attribute(ldap, ldap_result_entry, ber);
	}
	
	add_assoc_long(return_value, "count", num_attrib);
}
/* }}} */


/* {{{ proto array ldap_get_values(int link, int result, string attribute)
   Get all values from a result entry */
PHP_FUNCTION(ldap_get_values)
{
	pval **link, **result_entry, **attr;
	LDAP *ldap;
	LDAPMessage *ldap_result_entry;
	char *attribute;
	char **ldap_value;
	int i, num_values;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &link,&result_entry, &attr) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	ldap_result_entry = _get_ldap_result_entry(result_entry);
	if (ldap_result_entry == NULL) RETURN_FALSE;

	convert_to_string_ex(attr);
	attribute = (*attr)->value.str.val;

	if ((ldap_value = ldap_get_values(ldap, ldap_result_entry, attribute)) == NULL) {
#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
		php_error(E_WARNING, "LDAP: Cannot get the value(s) of attribute %s", ldap_err2string(ldap_get_lderrno(ldap,NULL,NULL)));
#else
		php_error(E_WARNING, "LDAP: Cannot get the value(s) of attribute %s", ldap_err2string(ldap->ld_errno));
#endif
#endif
		RETURN_FALSE;
	}

	num_values = ldap_count_values(ldap_value);

	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

	for(i=0; i<num_values; i++) {
		add_next_index_string(return_value, ldap_value[i], 1);
	}
	
	add_assoc_long(return_value, "count", num_values);

	ldap_value_free(ldap_value);
}
/* }}} */


/* {{{ proto array ldap_get_values_len(int link, int result, string attribute)
   Get the lengths for all values from a result entry */
PHP_FUNCTION(ldap_get_values_len)
{
	pval **link, **result_entry, **attr;
	LDAP* ldap;
	LDAPMessage* ldap_result_entry;
	char* attribute;
	struct berval **ldap_value_len;
	int i, num_values;
	
	if (ZEND_NUM_ARGS() != 3 ||
	    zend_get_parameters_ex(3, &link, &result_entry, &attr) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if ((ldap = _get_ldap_link(link)) == NULL) {
		RETURN_FALSE;
	}
	
	ldap_result_entry = _get_ldap_result_entry(result_entry);
	convert_to_string_ex(attr);
	attribute = (*attr)->value.str.val;
	
	if ((ldap_value_len = ldap_get_values_len(ldap, ldap_result_entry, attribute)) == NULL) {
#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
		php_error(E_WARNING, "LDAP: Cannot get the value(s) of attribute %s", ldap_err2string(ldap_get_lderrno(ldap,NULL,NULL)));
#else
		php_error(E_WARNING, "LDAP: Cannot get the value(s) of attribute %s", ldap_err2string(ldap->ld_errno));
#endif
#else
		php_error(E_WARNING, "LDAP: Cannot get the value(s) of attribute %s", ldap_err2string(ldap_get_lderrno(ldap,NULL,NULL)));
#endif
		RETURN_FALSE;
	}
	
	num_values = ldap_count_values_len(ldap_value_len);
	if (array_init(return_value) == FAILURE) {
		php_error(E_ERROR, "Cannot initialize return value");
		RETURN_FALSE;
	}
	
	for (i=0; i<num_values; i++) {
		add_next_index_string(return_value, ldap_value_len[i]->bv_val, 1);
	}
	
	add_assoc_long(return_value, "count", num_values);
}
/* }}} */
		

/* {{{ proto string ldap_get_dn(int link, int result)
   Get the DN of a result entry */
PHP_FUNCTION(ldap_get_dn) 
{
	pval **link,**entryp;
	LDAP *ld;
	LDAPMessage *entry;
	char *text;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &entryp) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ld = _get_ldap_link(link);
	if (ld == NULL) RETURN_FALSE;

	entry = _get_ldap_result_entry(entryp);
	if (entry == NULL) RETURN_FALSE;

	text = ldap_get_dn(ld, entry);
	if ( text != NULL ) {
		RETVAL_STRING(text,1);
#ifdef WINDOWS
		ldap_memfree(text);
#endif
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto array ldap_explode_dn(string dn, int with_attrib)
   Splits DN into its component parts */
PHP_FUNCTION(ldap_explode_dn)
{
	pval **dn, **with_attrib;
	char **ldap_value;
	int i, count;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &dn,&with_attrib) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(dn);
	convert_to_long_ex(with_attrib);

	ldap_value = ldap_explode_dn((*dn)->value.str.val,(*with_attrib)->value.lval);

	i=0;
	while(ldap_value[i] != NULL) i++;
	count = i;

	if (array_init(return_value) == FAILURE) {
		RETURN_FALSE;
	}

	add_assoc_long(return_value, "count", count);
	for(i=0; i<count; i++) {
		add_index_string(return_value, i, ldap_value[i], 1);
	}

	ldap_value_free(ldap_value);
}
/* }}} */


/* {{{ proto string ldap_dn2ufn(string dn)
   Convert DN to User Friendly Naming format */
PHP_FUNCTION(ldap_dn2ufn)
{
	pval **dn;
	char *ufn;

	if (ZEND_NUM_ARGS() !=1 || zend_get_parameters_ex(1,&dn)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(dn);
	
	ufn = ldap_dn2ufn((*dn)->value.str.val);
	
	if (ufn !=NULL) {
		RETVAL_STRING(ufn,1);
#ifdef WINDOWS
		ldap_memfree(ufn);
#endif
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* added to fix use of ldap_modify_add for doing an ldap_add, gerrit thomson.   */
#define PHP_LD_FULL_ADD 0xff
static void php_ldap_do_modify(INTERNAL_FUNCTION_PARAMETERS, int oper)
{
	pval **link, **dn, **entry, **value, **ivalue;
	LDAP *ldap;
	char *ldap_dn;
	LDAPMod **ldap_mods;
	int i, j, num_attribs, num_values;
	char *attribute;
	ulong index;
	int is_full_add=0; /* flag for full add operation so ldap_mod_add can be put back into oper, gerrit THomson */
 
	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &link, &dn,&entry) == FAILURE) {
		WRONG_PARAM_COUNT;
	}	

	if ((*entry)->type != IS_ARRAY) {
		php_error(E_WARNING, "LDAP: Expected Array as the last element");
		RETURN_FALSE;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	convert_to_string_ex(dn);
	ldap_dn = (*dn)->value.str.val;

	num_attribs = zend_hash_num_elements((*entry)->value.ht);

	ldap_mods = emalloc((num_attribs+1) * sizeof(LDAPMod *));

	zend_hash_internal_pointer_reset((*entry)->value.ht);
        /* added by gerrit thomson to fix ldap_add using ldap_mod_add */
        if ( oper == PHP_LD_FULL_ADD )
        {
                oper = LDAP_MOD_ADD;
                is_full_add = 1;
        }
	/* end additional , gerrit thomson */

	for(i=0; i<num_attribs; i++) {
		ldap_mods[i] = emalloc(sizeof(LDAPMod));

		ldap_mods[i]->mod_op = oper;

		if (zend_hash_get_current_key((*entry)->value.ht,&attribute, &index) == HASH_KEY_IS_STRING) {
			ldap_mods[i]->mod_type = estrdup(attribute);
			efree(attribute);
		} else {
			php_error(E_WARNING, "LDAP: Unknown Attribute in the data");
		}

		zend_hash_get_current_data((*entry)->value.ht, (void **)&value);

		if ((*value)->type != IS_ARRAY) {
			num_values = 1;
		} else {
			num_values = zend_hash_num_elements((*value)->value.ht);
		}

		ldap_mods[i]->mod_values = emalloc((num_values+1) * sizeof(char *));

/* allow for arrays with one element, no allowance for arrays with none but probably not required, gerrit thomson. */
/*              if (num_values == 1) {*/
                if ((num_values == 1) && ((*value)->type != IS_ARRAY)) {
			convert_to_string_ex(value);
			ldap_mods[i]->mod_values[0] = (*value)->value.str.val;
			ldap_mods[i]->mod_values[0][(*value)->value.str.len] = '\0';
		} else {	
			for(j=0; j<num_values; j++) {
				zend_hash_index_find((*value)->value.ht,j, (void **) &ivalue);
				convert_to_string_ex(ivalue);
				ldap_mods[i]->mod_values[j] = (*ivalue)->value.str.val;
				ldap_mods[i]->mod_values[j][(*ivalue)->value.str.len] = '\0';
			}
		}
		ldap_mods[i]->mod_values[num_values] = NULL;

		zend_hash_move_forward((*entry)->value.ht);
	}
	ldap_mods[num_attribs] = NULL;

/* check flag to see if do_mod was called to perform full add , gerrit thomson */
/* 	if (oper == LDAP_MOD_ADD) { */
        if (is_full_add == 1) {
		if (ldap_add_s(ldap, ldap_dn, ldap_mods) != LDAP_SUCCESS) {
			ldap_perror(ldap, "LDAP");
			php_error(E_WARNING, "LDAP: add operation could not be completed.");
			RETVAL_FALSE;
		} else RETVAL_TRUE;
	} else {
		if (ldap_modify_s(ldap, ldap_dn, ldap_mods) != LDAP_SUCCESS) {
			php_error(E_WARNING, "LDAP: modify operation could not be completed.");
			RETVAL_FALSE;
		} else RETVAL_TRUE;	
	}

	for(i=0; i<num_attribs; i++) {
		efree(ldap_mods[i]->mod_type);
		efree(ldap_mods[i]->mod_values);
		efree(ldap_mods[i]);
	}
	efree(ldap_mods);	

	return;
}


/* {{{ proto int ldap_add(int link, string dn, array entry)
   Add entries to LDAP directory */
PHP_FUNCTION(ldap_add)
{
	/* use a newly define parameter into the do_modify so ldap_mod_add can be used the way it is supposed to be used , Gerrit THomson */
	/* php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_ADD);*/
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_LD_FULL_ADD);
}
/* }}} */


/* {{{ proto int ldap_modify(int link, string dn, array entry)
   Modify an LDAP entry */
PHP_FUNCTION(ldap_modify)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_REPLACE); 
}
/* }}} */


/* three functions for attribute base modifications, gerrit Thomson */

/* {{{ proto int ldap_mod_replace(int link, string dn, array entry)
   Replace attribute values with new ones */
PHP_FUNCTION(ldap_mod_replace)
{
        php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_REPLACE);}
/* }}} */


/* {{{ proto int ldap_mod_add(int link, string dn, array entry)
   Add attribute values to current */
PHP_FUNCTION(ldap_mod_add)
{
        php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_ADD);
}
/* }}} */


/* {{{ proto int ldap_mod_del(int link, string dn, array entry)
   Delete attribute values */
PHP_FUNCTION(ldap_mod_del)
{
        php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_DELETE);
}
/* }}} */

/* end of attribute based functions , gerrit thomson */


/* {{{ proto int ldap_delete(int link, string dn)
   Delete an entry from a directory */
PHP_FUNCTION(ldap_delete)
{
	pval **link, **dn;
	LDAP *ldap;
	char *ldap_dn;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &link, &dn) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) RETURN_FALSE;

	convert_to_string_ex(dn);
	ldap_dn = (*dn)->value.str.val;

	if (ldap_delete_s(ldap, ldap_dn) != LDAP_SUCCESS) {
		ldap_perror(ldap, "LDAP");
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int ldap_errno(int link)
   Get the current ldap error number */
PHP_FUNCTION(ldap_errno) {
	LDAP* ldap;
	pval** ldap_link;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ht, &ldap_link) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(ldap_link);

	ldap = _get_ldap_link(ldap_link);
	if (ldap == NULL) {
		RETURN_LONG(0);
	}

#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
	RETURN_LONG( ldap_get_lderrno(ldap, NULL, NULL) );
#else
	RETURN_LONG( ldap->ld_errno );
#endif
#else
	RETURN_LONG( ldap_get_lderrno(ldap, NULL, NULL) );
#endif
}
/* }}} */


/* {{{ proto string ldap_err2str(int errno)
   Convert error number to error string */
PHP_FUNCTION(ldap_err2str) {
	zval** perrno;

	if ( ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ht, &perrno) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(perrno);
	RETURN_STRING(ldap_err2string((*perrno)->value.lval), 1);
}
/* }}} */


/* {{{ proto string ldap_error(int link)
   Get the current ldap error string */
PHP_FUNCTION(ldap_error) {
	LDAP* ldap;
	pval** link;
	int ld_errno;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(ht, &link) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ldap = _get_ldap_link(link);
	if (ldap == NULL) {
		RETURN_FALSE;
	}

#if !HAVE_NSLDAP
#if LDAP_API_VERSION > 2000
	ld_errno = ldap_get_lderrno(ldap, NULL, NULL);
#else
	ld_errno = ldap->ld_errno;
#endif
#else
	ld_errno = ldap_get_lderrno(ldap, NULL, NULL);
#endif

	RETURN_STRING(ldap_err2string(ld_errno), 1);
}
/* }}} */
