/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2015 Zend Technologies Ltd. (http://www.zend.com) |
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

#include <ctype.h>

#include "zend.h"
#include "zend_operators.h"
#include "zend_variables.h"
#include "zend_globals.h"
#include "zend_list.h"
#include "zend_API.h"
#include "zend_strtod.h"
#include "zend_exceptions.h"
#include "zend_closures.h"

#if ZEND_USE_TOLOWER_L
#include <locale.h>
static _locale_t current_locale = NULL;
/* this is true global! may lead to strange effects on ZTS, but so may setlocale() */
#define zend_tolower(c) _tolower_l(c, current_locale)
#else
#define zend_tolower(c) tolower(c)
#endif

#define TYPE_PAIR(t1,t2) (((t1) << 4) | (t2))

static const unsigned char tolower_map[256] = {
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

#define zend_tolower_ascii(c) (tolower_map[(unsigned char)(c)])

/**
 * Functions using locale lowercase:
 	 	zend_binary_strncasecmp_l
 	 	zend_binary_strcasecmp_l
		zend_binary_zval_strcasecmp
		zend_binary_zval_strncasecmp
		string_compare_function_ex
		string_case_compare_function
 * Functions using ascii lowercase:
  		zend_str_tolower_copy
		zend_str_tolower_dup
		zend_str_tolower
		zend_binary_strcasecmp
		zend_binary_strncasecmp
 */

ZEND_API int ZEND_FASTCALL zend_atoi(const char *str, int str_len) /* {{{ */
{
	int retval;

	if (!str_len) {
		str_len = (int)strlen(str);
	}
	retval = ZEND_STRTOL(str, NULL, 0);
	if (str_len>0) {
		switch (str[str_len-1]) {
			case 'g':
			case 'G':
				retval *= 1024;
				/* break intentionally missing */
			case 'm':
			case 'M':
				retval *= 1024;
				/* break intentionally missing */
			case 'k':
			case 'K':
				retval *= 1024;
				break;
		}
	}
	return retval;
}
/* }}} */

ZEND_API zend_long ZEND_FASTCALL zend_atol(const char *str, int str_len) /* {{{ */
{
	zend_long retval;

	if (!str_len) {
		str_len = (int)strlen(str);
	}
	retval = ZEND_STRTOL(str, NULL, 0);
	if (str_len>0) {
		switch (str[str_len-1]) {
			case 'g':
			case 'G':
				retval *= 1024;
				/* break intentionally missing */
			case 'm':
			case 'M':
				retval *= 1024;
				/* break intentionally missing */
			case 'k':
			case 'K':
				retval *= 1024;
				break;
		}
	}
	return retval;
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_scalar_to_number(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		case IS_REFERENCE:
			if (Z_REFCOUNT_P(op) == 1) {
				ZVAL_UNREF(op);
			} else {
				Z_DELREF_P(op);
				ZVAL_COPY_VALUE(op, Z_REFVAL_P(op));
			}
			goto try_again;
		case IS_STRING:
			{
				zend_string *str;

				str = Z_STR_P(op);
				if ((Z_TYPE_INFO_P(op)=is_numeric_string(str->val, str->len, &Z_LVAL_P(op), &Z_DVAL_P(op), 1)) == 0) {
					ZVAL_LONG(op, 0);
				}
				zend_string_release(str);
				break;
			}
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(op, 0);
			break;
		case IS_TRUE:
			ZVAL_LONG(op, 1);
			break;
		case IS_RESOURCE:
			{
				zend_long l = Z_RES_HANDLE_P(op);
				zval_ptr_dtor(op);
				ZVAL_LONG(op, l);
			}
			break;
		case IS_OBJECT:
			convert_to_long_base(op, 10);
			break;
	}
}
/* }}} */

/* {{{ zendi_convert_scalar_to_number */
#define zendi_convert_scalar_to_number(op, holder, result)			\
	if (op==result) {												\
		if (Z_TYPE_P(op) != IS_LONG) {								\
			convert_scalar_to_number(op);					\
		}															\
	} else {														\
		switch (Z_TYPE_P(op)) {										\
			case IS_STRING:											\
				{													\
					if ((Z_TYPE_INFO(holder)=is_numeric_string(Z_STRVAL_P(op), Z_STRLEN_P(op), &Z_LVAL(holder), &Z_DVAL(holder), 1)) == 0) {	\
						ZVAL_LONG(&(holder), 0);							\
					}														\
					(op) = &(holder);										\
					break;													\
				}															\
			case IS_NULL:													\
			case IS_FALSE:													\
				ZVAL_LONG(&(holder), 0);									\
				(op) = &(holder);											\
				break;														\
			case IS_TRUE:													\
				ZVAL_LONG(&(holder), 1);									\
				(op) = &(holder);											\
				break;														\
			case IS_RESOURCE:												\
				ZVAL_LONG(&(holder), Z_RES_HANDLE_P(op));					\
				(op) = &(holder);											\
				break;														\
			case IS_OBJECT:													\
				ZVAL_DUP(&(holder), op);										\
				convert_to_long_base(&(holder), 10);						\
				if (Z_TYPE(holder) == IS_LONG) {							\
					(op) = &(holder);										\
				}															\
				break;														\
		}																	\
	}

/* }}} */

/* {{{ convert_object_to_type */
#define convert_object_to_type(op, dst, ctype, conv_func)									\
	ZVAL_UNDEF(dst);																		\
	if (Z_OBJ_HT_P(op)->cast_object) {														\
		if (Z_OBJ_HT_P(op)->cast_object(op, dst, ctype) == FAILURE) {				\
			zend_error(E_RECOVERABLE_ERROR,													\
				"Object of class %s could not be converted to %s", Z_OBJCE_P(op)->name->val,\
			zend_get_type_by_const(ctype));													\
		} 																					\
	} else if (Z_OBJ_HT_P(op)->get) {														\
		zval *newop = Z_OBJ_HT_P(op)->get(op, dst);								\
		if (Z_TYPE_P(newop) != IS_OBJECT) {													\
			/* for safety - avoid loop */													\
			ZVAL_COPY_VALUE(dst, newop);													\
			conv_func(dst);																	\
		}																					\
	}

/* }}} */

#define convert_op1_op2_long(op1, op1_lval, op2, op2_lval, op, op_func) \
	do {																\
		if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {						\
			if (Z_ISREF_P(op1)) {										\
				op1 = Z_REFVAL_P(op1);									\
				if (Z_TYPE_P(op1) == IS_LONG) {							\
					op1_lval = Z_LVAL_P(op1);							\
					break;												\
				}														\
			}															\
			ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(op, op_func);			\
			op1_lval = _zval_get_long_func(op1);						\
		} else {														\
			op1_lval = Z_LVAL_P(op1);									\
		}																\
	} while (0);														\
	do {																\
		if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {						\
			if (Z_ISREF_P(op2)) {										\
				op2 = Z_REFVAL_P(op2);									\
				if (Z_TYPE_P(op2) == IS_LONG) {							\
					op2_lval = Z_LVAL_P(op2);							\
					break;												\
				}														\
			}															\
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(op);					\
			op2_lval = _zval_get_long_func(op2);						\
		} else {														\
			op2_lval = Z_LVAL_P(op2);									\
		}																\
	} while (0);

ZEND_API void ZEND_FASTCALL convert_to_long(zval *op) /* {{{ */
{
	if (Z_TYPE_P(op) != IS_LONG) {
		convert_to_long_base(op, 10);
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_long_base(zval *op, int base) /* {{{ */
{
	zend_long tmp;

	switch (Z_TYPE_P(op)) {
		case IS_NULL:
		case IS_FALSE:
			ZVAL_LONG(op, 0);
			break;
		case IS_TRUE:
			ZVAL_LONG(op, 1);
			break;
		case IS_RESOURCE:
			tmp = Z_RES_HANDLE_P(op);
			zval_ptr_dtor(op);
			ZVAL_LONG(op, tmp);
			break;
		case IS_LONG:
			break;
		case IS_DOUBLE:
			ZVAL_LONG(op, zend_dval_to_lval(Z_DVAL_P(op)));
			break;
		case IS_STRING:
			{
				zend_string *str = Z_STR_P(op);

				ZVAL_LONG(op, ZEND_STRTOL(str->val, NULL, base));
				zend_string_release(str);
			}
			break;
		case IS_ARRAY:
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			zval_dtor(op);
			ZVAL_LONG(op, tmp);
			break;
		case IS_OBJECT:
			{
				zval dst;

				convert_object_to_type(op, &dst, IS_LONG, convert_to_long);
				zval_dtor(op);

				if (Z_TYPE(dst) == IS_LONG) {
					ZVAL_COPY_VALUE(op, &dst);
				} else {
					zend_error(E_NOTICE, "Object of class %s could not be converted to int", Z_OBJCE_P(op)->name->val);

					ZVAL_LONG(op, 1);
				}
				return;
			}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_double(zval *op) /* {{{ */
{
	double tmp;

	switch (Z_TYPE_P(op)) {
		case IS_NULL:
		case IS_FALSE:
			ZVAL_DOUBLE(op, 0.0);
			break;
		case IS_TRUE:
			ZVAL_DOUBLE(op, 1.0);
			break;
		case IS_RESOURCE: {
				double d = (double) Z_RES_HANDLE_P(op);
				zval_ptr_dtor(op);
				ZVAL_DOUBLE(op, d);
			}
			break;
		case IS_LONG:
			ZVAL_DOUBLE(op, (double) Z_LVAL_P(op));
			break;
		case IS_DOUBLE:
			break;
		case IS_STRING:
			{
				zend_string *str = Z_STR_P(op);

				ZVAL_DOUBLE(op, zend_strtod(str->val, NULL));
				zend_string_release(str);
			}
			break;
		case IS_ARRAY:
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			zval_dtor(op);
			ZVAL_DOUBLE(op, tmp);
			break;
		case IS_OBJECT:
			{
				zval dst;

				convert_object_to_type(op, &dst, IS_DOUBLE, convert_to_double);
				zval_dtor(op);

				if (Z_TYPE(dst) == IS_DOUBLE) {
					ZVAL_COPY_VALUE(op, &dst);
				} else {
					zend_error(E_NOTICE, "Object of class %s could not be converted to float", Z_OBJCE_P(op)->name->val);

					ZVAL_DOUBLE(op, 1.0);
				}
				break;
			}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_null(zval *op) /* {{{ */
{
	if (Z_TYPE_P(op) == IS_OBJECT) {
		if (Z_OBJ_HT_P(op)->cast_object) {
			zval org;

			ZVAL_COPY_VALUE(&org, op);
			if (Z_OBJ_HT_P(op)->cast_object(&org, op, IS_NULL) == SUCCESS) {
				zval_dtor(&org);
				return;
			}
			ZVAL_COPY_VALUE(op, &org);
		}
	}

	zval_dtor(op);
	ZVAL_NULL(op);
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_boolean(zval *op) /* {{{ */
{
	int tmp;

	switch (Z_TYPE_P(op)) {
		case IS_FALSE:
		case IS_TRUE:
			break;
		case IS_NULL:
			ZVAL_FALSE(op);
			break;
		case IS_RESOURCE: {
				zend_long l = (Z_RES_HANDLE_P(op) ? 1 : 0);

				zval_ptr_dtor(op);
				ZVAL_BOOL(op, l);
			}
			break;
		case IS_LONG:
			ZVAL_BOOL(op, Z_LVAL_P(op) ? 1 : 0);
			break;
		case IS_DOUBLE:
			ZVAL_BOOL(op, Z_DVAL_P(op) ? 1 : 0);
			break;
		case IS_STRING:
			{
				zend_string *str = Z_STR_P(op);

				if (str->len == 0
					|| (str->len == 1 && str->val[0] == '0')) {
					ZVAL_FALSE(op);
				} else {
					ZVAL_TRUE(op);
				}
				zend_string_release(str);
			}
			break;
		case IS_ARRAY:
			tmp = (zend_hash_num_elements(Z_ARRVAL_P(op))?1:0);
			zval_dtor(op);
			ZVAL_BOOL(op, tmp);
			break;
		case IS_OBJECT:
			{
				zval dst;

				convert_object_to_type(op, &dst, _IS_BOOL, convert_to_boolean);
				zval_dtor(op);

				if (Z_TYPE(dst) == IS_FALSE || Z_TYPE(dst) == IS_TRUE) {
					ZVAL_COPY_VALUE(op, &dst);
				} else {
					ZVAL_TRUE(op);
				}
				break;
			}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL _convert_to_cstring(zval *op ZEND_FILE_LINE_DC) /* {{{ */
{
	_convert_to_string(op ZEND_FILE_LINE_CC);
}
/* }}} */

ZEND_API void ZEND_FASTCALL _convert_to_string(zval *op ZEND_FILE_LINE_DC) /* {{{ */
{
	switch (Z_TYPE_P(op)) {
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE: {
			ZVAL_EMPTY_STRING(op);
			break;
		}
		case IS_TRUE:
			ZVAL_NEW_STR(op, zend_string_init("1", 1, 0));
			break;
		case IS_STRING:
			break;
		case IS_RESOURCE: {
			char buf[sizeof("Resource id #") + MAX_LENGTH_OF_LONG];
			int len = snprintf(buf, sizeof(buf), "Resource id #" ZEND_LONG_FMT, (zend_long)Z_RES_HANDLE_P(op));
			zval_ptr_dtor(op);
			ZVAL_NEW_STR(op, zend_string_init(buf, len, 0));
			break;
		}
		case IS_LONG: {
			ZVAL_NEW_STR(op, zend_long_to_str(Z_LVAL_P(op)));
			break;
		}
		case IS_DOUBLE: {
			zend_string *str;
			double dval = Z_DVAL_P(op);

			str = zend_strpprintf(0, "%.*G", (int) EG(precision), dval);
			/* %G already handles removing trailing zeros from the fractional part, yay */
			ZVAL_NEW_STR(op, str);
			break;
		}
		case IS_ARRAY:
			zend_error(E_NOTICE, "Array to string conversion");
			zval_dtor(op);
			ZVAL_NEW_STR(op, zend_string_init("Array", sizeof("Array")-1, 0));
			break;
		case IS_OBJECT: {
			zval dst;

			convert_object_to_type(op, &dst, IS_STRING, convert_to_string);

			if (Z_TYPE(dst) == IS_STRING) {
				zval_dtor(op);
				ZVAL_COPY_VALUE(op, &dst);
			} else {
				zend_error(E_NOTICE, "Object of class %s to string conversion", Z_OBJCE_P(op)->name->val);
				zval_dtor(op);
				ZVAL_NEW_STR(op, zend_string_init("Object", sizeof("Object")-1, 0));
			}
			break;
		}
		EMPTY_SWITCH_DEFAULT_CASE()
	}
}
/* }}} */

static void convert_scalar_to_array(zval *op) /* {{{ */
{
	zval entry;

	ZVAL_COPY_VALUE(&entry, op);

	ZVAL_NEW_ARR(op);
	zend_hash_init(Z_ARRVAL_P(op), 8, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_index_add_new(Z_ARRVAL_P(op), 0, &entry);
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_array(zval *op) /* {{{ */
{

	switch (Z_TYPE_P(op)) {
		case IS_ARRAY:
			break;
/* OBJECTS_OPTIMIZE */
		case IS_OBJECT:
			if (Z_OBJCE_P(op) == zend_ce_closure) {
				convert_scalar_to_array(op);
			} else {
				if (Z_OBJ_HT_P(op)->get_properties) {
					HashTable *obj_ht = Z_OBJ_HT_P(op)->get_properties(op);
					if (obj_ht) {
						zval arr;
						ZVAL_ARR(&arr, zend_array_dup(obj_ht));
						zval_dtor(op);
						ZVAL_COPY_VALUE(op, &arr);
						return;
					}
				} else {
					zval dst;
					convert_object_to_type(op, &dst, IS_ARRAY, convert_to_array);

					if (Z_TYPE(dst) == IS_ARRAY) {
						zval_dtor(op);
						ZVAL_COPY_VALUE(op, &dst);
						return;
					}
				}

				zval_dtor(op);
				array_init(op);
			}
			break;
		case IS_NULL:
			ZVAL_NEW_ARR(op);
			zend_hash_init(Z_ARRVAL_P(op), 8, NULL, ZVAL_PTR_DTOR, 0);
			break;
		default:
			convert_scalar_to_array(op);
			break;
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL convert_to_object(zval *op) /* {{{ */
{

	switch (Z_TYPE_P(op)) {
		case IS_ARRAY:
			{
				HashTable *properties = emalloc(sizeof(HashTable));
				zend_array *arr = Z_ARR_P(op);

				memcpy(properties, Z_ARRVAL_P(op), sizeof(HashTable));
				object_and_properties_init(op, zend_standard_class_def, properties);
				if (--GC_REFCOUNT(arr) == 0) {
					efree_size(arr, sizeof(zend_array));
				}
				break;
			}
		case IS_OBJECT:
			break;
		case IS_NULL:
			object_init(op);
			break;
		default: {
			zval tmp;
			ZVAL_COPY_VALUE(&tmp, op);
			object_init(op);
			zend_hash_str_add_new(Z_OBJPROP_P(op), "scalar", sizeof("scalar")-1, &tmp);
			break;
		}
	}
}
/* }}} */

ZEND_API void multi_convert_to_long_ex(int argc, ...) /* {{{ */
{
	zval *arg;
	va_list ap;

	va_start(ap, argc);

	while (argc--) {
		arg = va_arg(ap, zval *);
		convert_to_long_ex(arg);
	}

	va_end(ap);
}
/* }}} */

ZEND_API void multi_convert_to_double_ex(int argc, ...) /* {{{ */
{
	zval *arg;
	va_list ap;

	va_start(ap, argc);

	while (argc--) {
		arg = va_arg(ap, zval *);
		convert_to_double_ex(arg);
	}

	va_end(ap);
}
/* }}} */

ZEND_API void multi_convert_to_string_ex(int argc, ...) /* {{{ */
{
	zval *arg;
	va_list ap;

	va_start(ap, argc);

	while (argc--) {
		arg = va_arg(ap, zval *);
		convert_to_string_ex(arg);
	}

	va_end(ap);
}
/* }}} */

ZEND_API zend_long ZEND_FASTCALL _zval_get_long_func(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		case IS_NULL:
		case IS_FALSE:
			return 0;
		case IS_TRUE:
			return 1;
		case IS_RESOURCE:
			return Z_RES_HANDLE_P(op);
		case IS_LONG:
			return Z_LVAL_P(op);
		case IS_DOUBLE:
			return zend_dval_to_lval(Z_DVAL_P(op));
		case IS_STRING:
			return ZEND_STRTOL(Z_STRVAL_P(op), NULL, 10);
		case IS_ARRAY:
			return zend_hash_num_elements(Z_ARRVAL_P(op)) ? 1 : 0;
		case IS_OBJECT:
			{
				zval dst;
				convert_object_to_type(op, &dst, IS_LONG, convert_to_long);
				if (Z_TYPE(dst) == IS_LONG) {
					return Z_LVAL(dst);
				} else {
					zend_error(E_NOTICE, "Object of class %s could not be converted to int", Z_OBJCE_P(op)->name->val);
					return 1;
				}
			}
		case IS_REFERENCE:
			op = Z_REFVAL_P(op);
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return 0;
}
/* }}} */

ZEND_API double ZEND_FASTCALL _zval_get_double_func(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		case IS_NULL:
		case IS_FALSE:
			return 0.0;
		case IS_TRUE:
			return 1.0;
		case IS_RESOURCE:
			return (double) Z_RES_HANDLE_P(op);
		case IS_LONG:
			return (double) Z_LVAL_P(op);
		case IS_DOUBLE:
			return Z_DVAL_P(op);
		case IS_STRING:
			return zend_strtod(Z_STRVAL_P(op), NULL);
		case IS_ARRAY:
			return zend_hash_num_elements(Z_ARRVAL_P(op)) ? 1.0 : 0.0;
		case IS_OBJECT:
			{
				zval dst;
				convert_object_to_type(op, &dst, IS_DOUBLE, convert_to_double);

				if (Z_TYPE(dst) == IS_DOUBLE) {
					return Z_DVAL(dst);
				} else {
					zend_error(E_NOTICE, "Object of class %s could not be converted to double", Z_OBJCE_P(op)->name->val);

					return 1.0;
				}
			}
		case IS_REFERENCE:
			op = Z_REFVAL_P(op);
			goto try_again;
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return 0.0;
}
/* }}} */

ZEND_API zend_string* ZEND_FASTCALL _zval_get_string_func(zval *op) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op)) {
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE:
			return STR_EMPTY_ALLOC();
		case IS_TRUE:
			return zend_string_init("1", 1, 0);
		case IS_RESOURCE: {
			char buf[sizeof("Resource id #") + MAX_LENGTH_OF_LONG];
			int len;

			len = snprintf(buf, sizeof(buf), "Resource id #" ZEND_LONG_FMT, (zend_long)Z_RES_HANDLE_P(op));
			return zend_string_init(buf, len, 0);
		}
		case IS_LONG: {
			return zend_long_to_str(Z_LVAL_P(op));
		}
		case IS_DOUBLE: {
			return zend_strpprintf(0, "%.*G", (int) EG(precision), Z_DVAL_P(op));
		}
		case IS_ARRAY:
			zend_error(E_NOTICE, "Array to string conversion");
			return zend_string_init("Array", sizeof("Array")-1, 0);
		case IS_OBJECT: {
			zval tmp;
			if (Z_OBJ_HT_P(op)->cast_object) {
				if (Z_OBJ_HT_P(op)->cast_object(op, &tmp, IS_STRING) == SUCCESS) {
					return Z_STR(tmp);
				}
			} else if (Z_OBJ_HT_P(op)->get) {
				zval *z = Z_OBJ_HT_P(op)->get(op, &tmp);
				if (Z_TYPE_P(z) != IS_OBJECT) {
					zend_string *str = zval_get_string(z);
					zval_ptr_dtor(z);
					return str;
				}
				zval_ptr_dtor(z);
			}
			zend_error(EG(exception) ? E_ERROR : E_RECOVERABLE_ERROR, "Object of class %s could not be converted to string", Z_OBJCE_P(op)->name->val);
			return STR_EMPTY_ALLOC();
		}
		case IS_REFERENCE:
			op = Z_REFVAL_P(op);
			goto try_again;
		case IS_STRING:
			return zend_string_copy(Z_STR_P(op));
		EMPTY_SWITCH_DEFAULT_CASE()
	}
	return NULL;
}
/* }}} */

ZEND_API int ZEND_FASTCALL add_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int converted = 0;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG): {
				zend_long lval = Z_LVAL_P(op1) + Z_LVAL_P(op2);

				/* check for overflow by comparing sign bits */
				if ((Z_LVAL_P(op1) & LONG_SIGN_MASK) == (Z_LVAL_P(op2) & LONG_SIGN_MASK)
					&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != (lval & LONG_SIGN_MASK)) {

					ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) + (double) Z_LVAL_P(op2));
				} else {
					ZVAL_LONG(result, lval);
				}
				return SUCCESS;
			}

			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) + Z_DVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) + ((double)Z_LVAL_P(op2)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) + Z_DVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_ARRAY, IS_ARRAY):
				if ((result == op1) && (result == op2)) {
					/* $a += $a */
					return SUCCESS;
				}
				if (result != op1) {
					ZVAL_DUP(result, op1);
				}
				zend_hash_merge(Z_ARRVAL_P(result), Z_ARRVAL_P(op2), zval_add_ref, 0);
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
				} else if (!converted) {
					ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_ADD, add_function);

					zendi_convert_scalar_to_number(op1, op1_copy, result);
					zendi_convert_scalar_to_number(op2, op2_copy, result);
					converted = 1;
				} else {
					zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
					return FAILURE; /* unknown datatype */
				}
		}
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL sub_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int converted = 0;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG): {
				zend_long lval = Z_LVAL_P(op1) - Z_LVAL_P(op2);

				/* check for overflow by comparing sign bits */
				if ((Z_LVAL_P(op1) & LONG_SIGN_MASK) != (Z_LVAL_P(op2) & LONG_SIGN_MASK)
					&& (Z_LVAL_P(op1) & LONG_SIGN_MASK) != (lval & LONG_SIGN_MASK)) {

					ZVAL_DOUBLE(result, (double) Z_LVAL_P(op1) - (double) Z_LVAL_P(op2));
				} else {
					ZVAL_LONG(result, lval);
				}
				return SUCCESS;

			}
			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) - Z_DVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) - ((double)Z_LVAL_P(op2)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) - Z_DVAL_P(op2));
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
				} else if (!converted) {
					ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_SUB, sub_function);

					zendi_convert_scalar_to_number(op1, op1_copy, result);
					zendi_convert_scalar_to_number(op2, op2_copy, result);
					converted = 1;
				} else {
					zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
					return FAILURE; /* unknown datatype */
				}
		}
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL mul_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int converted = 0;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG): {
				zend_long overflow;

				ZEND_SIGNED_MULTIPLY_LONG(Z_LVAL_P(op1),Z_LVAL_P(op2), Z_LVAL_P(result),Z_DVAL_P(result),overflow);
				Z_TYPE_INFO_P(result) = overflow ? IS_DOUBLE : IS_LONG;
				return SUCCESS;

			}
			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				ZVAL_DOUBLE(result, ((double)Z_LVAL_P(op1)) * Z_DVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) * ((double)Z_LVAL_P(op2)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) * Z_DVAL_P(op2));
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
				} else if (!converted) {
					ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_MUL, mul_function);

					zendi_convert_scalar_to_number(op1, op1_copy, result);
					zendi_convert_scalar_to_number(op2, op2_copy, result);
					converted = 1;
				} else {
					zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
					return FAILURE; /* unknown datatype */
				}
		}
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL pow_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int converted = 0;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG):
				if (Z_LVAL_P(op2) >= 0) {
					zend_long l1 = 1, l2 = Z_LVAL_P(op1), i = Z_LVAL_P(op2);

					if (i == 0) {
						ZVAL_LONG(result, 1L);
						return SUCCESS;
					} else if (l2 == 0) {
						ZVAL_LONG(result, 0);
						return SUCCESS;
					}

					while (i >= 1) {
						zend_long overflow;
						double dval = 0.0;

						if (i % 2) {
							--i;
							ZEND_SIGNED_MULTIPLY_LONG(l1, l2, l1, dval, overflow);
							if (overflow) {
								ZVAL_DOUBLE(result, dval * pow(l2, i));
								return SUCCESS;
							}
						} else {
							i /= 2;
							ZEND_SIGNED_MULTIPLY_LONG(l2, l2, l2, dval, overflow);
							if (overflow) {
								ZVAL_DOUBLE(result, (double)l1 * pow(dval, i));
								return SUCCESS;
							}
						}
					}
					/* i == 0 */
					ZVAL_LONG(result, l1);
				} else {
					ZVAL_DOUBLE(result, pow((double)Z_LVAL_P(op1), (double)Z_LVAL_P(op2)));
				}
				return SUCCESS;

			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				ZVAL_DOUBLE(result, pow((double)Z_LVAL_P(op1), Z_DVAL_P(op2)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				ZVAL_DOUBLE(result, pow(Z_DVAL_P(op1), (double)Z_LVAL_P(op2)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				ZVAL_DOUBLE(result, pow(Z_DVAL_P(op1), Z_DVAL_P(op2)));
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
				} else if (!converted) {
					ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_POW, pow_function);

					if (Z_TYPE_P(op1) == IS_ARRAY) {
						ZVAL_LONG(result, 0);
						return SUCCESS;
					} else {
						zendi_convert_scalar_to_number(op1, op1_copy, result);
					}
					if (Z_TYPE_P(op2) == IS_ARRAY) {
						ZVAL_LONG(result, 1L);
						return SUCCESS;
					} else {
						zendi_convert_scalar_to_number(op2, op2_copy, result);
					}
					converted = 1;
				} else {
					zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
					return FAILURE;
				}
		}
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL div_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int converted = 0;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG):
				if (Z_LVAL_P(op2) == 0) {
					zend_error(E_WARNING, "Division by zero");
					ZVAL_DOUBLE(result, ((double) Z_LVAL_P(op1) / (double) Z_LVAL_P(op2)));
					return SUCCESS;
				} else if (Z_LVAL_P(op2) == -1 && Z_LVAL_P(op1) == ZEND_LONG_MIN) {
					/* Prevent overflow error/crash */
					ZVAL_DOUBLE(result, (double) ZEND_LONG_MIN / -1);
					return SUCCESS;
				}
				if (Z_LVAL_P(op1) % Z_LVAL_P(op2) == 0) { /* integer */
					ZVAL_LONG(result, Z_LVAL_P(op1) / Z_LVAL_P(op2));
				} else {
					ZVAL_DOUBLE(result, ((double) Z_LVAL_P(op1)) / Z_LVAL_P(op2));
				}
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				if (Z_LVAL_P(op2) == 0) {
					zend_error(E_WARNING, "Division by zero");
				}
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) / (double)Z_LVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				if (Z_DVAL_P(op2) == 0) {
					zend_error(E_WARNING, "Division by zero");
				}
				ZVAL_DOUBLE(result, (double)Z_LVAL_P(op1) / Z_DVAL_P(op2));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				if (Z_DVAL_P(op2) == 0) {
					zend_error(E_WARNING, "Division by zero");
				}
				ZVAL_DOUBLE(result, Z_DVAL_P(op1) / Z_DVAL_P(op2));
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
				} else if (!converted) {
					ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_DIV, div_function);

					zendi_convert_scalar_to_number(op1, op1_copy, result);
					zendi_convert_scalar_to_number(op2, op2_copy, result);
					converted = 1;
				} else {
					zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
					return FAILURE; /* unknown datatype */
				}
		}
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL mod_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, ZEND_MOD, mod_function);

	if (op1 == result) {
		zval_dtor(result);
	}

	if (op2_lval == 0) {
		/* modulus by zero */
		if (EG(current_execute_data) && !CG(in_compilation)) {
			zend_throw_exception_ex(NULL, 0, "Division by zero");
		} else {
			zend_error_noreturn(E_ERROR, "Division by zero");
		}
		ZVAL_UNDEF(result);
		return FAILURE;
	}

	if (op2_lval == -1) {
		/* Prevent overflow error/crash if op1==LONG_MIN */
		ZVAL_LONG(result, 0);
		return SUCCESS;
	}

	ZVAL_LONG(result, op1_lval % op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL boolean_xor_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	int op1_val, op2_val;

	do {
		if (Z_TYPE_P(op1) == IS_FALSE) {
			op1_val = 0;
		} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
			op1_val = 1;
		} else {
			if (Z_ISREF_P(op1)) {
				op1 = Z_REFVAL_P(op1);
				if (Z_TYPE_P(op1) == IS_FALSE) {
					op1_val = 0;
					break;
				} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
					op1_val = 1;
					break;
				}
			}
			ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BOOL_XOR, boolean_xor_function);
			op1_val = zval_is_true(op1);
		}
	} while (0);
	do {
		if (Z_TYPE_P(op2) == IS_FALSE) {
			op2_val = 0;
		} else if (EXPECTED(Z_TYPE_P(op2) == IS_TRUE)) {
			op2_val = 1;
		} else {
			if (Z_ISREF_P(op2)) {
				op2 = Z_REFVAL_P(op2);
				if (Z_TYPE_P(op2) == IS_FALSE) {
					op2_val = 0;
					break;
				} else if (EXPECTED(Z_TYPE_P(op2) == IS_TRUE)) {
					op2_val = 1;
					break;
				}
			}
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BOOL_XOR);
			op2_val = zval_is_true(op2);
		}
	} while (0);

	ZVAL_BOOL(result, op1_val ^ op2_val);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL boolean_not_function(zval *result, zval *op1) /* {{{ */
{
	if (Z_TYPE_P(op1) < IS_TRUE) {
		ZVAL_TRUE(result);
	} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
		ZVAL_FALSE(result);
	} else {
		if (Z_ISREF_P(op1)) {
			op1 = Z_REFVAL_P(op1);
			if (Z_TYPE_P(op1) < IS_TRUE) {
				ZVAL_TRUE(result);
				return SUCCESS;
			} else if (EXPECTED(Z_TYPE_P(op1) == IS_TRUE)) {
				ZVAL_FALSE(result);
				return SUCCESS;
			}
		}
		ZEND_TRY_UNARY_OBJECT_OPERATION(ZEND_BOOL_NOT);

		ZVAL_BOOL(result, !zval_is_true(op1));
	}
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL bitwise_not_function(zval *result, zval *op1) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op1)) {
		case IS_LONG:
			ZVAL_LONG(result, ~Z_LVAL_P(op1));
			return SUCCESS;
		case IS_DOUBLE:
			ZVAL_LONG(result, ~zend_dval_to_lval(Z_DVAL_P(op1)));
			return SUCCESS;
		case IS_STRING: {
			size_t i;

			if (Z_STRLEN_P(op1) == 1) {
				zend_uchar not = (zend_uchar) ~*Z_STRVAL_P(op1);
				if (CG(one_char_string)[not]) {
					ZVAL_INTERNED_STR(result, CG(one_char_string)[not]);
				} else {
					ZVAL_NEW_STR(result, zend_string_init((char *) &not, 1, 0));
				}
			} else {
				ZVAL_NEW_STR(result, zend_string_alloc(Z_STRLEN_P(op1), 0));
				for (i = 0; i < Z_STRLEN_P(op1); i++) {
					Z_STRVAL_P(result)[i] = ~Z_STRVAL_P(op1)[i];
				}
				Z_STRVAL_P(result)[i] = 0;
			}
			return SUCCESS;
		}
		case IS_REFERENCE:
			op1 = Z_REFVAL_P(op1);
			goto try_again;
		default:
			ZEND_TRY_UNARY_OBJECT_OPERATION(ZEND_BW_NOT);

			zend_error(E_EXCEPTION | E_ERROR, "Unsupported operand types");
			return FAILURE;
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL bitwise_or_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		ZVAL_LONG(result, Z_LVAL_P(op1) | Z_LVAL_P(op2));
		return SUCCESS;
	}

	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);

	if (Z_TYPE_P(op1) == IS_STRING && EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;

		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				zend_uchar or = (zend_uchar) (*Z_STRVAL_P(op1) | *Z_STRVAL_P(op2));
				if (CG(one_char_string)[or]) {
					ZVAL_INTERNED_STR(result, CG(one_char_string)[or]);
				} else {
					ZVAL_NEW_STR(result, zend_string_init((char *) &or, 1, 0));
				}
				return SUCCESS;
			}
			longer = op1;
			shorter = op2;
		} else {
			longer = op2;
			shorter = op1;
		}

		str = zend_string_alloc(Z_STRLEN_P(longer), 0);
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			str->val[i] = Z_STRVAL_P(longer)[i] | Z_STRVAL_P(shorter)[i];
		}
		memcpy(str->val + i, Z_STRVAL_P(longer) + i, Z_STRLEN_P(longer) - i + 1);
		if (result==op1) {
			zend_string_release(Z_STR_P(result));
		}
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}

	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_OR, bitwise_or_function);
		op1_lval = _zval_get_long_func(op1);
	} else {
		op1_lval = Z_LVAL_P(op1);
	}
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_OR);
		op2_lval = _zval_get_long_func(op2);
	} else {
		op2_lval = Z_LVAL_P(op2);
	}

	if (op1 == result) {
		zval_dtor(result);
	}
	ZVAL_LONG(result, op1_lval | op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL bitwise_and_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		ZVAL_LONG(result, Z_LVAL_P(op1) & Z_LVAL_P(op2));
		return SUCCESS;
	}

	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);

	if (Z_TYPE_P(op1) == IS_STRING && Z_TYPE_P(op2) == IS_STRING) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;

		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				zend_uchar and = (zend_uchar) (*Z_STRVAL_P(op1) & *Z_STRVAL_P(op2));
				if (CG(one_char_string)[and]) {
					ZVAL_INTERNED_STR(result, CG(one_char_string)[and]);
				} else {
					ZVAL_NEW_STR(result, zend_string_init((char *) &and, 1, 0));
				}
				return SUCCESS;
			}
			longer = op1;
			shorter = op2;
		} else {
			longer = op2;
			shorter = op1;
		}

		str = zend_string_alloc(Z_STRLEN_P(shorter), 0);
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			str->val[i] = Z_STRVAL_P(shorter)[i] & Z_STRVAL_P(longer)[i];
		}
		str->val[i] = 0;
		if (result==op1) {
			zend_string_release(Z_STR_P(result));
		}
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}

	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_AND, bitwise_or_function);
		op1_lval = _zval_get_long_func(op1);
	} else {
		op1_lval = Z_LVAL_P(op1);
	}
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_AND);
		op2_lval = _zval_get_long_func(op2);
	} else {
		op2_lval = Z_LVAL_P(op2);
	}

	if (op1 == result) {
		zval_dtor(result);
	}
	ZVAL_LONG(result, op1_lval & op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL bitwise_xor_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	if (EXPECTED(Z_TYPE_P(op1) == IS_LONG) && EXPECTED(Z_TYPE_P(op2) == IS_LONG)) {
		ZVAL_LONG(result, Z_LVAL_P(op1) ^ Z_LVAL_P(op2));
		return SUCCESS;
	}

	ZVAL_DEREF(op1);
	ZVAL_DEREF(op2);

	if (Z_TYPE_P(op1) == IS_STRING && Z_TYPE_P(op2) == IS_STRING) {
		zval *longer, *shorter;
		zend_string *str;
		size_t i;

		if (EXPECTED(Z_STRLEN_P(op1) >= Z_STRLEN_P(op2))) {
			if (EXPECTED(Z_STRLEN_P(op1) == Z_STRLEN_P(op2)) && Z_STRLEN_P(op1) == 1) {
				zend_uchar xor = (zend_uchar) (*Z_STRVAL_P(op1) ^ *Z_STRVAL_P(op2));
				if (CG(one_char_string)[xor]) {
					ZVAL_INTERNED_STR(result, CG(one_char_string)[xor]);
				} else {
					ZVAL_NEW_STR(result, zend_string_init((char *) &xor, 1, 0));
				}
				return SUCCESS;
			}
			longer = op1;
			shorter = op2;
		} else {
			longer = op2;
			shorter = op1;
		}

		str = zend_string_alloc(Z_STRLEN_P(shorter), 0);
		for (i = 0; i < Z_STRLEN_P(shorter); i++) {
			str->val[i] = Z_STRVAL_P(shorter)[i] ^ Z_STRVAL_P(longer)[i];
		}
		str->val[i] = 0;
		if (result==op1) {
			zend_string_release(Z_STR_P(result));
		}
		ZVAL_NEW_STR(result, str);
		return SUCCESS;
	}

	if (UNEXPECTED(Z_TYPE_P(op1) != IS_LONG)) {
		ZEND_TRY_BINARY_OP1_OBJECT_OPERATION(ZEND_BW_XOR, bitwise_or_function);
		op1_lval = _zval_get_long_func(op1);
	} else {
		op1_lval = Z_LVAL_P(op1);
	}
	if (UNEXPECTED(Z_TYPE_P(op2) != IS_LONG)) {
		ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_BW_XOR);
		op2_lval = _zval_get_long_func(op2);
	} else {
		op2_lval = Z_LVAL_P(op2);
	}

	if (op1 == result) {
		zval_dtor(result);
	}
	ZVAL_LONG(result, op1_lval ^ op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL shift_left_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, ZEND_SL, shift_left_function);

	if (op1 == result) {
		zval_dtor(result);
	}

	/* prevent wrapping quirkiness on some processors where << 64 + x == << x */
	if (UNEXPECTED((zend_ulong)op2_lval >= SIZEOF_ZEND_LONG * 8)) {
		if (EXPECTED(op2_lval > 0)) {
			ZVAL_LONG(result, 0);
			return SUCCESS;
		} else {
			if (EG(current_execute_data) && !CG(in_compilation)) {
				zend_throw_exception_ex(NULL, 0, "Bit shift by negative number");
			} else {
				zend_error_noreturn(E_ERROR, "Bit shift by negative number");
			}
			ZVAL_UNDEF(result);
			return FAILURE;
		}
	}

	ZVAL_LONG(result, op1_lval << op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL shift_right_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_long op1_lval, op2_lval;

	convert_op1_op2_long(op1, op1_lval, op2, op2_lval, ZEND_SR, shift_right_function);

	if (op1 == result) {
		zval_dtor(result);
	}

	/* prevent wrapping quirkiness on some processors where >> 64 + x == >> x */
	if (UNEXPECTED((zend_ulong)op2_lval >= SIZEOF_ZEND_LONG * 8)) {
		if (EXPECTED(op2_lval > 0)) {
			ZVAL_LONG(result, (op1_lval < 0) ? -1 : 0);
			return SUCCESS;
		} else {
			if (EG(current_execute_data) && !CG(in_compilation)) {
				zend_throw_exception_ex(NULL, 0, "Bit shift by negative number");
			} else {
				zend_error_noreturn(E_ERROR, "Bit shift by negative number");
			}
			ZVAL_UNDEF(result);
			return FAILURE;
		}
	}

	ZVAL_LONG(result, op1_lval >> op2_lval);
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL concat_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zval op1_copy, op2_copy;
	int use_copy1 = 0, use_copy2 = 0;

	do {
	 	if (UNEXPECTED(Z_TYPE_P(op1) != IS_STRING)) {
	 		if (Z_ISREF_P(op1)) {
	 			op1 = Z_REFVAL_P(op1);
	 			if (Z_TYPE_P(op1) == IS_STRING) break;
	 		}
			ZEND_TRY_BINARY_OBJECT_OPERATION(ZEND_CONCAT, concat_function);
			use_copy1 = zend_make_printable_zval(op1, &op1_copy);
			if (use_copy1) {
				/* We have created a converted copy of op1. Therefore, op1 won't become the result so
				 * we have to free it.
				 */
				if (result == op1) {
					zval_dtor(op1);
					if (UNEXPECTED(op1 == op2)) {
						op2 = &op1_copy;
					}
				}
				op1 = &op1_copy;
			}
		}
	} while (0);
	do {
		if (UNEXPECTED(Z_TYPE_P(op2) != IS_STRING)) {
	 		if (Z_ISREF_P(op2)) {
	 			op2 = Z_REFVAL_P(op2);
	 			if (Z_TYPE_P(op2) == IS_STRING) break;
	 		}
			ZEND_TRY_BINARY_OP2_OBJECT_OPERATION(ZEND_CONCAT);
			use_copy2 = zend_make_printable_zval(op2, &op2_copy);
			if (use_copy2) {
				op2 = &op2_copy;
			}
		}
	} while (0);

	{
		size_t op1_len = Z_STRLEN_P(op1);
		size_t op2_len = Z_STRLEN_P(op2);
		size_t result_len = op1_len + op2_len;
		zend_string *result_str;

		if (UNEXPECTED(op1_len > SIZE_MAX - op2_len)) {
			zend_error(E_EXCEPTION | E_ERROR, "String size overflow");
			ZVAL_FALSE(result);
			return FAILURE;
		}

		if (result == op1 && Z_REFCOUNTED_P(result)) {
			/* special case, perform operations on result */
			result_str = zend_string_extend(Z_STR_P(result), result_len, 0);
		} else {
			result_str = zend_string_alloc(result_len, 0);
			memcpy(result_str->val, Z_STRVAL_P(op1), op1_len);
		}

		/* This has to happen first to account for the cases where result == op1 == op2 and
		 * the realloc is done. In this case this line will also update Z_STRVAL_P(op2) to
		 * point to the new string. The first op2_len bytes of result will still be the same. */
		ZVAL_NEW_STR(result, result_str);

		memcpy(result_str->val + op1_len, Z_STRVAL_P(op2), op2_len);
		result_str->val[result_len] = '\0';
	}

	if (UNEXPECTED(use_copy1)) {
		zval_dtor(op1);
	}
	if (UNEXPECTED(use_copy2)) {
		zval_dtor(op2);
	}
	return SUCCESS;
}
/* }}} */

