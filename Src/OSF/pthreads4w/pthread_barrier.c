// pthread_barrier.c 
//
/*
 * Description:
 * This translation unit implements barrier primitives.
 *
 * --------------------------------------------------------------------------
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

int pthread_barrier_init(pthread_barrier_t * barrier, const pthread_barrierattr_t * attr, unsigned int count)
{
	if(!barrier || !count) {
		return EINVAL;
	}
	else {
		pthread_barrier_t b = (pthread_barrier_t)SAlloc::C(1, sizeof(*b));
		if(b) {
			b->pshared = (attr != NULL && *attr != NULL ? (*attr)->pshared : PTHREAD_PROCESS_PRIVATE);
			b->nCurrentBarrierHeight = b->nInitialBarrierHeight = count;
			b->lock = 0;
			const int semr = sem_init(&(b->semBarrierBreeched), b->pshared, 0);
			if(semr == 0) {
				*barrier = b;
				return 0;
			}
			else {
				SAlloc::F(b);
				return semr;
			}
		}
		else
			return ENOMEM;
	}
}

int pthread_barrier_destroy(pthread_barrier_t * barrier)
{
	int result = 0;
	pthread_barrier_t b;
	__ptw32_mcs_local_node_t node;
	if(barrier == NULL || *barrier == (pthread_barrier_t)__PTW32_OBJECT_INVALID) {
		return EINVAL;
	}
	if(0 != __ptw32_mcs_lock_try_acquire(&(*barrier)->lock, &node)) {
		return EBUSY;
	}
	b = *barrier;
	if(b->nCurrentBarrierHeight < b->nInitialBarrierHeight) {
		result = EBUSY;
	}
	else {
		if(0 == (result = sem_destroy(&(b->semBarrierBreeched)))) {
			*barrier = (pthread_barrier_t)__PTW32_OBJECT_INVALID;
			/*
			 * Release the lock before freeing b.
			 *
			 * FIXME: There may be successors which, when we release the lock,
			 * will be linked into b->lock, which will be corrupted at some
			 * point with undefined results for the application. To fix this
			 * will require changing pthread_barrier_t from a pointer to
			 * pthread_barrier_t_ to an instance. This is a change to the ABI
			 * and will require a major version number increment.
			 */
			__ptw32_mcs_lock_release(&node);
			SAlloc::F(b);
			return 0;
		}
		else {
			/*
			 * This should not ever be reached.
			 * Restore the barrier to working condition before returning.
			 */
			(void)sem_init(&(b->semBarrierBreeched), b->pshared, 0);
		}
		if(result != 0) {
			// The barrier still exists and is valid in the event of any error above.
			result = EBUSY;
		}
	}
	__ptw32_mcs_lock_release(&node);
	return result;
}

