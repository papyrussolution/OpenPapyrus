/*
 * pthread_join.c
 * Description: This translation unit implements functions related to thread synchronisation.
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
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function waits for 'thread' to terminate and
 *   returns the thread's exit value if 'value_ptr' is not
 *   NULL. This also detaches the thread on successful
 *   completion.
 *
 * PARAMETERS
 *   thread
 *     an instance of pthread_t
 *
 *   value_ptr
 *     pointer to an instance of pointer to void
 *
 *
 * DESCRIPTION
 *   This function waits for 'thread' to terminate and
 *   returns the thread's exit value if 'value_ptr' is not
 *   NULL. This also detaches the thread on successful
 *   completion.
 *   NOTE:   detached threads cannot be joined or canceled
 *
 * RESULTS
 *     0               'thread' has completed
 *     EINVAL          thread is not a joinable thread,
 *     ESRCH           no thread could be found with ID 'thread',
 *     ENOENT          thread couldn't find it's own valid handle,
 *     EDEADLK         attempt to join thread with self
 *
 * ------------------------------------------------------
 */
int pthread_join(pthread_t thread, void ** value_ptr)
{
	int result;
	pthread_t self;
	__ptw32_thread_t * tp = static_cast<__ptw32_thread_t *>(thread.p);
	__ptw32_mcs_local_node_t node;
	__ptw32_mcs_lock_acquire(&__ptw32_thread_reuse_lock, &node);
	if(!tp || thread.x != tp->ptHandle.x)
		result = ESRCH;
	else if(PTHREAD_CREATE_DETACHED == tp->detachState)
		result = EINVAL;
	else
		result = 0;
	__ptw32_mcs_lock_release(&node);
	if(result == 0) {
		/*
		 * The target thread is joinable and can't be reused before we join it.
		 */
		self = pthread_self();
		if(!self.p)
			result = ENOENT;
		else if(pthread_equal(self, thread))
			result = EDEADLK;
		else {
			// 
			// Pthread_join is a cancellation point.
			// If we are canceled then our target thread must not be
			// detached (destroyed). This is guaranteed because
			// pthreadCancelableWait will not return if we are canceled.
			// 
			result = pthreadCancelableWait(tp->threadH);
			if(result == 0) {
				ASSIGN_PTR(value_ptr, tp->exitStatus);
				// 
				// The result of making multiple simultaneous calls to pthread_join() or 
				// pthread_detach() specifying the same target is undefined.
				// 
				result = pthread_detach(thread);
			}
			else
				result = ESRCH;
		}
	}
	return result;
}
