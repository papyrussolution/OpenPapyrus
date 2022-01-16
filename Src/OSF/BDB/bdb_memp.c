// BDB_MEMP.C
// See the file LICENSE for redistribution information.
// Copyright (c) 1999, 2011 Oracle and/or its affiliates.  All rights reserved.
//
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop

typedef struct {
	DB_MPOOL_HASH * track_hp; // Hash bucket
	roff_t track_off;         // Page file offset
	db_pgno_t track_pgno;     // Page number
} BH_TRACK;

static int __memp_pgwrite(ENV*, DB_MPOOLFILE*, DB_MPOOL_HASH*, BH *);
static int __memp_get_clear_len(const DB_MPOOLFILE*, uint32 *);
static int __memp_get_lsn_offset(const DB_MPOOLFILE*, int32 *);
static int __memp_get_maxsize(DB_MPOOLFILE*, uint32*, uint32 *);
static int __memp_set_maxsize(DB_MPOOLFILE*, uint32, uint32);
static int __memp_set_priority(DB_MPOOLFILE*, DB_CACHE_PRIORITY);
static int __memp_get_last_pgno_pp(DB_MPOOLFILE*, db_pgno_t *);
static int __memp_mpf_alloc(DB_MPOOL*, DB_MPOOLFILE*, const char *, uint32, uint32, MPOOLFILE**);
static int __memp_mpf_find(ENV*, DB_MPOOLFILE*, DB_MPOOL_HASH*, const char *, uint32, MPOOLFILE**);
static int __pgno_cmp(const void *, const void *);
static int __memp_init_config(ENV*, MPOOL *);
static void __memp_region_size(ENV*, roff_t*, uint32 *);
static int FASTCALL __memp_map_regions(DB_MPOOL *);
static int __memp_merge_buckets(DB_MPOOL*, uint32, uint32, uint32);
static int __bhcmp(const void *, const void *);
static int __memp_close_flush_files(ENV*, int);
static int __memp_sync_files(ENV *);
static int __memp_sync_file(ENV*, MPOOLFILE*, void *, uint32*, uint32);
static int __memp_trickle(ENV *, int, int *);

#ifdef HAVE_STATISTICS // {
static void __memp_print_bh(ENV*, DB_MPOOL*, const char *, BH*, roff_t *);
static int __memp_print_all(ENV*, uint32);
static int __memp_print_stats(ENV*, uint32);
static int __memp_print_hash(ENV*, DB_MPOOL*, REGINFO*, roff_t*, uint32);
static int __memp_stat(ENV*, DB_MPOOL_STAT**, DB_MPOOL_FSTAT***, uint32);
static void __memp_stat_wait(ENV*, REGINFO*, MPOOL*, DB_MPOOL_STAT*, uint32);
static int __memp_file_stats(ENV*, MPOOLFILE*, void *, uint32*, uint32);
static int __memp_count_files(ENV*, MPOOLFILE*, void *, uint32*, uint32);
static int __memp_get_files(ENV*, MPOOLFILE*, void *, uint32*, uint32);
static int __memp_print_files(ENV*, MPOOLFILE*, void *, uint32*, uint32);
#endif // } HAVE_STATISTICS

#define MPOOL_DEFAULT_PAGESIZE  SKILOBYTE(4)
/*
 * This configuration parameter limits the number of hash buckets which
 * __memp_alloc() searches through while excluding buffers with a 'high' priority.
 */
#if !defined(MPOOL_ALLOC_SEARCH_LIMIT)
	#define MPOOL_ALLOC_SEARCH_LIMIT        500
#endif
/*
 * __memp_alloc --
 *	Allocate some space from a cache region.
 */
int __memp_alloc(DB_MPOOL * dbmp, REGINFO * infop, MPOOLFILE * mfp, size_t len, roff_t * offsetp, void * retp)
{
	BH * bhp, * current_bhp, * mvcc_bhp, * oldest_bhp;
	BH_FROZEN_PAGE * frozen_bhp;
	DB_LSN vlsn;
	DB_MPOOL_HASH * hp, * hp_tmp;
	MPOOLFILE * bh_mfp;
	size_t freed_space;
	uint32 bucket_priority, cache_reduction;
	uint32 high_priority, priority;
	uint32 lru_generation;
	int b_lock;
	int need_free, obsolete, ret;
	uint8 * endp;
	void * p;
	ENV * env = dbmp->env;
	MPOOL * c_mp = (MPOOL *)infop->primary;
	DB_MPOOL_HASH * dbht = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab);
	DB_MPOOL_HASH * hp_end = &dbht[c_mp->htab_buckets];
	DB_MPOOL_HASH * hp_saved = NULL;
	uint32 priority_saved = 0;
	int    write_error = 0;
	uint32 buckets = 0;
	uint32 buffers = 0;
	uint32 put_counter = 0;
	uint32 total_buckets = 0;
	int    aggressive = 0;
	int    alloc_freeze = 0;
	int    giveup = 0;
	int    got_oldest = 0;
	int    h_locked = 0;
	/*
	 * If we're allocating a buffer, and the one we're discarding is the
	 * same size, we don't want to waste the time to re-integrate it into
	 * the shared memory free list.  If the DB_MPOOLFILE argument isn't
	 * NULL, we'll compare the underlying page sizes of the two buffers
	 * before free-ing and re-allocating buffers.
	 */
	if(mfp) {
		len = SSZA(BH, buf)+mfp->pagesize;
		// Add space for alignment padding for MVCC diagnostics
		MVCC_BHSIZE(mfp, len);
	}
	STAT_INC(env, mpool, nallocs, c_mp->stat.st_alloc, len);
	MPOOL_REGION_LOCK(env, infop);
	/*
	 * First we try to allocate from free memory.  If that fails, scan the
	 * buffer pool to find buffers with low priorities.  We consider small
	 * sets of hash buckets each time to limit the amount of work needing
	 * to be done.  This approximates LRU, but not very well.  We either
	 * find a buffer of the same size to use, or we will free 3 times what
	 * we need in the hopes it will coalesce into a contiguous chunk of the
	 * right size.  In the latter case we branch back here and try again.
	 */
alloc:
	if((ret = __env_alloc(infop, len, &p)) == 0) {
		if(mfp) {
			//
			// For MVCC diagnostics, align the pointer so that the buffer starts on a page boundary.
			//
			MVCC_BHALIGN(p);
			bhp = (BH *)p;
			if((ret = __mutex_alloc(env, MTX_MPOOL_BH, DB_MUTEX_SHARED, &bhp->mtx_buf)) != 0) {
				MVCC_BHUNALIGN(bhp);
				__env_alloc_free(infop, bhp);
				goto search;
			}
			c_mp->pages++;
		}
		MPOOL_REGION_UNLOCK(env, infop);
found:
		ASSIGN_PTR(offsetp, R_OFFSET(infop, p));
		*(void **)retp = p;
		//
		// Update the search statistics.
		//
		// We're not holding the region locked here, these statistics can't be trusted.
		//
#ifdef HAVE_STATISTICS
		total_buckets += buckets;
		if(total_buckets != 0) {
			if(total_buckets > c_mp->stat.st_alloc_max_buckets)
				STAT_SET(env, mpool, alloc_max_buckets, c_mp->stat.st_alloc_max_buckets, total_buckets, infop->id);
			STAT_ADJUST(env, mpool, alloc_buckets, c_mp->stat.st_alloc_buckets, total_buckets, infop->id);
		}
		if(buffers != 0) {
			if(buffers > c_mp->stat.st_alloc_max_pages)
				STAT_SET(env, mpool, alloc_max_pages, c_mp->stat.st_alloc_max_pages, buffers, infop->id);
			STAT_ADJUST(env, mpool, alloc_pages, c_mp->stat.st_alloc_pages, buffers, infop->id);
		}
#endif
		return 0;
	}
	else if(giveup || c_mp->pages == 0) {
		MPOOL_REGION_UNLOCK(env, infop);
		__db_errx(env, DB_STR("3017", "unable to allocate space from the buffer cache"));
		return (ret == ENOMEM && write_error != 0) ? EIO : ret;
	}
search:
	/*
	 * Anything newer than 1/10th of the buffer pool is ignored during the
	 * first MPOOL_SEARCH_ALLOC_LIMIT buckets worth of allocation.
	 */
	cache_reduction = c_mp->pages/10;
	high_priority = aggressive ? MPOOL_LRU_MAX : c_mp->lru_priority-cache_reduction;
	lru_generation = c_mp->lru_generation;
	ret = 0;
	//
	// We re-attempt the allocation every time we've freed 3 times what we need.  Reset our free-space counter.
	//
	freed_space = 0;
	total_buckets += buckets;
	buckets = 0;
	//
	// Walk the hash buckets and find the next two with potentially useful
	// buffers.  Free the buffer with the lowest priority from the buckets' chains.
	//
	for(;; ) {
		// All pages have been freed, make one last try
		if(c_mp->pages == 0)
			goto alloc;
		// Check for wrap around
		hp = &dbht[c_mp->last_checked++];
		if(hp >= hp_end) {
			c_mp->last_checked = 0;
			hp = &dbht[c_mp->last_checked++];
		}
		/*
		 * The failure mode is when there are too many buffers we can't
		 * write or there's not enough memory in the system to support
		 * the number of pinned buffers.
		 *
		 * Get aggressive if we've reviewed the entire cache without
		 * freeing the needed space.  (The code resets "aggressive"
		 * when we free any space.)  Aggressive means:
		 *
		 * a: set a flag to attempt to flush high priority buffers as
		 *  well as other buffers.
		 * b: look at a buffer in every hash bucket rather than choose
		 *  the more preferable of two.
		 * c: start to think about giving up.
		 *
		 * If we get here three or more times, sync the mpool to force
		 * out queue extent pages.  While we might not have enough
		 * space for what we want and flushing is expensive, why not?
		 * Then sleep for a second, hopefully someone else will run and
		 * free up some memory.
		 *
		 * Always try to allocate memory too, in case some other thread
		 * returns its memory to the region.
		 *
		 * We don't have any way to know an allocation has no way to
		 * succeed.  Fail if no pages are returned to the cache after
		 * we've been trying for a relatively long time.
		 *
		 * !!!
		 * This test ignores pathological cases like no buffers in the
		 * system -- we check for that early on, so it isn't possible.
		 */
		if(buckets++ == c_mp->htab_buckets) {
			if(freed_space > 0)
				goto alloc;
			MPOOL_REGION_UNLOCK(env, infop);
			aggressive++;
			/*
			 * Once aggressive, we consider all buffers. By setting
			 * this to MPOOL_LRU_MAX, we'll still select a victim
			 * even if all buffers have the highest normal priority.
			 */
			high_priority = MPOOL_LRU_MAX;
			PERFMON4(env, mpool, alloc_wrap, len, infop->id, aggressive, c_mp->put_counter);
			switch(aggressive) {
			    case 1: break;
			    case 2: put_counter = c_mp->put_counter; break;
			    case 3:
			    case 4:
			    case 5:
			    case 6:
					__memp_sync_int(env, NULL, 0, DB_SYNC_ALLOC, 0, 0);
					__os_yield(env, 1, 0);
					break;
			    default:
					aggressive = 1;
					if(put_counter == c_mp->put_counter)
						giveup = 1;
					break;
			}
			MPOOL_REGION_LOCK(env, infop);
			goto alloc;
		}
		/*
		 * Skip empty buckets.
		 *
		 * We can check for empty buckets before locking the hash
		 * bucket as we only care if the pointer is zero or non-zero.
		 */
		if(SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL)
			continue;
		/* Set aggressive if we have already searched for too long. */
		if(aggressive == 0 && buckets >= MPOOL_ALLOC_SEARCH_LIMIT) {
			aggressive = 1;
			/* Once aggressive, we consider all buffers. */
			high_priority = MPOOL_LRU_MAX;
		}
		/* Unlock the region and lock the hash bucket. */
		MPOOL_REGION_UNLOCK(env, infop);
		MUTEX_READLOCK(env, hp->mtx_hash);
		h_locked = 1;
		b_lock = 0;

		/*
		 * Find a buffer we can use.
		 *
		 * We use the lowest-LRU singleton buffer if we find one and
		 * it's better than the result of another hash bucket we've
		 * reviewed.  We do not use a buffer which has a priority
		 * greater than high_priority unless we are being aggressive.
		 *
		 * With MVCC buffers, the situation is more complicated: we
		 * don't want to free a buffer out of the middle of an MVCC
		 * chain, since that requires I/O.  So, walk the buffers,
		 * looking for an obsolete buffer at the end of an MVCC chain.
		 * Once a buffer becomes obsolete, its LRU priority is
		 * irrelevant because that version can never be accessed again.
		 *
		 * If we don't find any obsolete MVCC buffers, we will get
		 * aggressive, and in that case consider the lowest priority
		 * buffer within a chain.
		 *
		 * Ignore referenced buffers, we can't get rid of them.
		 */
retry_search:
		bhp = NULL;
		bucket_priority = high_priority;
		obsolete = 0;
		SH_TAILQ_FOREACH(current_bhp, &hp->hash_bucket, hq, __bh) {
			/*
			 * First, do the standard LRU check for singletons.
			 * We can use the buffer if it is unreferenced, has a
			 * priority that isn't too high (unless we are
			 * aggressive), and is better than the best candidate
			 * we have found so far in this bucket.
			 */
			if(SH_CHAIN_SINGLETON(current_bhp, vc)) {
				if(BH_REFCOUNT(current_bhp) == 0 && bucket_priority > current_bhp->priority) {
					bucket_priority = current_bhp->priority;
					if(bhp)
						atomic_dec(env, &bhp->ref);
					bhp = current_bhp;
					atomic_inc(env, &bhp->ref);
				}
				continue;
			}
			/*
			 * For MVCC buffers, walk through the chain.  If we are
			 * aggressive, choose the best candidate from within
			 * the chain for freezing.
			 */
			for(mvcc_bhp = oldest_bhp = current_bhp; mvcc_bhp; oldest_bhp = mvcc_bhp, mvcc_bhp = SH_CHAIN_PREV(mvcc_bhp, vc, __bh)) {
				DB_ASSERT(env, mvcc_bhp != SH_CHAIN_PREV(mvcc_bhp, vc, __bh));
				if(aggressive > 1 && BH_REFCOUNT(mvcc_bhp) == 0 && !F_ISSET(mvcc_bhp, BH_FROZEN) && (!bhp || bhp->priority > mvcc_bhp->priority)) {
					if(bhp)
						atomic_dec(env, &bhp->ref);
					bhp = mvcc_bhp;
					atomic_inc(env, &bhp->ref);
				}
			}
			/*
			 * oldest_bhp is the last buffer on the MVCC chain, and
			 * an obsolete buffer at the end of the MVCC chain gets
			 * used without further search.
			 *
			 * If the buffer isn't obsolete with respect to the
			 * cached old reader LSN, recalculate the oldest reader
			 * LSN and check again.
			 */
			if(BH_REFCOUNT(oldest_bhp) != 0)
				continue;
retry_obsolete:
			if(BH_OBSOLETE(oldest_bhp, hp->old_reader, vlsn)) {
				obsolete = 1;
				if(bhp)
					atomic_dec(env, &bhp->ref);
				bhp = oldest_bhp;
				atomic_inc(env, &bhp->ref);
				goto this_buffer;
			}
			if(!got_oldest) {
				if((ret = __txn_oldest_reader(env, &hp->old_reader)) != 0)
					return ret;
				got_oldest = 1;
				goto retry_obsolete;
			}
		}
		/*
		 * bhp is either NULL or the best candidate buffer.
		 * We'll use the chosen buffer only if we have compared its
		 * priority against one chosen from another hash bucket.
		 */
		if(bhp == NULL)
			goto next_hb;
		priority = bhp->priority;
		/*
		 * Compare two hash buckets and select the one with the lower
		 * priority. Performance testing showed looking at two improves
		 * the LRU-ness and looking at more only does a little better.
		 */
		if(hp_saved == NULL) {
			hp_saved = hp;
			priority_saved = priority;
			goto next_hb;
		}
		/*
		 * If the buffer we just found is a better choice than our
		 * previous choice, use it.
		 *
		 * If the previous choice was better, pretend we're moving
		 * from this hash bucket to the previous one and re-do the
		 * search.
		 *
		 * We don't worry about simply swapping between two buckets
		 * because that could only happen if a buffer was removed
		 * from the chain, or its priority updated.   If a buffer
		 * is removed from the chain, some other thread has managed
		 * to discard a buffer, so we're moving forward.  Updating
		 * a buffer's priority will make it a high-priority buffer,
		 * so we'll ignore it when we search again, and so we will
		 * eventually zero in on a buffer to use, or we'll decide
		 * there are no buffers we can use.
		 *
		 * If there's only a single hash bucket with buffers, we'll
		 * search the bucket once, choose a buffer, walk the entire
		 * list of buckets and search it again.   In the case of a
		 * system that's busy, it's possible to imagine a case where
		 * we'd loop for a long while.  For that reason, and because
		 * the test is easy, we special case and test for it.
		 */
		if(priority > priority_saved && hp != hp_saved) {
			MUTEX_UNLOCK(env, hp->mtx_hash);
			hp_tmp = hp_saved;
			hp_saved = hp;
			hp = hp_tmp;
			priority_saved = priority;
			MUTEX_READLOCK(env, hp->mtx_hash);
			h_locked = 1;
			DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
			atomic_dec(env, &bhp->ref);
			goto retry_search;
		}
		/*
		 * If another thread has called __memp_reset_lru() while we were
		 * looking for this buffer, it is possible that we've picked a
		 * poor choice for a victim. If so toss it and start over.
		 */
		if(lru_generation != c_mp->lru_generation) {
			DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
			atomic_dec(env, &bhp->ref);
			MUTEX_UNLOCK(env, hp->mtx_hash);
			MPOOL_REGION_LOCK(env, infop);
			hp_saved = NULL;
			goto search;
		}
this_buffer:
		buffers++;
		//
		// Discard any previously remembered hash bucket, we've got a winner.
		//
		hp_saved = NULL;
		// Drop the hash mutex and lock the buffer exclusively
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		// Don't bother trying to latch a busy buffer
		if(BH_REFCOUNT(bhp) > 1)
			goto next_hb;
		// We cannot block as the caller is probably holding locks
		if((ret = MUTEX_TRYLOCK(env, bhp->mtx_buf)) != 0) {
			if(ret != DB_LOCK_NOTGRANTED)
				return ret;
			goto next_hb;
		}
		F_SET(bhp, BH_EXCLUSIVE);
		b_lock = 1;
		// Someone may have grabbed it while we got the lock
		if(BH_REFCOUNT(bhp) != 1)
			goto next_hb;
		/* Find the associated MPOOLFILE. */
		bh_mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
		/* If the page is dirty, write it. */
		ret = 0;
		if(F_ISSET(bhp, BH_DIRTY)) {
			DB_ASSERT(env, atomic_read(&hp->hash_page_dirty) > 0);
			ret = __memp_bhwrite(dbmp, hp, bh_mfp, bhp, 0);
			DB_ASSERT(env, atomic_read(&bhp->ref) > 0);
			/*
			 * If a write fails for any reason, we can't proceed.
			 *
			 * If there's a write error and we're having problems
			 * finding something to allocate, avoid selecting this
			 * buffer again by maximizing its priority.
			 */
			if(ret != 0) {
				if(ret != EPERM) {
					write_error++;
					__db_errx(env, DB_STR_A("3018", "%s: unwritable page %d remaining in the cache after error %d", "%s %d %d"), __memp_fns(dbmp, bh_mfp), bhp->pgno, ret);
				}
				bhp->priority = MPOOL_LRU_REDZONE;
				goto next_hb;
			}
			STAT_INC(env, mpool, dirty_eviction, c_mp->stat.st_rw_evict, infop->id);
		}
		else
			STAT_INC(env, mpool, clean_eviction, c_mp->stat.st_ro_evict, infop->id);
		//
		// Freeze this buffer, if necessary.  That is, if the buffer is
		// part of an MVCC chain and could be required by a reader.
		//
		if(SH_CHAIN_HASPREV(bhp, vc) || (SH_CHAIN_HASNEXT(bhp, vc) && !obsolete)) {
			if(!aggressive || F_ISSET(bhp, BH_DIRTY|BH_FROZEN))
				goto next_hb;
			ret = __memp_bh_freeze(dbmp, infop, hp, bhp, &alloc_freeze);
			if(ret == EIO)
				write_error++;
			if(oneof4(ret, EBUSY, EIO, ENOMEM, ENOSPC)) {
				ret = 0;
				goto next_hb;
			}
			else if(ret != 0) {
				DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
				atomic_dec(env, &bhp->ref);
				DB_ASSERT(env, b_lock);
				F_CLR(bhp, BH_EXCLUSIVE);
				MUTEX_UNLOCK(env, bhp->mtx_buf);
				DB_ASSERT(env, !h_locked);
				return ret;
			}
		}
		MUTEX_LOCK(env, hp->mtx_hash);
		h_locked = 1;
		/*
		 * We released the hash bucket lock while doing I/O, so another
		 * thread may have acquired this buffer and incremented the ref
		 * count or dirtied the buffer or installed a new version after
		 * we wrote it, in which case we can't have it.
		 */
		if(BH_REFCOUNT(bhp) != 1 || F_ISSET(bhp, BH_DIRTY) || (SH_CHAIN_HASNEXT(bhp, vc) &&
			SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off != bhp->td_off && !BH_OBSOLETE(bhp, hp->old_reader, vlsn)))
			goto next_hb;
		/*
		 * If the buffer is frozen, thaw it and look for another one
		 * we can use. (Calling __memp_bh_freeze above will not mark bhp BH_FROZEN.)
		 */
		if(F_ISSET(bhp, BH_FROZEN)) {
			DB_ASSERT(env, obsolete || SH_CHAIN_SINGLETON(bhp, vc));
			DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
			if(!F_ISSET(bhp, BH_THAWED)) {
				/*
				 * This call releases the hash bucket mutex.
				 * We're going to retry the search, so we need to re-lock it.
				 */
				if((ret = __memp_bh_thaw(dbmp, infop, hp, bhp, NULL)) != 0)
					return ret;
				MUTEX_READLOCK(env, hp->mtx_hash);
			}
			else {
				need_free = (atomic_dec(env, &bhp->ref) == 0);
				F_CLR(bhp, BH_EXCLUSIVE);
				MUTEX_UNLOCK(env, bhp->mtx_buf);
				if(need_free) {
					MPOOL_REGION_LOCK(env, infop);
					SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, bhp, hq);
					MPOOL_REGION_UNLOCK(env, infop);
				}
			}
			bhp = NULL;
			b_lock = alloc_freeze = 0;
			goto retry_search;
		}
		/*
		 * If we need some empty buffer headers for freezing, turn the
		 * buffer we've found into frozen headers and put them on the
		 * free list.  Only reset alloc_freeze if we've actually
		 * allocated some frozen buffer headers.
		 */
		if(alloc_freeze) {
			if((ret = __memp_bhfree(dbmp, infop, bh_mfp, hp, bhp, 0)) != 0)
				return ret;
			b_lock = 0;
			h_locked = 0;
			MVCC_MPROTECT(bhp->buf, bh_mfp->pagesize, PROT_READ|PROT_WRITE|PROT_EXEC);
			MPOOL_REGION_LOCK(env, infop);
			SH_TAILQ_INSERT_TAIL(&c_mp->alloc_frozen, (BH_FROZEN_ALLOC *)bhp, links);
			frozen_bhp = (BH_FROZEN_PAGE *)((BH_FROZEN_ALLOC *)bhp+1);
			endp = (uint8 *)bhp->buf+bh_mfp->pagesize;
			while((uint8 *)(frozen_bhp+1) < endp) {
				frozen_bhp->header.mtx_buf = MUTEX_INVALID;
				SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, (BH *)frozen_bhp, hq);
				frozen_bhp++;
			}
			MPOOL_REGION_UNLOCK(env, infop);
			alloc_freeze = 0;
			MUTEX_READLOCK(env, hp->mtx_hash);
			h_locked = 1;
			goto retry_search;
		}
		/*
		 * Check to see if the buffer is the size we're looking for.
		 * If so, we can simply reuse it.  Otherwise, free the buffer
		 * and its space and keep looking.
		 */
		if(mfp && mfp->pagesize == bh_mfp->pagesize) {
			if((ret = __memp_bhfree(dbmp, infop, bh_mfp, hp, bhp, 0)) != 0)
				return ret;
			p = bhp;
			goto found;
		}
		freed_space += sizeof(*bhp)+bh_mfp->pagesize;
		if((ret = __memp_bhfree(dbmp, infop, bh_mfp, hp, bhp, BH_FREE_FREEMEM)) != 0)
			return ret;
		// Reset "aggressive" and "write_error" if we free any space.
		SETMIN(aggressive, 1);
		write_error = 0;
		/*
		 * Unlock this buffer and re-acquire the region lock. If
		 * we're reaching here as a result of calling memp_bhfree, the
		 * buffer lock has already been discarded.
		 */
		if(0) {
next_hb:
			if(bhp) {
				DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
				atomic_dec(env, &bhp->ref);
				if(b_lock) {
					F_CLR(bhp, BH_EXCLUSIVE);
					MUTEX_UNLOCK(env, bhp->mtx_buf);
				}
			}
			if(h_locked)
				MUTEX_UNLOCK(env, hp->mtx_hash);
			h_locked = 0;
		}
		MPOOL_REGION_LOCK(env, infop);
		/*
		 * Retry the allocation as soon as we've freed up sufficient
		 * space.  We're likely to have to coalesce of memory to
		 * satisfy the request, don't try until it's likely (possible?) we'll succeed.
		 */
		if(freed_space >= 3*len)
			goto alloc;
	}
	/* NOTREACHED */
}
//
// __memp_free --
// Free some space from a cache region.
//
void FASTCALL __memp_free(REGINFO * infop, void * buf)
{
	__env_alloc_free(infop, buf);
}
/*
 * __memp_bhwrite --
 *	Write the page associated with a given buffer header.
 */
int __memp_bhwrite(DB_MPOOL * dbmp, DB_MPOOL_HASH * hp, MPOOLFILE * mfp, BH * bhp, int open_extents)
{
	DB_MPOOLFILE * dbmfp;
	DB_MPREG * mpreg;
	int ret;
	ENV * env = dbmp->env;
	/*
	 * If the file has been removed or is a closed temporary file, we're
	 * done -- the page-write function knows how to handle the fact that
	 * we don't have (or need!) any real file descriptor information.
	 */
	if(mfp->deadfile)
		return __memp_pgwrite(env, NULL, hp, bhp);
	/*
	 * Walk the process' DB_MPOOLFILE list and find a file descriptor for
	 * the file.  We also check that the descriptor is open for writing.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q)
	if(dbmfp->mfp == mfp && !F_ISSET(dbmfp, MP_READONLY)) {
		++dbmfp->ref;
		break;
	}
	MUTEX_UNLOCK(env, dbmp->mutex);
	if(dbmfp) {
		/*
		 * Temporary files may not have been created.  We only handle
		 * temporary files in this path, because only the process that
		 * created a temporary file will ever flush buffers to it.
		 */
		if(dbmfp->fhp == NULL) {
			/* We may not be allowed to create backing files. */
			if(mfp->no_backing_file) {
				--dbmfp->ref;
				return EPERM;
			}
			MUTEX_LOCK(env, dbmp->mutex);
			if(dbmfp->fhp == NULL) {
				ret = __db_tmp_open(env, F_ISSET(env->dbenv, DB_ENV_DIRECT_DB) ? DB_OSO_DIRECT : 0, &dbmfp->fhp);
			}
			else
				ret = 0;
			MUTEX_UNLOCK(env, dbmp->mutex);
			if(ret != 0) {
				__db_errx(env, DB_STR("3014", "unable to create temporary backing file"));
				--dbmfp->ref;
				return ret;
			}
		}
		goto pgwrite;
	}
	/*
	 * There's no file handle for this file in our process.
	 *
	 * !!!
	 * It's the caller's choice if we're going to open extent files.
	 */
	if(!open_extents && F_ISSET(mfp, MP_EXTENT))
		return EPERM;
	/*
	 * !!!
	 * Don't try to attach to temporary files.  There are two problems in
	 * trying to do that.  First, if we have different privileges than the
	 * process that "owns" the temporary file, we might create the backing
	 * disk file such that the owning process couldn't read/write its own
	 * buffers, e.g., memp_trickle running as root creating a file owned
	 * as root, mode 600.  Second, if the temporary file has already been
	 * created, we don't have any way of finding out what its real name is,
	 * and, even if we did, it was already unlinked (so that it won't be
	 * left if the process dies horribly).  This decision causes a problem,
	 * however: if the temporary file consumes the entire buffer cache,
	 * and the owner doesn't flush the buffers to disk, we could end up
	 * with resource starvation, and the memp_trickle thread couldn't do
	 * anything about it.  That's a pretty unlikely scenario, though.
	 *
	 * Note we should never get here when the temporary file in question
	 * has already been closed in another process, in which case it should
	 * be marked dead.
	 */
	if(F_ISSET(mfp, MP_TEMP) || mfp->no_backing_file)
		return EPERM;
	/*
	 * It's not a page from a file we've opened.  If the file requires
	 * application-specific input/output processing, see if this process
	 * has ever registered information as to how to write this type of
	 * file.  If not, there's nothing we can do.
	 */
	if(mfp->ftype != 0 && mfp->ftype != DB_FTYPE_SET) {
		MUTEX_LOCK(env, dbmp->mutex);
		LIST_FOREACH(mpreg, &dbmp->dbregq, q)
		if(mpreg->ftype == mfp->ftype)
			break;
		MUTEX_UNLOCK(env, dbmp->mutex);
		if(mpreg == NULL)
			return EPERM;
	}
	/*
	 * Try and open the file, specifying the known underlying shared area.
	 *
	 * !!!
	 * There's no negative cache, so we may repeatedly try and open files
	 * that we have previously tried (and failed) to open.
	 */
	if((ret = __memp_fcreate(env, &dbmfp)) != 0)
		return ret;
	if((ret = __memp_fopen(dbmfp, mfp, NULL, NULL, DB_DURABLE_UNKNOWN, 0, mfp->pagesize)) != 0) {
		__memp_fclose(dbmfp, 0);
		/*
		 * Ignore any error if the file is marked dead, assume the file
		 * was removed from under us.
		 */
		if(!mfp->deadfile)
			return ret;
		dbmfp = NULL;
	}
pgwrite:
	MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ|PROT_WRITE|PROT_EXEC);
	ret = __memp_pgwrite(env, dbmfp, hp, bhp);
	if(dbmfp == NULL)
		return ret;
	/*
	 * Discard our reference, and, if we're the last reference, make sure
	 * the file eventually gets closed.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	if(dbmfp->ref == 1)
		F_SET(dbmfp, MP_FLUSH);
	else
		--dbmfp->ref;
	MUTEX_UNLOCK(env, dbmp->mutex);
	return ret;
}
/*
 * __memp_pgread --
 *	Read a page from a file.
 */
int __memp_pgread(DB_MPOOLFILE * dbmfp, BH * bhp, int can_create)
{
	size_t len, nr;
	int ret;
	ENV * env = dbmfp->env;
	MPOOLFILE * mfp = dbmfp->mfp;
	uint32 pagesize = mfp->pagesize;
	/* We should never be called with a dirty or unlocked buffer. */
	DB_ASSERT(env, !F_ISSET(bhp, BH_DIRTY_CREATE|BH_FROZEN));
	DB_ASSERT(env, can_create || F_ISSET(bhp, BH_TRASH) || !F_ISSET(bhp, BH_DIRTY));
	DB_ASSERT(env, F_ISSET(bhp, BH_EXCLUSIVE));

	/* Mark the buffer as in transistion. */
	F_SET(bhp, BH_TRASH);

	/*
	 * Temporary files may not yet have been created.  We don't create
	 * them now, we create them when the pages have to be flushed.
	 */
	nr = 0;
	if(dbmfp->fhp) {
		PERFMON3(env, mpool, read, __memp_fn(dbmfp), bhp->pgno, bhp);
		if((ret = __os_io(env, DB_IO_READ, dbmfp->fhp, bhp->pgno, pagesize, 0, pagesize, bhp->buf, &nr)) != 0)
			goto err;
	}
	/*
	 * The page may not exist; if it doesn't, nr may well be 0, but we
	 * expect the underlying OS calls not to return an error code in
	 * this case.
	 */
	if(nr < pagesize) {
		/*
		 * Don't output error messages for short reads.  In particular,
		 * DB recovery processing may request pages never written to
		 * disk or for which only some part have been written to disk,
		 * in which case we won't find the page.  The caller must know
		 * how to handle the error.
		 */
		if(!can_create) {
			ret = DB_PAGE_NOTFOUND;
			goto err;
		}
		/* Clear any bytes that need to be cleared. */
		len = mfp->clear_len == DB_CLEARLEN_NOTSET ? pagesize : mfp->clear_len;
		memzero(bhp->buf, len);
#if defined(DIAGNOSTIC) || defined(UMRW)
		/*
		 * If we're running in diagnostic mode, corrupt any bytes on
		 * the page that are unknown quantities for the caller.
		 */
		if(len < pagesize)
			memset(bhp->buf+len, CLEAR_BYTE, pagesize-len);
#endif
		STAT_INC_VERB(env, mpool, page_create, mfp->stat.st_page_create, __memp_fn(dbmfp), bhp->pgno);
	}
	else
		STAT_INC_VERB(env, mpool, page_in, mfp->stat.st_page_in, __memp_fn(dbmfp), bhp->pgno);
	// Call any pgin function
	ret = mfp->ftype == 0 ? 0 : __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1);
	//
	// If no errors occurred, the data is now valid, clear the BH_TRASH flag.
	//
	if(!ret)
		F_CLR(bhp, BH_TRASH);
err:
	return ret;
}
/*
 * __memp_pgwrite --
 *	Write a page to a file.
 */
