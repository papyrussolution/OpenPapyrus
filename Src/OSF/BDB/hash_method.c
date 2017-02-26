/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
// @v9.5.5 #include "dbinc/db_page.h"
// @v9.5.5 #include "dbinc/lock.h"
// @v9.5.5 #include "dbinc/mp.h"
// @v9.5.5 #include "dbinc/crypto.h"
// @v9.5.5 #include "dbinc/btree.h"
// @v9.5.5 #include "dbinc/hash.h"
#pragma hdrstop

static int __ham_set_h_ffactor __P((DB*, uint32));
static int __ham_get_h_hash __P((DB*, uint32 (* *)(DB*, const void *, uint32)));
static int __ham_set_h_hash __P((DB*, uint32 (*)(DB *, const void *, uint32)));
static int __ham_set_h_nelem __P((DB*, uint32));

static int __ham_get_h_compare __P((DB*, int (* *)(DB*, const DBT*, const DBT *)));

/*
 * __ham_db_create --
 *	Hash specific initialization of the DB structure.
 *
 * PUBLIC: int __ham_db_create __P((DB *));
 */
int __ham_db_create(DB * dbp)
{
	HASH * hashp;
	int ret;
	if((ret = __os_malloc(dbp->env, sizeof(HASH), &dbp->h_internal)) != 0)
		return ret;
	hashp = (HASH *)dbp->h_internal;
	hashp->h_nelem = 0;                     /* Defaults. */
	hashp->h_ffactor = 0;
	hashp->h_hash = NULL;
	hashp->h_compare = NULL;
	dbp->get_h_ffactor = __ham_get_h_ffactor;
	dbp->set_h_ffactor = __ham_set_h_ffactor;
	dbp->get_h_hash = __ham_get_h_hash;
	dbp->set_h_hash = __ham_set_h_hash;
	dbp->get_h_compare = __ham_get_h_compare;
	dbp->set_h_compare = __ham_set_h_compare;
	dbp->get_h_nelem = __ham_get_h_nelem;
	dbp->set_h_nelem = __ham_set_h_nelem;
	return 0;
}
/*
 * PUBLIC: int __ham_db_close __P((DB *));
 */
int __ham_db_close(DB * dbp)
{
	if(dbp->h_internal) {
		__os_free(dbp->env, dbp->h_internal);
		dbp->h_internal = NULL;
	}
	return 0;
}
/*
 * __ham_get_h_ffactor --
 *
 * PUBLIC: int __ham_get_h_ffactor __P((DB *, uint32 *));
 */
int __ham_get_h_ffactor(DB * dbp, uint32 * h_ffactorp)
{
	HASH * hashp = (HASH *)dbp->h_internal;
	*h_ffactorp = hashp->h_ffactor;
	return 0;
}
/*
 * __ham_set_h_ffactor --
 *	Set the fill factor.
 */
static int __ham_set_h_ffactor(DB * dbp, uint32 h_ffactor)
{
	HASH * hashp;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_h_ffactor");
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	hashp = (HASH *)dbp->h_internal;
	hashp->h_ffactor = h_ffactor;
	return 0;
}
/*
 * __ham_get_h_hash --
 *	Get the hash function.
 */
static int __ham_get_h_hash(DB * dbp, uint32(**funcp) __P((DB*, const void *, uint32)))
{
	HASH * hashp;
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	hashp = (HASH *)dbp->h_internal;
	if(funcp != NULL)
		*funcp = hashp->h_hash;
	return 0;
}
/*
 * __ham_set_h_hash --
 *	Set the hash function.
 */
static int __ham_set_h_hash(DB * dbp, uint32(*func) __P((DB*, const void *, uint32)))
{
	HASH * hashp;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_h_hash");
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	hashp = (HASH *)dbp->h_internal;
	hashp->h_hash = func;
	return 0;
}
/*
 * __ham_get_h_compare --
 *	Get the comparison function.
 */
static int __ham_get_h_compare(DB * dbp, int (**funcp)(DB*, const DBT*, const DBT *))
{
	HASH * t;
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	t = (HASH *)dbp->h_internal;
	if(funcp != NULL)
		*funcp = t->h_compare;
	return 0;
}
/*
 * __ham_set_h_compare --
 *	Set the comparison function.
 *
 * PUBLIC: int __ham_set_h_compare
 * PUBLIC:         __P((DB *, int (*)(DB *, const DBT *, const DBT *)));
 */
int __ham_set_h_compare(DB * dbp, int (*func)(DB*, const DBT*, const DBT *))
{
	HASH * t;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_h_compare");
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	t = (HASH *)dbp->h_internal;
	t->h_compare = func;
	return 0;
}
/*
 * __db_get_h_nelem --
 *
 * PUBLIC: int __ham_get_h_nelem __P((DB *, uint32 *));
 */
int __ham_get_h_nelem(DB * dbp, uint32 * h_nelemp)
{
	HASH * hashp;
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	hashp = (HASH *)dbp->h_internal;
	*h_nelemp = hashp->h_nelem;
	return 0;
}
/*
 * __ham_set_h_nelem --
 *	Set the table size.
 */
static int __ham_set_h_nelem(DB * dbp, uint32 h_nelem)
{
	HASH * hashp;
	DB_ILLEGAL_AFTER_OPEN(dbp, "DB->set_h_nelem");
	DB_ILLEGAL_METHOD(dbp, DB_OK_HASH);
	hashp = (HASH *)dbp->h_internal;
	hashp->h_nelem = h_nelem;
	return 0;
}
/*
 * __ham_copy_config
 *	Copy the configuration of one DB handle to another.
 * PUBLIC: void __ham_copy_config __P((DB *, DB*, uint32));
 */
void __ham_copy_config(DB * src, DB * dst, uint32 nparts)
{
	HASH * s = (HASH *)src->h_internal;
	HASH * d = (HASH *)dst->h_internal;
	d->h_ffactor = s->h_ffactor;
	d->h_nelem = s->h_nelem/nparts;
	d->h_hash = s->h_hash;
	d->h_compare = s->h_compare;
}
