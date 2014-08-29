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
#include "zend_extensions.h"
#include "zend_modules.h"
#include "zend_constants.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_builtin_functions.h"
#include "zend_ini.h"
#include "zend_vm.h"
#include "zend_dtrace.h"
#include "zend_virtual_cwd.h"

#ifdef ZTS
# define GLOBAL_FUNCTION_TABLE		global_function_table
# define GLOBAL_CLASS_TABLE			global_class_table
# define GLOBAL_CONSTANTS_TABLE		global_constants_table
# define GLOBAL_AUTO_GLOBALS_TABLE	global_auto_globals_table
#else
# define GLOBAL_FUNCTION_TABLE		CG(function_table)
# define GLOBAL_CLASS_TABLE			CG(class_table)
# define GLOBAL_AUTO_GLOBALS_TABLE	CG(auto_globals)
# define GLOBAL_CONSTANTS_TABLE		EG(zend_constants)
#endif

/* true multithread-shared globals */
ZEND_API zend_class_entry *zend_standard_class_def = NULL;
ZEND_API size_t (*zend_printf)(const char *format, ...);
ZEND_API zend_write_func_t zend_write;
ZEND_API FILE *(*zend_fopen)(const char *filename, char **opened_path TSRMLS_DC);
ZEND_API int (*zend_stream_open_function)(const char *filename, zend_file_handle *handle TSRMLS_DC);
ZEND_API void (*zend_block_interruptions)(void);
ZEND_API void (*zend_unblock_interruptions)(void);
ZEND_API void (*zend_ticks_function)(int ticks TSRMLS_DC);
ZEND_API void (*zend_error_cb)(int type, const char *error_filename, const uint error_lineno, const char *format, va_list args);
size_t (*zend_vspprintf)(char **pbuf, size_t max_len, const char *format, va_list ap);
zend_string *(*zend_vstrpprintf)(size_t max_len, const char *format, va_list ap);
ZEND_API char *(*zend_getenv)(char *name, size_t name_len TSRMLS_DC);
ZEND_API char *(*zend_resolve_path)(const char *filename, int filename_len TSRMLS_DC);

void (*zend_on_timeout)(int seconds TSRMLS_DC);

static void (*zend_message_dispatcher_p)(zend_long message, const void *data TSRMLS_DC);
static int (*zend_get_configuration_directive_p)(const char *name, uint name_length, zval *contents);

static ZEND_INI_MH(OnUpdateErrorReporting) /* {{{ */
{
	if (!new_value) {
		EG(error_reporting) = E_ALL & ~E_NOTICE & ~E_STRICT & ~E_DEPRECATED;
	} else {
		EG(error_reporting) = atoi(new_value);
	}
	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(OnUpdateGCEnabled) /* {{{ */
{
	OnUpdateBool(entry, new_value, new_value_length, mh_arg1, mh_arg2, mh_arg3, stage TSRMLS_CC);

	if (GC_G(gc_enabled)) {
		gc_init(TSRMLS_C);
	}

	return SUCCESS;
}
/* }}} */

static ZEND_INI_MH(OnUpdateScriptEncoding) /* {{{ */
{
	if (!CG(multibyte)) {
		return FAILURE;
	}
	if (!zend_multibyte_get_functions(TSRMLS_C)) {
		return SUCCESS;
	}
	return zend_multibyte_set_script_encoding_by_string(new_value, new_value_length TSRMLS_CC);
}
/* }}} */


ZEND_INI_BEGIN()
	ZEND_INI_ENTRY("error_reporting",				NULL,		ZEND_INI_ALL,		OnUpdateErrorReporting)
	STD_ZEND_INI_BOOLEAN("zend.enable_gc",				"1",	ZEND_INI_ALL,		OnUpdateGCEnabled,      gc_enabled,     zend_gc_globals,        gc_globals)
 	STD_ZEND_INI_BOOLEAN("zend.multibyte", "0", ZEND_INI_PERDIR, OnUpdateBool, multibyte,      zend_compiler_globals, compiler_globals)
 	ZEND_INI_ENTRY("zend.script_encoding",			NULL,		ZEND_INI_ALL,		OnUpdateScriptEncoding)
 	STD_ZEND_INI_BOOLEAN("zend.detect_unicode",			"1",	ZEND_INI_ALL,		OnUpdateBool, detect_unicode, zend_compiler_globals, compiler_globals)
#ifdef ZEND_SIGNALS
	STD_ZEND_INI_BOOLEAN("zend.signal_check", "0", ZEND_INI_SYSTEM, OnUpdateBool, check, zend_signal_globals_t, zend_signal_globals)
#endif
ZEND_INI_END()


#ifdef ZTS
ZEND_API int compiler_globals_id;
ZEND_API int executor_globals_id;
static HashTable *global_function_table = NULL;
static HashTable *global_class_table = NULL;
static HashTable *global_constants_table = NULL;
static HashTable *global_auto_globals_table = NULL;
static HashTable *global_persistent_list = NULL;
#endif

ZEND_API zend_utility_values zend_uv;

/* version information */
static char *zend_version_info;
static uint zend_version_info_length;
#define ZEND_CORE_VERSION_INFO	"Zend Engine v" ZEND_VERSION ", Copyright (c) 1998-2014 Zend Technologies\n"
#define PRINT_ZVAL_INDENT 4

static void print_hash(zend_write_func_t write_func, HashTable *ht, int indent, zend_bool is_object TSRMLS_DC) /* {{{ */
{
	zval *tmp;
	zend_string *string_key;
	zend_ulong num_key;
	int i;

	for (i = 0; i < indent; i++) {
		ZEND_PUTS_EX(" ");
	}
	ZEND_PUTS_EX("(\n");
	indent += PRINT_ZVAL_INDENT;
	ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, string_key, tmp) {
		if (Z_TYPE_P(tmp) == IS_INDIRECT) {
			tmp = Z_INDIRECT_P(tmp);
			if (Z_TYPE_P(tmp) == IS_UNDEF) {
				continue;
			}
		}
		for (i = 0; i < indent; i++) {
			ZEND_PUTS_EX(" ");
		}
		ZEND_PUTS_EX("[");
		if (string_key) {
			if (is_object) {
				const char *prop_name, *class_name;
				int prop_len;
				int mangled = zend_unmangle_property_name_ex(string_key->val, string_key->len, &class_name, &prop_name, &prop_len);

				ZEND_WRITE_EX(prop_name, prop_len);
				if (class_name && mangled == SUCCESS) {
					if (class_name[0]=='*') {
						ZEND_PUTS_EX(":protected");
					} else {
						ZEND_PUTS_EX(":");
						ZEND_PUTS_EX(class_name);
						ZEND_PUTS_EX(":private");
					}
				}
			} else {
				ZEND_WRITE_EX(string_key->val, string_key->len);
			}
		} else {
			char key[25];
			snprintf(key, sizeof(key), ZEND_LONG_FMT, num_key);
			ZEND_PUTS_EX(key);
		}
		ZEND_PUTS_EX("] => ");
		zend_print_zval_r_ex(write_func, tmp, indent+PRINT_ZVAL_INDENT TSRMLS_CC);
		ZEND_PUTS_EX("\n");
	} ZEND_HASH_FOREACH_END();
	indent -= PRINT_ZVAL_INDENT;
	for (i = 0; i < indent; i++) {
		ZEND_PUTS_EX(" ");
	}
	ZEND_PUTS_EX(")\n");
}
/* }}} */

