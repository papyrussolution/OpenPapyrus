/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2011 Oracle and/or its affiliates.  All rights reserved.
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
 * __os_unique_id --
 *	Return a unique 32-bit value.
 *
 * PUBLIC: void __os_unique_id __P((ENV *, uint32 *));
 */
void __os_unique_id(ENV * env, uint32 * idp)
{
	DB_ENV * dbenv = env ? env->dbenv : 0;
	db_timespec v;
	pid_t pid;
	uint32 id;
	*idp = 0;
	/*
	 * Our randomized value is comprised of our process ID, the current
	 * time of day and a stack address, all XOR'd together.
	 */
	__os_id(dbenv, &pid, NULL);
	__os_gettime(env, &v, 1);
	id = (uint32)pid^(uint32)v.tv_sec^(uint32)v.tv_nsec^P_TO_UINT32(&pid);
	/*
	 * We could try and find a reasonable random-number generator, but
	 * that's not all that easy to do.  Seed and use srand()/rand(), if
	 * we can find them.
	 */
	if(DB_GLOBAL(uid_init) == 0) {
		DB_GLOBAL(uid_init) = 1;
		srand((uint)id);
	}
	id ^= (uint)rand();
	*idp = id;
}