ZEND_API int string_compare_function_ex(zval *result, zval *op1, zval *op2, zend_bool case_insensitive) /* {{{ */
{
	zend_string *str1 = zval_get_string(op1);
	zend_string *str2 = zval_get_string(op2);

	if (case_insensitive) {
		ZVAL_LONG(result, zend_binary_strcasecmp_l(str1->val, str1->len, str2->val, str1->len));
	} else {
		ZVAL_LONG(result, zend_binary_strcmp(str1->val, str1->len, str2->val, str2->len));
	}

	zend_string_release(str1);
	zend_string_release(str2);
	return SUCCESS;
}
/* }}} */

ZEND_API int string_compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING) &&
	    EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		if (Z_STR_P(op1) == Z_STR_P(op2)) {
			ZVAL_LONG(result, 0);
		} else {
			ZVAL_LONG(result, zend_binary_strcmp(Z_STRVAL_P(op1), Z_STRLEN_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op2)));
		}
	} else {
		zend_string *str1 = zval_get_string(op1);
		zend_string *str2 = zval_get_string(op2);

		ZVAL_LONG(result, zend_binary_strcmp(str1->val, str1->len, str2->val, str2->len));

		zend_string_release(str1);
		zend_string_release(str2);
	}
	return SUCCESS;
}
/* }}} */

