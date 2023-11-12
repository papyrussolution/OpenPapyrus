// PTW32.C
//
/*
 * Description: This translation unit implements miscellaneous thread functions.
 *
 *   Pthreads4w - POSIX Threads for Windows
 *   Copyright 1998 John E. Bossom
 *   Copyright 1999-2018, Pthreads4w contributors
 *
 *   Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *   The current list of contributors is contained
 *   in the file CONTRIBUTORS included with the source
 *   code distribution. The list can also be seen at the
 *   following World Wide Web location: https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */
#include <sl_pthreads4w.h>
#pragma hdrstop

static const int64_t NANOSEC_PER_SEC = SlConst::OneBillion;
static const int64_t NANOSEC_PER_MILLISEC = 1000000;
static const int64_t MILLISEC_PER_SEC = 1000;
static struct pthread_mutexattr_t_ __ptw32_recursive_mutexattr_s = {PTHREAD_PROCESS_PRIVATE, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_STALLED};
static struct pthread_mutexattr_t_ __ptw32_errorcheck_mutexattr_s = {PTHREAD_PROCESS_PRIVATE, PTHREAD_MUTEX_ERRORCHECK, PTHREAD_MUTEX_STALLED};
static pthread_mutexattr_t __ptw32_recursive_mutexattr = &__ptw32_recursive_mutexattr_s;
static pthread_mutexattr_t __ptw32_errorcheck_mutexattr = &__ptw32_errorcheck_mutexattr_s;

#if defined(NEED_CALLOC)
	void * __ptw32_calloc(size_t n, size_t s)
	{
		unsigned int m = n * s;
		void * p = SAlloc::M(m);
		if(!p)
			return NULL;
		memzero(p, m);
		return p;
	}
#endif

pthread_t __ptw32_new()
{
	pthread_t nil = {NULL, 0};
	__ptw32_thread_t * tp;
	/*
	 * If there's a reusable pthread_t then use it.
	 */
	pthread_t t = __ptw32_threadReusePop();
	if(t.p) {
		tp = static_cast<__ptw32_thread_t *>(t.p);
	}
	else {
		/* No reuse threads available */
		tp = static_cast<__ptw32_thread_t *>(SAlloc::C(1, sizeof(__ptw32_thread_t)));
		if(tp == NULL)
			return nil;
		/* ptHandle.p needs to point to it's parent __ptw32_thread_t. */
		t.p = tp->ptHandle.p = tp;
		t.x = tp->ptHandle.x = 0;
	}
	/* Set default state. */
	tp->seqNumber = ++__ptw32_threadSeqNumber;
	tp->sched_priority = THREAD_PRIORITY_NORMAL;
	tp->detachState = PTHREAD_CREATE_JOINABLE;
	tp->cancelState = PTHREAD_CANCEL_ENABLE;
	tp->cancelType = PTHREAD_CANCEL_DEFERRED;
	tp->stateLock = 0;
	tp->threadLock = 0;
	tp->robustMxListLock = 0;
	tp->robustMxList = NULL;
	tp->name = NULL;
#if defined(HAVE_CPU_AFFINITY)
	CPU_ZERO(reinterpret_cast<cpu_set_t *>(&tp->cpuset));
#endif
	tp->cancelEvent = CreateEvent(0, (int)TRUE/* manualReset  */, (int)FALSE/* setSignaled  */, NULL);
	if(tp->cancelEvent == NULL) {
		__ptw32_threadReusePush(tp->ptHandle);
		return nil;
	}
	return t;
}
/*
 * __ptw32_getprocessors()
 *
 * Get the number of CPUs available to the process.
 *
 * If the available number of CPUs is 1 then pthread_spin_lock()
 * will block rather than spin if the lock is already owned.
 *
 * pthread_spin_init() calls this routine when initialising
 * a spinlock. If the number of available processors changes
 * (after a call to SetProcessAffinityMask()) then only
 * newly initialised spinlocks will notice.
 */
int __ptw32_getprocessors(int * count)
{
	DWORD_PTR vProcessCPUs;
	DWORD_PTR vSystemCPUs;
	int result = 0;
#if defined(NEED_PROCESS_AFFINITY_MASK)
	*count = 1;
#else
	if(GetProcessAffinityMask(GetCurrentProcess(), &vProcessCPUs, &vSystemCPUs)) {
		int CPUs = 0;
		for(DWORD_PTR bit = 1; bit != 0; bit <<= 1) {
			if(vProcessCPUs & bit)
				CPUs++;
		}
		*count = CPUs;
	}
	else
		result = EAGAIN;
#endif
	return result;
}
//
// Descr: Return 0 if the attr object is valid, non-zero otherwise. 
//
int FASTCALL __ptw32_is_attr(const pthread_attr_t * attr)
{
	return (attr == NULL || *attr == NULL || (*attr)->valid !=  __PTW32_ATTR_VALID);
}

INLINE int __ptw32_cond_check_need_init(pthread_cond_t * cond)
{
	int result = 0;
	__ptw32_mcs_local_node_t node;
	/*
	 * The following guarded test is specifically for statically
	 * initialised condition variables (via PTHREAD_OBJECT_INITIALIZER).
	 */
	__ptw32_mcs_lock_acquire(&__ptw32_cond_test_init_lock, &node);
	/*
	 * We got here possibly under race
	 * conditions. Check again inside the critical section.
	 * If a static cv has been destroyed, the application can
	 * re-initialise it only by calling pthread_cond_init()
	 * explicitly.
	 */
	if(*cond == PTHREAD_COND_INITIALIZER) {
		result = pthread_cond_init(cond, NULL);
	}
	else if(*cond == NULL) {
		/*
		 * The cv has been destroyed while we were waiting to
		 * initialise it, so the operation that caused the
		 * auto-initialisation should fail.
		 */
		result = EINVAL;
	}
	__ptw32_mcs_lock_release(&node);
	return result;
}

#if defined(__PTW32_CLEANUP_CXX)
	#if defined(_MSC_VER)
		#include <eh.h>
	#elif defined(__WATCOMC__)
		#include <eh.h>
		#include <exceptio.h>
	#else
		#if defined(__GNUC__) && __GNUC__ < 3
			#include <new.h>
		#else
			#include <new>
			using std::terminate;
		#endif
	#endif
#endif
/*
 * -------------------------------------------------------------------
 * DOCPRIVATE
 *
 * This the routine runs through all thread keys and calls
 * the destroy routines on the user's data for the current thread.
 * It simulates the behaviour of POSIX Threads.
 *
 * PARAMETERS
 *     thread
 *             an instance of pthread_t
 *
 * RETURNS
 *     N/A
 * -------------------------------------------------------------------
 */
