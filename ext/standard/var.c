/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2014 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jani Lehtimäki <jkl@njet.net>                               |
   |          Thies C. Arntzen <thies@thieso.net>                         |
   |          Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

/* {{{ includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "php.h"
#include "php_string.h"
#include "php_var.h"
#include "php_smart_str.h"
#include "basic_functions.h"
#include "php_incomplete_class.h"

#define COMMON (is_ref ? "&" : "")
/* }}} */

static int php_array_element_dump(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;

	level = va_arg(args, int);

	if (hash_key->key == NULL) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		php_printf("%*c[\"", level + 1, ' ');
		PHPWRITE(hash_key->key->val, hash_key->key->len);
		php_printf("\"]=>\n");
	}
	php_var_dump(zv, level + 2 TSRMLS_CC);
	return 0;
}
/* }}} */

static int php_object_property_dump(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	const char *prop_name, *class_name;

	level = va_arg(args, int);

	if (hash_key->key == NULL) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		int unmangle = zend_unmangle_property_name(hash_key->key->val, hash_key->key->len, &class_name, &prop_name);
		php_printf("%*c[", level + 1, ' ');

		if (class_name && unmangle == SUCCESS) {
			if (class_name[0] == '*') {
				php_printf("\"%s\":protected", prop_name);
			} else {
				php_printf("\"%s\":\"%s\":private", prop_name, class_name);
			}
		} else {
			php_printf("\"");
			PHPWRITE(hash_key->key->val, hash_key->key->len);
			php_printf("\"");
		}
		ZEND_PUTS("]=>\n");
	}
	php_var_dump(zv, level + 2 TSRMLS_CC);
	return 0;
}
/* }}} */

PHPAPI void php_var_dump(zval *struc, int level TSRMLS_DC) /* {{{ */
{
	HashTable *myht;
	zend_string *class_name;
	int (*php_element_dump_func)(zval* TSRMLS_DC, int, va_list, zend_hash_key*);
	int is_temp;
	int is_ref = 0;

	if (level > 1) {
		php_printf("%*c", level - 1, ' ');
	}

	if (Z_TYPE_P(struc) == IS_REFERENCE) {
		is_ref = 1;
		struc = Z_REFVAL_P(struc);
	}
	
	switch (Z_TYPE_P(struc)) {
	case IS_BOOL:
		php_printf("%sbool(%s)\n", COMMON, Z_LVAL_P(struc) ? "true" : "false");
		break;
	case IS_NULL:
		php_printf("%sNULL\n", COMMON);
		break;
	case IS_LONG:
		php_printf("%sint(%ld)\n", COMMON, Z_LVAL_P(struc));
		break;
	case IS_DOUBLE:
		php_printf("%sfloat(%.*G)\n", COMMON, (int) EG(precision), Z_DVAL_P(struc));
		break;
	case IS_STRING:
		php_printf("%sstring(%d) \"", COMMON, Z_STRLEN_P(struc));
		PHPWRITE(Z_STRVAL_P(struc), Z_STRLEN_P(struc));
		PUTS("\"\n");
		break;
	case IS_ARRAY:
		myht = Z_ARRVAL_P(struc);
		if (++myht->nApplyCount > 1) {
			PUTS("*RECURSION*\n");
			--myht->nApplyCount;
			return;
		}
		php_printf("%sarray(%d) {\n", COMMON, zend_hash_num_elements(myht));
		php_element_dump_func = php_array_element_dump;
		is_temp = 0;
		goto head_done;
	case IS_OBJECT:
		myht = Z_OBJDEBUG_P(struc, is_temp);
		if (myht && ++myht->nApplyCount > 1) {
			PUTS("*RECURSION*\n");
			--myht->nApplyCount;
			return;
		}

		if (Z_OBJ_HANDLER_P(struc, get_class_name)) {
			class_name = Z_OBJ_HANDLER_P(struc, get_class_name)(struc, 0 TSRMLS_CC);
			php_printf("%sobject(%s)#%d (%d) {\n", COMMON, class_name->val, Z_OBJ_HANDLE_P(struc), myht ? zend_hash_num_elements(myht) : 0);
			STR_RELEASE(class_name);
		} else {
			php_printf("%sobject(unknown class)#%d (%d) {\n", COMMON, Z_OBJ_HANDLE_P(struc), myht ? zend_hash_num_elements(myht) : 0);
		}
		php_element_dump_func = php_object_property_dump;
head_done:
		if (myht) {
			zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) php_element_dump_func, 1, level);
			--myht->nApplyCount;
			if (is_temp) {
				zend_hash_destroy(myht);
				efree(myht);
			}
		}
		if (level > 1) {
			php_printf("%*c", level-1, ' ');
		}
		PUTS("}\n");
		break;
	case IS_RESOURCE: {
		const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(struc) TSRMLS_CC);
		php_printf("%sresource(%ld) of type (%s)\n", COMMON, Z_RES_P(struc)->handle, type_name ? type_name : "Unknown");
		break;
	}
	default:
		php_printf("%sUNKNOWN:0\n", COMMON);
		break;
	}
}
/* }}} */

