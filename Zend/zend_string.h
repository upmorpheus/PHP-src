/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   +----------------------------------------------------------------------+
*/

/* $Id: $ */

#ifndef ZEND_STRING_H
#define ZEND_STRING_H

#include "zend.h"

BEGIN_EXTERN_C()

ZEND_API extern zend_string *(*zend_new_interned_string)(zend_string *str TSRMLS_DC);
ZEND_API extern void (*zend_interned_strings_snapshot)(TSRMLS_D);
ZEND_API extern void (*zend_interned_strings_restore)(TSRMLS_D);

ZEND_API zend_ulong zend_hash_func(const char *str, uint len);
void zend_interned_strings_init(TSRMLS_D);
void zend_interned_strings_dtor(TSRMLS_D);

END_EXTERN_C()

#ifndef ZTS
# define IS_INTERNED(s)					((s)->gc.u.v.flags & IS_STR_INTERNED)
#else
# define IS_INTERNED(s)					0
#endif

#define STR_HASH_VAL(s)					zend_str_hash_val(s)
#define STR_FORGET_HASH_VAL(s)			zend_str_forget_hash_val(s)

#define STR_REFCOUNT(s)					zend_str_refcount(s)
#define STR_ADDREF(s)					zend_str_addref(s)
#define STR_DELREF(s)					zend_str_delref(s)
#define STR_ALLOC(len, persistent)		zend_str_alloc(len, persistent)
#define STR_INIT(str, len, persistent)	zend_str_init(str, len, persistent)
#define STR_COPY(s)						zend_str_copy(s)
#define STR_DUP(s, persistent)			zend_str_dup(s, persistent)
#define STR_EREALLOC(s, len)			zend_str_erealloc(s, len)
#define STR_FREE(s)						zend_str_free(s)
#define STR_RELEASE(s)					zend_str_release(s)
#define STR_EMPTY_ALLOC()				CG(empty_string)

static zend_always_inline zend_ulong zend_str_hash_val(zend_string *s)
{
	if (!s->h) {
		s->h = zend_hash_func(s->val, s->len + 1);
	}
	return s->h;
}

static zend_always_inline void zend_str_forget_hash_val(zend_string *s)
{
	s->h = 0;
}

static zend_always_inline zend_uint zend_str_refcount(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		return s->gc.refcount;
	}
	return 1;
}

static zend_always_inline zend_uint zend_str_addref(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		return ++s->gc.refcount;
	}
	return 1;
}

static zend_always_inline zend_uint zend_str_delref(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		return --s->gc.refcount;
	}
	return 1;
}

static zend_always_inline zend_string *zend_str_alloc(int len, int persistent)
{
	zend_string *ret = pemalloc(sizeof(zend_string) + len, persistent);

	ret->gc.refcount = 1;
	ret->gc.u.v.type = IS_STRING;
	ret->gc.u.v.flags = (persistent ? IS_STR_PERSISTENT : 0);
	ret->h = 0;
	ret->len = len;
	return ret;
}

static zend_always_inline zend_string *zend_str_init(const char *str, int len, int persistent)
{
	zend_string *ret = STR_ALLOC(len, persistent);

	memcpy(ret->val, str, len + 1);
	return ret;
}

static zend_always_inline zend_string *zend_str_copy(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		STR_ADDREF(s);
	}
	return s;
}

static zend_always_inline zend_string *zend_str_dup(zend_string *s, int persistent)
{
	if (IS_INTERNED(s)) {
		return s;
	} else {
		return STR_INIT(s->val, s->len, persistent);
	}
}

static zend_always_inline zend_string *zend_str_erealloc(zend_string *s, int len)
{
	zend_string *ret;

	if (IS_INTERNED(s)) {
		ret = STR_ALLOC(len, 0);
		memcpy(ret->val, s->val, (len > s->len ? s->len : len) + 1);
	} else if (STR_REFCOUNT(s) == 1) {
		ret = erealloc(s, sizeof(zend_string) + len);
		ret->len = len;
		STR_FORGET_HASH_VAL(ret);
	} else {
		ret = STR_ALLOC(len, 0);
		memcpy(ret->val, s->val, (len > s->len ? s->len : len) + 1);
		STR_DELREF(s);
	}
	return ret;
}

static zend_always_inline void zend_str_free(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		pefree(s, s->gc.u.v.flags & IS_STR_PERSISTENT);
	}
}

static zend_always_inline void zend_str_release(zend_string *s)
{
	if (!IS_INTERNED(s)) {
		if (STR_DELREF(s) == 0) {
			STR_FREE(s);
		}
	}
}

/*
 * DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 *
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp.lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + str[i]''. This is one of the best
 * known hash functions for strings. Because it is both computed very
 * fast and distributes very well.
 *
 * The magic of number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an explanation: if one experimentally tests all
 * multipliers between 1 and 256 (as RSE did now) one detects that even
 * numbers are not useable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%. 
 *
 * If one compares the Chi^2 values of the variants, the number 33 not
 * even has the best value. But the number 33 and a few other equally
 * good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
 * advantage to the remaining numbers in the large set of possible
 * multipliers: their multiply operation can be replaced by a faster
 * operation based on just one shift plus either a single addition
 * or subtraction operation. And because a hash function has to both
 * distribute good _and_ has to be very fast to compute, those few
 * numbers should be preferred and seems to be the reason why Daniel J.
 * Bernstein also preferred it.
 *
 *
 *                  -- Ralf S. Engelschall <rse@engelschall.com>
 */

static inline ulong zend_inline_hash_func(const char *str, uint len)
{
	register ulong hash = 5381;

	/* variant with the hash unrolled eight times */
	for (; len >= 8; len -= 8) {
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
		hash = ((hash << 5) + hash) + *str++;
	}
	switch (len) {
		case 7: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 6: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 5: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 4: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 3: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 2: hash = ((hash << 5) + hash) + *str++; /* fallthrough... */
		case 1: hash = ((hash << 5) + hash) + *str++; break;
		case 0: break;
EMPTY_SWITCH_DEFAULT_CASE()
	}
	return hash;
}

//???#define str_estrndup(str, len) (IS_INTERNED(str) ? (str) : estrndup((str), (len)))
//???#define str_strndup(str, len)	(IS_INTERNED(str) ? (str) : zend_strndup((str), (len)));

#endif /* ZEND_STRING_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
