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
#include "zend_API.h"
#include "zend_builtin_functions.h"
#include "zend_constants.h"
#include "zend_ini.h"
#include "zend_exceptions.h"
#include "zend_extensions.h"
#include "zend_closures.h"

#undef ZEND_TEST_EXCEPTIONS

static ZEND_FUNCTION(zend_version);
static ZEND_FUNCTION(func_num_args);
static ZEND_FUNCTION(func_get_arg);
static ZEND_FUNCTION(func_get_args);
static ZEND_FUNCTION(strlen);
static ZEND_FUNCTION(strcmp);
static ZEND_FUNCTION(strncmp);
static ZEND_FUNCTION(strcasecmp);
static ZEND_FUNCTION(strncasecmp);
static ZEND_FUNCTION(each);
static ZEND_FUNCTION(error_reporting);
static ZEND_FUNCTION(define);
static ZEND_FUNCTION(defined);
static ZEND_FUNCTION(get_class);
static ZEND_FUNCTION(get_called_class);
static ZEND_FUNCTION(get_parent_class);
static ZEND_FUNCTION(method_exists);
static ZEND_FUNCTION(property_exists);
static ZEND_FUNCTION(class_exists);
static ZEND_FUNCTION(interface_exists);
static ZEND_FUNCTION(trait_exists);
static ZEND_FUNCTION(function_exists);
static ZEND_FUNCTION(class_alias);
#if ZEND_DEBUG
static ZEND_FUNCTION(leak);
static ZEND_FUNCTION(leak_variable);
#ifdef ZEND_TEST_EXCEPTIONS
static ZEND_FUNCTION(crash);
#endif
#endif
static ZEND_FUNCTION(get_included_files);
static ZEND_FUNCTION(is_subclass_of);
static ZEND_FUNCTION(is_a);
static ZEND_FUNCTION(get_class_vars);
static ZEND_FUNCTION(get_object_vars);
static ZEND_FUNCTION(get_class_methods);
static ZEND_FUNCTION(trigger_error);
static ZEND_FUNCTION(set_error_handler);
static ZEND_FUNCTION(restore_error_handler);
static ZEND_FUNCTION(set_exception_handler);
static ZEND_FUNCTION(restore_exception_handler);
static ZEND_FUNCTION(get_declared_classes);
static ZEND_FUNCTION(get_declared_traits);
static ZEND_FUNCTION(get_declared_interfaces);
static ZEND_FUNCTION(get_defined_functions);
static ZEND_FUNCTION(get_defined_vars);
static ZEND_FUNCTION(create_function);
static ZEND_FUNCTION(get_resource_type);
static ZEND_FUNCTION(get_loaded_extensions);
static ZEND_FUNCTION(extension_loaded);
static ZEND_FUNCTION(get_extension_funcs);
static ZEND_FUNCTION(get_defined_constants);
static ZEND_FUNCTION(debug_backtrace);
static ZEND_FUNCTION(debug_print_backtrace);
#if ZEND_DEBUG
static ZEND_FUNCTION(zend_test_func);
#ifdef ZTS
static ZEND_FUNCTION(zend_thread_id);
#endif
#endif
static ZEND_FUNCTION(gc_collect_cycles);
static ZEND_FUNCTION(gc_enabled);
static ZEND_FUNCTION(gc_enable);
static ZEND_FUNCTION(gc_disable);

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO(arginfo_zend__void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_func_get_arg, 0, 0, 1)
	ZEND_ARG_INFO(0, arg_num)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_strlen, 0, 0, 1)
	ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_strcmp, 0, 0, 2)
	ZEND_ARG_INFO(0, str1)
	ZEND_ARG_INFO(0, str2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_strncmp, 0, 0, 3)
	ZEND_ARG_INFO(0, str1)
	ZEND_ARG_INFO(0, str2)
	ZEND_ARG_INFO(0, len)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_each, 0, 0, 1)
	ZEND_ARG_INFO(1, arr)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_error_reporting, 0, 0, 0)
	ZEND_ARG_INFO(0, new_error_level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_define, 0, 0, 3)
	ZEND_ARG_INFO(0, constant_name)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, case_insensitive)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_defined, 0, 0, 1)
	ZEND_ARG_INFO(0, constant_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_class, 0, 0, 0)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_is_subclass_of, 0, 0, 2)
	ZEND_ARG_INFO(0, object)
	ZEND_ARG_INFO(0, class_name)
	ZEND_ARG_INFO(0, allow_string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_class_vars, 0, 0, 1)
	ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_object_vars, 0, 0, 1)
	ZEND_ARG_INFO(0, obj)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_class_methods, 0, 0, 1)
	ZEND_ARG_INFO(0, class)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_method_exists, 0, 0, 2)
	ZEND_ARG_INFO(0, object)
	ZEND_ARG_INFO(0, method)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_property_exists, 0, 0, 2)
	ZEND_ARG_INFO(0, object_or_class)
	ZEND_ARG_INFO(0, property_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_exists, 0, 0, 1)
	ZEND_ARG_INFO(0, classname)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_trait_exists, 0, 0, 1)
	ZEND_ARG_INFO(0, traitname)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_function_exists, 0, 0, 1)
	ZEND_ARG_INFO(0, function_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_alias, 0, 0, 2)
	ZEND_ARG_INFO(0, user_class_name)
	ZEND_ARG_INFO(0, alias_name)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()

#if ZEND_DEBUG
ZEND_BEGIN_ARG_INFO_EX(arginfo_leak_variable, 0, 0, 1)
	ZEND_ARG_INFO(0, variable)
	ZEND_ARG_INFO(0, leak_data)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_trigger_error, 0, 0, 1)
	ZEND_ARG_INFO(0, message)
	ZEND_ARG_INFO(0, error_type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set_error_handler, 0, 0, 1)
	ZEND_ARG_INFO(0, error_handler)
	ZEND_ARG_INFO(0, error_types)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set_exception_handler, 0, 0, 1)
	ZEND_ARG_INFO(0, exception_handler)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_create_function, 0, 0, 2)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, code)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_resource_type, 0, 0, 1)
	ZEND_ARG_INFO(0, res)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_loaded_extensions, 0, 0, 0)
	ZEND_ARG_INFO(0, zend_extensions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_get_defined_constants, 0, 0, 0)
	ZEND_ARG_INFO(0, categorize)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_debug_backtrace, 0, 0, 0)
	ZEND_ARG_INFO(0, options)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_debug_print_backtrace, 0, 0, 0)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_extension_loaded, 0, 0, 1)
	ZEND_ARG_INFO(0, extension_name)
ZEND_END_ARG_INFO()
/* }}} */

static const zend_function_entry builtin_functions[] = { /* {{{ */
	ZEND_FE(zend_version,		arginfo_zend__void)
	ZEND_FE(func_num_args,		arginfo_zend__void)
	ZEND_FE(func_get_arg,		arginfo_func_get_arg)
	ZEND_FE(func_get_args,		arginfo_zend__void)
	ZEND_FE(strlen,				arginfo_strlen)
	ZEND_FE(strcmp,				arginfo_strcmp)
	ZEND_FE(strncmp,			arginfo_strncmp)
	ZEND_FE(strcasecmp,			arginfo_strcmp)
	ZEND_FE(strncasecmp,		arginfo_strncmp)
	ZEND_FE(each,				arginfo_each)
	ZEND_FE(error_reporting,	arginfo_error_reporting)
	ZEND_FE(define,				arginfo_define)
	ZEND_FE(defined,			arginfo_defined)
	ZEND_FE(get_class,			arginfo_get_class)
	ZEND_FE(get_called_class,	arginfo_zend__void)
	ZEND_FE(get_parent_class,	arginfo_get_class)
	ZEND_FE(method_exists,		arginfo_method_exists)
	ZEND_FE(property_exists,	arginfo_property_exists)
	ZEND_FE(class_exists,		arginfo_class_exists)
	ZEND_FE(interface_exists,	arginfo_class_exists)
	ZEND_FE(trait_exists,		arginfo_trait_exists)
	ZEND_FE(function_exists,	arginfo_function_exists)
	ZEND_FE(class_alias,		arginfo_class_alias)
#if ZEND_DEBUG
	ZEND_FE(leak,				NULL)
	ZEND_FE(leak_variable,		arginfo_leak_variable)
#ifdef ZEND_TEST_EXCEPTIONS
	ZEND_FE(crash,				NULL)
#endif
#endif
	ZEND_FE(get_included_files,	arginfo_zend__void)
	ZEND_FALIAS(get_required_files,	get_included_files,		arginfo_zend__void)
	ZEND_FE(is_subclass_of,		arginfo_is_subclass_of)
	ZEND_FE(is_a,				arginfo_is_subclass_of)
	ZEND_FE(get_class_vars,		arginfo_get_class_vars)
	ZEND_FE(get_object_vars,	arginfo_get_object_vars)
	ZEND_FE(get_class_methods,	arginfo_get_class_methods)
	ZEND_FE(trigger_error,		arginfo_trigger_error)
	ZEND_FALIAS(user_error,		trigger_error,		arginfo_trigger_error)
	ZEND_FE(set_error_handler,			arginfo_set_error_handler)
	ZEND_FE(restore_error_handler,		arginfo_zend__void)
	ZEND_FE(set_exception_handler,		arginfo_set_exception_handler)
	ZEND_FE(restore_exception_handler,	arginfo_zend__void)
	ZEND_FE(get_declared_classes, 		arginfo_zend__void)
	ZEND_FE(get_declared_traits, 		arginfo_zend__void)
	ZEND_FE(get_declared_interfaces, 	arginfo_zend__void)
	ZEND_FE(get_defined_functions, 		arginfo_zend__void)
	ZEND_FE(get_defined_vars,			arginfo_zend__void)
	ZEND_FE(create_function,			arginfo_create_function)
	ZEND_FE(get_resource_type,			arginfo_get_resource_type)
	ZEND_FE(get_loaded_extensions,		arginfo_get_loaded_extensions)
	ZEND_FE(extension_loaded,			arginfo_extension_loaded)
	ZEND_FE(get_extension_funcs,		arginfo_extension_loaded)
	ZEND_FE(get_defined_constants,		arginfo_get_defined_constants)
	ZEND_FE(debug_backtrace, 			arginfo_debug_backtrace)
	ZEND_FE(debug_print_backtrace, 		arginfo_debug_print_backtrace)
#if ZEND_DEBUG
	ZEND_FE(zend_test_func,		NULL)
#ifdef ZTS
	ZEND_FE(zend_thread_id,		NULL)
#endif
#endif
	ZEND_FE(gc_collect_cycles, 	arginfo_zend__void)
	ZEND_FE(gc_enabled, 		arginfo_zend__void)
	ZEND_FE(gc_enable, 			arginfo_zend__void)
	ZEND_FE(gc_disable, 		arginfo_zend__void)
	ZEND_FE_END
};
/* }}} */

