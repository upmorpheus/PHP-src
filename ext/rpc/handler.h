#ifndef HANDLER_H
#define HANDLER_H

#include "php.h"

#define RPC_HANDLER(layer)				{#layer, layer##_handler_init, &layer##_object_handlers, &layer##_class_entry, layer##_function_entry}

#define RPC_DECLARE_HANDLER(layer)		void layer##_handler_init();					\
										rpc_object_handlers layer##_object_handlers;	\
										zend_class_entry layer##_class_entry;			\
										function_entry layer##_function_entry[];

#define RPC_INIT_FUNCTION(layer)		void layer##_handler_init()

#define RPC_REGISTER_HANDLERS(layer)	zend_class_entry layer##_class_entry;			\
										rpc_object_handlers layer##_object_handlers;	\

  
#define RPC_FUNCTION_ENTRY_START(layer)	function_entry layer##_function_entry[] = {		\
											PHP_FALIAS(layer##_load, rpc_load, NULL)	\
											PHP_FALIAS(layer##_call, rpc_call, NULL)	\
											PHP_FALIAS(layer##_get, rpc_get, NULL)		\
											PHP_FALIAS(layer##_set, rpc_set, NULL)

#define RPC_FUNCTION_ENTRY_END()			{NULL, NULL, NULL}							\
										};


typedef struct _rpc_object_handlers {
	int i;
} rpc_object_handlers;

typedef struct _rpc_handler_entry {
	char				*name;
	void (*rpc_handler_init)();
	rpc_object_handlers	*handlers;
	zend_class_entry	*ce;
	function_entry		*functions;
} rpc_handler_entry;

typedef struct _rpc_internal {
	zend_class_entry	*ce;
	rpc_object_handlers	**handlers;
	void				*data;
	int					refcount;
} rpc_internal;

#endif /* HANDLER_H */