void __ptw32_callUserDestroyRoutines(pthread_t thread)
{
	ThreadKeyAssoc * assoc;
	if(thread.p) {
		__ptw32_mcs_local_node_t threadLock;
		__ptw32_mcs_local_node_t keyLock;
		int assocsRemaining;
		int iterations = 0;
		__ptw32_thread_t * sp = static_cast<__ptw32_thread_t *>(thread.p);
		/*
		 * Run through all Thread<-->Key associations for the current thread.
		 *
		 * Do this process at most PTHREAD_DESTRUCTOR_ITERATIONS times.
		 */
		do {
			assocsRemaining = 0;
			iterations++;
			__ptw32_mcs_lock_acquire(&(sp->threadLock), &threadLock);
			/*
			 * The pointer to the next assoc is stored in the thread struct so that
			 * the assoc destructor in pthread_key_delete can adjust it
			 * if it deletes this assoc. This can happen if we fail to acquire
			 * both locks below, and are forced to release all of our locks,
			 * leaving open the opportunity for pthread_key_delete to get in
			 * before us.
			 */
			sp->nextAssoc = sp->keys;
			__ptw32_mcs_lock_release(&threadLock);
			for(;;) {
				void * value;
				pthread_key_t k;
				void (* destructor)(void *);
				/*
				 * First we need to serialise with pthread_key_delete by locking
				 * both assoc guards, but in the reverse order to our convention,
				 * so we must be careful to avoid deadlock.
				 */
				__ptw32_mcs_lock_acquire(&(sp->threadLock), &threadLock);
				if((assoc = static_cast<ThreadKeyAssoc *>(sp->nextAssoc)) == NULL) {
					/* Finished */
					__ptw32_mcs_lock_release(&threadLock);
					break;
				}
				else {
					/*
					 * assoc->key must be valid because assoc can't change or be
					 * removed from our chain while we hold at least one lock. If
					 * the assoc was on our key chain then the key has not been
					 * deleted yet.
					 *
					 * Now try to acquire the second lock without deadlocking.
					 * If we fail, we need to relinquish the first lock and the
					 * processor and then try to acquire them all again.
					 */
					if(__ptw32_mcs_lock_try_acquire(&(assoc->key->keyLock), &keyLock) == EBUSY) {
						__ptw32_mcs_lock_release(&threadLock);
						Sleep(0);
						/*
						 * Go around again.
						 * If pthread_key_delete has removed this assoc in the meantime,
						 * sp->nextAssoc will point to a new assoc.
						 */
						continue;
					}
				}
				/* We now hold both locks */
				sp->nextAssoc = assoc->nextKey;
				/*
				 * Key still active; pthread_key_delete
				 * will block on these same mutexes before
				 * it can release actual key; therefore,
				 * key is valid and we can call the destroy
				 * routine;
				 */
				k = assoc->key;
				destructor = k->destructor;
				value = TlsGetValue(k->key);
				TlsSetValue(k->key, NULL);
				// Every assoc->key exists and has a destructor
				if(value && iterations <= PTHREAD_DESTRUCTOR_ITERATIONS) {
					/*
					 * Unlock both locks before the destructor runs.
					 * POSIX says pthread_key_delete can be run from destructors,
					 * and that probably includes with this key as target.
					 * pthread_setspecific can also be run from destructors and
					 * also needs to be able to access the assocs.
					 */
					__ptw32_mcs_lock_release(&threadLock);
					__ptw32_mcs_lock_release(&keyLock);
					assocsRemaining++;
#if defined(__cplusplus) && !defined(SLIBINCLUDED)
					try {
						/*
						 * Run the caller's cleanup routine.
						 */
						destructor(value);
					}
					catch(...) {
						/*
						 * A system unexpected exception has occurred
						 * running the user's destructor.
						 * We get control back within this block in case
						 * the application has set up it's own terminate
						 * handler. Since we are leaving the thread we
						 * should not get any internal pthreads
						 * exceptions.
						 */
						terminate();
					}
#else /* __cplusplus */
					destructor(value); // Run the caller's cleanup routine.
#endif /* __cplusplus */
				}
				else {
					//
					// Remove association from both the key and thread chains and reclaim it's memory resources.
					//
					__ptw32_tkAssocDestroy(assoc);
					__ptw32_mcs_lock_release(&threadLock);
					__ptw32_mcs_lock_release(&keyLock);
				}
			}
		} while(assocsRemaining);
	}
}
/*
 * About MCS locks:
 *
 * MCS locks are queue-based locks, where the queue nodes are local to the
 * thread. The 'lock' is nothing more than a global pointer that points to
 * the last node in the queue, or is NULL if the queue is empty.
 *
 * Originally designed for use as spin locks requiring no kernel resources
 * for synchronisation or blocking, the implementation below has adapted
 * the MCS spin lock for use as a general mutex that will suspend threads
 * when there is lock contention.
 *
 * Because the queue nodes are thread-local, most of the memory read/write
 * operations required to add or remove nodes from the queue do not trigger
 * cache-coherence updates.
 *
 * Like 'named' mutexes, MCS locks consume system resources transiently -
 * they are able to acquire and free resources automatically - but MCS
 * locks do not require any unique 'name' to identify the lock to all
 * threads using it.
 *
 * Usage of MCS locks:
 *
 * - you need a global __ptw32_mcs_lock_t instance initialised to 0 or NULL.
 * - you need a local thread-scope __ptw32_mcs_local_node_t instance, which
 *   may serve several different locks but you need at least one node for
 *   every lock held concurrently by a thread.
 *
 * E.g.:
 *
 * __ptw32_mcs_lock_t lock1 = 0;
 * __ptw32_mcs_lock_t lock2 = 0;
 *
 * void *mythread(void *arg)
 * {
 *   __ptw32_mcs_local_node_t node;
 *
 *   __ptw32_mcs_acquire (&lock1, &node);
 *   __ptw32_mcs_lock_release (&node);
 *
 *   __ptw32_mcs_lock_acquire (&lock2, &node);
 *   __ptw32_mcs_lock_release (&node);
 *   {
 *   __ptw32_mcs_local_node_t nodex;
 *
 *   __ptw32_mcs_lock_acquire (&lock1, &node);
 *   __ptw32_mcs_lock_acquire (&lock2, &nodex);
 *
 *   __ptw32_mcs_lock_release (&nodex);
 *   __ptw32_mcs_lock_release (&node);
 *   }
 *   return (void *)0;
 * }
 *
 */
/*
 * __ptw32_mcs_flag_set -- notify another thread about an event.
 *
 * Set event if an event handle has been stored in the flag, and
 * set flag to -1 otherwise. Note that -1 cannot be a valid handle value.
 */
