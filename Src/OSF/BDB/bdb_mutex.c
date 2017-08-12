// BDB_MUTEX.C
// See the file LICENSE for redistribution information.
// Copyright (c) 1999, 2011 Oracle and/or its affiliates.  All rights reserved.
//
#include "db_config.h"
#include "db_int.h"
#pragma hdrstop
#define LOAD_ACTUAL_MUTEX_CODE
//#include "db_int.h"
//#include "dbinc/atomic.h"
#include "dbinc/mutex_int.h" // This is where we load in the actual mutex declarations.
//
// STATICS
//
static db_size_t __mutex_align_size(ENV *);
static int __mutex_region_init(ENV *, DB_MUTEXMGR *);
static size_t __mutex_region_size(ENV *);
static size_t __mutex_region_max(ENV *);
//
static int __mutex_print_all(ENV*, uint32);
static const char * __mutex_print_id(int);
static int __mutex_print_stats(ENV*, uint32);
static void __mutex_print_summary(ENV *);
static int __mutex_stat(ENV*, DB_MUTEX_STAT**, uint32);
//
static inline int __db_tas_mutex_lock_int(ENV*, db_mutex_t, db_timeout_t, int);
static inline int __db_tas_mutex_readlock_int(ENV*, db_mutex_t, int);
/*
 * __mutex_alloc --
 *	Allocate a mutex from the mutex region.
 *
 * PUBLIC: int __mutex_alloc __P((ENV *, int, uint32, db_mutex_t *));
 */
int __mutex_alloc(ENV * env, int alloc_id, uint32 flags, db_mutex_t * indxp)
{
	/* The caller may depend on us to initialize. */
	*indxp = MUTEX_INVALID;
	/*
	 * If this is not an application lock, and we've turned off locking,
	 * or the ENV handle isn't thread-safe, and this is a thread lock
	 * or the environment isn't multi-process by definition, there's no
	 * need to mutex at all.
	 */
	if(alloc_id != MTX_APPLICATION && alloc_id != MTX_MUTEX_TEST && (F_ISSET(env->dbenv, DB_ENV_NOLOCKING) ||
	    (!F_ISSET(env, ENV_THREAD) && (LF_ISSET(DB_MUTEX_PROCESS_ONLY) || F_ISSET(env, ENV_PRIVATE)))))
		return 0;
	/* Private environments never share mutexes. */
	if(F_ISSET(env, ENV_PRIVATE))
		LF_SET(DB_MUTEX_PROCESS_ONLY);
	/*
	 * If we have a region in which to allocate the mutexes, lock it and do the allocation.
	 */
	if(!MUTEX_ON(env)) {
		__db_errx(env, DB_STR("2033", "Mutex allocated before mutex region."));
		return __env_panic(env, EINVAL);
	}
	return __mutex_alloc_int(env, 1, alloc_id, flags, indxp);
}
/*
 * __mutex_alloc_int --
 *	Internal routine to allocate a mutex.
 */
int __mutex_alloc_int(ENV * env, int locksys, int alloc_id, uint32 flags, db_mutex_t * indxp)
{
	DB_MUTEX * mutexp;
	db_mutex_t i;
	size_t len;
	uint32 cnt;
	DB_ENV * dbenv = env->dbenv;
	DB_MUTEXMGR * mtxmgr = env->mutex_handle;
	DB_MUTEXREGION * mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	int ret = 0;
	/*
	 * If we're not initializing the mutex region, then lock the region to
	 * allocate new mutexes.  Drop the lock before initializing the mutex,
	 * mutex initialization may require a system call.
	 */
	if(locksys)
		MUTEX_SYSTEM_LOCK(env);
	if(mtxregion->mutex_next == MUTEX_INVALID) {
		if(mtxregion->stat.st_mutex_max != 0 && mtxregion->stat.st_mutex_cnt >= mtxregion->stat.st_mutex_max) {
nomem:
			__db_errx(env, DB_STR("2034", "unable to allocate memory for mutex; resize mutex region"));
			if(locksys)
				MUTEX_SYSTEM_UNLOCK(env);
			return (ret == 0) ? ENOMEM : ret;
		}
		cnt = mtxregion->stat.st_mutex_cnt/2;
		SETMAX(cnt, 8);
		if(mtxregion->stat.st_mutex_max != 0 && mtxregion->stat.st_mutex_cnt+cnt > mtxregion->stat.st_mutex_max)
			cnt = mtxregion->stat.st_mutex_max-mtxregion->stat.st_mutex_cnt;
		if(F_ISSET(env, ENV_PRIVATE)) {
			F_SET(&mtxmgr->reginfo, REGION_TRACKED);
			while(__env_alloc(&mtxmgr->reginfo, (cnt*mtxregion->mutex_size)+mtxregion->stat.st_mutex_align, &i) != 0)
				if((cnt>>1) == 0)
					break;
			F_CLR(&mtxmgr->reginfo, REGION_TRACKED);
			i = (db_mutex_t)ALIGNP_INC(i, mtxregion->stat.st_mutex_align);
		}
		else {
			len = cnt*mtxregion->mutex_size;
			if((ret = __env_alloc_extend(&mtxmgr->reginfo, R_ADDR(&mtxmgr->reginfo, mtxregion->mutex_off_alloc), &len)) != 0)
				goto nomem;
			cnt = (uint32)(len/mtxregion->mutex_size);
			i = mtxregion->stat.st_mutex_cnt+1;
		}
		if(cnt == 0)
			goto nomem;
		mutexp = MUTEXP_SET(env, i);
		mtxregion->stat.st_mutex_free = cnt;
		mtxregion->mutex_next = i;
		mtxregion->stat.st_mutex_cnt += cnt;
		while(--cnt > 0) {
			mutexp->flags = 0;
			if(F_ISSET(env, ENV_PRIVATE))
				mutexp->mutex_next_link = (uintptr_t)(mutexp+1);
			else
				mutexp->mutex_next_link = ++i;
			mutexp++;
		}
		mutexp->flags = 0;
		mutexp->mutex_next_link = MUTEX_INVALID;
	}
	*indxp = mtxregion->mutex_next;
	mutexp = MUTEXP_SET(env, *indxp);
	DB_ASSERT(env, ((uintptr_t)mutexp&(dbenv->mutex_align-1)) == 0);
	mtxregion->mutex_next = mutexp->mutex_next_link;

	--mtxregion->stat.st_mutex_free;
	++mtxregion->stat.st_mutex_inuse;
	if(mtxregion->stat.st_mutex_inuse > mtxregion->stat.st_mutex_inuse_max)
		mtxregion->stat.st_mutex_inuse_max = mtxregion->stat.st_mutex_inuse;
	if(locksys)
		MUTEX_SYSTEM_UNLOCK(env);
	/* Initialize the mutex. */
	memzero(mutexp, sizeof(*mutexp));
	F_SET(mutexp, DB_MUTEX_ALLOCATED|LF_ISSET(DB_MUTEX_LOGICAL_LOCK|DB_MUTEX_PROCESS_ONLY|DB_MUTEX_SHARED));
	/*
	 * If the mutex is associated with a single process, set the process
	 * ID.  If the application ever calls DbEnv::failchk, we'll need the
	 * process ID to know if the mutex is still in use.
	 */
	if(LF_ISSET(DB_MUTEX_PROCESS_ONLY))
		dbenv->thread_id(dbenv, &mutexp->pid, 0);
#ifdef HAVE_STATISTICS
	mutexp->alloc_id = alloc_id;
#else
	COMPQUIET(alloc_id, 0);
#endif
	if((ret = __mutex_init(env, *indxp, flags)) != 0)
		__mutex_free_int(env, locksys, indxp);
	return ret;
}
/*
 * __mutex_free --
 *	Free a mutex.
 *
 * PUBLIC: int __mutex_free __P((ENV *, db_mutex_t *));
 */
int FASTCALL __mutex_free(ENV * env, db_mutex_t * indxp)
{
	/*
	 * There is no explicit ordering in how the regions are cleaned up
	 * up and/or discarded when an environment is destroyed (either a
	 * private environment is closed or a public environment is removed).
	 * The way we deal with mutexes is to clean up all remaining mutexes
	 * when we close the mutex environment (because we have to be able to
	 * do that anyway, after a crash), which means we don't have to deal
	 * with region cleanup ordering on normal environment destruction.
	 * All that said, what it really means is we can get here without a
	 * mpool region.  It's OK, the mutex has been, or will be, destroyed.
	 *
	 * If the mutex has never been configured, we're done.
	 */
	return (!MUTEX_ON(env) || *indxp == MUTEX_INVALID) ? 0 : __mutex_free_int(env, 1, indxp);
}
/*
 * __mutex_free_int --
 *	Internal routine to free a mutex.
 *
 * PUBLIC: int __mutex_free_int __P((ENV *, int, db_mutex_t *));
 */
int __mutex_free_int(ENV * env, int locksys, db_mutex_t * indxp)
{
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	int ret;
	db_mutex_t mutex = *indxp;
	*indxp = MUTEX_INVALID;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(env, mutex);
	DB_ASSERT(env, F_ISSET(mutexp, DB_MUTEX_ALLOCATED));
	F_CLR(mutexp, DB_MUTEX_ALLOCATED);
	ret = __mutex_destroy(env, mutex);
	if(locksys)
		MUTEX_SYSTEM_LOCK(env);
	/* Link the mutex on the head of the free list. */
	mutexp->mutex_next_link = mtxregion->mutex_next;
	mtxregion->mutex_next = mutex;
	++mtxregion->stat.st_mutex_free;
	--mtxregion->stat.st_mutex_inuse;
	if(locksys)
		MUTEX_SYSTEM_UNLOCK(env);
	return ret;
}
/*
 * __mutex_refresh --
 *	Reinitialize a mutex, if we are not sure of its state.
 *
 * PUBLIC: int __mutex_refresh __P((ENV *, db_mutex_t));
 */