ZEND_MINIT_FUNCTION(core) { /* {{{ */
	zend_class_entry class_entry;

	INIT_CLASS_ENTRY(class_entry, "stdClass", NULL);
	zend_standard_class_def = zend_register_internal_class(&class_entry TSRMLS_CC);

	zend_register_default_classes(TSRMLS_C);

	return SUCCESS;
}
/* }}} */

zend_module_entry zend_builtin_module = { /* {{{ */
    STANDARD_MODULE_HEADER,
	"Core",
	builtin_functions,
	ZEND_MINIT(core),
	NULL,
	NULL,
	NULL,
	NULL,
	ZEND_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

int zend_startup_builtin_functions(TSRMLS_D) /* {{{ */
{
	zend_builtin_module.module_number = 0;
	zend_builtin_module.type = MODULE_PERSISTENT;
	return (EG(current_module) = zend_register_module_ex(&zend_builtin_module TSRMLS_CC)) == NULL ? FAILURE : SUCCESS;
}
/* }}} */

/* {{{ proto string zend_version(void)
   Get the version of the Zend Engine */
ZEND_FUNCTION(zend_version)
{
	RETURN_STRINGL(ZEND_VERSION, sizeof(ZEND_VERSION)-1);
}
/* }}} */

/* {{{ proto int gc_collect_cycles(void)
   Forces collection of any existing garbage cycles.
   Returns number of freed zvals */
ZEND_FUNCTION(gc_collect_cycles)
{
	RETURN_LONG(gc_collect_cycles(TSRMLS_C));
}
/* }}} */

/* {{{ proto void gc_enabled(void)
   Returns status of the circular reference collector */
ZEND_FUNCTION(gc_enabled)
{
	RETURN_BOOL(GC_G(gc_enabled));
}
/* }}} */

/* {{{ proto void gc_enable(void)
   Activates the circular reference collector */
ZEND_FUNCTION(gc_enable)
{
	zend_string *key = STR_INIT("zend.enable_gc", sizeof("zend.enable_gc")-1, 0);
	zend_alter_ini_entry(key, "1", sizeof("1")-1, ZEND_INI_USER, ZEND_INI_STAGE_RUNTIME);
	STR_RELEASE(key);
}
/* }}} */

/* {{{ proto void gc_disable(void)
   Deactivates the circular reference collector */
ZEND_FUNCTION(gc_disable)
{
	zend_string *key = STR_INIT("zend.enable_gc", sizeof("zend.enable_gc")-1, 0);
	zend_alter_ini_entry(key, "0", sizeof("0")-1, ZEND_INI_USER, ZEND_INI_STAGE_RUNTIME);
	STR_RELEASE(key);
}
/* }}} */

/* {{{ proto int func_num_args(void)
   Get the number of arguments that were passed to the function */
ZEND_FUNCTION(func_num_args)
{
	zend_execute_data *ex = EG(current_execute_data)->prev_execute_data;

	if (ex && ex->function_state.arguments) {
		RETURN_LONG(Z_LVAL_P(ex->function_state.arguments));
	} else {
		zend_error(E_WARNING, "func_num_args():  Called from the global scope - no function context");
		RETURN_LONG(-1);
	}
}
/* }}} */


/* {{{ proto mixed func_get_arg(int arg_num)
   Get the $arg_num'th argument that was passed to the function */
ZEND_FUNCTION(func_get_arg)
{
	zval *p;
	int arg_count;
	zval *arg;
	long requested_offset;
	zend_execute_data *ex = EG(current_execute_data)->prev_execute_data;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &requested_offset) == FAILURE) {
		return;
	}

	if (requested_offset < 0) {
		zend_error(E_WARNING, "func_get_arg():  The argument number should be >= 0");
		RETURN_FALSE;
	}

	if (!ex || !ex->function_state.arguments) {
		zend_error(E_WARNING, "func_get_arg():  Called from the global scope - no function context");
		RETURN_FALSE;
	}

	p = ex->function_state.arguments;
	arg_count = Z_LVAL_P(p);		/* this is the amount of arguments passed to func_get_arg(); */

	if (requested_offset >= arg_count) {
		zend_error(E_WARNING, "func_get_arg():  Argument %ld not passed to function", requested_offset);
		RETURN_FALSE;
	}

	arg = p-(arg_count-requested_offset);
	RETURN_ZVAL_FAST(arg);
}
/* }}} */


/* {{{ proto array func_get_args()
   Get an array of the arguments that were passed to the function */
ZEND_FUNCTION(func_get_args)
{
	zval *p;
	int arg_count;
	int i;
	zend_execute_data *ex = EG(current_execute_data)->prev_execute_data;

	if (!ex || !ex->function_state.arguments) {
		zend_error(E_WARNING, "func_get_args():  Called from the global scope - no function context");
		RETURN_FALSE;
	}

	p = ex->function_state.arguments;
	arg_count = Z_LVAL_P(p);		/* this is the amount of arguments passed to func_get_args(); */

	array_init_size(return_value, arg_count);
	for (i=0; i<arg_count; i++) {
		zval *element, *arg, tmp;

		arg = p-(arg_count-i);
		if (!Z_ISREF_P(arg)) {
			element = arg;
			if (Z_REFCOUNTED_P(element)) Z_ADDREF_P(element);
		} else {
			ZVAL_DUP(&tmp, Z_REFVAL_P(arg));
			element = &tmp;
	    }
		zend_hash_next_index_insert(Z_ARRVAL_P(return_value), element);
	}
}
/* }}} */


/* {{{ proto int strlen(string str)
   Get string length */
ZEND_FUNCTION(strlen)
{
	char *s1;
	int s1_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &s1, &s1_len) == FAILURE) {
		return;
	}

	RETVAL_LONG(s1_len);
}
/* }}} */


/* {{{ proto int strcmp(string str1, string str2)
   Binary safe string comparison */
ZEND_FUNCTION(strcmp)
{
	char *s1, *s2;
	int s1_len, s2_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &s1, &s1_len, &s2, &s2_len) == FAILURE) {
		return;
	}

	RETURN_LONG(zend_binary_strcmp(s1, s1_len, s2, s2_len));
}
/* }}} */


/* {{{ proto int strncmp(string str1, string str2, int len)
   Binary safe string comparison */
ZEND_FUNCTION(strncmp)
{
	char *s1, *s2;
	int s1_len, s2_len;
	long len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &s1, &s1_len, &s2, &s2_len, &len) == FAILURE) {
		return;
	}

	if (len < 0) {
		zend_error(E_WARNING, "Length must be greater than or equal to 0");
		RETURN_FALSE;
	}

	RETURN_LONG(zend_binary_strncmp(s1, s1_len, s2, s2_len, len));
}
/* }}} */


/* {{{ proto int strcasecmp(string str1, string str2)
   Binary safe case-insensitive string comparison */
ZEND_FUNCTION(strcasecmp)
{
	char *s1, *s2;
	int s1_len, s2_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &s1, &s1_len, &s2, &s2_len) == FAILURE) {
		return;
	}

	RETURN_LONG(zend_binary_strcasecmp(s1, s1_len, s2, s2_len));
}
/* }}} */


/* {{{ proto int strncasecmp(string str1, string str2, int len)
   Binary safe string comparison */
ZEND_FUNCTION(strncasecmp)
{
	char *s1, *s2;
	int s1_len, s2_len;
	long len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &s1, &s1_len, &s2, &s2_len, &len) == FAILURE) {
		return;
	}

	if (len < 0) {
		zend_error(E_WARNING, "Length must be greater than or equal to 0");
		RETURN_FALSE;
	}

	RETURN_LONG(zend_binary_strncasecmp(s1, s1_len, s2, s2_len, len));
}
/* }}} */


/* {{{ proto array each(array arr)
   Return the currently pointed key..value pair in the passed array, and advance the pointer to the next element */
