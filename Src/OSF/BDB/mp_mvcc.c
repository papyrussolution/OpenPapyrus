/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006, 2011 Oracle and/or its affiliates.  All rights reserved.
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

static int __pgno_cmp(const void *, const void *);
/*
 * __memp_bh_settxn --
 *	Set the transaction that owns the given buffer.
 *
 * PUBLIC: int __memp_bh_settxn __P((DB_MPOOL *, MPOOLFILE *mfp, BH *, void *));
 */
int __memp_bh_settxn(DB_MPOOL * dbmp, MPOOLFILE * mfp, BH * bhp, void * vtd)
{
	ENV * env = dbmp->env;
	TXN_DETAIL * td = (TXN_DETAIL *)vtd;
	if(td == NULL) {
		__db_errx(env, DB_STR_A("3002", "%s: non-transactional update to a multiversion file", "%s"), __memp_fns(dbmp, mfp));
		return EINVAL;
	}
	if(bhp->td_off != INVALID_ROFF) {
		DB_ASSERT(env, BH_OWNER(env, bhp) == td);
		return 0;
	}
	bhp->td_off = R_OFFSET(&env->tx_handle->reginfo, td);
	return __txn_add_buffer(env, td);
}
/*
 * __memp_skip_curadj --
 *	Indicate whether a cursor adjustment can be skipped for a snapshot
 *	cursor.
 *
 * PUBLIC: int __memp_skip_curadj __P((DBC *, db_pgno_t));
 */
int __memp_skip_curadj(DBC * dbc, db_pgno_t pgno)
{
	BH * bhp;
	DB_MPOOL * dbmp;
	DB_MPOOLFILE * dbmfp;
	DB_MPOOL_HASH * hp;
	DB_TXN * txn;
	ENV * env;
	MPOOLFILE * mfp;
	REGINFO * infop;
	roff_t mf_offset;
	int ret, skip;
	uint32 bucket;
	env = dbc->env;
	dbmp = env->mp_handle;
	dbmfp = dbc->dbp->mpf;
	mfp = dbmfp->mfp;
	mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	skip = 0;
	for(txn = dbc->txn; txn->parent != NULL; txn = txn->parent)
		;
	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	MP_GET_BUCKET(env, mfp, pgno, &infop, hp, bucket, ret);
	if(ret != 0) {
		/* Panic: there is no way to return the error. */
		__env_panic(env, ret);
		return 0;
	}
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		if(bhp->pgno != pgno || bhp->mf_offset != mf_offset)
			continue;
		if(!BH_OWNED_BY(env, bhp, txn))
			skip = 1;
		break;
	}
	MUTEX_UNLOCK(env, hp->mtx_hash);
	return skip;
}

#define DB_FREEZER_MAGIC 0x06102002

/*
 * __memp_bh_freeze --
 *	Save a buffer to temporary storage in case it is needed later by
 *	a snapshot transaction.  This function should be called with the buffer
 *	locked and will exit with it locked.  A BH_FROZEN buffer header is
 *	allocated to represent the frozen data in mpool.
 *
 * PUBLIC: int __memp_bh_freeze __P((DB_MPOOL *, REGINFO *, DB_MPOOL_HASH *,
 * PUBLIC:     BH *, int *));
 */
