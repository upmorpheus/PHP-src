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

#include "zend.h"
#include "zend_llist.h"
#include "zend_qsort.h"

ZEND_API void zend_llist_init(zend_llist *l, size_t size, llist_dtor_func_t dtor, unsigned char persistent)
{
	l->head  = NULL;
	l->tail  = NULL;
	l->count = 0;
	l->size  = size;
	l->dtor  = dtor;
	l->persistent = persistent;
}


ZEND_API void zend_llist_add_element(zend_llist *l, void *element)
{
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);

	tmp->prev = l->tail;
	tmp->next = NULL;
	if (l->tail) {
		l->tail->next = tmp;
	} else {
		l->head = tmp;
	}
	l->tail = tmp;
	memcpy(tmp->data, element, l->size);

	++l->count;
}


ZEND_API void zend_llist_prepend_element(zend_llist *l, void *element)
{
	zend_llist_element *tmp = pemalloc(sizeof(zend_llist_element)+l->size-1, l->persistent);

	tmp->next = l->head;
	tmp->prev = NULL;
	if (l->head) {
		l->head->prev = tmp;
	} else {
		l->tail = tmp;
	}
	l->head = tmp;
	memcpy(tmp->data, element, l->size);

	++l->count;
}


#define DEL_LLIST_ELEMENT(current, l) \
			if ((current)->prev) {\
				(current)->prev->next = (current)->next;\
			} else {\
				(l)->head = (current)->next;\
			}\
			if ((current)->next) {\
				(current)->next->prev = (current)->prev;\
			} else {\
				(l)->tail = (current)->prev;\
			}\
			if ((l)->dtor) {\
				(l)->dtor((current)->data);\
			}\
			pefree((current), (l)->persistent);\
			--l->count;


ZEND_API void zend_llist_del_element(zend_llist *l, void *element, int (*compare)(void *element1, void *element2))
{
	zend_llist_element *current=l->head;

	while (current) {
		if (compare(current->data, element)) {
			DEL_LLIST_ELEMENT(current, l);
			break;
		}
		current = current->next;
	}
}


ZEND_API void zend_llist_destroy(zend_llist *l)
{
	zend_llist_element *current=l->head, *next;
	
	while (current) {
		next = current->next;
		if (l->dtor) {
			l->dtor(current->data);
		}
		pefree(current, l->persistent);
		current = next;
	}

	l->count = 0;
}


ZEND_API void zend_llist_clean(zend_llist *l)
{
	zend_llist_destroy(l);
	l->head = l->tail = NULL;
}


ZEND_API void *zend_llist_remove_tail(zend_llist *l)
{
	zend_llist_element *old_tail;
	void *data;

	if ((old_tail = l->tail)) {
		if (old_tail->prev) {
			old_tail->prev->next = NULL;
		} else {
			l->head = NULL;
		}
        
		data = old_tail->data;

		l->tail = old_tail->prev;
		if (l->dtor) {
			l->dtor(data);
		}
		pefree(old_tail, l->persistent);

		--l->count;

		return data;
	}

	return NULL;
}


ZEND_API void zend_llist_copy(zend_llist *dst, zend_llist *src)
{
	zend_llist_element *ptr;

	zend_llist_init(dst, src->size, src->dtor, src->persistent);
	ptr = src->head;
	while (ptr) {
		zend_llist_add_element(dst, ptr->data);
		ptr = ptr->next;
	}
}


ZEND_API void zend_llist_apply_with_del(zend_llist *l, int (*func)(void *data))
{
	zend_llist_element *element, *next;

	element=l->head;
	while (element) {
		next = element->next;
		if (func(element->data)) {
			DEL_LLIST_ELEMENT(element, l);
		}
		element = next;
	}
}


ZEND_API void zend_llist_apply(zend_llist *l, llist_apply_func_t func TSRMLS_DC)
{
	zend_llist_element *element;

	for (element=l->head; element; element=element->next) {
		func(element->data TSRMLS_CC);
	}
}

ZEND_API void zend_llist_sort(zend_llist *l, llist_compare_func_t comp_func TSRMLS_DC)
{
	size_t i;

	zend_llist_element **elements;
	zend_llist_element *element, **ptr;

	if (l->count <= 0) {
		return;
	}

	elements = (zend_llist_element **) emalloc(l->count * sizeof(zend_llist_element *));

	ptr = &elements[0];

	for (element=l->head; element; element=element->next) {
		*ptr++ = element;
	}

	zend_qsort(elements, l->count, sizeof(zend_llist_element *), (compare_func_t) comp_func TSRMLS_CC);

	l->head = elements[0];
	elements[0]->prev = NULL;

	for (i = 1; i < l->count; i++) {
		elements[i]->prev = elements[i-1];
		elements[i-1]->next = elements[i];
	}
	elements[i-1]->next = NULL;
	l->tail = elements[i-1];
	efree(elements);
}


ZEND_API void zend_llist_apply_with_argument(zend_llist *l, llist_apply_with_arg_func_t func, void *arg TSRMLS_DC)
{
	zend_llist_element *element;

	for (element=l->head; element; element=element->next) {
		func(element->data, arg TSRMLS_CC);
	}
}


ZEND_API void zend_llist_apply_with_arguments(zend_llist *l, llist_apply_with_args_func_t func TSRMLS_DC, int num_args, ...)
{
	zend_llist_element *element;
	va_list args;

	va_start(args, num_args);
	for (element=l->head; element; element=element->next) {
		func(element->data, num_args, args TSRMLS_CC);
	}
	va_end(args);
}


ZEND_API int zend_llist_count(zend_llist *l)
{
	return l->count;
}


ZEND_API void *zend_llist_get_first_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	*current = l->head;
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}


ZEND_API void *zend_llist_get_last_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	*current = l->tail;
	if (*current) {
		return (*current)->data;
	} else {
		return NULL;
	}
}


ZEND_API void *zend_llist_get_next_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->next;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}


ZEND_API void *zend_llist_get_prev_ex(zend_llist *l, zend_llist_position *pos)
{
	zend_llist_position *current = pos ? pos : &l->traverse_ptr;

	if (*current) {
		*current = (*current)->prev;
		if (*current) {
			return (*current)->data;
		}
	}
	return NULL;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
