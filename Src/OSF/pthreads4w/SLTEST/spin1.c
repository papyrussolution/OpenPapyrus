/*
 * spin1.c
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

#define GetDurationMilliSecs(_TStart, _TStop) ((_TStop.time*1000+_TStop.millitm) - (_TStart.time*1000+_TStart.millitm))
//
// Descr: Create a simple spinlock object, lock it, and then unlock it again.
//   This is the simplest test of the pthread mutex family that we can do
//
int PThr4wTest_Spin1()
{
	static pthread_spinlock_t lock;
	assert(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_spin_lock(&lock) == 0);
	assert(pthread_spin_unlock(&lock) == 0);
	assert(pthread_spin_destroy(&lock) == 0);
	assert(pthread_spin_lock(&lock) == EINVAL);
	return 0;
}
//
// Descr: Declare a spinlock object, lock it, trylock it, and then unlock it again.
//
int PThr4wTest_Spin2()
{
	static pthread_spinlock_t lock = NULL;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_spin_trylock(&lock) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_spin_lock(&lock) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_spin_unlock(&lock) == 0);
	assert(pthread_spin_destroy(&lock) == 0);
	assert(washere == 1);
	return 0;
}
//
// Thread A locks spin - thread B tries to unlock. This should succeed, but it's undefined behaviour.
//
int PThr4wTest_Spin3()
{
	static int wasHere = 0;
	static pthread_spinlock_t spin;

	class InnerBlock {
	public:
		static void * unlocker(void * arg)
		{
			int expectedResult = (int)(size_t)arg;
			wasHere++;
			assert(pthread_spin_unlock(&spin) == expectedResult);
			wasHere++;
			return NULL;
		}
	};
	pthread_t t;
	wasHere = 0;
	assert(pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_spin_lock(&spin) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)0) == 0);
	assert(pthread_join(t, NULL) == 0);
	/*
	 * Our spinlocks don't record the owner thread so any thread can unlock the spinlock,
	 * but nor is it an error for any thread to unlock a spinlock that is not locked.
	 */
	assert(pthread_spin_unlock(&spin) == 0);
	assert(pthread_spin_destroy(&spin) == 0);
	assert(wasHere == 2);
	return 0;
}

int PThr4wTest_Spin4()
{
	static pthread_spinlock_t lock = PTHREAD_SPINLOCK_INITIALIZER;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			__PTW32_FTIME(&currSysTimeStart);
			washere = 1;
			assert(pthread_spin_lock(&lock) == 0);
			assert(pthread_spin_unlock(&lock) == 0);
			__PTW32_FTIME(&currSysTimeStop);
			return (void*)(size_t)GetDurationMilliSecs(currSysTimeStart, currSysTimeStop);
		}
	};
	void * result = (void*)0;
	pthread_t t;
	int CPUs;
	__PTW32_STRUCT_TIMEB sysTime;
	if((CPUs = pthread_num_processors_np()) == 1) {
		printf("Test not run - it requires multiple CPUs.\n");
		exit(0);
	}
	assert(pthread_spin_lock(&lock) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	while(washere == 0) {
		sched_yield();
	}
	do {
		sched_yield();
		__PTW32_FTIME(&sysTime);
	} while(GetDurationMilliSecs(currSysTimeStart, sysTime) <= 1000);
	assert(pthread_spin_unlock(&lock) == 0);
	assert(pthread_join(t, &result) == 0);
	assert((int)(size_t)result > 1000);
	assert(pthread_spin_destroy(&lock) == 0);
	assert(washere == 1);
	return 0;
}