/* {{{ proto void var_dump(mixed var)
   Dumps a string representation of variable to output */
PHP_FUNCTION(var_dump)
{
	zval *args;
	int argc;
	int	i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &argc) == FAILURE) {
		return;
	}

	for (i = 0; i < argc; i++) {
		php_var_dump(&args[i], 1 TSRMLS_CC);
	}
	efree(args);
}
/* }}} */

static int zval_array_element_dump(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;

	level = va_arg(args, int);

	if (hash_key->key == NULL) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		/* XXX: perphaps when we are inside the class we should permit access to
		 * private & protected values
		 */
		if (va_arg(args, int) && hash_key->key->val[0] == '\0') {
			return 0;
		}
		php_printf("%*c[\"", level + 1, ' ');
		PHPWRITE(hash_key->key->val, hash_key->key->len);
		php_printf("\"]=>\n");
	}
	php_debug_zval_dump(zv, level + 2 TSRMLS_CC);
	return 0;
}
/* }}} */

static int zval_object_property_dump(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	const char *prop_name, *class_name;

	level = va_arg(args, int);

	if (hash_key->key == NULL) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		zend_unmangle_property_name(hash_key->key->val, hash_key->key->len, &class_name, &prop_name);
		php_printf("%*c[", level + 1, ' ');

		if (class_name) {
			if (class_name[0] == '*') {
				php_printf("\"%s\":protected", prop_name);
			} else {
				php_printf("\"%s\":\"%s\":private", prop_name, class_name);
			}
		} else {
			php_printf("\"%s\"", prop_name);
		}
		ZEND_PUTS("]=>\n");
	}
	php_debug_zval_dump(zv, level + 2 TSRMLS_CC);
	return 0;
}
/* }}} */

