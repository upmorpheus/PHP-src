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
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef ZEND_HASH_H
#define ZEND_HASH_H

#include <sys/types.h>
#include "zend.h"

#define HASH_KEY_IS_STRING 1
#define HASH_KEY_IS_LONG 2
#define HASH_KEY_NON_EXISTENT 3
#define HASH_KEY_NON_EXISTANT HASH_KEY_NON_EXISTENT /* Keeping old define (with typo) for backward compatibility */

#define HASH_UPDATE 			(1<<0)
#define HASH_ADD				(1<<1)
#define HASH_NEXT_INSERT		(1<<2)
#define HASH_UPDATE_INDIRECT	(1<<3)
#define HASH_ADD_NEW			(1<<4)

#define INVALID_IDX ((uint)-1)

#define HASH_FLAG_PERSISTENT       (1<<0)
#define HASH_FLAG_APPLY_PROTECTION (1<<1)
#define HASH_FLAG_PACKED           (1<<2)

#define HASH_MASK_CONSISTENCY      0x60

typedef struct _zend_hash_key {
	ulong        h;
	zend_string *key;
} zend_hash_key;

typedef zend_bool (*merge_checker_func_t)(HashTable *target_ht, zval *source_data, zend_hash_key *hash_key, void *pParam);

typedef uint HashPosition;

BEGIN_EXTERN_C()

/* startup/shutdown */
ZEND_API void _zend_hash_init(HashTable *ht, uint nSize, dtor_func_t pDestructor, zend_bool persistent ZEND_FILE_LINE_DC);
ZEND_API void _zend_hash_init_ex(HashTable *ht, uint nSize, dtor_func_t pDestructor, zend_bool persistent, zend_bool bApplyProtection ZEND_FILE_LINE_DC);
ZEND_API void zend_hash_destroy(HashTable *ht);
ZEND_API void zend_hash_clean(HashTable *ht);
#define zend_hash_init(ht, nSize, pHashFunction, pDestructor, persistent)						_zend_hash_init((ht), (nSize), (pDestructor), (persistent) ZEND_FILE_LINE_CC)
#define zend_hash_init_ex(ht, nSize, pHashFunction, pDestructor, persistent, bApplyProtection)		_zend_hash_init_ex((ht), (nSize), (pDestructor), (persistent), (bApplyProtection) ZEND_FILE_LINE_CC)

ZEND_API void zend_hash_real_init(HashTable *ht, int packed);
ZEND_API void zend_hash_packed_to_hash(HashTable *ht);
ZEND_API void zend_hash_to_packed(HashTable *ht);