static int __memp_pgwrite(ENV * env, DB_MPOOLFILE * dbmfp, DB_MPOOL_HASH * hp, BH * bhp)
{
	DB_LSN lsn;
	MPOOLFILE * mfp;
	size_t nw;
	int ret = 0;
	void * buf = 0;
	/*
	 * Since writing does not require exclusive access, another thread
	 * could have already written this buffer.
	 */
	if(!F_ISSET(bhp, BH_DIRTY))
		return 0;
	mfp = dbmfp == NULL ? NULL : dbmfp->mfp;
	// We should never be called with a frozen or trashed buffer
	DB_ASSERT(env, !F_ISSET(bhp, BH_FROZEN|BH_TRASH));
	/*
	 * It's possible that the underlying file doesn't exist, either
	 * because of an outright removal or because it was a temporary
	 * file that's been closed.
	 *
	 * !!!
	 * Once we pass this point, we know that dbmfp and mfp aren't NULL,
	 * and that we have a valid file reference.
	 */
	if(mfp == NULL || mfp->deadfile)
		goto file_dead;
	/*
	 * If the page is in a file for which we have LSN information, we have
	 * to ensure the appropriate log records are on disk.
	 */
	if(LOGGING_ON(env) && mfp->lsn_off != DB_LSN_OFF_NOTSET && !IS_CLIENT_PGRECOVER(env)) {
		memcpy(&lsn, bhp->buf+mfp->lsn_off, sizeof(DB_LSN));
		if(!IS_NOT_LOGGED_LSN(lsn) && (ret = __log_flush(env, &lsn)) != 0)
			goto err;
	}
#ifdef DIAGNOSTIC
	/*
	 * Verify write-ahead logging semantics.
	 *
	 * !!!
	 * Two special cases.  There is a single field on the meta-data page,
	 * the last-page-number-in-the-file field, for which we do not log
	 * changes.  If the page was originally created in a database that
	 * didn't have logging turned on, we can see a page marked dirty but
	 * for which no corresponding log record has been written.  However,
	 * the only way that a page can be created for which there isn't a
	 * previous log record and valid LSN is when the page was created
	 * without logging turned on, and so we check for that special-case
	 * LSN value.
	 *
	 * Second, when a client is reading database pages from a master
	 * during an internal backup, we may get pages modified after
	 * the current end-of-log.
	 */
	if(LOGGING_ON(env) && !IS_NOT_LOGGED_LSN(LSN(bhp->buf)) && !IS_CLIENT_PGRECOVER(env)) {
		/*
		 * There is a potential race here.  If we are in the midst of
		 * switching log files, it's possible we could test against the
		 * old file and the new offset in the log region's LSN.  If we
		 * fail the first test, acquire the log mutex and check again.
		 */
		DB_LOG * dblp = env->lg_handle;
		LOG * lp = dblp->reginfo.primary;
		if(!lp->db_log_inmemory && LOG_COMPARE(&lp->s_lsn, &LSN(bhp->buf)) <= 0) {
			MUTEX_LOCK(env, lp->mtx_flush);
			DB_ASSERT(env, F_ISSET(env->dbenv, DB_ENV_NOLOCKING) || LOG_COMPARE(&lp->s_lsn, &LSN(bhp->buf)) > 0);
			MUTEX_UNLOCK(env, lp->mtx_flush);
		}
	}
#endif

	/*
	 * Call any pgout function.  If we have the page exclusive then
	 * we are going to reuse it otherwise make a copy of the page so
	 * that others can continue looking at the page while we write it.
	 */
	buf = bhp->buf;
	if(mfp->ftype != 0) {
		if(F_ISSET(bhp, BH_EXCLUSIVE))
			F_SET(bhp, BH_TRASH);
		else {
			if((ret = __os_malloc(env, mfp->pagesize, &buf)) != 0)
				goto err;
			memcpy(buf, bhp->buf, mfp->pagesize);
		}
		if((ret = __memp_pg(dbmfp, bhp->pgno, buf, 0)) != 0)
			goto err;
	}
	PERFMON3(env, mpool, write, __memp_fn(dbmfp), bhp->pgno, bhp);
	/* Write the page. */
	if((ret = __os_io(env, DB_IO_WRITE, dbmfp->fhp, bhp->pgno, mfp->pagesize, 0, mfp->pagesize, (uint8 *)buf, &nw)) != 0) {
		__db_errx(env, DB_STR_A("3015", "%s: write failed for page %lu", "%s %lu"), __memp_fn(dbmfp), (ulong)bhp->pgno);
		goto err;
	}
	STAT_INC_VERB(env, mpool, page_out, mfp->stat.st_page_out, __memp_fn(dbmfp), bhp->pgno);
	if(bhp->pgno > mfp->last_flushed_pgno) {
		MUTEX_LOCK(env, mfp->mutex);
		if(bhp->pgno > mfp->last_flushed_pgno)
			mfp->last_flushed_pgno = bhp->pgno;
		MUTEX_UNLOCK(env, mfp->mutex);
	}
err:
file_dead:
	if(buf != bhp->buf)
		__os_free(env, buf);
	/*
	 * !!!
	 * Once we pass this point, dbmfp and mfp may be NULL, we may not have
	 * a valid file reference.
	 */
	/*
	 * Update the hash bucket statistics, reset the flags.  If we were
	 * successful, the page is no longer dirty.  Someone else may have
	 * also written the page so we need to latch the hash bucket here
	 * to get the accounting correct.  Since we have the buffer
	 * shared it cannot be marked dirty again till we release it.
	 * This is the only place we update the flags field only holding
	 * a shared latch.
	 */
	if(F_ISSET(bhp, BH_DIRTY|BH_TRASH)) {
		MUTEX_LOCK(env, hp->mtx_hash);
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		if(ret == 0 && F_ISSET(bhp, BH_DIRTY)) {
			F_CLR(bhp, BH_DIRTY|BH_DIRTY_CREATE);
			DB_ASSERT(env, atomic_read(&hp->hash_page_dirty) > 0);
			atomic_dec(env, &hp->hash_page_dirty);
		}
		/* put the page back if necessary. */
		if((ret != 0 || BH_REFCOUNT(bhp) > 1) && F_ISSET(bhp, BH_TRASH)) {
			ret = __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1);
			F_CLR(bhp, BH_TRASH);
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	return ret;
}
/*
 * __memp_pg --
 *	Call the pgin/pgout routine.
 */
int __memp_pg(DB_MPOOLFILE * dbmfp, db_pgno_t pgno, void * buf, int is_pgin)
{
	DBT dbt, * dbtp;
	DB_MPREG * mpreg;
	int ftype, ret;
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOLFILE * mfp = dbmfp->mfp;
	if((ftype = mfp->ftype) == DB_FTYPE_SET)
		mpreg = dbmp->pg_inout;
	else {
		MUTEX_LOCK(env, dbmp->mutex);
		LIST_FOREACH(mpreg, &dbmp->dbregq, q)
		if(ftype == mpreg->ftype)
			break;
		MUTEX_UNLOCK(env, dbmp->mutex);
	}
	if(mpreg == NULL)
		return 0;
	if(mfp->pgcookie_len == 0)
		dbtp = NULL;
	else {
		DB_SET_DBT(dbt, R_ADDR(dbmp->reginfo, mfp->pgcookie_off), mfp->pgcookie_len);
		dbtp = &dbt;
	}
	if(is_pgin) {
		if(mpreg->pgin && (ret = mpreg->pgin(env->dbenv, pgno, buf, dbtp)) != 0)
			goto err;
	}
	else
	if(mpreg->pgout && (ret = mpreg->pgout(env->dbenv, pgno, buf, dbtp)) != 0)
		goto err;
	return 0;
err:
	__db_errx(env, DB_STR_A("3016", "%s: %s failed for page %lu", "%s %s %lu"), __memp_fn(dbmfp),
		is_pgin ? DB_STR_P("pgin") : DB_STR_P("pgout"), (ulong)pgno);
	return ret;
}
/*
 * __memp_bhfree --
 *	Free a bucket header and its referenced data.
 */
int __memp_bhfree(DB_MPOOL * dbmp, REGINFO * infop, MPOOLFILE * mfp, DB_MPOOL_HASH * hp, BH * bhp, uint32 flags)
{
	ENV * env;
#ifdef DIAGNOSTIC
	DB_LSN vlsn;
#endif
	BH * prev_bhp;
	MPOOL * c_mp;
	int t_ret;
#ifdef DIAG_MVCC
	size_t pagesize;
#endif
	int ret = 0;
	/*
	 * Assumes the hash bucket is locked and the MPOOL is not.
	 */
	env = dbmp->env;
#ifdef DIAG_MVCC
	if(mfp)
		pagesize = mfp->pagesize;
#endif
	DB_ASSERT(env, LF_ISSET(BH_FREE_UNLOCKED) || (hp != NULL && MUTEX_IS_OWNED(env, hp->mtx_hash)));
	DB_ASSERT(env, BH_REFCOUNT(bhp) == 1 && !F_ISSET(bhp, BH_DIRTY|BH_FROZEN));
	DB_ASSERT(env, LF_ISSET(BH_FREE_UNLOCKED) || SH_CHAIN_SINGLETON(bhp, vc) || (SH_CHAIN_HASNEXT(bhp, vc) &&
		(SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off == bhp->td_off || bhp->td_off == INVALID_ROFF ||
		IS_MAX_LSN(*VISIBLE_LSN(env, bhp)) || BH_OBSOLETE(bhp, hp->old_reader, vlsn))));
	PERFMON3(env, mpool, evict, __memp_fns(dbmp, mfp), bhp->pgno, bhp);
	/*
	 * Delete the buffer header from the hash bucket queue or the
	 * version chain.
	 */
	if(hp == NULL)
		goto no_hp;
	prev_bhp = SH_CHAIN_PREV(bhp, vc, __bh);
	if(!SH_CHAIN_HASNEXT(bhp, vc)) {
		if(prev_bhp)
			SH_TAILQ_INSERT_AFTER(&hp->hash_bucket, bhp, prev_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
	}
	SH_CHAIN_REMOVE(bhp, vc, __bh);
	/*
	 * Remove the reference to this buffer from the transaction that
	 * created it, if any.  When the BH_FREE_UNLOCKED flag is set, we're
	 * discarding the environment, so the transaction region is already
	 * gone.
	 */
	if(bhp->td_off != INVALID_ROFF && !LF_ISSET(BH_FREE_UNLOCKED)) {
		ret = __txn_remove_buffer(env, BH_OWNER(env, bhp), hp->mtx_hash);
		bhp->td_off = INVALID_ROFF;
	}
	/*
	 * We're going to use the memory for something else -- it had better be
	 * accessible.
	 */
no_hp:
	if(mfp)
		MVCC_MPROTECT(bhp->buf, pagesize, PROT_READ|PROT_WRITE|PROT_EXEC);
	/*
	 * Discard the hash bucket's mutex, it's no longer needed, and
	 * we don't want to be holding it when acquiring other locks.
	 */
	if(!LF_ISSET(BH_FREE_UNLOCKED))
		MUTEX_UNLOCK(env, hp->mtx_hash);
	/*
	 * If we're only removing this header from the chain for reuse, we're done.
	 */
	if(LF_ISSET(BH_FREE_REUSE))
		return ret;
	/*
	 * If we're not reusing the buffer immediately, free the buffer for
	 * real.
	 */
	if(!LF_ISSET(BH_FREE_UNLOCKED))
		MUTEX_UNLOCK(env, bhp->mtx_buf);
	if(LF_ISSET(BH_FREE_FREEMEM)) {
		if((ret = __mutex_free(env, &bhp->mtx_buf)) != 0)
			return ret;
		MPOOL_REGION_LOCK(env, infop);
		MVCC_BHUNALIGN(bhp);
		__memp_free(infop, bhp);
		c_mp = (MPOOL *)infop->primary;
		c_mp->pages--;
		MPOOL_REGION_UNLOCK(env, infop);
	}
	if(!mfp)
		return ret;
	/*
	 * Decrement the reference count of the underlying MPOOLFILE.
	 * If this is its last reference, remove it.
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if(--mfp->block_cnt == 0 && mfp->mpf_cnt == 0) {
		if((t_ret = __memp_mf_discard(dbmp, mfp, 0)) != 0 && ret == 0)
			ret = t_ret;
	}
	else
		MUTEX_UNLOCK(env, mfp->mutex);
	return ret;
}
/*
 * __memp_fget_pp --
 *	DB_MPOOLFILE->get pre/post processing.
 */
int __memp_fget_pp(DB_MPOOLFILE * dbmfp, db_pgno_t * pgnoaddr, DB_TXN * txnp, uint32 flags, void * addrp)
{
	DB_THREAD_INFO * ip;
	int rep_blocked;
	int ret;
	ENV * env = dbmfp->env;
	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->get");
	/*
	 * Validate arguments.
	 *
	 * !!!
	 * Don't test for DB_MPOOL_CREATE and DB_MPOOL_NEW flags for readonly
	 * files here, and create non-existent pages in readonly files if the
	 * flags are set, later.  The reason is that the hash access method
	 * wants to get empty pages that don't really exist in readonly files.
	 * The only alternative is for hash to write the last "bucket" all the
	 * time, which we don't want to do because one of our big goals in life
	 * is to keep database files small.  It's sleazy as hell, but we catch
	 * any attempt to actually write the file in memp_fput().
	 */
#undef  OKFLAGS
#define OKFLAGS         (DB_MPOOL_CREATE|DB_MPOOL_DIRTY|DB_MPOOL_EDIT|DB_MPOOL_LAST|DB_MPOOL_NEW)
	if(flags != 0) {
		if((ret = __db_fchk(env, "memp_fget", flags, OKFLAGS)) != 0)
			return ret;
		switch(FLD_CLR(flags, DB_MPOOL_DIRTY|DB_MPOOL_EDIT)) {
		    case DB_MPOOL_CREATE:
		    case DB_MPOOL_LAST:
		    case DB_MPOOL_NEW:
		    case 0:
			break;
		    default:
			return __db_ferr(env, "memp_fget", 1);
		}
	}
	ENV_ENTER(env, ip);
	rep_blocked = 0;
	if(!txnp && IS_ENV_REPLICATED(env)) {
		if((ret = __op_rep_enter(env, 0, 1)) != 0)
			goto err;
		rep_blocked = 1;
	}
	ret = __memp_fget(dbmfp, pgnoaddr, ip, txnp, flags, addrp);
	/*
	 * We only decrement the count in op_rep_exit if the operation fails.
	 * Otherwise the count will be decremented when the page is no longer
	 * pinned in memp_fput.
	 */
	if(ret != 0 && rep_blocked)
		__op_rep_exit(env);
	/* Similarly if an app has a page pinned it is ACTIVE. */
err:
	if(ret != 0)
		ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_fget --
 *	Get a page from the file.
 */
int FASTCALL __memp_fget(DB_MPOOLFILE * dbmfp, db_pgno_t * pgnoaddr, DB_THREAD_INFO * ip, DB_TXN * txn, uint32 flags, void * addrp)
{
	enum { FIRST_FOUND, FIRST_MISS, SECOND_FOUND, SECOND_MISS } state;
	BH * alloc_bhp, * bhp, * oldest_bhp;
	ENV * env;
	DB_LSN * read_lsnp, vlsn;
	DB_MPOOL * dbmp;
	DB_MPOOL_HASH * hp;
	MPOOL * c_mp;
	MPOOLFILE * mfp;
	PIN_LIST * list, * lp;
	REGENV * renv;
	REGINFO * infop, * t_infop, * reginfo;
	TXN_DETAIL * td;
	roff_t list_off, mf_offset;
	uint32 bucket, pinmax, st_hsearch;
	int b_incr, b_lock, h_locked, dirty, extending;
	int makecopy, mvcc, need_free, ret;
#ifdef DIAGNOSTIC
	DB_LOCKTAB * lt;
	DB_LOCKER * locker;
#endif
	*(void **)addrp = NULL;
	COMPQUIET(c_mp, 0);
	COMPQUIET(infop, 0);
	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;
	mvcc = atomic_read(&mfp->multiversion) && (txn != NULL);
	mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	alloc_bhp = bhp = oldest_bhp = NULL;
	read_lsnp = NULL;
	td = NULL;
	hp = NULL;
	b_incr = b_lock = h_locked = extending = makecopy = ret = 0;
	if(LF_ISSET(DB_MPOOL_DIRTY)) {
		if(F_ISSET(dbmfp, MP_READONLY)) {
			__db_errx(env, DB_STR_A("3021", "%s: dirty flag set for readonly file page", "%s"), __memp_fn(dbmfp));
			return EINVAL;
		}
		if((ret = __db_fcchk(env, "DB_MPOOLFILE->get", flags, DB_MPOOL_DIRTY, DB_MPOOL_EDIT)) != 0)
			return ret;
	}
	dirty = LF_ISSET(DB_MPOOL_DIRTY|DB_MPOOL_EDIT|DB_MPOOL_FREE);
	LF_CLR(DB_MPOOL_DIRTY|DB_MPOOL_EDIT);
	/*
	 * If the transaction is being used to update a multiversion database
	 * for the first time, set the read LSN.  In addition, if this is an
	 * update, allocate a mutex.  If no transaction has been supplied, that
	 * will be caught later, when we know whether one is required.
	 */
	if(mvcc && txn && txn->td) {
		/* We're only interested in the ultimate parent transaction. */
		while(txn->parent)
			txn = txn->parent;
		td = (TXN_DETAIL *)txn->td;
		if(F_ISSET(txn, TXN_SNAPSHOT)) {
			read_lsnp = &td->read_lsn;
			if(IS_MAX_LSN(*read_lsnp) && (ret = __log_current_lsn_int(env, read_lsnp, NULL, NULL)) != 0)
				return ret;
		}
		if((dirty || LF_ISSET(DB_MPOOL_CREATE|DB_MPOOL_NEW)) && td->mvcc_mtx == MUTEX_INVALID && (ret = __mutex_alloc(env, MTX_TXN_MVCC, 0, &td->mvcc_mtx)) != 0)
			return ret;
	}
	switch(flags) {
	    case DB_MPOOL_LAST: // Get the last page number in the file.
			MUTEX_LOCK(env, mfp->mutex);
			*pgnoaddr = mfp->last_pgno;
			MUTEX_UNLOCK(env, mfp->mutex);
			break;
	    case DB_MPOOL_NEW: // If always creating a page, skip the first search of the hash bucket.
			goto newpg;
	    case DB_MPOOL_CREATE:
	    default:
			break;
	}
	/*
	 * If mmap'ing the file and the page is not past the end of the file,
	 * just return a pointer.  We can't use R_ADDR here: this is an offset
	 * into an mmap'd file, not a shared region, and doesn't change for
	 * private environments.
	 *
	 * The page may be past the end of the file, so check the page number
	 * argument against the original length of the file.  If we previously
	 * returned pages past the original end of the file, last_pgno will
	 * have been updated to match the "new" end of the file, and checking
	 * against it would return pointers past the end of the mmap'd region.
	 *
	 * If another process has opened the file for writing since we mmap'd
	 * it, we will start playing the game by their rules, i.e. everything
	 * goes through the cache.  All pages previously returned will be safe,
	 * as long as the correct locking protocol was observed.
	 *
	 * We don't discard the map because we don't know when all of the
	 * pages will have been discarded from the process' address space.
	 * It would be possible to do so by reference counting the open
	 * pages from the mmap, but it's unclear to me that it's worth it.
	 */
	if(dbmfp->addr && F_ISSET(mfp, MP_CAN_MMAP) && *pgnoaddr <= mfp->orig_last_pgno) {
		*(void **)addrp = (uint8 *)dbmfp->addr+(*pgnoaddr*mfp->pagesize);
		STAT_INC_VERB(env, mpool, map, mfp->stat.st_map, __memp_fn(dbmfp), *pgnoaddr);
		return 0;
	}
	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	MP_GET_BUCKET(env, mfp, *pgnoaddr, &infop, hp, bucket, ret);
	if(ret != 0)
		return ret;
	c_mp = (MPOOL *)infop->primary;
	if(0) {
		/* if we search again, get an exclusive lock. */
retry:
		MUTEX_LOCK(env, hp->mtx_hash);
	}
	/* Search the hash chain for the page. */
	st_hsearch = 0;
	h_locked = 1;
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		++st_hsearch;
		if(bhp->pgno != *pgnoaddr || bhp->mf_offset != mf_offset)
			continue;
		// Snapshot reads -- get the version visible at read_lsn
		if(read_lsnp) {
			while(bhp && !BH_OWNED_BY(env, bhp, txn) && !BH_VISIBLE(env, bhp, read_lsnp, vlsn))
				bhp = SH_CHAIN_PREV(bhp, vc, __bh);
			//
			// We can get a null bhp if we are looking for a
			// page that was created after the transaction was
			// started so its not visible  (i.e. page added to the BTREE in a subsequent txn).
			//
			if(!bhp) {
				ret = DB_PAGE_NOTFOUND;
				goto err;
			}
		}
		makecopy = mvcc && dirty && !BH_OWNED_BY(env, bhp, txn);
		/*
		 * Increment the reference count.  This signals that the
		 * buffer may not be discarded.  We must drop the hash
		 * mutex before we lock the buffer mutex.
		 */
		if(BH_REFCOUNT(bhp) == UINT16_MAX) {
			__db_errx(env, DB_STR_A("3022", "%s: page %lu: reference count overflow", "%s %lu"), __memp_fn(dbmfp), (ulong)bhp->pgno);
			ret = __env_panic(env, EINVAL);
			goto err;
		}
		atomic_inc(env, &bhp->ref);
		b_incr = 1;
		/*
		 * Lock the buffer. If the page is being read in or modified it
		 * will be exclusively locked and we will block.
		 */
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		if(dirty || extending || makecopy || F_ISSET(bhp, BH_FROZEN)) {
xlatch:
			if(LF_ISSET(DB_MPOOL_TRY)) {
				if((ret = MUTEX_TRYLOCK(env, bhp->mtx_buf)) != 0)
					goto err;
			}
			else
				MUTEX_LOCK(env, bhp->mtx_buf);
			F_SET(bhp, BH_EXCLUSIVE);
		}
		else if(LF_ISSET(DB_MPOOL_TRY)) {
			if((ret = MUTEX_TRY_READLOCK(env, bhp->mtx_buf)) != 0)
				goto err;
		}
		else
			MUTEX_READLOCK(env, bhp->mtx_buf);
#ifdef HAVE_SHARED_LATCHES
		/*
		 * If buffer is still in transit once we have a shared latch,
		 * upgrade to an exclusive latch.
		 */
		if(F_ISSET(bhp, BH_FREED|BH_TRASH) && !F_ISSET(bhp, BH_EXCLUSIVE)) {
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			goto xlatch;
		}
#else
		F_SET(bhp, BH_EXCLUSIVE);
#endif
		b_lock = 1;
		/*
		 * If the buffer was frozen before we waited for any I/O to
		 * complete and is still frozen, we will need to thaw it.
		 * Otherwise, it was thawed while we waited, and we need to search again.
		 */
		if(F_ISSET(bhp, BH_THAWED)) {
thawed:
			need_free = (atomic_dec(env, &bhp->ref) == 0);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			if(need_free) {
				MPOOL_REGION_LOCK(env, infop);
				SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, bhp, hq);
				MPOOL_REGION_UNLOCK(env, infop);
			}
			bhp = NULL;
			goto retry;
		}
		/*
		 * If the buffer we wanted was frozen or thawed while we
		 * waited, we need to start again.  That is indicated by
		 * a new buffer header in the version chain owned by the same
		 * transaction as the one we pinned.
		 *
		 * Also, if we're doing an unversioned read on a multiversion
		 * file, another thread may have dirtied this buffer while we
		 * swapped from the hash bucket lock to the buffer lock.
		 */
		if(SH_CHAIN_HASNEXT(bhp, vc) && (SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off == bhp->td_off || (!dirty && !read_lsnp))) {
			DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) != 0);
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			goto retry;
		}
		else if(dirty && SH_CHAIN_HASNEXT(bhp, vc)) {
			ret = DB_LOCK_DEADLOCK;
			goto err;
		}
		else if(F_ISSET(bhp, BH_FREED) && flags != DB_MPOOL_CREATE && flags != DB_MPOOL_NEW && flags != DB_MPOOL_FREE) {
			ret = DB_PAGE_NOTFOUND;
			goto err;
		}
		// Is it worthwhile to publish oh-so-frequent cache hits?
		STAT_INC_VERB(env, mpool, hit, mfp->stat.st_cache_hit, __memp_fn(dbmfp), *pgnoaddr);
		break;
	}
#ifdef HAVE_STATISTICS
	/*
	 * Update the hash bucket search statistics -- do now because our next
	 * search may be for a different bucket. Are these too frequent also?
	 */
	STAT_INC_VERB(env, mpool, hash_search, c_mp->stat.st_hash_searches, __memp_fn(dbmfp), *pgnoaddr);
	if(st_hsearch > c_mp->stat.st_hash_longest)
		STAT_SET_VERB(env, mpool, hash_longest, c_mp->stat.st_hash_longest, st_hsearch, __memp_fn(dbmfp), *pgnoaddr);
	STAT_ADJUST_VERB(env, mpool, hash_examined, c_mp->stat.st_hash_searches, st_hsearch, __memp_fn(dbmfp), *pgnoaddr);
#endif
	/*
	 * There are 4 possible paths to this location:
	 *
	 * FIRST_MISS:
	 *	Didn't find the page in the hash bucket on our first pass:
	 *	bhp == NULL, alloc_bhp == NULL
	 *
	 * FIRST_FOUND:
	 *	Found the page in the hash bucket on our first pass:
	 *	bhp != NULL, alloc_bhp == NULL
	 *
	 * SECOND_FOUND:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and found the page in the hash bucket on
	 *	our second pass:
	 *	bhp != NULL, alloc_bhp != NULL
	 *
	 * SECOND_MISS:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and didn't find the page in the hash bucket
	 *	on our second pass:
	 *	bhp == NULL, alloc_bhp != NULL
	 */
	state = (bhp == NULL) ? ((alloc_bhp == NULL) ? FIRST_MISS : SECOND_MISS) : ((alloc_bhp == NULL) ? FIRST_FOUND : SECOND_FOUND);
	switch(state) {
	    case FIRST_FOUND:
		/*
		 * If we are to free the buffer, then this had better be the
		 * only reference. If so, just free the buffer.  If not,
		 * complain and get out.
		 */
		if(flags == DB_MPOOL_FREE) {
freebuf:
			MUTEX_LOCK(env, hp->mtx_hash);
			h_locked = 1;
			if(F_ISSET(bhp, BH_DIRTY)) {
				F_CLR(bhp, BH_DIRTY|BH_DIRTY_CREATE);
				DB_ASSERT(env, atomic_read(&hp->hash_page_dirty) > 0);
				atomic_dec(env, &hp->hash_page_dirty);
			}
			//
			// If the buffer we found is already freed, we're done. If the ref count is not 1 then someone may be
			// peeking at the buffer.  We cannot free it until they determine that it is not what they want.  Clear the
			// buffer so that waiting threads get an empty page.
			//
			if(F_ISSET(bhp, BH_FREED))
				goto done;
			else if(BH_REFCOUNT(bhp) != 1 || !SH_CHAIN_SINGLETON(bhp, vc)) {
				//
				// Create an empty page in the chain for subsequent gets.  Otherwise, a thread that
				// re-creates this page while it is still in cache will see stale data.
				//
				F_SET(bhp, BH_FREED);
				F_CLR(bhp, BH_TRASH);
			}
			else if(F_ISSET(bhp, BH_FROZEN)) {
				//
				// Freeing a singleton frozen buffer: just free
				// it.  This call will release the hash bucket mutex.
				//
				ret = __memp_bh_thaw(dbmp, infop, hp, bhp, 0);
				bhp = NULL;
				b_incr = b_lock = h_locked = 0;
			}
			else {
				ret = __memp_bhfree(dbmp, infop, mfp, hp, bhp, BH_FREE_FREEMEM);
				bhp = NULL;
				b_incr = b_lock = h_locked = 0;
			}
			goto done;
		}
		else if(F_ISSET(bhp, BH_FREED|BH_TRASH)) {
revive:
			DB_ASSERT(env, F_ISSET(bhp, BH_TRASH) || oneof2(flags, DB_MPOOL_CREATE, DB_MPOOL_NEW));
			if(F_ISSET(bhp, BH_FREED))
				makecopy = makecopy || (mvcc && !BH_OWNED_BY(env, bhp, txn)) || F_ISSET(bhp, BH_FROZEN);
			if(flags == DB_MPOOL_CREATE) {
				MUTEX_LOCK(env, mfp->mutex);
				if(*pgnoaddr > mfp->last_pgno)
					mfp->last_pgno = *pgnoaddr;
				MUTEX_UNLOCK(env, mfp->mutex);
			}
		}
		if(mvcc) {
			//
			// With multiversion databases, we might need to allocate a new buffer into which we can copy the one
			// that we found.  In that case, check the last buffer in the chain to see whether we can reuse an obsolete buffer.
			//
			// To provide snapshot isolation, we need to make sure that we've seen a buffer older than the oldest
			// snapshot read LSN.
			//
reuse:
			if((makecopy || F_ISSET(bhp, BH_FROZEN)) && !h_locked) {
				MUTEX_LOCK(env, hp->mtx_hash);
				h_locked = 1;
			}
			if((makecopy || F_ISSET(bhp, BH_FROZEN)) && SH_CHAIN_HASPREV(bhp, vc)) {
				oldest_bhp = SH_CHAIN_PREVP(bhp, vc, __bh);
				while(SH_CHAIN_HASPREV(oldest_bhp, vc))
					oldest_bhp = SH_CHAIN_PREVP(oldest_bhp, vc, __bh);
				if(BH_REFCOUNT(oldest_bhp) == 0 && !BH_OBSOLETE(oldest_bhp, hp->old_reader, vlsn) && (ret = __txn_oldest_reader(env, &hp->old_reader)) != 0)
					goto err;
				if(BH_OBSOLETE(oldest_bhp, hp->old_reader, vlsn) && BH_REFCOUNT(oldest_bhp) == 0) {
					DB_ASSERT(env, !F_ISSET(oldest_bhp, BH_DIRTY));
					atomic_inc(env, &oldest_bhp->ref);
					if(F_ISSET(oldest_bhp, BH_FROZEN)) {
						//
						// This call will release the hash bucket mutex.
						//
						ret = __memp_bh_thaw(dbmp, infop, hp, oldest_bhp, 0);
						h_locked = 0;
						if(ret != 0)
							goto err;
						goto reuse;
					}
					if((ret = __memp_bhfree(dbmp, infop, mfp, hp, oldest_bhp, BH_FREE_REUSE)) != 0)
						goto err;
					alloc_bhp = oldest_bhp;
					h_locked = 0;
				}
				DB_ASSERT(env, alloc_bhp == NULL || !F_ISSET(alloc_bhp, BH_FROZEN));
			}
		}
		// We found the buffer or we're ready to copy -- we're done.
		if(!(makecopy || F_ISSET(bhp, BH_FROZEN)) || alloc_bhp)
			break;
	    // @fallthrough
	    case FIRST_MISS:
		/*
		 * We didn't find the buffer in our first check.  Figure out
		 * if the page exists, and allocate structures so we can add
		 * the page to the buffer pool.
		 */
		if(h_locked)
			MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		/*
		 * The buffer is not in the pool, so we don't need to free it.
		 */
		if(LF_ISSET(DB_MPOOL_FREE) && (bhp == NULL || F_ISSET(bhp, BH_FREED) || !makecopy))
			goto done;
		if(bhp)
			goto alloc;
newpg:          /*
		 * If DB_MPOOL_NEW is set, we have to allocate a page number.
		 * If neither DB_MPOOL_CREATE or DB_MPOOL_NEW is set, then
		 * it's an error to try and get a page past the end of file.
		 */
		DB_ASSERT(env, !h_locked);
		MUTEX_LOCK(env, mfp->mutex);
		switch(flags) {
		    case DB_MPOOL_NEW:
			extending = 1;
			if(mfp->maxpgno != 0 && mfp->last_pgno >= mfp->maxpgno) {
				__db_errx(env, DB_STR_A("3023", "%s: file limited to %lu pages", "%s %lu"), __memp_fn(dbmfp), (ulong)mfp->maxpgno);
				ret = ENOSPC;
			}
			else
				*pgnoaddr = mfp->last_pgno+1;
			break;
		    case DB_MPOOL_CREATE:
			if(mfp->maxpgno != 0 && *pgnoaddr > mfp->maxpgno) {
				__db_errx(env, DB_STR_A("3024", "%s: file limited to %lu pages", "%s %lu"), __memp_fn(dbmfp), (ulong)mfp->maxpgno);
				ret = ENOSPC;
			}
			else if(!extending)
				extending = *pgnoaddr > mfp->last_pgno;
			break;
		    default:
			ret = *pgnoaddr > mfp->last_pgno ? DB_PAGE_NOTFOUND : 0;
			break;
		}
		MUTEX_UNLOCK(env, mfp->mutex);
		if(ret != 0)
			goto err;
		/*
		 * !!!
		 * In the DB_MPOOL_NEW code path, hp, infop and c_mp have
		 * not yet been initialized.
		 */
		if(hp == NULL) {
			MP_GET_BUCKET(env, mfp, *pgnoaddr, &infop, hp, bucket, ret);
			if(ret != 0)
				goto err;
			MUTEX_UNLOCK(env, hp->mtx_hash);
			c_mp = (MPOOL *)infop->primary;
		}
alloc:
		// Allocate a new buffer header and data space.
		if(alloc_bhp == NULL && (ret = __memp_alloc(dbmp, infop, mfp, 0, NULL, &alloc_bhp)) != 0)
			goto err;
		/* Initialize enough so we can call __memp_bhfree. */
		alloc_bhp->flags = 0;
		atomic_init(&alloc_bhp->ref, 1);
#ifdef DIAGNOSTIC
		if((uintptr_t)alloc_bhp->buf&(sizeof(size_t)-1)) {
			__db_errx(env, DB_STR("3025", "DB_MPOOLFILE->get: buffer data is NOT size_t aligned"));
			ret = __env_panic(env, EINVAL);
			goto err;
		}
#endif
		/*
		 * If we're doing copy-on-write, we will already have the
		 * buffer header.  In that case, we don't need to search again.
		 */
		if(bhp)
			break;
		/*
		 * If we are extending the file, we'll need the mfp lock
		 * again.
		 */
		if(extending)
			MUTEX_LOCK(env, mfp->mutex);
		/*
		 * DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control.  (That guarantee is interesting
		 * for DB_MPOOL_NEW, unlike DB_MPOOL_CREATE, because the caller
		 * did not specify the page number, and so, may reasonably not
		 * have any way to lock the page outside of mpool.) Regardless,
		 * if we allocate the page, and some other thread of control
		 * requests the page by number, we will not detect that and the
		 * thread of control that allocated using DB_MPOOL_NEW may not
		 * have a chance to initialize the page.  (Note: we *could*
		 * detect this case if we set a flag in the buffer header which
		 * guaranteed that no gets of the page would succeed until the
		 * reference count went to 0, that is, until the creating page
		 * put the page.)  What we do guarantee is that if two threads
		 * of control are both doing DB_MPOOL_NEW calls, they won't
		 * collide, that is, they won't both get the same page.
		 *
		 * There's a possibility that another thread allocated the page
		 * we were planning to allocate while we were off doing buffer
		 * allocation.  We can do that by making sure the page number
		 * we were going to use is still available.  If it's not, then
		 * we check to see if the next available page number hashes to
		 * the same mpool region as the old one -- if it does, we can
		 * continue, otherwise, we have to start over.
		 */
		if(flags == DB_MPOOL_NEW && *pgnoaddr != mfp->last_pgno+1) {
			*pgnoaddr = mfp->last_pgno+1;
			MP_GET_REGION(dbmfp, *pgnoaddr, &t_infop, ret);
			if(ret != 0)
				goto err;
			if(t_infop != infop) {
				//
				// flags == DB_MPOOL_NEW, so extending is set and we're holding the mfp locked.
				//
				MUTEX_UNLOCK(env, mfp->mutex);
				hp = NULL;
				goto newpg;
			}
		}
		/*
		 * We released the mfp lock, so another thread might have
		 * extended the file.  Update the last_pgno and initialize
		 * the file, as necessary, if we extended the file.
		 */
		if(extending) {
			if(*pgnoaddr > mfp->last_pgno)
				mfp->last_pgno = *pgnoaddr;
			else
				extending = 0;
			MUTEX_UNLOCK(env, mfp->mutex);
			if(ret != 0)
				goto err;
		}
		goto retry;
	    case SECOND_FOUND:
		/*
		 * We allocated buffer space for the requested page, but then
		 * found the page in the buffer cache on our second check.
		 * That's OK -- we can use the page we found in the pool,
		 * unless DB_MPOOL_NEW is set.  If we're about to copy-on-write,
		 * this is exactly the situation we want.
		 *
		 * For multiversion files, we may have left some pages in cache
		 * beyond the end of a file after truncating.  In that case, we
		 * would get to here with extending set.  If so, we need to
		 * insert the new page in the version chain similar to when
		 * we copy on write.
		 */
		if(F_ISSET(bhp, BH_FREED) && (flags == DB_MPOOL_NEW || flags == DB_MPOOL_CREATE))
			goto revive;
		else if(flags == DB_MPOOL_FREE)
			goto freebuf;
		else if(makecopy || F_ISSET(bhp, BH_FROZEN))
			break;
		/*
		 * We can't use the page we found in the pool if DB_MPOOL_NEW
		 * was set.  (For details, see the above comment beginning
		 * "DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control".)  If DB_MPOOL_NEW is set, we
		 * release our pin on this particular buffer, and try to get
		 * another one.
		 */
		if(flags == DB_MPOOL_NEW) {
			DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) != 0);
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			if(F_ISSET(bhp, BH_EXCLUSIVE))
				F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			hp = NULL;
			goto newpg;
		}
		break;
	    case SECOND_MISS:
		/*
		 * We allocated buffer space for the requested page, and found
		 * the page still missing on our second pass through the buffer
		 * cache.  Instantiate the page.
		 */
		DB_ASSERT(env, alloc_bhp != NULL);
		bhp = alloc_bhp;
		alloc_bhp = NULL;

		/*
		 * Initialize all the BH and hash bucket fields so we can call
		 * __memp_bhfree if an error occurs.
		 *
		 * Append the buffer to the tail of the bucket list.
		 */
		bhp->priority = MPOOL_LRU_REDZONE;
		bhp->pgno = *pgnoaddr;
		bhp->mf_offset = mf_offset;
		bhp->bucket = bucket;
		bhp->region = (int)(infop-dbmp->reginfo);
		bhp->td_off = INVALID_ROFF;
		SH_CHAIN_INIT(bhp, vc);
		bhp->flags = 0;
		/*
		 * Reference the buffer and lock exclusive.  We either
		 * need to read the buffer or create it from scratch
		 * and don't want anyone looking at it till we do.
		 */
		MUTEX_LOCK(env, bhp->mtx_buf);
		b_lock = 1;
		F_SET(bhp, BH_EXCLUSIVE);
		b_incr = 1;
		/* We created a new page, it starts dirty. */
		if(extending) {
			atomic_inc(env, &hp->hash_page_dirty);
			F_SET(bhp, BH_DIRTY|BH_DIRTY_CREATE);
		}
		MUTEX_REQUIRED(env, hp->mtx_hash);
		SH_TAILQ_INSERT_HEAD(&hp->hash_bucket, bhp, hq, __bh);
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		/*
		 * If we created the page, zero it out.  If we didn't create
		 * the page, read from the backing file.
		 *
		 * !!!
		 * DB_MPOOL_NEW doesn't call the pgin function.
		 *
		 * If DB_MPOOL_CREATE is used, then the application's pgin
		 * function has to be able to handle pages of 0's -- if it
		 * uses DB_MPOOL_NEW, it can detect all of its page creates,
		 * and not bother.
		 *
		 * If we're running in diagnostic mode, smash any bytes on the
		 * page that are unknown quantities for the caller.
		 *
		 * Otherwise, read the page into memory, optionally creating it
		 * if DB_MPOOL_CREATE is set.
		 */
		if(extending) {
			MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ|PROT_WRITE);
			memzero(bhp->buf, (mfp->clear_len == DB_CLEARLEN_NOTSET) ? mfp->pagesize : mfp->clear_len);