ZEND_API int string_case_compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (EXPECTED(Z_TYPE_P(op1) == IS_STRING) &&
	    EXPECTED(Z_TYPE_P(op2) == IS_STRING)) {
		if (Z_STR_P(op1) == Z_STR_P(op2)) {
			ZVAL_LONG(result, 0);
		} else {
			ZVAL_LONG(result, zend_binary_strcasecmp_l(Z_STRVAL_P(op1), Z_STRLEN_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op2)));
		}
	} else {
		zend_string *str1 = zval_get_string(op1);
		zend_string *str2 = zval_get_string(op2);

		ZVAL_LONG(result, zend_binary_strcasecmp_l(str1->val, str1->len, str2->val, str1->len));

		zend_string_release(str1);
		zend_string_release(str2);
	}
	return SUCCESS;
}
/* }}} */

#if HAVE_STRCOLL
ZEND_API int string_locale_compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	zend_string *str1 = zval_get_string(op1);
	zend_string *str2 = zval_get_string(op2);

	ZVAL_LONG(result, strcoll(str1->val, str2->val));

	zend_string_release(str1);
	zend_string_release(str2);
	return SUCCESS;
}
/* }}} */
#endif

ZEND_API int numeric_compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	double d1, d2;

	d1 = zval_get_double(op1);
	d2 = zval_get_double(op2);

	ZVAL_LONG(result, ZEND_NORMALIZE_BOOL(d1 - d2));

	return SUCCESS;
}
/* }}} */

