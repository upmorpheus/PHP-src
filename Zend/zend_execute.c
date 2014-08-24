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

#define ZEND_INTENSIVE_DEBUGGING 0

#include <stdio.h>
#include <signal.h>

#include "zend.h"
#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_API.h"
#include "zend_ptr_stack.h"
#include "zend_constants.h"
#include "zend_extensions.h"
#include "zend_ini.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_closures.h"
#include "zend_generators.h"
#include "zend_vm.h"
#include "zend_dtrace.h"

/* Virtual current working directory support */
#include "zend_virtual_cwd.h"

#define _CONST_CODE  0
#define _TMP_CODE    1
#define _VAR_CODE    2
#define _UNUSED_CODE 3
#define _CV_CODE     4

typedef int (*incdec_t)(zval *);

#define get_zval_ptr(op_type, node, ex, should_free, type) _get_zval_ptr(op_type, node, ex, should_free, type TSRMLS_CC)
#define get_zval_ptr_deref(op_type, node, ex, should_free, type) _get_zval_ptr_deref(op_type, node, ex, should_free, type TSRMLS_CC)
#define get_zval_ptr_ptr(op_type, node, ex, should_free, type) _get_zval_ptr_ptr(op_type, node, ex, should_free, type TSRMLS_CC)
#define get_zval_ptr_ptr_undef(op_type, node, ex, should_free, type) _get_zval_ptr_ptr(op_type, node, ex, should_free, type TSRMLS_CC)
#define get_obj_zval_ptr(op_type, node, ex, should_free, type) _get_obj_zval_ptr(op_type, node, ex, should_free, type TSRMLS_CC)
#define get_obj_zval_ptr_ptr(op_type, node, ex, should_free, type) _get_obj_zval_ptr_ptr(op_type, node, ex, should_free, type TSRMLS_CC)

/* Prototypes */
static void zend_extension_statement_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC);
static void zend_extension_fcall_begin_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC);
static void zend_extension_fcall_end_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC);

#define RETURN_VALUE_USED(opline) (!((opline)->result_type & EXT_TYPE_UNUSED))

#define TEMP_VAR_STACK_LIMIT 2000

static zend_always_inline void zend_pzval_unlock_func(zval *z, zend_free_op *should_free)
{
	should_free->var = NULL;
	if (Z_REFCOUNTED_P(z) && !Z_DELREF_P(z)) {
		Z_SET_REFCOUNT_P(z, 1);
		should_free->var = z;
		/* should_free->is_var = 1; */
	}
}

static ZEND_FUNCTION(pass)
{
}

static const zend_internal_function zend_pass_function = {
	ZEND_INTERNAL_FUNCTION, /* type              */
	NULL,                   /* name              */
	NULL,                   /* scope             */
	0,                      /* fn_flags          */
	NULL,                   /* prototype         */
	0,                      /* num_args          */
	0,                      /* required_num_args */
	NULL,                   /* arg_info          */
	ZEND_FN(pass),          /* handler           */
	NULL                    /* module            */
};

#undef zval_ptr_dtor
#define zval_ptr_dtor(zv) i_zval_ptr_dtor(zv ZEND_FILE_LINE_CC TSRMLS_CC)
#define zval_ptr_dtor_nogc(zv) i_zval_ptr_dtor_nogc(zv ZEND_FILE_LINE_CC TSRMLS_CC)

#define PZVAL_UNLOCK(z, f) zend_pzval_unlock_func(z, f)
#define PZVAL_LOCK(z) if (Z_REFCOUNTED_P(z)) Z_ADDREF_P((z))
#define SELECTIVE_PZVAL_LOCK(pzv, opline)	if (RETURN_VALUE_USED(opline)) { PZVAL_LOCK(pzv); }

#define EXTRACT_ZVAL_PTR(zv) do {						\
		zval *__zv = (zv);								\
		if (Z_TYPE_P(__zv) == IS_INDIRECT) {			\
			ZVAL_COPY(__zv, Z_INDIRECT_P(__zv));		\
		}												\
	} while (0)

#define FREE_OP(should_free) \
	if (should_free.var) { \
		if ((zend_uintptr_t)should_free.var & 1L) { \
			zval_dtor((zval*)((zend_uintptr_t)should_free.var & ~1L)); \
		} else { \
			zval_ptr_dtor_nogc(should_free.var); \
		} \
	}

#define FREE_OP_IF_VAR(should_free) \
	if (should_free.var != NULL && (((zend_uintptr_t)should_free.var & 1L) == 0)) { \
		zval_ptr_dtor_nogc(should_free.var); \
	}

#define FREE_OP_VAR_PTR(should_free) \
	if (should_free.var) { \
		zval_ptr_dtor_nogc(should_free.var); \
	}

#define TMP_FREE(z) (zval*)(((zend_uintptr_t)(z)) | 1L)

#define IS_TMP_FREE(should_free) ((zend_uintptr_t)should_free.var & 1L)

/* End of zend_execute_locks.h */

#define CV_DEF_OF(i) (EX(func)->op_array.vars[i])

#define CTOR_CALL_BIT    0x1
#define CTOR_USED_BIT    0x2

#define IS_CTOR_CALL(ce) (((zend_uintptr_t)(ce)) & CTOR_CALL_BIT)
#define IS_CTOR_USED(ce) (((zend_uintptr_t)(ce)) & CTOR_USED_BIT)

#define ENCODE_CTOR(ce, used) \
	((zend_class_entry*)(((zend_uintptr_t)(ce)) | CTOR_CALL_BIT | ((used) ? CTOR_USED_BIT : 0)))
#define DECODE_CTOR(ce) \
	((zend_class_entry*)(((zend_uintptr_t)(ce)) & ~(CTOR_CALL_BIT|CTOR_USED_BIT)))

#undef EX
#define EX(element) execute_data->element

ZEND_API zval* zend_get_compiled_variable_value(const zend_execute_data *execute_data, zend_uint var)
{
	return EX_VAR(var);
}

static zend_always_inline zval *_get_zval_ptr_tmp(zend_uint var, const zend_execute_data *execute_data, zend_free_op *should_free TSRMLS_DC)
{
	zval *ret = EX_VAR(var);
	should_free->var = ret;

	ZEND_ASSERT(Z_TYPE_P(ret) != IS_REFERENCE);

	return ret;
}

