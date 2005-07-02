/* 
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2004 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Derick Rethans <derick@derickrethans.nl>                    |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_DATE_H
#define PHP_DATE_H

extern zend_module_entry date_module_entry;
#define phpext_date_ptr &date_module_entry

PHP_FUNCTION(date);
PHP_FUNCTION(gmdate);
PHP_FUNCTION(strtotime);
PHP_FUNCTION(date_timezone_set);
PHP_FUNCTION(date_timezone_get);

PHP_RINIT_FUNCTION(date);
PHP_RSHUTDOWN_FUNCTION(date);
PHP_MINIT_FUNCTION(date);
PHP_MSHUTDOWN_FUNCTION(date);
PHP_MINFO_FUNCTION(date);

ZEND_BEGIN_MODULE_GLOBALS(date)
	char *default_timezone;
	char *timezone;
ZEND_END_MODULE_GLOBALS(date)

#ifdef ZTS
#define DATEG(v) TSRMG(date_globals_id, zend_date_globals *, v)
#else
#define DATEG(v) (date_globals.v)
#endif

/* Backwards compability wrapper */
signed long php_parse_date(char *string, signed long *now);

#endif /* PHP_DATE_H */
