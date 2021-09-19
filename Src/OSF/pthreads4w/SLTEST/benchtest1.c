/*
 * benchtest1.c
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

enum {
	OLD_WIN32CS,
	OLD_WIN32MUTEX
};

struct old_mutex_t_ {
	HANDLE mutex;
	CRITICAL_SECTION cs;
};

struct old_mutexattr_t_ {
	int pshared;
};

typedef struct old_mutex_t_ * old_mutex_t;
typedef struct old_mutexattr_t_ * old_mutexattr_t;

// @sobolev (yet defined at implement.h) #define  __PTW32_OBJECT_AUTO_INIT ((void*)-1)
// 
// Dummy use of j, otherwise the loop may be removed by the optimiser
// when doing the overhead timing with an empty loop.
// 
#define TESTSTART { int i, j = 0, k = 0;  __PTW32_FTIME(&currSysTimeStart); for(i = 0; i < ITERATIONS; i++) { j++;
#define TESTSTOP };  __PTW32_FTIME(&currSysTimeStop); if(j + k == i) j++; }

static int old_mutex_use = OLD_WIN32CS;
static BOOL (WINAPI *__ptw32_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
static HINSTANCE __ptw32_h_kernel32;

static void dummy_call(int * a)
{
}

static void interlocked_inc_with_conditionals(int * a)
{
	if(a)
		if(InterlockedIncrement((long*)a) == -1) {
			*a = 0;
		}
}

static void interlocked_dec_with_conditionals(int * a)
{
	if(a)
		if(InterlockedDecrement((long*)a) == -1) {
			*a = 0;
		}
}

static int old_mutex_init(old_mutex_t * mutex, const old_mutexattr_t * attr)
{
	int result = 0;
	old_mutex_t mx;
	if(mutex == NULL) {
		return EINVAL;
	}
	mx = (old_mutex_t)SAlloc::C(1, sizeof(*mx));
	if(mx == NULL) {
		result = ENOMEM;
		goto FAIL0;
	}
	mx->mutex = 0;
	if(attr != NULL && *attr != NULL && (*attr)->pshared == PTHREAD_PROCESS_SHARED) {
		result = ENOSYS;
	}
	else {
		CRITICAL_SECTION cs;
		/*
		 * Load KERNEL32 and try to get address of TryEnterCriticalSection
		 */
		__ptw32_h_kernel32 = LoadLibrary(TEXT("KERNEL32.DLL"));
		__ptw32_try_enter_critical_section = (BOOL(WINAPI *)(LPCRITICAL_SECTION))
#if defined(NEED_UNICODE_CONSTS)
		    GetProcAddress(__ptw32_h_kernel32, (const TCHAR*)TEXT("TryEnterCriticalSection"));
#else
		    GetProcAddress(__ptw32_h_kernel32, (LPCSTR)"TryEnterCriticalSection");
#endif
		if(__ptw32_try_enter_critical_section != NULL) {
			InitializeCriticalSection(&cs);
			if((*__ptw32_try_enter_critical_section)(&cs)) {
				LeaveCriticalSection(&cs);
			}
			else {
				/*
				 * Not really supported (Win98?).
				 */
				__ptw32_try_enter_critical_section = NULL;
			}
			DeleteCriticalSection(&cs);
		}
		if(__ptw32_try_enter_critical_section == NULL) {
			::FreeLibrary(__ptw32_h_kernel32);
			__ptw32_h_kernel32 = 0;
		}
		if(old_mutex_use == OLD_WIN32CS) {
			InitializeCriticalSection(&mx->cs);
		}
		else if(old_mutex_use == OLD_WIN32MUTEX) {
			mx->mutex = CreateMutex(NULL, FALSE, NULL);
			if(mx->mutex == 0) {
				result = EAGAIN;
			}
		}
		else {
			result = EINVAL;
		}
	}
	if(result != 0 && mx != NULL) {
		ZFREE(mx);
	}
FAIL0:
	*mutex = mx;
	return(result);
}

