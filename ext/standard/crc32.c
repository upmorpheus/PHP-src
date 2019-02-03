/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Rasmus Lerdorf <rasmus@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "php.h"
#include "basic_functions.h"
#include "crc32.h"

/* {{{ proto string crc32(string str)
   Calculate the crc32 polynomial of a string */
PHP_NAMED_FUNCTION(php_if_crc32)
{
	char *p;
	size_t nr;
	uint32_t crcinit = 0;
	register uint32_t crc;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STRING(p, nr)
	ZEND_PARSE_PARAMETERS_END();

	crc = crcinit^0xFFFFFFFF;

	for (; nr--; ++p) {
		crc = ((crc >> 8) & 0x00FFFFFF) ^ crc32tab[(crc ^ (*p)) & 0xFF ];
	}
	RETVAL_LONG(crc^0xFFFFFFFF);
}
/* }}} */
