/*
 * mutex1.c
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
// Create a simple mutex object, lock it, and then unlock it again.
// This is the simplest test of the pthread mutex family that we can do.
// 
// Depends on API functions: pthread_mutex_init(),  pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_destroy()
// 
int PThr4wTest_Mutex1()
{
	static pthread_mutex_t mutex = NULL;
	assert(mutex == NULL);
	assert(pthread_mutex_init(&mutex, NULL) == 0);
	assert(mutex != NULL);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	return 0;
}
//
// As for mutex1.c but with type set to PTHREAD_MUTEX_ERRORCHECK.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
int PThr4wTest_Mutex1e()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	assert(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(mutex == NULL);
	assert(pthread_mutex_init(&mutex, &mxAttr) == 0);
	assert(mutex != NULL);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// As for mutex1.c but with type set to PTHREAD_MUTEX_NORMAL.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
int PThr4wTest_Mutex1n()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	assert(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	assert(mutex == NULL);
	assert(pthread_mutex_init(&mutex, &mxAttr) == 0);
	assert(mutex != NULL);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// As for mutex1.c but with type set to PTHREAD_MUTEX_RECURSIVE.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
int PThr4wTest_Mutex1r()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	assert(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	assert(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);
	assert(mutex == NULL);
	assert(pthread_mutex_init(&mutex, &mxAttr) == 0);
	assert(mutex != NULL);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex2()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	assert(mutex == PTHREAD_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(mutex != PTHREAD_MUTEX_INITIALIZER);
	assert(mutex != NULL);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex2e()
{
	static pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
	assert(mutex == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(mutex != PTHREAD_ERRORCHECK_MUTEX_INITIALIZER);
	assert(mutex != NULL);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex2r()
{
	static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
	assert(mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
	assert(pthread_mutex_lock(&mutex) == 0);
	assert(mutex != PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
	assert(mutex != NULL);
	assert(pthread_mutex_unlock(&mutex) == 0);
	assert(pthread_mutex_destroy(&mutex) == 0);
	assert(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex3()
{
	static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	static int washere = 0;
	
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex3e()
{
	static pthread_mutex_t mutex1 = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex3r()
{
	static pthread_mutex_t mutex1 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Thread A locks mutex - thread B tries to unlock.
// Depends on API functions:
//  pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
int PThr4wTest_Mutex4()
{
	static int wasHere = 0;
	static pthread_mutex_t mutex1;

	class InnerBlock {
	public:
		static void * unlocker(void * arg)
		{
			int expectedResult = (int)(size_t)arg;
			wasHere++;
			assert(pthread_mutex_unlock(&mutex1) == expectedResult);
			wasHere++;
			return NULL;
		}
	};
	pthread_t t;
	pthread_mutexattr_t ma;
	assert(pthread_mutexattr_init(&ma) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(ma)
	wasHere = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_DEFAULT) == 0);
	assert(pthread_mutex_init(&mutex1, &ma) == 0);
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)(IS_ROBUST ? EPERM : 0)) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(wasHere == 2);

	wasHere = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	assert(pthread_mutex_init(&mutex1, &ma) == 0);
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)(IS_ROBUST ? EPERM : 0)) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(wasHere == 2);

	wasHere = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(pthread_mutex_init(&mutex1, &ma) == 0);
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)EPERM) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(wasHere == 2);

	wasHere = 0;
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	assert(pthread_mutex_init(&mutex1, &ma) == 0);
	assert(pthread_mutex_lock(&mutex1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)EPERM) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_mutex_unlock(&mutex1) == 0);
	assert(wasHere == 2);
	END_MUTEX_STALLED_ROBUST(ma)
	return 0;
}
//
// Confirm the equality/inequality of the various mutex types, and the default not-set value.
//
int PThr4wTest_Mutex5()
{
	static pthread_mutexattr_t mxAttr;
	static int _optimiseFoil; // Prevent optimiser from removing dead or obvious asserts. 
	#define FOIL(x) (_optimiseFoil = x)
	int mxType = -1;
	assert(FOIL(PTHREAD_MUTEX_DEFAULT) == PTHREAD_MUTEX_NORMAL);
	assert(FOIL(PTHREAD_MUTEX_DEFAULT) != PTHREAD_MUTEX_ERRORCHECK);
	assert(FOIL(PTHREAD_MUTEX_DEFAULT) != PTHREAD_MUTEX_RECURSIVE);
	assert(FOIL(PTHREAD_MUTEX_RECURSIVE) != PTHREAD_MUTEX_ERRORCHECK);
	assert(FOIL(PTHREAD_MUTEX_NORMAL) == PTHREAD_MUTEX_FAST_NP);
	assert(FOIL(PTHREAD_MUTEX_RECURSIVE) == PTHREAD_MUTEX_RECURSIVE_NP);
	assert(FOIL(PTHREAD_MUTEX_ERRORCHECK) == PTHREAD_MUTEX_ERRORCHECK_NP);
	assert(pthread_mutexattr_init(&mxAttr) == 0);
	assert(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	assert(mxType == PTHREAD_MUTEX_NORMAL);
	return 0;
	#undef FOIL
}
