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
   | Authors: Christian Stocker <chregu@php.net>                          |
   |          Rob Richards <rrichards@php.net>                            |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#if HAVE_LIBXML && HAVE_DOM
#include "php_dom.h"


/*
* class DOMEntity extends DOMNode 
*
* URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-527DCFF2
* Since: 
*/

zend_function_entry php_dom_entity_class_functions[] = {
	{NULL, NULL, NULL}
};

/* {{{ proto publicId	string	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-D7303025
Since: 
*/
int dom_entity_public_id_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlEntity *nodep;

	nodep = (xmlEntity *) dom_object_get_node(obj);

	if (nodep == NULL) {
		php_dom_throw_error(INVALID_STATE_ERR, 0 TSRMLS_CC);
		return FAILURE;
	}

	ALLOC_ZVAL(*retval);
	if (nodep->etype != XML_EXTERNAL_GENERAL_UNPARSED_ENTITY) {
		ZVAL_NULL(*retval);
	} else {
		ZVAL_STRING(*retval, (char *) (nodep->ExternalID), 1);
	}

	return SUCCESS;
}

/* }}} */



/* {{{ proto systemId	string	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-D7C29F3E
Since: 
*/
int dom_entity_system_id_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlEntity *nodep;

	nodep = (xmlEntity *) dom_object_get_node(obj);

	if (nodep == NULL) {
		php_dom_throw_error(INVALID_STATE_ERR, 0 TSRMLS_CC);
		return FAILURE;
	}

	ALLOC_ZVAL(*retval);
	if (nodep->etype != XML_EXTERNAL_GENERAL_UNPARSED_ENTITY) {
		ZVAL_NULL(*retval);
	} else {
		ZVAL_STRING(*retval, (char *) (nodep->SystemID), 1);
	}

	return SUCCESS;
}

/* }}} */



/* {{{ proto notationName	string	
readonly=yes 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#ID-6ABAEB38
Since: 
*/
int dom_entity_notation_name_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	xmlEntity *nodep;
	char *content;

	nodep = (xmlEntity *) dom_object_get_node(obj);

	if (nodep == NULL) {
		php_dom_throw_error(INVALID_STATE_ERR, 0 TSRMLS_CC);
		return FAILURE;
	}

	ALLOC_ZVAL(*retval);
	if (nodep->etype != XML_EXTERNAL_GENERAL_UNPARSED_ENTITY) {
		ZVAL_NULL(*retval);
	} else {
		content = xmlNodeGetContent((xmlNodePtr) nodep);
		ZVAL_STRING(*retval, content, 1);
		xmlFree(content);
	}

	return SUCCESS;
}

/* }}} */



/* {{{ proto actualEncoding	string	
readonly=no 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#Entity3-actualEncoding
Since: DOM Level 3
*/
int dom_entity_actual_encoding_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_NULL(*retval);
	return SUCCESS;
}

int dom_entity_actual_encoding_write(dom_object *obj, zval *newval TSRMLS_DC)
{
	return SUCCESS;
}

/* }}} */



/* {{{ proto encoding	string	
readonly=no 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#Entity3-encoding
Since: DOM Level 3
*/
int dom_entity_encoding_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_NULL(*retval);
	return SUCCESS;
}

int dom_entity_encoding_write(dom_object *obj, zval *newval TSRMLS_DC)
{
	return SUCCESS;
}

/* }}} */



/* {{{ proto version	string	
readonly=no 
URL: http://www.w3.org/TR/2003/WD-DOM-Level-3-Core-20030226/DOM3-Core.html#Entity3-version
Since: DOM Level 3
*/
int dom_entity_version_read(dom_object *obj, zval **retval TSRMLS_DC)
{
	ALLOC_ZVAL(*retval);
	ZVAL_NULL(*retval);
	return SUCCESS;
}

int dom_entity_version_write(dom_object *obj, zval *newval TSRMLS_DC)
{
	return SUCCESS;
}

/* }}} */

#endif
