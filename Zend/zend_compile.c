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
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include <zend_language_parser.h>
#include "zend.h"
#include "zend_compile.h"
#include "zend_constants.h"
#include "zend_llist.h"
#include "zend_API.h"
#include "zend_exceptions.h"
#include "zend_virtual_cwd.h"
#include "zend_multibyte.h"
#include "zend_language_scanner.h"

#define CONSTANT_EX(op_array, op) \
	(op_array)->literals[op]

#define CONSTANT(op) \
	CONSTANT_EX(CG(active_op_array), op)

#define SET_NODE(target, src) do { \
		target ## _type = (src)->op_type; \
		if ((src)->op_type == IS_CONST) { \
			target.constant = zend_add_literal(CG(active_op_array), &(src)->u.constant TSRMLS_CC); \
		} else { \
			target = (src)->u.op; \
		} \
	} while (0)

#define GET_NODE(target, src) do { \
		(target)->op_type = src ## _type; \
		if ((target)->op_type == IS_CONST) { \
			(target)->u.constant = CONSTANT(src.constant); \
		} else { \
			(target)->u.op = src; \
		} \
	} while (0)

#define COPY_NODE(target, src) do { \
		target ## _type = src ## _type; \
		target = src; \
	} while (0)

#define GET_CACHE_SLOT(literal) do { \
		Z_CACHE_SLOT(CG(active_op_array)->literals[literal]) = CG(active_op_array)->last_cache_slot++; \
		if ((CG(active_op_array)->fn_flags & ZEND_ACC_INTERACTIVE) && CG(active_op_array)->run_time_cache) { \
			CG(active_op_array)->run_time_cache = erealloc(CG(active_op_array)->run_time_cache, CG(active_op_array)->last_cache_slot * sizeof(void*)); \
			CG(active_op_array)->run_time_cache[CG(active_op_array)->last_cache_slot - 1] = NULL; \
		} \
	} while (0)

#define POLYMORPHIC_CACHE_SLOT_SIZE 2

#define GET_POLYMORPHIC_CACHE_SLOT(literal) do { \
		Z_CACHE_SLOT(CG(active_op_array)->literals[literal]) = CG(active_op_array)->last_cache_slot; \
		CG(active_op_array)->last_cache_slot += POLYMORPHIC_CACHE_SLOT_SIZE; \
		if ((CG(active_op_array)->fn_flags & ZEND_ACC_INTERACTIVE) && CG(active_op_array)->run_time_cache) { \
			CG(active_op_array)->run_time_cache = erealloc(CG(active_op_array)->run_time_cache, CG(active_op_array)->last_cache_slot * sizeof(void*)); \
			CG(active_op_array)->run_time_cache[CG(active_op_array)->last_cache_slot - 1] = NULL; \
			CG(active_op_array)->run_time_cache[CG(active_op_array)->last_cache_slot - 2] = NULL; \
		} \
	} while (0)

#define FREE_POLYMORPHIC_CACHE_SLOT(literal) do { \
		if (Z_CACHE_SLOT(CG(active_op_array)->literals[literal]) != -1 && \
		    Z_CACHE_SLOT(CG(active_op_array)->literals[literal]) == \
		    CG(active_op_array)->last_cache_slot - POLYMORPHIC_CACHE_SLOT_SIZE) { \
			Z_CACHE_SLOT(CG(active_op_array)->literals[literal]) = -1; \
			CG(active_op_array)->last_cache_slot -= POLYMORPHIC_CACHE_SLOT_SIZE; \
		} \
	} while (0)

ZEND_API zend_op_array *(*zend_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);
ZEND_API zend_op_array *(*zend_compile_string)(zval *source_string, char *filename TSRMLS_DC);

#ifndef ZTS
ZEND_API zend_compiler_globals compiler_globals;
ZEND_API zend_executor_globals executor_globals;
#endif

static zend_property_info *zend_duplicate_property_info(zend_property_info *property_info TSRMLS_DC) /* {{{ */
{
	zend_property_info* new_property_info;
	
	new_property_info = zend_arena_alloc(&CG(arena), sizeof(zend_property_info));
	memcpy(new_property_info, property_info, sizeof(zend_property_info));
	STR_ADDREF(new_property_info->name);
	if (new_property_info->doc_comment) {
		STR_ADDREF(new_property_info->doc_comment);
	}
	return new_property_info;
}
/* }}} */

static zend_property_info *zend_duplicate_property_info_internal(zend_property_info *property_info) /* {{{ */
{
	zend_property_info* new_property_info = pemalloc(sizeof(zend_property_info), 1);
	memcpy(new_property_info, property_info, sizeof(zend_property_info));
	STR_ADDREF(new_property_info->name);
	return new_property_info;
}
/* }}} */

static void zend_destroy_property_info(zval *zv) /* {{{ */
{
	zend_property_info *property_info = Z_PTR_P(zv);

	STR_RELEASE(property_info->name);
	if (property_info->doc_comment) {
		STR_RELEASE(property_info->doc_comment);
	}
}
/* }}} */

static void zend_destroy_property_info_internal(zval *zv) /* {{{ */
{
	zend_property_info *property_info = Z_PTR_P(zv);

	STR_RELEASE(property_info->name);
	free(property_info);
}
/* }}} */

static zend_string *zend_new_interned_string_safe(zend_string *str TSRMLS_DC) {
	zend_string *interned_str;

	STR_ADDREF(str);
	interned_str = zend_new_interned_string(str TSRMLS_CC);
	if (str != interned_str) {
		return interned_str;
	} else {
		STR_RELEASE(str);
		return str;
	}
}

static void build_runtime_defined_function_key(zval *result, zend_string *name, unsigned char *lex_pos TSRMLS_DC) /* {{{ */
{
	char char_pos_buf[32];
	uint char_pos_len;
	const char *filename;

	char_pos_len = zend_sprintf(char_pos_buf, "%p", lex_pos);
	if (CG(active_op_array)->filename) {
		filename = CG(active_op_array)->filename->val;
	} else {
		filename = "-";
	}

	/* NULL, name length, filename length, last accepting char position length */
	ZVAL_NEW_STR(result, STR_ALLOC(1 + name->len + strlen(filename) + char_pos_len, 0));

 	/* must be binary safe */
 	Z_STRVAL_P(result)[0] = '\0';
 	sprintf(Z_STRVAL_P(result)+1, "%s%s%s", name->val, filename, char_pos_buf);
}
/* }}} */

static void init_compiler_declarables(TSRMLS_D) /* {{{ */
{
	ZVAL_LONG(&CG(declarables).ticks, 0);
}
/* }}} */

void zend_init_compiler_context(TSRMLS_D) /* {{{ */
{
	CG(context).opcodes_size = (CG(active_op_array)->fn_flags & ZEND_ACC_INTERACTIVE) ? INITIAL_INTERACTIVE_OP_ARRAY_SIZE : INITIAL_OP_ARRAY_SIZE;
	CG(context).vars_size = 0;
	CG(context).literals_size = 0;
	CG(context).current_brk_cont = -1;
	CG(context).backpatch_count = 0;
	CG(context).in_finally = 0;
	CG(context).labels = NULL;
}
/* }}} */

void zend_init_compiler_data_structures(TSRMLS_D) /* {{{ */
{
	zend_stack_init(&CG(bp_stack), sizeof(zend_llist));
	zend_stack_init(&CG(function_call_stack), sizeof(zend_function_call_entry));
	zend_stack_init(&CG(switch_cond_stack), sizeof(zend_switch_entry));
	zend_stack_init(&CG(foreach_copy_stack), sizeof(zend_op));
	CG(active_class_entry) = NULL;
	zend_llist_init(&CG(list_llist), sizeof(list_llist_element), NULL, 0);
	zend_llist_init(&CG(dimension_llist), sizeof(int), NULL, 0);
	zend_stack_init(&CG(list_stack), sizeof(zend_llist));
	CG(in_compilation) = 0;
	CG(start_lineno) = 0;
	ZVAL_UNDEF(&CG(current_namespace));
	CG(in_namespace) = 0;
	CG(has_bracketed_namespaces) = 0;
	CG(current_import) = NULL;
	CG(current_import_function) = NULL;
	CG(current_import_const) = NULL;
	zend_hash_init(&CG(const_filenames), 8, NULL, NULL, 0);
	init_compiler_declarables(TSRMLS_C);
	zend_stack_init(&CG(context_stack), sizeof(CG(context)));

	CG(encoding_declared) = 0;
}
/* }}} */

ZEND_API void file_handle_dtor(zend_file_handle *fh) /* {{{ */
{
	TSRMLS_FETCH();

	zend_file_handle_dtor(fh TSRMLS_CC);
}
/* }}} */

void init_compiler(TSRMLS_D) /* {{{ */
{
	CG(arena) = zend_arena_create(64 * 1024);
	CG(active_op_array) = NULL;
	memset(&CG(context), 0, sizeof(CG(context)));
	zend_init_compiler_data_structures(TSRMLS_C);
	zend_init_rsrc_list(TSRMLS_C);
	zend_hash_init(&CG(filenames_table), 8, NULL, free_string_zval, 0);
	zend_llist_init(&CG(open_files), sizeof(zend_file_handle), (void (*)(void *)) file_handle_dtor, 0);
	CG(unclean_shutdown) = 0;
}
/* }}} */

void shutdown_compiler(TSRMLS_D) /* {{{ */
{
	zend_stack_destroy(&CG(bp_stack));
	zend_stack_destroy(&CG(function_call_stack));
	zend_stack_destroy(&CG(switch_cond_stack));
	zend_stack_destroy(&CG(foreach_copy_stack));
	zend_stack_destroy(&CG(list_stack));
	zend_hash_destroy(&CG(filenames_table));
	zend_hash_destroy(&CG(const_filenames));
	zend_stack_destroy(&CG(context_stack));
	zend_arena_destroy(CG(arena));
}
/* }}} */

ZEND_API zend_string *zend_set_compiled_filename(zend_string *new_compiled_filename TSRMLS_DC) /* {{{ */
{
	zend_string *p;

	p = zend_hash_find_ptr(&CG(filenames_table), new_compiled_filename);
	if (p != NULL) {
		CG(compiled_filename) = p;
		return p;
	}
	p = STR_COPY(new_compiled_filename);
	zend_hash_update_ptr(&CG(filenames_table), new_compiled_filename, p);
	CG(compiled_filename) = p;
	return p;
}
/* }}} */

ZEND_API void zend_restore_compiled_filename(zend_string *original_compiled_filename TSRMLS_DC) /* {{{ */
{
	CG(compiled_filename) = original_compiled_filename;
}
/* }}} */

ZEND_API zend_string *zend_get_compiled_filename(TSRMLS_D) /* {{{ */
{
	return CG(compiled_filename);
}
/* }}} */

ZEND_API int zend_get_compiled_lineno(TSRMLS_D) /* {{{ */
{
	return CG(zend_lineno);
}
/* }}} */

ZEND_API zend_bool zend_is_compiling(TSRMLS_D) /* {{{ */
{
	return CG(in_compilation);
}
/* }}} */

static zend_uint get_temporary_variable(zend_op_array *op_array) /* {{{ */
{
	return (zend_uint)op_array->T++;
}
/* }}} */

static int lookup_cv(zend_op_array *op_array, zend_string* name TSRMLS_DC) /* {{{ */{
	int i = 0;
	ulong hash_value = STR_HASH_VAL(name);

	while (i < op_array->last_var) {
		if (op_array->vars[i]->val == name->val ||
		    (op_array->vars[i]->h == hash_value &&
		     op_array->vars[i]->len == name->len &&
		     memcmp(op_array->vars[i]->val, name->val, name->len) == 0)) {
			STR_RELEASE(name);
			return (int)(zend_intptr_t)EX_VAR_NUM_2(NULL, i);
		}
		i++;
	}
	i = op_array->last_var;
	op_array->last_var++;
	if (op_array->last_var > CG(context).vars_size) {
		CG(context).vars_size += 16; /* FIXME */
		op_array->vars = erealloc(op_array->vars, CG(context).vars_size * sizeof(zend_string*));
	}

	op_array->vars[i] = zend_new_interned_string(name TSRMLS_CC);
	return (int)(zend_intptr_t)EX_VAR_NUM_2(NULL, i);
}
/* }}} */

void zend_del_literal(zend_op_array *op_array, int n) /* {{{ */
{
	zval_dtor(&CONSTANT_EX(op_array, n));
	if (n + 1 == op_array->last_literal) {
		op_array->last_literal--;
	} else {
		ZVAL_UNDEF(&CONSTANT_EX(op_array, n));
	}
}
/* }}} */

/* Common part of zend_add_literal and zend_append_individual_literal */
static inline void zend_insert_literal(zend_op_array *op_array, zval *zv, int literal_position TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_CONSTANT) {
		STR_HASH_VAL(Z_STR_P(zv));
		Z_STR_P(zv) = zend_new_interned_string(Z_STR_P(zv) TSRMLS_CC);
		if (IS_INTERNED(Z_STR_P(zv))) {
			Z_TYPE_FLAGS_P(zv) &= ~ (IS_TYPE_REFCOUNTED | IS_TYPE_COPYABLE);
		}
	}
	ZVAL_COPY_VALUE(&CONSTANT_EX(op_array, literal_position), zv);
	Z_CACHE_SLOT(op_array->literals[literal_position]) = -1;
}
/* }}} */

/* Is used while compiling a function, using the context to keep track
   of an approximate size to avoid to relocate to often.
   Literals are truncated to actual size in the second compiler pass (pass_two()). */
int zend_add_literal(zend_op_array *op_array, zval *zv TSRMLS_DC) /* {{{ */
{
	int i = op_array->last_literal;
	op_array->last_literal++;
	if (i >= CG(context).literals_size) {
		while (i >= CG(context).literals_size) {
			CG(context).literals_size += 16; /* FIXME */
		}
		op_array->literals = (zval*)erealloc(op_array->literals, CG(context).literals_size * sizeof(zval));
	}
	zend_insert_literal(op_array, zv, i TSRMLS_CC);
	return i;
}
/* }}} */

static int zend_add_func_name_literal(zend_op_array *op_array, zval *zv TSRMLS_DC) /* {{{ */
{
	int ret;
	zend_string *lc_name;
	zval c;

	if (op_array->last_literal > 0 &&
	    &op_array->literals[op_array->last_literal - 1] == zv &&
	    Z_CACHE_SLOT(op_array->literals[op_array->last_literal - 1]) == -1) {
		/* we already have function name as last literal (do nothing) */
		ret = op_array->last_literal - 1;
	} else {
		ret = zend_add_literal(op_array, zv TSRMLS_CC);
	}

	lc_name = STR_ALLOC(Z_STRLEN_P(zv), 0);
	zend_str_tolower_copy(lc_name->val, Z_STRVAL_P(zv), Z_STRLEN_P(zv));
	ZVAL_NEW_STR(&c, lc_name);
	zend_add_literal(CG(active_op_array), &c TSRMLS_CC);

	return ret;
}
/* }}} */

static int zend_add_ns_func_name_literal(zend_op_array *op_array, zval *zv TSRMLS_DC) /* {{{ */
{
	int ret;
	zend_string *lc_name;
	const char *ns_separator;
	int lc_len;
	zval c;

	if (op_array->last_literal > 0 &&
	    &op_array->literals[op_array->last_literal - 1] == zv &&
	    Z_CACHE_SLOT(op_array->literals[op_array->last_literal - 1]) == -1) {
		/* we already have function name as last literal (do nothing) */
		ret = op_array->last_literal - 1;
	} else {
		ret = zend_add_literal(op_array, zv TSRMLS_CC);
	}

	lc_name = STR_ALLOC(Z_STRLEN_P(zv), 0);
	zend_str_tolower_copy(lc_name->val, Z_STRVAL_P(zv), Z_STRLEN_P(zv));
	ZVAL_NEW_STR(&c, lc_name);
	zend_add_literal(CG(active_op_array), &c TSRMLS_CC);

	ns_separator = (const char*)zend_memrchr(Z_STRVAL_P(zv), '\\', Z_STRLEN_P(zv));

	if (ns_separator != NULL) {
		ns_separator += 1;
		lc_len = Z_STRLEN_P(zv) - (ns_separator - Z_STRVAL_P(zv));
		lc_name = STR_ALLOC(lc_len, 0);
		zend_str_tolower_copy(lc_name->val, ns_separator, lc_len);
		ZVAL_NEW_STR(&c, lc_name);
		zend_add_literal(CG(active_op_array), &c TSRMLS_CC);
	}

	return ret;
}
/* }}} */

static int zend_add_class_name_literal(zend_op_array *op_array, zend_string *name TSRMLS_DC) {
	int ret;
	zend_string *lc_name;

	zval zv;
	ZVAL_STR(&zv, name);

	ret = zend_add_literal(op_array, &zv TSRMLS_CC);

	lc_name = STR_ALLOC(name->len, 0);
	zend_str_tolower_copy(lc_name->val, name->val, name->len);

	ZVAL_NEW_STR(&zv, lc_name);
	zend_add_literal(CG(active_op_array), &zv TSRMLS_CC);

	GET_CACHE_SLOT(ret);

	return ret;
}
/* }}} */

static int zend_add_const_name_literal(zend_op_array *op_array, zval *zv, int unqualified TSRMLS_DC) /* {{{ */
{
	int ret;
	char *name;
	zend_string *tmp_name;
	const char *ns_separator;
	int name_len, ns_len;
	zval c;

	if (op_array->last_literal > 0 &&
	    &op_array->literals[op_array->last_literal - 1] == zv &&
	    Z_CACHE_SLOT(op_array->literals[op_array->last_literal - 1]) == -1) {
		/* we already have function name as last literal (do nothing) */
		ret = op_array->last_literal - 1;
	} else {
		ret = zend_add_literal(op_array, zv TSRMLS_CC);
	}

	/* skip leading '\\' */
	if (Z_STRVAL_P(zv)[0] == '\\') {
		name_len = Z_STRLEN_P(zv) - 1;
		name = Z_STRVAL_P(zv) + 1;
	} else {
		name_len = Z_STRLEN_P(zv);
		name = Z_STRVAL_P(zv);
	}
	ns_separator = zend_memrchr(name, '\\', name_len);
	if (ns_separator) {
		ns_len = ns_separator - name;
	} else {
		ns_len = 0;
	}

	if (ns_len) {
		/* lowercased namespace name & original constant name */
		tmp_name = STR_INIT(name, name_len, 0);
		zend_str_tolower(tmp_name->val, ns_len);
		ZVAL_NEW_STR(&c, tmp_name);
		zend_add_literal(CG(active_op_array), &c TSRMLS_CC);

		/* lowercased namespace name & lowercased constant name */
		tmp_name = STR_ALLOC(name_len, 0);
		zend_str_tolower_copy(tmp_name->val, name, name_len);
		ZVAL_NEW_STR(&c, tmp_name);
		zend_add_literal(CG(active_op_array), &c TSRMLS_CC);
	}

	if (ns_len) {
		if (!unqualified) {
			return ret;
		}
		ns_len++;
		name += ns_len;
		name_len -= ns_len;
	}

	/* original constant name */
	tmp_name = STR_INIT(name, name_len, 0);
	ZVAL_NEW_STR(&c, tmp_name);
	zend_add_literal(CG(active_op_array), &c TSRMLS_CC);

	/* lowercased constant name */
	tmp_name = STR_ALLOC(name_len, 0);
	zend_str_tolower_copy(tmp_name->val, name, name_len);
	ZVAL_NEW_STR(&c, tmp_name);
	zend_add_literal(CG(active_op_array), &c TSRMLS_CC);

	return ret;
}
/* }}} */

#define LITERAL_STR(op, str) do { \
		zval _c; \
		ZVAL_STR(&_c, str); \
		op.constant = zend_add_literal(CG(active_op_array), &_c TSRMLS_CC); \
	} while (0)

#define LITERAL_STRINGL(op, str, len) do { \
		zval _c; \
		ZVAL_STRINGL(&_c, str, len); \
		op.constant = zend_add_literal(CG(active_op_array), &_c TSRMLS_CC); \
	} while (0)

#define LITERAL_LONG(op, val) do { \
		zval _c; \
		ZVAL_LONG(&_c, val); \
		op.constant = zend_add_literal(CG(active_op_array), &_c TSRMLS_CC); \
	} while (0)

#define LITERAL_LONG_EX(op_array, op, val) do { \
		zval _c; \
		ZVAL_LONG(&_c, val); \
		op.constant = zend_add_literal(op_array, &_c TSRMLS_CC); \
	} while (0)

#define LITERAL_NULL(op) do { \
		zval _c; \
		ZVAL_NULL(&_c); \
		op.constant = zend_add_literal(CG(active_op_array), &_c TSRMLS_CC); \
	} while (0)

#define MAKE_NOP(opline)	{ opline->opcode = ZEND_NOP;  memset(&opline->result,0,sizeof(opline->result)); memset(&opline->op1,0,sizeof(opline->op1)); memset(&opline->op2,0,sizeof(opline->op2)); opline->result_type=opline->op1_type=opline->op2_type=IS_UNUSED;  }

void zend_stop_lexing(TSRMLS_D) {
	LANG_SCNG(yy_cursor) = LANG_SCNG(yy_limit);
}

void zend_do_abstract_method(const znode *function_name, znode *modifiers, const znode *body TSRMLS_DC) /* {{{ */
{
	char *method_type;

	if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
		Z_LVAL(modifiers->u.constant) |= ZEND_ACC_ABSTRACT;
		method_type = "Interface";
	} else {
		method_type = "Abstract";
	}

	if (Z_LVAL(modifiers->u.constant) & ZEND_ACC_ABSTRACT) {
		if(Z_LVAL(modifiers->u.constant) & ZEND_ACC_PRIVATE) {
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot be declared private", method_type, CG(active_class_entry)->name->val, Z_STRVAL(function_name->u.constant));
		}
		if (Z_LVAL(body->u.constant) == ZEND_ACC_ABSTRACT) {
			zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

			opline->opcode = ZEND_RAISE_ABSTRACT_ERROR;
			SET_UNUSED(opline->op1);
			SET_UNUSED(opline->op2);
		} else {
			/* we had code in the function body */
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot contain body", method_type, CG(active_class_entry)->name->val, Z_STRVAL(function_name->u.constant));
		}
	} else {
		if (Z_LVAL(body->u.constant) == ZEND_ACC_ABSTRACT) {
			zend_error_noreturn(E_COMPILE_ERROR, "Non-abstract method %s::%s() must contain body", CG(active_class_entry)->name->val, Z_STRVAL(function_name->u.constant));
		}
	}
}
/* }}} */

static inline void do_begin_loop(TSRMLS_D) /* {{{ */
{
	zend_brk_cont_element *brk_cont_element;
	int parent;

	parent = CG(context).current_brk_cont;
	CG(context).current_brk_cont = CG(active_op_array)->last_brk_cont;
	brk_cont_element = get_next_brk_cont_element(CG(active_op_array));
	brk_cont_element->start = get_next_op_number(CG(active_op_array));
	brk_cont_element->parent = parent;
}
/* }}} */

static inline void do_end_loop(int cont_addr, int has_loop_var TSRMLS_DC) /* {{{ */
{
	if (!has_loop_var) {
		/* The start fileld is used to free temporary variables in case of exceptions.
		 * We won't try to free something of we don't have loop variable.
		 */
		CG(active_op_array)->brk_cont_array[CG(context).current_brk_cont].start = -1;
	}
	CG(active_op_array)->brk_cont_array[CG(context).current_brk_cont].cont = cont_addr;
	CG(active_op_array)->brk_cont_array[CG(context).current_brk_cont].brk = get_next_op_number(CG(active_op_array));
	CG(context).current_brk_cont = CG(active_op_array)->brk_cont_array[CG(context).current_brk_cont].parent;
}
/* }}} */

void zend_do_free(znode *op1 TSRMLS_DC) /* {{{ */
{
	if (op1->op_type==IS_TMP_VAR) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_FREE;
		SET_NODE(opline->op1, op1);
		SET_UNUSED(opline->op2);
	} else if (op1->op_type==IS_VAR) {
		zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];

		while (opline->opcode == ZEND_END_SILENCE || opline->opcode == ZEND_EXT_FCALL_END || opline->opcode == ZEND_OP_DATA) {
			opline--;
		}
		if (opline->result_type == IS_VAR
			&& opline->result.var == op1->u.op.var) {
			if (opline->opcode == ZEND_FETCH_R ||
			    opline->opcode == ZEND_FETCH_DIM_R ||
			    opline->opcode == ZEND_FETCH_OBJ_R ||
			    opline->opcode == ZEND_QM_ASSIGN_VAR) {
				/* It's very rare and useless case. It's better to use
				   additional FREE opcode and simplify the FETCH handlers
				   their selves */
				opline = get_next_op(CG(active_op_array) TSRMLS_CC);
				opline->opcode = ZEND_FREE;
				SET_NODE(opline->op1, op1);
				SET_UNUSED(opline->op2);
			} else {
				opline->result_type |= EXT_TYPE_UNUSED;
			}
		} else {
			while (opline>CG(active_op_array)->opcodes) {
				if (opline->opcode == ZEND_FETCH_DIM_R
				    && opline->op1_type == IS_VAR
				    && opline->op1.var == op1->u.op.var) {
					/* This should the end of a list() construct
					 * Mark its result as unused
					 */
					opline->extended_value = ZEND_FETCH_STANDARD;
					break;
				} else if (opline->result_type==IS_VAR
					&& opline->result.var == op1->u.op.var) {
					if (opline->opcode == ZEND_NEW) {
						opline->result_type |= EXT_TYPE_UNUSED;
						opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
						while (opline->opcode != ZEND_DO_FCALL || opline->op1.num != ZEND_CALL_CTOR) {
							opline--;
						}
						opline->op1.num |= ZEND_CALL_CTOR_RESULT_UNUSED;
					}
					break;
				}
				opline--;
			}
		}
	} else if (op1->op_type == IS_CONST) {
		zval_dtor(&op1->u.constant);
	}
}
/* }}} */

zend_uint zend_add_member_modifier(zend_uint flags, zend_uint new_flag) {
	zend_uint new_flags = flags | new_flag;
	if ((flags & ZEND_ACC_PPP_MASK) && (new_flag & ZEND_ACC_PPP_MASK)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Multiple access type modifiers are not allowed");
	}
	if ((flags & ZEND_ACC_ABSTRACT) && (new_flag & ZEND_ACC_ABSTRACT)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Multiple abstract modifiers are not allowed");
	}
	if ((flags & ZEND_ACC_STATIC) && (new_flag & ZEND_ACC_STATIC)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Multiple static modifiers are not allowed");
	}
	if ((flags & ZEND_ACC_FINAL) && (new_flag & ZEND_ACC_FINAL)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Multiple final modifiers are not allowed");
	}
	if ((new_flags & ZEND_ACC_ABSTRACT) && (new_flags & ZEND_ACC_FINAL)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use the final modifier on an abstract class member");
	}
	return new_flags;
}

void zend_do_handle_exception(TSRMLS_D) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_HANDLE_EXCEPTION;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

zend_string *zend_concat3(
	char *str1, size_t str1_len, char *str2, size_t str2_len, char *str3, size_t str3_len
) {
	size_t len = str1_len + str2_len + str3_len;
	zend_string *res = STR_ALLOC(len, 0);

	memcpy(res->val, str1, str1_len);
	memcpy(res->val + str1_len, str2, str2_len);
	memcpy(res->val + str1_len + str2_len, str3, str3_len);
	res->val[len] = '\0';

	return res;
}

zend_string *zend_concat_names(char *name1, size_t name1_len, char *name2, size_t name2_len) {
	return zend_concat3(name1, name1_len, "\\", 1, name2, name2_len);
}

zend_string *zend_prefix_with_ns(zend_string *name TSRMLS_DC) {
	if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
		zend_string *ns = Z_STR(CG(current_namespace));
		return zend_concat_names(ns->val, ns->len, name->val, name->len);
	} else {
		return STR_COPY(name);
	}
}

void *zend_hash_find_ptr_lc(HashTable *ht, char *str, size_t len) {
	void *result;
	zend_string *lcname = STR_ALLOC(len, 0);
	zend_str_tolower_copy(lcname->val, str, len);
	result = zend_hash_find_ptr(ht, lcname);
	STR_FREE(lcname);
	return result;
}

zend_string *zend_resolve_non_class_name(
	zend_string *name, zend_uint type, zend_bool *is_fully_qualified,
	zend_bool case_sensitive, HashTable *current_import_sub TSRMLS_DC
) {
	char *compound;
	*is_fully_qualified = 0;

	if (name->val[0] == '\\') {
		/* Remove \ prefix (only relevant if this is a string rather than a label) */
		return STR_INIT(name->val + 1, name->len - 1, 0);
	}

	if (type == ZEND_NAME_FQ) {
		*is_fully_qualified = 1;
		return STR_COPY(name);
	}

	if (type == ZEND_NAME_RELATIVE) {
		*is_fully_qualified = 1;
		return zend_prefix_with_ns(name TSRMLS_CC);
	}

	if (current_import_sub) {
		/* If an unqualified name is a function/const alias, replace it. */
		zend_string *import_name;
		if (case_sensitive) {
			import_name = zend_hash_find_ptr(current_import_sub, name);
		} else {
			import_name = zend_hash_find_ptr_lc(current_import_sub, name->val, name->len);
		}

		if (import_name) {
			*is_fully_qualified = 1;
			return STR_COPY(import_name);
		}
	}

	compound = memchr(name->val, '\\', name->len);
	if (compound) {
		*is_fully_qualified = 1;
	}

	if (compound && CG(current_import)) {
		/* If the first part of a qualified name is an alias, substitute it. */
		size_t len = compound - name->val;
		zend_string *import_name = zend_hash_find_ptr_lc(CG(current_import), name->val, len);

		if (import_name) {
			return zend_concat_names(
				import_name->val, import_name->len, name->val + len + 1, name->len - len - 1);
		}
	}

	return zend_prefix_with_ns(name TSRMLS_CC);
}
/* }}} */

zend_string *zend_resolve_function_name(
	zend_string *name, zend_uint type, zend_bool *is_fully_qualified TSRMLS_DC
) {
	return zend_resolve_non_class_name(
		name, type, is_fully_qualified, 0, CG(current_import_function) TSRMLS_CC);
}

zend_string *zend_resolve_const_name(
	zend_string *name, zend_uint type, zend_bool *is_fully_qualified TSRMLS_DC
) {
	return zend_resolve_non_class_name(
		name, type, is_fully_qualified, 1, CG(current_import_const) TSRMLS_CC);
}

zend_string *zend_resolve_class_name(
	zend_string *name, zend_uint type TSRMLS_DC
) {
	char *compound;

	if (type == ZEND_NAME_RELATIVE) {
		return zend_prefix_with_ns(name TSRMLS_CC);
	}

	if (type == ZEND_NAME_FQ || name->val[0] == '\\') {
		/* Remove \ prefix (only relevant if this is a string rather than a label) */
		if (name->val[0] == '\\') {
			name = STR_INIT(name->val + 1, name->len - 1, 0);
		} else {
			STR_ADDREF(name);
		}
		/* Ensure that \self, \parent and \static are not used */
		if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(name->val, name->len)) {
			zend_error_noreturn(E_COMPILE_ERROR, "'\\%s' is an invalid class name", name->val);
		}
		return name;
	}

	if (CG(current_import)) {
		compound = memchr(name->val, '\\', name->len);
		if (compound) {
			/* If the first part of a qualified name is an alias, substitute it. */
			size_t len = compound - name->val;
			zend_string *import_name = zend_hash_find_ptr_lc(CG(current_import), name->val, len);

			if (import_name) {
				return zend_concat_names(
					import_name->val, import_name->len, name->val + len + 1, name->len - len - 1);
			}
		} else {
			/* If an unqualified name is an alias, replace it. */
			zend_string *import_name
				= zend_hash_find_ptr_lc(CG(current_import), name->val, name->len);

			if (import_name) {
				return STR_COPY(import_name);
			}
		}
	}

	/* If not fully qualified and not an alias, prepend the current namespace */
	return zend_prefix_with_ns(name TSRMLS_CC);
}

zend_string *zend_resolve_class_name_ast(zend_ast *ast TSRMLS_DC) {
	zend_string *name = zend_ast_get_str(ast);
	return zend_resolve_class_name(name, ast->attr TSRMLS_CC);
}

static void ptr_dtor(zval *zv) /* {{{ */
{
	efree(Z_PTR_P(zv));
}
/* }}} */

static void str_dtor(zval *zv) {
	STR_RELEASE(Z_STR_P(zv));
}

void zend_resolve_goto_label(zend_op_array *op_array, zend_op *opline, int pass2 TSRMLS_DC) /* {{{ */
{
	zend_label *dest;
	long current, distance;
	zval *label;

	if (pass2) {
		label = opline->op2.zv;
	} else {
		label = &CONSTANT_EX(op_array, opline->op2.constant);
	}
	if (CG(context).labels == NULL ||
	    (dest = zend_hash_find_ptr(CG(context).labels, Z_STR_P(label))) == NULL) {

		if (pass2) {
			CG(in_compilation) = 1;
			CG(active_op_array) = op_array;
			CG(zend_lineno) = opline->lineno;
			zend_error_noreturn(E_COMPILE_ERROR, "'goto' to undefined label '%s'", Z_STRVAL_P(label));
		} else {
			/* Label is not defined. Delay to pass 2. */
			INC_BPC(op_array);
			return;
		}
	}

	opline->op1.opline_num = dest->opline_num;
	zval_dtor(label);
	ZVAL_NULL(label);

	/* Check that we are not moving into loop or switch */
	current = opline->extended_value;
	for (distance = 0; current != dest->brk_cont; distance++) {
		if (current == -1) {
			if (pass2) {
				CG(in_compilation) = 1;
				CG(active_op_array) = op_array;
				CG(zend_lineno) = opline->lineno;
			}
			zend_error_noreturn(E_COMPILE_ERROR, "'goto' into loop or switch statement is disallowed");
		}
		current = op_array->brk_cont_array[current].parent;
	}

	if (distance == 0) {
		/* Nothing to break out of, optimize to ZEND_JMP */
		opline->opcode = ZEND_JMP;
		opline->extended_value = 0;
		SET_UNUSED(opline->op2);
	} else {
		/* Set real break distance */
		ZVAL_LONG(label, distance);
	}

	if (pass2) {
		DEC_BPC(op_array);
	}
}
/* }}} */

void zend_release_labels(int temporary TSRMLS_DC) /* {{{ */
{
	if (CG(context).labels) {
		zend_hash_destroy(CG(context).labels);
		FREE_HASHTABLE(CG(context).labels);
		CG(context).labels = NULL;
	}
	if (!temporary && !zend_stack_is_empty(&CG(context_stack))) {
		zend_compiler_context *ctx = zend_stack_top(&CG(context_stack));
		CG(context) = *ctx;
		zend_stack_del_top(&CG(context_stack));
	}
}
/* }}} */

