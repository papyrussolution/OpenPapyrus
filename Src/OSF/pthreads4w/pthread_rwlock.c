// pthread_rwlock.c 
//
/*
 * Description: This translation unit implements read/write lock primitives.
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

int pthread_rwlock_init(pthread_rwlock_t * rwlock, const pthread_rwlockattr_t * attr)
{
	int result;
	pthread_rwlock_t rwl = 0;
	if(rwlock == NULL) {
		return EINVAL;
	}
	if(attr && *attr) {
		result = EINVAL; /* Not supported */
		goto DONE;
	}
	rwl = (pthread_rwlock_t)SAlloc::C(1, sizeof(*rwl));
	if(rwl == NULL) {
		result = ENOMEM;
		goto DONE;
	}
	rwl->nSharedAccessCount = 0;
	rwl->nExclusiveAccessCount = 0;
	rwl->nCompletedSharedAccessCount = 0;
	result = pthread_mutex_init(&rwl->mtxExclusiveAccess, NULL);
	if(result != 0) {
		goto FAIL0;
	}
	result = pthread_mutex_init(&rwl->mtxSharedAccessCompleted, NULL);
	if(result != 0) {
		goto FAIL1;
	}
	result = pthread_cond_init(&rwl->cndSharedAccessCompleted, NULL);
	if(result != 0) {
		goto FAIL2;
	}
	rwl->nMagic =  __PTW32_RWLOCK_MAGIC;
	result = 0;
	goto DONE;
FAIL2:
	(void)pthread_mutex_destroy(&(rwl->mtxSharedAccessCompleted));
FAIL1:
	(void)pthread_mutex_destroy(&(rwl->mtxExclusiveAccess));
FAIL0:
	SAlloc::F(rwl);
	rwl = NULL;
DONE:
	*rwlock = rwl;
	return result;
}

int pthread_rwlock_destroy(pthread_rwlock_t * rwlock)
{
	pthread_rwlock_t rwl;
	int result = 0, result1 = 0, result2 = 0;
	if(!rwlock || !*rwlock)
		return EINVAL;
	if(*rwlock != PTHREAD_RWLOCK_INITIALIZER) {
		rwl = *rwlock;
		if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
			return EINVAL;
		}
		if((result = pthread_mutex_lock(&(rwl->mtxExclusiveAccess))) != 0) {
			return result;
		}
		if((result = pthread_mutex_lock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
		/*
		 * Check whether any threads own/wait for the lock (wait for ex.access);
		 * report "BUSY" if so.
		 */
		if(rwl->nExclusiveAccessCount > 0 || rwl->nSharedAccessCount > rwl->nCompletedSharedAccessCount) {
			result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted));
			result1 = pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			result2 = EBUSY;
		}
		else {
			rwl->nMagic = 0;
			if((result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted))) != 0) {
				pthread_mutex_unlock(&rwl->mtxExclusiveAccess);
				return result;
			}
			if((result = pthread_mutex_unlock(&(rwl->mtxExclusiveAccess))) != 0) {
				return result;
			}
			*rwlock = NULL; /* Invalidate rwlock before anything else */
			result = pthread_cond_destroy(&(rwl->cndSharedAccessCompleted));
			result1 = pthread_mutex_destroy(&(rwl->mtxSharedAccessCompleted));
			result2 = pthread_mutex_destroy(&(rwl->mtxExclusiveAccess));
			SAlloc::F(rwl);
		}
	}
	else {
		__ptw32_mcs_local_node_t node;
		/*
		 * See notes in __ptw32_rwlock_check_need_init() above also.
		 */
		__ptw32_mcs_lock_acquire(&__ptw32_rwlock_test_init_lock, &node);
		/*
		 * Check again.
		 */
		if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
			/*
			 * This is all we need to do to destroy a statically
			 * initialised rwlock that has not yet been used (initialised).
			 * If we get to here, another thread
			 * waiting to initialise this rwlock will get an EINVAL.
			 */
			*rwlock = NULL;
		}
		else {
			/*
			 * The rwlock has been initialised while we were waiting
			 * so assume it's in use.
			 */
			result = EBUSY;
		}
		__ptw32_mcs_lock_release(&node);
	}
	return ((result != 0) ? result : ((result1 != 0) ? result1 : result2));
}

