/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2000 Andi Gutmans, Zeev Suraski                   |
   +----------------------------------------------------------------------+
   | This source file is subject to version 0.91 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        | 
   | available at through the world-wide-web at                           |
   | http://www.zend.com/license/0_91.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include "zend_extensions.h"

ZEND_API zend_llist zend_extensions;
static int last_resource_number;

int zend_load_extensions(char **extension_paths)
{
	char **p = extension_paths;

	if (!p) {
		return SUCCESS;
	}
	while (*p) {
		if (zend_load_extension(*p)==FAILURE) {
			return FAILURE;
		}
		p++;
	}
	return SUCCESS;
}


int zend_load_extension(char *path)
{
#if ZEND_EXTENSIONS_SUPPORT
	DL_HANDLE handle;
	zend_extension extension, *new_extension;
	zend_extension_version_info *extension_version_info;

	handle = DL_LOAD(path);
	if (!handle) {
#ifndef ZEND_WIN32
		fprintf(stderr, "Failed loading %s:  %s\n", path, dlerror());
#else
		fprintf(stderr, "Failed loading %s\n", path);
#endif
		return FAILURE;
	}

	extension_version_info = (zend_extension_version_info *) DL_FETCH_SYMBOL(handle, "extension_version_info");
	new_extension = (zend_extension *) DL_FETCH_SYMBOL(handle, "zend_extension_entry");
	if (!extension_version_info || !new_extension) {
		fprintf(stderr, "%s doesn't appear to be a valid Zend extension\n", path);
		return FAILURE;
	}

	if (extension_version_info->zend_extension_api_no > ZEND_EXTENSION_API_NO) {
		fprintf(stderr, "%s requires Zend version %s or later\n"
					"Current version %s, API version %d\n",
					new_extension->name,
					extension_version_info->required_zend_version,
					ZEND_VERSION,
					ZEND_EXTENSION_API_NO);
		DL_UNLOAD(handle);
		return FAILURE;
	} else if (extension_version_info->zend_extension_api_no < ZEND_EXTENSION_API_NO) {
		/* we may be able to allow for downwards compatability in some harmless cases. */
		fprintf(stderr, "%s is outdated (API version %d, current version %d)\n"
					"Contact %s at %s for a later version of this module.\n",
					new_extension->name,
					extension_version_info->zend_extension_api_no,
					ZEND_EXTENSION_API_NO,
					new_extension->author,
					new_extension->URL);
		DL_UNLOAD(handle);
		return FAILURE;
	} else if (ZTS_V!=extension_version_info->thread_safe) {
		fprintf(stderr, "Cannot load %s - it %s thread safe, whereas Zend %s\n",
					new_extension->name,
					(extension_version_info->thread_safe?"is":"isn't"),
					(ZTS_V?"is":"isn't"));
		DL_UNLOAD(handle);
		return FAILURE;
	} else if (ZEND_DEBUG!=extension_version_info->debug) {
		fprintf(stderr, "Cannot load %s - it %s debug information, whereas Zend %s\n",
					new_extension->name,
					(extension_version_info->debug?"contains":"does not contain"),
					(ZEND_DEBUG?"does":"does not"));
		DL_UNLOAD(handle);
		return FAILURE;
	}

	if (new_extension->startup) {
		if (new_extension->startup(new_extension)!=SUCCESS) {
			DL_UNLOAD(handle);
			return FAILURE;
		}
	}
	extension = *new_extension;
	extension.handle = handle;

	zend_llist_add_element(&zend_extensions, &extension);

	/*fprintf(stderr, "Loaded %s, version %s\n", extension.name, extension.version);*/

	zend_append_version_info(&extension);
	return SUCCESS;
#else
	fprintf(stderr, "Extensions are not supported on this platform.\n");
	return FAILURE;
#endif
}

static void zend_extension_shutdown(zend_extension *extension)
{
#if ZEND_EXTENSIONS_SUPPORT
	if (extension->shutdown) {
		extension->shutdown(extension);
	}
#endif
}


void zend_shutdown_extensions()
{
	zend_llist_apply(&zend_extensions, (void (*)(void *)) zend_extension_shutdown);
	zend_llist_destroy(&zend_extensions);
}


void zend_extension_dtor(zend_extension *extension)
{
#if ZEND_EXTENSIONS_SUPPORT
	DL_UNLOAD(extension->handle);
#endif
}


ZEND_API int zend_get_resource_handle(zend_extension *extension)
{
	if (last_resource_number<4) {
		extension->resource_number = last_resource_number;
		return last_resource_number;
	} else {
		return -1;
	}
}