static int old_mutex_lock(old_mutex_t * mutex)
{
	int result = 0;
	old_mutex_t mx;
	if(mutex == NULL || *mutex == NULL) {
		return EINVAL;
	}
	if(*mutex == (old_mutex_t)__PTW32_OBJECT_AUTO_INIT) {
		/*
		 * Don't use initialisers when benchtesting.
		 */
		result = EINVAL;
	}
	mx = *mutex;
	if(result == 0) {
		if(mx->mutex == 0) {
			EnterCriticalSection(&mx->cs);
		}
		else {
			result = (WaitForSingleObject(mx->mutex, INFINITE) == WAIT_OBJECT_0) ? 0 : EINVAL;
		}
	}
	return(result);
}

static int old_mutex_unlock(old_mutex_t * mutex)
{
	int result = 0;
	old_mutex_t mx;
	if(mutex == NULL || *mutex == NULL) {
		return EINVAL;
	}
	mx = *mutex;
	if(mx != (old_mutex_t)__PTW32_OBJECT_AUTO_INIT) {
		if(mx->mutex == 0) {
			LeaveCriticalSection(&mx->cs);
		}
		else {
			result = (ReleaseMutex(mx->mutex) ? 0 : EINVAL);
		}
	}
	else {
		result = EINVAL;
	}
	return(result);
}

static int old_mutex_trylock(old_mutex_t * mutex)
{
	int result = 0;
	old_mutex_t mx;
	if(mutex == NULL || *mutex == NULL) {
		return EINVAL;
	}
	if(*mutex == (old_mutex_t)__PTW32_OBJECT_AUTO_INIT) {
		/*
		 * Don't use initialisers when benchtesting.
		 */
		result = EINVAL;
	}
	mx = *mutex;
	if(result == 0) {
		if(mx->mutex == 0) {
			if(__ptw32_try_enter_critical_section == NULL) {
				result = 0;
			}
			else if((*__ptw32_try_enter_critical_section)(&mx->cs) != TRUE) {
				result = EBUSY;
			}
		}
		else {
			DWORD status = WaitForSingleObject(mx->mutex, 0);
			if(status != WAIT_OBJECT_0) {
				result = ((status == WAIT_TIMEOUT) ? EBUSY : EINVAL);
			}
		}
	}
	return(result);
}

