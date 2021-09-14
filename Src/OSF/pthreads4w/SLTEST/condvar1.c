/*
 * File: condvar1.c
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
 *     http://www.apache.org/licenses/LICENSE-2.0
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
// - Test initialisation and destruction of a CV.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_init returns 0, and
// - pthread_cond_destroy returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_init returns non-zero, or
// - pthread_cond_destroy returns non-zero.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar1()
{
	static pthread_cond_t cv = NULL;
	assert(cv == NULL);
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(cv != NULL);
	assert(pthread_cond_destroy(&cv) == 0);
	assert(cv == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test CV linked list management.
// Test Method (Validation or Falsification):
// - Validation:
//   Initiate and destroy several CVs in random order.
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - All initialised CVs destroyed without segfault.
// - Successfully broadcasts all remaining CVs after each CV is removed.
// 
int PThr4wTest_CondVar11()
{
	//static const int NUM_CV = 100;
	static pthread_cond_t cv[100];

	int i, j;
	for(i = 0; i < SIZEOFARRAY(cv); i++) {
		/* Traverse the list before every init of a CV. */
		assert(pthread_timechange_handler_np(NULL) == (void*)0);
		assert(pthread_cond_init(&cv[i], NULL) == 0);
	}
	j = SIZEOFARRAY(cv);
	srand((unsigned)time(NULL));
	do {
		i = (SIZEOFARRAY(cv) - 1) * rand() / RAND_MAX;
		if(cv[i] != NULL) {
			j--;
			assert(pthread_cond_destroy(&cv[i]) == 0);
			/* Traverse the list every time we remove a CV. */
			assert(pthread_timechange_handler_np(NULL) == (void*)0);
		}
	} while(j > 0);
	return 0;
}
// 
// Test Synopsis:
// - Test CV linked list management and serialisation.
// Test Method (Validation or Falsification):
// - Validation:
//   Initiate and destroy several CVs in random order.
//   Asynchronously traverse the CV list and broadcast.
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - All initialised CVs destroyed without segfault.
// - Successfully broadcasts all remaining CVs after each CV is removed.
// 
int PThr4wTest_CondVar12()
{
	//static const int NUM_CV = 5;
	const int NUM_LOOPS = 5;
	static pthread_cond_t cv[5];

	int i, j;
	void * result = (void*)-1;
	pthread_t t;
	for(int k = 0; k < NUM_LOOPS; k++) {
		for(i = 0; i < SIZEOFARRAY(cv); i++) {
			assert(pthread_cond_init(&cv[i], NULL) == 0);
		}
		j = SIZEOFARRAY(cv);
		(void)srand((unsigned)time(NULL));
		// Traverse the list asynchronously. 
		assert(pthread_create(&t, NULL, pthread_timechange_handler_np, NULL) == 0);
		do {
			i = (SIZEOFARRAY(cv) - 1) * rand() / RAND_MAX;
			if(cv[i] != NULL) {
				j--;
				assert(pthread_cond_destroy(&cv[i]) == 0);
			}
		} while(j > 0);
		assert(pthread_join(t, &result) == 0);
		assert((int)(size_t)result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test timed wait on a CV.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because the CV is never signaled, we expect the wait to time out.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar2()
{
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;

	struct timespec abstime = { 0, 0 }, reltime = { 1, 0 };
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	assert(pthread_mutex_lock(&mutex) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_cond_timedwait(&cv, &mutex, &abstime) == ETIMEDOUT);
	assert(pthread_mutex_unlock(&mutex) == 0);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			fprintf(stderr, "Result = %s\n", PThr4wErrorString[result]);
			fprintf(stderr, "\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			fprintf(stderr, "\tWaitersGone = %ld\n", cv->nWaitersGone);
			fprintf(stderr, "\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		assert(result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with no signal/broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because the CV is never signaled, we expect the waits to time out.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar21()
{
	const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static struct timespec abstime = { 0, 0 };
	static struct timespec reltime = { 5, 0 };

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == 0);
			assert(pthread_cond_timedwait(&cv, &mutex, &abstime) == ETIMEDOUT);
			assert(pthread_mutex_unlock(&mutex) == 0);
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void* result = (void*)0;
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_mutex_lock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	assert(pthread_mutex_unlock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_join(t[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			fprintf(stderr, "Result = %s\n", PThr4wErrorString[result]);
			fprintf(stderr, "\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			fprintf(stderr, "\tWaitersGone = %ld\n", cv->nWaitersGone);
			fprintf(stderr, "\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		assert(result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test basic function of a CV
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - The primary thread takes the lock before creating any threads.
//   The secondary thread blocks on the lock allowing the primary
//   thread to enter the cv wait state which releases the lock.
//   The secondary thread then takes the lock and signals the waiting primary thread.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar3()
{
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static int shared = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			assert(pthread_mutex_lock(&mutex) == 0);
			shared++;
			assert(pthread_mutex_unlock(&mutex) == 0);
			if((result = pthread_cond_signal(&cv)) != 0) {
				printf("Error = %s\n", PThr4wErrorString[result]);
			}
			assert(result == 0);
			return 0;
		}
	};
	const int NUMTHREADS = 2; // Including the primary thread
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	assert((t[0] = pthread_self()).p != NULL);
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(shared > 0))
		assert(pthread_cond_timedwait(&cv, &mutex, &abstime) == 0);
	assert(shared > 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_join(t[1], NULL) == 0);
	assert(pthread_cond_destroy(&cv) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with some signaled.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because some CVs are never signaled, we expect their waits to time out.
//   Some are signaled, the rest time out. Pthread_cond_destroy() will fail
//   unless all are accounted for, either signaled or timedout.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar31()
{
	static const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_cond_t cv1;
	static pthread_mutex_t mutex;
	static pthread_mutex_t mutex1;
	static struct timespec abstime = { 0, 0 }, reltime = { 5, 0 };
	static int timedout = 0;
	static int signaled = 0;
	static int awoken = 0;
	static int waiting = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result;
			assert(pthread_mutex_lock(&mutex1) == 0);
			++waiting;
			assert(pthread_mutex_unlock(&mutex1) == 0);
			assert(pthread_cond_signal(&cv1) == 0);
			assert(pthread_mutex_lock(&mutex) == 0);
			result = pthread_cond_timedwait(&cv, &mutex, &abstime);
			if(result == ETIMEDOUT) {
				timedout++;
			}
			else {
				awoken++;
			}
			assert(pthread_mutex_unlock(&mutex) == 0);
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void * result = (void*)0;
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(pthread_cond_init(&cv1, NULL) == 0);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	assert(pthread_mutex_init(&mutex1, NULL) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_mutex_lock(&mutex1) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	do {
		assert(pthread_cond_wait(&cv1, &mutex1) == 0);
	} while(NUMTHREADS > waiting);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	for(i = NUMTHREADS/3; i <= 2*NUMTHREADS/3; i++) {
//      assert(pthread_mutex_lock(&mutex) == 0);
		assert(pthread_cond_signal(&cv) == 0);
//      assert(pthread_mutex_unlock(&mutex) == 0);
		signaled++;
	}
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_join(t[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	fprintf(stderr, "awk = %d\n", awoken);
	fprintf(stderr, "sig = %d\n", signaled);
	fprintf(stderr, "tot = %d\n", timedout);
	assert(signaled == awoken);
	assert(timedout == NUMTHREADS - signaled);
	assert(pthread_cond_destroy(&cv1) == 0);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			fprintf(stderr, "Result = %s\n", PThr4wErrorString[result]);
			fprintf(stderr, "\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			fprintf(stderr, "\tWaitersGone = %ld\n", cv->nWaitersGone);
			fprintf(stderr, "\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		assert(result == 0);
	}
	assert(pthread_mutex_destroy(&mutex1) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with remainder broadcast awoken.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because some CVs are never signaled, we expect their waits to time out.
//   Some time out, the rest are broadcast signaled. Pthread_cond_destroy() will fail
//   unless all are accounted for, either signaled or timedout.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar32()
{
	static const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static struct timespec abstime, abstime2;
	static struct timespec reltime = { 5, 0 };
	static int timedout = 0;
	static int awoken = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result;
			assert(pthread_mutex_lock(&mutex) == 0);
			abstime2.tv_sec = abstime.tv_sec;
			if((int)(size_t)arg % 3 == 0) {
				abstime2.tv_sec += 2;
			}
			result = pthread_cond_timedwait(&cv, &mutex, &abstime2);
			assert(pthread_mutex_unlock(&mutex) == 0);
			if(result == ETIMEDOUT) {
				InterlockedIncrement((LPLONG)&timedout);
			}
			else {
				InterlockedIncrement((LPLONG)&awoken);
			}
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void* result = (void*)0;
	assert(pthread_cond_init(&cv, NULL) == 0);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	abstime2.tv_sec = abstime.tv_sec;
	abstime2.tv_nsec = abstime.tv_nsec;
	assert(pthread_mutex_lock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	assert(pthread_mutex_unlock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_join(t[i], &result) == 0);
		assert((int)(size_t)result == i);
		/*
		 * Approximately 2/3rds of the threads are expected to time out.
		 * Signal the remainder after some threads have woken up and exited
		 * and while some are still waking up after timeout.
		 * Also tests that redundant broadcasts don't return errors.
		 */
		// assert(pthread_mutex_lock(&mutex) == 0);
		if(InterlockedExchangeAdd((LPLONG)&awoken, 0L) > NUMTHREADS/3) {
			assert(pthread_cond_broadcast(&cv) == 0);
		}
		// assert(pthread_mutex_unlock(&mutex) == 0);
	}
	assert(awoken == NUMTHREADS - timedout);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			fprintf(stderr, "Result = %s\n", PThr4wErrorString[result]);
			fprintf(stderr, "\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			fprintf(stderr, "\tWaitersGone = %ld\n", cv->nWaitersGone);
			fprintf(stderr, "\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		assert(result == 0);
	}
	assert(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeouts and lost signals on a CV.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar33()
{
	static pthread_cond_t cnd;
	static pthread_mutex_t mtx;
	static const long NANOSEC_PER_SEC = 1000000000L;

	int rc;
	struct timespec abstime, reltime = { 0, NANOSEC_PER_SEC/2 };
	assert(pthread_cond_init(&cnd, 0) == 0);
	assert(pthread_mutex_init(&mtx, 0) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	/* Here pthread_cond_timedwait should time out after one second. */
	assert(pthread_mutex_lock(&mtx) == 0);
	assert((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);
	assert(pthread_mutex_unlock(&mtx) == 0);
	/* Here, the condition variable is signalled, but there are no
	   threads waiting on it. The signal should be lost and
	   the next pthread_cond_timedwait should time out too. */
	assert((rc = pthread_cond_signal(&cnd)) == 0);
	assert(pthread_mutex_lock(&mtx) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	assert((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);
	assert(pthread_mutex_unlock(&mtx) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test PTHREAD_COND_INITIALIZER.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test basic CV function but starting with a static initialised CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar4()
{
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static const int NUMTHREADS = 2;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
			cvthing.shared++;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			assert(pthread_cond_signal(&cvthing.notbusy) == 0);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	cvthing.shared = 0;
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&cvthing.lock) == 0);
	assert(cvthing.lock != PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == ETIMEDOUT);
	assert(cvthing.notbusy != PTHREAD_COND_INITIALIZER);
	assert(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(cvthing.shared > 0))
		assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
	assert(cvthing.shared > 0);
	assert(pthread_mutex_unlock(&cvthing.lock) == 0);
	assert(pthread_join(t[1], NULL) == 0);
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert(pthread_cond_destroy(&cvthing.notbusy) == 0);
	assert(cvthing.notbusy == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with one waiting CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar5()
{
	static const int NUMTHREADS = 2;
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
			cvthing.shared++;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	cvthing.shared = 0;
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&cvthing.lock) == 0);
	assert(cvthing.lock != PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == ETIMEDOUT);
	assert(cvthing.notbusy != PTHREAD_COND_INITIALIZER);
	assert(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(cvthing.shared > 0))
		assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
	assert(cvthing.shared > 0);
	assert(pthread_mutex_unlock(&cvthing.lock) == 0);
	assert(pthread_join(t[1], NULL) == 0);
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert(pthread_cond_destroy(&cvthing.notbusy) == 0);
	assert(cvthing.notbusy == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with NUMTHREADS (=5) waiting CVs.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar6()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 5, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			assert(pthread_mutex_lock(&start_flag) == 0);
			assert(pthread_mutex_unlock(&start_flag) == 0);
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
			while(!(cvthing.shared > 0))
				assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			assert(cvthing.shared > 0);
			awoken++;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	cvthing.shared = 0;
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&start_flag) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	assert(pthread_mutex_unlock(&start_flag) == 0);
	Sleep(1000); // Give threads time to start.
	assert(pthread_mutex_lock(&cvthing.lock) == 0);
	cvthing.shared++;
	assert(pthread_mutex_unlock(&cvthing.lock) == 0);
	assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);
	// Give threads time to complete.
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_join(t[i], NULL) == 0);
	}
	/*
	 * Cleanup the CV.
	 */
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert(pthread_cond_destroy(&cvthing.notbusy) == 0);
	assert(cvthing.notbusy == NULL);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here.
	assert(awoken == NUMTHREADS);
	return 0; // Success
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast with thread cancellation.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with NUMTHREADS (=5) waiting CVs, one is canceled while waiting.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar7()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 10, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			assert(pthread_mutex_lock(&start_flag) == 0);
			assert(pthread_mutex_unlock(&start_flag) == 0);
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			assert(cvthing.shared > 0);
			awoken++;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	cvthing.shared = 0;
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&start_flag) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	assert(pthread_mutex_unlock(&start_flag) == 0);
	Sleep(1000); // Give threads time to start.
	/*
	 * Cancel one of the threads.
	 */
	assert(pthread_cancel(t[1]) == 0);
	assert(pthread_join(t[1], NULL) == 0);
	assert(pthread_mutex_lock(&cvthing.lock) == 0);
	cvthing.shared++;
	assert(pthread_mutex_unlock(&cvthing.lock) == 0);
	/*
	 * Signal all remaining waiting threads.
	 */
	assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);
	/*
	 * Wait for all threads to complete.
	 */
	for(i = 2; i <= NUMTHREADS; i++)
		assert(pthread_join(t[i], NULL) == 0);
	/*
	 * Cleanup the CV.
	 */
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert(pthread_cond_destroy(&cvthing.notbusy) == 0);
	assert(cvthing.notbusy == NULL);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here.
	assert(awoken == (NUMTHREADS - 1));
	return 0; // Success
}
// 
// Test Synopsis:
// - Test multiple pthread_cond_broadcasts.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Make NUMTHREADS threads wait on CV, broadcast signal them, and then repeat.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_CondVar8()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 10, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			assert(pthread_mutex_lock(&start_flag) == 0);
			assert(pthread_mutex_unlock(&start_flag) == 0);
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			assert(cvthing.shared > 0);
			awoken++;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	int first, last;
	pthread_t t[NUMTHREADS + 1];
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	assert((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(first = 1, last = NUMTHREADS / 2;
	    first < NUMTHREADS;
	    first = last + 1, last = NUMTHREADS) {
		assert(pthread_mutex_lock(&start_flag) == 0);
		for(i = first; i <= last; i++) {
			threadbag[i].started = 0;
			threadbag[i].threadnum = i;
			assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
		}
		/*
		 * Code to control or manipulate child threads should probably go here.
		 */
		cvthing.shared = 0;
		assert(pthread_mutex_unlock(&start_flag) == 0);
		Sleep(100); // Give threads time to start.
		assert(pthread_mutex_lock(&cvthing.lock) == 0);
		cvthing.shared++;
		assert(pthread_mutex_unlock(&cvthing.lock) == 0);
		assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);
		// Give threads time to complete.
		for(i = first; i <= last; i++) {
			assert(pthread_join(t[i], NULL) == 0);
		}
		assert(awoken == (i - 1));
	}
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	/*
	 * Cleanup the CV.
	 */
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert(pthread_cond_destroy(&cvthing.notbusy) == 0);
	assert(cvthing.notbusy == NULL);
	assert(!failed);
	// Check any results here.
	assert(awoken == NUMTHREADS);
	return 0; // Success
}
// 
// Test Synopsis:
// - Test multiple pthread_cond_broadcasts with thread cancellation.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Make NUMTHREADS threads wait on CV, cancel one, broadcast signal them, and then repeat.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
int PThr4wTest_CondVar9()
{
	static const int NUMTHREADS = 9; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 5, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			assert(pthread_mutex_lock(&start_flag) == 0);
			assert(pthread_mutex_unlock(&start_flag) == 0);
			assert(pthread_mutex_lock(&cvthing.lock) == 0);
			/*
			 * pthread_cond_timedwait is a cancellation point and we're
			 * going to cancel some threads deliberately.
			 */
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				assert(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			assert(cvthing.shared > 0);
			awoken++;
			bag->finished = 1;
			assert(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	int first, last;
	int canceledThreads = 0;
	pthread_t t[NUMTHREADS + 1];
	assert((t[0] = pthread_self()).p != NULL);
	assert(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	assert(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(first = 1, last = NUMTHREADS / 2; first < NUMTHREADS; first = last + 1, last = NUMTHREADS) {
		int ct;
		assert(pthread_mutex_lock(&start_flag) == 0);
		for(i = first; i <= last; i++) {
			threadbag[i].started = threadbag[i].finished = 0;
			threadbag[i].threadnum = i;
			assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
		}
		/*
		 * Code to control or manipulate child threads should probably go here.
		 */
		cvthing.shared = 0;
		assert(pthread_mutex_unlock(&start_flag) == 0);
		Sleep(1000); // Give threads time to start.
		ct = (first + last) / 2;
		assert(pthread_cancel(t[ct]) == 0);
		canceledThreads++;
		assert(pthread_join(t[ct], NULL) == 0);
		assert(pthread_mutex_lock(&cvthing.lock) == 0);
		cvthing.shared++;
		assert(pthread_mutex_unlock(&cvthing.lock) == 0);
		assert(pthread_cond_broadcast(&cvthing.notbusy) == 0);
		/*
		 * Standard check that all threads started - and wait for them to finish.
		 */
		for(i = first; i <= last; i++) {
			failed = !threadbag[i].started;
			if(failed) {
				fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
			}
			else {
				assert(pthread_join(t[i], NULL) == 0 || threadbag[i].finished == 0);
//	      fprintf(stderr, "Thread %d: finished %d\n", i, threadbag[i].finished);
			}
		}
	}
	/*
	 * Cleanup the CV.
	 */
	assert(pthread_mutex_destroy(&cvthing.lock) == 0);
	assert(cvthing.lock == NULL);
	assert_e(pthread_cond_destroy(&cvthing.notbusy), ==, 0);
	assert(cvthing.notbusy == NULL);
	assert(!failed);
	// Check any results here.
	assert(awoken == NUMTHREADS - canceledThreads);
	return 0; // Success
}
