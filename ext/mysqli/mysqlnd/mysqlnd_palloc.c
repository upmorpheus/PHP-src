/*
  +----------------------------------------------------------------------+
  | PHP Version 6                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Georg Richter <georg@mysql.com>                             |
  |          Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */
#include "php.h"
#include "mysqlnd.h"
#include "mysqlnd_priv.h"
#include "mysqlnd_palloc.h"

#define MYSQLND_SILENT


#ifdef ZTS
#define LOCK_PCACHE(cache)  tsrm_mutex_lock((cache)->LOCK_access)
#define UNLOCK_PCACHE(cache) tsrm_mutex_unlock((cache)->LOCK_access)
#else
#define LOCK_PCACHE(cache)
#define UNLOCK_PCACHE(cache)
#endif


#if PHP_MAJOR_VERSION < 6
#define IS_UNICODE_DISABLED  (1)
#else
#define IS_UNICODE_DISABLED  (!UG(unicode))
#endif


/* {{{ mysqlnd_palloc_init_cache */
PHPAPI MYSQLND_ZVAL_PCACHE* mysqlnd_palloc_init_cache(unsigned int cache_size)
{
	MYSQLND_ZVAL_PCACHE *ret = calloc(1, sizeof(MYSQLND_ZVAL_PCACHE));
	unsigned int i;
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_init_cache %p]\n", ret);
#endif

#ifdef ZTS
	ret->LOCK_access = tsrm_mutex_alloc();
#endif

	ret->max_items = cache_size;
	ret->free_items = cache_size;
	ret->references = 1;

	/* 1. First initialize the free list part of the structure */
	/* One more for empty position of last_added - always 0x0, bounds checking */
	ret->free_list.ptr_line = calloc(ret->max_items + 1, sizeof(mysqlnd_zval *));
	ret->free_list.last_added = ret->free_list.ptr_line + ret->max_items;

	/* 3. Allocate and initialize our zvals and initialize the free list */
	ret->block = calloc(ret->max_items, sizeof(mysqlnd_zval));
	ret->last_in_block = &(ret->block[ret->max_items]);
	for (i = 0; i < ret->max_items; i++) {
		/* 1. Initialize */
		INIT_PZVAL(&(ret->block[i].zv));
		ZVAL_NULL(&(ret->block[i].zv));
		/* Assure it will never be freed before MSHUTDOWN */
		ZVAL_ADDREF(&(ret->block[i].zv));
		/* 2. Add to the free list  */
		*(--ret->free_list.last_added) = &(ret->block[i]);
	}
	

	return ret;
}
/* }}} */


/* {{{ mysqlnd_palloc_get_cache_reference */
MYSQLND_ZVAL_PCACHE* mysqlnd_palloc_get_cache_reference(MYSQLND_ZVAL_PCACHE * const cache)
{
	if (cache) {
		LOCK_PCACHE(cache);
		cache->references++;
		UNLOCK_PCACHE(cache);
	}
	return cache;
}
/* }}} */


/* {{{ mysqlnd_palloc_free_cache */
/*
  As this call will happen on MSHUTDOWN(), then we don't need to copy the zvals with
  copy_ctor but scrap what they point to with zval_dtor() and then just free our
  pre-allocated block. Precondition is that we ZVAL_NULL() the zvals when we put them
  to the free list after usage. We ZVAL_NULL() them when we allocate them in the 
  constructor of the cache.
*/
void mysqlnd_palloc_free_cache(MYSQLND_ZVAL_PCACHE *cache)
{
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_free_cache %p]\n", cache);
#endif

#ifdef ZTS
	tsrm_mutex_free(cache->LOCK_access);
#endif

	/* Data in pointed by 'block' was cleaned in RSHUTDOWN */
	free(cache->block);
	free(cache->free_list.ptr_line);
	free(cache);
}
/* }}} */


/* {{{ mysqlnd_palloc_init_thd_cache */
PHPAPI MYSQLND_THD_ZVAL_PCACHE* mysqlnd_palloc_init_thd_cache(MYSQLND_ZVAL_PCACHE * const cache)
{
	MYSQLND_THD_ZVAL_PCACHE *ret = calloc(1, sizeof(MYSQLND_THD_ZVAL_PCACHE));
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_init_thd_cache %p]\n", ret);
#endif
	ret->parent = mysqlnd_palloc_get_cache_reference(cache);

#ifdef ZTS
	ret->thread_id = tsrm_thread_id();
#endif

	ret->references = 1;

	/* 1. Initialize the GC list */
	ret->gc_list.ptr_line = calloc(cache->max_items, sizeof(mysqlnd_zval *));
	/* Backward and forward looping is possible */
	ret->gc_list.last_added = ret->gc_list.ptr_line;

	return ret;
}
/* }}} */