ZEND_FUNCTION(each)
{
	zval *array, *entry, tmp;
	ulong num_key;
	zval *inserted_pointer;
	HashTable *target_hash;
	zend_string *key;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &array) == FAILURE) {
		return;
	}

	ZVAL_DEREF(array);
	target_hash = HASH_OF(array);
	if (!target_hash) {
		zend_error(E_WARNING,"Variable passed to each() is not an array or object");
		return;
	}
	while (1) {
		entry = zend_hash_get_current_data(target_hash);
		if (!entry) {
			RETURN_FALSE;
		} else if (Z_TYPE_P(entry) == IS_INDIRECT) {
			entry = Z_INDIRECT_P(entry);
			if (Z_TYPE_P(entry) == IS_UNDEF) {
				zend_hash_move_forward(target_hash);
				continue;
			}
		}
		break;
	}
	array_init(return_value);

	/* add value elements */
	if (Z_ISREF_P(entry)) {
		ZVAL_DUP(&tmp, Z_REFVAL_P(entry));
		entry = &tmp;
		if (Z_REFCOUNTED_P(entry)) Z_ADDREF_P(entry);
	} else {
		if (Z_REFCOUNTED_P(entry)) Z_ADDREF_P(entry);
		if (Z_REFCOUNTED_P(entry)) Z_ADDREF_P(entry);
	}
	zend_hash_index_update(Z_ARRVAL_P(return_value), 1, entry);
	zend_hash_str_update(Z_ARRVAL_P(return_value), "value", sizeof("value")-1, entry);

	/* add the key elements */
	if (zend_hash_get_current_key(target_hash, &key, &num_key, 0) == HASH_KEY_IS_STRING) {
		inserted_pointer = add_get_index_str(return_value, 0, STR_COPY(key));
	} else {
		inserted_pointer = add_get_index_long(return_value, 0, num_key);
	}
	zend_hash_str_update(Z_ARRVAL_P(return_value), "key", sizeof("key")-1, inserted_pointer);
	if (Z_REFCOUNTED_P(inserted_pointer)) Z_ADDREF_P(inserted_pointer);
	zend_hash_move_forward(target_hash);
}
/* }}} */


/* {{{ proto int error_reporting([int new_error_level])
   Return the current error_reporting level, and if an argument was passed - change to the new level */
ZEND_FUNCTION(error_reporting)
{
	char *err;
	int err_len;
	int old_error_reporting;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &err, &err_len) == FAILURE) {
		return;
	}

	old_error_reporting = EG(error_reporting);
	if(ZEND_NUM_ARGS() != 0) {
		zend_string *key = STR_INIT("error_reporting", sizeof("error_reporting")-1, 0);
		zend_alter_ini_entry(key, err, err_len, ZEND_INI_USER, ZEND_INI_STAGE_RUNTIME);
		STR_RELEASE(key);
	}

	RETVAL_LONG(old_error_reporting);
}
/* }}} */


/* {{{ proto bool define(string constant_name, mixed value, boolean case_insensitive=false)
   Define a new constant */
ZEND_FUNCTION(define)
{
	zend_string *name;
	zval *val, val_free;
	zend_bool non_cs = 0;
	int case_sensitive = CONST_CS;
	zend_constant c;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Sz|b", &name, &val, &non_cs) == FAILURE) {
		return;
	}

	if(non_cs) {
		case_sensitive = 0;
	}

	/* class constant, check if there is name and make sure class is valid & exists */
	if (zend_memnstr(name->val, "::", sizeof("::") - 1, name->val + name->len)) {
		zend_error(E_WARNING, "Class constants cannot be defined or redefined");
		RETURN_FALSE;
	}

	ZVAL_UNDEF(&val_free);

repeat:
	switch (Z_TYPE_P(val)) {
		case IS_LONG:
		case IS_DOUBLE:
		case IS_STRING:
		case IS_BOOL:
		case IS_RESOURCE:
		case IS_NULL:
			break;
		case IS_OBJECT:
			if (Z_TYPE(val_free) == IS_UNDEF) {
				if (Z_OBJ_HT_P(val)->get) {
					zval rv;
					val = Z_OBJ_HT_P(val)->get(val, &rv TSRMLS_CC);
					ZVAL_COPY_VALUE(&val_free, val);
					goto repeat;
				} else if (Z_OBJ_HT_P(val)->cast_object) {
					if (Z_OBJ_HT_P(val)->cast_object(val, &val_free, IS_STRING TSRMLS_CC) == SUCCESS) {
						val = &val_free;
						break;
					}
				}
			}
			/* no break */
		default:
			zend_error(E_WARNING,"Constants may only evaluate to scalar values");
			zval_ptr_dtor(&val_free);
			RETURN_FALSE;
	}
	
	ZVAL_DUP(&c.value, val);
	zval_ptr_dtor(&val_free);
	c.flags = case_sensitive; /* non persistent */
	c.name = STR_COPY(name);
	c.module_number = PHP_USER_CONSTANT;
	if (zend_register_constant(&c TSRMLS_CC) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto bool defined(string constant_name)
   Check whether a constant exists */
ZEND_FUNCTION(defined)
{
	zend_string *name;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &name) == FAILURE) {
		return;
	}
	
	if (zend_get_constant_ex(name, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto string get_class([object object])
   Retrieves the class name */
ZEND_FUNCTION(get_class)
{
	zval *obj = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|o!", &obj) == FAILURE) {
		RETURN_FALSE;
	}

	if (!obj) {
		if (EG(scope)) {
			RETURN_STR(STR_COPY(EG(scope)->name));
		} else {
			zend_error(E_WARNING, "get_class() called without object from outside a class");
			RETURN_FALSE;
		}
	}

	RETURN_STR(zend_get_object_classname(Z_OBJ_P(obj) TSRMLS_CC));
}
/* }}} */


/* {{{ proto string get_called_class()
   Retrieves the "Late Static Binding" class name */
ZEND_FUNCTION(get_called_class)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (EG(called_scope)) {
		RETURN_STR(STR_COPY(EG(called_scope)->name));
	} else if (!EG(scope))  {
		zend_error(E_WARNING, "get_called_class() called from outside a class");
	}
	RETURN_FALSE;
}
/* }}} */


/* {{{ proto string get_parent_class([mixed object])
   Retrieves the parent class name for object or class or current scope. */
ZEND_FUNCTION(get_parent_class)
{
	zval *arg;
	zend_class_entry *ce = NULL;
	zend_string *name;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &arg) == FAILURE) {
		return;
	}

	if (!ZEND_NUM_ARGS()) {
		ce = EG(scope);
		if (ce && ce->parent) {
			RETURN_STR(STR_COPY(ce->parent->name));
		} else {
			RETURN_FALSE;
		}
	}

	if (Z_TYPE_P(arg) == IS_OBJECT) {
		if (Z_OBJ_HT_P(arg)->get_class_name
			&& (name = Z_OBJ_HT_P(arg)->get_class_name(Z_OBJ_P(arg), 1 TSRMLS_CC)) != NULL) {
			RETURN_STR(name);
		} else {
			ce = zend_get_class_entry(Z_OBJ_P(arg) TSRMLS_CC);
		}
	} else if (Z_TYPE_P(arg) == IS_STRING) {
	    ce = zend_lookup_class(Z_STR_P(arg) TSRMLS_CC);
	}

	if (ce && ce->parent) {
		RETURN_STR(STR_COPY(ce->parent->name));
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


static void is_a_impl(INTERNAL_FUNCTION_PARAMETERS, zend_bool only_subclass)
{
	zval *obj;
	zend_string *class_name;
	zend_class_entry *instance_ce;
	zend_class_entry *ce;
	zend_bool allow_string = only_subclass;
	zend_bool retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zS|b", &obj, &class_name, &allow_string) == FAILURE) {
		return;
	}
	/*
	 * allow_string - is_a default is no, is_subclass_of is yes. 
	 *   if it's allowed, then the autoloader will be called if the class does not exist.
	 *   default behaviour is different, as 'is_a' used to be used to test mixed return values
	 *   and there is no easy way to deprecate this.
	 */

	if (allow_string && Z_TYPE_P(obj) == IS_STRING) {
		instance_ce = zend_lookup_class(Z_STR_P(obj) TSRMLS_CC);
		if (!instance_ce) {
			RETURN_FALSE;
		}
	} else if (Z_TYPE_P(obj) == IS_OBJECT && HAS_CLASS_ENTRY(*obj)) {
		instance_ce = Z_OBJCE_P(obj);
	} else {
		RETURN_FALSE;
	}

	ce = zend_lookup_class_ex(class_name, NULL, 0 TSRMLS_CC);
	if (!ce) {
		retval = 0;
	} else {
		if (only_subclass && instance_ce == ce) {
			retval = 0;
 		} else {
			retval = instanceof_function(instance_ce, ce TSRMLS_CC);
		}
	}

	RETURN_BOOL(retval);
}


/* {{{ proto bool is_subclass_of(mixed object_or_string, string class_name [, bool allow_string=true])
   Returns true if the object has this class as one of its parents */
