/* This is a generated file, edit the .stub.php file instead. */

ZEND_BEGIN_ARG_INFO_EX(arginfo_shm_attach, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, memsize, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, perm, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_shm_detach, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, shm_identifier)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_shm_has_var, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, id)
	ZEND_ARG_TYPE_INFO(0, variable_key, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_shm_remove arginfo_shm_detach

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_shm_put_var, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, shm_identifier)
	ZEND_ARG_TYPE_INFO(0, variable_key, IS_LONG, 0)
	ZEND_ARG_INFO(0, variable)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_shm_get_var, 0, 0, 2)
	ZEND_ARG_INFO(0, id)
	ZEND_ARG_TYPE_INFO(0, variable_key, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_shm_remove_var arginfo_shm_has_var


ZEND_FUNCTION(shm_attach);
ZEND_FUNCTION(shm_detach);
ZEND_FUNCTION(shm_has_var);
ZEND_FUNCTION(shm_remove);
ZEND_FUNCTION(shm_put_var);
ZEND_FUNCTION(shm_get_var);
ZEND_FUNCTION(shm_remove_var);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(shm_attach, arginfo_shm_attach)
	ZEND_FE(shm_detach, arginfo_shm_detach)
	ZEND_FE(shm_has_var, arginfo_shm_has_var)
	ZEND_FE(shm_remove, arginfo_shm_remove)
	ZEND_FE(shm_put_var, arginfo_shm_put_var)
	ZEND_FE(shm_get_var, arginfo_shm_get_var)
	ZEND_FE(shm_remove_var, arginfo_shm_remove_var)
	ZEND_FE_END
};
