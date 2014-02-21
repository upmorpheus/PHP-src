/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_globals.h"
#include "zend_variables.h"
#include "zend_API.h"
#include "zend_objects.h"
#include "zend_objects_API.h"
#include "zend_object_handlers.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_compile.h"
#include "zend_hash.h"

#define DEBUG_OBJECT_HANDLERS 0

/* guard flags */
#define IN_GET		(1<<0)
#define IN_SET		(1<<1)
#define IN_UNSET	(1<<2)
#define IN_ISSET	(1<<3)

#define Z_OBJ_PROTECT_RECURSION(zval_p) \
	do { \
		if (Z_OBJ_APPLY_COUNT_P(zval_p) >= 3) { \
			zend_error(E_ERROR, "Nesting level too deep - recursive dependency?"); \
		} \
		Z_OBJ_INC_APPLY_COUNT_P(zval_p); \
	} while (0)


#define Z_OBJ_UNPROTECT_RECURSION(zval_p) \
	Z_OBJ_DEC_APPLY_COUNT_P(zval_p)

/*
  __X accessors explanation:

  if we have __get and property that is not part of the properties array is
  requested, we call __get handler. If it fails, we return uninitialized.

  if we have __set and property that is not part of the properties array is
  set, we call __set handler. If it fails, we do not change the array.

  for both handlers above, when we are inside __get/__set, no further calls for
  __get/__set for this property of this object will be made, to prevent endless
  recursion and enable accessors to change properties array.

  if we have __call and method which is not part of the class function table is
  called, we cal __call handler.
*/

ZEND_API void rebuild_object_properties(zend_object *zobj) /* {{{ */
{
	if (!zobj->properties) {
		HashPosition pos;
		zend_property_info *prop_info;
		zend_class_entry *ce = zobj->ce;

		ALLOC_HASHTABLE(zobj->properties);
		zend_hash_init(zobj->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
		if (ce->default_properties_count) {
			for (zend_hash_internal_pointer_reset_ex(&ce->properties_info, &pos);
			     (prop_info = zend_hash_get_current_data_ptr_ex(&ce->properties_info, &pos)) != NULL;
			     zend_hash_move_forward_ex(&ce->properties_info, &pos)) {
				if (/*prop_info->ce == ce &&*/
				    (prop_info->flags & ZEND_ACC_STATIC) == 0 &&
				    prop_info->offset >= 0 &&
				    Z_TYPE(zobj->properties_table[prop_info->offset]) != IS_UNDEF) {
					zval *zv = zend_hash_add(zobj->properties, prop_info->name, &zobj->properties_table[prop_info->offset]);
					ZVAL_INDIRECT(&zobj->properties_table[prop_info->offset], zv);
				}
			}
			while (ce->parent && ce->parent->default_properties_count) {
				ce = ce->parent;
				for (zend_hash_internal_pointer_reset_ex(&ce->properties_info, &pos);
				     (prop_info = zend_hash_get_current_data_ptr_ex(&ce->properties_info, &pos)) != NULL;
				     zend_hash_move_forward_ex(&ce->properties_info, &pos)) {
					if (prop_info->ce == ce &&
					    (prop_info->flags & ZEND_ACC_STATIC) == 0 &&
					    (prop_info->flags & ZEND_ACC_PRIVATE) != 0 &&
					    prop_info->offset >= 0 &&
						Z_TYPE(zobj->properties_table[prop_info->offset]) != IS_UNDEF) {
						zval *zv = zend_hash_add(zobj->properties, prop_info->name, &zobj->properties_table[prop_info->offset]);
						ZVAL_INDIRECT(&zobj->properties_table[prop_info->offset], zv);
					}
				}
			}
		}
	}
}
/* }}} */

ZEND_API HashTable *zend_std_get_properties(zval *object TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zobj = Z_OBJ_P(object);
	if (!zobj->properties) {
		rebuild_object_properties(zobj);
	}
	return zobj->properties;
}
/* }}} */

ZEND_API HashTable *zend_std_get_gc(zval *object, zval **table, int *n TSRMLS_DC) /* {{{ */
{
	if (Z_OBJ_HANDLER_P(object, get_properties) != zend_std_get_properties) {
		*table = NULL;
		*n = 0;
		return Z_OBJ_HANDLER_P(object, get_properties)(object TSRMLS_CC);
	} else {
		zend_object *zobj = Z_OBJ_P(object);

		if (zobj->properties) {
			*table = NULL;
			*n = 0;
			return zobj->properties;
		} else {
			*table = zobj->properties_table;
			*n = zobj->ce->default_properties_count;
			return NULL;
		}
	}
}
/* }}} */

ZEND_API HashTable *zend_std_get_debug_info(zval *object, int *is_temp TSRMLS_DC) /* {{{ */
{
	*is_temp = 0;
	return zend_std_get_properties(object TSRMLS_CC);
}
/* }}} */

static void zend_std_call_getter(zval *object, zval *member, zval *retval TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);

	/* __get handler is called with one argument:
	      property name

	   it should return whether the call was successfull or not
	*/

	SEPARATE_ARG_IF_REF(member);

	ZVAL_UNDEF(retval);
	zend_call_method_with_1_params(object, ce, &ce->__get, ZEND_GET_FUNC_NAME, retval, member);

	zval_ptr_dtor(member);

	if (Z_REFCOUNTED_P(retval)) {
		Z_DELREF_P(retval);
	}
}
/* }}} */

static int zend_std_call_setter(zval *object, zval *member, zval *value TSRMLS_DC) /* {{{ */
{
	zval retval;
	int result;
	zend_class_entry *ce = Z_OBJCE_P(object);

	SEPARATE_ARG_IF_REF(member);
	if (Z_REFCOUNTED_P(value)) Z_ADDREF_P(value);

	/* __set handler is called with two arguments:
	     property name
	     value to be set

	   it should return whether the call was successfull or not
	*/
	ZVAL_UNDEF(&retval);
	zend_call_method_with_2_params(object, ce, &ce->__set, ZEND_SET_FUNC_NAME, &retval, member, value);

	zval_ptr_dtor(member);
	zval_ptr_dtor(value);

	if (Z_TYPE(retval) != IS_UNDEF) {
		result = i_zend_is_true(&retval TSRMLS_CC) ? SUCCESS : FAILURE;
		zval_ptr_dtor(&retval);
		return result;
	} else {
		return FAILURE;
	}
}
/* }}} */

static void zend_std_call_unsetter(zval *object, zval *member TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);

	/* __unset handler is called with one argument:
	      property name
	*/

	SEPARATE_ARG_IF_REF(member);

	zend_call_method_with_1_params(object, ce, &ce->__unset, ZEND_UNSET_FUNC_NAME, NULL, member);

	zval_ptr_dtor(member);
}
/* }}} */

static void zend_std_call_issetter(zval *object, zval *member, zval *retval TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);

	/* __isset handler is called with one argument:
	      property name

	   it should return whether the property is set or not
	*/

	SEPARATE_ARG_IF_REF(member);

	ZVAL_UNDEF(retval);
	zend_call_method_with_1_params(object, ce, &ce->__isset, ZEND_ISSET_FUNC_NAME, retval, member);

	zval_ptr_dtor(member);
}
/* }}} */