static void print_flat_hash(HashTable *ht TSRMLS_DC) /* {{{ */
{
	zval *tmp;
	zend_string *string_key;
	zend_ulong num_key;
	int i = 0;

	ZEND_HASH_FOREACH_KEY_VAL_IND(ht, num_key, string_key, tmp) {
		if (i++ > 0) {
			ZEND_PUTS(",");
		}
		ZEND_PUTS("[");
		if (string_key) {
			ZEND_WRITE(string_key->val, string_key->len);
		} else {
			zend_printf(ZEND_ULONG_FMT, num_key);
		}
		ZEND_PUTS("] => ");
		zend_print_flat_zval_r(tmp TSRMLS_CC);
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

ZEND_API int zend_make_printable_zval(zval *expr, zval *expr_copy TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(expr) == IS_STRING) {
		return 0;
	}

again:
	switch (Z_TYPE_P(expr)) {
		case IS_NULL:
		case IS_FALSE:
			ZVAL_EMPTY_STRING(expr_copy);
		    break;
		case IS_TRUE:
		    if (CG(one_char_string)['1']) {
				ZVAL_INTERNED_STR(expr_copy, CG(one_char_string)['1']);
			} else {
				ZVAL_NEW_STR(expr_copy, zend_string_init("1", 1, 0));
			}
		    break;
		case IS_RESOURCE: {
				char buf[sizeof("Resource id #") + MAX_LENGTH_OF_LONG];
				int len;

				len = snprintf(buf, sizeof(buf), "Resource id #" ZEND_LONG_FMT, Z_RES_HANDLE_P(expr));
				ZVAL_NEW_STR(expr_copy, zend_string_init(buf, len, 0));
			}
			break;
		case IS_ARRAY:
			zend_error(E_NOTICE, "Array to string conversion");
			// TODO: use interned string ???
			ZVAL_NEW_STR(expr_copy, zend_string_init("Array", sizeof("Array") - 1, 0));
			break;
		case IS_OBJECT:
			if (Z_OBJ_HANDLER_P(expr, cast_object)) {
				Z_ADDREF_P(expr);
				if (Z_OBJ_HANDLER_P(expr, cast_object)(expr, expr_copy, IS_STRING TSRMLS_CC) == SUCCESS) {
					zval_ptr_dtor(expr);
					break;
				}
				zval_ptr_dtor(expr);
			}
			if (!Z_OBJ_HANDLER_P(expr, cast_object) && Z_OBJ_HANDLER_P(expr, get)) {
				zval rv;
				zval *z = Z_OBJ_HANDLER_P(expr, get)(expr, &rv TSRMLS_CC);

				Z_ADDREF_P(z);
				if (Z_TYPE_P(z) != IS_OBJECT) {
					if (zend_make_printable_zval(z, expr_copy TSRMLS_CC)) {
						zval_ptr_dtor(z);
					} else {
						ZVAL_ZVAL(expr_copy, z, 0, 1);
					}
					return 1;
				}
				zval_ptr_dtor(z);
			}
			zend_error(EG(exception) ? E_ERROR : E_RECOVERABLE_ERROR, "Object of class %s could not be converted to string", Z_OBJCE_P(expr)->name->val);
			ZVAL_EMPTY_STRING(expr_copy);
			break;
		case IS_DOUBLE:
			ZVAL_DUP(expr_copy, expr);
			zend_locale_sprintf_double(expr_copy ZEND_FILE_LINE_CC);
			break;
		case IS_REFERENCE:
			expr = Z_REFVAL_P(expr);
			if (Z_TYPE_P(expr) == IS_STRING) {
				ZVAL_STR(expr_copy, zend_string_copy(Z_STR_P(expr)));
				return 1;
			}
			goto again;
			break;
		default:
			ZVAL_DUP(expr_copy, expr);
			convert_to_string(expr_copy);
			break;
	}
	return 1;
}
/* }}} */

ZEND_API int zend_print_zval(zval *expr, int indent TSRMLS_DC) /* {{{ */
{
	return zend_print_zval_ex(zend_write, expr, indent TSRMLS_CC);
}
/* }}} */

ZEND_API int zend_print_zval_ex(zend_write_func_t write_func, zval *expr, int indent TSRMLS_DC) /* {{{ */
{
	zend_string *str = zval_get_string(expr);
	size_t len = str->len;

	if (len != 0) {
		write_func(str->val, len);
	}

	zend_string_release(str);
	return len;
}
/* }}} */

ZEND_API void zend_print_flat_zval_r(zval *expr TSRMLS_DC) /* {{{ */
{
	switch (Z_TYPE_P(expr)) {
		case IS_ARRAY:
			ZEND_PUTS("Array (");
			if (ZEND_HASH_APPLY_PROTECTION(Z_ARRVAL_P(expr)) &&
			    ++Z_ARRVAL_P(expr)->u.v.nApplyCount>1) {
				ZEND_PUTS(" *RECURSION*");
				Z_ARRVAL_P(expr)->u.v.nApplyCount--;
				return;
			}
			print_flat_hash(Z_ARRVAL_P(expr) TSRMLS_CC);
			ZEND_PUTS(")");
			if (ZEND_HASH_APPLY_PROTECTION(Z_ARRVAL_P(expr))) {
				Z_ARRVAL_P(expr)->u.v.nApplyCount--;
			}
			break;
		case IS_OBJECT:
		{
			HashTable *properties = NULL;
			zend_string *class_name = NULL;

			if (Z_OBJ_HANDLER_P(expr, get_class_name)) {
				class_name = Z_OBJ_HANDLER_P(expr, get_class_name)(Z_OBJ_P(expr), 0 TSRMLS_CC);
			}
			if (class_name) {
				zend_printf("%s Object (", class_name->val);
			} else {
				zend_printf("%s Object (", "Unknown Class");
			}
			if (class_name) {
				zend_string_release(class_name);
			}
			if (Z_OBJ_HANDLER_P(expr, get_properties)) {
				properties = Z_OBJPROP_P(expr);
			}
			if (properties) {
				if (++properties->u.v.nApplyCount>1) {
					ZEND_PUTS(" *RECURSION*");
					properties->u.v.nApplyCount--;
					return;
				}
				print_flat_hash(properties TSRMLS_CC);
				properties->u.v.nApplyCount--;
			}
			ZEND_PUTS(")");
			break;
		}
		default:
			zend_print_variable(expr TSRMLS_CC);
			break;
	}
}
/* }}} */

ZEND_API void zend_print_zval_r(zval *expr, int indent TSRMLS_DC) /* {{{ */
{
	zend_print_zval_r_ex(zend_write, expr, indent TSRMLS_CC);
}
/* }}} */

ZEND_API void zend_print_zval_r_ex(zend_write_func_t write_func, zval *expr, int indent TSRMLS_DC) /* {{{ */
{
	ZVAL_DEREF(expr);
	switch (Z_TYPE_P(expr)) {
		case IS_ARRAY:
			ZEND_PUTS_EX("Array\n");
			if (ZEND_HASH_APPLY_PROTECTION(Z_ARRVAL_P(expr)) &&
			    ++Z_ARRVAL_P(expr)->u.v.nApplyCount>1) {
				ZEND_PUTS_EX(" *RECURSION*");
				Z_ARRVAL_P(expr)->u.v.nApplyCount--;
				return;
			}
			print_hash(write_func, Z_ARRVAL_P(expr), indent, 0 TSRMLS_CC);
			if (ZEND_HASH_APPLY_PROTECTION(Z_ARRVAL_P(expr))) {
				Z_ARRVAL_P(expr)->u.v.nApplyCount--;
			}
			break;
		case IS_OBJECT:
			{
				HashTable *properties;
				zend_string *class_name = NULL;
				int is_temp;

				if (Z_OBJ_HANDLER_P(expr, get_class_name)) {
					class_name = Z_OBJ_HANDLER_P(expr, get_class_name)(Z_OBJ_P(expr), 0 TSRMLS_CC);
				}
				if (class_name) {
					ZEND_PUTS_EX(class_name->val);
				} else {
					ZEND_PUTS_EX("Unknown Class");
				}
				ZEND_PUTS_EX(" Object\n");
				if (class_name) {
					zend_string_release(class_name);
				}
				if ((properties = Z_OBJDEBUG_P(expr, is_temp)) == NULL) {
					break;
				}
				if (++properties->u.v.nApplyCount>1) {
					ZEND_PUTS_EX(" *RECURSION*");
					properties->u.v.nApplyCount--;
					return;
				}
				print_hash(write_func, properties, indent, 1 TSRMLS_CC);
				properties->u.v.nApplyCount--;
				if (is_temp) {
					zend_hash_destroy(properties);
					FREE_HASHTABLE(properties);
				}
				break;
			}
		default:
			zend_print_zval_ex(write_func, expr, indent TSRMLS_CC);
			break;
	}
}
/* }}} */

static FILE *zend_fopen_wrapper(const char *filename, char **opened_path TSRMLS_DC) /* {{{ */
{
	if (opened_path) {
		*opened_path = estrdup(filename);
	}
	return fopen(filename, "rb");
}
/* }}} */

#ifdef ZTS
static zend_bool asp_tags_default		  = 0;
static zend_bool short_tags_default		  = 1;
static uint32_t compiler_options_default = ZEND_COMPILE_DEFAULT;
#else
# define asp_tags_default			0
# define short_tags_default			1
# define compiler_options_default	ZEND_COMPILE_DEFAULT
#endif

static void zend_set_default_compile_time_values(TSRMLS_D) /* {{{ */
{
	/* default compile-time values */
	CG(asp_tags) = asp_tags_default;
	CG(short_tags) = short_tags_default;
	CG(compiler_options) = compiler_options_default;
}
/* }}} */

static void zend_init_exception_op(TSRMLS_D) /* {{{ */
{
	memset(EG(exception_op), 0, sizeof(EG(exception_op)));
	EG(exception_op)[0].opcode = ZEND_HANDLE_EXCEPTION;
	EG(exception_op)[0].op1_type = IS_UNUSED;
	EG(exception_op)[0].op2_type = IS_UNUSED;
	EG(exception_op)[0].result_type = IS_UNUSED;
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op));
	EG(exception_op)[1].opcode = ZEND_HANDLE_EXCEPTION;
	EG(exception_op)[1].op1_type = IS_UNUSED;
	EG(exception_op)[1].op2_type = IS_UNUSED;
	EG(exception_op)[1].result_type = IS_UNUSED;
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op)+1);
	EG(exception_op)[2].opcode = ZEND_HANDLE_EXCEPTION;
	EG(exception_op)[2].op1_type = IS_UNUSED;
	EG(exception_op)[2].op2_type = IS_UNUSED;
	EG(exception_op)[2].result_type = IS_UNUSED;
	ZEND_VM_SET_OPCODE_HANDLER(EG(exception_op)+2);
}
/* }}} */

