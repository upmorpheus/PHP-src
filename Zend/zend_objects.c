#include "zend.h"
#include "zend_globals.h"
#include "zend_variables.h"

#define ZEND_DEBUG_OBJECTS 0

static zend_object_handlers zoh = {
	zend_objects_get_address,
	NULL,
	zend_objects_add_ref,
	zend_objects_del_ref,
	zend_objects_delete_obj,
	zend_objects_clone_obj
};

void zend_objects_init(zend_objects *objects, zend_uint init_size)
{
	objects->object_buckets = (zend_object_bucket *) emalloc(init_size * sizeof(zend_object_bucket));
	objects->top = 1; /* Skip 0 so that handles are true */
	objects->size = init_size;
	objects->free_list_head = -1;
}

void zend_objects_destroy(zend_objects *objects)
{
	efree(objects->object_buckets);
}

zend_object_value zend_objects_new(zend_object **object, zend_class_entry *class_type)
{
	zend_object_handle handle;
	zend_object_value retval;

	TSRMLS_FETCH();

	if (EG(objects).free_list_head != -1) {
		handle = EG(objects).free_list_head;
		EG(objects).free_list_head = EG(objects).object_buckets[handle].bucket.free_list.next;
	} else {
		if (EG(objects).top == EG(objects).size) {
			EG(objects).size <<= 1;
			EG(objects).object_buckets = (zend_object_bucket *) erealloc(EG(objects).object_buckets, EG(objects).size * sizeof(zend_object_bucket));
		}
		handle = EG(objects).top++;
	}
	EG(objects).object_buckets[handle].valid = 1;
	EG(objects).object_buckets[handle].bucket.obj.refcount = 1;
	
	*object = &EG(objects).object_buckets[handle].bucket.obj.object;

	(*object)->ce = class_type;

	retval.handle = handle;
	retval.handlers = &zoh;
#if ZEND_DEBUG_OBJECTS
	fprintf(stderr, "Allocated object id #%d\n", handle);
#endif
	return retval;
}

zend_object *zend_objects_get_address(zend_object_handle handle)
{
	TSRMLS_FETCH();

	if (!EG(objects).object_buckets[handle].valid) {
		zend_error(E_ERROR, "Trying to access invalid object");
	}
	return &EG(objects).object_buckets[handle].bucket.obj.object;
}

void zend_objects_add_ref(zend_object_handle handle)
{
	TSRMLS_FETCH();

	if (!EG(objects).object_buckets[handle].valid) {
		zend_error(E_ERROR, "Trying to add reference to invalid object");
	}

	EG(objects).object_buckets[handle].bucket.obj.refcount++;
#if ZEND_DEBUG_OBJECTS
	fprintf(stderr, "Increased refcount of object id #%d\n", handle);
#endif
}

void zend_objects_delete_obj(zend_object_handle handle)
{
	zend_object *object;
	TSRMLS_FETCH();

	if (!EG(objects).object_buckets[handle].valid) {
		zend_error(E_ERROR, "Trying to delete invalid object");
	}

	object = &EG(objects).object_buckets[handle].bucket.obj.object;
	zend_hash_destroy(object->properties);
	efree(object->properties);
	EG(objects).object_buckets[handle].valid = 0;
#if ZEND_DEBUG_OBJECTS
	fprintf(stderr, "Deleted object id #%d\n", handle);
#endif

}

void zend_objects_del_ref(zend_object_handle handle)
{
	TSRMLS_FETCH();

	if (--EG(objects).object_buckets[handle].bucket.obj.refcount == 0) {
		zend_object *object;

		if (EG(objects).object_buckets[handle].valid) {
			object = &EG(objects).object_buckets[handle].bucket.obj.object;
			zend_hash_destroy(object->properties);
			efree(object->properties);
		}
		EG(objects).object_buckets[handle].bucket.free_list.next = EG(objects).free_list_head;
		EG(objects).free_list_head = handle;
		EG(objects).object_buckets[handle].valid = 0;
#if ZEND_DEBUG_OBJECTS
		fprintf(stderr, "Deallocated object id #%d\n", handle);
#endif
	}
#if ZEND_DEBUG_OBJECTS
	else {
		fprintf(stderr, "Decreased refcount of object id #%d\n", handle);
	}
#endif
}

zend_object_value zend_objects_clone_obj(zend_object_handle handle)
{
	zend_object_value retval;
	zend_object *old_object;
	zend_object *new_object;

	TSRMLS_FETCH();

	if (!EG(objects).object_buckets[handle].valid) {
		zend_error(E_ERROR, "Trying to clone invalid object");
	}

	old_object = &EG(objects).object_buckets[handle].bucket.obj.object;
	retval = zend_objects_new(&new_object, old_object->ce);
	ALLOC_HASHTABLE(new_object->properties);
	zend_hash_init(new_object->properties, 0, NULL, ZVAL_PTR_DTOR, 0);
	zend_hash_copy(new_object->properties, old_object->properties, (copy_ctor_func_t) zval_add_ref, (void *) NULL /* Not used anymore */, sizeof(zval *));

#if ZEND_DEBUG_OBJECTS
	fprintf(stderr, "Allocated object id #%d\n", handle);
#endif
	return retval;
}