INLINE void __ptw32_mcs_flag_set(HANDLE * flag)
{
	HANDLE e = (HANDLE)(__PTW32_INTERLOCKED_SIZE)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(
		(__PTW32_INTERLOCKED_SIZEPTR)flag, (__PTW32_INTERLOCKED_SIZE)-1, (__PTW32_INTERLOCKED_SIZE)0);
	/*
	 * NOTE: when e == -1 and the MSVC debugger is attached to
	 *    the process, we get an exception that halts the
	 *    program noting that the handle value is invalid;
	 *    although innocuous this behavior is cumbersome when
	 *    debugging.  Therefore we avoid calling SetEvent()
	 *    for 'known' invalid HANDLE values that can arise
	 *    when the above interlocked-compare-and-exchange
	 *    is executed.
	 */
	if(((HANDLE)0 != e) && ((HANDLE)-1 != e)) {
		/* another thread has already stored an event handle in the flag */
		SetEvent(e);
	}
}
/*
 * __ptw32_mcs_flag_wait -- wait for notification from another.
 *
 * Store an event handle in the flag and wait on it if the flag has not been
 * set, and proceed without creating an event otherwise.
 */
INLINE void __ptw32_mcs_flag_wait(HANDLE * flag)
{
	if((__PTW32_INTERLOCKED_SIZE)0 == __PTW32_INTERLOCKED_EXCHANGE_ADD_SIZE((__PTW32_INTERLOCKED_SIZEPTR)flag, (__PTW32_INTERLOCKED_SIZE)0)) { /* MBR fence */
		/* the flag is not set. create event. */
		HANDLE e = CreateEvent(NULL,  FALSE,  FALSE, NULL);
		if((__PTW32_INTERLOCKED_SIZE)0 ==  __PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE((__PTW32_INTERLOCKED_SIZEPTR)flag, (__PTW32_INTERLOCKED_SIZE)e, (__PTW32_INTERLOCKED_SIZE)0)) {
			/* stored handle in the flag. wait on it now. */
			WaitForSingleObject(e, INFINITE);
		}
		CloseHandle(e);
	}
}
/*
 * __ptw32_mcs_lock_acquire -- acquire an MCS lock.
 *
 * See:
 * J. M. Mellor-Crummey and M. L. Scott.
 * Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors.
 * ACM Transactions on Computer Systems, 9(1):21-65, Feb. 1991.
 */
#if defined (__PTW32_BUILD_INLINED)
	INLINE
#endif /* __PTW32_BUILD_INLINED */
void __ptw32_mcs_lock_acquire(__ptw32_mcs_lock_t * lock, __ptw32_mcs_local_node_t * node)
{
	node->lock = lock;
	node->nextFlag = 0;
	node->readyFlag = 0;
	node->next = 0; /* initially, no successor */
	/* queue for the lock */
	__ptw32_mcs_local_node_t * pred = static_cast<__ptw32_mcs_local_node_t *>(
		__PTW32_INTERLOCKED_EXCHANGE_PTR((__PTW32_INTERLOCKED_PVOID_PTR)lock, (__PTW32_INTERLOCKED_PVOID)node));
	if(pred) {
		/* the lock was not free. link behind predecessor. */
		__PTW32_INTERLOCKED_EXCHANGE_PTR((__PTW32_INTERLOCKED_PVOID_PTR)&pred->next,  (__PTW32_INTERLOCKED_PVOID)node);
		__ptw32_mcs_flag_set(&pred->nextFlag);
		__ptw32_mcs_flag_wait(&node->readyFlag);
	}
}
/*
 * __ptw32_mcs_lock_release -- release an MCS lock.
 *
 * See:
 * J. M. Mellor-Crummey and M. L. Scott.
 * Algorithms for Scalable Synchronization on Shared-Memory Multiprocessors.
 * ACM Transactions on Computer Systems, 9(1):21-65, Feb. 1991.
 */
#if defined (__PTW32_BUILD_INLINED)
	INLINE
#endif
void __ptw32_mcs_lock_release(__ptw32_mcs_local_node_t * node)
{
	__ptw32_mcs_lock_t * lock = node->lock;
	__ptw32_mcs_local_node_t * next = (__ptw32_mcs_local_node_t*)__PTW32_INTERLOCKED_EXCHANGE_ADD_SIZE((__PTW32_INTERLOCKED_SIZEPTR)&node->next,  (__PTW32_INTERLOCKED_SIZE)0); // MBR fence 
	if(0 == next) {
		/* no known successor */
		if(node == (__ptw32_mcs_local_node_t*)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR((__PTW32_INTERLOCKED_PVOID_PTR)lock, (__PTW32_INTERLOCKED_PVOID)0, (__PTW32_INTERLOCKED_PVOID)node)) {
			return; // no successor, lock is free now 
		}
		// wait for successor 
		__ptw32_mcs_flag_wait(&node->nextFlag);
		next = (__ptw32_mcs_local_node_t*)__PTW32_INTERLOCKED_EXCHANGE_ADD_SIZE((__PTW32_INTERLOCKED_SIZEPTR)&node->next,  (__PTW32_INTERLOCKED_SIZE)0); // MBR fence
	}
	else {
		// Even if the next is non-0, the successor may still be trying to set the next flag on us, therefore we must wait. 
		__ptw32_mcs_flag_wait(&node->nextFlag);
	}
	// pass the lock 
	__ptw32_mcs_flag_set(&next->readyFlag);
}
// 
// 
//
#if defined (__PTW32_BUILD_INLINED)
	INLINE
#endif
int __ptw32_mcs_lock_try_acquire(__ptw32_mcs_lock_t * lock, __ptw32_mcs_local_node_t * node)
{
	node->lock = lock;
	node->nextFlag = 0;
	node->readyFlag = 0;
	node->next = 0; /* initially, no successor */
	return ((__PTW32_INTERLOCKED_PVOID)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR((__PTW32_INTERLOCKED_PVOID_PTR)lock,
	       (__PTW32_INTERLOCKED_PVOID)node, (__PTW32_INTERLOCKED_PVOID)0) ==  (__PTW32_INTERLOCKED_PVOID)0) ? 0 : EBUSY;
}
/*
 * __ptw32_mcs_node_transfer -- move an MCS lock local node, usually from thread
 * space to, for example, global space so that another thread can release
 * the lock on behalf of the current lock owner.
 *
 * Example: used in pthread_barrier_wait where we want the last thread out of
 * the barrier to release the lock owned by the last thread to enter the barrier
 * (the one that releases all threads but not necessarily the last to leave).
 *
 * Should only be called by the thread that has the lock.
 */