#ifdef ZTS
static void function_copy_ctor(zval *zv)
{
	zend_function *old_func = Z_FUNC_P(zv);
	Z_FUNC_P(zv) = pemalloc(sizeof(zend_internal_function), 1);
	memcpy(Z_FUNC_P(zv), old_func, sizeof(zend_internal_function));
	function_add_ref(Z_FUNC_P(zv));
}

static void compiler_globals_ctor(zend_compiler_globals *compiler_globals TSRMLS_DC) /* {{{ */
{
	compiler_globals->compiled_filename = NULL;

	compiler_globals->function_table = (HashTable *) malloc(sizeof(HashTable));
	zend_hash_init_ex(compiler_globals->function_table, 1024, NULL, ZEND_FUNCTION_DTOR, 1, 0);
	zend_hash_copy(compiler_globals->function_table, global_function_table, function_copy_ctor);

	compiler_globals->class_table = (HashTable *) malloc(sizeof(HashTable));
	zend_hash_init_ex(compiler_globals->class_table, 64, NULL, ZEND_CLASS_DTOR, 1, 0);
	zend_hash_copy(compiler_globals->class_table, global_class_table, zend_class_add_ref);

	zend_set_default_compile_time_values(TSRMLS_C);

	compiler_globals->auto_globals = (HashTable *) malloc(sizeof(HashTable));
	zend_hash_init_ex(compiler_globals->auto_globals, 8, NULL, NULL, 1, 0);
	zend_hash_copy(compiler_globals->auto_globals, global_auto_globals_table, NULL /* empty element */);

	compiler_globals->last_static_member = zend_hash_num_elements(compiler_globals->class_table);
	if (compiler_globals->last_static_member) {
		compiler_globals->static_members_table = calloc(compiler_globals->last_static_member, sizeof(zval**));
	} else {
		compiler_globals->static_members_table = NULL;
	}
	compiler_globals->script_encoding_list = NULL;
}
/* }}} */