int __mutex_refresh(ENV * env, db_mutex_t mutex)
{
	int ret;
	DB_MUTEX * mutexp = MUTEXP_SET(env, mutex);
	uint32 flags = mutexp->flags;
	if((ret = __mutex_destroy(env, mutex)) == 0) {
		memzero(mutexp, sizeof(*mutexp));
		F_SET(mutexp, DB_MUTEX_ALLOCATED|LF_ISSET(DB_MUTEX_LOGICAL_LOCK|DB_MUTEX_PROCESS_ONLY|DB_MUTEX_SHARED));
		LF_CLR(DB_MUTEX_LOCKED);
		ret = __mutex_init(env, mutex, flags);
	}
	return ret;
}
//
// __mut_failchk --
// Check for mutexes held by dead processes.
// 
// PUBLIC: int __mut_failchk(ENV *);
// 
int __mut_failchk(ENV * env)
{
	int ret = 0;
	if(!F_ISSET(env, ENV_PRIVATE)) {
		char buf[DB_THREADID_STRLEN];
		DB_ENV * dbenv = env->dbenv;
		DB_MUTEXMGR * mtxmgr = env->mutex_handle;
		DB_MUTEXREGION * mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
		DB_MUTEX * mutexp;
		MUTEX_SYSTEM_LOCK(env);
		for(db_mutex_t i = 1; i <= mtxregion->stat.st_mutex_cnt; ++i, ++mutexp) {
			mutexp = MUTEXP_SET(env, i);
			//
			// We're looking for per-process mutexes where the process has died.
			//
			if(!F_ISSET(mutexp, DB_MUTEX_ALLOCATED) || !F_ISSET(mutexp, DB_MUTEX_PROCESS_ONLY))
				continue;
			/*
			 * The thread that allocated the mutex may have exited, but
			 * we cannot reclaim the mutex if the process is still alive.
			 */
			if(dbenv->is_alive(dbenv, mutexp->pid, 0, DB_MUTEX_PROCESS_ONLY))
				continue;
			__db_msg(env, DB_STR_A("2017", "Freeing mutex for process: %s", "%s"), dbenv->thread_id_string(dbenv, mutexp->pid, 0, buf));
			// Unlock and free the mutex. 
			if(F_ISSET(mutexp, DB_MUTEX_LOCKED))
				MUTEX_UNLOCK(env, i);
			if((ret = __mutex_free_int(env, 0, &i)) != 0)
				break;
		}
		MUTEX_SYSTEM_UNLOCK(env);
	}
	return ret;
}
//
// Open a mutex region.
//
int __mutex_open(ENV * env, int create_ok)
{
	DB_ENV * dbenv;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	size_t size;
	uint32 cpu_count;
	int ret;
#ifndef HAVE_ATOMIC_SUPPORT
	uint i;
#endif
	dbenv = env->dbenv;
	if(dbenv->mutex_max == 0 && dbenv->mutex_cnt == 0 && dbenv->mutex_inc == 0 && F_ISSET(env, ENV_PRIVATE|ENV_THREAD) == ENV_PRIVATE)
		return 0;
	/*
	 * Initialize the ENV handle information if not already initialized.
	 *
	 * Align mutexes on the byte boundaries specified by the application.
	 */
	SETIFZ(dbenv->mutex_align, MUTEX_ALIGN);
	if(dbenv->mutex_tas_spins == 0) {
		cpu_count = __os_cpu_count();
		if((ret = __mutex_set_tas_spins(dbenv, cpu_count == 1 ? cpu_count : cpu_count*MUTEX_SPINS_PER_PROCESSOR)) != 0)
			return ret;
	}
	/*
	 * If the user didn't set an absolute value on the number of mutexes
	 * we'll need, figure it out.  We're conservative in our allocation,
	 * we need mutexes for DB handles, group-commit queues and other things
	 * applications allocate at run-time.  The application may have kicked
	 * up our count to allocate its own mutexes, add that in.
	 */
	if(dbenv->mutex_cnt == 0 && F_ISSET(env, ENV_PRIVATE|ENV_THREAD) != ENV_PRIVATE)
		dbenv->mutex_cnt = __lock_region_mutex_count(env) + __log_region_mutex_count(env) + __memp_region_mutex_count(env) + __txn_region_mutex_count(env);
	if(dbenv->mutex_max != 0 && dbenv->mutex_cnt > dbenv->mutex_max)
		dbenv->mutex_cnt = dbenv->mutex_max;
	/* Create/initialize the mutex manager structure. */
	if((ret = __os_calloc(env, 1, sizeof(DB_MUTEXMGR), &mtxmgr)) != 0)
		return ret;
	/* Join/create the mutex region. */
	mtxmgr->reginfo.env = env;
	mtxmgr->reginfo.type = REGION_TYPE_MUTEX;
	mtxmgr->reginfo.id = INVALID_REGION_ID;
	mtxmgr->reginfo.flags = REGION_JOIN_OK;
	size = __mutex_region_size(env);
	if(create_ok)
		F_SET(&mtxmgr->reginfo, REGION_CREATE_OK);
	if((ret = __env_region_attach(env, &mtxmgr->reginfo, size, size+__mutex_region_max(env))) != 0)
		goto err;
	/* If we created the region, initialize it. */
	if(F_ISSET(&mtxmgr->reginfo, REGION_CREATE))
		if((ret = __mutex_region_init(env, mtxmgr)) != 0)
			goto err;
	/* Set the local addresses. */
	mtxregion = (DB_MUTEXREGION *)(mtxmgr->reginfo.primary = R_ADDR(&mtxmgr->reginfo, mtxmgr->reginfo.rp->primary));
	mtxmgr->mutex_array = R_ADDR(&mtxmgr->reginfo, mtxregion->mutex_off);
	env->mutex_handle = mtxmgr;
#ifndef HAVE_ATOMIC_SUPPORT
	/* If necessary allocate the atomic emulation mutexes.  */
	if(F_ISSET(&mtxmgr->reginfo, REGION_CREATE))
		for(i = 0; i != MAX_ATOMIC_MUTEXES; i++)
			if((ret = __mutex_alloc_int(env, 0, MTX_ATOMIC_EMULATION, 0, &mtxregion->mtx_atomic[i])) != 0)
				return ret;
#endif
	return 0;
err:
	env->mutex_handle = NULL;
	if(mtxmgr->reginfo.addr)
		__env_region_detach(env, &mtxmgr->reginfo, 0);
	__os_free(env, mtxmgr);
	return ret;
}
//
// __mutex_region_init --
// Initialize a mutex region in shared memory.
// 
static int __mutex_region_init(ENV * env, DB_MUTEXMGR * mtxmgr)
{
	DB_MUTEX * mutexp;
	DB_MUTEXREGION * mtxregion;
	db_mutex_t mutex;
	int ret;
	void * mutex_array;
	DB_ENV * dbenv = env->dbenv;
	COMPQUIET(mutexp, 0);
	if((ret = __env_alloc(&mtxmgr->reginfo, sizeof(DB_MUTEXREGION), &mtxmgr->reginfo.primary)) != 0) {
		__db_errx(env, DB_STR("2013", "Unable to allocate memory for the mutex region"));
		return ret;
	}
	mtxmgr->reginfo.rp->primary = R_OFFSET(&mtxmgr->reginfo, mtxmgr->reginfo.primary);
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	memzero(mtxregion, sizeof(*mtxregion));
	mtxregion->mutex_size = __mutex_align_size(env);
	mtxregion->stat.st_mutex_align = dbenv->mutex_align;
	SETIFZ(dbenv->mutex_cnt, 1);
	mtxregion->stat.st_mutex_init = mtxregion->stat.st_mutex_cnt = dbenv->mutex_cnt;
	mtxregion->stat.st_mutex_max = dbenv->mutex_max;
	if(mtxregion->stat.st_mutex_max != 0)
		mtxregion->stat.st_mutex_max += dbenv->mutex_inc;
	mtxregion->stat.st_mutex_tas_spins = dbenv->mutex_tas_spins;
	/*
	 * Get a chunk of memory to be used for the mutexes themselves.  Each
	 * piece of the memory must be properly aligned, and that alignment
	 * may be more restrictive than the memory alignment returned by the
	 * underlying allocation code.  We already know how much memory each
	 * mutex in the array will take up, but we need to offset the first
	 * mutex in the array so the array begins properly aligned.
	 *
	 * The OOB mutex (MUTEX_INVALID) is 0.  To make this work, we ignore
	 * the first allocated slot when we build the free list.  We have to
	 * correct the count by 1 here, though, otherwise our counter will be
	 * off by 1.
	 */
	if((ret = __env_alloc(&mtxmgr->reginfo, mtxregion->stat.st_mutex_align+(mtxregion->stat.st_mutex_cnt+1)*mtxregion->mutex_size, &mutex_array)) != 0) {
		__db_errx(env, DB_STR("2014", "Unable to allocate memory for mutexes from the region"));
		return ret;
	}
	mtxregion->mutex_off_alloc = R_OFFSET(&mtxmgr->reginfo, mutex_array);
	mutex_array = ALIGNP_INC(mutex_array, mtxregion->stat.st_mutex_align);
	mtxregion->mutex_off = R_OFFSET(&mtxmgr->reginfo, mutex_array);
	mtxmgr->mutex_array = mutex_array;
	/*
	 * Put the mutexes on a free list and clear the allocated flag.
	 *
	 * The OOB mutex (MUTEX_INVALID) is 0, skip it.
	 *
	 * The comparison is <, not <=, because we're looking ahead one
	 * in each link.
	 */
	env->mutex_handle = mtxmgr;
	if(F_ISSET(env, ENV_PRIVATE)) {
		mutexp = (DB_MUTEX *)mutex_array;
		mutexp++;
		mutexp = (DB_MUTEX *)ALIGNP_INC(mutexp, mtxregion->stat.st_mutex_align);
		mtxregion->mutex_next = (db_mutex_t)mutexp;
	}
	else {
		mtxregion->mutex_next = 1;
		mutexp = MUTEXP_SET(env, 1);
	}
	for(mutex = 1; mutex < mtxregion->stat.st_mutex_cnt; ++mutex) {
		mutexp->flags = 0;
		mutexp->mutex_next_link = F_ISSET(env, ENV_PRIVATE) ? (db_mutex_t)(mutexp+1) : (mutex+1);
		mutexp++;
		mutexp = (DB_MUTEX *)ALIGNP_INC(mutexp, mtxregion->stat.st_mutex_align);
	}
	mutexp->flags = 0;
	mutexp->mutex_next_link = MUTEX_INVALID;
	mtxregion->stat.st_mutex_free = mtxregion->stat.st_mutex_cnt;
	mtxregion->stat.st_mutex_inuse = mtxregion->stat.st_mutex_inuse_max = 0;
	if((ret = __mutex_alloc(env, MTX_MUTEX_REGION, 0, &mutex)) != 0)
		return ret;
	mtxmgr->reginfo.mtx_alloc = mtxregion->mtx_region = mutex;

	/*
	 * This is the first place we can test mutexes and we need to
	 * know if they're working.  (They CAN fail, for example on
	 * SunOS, when using fcntl(2) for locking and using an
	 * in-memory filesystem as the database environment directory.
	 * But you knew that, I'm sure -- it probably wasn't worth
	 * mentioning.)
	 */
	mutex = MUTEX_INVALID;
	if((ret = __mutex_alloc(env, MTX_MUTEX_TEST, 0, &mutex) != 0) ||
	   (ret = __mutex_lock(env, mutex)) != 0 ||
	   (ret = __mutex_unlock(env, mutex)) != 0 ||
	   (ret = __mutex_trylock(env, mutex)) != 0 ||
	   (ret = __mutex_unlock(env, mutex)) != 0 ||
	   (ret = __mutex_free(env, &mutex)) != 0) {
		__db_errx(env, DB_STR("2015", "Unable to acquire/release a mutex; check configuration"));
		return ret;
	}
#ifdef HAVE_SHARED_LATCHES
	if((ret = __mutex_alloc(env, MTX_MUTEX_TEST, DB_MUTEX_SHARED, &mutex) != 0) ||
	   (ret = __mutex_lock(env, mutex)) != 0 ||
	   (ret = __mutex_tryrdlock(env, mutex)) != DB_LOCK_NOTGRANTED ||
	   (ret = __mutex_unlock(env, mutex)) != 0 ||
	   (ret = __mutex_rdlock(env, mutex)) != 0 ||
	   (ret = __mutex_rdlock(env, mutex)) != 0 ||
	   (ret = __mutex_unlock(env, mutex)) != 0 ||
	   (ret = __mutex_unlock(env, mutex)) != 0 ||
	   (ret = __mutex_free(env, &mutex)) != 0) {
		__db_errx(env, DB_STR("2016", "Unable to acquire/release a shared latch; check configuration"));
		return ret;
	}
#endif
	return 0;
}
/*
 * __mutex_env_refresh --
 *	Clean up after the mutex region on a close or failed open.
 *
 * PUBLIC: int __mutex_env_refresh(ENV *);
 */
