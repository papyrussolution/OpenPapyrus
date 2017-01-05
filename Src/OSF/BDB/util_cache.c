/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2011 Oracle and/or its affiliates.  All rights reserved.
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
 * __db_util_cache --
 *	Compute if we have enough cache.
 *
 * PUBLIC: int __db_util_cache __P((DB *, uint32 *, int *));
 */
int __db_util_cache(DB*dbp, uint32 * cachep, int * resizep)
{
	uint32 pgsize;
	int ret;
	/* Get the current page size. */
	if((ret = dbp->get_pagesize(dbp, &pgsize)) != 0)
		return ret;
	/*
	 * The current cache size is in cachep.  If it's insufficient, set the
	 * the memory referenced by resizep to 1 and set cachep to the minimum
	 * size needed.
	 *
	 * Make sure our current cache is big enough.  We want at least
	 * DB_MINPAGECACHE pages in the cache.
	 */
	if((*cachep/pgsize) < DB_MINPAGECACHE) {
		*resizep = 1;
		*cachep = pgsize*DB_MINPAGECACHE;
	}
	else
		*resizep = 0;
	return 0;
}