PHPAPI void php_debug_zval_dump(zval *struc, int level TSRMLS_DC) /* {{{ */
{
	HashTable *myht = NULL;
	zend_string *class_name;
	int (*zval_element_dump_func)(zval* TSRMLS_DC, int, va_list, zend_hash_key*);
	int is_temp = 0;
	int is_ref = 0;

	if (level > 1) {
		php_printf("%*c", level - 1, ' ');
	}

	if (Z_TYPE_P(struc) == IS_REFERENCE) {
		is_ref = 1;
		struc = Z_REFVAL_P(struc);
	}

	switch (Z_TYPE_P(struc)) {
	case IS_BOOL:
		php_printf("%sbool(%s)\n", COMMON, Z_LVAL_P(struc)?"true":"false");
		break;
	case IS_NULL:
		php_printf("%sNULL\n", COMMON);
		break;
	case IS_LONG:
		php_printf("%slong(%ld)\n", COMMON, Z_LVAL_P(struc));
		break;
	case IS_DOUBLE:
		php_printf("%sdouble(%.*G)\n", COMMON, (int) EG(precision), Z_DVAL_P(struc));
		break;
	case IS_STRING:
		php_printf("%sstring(%d) \"", COMMON, Z_STRLEN_P(struc));
		PHPWRITE(Z_STRVAL_P(struc), Z_STRLEN_P(struc));
		php_printf("\" refcount(%u)\n", IS_INTERNED(Z_STR_P(struc)) ? 1 : Z_REFCOUNT_P(struc));
		break;
	case IS_ARRAY:
		myht = Z_ARRVAL_P(struc);
		if (myht->nApplyCount > 1) {
			PUTS("*RECURSION*\n");
			return;
		}
		php_printf("%sarray(%d) refcount(%u){\n", COMMON, zend_hash_num_elements(myht), Z_REFCOUNT_P(struc));
		zval_element_dump_func = zval_array_element_dump;
		goto head_done;
	case IS_OBJECT:
		myht = Z_OBJDEBUG_P(struc, is_temp);
		if (myht && myht->nApplyCount > 1) {
			PUTS("*RECURSION*\n");
			return;
		}
		class_name = Z_OBJ_HANDLER_P(struc, get_class_name)(struc, 0 TSRMLS_CC);
		php_printf("%sobject(%s)#%d (%d) refcount(%u){\n", COMMON, class_name->val, Z_OBJ_HANDLE_P(struc), myht ? zend_hash_num_elements(myht) : 0, Z_REFCOUNT_P(struc));
		STR_RELEASE(class_name);
		zval_element_dump_func = zval_object_property_dump;
head_done:
		if (myht) {
			zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) zval_element_dump_func, 1, level, (Z_TYPE_P(struc) == IS_ARRAY ? 0 : 1));
			if (is_temp) {
				zend_hash_destroy(myht);
				efree(myht);
			}
		}
		if (level > 1) {
			php_printf("%*c", level - 1, ' ');
		}
		PUTS("}\n");
		break;
	case IS_RESOURCE: {
		const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(struc) TSRMLS_CC);
		php_printf("%sresource(%ld) of type (%s) refcount(%u)\n", COMMON, Z_RES_P(struc)->handle, type_name ? type_name : "Unknown", Z_REFCOUNT_P(struc));
		break;
	}
	default:
		php_printf("%sUNKNOWN:0\n", COMMON);
		break;
	}
}
/* }}} */

/* {{{ proto void debug_zval_dump(mixed var)
   Dumps a string representation of an internal zend value to output. */
PHP_FUNCTION(debug_zval_dump)
{
	zval *args;
	int argc;
	int	i;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &argc) == FAILURE) {
		return;
	}

	for (i = 0; i < argc; i++) {
		php_debug_zval_dump(&args[i], 1 TSRMLS_CC);
	}
	efree(args);
}
/* }}} */

#define buffer_append_spaces(buf, num_spaces) \
	do { \
		char *tmp_spaces; \
		int tmp_spaces_len; \
		tmp_spaces_len = spprintf(&tmp_spaces, 0,"%*c", num_spaces, ' '); \
		smart_str_appendl(buf, tmp_spaces, tmp_spaces_len); \
		efree(tmp_spaces); \
	} while(0);

static int php_array_element_export(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	smart_str *buf;

	level = va_arg(args, int);
	buf = va_arg(args, smart_str *);

	if (hash_key->key == NULL) { /* numeric key */
		buffer_append_spaces(buf, level+1);
		smart_str_append_long(buf, (long) hash_key->h);
		smart_str_appendl(buf, " => ", 4);

	} else { /* string key */
		char *tmp_str;
		int tmp_len;
		zend_string *key = php_addcslashes(hash_key->key->val, hash_key->key->len, 0, "'\\", 2 TSRMLS_CC);
		tmp_str = php_str_to_str_ex(key->val, key->len, "\0", 1, "' . \"\\0\" . '", 12, &tmp_len, 0, NULL);

		buffer_append_spaces(buf, level + 1);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, tmp_str, tmp_len);
		smart_str_appendl(buf, "' => ", 5);

		STR_RELEASE(key);
		efree(tmp_str);
	}
	php_var_export_ex(zv, level + 2, buf TSRMLS_CC);

	smart_str_appendc(buf, ',');
	smart_str_appendc(buf, '\n');
	
	return 0;
}
/* }}} */