static inline void zend_free_obj_get_result(zval *op) /* {{{ */
{
	if (Z_REFCOUNTED_P(op)) {
		if (Z_REFCOUNT_P(op) == 0) {
			zval_dtor(op);
		} else {
			zval_ptr_dtor(op);
		}
	}
}
/* }}} */

ZEND_API int compare_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	int ret;
	int converted = 0;
	zval op1_copy, op2_copy;
	zval *op_free, tmp_free;

	while (1) {
		switch (TYPE_PAIR(Z_TYPE_P(op1), Z_TYPE_P(op2))) {
			case TYPE_PAIR(IS_LONG, IS_LONG):
				ZVAL_LONG(result, Z_LVAL_P(op1)>Z_LVAL_P(op2)?1:(Z_LVAL_P(op1)<Z_LVAL_P(op2)?-1:0));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_LONG):
				Z_DVAL_P(result) = Z_DVAL_P(op1) - (double)Z_LVAL_P(op2);
				ZVAL_LONG(result, ZEND_NORMALIZE_BOOL(Z_DVAL_P(result)));
				return SUCCESS;

			case TYPE_PAIR(IS_LONG, IS_DOUBLE):
				Z_DVAL_P(result) = (double)Z_LVAL_P(op1) - Z_DVAL_P(op2);
				ZVAL_LONG(result, ZEND_NORMALIZE_BOOL(Z_DVAL_P(result)));
				return SUCCESS;

			case TYPE_PAIR(IS_DOUBLE, IS_DOUBLE):
				if (Z_DVAL_P(op1) == Z_DVAL_P(op2)) {
					ZVAL_LONG(result, 0);
				} else {
					Z_DVAL_P(result) = Z_DVAL_P(op1) - Z_DVAL_P(op2);
					ZVAL_LONG(result, ZEND_NORMALIZE_BOOL(Z_DVAL_P(result)));
				}
				return SUCCESS;

			case TYPE_PAIR(IS_ARRAY, IS_ARRAY):
				ZVAL_LONG(result, zend_compare_arrays(op1, op2));
				return SUCCESS;

			case TYPE_PAIR(IS_NULL, IS_NULL):
			case TYPE_PAIR(IS_NULL, IS_FALSE):
			case TYPE_PAIR(IS_FALSE, IS_NULL):
			case TYPE_PAIR(IS_FALSE, IS_FALSE):
			case TYPE_PAIR(IS_TRUE, IS_TRUE):
				ZVAL_LONG(result, 0);
				return SUCCESS;

			case TYPE_PAIR(IS_NULL, IS_TRUE):
				ZVAL_LONG(result, -1);
				return SUCCESS;

			case TYPE_PAIR(IS_TRUE, IS_NULL):
				ZVAL_LONG(result, 1);
				return SUCCESS;

			case TYPE_PAIR(IS_STRING, IS_STRING):
				if (Z_STR_P(op1) == Z_STR_P(op2)) {
					ZVAL_LONG(result, 0);
					return SUCCESS;
				}
				ZVAL_LONG(result, zendi_smart_strcmp(op1, op2));
				return SUCCESS;

			case TYPE_PAIR(IS_NULL, IS_STRING):
				ZVAL_LONG(result, Z_STRLEN_P(op2) == 0 ? 0 : -1);
				return SUCCESS;

			case TYPE_PAIR(IS_STRING, IS_NULL):
				ZVAL_LONG(result, Z_STRLEN_P(op1) == 0 ? 0 : 1);
				return SUCCESS;

			case TYPE_PAIR(IS_OBJECT, IS_NULL):
				ZVAL_LONG(result, 1);
				return SUCCESS;

			case TYPE_PAIR(IS_NULL, IS_OBJECT):
				ZVAL_LONG(result, -1);
				return SUCCESS;

			default:
				if (Z_ISREF_P(op1)) {
					op1 = Z_REFVAL_P(op1);
					continue;
				} else if (Z_ISREF_P(op2)) {
					op2 = Z_REFVAL_P(op2);
					continue;
				}

				if (Z_TYPE_P(op1) == IS_OBJECT && Z_OBJ_HANDLER_P(op1, compare)) {
					return Z_OBJ_HANDLER_P(op1, compare)(result, op1, op2);
				} else if (Z_TYPE_P(op2) == IS_OBJECT && Z_OBJ_HANDLER_P(op2, compare)) {
					return Z_OBJ_HANDLER_P(op2, compare)(result, op1, op2);
				}

				if (Z_TYPE_P(op1) == IS_OBJECT && Z_TYPE_P(op2) == IS_OBJECT) {
					if (Z_OBJ_P(op1) == Z_OBJ_P(op2)) {
						/* object handles are identical, apparently this is the same object */
						ZVAL_LONG(result, 0);
						return SUCCESS;
					}
					if (Z_OBJ_HANDLER_P(op1, compare_objects) == Z_OBJ_HANDLER_P(op2, compare_objects)) {
						ZVAL_LONG(result, Z_OBJ_HANDLER_P(op1, compare_objects)(op1, op2));
						return SUCCESS;
					}
				}
				if (Z_TYPE_P(op1) == IS_OBJECT) {
					if (Z_OBJ_HT_P(op1)->get) {
						zval rv;
						op_free = Z_OBJ_HT_P(op1)->get(op1, &rv);
						ret = compare_function(result, op_free, op2);
						zend_free_obj_get_result(op_free);
						return ret;
					} else if (Z_TYPE_P(op2) != IS_OBJECT && Z_OBJ_HT_P(op1)->cast_object) {
						ZVAL_UNDEF(&tmp_free);
						if (Z_OBJ_HT_P(op1)->cast_object(op1, &tmp_free, ((Z_TYPE_P(op2) == IS_FALSE || Z_TYPE_P(op2) == IS_TRUE) ? _IS_BOOL : Z_TYPE_P(op2))) == FAILURE) {
							ZVAL_LONG(result, 1);
							zend_free_obj_get_result(&tmp_free);
							return SUCCESS;
						}
						ret = compare_function(result, &tmp_free, op2);
						zend_free_obj_get_result(&tmp_free);
						return ret;
					}
				}
				if (Z_TYPE_P(op2) == IS_OBJECT) {
					if (Z_OBJ_HT_P(op2)->get) {
						zval rv;
						op_free = Z_OBJ_HT_P(op2)->get(op2, &rv);
						ret = compare_function(result, op1, op_free);
						zend_free_obj_get_result(op_free);
						return ret;
					} else if (Z_TYPE_P(op1) != IS_OBJECT && Z_OBJ_HT_P(op2)->cast_object) {
						ZVAL_UNDEF(&tmp_free);
						if (Z_OBJ_HT_P(op2)->cast_object(op2, &tmp_free, ((Z_TYPE_P(op1) == IS_FALSE || Z_TYPE_P(op1) == IS_TRUE) ? _IS_BOOL : Z_TYPE_P(op1))) == FAILURE) {
							ZVAL_LONG(result, -1);
							zend_free_obj_get_result(&tmp_free);
							return SUCCESS;
						}
						ret = compare_function(result, op1, &tmp_free);
						zend_free_obj_get_result(&tmp_free);
						return ret;
					} else if (Z_TYPE_P(op1) == IS_OBJECT) {
						ZVAL_LONG(result, 1);
						return SUCCESS;
					}
				}
				if (!converted) {
					if (Z_TYPE_P(op1) == IS_NULL || Z_TYPE_P(op1) == IS_FALSE) {
						ZVAL_LONG(result, zval_is_true(op2) ? -1 : 0);
						return SUCCESS;
					} else if (Z_TYPE_P(op2) == IS_NULL || Z_TYPE_P(op2) == IS_FALSE) {
						ZVAL_LONG(result, zval_is_true(op1) ? 1 : 0);
						return SUCCESS;
					} else if (Z_TYPE_P(op1) == IS_TRUE) {
						ZVAL_LONG(result, zval_is_true(op2) ? 0 : 1);
						return SUCCESS;
					} else if (Z_TYPE_P(op2) == IS_TRUE) {
						ZVAL_LONG(result, zval_is_true(op1) ? 0 : -1);
						return SUCCESS;
					} else {
						zendi_convert_scalar_to_number(op1, op1_copy, result);
						zendi_convert_scalar_to_number(op2, op2_copy, result);
						converted = 1;
					}
				} else if (Z_TYPE_P(op1)==IS_ARRAY) {
					ZVAL_LONG(result, 1);
					return SUCCESS;
				} else if (Z_TYPE_P(op2)==IS_ARRAY) {
					ZVAL_LONG(result, -1);
					return SUCCESS;
				} else if (Z_TYPE_P(op1)==IS_OBJECT) {
					ZVAL_LONG(result, 1);
					return SUCCESS;
				} else if (Z_TYPE_P(op2)==IS_OBJECT) {
					ZVAL_LONG(result, -1);
					return SUCCESS;
				} else {
					ZVAL_LONG(result, 0);
					return FAILURE;
				}
		}
	}
}
/* }}} */

