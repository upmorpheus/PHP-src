/*
   +----------------------------------------------------------------------+
   | Zend OPcache JIT                                                     |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Andy Wingo <wingo@igalia.com>                               |
   +----------------------------------------------------------------------+
*/

/* $Id:$ */

#ifndef _ZEND_WORKLIST_H_
#define _ZEND_WORKLIST_H_

#include "zend_arena.h"
#include "zend_bitset.h"

typedef struct _zend_worklist_stack {
	int *buf;
	int len;
	int capacity;
} zend_worklist_stack;

#define ZEND_WORKLIST_STACK_ALLOCA(s, _len) do { \
		(s)->buf = (int*)alloca(sizeof(int) * _len); \
		(s)->len = 0; \
		(s)->capacity = _len; \
	} while (0)

static inline int zend_worklist_stack_prepare(zend_arena **arena, zend_worklist_stack *stack, int len)
{
	ZEND_ASSERT(len >= 0);

	stack->buf = (int*)zend_arena_calloc(arena, sizeof(*stack->buf), len);
	if (!stack->buf) {
		return FAILURE;
	}
	stack->len = 0;
	stack->capacity = len;

	return SUCCESS;
}

static inline void zend_worklist_stack_push(zend_worklist_stack *stack, int i)
{
	ZEND_ASSERT(stack->len < stack->capacity);
	stack->buf[stack->len++] = i;
}

static inline int zend_worklist_stack_peek(zend_worklist_stack *stack)
{
	ZEND_ASSERT(stack->len);
	return stack->buf[stack->len - 1];
}

static inline int zend_worklist_stack_pop(zend_worklist_stack *stack)
{
	ZEND_ASSERT(stack->len);
	return stack->buf[--stack->len];
}

typedef struct _zend_worklist {
	zend_bitset visited;
	zend_worklist_stack stack;
} zend_worklist;

#define ZEND_WORKLIST_ALLOCA(w, _len) do { \
		(w)->visited = (zend_bitset)alloca(sizeof(zend_ulong) * zend_bitset_len(_len)); \
		memset((w)->visited, 0, sizeof(zend_ulong) * zend_bitset_len(_len)); \
		ZEND_WORKLIST_STACK_ALLOCA(&(w)->stack, _len); \
	} while (0)

static inline int zend_worklist_prepare(zend_arena **arena, zend_worklist *worklist, int len)
{
	ZEND_ASSERT(len >= 0);
	worklist->visited = (zend_bitset)zend_arena_calloc(arena, sizeof(zend_ulong), zend_bitset_len(len));
	if (!worklist->visited) {
		return FAILURE;
	}
	return zend_worklist_stack_prepare(arena, &worklist->stack, len);
}

static inline int zend_worklist_len(zend_worklist *worklist)
{
	return worklist->stack.len;
}

static inline int zend_worklist_push(zend_worklist *worklist, int i)
{
	ZEND_ASSERT(i >= 0 && i < worklist->stack.capacity);

	if (zend_bitset_in(worklist->visited, i)) {
		return 0;
	}

	zend_bitset_incl(worklist->visited, i);
	zend_worklist_stack_push(&worklist->stack, i);
	return 1;
}

static inline int zend_worklist_peek(zend_worklist *worklist)
{
	return zend_worklist_stack_peek(&worklist->stack);
}

static inline int zend_worklist_pop(zend_worklist *worklist)
{
	/* Does not clear visited flag */
	return zend_worklist_stack_pop(&worklist->stack);
}

#endif /* _ZEND_WORKLIST_H_ */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
