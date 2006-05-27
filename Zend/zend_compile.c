/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2006 Zend Technologies Ltd. (http://www.zend.com) |
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

#include <zend_language_parser.h>
#include "zend.h"
#include "zend_compile.h"
#include "zend_constants.h"
#include "zend_llist.h"
#include "zend_API.h"
#include "zend_fast_cache.h"

ZEND_API zend_op_array *(*zend_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);


#ifndef ZTS
ZEND_API zend_compiler_globals compiler_globals;
ZEND_API zend_executor_globals executor_globals;
#endif

static void zend_duplicate_property_info(zend_property_info *property_info)
{
	TSRMLS_FETCH();

	if (UG(unicode)) {
		property_info->name.u = eustrndup(property_info->name.u, property_info->name_length);
	} else {
		property_info->name.s = estrndup(property_info->name.s, property_info->name_length);
	}
	if (property_info->doc_comment) {
		property_info->doc_comment = estrndup(property_info->doc_comment, property_info->doc_comment_len);
	}
}


static void zend_duplicate_property_info_internal(zend_property_info *property_info)
{
	TSRMLS_FETCH();

	if (UG(unicode)) {
		property_info->name.u = zend_ustrndup(property_info->name.u, property_info->name_length);
	} else {
		property_info->name.s = zend_strndup(property_info->name.s, property_info->name_length);
	}
}


static void zend_destroy_property_info(zend_property_info *property_info)
{
	efree(property_info->name.v);
	if (property_info->doc_comment) {
		efree(property_info->doc_comment);
	}
}


static void zend_destroy_property_info_internal(zend_property_info *property_info)
{
	free(property_info->name.v);
}

static void build_runtime_defined_function_key(zval *result, zend_uchar type, zstr name, int name_length TSRMLS_DC)
{
	char char_pos_buf[32];
	uint char_pos_len;
	char *filename;
	uint filename_length;

	char_pos_len = zend_sprintf(char_pos_buf, "%p", LANG_SCNG(_yy_last_accepting_cpos));
	if (CG(active_op_array)->filename) {
		filename = CG(active_op_array)->filename;
	} else {
		filename = "-";
	}

	/* NULL, name length, filename length, last accepting char position length */
	filename_length = strlen(filename);
	Z_UNILEN_P(result) = 1+name_length+filename_length+char_pos_len;
	if (type == IS_UNICODE) {
		Z_USTRVAL_P(result) = (UChar *) eumalloc(Z_STRLEN_P(result)+1);		
		Z_USTRVAL_P(result)[0] = 0;
		memcpy(Z_USTRVAL_P(result)+1, name.u, UBYTES(name_length));

		/* UTODO: Support for non-ascii filenames */
		u_charsToUChars(filename, Z_USTRVAL_P(result)+1+name_length, filename_length);
		u_charsToUChars(char_pos_buf, Z_USTRVAL_P(result)+1+name_length+filename_length, char_pos_len);
		Z_USTRVAL_P(result)[1+name_length+filename_length+char_pos_len] = 0;
		Z_TYPE_P(result) = IS_UNICODE;
	} else {
		Z_STRVAL_P(result) = (char *) emalloc(Z_STRLEN_P(result)+1);
		Z_STRVAL_P(result)[0] = '\0';
		memcpy(Z_STRVAL_P(result)+1, name.s, name_length);
		memcpy(Z_STRVAL_P(result)+1+name_length, filename, filename_length);
		memcpy(Z_STRVAL_P(result)+1+name_length+filename_length, char_pos_buf, char_pos_len+1);
		Z_TYPE_P(result) = IS_STRING;
	}
	result->refcount = 1;
}


int zend_auto_global_arm(zend_auto_global *auto_global TSRMLS_DC)
{
	auto_global->armed = (auto_global->auto_global_callback ? 1 : 0);
	return 0;
}


int zend_auto_global_disable_jit(char *varname, zend_uint varname_length TSRMLS_DC)
{
	zend_auto_global *auto_global;

	if (zend_hash_find(CG(auto_globals), varname, varname_length+1, (void **) &auto_global)==FAILURE) {
		return FAILURE;
	}
	auto_global->armed = 0;
	return SUCCESS;
}


static void init_compiler_declarables(TSRMLS_D)
{
	Z_TYPE(CG(declarables).ticks) = IS_LONG;
	Z_LVAL(CG(declarables).ticks) = 0;
}


void zend_init_compiler_data_structures(TSRMLS_D)
{
	zend_stack_init(&CG(bp_stack));
	zend_stack_init(&CG(function_call_stack));
	zend_stack_init(&CG(switch_cond_stack));
	zend_stack_init(&CG(foreach_copy_stack));
	zend_stack_init(&CG(object_stack));
	zend_stack_init(&CG(declare_stack));
	CG(active_class_entry) = NULL;
	zend_llist_init(&CG(list_llist), sizeof(list_llist_element), NULL, 0);
	zend_llist_init(&CG(dimension_llist), sizeof(int), NULL, 0);
	zend_stack_init(&CG(list_stack));
	CG(handle_op_arrays) = 1;
	CG(in_compilation) = 0;
	CG(start_lineno) = 0;
	init_compiler_declarables(TSRMLS_C);
	zend_hash_apply(CG(auto_globals), (apply_func_t) zend_auto_global_arm TSRMLS_CC);
	zend_stack_init(&CG(labels_stack));
	CG(labels) = NULL;
}


void init_compiler(TSRMLS_D)
{
	CG(active_op_array) = NULL;
	zend_init_compiler_data_structures(TSRMLS_C);
	zend_init_rsrc_list(TSRMLS_C);
	zend_hash_init(&CG(filenames_table), 5, NULL, (dtor_func_t) free_estring, 0);
	zend_hash_init(&CG(script_encodings_table), 5, NULL, (dtor_func_t) free_estring, 0);
	zend_llist_init(&CG(open_files), sizeof(zend_file_handle), (void (*)(void *)) zend_file_handle_dtor, 0);
	CG(unclean_shutdown) = 0;
}


void shutdown_compiler(TSRMLS_D)
{
	zend_stack_destroy(&CG(bp_stack));
	zend_stack_destroy(&CG(function_call_stack));
	zend_stack_destroy(&CG(switch_cond_stack));
	zend_stack_destroy(&CG(foreach_copy_stack));
	zend_stack_destroy(&CG(object_stack));
	zend_stack_destroy(&CG(declare_stack));
	zend_stack_destroy(&CG(list_stack));
	zend_hash_destroy(&CG(script_encodings_table));
	zend_hash_destroy(&CG(filenames_table));
	zend_llist_destroy(&CG(open_files));
	zend_stack_destroy(&CG(labels_stack));
}


ZEND_API char *zend_set_compiled_filename(char *new_compiled_filename TSRMLS_DC)
{
	char **pp, *p;
	int length = strlen(new_compiled_filename);

	if (zend_hash_find(&CG(filenames_table), new_compiled_filename, length+1, (void **) &pp) == SUCCESS) {
		CG(compiled_filename) = *pp;
		return *pp;
	}
	p = estrndup(new_compiled_filename, length);
	zend_hash_update(&CG(filenames_table), new_compiled_filename, length+1, &p, sizeof(char *), (void **) &pp);
	CG(compiled_filename) = p;
	return p;
}


ZEND_API void zend_restore_compiled_filename(char *original_compiled_filename TSRMLS_DC)
{
	CG(compiled_filename) = original_compiled_filename;
}


ZEND_API char *zend_get_compiled_filename(TSRMLS_D)
{
	return CG(compiled_filename);
}


ZEND_API int zend_get_compiled_lineno(TSRMLS_D)
{
	return CG(zend_lineno);
}


ZEND_API zend_bool zend_is_compiling(TSRMLS_D)
{
	return CG(in_compilation);
}


ZEND_API char *zend_set_compiled_script_encoding(char *new_script_enc TSRMLS_DC)
{
	char **pp, *p;
	int length = strlen(new_script_enc);

	if (zend_hash_find(&CG(script_encodings_table), new_script_enc, length+1, (void **) &pp) == SUCCESS) {
		CG(script_encoding) = *pp;
		return *pp;
	}
	p = estrndup(new_script_enc, length);
	zend_hash_update(&CG(script_encodings_table), new_script_enc, length+1, &p, sizeof(char *), (void **) &pp);
	CG(script_encoding) = p;
	return p;
}


ZEND_API void zend_restore_compiled_script_encoding(char *original_script_enc TSRMLS_DC)
{
	CG(script_encoding) = original_script_enc;
}


ZEND_API char *zend_get_compiled_script_encoding(TSRMLS_D)
{
	return CG(script_encoding);
}


static zend_uint get_temporary_variable(zend_op_array *op_array)
{
	return (op_array->T)++ * sizeof(temp_variable);
}

static int lookup_cv(zend_op_array *op_array, zend_uchar type, zstr name, int name_len)
{
	int i = 0;
	ulong hash_value = zend_u_inline_hash_func(type, name, name_len+1);

	while (i < op_array->last_var) {
		if (op_array->vars[i].hash_value == hash_value &&
		    op_array->vars[i].name_len == name_len &&
		    !memcmp(op_array->vars[i].name.v, name.v, type==IS_UNICODE?UBYTES(name_len):name_len)) {
		  efree(name.v);
		  return i;
		}
		i++;
	}
	i = op_array->last_var;
	op_array->last_var++;
	if (op_array->last_var > op_array->size_var) {
		op_array->size_var += 16; /* FIXME */
		op_array->vars = erealloc(op_array->vars, op_array->size_var*sizeof(zend_compiled_variable));
	}
	op_array->vars[i].name = name; /* estrndup(name, name_len); */
	op_array->vars[i].name_len = name_len;
	op_array->vars[i].hash_value = hash_value;
	return i;
}


void zend_do_binary_op(zend_uchar op, znode *result, znode *op1, znode *op2 TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *op1;
	opline->op2 = *op2;
	*result = opline->result;
}


void zend_do_unary_op(zend_uchar op, znode *result, znode *op1 TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *op1;
	*result = opline->result;
	SET_UNUSED(opline->op2);
}

#define MAKE_NOP(opline)	{ opline->opcode = ZEND_NOP;  memset(&opline->result,0,sizeof(znode)); memset(&opline->op1,0,sizeof(znode)); memset(&opline->op2,0,sizeof(znode)); opline->result.op_type=opline->op1.op_type=opline->op2.op_type=IS_UNUSED;  }


static void zend_do_op_data(zend_op *data_op, znode *value TSRMLS_DC)
{
	data_op->opcode = ZEND_OP_DATA;
	data_op->op1 = *value;
	SET_UNUSED(data_op->op2);
}

void zend_do_binary_assign_op(zend_uchar op, znode *result, znode *op1, znode *op2 TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_OBJ_RW:
				last_op->opcode = op;
				last_op->extended_value = ZEND_ASSIGN_OBJ;

				zend_do_op_data(opline, op2 TSRMLS_CC);
				SET_UNUSED(opline->result);
				*result = last_op->result;
				return;
			case ZEND_FETCH_DIM_RW:
				last_op->opcode = op;
				last_op->extended_value = ZEND_ASSIGN_DIM;

				zend_do_op_data(opline, op2 TSRMLS_CC);
				opline->op2.u.var = get_temporary_variable(CG(active_op_array));
				opline->op2.u.EA.type = 0;
				opline->op2.op_type = IS_VAR;
				SET_UNUSED(opline->result);
				*result = last_op->result;
				return;
			default:
				break;
		}
	}

	opline->opcode = op;
	opline->op1 = *op1;
	opline->op2 = *op2;
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}

void fetch_simple_variable_ex(znode *result, znode *varname, int bp, zend_uchar op TSRMLS_DC)
{
	zend_op opline;
	zend_op *opline_ptr;
	zend_llist *fetch_list_ptr;

	if (varname->op_type == IS_CONST &&
	    (Z_TYPE(varname->u.constant) == IS_STRING ||
	     Z_TYPE(varname->u.constant) == IS_UNICODE) &&
	    !zend_u_is_auto_global(Z_TYPE(varname->u.constant), Z_UNIVAL(varname->u.constant), Z_UNILEN(varname->u.constant) TSRMLS_CC) &&
	    !(Z_UNILEN(varname->u.constant) == (sizeof("this")-1) &&
	      ZEND_U_EQUAL(Z_TYPE(varname->u.constant), Z_UNIVAL(varname->u.constant), Z_UNILEN(varname->u.constant), "this", sizeof("this")-1)) &&
	    (CG(active_op_array)->last == 0 ||
	     CG(active_op_array)->opcodes[CG(active_op_array)->last-1].opcode != ZEND_BEGIN_SILENCE)) {
		result->op_type = IS_CV;
		result->u.var = lookup_cv(CG(active_op_array), Z_TYPE(varname->u.constant), Z_UNIVAL(varname->u.constant), Z_UNILEN(varname->u.constant));
		result->u.EA.type = 0;
		return;
	}

	if (bp) {
		opline_ptr = &opline;
		init_op(opline_ptr TSRMLS_CC);
	} else {
		opline_ptr = get_next_op(CG(active_op_array) TSRMLS_CC);
	}

	opline_ptr->opcode = op;
	opline_ptr->result.op_type = IS_VAR;
	opline_ptr->result.u.EA.type = 0;
	opline_ptr->result.u.var = get_temporary_variable(CG(active_op_array));
	opline_ptr->op1 = *varname;
	*result = opline_ptr->result;
	SET_UNUSED(opline_ptr->op2);

	opline_ptr->op2.u.EA.type = ZEND_FETCH_LOCAL;
	if (varname->op_type == IS_CONST &&
	    (Z_TYPE(varname->u.constant) == IS_STRING ||
	     Z_TYPE(varname->u.constant) == IS_UNICODE)) {
		if (zend_u_is_auto_global(Z_TYPE(varname->u.constant), Z_UNIVAL(varname->u.constant), Z_UNILEN(varname->u.constant) TSRMLS_CC)) {
			opline_ptr->op2.u.EA.type = ZEND_FETCH_GLOBAL;
		}
	}

	if (bp) {
		zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
		zend_llist_add_element(fetch_list_ptr, opline_ptr);
	}
}

void fetch_simple_variable(znode *result, znode *varname, int bp TSRMLS_DC)
{
	/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	fetch_simple_variable_ex(result, varname, bp, ZEND_FETCH_W TSRMLS_CC);
}

void zend_do_fetch_static_member(znode *result, znode *class_znode TSRMLS_DC)
{
	zend_llist *fetch_list_ptr;
	zend_llist_element *le;
	zend_op *opline_ptr;
	zend_op opline;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
	if (result->op_type == IS_CV) {
		init_op(&opline TSRMLS_CC);

		opline.opcode = ZEND_FETCH_W;
		opline.result.op_type = IS_VAR;
		opline.result.u.EA.type = 0;
		opline.result.u.var = get_temporary_variable(CG(active_op_array));
		opline.op1.op_type = IS_CONST;
		ZVAL_TEXTL(&opline.op1.u.constant,
			CG(active_op_array)->vars[result->u.var].name,
			CG(active_op_array)->vars[result->u.var].name_len, 1);
		SET_UNUSED(opline.op2);
		opline.op2 = *class_znode;
		opline.op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
		*result = opline.result;

		zend_llist_add_element(fetch_list_ptr, &opline);
	} else {
		le = fetch_list_ptr->head;

		opline_ptr = (zend_op *)le->data;
		if (opline_ptr->opcode != ZEND_FETCH_W && opline_ptr->op1.op_type == IS_CV) {
			init_op(&opline TSRMLS_CC);
			opline.opcode = ZEND_FETCH_W;
			opline.result.op_type = IS_VAR;
			opline.result.u.EA.type = 0;
			opline.result.u.var = get_temporary_variable(CG(active_op_array));
			opline.op1.op_type = IS_CONST;
			ZVAL_TEXTL(&opline.op1.u.constant,
				CG(active_op_array)->vars[opline_ptr->op1.u.var].name,
				CG(active_op_array)->vars[opline_ptr->op1.u.var].name_len, 1);
			SET_UNUSED(opline.op2);
			opline.op2 = *class_znode;
			opline.op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
			opline_ptr->op1 = opline.result;

			zend_llist_prepend_element(fetch_list_ptr, &opline);
		} else {
			opline_ptr->op2 = *class_znode;
			opline_ptr->op2.u.EA.type = ZEND_FETCH_STATIC_MEMBER;
		}
	}
}

void fetch_array_begin(znode *result, znode *varname, znode *first_dim TSRMLS_DC)
{
	fetch_simple_variable(result, varname, 1 TSRMLS_CC);

	fetch_array_dim(result, result, first_dim TSRMLS_CC);
}


void fetch_array_dim(znode *result, znode *parent, znode *dim TSRMLS_DC)
{
	zend_op opline;
	zend_llist *fetch_list_ptr;

	init_op(&opline TSRMLS_CC);
	opline.opcode = ZEND_FETCH_DIM_W;	/* the backpatching routine assumes W */
	opline.result.op_type = IS_VAR;
	opline.result.u.EA.type = 0;
	opline.result.u.var = get_temporary_variable(CG(active_op_array));
	opline.op1 = *parent;
	opline.op2 = *dim;
	opline.extended_value = ZEND_FETCH_STANDARD;
	*result = opline.result;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);
	zend_llist_add_element(fetch_list_ptr, &opline);
}


void fetch_string_offset(znode *result, znode *parent, znode *offset TSRMLS_DC)
{
	fetch_array_dim(result, parent, offset TSRMLS_CC);
}


void zend_do_print(znode *result, znode *arg TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->opcode = ZEND_PRINT;
	opline->op1 = *arg;
	SET_UNUSED(opline->op2);
	*result = opline->result;
}


void zend_do_echo(znode *arg, zend_bool inline_html TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ECHO;
	opline->op1 = *arg;
	opline->extended_value = inline_html;
	SET_UNUSED(opline->op2);
}

void zend_do_abstract_method(znode *function_name, znode *modifiers, znode *body TSRMLS_DC)
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
			zend_error(E_COMPILE_ERROR, "%s function %v::%R() cannot be declared private", method_type, CG(active_class_entry)->name, Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant));
		}
		if (Z_LVAL(body->u.constant) == ZEND_ACC_ABSTRACT) {
			zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

			opline->opcode = ZEND_RAISE_ABSTRACT_ERROR;
			SET_UNUSED(opline->op1);
			SET_UNUSED(opline->op2);
		} else {
			/* we had code in the function body */
			zend_error(E_COMPILE_ERROR, "%s function %v::%R() cannot contain body", method_type, CG(active_class_entry)->name, Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant));
		}
	} else {
		if (Z_LVAL(body->u.constant) == ZEND_ACC_ABSTRACT) {
			zend_error(E_COMPILE_ERROR, "Non-abstract method %v::%R() must contain body", CG(active_class_entry)->name, Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant));
		}
	}
}

static zend_bool opline_is_fetch_this(zend_op *opline TSRMLS_DC)
{
	if ((opline->opcode == ZEND_FETCH_W) && (opline->op1.op_type == IS_CONST)
		&& (Z_TYPE(opline->op1.u.constant) == IS_STRING ||
	      Z_TYPE(opline->op1.u.constant) == IS_UNICODE)
	  && (Z_UNILEN(opline->op1.u.constant) == (sizeof("this")-1))
	  && ZEND_U_EQUAL(Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), "this", sizeof("this")-1)) {
		return 1;
	} else {
		return 0;
	}
}