#if defined(DIAGNOSTIC) || defined(UMRW)
			if(mfp->clear_len != DB_CLEARLEN_NOTSET)
				memset(bhp->buf+mfp->clear_len, CLEAR_BYTE, mfp->pagesize-mfp->clear_len);
#endif
			if(flags == DB_MPOOL_CREATE && mfp->ftype != 0 && (ret = __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1)) != 0)
				goto err;
			STAT_INC_VERB(env, mpool, page_create, mfp->stat.st_page_create, __memp_fn(dbmfp), *pgnoaddr);
		}
		else {
			F_SET(bhp, BH_TRASH);
			STAT_INC_VERB(env, mpool, miss, mfp->stat.st_cache_miss, __memp_fn(dbmfp), *pgnoaddr);
		}
		makecopy = mvcc && dirty && !extending;
		/* Increment buffer count referenced by MPOOLFILE. */
		MUTEX_LOCK(env, mfp->mutex);
		++mfp->block_cnt;
		MUTEX_UNLOCK(env, mfp->mutex);
	}
	DB_ASSERT(env, bhp != NULL && BH_REFCOUNT(bhp) != 0 && b_lock);
	DB_ASSERT(env, !F_ISSET(bhp, BH_FROZEN) || !F_ISSET(bhp, BH_FREED) || makecopy);
	/* We've got a buffer header we're re-instantiating. */
	if(F_ISSET(bhp, BH_FROZEN) && !F_ISSET(bhp, BH_FREED)) {
		if(alloc_bhp == NULL)
			goto reuse;
		/*
		 * To thaw the buffer, we must hold the hash bucket mutex,
		 * and the call to __memp_bh_thaw will release it.
		 */
		if(h_locked == 0)
			MUTEX_LOCK(env, hp->mtx_hash);
		h_locked = 1;
		/*
		 * If the empty buffer has been filled in the meantime, don't
		 * overwrite it.
		 */
		if(F_ISSET(bhp, BH_THAWED)) {
			MUTEX_UNLOCK(env, hp->mtx_hash);
			h_locked = 0;
			goto thawed;
		}
		ret = __memp_bh_thaw(dbmp, infop, hp, bhp, alloc_bhp);
		bhp = NULL;
		b_lock = h_locked = 0;
		if(ret != 0)
			goto err;
		bhp = alloc_bhp;
		alloc_bhp = NULL;
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		b_incr = b_lock = 1;
	}
	/*
	 * BH_TRASH --
	 * The buffer we found may need to be filled from the disk.
	 *
	 * It's possible for the read function to fail, which means we fail
	 * as well.  Discard the buffer on failure unless another thread
	 * is waiting on our I/O to complete.  It's OK to leave the buffer
	 * around, as the waiting thread will see the BH_TRASH flag set,
	 * and will also attempt to discard it.  If there's a waiter,
	 * we need to decrement our reference count.
	 */
	if(F_ISSET(bhp, BH_TRASH) && flags != DB_MPOOL_FREE && !F_ISSET(bhp, BH_FREED)) {
		MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ|PROT_WRITE);
		if((ret = __memp_pgread(dbmfp, bhp, LF_ISSET(DB_MPOOL_CREATE) ? 1 : 0)) != 0)
			goto err;
		DB_ASSERT(env, read_lsnp != NULL || !SH_CHAIN_HASNEXT(bhp, vc));
	}
	/* Copy-on-write. */
	if(makecopy) {
		/*
		 * If we read a page from disk that we want to modify, we now
		 * need to make copy, so we now need to allocate another buffer
		 * to hold the new copy.
		 */
		if(alloc_bhp == NULL)
			goto reuse;
		DB_ASSERT(env, bhp != NULL && alloc_bhp != bhp);
		DB_ASSERT(env, bhp->td_off == INVALID_ROFF || !IS_MAX_LSN(*VISIBLE_LSN(env, bhp)) || (F_ISSET(bhp, BH_FREED) && F_ISSET(bhp, BH_FROZEN)));
		DB_ASSERT(env, txn != NULL || (F_ISSET(bhp, BH_FROZEN) && F_ISSET(bhp, BH_FREED)));
		DB_ASSERT(env, (extending || flags == DB_MPOOL_FREE || F_ISSET(bhp, BH_FREED)) || !F_ISSET(bhp, BH_FROZEN|BH_TRASH));
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		if(BH_REFCOUNT(bhp) == 1)
			MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ);
		atomic_init(&alloc_bhp->ref, 1);
		MUTEX_LOCK(env, alloc_bhp->mtx_buf);
		alloc_bhp->priority = bhp->priority;
		alloc_bhp->pgno = bhp->pgno;
		alloc_bhp->bucket = bhp->bucket;
		alloc_bhp->region = bhp->region;
		alloc_bhp->mf_offset = bhp->mf_offset;
		alloc_bhp->td_off = INVALID_ROFF;
		if(txn == NULL) {
			DB_ASSERT(env, F_ISSET(bhp, BH_FROZEN) && F_ISSET(bhp, BH_FREED));
			if(bhp->td_off != INVALID_ROFF && (ret = __memp_bh_settxn(dbmp, mfp, alloc_bhp, BH_OWNER(env, bhp))) != 0)
				goto err;
		}
		else if((ret = __memp_bh_settxn(dbmp, mfp, alloc_bhp, td)) != 0)
			goto err;
		MVCC_MPROTECT(alloc_bhp->buf, mfp->pagesize, PROT_READ|PROT_WRITE);
		if(extending || F_ISSET(bhp, BH_FREED) || flags == DB_MPOOL_FREE) {
			memzero(alloc_bhp->buf, (mfp->clear_len == DB_CLEARLEN_NOTSET) ? mfp->pagesize : mfp->clear_len);
#if defined(DIAGNOSTIC) || defined(UMRW)
			if(mfp->clear_len != DB_CLEARLEN_NOTSET)
				memset(alloc_bhp->buf+mfp->clear_len, CLEAR_BYTE, mfp->pagesize-mfp->clear_len);
#endif
			if(mfp->ftype != 0 && (ret = __memp_pg(dbmfp, alloc_bhp->pgno, alloc_bhp->buf, 1)) != 0)
				goto err;
		}
		else
			memcpy(alloc_bhp->buf, bhp->buf, mfp->pagesize);
		MVCC_MPROTECT(alloc_bhp->buf, mfp->pagesize, 0);
		if(h_locked == 0)
			MUTEX_LOCK(env, hp->mtx_hash);
		MUTEX_REQUIRED(env, hp->mtx_hash);
		h_locked = 1;
		alloc_bhp->flags = BH_EXCLUSIVE|((flags == DB_MPOOL_FREE) ? BH_FREED : F_ISSET(bhp, BH_DIRTY|BH_DIRTY_CREATE));
		DB_ASSERT(env, flags != DB_MPOOL_FREE || !F_ISSET(bhp, BH_DIRTY));
		F_CLR(bhp, BH_DIRTY|BH_DIRTY_CREATE);
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		SH_CHAIN_INSERT_AFTER(bhp, alloc_bhp, vc, __bh);
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket, bhp, alloc_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) > 0);
		if(atomic_dec(env, &bhp->ref) == 0) {
			bhp->priority = c_mp->lru_priority;
			MVCC_MPROTECT(bhp->buf, mfp->pagesize, 0);
		}
		F_CLR(bhp, BH_EXCLUSIVE);
		MUTEX_UNLOCK(env, bhp->mtx_buf);

		bhp = alloc_bhp;
		DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
		b_incr = 1;
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		b_lock = 1;
		if(alloc_bhp != oldest_bhp) {
			MUTEX_LOCK(env, mfp->mutex);
			++mfp->block_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
		}
		alloc_bhp = NULL;
	}
	else if(mvcc && extending && (ret = __memp_bh_settxn(dbmp, mfp, bhp, td)) != 0)
		goto err;
	if(flags == DB_MPOOL_FREE) {
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		/* If we have created an empty buffer, it is not returned. */
		if(!F_ISSET(bhp, BH_FREED))
			goto freebuf;
		goto done;
	}
	/*
	 * Free the allocated memory, we no longer need it.
	 */
	if(alloc_bhp) {
		if((ret = __memp_bhfree(dbmp, infop, NULL, NULL, alloc_bhp, BH_FREE_FREEMEM|BH_FREE_UNLOCKED)) != 0)
			goto err;
		alloc_bhp = NULL;
	}
	if(dirty || extending || (F_ISSET(bhp, BH_FREED) && (flags == DB_MPOOL_CREATE || flags == DB_MPOOL_NEW))) {
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		if(F_ISSET(bhp, BH_FREED)) {
			DB_ASSERT(env, bhp->pgno <= mfp->last_pgno);
			memzero(bhp->buf, (mfp->clear_len == DB_CLEARLEN_NOTSET) ? mfp->pagesize : mfp->clear_len);
			F_CLR(bhp, BH_FREED);
			if(mfp->ftype != 0 && (ret = __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1)) != 0)
				goto err;
		}
		if(!F_ISSET(bhp, BH_DIRTY)) {
#ifdef DIAGNOSTIC
			MUTEX_LOCK(env, hp->mtx_hash);
#endif
			DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
			atomic_inc(env, &hp->hash_page_dirty);
			F_SET(bhp, BH_DIRTY);
#ifdef DIAGNOSTIC
			MUTEX_UNLOCK(env, hp->mtx_hash);
#endif
		}
	}
	else if(F_ISSET(bhp, BH_EXCLUSIVE)) {
		F_CLR(bhp, BH_EXCLUSIVE);
#ifdef HAVE_SHARED_LATCHES
		MUTEX_UNLOCK(env, bhp->mtx_buf);
		MUTEX_READLOCK(env, bhp->mtx_buf);
		/*
		 * If another thread has dirtied the page while we
		 * switched locks, we have to go through it all again.
		 */
		if(SH_CHAIN_HASNEXT(bhp, vc) && read_lsnp == NULL) {
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			goto retry;
		}
#endif
	}
	MVCC_MPROTECT(bhp->buf, mfp->pagesize, PROT_READ|(dirty || extending || F_ISSET(bhp, BH_DIRTY) ? PROT_WRITE : 0));
#ifdef DIAGNOSTIC
	MUTEX_LOCK(env, hp->mtx_hash);
	{
		BH * next_bhp = SH_CHAIN_NEXT(bhp, vc, __bh);
		DB_ASSERT(env, !atomic_read(&mfp->multiversion) || read_lsnp != NULL || next_bhp == NULL);
		DB_ASSERT(env, !mvcc || read_lsnp == NULL || bhp->td_off == INVALID_ROFF || BH_OWNED_BY(env, bhp, txn) ||
			(BH_VISIBLE(env, bhp, read_lsnp, vlsn) && (next_bhp == NULL || F_ISSET(next_bhp, BH_FROZEN) ||
	      (next_bhp->td_off != INVALID_ROFF && (BH_OWNER(env, next_bhp)->status != TXN_COMMITTED ||
	      IS_ZERO_LSN(BH_OWNER(env, next_bhp)->last_lsn) || !BH_VISIBLE(env, next_bhp, read_lsnp, vlsn))))));
	}
	MUTEX_UNLOCK(env, hp->mtx_hash);
#endif
	/*
	 * Record this pin for this thread.  Holding the page pinned
	 * without recording the pin is ok since we do not recover from
	 * a death from within the library itself.
	 */
	if(ip) {
		reginfo = env->reginfo;
		if(ip->dbth_pincount == ip->dbth_pinmax) {
			pinmax = ip->dbth_pinmax;
			renv = (REGENV *)reginfo->primary;
			MUTEX_LOCK(env, renv->mtx_regenv);
			if((ret = __env_alloc(reginfo, 2*pinmax*sizeof(PIN_LIST), &list)) != 0) {
				MUTEX_UNLOCK(env, renv->mtx_regenv);
				goto err;
			}
			memcpy(list, R_ADDR(reginfo, ip->dbth_pinlist), pinmax*sizeof(PIN_LIST));
			memzero(&list[pinmax], pinmax*sizeof(PIN_LIST));
			list_off = R_OFFSET(reginfo, list);
			list = (PIN_LIST *)R_ADDR(reginfo, ip->dbth_pinlist);
			ip->dbth_pinmax = 2*pinmax;
			ip->dbth_pinlist = list_off;
			if(list != ip->dbth_pinarray)
				__env_alloc_free(reginfo, list);
			MUTEX_UNLOCK(env, renv->mtx_regenv);
		}
		list = (PIN_LIST *)R_ADDR(reginfo, ip->dbth_pinlist);
		for(lp = list; lp < &list[ip->dbth_pinmax]; lp++)
			if(lp->b_ref == INVALID_ROFF)
				break;
		ip->dbth_pincount++;
		lp->b_ref = R_OFFSET(infop, bhp);
		lp->region = (int)(infop-dbmp->reginfo);
#ifdef DIAGNOSTIC
		if(dirty && ip->dbth_locker != INVALID_ROFF && ip->dbth_check_off == 0) {
			lt = env->lk_handle;
			locker = (DB_LOCKER *)(R_ADDR(&lt->reginfo, ip->dbth_locker));
			DB_ASSERT(env, __db_has_pagelock(env, locker, dbmfp, (PAGE *)bhp->buf, DB_LOCK_WRITE) == 0);
		}
#endif

	}
	/*
	 * During recovery we can read past the end of the file.  Also
	 * last_pgno is not versioned, so if this is an older version
	 * that is ok as well.
	 */
	DB_ASSERT(env, IS_RECOVERING(env) || bhp->pgno <= mfp->last_pgno || !SH_CHAIN_SINGLETON(bhp, vc));

#ifdef DIAGNOSTIC
	/* Update the file's pinned reference count. */
	MPOOL_SYSTEM_LOCK(env);
	++dbmfp->pinref;
	MPOOL_SYSTEM_UNLOCK(env);
	/*
	 * We want to switch threads as often as possible, and at awkward
	 * times.  Yield every time we get a new page to ensure contention.
	 */
	if(F_ISSET(env->dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif
	DB_ASSERT(env, alloc_bhp == NULL);
	DB_ASSERT(env, !(dirty || extending) || atomic_read(&hp->hash_page_dirty) > 0);
	DB_ASSERT(env, BH_REFCOUNT(bhp) > 0 && !F_ISSET(bhp, BH_FREED|BH_FROZEN|BH_TRASH));
	*(void **)addrp = bhp->buf;
	return 0;
done:
err:
	/*
	 * We should only get to here with ret == 0 if freeing a buffer.
	 * In that case, check that it has in fact been freed.
	 */
	DB_ASSERT(env, ret != 0 || flags != DB_MPOOL_FREE || bhp == NULL || (F_ISSET(bhp, BH_FREED) && !SH_CHAIN_HASNEXT(bhp, vc)));
	if(bhp) {
		if(b_incr)
			atomic_dec(env, &bhp->ref);
		if(b_lock) {
			F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
		}
	}
	if(h_locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);
	// If alloc_bhp is set, free the memory.
	if(alloc_bhp)
		__memp_bhfree(dbmp, infop, NULL, NULL, alloc_bhp, BH_FREE_FREEMEM|BH_FREE_UNLOCKED);
	return ret;
}
/*
 * __memp_fcreate_pp --
 *	ENV->memp_fcreate pre/post processing.
 */
int __memp_fcreate_pp(DB_ENV * dbenv, DB_MPOOLFILE ** retp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	// Validate arguments
	if((ret = __db_fchk(env, "DB_ENV->memp_fcreate", flags, 0)) != 0)
		return ret;
	if(REP_ON(env)) {
		__db_errx(env, DB_STR("3029", "DB_ENV->memp_fcreate: method not permitted when replication is configured"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);
	ret = __memp_fcreate(env, retp);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_fcreate --
 *	ENV->memp_fcreate.
 */
int __memp_fcreate(ENV * env, DB_MPOOLFILE ** retp)
{
	DB_MPOOLFILE * dbmfp;
	int ret;
	/* Allocate and initialize the per-process structure. */
	if((ret = __os_calloc(env, 1, sizeof(DB_MPOOLFILE), &dbmfp)) != 0)
		return ret;
	dbmfp->ref = 1;
	dbmfp->lsn_offset = DB_LSN_OFF_NOTSET;
	dbmfp->env = env;
	dbmfp->mfp = INVALID_ROFF;

	dbmfp->close = __memp_fclose_pp;
	dbmfp->get = __memp_fget_pp;
	dbmfp->get_clear_len = __memp_get_clear_len;
	dbmfp->get_fileid = __memp_get_fileid;
	dbmfp->get_flags = __memp_get_flags;
	dbmfp->get_ftype = __memp_get_ftype;
	dbmfp->get_last_pgno = __memp_get_last_pgno_pp;
	dbmfp->get_lsn_offset = __memp_get_lsn_offset;
	dbmfp->get_maxsize = __memp_get_maxsize;
	dbmfp->get_pgcookie = __memp_get_pgcookie;
	dbmfp->get_priority = __memp_get_priority;
	dbmfp->open = __memp_fopen_pp;
	dbmfp->put = __memp_fput_pp;
	dbmfp->set_clear_len = __memp_set_clear_len;
	dbmfp->set_fileid = __memp_set_fileid;
	dbmfp->set_flags = __memp_set_flags;
	dbmfp->set_ftype = __memp_set_ftype;
	dbmfp->set_lsn_offset = __memp_set_lsn_offset;
	dbmfp->set_maxsize = __memp_set_maxsize;
	dbmfp->set_pgcookie = __memp_set_pgcookie;
	dbmfp->set_priority = __memp_set_priority;
	dbmfp->sync = __memp_fsync_pp;
	*retp = dbmfp;
	return 0;
}
/*
 * __memp_get_clear_len --
 *	Get the clear length.
 */
static int __memp_get_clear_len(const DB_MPOOLFILE * dbmfp, uint32 * clear_lenp)
{
	*clear_lenp = dbmfp->clear_len;
	return 0;
}
/*
 * __memp_set_clear_len --
 *	DB_MPOOLFILE->set_clear_len.
 */
int __memp_set_clear_len(DB_MPOOLFILE * dbmfp, uint32 clear_len)
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_clear_len");
	dbmfp->clear_len = clear_len;
	return 0;
}
/*
 * __memp_get_fileid --
 *	DB_MPOOLFILE->get_fileid.
 */
int __memp_get_fileid(const DB_MPOOLFILE * dbmfp, uint8 * fileid)
{
	if(!F_ISSET(dbmfp, MP_FILEID_SET)) {
		__db_errx(dbmfp->env, DB_STR("3030", "get_fileid: file ID not set"));
		return EINVAL;
	}
	else {
		memcpy(fileid, dbmfp->fileid, DB_FILE_ID_LEN);
		return 0;
	}
}
/*
 * __memp_set_fileid --
 *	DB_MPOOLFILE->set_fileid.
 */
int __memp_set_fileid(DB_MPOOLFILE * dbmfp, uint8 * fileid)
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_fileid");
	memcpy(dbmfp->fileid, fileid, DB_FILE_ID_LEN);
	F_SET(dbmfp, MP_FILEID_SET);
	return 0;
}
/*
 * __memp_get_flags --
 *	Get the DB_MPOOLFILE flags;
 */
int __memp_get_flags(DB_MPOOLFILE * dbmfp, uint32 * flagsp)
{
	MPOOLFILE * mfp = dbmfp->mfp;
	*flagsp = 0;
	if(!mfp)
		*flagsp = FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE|DB_MPOOL_UNLINK);
	else {
		if(mfp->no_backing_file)
			FLD_SET(*flagsp, DB_MPOOL_NOFILE);
		if(mfp->unlink_on_close)
			FLD_SET(*flagsp, DB_MPOOL_UNLINK);
	}
	return 0;
}
/*
 * __memp_set_flags --
 *	Set the DB_MPOOLFILE flags;
 */
int __memp_set_flags(DB_MPOOLFILE * dbmfp, uint32 flags, int onoff)
{
	int ret;
	ENV * env = dbmfp->env;
	MPOOLFILE * mfp = dbmfp->mfp;
	switch(flags) {
	    case DB_MPOOL_NOFILE:
			if(!mfp) {
				SETFLAG(dbmfp->config_flags, DB_MPOOL_NOFILE, onoff);
			}
			else
				mfp->no_backing_file = onoff;
			break;
	    case DB_MPOOL_UNLINK:
			if(!mfp) {
				SETFLAG(dbmfp->config_flags, DB_MPOOL_UNLINK, onoff);
			}
			else
				mfp->unlink_on_close = onoff;
			break;
	    default:
			if((ret = __db_fchk(env, "DB_MPOOLFILE->set_flags", flags, DB_MPOOL_NOFILE|DB_MPOOL_UNLINK)) != 0)
				return ret;
			break;
	}
	return 0;
}
/*
 * __memp_get_ftype --
 *	Get the file type (as registered).
 */
int __memp_get_ftype(const DB_MPOOLFILE * dbmfp, int * ftypep)
{
	*ftypep = dbmfp->ftype;
	return 0;
}
/*
 * __memp_set_ftype --
 *	DB_MPOOLFILE->set_ftype.
 */
int __memp_set_ftype(DB_MPOOLFILE * dbmfp, int ftype)
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_ftype");
	dbmfp->ftype = ftype;
	return 0;
}
/*
 * __memp_get_lsn_offset --
 *	Get the page's LSN offset.
 */
static int __memp_get_lsn_offset(const DB_MPOOLFILE * dbmfp, int32 * lsn_offsetp)
{
	*lsn_offsetp = dbmfp->lsn_offset;
	return 0;
}
/*
 * __memp_set_lsn_offset --
 *	Set the page's LSN offset.
 */
int __memp_set_lsn_offset(DB_MPOOLFILE * dbmfp, int32 lsn_offset)
{
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_lsn_offset");
	dbmfp->lsn_offset = lsn_offset;
	return 0;
}
/*
 * __memp_get_maxsize --
 *	Get the file's maximum size.
 */
static int __memp_get_maxsize(DB_MPOOLFILE * dbmfp, uint32 * gbytesp, uint32 * bytesp)
{
	DB_THREAD_INFO * ip;
	ENV * env;
	MPOOLFILE * mfp;
	if((mfp = dbmfp->mfp) == NULL) {
		*gbytesp = dbmfp->gbytes;
		*bytesp = dbmfp->bytes;
	}
	else {
		env = dbmfp->env;
		ENV_ENTER(env, ip);
		MUTEX_LOCK(env, mfp->mutex);
		*gbytesp = (uint32)(mfp->maxpgno/(GIGABYTE/mfp->pagesize));
		*bytesp = (uint32)((mfp->maxpgno%(GIGABYTE/mfp->pagesize))*mfp->pagesize);
		MUTEX_UNLOCK(env, mfp->mutex);
		ENV_LEAVE(env, ip);
	}
	return 0;
}
/*
 * __memp_set_maxsize --
 *	Set the file's maximum size.
 */
static int __memp_set_maxsize(DB_MPOOLFILE * dbmfp, uint32 gbytes, uint32 bytes)
{
	DB_THREAD_INFO * ip;
	ENV * env;
	MPOOLFILE * mfp;
	if((mfp = dbmfp->mfp) == NULL) {
		dbmfp->gbytes = gbytes;
		dbmfp->bytes = bytes;
	}
	else {
		env = dbmfp->env;
		ENV_ENTER(env, ip);
		MUTEX_LOCK(env, mfp->mutex);
		mfp->maxpgno = (db_pgno_t)(gbytes*(GIGABYTE/mfp->pagesize));
		mfp->maxpgno += (db_pgno_t)((bytes+mfp->pagesize-1)/mfp->pagesize);
		MUTEX_UNLOCK(env, mfp->mutex);
		ENV_LEAVE(env, ip);
	}
	return 0;
}
/*
 * __memp_get_pgcookie --
 *	Get the pgin/pgout cookie.
 */
int __memp_get_pgcookie(const DB_MPOOLFILE * dbmfp, DBT * pgcookie)
{
	if(dbmfp->pgcookie == NULL) {
		pgcookie->size = 0;
		pgcookie->data = const_cast<char *>(""); // @badcast
	}
	else
		memcpy(pgcookie, dbmfp->pgcookie, sizeof(DBT));
	return 0;
}
/*
 * __memp_set_pgcookie --
 *	Set the pgin/pgout cookie.
 */
int __memp_set_pgcookie(DB_MPOOLFILE * dbmfp, DBT * pgcookie)
{
	DBT * cookie;
	MPF_ILLEGAL_AFTER_OPEN(dbmfp, "DB_MPOOLFILE->set_pgcookie");
	int ret = 0;
	ENV * env = dbmfp->env;
	if((ret = __os_calloc(env, 1, sizeof(*cookie), &cookie)) != 0)
		return ret;
	else if((ret = __os_malloc(env, pgcookie->size, &cookie->data)) != 0) {
		__os_free(env, cookie);
		return ret;
	}
	else {
		memcpy(cookie->data, pgcookie->data, pgcookie->size);
		cookie->size = pgcookie->size;
		dbmfp->pgcookie = cookie;
		return 0;
	}
}
/*
 * __memp_get_priority --
 *	Set the cache priority for pages from this file.
 */
int __memp_get_priority(const DB_MPOOLFILE * dbmfp, DB_CACHE_PRIORITY * priorityp)
{
	switch(dbmfp->priority) {
	    case MPOOL_PRI_VERY_LOW: *priorityp = DB_PRIORITY_VERY_LOW; break;
	    case MPOOL_PRI_LOW: *priorityp = DB_PRIORITY_LOW; break;
	    case MPOOL_PRI_DEFAULT: *priorityp = DB_PRIORITY_DEFAULT; break;
	    case MPOOL_PRI_HIGH: *priorityp = DB_PRIORITY_HIGH; break;
	    case MPOOL_PRI_VERY_HIGH: *priorityp = DB_PRIORITY_VERY_HIGH; break;
	    default:
			__db_errx(dbmfp->env, DB_STR_A("3031", "DB_MPOOLFILE->get_priority: unknown priority value: %d", "%d"), dbmfp->priority);
			return EINVAL;
	}
	return 0;
}
/*
 * __memp_set_priority --
 *	Set the cache priority for pages from this file.
 */
static int __memp_set_priority(DB_MPOOLFILE * dbmfp, DB_CACHE_PRIORITY priority)
{
	switch(priority) {
	    case DB_PRIORITY_VERY_LOW: dbmfp->priority = MPOOL_PRI_VERY_LOW; break;
	    case DB_PRIORITY_LOW: dbmfp->priority = MPOOL_PRI_LOW; break;
	    case DB_PRIORITY_DEFAULT: dbmfp->priority = MPOOL_PRI_DEFAULT; break;
	    case DB_PRIORITY_HIGH: dbmfp->priority = MPOOL_PRI_HIGH; break;
	    case DB_PRIORITY_VERY_HIGH: dbmfp->priority = MPOOL_PRI_VERY_HIGH; break;
	    default:
			__db_errx(dbmfp->env, DB_STR_A("3032", "DB_MPOOLFILE->set_priority: unknown priority value: %d", "%d"), priority);
			return EINVAL;
	}
	// Update the underlying file if we've already opened it.
	if(dbmfp->mfp)
		dbmfp->mfp->priority = dbmfp->priority;
	return 0;
}
/*
 * __memp_get_last_pgno --
 *	Return the page number of the last page in the file.
 *
 * !!!
 * The method is undocumented, but the handle is exported, users occasionally
 * ask for it.
 */
int __memp_get_last_pgno(DB_MPOOLFILE * dbmfp, db_pgno_t * pgnoaddr)
{
	ENV * env = dbmfp->env;
	MPOOLFILE * mfp = dbmfp->mfp;
	MUTEX_LOCK(env, mfp->mutex);
	*pgnoaddr = mfp->last_pgno;
	MUTEX_UNLOCK(env, mfp->mutex);
	return 0;
}
/*
 * __memp_get_last_pgno_pp --
 *	pre/post processing for __memp_get_last_pgno.
 */
static int __memp_get_last_pgno_pp(DB_MPOOLFILE * dbmfp, db_pgno_t * pgnoaddr)
{
	DB_THREAD_INFO * ip;
	int ret = 0;
	ENV_ENTER(dbmfp->env, ip);
	ret = __memp_get_last_pgno(dbmfp, pgnoaddr);
	ENV_LEAVE(dbmfp->env, ip);
	return ret;
}
/*
 * __memp_fn --
 *	On errors we print whatever is available as the file name.
 */
char * FASTCALL __memp_fn(DB_MPOOLFILE * dbmfp)
{
	return __memp_fns(dbmfp->env->mp_handle, dbmfp->mfp);
}
/*
 * __memp_fns --
 *	On errors we print whatever is available as the file name.
 */
char * __memp_fns(DB_MPOOL * dbmp, MPOOLFILE * mfp)
{
	return (mfp == NULL || mfp->path_off == 0) ? (char *)"unknown" : (char *)R_ADDR(dbmp->reginfo, mfp->path_off);
}
/*
 * __memp_fopen_pp --
 *	DB_MPOOLFILE->open pre/post processing.
 */