static void compiler_globals_dtor(zend_compiler_globals *compiler_globals TSRMLS_DC) /* {{{ */
{
	if (compiler_globals->function_table != GLOBAL_FUNCTION_TABLE) {
		zend_hash_destroy(compiler_globals->function_table);
		free(compiler_globals->function_table);
	}
	if (compiler_globals->class_table != GLOBAL_CLASS_TABLE) {
		zend_hash_destroy(compiler_globals->class_table);
		free(compiler_globals->class_table);
	}
	if (compiler_globals->auto_globals != GLOBAL_AUTO_GLOBALS_TABLE) {
		zend_hash_destroy(compiler_globals->auto_globals);
		free(compiler_globals->auto_globals);
	}
	if (compiler_globals->static_members_table) {
		free(compiler_globals->static_members_table);
	}
	if (compiler_globals->script_encoding_list) {
		pefree((char*)compiler_globals->script_encoding_list, 1);
	}
	compiler_globals->last_static_member = 0;
}
/* }}} */

static void executor_globals_ctor(zend_executor_globals *executor_globals TSRMLS_DC) /* {{{ */
{
	zend_startup_constants(TSRMLS_C);
	zend_copy_constants(EG(zend_constants), GLOBAL_CONSTANTS_TABLE);
	zend_init_rsrc_plist(TSRMLS_C);
	zend_init_exception_op(TSRMLS_C);
	EG(lambda_count) = 0;
	ZVAL_UNDEF(&EG(user_error_handler));
	ZVAL_UNDEF(&EG(user_exception_handler));
	EG(in_autoload) = NULL;
	EG(current_execute_data) = NULL;
	EG(current_module) = NULL;
	EG(exit_status) = 0;
#if XPFPA_HAVE_CW
	EG(saved_fpu_cw) = 0;
#endif
	EG(saved_fpu_cw_ptr) = NULL;
	EG(active) = 0;
}
/* }}} */

static void executor_globals_dtor(zend_executor_globals *executor_globals TSRMLS_DC) /* {{{ */
{
	zend_ini_shutdown(TSRMLS_C);
	if (&executor_globals->persistent_list != global_persistent_list) {
		zend_destroy_rsrc_list(&executor_globals->persistent_list TSRMLS_CC);
	}
	if (executor_globals->zend_constants != GLOBAL_CONSTANTS_TABLE) {
		zend_hash_destroy(executor_globals->zend_constants);
		free(executor_globals->zend_constants);
	}
}
/* }}} */

static void zend_new_thread_end_handler(THREAD_T thread_id TSRMLS_DC) /* {{{ */
{
	if (zend_copy_ini_directives(TSRMLS_C) == SUCCESS) {
		zend_ini_refresh_caches(ZEND_INI_STAGE_STARTUP TSRMLS_CC);
	}
}
/* }}} */
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
/* FreeBSD and DragonFly floating point precision fix */
#include <floatingpoint.h>
#endif

static void ini_scanner_globals_ctor(zend_ini_scanner_globals *scanner_globals_p TSRMLS_DC) /* {{{ */
{
	memset(scanner_globals_p, 0, sizeof(*scanner_globals_p));
}
/* }}} */

static void php_scanner_globals_ctor(zend_php_scanner_globals *scanner_globals_p TSRMLS_DC) /* {{{ */
{
	memset(scanner_globals_p, 0, sizeof(*scanner_globals_p));
}
/* }}} */

void zend_init_opcodes_handlers(void);

static void module_destructor_zval(zval *zv) /* {{{ */
{
	zend_module_entry *module = (zend_module_entry*)Z_PTR_P(zv);

	module_destructor(module);
	free(module);
}
/* }}} */

static void auto_global_dtor(zval *zv) /* {{{ */
{
	free(Z_PTR_P(zv));
}
/* }}} */

static zend_bool php_auto_globals_create_globals(zend_string *name TSRMLS_DC) /* {{{ */
{
	zval globals;

	ZVAL_ARR(&globals, &EG(symbol_table));
	ZVAL_NEW_REF(&globals, &globals);
	zend_hash_update(&EG(symbol_table).ht, name, &globals);
	return 0;
}
/* }}} */

