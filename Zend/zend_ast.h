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
   | Authors: Bob Weinand <bwoebi@php.net>                                |
   |          Dmitry Stogov <dmitry@zend.com>                             |
   |          Nikita Popov <nikic@php.net>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_AST_H
#define ZEND_AST_H

#include "zend.h"

#define ZEND_AST_SPECIAL_SHIFT      6
#define ZEND_AST_IS_LIST_SHIFT      7
#define ZEND_AST_NUM_CHILDREN_SHIFT 8

enum _zend_ast_kind {
	/* special nodes */
	ZEND_AST_ZVAL = 1 << ZEND_AST_SPECIAL_SHIFT,
	ZEND_AST_ZNODE,

	/* declaration nodes */
	ZEND_AST_FUNC_DECL,
	ZEND_AST_CLOSURE,
	ZEND_AST_METHOD,
	ZEND_AST_CLASS,

	/* list nodes */
	ZEND_AST_ARG_LIST = 1 << ZEND_AST_IS_LIST_SHIFT,
	ZEND_AST_LIST,
	ZEND_AST_ARRAY,
	ZEND_AST_ENCAPS_LIST,
	ZEND_AST_EXPR_LIST,
	ZEND_AST_STMT_LIST,
	ZEND_AST_IF,
	ZEND_AST_SWITCH_LIST,
	ZEND_AST_CATCH_LIST,
	ZEND_AST_PARAM_LIST,
	ZEND_AST_CLOSURE_USES,
	ZEND_AST_PROP_DECL,
	ZEND_AST_CONST_DECL,
	ZEND_AST_CLASS_CONST_DECL,
	ZEND_AST_NAME_LIST,
	ZEND_AST_TRAIT_ADAPTATIONS,
	ZEND_AST_USE,

	/* 0 child nodes */
	ZEND_AST_MAGIC_CONST = 0 << ZEND_AST_NUM_CHILDREN_SHIFT,
	ZEND_AST_TYPE,

	/* 1 child node */
	ZEND_AST_VAR = 1 << ZEND_AST_NUM_CHILDREN_SHIFT,
	ZEND_AST_CONST,
	ZEND_AST_RESOLVE_CLASS_NAME,
	ZEND_AST_UNPACK,
	ZEND_AST_UNARY_PLUS,
	ZEND_AST_UNARY_MINUS,
	ZEND_AST_CAST,
	ZEND_AST_EMPTY,
	ZEND_AST_ISSET,
	ZEND_AST_SILENCE,
	ZEND_AST_SHELL_EXEC,
	ZEND_AST_CLONE,
	ZEND_AST_EXIT,
	ZEND_AST_PRINT,
	ZEND_AST_INCLUDE_OR_EVAL,
	ZEND_AST_UNARY_OP,
	ZEND_AST_PRE_INC,
	ZEND_AST_PRE_DEC,
	ZEND_AST_POST_INC,
	ZEND_AST_POST_DEC,

	ZEND_AST_GLOBAL,
	ZEND_AST_UNSET,
	ZEND_AST_RETURN,
	ZEND_AST_LABEL,
	ZEND_AST_REF,
	ZEND_AST_HALT_COMPILER,
	ZEND_AST_ECHO,
	ZEND_AST_THROW,
	ZEND_AST_GOTO,
	ZEND_AST_BREAK,
	ZEND_AST_CONTINUE,

	/* 2 child nodes */
	ZEND_AST_DIM = 2 << ZEND_AST_NUM_CHILDREN_SHIFT,
	ZEND_AST_PROP,
	ZEND_AST_STATIC_PROP,
	ZEND_AST_CALL,
	ZEND_AST_CLASS_CONST,
	ZEND_AST_ASSIGN,
	ZEND_AST_ASSIGN_REF,
	ZEND_AST_ASSIGN_OP,
	ZEND_AST_BINARY_OP,
	ZEND_AST_GREATER,
	ZEND_AST_GREATER_EQUAL,
	ZEND_AST_AND,
	ZEND_AST_OR,
	ZEND_AST_ARRAY_ELEM,
	ZEND_AST_NEW,
	ZEND_AST_INSTANCEOF,
	ZEND_AST_YIELD,

	ZEND_AST_STATIC,
	ZEND_AST_WHILE,
	ZEND_AST_DO_WHILE,
	ZEND_AST_IF_ELEM,
	ZEND_AST_SWITCH,
	ZEND_AST_SWITCH_CASE,
	ZEND_AST_DECLARE,
	ZEND_AST_PROP_ELEM,
	ZEND_AST_CONST_ELEM,
	ZEND_AST_USE_TRAIT,
	ZEND_AST_TRAIT_PRECEDENCE,
	ZEND_AST_METHOD_REFERENCE,
	ZEND_AST_NAMESPACE,
	ZEND_AST_USE_ELEM,
	ZEND_AST_TRAIT_ALIAS,

	/* 3 child nodes */
	ZEND_AST_METHOD_CALL = 3 << ZEND_AST_NUM_CHILDREN_SHIFT,
	ZEND_AST_STATIC_CALL,
	ZEND_AST_CONDITIONAL,

	ZEND_AST_TRY,
	ZEND_AST_CATCH,
	ZEND_AST_PARAM,

	/* 4 child nodes */
	ZEND_AST_FOR = 4 << ZEND_AST_NUM_CHILDREN_SHIFT,
	ZEND_AST_FOREACH,
};

typedef unsigned short zend_ast_kind;
typedef unsigned short zend_ast_attr;

