/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2003 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"
#include "zend_interfaces.h"

zend_class_entry *zend_ce_traversable;
zend_class_entry *zend_ce_aggregate;
zend_class_entry *zend_ce_iterator;

/* {{{ zend_call_method 
 Only returns the returned zval if retval_ptr != NULL */
ZEND_API zval* zend_call_method(zval **object_pp, zend_class_entry *obj_ce, zend_function **fn_proxy, char *function_name, int function_name_len, zval **retval_ptr_ptr, int param_count, zval* arg1, zval* arg2 TSRMLS_DC)
{
	int result;
	zend_fcall_info fci;
	zval z_fname;
	zval *retval;

	zval **params[2];

	params[0] = &arg1;
	params[1] = &arg2;

	fci.size = sizeof(fci);
	/*fci.function_table = NULL; will be read form zend_class_entry of object if needed */
	fci.object_pp = object_pp;
	fci.function_name = &z_fname;
	fci.retval_ptr_ptr = retval_ptr_ptr ? retval_ptr_ptr : &retval;
	fci.param_count = param_count;
	fci.params = params;
	fci.no_separation = 1;
	fci.symbol_table = NULL;

	if (!fn_proxy && !obj_ce) {
		/* no interest in caching and no information already present that is
		 * needed later inside zend_call_function. */
		ZVAL_STRINGL(&z_fname, function_name, function_name_len, 0);
		result = zend_call_function(&fci, NULL TSRMLS_CC);
	} else {
		zend_fcall_info_cache fcic;

		fcic.initialized = 1;
		if (!obj_ce) {
			obj_ce = Z_OBJCE_PP(object_pp);
		}
		if (!fn_proxy || !*fn_proxy) {
			if (zend_hash_find(&obj_ce->function_table, function_name, function_name_len+1, (void **) &fcic.function_handler) == FAILURE) {
				/* error at c-level */
				zend_error(E_CORE_ERROR, "Couldn't find implementation for method %s::%s", obj_ce->name, function_name);
			}
			if (fn_proxy) {
				*fn_proxy = fcic.function_handler;
			}
		} else {
			fcic.function_handler = *fn_proxy;
		}
		fcic.calling_scope = obj_ce;
		fcic.object_pp = object_pp;
		result = zend_call_function(&fci, &fcic TSRMLS_CC);
	}
	if (result == FAILURE) {
		/* error at c-level */
		if (!obj_ce) {
			obj_ce = Z_OBJCE_PP(object_pp);
		}
		zend_error(E_CORE_ERROR, "Couldn't execute method %s::%s", obj_ce->name, function_name);
	}
	if (!retval_ptr_ptr) {
		if (retval) {
			zval_ptr_dtor(&retval);
		}
		return NULL;
	}
	return *retval_ptr_ptr;
}
/* }}} */

/* iterator interface, c-level functions used by engine */

typedef struct _zend_user_iterator {
	zend_object_iterator     it;
	zend_class_entry         *ce;
	zval                     *value;
} zend_user_iterator;

/* {{{ zend_user_new_iterator */
static zval *zend_user_new_iterator(zend_class_entry *ce, zval *object TSRMLS_DC)
{
	zval *retval;

	return zend_call_method_with_0_params(&object, ce, &ce->iterator_funcs.zf_new_iterator, "getiterator", &retval);

}
/* }}} */

/* {{{ zend_user_dtor */
static void zend_user_dtor(zend_object_iterator *_iter TSRMLS_DC)
{
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	zval *object = (zval*)iter->it.data;

	if (iter->value) {
		zval_ptr_dtor(&iter->value);
		iter->value = NULL;
	}
	zval_ptr_dtor(&object);
	efree(iter);
}
/* }}} */

/* {{{ zend_user_has_more */
static int zend_user_has_more(zend_object_iterator *_iter TSRMLS_DC)
{
	if (_iter) {
		zend_user_iterator *iter = (zend_user_iterator*)_iter;
		zval *object = (zval*)iter->it.data;
		zval *more;
		int result;
	
		zend_call_method_with_0_params(&object, iter->ce, &iter->ce->iterator_funcs.zf_has_more, "hasmore", &more);
		if (more) {
			result = i_zend_is_true(more);
			zval_dtor(more);
			FREE_ZVAL(more);
			return result ? SUCCESS : FAILURE;
		}
	}
	return FAILURE;
}
/* }}} */

