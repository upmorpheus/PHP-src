/* Generated by re2c 0.5 on Sat May 24 19:11:12 2003 */
#line 1 "/usr/src/php5/ext/standard/var_unserializer.re"
/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2003 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Sascha Schumann <sascha@schumann.cx>                         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "ext/standard/php_var.h"
#include "php_incomplete_class.h"

/* {{{ reference-handling for unserializer: var_* */
#define VAR_ENTRIES_MAX 1024

typedef struct {
	zval *data[VAR_ENTRIES_MAX];
	int used_slots;
	void *next;
} var_entries;

static inline void var_push(php_unserialize_data_t *var_hashx, zval **rval)
{
	var_entries *var_hash = var_hashx->first, *prev = NULL;

	while (var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		prev = var_hash;
		var_hash = var_hash->next;
	}

	if (!var_hash) {
		var_hash = emalloc(sizeof(var_entries));
		var_hash->used_slots = 0;
		var_hash->next = 0;

		if (!var_hashx->first)
			var_hashx->first = var_hash;
		else
			prev->next = var_hash;
	}

	var_hash->data[var_hash->used_slots++] = *rval;
}

PHPAPI void var_replace(php_unserialize_data_t *var_hashx, zval *ozval, zval **nzval)
{
	int i;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			if (var_hash->data[i] == ozval) {
				var_hash->data[i] = *nzval;
				return;
			}
		}
		var_hash = var_hash->next;
	}
}

static int var_access(php_unserialize_data_t *var_hashx, int id, zval ***store)
{
	var_entries *var_hash = var_hashx->first;
	
	while (id >= VAR_ENTRIES_MAX && var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		var_hash = var_hash->next;
		id -= VAR_ENTRIES_MAX;
	}

	if (!var_hash) return !SUCCESS;

	if (id >= var_hash->used_slots) return !SUCCESS;

	*store = &var_hash->data[id];

	return SUCCESS;
}

PHPAPI void var_destroy(php_unserialize_data_t *var_hashx)
{
	void *next;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}
}

/* }}} */

#define YYFILL(n) do { } while (0)
#define YYCTYPE unsigned char
#define YYCURSOR cursor
#define YYLIMIT limit
#define YYMARKER marker


#line 117




static inline int parse_iv2(const char *p, const char **q)
{
	char cursor;
	int result = 0;
	int neg = 0;

	switch (*p) {
		case '-':
			neg++;
			/* fall-through */
		case '+':
			p++;
	}
	
	while (1) {
		cursor = *p;
		if (cursor >= '0' && cursor <= '9') {
			result = result * 10 + cursor - '0';
		} else {
			break;
		}
		p++;
	}
	if (q) *q = p;
	if (neg) return -result;
	return result;
}

static inline int parse_iv(const char *p)
{
	return parse_iv2(p, NULL);
}

#define UNSERIALIZE_PARAMETER zval **rval, const char **p, const char *max, php_unserialize_data_t *var_hash TSRMLS_DC
#define UNSERIALIZE_PASSTHRU rval, p, max, var_hash TSRMLS_CC

