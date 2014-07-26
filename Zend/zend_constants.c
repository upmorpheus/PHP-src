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
#include "zend_constants.h"
#include "zend_execute.h"
#include "zend_variables.h"
#include "zend_operators.h"
#include "zend_globals.h"
#include "zend_API.h"

void free_zend_constant(zval *zv)
{
	zend_constant *c = Z_PTR_P(zv);

	if (!(c->flags & CONST_PERSISTENT)) {
		if (Z_REFCOUNTED(c->value) || Z_IMMUTABLE(c->value)) {
			_zval_dtor_func(Z_COUNTED(c->value) ZEND_FILE_LINE_CC);
		}
	} else {
		zval_internal_dtor(&c->value);
	}
	if (c->name) {
		STR_RELEASE(c->name);
	}
	pefree(c, c->flags & CONST_PERSISTENT);
}


static void copy_zend_constant(zval *zv)
{
	zend_constant *c = Z_PTR_P(zv);

	Z_PTR_P(zv) = pemalloc(sizeof(zend_constant), c->flags & CONST_PERSISTENT);
	memcpy(Z_PTR_P(zv), c, sizeof(zend_constant));

	c = Z_PTR_P(zv);
	c->name = STR_COPY(c->name);
	if (!(c->flags & CONST_PERSISTENT)) {
		zval_copy_ctor(&c->value);
	} else {
		if (Z_TYPE(c->value) == IS_STRING) {
			Z_STR(c->value) = STR_DUP(Z_STR(c->value), 1);
		}
	}
}


void zend_copy_constants(HashTable *target, HashTable *source)
{
	zend_hash_copy(target, source, copy_zend_constant);
}


static int clean_non_persistent_constant(zval *zv TSRMLS_DC)
{
	zend_constant *c = Z_PTR_P(zv);
	return (c->flags & CONST_PERSISTENT) ? ZEND_HASH_APPLY_STOP : ZEND_HASH_APPLY_REMOVE;
}


static int clean_non_persistent_constant_full(zval *zv TSRMLS_DC)
{
	zend_constant *c = Z_PTR_P(zv);
	return (c->flags & CONST_PERSISTENT) ? 0 : 1;
}


static int clean_module_constant(zval *el, void *arg TSRMLS_DC)
{
	zend_constant *c = (zend_constant *)Z_PTR_P(el);
	int module_number = *(int *)arg;

	if (c->module_number == module_number) {
		return 1;
	} else {
		return 0;
	}
}


void clean_module_constants(int module_number TSRMLS_DC)
{
	zend_hash_apply_with_argument(EG(zend_constants), clean_module_constant, (void *) &module_number TSRMLS_CC);
}


int zend_startup_constants(TSRMLS_D)
{
	EG(zend_constants) = (HashTable *) malloc(sizeof(HashTable));

	zend_hash_init(EG(zend_constants), 128, NULL, ZEND_CONSTANT_DTOR, 1);
	return SUCCESS;
}



