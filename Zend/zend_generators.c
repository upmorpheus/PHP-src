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
   | Authors: Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"
#include "zend_interfaces.h"
#include "zend_exceptions.h"
#include "zend_generators.h"

ZEND_API zend_class_entry *zend_ce_generator;
static zend_object_handlers zend_generator_handlers;

static zend_object *zend_generator_create(zend_class_entry *class_type TSRMLS_DC);

static void zend_generator_cleanup_unfinished_execution(zend_generator *generator TSRMLS_DC) /* {{{ */
{
	zend_execute_data *execute_data = generator->execute_data;
	zend_op_array *op_array = &execute_data->func->op_array;

	if (generator->send_target) {
		if (Z_REFCOUNTED_P(generator->send_target)) Z_DELREF_P(generator->send_target);
		generator->send_target = NULL;
	}

	/* Manually free loop variables, as execution couldn't reach their
	 * SWITCH_FREE / FREE opcodes. */
	{
		/* -1 required because we want the last run opcode, not the
		 * next to-be-run one. */
		uint32_t op_num = execute_data->opline - op_array->opcodes - 1;

		int i;
		for (i = 0; i < op_array->last_brk_cont; ++i) {
			zend_brk_cont_element *brk_cont = op_array->brk_cont_array + i;

			if (brk_cont->start < 0) {
				continue;
			} else if (brk_cont->start > op_num) {
				break;
			} else if (brk_cont->brk > op_num) {
				zend_op *brk_opline = op_array->opcodes + brk_cont->brk;

				if (brk_opline->opcode == ZEND_FREE) {
					zval *var = EX_VAR_2(execute_data, brk_opline->op1.var);
					zval_ptr_dtor_nogc(var);
				}
			}
		}
	}

	/* If yield was used as a function argument there may be active
	 * method calls those objects need to be freed */
	while (execute_data->call) {
		if (execute_data->call->object) {
			OBJ_RELEASE(execute_data->call->object);
		}
		execute_data->call = execute_data->call->prev_nested_call;
	}
}
/* }}} */

ZEND_API void zend_generator_close(zend_generator *generator, zend_bool finished_execution TSRMLS_DC) /* {{{ */
{
	if (Z_TYPE(generator->value) != IS_UNDEF) {
		zval_ptr_dtor(&generator->value);
		ZVAL_UNDEF(&generator->value);
	}

	if (Z_TYPE(generator->key) != IS_UNDEF) {
		zval_ptr_dtor(&generator->key);
		ZVAL_UNDEF(&generator->key);
	}

	if (generator->execute_data) {
		zend_execute_data *execute_data = generator->execute_data;
		zend_op_array *op_array = &execute_data->func->op_array;

		if (!execute_data->symbol_table) {
			zend_free_compiled_variables(execute_data TSRMLS_CC);
		} else {
			zend_clean_and_cache_symbol_table(execute_data->symbol_table TSRMLS_CC);
		}

		if (execute_data->object) {
			OBJ_RELEASE(execute_data->object);
		}

		/* A fatal error / die occurred during the generator execution. Trying to clean
		 * up the stack may not be safe in this case. */
		if (CG(unclean_shutdown)) {
			generator->execute_data = NULL;
			return;
		}

		zend_vm_stack_free_extra_args(generator->execute_data TSRMLS_CC);

		/* Some cleanups are only necessary if the generator was closued
		 * before it could finish execution (reach a return statement). */
		if (!finished_execution) {
			zend_generator_cleanup_unfinished_execution(generator TSRMLS_CC);
		}

		/* Free a clone of closure */
		if (op_array->fn_flags & ZEND_ACC_CLOSURE) {
			destroy_op_array(op_array TSRMLS_CC);
			efree_size(op_array, sizeof(zend_op_array));
		}

		efree(generator->stack);
		generator->execute_data = NULL;
	}
}
/* }}} */