void zend_do_assign(znode *result, znode *variable, znode *value TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (variable->op_type == IS_VAR) {
		int n = 0;

		while (last_op_number - n > 0) {
			zend_op *last_op;

			last_op = &CG(active_op_array)->opcodes[last_op_number-n-1];

			if (last_op->result.op_type == IS_VAR &&
			    last_op->result.u.var == variable->u.var) {
				if (last_op->opcode == ZEND_FETCH_OBJ_W) {
					if (n > 0) {
						*opline = *last_op;
						MAKE_NOP(last_op);
						last_op = opline;
						opline = get_next_op(CG(active_op_array) TSRMLS_CC);
					}
					last_op->opcode = ZEND_ASSIGN_OBJ;
					zend_do_op_data(opline, value TSRMLS_CC);
					SET_UNUSED(opline->result);
					*result = last_op->result;
					return;
				} else if (last_op->opcode == ZEND_FETCH_DIM_W) {
					if (n > 0) {
						*opline = *last_op;
						MAKE_NOP(last_op);
						last_op = opline;
						opline = get_next_op(CG(active_op_array) TSRMLS_CC);
					}
					last_op->opcode = ZEND_ASSIGN_DIM;
					zend_do_op_data(opline, value TSRMLS_CC);
					opline->op2.u.var = get_temporary_variable(CG(active_op_array));
					opline->op2.u.EA.type = 0;
					opline->op2.op_type = IS_VAR;
					SET_UNUSED(opline->result);
					*result = last_op->result;
					return;
				} else if (opline_is_fetch_this(last_op TSRMLS_CC)) {
					zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
				} else {
					break;
				}
			}
			n++;
		}
	}

	opline->opcode = ZEND_ASSIGN;
	opline->op1 = *variable;
	opline->op2 = *value;
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}

static inline zend_bool zend_is_function_or_method_call(znode *variable)
{
	zend_uint type = variable->u.EA.type;

	return  ((type & ZEND_PARSED_METHOD_CALL) || (type == ZEND_PARSED_FUNCTION_CALL));
}

void zend_do_assign_ref(znode *result, znode *lvar, znode *rvar TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		if (lvar->op_type == IS_VAR &&
		    opline_is_fetch_this(last_op TSRMLS_CC)) {
			zend_error(E_COMPILE_ERROR, "Cannot re-assign $this");
		}
	}

	opline->opcode = ZEND_ASSIGN_REF;
	if (zend_is_function_or_method_call(rvar)) {
		opline->extended_value = ZEND_RETURNS_FUNCTION;
	} else {
		opline->extended_value = 0;
	}
	if (result) {
		opline->result.op_type = IS_VAR;
		opline->result.u.EA.type = 0;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		*result = opline->result;
	} else {
		/* SET_UNUSED(opline->result); */
		opline->result.u.EA.type |= EXT_TYPE_UNUSED;
	}
	opline->op1 = *lvar;
	opline->op2 = *rvar;
}


static inline void do_begin_loop(TSRMLS_D)
{
	zend_brk_cont_element *brk_cont_element;
	int parent;

	parent = CG(active_op_array)->current_brk_cont;
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->last_brk_cont;
	brk_cont_element = get_next_brk_cont_element(CG(active_op_array));
	brk_cont_element->parent = parent;
}


static inline void do_end_loop(int cont_addr TSRMLS_DC)
{
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].cont = cont_addr;
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].brk = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].parent;
}


void zend_do_while_cond(znode *expr, znode *close_bracket_token TSRMLS_DC)
{
	int while_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *expr;
	close_bracket_token->u.opline_num = while_cond_op_number;
	SET_UNUSED(opline->op2);

	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}


void zend_do_while_end(znode *while_token, znode *close_bracket_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	/* add unconditional jump */
	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = while_token->u.opline_num;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	/* update while's conditional jmp */
	CG(active_op_array)->opcodes[close_bracket_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));

	do_end_loop(while_token->u.opline_num TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}


void zend_do_for_cond(znode *expr, znode *second_semicolon_token TSRMLS_DC)
{
	int for_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZNZ;
	opline->op1 = *expr;  /* the conditional expression */
	second_semicolon_token->u.opline_num = for_cond_op_number;
	SET_UNUSED(opline->op2);
}


void zend_do_for_before_statement(znode *cond_start, znode *second_semicolon_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = cond_start->u.opline_num;
	CG(active_op_array)->opcodes[second_semicolon_token->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	do_begin_loop(TSRMLS_C);

	INC_BPC(CG(active_op_array));
}


void zend_do_for_end(znode *second_semicolon_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = second_semicolon_token->u.opline_num+1;
	CG(active_op_array)->opcodes[second_semicolon_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	do_end_loop(second_semicolon_token->u.opline_num+1 TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}


void zend_do_pre_incdec(znode *result, znode *op1, zend_uchar op TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		if (last_op->opcode == ZEND_FETCH_OBJ_RW) {
			last_op->opcode = (op==ZEND_PRE_INC)?ZEND_PRE_INC_OBJ:ZEND_PRE_DEC_OBJ;
			last_op->result.op_type = IS_VAR;
			last_op->result.u.EA.type = 0;
			last_op->result.u.var = get_temporary_variable(CG(active_op_array));
			*result = last_op->result;
			return;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = op;
	opline->op1 = *op1;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}


void zend_do_post_incdec(znode *result, znode *op1, zend_uchar op TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		zend_op *last_op = &CG(active_op_array)->opcodes[last_op_number-1];

		if (last_op->opcode == ZEND_FETCH_OBJ_RW) {
			last_op->opcode = (op==ZEND_POST_INC)?ZEND_POST_INC_OBJ:ZEND_POST_DEC_OBJ;
			last_op->result.op_type = IS_TMP_VAR;
			last_op->result.u.var = get_temporary_variable(CG(active_op_array));
			*result = last_op->result;
			return;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = op;
	opline->op1 = *op1;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}


void zend_do_if_cond(znode *cond, znode *closing_bracket_token TSRMLS_DC)
{
	int if_cond_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *cond;
	closing_bracket_token->u.opline_num = if_cond_op_number;
	SET_UNUSED(opline->op2);
	INC_BPC(CG(active_op_array));
}


void zend_do_if_after_statement(znode *closing_bracket_token, unsigned char initialize TSRMLS_DC)
{
	int if_end_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_llist *jmp_list_ptr;

	opline->opcode = ZEND_JMP;
	/* save for backpatching */
	if (initialize) {
		zend_llist jmp_list;

		zend_llist_init(&jmp_list, sizeof(int), NULL, 0);
		zend_stack_push(&CG(bp_stack), (void *) &jmp_list, sizeof(zend_llist));
	}
	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	zend_llist_add_element(jmp_list_ptr, &if_end_op_number);

	CG(active_op_array)->opcodes[closing_bracket_token->u.opline_num].op2.u.opline_num = if_end_op_number+1;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_if_end(TSRMLS_D)
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_llist *jmp_list_ptr;
	zend_llist_element *le;

	zend_stack_top(&CG(bp_stack), (void **) &jmp_list_ptr);
	for (le=jmp_list_ptr->head; le; le = le->next) {
		CG(active_op_array)->opcodes[*((int *) le->data)].op1.u.opline_num = next_op_number;
	}
	zend_llist_destroy(jmp_list_ptr);
	zend_stack_del_top(&CG(bp_stack));
	DEC_BPC(CG(active_op_array));
}

void zend_check_writable_variable(znode *variable)
{
	zend_uint type = variable->u.EA.type;

	if (type & ZEND_PARSED_METHOD_CALL) {
		zend_error(E_COMPILE_ERROR, "Can't use method return value in write context");
	}
	if (type == ZEND_PARSED_FUNCTION_CALL) {
		zend_error(E_COMPILE_ERROR, "Can't use function return value in write context");
	}
}

void zend_do_begin_variable_parse(TSRMLS_D)
{
	zend_llist fetch_list;

	zend_llist_init(&fetch_list, sizeof(zend_op), NULL, 0);
	zend_stack_push(&CG(bp_stack), (void *) &fetch_list, sizeof(zend_llist));
}


void zend_do_end_variable_parse(int type, int arg_offset TSRMLS_DC)
{
	zend_llist *fetch_list_ptr;
	zend_llist_element *le;
	zend_op *opline, *opline_ptr=NULL;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	le = fetch_list_ptr->head;

	/* TODO: $foo->x->y->z = 1 should fetch "x" and "y" for R or RW, not just W */

	if (le) {
		opline_ptr = (zend_op *)le->data;
		if (opline_is_fetch_this(opline_ptr TSRMLS_CC)) {
			CG(active_op_array)->uses_this = 1;
		}

		while (1) {
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			memcpy(opline, opline_ptr, sizeof(zend_op));
			switch (type) {
				case BP_VAR_R:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for reading");
					}
					opline->opcode -= 3;
					break;
				case BP_VAR_W:
					break;
				case BP_VAR_RW:
					opline->opcode += 3;
					break;
				case BP_VAR_IS:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for reading");
					}
					opline->opcode += 6; /* 3+3 */
					break;
				case BP_VAR_FUNC_ARG:
					opline->opcode += 9; /* 3+3+3 */
					opline->extended_value = arg_offset;
					break;
				case BP_VAR_UNSET:
					if (opline->opcode == ZEND_FETCH_DIM_W && opline->op2.op_type == IS_UNUSED) {
						zend_error(E_COMPILE_ERROR, "Cannot use [] for unsetting");
					}
					opline->opcode += 12; /* 3+3+3+3 */
					break;
			}
			le = le->next;
			if (le == NULL) break;
			opline_ptr = (zend_op *)le->data;
		}
	}
	zend_llist_destroy(fetch_list_ptr);
	zend_stack_del_top(&CG(bp_stack));
}


void zend_do_init_string(znode *result TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_INIT_STRING;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->extended_value = CG(literal_type);
	*result = opline->result;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_add_char(znode *result, znode *op1, znode *op2 TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ADD_CHAR;
	opline->op1 = *op1;
	opline->op2 = *op2;
	opline->op2.op_type = IS_CONST;
	opline->extended_value = CG(literal_type);
	opline->extended_value = CG(literal_type);
	opline->result = opline->op1;
	*result = opline->result;
}


void zend_do_add_string(znode *result, znode *op1, znode *op2 TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ADD_STRING;
	opline->op1 = *op1;
	opline->op2 = *op2;
	opline->op2.op_type = IS_CONST;
	opline->result = opline->op1;
	*result = opline->result;
}


void zend_do_add_variable(znode *result, znode *op1, znode *op2 TSRMLS_DC)
{
	zend_op *opline;

	if (op1->op_type == IS_CONST) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_INIT_STRING;
		opline->result.op_type = IS_TMP_VAR;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		*result = opline->result;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);

		if (Z_STRLEN(op1->u.constant)>0) {
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			opline->opcode = ZEND_ADD_STRING;
			opline->result = *result;
			opline->op1 = *result;
			opline->op2 = *op1;
			opline->result = opline->op1;
		} else {
			zval_dtor(&op1->u.constant);
		}
	} else {
		*result = *op1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_ADD_VAR;
	opline->result = *result;
	opline->op1 = *result;
	opline->op2 = *op2;
	opline->extended_value = CG(literal_type);
	*result = opline->result;
}

void zend_do_free(znode *op1 TSRMLS_DC)
{
	if (op1->op_type==IS_TMP_VAR) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_FREE;
		opline->op1 = *op1;
		SET_UNUSED(opline->op2);
	} else if (op1->op_type==IS_VAR) {
		zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];

		while (opline->opcode == ZEND_END_SILENCE || opline->opcode == ZEND_EXT_FCALL_END || opline->opcode == ZEND_OP_DATA) {
			opline--;
		}
		if (opline->result.op_type == IS_VAR
			&& opline->result.u.var == op1->u.var) {
			opline->result.u.EA.type |= EXT_TYPE_UNUSED;
		} else {
			while (opline>CG(active_op_array)->opcodes) {
				if (opline->opcode == ZEND_FETCH_DIM_R
				    && opline->op1.op_type == IS_VAR
				    && opline->op1.u.var == op1->u.var) {
					/* This should the end of a list() construct
					 * Mark its result as unused
					 */
					opline->extended_value = ZEND_FETCH_STANDARD;
					break;
				} else if (opline->result.op_type==IS_VAR
					&& opline->result.u.var == op1->u.var) {
					if (opline->opcode == ZEND_NEW) {
						opline->result.u.EA.type |= EXT_TYPE_UNUSED;
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


int zend_do_verify_access_types(znode *current_access_type, znode *new_modifier)
{
	if ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_PPP_MASK)
		&& (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_PPP_MASK)
		&& ((Z_LVAL(current_access_type->u.constant) & ZEND_ACC_PPP_MASK) != (Z_LVAL(new_modifier->u.constant) & ZEND_ACC_PPP_MASK))) {
		zend_error(E_COMPILE_ERROR, "Multiple access type modifiers are not allowed");
	}
	if (((Z_LVAL(current_access_type->u.constant) | Z_LVAL(new_modifier->u.constant)) & (ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL)) == (ZEND_ACC_ABSTRACT | ZEND_ACC_FINAL)) {
		zend_error(E_COMPILE_ERROR, "Cannot use the final modifier on an abstract class member");
	}
	return (Z_LVAL(current_access_type->u.constant) | Z_LVAL(new_modifier->u.constant));
}


void zend_do_begin_function_declaration(znode *function_token, znode *function_name, int is_method, int return_reference, znode *fn_flags_znode TSRMLS_DC)
{
	zend_op_array op_array;
	zstr name = Z_UNIVAL(function_name->u.constant);
	int name_len = Z_UNILEN(function_name->u.constant);
	int function_begin_line = function_token->u.opline_num;
	zend_uint fn_flags;
	unsigned int lcname_len;
	zstr lcname;

	if (is_method) {
		if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
			if ((Z_LVAL(fn_flags_znode->u.constant) & ~(ZEND_ACC_STATIC|ZEND_ACC_PUBLIC))) {
				zend_error(E_COMPILE_ERROR, "Access type for interface method %v::%R() must be omitted", CG(active_class_entry)->name, Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant));
			}
			Z_LVAL(fn_flags_znode->u.constant) |= ZEND_ACC_ABSTRACT; /* propagates to the rest of the parser */
		}
		fn_flags = Z_LVAL(fn_flags_znode->u.constant); /* must be done *after* the above check */
	} else {
		fn_flags = 0;
	}
	if ((fn_flags & ZEND_ACC_STATIC) && (fn_flags & ZEND_ACC_ABSTRACT) && !(CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error(E_COMPILE_ERROR, "Static function %v%s%R() cannot be abstract", is_method ? CG(active_class_entry)->name : EMPTY_ZSTR, is_method ? "::" : "", Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant));
	}

	function_token->u.op_array = CG(active_op_array);
	lcname = zend_u_str_case_fold(Z_TYPE(function_name->u.constant), name, name_len, 0, &lcname_len);

	init_op_array(&op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE TSRMLS_CC);

	op_array.function_name = name;
	op_array.return_reference = return_reference;
	op_array.fn_flags = fn_flags;
	op_array.pass_rest_by_reference = 0;

	op_array.scope = is_method?CG(active_class_entry):NULL;
	op_array.prototype = NULL;

	op_array.line_start = zend_get_compiled_lineno(TSRMLS_C);

	if (is_method) {
		zstr short_class_name = CG(active_class_entry)->name;
		unsigned int short_class_name_length = CG(active_class_entry)->name_length;

		if (zend_u_hash_add(&CG(active_class_entry)->function_table, Z_TYPE(function_name->u.constant), lcname, lcname_len+1, &op_array, sizeof(zend_op_array), (void **) &CG(active_op_array)) == FAILURE) {
			zend_op_array *child_op_array, *parent_op_array;
			if (CG(active_class_entry)->parent
			    && (zend_u_hash_find(&CG(active_class_entry)->function_table, Z_TYPE(function_name->u.constant), name, name_len+1, (void **) &child_op_array) == SUCCESS)
			    && (zend_u_hash_find(&CG(active_class_entry)->parent->function_table, Z_TYPE(function_name->u.constant), name, name_len+1, (void **) &parent_op_array) == SUCCESS)
					&& (child_op_array == parent_op_array)) {
				zend_u_hash_update(&CG(active_class_entry)->function_table, Z_TYPE(function_name->u.constant), name, name_len+1, &op_array, sizeof(zend_op_array), (void **) &CG(active_op_array));
			} else {
				efree(lcname.v);
				zend_error(E_COMPILE_ERROR, "Cannot redeclare %v::%R()", CG(active_class_entry)->name, Z_TYPE(function_name->u.constant), name);
			}
		}

		if (fn_flags & ZEND_ACC_ABSTRACT) {
			CG(active_class_entry)->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}

		if (!(fn_flags & ZEND_ACC_PPP_MASK)) {
			fn_flags |= ZEND_ACC_PUBLIC;
		}

		if (!(CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE)) {
			short_class_name = zend_u_str_case_fold(UG(unicode)?IS_UNICODE:IS_STRING, CG(active_class_entry)->name, short_class_name_length, 0, &short_class_name_length);
			/* Improve after RC: cache the lowercase class name */

			if ((short_class_name_length == name_len) && (!memcmp(short_class_name.v, lcname.v, UG(unicode)?UBYTES(lcname_len):lcname_len))) {
				if (CG(active_class_entry)->constructor) {
					zend_error(E_STRICT, "Redefining already defined constructor for class %v", CG(active_class_entry)->name);
				} else {
					CG(active_class_entry)->constructor = (zend_function *) CG(active_op_array);
				}
			} else if ((lcname_len == sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1))) {
				if (CG(active_class_entry)->constructor) {
					zend_error(E_STRICT, "Redefining already defined constructor for class %v", CG(active_class_entry)->name);
				}
				CG(active_class_entry)->constructor = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_DESTRUCTOR_FUNC_NAME, sizeof(ZEND_DESTRUCTOR_FUNC_NAME)-1))) {
				CG(active_class_entry)->destructor = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_CLONE_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME)-1))) {
				CG(active_class_entry)->clone = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_CALL_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1))) {
				CG(active_class_entry)->__call = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_GET_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_GET_FUNC_NAME, sizeof(ZEND_GET_FUNC_NAME)-1))) {
				CG(active_class_entry)->__get = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_SET_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_SET_FUNC_NAME, sizeof(ZEND_SET_FUNC_NAME)-1))) {
				CG(active_class_entry)->__set = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_UNSET_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_UNSET_FUNC_NAME, sizeof(ZEND_UNSET_FUNC_NAME)-1))) {
				CG(active_class_entry)->__unset = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_ISSET_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_ISSET_FUNC_NAME, sizeof(ZEND_ISSET_FUNC_NAME)-1))) {
				CG(active_class_entry)->__isset = (zend_function *) CG(active_op_array);
			} else if ((lcname_len == sizeof(ZEND_TOSTRING_FUNC_NAME)-1) && (ZEND_U_EQUAL(Z_TYPE(function_name->u.constant), lcname, lcname_len, ZEND_TOSTRING_FUNC_NAME, sizeof(ZEND_TOSTRING_FUNC_NAME)-1))) {
				CG(active_class_entry)->__tostring = (zend_function *) CG(active_op_array);
			} else if (!(fn_flags & ZEND_ACC_STATIC)) {
				CG(active_op_array)->fn_flags |= ZEND_ACC_ALLOW_STATIC;
			}
			efree(short_class_name.v);
		}

		efree(lcname.v);
	} else {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_DECLARE_FUNCTION;
		opline->op1.op_type = IS_CONST;
		build_runtime_defined_function_key(&opline->op1.u.constant, Z_TYPE(function_name->u.constant), lcname, lcname_len TSRMLS_CC);
		opline->op2.op_type = IS_CONST;
		Z_TYPE(opline->op2.u.constant) = Z_TYPE(function_name->u.constant);
		Z_STRVAL(opline->op2.u.constant) = lcname.s;
		Z_STRLEN(opline->op2.u.constant) = lcname_len;
		opline->op2.u.constant.refcount = 1;
		opline->extended_value = ZEND_DECLARE_FUNCTION;
		zend_u_hash_update(CG(function_table), Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), &op_array, sizeof(zend_op_array), (void **) &CG(active_op_array));
	}

	if (CG(extended_info)) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_EXT_NOP;
		opline->lineno = function_begin_line;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
	}

	{
		/* Push a seperator to the switch and foreach stacks */
		zend_switch_entry switch_entry;

		switch_entry.cond.op_type = IS_UNUSED;
		switch_entry.default_case = 0;
		switch_entry.control_var = 0;

		zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry, sizeof(switch_entry));

		{
			/* Foreach stack separator */
			zend_op dummy_opline;

			dummy_opline.result.op_type = IS_UNUSED;
			dummy_opline.op1.op_type = IS_UNUSED;

			zend_stack_push(&CG(foreach_copy_stack), (void *) &dummy_opline, sizeof(zend_op));
		}
	}

	if (CG(doc_comment)) {
		CG(active_op_array)->doc_comment = CG(doc_comment);
		CG(active_op_array)->doc_comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}

	zend_stack_push(&CG(labels_stack), (void *) &CG(labels), sizeof(HashTable*));
	CG(labels) = NULL;
}

