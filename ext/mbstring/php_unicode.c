/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2017 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Wez Furlong (wez@thebrainroom.com)                           |
   +----------------------------------------------------------------------+

	Based on code from ucdata-2.5, which has the following Copyright:

	Copyright 2001 Computing Research Labs, New Mexico State University

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"

#if HAVE_MBSTRING

/* include case folding data generated from the official UnicodeData.txt file */
#include "mbstring.h"
#include "php_unicode.h"
#include "unicode_data.h"
#include "libmbfl/filters/mbfilter_ucs4.h"

ZEND_EXTERN_MODULE_GLOBALS(mbstring)

static int prop_lookup(unsigned long code, unsigned long n)
{
	long l, r, m;

	/*
	 * There is an extra node on the end of the offsets to allow this routine
	 * to work right.  If the index is 0xffff, then there are no nodes for the
	 * property.
	 */
	if ((l = _ucprop_offsets[n]) == 0xffff)
		return 0;

	/*
	 * Locate the next offset that is not 0xffff.  The sentinel at the end of
	 * the array is the max index value.
	 */
	for (m = 1; n + m < _ucprop_size && _ucprop_offsets[n + m] == 0xffff; m++)
		;

	r = _ucprop_offsets[n + m] - 1;

	while (l <= r) {
		/*
		 * Determine a "mid" point and adjust to make sure the mid point is at
		 * the beginning of a range pair.
		 */
		m = (l + r) >> 1;
		m -= (m & 1);
		if (code > _ucprop_ranges[m + 1])
			l = m + 2;
		else if (code < _ucprop_ranges[m])
			r = m - 2;
		else if (code >= _ucprop_ranges[m] && code <= _ucprop_ranges[m + 1])
			return 1;
	}
	return 0;

}

MBSTRING_API int php_unicode_is_prop1(unsigned long code, int prop)
{
	return prop_lookup(code, prop);
}

MBSTRING_API int php_unicode_is_prop(unsigned long code, ...)
{
	int result = 0;
	va_list va;
	va_start(va, code);

	while (1) {
		int prop = va_arg(va, int);
		if (prop < 0) {
			break;
		}

		if (prop_lookup(code, prop)) {
			result = 1;
			break;
		}
	}

	va_end(va);
	return result;
}

static unsigned long case_lookup(unsigned long code, long l, long r, int field)
{
	long m;

	/*
	 * Do the binary search.
	 */
	while (l <= r) {
		/*
		 * Determine a "mid" point and adjust to make sure the mid point is at
		 * the beginning of a case mapping triple.
		 */
		m = (l + r) >> 1;
		m -= (m % 3);
		if (code > _uccase_map[m])
			l = m + 3;
		else if (code < _uccase_map[m])
			r = m - 3;
		else if (code == _uccase_map[m])
			return _uccase_map[m + field];
	}

	return code;
}

MBSTRING_API unsigned long php_unicode_toupper(unsigned long code, enum mbfl_no_encoding enc)
{
	int field;
	long l, r;

	if (code < 0x80) {
		/* Fast path for ASCII */
		if (code >= 0x61 && code <= 0x7A) {
			if (enc == mbfl_no_encoding_8859_9 && code == 0x0069L) {
				return 0x0130L;
			}
			return code - 0x20;
		}
		return code;
	}

	if (php_unicode_is_upper(code))
		return code;

	if (php_unicode_is_lower(code)) {
		/*
		 * The character is lower case.
		 */
		field = 2;
		l = _uccase_len[0];
		r = (l + _uccase_len[1]) - 3;
	} else {
		/*
		 * The character is title case.
		 */
		field = 1;
		l = _uccase_len[0] + _uccase_len[1];
		r = _uccase_size - 3;
	}
	return case_lookup(code, l, r, field);
}

MBSTRING_API unsigned long php_unicode_tolower(unsigned long code, enum mbfl_no_encoding enc)
{
	int field;
	long l, r;

	if (code < 0x80) {
		/* Fast path for ASCII */
		if (code >= 0x41 && code <= 0x5A) {
			if (enc == mbfl_no_encoding_8859_9 && code == 0x0049L) {
				return 0x0131L;
			}
			return code + 0x20;
		}
		return code;
	}

	if (php_unicode_is_lower(code))
		return code;

	if (php_unicode_is_upper(code)) {
		/*
		 * The character is upper case.
		 */
		field = 1;
		l = 0;
		r = _uccase_len[0] - 3;
	} else {
		/*
		 * The character is title case.
		 */
		field = 2;
		l = _uccase_len[0] + _uccase_len[1];
		r = _uccase_size - 3;
	}
	return case_lookup(code, l, r, field);
}

