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

#ifndef ZEND_TYPES_H
#define ZEND_TYPES_H

typedef unsigned char zend_bool;
typedef unsigned char zend_uchar;
typedef unsigned int zend_uint;
typedef unsigned long zend_ulong;
typedef unsigned short zend_ushort;

#define HAVE_ZEND_LONG64
#ifdef ZEND_WIN32
typedef __int64 zend_long64;
typedef unsigned __int64 zend_ulong64;
#elif SIZEOF_LONG_LONG_INT == 8
typedef long long int zend_long64;
typedef unsigned long long int zend_ulong64;
#elif SIZEOF_LONG_LONG == 8
typedef long long zend_long64;
typedef unsigned long long zend_ulong64;
#else
# undef HAVE_ZEND_LONG64
#endif

#ifdef _WIN64
typedef __int64 zend_intptr_t;
typedef unsigned __int64 zend_uintptr_t;
#else
typedef long zend_intptr_t;
typedef unsigned long zend_uintptr_t;
#endif

typedef struct _zend_object_handlers zend_object_handlers;
typedef struct _zend_class_entry     zend_class_entry;
typedef union  _zend_function        zend_function;

typedef struct _zval_struct     zval;

typedef struct _zend_refcounted zend_refcounted;
typedef struct _zend_string     zend_string;
typedef struct _zend_array      zend_array;
typedef struct _zend_object     zend_object;
typedef struct _zend_resource   zend_resource;
typedef struct _zend_reference  zend_reference;
typedef struct _zend_ast_ref    zend_ast_ref;
typedef struct _zend_ast        zend_ast;
typedef struct _zend_str_offset zend_str_offset;

typedef int  (*compare_func_t)(const void *, const void * TSRMLS_DC);
typedef void (*sort_func_t)(void *, size_t, size_t, compare_func_t TSRMLS_DC);
typedef void (*dtor_func_t)(zval *pDest);
typedef void (*copy_ctor_func_t)(zval *pElement);

typedef union _zend_value {
	long              lval;				/* long value */
	double            dval;				/* double value */
	zend_refcounted  *counted;
	zend_string      *str;
	zend_array       *arr;
	zend_object      *obj;
	zend_resource    *res;
	zend_reference   *ref;
	zend_ast_ref     *ast;
	zval             *zv;
	void             *ptr;
	zend_class_entry *ce;
	zend_function    *func;
	zend_str_offset  *str_offset;
} zend_value;

struct _zval_struct {
	zend_value        value;			/* value */
	zend_uchar        type;				/* active type */
	zend_uchar        var_flags;		/* various IS_VAR flags */
	union {
		zend_uint     next;             /* hash collision chain */
	} u;
};

struct _zend_refcounted {
	zend_uint         refcount;			/* reference counter 32-bit */
	union {
		struct {
			zend_uchar        type;
			zend_uchar        flags;    /* used for strings & objects */
			zend_ushort       gc_info;  /* keeps GC root number (or 0) and color */
		} v;
		zend_uint long_type;
	} u;
};

struct _zend_string {
	zend_refcounted   gc;
	zend_ulong        h;                /* hash value */
	int               len;
	char              val[1];
};

struct _zend_str_offset {
	zend_refcounted   gc;
	zval             *str;
	int               offset;
};

typedef struct _Bucket {
	zend_ulong        h;                /* hash value (or numeric index)   */
	zend_string      *key;              /* string key or NULL for numerics */
	zval              val;
} Bucket;

typedef struct _HashTable {	
	zend_uint         nTableSize;
	zend_uint         nTableMask;
	zend_uint         nNumUsed;
	zend_uint         nNumOfElements;
	long              nNextFreeElement;
	Bucket           *arData;
	zend_uint        *arHash;
	dtor_func_t       pDestructor;
	zend_uint         nInternalPointer; 
	zend_uchar        flags;
	zend_uchar        nApplyCount;
} HashTable;

struct _zend_array {
	zend_refcounted   gc;
	HashTable         ht;
};

struct _zend_object {
	zend_refcounted   gc;
	zend_uint         handle; //??? may be removed?
	zend_class_entry *ce;
	const zend_object_handlers *handlers;
	HashTable        *properties;
	HashTable        *guards; /* protects from __get/__set ... recursion */
	zval              properties_table[1];
};

