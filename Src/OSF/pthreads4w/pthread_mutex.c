// pthread_mutex.c
//
/*
 * Description: This translation unit implements mutual exclusion (mutex) primitives.
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

int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr)
{
	int result = 0;
	pthread_mutex_t mx;
	if(mutex == NULL) {
		return EINVAL;
	}
	if(attr && *attr) {
		if((*attr)->pshared == PTHREAD_PROCESS_SHARED) {
			// Creating mutex that can be shared between processes.
#if _POSIX_THREAD_PROCESS_SHARED >= 0
			// Not implemented yet.
#error ERROR [__FILE__, line __LINE__]: Process shared mutexes are not supported yet.
#else
			return ENOSYS;
#endif
		}
	}
	mx = static_cast<pthread_mutex_t>(SAlloc::C(1, sizeof(*mx)));
	if(!mx) {
		result = ENOMEM;
	}
	else {
		mx->lock_idx = 0;
		mx->recursive_count = 0;
		mx->robustNode = NULL;
		if(!attr || !*attr) {
			mx->kind = PTHREAD_MUTEX_DEFAULT;
		}
		else {
			mx->kind = (*attr)->kind;
			if((*attr)->robustness == PTHREAD_MUTEX_ROBUST) {
				// 
				// Use the negative range to represent robust types.
				// Replaces a memory fetch with a register negate and incr in pthread_mutex_lock etc.
				// 
				// Map 0,1,..,n to -1,-2,..,(-n)-1
				// 
				mx->kind = -mx->kind - 1;
				mx->robustNode = (__ptw32_robust_node_t*)SAlloc::M(sizeof(__ptw32_robust_node_t));
				if(!mx->robustNode) {
					result = ENOMEM;
				}
				else {
					mx->robustNode->stateInconsistent =  __PTW32_ROBUST_CONSISTENT;
					mx->robustNode->mx = mx;
					mx->robustNode->next = NULL;
					mx->robustNode->prev = NULL;
				}
			}
		}
		if(result == 0) {
			mx->ownerThread.p = NULL;
			mx->event = CreateEvent(NULL,  FALSE/* manual reset = No */, FALSE/* initial state = not signalled */, NULL/* event name */);
			if(0 == mx->event) {
				result = ENOSPC;
			}
		}
	}
	if(result) {
		SAlloc::F(mx->robustNode);
		ZFREE(mx);
	}
	*mutex = mx;
	return result;
}

int pthread_mutex_destroy(pthread_mutex_t * mutex)
{
	int result = 0;
	pthread_mutex_t mx;
	// 
	// Let the system deal with invalid pointers.
	// 
	// Check to see if we have something to delete.
	//
	if(*mutex < PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		mx = *mutex;
		result = pthread_mutex_trylock(&mx);
		//
		// If trylock succeeded and the mutex is not recursively locked it can be destroyed.
		//
		if(!result || ENOTRECOVERABLE == result) {
			if(mx->kind != PTHREAD_MUTEX_RECURSIVE || 1 == mx->recursive_count) {
				// 
				// FIXME!!!
				// The mutex isn't held by another thread but we could still
				// be too late invalidating the mutex below since another thread
				// may already have entered mutex_lock and the check for a valid *mutex != NULL.
				// 
				*mutex = NULL;
				result = (0 == result) ? pthread_mutex_unlock(&mx) : 0;
				if(result == 0) {
					SAlloc::F(mx->robustNode);
					if(!CloseHandle(mx->event)) {
						*mutex = mx;
						result = EINVAL;
					}
					else
						SAlloc::F(mx);
				}
				else
					*mutex = mx; // Restore the mutex before we return the error.
			}
			else { // mx->recursive_count > 1 
				// The mutex must be recursive and already locked by us (this thread).
				mx->recursive_count--; /* Undo effect of pthread_mutex_trylock() above */
				result = EBUSY;
			}
		}
	}
	else {
		__ptw32_mcs_local_node_t node;
		/*
		 * See notes in __ptw32_mutex_check_need_init() above also.
		 */
		__ptw32_mcs_lock_acquire(&__ptw32_mutex_test_init_lock, &node);
		/*
		 * Check again.
		 */
		if(*mutex >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
			/*
			 * This is all we need to do to destroy a statically
			 * initialised mutex that has not yet been used (initialised).
			 * If we get to here, another thread
			 * waiting to initialise this mutex will get an EINVAL.
			 */
			*mutex = NULL;
		}
		else {
			result = EBUSY; // The mutex has been initialised while we were waiting so assume it's in use.
		}
		__ptw32_mcs_lock_release(&node);
	}
	return result;
}
/*
 * From the Sun Multi-threaded Programming Guide
 *
 * robustness defines the behavior when the owner of the mutex terminates without unlocking the
 * mutex, usually because its process terminated abnormally. The value of robustness that is
 * defined in pthread.h is PTHREAD_MUTEX_ROBUST or PTHREAD_MUTEX_STALLED. The
 * default value is PTHREAD_MUTEX_STALLED .
 * [] PTHREAD_MUTEX_STALLED
 * When the owner of the mutex terminates without unlocking the mutex, all subsequent calls
 * to pthread_mutex_lock() are blocked from progress in an unspecified manner.
 * [] PTHREAD_MUTEX_ROBUST
 * When the owner of the mutex terminates without unlocking the mutex, the mutex is
 * unlocked. The next owner of this mutex acquires the mutex with an error return of
 * EOWNERDEAD.
 * Note - Your application must always check the return code from pthread_mutex_lock() for
 * a mutex initialized with the PTHREAD_MUTEX_ROBUST attribute.
 * [] The new owner of this mutex should make the state protected by the mutex consistent.
 * This state might have been left inconsistent when the previous owner terminated.
 * [] If the new owner is able to make the state consistent, call
 * pthread_mutex_consistent() for the mutex before unlocking the mutex. This
 * marks the mutex as consistent and subsequent calls to pthread_mutex_lock() and
 * pthread_mutex_unlock() will behave in the normal manner.
 * [] If the new owner is not able to make the state consistent, do not call
 * pthread_mutex_consistent() for the mutex, but unlock the mutex.
 * All waiters are woken up and all subsequent calls to pthread_mutex_lock() fail to
 * acquire the mutex. The return code is ENOTRECOVERABLE. The mutex can be made
 * consistent by calling pthread_mutex_destroy() to uninitialize the mutex, and calling
 * pthread_mutex_int() to reinitialize the mutex.However, the state that was protected
 * by the mutex remains inconsistent and some form of application recovery is required.
 * [] If the thread that acquires the lock with EOWNERDEAD terminates without unlocking the
 * mutex, the next owner acquires the lock with an EOWNERDEAD return code.
 */