/* {{{ mysqlnd_palloc_get_thd_cache_reference */
MYSQLND_THD_ZVAL_PCACHE* mysqlnd_palloc_get_thd_cache_reference(MYSQLND_THD_ZVAL_PCACHE * const cache)
{
	if (cache) {
		++cache->references;
		mysqlnd_palloc_get_cache_reference(cache->parent);
	}
	return cache;
}
/* }}} */


/* {{{ mysqlnd_palloc_free_cache */
/*
  As this call will happen on MSHUTDOWN(), then we don't need to copy the zvals with
  copy_ctor but scrap what they point to with zval_dtor() and then just free our
  pre-allocated block. Precondition is that we ZVAL_NULL() the zvals when we put them
  to the free list after usage. We ZVAL_NULL() them when we allocate them in the 
  constructor of the cache.
*/
static
void mysqlnd_palloc_free_thd_cache(MYSQLND_THD_ZVAL_PCACHE *cache)
{
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_free_thd_cache %p]\n", cache);
#endif

	free(cache->gc_list.ptr_line);
	free(cache);
}
/* }}} */


/* {{{ mysqlnd_palloc_free_thd_cache_reference */
PHPAPI void mysqlnd_palloc_free_thd_cache_reference(MYSQLND_THD_ZVAL_PCACHE **cache)
{
	if (*cache) {
#ifndef MYSQLND_SILENT
		php_printf("[mysqlnd_palloc_free_thd_cache_reference %p] refs=%d\n", *cache, (*cache)->references);
#endif
		--(*cache)->parent->references;

		if (--(*cache)->references == 0) {
			mysqlnd_palloc_free_thd_cache(*cache);
		}
		*cache = NULL;
	}
}
/* }}} */


/*
  The cache line is a big contiguous array of zval pointers.
  Because the CPU cache will cache starting from an address, and not
  before it, then we have to organize our structure according to this.
  Thus, if 'last_added' is valid pointer (not NULL) then last_added is
  increased. When zval is cached, if there is room, last_added is decreased
  and then the zval pointer will be assigned to it. This means that some
  positions may become hot points and stay in the cache.
  Imagine we have 5 pointers in a line
  1. last_added = list_item->ptr_line + cache->max_items;
  2. get_zval -> *last_added = NULL. Use MAKE_STD_ZVAL
  3. get_zval -> *last_added = NULL. Use MAKE_STD_ZVAL
  4. get_zval -> *last_added = NULL. Use MAKE_STD_ZVAL
	0x0
	0x0
	0x0
	0x0
	0x0
	---
	empty_position, always 0x0 <-- last_added

  5. free_zval -> if (free_items++ != max_items) {// we can add more
				    *(--last_added) = zval_ptr;
				 }
	(memory addresses increase downwards)
	0x0
	0x0
	0x0
	0x0 
	0xA <-- last_added
	---
	0x0

  6. free_zval -> if (free_items++ != max_items) {// we can add more
				   *(--last_added) = zval_ptr;
				 }
	0x0
	0x0
	0x0
	0xB <-- last_added
	0xA 
	---
	0x0

  7. free_zval -> if (free_items++ != max_items) {// we can add more
				   *(--last_added) = zval_ptr;
				 }
	0x0
	0x0
	0xC <-- last_added
	0xB
	0xA
	---
	0x0

  8. get_zval -> *last_added != NULL. -> p = *last_added; *last_added++ = NULL;
	0x0
	0x0
	0x0
	0xB <-- last_added
	0xA
	---
	0x0

  9. get_zval -> *last_added != NULL. -> p = *last_added; *last_added++ = NULL;
	0x0
	0x0
	0x0
	0x0
	0xA <-- last_added
	---
	0x0

  10. get_zval -> *last_added != NULL. -> p = *last_added; *last_added++ = NULL;
	0x0
	0x0
	0x0
	0x0
	0x0
	---
	0x0 <-- last_added

*/


/* {{{ mysqlnd_palloc_get_zval */
void *mysqlnd_palloc_get_zval(MYSQLND_THD_ZVAL_PCACHE * const thd_cache, zend_bool *allocated)
{
	void *ret = NULL;

#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_get_zval %p] *last_added=%p free_items=%d\n",
				thd_cache, thd_cache? thd_cache->parent->free_list.last_added:NULL,
				thd_cache->parent->free_items);