/* additions/updates/changes */
ZEND_API zval *_zend_hash_add_or_update(HashTable *ht, zend_string *key, zval *pData, int flag ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_update(HashTable *ht, zend_string *key,zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_update_ind(HashTable *ht, zend_string *key,zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_add(HashTable *ht, zend_string *key,zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_add_new(HashTable *ht, zend_string *key,zval *pData ZEND_FILE_LINE_DC);

#define zend_hash_update(ht, key, pData) \
		_zend_hash_update(ht, key, pData ZEND_FILE_LINE_CC)
#define zend_hash_update_ind(ht, key, pData) \
		_zend_hash_update_ind(ht, key, pData ZEND_FILE_LINE_CC)
#define zend_hash_add(ht, key, pData) \
		_zend_hash_add(ht, key, pData ZEND_FILE_LINE_CC)
#define zend_hash_add_new(ht, key, pData) \
		_zend_hash_add_new(ht, key, pData ZEND_FILE_LINE_CC)

ZEND_API zval *_zend_hash_str_add_or_update(HashTable *ht, const char *key, int len, zval *pData, int flag ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_str_update(HashTable *ht, const char *key, int len, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_str_update_ind(HashTable *ht, const char *key, int len, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_str_add(HashTable *ht, const char *key, int len, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_str_add_new(HashTable *ht, const char *key, int len, zval *pData ZEND_FILE_LINE_DC);

#define zend_hash_str_update(ht, key, len, pData) \
		_zend_hash_str_update(ht, key, len, pData ZEND_FILE_LINE_CC)
#define zend_hash_str_update_ind(ht, key, len, pData) \
		_zend_hash_str_update_ind(ht, key, len, pData ZEND_FILE_LINE_CC)
#define zend_hash_str_add(ht, key, len, pData) \
		_zend_hash_str_add(ht, key, len, pData ZEND_FILE_LINE_CC)
#define zend_hash_str_add_new(ht, key, len, pData) \
		_zend_hash_str_add_new(ht, key, len, pData ZEND_FILE_LINE_CC)

ZEND_API zval *_zend_hash_index_update_or_next_insert(HashTable *ht, ulong h, zval *pData, int flag ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_index_add(HashTable *ht, ulong h, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_index_add_new(HashTable *ht, ulong h, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_index_update(HashTable *ht, ulong h, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_next_index_insert(HashTable *ht, zval *pData ZEND_FILE_LINE_DC);
ZEND_API zval *_zend_hash_next_index_insert_new(HashTable *ht, zval *pData ZEND_FILE_LINE_DC);

#define zend_hash_index_add(ht, h, pData) \
		_zend_hash_index_add(ht, h, pData ZEND_FILE_LINE_CC)
#define zend_hash_index_add_new(ht, h, pData) \
		_zend_hash_index_add_new(ht, h, pData ZEND_FILE_LINE_CC)
#define zend_hash_index_update(ht, h, pData) \
		_zend_hash_index_update(ht, h, pData ZEND_FILE_LINE_CC)
#define zend_hash_next_index_insert(ht, pData) \
		_zend_hash_next_index_insert(ht, pData ZEND_FILE_LINE_CC)
#define zend_hash_next_index_insert_new(ht, pData) \
		_zend_hash_next_index_insert_new(ht, pData ZEND_FILE_LINE_CC)

ZEND_API zval *zend_hash_index_add_empty_element(HashTable *ht, ulong h);
ZEND_API zval *zend_hash_add_empty_element(HashTable *ht, zend_string *key);
ZEND_API zval *zend_hash_str_add_empty_element(HashTable *ht, const char *key, int len);

#define ZEND_HASH_APPLY_KEEP				0
#define ZEND_HASH_APPLY_REMOVE				1<<0
#define ZEND_HASH_APPLY_STOP				1<<1

typedef int (*apply_func_t)(zval *pDest TSRMLS_DC);
typedef int (*apply_func_arg_t)(zval *pDest, void *argument TSRMLS_DC);
typedef int (*apply_func_args_t)(zval *pDest TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);

ZEND_API void zend_hash_graceful_destroy(HashTable *ht);
ZEND_API void zend_hash_graceful_reverse_destroy(HashTable *ht);
ZEND_API void zend_hash_apply(HashTable *ht, apply_func_t apply_func TSRMLS_DC);
ZEND_API void zend_hash_apply_with_argument(HashTable *ht, apply_func_arg_t apply_func, void * TSRMLS_DC);
ZEND_API void zend_hash_apply_with_arguments(HashTable *ht TSRMLS_DC, apply_func_args_t apply_func, int, ...);

/* This function should be used with special care (in other words,
 * it should usually not be used).  When used with the ZEND_HASH_APPLY_STOP
 * return value, it assumes things about the order of the elements in the hash.
 * Also, it does not provide the same kind of reentrancy protection that
 * the standard apply functions do.
 */
ZEND_API void zend_hash_reverse_apply(HashTable *ht, apply_func_t apply_func TSRMLS_DC);


/* Deletes */
ZEND_API int zend_hash_del(HashTable *ht, zend_string *key);
ZEND_API int zend_hash_del_ind(HashTable *ht, zend_string *key);
ZEND_API int zend_hash_str_del(HashTable *ht, const char *key, int len);
ZEND_API int zend_hash_str_del_ind(HashTable *ht, const char *key, int len);
ZEND_API int zend_hash_index_del(HashTable *ht, ulong h);

/* Data retreival */
ZEND_API zval *zend_hash_find(const HashTable *ht, zend_string *key);
ZEND_API zval *zend_hash_str_find(const HashTable *ht, const char *key, int len);
ZEND_API zval *zend_hash_index_find(const HashTable *ht, ulong h);

/* Misc */
ZEND_API int zend_hash_exists(const HashTable *ht, zend_string *key);
ZEND_API int zend_hash_str_exists(const HashTable *ht, const char *str, int len);
ZEND_API int zend_hash_index_exists(const HashTable *ht, ulong h);

/* traversing */
#define zend_hash_has_more_elements_ex(ht, pos) \
	(zend_hash_get_current_key_type_ex(ht, pos) == HASH_KEY_NON_EXISTENT ? FAILURE : SUCCESS)
ZEND_API int zend_hash_move_forward_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_move_backwards_ex(HashTable *ht, HashPosition *pos);
ZEND_API int zend_hash_get_current_key_ex(const HashTable *ht, zend_string **str_index, ulong *num_index, zend_bool duplicate, HashPosition *pos);
ZEND_API void zend_hash_get_current_key_zval_ex(const HashTable *ht, zval *key, HashPosition *pos);
ZEND_API int zend_hash_get_current_key_type_ex(HashTable *ht, HashPosition *pos);
ZEND_API zval *zend_hash_get_current_data_ex(HashTable *ht, HashPosition *pos);
ZEND_API void zend_hash_internal_pointer_reset_ex(HashTable *ht, HashPosition *pos);
ZEND_API void zend_hash_internal_pointer_end_ex(HashTable *ht, HashPosition *pos);

typedef struct _HashPointer {
	HashPosition pos;
	HashTable *ht;
	ulong h;
} HashPointer;

ZEND_API int zend_hash_get_pointer(const HashTable *ht, HashPointer *ptr);
ZEND_API int zend_hash_set_pointer(HashTable *ht, const HashPointer *ptr);

#define zend_hash_has_more_elements(ht) \
	zend_hash_has_more_elements_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_move_forward(ht) \
	zend_hash_move_forward_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_move_backwards(ht) \
	zend_hash_move_backwards_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_get_current_key(ht, str_index, num_index, duplicate) \
	zend_hash_get_current_key_ex(ht, str_index, num_index, duplicate, &(ht)->nInternalPointer)
#define zend_hash_get_current_key_zval(ht, key) \
	zend_hash_get_current_key_zval_ex(ht, key, &(ht)->nInternalPointer)
#define zend_hash_get_current_key_type(ht) \
	zend_hash_get_current_key_type_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_get_current_data(ht) \
	zend_hash_get_current_data_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_internal_pointer_reset(ht) \
	zend_hash_internal_pointer_reset_ex(ht, &(ht)->nInternalPointer)
#define zend_hash_internal_pointer_end(ht) \
	zend_hash_internal_pointer_end_ex(ht, &(ht)->nInternalPointer)

/* Copying, merging and sorting */
ZEND_API void zend_hash_copy(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor);
ZEND_API void _zend_hash_merge(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, int overwrite ZEND_FILE_LINE_DC);
ZEND_API void zend_hash_merge_ex(HashTable *target, HashTable *source, copy_ctor_func_t pCopyConstructor, merge_checker_func_t pMergeSource, void *pParam);
ZEND_API int zend_hash_sort(HashTable *ht, sort_func_t sort_func, compare_func_t compare_func, int renumber TSRMLS_DC);
ZEND_API int zend_hash_compare(HashTable *ht1, HashTable *ht2, compare_func_t compar, zend_bool ordered TSRMLS_DC);
ZEND_API zval *zend_hash_minmax(const HashTable *ht, compare_func_t compar, int flag TSRMLS_DC);

#define zend_hash_merge(target, source, pCopyConstructor, overwrite)					\
	_zend_hash_merge(target, source, pCopyConstructor, overwrite ZEND_FILE_LINE_CC)

#define zend_hash_num_elements(ht) \
	(ht)->nNumOfElements

#define zend_hash_next_free_element(ht) \
	(ht)->nNextFreeElement

ZEND_API int zend_hash_rehash(HashTable *ht);

ZEND_API void zend_array_dup(HashTable *target, HashTable *source);

#if ZEND_DEBUG
/* debug */
void zend_hash_display_pListTail(const HashTable *ht);
void zend_hash_display(const HashTable *ht);
#endif

END_EXTERN_C()

#define ZEND_INIT_SYMTABLE(ht)								\
	ZEND_INIT_SYMTABLE_EX(ht, 8, 0)

#define ZEND_INIT_SYMTABLE_EX(ht, n, persistent)			\
	zend_hash_init(ht, n, NULL, ZVAL_PTR_DTOR, persistent)

static inline int _zend_handle_numeric_str(const char *key, int length, ulong *idx)
{
	register const char *tmp = key;
	const char *end;

	if (*tmp > '9') {
		return 0;
	} else if (*tmp < '0') {
		if (*tmp != '-') {
			return 0;
		}
		tmp++;
		if (*tmp > '9' || *tmp < '0') {
			return 0;
		}
	}

	/* possibly a numeric index */
	end = key + length;

	if ((*end != '\0') /* not a null terminated string */
	 || (*tmp == '0' && length > 1) /* numbers with leading zeros */
	 || (end - tmp > MAX_LENGTH_OF_LONG - 1) /* number too long */
	 || (SIZEOF_LONG == 4 &&
	     end - tmp == MAX_LENGTH_OF_LONG - 1 &&
	     *tmp > '2')) { /* overflow */
		return 0;
	}
	*idx = (*tmp - '0');
	while (1) {
		++tmp;
		if (tmp == end) {
			if (*key == '-') {
				if (*idx-1 > LONG_MAX) { /* overflow */
					return 0;
				}
				*idx = 0 - *idx;
			} else if (*idx > LONG_MAX) { /* overflow */
				return 0;
			}
			return 1;
		}
		if (*tmp <= '9' && *tmp >= '0') {
			*idx = (*idx * 10) + (*tmp - '0');
		} else {
			return 0;
		}
	}
}

#define ZEND_HANDLE_NUMERIC_STR(key, length, idx) \
	_zend_handle_numeric_str(key, length, &idx)

#define ZEND_HANDLE_NUMERIC(key, idx) \
	_zend_handle_numeric_str((key)->val, (key)->len, &idx)


static inline zval *zend_hash_find_ind(const HashTable *ht, zend_string *key)
{
	zval *zv;

	zv = zend_hash_find(ht, key);
	return (zv && Z_TYPE_P(zv) == IS_INDIRECT) ? Z_INDIRECT_P(zv) : zv;
}


static inline int zend_hash_exists_ind(const HashTable *ht, zend_string *key)
{
	zval *zv;

	zv = zend_hash_find(ht, key);
	return zv && (Z_TYPE_P(zv) != IS_INDIRECT ||
			Z_TYPE_P(Z_INDIRECT_P(zv)) != IS_UNDEF);
}


static inline zval *zend_hash_str_find_ind(const HashTable *ht, const char *str, int len)
{
	zval *zv;

	zv = zend_hash_str_find(ht, str, len);
	return (zv && Z_TYPE_P(zv) == IS_INDIRECT) ? Z_INDIRECT_P(zv) : zv;
}


static inline zval *zend_symtable_update(HashTable *ht, zend_string *key, zval *pData)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_update(ht, idx, pData);
	} else {
		return zend_hash_update(ht, key, pData);
	}
}


static inline zval *zend_symtable_update_ind(HashTable *ht, zend_string *key, zval *pData)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_update(ht, idx, pData);
	} else {
		return zend_hash_update_ind(ht, key, pData);
	}
}


static inline int zend_symtable_del(HashTable *ht, zend_string *key)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_del(ht, idx);
	} else {
		return zend_hash_del(ht, key);
	}
}


static inline int zend_symtable_del_ind(HashTable *ht, zend_string *key)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_del(ht, idx);
	} else {
		return zend_hash_del_ind(ht, key);
	}
}


static inline zval *zend_symtable_find(const HashTable *ht, zend_string *key)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_find(ht, idx);
	} else {
		return zend_hash_find(ht, key);
	}
}


static inline zval *zend_symtable_find_ind(const HashTable *ht, zend_string *key)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_find(ht, idx);
	} else {
		return zend_hash_find_ind(ht, key);
	}
}