static zend_bool zend_is_call(zend_ast *ast);

static int generate_free_switch_expr(zend_switch_entry *switch_entry TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	if (switch_entry->cond.op_type != IS_VAR && switch_entry->cond.op_type != IS_TMP_VAR) {
		return (switch_entry->cond.op_type == IS_UNUSED);
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = (switch_entry->cond.op_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
	SET_NODE(opline->op1, &switch_entry->cond);
	SET_UNUSED(opline->op2);

	return 0;
}
/* }}} */

static int generate_free_foreach_copy(const zend_op *foreach_copy TSRMLS_DC) /* {{{ */
{
	zend_op *opline;

	/* If we reach the separator then stop applying the stack */
	if (foreach_copy->result_type == IS_UNUSED) {
		return 1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = (foreach_copy->result_type == IS_TMP_VAR) ? ZEND_FREE : ZEND_SWITCH_FREE;
	COPY_NODE(opline->op1, foreach_copy->result);
	SET_UNUSED(opline->op2);

	return 0;
}
/* }}} */

static zend_uint zend_add_try_element(zend_uint try_op TSRMLS_DC) /* {{{ */
{
	zend_op_array *op_array = CG(active_op_array);
	zend_uint try_catch_offset = op_array->last_try_catch++;
	zend_try_catch_element *elem;

	op_array->try_catch_array = safe_erealloc(
		op_array->try_catch_array, sizeof(zend_try_catch_element), op_array->last_try_catch, 0);

	elem = &op_array->try_catch_array[try_catch_offset];
	elem->try_op = try_op;
	elem->catch_op = 0;
	elem->finally_op = 0;
	elem->finally_end = 0;

	return try_catch_offset;
}
/* }}} */

ZEND_API void function_add_ref(zend_function *function) /* {{{ */
{
	if (function->type == ZEND_USER_FUNCTION) {
		zend_op_array *op_array = &function->op_array;

		(*op_array->refcount)++;
		if (op_array->static_variables) {
			HashTable *static_variables = op_array->static_variables;

			ALLOC_HASHTABLE(op_array->static_variables);
			zend_array_dup(op_array->static_variables, static_variables);
		}
		op_array->run_time_cache = NULL;
	} else if (function->type == ZEND_INTERNAL_FUNCTION) {
		if (function->common.function_name) {
			STR_ADDREF(function->common.function_name);
		}
	}
}
/* }}} */

static void do_inherit_parent_constructor(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_function *function, *new_function;

	if (!ce->parent) {
		return;
	}

	/* You cannot change create_object */
	ce->create_object = ce->parent->create_object;

	/* Inherit special functions if needed */
	if (!ce->get_iterator) {
		ce->get_iterator = ce->parent->get_iterator;
	}
	if (!ce->iterator_funcs.funcs) {
		ce->iterator_funcs.funcs = ce->parent->iterator_funcs.funcs;
	}
	if (!ce->__get) {
		ce->__get   = ce->parent->__get;
	}
	if (!ce->__set) {
		ce->__set = ce->parent->__set;
	}
	if (!ce->__unset) {
		ce->__unset = ce->parent->__unset;
	}
	if (!ce->__isset) {
		ce->__isset = ce->parent->__isset;
	}
	if (!ce->__call) {
		ce->__call = ce->parent->__call;
	}
	if (!ce->__callstatic) {
		ce->__callstatic = ce->parent->__callstatic;
	}
	if (!ce->__tostring) {
		ce->__tostring = ce->parent->__tostring;
	}
	if (!ce->clone) {
		ce->clone = ce->parent->clone;
	}
	if(!ce->serialize) {
		ce->serialize = ce->parent->serialize;
	}
	if(!ce->unserialize) {
		ce->unserialize = ce->parent->unserialize;
	}
	if (!ce->destructor) {
		ce->destructor   = ce->parent->destructor;
	}
	if (!ce->__debugInfo) {
		ce->__debugInfo = ce->parent->__debugInfo;
	}
	if (ce->constructor) {
		if (ce->parent->constructor && ce->parent->constructor->common.fn_flags & ZEND_ACC_FINAL) {
			zend_error(E_ERROR, "Cannot override final %s::%s() with %s::%s()",
				ce->parent->name->val, ce->parent->constructor->common.function_name->val,
				ce->name->val, ce->constructor->common.function_name->val
				);
		}
		return;
	}

	if ((function = zend_hash_str_find_ptr(&ce->parent->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1)) != NULL) {
		/* inherit parent's constructor */
		if (function->type == ZEND_INTERNAL_FUNCTION) {
			new_function = pemalloc(sizeof(zend_internal_function), 1);
			memcpy(new_function, function, sizeof(zend_internal_function));
		} else {
			new_function = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
			memcpy(new_function, function, sizeof(zend_op_array));
		}
		zend_hash_str_update_ptr(&ce->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1, new_function);
		function_add_ref(new_function);
	} else {
		/* Don't inherit the old style constructor if we already have the new style constructor */
		zend_string *lc_class_name;
		zend_string *lc_parent_class_name;

		lc_class_name = STR_ALLOC(ce->name->len, 0);
		zend_str_tolower_copy(lc_class_name->val, ce->name->val, ce->name->len);
		if (!zend_hash_exists(&ce->function_table, lc_class_name)) {
			lc_parent_class_name = STR_ALLOC(ce->parent->name->len, 0);
			zend_str_tolower_copy(lc_parent_class_name->val, ce->parent->name->val, ce->parent->name->len);
			if (!zend_hash_exists(&ce->function_table, lc_parent_class_name) &&
					(function = zend_hash_find_ptr(&ce->parent->function_table, lc_parent_class_name)) != NULL) {
				if (function->common.fn_flags & ZEND_ACC_CTOR) {
					/* inherit parent's constructor */
					new_function = pemalloc(sizeof(zend_function), function->type == ZEND_INTERNAL_FUNCTION);
					memcpy(new_function, function, sizeof(zend_function));
					zend_hash_update_ptr(&ce->function_table, lc_parent_class_name, new_function);
					function_add_ref(new_function);
				}
			}
			STR_RELEASE(lc_parent_class_name);
		}
		STR_FREE(lc_class_name);
	}
	ce->constructor = ce->parent->constructor;
}
/* }}} */

char *zend_visibility_string(zend_uint fn_flags) /* {{{ */
{
	if (fn_flags & ZEND_ACC_PRIVATE) {
		return "private";
	}
	if (fn_flags & ZEND_ACC_PROTECTED) {
		return "protected";
	}
	if (fn_flags & ZEND_ACC_PUBLIC) {
		return "public";
	}
	return "";
}
/* }}} */

static zend_function *do_inherit_method(zend_function *old_function TSRMLS_DC) /* {{{ */
{
	zend_function *new_function;

	if (old_function->type == ZEND_INTERNAL_FUNCTION) {
		new_function = pemalloc(sizeof(zend_internal_function), 1);
		memcpy(new_function, old_function, sizeof(zend_internal_function));
	} else {
		new_function = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
		memcpy(new_function, old_function, sizeof(zend_op_array));
	}
	/* The class entry of the derived function intentionally remains the same
	 * as that of the parent class.  That allows us to know in which context
	 * we're running, and handle private method calls properly.
	 */
	function_add_ref(new_function);
	return new_function;
}
/* }}} */

static zend_bool zend_do_perform_implementation_check(const zend_function *fe, const zend_function *proto TSRMLS_DC) /* {{{ */
{
	zend_uint i, num_args;

	/* If it's a user function then arg_info == NULL means we don't have any parameters but
	 * we still need to do the arg number checks.  We are only willing to ignore this for internal
	 * functions because extensions don't always define arg_info.
	 */
	if (!proto || (!proto->common.arg_info && proto->common.type != ZEND_USER_FUNCTION)) {
		return 1;
	}

	/* Checks for constructors only if they are declared in an interface,
	 * or explicitly marked as abstract
	 */
	if ((fe->common.fn_flags & ZEND_ACC_CTOR)
		&& ((proto->common.scope->ce_flags & ZEND_ACC_INTERFACE) == 0
			&& (proto->common.fn_flags & ZEND_ACC_ABSTRACT) == 0)) {
		return 1;
	}

	/* If both methods are private do not enforce a signature */
    if ((fe->common.fn_flags & ZEND_ACC_PRIVATE) && (proto->common.fn_flags & ZEND_ACC_PRIVATE)) {
		return 1;
	}

	/* check number of arguments */
	if (proto->common.required_num_args < fe->common.required_num_args
		|| proto->common.num_args > fe->common.num_args) {
		return 0;
	}

	/* by-ref constraints on return values are covariant */
	if ((proto->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)
		&& !(fe->common.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
		return 0;
	}

	if ((proto->common.fn_flags & ZEND_ACC_VARIADIC)
		&& !(fe->common.fn_flags & ZEND_ACC_VARIADIC)) {
		return 0;
	}

	/* For variadic functions any additional (optional) arguments that were added must be
	 * checked against the signature of the variadic argument, so in this case we have to
	 * go through all the parameters of the function and not just those present in the
	 * prototype. */
	num_args = proto->common.num_args;
	if ((fe->common.fn_flags & ZEND_ACC_VARIADIC)
		&& fe->common.num_args > proto->common.num_args) {
		num_args = fe->common.num_args;
	}

	for (i = 0; i < num_args; i++) {
		zend_arg_info *fe_arg_info = &fe->common.arg_info[i];

		zend_arg_info *proto_arg_info;
		if (i < proto->common.num_args) {
			proto_arg_info = &proto->common.arg_info[i];
		} else {
			proto_arg_info = &proto->common.arg_info[proto->common.num_args-1];
		}

		if (ZEND_LOG_XOR(fe_arg_info->class_name, proto_arg_info->class_name)) {
			/* Only one has a type hint and the other one doesn't */
			return 0;
		}

		if (fe_arg_info->class_name) {
			zend_string *fe_class_name, *proto_class_name;

			if (!strcasecmp(fe_arg_info->class_name, "parent") && proto->common.scope) {
				fe_class_name = STR_COPY(proto->common.scope->name);
			} else if (!strcasecmp(fe_arg_info->class_name, "self") && fe->common.scope) {
				fe_class_name = STR_COPY(fe->common.scope->name);
			} else {
				fe_class_name = STR_INIT(
					fe_arg_info->class_name,
					fe_arg_info->class_name_len, 0);
			}

			if (!strcasecmp(proto_arg_info->class_name, "parent") && proto->common.scope && proto->common.scope->parent) {
				proto_class_name = STR_COPY(proto->common.scope->parent->name);
			} else if (!strcasecmp(proto_arg_info->class_name, "self") && proto->common.scope) {
				proto_class_name = STR_COPY(proto->common.scope->name);
			} else {
				proto_class_name = STR_INIT(
					proto_arg_info->class_name,
					proto_arg_info->class_name_len, 0);
			}

			if (strcasecmp(fe_class_name->val, proto_class_name->val)!=0) {
				const char *colon;

				if (fe->common.type != ZEND_USER_FUNCTION) {
					STR_RELEASE(proto_class_name);
					STR_RELEASE(fe_class_name);
					return 0;
			    } else if (strchr(proto_class_name->val, '\\') != NULL ||
						(colon = zend_memrchr(fe_class_name->val, '\\', fe_class_name->len)) == NULL ||
						strcasecmp(colon+1, proto_class_name->val) != 0) {
					zend_class_entry *fe_ce, *proto_ce;

					fe_ce = zend_lookup_class(fe_class_name TSRMLS_CC);
					proto_ce = zend_lookup_class(proto_class_name TSRMLS_CC);

					/* Check for class alias */
					if (!fe_ce || !proto_ce ||
							fe_ce->type == ZEND_INTERNAL_CLASS ||
							proto_ce->type == ZEND_INTERNAL_CLASS ||
							fe_ce != proto_ce) {
						STR_RELEASE(proto_class_name);
						STR_RELEASE(fe_class_name);
						return 0;
					}
				}
			}
			STR_RELEASE(proto_class_name);
			STR_RELEASE(fe_class_name);
		}
		if (fe_arg_info->type_hint != proto_arg_info->type_hint) {
			/* Incompatible type hint */
			return 0;
		}

		/* by-ref constraints on arguments are invariant */
		if (fe_arg_info->pass_by_reference != proto_arg_info->pass_by_reference) {
			return 0;
		}
	}

	return 1;
}
/* }}} */

#define REALLOC_BUF_IF_EXCEED(buf, offset, length, size) \
	if (UNEXPECTED(offset - buf + size >= length)) { 	\
		length += size + 1; 				\
		buf = erealloc(buf, length); 		\
	}

static char *zend_get_function_declaration(zend_function *fptr TSRMLS_DC) /* {{{ */
{
	char *offset, *buf;
	zend_uint length = 1024;

	offset = buf = (char *)emalloc(length * sizeof(char));
	if (fptr->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE) {
		*(offset++) = '&';
		*(offset++) = ' ';
	}

	if (fptr->common.scope) {
		memcpy(offset, fptr->common.scope->name->val, fptr->common.scope->name->len);
		offset += fptr->common.scope->name->len;
		*(offset++) = ':';
		*(offset++) = ':';
	}

	{
		size_t name_len = fptr->common.function_name->len;
		REALLOC_BUF_IF_EXCEED(buf, offset, length, name_len);
		memcpy(offset, fptr->common.function_name->val, name_len);
		offset += name_len;
	}

	*(offset++) = '(';
	if (fptr->common.arg_info) {
		zend_uint i, required;
		zend_arg_info *arg_info = fptr->common.arg_info;

		required = fptr->common.required_num_args;
		for (i = 0; i < fptr->common.num_args;) {
			if (arg_info->class_name) {
				const char *class_name;
				zend_uint class_name_len;
				if (!strcasecmp(arg_info->class_name, "self") && fptr->common.scope ) {
					class_name = fptr->common.scope->name->val;
					class_name_len = fptr->common.scope->name->len;
				} else if (!strcasecmp(arg_info->class_name, "parent") && fptr->common.scope->parent) {
					class_name = fptr->common.scope->parent->name->val;
					class_name_len = fptr->common.scope->parent->name->len;
				} else {
					class_name = arg_info->class_name;
					class_name_len = arg_info->class_name_len;
				}
				REALLOC_BUF_IF_EXCEED(buf, offset, length, class_name_len);
				memcpy(offset, class_name, class_name_len);
				offset += class_name_len;
				*(offset++) = ' ';
			} else if (arg_info->type_hint) {
				zend_uint type_name_len;
				char *type_name = zend_get_type_by_const(arg_info->type_hint);
				type_name_len = strlen(type_name);
				REALLOC_BUF_IF_EXCEED(buf, offset, length, type_name_len);
				memcpy(offset, type_name, type_name_len);
				offset += type_name_len;
				*(offset++) = ' ';
			}

			if (arg_info->pass_by_reference) {
				*(offset++) = '&';
			}

			if (arg_info->is_variadic) {
				*(offset++) = '.';
				*(offset++) = '.';
				*(offset++) = '.';
			}

			*(offset++) = '$';

			if (arg_info->name) {
				REALLOC_BUF_IF_EXCEED(buf, offset, length, arg_info->name_len);
				memcpy(offset, arg_info->name, arg_info->name_len);
				offset += arg_info->name_len;
			} else {
				zend_uint idx = i;
				memcpy(offset, "param", 5);
				offset += 5;
				do {
					*(offset++) = (char) (idx % 10) + '0';
					idx /= 10;
				} while (idx > 0);
			}
			if (i >= required && !arg_info->is_variadic) {
				*(offset++) = ' ';
				*(offset++) = '=';
				*(offset++) = ' ';
				if (fptr->type == ZEND_USER_FUNCTION) {
					zend_op *precv = NULL;
					{
						zend_uint idx  = i;
						zend_op *op = ((zend_op_array *)fptr)->opcodes;
						zend_op *end = op + ((zend_op_array *)fptr)->last;

						++idx;
						while (op < end) {
							if ((op->opcode == ZEND_RECV || op->opcode == ZEND_RECV_INIT)
									&& op->op1.num == (long)idx)
							{
								precv = op;
							}
							++op;
						}
					}
					if (precv && precv->opcode == ZEND_RECV_INIT && precv->op2_type != IS_UNUSED) {
						zval *zv = precv->op2.zv;

						if (Z_TYPE_P(zv) == IS_CONSTANT) {
							REALLOC_BUF_IF_EXCEED(buf, offset, length, Z_STRLEN_P(zv));
							memcpy(offset, Z_STRVAL_P(zv), Z_STRLEN_P(zv));
							offset += Z_STRLEN_P(zv);
						} else if (Z_TYPE_P(zv) == IS_FALSE) {
							memcpy(offset, "false", 5);
							offset += 5;
						} else if (Z_TYPE_P(zv) == IS_TRUE) {
							memcpy(offset, "true", 4);
							offset += 4;
						} else if (Z_TYPE_P(zv) == IS_NULL) {
							memcpy(offset, "NULL", 4);
							offset += 4;
						} else if (Z_TYPE_P(zv) == IS_STRING) {
							*(offset++) = '\'';
							REALLOC_BUF_IF_EXCEED(buf, offset, length, MIN(Z_STRLEN_P(zv), 10));
							memcpy(offset, Z_STRVAL_P(zv), MIN(Z_STRLEN_P(zv), 10));
							offset += MIN(Z_STRLEN_P(zv), 10);
							if (Z_STRLEN_P(zv) > 10) {
								*(offset++) = '.';
								*(offset++) = '.';
								*(offset++) = '.';
							}
							*(offset++) = '\'';
						} else if (Z_TYPE_P(zv) == IS_ARRAY) {
							memcpy(offset, "Array", 5);
							offset += 5;
						} else if (Z_TYPE_P(zv) == IS_CONSTANT_AST) {
							memcpy(offset, "<expression>", 12);
							offset += 12;
						} else {
							zend_string *str = zval_get_string(zv);
							REALLOC_BUF_IF_EXCEED(buf, offset, length, str->len);
							memcpy(offset, str->val, str->len);
							offset += str->len;
							STR_RELEASE(str);
						}
					}
				} else {
					memcpy(offset, "NULL", 4);
					offset += 4;
				}
			}

			if (++i < fptr->common.num_args) {
				*(offset++) = ',';
				*(offset++) = ' ';
			}
			arg_info++;
			REALLOC_BUF_IF_EXCEED(buf, offset, length, 32);
		}
	}
	*(offset++) = ')';
	*offset = '\0';

	return buf;
}
/* }}} */

static void do_inheritance_check_on_method(zend_function *child, zend_function *parent TSRMLS_DC) /* {{{ */
{
	zend_uint child_flags;
	zend_uint parent_flags = parent->common.fn_flags;

	if ((parent->common.scope->ce_flags & ZEND_ACC_INTERFACE) == 0
		&& parent->common.fn_flags & ZEND_ACC_ABSTRACT
		&& parent->common.scope != (child->common.prototype ? child->common.prototype->common.scope : child->common.scope)
		&& child->common.fn_flags & (ZEND_ACC_ABSTRACT|ZEND_ACC_IMPLEMENTED_ABSTRACT)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Can't inherit abstract function %s::%s() (previously declared abstract in %s)",
			parent->common.scope->name->val,
			child->common.function_name->val,
			child->common.prototype ? child->common.prototype->common.scope->name->val : child->common.scope->name->val);
	}

	if (parent_flags & ZEND_ACC_FINAL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot override final method %s::%s()", ZEND_FN_SCOPE_NAME(parent), child->common.function_name->val);
	}

	child_flags	= child->common.fn_flags;
	/* You cannot change from static to non static and vice versa.
	 */
	if ((child_flags & ZEND_ACC_STATIC) != (parent_flags & ZEND_ACC_STATIC)) {
		if (child->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot make non static method %s::%s() static in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name->val, ZEND_FN_SCOPE_NAME(child));
		} else {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot make static method %s::%s() non static in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name->val, ZEND_FN_SCOPE_NAME(child));
		}
	}

	/* Disallow making an inherited method abstract. */
	if ((child_flags & ZEND_ACC_ABSTRACT) && !(parent_flags & ZEND_ACC_ABSTRACT)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot make non abstract method %s::%s() abstract in class %s", ZEND_FN_SCOPE_NAME(parent), child->common.function_name->val, ZEND_FN_SCOPE_NAME(child));
	}

	if (parent_flags & ZEND_ACC_CHANGED) {
		child->common.fn_flags |= ZEND_ACC_CHANGED;
	} else {
		/* Prevent derived classes from restricting access that was available in parent classes
		 */
		if ((child_flags & ZEND_ACC_PPP_MASK) > (parent_flags & ZEND_ACC_PPP_MASK)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Access level to %s::%s() must be %s (as in class %s)%s", ZEND_FN_SCOPE_NAME(child), child->common.function_name->val, zend_visibility_string(parent_flags), ZEND_FN_SCOPE_NAME(parent), (parent_flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if (((child_flags & ZEND_ACC_PPP_MASK) < (parent_flags & ZEND_ACC_PPP_MASK))
			&& ((parent_flags & ZEND_ACC_PPP_MASK) & ZEND_ACC_PRIVATE)) {
			child->common.fn_flags |= ZEND_ACC_CHANGED;
		}
	}

	if (parent_flags & ZEND_ACC_PRIVATE) {
		child->common.prototype = NULL;
	} else if (parent_flags & ZEND_ACC_ABSTRACT) {
		child->common.fn_flags |= ZEND_ACC_IMPLEMENTED_ABSTRACT;
		child->common.prototype = parent;
	} else if (!(parent->common.fn_flags & ZEND_ACC_CTOR) || (parent->common.prototype && (parent->common.prototype->common.scope->ce_flags & ZEND_ACC_INTERFACE))) {
		/* ctors only have a prototype if it comes from an interface */
		child->common.prototype = parent->common.prototype ? parent->common.prototype : parent;
	}

	if (child->common.prototype && (child->common.prototype->common.fn_flags & ZEND_ACC_ABSTRACT)) {
		if (!zend_do_perform_implementation_check(child, child->common.prototype TSRMLS_CC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Declaration of %s::%s() must be compatible with %s", ZEND_FN_SCOPE_NAME(child), child->common.function_name->val, zend_get_function_declaration(child->common.prototype TSRMLS_CC));
		}
	} else if (EG(error_reporting) & E_STRICT || Z_TYPE(EG(user_error_handler)) != IS_UNDEF) { /* Check E_STRICT (or custom error handler) before the check so that we save some time */
		if (!zend_do_perform_implementation_check(child, parent TSRMLS_CC)) {
			char *method_prototype = zend_get_function_declaration(parent TSRMLS_CC);
			zend_error(E_STRICT, "Declaration of %s::%s() should be compatible with %s", ZEND_FN_SCOPE_NAME(child), child->common.function_name->val, method_prototype);
			efree(method_prototype);
		}
	}
}
/* }}} */

static zend_bool do_inherit_method_check(HashTable *child_function_table, zend_function *parent, zend_string *key, zend_class_entry *child_ce) /* {{{ */
{
	zend_uint parent_flags = parent->common.fn_flags;
	zend_function *child;
	TSRMLS_FETCH();

	if ((child = zend_hash_find_ptr(child_function_table, key)) == NULL) {
		if (parent_flags & (ZEND_ACC_ABSTRACT)) {
			child_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		return 1; /* method doesn't exist in child, copy from parent */
	}

	do_inheritance_check_on_method(child, parent TSRMLS_CC);

	return 0;
}
/* }}} */

static zend_bool do_inherit_property_access_check(HashTable *target_ht, zend_property_info *parent_info, zend_string *key, zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_property_info *child_info;
	zend_class_entry *parent_ce = ce->parent;

	if (parent_info->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_SHADOW)) {
		if ((child_info = zend_hash_find_ptr(&ce->properties_info, key)) != NULL) {
			child_info->flags |= ZEND_ACC_CHANGED;
		} else {
			if(ce->type & ZEND_INTERNAL_CLASS) {
				child_info = zend_duplicate_property_info_internal(parent_info);
			} else {
				child_info = zend_duplicate_property_info(parent_info TSRMLS_CC);
			}
			zend_hash_update_ptr(&ce->properties_info, key, child_info);
			child_info->flags &= ~ZEND_ACC_PRIVATE; /* it's not private anymore */
			child_info->flags |= ZEND_ACC_SHADOW; /* but it's a shadow of private */
		}
		return 0; /* don't copy access information to child */
	}

	if ((child_info = zend_hash_find_ptr(&ce->properties_info, key)) != NULL) {
		if ((parent_info->flags & ZEND_ACC_STATIC) != (child_info->flags & ZEND_ACC_STATIC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s%s::$%s as %s%s::$%s",
				(parent_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", parent_ce->name->val, key->val,
				(child_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", ce->name->val, key->val);

		}

		if(parent_info->flags & ZEND_ACC_CHANGED) {
			child_info->flags |= ZEND_ACC_CHANGED;
		}

		if ((child_info->flags & ZEND_ACC_PPP_MASK) > (parent_info->flags & ZEND_ACC_PPP_MASK)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Access level to %s::$%s must be %s (as in class %s)%s", ce->name->val, key->val, zend_visibility_string(parent_info->flags), parent_ce->name->val, (parent_info->flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if ((child_info->flags & ZEND_ACC_STATIC) == 0) {
			zval_ptr_dtor(&(ce->default_properties_table[parent_info->offset]));
			ce->default_properties_table[parent_info->offset] = ce->default_properties_table[child_info->offset];
			ZVAL_UNDEF(&ce->default_properties_table[child_info->offset]);
			child_info->offset = parent_info->offset;
		}
		return 0;	/* Don't copy from parent */
	} else {
		return 1;	/* Copy from parent */
	}
}
/* }}} */

static inline void do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	if (!(ce->ce_flags & ZEND_ACC_INTERFACE) && iface->interface_gets_implemented && iface->interface_gets_implemented(iface, ce TSRMLS_CC) == FAILURE) {
		zend_error(E_CORE_ERROR, "Class %s could not implement interface %s", ce->name->val, iface->name->val);
	}
	if (ce == iface) {
		zend_error(E_ERROR, "Interface %s cannot implement itself", ce->name->val);
	}
}
/* }}} */

ZEND_API void zend_do_inherit_interfaces(zend_class_entry *ce, const zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	/* expects interface to be contained in ce's interface list already */
	zend_uint i, ce_num, if_num = iface->num_interfaces;
	zend_class_entry *entry;

	if (if_num==0) {
		return;
	}
	ce_num = ce->num_interfaces;

	if (ce->type == ZEND_INTERNAL_CLASS) {
		ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	} else {
		ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (ce_num + if_num));
	}

	/* Inherit the interfaces, only if they're not already inherited by the class */
	while (if_num--) {
		entry = iface->interfaces[if_num];
		for (i = 0; i < ce_num; i++) {
			if (ce->interfaces[i] == entry) {
				break;
			}
		}
		if (i == ce_num) {
			ce->interfaces[ce->num_interfaces++] = entry;
		}
	}

	/* and now call the implementing handlers */
	while (ce_num < ce->num_interfaces) {
		do_implement_interface(ce, ce->interfaces[ce_num++] TSRMLS_CC);
	}
}
/* }}} */

#ifdef ZTS
# define zval_property_ctor(parent_ce, ce) \
	(((parent_ce)->type != (ce)->type) ? ZVAL_COPY_CTOR : zval_add_ref)
#else
# define zval_property_ctor(parent_ce, ce) \
	zval_add_ref
#endif

static void do_inherit_class_constant(zend_string *name, zval *zv, zend_class_entry *ce, zend_class_entry *parent_ce TSRMLS_DC) /* {{{ */
{
	if (!Z_ISREF_P(zv)) {
		if (parent_ce->type == ZEND_INTERNAL_CLASS) {
			ZVAL_NEW_PERSISTENT_REF(zv, zv);
		} else {
			ZVAL_NEW_REF(zv, zv);
		}
	}
	if (Z_CONSTANT_P(Z_REFVAL_P(zv))) {
		ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
	}
	if (zend_hash_add(&ce->constants_table, name, zv)) {
		Z_ADDREF_P(zv);
	}
}
/* }}} */

ZEND_API void zend_do_inheritance(zend_class_entry *ce, zend_class_entry *parent_ce TSRMLS_DC) /* {{{ */
{
	zend_property_info *property_info;
	zend_function *func;
	zend_string *key;
	zval *zv;

	if ((ce->ce_flags & ZEND_ACC_INTERFACE)
		&& !(parent_ce->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Interface %s may not inherit from class (%s)", ce->name->val, parent_ce->name->val);
	}
	if (parent_ce->ce_flags & ZEND_ACC_FINAL_CLASS) {
		zend_error_noreturn(E_COMPILE_ERROR, "Class %s may not inherit from final class (%s)", ce->name->val, parent_ce->name->val);
	}

	ce->parent = parent_ce;
	/* Copy serialize/unserialize callbacks */
	if (!ce->serialize) {
		ce->serialize   = parent_ce->serialize;
	}
	if (!ce->unserialize) {
		ce->unserialize = parent_ce->unserialize;
	}

	/* Inherit interfaces */
	zend_do_inherit_interfaces(ce, parent_ce TSRMLS_CC);

	/* Inherit properties */
	if (parent_ce->default_properties_count) {
		int i = ce->default_properties_count + parent_ce->default_properties_count;

		ce->default_properties_table = perealloc(ce->default_properties_table, sizeof(zval) * i, ce->type == ZEND_INTERNAL_CLASS);
		if (ce->default_properties_count) {
			while (i-- > parent_ce->default_properties_count) {
				ce->default_properties_table[i] = ce->default_properties_table[i - parent_ce->default_properties_count];
			}
		}
		for (i = 0; i < parent_ce->default_properties_count; i++) {
#ifdef ZTS
			if (parent_ce->type != ce->type) {
				ZVAL_DUP(&ce->default_properties_table[i], &parent_ce->default_properties_table[i]);
				if (Z_OPT_CONSTANT(ce->default_properties_table[i])) {
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
				}
				continue;
			}
#endif

			ZVAL_COPY(&ce->default_properties_table[i], &parent_ce->default_properties_table[i]);
			if (Z_OPT_CONSTANT(ce->default_properties_table[i])) {
				ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
			}
		}
		ce->default_properties_count += parent_ce->default_properties_count;
	}

	if (parent_ce->type != ce->type) {
		/* User class extends internal class */
		zend_update_class_constants(parent_ce  TSRMLS_CC);
		if (parent_ce->default_static_members_count) {
			int i = ce->default_static_members_count + parent_ce->default_static_members_count;

			ce->default_static_members_table = erealloc(ce->default_static_members_table, sizeof(zval) * i);
			if (ce->default_static_members_count) {
				while (i-- > parent_ce->default_static_members_count) {
					ce->default_static_members_table[i] = ce->default_static_members_table[i - parent_ce->default_static_members_count];
				}
			}
			for (i = 0; i < parent_ce->default_static_members_count; i++) {
				ZVAL_MAKE_REF(&CE_STATIC_MEMBERS(parent_ce)[i]);
				ce->default_static_members_table[i] = CE_STATIC_MEMBERS(parent_ce)[i];
				Z_ADDREF(ce->default_static_members_table[i]);
				if (Z_CONSTANT_P(Z_REFVAL(ce->default_static_members_table[i]))) {
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
				}
			}
			ce->default_static_members_count += parent_ce->default_static_members_count;
			ce->static_members_table = ce->default_static_members_table;
		}
	} else {
		if (parent_ce->default_static_members_count) {
			int i = ce->default_static_members_count + parent_ce->default_static_members_count;

			ce->default_static_members_table = perealloc(ce->default_static_members_table, sizeof(zval) * i, ce->type == ZEND_INTERNAL_CLASS);
			if (ce->default_static_members_count) {
				while (i-- > parent_ce->default_static_members_count) {
					ce->default_static_members_table[i] = ce->default_static_members_table[i - parent_ce->default_static_members_count];
				}
			}
			for (i = 0; i < parent_ce->default_static_members_count; i++) {
				ZVAL_MAKE_REF(&parent_ce->default_static_members_table[i]);
				ce->default_static_members_table[i] = parent_ce->default_static_members_table[i];
				Z_ADDREF(ce->default_static_members_table[i]);
				if (Z_CONSTANT_P(Z_REFVAL(ce->default_static_members_table[i]))) {
					ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
				}
			}
			ce->default_static_members_count += parent_ce->default_static_members_count;
			if (ce->type == ZEND_USER_CLASS) {
				ce->static_members_table = ce->default_static_members_table;
			}
		}
	}

	ZEND_HASH_FOREACH_PTR(&ce->properties_info, property_info) {
		if (property_info->ce == ce) {
			if (property_info->flags & ZEND_ACC_STATIC) {
				property_info->offset += parent_ce->default_static_members_count;
			} else {
				property_info->offset += parent_ce->default_properties_count;
			}
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_FOREACH_STR_KEY_PTR(&parent_ce->properties_info, key, property_info) {
		if (do_inherit_property_access_check(&ce->properties_info, property_info, key, ce TSRMLS_CC)) {
			if (ce->type & ZEND_INTERNAL_CLASS) {
				property_info = zend_duplicate_property_info_internal(property_info);
			} else {
				property_info = zend_duplicate_property_info(property_info TSRMLS_CC);
			}
			zend_hash_add_new_ptr(&ce->properties_info, key, property_info);
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_FOREACH_STR_KEY_VAL(&parent_ce->constants_table, key, zv) {
		do_inherit_class_constant(key, zv, ce, parent_ce TSRMLS_CC);
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_FOREACH_STR_KEY_PTR(&parent_ce->function_table, key, func) {
		if (do_inherit_method_check(&ce->function_table, func, key, ce)) {
			zend_function *new_func = do_inherit_method(func TSRMLS_CC);
			zend_hash_add_new_ptr(&ce->function_table, key, new_func);
		}
	} ZEND_HASH_FOREACH_END();

	do_inherit_parent_constructor(ce TSRMLS_CC);

	if (ce->ce_flags & ZEND_ACC_IMPLICIT_ABSTRACT_CLASS && ce->type == ZEND_INTERNAL_CLASS) {
		ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	} else if (!(ce->ce_flags & (ZEND_ACC_IMPLEMENT_INTERFACES|ZEND_ACC_IMPLEMENT_TRAITS))) {
		/* The verification will be done in runtime by ZEND_VERIFY_ABSTRACT_CLASS */
		zend_verify_abstract_class(ce TSRMLS_CC);
	}
	ce->ce_flags |= parent_ce->ce_flags & ZEND_HAS_STATIC_IN_METHODS;
}
/* }}} */

static zend_bool do_inherit_constant_check(HashTable *child_constants_table, zval *parent_constant, zend_string *name, const zend_class_entry *iface) /* {{{ */
{
	zval *old_constant;

	if ((old_constant = zend_hash_find(child_constants_table, name)) != NULL) {
		if (!Z_ISREF_P(old_constant) ||
		    !Z_ISREF_P(parent_constant) ||
		    Z_REFVAL_P(old_constant) != Z_REFVAL_P(parent_constant)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot inherit previously-inherited or override constant %s from interface %s", name->val, iface->name->val);
		}
		return 0;
	}
	return 1;
}
/* }}} */

static void do_inherit_iface_constant(zend_string *name, zval *zv, zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	if (do_inherit_constant_check(&ce->constants_table, zv, name, iface)) {
		ZVAL_MAKE_REF(zv);
		Z_ADDREF_P(zv);
		if (Z_CONSTANT_P(Z_REFVAL_P(zv))) {
			ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
		}
		zend_hash_update(&ce->constants_table, name, zv);
	}
}
/* }}} */

ZEND_API void zend_do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC) /* {{{ */
{
	zend_uint i, ignore = 0;
	zend_uint current_iface_num = ce->num_interfaces;
	zend_uint parent_iface_num  = ce->parent ? ce->parent->num_interfaces : 0;
	zend_function *func;
	zend_string *key;
	zval *zv;

	for (i = 0; i < ce->num_interfaces; i++) {
		if (ce->interfaces[i] == NULL) {
			memmove(ce->interfaces + i, ce->interfaces + i + 1, sizeof(zend_class_entry*) * (--ce->num_interfaces - i));
			i--;
		} else if (ce->interfaces[i] == iface) {
			if (i < parent_iface_num) {
				ignore = 1;
			} else {
				zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot implement previously implemented interface %s", ce->name->val, iface->name->val);
			}
		}
	}
	if (ignore) {
		/* Check for attempt to redeclare interface constants */
		ZEND_HASH_FOREACH_STR_KEY_VAL(&ce->constants_table, key, zv) {
			do_inherit_constant_check(&iface->constants_table, zv, key, iface);
		} ZEND_HASH_FOREACH_END();
	} else {
		if (ce->num_interfaces >= current_iface_num) {
			if (ce->type == ZEND_INTERNAL_CLASS) {
				ce->interfaces = (zend_class_entry **) realloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			} else {
				ce->interfaces = (zend_class_entry **) erealloc(ce->interfaces, sizeof(zend_class_entry *) * (++current_iface_num));
			}
		}
		ce->interfaces[ce->num_interfaces++] = iface;

		ZEND_HASH_FOREACH_STR_KEY_VAL(&iface->constants_table, key, zv) {
			do_inherit_iface_constant(key, zv, ce, iface TSRMLS_CC);
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_STR_KEY_PTR(&iface->function_table, key, func) {
			if (do_inherit_method_check(&ce->function_table, func, key, ce)) {
				zend_function *new_func = do_inherit_method(func TSRMLS_CC);
				zend_hash_add_new_ptr(&ce->function_table, key, new_func);
			}
		} ZEND_HASH_FOREACH_END();

		do_implement_interface(ce, iface TSRMLS_CC);
		zend_do_inherit_interfaces(ce, iface TSRMLS_CC);
	}
}
/* }}} */

ZEND_API void zend_do_implement_trait(zend_class_entry *ce, zend_class_entry *trait TSRMLS_DC) /* {{{ */
{
	zend_uint i, ignore = 0;
	zend_uint current_trait_num = ce->num_traits;
	zend_uint parent_trait_num  = ce->parent ? ce->parent->num_traits : 0;

	for (i = 0; i < ce->num_traits; i++) {
		if (ce->traits[i] == NULL) {
			memmove(ce->traits + i, ce->traits + i + 1, sizeof(zend_class_entry*) * (--ce->num_traits - i));
			i--;
		} else if (ce->traits[i] == trait) {
			if (i < parent_trait_num) {
				ignore = 1;
			}
		}
	}
	if (!ignore) {
		if (ce->num_traits >= current_trait_num) {
			if (ce->type == ZEND_INTERNAL_CLASS) {
				ce->traits = (zend_class_entry **) realloc(ce->traits, sizeof(zend_class_entry *) * (++current_trait_num));
			} else {
				ce->traits = (zend_class_entry **) erealloc(ce->traits, sizeof(zend_class_entry *) * (++current_trait_num));
			}
		}
		ce->traits[ce->num_traits++] = trait;
	}
}
/* }}} */

static zend_bool zend_traits_method_compatibility_check(zend_function *fn, zend_function *other_fn TSRMLS_DC) /* {{{ */
{
	zend_uint    fn_flags = fn->common.scope->ce_flags;
	zend_uint other_flags = other_fn->common.scope->ce_flags;

	return zend_do_perform_implementation_check(fn, other_fn TSRMLS_CC)
		&& ((other_fn->common.scope->ce_flags & ZEND_ACC_INTERFACE) || zend_do_perform_implementation_check(other_fn, fn TSRMLS_CC))
		&& ((fn_flags & (ZEND_ACC_FINAL|ZEND_ACC_STATIC)) ==
		    (other_flags & (ZEND_ACC_FINAL|ZEND_ACC_STATIC))); /* equal final and static qualifier */
}
/* }}} */

static void zend_add_magic_methods(zend_class_entry* ce, zend_string* mname, zend_function* fe TSRMLS_DC) /* {{{ */
{
	if (!strncmp(mname->val, ZEND_CLONE_FUNC_NAME, mname->len)) {
		ce->clone = fe; fe->common.fn_flags |= ZEND_ACC_CLONE;
	} else if (!strncmp(mname->val, ZEND_CONSTRUCTOR_FUNC_NAME, mname->len)) {
		if (ce->constructor) {
			zend_error_noreturn(E_COMPILE_ERROR, "%s has colliding constructor definitions coming from traits", ce->name->val);
		}
		ce->constructor = fe; fe->common.fn_flags |= ZEND_ACC_CTOR;
	} else if (!strncmp(mname->val, ZEND_DESTRUCTOR_FUNC_NAME,  mname->len)) {
		ce->destructor = fe; fe->common.fn_flags |= ZEND_ACC_DTOR;
	} else if (!strncmp(mname->val, ZEND_GET_FUNC_NAME, mname->len)) {
		ce->__get = fe;
	} else if (!strncmp(mname->val, ZEND_SET_FUNC_NAME, mname->len)) {
		ce->__set = fe;
	} else if (!strncmp(mname->val, ZEND_CALL_FUNC_NAME, mname->len)) {
		ce->__call = fe;
	} else if (!strncmp(mname->val, ZEND_UNSET_FUNC_NAME, mname->len)) {
		ce->__unset = fe;
	} else if (!strncmp(mname->val, ZEND_ISSET_FUNC_NAME, mname->len)) {
		ce->__isset = fe;
	} else if (!strncmp(mname->val, ZEND_CALLSTATIC_FUNC_NAME, mname->len)) {
		ce->__callstatic = fe;
	} else if (!strncmp(mname->val, ZEND_TOSTRING_FUNC_NAME, mname->len)) {
		ce->__tostring = fe;
	} else if (!strncmp(mname->val, ZEND_DEBUGINFO_FUNC_NAME, mname->len)) {
		ce->__debugInfo = fe;
	} else if (ce->name->len == mname->len) {
		zend_string *lowercase_name = STR_ALLOC(ce->name->len, 0);
		zend_str_tolower_copy(lowercase_name->val, ce->name->val, ce->name->len);
		lowercase_name = zend_new_interned_string(lowercase_name TSRMLS_CC);
		if (!memcmp(mname->val, lowercase_name->val, mname->len)) {
			if (ce->constructor) {
				zend_error_noreturn(E_COMPILE_ERROR, "%s has colliding constructor definitions coming from traits", ce->name->val);
			}
			ce->constructor = fe;
			fe->common.fn_flags |= ZEND_ACC_CTOR;
		}
		STR_RELEASE(lowercase_name);
	}
}
/* }}} */

static void zend_add_trait_method(zend_class_entry *ce, const char *name, zend_string *key, zend_function *fn, HashTable **overriden TSRMLS_DC) /* {{{ */
{
	zend_function *existing_fn = NULL;
	zend_function *new_fn;

	if ((existing_fn = zend_hash_find_ptr(&ce->function_table, key)) != NULL) {
		if (existing_fn->common.scope == ce) {
			/* members from the current class override trait methods */
			/* use temporary *overriden HashTable to detect hidden conflict */
			if (*overriden) {
				if ((existing_fn = zend_hash_find_ptr(*overriden, key)) != NULL) {
					if (existing_fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
						/* Make sure the trait method is compatible with previosly declared abstract method */
						if (!zend_traits_method_compatibility_check(fn, existing_fn TSRMLS_CC)) {
							zend_error_noreturn(E_COMPILE_ERROR, "Declaration of %s must be compatible with %s",
								zend_get_function_declaration(fn TSRMLS_CC),
								zend_get_function_declaration(existing_fn TSRMLS_CC));
						}
					} else if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
						/* Make sure the abstract declaration is compatible with previous declaration */
						if (!zend_traits_method_compatibility_check(existing_fn, fn TSRMLS_CC)) {
							zend_error_noreturn(E_COMPILE_ERROR, "Declaration of %s must be compatible with %s",
								zend_get_function_declaration(fn TSRMLS_CC),
								zend_get_function_declaration(existing_fn TSRMLS_CC));
						}
						return;
					}
				}
			} else {
				ALLOC_HASHTABLE(*overriden);
				zend_hash_init_ex(*overriden, 8, NULL, ptr_dtor, 0, 0);
			}
			fn = zend_hash_update_mem(*overriden, key, fn, sizeof(zend_function));
			return;
		} else if (existing_fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
			/* Make sure the trait method is compatible with previosly declared abstract method */
			if (!zend_traits_method_compatibility_check(fn, existing_fn TSRMLS_CC)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Declaration of %s must be compatible with %s",
					zend_get_function_declaration(fn TSRMLS_CC),
					zend_get_function_declaration(existing_fn TSRMLS_CC));
			}
		} else if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
			/* Make sure the abstract declaration is compatible with previous declaration */
			if (!zend_traits_method_compatibility_check(existing_fn, fn TSRMLS_CC)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Declaration of %s must be compatible with %s",
					zend_get_function_declaration(fn TSRMLS_CC),
					zend_get_function_declaration(existing_fn TSRMLS_CC));
			}
			return;
		} else if ((existing_fn->common.scope->ce_flags & ZEND_ACC_TRAIT) == ZEND_ACC_TRAIT) {
			/* two traits can't define the same non-abstract method */
#if 1
			zend_error_noreturn(E_COMPILE_ERROR, "Trait method %s has not been applied, because there are collisions with other trait methods on %s",
				name, ce->name->val);
#else		/* TODO: better error message */
			zend_error_noreturn(E_COMPILE_ERROR, "Trait method %s::%s has not been applied as %s::%s, because of collision with %s::%s",
				fn->common.scope->name->val, fn->common.function_name->val,
				ce->name->val, name,
				existing_fn->common.scope->name->val, existing_fn->common.function_name->val);
#endif
		} else {
			/* inherited members are overridden by members inserted by traits */
			/* check whether the trait method fulfills the inheritance requirements */
			do_inheritance_check_on_method(fn, existing_fn TSRMLS_CC);
		}
	}

	function_add_ref(fn);
	new_fn = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	memcpy(new_fn, fn, sizeof(zend_op_array));
	fn = zend_hash_update_ptr(&ce->function_table, key, new_fn);
	zend_add_magic_methods(ce, key, fn TSRMLS_CC);
}
/* }}} */

static void zend_fixup_trait_method(zend_function *fn, zend_class_entry *ce) /* {{{ */
{
	if ((fn->common.scope->ce_flags & ZEND_ACC_TRAIT) == ZEND_ACC_TRAIT) {

		fn->common.scope = ce;

		if (fn->common.fn_flags & ZEND_ACC_ABSTRACT) {
			ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		if (fn->op_array.static_variables) {
			ce->ce_flags |= ZEND_HAS_STATIC_IN_METHODS;
		}
	}
}
/* }}} */

static int zend_traits_copy_functions(zend_string *fnname, zend_function *fn, zend_class_entry *ce, HashTable **overriden, HashTable *exclude_table TSRMLS_DC) /* {{{ */
{
	zend_trait_alias  *alias, **alias_ptr;
	zend_string       *lcname;
	zend_function      fn_copy;

	/* apply aliases which are qualified with a class name, there should not be any ambiguity */
	if (ce->trait_aliases) {
		alias_ptr = ce->trait_aliases;
		alias = *alias_ptr;
		while (alias) {
			/* Scope unset or equal to the function we compare to, and the alias applies to fn */
			if (alias->alias != NULL
				&& (!alias->trait_method->ce || fn->common.scope == alias->trait_method->ce)
				&& alias->trait_method->method_name->len == fnname->len
				&& (zend_binary_strcasecmp(alias->trait_method->method_name->val, alias->trait_method->method_name->len, fnname->val, fnname->len) == 0)) {
				fn_copy = *fn;

				/* if it is 0, no modifieres has been changed */
				if (alias->modifiers) {
					fn_copy.common.fn_flags = alias->modifiers | (fn->common.fn_flags ^ (fn->common.fn_flags & ZEND_ACC_PPP_MASK));
				}

				lcname = STR_ALLOC(alias->alias->len, 0);
				zend_str_tolower_copy(lcname->val, alias->alias->val, alias->alias->len);
				zend_add_trait_method(ce, alias->alias->val, lcname, &fn_copy, overriden TSRMLS_CC);
				STR_RELEASE(lcname);

				/* Record the trait from which this alias was resolved. */
				if (!alias->trait_method->ce) {
					alias->trait_method->ce = fn->common.scope;
				}
			}
			alias_ptr++;
			alias = *alias_ptr;
		}
	}

	if (exclude_table == NULL || zend_hash_find(exclude_table, fnname) == NULL) {
		/* is not in hashtable, thus, function is not to be excluded */
		fn_copy = *fn;

		/* apply aliases which have not alias name, just setting visibility */
		if (ce->trait_aliases) {
			alias_ptr = ce->trait_aliases;
			alias = *alias_ptr;
			while (alias) {
				/* Scope unset or equal to the function we compare to, and the alias applies to fn */
				if (alias->alias == NULL && alias->modifiers != 0
					&& (!alias->trait_method->ce || fn->common.scope == alias->trait_method->ce)
					&& (alias->trait_method->method_name->len == fnname->len)
					&& (zend_binary_strcasecmp(alias->trait_method->method_name->val, alias->trait_method->method_name->len, fnname->val, fnname->len) == 0)) {

					fn_copy.common.fn_flags = alias->modifiers | (fn->common.fn_flags ^ (fn->common.fn_flags & ZEND_ACC_PPP_MASK));

					/** Record the trait from which this alias was resolved. */
					if (!alias->trait_method->ce) {
						alias->trait_method->ce = fn->common.scope;
					}
				}
				alias_ptr++;
				alias = *alias_ptr;
			}
		}

		zend_add_trait_method(ce, fn->common.function_name->val, fnname, &fn_copy, overriden TSRMLS_CC);
	}

	return ZEND_HASH_APPLY_KEEP;
}
/* }}} */

static void zend_check_trait_usage(zend_class_entry *ce, zend_class_entry *trait TSRMLS_DC) /* {{{ */
{
	zend_uint i;

	if ((trait->ce_flags & ZEND_ACC_TRAIT) != ZEND_ACC_TRAIT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Class %s is not a trait, Only traits may be used in 'as' and 'insteadof' statements", trait->name->val);
	}

	for (i = 0; i < ce->num_traits; i++) {
		if (ce->traits[i] == trait) {
			return;
		}
	}
	zend_error_noreturn(E_COMPILE_ERROR, "Required Trait %s wasn't added to %s", trait->name->val, ce->name->val);
}
/* }}} */

static void zend_traits_init_trait_structures(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	size_t i, j = 0;
	zend_trait_precedence *cur_precedence;
	zend_trait_method_reference *cur_method_ref;
	zend_string *lcname;
	zend_bool method_exists;

	/* resolve class references */
	if (ce->trait_precedences) {
		i = 0;
		while ((cur_precedence = ce->trait_precedences[i])) {
			/** Resolve classes for all precedence operations. */
			if (cur_precedence->exclude_from_classes) {
				cur_method_ref = cur_precedence->trait_method;
				if (!(cur_precedence->trait_method->ce = zend_fetch_class(cur_method_ref->class_name,
								ZEND_FETCH_CLASS_TRAIT|ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC))) {
					zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", cur_method_ref->class_name->val);
				}
				zend_check_trait_usage(ce, cur_precedence->trait_method->ce TSRMLS_CC);

				/** Ensure that the prefered method is actually available. */
				lcname = STR_ALLOC(cur_method_ref->method_name->len, 0);
				zend_str_tolower_copy(lcname->val, 
					cur_method_ref->method_name->val,
					cur_method_ref->method_name->len);
				method_exists = zend_hash_exists(&cur_method_ref->ce->function_table,
												 lcname);
				STR_FREE(lcname);
				if (!method_exists) {
					zend_error_noreturn(E_COMPILE_ERROR,
							   "A precedence rule was defined for %s::%s but this method does not exist",
							   cur_method_ref->ce->name->val,
							   cur_method_ref->method_name->val);
				}

				/** With the other traits, we are more permissive.
					We do not give errors for those. This allows to be more
					defensive in such definitions.
					However, we want to make sure that the insteadof declaration
					is consistent in itself.
				 */
				j = 0;
				while (cur_precedence->exclude_from_classes[j].class_name) {
					zend_string* class_name = cur_precedence->exclude_from_classes[j].class_name;

					if (!(cur_precedence->exclude_from_classes[j].ce = zend_fetch_class(class_name, ZEND_FETCH_CLASS_TRAIT |ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC))) {
						zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", class_name->val);
					}
					zend_check_trait_usage(ce, cur_precedence->exclude_from_classes[j].ce TSRMLS_CC);

					/* make sure that the trait method is not from a class mentioned in
					 exclude_from_classes, for consistency */
					if (cur_precedence->trait_method->ce == cur_precedence->exclude_from_classes[i].ce) {
						zend_error_noreturn(E_COMPILE_ERROR,
								   "Inconsistent insteadof definition. "
								   "The method %s is to be used from %s, but %s is also on the exclude list",
								   cur_method_ref->method_name->val,
								   cur_precedence->trait_method->ce->name->val,
								   cur_precedence->trait_method->ce->name->val);
					}

					STR_RELEASE(class_name);
					j++;
				}
			}
			i++;
		}
	}

	if (ce->trait_aliases) {
		i = 0;
		while (ce->trait_aliases[i]) {
			/** For all aliases with an explicit class name, resolve the class now. */
			if (ce->trait_aliases[i]->trait_method->class_name) {
				cur_method_ref = ce->trait_aliases[i]->trait_method;
				if (!(cur_method_ref->ce = zend_fetch_class(cur_method_ref->class_name, ZEND_FETCH_CLASS_TRAIT|ZEND_FETCH_CLASS_NO_AUTOLOAD TSRMLS_CC))) {
					zend_error_noreturn(E_COMPILE_ERROR, "Could not find trait %s", cur_method_ref->class_name->val);
				}
				zend_check_trait_usage(ce, cur_method_ref->ce TSRMLS_CC);

				/** And, ensure that the referenced method is resolvable, too. */
				lcname = STR_ALLOC(cur_method_ref->method_name->len, 0);
				zend_str_tolower_copy(lcname->val,
					cur_method_ref->method_name->val,
					cur_method_ref->method_name->len);
				method_exists = zend_hash_exists(&cur_method_ref->ce->function_table,
						lcname);
				STR_FREE(lcname);

				if (!method_exists) {
					zend_error_noreturn(E_COMPILE_ERROR, "An alias was defined for %s::%s but this method does not exist", cur_method_ref->ce->name->val, cur_method_ref->method_name->val);
				}
			}
			i++;
		}
	}
}
/* }}} */

static void zend_traits_compile_exclude_table(HashTable* exclude_table, zend_trait_precedence **precedences, zend_class_entry *trait) /* {{{ */
{
	size_t i = 0, j;

	if (!precedences) {
		return;
	}
	while (precedences[i]) {
		if (precedences[i]->exclude_from_classes) {
			j = 0;
			while (precedences[i]->exclude_from_classes[j].ce) {
				if (precedences[i]->exclude_from_classes[j].ce == trait) {
					zend_string *lcname = STR_ALLOC(precedences[i]->trait_method->method_name->len, 0);
										
					zend_str_tolower_copy(lcname->val,
						precedences[i]->trait_method->method_name->val,
						precedences[i]->trait_method->method_name->len);
					if (zend_hash_add_empty_element(exclude_table, lcname) == NULL) {
						STR_RELEASE(lcname);
						zend_error_noreturn(E_COMPILE_ERROR, "Failed to evaluate a trait precedence (%s). Method of trait %s was defined to be excluded multiple times", precedences[i]->trait_method->method_name->val, trait->name->val);
					}
					STR_RELEASE(lcname);
				}
				++j;
			}
		}
		++i;
	}
}
/* }}} */

static void zend_do_traits_method_binding(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	zend_uint i;
	HashTable *overriden = NULL;
	zend_string *key;
	zend_function *fn;

	for (i = 0; i < ce->num_traits; i++) {
		if (ce->trait_precedences) {
			HashTable exclude_table;

			/* TODO: revisit this start size, may be its not optimal */
			zend_hash_init_ex(&exclude_table, 8, NULL, NULL, 0, 0);

			zend_traits_compile_exclude_table(&exclude_table, ce->trait_precedences, ce->traits[i]);

			/* copies functions, applies defined aliasing, and excludes unused trait methods */
			ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->traits[i]->function_table, key, fn) {
				zend_traits_copy_functions(key, fn, ce, &overriden, &exclude_table TSRMLS_CC);
			} ZEND_HASH_FOREACH_END();

			zend_hash_destroy(&exclude_table);
		} else {
			ZEND_HASH_FOREACH_STR_KEY_PTR(&ce->traits[i]->function_table, key, fn) {
				zend_traits_copy_functions(key, fn, ce, &overriden, NULL TSRMLS_CC);
			} ZEND_HASH_FOREACH_END();
		}
	}

	ZEND_HASH_FOREACH_PTR(&ce->function_table, fn) {
		zend_fixup_trait_method(fn, ce);
	} ZEND_HASH_FOREACH_END();

	if (overriden) {
		zend_hash_destroy(overriden);
		FREE_HASHTABLE(overriden);
	}
}
/* }}} */

static zend_class_entry* find_first_definition(zend_class_entry *ce, size_t current_trait, zend_string *prop_name, zend_class_entry *coliding_ce) /* {{{ */
{
	size_t i;

	if (coliding_ce == ce) {
		for (i = 0; i < current_trait; i++) {
			if (zend_hash_exists(&ce->traits[i]->properties_info, prop_name)) {
				return ce->traits[i];
			}
		}
	}

	return coliding_ce;
}
/* }}} */

static void zend_do_traits_property_binding(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	size_t i;
	zend_property_info *property_info;
	zend_property_info *coliding_prop;
	zval compare_result;
	zend_string* prop_name;
	const char* class_name_unused;
	zend_bool not_compatible;
	zval* prop_value;
	zend_uint flags;
	zend_string *doc_comment;

	/* In the following steps the properties are inserted into the property table
	 * for that, a very strict approach is applied:
	 * - check for compatibility, if not compatible with any property in class -> fatal
	 * - if compatible, then strict notice
	 */
	for (i = 0; i < ce->num_traits; i++) {
		ZEND_HASH_FOREACH_PTR(&ce->traits[i]->properties_info, property_info) {
			/* first get the unmangeld name if necessary,
			 * then check whether the property is already there
			 */
			flags = property_info->flags;
			if ((flags & ZEND_ACC_PPP_MASK) == ZEND_ACC_PUBLIC) {
				prop_name = STR_COPY(property_info->name);
			} else {
				const char *pname;
				int pname_len;

				/* for private and protected we need to unmangle the names */
				zend_unmangle_property_name_ex(property_info->name->val, property_info->name->len,
											&class_name_unused, &pname, &pname_len);
				prop_name = STR_INIT(pname, pname_len, 0);
			}

			/* next: check for conflicts with current class */
			if ((coliding_prop = zend_hash_find_ptr(&ce->properties_info, prop_name)) != NULL) {
				if (coliding_prop->flags & ZEND_ACC_SHADOW) {
					zend_hash_del(&ce->properties_info, prop_name);
					flags |= ZEND_ACC_CHANGED;
				} else {
					if ((coliding_prop->flags & (ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC))
						== (flags & (ZEND_ACC_PPP_MASK | ZEND_ACC_STATIC))) {
						/* flags are identical, now the value needs to be checked */
						if (flags & ZEND_ACC_STATIC) {
							not_compatible = (FAILURE == compare_function(&compare_result,
											  &ce->default_static_members_table[coliding_prop->offset],
											  &ce->traits[i]->default_static_members_table[property_info->offset] TSRMLS_CC))
								  || (Z_LVAL(compare_result) != 0);
						} else {
							not_compatible = (FAILURE == compare_function(&compare_result,
											  &ce->default_properties_table[coliding_prop->offset],
											  &ce->traits[i]->default_properties_table[property_info->offset] TSRMLS_CC))
								  || (Z_LVAL(compare_result) != 0);
						}
					} else {
						/* the flags are not identical, thus, we assume properties are not compatible */
						not_compatible = 1;
					}

					if (not_compatible) {
						zend_error_noreturn(E_COMPILE_ERROR,
							   "%s and %s define the same property ($%s) in the composition of %s. However, the definition differs and is considered incompatible. Class was composed",
								find_first_definition(ce, i, prop_name, coliding_prop->ce)->name->val,
								property_info->ce->name->val,
								prop_name->val,
								ce->name->val);
					} else {
						zend_error(E_STRICT,
							   "%s and %s define the same property ($%s) in the composition of %s. This might be incompatible, to improve maintainability consider using accessor methods in traits instead. Class was composed",
								find_first_definition(ce, i, prop_name, coliding_prop->ce)->name->val,
								property_info->ce->name->val,
								prop_name->val,
								ce->name->val);
						STR_RELEASE(prop_name);
						continue;
					}
				}
			}

			/* property not found, so lets add it */
			if (flags & ZEND_ACC_STATIC) {
				prop_value = &ce->traits[i]->default_static_members_table[property_info->offset];
			} else {
				prop_value = &ce->traits[i]->default_properties_table[property_info->offset];
			}
			if (Z_REFCOUNTED_P(prop_value)) Z_ADDREF_P(prop_value);

			doc_comment = property_info->doc_comment ? STR_COPY(property_info->doc_comment) : NULL;
			zend_declare_property_ex(ce, prop_name,
									 prop_value, flags,
								     doc_comment TSRMLS_CC);
			STR_RELEASE(prop_name);
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */

static void zend_do_check_for_inconsistent_traits_aliasing(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{
	int i = 0;
	zend_trait_alias* cur_alias;
	zend_string* lc_method_name;

	if (ce->trait_aliases) {
		while (ce->trait_aliases[i]) {
			cur_alias = ce->trait_aliases[i];
			/** The trait for this alias has not been resolved, this means, this
				alias was not applied. Abort with an error. */
			if (!cur_alias->trait_method->ce) {
				if (cur_alias->alias) {
					/** Plain old inconsistency/typo/bug */
					zend_error_noreturn(E_COMPILE_ERROR,
							   "An alias (%s) was defined for method %s(), but this method does not exist",
							   cur_alias->alias->val,
							   cur_alias->trait_method->method_name->val);
				} else {
					/** Here are two possible cases:
						1) this is an attempt to modifiy the visibility
						   of a method introduce as part of another alias.
						   Since that seems to violate the DRY principle,
						   we check against it and abort.
						2) it is just a plain old inconsitency/typo/bug
						   as in the case where alias is set. */

					lc_method_name = STR_ALLOC(cur_alias->trait_method->method_name->len, 0);
					zend_str_tolower_copy(
						lc_method_name->val,
						cur_alias->trait_method->method_name->val,
						cur_alias->trait_method->method_name->len);
					if (zend_hash_exists(&ce->function_table,
										 lc_method_name)) {
						STR_FREE(lc_method_name);
						zend_error_noreturn(E_COMPILE_ERROR,
								   "The modifiers for the trait alias %s() need to be changed in the same statment in which the alias is defined. Error",
								   cur_alias->trait_method->method_name->val);
					} else {
						STR_FREE(lc_method_name);
						zend_error_noreturn(E_COMPILE_ERROR,
								   "The modifiers of the trait method %s() are changed, but this method does not exist. Error",
								   cur_alias->trait_method->method_name->val);

					}
				}
			}
			i++;
		}
	}
}
/* }}} */

ZEND_API void zend_do_bind_traits(zend_class_entry *ce TSRMLS_DC) /* {{{ */
{

	if (ce->num_traits <= 0) {
		return;
	}

	/* complete initialization of trait strutures in ce */
	zend_traits_init_trait_structures(ce TSRMLS_CC);

	/* first care about all methods to be flattened into the class */
	zend_do_traits_method_binding(ce TSRMLS_CC);

	/* Aliases which have not been applied indicate typos/bugs. */
	zend_do_check_for_inconsistent_traits_aliasing(ce TSRMLS_CC);

	/* then flatten the properties into it, to, mostly to notfiy developer about problems */
	zend_do_traits_property_binding(ce TSRMLS_CC);

	/* verify that all abstract methods from traits have been implemented */
	zend_verify_abstract_class(ce TSRMLS_CC);

	/* now everything should be fine and an added ZEND_ACC_IMPLICIT_ABSTRACT_CLASS should be removed */
	if (ce->ce_flags & ZEND_ACC_IMPLICIT_ABSTRACT_CLASS) {
		ce->ce_flags -= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
	}
}
/* }}} */

ZEND_API int do_bind_function(const zend_op_array *op_array, zend_op *opline, HashTable *function_table, zend_bool compile_time TSRMLS_DC) /* {{{ */
{
	zend_function *function, *new_function;
	zval *op1, *op2;

	if (compile_time) {
		op1 = &CONSTANT_EX(op_array, opline->op1.constant);
		op2 = &CONSTANT_EX(op_array, opline->op2.constant);
	} else {
		op1 = opline->op1.zv;
		op2 = opline->op2.zv;
	}

	function = zend_hash_find_ptr(function_table, Z_STR_P(op1));
	new_function = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	memcpy(new_function, function, sizeof(zend_op_array));
	if (zend_hash_add_ptr(function_table, Z_STR_P(op2), new_function) == NULL) {
		int error_level = compile_time ? E_COMPILE_ERROR : E_ERROR;
		zend_function *old_function;

		efree(new_function);
		if ((old_function = zend_hash_find_ptr(function_table, Z_STR_P(op2))) != NULL
			&& old_function->type == ZEND_USER_FUNCTION
			&& old_function->op_array.last > 0) {
			zend_error(error_level, "Cannot redeclare %s() (previously declared in %s:%d)",
						function->common.function_name->val,
						old_function->op_array.filename->val,
						old_function->op_array.opcodes[0].lineno);
		} else {
			zend_error(error_level, "Cannot redeclare %s()", function->common.function_name->val);
		}
		return FAILURE;
	} else {
		(*function->op_array.refcount)++;
		function->op_array.static_variables = NULL; /* NULL out the unbound function */
		return SUCCESS;
	}
}
/* }}} */

ZEND_API zend_class_entry *do_bind_class(const zend_op_array* op_array, const zend_op *opline, HashTable *class_table, zend_bool compile_time TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce;
	zval *op1, *op2;

	if (compile_time) {
		op1 = &CONSTANT_EX(op_array, opline->op1.constant);
		op2 = &CONSTANT_EX(op_array, opline->op2.constant);
	} else {
		op1 = opline->op1.zv;
		op2 = opline->op2.zv;
	}
	if ((ce = zend_hash_find_ptr(class_table, Z_STR_P(op1))) == NULL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Internal Zend error - Missing class information for %s", Z_STRVAL_P(op1));
		return NULL;
	}
	ce->refcount++;
	if (zend_hash_add_ptr(class_table, Z_STR_P(op2), ce) == NULL) {
		ce->refcount--;
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare class %s", ce->name->val);
		}
		return NULL;
	} else {
		if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_IMPLEMENT_INTERFACES|ZEND_ACC_IMPLEMENT_TRAITS))) {
			zend_verify_abstract_class(ce TSRMLS_CC);
		}
		return ce;
	}
}
/* }}} */

ZEND_API zend_class_entry *do_bind_inherited_class(const zend_op_array *op_array, const zend_op *opline, HashTable *class_table, zend_class_entry *parent_ce, zend_bool compile_time TSRMLS_DC) /* {{{ */
{
	zend_class_entry *ce;
	zval *op1, *op2;

	if (compile_time) {
		op1 = &CONSTANT_EX(op_array, opline->op1.constant);
		op2 = &CONSTANT_EX(op_array, opline->op2.constant);
	} else {
		op1 = opline->op1.zv;
		op2 = opline->op2.zv;
	}

	ce = zend_hash_find_ptr(class_table, Z_STR_P(op1));

	if (!ce) {
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare class %s", Z_STRVAL_P(op2));
		}
		return NULL;
	}

	if (parent_ce->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot extend from interface %s", ce->name->val, parent_ce->name->val);
	} else if ((parent_ce->ce_flags & ZEND_ACC_TRAIT) == ZEND_ACC_TRAIT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Class %s cannot extend from trait %s", ce->name->val, parent_ce->name->val);
	}

	zend_do_inheritance(ce, parent_ce TSRMLS_CC);

	ce->refcount++;

	/* Register the derived class */
	if (zend_hash_add_ptr(class_table, Z_STR_P(op2), ce) == NULL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare class %s", ce->name->val);
	}
	return ce;
}
/* }}} */

void zend_do_early_binding(TSRMLS_D) /* {{{ */
{
	zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
	HashTable *table;

	while (opline->opcode == ZEND_TICKS && opline > CG(active_op_array)->opcodes) {
		opline--;
	}

	switch (opline->opcode) {
		case ZEND_DECLARE_FUNCTION:
			if (do_bind_function(CG(active_op_array), opline, CG(function_table), 1 TSRMLS_CC) == FAILURE) {
				return;
			}
			table = CG(function_table);
			break;
		case ZEND_DECLARE_CLASS:
			if (do_bind_class(CG(active_op_array), opline, CG(class_table), 1 TSRMLS_CC) == NULL) {
				return;
			}
			table = CG(class_table);
			break;
		case ZEND_DECLARE_INHERITED_CLASS:
			{
				zend_op *fetch_class_opline = opline-1;
				zval *parent_name;
				zend_class_entry *ce;

				parent_name = &CONSTANT(fetch_class_opline->op2.constant);
				if (((ce = zend_lookup_class(Z_STR_P(parent_name) TSRMLS_CC)) == NULL) ||
				    ((CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_CLASSES) &&
				     (ce->type == ZEND_INTERNAL_CLASS))) {
				    if (CG(compiler_options) & ZEND_COMPILE_DELAYED_BINDING) {
						zend_uint *opline_num = &CG(active_op_array)->early_binding;

						while (*opline_num != -1) {
							opline_num = &CG(active_op_array)->opcodes[*opline_num].result.opline_num;
						}
						*opline_num = opline - CG(active_op_array)->opcodes;
						opline->opcode = ZEND_DECLARE_INHERITED_CLASS_DELAYED;
						opline->result_type = IS_UNUSED;
						opline->result.opline_num = -1;
					}
					return;
				}
				if (do_bind_inherited_class(CG(active_op_array), opline, CG(class_table), ce, 1 TSRMLS_CC) == NULL) {
					return;
				}
				/* clear unnecessary ZEND_FETCH_CLASS opcode */
				zend_del_literal(CG(active_op_array), fetch_class_opline->op2.constant);
				MAKE_NOP(fetch_class_opline);

				table = CG(class_table);
				break;
			}
		case ZEND_VERIFY_ABSTRACT_CLASS:
		case ZEND_ADD_INTERFACE:
		case ZEND_ADD_TRAIT:
		case ZEND_BIND_TRAITS:
			/* We currently don't early-bind classes that implement interfaces */
			/* Classes with traits are handled exactly the same, no early-bind here */
			return;
		default:
			zend_error_noreturn(E_COMPILE_ERROR, "Invalid binding type");
			return;
	}

	zend_hash_del(table, Z_STR(CONSTANT(opline->op1.constant)));
	zend_del_literal(CG(active_op_array), opline->op1.constant);
	zend_del_literal(CG(active_op_array), opline->op2.constant);
	MAKE_NOP(opline);
}
/* }}} */

ZEND_API void zend_do_delayed_early_binding(const zend_op_array *op_array TSRMLS_DC) /* {{{ */
{
	if (op_array->early_binding != -1) {
		zend_bool orig_in_compilation = CG(in_compilation);
		zend_uint opline_num = op_array->early_binding;
		zend_class_entry *ce;

		CG(in_compilation) = 1;
		while (opline_num != -1) {
			if ((ce = zend_lookup_class(Z_STR_P(op_array->opcodes[opline_num-1].op2.zv) TSRMLS_CC)) != NULL) {
				do_bind_inherited_class(op_array, &op_array->opcodes[opline_num], EG(class_table), ce, 0 TSRMLS_CC);
			}
			opline_num = op_array->opcodes[opline_num].result.opline_num;
		}
		CG(in_compilation) = orig_in_compilation;
	}
}
/* }}} */

ZEND_API zend_string *zend_mangle_property_name(const char *src1, int src1_length, const char *src2, int src2_length, int internal) /* {{{ */
{
	zend_string *prop_name;
	int prop_name_length;

	prop_name_length = 1 + src1_length + 1 + src2_length;
	prop_name = STR_ALLOC(prop_name_length, internal);
	prop_name->val[0] = '\0';
	memcpy(prop_name->val + 1, src1, src1_length+1);
	memcpy(prop_name->val + 1 + src1_length + 1, src2, src2_length+1);
	return prop_name;
}
/* }}} */

static int zend_strnlen(const char* s, int maxlen) /* {{{ */
{
	int len = 0;
	while (*s++ && maxlen--) len++;
	return len;
}
/* }}} */

ZEND_API int zend_unmangle_property_name_ex(const char *mangled_property, int len, const char **class_name, const char **prop_name, int *prop_len) /* {{{ */
{
	int class_name_len;

	*class_name = NULL;

	if (mangled_property[0]!=0) {
		*prop_name = mangled_property;
		if (prop_len) {
			*prop_len = len;
		}
		return SUCCESS;
	}
	if (len < 3 || mangled_property[1]==0) {
		zend_error(E_NOTICE, "Illegal member variable name");
		*prop_name = mangled_property;
		if (prop_len) {
			*prop_len = len;
		}
		return FAILURE;
	}

	class_name_len = zend_strnlen(mangled_property + 1, --len - 1) + 1;
	if (class_name_len >= len || mangled_property[class_name_len]!=0) {
		zend_error(E_NOTICE, "Corrupt member variable name");
		*prop_name = mangled_property;
		if (prop_len) {
			*prop_len = len + 1;
		}
		return FAILURE;
	}
	*class_name = mangled_property + 1;
	*prop_name = (*class_name) + class_name_len;
	if (prop_len) {
		*prop_len = len - class_name_len;
	}
	return SUCCESS;
}
/* }}} */

static zend_constant *zend_get_ct_const(zend_string *name, int all_internal_constants_substitution TSRMLS_DC) /* {{{ */
{
	zend_constant *c = NULL;
	char *lookup_name;

	if (name->val[0] == '\\') {
		c = zend_hash_str_find_ptr(EG(zend_constants), name->val + 1, name->len - 1);
		if (!c) {
			lookup_name = zend_str_tolower_dup(name->val + 1, name->len - 1);
			c = zend_hash_str_find_ptr(EG(zend_constants), lookup_name, name->len - 1);
			efree(lookup_name);

			if (c && (c->flags & CONST_CT_SUBST) && !(c->flags & CONST_CS)) {
				return c;
			}
			return NULL;
		}
	} else if ((c = zend_hash_find_ptr(EG(zend_constants), name)) == NULL) {
		lookup_name = zend_str_tolower_dup(name->val, name->len);
		c = zend_hash_str_find_ptr(EG(zend_constants), lookup_name, name->len);
		efree(lookup_name);

		if (c && (c->flags & CONST_CT_SUBST) && !(c->flags & CONST_CS)) {
			return c;
		}
		return NULL;
	}

	if (c->flags & CONST_CT_SUBST) {
		return c;
	}
	if (all_internal_constants_substitution &&
	    (c->flags & CONST_PERSISTENT) &&
	    !(CG(compiler_options) & ZEND_COMPILE_NO_CONSTANT_SUBSTITUTION) &&
	    !Z_CONSTANT(c->value)) {
		return c;
	}
	return NULL;
}
/* }}} */

static int zend_constant_ct_subst(znode *result, zval *const_name, int all_internal_constants_substitution TSRMLS_DC) /* {{{ */
{
	zend_constant *c = zend_get_ct_const(Z_STR_P(const_name),
		all_internal_constants_substitution TSRMLS_CC);

	if (c) {
		result->op_type = IS_CONST;
		result->u.constant = c->value;
		zval_copy_ctor(&result->u.constant);
		return 1;
	}
	return 0;
}
/* }}} */

void zend_init_list(void *result, void *item TSRMLS_DC) /* {{{ */
{
	void** list = emalloc(sizeof(void*) * 2);

	list[0] = item;
	list[1] = NULL;

	*(void**)result = list;
}
/* }}} */

void zend_add_to_list(void *result, void *item TSRMLS_DC) /* {{{ */
{
	void** list = *(void**)result;
	size_t n = 0;

	if (list) {
		while (list[n]) {
			n++;
		}
	}

	list = erealloc(list, sizeof(void*) * (n+2));

	list[n]   = item;
	list[n+1] = NULL;

	*(void**)result = list;
}
/* }}} */

void zend_do_extended_info(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_STMT;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_extended_fcall_begin(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_BEGIN;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

void zend_do_extended_fcall_end(TSRMLS_D) /* {{{ */
{
	zend_op *opline;

	if (!(CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_END;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}
/* }}} */

zend_bool zend_is_auto_global(zend_string *name TSRMLS_DC) /* {{{ */
{
	zend_auto_global *auto_global;

	if ((auto_global = zend_hash_find_ptr(CG(auto_globals), name)) != NULL) {
		if (auto_global->armed) {
			auto_global->armed = auto_global->auto_global_callback(auto_global->name TSRMLS_CC);
		}
		return 1;
	}
	return 0;
}
/* }}} */

int zend_register_auto_global(zend_string *name, zend_bool jit, zend_auto_global_callback auto_global_callback TSRMLS_DC) /* {{{ */
{
	zend_auto_global auto_global;
	int retval;

	auto_global.name = zend_new_interned_string(name TSRMLS_CC);
	auto_global.auto_global_callback = auto_global_callback;
	auto_global.jit = jit;

	retval = zend_hash_add_mem(CG(auto_globals), name, &auto_global, sizeof(zend_auto_global)) != NULL ? SUCCESS : FAILURE;

	STR_RELEASE(auto_global.name);
	return retval;
}
/* }}} */

ZEND_API void zend_activate_auto_globals(TSRMLS_D) /* {{{ */
{
	zend_auto_global *auto_global;

	ZEND_HASH_FOREACH_PTR(CG(auto_globals), auto_global) {
		if (auto_global->jit) {
			auto_global->armed = 1;
		} else if (auto_global->auto_global_callback) {
			auto_global->armed = auto_global->auto_global_callback(auto_global->name TSRMLS_CC);
		} else {
			auto_global->armed = 0;
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

int zendlex(zend_parser_stack_elem *elem TSRMLS_DC) /* {{{ */
{
	zval zv;
	int retval;

	if (CG(increment_lineno)) {
		CG(zend_lineno)++;
		CG(increment_lineno) = 0;
	}

again:
	ZVAL_UNDEF(&zv);
	retval = lex_scan(&zv TSRMLS_CC);
	switch (retval) {
		case T_COMMENT:
		case T_DOC_COMMENT:
		case T_OPEN_TAG:
		case T_WHITESPACE:
			goto again;

		case T_CLOSE_TAG:
			if (LANG_SCNG(yy_text)[LANG_SCNG(yy_leng)-1] != '>') {
				CG(increment_lineno) = 1;
			}
			if (CG(has_bracketed_namespaces) && !CG(in_namespace)) {
				goto again;
			}
			retval = ';'; /* implicit ; */
			break;
		case T_OPEN_TAG_WITH_ECHO:
			retval = T_ECHO;
			break;
	}
	if (Z_TYPE(zv) != IS_UNDEF) {
		elem->ast = zend_ast_create_zval(&zv);
	}

	return retval;
}
/* }}} */

ZEND_API void zend_initialize_class_data(zend_class_entry *ce, zend_bool nullify_handlers TSRMLS_DC) /* {{{ */
{
	zend_bool persistent_hashes = (ce->type == ZEND_INTERNAL_CLASS) ? 1 : 0;
	dtor_func_t zval_ptr_dtor_func = ((persistent_hashes) ? ZVAL_INTERNAL_PTR_DTOR : ZVAL_PTR_DTOR);

	ce->refcount = 1;
	ce->ce_flags = ZEND_ACC_CONSTANTS_UPDATED;

	ce->default_properties_table = NULL;
	ce->default_static_members_table = NULL;
	zend_hash_init_ex(&ce->properties_info, 8, NULL, (persistent_hashes ? zend_destroy_property_info_internal : zend_destroy_property_info), persistent_hashes, 0);
	zend_hash_init_ex(&ce->constants_table, 8, NULL, zval_ptr_dtor_func, persistent_hashes, 0);
	zend_hash_init_ex(&ce->function_table, 8, NULL, ZEND_FUNCTION_DTOR, persistent_hashes, 0);

	if (ce->type == ZEND_INTERNAL_CLASS) {
#ifdef ZTS
		int n = zend_hash_num_elements(CG(class_table));

		if (CG(static_members_table) && n >= CG(last_static_member)) {
			/* Support for run-time declaration: dl() */
			CG(last_static_member) = n+1;
			CG(static_members_table) = realloc(CG(static_members_table), (n+1)*sizeof(zval*));
			CG(static_members_table)[n] = NULL;
		}
		ce->static_members_table = (zval*)(zend_intptr_t)n;
#else
		ce->static_members_table = NULL;
#endif
	} else {
		ce->static_members_table = ce->default_static_members_table;
		ce->info.user.doc_comment = NULL;
	}

	ce->default_properties_count = 0;
	ce->default_static_members_count = 0;

	if (nullify_handlers) {
		ce->constructor = NULL;
		ce->destructor = NULL;
		ce->clone = NULL;
		ce->__get = NULL;
		ce->__set = NULL;
		ce->__unset = NULL;
		ce->__isset = NULL;
		ce->__call = NULL;
		ce->__callstatic = NULL;
		ce->__tostring = NULL;
		ce->create_object = NULL;
		ce->get_iterator = NULL;
		ce->iterator_funcs.funcs = NULL;
		ce->interface_gets_implemented = NULL;
		ce->get_static_method = NULL;
		ce->parent = NULL;
		ce->num_interfaces = 0;
		ce->interfaces = NULL;
		ce->num_traits = 0;
		ce->traits = NULL;
		ce->trait_aliases = NULL;
		ce->trait_precedences = NULL;
		ce->serialize = NULL;
		ce->unserialize = NULL;
		ce->serialize_func = NULL;
		ce->unserialize_func = NULL;
		ce->__debugInfo = NULL;
		if (ce->type == ZEND_INTERNAL_CLASS) {
			ce->info.internal.module = NULL;
			ce->info.internal.builtin_functions = NULL;
		}
	}
}
/* }}} */

int zend_get_class_fetch_type(const char *class_name, uint class_name_len) /* {{{ */
{
	if ((class_name_len == sizeof("self")-1) &&
		!strncasecmp(class_name, "self", sizeof("self")-1)) {
		return ZEND_FETCH_CLASS_SELF;
	} else if ((class_name_len == sizeof("parent")-1) &&
		!strncasecmp(class_name, "parent", sizeof("parent")-1)) {
		return ZEND_FETCH_CLASS_PARENT;
	} else if ((class_name_len == sizeof("static")-1) &&
		!strncasecmp(class_name, "static", sizeof("static")-1)) {
		return ZEND_FETCH_CLASS_STATIC;
	} else {
		return ZEND_FETCH_CLASS_DEFAULT;
	}
}
/* }}} */

ZEND_API zend_string *zend_get_compiled_variable_name(const zend_op_array *op_array, zend_uint var) /* {{{ */
{
	return op_array->vars[EX_VAR_TO_NUM(var)];
}
/* }}} */

zend_ast *zend_ast_append_str(zend_ast *left_ast, zend_ast *right_ast) {
	zval *left_zv = zend_ast_get_zval(left_ast);
	zend_string *left = Z_STR_P(left_zv);
	zend_string *right = zend_ast_get_str(right_ast);

	zend_string *result;
	size_t left_len = left->len;
	size_t len = left_len + right->len + 1; /* left\right */

	result = STR_REALLOC(left, len, 0);
	result->val[left_len] = '\\';
	memcpy(&result->val[left_len + 1], right->val, right->len);
	result->val[len] = '\0';
	STR_RELEASE(right);

	ZVAL_STR(left_zv, result);
	return left_ast;
}

void zend_verify_namespace(TSRMLS_D) /* {{{ */
{
	if (CG(has_bracketed_namespaces) && !CG(in_namespace)) {
		zend_error_noreturn(E_COMPILE_ERROR, "No code may exist outside of namespace {}");
	}
}
/* }}} */

void zend_do_end_namespace(TSRMLS_D) /* {{{ */
{
	CG(in_namespace) = 0;
	if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
		zval_dtor(&CG(current_namespace));
		ZVAL_UNDEF(&CG(current_namespace));
	}
	if (CG(current_import)) {
		zend_hash_destroy(CG(current_import));
		efree(CG(current_import));
		CG(current_import) = NULL;
	}
	if (CG(current_import_function)) {
		zend_hash_destroy(CG(current_import_function));
		efree(CG(current_import_function));
		CG(current_import_function) = NULL;
	}
	if (CG(current_import_const)) {
		zend_hash_destroy(CG(current_import_const));
		efree(CG(current_import_const));
		CG(current_import_const) = NULL;
	}
}
/* }}} */

void zend_do_end_compilation(TSRMLS_D) /* {{{ */
{
	CG(has_bracketed_namespaces) = 0;
	zend_do_end_namespace(TSRMLS_C);
}
/* }}} */

ZEND_API void zend_make_immutable_array(zval *zv TSRMLS_DC) /* {{{ */
{
	zend_constant *c;

	if (Z_IMMUTABLE_P(zv)) {
		return;
	}

	Z_TYPE_FLAGS_P(zv) = IS_TYPE_IMMUTABLE;
	GC_REFCOUNT(Z_COUNTED_P(zv)) = 2;
	Z_ARRVAL_P(zv)->u.flags &= ~HASH_FLAG_APPLY_PROTECTION;

	/* store as an anonymous constant */
	c = emalloc(sizeof(zend_constant));
	ZVAL_COPY_VALUE(&c->value, zv);
	c->flags = 0;
	c->name = NULL;
	c->module_number = PHP_USER_CONSTANT;
	zend_hash_next_index_insert_ptr(EG(zend_constants), c);
}
/* }}} */

void zend_make_immutable_array_r(zval *zv TSRMLS_DC) /* {{{ */
{
	zval *el;

	if (Z_IMMUTABLE_P(zv)) {
		return;
	}
	zend_make_immutable_array(zv TSRMLS_CC);
	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zv), el) {
		if (Z_TYPE_P(el) == IS_ARRAY) {
			zend_make_immutable_array_r(el TSRMLS_CC);			
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

void zend_compile_const_expr(zend_ast **ast_ptr TSRMLS_DC);

// TODO.AST Sort out the whole constant folding issue
static void _tmp_compile_const_expr(zval *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *orig_ast = ast;
	zend_eval_const_expr(&ast TSRMLS_CC);
	zend_compile_const_expr(&ast TSRMLS_CC);
	if (ast->kind == ZEND_AST_ZVAL) {
		ZVAL_COPY_VALUE(result, zend_ast_get_zval(ast));
		if (Z_TYPE_P(result) == IS_ARRAY) {
			zend_make_immutable_array_r(result TSRMLS_CC);			
		}
		orig_ast->kind = ZEND_AST_ZNODE;
	} else {
		ZVAL_NEW_AST(result, zend_ast_copy(ast));
	}
}

/* {{{ zend_dirname
   Returns directory name component of path */
ZEND_API size_t zend_dirname(char *path, size_t len)
{
	register char *end = path + len - 1;
	unsigned int len_adjust = 0;

#ifdef PHP_WIN32
	/* Note that on Win32 CWD is per drive (heritage from CP/M).
	 * This means dirname("c:foo") maps to "c:." or "c:" - which means CWD on C: drive.
	 */
	if ((2 <= len) && isalpha((int)((unsigned char *)path)[0]) && (':' == path[1])) {
		/* Skip over the drive spec (if any) so as not to change */
		path += 2;
		len_adjust += 2;
		if (2 == len) {
			/* Return "c:" on Win32 for dirname("c:").
			 * It would be more consistent to return "c:."
			 * but that would require making the string *longer*.
			 */
			return len;
		}
	}
#elif defined(NETWARE)
	/*
	 * Find the first occurrence of : from the left
	 * move the path pointer to the position just after :
	 * increment the len_adjust to the length of path till colon character(inclusive)
	 * If there is no character beyond : simple return len
	 */
	char *colonpos = NULL;
	colonpos = strchr(path, ':');
	if (colonpos != NULL) {
		len_adjust = ((colonpos - path) + 1);
		path += len_adjust;
		if (len_adjust == len) {
			return len;
		}
	}
#endif

	if (len == 0) {
		/* Illegal use of this function */
		return 0;
	}

	/* Strip trailing slashes */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* The path only contained slashes */
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}

	/* Strip filename */
	while (end >= path && !IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		/* No slash found, therefore return '.' */
#ifdef NETWARE
		if (len_adjust == 0) {
			path[0] = '.';
			path[1] = '\0';
			return 1; /* only one character */
		} else {
			path[0] = '\0';
			return len_adjust;
		}
#else
		path[0] = '.';
		path[1] = '\0';
		return 1 + len_adjust;
#endif
	}

	/* Strip slashes which came before the file name */
	while (end >= path && IS_SLASH_P(end)) {
		end--;
	}
	if (end < path) {
		path[0] = DEFAULT_SLASH;
		path[1] = '\0';
		return 1 + len_adjust;
	}
	*(end+1) = '\0';

	return (size_t)(end + 1 - path) + len_adjust;
}
/* }}} */

static inline zend_bool zend_str_equals(zend_string *str, char *c) {
	size_t len = strlen(c);
	return str->len == len && !memcmp(str->val, c, len);
}


static void zend_adjust_for_fetch_type(zend_op *opline, zend_uint type) {
	switch (type & BP_VAR_MASK) {
		case BP_VAR_R:
			return;
		case BP_VAR_W:
			opline->opcode += 3;
			return;
		case BP_VAR_REF:
			opline->opcode += 3;
			opline->extended_value |= ZEND_FETCH_MAKE_REF;
			return;
		case BP_VAR_RW:
			opline->opcode += 6;
			return;
		case BP_VAR_IS:
			opline->opcode += 9;
			return;
		case BP_VAR_FUNC_ARG:
			opline->opcode += 12;
			opline->extended_value |= type >> BP_VAR_SHIFT;
			return;
		case BP_VAR_UNSET:
			opline->opcode += 15;
			return;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static zend_op *emit_op(znode *result, zend_uchar opcode, znode *op1, znode *op2 TSRMLS_DC) {
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = opcode;

	if (op1 == NULL) {
		SET_UNUSED(opline->op1);
	} else {
		SET_NODE(opline->op1, op1);
	}

	if (op2 == NULL) {
		SET_UNUSED(opline->op2);
	} else {
		SET_NODE(opline->op2, op2);
	}

	if (result == NULL) {
		SET_UNUSED(opline->result);
	} else {
		opline->result_type = IS_VAR;
		opline->result.var = get_temporary_variable(CG(active_op_array));
		GET_NODE(result, opline->result);
	}
	return opline;
}

static zend_op *emit_op_tmp(znode *result, zend_uchar opcode, znode *op1, znode *op2 TSRMLS_DC) {
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = opcode;

	if (op1 == NULL) {
		SET_UNUSED(opline->op1);
	} else {
		SET_NODE(opline->op1, op1);
	}

	if (op2 == NULL) {
		SET_UNUSED(opline->op2);
	} else {
		SET_NODE(opline->op2, op2);
	}

	opline->result_type = IS_TMP_VAR;
	opline->result.var = get_temporary_variable(CG(active_op_array));
	GET_NODE(result, opline->result);

	return opline;
}

static void zend_emit_tick(TSRMLS_D) {
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_TICKS;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	opline->extended_value = Z_LVAL(CG(declarables).ticks);
}

static zend_op *zend_emit_op_data(znode *value TSRMLS_DC) {
	return emit_op(NULL, ZEND_OP_DATA, value, NULL TSRMLS_CC);
}

static zend_uint zend_emit_jump(zend_uint opnum_target TSRMLS_DC) {
	zend_uint opnum = get_next_op_number(CG(active_op_array));
	zend_op *opline = emit_op(NULL, ZEND_JMP, NULL, NULL TSRMLS_CC);
	opline->op1.opline_num = opnum_target;
	return opnum;
}

static zend_uint zend_emit_cond_jump(
	zend_uchar opcode, znode *cond, zend_uint opnum_target TSRMLS_DC
) {
	zend_uint opnum = get_next_op_number(CG(active_op_array));
	zend_op *opline = emit_op(NULL, opcode, cond, NULL TSRMLS_CC);
	opline->op2.opline_num = opnum_target;
	return opnum;
}

static void zend_update_jump_target(zend_uint opnum_jump, zend_uint opnum_target TSRMLS_DC) {
	zend_op *opline = &CG(active_op_array)->opcodes[opnum_jump];
	switch (opline->opcode) {
		case ZEND_JMP:
			opline->op1.opline_num = opnum_target;
			break;
		case ZEND_JMPZ:
		case ZEND_JMPNZ:
		case ZEND_JMPZ_EX:
		case ZEND_JMPNZ_EX:
			opline->op2.opline_num = opnum_target;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static void zend_update_jump_target_to_next(zend_uint opnum_jump TSRMLS_DC) {
	zend_update_jump_target(opnum_jump, get_next_op_number(CG(active_op_array)) TSRMLS_CC);
}

void zend_emit_final_return(zval *zv TSRMLS_DC) {
	znode zn;
	zend_bool returns_reference = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;

	zn.op_type = IS_CONST;
	if (zv) {
		ZVAL_COPY_VALUE(&zn.u.constant, zv);
	} else {
		ZVAL_NULL(&zn.u.constant);
	}

	emit_op(NULL, returns_reference ? ZEND_RETURN_BY_REF : ZEND_RETURN, &zn, NULL TSRMLS_CC);
}


static zend_bool zend_is_variable(zend_ast *ast) {
	return ast->kind == ZEND_AST_VAR || ast->kind == ZEND_AST_DIM
		|| ast->kind == ZEND_AST_PROP || ast->kind == ZEND_AST_STATIC_PROP
		|| ast->kind == ZEND_AST_CALL || ast->kind == ZEND_AST_METHOD_CALL
		|| ast->kind == ZEND_AST_STATIC_CALL;
}

static zend_bool zend_is_call(zend_ast *ast) {
	return ast->kind == ZEND_AST_CALL
		|| ast->kind == ZEND_AST_METHOD_CALL
		|| ast->kind == ZEND_AST_STATIC_CALL;
}

static zend_bool zend_is_unticked_stmt(zend_ast *ast) {
	return ast->kind == ZEND_AST_STMT_LIST || ast->kind == ZEND_AST_LABEL;
}

static zend_bool zend_can_write_to_variable(zend_ast *ast) {
	while (ast->kind == ZEND_AST_DIM || ast->kind == ZEND_AST_PROP) {
		ast = ast->child[0];
	}

	return zend_is_variable(ast);
}

static zend_bool zend_is_const_default_class_ref(zend_ast *name_ast) {
	zval *name;
	int fetch_type;

	if (name_ast->kind != ZEND_AST_ZVAL) {
		return 0;
	}

	/* Fully qualified names are always default refs */
	if (!name_ast->attr) {
		return 1;
	}

	name = zend_ast_get_zval(name_ast);
	fetch_type = zend_get_class_fetch_type(Z_STRVAL_P(name), Z_STRLEN_P(name));
	return fetch_type == ZEND_FETCH_CLASS_DEFAULT;
}

static void zend_handle_numeric_op2(zend_op *opline TSRMLS_DC) {
	if (opline->op2_type == IS_CONST && Z_TYPE(CONSTANT(opline->op2.constant)) == IS_STRING) {
		ulong index;

		if (ZEND_HANDLE_NUMERIC(Z_STR(CONSTANT(opline->op2.constant)), index)) {
			zval_dtor(&CONSTANT(opline->op2.constant));
			ZVAL_LONG(&CONSTANT(opline->op2.constant), index);
		}
	}
}

static void zend_set_class_name_op1(zend_op *opline, znode *class_node TSRMLS_DC) {
	if (class_node->op_type == IS_CONST) {
		opline->op1_type = IS_CONST;
		opline->op1.constant = zend_add_class_name_literal(
			CG(active_op_array), Z_STR(class_node->u.constant) TSRMLS_CC);
	} else {
		SET_NODE(opline->op1, class_node);
	}
}

static zend_op *zend_compile_class_ref(znode *result, zend_ast *name_ast TSRMLS_DC) {
	zend_op *opline;

	if (name_ast->kind == ZEND_AST_ZVAL) {
		zval *name = zend_ast_get_zval(name_ast);
		int fetch_type = zend_get_class_fetch_type(Z_STRVAL_P(name), Z_STRLEN_P(name));

		opline = emit_op(result, ZEND_FETCH_CLASS, NULL, NULL TSRMLS_CC);
		opline->extended_value = fetch_type;

		if (fetch_type == ZEND_FETCH_CLASS_DEFAULT) {
			opline->op2_type = IS_CONST;
			opline->op2.constant = zend_add_class_name_literal(CG(active_op_array),
				zend_resolve_class_name_ast(name_ast TSRMLS_CC) TSRMLS_CC);
		}
	} else {
		znode name_node;
		zend_compile_expr(&name_node, name_ast TSRMLS_CC);
		opline = emit_op(result, ZEND_FETCH_CLASS, NULL, &name_node TSRMLS_CC);
		opline->extended_value = ZEND_FETCH_CLASS_DEFAULT;
	}

	return opline;
}

static int zend_try_compile_cv(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	if (name_ast->kind == ZEND_AST_ZVAL) {
		zend_string *name = zval_get_string(zend_ast_get_zval(name_ast));

		if (zend_is_auto_global(name TSRMLS_CC)) {
			STR_RELEASE(name);
			return FAILURE;
		}

		result->op_type = IS_CV;
		result->u.op.var = lookup_cv(CG(active_op_array), name TSRMLS_CC);

		if (name->len == sizeof("this") - 1 && !memcmp(name->val, "this", sizeof("this") - 1)) {
			CG(active_op_array)->this_var = result->u.op.var;
		}
		return SUCCESS;
	}

	return FAILURE;
}

static zend_op *zend_compile_simple_var_no_cv(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	znode name_node;
	zend_op *opline;

	/* there is a chance someone is accessing $this */
	if (ast->kind != ZEND_AST_ZVAL
		&& CG(active_op_array)->scope && CG(active_op_array)->this_var == -1
	) {
		zend_string *key = STR_INIT("this", sizeof("this") - 1, 0);
		CG(active_op_array)->this_var = lookup_cv(CG(active_op_array), key TSRMLS_CC);
	}

	zend_compile_expr(&name_node, name_ast TSRMLS_CC);

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline = emit_op(result, ZEND_FETCH_R, &name_node, NULL TSRMLS_CC);

	opline->extended_value = ZEND_FETCH_LOCAL;
	if (name_node.op_type == IS_CONST) {
		if (zend_is_auto_global(Z_STR(name_node.u.constant) TSRMLS_CC)) {
			opline->extended_value = ZEND_FETCH_GLOBAL;
		}
	}

	return opline;
}

static void zend_compile_simple_var(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	if (zend_try_compile_cv(result, ast TSRMLS_CC) == FAILURE) {
		zend_op *opline = zend_compile_simple_var_no_cv(result, ast, type TSRMLS_CC);
		zend_adjust_for_fetch_type(opline, type);
	}
}

static void zend_separate_if_call_and_write(znode *node, zend_ast *ast, int type TSRMLS_DC) {
	if (type != BP_VAR_R && type != BP_VAR_IS && zend_is_call(ast)) {
		zend_op *opline = emit_op(NULL, ZEND_SEPARATE, node, NULL TSRMLS_CC);
		opline->result_type = IS_VAR;
		opline->result.var = opline->op1.var;
	}
}

zend_op *zend_compile_dim_common(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];
	zend_ast *dim_ast = ast->child[1];

	znode var_node, dim_node;
	zend_op *opline;

	if (dim_ast == NULL) {
		if (type == BP_VAR_R || type == BP_VAR_IS) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for reading");
		}
		if (type == BP_VAR_UNSET) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use [] for unsetting");
		}
		dim_node.op_type = IS_UNUSED;
	} else {
		zend_compile_expr(&dim_node, dim_ast TSRMLS_CC);
	}

	zend_compile_var(&var_node, var_ast, type TSRMLS_CC);
	zend_separate_if_call_and_write(&var_node, var_ast, type TSRMLS_CC);

	opline = emit_op(result, ZEND_FETCH_DIM_R, &var_node, &dim_node TSRMLS_CC);
	zend_handle_numeric_op2(opline TSRMLS_CC);

	return opline;
}

void zend_compile_dim(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_op *opline = zend_compile_dim_common(result, ast, type TSRMLS_CC);
	zend_adjust_for_fetch_type(opline, type);
}

static zend_bool is_this_fetch(zend_ast *ast) {
	if (ast->kind != ZEND_AST_VAR || ast->child[0]->kind != ZEND_AST_ZVAL) {
		return 0;
	}

	zval *name = zend_ast_get_zval(ast->child[0]);
	return Z_TYPE_P(name) == IS_STRING && Z_STRLEN_P(name) == sizeof("this") - 1
		&& !memcmp(Z_STRVAL_P(name), "this", sizeof("this") - 1);
}

zend_op *zend_compile_prop_common(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *obj_ast = ast->child[0];
	zend_ast *prop_ast = ast->child[1];

	znode obj_node, prop_node;
	zend_op *opline;

	zend_compile_expr(&prop_node, prop_ast TSRMLS_CC);
	if (is_this_fetch(obj_ast)) {
		obj_node.op_type = IS_UNUSED;
	} else {
		zend_compile_var(&obj_node, obj_ast, type TSRMLS_CC);
		zend_separate_if_call_and_write(&obj_node, obj_ast, type TSRMLS_CC);
	}

	opline = emit_op(result, ZEND_FETCH_OBJ_R, &obj_node, &prop_node TSRMLS_CC);
	if (opline->op2_type == IS_CONST && Z_TYPE(CONSTANT(opline->op2.constant)) == IS_STRING) {
		GET_POLYMORPHIC_CACHE_SLOT(opline->op2.constant);
	}

	return opline;
}

void zend_compile_prop(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_op *opline = zend_compile_prop_common(result, ast, type TSRMLS_CC);
	zend_adjust_for_fetch_type(opline, type);
}

zend_op *zend_compile_static_prop_common(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *class_ast = ast->child[0];
	zend_ast *prop_ast = ast->child[1];

	znode class_node, prop_node;
	zend_op *opline;

	if (zend_is_const_default_class_ref(class_ast)) {
		class_node.op_type = IS_CONST;
		ZVAL_STR(&class_node.u.constant, zend_resolve_class_name_ast(class_ast TSRMLS_CC));
	} else {
		zend_compile_class_ref(&class_node, class_ast TSRMLS_CC);
	}

	zend_compile_expr(&prop_node, prop_ast TSRMLS_CC);

	opline = emit_op(result, ZEND_FETCH_R, &prop_node, NULL TSRMLS_CC);
	if (opline->op1_type == IS_CONST) {
		GET_POLYMORPHIC_CACHE_SLOT(opline->op1.constant);
	}
	if (class_node.op_type == IS_CONST) {
		opline->op2_type = IS_CONST;
		opline->op2.constant = zend_add_class_name_literal(
			CG(active_op_array), Z_STR(class_node.u.constant) TSRMLS_CC);
	} else {
		SET_NODE(opline->op2, &class_node);
	}
	opline->extended_value |= ZEND_FETCH_STATIC_MEMBER;

	return opline;
}

void zend_compile_static_prop(znode *result, zend_ast *ast, zend_uint type TSRMLS_DC) {
	zend_op *opline = zend_compile_static_prop_common(result, ast, type TSRMLS_CC);
	zend_adjust_for_fetch_type(opline, type);
}

static zend_uchar get_list_fetch_opcode(zend_uchar op_type) {
	switch (op_type) {
		case IS_VAR:
		case IS_CV:
			return ZEND_FETCH_DIM_R;
		case IS_TMP_VAR:
		case IS_CONST:
			return ZEND_FETCH_DIM_TMP_VAR;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static void zend_compile_list_assign(znode *result, zend_ast *ast, znode *expr_node TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;

	if (list->children == 1 && !list->child[0]) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use empty list");
	}

	for (i = 0; i < list->children; ++i) {
		zend_ast *var_ast = list->child[i];
		znode fetch_result, dim_node, var_node, assign_result;
		zend_op *opline;

		if (var_ast == NULL) {
			continue;
		}

		dim_node.op_type = IS_CONST;
		ZVAL_LONG(&dim_node.u.constant, i);

		opline = emit_op(&fetch_result,
			get_list_fetch_opcode(expr_node->op_type), expr_node, &dim_node TSRMLS_CC);
		opline->extended_value |= ZEND_FETCH_ADD_LOCK;

		if (var_ast->kind != ZEND_AST_LIST) {
			if (is_this_fetch(var_ast)) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
			}
			zend_compile_var(&var_node, var_ast, BP_VAR_W TSRMLS_CC);
			emit_op(&assign_result, ZEND_ASSIGN, &var_node, &fetch_result TSRMLS_CC);
			zend_do_free(&assign_result TSRMLS_CC);
		} else {
			zend_compile_list_assign(&assign_result, var_ast, &fetch_result TSRMLS_CC);
			zend_do_free(&assign_result TSRMLS_CC);
		}
	}
	*result = *expr_node;
}

void zend_ensure_writable_variable(const zend_ast *ast) {
	if (ast->kind == ZEND_AST_CALL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Can't use function return value in write context");
	}
	if (ast->kind == ZEND_AST_METHOD_CALL || ast->kind == ZEND_AST_STATIC_CALL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Can't use method return value in write context");
	}
}

/* Detects $a... = $a pattern */
zend_bool zend_is_assign_to_self(zend_ast *var_ast, zend_ast *expr_ast TSRMLS_DC) {
	if (expr_ast->kind != ZEND_AST_VAR || expr_ast->child[0]->kind != ZEND_AST_ZVAL) {
		return 0;
	}

	while (zend_is_variable(var_ast) && var_ast->kind != ZEND_AST_VAR) {
		var_ast = var_ast->child[0];
	}

	if (var_ast->kind != ZEND_AST_VAR || var_ast->child[0]->kind != ZEND_AST_ZVAL) {
		return 0;
	}

	{
		zend_string *name1 = zval_get_string(zend_ast_get_zval(var_ast->child[0]));
		zend_string *name2 = zval_get_string(zend_ast_get_zval(expr_ast->child[0]));
		zend_bool result = name1->len == name2->len && !memcmp(name1->val, name2->val, name1->len);
		STR_RELEASE(name1);
		STR_RELEASE(name2);
		return result;
	}
}

void zend_compile_assign(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];
	zend_ast *expr_ast = ast->child[1];

	znode var_node, expr_node;
	zend_op *opline;

	if (is_this_fetch(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	zend_ensure_writable_variable(var_ast);

	if (zend_is_assign_to_self(var_ast, expr_ast TSRMLS_CC)) {
		zend_compile_simple_var_no_cv(&expr_node, expr_ast, BP_VAR_R TSRMLS_CC);
	} else {
		zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);
	}

	switch (var_ast->kind) {
		case ZEND_AST_VAR:
		case ZEND_AST_STATIC_PROP:
			zend_compile_var(&var_node, var_ast, BP_VAR_W TSRMLS_CC);
			emit_op(result, ZEND_ASSIGN, &var_node, &expr_node TSRMLS_CC);
			return;
		case ZEND_AST_DIM:
			opline = zend_compile_dim_common(result, var_ast, BP_VAR_W TSRMLS_CC);
			opline->opcode = ZEND_ASSIGN_DIM;

			opline = zend_emit_op_data(&expr_node TSRMLS_CC);
			opline->op2.var = get_temporary_variable(CG(active_op_array));
			opline->op2_type = IS_VAR;
			return;
		case ZEND_AST_PROP:
			opline = zend_compile_prop_common(result, var_ast, BP_VAR_W TSRMLS_CC);
			opline->opcode = ZEND_ASSIGN_OBJ;

			zend_emit_op_data(&expr_node TSRMLS_CC);
			return;
		case ZEND_AST_LIST:
			zend_compile_list_assign(result, var_ast, &expr_node TSRMLS_CC);
			return;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
}

zend_op *zend_compile_assign_ref_common(
	znode *result, zend_ast *target_ast, znode *source_node TSRMLS_DC
) {
	znode target_node;
	zend_op *opline;

	if (is_this_fetch(target_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	zend_ensure_writable_variable(target_ast);
	zend_compile_var(&target_node, target_ast, BP_VAR_W TSRMLS_CC);

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_ASSIGN_REF;
	SET_NODE(opline->op1, &target_node);
	SET_NODE(opline->op2, source_node);

	if (result) {
		opline->result_type = IS_VAR;
		opline->result.var = get_temporary_variable(CG(active_op_array));
		GET_NODE(result, opline->result);
	} else {
		opline->result_type = IS_UNUSED | EXT_TYPE_UNUSED;
	}

	return opline;
}

void zend_compile_assign_ref(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *target_ast = ast->child[0];
	zend_ast *source_ast = ast->child[1];

	znode source_node;
	zend_op *opline;

	zend_compile_var(&source_node, source_ast, BP_VAR_REF TSRMLS_CC);
	opline = zend_compile_assign_ref_common(result, target_ast, &source_node TSRMLS_CC);

	if (zend_is_call(source_ast)) {
		opline->extended_value = ZEND_RETURNS_FUNCTION;
	} else if (source_ast->kind == ZEND_AST_NEW) {
		opline->extended_value = ZEND_RETURNS_NEW;
	}
}

void zend_compile_compound_assign(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];
	zend_ast *expr_ast = ast->child[1];
	zend_uint opcode = ast->attr;

	znode var_node, expr_node;
	zend_op *opline;

	zend_ensure_writable_variable(var_ast);
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	switch (var_ast->kind) {
		case ZEND_AST_VAR:
		case ZEND_AST_STATIC_PROP:
			zend_compile_var(&var_node, var_ast, BP_VAR_RW TSRMLS_CC);
			emit_op(result, opcode, &var_node, &expr_node TSRMLS_CC);
			return;
		case ZEND_AST_DIM:
			opline = zend_compile_dim_common(result, var_ast, BP_VAR_RW TSRMLS_CC);
			opline->opcode = opcode;
			opline->extended_value = ZEND_ASSIGN_DIM;

			opline = zend_emit_op_data(&expr_node TSRMLS_CC);
			opline->op2.var = get_temporary_variable(CG(active_op_array));
			opline->op2_type = IS_VAR;
			return;
		case ZEND_AST_PROP:
			opline = zend_compile_prop_common(result, var_ast, BP_VAR_RW TSRMLS_CC);
			opline->opcode = opcode;
			opline->extended_value = ZEND_ASSIGN_OBJ;

			opline = zend_emit_op_data(&expr_node TSRMLS_CC);
			return;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

zend_uint zend_compile_args(zend_ast *ast, zend_function *fbc TSRMLS_DC) {
	// TODO.AST &var error
	zend_ast_list *args = zend_ast_get_list(ast);
	zend_uint i;
	zend_bool uses_arg_unpack = 0;
	zend_uint arg_count = 0; /* number of arguments not including unpacks */

	for (i = 0; i < args->children; ++i) {
		zend_ast *arg = args->child[i];
		zend_uint arg_num = i + 1;

		znode arg_node;
		zend_op *opline;
		zend_uchar opcode;
		zend_ulong flags = 0;

		if (arg->kind == ZEND_AST_UNPACK) {
			uses_arg_unpack = 1;
			fbc = NULL;
			
			zend_compile_expr(&arg_node, arg->child[0] TSRMLS_CC);
			opline = emit_op(NULL, ZEND_SEND_UNPACK, &arg_node, NULL TSRMLS_CC);
			opline->op2.num = arg_count;
			continue;
		}

		if (uses_arg_unpack) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use positional argument after argument unpacking");
		}

		arg_count++;
		if (zend_is_variable(arg)) {
			if (zend_is_call(arg)) {
				zend_compile_var(&arg_node, arg, BP_VAR_R TSRMLS_CC);
				opcode = ZEND_SEND_VAR_NO_REF;
				flags |= ZEND_ARG_SEND_FUNCTION;
				if (fbc && ARG_SHOULD_BE_SENT_BY_REF(fbc, arg_num)) {
					flags |= ZEND_ARG_SEND_BY_REF;
					if (ARG_MAY_BE_SENT_BY_REF(fbc, arg_num)) {
						flags |= ZEND_ARG_SEND_SILENT;
					}
				}
			} else if (fbc) {
				if (ARG_SHOULD_BE_SENT_BY_REF(fbc, arg_num)) {
					zend_compile_var(&arg_node, arg, BP_VAR_W TSRMLS_CC);
					opcode = ZEND_SEND_REF;
				} else {
					zend_compile_var(&arg_node, arg, BP_VAR_R TSRMLS_CC);
					opcode = ZEND_SEND_VAR;
				}
			} else {
				zend_compile_var(&arg_node, arg,
					BP_VAR_FUNC_ARG | (arg_num << BP_VAR_SHIFT) TSRMLS_CC);
				opcode = ZEND_SEND_VAR_EX;
			}
		} else {
			zend_compile_expr(&arg_node, arg TSRMLS_CC);
			if (arg_node.op_type & (IS_VAR|IS_CV)) {
				opcode = ZEND_SEND_VAR_NO_REF;
				if (fbc && ARG_SHOULD_BE_SENT_BY_REF(fbc, arg_num)) {
					flags |= ZEND_ARG_SEND_BY_REF;
				}
			} else {
				if (fbc) {
					opcode = ZEND_SEND_VAL;
					if (ARG_MUST_BE_SENT_BY_REF(fbc, arg_num)) {
						zend_error_noreturn(E_COMPILE_ERROR, "Only variables can be passed by reference");
					}
				} else {
					opcode = ZEND_SEND_VAL_EX;
				}
			}
		}

		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = opcode;
		SET_NODE(opline->op1, &arg_node);
		SET_UNUSED(opline->op2);
		opline->op2.opline_num = arg_num;

		if (opcode == ZEND_SEND_VAR_NO_REF) {
			if (fbc) {
				flags |= ZEND_ARG_COMPILE_TIME_BOUND;
			}
			opline->extended_value = flags;
		} else if (fbc) {
			opline->extended_value = ZEND_ARG_COMPILE_TIME_BOUND;
		}
	}

	return arg_count;
}

void zend_compile_call_common(znode *result, zend_ast *args_ast, zend_function *fbc TSRMLS_DC) {
	zend_op *opline;
	zend_uint opnum_init = get_next_op_number(CG(active_op_array)) - 1;
	zend_uint arg_count;
	zend_uint call_flags;

	zend_do_extended_fcall_begin(TSRMLS_C);

	arg_count = zend_compile_args(args_ast, fbc TSRMLS_CC);

	opline = &CG(active_op_array)->opcodes[opnum_init];
	opline->extended_value = arg_count;

	call_flags = (opline->opcode == ZEND_NEW ? ZEND_CALL_CTOR : 0);
	opline = emit_op(result, ZEND_DO_FCALL, NULL, NULL TSRMLS_CC);
	opline->op1.num = call_flags;

	zend_do_extended_fcall_end(TSRMLS_C);
}

zend_bool zend_compile_function_name(znode *name_node, zend_ast *name_ast TSRMLS_DC) {
	zend_string *orig_name = zend_ast_get_str(name_ast);
	zend_bool is_fully_qualified;

	name_node->op_type = IS_CONST;
	ZVAL_STR(&name_node->u.constant, zend_resolve_function_name(
		orig_name, name_ast->attr, &is_fully_qualified TSRMLS_CC));

	return !is_fully_qualified && Z_TYPE(CG(current_namespace)) != IS_UNDEF;
}

void zend_compile_ns_call(znode *result, znode *name_node, zend_ast *args_ast TSRMLS_DC) {
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INIT_NS_FCALL_BY_NAME;
	SET_UNUSED(opline->op1);
	opline->op2_type = IS_CONST;
	opline->op2.constant = zend_add_ns_func_name_literal(
		CG(active_op_array), &name_node->u.constant TSRMLS_CC);
	GET_CACHE_SLOT(opline->op2.constant);

	zend_compile_call_common(result, args_ast, NULL TSRMLS_CC);
}

void zend_compile_dynamic_call(znode *result, znode *name_node, zend_ast *args_ast TSRMLS_DC) {
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INIT_FCALL_BY_NAME;
	SET_UNUSED(opline->op1);
	if (name_node->op_type == IS_CONST && Z_TYPE(name_node->u.constant) == IS_STRING) {
		opline->op2_type = IS_CONST;
		opline->op2.constant
			= zend_add_func_name_literal(CG(active_op_array), &name_node->u.constant TSRMLS_CC);
		GET_CACHE_SLOT(opline->op2.constant);
	} else {
		SET_NODE(opline->op2, name_node);
	}

	zend_compile_call_common(result, args_ast, NULL TSRMLS_CC);
}

static zend_bool zend_args_contain_unpack(zend_ast_list *args) {
	zend_uint i;
	for (i = 0; i < args->children; ++i) {
		if (args->child[i]->kind == ZEND_AST_UNPACK) {
			return 1;
		}
	}
	return 0;
}

int zend_compile_func_strlen(znode *result, zend_ast_list *args TSRMLS_DC) {
	znode arg_node;

	if ((CG(compiler_options) & ZEND_COMPILE_NO_BUILTIN_STRLEN)
		|| args->children != 1 || args->child[0]->kind == ZEND_AST_UNPACK
	) {
		return FAILURE;
	}

	zend_compile_expr(&arg_node, args->child[0] TSRMLS_CC);
	emit_op(result, ZEND_STRLEN, &arg_node, NULL TSRMLS_CC);
	return SUCCESS;
}

int zend_compile_func_typecheck(znode *result, zend_ast_list *args, zend_uint type TSRMLS_DC) {
	znode arg_node;
	zend_op *opline;

	if (args->children != 1 || args->child[0]->kind == ZEND_AST_UNPACK) {
		return FAILURE;
	}
	
	zend_compile_expr(&arg_node, args->child[0] TSRMLS_CC);
	opline = emit_op(result, ZEND_TYPE_CHECK, &arg_node, NULL TSRMLS_CC);
	opline->extended_value = type;
	return SUCCESS;
}

int zend_compile_func_defined(znode *result, zend_ast_list *args TSRMLS_DC) {
	zend_string *name;
	zend_op *opline;

	if (args->children != 1 || args->child[0]->kind != ZEND_AST_ZVAL) {
		return FAILURE;
	}

	name = zval_get_string(zend_ast_get_zval(args->child[0]));
	if (zend_memrchr(name->val, '\\', name->len) || zend_memrchr(name->val, ':', name->len)) {
		STR_RELEASE(name);
		return FAILURE;
	}

	opline = emit_op(result, ZEND_DEFINED, NULL, NULL TSRMLS_CC);
	opline->op1_type = IS_CONST;
	LITERAL_STR(opline->op1, name);
	GET_CACHE_SLOT(opline->op1.constant);

	/* Lowercase constant name in a separate literal */
	{
		zval c;
		zend_string *lcname = STR_ALLOC(name->len, 0);
		zend_str_tolower_copy(lcname->val, name->val, name->len);
		ZVAL_NEW_STR(&c, lcname);
		zend_add_literal(CG(active_op_array), &c TSRMLS_CC);
	}
	return SUCCESS;
}

static int zend_try_compile_ct_bound_init_user_func(
	znode *result, zend_ast *name_ast, zend_uint num_args TSRMLS_DC
) {
	zend_string *name, *lcname;
	zend_function *fbc;
	zend_op *opline;

	if (name_ast->kind != ZEND_AST_CONST || Z_TYPE_P(zend_ast_get_zval(name_ast)) != IS_STRING) {
		return FAILURE;
	}

	name = zend_ast_get_str(name_ast);
	lcname = STR_ALLOC(name->len, 0);
	zend_str_tolower_copy(lcname->val, name->val, name->len);

	fbc = zend_hash_find_ptr(CG(function_table), lcname);
	if (!fbc || (fbc->type == ZEND_INTERNAL_FUNCTION &&
		(CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS))
	) {
		STR_FREE(lcname);
		return FAILURE;
	}

	opline = emit_op(NULL, ZEND_INIT_FCALL, NULL, NULL TSRMLS_CC);
	opline->op2_type = IS_CONST;
	LITERAL_STR(opline->op2, lcname);
	opline->extended_value = num_args;

	return SUCCESS;
}

static void zend_compile_init_user_func(
	znode *result, zend_ast *name_ast, zend_uint num_args, zend_string *orig_func_name TSRMLS_DC
) {
	zend_op *opline;
	znode name_node;

	if (zend_try_compile_ct_bound_init_user_func(result, name_ast, num_args TSRMLS_CC) == SUCCESS) {
		return;
	}

	zend_compile_expr(&name_node, name_ast TSRMLS_CC);

	opline = emit_op(NULL, ZEND_INIT_USER_CALL, NULL, &name_node TSRMLS_CC);
	opline->op1_type = IS_CONST;
	LITERAL_STR(opline->op1, STR_COPY(orig_func_name));
	opline->extended_value = num_args;
}

/* cufa = call_user_func_array */
int zend_compile_func_cufa(znode *result, zend_ast_list *args, zend_string *lcname TSRMLS_DC) {
	znode arg_node;

	if (args->children != 2 || zend_args_contain_unpack(args)) {
		return FAILURE;
	}

	zend_compile_init_user_func(NULL, args->child[0], 1, lcname TSRMLS_CC);
	zend_compile_expr(&arg_node, args->child[1] TSRMLS_CC);
	emit_op(NULL, ZEND_SEND_ARRAY, &arg_node, NULL TSRMLS_CC);
	emit_op(result, ZEND_DO_FCALL, NULL, NULL TSRMLS_CC);

	return SUCCESS;
}

/* cuf = call_user_func */
int zend_compile_func_cuf(znode *result, zend_ast_list *args, zend_string *lcname TSRMLS_DC) {
	zend_uint i;

	if (args->children < 1 || zend_args_contain_unpack(args)) {
		return FAILURE;
	}

	zend_compile_init_user_func(NULL, args->child[0], args->children - 1, lcname TSRMLS_CC);
	for (i = 1; i < args->children; ++i) {
		zend_ast *arg_ast = args->child[i];
		znode arg_node;
		zend_op *opline;
		zend_bool send_user = 0;

		if (zend_is_variable(arg_ast) && !zend_is_call(arg_ast)) {
			zend_compile_var(&arg_node, arg_ast, BP_VAR_FUNC_ARG | (i << BP_VAR_SHIFT) TSRMLS_CC);
			send_user = 1;
		} else {
			zend_compile_expr(&arg_node, arg_ast TSRMLS_CC);
			if (arg_node.op_type & (IS_VAR|IS_CV)) {
				send_user = 1;
			}
		}

		if (send_user) {
			opline = emit_op(NULL, ZEND_SEND_USER, &arg_node, NULL TSRMLS_CC);
		} else {
			opline = emit_op(NULL, ZEND_SEND_VAL, &arg_node, NULL TSRMLS_CC);
		}

		opline->op2.opline_num = i;
	}
	emit_op(result, ZEND_DO_FCALL, NULL, NULL TSRMLS_CC);

	return SUCCESS;
}

int zend_try_compile_special_func(
	znode *result, zend_string *lcname, zend_ast_list *args TSRMLS_DC
) {
	if (zend_str_equals(lcname, "strlen")) {
		return zend_compile_func_strlen(result, args TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_null")) {
		return zend_compile_func_typecheck(result, args, IS_NULL TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_bool")) {
		return zend_compile_func_typecheck(result, args, _IS_BOOL TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_long")) {
		return zend_compile_func_typecheck(result, args, IS_LONG TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_float")) {
		return zend_compile_func_typecheck(result, args, IS_DOUBLE TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_string")) {
		return zend_compile_func_typecheck(result, args, IS_STRING TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_array")) {
		return zend_compile_func_typecheck(result, args, IS_ARRAY TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_object")) {
		return zend_compile_func_typecheck(result, args, IS_OBJECT TSRMLS_CC);
	} else if (zend_str_equals(lcname, "is_resource")) {
		return zend_compile_func_typecheck(result, args, IS_RESOURCE TSRMLS_CC);
	} else if (zend_str_equals(lcname, "defined")) {
		return zend_compile_func_defined(result, args TSRMLS_CC);
	} else if (zend_str_equals(lcname, "call_user_func_array")) {
		return zend_compile_func_cufa(result, args, lcname TSRMLS_CC);
	} else if (zend_str_equals(lcname, "call_user_func")) {
		return zend_compile_func_cuf(result, args, lcname TSRMLS_CC);
	} else {
		return FAILURE;
	}
}

void zend_compile_call(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	zend_ast *args_ast = ast->child[1];

	znode name_node;

	if (name_ast->kind != ZEND_AST_ZVAL || Z_TYPE_P(zend_ast_get_zval(name_ast)) != IS_STRING) {
		zend_compile_expr(&name_node, name_ast TSRMLS_CC);
		zend_compile_dynamic_call(result, &name_node, args_ast TSRMLS_CC);
		return;
	}

	{
		zend_bool needs_runtime_resolution = zend_compile_function_name(&name_node, name_ast TSRMLS_CC);
		if (needs_runtime_resolution) {
			zend_compile_ns_call(result, &name_node, args_ast TSRMLS_CC);
			return;
		}
	}

	{
		zval *name = &name_node.u.constant;
		zend_string *lcname = STR_ALLOC(Z_STRLEN_P(name), 0);
		zend_function *fbc;
		zend_op *opline;

		zend_str_tolower_copy(lcname->val, Z_STRVAL_P(name), Z_STRLEN_P(name));

		fbc = zend_hash_find_ptr(CG(function_table), lcname);
		if (!fbc || (fbc->type == ZEND_INTERNAL_FUNCTION &&
			(CG(compiler_options) & ZEND_COMPILE_IGNORE_INTERNAL_FUNCTIONS))
		) {
			STR_RELEASE(lcname);
			zend_compile_dynamic_call(result, &name_node, args_ast TSRMLS_CC);
			return;
		}

		if (zend_try_compile_special_func(result, lcname,
				zend_ast_get_list(args_ast) TSRMLS_CC) == SUCCESS
		) {
			STR_RELEASE(lcname);
			zval_ptr_dtor(&name_node.u.constant);
			return;
		}

		zval_ptr_dtor(&name_node.u.constant);
		ZVAL_NEW_STR(&name_node.u.constant, lcname);

		opline = emit_op(NULL, ZEND_INIT_FCALL, NULL, &name_node TSRMLS_CC);
		GET_CACHE_SLOT(opline->op2.constant);

		zend_compile_call_common(result, args_ast, fbc TSRMLS_CC);
	}
}

void zend_compile_method_call(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	//zend_ast *obj_ast = ast->child[0];
	//zend_ast *method_ast = ast->child[1];
	zend_ast *args_ast = ast->child[2];

	// TODO.AST __clone check - WTF is that done in here?!

	// TODO.AST using an evil overload from AST_METHOD to AST_PROP here ...
	zend_op *opline = zend_compile_prop_common(NULL, ast, BP_VAR_R TSRMLS_CC);
	opline->opcode = ZEND_INIT_METHOD_CALL;

	if (opline->op2_type == IS_CONST) {
		zval name;
		name = CONSTANT(opline->op2.constant);
		if (Z_TYPE(name) != IS_STRING) {
			zend_error_noreturn(E_COMPILE_ERROR, "Method name must be a string");
		}
		Z_STR(name) = STR_COPY(Z_STR(name));
		FREE_POLYMORPHIC_CACHE_SLOT(opline->op2.constant);
		opline->op2.constant =
			zend_add_func_name_literal(CG(active_op_array), &name TSRMLS_CC);
		GET_POLYMORPHIC_CACHE_SLOT(opline->op2.constant);
	}

	zend_compile_call_common(result, args_ast, NULL TSRMLS_CC);
}

zend_bool zend_is_constructor(zend_string *name) {
	char *lcname = zend_str_tolower_dup(name->val, name->len);
	zend_bool is_ctor = name->len == sizeof(ZEND_CONSTRUCTOR_FUNC_NAME) - 1 &&
		!memcmp(lcname, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME) - 1);
	efree(lcname);
	return is_ctor;
}

void zend_compile_static_call(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	zend_ast *class_ast = ast->child[0];
	zend_ast *method_ast = ast->child[1];
	zend_ast *args_ast = ast->child[2];

	znode class_node, method_node;
	zend_op *opline;
	zend_ulong extended_value = 0;

	if (zend_is_const_default_class_ref(class_ast)) {
		class_node.op_type = IS_CONST;
		ZVAL_STR(&class_node.u.constant, zend_resolve_class_name_ast(class_ast TSRMLS_CC));
	} else {
		opline = zend_compile_class_ref(&class_node, class_ast TSRMLS_CC);
		extended_value = opline->extended_value;
	}

	zend_compile_expr(&method_node, method_ast TSRMLS_CC);
	if (method_node.op_type == IS_CONST) {
		zval *name = &method_node.u.constant;
		if (Z_TYPE_P(name) != IS_STRING) {
			zend_error_noreturn(E_COMPILE_ERROR, "Method name must be a string");
		}
		if (zend_is_constructor(Z_STR_P(name))) {
			zval_ptr_dtor(name);
			method_node.op_type = IS_UNUSED;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INIT_STATIC_METHOD_CALL;
	opline->extended_value = extended_value;

	zend_set_class_name_op1(opline, &class_node TSRMLS_CC);

	if (method_node.op_type == IS_CONST) {
		opline->op2_type = IS_CONST;
		opline->op2.constant =
			zend_add_func_name_literal(CG(active_op_array), &method_node.u.constant TSRMLS_CC);
		if (opline->op1_type == IS_CONST) {
			GET_CACHE_SLOT(opline->op2.constant);
		} else {
			GET_POLYMORPHIC_CACHE_SLOT(opline->op2.constant);
		}
	} else {
		SET_NODE(opline->op2, &method_node);
	}

	zend_compile_call_common(result, args_ast, NULL TSRMLS_CC);
}

void zend_compile_new(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *class_ast = ast->child[0];
	zend_ast *args_ast = ast->child[1];

	znode class_node, ctor_result;
	zend_op *opline;
	zend_uint opnum;

	zend_compile_class_ref(&class_node, class_ast TSRMLS_CC);

	opnum = get_next_op_number(CG(active_op_array));
	opline = emit_op(result, ZEND_NEW, &class_node, NULL TSRMLS_CC);

	zend_compile_call_common(&ctor_result, args_ast, NULL TSRMLS_CC);
	zend_do_free(&ctor_result TSRMLS_CC);

	/* New jumps over ctor call if ctor does not exist */
	opline = &CG(active_op_array)->opcodes[opnum];
	opline->op2.opline_num = get_next_op_number(CG(active_op_array));
}

void zend_compile_clone(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *obj_ast = ast->child[0];

	znode obj_node;
	zend_compile_expr(&obj_node, obj_ast TSRMLS_CC);

	emit_op(result, ZEND_CLONE, &obj_node, NULL TSRMLS_CC);
}

void zend_compile_global_var(zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];
	zend_ast *name_ast = var_ast->child[0];

	znode name_node, result;

	zend_compile_expr(&name_node, name_ast TSRMLS_CC);
	if (name_node.op_type == IS_CONST) {
		if (Z_TYPE(name_node.u.constant) != IS_STRING) {
			convert_to_string(&name_node.u.constant);
		}
	}

	if (zend_try_compile_cv(&result, var_ast TSRMLS_CC) == SUCCESS) {
		emit_op(NULL, ZEND_BIND_GLOBAL, &result, &name_node TSRMLS_CC);
	} else {
		emit_op(&result, ZEND_FETCH_W, &name_node, NULL TSRMLS_CC);

		// TODO.AST Avoid double fetch
		//opline->extended_value = ZEND_FETCH_GLOBAL_LOCK;

		zend_compile_assign_ref_common(NULL, var_ast, &result TSRMLS_CC);
	}
}

static void zend_compile_static_var_common(
	zend_ast *var_ast, zval *value, zend_bool by_ref TSRMLS_DC
) {
	znode var_node, result;
	zend_op *opline;

	zend_compile_expr(&var_node, var_ast TSRMLS_CC);

	if (!CG(active_op_array)->static_variables) {
		if (CG(active_op_array)->scope) {
			CG(active_op_array)->scope->ce_flags |= ZEND_HAS_STATIC_IN_METHODS;
		}
		ALLOC_HASHTABLE(CG(active_op_array)->static_variables);
		zend_hash_init(CG(active_op_array)->static_variables, 8, NULL, ZVAL_PTR_DTOR, 0);
	}

	zend_hash_update(CG(active_op_array)->static_variables, Z_STR(var_node.u.constant), value);

	opline = emit_op(&result, by_ref ? ZEND_FETCH_W : ZEND_FETCH_R, &var_node, NULL TSRMLS_CC);
	opline->extended_value = ZEND_FETCH_STATIC;

	if (by_ref) {
		zend_ast *fetch_ast = zend_ast_create_unary(ZEND_AST_VAR, var_ast);
		zend_compile_assign_ref_common(NULL, fetch_ast, &result TSRMLS_CC);
	} else {
		zend_ast *fetch_ast = zend_ast_create_unary(ZEND_AST_VAR, var_ast);
		zend_ast *znode_ast = zend_ast_create_znode(&result);
		zend_ast *assign_ast = zend_ast_create_binary(ZEND_AST_ASSIGN, fetch_ast, znode_ast);
		znode dummy_node;
		zend_compile_expr(&dummy_node, assign_ast TSRMLS_CC);
		zend_do_free(&dummy_node TSRMLS_CC);
	}
}

void zend_compile_static_var(zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];
	zend_ast *value_ast = ast->child[1];
	zval value_zv;

	if (value_ast) {
		_tmp_compile_const_expr(&value_zv, value_ast TSRMLS_CC);
	} else {
		ZVAL_NULL(&value_zv);
	}

	zend_compile_static_var_common(var_ast, &value_zv, 1 TSRMLS_CC);
}

void zend_compile_unset(zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];

	znode var_node;
	zend_op *opline;
	switch (var_ast->kind) {
		case ZEND_AST_VAR:
			if (zend_try_compile_cv(&var_node, var_ast TSRMLS_CC) == SUCCESS) {
				opline = emit_op(NULL, ZEND_UNSET_VAR, &var_node, NULL TSRMLS_CC);
				opline->extended_value = ZEND_FETCH_LOCAL | ZEND_QUICK_SET;
			} else {
				opline = zend_compile_simple_var_no_cv(NULL, var_ast, BP_VAR_UNSET TSRMLS_CC);
				opline->opcode = ZEND_UNSET_VAR;
			}
			return;
		case ZEND_AST_DIM:
			opline = zend_compile_dim_common(NULL, var_ast, BP_VAR_UNSET TSRMLS_CC);
			opline->opcode = ZEND_UNSET_DIM;
			return;
		case ZEND_AST_PROP:
			opline = zend_compile_prop_common(NULL, var_ast, BP_VAR_UNSET TSRMLS_CC);
			opline->opcode = ZEND_UNSET_OBJ;
			return;
		case ZEND_AST_STATIC_PROP:
			opline = zend_compile_static_prop_common(NULL, var_ast, BP_VAR_UNSET TSRMLS_CC);
			opline->opcode = ZEND_UNSET_VAR;
			return;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static void zend_free_foreach_and_switch_variables(TSRMLS_D) {
	zend_uint opnum_start, opnum_end, i;

	opnum_start = get_next_op_number(CG(active_op_array));

#ifdef ZTS
	zend_stack_apply_with_argument(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_switch_expr TSRMLS_CC);
	zend_stack_apply_with_argument(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_foreach_copy TSRMLS_CC);
#else
	zend_stack_apply(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_switch_expr);
	zend_stack_apply(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_foreach_copy);
#endif

	opnum_end = get_next_op_number(CG(active_op_array));

	for (i = opnum_start; i < opnum_end; ++i) {
		CG(active_op_array)->opcodes[i].extended_value |= EXT_TYPE_FREE_ON_RETURN;
	}
}

void zend_compile_return(zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	zend_bool by_ref = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;

	znode expr_node;
	zend_op *opline;

	if (!expr_ast) {
		expr_node.op_type = IS_CONST;
		ZVAL_NULL(&expr_node.u.constant);
	} else if (by_ref && zend_is_variable(expr_ast) && !zend_is_call(expr_ast)) {
		zend_compile_var(&expr_node, expr_ast, BP_VAR_REF TSRMLS_CC);
	} else {
		zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);
	}

	zend_free_foreach_and_switch_variables(TSRMLS_C);

	if (CG(context).in_finally) {
		emit_op(NULL, ZEND_DISCARD_EXCEPTION, NULL, NULL TSRMLS_CC);
	}

	opline = emit_op(NULL, by_ref ? ZEND_RETURN_BY_REF : ZEND_RETURN, &expr_node, NULL TSRMLS_CC);
	if (expr_ast) {
		if (zend_is_call(expr_ast)) {
			opline->extended_value = ZEND_RETURNS_FUNCTION;
		} else if (!zend_is_variable(expr_ast)) {
			opline->extended_value = ZEND_RETURNS_VALUE;
		}
	}
}

void zend_compile_echo(zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];

	znode expr_node;
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	emit_op(NULL, ZEND_ECHO, &expr_node, NULL TSRMLS_CC);
}

void zend_compile_throw(zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];

	znode expr_node;
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	emit_op(NULL, ZEND_THROW, &expr_node, NULL TSRMLS_CC);
}

void zend_compile_break_continue(zend_ast *ast TSRMLS_DC) {
	zend_ast *depth_ast = ast->child[0];

	znode depth_node;
	zend_op *opline;

	ZEND_ASSERT(ast->kind == ZEND_AST_BREAK || ast->kind == ZEND_AST_CONTINUE);

	if (depth_ast) {
		if (depth_ast->kind != ZEND_AST_ZVAL) {
			zend_error_noreturn(E_COMPILE_ERROR, "'%s' operator with non-constant operand "
				"is no longer supported", ast->kind == ZEND_AST_BREAK ? "break" : "continue");
		}

		zend_compile_expr(&depth_node, depth_ast TSRMLS_CC);

		if (Z_TYPE(depth_node.u.constant) != IS_LONG || Z_LVAL(depth_node.u.constant) < 1) {
			zend_error_noreturn(E_COMPILE_ERROR, "'%s' operator accepts only positive numbers",
				ast->kind == ZEND_AST_BREAK ? "break" : "continue");
		}
	} else {
		depth_node.op_type = IS_CONST;
		ZVAL_LONG(&depth_node.u.constant, 1);
	}

	opline = emit_op(NULL, ast->kind == ZEND_AST_BREAK ? ZEND_BRK : ZEND_CONT,
		NULL, &depth_node TSRMLS_CC);
	opline->op1.opline_num = CG(context).current_brk_cont;
}

void zend_compile_goto(zend_ast *ast TSRMLS_DC) {
	zend_ast *label_ast = ast->child[0];
	znode label_node;
	zend_op *opline;

	zend_compile_expr(&label_node, label_ast TSRMLS_CC);
	opline = emit_op(NULL, ZEND_GOTO, NULL, &label_node TSRMLS_CC);
	opline->extended_value = CG(context).current_brk_cont;
	zend_resolve_goto_label(CG(active_op_array), opline, 0 TSRMLS_CC);
}

void zend_compile_label(zend_ast *ast TSRMLS_DC) {
	zval *label = zend_ast_get_zval(ast->child[0]);
	zend_label dest;

	ZEND_ASSERT(Z_TYPE_P(label) == IS_STRING);

	if (!CG(context).labels) {
		ALLOC_HASHTABLE(CG(context).labels);
		zend_hash_init(CG(context).labels, 8, NULL, ptr_dtor, 0);
	}

	dest.brk_cont = CG(context).current_brk_cont;
	dest.opline_num = get_next_op_number(CG(active_op_array));

	if (!zend_hash_add_mem(CG(context).labels, Z_STR_P(label), &dest, sizeof(zend_label))) {
		zend_error_noreturn(E_COMPILE_ERROR, "Label '%s' already defined", Z_STRVAL_P(label));
	}
}

void zend_compile_while(zend_ast *ast TSRMLS_DC) {
	zend_ast *cond_ast = ast->child[0];
	zend_ast *stmt_ast = ast->child[1];

	znode cond_node;
	zend_uint opnum_start, opnum_jmpz;

	opnum_start = get_next_op_number(CG(active_op_array));
	zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);

	opnum_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0 TSRMLS_CC);
	do_begin_loop(TSRMLS_C);

	zend_compile_stmt(stmt_ast TSRMLS_CC);

	zend_emit_jump(opnum_start TSRMLS_CC);

	zend_update_jump_target_to_next(opnum_jmpz TSRMLS_CC);

	do_end_loop(opnum_start, 0 TSRMLS_CC);
}

void zend_compile_do_while(zend_ast *ast TSRMLS_DC) {
	zend_ast *stmt_ast = ast->child[0];
	zend_ast *cond_ast = ast->child[1];

	znode cond_node;
	zend_uint opnum_start, opnum_cond;

	do_begin_loop(TSRMLS_C);

	opnum_start = get_next_op_number(CG(active_op_array));
	zend_compile_stmt(stmt_ast TSRMLS_CC);

	opnum_cond = get_next_op_number(CG(active_op_array));
	zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);

	zend_emit_cond_jump(ZEND_JMPNZ, &cond_node, opnum_start TSRMLS_CC);

	do_end_loop(opnum_cond, 0 TSRMLS_CC);
}

void zend_compile_expr_list(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list;
	zend_uint i;

	result->op_type = IS_CONST;
	ZVAL_TRUE(&result->u.constant);

	if (!ast) {
		return;
	}

	list = zend_ast_get_list(ast);
	for (i = 0; i < list->children; ++i) {
		zend_ast *expr_ast = list->child[i];

		zend_do_free(result TSRMLS_CC);
		zend_compile_expr(result, expr_ast TSRMLS_CC);
	}
}

void zend_compile_for(zend_ast *ast TSRMLS_DC) {
	zend_ast *init_ast = ast->child[0];
	zend_ast *cond_ast = ast->child[1];
	zend_ast *loop_ast = ast->child[2];
	zend_ast *stmt_ast = ast->child[3];

	znode result;
	zend_uint opnum_cond, opnum_jmpz, opnum_loop;

	zend_compile_expr_list(&result, init_ast TSRMLS_CC);
	zend_do_free(&result TSRMLS_CC);

	opnum_cond = get_next_op_number(CG(active_op_array));
	zend_compile_expr_list(&result, cond_ast TSRMLS_CC);
	zend_do_extended_info(TSRMLS_C);

	opnum_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &result, 0 TSRMLS_CC);
	do_begin_loop(TSRMLS_C);

	zend_compile_stmt(stmt_ast TSRMLS_CC);

	opnum_loop = get_next_op_number(CG(active_op_array));
	zend_compile_expr_list(&result, loop_ast TSRMLS_CC);
	zend_do_free(&result TSRMLS_CC);

	zend_emit_jump(opnum_cond TSRMLS_CC);

	zend_update_jump_target_to_next(opnum_jmpz TSRMLS_CC);

	do_end_loop(opnum_loop, 0 TSRMLS_CC);
}

void zend_compile_foreach(zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	zend_ast *value_ast = ast->child[1];
	zend_ast *key_ast = ast->child[2];
	zend_ast *stmt_ast = ast->child[3];
	zend_bool by_ref = value_ast->kind == ZEND_AST_REF;
	zend_bool is_variable = zend_is_variable(expr_ast) && !zend_is_call(expr_ast)
		&& zend_can_write_to_variable(expr_ast);

	znode expr_node, reset_node, value_node, key_node, dummy_node;
	zend_op *opline;
	zend_uint opnum_reset, opnum_fetch;
	zend_op foreach_stack_opline;

	if (key_ast) {
		if (key_ast->kind == ZEND_AST_REF) {
			zend_error_noreturn(E_COMPILE_ERROR, "Key element cannot be a reference");
		}
		if (key_ast->kind == ZEND_AST_LIST) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use list as key element");
		}
	}

	if (by_ref) {
		value_ast = value_ast->child[0];
	}

	if (by_ref && is_variable) {
		zend_compile_var(&expr_node, expr_ast, BP_VAR_W TSRMLS_CC);
	} else {
		zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);
	}

	opnum_reset = get_next_op_number(CG(active_op_array));
	opline = emit_op(&reset_node, ZEND_FE_RESET, &expr_node, NULL TSRMLS_CC);
	if (by_ref && is_variable) {
		opline->extended_value = ZEND_FE_RESET_VARIABLE | ZEND_FE_RESET_REFERENCE; // ???
	}

	SET_NODE(foreach_stack_opline.result, &reset_node);
	zend_stack_push(&CG(foreach_copy_stack), &foreach_stack_opline);

	opnum_fetch = get_next_op_number(CG(active_op_array));
	opline = emit_op(&value_node, ZEND_FE_FETCH, &reset_node, NULL TSRMLS_CC);
	if (by_ref) {
		opline->extended_value |= ZEND_FE_FETCH_BYREF;
	}
	if (key_ast) {
		opline->extended_value |= ZEND_FE_FETCH_WITH_KEY;
	}

	opline = emit_op(NULL, ZEND_OP_DATA, NULL, NULL TSRMLS_CC);

	/* Allocate enough space to keep HashPointer on VM stack */
	opline->op1_type = IS_TMP_VAR;
	opline->op1.var = get_temporary_variable(CG(active_op_array));
	if (sizeof(HashPointer) > sizeof(zval)) {
		/* Make sure 1 zval is enough for HashPointer (2 must be enough) */
		get_temporary_variable(CG(active_op_array));
	}

	if (key_ast) {
		opline->result_type = IS_TMP_VAR;
		opline->result.var = get_temporary_variable(CG(active_op_array));
		GET_NODE(&key_node, opline->result);
	}

	if (value_ast->attr == ZEND_AST_LIST) {
		zend_compile_list_assign(&dummy_node, value_ast, &value_node TSRMLS_CC);
		zend_do_free(&dummy_node TSRMLS_CC);
	} else if (by_ref) {
		zend_compile_assign_ref_common(NULL, value_ast, &value_node TSRMLS_CC);
	} else {
		zend_ast *znode_ast = zend_ast_create_znode(&value_node);
		zend_ast *assign_ast = zend_ast_create_binary(ZEND_AST_ASSIGN, value_ast, znode_ast);
		zend_compile_expr(&dummy_node, assign_ast TSRMLS_CC);
		zend_do_free(&dummy_node TSRMLS_CC);
	}

	if (key_ast) {
		zend_ast *znode_ast = zend_ast_create_znode(&key_node);
		zend_ast *assign_ast = zend_ast_create_binary(ZEND_AST_ASSIGN, key_ast, znode_ast);
		zend_compile_expr(&dummy_node, assign_ast TSRMLS_CC);
		zend_do_free(&dummy_node TSRMLS_CC);
	}

	do_begin_loop(TSRMLS_C);

	zend_compile_stmt(stmt_ast TSRMLS_CC);

	zend_emit_jump(opnum_fetch TSRMLS_CC);

	opline = &CG(active_op_array)->opcodes[opnum_reset];
	opline->op2.opline_num = get_next_op_number(CG(active_op_array));

	opline = &CG(active_op_array)->opcodes[opnum_fetch];
	opline->op2.opline_num = get_next_op_number(CG(active_op_array));

	do_end_loop(opnum_fetch, 1 TSRMLS_CC);

	{
		zend_op *container_ptr = zend_stack_top(&CG(foreach_copy_stack));
		generate_free_foreach_copy(container_ptr TSRMLS_CC);
		zend_stack_del_top(&CG(foreach_copy_stack));
	}
}

void zend_compile_if(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	zend_uint *jmp_opnums = safe_emalloc(sizeof(zend_uint), list->children - 1, 0);
	zend_uint opnum_last_jmpz = 0;

	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *cond_ast = elem_ast->child[0];
		zend_ast *stmt_ast = elem_ast->child[1];

		znode cond_node;
		if (cond_ast) {
			zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);
			opnum_last_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0 TSRMLS_CC);
		}

		zend_compile_stmt(stmt_ast TSRMLS_CC);

		if (i != list->children - 1) {
			jmp_opnums[i] = zend_emit_jump(0 TSRMLS_CC);
		}

		if (cond_ast) {
			zend_update_jump_target_to_next(opnum_last_jmpz TSRMLS_CC);
		}
	}

	for (i = 0; i < list->children - 1; ++i) {
		zend_update_jump_target_to_next(jmp_opnums[i] TSRMLS_CC);
	}

	efree(jmp_opnums);
}

void zend_compile_switch(zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	zend_ast_list *cases = zend_ast_get_list(ast->child[1]);

	zend_uint i;
	zend_bool has_default_case = 0;
	zend_switch_entry switch_entry;

	znode expr_node, case_node;
	zend_op *opline;
	zend_uint *jmpnz_opnums = safe_emalloc(sizeof(zend_uint), cases->children, 0);
	zend_uint opnum_default_jmp;

	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	switch_entry.cond = expr_node;
	switch_entry.default_case = -1;
	switch_entry.control_var = -1;
	zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry);

	do_begin_loop(TSRMLS_C);

	case_node.op_type = IS_TMP_VAR;
	case_node.u.op.var = get_temporary_variable(CG(active_op_array));

	for (i = 0; i < cases->children; ++i) {
		zend_ast *case_ast = cases->child[i];
		zend_ast *cond_ast = case_ast->child[0];
		znode cond_node;

		if (!cond_ast) {
			has_default_case = 1;
			continue;
		}

		zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);

		opline = emit_op(NULL, ZEND_CASE, &expr_node, &cond_node TSRMLS_CC);
		SET_NODE(opline->result, &case_node);
		if (opline->op1_type == IS_CONST) {
			zval_copy_ctor(&CONSTANT(opline->op1.constant));
		}

		jmpnz_opnums[i] = zend_emit_cond_jump(ZEND_JMPNZ, &case_node, 0 TSRMLS_CC);
	}

	opnum_default_jmp = zend_emit_jump(0 TSRMLS_CC);

	for (i = 0; i < cases->children; ++i) {
		zend_ast *case_ast = cases->child[i];
		zend_ast *cond_ast = case_ast->child[0];
		zend_ast *stmt_ast = case_ast->child[1];

		if (cond_ast) {
			zend_update_jump_target_to_next(jmpnz_opnums[i] TSRMLS_CC);
		} else {
			zend_update_jump_target_to_next(opnum_default_jmp TSRMLS_CC);
		}

		zend_compile_stmt(stmt_ast TSRMLS_CC);
	}

	if (!has_default_case) {
		zend_update_jump_target_to_next(opnum_default_jmp TSRMLS_CC);
	}

	do_end_loop(get_next_op_number(CG(active_op_array)), 1 TSRMLS_CC);

	if (expr_node.op_type == IS_VAR || expr_node.op_type == IS_TMP_VAR) {
		emit_op(NULL, expr_node.op_type == IS_TMP_VAR ? ZEND_FREE : ZEND_SWITCH_FREE,
			&expr_node, NULL TSRMLS_CC);
	} else if (expr_node.op_type == IS_CONST) {
		zval_dtor(&expr_node.u.constant);
	}

	zend_stack_del_top(&CG(switch_cond_stack));
	efree(jmpnz_opnums);
}

void zend_compile_try(zend_ast *ast TSRMLS_DC) {
	zend_ast *try_ast = ast->child[0];
	zend_ast_list *catches = zend_ast_get_list(ast->child[1]);
	zend_ast *finally_ast = ast->child[2];

	zend_uint i;
	zend_op *opline;
	zend_uint try_catch_offset = zend_add_try_element(
		get_next_op_number(CG(active_op_array)) TSRMLS_CC);
	zend_uint *jmp_opnums = safe_emalloc(sizeof(zend_uint), catches->children, 0);

	if (catches->children == 0 && !finally_ast) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use try without catch or finally");
	}

	zend_compile_stmt(try_ast TSRMLS_CC);

	if (catches->children != 0) {
		jmp_opnums[0] = zend_emit_jump(0 TSRMLS_CC);
	}

	for (i = 0; i < catches->children; ++i) {
		zend_ast *catch_ast = catches->child[i];
		zend_ast *class_ast = catch_ast->child[0];
		zend_ast *var_ast = catch_ast->child[1];
		zend_ast *stmt_ast = catch_ast->child[2];
		zval *var_name = zend_ast_get_zval(var_ast);
		zend_bool is_last_catch = (i + 1 == catches->children);

		zend_uint opnum_catch;

		if (!zend_is_const_default_class_ref(class_ast)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Bad class name in the catch statement");
		}

		opnum_catch = get_next_op_number(CG(active_op_array));
		if (i == 0) {
			CG(active_op_array)->try_catch_array[try_catch_offset].catch_op = opnum_catch;
		}

		CG(zend_lineno) = catch_ast->lineno;

		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_CATCH;
		opline->op1_type = IS_CONST;
		opline->op1.constant = zend_add_class_name_literal(CG(active_op_array),
			zend_resolve_class_name_ast(class_ast TSRMLS_CC) TSRMLS_CC);

		opline->op2_type = IS_CV;
		opline->op2.var = lookup_cv(CG(active_op_array), STR_COPY(Z_STR_P(var_name)) TSRMLS_CC);
		opline->result.num = is_last_catch;

		zend_compile_stmt(stmt_ast TSRMLS_CC);

		if (!is_last_catch) {
			jmp_opnums[i + 1] = zend_emit_jump(0 TSRMLS_CC);
		}

		opline = &CG(active_op_array)->opcodes[opnum_catch];
		opline->extended_value = get_next_op_number(CG(active_op_array));
	}

	for (i = 0; i < catches->children; ++i) {
		zend_update_jump_target_to_next(jmp_opnums[i] TSRMLS_CC);
	}

	if (finally_ast) {
		zend_uint opnum_jmp = get_next_op_number(CG(active_op_array)) + 1;

		opline = emit_op(NULL, ZEND_FAST_CALL, NULL, NULL TSRMLS_CC);
		opline->op1.opline_num = opnum_jmp + 1;

		emit_op(NULL, ZEND_JMP, NULL, NULL TSRMLS_CC);

		CG(context).in_finally++;
		zend_compile_stmt(finally_ast TSRMLS_CC);
		CG(context).in_finally--;

		CG(active_op_array)->try_catch_array[try_catch_offset].finally_op = opnum_jmp + 1;
		CG(active_op_array)->try_catch_array[try_catch_offset].finally_end
			= get_next_op_number(CG(active_op_array));
		CG(active_op_array)->has_finally_block = 1;

		emit_op(NULL, ZEND_FAST_RET, NULL, NULL TSRMLS_CC);

		zend_update_jump_target_to_next(opnum_jmp TSRMLS_CC);
	}

	efree(jmp_opnums);
}

void zend_compile_declare(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *declares = zend_ast_get_list(ast->child[0]);
	zend_ast *stmt_ast = ast->child[1];
	zend_declarables orig_declarables = CG(declarables);
	zend_uint i;

	for (i = 0; i < declares->children; ++i) {
		zend_ast *declare_ast = declares->child[i];
		zend_ast *name_ast = declare_ast->child[0];
		zend_ast *value_ast = declare_ast->child[1];

		zend_string *name = zend_ast_get_str(name_ast);
		zval value_zv;

		_tmp_compile_const_expr(&value_zv, value_ast TSRMLS_CC);

		if (!zend_binary_strcasecmp(name->val, name->len, "ticks", sizeof("ticks") - 1)) {
			convert_to_long(&value_zv);
			ZVAL_COPY_VALUE(&CG(declarables).ticks, &value_zv);
		} else if (!zend_binary_strcasecmp(name->val, name->len, "encoding", sizeof("encoding") - 1)) {
			if (Z_TYPE(value_zv) == IS_CONSTANT) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot use constants as encoding");
			}

			/*
			 * Check that the pragma comes before any opcodes. If the compilation
			 * got as far as this, the previous portion of the script must have been
			 * parseable according to the .ini script_encoding setting. We still
			 * want to tell them to put declare() at the top.
			 */
			{
				zend_uint num = CG(active_op_array)->last;
				/* ignore ZEND_EXT_STMT and ZEND_TICKS */
				while (num > 0 &&
					   (CG(active_op_array)->opcodes[num-1].opcode == ZEND_EXT_STMT ||
						CG(active_op_array)->opcodes[num-1].opcode == ZEND_TICKS)) {
					--num;
				}

				if (num > 0) {
					zend_error_noreturn(E_COMPILE_ERROR, "Encoding declaration pragma must be "
						"the very first statement in the script");
				}
			}

			if (CG(multibyte)) {
				const zend_encoding *new_encoding, *old_encoding;
				zend_encoding_filter old_input_filter;

				CG(encoding_declared) = 1;

				convert_to_string(&value_zv);
				new_encoding = zend_multibyte_fetch_encoding(Z_STRVAL(value_zv) TSRMLS_CC);
				if (!new_encoding) {
					zend_error(E_COMPILE_WARNING, "Unsupported encoding [%s]", Z_STRVAL(value_zv));
				} else {
					old_input_filter = LANG_SCNG(input_filter);
					old_encoding = LANG_SCNG(script_encoding);
					zend_multibyte_set_filter(new_encoding TSRMLS_CC);

					/* need to re-scan if input filter changed */
					if (old_input_filter != LANG_SCNG(input_filter) ||
						 (old_input_filter && new_encoding != old_encoding)) {
						zend_multibyte_yyinput_again(old_input_filter, old_encoding TSRMLS_CC);
					}
				}
			} else {
				zend_error(E_COMPILE_WARNING, "declare(encoding=...) ignored because "
					"Zend multibyte feature is turned off by settings");
			}
			zval_dtor(&value_zv);
		} else {
			zend_error(E_COMPILE_WARNING, "Unsupported declare '%s'", name->val);
			zval_dtor(&value_zv);
		}
	}

	if (stmt_ast) {
		zend_compile_stmt(stmt_ast TSRMLS_CC);

		CG(declarables) = orig_declarables;
	}
}

void zend_compile_stmt_list(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	for (i = 0; i < list->children; ++i) {
		zend_compile_stmt(list->child[i] TSRMLS_CC);
	}
}

void zend_compile_params(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	zend_op_array *op_array = CG(active_op_array);
	zend_arg_info *arg_infos;

	if (list->children == 0) {
		return;
	}
	
	arg_infos = safe_emalloc(sizeof(zend_arg_info), list->children, 0);
	for (i = 0; i < list->children; ++i) {
		zend_ast *param_ast = list->child[i];
		zend_ast *type_ast = param_ast->child[0];
		zend_ast *var_ast = param_ast->child[1];
		zend_ast *default_ast = param_ast->child[2];
		zend_string *name = zend_ast_get_str(var_ast);
		zend_bool is_ref = (param_ast->attr & ZEND_PARAM_REF) != 0;
		zend_bool is_variadic = (param_ast->attr & ZEND_PARAM_VARIADIC) != 0;

		znode var_node, default_node;
		zend_uchar opcode;
		zend_op *opline;
		zend_arg_info *arg_info;

		if (zend_is_auto_global(name TSRMLS_CC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign auto-global variable %s",
				name->val);
		}

		var_node.op_type = IS_CV;
		var_node.u.op.var = lookup_cv(CG(active_op_array), STR_COPY(name) TSRMLS_CC);

		if (EX_VAR_TO_NUM(var_node.u.op.var) != i) {
			zend_error_noreturn(E_COMPILE_ERROR, "Redefinition of parameter %s",
				name->val);
		} else if (name->len == sizeof("this") - 1
		           && !memcmp(name->val, "this", sizeof("this") - 1)
		) {
			if (op_array->scope && (op_array->fn_flags & ZEND_ACC_STATIC) == 0) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
			}
			op_array->this_var = var_node.u.op.var;
		}

		if (op_array->fn_flags & ZEND_ACC_VARIADIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Only the last parameter can be variadic");
		}

		if (is_variadic) {
			opcode = ZEND_RECV_VARIADIC;
			default_node.op_type = IS_UNUSED;
			op_array->fn_flags |= ZEND_ACC_VARIADIC;

			if (default_ast) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Variadic parameter cannot have a default value");
			}
		} else if (default_ast) {
			opcode = ZEND_RECV_INIT;
			default_node.op_type = IS_CONST;
			_tmp_compile_const_expr(&default_node.u.constant, default_ast TSRMLS_CC);
		} else {
			opcode = ZEND_RECV;
			default_node.op_type = IS_UNUSED;
			op_array->required_num_args = i + 1;
		}

		opline = emit_op(NULL, opcode, NULL, &default_node TSRMLS_CC);
		SET_NODE(opline->result, &var_node);
		opline->op1.num = i + 1;

		arg_info = &arg_infos[i];
		arg_info->name = estrndup(name->val, name->len);
		arg_info->name_len = name->len;
		arg_info->pass_by_reference = is_ref;
		arg_info->is_variadic = is_variadic;
		arg_info->type_hint = 0;
		arg_info->allow_null = 1;
		arg_info->class_name = NULL;
		arg_info->class_name_len = 0;

		if (type_ast) {
			zend_bool has_null_default = default_ast
				&& (Z_TYPE(default_node.u.constant) == IS_NULL
					|| (Z_TYPE(default_node.u.constant) == IS_CONSTANT
						&& strcasecmp(Z_STRVAL(default_node.u.constant), "NULL"))
					|| Z_TYPE(default_node.u.constant) == IS_CONSTANT_AST); // ???

			op_array->fn_flags |= ZEND_ACC_HAS_TYPE_HINTS;
			arg_info->allow_null = has_null_default;

			if (type_ast->kind == ZEND_AST_TYPE) {
				arg_info->type_hint = type_ast->attr;
				if (arg_info->type_hint == IS_ARRAY) {
					if (default_ast && !has_null_default
						&& Z_TYPE(default_node.u.constant) != IS_ARRAY
					) {
						zend_error_noreturn(E_COMPILE_ERROR, "Default value for parameters "
							"with array type hint can only be an array or NULL");
					}
				} else if (arg_info->type_hint == IS_CALLABLE && default_ast) {
					if (default_ast && !has_null_default) {
						zend_error_noreturn(E_COMPILE_ERROR, "Default value for parameters "
							"with callable type hint can only be NULL");
					}
				}
			} else {
				zend_string *class_name = zend_ast_get_str(type_ast);

				if (zend_is_const_default_class_ref(type_ast)) {
					class_name = zend_resolve_class_name_ast(type_ast TSRMLS_CC);
				} else {
					STR_ADDREF(class_name);
				}

				arg_info->type_hint = IS_OBJECT;
				arg_info->class_name = estrndup(class_name->val, class_name->len);
				arg_info->class_name_len = class_name->len;

				STR_RELEASE(class_name);

				if (default_ast && !has_null_default) {
						zend_error_noreturn(E_COMPILE_ERROR, "Default value for parameters "
							"with a class type hint can only be NULL");
				}
			}
		}
	}

	/* These are assigned at the end to avoid unitialized memory in case of an error */
	op_array->num_args = list->children;
	op_array->arg_info = arg_infos;
}

void zend_compile_closure_uses(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;

	for (i = 0; i < list->children; ++i) {
		zend_ast *var_ast = list->child[i];
		zend_string *name = zend_ast_get_str(var_ast);
		zend_bool by_ref = var_ast->attr;
		zval zv;

		if (name->len == sizeof("this") - 1 && !memcmp(name->val, "this", sizeof("this") - 1)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use $this as lexical variable");
		}

		ZVAL_NULL(&zv);
		Z_CONST_FLAGS(zv) = by_ref ? IS_LEXICAL_REF : IS_LEXICAL_VAR;

		zend_compile_static_var_common(var_ast, &zv, by_ref TSRMLS_CC);
	}
}

void zend_begin_method_decl(
	zend_op_array *op_array, zend_string *name, zend_bool has_body TSRMLS_DC
) {
	zend_class_entry *ce = CG(active_class_entry);
	zend_bool in_interface = (ce->ce_flags & ZEND_ACC_INTERFACE) != 0;
	zend_bool in_trait = ZEND_CE_IS_TRAIT(ce);
	zend_bool is_public = (op_array->fn_flags & ZEND_ACC_PUBLIC) != 0;
	zend_bool is_static = (op_array->fn_flags & ZEND_ACC_STATIC) != 0;

	zend_string *lcname;

	if (in_interface) {
		if ((op_array->fn_flags & ZEND_ACC_PPP_MASK) != ZEND_ACC_PUBLIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Access type for interface method "
				"%s::%s() must be omitted", ce->name->val, name->val);
		}
		op_array->fn_flags |= ZEND_ACC_ABSTRACT;
	} else if (is_static && (op_array->fn_flags & ZEND_ACC_ABSTRACT)) {
		zend_error(E_STRICT, "Static function %s::%s() should not be abstract",
			ce->name->val, name->val);
	}

	if (op_array->fn_flags & ZEND_ACC_ABSTRACT) {
		//zend_op *opline;

		if (op_array->fn_flags & ZEND_ACC_PRIVATE) {
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot be declared private",
				in_interface ? "Interface" : "Abstract", ce->name->val, name->val);
		}

		if (has_body) {
			zend_error_noreturn(E_COMPILE_ERROR, "%s function %s::%s() cannot contain body",
				in_interface ? "Interface" : "Abstract", ce->name->val, name->val);
		}

		ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;

		/*opline = get_next_op(op_array TSRMLS_CC);
		opline->opcode = ZEND_RAISE_ABSTRACT_ERROR;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);*/
	} else if (!has_body) {
		zend_error_noreturn(E_COMPILE_ERROR, "Non-abstract method %s::%s() must contain body",
			ce->name->val, name->val);
	}

	op_array->scope = ce;
	op_array->function_name = STR_COPY(name);

	lcname = STR_ALLOC(name->len, 0);
	zend_str_tolower_copy(lcname->val, name->val, name->len);
	lcname = zend_new_interned_string(lcname TSRMLS_CC);

	if (zend_hash_add_ptr(&ce->function_table, lcname, op_array) == NULL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::%s()",
			ce->name->val, name->val);
	}

	if (in_interface) {
		if (zend_str_equals(lcname, ZEND_CALL_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __call() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_CALLSTATIC_FUNC_NAME)) {
			if (!is_public || !is_static) {
				zend_error(E_WARNING, "The magic method __callStatic() must have "
					"public visibility and be static");
			}
		} else if (zend_str_equals(lcname, ZEND_GET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __get() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_SET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __set() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_UNSET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __unset() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_ISSET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __isset() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_TOSTRING_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __toString() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_INVOKE_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __invoke() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_DEBUGINFO_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __debugInfo() must have "
					"public visibility and cannot be static");
			}
		}
	} else {
		ALLOCA_FLAG(use_heap);
		char *class_lcname = do_alloca(ce->name->len + 1, use_heap);
		zend_str_tolower_copy(class_lcname, ce->name->val, ce->name->len);

		if (!in_trait && lcname->len == ce->name->len
			&& !memcmp(class_lcname, lcname->val, lcname->len)
		) {
			if (!ce->constructor) {
				ce->constructor = (zend_function *) op_array;
			}
		} else if (zend_str_equals(lcname, ZEND_CONSTRUCTOR_FUNC_NAME)) {
			if (CG(active_class_entry)->constructor) {
				zend_error(E_STRICT, "Redefining already defined constructor for class %s",
					ce->name->val);
			}
			ce->constructor = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_DESTRUCTOR_FUNC_NAME)) {
			ce->destructor = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_CLONE_FUNC_NAME)) {
			ce->clone = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_CALL_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __call() must have "
					"public visibility and cannot be static");
			}
			ce->__call = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_CALLSTATIC_FUNC_NAME)) {
			if (!is_public || !is_static) {
				zend_error(E_WARNING, "The magic method __callStatic() must have "
					"public visibility and be static");
			}
			ce->__callstatic = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_GET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __get() must have "
					"public visibility and cannot be static");
			}
			ce->__get = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_SET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __set() must have "
					"public visibility and cannot be static");
			}
			ce->__set = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_UNSET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __unset() must have "
					"public visibility and cannot be static");
			}
			ce->__unset = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_ISSET_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __isset() must have "
					"public visibility and cannot be static");
			}
			ce->__isset = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_TOSTRING_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __toString() must have "
					"public visibility and cannot be static");
			}
			ce->__tostring = (zend_function *) op_array;
		} else if (zend_str_equals(lcname, ZEND_INVOKE_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __invoke() must have "
					"public visibility and cannot be static");
			}
		} else if (zend_str_equals(lcname, ZEND_DEBUGINFO_FUNC_NAME)) {
			if (!is_public || is_static) {
				zend_error(E_WARNING, "The magic method __debugInfo() must have "
					"public visibility and cannot be static");
			}
			ce->__debugInfo = (zend_function *) op_array;
		} else if (!is_static) {
			op_array->fn_flags |= ZEND_ACC_ALLOW_STATIC;
		}

		free_alloca(class_lcname, use_heap);
	}

	STR_RELEASE(lcname);
}

