// pthread_cond.c
//
/*
 * Description: This translation unit implements condition variables and their primitives.
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
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
/*
 * DOCPUBLIC
 *   This function initializes a condition variable.
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *   attr
 *     specifies optional creation attributes.
 *
 * DESCRIPTION
 *   This function initializes a condition variable.
 *
 * RESULTS
 *     0               successfully created condition variable,
 *     EINVAL          'attr' is invalid,
 *     EAGAIN          insufficient resources (other than memory,
 *     ENOMEM          insufficient memory,
 *     EBUSY           'cond' is already initialized,
 *
 * ------------------------------------------------------
 */
int pthread_cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr)
{
	int result;
	pthread_cond_t cv = NULL;
	if(cond == NULL) {
		return EINVAL;
	}
	if((attr && *attr) && ((*attr)->pshared == PTHREAD_PROCESS_SHARED)) {
		// Creating condition variable that can be shared between processes.
		result = ENOSYS;
		goto DONE;
	}
	cv = static_cast<pthread_cond_t>(SAlloc::C(1, sizeof(*cv)));
	if(cv == NULL) {
		result = ENOMEM;
		goto DONE;
	}
	cv->nWaitersBlocked = 0;
	cv->nWaitersToUnblock = 0;
	cv->nWaitersGone = 0;
	if(sem_init(&(cv->semBlockLock), 0, 1) != 0) {
		result =  __PTW32_GET_ERRNO();
		goto FAIL0;
	}
	if(sem_init(&(cv->semBlockQueue), 0, 0) != 0) {
		result =  __PTW32_GET_ERRNO();
		goto FAIL1;
	}
	if((result = pthread_mutex_init(&(cv->mtxUnblockLock), 0)) != 0) {
		goto FAIL2;
	}
	result = 0;
	goto DONE;
	/*
	 * -------------
	 * Failed...
	 * -------------
	 */
FAIL2:
	sem_destroy(&(cv->semBlockQueue));
FAIL1:
	sem_destroy(&(cv->semBlockLock));
FAIL0:
	SAlloc::F(cv);
	cv = NULL;
DONE:
	if(result == 0) {
		__ptw32_mcs_local_node_t node;
		__ptw32_mcs_lock_acquire(&__ptw32_cond_list_lock, &node);
		cv->next = NULL;
		cv->prev = __ptw32_cond_list_tail;
		if(__ptw32_cond_list_tail != NULL) {
			__ptw32_cond_list_tail->next = cv;
		}
		__ptw32_cond_list_tail = cv;
		if(__ptw32_cond_list_head == NULL) {
			__ptw32_cond_list_head = cv;
		}
		__ptw32_mcs_lock_release(&node);
	}
	*cond = cv;
	return result;
}
/*
 * DOCPUBLIC
 *   This function destroys a condition variable
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *
 * DESCRIPTION
 *   This function destroys a condition variable.
 *
 *   NOTES:
 *     1)      A condition variable can be destroyed
 *             immediately after all the threads that
 *             are blocked on it are awakened. e.g.
 *
 *             struct list {
 *               pthread_mutex_t lm;
 *               ...
 *             }
 *
 *             struct elt {
 *               key k;
 *               int busy;
 *               pthread_cond_t notbusy;
 *               ...
 *             }
 *
 *
 *             struct elt *
 *             list_find(struct list *lp, key k)
 *             {
 *               struct elt *ep;
 *
 *               pthread_mutex_lock(&lp->lm);
 *               while((ep = find_elt(l,k) != NULL) && ep->busy)
 *                 pthread_cond_wait(&ep->notbusy, &lp->lm);
 *               if(ep != NULL)
 *                 ep->busy = 1;
 *               pthread_mutex_unlock(&lp->lm);
 *               return(ep);
 *             }
 *
 *             delete_elt(struct list *lp, struct elt *ep)
 *             {
 *               pthread_mutex_lock(&lp->lm);
 *               assert(ep->busy);
 *               ... remove ep from list ...
 *               ep->busy = 0;
 *           (A) pthread_cond_broadcast(&ep->notbusy);
 *               pthread_mutex_unlock(&lp->lm);
 *           (B) pthread_cond_destroy(&rp->notbusy);
 *               free(ep);
 *             }
 *
 *             In this example, the condition variable
 *             and its list element may be freed (line B)
 *             immediately after all threads waiting for
 *             it are awakened (line A), since the mutex
 *             and the code ensure that no other thread
 *             can touch the element to be deleted.
 *
 * RESULTS
 *     0               successfully released condition variable,
 *     EINVAL          'cond' is invalid,
 *     EBUSY           'cond' is in use,
 */
