/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jani Lehtim�ki <jkl@njet.net>                               |
   |          Thies C. Arntzen <thies@digicol.de>                         |
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
#include "basic_functions.h"

#define COMMON ((*struc)->is_ref?"&":"")

/* }}} */
/* {{{ php_var_dump */

static int php_array_element_dump(zval **zv, int num_args, va_list args, zend_hash_key *hash_key)
{
	int level;

	level = va_arg(args, int);

	if (hash_key->nKeyLength==0) { /* numeric key */
		php_printf("%*c[%ld]=>\n", level + 1, ' ', hash_key->h);
	} else { /* string key */
		php_printf("%*c[\"%s\"]=>\n", level + 1, ' ', hash_key->arKey);
	}
	php_var_dump(zv, level + 2);
	return 0;
}

static char *php_lookup_class_name(zval **object, size_t *nlen, zend_bool del);

#define INCOMPLETE_CLASS_MSG \
		"The script tried to execute a method or "  \
		"access a property of an incomplete object. " \
		"Please ensure that the class definition <b>%s</b> of the object " \
		"you are trying to operate on was loaded _before_ " \
		"the session was started"

static void incomplete_class_message(zend_property_reference *ref)
{
	char buf[1024];
	char *class_name;

	class_name = php_lookup_class_name(&ref->object, NULL, 0);
	
	if (!class_name)
		class_name = estrdup("unknown");
	
	snprintf(buf, 1023, INCOMPLETE_CLASS_MSG, class_name);
	
	efree(class_name);

	php_error(E_ERROR, buf);
}

static void incomplete_class_call_func(INTERNAL_FUNCTION_PARAMETERS, zend_property_reference *property_reference)
{
	incomplete_class_message(property_reference);
}

static int incomplete_class_set_property(zend_property_reference *property_reference, zval *value)
{
	incomplete_class_message(property_reference);
	
	/* does not reach this point */
	return (0);
}

static zval incomplete_class_get_property(zend_property_reference *property_reference)
{
	zval foo;
	
	incomplete_class_message(property_reference);

	/* does not reach this point */
	return (foo);
}

#define INCOMPLETE_CLASS "__PHP_Incomplete_Class"

static void php_create_incomplete_class(BLS_D)
{
	zend_class_entry incomplete_class;

	INIT_OVERLOADED_CLASS_ENTRY(incomplete_class, INCOMPLETE_CLASS, NULL,
			incomplete_class_call_func,
			incomplete_class_get_property,
			incomplete_class_set_property);

	BG(incomplete_class) = zend_register_internal_class(&incomplete_class);
}

#define MAGIC_MEMBER "__PHP_Incomplete_Class_Name"

static char *php_lookup_class_name(zval **object, size_t *nlen, zend_bool del)
{
	zval **val;
	char *retval = NULL;

	if (zend_hash_find((*object)->value.obj.properties, MAGIC_MEMBER, sizeof(MAGIC_MEMBER), (void **) &val) == SUCCESS) {
		retval = estrndup(Z_STRVAL_PP(val), Z_STRLEN_PP(val));

		if (nlen)
			*nlen = Z_STRLEN_PP(val);

		if (del)
			zend_hash_del((*object)->value.obj.properties, MAGIC_MEMBER, sizeof(MAGIC_MEMBER));
	}

	return (retval);
}

static void php_store_class_name(zval **object, const char *name, size_t len)
{
	zval *val;

	MAKE_STD_ZVAL(val);

	Z_TYPE_P(val)   = IS_STRING;
	Z_STRVAL_P(val) = estrndup(name, len);
	Z_STRLEN_P(val) = len;

	zend_hash_update((*object)->value.obj.properties, MAGIC_MEMBER, sizeof(MAGIC_MEMBER), &val, sizeof(val), NULL);
}