#endif

	if (thd_cache) {
		MYSQLND_ZVAL_PCACHE *cache = thd_cache->parent;
		LOCK_PCACHE(cache);
		
		if ((ret = *cache->free_list.last_added)) {
			*cache->free_list.last_added++ = NULL;
			*allocated = FALSE;
#ifdef ZTS
			((mysqlnd_zval *) ret)->thread_id = thd_cache->thread_id;
#endif
			--cache->free_items;
			++cache->get_hits;
		} else {
			++cache->get_misses;
		}
		UNLOCK_PCACHE(cache);
	}
	if (!ret) {
		/*
		  We allocate a bit more. The user of this function will use it, but at
		  end it will use only the zval part. Because the zval part is first then
		  when freeing the zval part the whole allocated block will be cleaned, not
		  only the zval part (by the Engine when destructing the zval).
		*/
		ALLOC_ZVAL(ret);
		INIT_PZVAL((zval *) ret);
		*allocated = TRUE;
	} else {
		/* This will set the refcount to 1, increase it, to keep the variable */
		INIT_PZVAL(&((mysqlnd_zval *) ret)->zv);
		ZVAL_ADDREF(&(((mysqlnd_zval *)ret)->zv));
	}

	return ret;
}
/* }}} */


/* {{{ mysqlnd_palloc_zval_ptr_dtor */
void mysqlnd_palloc_zval_ptr_dtor(zval **zv, MYSQLND_THD_ZVAL_PCACHE * const thd_cache,
								  enum_mysqlnd_res_type type, zend_bool *copy_ctor_called TSRMLS_DC)
{
	MYSQLND_ZVAL_PCACHE *cache;
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_zval_ptr_dtor %p] parent_block=%p last_in_block=%p *zv=%p type=%d refc=%d\n",
				thd_cache,
				thd_cache->parent? thd_cache->parent->block:NULL,
				thd_cache->parent? thd_cache->parent->last_in_block:NULL,
				*zv, type, ZVAL_REFCOUNT(*zv));
#endif
	*copy_ctor_called = FALSE;
	/* Check whether cache is used and the zval is from the cache */
	if (!thd_cache || !(cache = thd_cache->parent) || ((char *)*zv < (char *)thd_cache->parent->block ||
													   (char *)*zv > (char *)thd_cache->parent->last_in_block)) {
		/*
		  This zval is not from the cache block.
		  Thus the refcount is -1 than of a zval from the cache,
		  because the zvals from the cache are owned by it.
		*/
		if (type == MYSQLND_RES_PS_BUF || type == MYSQLND_RES_PS_UNBUF) {
			; /* do nothing, zval_ptr_dtor will do the job*/
		} else if (ZVAL_REFCOUNT(*zv) > 1) {
			/*
			  Not a prepared statement, then we have to
			  call copy_ctor and then zval_ptr_dtor()

			  In Unicode mode the destruction  of the zvals should not call
			  zval_copy_ctor() because then we will leak.
			  I suppose we can use UG(unicode) in mysqlnd.c when freeing a result set
			  to check if we need to call copy_ctor().

			  If the type is IS_UNICODE, which can happen with PHP6, then we don't
			  need to copy_ctor, as the data doesn't point to our internal buffers.
			  If it's string (in PHP5 always) and in PHP6 if data is binary, then
			  it still points to internal buffers and has to be copied.
			*/
			if (Z_TYPE_PP(zv) == IS_STRING) {
				zval_copy_ctor(*zv);
			}
			*copy_ctor_called = TRUE;
		} else {
			if (Z_TYPE_PP(zv) == IS_STRING) {
				ZVAL_NULL(*zv);
			}
		}
		zval_ptr_dtor(zv);
		return;
	}

	/* The zval is from our cache */
	/* refcount is always > 1, because we call ZVAL_ADDREF(). Thus test refcount > 2 */
	if (ZVAL_REFCOUNT(*zv) > 2) {
		/*
		  Because the zval is first element in mysqlnd_zval structure, then we can
		  do upcasting from zval to mysqlnd_zval here. Because we know that this
		  zval is part of our pre-allocated block.

		  Now check whether this zval points to ZE allocated memory or to our
		  buffers. If it points to the internal buffers, call copy_ctor()
		  which will do estrndup for strings. And nothing for null, int, double.

		  This branch will be skipped for PS, because there is no need to copy
		  what is pointed by them, as they don't point to the internal buffers.
		*/
		if (((mysqlnd_zval *)*zv)->point_type == MYSQLND_POINTS_INT_BUFFER) {
			zval_copy_ctor(*zv);
			*copy_ctor_called = TRUE;
			((mysqlnd_zval *)*zv)->point_type = MYSQLND_POINTS_EXT_BUFFER;
		}
		/*
		  This will decrease the counter of the user-level (mysqlnd). When the engine
		  layer (the script) has finished working this this zval, when the variable is
		  no more used, out of scope whatever, then it will try zval_ptr_dtor() but
		  and the refcount will reach 1 and the engine won't try to destruct the
		  memory allocated by us.
		*/
		zval_ptr_dtor(zv);

		/*
		  Unfortunately, we can't return this variable to the free_list
		  because it's still used. And this cleaning up will happen at request
		  shutdown :(.
		*/
		LOCK_PCACHE(cache);
		++cache->put_misses;
		*(thd_cache->gc_list.last_added++) = (mysqlnd_zval *)*zv;
		UNLOCK_PCACHE(cache);
	} else {
		/* No user reference */
		if (((mysqlnd_zval *)*zv)->point_type == MYSQLND_POINTS_EXT_BUFFER) {
			/* PS are here and also in Unicode mode, for non-binary  */
			zval_dtor(*zv);
		}
		LOCK_PCACHE(cache);
		++cache->put_hits;
		++cache->free_items;
		((mysqlnd_zval *)*zv)->point_type = MYSQLND_POINTS_FREE;
		ZVAL_DELREF(*zv);	/* Make it 1 */
		ZVAL_NULL(*zv);
#ifdef ZTS
		memset(&((mysqlnd_zval *)*zv)->thread_id, 0, sizeof(THREAD_T));
#endif
		*(--cache->free_list.last_added) = (mysqlnd_zval *)*zv;

		UNLOCK_PCACHE(cache);
	}
}
/* }}} */