static zend_always_inline int zend_verify_property_access(zend_property_info *property_info, zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	switch (property_info->flags & ZEND_ACC_PPP_MASK) {
		case ZEND_ACC_PUBLIC:
			return 1;
		case ZEND_ACC_PROTECTED:
			return zend_check_protected(property_info->ce, EG(scope));
		case ZEND_ACC_PRIVATE:
			if ((ce==EG(scope) || property_info->ce == EG(scope)) && EG(scope)) {
				return 1;
			} else {
				return 0;
			}
			break;
	}
	return 0;
}
/* }}} */

static zend_always_inline zend_bool is_derived_class(zend_class_entry *child_class, zend_class_entry *parent_class) /* {{{ */
{
	child_class = child_class->parent;
	while (child_class) {
		if (child_class == parent_class) {
			return 1;
		}
		child_class = child_class->parent;
	}

	return 0;
}
/* }}} */

static zend_always_inline struct _zend_property_info *zend_get_property_info_quick(zend_class_entry *ce, zval *member, int silent, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_property_info *property_info;
	zend_property_info *scope_property_info;
	zend_bool denied_access = 0;

	if (key && (property_info = CACHED_POLYMORPHIC_PTR(key->cache_slot, ce)) != NULL) {
		return property_info;
	}

	if (UNEXPECTED(Z_STRVAL_P(member)[0] == '\0')) {
		if (!silent) {
			if (Z_STRLEN_P(member) == 0) {
				zend_error_noreturn(E_ERROR, "Cannot access empty property");
			} else {
				zend_error_noreturn(E_ERROR, "Cannot access property started with '\\0'");
			}
		}
		return NULL;
	}
	property_info = NULL;
	if ((property_info = zend_hash_find_ptr(&ce->properties_info, Z_STR_P(member))) != NULL) {
		if (UNEXPECTED((property_info->flags & ZEND_ACC_SHADOW) != 0)) {
			/* if it's a shadow - go to access it's private */
			property_info = NULL;
		} else {
			if (EXPECTED(zend_verify_property_access(property_info, ce TSRMLS_CC) != 0)) {
				if (EXPECTED((property_info->flags & ZEND_ACC_CHANGED) != 0)
					&& EXPECTED(!(property_info->flags & ZEND_ACC_PRIVATE))) {
					/* We still need to make sure that we're not in a context
					 * where the right property is a different 'statically linked' private
					 * continue checking below...
					 */
				} else {
					if (UNEXPECTED((property_info->flags & ZEND_ACC_STATIC) != 0) && !silent) {
						zend_error(E_STRICT, "Accessing static property %s::$%s as non static", ce->name->val, Z_STRVAL_P(member));
					}
					if (key) {
						CACHE_POLYMORPHIC_PTR(key->cache_slot, ce, property_info);
					}
					return property_info;
				}
			} else {
				/* Try to look in the scope instead */
				denied_access = 1;
			}
		}
	}
	if (EG(scope) != ce
		&& EG(scope)
		&& is_derived_class(ce, EG(scope))
		&& (scope_property_info = zend_hash_find_ptr(&EG(scope)->properties_info, Z_STR_P(member))) != NULL
		&& scope_property_info->flags & ZEND_ACC_PRIVATE) {
		if (key) {
			CACHE_POLYMORPHIC_PTR(key->cache_slot, ce, scope_property_info);
		}
		return scope_property_info;
	} else if (property_info) {
		if (UNEXPECTED(denied_access != 0)) {
			/* Information was available, but we were denied access.  Error out. */
			if (!silent) {
				zend_error_noreturn(E_ERROR, "Cannot access %s property %s::$%s", zend_visibility_string(property_info->flags), ce->name, Z_STRVAL_P(member));
			}
			return NULL;
		} else {
			/* fall through, return property_info... */
			if (key) {
				CACHE_POLYMORPHIC_PTR(key->cache_slot, ce, property_info);
			}
		}
	} else {
		EG(std_property_info).flags = ZEND_ACC_PUBLIC;
		EG(std_property_info).name = Z_STR_P(member);
		EG(std_property_info).ce = ce;
		EG(std_property_info).offset = -1;
		property_info = &EG(std_property_info);
	}
	return property_info;
}
/* }}} */

ZEND_API struct _zend_property_info *zend_get_property_info(zend_class_entry *ce, zval *member, int silent TSRMLS_DC) /* {{{ */
{
	return zend_get_property_info_quick(ce, member, silent, NULL TSRMLS_CC);
}
/* }}} */

ZEND_API int zend_check_property_access(zend_object *zobj, zend_string *prop_info_name TSRMLS_DC) /* {{{ */
{
	zend_property_info *property_info;
	const char *class_name, *prop_name;
	zval member;
	int prop_name_len;

	zend_unmangle_property_name_ex(prop_info_name->val, prop_info_name->len, &class_name, &prop_name, &prop_name_len);
	ZVAL_STRINGL(&member, prop_name, prop_name_len);
	property_info = zend_get_property_info_quick(zobj->ce, &member, 1, NULL TSRMLS_CC);
	if (!property_info) {
		return FAILURE;
	}
	if (class_name && class_name[0] != '*') {
		if (!(property_info->flags & ZEND_ACC_PRIVATE)) {
			/* we we're looking for a private prop but found a non private one of the same name */
			return FAILURE;
		} else if (strcmp(prop_info_name->val+1, property_info->name->val+1)) {
			/* we we're looking for a private prop but found a private one of the same name but another class */
			return FAILURE;
		}
	}
	return zend_verify_property_access(property_info, zobj->ce TSRMLS_CC) ? SUCCESS : FAILURE;
}
/* }}} */

static long *zend_get_property_guard(zend_object *zobj, zend_property_info *property_info, zval *member) /* {{{ */
{
	zend_property_info info;
	zval stub, *guard;

	if (!property_info) {
		property_info = &info;
		info.name = Z_STR_P(member);
	} else if(property_info->name->val[0] == '\0'){
		const char *class_name = NULL, *prop_name = NULL;
		zend_unmangle_property_name(property_info->name->val, property_info->name->len, &class_name, &prop_name);
		if (class_name) {
			/* use unmangled name for protected properties */
			info.name = STR_INIT(prop_name, strlen(prop_name), 0);
			property_info = &info;
		}
	}
	if (!zobj->guards) {
		ALLOC_HASHTABLE(zobj->guards);
		zend_hash_init(zobj->guards, 0, NULL, NULL, 0);
	} else if ((guard = zend_hash_find(zobj->guards, property_info->name)) != NULL) {
		return &Z_LVAL_P(guard);
	}

	ZVAL_LONG(&stub, 0);
	guard = zend_hash_add(zobj->guards, property_info->name, &stub);
	return &Z_LVAL_P(guard);
}
/* }}} */