static inline int zend_symtable_exists(HashTable *ht, zend_string *key)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC(key, idx)) {
		return zend_hash_index_exists(ht, idx);
	} else {
		return zend_hash_exists(ht, key);
	}
}


static inline zval *zend_symtable_str_update(HashTable *ht, const char *str, int len, zval *pData)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_update(ht, idx, pData);
	} else {
		return zend_hash_str_update(ht, str, len, pData);
	}
}


static inline zval *zend_symtable_str_update_ind(HashTable *ht, const char *str, int len, zval *pData)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_update(ht, idx, pData);
	} else {
		return zend_hash_str_update_ind(ht, str, len, pData);
	}
}


static inline int zend_symtable_str_del(HashTable *ht, const char *str, int len)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_del(ht, idx);
	} else {
		return zend_hash_str_del(ht, str, len);
	}
}


static inline int zend_symtable_str_del_ind(HashTable *ht, const char *str, int len)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_del(ht, idx);
	} else {
		return zend_hash_str_del_ind(ht, str, len);
	}
}


static inline zval *zend_symtable_str_find(HashTable *ht, const char *str, int len)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_find(ht, idx);
	} else {
		return zend_hash_str_find(ht, str, len);
	}
}


static inline int zend_symtable_str_exists(HashTable *ht, const char *str, int len)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_exists(ht, idx);
	} else {
		return zend_hash_str_exists(ht, str, len);
	}
}

