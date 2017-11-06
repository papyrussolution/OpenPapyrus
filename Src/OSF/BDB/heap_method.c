/*-
 * See the file LICENSE for redistribution information.
 * Copyright (c) 2010, 2011 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
/*
 * __heap_db_create --
 *	Heap specific initialization of the DB structure.
 *
 * PUBLIC: int __heap_db_create __P((DB *));
 */
int __heap_db_create(DB * dbp)
{
	HEAP * h;
	int ret;
	if((ret = __os_calloc(dbp->env, 1, sizeof(HEAP), &h)) != 0)
		return ret;
	dbp->heap_internal = h;
	h->region_size = HEAP_DEFAULT_REGION_MAX;
	dbp->get_heapsize = __heap_get_heapsize;
	dbp->set_heapsize = __heap_set_heapsize;
	return 0;
}
/*
 * __heap_db_close --
 *      Heap specific discard of the DB structure.
 *
 * PUBLIC: int __heap_db_close __P((DB *));
 */
int __heap_db_close(DB * dbp)
{
	HEAP * h;
	int ret = 0;
	if((h = (HEAP *)dbp->heap_internal) == NULL)
		return 0;
	__os_free(dbp->env, h);
	dbp->heap_internal = NULL;
	return 0;
}
/*
 * __heap_get_heapsize --
 *	Get the initial size of the heap.
 *
 * PUBLIC: int __heap_get_heapsize __P((DB *, uint32 *, uint32 *));
 */
int __heap_get_heapsize(DB * dbp, uint32 * gbytes, uint32 * bytes)
{
	HEAP * h;
	DB_ILLEGAL_METHOD(dbp, DB_OK_HEAP);
	h = (HEAP *)dbp->heap_internal;
	*gbytes = h->gbytes;
	*bytes = h->bytes;
	return 0;
}
/*
 * __heap_set_heapsize --
 *	Set the initial size of the heap.
 *
 * PUBLIC: int __heap_set_heapsize __P((DB *, uint32, uint32, uint32));
 */
int __heap_set_heapsize(DB * dbp, uint32 gbytes, uint32 bytes, uint32 flags)
{
	HEAP * h;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_heapsize");
	DB_ILLEGAL_METHOD(dbp, DB_OK_HEAP);
	COMPQUIET(flags, 0);
	h = (HEAP *)dbp->heap_internal;
	h->gbytes = gbytes;
	h->bytes = bytes;
	return 0;
}
/*
 * __heap_exist --
 *      Test to see if heap exists or not, used in Perl interface
 *
 * PUBLIC: int __heap_exist();
 */
int __heap_exist()
{
	return 1;
}

