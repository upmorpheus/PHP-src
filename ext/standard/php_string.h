/* 
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2009 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Rasmus Lerdorf <rasmus@php.net>                             |
   |          Stig S�ther Bakken <ssb@php.net>                            |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

/* Synced with php 3.0 revision 1.43 1999-06-16 [ssb] */

#ifndef PHP_STRING_H
#define PHP_STRING_H

PHP_FUNCTION(strspn);
PHP_FUNCTION(strcspn);
PHP_FUNCTION(str_replace);
PHP_FUNCTION(str_ireplace);
PHP_FUNCTION(rtrim);
PHP_FUNCTION(trim);
PHP_FUNCTION(ltrim);
PHP_FUNCTION(soundex);
PHP_FUNCTION(levenshtein);

PHP_FUNCTION(count_chars);
PHP_FUNCTION(wordwrap);
PHP_FUNCTION(explode);
PHP_FUNCTION(implode);
PHP_FUNCTION(strtok);
PHP_FUNCTION(strtoupper);
PHP_FUNCTION(strtolower);
PHP_FUNCTION(strtotitle);
PHP_FUNCTION(basename);
PHP_FUNCTION(dirname);
PHP_FUNCTION(pathinfo);
PHP_FUNCTION(strstr);
PHP_FUNCTION(strpos);
PHP_FUNCTION(stripos);
PHP_FUNCTION(strrpos);
PHP_FUNCTION(strripos);
PHP_FUNCTION(strrchr);
PHP_FUNCTION(substr);
PHP_FUNCTION(quotemeta);
PHP_FUNCTION(ucfirst);
PHP_FUNCTION(lcfirst);
PHP_FUNCTION(ucwords);
PHP_FUNCTION(strtr);
PHP_FUNCTION(strrev);
PHP_FUNCTION(hebrev);
PHP_FUNCTION(hebrevc);
PHP_FUNCTION(user_sprintf);
PHP_FUNCTION(user_printf);
PHP_FUNCTION(vprintf);
PHP_FUNCTION(vsprintf);
PHP_FUNCTION(addcslashes);
PHP_FUNCTION(addslashes);
PHP_FUNCTION(stripcslashes);
PHP_FUNCTION(stripslashes);
PHP_FUNCTION(chr);
PHP_FUNCTION(ord);
PHP_FUNCTION(nl2br);
PHP_FUNCTION(setlocale);
PHP_FUNCTION(localeconv);
PHP_FUNCTION(nl_langinfo);
PHP_FUNCTION(stristr);
PHP_FUNCTION(chunk_split);
PHP_FUNCTION(parse_str);
PHP_FUNCTION(str_getcsv);
PHP_FUNCTION(bin2hex);
PHP_FUNCTION(similar_text);
PHP_FUNCTION(strip_tags);
PHP_FUNCTION(str_repeat);
PHP_FUNCTION(substr_replace);
PHP_FUNCTION(strnatcmp);
PHP_FUNCTION(strnatcasecmp);
PHP_FUNCTION(substr_count);
PHP_FUNCTION(str_pad);
PHP_FUNCTION(sscanf);
PHP_FUNCTION(str_shuffle);
PHP_FUNCTION(str_word_count);
PHP_FUNCTION(str_split);
PHP_FUNCTION(strpbrk);
PHP_FUNCTION(substr_compare);
#ifdef HAVE_STRCOLL
PHP_FUNCTION(strcoll);
#endif
#if HAVE_STRFMON
PHP_FUNCTION(money_format);
#endif

#if defined(HAVE_LOCALECONV) && defined(ZTS)
PHP_MINIT_FUNCTION(localeconv);
PHP_MSHUTDOWN_FUNCTION(localeconv);
#endif
#if HAVE_NL_LANGINFO
PHP_MINIT_FUNCTION(nl_langinfo);
#endif

#define strnatcmp(a, b) \
	strnatcmp_ex(a, strlen(a), b, strlen(b), 0)
#define strnatcasecmp(a, b) \
	strnatcmp_ex(a, strlen(a), b, strlen(b), 1)
#define u_strnatcmp(a, b) \
	u_strnatcmp_ex(a, u_strlen(a), b, strlen(b), 0)
#define u_strnatcasecmp(a, b) \
	u_strnatcmp_ex(a, u_strlen(a), b, strlen(b), 1)
PHPAPI int strnatcmp_ex(char const *a, size_t a_len, char const *b, size_t b_len, int fold_case);
PHPAPI int u_strnatcmp_ex(UChar const *a, size_t a_len, UChar const *b, size_t b_len, int fold_case);

#ifdef HAVE_LOCALECONV
PHPAPI struct lconv *localeconv_r(struct lconv *out);
#endif