int zend_startup(zend_utility_functions *utility_functions, char **extensions TSRMLS_DC) /* {{{ */
{
#ifdef ZTS
	zend_compiler_globals *compiler_globals;
	zend_executor_globals *executor_globals;
	extern ZEND_API ts_rsrc_id ini_scanner_globals_id;
	extern ZEND_API ts_rsrc_id language_scanner_globals_id;
#else
	extern zend_ini_scanner_globals ini_scanner_globals;
	extern zend_php_scanner_globals language_scanner_globals;
#endif

	start_memory_manager(TSRMLS_C);

	virtual_cwd_startup(); /* Could use shutdown to free the main cwd but it would just slow it down for CGI */

#if defined(__FreeBSD__) || defined(__DragonFly__)
	/* FreeBSD and DragonFly floating point precision fix */
	fpsetmask(0);
#endif

	zend_startup_strtod();
	zend_startup_extensions_mechanism();

	/* Set up utility functions and values */
	zend_error_cb = utility_functions->error_function;
	zend_printf = utility_functions->printf_function;
	zend_write = (zend_write_func_t) utility_functions->write_function;
	zend_fopen = utility_functions->fopen_function;
	if (!zend_fopen) {
		zend_fopen = zend_fopen_wrapper;
	}
	zend_stream_open_function = utility_functions->stream_open_function;
	zend_message_dispatcher_p = utility_functions->message_handler;
#ifndef ZEND_SIGNALS
	zend_block_interruptions = utility_functions->block_interruptions;
	zend_unblock_interruptions = utility_functions->unblock_interruptions;
#endif
	zend_get_configuration_directive_p = utility_functions->get_configuration_directive;
	zend_ticks_function = utility_functions->ticks_function;
	zend_on_timeout = utility_functions->on_timeout;
	zend_vspprintf = utility_functions->vspprintf_function;
	zend_vstrpprintf = utility_functions->vstrpprintf_function;
	zend_getenv = utility_functions->getenv_function;
	zend_resolve_path = utility_functions->resolve_path_function;

#if HAVE_DTRACE
/* build with dtrace support */
	zend_compile_file = dtrace_compile_file;
	zend_execute_ex = dtrace_execute_ex;
	zend_execute_internal = dtrace_execute_internal;
#else
	zend_compile_file = compile_file;
	zend_execute_ex = execute_ex;
	zend_execute_internal = NULL;
#endif /* HAVE_SYS_SDT_H */
	zend_compile_string = compile_string;
	zend_throw_exception_hook = NULL;

	zend_init_opcodes_handlers();

	/* set up version */
	zend_version_info = strdup(ZEND_CORE_VERSION_INFO);
	zend_version_info_length = sizeof(ZEND_CORE_VERSION_INFO) - 1;

	GLOBAL_FUNCTION_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_CLASS_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_AUTO_GLOBALS_TABLE = (HashTable *) malloc(sizeof(HashTable));
	GLOBAL_CONSTANTS_TABLE = (HashTable *) malloc(sizeof(HashTable));

	zend_hash_init_ex(GLOBAL_FUNCTION_TABLE, 1024, NULL, ZEND_FUNCTION_DTOR, 1, 0);
	zend_hash_init_ex(GLOBAL_CLASS_TABLE, 64, NULL, ZEND_CLASS_DTOR, 1, 0);
	zend_hash_init_ex(GLOBAL_AUTO_GLOBALS_TABLE, 8, NULL, auto_global_dtor, 1, 0);
	zend_hash_init_ex(GLOBAL_CONSTANTS_TABLE, 128, NULL, ZEND_CONSTANT_DTOR, 1, 0);

	zend_hash_init_ex(&module_registry, 32, NULL, module_destructor_zval, 1, 0);
	zend_init_rsrc_list_dtors();

#ifdef ZTS
	ts_allocate_id(&compiler_globals_id, sizeof(zend_compiler_globals), (ts_allocate_ctor) compiler_globals_ctor, (ts_allocate_dtor) compiler_globals_dtor);
	ts_allocate_id(&executor_globals_id, sizeof(zend_executor_globals), (ts_allocate_ctor) executor_globals_ctor, (ts_allocate_dtor) executor_globals_dtor);
	ts_allocate_id(&language_scanner_globals_id, sizeof(zend_php_scanner_globals), (ts_allocate_ctor) php_scanner_globals_ctor, NULL);
	ts_allocate_id(&ini_scanner_globals_id, sizeof(zend_ini_scanner_globals), (ts_allocate_ctor) ini_scanner_globals_ctor, NULL);
	compiler_globals = ts_resource(compiler_globals_id);
	executor_globals = ts_resource(executor_globals_id);

	compiler_globals_dtor(compiler_globals TSRMLS_CC);
	compiler_globals->in_compilation = 0;
	compiler_globals->function_table = (HashTable *) malloc(sizeof(HashTable));
	compiler_globals->class_table = (HashTable *) malloc(sizeof(HashTable));

	*compiler_globals->function_table = *GLOBAL_FUNCTION_TABLE;
	*compiler_globals->class_table = *GLOBAL_CLASS_TABLE;
	compiler_globals->auto_globals = GLOBAL_AUTO_GLOBALS_TABLE;

	zend_hash_destroy(executor_globals->zend_constants);
	*executor_globals->zend_constants = *GLOBAL_CONSTANTS_TABLE;
#else
	ini_scanner_globals_ctor(&ini_scanner_globals TSRMLS_CC);
	php_scanner_globals_ctor(&language_scanner_globals TSRMLS_CC);
	zend_set_default_compile_time_values(TSRMLS_C);
	ZVAL_UNDEF(&EG(user_error_handler));
	ZVAL_UNDEF(&EG(user_exception_handler));
#endif

	zend_interned_strings_init(TSRMLS_C);
	zend_startup_builtin_functions(TSRMLS_C);
	zend_register_standard_constants(TSRMLS_C);
	zend_register_auto_global(zend_string_init("GLOBALS", sizeof("GLOBALS") - 1, 1), 1, php_auto_globals_create_globals TSRMLS_CC);

#ifndef ZTS
	zend_init_rsrc_plist(TSRMLS_C);
	zend_init_exception_op(TSRMLS_C);
#endif

	zend_ini_startup(TSRMLS_C);

#ifdef ZTS
	tsrm_set_new_thread_end_handler(zend_new_thread_end_handler);
#endif

	return SUCCESS;
}
/* }}} */

void zend_register_standard_ini_entries(TSRMLS_D) /* {{{ */
{
	int module_number = 0;

	REGISTER_INI_ENTRIES();
}
/* }}} */

/* Unlink the global (r/o) copies of the class, function and constant tables,
 * and use a fresh r/w copy for the startup thread
 */
void zend_post_startup(TSRMLS_D) /* {{{ */
{
#ifdef ZTS
	zend_encoding **script_encoding_list;

	zend_compiler_globals *compiler_globals = ts_resource(compiler_globals_id);
	zend_executor_globals *executor_globals = ts_resource(executor_globals_id);

	*GLOBAL_FUNCTION_TABLE = *compiler_globals->function_table;
	*GLOBAL_CLASS_TABLE = *compiler_globals->class_table;
	*GLOBAL_CONSTANTS_TABLE = *executor_globals->zend_constants;

	asp_tags_default = CG(asp_tags);
	short_tags_default = CG(short_tags);
	compiler_options_default = CG(compiler_options);

	zend_destroy_rsrc_list(&EG(persistent_list) TSRMLS_CC);
	free(compiler_globals->function_table);
	free(compiler_globals->class_table);
	if ((script_encoding_list = (zend_encoding **)compiler_globals->script_encoding_list)) {
		compiler_globals_ctor(compiler_globals, tsrm_ls);
		compiler_globals->script_encoding_list = (const zend_encoding **)script_encoding_list;
	} else {
		compiler_globals_ctor(compiler_globals, tsrm_ls);
	}
	free(EG(zend_constants));

	virtual_cwd_deactivate(TSRMLS_C);

	executor_globals_ctor(executor_globals, tsrm_ls);
	global_persistent_list = &EG(persistent_list);
	zend_copy_ini_directives(TSRMLS_C);
#else
	virtual_cwd_deactivate(TSRMLS_C);
#endif
}
/* }}} */