INLINE int __ptw32_robust_mutex_inherit(pthread_mutex_t * mutex)
{
	int result;
	pthread_mutex_t mx = *mutex;
	__ptw32_robust_node_t* robust = mx->robustNode;
	switch((LONG)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&robust->stateInconsistent,
		(__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_INCONSISTENT, (__PTW32_INTERLOCKED_LONG)-1 /* The terminating thread sets this */)) {
		case -1L: result = EOWNERDEAD; break;
		case (LONG)__PTW32_ROBUST_NOTRECOVERABLE: result = ENOTRECOVERABLE; break;
		default: result = 0; break;
	}
	return result;
}
/*
 * The next two internal support functions depend on being
 * called only by the thread that owns the robust mutex. This
 * enables us to avoid additional locks.
 * Any mutex currently in the thread's robust mutex list is held
 * by the thread, again eliminating the need for locks.
 * The forward/backward links allow the thread to unlock mutexes
 * in any order, not necessarily the reverse locking order.
 * This is all possible because it is an error if a thread that
 * does not own the [robust] mutex attempts to unlock it.
 */
INLINE void __ptw32_robust_mutex_add(pthread_mutex_t* mutex, pthread_t self)
{
	pthread_mutex_t mx = *mutex;
	__ptw32_thread_t* tp = (__ptw32_thread_t *)self.p;
	__ptw32_robust_node_t* robust = mx->robustNode;
	__ptw32_robust_node_t ** list = &tp->robustMxList;
	mx->ownerThread = self;
	if(NULL == *list) {
		robust->prev = NULL;
		robust->next = NULL;
		*list = robust;
	}
	else {
		robust->prev = NULL;
		robust->next = *list;
		(*list)->prev = robust;
		*list = robust;
	}
}

INLINE void __ptw32_robust_mutex_remove(pthread_mutex_t* mutex, __ptw32_thread_t* otp)
{
	pthread_mutex_t mx = *mutex;
	__ptw32_robust_node_t* robust = mx->robustNode;
	__ptw32_robust_node_t ** list = &(((__ptw32_thread_t *)mx->ownerThread.p)->robustMxList);
	mx->ownerThread.p = otp;
	if(robust->next) {
		robust->next->prev = robust->prev;
	}
	if(robust->prev) {
		robust->prev->next = robust->next;
	}
	if(*list == robust) {
		*list = robust->next;
	}
}

