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
   | Authors: Felipe Pena <felipe@php.net>                                |
   | Authors: Joe Watkins <joe.watkins@live.co.uk>                        |
   | Authors: Bob Weinand <bwoebi@php.net>                                |
   +----------------------------------------------------------------------+
*/

#ifndef PHPDBG_UTILS_H
#define PHPDBG_UTILS_H

/**
 * Input scan functions
 */
PHPDBG_API int phpdbg_is_numeric(const char*);
PHPDBG_API int phpdbg_is_empty(const char*);
PHPDBG_API int phpdbg_is_addr(const char*);
PHPDBG_API int phpdbg_is_class_method(const char*, size_t, char**, char**);
PHPDBG_API const char *phpdbg_current_file(TSRMLS_D);
PHPDBG_API char *phpdbg_resolve_path(const char* TSRMLS_DC);
PHPDBG_API char *phpdbg_trim(const char*, size_t, size_t*);
PHPDBG_API const zend_function *phpdbg_get_function(const char*, const char* TSRMLS_DC);

/* {{{ Color Management */
#define PHPDBG_COLOR_LEN 12
#define PHPDBG_COLOR_D(color, code) \
	{color, sizeof(color)-1, code}
#define PHPDBG_COLOR_END \
	{NULL, 0L, {0}}
#define PHPDBG_ELEMENT_LEN 3
#define PHPDBG_ELEMENT_D(name, id) \
	{name, sizeof(name)-1, id}
#define PHPDBG_ELEMENT_END \
	{NULL, 0L, 0}

#define PHPDBG_COLOR_INVALID	-1
#define PHPDBG_COLOR_PROMPT 	 0
#define PHPDBG_COLOR_ERROR		 1
#define PHPDBG_COLOR_NOTICE		 2
#define PHPDBG_COLORS			 3

typedef struct _phpdbg_color_t {
	char       *name;
	size_t      name_length;
	const char  code[PHPDBG_COLOR_LEN];
} phpdbg_color_t;

typedef struct _phpdbg_element_t {
	char		*name;
	size_t		name_length;
	int			id;
} phpdbg_element_t;

PHPDBG_API const phpdbg_color_t *phpdbg_get_color(const char *name, size_t name_length TSRMLS_DC);
PHPDBG_API void phpdbg_set_color(int element, const phpdbg_color_t *color TSRMLS_DC);
PHPDBG_API void phpdbg_set_color_ex(int element, const char *name, size_t name_length TSRMLS_DC);
PHPDBG_API const phpdbg_color_t *phpdbg_get_colors(TSRMLS_D);
PHPDBG_API int phpdbg_get_element(const char *name, size_t len TSRMLS_DC); /* }}} */

/* {{{ Prompt Management */
PHPDBG_API void phpdbg_set_prompt(const char* TSRMLS_DC);
PHPDBG_API const char *phpdbg_get_prompt(TSRMLS_D); /* }}} */

/* {{{ Console Width */
PHPDBG_API int phpdbg_get_terminal_width(TSRMLS_D); /* }}} */

PHPDBG_API void phpdbg_set_async_io(int fd);

int phpdbg_rebuild_symtable(TSRMLS_D);

int phpdbg_safe_class_lookup(const char *name, int name_length, zend_class_entry **ce TSRMLS_DC);

char *phpdbg_get_property_key(char *key);

typedef int (*phpdbg_parse_var_func)(char *name, size_t len, char *keyname, size_t keylen, HashTable *parent, zval *zv TSRMLS_DC);
typedef int (*phpdbg_parse_var_with_arg_func)(char *name, size_t len, char *keyname, size_t keylen, HashTable *parent, zval *zv, void *arg TSRMLS_DC);

PHPDBG_API int phpdbg_parse_variable(char *input, size_t len, HashTable *parent, size_t i, phpdbg_parse_var_func callback, zend_bool silent TSRMLS_DC);
PHPDBG_API int phpdbg_parse_variable_with_arg(char *input, size_t len, HashTable *parent, size_t i, phpdbg_parse_var_with_arg_func callback, zend_bool silent, void *arg TSRMLS_DC);

int phpdbg_is_auto_global(char *name, int len TSRMLS_DC);

PHPDBG_API void phpdbg_xml_var_dump(zval *zv TSRMLS_DC);

#endif /* PHPDBG_UTILS_H */