zval *zend_std_read_property(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zval tmp_member;
	zval *retval;
	zval rv;
	zend_property_info *property_info;
	int silent;

	silent = (type == BP_VAR_IS);
	zobj = Z_OBJ_P(object);

	ZVAL_UNDEF(&tmp_member);
	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		ZVAL_DUP(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Read object #%d property: %s\n", Z_OBJ_HANDLE_P(object), Z_STRVAL_P(member));
#endif

	/* make zend_get_property_info silent if we have getter - we may want to use it */
	property_info = zend_get_property_info_quick(zobj->ce, member, silent || (zobj->ce->__get != NULL), key TSRMLS_CC);

	if (EXPECTED(property_info != NULL)) {
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0 &&
		    Z_TYPE(zobj->properties_table[property_info->offset]) != IS_UNDEF) {
			retval = &zobj->properties_table[property_info->offset];
			if (Z_TYPE_P(retval) == IS_INDIRECT) {
				retval = Z_INDIRECT_P(retval);
			}
			goto exit;
		}
		if (UNEXPECTED(!zobj->properties)) {
			retval = zend_hash_find(zobj->properties, property_info->name);
			if (retval) goto exit;
		}
	}

	if (zobj->ce->__get) {
		long *guard = zend_get_property_guard(zobj, property_info, member);
		if (!((*guard) & IN_GET)) {
			/* have getter - try with it! */
			Z_ADDREF_P(object);
			if (Z_ISREF_P(object)) {
				SEPARATE_ZVAL(object);
			}
			*guard |= IN_GET; /* prevent circular getting */
			zend_std_call_getter(object, member, &rv TSRMLS_CC);
			*guard &= ~IN_GET;

//???
#if 0
			if (rv) {
				retval = rv;
				if (!Z_ISREF_P(rv) &&
				    (type == BP_VAR_W || type == BP_VAR_RW  || type == BP_VAR_UNSET)) {
					if (Z_REFCOUNT_P(rv) > 0) {
						zval *tmp = rv;

						ALLOC_ZVAL(rv);
						ZVAL_DUP(rv, tmp);
						Z_UNSET_ISREF_P(rv);
						Z_SET_REFCOUNT_P(rv, 0);
					}
					if (UNEXPECTED(Z_TYPE_P(rv) != IS_OBJECT)) {
						zend_error(E_NOTICE, "Indirect modification of overloaded property %s::$%s has no effect", zobj->ce->name, Z_STRVAL_P(member));
					}
				}
			} else {
				retval = &EG(uninitialized_zval_ptr);
			}
			if (EXPECTED(*retval != object)) {
				zval_ptr_dtor(&object);
			} else {
				Z_DELREF_P(object);
			}
#endif
		} else {
			if (Z_STRVAL_P(member)[0] == '\0') {
				if (Z_STRLEN_P(member) == 0) {
					zend_error(E_ERROR, "Cannot access empty property");
				} else {
					zend_error(E_ERROR, "Cannot access property started with '\\0'");
				}
			}
		}
	} else {
		if (!silent) {
			zend_error(E_NOTICE,"Undefined property: %s::$%s", zobj->ce->name->val, Z_STRVAL_P(member));
		}
		retval = &EG(uninitialized_zval);
	}
exit:
	if (UNEXPECTED(Z_TYPE(tmp_member) != IS_UNDEF)) {
		Z_ADDREF_P(retval);
		zval_ptr_dtor(&tmp_member);
		Z_DELREF_P(retval);
	}
	return retval;
}
/* }}} */

ZEND_API void zend_std_write_property(zval *object, zval *member, zval *value, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zval tmp_member;
	zval *variable_ptr;
	zend_property_info *property_info;

	zobj = Z_OBJ_P(object);

	ZVAL_UNDEF(&tmp_member);
 	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		ZVAL_DUP(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

	property_info = zend_get_property_info_quick(zobj->ce, member, (zobj->ce->__set != NULL), key TSRMLS_CC);

	if (EXPECTED(property_info != NULL)) {
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0 &&
		    Z_TYPE(zobj->properties_table[property_info->offset]) != IS_UNDEF) {
			variable_ptr = &zobj->properties_table[property_info->offset];
			if (Z_TYPE_P(variable_ptr) == IS_INDIRECT) {
				variable_ptr = Z_INDIRECT_P(variable_ptr);
			}
			goto found;
		}
		if (EXPECTED(zobj->properties != NULL)) {
			if ((variable_ptr = zend_hash_find(zobj->properties, property_info->name)) != NULL) {
found:
				/* if we already have this value there, we don't actually need to do anything */
				if (EXPECTED(variable_ptr != value)) {
					/* if we are assigning reference, we shouldn't move it, but instead assign variable
					   to the same pointer */
					if (Z_ISREF_P(variable_ptr)) {
						zval garbage;

						ZVAL_COPY_VALUE(&garbage, Z_REFVAL_P(variable_ptr)); /* old value should be destroyed */

						/* To check: can't *variable_ptr be some system variable like error_zval here? */
						ZVAL_COPY_VALUE(Z_REFVAL_P(variable_ptr), value);
						if (Z_REFCOUNT_P(value) > 0) {
							zval_copy_ctor(Z_REFVAL_P(variable_ptr));
						}
						zval_dtor(&garbage);
					} else {
						zval garbage;

						ZVAL_COPY_VALUE(&garbage, variable_ptr);

						/* if we assign referenced variable, we should separate it */
						if (IS_REFCOUNTED(Z_TYPE_P(value))) {
							Z_ADDREF_P(value);
							if (Z_ISREF_P(value)) {
								SEPARATE_ZVAL(value);
							}
						}
						ZVAL_COPY_VALUE(variable_ptr, value);
						zval_ptr_dtor(&garbage);
					}
				}
				return;
			}
		}
	}

	if (zobj->ce->__set) {
		long *guard = zend_get_property_guard(zobj, property_info, member);

	    if (!((*guard) & IN_SET)) {
			Z_ADDREF_P(object);
			if (Z_ISREF_P(object)) {
				SEPARATE_ZVAL(object);
			}
			(*guard) |= IN_SET; /* prevent circular setting */
			if (zend_std_call_setter(object, member, value TSRMLS_CC) != SUCCESS) {
				/* for now, just ignore it - __set should take care of warnings, etc. */
			}
			(*guard) &= ~IN_SET;
			zval_ptr_dtor(object);
		} else {
			if (Z_STRVAL_P(member)[0] == '\0') {
				if (Z_STRLEN_P(member) == 0) {
					zend_error(E_ERROR, "Cannot access empty property");
				} else {
					zend_error(E_ERROR, "Cannot access property started with '\\0'");
				}
			}
		}
	} else if (EXPECTED(property_info != NULL)) {
		/* if we assign referenced variable, we should separate it */
		if (IS_REFCOUNTED(Z_TYPE_P(value))) {
			Z_ADDREF_P(value);
			if (Z_ISREF_P(value)) {
				SEPARATE_ZVAL(value);
			}
		}
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0) {

			if (zobj->properties) {
				zval *zv = zend_hash_update(zobj->properties, property_info->name, value);
			    ZVAL_INDIRECT(&zobj->properties_table[property_info->offset], zv);
			} else {
				ZVAL_COPY_VALUE(&zobj->properties_table[property_info->offset], value);
			}
		} else {
			if (!zobj->properties) {
				rebuild_object_properties(zobj);
			}
			zend_hash_update(zobj->properties, property_info->name, value);
		}
	}

	if (UNEXPECTED(Z_TYPE(tmp_member) != IS_UNDEF)) {
		zval_ptr_dtor(&tmp_member);
	}
}
/* }}} */

