/* 
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2006 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   |          Rasmus Lerdorf <rasmus@php.net>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "zend_operators.h"
#include "datetime.h"
#include "php_globals.h"

#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdio.h>

char *mon_full_names[] = {
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December"
};

char *mon_short_names[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *day_full_names[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

char *day_short_names[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/* {{{ php_std_date
   Return date string in standard format for http headers */
PHPAPI char *php_std_date(time_t t TSRMLS_DC)
{
	struct tm *tm1, tmbuf;
	char *str;

	tm1 = php_gmtime_r(&t, &tmbuf);
	str = emalloc(81);
	if (PG(y2k_compliance)) {
		snprintf(str, 80, "%s, %02d %s %04d %02d:%02d:%02d GMT",
				day_short_names[tm1->tm_wday],
				tm1->tm_mday,
				mon_short_names[tm1->tm_mon],
				tm1->tm_year + 1900,
				tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	} else {
		snprintf(str, 80, "%s, %02d-%s-%02d %02d:%02d:%02d GMT",
				day_full_names[tm1->tm_wday],
				tm1->tm_mday,
				mon_short_names[tm1->tm_mon],
				((tm1->tm_year) % 100),
				tm1->tm_hour, tm1->tm_min, tm1->tm_sec);
	}
	
	str[79] = 0;
	return (str);
}
/* }}} */


#if HAVE_STRPTIME
#ifndef HAVE_STRPTIME_DECL_FAILS
char *strptime(const char *s, const char *format, struct tm *tm);
#endif

/* {{{ proto string strptime(string timestamp, string format) U
   Parse a time/date generated with strftime() */
PHP_FUNCTION(strptime)
{
	zstr       ts;
	int        ts_length;
	zstr       format;
	int        format_length;
	struct tm  parsed_time;
	char      *unparsed_part;
	zend_uchar type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "TT", 
		&ts, &ts_length, &type, &format, &format_length, &type) == FAILURE) {
		return;
	}

	if (type == IS_UNICODE) {
		char *temp;
		int temp_len;

		if (zend_unicode_to_string(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &temp, &temp_len, ts.u, ts_length TSRMLS_CC) == FAILURE) {
			RETURN_FALSE;
		}
		ts.s = temp;
		ts_length = temp_len;

		if (zend_unicode_to_string(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &temp, &temp_len, format.u, format_length TSRMLS_CC) == FAILURE) {
			RETURN_FALSE;
		}
		format.s = temp;
		format_length = temp_len;
	}

	memset(&parsed_time, 0, sizeof(parsed_time));

	unparsed_part = strptime(ts.s, format.s, &parsed_time);
	if (unparsed_part == NULL) {
		RETURN_FALSE;
	}

	array_init(return_value);
	add_ascii_assoc_long(return_value, "tm_sec",   parsed_time.tm_sec);
	add_ascii_assoc_long(return_value, "tm_min",   parsed_time.tm_min);
	add_ascii_assoc_long(return_value, "tm_hour",  parsed_time.tm_hour);
	add_ascii_assoc_long(return_value, "tm_mday",  parsed_time.tm_mday);
	add_ascii_assoc_long(return_value, "tm_mon",   parsed_time.tm_mon);
	add_ascii_assoc_long(return_value, "tm_year",  parsed_time.tm_year);
	add_ascii_assoc_long(return_value, "tm_wday",  parsed_time.tm_wday);
	add_ascii_assoc_long(return_value, "tm_yday",  parsed_time.tm_yday);
	if (type == IS_UNICODE) {
		UChar *temp;
		int temp_len;

		zend_string_to_unicode(ZEND_U_CONVERTER(UG(runtime_encoding_conv)), &temp, &temp_len, unparsed_part, strlen(unparsed_part) TSRMLS_CC);
		add_ascii_assoc_unicodel(return_value, "unparsed", temp, temp_len, 0);

		efree(ts.s);
		efree(format.s);
	} else {
		add_ascii_assoc_string(return_value, "unparsed", unparsed_part, 1);
	}
}
/* }}} */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