static inline int process_nested_data(UNSERIALIZE_PARAMETER, HashTable *ht, int elements)
{
	while (elements-- > 0) {
		zval *key, *data;

		ALLOC_INIT_ZVAL(key);

		if (!php_var_unserialize(&key, p, max, NULL TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			return 0;
		}

		ALLOC_INIT_ZVAL(data);

		if (!php_var_unserialize(&data, p, max, var_hash TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			zval_dtor(data);
			FREE_ZVAL(data);
			return 0;
		}

		switch (Z_TYPE_P(key)) {
			case IS_LONG:
				zend_hash_index_update(ht, Z_LVAL_P(key), &data, sizeof(data), NULL);
				break;
			case IS_STRING:
				zend_hash_update(ht, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &data, sizeof(data), NULL);
				break;

		}
		
		zval_dtor(key);
		FREE_ZVAL(key);
	}

	return 1;
}

static inline int finish_nested_data(UNSERIALIZE_PARAMETER)
{
	if (*((*p)++) == '}') 
		return 1;

#if SOMETHING_NEW_MIGHT_LEAD_TO_CRASH_ENABLE_IF_YOU_ARE_BRAVE
	zval_ptr_dtor(rval);
#endif
	return 0;
}

static inline int object_common1(UNSERIALIZE_PARAMETER, zend_class_entry *ce)
{
	int elements;

	elements = parse_iv2((*p) + 2, p);

	(*p) += 2;
	
	object_init_ex(*rval, ce);
	return elements;
}

static inline int object_common2(UNSERIALIZE_PARAMETER, int elements)
{
	zval *retval_ptr = NULL;
	zval fname;

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_OBJPROP_PP(rval), elements)) {
		return 0;
	}

	INIT_PZVAL(&fname);
	ZVAL_STRINGL(&fname, "__wakeup", sizeof("__wakeup") - 1, 0);
	call_user_function_ex(CG(function_table), rval, &fname, &retval_ptr, 0, 0, 1, NULL TSRMLS_CC);

	if (retval_ptr)
		zval_ptr_dtor(&retval_ptr);

	return finish_nested_data(UNSERIALIZE_PASSTHRU);

}

PHPAPI int php_var_unserialize(UNSERIALIZE_PARAMETER)
{
	const unsigned char *cursor, *limit, *marker, *start;
	zval **rval_ref;

	limit = cursor = *p;
	
	if (var_hash && cursor[0] != 'R') {
		var_push(var_hash, rval);
	}

	start = cursor;

	
	
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if((YYLIMIT - YYCURSOR) < 5) YYFILL(5);
	yych = *YYCURSOR;
	if(yych <= 'd'){
		if(yych <= 'R'){
			if(yych <= 'N'){
				if(yych <= 'M')	goto yy15;
				goto yy5;
			} else {
				if(yych <= 'O')	goto yy12;
				if(yych <= 'Q')	goto yy15;
				goto yy3;
			}
		} else {
			if(yych <= 'a'){
				if(yych <= '`')	goto yy15;
				goto yy10;
			} else {
				if(yych <= 'b')	goto yy6;
				if(yych <= 'c')	goto yy15;
				goto yy8;
			}
		}
	} else {
		if(yych <= 'r'){
			if(yych <= 'i'){
				if(yych <= 'h')	goto yy15;
				goto yy7;
			} else {
				if(yych == 'o')	goto yy11;
				goto yy15;
			}
		} else {
			if(yych <= '|'){
				if(yych <= 's')	goto yy9;
				goto yy15;
			} else {
				if(yych <= '}')	goto yy13;
				if(yych <= '\277')	goto yy15;
			}
		}
	}
yy2:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy4;
	}
yy3:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy74;
yy4:
#line 441
	{ return 0; }
yy5:	yych = *++YYCURSOR;
	if(yych == ';')	goto yy72;
	goto yy4;
yy6:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy66;
	goto yy4;
yy7:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy60;
	goto yy4;
yy8:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy44;
	goto yy4;
yy9:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy37;
	goto yy4;
yy10:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy30;
	goto yy4;
yy11:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy23;
	goto yy4;
yy12:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy16;
	goto yy4;
yy13:	yych = *++YYCURSOR;
yy14:
#line 435
	{
	/* this is the case where we have less data than planned */
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unexpected end of serialized data");
	return 0; /* not sure if it should be 0 or 1 here? */
}
yy15:	yych = *++YYCURSOR;
	goto yy4;
yy16:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy18;
	if(yych == '+')	goto yy17;
	if(yych != '-')	goto yy2;
yy17:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy18;
	goto yy2;
yy18:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy19:	if(yybm[0+yych] & 128)	goto yy18;
	if(yych != ':')	goto yy2;