ZEND_FUNCTION(is_subclass_of)
{
	is_a_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */


/* {{{ proto bool is_a(mixed object_or_string, string class_name [, bool allow_string=false])
   Returns true if the first argument is an object and is this class or has this class as one of its parents, */
ZEND_FUNCTION(is_a)
{
	is_a_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */


/* {{{ add_class_vars */
static void add_class_vars(zend_class_entry *ce, int statics, zval *return_value TSRMLS_DC)
{
	zend_property_info *prop_info;
	zval *prop, prop_copy;
	zend_string *key;

	ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->properties_info, key, prop_info) {
		if (((prop_info->flags & ZEND_ACC_SHADOW) &&
		     prop_info->ce != EG(scope)) ||
		    ((prop_info->flags & ZEND_ACC_PROTECTED) &&
		     !zend_check_protected(prop_info->ce, EG(scope))) ||
		    ((prop_info->flags & ZEND_ACC_PRIVATE) &&
		      ce != EG(scope) &&
			  prop_info->ce != EG(scope))) {
			continue;
		}
		prop = NULL;
		if (prop_info->offset >= 0) {
			if (statics && (prop_info->flags & ZEND_ACC_STATIC) != 0) {
				prop = &ce->default_static_members_table[prop_info->offset];
			} else if (!statics && (prop_info->flags & ZEND_ACC_STATIC) == 0) {
				prop = &ce->default_properties_table[prop_info->offset];
 			}
		}
		if (!prop || Z_TYPE_P(prop) == IS_UNDEF) {
			continue;
		}

		/* copy: enforce read only access */
		ZVAL_DUP_DEREF(&prop_copy, prop);

		/* this is necessary to make it able to work with default array
		 * properties, returned to user */
		if (Z_OPT_CONSTANT(prop_copy)) {
			zval_update_constant(&prop_copy, 0 TSRMLS_CC);
		}

		zend_hash_update(Z_ARRVAL_P(return_value), key, &prop_copy);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */


/* {{{ proto array get_class_vars(string class_name)
   Returns an array of default properties of the class. */
ZEND_FUNCTION(get_class_vars)
{
	zend_string *class_name;
	zend_class_entry *ce;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &class_name) == FAILURE) {
		return;
	}

	ce = zend_lookup_class(class_name TSRMLS_CC);
	if (!ce) {
		RETURN_FALSE;
	} else {
		array_init(return_value);
		zend_update_class_constants(ce TSRMLS_CC);
		add_class_vars(ce, 0, return_value TSRMLS_CC);
		add_class_vars(ce, 1, return_value TSRMLS_CC);
	}
}
/* }}} */


/* {{{ proto array get_object_vars(object obj)
   Returns an array of object properties */
ZEND_FUNCTION(get_object_vars)
{
	zval *obj;
	zval *value;
	HashTable *properties;
	zend_string *key;
	const char *prop_name, *class_name;
	uint prop_len;
	zend_object *zobj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &obj) == FAILURE) {
		return;
	}

	if (Z_OBJ_HT_P(obj)->get_properties == NULL) {
		RETURN_FALSE;
	}

	properties = Z_OBJ_HT_P(obj)->get_properties(obj TSRMLS_CC);

	if (properties == NULL) {
		RETURN_FALSE;
	}

	zobj = Z_OBJ_P(obj);

	array_init(return_value);

	ZEND_HASH_FOREACH_STR_KEY_VAL_IND(properties, key, value) {
		if (key) {
			if (zend_check_property_access(zobj, key TSRMLS_CC) == SUCCESS) {
				/* Not separating references */
				if (Z_REFCOUNTED_P(value)) Z_ADDREF_P(value);
				if (key->val[0] == 0) {
					zend_unmangle_property_name_ex(key->val, key->len, &class_name, &prop_name, (int*) &prop_len);
					zend_hash_str_update(Z_ARRVAL_P(return_value), prop_name, prop_len, value);
				} else {
					zend_hash_update(Z_ARRVAL_P(return_value), key, value);
				}
			}
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

static int same_name(const char *key, const char *name, zend_uint name_len)
{
	char *lcname = zend_str_tolower_dup(name, name_len);
	int ret = memcmp(lcname, key, name_len) == 0;
	efree(lcname);
	return ret;
}

/* {{{ proto array get_class_methods(mixed class)
   Returns an array of method names for class or class instance. */
ZEND_FUNCTION(get_class_methods)
{
	zval *klass;
	zval method_name;
	zend_class_entry *ce = NULL;
	zend_function *mptr;
	zend_string *key;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &klass) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(klass) == IS_OBJECT) {
		/* TBI!! new object handlers */
		if (!HAS_CLASS_ENTRY(*klass)) {
			RETURN_FALSE;
		}
		ce = Z_OBJCE_P(klass);
	} else if (Z_TYPE_P(klass) == IS_STRING) {
	    ce = zend_lookup_class(Z_STR_P(klass) TSRMLS_CC);
	}

	if (!ce) {
		RETURN_NULL();
	}

	array_init(return_value);

	ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->function_table, key, mptr) {

		if ((mptr->common.fn_flags & ZEND_ACC_PUBLIC) 
		 || (EG(scope) &&
		     (((mptr->common.fn_flags & ZEND_ACC_PROTECTED) &&
		       zend_check_protected(mptr->common.scope, EG(scope)))
		   || ((mptr->common.fn_flags & ZEND_ACC_PRIVATE) &&
		       EG(scope) == mptr->common.scope)))) {
			uint len = mptr->common.function_name->len;

			/* Do not display old-style inherited constructors */
			if (!key) {
// TODO: we have to duplicate it, becaise it may be stored in opcache SHM ???
				ZVAL_STR(&method_name, STR_DUP(mptr->common.function_name, 0));
				zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &method_name);
			} else if ((mptr->common.fn_flags & ZEND_ACC_CTOR) == 0 ||
			    mptr->common.scope == ce ||
			    zend_binary_strcasecmp(key->val, key->len, mptr->common.function_name->val, len) == 0) {

				if (mptr->type == ZEND_USER_FUNCTION &&
				    *mptr->op_array.refcount > 1 &&
			    	(len != key->len ||
			    	 !same_name(key->val, mptr->common.function_name->val, len))) {
					ZVAL_STR(&method_name, STR_COPY(zend_find_alias_name(mptr->common.scope, key)));
					zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &method_name);
				} else {
// TODO: we have to duplicate it, becaise it may be stored in opcache SHM ???
					ZVAL_STR(&method_name, STR_DUP(mptr->common.function_name, 0));
					zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &method_name);
				}
			}
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */


/* {{{ proto bool method_exists(object object, string method)
   Checks if the class method exists */
ZEND_FUNCTION(method_exists)
{
	zval *klass; 
	zend_string *method_name;
	zend_string *lcname;
	zend_class_entry * ce;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zS", &klass, &method_name) == FAILURE) {
		return;
	}
	if (Z_TYPE_P(klass) == IS_OBJECT) {
		ce = Z_OBJCE_P(klass);
	} else if (Z_TYPE_P(klass) == IS_STRING) {
		if ((ce = zend_lookup_class(Z_STR_P(klass) TSRMLS_CC)) == NULL) {
			RETURN_FALSE;
		}
	} else {
		RETURN_FALSE;
	}

	lcname = STR_ALLOC(method_name->len, 0);
	zend_str_tolower_copy(lcname->val, method_name->val, method_name->len);
	if (zend_hash_exists(&ce->function_table, lcname)) {
		STR_FREE(lcname);
		RETURN_TRUE;
	} else {
		union _zend_function *func = NULL;

		if (Z_TYPE_P(klass) == IS_OBJECT 
		&& Z_OBJ_HT_P(klass)->get_method != NULL
		&& (func = Z_OBJ_HT_P(klass)->get_method(&Z_OBJ_P(klass), method_name, NULL TSRMLS_CC)) != NULL
		) {
			if (func->type == ZEND_INTERNAL_FUNCTION 
			&& (func->common.fn_flags & ZEND_ACC_CALL_VIA_HANDLER) != 0
			) {
				/* Returns true to the fake Closure's __invoke */
				RETVAL_BOOL((func->common.scope == zend_ce_closure
					&& (method_name->len == sizeof(ZEND_INVOKE_FUNC_NAME)-1)
					&& memcmp(lcname->val, ZEND_INVOKE_FUNC_NAME, sizeof(ZEND_INVOKE_FUNC_NAME)-1) == 0) ? 1 : 0);
					
				STR_FREE(lcname);
				STR_RELEASE(func->common.function_name);
				efree(func);
				return;
			}
			efree(lcname);
			RETURN_TRUE;
		}
	}
	efree(lcname);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool property_exists(mixed object_or_class, string property_name)
   Checks if the object or class has a property */
