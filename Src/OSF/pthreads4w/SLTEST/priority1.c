/*
 * File: priority1.c
 *      Pthreads4w - POSIX Threads for Windows
 *      Copyright 1998 John E. Bossom
 *      Copyright 1999-2018, Pthreads4w contributors
 *
 *      Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *
 *      https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"
// 
// Test Synopsis:
// - Test thread priority explicit setting using thread attribute.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Priority1()
{
	static const int PTW32TEST_THREAD_INIT_PRIO = 0;
	static const int PTW32TEST_MAXPRIORITIES = 512;
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int policy;
			struct sched_param param;
			pthread_t threadID = pthread_self();
			assert(pthread_getschedparam(threadID, &policy, &param) == 0);
			assert(policy == SCHED_OTHER);
			return (void*)(size_t)(param.sched_priority);
		}
		static void * getValidPriorities(void * arg)
		{
			int prioSet;
			pthread_t threadID = pthread_self();
			HANDLE threadH = pthread_getw32threadhandle_np(threadID);
			printf("Using GetThreadPriority\n");
			printf("%10s %10s\n", "Set value", "Get value");
			for(prioSet = minPrio; prioSet <= maxPrio; prioSet++) {
				/*
				 * If prioSet is invalid then the threads priority is unchanged
				 * from the previous value. Make the previous value a known
				 * one so that we can check later.
				 */
				if(prioSet < 0)
					SetThreadPriority(threadH, THREAD_PRIORITY_LOWEST);
				else
					SetThreadPriority(threadH, THREAD_PRIORITY_HIGHEST);
				SetThreadPriority(threadH, prioSet);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
				printf("%10d %10d\n", prioSet, validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)]);
			}
			return (void*)0;
		}
	};
	pthread_t t;
	pthread_attr_t attr;
	void * result = NULL;
	struct sched_param param;
	assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	assert(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);
	/* Set the thread's priority to a known initial value. */
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	printf("Using pthread_getschedparam\n");
	printf("%10s %10s %10s\n", "Set value", "Get value", "Win priority");
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		int prio;
		assert(pthread_attr_setschedparam(&attr, &param) == 0);
		assert(pthread_create(&t, &attr, InnerBlock::func, (void*)&attr) == 0);
		assert((prio = GetThreadPriority(pthread_getw32threadhandle_np(t))) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		assert(pthread_join(t, &result) == 0);
		assert(param.sched_priority == (int)(size_t)result);
		printf("%10d %10d %10d\n", param.sched_priority, (int)(size_t)result, prio);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test thread priority setting after creation.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Priority2()
{
	static const int PTW32TEST_THREAD_INIT_PRIO = 0;
	static const int PTW32TEST_MAXPRIORITIES = 512;
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];
	static pthread_barrier_t startBarrier;
	static pthread_barrier_t endBarrier;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int policy;
			int result;
			struct sched_param param;
			result = pthread_barrier_wait(&startBarrier);
			assert(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
			assert(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
			assert(policy == SCHED_OTHER);
			result = pthread_barrier_wait(&endBarrier);
			assert(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
			return (void*)(size_t)param.sched_priority;
		}
		static void * getValidPriorities(void * arg)
		{
			int prioSet;
			pthread_t thread = pthread_self();
			HANDLE threadH = pthread_getw32threadhandle_np(thread);
			struct sched_param param;
			for(prioSet = minPrio;
				prioSet <= maxPrio;
				prioSet++) {
				/*
				 * If prioSet is invalid then the threads priority is unchanged
				 * from the previous value. Make the previous value a known
				 * one so that we can check later.
				 */
				param.sched_priority = prioSet;
				assert(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
			}
			return (void*)0;
		}
	};
	pthread_t t;
	void * result = NULL;
	int result2;
	struct sched_param param;
	assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	assert(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(pthread_barrier_init(&startBarrier, NULL, 2) == 0);
	assert(pthread_barrier_init(&endBarrier, NULL, 2) == 0);
	/* Set the thread's priority to a known initial value.
	 * If the new priority is invalid then the threads priority
	 * is unchanged from the previous value.
	 */
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
		assert(pthread_setschedparam(t, SCHED_OTHER, &param) == 0);
		result2 = pthread_barrier_wait(&startBarrier);
		assert(result2 == 0 || result2 == PTHREAD_BARRIER_SERIAL_THREAD);
		result2 = pthread_barrier_wait(&endBarrier);
		assert(result2 == 0 || result2 == PTHREAD_BARRIER_SERIAL_THREAD);
		assert(GetThreadPriority(pthread_getw32threadhandle_np(t)) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		pthread_join(t, &result);
		assert(param.sched_priority == (int)(size_t)result);
	}
	return 0;
}