static int hash_zval_identical_function(zval *z1, zval *z2) /* {{{ */
{
	zval result;

	/* is_identical_function() returns 1 in case of identity and 0 in case
	 * of a difference;
	 * whereas this comparison function is expected to return 0 on identity,
	 * and non zero otherwise.
	 */
	ZVAL_DEREF(z1);
	ZVAL_DEREF(z2);
	if (is_identical_function(&result, z1, z2)==FAILURE) {
		return 1;
	}
	return Z_TYPE(result) != IS_TRUE;
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_is_identical(zval *op1, zval *op2) /* {{{ */
{
	if (Z_TYPE_P(op1) != Z_TYPE_P(op2)) {
		return 0;
	}
	switch (Z_TYPE_P(op1)) {
		case IS_NULL:
		case IS_FALSE:
		case IS_TRUE:
			return 1;
		case IS_LONG:
			return (Z_LVAL_P(op1) == Z_LVAL_P(op2));
		case IS_RESOURCE:
			return (Z_RES_P(op1) == Z_RES_P(op2));
		case IS_DOUBLE:
			return (Z_DVAL_P(op1) == Z_DVAL_P(op2));
		case IS_STRING:
			return (Z_STR_P(op1) == Z_STR_P(op2) ||
				(Z_STRLEN_P(op1) == Z_STRLEN_P(op2) &&
				 memcmp(Z_STRVAL_P(op1), Z_STRVAL_P(op2), Z_STRLEN_P(op1)) == 0));
		case IS_ARRAY:
			return (Z_ARRVAL_P(op1) == Z_ARRVAL_P(op2) ||
				zend_hash_compare(Z_ARRVAL_P(op1), Z_ARRVAL_P(op2), (compare_func_t) hash_zval_identical_function, 1) == 0);
		case IS_OBJECT:
			return (Z_OBJ_P(op1) == Z_OBJ_P(op2) && Z_OBJ_HT_P(op1) == Z_OBJ_HT_P(op2));
		default:
			return 0;
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_identical_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, zend_is_identical(op1, op2));
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_not_identical_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	ZVAL_BOOL(result, !zend_is_identical(op1, op2));
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (compare_function(result, op1, op2) == FAILURE) {
		return FAILURE;
	}
	ZVAL_BOOL(result, (Z_LVAL_P(result) == 0));
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_not_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (compare_function(result, op1, op2) == FAILURE) {
		return FAILURE;
	}
	ZVAL_BOOL(result, (Z_LVAL_P(result) != 0));
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_smaller_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (compare_function(result, op1, op2) == FAILURE) {
		return FAILURE;
	}
	ZVAL_BOOL(result, (Z_LVAL_P(result) < 0));
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL is_smaller_or_equal_function(zval *result, zval *op1, zval *op2) /* {{{ */
{
	if (compare_function(result, op1, op2) == FAILURE) {
		return FAILURE;
	}
	ZVAL_BOOL(result, (Z_LVAL_P(result) <= 0));
	return SUCCESS;
}
/* }}} */

static zend_bool ZEND_FASTCALL instanceof_interface_only(const zend_class_entry *instance_ce, const zend_class_entry *ce) /* {{{ */
{
	uint32_t i;

	for (i = 0; i < instance_ce->num_interfaces; i++) {
		if (instanceof_interface_only(instance_ce->interfaces[i], ce)) {
			return 1;
		}
	}
	return 0;
}
/* }}} */

static zend_always_inline zend_bool instanceof_class(const zend_class_entry *instance_ce, const zend_class_entry *ce) /* {{{ */
{
	while (instance_ce) {
		if (instance_ce == ce) {
			return 1;
		}
		instance_ce = instance_ce->parent;
	}
	return 0;
}
/* }}} */

static zend_bool ZEND_FASTCALL instanceof_interface(const zend_class_entry *instance_ce, const zend_class_entry *ce) /* {{{ */
{
	uint32_t i;

	for (i = 0; i < instance_ce->num_interfaces; i++) {
		if (instanceof_interface(instance_ce->interfaces[i], ce)) {
			return 1;
		}
	}
	return instanceof_class(instance_ce, ce);
}
/* }}} */

ZEND_API zend_bool ZEND_FASTCALL instanceof_function_ex(const zend_class_entry *instance_ce, const zend_class_entry *ce, zend_bool interfaces_only) /* {{{ */
{
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		if (!interfaces_only) {
			if (instanceof_interface_only(instance_ce, ce)) {
				return 1;
			}
		} else {
			return instanceof_interface(instance_ce, ce);
		}
	}
	if (!interfaces_only) {
		return instanceof_class(instance_ce, ce);
	}
	return 0;
}
/* }}} */

ZEND_API zend_bool ZEND_FASTCALL instanceof_function(const zend_class_entry *instance_ce, const zend_class_entry *ce) /* {{{ */
{
	if (ce->ce_flags & ZEND_ACC_INTERFACE) {
		return instanceof_interface(instance_ce, ce);
	} else {
		return instanceof_class(instance_ce, ce);
	}
}
/* }}} */

#define LOWER_CASE 1
#define UPPER_CASE 2
#define NUMERIC 3

static void ZEND_FASTCALL increment_string(zval *str) /* {{{ */
{
	int carry=0;
	size_t pos=Z_STRLEN_P(str)-1;
	char *s;
	zend_string *t;
	int last=0; /* Shut up the compiler warning */
	int ch;

	if (Z_STRLEN_P(str) == 0) {
		zend_string_release(Z_STR_P(str));
		Z_STR_P(str) = zend_string_init("1", sizeof("1")-1, 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
		return;
	}

	if (!Z_REFCOUNTED_P(str)) {
		Z_STR_P(str) = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
		Z_TYPE_INFO_P(str) = IS_STRING_EX;
	} else if (Z_REFCOUNT_P(str) > 1) {
		Z_DELREF_P(str);
		Z_STR_P(str) = zend_string_init(Z_STRVAL_P(str), Z_STRLEN_P(str), 0);
	} else {
		zend_string_forget_hash_val(Z_STR_P(str));
	}
	s = Z_STRVAL_P(str);

	do {
		ch = s[pos];
		if (ch >= 'a' && ch <= 'z') {
			if (ch == 'z') {
				s[pos] = 'a';
				carry=1;
			} else {
				s[pos]++;
				carry=0;
			}
			last=LOWER_CASE;
		} else if (ch >= 'A' && ch <= 'Z') {
			if (ch == 'Z') {
				s[pos] = 'A';
				carry=1;
			} else {
				s[pos]++;
				carry=0;
			}
			last=UPPER_CASE;
		} else if (ch >= '0' && ch <= '9') {
			if (ch == '9') {
				s[pos] = '0';
				carry=1;
			} else {
				s[pos]++;
				carry=0;
			}
			last = NUMERIC;
		} else {
			carry=0;
			break;
		}
		if (carry == 0) {
			break;
		}
	} while (pos-- > 0);

	if (carry) {
		t = zend_string_alloc(Z_STRLEN_P(str)+1, 0);
		memcpy(t->val + 1, Z_STRVAL_P(str), Z_STRLEN_P(str));
		t->val[Z_STRLEN_P(str) + 1] = '\0';
		switch (last) {
			case NUMERIC:
				t->val[0] = '1';
				break;
			case UPPER_CASE:
				t->val[0] = 'A';
				break;
			case LOWER_CASE:
				t->val[0] = 'a';
				break;
		}
		zend_string_free(Z_STR_P(str));
		ZVAL_NEW_STR(str, t);
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL increment_function(zval *op1) /* {{{ */
{
try_again:
	switch (Z_TYPE_P(op1)) {
		case IS_LONG:
			fast_long_increment_function(op1);
			break;
		case IS_DOUBLE:
			Z_DVAL_P(op1) = Z_DVAL_P(op1) + 1;
			break;
		case IS_NULL:
			ZVAL_LONG(op1, 1);
			break;
		case IS_STRING: {
				zend_long lval;
				double dval;

				switch (is_numeric_string(Z_STRVAL_P(op1), Z_STRLEN_P(op1), &lval, &dval, 0)) {
					case IS_LONG:
						zend_string_release(Z_STR_P(op1));
						if (lval == ZEND_LONG_MAX) {
							/* switch to double */
							double d = (double)lval;
							ZVAL_DOUBLE(op1, d+1);
						} else {
							ZVAL_LONG(op1, lval+1);
						}
						break;
					case IS_DOUBLE:
						zend_string_release(Z_STR_P(op1));
						ZVAL_DOUBLE(op1, dval+1);
						break;
					default:
						/* Perl style string increment */
						increment_string(op1);
						break;
				}
			}
			break;
		case IS_OBJECT:
			if (Z_OBJ_HANDLER_P(op1, get)
			   && Z_OBJ_HANDLER_P(op1, set)) {
				/* proxy object */
				zval rv;
				zval *val;

				val = Z_OBJ_HANDLER_P(op1, get)(op1, &rv);
				Z_ADDREF_P(val);
				increment_function(val);
				Z_OBJ_HANDLER_P(op1, set)(op1, val);
				zval_ptr_dtor(val);
			} else if (Z_OBJ_HANDLER_P(op1, do_operation)) {
				zval op2;
				int res;

				ZVAL_LONG(&op2, 1);
				res = Z_OBJ_HANDLER_P(op1, do_operation)(ZEND_ADD, op1, op1, &op2);
				zval_ptr_dtor(&op2);

				return res;
			}
			return FAILURE;
		case IS_REFERENCE:
			op1 = Z_REFVAL_P(op1);
			goto try_again;
		default:
			return FAILURE;
	}
	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL decrement_function(zval *op1) /* {{{ */
{
	zend_long lval;
	double dval;

try_again:
	switch (Z_TYPE_P(op1)) {
		case IS_LONG:
			fast_long_decrement_function(op1);
			break;
		case IS_DOUBLE:
			Z_DVAL_P(op1) = Z_DVAL_P(op1) - 1;
			break;
		case IS_STRING:		/* Like perl we only support string increment */
			if (Z_STRLEN_P(op1) == 0) { /* consider as 0 */
				zend_string_release(Z_STR_P(op1));
				ZVAL_LONG(op1, -1);
				break;
			}
			switch (is_numeric_string(Z_STRVAL_P(op1), Z_STRLEN_P(op1), &lval, &dval, 0)) {
				case IS_LONG:
					zend_string_release(Z_STR_P(op1));
					if (lval == ZEND_LONG_MIN) {
						double d = (double)lval;
						ZVAL_DOUBLE(op1, d-1);
					} else {
						ZVAL_LONG(op1, lval-1);
					}
					break;
				case IS_DOUBLE:
					zend_string_release(Z_STR_P(op1));
					ZVAL_DOUBLE(op1, dval - 1);
					break;
			}
			break;
		case IS_OBJECT:
			if (Z_OBJ_HANDLER_P(op1, get)
			   && Z_OBJ_HANDLER_P(op1, set)) {
				/* proxy object */
				zval rv;
				zval *val;

				val = Z_OBJ_HANDLER_P(op1, get)(op1, &rv);
				Z_ADDREF_P(val);
				decrement_function(val);
				Z_OBJ_HANDLER_P(op1, set)(op1, val);
				zval_ptr_dtor(val);
			} else if (Z_OBJ_HANDLER_P(op1, do_operation)) {
				zval op2;
				int res;

				ZVAL_LONG(&op2, 1);
				res = Z_OBJ_HANDLER_P(op1, do_operation)(ZEND_SUB, op1, op1, &op2);
				zval_ptr_dtor(&op2);

				return res;
			}
			return FAILURE;
		case IS_REFERENCE:
			op1 = Z_REFVAL_P(op1);
			goto try_again;
		default:
			return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_is_true(zval *op) /* {{{ */
{
	return i_zend_is_true(op);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_object_is_true(zval *op) /* {{{ */
{
	if (Z_OBJ_HT_P(op)->cast_object) {
		zval tmp;
		if (Z_OBJ_HT_P(op)->cast_object(op, &tmp, _IS_BOOL) == SUCCESS) {
			return Z_TYPE(tmp) == IS_TRUE;
		}
		zend_error(E_RECOVERABLE_ERROR, "Object of class %s could not be converted to boolean", Z_OBJ_P(op)->ce->name->val);
	} else if (Z_OBJ_HT_P(op)->get) {
		int result;
		zval rv;
		zval *tmp = Z_OBJ_HT_P(op)->get(op, &rv);

		if (Z_TYPE_P(tmp) != IS_OBJECT) {
			/* for safety - avoid loop */
			result = i_zend_is_true(tmp);
			zval_ptr_dtor(tmp);
			return result;
		}
	}
	return 1;
}
/* }}} */

#ifdef ZEND_USE_TOLOWER_L
ZEND_API void zend_update_current_locale(void) /* {{{ */
{
	current_locale = _get_current_locale();
}
/* }}} */
#endif

ZEND_API char* ZEND_FASTCALL zend_str_tolower_copy(char *dest, const char *source, size_t length) /* {{{ */
{
	register unsigned char *str = (unsigned char*)source;
	register unsigned char *result = (unsigned char*)dest;
	register unsigned char *end = str + length;

	while (str < end) {
		*result++ = zend_tolower_ascii(*str++);
	}
	*result = '\0';

	return dest;
}
/* }}} */

ZEND_API char* ZEND_FASTCALL zend_str_tolower_dup(const char *source, size_t length) /* {{{ */
{
	return zend_str_tolower_copy((char *)emalloc(length+1), source, length);
}
/* }}} */

ZEND_API void ZEND_FASTCALL zend_str_tolower(char *str, size_t length) /* {{{ */
{
	register unsigned char *p = (unsigned char*)str;
	register unsigned char *end = p + length;

	while (p < end) {
		*p = zend_tolower_ascii(*p);
		p++;
	}
}
/* }}} */

ZEND_API zend_string* ZEND_FASTCALL zend_string_tolower(zend_string *str) /* {{{ */
{
	register unsigned char *p = (unsigned char*)str->val;
	register unsigned char *end = p + str->len;

	while (p < end) {
		if (*p != zend_tolower_ascii(*p)) {
			zend_string *res = zend_string_alloc(str->len, 0);
			register unsigned char *r;

			if (p != (unsigned char*)str->val) {
				memcpy(res->val, str->val, p - (unsigned char*)str->val);
			}
			r = p + (res->val - str->val);
			while (p < end) {
				*r = zend_tolower_ascii(*p);
				p++;
				r++;
			}
			*r = '\0';
			return res;
		}
		p++;
	}
	return zend_string_copy(str);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strcmp(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	int retval;

	if (s1 == s2) {
		return 0;
	}
	retval = memcmp(s1, s2, MIN(len1, len2));
	if (!retval) {
		return (int)(len1 - len2);
	} else {
		return retval;
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strncmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	int retval;

	if (s1 == s2) {
		return 0;
	}
	retval = memcmp(s1, s2, MIN(length, MIN(len1, len2)));
	if (!retval) {
		return (int)(MIN(length, len1) - MIN(length, len2));
	} else {
		return retval;
	}
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	size_t len;
	int c1, c2;

	if (s1 == s2) {
		return 0;
	}

	len = MIN(len1, len2);
	while (len--) {
		c1 = zend_tolower_ascii(*(unsigned char *)s1++);
		c2 = zend_tolower_ascii(*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}

	return (int)(len1 - len2);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	size_t len;
	int c1, c2;

	if (s1 == s2) {
		return 0;
	}
	len = MIN(length, MIN(len1, len2));
	while (len--) {
		c1 = zend_tolower_ascii(*(unsigned char *)s1++);
		c2 = zend_tolower_ascii(*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}

	return (int)(MIN(length, len1) - MIN(length, len2));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strcasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2) /* {{{ */
{
	size_t len;
	int c1, c2;

	if (s1 == s2) {
		return 0;
	}

	len = MIN(len1, len2);
	while (len--) {
		c1 = zend_tolower((int)*(unsigned char *)s1++);
		c2 = zend_tolower((int)*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}

	return (int)(len1 - len2);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_strncasecmp_l(const char *s1, size_t len1, const char *s2, size_t len2, size_t length) /* {{{ */
{
	size_t len;
	int c1, c2;

	if (s1 == s2) {
		return 0;
	}
	len = MIN(length, MIN(len1, len2));
	while (len--) {
		c1 = zend_tolower((int)*(unsigned char *)s1++);
		c2 = zend_tolower((int)*(unsigned char *)s2++);
		if (c1 != c2) {
			return c1 - c2;
		}
	}

	return (int)(MIN(length, len1) - MIN(length, len2));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_zval_strcmp(zval *s1, zval *s2) /* {{{ */
{
	return zend_binary_strcmp(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_zval_strncmp(zval *s1, zval *s2, zval *s3) /* {{{ */
{
	return zend_binary_strncmp(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2), Z_LVAL_P(s3));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_zval_strcasecmp(zval *s1, zval *s2) /* {{{ */
{
	return zend_binary_strcasecmp_l(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_binary_zval_strncasecmp(zval *s1, zval *s2, zval *s3) /* {{{ */
{
	return zend_binary_strncasecmp_l(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2), Z_LVAL_P(s3));
}
/* }}} */

ZEND_API zend_long ZEND_FASTCALL zendi_smart_strcmp(zval *s1, zval *s2) /* {{{ */
{
	int ret1, ret2;
	int oflow1, oflow2;
	zend_long lval1 = 0, lval2 = 0;
	double dval1 = 0.0, dval2 = 0.0;

	if ((ret1 = is_numeric_string_ex(Z_STRVAL_P(s1), Z_STRLEN_P(s1), &lval1, &dval1, 0, &oflow1)) &&
		(ret2 = is_numeric_string_ex(Z_STRVAL_P(s2), Z_STRLEN_P(s2), &lval2, &dval2, 0, &oflow2))) {
#if ZEND_ULONG_MAX == 0xFFFFFFFF
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0. &&
			((oflow1 == 1 && dval1 > 9007199254740991. /*0x1FFFFFFFFFFFFF*/)
			|| (oflow1 == -1 && dval1 < -9007199254740991.))) {
#else
		if (oflow1 != 0 && oflow1 == oflow2 && dval1 - dval2 == 0.) {
#endif
			/* both values are integers overflown to the same side, and the
			 * double comparison may have resulted in crucial accuracy lost */
			goto string_cmp;
		}
		if ((ret1 == IS_DOUBLE) || (ret2 == IS_DOUBLE)) {
			if (ret1 != IS_DOUBLE) {
				if (oflow2) {
					/* 2nd operand is integer > LONG_MAX (oflow2==1) or < LONG_MIN (-1) */
					return -1 * oflow2;
				}
				dval1 = (double) lval1;
			} else if (ret2 != IS_DOUBLE) {
				if (oflow1) {
					return oflow1;
				}
				dval2 = (double) lval2;
			} else if (dval1 == dval2 && !zend_finite(dval1)) {
				/* Both values overflowed and have the same sign,
				 * so a numeric comparison would be inaccurate */
				goto string_cmp;
			}
			dval1 = dval1 - dval2;
			return ZEND_NORMALIZE_BOOL(dval1);
		} else { /* they both have to be long's */
			return lval1 > lval2 ? 1 : (lval1 < lval2 ? -1 : 0);
		}
	} else {
		int strcmp_ret;
string_cmp:
		strcmp_ret = zend_binary_strcmp(Z_STRVAL_P(s1), Z_STRLEN_P(s1), Z_STRVAL_P(s2), Z_STRLEN_P(s2));
		return ZEND_NORMALIZE_BOOL(strcmp_ret);
	}
}
/* }}} */

static int hash_zval_compare_function(zval *z1, zval *z2) /* {{{ */
{
	zval result;

	if (compare_function(&result, z1, z2)==FAILURE) {
		return 1;
	}
	return Z_LVAL(result);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_compare_symbol_tables(HashTable *ht1, HashTable *ht2) /* {{{ */
{
	return ht1 == ht2 ? 0 : zend_hash_compare(ht1, ht2, (compare_func_t) hash_zval_compare_function, 0);
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_compare_arrays(zval *a1, zval *a2) /* {{{ */
{
	return zend_compare_symbol_tables(Z_ARRVAL_P(a1), Z_ARRVAL_P(a2));
}
/* }}} */

ZEND_API int ZEND_FASTCALL zend_compare_objects(zval *o1, zval *o2) /* {{{ */
{
	if (Z_OBJ_P(o1) == Z_OBJ_P(o2)) {
		return 0;
	}

	if (Z_OBJ_HT_P(o1)->compare_objects == NULL) {
		return 1;
	} else {
		return Z_OBJ_HT_P(o1)->compare_objects(o1, o2);
	}
}
/* }}} */

ZEND_API void ZEND_FASTCALL zend_locale_sprintf_double(zval *op ZEND_FILE_LINE_DC) /* {{{ */
{
	zend_string *str;

	str = zend_strpprintf(0, "%.*G", (int) EG(precision), (double)Z_DVAL_P(op));
	ZVAL_NEW_STR(op, str);
}
/* }}} */

ZEND_API zend_string* ZEND_FASTCALL zend_long_to_str(zend_long num) /* {{{ */
{
	char buf[MAX_LENGTH_OF_LONG + 1];
	char *res = zend_print_long_to_buf(buf + sizeof(buf) - 1, num);
	return zend_string_init(res, buf + sizeof(buf) - 1 - res, 0);
}
/* }}} */

ZEND_API zend_uchar ZEND_FASTCALL is_numeric_str_function(const zend_string *str, zend_long *lval, double *dval) /* {{{ */ {
    return is_numeric_string_ex(str->val, str->len, lval, dval, -1, NULL);
}
/* }}} */

ZEND_API zend_uchar ZEND_FASTCALL _is_numeric_string_ex(const char *str, size_t length, zend_long *lval, double *dval, int allow_errors, int *oflow_info) /* {{{ */
{
	const char *ptr;
	int digits = 0, dp_or_e = 0;
	double local_dval = 0.0;
	zend_uchar type;
	zend_long tmp_lval = 0;
	int neg = 0;

	if (!length) {
		return 0;
	}

	if (oflow_info != NULL) {
		*oflow_info = 0;
	}

	/* Skip any whitespace
	 * This is much faster than the isspace() function */
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r' || *str == '\v' || *str == '\f') {
		str++;
		length--;
	}
	ptr = str;

	if (*ptr == '-') {
		neg = 1;
		ptr++;
	} else if (*ptr == '+') {
		ptr++;
	}

	if (ZEND_IS_DIGIT(*ptr)) {
		/* Skip any leading 0s */
		while (*ptr == '0') {
			ptr++;
		}

		/* Count the number of digits. If a decimal point/exponent is found,
		 * it's a double. Otherwise, if there's a dval or no need to check for
		 * a full match, stop when there are too many digits for a long */
		for (type = IS_LONG; !(digits >= MAX_LENGTH_OF_LONG && (dval || allow_errors == 1)); digits++, ptr++) {
check_digits:
			if (ZEND_IS_DIGIT(*ptr)) {
				tmp_lval = tmp_lval * 10 + (*ptr) - '0';
				continue;
			} else if (*ptr == '.' && dp_or_e < 1) {
				goto process_double;
			} else if ((*ptr == 'e' || *ptr == 'E') && dp_or_e < 2) {
				const char *e = ptr + 1;

				if (*e == '-' || *e == '+') {
					ptr = e++;
				}
				if (ZEND_IS_DIGIT(*e)) {
					goto process_double;
				}
			}

			break;
		}

		if (digits >= MAX_LENGTH_OF_LONG) {
			if (oflow_info != NULL) {
				*oflow_info = *str == '-' ? -1 : 1;
			}
			dp_or_e = -1;
			goto process_double;
		}
	} else if (*ptr == '.' && ZEND_IS_DIGIT(ptr[1])) {
process_double:
		type = IS_DOUBLE;

		/* If there's a dval, do the conversion; else continue checking
		 * the digits if we need to check for a full match */
		if (dval) {
			local_dval = zend_strtod(str, &ptr);
		} else if (allow_errors != 1 && dp_or_e != -1) {
			dp_or_e = (*ptr++ == '.') ? 1 : 2;
			goto check_digits;
		}
	} else {
		return 0;
	}

	if (ptr != str + length) {
		if (!allow_errors) {
			return 0;
		}
		if (allow_errors == -1) {
			zend_error(E_NOTICE, "A non well formed numeric value encountered");
		}
	}

	if (type == IS_LONG) {
		if (digits == MAX_LENGTH_OF_LONG - 1) {
			int cmp = strcmp(&ptr[-digits], long_min_digits);

			if (!(cmp < 0 || (cmp == 0 && *str == '-'))) {
				if (dval) {
					*dval = zend_strtod(str, NULL);
				}
				if (oflow_info != NULL) {
					*oflow_info = *str == '-' ? -1 : 1;
				}

				return IS_DOUBLE;
			}
		}

		if (lval) {
			if (neg) {
				tmp_lval = -tmp_lval;
			}
			*lval = tmp_lval;
		}

		return IS_LONG;
	} else {
		if (dval) {
			*dval = local_dval;
		}

		return IS_DOUBLE;
	}
}
/* }}} */

/* 
 * String matching - Sunday algorithm
 * http://www.iti.fh-flensburg.de/lang/algorithmen/pattern/sundayen.htm
 */
static zend_always_inline void zend_memnstr_ex_pre(unsigned int td[], const char *needle, size_t needle_len, int reverse) /* {{{ */ {
	int i;

	for (i = 0; i < 256; i++) {
		td[i] = needle_len + 1;
	}

	if (reverse) {
		for (i = needle_len - 1; i >= 0; i--) {
			td[(unsigned char)needle[i]] = i + 1;
		}
	} else {
		for (i = 0; i < needle_len; i++) {
			td[(unsigned char)needle[i]] = (int)needle_len - i;
		}
	}
}
/* }}} */

ZEND_API const char* ZEND_FASTCALL zend_memnstr_ex(const char *haystack, const char *needle, size_t needle_len, char *end) /* {{{ */
{
	unsigned int td[256];
	register size_t i;
	register const char *p;

	if (needle_len == 0 || (end - haystack) == 0) {
		return NULL;
	}

	zend_memnstr_ex_pre(td, needle, needle_len, 0);

	p = haystack;
	end -= needle_len;

	while (p <= end) {
		for (i = 0; i < needle_len; i++) {
			if (needle[i] != p[i]) {
				break;
			}
		}
		if (i == needle_len) {
			return p;
		}
		p += td[(unsigned char)(p[needle_len])];
	}

	return NULL;
}
/* }}} */

ZEND_API const char* ZEND_FASTCALL zend_memnrstr_ex(const char *haystack, const char *needle, size_t needle_len, char *end) /* {{{ */
{
	unsigned int td[256];
	register size_t i;
	register const char *p;

	if (needle_len == 0 || (end - haystack) == 0) {
		return NULL;
	}

	zend_memnstr_ex_pre(td, needle, needle_len, 1);

	p = end;
	p -= needle_len;

	while (p >= haystack) {
		for (i = 0; i < needle_len; i++) {
			if (needle[i] != p[i]) {
				break;
			}
		}

		if (i == needle_len) {
			return (const char *)p;
		}
		
		if (UNEXPECTED(p == haystack)) {
			return NULL;
		}

		p -= td[(unsigned char)(p[-1])];
	}

	return NULL;
}
/* }}} */

#if !ZEND_DVAL_TO_LVAL_CAST_OK
# if SIZEOF_ZEND_LONG == 4
ZEND_API zend_long ZEND_FASTCALL zend_dval_to_lval_slow(double d)
{
	double	two_pow_32 = pow(2., 32.),
			dmod;

	dmod = fmod(d, two_pow_32);
	if (dmod < 0) {
		/* we're going to make this number positive; call ceil()
		 * to simulate rounding towards 0 of the negative number */
		dmod = ceil(dmod);// + two_pow_32;
	}
	return (zend_long)(zend_ulong)dmod;
}
#else
ZEND_API zend_long ZEND_FASTCALL zend_dval_to_lval_slow(double d)
{
	double	two_pow_64 = pow(2., 64.),
			dmod;

	dmod = fmod(d, two_pow_64);
	if (dmod < 0) {
		/* no need to call ceil; original double must have had no
		 * fractional part, hence dmod does not have one either */
		dmod += two_pow_64;
	}
	return (zend_long)(zend_ulong)dmod;
}
#endif
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
