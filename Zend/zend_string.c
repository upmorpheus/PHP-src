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

#include "zend.h"
#include "zend_globals.h"

ZEND_API zend_string *(*zend_new_interned_string)(zend_string *str TSRMLS_DC);
ZEND_API void (*zend_interned_strings_snapshot)(TSRMLS_D);
ZEND_API void (*zend_interned_strings_restore)(TSRMLS_D);

static zend_string *zend_new_interned_string_int(zend_string *str TSRMLS_DC);
static void zend_interned_strings_snapshot_int(TSRMLS_D);
static void zend_interned_strings_restore_int(TSRMLS_D);

ZEND_API zend_ulong zend_hash_func(const char *str, uint len)
{
	return zend_inline_hash_func(str, len);
}

static void _str_dtor(zval *zv)
{
	zend_string *str = Z_STR_P(zv);
	str->gc.u.v.flags &= ~IS_STR_INTERNED;
	str->gc.refcount = 1;
}

void zend_interned_strings_init(TSRMLS_D)
{
#ifndef ZTS
	zend_string *str;

	zend_hash_init(&CG(interned_strings), 0, NULL, _str_dtor, 1);
	
	CG(interned_strings).nTableMask = CG(interned_strings).nTableSize - 1;
	CG(interned_strings).arData = (Bucket*) pecalloc(CG(interned_strings).nTableSize, sizeof(Bucket), CG(interned_strings).flags & HASH_FLAG_PERSISTENT);
	CG(interned_strings).arHash = (zend_uint*) pecalloc(CG(interned_strings).nTableSize, sizeof(zend_uint), CG(interned_strings).flags & HASH_FLAG_PERSISTENT);
	memset(CG(interned_strings).arHash, INVALID_IDX, CG(interned_strings).nTableSize * sizeof(zend_uint));

	/* interned empty string */
	str = STR_ALLOC(sizeof("")-1, 1);
	str->val[0] = '\000';
	CG(empty_string) = zend_new_interned_string_int(str TSRMLS_CC);
#else
	str = STR_ALLOC(sizeof("")-1, 1);
	str->val[0] = '\000';
	str->gc.u.v.flags |= IS_STR_INTERNED;
	CG(empty_string) = str;
#endif

	zend_new_interned_string = zend_new_interned_string_int;
	zend_interned_strings_snapshot = zend_interned_strings_snapshot_int;
	zend_interned_strings_restore = zend_interned_strings_restore_int;
}

void zend_interned_strings_dtor(TSRMLS_D)
{
#ifndef ZTS
	zend_hash_destroy(&CG(interned_strings));
//???	free(CG(interned_strings).arData);
//???	free(CG(interned_strings).arHash);
#endif
}