struct _zend_resource {
	zend_refcounted   gc;
	long              handle; //??? may be removed?
	int               type;
	void             *ptr;
};

struct _zend_reference {
	zend_refcounted   gc;
	zval              val;
};

struct _zend_ast_ref {
	zend_refcounted   gc;
	zend_ast         *ast;
};

/* data types */
#define IS_UNDEF					0
#define IS_NULL						1
#define IS_INDIRECT             	2
#define IS_BOOL						3
#define IS_LONG						4
#define IS_DOUBLE					5
//#define IS_INTERNED_STRING			6
#define IS_STRING					7
#define IS_ARRAY					8
#define IS_OBJECT					9
#define IS_RESOURCE					10
#define IS_REFERENCE				11

#define IS_CONSTANT					12
#define IS_CONSTANT_ARRAY			13
#define IS_CONSTANT_AST				14
#define IS_CALLABLE					15

#define IS_STR_OFFSET				16
#define IS_PTR						17

/* Ugly hack to support constants as static array indices */
#define IS_CONSTANT_TYPE_MASK		0x00f
#define IS_CONSTANT_UNQUALIFIED		0x010
//???#define IS_CONSTANT_INDEX			0x080
#define IS_LEXICAL_VAR				0x020
#define IS_LEXICAL_REF				0x040
#define IS_CONSTANT_IN_NAMESPACE	0x100

#define IS_CONSTANT_TYPE(type)		\
	(((type) & IS_CONSTANT_TYPE_MASK) >= IS_CONSTANT && ((type) & IS_CONSTANT_TYPE_MASK) <= IS_CONSTANT_AST)

/* All data types < IS_STRING have their constructor/destructors skipped */
#define IS_REFCOUNTED(type)			((type) >= IS_STRING)

#define Z_TYPE(zval)				(zval).type
#define Z_TYPE_P(zval_p)			Z_TYPE(*(zval_p))

/* zval.var_flags */
#define IS_VAR_RET_REF				(1<<0) /* return by by reference */

/* string flags (zval.value->gc.u.flags) */
#define IS_STR_PERSISTENT			(1<<0) /* allocated using malloc   */
#define IS_STR_INTERNED				(1<<1) /* interned string          */
#define IS_STR_PERMANENT        	(1<<2) /* relives request boundary */

#define IS_STR_CONSTANT             (1<<3) /* constant index */
#define IS_STR_CONSTANT_UNQUALIFIED (1<<4) /* the same as IS_CONSTANT_UNQUALIFIED */
#define IS_STR_AST                  (1<<5) /* constant expression index */

/* object flags (zval.value->gc.u.vflags) */
#define IS_OBJ_APPLY_COUNT			0x07
#define IS_OBJ_DESTRUCTOR_CALLED	(1<<3)

#define Z_OBJ_APPLY_COUNT(zval) \
	(Z_OBJ(zval)->gc.u.v.flags & IS_OBJ_APPLY_COUNT)

#define Z_OBJ_INC_APPLY_COUNT(zval) do { \
		Z_OBJ(zval)->gc.u.v.flags = \
			(Z_OBJ(zval)->gc.u.v.flags & ~IS_OBJ_APPLY_COUNT) | \
			((Z_OBJ(zval)->gc.u.v.flags & IS_OBJ_APPLY_COUNT) + 1); \
	} while (0)
	
#define Z_OBJ_DEC_APPLY_COUNT(zval) do { \
		Z_OBJ(zval)->gc.u.v.flags = \
			(Z_OBJ(zval)->gc.u.v.flags & ~IS_OBJ_APPLY_COUNT) | \
			((Z_OBJ(zval)->gc.u.v.flags & IS_OBJ_APPLY_COUNT) - 1); \
	} while (0)

#define Z_OBJ_APPLY_COUNT_P(zv)     Z_OBJ_APPLY_COUNT(*(zv))
#define Z_OBJ_INC_APPLY_COUNT_P(zv) Z_OBJ_INC_APPLY_COUNT(*(zv))
#define Z_OBJ_DEC_APPLY_COUNT_P(zv) Z_OBJ_DEC_APPLY_COUNT(*(zv))