#if defined (__PTW32_BUILD_INLINED)
INLINE
#endif /* __PTW32_BUILD_INLINED */
void __ptw32_mcs_node_transfer(__ptw32_mcs_local_node_t * new_node, __ptw32_mcs_local_node_t * old_node)
{
	new_node->lock = old_node->lock;
	new_node->nextFlag = 0; /* Not needed - used only in initial Acquire */
	new_node->readyFlag = 0; /* Not needed - we were waiting on this */
	new_node->next = 0;
	if((__ptw32_mcs_local_node_t*)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR((__PTW32_INTERLOCKED_PVOID_PTR)new_node->lock, (__PTW32_INTERLOCKED_PVOID)new_node, (__PTW32_INTERLOCKED_PVOID)old_node)
	    != old_node) {
		/*
		 * A successor has queued after us, so wait for them to link to us
		 */
		while(0 == old_node->next) {
			sched_yield();
		}
		/* we must wait for the next Node to finish inserting itself. */
		__ptw32_mcs_flag_wait(&old_node->nextFlag);
		/*
		 * Copy the nextFlag state also so we don't block on it when releasing this lock.
		 */
		new_node->next = old_node->next;
		new_node->nextFlag = old_node->nextFlag;
	}
}

INLINE int __ptw32_mutex_check_need_init(pthread_mutex_t * mutex)
{
	int result = 0;
	pthread_mutex_t mtx;
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&__ptw32_mutex_test_init_lock, &node);
	/*
	 * We got here possibly under race
	 * conditions. Check again inside the critical section
	 * and only initialise if the mutex is valid (not been destroyed).
	 * If a static mutex has been destroyed, the application can
	 * re-initialise it only by calling pthread_mutex_init()
	 * explicitly.
	 */
	mtx = *mutex;
	if(mtx == PTHREAD_MUTEX_INITIALIZER) {
		result = pthread_mutex_init(mutex, NULL);
	}
	else if(mtx == PTHREAD_RECURSIVE_MUTEX_INITIALIZER) {
		result = pthread_mutex_init(mutex, &__ptw32_recursive_mutexattr);
	}
	else if(mtx == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		result = pthread_mutex_init(mutex, &__ptw32_errorcheck_mutexattr);
	}
	else if(mtx == NULL) {
		/*
		 * The mutex has been destroyed while we were waiting to
		 * initialise it, so the operation that caused the
		 * auto-initialisation should fail.
		 */
		result = EINVAL;
	}
	__ptw32_mcs_lock_release(&node);
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPRIVATE
 *   This function performs process wide initialization for
 *   the pthread library.
 *
 * PARAMETERS
 *   N/A
 *
 * DESCRIPTION
 *   This function performs process wide initialization for
 *   the pthread library.
 *   If successful, this routine sets the global variable
 *   __ptw32_processInitialized to TRUE.
 *
 * RESULTS
 *     TRUE    if successful,
 *     FALSE   otherwise
 *
 * ------------------------------------------------------
 */
int __ptw32_processInitialize()
{
	if(__ptw32_processInitialized) {
		return TRUE;
	}
	/*
	 * Explicitly initialise all variables from global.c
	 */
	__ptw32_threadReuseTop =  __PTW32_THREAD_REUSE_EMPTY;
	__ptw32_threadReuseBottom =  __PTW32_THREAD_REUSE_EMPTY;
	__ptw32_selfThreadKey = NULL;
	__ptw32_cleanupKey = NULL;
	__ptw32_cond_list_head = NULL;
	__ptw32_cond_list_tail = NULL;
	__ptw32_concurrency = 0;
	// What features have been auto-detected 
	__ptw32_features = 0;
	__ptw32_threadSeqNumber = 0; // Global [process wide] thread sequence Number
	/*
	 * Function pointer to QueueUserAPCEx if it exists, otherwise
	 * it will be set at runtime to a substitute routine which cannot unblock blocked threads.
	 */
	__ptw32_register_cancellation = NULL;
	__ptw32_thread_reuse_lock = 0; // Global lock for managing pthread_t struct reuse.
	__ptw32_mutex_test_init_lock = 0; // Global lock for testing internal state of statically declared mutexes.
	__ptw32_cond_test_init_lock = 0; // Global lock for testing internal state of PTHREAD_COND_INITIALIZER created condition variables.
	__ptw32_rwlock_test_init_lock = 0; // Global lock for testing internal state of PTHREAD_RWLOCK_INITIALIZER created read/write locks.
	__ptw32_spinlock_test_init_lock = 0; // Global lock for testing internal state of PTHREAD_SPINLOCK_INITIALIZER created spin locks.
	/*
	 * Global lock for condition variable linked list. The list exists
	 * to wake up CVs when a WM_TIMECHANGE message arrives. See
	 * w32_TimeChangeHandler.c.
	 */
	__ptw32_cond_list_lock = 0;
  #if defined(_UWIN)
	pthread_count = 0; // Keep a count of the number of threads.
  #endif
	__ptw32_processInitialized =  TRUE;
	/*
	 * Initialize Keys
	 */
	if((pthread_key_create(&__ptw32_selfThreadKey, NULL) != 0) || (pthread_key_create(&__ptw32_cleanupKey, NULL) != 0)) {
		__ptw32_processTerminate();
	}
	return (__ptw32_processInitialized);
}
/*
 * ------------------------------------------------------
 * DOCPRIVATE
 *   This function performs process wide termination for
 *   the pthread library.
 *
 * PARAMETERS
 *   N/A
 *
 * DESCRIPTION
 *   This function performs process wide termination for
 *   the pthread library.
 *   This routine sets the global variable
 *   __ptw32_processInitialized to FALSE
 *
 * RESULTS
 *     N/A
 *
 * ------------------------------------------------------
 */
void __ptw32_processTerminate()
{
	if(__ptw32_processInitialized) {
		__ptw32_thread_t * tp, * tpNext;
		__ptw32_mcs_local_node_t node;
		if(__ptw32_selfThreadKey) {
			/*
			 * Release __ptw32_selfThreadKey
			 */
			pthread_key_delete(__ptw32_selfThreadKey);
			__ptw32_selfThreadKey = NULL;
		}
		if(__ptw32_cleanupKey) {
			/*
			 * Release __ptw32_cleanupKey
			 */
			pthread_key_delete(__ptw32_cleanupKey);
			__ptw32_cleanupKey = NULL;
		}
		__ptw32_mcs_lock_acquire(&__ptw32_thread_reuse_lock, &node);
		tp = __ptw32_threadReuseTop;
		while(tp !=  __PTW32_THREAD_REUSE_EMPTY) {
			tpNext = tp->prevReuse;
			SAlloc::F(tp);
			tp = tpNext;
		}
		__ptw32_mcs_lock_release(&node);
		__ptw32_processInitialized =  FALSE;
	}
}

#if defined (__PTW32_BUILD_INLINED)
	INLINE