int __mutex_env_refresh(ENV * env)
{
	int ret;
	DB_MUTEXMGR * mtxmgr = env->mutex_handle;
	REGINFO * reginfo = &mtxmgr->reginfo;
	DB_MUTEXREGION * mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if(F_ISSET(env, ENV_PRIVATE)) {
		reginfo->mtx_alloc = MUTEX_INVALID;
#ifdef HAVE_MUTEX_SYSTEM_RESOURCES
		/*
		 * If destroying the mutex region, return any system resources
		 * to the system.
		 */
		__mutex_resource_return(env, reginfo);
#endif
		/* Discard the mutex array. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, mtxregion->mutex_off_alloc));
	}
	/* Detach from the region. */
	ret = __env_region_detach(env, reginfo, 0);
	__os_free(env, mtxmgr);
	env->mutex_handle = NULL;
	return ret;
}
/*
 *	Return how much memory each mutex will take up if an array of them
 *	are to be properly aligned, individually, within the array.
 */
static db_size_t __mutex_align_size(ENV * env)
{
	DB_ENV * dbenv = env->dbenv;
	return (db_size_t)DB_ALIGN(sizeof(DB_MUTEX), dbenv->mutex_align);
}
/*
 *	 Return the amount of space needed for the mutex region.
 */
static size_t __mutex_region_size(ENV * env)
{
	DB_ENV * dbenv = env->dbenv;
	size_t s = sizeof(DB_MUTEXMGR)+1024;
	/* We discard one mutex for the OOB slot. */
	s += __env_alloc_size((dbenv->mutex_cnt+1)*__mutex_align_size(env));
	return s;
}
/*
 *	 Return the amount of space needed to reach the maximum size.
 */
static size_t __mutex_region_max(ENV * env)
{
	uint32 max;
	DB_ENV * dbenv = env->dbenv;
	if((max = dbenv->mutex_max) == 0) {
		if(F_ISSET(env, ENV_PRIVATE|ENV_THREAD) == ENV_PRIVATE)
			max = dbenv->mutex_inc+1;
		else
			max = __lock_region_mutex_max(env)+__txn_region_mutex_max(env)+__log_region_mutex_max(env)+dbenv->mutex_inc+100;
	}
	else if(max <= dbenv->mutex_cnt)
		return 0;
	else
		max -= dbenv->mutex_cnt;
	return __env_alloc_size(max*__mutex_align_size(env));
}

#ifdef  HAVE_MUTEX_SYSTEM_RESOURCES
/*
 * __mutex_resource_return
 *	Return any system-allocated mutex resources to the system.
 *
 * PUBLIC: void __mutex_resource_return __P((ENV *, REGINFO *));
 */
void __mutex_resource_return(ENV * env, REGINFO * infop)
{
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr, mtxmgr_st;
	DB_MUTEXREGION * mtxregion;
	db_mutex_t i, indx;
	void * orig_handle, * chunk;
	uintmax_t size;
	/*
	 * This routine is called in two cases: when discarding the regions
	 * from a previous Berkeley DB run, during recovery, and two, when
	 * discarding regions as we shut down the database environment.
	 *
	 * Walk the list of mutexes and destroy any live ones.
	 *
	 * This is just like joining a region -- the REGINFO we're handed is
	 * the same as the one returned by __env_region_attach(), all we have
	 * to do is fill in the links.
	 *
	 * !!!
	 * The region may be corrupted, of course.  We're safe because the
	 * only things we look at are things that are initialized when the
	 * region is created, and never modified after that.
	 */
	memzero(&mtxmgr_st, sizeof(mtxmgr_st));
	mtxmgr = &mtxmgr_st;
	mtxmgr->reginfo = *infop;
	mtxregion = mtxmgr->reginfo.primary = R_ADDR(&mtxmgr->reginfo, mtxmgr->reginfo.rp->primary);
	mtxmgr->mutex_array = R_ADDR(&mtxmgr->reginfo, mtxregion->mutex_off);
	/*
	 * This is a little strange, but the mutex_handle is what all of the
	 * underlying mutex routines will use to determine if they should do
	 * any work and to find their information.  Save/restore the handle
	 * around the work loop.
	 *
	 * The OOB mutex (MUTEX_INVALID) is 0, skip it.
	 */
	orig_handle = env->mutex_handle;
	env->mutex_handle = mtxmgr;
	if(F_ISSET(env, ENV_PRIVATE)) {
		mutexp = (DB_MUTEX *)mtxmgr->mutex_array+1;
		chunk = NULL;
		size = __env_elem_size(env, (void *)mtxregion->mutex_off_alloc);
		size -= sizeof(*mutexp);
	}
	else
		mutexp = MUTEXP_SET(env, 1);
	for(i = 1; i <= mtxregion->stat.st_mutex_cnt; ++i) {
		if(F_ISSET(env, ENV_PRIVATE))
			indx = (db_mutex_t)mutexp;
		else
			indx = i;
		if(F_ISSET(mutexp, DB_MUTEX_ALLOCATED))
			__mutex_destroy(env, indx);
		mutexp++;
		if(F_ISSET(env, ENV_PRIVATE) && (size -= sizeof(*mutexp)) < sizeof(*mutexp)) {
			mutexp = __env_get_chunk(&mtxmgr->reginfo, &chunk, &size);
			mutexp = ALIGNP_INC(mutexp, mtxregion->stat.st_mutex_align);
		}
	}
	env->mutex_handle = orig_handle;
}
#endif

#ifdef HAVE_STATISTICS
/*
 * __mutex_stat_pp --
 *	ENV->mutex_stat pre/post processing.
 *
 * PUBLIC: int __mutex_stat_pp __P((DB_ENV *, DB_MUTEX_STAT **, uint32));
 */
int __mutex_stat_pp(DB_ENV * dbenv, DB_MUTEX_STAT ** statp, uint32 flags)
{
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mutex_handle, "DB_ENV->mutex_stat", DB_INIT_MUTEX);
	if((ret = __db_fchk(env, "DB_ENV->mutex_stat", flags, DB_STAT_CLEAR)) != 0)
		return ret;
	{
		DB_THREAD_INFO * ip;
		ENV_ENTER(env, ip);
		REPLICATION_WRAP(env, (__mutex_stat(env, statp, flags)), 0, ret);
		ENV_LEAVE(env, ip);
	}
	return ret;
}
/*
 * __mutex_stat --
 *	ENV->mutex_stat.
 */
static int __mutex_stat(ENV * env, DB_MUTEX_STAT ** statp, uint32 flags)
{
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	DB_MUTEX_STAT * stats;
	int ret;
	*statp = NULL;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	if((ret = __os_umalloc(env, sizeof(DB_MUTEX_STAT), &stats)) != 0)
		return ret;
	MUTEX_SYSTEM_LOCK(env);
	/*
	 * Most fields are maintained in the underlying region structure.
	 * Region size and region mutex are not.
	 */
	*stats = mtxregion->stat;
	stats->st_regsize = mtxmgr->reginfo.rp->size;
	stats->st_regmax = mtxmgr->reginfo.rp->max;
	__mutex_set_wait_info(env, mtxregion->mtx_region, &stats->st_region_wait, &stats->st_region_nowait);
	if(LF_ISSET(DB_STAT_CLEAR))
		__mutex_clear(env, mtxregion->mtx_region);
	MUTEX_SYSTEM_UNLOCK(env);
	*statp = stats;
	return 0;
}
/*
 * __mutex_stat_print_pp --
 *	ENV->mutex_stat_print pre/post processing.
 *
 * PUBLIC: int __mutex_stat_print_pp(DB_ENV *, uint32);
 */