int pthread_mutex_consistent(pthread_mutex_t* mutex)
{
	pthread_mutex_t mx = *mutex;
	int result = 0;
	// 
	// Let the system deal with invalid pointers.
	// 
	if(!mx) {
		return EINVAL;
	}
	if(mx->kind >= 0 || (__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_INCONSISTENT !=  __PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG(
		    (__PTW32_INTERLOCKED_LONGPTR)&mx->robustNode->stateInconsistent,
		    (__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_CONSISTENT,
		    (__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_INCONSISTENT)) {
		result = EINVAL;
	}
	return result;
}

int pthread_mutex_lock(pthread_mutex_t * mutex)
{
	// 
	// Let the system deal with invalid pointers.
	// 
	pthread_mutex_t mx = *mutex;
	int kind;
	int result = 0;
	if(mx == NULL) {
		return EINVAL;
	}
	// 
	// We do a quick check to see if we need to do more work
	// to initialise a static mutex. We check
	// again inside the guarded section of __ptw32_mutex_check_need_init()
	// to avoid race conditions.
	// 
	if(mx >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		result = __ptw32_mutex_check_need_init(mutex);
		if(result != 0)
			return result;
		mx = *mutex;
	}
	kind = mx->kind;
	if(kind >= 0) {
		// Non-robust 
		if(PTHREAD_MUTEX_NORMAL == kind) {
			if(__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1) != 0) {
				while(__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
					if(WAIT_OBJECT_0 != WaitForSingleObject(mx->event, INFINITE)) {
						result = EINVAL;
						break;
					}
				}
			}
		}
		else {
			pthread_t self = pthread_self();
			if(__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0) == 0) {
				mx->recursive_count = 1;
				mx->ownerThread = self;
			}
			else {
				if(pthread_equal(mx->ownerThread, self)) {
					if(kind == PTHREAD_MUTEX_RECURSIVE) {
						mx->recursive_count++;
					}
					else {
						result = EDEADLK;
					}
				}
				else {
					while(__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
						if(WAIT_OBJECT_0 != WaitForSingleObject(mx->event, INFINITE)) {
							result = EINVAL;
							break;
						}
					}
					if(result == 0) {
						mx->recursive_count = 1;
						mx->ownerThread = self;
					}
				}
			}
		}
	}
	else {
		// 
		// Robust types
		// All types record the current owner thread.
		// The mutex is added to a per thread list when ownership is acquired.
		// 
		__ptw32_robust_state_t* statePtr = &mx->robustNode->stateInconsistent;
		if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE ==  __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
			result = ENOTRECOVERABLE;
		}
		else {
			pthread_t self = pthread_self();
			kind = -kind - 1; /* Convert to non-robust range */
			if(PTHREAD_MUTEX_NORMAL == kind) {
				if(__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1) != 0) {
					while((result = __ptw32_robust_mutex_inherit(mutex)) == 0 && __PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
						if(WAIT_OBJECT_0 != WaitForSingleObject(mx->event, INFINITE)) {
							result = EINVAL;
							break;
						}
						if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE == __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
							/* Unblock the next thread */
							SetEvent(mx->event);
							result = ENOTRECOVERABLE;
							break;
						}
					}
				}
				if(result == 0 || result == EOWNERDEAD) {
					/*
					 * Add mutex to the per-thread robust mutex currently-held list.
					 * If the thread terminates, all mutexes in this list will be unlocked.
					 */
					__ptw32_robust_mutex_add(mutex, self);
				}
			}
			else {
				if(__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0) == 0) {
					mx->recursive_count = 1;
					/*
					 * Add mutex to the per-thread robust mutex currently-held list.
					 * If the thread terminates, all mutexes in this list will be unlocked.
					 */
					__ptw32_robust_mutex_add(mutex, self);
				}
				else {
					if(pthread_equal(mx->ownerThread, self)) {
						if(PTHREAD_MUTEX_RECURSIVE == kind) {
							mx->recursive_count++;
						}
						else {
							result = EDEADLK;
						}
					}
					else {
						while((result = __ptw32_robust_mutex_inherit(mutex)) == 0 && __PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
							if(WAIT_OBJECT_0 != WaitForSingleObject(mx->event, INFINITE)) {
								result = EINVAL;
								break;
							}
							if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE == __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
								// Unblock the next thread 
								SetEvent(mx->event);
								result = ENOTRECOVERABLE;
								break;
							}
						}
						if(result == 0 || result == EOWNERDEAD) {
							mx->recursive_count = 1;
							// 
							// Add mutex to the per-thread robust mutex currently-held list.
							// If the thread terminates, all mutexes in this list will be unlocked.
							// 
							__ptw32_robust_mutex_add(mutex, self);
						}
					}
				}
			}
		}
	}
	return result;
}