static void zend_generator_dtor_storage(zend_object *object TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*) object;
	zend_execute_data *ex = generator->execute_data;
	uint32_t op_num, finally_op_num;
	int i;

	if (!ex || !(ex->func->op_array.fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK)) {
		return;
	}

	/* -1 required because we want the last run opcode, not the
	 * next to-be-run one. */
	op_num = ex->opline - ex->func->op_array.opcodes - 1;

	/* Find next finally block */
	finally_op_num = 0;
	for (i = 0; i < ex->func->op_array.last_try_catch; i++) {
		zend_try_catch_element *try_catch = &ex->func->op_array.try_catch_array[i];

		if (op_num < try_catch->try_op) {
			break;
		}

		if (op_num < try_catch->finally_op) {
			finally_op_num = try_catch->finally_op;
		}
	}

	/* If a finally block was found we jump directly to it and
	 * resume the generator. */
	if (finally_op_num) {
		ex->opline = &ex->func->op_array.opcodes[finally_op_num];
		ex->fast_ret = NULL;
		generator->flags |= ZEND_GENERATOR_FORCED_CLOSE;
		zend_generator_resume(generator TSRMLS_CC);
	}
}
/* }}} */

static void zend_generator_free_storage(zend_object *object TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*) object;

	zend_generator_close(generator, 0 TSRMLS_CC);

	zend_object_std_dtor(&generator->std TSRMLS_CC);

	if (generator->iterator) {
		zend_iterator_dtor(generator->iterator TSRMLS_CC);
	}
}
/* }}} */

static zend_object *zend_generator_create(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	zend_generator *generator;

	generator = emalloc(sizeof(zend_generator));
	memset(generator, 0, sizeof(zend_generator));

	/* The key will be incremented on first use, so it'll start at 0 */
	generator->largest_used_integer_key = -1;

	zend_object_std_init(&generator->std, class_type TSRMLS_CC);
	generator->std.handlers = &zend_generator_handlers;

	return (zend_object*)generator;
}
/* }}} */

static int copy_closure_static_var(zval *var TSRMLS_DC, int num_args, va_list args, zend_hash_key *key) /* {{{ */
{
	HashTable *target = va_arg(args, HashTable *);

	ZVAL_MAKE_REF(var);
	Z_ADDREF_P(var);
	zend_hash_update(target, key->key, var);
	return 0;
}
/* }}} */

/* Requires globals EG(scope), EG(This) and EG(current_execute_data). */
ZEND_API void zend_generator_create_zval(zend_execute_data *call, zend_op_array *op_array, zval *return_value TSRMLS_DC) /* {{{ */
{
	zend_generator *generator;
	zend_execute_data *current_execute_data;
	zend_execute_data *execute_data;
	zend_vm_stack current_stack = EG(argument_stack);

	/* Create a clone of closure, because it may be destroyed */
	if (op_array->fn_flags & ZEND_ACC_CLOSURE) {
		zend_op_array *op_array_copy = (zend_op_array*)emalloc(sizeof(zend_op_array));
		*op_array_copy = *op_array;

		(*op_array->refcount)++;
		op_array->run_time_cache = NULL;
		if (op_array->static_variables) {
			ALLOC_HASHTABLE(op_array_copy->static_variables);
			zend_hash_init(
				op_array_copy->static_variables, 
				zend_hash_num_elements(op_array->static_variables),
				NULL, ZVAL_PTR_DTOR, 0
			);
			zend_hash_apply_with_arguments(
				op_array->static_variables TSRMLS_CC,
				copy_closure_static_var, 1, 
				op_array_copy->static_variables
			);
		}

		op_array = op_array_copy;
	}
	
	/* Create new execution context. We have to back up and restore
	 * EG(current_execute_data) here. */
	current_execute_data = EG(current_execute_data);
	execute_data = zend_create_generator_execute_data(call, op_array, return_value TSRMLS_CC);
	EG(current_execute_data) = current_execute_data;

	object_init_ex(return_value, zend_ce_generator);

	if (Z_OBJ(EG(This))) {
		Z_ADDREF(EG(This));
	}

	/* Save execution context in generator object. */
	generator = (zend_generator *) Z_OBJ_P(return_value);
	execute_data->prev_execute_data = NULL;
	generator->execute_data = execute_data;
	generator->stack = EG(argument_stack);
	EG(argument_stack) = current_stack;

	/* EX(return_value) keeps pointer to zend_object (not a real zval) */
	execute_data->return_value = (zval*)generator;
}
/* }}} */