int __mutex_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	ENV_REQUIRES_CONFIG(env, env->mutex_handle, "DB_ENV->mutex_stat_print", DB_INIT_MUTEX);
	if((ret = __db_fchk(env, "DB_ENV->mutex_stat_print", flags, DB_STAT_ALL|DB_STAT_ALLOC|DB_STAT_CLEAR)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__mutex_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __mutex_stat_print
 *	ENV->mutex_stat_print method.
 *
 * PUBLIC: int __mutex_stat_print __P((ENV *, uint32));
 */
int __mutex_stat_print(ENV * env, uint32 flags)
{
	int ret;
	uint32 orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR|DB_STAT_SUBSYSTEM);
	if(flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __mutex_print_stats(env, orig_flags);
		__mutex_print_summary(env);
		if(flags == 0 || ret != 0)
			return ret;
	}
	if(LF_ISSET(DB_STAT_ALL))
		ret = __mutex_print_all(env, orig_flags);
	return 0;
}

static void __mutex_print_summary(ENV * env)
{
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	void * chunk;
	db_mutex_t i;
	uint32 counts[MTX_MAX_ENTRY+2];
	uintmax_t size;
	int alloc_id;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	memzero(counts, sizeof(counts));
	size = 0;
	if(F_ISSET(env, ENV_PRIVATE)) {
		mutexp = (DB_MUTEX *)mtxmgr->mutex_array+1;
		chunk = NULL;
		size = __env_elem_size(env, ROFF_TO_P(mtxregion->mutex_off_alloc));
		size -= sizeof(*mutexp);
	}
	else
		mutexp = MUTEXP_SET(env, 1);
	for(i = 1; i <= mtxregion->stat.st_mutex_cnt; ++i) {
		if(!F_ISSET(mutexp, DB_MUTEX_ALLOCATED))
			counts[0]++;
		else if(mutexp->alloc_id > MTX_MAX_ENTRY)
			counts[MTX_MAX_ENTRY+1]++;
		else
			counts[mutexp->alloc_id]++;
		mutexp++;
		if(F_ISSET(env, ENV_PRIVATE) && (size -= sizeof(*mutexp)) < sizeof(*mutexp)) {
			mutexp = (DB_MUTEX *)__env_get_chunk(&mtxmgr->reginfo, &chunk, &size);
			mutexp = (DB_MUTEX *)ALIGNP_INC(mutexp, mtxregion->stat.st_mutex_align);
		}
	}
	__db_msg(env, "Mutex counts");
	__db_msg(env, "%d\tUnallocated", counts[0]);
	for(alloc_id = 1; alloc_id <= MTX_TXN_REGION+1; alloc_id++)
		if(counts[alloc_id] != 0)
			__db_msg(env, "%lu\t%s", (ulong)counts[alloc_id], __mutex_print_id(alloc_id));
}
/*
 * __mutex_print_stats --
 *	Display default mutex region statistics.
 */
static int __mutex_print_stats(ENV * env, uint32 flags)
{
	DB_MUTEX_STAT * sp;
	int ret;
	if((ret = __mutex_stat(env, &sp, LF_ISSET(DB_STAT_CLEAR))) != 0)
		return ret;
	if(LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default mutex region information:");
	__db_dlbytes(env, "Mutex region size", (ulong)0, (ulong)0, (ulong)sp->st_regsize);
	__db_dlbytes(env, "Mutex region max size", (ulong)0, (ulong)0, (ulong)sp->st_regmax);
	__db_dl_pct(env, "The number of region locks that required waiting", (ulong)sp->st_region_wait, DB_PCT(sp->st_region_wait, sp->st_region_wait+sp->st_region_nowait), 0);
	STAT_ULONG("Mutex alignment", sp->st_mutex_align);
	STAT_ULONG("Mutex test-and-set spins", sp->st_mutex_tas_spins);
	STAT_ULONG("Mutex initial count", sp->st_mutex_init);
	STAT_ULONG("Mutex total count", sp->st_mutex_cnt);
	STAT_ULONG("Mutex max count", sp->st_mutex_max);
	STAT_ULONG("Mutex free count", sp->st_mutex_free);
	STAT_ULONG("Mutex in-use count", sp->st_mutex_inuse);
	STAT_ULONG("Mutex maximum in-use count", sp->st_mutex_inuse_max);
	__os_ufree(env, sp);
	return 0;
}
/*
 * __mutex_print_all --
 *	Display debugging mutex region statistics.
 */
static int __mutex_print_all(ENV * env, uint32 flags)
{
	static const FN fn[] = {
		{ DB_MUTEX_ALLOCATED,           "alloc" },
		{ DB_MUTEX_LOCKED,              "locked" },
		{ DB_MUTEX_LOGICAL_LOCK,        "logical" },
		{ DB_MUTEX_PROCESS_ONLY,        "process-private" },
		{ DB_MUTEX_SELF_BLOCK,          "self-block" },
		{ 0,                            NULL }
	};
	DB_MSGBUF mb, * mbp;
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	db_mutex_t i;
	uintmax_t size;
	void * chunk;
	DB_MSGBUF_INIT(&mb);
	mbp = &mb;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	__db_print_reginfo(env, &mtxmgr->reginfo, "Mutex", flags);
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_MUTEXREGION structure:");
	__mutex_print_debug_single(env, "DB_MUTEXREGION region mutex", mtxregion->mtx_region, flags);
	STAT_ULONG("Size of the aligned mutex", mtxregion->mutex_size);
	STAT_ULONG("Next free mutex", mtxregion->mutex_next);
	/*
	 * The OOB mutex (MUTEX_INVALID) is 0, skip it.
	 *
	 * We're not holding the mutex region lock, so we're racing threads of
	 * control allocating mutexes.  That's OK, it just means we display or
	 * clear statistics while mutexes are moving.
	 */
	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "mutex\twait/nowait, pct wait, holder, flags");
	size = 0;
	if(F_ISSET(env, ENV_PRIVATE)) {
		mutexp = (DB_MUTEX *)mtxmgr->mutex_array+1;
		chunk = NULL;
		size = __env_elem_size(env, ROFF_TO_P(mtxregion->mutex_off_alloc));
		size -= sizeof(*mutexp);
	}
	else
		mutexp = MUTEXP_SET(env, 1);
	for(i = 1; i <= mtxregion->stat.st_mutex_cnt; ++i) {
		if(!F_ISSET(mutexp, DB_MUTEX_ALLOCATED))
			continue;
		__db_msgadd(env, mbp, "%5lu\t", (ulong)i);
		__mutex_print_debug_stats(env, mbp, F_ISSET(env, ENV_PRIVATE) ? (db_mutex_t)mutexp : i, flags);
		if(mutexp->alloc_id != 0)
			__db_msgadd(env, mbp, ", %s", __mutex_print_id(mutexp->alloc_id));
		__db_prflags(env, mbp, mutexp->flags, fn, " (", ")");
		DB_MSGBUF_FLUSH(env, mbp);
		mutexp++;
		if(F_ISSET(env, ENV_PRIVATE) && (size -= sizeof(*mutexp)) < sizeof(*mutexp)) {
			mutexp = (DB_MUTEX *)__env_get_chunk(&mtxmgr->reginfo, &chunk, &size);
			mutexp = (DB_MUTEX *)ALIGNP_INC(mutexp, mtxregion->stat.st_mutex_align);
		}
	}
	return 0;
}
/*
 * __mutex_print_debug_single --
 *	Print mutex internal debugging statistics for a single mutex on a
 *	single output line.
 */
void __mutex_print_debug_single(ENV * env, const char * tag, db_mutex_t mutex, uint32 flags)
{
	DB_MSGBUF mb, * mbp;
	DB_MSGBUF_INIT(&mb);
	mbp = &mb;
	if(LF_ISSET(DB_STAT_SUBSYSTEM))
		LF_CLR(DB_STAT_CLEAR);
	__db_msgadd(env, mbp, "%lu\t%s ", (ulong)mutex, tag);
	__mutex_print_debug_stats(env, mbp, mutex, flags);
	DB_MSGBUF_FLUSH(env, mbp);
}
/*
 * __mutex_print_debug_stats --
 *	Print mutex internal debugging statistics, that is, the statistics
 *	in the [] square brackets.
 */
void __mutex_print_debug_stats(ENV * env, DB_MSGBUF * mbp, db_mutex_t mutex, uint32 flags)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	ulong value;
	char buf[DB_THREADID_STRLEN];
 #if defined(HAVE_SHARED_LATCHES) && (defined(HAVE_MUTEX_HYBRID) || !defined(HAVE_MUTEX_PTHREADS))
	int sharecount;
 #endif
	if(mutex == MUTEX_INVALID) {
		__db_msgadd(env, mbp, "[!Set]");
		return;
	}
	dbenv = env->dbenv;
	mutexp = MUTEXP_SET(env, mutex);
	__db_msgadd(env, mbp, "[");
	if((value = mutexp->mutex_set_wait) < 10000000)
		__db_msgadd(env, mbp, "%lu", value);
	else
		__db_msgadd(env, mbp, "%luM", value/1000000);
	if((value = mutexp->mutex_set_nowait) < 10000000)
		__db_msgadd(env, mbp, "/%lu", value);
	else
		__db_msgadd(env, mbp, "/%luM", value/1000000);
	__db_msgadd(env, mbp, " %d%% ", DB_PCT(mutexp->mutex_set_wait, mutexp->mutex_set_wait+mutexp->mutex_set_nowait));

 #if defined(HAVE_SHARED_LATCHES)
	if(F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		__db_msgadd(env, mbp, " rd ");
		if((value = mutexp->mutex_set_rd_wait) < 10000000)
			__db_msgadd(env, mbp, "%lu", value);
		else
			__db_msgadd(env, mbp, "%luM", value/1000000);
		if((value = mutexp->mutex_set_rd_nowait) < 10000000)
			__db_msgadd(env, mbp, "/%lu", value);
		else
			__db_msgadd(env, mbp, "/%luM", value/1000000);
		__db_msgadd(env, mbp, " %d%% ", DB_PCT(mutexp->mutex_set_rd_wait, mutexp->mutex_set_rd_wait+mutexp->mutex_set_rd_nowait));
	}
 #endif
	if(F_ISSET(mutexp, DB_MUTEX_LOCKED))
		__db_msgadd(env, mbp, "%s]", dbenv->thread_id_string(dbenv, mutexp->pid, mutexp->tid, buf));
	/* Pthreads-based shared latches do not expose the share count. */
 #if defined(HAVE_SHARED_LATCHES) && (defined(HAVE_MUTEX_HYBRID) || \
	!defined(HAVE_MUTEX_PTHREADS))
	else if(F_ISSET(mutexp, DB_MUTEX_SHARED) && (sharecount = atomic_read(&mutexp->sharecount)) != 0) {
		if(sharecount == 1)
			__db_msgadd(env, mbp, "1 reader");
		else
			__db_msgadd(env, mbp, "%d readers", sharecount);
		/* Show the thread which last acquired the latch. */
		__db_msgadd(env, mbp, " %s]", dbenv->thread_id_string(dbenv, mutexp->pid, mutexp->tid, buf));
	}
 #endif
	else
		__db_msgadd(env, mbp, "!Own]");

 #ifdef HAVE_MUTEX_HYBRID
	if(mutexp->hybrid_wait != 0 || mutexp->hybrid_wakeup != 0)
		__db_msgadd(env, mbp, " <wakeups %d/%d>", mutexp->hybrid_wait, mutexp->hybrid_wakeup);
 #endif
	if(LF_ISSET(DB_STAT_CLEAR))
		__mutex_clear(env, mutex);
}

static const char * __mutex_print_id(int alloc_id)
{
	switch(alloc_id) {
	    case MTX_APPLICATION:           return "application allocated";
	    case MTX_ATOMIC_EMULATION:      return "atomic emulation";
	    case MTX_DB_HANDLE:             return "db handle";
	    case MTX_ENV_DBLIST:            return "env dblist";
	    case MTX_ENV_HANDLE:            return "env handle";
	    case MTX_ENV_REGION:            return "env region";
	    case MTX_LOCK_REGION:           return "lock region";
	    case MTX_LOGICAL_LOCK:          return "logical lock";
	    case MTX_LOG_FILENAME:          return "log filename";
	    case MTX_LOG_FLUSH:             return "log flush";
	    case MTX_LOG_HANDLE:            return "log handle";
	    case MTX_LOG_REGION:            return "log region";
	    case MTX_MPOOLFILE_HANDLE:      return "mpoolfile handle";
	    case MTX_MPOOL_BH:              return "mpool buffer";
	    case MTX_MPOOL_FH:              return "mpool filehandle";
	    case MTX_MPOOL_FILE_BUCKET:     return "mpool file bucket";
	    case MTX_MPOOL_HANDLE:          return "mpool handle";
	    case MTX_MPOOL_HASH_BUCKET:     return "mpool hash bucket";
	    case MTX_MPOOL_REGION:          return "mpool region";
	    case MTX_MUTEX_REGION:          return "mutex region";
	    case MTX_MUTEX_TEST:            return "mutex test";
	    case MTX_REPMGR:                return "replication manager";
	    case MTX_REP_CHKPT:             return "replication checkpoint";
	    case MTX_REP_DATABASE:          return "replication database";
	    case MTX_REP_DIAG:              return "replication diagnostics";
	    case MTX_REP_EVENT:             return "replication event";
	    case MTX_REP_REGION:            return "replication region";
	    case MTX_REP_START:             return "replication role config";
	    case MTX_REP_WAITER:            return "replication txn apply";
	    case MTX_SEQUENCE:              return "sequence";
	    case MTX_TWISTER:               return "twister";
	    case MTX_TCL_EVENTS:            return "Tcl events";
	    case MTX_TXN_ACTIVE:            return "txn active list";
	    case MTX_TXN_CHKPT:             return "transaction checkpoint";
	    case MTX_TXN_COMMIT:            return "txn commit";
	    case MTX_TXN_MVCC:              return "txn mvcc";
	    case MTX_TXN_REGION:            return "txn region";
	    default:                        return "unknown mutex type";
		/* NOTREACHED */
	}
}
/*
* __mutex_set_wait_info --
*	Return mutex statistics.
*/
void __mutex_set_wait_info(ENV * env, db_mutex_t mutex, uintmax_t * waitp, uintmax_t * nowaitp)
{
	DB_MUTEX * mutexp;
	if(mutex == MUTEX_INVALID) {
		*waitp = 0;
		*nowaitp = 0;
		return;
	}
	mutexp = MUTEXP_SET(env, mutex);
	*waitp = mutexp->mutex_set_wait;
	*nowaitp = mutexp->mutex_set_nowait;
}
/*
 * __mutex_clear --
 *	Clear mutex statistics.
 */