zval *zend_std_read_dimension(zval *object, zval *offset, int type TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zval retval, tmp;

	if (EXPECTED(instanceof_function_ex(ce, zend_ce_arrayaccess, 1 TSRMLS_CC) != 0)) {
		if(offset == NULL) {
			/* [] construct */
			ZVAL_UNDEF(&tmp);
			offset = &tmp;
		} else {
			SEPARATE_ARG_IF_REF(offset);
		}
		zend_call_method_with_1_params(object, ce, NULL, "offsetget", &retval, offset);

		zval_ptr_dtor(offset);

		if (UNEXPECTED(Z_TYPE(retval) == IS_UNDEF)) {
			if (UNEXPECTED(!EG(exception))) {
				zend_error_noreturn(E_ERROR, "Undefined offset for object of type %s used as array", ce->name->val);
			}
			return NULL;
		}

		/* Undo PZVAL_LOCK() */
		if (Z_REFCOUNTED(retval)) Z_DELREF(retval);

		// TODO: FIXME???
		//???return &retval;
		return NULL;
	} else {
		zend_error_noreturn(E_ERROR, "Cannot use object of type %s as array", ce->name->val);
		return NULL;
	}
}
/* }}} */

static void zend_std_write_dimension(zval *object, zval *offset, zval *value TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zval tmp;

	if (EXPECTED(instanceof_function_ex(ce, zend_ce_arrayaccess, 1 TSRMLS_CC) != 0)) {
		if (!offset) {
			ZVAL_UNDEF(&tmp);
			offset = &tmp;
		} else {
			SEPARATE_ARG_IF_REF(offset);
		}
		zend_call_method_with_2_params(object, ce, NULL, "offsetset", NULL, offset, value);
		zval_ptr_dtor(offset);
	} else {
		zend_error_noreturn(E_ERROR, "Cannot use object of type %s as array", ce->name->val);
	}
}
/* }}} */

static int zend_std_has_dimension(zval *object, zval *offset, int check_empty TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);
	zval retval;
	int result;

	if (EXPECTED(instanceof_function_ex(ce, zend_ce_arrayaccess, 1 TSRMLS_CC) != 0)) {
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(object, ce, NULL, "offsetexists", &retval, offset);
		if (EXPECTED(Z_TYPE(retval) != IS_UNDEF)) {
			result = i_zend_is_true(&retval TSRMLS_CC);
			zval_ptr_dtor(&retval);
			if (check_empty && result && EXPECTED(!EG(exception))) {
				zend_call_method_with_1_params(object, ce, NULL, "offsetget", &retval, offset);
				if (EXPECTED(Z_TYPE(retval) != IS_UNDEF)) {
					result = i_zend_is_true(&retval TSRMLS_CC);
					zval_ptr_dtor(&retval);
				}
			}
		} else {
			result = 0;
		}
		zval_ptr_dtor(offset);
	} else {
		zend_error_noreturn(E_ERROR, "Cannot use object of type %s as array", ce->name->val);
		return 0;
	}
	return result;
}
/* }}} */

static zval *zend_std_get_property_ptr_ptr(zval *object, zval *member, int type, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zval tmp_member;
	zval *retval;
	zend_property_info *property_info;
	long *guard;

	zobj = Z_OBJ_P(object);

	ZVAL_UNDEF(&tmp_member);
 	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		ZVAL_DUP(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

#if DEBUG_OBJECT_HANDLERS
	fprintf(stderr, "Ptr object #%d property: %s\n", Z_OBJ_HANDLE_P(object), Z_STRVAL_P(member));
#endif

	property_info = zend_get_property_info_quick(zobj->ce, member, (zobj->ce->__get != NULL), key TSRMLS_CC);

	if (EXPECTED(property_info != NULL)) {
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0 &&
		    Z_TYPE(zobj->properties_table[property_info->offset]) != IS_UNDEF) {
			retval = &zobj->properties_table[property_info->offset];
			if (Z_TYPE_P(retval) == IS_INDIRECT) {
				retval = Z_INDIRECT_P(retval);
			}
			goto exit;
		}
		if (UNEXPECTED(!zobj->properties)) {
			retval = zend_hash_find(zobj->properties, property_info->name);
			if (retval) goto exit;
		}
	}

	if (!zobj->ce->__get ||
		(guard = zend_get_property_guard(zobj, property_info, member)) ||
		(property_info && ((*guard) & IN_GET))) {

		/* we don't have access controls - will just add it */
		if(UNEXPECTED(type == BP_VAR_RW || type == BP_VAR_R)) {
			zend_error(E_NOTICE, "Undefined property: %s::$%s", zobj->ce->name->val, Z_STRVAL_P(member));
		}
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0) {
			if (zobj->properties) {				
				zval tmp;

				ZVAL_NULL(&tmp);
				retval = zend_hash_update(zobj->properties, property_info->name, &tmp);
			    ZVAL_INDIRECT(&zobj->properties_table[property_info->offset], retval);
			} else {
				retval = &zobj->properties_table[property_info->offset];
				ZVAL_NULL(retval);
			}
		} else {
			if (!zobj->properties) {
				rebuild_object_properties(zobj);
			}
			zend_hash_update(zobj->properties, property_info->name, retval);
		}
	} else {
		/* we do have getter - fail and let it try again with usual get/set */
		retval = NULL;
	}

exit:
	if (UNEXPECTED(Z_TYPE(tmp_member) != IS_UNDEF)) {
		zval_dtor(&tmp_member);
	}
	return retval;
}
/* }}} */

