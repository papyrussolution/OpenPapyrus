/*
 * barrier1.c
 * --------------------------------------------------------------------------
 *
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
 *
 * --------------------------------------------------------------------------
 *
 * Create a barrier object and then destroy it.
 * Declare a single barrier object, wait on it, and then destroy it.
 * Declare a single barrier object with barrier attribute, wait on it, and then destroy it.
 * Declare a single barrier object, multiple wait on it, and then destroy it.
 *
 * Set up a series of barriers at different heights and test various numbers
 * of threads accessing, especially cases where there are more threads than the
 * barrier height (count), i.e. test contention when the barrier is released.
 *
 * Destroy the barrier after initial count threads are released then let additional threads attempt to wait on it.
 *
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"

int PThr4wTest_Barrier1()
{
	static pthread_barrier_t barrier = NULL;
	assert(barrier == NULL);
	assert(pthread_barrier_init(&barrier, NULL, 1) == 0);
	assert(barrier != NULL);
	assert(pthread_barrier_destroy(&barrier) == 0);
	assert(barrier == NULL);
	return 0;
}

int PThr4wTest_Barrier2()
{
	static pthread_barrier_t barrier = NULL;
	assert(pthread_barrier_init(&barrier, NULL, 1) == 0);
	assert(pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD);
	assert(pthread_barrier_destroy(&barrier) == 0);
	return 0;
}

int PThr4wTest_Barrier3()
{
	static pthread_barrier_t barrier = NULL;
	static void * result = (void*)1;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			return (void*)(size_t)pthread_barrier_wait(&barrier);
		}
	};
	pthread_t t;
	pthread_barrierattr_t ba;
	assert(pthread_barrierattr_init(&ba) == 0);
	assert(pthread_barrierattr_setpshared(&ba, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_barrier_init(&barrier, &ba, 1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert((int)(size_t)result == PTHREAD_BARRIER_SERIAL_THREAD);
	assert(pthread_barrier_destroy(&barrier) == 0);
	assert(pthread_barrierattr_destroy(&ba) == 0);
	return 0;
}

int PThr4wTest_Barrier4()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			assert(pthread_mutex_lock(&mx) == 0);
			if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
				serialThreadCount++;
			}
			else if(0 == result) {
				otherThreadCount++;
			}
			else {
				printf("Barrier wait failed: error = %s\n", PThr4wErrorString[result]);
				fflush(stdout);
				return NULL;
			}
			assert(pthread_mutex_unlock(&mx) == 0);
			return NULL;
		}
	};
	const int NUMTHREADS = 16;
	pthread_t t[NUMTHREADS + 1];
	for(int j = 1; j <= NUMTHREADS; j++) {
		int i;
		printf("Barrier height = %d\n", j);
		serialThreadCount = 0;
		assert(pthread_barrier_init(&barrier, NULL, j) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, NULL) == 0);
		}
		for(i = 1; i <= j; i++) {
			assert(pthread_join(t[i], NULL) == 0);
		}
		assert(serialThreadCount == 1);
		assert(pthread_barrier_destroy(&barrier) == 0);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

int PThr4wTest_Barrier5()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static LONG totalThreadCrossings;
	class InnerBlock {
	public:
		static void * func(void * crossings)
		{
			int result;
			int serialThreads = 0;
			while((LONG)(size_t)crossings >= (LONG)InterlockedIncrement((LPLONG)&totalThreadCrossings)) {
				result = pthread_barrier_wait(&barrier);
				if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
					serialThreads++;
				}
				else if(result != 0) {
					printf("Barrier failed: result = %s\n", PThr4wErrorString[result]);
					fflush(stdout);
					return NULL;
				}
			}
			return (void*)(size_t)serialThreads;
		}
	};
	const int NUMTHREADS = 15;
	const int HEIGHT = 10;
	const int BARRIERMULTIPLE = 1000;
	int i, j;
	void * result;
	int serialThreadsTotal;
	LONG Crossings;
	pthread_t t[NUMTHREADS + 1];
	for(j = 1; j <= NUMTHREADS; j++) {
		int height = (j < HEIGHT) ? j : HEIGHT;
		totalThreadCrossings = 0;
		Crossings = height * BARRIERMULTIPLE;
		printf("Threads=%d, Barrier height=%d\n", j, height);
		assert(pthread_barrier_init(&barrier, NULL, height) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, (void*)(size_t)Crossings) == 0);
		}
		serialThreadsTotal = 0;
		for(i = 1; i <= j; i++) {
			assert(pthread_join(t[i], &result) == 0);
			serialThreadsTotal += (int)(size_t)result;
		}
		assert(serialThreadsTotal == BARRIERMULTIPLE);
		assert(pthread_barrier_destroy(&barrier) == 0);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

int PThr4wTest_Barrier6()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			assert(pthread_mutex_lock(&mx) == 0);
			if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
				serialThreadCount++;
			}
			else if(0 == result) {
				otherThreadCount++;
			}
			assert(pthread_mutex_unlock(&mx) == 0);
			return NULL;
		}
	};
	const int NUMTHREADS = 31;
	pthread_t t[NUMTHREADS + 1];
	for(int j = 1; j <= NUMTHREADS; j++) {
		int i;
		int howHigh = j/2 + 1;
		printf("Barrier height = %d, Total threads %d\n", howHigh, j);
		serialThreadCount = 0;
		otherThreadCount = 0;
		assert(pthread_barrier_init(&barrier, NULL, howHigh) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, NULL) == 0);
			if(i == howHigh) {
				for(int k = 1; k <= howHigh; k++) {
					assert(pthread_join(t[k], NULL) == 0);
				}
				assert(pthread_barrier_destroy(&barrier) == 0);
			}
		}
		for(i = howHigh+1; i <= j; i++) {
			assert(pthread_join(t[i], NULL) == 0);
		}
		assert(serialThreadCount == 1);
		assert(otherThreadCount == (howHigh - 1));
		assert(pthread_barrier_destroy(&barrier) == EINVAL);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
	return 0;
}
