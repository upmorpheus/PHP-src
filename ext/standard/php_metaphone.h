/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Thies C. Arntzen <thies@digicol.de>						  |
   +----------------------------------------------------------------------+
 */
 
/* $Id$ */

#ifndef _php_metaphone_h
#define _php_metaphone_h

char Lookahead(char *word, int how_far);
extern zend_module_entry metaphone_module_entry;
#define phpext_metaphone_ptr &metaphone_module_entry

#endif
