/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"

#if DBA_GDBM
#include "php_gdbm.h"

#include <gdbm.h>

#define GDBM_DATA dba_gdbm_data *dba = info->dbf
#define GDBM_GKEY datum gkey; gkey.dptr = (char *) key; gkey.dsize = keylen

typedef struct {
	GDBM_FILE dbf;
	datum nextkey;
} dba_gdbm_data;

DBA_OPEN_FUNC(gdbm)
{
	GDBM_FILE dbf;
	int gmode = 0;
	int filemode = 0644;

	gmode = info->mode == DBA_READER ? GDBM_READER :
		info->mode == DBA_WRITER ? GDBM_WRITER : 
		info->mode == DBA_CREAT  ? GDBM_WRCREAT : 
		info->mode == DBA_TRUNC ? GDBM_NEWDB : -1;
		
	if(gmode == -1) 
		return FAILURE;

	if(info->argc > 0) {
		convert_to_long_ex(info->argv[0]);
		filemode = (*info->argv[0])->value.lval;
	}

	dbf = gdbm_open(info->path, 0, gmode, filemode, NULL);
	
	if(dbf) {
		info->dbf = malloc(sizeof(dba_gdbm_data));
		memset(info->dbf, 0, sizeof(dba_gdbm_data));
		((dba_gdbm_data *) info->dbf)->dbf = dbf;
		return SUCCESS;
	}
	return FAILURE;
}

DBA_CLOSE_FUNC(gdbm)
{
	GDBM_DATA;
	
	if(dba->nextkey.dptr) free(dba->nextkey.dptr);
	gdbm_close(dba->dbf);
	free(dba);
}

DBA_FETCH_FUNC(gdbm)
{
	GDBM_DATA;
	datum gval;
	char *new = NULL;

	GDBM_GKEY;
	gval = gdbm_fetch(dba->dbf, gkey);
	if(gval.dptr) {
		if(newlen) *newlen = gval.dsize;
		new = estrndup(gval.dptr, gval.dsize);
		free(gval.dptr);
	}
	return new;
}

DBA_UPDATE_FUNC(gdbm)
{
	datum gval;
	GDBM_DATA;

	GDBM_GKEY;
	gval.dptr = (char *) val;
	gval.dsize = vallen;

	if(gdbm_store(dba->dbf, gkey, gval, 
				mode == 1 ? GDBM_INSERT : GDBM_REPLACE) == 0)
		return SUCCESS;
	printf("XXX %s\n", gdbm_strerror(gdbm_errno));
	return FAILURE;
}

DBA_EXISTS_FUNC(gdbm)
{
	GDBM_DATA;
	GDBM_GKEY;

	return gdbm_exists(dba->dbf, gkey) ? SUCCESS : FAILURE;
}

DBA_DELETE_FUNC(gdbm)
{
	GDBM_DATA;
	GDBM_GKEY;
	
	return gdbm_delete(dba->dbf, gkey) == -1 ? FAILURE : SUCCESS;
}

DBA_FIRSTKEY_FUNC(gdbm)
{
	GDBM_DATA;
	datum gkey;
	char *key = NULL;

	if(dba->nextkey.dptr) {
		free(dba->nextkey.dptr);
	}
	
	gkey = gdbm_firstkey(dba->dbf);
	if(gkey.dptr) {
		key = estrndup(gkey.dptr, gkey.dsize);
		if(newlen) *newlen = gkey.dsize;
		dba->nextkey = gkey;
	} else {
		dba->nextkey.dptr = NULL;
	}
	return key;
}

DBA_NEXTKEY_FUNC(gdbm)
{
	GDBM_DATA;
	char *nkey = NULL;
	datum gkey;

	if(!dba->nextkey.dptr) return NULL;
	
	gkey = gdbm_nextkey(dba->dbf, dba->nextkey);
	free(dba->nextkey.dptr);
	if(gkey.dptr) {
		nkey = estrndup(gkey.dptr, gkey.dsize);
		if(newlen) *newlen = gkey.dsize;
		dba->nextkey = gkey;
	} else {
		dba->nextkey.dptr = NULL;
	}
	return nkey;
}

DBA_OPTIMIZE_FUNC(gdbm)
{
	GDBM_DATA;
	gdbm_reorganize(dba->dbf);
	return SUCCESS;
}

DBA_SYNC_FUNC(gdbm) 
{
	GDBM_DATA;

	gdbm_sync(dba->dbf);
	return SUCCESS;
}
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