static int php_object_element_export(zval *zv TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key) /* {{{ */
{
	int level;
	smart_str *buf;

	level = va_arg(args, int);
	buf = va_arg(args, smart_str *);

	buffer_append_spaces(buf, level + 2);
	if (hash_key->key != NULL) {
		const char *class_name; /* ignored, but must be passed to unmangle */
		const char *pname;
		zend_string *pname_esc;
		
		zend_unmangle_property_name(hash_key->key->val, hash_key->key->len,
				&class_name, &pname);
		pname_esc = php_addcslashes(pname, strlen(pname), 0, "'\\", 2 TSRMLS_CC);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, pname_esc->val, pname_esc->len);
		smart_str_appendc(buf, '\'');
		STR_RELEASE(pname_esc);
	} else {
		smart_str_append_long(buf, (long) hash_key->h);
	}
	smart_str_appendl(buf, " => ", 4);
	php_var_export_ex(zv, level + 2, buf TSRMLS_CC);
	smart_str_appendc(buf, ',');
	smart_str_appendc(buf, '\n');
	return 0;
}
/* }}} */

PHPAPI void php_var_export_ex(zval *struc, int level, smart_str *buf TSRMLS_DC) /* {{{ */
{
	HashTable *myht;
	char *tmp_str;
	int tmp_len;
	zend_string *class_name;
	zend_string *ztmp;

	switch (Z_TYPE_P(struc)) {
	case IS_BOOL:
		if (Z_LVAL_P(struc)) {
			smart_str_appendl(buf, "true", 4);
		} else {
			smart_str_appendl(buf, "false", 5);
		}
		break;
	case IS_NULL:
		smart_str_appendl(buf, "NULL", 4);
		break;
	case IS_LONG:
		smart_str_append_long(buf, Z_LVAL_P(struc));
		break;
	case IS_DOUBLE:
		tmp_len = spprintf(&tmp_str, 0,"%.*H", PG(serialize_precision), Z_DVAL_P(struc));
		smart_str_appendl(buf, tmp_str, tmp_len);
		efree(tmp_str);
		break;
	case IS_STRING:
		ztmp = php_addcslashes(Z_STRVAL_P(struc), Z_STRLEN_P(struc), 0, "'\\", 2 TSRMLS_CC);
		tmp_str = php_str_to_str_ex(ztmp->val, ztmp->len, "\0", 1, "' . \"\\0\" . '", 12, &tmp_len, 0, NULL);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, tmp_str, tmp_len);
		smart_str_appendc(buf, '\'');

		STR_RELEASE(ztmp);
		efree(tmp_str);
		break;
	case IS_ARRAY:
		myht = Z_ARRVAL_P(struc);
		if (myht->nApplyCount > 0){
			smart_str_appendl(buf, "NULL", 4);
			zend_error(E_WARNING, "var_export does not handle circular references");
			return;
		}
		if (level > 1) {
			smart_str_appendc(buf, '\n');
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendl(buf, "array (\n", 8);
		zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) php_array_element_export, 2, level, buf);

		if (level > 1) {
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendc(buf, ')');
    
		break;

	case IS_OBJECT:
		myht = Z_OBJPROP_P(struc);
		if(myht && myht->nApplyCount > 0){
			smart_str_appendl(buf, "NULL", 4);
			zend_error(E_WARNING, "var_export does not handle circular references");
			return;
		}
		if (level > 1) {
			smart_str_appendc(buf, '\n');
			buffer_append_spaces(buf, level - 1);
		}
		class_name = Z_OBJ_HANDLER_P(struc, get_class_name)(struc, 0 TSRMLS_CC);

		smart_str_appendl(buf, class_name->val, class_name->len);
		smart_str_appendl(buf, "::__set_state(array(\n", 21);

		STR_RELEASE(class_name);
		if (myht) {
			zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) php_object_element_export, 1, level, buf);
		}
		if (level > 1) {
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendl(buf, "))", 2);

		break;
	default:
		smart_str_appendl(buf, "NULL", 4);
		break;
	}
}
/* }}} */

/* FOR BC reasons, this will always perform and then print */
PHPAPI void php_var_export(zval *struc, int level TSRMLS_DC) /* {{{ */
{
	smart_str buf = {0};
	php_var_export_ex(struc, level, &buf TSRMLS_CC);
	smart_str_0(&buf);
	PHPWRITE(buf.s->val, buf.s->len);
	smart_str_free(&buf);
}
/* }}} */


/* {{{ proto mixed var_export(mixed var [, bool return])
   Outputs or returns a string representation of a variable */