#define Z_REFCOUNTED(zval)			(IS_REFCOUNTED(Z_TYPE(zval)) && \
									 (Z_TYPE(zval) != IS_STRING || \
									  !IS_INTERNED(Z_STR(zval))))
#define Z_REFCOUNTED_P(zval_p)		Z_REFCOUNTED(*(zval_p))

#define Z_ISREF(zval)				(Z_TYPE(zval) == IS_REFERENCE)
#define Z_ISREF_P(zval_p)			Z_ISREF(*(zval_p))

#define Z_BVAL(zval)				(zend_bool)(zval).value.lval
#define Z_BVAL_P(zval_p)			Z_LVAL(*(zval_p))

#define Z_LVAL(zval)				(zval).value.lval
#define Z_LVAL_P(zval_p)			Z_LVAL(*(zval_p))

#define Z_DVAL(zval)				(zval).value.dval
#define Z_DVAL_P(zval_p)			Z_DVAL(*(zval_p))

#define Z_COUNTED(zval)				(zval).value.counted
#define Z_COUNTED_P(zval_p)			Z_COUNTED(*(zval_p))

#define Z_GC_TYPE(zval)				Z_COUNTED(zval)->type
#define Z_GC_TYPE_P(zval_p)			Z_GC_TYPE(*(zval_p))

#define Z_GC_INFO(zval)				Z_COUNTED(zval)->u.v.gc_info
#define Z_GC_INFO_P(zval_p)			Z_GC_INFO(*(zval_p))

#define Z_STR(zval)					(zval).value.str
#define Z_STR_P(zval_p)				Z_STR(*(zval_p))

#define Z_STRVAL(zval)				Z_STR(zval)->val
#define Z_STRVAL_P(zval_p)			Z_STRVAL(*(zval_p))

#define Z_STRLEN(zval)				Z_STR(zval)->len
#define Z_STRLEN_P(zval_p)			Z_STRLEN(*(zval_p))

#define Z_STRHASH(zval)				Z_STR(zval)->h
#define Z_STRHASH_P(zval_p)			Z_STRHASH(*(zval_p))

#define Z_ARR(zval)					(zval).value.arr
#define Z_ARR_P(zval_p)				Z_ARR(*(zval_p))

#define Z_ARRVAL(zval)				(&Z_ARR(zval)->ht)
#define Z_ARRVAL_P(zval_p)			Z_ARRVAL(*(zval_p))

#define Z_OBJ(zval)					(zval).value.obj
#define Z_OBJ_P(zval_p)				Z_OBJ(*(zval_p))

#define Z_OBJ_HT(zval)				Z_OBJ(zval)->handlers
#define Z_OBJ_HT_P(zval_p)			Z_OBJ_HT(*(zval_p))

#define Z_OBJ_HANDLER(zval, hf)		Z_OBJ_HT((zval))->hf
#define Z_OBJ_HANDLER_P(zv_p, hf)	Z_OBJ_HANDLER(*(zv_p), hf)

#define Z_OBJ_HANDLE(zval)          (Z_OBJ((zval)))->handle
#define Z_OBJ_HANDLE_P(zval_p)      Z_OBJ_HANDLE(*(zval_p))

#define Z_OBJCE(zval)				zend_get_class_entry(&(zval) TSRMLS_CC)
#define Z_OBJCE_P(zval_p)			Z_OBJCE(*(zval_p))

#define Z_OBJPROP(zval)				Z_OBJ_HT((zval))->get_properties(&(zval) TSRMLS_CC)
#define Z_OBJPROP_P(zval_p)			Z_OBJPROP(*(zval_p))

#define Z_OBJDEBUG(zval,tmp)		(Z_OBJ_HANDLER((zval),get_debug_info)?Z_OBJ_HANDLER((zval),get_debug_info)(&(zval),&tmp TSRMLS_CC):(tmp=0,Z_OBJ_HANDLER((zval),get_properties)?Z_OBJPROP(zval):NULL))
#define Z_OBJDEBUG_P(zval_p,tmp)	Z_OBJDEBUG(*(zval_p), tmp)