int __memp_bh_freeze(DB_MPOOL * dbmp, REGINFO * infop, DB_MPOOL_HASH * hp, BH * bhp, int * need_frozenp)
{
	BH * frozen_bhp;
	BH_FROZEN_ALLOC * frozen_alloc;
	db_mutex_t mutex;
	db_pgno_t maxpgno, newpgno, nextfree;
	size_t nio;
	int t_ret;
	uint32 magic, nbucket, ncache;
	char filename[100];
	ENV * env = dbmp->env;
	MPOOL * c_mp = (MPOOL *)infop->primary;
	int created = 0;
	int h_locked = 0;
	int ret = 0;
	/* Find the associated MPOOLFILE. */
	MPOOLFILE * mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
	const uint32 pagesize = mfp->pagesize;
	char * real_name = NULL;
	DB_FH * fhp = NULL;
	MVCC_MPROTECT(bhp->buf, pagesize, PROT_READ|PROT_WRITE);
	MPOOL_REGION_LOCK(env, infop);
	frozen_bhp = SH_TAILQ_FIRST(&c_mp->free_frozen, __bh);
	if(frozen_bhp) {
		SH_TAILQ_REMOVE(&c_mp->free_frozen, frozen_bhp, hq, __bh);
		*need_frozenp = SH_TAILQ_EMPTY(&c_mp->free_frozen);
	}
	else {
		*need_frozenp = 1;
		/* There might be a small amount of unallocated space. */
		if(__env_alloc(infop, sizeof(BH_FROZEN_ALLOC)+sizeof(BH_FROZEN_PAGE), &frozen_alloc) == 0) {
			frozen_bhp = (BH *)(frozen_alloc+1);
			frozen_bhp->mtx_buf = MUTEX_INVALID;
			SH_TAILQ_INSERT_TAIL(&c_mp->alloc_frozen, frozen_alloc, links);
		}
	}
	MPOOL_REGION_UNLOCK(env, infop);
	/*
	 * If we can't get a frozen buffer header, return ENOMEM immediately:
	 * we don't want to call __memp_alloc recursively.  __memp_alloc will
	 * turn the next free page it finds into frozen buffer headers.
	 */
	if(frozen_bhp == NULL) {
		ret = ENOMEM;
		goto err;
	}
	/*
	 * For now, keep things simple and have one file per page size per
	 * hash bucket.  This improves concurrency but can mean lots of files
	 * if there is lots of freezing.
	 */
	ncache = (uint32)(infop-dbmp->reginfo);
	nbucket = (uint32)(hp-(DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab));
	snprintf(filename, sizeof(filename), "__db.freezer.%lu.%lu.%luK", (ulong)ncache, (ulong)nbucket, (ulong)pagesize/1024);
	if((ret = __db_appname(env, DB_APP_NONE, filename, NULL, &real_name)) != 0)
		goto err;
	MUTEX_LOCK(env, hp->mtx_hash);
	h_locked = 1;
	DB_ASSERT(env, F_ISSET(bhp, BH_EXCLUSIVE) && !F_ISSET(bhp, BH_FROZEN));
	if(BH_REFCOUNT(bhp) > 1 || F_ISSET(bhp, BH_DIRTY)) {
		ret = EBUSY;
		goto err;
	}
	if((ret = __os_open(env, real_name, pagesize, DB_OSO_CREATE|DB_OSO_EXCL, env->db_mode, &fhp)) == 0) {
		/* We're creating the file -- initialize the metadata page. */
		created = 1;
		magic = DB_FREEZER_MAGIC;
		maxpgno = newpgno = 0;
		if((ret = __os_write(env, fhp, &magic, sizeof(uint32), &nio)) != 0 ||
		   (ret = __os_write(env, fhp, &newpgno, sizeof(db_pgno_t), &nio)) != 0 ||
		   (ret = __os_write(env, fhp, &maxpgno, sizeof(db_pgno_t), &nio)) != 0 ||
		   (ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
			goto err;
	}
	else if(ret == EEXIST)
		ret = __os_open(env, real_name, pagesize, 0, env->db_mode, &fhp);
	if(ret != 0)
		goto err;
	if((ret = __os_read(env, fhp, &magic, sizeof(uint32), &nio)) != 0 ||
	   (ret = __os_read(env, fhp, &newpgno, sizeof(db_pgno_t), &nio)) != 0 ||
	   (ret = __os_read(env, fhp, &maxpgno, sizeof(db_pgno_t), &nio)) != 0)
		goto err;
	if(magic != DB_FREEZER_MAGIC) {
		ret = EINVAL;
		goto err;
	}
	if(newpgno == 0) {
		newpgno = ++maxpgno;
		if((ret = __os_seek(env, fhp, 0, 0, sizeof(uint32)+sizeof(db_pgno_t))) != 0 ||
		   (ret = __os_write(env, fhp, &maxpgno, sizeof(db_pgno_t), &nio)) != 0)
			goto err;
	}
	else {
		if((ret = __os_seek(env, fhp, newpgno, pagesize, 0)) != 0 ||
		   (ret = __os_read(env, fhp, &nextfree, sizeof(db_pgno_t), &nio)) != 0)
			goto err;
		if((ret = __os_seek(env, fhp, 0, 0, sizeof(uint32))) != 0 ||
		   (ret = __os_write(env, fhp, &nextfree, sizeof(db_pgno_t), &nio)) != 0)
			goto err;
	}
	/* Write the buffer to the allocated page. */
	if((ret = __os_io(env, DB_IO_WRITE, fhp, newpgno, pagesize, 0, pagesize, bhp->buf, &nio)) != 0)
		goto err;
	ret = __os_closehandle(env, fhp);
	fhp = NULL;
	if(ret != 0)
		goto err;
	/*
	 * Set up the frozen_bhp with the freezer page number.  The original
	 * buffer header is about to be freed, so transfer resources to the
	 * frozen header here.
	 */
	mutex = frozen_bhp->mtx_buf;
#ifdef DIAG_MVCC
	memcpy(frozen_bhp, bhp, SSZ(BH, align_off));
#else
	memcpy(frozen_bhp, bhp, SSZA(BH, buf));
#endif
	atomic_init(&frozen_bhp->ref, 0);
	if(mutex != MUTEX_INVALID)
		frozen_bhp->mtx_buf = mutex;
	else if((ret = __mutex_alloc(env, MTX_MPOOL_BH, DB_MUTEX_SHARED, &frozen_bhp->mtx_buf)) != 0)
		goto err;
	F_SET(frozen_bhp, BH_FROZEN);
	F_CLR(frozen_bhp, BH_EXCLUSIVE);
	((BH_FROZEN_PAGE *)frozen_bhp)->spgno = newpgno;
	/*
	 * We're about to add the frozen buffer header to the version chain, so
	 * we have temporarily created another buffer for the owning
	 * transaction.
	 */
	if(frozen_bhp->td_off != INVALID_ROFF && (ret = __txn_add_buffer(env, BH_OWNER(env, frozen_bhp))) != 0) {
		__env_panic(env, ret);
		goto err;
	}
	STAT_INC(env, mpool, freeze, hp->hash_frozen, bhp->pgno);
	/*
	 * Add the frozen buffer to the version chain and update the hash
	 * bucket if this is the head revision.  The original buffer will be
	 * freed by __memp_alloc calling __memp_bhfree (assuming no other
	 * thread has blocked waiting for it while we were freezing).
	 */
	SH_CHAIN_INSERT_AFTER(bhp, frozen_bhp, vc, __bh);
	if(!SH_CHAIN_HASNEXT(frozen_bhp, vc)) {
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket, bhp, frozen_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
	}
	MUTEX_UNLOCK(env, hp->mtx_hash);
	h_locked = 0;
	/*
	 * Increment the file's block count -- freeing the original buffer will
	 * decrement it.
	 */
	MUTEX_LOCK(env, mfp->mutex);
	++mfp->block_cnt;
	MUTEX_UNLOCK(env, mfp->mutex);
	if(0) {
err:            
		if(fhp != NULL && (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
			ret = t_ret;
		if(created) {
			DB_ASSERT(env, h_locked);
			if((t_ret = __os_unlink(env, real_name, 0)) != 0 && ret == 0)
				ret = t_ret;
		}
		if(h_locked)
			MUTEX_UNLOCK(env, hp->mtx_hash);
		if(ret == 0)
			ret = EIO;
		if(frozen_bhp != NULL) {
			MPOOL_REGION_LOCK(env, infop);
			SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, frozen_bhp, hq);
			MPOOL_REGION_UNLOCK(env, infop);
		}
	}
	if(real_name != NULL)
		__os_free(env, real_name);
	if(ret != 0 && ret != EBUSY && ret != ENOMEM)
		__db_err(env, ret, "__memp_bh_freeze");
	return ret;
}

static int __pgno_cmp(const void * a, const void * b)
{
	db_pgno_t * ap = (db_pgno_t *)a;
	db_pgno_t * bp = (db_pgno_t *)b;
	return (int)(*ap-*bp);
}
/*
 * __memp_bh_thaw --
 *	Free a buffer header in temporary storage.  Optionally restore the
 *	buffer (if alloc_bhp != NULL).  This function should be
 *	called with the hash bucket locked and will return with it unlocked.
 *
 * PUBLIC: int __memp_bh_thaw __P((DB_MPOOL *, REGINFO *,
 * PUBLIC:	DB_MPOOL_HASH *, BH *, BH *));
 */
int __memp_bh_thaw(DB_MPOOL * dbmp, REGINFO * infop, DB_MPOOL_HASH * hp, BH * frozen_bhp, BH * alloc_bhp)
{
#ifdef DIAGNOSTIC
	DB_LSN vlsn;
#endif
	db_mutex_t mutex;
	db_pgno_t * ppgno, freepgno, maxpgno, spgno;
	size_t nio;
	uint32 listsize, magic, nbucket, ncache, ntrunc, nfree;
#ifdef HAVE_FTRUNCATE
	int i;
#endif
	int h_locked, needfree, t_ret;
	char filename[100];
	ENV * env = dbmp->env;
	DB_FH * fhp = NULL;
	MPOOL * c_mp = (MPOOL *)infop->primary;
	MPOOLFILE * mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, frozen_bhp->mf_offset);
	db_pgno_t * freelist = NULL;
	uint32 pagesize = mfp->pagesize;
	int ret = 0;
	char * real_name = NULL;
	MUTEX_REQUIRED(env, hp->mtx_hash);
	DB_ASSERT(env, F_ISSET(frozen_bhp, BH_EXCLUSIVE) || alloc_bhp == NULL);
	h_locked = 1;
	DB_ASSERT(env, F_ISSET(frozen_bhp, BH_FROZEN) && !F_ISSET(frozen_bhp, BH_THAWED));
	DB_ASSERT(env, alloc_bhp  || SH_CHAIN_SINGLETON(frozen_bhp, vc) || (SH_CHAIN_HASNEXT(frozen_bhp, vc) && BH_OBSOLETE(frozen_bhp, hp->old_reader, vlsn)));
	DB_ASSERT(env, !alloc_bhp || !F_ISSET(alloc_bhp, BH_FROZEN));
	spgno = ((BH_FROZEN_PAGE *)frozen_bhp)->spgno;
	if(alloc_bhp != NULL) {
		mutex = alloc_bhp->mtx_buf;
#ifdef DIAG_MVCC
		memcpy(alloc_bhp, frozen_bhp, SSZ(BH, align_off));
#else
		memcpy(alloc_bhp, frozen_bhp, SSZA(BH, buf));
#endif
		alloc_bhp->mtx_buf = mutex;
		MUTEX_LOCK(env, alloc_bhp->mtx_buf);
		atomic_init(&alloc_bhp->ref, 1);
		F_CLR(alloc_bhp, BH_FROZEN);
	}
	//
	// For now, keep things simple and have one file per page size per
	// hash bucket.  This improves concurrency but can mean lots of files if there is lots of freezing.
	//
	ncache = (uint32)(infop-dbmp->reginfo);
	nbucket = (uint32)(hp-(DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab));
	snprintf(filename, sizeof(filename), "__db.freezer.%lu.%lu.%luK", (ulong)ncache, (ulong)nbucket, (ulong)pagesize/1024);
	if((ret = __db_appname(env, DB_APP_NONE, filename, NULL, &real_name)) != 0)
		goto err;
	if((ret = __os_open(env, real_name, pagesize, 0, env->db_mode, &fhp)) != 0)
		goto err;
	// Read the first free page number -- we're about to free the page after we we read it.
	if((ret = __os_read(env, fhp, &magic, sizeof(uint32), &nio)) != 0 ||
	   (ret = __os_read(env, fhp, &freepgno, sizeof(db_pgno_t), &nio)) != 0 ||
	   (ret = __os_read(env, fhp, &maxpgno, sizeof(db_pgno_t), &nio)) != 0)
		goto err;
	if(magic != DB_FREEZER_MAGIC) {
		ret = EINVAL;
		goto err;
	}
	// Read the buffer from the frozen page.
	if(alloc_bhp) {
		DB_ASSERT(env, !F_ISSET(frozen_bhp, BH_FREED));
		if((ret = __os_io(env, DB_IO_READ, fhp, spgno, pagesize, 0, pagesize, alloc_bhp->buf, &nio)) != 0)
			goto err;
	}
	//
	// Free the page from the file.  If it's the last page, truncate.
	// Otherwise, update free page linked list.
	//
	needfree = 1;
	if(spgno == maxpgno) {
		listsize = 100;
		if((ret = __os_malloc(env, listsize*sizeof(db_pgno_t), &freelist)) != 0)
			goto err;
		nfree = 0;
		while(freepgno != 0) {
			if(nfree == listsize-1) {
				listsize *= 2;
				if((ret = __os_realloc(env, listsize*sizeof(db_pgno_t), &freelist)) != 0)
					goto err;
			}
			freelist[nfree++] = freepgno;
			if((ret = __os_seek(env, fhp, freepgno, pagesize, 0)) != 0 || (ret = __os_read(env, fhp, &freepgno, sizeof(db_pgno_t), &nio)) != 0)
				goto err;
		}
		freelist[nfree++] = spgno;
		qsort(freelist, nfree, sizeof(db_pgno_t), __pgno_cmp);
		for(ppgno = &freelist[nfree-1]; ppgno > freelist; ppgno--)
			if(*(ppgno-1) != *ppgno-1)
				break;
		ntrunc = (uint32)(&freelist[nfree]-ppgno);
		if(ntrunc == (uint32)maxpgno) {
			needfree = 0;
			ret = __os_closehandle(env, fhp);
			fhp = NULL;
			if(ret || (ret = __os_unlink(env, real_name, 0)) != 0)
				goto err;
		}
#ifdef HAVE_FTRUNCATE
		else {
			maxpgno -= (db_pgno_t)ntrunc;
			if((ret = __os_truncate(env, fhp, maxpgno+1, pagesize)) != 0)
				goto err;
			/* Fix up the linked list */
			freelist[nfree-ntrunc] = 0;
			if((ret = __os_seek(env, fhp, 0, 0, sizeof(uint32))) != 0 ||
			   (ret = __os_write(env, fhp, &freelist[0], sizeof(db_pgno_t), &nio)) != 0 ||
			   (ret = __os_write(env, fhp, &maxpgno, sizeof(db_pgno_t), &nio)) != 0)
				goto err;
			for(i = 0; i < (int)(nfree-ntrunc); i++)
				if((ret = __os_seek(env, fhp, freelist[i], pagesize, 0)) != 0 || (ret = __os_write(env, fhp, &freelist[i+1], sizeof(db_pgno_t), &nio)) != 0)
					goto err;
			needfree = 0;
		}
#endif
	}
	if(needfree) {
		if((ret = __os_seek(env, fhp, spgno, pagesize, 0)) != 0 ||
		   (ret = __os_write(env, fhp, &freepgno, sizeof(db_pgno_t), &nio)) != 0 ||
		   (ret = __os_seek(env, fhp, 0, 0, sizeof(uint32))) != 0 ||
		   (ret = __os_write(env, fhp, &spgno, sizeof(db_pgno_t), &nio)) != 0)
			goto err;
		ret = __os_closehandle(env, fhp);
		fhp = NULL;
		if(ret != 0)
			goto err;
	}
	/*
	 * Add the thawed buffer (if any) to the version chain.  We can't
	 * do this any earlier, because we can't guarantee that another thread
	 * won't be waiting for it, which means we can't clean up if there are
	 * errors reading from the freezer.  We can't do it any later, because
	 * we're about to free frozen_bhp, and without it we would need to do
	 * another cache lookup to find out where the new page should live.
	 */
	MUTEX_REQUIRED(env, hp->mtx_hash);
	if(alloc_bhp != NULL) {
		alloc_bhp->priority = c_mp->lru_priority;
		SH_CHAIN_INSERT_AFTER(frozen_bhp, alloc_bhp, vc, __bh);
		if(!SH_CHAIN_HASNEXT(alloc_bhp, vc)) {
			SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket, frozen_bhp, alloc_bhp, hq, __bh);
			SH_TAILQ_REMOVE(&hp->hash_bucket, frozen_bhp, hq, __bh);
		}
	}
	else if(!SH_CHAIN_HASNEXT(frozen_bhp, vc)) {
		if(SH_CHAIN_HASPREV(frozen_bhp, vc))
			SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket, frozen_bhp, SH_CHAIN_PREV(frozen_bhp, vc, __bh), hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, frozen_bhp, hq, __bh);
	}
	SH_CHAIN_REMOVE(frozen_bhp, vc, __bh);
	if(!alloc_bhp && frozen_bhp->td_off != INVALID_ROFF && (ret = __txn_remove_buffer(env, BH_OWNER(env, frozen_bhp), MUTEX_INVALID)) != 0) {
		__env_panic(env, ret);
		goto err;
	}
	frozen_bhp->td_off = INVALID_ROFF;
	/*
	 * If other threads are waiting for this buffer as well, they will have
	 * incremented the reference count and will be waiting on the mutex.
	 * For that reason, we can't unconditionally free the memory here.
	 */
	needfree = (atomic_dec(env, &frozen_bhp->ref) == 0);
	if(!needfree)
		F_SET(frozen_bhp, BH_THAWED);
	MUTEX_UNLOCK(env, hp->mtx_hash);
	if(F_ISSET(frozen_bhp, BH_EXCLUSIVE))
		MUTEX_UNLOCK(env, frozen_bhp->mtx_buf);
	h_locked = 0;
	if(needfree) {
		MPOOL_REGION_LOCK(env, infop);
		SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, frozen_bhp, hq);
		MPOOL_REGION_UNLOCK(env, infop);
	}
#ifdef HAVE_STATISTICS
	if(alloc_bhp != NULL)
		STAT_INC_VERB(env, mpool, thaw, hp->hash_thawed, __memp_fns(dbmp, mfp), frozen_bhp->pgno);
	else
		STAT_INC_VERB(env, mpool, free_frozen, hp->hash_frozen_freed, __memp_fns(dbmp, mfp), frozen_bhp->pgno);
#endif
	if(0) {
err:            
		if(h_locked)
			MUTEX_UNLOCK(env, hp->mtx_hash);
		SETIFZ(ret, EIO);
	}
	__os_free(env, real_name);
	__os_free(env, freelist);
	if(fhp != NULL && (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;
	if(ret != 0)
		__db_err(env, ret, "__memp_bh_thaw");
	return ret;
}