int __memp_fopen_pp(DB_MPOOLFILE * dbmfp, const char * path, uint32 flags, int mode, size_t pagesize)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbmfp->env;
	// Validate arguments
	if((ret = __db_fchk(env, "DB_MPOOLFILE->open", flags, DB_CREATE|DB_DIRECT|DB_EXTENT|DB_MULTIVERSION|DB_NOMMAP|DB_ODDFILESIZE|DB_RDONLY|DB_TRUNCATE)) != 0)
		return ret;
	//
	// Require a non-zero, power-of-two pagesize, smaller than the clear length.
	//
	// @sobolev if(pagesize == 0 || !POWER_OF_TWO(pagesize)) {
	if(pagesize == 0 || !IsPowerOfTwo(pagesize)) {
		__db_errx(env, DB_STR("3033", "DB_MPOOLFILE->open: page sizes must be a power-of-2"));
		return EINVAL;
	}
	if(dbmfp->clear_len > pagesize) {
		__db_errx(env, DB_STR("3034", "DB_MPOOLFILE->open: clear length larger than page size"));
		return EINVAL;
	}
	/* Read-only checks, and local flag. */
	if(LF_ISSET(DB_RDONLY) && path == NULL) {
		__db_errx(env, DB_STR("3035", "DB_MPOOLFILE->open: temporary files can't be readonly"));
		return EINVAL;
	}
	if(LF_ISSET(DB_MULTIVERSION) && !TXN_ON(env)) {
		__db_errx(env, DB_STR("3036", "DB_MPOOLFILE->open: DB_MULTIVERSION requires transactions"));
		return EINVAL;
	}
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_fopen(dbmfp, NULL, path, NULL, flags, mode, pagesize)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_fopen --
 *	DB_MPOOLFILE->open.
 */
int __memp_fopen(DB_MPOOLFILE * dbmfp, MPOOLFILE * mfp, const char * path, const char ** dirp, uint32 flags, int mode, size_t pgsize)
{
	DB_ENV * dbenv;
	DB_MPOOL * dbmp;
	DB_MPOOLFILE * tmp_dbmfp;
	DB_MPOOL_HASH * hp;
	ENV * env;
	MPOOL * mp;
	MPOOLFILE * alloc_mfp;
	size_t maxmap;
	db_pgno_t last_pgno;
	uint32 bucket, mbytes, bytes, oflags, pagesize;
	int refinc, ret;
	char * rpath;
	/* If this handle is already open, return. */
	if(F_ISSET(dbmfp, MP_OPEN_CALLED))
		return 0;
	env = dbmfp->env;
	dbmp = env->mp_handle;
	dbenv = env->dbenv;
	mp = (MPOOL *)dbmp->reginfo[0].primary;
	alloc_mfp = NULL;
	mbytes = bytes = 0;
	refinc = ret = 0;
	rpath = NULL;

	/*
	 * We're keeping the page size as a size_t in the public API, but
	 * it's a uint32 everywhere internally.
	 */
	pagesize = (uint32)pgsize;
	/*
	 * We're called internally with a specified mfp, in which case the
	 * path is NULL, but we'll get the path from the underlying region
	 * information.  Otherwise, if the path is NULL, it's a temporary
	 * file -- we know we can't join any existing files, and we'll delay
	 * the open until we actually need to write the file. All temporary
	 * files will go into the first hash bucket.
	 */
	DB_ASSERT(env, mfp == NULL || path == NULL);
	bucket = 0;
	hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	if(!mfp) {
		if(!path)
			goto alloc;
		/*
		 * Hash to the proper file table entry and walk it.
		 *
		 * The fileID is a filesystem unique number (e.g., a
		 * UNIX dev/inode pair) plus a timestamp.  If files are
		 * removed and created in less than a second, the fileID
		 * can be repeated.  The problem with repetition happens
		 * when the file that previously had the fileID value still
		 * has pages in the pool, since we don't want to use them
		 * to satisfy requests for the new file. Because the
		 * DB_TRUNCATE flag reuses the dev/inode pair, repeated
		 * opens with that flag set guarantees matching fileIDs
		 * when the machine can open a file and then re-open
		 * with truncate within a second.  For this reason, we
		 * pass that flag down, and, if we find a matching entry,
		 * we ensure that it's never found again, and we create
		 * a new entry for the current request.
		 */
		if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
			bucket = FNBUCKET(path, sstrlen(path));
		else
			bucket = FNBUCKET(dbmfp->fileid, DB_FILE_ID_LEN);
		hp += bucket;
		/*
		 * If we are passed a FILEID find the MPOOLFILE and inc
		 * its ref count.  That way it cannot go away while we
		 * open it.
		 */
		if(F_ISSET(dbmfp, MP_FILEID_SET)) {
			MUTEX_LOCK(env, hp->mtx_hash);
			ret = __memp_mpf_find(env, dbmfp, hp, path, flags, &mfp);
			MUTEX_UNLOCK(env, hp->mtx_hash);
			if(ret != 0)
				goto err;
			if(mfp)
				refinc = 1;
		}
	}
	else {
		/*
		 * Deadfile can only be set if mpf_cnt goes to zero (or if we
		 * failed creating the file DB_AM_DISCARD).  Increment the ref
		 * count so the file cannot become dead and be unlinked.
		 */
		MUTEX_LOCK(env, mfp->mutex);
		if(!mfp->deadfile) {
			++mfp->mpf_cnt;
			refinc = 1;
		}
		MUTEX_UNLOCK(env, mfp->mutex);
		/*
		 * Test one last time to see if the file is dead -- it may have
		 * been removed.  This happens when a checkpoint trying to open
		 * the file to flush a buffer races with the Db::remove method.
		 * The error will be ignored, so don't output an error message.
		 */
		if(mfp->deadfile)
			return EINVAL;
	}
	/*
	 * Share the underlying file descriptor if that's possible.
	 */
	if(mfp && !FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE)) {
		MUTEX_LOCK(env, dbmp->mutex);
		TAILQ_FOREACH(tmp_dbmfp, &dbmp->dbmfq, q)
		if(mfp == tmp_dbmfp->mfp && (F_ISSET(dbmfp, MP_READONLY) || !F_ISSET(tmp_dbmfp, MP_READONLY))) {
			++tmp_dbmfp->fhp->ref;
			dbmfp->fhp = tmp_dbmfp->fhp;
			dbmfp->addr = tmp_dbmfp->addr;
			break;
		}
		MUTEX_UNLOCK(env, dbmp->mutex);
		if(dbmfp->fhp)
			goto have_mfp;
	}
	/*
	 * If there's no backing file, we can join existing files in the cache,
	 * but there's nothing to read from disk.
	 */
	if(!FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE)) {
		/* Convert MP open flags to DB OS-layer open flags. */
		oflags = 0;
		if(LF_ISSET(DB_CREATE))
			oflags |= DB_OSO_CREATE;
		if(LF_ISSET(DB_DIRECT))
			oflags |= DB_OSO_DIRECT;
		if(LF_ISSET(DB_RDONLY)) {
			F_SET(dbmfp, MP_READONLY);
			oflags |= DB_OSO_RDONLY;
		}
		/*
		 * XXX
		 * A grievous layering violation, the DB_DSYNC_DB flag
		 * was left in the ENV structure and not driven through
		 * the cache API.  This needs to be fixed when the general
		 * API configuration is fixed.
		 */
		if(F_ISSET(env->dbenv, DB_ENV_DSYNC_DB))
			oflags |= DB_OSO_DSYNC;
		/*
		 * Get the real name for this file and open it.
		 *
		 * Supply a page size so os_open can decide whether to
		 * turn buffering off if the DB_DIRECT_DB flag is set.
		 *
		 * Acquire the region lock if we're using a path from
		 * an underlying MPOOLFILE -- there's a race in accessing
		 * the path name stored in the region, __memp_nameop may
		 * be simultaneously renaming the file.
		 */
		if(mfp) {
			MPOOL_SYSTEM_LOCK(env);
			path = (const char *)R_ADDR(dbmp->reginfo, mfp->path_off);
		}
		if((ret = __db_appname(env, DB_APP_DATA, path, dirp, &rpath)) == 0)
			ret = __os_open(env, rpath, (uint32)pagesize, oflags, mode, &dbmfp->fhp);
		if(mfp)
			MPOOL_SYSTEM_UNLOCK(env);
		if(ret != 0)
			goto err;
		/*
		 * Cache file handles are shared, and have mutexes to
		 * protect the underlying file handle across seek and
		 * read/write calls.
		 */
		dbmfp->fhp->ref = 1;
		if((ret = __mutex_alloc(env, MTX_MPOOL_FH, DB_MUTEX_PROCESS_ONLY, &dbmfp->fhp->mtx_fh)) != 0)
			goto err;
		/* Figure out the file's size. */
		if((ret = __os_ioinfo(env, rpath, dbmfp->fhp, &mbytes, &bytes, NULL)) != 0) {
			__db_err(env, ret, "%s", rpath);
			goto err;
		}
		/*
		 * Don't permit files that aren't a multiple of the pagesize,
		 * and find the number of the last page in the file, all the
		 * time being careful not to overflow 32 bits.
		 *
		 * During verify or recovery, we might have to cope with a
		 * truncated file; if the file size is not a multiple of the
		 * page size, round down to a page, we'll take care of the
		 * partial page outside the mpool system.
		 */
		DB_ASSERT(env, pagesize != 0);
		if(bytes%pagesize != 0) {
			if(LF_ISSET(DB_ODDFILESIZE))
				bytes -= (uint32)(bytes%pagesize);
			else {
				__db_errx(env, DB_STR_A("3037", "%s: file size not a multiple of the pagesize", "%s"), rpath);
				ret = EINVAL;
				goto err;
			}
		}
		/*
		 * Get the file id if we weren't given one.  Generated file id's
		 * don't use timestamps, otherwise there'd be no chance of any
		 * other process joining the party.  Don't bother looking for
		 * this id in the hash table, its new.
		 */
		if(mfp == NULL && !F_ISSET(dbmfp, MP_FILEID_SET)) {
			if((ret = __os_fileid(env, rpath, 0, dbmfp->fileid)) != 0)
				goto err;
			F_SET(dbmfp, MP_FILEID_SET);
			goto alloc;
		}
	}
	if(mfp)
		goto have_mfp;
	/*
	 * We can race with another process opening the same file when
	 * we allocate the mpoolfile structure.  We will come back
	 * here and check the hash table again to see if it has appeared.
	 * For most files this is not a problem, since the name is locked
	 * at a higher layer but QUEUE extent files are not locked.
	 */
check:  MUTEX_LOCK(env, hp->mtx_hash);
	if((ret = __memp_mpf_find(env, dbmfp, hp, path, flags, &mfp) != 0))
		goto err;
	if(alloc_mfp && mfp == NULL) {
		mfp = alloc_mfp;
		alloc_mfp = NULL;
		SH_TAILQ_INSERT_HEAD(&hp->hash_bucket, mfp, q, __mpoolfile);
	}
	else if(mfp) {
		/*
		 * Some things about a file cannot be changed: the clear length,
		 * page size, or LSN location.  However, if this is an attempt
		 * to open a named in-memory file, we may not yet have that
		 * information. so accept uninitialized entries.
		 *
		 * The file type can change if the application's pre- and post-
		 * processing needs change.  For example, an application that
		 * created a hash subdatabase in a database that was previously
		 * all btree.
		 *
		 * !!!
		 * We do not check to see if the pgcookie information changed,
		 * or update it if it is.
		 */
		if((dbmfp->clear_len != DB_CLEARLEN_NOTSET && mfp->clear_len != DB_CLEARLEN_NOTSET &&
		    dbmfp->clear_len != mfp->clear_len) || (pagesize != 0 && pagesize != mfp->pagesize) ||
		   (dbmfp->lsn_offset != DB_LSN_OFF_NOTSET && mfp->lsn_off != DB_LSN_OFF_NOTSET &&
		    dbmfp->lsn_offset != mfp->lsn_off)) {
			__db_errx(env, DB_STR_A("3038", "%s: clear length, page size or LSN location changed", "%s"), path);
			MUTEX_UNLOCK(env, hp->mtx_hash);
			ret = EINVAL;
			goto err;
		}
	}
	MUTEX_UNLOCK(env, hp->mtx_hash);
	if(alloc_mfp) {
		MUTEX_LOCK(env, alloc_mfp->mutex);
		if((ret = __memp_mf_discard(dbmp, alloc_mfp, 0)) != 0)
			goto err;
	}
	if(!mfp) {
		/*
		 * If we didn't find the file and this is an in-memory file,
		 * then the create flag should be set.
		 */
		if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE) && !LF_ISSET(DB_CREATE)) {
			ret = ENOENT;
			goto err;
		}
alloc:          /*
		 * Get the file ID if we weren't given one.  Generated file
		 * ID's don't use timestamps, otherwise there'd be no
		 * chance of any other process joining the party.
		 */
		if(path && !FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE) && !F_ISSET(dbmfp, MP_FILEID_SET) && (ret = __os_fileid(env, rpath, 0, dbmfp->fileid)) != 0)
			goto err;
		if((ret = __memp_mpf_alloc(dbmp, dbmfp, path, pagesize, flags, &alloc_mfp)) != 0)
			goto err;
		/*
		 * If the user specifies DB_MPOOL_LAST or DB_MPOOL_NEW on a
		 * page get, we have to increment the last page in the file.
		 * Figure it out and save it away.
		 *
		 * Note correction: page numbers are zero-based, not 1-based.
		 */
		DB_ASSERT(env, pagesize != 0);
		last_pgno = (db_pgno_t)(mbytes*(MEGABYTE/pagesize));
		last_pgno += (db_pgno_t)(bytes/pagesize);
		if(last_pgno != 0)
			--last_pgno;
		alloc_mfp->last_flushed_pgno = alloc_mfp->orig_last_pgno = alloc_mfp->last_pgno = last_pgno;
		alloc_mfp->bucket = bucket;
		/* Go back and see if someone else has opened the file. */
		if(path)
			goto check;
		mfp = alloc_mfp;
		/* This is a temp, noone else can see it, put it at the end. */
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_INSERT_TAIL(&hp->hash_bucket, mfp, q);
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
have_mfp:
	/*
	 * We need to verify that all handles open a file either durable or not
	 * durable.  This needs to be cross process and cross sub-databases, so
	 * mpool is the place to do it.
	 */
	if(!LF_ISSET(DB_DURABLE_UNKNOWN|DB_RDONLY)) {
		if(F_ISSET(mfp, MP_DURABLE_UNKNOWN)) {
			if(LF_ISSET(DB_TXN_NOT_DURABLE))
				F_SET(mfp, MP_NOT_DURABLE);
			F_CLR(mfp, MP_DURABLE_UNKNOWN);
		}
		else if(!LF_ISSET(DB_TXN_NOT_DURABLE) != !F_ISSET(mfp, MP_NOT_DURABLE)) {
			__db_errx(env, DB_STR("3039", "Cannot open DURABLE and NOT DURABLE handles in the same file"));
			ret = EINVAL;
			goto err;
		}
	}
	if(LF_ISSET(DB_MULTIVERSION)) {
		atomic_inc(env, &mfp->multiversion);
		F_SET(dbmfp, MP_MULTIVERSION);
	}
	/*
	 * All paths to here have initialized the mfp variable to reference
	 * the selected (or allocated) MPOOLFILE.
	 */
	dbmfp->mfp = mfp;

	/*
	 * Check to see if we can mmap the file.  If a file:
	 *	+ isn't temporary
	 *	+ is read-only
	 *	+ doesn't require any pgin/pgout support
	 *	+ the DB_NOMMAP flag wasn't set (in either the file open or
	 *	  the environment in which it was opened)
	 *	+ and is less than mp_mmapsize bytes in size
	 *
	 * we can mmap it instead of reading/writing buffers.  Don't do error
	 * checking based on the mmap call failure.  We want to do normal I/O
	 * on the file if the reason we failed was because the file was on an
	 * NFS mounted partition, and we can fail in buffer I/O just as easily
	 * as here.
	 *
	 * We'd like to test to see if the file is too big to mmap.  Since we
	 * don't know what size or type off_t's or size_t's are, or the largest
	 * unsigned integral type is, or what random insanity the local C
	 * compiler will perpetrate, doing the comparison in a portable way is
	 * flatly impossible.  Hope that mmap fails if the file is too large.
	 */
#define DB_MAXMMAPSIZE  SMEGABYTE(10)
	if(F_ISSET(mfp, MP_CAN_MMAP) && dbmfp->addr == NULL) {
		maxmap = dbenv->mp_mmapsize == 0 ? DB_MAXMMAPSIZE : dbenv->mp_mmapsize;
		if(path == NULL || FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
			F_CLR(mfp, MP_CAN_MMAP);
		else if(!F_ISSET(dbmfp, MP_READONLY))
			F_CLR(mfp, MP_CAN_MMAP);
		else if(dbmfp->ftype != 0)
			F_CLR(mfp, MP_CAN_MMAP);
		else if(LF_ISSET(DB_NOMMAP) || F_ISSET(dbenv, DB_ENV_NOMMAP))
			F_CLR(mfp, MP_CAN_MMAP);
		else {
			MPOOL_SYSTEM_LOCK(env);
			maxmap = mp->mp_mmapsize == 0 ? DB_MAXMMAPSIZE : mp->mp_mmapsize;
			MPOOL_SYSTEM_UNLOCK(env);
			if(mbytes > maxmap/MEGABYTE || (mbytes == maxmap/MEGABYTE && bytes >= maxmap%MEGABYTE))
				F_CLR(mfp, MP_CAN_MMAP);
		}
		dbmfp->addr = NULL;
		if(F_ISSET(mfp, MP_CAN_MMAP)) {
			dbmfp->len = (size_t)mbytes*MEGABYTE+bytes;
			if(__os_mapfile(env, rpath, dbmfp->fhp, dbmfp->len, 1, &dbmfp->addr) != 0) {
				dbmfp->addr = NULL;
				F_CLR(mfp, MP_CAN_MMAP);
			}
		}
	}
	F_SET(dbmfp, MP_OPEN_CALLED);

	/*
	 * Add the file to the process' list of DB_MPOOLFILEs.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_INSERT_TAIL(&dbmp->dbmfq, dbmfp, q);
	MUTEX_UNLOCK(env, dbmp->mutex);
	if(0) {
err:
		if(refinc) {
			/*
			 * If mpf_cnt goes to zero here and unlink_on_close is
			 * set, then we missed the last close, but there was an
			 * error trying to open the file, so we probably cannot
			 * unlink it anyway.
			 */
			MUTEX_LOCK(env, mfp->mutex);
			--mfp->mpf_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
		}
	}
	__os_free(env, rpath);
	return ret;
}
/*
 * __memp_mpf_find --
 *	Search a hash bucket for a MPOOLFILE.
 */
static int __memp_mpf_find(ENV * env, DB_MPOOLFILE * dbmfp, DB_MPOOL_HASH * hp, const char * path, uint32 flags, MPOOLFILE ** mfpp)
{
	MPOOLFILE * mfp;
	DB_MPOOL * dbmp = env->mp_handle;
	SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
		/* Skip dead files and temporary files. */
		if(mfp->deadfile || F_ISSET(mfp, MP_TEMP))
			continue;
		/*
		 * Any remaining DB_MPOOL_NOFILE databases are in-memory
		 * named databases and need only match other in-memory databases with the same name.
		 */
		if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE)) {
			if(!mfp->no_backing_file)
				continue;
			if(strcmp(path, (const char *)R_ADDR(dbmp->reginfo, mfp->path_off)))
				continue;
			// 
			// We matched an in-memory file; grab the fileid if it is set in the region, but not in the dbmfp.
			// 
			if(!F_ISSET(dbmfp, MP_FILEID_SET))
				__memp_set_fileid(dbmfp, (uint8 *)R_ADDR(dbmp->reginfo, mfp->fileid_off));
		}
		else if(memcmp(dbmfp->fileid, R_ADDR(dbmp->reginfo, mfp->fileid_off), DB_FILE_ID_LEN) != 0)
			continue;
		/*
		 * If the file is being truncated, remove it from the system
		 * and create a new entry.
		 *
		 * !!!
		 * We should be able to set mfp to NULL and break out of the
		 * loop, but I like the idea of checking all the entries.
		 */
		if(LF_ISSET(DB_TRUNCATE)) {
			MUTEX_LOCK(env, mfp->mutex);
			mfp->deadfile = 1;
			MUTEX_UNLOCK(env, mfp->mutex);
			continue;
		}
		/*
		 * Check to see if this file has died while we waited.
		 *
		 * We normally don't lock the deadfile field when we read it as
		 * we only care if the field is zero or non-zero.  We do lock
		 * on read when searching for a matching MPOOLFILE so that two
		 * threads of control don't race between setting the deadfile
		 * bit and incrementing the reference count, that is, a thread
		 * of control decrementing the reference count and then setting
		 * deadfile because the reference count is 0 blocks us finding
		 * the file without knowing it's about to be marked dead.
		 */
		MUTEX_LOCK(env, mfp->mutex);
		if(mfp->deadfile) {
			MUTEX_UNLOCK(env, mfp->mutex);
			continue;
		}
		++mfp->mpf_cnt;
		MUTEX_UNLOCK(env, mfp->mutex);
		// Initialize any fields that are not yet set. 
		if(dbmfp->ftype != 0)
			mfp->ftype = dbmfp->ftype;
		if(dbmfp->clear_len != DB_CLEARLEN_NOTSET)
			mfp->clear_len = dbmfp->clear_len;
		if(dbmfp->lsn_offset != -1)
			mfp->lsn_off = dbmfp->lsn_offset;
		break;
	}
	*mfpp = mfp;
	return 0;
}

static int __memp_mpf_alloc(DB_MPOOL * dbmp, DB_MPOOLFILE * dbmfp, const char * path, uint32 pagesize, uint32 flags, MPOOLFILE ** retmfp)
{
	MPOOLFILE * mfp;
	void * p;
	ENV * env = dbmp->env;
	int ret = 0;
	/* Allocate and initialize a new MPOOLFILE. */
	if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, sizeof(MPOOLFILE), NULL, &mfp)) != 0)
		goto err;
	memzero(mfp, sizeof(MPOOLFILE));
	mfp->mpf_cnt = 1;
	mfp->ftype = dbmfp->ftype;
	mfp->pagesize = pagesize;
	mfp->lsn_off = dbmfp->lsn_offset;
	mfp->clear_len = dbmfp->clear_len;
	mfp->priority = dbmfp->priority;
	if(dbmfp->gbytes != 0 || dbmfp->bytes != 0) {
		mfp->maxpgno = (db_pgno_t)(dbmfp->gbytes*(GIGABYTE/mfp->pagesize));
		mfp->maxpgno += (db_pgno_t)((dbmfp->bytes+mfp->pagesize-1)/mfp->pagesize);
	}
	if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
		mfp->no_backing_file = 1;
	if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_UNLINK))
		mfp->unlink_on_close = 1;
	F_SET(mfp, MP_CAN_MMAP);
	if(F_ISSET(env->dbenv, DB_ENV_DATABASE_LOCKING))
		F_SET(mfp, MP_DATABASE_LOCKING);
	if(LF_ISSET(DB_DIRECT))
		F_SET(mfp, MP_DIRECT);
	if(LF_ISSET(DB_DURABLE_UNKNOWN|DB_RDONLY))
		F_SET(mfp, MP_DURABLE_UNKNOWN);
	if(LF_ISSET(DB_EXTENT))
		F_SET(mfp, MP_EXTENT);
	if(LF_ISSET(DB_TXN_NOT_DURABLE))
		F_SET(mfp, MP_NOT_DURABLE);
	// 
	// An in-memory database with no name is a temp file.  Named in-memory databases get an artificially  bumped reference
	// count so they don't disappear on close; they need a remove to make them disappear.
	// 
	if(!path)
		F_SET(mfp, MP_TEMP);
	else if(FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
		mfp->mpf_cnt++;
	// Copy the file identification string into shared memory. 
	if(F_ISSET(dbmfp, MP_FILEID_SET)) {
		if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, DB_FILE_ID_LEN, &mfp->fileid_off, &p)) != 0)
			goto err;
		memcpy(p, dbmfp->fileid, DB_FILE_ID_LEN);
	}
	// Copy the file path into shared memory.
	if(path) {
		if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, sstrlen(path)+1, &mfp->path_off, &p)) != 0)
			goto err;
		memcpy(p, path, sstrlen(path)+1);
	}
	// Copy the page cookie into shared memory.
	if(!dbmfp->pgcookie || !dbmfp->pgcookie->size) {
		mfp->pgcookie_len = 0;
		mfp->pgcookie_off = 0;
	}
	else {
		if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, dbmfp->pgcookie->size, &mfp->pgcookie_off, &p)) != 0)
			goto err;
		memcpy(p, dbmfp->pgcookie->data, dbmfp->pgcookie->size);
		mfp->pgcookie_len = dbmfp->pgcookie->size;
	}
	if((ret = __mutex_alloc(env, MTX_MPOOLFILE_HANDLE, 0, &mfp->mutex)) != 0)
		goto err;
	*retmfp = mfp;
err:
	return ret;
}
/*
 *	DB_MPOOLFILE->close pre/post processing.
 */
