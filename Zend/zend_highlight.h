/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2000 Andi Gutmans, Zeev Suraski                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.91 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_91.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#ifndef _HIGHLIGHT_H
#define _HIGHLIGHT_H

#define HL_COMMENT_COLOR     "#FF8000"    /* orange */
#define HL_DEFAULT_COLOR     "#0000BB"    /* blue */
#define HL_HTML_COLOR        "#000000"    /* black */
#define HL_STRING_COLOR      "#DD0000"    /* red */
#define HL_BG_COLOR          "#FFFFFF"    /* white */
#define HL_KEYWORD_COLOR     "#007700"    /* green */


typedef struct _zend_syntax_highlighter_ini {
	char *highlight_html;
	char *highlight_comment;
	char *highlight_default;
	char *highlight_string;
	char *highlight_keyword;
} zend_syntax_highlighter_ini;


BEGIN_EXTERN_C()
ZEND_API void zend_highlight(zend_syntax_highlighter_ini *syntax_highlighter_ini);
int highlight_file(char *filename, zend_syntax_highlighter_ini *syntax_highlighter_ini);
int highlight_string(zval *str, zend_syntax_highlighter_ini *syntax_highlighter_ini);
END_EXTERN_C()

extern zend_syntax_highlighter_ini syntax_highlighter_ini;

#endif