void php_var_dump(pval **struc, int level)
{
	HashTable *myht;

	if (level>1) {
		php_printf("%*c", level-1, ' ');
	}

	switch ((*struc)->type) {
		case IS_BOOL:
			php_printf("%sbool(%s)\n", COMMON, ((*struc)->value.lval?"true":"false"));
			break;
		case IS_NULL:
			php_printf("%sNULL\n", COMMON);
			break;
		case IS_LONG:
			php_printf("%sint(%ld)\n", COMMON, (*struc)->value.lval);
			break;
		case IS_DOUBLE: {
				ELS_FETCH();

				php_printf("%sfloat(%.*G)\n", COMMON, (int) EG(precision), (*struc)->value.dval);
			}
			break;
		case IS_STRING:
			php_printf("%sstring(%d) \"", COMMON, (*struc)->value.str.len);
			PHPWRITE((*struc)->value.str.val, (*struc)->value.str.len);
			PUTS("\"\n");
			break;
		case IS_ARRAY:
			myht = HASH_OF(*struc);
			php_printf("%sarray(%d) {\n", COMMON, zend_hash_num_elements(myht));
			goto head_done;
		case IS_OBJECT:
			myht = HASH_OF(*struc);
			php_printf("%sobject(%s)(%d) {\n", COMMON, (*struc)->value.obj.ce->name, zend_hash_num_elements(myht));
head_done:
			zend_hash_apply_with_arguments(myht, (ZEND_STD_HASH_APPLIER) php_array_element_dump, 1, level);
			if (level>1) {
				php_printf("%*c\n", level-1, ' ');
			}
			PUTS("}\n");
			break;	
		case IS_RESOURCE: {
			int type;
			zend_list_find((*struc)->value.lval, &type);
			php_printf("%sresource(%ld) of type %d\n", COMMON, (*struc)->value.lval, type);
			break;
		}
		default:
			php_printf("%sUNKNOWN:0\n",COMMON);
			break;
	}
}

/* }}} */


/* {{{ proto void var_dump(mixed var)
   Dumps a string representation of variable to output */
PHP_FUNCTION(var_dump)
{
	zval ***args;
	int argc;
	int	i;
	
	argc = ZEND_NUM_ARGS();
	
	args = (zval ***)emalloc(argc * sizeof(zval **));
	if (ZEND_NUM_ARGS() == 0 || zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}
	
	for (i=0; i<argc; i++)
		php_var_dump(args[i], 1);
	
	efree(args);
}
/* }}} */


/* {{{ php_var_dump */


#define STR_CAT(P,S,I) {\
	pval *__p = (P);\
	ulong __i = __p->value.str.len;\
	__p->value.str.len += (I);\
	if (__p->value.str.val) {\
		__p->value.str.val = (char *)erealloc(__p->value.str.val, __p->value.str.len + 1);\
	} else {\
		__p->value.str.val = emalloc(__p->value.str.len + 1);\
		*__p->value.str.val = 0;\
	}\
	strcat(__p->value.str.val + __i, (S));\
}

#define PHP_SET_CLASS_ATTRIBUTES() 				\
	if ((*struc)->value.obj.ce == BG(incomplete_class)) {				\
		class_name = php_lookup_class_name(struc, &name_len, 1);		\
		free_class_name = 1;											\
	} else {															\
		class_name = (*struc)->value.obj.ce->name;						\
		name_len   = (*struc)->value.obj.ce->name_length;				\
	}

#define PHP_CLEANUP_CLASS_ATTRIBUTES()									\
	if (free_class_name) efree(class_name)

#define PHP_CLASS_ATTRIBUTES											\
	char *class_name;													\
	size_t name_len;													\
	zend_bool free_class_name = 0										\


/* }}} */
/* {{{ php_var_serialize */