void zend_shutdown(TSRMLS_D) /* {{{ */
{
#ifdef ZEND_SIGNALS
	zend_signal_shutdown(TSRMLS_C);
#endif
#ifdef ZEND_WIN32
	zend_shutdown_timeout_thread();
#endif
	zend_destroy_rsrc_list(&EG(persistent_list) TSRMLS_CC);
	zend_destroy_modules();

 	virtual_cwd_deactivate(TSRMLS_C);
 	virtual_cwd_shutdown();

	zend_hash_destroy(GLOBAL_FUNCTION_TABLE);
	zend_hash_destroy(GLOBAL_CLASS_TABLE);

	zend_hash_destroy(GLOBAL_AUTO_GLOBALS_TABLE);
	free(GLOBAL_AUTO_GLOBALS_TABLE);

	zend_shutdown_extensions(TSRMLS_C);
	free(zend_version_info);

	free(GLOBAL_FUNCTION_TABLE);
	free(GLOBAL_CLASS_TABLE);

	zend_hash_destroy(GLOBAL_CONSTANTS_TABLE);
	free(GLOBAL_CONSTANTS_TABLE);
	zend_shutdown_strtod();

#ifdef ZTS
	GLOBAL_FUNCTION_TABLE = NULL;
	GLOBAL_CLASS_TABLE = NULL;
	GLOBAL_AUTO_GLOBALS_TABLE = NULL;
	GLOBAL_CONSTANTS_TABLE = NULL;
#endif
	zend_destroy_rsrc_list_dtors();

	zend_interned_strings_dtor(TSRMLS_C);
}
/* }}} */

void zend_set_utility_values(zend_utility_values *utility_values) /* {{{ */
{
	zend_uv = *utility_values;
	zend_uv.import_use_extension_length = strlen(zend_uv.import_use_extension);
}
/* }}} */

/* this should be compatible with the standard zenderror */
void zenderror(const char *error) /* {{{ */
{
	zend_error(E_PARSE, "%s", error);
}
/* }}} */

BEGIN_EXTERN_C()
ZEND_API void _zend_bailout(char *filename, uint lineno) /* {{{ */
{
	TSRMLS_FETCH();

	if (!EG(bailout)) {
		zend_output_debug_string(1, "%s(%d) : Bailed out without a bailout address!", filename, lineno);
		exit(-1);
	}
	CG(unclean_shutdown) = 1;
	CG(active_class_entry) = NULL;
	CG(in_compilation) = 0;
	EG(current_execute_data) = NULL;
	LONGJMP(*EG(bailout), FAILURE);
}
/* }}} */
END_EXTERN_C()

void zend_append_version_info(const zend_extension *extension) /* {{{ */
{
	char *new_info;
	uint new_info_length;

	new_info_length = sizeof("    with  v, , by \n")
						+ strlen(extension->name)
						+ strlen(extension->version)
						+ strlen(extension->copyright)
						+ strlen(extension->author);

	new_info = (char *) malloc(new_info_length + 1);

	snprintf(new_info, new_info_length, "    with %s v%s, %s, by %s\n", extension->name, extension->version, extension->copyright, extension->author);

	zend_version_info = (char *) realloc(zend_version_info, zend_version_info_length+new_info_length + 1);
	strncat(zend_version_info, new_info, new_info_length);
	zend_version_info_length += new_info_length;
	free(new_info);
}
/* }}} */

ZEND_API char *get_zend_version(void) /* {{{ */
{
	return zend_version_info;
}
/* }}} */

ZEND_API void zend_activate(TSRMLS_D) /* {{{ */
{
#ifdef ZTS
	virtual_cwd_activate(TSRMLS_C);
#endif
	gc_reset(TSRMLS_C);
	init_compiler(TSRMLS_C);
	init_executor(TSRMLS_C);
	startup_scanner(TSRMLS_C);
}
/* }}} */

void zend_call_destructors(TSRMLS_D) /* {{{ */
{
	zend_try {
		shutdown_destructors(TSRMLS_C);
	} zend_end_try();
}
/* }}} */

ZEND_API void zend_deactivate(TSRMLS_D) /* {{{ */
{
	/* we're no longer executing anything */
	EG(current_execute_data) = NULL;

	zend_try {
		shutdown_scanner(TSRMLS_C);
	} zend_end_try();

	/* shutdown_executor() takes care of its own bailout handling */
	shutdown_executor(TSRMLS_C);

	zend_try {
		shutdown_compiler(TSRMLS_C);
	} zend_end_try();

	zend_destroy_rsrc_list(&EG(regular_list) TSRMLS_CC);

#if ZEND_DEBUG
	if (GC_G(gc_enabled) && !CG(unclean_shutdown)) {
		gc_collect_cycles(TSRMLS_C);
	}
#endif

#if GC_BENCH
	fprintf(stderr, "GC Statistics\n");
	fprintf(stderr, "-------------\n");
	fprintf(stderr, "Runs:               %d\n", GC_G(gc_runs));
	fprintf(stderr, "Collected:          %d\n", GC_G(collected));
	fprintf(stderr, "Root buffer length: %d\n", GC_G(root_buf_length));
	fprintf(stderr, "Root buffer peak:   %d\n\n", GC_G(root_buf_peak));
	fprintf(stderr, "      Possible            Remove from  Marked\n");
	fprintf(stderr, "        Root    Buffered     buffer     grey\n");
	fprintf(stderr, "      --------  --------  -----------  ------\n");
	fprintf(stderr, "ZVAL  %8d  %8d  %9d  %8d\n", GC_G(zval_possible_root), GC_G(zval_buffered), GC_G(zval_remove_from_buffer), GC_G(zval_marked_grey));
	fprintf(stderr, "ZOBJ  %8d  %8d  %9d  %8d\n", GC_G(zobj_possible_root), GC_G(zobj_buffered), GC_G(zobj_remove_from_buffer), GC_G(zobj_marked_grey));
#endif

	zend_try {
		zend_ini_deactivate(TSRMLS_C);
	} zend_end_try();
}
/* }}} */

BEGIN_EXTERN_C()
ZEND_API void zend_message_dispatcher(zend_long message, const void *data TSRMLS_DC) /* {{{ */
{
	if (zend_message_dispatcher_p) {
		zend_message_dispatcher_p(message, data TSRMLS_CC);
	}
}
/* }}} */
END_EXTERN_C()