int __memp_fclose_pp(DB_MPOOLFILE * dbmfp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbmfp->env;
	// Validate arguments, but as a handle destructor, we can't fail.
	if(flags != 0)
		__db_ferr(env, "DB_MPOOLFILE->close", 0);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_fclose(dbmfp, 0)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 *	DB_MPOOLFILE->close.
 */
int FASTCALL __memp_fclose(DB_MPOOLFILE * dbmfp, uint32 flags)
{
	MPOOLFILE * mfp;
	char * rpath;
	uint32 ref;
	int deleted, t_ret;
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	int ret = 0;
	/*
	 * Remove the DB_MPOOLFILE from the process' list.
	 *
	 * It's possible the underlying mpool cache may never have been created.
	 * In that case, all we have is a structure, discard it.
	 *
	 * It's possible the DB_MPOOLFILE was never added to the DB_MPOOLFILE
	 * file list, check the MP_OPEN_CALLED flag to be sure.
	 */
	if(dbmp == NULL)
		goto done;
	MUTEX_LOCK(env, dbmp->mutex);
	DB_ASSERT(env, dbmfp->ref >= 1);
	if((ref = --dbmfp->ref) == 0 && F_ISSET(dbmfp, MP_OPEN_CALLED))
		TAILQ_REMOVE(&dbmp->dbmfq, dbmfp, q);
	/*
	 * Decrement the file descriptor's ref count -- if we're the last ref,
	 * we'll discard the file descriptor.
	 */
	if(ref == 0 && dbmfp->fhp && --dbmfp->fhp->ref > 0)
		dbmfp->fhp = NULL;
	MUTEX_UNLOCK(env, dbmp->mutex);
	if(ref != 0)
		return 0;
	/* Complain if pinned blocks never returned. */
	if(dbmfp->pinref != 0) {
		__db_errx(env, DB_STR_A("3040", "%s: close: %lu blocks left pinned", "%s %lu"), __memp_fn(dbmfp), (ulong)dbmfp->pinref);
		ret = __env_panic(env, DB_RUNRECOVERY);
	}
	/* Discard any mmap information. */
	if(dbmfp->addr && (ret = __os_unmapfile(env, dbmfp->addr, dbmfp->len)) != 0)
		__db_err(env, ret, "%s", __memp_fn(dbmfp));
	/*
	 * Close the file and discard the descriptor structure; temporary
	 * files may not yet have been created.
	 */
	if(dbmfp->fhp) {
		if((t_ret = __mutex_free(env, &dbmfp->fhp->mtx_fh)) != 0 && ret == 0)
			ret = t_ret;
		if((t_ret = __os_closehandle(env, dbmfp->fhp)) != 0) {
			__db_err(env, t_ret, "%s", __memp_fn(dbmfp));
			SETIFZ(ret, t_ret);
		}
		dbmfp->fhp = NULL;
	}
	/*
	 * Discard our reference on the underlying MPOOLFILE, and close it
	 * if it's no longer useful to anyone.  It possible the open of the
	 * file never happened or wasn't successful, in which case, mpf will
	 * be NULL and MP_OPEN_CALLED will not be set.
	 */
	mfp = dbmfp->mfp;
	DB_ASSERT(env, (F_ISSET(dbmfp, MP_OPEN_CALLED) && mfp) || (!F_ISSET(dbmfp, MP_OPEN_CALLED) && mfp == NULL));
	if(!F_ISSET(dbmfp, MP_OPEN_CALLED))
		goto done;
	/*
	 * If it's a temp file, all outstanding references belong to unflushed
	 * buffers.  (A temp file can only be referenced by one DB_MPOOLFILE).
	 * We don't care about preserving any of those buffers, so mark the
	 * MPOOLFILE as dead so that even the dirty ones just get discarded
	 * when we try to flush them.
	 */
	deleted = 0;
	if(!LF_ISSET(DB_MPOOL_NOLOCK))
		MUTEX_LOCK(env, mfp->mutex);
	if(F_ISSET(dbmfp, MP_MULTIVERSION))
		atomic_dec(env, &mfp->multiversion);
	if(--mfp->mpf_cnt == 0 || LF_ISSET(DB_MPOOL_DISCARD)) {
		if(LF_ISSET(DB_MPOOL_DISCARD) || F_ISSET(mfp, MP_TEMP) || mfp->unlink_on_close) {
			mfp->deadfile = 1;
		}
		if(mfp->unlink_on_close) {
			if((t_ret = __db_appname(dbmp->env, DB_APP_DATA, (const char *)R_ADDR(dbmp->reginfo, mfp->path_off), NULL, &rpath)) != 0 && ret == 0)
				ret = t_ret;
			if(t_ret == 0) {
				if((t_ret = __os_unlink(dbmp->env, rpath, 0)) != 0 && ret == 0)
					ret = t_ret;
				__os_free(env, rpath);
			}
		}
		if(mfp->mpf_cnt == 0) {
			F_CLR(mfp, MP_NOT_DURABLE);
			F_SET(mfp, MP_DURABLE_UNKNOWN);
		}
		if(mfp->block_cnt == 0) {
			/*
			 * We should never discard this mp file if our caller
			 * is holding the lock on it.  See comment in
			 * __memp_sync_file.
			 */
			DB_ASSERT(env, !LF_ISSET(DB_MPOOL_NOLOCK));
			if((t_ret = __memp_mf_discard(dbmp, mfp, 0)) != 0 && ret == 0)
				ret = t_ret;
			deleted = 1;
		}
	}
	if(!deleted && !LF_ISSET(DB_MPOOL_NOLOCK))
		MUTEX_UNLOCK(env, mfp->mutex);
done:   /* Discard the DB_MPOOLFILE structure. */
	if(dbmfp->pgcookie) {
		__os_free(env, dbmfp->pgcookie->data);
		__os_free(env, dbmfp->pgcookie);
	}
	__os_free(env, dbmfp);
	return ret;
}
//
// __memp_mf_discard --
// Discard an MPOOLFILE.
//
int __memp_mf_discard(DB_MPOOL * dbmp, MPOOLFILE * mfp, int hp_locked)
{
#ifdef HAVE_STATISTICS
	DB_MPOOL_STAT * sp;
#endif
	int need_sync, t_ret;
	int ret = 0;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	hp += mfp->bucket;
	/*
	 * Expects caller to be holding the MPOOLFILE mutex.
	 *
	 * When discarding a file, we have to flush writes from it to disk.
	 * The scenario is that dirty buffers from this file need to be
	 * flushed to satisfy a future checkpoint, but when the checkpoint
	 * calls mpool sync, the sync code won't know anything about them.
	 * Ignore files not written, discarded, or only temporary.
	 */
	need_sync = mfp->file_written && !mfp->deadfile && !F_ISSET(mfp, MP_TEMP) && !mfp->no_backing_file;
	/*
	 * We have to release the MPOOLFILE mutex before acquiring the region
	 * mutex so we don't deadlock.  Make sure nobody ever looks at this
	 * structure again.
	 */
	mfp->deadfile = 1;
	/* Discard the mutex we're holding and return it too the pool. */
	MUTEX_UNLOCK(env, mfp->mutex);
	if((t_ret = __mutex_free(env, &mfp->mutex)) != 0 && ret == 0)
		ret = t_ret;
	/*
	 * Lock the bucket and delete from the list of MPOOLFILEs.
	 * If this function is called by __memp_discard_all_mpfs,
	 * the MPOOLFILE hash bucket is already locked.
	 */
	if(!hp_locked)
		MUTEX_LOCK(env, hp->mtx_hash);
	SH_TAILQ_REMOVE(&hp->hash_bucket, mfp, q, __mpoolfile);
	if(!hp_locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);
	/* Lock the region and collect stats and free the space. */
	MPOOL_SYSTEM_LOCK(env);
	if(need_sync && (t_ret = __memp_mf_sync(dbmp, mfp, 0)) != 0 && ret == 0)
		ret = t_ret;
#ifdef HAVE_STATISTICS
	/* Copy the statistics into the region. */
	sp = &mp->stat;
	sp->st_cache_hit += mfp->stat.st_cache_hit;
	sp->st_cache_miss += mfp->stat.st_cache_miss;
	sp->st_map += mfp->stat.st_map;
	sp->st_page_create += mfp->stat.st_page_create;
	sp->st_page_in += mfp->stat.st_page_in;
	sp->st_page_out += mfp->stat.st_page_out;
#endif
	/* Free the space. */
	if(mfp->path_off != 0)
		__memp_free(&dbmp->reginfo[0], R_ADDR(dbmp->reginfo, mfp->path_off));
	if(mfp->fileid_off != 0)
		__memp_free(&dbmp->reginfo[0], R_ADDR(dbmp->reginfo, mfp->fileid_off));
	if(mfp->pgcookie_off != 0)
		__memp_free(&dbmp->reginfo[0], R_ADDR(dbmp->reginfo, mfp->pgcookie_off));
	__memp_free(&dbmp->reginfo[0], mfp);
	MPOOL_SYSTEM_UNLOCK(env);
	return ret;
}
/*
 * __memp_inmemlist --
 *	Return a list of the named in-memory databases.
 */
int __memp_inmemlist(ENV * env, char *** namesp, int * cntp)
{
	MPOOLFILE * mfp;
	int i, ret;
	char ** names = NULL;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	int arraysz = 0;
	int cnt = 0;
	for(i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
			/* Skip dead files and temporary files. */
			if(mfp->deadfile || F_ISSET(mfp, MP_TEMP))
				continue;
			/* Skip entries that allow files. */
			if(!mfp->no_backing_file)
				continue;
			/* We found one. */
			if(cnt >= arraysz) {
				arraysz += 100;
				if((ret = __os_realloc(env, (uint)arraysz*sizeof(names[0]), &names)) != 0)
					goto nomem;
			}
			if((ret = __os_strdup(env, (const char *)R_ADDR(dbmp->reginfo, mfp->path_off), &names[cnt])) != 0)
				goto nomem;
			cnt++;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	*namesp = names;
	*cntp = cnt;
	return 0;
nomem:
	MUTEX_UNLOCK(env, hp->mtx_hash);
	if(names) {
		while(--cnt >= 0)
			__os_free(env, names[cnt]);
		__os_free(env, names);
	}
	/* Make sure we don't return any garbage. */
	*cntp = 0;
	*namesp = NULL;
	return ret;
}
/*
 * __memp_fput_pp --
 *	DB_MPOOLFILE->put pre/post processing.
 */
int __memp_fput_pp(DB_MPOOLFILE * dbmfp, void * pgaddr, DB_CACHE_PRIORITY priority, uint32 flags)
{
	int ret = 0;
	int t_ret;
	ENV * env = dbmfp->env;
	if(flags != 0)
		ret = __db_ferr(env, "DB_MPOOLFILE->put", 0);
	else {
		DB_THREAD_INFO * ip;
		MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->put");
		ENV_ENTER(env, ip);
		ret = __memp_fput(dbmfp, ip, pgaddr, priority);
		if(IS_ENV_REPLICATED(env) && (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
			ret = t_ret;
		ENV_LEAVE(env, ip);
	}
	return ret;
}
//
// __memp_reset_lru --
// Reset the cache LRU priority when it reaches the upper limit.
//
static int FASTCALL __memp_reset_lru(ENV * env, REGINFO * infop)
{
	BH * bhp, * tbhp;
	DB_MPOOL_HASH * hp;
	uint32 bucket;
	/*
	 * Update the priority so all future allocations will start at the
	 * bottom. Lock this cache region to ensure that exactly one thread
	 * will reset this cache's buffers.
	 */
	MPOOL * c_mp = (MPOOL *)infop->primary;
	MPOOL_REGION_LOCK(env, infop);
	const int reset = (c_mp->lru_priority >= MPOOL_LRU_DECREMENT);
	if(reset) {
		c_mp->lru_priority -= MPOOL_LRU_DECREMENT;
		c_mp->lru_generation++;
	}
	MPOOL_REGION_UNLOCK(env, infop);
	if(reset) {
		/* Reduce the priority of every buffer in this cache region. */
		for(hp = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab), bucket = 0; bucket < c_mp->htab_buckets; ++hp, ++bucket) {
			/*
			 * Skip empty buckets.
			 *
			 * We can check for empty buckets before locking as we
			 * only care if the pointer is zero or non-zero.
			 */
			if(SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL)
				continue;
			MUTEX_LOCK(env, hp->mtx_hash);
			SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
				for(tbhp = bhp; tbhp; tbhp = SH_CHAIN_PREV(tbhp, vc, __bh)) {
					if(tbhp->priority > MPOOL_LRU_DECREMENT)
						tbhp->priority -= MPOOL_LRU_DECREMENT;
					else
						tbhp->priority = 0;
				}
			}
			MUTEX_UNLOCK(env, hp->mtx_hash);
		}
		COMPQUIET(env, 0);
	}
	return 0;
}

int FASTCALL __memp_fput(DB_MPOOLFILE * dbmfp, DB_THREAD_INFO * ip, void * pgaddr, DB_CACHE_PRIORITY priority)
{
	int    ret = 0;
	if(pgaddr) {
		DB_MPOOL_HASH * hp;
		MPOOL * c_mp;
		PIN_LIST * list, * lp;
		REGINFO * infop, * reginfo;
		roff_t b_ref;
		int region;
		int adjust, pfactor, t_ret;
		char buf[DB_THREADID_STRLEN];
		ENV * env = dbmfp->env;
		DB_ENV * dbenv = env->dbenv;
		DB_MPOOL * dbmp = env->mp_handle;
		MPOOLFILE * mfp = dbmfp->mfp;
		BH * bhp = (BH *)((uint8 *)pgaddr-SSZA(BH, buf));
		//
		// If this is marked dummy, we are using it to unpin a buffer for another thread.
		//
		if(F_ISSET(dbmfp, MP_DUMMY))
			goto unpin;
		//
		// If we're mapping the file, there's nothing to do.  Because we can
		// stop mapping the file at any time, we have to check on each buffer
		// to see if the address we gave the application was part of the map region.
		//
		if(dbmfp->addr && pgaddr >= dbmfp->addr && (uint8 *)pgaddr <= (uint8 *)dbmfp->addr+dbmfp->len)
			return 0;
		DB_ASSERT(env, IS_RECOVERING(env) || bhp->pgno <= mfp->last_pgno || F_ISSET(bhp, BH_FREED) || !SH_CHAIN_SINGLETON(bhp, vc));
	#ifdef DIAGNOSTIC
		//
		// Decrement the per-file pinned buffer count (mapped pages aren't counted).
		//
		MPOOL_SYSTEM_LOCK(env);
		if(dbmfp->pinref == 0) {
			MPOOL_SYSTEM_UNLOCK(env);
			__db_errx(env, DB_STR_A("3011", "%s: more pages returned than retrieved", "%s"), __memp_fn(dbmfp));
			return __env_panic(env, EACCES);
		}
		--dbmfp->pinref;
		MPOOL_SYSTEM_UNLOCK(env);
	#endif
	unpin:
		infop = &dbmp->reginfo[bhp->region];
		c_mp = (MPOOL *)infop->primary;
		hp = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab);
		hp = &hp[bhp->bucket];
		//
		// Check for a reference count going to zero.  This can happen if the application returns a page twice.
		//
		if(atomic_read(&bhp->ref) == 0) {
			__db_errx(env, DB_STR_A("3012", "%s: page %lu: unpinned page returned", "%s %lu"), __memp_fn(dbmfp), (ulong)bhp->pgno);
			DB_ASSERT(env, atomic_read(&bhp->ref) != 0);
			return __env_panic(env, EACCES);
		}
		// Note the activity so allocation won't decide to quit.
		++c_mp->put_counter;
		if(ip) {
			reginfo = env->reginfo;
			list = (PIN_LIST *)R_ADDR(reginfo, ip->dbth_pinlist);
			region = (int)(infop-dbmp->reginfo);
			b_ref = R_OFFSET(infop, bhp);
			for(lp = list; lp < &list[ip->dbth_pinmax]; lp++)
				if(lp->b_ref == b_ref && lp->region == region)
					break;
			if(lp == &list[ip->dbth_pinmax]) {
				__db_errx(env, DB_STR_A("3013", "__memp_fput: pinned buffer not found for thread %s", "%s"), dbenv->thread_id_string(dbenv, ip->dbth_pid, ip->dbth_tid, buf));
				return __env_panic(env, EINVAL);
			}
			lp->b_ref = INVALID_ROFF;
			ip->dbth_pincount--;
		}
		//
		// Mark the file dirty.
		//
		if(F_ISSET(bhp, BH_EXCLUSIVE) && F_ISSET(bhp, BH_DIRTY)) {
			DB_ASSERT(env, atomic_read(&hp->hash_page_dirty) > 0);
			mfp->file_written = 1;
		}
		//
		// If more than one reference to the page we're done.  Ignore the discard flags (for now) and leave the buffer's priority alone.
		// We are doing this a little early as the remaining ref may or may not be a write behind.  If it is we set the priority
		// here, if not it will get set again later.  We might race and miss setting the priority which would leave it wrong for a while.
		//
		DB_ASSERT(env, atomic_read(&bhp->ref) != 0);
		if(atomic_dec(env, &bhp->ref) > 1 || (atomic_read(&bhp->ref) == 1 && !F_ISSET(bhp, BH_DIRTY))) {
			//
			// __memp_pgwrite only has a shared lock while it clears
			// the BH_DIRTY bit. If we only have a shared latch then we can't touch the flags bits.
			//
			if(F_ISSET(bhp, BH_EXCLUSIVE))
				F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			return 0;
		}
		// The buffer should not be accessed again
	#ifdef DIAG_MVCC
		MUTEX_LOCK(env, hp->mtx_hash);
		if(BH_REFCOUNT(bhp) == 0)
			MVCC_MPROTECT(bhp->buf, mfp->pagesize, 0);
		MUTEX_UNLOCK(env, hp->mtx_hash);
	#endif
		// Update priority values
		if(priority == DB_PRIORITY_VERY_LOW || mfp->priority == MPOOL_PRI_VERY_LOW)
			bhp->priority = 0;
		else {
			//
			// We don't lock the LRU priority or the pages field, if
			// we get garbage (which won't happen on a 32-bit machine), it
			// only means a buffer has the wrong priority.
			//
			bhp->priority = c_mp->lru_priority;
			switch(priority) {
				default:
				case DB_PRIORITY_UNCHANGED: pfactor = mfp->priority; break;
				case DB_PRIORITY_VERY_LOW:  pfactor = MPOOL_PRI_VERY_LOW; break;
				case DB_PRIORITY_LOW:       pfactor = MPOOL_PRI_LOW; break;
				case DB_PRIORITY_DEFAULT:   pfactor = MPOOL_PRI_DEFAULT; break;
				case DB_PRIORITY_HIGH:      pfactor = MPOOL_PRI_HIGH; break;
				case DB_PRIORITY_VERY_HIGH: pfactor = MPOOL_PRI_VERY_HIGH; break;
			}
			adjust = 0;
			if(pfactor != 0)
				adjust = (int)c_mp->pages/pfactor;
			if(F_ISSET(bhp, BH_DIRTY))
				adjust += (int)c_mp->pages/MPOOL_PRI_DIRTY;
			if(adjust > 0) {
				if(MPOOL_LRU_REDZONE-bhp->priority >= (uint32)adjust)
					bhp->priority += adjust;
			}
			else if(adjust < 0)
				if(bhp->priority > (uint32)-adjust)
					bhp->priority += adjust;
		}
		//
		// __memp_pgwrite only has a shared lock while it clears the
		// BH_DIRTY bit. If we only have a shared latch then we can't touch the flags bits.
		//
		if(F_ISSET(bhp, BH_EXCLUSIVE))
			F_CLR(bhp, BH_EXCLUSIVE);
		MUTEX_UNLOCK(env, bhp->mtx_buf);
		//
		// On every buffer put we update the cache lru priority and check
		// for wraparound. The increment doesn't need to be atomic: occasional
		// lost increments are okay; __memp_reset_lru handles race conditions.
		//
		if(++c_mp->lru_priority >= MPOOL_LRU_REDZONE && (t_ret = __memp_reset_lru(env, infop)) != 0 && ret == 0)
			ret = t_ret;
	}
	return ret;
}
/*
 * __memp_unpin_buffers --
 *	Unpin buffers pinned by a thread.
 */
int __memp_unpin_buffers(ENV * env, DB_THREAD_INFO * ip)
{
	DB_MPOOL * dbmp;
	DB_MPOOLFILE dbmf;
	REGINFO * reginfo;
	int ret = 0;
	memzero(&dbmf, sizeof(dbmf));
	dbmf.env = env;
	dbmf.flags = MP_DUMMY;
	dbmp = env->mp_handle;
	reginfo = env->reginfo;
	PIN_LIST * list = (PIN_LIST *)R_ADDR(reginfo, ip->dbth_pinlist);
	for(PIN_LIST * lp = list; lp < &list[ip->dbth_pinmax]; lp++) {
		if(lp->b_ref != INVALID_ROFF) {
			REGINFO * rinfop = &dbmp->reginfo[lp->region];
			BH * bhp = (BH *)R_ADDR(rinfop, lp->b_ref);
			dbmf.mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
			if((ret = __memp_fput(&dbmf, ip, (uint8 *)bhp+SSZA(BH, buf), DB_PRIORITY_UNCHANGED)) != 0)
				return ret;
		}
	}
	return 0;
}
/*
 * __memp_dirty --
 *	Upgrade a page from a read-only to a writable pointer.
 */
int FASTCALL __memp_dirty(DB_MPOOLFILE * dbmfp, void * addrp, DB_THREAD_INFO * ip, DB_TXN * txn, DB_CACHE_PRIORITY priority, uint32 flags)
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
	for(ancestor = txn; ancestor && ancestor->parent; ancestor = ancestor->parent)
		;
	if(mvcc && txn && flags == DB_MPOOL_DIRTY && (!BH_OWNED_BY(env, bhp, ancestor) || SH_CHAIN_HASNEXT(bhp, vc))) {
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
/*
 * __memp_bh_settxn --
 *	Set the transaction that owns the given buffer.
 */
int __memp_bh_settxn(DB_MPOOL * dbmp, MPOOLFILE * mfp, BH * bhp, void * vtd)
{
	ENV * env = dbmp->env;
	TXN_DETAIL * td = (TXN_DETAIL *)vtd;
	if(!td) {
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
	DB_MPOOL_HASH * hp;
	DB_TXN * txn;
	REGINFO * infop;
	int ret = 0;
	uint32 bucket;
	ENV * env = dbc->env;
	DB_MPOOL * dbmp = env->mp_handle;
	DB_MPOOLFILE * dbmfp = dbc->dbp->mpf;
	MPOOLFILE * mfp = dbmfp->mfp;
	roff_t mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	int skip = 0;
	for(txn = dbc->txn; txn->parent; txn = txn->parent)
		;
	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	MP_GET_BUCKET(env, mfp, pgno, &infop, hp, bucket, ret);
	if(ret != 0) {
		// Panic: there is no way to return the error
		__env_panic(env, ret);
		return 0;
	}
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		if(bhp->pgno == pgno && bhp->mf_offset == mf_offset) {
			if(!BH_OWNED_BY(env, bhp, txn))
				skip = 1;
			break;
		}
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
		if(fhp && (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
			ret = t_ret;
		if(created) {
			DB_ASSERT(env, h_locked);
			if((t_ret = __os_unlink(env, real_name, 0)) != 0 && ret == 0)
				ret = t_ret;
		}
		if(h_locked)
			MUTEX_UNLOCK(env, hp->mtx_hash);
		SETIFZ(ret, EIO);
		if(frozen_bhp) {
			MPOOL_REGION_LOCK(env, infop);
			SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen, frozen_bhp, hq);
			MPOOL_REGION_UNLOCK(env, infop);
		}
	}
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
	if(alloc_bhp) {
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
	if(alloc_bhp) {
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
	if(alloc_bhp)
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
	if(fhp && (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
		ret = t_ret;
	if(ret != 0)
		__db_err(env, ret, "__memp_bh_thaw");
	return ret;
}
/*
 * __memp_open --
 *	Internal version of memp_open: only called from ENV->open.
 */
int __memp_open(ENV * env, int create_ok)
{
	DB_MPOOL * dbmp;
	MPOOL * mp, * mp_i;
	REGINFO reginfo;
	roff_t max_size, reg_size;
	uint i, max_nreg;
	uint32 htab_buckets, * regids;
	int ret;
	DB_ENV * dbenv = env->dbenv;
	roff_t cache_size = 0;
	// Calculate the region size and hash bucket count. 
	__memp_region_size(env, &max_size, &htab_buckets);
	// Create and initialize the DB_MPOOL structure. 
	if((ret = __os_calloc(env, 1, sizeof(*dbmp), &dbmp)) != 0)
		return ret;
	LIST_INIT(&dbmp->dbregq);
	TAILQ_INIT(&dbmp->dbmfq);
	dbmp->env = env;

	/* Join/create the first mpool region. */
	memzero(&reginfo, sizeof(REGINFO));
	reginfo.env = env;
	reginfo.type = REGION_TYPE_MPOOL;
	reginfo.id = INVALID_REGION_ID;
	reginfo.flags = REGION_JOIN_OK;

	/* Calculate the minimum allocation. */
	reg_size = sizeof(MPOOL);
	reg_size += MPOOL_FILE_BUCKETS*sizeof(DB_MPOOL_HASH);
	reg_size += htab_buckets*sizeof(DB_MPOOL_HASH);
	reg_size += (dbenv->mp_pagesize == 0 ? MPOOL_DEFAULT_PAGESIZE : dbenv->mp_pagesize)*10;
	if(reg_size > max_size)
		reg_size = max_size;
	if(create_ok)
		F_SET(&reginfo, REGION_CREATE_OK);
	if((ret = __env_region_attach(env, &reginfo, reg_size, max_size)) != 0)
		goto err;
	cache_size = reginfo.rp->max;
	if(F_ISSET(env, ENV_PRIVATE))
		reginfo.max_alloc = reginfo.rp->max;
	/*
	 * If we created the region, initialize it.  Create or join any
	 * additional regions.
	 */
	if(F_ISSET(&reginfo, REGION_CREATE)) {
		/*
		 * We define how many regions there are going to be, allocate
		 * the REGINFO structures and create them.  Make sure we don't
		 * clear the wrong entries on error.
		 */
		max_nreg = __memp_max_regions(env);
		if((ret = __os_calloc(env, max_nreg, sizeof(REGINFO), &dbmp->reginfo)) != 0)
			goto err;
		/* Make sure we don't clear the wrong entries on error. */
		dbmp->reginfo[0] = reginfo;
		for(i = 1; i < max_nreg; ++i)
			dbmp->reginfo[i].id = INVALID_REGION_ID;
		/* Initialize the first region. */
		if((ret = __memp_init(env, dbmp, 0, htab_buckets, max_nreg)) != 0)
			goto err;
		/*
		 * Create/initialize remaining regions and copy their IDs into
		 * the first region.
		 */
		mp = (MPOOL *)R_ADDR(dbmp->reginfo, dbmp->reginfo[0].rp->primary);
		regids = (uint32 *)R_ADDR(dbmp->reginfo, mp->regids);
		regids[0] = dbmp->reginfo[0].id;
		for(i = 1; i < dbenv->mp_ncache; ++i) {
			dbmp->reginfo[i].env = env;
			dbmp->reginfo[i].type = REGION_TYPE_MPOOL;
			dbmp->reginfo[i].id = INVALID_REGION_ID;
			dbmp->reginfo[i].flags = REGION_CREATE_OK;
			if((ret = __env_region_attach(env, &dbmp->reginfo[i], reg_size, max_size)) != 0)
				goto err;
			if(F_ISSET(env, ENV_PRIVATE))
				dbmp->reginfo[i].max_alloc = max_size;
			cache_size += dbmp->reginfo[i].rp->max;
			if((ret = __memp_init(env, dbmp, i, htab_buckets, max_nreg)) != 0)
				goto err;
			regids[i] = dbmp->reginfo[i].id;
		}
		mp->gbytes = (uint32)(cache_size/GIGABYTE);
		mp->bytes = (uint32)(cache_size%GIGABYTE);
	}
	else {
		/*
		 * Determine how many regions there are going to be, allocate
		 * the REGINFO structures and fill in local copies of that
		 * information.
		 */
		mp = (MPOOL *)R_ADDR(&reginfo, reginfo.rp->primary);
		dbenv->mp_ncache = mp->nreg;
		if((ret = __os_calloc(env, mp->max_nreg, sizeof(REGINFO), &dbmp->reginfo)) != 0)
			goto err;
		/* Make sure we don't clear the wrong entries on error. */
		for(i = 0; i < dbenv->mp_ncache; ++i)
			dbmp->reginfo[i].id = INVALID_REGION_ID;
		dbmp->reginfo[0] = reginfo;

		/* Join remaining regions. */
		regids = (uint32 *)R_ADDR(dbmp->reginfo, mp->regids);
		for(i = 1; i < dbenv->mp_ncache; ++i) {
			dbmp->reginfo[i].env = env;
			dbmp->reginfo[i].type = REGION_TYPE_MPOOL;
			dbmp->reginfo[i].id = regids[i];
			dbmp->reginfo[i].flags = REGION_JOIN_OK;
			if((ret = __env_region_attach(env, &dbmp->reginfo[i], 0, 0)) != 0)
				goto err;
		}
	}
	/* Set the local addresses for the regions. */
	for(i = 0; i < dbenv->mp_ncache; ++i) {
		mp_i = (MPOOL *)(dbmp->reginfo[i].primary = R_ADDR(&dbmp->reginfo[i], dbmp->reginfo[i].rp->primary));
		dbmp->reginfo[i].mtx_alloc = mp_i->mtx_region;
	}
	/* If the region is threaded, allocate a mutex to lock the handles. */
	if((ret = __mutex_alloc(env, MTX_MPOOL_HANDLE, DB_MUTEX_PROCESS_ONLY, &dbmp->mutex)) != 0)
		goto err;
	env->mp_handle = dbmp;
	/* A process joining the region may reset the mpool configuration. */
	if((ret = __memp_init_config(env, mp)) != 0)
		return ret;
	return 0;
err:
	env->mp_handle = NULL;
	if(dbmp->reginfo && dbmp->reginfo[0].addr) {
		for(i = 0; i < dbenv->mp_ncache; ++i)
			if(dbmp->reginfo[i].id != INVALID_REGION_ID)
				__env_region_detach(env, &dbmp->reginfo[i], 0);
		__os_free(env, dbmp->reginfo);
	}
	__mutex_free(env, &dbmp->mutex);
	__os_free(env, dbmp);
	return ret;
}
/*
 * __memp_init --
 *	Initialize a MPOOL structure in shared memory.
 */
int __memp_init(ENV * env, DB_MPOOL * dbmp, uint reginfo_off, uint32 htab_buckets, uint max_nreg)
{
	BH * frozen_bhp;
	BH_FROZEN_ALLOC * frozen;
	DB_MPOOL_HASH * htab, * hp;
	MPOOL * mp, * main_mp;
	db_mutex_t mtx_base, mtx_discard, mtx_prev;
	uint32 i;
	int ret;
	void * p;
	DB_ENV * dbenv = env->dbenv;
	REGINFO * infop = &dbmp->reginfo[reginfo_off];
	if((ret = __env_alloc(infop, sizeof(MPOOL), &infop->primary)) != 0)
		goto mem_err;
	infop->rp->primary = R_OFFSET(infop, infop->primary);
	mp = (MPOOL *)infop->primary;
	memzero(mp, sizeof(*mp));
	if((ret = __mutex_alloc(env, MTX_MPOOL_REGION, 0, &mp->mtx_region)) != 0)
		return ret;
	if(reginfo_off == 0) {
		ZERO_LSN(mp->lsn);
		mp->nreg = dbenv->mp_ncache;
		mp->max_nreg = max_nreg;
		if((ret = __env_alloc(&dbmp->reginfo[0], max_nreg*sizeof(uint32), &p)) != 0)
			goto mem_err;
		mp->regids = R_OFFSET(dbmp->reginfo, p);
		mp->nbuckets = dbenv->mp_ncache*htab_buckets;
		/* Allocate file table space and initialize it. */
		if((ret = __env_alloc(infop, MPOOL_FILE_BUCKETS*sizeof(DB_MPOOL_HASH), &htab)) != 0)
			goto mem_err;
		mp->ftab = R_OFFSET(infop, htab);
		for(i = 0; i < MPOOL_FILE_BUCKETS; i++) {
			if((ret = __mutex_alloc(env, MTX_MPOOL_FILE_BUCKET, 0, &htab[i].mtx_hash)) != 0)
				return ret;
			SH_TAILQ_INIT(&htab[i].hash_bucket);
			atomic_init(&htab[i].hash_page_dirty, 0);
		}
		/*
		 * Allocate all of the hash bucket mutexes up front.  We do
		 * this so that we don't need to free and reallocate mutexes as
		 * the cache is resized.
		 */
		mtx_base = mtx_prev = MUTEX_INVALID;
		if(!MUTEX_ON(env) || F_ISSET(env, ENV_PRIVATE))
			goto no_prealloc;
		for(i = 0; i < mp->max_nreg*dbenv->mp_mtxcount; i++) {
			if((ret = __mutex_alloc(env, MTX_MPOOL_HASH_BUCKET, DB_MUTEX_SHARED, &mtx_discard)) != 0)
				return ret;
			if(i == 0)
				mtx_base = mtx_discard;
			else
				DB_ASSERT(env, mtx_base == MUTEX_INVALID || mtx_discard == mtx_prev+1);
			mtx_prev = mtx_discard;
		}
	}
	else {
		main_mp = (MPOOL *)dbmp->reginfo[0].primary;
		htab = (DB_MPOOL_HASH *)R_ADDR(&dbmp->reginfo[0], main_mp->htab);
		mtx_base = htab[0].mtx_hash;
	}
	/*
	 * We preallocated all of the mutexes in a block, so for regions after
	 * the first, we skip mutexes in use in earlier regions.  Each region
	 * has the same number of buckets
	 */
no_prealloc:
	if(MUTEX_ON(env))
		mtx_base += reginfo_off*dbenv->mp_mtxcount;
	/* Allocate hash table space and initialize it. */
	if((ret = __env_alloc(infop, htab_buckets*sizeof(DB_MPOOL_HASH), &htab)) != 0)
		goto mem_err;
	mp->htab = R_OFFSET(infop, htab);
	for(i = 0; i < htab_buckets; i++) {
		hp = &htab[i];
		if(!MUTEX_ON(env) || dbenv->mp_mtxcount == 0)
			hp->mtx_hash = MUTEX_INVALID;
		else if(F_ISSET(env, ENV_PRIVATE)) {
			if(i >= dbenv->mp_mtxcount)
				hp->mtx_hash = htab[i%dbenv->mp_mtxcount].mtx_hash;
			else if((ret = __mutex_alloc(env, MTX_MPOOL_HASH_BUCKET, DB_MUTEX_SHARED, &hp->mtx_hash)) != 0)
				return ret;
		}
		else
			hp->mtx_hash = mtx_base+(i%dbenv->mp_mtxcount);
		SH_TAILQ_INIT(&hp->hash_bucket);
		atomic_init(&hp->hash_page_dirty, 0);
#ifdef HAVE_STATISTICS
		hp->hash_io_wait = 0;
		hp->hash_frozen = hp->hash_thawed = hp->hash_frozen_freed = 0;
#endif
		hp->flags = 0;
		ZERO_LSN(hp->old_reader);
	}
	mp->htab_buckets = htab_buckets;
	mp->htab_mutexes = dbenv->mp_mtxcount;
	mp->pagesize = dbenv->mp_pagesize == 0 ? MPOOL_DEFAULT_PAGESIZE : dbenv->mp_pagesize;
	SH_TAILQ_INIT(&mp->free_frozen);
	SH_TAILQ_INIT(&mp->alloc_frozen);
	/*
	 * Pre-allocate one frozen buffer header.  This avoids situations where
	 * the cache becomes full of pages and we don't even have the 28 bytes
	 * (or so) available to allocate a frozen buffer header.
	 */
	if((ret = __env_alloc(infop, sizeof(BH_FROZEN_ALLOC)+sizeof(BH_FROZEN_PAGE), &frozen)) != 0)
		goto mem_err;
	SH_TAILQ_INSERT_TAIL(&mp->alloc_frozen, frozen, links);
	frozen_bhp = (BH *)(frozen+1);
	frozen_bhp->mtx_buf = MUTEX_INVALID;
	SH_TAILQ_INSERT_TAIL(&mp->free_frozen, frozen_bhp, hq);
	/*
	 * Only the environment creator knows the total cache size,
	 * fill in those fields now.
	 */
	mp->gbytes = dbenv->mp_gbytes;
	mp->bytes = dbenv->mp_bytes;
	infop->mtx_alloc = mp->mtx_region;
	return 0;
mem_err:
	__db_errx(env, DB_STR("3026", "Unable to allocate memory for mpool region"));
	return ret;
}

uint32 __memp_max_regions(ENV * env)
{
	roff_t reg_size, max_size;
	size_t max_nreg;
	DB_ENV * dbenv = env->dbenv;
	if(dbenv->mp_max_gbytes == 0 && dbenv->mp_max_bytes == 0)
		return dbenv->mp_ncache;
	__memp_region_size(env, &reg_size, 0);
	max_size = (roff_t)dbenv->mp_max_gbytes*GIGABYTE+dbenv->mp_max_bytes;
	max_nreg = (max_size+reg_size/2)/reg_size;
	/* Sanity check that the number of regions fits in 32 bits. */
	DB_ASSERT(env, max_nreg == (uint32)max_nreg);
	if(max_nreg <= dbenv->mp_ncache)
		max_nreg = dbenv->mp_ncache;
	return (uint32)max_nreg;
}
/*
 * __memp_region_size --
 *	Size the region and figure out how many hash buckets we'll have.
 */
static void __memp_region_size(ENV * env, roff_t * reg_sizep, uint32 * htab_bucketsp)
{
	roff_t reg_size, cache_size;
	uint32 pgsize;
	DB_ENV * dbenv = env->dbenv;
	/*
	 * Figure out how big each cache region is.  Cast an operand to roff_t
	 * so we do 64-bit arithmetic as appropriate.
	 */
	cache_size = (roff_t)dbenv->mp_gbytes*GIGABYTE+dbenv->mp_bytes;
	reg_size = cache_size/dbenv->mp_ncache;
	ASSIGN_PTR(reg_sizep, reg_size);
	/*
	 * Figure out how many hash buckets each region will have.  Assume we
	 * want to keep the hash chains with under 3 pages on each chain.  We
	 * don't know the pagesize in advance, and it may differ for different
	 * files.  Use a pagesize of 4K for the calculation -- we walk these
	 * chains a lot, they must be kept short.  We use 2.5 as this maintains
	 * compatibility with previous releases.
	 *
	 * XXX
	 * Cache sizes larger than 10TB would cause 32-bit wrapping in the
	 * calculation of the number of hash buckets.  This probably isn't
	 * something we need to worry about right now, but is checked when the
	 * cache size is set.
	 */
	if(htab_bucketsp) {
		if(dbenv->mp_tablesize != 0)
			*htab_bucketsp = __db_tablesize(dbenv->mp_tablesize);
		else {
			if((pgsize = dbenv->mp_pagesize) == 0)
				pgsize = MPOOL_DEFAULT_PAGESIZE;
			*htab_bucketsp = __db_tablesize((uint32)(reg_size/(2.5*pgsize)));
		}
	}
}
/*
 * __memp_region_mutex_count --
 *	Return the number of mutexes the mpool region will need.
 *
 * PUBLIC: uint32 __memp_region_mutex_count(ENV *);
 */
uint32 __memp_region_mutex_count(ENV * env)
{
	uint32 htab_buckets;
	roff_t reg_size;
	uint32 max_region, num_per_cache, pgsize;
	DB_ENV * dbenv = env->dbenv;
	__memp_region_size(env, &reg_size, &htab_buckets);
	if(F_ISSET(env->dbenv, DB_ENV_MULTIVERSION))
		pgsize = sizeof(BH_FROZEN_ALLOC)+sizeof(BH_FROZEN_PAGE);
	if((pgsize = dbenv->mp_pagesize) == 0)
		pgsize = MPOOL_DEFAULT_PAGESIZE;
	max_region = __memp_max_regions(env);
	/*
	 * We need a couple of mutexes for the region itself, one for each
	 * file handle (MPOOLFILE) the application allocates, one for each
	 * of the MPOOL_FILE_BUCKETS, and each cache has one mutex per
	 * hash bucket. We then need one mutex per page in the cache,
	 * the worst case is really big if the pages are 512 bytes.
	 */
	if(dbenv->mp_mtxcount != 0)
		htab_buckets = dbenv->mp_mtxcount;
	else
		dbenv->mp_mtxcount = htab_buckets;
	num_per_cache = htab_buckets+(uint32)(reg_size/pgsize);
	return (max_region*num_per_cache)+50+MPOOL_FILE_BUCKETS;
}
/*
 * __memp_init_config --
 *	Initialize shared configuration information.
 */
static int __memp_init_config(ENV * env, MPOOL * mp)
{
	DB_ENV * dbenv = env->dbenv;
	MPOOL_SYSTEM_LOCK(env);
	if(dbenv->mp_mmapsize != 0)
		mp->mp_mmapsize = (db_size_t)dbenv->mp_mmapsize;
	if(dbenv->mp_maxopenfd != 0)
		mp->mp_maxopenfd = dbenv->mp_maxopenfd;
	if(dbenv->mp_maxwrite != 0)
		mp->mp_maxwrite = dbenv->mp_maxwrite;
	if(dbenv->mp_maxwrite_sleep != 0)
		mp->mp_maxwrite_sleep = dbenv->mp_maxwrite_sleep;
	MPOOL_SYSTEM_UNLOCK(env);
	return 0;
}
/*
 * __memp_env_refresh --
 *	Clean up after the mpool system on a close or failed open.
 */
int __memp_env_refresh(ENV * env)
{
	BH * bhp;
	BH_FROZEN_ALLOC * frozen_alloc;
	DB_MPOOLFILE * dbmfp;
	DB_MPREG * mpreg;
	MPOOL * c_mp;
	REGINFO * infop;
	uint32 bucket, i;
	int t_ret;
	int ret = 0;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	const uint32 nreg = mp->nreg;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(&dbmp->reginfo[0], mp->htab);
	//
	// If a private region, return the memory to the heap.  Not needed for
	// filesystem-backed or system shared memory regions, that memory isn't
	// owned by any particular process.
	//
	if(F_ISSET(env, ENV_PRIVATE)) {
		// Discard buffers
		for(i = 0; i < nreg; ++i) {
			infop = &dbmp->reginfo[i];
			c_mp = (MPOOL *)infop->primary;
			for(hp = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab), bucket = 0; bucket < c_mp->htab_buckets; ++hp, ++bucket) {
				while((bhp = SH_TAILQ_FIRST(&hp->hash_bucket, __bh)) != NULL)
					if(F_ISSET(bhp, BH_FROZEN))
						SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
					else {
						if(F_ISSET(bhp, BH_DIRTY)) {
							atomic_dec(env, &hp->hash_page_dirty);
							F_CLR(bhp, BH_DIRTY|BH_DIRTY_CREATE);
						}
						atomic_inc(env, &bhp->ref);
						if((t_ret = __memp_bhfree(dbmp, infop, (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset), hp, bhp, BH_FREE_FREEMEM|BH_FREE_UNLOCKED)) != 0 && ret == 0)
							ret = t_ret;
					}
			}
			MPOOL_REGION_LOCK(env, infop);
			while((frozen_alloc = SH_TAILQ_FIRST(&c_mp->alloc_frozen, __bh_frozen_a)) != NULL) {
				SH_TAILQ_REMOVE(&c_mp->alloc_frozen, frozen_alloc, links, __bh_frozen_a);
				__env_alloc_free(infop, frozen_alloc);
			}
			MPOOL_REGION_UNLOCK(env, infop);
		}
	}
	// Discard DB_MPOOLFILEs
	while((dbmfp = TAILQ_FIRST(&dbmp->dbmfq)) != NULL)
		if((t_ret = __memp_fclose(dbmfp, 0)) != 0 && ret == 0)
			ret = t_ret;
	// Discard DB_MPREGs
	__os_free(env, dbmp->pg_inout);
	while((mpreg = LIST_FIRST(&dbmp->dbregq)) != NULL) {
		LIST_REMOVE(mpreg, q);
		__os_free(env, mpreg);
	}
	// Discard the DB_MPOOL thread mutex
	if((t_ret = __mutex_free(env, &dbmp->mutex)) != 0 && ret == 0)
		ret = t_ret;
	if(F_ISSET(env, ENV_PRIVATE)) {
		// Discard REGION IDs
		infop = &dbmp->reginfo[0];
		infop->mtx_alloc = MUTEX_INVALID;
		__memp_free(infop, R_ADDR(infop, mp->regids));
		// Discard all the MPOOLFILEs
		if((t_ret = __memp_discard_all_mpfs(env, mp)) != 0 && ret == 0)
			ret = t_ret;
		// Discard the File table
		__memp_free(infop, R_ADDR(infop, mp->ftab));
		// Discard Hash tables
		for(i = 0; i < nreg; ++i) {
			infop = &dbmp->reginfo[i];
			c_mp = (MPOOL *)infop->primary;
			infop->mtx_alloc = MUTEX_INVALID;
			__memp_free(infop, R_ADDR(infop, c_mp->htab));
		}
	}
	// Detach from the region
	for(i = 0; i < nreg; ++i) {
		infop = &dbmp->reginfo[i];
		if((t_ret = __env_region_detach(env, infop, 0)) != 0 && ret == 0)
			ret = t_ret;
	}
	// Discard DB_MPOOL
	__os_free(env, dbmp->reginfo);
	__os_free(env, dbmp);
	env->mp_handle = NULL;
	return ret;
}
//
// memp_register_pp --
// ENV->memp_register pre/post processing.
//
int __memp_register_pp(DB_ENV * dbenv, int ftype, int (*pgin)(DB_ENV*, db_pgno_t, void *, DBT *), int (*pgout)(DB_ENV*, db_pgno_t, void *, DBT *))
{
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "DB_ENV->memp_register", DB_INIT_MPOOL);
	if(REP_ON(env)) {
		__db_errx(env, DB_STR_A("3001", "%smethod not permitted when replication is configured", "%s"), "DB_ENV->memp_register: ");
		ret = EINVAL;
	}
	else {
		DB_THREAD_INFO * ip;
		ENV_ENTER(env, ip);
		ret = __memp_register(env, ftype, pgin, pgout);
		ENV_LEAVE(env, ip);
	}
	return ret;
}
/*
 * memp_register --
 *	ENV->memp_register.
 */
int __memp_register(ENV * env, int ftype, int (*pgin)(DB_ENV*, db_pgno_t, void *, DBT *), int (*pgout)(DB_ENV*, db_pgno_t, void *, DBT *))
{
	DB_MPREG * mpreg;
	int ret;
	DB_MPOOL * dbmp = env->mp_handle;
	/*
	 * We keep the DB pgin/pgout functions outside of the linked list
	 * to avoid locking/unlocking the linked list on every page I/O.
	 *
	 * The Berkeley DB I/O conversion functions are registered when the
	 * environment is first created, so there's no need for locking here.
	 */
	if(ftype == DB_FTYPE_SET) {
		if(dbmp->pg_inout)
			return 0;
		if((ret = __os_malloc(env, sizeof(DB_MPREG), &dbmp->pg_inout)) != 0)
			return ret;
		dbmp->pg_inout->ftype = ftype;
		dbmp->pg_inout->pgin = pgin;
		dbmp->pg_inout->pgout = pgout;
		return 0;
	}
	/*
	 * The item may already have been registered.  If already registered,
	 * just update the entry, although it's probably unchanged.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	LIST_FOREACH(mpreg, &dbmp->dbregq, q)
	if(mpreg->ftype == ftype) {
		mpreg->pgin = pgin;
		mpreg->pgout = pgout;
		break;
	}
	if(mpreg == NULL) {                     /* New entry. */
		if((ret = __os_malloc(env, sizeof(DB_MPREG), &mpreg)) != 0)
			return ret;
		mpreg->ftype = ftype;
		mpreg->pgin = pgin;
		mpreg->pgout = pgout;
		LIST_INSERT_HEAD(&dbmp->dbregq, mpreg, q);
	}
	MUTEX_UNLOCK(env, dbmp->mutex);
	return 0;
}

int __memp_get_bucket(ENV * env, MPOOLFILE * mfp, db_pgno_t pgno, REGINFO ** infopp, DB_MPOOL_HASH ** hpp, uint32 * bucketp)
{
	DB_MPOOL_HASH * hp;
	MPOOL * c_mp;
	REGINFO * infop;
	uint32 bucket, new_bucket, new_nbuckets, region;
	uint32 * regids;
	DB_MPOOL * dbmp = env->mp_handle;
	roff_t mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	int    ret = 0;
	for(;; ) {
		const uint32 nbuckets = mp->nbuckets;
		MP_BUCKET(mf_offset, pgno, nbuckets, bucket);
		/*
		 * Once we work out which region we are looking in, we have to
		 * check that we have that region mapped, and that the version
		 * we have matches the ID in the main mpool region.  Otherwise
		 * we have to go and map in any regions that don't match and retry.
		 */
		region = NREGION(mp, bucket);
		regids = (uint32 *)R_ADDR(dbmp->reginfo, mp->regids);
		for(;; ) {
			infop = *infopp = &dbmp->reginfo[region];
			c_mp = (MPOOL *)infop->primary;
			/* If we have the correct region mapped, we're done. */
			if(c_mp && regids[region] == infop->id)
				break;
			if((ret = __memp_map_regions(dbmp)) != 0)
				return ret;
		}
		// If our caller wants the hash bucket, lock it here.
		if(hpp) {
			hp = (DB_MPOOL_HASH *)R_ADDR(infop, c_mp->htab);
			hp = &hp[bucket-region*mp->htab_buckets];
			MUTEX_READLOCK(env, hp->mtx_hash);
			/*
			 * Check that we still have the correct region mapped.
			 */
			if(regids[region] != infop->id) {
				MUTEX_UNLOCK(env, hp->mtx_hash);
				continue;
			}
			/*
			 * Now that the bucket is locked, we need to check that
			 * the cache has not been resized while we waited.
			 */
			new_nbuckets = mp->nbuckets;
			if(nbuckets != new_nbuckets) {
				MP_BUCKET(mf_offset, pgno, new_nbuckets, new_bucket);
				if(new_bucket != bucket) {
					MUTEX_UNLOCK(env, hp->mtx_hash);
					continue;
				}
			}
			*hpp = hp;
		}
		break;
	}
	ASSIGN_PTR(bucketp, bucket-region*mp->htab_buckets);
	return ret;
}

static int __memp_merge_buckets(DB_MPOOL * dbmp, uint32 new_nbuckets, uint32 old_bucket, uint32 new_bucket)
{
	BH * alloc_bhp, * bhp, * current_bhp, * next_bhp;
	DB_LSN vlsn;
	DB_MPOOL_HASH * new_hp, * old_hp;
	MPOOL * new_mp, * old_mp;
	MPOOLFILE * mfp;
	REGINFO * new_infop, * old_infop;
	uint32 bucket, high_mask, new_region, old_region;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	BH * new_bhp = NULL;
	int  ret = 0;
	MP_MASK(new_nbuckets, high_mask);
	old_region = NREGION(mp, old_bucket);
	old_infop = &dbmp->reginfo[old_region];
	old_mp = (MPOOL *)old_infop->primary;
	old_hp = (DB_MPOOL_HASH *)R_ADDR(old_infop, old_mp->htab);
	old_hp = &old_hp[old_bucket-old_region*mp->htab_buckets];

	new_region = NREGION(mp, new_bucket);
	new_infop = &dbmp->reginfo[new_region];
	new_mp = (MPOOL *)new_infop->primary;
	new_hp = (DB_MPOOL_HASH *)R_ADDR(new_infop, new_mp->htab);
	new_hp = &new_hp[new_bucket-new_region*mp->htab_buckets];

	/*
	 * Before merging, we need to check that there are no old buffers left
	 * in the target hash bucket after a previous split.
	 */
free_old:
	MUTEX_LOCK(env, new_hp->mtx_hash);
	SH_TAILQ_FOREACH(bhp, &new_hp->hash_bucket, hq, __bh) {
		MP_BUCKET(bhp->mf_offset, bhp->pgno, mp->nbuckets, bucket);
		if(bucket != new_bucket) {
			/*
			 * There is no way that an old buffer can be locked
			 * after a split, since everyone will look for it in
			 * the new hash bucket.
			 */
			DB_ASSERT(env, !F_ISSET(bhp, BH_DIRTY) && atomic_read(&bhp->ref) == 0);
			atomic_inc(env, &bhp->ref);
			mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
			if((ret = __memp_bhfree(dbmp, new_infop, mfp, new_hp, bhp, BH_FREE_FREEMEM)) != 0) {
				MUTEX_UNLOCK(env, new_hp->mtx_hash);
				return ret;
			}
			/*
			 * The free has modified the list of buffers and
			 * dropped the mutex.  We need to start again.
			 */
			goto free_old;
		}
	}
	MUTEX_UNLOCK(env, new_hp->mtx_hash);
	/*
	 * Before we begin, make sure that all of the buffers we care about are
	 * not in use and not frozen.  We do this because we can't drop the old
	 * hash bucket mutex once we start moving buffers around.
	 */
retry:
	MUTEX_LOCK(env, old_hp->mtx_hash);
	SH_TAILQ_FOREACH(bhp, &old_hp->hash_bucket, hq, __bh) {
		MP_HASH_BUCKET(MP_HASH(bhp->mf_offset, bhp->pgno), new_nbuckets, high_mask, bucket);
		if(bucket == new_bucket && atomic_read(&bhp->ref) != 0) {
			MUTEX_UNLOCK(env, old_hp->mtx_hash);
			__os_yield(env, 0, 0);
			goto retry;
		}
		else if(bucket == new_bucket && F_ISSET(bhp, BH_FROZEN)) {
			atomic_inc(env, &bhp->ref);
			/*
			 * We need to drop the hash bucket mutex to avoid
			 * self-blocking when we allocate a new buffer.
			 */
			MUTEX_UNLOCK(env, old_hp->mtx_hash);
			MUTEX_LOCK(env, bhp->mtx_buf);
			F_SET(bhp, BH_EXCLUSIVE);
			if(BH_OBSOLETE(bhp, old_hp->old_reader, vlsn))
				alloc_bhp = NULL;
			else {
				mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
				if((ret = __memp_alloc(dbmp, old_infop, mfp, 0, NULL, &alloc_bhp)) != 0)
					goto err;
			}
			/*
			 * But we need to lock the hash bucket again before
			 * thawing the buffer.  The call to __memp_bh_thaw
			 * will unlock the hash bucket mutex.
			 */
			MUTEX_LOCK(env, old_hp->mtx_hash);
			if(F_ISSET(bhp, BH_THAWED)) {
				ret = __memp_bhfree(dbmp, old_infop, NULL, NULL, alloc_bhp, BH_FREE_FREEMEM|BH_FREE_UNLOCKED);
			}
			else
				ret = __memp_bh_thaw(dbmp, old_infop, old_hp, bhp, alloc_bhp);
			/*
			 * We've dropped the mutex in order to thaw, so we need
			 * to go back to the beginning and check that all of
			 * the buffers we care about are still unlocked and
			 * unreferenced.
			 */
err:
			atomic_dec(env, &bhp->ref);
			F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			if(ret != 0)
				return ret;
			goto retry;
		}
	}
	/*
	 * We now know that all of the buffers we care about are unlocked and
	 * unreferenced.  Go ahead and copy them.
	 */
	SH_TAILQ_FOREACH(bhp, &old_hp->hash_bucket, hq, __bh) {
		MP_HASH_BUCKET(MP_HASH(bhp->mf_offset, bhp->pgno), new_nbuckets, high_mask, bucket);
		mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
		/*
		 * We ignore buffers that don't hash to the new bucket.  We
		 * could also ignore clean buffers which are not part of a
		 * multiversion chain as long as they have a backing file.
		 */
		if(bucket != new_bucket || (!F_ISSET(bhp, BH_DIRTY) && SH_CHAIN_SINGLETON(bhp, vc) && !mfp->no_backing_file))
			continue;
		for(current_bhp = bhp, next_bhp = NULL; current_bhp; current_bhp = SH_CHAIN_PREV(current_bhp, vc, __bh), next_bhp = alloc_bhp) {
			// Allocate in the new region
			if((ret = __memp_alloc(dbmp, new_infop, mfp, 0, NULL, &alloc_bhp)) != 0)
				break;
			alloc_bhp->ref = current_bhp->ref;
			alloc_bhp->priority = current_bhp->priority;
			alloc_bhp->pgno = current_bhp->pgno;
			alloc_bhp->mf_offset = current_bhp->mf_offset;
			alloc_bhp->flags = current_bhp->flags;
			alloc_bhp->td_off = current_bhp->td_off;

			/*
			 * We've duplicated the buffer, so now we need to
			 * update reference counts, including the counts in the
			 * per-MPOOLFILE and the transaction detail (for MVCC
			 * buffers).
			 */
			MUTEX_LOCK(env, mfp->mutex);
			++mfp->block_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
			if(alloc_bhp->td_off != INVALID_ROFF && (ret = __txn_add_buffer(env, (TXN_DETAIL *)R_ADDR(&env->tx_handle->reginfo, alloc_bhp->td_off))) != 0)
				break;
			memcpy(alloc_bhp->buf, bhp->buf, mfp->pagesize);
			/*
			 * We build up the MVCC chain first, then insert the
			 * head (stored in new_bhp) once.
			 */
			if(next_bhp == NULL) {
				SH_CHAIN_INIT(alloc_bhp, vc);
				new_bhp = alloc_bhp;
			}
			else
				SH_CHAIN_INSERT_BEFORE(next_bhp, alloc_bhp, vc, __bh);
		}
		DB_ASSERT(env, new_hp->mtx_hash != old_hp->mtx_hash);
		MUTEX_LOCK(env, new_hp->mtx_hash);
		SH_TAILQ_INSERT_TAIL(&new_hp->hash_bucket, new_bhp, hq);
		if(F_ISSET(new_bhp, BH_DIRTY))
			atomic_inc(env, &new_hp->hash_page_dirty);
		if(F_ISSET(bhp, BH_DIRTY)) {
			F_CLR(bhp, BH_DIRTY);
			atomic_dec(env, &old_hp->hash_page_dirty);
		}
		MUTEX_UNLOCK(env, new_hp->mtx_hash);
	}
	if(!ret)
		mp->nbuckets = new_nbuckets;
	MUTEX_UNLOCK(env, old_hp->mtx_hash);
	return ret;
}

static int FASTCALL __memp_add_bucket(DB_MPOOL * dbmp)
{
	uint32 high_mask, old_bucket;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	uint32 new_bucket = mp->nbuckets;
	/* We should always be adding buckets to the last region. */
	DB_ASSERT(env, NREGION(mp, new_bucket) == mp->nreg-1);
	MP_MASK(mp->nbuckets, high_mask);
	old_bucket = new_bucket&(high_mask>>1);

	/*
	 * With fixed-sized regions, the new region is always smaller than the
	 * existing total cache size, so buffers always need to be copied.  If
	 * we implement variable region sizes, it's possible that we will be
	 * splitting a hash bucket in the new region.  Catch that here.
	 */
	DB_ASSERT(env, NREGION(mp, old_bucket) != NREGION(mp, new_bucket));
	return __memp_merge_buckets(dbmp, mp->nbuckets+1, old_bucket, new_bucket);
}

static int FASTCALL __memp_add_region(DB_MPOOL * dbmp)
{
	int ret = 0;
	uint i;
	uint32 * regids;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	roff_t cache_size = (roff_t)mp->gbytes*GIGABYTE+mp->bytes;
	/* All cache regions are the same size. */
	roff_t reg_size = dbmp->reginfo[0].rp->size;
	REGINFO * infop = &dbmp->reginfo[mp->nreg];
	infop->env = env;
	infop->type = REGION_TYPE_MPOOL;
	infop->id = INVALID_REGION_ID;
	infop->flags = REGION_CREATE_OK;
	if((ret = __env_region_attach(env, infop, reg_size, reg_size)) != 0)
		return ret;
	if((ret = __memp_init(env, dbmp, mp->nreg, mp->htab_buckets, mp->max_nreg)) != 0)
		return ret;
	cache_size += reg_size;
	mp->gbytes = (uint32)(cache_size/GIGABYTE);
	mp->bytes = (uint32)(cache_size%GIGABYTE);
	regids = (uint32 *)R_ADDR(dbmp->reginfo, mp->regids);
	regids[mp->nreg++] = infop->id;
	for(i = 0; i < mp->htab_buckets; i++)
		if((ret = __memp_add_bucket(dbmp)) != 0)
			break;
	return ret;
}

static int FASTCALL __memp_remove_bucket(DB_MPOOL * dbmp)
{
	uint32 high_mask, new_bucket;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	uint32 old_bucket = mp->nbuckets-1;
	// We should always be removing buckets from the last region.
	DB_ASSERT(env, NREGION(mp, old_bucket) == mp->nreg-1);
	MP_MASK(mp->nbuckets-1, high_mask);
	new_bucket = old_bucket&(high_mask>>1);
	return __memp_merge_buckets(dbmp, mp->nbuckets-1, old_bucket, new_bucket);
}

static int FASTCALL __memp_remove_region(DB_MPOOL * dbmp)
{
	DB_MPOOL_HASH * hp;
	REGINFO * infop;
	uint i;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	roff_t reg_size = dbmp->reginfo[0].rp->size;
	roff_t cache_size = (roff_t)mp->gbytes*GIGABYTE+mp->bytes;
	int ret = 0;
	if(mp->nreg == 1) {
		__db_errx(env, DB_STR("3019", "cannot remove the last cache"));
		return EINVAL;
	}
	for(i = 0; i < mp->htab_buckets; i++)
		if((ret = __memp_remove_bucket(dbmp)) != 0)
			return ret;
	/* Detach from the region then destroy it. */
	infop = &dbmp->reginfo[mp->nreg];
	if(F_ISSET(env, ENV_PRIVATE)) {
		hp = (DB_MPOOL_HASH *)R_ADDR(infop, ((MPOOL *)infop->primary)->htab);
		for(i = 0; i < env->dbenv->mp_mtxcount; i++)
			if((ret = __mutex_free(env, &hp[i].mtx_hash)) != 0)
				return ret;
	}
	ret = __env_region_detach(env, infop, 1);
	if(!ret) {
		mp->nreg--;
		cache_size -= reg_size;
		mp->gbytes = (uint32)(cache_size/GIGABYTE);
		mp->bytes = (uint32)(cache_size%GIGABYTE);
	}
	return ret;
}

static int FASTCALL __memp_map_regions(DB_MPOOL * dbmp)
{
	uint i;
	ENV * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	uint32 * regids = (uint32 *)R_ADDR(dbmp->reginfo, mp->regids);
	int ret = 0;
	for(i = 1; i < mp->nreg; ++i) {
		if(dbmp->reginfo[i].primary && dbmp->reginfo[i].id == regids[i])
			continue;
		if(dbmp->reginfo[i].primary)
			ret = __env_region_detach(env, &dbmp->reginfo[i], 0);
		dbmp->reginfo[i].env = env;
		dbmp->reginfo[i].type = REGION_TYPE_MPOOL;
		dbmp->reginfo[i].id = regids[i];
		dbmp->reginfo[i].flags = REGION_JOIN_OK;
		if((ret = __env_region_attach(env, &dbmp->reginfo[i], 0, 0)) != 0)
			return ret;
		dbmp->reginfo[i].primary = R_ADDR(&dbmp->reginfo[i], dbmp->reginfo[i].rp->primary);
	}
	for(; i < mp->max_nreg; i++)
		if(dbmp->reginfo[i].primary && (ret = __env_region_detach(env, &dbmp->reginfo[i], 0)) != 0)
			break;
	return ret;
}

int __memp_resize(DB_MPOOL * dbmp, uint32 gbytes, uint32 bytes)
{
	int    ret = 0;
	ENV  * env = dbmp->env;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	roff_t reg_size = dbmp->reginfo[0].rp->size;
	roff_t total_size = (roff_t)gbytes*GIGABYTE+bytes;
	uint32 ncache = (uint32)((total_size+reg_size/2)/reg_size);
	if(ncache < 1)
		ncache = 1;
	else if(ncache > mp->max_nreg) {
		__db_errx(env, DB_STR_A("3020", "cannot resize to %lu cache regions: maximum is %lu", "%lu %lu"), (ulong)ncache, (ulong)mp->max_nreg);
		return EINVAL;
	}
	ret = 0;
	MUTEX_LOCK(env, mp->mtx_resize);
	while(mp->nreg != ncache)
		if((ret = (mp->nreg < ncache ? __memp_add_region(dbmp) : __memp_remove_region(dbmp))) != 0)
			break;
	MUTEX_UNLOCK(env, mp->mtx_resize);
	return ret;
}

int __memp_get_cache_max(DB_ENV * dbenv, uint32 * max_gbytesp, uint32 * max_bytesp)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	roff_t reg_size, max_size;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_ncache", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		reg_size = dbmp->reginfo[0].rp->size;
		max_size = mp->max_nreg*reg_size;
		*max_gbytesp = (uint32)(max_size/GIGABYTE);
		*max_bytesp = (uint32)(max_size%GIGABYTE);
	}
	else {
		*max_gbytesp = dbenv->mp_max_gbytes;
		*max_bytesp = dbenv->mp_max_bytes;
	}
	return 0;
}

int __memp_set_cache_max(DB_ENV * dbenv, uint32 max_gbytes, uint32 max_bytes)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_cache_max");
	dbenv->mp_max_gbytes = max_gbytes;
	dbenv->mp_max_bytes = max_bytes;
	return 0;
}

#ifdef HAVE_STATISTICS
/*
 * __memp_stat_pp --
 *	DB_ENV->memp_stat pre/post processing.
 */
int __memp_stat_pp(DB_ENV * dbenv, DB_MPOOL_STAT ** gspp, DB_MPOOL_FSTAT *** fspp, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "DB_ENV->memp_stat", DB_INIT_MPOOL);
	if((ret = __db_fchk(env, "DB_ENV->memp_stat", flags, DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_stat(env, gspp, fspp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_stat --
 *	ENV->memp_stat
 */
static int __memp_stat(ENV * env, DB_MPOOL_STAT ** gspp, DB_MPOOL_FSTAT *** fspp, uint32 flags)
{
	DB_MPOOL_FSTAT ** tfsp;
	DB_MPOOL_STAT * sp;
	MPOOL * c_mp;
	size_t len;
	int ret;
	uint32 i;
	uintmax_t tmp_wait, tmp_nowait;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	/* Global statistics. */
	if(gspp) {
		*gspp = NULL;
		if((ret = __os_umalloc(env, sizeof(**gspp), gspp)) != 0)
			return ret;
		memzero(*gspp, sizeof(**gspp));
		sp = *gspp;
		/*
		 * Initialization and information that is not maintained on
		 * a per-cache basis.  Note that configuration information
		 * may be modified at any time, and so we have to lock.
		 */
		sp->st_gbytes = mp->gbytes;
		sp->st_bytes = mp->bytes;
		sp->st_pagesize = mp->pagesize;
		sp->st_ncache = mp->nreg;
		sp->st_max_ncache = mp->max_nreg;
		sp->st_regsize = dbmp->reginfo[0].rp->size;
		sp->st_regmax = dbmp->reginfo[0].rp->max;
		sp->st_sync_interrupted = mp->stat.st_sync_interrupted;
		MPOOL_SYSTEM_LOCK(env);
		sp->st_mmapsize = mp->mp_mmapsize;
		sp->st_maxopenfd = mp->mp_maxopenfd;
		sp->st_maxwrite = mp->mp_maxwrite;
		sp->st_maxwrite_sleep = mp->mp_maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);
		/* Walk the cache list and accumulate the global information. */
		for(i = 0; i < mp->nreg; ++i) {
			c_mp = (MPOOL *)dbmp->reginfo[i].primary;
			sp->st_map += c_mp->stat.st_map;
			sp->st_cache_hit += c_mp->stat.st_cache_hit;
			sp->st_cache_miss += c_mp->stat.st_cache_miss;
			sp->st_page_create += c_mp->stat.st_page_create;
			sp->st_page_in += c_mp->stat.st_page_in;
			sp->st_page_out += c_mp->stat.st_page_out;
			sp->st_ro_evict += c_mp->stat.st_ro_evict;
			sp->st_rw_evict += c_mp->stat.st_rw_evict;
			sp->st_page_trickle += c_mp->stat.st_page_trickle;
			sp->st_pages += c_mp->pages;
			/*
			 * st_page_dirty	calculated by __memp_stat_hash st_page_clean	calculated here
			 */
			__memp_stat_hash(&dbmp->reginfo[i], c_mp, &sp->st_page_dirty);
			sp->st_page_clean = sp->st_pages-sp->st_page_dirty;
			sp->st_hash_buckets += c_mp->htab_buckets;
			sp->st_hash_mutexes += c_mp->htab_mutexes;
			sp->st_hash_searches += c_mp->stat.st_hash_searches;
			sp->st_hash_longest += c_mp->stat.st_hash_longest;
			sp->st_hash_examined += c_mp->stat.st_hash_examined;
			/*
			 * st_hash_nowait	calculated by __memp_stat_wait st_hash_wait
			 */
			__memp_stat_wait(env, &dbmp->reginfo[i], c_mp, sp, flags);
			__mutex_set_wait_info(env, c_mp->mtx_region, &tmp_wait, &tmp_nowait);
			sp->st_region_nowait += tmp_nowait;
			sp->st_region_wait += tmp_wait;
			sp->st_alloc += c_mp->stat.st_alloc;
			sp->st_alloc_buckets += c_mp->stat.st_alloc_buckets;
			if(sp->st_alloc_max_buckets < c_mp->stat.st_alloc_max_buckets)
				sp->st_alloc_max_buckets = c_mp->stat.st_alloc_max_buckets;
			sp->st_alloc_pages += c_mp->stat.st_alloc_pages;
			if(sp->st_alloc_max_pages < c_mp->stat.st_alloc_max_pages)
				sp->st_alloc_max_pages = c_mp->stat.st_alloc_max_pages;
			if(LF_ISSET(DB_STAT_CLEAR)) {
				if(!LF_ISSET(DB_STAT_SUBSYSTEM))
					__mutex_clear(env, c_mp->mtx_region);
				memzero(&c_mp->stat, sizeof(c_mp->stat));
			}
		}
		/*
		 * We have duplicate statistics fields in per-file structures
		 * and the cache.  The counters are only incremented in the
		 * per-file structures, except if a file is flushed from the
		 * mpool, at which time we copy its information into the cache
		 * statistics.  We added the cache information above, now we
		 * add the per-file information.
		 */
		if((ret = __memp_walk_files(env, mp, __memp_file_stats, sp, NULL, fspp == NULL ? LF_ISSET(DB_STAT_CLEAR) : 0)) != 0)
			return ret;
	}
	/* Per-file statistics. */
	if(fspp) {
		*fspp = NULL;
		/* Count the MPOOLFILE structures. */
		i = 0;
		len = 0;
		if((ret = __memp_walk_files(env, mp, __memp_count_files, &len, &i, flags)) != 0)
			return ret;
		if(i == 0)
			return 0;
		len += sizeof(DB_MPOOL_FSTAT *); /* Trailing NULL */
		/* Allocate space */
		if((ret = __os_umalloc(env, len, fspp)) != 0)
			return ret;
		tfsp = *fspp;
		*tfsp = NULL;
		/*
		 * Files may have been opened since we counted, don't walk
		 * off the end of the allocated space.
		 */
		if((ret = __memp_walk_files(env, mp, __memp_get_files, &tfsp, &i, flags)) != 0)
			return ret;
		*++tfsp = NULL;
	}
	return 0;
}

static int __memp_file_stats(ENV * env, MPOOLFILE * mfp, void * argp, uint32 * countp, uint32 flags)
{
	DB_MPOOL_STAT * sp;
	COMPQUIET(env, 0);
	COMPQUIET(countp, 0);
	sp = (DB_MPOOL_STAT *)argp;
	sp->st_map += mfp->stat.st_map;
	sp->st_cache_hit += mfp->stat.st_cache_hit;
	sp->st_cache_miss += mfp->stat.st_cache_miss;
	sp->st_page_create += mfp->stat.st_page_create;
	sp->st_page_in += mfp->stat.st_page_in;
	sp->st_page_out += mfp->stat.st_page_out;
	if(LF_ISSET(DB_STAT_CLEAR))
		memzero(&mfp->stat, sizeof(mfp->stat));
	return 0;
}

static int __memp_count_files(ENV * env, MPOOLFILE * mfp, void * argp, uint32 * countp, uint32 flags)
{
	DB_MPOOL * dbmp;
	size_t len;
	COMPQUIET(flags, 0);
	dbmp = env->mp_handle;
	len = *(size_t *)argp;
	(*countp)++;
	len += sizeof(DB_MPOOL_FSTAT *)+sizeof(DB_MPOOL_FSTAT)+sstrlen(__memp_fns(dbmp, mfp))+1;
	*(size_t *)argp = len;
	return 0;
}
/*
 * __memp_get_files --
 *	get file specific statistics
 *
 * Build each individual entry.  We assume that an array of pointers are
 * aligned correctly to be followed by an array of structures, which should
 * be safe (in this particular case, the first element of the structure
 * is a pointer, so we're doubly safe).  The array is followed by space
 * for the text file names.
 */
static int __memp_get_files(ENV * env, MPOOLFILE * mfp, void * argp, uint32 * countp, uint32 flags)
{
	DB_MPOOL_FSTAT * tstruct;
	char * name, * tname;
	size_t nlen;
	if(*countp) {
		DB_MPOOL * dbmp = env->mp_handle;
		DB_MPOOL_FSTAT ** tfsp = *(DB_MPOOL_FSTAT ***)argp;
		if(*tfsp == NULL) {
			// Add 1 to count because we need to skip over the NULL
			tstruct = (DB_MPOOL_FSTAT *)(tfsp+*countp+1);
			tname = (char *)(tstruct+*countp);
			*tfsp = tstruct;
		}
		else {
			tstruct = *tfsp+1;
			tname = (*tfsp)->file_name+sstrlen((*tfsp)->file_name)+1;
			*++tfsp = tstruct;
		}
		name = __memp_fns(dbmp, mfp);
		nlen = sstrlen(name)+1;
		memcpy(tname, name, nlen);
		memcpy(tstruct, &mfp->stat, sizeof(mfp->stat));
		tstruct->file_name = tname;
		// Grab the pagesize from the mfp
		tstruct->st_pagesize = mfp->pagesize;
		*(DB_MPOOL_FSTAT ***)argp = tfsp;
		(*countp)--;
		if(LF_ISSET(DB_STAT_CLEAR))
			memzero(&mfp->stat, sizeof(mfp->stat));
	}
	return 0;
}
/*
 * __memp_stat_print_pp --
 *	ENV->memp_stat_print pre/post processing.
 *
 * PUBLIC: int __memp_stat_print_pp(DB_ENV *, uint32);
 */
int __memp_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "DB_ENV->memp_stat_print", DB_INIT_MPOOL);
 #define DB_STAT_MEMP_FLAGS                                              \
	(DB_STAT_ALL|DB_STAT_ALLOC|DB_STAT_CLEAR|DB_STAT_MEMP_HASH)
	if((ret = __db_fchk(env, "DB_ENV->memp_stat_print", flags, DB_STAT_MEMP_FLAGS)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}

 #define FMAP_ENTRIES    200                    /* Files we map. */
/*
 * __memp_stat_print --
 *	ENV->memp_stat_print method.
 *
 * PUBLIC: int  __memp_stat_print __P((ENV *, uint32));
 */
int __memp_stat_print(ENV * env, uint32 flags)
{
	int ret;
	uint32 orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM);
	if(flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __memp_print_stats(env, LF_ISSET(DB_STAT_ALL) ? flags : orig_flags);
		if(flags == 0 || ret != 0)
			return ret;
	}
	return (LF_ISSET(DB_STAT_ALL|DB_STAT_MEMP_HASH) && (ret = __memp_print_all(env, orig_flags)) != 0) ? ret : 0;
}
/*
 * __memp_print_stats --
 *	Display default mpool region statistics.
 */
static int __memp_print_stats(ENV * env, uint32 flags)
{
	DB_MPOOL_FSTAT ** fsp, ** tfsp;
	DB_MPOOL_STAT * gsp;
	int ret = __memp_stat(env, &gsp, &fsp, flags);
	if(ret != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default cache region information:");
	__db_dlbytes(env, "Total cache size", (ulong)gsp->st_gbytes, (ulong)0, (ulong)gsp->st_bytes);
	__db_dl(env, "Number of caches", (ulong)gsp->st_ncache);
	__db_dl(env, "Maximum number of caches", (ulong)gsp->st_max_ncache);
	__db_dlbytes(env, "Pool individual cache size", (ulong)0, (ulong)0, (ulong)gsp->st_regsize);
	__db_dlbytes(env, "Pool individual cache max", (ulong)0, (ulong)0, (ulong)gsp->st_regmax);
	__db_dlbytes(env, "Maximum memory-mapped file size", (ulong)0, (ulong)0, (ulong)gsp->st_mmapsize);
	STAT_LONG("Maximum open file descriptors", gsp->st_maxopenfd);
	STAT_LONG("Maximum sequential buffer writes", gsp->st_maxwrite);
	STAT_LONG("Sleep after writing maximum sequential buffers", gsp->st_maxwrite_sleep);
	__db_dl(env, "Requested pages mapped into the process' address space", (ulong)gsp->st_map);
	__db_dl_pct(env, "Requested pages found in the cache", (ulong)gsp->st_cache_hit, DB_PCT(gsp->st_cache_hit, gsp->st_cache_hit+gsp->st_cache_miss), 0);
	__db_dl(env, "Requested pages not found in the cache", (ulong)gsp->st_cache_miss);
	__db_dl(env, "Pages created in the cache", (ulong)gsp->st_page_create);
	__db_dl(env, "Pages read into the cache", (ulong)gsp->st_page_in);
	__db_dl(env, "Pages written from the cache to the backing file", (ulong)gsp->st_page_out);
	__db_dl(env, "Clean pages forced from the cache", (ulong)gsp->st_ro_evict);
	__db_dl(env, "Dirty pages forced from the cache", (ulong)gsp->st_rw_evict);
	__db_dl(env, "Dirty pages written by trickle-sync thread", (ulong)gsp->st_page_trickle);
	__db_dl(env, "Current total page count", (ulong)gsp->st_pages);
	__db_dl(env, "Current clean page count", (ulong)gsp->st_page_clean);
	__db_dl(env, "Current dirty page count", (ulong)gsp->st_page_dirty);
	__db_dl(env, "Number of hash buckets used for page location", (ulong)gsp->st_hash_buckets);
	__db_dl(env, "Number of mutexes for the hash buckets", (ulong)gsp->st_hash_mutexes);
	__db_dl(env, "Assumed page size used", (ulong)gsp->st_pagesize);
	__db_dl(env, "Total number of times hash chains searched for a page", (ulong)gsp->st_hash_searches);
	__db_dl(env, "The longest hash chain searched for a page", (ulong)gsp->st_hash_longest);
	__db_dl(env, "Total number of hash chain entries checked for page", (ulong)gsp->st_hash_examined);
	__db_dl_pct(env, "The number of hash bucket locks that required waiting", (ulong)gsp->st_hash_wait, DB_PCT(gsp->st_hash_wait, gsp->st_hash_wait+gsp->st_hash_nowait), 0);
	__db_dl_pct(env, "The maximum number of times any hash bucket lock was waited for", (ulong)gsp->st_hash_max_wait, DB_PCT(gsp->st_hash_max_wait, gsp->st_hash_max_wait+gsp->st_hash_max_nowait), 0);
	__db_dl_pct(env, "The number of region locks that required waiting", (ulong)gsp->st_region_wait, DB_PCT(gsp->st_region_wait, gsp->st_region_wait+gsp->st_region_nowait), 0);
	__db_dl(env, "The number of buffers frozen", (ulong)gsp->st_mvcc_frozen);
	__db_dl(env, "The number of buffers thawed", (ulong)gsp->st_mvcc_thawed);
	__db_dl(env, "The number of frozen buffers freed", (ulong)gsp->st_mvcc_freed);
	__db_dl(env, "The number of page allocations", (ulong)gsp->st_alloc);
	__db_dl(env, "The number of hash buckets examined during allocations", (ulong)gsp->st_alloc_buckets);
	__db_dl(env, "The maximum number of hash buckets examined for an allocation", (ulong)gsp->st_alloc_max_buckets);
	__db_dl(env, "The number of pages examined during allocations", (ulong)gsp->st_alloc_pages);
	__db_dl(env, "The max number of pages examined for an allocation", (ulong)gsp->st_alloc_max_pages);
	__db_dl(env, "Threads waited on page I/O", (ulong)gsp->st_io_wait);
	__db_dl(env, "The number of times a sync is interrupted", (ulong)gsp->st_sync_interrupted);

	for(tfsp = fsp; fsp && *tfsp; ++tfsp) {
		if(LF_ISSET(DB_STAT_ALL))
			__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Pool File: %s", (*tfsp)->file_name);
		__db_dl(env, "Page size", (ulong)(*tfsp)->st_pagesize);
		__db_dl(env, "Requested pages mapped into the process' address space", (ulong)(*tfsp)->st_map);
		__db_dl_pct(env, "Requested pages found in the cache", (ulong)(*tfsp)->st_cache_hit, DB_PCT((*tfsp)->st_cache_hit, (*tfsp)->st_cache_hit+(*tfsp)->st_cache_miss), 0);
		__db_dl(env, "Requested pages not found in the cache", (ulong)(*tfsp)->st_cache_miss);
		__db_dl(env, "Pages created in the cache", (ulong)(*tfsp)->st_page_create);
		__db_dl(env, "Pages read into the cache", (ulong)(*tfsp)->st_page_in);
		__db_dl(env, "Pages written from the cache to the backing file", (ulong)(*tfsp)->st_page_out);
	}
	__os_ufree(env, fsp);
	__os_ufree(env, gsp);
	return 0;
}
/*
 * __memp_print_all --
 *	Display debugging mpool region statistics.
 */
static int __memp_print_all(ENV * env, uint32 flags)
{
	static const FN cfn[] = {
		{ DB_MPOOL_NOFILE,      "DB_MPOOL_NOFILE" },
		{ DB_MPOOL_UNLINK,      "DB_MPOOL_UNLINK" },
		{ 0,                    NULL }
	};
	DB_MPOOLFILE * dbmfp;
	roff_t fmap[FMAP_ENTRIES+1];
	uint32 i, cnt;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	int ret = 0;
	MPOOL_SYSTEM_LOCK(env);
	__db_print_reginfo(env, dbmp->reginfo, "Mpool", flags);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "MPOOL structure:");
	__mutex_print_debug_single(env, "MPOOL region mutex", mp->mtx_region, flags);
	STAT_LSN("Maximum checkpoint LSN", &mp->lsn);
	STAT_ULONG("Hash table entries", mp->htab_buckets);
	STAT_ULONG("Hash table mutexes", mp->htab_mutexes);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_MPOOL handle information:");
	__mutex_print_debug_single(env, "DB_MPOOL handle mutex", dbmp->mutex, flags);
	STAT_ULONG("Underlying cache regions", mp->nreg);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_MPOOLFILE structures:");
	for(cnt = 0, dbmfp = TAILQ_FIRST(&dbmp->dbmfq); dbmfp; dbmfp = TAILQ_NEXT(dbmfp, q), ++cnt) {
		__db_msg(env, "File #%lu: %s: per-process, %s", (ulong)cnt+1, __memp_fn(dbmfp), F_ISSET(dbmfp, MP_READONLY) ? "readonly" : "read/write");
		STAT_ULONG("Reference count", dbmfp->ref);
		STAT_ULONG("Pinned block reference count", dbmfp->ref);
		STAT_ULONG("Clear length", dbmfp->clear_len);
		__db_print_fileid(env, dbmfp->fileid, "\tID");
		STAT_ULONG("File type", dbmfp->ftype);
		STAT_ULONG("LSN offset", dbmfp->lsn_offset);
		STAT_ULONG("Max gbytes", dbmfp->gbytes);
		STAT_ULONG("Max bytes", dbmfp->bytes);
		STAT_ULONG("Cache priority", dbmfp->priority);
		STAT_POINTER("mmap address", dbmfp->addr);
		STAT_ULONG("mmap length", dbmfp->len);
		__db_prflags(env, NULL, dbmfp->flags, cfn, NULL, "\tFlags");
		__db_print_fh(env, "File handle", dbmfp->fhp, flags);
	}
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "MPOOLFILE structures:");
	cnt = 0;
	ret = __memp_walk_files(env, mp, __memp_print_files, fmap, &cnt, flags);
	MPOOL_SYSTEM_UNLOCK(env);
	if(ret != 0)
		return ret;
	if(cnt < FMAP_ENTRIES)
		fmap[cnt] = INVALID_ROFF;
	else
		fmap[FMAP_ENTRIES] = INVALID_ROFF;
	/* Dump the individual caches. */
	for(i = 0; i < mp->nreg; ++i) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Cache #%d:", i+1);
		if(i > 0)
			__env_alloc_print(&dbmp->reginfo[i], flags);
		if((ret = __memp_print_hash(env, dbmp, &dbmp->reginfo[i], fmap, flags)) != 0)
			break;
	}
	return ret;
}

static int __memp_print_files(ENV * env, MPOOLFILE * mfp, void * argp, uint32 * countp, uint32 flags)
{
	roff_t * fmap;
	DB_MPOOL * dbmp;
	uint32 mfp_flags;
	static const FN fn[] = {
		{ MP_CAN_MMAP,          "MP_CAN_MMAP" },
		{ MP_DIRECT,            "MP_DIRECT" },
		{ MP_EXTENT,            "MP_EXTENT" },
		{ MP_FAKE_DEADFILE,     "deadfile" },
		{ MP_FAKE_FILEWRITTEN,  "file written" },
		{ MP_FAKE_NB,           "no backing file" },
		{ MP_FAKE_UOC,          "unlink on close" },
		{ MP_NOT_DURABLE,       "not durable" },
		{ MP_TEMP,              "MP_TEMP" },
		{ 0,                    NULL }
	};
	dbmp = env->mp_handle;
	fmap = (roff_t *)argp;
	__db_msg(env, "File #%d: %s", *countp+1, __memp_fns(dbmp, mfp));
	__mutex_print_debug_single(env, "Mutex", mfp->mutex, flags);
	MUTEX_LOCK(env, mfp->mutex);
	STAT_ULONG("Revision count", mfp->revision);
	STAT_ULONG("Reference count", mfp->mpf_cnt);
	STAT_ULONG("Block count", mfp->block_cnt);
	STAT_ULONG("Last page number", mfp->last_pgno);
	STAT_ULONG("Original last page number", mfp->orig_last_pgno);
	STAT_ULONG("Maximum page number", mfp->maxpgno);
	STAT_LONG("Type", mfp->ftype);
	STAT_LONG("Priority", mfp->priority);
	STAT_LONG("Page's LSN offset", mfp->lsn_off);
	STAT_LONG("Page's clear length", mfp->clear_len);
	__db_print_fileid(env, (uint8 *)R_ADDR(dbmp->reginfo, mfp->fileid_off), "\tID");
	mfp_flags = 0;
	if(mfp->deadfile)
		FLD_SET(mfp_flags, MP_FAKE_DEADFILE);
	if(mfp->file_written)
		FLD_SET(mfp_flags, MP_FAKE_FILEWRITTEN);
	if(mfp->no_backing_file)
		FLD_SET(mfp_flags, MP_FAKE_NB);
	if(mfp->unlink_on_close)
		FLD_SET(mfp_flags, MP_FAKE_UOC);
	__db_prflags(env, NULL, mfp_flags, fn, NULL, "\tFlags");
	if(*countp < FMAP_ENTRIES)
		fmap[*countp] = R_OFFSET(dbmp->reginfo, mfp);
	(*countp)++;
	MUTEX_UNLOCK(env, mfp->mutex);
	return 0;
}
/*
 * __memp_print_hash --
 *	Display hash bucket statistics for a cache.
 */
static int __memp_print_hash(ENV * env, DB_MPOOL * dbmp, REGINFO * reginfo, roff_t * fmap, uint32 flags)
{
	BH * bhp, * vbhp;
	DB_MPOOL_HASH * hp;
	DB_MSGBUF mb;
	uint32 bucket;
	MPOOL * c_mp = (MPOOL *)reginfo->primary;
	DB_MSGBUF_INIT(&mb);
	STAT_ULONG("Hash table last-checked", c_mp->last_checked);
	STAT_ULONG("Hash table LRU priority", c_mp->lru_priority);
	STAT_ULONG("Hash table LRU generation", c_mp->lru_generation);
	STAT_ULONG("Put counter", c_mp->put_counter);

	/* Display the hash table list of BH's. */
	__db_msg(env, "BH hash table (%lu hash slots)", (ulong)c_mp->htab_buckets);
	__db_msg(env, "bucket #: priority, I/O wait, [mutex]");
	__db_msg(env, "\tpageno, file, ref, LSN, address, priority, flags");

	for(hp = (DB_MPOOL_HASH *)R_ADDR(reginfo, c_mp->htab), bucket = 0; bucket < c_mp->htab_buckets; ++hp, ++bucket) {
		MUTEX_READLOCK(env, hp->mtx_hash);
		if((bhp = SH_TAILQ_FIRST(&hp->hash_bucket, __bh)) != NULL) {
			__db_msgadd(env, &mb, "bucket %lu: %lu (%lu dirty)", (ulong)bucket, (ulong)hp->hash_io_wait, (ulong)atomic_read(&hp->hash_page_dirty));
			if(hp->hash_frozen != 0)
				__db_msgadd(env, &mb, "(MVCC %lu/%lu/%lu) ", (ulong)hp->hash_frozen, (ulong)hp->hash_thawed, (ulong)hp->hash_frozen_freed);
			__mutex_print_debug_stats(env, &mb, hp->mtx_hash, flags);
			DB_MSGBUF_FLUSH(env, &mb);
		}
		for(; bhp; bhp = SH_TAILQ_NEXT(bhp, hq, __bh)) {
			__memp_print_bh(env, dbmp, NULL, bhp, fmap);
			/* Print the version chain, if it exists. */
			for(vbhp = SH_CHAIN_PREV(bhp, vc, __bh); vbhp; vbhp = SH_CHAIN_PREV(vbhp, vc, __bh)) {
				__memp_print_bh(env, dbmp, " next:\t", vbhp, fmap);
			}
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	return 0;
}
/*
 * __memp_print_bh --
 *	Display a BH structure.
 */
static void __memp_print_bh(ENV * env, DB_MPOOL * dbmp, const char * prefix, BH * bhp, roff_t * fmap)
{
	static const FN fn[] = {
		{ BH_CALLPGIN,          "callpgin" },
		{ BH_DIRTY,             "dirty" },
		{ BH_DIRTY_CREATE,      "created" },
		{ BH_DISCARD,           "discard" },
		{ BH_EXCLUSIVE,         "exclusive" },
		{ BH_FREED,             "freed" },
		{ BH_FROZEN,            "frozen" },
		{ BH_TRASH,             "trash" },
		{ BH_THAWED,            "thawed" },
		{ 0,                    NULL }
	};
	DB_MSGBUF mb;
	int i;
	DB_MSGBUF_INIT(&mb);
	if(prefix)
		__db_msgadd(env, &mb, "%s", prefix);
	else
		__db_msgadd(env, &mb, "\t");
	for(i = 0; i < FMAP_ENTRIES; ++i)
		if(fmap[i] == INVALID_ROFF || fmap[i] == bhp->mf_offset)
			break;
	if(fmap[i] == INVALID_ROFF)
		__db_msgadd(env, &mb, "%5lu, %lu, ", (ulong)bhp->pgno, (ulong)bhp->mf_offset);
	else
		__db_msgadd(env, &mb, "%5lu, #%d, ", (ulong)bhp->pgno, i+1);
	__db_msgadd(env, &mb, "%2lu, %lu/%lu", (ulong)atomic_read(&bhp->ref),
		F_ISSET(bhp, BH_FROZEN) ? 0 : (ulong)LSN(bhp->buf).file, F_ISSET(bhp, BH_FROZEN) ? 0 : (ulong)LSN(bhp->buf).Offset_);
	if(bhp->td_off != INVALID_ROFF)
		__db_msgadd(env, &mb, " (@%lu/%lu 0x%x)", (ulong)VISIBLE_LSN(env, bhp)->file, (ulong)VISIBLE_LSN(env, bhp)->Offset_, BH_OWNER(env, bhp)->txnid);
	__db_msgadd(env, &mb, ", %#08lx, %lu", (ulong)R_OFFSET(dbmp->reginfo, bhp), (ulong)bhp->priority);
	__db_prflags(env, &mb, bhp->flags, fn, " (", ")");
	DB_MSGBUF_FLUSH(env, &mb);
}
/*
 * __memp_stat_wait --
 *	Total hash bucket wait stats into the region.
 */
static void __memp_stat_wait(ENV * env, REGINFO * reginfo, MPOOL * mp, DB_MPOOL_STAT * mstat, uint32 flags)
{
	DB_MPOOL_HASH * hp;
	uint32 i;
	uintmax_t tmp_nowait, tmp_wait;
	mstat->st_hash_max_wait = 0;
	hp = (DB_MPOOL_HASH *)R_ADDR(reginfo, mp->htab);
	for(i = 0; i < mp->htab_buckets; i++, hp++) {
		__mutex_set_wait_info(env, hp->mtx_hash, &tmp_wait, &tmp_nowait);
		mstat->st_hash_nowait += tmp_nowait;
		mstat->st_hash_wait += tmp_wait;
		if(tmp_wait > mstat->st_hash_max_wait) {
			mstat->st_hash_max_wait = tmp_wait;
			mstat->st_hash_max_nowait = tmp_nowait;
		}
		if(LF_ISSET(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM) == DB_STAT_CLEAR)
			__mutex_clear(env, hp->mtx_hash);
		mstat->st_io_wait += hp->hash_io_wait;
		mstat->st_mvcc_frozen += hp->hash_frozen;
		mstat->st_mvcc_thawed += hp->hash_thawed;
		mstat->st_mvcc_freed += hp->hash_frozen_freed;
		if(LF_ISSET(DB_STAT_CLEAR)) {
			hp->hash_io_wait = 0;
			hp->hash_frozen = 0;
			hp->hash_thawed = 0;
			hp->hash_frozen_freed = 0;
		}
	}
}

#else /* !HAVE_STATISTICS */

int __memp_stat_pp(DB_ENV * dbenv, DB_MPOOL_STAT ** gspp, DB_MPOOL_FSTAT *** fspp, uint32 flags)
{
	COMPQUIET(gspp, 0);
	COMPQUIET(fspp, 0);
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}

int __memp_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}
#endif
/*
 * __memp_stat_hash --
 *	Total hash bucket stats (other than mutex wait) into the region.
 */
void __memp_stat_hash(REGINFO * reginfo, MPOOL * mp, uint32 * dirtyp)
{
	uint32 dirty, i;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(reginfo, mp->htab);
	for(i = 0, dirty = 0; i < mp->htab_buckets; i++, hp++)
		dirty += (uint32)atomic_read(&hp->hash_page_dirty);
	*dirtyp = dirty;
}
/*
 * __memp_walk_files --
 */
int __memp_walk_files(ENV * env, MPOOL * mp, int (*func)(ENV*, MPOOLFILE*, void *, uint32*, uint32), void * arg, uint32 * countp, uint32 flags)
{
	MPOOLFILE * mfp;
	int i, t_ret;
	DB_MPOOL * dbmp = env->mp_handle;
	int ret = 0;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	for(i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
			if((t_ret = func(env, mfp, arg, countp, flags)) != 0 && ret == 0)
				ret = t_ret;
			if(ret != 0 && !LF_ISSET(DB_STAT_MEMP_NOERROR))
				break;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
		if(ret != 0 && !LF_ISSET(DB_STAT_MEMP_NOERROR))
			break;
	}
	return ret;
}
/*
 * __memp_discard_all_mpfs --
 *	Force discard all mpoolfiles. When closing a private environment, we
 *	always want to discard all mpoolfiles to avoid memory leak.
 */
int __memp_discard_all_mpfs(ENV * env, MPOOL * mp)
{
	int i;
	int ret = 0;
	int t_ret = 0;
	MPOOLFILE * mfp = NULL;
	DB_MPOOL * dbmp = env->mp_handle;
	DB_MPOOL_HASH * hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	for(i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(env, hp->mtx_hash);
		while((mfp = SH_TAILQ_FIRST(&hp->hash_bucket, __mpoolfile)) != NULL) {
			MUTEX_LOCK(env, mfp->mutex);
			if((t_ret = __memp_mf_discard(dbmp, mfp, 1)) != 0 && ret == 0)
				ret = t_ret;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	return ret;
}
/*
 * __memp_sync_pp --
 *	ENV->memp_sync pre/post processing.
 */
int __memp_sync_pp(DB_ENV * dbenv, DB_LSN * lsnp)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "memp_sync", DB_INIT_MPOOL);
	/*
	 * If no LSN is provided, flush the entire cache (reasonable usage
	 * even if there's no log subsystem configured).
	 */
	if(lsnp)
		ENV_REQUIRES_CONFIG(env, env->lg_handle, "memp_sync", DB_INIT_LOG);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_sync(env, DB_SYNC_CACHE, lsnp)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_sync --
 *	ENV->memp_sync.
 */
int __memp_sync(ENV * env, uint32 flags, DB_LSN * lsnp)
{
	int interrupted, ret;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	/* If we've flushed to the requested LSN, return that information. */
	if(lsnp) {
		MPOOL_SYSTEM_LOCK(env);
		if(LOG_COMPARE(lsnp, &mp->lsn) <= 0) {
			*lsnp = mp->lsn;
			MPOOL_SYSTEM_UNLOCK(env);
			return 0;
		}
		MPOOL_SYSTEM_UNLOCK(env);
	}
	if((ret = __memp_sync_int(env, NULL, 0, flags, NULL, &interrupted)) != 0)
		return ret;
	if(!interrupted && lsnp) {
		MPOOL_SYSTEM_LOCK(env);
		if(LOG_COMPARE(lsnp, &mp->lsn) > 0)
			mp->lsn = *lsnp;
		MPOOL_SYSTEM_UNLOCK(env);
	}
	return 0;
}
/*
 * __memp_fsync_pp --
 *	DB_MPOOLFILE->sync pre/post processing.
 */
int __memp_fsync_pp(DB_MPOOLFILE * dbmfp)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbmfp->env;
	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->sync");
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_fsync(dbmfp)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_fsync --
 *	DB_MPOOLFILE->sync.
 */
int __memp_fsync(DB_MPOOLFILE * dbmfp)
{
	MPOOLFILE * mfp = dbmfp->mfp;
	/*
	 * If this handle doesn't have a file descriptor that's open for
	 * writing, or if the file is a temporary, or if the file hasn't
	 * been written since it was flushed, there's no reason to proceed
	 * further.
	 */
	if(F_ISSET(dbmfp, MP_READONLY))
		return 0;
	if(F_ISSET(dbmfp->mfp, MP_TEMP) || dbmfp->mfp->no_backing_file)
		return 0;
	if(mfp->file_written == 0)
		return 0;
	return __memp_sync_int(dbmfp->env, dbmfp, 0, DB_SYNC_FILE, 0, 0);
}
/*
 * __mp_xxx_fh --
 *	Return a file descriptor for DB 1.85 compatibility locking.
 */
int __mp_xxx_fh(DB_MPOOLFILE * dbmfp, DB_FH ** fhp)
{
	int ret;
	/*
	 * This is a truly spectacular layering violation, intended ONLY to
	 * support compatibility for the DB 1.85 DB->fd call.
	 *
	 * Sync the database file to disk, creating the file as necessary.
	 *
	 * We skip the MP_READONLY and MP_TEMP tests done by memp_fsync(3).
	 * The MP_READONLY test isn't interesting because we will either
	 * already have a file descriptor (we opened the database file for
	 * reading) or we aren't readonly (we created the database which
	 * requires write privileges).  The MP_TEMP test isn't interesting
	 * because we want to write to the backing file regardless so that
	 * we get a file descriptor to return.
	 */
	if((*fhp = dbmfp->fhp) != NULL)
		return 0;
	if((ret = __memp_sync_int(dbmfp->env, dbmfp, 0, DB_SYNC_FILE, NULL, NULL)) == 0)
		*fhp = dbmfp->fhp;
	return ret;
}
/*
 * __memp_sync_int --
 *	Mpool sync internal function.
 */
int __memp_sync_int(ENV * env, DB_MPOOLFILE * dbmfp, uint32 trickle_max, uint32 flags, uint32 * wrote_totalp, int * interruptedp)
{
	BH * bhp;
	BH_TRACK * bharray;
	DB_MPOOL_HASH * hp;
	MPOOL * c_mp;
	MPOOLFILE * mfp;
	db_mutex_t mutex;
	uint32 ar_cnt, ar_max, i, n_cache, remaining;
	int32 wrote_cnt;
	int dirty, maxopenfd, required_write, ret, t_ret;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	roff_t last_mf_offset = INVALID_ROFF;
	int filecnt = 0;
	uint32 wrote_total = 0;
	ASSIGN_PTR(wrote_totalp, 0);
	ASSIGN_PTR(interruptedp, 0);
	/*
	 * If we're flushing the cache, it's a checkpoint or we're flushing a
	 * specific file, we really have to write the blocks and we have to
	 * confirm they made it to disk.  Otherwise, we can skip a block if it's hard to get.
	 */
	required_write = LF_ISSET(DB_SYNC_CACHE|DB_SYNC_CHECKPOINT|DB_SYNC_FILE|DB_SYNC_QUEUE_EXTENT);
	/* Get shared configuration information. */
	MPOOL_SYSTEM_LOCK(env);
	maxopenfd = mp->mp_maxopenfd;
	MPOOL_SYSTEM_UNLOCK(env);
	/* Assume one dirty page per bucket. */
	ar_max = mp->nreg*mp->htab_buckets;
	if((ret = __os_malloc(env, ar_max*sizeof(BH_TRACK), &bharray)) != 0)
		return ret;
	/*
	 * Walk each cache's list of buffers and mark all dirty buffers to be
	 * written and all dirty buffers to be potentially written, depending on our flags.
	 */
	for(ar_cnt = 0, n_cache = 0; n_cache < mp->nreg; ++n_cache) {
		c_mp = (MPOOL *)dbmp->reginfo[n_cache].primary;
		hp = (DB_MPOOL_HASH *)R_ADDR(&dbmp->reginfo[n_cache], c_mp->htab);
		for(i = 0; i < c_mp->htab_buckets; i++, hp++) {
			/*
			 * We can check for empty buckets before locking as
			 * we only care if the pointer is zero or non-zero.
			 * We can ignore empty or clean buckets because we
			 * only need write buffers that were dirty before we started.
			 */
#ifdef DIAGNOSTIC
			if(SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL)
#else
			if(atomic_read(&hp->hash_page_dirty) == 0)
#endif
				continue;
			dirty = 0;
			MUTEX_LOCK(env, hp->mtx_hash);
			SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
				// Always ignore clean pages
				if(!F_ISSET(bhp, BH_DIRTY))
					continue;
				dirty++;
				mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
				// Ignore in-memory files, unless the file is specifically being flushed.
				if(mfp->no_backing_file)
					continue;
				if(!LF_ISSET(DB_SYNC_FILE) && F_ISSET(mfp, MP_TEMP))
					continue;
				// Ignore files that aren't involved in DB's transactional operations during checkpoints.
				if(LF_ISSET(DB_SYNC_CHECKPOINT) && mfp->lsn_off == DB_LSN_OFF_NOTSET)
					continue;
				// Ignore files that aren't Queue extent files if we're flushing a Queue file with extents.
				if(LF_ISSET(DB_SYNC_QUEUE_EXTENT) && !F_ISSET(mfp, MP_EXTENT))
					continue;
				// If we're flushing a specific file, see if this page is from that file.
				if(dbmfp && mfp != dbmfp->mfp)
					continue;
				// Track the buffer, we want it.
				bharray[ar_cnt].track_hp = hp;
				bharray[ar_cnt].track_pgno = bhp->pgno;
				bharray[ar_cnt].track_off = bhp->mf_offset;
				ar_cnt++;
				//
				// If we run out of space, double and continue.
				// Don't stop at trickle_max, we want to sort
				// as large a sample set as possible in order
				// to minimize disk seeks.
				//
				if(ar_cnt >= ar_max) {
					if((ret = __os_realloc(env, (ar_max*2)*sizeof(BH_TRACK), &bharray)) != 0)
						break;
					ar_max *= 2;
				}
			}
			if(ret != 0)
				goto err;
			/*
			 * We are only checking this in diagnostic mode
			 * since it requires extra latching to keep the count
			 * in sync with the number of bits counted.
			 */
			DB_ASSERT(env, dirty == (int)atomic_read(&hp->hash_page_dirty));
			MUTEX_UNLOCK(env, hp->mtx_hash);
			/* Check if the call has been interrupted. */
			if(LF_ISSET(DB_SYNC_INTERRUPT_OK) && FLD_ISSET(mp->config_flags, DB_MEMP_SYNC_INTERRUPT)) {
				STAT(++mp->stat.st_sync_interrupted);
				ASSIGN_PTR(interruptedp, 1);
				goto err;
			}
		}
	}
	// If there no buffers to write, we're done
	if(ar_cnt == 0)
		goto done;
	/*
	 * Write the buffers in file/page order, trying to reduce seeks by the
	 * filesystem and, when pages are smaller than filesystem block sizes,
	 * reduce the actual number of writes.
	 */
	if(ar_cnt > 1)
		qsort(bharray, ar_cnt, sizeof(BH_TRACK), __bhcmp);
	//
	// If we're trickling buffers, only write enough to reach the correct percentage.
	//
	if(LF_ISSET(DB_SYNC_TRICKLE) && ar_cnt > trickle_max)
		ar_cnt = trickle_max;
	/*
	 * Flush the log.  We have to ensure the log records reflecting the
	 * changes on the database pages we're writing have already made it
	 * to disk.  We still have to check the log each time we write a page
	 * (because pages we are about to write may be modified after we have
	 * flushed the log), but in general this will at least avoid any I/O
	 * on the log's part.
	 */
	if(LOGGING_ON(env) && (ret = __log_flush(env, NULL)) != 0)
		goto err;
	/*
	 * Walk the array, writing buffers.  When we write a buffer, we NULL
	 * out its hash bucket pointer so we don't process a slot more than
	 * once.
	 */
	for(i = wrote_cnt = 0, remaining = ar_cnt; remaining > 0; ++i) {
		if(i >= ar_cnt) {
			i = 0;
			__os_yield(env, 1, 0);
		}
		if((hp = bharray[i].track_hp) == NULL)
			continue;
		/* Lock the hash bucket and find the buffer. */
		mutex = hp->mtx_hash;
		MUTEX_READLOCK(env, mutex);
		SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh)
		if(bhp->pgno == bharray[i].track_pgno && bhp->mf_offset == bharray[i].track_off)
			break;
		/*
		 * If we can't find the buffer we're done, somebody else had
		 * to have written it.
		 *
		 * If the buffer isn't dirty, we're done, there's no work
		 * needed.
		 */
		if(bhp == NULL || !F_ISSET(bhp, BH_DIRTY)) {
			MUTEX_UNLOCK(env, mutex);
			--remaining;
			bharray[i].track_hp = NULL;
			continue;
		}
		/*
		 * If the buffer is locked by another thread, ignore it, we'll
		 * come back to it.
		 */
		if(F_ISSET(bhp, BH_EXCLUSIVE)) {
			MUTEX_UNLOCK(env, mutex);
			if(!required_write) {
				--remaining;
				bharray[i].track_hp = NULL;
			}
			continue;
		}
		/* Pin the buffer into memory. */
		atomic_inc(env, &bhp->ref);
		MUTEX_UNLOCK(env, mutex);
		MUTEX_READLOCK(env, bhp->mtx_buf);
		DB_ASSERT(env, !F_ISSET(bhp, BH_EXCLUSIVE));
		/*
		 * When swapping the hash bucket mutex for the buffer mutex,
		 * we may have raced with an MVCC update.  In that case, we
		 * no longer have the most recent version, and need to retry
		 * (the buffer header we have pinned will no longer be marked
		 * dirty, so we can't just write it).
		 */
		if(SH_CHAIN_HASNEXT(bhp, vc)) {
			atomic_dec(env, &bhp->ref);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			continue;
		}
		/* we will dispose of this buffer. */
		--remaining;
		bharray[i].track_hp = NULL;
		/*
		 * If we've switched files, check to see if we're configured
		 * to close file descriptors.
		 */
		if(maxopenfd != 0 && bhp->mf_offset != last_mf_offset) {
			if(++filecnt >= maxopenfd) {
				filecnt = 0;
				if((t_ret = __memp_close_flush_files(env, 1)) != 0 && ret == 0)
					ret = t_ret;
			}
			last_mf_offset = bhp->mf_offset;
		}
		//
		// If the buffer is dirty, we write it.  We only try to write the buffer once.
		//
		if(F_ISSET(bhp, BH_DIRTY)) {
			mfp = (MPOOLFILE *)R_ADDR(dbmp->reginfo, bhp->mf_offset);
			if((t_ret = __memp_bhwrite(dbmp, hp, mfp, bhp, 1)) == 0) {
				++wrote_cnt;
				++wrote_total;
			}
			else {
				SETIFZ(ret, t_ret);
				__db_errx(env, DB_STR_A("3027", "%s: unable to flush page: %lu", "%s %lu"), __memp_fns(dbmp, mfp), (ulong)bhp->pgno);

			}
		}
		// Discard our buffer reference
		DB_ASSERT(env, atomic_read(&bhp->ref) > 0);
		atomic_dec(env, &bhp->ref);
		MUTEX_UNLOCK(env, bhp->mtx_buf);
		// Check if the call has been interrupted
		if(LF_ISSET(DB_SYNC_INTERRUPT_OK) && FLD_ISSET(mp->config_flags, DB_MEMP_SYNC_INTERRUPT)) {
			STAT(++mp->stat.st_sync_interrupted);
			ASSIGN_PTR(interruptedp, 1);
			goto err;
		}
		//
		// Sleep after some number of writes to avoid disk saturation.
		// Don't cache the max writes value, an application shutting
		// down might reset the value in order to do a fast flush or checkpoint.
		//
		if(!LF_ISSET(DB_SYNC_SUPPRESS_WRITE) && !FLD_ISSET(mp->config_flags, DB_MEMP_SUPPRESS_WRITE) && mp->mp_maxwrite != 0 && wrote_cnt >= mp->mp_maxwrite) {
			wrote_cnt = 0;
			__os_yield(env, 0, (ulong)mp->mp_maxwrite_sleep);
		}
	}
done:
	/*
	 * If a write is required, we have to force the pages to disk.  We
	 * don't do this as we go along because we want to give the OS as
	 * much time as possible to lazily flush, and because we have to flush
	 * files that might not even have had dirty buffers in the cache, so
	 * we have to walk the files list.
	 */
	if(ret == 0 && required_write) {
		ret = (dbmfp == NULL) ? __memp_sync_files(env) : __os_fsync(env, dbmfp->fhp);
	}
	// If we've opened files to flush pages, close them.
	if((t_ret = __memp_close_flush_files(env, 0)) != 0 && ret == 0)
		ret = t_ret;
err:
	__os_free(env, bharray);
	ASSIGN_PTR(wrote_totalp, wrote_total);
	return ret;
}

static int __memp_sync_file(ENV * env, MPOOLFILE * mfp, void * argp, uint32 * countp, uint32 flags)
{
	DB_MPOOL * dbmp;
	DB_MPOOLFILE * dbmfp;
	int ret, t_ret;
	COMPQUIET(countp, 0);
	COMPQUIET(flags, 0);
	if(!mfp->file_written || mfp->no_backing_file || mfp->deadfile || F_ISSET(mfp, MP_TEMP))
		return 0;
	/*
	 * Pin the MPOOLFILE structure into memory, and release the
	 * region mutex allowing us to walk the linked list.  We'll
	 * re-acquire that mutex to move to the next entry in the list.
	 *
	 * This works because we only need to flush current entries,
	 * we don't care about new entries being added, and the linked
	 * list is never re-ordered, a single pass is sufficient.  It
	 * requires MPOOLFILE structures removed before we get to them
	 * be flushed to disk, but that's nothing new, they could have
	 * been removed while checkpoint was running, too.
	 *
	 * Once we have the MPOOLFILE lock, re-check the MPOOLFILE is
	 * not being discarded.  (A thread removing the MPOOLFILE
	 * will: hold the MPOOLFILE mutex, set deadfile, drop the
	 * MPOOLFILE mutex and then acquire the region MUTEX to walk
	 * the linked list and remove the MPOOLFILE structure.  Make
	 * sure the MPOOLFILE wasn't marked dead while we waited for the mutex.
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if(!mfp->file_written || mfp->deadfile) {
		MUTEX_UNLOCK(env, mfp->mutex);
		return 0;
	}
	++mfp->mpf_cnt;
	MUTEX_UNLOCK(env, mfp->mutex);
	/*
	 * Look for an already open, writable handle (fsync doesn't
	 * work on read-only Windows handles).
	 */
	dbmp = env->mp_handle;
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q) {
		if(dbmfp->mfp != mfp || F_ISSET(dbmfp, MP_READONLY))
			continue;
		/*
		 * We don't want to hold the mutex while calling sync.
		 * Increment the DB_MPOOLFILE handle ref count to pin
		 * it into memory.
		 */
		++dbmfp->ref;
		break;
	}
	MUTEX_UNLOCK(env, dbmp->mutex);
	/* If we don't find a handle we can use, open one. */
	if(dbmfp == NULL) {
		if((ret = __memp_mf_sync(dbmp, mfp, 1)) != 0) {
			__db_err(env, ret, DB_STR_A("3028", "%s: unable to flush", "%s"), (char *)R_ADDR(dbmp->reginfo, mfp->path_off));
		}
	}
	else
		ret = __os_fsync(env, dbmfp->fhp);
	//
	// Re-acquire the MPOOLFILE mutex, we need it to modify the reference count.
	//
	MUTEX_LOCK(env, mfp->mutex);
	/*
	 * If we wrote the file and there are no other references (or there
	 * is a single reference, and it's the one we opened to write
	 * buffers during checkpoint), clear the file_written flag.  We
	 * do this so that applications opening thousands of files don't
	 * loop here opening and flushing those files during checkpoint.
	 *
	 * The danger here is if a buffer were to be written as part of
	 * a checkpoint, and then not be flushed to disk.  This cannot
	 * happen because we only clear file_written when there are no
	 * other users of the MPOOLFILE in the system, and, as we hold
	 * the region lock, no possibility of another thread of control
	 * racing with us to open a MPOOLFILE.
	 */
	if(mfp->mpf_cnt == 1 || (mfp->mpf_cnt == 2 && dbmfp && F_ISSET(dbmfp, MP_FLUSH))) {
		mfp->file_written = 0;
		/*
		 * We may be the last reference for a MPOOLFILE, as we
		 * weren't holding the MPOOLFILE mutex when flushing
		 * it's buffers to disk.  If we can discard it, set
		 * a flag to schedule a clean-out pass.   (Not likely,
		 * I mean, what are the chances that there aren't any
		 * buffers in the pool?  Regardless, it might happen.)
		 */
		if(mfp->mpf_cnt == 1 && mfp->block_cnt == 0)
			*(int *)argp = 1;
	}
	/*
	 * If we found the file we must close it in case we are the last
	 * reference to the dbmfp.  NOTE: since we have incremented
	 * mfp->mpf_cnt this cannot be the last reference to the mfp.
	 * This is important since we are called with the hash bucket
	 * locked.  The mfp will get freed via the cleanup pass.
	 */
	if(dbmfp && (t_ret = __memp_fclose(dbmfp, DB_MPOOL_NOLOCK)) != 0 && ret == 0)
		ret = t_ret;
	--mfp->mpf_cnt;
	/* Unlock the MPOOLFILE. */
	MUTEX_UNLOCK(env, mfp->mutex);
	return ret;
}
/*
 * __memp_sync_files --
 *	Sync all the files in the environment, open or not.
 */
