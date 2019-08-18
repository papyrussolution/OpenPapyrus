/*
 * pthread_mutex_init.c
 *
 * Description:
 * This translation unit implements mutual exclusion (mutex) primitives.
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
 *   following World Wide Web location:
 *
 *   https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
			/*
			 * Creating mutex that can be shared between
			 * processes.
			 */
#if _POSIX_THREAD_PROCESS_SHARED >= 0
			/*
			 * Not implemented yet.
			 */
#error ERROR [__FILE__, line __LINE__]: Process shared mutexes are not supported yet.
#else
			return ENOSYS;
#endif /* _POSIX_THREAD_PROCESS_SHARED */
		}
	}
	mx = (pthread_mutex_t)SAlloc::C(1, sizeof(*mx));
	if(mx == NULL) {
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
				/*
				 * Use the negative range to represent robust types.
				 * Replaces a memory fetch with a register negate and incr
				 * in pthread_mutex_lock etc.
				 *
				 * Map 0,1,..,n to -1,-2,..,(-n)-1
				 */
				mx->kind = -mx->kind - 1;
				mx->robustNode = (__ptw32_robust_node_t*)SAlloc::M(sizeof(__ptw32_robust_node_t));
				if(NULL == mx->robustNode) {
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
		if(0 == result) {
			mx->ownerThread.p = NULL;
			mx->event = CreateEvent(NULL,  __PTW32_FALSE/* manual reset = No */, __PTW32_FALSE/* initial state = not signalled */, NULL/* event name */);
			if(0 == mx->event) {
				result = ENOSPC;
			}
		}
	}
	if(0 != result) {
		SAlloc::F(mx->robustNode);
		SAlloc::F(mx);
		mx = NULL;
	}
	*mutex = mx;
	return result;
}