int pthread_barrier_wait(pthread_barrier_t * barrier)
{
	int result;
	pthread_barrier_t b;
	__ptw32_mcs_local_node_t node;
	if(barrier == NULL || *barrier == (pthread_barrier_t)__PTW32_OBJECT_INVALID) {
		return EINVAL;
	}
	__ptw32_mcs_lock_acquire(&(*barrier)->lock, &node);
	b = *barrier;
	if(--b->nCurrentBarrierHeight == 0) {
		/*
		 * We are the last thread to arrive at the barrier before it releases us.
		 * Move our MCS local node to the global scope barrier handle so that the
		 * last thread out (not necessarily us) can release the lock.
		 */
		__ptw32_mcs_node_transfer(&b->proxynode, &node);
		/*
		 * Any threads that have not quite entered sem_wait below when the
		 * multiple_post has completed will nevertheless continue through
		 * the semaphore (barrier).
		 */
		result = (b->nInitialBarrierHeight > 1 ? sem_post_multiple(&(b->semBarrierBreeched), b->nInitialBarrierHeight - 1) : 0);
	}
	else {
		__ptw32_mcs_lock_release(&node);
		/*
		 * Use the non-cancelable version of sem_wait().
		 *
		 * It is possible that all nInitialBarrierHeight-1 threads are
		 * at this point when the last thread enters the barrier, resets
		 * nCurrentBarrierHeight = nInitialBarrierHeight and leaves.
		 * If pthread_barrier_destroy is called at that moment then the
		 * barrier will be destroyed along with the semas.
		 */
		result = __ptw32_semwait(&(b->semBarrierBreeched));
	}
	if((__PTW32_INTERLOCKED_LONG)__PTW32_INTERLOCKED_INCREMENT_LONG((__PTW32_INTERLOCKED_LONGPTR)&b->nCurrentBarrierHeight) == (__PTW32_INTERLOCKED_LONG)b->nInitialBarrierHeight) {
		/*
		 * We are the last thread to cross this barrier
		 */
		__ptw32_mcs_lock_release(&b->proxynode);
		if(result == 0) {
			result = PTHREAD_BARRIER_SERIAL_THREAD;
		}
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a barrier attributes object with default attributes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_barrierattr_t
 *
 * DESCRIPTION
 *   Initializes a barrier attributes object with default attributes.
 *
 *   NOTES:
 *     1)      Used to define barrier types
 *
 * RESULTS
 *     0               successfully initialized attr,
 *     ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_barrierattr_init(pthread_barrierattr_t * attr)
{
	int result = 0;
	pthread_barrierattr_t ba = static_cast<pthread_barrierattr_t>(SAlloc::C(1, sizeof(*ba)));
	if(!ba)
		result = ENOMEM;
	else
		ba->pshared = PTHREAD_PROCESS_PRIVATE;
	*attr = ba;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Destroys a barrier attributes object. The object can no longer be used.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_barrierattr_t
 *
 * DESCRIPTION
 *   Destroys a barrier attributes object. The object can no longer be used.
 *
 *   NOTES:
 *     1)      Does not affect barrieres created using 'attr'
 *
 * RESULTS
 *     0               successfully released attr,
 *     EINVAL          'attr' is invalid.
 *
 * ------------------------------------------------------
 */
int pthread_barrierattr_destroy(pthread_barrierattr_t * attr)
{
	int result = 0;
	if(!attr || !*attr) {
		result = EINVAL;
	}
	else {
		pthread_barrierattr_t ba = *attr;
		*attr = NULL;
		SAlloc::F(ba);
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Determine whether barriers created with 'attr' can be
 *   shared between processes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_barrierattr_t
 *
 *   pshared
 *     will be set to one of:
 *
 *             PTHREAD_PROCESS_SHARED
 *                     May be shared if in shared memory
 *
 *             PTHREAD_PROCESS_PRIVATE
 *                     Cannot be shared.
 *
 *
 * DESCRIPTION
 *   Mutexes creatd with 'attr' can be shared between
 *   processes if pthread_barrier_t variable is allocated
 *   in memory shared by these processes.
 *   NOTES:
 *     1)      pshared barriers MUST be allocated in shared
 *             memory.
 *     2)      The following macro is defined if shared barriers
 *             are supported:
 *                     _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *     0               successfully retrieved attribute,
 *     EINVAL          'attr' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_barrierattr_getpshared(const pthread_barrierattr_t * attr, int * pshared)
{
	int result = 0;
	if(attr && *attr && pshared)
		*pshared = (*attr)->pshared;
	else
		result = EINVAL;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Barriers created with 'attr' can be shared between
 *   processes if pthread_barrier_t variable is allocated
 *   in memory shared by these processes.
 *
 * PARAMETERS
 *   attr
 *     pointer to an instance of pthread_barrierattr_t
 *
 *   pshared
 *     must be one of:
 *             PTHREAD_PROCESS_SHARED May be shared if in shared memory
 *             PTHREAD_PROCESS_PRIVATE Cannot be shared.
 *
 * DESCRIPTION
 *   Mutexes creatd with 'attr' can be shared between
 *   processes if pthread_barrier_t variable is allocated
 *   in memory shared by these processes.
 *
 *   NOTES:
 *     1)      pshared barriers MUST be allocated in shared memory.
 *     2)      The following macro is defined if shared barriers are supported: _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *     0               successfully set attribute,
 *     EINVAL          'attr' or pshared is invalid,
 *     ENOSYS          PTHREAD_PROCESS_SHARED not supported,
 *
 * ------------------------------------------------------
 */
int pthread_barrierattr_setpshared(pthread_barrierattr_t * attr, int pshared)
{
	int result;
	if((attr && *attr) && ((pshared == PTHREAD_PROCESS_SHARED) || (pshared == PTHREAD_PROCESS_PRIVATE))) {
		if(pshared == PTHREAD_PROCESS_SHARED) {
#if !defined( _POSIX_THREAD_PROCESS_SHARED )
			result = ENOSYS;
			pshared = PTHREAD_PROCESS_PRIVATE;
#else
			result = 0;
#endif
		}
		else
			result = 0;
		(*attr)->pshared = pshared;
	}
	else
		result = EINVAL;
	return result;
}
