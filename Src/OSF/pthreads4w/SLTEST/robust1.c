/*
 * robust1.c
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
// For all robust mutex types.
// Thread A locks mutex
// Thread A terminates with no threads waiting on robust mutex
// Thread B acquires (inherits) mutex and unlocks
// Main attempts to lock mutex with unrecovered state.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock(),
//   pthread_mutex_destroy(), pthread_mutexattr_init(), pthread_mutexattr_setrobust(), pthread_mutexattr_settype(), pthread_mutexattr_destroy()
// 
int PThr4wTest_Robust1()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	//
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			return 0;
		}
		static void * inheritor(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	//
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	// Default (NORMAL) type 
	lockCount = 0;
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	Sleep(100); // @sobolev
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_unlock(&mutex) == EPERM);
	assert(pthread_mutex_destroy(&mutex) == 0);

	// NORMAL type 
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_unlock(&mutex) == EPERM);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_unlock(&mutex) == EPERM);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_unlock(&mutex) == EPERM);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
// 
// For all robust mutex types.
// Thread A locks mutex
// Thread B blocks on mutex
// Thread A terminates with threads waiting on robust mutex
// Thread B awakes and inherits mutex and unlocks
// Main attempts to lock mutex with unrecovered state.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock(),
//   pthread_mutex_destroy(), pthread_mutexattr_init(), pthread_mutexattr_setrobust(), pthread_mutexattr_settype(), pthread_mutexattr_destroy()
// 
int PThr4wTest_Robust2()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;

	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);

	/* Default (NORMAL) type */
	lockCount = 0;
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* NORMAL type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
// 
// For all robust mutex types.
// Thread A locks mutex
// Thread B blocks on mutex
// Thread A terminates with threads waiting on robust mutex
// Thread B awakes and inherits mutex, sets consistent and unlocks
// Main acquires mutex with recovered state.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_consistent(), 
//   pthread_mutex_destroy(), pthread_mutexattr_init(), pthread_mutexattr_setrobust(), pthread_mutexattr_settype(), pthread_mutexattr_destroy()
// 
int PThr4wTest_Robust3()
{
	static int lockCount;
	static pthread_mutex_t mutex;

	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			assert(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_consistent(&mutex) == 0);
			assert(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	/* Default (NORMAL) type */
	lockCount = 0;
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* NORMAL type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	assert(pthread_mutex_init(&mutex, &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 2);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
// 
// Thread A locks multiple robust mutexes
// Thread B blocks on same mutexes in different orderings
// Thread A terminates with thread waiting on mutexes
// Thread B awakes and inherits each mutex in turn, sets consistent and unlocks
// Main acquires mutexes with recovered state.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_destroy(),
//   pthread_mutexattr_init(), pthread_mutexattr_setrobust(), pthread_mutexattr_settype(), pthread_mutexattr_destroy()
// 
int PThr4wTest_Robust4()
{
	static int lockCount;
	static pthread_mutex_t mutex[3];
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			assert(pthread_mutex_lock(&mutex[0]) == 0);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[1]) == 0);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[2]) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			int * o = static_cast<int *>(arg);
			assert(pthread_mutex_lock(&mutex[o[0]]) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[o[1]]) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[o[2]]) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_consistent(&mutex[o[2]]) == 0);
			assert(pthread_mutex_consistent(&mutex[o[1]]) == 0);
			assert(pthread_mutex_consistent(&mutex[o[0]]) == 0);
			assert(pthread_mutex_unlock(&mutex[o[2]]) == 0);
			assert(pthread_mutex_unlock(&mutex[o[1]]) == 0);
			assert(pthread_mutex_unlock(&mutex[o[0]]) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	int order[3];
	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	order[0] = 0;
	order[1] = 1;
	order[2] = 2;
	lockCount = 0;
	assert(pthread_mutex_init(&mutex[0], &ma) == 0);
	assert(pthread_mutex_init(&mutex[1], &ma) == 0);
	assert(pthread_mutex_init(&mutex[2], &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 6);
	assert(pthread_mutex_lock(&mutex[0]) == 0);
	assert(pthread_mutex_unlock(&mutex[0]) == 0);
	assert(pthread_mutex_destroy(&mutex[0]) == 0);
	assert(pthread_mutex_lock(&mutex[1]) == 0);
	assert(pthread_mutex_unlock(&mutex[1]) == 0);
	assert(pthread_mutex_destroy(&mutex[1]) == 0);
	assert(pthread_mutex_lock(&mutex[2]) == 0);
	assert(pthread_mutex_unlock(&mutex[2]) == 0);
	assert(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 1;
	order[1] = 0;
	order[2] = 2;
	lockCount = 0;
	assert(pthread_mutex_init(&mutex[0], &ma) == 0);
	assert(pthread_mutex_init(&mutex[1], &ma) == 0);
	assert(pthread_mutex_init(&mutex[2], &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 6);
	assert(pthread_mutex_lock(&mutex[0]) == 0);
	assert(pthread_mutex_unlock(&mutex[0]) == 0);
	assert(pthread_mutex_destroy(&mutex[0]) == 0);
	assert(pthread_mutex_lock(&mutex[1]) == 0);
	assert(pthread_mutex_unlock(&mutex[1]) == 0);
	assert(pthread_mutex_destroy(&mutex[1]) == 0);
	assert(pthread_mutex_lock(&mutex[2]) == 0);
	assert(pthread_mutex_unlock(&mutex[2]) == 0);
	assert(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 0;
	order[1] = 2;
	order[2] = 1;
	lockCount = 0;
	assert(pthread_mutex_init(&mutex[0], &ma) == 0);
	assert(pthread_mutex_init(&mutex[1], &ma) == 0);
	assert(pthread_mutex_init(&mutex[2], &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 6);
	assert(pthread_mutex_lock(&mutex[0]) == 0);
	assert(pthread_mutex_unlock(&mutex[0]) == 0);
	assert(pthread_mutex_destroy(&mutex[0]) == 0);
	assert(pthread_mutex_lock(&mutex[1]) == 0);
	assert(pthread_mutex_unlock(&mutex[1]) == 0);
	assert(pthread_mutex_destroy(&mutex[1]) == 0);
	assert(pthread_mutex_lock(&mutex[2]) == 0);
	assert(pthread_mutex_unlock(&mutex[2]) == 0);
	assert(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 2;
	order[1] = 1;
	order[2] = 0;
	lockCount = 0;
	assert(pthread_mutex_init(&mutex[0], &ma) == 0);
	assert(pthread_mutex_init(&mutex[1], &ma) == 0);
	assert(pthread_mutex_init(&mutex[2], &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 6);
	assert(pthread_mutex_lock(&mutex[0]) == 0);
	assert(pthread_mutex_unlock(&mutex[0]) == 0);
	assert(pthread_mutex_destroy(&mutex[0]) == 0);
	assert(pthread_mutex_lock(&mutex[1]) == 0);
	assert(pthread_mutex_unlock(&mutex[1]) == 0);
	assert(pthread_mutex_destroy(&mutex[1]) == 0);
	assert(pthread_mutex_lock(&mutex[2]) == 0);
	assert(pthread_mutex_unlock(&mutex[2]) == 0);
	assert(pthread_mutex_destroy(&mutex[2]) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
// 
// Thread A locks multiple robust mutexes
// Thread B blocks on same mutexes
// Thread A terminates with thread waiting on mutexes
// Thread B awakes and inherits each mutex in turn
// Thread B terminates leaving orphaned mutexes
// Main inherits mutexes, sets consistent and unlocks.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutex_init(), pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_destroy(),
//   pthread_mutexattr_init(), pthread_mutexattr_setrobust(), pthread_mutexattr_settype(), pthread_mutexattr_destroy()
// 
int PThr4wTest_Robust5()
{
	static int lockCount;
	static pthread_mutex_t mutex[3];

	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			assert(pthread_mutex_lock(&mutex[0]) == 0);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[1]) == 0);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[2]) == 0);
			lockCount++;
			return 0;
		}
		static void * inheritor(void * arg)
		{
			assert(pthread_mutex_lock(&mutex[0]) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[1]) == EOWNERDEAD);
			lockCount++;
			assert(pthread_mutex_lock(&mutex[2]) == EOWNERDEAD);
			lockCount++;
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	lockCount = 0;
	assert(pthread_mutex_init(&mutex[0], &ma) == 0);
	assert(pthread_mutex_init(&mutex[1], &ma) == 0);
	assert(pthread_mutex_init(&mutex[2], &ma) == 0);
	assert(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	assert(pthread_join(to, NULL) == 0);
	assert(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	assert(pthread_join(ti, NULL) == 0);
	assert(lockCount == 6);
	assert(pthread_mutex_lock(&mutex[0]) == EOWNERDEAD);
	assert(pthread_mutex_consistent(&mutex[0]) == 0);
	assert(pthread_mutex_unlock(&mutex[0]) == 0);
	assert(pthread_mutex_destroy(&mutex[0]) == 0);
	assert(pthread_mutex_lock(&mutex[1]) == EOWNERDEAD);
	assert(pthread_mutex_consistent(&mutex[1]) == 0);
	assert(pthread_mutex_unlock(&mutex[1]) == 0);
	assert(pthread_mutex_destroy(&mutex[1]) == 0);
	assert(pthread_mutex_lock(&mutex[2]) == EOWNERDEAD);
	assert(pthread_mutex_consistent(&mutex[2]) == 0);
	assert(pthread_mutex_unlock(&mutex[2]) == 0);
	assert(pthread_mutex_destroy(&mutex[2]) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
