/*
 * Test for pthread_exit().
 *
 *
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
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"
#ifndef _UWIN
	#include <process.h>
#endif
#include <pthread.h>

int PThr4wTest_Exit1()
{
	// A simple test first. 
	pthread_exit((void*)0);
	return 1; // Not reached 
}

int PThr4wTest_Exit2()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int failed = (int)arg;
			pthread_exit(arg);
			/* Never reached. */
			/*
			 * Trick gcc compiler into not issuing a warning here
			 */
			assert(failed - (int)arg);
			return NULL;
		}
	};
	pthread_t t;
	assert(pthread_create(&t, NULL, InnerBlock::func, (void*)NULL) == 0);
	Sleep(100);
	return 0;
}

int PThr4wTest_Exit3()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int failed = (int)arg;
			pthread_exit(arg);
			/* Never reached. */
			/*
			 * assert(0) in a way to prevent warning or optimising away.
			 */
			assert(failed - (int)arg);
			return NULL;
		}
	};
	pthread_t id[4];
	int i;
	/* Create a few threads and then exit. */
	for(i = 0; i < 4; i++) {
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	Sleep(400);
	return 0; // Success
}
// 
// Test Synopsis: Test calling pthread_exit from a Win32 thread without having created an implicit POSIX handle for it.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Exit4()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		#if !defined (__MINGW32__) || defined (__MSVCRT__)
			static uint __stdcall Win32thread(void * arg)
		#else
			static void Win32thread(void * arg)
		#endif
		{
			int result = 1;
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			// Doesn't return and doesn't create an implicit POSIX handle.
			pthread_exit((void*)(size_t)result);
			return 0;
		}
	};
	int failed = 0;
	int i;
	HANDLE h[NUMTHREADS + 1];
	uint thrAddr; /* Dummy variable to pass a valid location to _beginthreadex (Win98). */
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		h[i] = (HANDLE)_beginthreadex(NULL, 0, InnerBlock::Win32thread, (void*)&threadbag[i], 0, &thrAddr);
#else
		h[i] = (HANDLE)_beginthread(InnerBlock::Win32thread, 0, (void*)&threadbag[i]);
#endif
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	Sleep(NUMTHREADS * 100); // Give threads time to run.
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
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		assert(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		// Can't get a result code.
		result = 1;
#endif
		fail = (result != 1);
		if(fail) {
			fprintf(stderr, "Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	assert(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test calling pthread_exit from a Win32 thread having created an implicit POSIX handle for it.
// Test Method (Validation or Falsification):
// - Validate return value and that POSIX handle is created and destroyed.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock pthread_testcancel, pthread_cancel
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Exit5()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		#if !defined (__MINGW32__) || defined (__MSVCRT__)
		static uint __stdcall Win32thread(void * arg)
		#else
		static void Win32thread(void * arg)
		#endif
		{
			int result = 1;
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			assert((bag->self = pthread_self()).p != NULL);
			assert(pthread_kill(bag->self, 0) == 0);
			// Doesn't return.
			pthread_exit((void*)(size_t)result);
			return 0;
		}
	};
	int failed = 0;
	int i;
	HANDLE h[NUMTHREADS + 1];
	unsigned thrAddr; /* Dummy variable to pass a valid location to _beginthreadex (Win98). */
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		h[i] = (HANDLE)_beginthreadex(NULL, 0, InnerBlock::Win32thread, (void*)&threadbag[i], 0, &thrAddr);
#else
		h[i] = (HANDLE)_beginthread(InnerBlock::Win32thread, 0, (void*)&threadbag[i]);
#endif
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	Sleep(NUMTHREADS * 100); // Give threads time to run.
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
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		assert(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		result = 1; // Can't get a result code.
#endif
		assert(threadbag[i].self.p != NULL);
		assert(pthread_kill(threadbag[i].self, 0) == ESRCH);
		fail = (result != 1);
		if(fail) {
			fprintf(stderr, "Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	assert(!failed);
	return 0; // Success
}

int PThr4wTest_Exit6()
{
	static pthread_key_t key;
	static int where;

	class InnerBlock {
	public:
		static unsigned __stdcall start_routine(void * arg)
		{
			int * val = (int*)malloc(4);
			where = 2;
			//printf("start_routine: native thread\n");
			*val = 48;
			pthread_setspecific(key, val);
			return 0;
		}
		static void key_dtor(void * arg)
		{
			//printf("key_dtor: %d\n", *(int*)arg);
			if(where == 2)
				printf("Library has thread exit POSIX cleanup for native threads.\n");
			else
				printf("Library has process exit POSIX cleanup for native threads.\n");
			free(arg);
		}
	};
	HANDLE hthread;
	where = 1;
	pthread_key_create(&key, InnerBlock::key_dtor);
	hthread = (HANDLE)_beginthreadex(NULL, 0, InnerBlock::start_routine, NULL, 0, NULL);
	WaitForSingleObject(hthread, INFINITE);
	CloseHandle(hthread);
	where = 3;
	pthread_key_delete(key);
	//printf("main: exiting\n");
	return 0;
}
