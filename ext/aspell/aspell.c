/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors:                                                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#if defined(COMPILE_DL)
#include "phpdl.h"
#endif
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#if HAVE_ASPELL

#include "php_aspell.h"
#include <aspell-c.h>

function_entry aspell_functions[] = {
	PHP_FE(aspell_new,								NULL)
	PHP_FE(aspell_check,							NULL)
	PHP_FE(aspell_check_raw,						NULL)
	PHP_FE(aspell_suggest,							NULL)
	{NULL, NULL, NULL}
};

static int le_aspell;

zend_module_entry aspell_module_entry = {
	"aspell", aspell_functions, PHP_MINIT(aspell), NULL, NULL, NULL, PHP_MINFO(aspell), STANDARD_MODULE_PROPERTIES
};


#if COMPILE_DL
DLEXPORT zend_module_entry *get_module(void) { return &aspell_module_entry; }
#endif

static void php_aspell_close(aspell *sc)
{
	aspell_free(sc);
}


PHP_MINIT_FUNCTION(aspell)
{
    le_aspell = register_list_destructors(php_aspell_close,NULL);
	return SUCCESS;

}

/* {{{ proto int aspell_new(string master [, string personal])
   Load a dictionary */
PHP_FUNCTION(aspell_new)
{
	pval **master,**personal;
	int argc;
	aspell *sc;
	int ind;
	
	argc = ARG_COUNT(ht);
	if (argc < 1 || argc > 2 || zend_get_parameters_ex(argc,&master,&personal) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_string_ex(master);
	if(argc==2)
	  {
		convert_to_string_ex(personal) ;
		sc=aspell_new((*master)->value.str.val,(*personal)->value.str.val);
	  }
	else
	  sc=aspell_new((*master)->value.str.val,"");

	ind = zend_list_insert(sc, le_aspell);
	RETURN_LONG(ind);
}
/* }}} */


/* {{{ proto array aspell_suggest(aspell int, string word)
   Return array of Suggestions */
PHP_FUNCTION(aspell_suggest)
{
	pval **scin, **word;
	int argc;
	aspell *sc;
	int ind,type;
	aspellSuggestions *sug;
	size_t i;

	
	argc = ARG_COUNT(ht);
	if (argc != 2 || zend_get_parameters_ex(argc, &scin,&word) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	convert_to_long_ex(scin);
	convert_to_string_ex(word);
	sc = (aspell *)zend_list_find((*scin)->value.lval, &type);
	if(!sc)
	  {
		php_error(E_WARNING, "%d is not an ASPELL result index",(*scin)->value.lval);
		RETURN_FALSE;
	  }

	if (array_init(return_value) == FAILURE) {
                RETURN_FALSE;
	}

	sug = aspell_suggest(sc, (*word)->value.str.val);
	  for (i = 0; i != sug->size; ++i) {
                add_next_index_string(return_value,(char *)sug->data[i],1);
	  }
	  aspell_free_suggestions(sug);
}
/* }}} */

/* {{{ proto int aspell_check(aspell int, string word)
   Return if word is valid */
PHP_FUNCTION(aspell_check)
{
   int type;
   pval **scin,**word;
   aspell *sc;

   int argc;
    argc = ARG_COUNT(ht);
    if (argc != 2 || zend_get_parameters_ex(argc, &scin,&word) == FAILURE) {
        WRONG_PARAM_COUNT;
    }
    convert_to_long_ex(scin);
    convert_to_string_ex(word);
    sc= (aspell *) zend_list_find((*scin)->value.lval, &type);
    if(!sc)
      {
        php_error(E_WARNING, "%d is not an ASPELL result index",(*scin)->value.lval);
        RETURN_FALSE;
      }
    if (aspell_check(sc, (*word)->value.str.val)) 
      {
	RETURN_TRUE;
      }
    else 
      {
  RETURN_FALSE;
      }
}
/* }}} */

/* {{{ proto int aspell_check_raw(aspell int, string word)
   Return if word is valid, ignoring case or trying to trim it in any way */
PHP_FUNCTION(aspell_check_raw)
{
  pval **scin,**word;
  int type;
  int argc;
  aspell *sc;

    argc = ARG_COUNT(ht);
    if (argc != 2 || zend_get_parameters_ex(argc, &scin,&word) == FAILURE) {
        WRONG_PARAM_COUNT;
    }
    convert_to_long_ex(scin);
    convert_to_string_ex(word);
    sc = (aspell *)zend_list_find((*scin)->value.lval, &type);
    if(!sc)
      {
        php_error(E_WARNING, "%d is not an ASPELL result index",(*scin)->value.lval);
        RETURN_FALSE;
      }
	if (aspell_check_raw(sc, (*word)->value.str.val)) 
	  {
	    RETURN_TRUE;
	  }
	else
	  {
	    RETURN_FALSE;
	      }
}
/* }}} */

PHP_MINFO_FUNCTION(aspell)
{
	php_printf("ASpell support enabled");

}

#endif