static void zend_begin_func_decl(
	znode *result, zend_op_array *op_array, zend_ast_decl *decl TSRMLS_DC
) {
	zend_ast *params_ast = decl->child[0];
	zend_string *name = decl->name, *lcname;
	zend_op *opline;

	op_array->function_name = name = zend_prefix_with_ns(name TSRMLS_CC);

	lcname = STR_ALLOC(name->len, 0);
	zend_str_tolower_copy(lcname->val, name->val, name->len);

	if (CG(current_import_function)) {
		zend_string *import_name = zend_hash_find_ptr(CG(current_import_function), lcname);
		if (import_name) {
			char *import_name_lc = zend_str_tolower_dup(import_name->val, import_name->len);

			if (import_name->len != name->len
				|| memcmp(import_name_lc, lcname->val, name->len)
			) {
				zend_error(E_COMPILE_ERROR, "Cannot declare function %s "
					"because the name is already in use", name->val);
			}

			efree(import_name_lc);
		}
	}

	if (zend_str_equals(lcname, ZEND_AUTOLOAD_FUNC_NAME)
		&& zend_ast_get_list(params_ast)->children != 1
	) {
		zend_error_noreturn(E_COMPILE_ERROR, "%s() must take exactly 1 argument",
			ZEND_AUTOLOAD_FUNC_NAME);
	}

	if (op_array->fn_flags & ZEND_ACC_CLOSURE) {
		opline = emit_op_tmp(result, ZEND_DECLARE_LAMBDA_FUNCTION, NULL, NULL TSRMLS_CC);
	} else {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_DECLARE_FUNCTION;
		opline->op2_type = IS_CONST;
		LITERAL_STR(opline->op2, STR_COPY(lcname));
	}

	{
		zval key;
		build_runtime_defined_function_key(&key, lcname, decl->lex_pos TSRMLS_CC);

		opline->op1_type = IS_CONST;
		opline->op1.constant = zend_add_literal(CG(active_op_array), &key TSRMLS_CC);

		zend_hash_update_ptr(CG(function_table), Z_STR(key), op_array);
	}

	STR_RELEASE(lcname);
}