int pthread_cond_destroy(pthread_cond_t * cond)
{
	pthread_cond_t cv;
	int result = 0, result1 = 0, result2 = 0;
	/*
	 * Assuming any race condition here is harmless.
	 */
	if(cond == NULL || *cond == NULL) {
		return EINVAL;
	}
	if(*cond != PTHREAD_COND_INITIALIZER) {
		__ptw32_mcs_local_node_t node;
		__ptw32_mcs_lock_acquire(&__ptw32_cond_list_lock, &node);
		cv = *cond;
		/*
		 * Close the gate; this will synchronize this thread with
		 * all already signaled waiters to let them retract their
		 * waiter status - SEE NOTE 1 ABOVE!!!
		 */
		if(__ptw32_semwait(&(cv->semBlockLock)) != 0) { /* Non-cancelable */
			result =  __PTW32_GET_ERRNO();
		}
		else {
			/*
			 * !TRY! lock mtxUnblockLock; try will detect busy condition
			 * and will not cause a deadlock with respect to concurrent
			 * signal/broadcast.
			 */
			if((result = pthread_mutex_trylock(&(cv->mtxUnblockLock))) != 0) {
				sem_post(&(cv->semBlockLock));
			}
		}
		if(result != 0) {
			__ptw32_mcs_lock_release(&node);
			return result;
		}
		/*
		 * Check whether cv is still busy (still has waiters)
		 */
		if(cv->nWaitersBlocked > cv->nWaitersGone) {
			if(sem_post(&(cv->semBlockLock)) != 0) {
				result =  __PTW32_GET_ERRNO();
			}
			result1 = pthread_mutex_unlock(&(cv->mtxUnblockLock));
			result2 = EBUSY;
		}
		else {
			/*
			 * Now it is safe to destroy
			 */
			*cond = NULL;
			if(sem_destroy(&(cv->semBlockLock)) != 0) {
				result =  __PTW32_GET_ERRNO();
			}
			if(sem_destroy(&(cv->semBlockQueue)) != 0) {
				result1 =  __PTW32_GET_ERRNO();
			}
			if((result2 = pthread_mutex_unlock(&(cv->mtxUnblockLock))) == 0) {
				result2 = pthread_mutex_destroy(&(cv->mtxUnblockLock));
			}
			/* Unlink the CV from the list */
			if(__ptw32_cond_list_head == cv)
				__ptw32_cond_list_head = cv->next;
			else
				cv->prev->next = cv->next;
			if(__ptw32_cond_list_tail == cv)
				__ptw32_cond_list_tail = cv->prev;
			else
				cv->next->prev = cv->prev;
			SAlloc::F(cv);
		}
		__ptw32_mcs_lock_release(&node);
	}
	else {
		__ptw32_mcs_local_node_t node;
		/*
		 * See notes in __ptw32_cond_check_need_init() above also.
		 */
		__ptw32_mcs_lock_acquire(&__ptw32_cond_test_init_lock, &node);
		/*
		 * Check again.
		 */
		if(*cond == PTHREAD_COND_INITIALIZER) {
			/*
			 * This is all we need to do to destroy a statically
			 * initialised cond that has not yet been used (initialised).
			 * If we get to here, another thread waiting to initialise
			 * this cond will get an EINVAL. That's OK.
			 */
			*cond = NULL;
		}
		else
			result = EBUSY; // The cv has been initialised while we were waiting so assume it's in use.
		__ptw32_mcs_lock_release(&node);
	}
	return ((result != 0) ? result : ((result1 != 0) ? result1 : result2));
}
/* -------------------------------------------------------------
 * Algorithm:
 * The algorithm used in this implementation is that developed by
 * Alexander Terekhov in colaboration with Louis Thomas. The bulk
 * of the discussion is recorded in the file README.CV, which contains
 * several generations of both colaborators original algorithms. The final
 * algorithm used here is the one referred to as
 *
 *  Algorithm 8a / IMPL_SEM,UNBLOCK_STRATEGY == UNBLOCK_ALL
 *
 * presented below in pseudo-code as it appeared:
 *
 *
 * given:
 * semBlockLock - bin.semaphore
 * semBlockQueue - semaphore
 * mtxExternal - mutex or CS
 * mtxUnblockLock - mutex or CS
 * nWaitersGone - int
 * nWaitersBlocked - int
 * nWaitersToUnblock - int
 *
 * wait( timeout ) {
 *
 *   [auto: register int result          ]     // error checking omitted
 *   [auto: register int nSignalsWasLeft ]
 *   [auto: register int nWaitersWasGone ]
 *
 *   sem_wait( semBlockLock );
 *   nWaitersBlocked++;
 *   sem_post( semBlockLock );
 *
 *   unlock( mtxExternal );
 *   bTimedOut = sem_wait( semBlockQueue,timeout );
 *
 *   lock( mtxUnblockLock );
 *   if( 0 != (nSignalsWasLeft = nWaitersToUnblock)) {
 *  if( bTimeout ) {                       // timeout (or canceled)
 *    if( 0 != nWaitersBlocked ) {
 *      nWaitersBlocked--;
 *    }
 *    else {
 *      nWaitersGone++;                     // count spurious wakeups.
 *    }
 *  }
 *  if( 0 == --nWaitersToUnblock ) {
 *    if( 0 != nWaitersBlocked ) {
 *      sem_post( semBlockLock );           // open the gate.
 *      nSignalsWasLeft = 0;                // do not open the gate
 *                                    // below again.
 *    }
 *    else if( 0 != (nWaitersWasGone = nWaitersGone)) {
 *      nWaitersGone = 0;
 *    }
 *  }
 *   }
 *   else if( INT_MAX/2 == ++nWaitersGone ) { // timeout/canceled or
 *                                    // spurious semaphore :-)
 *  sem_wait( semBlockLock );
 *  nWaitersBlocked -= nWaitersGone;     // something is going on here
 *                                 //  - test of timeouts? :-)
 *  sem_post( semBlockLock );
 *  nWaitersGone = 0;
 *   }
 *   unlock( mtxUnblockLock );
 *
 *   if( 1 == nSignalsWasLeft ) {
 *  if( 0 != nWaitersWasGone ) {
 *    // sem_adjust( semBlockQueue,-nWaitersWasGone );
 *    while(nWaitersWasGone-- ) {
 *      sem_wait( semBlockQueue );       // better now than spurious later
 *    }
 *  } sem_post( semBlockLock );          // open the gate
 *   }
 *
 *   lock( mtxExternal );
 *
 *   return ( bTimedOut ) ? ETIMEOUT : 0;
 * }
 *
 * signal(bAll) {
 *
 *   [auto: register int result         ]
 *   [auto: register int nSignalsToIssue]
 *
 *   lock( mtxUnblockLock );
 *
 *   if( 0 != nWaitersToUnblock ) {        // the gate is closed!!!
 *  if( 0 == nWaitersBlocked ) {        // NO-OP
 *    return unlock( mtxUnblockLock );
 *  }
 *  if(bAll) {
 *    nWaitersToUnblock += nSignalsToIssue=nWaitersBlocked;
 *    nWaitersBlocked = 0;
 *  }
 *  else {
 *    nSignalsToIssue = 1;
 *    nWaitersToUnblock++;
 *    nWaitersBlocked--;
 *  }
 *   }
 *   else if( nWaitersBlocked > nWaitersGone ) { // HARMLESS RACE CONDITION!
 *  sem_wait( semBlockLock );                  // close the gate
 *  if( 0 != nWaitersGone ) {
 *    nWaitersBlocked -= nWaitersGone;
 *    nWaitersGone = 0;
 *  }
 *  if(bAll) {
 *    nSignalsToIssue = nWaitersToUnblock = nWaitersBlocked;
 *    nWaitersBlocked = 0;
 *  }
 *  else {
 *    nSignalsToIssue = nWaitersToUnblock = 1;
 *    nWaitersBlocked--;
 *  }
 *   }
 *   else { // NO-OP
 *  return unlock( mtxUnblockLock );
 *   }
 *
 *   unlock( mtxUnblockLock );
 *   sem_post( semBlockQueue,nSignalsToIssue );
 *   return result;
 * }
 * -------------------------------------------------------------
 *
 *  Algorithm 9 / IMPL_SEM,UNBLOCK_STRATEGY == UNBLOCK_ALL
 *
 * presented below in pseudo-code; basically 8a...
 *                             ...BUT W/O "spurious wakes" prevention:
 *
 *
 * given:
 * semBlockLock - bin.semaphore
 * semBlockQueue - semaphore
 * mtxExternal - mutex or CS
 * mtxUnblockLock - mutex or CS
 * nWaitersGone - int
 * nWaitersBlocked - int
 * nWaitersToUnblock - int
 *
 * wait( timeout ) {
 *
 *   [auto: register int result          ]     // error checking omitted
 *   [auto: register int nSignalsWasLeft ]
 *
 *   sem_wait( semBlockLock );
 *   ++nWaitersBlocked;
 *   sem_post( semBlockLock );
 *
 *   unlock( mtxExternal );
 *   bTimedOut = sem_wait( semBlockQueue,timeout );
 *
 *   lock( mtxUnblockLock );
 *   if( 0 != (nSignalsWasLeft = nWaitersToUnblock)) {
 *  --nWaitersToUnblock;
 *   }
 *   else if( INT_MAX/2 == ++nWaitersGone ) { // timeout/canceled or
 *                                    // spurious semaphore :-)
 *  sem_wait( semBlockLock );
 *  nWaitersBlocked -= nWaitersGone;        // something is going on here
 *                                    //  - test of timeouts? :-)
 *  sem_post( semBlockLock );
 *  nWaitersGone = 0;
 *   }
 *   unlock( mtxUnblockLock );
 *
 *   if( 1 == nSignalsWasLeft ) {
 *  sem_post( semBlockLock );               // open the gate
 *   }
 *
 *   lock( mtxExternal );
 *
 *   return ( bTimedOut ) ? ETIMEOUT : 0;
 * }
 *
 * signal(bAll) {
 *
 *   [auto: register int result         ]
 *   [auto: register int nSignalsToIssue]
 *
 *   lock( mtxUnblockLock );
 *
 *   if( 0 != nWaitersToUnblock ) {        // the gate is closed!!!
 *  if( 0 == nWaitersBlocked ) {        // NO-OP
 *    return unlock( mtxUnblockLock );
 *  }
 *  if(bAll) {
 *    nWaitersToUnblock += nSignalsToIssue=nWaitersBlocked;
 *    nWaitersBlocked = 0;
 *  }
 *  else {
 *    nSignalsToIssue = 1;
 *    ++nWaitersToUnblock;
 *    --nWaitersBlocked;
 *  }
 *   }
 *   else if( nWaitersBlocked > nWaitersGone ) { // HARMLESS RACE CONDITION!
 *  sem_wait( semBlockLock );                  // close the gate
 *  if( 0 != nWaitersGone ) {
 *    nWaitersBlocked -= nWaitersGone;
 *    nWaitersGone = 0;
 *  }
 *  if(bAll) {
 *    nSignalsToIssue = nWaitersToUnblock = nWaitersBlocked;
 *    nWaitersBlocked = 0;
 *  }
 *  else {
 *    nSignalsToIssue = nWaitersToUnblock = 1;
 *    --nWaitersBlocked;
 *  }
 *   }
 *   else { // NO-OP
 *  return unlock( mtxUnblockLock );
 *   }
 *
 *   unlock( mtxUnblockLock );
 *   sem_post( semBlockQueue,nSignalsToIssue );
 *   return result;
 * }
 * -------------------------------------------------------------
 */