void zend_do_handle_exception(TSRMLS_D)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_HANDLE_EXCEPTION;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_end_function_declaration(znode *function_token TSRMLS_DC)
{
	unsigned int lcname_len;
	zstr lcname;
	char lcname_buf[16];
	int name_len;

	zend_do_extended_info(TSRMLS_C);
	zend_do_return(NULL, 0 TSRMLS_CC);
	zend_do_handle_exception(TSRMLS_C);

	pass_two(CG(active_op_array) TSRMLS_CC);

	zend_release_labels(TSRMLS_C);

	if (CG(active_class_entry)) {
		zend_check_magic_method_implementation(CG(active_class_entry), (zend_function*)CG(active_op_array), E_COMPILE_ERROR TSRMLS_CC);
	} else {
		zend_uchar utype;

		/* we don't care if the function name is longer, in fact lowercasing only
		 * the beginning of the name speeds up the check process */
		if (UG(unicode)) {
			utype = IS_UNICODE;
			name_len = u_strlen(CG(active_op_array)->function_name.u);
			lcname = zend_u_str_case_fold(utype, CG(active_op_array)->function_name, name_len, 0, &lcname_len);
		} else {
			utype = IS_STRING;
			lcname_len = name_len = strlen(CG(active_op_array)->function_name.s);
			zend_str_tolower_copy(lcname_buf, CG(active_op_array)->function_name.s, MIN(name_len, sizeof(lcname_buf)-1));
			lcname_buf[sizeof(lcname_buf)-1] = '\0'; /* zend_str_tolower_copy won't necessarily set the zero byte */
			lcname.s = lcname_buf;
		}
		if (lcname_len == sizeof(ZEND_AUTOLOAD_FUNC_NAME) - 1 &&
		    ZEND_U_EQUAL(utype, lcname, lcname_len, ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME)-1) &&
		    CG(active_op_array)->num_args != 1) {
			zend_error(E_COMPILE_ERROR, "%s() must take exactly 1 argument", ZEND_AUTOLOAD_FUNC_NAME);
		}
		if (lcname.s != lcname_buf) {
			efree(lcname.s);
		}
	}

	CG(active_op_array)->line_end = zend_get_compiled_lineno(TSRMLS_C);
	CG(active_op_array) = function_token->u.op_array;


	/* Pop the switch and foreach seperators */
	zend_stack_del_top(&CG(switch_cond_stack));
	zend_stack_del_top(&CG(foreach_copy_stack));
}


static inline int ZEND_U_CASE_EQUAL(zend_uchar type, zstr ustr, int ulen, char *str, int slen)
{
	zstr lcname;
	unsigned int lcname_len;
	int ret;

	lcname = zend_u_str_case_fold(type, ustr, ulen, 0, &lcname_len);
	ret = ZEND_U_EQUAL(type, lcname, lcname_len, str, slen);
	efree(lcname.v);
	return ret;
}


void zend_do_receive_arg(zend_uchar op, znode *var, znode *offset, znode *initialization, znode *class_type, znode *varname, zend_uchar pass_by_reference TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_arg_info *cur_arg_info;

	CG(active_op_array)->num_args++;
	opline->opcode = op;
	opline->result = *var;
	opline->op1 = *offset;
	if (op == ZEND_RECV_INIT) {
		opline->op2 = *initialization;
	} else {
		CG(active_op_array)->required_num_args = CG(active_op_array)->num_args;
		SET_UNUSED(opline->op2);
	}
	CG(active_op_array)->arg_info = erealloc(CG(active_op_array)->arg_info, sizeof(zend_arg_info)*(CG(active_op_array)->num_args));
	cur_arg_info = &CG(active_op_array)->arg_info[CG(active_op_array)->num_args-1];
	if (Z_TYPE(varname->u.constant) == IS_UNICODE) {
		cur_arg_info->name.u = eustrndup(Z_USTRVAL(varname->u.constant), Z_USTRLEN(varname->u.constant));
	} else {
		cur_arg_info->name.s = estrndup(Z_STRVAL(varname->u.constant), Z_STRLEN(varname->u.constant));
	}
	cur_arg_info->name_len = Z_UNILEN(varname->u.constant);
	cur_arg_info->array_type_hint = 0;
	cur_arg_info->allow_null = 1;
	cur_arg_info->pass_by_reference = pass_by_reference;

	if (class_type->op_type != IS_UNUSED) {
		cur_arg_info->allow_null = 0;
		if (Z_TYPE(class_type->u.constant) == IS_STRING || Z_TYPE(class_type->u.constant) == IS_UNICODE) {
			cur_arg_info->class_name = Z_UNIVAL(class_type->u.constant);
			cur_arg_info->class_name_len = Z_UNILEN(class_type->u.constant);
			if (op == ZEND_RECV_INIT) {
				if (Z_TYPE(initialization->u.constant) == IS_NULL ||
				    (Z_TYPE(initialization->u.constant) == IS_CONSTANT &&
				    Z_UNILEN(initialization->u.constant) == sizeof("null") - 1 &&
				    ZEND_U_CASE_EQUAL(UG(unicode)?IS_UNICODE:IS_STRING, Z_UNIVAL(initialization->u.constant), Z_UNILEN(initialization->u.constant), "null", sizeof("null")-1))) {
					cur_arg_info->allow_null = 1;
				} else {
					zend_error(E_COMPILE_ERROR, "Default value for parameters with a class type hint can only be NULL");
				}
			}
		} else {
			cur_arg_info->array_type_hint = 1;
			cur_arg_info->class_name = NULL_ZSTR;
			cur_arg_info->class_name_len = 0;
			if (op == ZEND_RECV_INIT) {
				if (Z_TYPE(initialization->u.constant) == IS_NULL ||
				    (Z_TYPE(initialization->u.constant) == IS_CONSTANT &&
				    Z_UNILEN(initialization->u.constant) == sizeof("null") - 1 &&
				    ZEND_U_CASE_EQUAL(UG(unicode)?IS_UNICODE:IS_STRING, Z_UNIVAL(initialization->u.constant), Z_UNILEN(initialization->u.constant), "null", sizeof("null")-1))) {
					cur_arg_info->allow_null = 1;
				} else if (Z_TYPE(initialization->u.constant) != IS_ARRAY && Z_TYPE(initialization->u.constant) != IS_CONSTANT_ARRAY) {
					zend_error(E_COMPILE_ERROR, "Default value for parameters with array type hint can only be an array or NULL");
				}
			}
		}
	} else {
		cur_arg_info->class_name = NULL_ZSTR;
		cur_arg_info->class_name_len = 0;
	}
	opline->result.u.EA.type |= EXT_TYPE_UNUSED;
}


int zend_do_begin_function_call(znode *function_name TSRMLS_DC)
{
	zend_function *function;
	unsigned int lcname_len;
	zstr lcname;

	lcname = zend_u_str_case_fold(Z_TYPE(function_name->u.constant), Z_UNIVAL(function_name->u.constant), Z_UNILEN(function_name->u.constant), 0, &lcname_len);
	if (zend_u_hash_find(CG(function_table), Z_TYPE(function_name->u.constant), lcname, lcname_len+1, (void **) &function)==FAILURE) {
		zend_do_begin_dynamic_function_call(function_name TSRMLS_CC);
		efree(lcname.v);
		return 1; /* Dynamic */
	}
	efree(Z_UNIVAL(function_name->u.constant).v);
	if (Z_TYPE(function_name->u.constant) == IS_UNICODE) {
		Z_USTRVAL(function_name->u.constant) = lcname.u;
		Z_USTRLEN(function_name->u.constant) = lcname_len;
	} else {
		Z_STRVAL(function_name->u.constant) = lcname.s;
	}

	switch (function->type) {
		case ZEND_USER_FUNCTION:	{
				zend_op_array *op_array = (zend_op_array *) function;

				zend_stack_push(&CG(function_call_stack), (void *) &op_array, sizeof(zend_function *));
			}
			break;
		case ZEND_INTERNAL_FUNCTION: {
				zend_internal_function *internal_function = (zend_internal_function *) function;

				zend_stack_push(&CG(function_call_stack), (void *) &internal_function, sizeof(zend_function *));
			}
			break;
	}
	zend_do_extended_fcall_begin(TSRMLS_C);
	return 0;
}



void zend_do_begin_method_call(znode *left_bracket TSRMLS_DC)
{
	zend_op *last_op;
	int last_op_number;
	unsigned char *ptr = NULL;

	zend_do_end_variable_parse(BP_VAR_R, 0 TSRMLS_CC);
	zend_do_begin_variable_parse(TSRMLS_C);

	last_op_number = get_next_op_number(CG(active_op_array))-1;
	last_op = &CG(active_op_array)->opcodes[last_op_number];

	if (last_op->op2.op_type == IS_CONST &&
	    (Z_TYPE(last_op->op2.u.constant) == IS_STRING ||
	     Z_TYPE(last_op->op2.u.constant) == IS_UNICODE) &&
	    Z_UNILEN(last_op->op2.u.constant) == sizeof(ZEND_CLONE_FUNC_NAME)-1) {
		zstr lcname;
		unsigned int lcname_len;

		lcname = zend_u_str_case_fold(Z_TYPE(last_op->op2.u.constant), Z_UNIVAL(last_op->op2.u.constant), Z_UNILEN(last_op->op2.u.constant), 0, &lcname_len);
		if (ZEND_U_EQUAL(Z_TYPE(last_op->op2.u.constant), lcname, lcname_len, ZEND_CLONE_FUNC_NAME, sizeof(ZEND_CLONE_FUNC_NAME)-1)) {
			efree(lcname.v);
			zend_error(E_COMPILE_ERROR, "Cannot call __clone() method on objects - use 'clone $obj' instead");
		}
		efree(lcname.v);
	}

	if (last_op->opcode == ZEND_FETCH_OBJ_R) {
		last_op->opcode = ZEND_INIT_METHOD_CALL;
		Z_LVAL(left_bracket->u.constant) = ZEND_INIT_FCALL_BY_NAME;
	} else {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_INIT_FCALL_BY_NAME;
		opline->op2 = *left_bracket;
		opline->extended_value = 0;
		SET_UNUSED(opline->op1);
	}

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
}


void zend_do_clone(znode *result, znode *expr TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_CLONE;
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	*result = opline->result;
}


void zend_do_begin_dynamic_function_call(znode *function_name TSRMLS_DC)
{
	unsigned char *ptr = NULL;
	zend_op *opline;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INIT_FCALL_BY_NAME;
	opline->op2 = *function_name;
	opline->extended_value = 0;

	SET_UNUSED(opline->op1);

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
}


void zend_do_fetch_class(znode *result, znode *class_name TSRMLS_DC)
{
	long fetch_class_op_number;
	zend_op *opline;

	fetch_class_op_number = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_FETCH_CLASS;
	SET_UNUSED(opline->op1);
	opline->extended_value = ZEND_FETCH_CLASS_GLOBAL;
	CG(catch_begin) = fetch_class_op_number;
	if (class_name->op_type == IS_CONST) {
		int fetch_type;

		fetch_type = zend_get_class_fetch_type(Z_TYPE(class_name->u.constant), Z_UNIVAL(class_name->u.constant), Z_UNILEN(class_name->u.constant));
		switch (fetch_type) {
			case ZEND_FETCH_CLASS_SELF:
			case ZEND_FETCH_CLASS_PARENT:
				SET_UNUSED(opline->op2);
				opline->extended_value = fetch_type;
				zval_dtor(&class_name->u.constant);
				break;
			default:
				opline->op2 = *class_name;
				break;
		}
	} else {
		opline->op2 = *class_name;
	}
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.u.EA.type = opline->extended_value;
	opline->result.op_type = IS_CONST; /* FIXME: Hack so that INIT_FCALL_BY_NAME still knows this is a class */
	*result = opline->result;
}


void zend_do_fetch_class_name(znode *result, znode *class_name_entry, znode *class_name TSRMLS_DC)
{
	zend_uint length;

	if (!result) {
		result = class_name_entry;
	} else {
		*result = *class_name_entry;
	}

	length = sizeof("::")-1 + Z_USTRLEN(result->u.constant) + Z_USTRLEN(class_name->u.constant);
	if (Z_TYPE(result->u.constant) == IS_UNICODE) {
		Z_USTRVAL(result->u.constant) = erealloc(Z_USTRVAL(result->u.constant), UBYTES(length+1));
		Z_USTRVAL(result->u.constant)[Z_USTRLEN(result->u.constant)] = ':';
		Z_USTRVAL(result->u.constant)[Z_USTRLEN(result->u.constant)+1] = ':';
		memcpy(&Z_USTRVAL(result->u.constant)[Z_USTRLEN(result->u.constant) + sizeof("::")-1], Z_STRVAL(class_name->u.constant), UBYTES(Z_USTRLEN(class_name->u.constant)+1));
		STR_FREE(Z_USTRVAL(class_name->u.constant));
	} else {
		Z_STRVAL(result->u.constant) = erealloc(Z_STRVAL(result->u.constant), length+1);
		memcpy(&Z_STRVAL(result->u.constant)[Z_STRLEN(result->u.constant)], "::", sizeof("::")-1);
		memcpy(&Z_STRVAL(result->u.constant)[Z_STRLEN(result->u.constant) + sizeof("::")-1], Z_STRVAL(class_name->u.constant), Z_STRLEN(class_name->u.constant)+1);
		STR_FREE(Z_STRVAL(class_name->u.constant));
	}
	Z_STRLEN(result->u.constant) = length;
}

void zend_do_begin_class_member_function_call(znode *class_name, znode *method_name TSRMLS_DC)
{
	unsigned char *ptr = NULL;
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_INIT_STATIC_METHOD_CALL;
	opline->op1 = *class_name;
	opline->op2 = *method_name;

	if (opline->op2.op_type == IS_CONST) {
		if ((sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1) == Z_UNILEN(opline->op2.u.constant) &&
		    ZEND_U_EQUAL(Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant), ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME)-1)) {
			zval_dtor(&opline->op2.u.constant);
			SET_UNUSED(opline->op2);
		} else {
			if (Z_TYPE(opline->op2.u.constant) == IS_UNICODE) {
				unsigned int lcname_len;
				zstr lcname;

				lcname = zend_u_str_case_fold(Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant), 0, &lcname_len);
				efree(Z_USTRVAL(opline->op2.u.constant));
				Z_USTRVAL(opline->op2.u.constant) = lcname.u;
				Z_USTRLEN(opline->op2.u.constant) = lcname_len;
			} else {
				zend_str_tolower(Z_STRVAL(opline->op2.u.constant), Z_STRLEN(opline->op2.u.constant));
			}
		}
	}

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(zend_function *));
	zend_do_extended_fcall_begin(TSRMLS_C);
}


void zend_do_end_function_call(znode *function_name, znode *result, znode *argument_list, int is_method, int is_dynamic_fcall TSRMLS_DC)
{
	zend_op *opline;

	if (is_method && function_name && function_name->op_type == IS_UNUSED) {
		/* clone */
		if (Z_LVAL(argument_list->u.constant) != 0) {
			zend_error(E_WARNING, "Clone method does not require arguments");
		}
		opline = &CG(active_op_array)->opcodes[Z_LVAL(function_name->u.constant)];
	} else {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		if (!is_method && !is_dynamic_fcall && function_name->op_type==IS_CONST) {
			opline->opcode = ZEND_DO_FCALL;
			opline->op1 = *function_name;
		} else {
			opline->opcode = ZEND_DO_FCALL_BY_NAME;
			SET_UNUSED(opline->op1);
		}
	}

	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_VAR;
	*result = opline->result;
	SET_UNUSED(opline->op2);

	zend_stack_del_top(&CG(function_call_stack));
	opline->extended_value = Z_LVAL(argument_list->u.constant);
}