int pthread_mutex_trylock(pthread_mutex_t * mutex)
{
	// 
	// Let the system deal with invalid pointers.
	// 
	pthread_mutex_t mx = *mutex;
	int kind;
	int result = 0;
	if(!mx) {
		return EINVAL;
	}
	// 
	// We do a quick check to see if we need to do more work
	// to initialise a static mutex. We check
	// again inside the guarded section of __ptw32_mutex_check_need_init()
	// to avoid race conditions.
	// 
	if(mx >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		result = __ptw32_mutex_check_need_init(mutex);
		if(result)
			return result;
		mx = *mutex;
	}
	kind = mx->kind;
	if(kind >= 0) {
		/* Non-robust */
		if(0 == __PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0)) {
			if(kind != PTHREAD_MUTEX_NORMAL) {
				mx->recursive_count = 1;
				mx->ownerThread = pthread_self();
			}
		}
		else {
			if(kind == PTHREAD_MUTEX_RECURSIVE && pthread_equal(mx->ownerThread, pthread_self()))
				mx->recursive_count++;
			else
				result = EBUSY;
		}
	}
	else {
		/*
		 * Robust types
		 * All types record the current owner thread.
		 * The mutex is added to a per thread list when ownership is acquired.
		 */
		pthread_t self;
		__ptw32_robust_state_t* statePtr = &mx->robustNode->stateInconsistent;
		if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE == __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
			return ENOTRECOVERABLE;
		}
		self = pthread_self();
		kind = -kind - 1; /* Convert to non-robust range */
		if(0 ==  __PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0)) {
			if(kind != PTHREAD_MUTEX_NORMAL)
				mx->recursive_count = 1;
			__ptw32_robust_mutex_add(mutex, self);
		}
		else {
			if(PTHREAD_MUTEX_RECURSIVE == kind && pthread_equal(mx->ownerThread, pthread_self()))
				mx->recursive_count++;
			else {
				if(EOWNERDEAD == (result = __ptw32_robust_mutex_inherit(mutex))) {
					mx->recursive_count = 1;
					__ptw32_robust_mutex_add(mutex, self);
				}
				else if(result == 0)
					result = EBUSY;
			}
		}
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DESCRIPTION
 *   This function waits on an event until signaled or until
 *   abstime passes.
 *   If abstime has passed when this routine is called then
 *   it returns a result to indicate this.
 *
 *   If 'abstime' is a NULL pointer then this function will
 *   block until it can successfully decrease the value or
 *   until interrupted by a signal.
 *
 *   This routine is not a cancellation point.
 *
 * RESULTS
 *     0               successfully signaled,
 *     ETIMEDOUT       abstime passed
 *     EINVAL          'event' is not a valid event,
 *
 * ------------------------------------------------------
 */
static INLINE int __ptw32_timed_eventwait(HANDLE event, const struct timespec * abstime)
{
	DWORD milliseconds;
	DWORD status;
	if(!event)
		return EINVAL;
	else {
		if(abstime == NULL)
			milliseconds = INFINITE;
		else
			milliseconds = __ptw32_relmillisecs(abstime); // Calculate timeout as milliseconds from current system time.
		status = WaitForSingleObject(event, milliseconds);
		if(status != WAIT_OBJECT_0)
			if(status == WAIT_TIMEOUT)
				return ETIMEDOUT;
			else
				return EINVAL;
	}
	return 0;
}

