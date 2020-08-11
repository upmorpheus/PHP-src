/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 47d926b2ba30bb3616f4175664723352fff164e7 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_COMPersistHelper___construct, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, com_object, VARIANT, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_COMPersistHelper_GetCurFileName, 0, 0, MAY_BE_STRING|MAY_BE_FALSE)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_COMPersistHelper_SaveToFile, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, remember, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_COMPersistHelper_LoadFromFile, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_COMPersistHelper_GetMaxStreamSize, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_COMPersistHelper_InitNew, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_COMPersistHelper_LoadFromStream, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, stream)
ZEND_END_ARG_INFO()

#define arginfo_class_COMPersistHelper_SaveToStream arginfo_class_COMPersistHelper_LoadFromStream


ZEND_METHOD(COMPersistHelper, __construct);
ZEND_METHOD(COMPersistHelper, GetCurFileName);
ZEND_METHOD(COMPersistHelper, SaveToFile);
ZEND_METHOD(COMPersistHelper, LoadFromFile);
ZEND_METHOD(COMPersistHelper, GetMaxStreamSize);
ZEND_METHOD(COMPersistHelper, InitNew);
ZEND_METHOD(COMPersistHelper, LoadFromStream);
ZEND_METHOD(COMPersistHelper, SaveToStream);


static const zend_function_entry class_COMPersistHelper_methods[] = {
	ZEND_ME(COMPersistHelper, __construct, arginfo_class_COMPersistHelper___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, GetCurFileName, arginfo_class_COMPersistHelper_GetCurFileName, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, SaveToFile, arginfo_class_COMPersistHelper_SaveToFile, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, LoadFromFile, arginfo_class_COMPersistHelper_LoadFromFile, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, GetMaxStreamSize, arginfo_class_COMPersistHelper_GetMaxStreamSize, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, InitNew, arginfo_class_COMPersistHelper_InitNew, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, LoadFromStream, arginfo_class_COMPersistHelper_LoadFromStream, ZEND_ACC_PUBLIC)
	ZEND_ME(COMPersistHelper, SaveToStream, arginfo_class_COMPersistHelper_SaveToStream, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