#define Z_RES(zval)					(zval).value.res
#define Z_RES_P(zval_p)				Z_RES(*zval_p)

#define Z_RES_HANDLE(zval)			Z_RES(zval)->handle
#define Z_RES_HANDLE_P(zval_p)		Z_RES_HANDLE(*zval_p)

#define Z_RES_TYPE(zval)			Z_RES(zval)->type
#define Z_RES_TYPE_P(zval_p)		Z_RES_TYPE(*zval_p)

#define Z_RES_VAL(zval)				Z_RES(zval)->ptr
#define Z_RES_VAL_P(zval_p)			Z_RES_VAL(*zval_p)

#define Z_REF(zval)					(zval).value.ref
#define Z_REF_P(zval_p)				Z_REF(*(zval_p))

#define Z_REFVAL(zval)				&Z_REF(zval)->val
#define Z_REFVAL_P(zval_p)			Z_REFVAL(*(zval_p))

#define Z_AST(zval)					(zval).value.ast
#define Z_AST_P(zval_p)				Z_AST(*(zval_p))

#define Z_ASTVAL(zval)				(zval).value.ast->ast
#define Z_ASTVAL_P(zval_p)			Z_ASTVAL(*(zval_p))

#define Z_INDIRECT(zval)			(zval).value.zv
#define Z_INDIRECT_P(zval_p)		Z_INDIRECT(*(zval_p))

#define Z_CE(zval)					(zval).value.ce
#define Z_CE_P(zval_p)				Z_CE(*(zval_p))

#define Z_FUNC(zval)				(zval).value.func
#define Z_FUNC_P(zval_p)			Z_FUNC(*(zval_p))

#define Z_PTR(zval)					(zval).value.ptr
#define Z_PTR_P(zval_p)				Z_PTR(*(zval_p))

#define Z_STR_OFFSET(zval)			(zval).value.str_offset
#define Z_STR_OFFSET_P(zval_p)		Z_STR_OFFSET(*(zval_p))

#define ZVAL_UNDEF(z) do {			\
		Z_TYPE_P(z) = IS_UNDEF;		\
	} while (0)

#define ZVAL_NULL(z) do {			\
		Z_TYPE_P(z) = IS_NULL;		\
	} while (0)

#define ZVAL_BOOL(z, b) do {		\
		zval *__z = (z);			\
		Z_LVAL_P(__z) = ((b) != 0);	\
		Z_TYPE_P(__z) = IS_BOOL;	\
	} while (0)

#define ZVAL_LONG(z, l) {			\
		zval *__z = (z);			\
		Z_LVAL_P(__z) = l;			\
		Z_TYPE_P(__z) = IS_LONG;	\
	}

#define ZVAL_DOUBLE(z, d) {			\
		zval *__z = (z);			\
		Z_DVAL_P(__z) = d;			\
		Z_TYPE_P(__z) = IS_DOUBLE;	\
	}

#define ZVAL_STR(z, s) do {						\
		zval *__z = (z);						\
		Z_STR_P(__z) = (s);						\
		Z_TYPE_P(__z) = IS_STRING;				\
	} while (0)

#define ZVAL_ARR(z, a) do {						\
		zval *__z = (z);						\
		Z_ARR_P(__z) = (a);						\
		Z_TYPE_P(__z) = IS_ARRAY;				\
	} while (0)

#define ZVAL_NEW_ARR(z) do {									\
		zval *__z = (z);										\
		zend_array *_arr = emalloc(sizeof(zend_array));			\
		_arr->gc.refcount = 1;									\
		_arr->gc.u.v.type = IS_ARRAY;							\
		_arr->gc.u.v.gc_info = 0;								\
		Z_ARR_P(__z) = _arr;									\
		Z_TYPE_P(__z) = IS_ARRAY;								\
	} while (0)

#define ZVAL_NEW_PERSISTENT_ARR(z) do {							\
		zval *__z = (z);										\
		zend_array *_arr = malloc(sizeof(zend_array));			\
		_arr->gc.refcount = 1;									\
		_arr->gc.u.v.type = IS_ARRAY;							\
		_arr->gc.u.v.gc_info = 0;								\
		Z_ARR_P(__z) = _arr;									\
		Z_TYPE_P(__z) = IS_ARRAY;								\
	} while (0)