static void zend_std_unset_property(zval *object, zval *member, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zval tmp_member;
	zend_property_info *property_info;

	zobj = Z_OBJ_P(object);

	ZVAL_UNDEF(&tmp_member);
 	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		ZVAL_DUP(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

	property_info = zend_get_property_info_quick(zobj->ce, member, (zobj->ce->__unset != NULL), key TSRMLS_CC);

	if (EXPECTED(property_info != NULL) &&
	    EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
	    property_info->offset >= 0) {
		zval_ptr_dtor(&zobj->properties_table[property_info->offset]);
		ZVAL_UNDEF(&zobj->properties_table[property_info->offset]);
		if (!zobj->properties) goto exit;
	}
	if (UNEXPECTED(!property_info) ||
        !zobj->properties ||
        UNEXPECTED(zend_hash_del(zobj->properties, property_info->name) == FAILURE)) {
	
		if (zobj->ce->__unset) {
			long *guard = zend_get_property_guard(zobj, property_info, member);
			if (!((*guard) & IN_UNSET)) {
				/* have unseter - try with it! */
				Z_ADDREF_P(object);
				if (Z_ISREF_P(object)) {
					SEPARATE_ZVAL(object);
				}
				(*guard) |= IN_UNSET; /* prevent circular unsetting */
				zend_std_call_unsetter(object, member TSRMLS_CC);
				(*guard) &= ~IN_UNSET;
				zval_ptr_dtor(object);
			} else {
				if (Z_STRVAL_P(member)[0] == '\0') {
					if (Z_STRLEN_P(member) == 0) {
						zend_error(E_ERROR, "Cannot access empty property");
					} else {
						zend_error(E_ERROR, "Cannot access property started with '\\0'");
					}
				}
			}
		}
	}

exit:
	if (UNEXPECTED(Z_TYPE(tmp_member) != IS_NULL)) {
		zval_ptr_dtor(&tmp_member);
	}
}
/* }}} */

static void zend_std_unset_dimension(zval *object, zval *offset TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce = Z_OBJCE_P(object);

	if (instanceof_function_ex(ce, zend_ce_arrayaccess, 1 TSRMLS_CC)) {
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(object, ce, NULL, "offsetunset", NULL, offset);
		zval_ptr_dtor(offset);
	} else {
		zend_error_noreturn(E_ERROR, "Cannot use object of type %s as array", ce->name->val);
	}
}
/* }}} */

ZEND_API void zend_std_call_user_call(INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zend_internal_function *func = (zend_internal_function *)EG(current_execute_data)->function_state.function;
	zval method_name, method_args;
	zval method_result;
	zend_class_entry *ce = Z_OBJCE_P(this_ptr);

	array_init_size(&method_args, ZEND_NUM_ARGS());

	if (UNEXPECTED(zend_copy_parameters_array(ZEND_NUM_ARGS(), &method_args TSRMLS_CC) == FAILURE)) {
		zval_dtor(&method_args);
		zend_error_noreturn(E_ERROR, "Cannot get arguments for __call");
		RETURN_FALSE;
	}

	ZVAL_STR(&method_name, func->function_name); /* no dup - it's a copy */

	/* __call handler is called with two arguments:
	   method name
	   array of method parameters

	*/
	ZVAL_UNDEF(&method_result);
	zend_call_method_with_2_params(this_ptr, ce, &ce->__call, ZEND_CALL_FUNC_NAME, &method_result, &method_name, &method_args);

	if (Z_TYPE(method_result) != IS_UNDEF) {
		RETVAL_ZVAL_FAST(&method_result);
		zval_ptr_dtor(&method_result);
	}

	/* now destruct all auxiliaries */
	zval_ptr_dtor(&method_args);
	zval_ptr_dtor(&method_name);

	/* destruct the function also, then - we have allocated it in get_method */
	efree(func);
}
/* }}} */

/* Ensures that we're allowed to call a private method.
 * Returns the function address that should be called, or NULL
 * if no such function exists.
 */
static inline zend_function *zend_check_private_int(zend_function *fbc, zend_class_entry *ce, zend_string *function_name TSRMLS_DC) /* {{{ */
{
    zval *func;

	if (!ce) {
		return 0;
	}

	/* We may call a private function if:
	 * 1.  The class of our object is the same as the scope, and the private
	 *     function (EX(fbc)) has the same scope.
	 * 2.  One of our parent classes are the same as the scope, and it contains
	 *     a private function with the same name that has the same scope.
	 */
	if (fbc->common.scope == ce && EG(scope) == ce) {
		/* rule #1 checks out ok, allow the function call */
		return fbc;
	}


	/* Check rule #2 */
	ce = ce->parent;
	while (ce) {
		if (ce == EG(scope)) {
			if ((func = zend_hash_find(&ce->function_table, function_name))) {
				fbc = Z_FUNC_P(func);
				if (fbc->common.fn_flags & ZEND_ACC_PRIVATE
					&& fbc->common.scope == EG(scope)) {
					return fbc;
				}
			}
			break;
		}
		ce = ce->parent;
	}
	return NULL;
}
/* }}} */

ZEND_API int zend_check_private(zend_function *fbc, zend_class_entry *ce, zend_string *function_name TSRMLS_DC) /* {{{ */
{
	return zend_check_private_int(fbc, ce, function_name TSRMLS_CC) != NULL;
}
/* }}} */

/* Ensures that we're allowed to call a protected method.
 */
ZEND_API int zend_check_protected(zend_class_entry *ce, zend_class_entry *scope) /* {{{ */
{
	zend_class_entry *fbc_scope = ce;

	/* Is the context that's calling the function, the same as one of
	 * the function's parents?
	 */
	while (fbc_scope) {
		if (fbc_scope==scope) {
			return 1;
		}
		fbc_scope = fbc_scope->parent;
	}

	/* Is the function's scope the same as our current object context,
	 * or any of the parents of our context?
	 */
	while (scope) {
		if (scope==ce) {
			return 1;
		}
		scope = scope->parent;
	}
	return 0;
}
/* }}} */

static inline union _zend_function *zend_get_user_call_function(zend_class_entry *ce, zend_string *method_name) /* {{{ */
{
	zend_internal_function *call_user_call = emalloc(sizeof(zend_internal_function));
	call_user_call->type = ZEND_INTERNAL_FUNCTION;
	call_user_call->module = (ce->type == ZEND_INTERNAL_CLASS) ? ce->info.internal.module : NULL;
	call_user_call->handler = zend_std_call_user_call;
	call_user_call->arg_info = NULL;
	call_user_call->num_args = 0;
	call_user_call->scope = ce;
	call_user_call->fn_flags = ZEND_ACC_CALL_VIA_HANDLER;
	call_user_call->function_name = STR_COPY(method_name);

	return (union _zend_function *)call_user_call;
}
/* }}} */

static union _zend_function *zend_std_get_method(zval *object, zend_string *method_name, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zval *func;
	zend_function *fbc;
	zend_object *zobj = Z_OBJ_P(object);
	zend_string *lc_method_name;

	if (EXPECTED(key != NULL)) {
		lc_method_name = Z_STR(key->constant);
	} else {
		lc_method_name = STR_ALLOC(method_name->len, 0);
		zend_str_tolower_copy(lc_method_name->val, method_name->val, method_name->len);
	}

	if (UNEXPECTED((func = zend_hash_find(&zobj->ce->function_table, lc_method_name)) == NULL)) {
		if (UNEXPECTED(!key)) {
			STR_FREE(lc_method_name);
		}
		if (zobj->ce->__call) {
			return zend_get_user_call_function(zobj->ce, method_name);
		} else {
			return NULL;
		}
	}

	fbc = Z_FUNC_P(func);
	/* Check access level */
	if (fbc->op_array.fn_flags & ZEND_ACC_PRIVATE) {
		zend_function *updated_fbc;

		/* Ensure that if we're calling a private function, we're allowed to do so.
		 * If we're not and __call() handler exists, invoke it, otherwise error out.
		 */
		updated_fbc = zend_check_private_int(fbc, Z_OBJ_HANDLER_P(object, get_class_entry)(object TSRMLS_CC), lc_method_name TSRMLS_CC);
		if (EXPECTED(updated_fbc != NULL)) {
			fbc = updated_fbc;
		} else {
			if (zobj->ce->__call) {
				fbc = zend_get_user_call_function(zobj->ce, method_name);
			} else {
				zend_error_noreturn(E_ERROR, "Call to %s method %s::%s() from context '%s'", zend_visibility_string(fbc->common.fn_flags), ZEND_FN_SCOPE_NAME(fbc), method_name->val, EG(scope) ? EG(scope)->name->val : "");
			}
		}
	} else {
		/* Ensure that we haven't overridden a private function and end up calling
		 * the overriding public function...
		 */
		if (EG(scope) &&
		    is_derived_class(fbc->common.scope, EG(scope)) &&
		    fbc->op_array.fn_flags & ZEND_ACC_CHANGED) {
			if ((func = zend_hash_find(&EG(scope)->function_table, lc_method_name)) != SUCCESS) {
				zend_function *priv_fbc = Z_FUNC_P(func);
				if (priv_fbc->common.fn_flags & ZEND_ACC_PRIVATE
					&& priv_fbc->common.scope == EG(scope)) {
					fbc = priv_fbc;
				}
			}
		}
		if ((fbc->common.fn_flags & ZEND_ACC_PROTECTED)) {
			/* Ensure that if we're calling a protected function, we're allowed to do so.
			 * If we're not and __call() handler exists, invoke it, otherwise error out.
			 */
			if (UNEXPECTED(!zend_check_protected(zend_get_function_root_class(fbc), EG(scope)))) {
				if (zobj->ce->__call) {
					fbc = zend_get_user_call_function(zobj->ce, method_name);
				} else {
					zend_error_noreturn(E_ERROR, "Call to %s method %s::%s() from context '%s'", zend_visibility_string(fbc->common.fn_flags), ZEND_FN_SCOPE_NAME(fbc), method_name->val, EG(scope) ? EG(scope)->name->val : "");
				}
			}
		}
	}

	if (UNEXPECTED(!key)) {
		STR_FREE(lc_method_name);
	}
	return fbc;
}
/* }}} */

ZEND_API void zend_std_callstatic_user_call(INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zend_internal_function *func = (zend_internal_function *)EG(current_execute_data)->function_state.function;
	zval method_name, method_args;
	zval method_result;
	zend_class_entry *ce = EG(scope);

	array_init_size(&method_args, ZEND_NUM_ARGS());

	if (UNEXPECTED(zend_copy_parameters_array(ZEND_NUM_ARGS(), &method_args TSRMLS_CC) == FAILURE)) {
		zval_dtor(&method_args);
		zend_error_noreturn(E_ERROR, "Cannot get arguments for " ZEND_CALLSTATIC_FUNC_NAME);
		RETURN_FALSE;
	}

	ZVAL_STR(&method_name, func->function_name); /* no dup - it's a copy */

	/* __callStatic handler is called with two arguments:
	   method name
	   array of method parameters
	*/
	ZVAL_UNDEF(&method_result);
	zend_call_method_with_2_params(NULL, ce, &ce->__callstatic, ZEND_CALLSTATIC_FUNC_NAME, &method_result, &method_name, &method_args);

	if (Z_TYPE(method_result) != IS_UNDEF) {
		RETVAL_ZVAL_FAST(&method_result);
		zval_ptr_dtor(&method_result);
	}

	/* now destruct all auxiliaries */
	zval_ptr_dtor(&method_args);
	zval_ptr_dtor(&method_name);

	/* destruct the function also, then - we have allocated it in get_method */
	efree(func);
}
/* }}} */

static inline union _zend_function *zend_get_user_callstatic_function(zend_class_entry *ce, zend_string *method_name) /* {{{ */
{
	zend_internal_function *callstatic_user_call = emalloc(sizeof(zend_internal_function));
	callstatic_user_call->type     = ZEND_INTERNAL_FUNCTION;
	callstatic_user_call->module   = (ce->type == ZEND_INTERNAL_CLASS) ? ce->info.internal.module : NULL;
	callstatic_user_call->handler  = zend_std_callstatic_user_call;
	callstatic_user_call->arg_info = NULL;
	callstatic_user_call->num_args = 0;
	callstatic_user_call->scope    = ce;
	callstatic_user_call->fn_flags = ZEND_ACC_STATIC | ZEND_ACC_PUBLIC | ZEND_ACC_CALL_VIA_HANDLER;
	callstatic_user_call->function_name = STR_COPY(method_name);

	return (zend_function *)callstatic_user_call;
}
/* }}} */

/* This is not (yet?) in the API, but it belongs in the built-in objects callbacks */

ZEND_API zend_function *zend_std_get_static_method(zend_class_entry *ce, zend_string *function_name, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_function *fbc = NULL;
	char *lc_class_name;
	zend_string *lc_function_name;

	if (EXPECTED(key != NULL)) {
		lc_function_name = Z_STR(key->constant);
	} else {
		lc_function_name = STR_ALLOC(function_name->len, 0);
		zend_str_tolower_copy(lc_function_name->val, function_name->val, function_name->len);
	}

	if (function_name->len == ce->name->len && ce->constructor) {
		lc_class_name = zend_str_tolower_dup(ce->name->val, ce->name->len);
		/* Only change the method to the constructor if the constructor isn't called __construct
		 * we check for __ so we can be binary safe for lowering, we should use ZEND_CONSTRUCTOR_FUNC_NAME
		 */
		if (!memcmp(lc_class_name, lc_function_name->val, function_name->len) && memcmp(ce->constructor->common.function_name->val, "__", sizeof("__") - 1)) {
			fbc = ce->constructor;
		}
		efree(lc_class_name);
	}

	if (EXPECTED(!fbc)) {
		zval *func = zend_hash_find(&ce->function_table, lc_function_name);
		if (EXPECTED(func != NULL)) {
			fbc = Z_FUNC_P(func);
		} else {
			if (UNEXPECTED(!key)) {
				STR_FREE(lc_function_name);
			}
			if (ce->__call &&
			    Z_TYPE(EG(This)) == IS_OBJECT &&
			    Z_OBJ_HT(EG(This))->get_class_entry &&
			    instanceof_function(Z_OBJCE(EG(This)), ce TSRMLS_CC)) {
				return zend_get_user_call_function(ce, function_name);
			} else if (ce->__callstatic) {
				return zend_get_user_callstatic_function(ce, function_name);
			} else {
	   			return NULL;
			}
		}
	}

#if MBO_0
	/* right now this function is used for non static method lookup too */
	/* Is the function static */
	if (UNEXPECTED(!(fbc->common.fn_flags & ZEND_ACC_STATIC))) {
		zend_error_noreturn(E_ERROR, "Cannot call non static method %s::%s() without object", ZEND_FN_SCOPE_NAME(fbc), fbc->common.function_name);
	}
#endif
	if (fbc->op_array.fn_flags & ZEND_ACC_PUBLIC) {
		/* No further checks necessary, most common case */
	} else if (fbc->op_array.fn_flags & ZEND_ACC_PRIVATE) {
		zend_function *updated_fbc;

		/* Ensure that if we're calling a private function, we're allowed to do so.
		 */
		updated_fbc = zend_check_private_int(fbc, EG(scope), lc_function_name TSRMLS_CC);
		if (EXPECTED(updated_fbc != NULL)) {
			fbc = updated_fbc;
		} else {
			if (ce->__callstatic) {
				fbc = zend_get_user_callstatic_function(ce, function_name);
			} else {
				zend_error_noreturn(E_ERROR, "Call to %s method %s::%s() from context '%s'", zend_visibility_string(fbc->common.fn_flags), ZEND_FN_SCOPE_NAME(fbc), function_name->val, EG(scope) ? EG(scope)->name->val : "");
			}
		}
	} else if ((fbc->common.fn_flags & ZEND_ACC_PROTECTED)) {
		/* Ensure that if we're calling a protected function, we're allowed to do so.
		 */
		if (UNEXPECTED(!zend_check_protected(zend_get_function_root_class(fbc), EG(scope)))) {
			if (ce->__callstatic) {
				fbc = zend_get_user_callstatic_function(ce, function_name);
			} else {
				zend_error_noreturn(E_ERROR, "Call to %s method %s::%s() from context '%s'", zend_visibility_string(fbc->common.fn_flags), ZEND_FN_SCOPE_NAME(fbc), function_name->val, EG(scope) ? EG(scope)->name->val : "");
			}
		}
	}

	if (UNEXPECTED(!key)) {
		STR_FREE(lc_function_name);
	}

	return fbc;
}
/* }}} */

ZEND_API zval *zend_std_get_static_property(zend_class_entry *ce, zend_string *property_name, zend_bool silent, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_property_info *property_info;

	if (UNEXPECTED(!key) ||
	    (property_info = CACHED_POLYMORPHIC_PTR(key->cache_slot, ce)) == NULL) {

		if (UNEXPECTED((property_info = zend_hash_find_ptr(&ce->properties_info, property_name)) == NULL)) {
			if (!silent) {
				zend_error_noreturn(E_ERROR, "Access to undeclared static property: %s::$%s", ce->name->val, property_name->val);
			}
			return NULL;
		}

		if (UNEXPECTED(!zend_verify_property_access(property_info, ce TSRMLS_CC))) {
			if (!silent) {
				zend_error_noreturn(E_ERROR, "Cannot access %s property %s::$%s", zend_visibility_string(property_info->flags), ce->name, property_name);
			}
			return NULL;
		}

		if (UNEXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0)) {
			if (!silent) {
				zend_error_noreturn(E_ERROR, "Access to undeclared static property: %s::$%s", ce->name, property_name);
			}
			return NULL;
		}

		zend_update_class_constants(ce TSRMLS_CC);

		if (EXPECTED(key != NULL)) {
			CACHE_POLYMORPHIC_PTR(key->cache_slot, ce, property_info);
		}
	}

	if (UNEXPECTED(CE_STATIC_MEMBERS(ce) == NULL) ||
	    UNEXPECTED(Z_TYPE(CE_STATIC_MEMBERS(ce)[property_info->offset]) == IS_UNDEF)) {
		if (!silent) {
			zend_error_noreturn(E_ERROR, "Access to undeclared static property: %s::$%s", ce->name->val, property_name->val);
		}
		return NULL;
	}
	
	return &CE_STATIC_MEMBERS(ce)[property_info->offset];
}
/* }}} */

