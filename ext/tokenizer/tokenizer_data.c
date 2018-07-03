/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Johannes Schlueter <johannes@php.net>                        |
   +----------------------------------------------------------------------+
*/

/*
   DO NOT EDIT THIS FILE!
   This file is generated using tokenizer_data_gen.sh
*/

#include "php.h"
#include "zend.h"
#include <zend_language_parser.h>


void tokenizer_register_constants(INIT_FUNC_ARGS) {
	REGISTER_LONG_CONSTANT("T_REQUIRE_ONCE", T_REQUIRE_ONCE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_REQUIRE", T_REQUIRE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_EVAL", T_EVAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INCLUDE_ONCE", T_INCLUDE_ONCE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INCLUDE", T_INCLUDE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LOGICAL_OR", T_LOGICAL_OR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LOGICAL_XOR", T_LOGICAL_XOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LOGICAL_AND", T_LOGICAL_AND, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PRINT", T_PRINT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_YIELD", T_YIELD, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DOUBLE_ARROW", T_DOUBLE_ARROW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_YIELD_FROM", T_YIELD_FROM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_POW_EQUAL", T_POW_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SR_EQUAL", T_SR_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SL_EQUAL", T_SL_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_XOR_EQUAL", T_XOR_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_OR_EQUAL", T_OR_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_AND_EQUAL", T_AND_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_MOD_EQUAL", T_MOD_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CONCAT_EQUAL", T_CONCAT_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DIV_EQUAL", T_DIV_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_MUL_EQUAL", T_MUL_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_MINUS_EQUAL", T_MINUS_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PLUS_EQUAL", T_PLUS_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_COALESCE", T_COALESCE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_BOOLEAN_OR", T_BOOLEAN_OR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_BOOLEAN_AND", T_BOOLEAN_AND, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SPACESHIP", T_SPACESHIP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_NOT_IDENTICAL", T_IS_NOT_IDENTICAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_IDENTICAL", T_IS_IDENTICAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_NOT_EQUAL", T_IS_NOT_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_EQUAL", T_IS_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_GREATER_OR_EQUAL", T_IS_GREATER_OR_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IS_SMALLER_OR_EQUAL", T_IS_SMALLER_OR_EQUAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SR", T_SR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SL", T_SL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INSTANCEOF", T_INSTANCEOF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_UNSET_CAST", T_UNSET_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_BOOL_CAST", T_BOOL_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_OBJECT_CAST", T_OBJECT_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ARRAY_CAST", T_ARRAY_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_STRING_CAST", T_STRING_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DOUBLE_CAST", T_DOUBLE_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INT_CAST", T_INT_CAST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DEC", T_DEC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INC", T_INC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_POW", T_POW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CLONE", T_CLONE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_NEW", T_NEW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ELSEIF", T_ELSEIF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ELSE", T_ELSE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDIF", T_ENDIF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PUBLIC", T_PUBLIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PROTECTED", T_PROTECTED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PRIVATE", T_PRIVATE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FINAL", T_FINAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ABSTRACT", T_ABSTRACT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_STATIC", T_STATIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LNUMBER", T_LNUMBER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DNUMBER", T_DNUMBER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_STRING", T_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_VARIABLE", T_VARIABLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INLINE_HTML", T_INLINE_HTML, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENCAPSED_AND_WHITESPACE", T_ENCAPSED_AND_WHITESPACE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CONSTANT_ENCAPSED_STRING", T_CONSTANT_ENCAPSED_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_STRING_VARNAME", T_STRING_VARNAME, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_NUM_STRING", T_NUM_STRING, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_EXIT", T_EXIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IF", T_IF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ECHO", T_ECHO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DO", T_DO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_WHILE", T_WHILE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDWHILE", T_ENDWHILE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FOR", T_FOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDFOR", T_ENDFOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FOREACH", T_FOREACH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDFOREACH", T_ENDFOREACH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DECLARE", T_DECLARE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDDECLARE", T_ENDDECLARE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_AS", T_AS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_SWITCH", T_SWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ENDSWITCH", T_ENDSWITCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CASE", T_CASE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DEFAULT", T_DEFAULT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_BREAK", T_BREAK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CONTINUE", T_CONTINUE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_GOTO", T_GOTO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FUNCTION", T_FUNCTION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CONST", T_CONST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_RETURN", T_RETURN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_TRY", T_TRY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CATCH", T_CATCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FINALLY", T_FINALLY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_THROW", T_THROW, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_USE", T_USE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INSTEADOF", T_INSTEADOF, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_GLOBAL", T_GLOBAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_VAR", T_VAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_UNSET", T_UNSET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ISSET", T_ISSET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_EMPTY", T_EMPTY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_HALT_COMPILER", T_HALT_COMPILER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CLASS", T_CLASS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_TRAIT", T_TRAIT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_INTERFACE", T_INTERFACE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_EXTENDS", T_EXTENDS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_IMPLEMENTS", T_IMPLEMENTS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_OBJECT_OPERATOR", T_OBJECT_OPERATOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LIST", T_LIST, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ARRAY", T_ARRAY, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CALLABLE", T_CALLABLE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_LINE", T_LINE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FILE", T_FILE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DIR", T_DIR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CLASS_C", T_CLASS_C, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_TRAIT_C", T_TRAIT_C, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_METHOD_C", T_METHOD_C, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_FUNC_C", T_FUNC_C, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_COMMENT", T_COMMENT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DOC_COMMENT", T_DOC_COMMENT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_OPEN_TAG", T_OPEN_TAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_OPEN_TAG_WITH_ECHO", T_OPEN_TAG_WITH_ECHO, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CLOSE_TAG", T_CLOSE_TAG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_WHITESPACE", T_WHITESPACE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_START_HEREDOC", T_START_HEREDOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_END_HEREDOC", T_END_HEREDOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DOLLAR_OPEN_CURLY_BRACES", T_DOLLAR_OPEN_CURLY_BRACES, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_CURLY_OPEN", T_CURLY_OPEN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_PAAMAYIM_NEKUDOTAYIM", T_PAAMAYIM_NEKUDOTAYIM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_NAMESPACE", T_NAMESPACE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_NS_C", T_NS_C, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_NS_SEPARATOR", T_NS_SEPARATOR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_ELLIPSIS", T_ELLIPSIS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("T_DOUBLE_COLON", T_PAAMAYIM_NEKUDOTAYIM, CONST_CS | CONST_PERSISTENT);
}

char *get_token_type_name(int token_type)
{
	switch (token_type) {

		case T_REQUIRE_ONCE: return "T_REQUIRE_ONCE";
		case T_REQUIRE: return "T_REQUIRE";
		case T_EVAL: return "T_EVAL";
		case T_INCLUDE_ONCE: return "T_INCLUDE_ONCE";
		case T_INCLUDE: return "T_INCLUDE";
		case T_LOGICAL_OR: return "T_LOGICAL_OR";
		case T_LOGICAL_XOR: return "T_LOGICAL_XOR";
		case T_LOGICAL_AND: return "T_LOGICAL_AND";
		case T_PRINT: return "T_PRINT";
		case T_YIELD: return "T_YIELD";
		case T_DOUBLE_ARROW: return "T_DOUBLE_ARROW";
		case T_YIELD_FROM: return "T_YIELD_FROM";
		case T_POW_EQUAL: return "T_POW_EQUAL";
		case T_SR_EQUAL: return "T_SR_EQUAL";
		case T_SL_EQUAL: return "T_SL_EQUAL";
		case T_XOR_EQUAL: return "T_XOR_EQUAL";
		case T_OR_EQUAL: return "T_OR_EQUAL";
		case T_AND_EQUAL: return "T_AND_EQUAL";
		case T_MOD_EQUAL: return "T_MOD_EQUAL";
		case T_CONCAT_EQUAL: return "T_CONCAT_EQUAL";
		case T_DIV_EQUAL: return "T_DIV_EQUAL";
		case T_MUL_EQUAL: return "T_MUL_EQUAL";
		case T_MINUS_EQUAL: return "T_MINUS_EQUAL";
		case T_PLUS_EQUAL: return "T_PLUS_EQUAL";
		case T_COALESCE: return "T_COALESCE";
		case T_BOOLEAN_OR: return "T_BOOLEAN_OR";
		case T_BOOLEAN_AND: return "T_BOOLEAN_AND";
		case T_SPACESHIP: return "T_SPACESHIP";
		case T_IS_NOT_IDENTICAL: return "T_IS_NOT_IDENTICAL";
		case T_IS_IDENTICAL: return "T_IS_IDENTICAL";
		case T_IS_NOT_EQUAL: return "T_IS_NOT_EQUAL";
		case T_IS_EQUAL: return "T_IS_EQUAL";
		case T_IS_GREATER_OR_EQUAL: return "T_IS_GREATER_OR_EQUAL";
		case T_IS_SMALLER_OR_EQUAL: return "T_IS_SMALLER_OR_EQUAL";
		case T_SR: return "T_SR";
		case T_SL: return "T_SL";
		case T_INSTANCEOF: return "T_INSTANCEOF";
		case T_UNSET_CAST: return "T_UNSET_CAST";
		case T_BOOL_CAST: return "T_BOOL_CAST";
		case T_OBJECT_CAST: return "T_OBJECT_CAST";
		case T_ARRAY_CAST: return "T_ARRAY_CAST";
		case T_STRING_CAST: return "T_STRING_CAST";
		case T_DOUBLE_CAST: return "T_DOUBLE_CAST";
		case T_INT_CAST: return "T_INT_CAST";
		case T_DEC: return "T_DEC";
		case T_INC: return "T_INC";
		case T_POW: return "T_POW";
		case T_CLONE: return "T_CLONE";
		case T_NEW: return "T_NEW";
		case T_ELSEIF: return "T_ELSEIF";
		case T_ELSE: return "T_ELSE";
		case T_ENDIF: return "T_ENDIF";
		case T_PUBLIC: return "T_PUBLIC";
		case T_PROTECTED: return "T_PROTECTED";
		case T_PRIVATE: return "T_PRIVATE";
		case T_FINAL: return "T_FINAL";
		case T_ABSTRACT: return "T_ABSTRACT";
		case T_STATIC: return "T_STATIC";
		case T_LNUMBER: return "T_LNUMBER";
		case T_DNUMBER: return "T_DNUMBER";
		case T_STRING: return "T_STRING";
		case T_VARIABLE: return "T_VARIABLE";
		case T_INLINE_HTML: return "T_INLINE_HTML";
		case T_ENCAPSED_AND_WHITESPACE: return "T_ENCAPSED_AND_WHITESPACE";
		case T_CONSTANT_ENCAPSED_STRING: return "T_CONSTANT_ENCAPSED_STRING";
		case T_STRING_VARNAME: return "T_STRING_VARNAME";
		case T_NUM_STRING: return "T_NUM_STRING";
		case T_EXIT: return "T_EXIT";
		case T_IF: return "T_IF";
		case T_ECHO: return "T_ECHO";
		case T_DO: return "T_DO";
		case T_WHILE: return "T_WHILE";
		case T_ENDWHILE: return "T_ENDWHILE";
		case T_FOR: return "T_FOR";
		case T_ENDFOR: return "T_ENDFOR";
		case T_FOREACH: return "T_FOREACH";
		case T_ENDFOREACH: return "T_ENDFOREACH";
		case T_DECLARE: return "T_DECLARE";
		case T_ENDDECLARE: return "T_ENDDECLARE";
		case T_AS: return "T_AS";
		case T_SWITCH: return "T_SWITCH";
		case T_ENDSWITCH: return "T_ENDSWITCH";
		case T_CASE: return "T_CASE";
		case T_DEFAULT: return "T_DEFAULT";
		case T_BREAK: return "T_BREAK";
		case T_CONTINUE: return "T_CONTINUE";
		case T_GOTO: return "T_GOTO";
		case T_FUNCTION: return "T_FUNCTION";
		case T_CONST: return "T_CONST";
		case T_RETURN: return "T_RETURN";
		case T_TRY: return "T_TRY";
		case T_CATCH: return "T_CATCH";
		case T_FINALLY: return "T_FINALLY";
		case T_THROW: return "T_THROW";
		case T_USE: return "T_USE";
		case T_INSTEADOF: return "T_INSTEADOF";
		case T_GLOBAL: return "T_GLOBAL";
		case T_VAR: return "T_VAR";
		case T_UNSET: return "T_UNSET";
		case T_ISSET: return "T_ISSET";
		case T_EMPTY: return "T_EMPTY";
		case T_HALT_COMPILER: return "T_HALT_COMPILER";
		case T_CLASS: return "T_CLASS";
		case T_TRAIT: return "T_TRAIT";
		case T_INTERFACE: return "T_INTERFACE";
		case T_EXTENDS: return "T_EXTENDS";
		case T_IMPLEMENTS: return "T_IMPLEMENTS";
		case T_OBJECT_OPERATOR: return "T_OBJECT_OPERATOR";
		case T_LIST: return "T_LIST";
		case T_ARRAY: return "T_ARRAY";
		case T_CALLABLE: return "T_CALLABLE";
		case T_LINE: return "T_LINE";
		case T_FILE: return "T_FILE";
		case T_DIR: return "T_DIR";
		case T_CLASS_C: return "T_CLASS_C";
		case T_TRAIT_C: return "T_TRAIT_C";
		case T_METHOD_C: return "T_METHOD_C";
		case T_FUNC_C: return "T_FUNC_C";
		case T_COMMENT: return "T_COMMENT";
		case T_DOC_COMMENT: return "T_DOC_COMMENT";
		case T_OPEN_TAG: return "T_OPEN_TAG";
		case T_OPEN_TAG_WITH_ECHO: return "T_OPEN_TAG_WITH_ECHO";
		case T_CLOSE_TAG: return "T_CLOSE_TAG";
		case T_WHITESPACE: return "T_WHITESPACE";
		case T_START_HEREDOC: return "T_START_HEREDOC";
		case T_END_HEREDOC: return "T_END_HEREDOC";
		case T_DOLLAR_OPEN_CURLY_BRACES: return "T_DOLLAR_OPEN_CURLY_BRACES";
		case T_CURLY_OPEN: return "T_CURLY_OPEN";
		case T_PAAMAYIM_NEKUDOTAYIM: return "T_DOUBLE_COLON";
		case T_NAMESPACE: return "T_NAMESPACE";
		case T_NS_C: return "T_NS_C";
		case T_NS_SEPARATOR: return "T_NS_SEPARATOR";
		case T_ELLIPSIS: return "T_ELLIPSIS";

	}
	return "UNKNOWN";
}