static inline void *zend_hash_add_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_add(ht, key, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_add_new_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_add_new(ht, key, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_str_add_ptr(HashTable *ht, const char *str, int len, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_str_add(ht, str, len, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_update_ptr(HashTable *ht, zend_string *key, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_update(ht, key, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_str_update_ptr(HashTable *ht, const char *str, int len, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_str_update(ht, str, len, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_add_mem(HashTable *ht, zend_string *key, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	if ((zv = zend_hash_add(ht, key, &tmp))) {
		Z_PTR_P(zv) = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
		memcpy(Z_PTR_P(zv), pData, size);
		return Z_PTR_P(zv);
	}
	return NULL;
}

static inline void *zend_hash_str_add_mem(HashTable *ht, const char *str, int len, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	if ((zv = zend_hash_str_add(ht, str, len, &tmp))) {
		Z_PTR_P(zv) = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
		memcpy(Z_PTR_P(zv), pData, size);
		return Z_PTR_P(zv);
	}
	return NULL;
}

static inline void *zend_hash_update_mem(HashTable *ht, zend_string *key, void *pData, size_t size)
{
	void *p;

	p = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
	memcpy(p, pData, size);
	return zend_hash_update_ptr(ht, key, p);
}

static inline void *zend_hash_str_update_mem(HashTable *ht, const char *str, int len, void *pData, size_t size)
{
	void *p;

	p = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
	memcpy(p, pData, size);
	return zend_hash_str_update_ptr(ht, str, len, p);
}

static inline void *zend_hash_index_update_ptr(HashTable *ht, ulong h, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_index_update(ht, h, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_next_index_insert_ptr(HashTable *ht, void *pData)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, pData);
	zv = zend_hash_next_index_insert(ht, &tmp);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_index_update_mem(HashTable *ht, ulong h, void *pData, size_t size)
{
	void *p;

	p = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
	memcpy(p, pData, size);
	return zend_hash_index_update_ptr(ht, h, p);
}

static inline void *zend_hash_next_index_insert_mem(HashTable *ht, void *pData, size_t size)
{
	zval tmp, *zv;

	ZVAL_PTR(&tmp, NULL);
	if ((zv = zend_hash_next_index_insert(ht, &tmp))) {
		Z_PTR_P(zv) = pemalloc(size, ht->u.flags & HASH_FLAG_PERSISTENT);
		memcpy(Z_PTR_P(zv), pData, size);
		return Z_PTR_P(zv);
	}
	return NULL;
}

static inline void *zend_hash_find_ptr(const HashTable *ht, zend_string *key)
{
	zval *zv;

	zv = zend_hash_find(ht, key);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_str_find_ptr(const HashTable *ht, const char *str, int len)
{
	zval *zv;

	zv = zend_hash_str_find(ht, str, len);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_hash_index_find_ptr(const HashTable *ht, ulong h)
{
	zval *zv;

	zv = zend_hash_index_find(ht, h);
	return zv ? Z_PTR_P(zv) : NULL;
}

static inline void *zend_symtable_str_find_ptr(HashTable *ht, const char *str, int len)
{
	ulong idx;

	if (ZEND_HANDLE_NUMERIC_STR(str, len, idx)) {
		return zend_hash_index_find_ptr(ht, idx);
	} else {
		return zend_hash_str_find_ptr(ht, str, len);
	}
}

static inline void *zend_hash_get_current_data_ptr_ex(HashTable *ht, HashPosition *pos)
{
	zval *zv;

	zv = zend_hash_get_current_data_ex(ht, pos);
	return zv ? Z_PTR_P(zv) : NULL;
}

#define zend_hash_get_current_data_ptr(ht) \
	zend_hash_get_current_data_ptr_ex(ht, &(ht)->nInternalPointer)

#define ZEND_HASH_FOREACH(_ht, indirect) do { \
		uint _idx; \
		for (_idx = 0; _idx < (_ht)->nNumUsed; _idx++) { \
			Bucket *_p = (_ht)->arData + _idx; \
			zval *_z = &_p->val; \
			if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
				_z = Z_INDIRECT_P(_z); \
			} \
			if (Z_TYPE_P(_z) == IS_UNDEF) continue;

#define ZEND_HASH_REVERSE_FOREACH(_ht, indirect) do { \
		uint _idx; \
		for (_idx = (_ht)->nNumUsed; _idx > 0; _idx--) { \
			Bucket *_p = (_ht)->arData + _idx - 1; \
			zval *_z = &_p->val; \
			if (indirect && Z_TYPE_P(_z) == IS_INDIRECT) { \
				_z = Z_INDIRECT_P(_z); \
			} \
			if (Z_TYPE_P(_z) == IS_UNDEF) continue;

#define ZEND_HASH_FOREACH_END() \
		} \
	} while (0)
	
#define ZEND_HASH_FOREACH_BUCKET(ht, _bucket) \
	ZEND_HASH_FOREACH(ht, 0); \
	_bucket = _p;

#define ZEND_HASH_FOREACH_VAL(ht, _val) \
	ZEND_HASH_FOREACH(ht, 0); \
	_val = _z;

#define ZEND_HASH_FOREACH_VAL_IND(ht, _val) \
	ZEND_HASH_FOREACH(ht, 1); \
	_val = _z;

#define ZEND_HASH_FOREACH_PTR(ht, _ptr) \
	ZEND_HASH_FOREACH(ht, 0); \
	_ptr = Z_PTR_P(_z);

#define ZEND_HASH_FOREACH_NUM_KEY(ht, _h) \
	ZEND_HASH_FOREACH(ht, 0); \
	_h = _p->h;

#define ZEND_HASH_FOREACH_STR_KEY(ht, _key) \
	ZEND_HASH_FOREACH(ht, 0); \
	_key = _p->key;
		
#define ZEND_HASH_FOREACH_KEY(ht, _h, _key) \
	ZEND_HASH_FOREACH(ht, 0); \
	_h = _p->h; \
	_key = _p->key;
		
#define ZEND_HASH_FOREACH_STR_KEY_VAL(ht, _key, _val) \
	ZEND_HASH_FOREACH(ht, 0); \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	ZEND_HASH_FOREACH(ht, 0); \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, _key, _val) \
	ZEND_HASH_FOREACH(ht, 1); \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	ZEND_HASH_FOREACH(ht, 1); \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_FOREACH_STR_KEY_PTR(ht, _key, _ptr) \
	ZEND_HASH_FOREACH(ht, 0); \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

#define ZEND_HASH_FOREACH_KEY_PTR(ht, _h, _key, _ptr) \
	ZEND_HASH_FOREACH(ht, 0); \
	_h = _p->h; \
	_key = _p->key; \
	_ptr = Z_PTR_P(_z);

#define ZEND_HASH_REVERSE_FOREACH_VAL(ht, _val) \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	_val = _z;

#define ZEND_HASH_REVERSE_FOREACH_PTR(ht, _ptr) \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	_ptr = Z_PTR_P(_z);

#define ZEND_HASH_REVERSE_FOREACH_VAL_IND(ht, _val) \
	ZEND_HASH_REVERSE_FOREACH(ht, 1); \
	_val = _z;

#define ZEND_HASH_REVERSE_FOREACH_KEY_VAL(ht, _h, _key, _val) \
	ZEND_HASH_REVERSE_FOREACH(ht, 0); \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_REVERSE_FOREACH_KEY_VAL_IND(ht, _h, _key, _val) \
	ZEND_HASH_REVERSE_FOREACH(ht, 1); \
	_h = _p->h; \
	_key = _p->key; \
	_val = _z;

#define ZEND_HASH_APPLY_PROTECTION(ht) \
	((ht)->u.flags & HASH_FLAG_APPLY_PROTECTION)

#define ZEND_HASH_APPLY_SHIFT 8
#define ZEND_HASH_GET_APPLY_COUNT(ht) ((ht)->u.flags >> ZEND_HASH_APPLY_SHIFT)
#define ZEND_HASH_INC_APPLY_COUNT(ht) ((ht)->u.flags += (1 << ZEND_HASH_APPLY_SHIFT))
#define ZEND_HASH_DEC_APPLY_COUNT(ht) ((ht)->u.flags -= (1 << ZEND_HASH_APPLY_SHIFT))

#endif							/* ZEND_HASH_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