PHPAPI char *php_strtoupper(char *s, size_t len);
PHPAPI char *php_strtolower(char *s, size_t len);
PHPAPI UChar *php_u_strtoupper(UChar *s, int *len, const char *locale);
PHPAPI UChar *php_u_strtolower(UChar *s, int *len, const char *locale);
PHPAPI char *php_strtr(char *str, int len, char *str_from, char *str_to, int trlen);
PHPAPI UChar *php_u_addslashes(UChar *str, int length, int *new_length, int freeit TSRMLS_DC);
PHPAPI UChar *php_u_addslashes_ex(UChar *str, int length, int *new_length, int freeit, int ignore_sybase TSRMLS_DC);
PHPAPI char *php_addslashes(char *str, int length, int *new_length, int freeit TSRMLS_DC);
PHPAPI char *php_addslashes_ex(char *str, int length, int *new_length, int freeit, int ignore_sybase TSRMLS_DC);
PHPAPI char *php_addcslashes(char *str, int length, int *new_length, int freeit, char *what, int wlength TSRMLS_DC);
PHPAPI void php_stripslashes(char *str, int *len TSRMLS_DC);
PHPAPI void php_u_stripslashes(UChar *str, int *len TSRMLS_DC);
PHPAPI void php_stripcslashes(char *str, int *len);
PHPAPI void php_u_basename(UChar *s, size_t len, UChar *suffix, size_t sufflen, UChar **p_ret, size_t *p_len TSRMLS_DC);
PHPAPI void php_basename(char *s, size_t len, char *suffix, size_t sufflen, char **p_ret, size_t *p_len TSRMLS_DC);
PHPAPI size_t php_u_dirname(UChar *str, size_t len);
PHPAPI size_t php_dirname(char *str, size_t len);
PHPAPI UChar *php_u_stristr(UChar *s, UChar *t, int s_len, int t_len, zend_bool find_first TSRMLS_DC);
PHPAPI char *php_stristr(char *s, char *t, size_t s_len, size_t t_len);
PHPAPI int php_u_strspn(UChar *s1, UChar *s2, UChar *s1_end, UChar *s2_end);
PHPAPI size_t php_strspn(char *s1, char *s2, char *s1_end, char *s2_end);
PHPAPI int php_u_strcspn(UChar *s1, UChar *s2, UChar *s1_end, UChar *s2_end);
PHPAPI char *php_str_to_str_ex(char *haystack, int length, char *needle,
		int needle_len, char *str, int str_len, int *_new_length, int case_sensitivity, int *replace_count);
PHPAPI char *php_str_to_str(char *haystack, int length, char *needle,
		int needle_len, char *str, int str_len, int *_new_length);
PHPAPI UChar *php_u_str_to_str_ex(UChar *haystack, int length,
	UChar *needle, int needle_len, UChar *repl, int repl_len, int *_new_length, int *replace_count);
PHPAPI char *php_trim(char *c, int len, char *what, int what_len, zval *return_value, int mode TSRMLS_DC);
PHPAPI int php_u_strip_tags(UChar *rbuf, int len, int *stateptr, UChar *allow, int allow_len TSRMLS_DC);
PHPAPI size_t php_strip_tags(char *rbuf, int len, int *state, char *allow, int allow_len);
PHPAPI size_t php_strip_tags_ex(char *rbuf, int len, int *stateptr, char *allow, int allow_len, zend_bool allow_tag_spaces);
PHPAPI int php_char_to_str_ex(char *str, uint len, char from, char *to, int to_len, zval *result, int case_sensitivity, int *replace_count);
PHPAPI int php_char_to_str(char *str, uint len, char from, char *to, int to_len, zval *result);
PHPAPI void php_implode(zval *delim, zval *arr, zval *return_value TSRMLS_DC);
PHPAPI void php_explode(char *delim, uint delim_len, char *str, uint str_len, zend_uchar str_type, zval *return_value, int limit);

PHPAPI size_t php_strspn(char *s1, char *s2, char *s1_end, char *s2_end); 
PHPAPI size_t php_strcspn(char *s1, char *s2, char *s1_end, char *s2_end); 

#ifndef HAVE_STRERROR
PHPAPI char *php_strerror(int errnum);
#define strerror php_strerror
#endif

#ifndef HAVE_MBLEN
# define php_mblen(ptr, len) 1
#else
# if defined(_REENTRANT) && defined(HAVE_MBRLEN) && defined(HAVE_MBSTATE_T)
#  define php_mblen(ptr, len) ((ptr) == NULL ? mbsinit(&BG(mblen_state)): (int)mbrlen(ptr, len, &BG(mblen_state)))
# else
#  define php_mblen(ptr, len) mblen(ptr, len)
# endif
#endif

void register_string_constants(INIT_FUNC_ARGS);

#endif /* PHP_STRING_H */