void __mutex_clear(ENV * env, db_mutex_t mutex)
{
	if(MUTEX_ON(env)) {
		DB_MUTEX * mutexp = MUTEXP_SET(env, mutex);
		mutexp->mutex_set_wait = mutexp->mutex_set_nowait = 0;
 #ifdef HAVE_SHARED_LATCHES
		mutexp->mutex_set_rd_wait = mutexp->mutex_set_rd_nowait = 0;
 #endif
 #ifdef HAVE_MUTEX_HYBRID
		mutexp->hybrid_wait = mutexp->hybrid_wakeup = 0;
 #endif
	}
}

#else /* !HAVE_STATISTICS */

int __mutex_stat_pp(DB_ENV * dbenv, DB_MUTEX_STAT ** statp, uint32 flags)
{
	COMPQUIET(statp, 0);
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}

int __mutex_stat_print_pp(DB_ENV * dbenv, uint32 flags)
{
	COMPQUIET(flags, 0);
	return __db_stat_not_built(dbenv->env);
}
#endif
/*
 * __db_tas_mutex_init --
 *	Initialize a test-and-set mutex.
 *
 * PUBLIC: int __db_tas_mutex_init __P((ENV *, db_mutex_t, uint32));
 */
int __db_tas_mutex_init(ENV*env, db_mutex_t mutex, uint32 flags)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	int ret;
#ifndef HAVE_MUTEX_HYBRID
	COMPQUIET(flags, 0);
#endif
	dbenv = env->dbenv;
	mutexp = MUTEXP_SET(env, mutex);
	/* Check alignment. */
	if(((uintptr_t)mutexp&(dbenv->mutex_align-1)) != 0) {
		__db_errx(env, DB_STR("2028", "TAS: mutex not appropriately aligned"));
		return EINVAL;
	}
#ifdef HAVE_SHARED_LATCHES
	if(F_ISSET(mutexp, DB_MUTEX_SHARED))
		atomic_init(&mutexp->sharecount, 0);
	else
#endif
	if(MUTEX_INIT(&mutexp->tas)) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, DB_STR("2029", "TAS: mutex initialize"));
		return __os_posix_err(ret);
	}
#ifdef HAVE_MUTEX_HYBRID
	if((ret = __db_pthread_mutex_init(env, mutex, flags|DB_MUTEX_SELF_BLOCK)) != 0)
		return ret;
#endif
	return 0;
}
/*
 * __db_tas_mutex_lock_int
 *     Internal function to lock a mutex, or just try to lock it without waiting
 */
inline static int __db_tas_mutex_lock_int(ENV*env, db_mutex_t mutex, db_timeout_t timeout, int nowait)
{
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	DB_THREAD_INFO * ip;
	db_timespec now, timespec;
	uint32 nspins;
	int ret;
#ifdef HAVE_MUTEX_HYBRID
	const ulong micros = 0;
#else
	ulong micros, max_micros;
	db_timeout_t time_left;
#endif
	DB_ENV * dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(env, mutex);
	CHECK_MTX_THREAD(env, mutexp);
#ifdef HAVE_STATISTICS
	if(F_ISSET(mutexp, DB_MUTEX_LOCKED))
		STAT_INC(env, mutex, set_wait, mutexp->mutex_set_wait, mutex);
	else
		STAT_INC(env, mutex, set_nowait, mutexp->mutex_set_nowait, mutex);
#endif
#ifndef HAVE_MUTEX_HYBRID
	/*
	 * Wait 1ms initially, up to 10ms for mutexes backing logical database
	 * locks, and up to 25 ms for mutual exclusion data structure mutexes.
	 * SR: #7675
	 */
	micros = 1000;
	max_micros = F_ISSET(mutexp, DB_MUTEX_LOGICAL_LOCK) ? 10000 : 25000;
#endif
	/* Clear the ending timespec so it'll be initialed upon first need. */
	if(timeout != 0)
		timespecclear(&timespec);
	/*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times in the loop.
	 */
	ip = NULL;

loop:   /* Attempt to acquire the resource for N spins. */
	for(nspins = mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
#ifdef HAVE_MUTEX_S390_CC_ASSEMBLY
		tsl_t zero = 0;
#endif

#ifdef HAVE_MUTEX_HPPA_MSEM_INIT
relock:
#endif
		/*
		 * Avoid interlocked instructions until they're likely to
		 * succeed by first checking whether it is held
		 */
		if(MUTEXP_IS_BUSY(mutexp) || !MUTEXP_ACQUIRE(mutexp)) {
			if(F_ISSET(dbenv, DB_ENV_FAILCHK) && ip == NULL && dbenv->is_alive(dbenv, mutexp->pid, mutexp->tid, 0) == 0) {
				ret = __env_set_state(env, &ip, THREAD_VERIFY);
				if(ret != 0 || ip->dbth_state == THREAD_FAILCHK)
					return DB_RUNRECOVERY;
			}
			if(nowait)
				return DB_LOCK_NOTGRANTED;
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}
		MEMBAR_ENTER();

#ifdef HAVE_MUTEX_HPPA_MSEM_INIT
		/*
		 * HP semaphores are unlocked automatically when a holding
		 * process exits.  If the mutex appears to be locked
		 * (F_ISSET(DB_MUTEX_LOCKED)) but we got here, assume this
		 * has happened.  Set the pid and tid into the mutex and
		 * lock again.  (The default state of the mutexes used to
		 * block in __lock_get_internal is locked, so exiting with
		 * a locked mutex is reasonable behavior for a process that
		 * happened to initialize or use one of them.)
		 */
		if(F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
			goto relock;
		}
		/*
		 * If we make it here, the mutex isn't locked, the diagnostic
		 * won't fire, and we were really unlocked by someone calling
		 * the DB mutex unlock function.
		 */
#endif
#ifdef DIAGNOSTIC
		if(F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			__db_errx(env, DB_STR_A("2030", "TAS lock failed: lock %ld currently in use: ID: %s", "%ld %s"), (long)mutex,
				dbenv->thread_id_string(dbenv, mutexp->pid, mutexp->tid, buf));
			return __env_panic(env, EACCES);
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
#ifdef DIAGNOSTIC
		/*
		 * We want to switch threads as often as possible.  Yield
		 * every time we get a mutex to ensure contention.
		 */
		if(F_ISSET(dbenv, DB_ENV_YIELDCPU))
			__os_yield(env, 0, 0);
#endif
		return 0;
	}
	/*
	 * We need to wait for the lock to become available.
	 * Possibly setup timeouts if this is the first wait, or
	 * check expiration times for the second and subsequent waits.
	 */
	if(timeout != 0) {
		// Set the expiration time if this is the first sleep
		if(!timespecisset(&timespec))
			__clock_set_expires(env, &timespec, timeout);
		else {
			timespecclear(&now);
			if(__clock_expired(env, &now, &timespec))
				return DB_TIMEOUT;
#ifndef HAVE_MUTEX_HYBRID
			timespecsub(&now, &timespec);
			DB_TIMESPEC_TO_TIMEOUT(time_left, &now, 0);
			time_left = timeout-time_left;
			SETMIN(micros, time_left);
#endif
		}
	}
	/*
	 * This yields for a while for tas mutexes, and just gives up the
	 * processor for hybrid mutexes.
	 * By yielding here we can get the other thread to give up the
	 * mutex before calling the more expensive library mutex call.
	 * Tests have shown this to be a big win when there is contention.
	 */
	PERFMON4(env, mutex, suspend, mutex, TRUE, mutexp->alloc_id, mutexp);
	__os_yield(env, 0, micros);
	PERFMON4(env, mutex, resume, mutex, TRUE, mutexp->alloc_id, mutexp);

#if defined(HAVE_MUTEX_HYBRID)
	if(!MUTEXP_IS_BUSY(mutexp))
		goto loop;
	/* Wait until the mutex can be obtained exclusively or it times out. */
	if((ret = __db_hybrid_mutex_suspend(env, mutex, timeout == 0 ? NULL : &timespec, TRUE)) != 0)
		return ret;
#else
	if((micros <<= 1) > max_micros)
		micros = max_micros;
#endif

	/*
	 * We're spinning.  The environment might be hung, and somebody else
	 * has already recovered it.  The first thing recovery does is panic
	 * the environment.  Check to see if we're never going to get this
	 * mutex.
	 */
	PANIC_CHECK(env);
	goto loop;
}
/*
 * __db_tas_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_tas_mutex_lock __P((ENV *, db_mutex_t, db_timeout_t));
 */
int __db_tas_mutex_lock(ENV*env, db_mutex_t mutex, db_timeout_t timeout)
{
	return __db_tas_mutex_lock_int(env, mutex, timeout, 0);
}
/*
 * __db_tas_mutex_trylock
 *	Try to exclusively lock a mutex without ever blocking - ever!
 *
 *	Returns 0 on success,
 *		DB_LOCK_NOTGRANTED on timeout
 *		Possibly DB_RUNRECOVERY if DB_ENV_FAILCHK or panic.
 *
 *	This will work for DB_MUTEX_SHARED, though it always tries
 *	for exclusive access.
 *
 * PUBLIC: int __db_tas_mutex_trylock __P((ENV *, db_mutex_t));
 */
int __db_tas_mutex_trylock(ENV * env, db_mutex_t mutex)
{
	return __db_tas_mutex_lock_int(env, mutex, 0, 1);
}

#if defined(HAVE_SHARED_LATCHES)
/*
 * __db_tas_mutex_readlock_int
 *    Internal function to get a shared lock on a latch, blocking if necessary.
 *
 */
static inline int __db_tas_mutex_readlock_int(ENV * env, db_mutex_t mutex, int nowait)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	DB_THREAD_INFO * ip;
	int lock;
	uint32 nspins;
	int ret;
 #ifndef HAVE_MUTEX_HYBRID
	ulong micros, max_micros;
 #endif
	dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(env, mutex);
	CHECK_MTX_THREAD(env, mutexp);
	DB_ASSERT(env, F_ISSET(mutexp, DB_MUTEX_SHARED));
 #ifdef HAVE_STATISTICS
	if(F_ISSET(mutexp, DB_MUTEX_LOCKED))
		STAT_INC(env, mutex, set_rd_wait, mutexp->mutex_set_rd_wait, mutex);
	else
		STAT_INC(env, mutex, set_rd_nowait, mutexp->mutex_set_rd_nowait, mutex);
 #endif

 #ifndef HAVE_MUTEX_HYBRID
	/*
	 * Wait 1ms initially, up to 10ms for mutexes backing logical database
	 * locks, and up to 25 ms for mutual exclusion data structure mutexes.
	 * SR: #7675
	 */
	micros = 1000;
	max_micros = F_ISSET(mutexp, DB_MUTEX_LOGICAL_LOCK) ? 10000 : 25000;
 #endif

loop:   /* Attempt to acquire the resource for N spins. */
	for(nspins = mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		lock = atomic_read(&mutexp->sharecount);
		if(lock == MUTEX_SHARE_ISEXCLUSIVE || !atomic_compare_exchange(env, &mutexp->sharecount, lock, lock+1)) {
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}
		MEMBAR_ENTER();
		/* For shared lactches the threadid is the last requestor's id.
		 */
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);

		return 0;
	}
	/*
	 * Waiting for the latched must be avoided when it could allow a
	 * 'failchk'ing thread to hang.
	 */
	if(F_ISSET(dbenv, DB_ENV_FAILCHK) && dbenv->is_alive(dbenv, mutexp->pid, mutexp->tid, 0) == 0) {
		ret = __env_set_state(env, &ip, THREAD_VERIFY);
		if(ret != 0 || ip->dbth_state == THREAD_FAILCHK)
			return DB_RUNRECOVERY;
	}
	/*
	 * It is possible to spin out when the latch is just shared, due to
	 * many threads or interrupts interfering with the compare&exchange.
	 * Avoid spurious DB_LOCK_NOTGRANTED returns by retrying.
	 */
	if(nowait) {
		if(atomic_read(&mutexp->sharecount) != MUTEX_SHARE_ISEXCLUSIVE)
			goto loop;
		return DB_LOCK_NOTGRANTED;
	}
	/* Wait for the lock to become available. */
 #ifdef HAVE_MUTEX_HYBRID
	/*
	 * By yielding here we can get the other thread to give up the
	 * mutex before calling the more expensive library mutex call.
	 * Tests have shown this to be a big win when there is contention.
	 */
	PERFMON4(env, mutex, suspend, mutex, FALSE, mutexp->alloc_id, mutexp);
	__os_yield(env, 0, 0);
	PERFMON4(env, mutex, resume, mutex, FALSE, mutexp->alloc_id, mutexp);
	if(atomic_read(&mutexp->sharecount) != MUTEX_SHARE_ISEXCLUSIVE)
		goto loop;
	/* Wait until the mutex is no longer exclusively locked. */
	if((ret = __db_hybrid_mutex_suspend(env, mutex, NULL, FALSE)) != 0)
		return ret;
 #else
	PERFMON4(env, mutex, suspend, mutex, FALSE, mutexp->alloc_id, mutexp);
	__os_yield(env, 0, micros);
	PERFMON4(env, mutex, resume, mutex, FALSE, mutexp->alloc_id, mutexp);
	if((micros <<= 1) > max_micros)
		micros = max_micros;
 #endif

	/*
	 * We're spinning.  The environment might be hung, and somebody else
	 * has already recovered it.  The first thing recovery does is panic
	 * the environment.  Check to see if we're never going to get this
	 * mutex.
	 */
	PANIC_CHECK(env);
	goto loop;
}
/*
 * __db_tas_mutex_readlock
 *	Get a shared lock on a latch, waiting if necessary.
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_tas_mutex_readlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int __db_tas_mutex_readlock(ENV*env, db_mutex_t mutex)
{
	return __db_tas_mutex_readlock_int(env, mutex, 0);
}
/*
 * __db_tas_mutex_tryreadlock
 *	Try to get a shared lock on a latch; don't wait when busy.
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_tas_mutex_tryreadlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int __db_tas_mutex_tryreadlock(ENV*env, db_mutex_t mutex)
{
	return __db_tas_mutex_readlock_int(env, mutex, 1);
}
#endif
/*
 * __db_tas_mutex_unlock --
 *	Release a mutex.
 *
 * Hybrid shared latch wakeup
 *	When an exclusive requester waits for the last shared holder to
 *	release, it increments mutexp->wait and pthread_cond_wait()'s. The
 *	last shared unlock calls __db_pthread_mutex_unlock() to wake it.
 */
