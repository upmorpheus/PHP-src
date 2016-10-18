/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2016 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig S�ther Bakken <ssb@php.net>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <errno.h>

#include <stdio.h>
#ifdef PHP_WIN32
#include "win32/time.h"
#else
#include <sys/time.h>
#endif

#include "php_random.h"
#include "uniqid.h"

#define PHP_UNIQID_ENTROPY_LEN 10

/* {{{ proto string uniqid([string prefix [, bool more_entropy]])
   Generates a unique ID */
#ifdef HAVE_GETTIMEOFDAY
PHP_FUNCTION(uniqid)
{
	char *prefix = "";
#if defined(__CYGWIN__)
	zend_bool more_entropy = 1;
#else
	zend_bool more_entropy = 0;
#endif
	zend_string *uniqid;
	int sec, usec;
	size_t prefix_len = 0;
	struct timeval tv;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sb", &prefix, &prefix_len,
							  &more_entropy)) {
		return;
	}

#if HAVE_USLEEP && !defined(PHP_WIN32)
	if (!more_entropy) {
#if defined(__CYGWIN__)
		php_error_docref(NULL, E_WARNING, "You must use 'more entropy' under CYGWIN");
		RETURN_FALSE;
#else
		usleep(1);
#endif
	}
#endif
	gettimeofday((struct timeval *) &tv, (struct timezone *) NULL);
	sec = (int) tv.tv_sec;
	usec = (int) (tv.tv_usec % 0x100000);

	/* The max value usec can have is 0xF423F, so we use only five hex
	 * digits for usecs.
	 */
	if (more_entropy) {
		int i;
		unsigned char c, entropy[PHP_UNIQID_ENTROPY_LEN+1];

		for(i = 0; i < PHP_UNIQID_ENTROPY_LEN;) {
			php_random_bytes_throw(&c, sizeof(c));
			/* Avoid modulo bias */
			if (c > 249) {
				continue;
			}
			entropy[i] = c % 10 + '0';
			i++;
		}
		/* Set . for compatibility */
		entropy[1] = '.';
		entropy[PHP_UNIQID_ENTROPY_LEN] = '\0';
		uniqid = strpprintf(0, "%s%08x%05x%s", prefix, sec, usec, entropy);
	} else {
		uniqid = strpprintf(0, "%s%08x%05x", prefix, sec, usec);
	}

	RETURN_STR(uniqid);
}
#endif
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