static int __memp_sync_files(ENV * env)
{
	DB_MPOOL_HASH * hp;
	MPOOLFILE * mfp, * next_mfp;
	int i;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	int need_discard_pass = 0;
	int ret = __memp_walk_files(env, mp, __memp_sync_file, &need_discard_pass, 0, DB_STAT_MEMP_NOERROR);
	/*
	 * We may need to do a last pass through the MPOOLFILE list -- if we
	 * were the last reference to an MPOOLFILE, we need to clean it out.
	 */
	if(!need_discard_pass)
		return ret;
	hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	for(i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
retry:
		MUTEX_LOCK(env, hp->mtx_hash);
		for(mfp = SH_TAILQ_FIRST(&hp->hash_bucket, __mpoolfile); mfp; mfp = next_mfp) {
			next_mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile);
			/*
			 * Do a fast check -- we can check for zero/non-zero
			 * without a mutex on the MPOOLFILE.  If likely to
			 * succeed, lock the MPOOLFILE down and look for real.
			 */
			if(mfp->deadfile || mfp->block_cnt != 0 || mfp->mpf_cnt != 0)
				continue;
			MUTEX_LOCK(env, mfp->mutex);
			if(!mfp->deadfile && mfp->block_cnt == 0 && mfp->mpf_cnt == 0) {
				MUTEX_UNLOCK(env, hp->mtx_hash);
				__memp_mf_discard(dbmp, mfp, 0);
				goto retry;
			}
			else
				MUTEX_UNLOCK(env, mfp->mutex);
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	return ret;
}
/*
 * __memp_mf_sync --
 *	Flush an MPOOLFILE, when no currently open handle is available.
 */
int __memp_mf_sync(DB_MPOOL * dbmp, MPOOLFILE * mfp, int locked)
{
	DB_FH * fhp;
	DB_MPOOL_HASH * hp;
	ENV * env;
	MPOOL * mp;
	int ret, t_ret;
	char * rpath;
	COMPQUIET(hp, 0);
	env = dbmp->env;
	//
	// We need to be holding the hash lock: we're using the path name
	// and __memp_nameop might try and rename the file.
	//
	if(!locked) {
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
		hp += FNBUCKET(R_ADDR(dbmp->reginfo, mfp->fileid_off), DB_FILE_ID_LEN);
		MUTEX_LOCK(env, hp->mtx_hash);
	}
	if((ret = __db_appname(env, DB_APP_DATA, (const char *)R_ADDR(dbmp->reginfo, mfp->path_off), NULL, &rpath)) == 0) {
		if((ret = __os_open(env, rpath, 0, 0, 0, &fhp)) == 0) {
			ret = __os_fsync(env, fhp);
			if((t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
				ret = t_ret;
		}
		__os_free(env, rpath);
	}
	if(!locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);
	return ret;
}
/*
 * __memp_close_flush_files --
 *	Close files opened only to flush buffers.
 */
static int __memp_close_flush_files(ENV * env, int dosync)
{
	DB_MPOOLFILE * dbmfp;
	MPOOLFILE * mfp;
	int ret;
	DB_MPOOL * dbmp = env->mp_handle;
	/*
	 * The routine exists because we must close files opened by sync to
	 * flush buffers.  There are two cases: first, extent files have to
	 * be closed so they may be removed when empty.  Second, regular
	 * files have to be closed so we don't run out of descriptors (for
	 * example, an application partitioning its data into databases
	 * based on timestamps, so there's a continually increasing set of
	 * files).
	 *
	 * We mark files opened in the __memp_bhwrite() function with the
	 * MP_FLUSH flag.  Here we walk through our file descriptor list,
	 * and, if a file was opened by __memp_bhwrite(), we close it.
	 */
retry:
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q)
	if(F_ISSET(dbmfp, MP_FLUSH)) {
		F_CLR(dbmfp, MP_FLUSH);
		MUTEX_UNLOCK(env, dbmp->mutex);
		if(dosync) {
			/*
			 * If we have the only open handle on the file,
			 * clear the dirty flag so we don't re-open and
			 * sync it again when discarding the MPOOLFILE
			 * structure.  Clear the flag before the sync
			 * so can't race with a thread writing the file.
			 */
			mfp = dbmfp->mfp;
			if(mfp->mpf_cnt == 1) {
				MUTEX_LOCK(env, mfp->mutex);
				if(mfp->mpf_cnt == 1)
					mfp->file_written = 0;
				MUTEX_UNLOCK(env, mfp->mutex);
			}
			if((ret = __os_fsync(env, dbmfp->fhp)) != 0)
				return ret;
		}
		if((ret = __memp_fclose(dbmfp, 0)) != 0)
			return ret;
		goto retry;
	}
	MUTEX_UNLOCK(env, dbmp->mutex);
	return 0;
}

static int __bhcmp(const void * p1, const void * p2)
{
	BH_TRACK * bhp1, * bhp2;
	bhp1 = (BH_TRACK *)p1;
	bhp2 = (BH_TRACK *)p2;
	/* Sort by file (shared memory pool offset). */
	if(bhp1->track_off < bhp2->track_off)
		return -1;
	if(bhp1->track_off > bhp2->track_off)
		return 1;
	/*
	 * !!!
	 * Defend against badly written quicksort code calling the comparison
	 * function with two identical pointers (e.g., WATCOM C++ (Power++)).
	 */
	if(bhp1->track_pgno < bhp2->track_pgno)
		return -1;
	if(bhp1->track_pgno > bhp2->track_pgno)
		return 1;
	return 0;
}
/*
 * __memp_trickle_pp --
 *	ENV->memp_trickle pre/post processing.
 */
int __memp_trickle_pp(DB_ENV * dbenv, int pct, int * nwrotep)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "memp_trickle", DB_INIT_MPOOL);
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_trickle(env, pct, nwrotep)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __memp_trickle --
 *	ENV->memp_trickle.
 */
static int __memp_trickle(ENV * env, int pct, int * nwrotep)
{
	MPOOL * c_mp;
	uint32 clean, dirty, i, need_clean, total, dtmp, wrote;
	int ret;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
	ASSIGN_PTR(nwrotep, 0);
	if(pct < 1 || pct > 100) {
		__db_errx(env, DB_STR_A("3007", "DB_ENV->memp_trickle: %d: percent must be between 1 and 100", "%d"), pct);
		return EINVAL;
	}
	/*
	 * Loop through the caches counting total/dirty buffers.
	 *
	 * XXX
	 * Using hash_page_dirty is our only choice at the moment, but it's not
	 * as correct as we might like in the presence of pools having more
	 * than one page size, as a free 512B buffer may not be equivalent to
	 * having a free 8KB buffer.
	 */
	for(ret = 0, i = dirty = total = 0; i < mp->nreg; ++i) {
		c_mp = (MPOOL *)dbmp->reginfo[i].primary;
		total += c_mp->pages;
		__memp_stat_hash(&dbmp->reginfo[i], c_mp, &dtmp);
		dirty += dtmp;
	}
	/*
	 * If there are sufficient clean buffers, no buffers or no dirty
	 * buffers, we're done.
	 */
	if(total == 0 || dirty == 0)
		return 0;
	/*
	 * The total number of pages is an exact number, but the dirty page
	 * count can change while we're walking the hash buckets, and it's
	 * even possible the dirty page count ends up larger than the total
	 * number of pages.
	 */
	clean = total > dirty ? total-dirty : 0;
	need_clean = (total*(uint)pct)/100;
	if(clean >= need_clean)
		return 0;
	need_clean -= clean;
	ret = __memp_sync_int(env, NULL, need_clean, DB_SYNC_TRICKLE|DB_SYNC_INTERRUPT_OK, &wrote, 0);
	STAT((mp->stat.st_page_trickle += wrote));
	ASSIGN_PTR(nwrotep, (int)wrote);
	return ret;
}
/*
 * __memp_env_create --
 *	Mpool specific creation of the DB_ENV structure.
 */
int __memp_env_create(DB_ENV * dbenv)
{
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 *
	 * We default to 32 8K pages.  We don't default to a flat 256K, because
	 * we want to include the size of the buffer header which can vary
	 * from system to system.
	 */
	dbenv->mp_bytes = 32*((8*1024)+sizeof(BH))+37*sizeof(DB_MPOOL_HASH);
	dbenv->mp_ncache = 1;
	return 0;
}
/*
 * __memp_env_destroy --
 *	Mpool specific destruction of the DB_ENV structure.
 */
void __memp_env_destroy(DB_ENV * dbenv)
{
	COMPQUIET(dbenv, 0);
}
/*
 * __memp_get_cachesize --
 *	{DB_ENV,DB}->get_cachesize.
 */
int __memp_get_cachesize(DB_ENV * dbenv, uint32 * gbytesp, uint32 * bytesp, int * ncachep)
{
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_cachesize", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		DB_MPOOL * dbmp = env->mp_handle;
		MPOOL * mp = (MPOOL *)dbmp->reginfo[0].primary;
		ASSIGN_PTR(gbytesp, mp->gbytes);
		ASSIGN_PTR(bytesp, mp->bytes);
		ASSIGN_PTR(ncachep, (int)mp->nreg);
	}
	else {
		ASSIGN_PTR(gbytesp, dbenv->mp_gbytes);
		ASSIGN_PTR(bytesp, dbenv->mp_bytes);
		ASSIGN_PTR(ncachep, (int)dbenv->mp_ncache);
	}
	return 0;
}
/*
 * __memp_set_cachesize --
 *	{DB_ENV,DB}->set_cachesize.
 */