static int old_mutex_destroy(old_mutex_t * mutex)
{
	int result = 0;
	old_mutex_t mx;
	if(mutex == NULL || *mutex == NULL) {
		return EINVAL;
	}
	if(*mutex != (old_mutex_t)__PTW32_OBJECT_AUTO_INIT) {
		mx = *mutex;
		if((result = old_mutex_trylock(&mx)) == 0) {
			*mutex = NULL;
			old_mutex_unlock(&mx);
			if(mx->mutex == 0) {
				DeleteCriticalSection(&mx->cs);
			}
			else {
				result = (CloseHandle(mx->mutex) ? 0 : EINVAL);
			}
			if(result == 0) {
				mx->mutex = 0;
				SAlloc::F(mx);
			}
			else {
				*mutex = mx;
			}
		}
	}
	else {
		result = EINVAL;
	}
	if(__ptw32_try_enter_critical_section != NULL) {
		FreeLibrary(__ptw32_h_kernel32);
		__ptw32_h_kernel32 = 0;
	}
	return(result);
}
//
//
//
#define  __PTW32_MUTEX_TYPES
#define GetDurationMilliSecs(_TStart, _TStop) ((long)((_TStop.time*1000+_TStop.millitm) - (_TStart.time*1000+_TStart.millitm)))
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over lock/unlock for each mutex type.
// 
int PThr4wTest_Benchtest1()
{
	static const long ITERATIONS = 10000000L;
	static pthread_mutex_t mx;
	static pthread_mutexattr_t ma;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static long durationMilliSecs;
	static long overHeadMilliSecs = 0;
	static int two = 2;
	static int one = 1;
	static int zero = 0;
	static int iter;

	class InnerBlock {
	public:
		static void runTest(char * testNameString, int mType)
		{
		#ifdef  __PTW32_MUTEX_TYPES
			assert(pthread_mutexattr_settype(&ma, mType) == 0);
		#endif
			assert(pthread_mutex_init(&mx, &ma) == 0);
			TESTSTART assert((pthread_mutex_lock(&mx), 1) == one); assert((pthread_mutex_unlock(&mx), 2) == two); TESTSTOP assert(pthread_mutex_destroy(&mx) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	int i = 0;
	CRITICAL_SECTION cs;
	old_mutex_t ox;
	pthread_mutexattr_init(&ma);
	printf("=============================================================================\n");
	printf("\nLock plus unlock on an unlocked mutex.\n%ld iterations\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	TESTSTART assert(1 == one); assert(2 == two); TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	TESTSTART assert((dummy_call(&i), 1) == one); assert((dummy_call(&i), 2) == two); TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Dummy call x 2", durationMilliSecs, (float)(durationMilliSecs * 1E3 / ITERATIONS));
	TESTSTART assert((interlocked_inc_with_conditionals(&i), 1) == one);
	assert((interlocked_dec_with_conditionals(&i), 2) == two);
	TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Dummy call -> Interlocked with cond x 2", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	TESTSTART assert((InterlockedIncrement((LPLONG)&i), 1) == (LONG)one);
	assert((InterlockedDecrement((LPLONG)&i), 2) == (LONG)two);
	TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "InterlockedOp x 2", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	InitializeCriticalSection(&cs);
	TESTSTART assert((EnterCriticalSection(&cs), 1) == one);
	assert((LeaveCriticalSection(&cs), 2) == two);
	TESTSTOP DeleteCriticalSection(&cs);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Simple Critical Section", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	old_mutex_use = OLD_WIN32CS;
	assert(old_mutex_init(&ox, NULL) == 0);
	TESTSTART assert(old_mutex_lock(&ox) == zero);
	assert(old_mutex_unlock(&ox) == zero);
	TESTSTOP assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	old_mutex_use = OLD_WIN32MUTEX;
	assert(old_mutex_init(&ox, NULL) == 0);
	TESTSTART assert(old_mutex_lock(&ox) == zero);
	assert(old_mutex_unlock(&ox) == zero);
	TESTSTOP assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Win32 Mutex (W9x)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	printf(".............................................................................\n");
	// 
	// Now we can start the actual tests
	// 
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf(".............................................................................\n");
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT (Robust)", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL (Robust)", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK (Robust)", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE (Robust)", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf("=============================================================================\n");
	/*
	 * End of tests.
	 */
	pthread_mutexattr_destroy(&ma);
	one = i; /* Dummy assignment to avoid 'variable unused' warning */
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Two threads iterate over lock/unlock for each mutex type.
//   The two threads are forced into lock-step using two mutexes,
//   forcing the threads to block on each lock operation. The
//   time measured is therefore the worst case senario.
// 
int PThr4wTest_Benchtest2()
{
	static const long ITERATIONS = 100000L;
	static pthread_mutex_t gate1;
	static pthread_mutex_t gate2;
	static old_mutex_t ox1;
	static old_mutex_t ox2;
	static CRITICAL_SECTION cs1;
	static CRITICAL_SECTION cs2;
	static pthread_mutexattr_t ma;
	static long durationMilliSecs;
	static long overHeadMilliSecs = 0;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static pthread_t worker;
	static int running = 0;

	class InnerBlock {
	public:
		static void * overheadThread(void * arg)
		{
			do {
				sched_yield();
			} while(running);
			return NULL;
		}
		static void * oldThread(void * arg)
		{
			do {
				old_mutex_lock(&ox1);
				old_mutex_lock(&ox2);
				old_mutex_unlock(&ox1);
				sched_yield();
				old_mutex_unlock(&ox2);
			} while(running);
			return NULL;
		}
		static void * workerThread(void * arg)
		{
			do {
				pthread_mutex_lock(&gate1);
				pthread_mutex_lock(&gate2);
				pthread_mutex_unlock(&gate1);
				sched_yield();
				pthread_mutex_unlock(&gate2);
			} while(running);
			return NULL;
		}
		static void * CSThread(void * arg)
		{
			do {
				EnterCriticalSection(&cs1);
				EnterCriticalSection(&cs2);
				LeaveCriticalSection(&cs1);
				sched_yield();
				LeaveCriticalSection(&cs2);
			} while(running);
			return NULL;
		}
		static void runTest(char * testNameString, int mType)
		{
		#ifdef  __PTW32_MUTEX_TYPES
			assert(pthread_mutexattr_settype(&ma, mType) == 0);
		#endif
			assert(pthread_mutex_init(&gate1, &ma) == 0);
			assert(pthread_mutex_init(&gate2, &ma) == 0);
			assert(pthread_mutex_lock(&gate1) == 0);
			assert(pthread_mutex_lock(&gate2) == 0);
			running = 1;
			assert(pthread_create(&worker, NULL, workerThread, NULL) == 0);
			TESTSTART
				 pthread_mutex_unlock(&gate1);
			sched_yield();
			pthread_mutex_unlock(&gate2);
			pthread_mutex_lock(&gate1);
			pthread_mutex_lock(&gate2);
			TESTSTOP
				running = 0;
			assert(pthread_mutex_unlock(&gate2) == 0);
			assert(pthread_mutex_unlock(&gate1) == 0);
			assert(pthread_join(worker, NULL) == 0);
			assert(pthread_mutex_destroy(&gate2) == 0);
			assert(pthread_mutex_destroy(&gate1) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4 /* Four locks/unlocks per iteration */);
		}
	};
	assert(pthread_mutexattr_init(&ma) == 0);
	printf("=============================================================================\n");
	printf("\nLock plus unlock on a locked mutex.\n");
	printf("%ld iterations, four locks/unlocks per iteration.\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	/*
	 * Time the loop overhead so we can subtract it from the actual test times.
	 */
	running = 1;
	assert(pthread_create(&worker, NULL, InnerBlock::overheadThread, NULL) == 0);
	TESTSTART sched_yield();
	sched_yield();
	TESTSTOP
	    running = 0;
	assert(pthread_join(worker, NULL) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;

	InitializeCriticalSection(&cs1);
	InitializeCriticalSection(&cs2);
	EnterCriticalSection(&cs1);
	EnterCriticalSection(&cs2);
	running = 1;
	assert(pthread_create(&worker, NULL, InnerBlock::CSThread, NULL) == 0);
	TESTSTART LeaveCriticalSection(&cs1);
	sched_yield();
	LeaveCriticalSection(&cs2);
	EnterCriticalSection(&cs1);
	EnterCriticalSection(&cs2);
	TESTSTOP
	    running = 0;
	LeaveCriticalSection(&cs2);
	LeaveCriticalSection(&cs1);
	assert(pthread_join(worker, NULL) == 0);
	DeleteCriticalSection(&cs2);
	DeleteCriticalSection(&cs1);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Simple Critical Section", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);

	old_mutex_use = OLD_WIN32CS;
	assert(old_mutex_init(&ox1, NULL) == 0);
	assert(old_mutex_init(&ox2, NULL) == 0);
	assert(old_mutex_lock(&ox1) == 0);
	assert(old_mutex_lock(&ox2) == 0);
	running = 1;
	assert(pthread_create(&worker, NULL, InnerBlock::oldThread, NULL) == 0);
	TESTSTART
		 old_mutex_unlock(&ox1);
	sched_yield();
	old_mutex_unlock(&ox2);
	old_mutex_lock(&ox1);
	old_mutex_lock(&ox2);
	TESTSTOP
	    running = 0;
	assert(old_mutex_unlock(&ox1) == 0);
	assert(old_mutex_unlock(&ox2) == 0);
	assert(pthread_join(worker, NULL) == 0);
	assert(old_mutex_destroy(&ox2) == 0);
	assert(old_mutex_destroy(&ox1) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);

	old_mutex_use = OLD_WIN32MUTEX;
	assert(old_mutex_init(&ox1, NULL) == 0);
	assert(old_mutex_init(&ox2, NULL) == 0);
	assert(old_mutex_lock(&ox1) == 0);
	assert(old_mutex_lock(&ox2) == 0);
	running = 1;
	assert(pthread_create(&worker, NULL, InnerBlock::oldThread, NULL) == 0);
	TESTSTART
		 old_mutex_unlock(&ox1);
	sched_yield();
	old_mutex_unlock(&ox2);
	old_mutex_lock(&ox1);
	old_mutex_lock(&ox2);
	TESTSTOP
	    running = 0;
	assert(old_mutex_unlock(&ox1) == 0);
	assert(old_mutex_unlock(&ox2) == 0);
	assert(pthread_join(worker, NULL) == 0);
	assert(old_mutex_destroy(&ox2) == 0);
	assert(old_mutex_destroy(&ox1) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Win32 Mutex (W9x)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);
	printf(".............................................................................\n");
	/*
	 * Now we can start the actual tests
	 */
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf(".............................................................................\n");
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT (Robust)", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL (Robust)", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK (Robust)", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE (Robust)", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf("=============================================================================\n");
	/*
	 * End of tests.
	 */
	pthread_mutexattr_destroy(&ma);
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over a trylock on a locked mutex for each mutex type.
// 
int PThr4wTest_Benchtest3()
{
	static const long ITERATIONS = 10000000L;
	static pthread_mutex_t mx;
	static old_mutex_t ox;
	static pthread_mutexattr_t ma;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static long durationMilliSecs;
	static long overHeadMilliSecs = 0;

	class InnerBlock {
	public:
		static void * trylockThread(void * arg)
		{
			TESTSTART pthread_mutex_trylock(&mx); TESTSTOP
			return NULL;
		}
		static void * oldTrylockThread(void * arg)
		{
			TESTSTART old_mutex_trylock(&ox); TESTSTOP
			return NULL;
		}
		static void runTest(char * testNameString, int mType)
		{
			pthread_t t;
		#ifdef  __PTW32_MUTEX_TYPES
			pthread_mutexattr_settype(&ma, mType);
		#endif
			assert(pthread_mutex_init(&mx, &ma) == 0);
			assert(pthread_mutex_lock(&mx) == 0);
			assert(pthread_create(&t, NULL, trylockThread, 0) == 0);
			assert(pthread_join(t, NULL) == 0);
			assert(pthread_mutex_unlock(&mx) == 0);
			assert(pthread_mutex_destroy(&mx) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	pthread_t t;
	assert(pthread_mutexattr_init(&ma) == 0);
	printf("=============================================================================\n");
	printf("\nTrylock on a locked mutex.\n");
	printf("%ld iterations.\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	TESTSTART TESTSTOP durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	old_mutex_use = OLD_WIN32CS;
	assert(old_mutex_init(&ox, NULL) == 0);
	assert(old_mutex_lock(&ox) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::oldTrylockThread, 0) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(old_mutex_unlock(&ox) == 0);
	assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	old_mutex_use = OLD_WIN32MUTEX;
	assert(old_mutex_init(&ox, NULL) == 0);
	assert(old_mutex_lock(&ox) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::oldTrylockThread, 0) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(old_mutex_unlock(&ox) == 0);
	assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Win32 Mutex (W9x)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	printf(".............................................................................\n");
	/*
	 * Now we can start the actual tests
	 */
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf(".............................................................................\n");
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT (Robust)", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL (Robust)", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK (Robust)", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE (Robust)", PTHREAD_MUTEX_RECURSIVE);
#else
	runTest("Non-blocking lock", 0);
#endif
	printf("=============================================================================\n");
	/*
	 * End of tests.
	 */
	pthread_mutexattr_destroy(&ma);
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over trylock/unlock for each mutex type.
// 
int PThr4wTest_Benchtest4()
{
	static const long ITERATIONS = 10000000L;
	static pthread_mutex_t mx;
	static old_mutex_t ox;
	static pthread_mutexattr_t ma;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static long durationMilliSecs;
	static long overHeadMilliSecs = 0;

	class InnerBlock {
	public:
		static void oldRunTest(char * testNameString, int mType)
		{
		}
		static void runTest(char * testNameString, int mType)
		{
		#ifdef  __PTW32_MUTEX_TYPES
			pthread_mutexattr_settype(&ma, mType);
		#endif
			pthread_mutex_init(&mx, &ma);
			TESTSTART pthread_mutex_trylock(&mx); pthread_mutex_unlock(&mx); TESTSTOP pthread_mutex_destroy(&mx);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	pthread_mutexattr_init(&ma);
	printf("=============================================================================\n");
	printf("Trylock plus unlock on an unlocked mutex.\n");
	printf("%ld iterations.\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	TESTSTART TESTSTOP durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	old_mutex_use = OLD_WIN32CS;
	assert(old_mutex_init(&ox, NULL) == 0);
	TESTSTART old_mutex_trylock(&ox); old_mutex_unlock(&ox); TESTSTOP assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	old_mutex_use = OLD_WIN32MUTEX;
	assert(old_mutex_init(&ox, NULL) == 0);
	TESTSTART old_mutex_trylock(&ox); old_mutex_unlock(&ox); TESTSTOP assert(old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Win32 Mutex (W9x)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	printf(".............................................................................\n");
	// 
	// Now we can start the actual tests
	// 
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE", PTHREAD_MUTEX_RECURSIVE);
#else
	InnerBlock::runTest("Non-blocking lock", 0);
#endif
	printf(".............................................................................\n");
	pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST);
#ifdef  __PTW32_MUTEX_TYPES
	InnerBlock::runTest("PTHREAD_MUTEX_DEFAULT (Robust)", PTHREAD_MUTEX_DEFAULT);
	InnerBlock::runTest("PTHREAD_MUTEX_NORMAL (Robust)", PTHREAD_MUTEX_NORMAL);
	InnerBlock::runTest("PTHREAD_MUTEX_ERRORCHECK (Robust)", PTHREAD_MUTEX_ERRORCHECK);
	InnerBlock::runTest("PTHREAD_MUTEX_RECURSIVE (Robust)", PTHREAD_MUTEX_RECURSIVE);
#else
	InnerBlock::runTest("Non-blocking lock", 0);
#endif
	printf("=============================================================================\n");
	// 
	// End of tests.
	// 
	pthread_mutexattr_destroy(&ma);
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Semaphore
//   Single thread iteration over post/wait for a semaphore.
// 
int PThr4wTest_Benchtest5()
{
	static const long ITERATIONS = 1000000L;
	static sem_t sema;
	static HANDLE w32sema;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static long durationMilliSecs;
	static long overHeadMilliSecs = 0;
	static int one = 1;
	static int zero = 0;

	class InnerBlock {
	public:
		static void reportTest(char * testNameString)
		{
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	printf("=============================================================================\n");
	printf("\nOperations on a semaphore.\n%ld iterations\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	TESTSTART assert(1 == one); TESTSTOP
    durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	// 
	// Now we can start the actual tests
	// 
	assert((w32sema = CreateSemaphore(NULL, (long)0, (long)ITERATIONS, NULL)) != 0);
	TESTSTART assert((ReleaseSemaphore(w32sema, 1, NULL), 1) == one);
	TESTSTOP assert(CloseHandle(w32sema) != 0);
	InnerBlock::reportTest("W32 Post with no waiters");
	assert((w32sema = CreateSemaphore(NULL, (long)ITERATIONS, (long)ITERATIONS, NULL)) != 0);
	TESTSTART assert((WaitForSingleObject(w32sema, INFINITE), 1) == one);
	TESTSTOP assert(CloseHandle(w32sema) != 0);
	InnerBlock::reportTest("W32 Wait without blocking");
	assert(sem_init(&sema, 0, 0) == 0);
	TESTSTART assert((sem_post(&sema), 1) == one);
	TESTSTOP assert(sem_destroy(&sema) == 0);
	InnerBlock::reportTest("POSIX Post with no waiters");
	assert(sem_init(&sema, 0, ITERATIONS) == 0);
	TESTSTART assert((sem_wait(&sema), 1) == one);
	TESTSTOP assert(sem_destroy(&sema) == 0);
	InnerBlock::reportTest("POSIX Wait without blocking");
	printf("=============================================================================\n");
	// 
	// End of tests.
	// 
	return 0;
}