static zend_always_inline zval *_get_zval_ptr_var(zend_uint var, const zend_execute_data *execute_data, zend_free_op *should_free TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	should_free->var = ret;
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_var_deref(zend_uint var, const zend_execute_data *execute_data, zend_free_op *should_free TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	should_free->var = ret;
	ZVAL_DEREF(ret);
	return ret;
}

static zend_never_inline zval *_get_zval_cv_lookup(zval *ptr, zend_uint var, int type, const zend_execute_data *execute_data TSRMLS_DC)
{
	zend_string *cv;

	switch (type) {
		case BP_VAR_R:
		case BP_VAR_UNSET:
			cv = CV_DEF_OF(EX_VAR_TO_NUM(var));
			zend_error(E_NOTICE, "Undefined variable: %s", cv->val);
			/* break missing intentionally */
		case BP_VAR_IS:
			ptr = &EG(uninitialized_zval);
			break;
		case BP_VAR_RW:
			cv = CV_DEF_OF(EX_VAR_TO_NUM(var));
			zend_error(E_NOTICE, "Undefined variable: %s", cv->val);
			/* break missing intentionally */
		case BP_VAR_W:
			ZVAL_NULL(ptr);
			break;
	}
	return ptr;
}

static zend_always_inline zval *_get_zval_cv_lookup_BP_VAR_R(zval *ptr, zend_uint var, const zend_execute_data *execute_data TSRMLS_DC)
{
	zend_string *cv = CV_DEF_OF(EX_VAR_TO_NUM(var));

	zend_error(E_NOTICE, "Undefined variable: %s", cv->val);
	return &EG(uninitialized_zval);
}

static zend_always_inline zval *_get_zval_cv_lookup_BP_VAR_UNSET(zval *ptr, zend_uint var, const zend_execute_data *execute_data TSRMLS_DC)
{
	zend_string *cv = CV_DEF_OF(EX_VAR_TO_NUM(var));

	zend_error(E_NOTICE, "Undefined variable: %s", cv->val);
	return &EG(uninitialized_zval);
}

static zend_always_inline zval *_get_zval_cv_lookup_BP_VAR_IS(zval *ptr, zend_uint var, const zend_execute_data *execute_data TSRMLS_DC)
{
	return &EG(uninitialized_zval);
}

static zend_always_inline zval *_get_zval_cv_lookup_BP_VAR_RW(zval *ptr, zend_uint var, const zend_execute_data *execute_data TSRMLS_DC)
{
	zend_string *cv = CV_DEF_OF(EX_VAR_TO_NUM(var));

	ZVAL_NULL(ptr);
	zend_error(E_NOTICE, "Undefined variable: %s", cv->val);
	return ptr;
}

static zend_always_inline zval *_get_zval_cv_lookup_BP_VAR_W(zval *ptr, zend_uint var, const zend_execute_data *execute_data TSRMLS_DC)
{
	ZVAL_NULL(ptr);
	return ptr;
}

static zend_always_inline zval *_get_zval_ptr_cv(const zend_execute_data *execute_data, zend_uint var, int type TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup(ret, var, type, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_deref(const zend_execute_data *execute_data, zend_uint var, int type TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup(ret, var, type, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_R(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_R(ret, var, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_R(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_R(ret, var, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_UNSET(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_UNSET(ret, var, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_UNSET(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_UNSET(ret, var, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_IS(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (Z_TYPE_P(ret) == IS_UNDEF) {
		return _get_zval_cv_lookup_BP_VAR_IS(ret, var, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_IS(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (Z_TYPE_P(ret) == IS_UNDEF) {
		return _get_zval_cv_lookup_BP_VAR_IS(ret, var, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_RW(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_RW(ret, var, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_RW(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (UNEXPECTED(Z_TYPE_P(ret) == IS_UNDEF)) {
		return _get_zval_cv_lookup_BP_VAR_RW(ret, var, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_BP_VAR_W(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (Z_TYPE_P(ret) == IS_UNDEF) {
		return _get_zval_cv_lookup_BP_VAR_W(ret, var, execute_data TSRMLS_CC);
	}
	return ret;
}

static zend_always_inline zval *_get_zval_ptr_cv_undef_BP_VAR_W(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	return EX_VAR(var);
}

static zend_always_inline zval *_get_zval_ptr_cv_deref_BP_VAR_W(const zend_execute_data *execute_data, zend_uint var TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (Z_TYPE_P(ret) == IS_UNDEF) {
		return _get_zval_cv_lookup_BP_VAR_W(ret, var, execute_data TSRMLS_CC);
	}
	ZVAL_DEREF(ret);
	return ret;
}

static inline zval *_get_zval_ptr(int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC)
{
	zval *ret;

	switch (op_type) {
		case IS_CONST:
			should_free->var = NULL;
			return node->zv;
			break;
		case IS_TMP_VAR:
			ret = EX_VAR(node->var);
			should_free->var = TMP_FREE(ret);
			return ret;
			break;
		case IS_VAR:
			return _get_zval_ptr_var(node->var, execute_data, should_free TSRMLS_CC);
			break;
		case IS_UNUSED:
			should_free->var = NULL;
			return NULL;
			break;
		case IS_CV:
		default:
			should_free->var = NULL;
			return _get_zval_ptr_cv(execute_data, node->var, type TSRMLS_CC);
			break;
	}
	return NULL;
}

static inline zval *_get_zval_ptr_deref(int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC)
{
	zval *ret;

	switch (op_type) {
		case IS_CONST:
			should_free->var = NULL;
			return node->zv;
			break;
		case IS_TMP_VAR:
			ret = EX_VAR(node->var);
			should_free->var = TMP_FREE(ret);
			return ret;
			break;
		case IS_VAR:
			return _get_zval_ptr_var_deref(node->var, execute_data, should_free TSRMLS_CC);
			break;
		case IS_UNUSED:
			should_free->var = NULL;
			return NULL;
			break;
		case IS_CV:
		default:
			should_free->var = NULL;
			return _get_zval_ptr_cv_deref(execute_data, node->var, type TSRMLS_CC);
			break;
	}
	return NULL;
}

static zend_always_inline zval *_get_zval_ptr_ptr_var(zend_uint var, const zend_execute_data *execute_data, zend_free_op *should_free TSRMLS_DC)
{
	zval *ret = EX_VAR(var);

	if (EXPECTED(Z_TYPE_P(ret) == IS_INDIRECT)) {
		should_free->var = NULL;
		return Z_INDIRECT_P(ret);
	} else if (!Z_REFCOUNTED_P(ret)) {
		should_free->var = ret; //???
		return ret;
	} else {
		PZVAL_UNLOCK(ret, should_free);
		return ret;
	}
}

static inline zval *_get_zval_ptr_ptr(int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC)
{
	if (op_type == IS_CV) {
		should_free->var = NULL;
		return _get_zval_ptr_cv(execute_data, node->var, type TSRMLS_CC);
	} else /* if (op_type == IS_VAR) */ {
		ZEND_ASSERT(op_type == IS_VAR);
		return _get_zval_ptr_ptr_var(node->var, execute_data, should_free TSRMLS_CC);
	}
}

static zend_always_inline zval *_get_obj_zval_ptr_unused(TSRMLS_D)
{
	if (EXPECTED(Z_OBJ(EG(This)) != NULL)) {
		return &EG(This);
	} else {
		zend_error_noreturn(E_ERROR, "Using $this when not in object context");
		return NULL;
	}
}

static inline zval *_get_obj_zval_ptr(int op_type, znode_op *op, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC)
{
	if (op_type == IS_UNUSED) {
		if (EXPECTED(Z_OBJ(EG(This)) != NULL)) {
			should_free->var = NULL;
			return &EG(This);
		} else {
			zend_error_noreturn(E_ERROR, "Using $this when not in object context");
		}
	}
	return get_zval_ptr(op_type, op, execute_data, should_free, type);
}

static inline zval *_get_obj_zval_ptr_ptr(int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC)
{
	if (op_type == IS_UNUSED) {
		if (EXPECTED(Z_OBJ(EG(This)) != NULL)) {
			should_free->var = NULL;
			return &EG(This);
		} else {
			zend_error_noreturn(E_ERROR, "Using $this when not in object context");
		}
	}
	return get_zval_ptr_ptr(op_type, node, execute_data, should_free, type);
}

static inline void zend_assign_to_variable_reference(zval *variable_ptr, zval *value_ptr TSRMLS_DC)
{
	if (EXPECTED(variable_ptr != value_ptr)) {
		ZVAL_MAKE_REF(value_ptr);
		Z_ADDREF_P(value_ptr);
		zval_ptr_dtor(variable_ptr);
		ZVAL_REF(variable_ptr, Z_REF_P(value_ptr));
	} else {
		ZVAL_MAKE_REF(variable_ptr);
	}
}

/* this should modify object only if it's empty */
static inline zval* make_real_object(zval *object_ptr TSRMLS_DC)
{
	zval *object = object_ptr;

	ZVAL_DEREF(object);
	if (UNEXPECTED(Z_TYPE_P(object) != IS_OBJECT)) {
		if (Z_TYPE_P(object) == IS_NULL
			|| Z_TYPE_P(object) == IS_FALSE
			|| (Z_TYPE_P(object) == IS_STRING && Z_STRSIZE_P(object) == 0)) {
			zval_ptr_dtor_nogc(object);
			object_init(object);
			zend_error(E_WARNING, "Creating default object from empty value");
		}
	}
	return object;
}

ZEND_API char * zend_verify_arg_class_kind(const zend_arg_info *cur_arg_info, zend_uint_t fetch_type, char **class_name, zend_class_entry **pce TSRMLS_DC)
{
	zend_string *key;
	ALLOCA_FLAG(use_heap);

	STR_ALLOCA_INIT(key, cur_arg_info->class_name, cur_arg_info->class_name_len, use_heap);
	*pce = zend_fetch_class(key, (fetch_type | ZEND_FETCH_CLASS_AUTO | ZEND_FETCH_CLASS_NO_AUTOLOAD) TSRMLS_CC);
	STR_ALLOCA_FREE(key, use_heap);

	*class_name = (*pce) ? (*pce)->name->val : (char*)cur_arg_info->class_name;
	if (*pce && (*pce)->ce_flags & ZEND_ACC_INTERFACE) {
		return "implement interface ";
	} else {
		return "be an instance of ";
	}
}

ZEND_API void zend_verify_arg_error(int error_type, const zend_function *zf, zend_uint arg_num, const char *need_msg, const char *need_kind, const char *given_msg, const char *given_kind, zval *arg TSRMLS_DC)
{
	zend_execute_data *ptr = EG(current_execute_data)->prev_execute_data;
	const char *fname = zf->common.function_name->val;
	const char *fsep;
	const char *fclass;
	zval old_arg;

	if (zf->common.scope) {
		fsep =  "::";
		fclass = zf->common.scope->name->val;
	} else {
		fsep =  "";
		fclass = "";
	}

	if (arg && zf->common.type == ZEND_USER_FUNCTION) {
		ZVAL_COPY_VALUE(&old_arg, arg);
		ZVAL_UNDEF(arg);
	}

	if (zf->common.type == ZEND_USER_FUNCTION && ptr && ptr->func && ZEND_USER_CODE(ptr->func->common.type)) {
		zend_error(error_type, "Argument %d passed to %s%s%s() must %s%s, %s%s given, called in %s on line %d and defined", arg_num, fclass, fsep, fname, need_msg, need_kind, given_msg, given_kind, ptr->func->op_array.filename->val, ptr->opline->lineno);
	} else {
		zend_error(error_type, "Argument %d passed to %s%s%s() must %s%s, %s%s given", arg_num, fclass, fsep, fname, need_msg, need_kind, given_msg, given_kind);
	}

	if (arg && zf->common.type == ZEND_USER_FUNCTION) {
		ZVAL_COPY_VALUE(arg, &old_arg);
	}
}

static void zend_verify_arg_type(zend_function *zf, zend_uint arg_num, zval *arg, zend_uint_t fetch_type TSRMLS_DC)
{
	zend_arg_info *cur_arg_info;
	char *need_msg;
	zend_class_entry *ce;

	if (UNEXPECTED(!zf->common.arg_info)) {
		return;
	}

	if (EXPECTED(arg_num <= zf->common.num_args)) {
		cur_arg_info = &zf->common.arg_info[arg_num-1];
	} else if (zf->common.fn_flags & ZEND_ACC_VARIADIC) {
		cur_arg_info = &zf->common.arg_info[zf->common.num_args-1];
	} else {
		return;
	}

	if (cur_arg_info->class_name) {
		char *class_name;

		ZVAL_DEREF(arg);
		if (Z_TYPE_P(arg) == IS_OBJECT) {
			need_msg = zend_verify_arg_class_kind(cur_arg_info, fetch_type, &class_name, &ce TSRMLS_CC);
			if (!ce || !instanceof_function(Z_OBJCE_P(arg), ce TSRMLS_CC)) {
				zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, need_msg, class_name, "instance of ", Z_OBJCE_P(arg)->name->val, arg TSRMLS_CC);
			}
		} else if (Z_TYPE_P(arg) != IS_NULL || !cur_arg_info->allow_null) {
			need_msg = zend_verify_arg_class_kind(cur_arg_info, fetch_type, &class_name, &ce TSRMLS_CC);
			zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, need_msg, class_name, zend_zval_type_name(arg), "", arg TSRMLS_CC);
		}
	} else if (cur_arg_info->type_hint) {
		if (cur_arg_info->type_hint == IS_ARRAY) {
			ZVAL_DEREF(arg);
			if (Z_TYPE_P(arg) != IS_ARRAY && (Z_TYPE_P(arg) != IS_NULL || !cur_arg_info->allow_null)) {
				zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, "be of the type array", "", zend_zval_type_name(arg), "", arg TSRMLS_CC);
			}
		} else if (cur_arg_info->type_hint == IS_CALLABLE) {
			if (!zend_is_callable(arg, IS_CALLABLE_CHECK_SILENT, NULL TSRMLS_CC) && (Z_TYPE_P(arg) != IS_NULL || !cur_arg_info->allow_null)) {
				zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, "be callable", "", zend_zval_type_name(arg), "", arg TSRMLS_CC);
			}
#if ZEND_DEBUG
		} else {
			zend_error(E_ERROR, "Unknown typehint");
#endif
		}
	}
}

static inline int zend_verify_missing_arg_type(zend_function *zf, zend_uint arg_num, zend_uint_t fetch_type TSRMLS_DC)
{
	zend_arg_info *cur_arg_info;
	char *need_msg;
	zend_class_entry *ce;

	if (UNEXPECTED(!zf->common.arg_info)) {
		return 1;
	}

	if (EXPECTED(arg_num <= zf->common.num_args)) {
		cur_arg_info = &zf->common.arg_info[arg_num-1];
	} else if (zf->common.fn_flags & ZEND_ACC_VARIADIC) {
		cur_arg_info = &zf->common.arg_info[zf->common.num_args-1];
	} else {
		return 1;
	}

	if (cur_arg_info->class_name) {
		char *class_name;

		need_msg = zend_verify_arg_class_kind(cur_arg_info, fetch_type, &class_name, &ce TSRMLS_CC);
		zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, need_msg, class_name, "none", "", NULL TSRMLS_CC);
	} else if (cur_arg_info->type_hint) {
		if (cur_arg_info->type_hint == IS_ARRAY) {
			zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, "be of the type array", "", "none", "", NULL TSRMLS_CC);
		} else if (cur_arg_info->type_hint == IS_CALLABLE) {
			zend_verify_arg_error(E_RECOVERABLE_ERROR, zf, arg_num, "be callable", "", "none", "", NULL TSRMLS_CC);
#if ZEND_DEBUG
		} else {
			zend_error(E_ERROR, "Unknown typehint");
#endif
		}
	}
	return 0;
}

static void zend_verify_missing_arg(zend_execute_data *execute_data, zend_uint arg_num TSRMLS_DC)
{
	if (EXPECTED(!(EX(func)->common.fn_flags & ZEND_ACC_HAS_TYPE_HINTS)) ||
	    zend_verify_missing_arg_type(EX(func), arg_num, EX(opline)->extended_value TSRMLS_CC)) {
		const char *class_name = EX(func)->common.scope ? EX(func)->common.scope->name->val : "";
		const char *space = EX(func)->common.scope ? "::" : "";
		const char *func_name = EX(func)->common.function_name ? EX(func)->common.function_name->val : "main";
		zend_execute_data *ptr = EX(prev_execute_data);

		if (ptr && ptr->func && ZEND_USER_CODE(ptr->func->common.type)) {
			zend_error(E_WARNING, "Missing argument %u for %s%s%s(), called in %s on line %d and defined", arg_num, class_name, space, func_name, ptr->func->op_array.filename->val, ptr->opline->lineno);
		} else {
			zend_error(E_WARNING, "Missing argument %u for %s%s%s()", arg_num, class_name, space, func_name);
		}
	}
}

static inline void zend_assign_to_object(zval *retval, zval *object_ptr, zval *property_name, int value_type, znode_op *value_op, const zend_execute_data *execute_data, int opcode, void **cache_slot TSRMLS_DC)
{
	zend_free_op free_value;
 	zval *value = get_zval_ptr(value_type, value_op, execute_data, &free_value, BP_VAR_R);
 	zval tmp;
 	zval *object = object_ptr;

 	ZVAL_DEREF(object);
	if (Z_TYPE_P(object) != IS_OBJECT) {
		if (object == &EG(error_zval)) {
 			if (retval) {
 				ZVAL_NULL(retval);
			}
			FREE_OP(free_value);
			return;
		}
		if (Z_TYPE_P(object) == IS_NULL ||
		    Z_TYPE_P(object) == IS_FALSE ||
		    (Z_TYPE_P(object) == IS_STRING && Z_STRSIZE_P(object) == 0)) {
			zend_object *obj;

			zval_ptr_dtor(object);
			object_init(object);
			Z_ADDREF_P(object);
			obj = Z_OBJ_P(object);
			zend_error(E_WARNING, "Creating default object from empty value");
			if (GC_REFCOUNT(obj) == 1) {
				/* the enclosing container was deleted, obj is unreferenced */
				if (retval) {
					ZVAL_NULL(retval);
				}
				FREE_OP(free_value);
				OBJ_RELEASE(obj);
				return;
			}
			Z_DELREF_P(object);
		} else {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
			if (retval) {
				ZVAL_NULL(retval);
			}
			FREE_OP(free_value);
			return;
		}
	}

	/* separate our value if necessary */
	if (value_type == IS_TMP_VAR) {
		ZVAL_COPY_VALUE(&tmp, value);
		value = &tmp;
	} else if (value_type == IS_CONST) {
		if (UNEXPECTED(Z_OPT_COPYABLE_P(value))) {
			ZVAL_COPY_VALUE(&tmp, value);
			zval_copy_ctor_func(&tmp);
			value = &tmp;
		}
	} else if (Z_REFCOUNTED_P(value)) {
		Z_ADDREF_P(value);
	}

	if (opcode == ZEND_ASSIGN_OBJ) {
		if (!Z_OBJ_HT_P(object)->write_property) {
			zend_error(E_WARNING, "Attempt to assign property of non-object");
			if (retval) {
				ZVAL_NULL(retval);
			}
			if (value_type == IS_CONST) {
				zval_ptr_dtor(value);
			}
			FREE_OP(free_value);
			return;
		}
		Z_OBJ_HT_P(object)->write_property(object, property_name, value, cache_slot TSRMLS_CC);
	} else {
		/* Note:  property_name in this case is really the array index! */
		if (!Z_OBJ_HT_P(object)->write_dimension) {
			zend_error_noreturn(E_ERROR, "Cannot use object as array");
		}
		Z_OBJ_HT_P(object)->write_dimension(object, property_name, value TSRMLS_CC);
	}

	if (retval && !EG(exception)) {
		ZVAL_COPY(retval, value);
	}
	zval_ptr_dtor(value);
	FREE_OP_IF_VAR(free_value);
}

static void zend_assign_to_string_offset(zval *str_offset, zval *value, int value_type, zval *result TSRMLS_DC)
{
	zval *str = Z_STR_OFFSET_STR_P(str_offset);
	zend_uint offset = Z_STR_OFFSET_IDX_P(str_offset);
	zend_string *old_str;

	if ((int)offset < 0) {
		zend_error(E_WARNING, "Illegal string offset:  %d", offset);
		STR_RELEASE(Z_STR_P(str));
		if (result) {
			ZVAL_NULL(result);
		}
		return;
	}

	old_str = Z_STR_P(str);
	if (offset >= Z_STRSIZE_P(str)) {
		int old_len = Z_STRSIZE_P(str);
		Z_STR_P(str) = STR_REALLOC(Z_STR_P(str), offset + 1, 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
		memset(Z_STRVAL_P(str) + old_len, ' ', offset - old_len);
		Z_STRVAL_P(str)[offset+1] = 0;
	} else if (IS_INTERNED(Z_STR_P(str))) {
		Z_STR_P(str) = STR_INIT(Z_STRVAL_P(str), Z_STRSIZE_P(str), 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
	}

	if (Z_TYPE_P(value) != IS_STRING) {
		zend_string *tmp = zval_get_string(value);

		Z_STRVAL_P(str)[offset] = tmp->val[0];
		STR_RELEASE(tmp);
	} else {
		Z_STRVAL_P(str)[offset] = Z_STRVAL_P(value)[0];
		if (value_type == IS_TMP_VAR) {
			/* we can safely free final_value here
			 * because separation is done only
			 * in case value_type == IS_VAR */
			zval_dtor(value);
		}
	}
	/*
	 * the value of an assignment to a string offset is undefined
	T(result->u.var).var = &T->str_offset.str;
	*/

	STR_RELEASE(old_str);
	if (result) {
		zend_uchar c = (zend_uchar)Z_STRVAL_P(str)[offset];

		if (CG(one_char_string)[c]) {
			ZVAL_INT_STR(result, CG(one_char_string)[c]);
		} else {
			ZVAL_NEW_STR(result, STR_INIT(Z_STRVAL_P(str) + offset, 1, 0));
		}
	}
}

static inline zval* zend_assign_tmp_to_variable(zval *variable_ptr, zval *value TSRMLS_DC)
{
	ZVAL_DEREF(variable_ptr);

	if (UNEXPECTED(Z_REFCOUNTED_P(variable_ptr))) {
		zend_refcounted *garbage;

		if (Z_TYPE_P(variable_ptr) == IS_OBJECT &&
		    UNEXPECTED(Z_OBJ_HANDLER_P(variable_ptr, set) != NULL)) {
			Z_OBJ_HANDLER_P(variable_ptr, set)(variable_ptr, value TSRMLS_CC);
			return variable_ptr;
		}

		garbage = Z_COUNTED_P(variable_ptr);
		if (UNEXPECTED(GC_REFCOUNT(garbage) > 1)) {
			/* we need to split */
			GC_REFCOUNT(garbage)--;
			/* optimized version of GC_ZVAL_CHECK_POSSIBLE_ROOT(variable_ptr) */
			if ((Z_COLLECTABLE_P(variable_ptr)) &&
	    		UNEXPECTED(!GC_INFO(garbage))) {
				gc_possible_root(garbage TSRMLS_CC);
			}
		} else {
			ZVAL_COPY_VALUE(variable_ptr, value);
			_zval_dtor_func(garbage ZEND_FILE_LINE_CC);
			return variable_ptr;
		}
	}

	ZVAL_COPY_VALUE(variable_ptr, value);
	
	return variable_ptr;
}

static inline zval* zend_assign_const_to_variable(zval *variable_ptr, zval *value TSRMLS_DC)
{
	ZVAL_DEREF(variable_ptr);

	if (UNEXPECTED(Z_REFCOUNTED_P(variable_ptr))) {
		zend_refcounted *garbage;

		if (Z_TYPE_P(variable_ptr) == IS_OBJECT &&
		    UNEXPECTED(Z_OBJ_HANDLER_P(variable_ptr, set) != NULL)) {
			Z_OBJ_HANDLER_P(variable_ptr, set)(variable_ptr, value TSRMLS_CC);
			return variable_ptr;
		}

		garbage = Z_COUNTED_P(variable_ptr);
		if (UNEXPECTED(GC_REFCOUNT(garbage) > 1)) {
			/* we need to split */
			GC_REFCOUNT(garbage)--;
			/* optimized version of GC_ZVAL_CHECK_POSSIBLE_ROOT(variable_ptr) */
			if (Z_COLLECTABLE_P(variable_ptr) &&
	    		UNEXPECTED(!GC_INFO(garbage))) {
				gc_possible_root(garbage TSRMLS_CC);
			}
	 	} else {
			ZVAL_COPY_VALUE(variable_ptr, value);
			/* IS_CONST can't be IS_OBJECT, IS_RESOURCE or IS_REFERENCE */
			if (UNEXPECTED(Z_OPT_COPYABLE_P(variable_ptr))) {
				zval_copy_ctor_func(variable_ptr);
			}
			_zval_dtor_func(garbage ZEND_FILE_LINE_CC);
			return variable_ptr;
		}
	}
	
	ZVAL_COPY_VALUE(variable_ptr, value);
	/* IS_CONST can't be IS_OBJECT, IS_RESOURCE or IS_REFERENCE */
	if (UNEXPECTED(Z_OPT_COPYABLE_P(variable_ptr))) {
		zval_copy_ctor_func(variable_ptr);
	}

	return variable_ptr;
}

static inline zval* zend_assign_to_variable(zval *variable_ptr, zval *value TSRMLS_DC)
{
	zend_refcounted *garbage;

	if (EXPECTED(!Z_REFCOUNTED_P(variable_ptr))) {	
		goto assign_simple;
	} else if (UNEXPECTED(variable_ptr == value)) {
		return variable_ptr;
	}
	if (Z_ISREF_P(variable_ptr)) {
		variable_ptr = Z_REFVAL_P(variable_ptr);
		if (EXPECTED(!Z_REFCOUNTED_P(variable_ptr))) {
			goto assign_simple;
		} else if (UNEXPECTED(variable_ptr == value)) {
			return variable_ptr;
		}
	}

	if (Z_TYPE_P(variable_ptr) == IS_OBJECT &&
	    UNEXPECTED(Z_OBJ_HANDLER_P(variable_ptr, set) != NULL)) {
		Z_OBJ_HANDLER_P(variable_ptr, set)(variable_ptr, value TSRMLS_CC);
	} else {
		if (Z_REFCOUNT_P(variable_ptr)==1) {
			garbage = Z_COUNTED_P(variable_ptr);
			if (UNEXPECTED(Z_REFCOUNTED_P(value))) {
				if (EXPECTED(!Z_ISREF_P(value))) {
					Z_ADDREF_P(value);
				} else {
					if (Z_REFCOUNT_P(value) == 1) {
						ZVAL_UNREF(value);
					} else {
						value = Z_REFVAL_P(value);
					}
					if (Z_REFCOUNTED_P(value)) {
						if (UNEXPECTED(variable_ptr == value)) {
							return variable_ptr;
						}
						Z_ADDREF_P(value);
					}
				}
			}
			ZVAL_COPY_VALUE(variable_ptr, value);
			_zval_dtor_func(garbage ZEND_FILE_LINE_CC);
		} else { /* we need to split */
			Z_DELREF_P(variable_ptr);
			GC_ZVAL_CHECK_POSSIBLE_ROOT(variable_ptr);
assign_simple:
			if (UNEXPECTED(Z_REFCOUNTED_P(value))) {
				if (EXPECTED(!Z_ISREF_P(value))) {
					Z_ADDREF_P(value);
				} else {
					if (Z_REFCOUNT_P(value) == 1) {
						ZVAL_UNREF(value);
					} else {
						value = Z_REFVAL_P(value);
					}
					if (Z_REFCOUNTED_P(value)) {
						Z_ADDREF_P(value);
					}
				}
			}
			ZVAL_COPY_VALUE(variable_ptr, value);
		}
	}
	return variable_ptr;
}

/* Utility Functions for Extensions */
static void zend_extension_statement_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->statement_handler) {
		extension->statement_handler(op_array);
	}
}


static void zend_extension_fcall_begin_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->fcall_begin_handler) {
		extension->fcall_begin_handler(op_array);
	}
}


static void zend_extension_fcall_end_handler(const zend_extension *extension, zend_op_array *op_array TSRMLS_DC)
{
	if (extension->fcall_end_handler) {
		extension->fcall_end_handler(op_array);
	}
}


static zend_always_inline HashTable *zend_get_target_symbol_table(zend_execute_data *execute_data, int fetch_type TSRMLS_DC)
{
	HashTable *ht;

	if (EXPECTED(fetch_type == ZEND_FETCH_GLOBAL_LOCK) || 
	    EXPECTED(fetch_type == ZEND_FETCH_GLOBAL)) {
		ht = &EG(symbol_table).ht;
	} else if (EXPECTED(fetch_type == ZEND_FETCH_STATIC)) {
		ZEND_ASSERT(EX(func)->op_array.static_variables != NULL);
		ht = EX(func)->op_array.static_variables;
	} else {
		ZEND_ASSERT(fetch_type == ZEND_FETCH_LOCAL);
		if (!EX(symbol_table)) {
			zend_rebuild_symbol_table(TSRMLS_C);
		}
		ht = &EX(symbol_table)->ht;
	}
	return ht;
}

static zend_always_inline zval *zend_fetch_dimension_address_inner(HashTable *ht, const zval *dim, int dim_type, int type TSRMLS_DC)
{
	zval *retval;
	zend_string *offset_key;
	zend_uint_t hval;

	if (EXPECTED(Z_TYPE_P(dim) == IS_INT)) {
		hval = Z_IVAL_P(dim);
num_index:
		retval = zend_hash_index_find(ht, hval);
		if (retval == NULL) {
			switch (type) {
				case BP_VAR_R:
					zend_error(E_NOTICE,"Undefined offset: " ZEND_UINT_FMT, hval);
					/* break missing intentionally */
				case BP_VAR_UNSET:
				case BP_VAR_IS:
					retval = &EG(uninitialized_zval);
					break;
				case BP_VAR_RW:
					zend_error(E_NOTICE,"Undefined offset: " ZEND_UINT_FMT, hval);
					/* break missing intentionally */
				case BP_VAR_W:
					retval = zend_hash_index_add_new(ht, hval, &EG(uninitialized_zval));
					break;
			}
		}
	} else if (EXPECTED(Z_TYPE_P(dim) == IS_STRING)) {
		offset_key = Z_STR_P(dim);
		if (dim_type != IS_CONST) {
			if (ZEND_HANDLE_NUMERIC(offset_key, hval)) {
				goto num_index;
			}
		}
str_index:
		retval = zend_hash_find(ht, offset_key);
		if (retval) {
			/* support for $GLOBALS[...] */
			if (UNEXPECTED(Z_TYPE_P(retval) == IS_INDIRECT)) {
				retval = Z_INDIRECT_P(retval);
				if (UNEXPECTED(Z_TYPE_P(retval) == IS_UNDEF)) {
					switch (type) {
						case BP_VAR_R:
							zend_error(E_NOTICE, "Undefined index: %s", offset_key->val);
							/* break missing intentionally */
						case BP_VAR_UNSET:
						case BP_VAR_IS:
							retval = &EG(uninitialized_zval);
							break;
						case BP_VAR_RW:
							zend_error(E_NOTICE,"Undefined index: %s", offset_key->val);
							/* break missing intentionally */
						case BP_VAR_W:
							ZVAL_NULL(retval);
							break;
					}
				}
			}
		} else { 
			switch (type) {
				case BP_VAR_R:
					zend_error(E_NOTICE, "Undefined index: %s", offset_key->val);
					/* break missing intentionally */
				case BP_VAR_UNSET:
				case BP_VAR_IS:
					retval = &EG(uninitialized_zval);
					break;
				case BP_VAR_RW:
					zend_error(E_NOTICE,"Undefined index: %s", offset_key->val);
					/* break missing intentionally */
				case BP_VAR_W:
					retval = zend_hash_add_new(ht, offset_key, &EG(uninitialized_zval));
					break;
			}
		}
	} else {
		switch (Z_TYPE_P(dim)) {
			case IS_NULL:
				offset_key = STR_EMPTY_ALLOC();
				goto str_index;
			case IS_DOUBLE:
				hval = zend_dval_to_ival(Z_DVAL_P(dim));
				goto num_index;
			case IS_RESOURCE:
				zend_error(E_STRICT, "Resource ID#%pd used as offset, casting to integer (%pd)", Z_RES_HANDLE_P(dim), Z_RES_HANDLE_P(dim));
				hval = Z_RES_HANDLE_P(dim);
				goto num_index;
			case IS_FALSE:
				hval = 0;
				goto num_index;
			case IS_TRUE:
				hval = 1;
				goto num_index;
			default:
				zend_error(E_WARNING, "Illegal offset type");
				retval = (type == BP_VAR_W || type == BP_VAR_RW) ?
					&EG(error_zval) : &EG(uninitialized_zval);
		}
	}
	return retval;
}

static zend_always_inline void zend_fetch_dimension_address(zval *result, zval *container_ptr, zval *dim, int dim_type, int type, int is_ref TSRMLS_DC)
{
    zval *retval;
    zval *container = container_ptr;

	ZVAL_DEREF(container);
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
		SEPARATE_ARRAY(container);
fetch_from_array:
		if (dim == NULL) {
			retval = zend_hash_next_index_insert(Z_ARRVAL_P(container), &EG(uninitialized_zval));
			if (retval == NULL) {
				zend_error(E_WARNING, "Cannot add element to the array as the next element is already occupied");
				retval = &EG(error_zval);
			}
		} else {
			retval = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, dim_type, type TSRMLS_CC);
		}
		if (is_ref) {
			ZVAL_MAKE_REF(retval);
			Z_ADDREF_P(retval);
			ZVAL_REF(result, Z_REF_P(retval));
		} else {
			ZVAL_INDIRECT(result, retval);
		}
	} else if (EXPECTED(Z_TYPE_P(container) == IS_STRING)) {
		zend_int_t offset;

		if (type != BP_VAR_UNSET && UNEXPECTED(Z_STRSIZE_P(container) == 0)) {
			zval_dtor(container);
convert_to_array:
			ZVAL_NEW_ARR(container);
			zend_hash_init(Z_ARRVAL_P(container), 8, NULL, ZVAL_PTR_DTOR, 0);
			goto fetch_from_array;
		}
		if (dim == NULL) {
			zend_error_noreturn(E_ERROR, "[] operator not supported for strings");
		}

		if (type != BP_VAR_UNSET) {
			SEPARATE_STRING(container);
		}

		if (UNEXPECTED(Z_TYPE_P(dim) != IS_INT)) {
			switch(Z_TYPE_P(dim)) {
				case IS_STRING:
					if (IS_INT == is_numeric_string(Z_STRVAL_P(dim), Z_STRSIZE_P(dim), NULL, NULL, -1)) {
						break;
					}
					if (type != BP_VAR_UNSET) {
						zend_error(E_WARNING, "Illegal string offset '%s'", Z_STRVAL_P(dim));
					}
					break;
				case IS_DOUBLE:
				case IS_NULL:
				case IS_FALSE:
				case IS_TRUE:
					zend_error(E_NOTICE, "String offset cast occurred");
					break;
				default:
					zend_error(E_WARNING, "Illegal offset type");
					break;
			}

			offset = zval_get_int(dim);
		} else {
			offset = Z_IVAL_P(dim);
		}

		if (!IS_INTERNED(Z_STR_P(container))) STR_ADDREF(Z_STR_P(container));
		ZVAL_STR_OFFSET(result, container, offset);
	} else if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		if (!Z_OBJ_HT_P(container)->read_dimension) {
			zend_error_noreturn(E_ERROR, "Cannot use object as array");
		} else {
			retval = Z_OBJ_HT_P(container)->read_dimension(container, dim, type, result TSRMLS_CC);

			if (UNEXPECTED(retval == &EG(uninitialized_zval))) {
				zend_class_entry *ce = Z_OBJCE_P(container);

				ZVAL_NULL(result);
				zend_error(E_NOTICE, "Indirect modification of overloaded element of %s has no effect", ce->name->val);
			} else if (retval && Z_TYPE_P(retval) != IS_UNDEF) {
				if (!Z_ISREF_P(retval)) {
					if (Z_REFCOUNTED_P(retval) &&
					    Z_REFCOUNT_P(retval) > 1) {
						if (Z_TYPE_P(retval) != IS_OBJECT) {
							Z_DELREF_P(retval);
							ZVAL_DUP(result, retval);
							retval = result;
						} else {
							ZVAL_COPY(result, retval);
							retval = result;
						}
					}
					if (Z_TYPE_P(retval) != IS_OBJECT) {
						zend_class_entry *ce = Z_OBJCE_P(container);
						zend_error(E_NOTICE, "Indirect modification of overloaded element of %s has no effect", ce->name->val);
					}
				}
				if (result != retval) {
					if (is_ref) {
						ZVAL_MAKE_REF(retval);
						Z_ADDREF_P(retval);
						ZVAL_REF(result, Z_REF_P(retval));
					} else {
						ZVAL_INDIRECT(result, retval);
					}
				}
			} else {
				ZVAL_INDIRECT(result, &EG(error_zval));
			}
		}
	} else if (EXPECTED(Z_TYPE_P(container) == IS_NULL)) {
		if (container == &EG(error_zval)) {
			ZVAL_INDIRECT(result, &EG(error_zval));
		} else if (type != BP_VAR_UNSET) {
			goto convert_to_array;
		} else {
			/* for read-mode only */
			ZVAL_NULL(result);
		}
	} else {
		if (type != BP_VAR_UNSET &&
		    Z_TYPE_P(container) == IS_FALSE) {
			goto convert_to_array;
		}
		if (type == BP_VAR_UNSET) {
			zend_error(E_WARNING, "Cannot unset offset in a non-array variable");
			ZVAL_NULL(result);
		} else {
			zend_error(E_WARNING, "Cannot use a scalar value as an array");
			ZVAL_INDIRECT(result, &EG(error_zval));
		}
	}
}

static zend_never_inline void zend_fetch_dimension_address_W(zval *result, zval *container_ptr, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_W, 0 TSRMLS_CC);
}

static zend_never_inline void zend_fetch_dimension_address_W_ref(zval *result, zval *container_ptr, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_W, 1 TSRMLS_CC);
}

static zend_never_inline void zend_fetch_dimension_address_RW(zval *result, zval *container_ptr, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_RW, 0 TSRMLS_CC);
}

static zend_never_inline void zend_fetch_dimension_address_UNSET(zval *result, zval *container_ptr, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address(result, container_ptr, dim, dim_type, BP_VAR_UNSET, 0 TSRMLS_CC);
}

static zend_always_inline void zend_fetch_dimension_address_read(zval *result, zval *container, zval *dim, int dim_type, int type TSRMLS_DC)
{
	zval *retval;

	ZVAL_DEREF(container);
	if (EXPECTED(Z_TYPE_P(container) == IS_ARRAY)) {
		retval = zend_fetch_dimension_address_inner(Z_ARRVAL_P(container), dim, dim_type, type TSRMLS_CC);
		ZVAL_COPY(result, retval);
	} else if (EXPECTED(Z_TYPE_P(container) == IS_STRING)) {
		zend_int_t offset;

		if (UNEXPECTED(Z_TYPE_P(dim) != IS_INT)) {
			switch(Z_TYPE_P(dim)) {
				/* case IS_INT: */
				case IS_STRING:
					if (IS_INT == is_numeric_string(Z_STRVAL_P(dim), Z_STRSIZE_P(dim), NULL, NULL, -1)) {
						break;
					}
					if (type != BP_VAR_IS) {
						zend_error(E_WARNING, "Illegal string offset '%s'", Z_STRVAL_P(dim));
					}
					break;
				case IS_DOUBLE:
				case IS_NULL:
				case IS_FALSE:
				case IS_TRUE:
					if (type != BP_VAR_IS) {
						zend_error(E_NOTICE, "String offset cast occurred");
					}
					break;
				default:
					zend_error(E_WARNING, "Illegal offset type");
					break;
			}

			offset = zval_get_int(dim);
		} else {
			offset = Z_IVAL_P(dim);
		}

		if (UNEXPECTED(offset < 0) || UNEXPECTED(Z_STRSIZE_P(container) <= offset)) {
			if (type != BP_VAR_IS) {
				zend_error(E_NOTICE, "Uninitialized string offset: %ld", offset);
			}
			ZVAL_EMPTY_STRING(result);
		} else {
			zend_uchar c = (zend_uchar)Z_STRVAL_P(container)[offset];

			if (CG(one_char_string)[c]) {
				ZVAL_INT_STR(result, CG(one_char_string)[c]);
			} else {
				ZVAL_NEW_STR(result, STR_INIT(Z_STRVAL_P(container) + offset, 1, 0));
			}
		}
	} else if (EXPECTED(Z_TYPE_P(container) == IS_OBJECT)) {
		if (!Z_OBJ_HT_P(container)->read_dimension) {
			zend_error_noreturn(E_ERROR, "Cannot use object as array");
		} else {
			retval = Z_OBJ_HT_P(container)->read_dimension(container, dim, type, result TSRMLS_CC);

			if (result) {
				if (retval) {
					if (result != retval) {
						ZVAL_COPY(result, retval);
					}
				} else {
					ZVAL_NULL(result);
				}
			}
		}
	} else {
		ZVAL_NULL(result);
	}
}

static zend_never_inline void zend_fetch_dimension_address_read_R(zval *result, zval *container, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address_read(result, container, dim, dim_type, BP_VAR_R TSRMLS_CC);
}

static zend_never_inline void zend_fetch_dimension_address_read_IS(zval *result, zval *container, zval *dim, int dim_type TSRMLS_DC)
{
	zend_fetch_dimension_address_read(result, container, dim, dim_type, BP_VAR_IS TSRMLS_CC);
}

ZEND_API void zend_fetch_dimension_by_zval(zval *result, zval *container, zval *dim TSRMLS_DC)
{
	zend_fetch_dimension_address_read_R(result, container, dim, IS_TMP_VAR TSRMLS_CC);
}

static void zend_fetch_property_address(zval *result, zval *container_ptr, zval *prop_ptr, void **cache_slot, int type, int is_ref TSRMLS_DC)
{
	zval *container = container_ptr;

	ZVAL_DEREF(container);
	if (Z_TYPE_P(container) != IS_OBJECT) {
		if (container == &EG(error_zval)) {
			ZVAL_INDIRECT(result, &EG(error_zval));
			return;
		}

		/* this should modify object only if it's empty */
		if (type != BP_VAR_UNSET &&
		    ((Z_TYPE_P(container) == IS_NULL ||
		      Z_TYPE_P(container) == IS_FALSE ||
		      (Z_TYPE_P(container) == IS_STRING && Z_STRSIZE_P(container)==0)))) {
			zval_ptr_dtor_nogc(container);
			object_init(container);
		} else {
			zend_error(E_WARNING, "Attempt to modify property of non-object");
			ZVAL_INDIRECT(result, &EG(error_zval));
			return;
		}
	}

	if (Z_OBJ_HT_P(container)->get_property_ptr_ptr) {
		zval *ptr = Z_OBJ_HT_P(container)->get_property_ptr_ptr(container, prop_ptr, type, cache_slot TSRMLS_CC);
		if (NULL == ptr) {
			if (Z_OBJ_HT_P(container)->read_property &&
				(ptr = Z_OBJ_HT_P(container)->read_property(container, prop_ptr, type, cache_slot, result TSRMLS_CC)) != NULL) {
				if (ptr != result) {
					if (is_ref && ptr != &EG(uninitialized_zval)) {
						ZVAL_MAKE_REF(ptr);
						Z_ADDREF_P(ptr);
						ZVAL_REF(result, Z_REF_P(ptr));
					} else {
						ZVAL_INDIRECT(result, ptr);
					}
				}
			} else {
				zend_error_noreturn(E_ERROR, "Cannot access undefined property for object with overloaded property access");
			}
		} else {
			if (is_ref) {
				ZVAL_MAKE_REF(ptr);
				Z_ADDREF_P(ptr);
				ZVAL_REF(result, Z_REF_P(ptr));
			} else {
				ZVAL_INDIRECT(result, ptr);
			}
		}
	} else if (Z_OBJ_HT_P(container)->read_property) {
		zval *ptr = Z_OBJ_HT_P(container)->read_property(container, prop_ptr, type, cache_slot, result TSRMLS_CC);
		if (ptr != result) {
			if (is_ref && ptr != &EG(uninitialized_zval)) {
				ZVAL_MAKE_REF(ptr);
				Z_ADDREF_P(ptr);
				ZVAL_REF(result, Z_REF_P(ptr));
			} else {
				ZVAL_INDIRECT(result, ptr);
			}
		}
	} else {
		zend_error(E_WARNING, "This object doesn't support property references");
		ZVAL_INDIRECT(result, &EG(error_zval));
	}
}

static inline zend_brk_cont_element* zend_brk_cont(int nest_levels, int array_offset, const zend_op_array *op_array, const zend_execute_data *execute_data TSRMLS_DC)
{
	int original_nest_levels = nest_levels;
	zend_brk_cont_element *jmp_to;

	do {
		if (array_offset==-1) {
			zend_error_noreturn(E_ERROR, "Cannot break/continue %d level%s", original_nest_levels, (original_nest_levels == 1) ? "" : "s");
		}
		jmp_to = &op_array->brk_cont_array[array_offset];
		if (nest_levels>1) {
			zend_op *brk_opline = &op_array->opcodes[jmp_to->brk];

			if (brk_opline->opcode == ZEND_SWITCH_FREE) {
				if (!(brk_opline->extended_value & EXT_TYPE_FREE_ON_RETURN)) {
					zval_ptr_dtor(EX_VAR(brk_opline->op1.var));
				}
			} else if (brk_opline->opcode == ZEND_FREE) {
				if (!(brk_opline->extended_value & EXT_TYPE_FREE_ON_RETURN)) {
					zval_dtor(EX_VAR(brk_opline->op1.var));
				}
			}
		}
		array_offset = jmp_to->parent;
	} while (--nest_levels > 0);
	return jmp_to;
}

#if ZEND_INTENSIVE_DEBUGGING

#define CHECK_SYMBOL_TABLES()													\
	zend_hash_apply(&EG(symbol_table), zend_check_symbol TSRMLS_CC);			\
	if (&EG(symbol_table)!=EX(symbol_table)) {							\
		zend_hash_apply(EX(symbol_table), zend_check_symbol TSRMLS_CC);	\
	}

static int zend_check_symbol(zval *pz TSRMLS_DC)
{
	if (Z_TYPE_P(pz) == IS_INDIRECT) {
		pz = Z_INDIRECT_P(pz);
	}
	if (Z_TYPE_P(pz) > 10) {
		fprintf(stderr, "Warning!  %x has invalid type!\n", *pz);
/* See http://support.microsoft.com/kb/190351 */
#ifdef PHP_WIN32
		fflush(stderr);
#endif
	} else if (Z_TYPE_P(pz) == IS_ARRAY) {
		zend_hash_apply(Z_ARRVAL_P(pz), zend_check_symbol TSRMLS_CC);
	} else if (Z_TYPE_P(pz) == IS_OBJECT) {
		/* OBJ-TBI - doesn't support new object model! */
		zend_hash_apply(Z_OBJPROP_P(pz), zend_check_symbol TSRMLS_CC);
	}

	return 0;
}


#else
#define CHECK_SYMBOL_TABLES()
#endif

ZEND_API opcode_handler_t *zend_opcode_handlers;

ZEND_API void execute_internal(zend_execute_data *execute_data, zval *return_value TSRMLS_DC)
{
	execute_data->func->internal_function.handler(execute_data->num_args, return_value TSRMLS_CC);
}

void zend_clean_and_cache_symbol_table(zend_array *symbol_table TSRMLS_DC) /* {{{ */
{
	if (EG(symtable_cache_ptr) >= EG(symtable_cache_limit)) {
		zend_hash_destroy(&symbol_table->ht);
		FREE_HASHTABLE(symbol_table);
	} else {
		/* clean before putting into the cache, since clean
		   could call dtors, which could use cached hash */
		zend_hash_clean(&symbol_table->ht);
		*(++EG(symtable_cache_ptr)) = symbol_table;
	}
}
/* }}} */

static zend_always_inline void i_free_compiled_variables(zend_execute_data *execute_data TSRMLS_DC) /* {{{ */
{
	if (EXPECTED(EX(func)->op_array.last_var > 0)) {
		zval *cv = EX_VAR_NUM(0);
		zval *end = cv + EX(func)->op_array.last_var;
		do {
			zval_ptr_dtor(cv);
			cv++;
	 	} while (cv != end);
	}
}
/* }}} */

void zend_free_compiled_variables(zend_execute_data *execute_data TSRMLS_DC) /* {{{ */
{
	i_free_compiled_variables(execute_data TSRMLS_CC);
}
/* }}} */

/*
 * Stack Frame Layout (the whole stack frame is allocated at once)
 * ==================
 *
 *                             +========================================+
 * EG(current_execute_data) -> | zend_execute_data                      |
 *                             +----------------------------------------+
 *     EX_CV_NUM(0) ---------> | VAR[0] = ARG[1]                        |
 *                             | ...                                    |
 *                             | VAR[op_array->num_args-1] = ARG[N]     |
 *                             | ...                                    |
 *                             | VAR[op_array->last_var-1]              |
 *                             | VAR[op_array->last_var] = TMP[0]       |
 *                             | ...                                    |
 *                             | VAR[op_array->last_var+op_array->T-1]  |
 *                             | ARG[N+1] (extra_args)                  |
 *                             | ...                                    |
 *                             +----------------------------------------+
 */

static zend_always_inline void i_init_func_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value, vm_frame_kind frame_kind TSRMLS_DC) /* {{{ */
{
	zend_uint first_extra_arg;
	ZEND_ASSERT(EX(func) == (zend_function*)op_array);
	ZEND_ASSERT(EX(object) == Z_OBJ(EG(This)));

	EX(return_value) = return_value;
	EX(frame_kind) = frame_kind;
	ZVAL_UNDEF(&EX(old_error_reporting));
	EX(delayed_exception) = NULL;
	EX(call) = NULL;

	EX(opline) = op_array->opcodes;
	if (EXPECTED((op_array->fn_flags & ZEND_ACC_HAS_TYPE_HINTS) == 0)) {
		/* Skip useless ZEND_RECV opcodes */
		EX(opline) += MIN(EX(num_args), op_array->required_num_args);
	}
	EX(scope) = EG(scope);

	first_extra_arg = op_array->num_args;

	if (UNEXPECTED((op_array->fn_flags & ZEND_ACC_VARIADIC) != 0)) {
		first_extra_arg--;
	}
	if (UNEXPECTED(EX(num_args) > first_extra_arg)) {
		/* move extra args into separate array after all CV and TMP vars */
		zval *extra_args = EX_VAR_NUM(op_array->last_var + op_array->T);

		memmove(extra_args, EX_VAR_NUM(first_extra_arg), sizeof(zval) * (EX(num_args) - first_extra_arg));
	}

	do {
		/* Initialize CV variables (skip arguments) */
		int num_args = MIN(op_array->num_args, EX(num_args));

		if (EXPECTED(num_args < op_array->last_var)) {
			zval *var = EX_VAR_NUM(num_args);
			zval *end = EX_VAR_NUM(op_array->last_var);

			do {
				ZVAL_UNDEF(var);
				var++;
			} while (var != end);
		}
	} while (0);

	if (op_array->this_var != -1 && EX(object)) {
		ZVAL_OBJ(EX_VAR(op_array->this_var), EX(object));
		GC_REFCOUNT(EX(object))++;
	}

	if (!op_array->run_time_cache && op_array->last_cache_slot) {
		if (op_array->function_name) {
			op_array->run_time_cache = zend_arena_calloc(&CG(arena), op_array->last_cache_slot, sizeof(void*));
		} else {
			op_array->run_time_cache = ecalloc(op_array->last_cache_slot, sizeof(void*));
		}
	}
	EX(run_time_cache) = op_array->run_time_cache;

	EG(current_execute_data) = execute_data;
}
/* }}} */

static zend_always_inline void i_init_code_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value, vm_frame_kind frame_kind TSRMLS_DC) /* {{{ */
{
	ZEND_ASSERT(EX(func) == (zend_function*)op_array);
	ZEND_ASSERT(EX(object) == Z_OBJ(EG(This)));

	EX(return_value) = return_value;
	EX(frame_kind) = frame_kind;
	ZVAL_UNDEF(&EX(old_error_reporting));
	EX(delayed_exception) = NULL;
	EX(call) = NULL;

	EX(opline) = UNEXPECTED((op_array->fn_flags & ZEND_ACC_INTERACTIVE) != 0) && EG(start_op) ? EG(start_op) : op_array->opcodes;
	EX(scope) = EG(scope);

	zend_attach_symbol_table(execute_data);

	if (!op_array->run_time_cache && op_array->last_cache_slot) {
		if (op_array->function_name) {
			op_array->run_time_cache = zend_arena_calloc(&CG(arena), op_array->last_cache_slot, sizeof(void*));
		} else {
			op_array->run_time_cache = ecalloc(op_array->last_cache_slot, sizeof(void*));
		}
	}
	EX(run_time_cache) = op_array->run_time_cache;

	EG(current_execute_data) = execute_data;
}
/* }}} */

static zend_always_inline void i_init_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value, vm_frame_kind frame_kind TSRMLS_DC) /* {{{ */
{
	ZEND_ASSERT(EX(func) == (zend_function*)op_array);
	ZEND_ASSERT(EX(object) == Z_OBJ(EG(This)));

	EX(return_value) = return_value;
	EX(frame_kind) = frame_kind;
	ZVAL_UNDEF(&EX(old_error_reporting));
	EX(delayed_exception) = NULL;
	EX(call) = NULL;

	EX(opline) = UNEXPECTED((op_array->fn_flags & ZEND_ACC_INTERACTIVE) != 0) && EG(start_op) ? EG(start_op) : op_array->opcodes;
	EX(scope) = EG(scope);

	if (UNEXPECTED(EX(symbol_table) != NULL)) {
		zend_attach_symbol_table(execute_data);
	} else {
		zend_uint first_extra_arg = op_array->num_args;
		
		if (UNEXPECTED((op_array->fn_flags & ZEND_ACC_VARIADIC) != 0)) {
			first_extra_arg--;
		}
		if (UNEXPECTED(EX(num_args) > first_extra_arg)) {
			/* move extra args into separate array after all CV and TMP vars */
			zval *extra_args = EX_VAR_NUM(op_array->last_var + op_array->T);

			memmove(extra_args, EX_VAR_NUM(first_extra_arg), sizeof(zval) * (EX(num_args) - first_extra_arg));
		}

		do {
			/* Initialize CV variables (skip arguments) */
			int num_args = MIN(op_array->num_args, EX(num_args));

			if (EXPECTED(num_args < op_array->last_var)) {
				zval *var = EX_VAR_NUM(num_args);
				zval *end = EX_VAR_NUM(op_array->last_var);

				do {
					ZVAL_UNDEF(var);
					var++;
				} while (var != end);
			}
		} while (0);

		if (op_array->this_var != -1 && EX(object)) {
			ZVAL_OBJ(EX_VAR(op_array->this_var), EX(object));
			GC_REFCOUNT(EX(object))++;
		}
	}

	if (!op_array->run_time_cache && op_array->last_cache_slot) {
		if (op_array->function_name) {
			op_array->run_time_cache = zend_arena_calloc(&CG(arena), op_array->last_cache_slot, sizeof(void*));
		} else {
			op_array->run_time_cache = ecalloc(op_array->last_cache_slot, sizeof(void*));
		}
	}
	EX(run_time_cache) = op_array->run_time_cache;

	EG(current_execute_data) = execute_data;
}
/* }}} */

ZEND_API zend_execute_data *zend_create_generator_execute_data(zend_execute_data *call, zend_op_array *op_array, zval *return_value TSRMLS_DC) /* {{{ */
{
	/*
	 * Normally the execute_data is allocated on the VM stack (because it does
	 * not actually do any allocation and thus is faster). For generators
	 * though this behavior would be suboptimal, because the (rather large)
	 * structure would have to be copied back and forth every time execution is
	 * suspended or resumed. That's why for generators the execution context
	 * is allocated using a separate VM stack, thus allowing to save and
	 * restore it simply by replacing a pointer.
	 */
	zend_execute_data *execute_data;
	zend_uint num_args = call->num_args;

	EG(argument_stack) = zend_vm_stack_new_page(
		MAX(ZEND_VM_STACK_PAGE_SIZE, 
			ZEND_CALL_FRAME_SLOT + MAX(op_array->last_var + op_array->T, num_args)));
	EG(argument_stack)->prev = NULL;

	execute_data = zend_vm_stack_push_call_frame(
		(zend_function*)op_array,
		num_args,
		call->flags,
		call->called_scope,
		call->object,
		NULL TSRMLS_CC);
	EX(num_args) = num_args;

	/* copy arguments */
	if (num_args > 0) {
		zval *arg_src = ZEND_CALL_ARG(call, 1);
		zval *arg_dst = ZEND_CALL_ARG(execute_data, 1);
		int i;

		for (i = 0; i < num_args; i++) {
			ZVAL_COPY_VALUE(arg_dst + i, arg_src + i);
		}
	}

	EX(symbol_table) = NULL;

	i_init_func_execute_data(execute_data, op_array, return_value, VM_FRAME_TOP_FUNCTION TSRMLS_CC);

	return execute_data;
}
/* }}} */

ZEND_API void zend_init_execute_data(zend_execute_data *execute_data, zend_op_array *op_array, zval *return_value, vm_frame_kind frame_kind TSRMLS_DC) /* {{{ */
{
	EX(prev_execute_data) = EG(current_execute_data);
	i_init_execute_data(execute_data, op_array, return_value, frame_kind TSRMLS_CC);
}
/* }}} */

static zend_always_inline zend_bool zend_is_by_ref_func_arg_fetch(zend_op *opline, zend_execute_data *call TSRMLS_DC) /* {{{ */
{
	zend_uint arg_num = opline->extended_value & ZEND_FETCH_ARG_MASK;
	return ARG_SHOULD_BE_SENT_BY_REF(call->func, arg_num);
}
/* }}} */

static zend_execute_data *zend_vm_stack_copy_call_frame(zend_execute_data *call, zend_uint passed_args, zend_uint additional_args TSRMLS_DC) /* {{{ */
{
	zend_execute_data *new_call;
	int used_stack = (EG(argument_stack)->top - (zval*)call) + additional_args;
		
	/* copy call frame into new stack segment */
	zend_vm_stack_extend(used_stack TSRMLS_CC);
	new_call = (zend_execute_data*)EG(argument_stack)->top;
	EG(argument_stack)->top += used_stack;		
	*new_call = *call;
	if (passed_args) {
		zval *src = ZEND_CALL_ARG(call, 1);
		zval *dst = ZEND_CALL_ARG(new_call, 1);
		do {
			ZVAL_COPY_VALUE(dst, src);
			passed_args--;
			src++;
			dst++;
		} while (passed_args);
	}

	/* delete old call_frame from previous stack segment */
	EG(argument_stack)->prev->top = (zval*)call;

	/* delete previous stack segment if it becames empty */
	if (UNEXPECTED(EG(argument_stack)->prev->top == ZEND_VM_STACK_ELEMETS(EG(argument_stack)->prev))) {
		zend_vm_stack r = EG(argument_stack)->prev;

		EG(argument_stack)->prev = r->prev;
		efree(r);
	}

	return new_call;
}
/* }}} */

static zend_always_inline void zend_vm_stack_extend_call_frame(zend_execute_data **call, zend_uint passed_args, zend_uint additional_args TSRMLS_DC) /* {{{ */
{
	if (EXPECTED(EG(argument_stack)->end - EG(argument_stack)->top > additional_args)) {
		EG(argument_stack)->top += additional_args;
	} else {
		*call = zend_vm_stack_copy_call_frame(*call, passed_args, additional_args TSRMLS_CC);
	}
}
/* }}} */

#define ZEND_VM_NEXT_OPCODE() \
	CHECK_SYMBOL_TABLES() \
	ZEND_VM_INC_OPCODE(); \
	ZEND_VM_CONTINUE()

#define ZEND_VM_SET_OPCODE(new_op) \
	CHECK_SYMBOL_TABLES() \
	OPLINE = new_op

#define ZEND_VM_SET_RELATIVE_OPCODE(opline, offset) \
	CHECK_SYMBOL_TABLES() \
	OPLINE = ((zend_op*)(((char*)opline)+(offset)))

#define ZEND_VM_JMP(new_op) \
	if (EXPECTED(!EG(exception))) { \
		ZEND_VM_SET_OPCODE(new_op); \
	} else { \
		LOAD_OPLINE(); \
	} \
	ZEND_VM_CONTINUE()

#define ZEND_VM_INC_OPCODE() \
	OPLINE++

#ifdef __GNUC__
# define ZEND_VM_GUARD(name) __asm__("#" #name)
#else
# define ZEND_VM_GUARD(name)
#endif

#include "zend_vm_execute.h"

ZEND_API int zend_set_user_opcode_handler(zend_uchar opcode, user_opcode_handler_t handler)
{
	if (opcode != ZEND_USER_OPCODE) {
		if (handler == NULL) {
			/* restore the original handler */
			zend_user_opcodes[opcode] = opcode;
		} else {
			zend_user_opcodes[opcode] = ZEND_USER_OPCODE;
		}
		zend_user_opcode_handlers[opcode] = handler;
		return SUCCESS;
	}
	return FAILURE;
}

ZEND_API user_opcode_handler_t zend_get_user_opcode_handler(zend_uchar opcode)
{
	return zend_user_opcode_handlers[opcode];
}

ZEND_API zval *zend_get_zval_ptr(int op_type, const znode_op *node, const zend_execute_data *execute_data, zend_free_op *should_free, int type TSRMLS_DC) {
	return get_zval_ptr(op_type, node, execute_data, should_free, type);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