void php_var_serialize(pval *buf, pval **struc)
{
	char s[256];
	ulong slen;
	int i;
	HashTable *myht;
	BLS_FETCH();

	switch ((*struc)->type) {
		case IS_BOOL:
			slen = sprintf(s, "b:%ld;", (*struc)->value.lval);
			STR_CAT(buf, s, slen);
			return;

		case IS_NULL:
			STR_CAT(buf, "N;", 2);
			return;

		case IS_LONG:
			slen = sprintf(s, "i:%ld;", (*struc)->value.lval);
			STR_CAT(buf, s, slen);
			return;

		case IS_DOUBLE: {
				ELS_FETCH();
				slen = sprintf(s, "d:%.*G;",(int) EG(precision), (*struc)->value.dval);
				STR_CAT(buf, s, slen);
			}
			return;

		case IS_STRING:{
				char *p;
				
				i = buf->value.str.len;
				slen = sprintf(s, "s:%d:\"", (*struc)->value.str.len);
				STR_CAT(buf, s, slen + (*struc)->value.str.len + 2);
				p = buf->value.str.val + i + slen;
				if ((*struc)->value.str.len > 0) {
					memcpy(p, (*struc)->value.str.val, (*struc)->value.str.len);
					p += (*struc)->value.str.len;
				}
				*p++ = '\"';
				*p++ = ';';
				*p = 0;
			}
			return;

		case IS_OBJECT: {
				zval *retval_ptr = NULL;
				zval *fname;
				int res;
				PHP_CLASS_ATTRIBUTES;
				CLS_FETCH();

				MAKE_STD_ZVAL(fname);
				ZVAL_STRING(fname,"__sleep",1);

				res =  call_user_function_ex(CG(function_table), *struc, fname, &retval_ptr, 0, 0, 1, NULL);

				if (res == SUCCESS) {
					if (retval_ptr && HASH_OF(retval_ptr)) {
						int count = zend_hash_num_elements(HASH_OF(retval_ptr));
						
						PHP_SET_CLASS_ATTRIBUTES();
						slen = sprintf(s, "O:%d:\"%s\":%d:{",name_len,class_name, count);
						PHP_CLEANUP_CLASS_ATTRIBUTES();
						
						STR_CAT(buf, s, slen);
						if (count > 0) {
							char *key;
							zval **d,**name;
							ulong index;
							HashPosition pos;
							
							zend_hash_internal_pointer_reset_ex(HASH_OF(retval_ptr),&pos);
							for (;; zend_hash_move_forward_ex(HASH_OF(retval_ptr),&pos)) {
								if ((i = zend_hash_get_current_key_ex(HASH_OF(retval_ptr), &key, NULL, &index, &pos)) == HASH_KEY_NON_EXISTANT) {
									break;
								}

								zend_hash_get_current_data_ex(HASH_OF(retval_ptr), (void **) (&name), &pos);

								if ((*name)->type != IS_STRING) {
									php_error(E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize.");
									continue;
								}

								php_var_serialize(buf, name);
								
								if (zend_hash_find((*struc)->value.obj.properties,(*name)->value.str.val,(*name)->value.str.len+1,(void*)&d) == SUCCESS) {
									php_var_serialize(buf,d);	
								}
							}
						}
						STR_CAT(buf, "}", 1);
					}
				} else {
					zval_dtor(fname);
					FREE_ZVAL(fname);

					if (retval_ptr) {
						zval_ptr_dtor(&retval_ptr);
					}
					goto std_array;
				}

				zval_dtor(fname);
				FREE_ZVAL(fname);

				if (retval_ptr) {
					zval_ptr_dtor(&retval_ptr);
				}
				return;	
			}

		case IS_ARRAY:
		  std_array:
			myht = HASH_OF(*struc);
			i = zend_hash_num_elements(myht);
			if ((*struc)->type == IS_ARRAY) {
				slen = sprintf(s, "a:%d:{", i);
			} else {
				PHP_CLASS_ATTRIBUTES;

				PHP_SET_CLASS_ATTRIBUTES();
				slen = sprintf(s, "O:%d:\"%s\":%d:{",name_len,class_name,i);
				PHP_CLEANUP_CLASS_ATTRIBUTES();
			}
			STR_CAT(buf, s, slen);
			if (i > 0) {
				char *key;
				pval **data,*d;
				ulong index;
				HashPosition pos;
				
				zend_hash_internal_pointer_reset_ex(myht, &pos);
				for (;; zend_hash_move_forward_ex(myht, &pos)) {
					if ((i = zend_hash_get_current_key_ex(myht, &key, NULL, &index, &pos)) == HASH_KEY_NON_EXISTANT) {
						break;
					}
					if (zend_hash_get_current_data_ex(myht, (void **) (&data), &pos) != SUCCESS || !data || ((*data) == (*struc))) {
						if (i == HASH_KEY_IS_STRING)
							efree(key);
						continue;
					}

					switch (i) {
						case HASH_KEY_IS_LONG:
							MAKE_STD_ZVAL(d);	
							d->type = IS_LONG;
							d->value.lval = index;
							php_var_serialize(buf, &d);
							FREE_ZVAL(d);
							break;
						case HASH_KEY_IS_STRING:
							MAKE_STD_ZVAL(d);	
							d->type = IS_STRING;
							d->value.str.val = key;
							d->value.str.len = strlen(key);
							php_var_serialize(buf, &d);
							efree(key);
							FREE_ZVAL(d);
							break;
					}
					php_var_serialize(buf, data);
				}
			}
			STR_CAT(buf, "}", 1);
			return;

		default:
			STR_CAT(buf, "i:0;", 4);
			return;
	} 
}

/* }}} */
/* {{{ php_var_dump */

int php_var_unserialize(pval **rval, const char **p, const char *max)
{
	const char *q;
	char *str;
	int i;
	char cur;
	HashTable *myht;
	ELS_FETCH();
	BLS_FETCH();

	switch (cur = **p) {
		case 'N':
			if (*((*p) + 1) != ';') {
				return 0;
			}
			(*p)++;
			INIT_PZVAL(*rval);
			(*rval)->type = IS_NULL;
			(*p)++;
			return 1;

		case 'b': /* bool */
		case 'i':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			q = *p;
			while (**p && **p != ';') {
				(*p)++;
			}
			if (**p != ';') {
				return 0;
			}
			(*p)++;
			if (cur == 'b') {
				(*rval)->type = IS_BOOL;
			} else {
				(*rval)->type = IS_LONG;
			}
			INIT_PZVAL(*rval);
			(*rval)->value.lval = atol(q + 2);
			return 1;

		case 'd':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			q = *p;
			while (**p && **p != ';') {
				(*p)++;
			}
			if (**p != ';') {
				return 0;
			}
			(*p)++;
			(*rval)->type = IS_DOUBLE;
			INIT_PZVAL(*rval);
			(*rval)->value.dval = atof(q + 2);
			return 1;

		case 's':
			if (*((*p) + 1) != ':') {
				return 0;
			}
			(*p) += 2;
			q = *p;
			while (**p && **p != ':') {
				(*p)++;
			}
			if (**p != ':') {
				return 0;
			}
			i = atoi(q);
			if (i < 0 || (*p + 3 + i) > max || *((*p) + 1) != '\"' ||
				*((*p) + 2 + i) != '\"' || *((*p) + 3 + i) != ';') {
				return 0;
			}
			(*p) += 2;

			if (i == 0) {
			  	str = empty_string;
			} else  {
			  	str = estrndup(*p,i);
			}
			(*p) += i + 2;
			(*rval)->type = IS_STRING;
			(*rval)->value.str.val = str;
			(*rval)->value.str.len = i;
			INIT_PZVAL(*rval);
			return 1;

		case 'a':
		case 'o':
		case 'O': {
			zend_bool incomplete_class = 0;
			char *class_name = NULL;
			size_t name_len = 0;
			
			INIT_PZVAL(*rval);

			if (cur == 'a') {
				(*rval)->type = IS_ARRAY;
				ALLOC_HASHTABLE((*rval)->value.ht);
				myht = (*rval)->value.ht;
			} else {
				zend_class_entry *ce;

				if (cur == 'O') { /* php4 serialized - we get the class-name */
					if (*((*p) + 1) != ':') {
						return 0;
					}
					(*p) += 2;
					q = *p;
					while (**p && **p != ':') {
						(*p)++;
					}
					if (**p != ':') {
						return 0;
					}
					name_len = i = atoi(q);
					if (i < 0 || (*p + 3 + i) > max || *((*p) + 1) != '\"' ||
						*((*p) + 2 + i) != '\"' || *((*p) + 3 + i) != ':') {
						return 0;
					}
					(*p) += 2;
					class_name = emalloc(i + 1);
					if (i > 0) {
						memcpy(class_name, *p, i);
					}
					class_name[i] = 0;
					(*p) += i;
					
					if (zend_hash_find(EG(class_table), class_name, i+1, (void **) &ce)==FAILURE) {
						incomplete_class = 1;
						if (BG(incomplete_class) == NULL)
							php_create_incomplete_class(BLS_C);
						ce = BG(incomplete_class);
					}
				} else { /* old php 3.0 data 'o' */
					ce = &zend_standard_class_def;
				}

				object_init_ex(*rval,ce);
				myht = (*rval)->value.obj.properties;

				if (incomplete_class)
					php_store_class_name(rval, class_name, name_len);

				if (class_name)
					efree(class_name);
			}

			(*p) += 2;
			i = atoi(*p);

			if (cur == 'a') { /* object_init_ex will init the HashTable for objects! */
				zend_hash_init(myht, i + 1, NULL, ZVAL_PTR_DTOR, 0);
			}

			while (**p && **p != ':') {
				(*p)++;
			}
			if (**p != ':' || *((*p) + 1) != '{') {
				return 0;
			}
			for ((*p) += 2; **p && **p != '}' && i > 0; i--) {
				pval *key;
				pval *data;
				
				ALLOC_INIT_ZVAL(key);
				ALLOC_INIT_ZVAL(data);

				if (!php_var_unserialize(&key, p, max)) {
					zval_dtor(key);
					FREE_ZVAL(key);
					FREE_ZVAL(data);
					return 0;
				}
				if (!php_var_unserialize(&data, p, max)) {
					zval_dtor(key);
					FREE_ZVAL(key);
					zval_dtor(data);
					FREE_ZVAL(data);
					return 0;
				}
				switch (key->type) {
					case IS_LONG:
						zend_hash_index_update(myht, key->value.lval, &data, sizeof(data), NULL);
						break;
					case IS_STRING:
						zend_hash_update(myht, key->value.str.val, key->value.str.len + 1, &data, sizeof(data), NULL);
						break;
				}
				zval_dtor(key);
				FREE_ZVAL(key);
			}

			if ((*rval)->type == IS_OBJECT) {
				zval *retval_ptr = NULL;
				zval *fname;
				CLS_FETCH();

				MAKE_STD_ZVAL(fname);
				ZVAL_STRING(fname,"__wakeup",1);

				call_user_function_ex(CG(function_table), *rval, fname, &retval_ptr, 0, 0, 1, NULL);

				zval_dtor(fname);
				FREE_ZVAL(fname);
				if (retval_ptr)
					zval_ptr_dtor(&retval_ptr);
			}

			return *((*p)++) == '}';
		  }
	}

	return 0;
}

/* }}} */
/* {{{ proto string serialize(mixed variable)
   Returns a string representation of variable (which can later be unserialized) */
PHP_FUNCTION(serialize)
{
	pval **struc;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &struc) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	return_value->type = IS_STRING;
	return_value->value.str.val = NULL;
	return_value->value.str.len = 0;
	php_var_serialize(return_value, struc);
}

/* }}} */
/* {{{ proto mixed unserialize(string variable_representation)
   Takes a string representation of variable and recreates it */


PHP_FUNCTION(unserialize)
{
	pval **buf;
	
	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &buf) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if ((*buf)->type == IS_STRING) {
		const char *p = (*buf)->value.str.val;

		if ((*buf)->value.str.len == 0) {
			RETURN_FALSE;
		}

		if (!php_var_unserialize(&return_value, &p, p + (*buf)->value.str.len)) {
			zval_dtor(return_value);
			php_error(E_NOTICE, "unserialize() failed at offset %d of %d bytes",p-(*buf)->value.str.val,(*buf)->value.str.len);
			RETURN_FALSE;
		}
	} else {
		php_error(E_NOTICE, "argument passed to unserialize() is not an string");
		RETURN_FALSE;
	}
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