#endif /* __PTW32_BUILD_INLINED */
DWORD __ptw32_relmillisecs(const struct timespec * abstime)
{
	DWORD milliseconds;
	int64_t tmpAbsNanoseconds;
	int64_t tmpCurrNanoseconds;
	struct timespec currSysTime;
	FILETIME ft;
#if defined(WINCE)
	SYSTEMTIME st;
#endif
	/*
	 * Calculate timeout as milliseconds from current system time.
	 */
	/*
	 * subtract current system time from abstime in a way that checks
	 * that abstime is never in the past, or is never equivalent to the
	 * defined INFINITE value (0xFFFFFFFF).
	 *
	 * Assume all integers are unsigned, i.e. cannot test if less than 0.
	 */
	tmpAbsNanoseconds = (int64_t)abstime->tv_nsec + ((int64_t)abstime->tv_sec * NANOSEC_PER_SEC);
	// get current system time 
#if defined(WINCE)
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
#else
	GetSystemTimeAsFileTime(&ft);
#endif
	__ptw32_filetime_to_timespec(&ft, &currSysTime);
	tmpCurrNanoseconds = (int64_t)currSysTime.tv_nsec + ((int64_t)currSysTime.tv_sec * NANOSEC_PER_SEC);
	if(tmpAbsNanoseconds > tmpCurrNanoseconds) {
		int64_t deltaNanoseconds = tmpAbsNanoseconds - tmpCurrNanoseconds;
		if(deltaNanoseconds >= ((int64_t)INFINITE * NANOSEC_PER_MILLISEC))
			milliseconds = INFINITE - 1; // Timeouts must be finite 
		else
			milliseconds = (DWORD)(deltaNanoseconds / NANOSEC_PER_MILLISEC);
	}
	else
		milliseconds = 0; // The abstime given is in the past 
	if(milliseconds == 0 && tmpAbsNanoseconds > tmpCurrNanoseconds) {
		/*
		 * millisecond granularity was too small to represent the wait time.
		 * return the minimum time in milliseconds.
		 */
		milliseconds = 1;
	}
	return milliseconds;
}
/*
 * Return the first parameter "abstime" modified to represent the current system time.
 * If "relative" is not NULL it represents an interval to add to "abstime".
 */
struct timespec * pthread_win32_getabstime_np(struct timespec * abstime, const struct timespec * relative)                  
{
	int64_t sec;
	int64_t nsec;
	struct timespec currSysTime;
	FILETIME ft;
	// get current system time 
#if defined(WINCE)
	SYSTEMTIME st;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
#else
	GetSystemTimeAsFileTime(&ft);
#endif
	__ptw32_filetime_to_timespec(&ft, &currSysTime);
	sec = currSysTime.tv_sec;
	nsec = currSysTime.tv_nsec;
	if(relative) {
		nsec += relative->tv_nsec;
		if(nsec >= NANOSEC_PER_SEC) {
			sec++;
			nsec -= NANOSEC_PER_SEC;
		}
		sec += relative->tv_sec;
	}
	abstime->tv_sec = (time_t)sec;
	abstime->tv_nsec = (long)nsec;
	return abstime;
}
/*
 * How it works:
 * A pthread_t is a struct (2x32 bit scalar types on x86, 2x64 bit on x86_64)
 * [FIXME: This is not true, x86_64 is 64 bit pointer and 32 bit counter. This should be fixed in version 3.0.0]
 * which is normally passed/returned by value to/from pthreads routines.
 * Applications are therefore storing a copy of the struct as it is at that time.
 *
 * The original pthread_t struct plus all copies of it contain the address of
 * the thread state struct __ptw32_thread_t_ (p), plus a reuse counter (x). Each
 * __ptw32_thread_t contains the original copy of it's pthread_t (ptHandle).
 * Once malloced, a __ptw32_thread_t_ struct is not freed until the process exits.
 *
 * The thread reuse stack is a simple LILO stack managed through a singly
 * linked list element in the __ptw32_thread_t.
 *
 * Each time a thread is destroyed, the __ptw32_thread_t address is pushed onto the
 * reuse stack after it's ptHandle's reuse counter has been incremented.
 *
 * The following can now be said from this:
 * - two pthread_t's refer to the same thread iff their __ptw32_thread_t reference
 * pointers are equal and their reuse counters are equal. That is,
 *
 *   equal = (a.p == b.p && a.x == b.x)
 *
 * - a pthread_t copy refers to a destroyed thread if the reuse counter in
 * the copy is not equal to (i.e less than) the reuse counter in the original.
 *
 *   threadDestroyed = (copy.x != ((__ptw32_thread_t *)copy.p)->ptHandle.x)
 *
 */
/*
 * Pop a clean pthread_t struct off the reuse stack.
 */
pthread_t __ptw32_threadReusePop()
{
	pthread_t t = {NULL, 0};
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&__ptw32_thread_reuse_lock, &node);
	if(__PTW32_THREAD_REUSE_EMPTY != __ptw32_threadReuseTop) {
		__ptw32_thread_t * tp = __ptw32_threadReuseTop;
		__ptw32_threadReuseTop = tp->prevReuse;
		if(__PTW32_THREAD_REUSE_EMPTY == __ptw32_threadReuseTop) {
			__ptw32_threadReuseBottom =  __PTW32_THREAD_REUSE_EMPTY;
		}
		tp->prevReuse = NULL;
		t = tp->ptHandle;
	}
	__ptw32_mcs_lock_release(&node);
	return t;
}
/*
 * Push a clean pthread_t struct onto the reuse stack.
 * Must be re-initialised when reused.
 * All object elements (mutexes, events etc) must have been either
 * destroyed before this, or never initialised.
 */
void __ptw32_threadReusePush(pthread_t thread)
{
	__ptw32_thread_t * tp = static_cast<__ptw32_thread_t *>(thread.p);
	pthread_t t;
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&__ptw32_thread_reuse_lock, &node);
	t = tp->ptHandle;
	memzero(tp, sizeof(__ptw32_thread_t));
	/* Must restore the original POSIX handle that we just wiped. */
	tp->ptHandle = t;
	/* Bump the reuse counter now */
#if defined (__PTW32_THREAD_ID_REUSE_INCREMENT)
	tp->ptHandle.x +=  __PTW32_THREAD_ID_REUSE_INCREMENT;
#else
	tp->ptHandle.x++;
#endif
	tp->state = PThreadStateReuse;
	tp->prevReuse =  __PTW32_THREAD_REUSE_EMPTY;
	if(__PTW32_THREAD_REUSE_EMPTY != __ptw32_threadReuseBottom) {
		__ptw32_threadReuseBottom->prevReuse = tp;
	}
	else {
		__ptw32_threadReuseTop = tp;
	}
	__ptw32_threadReuseBottom = tp;
	__ptw32_mcs_lock_release(&node);
}
/*
 * __ptw32_throw
 *
 * All cancelled and explicitly exited POSIX threads go through
 * here. This routine knows how to exit both POSIX initiated threads and
 * 'implicit' POSIX threads for each of the possible language modes (C,
 * C++, and SEH).
 */