PHP_FUNCTION(var_export)
{
	zval *var;
	zend_bool return_output = 0;
	smart_str buf = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &var, &return_output) == FAILURE) {
		return;
	}

	php_var_export_ex(var, 1, &buf TSRMLS_CC);
	smart_str_0 (&buf);

	if (return_output) {
		RETURN_STR(buf.s);
	} else {
		PHPWRITE(buf.s->val, buf.s->len);
		smart_str_free(&buf);
	}
}
/* }}} */

static void php_var_serialize_intern(smart_str *buf, zval *struc, HashTable *var_hash TSRMLS_DC);

static inline int php_add_var_hash(HashTable *var_hash, zval *var, zval *var_old TSRMLS_DC) /* {{{ */
{
	zval var_no, *zv;
	char id[32], *p;
	register int len;

	if ((Z_TYPE_P(var) == IS_OBJECT) && Z_OBJ_HT_P(var)->get_class_entry) {
		p = smart_str_print_long(id + sizeof(id) - 1,
				(long) Z_OBJ_P(var));
		*(--p) = 'O';
		len = id + sizeof(id) - 1 - p;
	} else {
		p = smart_str_print_long(id + sizeof(id) - 1, (long) var);
		len = id + sizeof(id) - 1 - p;
	}

	if ((zv = zend_hash_str_find(var_hash, p, len)) != NULL) {
		ZVAL_COPY_VALUE(var_old, zv);
		if (!Z_ISREF_P(var)) {
			/* we still need to bump up the counter, since non-refs will
			 * be counted separately by unserializer */
			ZVAL_LONG(&var_no, -1);
			zend_hash_next_index_insert(var_hash, &var_no);
		}
#if 0
		fprintf(stderr, "- had var (%d): %lu\n", Z_TYPE_P(var), **(ulong**)var_old);
#endif
		return FAILURE;
	}

	/* +1 because otherwise hash will think we are trying to store NULL pointer */
	ZVAL_LONG(&var_no, zend_hash_num_elements(var_hash) + 1);
	zend_hash_str_add(var_hash, p, len, &var_no);
#if 0
	fprintf(stderr, "+ add var (%d): %lu\n", Z_TYPE_P(var), Z_LVAL(var_no));
#endif
	return SUCCESS;
}
/* }}} */

static inline void php_var_serialize_long(smart_str *buf, long val) /* {{{ */
{
	smart_str_appendl(buf, "i:", 2);
	smart_str_append_long(buf, val);
	smart_str_appendc(buf, ';');
}
/* }}} */

static inline void php_var_serialize_string(smart_str *buf, char *str, int len) /* {{{ */
{
	smart_str_appendl(buf, "s:", 2);
	smart_str_append_long(buf, len);
	smart_str_appendl(buf, ":\"", 2);
	smart_str_appendl(buf, str, len);
	smart_str_appendl(buf, "\";", 2);
}
/* }}} */

static inline zend_bool php_var_serialize_class_name(smart_str *buf, zval *struc TSRMLS_DC) /* {{{ */
{
	PHP_CLASS_ATTRIBUTES;

	PHP_SET_CLASS_ATTRIBUTES(struc);
	smart_str_appendl(buf, "O:", 2);
	smart_str_append_long(buf, (int)class_name->len);
	smart_str_appendl(buf, ":\"", 2);
	smart_str_appendl(buf, class_name->val, class_name->len);
	smart_str_appendl(buf, "\":", 2);
	PHP_CLEANUP_CLASS_ATTRIBUTES();
	return incomplete_class;
}
/* }}} */