#define ZVAL_OBJ(z, o) do {						\
		zval *__z = (z);						\
		Z_OBJ_P(__z) = (o);						\
		Z_TYPE_P(__z) = IS_OBJECT;				\
	} while (0)

#define ZVAL_RES(z, r) do {						\
		zval *__z = (z);						\
		Z_RES_P(__z) = (r);						\
		Z_TYPE_P(__z) = IS_RESOURCE;			\
	} while (0)

#define ZVAL_NEW_RES(z, h, p, t) do {							\
		zend_resource *_res = emalloc(sizeof(zend_resource));	\
		_res->gc.refcount = 1;									\
		_res->gc.u.v.type = IS_RESOURCE;						\
		_res->gc.u.v.gc_info = 0;								\
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		zval *__z = (z);										\
		Z_RES_P(__z) = _res;									\
		Z_TYPE_P(__z) = IS_RESOURCE;							\
	} while (0)

#define ZVAL_NEW_PERSISTENT_RES(z, h, p, t) do {				\
		zend_resource *_res = malloc(sizeof(zend_resource));	\
		_res->gc.refcount = 1;									\
		_res->gc.u.v.type = IS_RESOURCE;						\
		_res->gc.u.v.gc_info = 0;								\
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		zval *__z = (z);										\
		Z_RES_P(__z) = _res;									\
		Z_TYPE_P(__z) = IS_RESOURCE;							\
	} while (0)

#define ZVAL_REF(z, r) do {										\
		zval *__z = (z);										\
		Z_REF_P(__z) = (r);										\
		Z_TYPE_P(__z) = IS_REFERENCE;							\
	} while (0)

#define ZVAL_NEW_REF(z, r) do {									\
		zend_reference *_ref = emalloc(sizeof(zend_reference));	\
		_ref->gc.refcount = 1;									\
		_ref->gc.u.v.type = IS_REFERENCE;						\
		_ref->gc.u.v.gc_info = 0;								\
		_ref->val = *(r);										\
		Z_REF_P(z) = _ref;										\
		Z_TYPE_P(z) = IS_REFERENCE;								\
	} while (0)

#define ZVAL_NEW_AST(z, a) do {									\
		zval *__z = (z);										\
		zend_ast_ref *_ast = emalloc(sizeof(zend_ast_ref));		\
		_ast->gc.refcount = 1;									\
		_ast->gc.u.v.type = IS_CONSTANT_AST;					\
		_ast->gc.u.v.gc_info = 0;								\
		_ast->ast = (a);										\
		Z_AST_P(__z) = _ast;									\
		Z_TYPE_P(__z) = IS_CONSTANT_AST;						\
	} while (0)

#define ZVAL_INDIRECT(z, v) do {								\
		Z_INDIRECT_P(z) = (v);									\
		Z_TYPE_P(z) = IS_INDIRECT;								\
	} while (0)

#define ZVAL_PTR(z, p) do {										\
		Z_PTR_P(z) = (p);										\
		Z_TYPE_P(z) = IS_PTR;									\
	} while (0)

#define ZVAL_FUNC(z, f) do {									\
		Z_FUNC_P(z) = (f);										\
		Z_TYPE_P(z) = IS_PTR;									\
	} while (0)

#define ZVAL_CE(z, c) do {										\
		Z_CE_P(z) = (c);										\
		Z_TYPE_P(z) = IS_PTR;									\
	} while (0)

#define ZVAL_STR_OFFSET(z, s, o) do {							\
		zend_str_offset *x = emalloc(sizeof(zend_str_offset));	\
		x->gc.refcount = 1;										\
		x->gc.u.v.type = IS_STR_OFFSET;							\
		x->gc.u.v.gc_info = 0;									\
		x->str = (s);											\
		x->offset = (o);										\
		Z_STR_OFFSET_P(z) = x;									\
		Z_TYPE_P(z) = IS_STR_OFFSET;							\
	} while (0)

#endif /* ZEND_TYPES_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
