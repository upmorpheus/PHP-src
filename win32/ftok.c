/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2017 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Anatol Belski <ab@php.net>                                   |
   +----------------------------------------------------------------------+
 */

#include "ipc.h"

#include <windows.h>
#include <sys/stat.h>

#include "ioutil.h"

PHP_WIN32_IPC_API key_t
ftok(const char *pathname, int proj_id)
{/*{{{*/
	HANDLE fh;
	struct _stat st;
	BY_HANDLE_FILE_INFORMATION bhfi;
	key_t ret;
	PHP_WIN32_IOUTIL_INIT_W(pathname)

	if (!pathw) {
		return (key_t)-1;
	}

	if (_wstat(pathw, &st) < 0) {
		PHP_WIN32_IOUTIL_CLEANUP_W()
		return (key_t)-1;
	}

	if ((fh = CreateFileW(pathw, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE) {
		PHP_WIN32_IOUTIL_CLEANUP_W()
		return (key_t)-1;
	}

	if (!GetFileInformationByHandle(fh, &bhfi)) {
		PHP_WIN32_IOUTIL_CLEANUP_W()
		CloseHandle(fh);
		return (key_t)-1;
	}

	ret = (key_t) ((proj_id & 0xff) << 24 | (st.st_dev & 0xff) << 16 | ((bhfi.nFileIndexLow | (__int64)bhfi.nFileIndexHigh << 32) & 0xffff));

	CloseHandle(fh);
	PHP_WIN32_IOUTIL_CLEANUP_W()

	return ret;
}/*}}}*/

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