yy20:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
yy21:	yych = *++YYCURSOR;
yy22:
#line 350
	{
	int len;
	int elements;
	int len2;
	char *class_name;
	zend_class_entry *ce;
	zend_class_entry **pce;
	int incomplete_class = 0;
	
	zval *user_func;
	zval *retval_ptr;
	zval **args[1];
	zval *arg_func_name;
	
	INIT_PZVAL(*rval);
	len2 = len = parse_iv(start + 2);
	if (len == 0)
		return 0;

	class_name = estrndup(YYCURSOR, len);
	YYCURSOR += len;

	while (len-- > 0) {
		if (class_name[len] >= 'A' && class_name[len] <= 'Z') {
			class_name[len] = class_name[len] - 'A' + 'a';
		}
	}
	
	do {
		/* Try to find class directly */
		if (zend_lookup_class(class_name, len2, &pce TSRMLS_CC) == SUCCESS) {
			ce = *pce;
			break;
		}
		
		/* Check for unserialize callback */
		if ((PG(unserialize_callback_func) == NULL) || (PG(unserialize_callback_func)[0] == '\0')) {
			incomplete_class = 1;
			ce = PHP_IC_ENTRY;
			break;
		}
		
		/* Call unserialize callback */
		MAKE_STD_ZVAL(user_func);
		ZVAL_STRING(user_func, PG(unserialize_callback_func), 1);
		args[0] = &arg_func_name;
		MAKE_STD_ZVAL(arg_func_name);
		ZVAL_STRING(arg_func_name, class_name, 1);
		if (call_user_function_ex(CG(function_table), NULL, user_func, &retval_ptr, 1, args, 0, NULL TSRMLS_CC) != SUCCESS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "defined (%s) but not found", user_func->value.str.val);
			incomplete_class = 1;
			ce = PHP_IC_ENTRY;
			zval_ptr_dtor(&user_func);
			zval_ptr_dtor(&arg_func_name);
			break;
		}
		if (retval_ptr) {
			zval_ptr_dtor(&retval_ptr);
		}
		
		/* The callback function may have defined the class */
		if (zend_lookup_class(class_name, len2, &pce TSRMLS_CC) == SUCCESS) {
			ce = *pce;
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Function %s() hasn't defined the class it was called for", user_func->value.str.val);
			incomplete_class = 1;
			ce = PHP_IC_ENTRY;
		}

		zval_ptr_dtor(&user_func);
		zval_ptr_dtor(&arg_func_name);
		break;
	} while (1);
	
	*p = YYCURSOR;
	elements = object_common1(UNSERIALIZE_PASSTHRU, ce);

	if (incomplete_class) {
		php_store_class_name(*rval, class_name, len2);
	}
	efree(class_name);

	return object_common2(UNSERIALIZE_PASSTHRU, elements);
}
yy23:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy24;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy25;
		goto yy2;
	}
yy24:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy25:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy26:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy25;
	if(yych >= ';')	goto yy2;
yy27:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
yy28:	yych = *++YYCURSOR;
yy29:
#line 342
	{

	INIT_PZVAL(*rval);
	
	return object_common2(UNSERIALIZE_PASSTHRU,
			object_common1(UNSERIALIZE_PASSTHRU, ZEND_STANDARD_CLASS_DEF_PTR));
}
yy30:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy31;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy32;
		goto yy2;
	}
yy31:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy32:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy33:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy32;
	if(yych >= ';')	goto yy2;
yy34:	yych = *++YYCURSOR;
	if(yych != '{')	goto yy2;
yy35:	yych = *++YYCURSOR;
yy36:
#line 324
	{
	int elements = parse_iv(start + 2);

	*p = YYCURSOR;

	INIT_PZVAL(*rval);
	Z_TYPE_PP(rval) = IS_ARRAY;
	ALLOC_HASHTABLE(Z_ARRVAL_PP(rval));

	zend_hash_init(Z_ARRVAL_PP(rval), elements + 1, NULL, ZVAL_PTR_DTOR, 0);

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_ARRVAL_PP(rval), elements)) {
		return 0;
	}

	return finish_nested_data(UNSERIALIZE_PASSTHRU);
}
yy37:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy38;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy39;
		goto yy2;
	}