static zend_string *zend_new_interned_string_int(zend_string *str TSRMLS_DC)
{
#ifndef ZTS
	ulong h;
	uint nIndex;
	uint idx;
	Bucket *p;

	if (IS_INTERNED(str)) {
		return str;
	}

	h = STR_HASH_VAL(str);
	nIndex = h & CG(interned_strings).nTableMask;
	idx = CG(interned_strings).arHash[nIndex];
	while (idx != INVALID_IDX) {
		p = CG(interned_strings).arData + idx;
		if ((p->h == h) && (p->key->len == str->len)) {
			if (!memcmp(p->key->val, str->val, str->len)) {
				STR_RELEASE(str);
				return p->key;
			}
		}
		idx = p->val.u.next;
	}
	
	str->gc.refcount = 1;
//	str->gc.u.v.type = IS_INTERNED_STRING;
	str->gc.u.v.flags |= IS_STR_INTERNED;

//???	if (CG(interned_strings_top) + ZEND_MM_ALIGNED_SIZE(sizeof(Bucket) + nKeyLength) >=
//???	    CG(interned_strings_end)) {
//???	    /* no memory */
//???		return arKey;
//???	}

//???	info = (zend_string_info*) CG(interned_strings_top);
//???	CG(interned_strings_top) += ZEND_MM_ALIGNED_SIZE(sizeof(zend_string_info) + nKeyLength);

//???	memcpy((char*)(info+1), arKey, nKeyLength);
//???	if (free_src) {
//???		efree((void *)arKey);
//???	}
//???	info->nKeyLength = nKeyLength;
//???	info->h = h;
	
	if (CG(interned_strings).nNumUsed >= CG(interned_strings).nTableSize) {
		if ((CG(interned_strings).nTableSize << 1) > 0) {	/* Let's double the table size */
			Bucket *d = (Bucket *) perealloc_recoverable(CG(interned_strings).arData, (CG(interned_strings).nTableSize << 1) * sizeof(Bucket), CG(interned_strings).flags & HASH_FLAG_PERSISTENT);
			zend_uint *h = (zend_uint *) perealloc_recoverable(CG(interned_strings).arHash, (CG(interned_strings).nTableSize << 1) * sizeof(zend_uint), CG(interned_strings).flags & HASH_FLAG_PERSISTENT);

			if (d && h) {
				HANDLE_BLOCK_INTERRUPTIONS();
				CG(interned_strings).arData = d;
				CG(interned_strings).arHash = h;
				CG(interned_strings).nTableSize = (CG(interned_strings).nTableSize << 1);
				CG(interned_strings).nTableMask = CG(interned_strings).nTableSize - 1;
				zend_hash_rehash(&CG(interned_strings));
				HANDLE_UNBLOCK_INTERRUPTIONS();
			}
		}
	}

	HANDLE_BLOCK_INTERRUPTIONS();
	
	idx = CG(interned_strings).nNumUsed++;
	CG(interned_strings).nNumOfElements++;
	p = CG(interned_strings).arData + idx;
	p->h = h;
	p->key = str;
	Z_STR(p->val) = str;
	Z_TYPE(p->val) = IS_STRING;
	nIndex = h & CG(interned_strings).nTableMask;
	p->val.u.next = CG(interned_strings).arHash[nIndex];
	CG(interned_strings).arHash[nIndex] = idx;
		
	HANDLE_UNBLOCK_INTERRUPTIONS();

	return str;
#else
	return str;
#endif
}

static void zend_interned_strings_snapshot_int(TSRMLS_D)
{
#ifndef ZTS
	uint idx;
	Bucket *p;

	idx = CG(interned_strings).nNumUsed;
	while (idx > 0) {
		idx--;
		p = CG(interned_strings).arData + idx;
		ZEND_ASSERT(p->key->gc.u.v.flags & IS_STR_PERSISTENT);
		p->key->gc.u.v.flags |= IS_STR_PERMANENT;
	}
#endif
}

static void zend_interned_strings_restore_int(TSRMLS_D)
{
#ifndef ZTS
	uint nIndex;
	uint idx;
	Bucket *p;

	idx = CG(interned_strings).nNumUsed;
	while (idx > 0) {
		idx--;
		p = CG(interned_strings).arData + idx;
		if (p->key->gc.u.v.flags & IS_STR_PERMANENT) break;
		CG(interned_strings).nNumUsed--;
		CG(interned_strings).nNumOfElements--;

		p->key->gc.u.v.flags &= ~IS_STR_INTERNED;
		p->key->gc.refcount = 1;
		STR_FREE(p->key);

		nIndex = p->h & CG(interned_strings).nTableMask;
		if (CG(interned_strings).arHash[nIndex] == idx) {
			CG(interned_strings).arHash[nIndex] = p->val.u.next;
		} else {
			uint prev = CG(interned_strings).arHash[nIndex];
			while (CG(interned_strings).arData[prev].val.u.next != idx) {
				prev = CG(interned_strings).arData[prev].val.u.next;
 			}
			CG(interned_strings).arData[prev].val.u.next = p->val.u.next;
 		}
 	}
#endif
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