// 
// Arguments for cond_wait_cleanup, since we can only pass a single void * to it.
// 
typedef struct {
	pthread_mutex_t * mutexPtr;
	pthread_cond_t cv;
	int * resultPtr;
} __ptw32_cond_wait_cleanup_args_t;

static void __PTW32_CDECL __ptw32_cond_wait_cleanup(void * args)
{
	__ptw32_cond_wait_cleanup_args_t * cleanup_args =
	    (__ptw32_cond_wait_cleanup_args_t*)args;
	pthread_cond_t cv = cleanup_args->cv;
	int * resultPtr = cleanup_args->resultPtr;
	int nSignalsWasLeft;
	int result;
	/*
	 * Whether we got here as a result of signal/broadcast or because of
	 * timeout on wait or thread cancellation we indicate that we are no
	 * longer waiting. The waiter is responsible for adjusting waiters
	 * (to)unblock(ed) counts (protected by unblock lock).
	 */
	if((result = pthread_mutex_lock(&(cv->mtxUnblockLock))) != 0) {
		*resultPtr = result;
		return;
	}

	if(0 != (nSignalsWasLeft = cv->nWaitersToUnblock)) {
		--(cv->nWaitersToUnblock);
	}
	else if(INT_MAX / 2 == ++(cv->nWaitersGone)) {
		/* Use the non-cancellable version of sem_wait() */
		if(__ptw32_semwait(&(cv->semBlockLock)) != 0) {
			*resultPtr =  __PTW32_GET_ERRNO();
			/*
			 * This is a fatal error for this CV,
			 * so we deliberately don't unlock
			 * cv->mtxUnblockLock before returning.
			 */
			return;
		}
		cv->nWaitersBlocked -= cv->nWaitersGone;
		if(sem_post(&(cv->semBlockLock)) != 0) {
			*resultPtr =  __PTW32_GET_ERRNO();
			/*
			 * This is a fatal error for this CV,
			 * so we deliberately don't unlock
			 * cv->mtxUnblockLock before returning.
			 */
			return;
		}
		cv->nWaitersGone = 0;
	}

	if((result = pthread_mutex_unlock(&(cv->mtxUnblockLock))) != 0) {
		*resultPtr = result;
		return;
	}
	if(1 == nSignalsWasLeft) {
		if(sem_post(&(cv->semBlockLock)) != 0) {
			*resultPtr =  __PTW32_GET_ERRNO();
			return;
		}
	}
	/*
	 * XSH: Upon successful return, the mutex has been locked and is owned
	 * by the calling thread.
	 */
	if((result = pthread_mutex_lock(cleanup_args->mutexPtr)) != 0) {
		*resultPtr = result;
	}
}