ZEND_FUNCTION(property_exists)
{
	zval *object;
	zend_string *property;
	zend_class_entry *ce;
	zend_property_info *property_info;
	zval property_z;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zS", &object, &property) == FAILURE) {
		return;
	}

	if (property == NULL) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(object) == IS_STRING) {
		ce = zend_lookup_class(Z_STR_P(object) TSRMLS_CC);
		if (!ce) {
			RETURN_FALSE;
		}			
	} else if (Z_TYPE_P(object) == IS_OBJECT) {
		ce = Z_OBJCE_P(object);
	} else {
		zend_error(E_WARNING, "First parameter must either be an object or the name of an existing class");
		RETURN_NULL();
	}

	if ((property_info = zend_hash_find_ptr(&ce->properties_info, property)) != NULL
		&& (property_info->flags & ZEND_ACC_SHADOW) == 0) {
		RETURN_TRUE;
	}

	ZVAL_STR(&property_z, property);

	if (Z_TYPE_P(object) ==  IS_OBJECT &&
		Z_OBJ_HANDLER_P(object, has_property) && 
		Z_OBJ_HANDLER_P(object, has_property)(object, &property_z, 2, -1 TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */


/* {{{ proto bool class_exists(string classname [, bool autoload])
   Checks if the class exists */
ZEND_FUNCTION(class_exists)
{
	zend_string *class_name;
	zend_string *lc_name;
	zend_class_entry *ce;
	zend_bool autoload = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|b", &class_name, &autoload) == FAILURE) {
		return;
	}

	if (!autoload) {
		if (class_name->val[0] == '\\') {
			/* Ignore leading "\" */
			lc_name = STR_ALLOC(class_name->len - 1, 0);
			zend_str_tolower_copy(lc_name->val, class_name->val + 1, class_name->len - 1);
		} else {
			lc_name = STR_ALLOC(class_name->len, 0);
			zend_str_tolower_copy(lc_name->val, class_name->val, class_name->len);
		}
		ce = zend_hash_find_ptr(EG(class_table), lc_name);
		STR_FREE(lc_name);
		RETURN_BOOL(ce && !((ce->ce_flags & (ZEND_ACC_INTERFACE | ZEND_ACC_TRAIT)) > ZEND_ACC_EXPLICIT_ABSTRACT_CLASS));
	}

	ce = zend_lookup_class(class_name TSRMLS_CC);
 	if (ce) {
 		RETURN_BOOL((ce->ce_flags & (ZEND_ACC_INTERFACE | (ZEND_ACC_TRAIT - ZEND_ACC_EXPLICIT_ABSTRACT_CLASS))) == 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool interface_exists(string classname [, bool autoload])
   Checks if the class exists */
ZEND_FUNCTION(interface_exists)
{
	zend_string *iface_name, *lc_name;
	zend_class_entry *ce;
	zend_bool autoload = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|b", &iface_name, &autoload) == FAILURE) {
		return;
	}

	if (!autoload) {
		if (iface_name->val[0] == '\\') {
			/* Ignore leading "\" */
			lc_name = STR_ALLOC(iface_name->len - 1, 0);
			zend_str_tolower_copy(lc_name->val, iface_name->val + 1, iface_name->len - 1);
		} else {
			lc_name = STR_ALLOC(iface_name->len, 0);
			zend_str_tolower_copy(lc_name->val, iface_name->val, iface_name->len);
		}
		ce = zend_hash_find_ptr(EG(class_table), lc_name);
		STR_FREE(lc_name);
		RETURN_BOOL(ce && ce->ce_flags & ZEND_ACC_INTERFACE);
	}

	ce = zend_lookup_class(iface_name TSRMLS_CC);
	if (ce) {
 		RETURN_BOOL((ce->ce_flags & ZEND_ACC_INTERFACE) > 0);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool trait_exists(string traitname [, bool autoload])
 Checks if the trait exists */
ZEND_FUNCTION(trait_exists)
{
	zend_string *trait_name, *lc_name;
	zend_class_entry *ce;
	zend_bool autoload = 1;
  
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S|b", &trait_name, &autoload) == FAILURE) {
		return;
	}
  
	if (!autoload) {
		if (trait_name->val[0] == '\\') {
			/* Ignore leading "\" */
			lc_name = STR_ALLOC(trait_name->len - 1, 0);
			zend_str_tolower_copy(lc_name->val, trait_name->val + 1, trait_name->len - 1);
		} else {
			lc_name = STR_ALLOC(trait_name->len, 0);
			zend_str_tolower_copy(lc_name->val, trait_name->val, trait_name->len);
		}
		ce = zend_hash_find_ptr(EG(class_table), lc_name);
		STR_FREE(lc_name);
		RETURN_BOOL(ce && ((ce->ce_flags & ZEND_ACC_TRAIT) > ZEND_ACC_EXPLICIT_ABSTRACT_CLASS));
	}
  
	ce = zend_lookup_class(trait_name TSRMLS_CC);
	if (ce) {
 		RETURN_BOOL((ce->ce_flags & ZEND_ACC_TRAIT) > ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto bool function_exists(string function_name) 
   Checks if the function exists */
ZEND_FUNCTION(function_exists)
{
	char *name;
	int name_len;
	zend_function *func;
	zend_string *lcname;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	if (name[0] == '\\') {
		/* Ignore leading "\" */
		lcname = STR_ALLOC(name_len - 1, 0);
		zend_str_tolower_copy(lcname->val, name + 1, name_len - 1);
	} else {
		lcname = STR_ALLOC(name_len, 0);
		zend_str_tolower_copy(lcname->val, name, name_len);
	}
	
	func = zend_hash_find_ptr(EG(function_table), lcname);	
	STR_FREE(lcname);

	/*
	 * A bit of a hack, but not a bad one: we see if the handler of the function
	 * is actually one that displays "function is disabled" message.
	 */
	RETURN_BOOL(func && (func->type != ZEND_INTERNAL_FUNCTION ||
		func->internal_function.handler != zif_display_disabled_function));
}
/* }}} */

/* {{{ proto bool class_alias(string user_class_name , string alias_name [, bool autoload])
   Creates an alias for user defined class */
ZEND_FUNCTION(class_alias)
{
	zend_string *class_name;
	char *alias_name;
	zend_class_entry *ce;
	int alias_name_len;
	zend_bool autoload = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ss|b", &class_name, &alias_name, &alias_name_len, &autoload) == FAILURE) {
		return;
	}

	ce = zend_lookup_class_ex(class_name, NULL, autoload TSRMLS_CC);
	
	if (ce) {
		if (ce->type == ZEND_USER_CLASS) { 
			if (zend_register_class_alias_ex(alias_name, alias_name_len, ce TSRMLS_CC) == SUCCESS) {
				RETURN_TRUE;
			} else {
				zend_error(E_WARNING, "Cannot redeclare class %s", alias_name);
				RETURN_FALSE;
			}
		} else {
			zend_error(E_WARNING, "First argument of class_alias() must be a name of user defined class");
			RETURN_FALSE;
		}
	} else {
		zend_error(E_WARNING, "Class '%s' not found", class_name->val);
		RETURN_FALSE;
	}
}
/* }}} */

#if ZEND_DEBUG
/* {{{ proto void leak(int num_bytes=3)
   Cause an intentional memory leak, for testing/debugging purposes */
ZEND_FUNCTION(leak)
{
	long leakbytes=3;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &leakbytes) == FAILURE) {
		return;
	}

	emalloc(leakbytes);
}
/* }}} */

/* {{{ proto leak_variable(mixed variable [, bool leak_data]) */
ZEND_FUNCTION(leak_variable)
{
	zval *zv;
	zend_bool leak_data = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &zv, &leak_data) == FAILURE) {
		return;
	}

	if (!leak_data) {
		Z_ADDREF_P(zv);
	} else if (Z_TYPE_P(zv) == IS_RESOURCE) {
		Z_ADDREF_P(zv);
	} else if (Z_TYPE_P(zv) == IS_OBJECT) {
		Z_ADDREF_P(zv);
	} else {
		zend_error(E_WARNING, "Leaking non-zval data is only applicable to resources and objects");
	}
}
/* }}} */


#ifdef ZEND_TEST_EXCEPTIONS
ZEND_FUNCTION(crash)
{
	char *nowhere=NULL;

	memcpy(nowhere, "something", sizeof("something"));
}
#endif

#endif /* ZEND_DEBUG */

/* {{{ proto array get_included_files(void)
   Returns an array with the file names that were include_once()'d */
ZEND_FUNCTION(get_included_files)
{
	zend_string *entry;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	ZEND_HASH_FOREACH_STR_KEY(&EG(included_files), entry) {
		if (entry) {
			add_next_index_str(return_value, STR_COPY(entry));
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */


/* {{{ proto void trigger_error(string message [, int error_type])
   Generates a user-level error/warning/notice message */
ZEND_FUNCTION(trigger_error)
{
	long error_type = E_USER_NOTICE;
	char *message;
	int message_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &message, &message_len, &error_type) == FAILURE) {
		return;
	}

	switch (error_type) {
		case E_USER_ERROR:
		case E_USER_WARNING:
		case E_USER_NOTICE:
		case E_USER_DEPRECATED:
			break;
		default:
			zend_error(E_WARNING, "Invalid error type specified");
			RETURN_FALSE;
			break;
	}

	zend_error((int)error_type, "%s", message);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto string set_error_handler(string error_handler [, int error_types])
   Sets a user-defined error handler function.  Returns the previously defined error handler, or false on error */
ZEND_FUNCTION(set_error_handler)
{
	zval *error_handler;
	zend_string *error_handler_name = NULL;
	long error_type = E_ALL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &error_handler, &error_type) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(error_handler) != IS_NULL) { /* NULL == unset */
		if (!zend_is_callable(error_handler, 0, &error_handler_name TSRMLS_CC)) {
			zend_error(E_WARNING, "%s() expects the argument (%s) to be a valid callback",
					   get_active_function_name(TSRMLS_C), error_handler_name?error_handler_name->val:"unknown");
			STR_RELEASE(error_handler_name);
			return;
		}
		STR_RELEASE(error_handler_name);
	}

	if (Z_TYPE(EG(user_error_handler)) != IS_UNDEF) {
		RETVAL_ZVAL(&EG(user_error_handler), 1, 0);

		zend_stack_push(&EG(user_error_handlers_error_reporting), &EG(user_error_handler_error_reporting), sizeof(EG(user_error_handler_error_reporting)));
		zend_stack_push(&EG(user_error_handlers), &EG(user_error_handler), sizeof(zval));
	}

	if (Z_TYPE_P(error_handler) == IS_NULL) { /* unset user-defined handler */
		ZVAL_UNDEF(&EG(user_error_handler));
		return;
	}

	ZVAL_DUP(&EG(user_error_handler), error_handler);
	EG(user_error_handler_error_reporting) = (int)error_type;
}
/* }}} */


/* {{{ proto void restore_error_handler(void)
   Restores the previously defined error handler function */
ZEND_FUNCTION(restore_error_handler)
{
	if (Z_TYPE(EG(user_error_handler)) != IS_UNDEF) {
		zval zeh;
		
		ZVAL_COPY_VALUE(&zeh, &EG(user_error_handler));
		ZVAL_UNDEF(&EG(user_error_handler));
		zval_ptr_dtor(&zeh);
	}

	if (zend_stack_is_empty(&EG(user_error_handlers))) {
		ZVAL_UNDEF(&EG(user_error_handler));
	} else {
		zval *tmp;
		EG(user_error_handler_error_reporting) = zend_stack_int_top(&EG(user_error_handlers_error_reporting));
		zend_stack_del_top(&EG(user_error_handlers_error_reporting));
		zend_stack_top(&EG(user_error_handlers), (void**)&tmp);
		ZVAL_COPY_VALUE(&EG(user_error_handler), tmp);
		zend_stack_del_top(&EG(user_error_handlers));
	}
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto string set_exception_handler(callable exception_handler)
   Sets a user-defined exception handler function.  Returns the previously defined exception handler, or false on error */
ZEND_FUNCTION(set_exception_handler)
{
	zval *exception_handler;
	zend_string *exception_handler_name = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &exception_handler) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(exception_handler) != IS_NULL) { /* NULL == unset */
		if (!zend_is_callable(exception_handler, 0, &exception_handler_name TSRMLS_CC)) {
			zend_error(E_WARNING, "%s() expects the argument (%s) to be a valid callback",
					   get_active_function_name(TSRMLS_C), exception_handler_name?exception_handler_name->val:"unknown");
			STR_RELEASE(exception_handler_name);
			return;
		}
		STR_RELEASE(exception_handler_name);
	}

	if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
		RETVAL_ZVAL(&EG(user_exception_handler), 1, 0);

		zend_stack_push(&EG(user_exception_handlers), &EG(user_exception_handler), sizeof(zval));
	}

	if (Z_TYPE_P(exception_handler) == IS_NULL) { /* unset user-defined handler */
		ZVAL_UNDEF(&EG(user_exception_handler));
		return;
	}

	ZVAL_DUP(&EG(user_exception_handler), exception_handler);
}
/* }}} */