ZEND_API int zend_get_configuration_directive(const char *name, uint name_length, zval *contents) /* {{{ */
{
	if (zend_get_configuration_directive_p) {
		return zend_get_configuration_directive_p(name, name_length, contents);
	} else {
		return FAILURE;
	}
}
/* }}} */

#define SAVE_STACK(stack) do { \
		if (CG(stack).top) { \
			memcpy(&stack, &CG(stack), sizeof(zend_stack)); \
			CG(stack).top = CG(stack).max = 0; \
			CG(stack).elements = NULL; \
		} else { \
			stack.top = 0; \
		} \
	} while (0)

#define RESTORE_STACK(stack) do { \
		if (stack.top) { \
			zend_stack_destroy(&CG(stack)); \
			memcpy(&CG(stack), &stack, sizeof(zend_stack)); \
		} \
	} while (0)

ZEND_API void zend_error(int type, const char *format, ...) /* {{{ */
{
	char *str;
	int len;
	va_list args;
	va_list usr_copy;
	zval params[5];
	zval retval;
	const char *error_filename;
	uint error_lineno = 0;
	zval orig_user_error_handler;
	zend_bool in_compilation;
	zend_class_entry *saved_class_entry;
	zend_stack loop_var_stack;
	zend_stack delayed_oplines_stack;
	zend_stack context_stack;
	zend_array *symbol_table;
	TSRMLS_FETCH();

	/* Report about uncaught exception in case of fatal errors */
	if (EG(exception)) {
		zend_execute_data *ex;
		const zend_op *opline;

		switch (type) {
			case E_CORE_ERROR:
			case E_ERROR:
			case E_RECOVERABLE_ERROR:
			case E_PARSE:
			case E_COMPILE_ERROR:
			case E_USER_ERROR:
				ex = EG(current_execute_data);
				opline = NULL;
				while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
					ex = ex->prev_execute_data;
				}
				if (ex && ex->opline->opcode == ZEND_HANDLE_EXCEPTION &&
				    EG(opline_before_exception)) {
					opline = EG(opline_before_exception);
				}
				zend_exception_error(EG(exception), E_WARNING TSRMLS_CC);
				EG(exception) = NULL;
				if (opline) {
					ex->opline = opline;
				}
				break;
			default:
				break;
		}
	}

	/* Obtain relevant filename and lineno */
	switch (type) {
		case E_CORE_ERROR:
		case E_CORE_WARNING:
			error_filename = NULL;
			error_lineno = 0;
			break;
		case E_PARSE:
		case E_COMPILE_ERROR:
		case E_COMPILE_WARNING:
		case E_ERROR:
		case E_NOTICE:
		case E_STRICT:
		case E_DEPRECATED:
		case E_WARNING:
		case E_USER_ERROR:
		case E_USER_WARNING:
		case E_USER_NOTICE:
		case E_USER_DEPRECATED:
		case E_RECOVERABLE_ERROR:
			if (zend_is_compiling(TSRMLS_C)) {
				error_filename = zend_get_compiled_filename(TSRMLS_C)->val;
				error_lineno = zend_get_compiled_lineno(TSRMLS_C);
			} else if (zend_is_executing(TSRMLS_C)) {
				error_filename = zend_get_executed_filename(TSRMLS_C);
				if (error_filename[0] == '[') { /* [no active file] */
					error_filename = NULL;
					error_lineno = 0;
				} else {
					error_lineno = zend_get_executed_lineno(TSRMLS_C);
				}
			} else {
				error_filename = NULL;
				error_lineno = 0;
			}
			break;
		default:
			error_filename = NULL;
			error_lineno = 0;
			break;
	}
	if (!error_filename) {
		error_filename = "Unknown";
	}

#ifdef HAVE_DTRACE
	if(DTRACE_ERROR_ENABLED()) {
		char *dtrace_error_buffer;
		va_start(args, format);
		zend_vspprintf(&dtrace_error_buffer, 0, format, args);
		DTRACE_ERROR(dtrace_error_buffer, (char *)error_filename, error_lineno);
		efree(dtrace_error_buffer);
		va_end(args);
	}
#endif /* HAVE_DTRACE */

	va_start(args, format);

	/* if we don't have a user defined error handler */
	if (Z_TYPE(EG(user_error_handler)) == IS_UNDEF
		|| !(EG(user_error_handler_error_reporting) & type)
		|| EG(error_handling) != EH_NORMAL) {
		zend_error_cb(type, error_filename, error_lineno, format, args);
	} else switch (type) {
		case E_ERROR:
		case E_PARSE:
		case E_CORE_ERROR:
		case E_CORE_WARNING:
		case E_COMPILE_ERROR:
		case E_COMPILE_WARNING:
			/* The error may not be safe to handle in user-space */
			zend_error_cb(type, error_filename, error_lineno, format, args);
			break;
		default:
			/* Handle the error in user space */
/* va_copy() is __va_copy() in old gcc versions.
 * According to the autoconf manual, using
 * memcpy(&dst, &src, sizeof(va_list))
 * gives maximum portability. */
#ifndef va_copy
# ifdef __va_copy
#  define va_copy(dest, src)	__va_copy((dest), (src))
# else
#  define va_copy(dest, src)	memcpy(&(dest), &(src), sizeof(va_list))
# endif
#endif
			va_copy(usr_copy, args);
			len = zend_vspprintf(&str, 0, format, usr_copy);
			ZVAL_NEW_STR(&params[1], zend_string_init(str, len, 0));
			efree(str);
#ifdef va_copy
			va_end(usr_copy);
#endif

			ZVAL_LONG(&params[0], type);

			if (error_filename) {
				ZVAL_STRING(&params[2], error_filename);
			} else {
				ZVAL_NULL(&params[2]);
			}

			ZVAL_LONG(&params[3], error_lineno);

			symbol_table = zend_rebuild_symbol_table(TSRMLS_C);

			/* during shutdown the symbol table table can be still null */
			if (!symbol_table) {
				ZVAL_NULL(&params[4]);
			} else {
				ZVAL_NEW_ARR(&params[4]);
				zend_array_dup(Z_ARRVAL(params[4]), &symbol_table->ht);
			}

			ZVAL_COPY_VALUE(&orig_user_error_handler, &EG(user_error_handler));
			ZVAL_UNDEF(&EG(user_error_handler));

			/* User error handler may include() additinal PHP files.
			 * If an error was generated during comilation PHP will compile
			 * such scripts recursivly, but some CG() variables may be
			 * inconsistent. */

			in_compilation = CG(in_compilation);
			if (in_compilation) {
				saved_class_entry = CG(active_class_entry);
				CG(active_class_entry) = NULL;
				SAVE_STACK(loop_var_stack);
				SAVE_STACK(delayed_oplines_stack);
				SAVE_STACK(context_stack);
				CG(in_compilation) = 0;
			}

			ZVAL_UNDEF(&retval);
			if (call_user_function_ex(CG(function_table), NULL, &orig_user_error_handler, &retval, 5, params, 1, NULL TSRMLS_CC) == SUCCESS) {
				if (Z_TYPE(retval) != IS_UNDEF) {
					if (Z_TYPE(retval) == IS_FALSE) {
						zend_error_cb(type, error_filename, error_lineno, format, args);
					}
					zval_ptr_dtor(&retval);
				}
			} else if (!EG(exception)) {
				/* The user error handler failed, use built-in error handler */
				zend_error_cb(type, error_filename, error_lineno, format, args);
			}

			if (in_compilation) {
				CG(active_class_entry) = saved_class_entry;
				RESTORE_STACK(loop_var_stack);
				RESTORE_STACK(delayed_oplines_stack);
				RESTORE_STACK(context_stack);
				CG(in_compilation) = 1;
			}

			zval_ptr_dtor(&params[4]);
			zval_ptr_dtor(&params[3]);
			zval_ptr_dtor(&params[2]);
			zval_ptr_dtor(&params[1]);
			zval_ptr_dtor(&params[0]);

			if (Z_TYPE(EG(user_error_handler)) == IS_UNDEF) {
				ZVAL_COPY_VALUE(&EG(user_error_handler), &orig_user_error_handler);
			} else {
				zval_ptr_dtor(&orig_user_error_handler);
			}
			break;
	}

	va_end(args);

	if (type == E_PARSE) {
		/* eval() errors do not affect exit_status */
		if (!(EG(current_execute_data) &&
			EG(current_execute_data)->func &&
			ZEND_USER_CODE(EG(current_execute_data)->func->type) &&
			EG(current_execute_data)->opline->opcode == ZEND_INCLUDE_OR_EVAL &&
			EG(current_execute_data)->opline->extended_value == ZEND_EVAL)) {
			EG(exit_status) = 255;
		}
	}
}
/* }}} */

