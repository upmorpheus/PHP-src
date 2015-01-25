/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Christian Stocker <chregu@php.net>                          |
  |          Rob Richards <rrichards@php.net>                            |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_XML_COMMON_H
#define PHP_XML_COMMON_H

#include "ext/libxml/php_libxml.h"

typedef libxml_doc_props *dom_doc_propsptr;

typedef struct _dom_object {
	void *ptr;
	php_libxml_ref_obj *document;
	HashTable *prop_handler;
	zend_object std;
} dom_object;

static inline dom_object *php_dom_obj_from_obj(zend_object *obj) {
	return (dom_object*)((char*)(obj) - XtOffsetOf(dom_object, std));
}

#define Z_DOMOBJ_P(zv)  php_dom_obj_from_obj(Z_OBJ_P((zv)))

#ifdef PHP_WIN32
#	ifdef PHPAPI
#		undef PHPAPI
#	endif
#	ifdef DOM_EXPORTS
#		define PHPAPI __declspec(dllexport)
#	else
#		define PHPAPI __declspec(dllimport)
#	endif /* DOM_EXPORTS */
#elif defined(__GNUC__) && __GNUC__ >= 4
#	ifdef PHPAPI
#		undef PHPAPI
#	endif
#	define PHPAPI __attribute__ ((visibility("default")))
#endif /* PHP_WIN32 */

#define PHP_DOM_EXPORT PHPAPI

PHP_DOM_EXPORT extern zend_class_entry *dom_node_class_entry;
PHP_DOM_EXPORT dom_object *php_dom_object_get_data(xmlNodePtr obj);
PHP_DOM_EXPORT zend_bool php_dom_create_object(xmlNodePtr obj, zval* return_value, dom_object *domobj);
PHP_DOM_EXPORT xmlNodePtr dom_object_get_node(dom_object *obj);

#define DOM_XMLNS_NAMESPACE \
    (const xmlChar *) "http://www.w3.org/2000/xmlns/"

#define NODE_GET_OBJ(__ptr, __id, __prtype, __intern) { \
	__intern = Z_LIBXML_NODE_P(__id); \
	if (__intern->node == NULL || !(__ptr = (__prtype)__intern->node->node)) { \
  		php_error_docref(NULL, E_WARNING, "Couldn't fetch %s", \
			__intern->std.ce->name->val);\
  		RETURN_NULL();\
  	} \
}

#define DOC_GET_OBJ(__ptr, __id, __prtype, __intern) { \
	__intern = Z_LIBXML_NODE_P(__id); \
	if (__intern->document != NULL) { \
		if (!(__ptr = (__prtype)__intern->document->ptr)) { \
  			php_error_docref(NULL, E_WARNING, "Couldn't fetch %s", __intern->std.ce->name);\
  			RETURN_NULL();\
  		} \
	} \
}

#define DOM_RET_OBJ(obj, ret, domobject) \
	*ret = php_dom_create_object(obj, return_value, domobject)

#define DOM_GET_THIS(zval) \
	if (NULL == (zval = getThis())) { \
		php_error_docref(NULL, E_WARNING, "Underlying object missing"); \
		RETURN_FALSE; \
	}

#define DOM_GET_THIS_OBJ(__ptr, __id, __prtype, __intern) \
	DOM_GET_THIS(__id); \
	DOM_GET_OBJ(__ptr, __id, __prtype, __intern);

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
