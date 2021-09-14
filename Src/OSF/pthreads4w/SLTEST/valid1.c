/*
 * File: valid1.c
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
// - Test that thread validation works.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Valid1()
{
	const int NUMTHREADS = 1;
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
	void * result = NULL;
	washere = 0;
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert((int)(size_t)result == 0);
	assert(washere == 1);
	sched_yield();
	assert(pthread_kill(t, 0) == ESRCH);
	return 0;
}
// 
// Test Synopsis:
// - Confirm that thread validation fails for garbage thread ID.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
int PThr4wTest_Valid2()
{
	pthread_t NullThread =  __PTW32_THREAD_NULL_ID;
	assert(pthread_kill(NullThread, 0) == ESRCH);
	return 0;
}