int pthread_rwlock_rdlock(pthread_rwlock_t * rwlock)
{
	int result;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_lock(&(rwl->mtxExclusiveAccess))) != 0) {
		return result;
	}
	if(++rwl->nSharedAccessCount == INT_MAX) {
		if((result = pthread_mutex_lock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
		rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
		rwl->nCompletedSharedAccessCount = 0;
		if((result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
	}
	return (pthread_mutex_unlock(&(rwl->mtxExclusiveAccess)));
}

int pthread_rwlock_wrlock(pthread_rwlock_t * rwlock)
{
	int result;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_lock(&(rwl->mtxExclusiveAccess))) != 0) {
		return result;
	}
	if((result = pthread_mutex_lock(&(rwl->mtxSharedAccessCompleted))) != 0) {
		(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
		return result;
	}
	if(rwl->nExclusiveAccessCount == 0) {
		if(rwl->nCompletedSharedAccessCount > 0) {
			rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
			rwl->nCompletedSharedAccessCount = 0;
		}
		if(rwl->nSharedAccessCount > 0) {
			rwl->nCompletedSharedAccessCount = -rwl->nSharedAccessCount;
			/*
			 * This routine may be a cancellation point
			 * according to POSIX 1003.1j section 18.1.2.
			 */
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth(0)
#endif
			pthread_cleanup_push(__ptw32_rwlock_cancelwrwait, (void*)rwl);
			do {
				result = pthread_cond_wait(&(rwl->cndSharedAccessCompleted), &(rwl->mtxSharedAccessCompleted));
			}
			while(result == 0 && rwl->nCompletedSharedAccessCount < 0);
			pthread_cleanup_pop((result != 0) ? 1 : 0);
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth()
#endif
			if(result == 0) {
				rwl->nSharedAccessCount = 0;
			}
		}
	}
	if(result == 0) {
		rwl->nExclusiveAccessCount++;
	}
	return result;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t * rwlock)
{
	int result;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_trylock(&(rwl->mtxExclusiveAccess))) != 0) {
		return result;
	}
	if(++rwl->nSharedAccessCount == INT_MAX) {
		if((result = pthread_mutex_lock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
		rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
		rwl->nCompletedSharedAccessCount = 0;
		if((result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
	}
	return (pthread_mutex_unlock(&rwl->mtxExclusiveAccess));
}

int pthread_rwlock_trywrlock(pthread_rwlock_t * rwlock)
{
	int result, result1;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_trylock(&(rwl->mtxExclusiveAccess))) != 0) {
		return result;
	}
	if((result = pthread_mutex_trylock(&(rwl->mtxSharedAccessCompleted))) != 0) {
		result1 = pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
		return ((result1 != 0) ? result1 : result);
	}
	if(rwl->nExclusiveAccessCount == 0) {
		if(rwl->nCompletedSharedAccessCount > 0) {
			rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
			rwl->nCompletedSharedAccessCount = 0;
		}
		if(rwl->nSharedAccessCount > 0) {
			if((result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted))) != 0) {
				(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
				return result;
			}
			if((result = pthread_mutex_unlock(&(rwl->mtxExclusiveAccess))) == 0) {
				result = EBUSY;
			}
		}
		else {
			rwl->nExclusiveAccessCount = 1;
		}
	}
	else {
		result = EBUSY;
	}
	return result;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t * rwlock, const struct timespec * abstime)
{
	int result;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_timedlock(&(rwl->mtxExclusiveAccess), abstime)) != 0) {
		return result;
	}
	if(++rwl->nSharedAccessCount == INT_MAX) {
		if((result = pthread_mutex_timedlock(&(rwl->mtxSharedAccessCompleted), abstime)) != 0) {
			if(result == ETIMEDOUT) {
				++rwl->nCompletedSharedAccessCount;
			}
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
		rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
		rwl->nCompletedSharedAccessCount = 0;
		if((result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
			return result;
		}
	}
	return (pthread_mutex_unlock(&(rwl->mtxExclusiveAccess)));
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t * rwlock, const struct timespec * abstime)
{
	int result;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return EINVAL;
	}
	/*
	 * We do a quick check to see if we need to do more work
	 * to initialise a static rwlock. We check
	 * again inside the guarded section of __ptw32_rwlock_check_need_init()
	 * to avoid race conditions.
	 */
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		result = __ptw32_rwlock_check_need_init(rwlock);
		if(result != 0 && result != EBUSY) {
			return result;
		}
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if((result = pthread_mutex_timedlock(&(rwl->mtxExclusiveAccess), abstime)) != 0) {
		return result;
	}
	if((result = pthread_mutex_timedlock(&(rwl->mtxSharedAccessCompleted), abstime)) != 0) {
		(void)pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
		return result;
	}
	if(rwl->nExclusiveAccessCount == 0) {
		if(rwl->nCompletedSharedAccessCount > 0) {
			rwl->nSharedAccessCount -= rwl->nCompletedSharedAccessCount;
			rwl->nCompletedSharedAccessCount = 0;
		}
		if(rwl->nSharedAccessCount > 0) {
			rwl->nCompletedSharedAccessCount = -rwl->nSharedAccessCount;
			/*
			 * This routine may be a cancellation point
			 * according to POSIX 1003.1j section 18.1.2.
			 */
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth(0)
#endif
			pthread_cleanup_push(__ptw32_rwlock_cancelwrwait, (void*)rwl);
			do {
				result = pthread_cond_timedwait(&(rwl->cndSharedAccessCompleted), &(rwl->mtxSharedAccessCompleted), abstime);
			} while(result == 0 && rwl->nCompletedSharedAccessCount < 0);
			pthread_cleanup_pop((result != 0) ? 1 : 0);
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth()
#endif
			if(result == 0) {
				rwl->nSharedAccessCount = 0;
			}
		}
	}
	if(result == 0) {
		rwl->nExclusiveAccessCount++;
	}
	return result;
}