static INLINE int __ptw32_cond_timedwait(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime)
{
	int result = 0;
	pthread_cond_t cv;
	__ptw32_cond_wait_cleanup_args_t cleanup_args;
	if(cond == NULL || *cond == NULL) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static condition variable. We check
	 * again inside the guarded section of __ptw32_cond_check_need_init()
	 * to avoid race conditions.
	 */
	if(*cond == PTHREAD_COND_INITIALIZER) {
		result = __ptw32_cond_check_need_init(cond);
	}
	if(result != 0 && result != EBUSY) {
		return result;
	}
	cv = *cond;
	/* Thread can be cancelled in sem_wait() but this is OK */
	if(sem_wait(&(cv->semBlockLock)) != 0) {
		return __PTW32_GET_ERRNO();
	}
	++(cv->nWaitersBlocked);
	if(sem_post(&(cv->semBlockLock)) != 0) {
		return __PTW32_GET_ERRNO();
	}
	/*
	 * Setup this waiter cleanup handler
	 */
	cleanup_args.mutexPtr = mutex;
	cleanup_args.cv = cv;
	cleanup_args.resultPtr = &result;
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth(0)
#endif
	pthread_cleanup_push(__ptw32_cond_wait_cleanup, (void *)&cleanup_args);
	/*
	 * Now we can release 'mutex' and...
	 */
	if((result = pthread_mutex_unlock(mutex)) == 0) {
		/*
		 * ...wait to be awakened by
		 *     pthread_cond_signal, or
		 *     pthread_cond_broadcast, or
		 *     timeout, or
		 *     thread cancellation
		 *
		 * Note:
		 *
		 *   sem_timedwait is a cancellation point,
		 *   hence providing the mechanism for making
		 *   pthread_cond_wait a cancellation point.
		 *   We use the cleanup mechanism to ensure we
		 *   re-lock the mutex and adjust (to)unblock(ed) waiters
		 *   counts if we are cancelled, timed out or signalled.
		 */
		if(sem_timedwait(&(cv->semBlockQueue), abstime) != 0) {
			result =  __PTW32_GET_ERRNO();
		}
	}
	/*
	 * Always cleanup
	 */
	pthread_cleanup_pop(1);
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth()
#endif
	/*
	 * "result" can be modified by the cleanup handler.
	 */
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function waits on a condition variable until
 *   awakened by a signal or broadcast.
 *
 *   Caller MUST be holding the mutex lock; the
 *   lock is released and the caller is blocked waiting
 *   on 'cond'. When 'cond' is signaled, the mutex
 *   is re-acquired before returning to the caller.
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *   mutex
 *     pointer to an instance of pthread_mutex_t
 *
 * DESCRIPTION
 *   This function waits on a condition variable until
 *   awakened by a signal or broadcast.
 *
 *   NOTES:
 *   1)      The function must be called with 'mutex' LOCKED
 *     by the calling thread, or undefined behaviour will result.
 *   2)      This routine atomically releases 'mutex' and causes
 *     the calling thread to block on the condition variable.
 *     The blocked thread may be awakened by
 *             pthread_cond_signal or
 *             pthread_cond_broadcast.
 *
 * Upon successful completion, the 'mutex' has been locked and is owned by the calling thread.
 *
 * RESULTS
 *     0               caught condition; mutex released,
 *     EINVAL          'cond' or 'mutex' is invalid,
 *     EINVAL          different mutexes for concurrent waits,
 *     EINVAL          mutex is not held by the calling thread,
 *
 * ------------------------------------------------------
 */