/* {{{ proto void restore_exception_handler(void)
   Restores the previously defined exception handler function */
ZEND_FUNCTION(restore_exception_handler)
{
	if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
		zval_ptr_dtor(&EG(user_exception_handler));
	}
	if (zend_stack_is_empty(&EG(user_exception_handlers))) {
		ZVAL_UNDEF(&EG(user_exception_handler));
	} else {
		zval *tmp;

		zend_stack_top(&EG(user_exception_handlers), (void**)&tmp);
		ZVAL_COPY_VALUE(&EG(user_exception_handler), tmp);
		zend_stack_del_top(&EG(user_exception_handlers));
	}
	RETURN_TRUE;
}
/* }}} */

static int copy_class_or_interface_name(zend_class_entry **pce TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	zval *array = va_arg(args, zval *);
	zend_uint mask = va_arg(args, zend_uint);
	zend_uint comply = va_arg(args, zend_uint);
	zend_uint comply_mask = (comply)? mask:0;
	zend_class_entry *ce  = *pce;

	if ((hash_key->key && hash_key->key->val[0] != 0)
		&& (comply_mask == (ce->ce_flags & mask))) {
		if (ce->refcount > 1 && 
		    (ce->name->len != hash_key->key->len - 1 || 
		     !same_name(hash_key->key->val, ce->name->val, ce->name->len))) {
			add_next_index_str(array, STR_COPY(hash_key->key));
		} else {
			add_next_index_str(array, STR_COPY(ce->name));
		}
	}
	return ZEND_HASH_APPLY_KEEP;
}

/* {{{ proto array get_declared_traits()
   Returns an array of all declared traits. */
ZEND_FUNCTION(get_declared_traits)
{
	zend_uint mask = ZEND_ACC_TRAIT;
	zend_uint comply = 1;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	zend_hash_apply_with_arguments(EG(class_table) TSRMLS_CC, (apply_func_args_t) copy_class_or_interface_name, 3, return_value, mask, comply);
}
/* }}} */


/* {{{ proto array get_declared_classes()
   Returns an array of all declared classes. */
ZEND_FUNCTION(get_declared_classes)
{
	zend_uint mask = ZEND_ACC_INTERFACE | (ZEND_ACC_TRAIT & ~ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);
	zend_uint comply = 0;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	zend_hash_apply_with_arguments(EG(class_table) TSRMLS_CC, (apply_func_args_t) copy_class_or_interface_name, 3, return_value, mask, comply);
}
/* }}} */

/* {{{ proto array get_declared_interfaces()
   Returns an array of all declared interfaces. */
ZEND_FUNCTION(get_declared_interfaces)
{
	zend_uint mask = ZEND_ACC_INTERFACE;
	zend_uint comply = 1;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(return_value);
	zend_hash_apply_with_arguments(EG(class_table) TSRMLS_CC, (apply_func_args_t) copy_class_or_interface_name, 3, return_value, mask, comply);
}
/* }}} */


static int copy_function_name(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
	zend_function *func = Z_PTR_P(zv);
	zval *internal_ar = va_arg(args, zval *),
	     *user_ar     = va_arg(args, zval *);

	if (hash_key->key == NULL || hash_key->key->val[0] == 0) {
		return 0;
	}

	if (func->type == ZEND_INTERNAL_FUNCTION) {
		add_next_index_str(internal_ar, STR_COPY(hash_key->key));
	} else if (func->type == ZEND_USER_FUNCTION) {
		add_next_index_str(user_ar, STR_COPY(hash_key->key));
	}

	return 0;
}


/* {{{ proto array get_defined_functions(void)
   Returns an array of all defined functions */
ZEND_FUNCTION(get_defined_functions)
{
	zval internal, user, *ret;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	array_init(&internal);
	array_init(&user);
	array_init(return_value);

	zend_hash_apply_with_arguments(EG(function_table) TSRMLS_CC, (apply_func_args_t) copy_function_name, 2, &internal, &user);

	ret = zend_hash_str_add(Z_ARRVAL_P(return_value), "internal", sizeof("internal")-1, &internal);

	if (!ret) {
		zval_ptr_dtor(&internal);
		zval_ptr_dtor(&user);
		zval_dtor(return_value);
		zend_error(E_WARNING, "Cannot add internal functions to return value from get_defined_functions()");
		RETURN_FALSE;
	}

	ret = zend_hash_str_add(Z_ARRVAL_P(return_value), "user", sizeof("user")-1, &user);
	if (!ret) {		
		zval_ptr_dtor(&user);
		zval_dtor(return_value);
		zend_error(E_WARNING, "Cannot add user functions to return value from get_defined_functions()");
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto array get_defined_vars(void)
   Returns an associative array of names and values of all currently defined variable names (variables in the current scope) */
ZEND_FUNCTION(get_defined_vars)
{
	if (!EG(active_symbol_table)) {
		zend_rebuild_symbol_table(TSRMLS_C);
	}

	array_init_size(return_value, zend_hash_num_elements(&EG(active_symbol_table)->ht));

	zend_hash_copy(Z_ARRVAL_P(return_value), &EG(active_symbol_table)->ht, zval_add_ref);
}
/* }}} */


#define LAMBDA_TEMP_FUNCNAME	"__lambda_func"
/* {{{ proto string create_function(string args, string code)
   Creates an anonymous function, and returns its name (funny, eh?) */
ZEND_FUNCTION(create_function)
{
    zend_string *function_name;
	char *eval_code, *function_args, *function_code;
	int eval_code_length, function_args_len, function_code_len;
	int retval;
	char *eval_name;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &function_args, &function_args_len, &function_code, &function_code_len) == FAILURE) {
		return;
	}

	eval_code = (char *) emalloc(sizeof("function " LAMBDA_TEMP_FUNCNAME)
			+function_args_len
			+2	/* for the args parentheses */
			+2	/* for the curly braces */
			+function_code_len);

	eval_code_length = sizeof("function " LAMBDA_TEMP_FUNCNAME "(") - 1;
	memcpy(eval_code, "function " LAMBDA_TEMP_FUNCNAME "(", eval_code_length);

	memcpy(eval_code + eval_code_length, function_args, function_args_len);
	eval_code_length += function_args_len;

	eval_code[eval_code_length++] = ')';
	eval_code[eval_code_length++] = '{';

	memcpy(eval_code + eval_code_length, function_code, function_code_len);
	eval_code_length += function_code_len;

	eval_code[eval_code_length++] = '}';
	eval_code[eval_code_length] = '\0';

	eval_name = zend_make_compiled_string_description("runtime-created function" TSRMLS_CC);
	retval = zend_eval_stringl(eval_code, eval_code_length, NULL, eval_name TSRMLS_CC);
	efree(eval_code);
	efree(eval_name);

	if (retval==SUCCESS) {
		zend_op_array *new_function, *func;

		func = zend_hash_str_find_ptr(EG(function_table), LAMBDA_TEMP_FUNCNAME, sizeof(LAMBDA_TEMP_FUNCNAME)-1);
		if (!func) {
			zend_error(E_ERROR, "Unexpected inconsistency in create_function()");
			RETURN_FALSE;
		}
		new_function = emalloc(sizeof(zend_op_array));
		memcpy(new_function, func, sizeof(zend_op_array));
		function_add_ref((zend_function*)new_function);

		function_name = STR_ALLOC(sizeof("0lambda_")+MAX_LENGTH_OF_LONG, 0);
		function_name->val[0] = '\0';

		do {
			function_name->len = snprintf(function_name->val + 1, sizeof("lambda_")+MAX_LENGTH_OF_LONG, "lambda_%d", ++EG(lambda_count)) + 1;
		} while (zend_hash_add_ptr(EG(function_table), function_name, new_function) == NULL);
		zend_hash_str_del(EG(function_table), LAMBDA_TEMP_FUNCNAME, sizeof(LAMBDA_TEMP_FUNCNAME)-1);
		RETURN_STR(function_name);
	} else {
		zend_hash_str_del(EG(function_table), LAMBDA_TEMP_FUNCNAME, sizeof(LAMBDA_TEMP_FUNCNAME)-1);
		RETURN_FALSE;
	}
}
/* }}} */


#if ZEND_DEBUG
ZEND_FUNCTION(zend_test_func)
{
	zval *arg1, *arg2;

	zend_get_parameters(ZEND_NUM_ARGS(), 2, &arg1, &arg2);
}