int pthread_rwlock_unlock(pthread_rwlock_t * rwlock)
{
	int result, result1;
	pthread_rwlock_t rwl;
	if(!rwlock || !*rwlock) {
		return (EINVAL);
	}
	if(*rwlock == PTHREAD_RWLOCK_INITIALIZER) {
		// Assume any race condition here is harmless.
		return 0;
	}
	rwl = *rwlock;
	if(rwl->nMagic !=  __PTW32_RWLOCK_MAGIC) {
		return EINVAL;
	}
	if(rwl->nExclusiveAccessCount == 0) {
		if((result = pthread_mutex_lock(&(rwl->mtxSharedAccessCompleted))) != 0) {
			return result;
		}
		if(++rwl->nCompletedSharedAccessCount == 0) {
			result = pthread_cond_signal(&(rwl->cndSharedAccessCompleted));
		}
		result1 = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted));
	}
	else {
		rwl->nExclusiveAccessCount--;
		result = pthread_mutex_unlock(&(rwl->mtxSharedAccessCompleted));
		result1 = pthread_mutex_unlock(&(rwl->mtxExclusiveAccess));
	}
	return ((result != 0) ? result : result1);
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a rwlock attributes object with default attributes.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_rwlockattr_t
 *
 * DESCRIPTION
 *   Initializes a rwlock attributes object with default attributes.
 *
 * RESULTS
 *           0               successfully initialized attr,
 *           ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_rwlockattr_init(pthread_rwlockattr_t * attr)
{
	int result = 0;
	pthread_rwlockattr_t rwa = static_cast<pthread_rwlockattr_t>(SAlloc::C(1, sizeof(*rwa)));
	if(rwa == NULL)
		result = ENOMEM;
	else
		rwa->pshared = PTHREAD_PROCESS_PRIVATE;
	*attr = rwa;
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Destroys a rwlock attributes object. The object can
 *   no longer be used.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_rwlockattr_t
 *
 *
 * DESCRIPTION
 *   Destroys a rwlock attributes object. The object can
 *   no longer be used.
 *
 *   NOTES:
 *           1)      Does not affect rwlockss created using 'attr'
 *
 * RESULTS
 *           0               successfully released attr,
 *           EINVAL          'attr' is invalid.
 *
 * ------------------------------------------------------
 */
int pthread_rwlockattr_destroy(pthread_rwlockattr_t * attr)
{
	int result = 0;
	if(!attr || !*attr) {
		result = EINVAL;
	}
	else {
		pthread_rwlockattr_t rwa = *attr;
		*attr = NULL;
		SAlloc::F(rwa);
	}
	return result;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Determine whether rwlocks created with 'attr' can be shared between processes.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_rwlockattr_t
 *   pshared
 *           will be set to one of:
 *                   PTHREAD_PROCESS_SHARED - May be shared if in shared memory
 *                   PTHREAD_PROCESS_PRIVATE - Cannot be shared.
 *
 * DESCRIPTION
 *   Rwlocks creatd with 'attr' can be shared between
 *   processes if pthread_rwlock_t variable is allocated
 *   in memory shared by these processes.
 *   NOTES:
 *           1)      pshared rwlocks MUST be allocated in shared
 *                   memory.
 *           2)      The following macro is defined if shared rwlocks
 *                   are supported:
 *                           _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *           0               successfully retrieved attribute,
 *           EINVAL          'attr' is invalid,
 *
 * ------------------------------------------------------
 */
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t * attr, int * pshared)
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
 *   Rwlocks created with 'attr' can be shared between
 *   processes if pthread_rwlock_t variable is allocated
 *   in memory shared by these processes.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_rwlockattr_t
 *
 *   pshared
 *           must be one of:
 *
 *                   PTHREAD_PROCESS_SHARED
 *                           May be shared if in shared memory
 *
 *                   PTHREAD_PROCESS_PRIVATE
 *                           Cannot be shared.
 *
 * DESCRIPTION
 *   Rwlocks creatd with 'attr' can be shared between
 *   processes if pthread_rwlock_t variable is allocated
 *   in memory shared by these processes.
 *
 *   NOTES:
 *           1)      pshared rwlocks MUST be allocated in shared
 *                   memory.
 *
 *           2)      The following macro is defined if shared rwlocks
 *                   are supported:
 *                           _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *           0               successfully set attribute,
 *           EINVAL          'attr' or pshared is invalid,
 *           ENOSYS          PTHREAD_PROCESS_SHARED not supported,
 *
 * ------------------------------------------------------
 */
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t * attr, int pshared)
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