int pthread_mutex_timedlock(pthread_mutex_t * mutex, const struct timespec * abstime)
{
	// 
	// Let the system deal with invalid pointers.
	// 
	pthread_mutex_t mx = *mutex;
	int kind;
	int result = 0;
	if(mx == NULL) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static mutex. We check
	 * again inside the guarded section of __ptw32_mutex_check_need_init()
	 * to avoid race conditions.
	 */
	if(mx >= PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) {
		if((result = __ptw32_mutex_check_need_init(mutex)) != 0) {
			return result;
		}
		mx = *mutex;
	}
	kind = mx->kind;
	if(kind >= 0) {
		if(mx->kind == PTHREAD_MUTEX_NORMAL) {
			if((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
				    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx,
				    (__PTW32_INTERLOCKED_LONG)1) != 0) {
				while((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
					    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx,
					    (__PTW32_INTERLOCKED_LONG)-1) != 0) {
					if(0 != (result = __ptw32_timed_eventwait(mx->event, abstime))) {
						return result;
					}
				}
			}
		}
		else {
			pthread_t self = pthread_self();
			if((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, 
				(__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0) == 0) {
				mx->recursive_count = 1;
				mx->ownerThread = self;
			}
			else {
				if(pthread_equal(mx->ownerThread, self)) {
					if(mx->kind == PTHREAD_MUTEX_RECURSIVE) {
						mx->recursive_count++;
					}
					else
						return EDEADLK;
				}
				else {
					while((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
						    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx,
						    (__PTW32_INTERLOCKED_LONG)-1) != 0) {
						if(0 != (result = __ptw32_timed_eventwait(mx->event, abstime))) {
							return result;
						}
					}
					mx->recursive_count = 1;
					mx->ownerThread = self;
				}
			}
		}
	}
	else {
		/*
		 * Robust types
		 * All types record the current owner thread.
		 * The mutex is added to a per thread list when ownership is acquired.
		 */
		__ptw32_robust_state_t* statePtr = &mx->robustNode->stateInconsistent;
		if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE ==  __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG(
			    (__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
			result = ENOTRECOVERABLE;
		}
		else {
			pthread_t self = pthread_self();
			kind = -kind - 1; /* Convert to non-robust range */
			if(PTHREAD_MUTEX_NORMAL == kind) {
				if((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
					    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx,
					    (__PTW32_INTERLOCKED_LONG)1) != 0) {
					while(0 == (result = __ptw32_robust_mutex_inherit(mutex)) &&  (__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
						    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
						if(0 != (result = __ptw32_timed_eventwait(mx->event, abstime))) {
							return result;
						}
						if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE == __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG(
							    (__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
							/* Unblock the next thread */
							SetEvent(mx->event);
							result = ENOTRECOVERABLE;
							break;
						}
					}
					if(0 == result || EOWNERDEAD == result) {
						/*
						 * Add mutex to the per-thread robust mutex currently-held list.
						 * If the thread terminates, all mutexes in this list will be unlocked.
						 */
						__ptw32_robust_mutex_add(mutex, self);
					}
				}
			}
			else {
				pthread_t self = pthread_self();
				if(0 ==  (__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG(
					    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)1, (__PTW32_INTERLOCKED_LONG)0)) {
					mx->recursive_count = 1;
					/*
					 * Add mutex to the per-thread robust mutex currently-held list.
					 * If the thread terminates, all mutexes in this list will be unlocked.
					 */
					__ptw32_robust_mutex_add(mutex, self);
				}
				else {
					if(pthread_equal(mx->ownerThread, self)) {
						if(PTHREAD_MUTEX_RECURSIVE == kind) {
							mx->recursive_count++;
						}
						else {
							return EDEADLK;
						}
					}
					else {
						while(0 == (result = __ptw32_robust_mutex_inherit(mutex)) &&  (__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG(
							    (__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)-1) != 0) {
							if(0 != (result = __ptw32_timed_eventwait(mx->event, abstime))) {
								return result;
							}
						}
						if((__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE ==
						    __PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((__PTW32_INTERLOCKED_LONGPTR)statePtr, (__PTW32_INTERLOCKED_LONG)0)) {
							/* Unblock the next thread */
							SetEvent(mx->event);
							result = ENOTRECOVERABLE;
						}
						else if(0 == result || EOWNERDEAD == result) {
							mx->recursive_count = 1;
							/*
							 * Add mutex to the per-thread robust mutex currently-held list.
							 * If the thread terminates, all mutexes in this list will be unlocked.
							 */
							__ptw32_robust_mutex_add(mutex, self);
						}
					}
				}
			}
		}
	}
	return result;
}

int pthread_mutex_unlock(pthread_mutex_t * mutex)
{
	// 
	// Let the system deal with invalid pointers.
	// 
	pthread_mutex_t mx = *mutex;
	int kind;
	int result = 0;
	// 
	// If the thread calling us holds the mutex then there is no
	// race condition. If another thread holds the lock then we shouldn't be in here.
	// 
	if(mx < PTHREAD_ERRORCHECK_MUTEX_INITIALIZER) { // Remember, pointers are unsigned.
		kind = mx->kind;
		if(kind >= 0) {
			if(kind == PTHREAD_MUTEX_NORMAL) {
				LONG idx = (LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)0);
				if(idx != 0) {
					if(idx < 0) {
						if(SetEvent(mx->event) == 0) { // Someone may be waiting on that mutex.
							result = EINVAL;
						}
					}
				}
			}
			else {
				if(pthread_equal(mx->ownerThread, pthread_self())) {
					if(kind != PTHREAD_MUTEX_RECURSIVE || 0 == --mx->recursive_count) {
						mx->ownerThread.p = NULL;
						if((LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)0) < 0L) {
							if(SetEvent(mx->event) == 0) { // Someone may be waiting on that mutex.
								result = EINVAL;
							}
						}
					}
				}
				else
					result = EPERM;
			}
		}
		else {
			/* Robust types */
			pthread_t self = pthread_self();
			kind = -kind - 1; /* Convert to non-robust range */
			// 
			// The thread must own the lock regardless of type if the mutex is robust.
			// 
			if(pthread_equal(mx->ownerThread, self)) {
				__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->robustNode->stateInconsistent,
				    (__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_NOTRECOVERABLE,
				    (__PTW32_INTERLOCKED_LONG)__PTW32_ROBUST_INCONSISTENT);
				if(PTHREAD_MUTEX_NORMAL == kind) {
					__ptw32_robust_mutex_remove(mutex, NULL);
					if((LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)0) < 0) {
						if(SetEvent(mx->event) == 0) { // Someone may be waiting on that mutex.
							result = EINVAL;
						}
					}
				}
				else {
					if(kind != PTHREAD_MUTEX_RECURSIVE || 0 == --mx->recursive_count) {
						__ptw32_robust_mutex_remove(mutex, NULL);
						if((LONG)__PTW32_INTERLOCKED_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&mx->lock_idx, (__PTW32_INTERLOCKED_LONG)0) < 0) {
							if(SetEvent(mx->event) == 0) { // Someone may be waiting on that mutex.
								result = EINVAL;
							}
						}
					}
				}
			}
			else
				result = EPERM;
		}
	}
	else if(mx != PTHREAD_MUTEX_INITIALIZER) {
		// 
		// If mx is PTHREAD_ERRORCHECK_MUTEX_INITIALIZER or PTHREAD_RECURSIVE_MUTEX_INITIALIZER
		// we need to know we are doing something unexpected. For PTHREAD_MUTEX_INITIALIZER
		// (normal) mutexes we can just silently ignore it.
		// 
		result = EINVAL;
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a mutex attributes object with default
 *   attributes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *
 *
 * DESCRIPTION
 *   Initializes a mutex attributes object with default
 *   attributes.
 *
 *   NOTES:
 *     1)      Used to define mutex types
 *
 * RESULTS
 *     0               successfully initialized attr,
 *     ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_init(pthread_mutexattr_t * attr)
{
	int result = 0;
	pthread_mutexattr_t ma = static_cast<pthread_mutexattr_t>(SAlloc::C(1, sizeof(*ma)));
	if(ma == NULL)
		result = ENOMEM;
	else {
		ma->pshared = PTHREAD_PROCESS_PRIVATE;
		ma->kind = PTHREAD_MUTEX_DEFAULT;
		ma->robustness = PTHREAD_MUTEX_STALLED;
	}
	*attr = ma;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Destroys a mutex attributes object. The object can
 *   no longer be used.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *
 *
 * DESCRIPTION
 *   Destroys a mutex attributes object. The object can
 *   no longer be used.
 *
 *   NOTES:
 *     1)      Does not affect mutexes created using 'attr'
 *
 * RESULTS
 *     0               successfully released attr,
 *     EINVAL          'attr' is invalid.
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_destroy(pthread_mutexattr_t * attr)
{
	int result = 0;
	if(!attr || !*attr) {
		result = EINVAL;
	}
	else {
		pthread_mutexattr_t ma = *attr;
		*attr = NULL;
		SAlloc::F(ma);
	}
	return result;
}

int pthread_mutexattr_getkind_np(pthread_mutexattr_t * attr, int * kind)
{
	return pthread_mutexattr_gettype(attr, kind);
}

int pthread_mutexattr_setkind_np(pthread_mutexattr_t * attr, int kind)
{
	return pthread_mutexattr_settype(attr, kind);
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Determine whether mutexes created with 'attr' can be
 *   shared between processes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *   pshared
 *     will be set to one of:
 *             PTHREAD_PROCESS_SHARED
 *                     May be shared if in shared memory
 *             PTHREAD_PROCESS_PRIVATE
 *                     Cannot be shared.
 *
 * DESCRIPTION
 *   Mutexes creatd with 'attr' can be shared between
 *   processes if pthread_mutex_t variable is allocated
 *   in memory shared by these processes.
 *   NOTES:
 *     1)      pshared mutexes MUST be allocated in shared
 *             memory.
 *     2)      The following macro is defined if shared mutexes
 *             are supported:
 *                     _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *     0               successfully retrieved attribute,
 *     EINVAL          'attr' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_getpshared(const pthread_mutexattr_t * attr, int * pshared)
{
	int result = 0;
	if((attr && *attr) && pshared)
		*pshared = (*attr)->pshared;
	else
		result = EINVAL;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Mutexes created with 'attr' can be shared between
 *   processes if pthread_mutex_t variable is allocated
 *   in memory shared by these processes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *   pshared
 *     must be one of:
 *             PTHREAD_PROCESS_SHARED
 *                     May be shared if in shared memory
 *             PTHREAD_PROCESS_PRIVATE
 *                     Cannot be shared.
 *
 * DESCRIPTION
 *   Mutexes creatd with 'attr' can be shared between
 *   processes if pthread_mutex_t variable is allocated
 *   in memory shared by these processes.
 *
 *   NOTES:
 *     1)      pshared mutexes MUST be allocated in shared memory.
 *     2)      The following macro is defined if shared mutexes are supported: _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *     0               successfully set attribute,
 *     EINVAL          'attr' or pshared is invalid,
 *     ENOSYS          PTHREAD_PROCESS_SHARED not supported,
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_setpshared(pthread_mutexattr_t * attr, int pshared)
{
	int result;
	if((attr && *attr) && ((pshared == PTHREAD_PROCESS_SHARED) || (pshared == PTHREAD_PROCESS_PRIVATE))) {
		if(pshared == PTHREAD_PROCESS_SHARED) {
#if !defined( _POSIX_THREAD_PROCESS_SHARED )
			result = ENOSYS;
			pshared = PTHREAD_PROCESS_PRIVATE;
#else
			result = 0;
#endif /* _POSIX_THREAD_PROCESS_SHARED */
		}
		else
			result = 0;
		(*attr)->pshared = pshared;
	}
	else
		result = EINVAL;
	return result;
}
/*
 * ------------------------------------------------------
 *
 * DOCPUBLIC
 * The pthread_mutexattr_setrobust() and
 * pthread_mutexattr_getrobust() functions  respectively set and
 * get the mutex robust  attribute. This attribute is set in  the
 * robust parameter to these functions.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *  robust
 *     must be one of: PTHREAD_MUTEX_STALLED PTHREAD_MUTEX_ROBUST
 *
 * DESCRIPTION
 * The pthread_mutexattr_setrobust() and
 * pthread_mutexattr_getrobust() functions  respectively set and
 * get the mutex robust  attribute. This attribute is set in  the
 * robust  parameter to these functions. The default value of the
 * robust  attribute is  PTHREAD_MUTEX_STALLED.
 *
 * The robustness of mutex is contained in the robustness attribute
 * of the mutex attributes. Valid mutex robustness values are:
 *
 * PTHREAD_MUTEX_STALLED
 * No special actions are taken if the owner of the mutex is
 * terminated while holding the mutex lock. This can lead to
 * deadlocks if no other thread can unlock the mutex.
 * This is the default value.
 *
 * PTHREAD_MUTEX_ROBUST
 * If the process containing the owning thread of a robust mutex
 * terminates while holding the mutex lock, the next thread that
 * acquires the mutex shall be notified about the termination by
 * the return value [EOWNERDEAD] from the locking function. If the
 * owning thread of a robust mutex terminates while holding the mutex
 * lock, the next thread that acquires the mutex may be notified
 * about the termination by the return value [EOWNERDEAD]. The
 * notified thread can then attempt to mark the state protected by
 * the mutex as consistent again by a call to
 * pthread_mutex_consistent(). After a subsequent successful call to
 * pthread_mutex_unlock(), the mutex lock shall be released and can
 * be used normally by other threads. If the mutex is unlocked without
 * a call to pthread_mutex_consistent(), it shall be in a permanently
 * unusable state and all attempts to lock the mutex shall fail with
 * the error [ENOTRECOVERABLE]. The only permissible operation on such
 * a mutex is pthread_mutex_destroy().
 *
 * RESULTS
 *     0               successfully set attribute,
 *     EINVAL          'attr' or 'robust' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_getrobust(const pthread_mutexattr_t * attr, int * robust)
{
	int result = 0;
	if(attr && *attr && robust)
		*robust = (*attr)->robustness;
	else
		result = EINVAL;
	return result;
}
/*
 * ------------------------------------------------------
 *
 * DOCPUBLIC
 * The pthread_mutexattr_setrobust() and
 * pthread_mutexattr_getrobust() functions  respectively set and
 * get the mutex robust  attribute. This attribute is set in  the
 * robust parameter to these functions.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *
 *  robust
 *     must be one of: PTHREAD_MUTEX_STALLED PTHREAD_MUTEX_ROBUST
 *
 * DESCRIPTION
 * The pthread_mutexattr_setrobust() and
 * pthread_mutexattr_getrobust() functions  respectively set and
 * get the mutex robust  attribute. This attribute is set in  the
 * robust  parameter to these functions. The default value of the
 * robust  attribute is  PTHREAD_MUTEX_STALLED.
 *
 * The robustness of mutex is contained in the robustness attribute
 * of the mutex attributes. Valid mutex robustness values are:
 *
 * PTHREAD_MUTEX_STALLED
 * No special actions are taken if the owner of the mutex is
 * terminated while holding the mutex lock. This can lead to
 * deadlocks if no other thread can unlock the mutex.
 * This is the default value.
 *
 * PTHREAD_MUTEX_ROBUST
 * If the process containing the owning thread of a robust mutex
 * terminates while holding the mutex lock, the next thread that
 * acquires the mutex shall be notified about the termination by
 * the return value [EOWNERDEAD] from the locking function. If the
 * owning thread of a robust mutex terminates while holding the mutex
 * lock, the next thread that acquires the mutex may be notified
 * about the termination by the return value [EOWNERDEAD]. The
 * notified thread can then attempt to mark the state protected by
 * the mutex as consistent again by a call to
 * pthread_mutex_consistent(). After a subsequent successful call to
 * pthread_mutex_unlock(), the mutex lock shall be released and can
 * be used normally by other threads. If the mutex is unlocked without
 * a call to pthread_mutex_consistent(), it shall be in a permanently
 * unusable state and all attempts to lock the mutex shall fail with
 * the error [ENOTRECOVERABLE]. The only permissible operation on such
 * a mutex is pthread_mutex_destroy().
 *
 * RESULTS
 *     0               successfully set attribute,
 *     EINVAL          'attr' or 'robust' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_setrobust(pthread_mutexattr_t * attr, int robust)
{
	int result = EINVAL;
	if(attr && *attr) {
		switch(robust) {
			case PTHREAD_MUTEX_STALLED:
			case PTHREAD_MUTEX_ROBUST:
			    (*attr)->robustness = robust;
			    result = 0;
			    break;
		}
	}
	return result;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t * attr, int * kind)
{
	int result = 0;
	if(attr && *attr && kind)
		*kind = (*attr)->kind;
	else
		result = EINVAL;
	return result;
}
/*
 * ------------------------------------------------------
 *
 * DOCPUBLIC
 * The pthread_mutexattr_settype() and
 * pthread_mutexattr_gettype() functions  respectively set and
 * get the mutex type  attribute. This attribute is set in  the
 * type parameter to these functions.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_mutexattr_t
 *   type
 *     must be one of: PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL PTHREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_RECURSIVE
 *
 * DESCRIPTION
 * The pthread_mutexattr_settype() and
 * pthread_mutexattr_gettype() functions  respectively set and
 * get the mutex type  attribute. This attribute is set in  the
 * type  parameter to these functions. The default value of the
 * type  attribute is  PTHREAD_MUTEX_DEFAULT.
 *
 * The type of mutex is contained in the type  attribute of the
 * mutex attributes. Valid mutex types include:
 *
 * PTHREAD_MUTEX_NORMAL
 * This type of mutex does  not  detect  deadlock.  A
 * thread  attempting  to  relock  this mutex without
 * first unlocking it will  deadlock.  Attempting  to
 * unlock  a  mutex  locked  by  a  different  thread
 * results  in  undefined  behavior.  Attempting   to
 * unlock  an  unlocked  mutex  results  in undefined
 * behavior.
 *
 * PTHREAD_MUTEX_ERRORCHECK
 * This type of  mutex  provides  error  checking.  A
 * thread  attempting  to  relock  this mutex without
 * first unlocking it will return with  an  error.  A
 * thread  attempting to unlock a mutex which another
 * thread has locked will return  with  an  error.  A
 * thread attempting to unlock an unlocked mutex will
 * return with an error.
 *
 * PTHREAD_MUTEX_DEFAULT
 * Same as PTHREAD_MUTEX_NORMAL.
 *
 * PTHREAD_MUTEX_RECURSIVE
 * A thread attempting to relock this  mutex  without
 * first  unlocking  it  will  succeed in locking the
 * mutex. The relocking deadlock which can occur with
 * mutexes of type  PTHREAD_MUTEX_NORMAL cannot occur
 * with this type of mutex. Multiple  locks  of  this
 * mutex  require  the  same  number  of  unlocks  to
 * release  the  mutex  before  another  thread   can
 * acquire the mutex. A thread attempting to unlock a
 * mutex which another thread has locked will  return
 * with  an  error. A thread attempting to  unlock an
 * unlocked mutex will return  with  an  error.  This
 * type  of mutex is only supported for mutexes whose
 * process        shared         attribute         is
 * PTHREAD_PROCESS_PRIVATE.
 *
 * RESULTS
 *     0               successfully set attribute,
 *     EINVAL          'attr' or 'type' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_settype(pthread_mutexattr_t * attr, int kind)
{
	int result = 0;
	if(attr && *attr) {
		switch(kind) {
			case PTHREAD_MUTEX_FAST_NP:
			case PTHREAD_MUTEX_RECURSIVE_NP:
			case PTHREAD_MUTEX_ERRORCHECK_NP:
			    (*attr)->kind = kind;
			    break;
			default:
			    result = EINVAL;
			    break;
		}
	}
	else
		result = EINVAL;
	return result;
}