int __memp_set_cachesize(DB_ENV * dbenv, uint32 gbytes, uint32 bytes, int arg_ncache)
{
	DB_THREAD_INFO * ip;
	uint ncache;
	int ret = 0;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->set_cachesize", DB_INIT_MPOOL);
	/* Normalize the cache count. */
	ncache = arg_ncache <= 0 ? 1 : (uint)arg_ncache;
	/*
	 * You can only store 4GB-1 in an unsigned 32-bit value, so correct for
	 * applications that specify 4GB cache sizes -- we know what they meant.
	 */
	if(sizeof(roff_t) == 4 && gbytes/ncache == 4 && bytes == 0) {
		--gbytes;
		bytes = GIGABYTE-1;
	}
	else {
		gbytes += bytes/GIGABYTE;
		bytes %= GIGABYTE;
	}
	/*
	 * !!!
	 * With 32-bit region offsets, individual cache regions must be smaller
	 * than 4GB.  Also, cache sizes larger than 10TB would cause 32-bit
	 * wrapping in the calculation of the number of hash buckets.  See
	 * __memp_open for details.
	 */
	if(!F_ISSET(env, ENV_OPEN_CALLED)) {
		if(sizeof(roff_t) <= 4 && gbytes/ncache >= 4) {
			__db_errx(env, DB_STR("3003", "individual cache size too large: maximum is 4GB"));
			return EINVAL;
		}
		if(gbytes/ncache > 10000) {
			__db_errx(env, DB_STR("3004", "individual cache size too large: maximum is 10TB"));
			return EINVAL;
		}
	}
	/*
	 * If the application requested less than 500Mb, increase the cachesize
	 * by 25% and factor in the size of the hash buckets to account for our
	 * overhead.  (I'm guessing caches over 500Mb are specifically sized,
	 * that is, it's a large server and the application actually knows how
	 * much memory is available.  We only document the 25% overhead number,
	 * not the hash buckets, but I don't see a reason to confuse the issue,
	 * it shouldn't matter to an application.)
	 *
	 * There is a minimum cache size, regardless.
	 */
	if(gbytes == 0) {
		if(bytes < SMEGABYTE(500))
			bytes += (bytes/4)+37*sizeof(DB_MPOOL_HASH);
		if(bytes/ncache < DB_CACHESIZE_MIN)
			bytes = ncache*DB_CACHESIZE_MIN;
	}
	if(F_ISSET(env, ENV_OPEN_CALLED)) {
		ENV_ENTER(env, ip);
		ret = __memp_resize(env->mp_handle, gbytes, bytes);
		ENV_LEAVE(env, ip);
		return ret;
	}
	dbenv->mp_gbytes = gbytes;
	dbenv->mp_bytes = bytes;
	dbenv->mp_ncache = ncache;
	return 0;
}
/*
 * __memp_set_config --
 *	Set the cache subsystem configuration.
 */