void zend_compile_func_decl(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast_decl *decl = (zend_ast_decl *) ast;
	zend_ast *params_ast = decl->child[0];
	zend_ast *uses_ast = decl->child[1];
	zend_ast *stmt_ast = decl->child[2];
	zend_bool is_method = decl->kind == ZEND_AST_METHOD;

	zend_op_array *orig_op_array = CG(active_op_array);
	zend_op_array *op_array = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));

	// TODO.AST interactive (not just here - also bpc etc!)
	
	init_op_array(op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE TSRMLS_CC);

	op_array->fn_flags |= decl->flags;
	op_array->line_start = decl->start_lineno;
	op_array->line_end = decl->end_lineno;
	if (decl->doc_comment) {
		op_array->doc_comment = STR_COPY(decl->doc_comment);
	}
	if (decl->kind == ZEND_AST_CLOSURE) {
		op_array->fn_flags |= ZEND_ACC_CLOSURE;
	}

	if (is_method) {
		zend_bool has_body = stmt_ast != NULL;
		zend_begin_method_decl(op_array, decl->name, has_body TSRMLS_CC);
	} else {
		zend_begin_func_decl(result, op_array, decl TSRMLS_CC);
	}

	CG(active_op_array) = op_array;
	zend_stack_push(&CG(context_stack), (void *) &CG(context));
	zend_init_compiler_context(TSRMLS_C);

	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) {
		zend_op *opline_ext = emit_op(NULL, ZEND_EXT_NOP, NULL, NULL TSRMLS_CC);
		opline_ext->lineno = decl->start_lineno;
	}

	{
		/* Push a separator to the switch stack */
		zend_switch_entry switch_entry;

		switch_entry.cond.op_type = IS_UNUSED;
		switch_entry.default_case = 0;
		switch_entry.control_var = 0;

		zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry);
	}

	{
		/* Push a separator to the foreach stack */
		zend_op dummy_opline;

		dummy_opline.result_type = IS_UNUSED;

		zend_stack_push(&CG(foreach_copy_stack), (void *) &dummy_opline);
	}

	zend_compile_params(params_ast TSRMLS_CC);
	if (uses_ast) {
		zend_compile_closure_uses(uses_ast TSRMLS_CC);
	}
	zend_compile_stmt(stmt_ast TSRMLS_CC);

	if (is_method) {
		zend_check_magic_method_implementation(
			CG(active_class_entry), (zend_function *) op_array, E_COMPILE_ERROR TSRMLS_CC);
	}

	zend_do_extended_info(TSRMLS_C);
	zend_emit_final_return(NULL TSRMLS_CC);

	pass_two(CG(active_op_array) TSRMLS_CC);
	zend_release_labels(0 TSRMLS_CC);

	/* Pop the switch and foreach separators */
	zend_stack_del_top(&CG(switch_cond_stack));
	zend_stack_del_top(&CG(foreach_copy_stack));

	CG(active_op_array) = orig_op_array;
}