ZEND_API zend_bool zend_std_unset_static_property(zend_class_entry *ce, zend_string *property_name, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_error_noreturn(E_ERROR, "Attempt to unset static property %s::$%s", ce->name->val, property_name->val);
	return 0;
}
/* }}} */

ZEND_API union _zend_function *zend_std_get_constructor(zval *object TSRMLS_DC) /* {{{ */
{
	zend_object *zobj = Z_OBJ_P(object);
	zend_function *constructor = zobj->ce->constructor;

	if (constructor) {
		if (constructor->op_array.fn_flags & ZEND_ACC_PUBLIC) {
			/* No further checks necessary */
		} else if (constructor->op_array.fn_flags & ZEND_ACC_PRIVATE) {
			/* Ensure that if we're calling a private function, we're allowed to do so.
			 */
			if (UNEXPECTED(constructor->common.scope != EG(scope))) {
				if (EG(scope)) {
					zend_error_noreturn(E_ERROR, "Call to private %s::%s() from context '%s'", constructor->common.scope->name, constructor->common.function_name, EG(scope)->name);
				} else {
					zend_error_noreturn(E_ERROR, "Call to private %s::%s() from invalid context", constructor->common.scope->name, constructor->common.function_name);
				}
			}
		} else if ((constructor->common.fn_flags & ZEND_ACC_PROTECTED)) {
			/* Ensure that if we're calling a protected function, we're allowed to do so.
			 * Constructors only have prototype if they are defined by an interface but
			 * it is the compilers responsibility to take care of the prototype.
			 */
			if (UNEXPECTED(!zend_check_protected(zend_get_function_root_class(constructor), EG(scope)))) {
				if (EG(scope)) {
					zend_error_noreturn(E_ERROR, "Call to protected %s::%s() from context '%s'", constructor->common.scope->name, constructor->common.function_name, EG(scope)->name);
				} else {
					zend_error_noreturn(E_ERROR, "Call to protected %s::%s() from invalid context", constructor->common.scope->name, constructor->common.function_name);
				}
			}
		}
	}

	return constructor;
}
/* }}} */