#ifdef ZTS
ZEND_FUNCTION(zend_thread_id)
{
	RETURN_LONG((long)tsrm_thread_id());
}
#endif
#endif

/* {{{ proto string get_resource_type(resource res)
   Get the resource type name for a given resource */
ZEND_FUNCTION(get_resource_type)
{
	const char *resource_type;
	zval *z_resource_type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &z_resource_type) == FAILURE) {
		return;
	}

	resource_type = zend_rsrc_list_get_rsrc_type(Z_RES_P(z_resource_type) TSRMLS_CC);
	if (resource_type) {
		RETURN_STRING(resource_type);
	} else {
		RETURN_STRING("Unknown");
	}
}
/* }}} */


static int add_extension_info(zval *item, void *arg TSRMLS_DC)
{
	zval *name_array = (zval *)arg;
	zend_module_entry *module = (zend_module_entry*)Z_PTR_P(item);
	add_next_index_string(name_array, module->name);
	return 0;
}

static int add_zendext_info(zend_extension *ext, void *arg TSRMLS_DC)
{
	zval *name_array = (zval *)arg;
	add_next_index_string(name_array, ext->name);
	return 0;
}

static int add_constant_info(zval *item, void *arg TSRMLS_DC)
{
	zval *name_array = (zval *)arg;
	zend_constant *constant = (zend_constant*)Z_PTR_P(item);
	zval const_val;

	if (!constant->name) {
		/* skip special constants */
		return 0;
	}

	ZVAL_DUP(&const_val, &constant->value);
	zend_hash_update(Z_ARRVAL_P(name_array), constant->name, &const_val);
	return 0;
}


/* {{{ proto array get_loaded_extensions([bool zend_extensions]) U
   Return an array containing names of loaded extensions */
ZEND_FUNCTION(get_loaded_extensions)
{
	zend_bool zendext = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &zendext) == FAILURE) {
		return;
	}

	array_init(return_value);

	if (zendext) {
		zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t)add_zendext_info, return_value TSRMLS_CC);
	} else {
		zend_hash_apply_with_argument(&module_registry, (apply_func_arg_t)add_extension_info, return_value TSRMLS_CC);
	}
}
/* }}} */


/* {{{ proto array get_defined_constants([bool categorize])
   Return an array containing the names and values of all defined constants */
ZEND_FUNCTION(get_defined_constants)
{
	zend_bool categorize = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &categorize) == FAILURE) {
		return;
	}

	array_init(return_value);

	if (categorize) {
		zend_constant *val;
		int module_number;
		zval *modules;
		char **module_names;
		zend_module_entry *module;
		int i = 1;

		modules = ecalloc(zend_hash_num_elements(&module_registry) + 2, sizeof(zval));
		module_names = emalloc((zend_hash_num_elements(&module_registry) + 2) * sizeof(char *));

		module_names[0] = "internal";
		ZEND_HASH_FOREACH_PTR(&module_registry, module) {
			module_names[module->module_number] = (char *)module->name;
			i++;
		} ZEND_HASH_FOREACH_END();
		module_names[i] = "user";

		ZEND_HASH_FOREACH_PTR(EG(zend_constants), val) {
			zval const_val;

			if (!val->name) {
				/* skip special constants */
				continue;
			}

			if (val->module_number == PHP_USER_CONSTANT) {
				module_number = i;
			} else if (val->module_number > i || val->module_number < 0) {
				/* should not happen */
				continue;
			} else {
				module_number = val->module_number;
			}

			if (Z_TYPE(modules[module_number]) == IS_UNDEF) {
				array_init(&modules[module_number]);
				add_assoc_zval(return_value, module_names[module_number], &modules[module_number]);
			}

			ZVAL_DUP_DEREF(&const_val, &val->value);

			zend_hash_update(Z_ARRVAL(modules[module_number]), val->name, &const_val);
		} ZEND_HASH_FOREACH_END();

		efree(module_names);
		efree(modules);
	} else {
		zend_hash_apply_with_argument(EG(zend_constants), (apply_func_arg_t)add_constant_info, return_value TSRMLS_CC);
	}
}
/* }}} */


static void debug_backtrace_get_args(zval *curpos, zval *arg_array TSRMLS_DC)
{
	zval *p = curpos;
	zval *arg;
	int arg_count = Z_LVAL_P(p);

	array_init_size(arg_array, arg_count);
	p -= arg_count;

	while (--arg_count >= 0) {
		arg = p++;
		if (arg) {
			if (Z_REFCOUNTED_P(arg)) Z_ADDREF_P(arg);
			add_next_index_zval(arg_array, arg);
		} else {
			add_next_index_null(arg_array);
		}
	}
}

void debug_print_backtrace_args(zval *arg_array TSRMLS_DC)
{
	zval *tmp;
	int i = 0;

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(arg_array), tmp) {
		if (i++) {
			ZEND_PUTS(", ");
		}
		zend_print_flat_zval_r(tmp TSRMLS_CC);
	} ZEND_HASH_FOREACH_END();
}

/* {{{ proto void debug_print_backtrace([int options[, int limit]]) */
ZEND_FUNCTION(debug_print_backtrace)
{
	zend_execute_data *ptr, *skip;
	zend_object *object;
	int lineno, frameno = 0;
	const char *function_name;
	const char *filename;
	zend_string *class_name = NULL;
	char *call_type;
	const char *include_filename = NULL;
	zval arg_array;
	int indent = 0;
	long options = 0;
	long limit = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &options, &limit) == FAILURE) {
		return;
	}

	ZVAL_UNDEF(&arg_array);
	ptr = EG(current_execute_data);

	/* skip debug_backtrace() */
	object = ptr->object;
	ptr = ptr->prev_execute_data;

	while (ptr && (limit == 0 || frameno < limit)) {
		frameno++;
		class_name = NULL;
		call_type = NULL;   
		ZVAL_UNDEF(&arg_array);

		skip = ptr;
		/* skip internal handler */
		if (!skip->op_array &&
		    skip->prev_execute_data &&
		    skip->prev_execute_data->opline &&
		    skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL &&
		    skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL_BY_NAME &&
		    skip->prev_execute_data->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
			skip = skip->prev_execute_data;
		}

		if (skip->op_array) {
			filename = skip->op_array->filename->val;
			lineno = skip->opline->lineno;
		} else {
			filename = NULL;
			lineno = 0;
		}

		/* $this may be passed into regular internal functions */
		if (object &&
		    ptr->function_state.function->type == ZEND_INTERNAL_FUNCTION &&
		    !ptr->function_state.function->common.scope) {
			object = NULL;
		}

		function_name = (ptr->function_state.function->common.scope &&
			ptr->function_state.function->common.scope->trait_aliases) ?
				zend_resolve_method_name(
					object ?
						zend_get_class_entry(object TSRMLS_CC) : 
						ptr->function_state.function->common.scope,
					ptr->function_state.function)->val :
				(ptr->function_state.function->common.function_name ?
				 ptr->function_state.function->common.function_name->val :
				 NULL);

		if (function_name) {
			if (object) {
				if (ptr->function_state.function->common.scope) {
					class_name = ptr->function_state.function->common.scope->name;
				} else {
					class_name = zend_get_object_classname(object TSRMLS_CC);
				}

				call_type = "->";
			} else if (ptr->function_state.function->common.scope) {
				class_name = ptr->function_state.function->common.scope->name;
				call_type = "::";
			} else {
				class_name = NULL;
				call_type = NULL;
			}
			if ((! ptr->opline) || ((ptr->opline->opcode == ZEND_DO_FCALL_BY_NAME) || (ptr->opline->opcode == ZEND_DO_FCALL))) {
				if (ptr->function_state.arguments && (options & DEBUG_BACKTRACE_IGNORE_ARGS) == 0) {
					debug_backtrace_get_args(ptr->function_state.arguments, &arg_array TSRMLS_CC);
				}
			}
		} else {
			/* i know this is kinda ugly, but i'm trying to avoid extra cycles in the main execution loop */
			zend_bool build_filename_arg = 1;

			if (!ptr->opline || ptr->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
				/* can happen when calling eval from a custom sapi */
				function_name = "unknown";
				build_filename_arg = 0;
			} else
			switch (ptr->opline->extended_value) {
				case ZEND_EVAL:
					function_name = "eval";
					build_filename_arg = 0;
					break;
				case ZEND_INCLUDE:
					function_name = "include";
					break;
				case ZEND_REQUIRE:
					function_name = "require";
					break;
				case ZEND_INCLUDE_ONCE:
					function_name = "include_once";
					break;
				case ZEND_REQUIRE_ONCE:
					function_name = "require_once";
					break;
				default:
					/* this can actually happen if you use debug_backtrace() in your error_handler and 
					 * you're in the top-scope */
					function_name = "unknown"; 
					build_filename_arg = 0;
					break;
			}

			if (build_filename_arg && include_filename) {
				array_init(&arg_array);
				add_next_index_string(&arg_array, (char*)include_filename);
			}
			call_type = NULL;
		}
		zend_printf("#%-2d ", indent);
		if (class_name) {
			ZEND_PUTS(class_name->val);
			ZEND_PUTS(call_type);
		}
		zend_printf("%s(", function_name);
		if (Z_TYPE(arg_array) != IS_UNDEF) {
			debug_print_backtrace_args(&arg_array TSRMLS_CC);
			zval_ptr_dtor(&arg_array);
		}
		if (filename) {
			zend_printf(") called at [%s:%d]\n", filename, lineno);
		} else {
			zend_execute_data *prev = skip->prev_execute_data;

			while (prev) {
				if (prev->function_state.function &&
					prev->function_state.function->common.type != ZEND_USER_FUNCTION) {
					prev = NULL;
					break;
				}				    
				if (prev->op_array) {
					zend_printf(") called at [%s:%d]\n", prev->op_array->filename->val, prev->opline->lineno);
					break;
				}
				prev = prev->prev_execute_data;
			}
			if (!prev) {
				ZEND_PUTS(")\n");
			}
		}
		include_filename = filename;
		object = skip->object;
		ptr = skip->prev_execute_data;
		++indent;
	}
}

