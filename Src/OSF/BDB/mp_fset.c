/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2011 Oracle and/or its affiliates.  All rights reserved.
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
// @v9.5.5 #include "dbinc/log.h"
// @v9.5.5 #include "dbinc/txn.h"
/*
 * __memp_dirty --
 *	Upgrade a page from a read-only to a writable pointer.
 *
 * PUBLIC: int __memp_dirty __P((DB_MPOOLFILE *, void *,
 * PUBLIC:     DB_THREAD_INFO *, DB_TXN *, DB_CACHE_PRIORITY, uint32));
 */
int __memp_dirty(DB_MPOOLFILE * dbmfp, void * addrp, DB_THREAD_INFO * ip, DB_TXN * txn, DB_CACHE_PRIORITY priority, uint32 flags)
{
	DB_MPOOL_HASH * hp;
	DB_TXN * ancestor;
	MPOOL * c_mp;
#ifdef DIAG_MVCC
	MPOOLFILE * mfp;
#endif
	REGINFO * infop;
	int ret;
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	int mvcc = atomic_read(&dbmfp->mfp->multiversion);
	// Convert the page address to a buffer header. 
	void * pgaddr = *(void **)addrp;
	BH * bhp = (BH *)((uint8 *)pgaddr-SSZA(BH, buf));
	db_pgno_t pgno = bhp->pgno;
	// If we have it exclusively then its already dirty. 
	if(F_ISSET(bhp, BH_EXCLUSIVE)) {
		DB_ASSERT(env, F_ISSET(bhp, BH_DIRTY));
		return 0;
	}
	SETIFZ(flags, DB_MPOOL_DIRTY);
	DB_ASSERT(env, flags == DB_MPOOL_DIRTY || flags == DB_MPOOL_EDIT);
	if(F_ISSET(dbmfp, MP_READONLY)) {
		__db_errx(env, DB_STR_A("3008", "%s: dirty flag set for readonly file page", "%s"), __memp_fn(dbmfp));
		return EACCES;
	}
	for(ancestor = txn; ancestor != NULL && ancestor->parent != NULL; ancestor = ancestor->parent)
		;
	if(mvcc && txn != NULL && flags == DB_MPOOL_DIRTY && (!BH_OWNED_BY(env, bhp, ancestor) || SH_CHAIN_HASNEXT(bhp, vc))) {
		atomic_inc(env, &bhp->ref);
		*(void **)addrp = NULL;
		if((ret = __memp_fput(dbmfp, ip, pgaddr, priority)) != 0) {
			__db_errx(env, DB_STR_A("3009", "%s: error releasing a read-only page", "%s"), __memp_fn(dbmfp));
			atomic_dec(env, &bhp->ref);
			return ret;
		}
		if((ret = __memp_fget(dbmfp, &pgno, ip, txn, flags, addrp)) != 0) {
			if(ret != DB_LOCK_DEADLOCK)
				__db_errx(env, DB_STR_A("3010", "%s: error getting a page for writing", "%s"), __memp_fn(dbmfp));
			atomic_dec(env, &bhp->ref);
			return ret;
		}
		atomic_dec(env, &bhp->ref);
		/*
		 * If the MVCC handle count hasn't changed, we should get a
		 * different version of the page.
		 */
		DB_ASSERT(env, *(void **)addrp != pgaddr || mvcc != atomic_read(&dbmfp->mfp->multiversion));
		pgaddr = *(void **)addrp;
		bhp = (BH *)((uint8 *)pgaddr-SSZA(BH, buf));
		DB_ASSERT(env, pgno == bhp->pgno);
		return 0;
	}
	infop = &dbmp->reginfo[bhp->region];
	c_mp = (MPOOL *)infop->primary;
	hp = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab);
	hp = &hp[bhp->bucket];
	// Drop the shared latch and get an exclusive. We have the buf ref'ed.
	MUTEX_UNLOCK(env, bhp->mtx_buf);
	MUTEX_LOCK(env, bhp->mtx_buf);
	DB_ASSERT(env, !F_ISSET(bhp, BH_EXCLUSIVE));
	F_SET(bhp, BH_EXCLUSIVE);
	/* Set/clear the page bits. */
	if(!F_ISSET(bhp, BH_DIRTY)) {
#ifdef DIAGNOSTIC
		MUTEX_LOCK(env, hp->mtx_hash);
#endif
		atomic_inc(env, &hp->hash_page_dirty);
		F_SET(bhp, BH_DIRTY);
#ifdef DIAGNOSTIC
		MUTEX_UNLOCK(env, hp->mtx_hash);
#endif
	}
#ifdef DIAG_MVCC
	mfp = R_ADDR(env->mp_handle->reginfo, bhp->mf_offset);
	MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ|PROT_WRITE);
#endif
	DB_ASSERT(env, !F_ISSET(bhp, BH_DIRTY) || atomic_read(&hp->hash_page_dirty) != 0);
	return 0;
}
/*
 * __memp_shared --
 *	Downgrade a page from exlusively held to shared.
 *
 * PUBLIC: int __memp_shared __P((DB_MPOOLFILE *, void *));
 */
int __memp_shared(DB_MPOOLFILE * dbmfp, void * pgaddr)
{
	ENV * env = dbmfp->env;
	/* Convert the page address to a buffer header. */
	BH * bhp = (BH *)((uint8 *)pgaddr-SSZA(BH, buf));
	if(F_ISSET(bhp, BH_DIRTY))
		dbmfp->mfp->file_written = 1;
	DB_ASSERT(env, F_ISSET(bhp, BH_EXCLUSIVE));
	F_CLR(bhp, BH_EXCLUSIVE);
	MUTEX_UNLOCK(env, bhp->mtx_buf);
	MUTEX_READLOCK(env, bhp->mtx_buf);
	return 0;
}