static zend_function *zend_generator_get_constructor(zend_object *object TSRMLS_DC) /* {{{ */
{
	zend_error(E_RECOVERABLE_ERROR, "The \"Generator\" class is reserved for internal use and cannot be manually instantiated");

	return NULL;
}
/* }}} */

ZEND_API void zend_generator_resume(zend_generator *generator TSRMLS_DC) /* {{{ */
{
	/* The generator is already closed, thus can't resume */
	if (!generator->execute_data) {
		return;
	}

	if (generator->flags & ZEND_GENERATOR_CURRENTLY_RUNNING) {
		zend_error(E_ERROR, "Cannot resume an already running generator");
	}

	/* Drop the AT_FIRST_YIELD flag */
	generator->flags &= ~ZEND_GENERATOR_AT_FIRST_YIELD;

	{
		/* Backup executor globals */
		zend_execute_data *original_execute_data = EG(current_execute_data);
		zend_object *original_This;
		zend_class_entry *original_scope = EG(scope);
		zend_vm_stack original_stack = EG(argument_stack);

		original_This = Z_OBJ(EG(This));

		/* Set executor globals */
		EG(current_execute_data) = generator->execute_data;
		Z_OBJ(EG(This)) = generator->execute_data->object;
		EG(scope) = generator->execute_data->scope;
		EG(argument_stack) = generator->stack;

		/* We want the backtrace to look as if the generator function was
		 * called from whatever method we are current running (e.g. next()).
		 * So we have to link generator call frame with caller call frames */

		generator->execute_data->prev_execute_data = original_execute_data;

		/* Resume execution */
		generator->flags |= ZEND_GENERATOR_CURRENTLY_RUNNING;
		zend_execute_ex(generator->execute_data TSRMLS_CC);
		generator->flags &= ~ZEND_GENERATOR_CURRENTLY_RUNNING;

		/* Unlink generator call_frame from the caller */
		if (generator->execute_data) {
			generator->execute_data->prev_execute_data = NULL;
		}

		/* Restore executor globals */
		EG(current_execute_data) = original_execute_data;
		Z_OBJ(EG(This)) = original_This;
		EG(scope) = original_scope;
		EG(argument_stack) = original_stack;

		/* If an exception was thrown in the generator we have to internally
		 * rethrow it in the parent scope. */
		if (UNEXPECTED(EG(exception) != NULL)) {
			zend_throw_exception_internal(NULL TSRMLS_CC);
		}
	}
}
/* }}} */

static void zend_generator_ensure_initialized(zend_generator *generator TSRMLS_DC) /* {{{ */
{
	if (generator->execute_data && Z_TYPE(generator->value) == IS_UNDEF) {
		zend_generator_resume(generator TSRMLS_CC);
		generator->flags |= ZEND_GENERATOR_AT_FIRST_YIELD;
	}
}
/* }}} */

