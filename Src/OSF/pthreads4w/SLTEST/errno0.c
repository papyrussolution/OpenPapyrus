/*
 * File: errno0.c
 *
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads4w - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999-2018, Pthreads4w contributors
 *
 *      Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * This file is part of Pthreads4w.
 *
 *    Pthreads4w is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Pthreads4w is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Pthreads4w.  If not, see <http://www.gnu.org/licenses/>. *
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"
// 
// Test Synopsis: Test transmissibility of errno between library and exe
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
int PThr4wTest_Errno0()
{
	int err = 0;
	errno = 0;
	assert(errno == 0);
	assert(0 != sem_destroy(NULL));
	err =
#if defined(PTW32_USES_SEPARATE_CRT)
	    GetLastError();
#else
	    errno;
#endif
	assert(err != 0);
	assert(err == EINVAL);
	return 0; // Success
}
// 
// Test Synopsis: Test thread-safety of errno
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
int PThr4wTest_Errno1()
{
	const int NUMTHREADS = 3; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static pthread_mutex_t stop_here = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			assert(bag == &threadbag[bag->threadnum]);
			assert(bag->started == 0);
			bag->started = 1;
			errno = bag->threadnum;
			Sleep(1000);
			pthread_mutex_lock(&stop_here);
			assert(errno == bag->threadnum);
			pthread_mutex_unlock(&stop_here);
			Sleep(1000);
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	pthread_mutex_lock(&stop_here);
	errno = 0;
	assert((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		assert(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(2000);
	pthread_mutex_unlock(&stop_here);
	Sleep(NUMTHREADS * 1000); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			fprintf(stderr, "Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	assert(!failed);
	// Check any results here. Set "failed" and only print ouput on failure.
	for(i = 1; i <= NUMTHREADS; i++) {
		/* ... */
	}
	assert(!failed);
	return 0; // Success
}