/* {{{ zend_user_get_current_data */
static void zend_user_get_current_data(zend_object_iterator *_iter, zval ***data TSRMLS_DC)
{
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	zval *object = (zval*)iter->it.data;

	if (!iter->value) {
		zend_call_method_with_0_params(&object, iter->ce, &iter->ce->iterator_funcs.zf_current, "current", &iter->value);
	}
	*data = &iter->value;
}
/* }}} */

/* {{{ zend_user_get_current_key_default */
#if 0
static int zend_user_get_current_key_default(zend_object_iterator *_iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC)
{
	*int_key = _iter->index;
	return HASH_KEY_IS_LONG;
}
#endif
/* }}} */

/* {{{ zend_user_get_current_key */
static int zend_user_get_current_key(zend_object_iterator *_iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC)
{
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	zval *object = (zval*)iter->it.data;
	zval *retval;

	zend_call_method_with_0_params(&object, iter->ce, &iter->ce->iterator_funcs.zf_key, "key", &retval);

	switch (retval->type) {
		default: 
			zend_error(E_WARNING, "Illegal type returned from %s::key()", iter->ce->name);
		case IS_NULL:
			*str_key = "";
			*str_key_len = 0;
			*int_key = 0;
			zval_ptr_dtor(&retval);
			return HASH_KEY_IS_LONG;

		case IS_STRING:
			*str_key = estrndup(retval->value.str.val, retval->value.str.len);
			*str_key_len = retval->value.str.len+1;
			*int_key = 0;
			zval_ptr_dtor(&retval);
			return HASH_KEY_IS_STRING;

		case IS_DOUBLE:
		case IS_RESOURCE:
		case IS_BOOL: 
		case IS_LONG: {
				if (retval->type == IS_DOUBLE) {
					*int_key = (long)retval->value.dval;
				} else {
					*int_key = retval->value.lval;
				}
			}
			zval_ptr_dtor(&retval);
			return HASH_KEY_IS_LONG;
	}
}
/* }}} */

/* {{{ zend_user_move_forward */
static void zend_user_move_forward(zend_object_iterator *_iter TSRMLS_DC)
{
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	zval *object = (zval*)iter->it.data;

	if (iter->value) {
		zval_ptr_dtor(&iter->value);
		iter->value = NULL;
	}
	zend_call_method_with_0_params(&object, iter->ce, &iter->ce->iterator_funcs.zf_next, "next", NULL);
}
/* }}} */

/* {{{ zend_user_rewind */
static void zend_user_rewind(zend_object_iterator *_iter TSRMLS_DC)
{
	zend_user_iterator *iter = (zend_user_iterator*)_iter;
	zval *object = (zval*)iter->it.data;

	zend_call_method_with_0_params(&object, iter->ce, &iter->ce->iterator_funcs.zf_rewind, "rewind", NULL);
}
/* }}} */

zend_object_iterator_funcs zend_interface_iterator_funcs_iterator = {
	zend_user_dtor,
	zend_user_has_more,
	zend_user_get_current_data,
	zend_user_get_current_key,
	zend_user_move_forward,
	zend_user_rewind
};

/* {{{ zend_user_get_iterator */
static zend_object_iterator *zend_user_get_iterator(zend_class_entry *ce, zval *object TSRMLS_DC)
{
	zend_user_iterator *iterator = emalloc(sizeof(zend_user_iterator));

	object->refcount++;
	iterator->it.data = (void*)object;
	iterator->it.funcs = ce->iterator_funcs.funcs;
	iterator->ce = Z_OBJCE_P(object);
	iterator->value = NULL;
	return (zend_object_iterator*)iterator;
}
/* }}} */

/* {{{ zend_user_get_new_iterator */
static zend_object_iterator *zend_user_get_new_iterator(zend_class_entry *ce, zval *object TSRMLS_DC)
{
	zval *iterator = zend_user_new_iterator(ce, object TSRMLS_CC);

	zend_class_entry *ce_it = Z_TYPE_P(iterator) == IS_OBJECT ? Z_OBJCE_P(iterator) : NULL;

	if (!ce || !ce_it || !ce_it->get_iterator) {
		zend_error(E_WARNING, "Objects returned by %s::getIterator() must be traversable or implement interface Iterator", ce->name);
		zval_ptr_dtor(&iterator);
		return NULL;
	}
	iterator->refcount--; /* from return */
	return ce_it->get_iterator(ce_it, iterator TSRMLS_CC);
}
/* }}} */

