/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2005 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifndef SPL_DIRECTORY_H
#define SPL_DIRECTORY_H

#include "php.h"
#include "php_spl.h"

extern PHPAPI zend_class_entry *spl_ce_SplFileInfo;
extern PHPAPI zend_class_entry *spl_ce_DirectoryIterator;
extern PHPAPI zend_class_entry *spl_ce_RecursiveDirectoryIterator;
extern PHPAPI zend_class_entry *spl_ce_SplFileObject;

PHP_MINIT_FUNCTION(spl_directory);

typedef enum {
	SPL_FS_INFO,
	SPL_FS_DIR,
	SPL_FS_FILE,
} SPL_FS_OBJ_TYPE;

typedef struct _spl_filesystem_object {
	zend_object        std;
	char               *path;
	int                path_len;
	char               *file_name;
	int                file_name_len; 
	SPL_FS_OBJ_TYPE    type;
	union {
		struct {
			php_stream         *dirp;
			php_stream_dirent  entry;
			char               *sub_path;
			int                sub_path_len;
			int                index;
			int                is_recursive;
		} dir;
		struct {
			php_stream         *stream;
			php_stream_context *context;
			zval               *zcontext;
			char               *open_mode;
			int                open_mode_len;
			zval               *current_zval;
			char               *current_line;
			size_t             current_line_len;
			size_t             max_line_len;
			long               current_line_num;
			long               flags;
			zval               zresource;
			zend_function      *func_getCurr;
		} file;
	} u;
} spl_filesystem_object;

#define SPL_FILE_OBJECT_DROP_NEW_LINE      0x00000001 /* drop new lines */

#endif /* SPL_DIRECTORY_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
