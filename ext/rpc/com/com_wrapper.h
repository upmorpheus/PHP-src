#ifndef COM_H
#define COM_H

#if PHP_WIN32

#include "oleauto.h"

typedef struct comval_ {
#ifdef _DEBUG
	int resourceindex;
#endif	
	BOOL typelib;
	BOOL enumeration;
	int refcount;
	struct {
		IDispatch *dispatch;
		ITypeInfo *typeinfo;
		IEnumVARIANT *enumvariant;
	} i;
} comval;

#define ZVAL_COM(z,o) {																\
			zval *handle;															\
																					\
			/* OBJECTS_FIXME */														\
			Z_TYPE_P(z) = IS_OBJECT;												\
			Z_OBJCE_P(z) = &com_class_entry;										\
																					\
			ALLOC_HASHTABLE(Z_OBJPROP_P(z));										\
			zend_hash_init(Z_OBJPROP_P(z), 0, NULL, ZVAL_PTR_DTOR, 0);				\
																					\
			ALLOC_ZVAL(handle);														\
			INIT_PZVAL(handle);														\
			ZVAL_LONG(handle, zend_list_insert((o), IS_COM));						\
																					\
			zval_copy_ctor(handle);													\
			zend_hash_index_update(Z_OBJPROP_P(z), 0, &handle, sizeof(zval *), NULL);	\
		}

#define RETVAL_COM(o)	ZVAL_COM(&return_value, o)
#define RETURN_COM(o)	RETVAL_COM(o)												\
						return;

#define ALLOC_COM(z)	(z) = (comval *) emalloc(sizeof(comval))
#define IS_COM			php_COM_get_le_comval()

#define C_HASTLIB(x)	((x)->typelib)
#define C_HASENUM(x)	((x)->enumeration)
#define C_REFCOUNT(x)	((x)->refcount)
#define C_ISREFD(x)		C_REFCOUNT(x)

#define C_ADDREF(x)		(++((x)->refcount))
#define C_RELEASE(x)	(--((x)->refcount))

#define C_DISPATCH(x)		((x)->i.dispatch)
#define C_TYPEINFO(x)		((x)->i.typeinfo)
#define C_ENUMVARIANT(x)	((x)->i.enumvariant)

#define C_DISPATCH_VT(x)	(C_DISPATCH(x)->lpVtbl)
#define C_TYPEINFO_VT(x)	(C_TYPEINFO(x)->lpVtbl)
#define C_ENUMVARIANT_VT(x)	(C_ENUMVARIANT(x)->lpVtbl)

#endif  /* PHP_WIN32 */

#endif  /* COM_H */