/* }}} */

ZEND_API void zend_fetch_debug_backtrace(zval *return_value, int skip_last, int options, int limit TSRMLS_DC)
{
	zend_execute_data *ptr, *skip;
	zend_object *object = Z_OBJ(EG(This));
	int lineno, frameno = 0;
	const char *function_name;
	const char *filename;
	zend_string *class_name;
	const char *include_filename = NULL;
	zval stack_frame;

	ptr = EG(current_execute_data);

	/* skip "new Exception()" */
	if (ptr && (skip_last == 0) && ptr->opline && (ptr->opline->opcode == ZEND_NEW)) {
		object = ptr->object;
		ptr = ptr->prev_execute_data;
	}

	/* skip debug_backtrace() */
	if (skip_last-- && ptr) {
		object = ptr->object;
		ptr = ptr->prev_execute_data;
	}

	array_init(return_value);

	while (ptr && (limit == 0 || frameno < limit)) {
		frameno++;
		array_init(&stack_frame);

		skip = ptr;
		/* skip internal handler */
		if (!skip->op_array &&
		    skip->prev_execute_data &&
		    skip->prev_execute_data->opline &&
		    skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL &&
		    skip->prev_execute_data->opline->opcode != ZEND_DO_FCALL_BY_NAME &&
		    skip->prev_execute_data->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
			skip = skip->prev_execute_data;
		}

		if (skip->op_array) {
			filename = skip->op_array->filename->val;
			lineno = skip->opline->lineno;
			add_assoc_string_ex(&stack_frame, "file", sizeof("file")-1, (char*)filename);
			add_assoc_long_ex(&stack_frame, "line", sizeof("line")-1, lineno);

			/* try to fetch args only if an FCALL was just made - elsewise we're in the middle of a function
			 * and debug_baktrace() might have been called by the error_handler. in this case we don't 
			 * want to pop anything of the argument-stack */
		} else {
			zend_execute_data *prev = skip->prev_execute_data;

			while (prev) {
				if (prev->function_state.function &&
					prev->function_state.function->common.type != ZEND_USER_FUNCTION &&
					!(prev->function_state.function->common.type == ZEND_INTERNAL_FUNCTION &&
						(prev->function_state.function->common.fn_flags & ZEND_ACC_CALL_VIA_HANDLER))) {
					break;
				}				    
				if (prev->op_array) {
// TODO: we have to duplicate it, becaise it may be stored in opcache SHM ???
					add_assoc_str_ex(&stack_frame, "file", sizeof("file")-1, STR_DUP(prev->op_array->filename, 0));
					add_assoc_long_ex(&stack_frame, "line", sizeof("line")-1, prev->opline->lineno);
					break;
				}
				prev = prev->prev_execute_data;
			}
			filename = NULL;
		}

		/* $this may be passed into regular internal functions */
		if (object &&
		    ptr->function_state.function->type == ZEND_INTERNAL_FUNCTION &&
		    !ptr->function_state.function->common.scope) {
			object = NULL;
		}

		function_name = (ptr->function_state.function->common.scope &&
			ptr->function_state.function->common.scope->trait_aliases) ?
				zend_resolve_method_name(
					object ?
						zend_get_class_entry(object TSRMLS_CC) : 
						ptr->function_state.function->common.scope,
					ptr->function_state.function)->val :
				(ptr->function_state.function->common.function_name ?
				 ptr->function_state.function->common.function_name->val : 
				 NULL);

		if (function_name) {
			add_assoc_string_ex(&stack_frame, "function", sizeof("function")-1, (char*)function_name);

			if (object) {
				if (ptr->function_state.function->common.scope) {
					add_assoc_str_ex(&stack_frame, "class", sizeof("class")-1, STR_COPY(ptr->function_state.function->common.scope->name));
				} else {
					class_name = zend_get_object_classname(object TSRMLS_CC);
					add_assoc_str_ex(&stack_frame, "class", sizeof("class")-1, STR_COPY(class_name));
					
				}
				if ((options & DEBUG_BACKTRACE_PROVIDE_OBJECT) != 0) {
					zval zv;
					ZVAL_OBJ(&zv, object);
					add_assoc_zval_ex(&stack_frame, "object", sizeof("object")-1, &zv);
					Z_ADDREF(zv);
				}

				add_assoc_string_ex(&stack_frame, "type", sizeof("type")-1, "->");
			} else if (ptr->function_state.function->common.scope) {
				add_assoc_str_ex(&stack_frame, "class", sizeof("class")-1, STR_COPY(ptr->function_state.function->common.scope->name));
				add_assoc_string_ex(&stack_frame, "type", sizeof("type")-1, "::");
			}

			if ((options & DEBUG_BACKTRACE_IGNORE_ARGS) == 0 && 
				((! ptr->opline) || ((ptr->opline->opcode == ZEND_DO_FCALL_BY_NAME) || (ptr->opline->opcode == ZEND_DO_FCALL)))) {
				if (ptr->function_state.arguments) {
					zval args;
					debug_backtrace_get_args(ptr->function_state.arguments, &args TSRMLS_CC);
					add_assoc_zval_ex(&stack_frame, "args", sizeof("args")-1, &args);
				}
			}
		} else {
			/* i know this is kinda ugly, but i'm trying to avoid extra cycles in the main execution loop */
			zend_bool build_filename_arg = 1;

			if (!ptr->opline || ptr->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
				/* can happen when calling eval from a custom sapi */
				function_name = "unknown";
				build_filename_arg = 0;
			} else
			switch (ptr->opline->extended_value) {
				case ZEND_EVAL:
					function_name = "eval";
					build_filename_arg = 0;
					break;
				case ZEND_INCLUDE:
					function_name = "include";
					break;
				case ZEND_REQUIRE:
					function_name = "require";
					break;
				case ZEND_INCLUDE_ONCE:
					function_name = "include_once";
					break;
				case ZEND_REQUIRE_ONCE:
					function_name = "require_once";
					break;
				default:
					/* this can actually happen if you use debug_backtrace() in your error_handler and 
					 * you're in the top-scope */
					function_name = "unknown"; 
					build_filename_arg = 0;
					break;
			}

			if (build_filename_arg && include_filename) {
				zval arg_array;

				array_init(&arg_array);

				/* include_filename always points to the last filename of the last last called-function.
				   if we have called include in the frame above - this is the file we have included.
				 */

				add_next_index_string(&arg_array, (char*)include_filename);
				add_assoc_zval_ex(&stack_frame, "args", sizeof("args")-1, &arg_array);
			}

			add_assoc_string_ex(&stack_frame, "function", sizeof("function")-1, (char*)function_name);
		}

		add_next_index_zval(return_value, &stack_frame);

		include_filename = filename; 

		object = skip->object;
		ptr = skip->prev_execute_data;
	}
}
/* }}} */


/* {{{ proto array debug_backtrace([int options[, int limit]])
   Return backtrace as array */
ZEND_FUNCTION(debug_backtrace)
{
	long options = DEBUG_BACKTRACE_PROVIDE_OBJECT;
	long limit = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &options, &limit) == FAILURE) {
		return;
	}

	zend_fetch_debug_backtrace(return_value, 1, options, limit TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool extension_loaded(string extension_name)
   Returns true if the named extension is loaded */
ZEND_FUNCTION(extension_loaded)
{
	char *extension_name;
	int extension_name_len;
	zend_string *lcname;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &extension_name, &extension_name_len) == FAILURE) {
		return;
	}

	lcname = STR_ALLOC(extension_name_len, 0);
	zend_str_tolower_copy(lcname->val, extension_name, extension_name_len);
	if (zend_hash_exists(&module_registry, lcname)) {
		RETVAL_TRUE;
	} else {
		RETVAL_FALSE;
	}
	STR_FREE(lcname);
}
/* }}} */


/* {{{ proto array get_extension_funcs(string extension_name)
   Returns an array with the names of functions belonging to the named extension */
ZEND_FUNCTION(get_extension_funcs)
{
	char *extension_name;
	zend_string *lcname;
	int extension_name_len, array;
	zend_module_entry *module;
	zend_function *zif;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &extension_name, &extension_name_len) == FAILURE) {
		return;
	}
	if (strncasecmp(extension_name, "zend", sizeof("zend"))) {
		lcname = STR_ALLOC(extension_name_len, 0);
		zend_str_tolower_copy(lcname->val, extension_name, extension_name_len);
	} else {
		lcname = STR_INIT("core", sizeof("core")-1, 0);
	}
	module = zend_hash_find_ptr(&module_registry, lcname);
	STR_FREE(lcname);
	if (!module) {
		RETURN_FALSE;
	}

	if (module->functions) {
		/* avoid BC break, if functions list is empty, will return an empty array */
		array_init(return_value);
		array = 1;
	} else {
		array = 0;
	}

	ZEND_HASH_FOREACH_PTR(CG(function_table), zif) {
		if (zif->common.type == ZEND_INTERNAL_FUNCTION
			&& zif->internal_function.module == module) {
			if (!array) {
				array_init(return_value);
				array = 1;
			}
// TODO: we have to duplicate it, becaise it may be stored in opcache SHM ???
			add_next_index_str(return_value, STR_DUP(zif->common.function_name, 0));
		}
	} ZEND_HASH_FOREACH_END();

	if (!array) {
		RETURN_FALSE;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