void zend_compile_prop_decl(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint flags = list->attr;
	zend_class_entry *ce = CG(active_class_entry);
	zend_uint i;

	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error_noreturn(E_COMPILE_ERROR, "Interfaces may not include member variables");
	}

	if (flags & ZEND_ACC_ABSTRACT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Properties cannot be declared abstract");
	}

	for (i = 0; i < list->children; ++i) {
		zend_ast *prop_ast = list->child[i];
		zend_ast *name_ast = prop_ast->child[0];
		zend_ast *value_ast = prop_ast->child[1];
		zend_string *name = zend_ast_get_str(name_ast);
		zval value_zv;

		if (flags & ZEND_ACC_FINAL) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare property %s::$%s final, "
				"the final modifier is allowed only for methods and classes",
				ce->name->val, name->val);
		}

		if (zend_hash_exists(&ce->properties_info, name)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare %s::$%s",
				ce->name->val, name->val);
		}

		if (value_ast) {
			_tmp_compile_const_expr(&value_zv, value_ast TSRMLS_CC);
		} else {
			ZVAL_NULL(&value_zv);
		}

		name = zend_new_interned_string_safe(name TSRMLS_CC);
		zend_declare_property_ex(ce, name, &value_zv, flags, NULL /* TODO.AST doc comment */ TSRMLS_CC);
	}
}