int __db_tas_mutex_unlock(ENV * env, db_mutex_t mutex)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
#ifdef HAVE_MUTEX_HYBRID
	int ret;
 #ifdef MUTEX_DIAG
	int waiters;
 #endif
#endif
#ifdef HAVE_SHARED_LATCHES
	int sharecount;
#endif
	dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mutexp = MUTEXP_SET(env, mutex);
#if defined(HAVE_MUTEX_HYBRID) && defined(MUTEX_DIAG)
	waiters = mutexp->wait;
#endif

#if defined(DIAGNOSTIC)
 #if defined(HAVE_SHARED_LATCHES)
	if(F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		if(atomic_read(&mutexp->sharecount) == 0) {
			__db_errx(env, DB_STR_A("2031", "shared unlock %ld already unlocked", "%ld"), (long)mutex);
			return __env_panic(env, EACCES);
		}
	}
	else
 #endif
	if(!F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
		__db_errx(env, DB_STR_A("2032", "unlock %ld already unlocked", "%ld"), (long)mutex);
		return __env_panic(env, EACCES);
	}
#endif

#ifdef HAVE_SHARED_LATCHES
	if(F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		sharecount = atomic_read(&mutexp->sharecount);
		/*MUTEX_MEMBAR(mutexp->sharecount);*/		/* XXX why? */
		if(sharecount == MUTEX_SHARE_ISEXCLUSIVE) {
			F_CLR(mutexp, DB_MUTEX_LOCKED);
			/* Flush flag update before zeroing count */
			MEMBAR_EXIT();
			atomic_init(&mutexp->sharecount, 0);
		}
		else {
			DB_ASSERT(env, sharecount > 0);
			MEMBAR_EXIT();
			sharecount = atomic_dec(env, &mutexp->sharecount);
			DB_ASSERT(env, sharecount >= 0);
			if(sharecount > 0)
				return 0;
		}
	}
	else
#endif
	{
		F_CLR(mutexp, DB_MUTEX_LOCKED);
		MUTEX_UNSET(&mutexp->tas);
	}
#ifdef HAVE_MUTEX_HYBRID
 #ifdef DIAGNOSTIC
	if(F_ISSET(dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
 #endif
	/* Prevent the load of wait from being hoisted before MUTEX_UNSET */
	MUTEX_MEMBAR(mutexp->flags);
	if(mutexp->wait && (ret = __db_pthread_mutex_unlock(env, mutex)) != 0)
		return ret;
 #ifdef MUTEX_DIAG
	if(mutexp->wait)
		printf("tas_unlock %ld %x waiters! busy %x waiters %d/%d\n", mutex, pthread_self(), MUTEXP_BUSY_FIELD(mutexp), waiters, mutexp->wait);
 #endif
#endif
	return 0;
}
/*
 * __db_tas_mutex_destroy --
 *	Destroy a mutex.
 */
int __db_tas_mutex_destroy(ENV * env, db_mutex_t mutex)
{
	DB_MUTEX * mutexp;
#ifdef HAVE_MUTEX_HYBRID
	int ret;
#endif
	if(!MUTEX_ON(env))
		return 0;
	mutexp = MUTEXP_SET(env, mutex);
	MUTEX_DESTROY(&mutexp->tas);
#ifdef HAVE_MUTEX_HYBRID
	if((ret = __db_pthread_mutex_destroy(env, mutex)) != 0)
		return ret;
#endif
	COMPQUIET(mutexp, 0);        /* MUTEX_DESTROY may not be defined. */
	return 0;
}
//
//
//
/*
 * __mutex_alloc_pp --
 *	Allocate a mutex, application method.
 */
int __mutex_alloc_pp(DB_ENV * dbenv, uint32 flags, db_mutex_t * indxp)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	if((ret = __db_fchk(env, "DB_ENV->mutex_alloc", flags, DB_MUTEX_PROCESS_ONLY|DB_MUTEX_SELF_BLOCK)) != 0)
		return ret;
	ENV_ENTER(env, ip);
	ret = __mutex_alloc(env, MTX_APPLICATION, flags, indxp);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __mutex_free_pp --
 *	Destroy a mutex, application method.
 */
int __mutex_free_pp(DB_ENV * dbenv, db_mutex_t indx)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	if(indx == MUTEX_INVALID)
		return EINVAL;
	/*
	 * Internally Berkeley DB passes around the db_mutex_t address on
	 * free, because we want to make absolutely sure the slot gets
	 * overwritten with MUTEX_INVALID.  We don't export MUTEX_INVALID,
	 * so we don't export that part of the API, either.
	 */
	ENV_ENTER(env, ip);
	ret = __mutex_free(env, &indx);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __mutex_lock --
 *	Lock a mutex, application method.
 */
int __mutex_lock_pp(DB_ENV * dbenv, db_mutex_t indx)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	if(indx == MUTEX_INVALID)
		return EINVAL;
	ENV_ENTER(env, ip);
	ret = __mutex_lock(env, indx);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __mutex_unlock --
 *	Unlock a mutex, application method.
 */
int __mutex_unlock_pp(DB_ENV * dbenv, db_mutex_t indx)
{
	DB_THREAD_INFO * ip;
	int ret;
	ENV * env = dbenv->env;
	if(indx == MUTEX_INVALID)
		return EINVAL;
	ENV_ENTER(env, ip);
	ret = __mutex_unlock(env, indx);
	ENV_LEAVE(env, ip);
	return ret;
}
/*
 * __mutex_get_align --
 *	DB_ENV->mutex_get_align.
 */
int __mutex_get_align(DB_ENV * dbenv, uint32 * alignp)
{
	ENV * env = dbenv->env;
	if(MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*alignp = ((DB_MUTEXREGION *)env->mutex_handle->reginfo.primary)->stat.st_mutex_align;
	}
	else
		*alignp = dbenv->mutex_align;
	return 0;
}
/*
 * __mutex_set_align --
 *	DB_ENV->mutex_set_align.
 */
int __mutex_set_align(DB_ENV * dbenv, uint32 align)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_align");
	if(align == 0 || !POWER_OF_TWO(align)) {
		__db_errx(env, DB_STR("2018", "DB_ENV->mutex_set_align: alignment value must be a non-zero power-of-two"));
		return EINVAL;
	}
	dbenv->mutex_align = align;
	return 0;
}
/*
 * __mutex_get_increment --
 *	DB_ENV->mutex_get_increment.
 */