yy38:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy39:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy40:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy39;
	if(yych >= ';')	goto yy2;
yy41:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
yy42:	yych = *++YYCURSOR;
yy43:
#line 304
	{
	int len;
	char *str;

	len = parse_iv(start + 2);

	if (len == 0) {
		str = empty_string;
	} else {
		str = estrndup(YYCURSOR, len);
	}

	YYCURSOR += len + 2;
	*p = YYCURSOR;

	INIT_PZVAL(*rval);
	ZVAL_STRINGL(*rval, str, len, 0);
	return 1;
}
yy44:	yych = *++YYCURSOR;
	if(yych <= '-'){
		if(yych == '+')	goto yy45;
		if(yych <= ',')	goto yy2;
	} else {
		if(yych <= '.')	goto yy48;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy46;
		goto yy2;
	}
yy45:	yych = *++YYCURSOR;
	if(yych == '.')	goto yy48;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy46:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy47:	if(yych <= ':'){
		if(yych <= '.'){
			if(yych <= '-')	goto yy2;
			goto yy58;
		} else {
			if(yych <= '/')	goto yy2;
			if(yych <= '9')	goto yy46;
			goto yy2;
		}
	} else {
		if(yych <= 'E'){
			if(yych <= ';')	goto yy51;
			if(yych <= 'D')	goto yy2;
			goto yy53;
		} else {
			if(yych == 'e')	goto yy53;
			goto yy2;
		}
	}
yy48:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy49:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy50:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy49;
		if(yych <= ':')	goto yy2;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy53;
		} else {
			if(yych == 'e')	goto yy53;
			goto yy2;
		}
	}
yy51:	yych = *++YYCURSOR;
yy52:
#line 297
	{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_DOUBLE(*rval, atof(start + 2));
	return 1;
}
yy53:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy54;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy55;
		goto yy2;
	}
yy54:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych == '+')	goto yy57;
		goto yy2;
	} else {
		if(yych <= '-')	goto yy57;
		if(yych <= '/')	goto yy2;
		if(yych >= ':')	goto yy2;
	}
yy55:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy56:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy55;
	if(yych == ';')	goto yy51;
	goto yy2;
yy57:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy55;
	goto yy2;
yy58:	++YYCURSOR;
	if((YYLIMIT - YYCURSOR) < 4) YYFILL(4);
	yych = *YYCURSOR;
yy59:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy58;
		if(yych <= ':')	goto yy2;
		goto yy51;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy53;
		} else {
			if(yych == 'e')	goto yy53;
			goto yy2;
		}
	}
yy60:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy61;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy62;
		goto yy2;
	}
yy61:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy62:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy63:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy62;
	if(yych != ';')	goto yy2;
yy64:	yych = *++YYCURSOR;
yy65:
#line 290
	{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_LONG(*rval, parse_iv(start + 2));
	return 1;
}
yy66:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy67;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy68;
		goto yy2;
	}
yy67:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy68:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy69:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy68;
	if(yych != ';')	goto yy2;
yy70:	yych = *++YYCURSOR;
yy71:
#line 283
	{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_BOOL(*rval, parse_iv(start + 2));
	return 1;
}
yy72:	yych = *++YYCURSOR;
yy73:
#line 276
	{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_NULL(*rval);
	return 1;
}
yy74:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
	} else {
		if(yych <= '-')	goto yy75;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy76;
		goto yy2;
	}
yy75:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
yy76:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy77:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy76;
	if(yych != ';')	goto yy2;
yy78:	yych = *++YYCURSOR;
yy79:
#line 257
	{
	int id;

 	*p = YYCURSOR;
	if (!var_hash) return 0;

	id = parse_iv(start + 2) - 1;
	if (id == -1 || var_access(var_hash, id, &rval_ref) != SUCCESS) {
		return 0;
	}

	zval_ptr_dtor(rval);
	*rval = *rval_ref;
	(*rval)->refcount++;
	(*rval)->is_ref = 1;
	
	return 1;
}
}
#line 443


	return 0;
}
