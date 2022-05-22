// pthread_spin.c
//
/*
 * Description:
 * This translation unit implements spin lock primitives.
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

int pthread_spin_init(pthread_spinlock_t * lock, int pshared)
{
	pthread_spinlock_t s;
	int cpus = 0;
	int result = 0;
	if(lock == NULL) {
		return EINVAL;
	}
	if(0 != __ptw32_getprocessors(&cpus)) {
		cpus = 1;
	}
	if(cpus > 1) {
		if(pshared == PTHREAD_PROCESS_SHARED) {
			// Creating spinlock that can be shared between processes.
#if _POSIX_THREAD_PROCESS_SHARED >= 0
			// Not implemented yet.
	#error ERROR [__FILE__, line __LINE__]: Process shared spin locks are not supported yet.
#else
			return ENOSYS;
#endif
		}
	}
	s = static_cast<pthread_spinlock_t>(SAlloc::C(1, sizeof(*s)));
	if(!s) {
		return ENOMEM;
	}
	if(cpus > 1) {
		s->u.cpus = cpus;
		s->interlock =  __PTW32_SPIN_UNLOCKED;
	}
	else {
		pthread_mutexattr_t ma;
		result = pthread_mutexattr_init(&ma);
		if(result == 0) {
			ma->pshared = pshared;
			result = pthread_mutex_init(&(s->u.mutex), &ma);
			if(result == 0)
				s->interlock =  __PTW32_SPIN_USE_MUTEX;
		}
		(void)pthread_mutexattr_destroy(&ma);
	}
	if(result == 0) {
		*lock = s;
	}
	else {
		SAlloc::F(s);
		*lock = NULL;
	}
	return result;
}

int pthread_spin_destroy(pthread_spinlock_t * lock)
{
	pthread_spinlock_t s;
	int result = 0;
	if(lock == NULL || *lock == NULL) {
		return EINVAL;
	}
	if((s = *lock) != PTHREAD_SPINLOCK_INITIALIZER) {
		if(s->interlock ==  __PTW32_SPIN_USE_MUTEX) {
			result = pthread_mutex_destroy(&(s->u.mutex));
		}
		else if((__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_UNLOCKED != __PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&s->interlock,
		    (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_INVALID, (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_UNLOCKED)) {
			result = EINVAL;
		}
		if(result == 0) {
			/*
			 * We are relying on the application to ensure that all other threads
			 * have finished with the spinlock before destroying it.
			 */
			*lock = NULL;
			SAlloc::F(s);
		}
	}
	else {
		/*
		 * See notes in __ptw32_spinlock_check_need_init() above also.
		 */
		__ptw32_mcs_local_node_t node;
		__ptw32_mcs_lock_acquire(&__ptw32_spinlock_test_init_lock, &node);
		/*
		 * Check again.
		 */
		if(*lock == PTHREAD_SPINLOCK_INITIALIZER) {
			/*
			 * This is all we need to do to destroy a statically
			 * initialised spinlock that has not yet been used (initialised).
			 * If we get to here, another thread
			 * waiting to initialise this mutex will get an EINVAL.
			 */
			*lock = NULL;
		}
		else {
			// The spinlock has been initialised while we were waiting so assume it's in use.
			result = EBUSY;
		}
		__ptw32_mcs_lock_release(&node);
	}
	return result;
}

int pthread_spin_lock(pthread_spinlock_t * lock)
{
	pthread_spinlock_t s;
	if(!lock || !*lock) {
		return (EINVAL);
	}
	if(*lock == PTHREAD_SPINLOCK_INITIALIZER) {
		int result;
		if((result = __ptw32_spinlock_check_need_init(lock)) != 0) {
			return result;
		}
	}
	s = *lock;
	while((__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_LOCKED == __PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&s->interlock,
	    (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_LOCKED, (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_UNLOCKED)) {
	}
	if(s->interlock ==  __PTW32_SPIN_LOCKED) {
		return 0;
	}
	else if(s->interlock ==  __PTW32_SPIN_USE_MUTEX) {
		return pthread_mutex_lock(&(s->u.mutex));
	}
	return EINVAL;
}

int pthread_spin_trylock(pthread_spinlock_t * lock)
{
	if(lock && *lock) {
		if(*lock == PTHREAD_SPINLOCK_INITIALIZER) {
			int result = __ptw32_spinlock_check_need_init(lock);
			if(result != 0)
				return result;
		}
		pthread_spinlock_t s = *lock;
		switch((long)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&s->interlock,
			(__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_LOCKED, (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_UNLOCKED)) {
			case  __PTW32_SPIN_UNLOCKED: return 0;
			case  __PTW32_SPIN_LOCKED: return EBUSY;
			case  __PTW32_SPIN_USE_MUTEX: return pthread_mutex_trylock(&(s->u.mutex));
		}
	}
	return EINVAL;
}

int pthread_spin_unlock(pthread_spinlock_t * lock)
{
	pthread_spinlock_t s;
	if(lock && *lock) {
		s = *lock;
		if(s == PTHREAD_SPINLOCK_INITIALIZER) {
			return EPERM;
		}
		switch((long)__PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((__PTW32_INTERLOCKED_LONGPTR)&s->interlock,
			(__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_UNLOCKED, (__PTW32_INTERLOCKED_LONG)__PTW32_SPIN_LOCKED)) {
			case  __PTW32_SPIN_LOCKED:
			case  __PTW32_SPIN_UNLOCKED: return 0;
			case  __PTW32_SPIN_USE_MUTEX: return pthread_mutex_unlock(&(s->u.mutex));
		}
	}
	return EINVAL;
}