void zend_compile_class_const_decl(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_class_entry *ce = CG(active_class_entry);
	zend_uint i;

	for (i = 0; i < list->children; ++i) {
		zend_ast *const_ast = list->child[i];
		zend_ast *name_ast = const_ast->child[0];
		zend_ast *value_ast = const_ast->child[1];
		zend_string *name = zend_ast_get_str(name_ast);
		zval value_zv;

		if (ZEND_CE_IS_TRAIT(ce)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Traits cannot have constants");
			return;
		}

		_tmp_compile_const_expr(&value_zv, value_ast TSRMLS_CC);

		if (Z_TYPE(value_zv) == IS_ARRAY
			|| (Z_TYPE(value_zv) == IS_CONSTANT_AST && Z_ASTVAL(value_zv)->kind == ZEND_AST_ARRAY)
		) {
			zend_error_noreturn(E_COMPILE_ERROR, "Arrays are not allowed in class constants");
		}

		name = zend_new_interned_string_safe(name TSRMLS_CC);
		if (zend_hash_add(&ce->constants_table, name, &value_zv) == NULL) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redefine class constant %s::%s",
				ce->name->val, name->val);
		}

		if (Z_CONSTANT(value_zv)) {
			ce->ce_flags &= ~ZEND_ACC_CONSTANTS_UPDATED;
		}
	}
}