struct _zend_ast {
	zend_ast_kind kind; /* Type of the node (ZEND_AST_* enum constant) */
	zend_ast_attr attr; /* Additional attribute, use depending on node type */
	zend_uint lineno;   /* Line number */
	zend_ast *child[1]; /* Array of children (using struct hack) */
};

/* Same as zend_ast, but with children count, which is updated dynamically */
typedef struct _zend_ast_list {
	zend_ast_kind kind;
	zend_ast_attr attr;
	zend_uint lineno;
	zend_uint children;
	zend_ast *child[1];
} zend_ast_list;

/* Lineno is stored in val.u2.lineno */
typedef struct _zend_ast_zval {
	zend_ast_kind kind;
	zend_ast_attr attr;
	zval val;
} zend_ast_zval;

/* Separate structure for function and class declaration, as they need extra information. */
typedef struct _zend_ast_decl {
	zend_ast_kind kind;
	zend_ast_attr attr; /* Unused - for structure compatibility */
	zend_uint start_lineno;
	zend_uint end_lineno;
	zend_uint flags;
	unsigned char *lex_pos;
	zend_string *doc_comment;
	zend_string *name;
	zend_ast *child[3];
} zend_ast_decl;

ZEND_API zend_ast *zend_ast_create_zval_ex(zval *zv, zend_ast_attr attr);

ZEND_API zend_ast *zend_ast_create_ex(
	zend_uint children, zend_ast_kind kind, zend_ast_attr attr, ...);
ZEND_API zend_ast *zend_ast_create(
	zend_uint children, zend_ast_kind kind, ...);

ZEND_API zend_ast *zend_ast_create_decl(
	zend_ast_kind kind, zend_uint flags, zend_uint start_lineno, zend_string *doc_comment,
	zend_string *name, zend_ast *child0, zend_ast *child1, zend_ast *child2
);

ZEND_API zend_ast_list *zend_ast_create_list(zend_uint init_children, zend_ast_kind kind, ...);
ZEND_API zend_ast_list *zend_ast_list_add(zend_ast_list *list, zend_ast *op);

ZEND_API void zend_ast_evaluate(zval *result, zend_ast *ast, zend_class_entry *scope TSRMLS_DC);

ZEND_API zend_ast *zend_ast_copy(zend_ast *ast);
ZEND_API void zend_ast_destroy(zend_ast *ast);
ZEND_API void zend_ast_destroy_and_free(zend_ast *ast);

typedef void (*zend_ast_apply_func)(zend_ast **ast_ptr TSRMLS_DC);
ZEND_API void zend_ast_apply(zend_ast *ast, zend_ast_apply_func fn TSRMLS_DC);

static inline zend_bool zend_ast_is_list(zend_ast *ast) {
	return (ast->kind >> ZEND_AST_IS_LIST_SHIFT) & 1;
}
static inline zend_ast_list *zend_ast_get_list(zend_ast *ast) {
	ZEND_ASSERT(zend_ast_is_list(ast));
	return (zend_ast_list *) ast;
}

static inline zval *zend_ast_get_zval(zend_ast *ast) {
	ZEND_ASSERT(ast->kind == ZEND_AST_ZVAL);
	return &((zend_ast_zval *) ast)->val;
}
static inline zend_string *zend_ast_get_str(zend_ast *ast) {
	return Z_STR_P(zend_ast_get_zval(ast));
}

static inline zend_uint zend_ast_get_num_children(zend_ast *ast) {
	ZEND_ASSERT(!zend_ast_is_list(ast));
	return ast->kind >> ZEND_AST_NUM_CHILDREN_SHIFT;
}
static inline zend_uint zend_ast_get_lineno(zend_ast *ast) {
	if (ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(ast);
		return zv->u2.lineno;
	} else {
		return ast->lineno;
	}
}

static inline zend_ast *zend_ast_create_zval(zval *zv) {
	return zend_ast_create_zval_ex(zv, 0);
}
static inline zend_ast *zend_ast_create_zval_from_str(zend_string *str) {
	zval zv;
	ZVAL_STR(&zv, str);
	return zend_ast_create_zval(&zv);
}
static inline zend_ast *zend_ast_create_zval_from_long(long lval) {
	zval zv;
	ZVAL_LONG(&zv, lval);
	return zend_ast_create_zval(&zv);
}

static inline zend_ast *zend_ast_create_unary(zend_ast_kind kind, zend_ast *op0) {
	return zend_ast_create(1, kind, op0);
}
static inline zend_ast *zend_ast_create_binary(zend_ast_kind kind, zend_ast *op0, zend_ast *op1) {
	return zend_ast_create(2, kind, op0, op1);
}
static inline zend_ast *zend_ast_create_ternary(
	zend_ast_kind kind, zend_ast *op0, zend_ast *op1, zend_ast *op2
) {
	return zend_ast_create(3, kind, op0, op1, op2);
}

static inline zend_ast *zend_ast_create_binary_op(zend_uint opcode, zend_ast *op0, zend_ast *op1) {
	return zend_ast_create_ex(2, ZEND_AST_BINARY_OP, opcode, op0, op1);
}
static inline zend_ast *zend_ast_create_assign_op(zend_uint opcode, zend_ast *op0, zend_ast *op1) {
	return zend_ast_create_ex(2, ZEND_AST_ASSIGN_OP, opcode, op0, op1);
}
static inline zend_ast *zend_ast_create_cast(zend_uint type, zend_ast *op0) {
	return zend_ast_create_ex(1, ZEND_AST_CAST, type, op0);
}

#endif
