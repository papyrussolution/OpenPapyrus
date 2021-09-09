/*
 * Test for pthread_join().
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
// Depends on API functions: pthread_create(), pthread_exit().
//
int PThr4wTest_Join0()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(2000);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	pthread_t id;
	void* result = (void*)0;
	// Create a single thread and wait for it to exit. 
	assert(pthread_create(&id, NULL, InnerBlock::func, (void*)123) == 0);
	assert(pthread_join(id, &result) == 0);
	assert((int)(size_t)result == 123);
	return 0; // Success
}
//
// Depends on API functions: pthread_create(), pthread_join(), pthread_exit().
//
int PThr4wTest_Join1()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int i = (int)(size_t)arg;
			Sleep(i * 100);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	pthread_t id[4];
	int i;
	void * result = (void*)-1;
	// Create a few threads and then exit. 
	for(i = 0; i < 4; i++) {
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	// Some threads will finish before they are joined, some after. 
	Sleep(2 * 100 + 50);
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join2()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(1000);
			return arg;
		}
	};
	pthread_t id[4];
	int i;
	void * result = (void*)-1;
	// Create a few threads and then exit
	for(i = 0; i < 4; i++) {
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join3()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			sched_yield();
			return arg;
		}
	};
	pthread_t id[4];
	int i;
	void* result = (void*)-1;
	/* Create a few threads and then exit. */
	for(i = 0; i < 4; i++) {
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	/*
	 * Let threads exit before we join them.
	 * We should still retrieve the exit code for the threads.
	 */
	Sleep(1000);
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join4()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(1200);
			return arg;
		}
	};
	pthread_t id;
	struct timespec abstime, reltime = { 1, 0 };
	void * result = (void*)-1;
	assert(pthread_create(&id, NULL, InnerBlock::func, (void*)(size_t)999) == 0);
	/*
	 * Let thread start before we attempt to join it.
	 */
	Sleep(100);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	/* Test for pthread_timedjoin_np timeout */
	assert(pthread_timedjoin_np(id, &result, &abstime) == ETIMEDOUT);
	assert((int)(size_t)result == -1);
	/* Test for pthread_tryjoin_np behaviour before thread has exited */
	assert(pthread_tryjoin_np(id, &result) == EBUSY);
	assert((int)(size_t)result == -1);
	Sleep(500);
	/* Test for pthread_tryjoin_np behaviour after thread has exited */
	assert(pthread_tryjoin_np(id, &result) == 0);
	assert((int)(size_t)result == 999);
	return 0; // Success
}