int pthread_cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
	// The NULL abstime arg means INFINITE waiting.
	return (__ptw32_cond_timedwait(cond, mutex, NULL));
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function waits on a condition variable either until
 *   awakened by a signal or broadcast; or until the time
 *   specified by abstime passes.
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *   mutex
 *     pointer to an instance of pthread_mutex_t
 *   abstime
 *     pointer to an instance of (const struct timespec)
 *
 * DESCRIPTION
 *   This function waits on a condition variable either until
 *   awakened by a signal or broadcast; or until the time
 *   specified by abstime passes.
 *
 *   NOTES:
 *   1)      The function must be called with 'mutex' LOCKED
 *     by the calling thread, or undefined behaviour will result.
 *   2)      This routine atomically releases 'mutex' and causes
 *     the calling thread to block on the condition variable.
 *     The blocked thread may be awakened by
 *             pthread_cond_signal or
 *             pthread_cond_broadcast.
 *
 *
 * RESULTS
 *     0               caught condition; mutex released,
 *     EINVAL          'cond', 'mutex', or abstime is invalid,
 *     EINVAL          different mutexes for concurrent waits,
 *     EINVAL          mutex is not held by the calling thread,
 *     ETIMEDOUT       abstime ellapsed before cond was signaled.
 *
 * ------------------------------------------------------
 */
