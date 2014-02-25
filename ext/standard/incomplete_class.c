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
   | Author:  Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "basic_functions.h"
#include "php_incomplete_class.h"

#define INCOMPLETE_CLASS_MSG \
		"The script tried to execute a method or "  \
		"access a property of an incomplete object. " \
		"Please ensure that the class definition \"%s\" of the object " \
		"you are trying to operate on was loaded _before_ " \
		"unserialize() gets called or provide a __autoload() function " \
		"to load the class definition "

static zend_object_handlers php_incomplete_object_handlers;

/* {{{ incomplete_class_message
 */
static void incomplete_class_message(zval *object, int error_type TSRMLS_DC)
{
	zend_string *class_name;

	class_name = php_lookup_class_name(object);

	if (class_name) {
		php_error_docref(NULL TSRMLS_CC, error_type, INCOMPLETE_CLASS_MSG, class_name->val);
		STR_RELEASE(class_name);
	} else {
		php_error_docref(NULL TSRMLS_CC, error_type, INCOMPLETE_CLASS_MSG, "unknown");
	}
}
/* }}} */

static zval *incomplete_class_get_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_NOTICE TSRMLS_CC);

	if (type == BP_VAR_W || type == BP_VAR_RW) {
		return &EG(error_zval);
	} else {
		return &EG(uninitialized_zval);
	}
}
/* }}} */

static void incomplete_class_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_NOTICE TSRMLS_CC);
}
/* }}} */

static zval *incomplete_class_get_property_ptr_ptr(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_NOTICE TSRMLS_CC);
	return &EG(error_zval);
}
/* }}} */

static void incomplete_class_unset_property(zval *object, zval *member, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_NOTICE TSRMLS_CC);
}
/* }}} */

static int incomplete_class_has_property(zval *object, zval *member, int check_empty, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_NOTICE TSRMLS_CC);
	return 0;
}
/* }}} */

static union _zend_function *incomplete_class_get_method(zval *object, zend_string *method, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	incomplete_class_message(object, E_ERROR TSRMLS_CC);
	return NULL;
}
/* }}} */

/* {{{ php_create_incomplete_class
 */
static zend_object *php_create_incomplete_object(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object *object;

	object = zend_objects_new( class_type TSRMLS_CC);
	object->handlers = &php_incomplete_object_handlers;

	object_properties_init(object, class_type);

	return object;
}

PHPAPI zend_class_entry *php_create_incomplete_class(TSRMLS_D)
{
	zend_class_entry incomplete_class;

	INIT_CLASS_ENTRY(incomplete_class, INCOMPLETE_CLASS, NULL);
	incomplete_class.create_object = php_create_incomplete_object;

	memcpy(&php_incomplete_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	php_incomplete_object_handlers.read_property = incomplete_class_get_property;
	php_incomplete_object_handlers.has_property = incomplete_class_has_property;
	php_incomplete_object_handlers.unset_property = incomplete_class_unset_property;
	php_incomplete_object_handlers.write_property = incomplete_class_write_property;
	php_incomplete_object_handlers.get_property_ptr_ptr = incomplete_class_get_property_ptr_ptr;
    php_incomplete_object_handlers.get_method = incomplete_class_get_method;

	return zend_register_internal_class(&incomplete_class TSRMLS_CC);
}
/* }}} */

/* {{{ php_lookup_class_name
 */
PHPAPI zend_string *php_lookup_class_name(zval *object)
{
	zval *val;
	HashTable *object_properties;
	TSRMLS_FETCH();

	object_properties = Z_OBJPROP_P(object);

	if ((val = zend_hash_str_find(object_properties, MAGIC_MEMBER, sizeof(MAGIC_MEMBER)-1)) != NULL) {
		return STR_COPY(Z_STR_P(val));
	}

	return NULL;
}
/* }}} */

/* {{{ php_store_class_name
 */
PHPAPI void php_store_class_name(zval *object, const char *name, zend_uint len)
{
	zval val;
	TSRMLS_FETCH();


	ZVAL_STRINGL(&val, name, len);
	zend_hash_str_update(Z_OBJPROP_P(object), MAGIC_MEMBER, sizeof(MAGIC_MEMBER)-1, &val);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