MBSTRING_API unsigned long php_unicode_totitle(unsigned long code, enum mbfl_no_encoding enc)
{
	int field;
	long l, r;

	if (php_unicode_is_title(code))
		return code;

	/*
	 * The offset will always be the same for converting to title case.
	 */
	field = 2;

	if (php_unicode_is_upper(code)) {
		/*
		 * The character is upper case.
		 */
		l = 0;
		r = _uccase_len[0] - 3;
	} else {
		/*
		 * The character is lower case.
		 */
		l = _uccase_len[0];
		r = (l + _uccase_len[1]) - 3;
	}
	return case_lookup(code, l, r, field);

}


#define BE_ARY_TO_UINT32(ptr) (\
	((unsigned char*)(ptr))[0]<<24 |\
	((unsigned char*)(ptr))[1]<<16 |\
	((unsigned char*)(ptr))[2]<< 8 |\
	((unsigned char*)(ptr))[3] )

#define UINT32_TO_BE_ARY(ptr,val) { \
	unsigned int v = val; \
	((unsigned char*)(ptr))[0] = (v>>24) & 0xff,\
	((unsigned char*)(ptr))[1] = (v>>16) & 0xff,\
	((unsigned char*)(ptr))[2] = (v>> 8) & 0xff,\
	((unsigned char*)(ptr))[3] = (v    ) & 0xff;\
}

MBSTRING_API char *php_unicode_convert_case(
		int case_mode, const char *srcstr, size_t srclen, size_t *ret_len,
		const mbfl_encoding *src_encoding)
{
	char *unicode, *newstr;
	size_t unicode_len;
	unsigned char *unicode_ptr;
	size_t i;
	enum mbfl_no_encoding src_no_encoding = src_encoding->no_encoding;

	unicode = php_mb_convert_encoding_ex(srcstr, srclen, &mbfl_encoding_ucs4be, src_encoding, &unicode_len);
	if (unicode == NULL)
		return NULL;

	unicode_ptr = (unsigned char *)unicode;

	switch(case_mode) {
		case PHP_UNICODE_CASE_UPPER:
			for (i = 0; i < unicode_len; i+=4) {
				UINT32_TO_BE_ARY(&unicode_ptr[i],
					php_unicode_toupper(BE_ARY_TO_UINT32(&unicode_ptr[i]), src_no_encoding));
			}
			break;

		case PHP_UNICODE_CASE_LOWER:
			for (i = 0; i < unicode_len; i+=4) {
				UINT32_TO_BE_ARY(&unicode_ptr[i],
					php_unicode_tolower(BE_ARY_TO_UINT32(&unicode_ptr[i]), src_no_encoding));
			}
			break;

		case PHP_UNICODE_CASE_TITLE: {
			int mode = 0;

			for (i = 0; i < unicode_len; i+=4) {
				int res = php_unicode_is_prop(
					BE_ARY_TO_UINT32(&unicode_ptr[i]),
					UC_MN, UC_ME, UC_CF, UC_LM, UC_SK, UC_LU, UC_LL, UC_LT, UC_PO, UC_OS, -1);
				if (mode) {
					if (res) {
						UINT32_TO_BE_ARY(&unicode_ptr[i],
							php_unicode_tolower(BE_ARY_TO_UINT32(&unicode_ptr[i]), src_no_encoding));
					} else {
						mode = 0;
					}
				} else {
					if (res) {
						mode = 1;
						UINT32_TO_BE_ARY(&unicode_ptr[i],
							php_unicode_totitle(BE_ARY_TO_UINT32(&unicode_ptr[i]), src_no_encoding));
					}
				}
			}
		} break;

	}

	newstr = php_mb_convert_encoding_ex(
		unicode, unicode_len, src_encoding, &mbfl_encoding_ucs4be, ret_len);
	efree(unicode);

	return newstr;
}


#endif /* HAVE_MBSTRING */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