static void zend_generator_rewind(zend_generator *generator TSRMLS_DC) /* {{{ */
{
	zend_generator_ensure_initialized(generator TSRMLS_CC);

	if (!(generator->flags & ZEND_GENERATOR_AT_FIRST_YIELD)) {
		zend_throw_exception(NULL, "Cannot rewind a generator that was already run", 0 TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto void Generator::rewind()
 * Rewind the generator */
ZEND_METHOD(Generator, rewind)
{
	zend_generator *generator;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_rewind(generator TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Generator::valid()
 * Check whether the generator is valid */
ZEND_METHOD(Generator, valid)
{
	zend_generator *generator;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	RETURN_BOOL(Z_TYPE(generator->value) != IS_UNDEF);
}
/* }}} */

/* {{{ proto mixed Generator::current()
 * Get the current value */
ZEND_METHOD(Generator, current)
{
	zend_generator *generator;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	if (Z_TYPE(generator->value) != IS_UNDEF) {
		RETURN_ZVAL_FAST(&generator->value);
	}
}
/* }}} */

/* {{{ proto mixed Generator::key()
 * Get the current key */
ZEND_METHOD(Generator, key)
{
	zend_generator *generator;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	if (Z_TYPE(generator->key) != IS_UNDEF) {
		RETURN_ZVAL_FAST(&generator->key);
	}
}
/* }}} */

/* {{{ proto void Generator::next()
 * Advances the generator */
ZEND_METHOD(Generator, next)
{
	zend_generator *generator;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	zend_generator_resume(generator TSRMLS_CC);
}
/* }}} */

/* {{{ proto mixed Generator::send(mixed $value)
 * Sends a value to the generator */
ZEND_METHOD(Generator, send)
{
	zval *value;
	zend_generator *generator;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
		return;
	}

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC); 

	/* The generator is already closed, thus can't send anything */
	if (!generator->execute_data) {
		return;
	}

	/* Put sent value in the target VAR slot, if it is used */
	if (generator->send_target) {
		if (Z_REFCOUNTED_P(generator->send_target)) Z_DELREF_P(generator->send_target);
		ZVAL_COPY(generator->send_target, value);
	}

	zend_generator_resume(generator TSRMLS_CC);

	if (Z_TYPE(generator->value) != IS_UNDEF) {
		RETURN_ZVAL_FAST(&generator->value);
	}
}
/* }}} */

/* {{{ proto mixed Generator::throw(Exception $exception)
 * Throws an exception into the generator */
ZEND_METHOD(Generator, throw)
{
	zval *exception, exception_copy;
	zend_generator *generator;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &exception) == FAILURE) {
		return;
	}

	ZVAL_DUP(&exception_copy, exception);

	generator = (zend_generator *) Z_OBJ_P(getThis());

	zend_generator_ensure_initialized(generator TSRMLS_CC); 

	if (generator->execute_data) {
		/* Throw the exception in the context of the generator */
		zend_execute_data *current_execute_data = EG(current_execute_data);
		EG(current_execute_data) = generator->execute_data;

		zend_throw_exception_object(&exception_copy TSRMLS_CC);

		EG(current_execute_data) = current_execute_data;

		zend_generator_resume(generator TSRMLS_CC);

		if (Z_TYPE(generator->value) != IS_UNDEF) {
			RETURN_ZVAL_FAST(&generator->value);
		}
	} else {
		/* If the generator is already closed throw the exception in the
		 * current context */
		zend_throw_exception_object(&exception_copy TSRMLS_CC);
	}
}
/* }}} */

/* {{{ proto void Generator::__wakeup()
 * Throws an Exception as generators can't be serialized */
ZEND_METHOD(Generator, __wakeup)
{
	/* Just specifying the zend_class_unserialize_deny handler is not enough,
	 * because it is only invoked for C unserialization. For O the error has
	 * to be thrown in __wakeup. */

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zend_throw_exception(NULL, "Unserialization of 'Generator' is not allowed", 0 TSRMLS_CC);
}
/* }}} */

/* get_iterator implementation */

static void zend_generator_iterator_dtor(zend_object_iterator *iterator TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);
	generator->iterator = NULL;
	zval_ptr_dtor(&iterator->data);
	zend_iterator_dtor(iterator TSRMLS_CC);
}
/* }}} */

static int zend_generator_iterator_valid(zend_object_iterator *iterator TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	return Z_TYPE(generator->value) != IS_UNDEF ? SUCCESS : FAILURE;
}
/* }}} */

static zval *zend_generator_iterator_get_data(zend_object_iterator *iterator TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	return &generator->value;
}
/* }}} */

