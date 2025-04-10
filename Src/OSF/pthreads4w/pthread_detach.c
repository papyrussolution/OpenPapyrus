/*
 * pthread_detach.c
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
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function detaches the given thread.
 *
 * PARAMETERS
 *   thread
 *     an instance of a pthread_t
 *
 *
 * DESCRIPTION
 *   This function detaches the given thread. You may use it to
 *   detach the main thread or to detach a joinable thread.
 *   NOTE:   detached threads cannot be joined;
 *     storage is freed immediately on termination.
 *
 * RESULTS
 *     0               successfully detached the thread,
 *     EINVAL          thread is not a joinable thread,
 *     ENOSPC          a required resource has been exhausted,
 *     ESRCH           no thread could be found for 'thread',
 *
 * ------------------------------------------------------
 */
int pthread_detach(pthread_t thread)
{
	int result;
	BOOL destroyIt =  FALSE;
	__ptw32_thread_t * tp = static_cast<__ptw32_thread_t *>(thread.p);
	__ptw32_mcs_local_node_t reuseLock;
	__ptw32_mcs_lock_acquire(&__ptw32_thread_reuse_lock, &reuseLock);
	if(NULL == tp || thread.x != tp->ptHandle.x) {
		result = ESRCH;
	}
	else if(PTHREAD_CREATE_DETACHED == tp->detachState) {
		result = EINVAL;
	}
	else {
		__ptw32_mcs_local_node_t stateLock;
		// 
		// Joinable __ptw32_thread_t structs are not scavenged until
		// a join or detach is done. The thread may have exited already,
		// but all of the state and locks etc are still there.
		// 
		result = 0;
		__ptw32_mcs_lock_acquire(&tp->stateLock, &stateLock);
		if(tp->state < PThreadStateLast) {
			tp->detachState = PTHREAD_CREATE_DETACHED;
			if(tp->state == PThreadStateExiting)
				destroyIt = TRUE;
		}
		else if(tp->detachState != PTHREAD_CREATE_DETACHED)
			destroyIt =  TRUE; // Thread is joinable and has exited or is exiting.
		__ptw32_mcs_lock_release(&stateLock);
	}
	__ptw32_mcs_lock_release(&reuseLock);
	if(result == 0) {
		// Thread is joinable 
		if(destroyIt) {
			// The thread has exited or is exiting but has not been joined or
			// detached. Need to wait in case it's still exiting.
			::WaitForSingleObject(tp->threadH, INFINITE);
			__ptw32_threadDestroy(thread);
		}
	}
	return result;
}