int __mutex_get_increment(DB_ENV * dbenv, uint32 * incrementp)
{
	/*
	 * We don't maintain the increment in the region (it just makes
	 * no sense).  Return whatever we have configured on this handle,
	 * nobody is ever going to notice.
	 */
	*incrementp = dbenv->mutex_inc;
	return 0;
}
/*
 * __mutex_set_increment --
 *	DB_ENV->mutex_set_increment.
 */
int __mutex_set_increment(DB_ENV * dbenv, uint32 increment)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_increment");
	dbenv->mutex_cnt = 0;
	dbenv->mutex_inc = increment;
	return 0;
}
/*
 * __mutex_get_init --
 *	DB_ENV->mutex_get_init.
 */
int __mutex_get_init(DB_ENV * dbenv, uint32 * initp)
{
	ENV * env = dbenv->env;
	if(MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*initp = ((DB_MUTEXREGION *)env->mutex_handle->reginfo.primary)->stat.st_mutex_init;
	}
	else
		*initp = dbenv->mutex_cnt;
	return 0;
}
/*
 * __mutex_set_init --
 *	DB_ENV->mutex_set_init.
 */
int __mutex_set_init(DB_ENV * dbenv, uint32 init)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_init");
	dbenv->mutex_cnt = init;
	dbenv->mutex_inc = 0;
	return 0;
}
/*
 * __mutex_get_max --
 *	DB_ENV->mutex_get_max.
 */
int __mutex_get_max(DB_ENV * dbenv, uint32 * maxp)
{
	ENV * env = dbenv->env;
	if(MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*maxp = ((DB_MUTEXREGION *)env->mutex_handle->reginfo.primary)->stat.st_mutex_max;
	}
	else
		*maxp = dbenv->mutex_max;
	return 0;
}
/*
 * __mutex_set_max --
 *	DB_ENV->mutex_set_max.
 */
int __mutex_set_max(DB_ENV * dbenv, uint32 max)
{
	ENV * env = dbenv->env;
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_max");
	dbenv->mutex_max = max;
	dbenv->mutex_inc = 0;
	return 0;
}
/*
 * __mutex_get_tas_spins --
 *	DB_ENV->mutex_get_tas_spins.
 */
int __mutex_get_tas_spins(DB_ENV * dbenv, uint32 * tas_spinsp)
{
	ENV * env = dbenv->env;
	if(MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*tas_spinsp = ((DB_MUTEXREGION *)env->mutex_handle->reginfo.primary)->stat.st_mutex_tas_spins;
	}
	else
		*tas_spinsp = dbenv->mutex_tas_spins;
	return 0;
}
/*
 * __mutex_set_tas_spins --
 *	DB_ENV->mutex_set_tas_spins.
 */
int __mutex_set_tas_spins(DB_ENV * dbenv, uint32 tas_spins)
{
	ENV * env = dbenv->env;
	//
	// Bound the value -- less than 1 makes no sense, greater than 1M makes no sense.
	//
	if(tas_spins == 0)
		tas_spins = 1;
	else if(tas_spins > 1000000)
		tas_spins = 1000000;
	/*
	 * There's a theoretical race here, but I'm not interested in locking
	 * the test-and-set spin count.  The worst possibility is a thread
	 * reads out a bad spin count and spins until it gets the lock, but
	 * that's awfully unlikely.
	 */
	if(MUTEX_ON(env))
		((DB_MUTEXREGION *)env->mutex_handle->reginfo.primary)->stat.st_mutex_tas_spins = tas_spins;
	else
		dbenv->mutex_tas_spins = tas_spins;
	return 0;
}

#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
/*
 * Provide atomic operations for platforms which have mutexes yet do not have
 * native atomic operations configured. They are emulated by protected the
 * operation with a mutex.  The address of the atomic value selects which
 * mutex to use.
 */
/*
 * atomic_get_mutex -
 *	Map an address to the mutex to use to atomically modify it
 */
static inline db_mutex_t atomic_get_mutex(ENV * env, db_atomic_t * v)
{
	uint index;
	DB_MUTEXREGION * mtxreg;
	if(!MUTEX_ON(env))
		return MUTEX_INVALID;
	index = (uint)(((uintptr_t)(v))>>6)%MAX_ATOMIC_MUTEXES;
	mtxreg = (DB_MUTEXREGION *)env->mutex_handle->reginfo.primary;
	return mtxreg->mtx_atomic[index];
}
/*
 * __atomic_inc
 *	Use a mutex to provide an atomic increment function
 *
 * PUBLIC: #if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
 * PUBLIC: atomic_value_t __atomic_inc __P((ENV *, db_atomic_t *));
 * PUBLIC: #endif
 */
atomic_value_t __atomic_inc(ENV * env, db_atomic_t * v)
{
	int ret;
	db_mutex_t mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = ++v->value;
	MUTEX_UNLOCK(env, mtx);
	return ret;
}
/*
 * __atomic_dec
 *	Use a mutex to provide an atomic decrement function
 *
 * PUBLIC: #if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
 * PUBLIC: atomic_value_t __atomic_dec __P((ENV *, db_atomic_t *));
 * PUBLIC: #endif
 */
atomic_value_t __atomic_dec(ENV * env, db_atomic_t * v)
{
	int ret;
	db_mutex_t mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = --v->value;
	MUTEX_UNLOCK(env, mtx);
	return ret;
}
/*
 * atomic_compare_exchange
 *	Use a mutex to provide an atomic decrement function
 *
 * PUBLIC: #if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
 * PUBLIC: int atomic_compare_exchange
 * PUBLIC:     __P((ENV *, db_atomic_t *, atomic_value_t, atomic_value_t));
 * PUBLIC: #endif
 *	Returns 1 if the *v was equal to oldval, else 0
 *
 *	Side Effect:
 *		Sets the value to newval if and only if returning 1
 */
int atomic_compare_exchange(ENV * env, db_atomic_t * v, atomic_value_t oldval, atomic_value_t newval)
{
	db_mutex_t mtx;
	int ret;
	if(atomic_read(v) != oldval)
		return 0;
	mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = atomic_read(v) == oldval;
	if(ret)
		atomic_init(v, newval);
	MUTEX_UNLOCK(env, mtx);
	return ret;
}
#endif
//
// WIN32
//
/*
 * Common code to get an event handle.  This is executed whenever a mutex
 * blocks, or when unlocking a mutex that a thread is waiting on.  We can't
 * keep these handles around, since the mutex structure is in shared memory,
 * and each process gets its own handle value.
 *
 * We pass security attributes so that the created event is accessible by all
 * users, in case a Windows service is sharing an environment with a local
 * process run as a different user.
 */
static __inline int get_handle(ENV * env, DB_MUTEX * mutexp, HANDLE * eventp)
{
	static _TCHAR hex_digits[] = _T("0123456789abcdef");
	//
	_TCHAR idbuf[] = _T("db.m00000000");
	_TCHAR * p = idbuf+12;
	int ret = 0;
	uint32 id;
	for(id = (mutexp)->id; id != 0; id >>= 4)
		*--p = hex_digits[id&0xf];
#ifndef DB_WINCE
	if(DB_GLOBAL(win_sec_attr) == NULL) {
		InitializeSecurityDescriptor(&DB_GLOBAL(win_default_sec_desc), SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&DB_GLOBAL(win_default_sec_desc), TRUE, 0, FALSE);
		DB_GLOBAL(win_default_sec_attr).nLength = sizeof(SECURITY_ATTRIBUTES);
		DB_GLOBAL(win_default_sec_attr).bInheritHandle = FALSE;
		DB_GLOBAL(win_default_sec_attr).lpSecurityDescriptor = &DB_GLOBAL(win_default_sec_desc);
		DB_GLOBAL(win_sec_attr) = &DB_GLOBAL(win_default_sec_attr);
	}
#endif
	if((*eventp = CreateEvent(DB_GLOBAL(win_sec_attr), FALSE, FALSE, idbuf)) == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, DB_STR("2002", "Win32 create event failed"));
	}
	return ret;
}
/*
 * __db_win32_mutex_lock_int
 *	Internal function to lock a win32 mutex
 *
 *	If the wait paramter is 0, this function will return DB_LOCK_NOTGRANTED
 *	rather than wait.
 *
 */
static __inline int __db_win32_mutex_lock_int(ENV * env, db_mutex_t mutex, db_timeout_t timeout, int wait)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	DB_THREAD_INFO * ip;
	HANDLE event;
	uint32 ms, nspins;
	db_timespec now, tempspec, timeoutspec;
	db_timeout_t time_left;
	int ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER diag_now;
#endif
	dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(env, mutex);
	CHECK_MTX_THREAD(env, mutexp);
	if(timeout != 0) {
		timespecclear(&timeoutspec);
		__clock_set_expires(env, &timeoutspec, timeout);
	}
	/*
	 * See WINCE_ATOMIC_MAGIC definition for details.
	 * Use sharecount, because the value just needs to be a db_atomic_t
	 * memory mapped onto the same page as those being Interlocked*.
	 */
	WINCE_ATOMIC_MAGIC(&mutexp->sharecount);

	event = NULL;
	ms = 50;
	ret = 0;

	/*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times
	 * in the loop.
	 */
	ip = NULL;

