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
#include "zend_globals.h"
#include "zend_variables.h"
#include "zend_API.h"
#include "zend_objects_API.h"	

ZEND_API void zend_objects_store_init(zend_objects_store *objects, zend_uint init_size)
{
	objects->object_buckets = (zend_object **) emalloc(init_size * sizeof(zend_object*));
	objects->top = 1; /* Skip 0 so that handles are true */
	objects->size = init_size;
	objects->free_list_head = -1;
	memset(&objects->object_buckets[0], 0, sizeof(zend_object*));
}

ZEND_API void zend_objects_store_destroy(zend_objects_store *objects)
{
	efree(objects->object_buckets);
	objects->object_buckets = NULL;
}

ZEND_API void zend_objects_store_call_destructors(zend_objects_store *objects TSRMLS_DC)
{
	zend_uint i;

	for (i = 1; i < objects->top ; i++) {
		zend_object *obj = objects->object_buckets[i];

		if (IS_OBJ_VALID(obj)) {
			if (!(GC_FLAGS(obj) & IS_OBJ_DESTRUCTOR_CALLED)) {
				GC_FLAGS(obj) |= IS_OBJ_DESTRUCTOR_CALLED;
				GC_REFCOUNT(obj)++;
				obj->handlers->dtor_obj(obj TSRMLS_CC);
				GC_REFCOUNT(obj)--;
			}
		}
	}
}

ZEND_API void zend_objects_store_mark_destructed(zend_objects_store *objects TSRMLS_DC)
{
	zend_uint i;

	if (!objects->object_buckets) {
		return;
	}
	for (i = 1; i < objects->top ; i++) {
		zend_object *obj = objects->object_buckets[i];

		if (IS_OBJ_VALID(obj)) {
			GC_FLAGS(obj) |= IS_OBJ_DESTRUCTOR_CALLED;
		}
	}
}

ZEND_API void zend_objects_store_free_object_storage(zend_objects_store *objects TSRMLS_DC)
{
	zend_uint i;

	/* Free object properties but don't free object their selves */
	for (i = objects->top - 1; i > 0 ; i--) {
		zend_object *obj = objects->object_buckets[i];

		if (IS_OBJ_VALID(obj)) {
			if (!(GC_FLAGS(obj) & IS_OBJ_FREE_CALLED)) {
				GC_FLAGS(obj) |= IS_OBJ_FREE_CALLED;
				if (obj->handlers->free_obj) {
					GC_REFCOUNT(obj)++;
					obj->handlers->free_obj(obj TSRMLS_CC);
					GC_REFCOUNT(obj)--;
				}
			}
		}
	}

	/* Now free objects theirselves */
	for (i = 1; i < objects->top ; i++) {
		zend_object *obj = objects->object_buckets[i];

		if (IS_OBJ_VALID(obj)) {
			/* Not adding to free list as we are shutting down anyway */
			void *ptr = ((char*)obj) - obj->handlers->offset;
			GC_REMOVE_FROM_BUFFER(obj);
			efree(ptr);
		}
	}
}


/* Store objects API */

ZEND_API void zend_objects_store_put(zend_object *object TSRMLS_DC)
{
	int handle;

	if (EG(objects_store).free_list_head != -1) {
		handle = EG(objects_store).free_list_head;
		EG(objects_store).free_list_head = GET_OBJ_BUCKET_NUMBER(EG(objects_store).object_buckets[handle]);
	} else {
		if (EG(objects_store).top == EG(objects_store).size) {
			EG(objects_store).size <<= 1;
			EG(objects_store).object_buckets = (zend_object **) erealloc(EG(objects_store).object_buckets, EG(objects_store).size * sizeof(zend_object*));
		}
		handle = EG(objects_store).top++;
	}
	object->handle = handle;
	EG(objects_store).object_buckets[handle] = object;
}

#define ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(handle)															\
            SET_OBJ_BUCKET_NUMBER(EG(objects_store).object_buckets[handle], EG(objects_store).free_list_head);	\
			EG(objects_store).free_list_head = handle;

ZEND_API void zend_objects_store_free(zend_object *object TSRMLS_DC) /* {{{ */
{
	zend_uint handle = object->handle;
	void *ptr = ((char*)object) - object->handlers->offset;

	GC_REMOVE_FROM_BUFFER(object);
	efree(ptr);
	ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(handle);
}
/* }}} */

ZEND_API void zend_objects_store_del(zend_object *object TSRMLS_DC) /* {{{ */
{
	/*	Make sure we hold a reference count during the destructor call
		otherwise, when the destructor ends the storage might be freed
		when the refcount reaches 0 a second time
	 */
	if (EG(objects_store).object_buckets &&
	    IS_OBJ_VALID(EG(objects_store).object_buckets[object->handle])) {
		if (GC_REFCOUNT(object) == 0) {
			int failure = 0;

			if (!(GC_FLAGS(object) & IS_OBJ_DESTRUCTOR_CALLED)) {
				GC_FLAGS(object) |= IS_OBJ_DESTRUCTOR_CALLED;

				if (object->handlers->dtor_obj) {
					GC_REFCOUNT(object)++;
					zend_try {
						object->handlers->dtor_obj(object TSRMLS_CC);
					} zend_catch {
						failure = 1;
					} zend_end_try();
					GC_REFCOUNT(object)--;
				}
			}
			
			if (GC_REFCOUNT(object) == 0) {
				zend_uint handle = object->handle;
				void *ptr;

				EG(objects_store).object_buckets[handle] = SET_OBJ_INVALID(object);
				if (!(GC_FLAGS(object) & IS_OBJ_FREE_CALLED)) {
					GC_FLAGS(object) |= IS_OBJ_FREE_CALLED;
					if (object->handlers->free_obj) {
						zend_try {
							GC_REFCOUNT(object)++;
							object->handlers->free_obj(object TSRMLS_CC);
							GC_REFCOUNT(object)--;
						} zend_catch {
							failure = 1;
						} zend_end_try();
					}
				}
				ptr = ((char*)object) - object->handlers->offset;
				GC_REMOVE_FROM_BUFFER(object);
				efree(ptr);
				ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(handle);
			}
			
			if (failure) {
				zend_bailout();
			}
		} else {
			GC_REFCOUNT(object)--;
		}
	}
}
/* }}} */

