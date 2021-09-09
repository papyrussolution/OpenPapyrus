/*
 * File: reuse1.c
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
// - Confirm that thread reuse works for joined threads.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Reuse1()
{
	const int NUMTHREADS = 100;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			washere = 1;
			return arg;
		}
	};
	pthread_t t, last_t;
	pthread_attr_t attr;
	void * result = NULL;
	int i;
	assert(pthread_attr_init(&attr) == 0);;
	assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0);
	washere = 0;
	assert(pthread_create(&t, &attr, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, &result) == 0);;
	assert((int)(size_t)result == 0);
	assert(washere == 1);
	last_t = t;
	for(i = 1; i < NUMTHREADS; i++) {
		washere = 0;
		assert(pthread_create(&t, &attr, InnerBlock::func, (void*)(size_t)i) == 0);
		pthread_join(t, &result);
		assert((int)(size_t)result == i);
		assert(washere == 1);
		/* thread IDs should be unique */
		assert(!pthread_equal(t, last_t));
		/* thread struct pointers should be the same */
		assert(t.p == last_t.p);
		/* thread handle reuse counter should be different by one */
		assert(t.x == last_t.x+1);
		last_t = t;
	}
	return 0;
}
// 
// Test Synopsis:
// - Test that thread reuse works for detached threads.
// - Analyse thread struct reuse.
// Environment:
// - This test is implementation specific
// because it uses knowledge of internals that should be opaque to an application.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Reuse2()
{
	const int NUMTHREADS = 10000;
	static long done = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			sched_yield();
			InterlockedIncrement(&done);
			return (void*)0;
		}
	};
	pthread_t t[NUMTHREADS];
	pthread_attr_t attr;
	int i;
	uint notUnique = 0;
	uint totalHandles = 0;
	uint reuseMax = 0;
	uint reuseMin = NUMTHREADS;
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
	for(i = 0; i < NUMTHREADS; i++) {
		while(pthread_create(&t[i], &attr, InnerBlock::func, NULL) != 0)
			Sleep(1);
	}
	while(NUMTHREADS > InterlockedExchangeAdd((LPLONG)&done, 0L))
		Sleep(100);
	Sleep(100);
	// 
	// Analyse reuse by computing min and max number of times pthread_create()
	// returned the same pthread_t value.
	// 
	for(i = 0; i < NUMTHREADS; i++) {
		if(t[i].p != NULL) {
			uint j;
			uint thisMax = t[i].x;
			for(j = i+1; j < NUMTHREADS; j++) {
				if(t[i].p == t[j].p) {
					if(t[i].x == t[j].x)
						notUnique++;
					if(thisMax < t[j].x)
						thisMax = t[j].x;
					t[j].p = NULL;
				}
			}
			if(reuseMin > thisMax)
				reuseMin = thisMax;
			if(reuseMax < thisMax)
				reuseMax = thisMax;
		}
	}
	for(i = 0; i < NUMTHREADS; i++)
		if(t[i].p != NULL)
			totalHandles++;
	// 
	// pthread_t reuse counts start at 0, so we need to add 1
	// to the max and min values derived above.
	// 
	printf("For %d total threads:\n", NUMTHREADS);
	printf("Non-unique IDs = %d\n", notUnique);
	printf("Reuse maximum  = %d\n", reuseMax + 1);
	printf("Reuse minimum  = %d\n", reuseMin + 1);
	printf("Total handles  = %d\n", totalHandles);
	return 0;
}