int pthread_cond_timedwait(pthread_cond_t * cond, pthread_mutex_t * mutex, const struct timespec * abstime)
{
	return abstime ? __ptw32_cond_timedwait(cond, mutex, abstime) : EINVAL;
}
/*
 * Notes.
 *
 * Does not use the external mutex for synchronisation,
 * therefore semBlockLock is needed.
 * mtxUnblockLock is for LEVEL-2 synch. LEVEL-2 is the
 * state where the external mutex is not necessarily locked by
 * any thread, ie. between cond_wait unlocking and re-acquiring
 * the lock after having been signaled or a timeout or
 * cancellation.
 *
 * Uses the following CV elements:
 *   nWaitersBlocked nWaitersToUnblock nWaitersGone mtxUnblockLock semBlockLock semBlockQueue
 */
static INLINE int __ptw32_cond_unblock(pthread_cond_t * cond, int unblockAll)
{
	int result;
	pthread_cond_t cv;
	int nSignalsToIssue;
	if(cond == NULL || *cond == NULL) {
		return EINVAL;
	}
	cv = *cond;
	/*
	 * No-op if the CV is static and hasn't been initialised yet.
	 * Assuming that any race condition is harmless.
	 */
	if(cv == PTHREAD_COND_INITIALIZER) {
		return 0;
	}
	if((result = pthread_mutex_lock(&(cv->mtxUnblockLock))) != 0) {
		return result;
	}
	if(0 != cv->nWaitersToUnblock) {
		if(0 == cv->nWaitersBlocked) {
			return pthread_mutex_unlock(&(cv->mtxUnblockLock));
		}
		if(unblockAll) {
			cv->nWaitersToUnblock += (nSignalsToIssue = cv->nWaitersBlocked);
			cv->nWaitersBlocked = 0;
		}
		else {
			nSignalsToIssue = 1;
			cv->nWaitersToUnblock++;
			cv->nWaitersBlocked--;
		}
	}
	else if(cv->nWaitersBlocked > cv->nWaitersGone) {
		/* Use the non-cancellable version of sem_wait() */
		if(__ptw32_semwait(&(cv->semBlockLock)) != 0) {
			result =  __PTW32_GET_ERRNO();
			pthread_mutex_unlock(&(cv->mtxUnblockLock));
			return result;
		}
		if(0 != cv->nWaitersGone) {
			cv->nWaitersBlocked -= cv->nWaitersGone;
			cv->nWaitersGone = 0;
		}
		if(unblockAll) {
			nSignalsToIssue = cv->nWaitersToUnblock = cv->nWaitersBlocked;
			cv->nWaitersBlocked = 0;
		}
		else {
			nSignalsToIssue = cv->nWaitersToUnblock = 1;
			cv->nWaitersBlocked--;
		}
	}
	else {
		return pthread_mutex_unlock(&(cv->mtxUnblockLock));
	}
	if((result = pthread_mutex_unlock(&(cv->mtxUnblockLock))) == 0) {
		if(sem_post_multiple(&(cv->semBlockQueue), nSignalsToIssue) != 0) {
			result =  __PTW32_GET_ERRNO();
		}
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function signals a condition variable, waking
 *   one waiting thread.
 *   If SCHED_FIFO or SCHED_RR policy threads are waiting
 *   the highest priority waiter is awakened; otherwise,
 *   an unspecified waiter is awakened.
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *
 *
 * DESCRIPTION
 *   This function signals a condition variable, waking
 *   one waiting thread.
 *   If SCHED_FIFO or SCHED_RR policy threads are waiting
 *   the highest priority waiter is awakened; otherwise,
 *   an unspecified waiter is awakened.
 *
 *   NOTES:
 *
 *   1)      Use when any waiter can respond and only one need
 *     respond (all waiters being equal).
 *
 * RESULTS
 *     0               successfully signaled condition,
 *     EINVAL          'cond' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_cond_signal(pthread_cond_t * cond)
{
	/*
	 * The '0'(FALSE) unblockAll arg means unblock ONE waiter.
	 */
	return (__ptw32_cond_unblock(cond, 0));
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function broadcasts the condition variable, waking all current waiters.
 *
 * PARAMETERS
 *   cond
 *     pointer to an instance of pthread_cond_t
 *
 * DESCRIPTION
 *   This function signals a condition variable, waking all waiting threads.
 *
 *   NOTES:
 *
 *   1)      Use when more than one waiter may respond to
 *     predicate change or if any waiting thread may
 *     not be able to respond
 *
 * RESULTS
 *     0               successfully signalled condition to all waiting threads,
 *     EINVAL          'cond' is invalid
 *     ENOSPC          a required resource has been exhausted,
 *
 * ------------------------------------------------------
 */
int pthread_cond_broadcast(pthread_cond_t * cond)
{
	// The TRUE unblockAll arg means unblock ALL waiters.
	return (__ptw32_cond_unblock(cond,  TRUE));
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a condition variable attributes object
 *   with default attributes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_condattr_t
 *
 *
 * DESCRIPTION
 *   Initializes a condition variable attributes object
 *   with default attributes.
 *
 *   NOTES:
 *     1)      Use to define condition variable types
 *     2)      It is up to the application to ensure
 *             that it doesn't re-init an attribute
 *             without destroying it first. Otherwise
 *             a memory leak is created.
 *
 * RESULTS
 *     0               successfully initialized attr,
 *     ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_condattr_init(pthread_condattr_t * attr)
{
	int result = 0;
	pthread_condattr_t attr_result = static_cast<pthread_condattr_t>(SAlloc::C(1, sizeof(*attr_result)));
	if(attr_result == NULL)
		result = ENOMEM;
	*attr = attr_result;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Destroys a condition variable attributes object.
 *   The object can no longer be used.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_condattr_t
 *
 *
 * DESCRIPTION
 *   Destroys a condition variable attributes object.
 *   The object can no longer be used.
 *
 *   NOTES:
 *   1)      Does not affect condition variables created
 *     using 'attr'
 *
 * RESULTS
 *     0               successfully released attr,
 *     EINVAL          'attr' is invalid.
 *
 * ------------------------------------------------------
 */
int pthread_condattr_destroy(pthread_condattr_t * attr)
{
	int result = 0;
	if(!attr || !*attr)
		result = EINVAL;
	else {
		SAlloc::F(*attr);
		*attr = NULL;
		result = 0;
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Determine whether condition variables created with 'attr'
 *   can be shared between processes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_condattr_t
 *   pshared
 *     will be set to one of:
 *             PTHREAD_PROCESS_SHARED
 *                     May be shared if in shared memory
 *             PTHREAD_PROCESS_PRIVATE
 *                     Cannot be shared.
 *
 *
 * DESCRIPTION
 *   Condition Variables created with 'attr' can be shared
 *   between processes if pthread_cond_t variable is allocated
 *   in memory shared by these processes.
 *   NOTES:
 *   1)      pshared condition variables MUST be allocated in shared memory.
 *   2)      The following macro is defined if shared mutexes
 *     are supported:
 *             _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *     0               successfully retrieved attribute,
 *     EINVAL          'attr' or 'pshared' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_condattr_getpshared(const pthread_condattr_t * attr, int * pshared)
{
	int result = 0;
	if(attr && *attr && pshared)
		*pshared = (*attr)->pshared;
	else
		result = EINVAL;
	return result;
}