/* zend_object_store_set_object:
 * It is ONLY valid to call this function from within the constructor of an
 * overloaded object.  Its purpose is to set the object pointer for the object
 * when you can't possibly know its value until you have parsed the arguments
 * from the constructor function.  You MUST NOT use this function for any other
 * weird games, or call it at any other time after the object is constructed.
 * */
ZEND_API void zend_object_store_set_object(zval *zobject, zend_object *object TSRMLS_DC)
{
	EG(objects_store).object_buckets[Z_OBJ_HANDLE_P(zobject)] = object;
}

/* Called when the ctor was terminated by an exception */
ZEND_API void zend_object_store_ctor_failed(zend_object *obj TSRMLS_DC)
{
	GC_FLAGS(obj) |= IS_OBJ_DESTRUCTOR_CALLED;
}

/* Proxy objects workings */
typedef struct _zend_proxy_object {
	zend_object std;
	zval object;
	zval property;
} zend_proxy_object;

static zend_object_handlers zend_object_proxy_handlers;

ZEND_API void zend_objects_proxy_destroy(zend_object *object TSRMLS_DC)
{
}

ZEND_API void zend_objects_proxy_free_storage(zend_proxy_object *object TSRMLS_DC)
{
	zval_ptr_dtor(&object->object);
	zval_ptr_dtor(&object->property);
	efree(object);
}

ZEND_API void zend_objects_proxy_clone(zend_proxy_object *object, zend_proxy_object **object_clone TSRMLS_DC)
{
	*object_clone = emalloc(sizeof(zend_proxy_object));
	(*object_clone)->object = object->object;
	(*object_clone)->property = object->property;
	Z_ADDREF_P(&(*object_clone)->property);
	Z_ADDREF_P(&(*object_clone)->object);
}

ZEND_API zend_object *zend_object_create_proxy(zval *object, zval *member TSRMLS_DC)
{
	zend_proxy_object *obj = emalloc(sizeof(zend_proxy_object));

	GC_REFCOUNT(obj) = 1;
	GC_TYPE_INFO(obj) = IS_OBJECT;
	obj->std.ce = NULL;
	obj->std.properties = NULL;
	obj->std.guards = NULL;
	obj->std.handlers = &zend_object_proxy_handlers;
	
	ZVAL_COPY(&obj->object, object);
	ZVAL_DUP(&obj->property, member);

	return (zend_object*)obj;
}

ZEND_API void zend_object_proxy_set(zval *property, zval *value TSRMLS_DC)
{
	zend_proxy_object *probj = (zend_proxy_object*)Z_OBJ_P(property);

	if (Z_OBJ_HT(probj->object) && Z_OBJ_HT(probj->object)->write_property) {
		Z_OBJ_HT(probj->object)->write_property(&probj->object, &probj->property, value, NULL TSRMLS_CC);
	} else {
		zend_error(E_WARNING, "Cannot write property of object - no write handler defined");
	}
}

ZEND_API zval* zend_object_proxy_get(zval *property, zval *rv TSRMLS_DC)
{
	zend_proxy_object *probj = (zend_proxy_object*)Z_OBJ_P(property);

	if (Z_OBJ_HT(probj->object) && Z_OBJ_HT(probj->object)->read_property) {
		return Z_OBJ_HT(probj->object)->read_property(&probj->object, &probj->property, BP_VAR_R, NULL, rv TSRMLS_CC);
	} else {
		zend_error(E_WARNING, "Cannot read property of object - no read handler defined");
	}

	return NULL;
}

ZEND_API zend_object_handlers *zend_get_std_object_handlers(void)
{
	return &std_object_handlers;
}

static zend_object_handlers zend_object_proxy_handlers = {
	ZEND_OBJECTS_STORE_HANDLERS,

	NULL,						/* read_property */
	NULL,						/* write_property */
	NULL,						/* read dimension */
	NULL,						/* write_dimension */
	NULL,						/* get_property_ptr_ptr */
	zend_object_proxy_get,		/* get */
	zend_object_proxy_set,		/* set */
	NULL,						/* has_property */
	NULL,						/* unset_property */
	NULL,						/* has_dimension */
	NULL,						/* unset_dimension */
	NULL,						/* get_properties */
	NULL,						/* get_method */
	NULL,						/* call_method */
	NULL,						/* get_constructor */
	NULL,						/* get_class_entry */
	NULL,						/* get_class_name */
	NULL,						/* compare_objects */
	NULL,						/* cast_object */
	NULL,						/* count_elements */
};


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