void __ptw32_throw(DWORD exception)
{
	/*
	 * Don't use pthread_self() to avoid creating an implicit POSIX thread handle
	 * unnecessarily.
	 */
	__ptw32_thread_t * sp = static_cast<__ptw32_thread_t *>(pthread_getspecific(__ptw32_selfThreadKey));
#if defined(__PTW32_CLEANUP_SEH)
	DWORD exceptionInformation[3];
#endif
	sp->state = PThreadStateExiting;
	if(exception !=  __PTW32_EPS_CANCEL && exception !=  __PTW32_EPS_EXIT) {
		/* Should never enter here */
		exit(1);
	}
	if(!sp || sp->implicit) {
		/*
		 * We're inside a non-POSIX initialised Win32 thread
		 * so there is no point to jump or throw back to. Just do an
		 * explicit thread exit here after cleaning up POSIX
		 * residue (i.e. cleanup handlers, POSIX thread handle etc).
		 */
#if !defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
		unsigned exitCode = 0;
		switch(exception) {
			case  __PTW32_EPS_CANCEL:
			    exitCode = (uint)(size_t)PTHREAD_CANCELED;
			    break;
			case  __PTW32_EPS_EXIT:
			    if(sp)
				    exitCode = (uint)(size_t)sp->exitStatus;
			    break;
		}
#endif
#if defined (__PTW32_STATIC_LIB)
		pthread_win32_thread_detach_np();
#endif
#if !defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
		_endthreadex(exitCode);
#else
		_endthread();
#endif
	}
#if defined(__PTW32_CLEANUP_SEH)
	exceptionInformation[0] = (DWORD)(exception);
	exceptionInformation[1] = (DWORD)(0);
	exceptionInformation[2] = (DWORD)(0);
	RaiseException(EXCEPTION_PTW32_SERVICES, 0, 3, (ULONG_PTR*)exceptionInformation);
#else /* __PTW32_CLEANUP_SEH */

#if defined(__PTW32_CLEANUP_C)

	__ptw32_pop_cleanup_all(1);
	longjmp(sp->start_mark, exception);
#else /* __PTW32_CLEANUP_C */
#if defined(__PTW32_CLEANUP_CXX)
	switch(exception) {
		case  __PTW32_EPS_CANCEL:
		    throw __ptw32_exception_cancel();
		    break;
		case  __PTW32_EPS_EXIT:
		    throw __ptw32_exception_exit();
		    break;
	}
#else
#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.
#endif /* __PTW32_CLEANUP_CXX */
#endif /* __PTW32_CLEANUP_C */
#endif /* __PTW32_CLEANUP_SEH */
	/* Never reached */
}

void __ptw32_pop_cleanup_all(int execute)
{
	while(__ptw32_pop_cleanup(execute)) {
		;
	}
}

DWORD __ptw32_get_exception_services_code()
{
#if defined(__PTW32_CLEANUP_SEH)
	return EXCEPTION_PTW32_SERVICES;
#else
	return (DWORD)0;
#endif
}

#if defined(__PTW32_CLEANUP_SEH)
	static DWORD ExceptionFilter(EXCEPTION_POINTERS * ep, DWORD * ei)
	{
		switch(ep->ExceptionRecord->ExceptionCode) {
			case EXCEPTION_PTW32_SERVICES:
			{
				DWORD param;
				DWORD numParams = ep->ExceptionRecord->NumberParameters;
				numParams = (numParams > 3) ? 3 : numParams;
				for(param = 0; param < numParams; param++) {
					ei[param] = (DWORD)ep->ExceptionRecord->ExceptionInformation[param];
				}
				return EXCEPTION_EXECUTE_HANDLER;
				break;
			}
			default:
			{
				/*
			 * A system unexpected exception has occurred running the user's
			 * routine. We need to cleanup before letting the exception
			 * out of thread scope.
				 */
				pthread_t self = pthread_self();
				__ptw32_callUserDestroyRoutines(self);
				return EXCEPTION_CONTINUE_SEARCH;
				break;
			}
		}
	}
#elif defined(__PTW32_CLEANUP_CXX)
	#if defined(_MSC_VER)
		#include <eh.h>
	#elif defined(__WATCOMC__)
		#include <eh.h>
		#include <exceptio.h>
	#else
		#if defined(__GNUC__) && __GNUC__ < 3
			#include <new.h>
		#else
			#include <new>
			using std::terminate;
			using std::set_terminate;
		#endif
	#endif
#endif /* __PTW32_CLEANUP_CXX */
/*
 * MSVC6 does not optimize __ptw32_threadStart() safely
 * (i.e. tests/context1.c fails with "abnormal program
 * termination" in some configurations), and there's no
 * point to optimizing this routine anyway
 */
#ifdef _MSC_VER
	#pragma optimize("g", off)
	#pragma warning( disable : 4748 )
#endif

#if !defined (__MINGW32__) || (defined (__MSVCRT__) && !defined (__DMC__))
	unsigned __stdcall
#else
	void