static zend_trait_method_reference *zend_compile_method_ref(zend_ast *ast TSRMLS_DC) {
	zend_ast *class_ast = ast->child[0];
	zend_ast *method_ast = ast->child[1];

	zend_trait_method_reference *method_ref = emalloc(sizeof(zend_trait_method_reference));
	method_ref->ce = NULL;
	method_ref->method_name = STR_COPY(zend_ast_get_str(method_ast));

	if (class_ast) {
		method_ref->class_name = zend_resolve_class_name_ast(class_ast TSRMLS_CC);
	} else {
		method_ref->class_name = NULL;
	}

	return method_ref;
}

static zend_string **zend_compile_name_list(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_string **names = safe_emalloc(sizeof(zend_string *), list->children + 1, 0);
	zend_uint i;

	for (i = 0; i < list->children; ++i) {
		zend_ast *name_ast = list->child[i];
		names[i] = zend_resolve_class_name_ast(name_ast TSRMLS_CC);
	}

	names[list->children] = NULL;

	return names;
}

static void zend_compile_trait_precedence(zend_ast *ast TSRMLS_DC) {
	zend_ast *method_ref_ast = ast->child[0];
	zend_ast *insteadof_ast = ast->child[1];
	
	zend_trait_precedence *precedence = emalloc(sizeof(zend_trait_precedence));
	precedence->trait_method = zend_compile_method_ref(method_ref_ast TSRMLS_CC);
	precedence->exclude_from_classes
		= (void *) zend_compile_name_list(insteadof_ast TSRMLS_CC);

	zend_add_to_list(&CG(active_class_entry)->trait_precedences, precedence TSRMLS_CC);
}

static void zend_compile_trait_alias(zend_ast *ast TSRMLS_DC) {
	zend_ast *method_ref_ast = ast->child[0];
	zend_ast *alias_ast = ast->child[1];
	zend_uint modifiers = ast->attr;

	zend_trait_alias *alias;

	if (modifiers == ZEND_ACC_STATIC) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'static' as method modifier");
	} else if (modifiers == ZEND_ACC_ABSTRACT) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'abstract' as method modifier");
	} else if (modifiers == ZEND_ACC_FINAL) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use 'final' as method modifier");
	}

	alias = emalloc(sizeof(zend_trait_alias));
	alias->trait_method = zend_compile_method_ref(method_ref_ast TSRMLS_CC);
	alias->modifiers = modifiers;

	if (alias_ast) {
		alias->alias = STR_COPY(zend_ast_get_str(alias_ast));
	} else {
		alias->alias = NULL;
	}

	zend_add_to_list(&CG(active_class_entry)->trait_aliases, alias TSRMLS_CC);
}

void zend_compile_use_trait(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *traits = zend_ast_get_list(ast->child[0]);
	zend_ast_list *adaptations = ast->child[1] ? zend_ast_get_list(ast->child[1]) : NULL;
	zend_class_entry *ce = CG(active_class_entry);
	zend_op *opline;
	zend_uint i;

	for (i = 0; i < traits->children; ++i) {
		zend_ast *trait_ast = traits->child[i];
		zend_string *name = zend_ast_get_str(trait_ast);

		if (ce->ce_flags & ZEND_ACC_INTERFACE) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use traits inside of interfaces. "
				"%s is used in %s", name->val, ce->name->val);
		}

		switch (zend_get_class_fetch_type(name->val, name->len)) {
			case ZEND_FETCH_CLASS_SELF:
			case ZEND_FETCH_CLASS_PARENT:
			case ZEND_FETCH_CLASS_STATIC:
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot use '%s' as trait name "
					"as it is reserved", name->val);
				break;
		}

		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_ADD_TRAIT;
		SET_NODE(opline->op1, &CG(implementing_class));
		opline->extended_value = ZEND_FETCH_CLASS_TRAIT;
		opline->op2_type = IS_CONST;
		opline->op2.constant = zend_add_class_name_literal(CG(active_op_array),
			zend_resolve_class_name_ast(trait_ast TSRMLS_CC) TSRMLS_CC);

		ce->num_traits++;
	}

	if (!adaptations) {
		return;
	}

	for (i = 0; i < adaptations->children; ++i) {
		zend_ast *adaptation_ast = adaptations->child[i];
		switch (adaptation_ast->kind) {
			case ZEND_AST_TRAIT_PRECEDENCE:
				zend_compile_trait_precedence(adaptation_ast TSRMLS_CC);
				break;
			case ZEND_AST_TRAIT_ALIAS:
				zend_compile_trait_alias(adaptation_ast TSRMLS_CC);
				break;
			EMPTY_SWITCH_DEFAULT_CASE()
		}
	}
}

void zend_compile_implements(znode *class_node, zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	for (i = 0; i < list->children; ++i) {
		zend_ast *class_ast = list->child[i];
		zend_string *name = zend_ast_get_str(class_ast);

		zend_op *opline;

		/* Traits can not implement interfaces */
		if (ZEND_CE_IS_TRAIT(CG(active_class_entry))) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use '%s' as interface on '%s' "
				"since it is a Trait", name->val, CG(active_class_entry)->name->val);
		}

		if (!zend_is_const_default_class_ref(class_ast)) {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use '%s' as interface name as it is reserved", name->val);
		}

		opline = emit_op(NULL, ZEND_ADD_INTERFACE, class_node, NULL TSRMLS_CC);
		opline->extended_value = ZEND_FETCH_CLASS_INTERFACE;
		opline->op2_type = IS_CONST;
		opline->op2.constant = zend_add_class_name_literal(CG(active_op_array),
			zend_resolve_class_name_ast(class_ast TSRMLS_CC) TSRMLS_CC);

		CG(active_class_entry)->num_interfaces++;
	}
}

void zend_compile_class_decl(zend_ast *ast TSRMLS_DC) {
	zend_ast_decl *decl = (zend_ast_decl *) ast;
	zend_ast *extends_ast = decl->child[0];
	zend_ast *implements_ast = decl->child[1];
	zend_ast *stmt_ast = decl->child[2];

	zend_string *name = decl->name, *lcname, *import_name = NULL;
	zend_class_entry *ce = zend_arena_alloc(&CG(arena), sizeof(zend_class_entry));
	zend_op *opline;
	znode declare_node, extends_node;

	if (CG(active_class_entry)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Class declarations may not be nested");
		return;
	}

	if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(name->val, name->len)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot use '%s' as class name as it is reserved",
			name->val);
	}

	lcname = STR_ALLOC(name->len, 0);
	zend_str_tolower_copy(lcname->val, name->val, name->len);

	if (CG(current_import)) {
		import_name = zend_hash_find_ptr(CG(current_import), lcname);
	}

	if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
		name = zend_concat_names(Z_STRVAL(CG(current_namespace)), Z_STRLEN(CG(current_namespace)),
			name->val, name->len);

		STR_RELEASE(lcname);
		lcname = STR_ALLOC(name->len, 0);
		zend_str_tolower_copy(lcname->val, name->val, name->len);
	} else {
		STR_ADDREF(name);
	}

	if (import_name) {
		char *import_name_lc = zend_str_tolower_dup(import_name->val, import_name->len);
		if (lcname->len != import_name->len
			|| memcmp(import_name_lc, lcname->val, lcname->len)
		) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot declare class %s "
				"because the name is already in use", name->val);
		}
		efree(import_name_lc);
	}

	name = zend_new_interned_string(name TSRMLS_CC);
	lcname = zend_new_interned_string(lcname TSRMLS_CC);

	ce->type = ZEND_USER_CLASS;
	ce->name = name;
	zend_initialize_class_data(ce, 1 TSRMLS_CC);

	ce->ce_flags |= decl->flags;
	ce->info.user.filename = zend_get_compiled_filename(TSRMLS_C);
	ce->info.user.line_start = decl->start_lineno;
	ce->info.user.line_end = decl->end_lineno;
	if (decl->doc_comment) {
		ce->info.user.doc_comment = STR_COPY(decl->doc_comment);
	}

	if (extends_ast) {
		if (ZEND_CE_IS_TRAIT(ce)) {
			zend_error_noreturn(E_COMPILE_ERROR, "A trait (%s) cannot extend a class. "
				"Traits can only be composed from other traits with the 'use' keyword. Error",
				name->val);
		}

		if (!zend_is_const_default_class_ref(extends_ast)) {
			zend_string *extends_name = zend_ast_get_str(extends_ast);
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use '%s' as class name as it is reserved", extends_name->val);
		}

		zend_compile_class_ref(&extends_node, extends_ast TSRMLS_CC);
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->result.var = get_temporary_variable(CG(active_op_array));
	opline->result_type = IS_VAR;
	GET_NODE(&declare_node, opline->result);

	// TODO.AST drop this
	GET_NODE(&CG(implementing_class), opline->result);

	opline->op2_type = IS_CONST;
	LITERAL_STR(opline->op2, lcname);

	if (extends_ast) {
		opline->opcode = ZEND_DECLARE_INHERITED_CLASS;
		opline->extended_value = extends_node.u.op.var;
	} else {
		opline->opcode = ZEND_DECLARE_CLASS;
	}

	{
		zval key;
		build_runtime_defined_function_key(&key, lcname, decl->lex_pos TSRMLS_CC);
		opline->op1_type = IS_CONST;
		opline->op1.constant = zend_add_literal(CG(active_op_array), &key TSRMLS_CC);

		zend_hash_update_ptr(CG(class_table), Z_STR(key), ce);
	}

	CG(active_class_entry) = ce;

	if (implements_ast) {
		zend_compile_implements(&declare_node, implements_ast TSRMLS_CC);
	}

	zend_compile_stmt(stmt_ast TSRMLS_CC);

	if (ce->constructor) {
		ce->constructor->common.fn_flags |= ZEND_ACC_CTOR;
		if (ce->constructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Constructor %s::%s() cannot be static",
				ce->name->val, ce->constructor->common.function_name->val);
		}
	}
	if (ce->destructor) {
		ce->destructor->common.fn_flags |= ZEND_ACC_DTOR;
		if (ce->destructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Destructor %s::%s() cannot be static",
				ce->name->val, ce->destructor->common.function_name->val);
		}
	}
	if (ce->clone) {
		ce->clone->common.fn_flags |= ZEND_ACC_CLONE;
		if (ce->clone->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error_noreturn(E_COMPILE_ERROR, "Clone method %s::%s() cannot be static",
				ce->name->val, ce->clone->common.function_name->val);
		}
	}

	/* Check for traits and proceed like with interfaces.
	 * The only difference will be a combined handling of them in the end.
	 * Thus, we need another opcode here. */
	if (ce->num_traits > 0) {
		ce->traits = NULL;
		ce->num_traits = 0;
		ce->ce_flags |= ZEND_ACC_IMPLEMENT_TRAITS;

		emit_op(NULL, ZEND_BIND_TRAITS, &declare_node, NULL TSRMLS_CC);
	}

	if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS))
		&& (extends_ast || ce->num_interfaces > 0)
	) {
		zend_verify_abstract_class(ce TSRMLS_CC);
		if (ce->num_interfaces && !(ce->ce_flags & ZEND_ACC_IMPLEMENT_TRAITS)) {
			emit_op(NULL, ZEND_VERIFY_ABSTRACT_CLASS, &declare_node, NULL TSRMLS_CC);
		}
	}

	/* Inherit interfaces; reset number to zero, we need it for above check and
	 * will restore it during actual implementation.
	 * The ZEND_ACC_IMPLEMENT_INTERFACES flag disables double call to
	 * zend_verify_abstract_class() */
	if (ce->num_interfaces > 0) {
		ce->interfaces = NULL;
		ce->num_interfaces = 0;
		ce->ce_flags |= ZEND_ACC_IMPLEMENT_INTERFACES;
	}

	CG(active_class_entry) = NULL;
}

static HashTable *zend_get_import_ht(zend_uint type TSRMLS_DC) {
	switch (type) {
		case T_CLASS:
			if (!CG(current_import)) {
				CG(current_import) = emalloc(sizeof(HashTable));
				zend_hash_init(CG(current_import), 8, NULL, str_dtor, 0);
			}
			return CG(current_import);
		case T_FUNCTION:
			if (!CG(current_import_function)) {
				CG(current_import_function) = emalloc(sizeof(HashTable));
				zend_hash_init(CG(current_import_function), 8, NULL, str_dtor, 0);
			}
			return CG(current_import_function);
		case T_CONST:
			if (!CG(current_import_const)) {
				CG(current_import_const) = emalloc(sizeof(HashTable));
				zend_hash_init(CG(current_import_const), 8, NULL, str_dtor, 0);
			}
			return CG(current_import_const);
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static char *zend_get_use_type_str(zend_uint type) {
	switch (type) {
		case T_CLASS:
			return "";
		case T_FUNCTION:
			return " function";
		case T_CONST:
			return " const";
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

static void zend_check_already_in_use(
	zend_uint type, zend_string *old_name, zend_string *new_name, zend_string *check_name
) {
	if (old_name->len == check_name->len) {
		char *old_name_lc = zend_str_tolower_dup(old_name->val, old_name->len);
		if (!memcmp(old_name_lc, check_name->val, check_name->len)) {
			efree(old_name_lc);
			return;
		}
	}

	zend_error_noreturn(E_COMPILE_ERROR, "Cannot use%s %s as %s because the name "
		"is already in use", zend_get_use_type_str(type), old_name->val, new_name->val);
}

void zend_compile_use(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	zend_string *current_ns = Z_TYPE(CG(current_namespace)) != IS_UNDEF
		? Z_STR(CG(current_namespace)) : NULL;
	zend_uint type = ast->attr;
	HashTable *current_import = zend_get_import_ht(type TSRMLS_CC);
	zend_bool case_sensitive = type == T_CONST;

	for (i = 0; i < list->children; ++i) {
		zend_ast *use_ast = list->child[i];
		zend_ast *old_name_ast = use_ast->child[0];
		zend_ast *new_name_ast = use_ast->child[1];
		zend_string *old_name = zend_ast_get_str(old_name_ast);
		zend_string *new_name, *lookup_name;

		if (new_name_ast) {
			new_name = STR_COPY(zend_ast_get_str(new_name_ast));
		} else {
			/* The form "use A\B" is eqivalent to "use A\B as B" */
			const char *p = zend_memrchr(old_name->val, '\\', old_name->len);
			if (p) {
				new_name = STR_INIT(p + 1, old_name->len - (p - old_name->val + 1), 0);
			} else {
				new_name = STR_COPY(old_name);

				if (!current_ns) {
					if (type == T_CLASS && zend_str_equals(new_name, "strict")) {
						zend_error_noreturn(E_COMPILE_ERROR,
							"You seem to be trying to use a different language...");
					}

					zend_error(E_WARNING, "The use statement with non-compound name '%s' "
						"has no effect", new_name->val);
				}
			}
		}

		if (case_sensitive) {
			lookup_name = STR_COPY(new_name);
		} else {
			lookup_name = STR_ALLOC(new_name->len, 0);
			zend_str_tolower_copy(lookup_name->val, new_name->val, new_name->len);
		}

		if (type == T_CLASS
			&& (zend_str_equals(lookup_name, "self") || zend_str_equals(lookup_name, "parent"))
		) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use %s as %s because '%s' "
				"is a special class name", old_name->val, new_name->val, new_name->val);
		}

		if (current_ns) {
			zend_string *ns_name = STR_ALLOC(current_ns->len + 1 + new_name->len, 0);
			zend_str_tolower_copy(ns_name->val, current_ns->val, current_ns->len);
			ns_name->val[current_ns->len] = '\\';
			memcpy(ns_name->val + current_ns->len + 1, lookup_name->val, lookup_name->len);

			if (zend_hash_exists(CG(class_table), ns_name)) {
				zend_check_already_in_use(type, old_name, new_name, ns_name);
			}

			STR_FREE(ns_name);
		} else {
			switch (type) {
				case T_CLASS:
				{
					zend_class_entry *ce = zend_hash_find_ptr(CG(class_table), lookup_name);
					if (ce && ce->type == ZEND_USER_CLASS
						&& ce->info.user.filename == CG(compiled_filename)
					) {
						zend_check_already_in_use(type, old_name, new_name, lookup_name);
					}
					break;
				}
				case T_FUNCTION:
				{
					zend_function *fn = zend_hash_find_ptr(CG(function_table), lookup_name);
					if (fn && fn->type == ZEND_USER_FUNCTION
						&& !strcmp(fn->op_array.filename->val, CG(compiled_filename)->val)
					) {
						zend_check_already_in_use(type, old_name, new_name, lookup_name);
					}
					break;
				}
				case T_CONST:
				{
					zend_string *filename = zend_hash_find_ptr(&CG(const_filenames), lookup_name);
					if (filename && !strcmp(filename->val, CG(compiled_filename)->val)) {
						zend_check_already_in_use(type, old_name, new_name, lookup_name);
					}
					break;
				}
				EMPTY_SWITCH_DEFAULT_CASE()
			}
		}

		STR_ADDREF(old_name);
		if (!zend_hash_add_ptr(current_import, lookup_name, old_name)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use%s %s as %s because the name "
				"is already in use", zend_get_use_type_str(type), old_name->val, new_name->val);
		}

		STR_RELEASE(lookup_name);
		STR_RELEASE(new_name);
	}
}

void zend_compile_const_decl(zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	for (i = 0; i < list->children; ++i) {
		zend_ast *const_ast = list->child[i];
		zend_ast *name_ast = const_ast->child[0];
		zend_ast *value_ast = const_ast->child[1];
		zend_string *name = zend_ast_get_str(name_ast);

		zend_string *import_name;
		znode name_node, value_node;
		zval *value_zv = &value_node.u.constant;

		value_node.op_type = IS_CONST;
		_tmp_compile_const_expr(value_zv, value_ast TSRMLS_CC);

		if (Z_TYPE_P(value_zv) == IS_ARRAY
			|| (Z_TYPE_P(value_zv) == IS_CONSTANT_AST
				&& Z_ASTVAL_P(value_zv)->kind == ZEND_AST_ARRAY)
		) {
			zend_error_noreturn(E_COMPILE_ERROR, "Arrays are not allowed as constants");
		}

		if (zend_get_ct_const(name, 0 TSRMLS_CC)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot redeclare constant '%s'", name->val);
		}

		name = zend_prefix_with_ns(name TSRMLS_CC);

		if (CG(current_import_const)
			&& (import_name = zend_hash_find_ptr(CG(current_import_const), name))
		) {
			if (import_name->len != name->len || memcmp(import_name->val, name->val, name->len)) {
				zend_error(E_COMPILE_ERROR, "Cannot declare const %s because "
					"the name is already in use", name->val);
			}
		}

		name_node.op_type = IS_CONST;
		ZVAL_STR(&name_node.u.constant, name);

		emit_op(NULL, ZEND_DECLARE_CONST, &name_node, &value_node TSRMLS_CC);

		zend_hash_add_ptr(&CG(const_filenames), name, CG(compiled_filename));
	}
}

static void zend_reset_import_tables(TSRMLS_D) {
	if (CG(current_import)) {
		zend_hash_destroy(CG(current_import));
		efree(CG(current_import));
		CG(current_import) = NULL;
	}

	if (CG(current_import_function)) {
		zend_hash_destroy(CG(current_import_function));
		efree(CG(current_import_function));
		CG(current_import_function) = NULL;
	}

	if (CG(current_import_const)) {
		zend_hash_destroy(CG(current_import_const));
		efree(CG(current_import_const));
		CG(current_import_const) = NULL;
	}
}

void zend_compile_namespace(zend_ast *ast TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	zend_ast *stmt_ast = ast->child[1];
	zend_string *name;
	zend_bool with_bracket = stmt_ast != NULL;

	/* handle mixed syntax declaration or nested namespaces */
	if (!CG(has_bracketed_namespaces)) {
		if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
			/* previous namespace declarations were unbracketed */
			if (with_bracket) {
				zend_error_noreturn(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations "
					"with unbracketed namespace declarations");
			}
		}
	} else {
		/* previous namespace declarations were bracketed */
		if (!with_bracket) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot mix bracketed namespace declarations "
				"with unbracketed namespace declarations");
		} else if (Z_TYPE(CG(current_namespace)) != IS_UNDEF || CG(in_namespace)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Namespace declarations cannot be nested");
		}
	}

	if (((!with_bracket && Z_TYPE(CG(current_namespace)) == IS_UNDEF)
		 || (with_bracket && !CG(has_bracketed_namespaces))) && CG(active_op_array)->last > 0
	) {
		/* ignore ZEND_EXT_STMT and ZEND_TICKS */
		zend_uint num = CG(active_op_array)->last;
		while (num > 0 &&
		       (CG(active_op_array)->opcodes[num-1].opcode == ZEND_EXT_STMT ||
		        CG(active_op_array)->opcodes[num-1].opcode == ZEND_TICKS)) {
			--num;
		}
		if (num > 0) {
			zend_error_noreturn(E_COMPILE_ERROR, "Namespace declaration statement has to be the very first statement in the script");
		}
	}

	if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
		zval_dtor(&CG(current_namespace));
	}

	if (name_ast) {
		name = zend_ast_get_str(name_ast);

		if (ZEND_FETCH_CLASS_DEFAULT != zend_get_class_fetch_type(name->val, name->len)) {
			zend_error_noreturn(E_COMPILE_ERROR, "Cannot use '%s' as namespace name", name->val);
		}

		ZVAL_STR(&CG(current_namespace), STR_COPY(name));
	} else {
		ZVAL_UNDEF(&CG(current_namespace));
	}

	zend_reset_import_tables(TSRMLS_C);

	CG(in_namespace) = 1;
	if (with_bracket) {
		CG(has_bracketed_namespaces) = 1;
	}

	if (stmt_ast) {
		zend_compile_top_stmt(stmt_ast TSRMLS_CC);

		CG(in_namespace) = 0;
		zend_reset_import_tables(TSRMLS_C);

		if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
			zval_dtor(&CG(current_namespace));
			ZVAL_UNDEF(&CG(current_namespace));
		}
	}
}

void zend_compile_halt_compiler(zend_ast *ast TSRMLS_DC) {
	zend_ast *offset_ast = ast->child[0];
	long offset = Z_LVAL_P(zend_ast_get_zval(offset_ast));

	zend_string *filename, *name;
	const char const_name[] = "__COMPILER_HALT_OFFSET__";

	if (CG(has_bracketed_namespaces) && CG(in_namespace)) {
		zend_error_noreturn(E_COMPILE_ERROR, "__HALT_COMPILER() can only be used from the outermost scope");
	}

	filename = zend_get_compiled_filename(TSRMLS_C);
	name = zend_mangle_property_name(const_name, sizeof(const_name) - 1,
		filename->val, filename->len, 0);

	zend_register_long_constant(name->val, name->len, offset, CONST_CS, 0 TSRMLS_CC);
	STR_RELEASE(name);
}

void zend_compile_binary_op(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];
	zend_uint opcode = ast->attr;

	znode left_node, right_node;
	zend_compile_expr(&left_node, left_ast TSRMLS_CC);
	zend_compile_expr(&right_node, right_ast TSRMLS_CC);

	emit_op_tmp(result, opcode, &left_node, &right_node TSRMLS_CC);
}

/* We do not use zend_compile_binary_op for this because we want to retain the left-to-right
 * evaluation order. */
void zend_compile_greater(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];
	znode left_node, right_node;

	ZEND_ASSERT(ast->kind == ZEND_AST_GREATER || ast->kind == ZEND_AST_GREATER_EQUAL);

	zend_compile_expr(&left_node, left_ast TSRMLS_CC);
	zend_compile_expr(&right_node, right_ast TSRMLS_CC);

	emit_op_tmp(result, ast->kind == ZEND_AST_GREATER ? ZEND_IS_SMALLER : ZEND_IS_SMALLER_OR_EQUAL,
		&right_node, &left_node TSRMLS_CC);
}

void zend_compile_unary_op(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	zend_uint opcode = ast->attr;

	znode expr_node;
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	emit_op_tmp(result, opcode, &expr_node, NULL TSRMLS_CC);
}

void zend_compile_unary_pm(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	znode zero_node, expr_node;

	ZEND_ASSERT(ast->kind == ZEND_AST_UNARY_PLUS || ast->kind == ZEND_AST_UNARY_MINUS);

	zero_node.op_type = IS_CONST;
	ZVAL_LONG(&zero_node.u.constant, 0);

	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	emit_op_tmp(result, ast->kind == ZEND_AST_UNARY_PLUS ? ZEND_ADD : ZEND_SUB,
		&zero_node, &expr_node TSRMLS_CC);
}

void zend_compile_short_circuiting(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];

	znode left_node, right_node;
	zend_op *opline_jmpz, *opline_bool;
	zend_uint opnum_jmpz;

	ZEND_ASSERT(ast->kind == ZEND_AST_AND || ast->kind == ZEND_AST_OR);

	zend_compile_expr(&left_node, left_ast TSRMLS_CC);

	opnum_jmpz = get_next_op_number(CG(active_op_array));
	opline_jmpz = emit_op(NULL, ast->kind == ZEND_AST_AND ? ZEND_JMPZ_EX : ZEND_JMPNZ_EX,
		&left_node, NULL TSRMLS_CC);

	if (left_node.op_type == IS_TMP_VAR) {
		SET_NODE(opline_jmpz->result, &left_node);
	} else {
		opline_jmpz->result.var = get_temporary_variable(CG(active_op_array));
		opline_jmpz->result_type = IS_TMP_VAR;
	}
	GET_NODE(result, opline_jmpz->result);

	zend_compile_expr(&right_node, right_ast TSRMLS_CC);

	opline_bool = emit_op(NULL, ZEND_BOOL, &right_node, NULL TSRMLS_CC);
	SET_NODE(opline_bool->result, result);

	zend_update_jump_target_to_next(opnum_jmpz TSRMLS_CC);
}

void zend_compile_post_incdec(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];

	znode var_node;
	zend_op *opline;

	ZEND_ASSERT(ast->kind == ZEND_AST_POST_INC || ast->kind == ZEND_AST_POST_DEC);

	if (var_ast->kind == ZEND_AST_PROP) {
		opline = zend_compile_prop_common(NULL, var_ast, BP_VAR_RW TSRMLS_CC);
		opline->opcode = ast->kind == ZEND_AST_POST_INC ? ZEND_POST_INC_OBJ : ZEND_POST_DEC_OBJ;
		opline->result_type = IS_TMP_VAR;
		opline->result.var = get_temporary_variable(CG(active_op_array));
		GET_NODE(result, opline->result);
	} else {
		zend_compile_var(&var_node, var_ast, BP_VAR_RW TSRMLS_CC);
		emit_op_tmp(result, ast->kind == ZEND_AST_POST_INC ? ZEND_POST_INC : ZEND_POST_DEC,
			&var_node, NULL TSRMLS_CC);
	}
}

void zend_compile_pre_incdec(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];

	znode var_node;
	zend_op *opline;

	ZEND_ASSERT(ast->kind == ZEND_AST_PRE_INC || ast->kind == ZEND_AST_PRE_DEC);

	if (var_ast->kind == ZEND_AST_PROP) {
		opline = zend_compile_prop_common(result, var_ast, BP_VAR_RW TSRMLS_CC);
		opline->opcode = ast->kind == ZEND_AST_PRE_INC ? ZEND_PRE_INC_OBJ : ZEND_PRE_DEC_OBJ;
	} else {
		zend_compile_var(&var_node, var_ast, BP_VAR_RW TSRMLS_CC);
		emit_op(result, ast->kind == ZEND_AST_PRE_INC ? ZEND_PRE_INC : ZEND_PRE_DEC,
			&var_node, NULL TSRMLS_CC);
	}
}

void zend_compile_cast(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;
	zend_op *opline;

	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	opline = emit_op(result, ZEND_CAST, &expr_node, NULL TSRMLS_CC);
	opline->extended_value = ast->attr;
}

static void zend_compile_shorthand_conditional(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *cond_ast = ast->child[0];
	zend_ast *false_ast = ast->child[2];

	znode cond_node, false_node;
	zend_op *opline_jmp_set, *opline_qm_assign;
	zend_uint opnum_jmp_set;

	ZEND_ASSERT(ast->child[1] == NULL);

	zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);

	opnum_jmp_set = get_next_op_number(CG(active_op_array));
	emit_op_tmp(result, ZEND_JMP_SET, &cond_node, NULL TSRMLS_CC);

	zend_compile_expr(&false_node, false_ast TSRMLS_CC);

	opline_jmp_set = &CG(active_op_array)->opcodes[opnum_jmp_set];
	opline_jmp_set->op2.opline_num = get_next_op_number(CG(active_op_array)) + 1;
	if (cond_node.op_type == IS_VAR || cond_node.op_type == IS_CV
		|| false_node.op_type == IS_VAR || false_node.op_type == IS_CV
	) {
		opline_jmp_set->opcode = ZEND_JMP_SET_VAR;
		opline_jmp_set->result_type = IS_VAR;
		GET_NODE(result, opline_jmp_set->result);

		opline_qm_assign = emit_op(NULL, ZEND_QM_ASSIGN_VAR, &false_node, NULL TSRMLS_CC);
	} else {
		opline_qm_assign = emit_op(NULL, ZEND_QM_ASSIGN, &false_node, NULL TSRMLS_CC);
	}
	SET_NODE(opline_qm_assign->result, result);
}

void zend_compile_conditional(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *cond_ast = ast->child[0];
	zend_ast *true_ast = ast->child[1];
	zend_ast *false_ast = ast->child[2];

	znode cond_node, true_node, false_node;
	zend_op *opline_qm_assign1, *opline_qm_assign2;
	zend_uint opnum_jmpz, opnum_jmp, opnum_qm_assign1;

	if (!true_ast) {
		zend_compile_shorthand_conditional(result, ast TSRMLS_CC);
		return;
	}
	
	zend_compile_expr(&cond_node, cond_ast TSRMLS_CC);

	opnum_jmpz = zend_emit_cond_jump(ZEND_JMPZ, &cond_node, 0 TSRMLS_CC);

	zend_compile_expr(&true_node, true_ast TSRMLS_CC);

	opnum_qm_assign1 = get_next_op_number(CG(active_op_array));
	emit_op(result, ZEND_QM_ASSIGN, &true_node, NULL TSRMLS_CC);

	opnum_jmp = zend_emit_jump(0 TSRMLS_CC);

	zend_update_jump_target_to_next(opnum_jmpz TSRMLS_CC);

	zend_compile_expr(&false_node, false_ast TSRMLS_CC);

	opline_qm_assign1 = &CG(active_op_array)->opcodes[opnum_qm_assign1];
	if (true_node.op_type == IS_VAR || true_node.op_type == IS_CV
		|| false_node.op_type == IS_VAR || false_node.op_type == IS_CV
	) {
		opline_qm_assign1->opcode = ZEND_QM_ASSIGN_VAR;
		opline_qm_assign1->result_type = IS_VAR;
		GET_NODE(result, opline_qm_assign1->result);
	}

	opline_qm_assign2 = emit_op(NULL, opline_qm_assign1->opcode, &false_node, NULL TSRMLS_CC);
	SET_NODE(opline_qm_assign2->result, result);

	zend_update_jump_target_to_next(opnum_jmp TSRMLS_CC);
}

void zend_compile_print(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];

	znode expr_node;
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	emit_op_tmp(result, ZEND_PRINT, &expr_node, NULL TSRMLS_CC);
}

void zend_compile_exit(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];

	if (expr_ast) {
		znode expr_node;
		zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);
		emit_op(NULL, ZEND_EXIT, &expr_node, NULL TSRMLS_CC);
	} else {
		emit_op(NULL, ZEND_EXIT, NULL, NULL TSRMLS_CC);
	}

	result->op_type = IS_CONST;
	ZVAL_BOOL(&result->u.constant, 1);
}