static void php_var_serialize_class(smart_str *buf, zval *struc, zval *retval_ptr, HashTable *var_hash TSRMLS_DC) /* {{{ */
{
	int count;
	zend_bool incomplete_class;

	incomplete_class = php_var_serialize_class_name(buf, struc TSRMLS_CC);
	/* count after serializing name, since php_var_serialize_class_name
	 * changes the count if the variable is incomplete class */
	count = zend_hash_num_elements(HASH_OF(retval_ptr));
	if (incomplete_class) {
		--count;
	}
	smart_str_append_long(buf, count);
	smart_str_appendl(buf, ":{", 2);

	if (count > 0) {
		zend_string *key;
		zval *d, *name;
		ulong index;
		HashPosition pos;
		int i;
		zval nval, *nvalp;
		HashTable *propers;

		ZVAL_NULL(&nval);
		nvalp = &nval;

		zend_hash_internal_pointer_reset_ex(HASH_OF(retval_ptr), &pos);

		for (;; zend_hash_move_forward_ex(HASH_OF(retval_ptr), &pos)) {
			i = zend_hash_get_current_key_ex(HASH_OF(retval_ptr), &key, &index, 0, &pos);

			if (i == HASH_KEY_NON_EXISTENT) {
				break;
			}

			if (incomplete_class && strcmp(key->val, MAGIC_MEMBER) == 0) {
				continue;
			}
			name = zend_hash_get_current_data_ex(HASH_OF(retval_ptr), &pos);

			if (Z_TYPE_P(name) != IS_STRING) {
				php_error_docref(NULL TSRMLS_CC, E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize.");
				/* we should still add element even if it's not OK,
				 * since we already wrote the length of the array before */
				smart_str_appendl(buf,"N;", 2);
				continue;
			}
			propers = Z_OBJPROP_P(struc);
			if ((d = zend_hash_find(propers, Z_STR_P(name))) != NULL) {
				php_var_serialize_string(buf, Z_STRVAL_P(name), Z_STRLEN_P(name));
				php_var_serialize_intern(buf, d, var_hash TSRMLS_CC);
			} else {
				zend_class_entry *ce;
				ce = zend_get_class_entry(struc TSRMLS_CC);
				if (ce) {
					zend_string *prot_name, *priv_name;

					do {
						priv_name = zend_mangle_property_name(ce->name->val, ce->name->len, Z_STRVAL_P(name), Z_STRLEN_P(name), ce->type & ZEND_INTERNAL_CLASS);
						if ((d = zend_hash_find(propers, priv_name)) != NULL) {
							php_var_serialize_string(buf, priv_name->val, priv_name->len);
							STR_FREE(priv_name);
							php_var_serialize_intern(buf, d, var_hash TSRMLS_CC);
							break;
						}
						STR_FREE(priv_name);
						prot_name = zend_mangle_property_name("*", 1, Z_STRVAL_P(name), Z_STRLEN_P(name), ce->type & ZEND_INTERNAL_CLASS);
						if ((d = zend_hash_find(propers, prot_name)) != NULL) {
							php_var_serialize_string(buf, prot_name->val, prot_name->len);
							STR_FREE(prot_name);
							php_var_serialize_intern(buf, d, var_hash TSRMLS_CC);
							break;
						}
						STR_FREE(prot_name);
						php_var_serialize_string(buf, Z_STRVAL_P(name), Z_STRLEN_P(name));
						php_var_serialize_intern(buf, nvalp, var_hash TSRMLS_CC);
						php_error_docref(NULL TSRMLS_CC, E_NOTICE, "\"%s\" returned as member variable from __sleep() but does not exist", Z_STRVAL_P(name));
					} while (0);
				} else {
					php_var_serialize_string(buf, Z_STRVAL_P(name), Z_STRLEN_P(name));
					php_var_serialize_intern(buf, nvalp, var_hash TSRMLS_CC);
				}
			}
		}
	}
	smart_str_appendc(buf, '}');
}
/* }}} */

static void php_var_serialize_intern(smart_str *buf, zval *struc, HashTable *var_hash TSRMLS_DC) /* {{{ */
{
	int i;
	zval var_already;
	HashTable *myht;

	if (EG(exception)) {
		return;
	}

	ZVAL_UNDEF(&var_already);
	if (var_hash && php_add_var_hash(var_hash, struc, &var_already TSRMLS_CC) == FAILURE) {
		if (Z_ISREF_P(struc)) {
			smart_str_appendl(buf, "R:", 2);
			smart_str_append_long(buf, Z_LVAL(var_already));
			smart_str_appendc(buf, ';');
			return;
		} else if (Z_TYPE_P(struc) == IS_OBJECT) {
			smart_str_appendl(buf, "r:", 2);
			smart_str_append_long(buf, Z_LVAL(var_already));
			smart_str_appendc(buf, ';');
			return;
		}
	}

	switch (Z_TYPE_P(struc)) {
		case IS_BOOL:
			smart_str_appendl(buf, "b:", 2);
			smart_str_append_long(buf, Z_LVAL_P(struc));
			smart_str_appendc(buf, ';');
			return;

		case IS_NULL:
			smart_str_appendl(buf, "N;", 2);
			return;

		case IS_LONG:
			php_var_serialize_long(buf, Z_LVAL_P(struc));
			return;

		case IS_DOUBLE: {
				char *s;

				smart_str_appendl(buf, "d:", 2);
				s = (char *) safe_emalloc(PG(serialize_precision), 1, MAX_LENGTH_OF_DOUBLE + 1);
				php_gcvt(Z_DVAL_P(struc), PG(serialize_precision), '.', 'E', s);
				smart_str_appends(buf, s);
				smart_str_appendc(buf, ';');
				efree(s);
				return;
			}

		case IS_STRING:
			php_var_serialize_string(buf, Z_STRVAL_P(struc), Z_STRLEN_P(struc));
			return;

		case IS_OBJECT: {
				zval retval;
				zval fname;
				int res;
				zend_class_entry *ce = NULL;

				if (Z_OBJ_HT_P(struc)->get_class_entry) {
					ce = Z_OBJCE_P(struc);
				}

				if (ce && ce->serialize != NULL) {
					/* has custom handler */
					unsigned char *serialized_data = NULL;
					zend_uint serialized_length;

					if (ce->serialize(struc, &serialized_data, &serialized_length, (zend_serialize_data *)var_hash TSRMLS_CC) == SUCCESS) {
						smart_str_appendl(buf, "C:", 2);
						smart_str_append_long(buf, (int)Z_OBJCE_P(struc)->name->len);
						smart_str_appendl(buf, ":\"", 2);
						smart_str_appendl(buf, Z_OBJCE_P(struc)->name->val, Z_OBJCE_P(struc)->name->len);
						smart_str_appendl(buf, "\":", 2);

						smart_str_append_long(buf, (int)serialized_length);
						smart_str_appendl(buf, ":{", 2);
						smart_str_appendl(buf, serialized_data, serialized_length);
						smart_str_appendc(buf, '}');
					} else {
						smart_str_appendl(buf, "N;", 2);
					}
					if (serialized_data) {
						efree(serialized_data);
					}
					return;
				}

				if (ce && ce != PHP_IC_ENTRY && zend_hash_str_exists(&ce->function_table, "__sleep", sizeof("__sleep")-1)) {
//???					ZVAL_STRINGL(&fname, "__sleep", sizeof("__sleep") - 1, 0);
					ZVAL_STRINGL(&fname, "__sleep", sizeof("__sleep") - 1);
					BG(serialize_lock)++;
					res = call_user_function_ex(CG(function_table), struc, &fname, &retval, 0, 0, 1, NULL TSRMLS_CC);
					BG(serialize_lock)--;
                    
					if (EG(exception)) {
						zval_ptr_dtor(&retval);
						return;
					}

					if (res == SUCCESS) {
						if (Z_TYPE(retval) != IS_UNDEF) {
							if (HASH_OF(&retval)) {
								php_var_serialize_class(buf, struc, &retval, var_hash TSRMLS_CC);
							} else {
								php_error_docref(NULL TSRMLS_CC, E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize");
								/* we should still add element even if it's not OK,
								 * since we already wrote the length of the array before */
								smart_str_appendl(buf,"N;", 2);
							}
							zval_ptr_dtor(&retval);
						}
						return;
					}
				}

				zval_ptr_dtor(&retval);
				/* fall-through */
			}
		case IS_ARRAY: {
			zend_bool incomplete_class = 0;
			if (Z_TYPE_P(struc) == IS_ARRAY) {
				smart_str_appendl(buf, "a:", 2);
				myht = HASH_OF(struc);
			} else {
				incomplete_class = php_var_serialize_class_name(buf, struc TSRMLS_CC);
				myht = Z_OBJPROP_P(struc);
			}
			/* count after serializing name, since php_var_serialize_class_name
			 * changes the count if the variable is incomplete class */
			i = myht ? zend_hash_num_elements(myht) : 0;
			if (i > 0 && incomplete_class) {
				--i;
			}
			smart_str_append_long(buf, i);
			smart_str_appendl(buf, ":{", 2);
			if (i > 0) {
				zend_string *key;
				zval *data;
				ulong index;
				HashPosition pos;

				zend_hash_internal_pointer_reset_ex(myht, &pos);
				for (;; zend_hash_move_forward_ex(myht, &pos)) {
					i = zend_hash_get_current_key_ex(myht, &key, &index, 0, &pos);
					if (i == HASH_KEY_NON_EXISTENT) {
						break;
					}
					if (incomplete_class && strcmp(key->val, MAGIC_MEMBER) == 0) {
						continue;
					}

					switch (i) {
						case HASH_KEY_IS_LONG:
							php_var_serialize_long(buf, index);
							break;
						case HASH_KEY_IS_STRING:
							php_var_serialize_string(buf, key->val, key->len);
							break;
					}

					/* we should still add element even if it's not OK,
					 * since we already wrote the length of the array before */
					if ((data = zend_hash_get_current_data_ex(myht, &pos)) == NULL
						|| Z_ARR_P(data) == Z_ARR_P(struc)
						|| (Z_TYPE_P(data) == IS_ARRAY && Z_ARRVAL_P(data)->nApplyCount > 1)
					) {
						smart_str_appendl(buf, "N;", 2);
					} else {
						if (Z_TYPE_P(data) == IS_ARRAY) {
							Z_ARRVAL_P(data)->nApplyCount++;
						}
						php_var_serialize_intern(buf, data, var_hash TSRMLS_CC);
						if (Z_TYPE_P(data) == IS_ARRAY) {
							Z_ARRVAL_P(data)->nApplyCount--;
						}
					}
				}
			}
			smart_str_appendc(buf, '}');
			return;
		}
		default:
			smart_str_appendl(buf, "i:0;", 4);
			return;
	}
}
/* }}} */

PHPAPI void php_var_serialize(smart_str *buf, zval *struc, php_serialize_data_t *var_hash TSRMLS_DC) /* {{{ */
{
	php_var_serialize_intern(buf, struc, *var_hash TSRMLS_CC);
	smart_str_0(buf);
}
/* }}} */

/* {{{ proto string serialize(mixed variable)
   Returns a string representation of variable (which can later be unserialized) */
PHP_FUNCTION(serialize)
{
	zval *struc;
	php_serialize_data_t var_hash;
	smart_str buf = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &struc) == FAILURE) {
		return;
	}

	PHP_VAR_SERIALIZE_INIT(var_hash);
	php_var_serialize(&buf, struc, &var_hash TSRMLS_CC);
	PHP_VAR_SERIALIZE_DESTROY(var_hash);

	if (EG(exception)) {
		smart_str_free(&buf);
		RETURN_FALSE;
	}

	if (buf.s) {
		RETURN_STR(buf.s);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto mixed unserialize(string variable_representation[, int &consumed])
   Takes a string representation of variable and recreates it */
PHP_FUNCTION(unserialize)
{
	char *buf = NULL;
	int buf_len;
	const unsigned char *p;
	php_unserialize_data_t var_hash;
	zval *consumed = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &buf, &buf_len, &consumed) == FAILURE) {
		RETURN_FALSE;
	}

	if (buf_len == 0) {
		RETURN_FALSE;
	}

	p = (const unsigned char*) buf;
	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	if (!php_var_unserialize(return_value, &p, p + buf_len, &var_hash TSRMLS_CC)) {
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
		zval_dtor(return_value);
		if (!EG(exception)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Error at offset %ld of %d bytes", (long)((char*)p - buf), buf_len);
		}
		RETURN_FALSE;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

	if (consumed) {
		zval_dtor(consumed);
		ZVAL_LONG(consumed, ((char*)p) - buf);
	}
}
/* }}} */

/* {{{ proto int memory_get_usage([real_usage])
   Returns the allocated by PHP memory */
PHP_FUNCTION(memory_get_usage) {
	zend_bool real_usage = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &real_usage) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_LONG(zend_memory_usage(real_usage TSRMLS_CC));
}
/* }}} */

/* {{{ proto int memory_get_peak_usage([real_usage])
   Returns the peak allocated by PHP memory */
PHP_FUNCTION(memory_get_peak_usage) {
	zend_bool real_usage = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &real_usage) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_LONG(zend_memory_peak_usage(real_usage TSRMLS_CC));
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
