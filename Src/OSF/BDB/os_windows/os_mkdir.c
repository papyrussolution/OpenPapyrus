/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/crypto.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#pragma hdrstop
/*
 * __os_mkdir --
 *	Create a directory.
 */
int __os_mkdir(ENV * env, const char * name, int mode)
{
	DB_ENV * dbenv;
	_TCHAR * tname;
	int ret;
	dbenv = env == NULL ? NULL : env->dbenv;
	if(dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS|DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0013", "fileops: mkdir %s", "%s"), name);
	/* Make the directory, with paranoid permissions. */
	TO_TSTRING(env, name, tname, ret);
	if(ret != 0)
		return ret;
	RETRY_CHK(!CreateDirectory(tname, NULL), ret);
	FREE_STRING(env, tname);
	if(ret != 0)
		return __os_posix_err(ret);
	return ret;
}
