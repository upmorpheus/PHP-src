/*
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
  | Author: Keyur Govande (kgovande@gmail.com)                           |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_PS_TITLE_HEADER
#define PHP_PS_TITLE_HEADER

ZEND_BEGIN_ARG_INFO(arginfo_cli_set_process_title, 0)
    ZEND_ARG_INFO(0, title)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_cli_get_process_title, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(cli_set_process_title);
PHP_FUNCTION(cli_get_process_title);

#endif