int __memp_set_config(DB_ENV * dbenv, uint32 which, int on)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->memp_set_config", DB_INIT_MPOOL);
	switch(which) {
	    case DB_MEMP_SUPPRESS_WRITE:
	    case DB_MEMP_SYNC_INTERRUPT:
		if(MPOOL_ON(env)) {
			dbmp = env->mp_handle;
			mp = (MPOOL *)dbmp->reginfo[0].primary;
			SETFLAG(mp->config_flags, which, on);
		}
		break;
	    default:
		return EINVAL;
	}
	return 0;
}
/*
 * __memp_get_config --
 *	Return the cache subsystem configuration.
 */
int __memp_get_config(DB_ENV * dbenv, uint32 which, int * onp)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mp_handle, "DB_ENV->memp_get_config", DB_INIT_MPOOL);
	switch(which) {
	    case DB_MEMP_SUPPRESS_WRITE:
	    case DB_MEMP_SYNC_INTERRUPT:
		if(MPOOL_ON(env)) {
			dbmp = env->mp_handle;
			mp = (MPOOL *)dbmp->reginfo[0].primary;
			*onp = FLD_ISSET(mp->config_flags, which) ? 1 : 0;
		}
		else
			*onp = 0;
		break;
	    default:
		return EINVAL;
	}
	return 0;
}

int __memp_get_mp_max_openfd(DB_ENV * dbenv, int * maxopenfdp)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_openfd", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*maxopenfdp = mp->mp_maxopenfd;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else
		*maxopenfdp = dbenv->mp_maxopenfd;
	return 0;
}
/*
 * __memp_set_mp_max_openfd --
 *	Set the maximum number of open fd's when flushing the cache.
 */
int __memp_set_mp_max_openfd(DB_ENV * dbenv, int maxopenfd)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->set_mp_max_openfd", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		mp->mp_maxopenfd = maxopenfd;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else
		dbenv->mp_maxopenfd = maxopenfd;
	return 0;
}

int __memp_get_mp_max_write(DB_ENV * dbenv, int * maxwritep, db_timeout_t * maxwrite_sleepp)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_write", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*maxwritep = mp->mp_maxwrite;
		*maxwrite_sleepp = mp->mp_maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else {
		*maxwritep = dbenv->mp_maxwrite;
		*maxwrite_sleepp = dbenv->mp_maxwrite_sleep;
	}
	return 0;
}
/*
 * __memp_set_mp_max_write --
 *	Set the maximum continuous I/O count.
 */
int __memp_set_mp_max_write(DB_ENV * dbenv, int maxwrite, db_timeout_t maxwrite_sleep)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_write", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		mp->mp_maxwrite = maxwrite;
		mp->mp_maxwrite_sleep = maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else {
		dbenv->mp_maxwrite = maxwrite;
		dbenv->mp_maxwrite_sleep = maxwrite_sleep;
	}
	return 0;
}

int __memp_get_mp_mmapsize(DB_ENV * dbenv, size_t * mp_mmapsizep)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*mp_mmapsizep = mp->mp_mmapsize;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else
		*mp_mmapsizep = dbenv->mp_mmapsize;
	return 0;
}
/*
 * __memp_set_mp_mmapsize --
 *	DB_ENV->set_mp_mmapsize.
 */
int __memp_set_mp_mmapsize(DB_ENV * dbenv, size_t mp_mmapsize)
{
	DB_MPOOL * dbmp;
	DB_THREAD_INFO * ip;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->set_mp_max_mmapsize", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		/*
		 * We need to cast here because size_t and db_size_t can be
		 * different on a 64 bit build, when building in 32 bit
		 * compatibility mode. The cast is safe, because we check for
		 * overflow when the fields are assigned.
		 */
		mp->mp_mmapsize = (db_size_t)mp_mmapsize;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	}
	else
		dbenv->mp_mmapsize = (db_size_t)mp_mmapsize;
	return 0;
}

int __memp_get_mp_pagesize(DB_ENV * dbenv, uint32 * mp_pagesizep)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_pagesize", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		*mp_pagesizep = mp->pagesize;
	}
	else {
		*mp_pagesizep = dbenv->mp_pagesize;
	}
	return 0;
}
/*
 * __memp_set_mp_pagesize --
 *	DB_ENV->set_mp_pagesize.
 */
int __memp_set_mp_pagesize(DB_ENV * dbenv, uint32 mp_pagesize)
{
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mp_pagesize");
	dbenv->mp_pagesize = mp_pagesize;
	return 0;
}

int __memp_get_mp_tablesize(DB_ENV * dbenv, uint32 * mp_tablesizep)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_tablesize", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		*mp_tablesizep = mp->htab_buckets;
	}
	else
		*mp_tablesizep = dbenv->mp_tablesize;
	return 0;
}
/*
 * __memp_set_mp_tablesize --
 *	DB_ENV->set_mp_tablesize.
 */
int __memp_set_mp_tablesize(DB_ENV * dbenv, uint32 mp_tablesize)
{
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mp_tablesize");
	dbenv->mp_tablesize = mp_tablesize;
	return 0;
}

int __memp_get_mp_mtxcount(DB_ENV * dbenv, uint32 * mp_mtxcountp)
{
	DB_MPOOL * dbmp;
	MPOOL * mp;
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_mtxcount", DB_INIT_MPOOL);
	if(MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = (MPOOL *)dbmp->reginfo[0].primary;
		*mp_mtxcountp = mp->htab_mutexes;
	}
	else
		*mp_mtxcountp = dbenv->mp_mtxcount;
	return 0;
}
/*
 * __memp_set_mp_mtxcount --
 *	DB_ENV->set_mp_mtxcount.
 */
int __memp_set_mp_mtxcount(DB_ENV * dbenv, uint32 mp_mtxcount)
{
	ENV * env = dbenv->env;
	ENV_NOT_CONFIGURED(env, env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mp_mtxcount");
	dbenv->mp_mtxcount = mp_mtxcount;
	return 0;
}
/*
 * __memp_nameop
 *	Remove or rename a file in the pool.
 *
 * XXX
 * Undocumented interface: DB private.
 */
int __memp_nameop(ENV * env, uint8 * fileid, const char * newname, const char * fullold, const char * fullnew, int inmem)
{
	DB_MPOOL * dbmp = 0;
	DB_MPOOL_HASH * hp;
	DB_MPOOL_HASH * nhp = 0;
	MPOOL * mp;
	MPOOLFILE * mfp = 0;
	roff_t newname_off;
	uint32 bucket;
	int locked = 0;
	int ret = 0;
	size_t nlen;
	void * p = 0;
#undef  op_is_remove
#define op_is_remove    (newname == NULL)
	COMPQUIET(bucket, 0);
	COMPQUIET(hp, 0);
	COMPQUIET(newname_off, 0);
	COMPQUIET(nlen, 0);
	if(!MPOOL_ON(env))
		goto fsop;
	dbmp = env->mp_handle;
	mp = (MPOOL *)dbmp->reginfo[0].primary;
	hp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
	if(!op_is_remove) {
		nlen = sstrlen(newname);
		if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL,  nlen+1, &newname_off, &p)) != 0)
			return ret;
		memcpy(p, newname, nlen+1);
	}
	/*
	 * Remove or rename a file that the mpool might know about.  We assume
	 * that the fop layer has the file locked for exclusive access, so we
	 * don't worry about locking except for the mpool mutexes.  Checkpoint
	 * can happen at any time, independent of file locking, so we have to
	 * do the actual unlink or rename system call while holding
	 * all affected buckets locked.
	 *
	 * If this is a rename and this is a memory file then we need
	 * to make sure that the new name does not exist.  Since we
	 * are locking two buckets lock them in ascending order.
	 */
	if(inmem) {
		DB_ASSERT(env, fullold != NULL);
		hp += FNBUCKET(fullold, sstrlen(fullold));
		if(!op_is_remove) {
			bucket = FNBUCKET(newname, nlen);
			nhp = (DB_MPOOL_HASH *)R_ADDR(dbmp->reginfo, mp->ftab);
			nhp += bucket;
		}
	}
	else
		hp += FNBUCKET(fileid, DB_FILE_ID_LEN);
	if(nhp && nhp < hp)
		MUTEX_LOCK(env, nhp->mtx_hash);
	MUTEX_LOCK(env, hp->mtx_hash);
	if(nhp && nhp > hp)
		MUTEX_LOCK(env, nhp->mtx_hash);
	locked = 1;
	if(!op_is_remove && inmem) {
		SH_TAILQ_FOREACH(mfp, &nhp->hash_bucket, q, __mpoolfile)
		if(!mfp->deadfile && mfp->no_backing_file && strcmp(newname, (const char *)R_ADDR(dbmp->reginfo, mfp->path_off)) == 0)
			break;
		if(mfp) {
			ret = EEXIST;
			goto err;
		}
	}
	//
	// Find the file -- if mpool doesn't know about this file, that may not be an error.
	//
	SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
		// Ignore non-active files
		if(mfp->deadfile || F_ISSET(mfp, MP_TEMP))
			continue;
		// Try to match on fileid
		if(memcmp(fileid, R_ADDR(dbmp->reginfo, mfp->fileid_off), DB_FILE_ID_LEN) != 0)
			continue;
		break;
	}
	if(!mfp) {
		if(inmem) {
			ret = ENOENT;
			goto err;
		}
		else
			goto fsop;
	}
	if(op_is_remove) {
		MUTEX_LOCK(env, mfp->mutex);
		//
		// In-memory dbs have an artificially incremented ref count so
		// they do not get reclaimed as long as they exist.  Since we
		// are now deleting the database, we need to dec that count.
		//
		if(mfp->no_backing_file)
			mfp->mpf_cnt--;
		mfp->deadfile = 1;
		MUTEX_UNLOCK(env, mfp->mutex);
	}
	else {
		//
		// Else, it's a rename.  We've allocated memory for the new
		// name.  Swap it with the old one.  If it's in memory we need to move it the right bucket.
		//
		p = R_ADDR(dbmp->reginfo, mfp->path_off);
		mfp->path_off = newname_off;
		if(inmem && hp != nhp) {
			DB_ASSERT(env, nhp != NULL);
			SH_TAILQ_REMOVE(&hp->hash_bucket, mfp, q, __mpoolfile);
			mfp->bucket = bucket;
			SH_TAILQ_INSERT_TAIL(&nhp->hash_bucket, mfp, q);
		}
	}
fsop:
	//
	// If this is a real file, then mfp could be NULL, because
	// mpool isn't turned on, and we still need to do the file ops.
	//
	if(mfp == NULL || !mfp->no_backing_file) {
		if(op_is_remove) {
			/*
			 * !!!
			 * Replication may ask us to unlink a file that's been
			 * renamed.  Don't complain if it doesn't exist.
			 */
			if((ret = __os_unlink(env, fullold, 0)) == ENOENT)
				ret = 0;
		}
		else {
			// Defensive only, fullnew should never be NULL.
			DB_ASSERT(env, fullnew != NULL);
			if(fullnew == NULL) {
				ret = EINVAL;
				goto err;
			}
			ret = __os_rename(env, fullold, fullnew, 1);
		}
	}
	/* Delete the memory we no longer need. */
err:
	if(p) {
		MPOOL_REGION_LOCK(env, &dbmp->reginfo[0]);
		__memp_free(&dbmp->reginfo[0], p);
		MPOOL_REGION_UNLOCK(env, &dbmp->reginfo[0]);
	}
	/* If we have buckets locked, unlock them when done moving files. */
	if(locked == 1) {
		MUTEX_UNLOCK(env, hp->mtx_hash);
		if(nhp && nhp != hp)
			MUTEX_UNLOCK(env, nhp->mtx_hash);
	}
	return ret;
}
/*
 * __memp_ftruncate __
 *	Truncate the file.
 */
int __memp_ftruncate(DB_MPOOLFILE * dbmfp, DB_TXN * txn, DB_THREAD_INFO * ip, db_pgno_t pgno, uint32 flags)
{
	void * pagep;
	db_pgno_t last_pgno, pg;
	ENV * env = dbmfp->env;
	MPOOLFILE * mfp = dbmfp->mfp;
	int ret = 0;
	MUTEX_LOCK(env, mfp->mutex);
	last_pgno = mfp->last_pgno;
	MUTEX_UNLOCK(env, mfp->mutex);
	if(pgno > last_pgno) {
		if(LF_ISSET(MP_TRUNC_RECOVER))
			return 0;
		__db_errx(env, DB_STR("3005", "Truncate beyond the end of file"));
		return EINVAL;
	}
	pg = pgno;
	if(!LF_ISSET(MP_TRUNC_NOCACHE))
		do {
			if(mfp->block_cnt == 0)
				break;
			if((ret = __memp_fget(dbmfp, &pg, ip, txn, DB_MPOOL_FREE, &pagep)) != 0)
				return ret;
		} while(pg++ < last_pgno);
	/*
	 * If we are aborting an extend of a file, the call to __os_truncate
	 * could extend the file if the new page(s) had not yet been
	 * written to disk.  We do not want to extend the file to pages
	 * whose log records are not yet flushed [#14031].  In addition if
	 * we are out of disk space we can generate an error [#12743].
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if(!F_ISSET(mfp, MP_TEMP) && !mfp->no_backing_file && pgno <= mfp->last_flushed_pgno)
#ifdef HAVE_FTRUNCATE
		ret = __os_truncate(env, dbmfp->fhp, pgno, mfp->pagesize);
#else
		ret = __db_zero_extend(env, dbmfp->fhp, pgno, mfp->last_pgno, mfp->pagesize);
#endif
	/*
	 * This set could race with another thread of control that extending
	 * the file.  It's not a problem because we should have the page
	 * locked at a higher level of the system.
	 */
	if(!ret) {
		mfp->last_pgno = pgno-1;
		SETMIN(mfp->last_flushed_pgno, mfp->last_pgno);
	}
	MUTEX_UNLOCK(env, mfp->mutex);
	return ret;
}

#ifdef HAVE_FTRUNCATE
/*
 * Support routines for maintaining a sorted freelist while we try to rearrange
 * and truncate the file.
 */
/*
 * __memp_alloc_freelist --
 *	Allocate mpool space for the freelist.
 */
int __memp_alloc_freelist(DB_MPOOLFILE * dbmfp, uint32 nelems, db_pgno_t ** listp)
{
	void * retp;
	int ret;
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOLFILE * mfp = dbmfp->mfp;
	*listp = NULL;
	/*
	 * These fields are protected because the database layer
	 * has the metapage locked while manipulating them.
	 */
	mfp->free_ref++;
	if(mfp->free_size != 0)
		return EBUSY;
	/* Allocate at least a few slots. */
	mfp->free_cnt = nelems;
	if(nelems == 0)
		nelems = 50;
	if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, nelems*sizeof(db_pgno_t), &mfp->free_list, &retp)) != 0)
		return ret;
	mfp->free_size = nelems*sizeof(db_pgno_t);
	*listp = (db_pgno_t *)retp;
	return 0;
}
/*
 * __memp_free_freelist --
 *	Free the list.
 */
int __memp_free_freelist(DB_MPOOLFILE * dbmfp)
{
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOLFILE * mfp = dbmfp->mfp;
	DB_ASSERT(env, mfp->free_ref > 0);
	if(--mfp->free_ref > 0)
		return 0;
	DB_ASSERT(env, mfp->free_size != 0);
	MPOOL_SYSTEM_LOCK(env);
	__memp_free(dbmp->reginfo, R_ADDR(dbmp->reginfo, mfp->free_list));
	MPOOL_SYSTEM_UNLOCK(env);
	mfp->free_cnt = 0;
	mfp->free_list = 0;
	mfp->free_size = 0;
	return 0;
}
/*
 * __memp_get_freelst --
 *	Return current list.
 */
int __memp_get_freelist(DB_MPOOLFILE * dbmfp, uint32 * nelemp, db_pgno_t ** listp)
{
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOLFILE * mfp = dbmfp->mfp;
	if(mfp->free_size == 0) {
		*nelemp = 0;
		*listp = NULL;
	}
	else {
		*nelemp = mfp->free_cnt;
		*listp = (db_pgno_t *)R_ADDR(dbmp->reginfo, mfp->free_list);
	}
	return 0;
}
/*
 * __memp_extend_freelist --
 *	Extend the list.
 */
int __memp_extend_freelist(DB_MPOOLFILE * dbmfp, uint32 count, db_pgno_t ** listp)
{
	int ret;
	size_t size;
	void * retp;
	ENV * env = dbmfp->env;
	DB_MPOOL * dbmp = env->mp_handle;
	MPOOLFILE * mfp = dbmfp->mfp;
	if(mfp->free_size == 0)
		return EINVAL;
	if(count*sizeof(db_pgno_t) > mfp->free_size) {
		size = (size_t)DB_ALIGN(count*sizeof(db_pgno_t), 512);
 #ifdef HAVE_MIXED_SIZE_ADDRESSING
		if(size >= 0xFFFFFFFF) {
			__db_errx(env, DB_STR("3006", "Can't get the required free size while" "operating in mixed-size-addressing mode"));
			return EINVAL;
		}
 #endif
		*listp = (db_pgno_t *)R_ADDR(dbmp->reginfo, mfp->free_list);
		if((ret = __memp_alloc(dbmp, dbmp->reginfo, NULL, size, &mfp->free_list, &retp)) != 0)
			return ret;
		mfp->free_size = (db_size_t)size;
		memcpy(retp, *listp, mfp->free_cnt*sizeof(db_pgno_t));
		MPOOL_SYSTEM_LOCK(env);
		__memp_free(dbmp->reginfo, *listp);
		MPOOL_SYSTEM_UNLOCK(env);
	}
	mfp->free_cnt = count;
	*listp = (db_pgno_t *)R_ADDR(dbmp->reginfo, mfp->free_list);
	return 0;
}
#endif
/*
 * __memp_set_last_pgno -- set the last page of the file
 */
int __memp_set_last_pgno(DB_MPOOLFILE * dbmfp, db_pgno_t pgno)
{
	MPOOLFILE * mfp = dbmfp->mfp;
	if(mfp->mpf_cnt == 1) {
		MUTEX_LOCK(dbmfp->env, mfp->mutex);
		if(mfp->mpf_cnt == 1)
			dbmfp->mfp->last_pgno = pgno;
		MUTEX_UNLOCK(dbmfp->env, mfp->mutex);
	}
	return 0;
}