void zend_compile_yield(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *value_ast = ast->child[0];
	zend_ast *key_ast = ast->child[1];

	znode value_node, key_node;
	znode *value_node_ptr = NULL, *key_node_ptr = NULL;
	zend_op *opline;
	zend_bool returns_by_ref = (CG(active_op_array)->fn_flags & ZEND_ACC_RETURN_REFERENCE) != 0;

	if (!CG(active_op_array)->function_name) {
		zend_error_noreturn(E_COMPILE_ERROR, "The \"yield\" expression can only be used inside a function");
	}

	CG(active_op_array)->fn_flags |= ZEND_ACC_GENERATOR;

	if (key_ast) {
		zend_compile_expr(&key_node, key_ast TSRMLS_CC);
		key_node_ptr = &key_node;
	}

	if (value_ast) {
		if (returns_by_ref && zend_is_variable(value_ast) && !zend_is_call(value_ast)) {
			zend_compile_var(&value_node, value_ast, BP_VAR_REF TSRMLS_CC);
		} else {
			zend_compile_expr(&value_node, value_ast TSRMLS_CC);
		}
		value_node_ptr = &value_node;
	}

	opline = emit_op(result, ZEND_YIELD, value_node_ptr, key_node_ptr TSRMLS_CC);

	if (value_ast && returns_by_ref && zend_is_call(value_ast)) {
		opline->extended_value = ZEND_RETURNS_FUNCTION;
	}
}

void zend_compile_instanceof(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *obj_ast = ast->child[0];
	zend_ast *class_ast = ast->child[1];

	znode obj_node, class_node;
	zend_op *opline;

	zend_compile_expr(&obj_node, obj_ast TSRMLS_CC);
	if (obj_node.op_type == IS_CONST) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"instanceof expects an object instance, constant given");
	}

	opline = zend_compile_class_ref(&class_node, class_ast TSRMLS_CC);
	opline->extended_value |= ZEND_FETCH_CLASS_NO_AUTOLOAD;

	emit_op_tmp(result, ZEND_INSTANCEOF, &obj_node, &class_node TSRMLS_CC);
}

void zend_compile_include_or_eval(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	znode expr_node;
	zend_op *opline;

	zend_do_extended_fcall_begin(TSRMLS_C);
	zend_compile_expr(&expr_node, expr_ast TSRMLS_CC);

	opline = emit_op(result, ZEND_INCLUDE_OR_EVAL, &expr_node, NULL TSRMLS_CC);
	opline->extended_value = ast->attr;

	zend_do_extended_fcall_end(TSRMLS_C);
}

void zend_compile_isset_or_empty(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *var_ast = ast->child[0];

	znode var_node;
	zend_op *opline;

	ZEND_ASSERT(ast->kind == ZEND_AST_ISSET || ast->kind == ZEND_AST_EMPTY);

	if (!zend_is_variable(var_ast) || zend_is_call(var_ast)) {
		if (ast->kind == ZEND_AST_EMPTY) {
			/* empty(expr) can be transformed to !expr */
			zend_ast *not_ast = zend_ast_create_ex(1, ZEND_AST_UNARY_OP, ZEND_BOOL_NOT, var_ast);
			zend_compile_expr(result, not_ast TSRMLS_CC);
			return;
		} else {
			zend_error_noreturn(E_COMPILE_ERROR,
				"Cannot use isset() on the result of an expression "
				"(you can use \"null !== expression\" instead)");
		}
	}

	switch (var_ast->kind) {
		case ZEND_AST_VAR:
			if (zend_try_compile_cv(&var_node, var_ast TSRMLS_CC) == SUCCESS) {
				opline = emit_op(result, ZEND_ISSET_ISEMPTY_VAR, &var_node, NULL TSRMLS_CC);
				opline->extended_value = ZEND_FETCH_LOCAL | ZEND_QUICK_SET;
			} else {
				opline = zend_compile_simple_var_no_cv(result, var_ast, BP_VAR_IS TSRMLS_CC);
				opline->opcode = ZEND_ISSET_ISEMPTY_VAR;
			}
			break;
		case ZEND_AST_DIM:
			opline = zend_compile_dim_common(result, var_ast, BP_VAR_IS TSRMLS_CC);
			opline->opcode = ZEND_ISSET_ISEMPTY_DIM_OBJ;
			break;
		case ZEND_AST_PROP:
			opline = zend_compile_prop_common(result, var_ast, BP_VAR_IS TSRMLS_CC);
			opline->opcode = ZEND_ISSET_ISEMPTY_PROP_OBJ;
			break;
		case ZEND_AST_STATIC_PROP:
			opline = zend_compile_static_prop_common(result, var_ast, BP_VAR_IS TSRMLS_CC);
			opline->opcode = ZEND_ISSET_ISEMPTY_VAR;
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	opline->result_type = IS_TMP_VAR;
	opline->extended_value |= ast->kind == ZEND_AST_ISSET ? ZEND_ISSET : ZEND_ISEMPTY;
}

void zend_compile_silence(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];
	znode silence_node;

	emit_op_tmp(&silence_node, ZEND_BEGIN_SILENCE, NULL, NULL TSRMLS_CC);

	if (expr_ast->kind == ZEND_AST_VAR) {
		/* For @$var we need to force a FETCH instruction, otherwise the CV access will
		 * happen outside the silenced section. */
		zend_compile_simple_var_no_cv(result, expr_ast, BP_VAR_R TSRMLS_CC);
	} else {
		zend_compile_expr(result, expr_ast TSRMLS_CC);
	}

	emit_op(NULL, ZEND_END_SILENCE, &silence_node, NULL TSRMLS_CC);
}

void zend_compile_shell_exec(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *expr_ast = ast->child[0];

	zval fn_name;
	zend_ast *name_ast, *args_ast, *call_ast;

	ZVAL_STRING(&fn_name, "shell_exec");
	name_ast = zend_ast_create_zval(&fn_name);
	args_ast = (zend_ast *) zend_ast_create_list(1, ZEND_AST_ARG_LIST, expr_ast);
	call_ast = zend_ast_create_binary(ZEND_AST_CALL, name_ast, args_ast);

	zend_compile_expr(result, call_ast TSRMLS_CC);

	zval_ptr_dtor(&fn_name);
}

void zend_compile_array(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_op *opline;
	zend_uint i, opnum_init;
	zend_bool packed = 1;

	opnum_init = get_next_op_number(CG(active_op_array));

	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *value_ast = elem_ast->child[0];
		zend_ast *key_ast = elem_ast->child[1];
		zend_bool by_ref = elem_ast->attr;

		znode value_node, key_node, *key_node_ptr = NULL;

		if (key_ast) {
			zend_compile_expr(&key_node, key_ast TSRMLS_CC);
			key_node_ptr = &key_node;
		}

		if (by_ref) {
			zend_ensure_writable_variable(value_ast);
			zend_compile_var(&value_node, value_ast, BP_VAR_W TSRMLS_CC);
		} else {
			zend_compile_expr(&value_node, value_ast TSRMLS_CC);
		}

		if (i == 0) {
			opline = emit_op_tmp(result, ZEND_INIT_ARRAY, &value_node, key_node_ptr TSRMLS_CC);
			opline->extended_value = list->children << ZEND_ARRAY_SIZE_SHIFT;
		} else {
			opline = emit_op(NULL, ZEND_ADD_ARRAY_ELEMENT, &value_node, key_node_ptr TSRMLS_CC);
			SET_NODE(opline->result, result);
		}
		opline->extended_value |= by_ref;

		if (key_ast) {
			if (key_node.op_type == IS_CONST && Z_TYPE(key_node.u.constant) == IS_STRING) {
				zend_handle_numeric_op2(opline TSRMLS_CC);
				packed = 0;
			}
		}
	}

	/* Handle empty array */
	if (!list->children) {
		emit_op_tmp(result, ZEND_INIT_ARRAY, NULL, NULL TSRMLS_CC);
	}

	/* Add a flag to INIT_ARRAY if we know this array cannot be packed */
	if (!packed) {
		opline = &CG(active_op_array)->opcodes[opnum_init];
		opline->extended_value |= ZEND_ARRAY_NOT_PACKED;
	}
}

void zend_compile_const(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	zend_string *orig_name = zend_ast_get_str(name_ast);
	zend_bool is_fully_qualified;

	zval resolved_name;
	zend_op *opline;

	ZVAL_STR(&resolved_name, zend_resolve_const_name(
		orig_name, name_ast->attr, &is_fully_qualified TSRMLS_CC));

	if (zend_constant_ct_subst(result, &resolved_name, 1 TSRMLS_CC)) {
		zval_dtor(&resolved_name);
		return;
	}

	opline = emit_op_tmp(result, ZEND_FETCH_CONSTANT, NULL, NULL TSRMLS_CC);
	opline->op2_type = IS_CONST;

	if (is_fully_qualified) {
		opline->op2.constant = zend_add_const_name_literal(
			CG(active_op_array), &resolved_name, 0 TSRMLS_CC);
	} else {
		opline->extended_value = IS_CONSTANT_UNQUALIFIED;
		if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
			opline->extended_value |= IS_CONSTANT_IN_NAMESPACE;
			opline->op2.constant = zend_add_const_name_literal(
				CG(active_op_array), &resolved_name, 1 TSRMLS_CC);
		} else {
			opline->op2.constant = zend_add_const_name_literal(
				CG(active_op_array), &resolved_name, 0 TSRMLS_CC);
		}
	}
	GET_CACHE_SLOT(opline->op2.constant);
}

void zend_compile_class_const(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *class_ast = ast->child[0];
	zend_ast *const_ast = ast->child[1];

	znode class_node, const_node;
	zend_op *opline;

	if (zend_is_const_default_class_ref(class_ast)) {
		class_node.op_type = IS_CONST;
		ZVAL_STR(&class_node.u.constant, zend_resolve_class_name_ast(class_ast TSRMLS_CC));
	} else {
		zend_compile_class_ref(&class_node, class_ast TSRMLS_CC);
	}

	zend_compile_expr(&const_node, const_ast TSRMLS_CC);

	opline = emit_op_tmp(result, ZEND_FETCH_CONSTANT, NULL, &const_node TSRMLS_CC);

	zend_set_class_name_op1(opline, &class_node TSRMLS_CC);

	if (opline->op1_type == IS_CONST) {
		GET_CACHE_SLOT(opline->op2.constant);
	} else {
		GET_POLYMORPHIC_CACHE_SLOT(opline->op2.constant);
	}
}

void zend_compile_resolve_class_name(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast *name_ast = ast->child[0];
	zval *name = zend_ast_get_zval(name_ast);
	int fetch_type = zend_get_class_fetch_type(Z_STRVAL_P(name), Z_STRLEN_P(name));

	switch (fetch_type) {
		case ZEND_FETCH_CLASS_SELF:
			if (!CG(active_class_entry)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot access self::class when no class scope is active");
			}
			result->op_type = IS_CONST;
			ZVAL_STR(&result->u.constant, STR_COPY(CG(active_class_entry)->name));
			break;
        case ZEND_FETCH_CLASS_STATIC:
        case ZEND_FETCH_CLASS_PARENT:
			if (!CG(active_class_entry)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot access %s::class when no class scope is active",
					fetch_type == ZEND_FETCH_CLASS_STATIC ? "static" : "parent");
			} else {
				zval class_str_zv;
				zend_ast *class_str_ast, *class_const_ast;

				ZVAL_STRING(&class_str_zv, "class");
				class_str_ast = zend_ast_create_zval(&class_str_zv);
				class_const_ast = zend_ast_create_binary(
					ZEND_AST_CLASS_CONST, name_ast, class_str_ast);

				zend_compile_expr(result, class_const_ast TSRMLS_CC);

				zval_ptr_dtor(&class_str_zv);
			}
			break;
		case ZEND_FETCH_CLASS_DEFAULT:
			result->op_type = IS_CONST;
			ZVAL_STR(&result->u.constant, zend_resolve_class_name_ast(name_ast TSRMLS_CC));
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}

void zend_compile_encaps_list(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;

	ZEND_ASSERT(list->children > 0);

	result->op_type = IS_TMP_VAR;
	result->u.op.var = get_temporary_variable(CG(active_op_array));

	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		znode elem_node;
		zend_op *opline;

		zend_compile_expr(&elem_node, elem_ast TSRMLS_CC);

		if (elem_ast->kind == ZEND_AST_ZVAL) {
			zval *zv = &elem_node.u.constant;
			ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);

			if (Z_STRLEN_P(zv) > 1) {
				opline = get_next_op(CG(active_op_array) TSRMLS_CC);
				opline->opcode = ZEND_ADD_STRING;
			} else if (Z_STRLEN_P(zv) == 1) {
				char ch = *Z_STRVAL_P(zv);
				STR_RELEASE(Z_STR_P(zv));
				ZVAL_LONG(zv, ch);

				opline = get_next_op(CG(active_op_array) TSRMLS_CC);
				opline->opcode = ZEND_ADD_CHAR;
			} else {
				/* String can be empty after a variable at the end of a heredoc */
				STR_RELEASE(Z_STR_P(zv));
				continue;
			}
		} else {
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			opline->opcode = ZEND_ADD_VAR;
			ZEND_ASSERT(elem_node.op_type != IS_CONST);
		}

		if (i == 0) {
			SET_UNUSED(opline->op1);
		} else {
			SET_NODE(opline->op1, result);
		}
		SET_NODE(opline->op2, &elem_node);
		SET_NODE(opline->result, result);
	}
}

zend_bool zend_try_ct_compile_magic_const(zval *zv, zend_ast *ast TSRMLS_DC) {
	zend_op_array *op_array = CG(active_op_array);
	zend_class_entry *ce = CG(active_class_entry);

	switch (ast->attr) {
		case T_FUNC_C:
			if (op_array && op_array->function_name) {
				ZVAL_STR(zv, STR_COPY(op_array->function_name));
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		case T_METHOD_C:
			if (ce) {
				if (op_array && op_array->function_name) {
					ZVAL_STR(zv, zend_concat3(ce->name->val, ce->name->len, "::", 2,
						op_array->function_name->val, op_array->function_name->len));
				} else {
					ZVAL_STR(zv, STR_COPY(ce->name));
				}
			} else if (op_array && op_array->function_name) {
				ZVAL_STR(zv, STR_COPY(op_array->function_name));
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		case T_CLASS_C:
			if (ce) {
				if (ZEND_CE_IS_TRAIT(ce)) {
					return 0;
				} else {
					ZVAL_STR(zv, STR_COPY(ce->name));
				}
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		case T_TRAIT_C:
			if (ce && ZEND_CE_IS_TRAIT(ce)) {
				ZVAL_STR(zv, STR_COPY(ce->name));
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		case T_NS_C:
			if (Z_TYPE(CG(current_namespace)) != IS_UNDEF) {
				ZVAL_DUP(zv, &CG(current_namespace));
			} else {
				ZVAL_EMPTY_STRING(zv);
			}
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	return 1;
}

void zend_compile_magic_const(znode *result, zend_ast *ast TSRMLS_DC) {
	zend_class_entry *ce = CG(active_class_entry);

	if (zend_try_ct_compile_magic_const(&result->u.constant, ast TSRMLS_CC)) {
		result->op_type = IS_CONST;
		return;
	}

	ZEND_ASSERT(ast->attr == T_CLASS_C && ce && ZEND_CE_IS_TRAIT(ce));

	{
		zval const_zv;
		ZVAL_STRING(&const_zv, "__CLASS__");
		zend_ast *const_ast = zend_ast_create_unary(ZEND_AST_CONST,
			zend_ast_create_zval(&const_zv));
		zend_compile_const(result, const_ast TSRMLS_CC);
		zval_ptr_dtor(&const_zv);
	}
}

zend_bool zend_is_allowed_in_const_expr(zend_ast_kind kind) {
	return kind == ZEND_AST_ZVAL || kind == ZEND_AST_BINARY_OP
		|| kind == ZEND_AST_GREATER || kind == ZEND_AST_GREATER_EQUAL
		|| kind == ZEND_AST_AND || kind == ZEND_AST_OR
		|| kind == ZEND_AST_UNARY_OP
		|| kind == ZEND_AST_UNARY_PLUS || kind == ZEND_AST_UNARY_MINUS
		|| kind == ZEND_AST_CONDITIONAL
		|| kind == ZEND_AST_ARRAY || kind == ZEND_AST_ARRAY_ELEM
		|| kind == ZEND_AST_CONST || kind == ZEND_AST_CLASS_CONST
		|| kind == ZEND_AST_RESOLVE_CLASS_NAME || kind == ZEND_AST_MAGIC_CONST;
}

void zend_compile_const_expr_class_const(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *class_ast = ast->child[0];
	zend_ast *const_ast = ast->child[1];
	zend_string *class_name = zend_ast_get_str(class_ast);
	zend_string *const_name = zend_ast_get_str(const_ast);
	zval result;
	int fetch_type;

	if (class_ast->kind != ZEND_AST_ZVAL) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"Dynamic class names are not allowed in compile-time class constant references");
	}

	fetch_type = zend_get_class_fetch_type(class_name->val, class_name->len);

	if (ZEND_FETCH_CLASS_STATIC == fetch_type) {
		zend_error_noreturn(E_COMPILE_ERROR,
			"\"static::\" is not allowed in compile-time constants");
	}
	
	if (ZEND_FETCH_CLASS_DEFAULT == fetch_type) {
		class_name = zend_resolve_class_name_ast(class_ast TSRMLS_CC);
	} else {
		STR_ADDREF(class_name);
	}

	Z_STR(result) = zend_concat3(
		class_name->val, class_name->len, "::", 2, const_name->val, const_name->len);

	Z_TYPE_INFO(result) = IS_CONSTANT_EX;
	if (IS_INTERNED(Z_STR(result))) {
		Z_TYPE_FLAGS(result) &= ~ (IS_TYPE_REFCOUNTED | IS_TYPE_COPYABLE);
	}
	Z_CONST_FLAGS(result) = fetch_type;

	zend_ast_destroy(ast);
	STR_RELEASE(class_name);

	*ast_ptr = zend_ast_create_zval(&result);
}

void zend_compile_const_expr_const(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *name_ast = ast->child[0];
	zval *orig_name = zend_ast_get_zval(name_ast);
	zend_bool is_fully_qualified;

	znode result;
	zval resolved_name;

	if (zend_constant_ct_subst(&result, orig_name, 0 TSRMLS_CC)) {
		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&result.u.constant);
		return;
	}

	ZVAL_STR(&resolved_name, zend_resolve_const_name(
		Z_STR_P(orig_name), name_ast->attr, &is_fully_qualified TSRMLS_CC));

	Z_TYPE_INFO(resolved_name) = IS_CONSTANT_EX;
	if (!is_fully_qualified) {
		Z_CONST_FLAGS(resolved_name) = IS_CONSTANT_UNQUALIFIED;
	}

	zend_ast_destroy(ast);
	*ast_ptr = zend_ast_create_zval(&resolved_name);
}

void zend_compile_const_expr_resolve_class_name(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *name_ast = ast->child[0];
	zval *name = zend_ast_get_zval(name_ast);
	int fetch_type = zend_get_class_fetch_type(Z_STRVAL_P(name), Z_STRLEN_P(name));
	zval result;

	switch (fetch_type) {
		case ZEND_FETCH_CLASS_SELF:
			if (!CG(active_class_entry)) {
				zend_error_noreturn(E_COMPILE_ERROR,
					"Cannot access self::class when no class scope is active");
			}
			ZVAL_STR(&result, STR_COPY(CG(active_class_entry)->name));
			break;
        case ZEND_FETCH_CLASS_STATIC:
        case ZEND_FETCH_CLASS_PARENT:
			zend_error_noreturn(E_COMPILE_ERROR,
				"%s::class cannot be used for compile-time class name resolution",
				fetch_type == ZEND_FETCH_CLASS_STATIC ? "static" : "parent"
			);
			break;
		case ZEND_FETCH_CLASS_DEFAULT:
			ZVAL_STR(&result, zend_resolve_class_name_ast(name_ast TSRMLS_CC));
			break;
		EMPTY_SWITCH_DEFAULT_CASE()
	}

	zend_ast_destroy(ast);
	*ast_ptr = zend_ast_create_zval(&result);
}

void zend_compile_const_expr_magic_const(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_class_entry *ce = CG(active_class_entry);

	/* Other cases already resolved by constant folding */
	ZEND_ASSERT(ast->attr == T_CLASS_C && ce && ZEND_CE_IS_TRAIT(ce));

	{
		zval const_zv;
		ZVAL_STRING(&const_zv, "__CLASS__");
		Z_TYPE_INFO(const_zv) = IS_CONSTANT_EX;

		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&const_zv);
	}
}

void zend_compile_const_expr(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	if (ast == NULL || ast->kind == ZEND_AST_ZVAL) {
		return;
	}

	if (!zend_is_allowed_in_const_expr(ast->kind)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Constant expression contains invalid operations");
	}

	zend_ast_apply(ast, zend_compile_const_expr TSRMLS_CC);

	switch (ast->kind) {
		case ZEND_AST_CLASS_CONST:
			zend_compile_const_expr_class_const(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_CONST:
			zend_compile_const_expr_const(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_RESOLVE_CLASS_NAME:
			zend_compile_const_expr_resolve_class_name(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_MAGIC_CONST:
			zend_compile_const_expr_magic_const(ast_ptr TSRMLS_CC);
			break;
	}
}

/* Same as compile_stmt, but with early binding */
void zend_compile_top_stmt(zend_ast *ast TSRMLS_DC) {
	if (!ast) {
		return;
	}

	if (ast->kind == ZEND_AST_STMT_LIST) {
		zend_ast_list *list = zend_ast_get_list(ast);
		zend_uint i;
		for (i = 0; i < list->children; ++i) {
			zend_compile_top_stmt(list->child[i] TSRMLS_CC);
		}
		return;
	}

	zend_compile_stmt(ast TSRMLS_CC);

	if (ast->kind != ZEND_AST_NAMESPACE && ast->kind != ZEND_AST_HALT_COMPILER) {
		zend_verify_namespace(TSRMLS_C);
	}
	if (ast->kind == ZEND_AST_FUNC_DECL || ast->kind == ZEND_AST_CLASS) {
		CG(zend_lineno) = ((zend_ast_decl *) ast)->end_lineno;
		zend_do_early_binding(TSRMLS_C);
	}
}

void zend_compile_stmt(zend_ast *ast TSRMLS_DC) {
	if (!ast) {
		return;
	}

	CG(zend_lineno) = ast->lineno;

	switch (ast->kind) {
		case ZEND_AST_STMT_LIST:
			zend_compile_stmt_list(ast TSRMLS_CC);
			break;
		case ZEND_AST_GLOBAL:
			zend_compile_global_var(ast TSRMLS_CC);
			break;
		case ZEND_AST_STATIC:
			zend_compile_static_var(ast TSRMLS_CC);
			break;
		case ZEND_AST_UNSET:
			zend_compile_unset(ast TSRMLS_CC);
			break;
		case ZEND_AST_RETURN:
			zend_compile_return(ast TSRMLS_CC);
			break;
		case ZEND_AST_ECHO:
			zend_compile_echo(ast TSRMLS_CC);
			break;
		case ZEND_AST_THROW:
			zend_compile_throw(ast TSRMLS_CC);
			break;
		case ZEND_AST_BREAK:
		case ZEND_AST_CONTINUE:
			zend_compile_break_continue(ast TSRMLS_CC);
			break;
		case ZEND_AST_GOTO:
			zend_compile_goto(ast TSRMLS_CC);
			break;
		case ZEND_AST_LABEL:
			zend_compile_label(ast TSRMLS_CC);
			break;
		case ZEND_AST_WHILE:
			zend_compile_while(ast TSRMLS_CC);
			break;
		case ZEND_AST_DO_WHILE:
			zend_compile_do_while(ast TSRMLS_CC);
			break;
		case ZEND_AST_FOR:
			zend_compile_for(ast TSRMLS_CC);
			break;
		case ZEND_AST_FOREACH:
			zend_compile_foreach(ast TSRMLS_CC);
			break;
		case ZEND_AST_IF:
			zend_compile_if(ast TSRMLS_CC);
			break;
		case ZEND_AST_SWITCH:
			zend_compile_switch(ast TSRMLS_CC);
			break;
		case ZEND_AST_TRY:
			zend_compile_try(ast TSRMLS_CC);
			break;
		case ZEND_AST_DECLARE:
			zend_compile_declare(ast TSRMLS_CC);
			break;
		case ZEND_AST_FUNC_DECL:
		case ZEND_AST_METHOD:
			zend_compile_func_decl(NULL, ast TSRMLS_CC);
			break;
		case ZEND_AST_PROP_DECL:
			zend_compile_prop_decl(ast TSRMLS_CC);
			break;
		case ZEND_AST_CLASS_CONST_DECL:
			zend_compile_class_const_decl(ast TSRMLS_CC);
			break;
		case ZEND_AST_USE_TRAIT:
			zend_compile_use_trait(ast TSRMLS_CC);
			break;
		case ZEND_AST_CLASS:
			zend_compile_class_decl(ast TSRMLS_CC);
			break;
		case ZEND_AST_USE:
			zend_compile_use(ast TSRMLS_CC);
			break;
		case ZEND_AST_CONST_DECL:
			zend_compile_const_decl(ast TSRMLS_CC);
			break;
		case ZEND_AST_NAMESPACE:
			zend_compile_namespace(ast TSRMLS_CC);
			break;
		case ZEND_AST_HALT_COMPILER:
			zend_compile_halt_compiler(ast TSRMLS_CC);
			break;
		default:
		{
			znode result;
			zend_compile_expr(&result, ast TSRMLS_CC);
			zend_do_free(&result TSRMLS_CC);
		}
	}

	if (Z_LVAL(CG(declarables).ticks) && !zend_is_unticked_stmt(ast)) {
		zend_emit_tick(TSRMLS_C);
	}
}

void zend_compile_expr(znode *result, zend_ast *ast TSRMLS_DC) {
	//CG(zend_lineno) = ast->lineno;
	CG(zend_lineno) = zend_ast_get_lineno(ast);

	switch (ast->kind) {
		case ZEND_AST_ZVAL:
			ZVAL_COPY(&result->u.constant, zend_ast_get_zval(ast));
			result->op_type = IS_CONST;
			return;
		case ZEND_AST_ZNODE:
			*result = *zend_ast_get_znode(ast);
			return;
		case ZEND_AST_VAR:
		case ZEND_AST_DIM:
		case ZEND_AST_PROP:
		case ZEND_AST_STATIC_PROP:
		case ZEND_AST_CALL:
		case ZEND_AST_METHOD_CALL:
		case ZEND_AST_STATIC_CALL:
			zend_compile_var(result, ast, BP_VAR_R TSRMLS_CC);
			return;
		case ZEND_AST_ASSIGN:
			zend_compile_assign(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_ASSIGN_REF:
			zend_compile_assign_ref(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_NEW:
			zend_compile_new(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CLONE:
			zend_compile_clone(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_ASSIGN_OP:
			zend_compile_compound_assign(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_BINARY_OP:
			zend_compile_binary_op(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_GREATER:
		case ZEND_AST_GREATER_EQUAL:
			zend_compile_greater(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_UNARY_OP:
			zend_compile_unary_op(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_UNARY_PLUS:
		case ZEND_AST_UNARY_MINUS:
			zend_compile_unary_pm(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_AND:
		case ZEND_AST_OR:
			zend_compile_short_circuiting(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_POST_INC:
		case ZEND_AST_POST_DEC:
			zend_compile_post_incdec(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_PRE_INC:
		case ZEND_AST_PRE_DEC:
			zend_compile_pre_incdec(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CAST:
			zend_compile_cast(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CONDITIONAL:
			zend_compile_conditional(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_PRINT:
			zend_compile_print(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_EXIT:
			zend_compile_exit(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_YIELD:
			zend_compile_yield(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_INSTANCEOF:
			zend_compile_instanceof(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_INCLUDE_OR_EVAL:
			zend_compile_include_or_eval(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_ISSET:
		case ZEND_AST_EMPTY:
			zend_compile_isset_or_empty(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_SILENCE:
			zend_compile_silence(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_SHELL_EXEC:
			zend_compile_shell_exec(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_ARRAY:
			zend_compile_array(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CONST:
			zend_compile_const(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CLASS_CONST:
			zend_compile_class_const(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_RESOLVE_CLASS_NAME:
			zend_compile_resolve_class_name(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_ENCAPS_LIST:
			zend_compile_encaps_list(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_MAGIC_CONST:
			zend_compile_magic_const(result, ast TSRMLS_CC);
			return;
		case ZEND_AST_CLOSURE:
			zend_compile_func_decl(result, ast TSRMLS_CC);
			return;
		default:
			ZEND_ASSERT(0 /* not supported */);
	}
}

void zend_compile_var(znode *result, zend_ast *ast, int type TSRMLS_DC) {
	switch (ast->kind) {
		case ZEND_AST_VAR:
			zend_compile_simple_var(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_DIM:
			zend_compile_dim(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_PROP:
			zend_compile_prop(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_STATIC_PROP:
			zend_compile_static_prop(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_CALL:
			zend_compile_call(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_METHOD_CALL:
			zend_compile_method_call(result, ast, type TSRMLS_CC);
			return;
		case ZEND_AST_STATIC_CALL:
			zend_compile_static_call(result, ast, type TSRMLS_CC);
			return;
		default:
			if (type == BP_VAR_W || type == BP_VAR_REF
				|| type == BP_VAR_RW || type == BP_VAR_UNSET
			) {
				/* For BC reasons =& new Foo is allowed */
				if (type != BP_VAR_REF || ast->kind != ZEND_AST_NEW) {
					zend_error_noreturn(E_COMPILE_ERROR,
						"Cannot use temporary expression in write context");
				}
			}

			zend_compile_expr(result, ast TSRMLS_CC);
			return;
	}
}

void zend_eval_const_binary_op(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];
	zend_uchar opcode = ast->attr;

	if (left_ast->kind == ZEND_AST_ZVAL && right_ast->kind == ZEND_AST_ZVAL) {
		binary_op_type op = get_binary_op(opcode);
		zval result;
		op(&result, zend_ast_get_zval(left_ast), zend_ast_get_zval(right_ast) TSRMLS_CC);
		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&result);
	}
}

void zend_eval_const_unary_pm(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *expr_ast = ast->child[0];

	ZEND_ASSERT(ast->kind == ZEND_AST_UNARY_PLUS || ast->kind == ZEND_AST_UNARY_MINUS);

	if (expr_ast->kind == ZEND_AST_ZVAL) {
		binary_op_type op = ast->kind == ZEND_AST_UNARY_PLUS
			? add_function : sub_function;

		zval left, result;
		ZVAL_LONG(&left, 0);
		op(&result, &left, zend_ast_get_zval(expr_ast) TSRMLS_CC);
		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&result);
	}
}

void zend_eval_const_greater(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast *left_ast = ast->child[0];
	zend_ast *right_ast = ast->child[1];

	ZEND_ASSERT(ast->kind == ZEND_AST_GREATER || ast->kind == ZEND_AST_GREATER_EQUAL);

	if (left_ast->kind == ZEND_AST_ZVAL && right_ast->kind == ZEND_AST_ZVAL) {
		binary_op_type op = ast->kind == ZEND_AST_GREATER
			? is_smaller_function : is_smaller_or_equal_function;

		zval result;
		op(&result, zend_ast_get_zval(right_ast), zend_ast_get_zval(left_ast) TSRMLS_CC);
		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&result);
	}
}

void zend_eval_const_array(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zend_ast_list *list = zend_ast_get_list(ast);
	zend_uint i;
	zval array;

	/* First ensure that *all* child nodes are constant */
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *value_ast = elem_ast->child[0];
		zend_ast *key_ast = elem_ast->child[1];
		zend_bool by_ref = elem_ast->attr;

		if (by_ref || (key_ast && key_ast->kind != ZEND_AST_ZVAL) || value_ast->kind != ZEND_AST_ZVAL) {
			return;
		}
	}

	array_init_size(&array, list->children);
	for (i = 0; i < list->children; ++i) {
		zend_ast *elem_ast = list->child[i];
		zend_ast *value_ast = elem_ast->child[0];
		zend_ast *key_ast = elem_ast->child[1];

		zval *value = zend_ast_get_zval(value_ast);
		if (Z_REFCOUNTED_P(value)) Z_ADDREF_P(value);

		if (key_ast) {
			zval *key = zend_ast_get_zval(key_ast);
			switch (Z_TYPE_P(key)) {
				case IS_LONG:
					zend_hash_index_update(Z_ARRVAL(array), Z_LVAL_P(key), value);
					break;
				case IS_STRING:
					zend_symtable_update(Z_ARRVAL(array), Z_STR_P(key), value);
					break;
				case IS_DOUBLE:
					zend_hash_index_update(Z_ARRVAL(array),
						zend_dval_to_lval(Z_DVAL_P(key)), value);
					break;
				case IS_FALSE:
					zend_hash_index_update(Z_ARRVAL(array), 0, value);
					break;
				case IS_TRUE:
					zend_hash_index_update(Z_ARRVAL(array), 1, value);
					break;
				case IS_NULL:
					zend_hash_update(Z_ARRVAL(array), STR_EMPTY_ALLOC(), value);
					break;
				default:
					zend_error(E_COMPILE_ERROR, "Illegal offset type");
					break;
			}
		} else {
			zend_hash_next_index_insert(Z_ARRVAL(array), value);
		}
	}

	zend_ast_destroy(ast);
	zend_make_immutable_array(&array TSRMLS_CC);
	*ast_ptr = zend_ast_create_zval(&array);
}

void zend_eval_const_magic_const(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	zval result;

	if (zend_try_ct_compile_magic_const(&result, ast TSRMLS_CC)) {
		zend_ast_destroy(ast);
		*ast_ptr = zend_ast_create_zval(&result);
	}
}

void zend_eval_const_expr(zend_ast **ast_ptr TSRMLS_DC) {
	zend_ast *ast = *ast_ptr;
	if (!ast || ast->kind == ZEND_AST_ZVAL || ast->kind == ZEND_AST_ZNODE) {
		return;
	}

	zend_ast_apply(ast, zend_eval_const_expr TSRMLS_CC);

	switch (ast->kind) {
		case ZEND_AST_BINARY_OP:
			zend_eval_const_binary_op(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_GREATER:
		case ZEND_AST_GREATER_EQUAL:
			zend_eval_const_greater(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_UNARY_PLUS:
		case ZEND_AST_UNARY_MINUS:
			zend_eval_const_unary_pm(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_ARRAY:
			zend_eval_const_array(ast_ptr TSRMLS_CC);
			break;
		case ZEND_AST_MAGIC_CONST:
			zend_eval_const_magic_const(ast_ptr TSRMLS_CC);
			break;
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