void zend_do_pass_param(znode *param, zend_uchar op, int offset TSRMLS_DC)
{
	zend_op *opline;
	int original_op=op;
	zend_function **function_ptr_ptr, *function_ptr;
	int send_by_reference;
	int send_function = 0;

	zend_stack_top(&CG(function_call_stack), (void **) &function_ptr_ptr);
	function_ptr = *function_ptr_ptr;

	if (original_op==ZEND_SEND_REF) {
		if (function_ptr &&
		    function_ptr->common.function_name.v &&
		    function_ptr->common.type == ZEND_USER_FUNCTION &&
		    !ARG_SHOULD_BE_SENT_BY_REF(function_ptr, (zend_uint) offset)) {
			zend_error(E_STRICT,
				"Call-time pass-by-reference has been deprecated; "
				"if you would like to pass argument by reference, modify the declaration of %R().",
				UG(unicode)?IS_UNICODE:IS_STRING,
				function_ptr->common.function_name.v);
		} else{
			zend_error(E_STRICT,
				"Call-time pass-by-reference has been deprecated");
		}
		send_by_reference = 1;
	} else if (function_ptr) {
		if (ARG_MAY_BE_SENT_BY_REF(function_ptr, (zend_uint) offset)) {
			op = (param->op_type & (IS_VAR|IS_CV))?ZEND_SEND_REF:ZEND_SEND_VAL;
			send_by_reference = 0;
		} else {
			send_by_reference = ARG_SHOULD_BE_SENT_BY_REF(function_ptr, (zend_uint) offset) ? ZEND_ARG_SEND_BY_REF : 0;
		}
	} else {
		send_by_reference = 0;
	}

	if (op == ZEND_SEND_VAR && zend_is_function_or_method_call(param)) {
		/* Method call */
		op = ZEND_SEND_VAR_NO_REF;
		send_function = ZEND_ARG_SEND_FUNCTION;
	} else if (op == ZEND_SEND_VAL && (param->op_type & (IS_VAR|IS_CV))) {
		op = ZEND_SEND_VAR_NO_REF;
	}

	if (op!=ZEND_SEND_VAR_NO_REF && send_by_reference==ZEND_ARG_SEND_BY_REF) {
		/* change to passing by reference */
		switch (param->op_type) {
			case IS_VAR:
			case IS_CV:
				op = ZEND_SEND_REF;
				break;
			default:
				zend_error(E_COMPILE_ERROR, "Only variables can be passed by reference");
				break;
		}
	}

	if (original_op == ZEND_SEND_VAR) {
		switch (op) {
			case ZEND_SEND_VAR_NO_REF:
				zend_do_end_variable_parse(BP_VAR_R, 0 TSRMLS_CC);
				break;
			case ZEND_SEND_VAR:
				if (function_ptr) {
					zend_do_end_variable_parse(BP_VAR_R, 0 TSRMLS_CC);
				} else {
					zend_do_end_variable_parse(BP_VAR_FUNC_ARG, offset TSRMLS_CC);
				}
				break;
			case ZEND_SEND_REF:
				zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
				break;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	if (op == ZEND_SEND_VAR_NO_REF) {
		if (function_ptr) {
			opline->extended_value = ZEND_ARG_COMPILE_TIME_BOUND | send_by_reference | send_function;
		} else {
			opline->extended_value = send_function;
		}
	} else {
		if (function_ptr) {
			opline->extended_value = ZEND_DO_FCALL;
		} else {
			opline->extended_value = ZEND_DO_FCALL_BY_NAME;
		}
	}
	opline->opcode = op;
	opline->op1 = *param;
	opline->op2.u.opline_num = offset;
	SET_UNUSED(opline->op2);
}


static int generate_free_switch_expr(zend_switch_entry *switch_entry TSRMLS_DC)
{
	zend_op *opline;

	if (switch_entry->cond.op_type != IS_VAR && switch_entry->cond.op_type != IS_TMP_VAR) {
		return (switch_entry->cond.op_type == IS_UNUSED);
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_SWITCH_FREE;
	opline->op1 = switch_entry->cond;
	SET_UNUSED(opline->op2);
	opline->extended_value = 0;
	return 0;
}

static int generate_free_foreach_copy(zend_op *foreach_copy TSRMLS_DC)
{
	zend_op *opline;

	/* If we reach the seperator then stop applying the stack */
	if (foreach_copy->result.op_type == IS_UNUSED && foreach_copy->op1.op_type == IS_UNUSED) {
		return 1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_SWITCH_FREE;
	opline->op1 = foreach_copy->result;
	SET_UNUSED(opline->op2);
	opline->extended_value = 1;

	if (foreach_copy->op1.op_type != IS_UNUSED) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_SWITCH_FREE;
		opline->op1 = foreach_copy->op1;
		SET_UNUSED(opline->op2);
		opline->extended_value = 0;
	}

	return 0;
}

void zend_do_return(znode *expr, int do_end_vparse TSRMLS_DC)
{
	zend_op *opline;

	if (do_end_vparse) {
		if (CG(active_op_array)->return_reference && !zend_is_function_or_method_call(expr)) {
			zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
		} else {
			zend_do_end_variable_parse(BP_VAR_R, 0 TSRMLS_CC);
		}
	}

#ifdef ZTS
	zend_stack_apply_with_argument(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_switch_expr TSRMLS_CC);
	zend_stack_apply_with_argument(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element, void *)) generate_free_foreach_copy TSRMLS_CC);
#else
	zend_stack_apply(&CG(switch_cond_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_switch_expr);
	zend_stack_apply(&CG(foreach_copy_stack), ZEND_STACK_APPLY_TOPDOWN, (int (*)(void *element)) generate_free_foreach_copy);
#endif

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_RETURN;

	if (expr) {
		opline->op1 = *expr;
	} else {
		opline->op1.op_type = IS_CONST;
		INIT_ZVAL(opline->op1.u.constant);
	}

	if (do_end_vparse) {
		if (zend_is_function_or_method_call(expr)) {
			opline->extended_value = ZEND_RETURNS_FUNCTION;
		} else {
			opline->extended_value = 0;
		}
	} else if (CG(active_op_array)->return_reference &&
	           expr && expr->u.EA.type == ZEND_PARSED_NEW) {
		opline->extended_value = ZEND_RETURNS_NEW;
		zend_error(E_STRICT, "Returning the return value of new by reference is deprecated");
	}

	SET_UNUSED(opline->op2);
}


static int zend_add_try_element(zend_uint try_op TSRMLS_DC)
{
	int try_catch_offset = CG(active_op_array)->last_try_catch++;

	CG(active_op_array)->try_catch_array = erealloc(CG(active_op_array)->try_catch_array, sizeof(zend_try_catch_element)*CG(active_op_array)->last_try_catch);
	CG(active_op_array)->try_catch_array[try_catch_offset].try_op = try_op;
	return try_catch_offset;
}

static void zend_add_catch_element(int offset, zend_uint catch_op TSRMLS_DC)
{
	CG(active_op_array)->try_catch_array[offset].catch_op = catch_op;
}


void zend_do_first_catch(znode *open_parentheses TSRMLS_DC)
{
	open_parentheses->u.opline_num = get_next_op_number(CG(active_op_array));
}


void zend_initialize_try_catch_element(znode *try_token TSRMLS_DC)
{
	zend_add_catch_element(try_token->u.opline_num, get_next_op_number(CG(active_op_array)) TSRMLS_CC);
}


void zend_do_mark_last_catch(znode *first_catch, znode *last_additional_catch TSRMLS_DC)
{
	if (last_additional_catch->u.opline_num == -1) {
		CG(active_op_array)->opcodes[first_catch->u.opline_num].op1.u.EA.type = 1;
	} else {
		CG(active_op_array)->opcodes[last_additional_catch->u.opline_num].op1.u.EA.type = 1;
	}
	DEC_BPC(CG(active_op_array));
}


void zend_do_try(znode *try_token TSRMLS_DC)
{
	try_token->u.opline_num = zend_add_try_element(get_next_op_number(CG(active_op_array)) TSRMLS_CC);
	INC_BPC(CG(active_op_array));
}


void zend_do_begin_catch(znode *try_token, znode *catch_class, znode *catch_var, zend_bool first_catch TSRMLS_DC)
{
	long catch_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (catch_op_number > 0) {
		opline = &CG(active_op_array)->opcodes[catch_op_number-1];
		if (opline->opcode == ZEND_FETCH_CLASS) {
			opline->extended_value |= ZEND_FETCH_CLASS_NO_AUTOLOAD;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_CATCH;
	opline->op1 = *catch_class;
/*	SET_UNUSED(opline->op1); */ /* FIXME: Define IS_CLASS or something like that */
	opline->op2 = *catch_var;
	opline->op1.u.EA.type = 0; /* 1 means it's the last catch in the block */

	try_token->u.opline_num = catch_op_number;
}

void zend_do_end_catch(znode *try_token TSRMLS_DC)
{
	CG(active_op_array)->opcodes[try_token->u.opline_num].extended_value = get_next_op_number(CG(active_op_array));
}

void zend_do_throw(znode *expr TSRMLS_DC)
{
	zend_op *opline;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_THROW;
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
}

ZEND_API void function_add_ref(zend_function *function TSRMLS_DC)
{
	if (function->type == ZEND_USER_FUNCTION) {
		zend_op_array *op_array = &function->op_array;

		(*op_array->refcount)++;
		if (op_array->static_variables) {
			HashTable *static_variables = op_array->static_variables;
			zval *tmp_zval;

			ALLOC_HASHTABLE(op_array->static_variables);
			zend_u_hash_init(op_array->static_variables, 2, NULL, ZVAL_PTR_DTOR, 0, UG(unicode));
			zend_hash_copy(op_array->static_variables, static_variables, (copy_ctor_func_t) zval_add_ref, (void *) &tmp_zval, sizeof(zval *));
		}
	}
}

static void do_inherit_parent_constructor(zend_class_entry *ce TSRMLS_DC)
{
	zend_function *function;

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
	if (ce->constructor) {
		if (ce->parent->constructor && ce->parent->constructor->common.fn_flags & ZEND_ACC_FINAL) {
			zend_error(E_ERROR, "Cannot override final %v::%v() with %v::%v()",
				ce->parent->name, ce->parent->constructor->common.function_name,
				ce->name, ce->constructor->common.function_name
				);
		}
		return;
	}

	if (zend_hash_find(&ce->parent->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME), (void **)&function)==SUCCESS) {
		/* inherit parent's constructor */
		zend_hash_update(&ce->function_table, ZEND_CONSTRUCTOR_FUNC_NAME, sizeof(ZEND_CONSTRUCTOR_FUNC_NAME), function, sizeof(zend_function), NULL);
		function_add_ref(function TSRMLS_CC);
	} else {
		/* Don't inherit the old style constructor if we already have the new style constructor */
		unsigned int lc_class_name_len, lc_parent_class_name_len;
		zstr lc_class_name;
		zstr lc_parent_class_name;
		zend_uchar utype = UG(unicode)?IS_UNICODE:IS_STRING;

			lc_class_name = zend_u_str_case_fold(utype, ce->name, ce->name_length, 0, &lc_class_name_len);
			if (!zend_u_hash_exists(&ce->function_table, utype, lc_class_name, lc_class_name_len+1)) {
				lc_parent_class_name = zend_u_str_case_fold(utype, ce->parent->name, ce->parent->name_length, 0, &lc_parent_class_name_len);
				if (zend_u_hash_find(&ce->parent->function_table, utype, lc_parent_class_name, lc_parent_class_name_len+1, (void **)&function)==SUCCESS) {
				if (function->common.fn_flags & ZEND_ACC_CTOR) {
					/* inherit parent's constructor */
					zend_u_hash_update(&ce->function_table, utype, lc_class_name, lc_class_name_len+1, function, sizeof(zend_function), NULL);
					function_add_ref(function TSRMLS_CC);
				}
			}
			efree(lc_parent_class_name.v);
		}
		efree(lc_class_name.v);
	}
	ce->constructor = ce->parent->constructor;
}


char *zend_visibility_string(zend_uint fn_flags)
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


static void do_inherit_method(zend_function *function)
{
	/* The class entry of the derived function intentionally remains the same
	 * as that of the parent class.  That allows us to know in which context
	 * we're running, and handle private method calls properly.
	 */
	TSRMLS_FETCH();
	function_add_ref(function TSRMLS_CC);
}


static zend_bool zend_do_perform_implementation_check(zend_function *fe, zend_function *proto TSRMLS_DC)
{
	zend_uint i;

	/* If it's a user function then arg_info == NULL means we don't have any parameters but we still need to do the arg number checks.  We are only willing to ignore this for internal functions because extensions don't always define arg_info. */
	if (!proto || (!proto->common.arg_info && proto->common.type != ZEND_USER_FUNCTION)) {
		return 1;
	}

	/* Checks for constructors only if they are declared in an interface */
	if ((fe->common.fn_flags & ZEND_ACC_CTOR) && !(proto->common.scope->ce_flags & ZEND_ACC_INTERFACE)) {
		return 1;
	}

	/* check number of arguments */
	if (proto->common.required_num_args != fe->common.required_num_args
		|| proto->common.num_args > fe->common.num_args) {
		return 0;
	}

	if (proto->common.pass_rest_by_reference
		&& !fe->common.pass_rest_by_reference) {
		return 0;
	}

	if (fe->common.return_reference != proto->common.return_reference) {
		return 0;
	}

	for (i=0; i < proto->common.num_args; i++) {
		if (ZEND_LOG_XOR(fe->common.arg_info[i].class_name.v, proto->common.arg_info[i].class_name.v)) {
			/* Only one has a type hint and the other one doesn't */
			return 0;
		}
		if (fe->common.arg_info[i].class_name.v &&
			(fe->common.arg_info[i].class_name_len != proto->common.arg_info[i].class_name_len ||
			 (!UG(unicode) && zend_binary_strcasecmp(fe->common.arg_info[i].class_name.s, fe->common.arg_info[i].class_name_len, proto->common.arg_info[i].class_name.s, proto->common.arg_info[i].class_name_len) != 0) ||
			 (UG(unicode) && zend_u_binary_strcasecmp(fe->common.arg_info[i].class_name.u, fe->common.arg_info[i].class_name_len, proto->common.arg_info[i].class_name.u, proto->common.arg_info[i].class_name_len) != 0))) {
			return 0;
		}
		if (fe->common.arg_info[i].array_type_hint != proto->common.arg_info[i].array_type_hint) {
			/* Only one has an array type hint and the other one doesn't */
			return 0;
		}
		if (fe->common.arg_info[i].pass_by_reference != proto->common.arg_info[i].pass_by_reference) {
			return 0;
		}
	}

	if (proto->common.pass_rest_by_reference) {
		for (i=proto->common.num_args; i < fe->common.num_args; i++) {
			if (!fe->common.arg_info[i].pass_by_reference) {
				return 0;
			}
		}
	}
	return 1;
}


static zend_bool do_inherit_method_check(HashTable *child_function_table, zend_function *parent, zend_hash_key *hash_key, zend_class_entry *child_ce)
{
	zend_uint child_flags;
	zend_uint parent_flags = parent->common.fn_flags;
	zend_function *child;
	TSRMLS_FETCH();

	if (zend_u_hash_quick_find(child_function_table, hash_key->type, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child)==FAILURE) {
		if (parent_flags & (ZEND_ACC_ABSTRACT)) {
			child_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
		}
		return 1; /* method doesn't exist in child, copy from parent */
	}

	if (parent->common.fn_flags & ZEND_ACC_ABSTRACT
		&& parent->common.scope != (child->common.prototype ? child->common.prototype->common.scope : child->common.scope)
		&& child->common.fn_flags & (ZEND_ACC_ABSTRACT|ZEND_ACC_IMPLEMENTED_ABSTRACT)) {
		zend_error(E_COMPILE_ERROR, "Can't inherit abstract function %v::%v() (previously declared abstract in %v)",
			parent->common.scope->name,
			child->common.function_name,
			child->common.prototype ? child->common.prototype->common.scope->name : child->common.scope->name);
	}

	if (parent_flags & ZEND_ACC_FINAL) {
		zend_error(E_COMPILE_ERROR, "Cannot override final method %v::%v()", ZEND_FN_SCOPE_NAME(parent), child->common.function_name.v);
	}

	child_flags	= child->common.fn_flags;
	/* You cannot change from static to non static and vice versa.
	 */
	if ((child_flags & ZEND_ACC_STATIC) != (parent_flags & ZEND_ACC_STATIC)) {
		if (child->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Cannot make non static method %v::%v() static in class %v", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
		} else {
			zend_error(E_COMPILE_ERROR, "Cannot make static method %v::%v() non static in class %v", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
		}
	}

	/* Disallow making an inherited method abstract. */
	if ((child_flags & ZEND_ACC_ABSTRACT) && !(parent_flags & ZEND_ACC_ABSTRACT)) {
		zend_error(E_COMPILE_ERROR, "Cannot make non abstract method %v::%v() abstract in class %v", ZEND_FN_SCOPE_NAME(parent), child->common.function_name, ZEND_FN_SCOPE_NAME(child));
	}

	if (parent_flags & ZEND_ACC_CHANGED) {
		child->common.fn_flags |= ZEND_ACC_CHANGED;
	} else {
		/* Prevent derived classes from restricting access that was available in parent classes
		 */
		if ((child_flags & ZEND_ACC_PPP_MASK) > (parent_flags & ZEND_ACC_PPP_MASK)) {
			zend_error(E_COMPILE_ERROR, "Access level to %v::%v() must be %s (as in class %v)%s", ZEND_FN_SCOPE_NAME(child), child->common.function_name, zend_visibility_string(parent_flags), ZEND_FN_SCOPE_NAME(parent), (parent_flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if (((child_flags & ZEND_ACC_PPP_MASK) < (parent_flags & ZEND_ACC_PPP_MASK))
			&& ((parent_flags & ZEND_ACC_PPP_MASK) & ZEND_ACC_PRIVATE)) {
			child->common.fn_flags |= ZEND_ACC_CHANGED;
		}
	}

	if (parent_flags & ZEND_ACC_ABSTRACT) {
		child->common.fn_flags |= ZEND_ACC_IMPLEMENTED_ABSTRACT;
		child->common.prototype = parent;
	} else {
		child->common.prototype = parent->common.prototype;
	}


	if (child->common.prototype) {
		if (!zend_do_perform_implementation_check(child, child->common.prototype TSRMLS_CC)) {
			zend_error(E_COMPILE_ERROR, "Declaration of %v::%v() must be compatible with that of %v::%v()", ZEND_FN_SCOPE_NAME(child), child->common.function_name, ZEND_FN_SCOPE_NAME(child->common.prototype), child->common.prototype->common.function_name);
		}
	} else if (EG(error_reporting) & E_STRICT) { /* Check E_STRICT before the check so that we save some time */
		if (!zend_do_perform_implementation_check(child, parent TSRMLS_CC)) {
			zend_error(E_STRICT, "Declaration of %v::%v() should be compatible with that of %v::%v()", ZEND_FN_SCOPE_NAME(child), child->common.function_name, ZEND_FN_SCOPE_NAME(parent), parent->common.function_name);
		}
	}

	return 0;
}


static zend_bool do_inherit_property_access_check(HashTable *target_ht, zend_property_info *parent_info, zend_hash_key *hash_key, zend_class_entry *ce)
{
	zend_property_info *child_info;
	zend_class_entry *parent_ce = ce->parent;
	zend_uchar utype;
	TSRMLS_FETCH();

	utype = UG(unicode)?IS_UNICODE:IS_STRING;
	if (parent_info->flags & (ZEND_ACC_PRIVATE|ZEND_ACC_SHADOW)) {
		if (zend_u_hash_quick_find(&ce->properties_info, hash_key->type, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child_info)==SUCCESS) {
			child_info->flags |= ZEND_ACC_CHANGED;
		} else {
			zend_u_hash_quick_update(&ce->properties_info, hash_key->type, hash_key->arKey, hash_key->nKeyLength, hash_key->h, parent_info, sizeof(zend_property_info), (void **) &child_info);
			if(ce->type & ZEND_INTERNAL_CLASS) {
				zend_duplicate_property_info_internal(child_info);
			} else {
				zend_duplicate_property_info(child_info);
			}
			child_info->flags &= ~ZEND_ACC_PRIVATE; /* it's not private anymore */
			child_info->flags |= ZEND_ACC_SHADOW; /* but it's a shadow of private */
		}
		return 0; /* don't copy access information to child */
	}

	if (zend_u_hash_quick_find(&ce->properties_info, hash_key->type, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void **) &child_info)==SUCCESS) {
		if ((parent_info->flags & ZEND_ACC_STATIC) != (child_info->flags & ZEND_ACC_STATIC)) {
			zend_error(E_COMPILE_ERROR, "Cannot redeclare %s%v::$%R as %s%v::$%R",
				(parent_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", parent_ce->name, hash_key->type, hash_key->arKey,
				(child_info->flags & ZEND_ACC_STATIC) ? "static " : "non static ", ce->name, hash_key->type, hash_key->arKey);
		}

		if(parent_info->flags & ZEND_ACC_CHANGED) {
			child_info->flags |= ZEND_ACC_CHANGED;
		}

		if ((child_info->flags & ZEND_ACC_PPP_MASK) > (parent_info->flags & ZEND_ACC_PPP_MASK)) {
			zend_error(E_COMPILE_ERROR, "Access level to %v::$%R must be %s (as in class %v)%s", ce->name, hash_key->type, hash_key->arKey, zend_visibility_string(parent_info->flags), parent_ce->name, (parent_info->flags&ZEND_ACC_PUBLIC) ? "" : " or weaker");
		} else if (child_info->flags & ZEND_ACC_IMPLICIT_PUBLIC) {
			if (!(parent_info->flags & ZEND_ACC_IMPLICIT_PUBLIC)) {
				/* Explicitly copy the default value from the parent (if it has one) */
				zval **pvalue;

				if (zend_u_hash_quick_find(&parent_ce->default_properties, utype, parent_info->name, parent_info->name_length+1, parent_info->h, (void **) &pvalue) == SUCCESS) {
					(*pvalue)->refcount++;
					zend_u_hash_del(&ce->default_properties, utype, child_info->name, child_info->name_length+1);
					zend_u_hash_quick_update(&ce->default_properties, utype, parent_info->name, parent_info->name_length+1, parent_info->h, pvalue, sizeof(zval *), NULL);
				}
			}
			return 1; /* Inherit from the parent */
		} else if ((child_info->flags & ZEND_ACC_PUBLIC) && (parent_info->flags & ZEND_ACC_PROTECTED)) {
			zstr prot_name;
			int prot_name_length;

			zend_u_mangle_property_name(&prot_name, &prot_name_length, utype, ZSTR("*"), 1, child_info->name, child_info->name_length, ce->type & ZEND_INTERNAL_CLASS);
			if (child_info->flags & ZEND_ACC_STATIC) {
				zval **prop;
				HashTable *ht;

				if (parent_ce->type != ce->type) {
					/* User class extends internal class */
					ht = CE_STATIC_MEMBERS(parent_ce);
				} else {
					ht = &parent_ce->default_static_members;
				}
				if (zend_u_hash_find(ht, utype, prot_name, prot_name_length+1, (void**)&prop) == SUCCESS) {
					zval **new_prop;
					if (zend_u_hash_find(&ce->default_static_members, utype, child_info->name, child_info->name_length+1, (void**)&new_prop) == SUCCESS) {
						if (Z_TYPE_PP(new_prop) != IS_NULL && Z_TYPE_PP(prop) != IS_NULL) {
							zstr prop_name, tmp;

							zend_u_unmangle_property_name(utype, child_info->name, &tmp, &prop_name);
							zend_error(E_COMPILE_ERROR, "Cannot change initial value of property static protected %v::$%v in class %v",
								parent_ce->name, prop_name, ce->name);
						}
					}
					(*prop)->refcount++;
					zend_u_hash_update(&ce->default_static_members, utype, child_info->name, child_info->name_length+1, (void**)prop, sizeof(zval*), NULL);
					zend_u_hash_del(&ce->default_static_members, utype, prot_name, prot_name_length+1);
				}
			} else {
				zend_u_hash_del(&ce->default_properties, utype, prot_name, prot_name_length+1);
			}
			pefree(prot_name.v, ce->type & ZEND_INTERNAL_CLASS);
		}
		return 0;	/* Don't copy from parent */
	} else {
		return 1;	/* Copy from parent */
	}
}



static inline void do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC)
{
	if (!(ce->ce_flags & ZEND_ACC_INTERFACE) && iface->interface_gets_implemented && iface->interface_gets_implemented(iface, ce TSRMLS_CC) == FAILURE) {
		zend_error(E_CORE_ERROR, "Class %v could not implement interface %v", ce->name, iface->name);
	}
	if (ce == iface) {
		zend_error(E_ERROR, "Interface %v cannot not implement itself", ce->name);
	}
}


ZEND_API void zend_do_inherit_interfaces(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC)
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


static void inherit_static_prop(zval **p)
{
	(*p)->refcount++;
	(*p)->is_ref = 1;
}


ZEND_API void zend_do_inheritance(zend_class_entry *ce, zend_class_entry *parent_ce TSRMLS_DC)
{
	if ((ce->ce_flags & ZEND_ACC_INTERFACE)
		&& !(parent_ce->ce_flags & ZEND_ACC_INTERFACE)) {
		zend_error(E_COMPILE_ERROR, "Interface %v may not inherit from class (%v)", ce->name, parent_ce->name);
	}
	if (parent_ce->ce_flags & ZEND_ACC_FINAL_CLASS) {
		zend_error(E_COMPILE_ERROR, "Class %v may not inherit from final class (%v)", ce->name, parent_ce->name);
	}

	ce->parent = parent_ce;
	/* Inherit interfaces */
	zend_do_inherit_interfaces(ce, parent_ce TSRMLS_CC);

	/* Inherit properties */
	zend_hash_merge(&ce->default_properties, &parent_ce->default_properties, (void (*)(void *)) zval_add_ref, NULL, sizeof(zval *), 0);
	if (parent_ce->type != ce->type) {
		/* User class extends internal class */
		zend_update_class_constants(parent_ce  TSRMLS_CC);
		zend_hash_merge(&ce->default_static_members, CE_STATIC_MEMBERS(parent_ce), (void (*)(void *)) inherit_static_prop, NULL, sizeof(zval *), 0);
	} else {
		zend_hash_merge(&ce->default_static_members, &parent_ce->default_static_members, (void (*)(void *)) inherit_static_prop, NULL, sizeof(zval *), 0);
	}
	zend_hash_merge_ex(&ce->properties_info, &parent_ce->properties_info, (copy_ctor_func_t) (ce->type & ZEND_INTERNAL_CLASS ? zend_duplicate_property_info_internal : zend_duplicate_property_info), sizeof(zend_property_info), (merge_checker_func_t) do_inherit_property_access_check, ce);

	zend_hash_merge(&ce->constants_table, &parent_ce->constants_table, (void (*)(void *)) zval_add_ref, NULL, sizeof(zval *), 0);
	zend_hash_merge_ex(&ce->function_table, &parent_ce->function_table, (copy_ctor_func_t) do_inherit_method, sizeof(zend_function), (merge_checker_func_t) do_inherit_method_check, ce);
	do_inherit_parent_constructor(ce TSRMLS_CC);

	if (ce->ce_flags & ZEND_ACC_IMPLICIT_ABSTRACT_CLASS && ce->type == ZEND_INTERNAL_CLASS) {
		ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;
	} else {
		zend_verify_abstract_class(ce TSRMLS_CC);
	}
}


static zend_bool do_inherit_constant_check(HashTable *child_constants_table, zval **parent_constant, zend_hash_key *hash_key, zend_class_entry *iface)
{
	zval **old_constant;

	if (zend_u_hash_quick_find(child_constants_table, hash_key->type, hash_key->arKey, hash_key->nKeyLength, hash_key->h, (void**)&old_constant) == SUCCESS) {
	  if (*old_constant != *parent_constant) {
			zend_error(E_COMPILE_ERROR, "Cannot inherit previously-inherited constant %R from interface %v", hash_key->type, hash_key->arKey, iface->name);
		}
		return 0;
	}
	return 1;
}


ZEND_API void zend_do_implement_interface(zend_class_entry *ce, zend_class_entry *iface TSRMLS_DC)
{
	zend_hash_merge_ex(&ce->constants_table, &iface->constants_table, (copy_ctor_func_t) zval_add_ref, sizeof(zval *), (merge_checker_func_t) do_inherit_constant_check, iface);
	zend_hash_merge_ex(&ce->function_table, &iface->function_table, (copy_ctor_func_t) do_inherit_method, sizeof(zend_function), (merge_checker_func_t) do_inherit_method_check, ce);

	do_implement_interface(ce, iface TSRMLS_CC);
	zend_do_inherit_interfaces(ce, iface TSRMLS_CC);
}


ZEND_API int do_bind_function(zend_op *opline, HashTable *function_table, zend_bool compile_time)
{
	zend_function *function;

	if (opline->opcode != ZEND_DECLARE_FUNCTION) {
		zend_error(E_COMPILE_ERROR, "Internal compiler error.  Please report!");
	}

	zend_u_hash_find(function_table, Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), (void *) &function);
	if (zend_u_hash_add(function_table, Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant)+1, function, sizeof(zend_function), NULL)==FAILURE) {
		int error_level = compile_time ? E_COMPILE_ERROR : E_ERROR;
		zend_function *function;

		if (zend_u_hash_find(function_table, Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant)+1, (void *) &function)==SUCCESS
			&& function->type==ZEND_USER_FUNCTION
			&& ((zend_op_array *) function)->last>0) {
			zend_error(error_level, "Cannot redeclare %R() (previously declared in %s:%d)",
				Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant),
						((zend_op_array *) function)->filename,
						((zend_op_array *) function)->opcodes[0].lineno);
		} else if (((zend_internal_function *)function)->module) {
			zend_error(error_level, "Cannot redeclare %R() (internal function exists in module %s)",
				Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant),
						((zend_internal_function *)function)->module->name);
		} else {
			zend_error(error_level, "Cannot redeclare %R()", Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant));
		}
		return FAILURE;
	} else {
		(*function->op_array.refcount)++;
		function->op_array.static_variables = NULL; /* NULL out the unbound function */
		return SUCCESS;
	}
}


