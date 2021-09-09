/*
 * File: cancel1.c
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
// Test Synopsis: Test setting cancel state and cancel type.
// Test Method (Validation or Falsification): -
// Requirements Tested:
// - pthread_setcancelstate function
// - pthread_setcanceltype function
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - pthread_create, pthread_self work.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
int PThr4wTest_Cancel1()
{
	const int NUMTHREADS = 2; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* ... */
			{
				int oldstate;
				int oldtype;
				assert(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate) == 0);
				assert(oldstate == PTHREAD_CANCEL_ENABLE); /* Check default */
				assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
				assert(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) == 0);
				assert(pthread_setcancelstate(oldstate, &oldstate) == 0);
				assert(oldstate == PTHREAD_CANCEL_DISABLE); /* Check setting */
				assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype) == 0);
				assert(oldtype == PTHREAD_CANCEL_DEFERRED); /* Check default */
				assert(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
				assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
				assert(pthread_setcanceltype(oldtype, &oldtype) == 0);
				assert(oldtype == PTHREAD_CANCEL_ASYNCHRONOUS); /* Check setting */
			}
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	assert((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	for(i = 1; i <= NUMTHREADS; i++) {
		/* ... */
	}
	assert(!failed);
	return 0; // Success
}

#if defined(__cplusplus) // Don't know how to identify if we are using SEH so it's only C++ for now
// 
// Test Synopsis: Test SEH or C++ cancel exception handling within application exception blocks.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Cancel2()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static pthread_barrier_t go = NULL;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			assert(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
			result = 1;
		#if !defined(__cplusplus)
			__try
		#else
			try
		#endif
			{
				/* Wait for go from main */
				pthread_barrier_wait(&go);
				pthread_barrier_wait(&go);

				pthread_testcancel();
			}
		#if !defined(__cplusplus)
			__except(EXCEPTION_EXECUTE_HANDLER)
		#else
		#if defined(__PtW32CatchAll)
			__PtW32CatchAll
		#else
			catch(...)
		#endif
		#endif
			{
				/*
				 * Should not get into here.
				 */
				result += 100;
			}
			/*
			 * Should not get to here either.
			 */
			result += 1000;
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	assert((t[0] = pthread_self()).p != NULL);
	assert(pthread_barrier_init(&go, NULL, NUMTHREADS + 1) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	pthread_barrier_wait(&go);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_cancel(t[i]) == 0);
	}
	pthread_barrier_wait(&go);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		assert(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			fprintf(stderr, "Thread %d: started %d: location %d\n", i, threadbag[i].started, (int)(size_t)result);
		}
		failed |= fail;
	}
	assert(!failed);
	assert(pthread_barrier_destroy(&go) == 0);
	return 0; // Success
}

#else /* defined(__cplusplus) */
	int PThr4wTest_Cancel2()
	{
		fprintf(stderr, "Test N/A for this compiler environment.\n");
		return 0;
	}
#endif /* defined(__cplusplus) */
// 
// Test Synopsis: Test asynchronous cancellation (alertable or non-alertable).
// Requirements Tested:
// - Async cancel if thread is not blocked (i.e. voluntarily resumes if blocked).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join.
// - quserex.dll and alertdrv.sys are not available.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Cancel3()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			// Set to known state and type 
			assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
			// We wait up to 10 seconds, waking every 0.1 seconds, for a cancellation to be applied to us.
			for(bag->count = 0; bag->count < 100; bag->count++)
				Sleep(100);
			return result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	assert((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(NUMTHREADS * 100);
	for(i = 1; i <= NUMTHREADS; i++) {
		assert(pthread_cancel(t[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to complete.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		/*
		 * The thread does not contain any cancellation points, so
		 * a return value of PTHREAD_CANCELED confirms that async
		 * cancellation succeeded.
		 */
		assert(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			fprintf(stderr, "Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	assert(!failed);
	return 0; // Success
}