/* {{{ mysqlnd_palloc_rinit */
PHPAPI MYSQLND_THD_ZVAL_PCACHE * mysqlnd_palloc_rinit(MYSQLND_ZVAL_PCACHE * cache)
{
	return mysqlnd_palloc_init_thd_cache(cache);
}
/* }}} */


/* {{{ mysqlnd_palloc_rshutdown */
PHPAPI void mysqlnd_palloc_rshutdown(MYSQLND_THD_ZVAL_PCACHE * thd_cache)
{
	MYSQLND_ZVAL_PCACHE *cache;
	mysqlnd_zval **p;
#ifndef MYSQLND_SILENT
	php_printf("[mysqlnd_palloc_rshutdown %p]\n", thd_cache);
#endif
	if (!thd_cache || !(cache = thd_cache->parent)) {
		return;
	}

	/*
	  Keep in mind that for pthreads pthread_equal() should be used to be
	  fully standard compliant. However, the PHP code all-around, incl. the
	  the Zend MM uses direct comparison.
	*/
	p = thd_cache->gc_list.ptr_line;
	while (p < thd_cache->gc_list.last_added) {
		zval_dtor(&(*p)->zv);
		p++;
	}

	p = thd_cache->gc_list.ptr_line;
	LOCK_PCACHE(cache);
	while (p < thd_cache->gc_list.last_added) {
		(*p)->point_type = MYSQLND_POINTS_FREE;
		*(--cache->free_list.last_added) = *p;
		++cache->free_items;
#ifdef ZTS
		memset(&((*p)->thread_id), 0, sizeof(THREAD_T));
#endif	
		p++;
	}
	UNLOCK_PCACHE(cache);

	mysqlnd_palloc_free_thd_cache_reference(&thd_cache);
}
/* }}} */


/* {{{ mysqlnd_palloc_rshutdown */
PHPAPI void mysqlnd_palloc_stats(const MYSQLND_ZVAL_PCACHE * const cache, zval *return_value)
{
	if (cache) {
#if PHP_MAJOR_VERSION >= 6
		TSRMLS_FETCH();
#endif

		LOCK_PCACHE(cache);
		array_init(return_value);
#if PHP_MAJOR_VERSION >= 6
		if (UG(unicode)) {
			UChar *ustr;
			int ulen;

			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "put_hits", sizeof("put_hits") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "put_misses", sizeof("put_misses") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "get_hits", sizeof("get_hits") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "get_misses", sizeof("get_misses") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "size", sizeof("size") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "free_items", sizeof("free_items") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
			zend_string_to_unicode(UG(utf8_conv), &ustr, &ulen, "references", sizeof("references") TSRMLS_CC);
			add_u_assoc_long_ex(return_value, IS_UNICODE, ZSTR(ustr), ulen + 1, cache->put_hits);
			efree(ustr);
		} else
#endif
		{
			add_assoc_long_ex(return_value, "put_hits",		sizeof("put_hits"),		cache->put_hits);
			add_assoc_long_ex(return_value, "put_misses",	sizeof("put_misses"),	cache->put_misses);
			add_assoc_long_ex(return_value, "get_hits",		sizeof("get_hits"),		cache->get_hits);
			add_assoc_long_ex(return_value, "get_misses",	sizeof("get_misses"),	cache->get_misses);
			add_assoc_long_ex(return_value, "size",			sizeof("size"),			cache->max_items);
			add_assoc_long_ex(return_value, "free_items",	sizeof("free_items"),	cache->free_items);
			add_assoc_long_ex(return_value, "references",	sizeof("references"),	cache->references);
		}
		UNLOCK_PCACHE(cache);
	} else {
		ZVAL_NULL(return_value);
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
