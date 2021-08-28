/*
 * File: exit4.c
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
 *
 * --------------------------------------------------------------------------
 *
 * Test Synopsis: Test calling pthread_exit from a Win32 thread
 *                without having created an implicit POSIX handle for it.
 *
 * Test Method (Validation or Falsification):
 * -
 *
 * Requirements Tested:
 * -
 *
 * Features Tested:
 * -
 *
 * Cases Tested:
 * -
 *
 * Description:
 * -
 *
 * Environment:
 * -
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - No output on success.
 *
 * Assumptions:
 * - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
 *   pthread_testcancel, pthread_cancel
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"
#ifndef _UWIN
	#include <process.h>
#endif
/*
 * Create NUMTHREADS threads in addition to the Main thread.
 */
enum {
	NUMTHREADS = 4
};

static bag_t threadbag[NUMTHREADS + 1];

#if !defined (__MINGW32__) || defined (__MSVCRT__)
	static uint __stdcall
#else
	static void
#endif
Win32thread(void * arg)
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

int PThr4wTest_Exit4()
{
	int failed = 0;
	int i;
	HANDLE h[NUMTHREADS + 1];
	uint thrAddr; /* Dummy variable to pass a valid location to _beginthreadex (Win98). */
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		h[i] = (HANDLE)_beginthreadex(NULL, 0, Win32thread, (void*)&threadbag[i], 0, &thrAddr);
#else
		h[i] = (HANDLE)_beginthread(Win32thread, 0, (void*)&threadbag[i]);
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
