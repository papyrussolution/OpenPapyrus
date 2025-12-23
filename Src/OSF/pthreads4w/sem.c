// SEM.C
//
/*
 * Purpose:
 *	Semaphores aren't actually part of PThreads.
 *	They are defined by the POSIX Standard:
 *		POSIX 1003.1-2001
 * -------------------------------------------------------------
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
#if defined(_MSC_VER)
	#pragma warning(disable:4100 ) // ignore warning "unreferenced formal parameter" 
#endif
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function initializes a semaphore. The initial value of the semaphore is 'value'
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *
 *   pshared
 *     if zero, this semaphore may only be shared between threads in the same process.
 *     if nonzero, the semaphore can be shared between processes
 *
 *   value
 *     initial value of the semaphore counter
 *
 * DESCRIPTION
 *   This function initializes a semaphore. The
 *   initial value of the semaphore is set to 'value'.
 *
 * RESULTS
 *     0               successfully created semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore, or
 *                     'value' >= SEM_VALUE_MAX
 *     ENOMEM          out of memory,
 *     ENOSPC          a required resource has been exhausted,
 *     ENOSYS          semaphores are not supported,
 *     EPERM           the process lacks appropriate privilege
 *
 * ------------------------------------------------------
 */
int sem_init(sem_t * sem, int pshared, unsigned int value)
{
	int result = 0;
	sem_t s = NULL;
	if(pshared != 0) {
		// Creating a semaphore that can be shared between processes
		result = EPERM;
	}
	else if(value > (uint)SEM_VALUE_MAX) {
		result = EINVAL;
	}
	else {
		s = static_cast<sem_t>(SAlloc::C(1, sizeof(*s)));
		if(!s) {
			result = ENOMEM;
		}
		else {
			s->value = value;
			s->lock = NULL;
#if defined(NEED_SEM)
			s->sem = CreateEvent(NULL, FALSE/* auto (not manual) reset */, FALSE/* initial state is unset */, NULL);
			if(0 == s->sem) {
				result = ENOSPC;
			}
			else {
				s->leftToUnblock = 0;
			}
#else /* NEED_SEM */
			if((s->sem = CreateSemaphore(NULL/* Always NULL */, 0L/* Force threads to wait */, (long)SEM_VALUE_MAX/* Maximum value */, NULL/* Name */)) == 0) {
				result = ENOSPC;
			}
#endif /* NEED_SEM */
			if(result != 0)
				SAlloc::F(s);
		}
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	*sem = s;
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function destroys an unnamed semaphore.
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *
 * DESCRIPTION
 *   This function destroys an unnamed semaphore.
 *
 * RESULTS
 *     0               successfully destroyed semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOSYS          semaphores are not supported,
 *     EBUSY           threads (or processes) are currently blocked on 'sem'
 *
 * ------------------------------------------------------
 */
int sem_destroy(sem_t * sem)
{
	int result = 0;
	sem_t s = NULL;
	if(!sem || !*sem) {
		result = EINVAL;
	}
	else {
		__ptw32_mcs_local_node_t node;
		s = *sem;
		if((result = __ptw32_mcs_lock_try_acquire(&s->lock, &node)) == 0) {
			if(s->value < 0) {
				result = EBUSY;
			}
			else {
				/*
				 * There are no threads currently blocked on this semaphore
				 * however there could be threads about to wait behind us.
				 * It is up to the application to ensure this is not the case.
				 */
				if(!CloseHandle(s->sem)) {
					result = EINVAL;
				}
			}
			__ptw32_mcs_lock_release(&node);
		}
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	SAlloc::F(s);
	return 0;
}

sem_t * sem_open(const char * name, int oflag, ...)
{
	/* Note: this is a POSIX.1b-1993 conforming stub; POSIX.1-2001 removed
	 * the requirement to provide this stub, and also removed the validity
	 * of ENOSYS as a resultant errno state; nevertheless, it makes sense
	 * to retain the POSIX.1b-1993 conforming behaviour here.
	 */
	__PTW32_SET_ERRNO(ENOSYS);
	return SEM_FAILED;
}

int sem_close(sem_t * sem)
{
	__PTW32_SET_ERRNO(ENOSYS);
	return -1;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function posts a wakeup to a semaphore.
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *
 * DESCRIPTION
 *   This function posts a wakeup to a semaphore. If there
 *   are waiting threads (or processes), one is awakened;
 *   otherwise, the semaphore value is incremented by one.
 *
 * RESULTS
 *     0               successfully posted semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOSYS          semaphores are not supported,
 *     ERANGE          semaphore count is too big
 *
 * ------------------------------------------------------
 */
int sem_post(sem_t * sem)
{
	int result = 0;
	__ptw32_mcs_local_node_t node;
	sem_t s = *sem;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	if(s->value < SEM_VALUE_MAX) {
#if defined(NEED_SEM)
		if(++s->value <= 0 && !SetEvent(s->sem)) {
			s->value--;
			result = EINVAL;
		}
#else
		if(++s->value <= 0 && !ReleaseSemaphore(s->sem, 1, NULL)) {
			s->value--;
			result = EINVAL;
		}
#endif
	}
	else
		result = ERANGE;
	__ptw32_mcs_lock_release(&node);
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function posts multiple wakeups to a semaphore.
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *   count
 *     counter, must be greater than zero.
 *
 * DESCRIPTION
 *   This function posts multiple wakeups to a semaphore. If there
 *   are waiting threads (or processes), n <= count are awakened;
 *   the semaphore value is incremented by count - n.
 *
 * RESULTS
 *     0               successfully posted semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore or count is less than or equal to zero.
 *     ERANGE          semaphore count is too big
 *
 * ------------------------------------------------------
 */
int sem_post_multiple(sem_t * sem, int count)
{
	__ptw32_mcs_local_node_t node;
	int result = 0;
	long waiters;
	sem_t s = *sem;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	if(s->value <= (SEM_VALUE_MAX - count)) {
		waiters = -s->value;
		s->value += count;
		if(waiters > 0) {
#if defined(NEED_SEM)
			if(SetEvent(s->sem)) {
				waiters--;
				s->leftToUnblock += count - 1;
				if(s->leftToUnblock > waiters) {
					s->leftToUnblock = waiters;
				}
			}
#else
			if(ReleaseSemaphore(s->sem,  (waiters<=count) ? waiters : count, 0)) {
				/* No action */
			}
#endif
			else {
				s->value -= count;
				result = EINVAL;
			}
		}
	}
	else {
		result = ERANGE;
	}
	__ptw32_mcs_lock_release(&node);
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function stores the current count value of the semaphore.
 * RESULTS
 *
 * Return value
 *    0                  sval has been set.
 *   -1                  failed, error in errno
 *
 *  in global errno
 *
 *   EINVAL              'sem' is not a valid semaphore,
 *   ENOSYS              this function is not supported,
 *
 *
 * PARAMETERS
 *   sem                 pointer to an instance of sem_t
 *   sval                pointer to int.
 *
 * DESCRIPTION
 *   This function stores the current count value of the semaphore
 *   pointed to by sem in the int pointed to by sval.
 */
int sem_getvalue(sem_t * sem, int * sval)
{
	int result = 0;
	__ptw32_mcs_local_node_t node;
	sem_t s = *sem;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	*sval = s->value;
	__ptw32_mcs_lock_release(&node);
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}

static void __PTW32_CDECL __ptw32_sem_wait_cleanup(void * sem)
{
	sem_t s = (sem_t)sem;
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	/*
	 * If sema is destroyed do nothing, otherwise:-
	 * If the sema is posted between us being canceled and us locking
	 * the sema again above then we need to consume that post but cancel
	 * anyway. If we don't get the semaphore we indicate that we're no longer waiting.
	 */
	if(*((sem_t*)sem) != NULL && !(WaitForSingleObject(s->sem, 0) == WAIT_OBJECT_0)) {
		++s->value;
#if defined(NEED_SEM)
		if(s->value > 0) {
			s->leftToUnblock = 0;
		}
#else
		/*
		 * Don't release the W32 sema, it doesn't need adjustment
		 * because it doesn't record the number of waiters.
		 */
#endif /* NEED_SEM */
	}
	__ptw32_mcs_lock_release(&node);
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function  waits on a semaphore.
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *
 * DESCRIPTION
 *   This function waits on a semaphore. If the
 *   semaphore value is greater than zero, it decreases
 *   its value by one. If the semaphore value is zero, then
 *   the calling thread (or process) is blocked until it can
 *   successfully decrease the value or until interrupted by
 *   a signal.
 *
 * RESULTS
 *     0               successfully decreased semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOSYS          semaphores are not supported,
 *     EINTR           the function was interrupted by a signal,
 *     EDEADLK         a deadlock condition was detected.
 *
 * ------------------------------------------------------
 */
int sem_wait(sem_t * sem)
{
	__ptw32_mcs_local_node_t node;
	int v;
	int result = 0;
	sem_t s = *sem;
	pthread_testcancel();
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	v = --s->value;
	__ptw32_mcs_lock_release(&node);
	if(v < 0) {
#if defined (__PTW32_CONFIG_MSVC7)
#pragma inline_depth(0)
#endif
		/* Must wait */
		pthread_cleanup_push(__ptw32_sem_wait_cleanup, (void *)s);
		result = pthreadCancelableWait(s->sem);
		/* Cleanup if we're canceled or on any other error */
		pthread_cleanup_pop(result);
#if defined (__PTW32_CONFIG_MSVC7)
#pragma inline_depth()
#endif
	}
#if defined(NEED_SEM)
	if(!result) {
		__ptw32_mcs_lock_acquire(&s->lock, &node);
		if(s->leftToUnblock > 0) {
			--s->leftToUnblock;
			SetEvent(s->sem);
		}
		__ptw32_mcs_lock_release(&node);
	}
#endif
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function tries to wait on a semaphore.
 *
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *
 * DESCRIPTION
 *   This function tries to wait on a semaphore. If the
 *   semaphore value is greater than zero, it decreases
 *   its value by one. If the semaphore value is zero, then
 *   this function returns immediately with the error EAGAIN
 *
 * RESULTS
 *     0               successfully decreased semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EAGAIN          the semaphore was already locked,
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOTSUP         sem_trywait is not supported,
 *     EINTR           the function was interrupted by a signal,
 *     EDEADLK         a deadlock condition was detected.
 *
 * ------------------------------------------------------
 */
int sem_trywait(sem_t * sem)
{
	int result = 0;
	sem_t s = *sem;
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	if(s->value > 0) {
		s->value--;
	}
	else {
		result = EAGAIN;
	}
	__ptw32_mcs_lock_release(&node);
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}

typedef struct {
	sem_t sem;
	int * resultPtr;
} sem_timedwait_cleanup_args_t;

static void __PTW32_CDECL __ptw32_sem_timedwait_cleanup(void * args)
{
	__ptw32_mcs_local_node_t node;
	sem_timedwait_cleanup_args_t * a = static_cast<sem_timedwait_cleanup_args_t *>(args);
	sem_t s = a->sem;
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	/*
	 * We either timed out or were cancelled.
	 * If someone has posted between then and now we try to take the semaphore.
	 * Otherwise the semaphore count may be wrong after we
	 * return. In the case of a cancellation, it is as if we
	 * were cancelled just before we return (after taking the semaphore)
	 * which is ok.
	 */
	if(WaitForSingleObject(s->sem, 0) == WAIT_OBJECT_0) {
		/* We got the semaphore on the second attempt */
		*(a->resultPtr) = 0;
	}
	else {
		/* Indicate we're no longer waiting */
		s->value++;
#if defined(NEED_SEM)
		if(s->value > 0) {
			s->leftToUnblock = 0;
		}
#else
		/*
		 * Don't release the W32 sema, it doesn't need adjustment
		 * because it doesn't record the number of waiters.
		 */
#endif
	}
	__ptw32_mcs_lock_release(&node);
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function waits on a semaphore possibly until 'abstime' time.
 * PARAMETERS
 *   sem
 *     pointer to an instance of sem_t
 *   abstime
 *     pointer to an instance of struct timespec
 *
 * DESCRIPTION
 *   This function waits on a semaphore. If the
 *   semaphore value is greater than zero, it decreases
 *   its value by one. If the semaphore value is zero, then
 *   the calling thread (or process) is blocked until it can
 *   successfully decrease the value or until interrupted by
 *   a signal.
 *
 *   If 'abstime' is a NULL pointer then this function will
 *   block until it can successfully decrease the value or
 *   until interrupted by a signal.
 *
 * RESULTS
 *     0               successfully decreased semaphore,
 *     -1              failed, error in errno
 * ERRNO
 *     EINVAL          'sem' is not a valid semaphore,
 *     ENOSYS          semaphores are not supported,
 *     EINTR           the function was interrupted by a signal,
 *     EDEADLK         a deadlock condition was detected.
 *     ETIMEDOUT       abstime elapsed before success.
 *
 * ------------------------------------------------------
 */
int sem_timedwait(sem_t * sem, const struct timespec * abstime)
{
	__ptw32_mcs_local_node_t node;
	DWORD milliseconds;
	int v;
	int result = 0;
	sem_t s = *sem;
	pthread_testcancel();
	if(!abstime)
		milliseconds = INFINITE;
	else
		milliseconds = __ptw32_relmillisecs(abstime); // Calculate timeout as milliseconds from current system time.
	__ptw32_mcs_lock_acquire(&s->lock, &node);
	v = --s->value;
	__ptw32_mcs_lock_release(&node);
	if(v < 0) {
#if defined(NEED_SEM)
		int timedout;
#endif
		sem_timedwait_cleanup_args_t cleanup_args;
		cleanup_args.sem = s;
		cleanup_args.resultPtr = &result;
#if defined (__PTW32_CONFIG_MSVC7)
#pragma inline_depth(0)
#endif
		/* Must wait */
		pthread_cleanup_push(__ptw32_sem_timedwait_cleanup, (void *)&cleanup_args);
#if defined(NEED_SEM)
		timedout =
#endif
		result = pthreadCancelableTimedWait(s->sem, milliseconds);
		pthread_cleanup_pop(result);
#if defined (__PTW32_CONFIG_MSVC7)
	#pragma inline_depth()
#endif
#if defined(NEED_SEM)
		if(!timedout) {
			__ptw32_mcs_lock_acquire(&s->lock, &node);
			if(s->leftToUnblock > 0) {
				--s->leftToUnblock;
				SetEvent(s->sem);
			}
			__ptw32_mcs_lock_release(&node);
		}
#endif
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	return 0;
}

int sem_unlink(const char * name)
{
	__PTW32_SET_ERRNO(ENOSYS);
	return -1;
}