#if defined(__GNUC__) && __GNUC__ >= 3 && !defined(__INTEL_COMPILER) && !defined(DARWIN) && !defined(__hpux) && !defined(_AIX) && !defined(__osf__)
void zend_error_noreturn(int type, const char *format, ...) __attribute__ ((alias("zend_error"),noreturn));
#endif

ZEND_API void zend_output_debug_string(zend_bool trigger_break, const char *format, ...) /* {{{ */
{
#if ZEND_DEBUG
	va_list args;

	va_start(args, format);
#	ifdef ZEND_WIN32
	{
		char output_buf[1024];

		vsnprintf(output_buf, 1024, format, args);
		OutputDebugString(output_buf);
		OutputDebugString("\n");
		if (trigger_break && IsDebuggerPresent()) {
			DebugBreak();
		}
	}
#	else
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
#	endif
	va_end(args);
#endif
}
/* }}} */

ZEND_API int zend_execute_scripts(int type TSRMLS_DC, zval *retval, int file_count, ...) /* {{{ */
{
	va_list files;
	int i;
	zend_file_handle *file_handle;
	zend_op_array *op_array;

	va_start(files, file_count);
	for (i = 0; i < file_count; i++) {
		file_handle = va_arg(files, zend_file_handle *);
		if (!file_handle) {
			continue;
		}
       
		op_array = zend_compile_file(file_handle, type TSRMLS_CC);
		if (file_handle->opened_path) {
			zend_hash_str_add_empty_element(&EG(included_files), file_handle->opened_path, strlen(file_handle->opened_path));
		}
		zend_destroy_file_handle(file_handle TSRMLS_CC);
		if (op_array) {
			zend_execute(op_array, retval TSRMLS_CC);
			zend_exception_restore(TSRMLS_C);
			if (EG(exception)) {
				if (Z_TYPE(EG(user_exception_handler)) != IS_UNDEF) {
					zval orig_user_exception_handler;
					zval params[1], retval2;
					zend_object *old_exception;
					old_exception = EG(exception);
					EG(exception) = NULL;
					ZVAL_OBJ(&params[0], old_exception);
					ZVAL_COPY_VALUE(&orig_user_exception_handler, &EG(user_exception_handler));
					ZVAL_UNDEF(&retval2);
					if (call_user_function_ex(CG(function_table), NULL, &orig_user_exception_handler, &retval2, 1, params, 1, NULL TSRMLS_CC) == SUCCESS) {
						zval_ptr_dtor(&retval2);
						if (EG(exception)) {
							OBJ_RELEASE(EG(exception));
							EG(exception) = NULL;
						}
						OBJ_RELEASE(old_exception);
					} else {
						EG(exception) = old_exception;
						zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
					}
				} else {
					zend_exception_error(EG(exception), E_ERROR TSRMLS_CC);
				}
			}
			destroy_op_array(op_array TSRMLS_CC);
			efree_size(op_array, sizeof(zend_op_array));
		} else if (type==ZEND_REQUIRE) {
			va_end(files);
			return FAILURE;
		}
	}
	va_end(files);

	return SUCCESS;
}
/* }}} */

#define COMPILED_STRING_DESCRIPTION_FORMAT "%s(%d) : %s"

ZEND_API char *zend_make_compiled_string_description(const char *name TSRMLS_DC) /* {{{ */
{
	const char *cur_filename;
	int cur_lineno;
	char *compiled_string_description;

	if (zend_is_compiling(TSRMLS_C)) {
		cur_filename = zend_get_compiled_filename(TSRMLS_C)->val;
		cur_lineno = zend_get_compiled_lineno(TSRMLS_C);
	} else if (zend_is_executing(TSRMLS_C)) {
		cur_filename = zend_get_executed_filename(TSRMLS_C);
		cur_lineno = zend_get_executed_lineno(TSRMLS_C);
	} else {
		cur_filename = "Unknown";
		cur_lineno = 0;
	}

	zend_spprintf(&compiled_string_description, 0, COMPILED_STRING_DESCRIPTION_FORMAT, cur_filename, cur_lineno, name);
	return compiled_string_description;
}
/* }}} */

void free_estring(char **str_p) /* {{{ */
{
	efree(*str_p);
}

void free_string_zval(zval *zv) /* {{{ */
{
	zend_string *str = Z_PTR_P(zv);
	zend_string_release(str);
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