ZEND_API zend_class_entry *do_bind_class(zend_op *opline, HashTable *class_table, zend_bool compile_time TSRMLS_DC)
{
	zend_class_entry *ce, **pce;

	if (zend_u_hash_find(class_table, Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), (void **) &pce)==FAILURE) {
		zend_error(E_COMPILE_ERROR, "Internal Zend error - Missing class information for %R", Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant));
		return NULL;
	} else {
		ce = *pce;
	}
	ce->refcount++;
	if (zend_u_hash_add(class_table, Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant)+1, &ce, sizeof(zend_class_entry *), NULL)==FAILURE) {
		ce->refcount--;
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error(E_COMPILE_ERROR, "Cannot redeclare class %v", ce->name);
		}
		return NULL;
	} else {
		if (!(ce->ce_flags & ZEND_ACC_INTERFACE)) {
			zend_verify_abstract_class(ce TSRMLS_CC);
		}
		return ce;
	}
}


ZEND_API zend_class_entry *do_bind_inherited_class(zend_op *opline, HashTable *class_table, zend_class_entry *parent_ce, zend_bool compile_time TSRMLS_DC)
{
	zend_class_entry *ce, **pce;
	int found_ce;

	found_ce = zend_u_hash_find(class_table, Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), (void **) &pce);

	if (found_ce == FAILURE) {
		if (!compile_time) {
			/* If we're in compile time, in practice, it's quite possible
			 * that we'll never reach this class declaration at runtime,
			 * so we shut up about it.  This allows the if (!defined('FOO')) { return; }
			 * approach to work.
			 */
			zend_error(E_COMPILE_ERROR, "Cannot redeclare class %v", ce->name);
		}
		return NULL;
	} else {
		ce = *pce;
	}

	if (parent_ce->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error(E_COMPILE_ERROR, "Class %v cannot extend from interface %v", ce->name, parent_ce->name);
	}

	zend_do_inheritance(ce, parent_ce TSRMLS_CC);

	ce->refcount++;

	/* Register the derived class */
	if (zend_u_hash_add(class_table, Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant)+1, pce, sizeof(zend_class_entry *), NULL)==FAILURE) {
		zend_error(E_COMPILE_ERROR, "Cannot redeclare class %v", ce->name);
		ce->refcount--;
		zend_hash_destroy(&ce->function_table);
		zend_hash_destroy(&ce->default_properties);
		zend_hash_destroy(&ce->properties_info);
		zend_hash_destroy(&ce->default_static_members);
		zend_hash_destroy(&ce->constants_table);
		return NULL;
	}
	return ce;
}


void zend_do_early_binding(TSRMLS_D)
{
	zend_op *opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
	HashTable *table;
	zend_bool is_abstract_class = 0;

	while (opline->opcode == ZEND_TICKS && opline > CG(active_op_array)->opcodes) {
		opline--;
	}

	switch (opline->opcode) {
		case ZEND_DECLARE_FUNCTION:
			if (do_bind_function(opline, CG(function_table), 1) == FAILURE) {
				return;
			}
			table = CG(function_table);
			break;
		case ZEND_DECLARE_CLASS:
		case ZEND_DECLARE_INHERITED_CLASS:
			is_abstract_class = 1;
			/* break missing intentionally */
		case ZEND_VERIFY_ABSTRACT_CLASS: {
				zend_op *verify_abstract_class_op = opline;

				if (!is_abstract_class) {
					opline--;
				}
				if (opline->opcode == ZEND_DECLARE_CLASS) {
					if (do_bind_class(opline, CG(class_table), 1 TSRMLS_CC) == NULL) {
						return;
					}
				} else if (opline->opcode == ZEND_DECLARE_INHERITED_CLASS) {
					zval *parent_name = &(opline-1)->op2.u.constant;
					zend_class_entry **pce;

					if (zend_u_lookup_class(Z_TYPE_P(parent_name), Z_UNIVAL_P(parent_name), Z_UNILEN_P(parent_name), &pce TSRMLS_CC) == FAILURE) {
						return;
					}
					if (do_bind_inherited_class(opline, CG(class_table), *pce, 1 TSRMLS_CC) == NULL) {
						return;
					}
					/* clear unnecessary ZEND_FETCH_CLASS opcode */
					if (opline > CG(active_op_array)->opcodes &&
					    (opline-1)->opcode == ZEND_FETCH_CLASS) {
					  zend_op *fetch_class_opline = opline-1;

						zval_dtor(&fetch_class_opline->op2.u.constant);
						fetch_class_opline->opcode = ZEND_NOP;
						memset(&fetch_class_opline->op1, 0, sizeof(znode));
						memset(&fetch_class_opline->op2, 0, sizeof(znode));
						SET_UNUSED(fetch_class_opline->op1);
						SET_UNUSED(fetch_class_opline->op2);
						SET_UNUSED(fetch_class_opline->result);
					}
				} else {
					/* We currently don't early-bind classes that implement interfaces */
					return;
				}
				table = CG(class_table);
				if (!is_abstract_class) {
					/* clear the verify_abstract_class op */
					init_op(verify_abstract_class_op TSRMLS_CC);
					SET_UNUSED(verify_abstract_class_op->op1);
					SET_UNUSED(verify_abstract_class_op->op2);
					verify_abstract_class_op->opcode = ZEND_NOP;
				}
			}

			break;
		case ZEND_ADD_INTERFACE:
			/* We currently don't early-bind classes that implement interfaces */
			return;
		default:
			zend_error(E_COMPILE_ERROR, "Invalid binding type");
			return;
	}

	zend_u_hash_del(table, Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant));
	zval_dtor(&opline->op1.u.constant);
	zval_dtor(&opline->op2.u.constant);
	opline->opcode = ZEND_NOP;
	memset(&opline->op1, 0, sizeof(znode));
	memset(&opline->op2, 0, sizeof(znode));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_boolean_or_begin(znode *expr1, znode *op_token TSRMLS_DC)
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPNZ_EX;
	if (expr1->op_type == IS_TMP_VAR) {
		opline->result = *expr1;
	} else {
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->result.op_type = IS_TMP_VAR;
	}
	opline->op1 = *expr1;
	SET_UNUSED(opline->op2);

	op_token->u.opline_num = next_op_number;

	*expr1 = opline->result;
}


void zend_do_boolean_or_end(znode *result, znode *expr1, znode *expr2, znode *op_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	*result = *expr1; /* we saved the original result in expr1 */
	opline->opcode = ZEND_BOOL;
	opline->result = *result;
	opline->op1 = *expr2;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[op_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
}


void zend_do_boolean_and_begin(znode *expr1, znode *op_token TSRMLS_DC)
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ_EX;
	if (expr1->op_type == IS_TMP_VAR) {
		opline->result = *expr1;
	} else {
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->result.op_type = IS_TMP_VAR;
	}
	opline->op1 = *expr1;
	SET_UNUSED(opline->op2);

	op_token->u.opline_num = next_op_number;

	*expr1 = opline->result;
}


void zend_do_boolean_and_end(znode *result, znode *expr1, znode *expr2, znode *op_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	*result = *expr1; /* we saved the original result in expr1 */
	opline->opcode = ZEND_BOOL;
	opline->result = *result;
	opline->op1 = *expr2;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[op_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
}


void zend_do_do_while_begin(TSRMLS_D)
{
	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}


void zend_do_do_while_end(znode *do_token, znode *expr_open_bracket, znode *expr TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPNZ;
	opline->op1 = *expr;
	opline->op2.u.opline_num = do_token->u.opline_num;
	SET_UNUSED(opline->op2);

	do_end_loop(expr_open_bracket->u.opline_num TSRMLS_CC);

	DEC_BPC(CG(active_op_array));
}


void zend_do_brk_cont(zend_uchar op, znode *expr TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = op;
	opline->op1.u.opline_num = CG(active_op_array)->current_brk_cont;
	SET_UNUSED(opline->op1);
	if (expr) {
		if (expr->op_type != IS_CONST) {
			zend_error(E_COMPILE_ERROR, "'%s' operator with non-constant operand is no longer supported", op == ZEND_BRK ? "break" : "continue");
		} else if (Z_TYPE(expr->u.constant) != IS_LONG || Z_LVAL(expr->u.constant) < 1) {
			zend_error(E_COMPILE_ERROR, "'%s' operator accepts only positive numbers", op == ZEND_BRK ? "break" : "continue");
		}
		opline->op2 = *expr;
	} else {
		Z_TYPE(opline->op2.u.constant) = IS_LONG;
		Z_LVAL(opline->op2.u.constant) = 1;
		INIT_PZVAL(&opline->op2.u.constant);
		opline->op2.op_type = IS_CONST;
	}
}


void zend_do_switch_cond(znode *cond TSRMLS_DC)
{
	zend_switch_entry switch_entry;

	switch_entry.cond = *cond;
	switch_entry.default_case = -1;
	switch_entry.control_var = -1;
	zend_stack_push(&CG(switch_cond_stack), (void *) &switch_entry, sizeof(switch_entry));

	do_begin_loop(TSRMLS_C);

	INC_BPC(CG(active_op_array));
}



void zend_do_switch_end(znode *case_list TSRMLS_DC)
{
	zend_op *opline;
	zend_switch_entry *switch_entry_ptr;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	/* add code to jmp to default case */
	if (switch_entry_ptr->default_case != -1) {
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_JMP;
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
		opline->op1.u.opline_num = switch_entry_ptr->default_case;
	}

	if (case_list->op_type != IS_UNUSED) { /* non-empty switch */
		int next_op_number = get_next_op_number(CG(active_op_array));

		CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
	}

	/* remember break/continue loop information */
	CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].brk = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->current_brk_cont = CG(active_op_array)->brk_cont_array[CG(active_op_array)->current_brk_cont].parent;

	if (switch_entry_ptr->cond.op_type==IS_VAR || switch_entry_ptr->cond.op_type==IS_TMP_VAR) {
		/* emit free for the switch condition*/
		opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_SWITCH_FREE;
		opline->op1 = switch_entry_ptr->cond;
		SET_UNUSED(opline->op2);
	}
	if (switch_entry_ptr->cond.op_type == IS_CONST) {
		zval_dtor(&switch_entry_ptr->cond.u.constant);
	}

	zend_stack_del_top(&CG(switch_cond_stack));

	DEC_BPC(CG(active_op_array));
}