void zend_register_standard_constants(TSRMLS_D)
{
	REGISTER_MAIN_LONG_CONSTANT("E_ERROR", E_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_RECOVERABLE_ERROR", E_RECOVERABLE_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_WARNING", E_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_PARSE", E_PARSE, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_NOTICE", E_NOTICE, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_STRICT", E_STRICT, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_DEPRECATED", E_DEPRECATED, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_CORE_ERROR", E_CORE_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_CORE_WARNING", E_CORE_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_COMPILE_ERROR", E_COMPILE_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_COMPILE_WARNING", E_COMPILE_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_ERROR", E_USER_ERROR, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_WARNING", E_USER_WARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_NOTICE", E_USER_NOTICE, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("E_USER_DEPRECATED", E_USER_DEPRECATED, CONST_PERSISTENT | CONST_CS);

	REGISTER_MAIN_LONG_CONSTANT("E_ALL", E_ALL, CONST_PERSISTENT | CONST_CS);

	REGISTER_MAIN_LONG_CONSTANT("DEBUG_BACKTRACE_PROVIDE_OBJECT", DEBUG_BACKTRACE_PROVIDE_OBJECT, CONST_PERSISTENT | CONST_CS);
	REGISTER_MAIN_LONG_CONSTANT("DEBUG_BACKTRACE_IGNORE_ARGS", DEBUG_BACKTRACE_IGNORE_ARGS, CONST_PERSISTENT | CONST_CS);
	/* true/false constants */
	{
		REGISTER_MAIN_BOOL_CONSTANT("TRUE", 1, CONST_PERSISTENT | CONST_CT_SUBST);
		REGISTER_MAIN_BOOL_CONSTANT("FALSE", 0, CONST_PERSISTENT | CONST_CT_SUBST);
		REGISTER_MAIN_BOOL_CONSTANT("ZEND_THREAD_SAFE", ZTS_V, CONST_PERSISTENT | CONST_CS);
		REGISTER_MAIN_BOOL_CONSTANT("ZEND_DEBUG_BUILD", ZEND_DEBUG, CONST_PERSISTENT | CONST_CS);
	}
	REGISTER_MAIN_NULL_CONSTANT("NULL", CONST_PERSISTENT | CONST_CT_SUBST);
}


int zend_shutdown_constants(TSRMLS_D)
{
	zend_hash_destroy(EG(zend_constants));
	free(EG(zend_constants));
	return SUCCESS;
}


void clean_non_persistent_constants(TSRMLS_D)
{
	if (EG(full_tables_cleanup)) {
		zend_hash_apply(EG(zend_constants), clean_non_persistent_constant_full TSRMLS_CC);
	} else {
		zend_hash_reverse_apply(EG(zend_constants), clean_non_persistent_constant TSRMLS_CC);
	}
}

ZEND_API void zend_register_null_constant(const char *name, uint name_len, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;
	
	ZVAL_NULL(&c.value);
	c.flags = flags;
	c.name = STR_INIT(name, name_len, flags & CONST_PERSISTENT);
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}

ZEND_API void zend_register_bool_constant(const char *name, uint name_len, zend_bool bval, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;
	
	ZVAL_BOOL(&c.value, bval);
	c.flags = flags;
	c.name = STR_INIT(name, name_len, flags & CONST_PERSISTENT);
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}

ZEND_API void zend_register_long_constant(const char *name, uint name_len, long lval, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;
	
	ZVAL_LONG(&c.value, lval);
	c.flags = flags;
	c.name = STR_INIT(name, name_len, flags & CONST_PERSISTENT);
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}


ZEND_API void zend_register_double_constant(const char *name, uint name_len, double dval, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;
	
	ZVAL_DOUBLE(&c.value, dval);
	c.flags = flags;
	c.name = STR_INIT(name, name_len, flags & CONST_PERSISTENT);
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}


ZEND_API void zend_register_stringl_constant(const char *name, uint name_len, char *strval, uint strlen, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;
	
	ZVAL_NEW_STR(&c.value, STR_INIT(strval, strlen, flags & CONST_PERSISTENT));
	c.flags = flags;
	c.name = STR_INIT(name, name_len, flags & CONST_PERSISTENT);
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}


ZEND_API void zend_register_string_constant(const char *name, uint name_len, char *strval, int flags, int module_number TSRMLS_DC)
{
	zend_register_stringl_constant(name, name_len, strval, strlen(strval), flags, module_number TSRMLS_CC);
}

static zend_constant *zend_get_special_constant(const char *name, uint name_len TSRMLS_DC)
{
	zend_constant *c;
	static char haltoff[] = "__COMPILER_HALT_OFFSET__";

	if (!EG(current_execute_data)) {
		return NULL;
	} else if (name_len == sizeof("__CLASS__")-1 &&
	          !memcmp(name, "__CLASS__", sizeof("__CLASS__")-1)) {

		/* Returned constants may be cached, so they have to be stored */
		if (EG(scope) && EG(scope)->name) {
			int const_name_len;
			zend_string *const_name;
			
			const_name_len = sizeof("\0__CLASS__") + EG(scope)->name->len;
			const_name = STR_ALLOC(const_name_len, 0);
			memcpy(const_name->val, "\0__CLASS__", sizeof("\0__CLASS__")-1);
			zend_str_tolower_copy(const_name->val + sizeof("\0__CLASS__")-1, EG(scope)->name->val, EG(scope)->name->len);
			if ((c = zend_hash_find_ptr(EG(zend_constants), const_name)) == NULL) {
				c = emalloc(sizeof(zend_constant));
				memset(c, 0, sizeof(zend_constant));
				ZVAL_STR(&c->value, STR_COPY(EG(scope)->name));
				zend_hash_add_ptr(EG(zend_constants), const_name, c);
			}
			STR_RELEASE(const_name);
		} else {
			zend_string *const_name = STR_INIT("\0__CLASS__", sizeof("\0__CLASS__")-1, 0);
			if ((c = zend_hash_find_ptr(EG(zend_constants), const_name)) == NULL) {
				c = emalloc(sizeof(zend_constant));
				memset(c, 0, sizeof(zend_constant));
				ZVAL_EMPTY_STRING(&c->value);
				zend_hash_add_ptr(EG(zend_constants), const_name, c);
			}
			STR_RELEASE(const_name);
		}
		return c;
	} else if (name_len == sizeof("__COMPILER_HALT_OFFSET__")-1 &&
	          !memcmp(name, "__COMPILER_HALT_OFFSET__", sizeof("__COMPILER_HALT_OFFSET__")-1)) {
		const char *cfilename;
		zend_string *haltname;
		int clen;

		cfilename = zend_get_executed_filename(TSRMLS_C);
		clen = strlen(cfilename);
		/* check for __COMPILER_HALT_OFFSET__ */
		haltname = zend_mangle_property_name(haltoff,
			sizeof("__COMPILER_HALT_OFFSET__") - 1, cfilename, clen, 0);
		c = zend_hash_find_ptr(EG(zend_constants), haltname);
		STR_FREE(haltname);
		return c;
	} else {
		return NULL;
	}
}


ZEND_API zval *zend_get_constant_str(const char *name, uint name_len TSRMLS_DC)
{
	zend_constant *c;
	ALLOCA_FLAG(use_heap)

	if ((c = zend_hash_str_find_ptr(EG(zend_constants), name, name_len)) == NULL) {
		char *lcname = do_alloca(name_len + 1, use_heap);
		zend_str_tolower_copy(lcname, name, name_len);
		if ((c = zend_hash_str_find_ptr(EG(zend_constants), lcname, name_len)) != NULL) {
			if (c->flags & CONST_CS) {
				c = NULL;
			}
		} else {
			c = zend_get_special_constant(name, name_len TSRMLS_CC);
		}
		free_alloca(lcname, use_heap);
	}

	return c ? &c->value : NULL;
}

ZEND_API zval *zend_get_constant(zend_string *name TSRMLS_DC)
{
	zend_constant *c;
	ALLOCA_FLAG(use_heap)

	if ((c = zend_hash_find_ptr(EG(zend_constants), name)) == NULL) {
		char *lcname = do_alloca(name->len + 1, use_heap);
		zend_str_tolower_copy(lcname, name->val, name->len);
		if ((c = zend_hash_str_find_ptr(EG(zend_constants), lcname, name->len)) != NULL) {
			if (c->flags & CONST_CS) {
				c = NULL;
			}
		} else {
			c = zend_get_special_constant(name->val, name->len TSRMLS_CC);
		}
		free_alloca(lcname, use_heap);
	}

	return c ? &c->value : NULL;
}

ZEND_API zval *zend_get_constant_ex(zend_string *cname, zend_class_entry *scope, ulong flags TSRMLS_DC)
{
	zend_constant *c;
	const char *colon;
	zend_class_entry *ce = NULL;
	zend_string *class_name;
	const char *name = cname->val;
	uint name_len = cname->len;

	/* Skip leading \\ */
	if (name[0] == '\\') {
		name += 1;
		name_len -= 1;
		cname = NULL;
	}

	if ((colon = zend_memrchr(name, ':', name_len)) &&
	    colon > name && (*(colon - 1) == ':')) {
		int class_name_len = colon - name - 1;
		int const_name_len = name_len - class_name_len - 2;
		zend_string *constant_name = STR_INIT(colon + 1, const_name_len, 0);
		char *lcname;
		zval *ret_constant = NULL;
		ALLOCA_FLAG(use_heap)

		class_name = STR_INIT(name, class_name_len, 0);
		lcname = do_alloca(class_name_len + 1, use_heap);
		zend_str_tolower_copy(lcname, name, class_name_len);
		if (!scope) {
			if (EG(current_execute_data)) {
				scope = EG(scope);
			} else {
				scope = CG(active_class_entry);
			}
		}

		if (class_name_len == sizeof("self")-1 &&
		    !memcmp(lcname, "self", sizeof("self")-1)) {
			if (scope) {
				ce = scope;
			} else {
				zend_error(E_ERROR, "Cannot access self:: when no class scope is active");
			}
		} else if (class_name_len == sizeof("parent")-1 &&
		           !memcmp(lcname, "parent", sizeof("parent")-1)) {
			if (!scope) {
				zend_error(E_ERROR, "Cannot access parent:: when no class scope is active");
			} else if (!scope->parent) {
				zend_error(E_ERROR, "Cannot access parent:: when current class scope has no parent");
			} else {
				ce = scope->parent;
			}
		} else if (class_name_len == sizeof("static")-1 &&
		           !memcmp(lcname, "static", sizeof("static")-1)) {
			if (EG(current_execute_data) && EG(current_execute_data)->called_scope) {
				ce = EG(current_execute_data)->called_scope;
			} else {
				zend_error(E_ERROR, "Cannot access static:: when no class scope is active");
			}
		} else {
			ce = zend_fetch_class(class_name, flags TSRMLS_CC);
		}
		free_alloca(lcname, use_heap);
		if (ce) {
			ret_constant = zend_hash_find(&ce->constants_table, constant_name);
			if (ret_constant == NULL) {
				if ((flags & ZEND_FETCH_CLASS_SILENT) == 0) {
					zend_error(E_ERROR, "Undefined class constant '%s::%s'", class_name->val, constant_name->val);
				}
			} else if (Z_ISREF_P(ret_constant)) {
				ret_constant = Z_REFVAL_P(ret_constant);
			}
		}
		STR_RELEASE(class_name);
		STR_FREE(constant_name);
		if (ret_constant && Z_CONSTANT_P(ret_constant)) {
			zval_update_constant_ex(ret_constant, 1, ce TSRMLS_CC);
		}
		return ret_constant;
	}

	/* non-class constant */
	if ((colon = zend_memrchr(name, '\\', name_len)) != NULL) {
		/* compound constant name */
		int prefix_len = colon - name;
		int const_name_len = name_len - prefix_len - 1;
		const char *constant_name = colon + 1;
		char *lcname;
		int lcname_len;
		ALLOCA_FLAG(use_heap)

		lcname_len = prefix_len + 1 + const_name_len;
		lcname = do_alloca(lcname_len + 1, use_heap);
		zend_str_tolower_copy(lcname, name, prefix_len);
		/* Check for namespace constant */

		lcname[prefix_len] = '\\';
		memcpy(lcname + prefix_len + 1, constant_name, const_name_len + 1);

		if ((c = zend_hash_str_find_ptr(EG(zend_constants), lcname, lcname_len)) == NULL) {
			/* try lowercase */
			zend_str_tolower(lcname + prefix_len + 1, const_name_len);
			if ((c = zend_hash_str_find_ptr(EG(zend_constants), lcname, lcname_len)) != NULL) {
				if ((c->flags & CONST_CS) != 0) {
					c = NULL;
				}
			}
		}
		free_alloca(lcname, use_heap);
		if (c) {
			return &c->value;
		}
		/* name requires runtime resolution, need to check non-namespaced name */
		if ((flags & IS_CONSTANT_UNQUALIFIED) != 0) {
			return zend_get_constant_str(constant_name, const_name_len TSRMLS_CC);
		}
		return NULL;
	}

	if (cname) {
		return zend_get_constant(cname TSRMLS_CC);
	} else {
		return zend_get_constant_str(name, name_len TSRMLS_CC);
	}
}

zend_constant *zend_quick_get_constant(const zval *key, ulong flags TSRMLS_DC)
{
	zend_constant *c;

	if ((c = zend_hash_find_ptr(EG(zend_constants), Z_STR_P(key))) == NULL) {
		key++;
		if ((c = zend_hash_find_ptr(EG(zend_constants), Z_STR_P(key))) == NULL ||
		    (c->flags & CONST_CS) != 0) {
			if ((flags & (IS_CONSTANT_IN_NAMESPACE|IS_CONSTANT_UNQUALIFIED)) == (IS_CONSTANT_IN_NAMESPACE|IS_CONSTANT_UNQUALIFIED)) {
				key++;
				if ((c = zend_hash_find_ptr(EG(zend_constants), Z_STR_P(key))) == NULL) {
				    key++;
					if ((c = zend_hash_find_ptr(EG(zend_constants), Z_STR_P(key))) == NULL ||
					    (c->flags & CONST_CS) != 0) {

						key--;
						c = zend_get_special_constant(Z_STRVAL_P(key), Z_STRLEN_P(key) TSRMLS_CC);
					}
				}
			} else {
				key--;
				c = zend_get_special_constant(Z_STRVAL_P(key), Z_STRLEN_P(key) TSRMLS_CC);
			}
		}
	}
	return c;
}

static void* zend_hash_add_constant(HashTable *ht, zend_string *key, zend_constant *c)
{
	void *ret;
	zend_constant *copy = pemalloc(sizeof(zend_constant), c->flags & CONST_PERSISTENT);

	memcpy(copy, c, sizeof(zend_constant));
	ret = zend_hash_add_ptr(ht, key, copy);
	if (!ret) {
		pefree(copy, c->flags & CONST_PERSISTENT);
	}
	return ret;
}

ZEND_API int zend_register_constant(zend_constant *c TSRMLS_DC)
{
	zend_string *lowercase_name = NULL;
	zend_string *name;
	int ret = SUCCESS;

#if 0
	printf("Registering constant for module %d\n", c->module_number);
#endif

	if (!(c->flags & CONST_CS)) {
		lowercase_name = STR_ALLOC(c->name->len, c->flags & CONST_PERSISTENT);
		zend_str_tolower_copy(lowercase_name->val, c->name->val, c->name->len);
		lowercase_name = zend_new_interned_string(lowercase_name TSRMLS_CC);
		name = lowercase_name;
	} else {
		char *slash = strrchr(c->name->val, '\\');
		if (slash) {
			lowercase_name = STR_INIT(c->name->val, c->name->len, c->flags & CONST_PERSISTENT);
			zend_str_tolower(lowercase_name->val, slash - c->name->val);
			lowercase_name = zend_new_interned_string(lowercase_name TSRMLS_CC);
			name = lowercase_name;
		} else {
			name = c->name;
		}
	}

	/* Check if the user is trying to define the internal pseudo constant name __COMPILER_HALT_OFFSET__ */
	if ((c->name->len == sizeof("__COMPILER_HALT_OFFSET__")-1
		&& !memcmp(name->val, "__COMPILER_HALT_OFFSET__", sizeof("__COMPILER_HALT_OFFSET__")-1))
		|| zend_hash_add_constant(EG(zend_constants), name, c) == NULL) {
		
		/* The internal __COMPILER_HALT_OFFSET__ is prefixed by NULL byte */
		if (c->name->val[0] == '\0' && c->name->len > sizeof("\0__COMPILER_HALT_OFFSET__")-1
			&& memcmp(name->val, "\0__COMPILER_HALT_OFFSET__", sizeof("\0__COMPILER_HALT_OFFSET__")) == 0) {
		}
		zend_error(E_NOTICE,"Constant %s already defined", name->val);
		STR_RELEASE(c->name);
		if (!(c->flags & CONST_PERSISTENT)) {
			zval_dtor(&c->value);
		}
		ret = FAILURE;
	}
	if (lowercase_name) {
		STR_RELEASE(lowercase_name);
	}
	return ret;
}


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