loop:   /* Attempt to acquire the mutex mutex_tas_spins times, if waiting. */
	for(nspins = mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		/*
		 * We can avoid the (expensive) interlocked instructions if
		 * the mutex is already busy.
		 */
		if(MUTEXP_IS_BUSY(mutexp) || !MUTEXP_ACQUIRE(mutexp)) {
			if(F_ISSET(dbenv, DB_ENV_FAILCHK) && !ip && dbenv->is_alive(dbenv, mutexp->pid, mutexp->tid, 0) == 0) {
				ret = __env_set_state(env, &ip, THREAD_VERIFY);
				if(ret != 0 || ip->dbth_state == THREAD_FAILCHK)
					return DB_RUNRECOVERY;
			}
			if(!wait)
				return DB_LOCK_NOTGRANTED;
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause before retrying. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}
#ifdef DIAGNOSTIC
		if(F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			__db_errx(env, DB_STR_A("2003", "Win32 lock failed: mutex already locked by %s", "%s"), dbenv->thread_id_string(dbenv, mutexp->pid, mutexp->tid, buf));
			return __env_panic(env, EACCES);
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
#ifdef HAVE_STATISTICS
		if(event == NULL)
			++mutexp->mutex_set_nowait;
		else
			++mutexp->mutex_set_wait;
#endif
		if(event) {
			CloseHandle(event);
			InterlockedDecrement(&mutexp->nwaiters);
#ifdef MUTEX_DIAG
			if(ret != WAIT_OBJECT_0) {
				QueryPerformanceCounter(&diag_now);
				printf(DB_STR_A("2004", "[%I64d]: Lost signal on mutex %p, id %d, ms %d\n", "%I64d %p %d %d"), diag_now.QuadPart, mutexp, mutexp->id, ms);
			}
#endif
		}
#ifdef DIAGNOSTIC
		/*
		 * We want to switch threads as often as possible.  Yield
		 * every time we get a mutex to ensure contention.
		 */
		if(F_ISSET(dbenv, DB_ENV_YIELDCPU))
			__os_yield(env, 0, 0);
#endif
		return 0;
	}
	/*
	 * Yield the processor; wait 50 ms initially, up to 1 second.  This
	 * loop is needed to work around a race where the signal from the
	 * unlocking thread gets lost.  We start at 50 ms because it's unlikely
	 * to happen often and we want to avoid wasting CPU.
	 */
	if(timeout != 0) {
		timespecclear(&now);
		if(__clock_expired(env, &now, &timeoutspec)) {
			if(event) {
				CloseHandle(event);
				InterlockedDecrement(&mutexp->nwaiters);
			}
			return DB_TIMEOUT;
		}
		// Reduce the event wait if the timeout would happen first
		tempspec = timeoutspec;
		timespecsub(&tempspec, &now);
		DB_TIMESPEC_TO_TIMEOUT(time_left, &tempspec, 0);
		time_left /= US_PER_MS;
		SETMIN(ms, time_left);
	}
	if(event == NULL) {
#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&diag_now);
		printf(DB_STR_A("2005", "[%I64d]: Waiting on mutex %p, id %d\n", "%I64d %p %d"), diag_now.QuadPart, mutexp, mutexp->id);
#endif
		InterlockedIncrement(&mutexp->nwaiters);
		if((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
	}
	if((ret = WaitForSingleObject(event, ms)) == WAIT_FAILED) {
		ret = __os_get_syserr();
		goto err;
	}
	if((ms <<= 1) > MS_PER_SEC)
		ms = MS_PER_SEC;
	PANIC_CHECK(env);
	goto loop;
err:
	__db_syserr(env, ret, DB_STR("2006", "Win32 lock failed"));
	return __env_panic(env, __os_posix_err(ret));
}
/*
 * __db_win32_mutex_init --
 *	Initialize a Win32 mutex.
 */
int __db_win32_mutex_init(ENV*env, db_mutex_t mutex, uint32 flags)
{
	DB_MUTEX * mutexp = MUTEXP_SET(env, mutex);
	mutexp->id = ((getpid()&0xffff)<<16)^P_TO_UINT32(mutexp);
	F_SET(mutexp, flags);
	return 0;
}
/*
 * __db_win32_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_win32_mutex_lock __P((ENV *, db_mutex_t, db_timeout_t));
 */
int __db_win32_mutex_lock(ENV*env, db_mutex_t mutex, db_timeout_t timeout)
{
	return __db_win32_mutex_lock_int(env, mutex, timeout, 1);
}
/*
 * __db_win32_mutex_trylock
 *	Try to lock a mutex, returning without waiting if it is busy
 */
int __db_win32_mutex_trylock(ENV * env, db_mutex_t mutex)
{
	return __db_win32_mutex_lock_int(env, mutex, 0, 0);
}

#if defined(HAVE_SHARED_LATCHES)
/*
 * __db_win32_mutex_readlock_int
 *	Try to lock a mutex, possibly waiting if requested and necessary.
 */
int __db_win32_mutex_readlock_int(ENV*env, db_mutex_t mutex, int nowait)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	DB_MUTEXMGR * mtxmgr;
	DB_MUTEXREGION * mtxregion;
	HANDLE event;
	uint32 nspins;
	int ms, ret;
	long exch_ret, mtx_val;
 #ifdef MUTEX_DIAG
	LARGE_INTEGER diag_now;
 #endif
	dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mtxmgr = env->mutex_handle;
	mtxregion = (DB_MUTEXREGION *)mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(env, mutex);
	CHECK_MTX_THREAD(env, mutexp);
	/*
	 * See WINCE_ATOMIC_MAGIC definition for details.
	 * Use sharecount, because the value just needs to be a db_atomic_t
	 * memory mapped onto the same page as those being Interlocked*.
	 */
	WINCE_ATOMIC_MAGIC(&mutexp->sharecount);
	event = NULL;
	ms = 50;
	ret = 0;
	/*
	 * This needs to be initialized, since if mutexp->tas
	 * is write locked on the first pass, it needs a value.
	 */
	exch_ret = 0;

loop:   /* Attempt to acquire the resource for N spins. */
	for(nspins = mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		//
		// We can avoid the (expensive) interlocked instructions if the mutex is already "set".
		//
retry:          
		mtx_val = atomic_read(&mutexp->sharecount);
		if(mtx_val == MUTEX_SHARE_ISEXCLUSIVE) {
			if(nowait)
				return DB_LOCK_NOTGRANTED;
			continue;
		}
		else if(!atomic_compare_exchange(env, &mutexp->sharecount,
				mtx_val, mtx_val+1)) {
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			goto retry;
		}
 #ifdef HAVE_STATISTICS
		if(event == NULL)
			++mutexp->mutex_set_rd_nowait;
		else
			++mutexp->mutex_set_rd_wait;
 #endif
		if(event) {
			CloseHandle(event);
			InterlockedDecrement(&mutexp->nwaiters);
 #ifdef MUTEX_DIAG
			if(ret != WAIT_OBJECT_0) {
				QueryPerformanceCounter(&diag_now);
				printf(DB_STR_A("2007", "[%I64d]: Lost signal on mutex %p, id %d, ms %d\n", "%I64d %p %d %d"), diag_now.QuadPart, mutexp, mutexp->id, ms);
			}
 #endif
		}
 #ifdef DIAGNOSTIC
		/*
		 * We want to switch threads as often as possible.  Yield
		 * every time we get a mutex to ensure contention.
		 */
		if(F_ISSET(dbenv, DB_ENV_YIELDCPU))
			__os_yield(env, 0, 0);
 #endif

		return 0;
	}
	/*
	 * Yield the processor; wait 50 ms initially, up to 1 second.  This
	 * loop is needed to work around a race where the signal from the
	 * unlocking thread gets lost.  We start at 50 ms because it's unlikely
	 * to happen often and we want to avoid wasting CPU.
	 */
	if(event == NULL) {
 #ifdef MUTEX_DIAG
		QueryPerformanceCounter(&diag_now);
		printf(DB_STR_A("2008", "[%I64d]: Waiting on mutex %p, id %d\n", "%I64d %p %d"), diag_now.QuadPart, mutexp, mutexp->id);
 #endif
		InterlockedIncrement(&mutexp->nwaiters);
		if((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
	}
	if((ret = WaitForSingleObject(event, ms)) == WAIT_FAILED) {
		ret = __os_get_syserr();
		goto err;
	}
	if((ms <<= 1) > MS_PER_SEC)
		ms = MS_PER_SEC;
	PANIC_CHECK(env);
	goto loop;
err:
	__db_syserr(env, ret, DB_STR("2009", "Win32 read lock failed"));
	return __env_panic(env, __os_posix_err(ret));
}
/*
 * __db_win32_mutex_readlock
 *	Get a shared lock on a latch
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_win32_mutex_readlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int __db_win32_mutex_readlock(ENV * env, db_mutex_t mutex)
{
	return __db_win32_mutex_readlock_int(env, mutex, 0);
}
/*
 * __db_win32_mutex_tryreadlock
 *	Try to a shared lock on a latch
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_win32_mutex_tryreadlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int __db_win32_mutex_tryreadlock(ENV * env, db_mutex_t mutex)
{
	return __db_win32_mutex_readlock_int(env, mutex, 1);
}
#endif
/*
 * __db_win32_mutex_unlock --
 *	Release a mutex.
 */
int __db_win32_mutex_unlock(ENV * env, db_mutex_t mutex)
{
	DB_ENV * dbenv;
	DB_MUTEX * mutexp;
	HANDLE event;
	int ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER diag_now;
#endif
	dbenv = env->dbenv;
	if(!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return 0;
	mutexp = MUTEXP_SET(env, mutex);
#ifdef DIAGNOSTIC
	if(!MUTEXP_IS_BUSY(mutexp) || !(F_ISSET(mutexp, DB_MUTEX_SHARED) || F_ISSET(mutexp, DB_MUTEX_LOCKED))) {
		__db_errx(env, DB_STR_A("2010", "Win32 unlock failed: lock already unlocked: mutex %d busy %d", "%d %d"), mutex, MUTEXP_BUSY_FIELD(mutexp));
		return __env_panic(env, EACCES);
	}
#endif
	/*
	 * If we have a shared latch, and a read lock (DB_MUTEX_LOCKED is only
	 * set for write locks), then decrement the latch. If the readlock is
	 * still held by other threads, just return. Otherwise go ahead and notify any waiting threads.
	 */
#ifdef HAVE_SHARED_LATCHES
	if(F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		if(F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			F_CLR(mutexp, DB_MUTEX_LOCKED);
			if((ret = InterlockedExchange((interlocked_val)(&atomic_read(&mutexp->sharecount)), 0)) != MUTEX_SHARE_ISEXCLUSIVE) {
				ret = DB_RUNRECOVERY;
				goto err;
			}
		}
		else if(InterlockedDecrement((interlocked_val)(&atomic_read(&mutexp->sharecount))) > 0)
			return 0;
	}
	else
#endif
	{
		F_CLR(mutexp, DB_MUTEX_LOCKED);
		MUTEX_UNSET(&mutexp->tas);
	}
	if(mutexp->nwaiters > 0) {
		if((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&diag_now);
		printf(DB_STR_A("2011", "[%I64d]: Signalling mutex %p, id %d\n", "%I64d %p %d"), diag_now.QuadPart, mutexp, mutexp->id);
#endif
		if(!PulseEvent(event)) {
			ret = __os_get_syserr();
			CloseHandle(event);
			goto err;
		}
		CloseHandle(event);
	}
	return 0;
err:
	__db_syserr(env, ret, DB_STR("2012", "Win32 unlock failed"));
	return __env_panic(env, __os_posix_err(ret));
}
/*
 * __db_win32_mutex_destroy --
 *	Destroy a mutex.
 */
int __db_win32_mutex_destroy(ENV * env, db_mutex_t mutex)
{
	return 0;
}

#ifndef DB_WINCE
/*
 * db_env_set_win_security
 *
 *	Set the SECURITY_ATTRIBUTES to be used by BDB on Windows.
 *	It should not be called while any BDB mutexes are locked.
 */
int db_env_set_win_security(SECURITY_ATTRIBUTES * sa)
{
	DB_GLOBAL(win_sec_attr) = sa;
	return 0;
}
#endif