static void zend_generator_iterator_get_key(zend_object_iterator *iterator, zval *key TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	if (Z_TYPE(generator->key) != IS_UNDEF) {
		ZVAL_ZVAL(key, &generator->key, 1, 0);
	} else {
		ZVAL_NULL(key);
	}
}
/* }}} */

static void zend_generator_iterator_move_forward(zend_object_iterator *iterator TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);

	zend_generator_ensure_initialized(generator TSRMLS_CC);

	zend_generator_resume(generator TSRMLS_CC);
}
/* }}} */

static void zend_generator_iterator_rewind(zend_object_iterator *iterator TSRMLS_DC) /* {{{ */
{
	zend_generator *generator = (zend_generator*)Z_OBJ(iterator->data);

	zend_generator_rewind(generator TSRMLS_CC);
}
/* }}} */

static zend_object_iterator_funcs zend_generator_iterator_functions = {
	zend_generator_iterator_dtor,
	zend_generator_iterator_valid,
	zend_generator_iterator_get_data,
	zend_generator_iterator_get_key,
	zend_generator_iterator_move_forward,
	zend_generator_iterator_rewind
};

zend_object_iterator *zend_generator_get_iterator(zend_class_entry *ce, zval *object, int by_ref TSRMLS_DC) /* {{{ */
{
	zend_object_iterator *iterator;
	zend_generator *generator = (zend_generator*)Z_OBJ_P(object);

	if (!generator->execute_data) {
		zend_throw_exception(NULL, "Cannot traverse an already closed generator", 0 TSRMLS_CC);
		return NULL;
	}

	if (by_ref && !(generator->execute_data->func->op_array.fn_flags & ZEND_ACC_RETURN_REFERENCE)) {
		zend_throw_exception(NULL, "You can only iterate a generator by-reference if it declared that it yields by-reference", 0 TSRMLS_CC);
		return NULL;
	}

	iterator = generator->iterator = emalloc(sizeof(zend_object_iterator));
	
	zend_iterator_init(iterator TSRMLS_CC);

	iterator->funcs = &zend_generator_iterator_functions;
	ZVAL_COPY(&iterator->data, object);

	return iterator;
}
/* }}} */

ZEND_BEGIN_ARG_INFO(arginfo_generator_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_generator_send, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_generator_throw, 0, 0, 1)
	ZEND_ARG_INFO(0, exception)
ZEND_END_ARG_INFO()

static const zend_function_entry generator_functions[] = {
	ZEND_ME(Generator, rewind,   arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, valid,    arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, current,  arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, key,      arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, next,     arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, send,     arginfo_generator_send, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, throw,    arginfo_generator_throw, ZEND_ACC_PUBLIC)
	ZEND_ME(Generator, __wakeup, arginfo_generator_void, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

void zend_register_generator_ce(TSRMLS_D) /* {{{ */
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Generator", generator_functions);
	zend_ce_generator = zend_register_internal_class(&ce TSRMLS_CC);
	zend_ce_generator->ce_flags |= ZEND_ACC_FINAL_CLASS;
	zend_ce_generator->create_object = zend_generator_create;
	zend_ce_generator->serialize = zend_class_serialize_deny;
	zend_ce_generator->unserialize = zend_class_unserialize_deny;

	/* get_iterator has to be assigned *after* implementing the inferface */
	zend_class_implements(zend_ce_generator TSRMLS_CC, 1, zend_ce_iterator);
	zend_ce_generator->get_iterator = zend_generator_get_iterator;
	zend_ce_generator->iterator_funcs.funcs = &zend_generator_iterator_functions;

	memcpy(&zend_generator_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	zend_generator_handlers.free_obj = zend_generator_free_storage;
	zend_generator_handlers.dtor_obj = zend_generator_dtor_storage;
	zend_generator_handlers.clone_obj = NULL;
	zend_generator_handlers.get_constructor = zend_generator_get_constructor;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