void zend_do_case_before_statement(znode *case_list, znode *case_token, znode *case_expr TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	int next_op_number;
	zend_switch_entry *switch_entry_ptr;
	znode result;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	if (switch_entry_ptr->control_var == -1) {
		switch_entry_ptr->control_var = get_temporary_variable(CG(active_op_array));
	}
	opline->opcode = ZEND_CASE;
	opline->result.u.var = switch_entry_ptr->control_var;
	opline->result.op_type = IS_TMP_VAR;
	opline->op1 = switch_entry_ptr->cond;
	opline->op2 = *case_expr;
	if (opline->op1.op_type == IS_CONST) {
		zval_copy_ctor(&opline->op1.u.constant);
	}
	result = opline->result;

	next_op_number = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_JMPZ;
	opline->op1 = result;
	SET_UNUSED(opline->op2);
	case_token->u.opline_num = next_op_number;

	if (case_list->op_type==IS_UNUSED) {
		return;
	}
	next_op_number = get_next_op_number(CG(active_op_array));
	CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
}


void zend_do_case_after_statement(znode *result, znode *case_token TSRMLS_DC)
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	result->u.opline_num = next_op_number;

	switch (CG(active_op_array)->opcodes[case_token->u.opline_num].opcode) {
		case ZEND_JMP:
			CG(active_op_array)->opcodes[case_token->u.opline_num].op1.u.opline_num = get_next_op_number(CG(active_op_array));
			break;
		case ZEND_JMPZ:
			CG(active_op_array)->opcodes[case_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
			break;
	}
}



void zend_do_default_before_statement(znode *case_list, znode *default_token TSRMLS_DC)
{
	int next_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	zend_switch_entry *switch_entry_ptr;

	zend_stack_top(&CG(switch_cond_stack), (void **) &switch_entry_ptr);

	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	default_token->u.opline_num = next_op_number;

	next_op_number = get_next_op_number(CG(active_op_array));
	switch_entry_ptr->default_case = next_op_number;

	if (case_list->op_type==IS_UNUSED) {
		return;
	}
	CG(active_op_array)->opcodes[case_list->u.opline_num].op1.u.opline_num = next_op_number;
}


void zend_do_begin_class_declaration(znode *class_token, znode *class_name, znode *parent_class_name TSRMLS_DC)
{
	zend_op *opline;
	int doing_inheritance = 0;
	zend_class_entry *new_class_entry = emalloc(sizeof(zend_class_entry));
	unsigned int lcname_len;
	zstr lcname = zend_u_str_case_fold(Z_TYPE(class_name->u.constant), Z_UNIVAL(class_name->u.constant), Z_UNILEN(class_name->u.constant), 0, &lcname_len);

	if (CG(active_class_entry)) {
		zend_error(E_COMPILE_ERROR, "Class declarations may not be nested");
		return;
	}

	if ((lcname_len == sizeof("self")-1 &&
	     ZEND_U_EQUAL(Z_TYPE(class_name->u.constant), lcname, lcname_len, "self", sizeof("self")-1)) ||
	    (lcname_len == sizeof("parent")-1 &&
	     ZEND_U_EQUAL(Z_TYPE(class_name->u.constant), lcname, lcname_len, "parent", sizeof("parent")-1))) {
		efree(lcname.v);
		zend_error(E_COMPILE_ERROR, "Cannot use '%R' as class name as it is reserved", Z_TYPE(class_name->u.constant), Z_UNIVAL(class_name->u.constant));
	}

	new_class_entry->type = ZEND_USER_CLASS;
	new_class_entry->name = Z_UNIVAL(class_name->u.constant);
	new_class_entry->name_length = Z_UNILEN(class_name->u.constant);

	zend_initialize_class_data(new_class_entry, 1 TSRMLS_CC);
	new_class_entry->filename = zend_get_compiled_filename(TSRMLS_C);
	new_class_entry->line_start = class_token->u.opline_num;
	new_class_entry->ce_flags |= class_token->u.EA.type;

	if (parent_class_name && parent_class_name->op_type != IS_UNUSED) {
		switch (parent_class_name->u.EA.type) {
			case ZEND_FETCH_CLASS_SELF:
				zend_error(E_COMPILE_ERROR, "Cannot use 'self' as class name as it is reserved");
				break;
			case ZEND_FETCH_CLASS_PARENT:
				zend_error(E_COMPILE_ERROR, "Cannot use 'parent' as class name as it is reserved");
				break;
			default:
				break;
		}
		doing_inheritance = 1;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->op1.op_type = IS_CONST;
	build_runtime_defined_function_key(&opline->op1.u.constant, Z_TYPE(class_name->u.constant), lcname, lcname_len TSRMLS_CC);

	opline->op2.op_type = IS_CONST;
	Z_TYPE(opline->op2.u.constant) = Z_TYPE(class_name->u.constant);
	opline->op2.u.constant.refcount = 1;

	if (doing_inheritance) {
		opline->extended_value = parent_class_name->u.var;
		opline->opcode = ZEND_DECLARE_INHERITED_CLASS;
	} else {
		opline->opcode = ZEND_DECLARE_CLASS;
	}

	if (Z_TYPE(opline->op2.u.constant) == IS_UNICODE) {
		Z_USTRVAL(opline->op2.u.constant) = lcname.u;
		Z_USTRLEN(opline->op2.u.constant) = lcname_len;
	} else {
		Z_STRVAL(opline->op2.u.constant) = lcname.s;
		Z_STRLEN(opline->op2.u.constant) = lcname_len;
	}

	zend_u_hash_update(CG(class_table), Z_TYPE(opline->op1.u.constant), Z_UNIVAL(opline->op1.u.constant), Z_UNILEN(opline->op1.u.constant), &new_class_entry, sizeof(zend_class_entry *), NULL);
	CG(active_class_entry) = new_class_entry;

	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_CONST;
	CG(implementing_class) = opline->result;

	if (CG(doc_comment)) {
		CG(active_class_entry)->doc_comment = CG(doc_comment);
		CG(active_class_entry)->doc_comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}
}


static void do_verify_abstract_class(TSRMLS_D)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_VERIFY_ABSTRACT_CLASS;
	opline->op1 = CG(implementing_class);
	SET_UNUSED(opline->op2);
}


void zend_do_end_class_declaration(znode *class_token, znode *parent_token TSRMLS_DC)
{
	zend_class_entry *ce = CG(active_class_entry);

	do_inherit_parent_constructor(ce TSRMLS_CC);

	if (ce->constructor) {
		ce->constructor->common.fn_flags |= ZEND_ACC_CTOR;
		if (ce->constructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Constructor %v::%v() cannot be static", ce->name, ce->constructor->common.function_name);
		}
	}
	if (ce->destructor) {
		ce->destructor->common.fn_flags |= ZEND_ACC_DTOR;
		if (ce->destructor->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Destructor %v::%v() cannot be static", ce->name, ce->destructor->common.function_name);
		}
	}
	if (ce->clone) {
		ce->clone->common.fn_flags |= ZEND_ACC_CLONE;
		if (ce->clone->common.fn_flags & ZEND_ACC_STATIC) {
			zend_error(E_COMPILE_ERROR, "Clone method %v::%v() cannot be static", ce->name, ce->clone->common.function_name);
		}
	}

	ce->line_end = zend_get_compiled_lineno(TSRMLS_C);

	/* Inherit interfaces */
	if (ce->num_interfaces > 0) {
		ce->interfaces = (zend_class_entry **) emalloc(sizeof(zend_class_entry *)*ce->num_interfaces);
		memset(ce->interfaces, 0, sizeof(zend_class_entry *)*ce->num_interfaces);
	}
	if (!(ce->ce_flags & (ZEND_ACC_INTERFACE|ZEND_ACC_EXPLICIT_ABSTRACT_CLASS))
		&& ((parent_token->op_type != IS_UNUSED) || (ce->num_interfaces > 0))) {
		zend_verify_abstract_class(ce TSRMLS_CC);
		if (ce->parent || ce->num_interfaces) {
			do_verify_abstract_class(TSRMLS_C);
		}
	}
	CG(active_class_entry) = NULL;
}


void zend_do_implements_interface(znode *interface_znode TSRMLS_DC)
{
	zend_op *opline;

	switch (interface_znode->u.EA.type) {
		case ZEND_FETCH_CLASS_SELF:
			zend_error(E_COMPILE_ERROR, "Cannot use 'self' as interface name as it is reserved");
			break;
		case ZEND_FETCH_CLASS_PARENT:
			zend_error(E_COMPILE_ERROR, "Cannot use 'parent' as interface name as it is reserved");
			break;
		default:
			if (CG(active_op_array)->last > 0) {
				opline = &CG(active_op_array)->opcodes[CG(active_op_array)->last-1];
				if (opline->opcode == ZEND_FETCH_CLASS) {
					opline->extended_value = ZEND_FETCH_CLASS_INTERFACE;
				}
			}
			break;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_ADD_INTERFACE;
	opline->op1 = CG(implementing_class);
	opline->op2 = *interface_znode;
	opline->extended_value = CG(active_class_entry)->num_interfaces++;
}


ZEND_API void zend_mangle_property_name(char **dest, int *dest_length, char *src1, int src1_length, char *src2, int src2_length, int internal)
{
	char *prop_name;
	int prop_name_length;

	prop_name_length = 1 + src1_length + 1 + src2_length;
	prop_name = pemalloc(prop_name_length + 1, internal);
	prop_name[0] = '\0';
	memcpy(prop_name + 1, src1, src1_length+1);
	memcpy(prop_name + 1 + src1_length + 1, src2, src2_length+1);

	*dest = prop_name;
	*dest_length = prop_name_length;
}

ZEND_API void zend_u_mangle_property_name(zstr *dest, int *dest_length, zend_uchar type, zstr src1, int src1_length, zstr src2, int src2_length, int internal)
{
	if (type == IS_UNICODE) {
		UChar *prop_name;
		int prop_name_length;

		prop_name_length = 1 + src1_length + 1 + src2_length;
		prop_name = pemalloc(UBYTES(prop_name_length + 1), internal);
		prop_name[0] = 0;
		if (src1_length == 1 && src1.s[0] == '*') {
			prop_name[1] = '*';
			prop_name[2] = 0;
		} else {
			memcpy(prop_name + 1, src1.s, UBYTES(src1_length+1));
		}
		memcpy(prop_name + 1 + src1_length + 1, src2.s, UBYTES(src2_length+1));

		dest->u = prop_name;
		*dest_length = prop_name_length;
	} else {
		zend_mangle_property_name(&dest->s, dest_length, src1.s, src1_length, src2.s, src2_length, internal);
	}
}

ZEND_API void zend_unmangle_property_name(char *mangled_property, char **class_name, char **prop_name)
{
	*prop_name = *class_name = NULL;

	if (mangled_property[0]!=0) {
		*prop_name = mangled_property;
		return;
	}

	*class_name = mangled_property+1;
	*prop_name = (*class_name)+strlen(*class_name)+1;
}

ZEND_API void zend_u_unmangle_property_name(zend_uchar type, zstr mangled_property, zstr *class_name, zstr *prop_name)
{
	if (type == IS_UNICODE) {
		prop_name->v = class_name->v = NULL;

		if ((mangled_property.u)[0]!=0) {
			*prop_name = mangled_property;
			return;
		}

		class_name->u = mangled_property.u + 1;
		prop_name->u = class_name->u + u_strlen(class_name->u)+1;
		if (class_name->u[0] == '*') {
			class_name->s = "*";
		}
	} else {
		zend_unmangle_property_name(mangled_property.s, &class_name->s, &prop_name->s);
	}
}

void zend_do_declare_property(znode *var_name, znode *value, zend_uint access_type TSRMLS_DC)
{
	zval *property;
	zend_property_info *existing_property_info;
	char *comment = NULL;
	int comment_len = 0;

	if (CG(active_class_entry)->ce_flags & ZEND_ACC_INTERFACE) {
		zend_error(E_COMPILE_ERROR, "Interfaces may not include member variables");
	}

	if (access_type & ZEND_ACC_ABSTRACT) {
		zend_error(E_COMPILE_ERROR, "Properties cannot be declared abstract");
	}

	if (access_type & ZEND_ACC_FINAL) {
		zend_error(E_COMPILE_ERROR, "Cannot declare property %v::$%R final, the final modifier is allowed only for methods",
			CG(active_class_entry)->name, Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant));
	}

	if (zend_u_hash_find(&CG(active_class_entry)->properties_info, Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant), Z_UNILEN(var_name->u.constant)+1, (void **) &existing_property_info)==SUCCESS) {
		if (!(existing_property_info->flags & ZEND_ACC_IMPLICIT_PUBLIC)) {
			zend_error(E_COMPILE_ERROR, "Cannot redeclare %v::$%R", CG(active_class_entry)->name, Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant));
		}
	}
	ALLOC_ZVAL(property);

	if (value) {
		*property = value->u.constant;
	} else {
		INIT_PZVAL(property);
		Z_TYPE_P(property) = IS_NULL;
	}

	if (CG(doc_comment)) {
		comment = CG(doc_comment);
		comment_len = CG(doc_comment_len);
		CG(doc_comment) = NULL;
		CG(doc_comment_len) = 0;
	}

	zend_u_declare_property_ex(CG(active_class_entry), Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant), Z_UNILEN(var_name->u.constant), property, access_type, comment, comment_len TSRMLS_CC);
	efree(Z_STRVAL(var_name->u.constant));
}


void zend_do_declare_class_constant(znode *var_name, znode *value TSRMLS_DC)
{
	zval *property;

	if(Z_TYPE(value->u.constant) == IS_CONSTANT_ARRAY) {
		zend_error(E_COMPILE_ERROR, "Arrays are not allowed in class constants");
	}

	ALLOC_ZVAL(property);
	*property = value->u.constant;

	if (zend_u_hash_add(&CG(active_class_entry)->constants_table, Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant), Z_UNILEN(var_name->u.constant)+1, &property, sizeof(zval *), NULL)==FAILURE) {
		FREE_ZVAL(property);
		zend_error(E_COMPILE_ERROR, "Cannot redefine class constant %v::%R", CG(active_class_entry)->name, Z_TYPE(var_name->u.constant), Z_UNIVAL(var_name->u.constant));
	}
	FREE_PNODE(var_name);
}



void zend_do_fetch_property(znode *result, znode *object, znode *property TSRMLS_DC)
{
	zend_op opline;
	zend_llist *fetch_list_ptr;
	zend_op *opline_ptr=NULL;

	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	if (fetch_list_ptr->count == 1) {
		zend_llist_element *le;

		le = fetch_list_ptr->head;
		opline_ptr = (zend_op *) le->data;

		if (opline_is_fetch_this(opline_ptr TSRMLS_CC)) {
			efree(Z_STRVAL(opline_ptr->op1.u.constant));
			SET_UNUSED(opline_ptr->op1); /* this means $this for objects */
			opline_ptr->op2 = *property;
			/* if it was usual fetch, we change it to object fetch */
			switch (opline_ptr->opcode) {
				case ZEND_FETCH_W:
					opline_ptr->opcode = ZEND_FETCH_OBJ_W;
					break;
				case ZEND_FETCH_R:
					opline_ptr->opcode = ZEND_FETCH_OBJ_R;
					break;
				case ZEND_FETCH_RW:
					opline_ptr->opcode = ZEND_FETCH_OBJ_RW;
					break;
				case ZEND_FETCH_IS:
					opline_ptr->opcode = ZEND_FETCH_OBJ_IS;
					break;
				case ZEND_FETCH_UNSET:
					opline_ptr->opcode = ZEND_FETCH_OBJ_UNSET;
					break;
				case ZEND_FETCH_FUNC_ARG:
					opline_ptr->opcode = ZEND_FETCH_OBJ_FUNC_ARG;
					break;
			}
			*result = opline_ptr->result;
			return;
		}
	}

	init_op(&opline TSRMLS_CC);
	opline.opcode = ZEND_FETCH_OBJ_W;	/* the backpatching routine assumes W */
	opline.result.op_type = IS_VAR;
	opline.result.u.EA.type = 0;
	opline.result.u.var = get_temporary_variable(CG(active_op_array));
	opline.op1 = *object;
	opline.op2 = *property;
	*result = opline.result;

	zend_llist_add_element(fetch_list_ptr, &opline);
}



void zend_do_declare_implicit_property(TSRMLS_D)
{
/* Fixes bug #26182. Not sure why we needed to do this in the first place.
   Has to be checked with Zeev.
*/
#if ANDI_0
	zend_op *opline_ptr;
	zend_llist_element *le;
	zend_llist *fetch_list_ptr;


	zend_stack_top(&CG(bp_stack), (void **) &fetch_list_ptr);

	if (fetch_list_ptr->count != 1) {
		return;
	}

	le = fetch_list_ptr->head;
	opline_ptr = (zend_op *) le->data;

	if (opline_ptr->op1.op_type == IS_UNUSED
		&& CG(active_class_entry)
		&& opline_ptr->op2.op_type == IS_CONST
	  && !zend_u_hash_exists(&CG(active_class_entry)->properties_info, Z_TYPE(opline_ptr->op2.u.constant), Z_UNIVAL(opline_ptr->op2.u.constant), Z_UNILEN(opline_ptr->op2.u.constant)+1)) {
		znode property;

		property = opline_ptr->op2;
		if (Z_TYPE(opline_ptr->op2.u.constant) == IS_UNICODE) {
			Z_USTRVAL(property.u.constant) = eustrndup(Z_USTRVAL(opline_ptr->op2.u.constant), Z_USTRLEN(opline_ptr->op2.u.constant));
		} else {
			Z_STRVAL(property.u.constant) = estrndup(Z_STRVAL(opline_ptr->op2.u.constant), Z_STRLEN(opline_ptr->op2.u.constant));
		}
		zend_do_declare_property(&property, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_IMPLICIT_PUBLIC TSRMLS_CC);
	}
#endif
}


void zend_do_push_object(znode *object TSRMLS_DC)
{
	zend_stack_push(&CG(object_stack), object, sizeof(znode));
}


void zend_do_pop_object(znode *object TSRMLS_DC)
{
	if (object) {
		znode *tmp;

		zend_stack_top(&CG(object_stack), (void **) &tmp);
		*object = *tmp;
	}
	zend_stack_del_top(&CG(object_stack));
}


void zend_do_begin_new_object(znode *new_token, znode *class_type TSRMLS_DC)
{
	zend_op *opline;
	unsigned char *ptr = NULL;

	new_token->u.opline_num = get_next_op_number(CG(active_op_array));
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_NEW;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *class_type;
	SET_UNUSED(opline->op2);

	zend_stack_push(&CG(function_call_stack), (void *) &ptr, sizeof(unsigned char *));
}


void zend_do_end_new_object(znode *result, znode *new_token, znode *argument_list TSRMLS_DC)
{
	znode ctor_result;

	zend_do_end_function_call(NULL, &ctor_result, argument_list, 1, 0 TSRMLS_CC);
	zend_do_free(&ctor_result TSRMLS_CC);

	CG(active_op_array)->opcodes[new_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array));
	*result = CG(active_op_array)->opcodes[new_token->u.opline_num].result;
}