int zend_compare_symbol_tables_i(HashTable *ht1, HashTable *ht2 TSRMLS_DC);

static int zend_std_compare_objects(zval *o1, zval *o2 TSRMLS_DC) /* {{{ */
{
	zend_object *zobj1, *zobj2;

	zobj1 = Z_OBJ_P(o1);
	zobj2 = Z_OBJ_P(o2);

	if (zobj1->ce != zobj2->ce) {
		return 1; /* different classes */
	}
	if (!zobj1->properties && !zobj2->properties) {
		int i;

		Z_OBJ_PROTECT_RECURSION(o1);
		Z_OBJ_PROTECT_RECURSION(o2);
		for (i = 0; i < zobj1->ce->default_properties_count; i++) {
			if (Z_TYPE(zobj1->properties_table[i]) != IS_UNDEF) {
				if (Z_TYPE(zobj2->properties_table[i]) != IS_UNDEF) {
					zval result;
					zval *p1 = &zobj1->properties_table[i];
					zval *p2 = &zobj2->properties_table[i];

					if (Z_TYPE_P(p1) == IS_INDIRECT) {
						p1 = Z_INDIRECT_P(p1);
					}
					if (Z_TYPE_P(p2) == IS_INDIRECT) {
						p1 = Z_INDIRECT_P(p2);
					}
					if (compare_function(&result, p1, p2 TSRMLS_CC)==FAILURE) {
						Z_OBJ_UNPROTECT_RECURSION(o1);
						Z_OBJ_UNPROTECT_RECURSION(o2);
						return 1;
					}
					if (Z_LVAL(result) != 0) {
						Z_OBJ_UNPROTECT_RECURSION(o1);
						Z_OBJ_UNPROTECT_RECURSION(o2);
						return Z_LVAL(result);
					}
				} else {
					Z_OBJ_UNPROTECT_RECURSION(o1);
					Z_OBJ_UNPROTECT_RECURSION(o2);
					return 1;
				}
			} else {
				if (Z_TYPE(zobj2->properties_table[i]) != IS_UNDEF) {
					Z_OBJ_UNPROTECT_RECURSION(o1);
					Z_OBJ_UNPROTECT_RECURSION(o2);
					return 1;
				}
			}
		}
		Z_OBJ_UNPROTECT_RECURSION(o1);
		Z_OBJ_UNPROTECT_RECURSION(o2);
		return 0;
	} else {
		if (!zobj1->properties) {
			rebuild_object_properties(zobj1);
		}
		if (!zobj2->properties) {
			rebuild_object_properties(zobj2);
		}
		return zend_compare_symbol_tables_i(zobj1->properties, zobj2->properties TSRMLS_CC);
	}
}
/* }}} */