#endif
__ptw32_threadStart(void * vthreadParms)
{
	ThreadParms * threadParms = (ThreadParms*)vthreadParms;
	pthread_t self;
	__ptw32_thread_t * sp;
	void *  (__PTW32_CDECL *start)(void *);
	void * arg;
#if defined(__PTW32_CLEANUP_SEH)
	DWORD ei[] = { 0, 0, 0 };
#endif
#if defined(__PTW32_CLEANUP_C)
	int setjmp_rc;
#endif
	__ptw32_mcs_local_node_t stateLock;
	void * status = (void *)0;
	self = threadParms->tid;
	sp = (__ptw32_thread_t *)self.p;
	start = threadParms->start;
	arg = threadParms->arg;
	SAlloc::F(threadParms);
#if !defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
#else
	/*
	 * _beginthread does not return the thread id and is running
	 * before it returns us the thread handle, and so we do it here.
	 */
	sp->thread = GetCurrentThreadId();
#endif
	pthread_setspecific(__ptw32_selfThreadKey, sp);
	/*
	 * Here we're using stateLock as a general-purpose lock
	 * to make the new thread wait until the creating thread
	 * has the new handle.
	 */
	__ptw32_mcs_lock_acquire(&sp->stateLock, &stateLock);
	sp->state = PThreadStateRunning;
	__ptw32_mcs_lock_release(&stateLock);
#if defined(__PTW32_CLEANUP_SEH)
	__try {
		/*
		 * Run the caller's routine;
		 */
		status = sp->exitStatus = (*start)(arg);
		sp->state = PThreadStateExiting;
#if defined(_UWIN)
		if(--pthread_count <= 0)
			exit(0);
#endif
	}
	__except(ExceptionFilter (GetExceptionInformation(), ei))
	{
		switch(ei[0]) {
			case  __PTW32_EPS_CANCEL:
			    status = sp->exitStatus = PTHREAD_CANCELED;
#if defined(_UWIN)
			    if(--pthread_count <= 0)
				    exit(0);
#endif
			    break;
			case  __PTW32_EPS_EXIT:
			    status = sp->exitStatus;
			    break;
			default:
			    status = sp->exitStatus = PTHREAD_CANCELED;
			    break;
		}
	}
#else /* __PTW32_CLEANUP_SEH */
	#if defined(__PTW32_CLEANUP_C)
		setjmp_rc = setjmp(sp->start_mark);
		if(0 == setjmp_rc) {
			/*
			 * Run the caller's routine;
			 */
			status = sp->exitStatus = (*start)(arg);
			sp->state = PThreadStateExiting;
		}
		else {
			switch(setjmp_rc) {
				case  __PTW32_EPS_CANCEL: status = sp->exitStatus = PTHREAD_CANCELED; break;
				case  __PTW32_EPS_EXIT: status = sp->exitStatus; break;
				default: status = sp->exitStatus = PTHREAD_CANCELED; break;
			}
		}
	#else /* __PTW32_CLEANUP_C */
		#if defined(__PTW32_CLEANUP_CXX)
			try {
				status = sp->exitStatus = (*start)(arg);
				sp->state = PThreadStateExiting;
			}
			catch(__ptw32_exception_cancel &) {
				/*
				 * Thread was canceled.
				 */
				status = sp->exitStatus = PTHREAD_CANCELED;
			}
			catch(__ptw32_exception_exit &) {
				/*
				 * Thread was exited via pthread_exit().
				 */
				status = sp->exitStatus;
			}
			catch(...) {
				/*
				 * Some other exception occurred. Clean up while we have
				 * the opportunity, and call the terminate handler.
				 */
				pthread_win32_thread_detach_np();
				terminate();
			}
		#else
			#error ERROR [__FILE__, line __LINE__]: Cleanup type undefined.
		#endif /* __PTW32_CLEANUP_CXX */
	#endif /* __PTW32_CLEANUP_C */
#endif /* __PTW32_CLEANUP_SEH */
#if defined (__PTW32_STATIC_LIB)
	/*
	 * We need to cleanup the pthread now if we have
	 * been statically linked, in which case the cleanup
	 * in DllMain won't get done. Joinable threads will
	 * only be partially cleaned up and must be fully cleaned
	 * up by pthread_join() or pthread_detach().
	 *
	 * Note: if this library has been statically linked,
	 * implicitly created pthreads (those created
	 * for Win32 threads which have called pthreads routines)
	 * must be cleaned up explicitly by the application
	 * by calling pthread_exit().
	 * For the dll, DllMain will do the cleanup automatically.
	 */
	pthread_win32_thread_detach_np();
#endif
#if !defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
	_endthreadex((uint)(size_t)status);
#else
	_endthread();
#endif
	/*
	 * Never reached.
	 */
#if !defined (__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
	return (uint)(size_t)status;
#endif
}
/*
 * Reset optimization
 */
#ifdef _MSC_VER
	#pragma optimize("", on)
#endif
#if defined  (__PTW32_USES_SEPARATE_CRT) && (defined(__PTW32_CLEANUP_CXX) || defined(__PTW32_CLEANUP_SEH))
__ptw32_terminate_handler pthread_win32_set_terminate_np(__ptw32_terminate_handler termFunction)
{
	return set_terminate(termFunction);
}
#endif

void __ptw32_threadDestroy(pthread_t thread)
{
	__ptw32_thread_t * tp = static_cast<__ptw32_thread_t *>(thread.p);
	if(tp) {
		/*
		 * Copy thread state so that the thread can be atomically NULLed.
		 */
#if !defined(__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
		HANDLE threadH = tp->threadH;
#endif
		HANDLE cancelEvent = tp->cancelEvent;
		/*
		 * Thread ID structs are never freed. They're NULLed and reused.
		 * This also sets the thread state to PThreadStateInitial before
		 * it is finally set to PThreadStateReuse.
		 */
		__ptw32_threadReusePush(thread);
		if(cancelEvent) {
			CloseHandle(cancelEvent);
		}
#if !defined(__MINGW32__) || defined (__MSVCRT__) || defined (__DMC__)
		/*
		 * See documentation for endthread vs endthreadex.
		 */
		if(threadH != 0) {
			CloseHandle(threadH);
		}
#endif
	}
}
/*
 * -------------------------------------------------------------------
 * This routine creates an association that
 * is unique for the given (thread,key) combination.The association
 * is referenced by both the thread and the key.
 * This association allows us to determine what keys the
 * current thread references and what threads a given key references.
 * See the detailed description
 * at the beginning of this file for further details.
 *
 * Notes:
 *   1)      New associations are pushed to the beginning of the
 *     chain so that the internal __ptw32_selfThreadKey association
 *     is always last, thus allowing selfThreadExit to
 *     be implicitly called last by pthread_exit.
 *   2)
 *
 * Parameters:
 *     thread
 *             current running thread.
 *     key
 *             key on which to create an association.
 * Returns:
 *    0              - if successful,
 *    ENOMEM         - not enough memory to create assoc or other object
 *    EINVAL         - an internal error occurred
 *    ENOSYS         - an internal error occurred
 * -------------------------------------------------------------------
 */
int __ptw32_tkAssocCreate(__ptw32_thread_t * sp, pthread_key_t key)
{
	/*
	 * Have to create an association and add it to both the key and the thread.
	 *
	 * Both key->keyLock and thread->threadLock are locked before entry to this routine.
	 */
	ThreadKeyAssoc * assoc = static_cast<ThreadKeyAssoc *>(SAlloc::C(1, sizeof(*assoc)));
	if(assoc == NULL) {
		return ENOMEM;
	}
	assoc->thread = sp;
	assoc->key = key;
	/*
	 * Register assoc with key
	 */
	assoc->prevThread = NULL;
	assoc->nextThread = static_cast<ThreadKeyAssoc *>(key->threads);
	if(assoc->nextThread)
		assoc->nextThread->prevThread = assoc;
	key->threads = (void *)assoc;
	/*
	 * Register assoc with thread
	 */
	assoc->prevKey = NULL;
	assoc->nextKey = static_cast<ThreadKeyAssoc *>(sp->keys);
	if(assoc->nextKey) {
		assoc->nextKey->prevKey = assoc;
	}
	sp->keys = (void *)assoc;
	return 0;
}
/*
 * This routine releases all resources for the given ThreadKeyAssoc
 * once it is no longer being referenced
 * ie) either the key or thread has stopped referencing it.
 *
 * Parameters:
 *     assoc
 *             an instance of ThreadKeyAssoc.
 * Returns:
 *   N/A
 */