static int zend_constant_ct_subst(znode *result, zval *const_name TSRMLS_DC)
{
	zend_constant *c = NULL;

	if (zend_u_hash_find(EG(zend_constants), Z_TYPE_P(const_name), Z_UNIVAL_P(const_name), Z_UNILEN_P(const_name)+1, (void **) &c) == FAILURE) {
		unsigned int lookup_name_len;
		zstr lookup_name = zend_u_str_case_fold(Z_TYPE_P(const_name), Z_UNIVAL_P(const_name), Z_UNILEN_P(const_name), 1, &lookup_name_len);

		if (zend_u_hash_find(EG(zend_constants), Z_TYPE_P(const_name), lookup_name, lookup_name_len+1, (void **) &c)==SUCCESS) {
			if ((c->flags & CONST_CS) && memcmp(c->name.v, Z_UNIVAL_P(const_name).v, UG(unicode)?UBYTES(Z_USTRLEN_P(const_name)):Z_STRLEN_P(const_name))!=0) {
				c = NULL;
			}
		} else {
			c = NULL;
		}
		efree(lookup_name.v);
	}
	if (c && (c->flags & CONST_CT_SUBST)) {
		zval_dtor(const_name);
		result->op_type = IS_CONST;
		result->u.constant = c->value;
		zval_copy_ctor(&result->u.constant);
		INIT_PZVAL(&result->u.constant);
		return 1;
	}
	return 0;
}

void zend_do_fetch_constant(znode *result, znode *constant_container, znode *constant_name, int mode TSRMLS_DC)
{
	switch (mode) {
		case ZEND_CT:
			if (constant_container) {
				zend_do_fetch_class_name(NULL, constant_container, constant_name TSRMLS_CC);
				*result = *constant_container;
				result->u.constant.type = IS_CONSTANT;
			} else if (!zend_constant_ct_subst(result, &constant_name->u.constant TSRMLS_CC)) {
				*result = *constant_name;
				result->u.constant.type = IS_CONSTANT;
			}
			break;
		case ZEND_RT:
			if (constant_container ||
			    !zend_constant_ct_subst(result, &constant_name->u.constant TSRMLS_CC)) {
				zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

				opline->opcode = ZEND_FETCH_CONSTANT;
				opline->result.op_type = IS_TMP_VAR;
				opline->result.u.var = get_temporary_variable(CG(active_op_array));
				if (constant_container) {
					opline->op1 = *constant_container;
				} else {
					SET_UNUSED(opline->op1);
				}
				opline->op2 = *constant_name;
				*result = opline->result;
			}
			break;
	}
}


void zend_do_shell_exec(znode *result, znode *cmd TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	switch (cmd->op_type) {
		case IS_TMP_VAR:
			opline->opcode = ZEND_SEND_VAL;
			break;
		default:
			opline->opcode = ZEND_SEND_VAR;
			break;
	}
	opline->op1 = *cmd;
	opline->op2.u.opline_num = 0;
	opline->extended_value = ZEND_DO_FCALL;
	SET_UNUSED(opline->op2);

	/* FIXME: exception support not added to this op2 */
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_DO_FCALL;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_VAR;
	Z_STRVAL(opline->op1.u.constant) = estrndup("shell_exec", sizeof("shell_exec")-1);
	Z_STRLEN(opline->op1.u.constant) = sizeof("shell_exec")-1;
	INIT_PZVAL(&opline->op1.u.constant);
	Z_TYPE(opline->op1.u.constant) = IS_STRING;
	opline->op1.op_type = IS_CONST;
	opline->extended_value = 1;
	SET_UNUSED(opline->op2);
	*result = opline->result;
}



void zend_do_init_array(znode *result, znode *expr, znode *offset, zend_bool is_ref TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_INIT_ARRAY;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->result.op_type = IS_TMP_VAR;
	*result = opline->result;
	if (expr) {
		opline->op1 = *expr;
		if (offset) {
			opline->op2 = *offset;
		} else {
			SET_UNUSED(opline->op2);
		}
	} else {
		SET_UNUSED(opline->op1);
		SET_UNUSED(opline->op2);
	}
	opline->extended_value = is_ref;
}


void zend_do_add_array_element(znode *result, znode *expr, znode *offset, zend_bool is_ref TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_ADD_ARRAY_ELEMENT;
	opline->result = *result;
	opline->op1 = *expr;
	if (offset) {
		opline->op2 = *offset;
	} else {
		SET_UNUSED(opline->op2);
	}
	opline->extended_value = is_ref;
}



void zend_do_add_static_array_element(znode *result, znode *offset, znode *expr)
{
	zval *element;
	TSRMLS_FETCH();

	ALLOC_ZVAL(element);
	*element = expr->u.constant;
	if (offset) {
		zend_uchar utype = Z_TYPE(offset->u.constant);

		switch (Z_TYPE(offset->u.constant)) {
			case IS_CONSTANT:
				/* Ugly hack to denote that this value has a constant index */
				Z_TYPE_P(element) |= IS_CONSTANT_INDEX;
				/* break missing intentionally */
				utype = UG(unicode)?IS_UNICODE:IS_STRING;
			case IS_STRING:
			case IS_UNICODE:
				zend_u_symtable_update(Z_ARRVAL(result->u.constant), utype, Z_UNIVAL(offset->u.constant), Z_UNILEN(offset->u.constant)+1, &element, sizeof(zval *), NULL);
				zval_dtor(&offset->u.constant);
				break;
			case IS_NULL:
				zend_symtable_update(Z_ARRVAL(result->u.constant), "", 1, &element, sizeof(zval *), NULL);
				break;
			case IS_LONG:
			case IS_BOOL:
				zend_hash_index_update(Z_ARRVAL(result->u.constant), Z_LVAL(offset->u.constant), &element, sizeof(zval *), NULL);
				break;
			case IS_DOUBLE:
				zend_hash_index_update(Z_ARRVAL(result->u.constant), (long)Z_DVAL(offset->u.constant), &element, sizeof(zval *), NULL);
				break;
			case IS_CONSTANT_ARRAY:
				zend_error(E_ERROR, "Illegal offset type");
				break;
		}
	} else {
		zend_hash_next_index_insert(Z_ARRVAL(result->u.constant), &element, sizeof(zval *), NULL);
	}
}


void zend_do_add_list_element(znode *element TSRMLS_DC)
{
	list_llist_element lle;

	if (element) {
		zend_check_writable_variable(element);

		lle.var = *element;
		zend_llist_copy(&lle.dimensions, &CG(dimension_llist));
		zend_llist_prepend_element(&CG(list_llist), &lle);
	}
	(*((int *)CG(dimension_llist).tail->data))++;
}


void zend_do_new_list_begin(TSRMLS_D)
{
	int current_dimension = 0;
	zend_llist_add_element(&CG(dimension_llist), &current_dimension);
}


void zend_do_new_list_end(TSRMLS_D)
{
	zend_llist_remove_tail(&CG(dimension_llist));
	(*((int *)CG(dimension_llist).tail->data))++;
}


void zend_do_list_init(TSRMLS_D)
{
	zend_stack_push(&CG(list_stack), &CG(list_llist), sizeof(zend_llist));
	zend_stack_push(&CG(list_stack), &CG(dimension_llist), sizeof(zend_llist));
	zend_llist_init(&CG(list_llist), sizeof(list_llist_element), NULL, 0);
	zend_llist_init(&CG(dimension_llist), sizeof(int), NULL, 0);
	zend_do_new_list_begin(TSRMLS_C);
}


void zend_do_list_end(znode *result, znode *expr TSRMLS_DC)
{
	zend_llist_element *le;
	zend_llist_element *dimension;
	zend_op *opline;
	znode last_container;
	int last_op_number;
	zend_op *last_op;

	le = CG(list_llist).head;
	while (le) {
		zend_llist *tmp_dimension_llist = &((list_llist_element *)le->data)->dimensions;
		dimension = tmp_dimension_llist->head;
		while (dimension) {
			opline = get_next_op(CG(active_op_array) TSRMLS_CC);
			if (dimension == tmp_dimension_llist->head) { /* first */
				last_container = *expr;
				switch (expr->op_type) {
					case IS_VAR:
					case IS_CV:
						opline->opcode = ZEND_FETCH_DIM_R;
						break;
					case IS_TMP_VAR:
						opline->opcode = ZEND_FETCH_DIM_TMP_VAR;
						break;
					case IS_CONST: /* fetch_dim_tmp_var will handle this bogus fetch */
						zval_copy_ctor(&expr->u.constant);
						opline->opcode = ZEND_FETCH_DIM_TMP_VAR;
						break;
				}
			} else {
				opline->opcode = ZEND_FETCH_DIM_R;
			}
			opline->result.op_type = IS_VAR;
			opline->result.u.EA.type = 0;
			opline->result.u.var = get_temporary_variable(CG(active_op_array));
			opline->op1 = last_container;
			opline->op2.op_type = IS_CONST;
			Z_TYPE(opline->op2.u.constant) = IS_LONG;
			Z_LVAL(opline->op2.u.constant) = *((int *) dimension->data);
			INIT_PZVAL(&opline->op2.u.constant);
			opline->extended_value = ZEND_FETCH_ADD_LOCK;
			last_container = opline->result;
			dimension = dimension->next;
		}
		((list_llist_element *) le->data)->value = last_container;
		zend_llist_destroy(&((list_llist_element *) le->data)->dimensions);
		zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
		last_op_number = get_next_op_number(CG(active_op_array))-1;
		last_op = &CG(active_op_array)->opcodes[last_op_number];
		zend_do_assign(result, &((list_llist_element *) le->data)->var, &((list_llist_element *) le->data)->value TSRMLS_CC);
		zend_do_free(result TSRMLS_CC);
		le = le->next;
	}
	zend_llist_destroy(&CG(dimension_llist));
	zend_llist_destroy(&CG(list_llist));
	*result = *expr;
	{
		zend_llist *p;

		/* restore previous lists */
		zend_stack_top(&CG(list_stack), (void **) &p);
		CG(dimension_llist) = *p;
		zend_stack_del_top(&CG(list_stack));
		zend_stack_top(&CG(list_stack), (void **) &p);
		CG(list_llist) = *p;
		zend_stack_del_top(&CG(list_stack));
	}
}

void zend_do_fetch_static_variable(znode *varname, znode *static_assignment, int fetch_type TSRMLS_DC)
{
	zval *tmp;
	zend_op *opline;
	znode lval;
	znode result;

	ALLOC_ZVAL(tmp);

	if (static_assignment) {
		*tmp = static_assignment->u.constant;
	} else {
		INIT_ZVAL(*tmp);
	}
	if (!CG(active_op_array)->static_variables) {
		ALLOC_HASHTABLE(CG(active_op_array)->static_variables);
		zend_u_hash_init(CG(active_op_array)->static_variables, 2, NULL, ZVAL_PTR_DTOR, 0, UG(unicode));
	}
	zend_u_hash_update(CG(active_op_array)->static_variables, Z_TYPE(varname->u.constant), Z_UNIVAL(varname->u.constant), Z_UNILEN(varname->u.constant)+1, &tmp, sizeof(zval *), NULL);

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_FETCH_W;		/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *varname;
	SET_UNUSED(opline->op2);
	opline->op2.u.EA.type = fetch_type;
	result = opline->result;

	if (varname->op_type == IS_CONST) {
		zval_copy_ctor(&varname->u.constant);
	}
	fetch_simple_variable(&lval, varname, 0 TSRMLS_CC); /* Relies on the fact that the default fetch is BP_VAR_W */

	zend_do_assign_ref(NULL, &lval, &result TSRMLS_CC);
	CG(active_op_array)->opcodes[CG(active_op_array)->last-1].result.u.EA.type |= EXT_TYPE_UNUSED;

/*	zval_dtor(&varname->u.constant); */
}

void zend_do_fetch_global_variable(znode *varname, znode *static_assignment, int fetch_type TSRMLS_DC)
{
	zend_op *opline;
	znode lval;
	znode result;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_FETCH_W;		/* the default mode must be Write, since fetch_simple_variable() is used to define function arguments */
	opline->result.op_type = IS_VAR;
	opline->result.u.EA.type = 0;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *varname;
	SET_UNUSED(opline->op2);
	opline->op2.u.EA.type = fetch_type;
	result = opline->result;

	if (varname->op_type == IS_CONST) {
		zval_copy_ctor(&varname->u.constant);
	}
	fetch_simple_variable(&lval, varname, 0 TSRMLS_CC); /* Relies on the fact that the default fetch is BP_VAR_W */

	zend_do_assign_ref(NULL, &lval, &result TSRMLS_CC);
	CG(active_op_array)->opcodes[CG(active_op_array)->last-1].result.u.EA.type |= EXT_TYPE_UNUSED;
}


void zend_do_cast(znode *result, znode *expr, int type TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_CAST;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *expr;
	SET_UNUSED(opline->op2);
	opline->extended_value = type;
	*result = opline->result;
}


void zend_do_include_or_eval(int type, znode *result, znode *op1 TSRMLS_DC)
{
	zend_do_extended_fcall_begin(TSRMLS_C);
	{
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_INCLUDE_OR_EVAL;
		opline->result.op_type = IS_VAR;
		opline->result.u.var = get_temporary_variable(CG(active_op_array));
		opline->op1 = *op1;
		SET_UNUSED(opline->op2);
		Z_LVAL(opline->op2.u.constant) = type;
		*result = opline->result;
	}
	zend_do_extended_fcall_end(TSRMLS_C);
}


void zend_do_indirect_references(znode *result, znode *num_references, znode *variable TSRMLS_DC)
{
	int i;

	zend_do_end_variable_parse(BP_VAR_R, 0 TSRMLS_CC);
	zend_do_normalization(variable, variable TSRMLS_CC);
	for (i=1; i<Z_LVAL(num_references->u.constant); i++) {
		fetch_simple_variable_ex(result, variable, 0, ZEND_FETCH_R TSRMLS_CC);
		zend_do_normalization(variable, result TSRMLS_CC);
	}
	zend_do_begin_variable_parse(TSRMLS_C);
	fetch_simple_variable(result, variable, 1 TSRMLS_CC);
}


void zend_do_unset(znode *variable TSRMLS_DC)
{
	zend_op *last_op;

	zend_check_writable_variable(variable);

	if (variable->op_type == IS_CV) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);
		opline->opcode = ZEND_UNSET_VAR;
		opline->op1.op_type = IS_CONST;
		ZVAL_TEXTL(&opline->op1.u.constant,
			CG(active_op_array)->vars[variable->u.var].name,
			CG(active_op_array)->vars[variable->u.var].name_len, 1);
		SET_UNUSED(opline->op2);
		opline->op2.u.EA.type = ZEND_FETCH_LOCAL;
		SET_UNUSED(opline->result);
	} else {
		last_op = &CG(active_op_array)->opcodes[get_next_op_number(CG(active_op_array))-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_UNSET:
				last_op->opcode = ZEND_UNSET_VAR;
				break;
			case ZEND_FETCH_DIM_UNSET:
				last_op->opcode = ZEND_UNSET_DIM;
				break;
			case ZEND_FETCH_OBJ_UNSET:
				last_op->opcode = ZEND_UNSET_OBJ;
				break;

		}
	}
}


void zend_do_isset_or_isempty(int type, znode *result, znode *variable TSRMLS_DC)
{
	zend_op *last_op;

	zend_do_end_variable_parse(BP_VAR_IS, 0 TSRMLS_CC);

	zend_check_writable_variable(variable);

	if (variable->op_type == IS_CV) {
		last_op = get_next_op(CG(active_op_array) TSRMLS_CC);
		last_op->opcode = ZEND_ISSET_ISEMPTY_VAR;
		last_op->op1.op_type = IS_CONST;
		ZVAL_TEXTL(&last_op->op1.u.constant,
			CG(active_op_array)->vars[variable->u.var].name,
			CG(active_op_array)->vars[variable->u.var].name_len, 1);
		SET_UNUSED(last_op->op2);
		last_op->op2.u.EA.type = ZEND_FETCH_LOCAL;
		last_op->result.u.var = get_temporary_variable(CG(active_op_array));
	} else {
		last_op = &CG(active_op_array)->opcodes[get_next_op_number(CG(active_op_array))-1];

		switch (last_op->opcode) {
			case ZEND_FETCH_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_VAR;
				break;
			case ZEND_FETCH_DIM_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_DIM_OBJ;
				break;
			case ZEND_FETCH_OBJ_IS:
				last_op->opcode = ZEND_ISSET_ISEMPTY_PROP_OBJ;
				break;
		}
	}
	last_op->result.op_type = IS_TMP_VAR;
	last_op->extended_value = type;

	*result = last_op->result;
}


void zend_do_instanceof(znode *result, znode *expr, znode *class_znode, int type TSRMLS_DC)
{
	int last_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	if (last_op_number > 0) {
		opline = &CG(active_op_array)->opcodes[last_op_number-1];
		if (opline->opcode == ZEND_FETCH_CLASS) {
			opline->extended_value |= ZEND_FETCH_CLASS_NO_AUTOLOAD;
		}
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_INSTANCEOF;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *expr;

	opline->op2 = *class_znode;

	*result = opline->result;
}


void zend_do_foreach_begin(znode *foreach_token, znode *open_brackets_token, znode *array, int variable TSRMLS_DC)
{
	zend_op *opline;
	zend_bool is_variable;
	zend_bool push_container = 0;

	if (variable) {
		if (zend_is_function_or_method_call(array)) {
			is_variable = 0;
		} else {
			is_variable = 1;
		}
		zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
		if (CG(active_op_array)->last > 0 &&
		    CG(active_op_array)->opcodes[CG(active_op_array)->last-1].opcode == ZEND_FETCH_OBJ_W) {
			/* Only lock the container if we are fetching from a real container and not $this */
			if (CG(active_op_array)->opcodes[CG(active_op_array)->last-1].op1.op_type == IS_VAR) {
				CG(active_op_array)->opcodes[CG(active_op_array)->last-1].extended_value |= ZEND_FETCH_ADD_LOCK;
				push_container = 1;
			}
		}
	} else {
		is_variable = 0;
	}

	/* save the location of FE_RESET */
	foreach_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	/* Preform array reset */
	opline->opcode = ZEND_FE_RESET;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *array;
	SET_UNUSED(opline->op2);
	opline->extended_value = is_variable ? ZEND_FE_RESET_VARIABLE : 0;
	*open_brackets_token = opline->result;

	{
		zend_op dummy_opline;

		dummy_opline.result = opline->result;
		if (push_container) {
			dummy_opline.op1 = CG(active_op_array)->opcodes[CG(active_op_array)->last-2].op1;
		} else {
			znode tmp;

			tmp.op_type = IS_UNUSED;
			dummy_opline.op1 = tmp;
		}
		zend_stack_push(&CG(foreach_copy_stack), (void *) &dummy_opline, sizeof(zend_op));
	}
}


void zend_do_foreach_fetch(znode *foreach_token, znode *open_brackets_token, znode *as_token TSRMLS_DC)
{
	zend_op *opline;

	/* save the location of FE_FETCH */
	as_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_FE_FETCH;
	opline->result.op_type = IS_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *open_brackets_token;
	opline->extended_value = 0;
	SET_UNUSED(opline->op2);

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_OP_DATA;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	SET_UNUSED(opline->result);
}


void zend_do_foreach_cont(znode *foreach_token, znode *as_token, znode *value, znode *key TSRMLS_DC)
{
	zend_op *opline;
	znode dummy, value_node;
	zend_bool assign_by_ref=0;

	opline = &CG(active_op_array)->opcodes[as_token->u.opline_num];
	if (key->op_type != IS_UNUSED) {
		znode *tmp;

		/* switch between the key and value... */
		tmp = key;
		key = value;
		value = tmp;

		/* Mark extended_value in case both key and value are being used */
		opline->extended_value |= ZEND_FE_FETCH_WITH_KEY;
	}

	if ((key->op_type != IS_UNUSED) && (key->u.EA.type & ZEND_PARSED_REFERENCE_VARIABLE)) {
		zend_error(E_COMPILE_ERROR, "Key element cannot be a reference");
	}

	if (value->u.EA.type & ZEND_PARSED_REFERENCE_VARIABLE) {
		assign_by_ref = 1;
		if (!(opline-1)->extended_value) {
			zend_error(E_COMPILE_ERROR, "Cannot create references to elements of a temporary array expression");
		}
		/* Mark extended_value for assign-by-reference */
		opline->extended_value |= ZEND_FE_FETCH_BYREF;
		CG(active_op_array)->opcodes[foreach_token->u.opline_num].extended_value |= ZEND_FE_RESET_REFERENCE;
	}

	value_node = opline->result;

	zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
	if (assign_by_ref) {
		/* Mark FE_FETCH as IS_VAR as it holds the data directly as a value */
		zend_do_assign_ref(NULL, value, &value_node TSRMLS_CC);
	} else {
		zend_do_assign(&dummy, value, &value_node TSRMLS_CC);
		zend_do_free(&dummy TSRMLS_CC);
	}

	if (key->op_type != IS_UNUSED) {
		znode key_node;

		zend_do_end_variable_parse(BP_VAR_W, 0 TSRMLS_CC);
		opline = &CG(active_op_array)->opcodes[as_token->u.opline_num+1];
		opline->result.op_type = IS_TMP_VAR;
		opline->result.u.EA.type = 0;
		opline->result.u.opline_num = get_temporary_variable(CG(active_op_array));
		key_node = opline->result;

		zend_do_assign(&dummy, key, &key_node TSRMLS_CC);
		zend_do_free(&dummy TSRMLS_CC);
	}

	do_begin_loop(TSRMLS_C);
	INC_BPC(CG(active_op_array));
}


void zend_do_foreach_end(znode *foreach_token, znode *as_token TSRMLS_DC)
{
	zend_op *container_ptr;
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMP;
	opline->op1.u.opline_num = as_token->u.opline_num;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[foreach_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array)); /* FE_RESET */
	CG(active_op_array)->opcodes[as_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array)); /* FE_FETCH */

	do_end_loop(as_token->u.opline_num TSRMLS_CC);

	zend_stack_top(&CG(foreach_copy_stack), (void **) &container_ptr);
	generate_free_foreach_copy(container_ptr TSRMLS_CC);
	zend_stack_del_top(&CG(foreach_copy_stack));

	DEC_BPC(CG(active_op_array));
}