/* {{{ zend_implement_traversable */
static int zend_implement_traversable(zend_class_entry *interface, zend_class_entry *class_type TSRMLS_DC)
{
	/* check that class_type is traversable at c-level or implements at least one of 'aggregate' and 'Iterator' */
	int i;

	if (class_type->get_iterator) {
		return SUCCESS;
	}	
	for (i = 0; i < class_type->num_interfaces; i++) {
		if (class_type->interfaces[i] == zend_ce_aggregate || class_type->interfaces[i] == zend_ce_iterator) {
			return SUCCESS;
		}
	}
	zend_error(E_CORE_ERROR, "Class %s must implement interface %s as part of either %s or %s",
		class_type->name,
		zend_ce_traversable->name,
		zend_ce_iterator->name,
		zend_ce_aggregate->name);
	return FAILURE;
}
/* }}} */

/* {{{ zend_implement_aggregate */
static int zend_implement_aggregate(zend_class_entry *interface, zend_class_entry *class_type TSRMLS_DC)
{
	if (class_type->get_iterator) {
		if (class_type->type == ZEND_INTERNAL_CLASS) {
			/* inheritance ensures the class has necessary userland methods */
			return SUCCESS;
		} else if (class_type->get_iterator != zend_user_get_new_iterator) {
			/* c-level get_iterator cannot be changed */
			return FAILURE;
		}
	}
	class_type->iterator_funcs.zf_new_iterator = NULL;
	class_type->get_iterator = zend_user_get_new_iterator;
	return SUCCESS;
}
/* }}} */

/* {{{ zend_implement_iterator */
static int zend_implement_iterator(zend_class_entry *interface, zend_class_entry *class_type TSRMLS_DC)
{
	if (class_type->get_iterator && class_type->get_iterator != zend_user_get_iterator) {
		if (class_type->type == ZEND_INTERNAL_CLASS) {
			/* inheritance ensures the class has the necessary userland methods */
			return SUCCESS;
		} else if (class_type->get_iterator != zend_user_get_new_iterator) {
			/* c-level get_iterator cannot be changed */
			return FAILURE;
		}
	}
	class_type->get_iterator = zend_user_get_iterator;
	class_type->iterator_funcs.zf_has_more = NULL;
	class_type->iterator_funcs.zf_current = NULL;
	class_type->iterator_funcs.zf_key = NULL;
	class_type->iterator_funcs.zf_next = NULL;
	class_type->iterator_funcs.zf_rewind = NULL;
	if (!class_type->iterator_funcs.funcs) {
		class_type->iterator_funcs.funcs = &zend_interface_iterator_funcs_iterator;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ function tables */
zend_function_entry zend_funcs_aggregate[] = {
	ZEND_ABSTRACT_ME(iterator, getIterator, NULL)
	{NULL, NULL, NULL}
};

zend_function_entry zend_funcs_iterator[] = {
	ZEND_ABSTRACT_ME(iterator, current,  NULL)
	ZEND_ABSTRACT_ME(iterator, next,     NULL)
	ZEND_ABSTRACT_ME(iterator, key,      NULL)
	ZEND_ABSTRACT_ME(iterator, hasMore,  NULL)
	ZEND_ABSTRACT_ME(iterator, rewind,   NULL)
	{NULL, NULL, NULL}
};

zend_function_entry *zend_funcs_traversable    = NULL;
/* }}} */

#define REGISTER_ITERATOR_INTERFACE(class_name, class_name_str) \
	{\
		zend_class_entry ce;\
		INIT_CLASS_ENTRY(ce, # class_name_str, zend_funcs_ ## class_name) \
		zend_ce_ ## class_name = zend_register_internal_interface(&ce TSRMLS_CC);\
		zend_ce_ ## class_name->interface_gets_implemented = zend_implement_ ## class_name;\
	}

#define REGISTER_ITERATOR_IMPLEMENT(class_name, interface_name) \
	zend_class_implements(zend_ce_ ## class_name TSRMLS_CC, 1, zend_ce_ ## interface_name)

/* {{{ zend_register_interfaces */
ZEND_API void zend_register_interfaces(TSRMLS_D)
{
	REGISTER_ITERATOR_INTERFACE(traversable, Traversable);

	REGISTER_ITERATOR_INTERFACE(aggregate, IteratorAggregate);
	REGISTER_ITERATOR_IMPLEMENT(aggregate, traversable);

	REGISTER_ITERATOR_INTERFACE(iterator, Iterator);
	REGISTER_ITERATOR_IMPLEMENT(iterator, traversable);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
