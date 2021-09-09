/*
 * File: semaphore1.c
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

#define MAX_COUNT 100
// 
// Test Synopsis: Verify trywait() returns -1 and sets EAGAIN.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore1()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			sem_t s;
			int result;
			assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert((result = sem_trywait(&s)) == -1);
			if(result == -1) {
				int err =
		#if defined (__PTW32_USES_SEPARATE_CRT)
					GetLastError();
		#else
					errno;
		#endif
				if(err != EAGAIN) {
					printf("thread: sem_trywait 1: expecting error %s: got %s\n", PThr4wErrorString[EAGAIN], PThr4wErrorString[err]); fflush(stdout);
				}
				assert(err == EAGAIN);
			}
			else {
				printf("thread: ok 1\n");
			}
			assert((result = sem_post(&s)) == 0);
			assert((result = sem_trywait(&s)) == 0);
			assert(sem_post(&s) == 0);
			return NULL;
		}
	};
	pthread_t t;
	sem_t s;
	void* result1 = (void*)-1;
	int result2;
	assert(pthread_create(&t, NULL, InnerBlock::thr, NULL) == 0);
	assert(pthread_join(t, &result1) == 0);
	assert((int)(size_t)result1 == 0);
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert((result2 = sem_trywait(&s)) == -1);
	if(result2 == -1) {
		int err =
#if defined (__PTW32_USES_SEPARATE_CRT)
		    GetLastError();
#else
		    errno;
#endif
		if(err != EAGAIN) {
			printf("main: sem_trywait 1: expecting error %s: got %s\n", PThr4wErrorString[EAGAIN], PThr4wErrorString[err]); fflush(stdout);
		}
		assert(err == EAGAIN);
	}
	else {
		printf("main: ok 1\n");
	}
	assert((result2 = sem_post(&s)) == 0);
	assert((result2 = sem_trywait(&s)) == 0);
	assert(sem_post(&s) == 0);
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct value.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore2()
{
	sem_t s;
	int value = 0;
	int i;
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, MAX_COUNT) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == MAX_COUNT);
//	  printf("Value = %ld\n", value);
	for(i = MAX_COUNT - 1; i >= 0; i--) {
		assert(sem_wait(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		assert(value == i);
	}
	for(i = 1; i <= MAX_COUNT; i++) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		assert(value == i);
	}
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore3()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[MAX_COUNT+1];
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	//printf("Value = %d\n", value);	fflush(stdout);
	assert(value == 0);
	for(i = 1; i <= MAX_COUNT; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			assert(sem_getvalue(&s, &value) == 0);
		}
		while(-value != i);
		//printf("1:Value = %d\n", value); fflush(stdout);
		assert(-value == i);
	}
	for(i = MAX_COUNT - 1; i >= 0; i--) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
		//printf("2:Value = %d\n", value);	fflush(stdout);
		assert(-value == i);
	}
	for(i = MAX_COUNT; i > 0; i--) {
		pthread_join(t[i], NULL);
	}
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters after threads are cancelled.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore4()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[MAX_COUNT+1];
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == 0);
	for(i = 1; i <= MAX_COUNT; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			assert(sem_getvalue(&s, &value) == 0);
		} while(value != -i);
		assert(-value == i);
	}
	assert(sem_getvalue(&s, &value) == 0);
	assert(-value == MAX_COUNT);
	assert(pthread_cancel(t[50]) == 0);
	{
		void* result;
		assert(pthread_join(t[50], &result) == 0);
	}
	assert(sem_getvalue(&s, &value) == 0);
	assert(-value == (MAX_COUNT - 1));
	for(i = MAX_COUNT - 2; i >= 0; i--) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
		assert(-value == i);
	}
	for(i = 1; i <= MAX_COUNT; i++)
		if(i != 50)
			assert(pthread_join(t[i], NULL) == 0);
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters after threads are cancelled.
// Test Method (Validation or Falsification):
// - Validation
// Requirements Tested:
// - sem_timedwait cancellation.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore4t()
{
	static const long NANOSEC_PER_SEC = 1000000000L;
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_timedwait(&s, NULL) == 0);
			return NULL;
		}
		static int timeoutwithnanos(sem_t sem, int nanoseconds)
		{
			struct timespec ts, rel;
			FILETIME ft_before, ft_after;
			int rc;
			rel.tv_sec = 0;
			rel.tv_nsec = nanoseconds;
			GetSystemTimeAsFileTime(&ft_before);
			rc = sem_timedwait(&sem, pthread_win32_getabstime_np(&ts, &rel));
			// This should have timed out 
			assert(rc != 0);
			assert(errno == ETIMEDOUT);
			GetSystemTimeAsFileTime(&ft_after);
			// We specified a non-zero wait. Time must advance.
			if(ft_before.dwLowDateTime == ft_after.dwLowDateTime && ft_before.dwHighDateTime == ft_after.dwHighDateTime) {
				printf("nanoseconds: %d, rc: %d, errno: %d. before filetime: %d, %d; after filetime: %d, %d\n",
					nanoseconds, rc, errno,
					(int)ft_before.dwLowDateTime, (int)ft_before.dwHighDateTime,
					(int)ft_after.dwLowDateTime, (int)ft_after.dwHighDateTime);
				printf("time must advance during sem_timedwait.");
				return 1;
			}
			return 0;
		}
		static int testtimeout()
		{
			int rc = 0;
			sem_t s2;
			int value = 0;
			assert(sem_init(&s2, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert(sem_getvalue(&s2, &value) == 0);
			assert(value == 0);
			rc += timeoutwithnanos(s2, 1000);  // 1 microsecond
			rc += timeoutwithnanos(s2, 10 * 1000); // 10 microseconds
			rc += timeoutwithnanos(s2, 100 * 1000); // 100 microseconds
			rc += timeoutwithnanos(s2, 1000 * 1000); // 1 millisecond
			return rc;
		}
		static int testmainstuff()
		{
			int value = 0;
			int i;
			pthread_t t[MAX_COUNT+1];
			assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert(sem_getvalue(&s, &value) == 0);
			assert(value == 0);
			for(i = 1; i <= MAX_COUNT; i++) {
				assert(pthread_create(&t[i], NULL, thr, NULL) == 0);
				do {
					sched_yield();
					assert(sem_getvalue(&s, &value) == 0);
				} while(value != -i);
				assert(-value == i);
			}
			assert(sem_getvalue(&s, &value) == 0);
			assert(-value == MAX_COUNT);
			assert(pthread_cancel(t[50]) == 0);
			assert(pthread_join(t[50], NULL) == 0);
			assert(sem_getvalue(&s, &value) == 0);
			assert(-value == MAX_COUNT - 1);
			for(i = MAX_COUNT - 2; i >= 0; i--) {
				assert(sem_post(&s) == 0);
				assert(sem_getvalue(&s, &value) == 0);
				assert(-value == i);
			}
			for(i = 1; i <= MAX_COUNT; i++) {
				if(i != 50) {
					assert(pthread_join(t[i], NULL) == 0);
				}
			}
			return 0;
		}
	};
	int rc = 0;
	rc += InnerBlock::testmainstuff();
	rc += InnerBlock::testtimeout();
	return rc;
}
// 
// Test Synopsis: Verify sem_destroy EBUSY race avoidance
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Semaphore5()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_post((sem_t*)arg) == 0);
			return 0;
		}
	};
	pthread_t t;
	sem_t s;
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::thr, (void*)&s) == 0);
	assert(sem_wait(&s) == 0);
	/*
	 * Normally we would retry this next, but we're only
	 * interested in unexpected results in this test.
	 */
	assert(sem_destroy(&s) == 0 || errno == EBUSY);
	assert(pthread_join(t, NULL) == 0);
	return 0;
}