static int zend_std_has_property(zval *object, zval *member, int has_set_exists, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	int result;
	zval *value = NULL;
	zval tmp_member;
	zend_property_info *property_info;

	zobj = Z_OBJ_P(object);

	ZVAL_UNDEF(&tmp_member);
	if (UNEXPECTED(Z_TYPE_P(member) != IS_STRING)) {
		ZVAL_DUP(&tmp_member, member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

	property_info = zend_get_property_info_quick(zobj->ce, member, 1, key TSRMLS_CC);

	if (EXPECTED(property_info != NULL)) {
		if (EXPECTED((property_info->flags & ZEND_ACC_STATIC) == 0) &&
		    property_info->offset >= 0 &&
		    Z_TYPE(zobj->properties_table[property_info->offset]) != IS_UNDEF) {
			value = &zobj->properties_table[property_info->offset];
			if (Z_TYPE_P(value) == IS_INDIRECT) {
				value = Z_INDIRECT_P(value);
			}
			goto found;
		}
		if (UNEXPECTED(zobj->properties != NULL)) {
			if ((value = zend_hash_find(zobj->properties, property_info->name)) != NULL) {
found:
				switch (has_set_exists) {
					case 0:
						result = (Z_TYPE_P(value) != IS_NULL);
						break;
					default:
						result = zend_is_true(value TSRMLS_CC);
						break;
					case 2:
						result = 1;
						break;
				}
			}
			goto exit;
		}
	}

	result = 0;
	if ((has_set_exists != 2) && zobj->ce->__isset) {
		long *guard = zend_get_property_guard(zobj, property_info, member);

		if (!((*guard) & IN_ISSET)) {
			zval rv;

			/* have issetter - try with it! */
			Z_ADDREF_P(object);
			if (Z_ISREF_P(object)) {
				SEPARATE_ZVAL(object);
			}
			(*guard) |= IN_ISSET; /* prevent circular getting */
			ZVAL_UNDEF(&rv);
			zend_std_call_issetter(object, member, &rv TSRMLS_CC);
			if (Z_TYPE(rv) != IS_UNDEF) {
				result = zend_is_true(&rv TSRMLS_CC);
				zval_ptr_dtor(&rv);
				if (has_set_exists && result) {
					if (EXPECTED(!EG(exception)) && zobj->ce->__get && !((*guard) & IN_GET)) {
						(*guard) |= IN_GET;
						ZVAL_UNDEF(&rv);
						zend_std_call_getter(object, member, &rv TSRMLS_CC);
						(*guard) &= ~IN_GET;
						if (Z_TYPE(rv) != IS_UNDEF) {
							Z_ADDREF(rv);
							result = i_zend_is_true(&rv TSRMLS_CC);
							zval_ptr_dtor(&rv);
						} else {
							result = 0;
						}
					} else {
						result = 0;
					}
				}
			}
			(*guard) &= ~IN_ISSET;
			zval_ptr_dtor(object);
		}
	}

exit:
	if (UNEXPECTED(Z_TYPE(tmp_member) != IS_UNDEF)) {
		zval_ptr_dtor(&tmp_member);
	}
	return result;
}
/* }}} */

zend_class_entry *zend_std_object_get_class(const zval *object TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zobj = Z_OBJ_P(object);

	return zobj->ce;
}
/* }}} */

zend_string* zend_std_object_get_class_name(const zval *object, int parent TSRMLS_DC) /* {{{ */
{
	zend_object *zobj;
	zend_class_entry *ce;
	zobj = Z_OBJ_P(object);

	if (parent) {
		if (!zobj->ce->parent) {
			return NULL;
		}
		ce = zobj->ce->parent;
	} else {
		ce = zobj->ce;
	}

	return STR_COPY(ce->name);
}
/* }}} */

ZEND_API int zend_std_cast_object_tostring(zval *readobj, zval *writeobj, int type TSRMLS_DC) /* {{{ */
{
	zval retval;
	zend_class_entry *ce;

	switch (type) {
		case IS_STRING:
		ZVAL_UNDEF(&retval);
			ce = Z_OBJCE_P(readobj);
			if (ce->__tostring &&
				(zend_call_method_with_0_params(readobj, ce, &ce->__tostring, "__tostring", &retval) || EG(exception))) {
				if (UNEXPECTED(EG(exception) != NULL)) {
					if (Z_TYPE(retval) != IS_UNDEF) {
						zval_ptr_dtor(&retval);
					}
					EG(exception) = NULL;
					zend_error_noreturn(E_ERROR, "Method %s::__toString() must not throw an exception", ce->name->val);
					return FAILURE;
				}
				if (EXPECTED(Z_TYPE(retval) == IS_STRING)) {
//???					INIT_PZVAL(writeobj);
					if (readobj == writeobj) {
						zval_ptr_dtor(readobj);
					}
					ZVAL_COPY_VALUE(writeobj, &retval);
					if (Z_TYPE_P(writeobj) != type) {
						convert_to_explicit_type(writeobj, type);
					}
					return SUCCESS;
				} else {
					zval_ptr_dtor(&retval);
//???					INIT_PZVAL(writeobj);
					if (readobj == writeobj) {
						zval_ptr_dtor(readobj);
					}
					ZVAL_EMPTY_STRING(writeobj);
					zend_error(E_RECOVERABLE_ERROR, "Method %s::__toString() must return a string value", ce->name->val);
					return SUCCESS;
				}
			}
			return FAILURE;
		case IS_BOOL:
			ZVAL_BOOL(writeobj, 1);
			return SUCCESS;
		case IS_LONG:
			ce = Z_OBJCE_P(readobj);
			zend_error(E_NOTICE, "Object of class %s could not be converted to int", ce->name->val);
			if (readobj == writeobj) {
				zval_dtor(readobj);
			}
			ZVAL_LONG(writeobj, 1);
			return SUCCESS;
		case IS_DOUBLE:
			ce = Z_OBJCE_P(readobj);
			zend_error(E_NOTICE, "Object of class %s could not be converted to double", ce->name->val);
			if (readobj == writeobj) {
				zval_dtor(readobj);
			}
			ZVAL_DOUBLE(writeobj, 1);
			return SUCCESS;
		default:
			ZVAL_NULL(writeobj);
			break;
	}
	return FAILURE;
}
/* }}} */

int zend_std_get_closure(zval *obj, zend_class_entry **ce_ptr, zend_function **fptr_ptr, zval *zobj_ptr TSRMLS_DC) /* {{{ */
{
	zval *func;
	zend_class_entry *ce;

	if (Z_TYPE_P(obj) != IS_OBJECT) {
		return FAILURE;
	}

	ce = Z_OBJCE_P(obj);

	if ((func = zend_hash_str_find(&ce->function_table, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1)) == NULL) {
		return FAILURE;
	}
	*fptr_ptr = Z_FUNC_P(func);

	*ce_ptr = ce;
	if ((*fptr_ptr)->common.fn_flags & ZEND_ACC_STATIC) {
		if (zobj_ptr) {
			ZVAL_UNDEF(zobj_ptr);
		}
	} else {
		if (zobj_ptr) {
			ZVAL_COPY_VALUE(zobj_ptr, obj);
		}
	}
	return SUCCESS;
}
/* }}} */

ZEND_API zend_object_handlers std_object_handlers = {
	zend_object_free,						/* free_obj */
	zend_objects_destroy_object,			/* dtor_obj */
	zend_objects_clone_obj,					/* clone_obj */

	zend_std_read_property,					/* read_property */
	zend_std_write_property,				/* write_property */
	zend_std_read_dimension,				/* read_dimension */
	zend_std_write_dimension,				/* write_dimension */
	zend_std_get_property_ptr_ptr,			/* get_property_ptr_ptr */
	NULL,									/* get */
	NULL,									/* set */
	zend_std_has_property,					/* has_property */
	zend_std_unset_property,				/* unset_property */
	zend_std_has_dimension,					/* has_dimension */
	zend_std_unset_dimension,				/* unset_dimension */
	zend_std_get_properties,				/* get_properties */
	zend_std_get_method,					/* get_method */
	NULL,									/* call_method */
	zend_std_get_constructor,				/* get_constructor */
	zend_std_object_get_class,				/* get_class_entry */
	zend_std_object_get_class_name,			/* get_class_name */
	zend_std_compare_objects,				/* compare_objects */
	zend_std_cast_object_tostring,			/* cast_object */
	NULL,									/* count_elements */
	NULL,									/* get_debug_info */
	zend_std_get_closure,					/* get_closure */
	zend_std_get_gc,						/* get_gc */
	NULL,									/* do_operation */
	NULL,									/* compare */
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