void zend_do_declare_begin(TSRMLS_D)
{
	zend_stack_push(&CG(declare_stack), &CG(declarables), sizeof(zend_declarables));
}


void zend_do_declare_stmt(znode *var, znode *val TSRMLS_DC)
{
	if (ZEND_U_EQUAL(Z_TYPE(var->u.constant), Z_UNIVAL(var->u.constant), Z_UNILEN(var->u.constant), "ticks", sizeof("ticks")-1)) {
		convert_to_long(&val->u.constant);
		CG(declarables).ticks = val->u.constant;
	} else if (UG(unicode) && ZEND_U_EQUAL(Z_TYPE(var->u.constant), Z_UNIVAL(var->u.constant), Z_UNILEN(var->u.constant), "encoding", sizeof("encoding")-1)) {
		UErrorCode status = U_ZERO_ERROR;

		if (Z_TYPE(val->u.constant) == IS_CONSTANT) {
			zend_error(E_COMPILE_ERROR, "Cannot use constants as encoding");
		}
		/*
		 * Check that the pragma comes before any opcodes. If the compilation
		 * got as far as this, the previous portion of the script must have been
		 * parseable according to the .ini script_encoding setting. We still
		 * want to tell them to put declare() at the top.
		 */
		if (CG(active_op_array)->last > 0) {
			zend_error(E_COMPILE_ERROR, "Encoding declaration pragma has to be the very first statement in the script");
		}
		convert_to_string(&val->u.constant);
		if (zend_prepare_scanner_converters(Z_STRVAL(val->u.constant), 1 TSRMLS_CC) == FAILURE) {
			zend_error(E_COMPILE_WARNING, "Unsupported encoding [%s]", Z_STRVAL(val->u.constant));
		}
		zend_set_compiled_script_encoding((char*)ucnv_getName(LANG_SCNG(output_conv), &status) TSRMLS_CC);
		/*
		 * Because we require declare(encoding=...) to be the very first thing,
		 * we can safely cache the script encoding in the op array here.
		 */
		CG(active_op_array)->script_encoding = zend_get_compiled_script_encoding(TSRMLS_C);
		efree(Z_STRVAL(val->u.constant));
	} else {
		zval_dtor(&val->u.constant);
	}
	zval_dtor(&var->u.constant);
}


void zend_do_declare_end(znode *declare_token TSRMLS_DC)
{
	zend_declarables *declarables;

	zend_stack_top(&CG(declare_stack), (void **) &declarables);
	/* We should restore if there was more than (current - start) - (ticks?1:0) opcodes */
	if ((get_next_op_number(CG(active_op_array)) - declare_token->u.opline_num) - ((Z_LVAL(CG(declarables).ticks))?1:0)) {
		CG(declarables) = *declarables;
	}
}


void zend_do_end_heredoc(TSRMLS_D)
{
	int opline_num = get_next_op_number(CG(active_op_array))-1;
	zend_op *opline = &CG(active_op_array)->opcodes[opline_num];

	if (opline->opcode != ZEND_ADD_STRING) {
		return;
	}

	if (Z_TYPE(opline->op2.u.constant) == IS_UNICODE) {
		Z_USTRVAL(opline->op2.u.constant)[(Z_USTRLEN(opline->op2.u.constant)--)-1] = 0;
		if (Z_USTRLEN(opline->op2.u.constant)>0) {
			if (Z_USTRVAL(opline->op2.u.constant)[Z_USTRLEN(opline->op2.u.constant)-1]=='\r') {
				Z_USTRVAL(opline->op2.u.constant)[(Z_USTRLEN(opline->op2.u.constant)--)-1] = 0;
			}
		}
	} else {
		Z_STRVAL(opline->op2.u.constant)[(Z_STRLEN(opline->op2.u.constant)--)-1] = 0;
		if (Z_STRLEN(opline->op2.u.constant)>0) {
			if (Z_STRVAL(opline->op2.u.constant)[Z_STRLEN(opline->op2.u.constant)-1]=='\r') {
				Z_STRVAL(opline->op2.u.constant)[(Z_STRLEN(opline->op2.u.constant)--)-1] = 0;
			}
		}
	}
}


void zend_do_exit(znode *result, znode *message TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXIT;
	opline->op1 = *message;
	SET_UNUSED(opline->op2);

	result->op_type = IS_CONST;
	Z_TYPE(result->u.constant) = IS_BOOL;
	Z_LVAL(result->u.constant) = 1;
}

void zend_do_begin_silence(znode *strudel_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_BEGIN_SILENCE;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
	*strudel_token = opline->result;
}


void zend_do_end_silence(znode *strudel_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_END_SILENCE;
	opline->op1 = *strudel_token;
	SET_UNUSED(opline->op2);
}


void zend_do_begin_qm_op(znode *cond, znode *qm_token TSRMLS_DC)
{
	int jmpz_op_number = get_next_op_number(CG(active_op_array));
	zend_op *opline;

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_JMPZ;
	opline->op1 = *cond;
	SET_UNUSED(opline->op2);
	opline->op2.u.opline_num = jmpz_op_number;
	*qm_token = opline->op2;

	INC_BPC(CG(active_op_array));
}


void zend_do_qm_true(znode *true_value, znode *qm_token, znode *colon_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	CG(active_op_array)->opcodes[qm_token->u.opline_num].op2.u.opline_num = get_next_op_number(CG(active_op_array))+1; /* jmp over the ZEND_JMP */

	opline->opcode = ZEND_QM_ASSIGN;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *true_value;
	SET_UNUSED(opline->op2);

	*qm_token = opline->result;
	colon_token->u.opline_num = get_next_op_number(CG(active_op_array));

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_JMP;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_qm_false(znode *result, znode *false_value, znode *qm_token, znode *colon_token TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_QM_ASSIGN;
	opline->result = *qm_token;
	opline->op1 = *false_value;
	SET_UNUSED(opline->op2);

	CG(active_op_array)->opcodes[colon_token->u.opline_num].op1.u.opline_num = get_next_op_number(CG(active_op_array));

	*result = opline->result;

	DEC_BPC(CG(active_op_array));
}


void zend_do_extended_info(TSRMLS_D)
{
	zend_op *opline;

	if (!CG(extended_info)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_STMT;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_extended_fcall_begin(TSRMLS_D)
{
	zend_op *opline;

	if (!CG(extended_info)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_BEGIN;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_extended_fcall_end(TSRMLS_D)
{
	zend_op *opline;

	if (!CG(extended_info)) {
		return;
	}

	opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_EXT_FCALL_END;
	SET_UNUSED(opline->op1);
	SET_UNUSED(opline->op2);
}


void zend_do_ticks(TSRMLS_D)
{
	if (Z_LVAL(CG(declarables).ticks)) {
		zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

		opline->opcode = ZEND_TICKS;
		opline->op1.u.constant = CG(declarables).ticks;
		opline->op1.op_type = IS_CONST;
		SET_UNUSED(opline->op2);
	}
}

void zend_auto_global_dtor(zend_auto_global *auto_global)
{
	free(auto_global->name);
}


zend_bool zend_u_is_auto_global(zend_uchar type, zstr name, uint name_len TSRMLS_DC)
{
	zend_auto_global *auto_global;

	if (zend_u_hash_find(CG(auto_globals), type, name, name_len+1, (void **) &auto_global)==SUCCESS) {
		if (auto_global->armed) {
			auto_global->armed = auto_global->auto_global_callback(auto_global->name, auto_global->name_len TSRMLS_CC);
		}
		return 1;
	}
	return 0;
}

zend_bool zend_is_auto_global(char *name, uint name_len TSRMLS_DC)
{
	return zend_u_is_auto_global(IS_STRING, ZSTR(name), name_len TSRMLS_CC);
}


int zend_register_auto_global(char *name, uint name_len, zend_auto_global_callback auto_global_callback TSRMLS_DC)
{
	zend_auto_global auto_global;

	auto_global.name = zend_strndup(name, name_len);
	auto_global.name_len = name_len;
	auto_global.auto_global_callback = auto_global_callback;

	return zend_hash_add(CG(auto_globals), name, name_len+1, &auto_global, sizeof(zend_auto_global), NULL);
}


int zendlex(znode *zendlval TSRMLS_DC)
{
	int retval;

again:
	if (CG(increment_lineno)) {
		CG(zend_lineno)++;
		CG(increment_lineno) = 0;
	}

	Z_TYPE(zendlval->u.constant) = IS_LONG;
	retval = lex_scan(&zendlval->u.constant TSRMLS_CC);
	switch (retval) {
		case T_COMMENT:
		case T_DOC_COMMENT:
		case T_OPEN_TAG:
		case T_WHITESPACE:
			goto again;

		case T_CLOSE_TAG:
			if (LANG_SCNG(yy_text)[LANG_SCNG(yy_leng)-1]=='\n'
				|| (LANG_SCNG(yy_text)[LANG_SCNG(yy_leng)-2]=='\r' && LANG_SCNG(yy_text)[LANG_SCNG(yy_leng)-1])) {
				CG(increment_lineno) = 1;
			}
			retval = ';'; /* implicit ; */
			break;
		case T_OPEN_TAG_WITH_ECHO:
			retval = T_ECHO;
			break;
		case T_END_HEREDOC:
			efree(Z_STRVAL(zendlval->u.constant));
			break;
	}

	INIT_PZVAL(&zendlval->u.constant);
	zendlval->op_type = IS_CONST;
	return retval;
}


ZEND_API void zend_initialize_class_data(zend_class_entry *ce, zend_bool nullify_handlers TSRMLS_DC)
{
	zend_bool persistent_hashes = (ce->type == ZEND_INTERNAL_CLASS) ? 1 : 0;
	dtor_func_t zval_ptr_dtor_func = ((persistent_hashes) ? ZVAL_INTERNAL_PTR_DTOR : ZVAL_PTR_DTOR);

	ce->refcount = 1;
	ce->constants_updated = 0;
	ce->ce_flags = 0;

	ce->doc_comment = NULL;
	ce->doc_comment_len = 0;

	zend_u_hash_init_ex(&ce->default_properties, 0, NULL, zval_ptr_dtor_func, persistent_hashes, UG(unicode), 0);
	zend_u_hash_init_ex(&ce->properties_info, 0, NULL, (dtor_func_t) (persistent_hashes ? zend_destroy_property_info_internal : zend_destroy_property_info), persistent_hashes, UG(unicode), 0);
	zend_u_hash_init_ex(&ce->default_static_members, 0, NULL, zval_ptr_dtor_func, persistent_hashes, UG(unicode), 0);
	zend_u_hash_init_ex(&ce->constants_table, 0, NULL, zval_ptr_dtor_func, persistent_hashes, UG(unicode), 0);
	zend_u_hash_init_ex(&ce->function_table, 0, NULL, ZEND_FUNCTION_DTOR, persistent_hashes, UG(unicode), 0);

	if (ce->type == ZEND_INTERNAL_CLASS) {
#ifdef ZTS
		int n = zend_hash_num_elements(CG(class_table));

		if (CG(static_members) && n >= CG(last_static_member)) {
			/* Support for run-time declaration: dl() */
			CG(last_static_member) = n+1;
			CG(static_members) = realloc(CG(static_members), (n+1)*sizeof(HashTable*));
			CG(static_members)[n] = NULL;
		}
		ce->static_members = (HashTable*)n;
#else
		ce->static_members = NULL;
#endif
	} else {
		ce->static_members = &ce->default_static_members;
	}

	if (nullify_handlers) {
		ce->constructor = NULL;
		ce->destructor = NULL;
		ce->clone = NULL;
		ce->__get = NULL;
		ce->__set = NULL;
		ce->__unset = NULL;
		ce->__isset = NULL;
		ce->__call = NULL;
		ce->__tostring = NULL;
		ce->create_object = NULL;
		ce->get_iterator = NULL;
		ce->iterator_funcs.funcs = NULL;
		ce->interface_gets_implemented = NULL;
		ce->parent = NULL;
		ce->num_interfaces = 0;
		ce->interfaces = NULL;
		ce->module = NULL;
		ce->serialize = NULL;
		ce->unserialize = NULL;
		ce->serialize_func = NULL;
		ce->unserialize_func = NULL;
		ce->builtin_functions = NULL;
	}
}


int zend_get_class_fetch_type(zend_uchar type, zstr class_name, uint class_name_len)
{
	if ((class_name_len == sizeof("self")-1) &&
	    ZEND_U_EQUAL(type, class_name, class_name_len, "self", sizeof("self")-1)) {
		return ZEND_FETCH_CLASS_SELF;
	} else if ((class_name_len == sizeof("parent")-1) &&
	    ZEND_U_EQUAL(type, class_name, class_name_len, "parent", sizeof("parent")-1)) {
		return ZEND_FETCH_CLASS_PARENT;
	} else {
		return ZEND_FETCH_CLASS_DEFAULT;
	}
}

ZEND_API zstr zend_get_compiled_variable_name(zend_op_array *op_array, zend_uint var, int* name_len)
{
	if (name_len) {
		*name_len = op_array->vars[var].name_len;
	}
	return op_array->vars[var].name;
}

void zend_do_normalization(znode *result, znode *str TSRMLS_DC)
{
	zend_op *opline;

	if (!UG(unicode)) {
		*result = *str;
		return;
	}
	opline = get_next_op(CG(active_op_array) TSRMLS_CC);
	opline->opcode = ZEND_U_NORMALIZE;
	opline->result.op_type = IS_TMP_VAR;
	opline->result.u.var = get_temporary_variable(CG(active_op_array));
	opline->op1 = *str;
	SET_UNUSED(opline->op2);
	*result = opline->result;
}

void zend_do_label(znode *label TSRMLS_DC)
{
	zend_op_array *oparray = CG(active_op_array);
	zend_label dest;

	if (!CG(labels)) {
		ALLOC_HASHTABLE(CG(labels));
		zend_hash_init(CG(labels), 4, NULL, NULL, 0);
	}

	dest.brk_cont = oparray->current_brk_cont;
	dest.opline_num = get_next_op_number(oparray);

	if (zend_u_hash_add(CG(labels), Z_TYPE(label->u.constant), Z_UNIVAL(label->u.constant),
					Z_UNILEN(label->u.constant) + 1, (void**)&dest, sizeof(zend_label), NULL) == FAILURE) {
		zend_error(E_COMPILE_ERROR, "Label '%R' already defined", Z_TYPE(label->u.constant), Z_UNIVAL(label->u.constant));
	}

	/* Done with label now */
	zval_dtor(&label->u.constant);
}

void zend_resolve_goto_label(zend_op_array *op_array, zend_op *opline, int pass2 TSRMLS_DC)
{
	zend_label *dest;
	long current, distance;

	if (CG(labels) == NULL ||
	    zend_u_hash_find(CG(labels), Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant), Z_UNILEN(opline->op2.u.constant)+1, (void**)&dest) == FAILURE) {

	    if (pass2) {
	    	CG(in_compilation) = 1;
	    	CG(active_op_array) = op_array;
	    	CG(zend_lineno) = opline->lineno;
			zend_error(E_COMPILE_ERROR, "'goto' to undefined label '%R'", Z_TYPE(opline->op2.u.constant), Z_UNIVAL(opline->op2.u.constant));
	    } else {
			/* Label is not defined. Delay to pass 2. */
			INC_BPC(op_array);
			return;
		}
	}

	opline->op1.u.opline_num = dest->opline_num;
	zval_dtor(&opline->op2.u.constant);

	/* Check that we are not moving into loop or switch */
	current = opline->extended_value;
	for (distance = 0; current != dest->brk_cont; distance++) {
		if (current == -1) {
		    if (pass2) {
		    	CG(in_compilation) = 1;
	    		CG(active_op_array) = op_array;
	    		CG(zend_lineno) = opline->lineno;
	    	}
			zend_error(E_COMPILE_ERROR, "'goto' into loop or switch statement is disallowed");
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
		ZVAL_LONG(&opline->op2.u.constant, distance);
	}

    if (pass2) {
		DEC_BPC(op_array);
    }
}

void zend_do_goto(znode *label TSRMLS_DC)
{
	zend_op *opline = get_next_op(CG(active_op_array) TSRMLS_CC);

	opline->opcode = ZEND_GOTO;
	opline->extended_value = CG(active_op_array)->current_brk_cont;
	SET_UNUSED(opline->op1);
	opline->op2 = *label;
	zend_resolve_goto_label(CG(active_op_array), opline, 0 TSRMLS_CC);
}

void zend_release_labels(TSRMLS_D)
{
	if (CG(labels)) {
		zend_hash_destroy(CG(labels));
		FREE_HASHTABLE(CG(labels));
	}
	if (!zend_stack_is_empty(&CG(labels_stack))) {
		HashTable **pht;

		zend_stack_top(&CG(labels_stack), (void**)&pht);
		CG(labels) = *pht;
		zend_stack_del_top(&CG(labels_stack));
	} else {
		CG(labels) = NULL;
	}
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
