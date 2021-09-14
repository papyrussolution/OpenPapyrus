/*
 * create1.c
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
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"
//
// Descr: Create a thread and check that it ran.
//
int PThr4wTest_Create1()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	// A dirty hack, but we cannot rely on pthread_join in this primitive test. 
	Sleep(2000);
	assert(washere == 1);
	return 0;
}
// 
// Test Synopsis: Test that threads have a Win32 handle when started.
// Test Method (Validation or Falsification): Statistical, not absolute (depends on sample size).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Create2()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			washere = 1;
			return 0;
		}
	};
	const int NUMTHREADS = 10000;
	pthread_t t;
	pthread_attr_t attr;
	void * result = NULL;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	for(int i = 0; i < NUMTHREADS; i++) {
		washere = 0;
		assert(pthread_create(&t, &attr, InnerBlock::func, NULL) == 0);
		assert(pthread_join(t, &result) == 0);
		assert((int)(size_t)result == 0);
		assert(washere == 1);
	}
	return 0;
}
// 
// Test Synopsis: Test passing arg to thread function.
// Test Method (Validation or Falsification): Statistical, not absolute (depends on sample size).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Create3()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			washere = (int)(size_t)arg;
			return 0;
		}
	};
	const int NUMTHREADS = 10000;
	pthread_t t;
	pthread_attr_t attr;
	void * result = NULL;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	for(int i = 0; i < NUMTHREADS; i++) {
		washere = 0;
		assert(pthread_create(&t, &attr, InnerBlock::func, (void*)(size_t)1) == 0);
		assert(pthread_join(t, &result) == 0);
		assert((int)(size_t)result == 0);
		assert(washere == 1);
	}
	return 0;
}
