/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Wez Furlong <wez@thebrainroom.com>                           |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef PHP_COM_DOTNET_H
#define PHP_COM_DOTNET_H

extern zend_module_entry com_dotnet_module_entry;
#define phpext_com_dotnet_ptr &com_dotnet_module_entry

#ifdef PHP_WIN32
# define PHP_COM_DOTNET_API __declspec(dllexport)
#else
# define PHP_COM_DOTNET_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(com_dotnet);
PHP_MSHUTDOWN_FUNCTION(com_dotnet);
PHP_RINIT_FUNCTION(com_dotnet);
PHP_RSHUTDOWN_FUNCTION(com_dotnet);
PHP_MINFO_FUNCTION(com_dotnet);

PHP_FUNCTION(confirm_com_dotnet_compiled);

ZEND_BEGIN_MODULE_GLOBALS(com_dotnet)
	zend_bool allow_dcom;
	zend_bool autoreg_verbose;
	zend_bool autoreg_on;
	zend_bool autoreg_case_sensitive;
	void *dotnet_runtime_stuff; /* opaque to avoid cluttering up other modules */
ZEND_END_MODULE_GLOBALS(com_dotnet)

#ifdef ZTS
# define COMG(v) TSRMG(com_dotnet_globals_id, zend_com_dotnet_globals *, v)
#else
# define COMG(v) (com_dotnet_globals.v)
#endif

extern ZEND_DECLARE_MODULE_GLOBALS(com_dotnet);

#endif	/* PHP_COM_DOTNET_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