void __ptw32_tkAssocDestroy(ThreadKeyAssoc * assoc)
{
	//
	// Both key->keyLock and thread->threadLock are locked before entry to this routine.
	//
	if(assoc) {
		// Remove assoc from thread's keys chain 
		ThreadKeyAssoc * prev = assoc->prevKey;
		ThreadKeyAssoc * next = assoc->nextKey;
		if(prev)
			prev->nextKey = next;
		if(next)
			next->prevKey = prev;
		if(assoc->thread->keys == assoc) {
			assoc->thread->keys = next; // We're at the head of the thread's keys chain 
		}
		if(assoc->thread->nextAssoc == assoc) {
			/*
			 * Thread is exiting and we're deleting the assoc to be processed next.
			 * Hand thread the assoc after this one.
			 */
			assoc->thread->nextAssoc = next;
		}
		// Remove assoc from key's threads chain 
		prev = assoc->prevThread;
		next = assoc->nextThread;
		if(prev)
			prev->nextThread = next;
		if(next)
			next->prevThread = prev;
		if(assoc->key->threads == assoc)
			assoc->key->threads = next; // We're at the head of the key's threads chain 
		SAlloc::F(assoc);
	}
}
/*
 * time between jan 1, 1601 and jan 1, 1970 in units of 100 nanoseconds
 */
//#define __PTW32_TIMESPEC_TO_FILETIME_OFFSET (((uint64_t)27111902UL << 32) + (uint64_t)3577643008UL)
/*
 * -------------------------------------------------------------------
 * converts struct timespec
 * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
 * into FILETIME (as set by GetSystemTimeAsFileTime), where the time is
 * expressed in 100 nanoseconds from Jan 1, 1601,
 * -------------------------------------------------------------------
 */
INLINE void __ptw32_timespec_to_filetime(const struct timespec * ts, FILETIME * ft)
{
	*reinterpret_cast<uint64_t *>(ft) = ts->tv_sec * 10000000UL + (ts->tv_nsec + 50) / 100 +  SlConst::Epoch1600_1970_Offs_100Ns;
}
/*
 * -------------------------------------------------------------------
 * converts FILETIME (as set by GetSystemTimeAsFileTime), where the time is
 * expressed in 100 nanoseconds from Jan 1, 1601,
 * into struct timespec
 * where the time is expressed in seconds and nanoseconds from Jan 1, 1970.
 * -------------------------------------------------------------------
 */
INLINE void __ptw32_filetime_to_timespec(const FILETIME * ft, struct timespec * ts)
{
	ts->tv_sec = (int)((*(uint64_t*)ft -  SlConst::Epoch1600_1970_Offs_100Ns) / 10000000UL);
	ts->tv_nsec = (int)((*(uint64_t*)ft -  SlConst::Epoch1600_1970_Offs_100Ns - ((uint64_t)ts->tv_sec * (uint64_t)10000000UL)) * 100);
}

INLINE int __ptw32_rwlock_check_need_init(pthread_rwlock_t * rwlock)
{
	int result = 0;
	__ptw32_mcs_local_node_t node;
	/*
	 * The following guarded test is specifically for statically
	 * initialised rwlocks (via PTHREAD_RWLOCK_INITIALIZER).
	 */
	__ptw32_mcs_lock_acquire(&__ptw32_rwlock_test_init_lock, &node);
	/*
	 * We got here possibly under race
	 * conditions. Check again inside the critical section
	 * and only initialise if the rwlock is valid (not been destroyed).
	 * If a static rwlock has been destroyed, the application can
	 * re-initialise it only by calling pthread_rwlock_init()
	 * explicitly.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = pthread_rwlock_init(rwlock, NULL);
	}
	else if(*rwlock == NULL) {
		/*
		 * The rwlock has been destroyed while we were waiting to
		 * initialise it, so the operation that caused the
		 * auto-initialisation should fail.
		 */
		result = EINVAL;
	}
	__ptw32_mcs_lock_release(&node);
	return result;
}

void __ptw32_rwlock_cancelwrwait(void * arg)
{
	pthread_rwlock_t rwl = (pthread_rwlock_t)arg;
	rwl->nSharedAccessCount = -rwl->nCompletedSharedAccessCount;
	rwl->nCompletedSharedAccessCount = 0;
	pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted));
	pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
}
#if !defined(_UWIN)
	//#include <process.h>
#endif
/*
 * ------------------------------------------------------
 * DESCRIPTION
 *   This function waits on a POSIX semaphore. If the
 *   semaphore value is greater than zero, it decreases
 *   its value by one. If the semaphore value is zero, then
 *   the calling thread (or process) is blocked until it can
 *   successfully decrease the value.
 *
 *   Unlike sem_wait(), this routine is non-cancelable.
 *
 * RESULTS
 *     0               successfully decreased semaphore,
 *     -1              failed, error in errno.
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOSYS          semaphores are not supported,
 *     EINTR           the function was interrupted by a signal,
 *     EDEADLK         a deadlock condition was detected.
 *
 * ------------------------------------------------------
 */
int __ptw32_semwait(sem_t * sem)
{
	__ptw32_mcs_local_node_t node;
	int v;
	int result = 0;
	sem_t s = *sem;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	v = --s->value;
	__ptw32_mcs_lock_release(&node);
	if(v < 0) {
		/* Must wait */
		if(WaitForSingleObject(s->sem, INFINITE) == WAIT_OBJECT_0) {
#if defined(NEED_SEM)
			__ptw32_mcs_lock_acquire(&s->lock, &node);
			if(s->leftToUnblock > 0) {
				--s->leftToUnblock;
				SetEvent(s->sem);
			}
			__ptw32_mcs_lock_release(&node);
#endif
			return 0;
		}
	}
	else {
		return 0;
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}

INLINE int __ptw32_spinlock_check_need_init(pthread_spinlock_t * lock)
{
	int result = 0;
	__ptw32_mcs_local_node_t node;
	/*
	 * The following guarded test is specifically for statically
	 * initialised spinlocks (via PTHREAD_SPINLOCK_INITIALIZER).
	 */
	__ptw32_mcs_lock_acquire(&__ptw32_spinlock_test_init_lock, &node);
	/*
	 * We got here possibly under race
	 * conditions. Check again inside the critical section
	 * and only initialise if the spinlock is valid (not been destroyed).
	 * If a static spinlock has been destroyed, the application can
	 * re-initialise it only by calling pthread_spin_init()
	 * explicitly.
	 */
	if(*lock == PTHREAD_SPINLOCK_INITIALIZER) {
		result = pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
	}
	else if(*lock == NULL) {
		/*
		 * The spinlock has been destroyed while we were waiting to
		 * initialise it, so the operation that caused the
		 * auto-initialisation should fail.
		 */
		result = EINVAL;
	}
	__ptw32_mcs_lock_release(&node);
	return result;
}
