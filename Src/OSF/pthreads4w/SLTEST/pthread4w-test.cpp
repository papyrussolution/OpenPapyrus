// PTHREAD4W-TEST.CPP
//
#include <sl_pthreads4w.h>
#pragma hdrstop
// 
// Some tests sneak a peek at ../implement.h
// This is used inside ../implement.h to control
// what these test apps see and don't see.
// 
#define  __PTW32_TEST_SNEAK_PEEK

#include <sys/timeb.h>
//#include <errno.h> // FIXME: May not be available on all platforms.

#define  __PTW32_THREAD_NULL_ID {NULL,0}
/*
 * Some non-thread POSIX API substitutes
 */
#if !defined(__MINGW64_VERSION_MAJOR)
	#define rand_r( _seed ) ( _seed == _seed? rand() : rand() )
#endif
#if defined(__MINGW32__)
	#include <stdint.h>
#elif defined(__BORLANDC__)
	#define int64_t ULONGLONG
#else
	#define int64_t _int64
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1400
	#define  __PTW32_FTIME(x) _ftime64_s(x)
	#define  __PTW32_STRUCT_TIMEB struct __timeb64
#elif ( defined(_MSC_VER) && _MSC_VER >= 1300 ) || ( defined(__MINGW32__) && __MSVCRT_VERSION__ >= 0x0601 )
	#define  __PTW32_FTIME(x) _ftime64(x)
	#define  __PTW32_STRUCT_TIMEB struct __timeb64
#else
	#define  __PTW32_FTIME(x) _ftime(x)
	#define  __PTW32_STRUCT_TIMEB struct _timeb
#endif

struct cvthing_t {
	pthread_cond_t notbusy;
	pthread_mutex_t lock;
	int shared;
};

struct sharedInt_t {
	int i;
	CRITICAL_SECTION cs;
};

struct bag_t {
	bag_t() : threadnum(0), started(0), finished(0), count(0), oncenum(0), myPrio(0), w32Thread(0) //, self(0)
	{
	}
	int threadnum;
	int started;
	int finished;
	// Add more per-thread state variables here
	int count;
	pthread_t self;
	int oncenum;
	int myPrio;
	HANDLE w32Thread;
};

extern const char * PThr4wErrorString[];
// 
// The Mingw32 PTHR4W_TEST_ASSERT macro calls the CRTDLL _assert function
// which pops up a dialog. We want to run in batch mode so
// we define our own PTHR4W_TEST_ASSERT macro.
// 
//#ifdef assert_Removed
	//#undef assert_Removed
//#endif
#ifndef ASSERT_TRACE
	#define ASSERT_TRACE 0
#else
	#undef ASSERT_TRACE
	#define ASSERT_TRACE 1
#endif

static int AssertOutput(bool success, const char * pExpr, const char * pFile__, int line__)
{
	if(success) {
		if(ASSERT_TRACE) {
			slfprintf_stderr("Assertion succeeded: (%s), file %s, line %d\n", pExpr, pFile__, line__);
			fflush(stderr);
		}
		return 0;
	}
	else {
		slfprintf_stderr("Assertion failed: (%s), file %s, line %d\n", pExpr, pFile__, line__);
		exit(1);
		return 0;
	}
}

#define PTHR4W_TEST_ASSERT(e) AssertOutput(e, #e, __FILE__, __LINE__)
/*
#define PTHR4W_TEST_ASSERT(e) ((e) ? ((ASSERT_TRACE) ? slfprintf_stderr("Assertion succeeded: (%s), file %s, line %d\n", \
	#e, __FILE__, (int) __LINE__), fflush(stderr) : 0) : \
	(slfprintf_stderr("Assertion failed: (%s), file %s, line %d\n", #e, __FILE__, (int) __LINE__), exit(1), 0))
*/

//extern int assertE;
static int assertE;
//extern void Implement_AssertE(bool condition, int iE, const char * pE, const char * pO, const char * pR, const char * pFile, const int line);

#define assert_e(e, o, r) Implement_AssertE(e o r, e, #e, #o, #r, __FILE__, __LINE__) 

/*#define assert_e(e, o, r) (((assertE = e) o (r)) ? ((ASSERT_TRACE) ? slfprintf_stderr("Assertion succeeded: (%s), file %s, line %d\n", \
	#e, __FILE__, (int)__LINE__), fflush(stderr) : 0) : \
	(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) ? \
	(slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %s\n", \
	#e,#o,#r, __FILE__, (int) __LINE__, error_string[assertE]), exit(1), 0) :\
	(slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %d\n", #e,#o,#r, __FILE__, (int) __LINE__, assertE), exit(1), 0))*/

#define BEGIN_MUTEX_STALLED_ROBUST(mxAttr) \
  for(;;) { \
      static int _i=0; \
      static int _robust; \
      pthread_mutexattr_getrobust(&(mxAttr), &_robust);

#define END_MUTEX_STALLED_ROBUST(mxAttr) \
      printf("Pass %s\n", _robust==PTHREAD_MUTEX_ROBUST?"Robust":"Non-robust"); \
      if(++_i > 1) \
        break; \
      else { \
          pthread_mutexattr_t *pma, *pmaEnd; \
          for(pma = &(mxAttr), pmaEnd = pma + sizeof(mxAttr)/sizeof(pthread_mutexattr_t); pma < pmaEnd; pthread_mutexattr_setrobust(pma++, PTHREAD_MUTEX_ROBUST)); \
        } \
    }

#define IS_ROBUST (_robust==PTHREAD_MUTEX_ROBUST)
#define GetDurationMilliSecs(_TStart, _TStop) ((long)((_TStop.time*1000+_TStop.millitm) - (_TStart.time*1000+_TStart.millitm)))
//
#if defined(_MSC_VER)
	#include <eh.h>
#else
	#if !defined(_MSC_VER) && defined(__cplusplus)
		#include <exception>
	#endif
	#if defined(__GNUC__) && __GNUC__ < 3
		#include <new.h>
	#else
		#include <new>
		using std::set_terminate;
	#endif
#endif
//
static const int PTW32TEST_THREAD_INIT_PRIO = 0;
static const int PTW32TEST_MAXPRIORITIES = 512;
#define SEMAPHORE_MAX_COUNT 100

//#define GetDurationMilliSecs(_TStart, _TStop) ((_TStop.time*1000+_TStop.millitm) - (_TStart.time*1000+_TStart.millitm))

const char * PThr4wErrorString[] = {
	"ZERO_or_EOK",
	"EPERM",
	"ENOFILE_or_ENOENT",
	"ESRCH",
	"EINTR",
	"EIO",
	"ENXIO",
	"E2BIG",
	"ENOEXEC",
	"EBADF",
	"ECHILD",
	"EAGAIN",
	"ENOMEM",
	"EACCES",
	"EFAULT",
	"UNKNOWN_15",
	"EBUSY",
	"EEXIST",
	"EXDEV",
	"ENODEV",
	"ENOTDIR",
	"EISDIR",
	"EINVAL",
	"ENFILE",
	"EMFILE",
	"ENOTTY",
	"UNKNOWN_26",
	"EFBIG",
	"ENOSPC",
	"ESPIPE",
	"EROFS",
	"EMLINK",
	"EPIPE",
	"EDOM",
	"ERANGE",
	"UNKNOWN_35",
	"EDEADLOCK_or_EDEADLK",
	"UNKNOWN_37",
	"ENAMETOOLONG",
	"ENOLCK",
	"ENOSYS",
	"ENOTEMPTY",
	#if  __PTW32_VERSION_MAJOR > 2
	"EILSEQ",
	#else
	"EILSEQ_or_EOWNERDEAD",
	"ENOTRECOVERABLE"
	#endif
};

static void Implement_AssertE(bool condition, int iE, const char * pE, const char * pO, const char * pR, const char * pFile, const int line) 
{
	assertE = iE;
	if(condition) {
		if(ASSERT_TRACE) {
			slfprintf_stderr("Assertion succeeded: (%s), file %s, line %d\n", pE, pFile, line);
			fflush(stderr);
		}
	}
	else {
		if(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) {
			slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %s\n", pE, pO, pR, pFile, line, PThr4wErrorString[assertE]);
		}
		else {
			slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %d\n", pE, pO, pR, pFile, line, assertE);
		}
		exit(1);
	}
	//(((assertE = e) o (r)) ? ((ASSERT_TRACE) ? slfprintf_stderr("Assertion succeeded: (%s), file %s, line %d\n", \
	//#e, __FILE__, (int)__LINE__), fflush(stderr) : 0) : \
	//(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) ? \
	//(slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %s\n", #e,#o,#r, __FILE__, (int) __LINE__, error_string[assertE]), exit(1), 0) :\
	//(slfprintf_stderr("Assertion failed: (%s %s %s), file %s, line %d, error %d\n", #e,#o,#r, __FILE__, (int) __LINE__, assertE), exit(1), 0))
}

static int PThr4wTest_Sizes()
{
	class InnerBlock {
	public:
		static void PrintTaggedSize(const char * pTitle, size_t sz)
		{
			if(pTitle == 0)
				printf("-------------------------------\n");
			else if(sz == 0)
				printf("%s\n", pTitle);
			else
				printf("%30s %4d\n", pTitle, static_cast<int>(sz));
		}
	};
	InnerBlock::PrintTaggedSize("Sizes of pthreads-win32 structs", 0);
	InnerBlock::PrintTaggedSize(0, 0);
	InnerBlock::PrintTaggedSize("pthread_t", sizeof(pthread_t));
	InnerBlock::PrintTaggedSize("__ptw32_thread_t", sizeof(__ptw32_thread_t));
	InnerBlock::PrintTaggedSize("pthread_attr_t_", sizeof(struct pthread_attr_t_));
	InnerBlock::PrintTaggedSize("sem_t_", sizeof(struct sem_t_));
	InnerBlock::PrintTaggedSize("pthread_mutex_t_", sizeof(struct pthread_mutex_t_));
	InnerBlock::PrintTaggedSize("pthread_mutexattr_t_", sizeof(struct pthread_mutexattr_t_));
	InnerBlock::PrintTaggedSize("pthread_spinlock_t_", sizeof(struct pthread_spinlock_t_));
	InnerBlock::PrintTaggedSize("pthread_barrier_t_", sizeof(struct pthread_barrier_t_));
	InnerBlock::PrintTaggedSize("pthread_barrierattr_t_", sizeof(struct pthread_barrierattr_t_));
	InnerBlock::PrintTaggedSize("pthread_key_t_", sizeof(struct pthread_key_t_));
	InnerBlock::PrintTaggedSize("pthread_cond_t_", sizeof(struct pthread_cond_t_));
	InnerBlock::PrintTaggedSize("pthread_condattr_t_", sizeof(struct pthread_condattr_t_));
	InnerBlock::PrintTaggedSize("pthread_rwlock_t_", sizeof(struct pthread_rwlock_t_));
	InnerBlock::PrintTaggedSize("pthread_rwlockattr_t_", sizeof(struct pthread_rwlockattr_t_));
	InnerBlock::PrintTaggedSize("pthread_once_t_", sizeof(struct pthread_once_t_));
	InnerBlock::PrintTaggedSize("__ptw32_cleanup_t", sizeof(struct __ptw32_cleanup_t));
	InnerBlock::PrintTaggedSize("__ptw32_mcs_node_t_", sizeof(struct __ptw32_mcs_node_t_));
	InnerBlock::PrintTaggedSize("sched_param", sizeof(struct sched_param));
	InnerBlock::PrintTaggedSize(0, 0);
	return 0;
}
//
// See if we have the TryEnterCriticalSection function.
// Does not use any part of pthreads.
// 
static int PThr4wTest_TryEnterCs1()
{
	// Function pointer to TryEnterCriticalSection if it exists - otherwise NULL
	static BOOL (WINAPI *_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
	static HINSTANCE _h_kernel32; // Handle to kernel32.dll

	CRITICAL_SECTION cs;
	SetLastError(0);
	printf("Last Error [main enter] %ld\n", (long)GetLastError());
	// 
	// Load KERNEL32 and try to get address of TryEnterCriticalSection
	// 
	_h_kernel32 = LoadLibrary(_T("KERNEL32.DLL"));
	_try_enter_critical_section = (BOOL (WINAPI *)(LPCRITICAL_SECTION))GetProcAddress(_h_kernel32, (LPCSTR)"TryEnterCriticalSection");
	if(_try_enter_critical_section != NULL) {
		InitializeCriticalSection(&cs);
		SetLastError(0);
		if((*_try_enter_critical_section)(&cs) != 0) {
			LeaveCriticalSection(&cs);
		}
		else {
			printf("Last Error [try enter] %ld\n", (long)GetLastError());
			_try_enter_critical_section = NULL;
		}
		DeleteCriticalSection(&cs);
	}
	FreeLibrary(_h_kernel32);
	printf("This system %s TryEnterCriticalSection.\n", (_try_enter_critical_section == NULL) ? "DOES NOT SUPPORT" : "SUPPORTS");
	printf("POSIX Mutexes will be based on Win32 %s.\n", (_try_enter_critical_section == NULL) ? "Mutexes" : "Critical Sections");
	return 0;
}
// 
// See if we have the TryEnterCriticalSection function.
// Does not use any part of pthreads.
// 
static int PThr4wTest_TryEnterCs2()
{
	// Function pointer to TryEnterCriticalSection if it exists - otherwise NULL
	static BOOL (WINAPI *_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
	// @sobolev LPCRITICAL_SECTION lpcs = NULL;
	SetLastError(0);
	printf("Last Error [main enter] %ld\n", (long)GetLastError());
	{
		CRITICAL_SECTION cs; // @sobolev
		InitializeCriticalSection(&cs); // @sobolev
		// Load KERNEL32 and try to get address of TryEnterCriticalSection
		HINSTANCE _h_kernel32 = LoadLibrary(_T("KERNEL32.DLL"));
		_try_enter_critical_section = (BOOL (WINAPI *)(LPCRITICAL_SECTION))GetProcAddress(_h_kernel32, (LPCSTR)"TryEnterCriticalSection");
		if(_try_enter_critical_section != NULL) {
			SetLastError(0);
			(*_try_enter_critical_section)(&cs); // @sobolev lpcs-->&cs
			printf("Last Error [try enter] %ld\n", (long)GetLastError());
		}
		FreeLibrary(_h_kernel32);
		DeleteCriticalSection(&cs); // @sobolev
	}
	printf("This system %s TryEnterCriticalSection.\n", (_try_enter_critical_section == NULL) ? "DOES NOT SUPPORT" : "SUPPORTS");
	printf("POSIX Mutexes will be based on Win32 %s.\n", (_try_enter_critical_section == NULL) ? "Mutexes" : "Critical Sections");
	return 0;
}
//
// Test for pthread_equal
// Depends on functions: pthread_self().
//
static int PThr4wTest_Equal0()
{
	pthread_t t1 = pthread_self();
	PTHR4W_TEST_ASSERT(pthread_equal(t1, pthread_self()) != 0);
	return 0; // Success
}
//
// Test for pthread_equal
// Depends on functions: pthread_create().
//
static int PThr4wTest_Equal1()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			Sleep(2000);
			return 0;
		}
	};
	pthread_t t1, t2;
	PTHR4W_TEST_ASSERT(pthread_create(&t1, NULL, InnerBlock::ThreadFunc, (void*)1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t2, NULL, InnerBlock::ThreadFunc, (void*)2) == 0);
	PTHR4W_TEST_ASSERT(pthread_equal(t1, t2) == 0);
	PTHR4W_TEST_ASSERT(pthread_equal(t1, t1) != 0);
	// This is a hack. We don't want to rely on pthread_join yet if we can help it. 
	Sleep(4000);
	return 0; // Success
}
// 
// Test Synopsis:
// - pthread_kill() does not support non zero signals..
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Kill1()
{
	PTHR4W_TEST_ASSERT(pthread_kill(pthread_self(), 1) == EINVAL);
	return 0;
}
//
// Descr: Test some basic assertions about the number of threads at runtime.
//
static int PThr4wTest_Count1()
{
	static const int NUMTHREADS = 30;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	static pthread_t threads[NUMTHREADS];
	static uint numThreads = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			pthread_mutex_lock(&lock);
			numThreads++;
			pthread_mutex_unlock(&lock);
			Sleep(1000);
			return 0;
		}
	};
	size_t i;
	//
	// Spawn NUMTHREADS threads. Each thread should increment the
	// numThreads variable, sleep for one second.
	//
	for(i = 0; i < SIZEOFARRAY(threads); i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&threads[i], NULL, InnerBlock::ThreadFunc, 0) == 0);
	}
	// Wait for all the threads to exit.
	for(i = 0; i < SIZEOFARRAY(threads); i++) {
		PTHR4W_TEST_ASSERT(pthread_join(threads[i], NULL) == 0);
	}
	// Check the number of threads created.
	PTHR4W_TEST_ASSERT((int)numThreads == SIZEOFARRAY(threads));
	return 0; // Success
}
//
// Depends on API functions: pthread_delay_np
//
static int PThr4wTest_Delay1()
{
	struct timespec interval = {1L, 500000000L};
	PTHR4W_TEST_ASSERT(pthread_delay_np(&interval) == 0);
	return 0;
}
//
// Depends on API functions: pthread_delay_np
//
static int PThr4wTest_Delay2()
{
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			struct timespec interval = {5, 500000000L};
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mx) == 0);
		#ifdef _MSC_VER
			#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, &mx);
			PTHR4W_TEST_ASSERT(pthread_delay_np(&interval) == 0);
			pthread_cleanup_pop(1);
		#ifdef _MSC_VER
			#pragma inline_depth()
		#endif
			return (void*)(size_t)1;
		}
	};
	pthread_t t;
	void * result = (void*)0;
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mx) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_cancel(t) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mx) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT(result == (void*)PTHREAD_CANCELED);
	return 0;
}
//
// Depends on API functions: pthread_create(), pthread_detach(), pthread_exit().
//
static int PThr4wTest_Detach1()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int i = reinterpret_cast<int>(arg);
			Sleep(i * 10);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	const int NUMTHREADS = 100;
	pthread_t id[NUMTHREADS];
	int i;
	// Create a few threads and then exit. 
	for(i = 0; i < SIZEOFARRAY(id); i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&id[i], NULL, InnerBlock::ThreadFunc, reinterpret_cast<void *>(i)) == 0);
	}
	// Some threads will finish before they are detached, some after. 
	Sleep(NUMTHREADS/2 * 10 + 50);
	for(i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_detach(id[i]) == 0);
	}
	Sleep(NUMTHREADS * 10 + 100);
	// 
	// Check that all threads are now invalid.
	// This relies on unique thread IDs - e.g. works with
	// pthreads-w32 or Solaris, but may not work for Linux, BSD etc.
	// 
	for(i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_kill(id[i], 0) == ESRCH);
	}
	return 0; // Success
}
// 
// Test for pthread_self().
// Depends on API functions: pthread_self()
// Implicitly depends on: pthread_getspecific(), pthread_setspecific()
// 
static int PThr4wTest_Self1()
{
	// 
	// This should always succeed unless the system has no resources (memory) left.
	// 
	pthread_t self;
#if defined (__PTW32_STATIC_LIB) && !(defined(_MSC_VER) || defined(__MINGW32__))
	pthread_win32_process_attach_np();
#endif
	self = pthread_self();
	PTHR4W_TEST_ASSERT(self.p != NULL);
#if defined (__PTW32_STATIC_LIB) && !(defined(_MSC_VER) || defined(__MINGW32__))
	pthread_win32_process_detach_np();
#endif
	return 0;
}
// 
// Test for pthread_self().
// Depends on API functions: pthread_create(), pthread_self()
// Implicitly depends on: pthread_getspecific(), pthread_setspecific()
// 
static int PThr4wTest_Self2()
{
	static pthread_t me;
	class InnerBlock {
	public:
		static void * entry(void * arg)
		{
			me = pthread_self();
			return arg;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::entry, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_equal(t, me) != 0);
	return 0; // Success
}
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
static int PThr4wTest_Valid1()
{
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	void * result = NULL;
	washere = 0;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	sched_yield();
	PTHR4W_TEST_ASSERT(pthread_kill(t, 0) == ESRCH);
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
static int PThr4wTest_Valid2()
{
	pthread_t NullThread =  __PTW32_THREAD_NULL_ID;
	PTHR4W_TEST_ASSERT(pthread_kill(NullThread, 0) == ESRCH);
	return 0;
}
// 
// Test Synopsis:
// - that unique thread sequence numbers are generated.
// Environment:
// - This test is implementation specific
// because it uses knowledge of internals that should be opaque to an application.
// Output:
// - File name, Line number, and failed expression on failure.
// - analysis output on success.
// Pass Criteria:
// - unique sequence numbers are generated for every new thread.
// 
static int PThr4wTest_Sequence1()
{
	static const int NUMTHREADS = PTHREAD_THREADS_MAX;
	static long done = 0;
	// 
	// seqmap should have 1 in every element except [0]
	// Thread sequence numbers start at 1 and we will also
	// include this main thread so we need NUMTHREADS+2 elements.
	// 
	static UINT64 seqmap[NUMTHREADS+2];

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			sched_yield();
			seqmap[(int)pthread_getunique_np(pthread_self())] = 1;
			InterlockedIncrement(&done);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	pthread_attr_t attr;
	int i;
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
	memzero(seqmap, sizeof(seqmap));
	for(i = 0; i < NUMTHREADS; i++) {
		if(NUMTHREADS/2 == i) {
			// Include this main thread, which will be an implicit pthread_t 
			seqmap[(int)pthread_getunique_np(pthread_self())] = 1;
		}
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], &attr, InnerBlock::ThreadFunc, NULL) == 0);
	}
	while(NUMTHREADS > InterlockedExchangeAdd((LPLONG)&done, 0L))
		Sleep(100);
	Sleep(100);
	PTHR4W_TEST_ASSERT(seqmap[0] == 0);
	for(i = 1; i < NUMTHREADS+2; i++) {
		PTHR4W_TEST_ASSERT(seqmap[i] == 1);
	}
	return 0;
}
//
// Descr: Create a thread and check that it ran.
//
static int PThr4wTest_Create1()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	// A dirty hack, but we cannot rely on pthread_join in this primitive test. 
	Sleep(2000);
	PTHR4W_TEST_ASSERT(washere == 1);
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
static int PThr4wTest_Create2()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
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
		PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == 0);
		PTHR4W_TEST_ASSERT(washere == 1);
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
static int PThr4wTest_Create3()
{
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
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
		PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, (void*)(size_t)1) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == 0);
		PTHR4W_TEST_ASSERT(washere == 1);
	}
	return 0;
}
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
static int PThr4wTest_Errno0()
{
	int err = 0;
	errno = 0;
	PTHR4W_TEST_ASSERT(errno == 0);
	PTHR4W_TEST_ASSERT(0 != sem_destroy(NULL));
	err =
#if defined(PTW32_USES_SEPARATE_CRT)
	    GetLastError();
#else
	    errno;
#endif
	PTHR4W_TEST_ASSERT(err != 0);
	PTHR4W_TEST_ASSERT(err == EINVAL);
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
static int PThr4wTest_Errno1()
{
	const int NUMTHREADS = 3; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static pthread_mutex_t stop_here = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			errno = bag->threadnum;
			Sleep(1000);
			pthread_mutex_lock(&stop_here);
			PTHR4W_TEST_ASSERT(errno == bag->threadnum);
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
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
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
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print ouput on failure.
	for(i = 1; i <= NUMTHREADS; i++) {
		/* ... */
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test context switching method.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - pthread_create, pthread_exit
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Context1()
{
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			washere = 1;
			Sleep(1000);
			return 0;
		}
		static void anotherEnding()
		{
			// Switched context
			washere++;
			pthread_exit(0);
		}
	};
	pthread_t t;
	HANDLE hThread;
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	hThread = ((__ptw32_thread_t*)t.p)->threadH;
	Sleep(500);
	SuspendThread(hThread);
	if(WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT) {
		/*
		 * Ok, thread did not exit before we got to it.
		 */
		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, &context);
		__PTW32_PROGCTR(context) = (DWORD_PTR)InnerBlock::anotherEnding;
		SetThreadContext(hThread, &context);
		ResumeThread(hThread);
	}
	else {
		printf("Exited early\n");
		fflush(stdout);
	}
	Sleep(1000);
	PTHR4W_TEST_ASSERT(washere == 2);
	return 0;
}

#ifdef _MSC_VER
	#pragma inline_depth(0)
	#pragma optimize("g", off)
#endif
#ifdef _MSC_VER
	#pragma inline_depth()
	#pragma optimize("", on)
#endif
// 
// Test Synopsis: Test context switching method.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - pthread_create, pthread_exit
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Context2()
{
	static int washere = 0;
	static volatile size_t tree_counter = 0;

	class InnerBlock {
		static size_t tree(size_t depth)
		{
			if(!depth--)
				return tree_counter++;
			return tree(depth) + tree(depth);
		}
	public:
		static void * ThreadFunc(void * arg)
		{
			washere = 1;
			return (void*)tree(64);
		}
		static void anotherEnding()
		{
			// Switched context
			washere++;
			pthread_exit(0);
		}
	};
	pthread_t t;
	HANDLE hThread;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	hThread = ((__ptw32_thread_t*)t.p)->threadH;
	Sleep(500);
	SuspendThread(hThread);
	if(WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT) {
		/*
		 * Ok, thread did not exit before we got to it.
		 */
		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, &context);
		__PTW32_PROGCTR(context) = (DWORD_PTR)InnerBlock::anotherEnding;
		SetThreadContext(hThread, &context);
		ResumeThread(hThread);
	}
	else {
		printf("Exited early\n");
		fflush(stdout);
	}
	Sleep(1000);
	PTHR4W_TEST_ASSERT(washere == 2);
	return 0;
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create(), pthread_exit().
//
static int PThr4wTest_Join0()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			Sleep(2000);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	pthread_t id;
	void* result = (void*)0;
	// Create a single thread and wait for it to exit. 
	PTHR4W_TEST_ASSERT(pthread_create(&id, NULL, InnerBlock::ThreadFunc, (void*)123) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(id, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 123);
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create(), pthread_join(), pthread_exit().
//
static int PThr4wTest_Join1()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int i = (int)(size_t)arg;
			Sleep(i * 100);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	pthread_t id[4];
	int i;
	void * result = (void*)-1;
	// Create a few threads and then exit. 
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&id[i], NULL, InnerBlock::ThreadFunc, (void*)(size_t)i) == 0);
	}
	// Some threads will finish before they are joined, some after. 
	Sleep(2 * 100 + 50);
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(id[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create().
//
static int PThr4wTest_Join2()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			Sleep(1000);
			return arg;
		}
	};
	pthread_t id[4];
	int i;
	void * result = (void*)-1;
	// Create a few threads and then exit
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&id[i], NULL, InnerBlock::ThreadFunc, (void*)(size_t)i) == 0);
	}
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(id[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Depends on API functions: pthread_create().
//
static int PThr4wTest_Join3()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			sched_yield();
			return arg;
		}
	};
	pthread_t id[4];
	int i;
	void* result = (void*)-1;
	/* Create a few threads and then exit. */
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	/*
	 * Let threads exit before we join them.
	 * We should still retrieve the exit code for the threads.
	 */
	Sleep(1000);
	for(i = 0; i < 4; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(id[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create().
//
static int PThr4wTest_Join4()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			Sleep(1200);
			return arg;
		}
	};
	pthread_t id;
	struct timespec abstime, reltime = { 1, 0 };
	void * result = (void*)-1;
	PTHR4W_TEST_ASSERT(pthread_create(&id, NULL, InnerBlock::ThreadFunc, (void*)(size_t)999) == 0);
	// 
	// Let thread start before we attempt to join it.
	// 
	Sleep(100);
	pthread_win32_getabstime_np(&abstime, &reltime);
	// Test for pthread_timedjoin_np timeout 
	PTHR4W_TEST_ASSERT(pthread_timedjoin_np(id, &result, &abstime) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT((int)(size_t)result == -1);
	// Test for pthread_tryjoin_np behaviour before thread has exited 
	PTHR4W_TEST_ASSERT(pthread_tryjoin_np(id, &result) == EBUSY);
	PTHR4W_TEST_ASSERT((int)(size_t)result == -1);
	Sleep(500);
	// Test for pthread_tryjoin_np behaviour after thread has exited 
	PTHR4W_TEST_ASSERT(pthread_tryjoin_np(id, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 999);
	return 0; // Success
}
// 
// Test Synopsis:
// - Confirm that thread reuse works for joined threads.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Reuse1()
{
	const int NUMTHREADS = 100;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			washere = 1;
			return arg;
		}
	};
	pthread_t t, last_t;
	pthread_attr_t attr;
	void * result = NULL;
	int i;
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);;
	PTHR4W_TEST_ASSERT(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0);
	washere = 0;
	PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);;
	PTHR4W_TEST_ASSERT((int)(size_t)result == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	last_t = t;
	for(i = 1; i < NUMTHREADS; i++) {
		washere = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, (void*)(size_t)i) == 0);
		pthread_join(t, &result);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
		PTHR4W_TEST_ASSERT(washere == 1);
		/* thread IDs should be unique */
		PTHR4W_TEST_ASSERT(!pthread_equal(t, last_t));
		/* thread struct pointers should be the same */
		PTHR4W_TEST_ASSERT(t.p == last_t.p);
		/* thread handle reuse counter should be different by one */
		PTHR4W_TEST_ASSERT(t.x == last_t.x+1);
		last_t = t;
	}
	return 0;
}
// 
// Test Synopsis:
// - Test that thread reuse works for detached threads.
// - Analyse thread struct reuse.
// Environment:
// - This test is implementation specific
// because it uses knowledge of internals that should be opaque to an application.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Reuse2()
{
	const int NUMTHREADS = 10000;
	static long done = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			sched_yield();
			InterlockedIncrement(&done);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	pthread_attr_t attr;
	int i;
	uint notUnique = 0;
	uint totalHandles = 0;
	uint reuseMax = 0;
	uint reuseMin = NUMTHREADS;
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
	for(i = 0; i < NUMTHREADS; i++) {
		while(pthread_create(&t[i], &attr, InnerBlock::ThreadFunc, NULL) != 0)
			Sleep(1);
	}
	while(NUMTHREADS > InterlockedExchangeAdd((LPLONG)&done, 0L))
		Sleep(100);
	Sleep(100);
	// 
	// Analyse reuse by computing min and max number of times pthread_create()
	// returned the same pthread_t value.
	// 
	for(i = 0; i < NUMTHREADS; i++) {
		if(t[i].p != NULL) {
			uint j;
			uint thisMax = t[i].x;
			for(j = i+1; j < NUMTHREADS; j++) {
				if(t[i].p == t[j].p) {
					if(t[i].x == t[j].x)
						notUnique++;
					if(thisMax < t[j].x)
						thisMax = t[j].x;
					t[j].p = NULL;
				}
			}
			if(reuseMin > thisMax)
				reuseMin = thisMax;
			if(reuseMax < thisMax)
				reuseMax = thisMax;
		}
	}
	for(i = 0; i < NUMTHREADS; i++)
		if(t[i].p != NULL)
			totalHandles++;
	// 
	// pthread_t reuse counts start at 0, so we need to add 1
	// to the max and min values derived above.
	// 
	printf("For %d total threads:\n", NUMTHREADS);
	printf("Non-unique IDs = %d\n", notUnique);
	printf("Reuse maximum  = %d\n", reuseMax + 1);
	printf("Reuse minimum  = %d\n", reuseMin + 1);
	printf("Total handles  = %d\n", totalHandles);
	return 0;
}
// 
// Test Synopsis:
// - Test thread priority explicit setting using thread attribute.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Priority1()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int policy;
			struct sched_param param;
			pthread_t threadID = pthread_self();
			PTHR4W_TEST_ASSERT(pthread_getschedparam(threadID, &policy, &param) == 0);
			PTHR4W_TEST_ASSERT(policy == SCHED_OTHER);
			return (void*)(size_t)(param.sched_priority);
		}
		static void * getValidPriorities(void * arg)
		{
			int prioSet;
			pthread_t threadID = pthread_self();
			HANDLE threadH = pthread_getw32threadhandle_np(threadID);
			printf("Using GetThreadPriority\n");
			printf("%10s %10s\n", "Set value", "Get value");
			for(prioSet = minPrio; prioSet <= maxPrio; prioSet++) {
				/*
				 * If prioSet is invalid then the threads priority is unchanged
				 * from the previous value. Make the previous value a known
				 * one so that we can check later.
				 */
				if(prioSet < 0)
					SetThreadPriority(threadH, THREAD_PRIORITY_LOWEST);
				else
					SetThreadPriority(threadH, THREAD_PRIORITY_HIGHEST);
				SetThreadPriority(threadH, prioSet);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
				printf("%10d %10d\n", prioSet, validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)]);
			}
			return 0;
		}
	};
	pthread_t t;
	pthread_attr_t attr;
	void * result = NULL;
	struct sched_param param;
	PTHR4W_TEST_ASSERT((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	PTHR4W_TEST_ASSERT((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);
	// Set the thread's priority to a known initial value. 
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	printf("Using pthread_getschedparam\n");
	printf("%10s %10s %10s\n", "Set value", "Get value", "Win priority");
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		int prio;
		PTHR4W_TEST_ASSERT(pthread_attr_setschedparam(&attr, &param) == 0);
		PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, (void*)&attr) == 0);
		PTHR4W_TEST_ASSERT((prio = GetThreadPriority(pthread_getw32threadhandle_np(t))) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT(param.sched_priority == (int)(size_t)result);
		printf("%10d %10d %10d\n", param.sched_priority, (int)(size_t)result, prio);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test thread priority setting after creation.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Priority2()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];
	static pthread_barrier_t startBarrier;
	static pthread_barrier_t endBarrier;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int policy;
			struct sched_param param;
			int result = pthread_barrier_wait(&startBarrier);
			PTHR4W_TEST_ASSERT(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
			PTHR4W_TEST_ASSERT(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
			PTHR4W_TEST_ASSERT(policy == SCHED_OTHER);
			result = pthread_barrier_wait(&endBarrier);
			PTHR4W_TEST_ASSERT(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
			return (void*)(size_t)param.sched_priority;
		}
		static void * getValidPriorities(void * arg)
		{
			int prioSet;
			pthread_t thread = pthread_self();
			HANDLE threadH = pthread_getw32threadhandle_np(thread);
			struct sched_param param;
			for(prioSet = minPrio;
				prioSet <= maxPrio;
				prioSet++) {
				// 
				// If prioSet is invalid then the threads priority is unchanged
				// from the previous value. Make the previous value a known
				// one so that we can check later.
				// 
				param.sched_priority = prioSet;
				PTHR4W_TEST_ASSERT(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
			}
			return 0;
		}
	};
	pthread_t t;
	void * result = NULL;
	int result2;
	struct sched_param param;
	PTHR4W_TEST_ASSERT((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	PTHR4W_TEST_ASSERT((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&startBarrier, NULL, 2) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&endBarrier, NULL, 2) == 0);
	// Set the thread's priority to a known initial value.
	// If the new priority is invalid then the threads priority
	// is unchanged from the previous value.
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_setschedparam(t, SCHED_OTHER, &param) == 0);
		result2 = pthread_barrier_wait(&startBarrier);
		PTHR4W_TEST_ASSERT(oneof2(result2, 0, PTHREAD_BARRIER_SERIAL_THREAD));
		result2 = pthread_barrier_wait(&endBarrier);
		PTHR4W_TEST_ASSERT(oneof2(result2, 0, PTHREAD_BARRIER_SERIAL_THREAD));
		PTHR4W_TEST_ASSERT(GetThreadPriority(pthread_getw32threadhandle_np(t)) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		pthread_join(t, &result);
		PTHR4W_TEST_ASSERT(param.sched_priority == (int)(size_t)result);
	}
	return 0;
}
// 
// Create a static pthread_once and test that it calls myfunc once.
// Depends on API functions: pthread_once(), pthread_create()
// 
static int PThr4wTest_Once1()
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	static int washere = 0;

	class InnerBlock {
	public:
		static void myfunc(void)
		{
			washere++;
		}
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_once(&once, myfunc) == 0);
			return 0;
		}
	};
	pthread_t t1, t2;
	PTHR4W_TEST_ASSERT(pthread_create(&t1, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t2, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Create several static pthread_once objects and channel several threads through each.
// Depends on API functions: pthread_once(), pthread_create()
//
static int PThr4wTest_Once2()
{
	static const int NUM_THREADS = 100; // Targeting each once control 
	static const int NUM_ONCE = 10;
	static pthread_once_t o = PTHREAD_ONCE_INIT;
	static pthread_once_t once[NUM_ONCE];
	static sharedInt_t numOnce;
	static sharedInt_t numThreads;

	class InnerBlock {
		static void myfunc(void)
		{
			EnterCriticalSection(&numOnce.cs);
			numOnce.i++;
			LeaveCriticalSection(&numOnce.cs);
			// Simulate slow once routine so that following threads pile up behind it
			Sleep(100);
		}
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_once(&once[(int)(size_t)arg], myfunc) == 0);
			EnterCriticalSection(&numThreads.cs);
			numThreads.i++;
			LeaveCriticalSection(&numThreads.cs);
			return (void*)(size_t)0;
		}
	};
	pthread_t t[NUM_THREADS][NUM_ONCE];
	int i, j;
	memzero(&numOnce, sizeof(sharedInt_t));
	memzero(&numThreads, sizeof(sharedInt_t));
	InitializeCriticalSection(&numThreads.cs);
	InitializeCriticalSection(&numOnce.cs);
	for(j = 0; j < NUM_ONCE; j++) {
		once[j] = o;
		for(i = 0; i < NUM_THREADS; i++) {
			// GCC build: create was failing with EAGAIN after 790 threads 
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::ThreadFunc, (void*)(size_t)j))
				sched_yield();
		}
	}
	for(j = 0; j < NUM_ONCE; j++)
		for(i = 0; i < NUM_THREADS; i++)
			if(pthread_join(t[i][j], NULL) != 0)
				printf("Join failed for [thread,once] = [%d,%d]\n", i, j);

	PTHR4W_TEST_ASSERT(numOnce.i == NUM_ONCE);
	PTHR4W_TEST_ASSERT(numThreads.i == NUM_THREADS * NUM_ONCE);
	DeleteCriticalSection(&numOnce.cs);
	DeleteCriticalSection(&numThreads.cs);
	return 0;
}
// 
// Create several pthread_once objects and channel several threads
// through each. Make the init_routine cancelable and cancel them with waiters waiting.
// 
// Depends on API functions: pthread_once(), pthread_create(), pthread_testcancel(), pthread_cancel(), pthread_once()
// 
static int PThr4wTest_Once3()
{
	static const int NUM_THREADS = 100; // Targeting each once control 
	static const int NUM_ONCE = 10;
	static pthread_once_t o = PTHREAD_ONCE_INIT;
	static pthread_once_t once[NUM_ONCE];
	static sharedInt_t numOnce;
	static sharedInt_t numThreads;

	class InnerBlock {
		static void myfunc(void)
		{
			EnterCriticalSection(&numOnce.cs);
			numOnce.i++;
			PTHR4W_TEST_ASSERT(numOnce.i > 0);
			LeaveCriticalSection(&numOnce.cs);
			// Simulate slow once routine so that following threads pile up behind it 
			Sleep(10);
			// Test for cancellation late so we're sure to have waiters. 
			pthread_testcancel();
		}
	public:
		static void * ThreadFunc(void * arg)
		{
			/*
			 * Cancel every thread. These threads are deferred cancelable only, so
			 * this thread will see it only when it performs the once routine (my_func).
			 * The result will be that every thread eventually cancels only when it
			 * becomes the new 'once' thread.
			 */
			PTHR4W_TEST_ASSERT(pthread_cancel(pthread_self()) == 0);
			/*
			 * Now we block on the 'once' control.
			 */
			PTHR4W_TEST_ASSERT(pthread_once(&once[(int)(size_t)arg], myfunc) == 0);
			/*
			 * We should never get to here.
			 */
			EnterCriticalSection(&numThreads.cs);
			numThreads.i++;
			LeaveCriticalSection(&numThreads.cs);
			return (void*)(size_t)0;
		}
	};
	pthread_t t[NUM_THREADS][NUM_ONCE];
	int i, j;
#if defined (__PTW32_CONFIG_MSVC6) && defined(__PTW32_CLEANUP_CXX)
	puts("If this test fails or hangs, rebuild the library with /EHa instead of /EHs.");
	puts("(This is a known issue with Microsoft VC++6.0.)");
	fflush(stdout);
#endif
	memzero(&numOnce, sizeof(sharedInt_t));
	memzero(&numThreads, sizeof(sharedInt_t));
	InitializeCriticalSection(&numThreads.cs);
	InitializeCriticalSection(&numOnce.cs);
	for(j = 0; j < NUM_ONCE; j++) {
		once[j] = o;
		for(i = 0; i < NUM_THREADS; i++) {
			// GCC build: create was failing with EAGAIN after 790 threads 
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::ThreadFunc, (void*)(size_t)j))
				sched_yield();
		}
	}
	for(j = 0; j < NUM_ONCE; j++)
		for(i = 0; i < NUM_THREADS; i++)
			if(pthread_join(t[i][j], NULL) != 0)
				printf("Join failed for [thread,once] = [%d,%d]\n", i, j);
	/*
	 * All threads will cancel, none will return normally from
	 * pthread_once and so numThreads should never be incremented. However,
	 * numOnce should be incremented by every thread (NUM_THREADS*NUM_ONCE).
	 */
	PTHR4W_TEST_ASSERT(numOnce.i == NUM_ONCE * NUM_THREADS);
	PTHR4W_TEST_ASSERT(numThreads.i == 0);
	DeleteCriticalSection(&numOnce.cs);
	DeleteCriticalSection(&numThreads.cs);
	return 0;
}
// 
// Create several pthread_once objects and channel several threads
// through each. Make the init_routine cancelable and cancel them
// waiters waiting. Vary the priorities.
// 
// Depends on API functions: pthread_once(), pthread_create(), pthread_testcancel(), pthread_cancel(), pthread_once()
// 
static int PThr4wTest_Once4()
{
	static const int NUM_THREADS = 100; // Targeting each once control 
	static const int NUM_ONCE = 10;
	static pthread_once_t o = PTHREAD_ONCE_INIT;
	static pthread_once_t once[NUM_ONCE];
	static sharedInt_t numOnce;
	static sharedInt_t numThreads;
	static bag_t threadbag[NUM_THREADS][NUM_ONCE];
	static CRITICAL_SECTION print_lock;

	class InnerBlock {
		static void mycleanupfunc(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			EnterCriticalSection(&print_lock);
			/*      once thrd  prio error */
			printf("%4d %4d %4d %4d\n", bag->oncenum, bag->threadnum, bag->myPrio, bag->myPrio - GetThreadPriority(bag->w32Thread));
			LeaveCriticalSection(&print_lock);
		}
		static void myinitfunc(void)
		{
			EnterCriticalSection(&numOnce.cs);
			numOnce.i++;
			LeaveCriticalSection(&numOnce.cs);
			/* Simulate slow once routine so that following threads pile up behind it */
			Sleep(10);
			/* test for cancellation late so we're sure to have waiters. */
			pthread_testcancel();
		}
	public:
		static void * ThreadFunc(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			struct sched_param param;
			/*
			 * Cancel every thread. These threads are deferred cancelable only, so
			 * only the thread performing the init_routine will see it (there are
			 * no other cancellation points here). The result will be that every thread
			 * eventually cancels only when it becomes the new initter.
			 */
			pthread_t self = pthread_self();
			bag->w32Thread = pthread_getw32threadhandle_np(self);
			/*
			 * Set priority between -2 and 2 inclusive.
			 */
			bag->myPrio = (bag->threadnum % 5) - 2;
			param.sched_priority = bag->myPrio;
			pthread_setschedparam(self, SCHED_OTHER, &param);

			/* Trigger a cancellation at the next cancellation point in this thread */
			pthread_cancel(self);
		#if 0
			pthread_cleanup_push(mycleanupfunc, arg);
			PTHR4W_TEST_ASSERT(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
			pthread_cleanup_pop(1);
		#else
			PTHR4W_TEST_ASSERT(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
		#endif
			EnterCriticalSection(&numThreads.cs);
			numThreads.i++;
			LeaveCriticalSection(&numThreads.cs);
			return 0;
		}
	};
	pthread_t t[NUM_THREADS][NUM_ONCE];
	int i, j;
#if defined (__PTW32_CONFIG_MSVC6) && defined(__PTW32_CLEANUP_CXX)
	puts("If this test fails or hangs, rebuild the library with /EHa instead of /EHs.");
	puts("(This is a known issue with Microsoft VC++6.0.)");
	fflush(stdout);
#endif
	memzero(&numOnce, sizeof(sharedInt_t));
	memzero(&numThreads, sizeof(sharedInt_t));
	InitializeCriticalSection(&print_lock);
	InitializeCriticalSection(&numThreads.cs);
	InitializeCriticalSection(&numOnce.cs);
#if 0
	/*       once thrd  prio change */
	printf("once thrd  prio  error\n");
#endif
	/*
	 * Set the priority class to realtime - otherwise normal
	 * Windows random priority boosting will obscure any problems.
	 */
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	/* Set main thread to lower prio than threads */
	SetThreadPriority(GetCurrentThread(), -2);
	for(j = 0; j < NUM_ONCE; j++) {
		once[j] = o;
		for(i = 0; i < NUM_THREADS; i++) {
			bag_t * bag = &threadbag[i][j];
			bag->threadnum = i;
			bag->oncenum = j;
			// GCC build: create was failing with EAGAIN after 790 threads
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::ThreadFunc, (void*)bag))
				sched_yield();
		}
	}
	for(j = 0; j < NUM_ONCE; j++)
		for(i = 0; i < NUM_THREADS; i++)
			if(pthread_join(t[i][j], NULL) != 0)
				printf("Join failed for [thread,once] = [%d,%d]\n", i, j);
	/*
	 * All threads will cancel, none will return normally from
	 * pthread_once and so numThreads should never be incremented. However,
	 * numOnce should be incremented by every thread (NUM_THREADS*NUM_ONCE).
	 */
	PTHR4W_TEST_ASSERT(numOnce.i == NUM_ONCE * NUM_THREADS);
	PTHR4W_TEST_ASSERT(numThreads.i == 0);
	DeleteCriticalSection(&numOnce.cs);
	DeleteCriticalSection(&numThreads.cs);
	DeleteCriticalSection(&print_lock);
	return 0;
}
// 
// Test Synopsis:
// - Test thread priority inheritance.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Inherit1()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int policy;
			struct sched_param param;
			PTHR4W_TEST_ASSERT(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
			return (void*)(size_t)param.sched_priority;
		}
		static void * ThreadFunc_getValidPriorities(void * arg)
		{
			int prioSet;
			pthread_t thread = pthread_self();
			HANDLE threadH = pthread_getw32threadhandle_np(thread);
			struct sched_param param;
			for(prioSet = minPrio; prioSet <= maxPrio; prioSet++) {
				// 
				// If prioSet is invalid then the threads priority is unchanged
				// from the previous value. Make the previous value a known
				// one so that we can check later.
				// 
				param.sched_priority = prioSet;
				PTHR4W_TEST_ASSERT(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
			}
			return 0;
		}
	};
	pthread_t t;
	pthread_t mainThread = pthread_self();
	pthread_attr_t attr;
	void * result = NULL;
	struct sched_param param;
	struct sched_param mainParam;
	int prio;
	int policy;
	int inheritsched = -1;
	pthread_t threadID = pthread_self();
	HANDLE threadH = pthread_getw32threadhandle_np(threadID);
	maxPrio = sched_get_priority_max(SCHED_OTHER);
	PTHR4W_TEST_ASSERT(maxPrio != -1);
	minPrio = sched_get_priority_min(SCHED_OTHER);
	PTHR4W_TEST_ASSERT(minPrio != -1);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc_getValidPriorities, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_getinheritsched(&attr, &inheritsched) == 0);
	PTHR4W_TEST_ASSERT(inheritsched == PTHREAD_INHERIT_SCHED);
	for(prio = minPrio; prio <= maxPrio; prio++) {
		mainParam.sched_priority = prio;
		// Set the thread's priority to a known initial value. 
		SetThreadPriority(threadH, PTW32TEST_THREAD_INIT_PRIO);
		// Change the main thread priority 
		PTHR4W_TEST_ASSERT(pthread_setschedparam(mainThread, SCHED_OTHER, &mainParam) == 0);
		PTHR4W_TEST_ASSERT(pthread_getschedparam(mainThread, &policy, &mainParam) == 0);
		PTHR4W_TEST_ASSERT(policy == SCHED_OTHER);
		// Priority returned below should be the level set by pthread_setschedparam(). 
		PTHR4W_TEST_ASSERT(mainParam.sched_priority == prio);
		PTHR4W_TEST_ASSERT(GetThreadPriority(threadH) == validPriorities[prio+(PTW32TEST_MAXPRIORITIES/2)]);
		for(param.sched_priority = prio;
		    param.sched_priority <= maxPrio;
		    param.sched_priority++) {
			// The new thread create should ignore this new priority 
			PTHR4W_TEST_ASSERT(pthread_attr_setschedparam(&attr, &param) == 0);
			PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, NULL) == 0);
			pthread_join(t, &result);
			PTHR4W_TEST_ASSERT((int)(size_t)result == mainParam.sched_priority);
		}
	}
	return 0;
}
// 
// Create a barrier object and then destroy it.
// Declare a single barrier object, wait on it, and then destroy it.
// Declare a single barrier object with barrier attribute, wait on it, and then destroy it.
// Declare a single barrier object, multiple wait on it, and then destroy it.
// 
// Set up a series of barriers at different heights and test various numbers
// of threads accessing, especially cases where there are more threads than the
// barrier height (count), i.e. test contention when the barrier is released.
// 
// Destroy the barrier after initial count threads are released then let additional threads attempt to wait on it.
// 
static int PThr4wTest_Barrier1()
{
	static pthread_barrier_t barrier = NULL;
	PTHR4W_TEST_ASSERT(barrier == NULL);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, NULL, 1) == 0);
	PTHR4W_TEST_ASSERT(barrier != NULL);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
	PTHR4W_TEST_ASSERT(barrier == NULL);
	return 0;
}

static int PThr4wTest_Barrier2()
{
	static pthread_barrier_t barrier = NULL;
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, NULL, 1) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
	return 0;
}

static int PThr4wTest_Barrier3()
{
	static pthread_barrier_t barrier = NULL;
	static void * result = (void*)1;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			return (void*)(size_t)pthread_barrier_wait(&barrier);
		}
	};
	pthread_t t;
	pthread_barrierattr_t ba;
	PTHR4W_TEST_ASSERT(pthread_barrierattr_init(&ba) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrierattr_setpshared(&ba, PTHREAD_PROCESS_PRIVATE) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, &ba, 1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == PTHREAD_BARRIER_SERIAL_THREAD);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrierattr_destroy(&ba) == 0);
	return 0;
}

static int PThr4wTest_Barrier4()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mx) == 0);
			if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
				serialThreadCount++;
			}
			else if(0 == result) {
				otherThreadCount++;
			}
			else {
				printf("Barrier wait failed: error = %s\n", PThr4wErrorString[result]);
				fflush(stdout);
				return NULL;
			}
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mx) == 0);
			return NULL;
		}
	};
	const int NUMTHREADS = 16;
	pthread_t t[NUMTHREADS + 1];
	for(int j = 1; j <= NUMTHREADS; j++) {
		int i;
		printf("Barrier height = %d\n", j);
		serialThreadCount = 0;
		PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, NULL, j) == 0);
		for(i = 1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::ThreadFunc, NULL) == 0);
		}
		for(i = 1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
		}
		PTHR4W_TEST_ASSERT(serialThreadCount == 1);
		PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

static int PThr4wTest_Barrier5()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static LONG totalThreadCrossings;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * crossings)
		{
			int result;
			int serialThreads = 0;
			while((LONG)(size_t)crossings >= (LONG)InterlockedIncrement((LPLONG)&totalThreadCrossings)) {
				result = pthread_barrier_wait(&barrier);
				if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
					serialThreads++;
				}
				else if(result != 0) {
					printf("Barrier failed: result = %s\n", PThr4wErrorString[result]);
					fflush(stdout);
					return NULL;
				}
			}
			return (void*)(size_t)serialThreads;
		}
	};
	const int NUMTHREADS = 15;
	const int HEIGHT = 10;
	const int BARRIERMULTIPLE = 1000;
	int i, j;
	void * result;
	int serialThreadsTotal;
	LONG Crossings;
	pthread_t t[NUMTHREADS + 1];
	for(j = 1; j <= NUMTHREADS; j++) {
		int height = (j < HEIGHT) ? j : HEIGHT;
		totalThreadCrossings = 0;
		Crossings = height * BARRIERMULTIPLE;
		printf("Threads=%d, Barrier height=%d\n", j, height);
		PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, NULL, height) == 0);
		for(i = 1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::ThreadFunc, (void*)(size_t)Crossings) == 0);
		}
		serialThreadsTotal = 0;
		for(i = 1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
			serialThreadsTotal += (int)(size_t)result;
		}
		PTHR4W_TEST_ASSERT(serialThreadsTotal == BARRIERMULTIPLE);
		PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

static int PThr4wTest_Barrier6()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mx) == 0);
			if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
				serialThreadCount++;
			}
			else if(0 == result) {
				otherThreadCount++;
			}
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mx) == 0);
			return NULL;
		}
	};
	const int NUMTHREADS = 31;
	pthread_t t[NUMTHREADS + 1];
	for(int j = 1; j <= NUMTHREADS; j++) {
		int i;
		int howHigh = j/2 + 1;
		printf("Barrier height = %d, Total threads %d\n", howHigh, j);
		serialThreadCount = 0;
		otherThreadCount = 0;
		PTHR4W_TEST_ASSERT(pthread_barrier_init(&barrier, NULL, howHigh) == 0);
		for(i = 1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::ThreadFunc, NULL) == 0);
			if(i == howHigh) {
				for(int k = 1; k <= howHigh; k++) {
					PTHR4W_TEST_ASSERT(pthread_join(t[k], NULL) == 0);
				}
				PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == 0);
			}
		}
		for(i = howHigh+1; i <= j; i++) {
			PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
		}
		PTHR4W_TEST_ASSERT(serialThreadCount == 1);
		PTHR4W_TEST_ASSERT(otherThreadCount == (howHigh - 1));
		PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&barrier) == EINVAL);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mx) == 0);
	return 0;
}
// 
// Test Synopsis: Verify trywait() returns -1 and sets EAGAIN.
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
static int PThr4wTest_Semaphore1()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			sem_t s;
			int result;
			PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			PTHR4W_TEST_ASSERT((result = sem_trywait(&s)) == -1);
			if(result == -1) {
				int err =
		#if defined (__PTW32_USES_SEPARATE_CRT)
					GetLastError();
		#else
					errno;
		#endif
				if(err != EAGAIN) {
					printf("thread: sem_trywait 1: expecting error %s: got %s\n", PThr4wErrorString[EAGAIN], PThr4wErrorString[err]); fflush(stdout);
				}
				PTHR4W_TEST_ASSERT(err == EAGAIN);
			}
			else {
				printf("thread: ok 1\n");
			}
			PTHR4W_TEST_ASSERT((result = sem_post(&s)) == 0);
			PTHR4W_TEST_ASSERT((result = sem_trywait(&s)) == 0);
			PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
			return NULL;
		}
	};
	pthread_t t;
	sem_t s;
	void* result1 = (void*)-1;
	int result2;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::thr, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result1) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result1 == 0);
	PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	PTHR4W_TEST_ASSERT((result2 = sem_trywait(&s)) == -1);
	if(result2 == -1) {
		int err =
#if defined (__PTW32_USES_SEPARATE_CRT)
		    GetLastError();
#else
		    errno;
#endif
		if(err != EAGAIN) {
			printf("main: sem_trywait 1: expecting error %s: got %s\n", PThr4wErrorString[EAGAIN], PThr4wErrorString[err]); fflush(stdout);
		}
		PTHR4W_TEST_ASSERT(err == EAGAIN);
	}
	else {
		printf("main: ok 1\n");
	}
	PTHR4W_TEST_ASSERT((result2 = sem_post(&s)) == 0);
	PTHR4W_TEST_ASSERT((result2 = sem_trywait(&s)) == 0);
	PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct value.
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
static int PThr4wTest_Semaphore2()
{
	sem_t s;
	int value = 0;
	int i;
	PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, SEMAPHORE_MAX_COUNT) == 0);
	PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
	PTHR4W_TEST_ASSERT(value == SEMAPHORE_MAX_COUNT);
//	  printf("Value = %ld\n", value);
	for(i = SEMAPHORE_MAX_COUNT - 1; i >= 0; i--) {
		PTHR4W_TEST_ASSERT(sem_wait(&s) == 0);
		PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		PTHR4W_TEST_ASSERT(value == i);
	}
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
		PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		PTHR4W_TEST_ASSERT(value == i);
	}
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters.
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
static int PThr4wTest_Semaphore3()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			PTHR4W_TEST_ASSERT(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[SEMAPHORE_MAX_COUNT+1];
	PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
	//printf("Value = %d\n", value);	fflush(stdout);
	PTHR4W_TEST_ASSERT(value == 0);
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
		}
		while(-value != i);
		//printf("1:Value = %d\n", value); fflush(stdout);
		PTHR4W_TEST_ASSERT(-value == i);
	}
	for(i = SEMAPHORE_MAX_COUNT - 1; i >= 0; i--) {
		PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
		PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
		//printf("2:Value = %d\n", value);	fflush(stdout);
		PTHR4W_TEST_ASSERT(-value == i);
	}
	for(i = SEMAPHORE_MAX_COUNT; i > 0; i--) {
		pthread_join(t[i], NULL);
	}
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters after threads are cancelled.
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
static int PThr4wTest_Semaphore4()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			PTHR4W_TEST_ASSERT(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[SEMAPHORE_MAX_COUNT+1];
	PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
	PTHR4W_TEST_ASSERT(value == 0);
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
		} while(value != -i);
		PTHR4W_TEST_ASSERT(-value == i);
	}
	PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
	PTHR4W_TEST_ASSERT(-value == SEMAPHORE_MAX_COUNT);
	PTHR4W_TEST_ASSERT(pthread_cancel(t[50]) == 0);
	{
		void* result;
		PTHR4W_TEST_ASSERT(pthread_join(t[50], &result) == 0);
	}
	PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
	PTHR4W_TEST_ASSERT(-value == (SEMAPHORE_MAX_COUNT - 1));
	for(i = SEMAPHORE_MAX_COUNT - 2; i >= 0; i--) {
		PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
		PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
		PTHR4W_TEST_ASSERT(-value == i);
	}
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++)
		if(i != 50)
			PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
	return 0;
}
// 
// Test Synopsis: Verify sem_getvalue returns the correct number of waiters after threads are cancelled.
// Test Method (Validation or Falsification):
// - Validation
// Requirements Tested:
// - sem_timedwait cancellation.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Semaphore4t()
{
	static const long NANOSEC_PER_SEC = 1000000000L;
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			PTHR4W_TEST_ASSERT(sem_timedwait(&s, NULL) == 0);
			return NULL;
		}
		static int timeoutwithnanos(sem_t sem, int nanoseconds)
		{
			struct timespec ts, rel;
			FILETIME ft_before, ft_after;
			int rc;
			rel.tv_sec = 0;
			rel.tv_nsec = nanoseconds;
			GetSystemTimeAsFileTime(&ft_before);
			rc = sem_timedwait(&sem, pthread_win32_getabstime_np(&ts, &rel));
			// This should have timed out 
			PTHR4W_TEST_ASSERT(rc != 0);
			PTHR4W_TEST_ASSERT(errno == ETIMEDOUT);
			GetSystemTimeAsFileTime(&ft_after);
			// We specified a non-zero wait. Time must advance.
			if(ft_before.dwLowDateTime == ft_after.dwLowDateTime && ft_before.dwHighDateTime == ft_after.dwHighDateTime) {
				printf("nanoseconds: %d, rc: %d, errno: %d. before filetime: %d, %d; after filetime: %d, %d\n",
					nanoseconds, rc, errno,
					(int)ft_before.dwLowDateTime, (int)ft_before.dwHighDateTime,
					(int)ft_after.dwLowDateTime, (int)ft_after.dwHighDateTime);
				printf("time must advance during sem_timedwait.");
				return 1;
			}
			return 0;
		}
		static int testtimeout()
		{
			int rc = 0;
			sem_t s2;
			int value = 0;
			PTHR4W_TEST_ASSERT(sem_init(&s2, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			PTHR4W_TEST_ASSERT(sem_getvalue(&s2, &value) == 0);
			PTHR4W_TEST_ASSERT(value == 0);
			rc += timeoutwithnanos(s2, 1000);  // 1 microsecond
			rc += timeoutwithnanos(s2, 10 * 1000); // 10 microseconds
			rc += timeoutwithnanos(s2, 100 * 1000); // 100 microseconds
			rc += timeoutwithnanos(s2, 1000 * 1000); // 1 millisecond
			return rc;
		}
		static int testmainstuff()
		{
			int value = 0;
			int i;
			pthread_t t[SEMAPHORE_MAX_COUNT+1];
			PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
			PTHR4W_TEST_ASSERT(value == 0);
			for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
				PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, thr, NULL) == 0);
				do {
					sched_yield();
					PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
				} while(value != -i);
				PTHR4W_TEST_ASSERT(-value == i);
			}
			PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
			PTHR4W_TEST_ASSERT(-value == SEMAPHORE_MAX_COUNT);
			PTHR4W_TEST_ASSERT(pthread_cancel(t[50]) == 0);
			PTHR4W_TEST_ASSERT(pthread_join(t[50], NULL) == 0);
			PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
			PTHR4W_TEST_ASSERT(-value == SEMAPHORE_MAX_COUNT - 1);
			for(i = SEMAPHORE_MAX_COUNT - 2; i >= 0; i--) {
				PTHR4W_TEST_ASSERT(sem_post(&s) == 0);
				PTHR4W_TEST_ASSERT(sem_getvalue(&s, &value) == 0);
				PTHR4W_TEST_ASSERT(-value == i);
			}
			for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
				if(i != 50) {
					PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
				}
			}
			return 0;
		}
	};
	int rc = 0;
	rc += InnerBlock::testmainstuff();
	rc += InnerBlock::testtimeout();
	return rc;
}
// 
// Test Synopsis: Verify sem_destroy EBUSY race avoidance
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
static int PThr4wTest_Semaphore5()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			PTHR4W_TEST_ASSERT(sem_post((sem_t*)arg) == 0);
			return 0;
		}
	};
	pthread_t t;
	sem_t s;
	PTHR4W_TEST_ASSERT(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::thr, (void*)&s) == 0);
	PTHR4W_TEST_ASSERT(sem_wait(&s) == 0);
	/*
	 * Normally we would retry this next, but we're only
	 * interested in unexpected results in this test.
	 */
	PTHR4W_TEST_ASSERT(sem_destroy(&s) == 0 || errno == EBUSY);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	return 0;
}
// 
// Author: Eyal Lebedinsky eyal@eyal.emu.id.au
// Written: Sep 1998.
// Version Date: 12 Sep 1998
// 
// Do we need to lock stdout or is it thread safe?
// 
// Used: pthread_t, pthread_attr_t, pthread_create(), pthread_join(), pthread_mutex_t, PTHREAD_MUTEX_INITIALIZER,
//   pthread_mutex_init() [not used now], pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
// What this program does is establish a work queue (implemented using
// four mutexes for each thread). It then schedules work (by storing
// a number in 'todo') and releases the threads. When the work is done
// the threads will block. The program then repeats the same thing once
// more (just to test the logic) and when the work is done it destroyes the threads.
// 
// The 'work' we do is simply burning CPU cycles in a loop.
// The 'todo' work queue is trivial - each threads pops one element
// off it by incrementing it, the poped number is the 'work' to do.
// When 'todo' reaches the limit (nwork) the queue is considered empty.
// 
// The number displayed at the end is the amount of work each thread
// did, so we can see if the load was properly distributed.
// 
// The program was written to test a threading setup (not seen here)
// rather than to demonstrate correct usage of the pthread facilities.
// 
// Note how each thread is given access to a thread control structure
// (TC) which is used for communicating to/from the main program (e.g.
// the threads knows its 'id' and also filles in the 'work' done).
// 
static int PThr4wTest_Eyal1()
{
	struct thread_control {
		int id;
		pthread_t thread;       /* thread id */
		pthread_mutex_t mutex_start;
		pthread_mutex_t mutex_started;
		pthread_mutex_t mutex_end;
		pthread_mutex_t mutex_ended;
		long work;              /* work done */
		int stat;               /* pthread_init status */
	};

	typedef struct thread_control TC;

	static TC * tcs = NULL;
	static int nthreads = 10;
	static int nwork = 100;
	static int quiet = 0;
	static int todo = -1;
	static pthread_mutex_t mutex_todo = PTHREAD_MUTEX_INITIALIZER;
	static pthread_mutex_t mutex_stdout = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void die(int ret)
		{
			ZFREE(tcs);
			if(ret)
				exit(ret);
		}
		static double waste_time(int n)
		{
			double s = 0.0;
			// Useless work.
			for(int i = n*100; i > 0; --i) {
				const double f = rand();
				const double g = rand();
				const double h = rand();
				s += 2.0 * f * g / (h != 0.0 ? (h * h) : 1.0);
			}
			return s;
		}
		static int do_work_unit(int who, int n)
		{
			static int nchars = 0;
			double f = 0.0;
			if(!quiet) {
				// get lock on stdout
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex_stdout) == 0);
				// do our job
				printf("%c", "0123456789abcdefghijklmnopqrstuvwxyz"[who]);
				if(!(++nchars % 50))
					printf("\n");
				fflush(stdout);
				// release lock on stdout
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex_stdout) == 0);
			}
			n = rand() % 10000; /* ignore incoming 'n' */
			f = waste_time(n);
			// This prevents the statement above from being optimised out 
			if(f > 0.0)
				return n;
			return n;
		}
		static int print_server(void * ptr)
		{
			int mywork;
			int n;
			TC * tc = (TC*)ptr;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tc->mutex_started) == 0);
			for(;;) {
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tc->mutex_start) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tc->mutex_start) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tc->mutex_ended) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tc->mutex_started) == 0);
				for(;;) {
					/*
					 * get lock on todo list
					 */
					PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex_todo) == 0);
					mywork = todo;
					if(todo >= 0) {
						++todo;
						if(todo >= nwork)
							todo = -1;
					}
					PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex_todo) == 0);
					if(mywork < 0)
						break;
					PTHR4W_TEST_ASSERT((n = do_work_unit(tc->id, mywork)) >= 0);
					tc->work += n;
				}
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tc->mutex_end) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tc->mutex_end) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tc->mutex_started) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tc->mutex_ended) == 0);
				if(-2 == mywork)
					break;
			}
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tc->mutex_started) == 0);
			return (0);
		}
		static void dosync(void)
		{
			int i;
			for(i = 0; i < nthreads; ++i) {
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tcs[i].mutex_end) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_start) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tcs[i].mutex_started) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_started) == 0);
			}
			/*
			 * Now threads do their work
			 */
			for(i = 0; i < nthreads; ++i) {
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tcs[i].mutex_start) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_end) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tcs[i].mutex_ended) == 0);
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_ended) == 0);
			}
		}
		static void dowork(void)
		{
			todo = 0;
			dosync();
			todo = 0;
			dosync();
		}
	};
	int i;
	PTHR4W_TEST_ASSERT(NULL != (tcs = (TC *)SAlloc::C(nthreads, sizeof(*tcs))));
	/*
	 * Launch threads
	 */
	for(i = 0; i < nthreads; ++i) {
		tcs[i].id = i;
		PTHR4W_TEST_ASSERT(pthread_mutex_init(&tcs[i].mutex_start, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_init(&tcs[i].mutex_started, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_init(&tcs[i].mutex_end, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_init(&tcs[i].mutex_ended, NULL) == 0);
		tcs[i].work = 0;
		PTHR4W_TEST_ASSERT(pthread_mutex_lock(&tcs[i].mutex_start) == 0);
		PTHR4W_TEST_ASSERT((tcs[i].stat = pthread_create(&tcs[i].thread, NULL, (void *(*)(void *))InnerBlock::print_server, (void*)&tcs[i])) == 0);
		/*
		 * Wait for thread initialisation
		 */
		{
			int trylock = 0;
			while(trylock == 0) {
				trylock = pthread_mutex_trylock(&tcs[i].mutex_started);
				PTHR4W_TEST_ASSERT(trylock == 0 || trylock == EBUSY);
				if(trylock == 0) {
					PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_started) == 0);
				}
			}
		}
	}
	InnerBlock::dowork();
	/*
	 * Terminate threads
	 */
	todo = -2; /* please terminate */
	InnerBlock::dosync();
	for(i = 0; i < nthreads; ++i) {
		if(0 == tcs[i].stat)
			PTHR4W_TEST_ASSERT(pthread_join(tcs[i].thread, NULL) == 0);
	}
	/*
	 * destroy locks
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex_stdout) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex_todo) == 0);
	/*
	 * Cleanup
	 */
	printf("\n");
	/*
	 * Show results
	 */
	for(i = 0; i < nthreads; ++i) {
		printf("%2d ", i);
		if(0 == tcs[i].stat)
			printf("%10ld\n", tcs[i].work);
		else
			printf("failed %d\n", tcs[i].stat);
		PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&tcs[i].mutex_start) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&tcs[i].mutex_start) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&tcs[i].mutex_started) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&tcs[i].mutex_end) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&tcs[i].mutex_ended) == 0);
	}
	InnerBlock::die(0);
	return (0);
}
// 
// Test Synopsis:
// - confirm accuracy of abstime calculations and timeouts
// Test Method (Validation or Falsification):
// - time actual CV wait timeout using a sequence of increasing sub 1 second timeouts.
// Output:
// - Printed measured elapsed time should closely match specified timeout.
// - Return code should always be ETIMEDOUT (usually 138 but possibly 10060)
// Pass Criteria:
// - Relies on observation.
// 
static int PThr4wTest_Timeouts()
{
	#define DEFAULT_MINTIME_INIT    999999999
	#define CYG_ONEBILLION          1000000000LL
	#define CYG_ONEMILLION          1000000LL
	#define CYG_ONEKAPPA            1000LL
	#define MSEC_F 1000000L
	#define USEC_F 1000L
	#define NSEC_F 1L
#if defined(_MSC_VER) && (_MSC_VER > 1200)
	typedef long long cyg_tim_t; //msvc > 6.0
#else
	typedef int64_t cyg_tim_t; //msvc 6.0
#endif
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER global_start;
	static pthread_mutexattr_t mattr_;
	static pthread_mutex_t mutex_;
	static pthread_condattr_t cattr_;
	static pthread_cond_t cv_;

	class InnerBlock {
		#if 1
			static int GetTimestampTS(struct timespec * tv)
			{
				struct __timeb64 timebuffer;
			#if !(_MSC_VER <= 1200)
				// @sobolev _ftime64_s(&timebuffer); //msvc > 6.0
				_ftime64(&timebuffer); //msvc > 6.0 // @sobolev
			#else
				_ftime(&timebuffer); //msvc = 6.0
			#endif
				tv->tv_sec = timebuffer.time;
				tv->tv_nsec = 1000000L * timebuffer.millitm;
				return 0;
			}
		#else
			static int GetTimestampTS(struct timespec * tv)
			{
				static LONGLONG epoch = 0;
				SYSTEMTIME local;
				FILETIME abs;
				LONGLONG now;
				if(!epoch) {
					memset(&local, 0, sizeof(SYSTEMTIME));
					local.wYear = 1970;
					local.wMonth = 1;
					local.wDay = 1;
					local.wHour = 0;
					local.wMinute = 0;
					local.wSecond = 0;
					SystemTimeToFileTime(&local, &abs);
					epoch = *(LONGLONG*)&abs;
				}
				GetSystemTime(&local);
				SystemTimeToFileTime(&local, &abs);
				now = *(LONGLONG*)&abs;
				now = now - epoch;
				tv->tv_sec = (long)(now / 10000000);
				tv->tv_nsec = (long)((now * 100) % 1000000000);
				return 0;
			}
		#endif
	public:
		static cyg_tim_t CYG_DIFFT(cyg_tim_t t1, cyg_tim_t t2)
		{
			return (cyg_tim_t)((t2 - t1) * CYG_ONEBILLION / frequency.QuadPart); //nsec
		}
		static void CYG_InitTimers()
		{
			QueryPerformanceFrequency(&frequency);
			global_start.QuadPart = 0;
		}
		static void CYG_MARK1(cyg_tim_t * T)
		{
			LARGE_INTEGER curTime;
			QueryPerformanceCounter(&curTime);
			*T = (curTime.QuadPart);// + global_start.QuadPart);
		}
		static int Init()
		{
			PTHR4W_TEST_ASSERT(0 == pthread_mutexattr_init(&mattr_));
			PTHR4W_TEST_ASSERT(0 == pthread_mutex_init(&mutex_, &mattr_));
			PTHR4W_TEST_ASSERT(0 == pthread_condattr_init(&cattr_));
			PTHR4W_TEST_ASSERT(0 == pthread_cond_init(&cv_, &cattr_));
			return 0;
		}
		static int Destroy()
		{
			PTHR4W_TEST_ASSERT(0 == pthread_cond_destroy(&cv_));
			PTHR4W_TEST_ASSERT(0 == pthread_mutex_destroy(&mutex_));
			PTHR4W_TEST_ASSERT(0 == pthread_mutexattr_destroy(&mattr_));
			PTHR4W_TEST_ASSERT(0 == pthread_condattr_destroy(&cattr_));
			return 0;
		}
		static int Wait(time_t sec, long nsec)
		{
			struct timespec abstime;
			long sc;
			int result = 0;
			GetTimestampTS(&abstime);
			abstime.tv_sec  += sec;
			abstime.tv_nsec += nsec;
			if((sc = (abstime.tv_nsec / 1000000000L))) {
				abstime.tv_sec += sc;
				abstime.tv_nsec %= 1000000000L;
			}
			PTHR4W_TEST_ASSERT(0 == pthread_mutex_lock(&mutex_));
			// 
			// We don't need to check the CV.
			// 
			result = pthread_cond_timedwait(&cv_, &mutex_, &abstime);
			PTHR4W_TEST_ASSERT(result != 0);
			PTHR4W_TEST_ASSERT(errno == ETIMEDOUT);
			pthread_mutex_unlock(&mutex_);
			return result;
		}
		static void printtim(cyg_tim_t rt, cyg_tim_t dt, int wres)
		{
			printf("wait result [%d]: timeout(ms) [expected/actual]: %ld/%ld\n", wres, (long)(rt/CYG_ONEMILLION), (long)(dt/CYG_ONEMILLION));
		}
	};
	int i = 0;
	int wres = 0;
	cyg_tim_t t1, t2, dt, rt;
	InnerBlock::CYG_InitTimers();
	InnerBlock::Init();
	while(i++ < 10) {
		rt = 90*i*MSEC_F;
		InnerBlock::CYG_MARK1(&t1);
		wres = InnerBlock::Wait(0, (long)(size_t)rt);
		InnerBlock::CYG_MARK1(&t2);
		dt = InnerBlock::CYG_DIFFT(t1, t2);
		InnerBlock::printtim(rt, dt, wres);
	}
	InnerBlock::Destroy();
	return 0;
	#undef DEFAULT_MINTIME_INIT
	#undef CYG_ONEBILLION
	#undef CYG_ONEMILLION
	#undef CYG_ONEKAPPA
	#undef MSEC_F
	#undef USEC_F
	#undef NSEC_F
}
// 
// Description: Create a thread and give it a name.
// The MSVC version should display the thread name in the MSVS debugger.
// Confirmed for MSVS10 Express:
//   VCExpress name_np1.exe /debugexe
//   did indeed display the thread name in the trace output.
// Depends on API functions: pthread_create, pthread_join, pthread_self, pthread_getname_np, pthread_setname_np, pthread_barrier_init, pthread_barrier_wait
// 
static int PThr4wTest_NameNp1()
{
	static int washere = 0;
	static pthread_barrier_t sync;
	#if defined (__PTW32_COMPATIBILITY_BSD)
		static int seqno = 0;
	#endif

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			char buf[32];
			pthread_t self = pthread_self();
			washere = 1;
			pthread_barrier_wait(&sync);
			PTHR4W_TEST_ASSERT(pthread_getname_np(self, buf, 32) == 0);
			printf("Thread name: %s\n", buf);
			pthread_barrier_wait(&sync);
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&sync, NULL, 2) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
#if defined (__PTW32_COMPATIBILITY_BSD)
	seqno++;
	PTHR4W_TEST_ASSERT(pthread_setname_np(t, "MyThread%d", (void*)&seqno) == 0);
#elif defined (__PTW32_COMPATIBILITY_TRU64)
	PTHR4W_TEST_ASSERT(pthread_setname_np(t, "MyThread1", NULL) == 0);
#else
	PTHR4W_TEST_ASSERT(pthread_setname_np(t, "MyThread1") == 0);
#endif
	pthread_barrier_wait(&sync);
	pthread_barrier_wait(&sync);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&sync) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Description: Create a thread and give it a name.
// The MSVC version should display the thread name in the MSVS debugger.
// Confirmed for MSVS10 Express:
//   VCExpress name_np1.exe /debugexe
//   did indeed display the thread name in the trace output.
// Depends on API functions: pthread_create, pthread_join, pthread_self, pthread_attr_init, pthread_getname_np, pthread_attr_setname_np, pthread_barrier_init, pthread_barrier_wait
// 
static int PThr4wTest_NameNp2()
{
	static int washere = 0;
	static pthread_attr_t attr;
	static pthread_barrier_t sync;
	#if defined (__PTW32_COMPATIBILITY_BSD)
		static int seqno = 0;
	#endif

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			char buf[32];
			pthread_t self = pthread_self();
			washere = 1;
			pthread_barrier_wait(&sync);
			PTHR4W_TEST_ASSERT(pthread_getname_np(self, buf, 32) == 0);
			printf("Thread name: %s\n", buf);
			pthread_barrier_wait(&sync);
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr) == 0);
#if defined (__PTW32_COMPATIBILITY_BSD)
	seqno++;
	PTHR4W_TEST_ASSERT(pthread_attr_setname_np(&attr, "MyThread%d", (void*)&seqno) == 0);
#elif defined (__PTW32_COMPATIBILITY_TRU64)
	PTHR4W_TEST_ASSERT(pthread_attr_setname_np(&attr, "MyThread1", NULL) == 0);
#else
	PTHR4W_TEST_ASSERT(pthread_attr_setname_np(&attr, "MyThread1") == 0);
#endif
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&sync, NULL, 2) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, &attr, InnerBlock::ThreadFunc, NULL) == 0);
	pthread_barrier_wait(&sync);
	pthread_barrier_wait(&sync);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&sync) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_destroy(&attr) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Keep statistics for each thread.
// 
struct rwlock_thread_t {
	int thread_num;
	pthread_t thread_id;
	cpu_set_t threadCpus;
	int updates;
	int reads;
	int changed;
	int seed;
};
// 
// Read-write lock and shared data
// 
struct rwlock_data_t {
	pthread_rwlock_t lock;
	int data;
	int updates;
};
// 
// Create a simple rwlock object and then destroy it.
// Depends on API functions: pthread_rwlock_init(), pthread_rwlock_destroy()
// 
static int PThr4wTest_RwLock1()
{
	static pthread_rwlock_t rwlock = NULL;
	PTHR4W_TEST_ASSERT(rwlock == NULL);
	PTHR4W_TEST_ASSERT(pthread_rwlock_init(&rwlock, NULL) == 0);
	PTHR4W_TEST_ASSERT(rwlock != NULL);
	PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, lock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_rdlock(), pthread_rwlock_unlock()
//
static int PThr4wTest_RwLock2()
{
	static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
	PTHR4W_TEST_ASSERT(rwlock == PTHREAD_RWLOCK_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(rwlock != PTHREAD_RWLOCK_INITIALIZER);
	PTHR4W_TEST_ASSERT(rwlock != NULL);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, timed-lock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_unlock()
//
static int PThr4wTest_RwLock2t()
{
	static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
	struct timespec abstime;
	struct timespec reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(rwlock == PTHREAD_RWLOCK_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock, &abstime) == 0);
	PTHR4W_TEST_ASSERT(rwlock != PTHREAD_RWLOCK_INITIALIZER);
	PTHR4W_TEST_ASSERT(rwlock != NULL);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&rwlock) == 0);
	PTHR4W_TEST_ASSERT(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, wrlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_wrlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
static int PThr4wTest_RwLock3()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-wrlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedwrlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
static int PThr4wTest_RwLock3t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime, reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_rwlock_timedwrlock(&rwlock1, &abstime) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, rdlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_rdlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
static int PThr4wTest_RwLock4()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-rdlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock4t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime = { 0, 0 };
	struct timespec reltime = { 1, 0 };
	PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock1, pthread_win32_getabstime_np(&abstime, &reltime)) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, rdlock it, tryrdlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_rdlock(), pthread_rwlock_tryrdlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock5()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_tryrdlock(&rwlock1) == 0);
			PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-rdlock it, tryrdlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_tryrdlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock5t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_tryrdlock(&rwlock1) == 0);
			PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime, reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Check writer and reader locking
// Depends on API functions: pthread_rwlock_rdlock(), pthread_rwlock_wrlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock6()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int bankAccount = 0;

	class InnerBlock {
	public:
		static void * wrfunc(void * arg)
		{
			int ba;
			PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&rwlock1) == 0);
			Sleep(200);
			bankAccount += 10;
			ba = bankAccount;
			PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			return ((void*)(size_t)ba);
		}
		static void * rdfunc(void * arg)
		{
			int ba;
			PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&rwlock1) == 0);
			ba = bankAccount;
			PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			return ((void*)(size_t)ba);
		}
	};
	pthread_t wrt1;
	pthread_t wrt2;
	pthread_t rdt;
	void* wr1Result = (void*)0;
	void* wr2Result = (void*)0;
	void* rdResult = (void*)0;
	bankAccount = 0;
	PTHR4W_TEST_ASSERT(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(50);
	PTHR4W_TEST_ASSERT(pthread_create(&rdt, NULL, InnerBlock::rdfunc, NULL) == 0);
	Sleep(50);
	PTHR4W_TEST_ASSERT(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt1, &wr1Result) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(rdt, &rdResult) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt2, &wr2Result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)wr1Result == 10);
	PTHR4W_TEST_ASSERT((int)(size_t)rdResult == 10);
	PTHR4W_TEST_ASSERT((int)(size_t)wr2Result == 20);
	return 0;
}
// 
// Check writer and reader locking with reader timeouts
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_wrlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock6t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int bankAccount = 0;

	class InnerBlock {
	public:
		static void * wrfunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&rwlock1) == 0);
			Sleep(2000);
			bankAccount += 10;
			PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			return ((void*)(size_t)bankAccount);
		}
		static void * rdfunc(void * arg)
		{
			int ba = -1;
			struct timespec abstime;
			(void)pthread_win32_getabstime_np(&abstime, NULL);
			if((int)(size_t)arg == 1) {
				abstime.tv_sec += 1;
				PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == ETIMEDOUT);
				ba = 0;
			}
			else if((int)(size_t)arg == 2) {
				abstime.tv_sec += 3;
				PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == 0);
				ba = bankAccount;
				PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
			}
			return ((void*)(size_t)ba);
		}
	};
	pthread_t wrt1;
	pthread_t wrt2;
	pthread_t rdt1;
	pthread_t rdt2;
	void* wr1Result = (void*)0;
	void* wr2Result = (void*)0;
	void* rd1Result = (void*)0;
	void* rd2Result = (void*)0;
	bankAccount = 0;
	PTHR4W_TEST_ASSERT(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(500);
	PTHR4W_TEST_ASSERT(pthread_create(&rdt1, NULL, InnerBlock::rdfunc, (void*)(size_t)1) == 0);
	Sleep(500);
	PTHR4W_TEST_ASSERT(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(500);
	PTHR4W_TEST_ASSERT(pthread_create(&rdt2, NULL, InnerBlock::rdfunc, (void*)(size_t)2) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt1, &wr1Result) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(rdt1, &rd1Result) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt2, &wr2Result) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(rdt2, &rd2Result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)wr1Result == 10);
	PTHR4W_TEST_ASSERT((int)(size_t)rd1Result == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)wr2Result == 20);
	PTHR4W_TEST_ASSERT((int)(size_t)rd2Result == 20);
	return 0;
}
// 
// Check writer and reader timeouts.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_timedwrlock(), pthread_rwlock_unlock()
// 
static int PThr4wTest_RwLock6t2()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int bankAccount = 0;
	static struct timespec abstime;
	static struct timespec reltime = { 1, 0 };

	class InnerBlock {
	public:
		static void * wrfunc(void * arg)
		{
			int result = pthread_rwlock_timedwrlock(&rwlock1, &abstime);
			if((int)(size_t)arg == 1) {
				PTHR4W_TEST_ASSERT(result == 0);
				Sleep(2000);
				bankAccount += 10;
				PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&rwlock1) == 0);
				return ((void*)(size_t)bankAccount);
			}
			else if((int)(size_t)arg == 2) {
				PTHR4W_TEST_ASSERT(result == ETIMEDOUT);
				return ((void*)100);
			}
			return ((void*)(size_t)-1);
		}
		static void * rdfunc(void * arg)
		{
			int ba = 0;
			PTHR4W_TEST_ASSERT(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == ETIMEDOUT);
			return ((void*)(size_t)ba);
		}
	};
	pthread_t wrt1;
	pthread_t wrt2;
	pthread_t rdt;
	void* wr1Result = (void*)0;
	void* wr2Result = (void*)0;
	void* rdResult = (void*)0;
	pthread_win32_getabstime_np(&abstime, &reltime);
	bankAccount = 0;
	PTHR4W_TEST_ASSERT(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, (void*)(size_t)1) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&rdt, NULL, InnerBlock::rdfunc, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, (void*)(size_t)2) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt1, &wr1Result) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(rdt, &rdResult) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(wrt2, &wr2Result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)wr1Result == 10);
	PTHR4W_TEST_ASSERT((int)(size_t)rdResult == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)wr2Result == 100);
	return 0;
}
// 
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// 
static int PThr4wTest_RwLock7(/*int argc, char * argv[]*/)
{
	static const int THREADS    = 5;
	static const int DATASIZE   = 7;
	static const int ITERATIONS = 1000000;
	static rwlock_thread_t threads[THREADS];
	static rwlock_data_t data[DATASIZE];

	class InnerBlock {
	public:
		//
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			rwlock_thread_t * self = static_cast<rwlock_thread_t *>(arg);
			int iteration;
			int element = 0;
			int seed = self->seed;
			int interval = 1 + rand_r(&seed) % 71;
			self->changed = 0;
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				if(iteration % (ITERATIONS / 10) == 0) {
					putchar('.');
					fflush(stdout);
				}
				/*
				 * Each "self->interval" iterations, perform an
				 * update operation (write lock instead of read lock).
				 */
				if((iteration % interval) == 0) {
					PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the times, to report later.
					 */
					PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				element = (element + 1) % DATASIZE;
			}
			return NULL;
		}
	};
	int count;
	int data_count;
	int thread_updates = 0;
	int data_updates = 0;
	int seed = 1;
	__PTW32_STRUCT_TIMEB currSysTime1;
	__PTW32_STRUCT_TIMEB currSysTime2;
	/*
	 * Initialize the shared data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;
		PTHR4W_TEST_ASSERT(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
	}
	__PTW32_FTIME(&currSysTime1);
	/*
	 * Create THREADS threads to access shared data.
	 */
	for(count = 0; count < THREADS; count++) {
		threads[count].thread_num = count;
		threads[count].updates = 0;
		threads[count].reads = 0;
		threads[count].seed = 1 + rand_r(&seed) % 71;
		PTHR4W_TEST_ASSERT(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	/*
	 * Wait for all threads to complete, and collect statistics.
	 */
	for(count = 0; count < THREADS; count++) {
		PTHR4W_TEST_ASSERT(pthread_join(threads[count].thread_id, NULL) == 0);
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		if(threads[count].changed > 0) {
			printf("Thread %d found changed elements %d times\n", count, threads[count].changed);
		}
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		thread_updates += threads[count].updates;
		printf("%02d: seed %d, updates %d, reads %d\n", count, threads[count].seed, threads[count].updates, threads[count].reads);
	}
	putchar('\n');
	fflush(stdout);
	/*
	 * Collect statistics for the data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data_updates += data[data_count].updates;
		printf("data %02d: value %d, %d updates\n", data_count, data[data_count].data, data[data_count].updates);
		PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&data[data_count].lock) == 0);
	}
	printf("%d thread updates, %d data updates\n", thread_updates, data_updates);
	__PTW32_FTIME(&currSysTime2);
	printf("\nstart: %ld/%d, stop: %ld/%d, duration:%ld\n", (long)currSysTime1.time, currSysTime1.millitm,
	    (long)currSysTime2.time, currSysTime2.millitm, ((long)((currSysTime2.time*1000+currSysTime2.millitm) - (currSysTime1.time*1000+currSysTime1.millitm))));
	return 0;
}
// 
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// Use CPU affinity to compare against non-affinity rwlock7.c
// 
static int PThr4wTest_RwLock71(/*int argc, char * argv[]*/)
{
	static const int THREADS    = 5;
	static const int DATASIZE   = 7;
	static const int ITERATIONS = 1000000;

	static rwlock_thread_t threads[THREADS];
	static rwlock_data_t data[DATASIZE];
	static cpu_set_t processCpus;
	static int cpu_count;

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			rwlock_thread_t * self = static_cast<rwlock_thread_t *>(arg);
			int iteration;
			int element = 0;
			int seed = self->seed;
			int interval = 1 + rand_r(&seed) % 71;
			/*
			 * Set each thread to a fixed (different if possible) cpu.
			 */
			CPU_ZERO(&self->threadCpus);
			CPU_SET(self->thread_num%cpu_count, &self->threadCpus);
			PTHR4W_TEST_ASSERT(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &self->threadCpus) == 0);
			self->changed = 0;
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				if(iteration % (ITERATIONS / 10) == 0) {
					putchar('.');
					fflush(stdout);
				}
				/*
				 * Each "self->interval" iterations, perform an
				 * update operation (write lock instead of read lock).
				 */
				if((iteration % interval) == 0) {
					PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the times, to report later.
					 */
					PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				element = (element + 1) % DATASIZE;
			}
			return NULL;
		}
	};
	int count;
	int data_count;
	int thread_updates = 0;
	int data_updates = 0;
	int seed = 1;
	pthread_t self = pthread_self();
	__PTW32_STRUCT_TIMEB currSysTime1;
	__PTW32_STRUCT_TIMEB currSysTime2;
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	PTHR4W_TEST_ASSERT((cpu_count = CPU_COUNT(&processCpus)) > 0);
	printf("CPUs: %d\n", cpu_count);
	/*
	 * Initialize the shared data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;
		PTHR4W_TEST_ASSERT(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
	}
	__PTW32_FTIME(&currSysTime1);
	/*
	 * Create THREADS threads to access shared data.
	 */
	for(count = 0; count < THREADS; count++) {
		threads[count].thread_num = count;
		threads[count].updates = 0;
		threads[count].reads = 0;
		threads[count].seed = 1 + rand_r(&seed) % 71;
		PTHR4W_TEST_ASSERT(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	/*
	 * Wait for all threads to complete, and collect statistics.
	 */
	for(count = 0; count < THREADS; count++) {
		PTHR4W_TEST_ASSERT(pthread_join(threads[count].thread_id, NULL) == 0);
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		if(threads[count].changed > 0) {
			printf("Thread %d found changed elements %d times\n", count, threads[count].changed);
		}
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		thread_updates += threads[count].updates;
		printf("%02d: seed %d, updates %d, reads %d, cpu %d\n", count, threads[count].seed,
		    threads[count].updates, threads[count].reads, threads[count].thread_num%cpu_count);
	}
	putchar('\n');
	fflush(stdout);
	/*
	 * Collect statistics for the data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data_updates += data[data_count].updates;
		printf("data %02d: value %d, %d updates\n", data_count, data[data_count].data, data[data_count].updates);
		PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&data[data_count].lock) == 0);
	}
	printf("%d thread updates, %d data updates\n", thread_updates, data_updates);
	__PTW32_FTIME(&currSysTime2);
	printf("\nstart: %ld/%d, stop: %ld/%d, duration:%ld\n", (long)currSysTime1.time, currSysTime1.millitm,
	    (long)currSysTime2.time, currSysTime2.millitm, ((long)((currSysTime2.time*1000+currSysTime2.millitm) - (currSysTime1.time*1000+currSysTime1.millitm))));
	return 0;
}
// 
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// Yield during each access to exercise lock contention code paths
// more than rwlock7.c does (particularly on uni-processor systems).
// 
static int PThr4wTest_RwLock8(/*int argc, char * argv[]*/)
{
	static const int THREADS    = 5;
	static const int DATASIZE   = 7;
	static const int ITERATIONS = 100000;

	static rwlock_thread_t threads[THREADS];
	static rwlock_data_t data[DATASIZE];

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		// 
		static void * thread_routine(void * arg)
		{
			rwlock_thread_t * self = static_cast<rwlock_thread_t *>(arg);
			int iteration;
			int element = 0;
			int seed = self->seed;
			int interval = 1 + rand_r(&seed) % 71;
			self->changed = 0;
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				if(iteration % (ITERATIONS / 10) == 0) {
					putchar('.');
					fflush(stdout);
				}
				/*
				 * Each "self->interval" iterations, perform an
				 * update operation (write lock instead of read lock).
				 */
				if((iteration % interval) == 0) {
					PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					sched_yield();
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the
					 * times, to report later.
					 */
					PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					sched_yield();
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				element = (element + 1) % DATASIZE;
			}
			return NULL;
		}
	};
	int count;
	int data_count;
	int thread_updates = 0;
	int data_updates = 0;
	int seed = 1;
	__PTW32_STRUCT_TIMEB currSysTime1;
	__PTW32_STRUCT_TIMEB currSysTime2;
	// 
	// Initialize the shared data.
	// 
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;
		PTHR4W_TEST_ASSERT(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
	}
	__PTW32_FTIME(&currSysTime1);
	// 
	// Create THREADS threads to access shared data.
	// 
	for(count = 0; count < THREADS; count++) {
		threads[count].thread_num = count;
		threads[count].updates = 0;
		threads[count].reads = 0;
		threads[count].seed = 1 + rand_r(&seed) % 71;
		PTHR4W_TEST_ASSERT(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	// 
	// Wait for all threads to complete, and collect statistics.
	// 
	for(count = 0; count < THREADS; count++) {
		PTHR4W_TEST_ASSERT(pthread_join(threads[count].thread_id, NULL) == 0);
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		if(threads[count].changed > 0) {
			printf("Thread %d found changed elements %d times\n", count, threads[count].changed);
		}
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		thread_updates += threads[count].updates;
		printf("%02d: seed %d, updates %d, reads %d\n", count, threads[count].seed, threads[count].updates, threads[count].reads);
	}
	putchar('\n');
	fflush(stdout);
	/*
	 * Collect statistics for the data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data_updates += data[data_count].updates;
		printf("data %02d: value %d, %d updates\n", data_count, data[data_count].data, data[data_count].updates);
		PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&data[data_count].lock) == 0);
	}
	printf("%d thread updates, %d data updates\n", thread_updates, data_updates);
	__PTW32_FTIME(&currSysTime2);
	printf("\nstart: %ld/%d, stop: %ld/%d, duration:%ld\n", (long)currSysTime1.time, currSysTime1.millitm,
	    (long)currSysTime2.time, currSysTime2.millitm, ((long)((currSysTime2.time*1000+currSysTime2.millitm) - (currSysTime1.time*1000+currSysTime1.millitm))));
	return 0;
}
// 
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// 
// Yield during each access to exercise lock contention code paths
// more than rwlock7.c does (particularly on uni-processor systems).
// 
// Use CPU affinity to compare against non-affinity rwlock8.c
// 
static int PThr4wTest_RwLock81(/*int argc, char * argv[]*/)
{
	static const int THREADS    = 5;
	static const int DATASIZE   = 7;
	static const int ITERATIONS = 100000;

	static rwlock_thread_t threads[THREADS];
	static rwlock_data_t data[DATASIZE];
	static cpu_set_t processCpus;
	static int cpu_count;

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		// 
		static void * thread_routine(void * arg)
		{
			rwlock_thread_t * self = static_cast<rwlock_thread_t *>(arg);
			int iteration;
			int element = 0;
			int seed = self->seed;
			int interval = 1 + rand_r(&seed) % 71;
			// 
			// Set each thread to a fixed (different if possible) cpu.
			// 
			CPU_ZERO(&self->threadCpus);
			CPU_SET(self->thread_num%cpu_count, &self->threadCpus);
			PTHR4W_TEST_ASSERT(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &self->threadCpus) == 0);
			self->changed = 0;
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				if(iteration % (ITERATIONS / 10) == 0) {
					putchar('.');
					fflush(stdout);
				}
				// 
				// Each "self->interval" iterations, perform an
				// update operation (write lock instead of read lock).
				// 
				if((iteration % interval) == 0) {
					PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					sched_yield();
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					// 
					// Look at the current data element to see whether
					// the current thread last updated it. Count the times, to report later.
					// 
					PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					sched_yield();
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				element = (element + 1) % DATASIZE;
			}
			return NULL;
		}
	};
	int count;
	int data_count;
	int thread_updates = 0;
	int data_updates = 0;
	int seed = 1;
	pthread_t self = pthread_self();
	__PTW32_STRUCT_TIMEB currSysTime1;
	__PTW32_STRUCT_TIMEB currSysTime2;
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	PTHR4W_TEST_ASSERT((cpu_count = CPU_COUNT(&processCpus)) > 0);
	printf("CPUs: %d\n", cpu_count);
	// 
	// Initialize the shared data.
	// 
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;

		PTHR4W_TEST_ASSERT(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
	}
	__PTW32_FTIME(&currSysTime1);
	// 
	// Create THREADS threads to access shared data.
	// 
	for(count = 0; count < THREADS; count++) {
		threads[count].thread_num = count;
		threads[count].updates = 0;
		threads[count].reads = 0;
		threads[count].seed = 1 + rand_r(&seed) % 71;
		PTHR4W_TEST_ASSERT(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	// 
	// Wait for all threads to complete, and collect statistics.
	// 
	for(count = 0; count < THREADS; count++) {
		PTHR4W_TEST_ASSERT(pthread_join(threads[count].thread_id, NULL) == 0);
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		if(threads[count].changed > 0) {
			printf("Thread %d found changed elements %d times\n", count, threads[count].changed);
		}
	}
	putchar('\n');
	fflush(stdout);
	for(count = 0; count < THREADS; count++) {
		thread_updates += threads[count].updates;
		printf("%02d: seed %d, updates %d, reads %d, cpu %d\n", count, threads[count].seed,
		    threads[count].updates, threads[count].reads, threads[count].thread_num%cpu_count);
	}
	putchar('\n');
	fflush(stdout);
	/*
	 * Collect statistics for the data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data_updates += data[data_count].updates;
		printf("data %02d: value %d, %d updates\n", data_count, data[data_count].data, data[data_count].updates);
		PTHR4W_TEST_ASSERT(pthread_rwlock_destroy(&data[data_count].lock) == 0);
	}
	printf("%d thread updates, %d data updates\n", thread_updates, data_updates);
	__PTW32_FTIME(&currSysTime2);
	printf("\nstart: %ld/%d, stop: %ld/%d, duration:%ld\n",
	    (long)currSysTime1.time, currSysTime1.millitm,
	    (long)currSysTime2.time, currSysTime2.millitm,
	    ((long)((currSysTime2.time*1000+currSysTime2.millitm) -
	    (currSysTime1.time*1000+currSysTime1.millitm))));
	return 0;
}
// 
// Same test as rwlock7.c but loop two or more times reinitialising the library
// each time, to test reinitialisation. We use a rwlock test because rw locks
// use CVs, mutexes and semaphores internally.
// 
// rwlock7.c description:
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// 
static int PThr4wTest_Reinit1()
{
	static const int THREADS    = 5;
	static const int DATASIZE   = 7;
	static const int ITERATIONS = 1000000;
	static const int LOOPS      = 3;

	static rwlock_thread_t threads[THREADS];
	static rwlock_data_t data[DATASIZE];

	class InnerBlock {
	public:
		//
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			rwlock_thread_t * self = static_cast<rwlock_thread_t *>(arg);
			int iteration;
			int element = 0;
			int seed = self->seed;
			int interval = 1 + rand_r(&seed) % 71;
			self->changed = 0;
			PTHR4W_TEST_ASSERT(pthread_getunique_np(self->thread_id) == (unsigned __int64)(self->thread_num + 2));
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				/*
				 * Each "self->interval" iterations, perform an
				 * update operation (write lock instead of read lock).
				 */
				if((iteration % interval) == 0) {
					PTHR4W_TEST_ASSERT(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the
					 * times, to report later.
					 */
					PTHR4W_TEST_ASSERT(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					PTHR4W_TEST_ASSERT(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				element = (element + 1) % DATASIZE;
			}
			return NULL;
		}
	};
	int count;
	int data_count;
	int reinit_count;
	int seed = 1;
	for(reinit_count = 0; reinit_count < LOOPS; reinit_count++) {
		// 
		// Initialize the shared data.
		// 
		for(data_count = 0; data_count < DATASIZE; data_count++) {
			data[data_count].data = 0;
			data[data_count].updates = 0;
			PTHR4W_TEST_ASSERT(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
		}
		// 
		// Create THREADS threads to access shared data.
		// 
		for(count = 0; count < THREADS; count++) {
			threads[count].thread_num = count;
			threads[count].updates = 0;
			threads[count].reads = 0;
			threads[count].seed = 1 + rand_r(&seed) % 71;
			PTHR4W_TEST_ASSERT(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
		}
		// 
		// Wait for all threads to complete, and collect statistics.
		// 
		for(count = 0; count < THREADS; count++) {
			PTHR4W_TEST_ASSERT(pthread_join(threads[count].thread_id, NULL) == 0);
		}
		pthread_win32_process_detach_np();
		pthread_win32_process_attach_np();
	}
	return 0;
}
//
// Create a simple mutex object, lock it, and then unlock it again.
// This is the simplest test of the pthread mutex family that we can do.
// 
// Depends on API functions: pthread_mutex_init(),  pthread_mutex_lock(), pthread_mutex_unlock(), pthread_mutex_destroy()
// 
static int PThr4wTest_Mutex1()
{
	static pthread_mutex_t mutex = NULL;
	PTHR4W_TEST_ASSERT(mutex == NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	return 0;
}
//
// As for mutex1.c but with type set to PTHREAD_MUTEX_ERRORCHECK.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
static int PThr4wTest_Mutex1e()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// As for mutex1.c but with type set to PTHREAD_MUTEX_NORMAL.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
static int PThr4wTest_Mutex1n()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// As for mutex1.c but with type set to PTHREAD_MUTEX_RECURSIVE.
// Create a simple mutex object, lock it, unlock it, then destroy it.
// This is the simplest test of the pthread mutex family that we can do.
// Depends on API functions: pthread_mutexattr_settype(), pthread_mutex_init(), pthread_mutex_destroy()
// 
static int PThr4wTest_Mutex1r()
{
	static pthread_mutex_t mutex = NULL;
	static pthread_mutexattr_t mxAttr;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex2()
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex != PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex2e()
{
	static pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex != PTHREAD_ERRORCHECK_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex2r()
{
	static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex != PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(mutex != NULL);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(mutex == NULL);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex3()
{
	static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
	static int washere = 0;
	
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex3e()
{
	static pthread_mutex_t mutex1 = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Declare a static mutex object, lock it, trylock it, and then unlock it again.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex3r()
{
	static pthread_mutex_t mutex1 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
// 
// Thread A locks mutex - thread B tries to unlock.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex4()
{
	static int wasHere = 0;
	static pthread_mutex_t mutex1;

	class InnerBlock {
	public:
		static void * unlocker(void * arg)
		{
			int expectedResult = (int)(size_t)arg;
			wasHere++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == expectedResult);
			wasHere++;
			return NULL;
		}
	};
	pthread_t t;
	pthread_mutexattr_t ma;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(ma)
	wasHere = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_DEFAULT) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex1, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)(IS_ROBUST ? EPERM : 0)) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(wasHere == 2);

	wasHere = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex1, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)(IS_ROBUST ? EPERM : 0)) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(wasHere == 2);

	wasHere = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex1, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)EPERM) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(wasHere == 2);

	wasHere = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex1, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)(size_t)EPERM) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(wasHere == 2);
	END_MUTEX_STALLED_ROBUST(ma)
	return 0;
}
//
// Confirm the equality/inequality of the various mutex types, and the default not-set value.
//
static int PThr4wTest_Mutex5()
{
	static pthread_mutexattr_t mxAttr;
	static int _optimiseFoil; // Prevent optimiser from removing dead or obvious asserts. 
	#define FOIL(x) (_optimiseFoil = x)
	int mxType = -1;
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_DEFAULT) == PTHREAD_MUTEX_NORMAL);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_DEFAULT) != PTHREAD_MUTEX_ERRORCHECK);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_DEFAULT) != PTHREAD_MUTEX_RECURSIVE);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_RECURSIVE) != PTHREAD_MUTEX_ERRORCHECK);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_NORMAL) == PTHREAD_MUTEX_FAST_NP);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_RECURSIVE) == PTHREAD_MUTEX_RECURSIVE_NP);
	PTHR4W_TEST_ASSERT(FOIL(PTHREAD_MUTEX_ERRORCHECK) == PTHREAD_MUTEX_ERRORCHECK_NP);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_NORMAL);
	return 0;
	#undef FOIL
}
// 
// Test the default (type not set) mutex type.
// Should be the same as PTHREAD_MUTEX_NORMAL.
// Thread locks mutex twice (recursive lock).
// Locking thread should deadlock on second attempt.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			/* Should wait here (deadlocked) */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 1) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 1);
	// Should succeed even though we don't own the lock because FAST mutexes don't check ownership.
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	while(lockCount < 2) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 2);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_ERRORCHECK mutex type.
// Thread locks mutex twice (recursive lock).
// This should fail with an EDEADLK error.
// The second unlock attempt should fail with an EPERM error.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6e()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == EDEADLK);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_ERRORCHECK);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&mxAttr) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_ERRORCHECK static mutex type.
// Thread locks mutex twice (recursive lock).
// This should fail with an EDEADLK error.
// The second unlock attempt should fail with an EPERM error.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype()
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6es()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == EDEADLK);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_ERRORCHECK_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_NORMAL mutex type.
// Thread locks mutex twice (recursive lock).
// The thread should deadlock.
// Depends on API functions: pthread_create(), pthread_mutexattr_init(), pthread_mutexattr_settype(), pthread_mutexattr_gettype(), pthread_mutex_init()
//   pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6n()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			// Should wait here (deadlocked) 
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_NORMAL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 1) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 1);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == (IS_ROBUST ? EPERM : 0));
	while(lockCount < (IS_ROBUST ? 1 : 2)) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == (IS_ROBUST ? 1 : 2));
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// Tests PTHREAD_MUTEX_RECURSIVE mutex type.
// Thread locks mutex twice (recursive lock).
// Both locks and unlocks should succeed.
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6r()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_RECURSIVE);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&mxAttr) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_RECURSIVE static mutex type.
// Thread locks mutex twice (recursive lock).
// Both locks and unlocks should succeed.
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype()
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex6rs()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_RECURSIVE_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Test the default (type not set) static mutex type.
// Should be the same as PTHREAD_MUTEX_NORMAL.
// Thread locks mutex twice (recursive lock).
// Locking thread should deadlock on second attempt.
// 
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
//
static int PThr4wTest_Mutex6s()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			/* Should wait here (deadlocked) */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(mutex == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 1) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 1);
	/*
	 * Should succeed even though we don't own the lock
	 * because FAST mutexes don't check ownership.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	while(lockCount < 2) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 2);
	return 0;
}
// 
// Test the default (type not set) mutex type.
// Should be the same as PTHREAD_MUTEX_NORMAL.
// Thread locks then trylocks mutex (attempted recursive lock).
// The thread should lock first time and EBUSY second time.
// 
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_trylock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex7()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex) == EBUSY);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 2) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 2);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_ERRORCHECK mutex type.
// Thread locks and then trylocks mutex (attempted recursive lock).
// Trylock should fail with an EBUSY error.
// The second unlock attempt should fail with an EPERM error.
// 
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(), 
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex7e()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex) == EBUSY);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_ERRORCHECK);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&mxAttr) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_NORMAL mutex type.
// Thread locks then trylocks mutex (attempted recursive lock).
// The thread should lock first time and EBUSY second time.
// Depends on API functions: pthread_create(), pthread_mutexattr_init(), pthread_mutexattr_settype(), pthread_mutexattr_gettype(), pthread_mutex_init(),
// pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex7n()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex) == EBUSY);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_NORMAL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 2) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 2);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&mxAttr) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_RECURSIVE mutex type.
// Thread locks mutex then trylocks mutex (recursive lock twice).
// Both locks and unlocks should succeed.
// Depends on API functions: pthread_create(), pthread_join(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex7r()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_trylock(&mutex) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return (void*)555;
		}
	};
	pthread_t t;
	void* result = (void*)0;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_RECURSIVE);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result == 555);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&mxAttr) == 0);
	return 0;
}
// 
// Test the default (type not set) mutex type exercising timedlock.
// Thread locks mutex, another thread timedlocks the mutex.
// Timed thread should timeout.
// Depends on API functions: pthread_mutex_lock(), pthread_mutex_timedlock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex8()
{
	static int lockCount = 0;
	static pthread_mutex_t mutex;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			struct timespec abstime, reltime = { 1, 0 };
			(void)pthread_win32_getabstime_np(&abstime, &reltime);
			PTHR4W_TEST_ASSERT(pthread_mutex_timedlock(&mutex, &abstime) == ETIMEDOUT);
			lockCount++;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 1) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 1);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	return 0;
}
// 
// Tests PTHREAD_MUTEX_ERRORCHECK mutex type exercising timedlock.
// Thread locks mutex, another thread timedlocks the mutex.
// Timed thread should timeout.
// 
// Depends on API functions: pthread_create(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_timedlock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex8e()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			struct timespec abstime, reltime = { 1, 0 };
			(void)pthread_win32_getabstime_np(&abstime, &reltime);
			PTHR4W_TEST_ASSERT(pthread_mutex_timedlock(&mutex, &abstime) == ETIMEDOUT);
			lockCount++;
			return 0;
		}
	};
	pthread_t t;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_ERRORCHECK);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(lockCount == 1);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// Tests PTHREAD_MUTEX_NORMAL mutex type exercising timedlock.
// Thread locks mutex, another thread timedlocks the mutex.
// Timed thread should timeout.
// Depends on API functions: pthread_create(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_timedlock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex8n()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			struct timespec abstime, reltime = { 1, 0 };
			(void)pthread_win32_getabstime_np(&abstime, &reltime);
			PTHR4W_TEST_ASSERT(pthread_mutex_timedlock(&mutex, &abstime) == ETIMEDOUT);
			lockCount++;
			return 0;
		}
	};
	pthread_t t;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_NORMAL);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	while(lockCount < 1) {
		Sleep(1);
	}
	PTHR4W_TEST_ASSERT(lockCount == 1);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
// 
// Tests PTHREAD_MUTEX_RECURSIVE mutex type exercising timedlock.
// Thread locks mutex, another thread timedlocks the mutex.
// Timed thread should timeout.
// Depends on API functions: pthread_create(), pthread_mutexattr_init(), pthread_mutexattr_destroy(), pthread_mutexattr_settype(),
//   pthread_mutexattr_gettype(), pthread_mutex_init(), pthread_mutex_destroy(), pthread_mutex_lock(), pthread_mutex_timedlock(), pthread_mutex_unlock()
// 
static int PThr4wTest_Mutex8r()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	static pthread_mutexattr_t mxAttr;

	class InnerBlock {
	public:
		static void * locker(void * arg)
		{
			struct timespec abstime, reltime = { 1, 0 };
			(void)pthread_win32_getabstime_np(&abstime, &reltime);
			PTHR4W_TEST_ASSERT(pthread_mutex_timedlock(&mutex, &abstime) == ETIMEDOUT);
			lockCount++;
			return 0;
		}
	};
	pthread_t t;
	int mxType = -1;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&mxAttr) == 0);
	BEGIN_MUTEX_STALLED_ROBUST(mxAttr)
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&mxAttr, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_gettype(&mxAttr, &mxType) == 0);
	PTHR4W_TEST_ASSERT(mxType == PTHREAD_MUTEX_RECURSIVE);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &mxAttr) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::locker, NULL) == 0);
	Sleep(2000);
	PTHR4W_TEST_ASSERT(lockCount == 1);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	END_MUTEX_STALLED_ROBUST(mxAttr)
	return 0;
}
//
// Descr: Create a simple spinlock object, lock it, and then unlock it again.
//   This is the simplest test of the pthread mutex family that we can do
//
static int PThr4wTest_Spin1()
{
	static pthread_spinlock_t lock;
	PTHR4W_TEST_ASSERT(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_lock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_unlock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_destroy(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_lock(&lock) == EINVAL);
	return 0;
}
//
// Descr: Declare a spinlock object, lock it, trylock it, and then unlock it again.
//
static int PThr4wTest_Spin2()
{
	static pthread_spinlock_t lock = NULL;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_spin_trylock(&lock) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_lock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_unlock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_destroy(&lock) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
//
// Thread A locks spin - thread B tries to unlock. This should succeed, but it's undefined behaviour.
//
static int PThr4wTest_Spin3()
{
	static int wasHere = 0;
	static pthread_spinlock_t spin;

	class InnerBlock {
	public:
		static void * unlocker(void * arg)
		{
			int expectedResult = (int)(size_t)arg;
			wasHere++;
			PTHR4W_TEST_ASSERT(pthread_spin_unlock(&spin) == expectedResult);
			wasHere++;
			return NULL;
		}
	};
	pthread_t t;
	wasHere = 0;
	PTHR4W_TEST_ASSERT(pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_lock(&spin) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::unlocker, (void*)0) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	// 
	// Our spinlocks don't record the owner thread so any thread can unlock the spinlock,
	// but nor is it an error for any thread to unlock a spinlock that is not locked.
	// 
	PTHR4W_TEST_ASSERT(pthread_spin_unlock(&spin) == 0);
	PTHR4W_TEST_ASSERT(pthread_spin_destroy(&spin) == 0);
	PTHR4W_TEST_ASSERT(wasHere == 2);
	return 0;
}

static int PThr4wTest_Spin4()
{
	static pthread_spinlock_t lock = PTHREAD_SPINLOCK_INITIALIZER;
	static __PTW32_STRUCT_TIMEB currSysTimeStart;
	static __PTW32_STRUCT_TIMEB currSysTimeStop;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			__PTW32_FTIME(&currSysTimeStart);
			washere = 1;
			PTHR4W_TEST_ASSERT(pthread_spin_lock(&lock) == 0);
			PTHR4W_TEST_ASSERT(pthread_spin_unlock(&lock) == 0);
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
	PTHR4W_TEST_ASSERT(pthread_spin_lock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	while(washere == 0) {
		sched_yield();
	}
	do {
		sched_yield();
		__PTW32_FTIME(&sysTime);
	} while(GetDurationMilliSecs(currSysTimeStart, sysTime) <= 1000);
	PTHR4W_TEST_ASSERT(pthread_spin_unlock(&lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
	PTHR4W_TEST_ASSERT((int)(size_t)result > 1000);
	PTHR4W_TEST_ASSERT(pthread_spin_destroy(&lock) == 0);
	PTHR4W_TEST_ASSERT(washere == 1);
	return 0;
}
//
// Test for pthread_exit()
//
static int PThr4wTest_Exit1()
{
	// A simple test first. 
	pthread_exit((void*)0);
	return 1; // Not reached 
}

static int PThr4wTest_Exit2()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int failed = (int)arg;
			pthread_exit(arg);
			// Never reached. 
			// Trick gcc compiler into not issuing a warning here
			PTHR4W_TEST_ASSERT(failed - (int)arg);
			return NULL;
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::ThreadFunc, NULL) == 0);
	Sleep(100);
	return 0;
}

static int PThr4wTest_Exit3()
{
	class InnerBlock {
	public:
		static void * ThreadFunc(void * arg)
		{
			int failed = (int)arg;
			pthread_exit(arg);
			// Never reached.
			/*
			 * PTHR4W_TEST_ASSERT(0) in a way to prevent warning or optimising away.
			 */
			PTHR4W_TEST_ASSERT(failed - (int)arg);
			return NULL;
		}
	};
	pthread_t id[4];
	// Create a few threads and then exit. 
	for(int i = 0; i < SIZEOFARRAY(id); i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&id[i], NULL, InnerBlock::ThreadFunc, (void*)(size_t)i) == 0);
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
static int PThr4wTest_Exit4()
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
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
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
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		PTHR4W_TEST_ASSERT(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		// Can't get a result code.
		result = 1;
#endif
		fail = (result != 1);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
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
static int PThr4wTest_Exit5()
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
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			PTHR4W_TEST_ASSERT((bag->self = pthread_self()).p != NULL);
			PTHR4W_TEST_ASSERT(pthread_kill(bag->self, 0) == 0);
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
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		PTHR4W_TEST_ASSERT(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		result = 1; // Can't get a result code.
#endif
		PTHR4W_TEST_ASSERT(threadbag[i].self.p != NULL);
		PTHR4W_TEST_ASSERT(pthread_kill(threadbag[i].self, 0) == ESRCH);
		fail = (result != 1);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}

static int PThr4wTest_Exit6()
{
	static pthread_key_t key;
	static int where;

	class InnerBlock {
	public:
		static unsigned __stdcall start_routine(void * arg)
		{
			int * val = (int *)SAlloc::M(4);
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
			SAlloc::F(arg);
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
// 
// Basic test of CPU_*() support routines.
// Have the process switch CPUs.
// Have the thread switch CPUs.
// Test thread CPU affinity setting.
// Test thread CPU affinity inheritance.
// Test thread CPU affinity from thread attributes.
// 
static int PThr4wTest_Affinity1()
{
	uint cpu;
	cpu_set_t newmask;
	cpu_set_t src1mask;
	cpu_set_t src2mask;
	cpu_set_t src3mask;
	CPU_ZERO(&newmask);
	CPU_ZERO(&src1mask);
	memzero(&src2mask, sizeof(cpu_set_t));
	PTHR4W_TEST_ASSERT(memcmp(&src1mask, &src2mask, sizeof(cpu_set_t)) == 0);
	PTHR4W_TEST_ASSERT(CPU_EQUAL(&src1mask, &src2mask));
	PTHR4W_TEST_ASSERT(CPU_COUNT(&src1mask) == 0);
	CPU_ZERO(&src1mask);
	CPU_ZERO(&src2mask);
	CPU_ZERO(&src3mask);
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &src1mask);                                /* 0b01010101010101010101010101010101 */
	}
	for(cpu = 0; cpu < sizeof(cpu_set_t)*4; cpu++) {
		CPU_SET(cpu, &src2mask);                                /* 0b00000000000000001111111111111111 */
	}
	for(cpu = sizeof(cpu_set_t)*4; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &src2mask);                                /* 0b01010101010101011111111111111111 */
	}
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &src3mask);                                /* 0b01010101010101010101010101010101 */
	}
	PTHR4W_TEST_ASSERT(CPU_COUNT(&src1mask) == (sizeof(cpu_set_t)*4));
	PTHR4W_TEST_ASSERT(CPU_COUNT(&src2mask) == ((sizeof(cpu_set_t)*4 + (sizeof(cpu_set_t)*2))));
	PTHR4W_TEST_ASSERT(CPU_COUNT(&src3mask) == (sizeof(cpu_set_t)*4));
	CPU_SET(0, &newmask);
	CPU_SET(1, &newmask);
	CPU_SET(3, &newmask);
	PTHR4W_TEST_ASSERT(CPU_ISSET(1, &newmask));
	CPU_CLR(1, &newmask);
	PTHR4W_TEST_ASSERT(!CPU_ISSET(1, &newmask));
	CPU_OR(&newmask, &src1mask, &src2mask);
	PTHR4W_TEST_ASSERT(CPU_EQUAL(&newmask, &src2mask));
	CPU_AND(&newmask, &src1mask, &src2mask);
	PTHR4W_TEST_ASSERT(CPU_EQUAL(&newmask, &src1mask));
	CPU_XOR(&newmask, &src1mask, &src3mask);
	memzero(&src2mask, sizeof(cpu_set_t));
	PTHR4W_TEST_ASSERT(memcmp(&newmask, &src2mask, sizeof(cpu_set_t)) == 0);
	/*
	* Need to confirm the bitwise logical right-shift in CpuCount().
	* i.e. zeros inserted into MSB on shift because cpu_set_t is unsigned.
	*/
	CPU_ZERO(&src1mask);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &src1mask);                                /* 0b10101010101010101010101010101010 */
	}
	PTHR4W_TEST_ASSERT(CPU_ISSET(sizeof(cpu_set_t)*8-1, &src1mask));
	PTHR4W_TEST_ASSERT(CPU_COUNT(&src1mask) == (sizeof(cpu_set_t)*4));
	return 0;
}

static int PThr4wTest_Affinity2()
{
	uint cpu;
	int result;
	cpu_set_t newmask;
	cpu_set_t mask;
	cpu_set_t switchmask;
	cpu_set_t flipmask;
	CPU_ZERO(&mask);
	CPU_ZERO(&switchmask);
	CPU_ZERO(&flipmask);
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &switchmask);                      /* 0b01010101010101010101010101010101 */
	}
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu++) {
		CPU_SET(cpu, &flipmask);                                /* 0b11111111111111111111111111111111 */
	}
	PTHR4W_TEST_ASSERT(sched_getaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
	PTHR4W_TEST_ASSERT(!CPU_EQUAL(&newmask, &mask));
	result = sched_setaffinity(0, sizeof(cpu_set_t), &newmask);
	if(result != 0) {
		int err =
#if defined (__PTW32_USES_SEPARATE_CRT)
		    GetLastError();
#else
		    errno;
#endif
		PTHR4W_TEST_ASSERT(err != ESRCH);
		PTHR4W_TEST_ASSERT(err != EFAULT);
		PTHR4W_TEST_ASSERT(err != EPERM);
		PTHR4W_TEST_ASSERT(err != EINVAL);
		PTHR4W_TEST_ASSERT(err != EAGAIN);
		PTHR4W_TEST_ASSERT(err == ENOSYS);
		PTHR4W_TEST_ASSERT(CPU_COUNT(&mask) == 1);
	}
	else {
		if(CPU_COUNT(&mask) > 1) {
			CPU_AND(&newmask, &mask, &switchmask); /* Remove every other CPU */
			PTHR4W_TEST_ASSERT(sched_setaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
			PTHR4W_TEST_ASSERT(sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0);
			CPU_XOR(&newmask, &mask, &flipmask); /* Switch to all alternative CPUs */
			PTHR4W_TEST_ASSERT(sched_setaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
			PTHR4W_TEST_ASSERT(sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0);
			PTHR4W_TEST_ASSERT(!CPU_EQUAL(&newmask, &mask));
		}
	}
	return 0;
}

static int PThr4wTest_Affinity3()
{
	int result;
	uint cpu;
	cpu_set_t newmask;
	cpu_set_t processCpus;
	cpu_set_t mask;
	cpu_set_t switchmask;
	cpu_set_t flipmask;
	pthread_t self = pthread_self();
	CPU_ZERO(&mask);
	CPU_ZERO(&switchmask);
	CPU_ZERO(&flipmask);
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	printf("This thread has a starting affinity with %d CPUs\n", CPU_COUNT(&processCpus));
	PTHR4W_TEST_ASSERT(!CPU_EQUAL(&mask, &processCpus));
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &switchmask);                      /* 0b01010101010101010101010101010101 */
	}
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu++) {
		CPU_SET(cpu, &flipmask);                                /* 0b11111111111111111111111111111111 */
	}
	result = pthread_setaffinity_np(self, sizeof(cpu_set_t), &processCpus);
	if(result != 0) {
		PTHR4W_TEST_ASSERT(result != ESRCH);
		PTHR4W_TEST_ASSERT(result != EFAULT);
		PTHR4W_TEST_ASSERT(result != EPERM);
		PTHR4W_TEST_ASSERT(result != EINVAL);
		PTHR4W_TEST_ASSERT(result != EAGAIN);
		PTHR4W_TEST_ASSERT(result == ENOSYS);
		PTHR4W_TEST_ASSERT(CPU_COUNT(&mask) == 1);
	}
	else {
		if(CPU_COUNT(&mask) > 1) {
			CPU_AND(&newmask, &processCpus, &switchmask); /* Remove every other CPU */
			PTHR4W_TEST_ASSERT(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
			PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
			PTHR4W_TEST_ASSERT(CPU_EQUAL(&mask, &newmask));
			CPU_XOR(&newmask, &mask, &flipmask); /* Switch to all alternative CPUs */
			PTHR4W_TEST_ASSERT(!CPU_EQUAL(&mask, &newmask));
			PTHR4W_TEST_ASSERT(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
			PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
			PTHR4W_TEST_ASSERT(CPU_EQUAL(&mask, &newmask));
		}
	}
	return 0;
}

static int PThr4wTest_Affinity4()
{
	uint cpu;
	cpu_set_t threadCpus;
	DWORD_PTR vThreadMask;
	cpu_set_t keepCpus;
	pthread_t self = pthread_self();
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	CPU_ZERO(&keepCpus);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &keepCpus); // 0b10101010101010101010101010101010
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), (*(PDWORD_PTR)&threadCpus) /* Violating Opacity*/);
		PTHR4W_TEST_ASSERT(pthread_setaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), vThreadMask);
		PTHR4W_TEST_ASSERT(vThreadMask != 0);
		PTHR4W_TEST_ASSERT(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
	}
	return 0;
}

static int PThr4wTest_Affinity5()
{
	typedef union {
		// Violates opacity 
		cpu_set_t cpuset;
		ulong bits;  /* To stop GCC complaining about %lx args to printf */
	} cpuset_to_ulint;
	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			HANDLE threadH = GetCurrentThread();
			cpu_set_t * parentCpus = (cpu_set_t*)arg;
			cpu_set_t threadCpus;
			DWORD_PTR vThreadMask;
			cpuset_to_ulint a, b;
			PTHR4W_TEST_ASSERT(pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &threadCpus) == 0);
			PTHR4W_TEST_ASSERT(CPU_EQUAL(parentCpus, &threadCpus));
			vThreadMask = SetThreadAffinityMask(threadH, (*(PDWORD_PTR)&threadCpus) /* Violating Opacity */);
			PTHR4W_TEST_ASSERT(vThreadMask != 0);
			PTHR4W_TEST_ASSERT(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
			a.cpuset = *parentCpus;
			b.cpuset = threadCpus;
			/* Violates opacity */
			printf("CPU affinity: Parent/Thread = 0x%lx/0x%lx\n", a.bits, b.bits);
			return 0;
		}
	};
	uint cpu;
	pthread_t tid;
	cpu_set_t threadCpus;
	DWORD_PTR vThreadMask;
	cpu_set_t keepCpus;
	pthread_t self = pthread_self();
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	CPU_ZERO(&keepCpus);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &keepCpus);                                /* 0b10101010101010101010101010101010 */
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		PTHR4W_TEST_ASSERT(pthread_create(&tid, NULL, InnerBlock::mythread, (void*)&threadCpus) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(tid, NULL) == 0);
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		PTHR4W_TEST_ASSERT(pthread_setaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), (*(PDWORD_PTR)&threadCpus) /* Violating Opacity*/);
		PTHR4W_TEST_ASSERT(vThreadMask != 0);
		PTHR4W_TEST_ASSERT(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
		PTHR4W_TEST_ASSERT(pthread_create(&tid, NULL, InnerBlock::mythread, (void*)&threadCpus) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(tid, NULL) == 0);
	}
	return 0;
}

static int PThr4wTest_Affinity6()
{
	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			pthread_attr_t * attrPtr = (pthread_attr_t*)arg;
			cpu_set_t threadCpus, attrCpus;
			PTHR4W_TEST_ASSERT(pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &threadCpus) == 0);
			PTHR4W_TEST_ASSERT(pthread_attr_getaffinity_np(attrPtr, sizeof(cpu_set_t), &attrCpus) == 0);
			PTHR4W_TEST_ASSERT(CPU_EQUAL(&attrCpus, &threadCpus));
			return 0;
		}
	};
	uint cpu;
	pthread_t tid;
	pthread_attr_t attr1, attr2;
	cpu_set_t threadCpus;
	cpu_set_t keepCpus;
	pthread_t self = pthread_self();
	if(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == ENOSYS) {
		printf("pthread_get/set_affinity_np API not supported for this platform: skipping test.");
		return 0;
	}
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr1) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_init(&attr2) == 0);
	CPU_ZERO(&keepCpus);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &keepCpus);                                /* 0b10101010101010101010101010101010 */
	}
	PTHR4W_TEST_ASSERT(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		PTHR4W_TEST_ASSERT(pthread_attr_setaffinity_np(&attr1, sizeof(cpu_set_t), &threadCpus) == 0);
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		PTHR4W_TEST_ASSERT(pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t), &threadCpus) == 0);
		PTHR4W_TEST_ASSERT(pthread_create(&tid, &attr1, InnerBlock::mythread, (void*)&attr1) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(tid, NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_create(&tid, &attr2, InnerBlock::mythread, (void*)&attr2) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(tid, NULL) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_attr_destroy(&attr1) == 0);
	PTHR4W_TEST_ASSERT(pthread_attr_destroy(&attr2) == 0);
	return 0;
}
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
static int PThr4wTest_Cancel1()
{
	const int NUMTHREADS = 2; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* ... */
			{
				int oldstate;
				int oldtype;
				PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate) == 0);
				PTHR4W_TEST_ASSERT(oldstate == PTHREAD_CANCEL_ENABLE); /* Check default */
				PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
				PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL) == 0);
				PTHR4W_TEST_ASSERT(pthread_setcancelstate(oldstate, &oldstate) == 0);
				PTHR4W_TEST_ASSERT(oldstate == PTHREAD_CANCEL_DISABLE); /* Check setting */
				PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype) == 0);
				PTHR4W_TEST_ASSERT(oldtype == PTHREAD_CANCEL_DEFERRED); /* Check default */
				PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
				PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
				PTHR4W_TEST_ASSERT(pthread_setcanceltype(oldtype, &oldtype) == 0);
				PTHR4W_TEST_ASSERT(oldtype == PTHREAD_CANCEL_ASYNCHRONOUS); /* Check setting */
			}
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	for(i = 1; i <= NUMTHREADS; i++) {
		/* ... */
	}
	PTHR4W_TEST_ASSERT(!failed);
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
static int PThr4wTest_Cancel2()
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
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			// Set to known state and type 
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
			result = 1;
		#if !defined(__cplusplus)
			__try
		#else
			try
		#endif
			{
				// Wait for go from main 
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
				// Should not get into here.
				result += 100;
			}
			// Should not get to here either.
			result += 1000;
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&go, NULL, NUMTHREADS + 1) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	pthread_barrier_wait(&go);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
	}
	pthread_barrier_wait(&go);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: location %d\n", i, threadbag[i].started, (int)(size_t)result);
		}
		failed |= fail;
	}
	PTHR4W_TEST_ASSERT(!failed);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&go) == 0);
	return 0; // Success
}

#else /* defined(__cplusplus) */
	static int PThr4wTest_Cancel2()
	{
		slfprintf_stderr("Test N/A for this compiler environment.\n");
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
static int PThr4wTest_Cancel3()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			// Set to known state and type 
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
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
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(NUMTHREADS * 100);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to complete.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
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
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test cancellation does not occur in deferred cancellation threads with no cancellation points.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions: pthread_create, pthread_self, pthread_cancel, pthread_join, pthread_setcancelstate, pthread_setcanceltype
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Cancel4()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void * result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
			/*
			 * We wait up to 2 seconds, waking every 0.1 seconds,
			 * for a cancellation to be applied to us.
			 */
			for(bag->count = 0; bag->count < 20; bag->count++)
				Sleep(100);
			return result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		/*
		 * The thread does not contain any cancellation points, so
		 * a return value of PTHREAD_CANCELED indicates that async
		 * cancellation occurred.
		 */
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result == PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test calling pthread_cancel from the main thread without calling pthread_self() in main.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Cancel5()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
			/*
			 * We wait up to 10 seconds, waking every 0.1 seconds,
			 * for a cancellation to be applied to us.
			 */
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
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
		/*
		 * The thread does not contain any cancellation points, so
		 * a return value of PTHREAD_CANCELED confirms that async
		 * cancellation succeeded.
		 */
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test double cancellation - asynchronous. Second attempt should fail (ESRCH).
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
static int PThr4wTest_Cancel6a()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
			/*
			 * We wait up to 10 seconds, waking every 0.1 seconds,
			 * for a cancellation to be applied to us.
			 */
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
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == ESRCH);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
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
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test double cancellation - deferred.
//   Second attempt should succeed (unless the canceled thread has started
//   cancellation already - not tested here).
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
static int PThr4wTest_Cancel6d()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) == 0);
			/*
			 * We wait up to 10 seconds, waking every 0.1 seconds,
			 * for a cancellation to be applied to us.
			 */
			for(bag->count = 0; bag->count < 100; bag->count++) {
				Sleep(100);
				pthread_testcancel();
			}
			return result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
		if(pthread_cancel(t[i]) != 0) {
			printf("Second cancellation failed but this is expected sometimes.\n");
		}
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test canceling a Win32 thread having created an implicit POSIX handle for it.
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
static int PThr4wTest_Cancel7()
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
			int i;
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			PTHR4W_TEST_ASSERT((bag->self = pthread_self()).p != NULL);
			PTHR4W_TEST_ASSERT(pthread_kill(bag->self, 0) == 0);
			for(i = 0; i < 100; i++) {
				Sleep(100);
				pthread_testcancel();
			}
		#if !defined (__MINGW32__) || defined (__MSVCRT__)
			return 0;
		#endif
		}
	};
	int failed = 0;
	int i;
	HANDLE h[NUMTHREADS + 1];
	uint thrAddr; // Dummy variable to pass a valid location to _beginthreadex (Win98). 
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		h[i] = (HANDLE)_beginthreadex(NULL, 0, InnerBlock::Win32thread, (void*)&threadbag[i], 0, &thrAddr);
#else
		h[i] = (HANDLE)_beginthread(InnerBlock::Win32thread, 0, (void*)&threadbag[i]);
#endif
	}
	// 
	// Code to control or manipulate child threads should probably go here.
	// 
	Sleep(500);
	// 
	// Cancel all threads.
	// 
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_kill(threadbag[i].self, 0) == 0);
		PTHR4W_TEST_ASSERT(pthread_cancel(threadbag[i].self) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		PTHR4W_TEST_ASSERT(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		// Can't get a result code.
		result = (int)(size_t)PTHREAD_CANCELED;
#endif
		PTHR4W_TEST_ASSERT(threadbag[i].self.p != NULL);
		PTHR4W_TEST_ASSERT(pthread_kill(threadbag[i].self, 0) == ESRCH);
		fail = (result != (int)(size_t)PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test cancelling a blocked Win32 thread having created an implicit POSIX handle for it.
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
static int PThr4wTest_Cancel8()
{
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static pthread_cond_t CV = PTHREAD_COND_INITIALIZER;
	static pthread_mutex_t CVLock = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		#if !defined (__MINGW32__) || defined (__MSVCRT__)
		static uint __stdcall Win32thread(void * arg)
		#else
		static void Win32thread(void * arg)
		#endif
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			PTHR4W_TEST_ASSERT((bag->self = pthread_self()).p != NULL);
			PTHR4W_TEST_ASSERT(pthread_kill(bag->self, 0) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&CVLock) == 0);
			pthread_cleanup_push(pthread_mutex_unlock, &CVLock);
			pthread_cond_wait(&CV, &CVLock);
			pthread_cleanup_pop(1);
		#if !defined (__MINGW32__) || defined (__MSVCRT__)
			return 0;
		#endif
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
	/*
	 * Cancel all threads.
	 */
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_kill(threadbag[i].self, 0) == 0);
		PTHR4W_TEST_ASSERT(pthread_cancel(threadbag[i].self) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		int result = 0;
#if !defined (__MINGW32__) || defined (__MSVCRT__)
		PTHR4W_TEST_ASSERT(GetExitCodeThread(h[i], (LPDWORD)&result) == TRUE);
#else
		// Can't get a result code.
		result = (int)(size_t)PTHREAD_CANCELED;
#endif
		PTHR4W_TEST_ASSERT(threadbag[i].self.p != NULL);
		PTHR4W_TEST_ASSERT(pthread_kill(threadbag[i].self, 0) == ESRCH);
		fail = (result != (int)(size_t)PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: count %d\n", i, threadbag[i].started, threadbag[i].count);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test true asynchronous cancellation with Alert driver.
// Requirements Tested:
// - Cancel threads, including those blocked on system recources such as network I/O.
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
static int PThr4wTest_Cancel9()
{
	class InnerBlock {
	public:
		static void * test_udp(void * arg)
		{
			struct sockaddr_in serverAddress;
			struct sockaddr_in clientAddress;
			SOCKET UDPSocket;
			int addr_len;
			int nbyte;
			char buffer[4096];
			WORD wsaVersion = MAKEWORD(2, 2);
			WSADATA wsaData;
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
			if(WSAStartup(wsaVersion, &wsaData) != 0) {
				return NULL;
			}
			UDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
			if((int)UDPSocket == -1) {
				printf("Server: socket ERROR \n");
				exit(-1);
			}
			serverAddress.sin_family = AF_INET;
			serverAddress.sin_addr.s_addr = INADDR_ANY;
			serverAddress.sin_port = htons(9003);
			if(bind(UDPSocket, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in))) {
				printf("Server: ERROR can't bind UDPSocket");
				exit(-1);
			}
			addr_len = sizeof(struct sockaddr);
			nbyte = 512;
			(void)recvfrom(UDPSocket, (char*)buffer, nbyte, 0, (struct sockaddr *)&clientAddress, &addr_len);
			closesocket(UDPSocket);
			WSACleanup();
			return NULL;
		}
		static void * test_sleep(void * arg)
		{
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
			Sleep(1000);
			return NULL;
		}
		static void * test_wait(void * arg)
		{
			HANDLE hEvent;
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
			hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			(void)WaitForSingleObject(hEvent, 1000); /* WAIT_IO_COMPLETION */
			return NULL;
		}
	};
	pthread_t t;
	void * result;
	if(pthread_win32_test_features_np(__PTW32_ALERTABLE_ASYNC_CANCEL)) {
		printf("Cancel sleeping thread.\n");
		PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::test_sleep, NULL) == 0);
		/* Sleep for a while; then cancel */
		Sleep(100);
		PTHR4W_TEST_ASSERT(pthread_cancel(t) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT(result == PTHREAD_CANCELED && "test_sleep");

		printf("Cancel waiting thread.\n");
		PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::test_wait, NULL) == 0);
		/* Sleep for a while; then cancel. */
		Sleep(100);
		PTHR4W_TEST_ASSERT(pthread_cancel(t) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT(result == PTHREAD_CANCELED && "test_wait");

		printf("Cancel blocked thread (blocked on network I/O).\n");
		PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::test_udp, NULL) == 0);
		/* Sleep for a while; then cancel. */
		Sleep(100);
		PTHR4W_TEST_ASSERT(pthread_cancel(t) == 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT(result == PTHREAD_CANCELED && "test_udp");
	}
	else {
		printf("Alertable async cancel not available.\n");
	}
	return 0; // Success
}
// 
// Test Thread Specific Data (TSD) key creation and destruction.
// Test Method (validation or falsification):
// - validation
// Requirements Tested:
// - keys are created for each existing thread including the main thread
// - keys are created for newly created threads
// - keys are thread specific
// - destroy routine is called on each thread exit including the main thread
// Output:
// - text to stdout
// Assumptions:
// - already validated:     pthread_create(), pthread_once()
// - main thread also has a POSIX thread identity
// 
static int PThr4wTest_Tsd1()
{
	const int NUM_THREADS = 100;
	static pthread_key_t key = NULL;
	static int accesscount[NUM_THREADS];
	static int thread_set[NUM_THREADS];
	static int thread_destroyed[NUM_THREADS];
	static pthread_barrier_t startBarrier;

	class InnerBlock {
	public:
		static void destroy_key(void * arg)
		{
			int * j = static_cast<int *>(arg);
			(*j)++;
			PTHR4W_TEST_ASSERT(*j == 2);
			thread_destroyed[j - accesscount] = 1;
		}
		static void setkey(void * arg)
		{
			int * j = static_cast<int *>(arg);
			thread_set[j - accesscount] = 1;
			PTHR4W_TEST_ASSERT(*j == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == NULL);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == arg);
			(*j)++;
			PTHR4W_TEST_ASSERT(*j == 1);
		}
		static void * mythread(void * arg)
		{
			(void)pthread_barrier_wait(&startBarrier);
			setkey(arg);
			return 0;
			/* Exiting the thread will call the key destructor. */
		}
	};
	int i;
	int fail = 0;
	pthread_t thread[NUM_THREADS];
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&startBarrier, NULL, NUM_THREADS/2) == 0);
	for(i = 1; i < NUM_THREADS/2; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	/*
	 * Here we test that existing threads will get a key created
	 * for them.
	 */
	PTHR4W_TEST_ASSERT(pthread_key_create(&key, InnerBlock::destroy_key) == 0);
	(void)pthread_barrier_wait(&startBarrier);
	/*
	 * Test main thread key.
	 */
	accesscount[0] = 0;
	InnerBlock::setkey((void*)&accesscount[0]);
	/*
	 * Here we test that new threads will get a key created
	 * for them.
	 */
	for(i = NUM_THREADS/2; i < NUM_THREADS; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	/*
	 * Wait for all threads to complete.
	 */
	for(i = 1; i < NUM_THREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(thread[i], NULL) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_key_delete(key) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&startBarrier) == 0);
	for(i = 1; i < NUM_THREADS; i++) {
		/*
		 * The counter is incremented once when the key is set to
		 * a value, and again when the key is destroyed. If the key
		 * doesn't get set for some reason then it will still be
		 * NULL and the destroy function will not be called, and
		 * hence accesscount will not equal 2.
		 */
		if(accesscount[i] != 2) {
			fail++;
			slfprintf_stderr("Thread %d key, set = %d, destroyed = %d\n",
			    i, thread_set[i], thread_destroyed[i]);
		}
	}
	fflush(stderr);
	return (fail);
}
// 
// Test Thread Specific Data (TSD) key creation and destruction.
// Test Method (validation or falsification):
// - validation
// Requirements Tested:
// - keys are created for each existing thread including the main thread
// - keys are created for newly created threads
// - keys are thread specific
// - destroy routine is called on each thread exit including the main thread
// Output:
// - text to stdout
// Assumptions:
// - already validated:     pthread_create(), pthread_once()
// - main thread also has a POSIX thread identity
// 
static int PThr4wTest_Tsd2()
{
	const int NUM_THREADS = 100;
	static pthread_key_t key = NULL;
	static int accesscount[NUM_THREADS];
	static int thread_set[NUM_THREADS];
	static int thread_destroyed[NUM_THREADS];
	static pthread_barrier_t startBarrier;

	class InnerBlock {
	public:
		static void destroy_key(void * arg)
		{
			int * j = static_cast<int *>(arg);
			(*j)++;
			/*
			 * Set TSD key from the destructor to test destructor iteration.
			 * The key value will have been set to NULL by the library before
			 * calling the destructor (with the value that the key had). We
			 * reset the key value here which should cause the destructor to be
			 * called a second time.
			 */
			if(*j == 2)
				PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			else
				PTHR4W_TEST_ASSERT(*j == 3);
			thread_destroyed[j - accesscount] = 1;
		}
		static void setkey(void * arg)
		{
			int * j = static_cast<int *>(arg);
			thread_set[j - accesscount] = 1;
			PTHR4W_TEST_ASSERT(*j == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == NULL);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == arg);
			(*j)++;
			PTHR4W_TEST_ASSERT(*j == 1);
		}
		static void * mythread(void * arg)
		{
			(void)pthread_barrier_wait(&startBarrier);
			setkey(arg);
			return 0;
			/* Exiting the thread will call the key destructor. */
		}
	};
	int i;
	int fail = 0;
	pthread_t thread[NUM_THREADS];
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&startBarrier, NULL, NUM_THREADS/2) == 0);
	for(i = 1; i < NUM_THREADS/2; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	/*
	 * Here we test that existing threads will get a key created for them.
	 */
	PTHR4W_TEST_ASSERT(pthread_key_create(&key, InnerBlock::destroy_key) == 0);
	(void)pthread_barrier_wait(&startBarrier);
	/*
	 * Test main thread key.
	 */
	accesscount[0] = 0;
	InnerBlock::setkey((void*)&accesscount[0]);
	/*
	 * Here we test that new threads will get a key created for them.
	 */
	for(i = NUM_THREADS/2; i < NUM_THREADS; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	/*
	 * Wait for all threads to complete.
	 */
	for(i = 1; i < NUM_THREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(thread[i], NULL) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_key_delete(key) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&startBarrier) == 0);
	for(i = 1; i < NUM_THREADS; i++) {
		/*
		 * The counter is incremented once when the key is set to
		 * a value, and again when the key is destroyed. If the key
		 * doesn't get set for some reason then it will still be
		 * NULL and the destroy function will not be called, and
		 * hence accesscount will not equal 2.
		 */
		if(accesscount[i] != 3) {
			fail++;
			slfprintf_stderr("Thread %d key, set = %d, destroyed = %d\n", i, thread_set[i], thread_destroyed[i]);
		}
	}
	fflush(stderr);
	return (fail);
}
// 
// Test Method (validation or falsification):
// - validation
// Requirements Tested:
// - keys are created for each existing thread including the main thread
// - keys are created for newly created threads
// - keys are thread specific
// - key is deleted before threads exit
// - key destructor function is not called
// Output:
// - text to stdout
// Assumptions:
// - already validated:     pthread_create(), pthread_once()
// - main thread also has a POSIX thread identity
// 
static int PThr4wTest_Tsd3()
{
	const int NUM_THREADS = 100;
	static pthread_key_t key = NULL;
	static int accesscount[NUM_THREADS];
	static int thread_set[NUM_THREADS];
	static int thread_destroyed[NUM_THREADS];
	static pthread_barrier_t startBarrier;
	static pthread_barrier_t progressSyncBarrier;

	class InnerBlock {
	public:
		static void destroy_key(void * arg)
		{
			// The destructor function should not be called if the key is deleted before the thread exits.
			slfprintf_stderr("The key destructor was called but should not have been.\n");
			exit(1);
		}
		static void setkey(void * arg)
		{
			int * j = static_cast<int *>(arg);
			thread_set[j - accesscount] = 1;
			PTHR4W_TEST_ASSERT(*j == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == NULL);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_setspecific(key, arg) == 0);
			PTHR4W_TEST_ASSERT(pthread_getspecific(key) == arg);
			(*j)++;
			PTHR4W_TEST_ASSERT(*j == 1);
		}
		static void * mythread(void * arg)
		{
			(void)pthread_barrier_wait(&startBarrier);
			setkey(arg);
			(void)pthread_barrier_wait(&progressSyncBarrier);
			(void)pthread_barrier_wait(&progressSyncBarrier);
			return 0;
		}
	};
	int i;
	int fail = 0;
	pthread_t thread[NUM_THREADS];
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&startBarrier, NULL, NUM_THREADS/2) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&progressSyncBarrier, NULL, NUM_THREADS) == 0);
	for(i = 1; i < NUM_THREADS/2; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	// Here we test that existing threads will get a key created for them.
	PTHR4W_TEST_ASSERT(pthread_key_create(&key, InnerBlock::destroy_key) == 0);
	(void)pthread_barrier_wait(&startBarrier);
	// Test main thread key.
	accesscount[0] = 0;
	InnerBlock::setkey((void*)&accesscount[0]);
	// Here we test that new threads will get a key created for them.
	for(i = NUM_THREADS/2; i < NUM_THREADS; i++) {
		accesscount[i] = thread_set[i] = thread_destroyed[i] = 0;
		PTHR4W_TEST_ASSERT(pthread_create(&thread[i], NULL, InnerBlock::mythread, (void*)&accesscount[i]) == 0);
	}
	(void)pthread_barrier_wait(&progressSyncBarrier);
	// Deleting the key should not call the key destructor.
	PTHR4W_TEST_ASSERT(pthread_key_delete(key) == 0);
	(void)pthread_barrier_wait(&progressSyncBarrier);
	// Wait for all threads to complete.
	for(i = 1; i < NUM_THREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(thread[i], NULL) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&startBarrier) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&progressSyncBarrier) == 0);
	for(i = 1; i < NUM_THREADS; i++) {
		// The counter is incremented once when the key is set to a value.
		if(accesscount[i] != 1) {
			fail++;
			slfprintf_stderr("Thread %d key, set = %d, destroyed = %d\n", i, thread_set[i], thread_destroyed[i]);
		}
	}
	fflush(stderr);
	return (fail);
}

#if defined(_MSC_VER) || defined(__cplusplus)
//
// Test Synopsis: Test cleanup handler executes (when thread is not canceled).
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
static int PThr4wTest_CleanUp0()
{
	static const int NUMTHREADS = 10;
	static bag_t threadbag[NUMTHREADS + 1];
	static sharedInt_t pop_count;
	class InnerBlock {
		static void increment_pop_count(void * arg)
		{
			sharedInt_t * sI = (sharedInt_t*)arg;
			EnterCriticalSection(&sI->cs);
			sI->i++;
			LeaveCriticalSection(&sI->cs);
		}
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(increment_pop_count, (void*)&pop_count);
			Sleep(100);
			pthread_cleanup_pop(1);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	memzero(&pop_count, sizeof(sharedInt_t));
	InitializeCriticalSection(&pop_count.cs);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
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
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result == PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: result %d\n", i, threadbag[i].started, (int)(size_t)result);
			fflush(stderr);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	PTHR4W_TEST_ASSERT(pop_count.i == NUMTHREADS);
	DeleteCriticalSection(&pop_count.cs);
	return 0; // Success
}
//
// Test Synopsis: Test cleanup handler executes (when thread is canceled).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
static int PThr4wTest_CleanUp1()
{
	static const int NUMTHREADS = 10;
	static bag_t threadbag[NUMTHREADS + 1];
	static sharedInt_t pop_count;
	class InnerBlock {
		static void
		#ifdef __PTW32_CLEANUP_C
		__cdecl
		#endif
		increment_pop_count(void * arg)
		{
			sharedInt_t * sI = (sharedInt_t*)arg;
			EnterCriticalSection(&sI->cs);
			sI->i++;
			LeaveCriticalSection(&sI->cs);
		}
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Set to known state and type */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(increment_pop_count, (void*)&pop_count);
			/*
			 * We don't have true async cancellation - it relies on the thread
			 * at least re-entering the run state at some point.
			 * We wait up to 10 seconds, waking every 0.1 seconds,
			 * for a cancellation to be applied to us.
			 */
			for(bag->count = 0; bag->count < 100; bag->count++)
				Sleep(100);

			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	memzero(&pop_count, sizeof(sharedInt_t));
	InitializeCriticalSection(&pop_count.cs);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(500);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(t[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = (result != PTHREAD_CANCELED);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: result %d\n", i, threadbag[i].started, (int)(size_t)result);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	PTHR4W_TEST_ASSERT(pop_count.i == NUMTHREADS);
	DeleteCriticalSection(&pop_count.cs);
	return 0; // Success
}
//
// Test Synopsis: Test cleanup handler executes (when thread is not canceled).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
static int PThr4wTest_CleanUp2()
{
	static const int NUMTHREADS = 10;
	static bag_t threadbag[NUMTHREADS + 1];
	static sharedInt_t pop_count;
	class InnerBlock {
		static void increment_pop_count(void * arg)
		{
			sharedInt_t * sI = (sharedInt_t*)arg;
			EnterCriticalSection(&sI->cs);
			sI->i++;
			LeaveCriticalSection(&sI->cs);
		}
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
		#ifdef _MSC_VER
			#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(increment_pop_count, (void*)&pop_count);
			sched_yield();
			pthread_cleanup_pop(1);
		#ifdef _MSC_VER
			#pragma inline_depth()
		#endif
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	memzero(&pop_count, sizeof(sharedInt_t));
	InitializeCriticalSection(&pop_count.cs);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	// 
	// Code to control or manipulate child threads should probably go here.
	// 
	Sleep(1000);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = ((int)(size_t)result != 0);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: result: %d\n", i, threadbag[i].started, (int)(size_t)result);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	PTHR4W_TEST_ASSERT(pop_count.i == NUMTHREADS);
	DeleteCriticalSection(&pop_count.cs);
	return 0; // Success
}
//
// Test Synopsis: Test cleanup handler does not execute (when thread is not canceled).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
static int PThr4wTest_CleanUp3()
{
	static const int NUMTHREADS = 10;
	static bag_t threadbag[NUMTHREADS + 1];
	static sharedInt_t pop_count;
	class InnerBlock {
		static void increment_pop_count(void * arg)
		{
			sharedInt_t * sI = (sharedInt_t*)arg;
			EnterCriticalSection(&sI->cs);
			sI->i++;
			LeaveCriticalSection(&sI->cs);
		}
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
		#ifdef _MSC_VER
			#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(increment_pop_count, (void*)&pop_count);
			sched_yield();
			EnterCriticalSection(&pop_count.cs);
			pop_count.i--;
			LeaveCriticalSection(&pop_count.cs);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
			#pragma inline_depth()
		#endif
			return (void*)(size_t)result;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	memzero(&pop_count, sizeof(sharedInt_t));
	InitializeCriticalSection(&pop_count.cs);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(1000);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		if(!threadbag[i].started) {
			failed |= !threadbag[i].started;
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		fail = ((int)(size_t)result != 0);
		if(fail) {
			slfprintf_stderr("Thread %d: started %d: result: %d\n", i, threadbag[i].started, (int)(size_t)result);
		}
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	PTHR4W_TEST_ASSERT(pop_count.i == -(NUMTHREADS));
	DeleteCriticalSection(&pop_count.cs);
	return 0; // Success
}

#else /* defined(_MSC_VER) || defined(__cplusplus) */
	static int PThr4wTest_CleanUp0() { return 0; }
	static int PThr4wTest_CleanUp1() { return 0; }
	static int PThr4wTest_CleanUp2() { return 0; }
	static int PThr4wTest_CleanUp3() { return 0; }
#endif /* defined(_MSC_VER) || defined(__cplusplus) */
// 
// Test Synopsis:
// - Test initialisation and destruction of a CV.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_init returns 0, and
// - pthread_cond_destroy returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_init returns non-zero, or
// - pthread_cond_destroy returns non-zero.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar1()
{
	static pthread_cond_t cv = NULL;
	PTHR4W_TEST_ASSERT(cv == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(cv != NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cv) == 0);
	PTHR4W_TEST_ASSERT(cv == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test CV linked list management.
// Test Method (Validation or Falsification):
// - Validation:
//   Initiate and destroy several CVs in random order.
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - All initialised CVs destroyed without segfault.
// - Successfully broadcasts all remaining CVs after each CV is removed.
// 
static int PThr4wTest_CondVar11()
{
	//static const int NUM_CV = 100;
	static pthread_cond_t cv[100];

	int i, j;
	for(i = 0; i < SIZEOFARRAY(cv); i++) {
		/* Traverse the list before every init of a CV. */
		PTHR4W_TEST_ASSERT(pthread_timechange_handler_np(NULL) == (void*)0);
		PTHR4W_TEST_ASSERT(pthread_cond_init(&cv[i], NULL) == 0);
	}
	j = SIZEOFARRAY(cv);
	srand((unsigned)time(NULL));
	do {
		i = (SIZEOFARRAY(cv) - 1) * rand() / RAND_MAX;
		if(cv[i] != NULL) {
			j--;
			PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cv[i]) == 0);
			/* Traverse the list every time we remove a CV. */
			PTHR4W_TEST_ASSERT(pthread_timechange_handler_np(NULL) == (void*)0);
		}
	} while(j > 0);
	return 0;
}
// 
// Test Synopsis:
// - Test CV linked list management and serialisation.
// Test Method (Validation or Falsification):
// - Validation:
//   Initiate and destroy several CVs in random order.
//   Asynchronously traverse the CV list and broadcast.
// Description:
// - Creates and then imediately destroys a CV. Does not test the CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - All initialised CVs destroyed without segfault.
// - Successfully broadcasts all remaining CVs after each CV is removed.
// 
static int PThr4wTest_CondVar12()
{
	//static const int NUM_CV = 5;
	const int NUM_LOOPS = 5;
	static pthread_cond_t cv[5];

	int i, j;
	void * result = (void*)-1;
	pthread_t t;
	for(int k = 0; k < NUM_LOOPS; k++) {
		for(i = 0; i < SIZEOFARRAY(cv); i++) {
			PTHR4W_TEST_ASSERT(pthread_cond_init(&cv[i], NULL) == 0);
		}
		j = SIZEOFARRAY(cv);
		(void)srand((unsigned)time(NULL));
		// Traverse the list asynchronously. 
		PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, pthread_timechange_handler_np, NULL) == 0);
		do {
			i = (SIZEOFARRAY(cv) - 1) * rand() / RAND_MAX;
			if(cv[i] != NULL) {
				j--;
				PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cv[i]) == 0);
			}
		} while(j > 0);
		PTHR4W_TEST_ASSERT(pthread_join(t, &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test timed wait on a CV.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because the CV is never signaled, we expect the wait to time out.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar2()
{
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;

	struct timespec abstime = { 0, 0 }, reltime = { 1, 0 };
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cv, &mutex, &abstime) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			slfprintf_stderr("Result = %s\n", PThr4wErrorString[result]);
			slfprintf_stderr("\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			slfprintf_stderr("\tWaitersGone = %ld\n", cv->nWaitersGone);
			slfprintf_stderr("\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		PTHR4W_TEST_ASSERT(result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with no signal/broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because the CV is never signaled, we expect the waits to time out.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar21()
{
	const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static struct timespec abstime = { 0, 0 };
	static struct timespec reltime = { 5, 0 };

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cv, &mutex, &abstime) == ETIMEDOUT);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void* result = (void*)0;
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
	}
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			slfprintf_stderr("Result = %s\n", PThr4wErrorString[result]);
			slfprintf_stderr("\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			slfprintf_stderr("\tWaitersGone = %ld\n", cv->nWaitersGone);
			slfprintf_stderr("\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		PTHR4W_TEST_ASSERT(result == 0);
	}
	return 0;
}
// 
// Test Synopsis:
// - Test basic function of a CV
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - The primary thread takes the lock before creating any threads.
//   The secondary thread blocks on the lock allowing the primary
//   thread to enter the cv wait state which releases the lock.
//   The secondary thread then takes the lock and signals the waiting primary thread.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar3()
{
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static int shared = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result = 0;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			shared++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			if((result = pthread_cond_signal(&cv)) != 0) {
				printf("Error = %s\n", PThr4wErrorString[result]);
			}
			PTHR4W_TEST_ASSERT(result == 0);
			return 0;
		}
	};
	const int NUMTHREADS = 2; // Including the primary thread
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(shared > 0))
		PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cv, &mutex, &abstime) == 0);
	PTHR4W_TEST_ASSERT(shared > 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t[1], NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cv) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with some signaled.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because some CVs are never signaled, we expect their waits to time out.
//   Some are signaled, the rest time out. Pthread_cond_destroy() will fail
//   unless all are accounted for, either signaled or timedout.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar31()
{
	static const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_cond_t cv1;
	static pthread_mutex_t mutex;
	static pthread_mutex_t mutex1;
	static struct timespec abstime = { 0, 0 }, reltime = { 5, 0 };
	static int timedout = 0;
	static int signaled = 0;
	static int awoken = 0;
	static int waiting = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
			++waiting;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
			PTHR4W_TEST_ASSERT(pthread_cond_signal(&cv1) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			result = pthread_cond_timedwait(&cv, &mutex, &abstime);
			if(result == ETIMEDOUT) {
				timedout++;
			}
			else {
				awoken++;
			}
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void * result = (void*)0;
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv1, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex1, NULL) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex1) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	do {
		PTHR4W_TEST_ASSERT(pthread_cond_wait(&cv1, &mutex1) == 0);
	} while(NUMTHREADS > waiting);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex1) == 0);
	for(i = NUMTHREADS/3; i <= 2*NUMTHREADS/3; i++) {
//      PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
		PTHR4W_TEST_ASSERT(pthread_cond_signal(&cv) == 0);
//      PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
		signaled++;
	}
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
	}
	slfprintf_stderr("awk = %d\n", awoken);
	slfprintf_stderr("sig = %d\n", signaled);
	slfprintf_stderr("tot = %d\n", timedout);
	PTHR4W_TEST_ASSERT(signaled == awoken);
	PTHR4W_TEST_ASSERT(timedout == NUMTHREADS - signaled);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cv1) == 0);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			slfprintf_stderr("Result = %s\n", PThr4wErrorString[result]);
			slfprintf_stderr("\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			slfprintf_stderr("\tWaitersGone = %ld\n", cv->nWaitersGone);
			slfprintf_stderr("\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		PTHR4W_TEST_ASSERT(result == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex1) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeout of multiple waits on a CV with remainder broadcast awoken.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Because some CVs are never signaled, we expect their waits to time out.
//   Some time out, the rest are broadcast signaled. Pthread_cond_destroy() will fail
//   unless all are accounted for, either signaled or timedout.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar32()
{
	static const int NUMTHREADS = 30;
	static pthread_cond_t cv;
	static pthread_mutex_t mutex;
	static struct timespec abstime, abstime2;
	static struct timespec reltime = { 5, 0 };
	static int timedout = 0;
	static int awoken = 0;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			int result;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			abstime2.tv_sec = abstime.tv_sec;
			if((int)(size_t)arg % 3 == 0) {
				abstime2.tv_sec += 2;
			}
			result = pthread_cond_timedwait(&cv, &mutex, &abstime2);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			if(result == ETIMEDOUT) {
				InterlockedIncrement((LPLONG)&timedout);
			}
			else {
				InterlockedIncrement((LPLONG)&awoken);
			}
			return arg;
		}
	};
	int i;
	pthread_t t[NUMTHREADS + 1];
	void* result = (void*)0;
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cv, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, NULL) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	abstime2.tv_sec = abstime.tv_sec;
	abstime2.tv_nsec = abstime.tv_nsec;
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)(size_t)i) == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(t[i], &result) == 0);
		PTHR4W_TEST_ASSERT((int)(size_t)result == i);
		/*
		 * Approximately 2/3rds of the threads are expected to time out.
		 * Signal the remainder after some threads have woken up and exited
		 * and while some are still waking up after timeout.
		 * Also tests that redundant broadcasts don't return errors.
		 */
		// PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
		if(InterlockedExchangeAdd((LPLONG)&awoken, 0L) > NUMTHREADS/3) {
			PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cv) == 0);
		}
		// PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	}
	PTHR4W_TEST_ASSERT(awoken == NUMTHREADS - timedout);
	{
		int result = pthread_cond_destroy(&cv);
		if(result != 0) {
			slfprintf_stderr("Result = %s\n", PThr4wErrorString[result]);
			slfprintf_stderr("\tWaitersBlocked = %ld\n", cv->nWaitersBlocked);
			slfprintf_stderr("\tWaitersGone = %ld\n", cv->nWaitersGone);
			slfprintf_stderr("\tWaitersToUnblock = %ld\n", cv->nWaitersToUnblock);
			fflush(stderr);
		}
		PTHR4W_TEST_ASSERT(result == 0);
	}
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test timeouts and lost signals on a CV.
// Test Method (Validation or Falsification):
// - Validation
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait does not return ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar33()
{
	static pthread_cond_t cnd;
	static pthread_mutex_t mtx;
	static const long NANOSEC_PER_SEC = 1000000000L;

	int rc;
	struct timespec abstime, reltime = { 0, NANOSEC_PER_SEC/2 };
	PTHR4W_TEST_ASSERT(pthread_cond_init(&cnd, 0) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mtx, 0) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	/* Here pthread_cond_timedwait should time out after one second. */
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mtx) == 0);
	PTHR4W_TEST_ASSERT((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mtx) == 0);
	/* Here, the condition variable is signalled, but there are no
	   threads waiting on it. The signal should be lost and
	   the next pthread_cond_timedwait should time out too. */
	PTHR4W_TEST_ASSERT((rc = pthread_cond_signal(&cnd)) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mtx) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT((rc = pthread_cond_timedwait(&cnd, &mtx, &abstime)) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mtx) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Test PTHREAD_COND_INITIALIZER.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test basic CV function but starting with a static initialised CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar4()
{
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static const int NUMTHREADS = 2;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
			cvthing.shared++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			PTHR4W_TEST_ASSERT(pthread_cond_signal(&cvthing.notbusy) == 0);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	cvthing.shared = 0;
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock != PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT(cvthing.notbusy != PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(cvthing.shared > 0))
		PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
	PTHR4W_TEST_ASSERT(cvthing.shared > 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t[1], NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cvthing.notbusy) == 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with one waiting CV.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - pthread_cond_timedwait returns 0.
// - Process returns zero exit status.
// Fail Criteria:
// - pthread_cond_timedwait returns ETIMEDOUT.
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar5()
{
	static const int NUMTHREADS = 2;
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
			cvthing.shared++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cvthing.notbusy) == 0);
			return 0;
		}
	};
	pthread_t t[NUMTHREADS];
	struct timespec abstime, reltime = { 5, 0 };
	cvthing.shared = 0;
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock != PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == ETIMEDOUT);
	PTHR4W_TEST_ASSERT(cvthing.notbusy != PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_create(&t[1], NULL, InnerBlock::mythread, (void*)1) == 0);
	pthread_win32_getabstime_np(&abstime, &reltime);
	while(!(cvthing.shared > 0))
		PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
	PTHR4W_TEST_ASSERT(cvthing.shared > 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t[1], NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cvthing.notbusy) == 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	return 0;
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with NUMTHREADS (=5) waiting CVs.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar6()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 5, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
			while(!(cvthing.shared > 0))
				PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			PTHR4W_TEST_ASSERT(cvthing.shared > 0);
			awoken++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	cvthing.shared = 0;
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
	Sleep(1000); // Give threads time to start.
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
	cvthing.shared++;
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cvthing.notbusy) == 0);
	// Give threads time to complete.
	for(i = 1; i <= NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
	}
	/*
	 * Cleanup the CV.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cvthing.notbusy) == 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here.
	PTHR4W_TEST_ASSERT(awoken == NUMTHREADS);
	return 0; // Success
}
// 
// Test Synopsis:
// - Test pthread_cond_broadcast with thread cancellation.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Test broadcast with NUMTHREADS (=5) waiting CVs, one is canceled while waiting.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar7()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 10, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			PTHR4W_TEST_ASSERT(cvthing.shared > 0);
			awoken++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	pthread_t t[NUMTHREADS + 1];
	cvthing.shared = 0;
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(i = 1; i <= NUMTHREADS; i++) {
		threadbag[i].started = 0;
		threadbag[i].threadnum = i;
		PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
	Sleep(1000); // Give threads time to start.
	/*
	 * Cancel one of the threads.
	 */
	PTHR4W_TEST_ASSERT(pthread_cancel(t[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t[1], NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
	cvthing.shared++;
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
	/*
	 * Signal all remaining waiting threads.
	 */
	PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cvthing.notbusy) == 0);
	/*
	 * Wait for all threads to complete.
	 */
	for(i = 2; i <= NUMTHREADS; i++)
		PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
	/*
	 * Cleanup the CV.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cvthing.notbusy) == 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here.
	PTHR4W_TEST_ASSERT(awoken == (NUMTHREADS - 1));
	return 0; // Success
}
// 
// Test Synopsis:
// - Test multiple pthread_cond_broadcasts.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Make NUMTHREADS threads wait on CV, broadcast signal them, and then repeat.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_CondVar8()
{
	static const int NUMTHREADS = 5; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 10, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			PTHR4W_TEST_ASSERT(cvthing.shared > 0);
			awoken++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	int first, last;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(first = 1, last = NUMTHREADS / 2;
	    first < NUMTHREADS;
	    first = last + 1, last = NUMTHREADS) {
		PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
		for(i = first; i <= last; i++) {
			threadbag[i].started = 0;
			threadbag[i].threadnum = i;
			PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
		}
		/*
		 * Code to control or manipulate child threads should probably go here.
		 */
		cvthing.shared = 0;
		PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
		Sleep(100); // Give threads time to start.
		PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
		cvthing.shared++;
		PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
		PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cvthing.notbusy) == 0);
		// Give threads time to complete.
		for(i = first; i <= last; i++) {
			PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0);
		}
		PTHR4W_TEST_ASSERT(awoken == (i - 1));
	}
	// Standard check that all threads started.
	for(i = 1; i <= NUMTHREADS; i++) {
		failed = !threadbag[i].started;
		if(failed) {
			slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
		}
	}
	/*
	 * Cleanup the CV.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&cvthing.notbusy) == 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here.
	PTHR4W_TEST_ASSERT(awoken == NUMTHREADS);
	return 0; // Success
}
// 
// Test Synopsis:
// - Test multiple pthread_cond_broadcasts with thread cancellation.
// Test Method (Validation or Falsification):
// - Validation
// Description:
// - Make NUMTHREADS threads wait on CV, cancel one, broadcast signal them, and then repeat.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
//
static int PThr4wTest_CondVar9()
{
	static const int NUMTHREADS = 9; // Create NUMTHREADS threads in addition to the Main thread.
	static bag_t threadbag[NUMTHREADS + 1];
	static cvthing_t cvthing = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0 };
	static pthread_mutex_t start_flag = PTHREAD_MUTEX_INITIALIZER;
	static struct timespec abstime, reltime = { 5, 0 };
	static int awoken;

	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			bag_t * bag = static_cast<bag_t *>(arg);
			PTHR4W_TEST_ASSERT(bag == &threadbag[bag->threadnum]);
			PTHR4W_TEST_ASSERT(bag->started == 0);
			bag->started = 1;
			/* Wait for the start gun */
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
			/*
			 * pthread_cond_timedwait is a cancellation point and we're
			 * going to cancel some threads deliberately.
			 */
		#ifdef _MSC_VER
		#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, (void*)&cvthing.lock);
			while(!(cvthing.shared > 0))
				PTHR4W_TEST_ASSERT(pthread_cond_timedwait(&cvthing.notbusy, &cvthing.lock, &abstime) == 0);
			pthread_cleanup_pop(0);
		#ifdef _MSC_VER
		#pragma inline_depth()
		#endif
			PTHR4W_TEST_ASSERT(cvthing.shared > 0);
			awoken++;
			bag->finished = 1;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
			return 0;
		}
	};
	int failed = 0;
	int i;
	int first, last;
	int canceledThreads = 0;
	pthread_t t[NUMTHREADS + 1];
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == PTHREAD_COND_INITIALIZER);
	PTHR4W_TEST_ASSERT(cvthing.lock == PTHREAD_MUTEX_INITIALIZER);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	PTHR4W_TEST_ASSERT((t[0] = pthread_self()).p != NULL);
	awoken = 0;
	for(first = 1, last = NUMTHREADS / 2; first < NUMTHREADS; first = last + 1, last = NUMTHREADS) {
		int ct;
		PTHR4W_TEST_ASSERT(pthread_mutex_lock(&start_flag) == 0);
		for(i = first; i <= last; i++) {
			threadbag[i].started = threadbag[i].finished = 0;
			threadbag[i].threadnum = i;
			PTHR4W_TEST_ASSERT(pthread_create(&t[i], NULL, InnerBlock::mythread, (void*)&threadbag[i]) == 0);
		}
		/*
		 * Code to control or manipulate child threads should probably go here.
		 */
		cvthing.shared = 0;
		PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&start_flag) == 0);
		Sleep(1000); // Give threads time to start.
		ct = (first + last) / 2;
		PTHR4W_TEST_ASSERT(pthread_cancel(t[ct]) == 0);
		canceledThreads++;
		PTHR4W_TEST_ASSERT(pthread_join(t[ct], NULL) == 0);
		PTHR4W_TEST_ASSERT(pthread_mutex_lock(&cvthing.lock) == 0);
		cvthing.shared++;
		PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&cvthing.lock) == 0);
		PTHR4W_TEST_ASSERT(pthread_cond_broadcast(&cvthing.notbusy) == 0);
		/*
		 * Standard check that all threads started - and wait for them to finish.
		 */
		for(i = first; i <= last; i++) {
			failed = !threadbag[i].started;
			if(failed) {
				slfprintf_stderr("Thread %d: started %d\n", i, threadbag[i].started);
			}
			else {
				PTHR4W_TEST_ASSERT(pthread_join(t[i], NULL) == 0 || threadbag[i].finished == 0);
//	      slfprintf_stderr("Thread %d: finished %d\n", i, threadbag[i].finished);
			}
		}
	}
	/*
	 * Cleanup the CV.
	 */
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&cvthing.lock) == 0);
	PTHR4W_TEST_ASSERT(cvthing.lock == NULL);
	assert_e(pthread_cond_destroy(&cvthing.notbusy), ==, 0);
	PTHR4W_TEST_ASSERT(cvthing.notbusy == NULL);
	PTHR4W_TEST_ASSERT(!failed);
	// Check any results here.
	PTHR4W_TEST_ASSERT(awoken == NUMTHREADS - canceledThreads);
	return 0; // Success
}
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
static int PThr4wTest_Robust1()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	//
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			return 0;
		}
		static void * inheritor(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	//
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	// Default (NORMAL) type 
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	Sleep(100); // @sobolev
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	// NORMAL type 
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == EPERM);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
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
static int PThr4wTest_Robust2()
{
	static int lockCount;
	static pthread_mutex_t mutex;
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;

	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);

	/* Default (NORMAL) type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* NORMAL type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == ENOTRECOVERABLE);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
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
static int PThr4wTest_Robust3()
{
	static int lockCount;
	static pthread_mutex_t mutex;

	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	/* Default (NORMAL) type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* NORMAL type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_NORMAL) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* ERRORCHECK type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);

	/* RECURSIVE type */
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 2);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
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
static int PThr4wTest_Robust4()
{
	static int lockCount;
	static pthread_mutex_t mutex[3];
	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
			lockCount++;
			Sleep(200);
			return 0;
		}
		static void * inheritor(void * arg)
		{
			int * o = static_cast<int *>(arg);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[o[0]]) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[o[1]]) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[o[2]]) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[o[2]]) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[o[1]]) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[o[0]]) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[o[2]]) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[o[1]]) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[o[0]]) == 0);
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	int order[3];
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	order[0] = 0;
	order[1] = 1;
	order[2] = 2;
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[0], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[1], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[2], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 6);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 1;
	order[1] = 0;
	order[2] = 2;
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[0], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[1], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[2], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 6);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 0;
	order[1] = 2;
	order[2] = 1;
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[0], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[1], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[2], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 6);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[2]) == 0);

	order[0] = 2;
	order[1] = 1;
	order[2] = 0;
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[0], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[1], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[2], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	Sleep(100);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, (void*)order) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 6);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
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
static int PThr4wTest_Robust5()
{
	static int lockCount;
	static pthread_mutex_t mutex[3];

	class InnerBlock {
	public:
		static void * owner(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == 0);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == 0);
			lockCount++;
			return 0;
		}
		static void * inheritor(void * arg)
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == EOWNERDEAD);
			lockCount++;
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == EOWNERDEAD);
			lockCount++;
			return 0;
		}
	};
	pthread_t to, ti;
	pthread_mutexattr_t ma;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_setrobust(&ma, PTHREAD_MUTEX_ROBUST) == 0);
	lockCount = 0;
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[0], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[1], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&mutex[2], &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&to, NULL, InnerBlock::owner, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(to, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&ti, NULL, InnerBlock::inheritor, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(ti, NULL) == 0);
	PTHR4W_TEST_ASSERT(lockCount == 6);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[0]) == EOWNERDEAD);
	PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[0]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[1]) == EOWNERDEAD);
	PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[1]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mutex[2]) == EOWNERDEAD);
	PTHR4W_TEST_ASSERT(pthread_mutex_consistent(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mutex[2]) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
	return 0;
}
// 
// Test Synopsis:
// - Stress test condition variables, mutexes, semaphores.
// Test Method (Validation or Falsification):
// - Validation
// Requirements Tested:
// - Correct accounting of semaphore and condition variable waiters.
// Description:
// Attempting to expose race conditions in cond vars, semaphores etc.
// - Master attempts to signal slave close to when timeout is due.
// - Master and slave do battle continuously until main tells them to stop.
// - Afterwards, the CV must be successfully destroyed (will return an
// error if there are waiters (including any internal semaphore waiters,
// which, if there are, cannot be real waiters).
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Pass Criteria:
// - CV is successfully destroyed.
// Fail Criteria:
// - CV destroy fails.
// 
static int PThr4wTest_Stress1()
{
	typedef struct {
		int value;
		pthread_cond_t cv;
		pthread_mutex_t mx;
	} mysig_t;
	static const uint ITERATIONS = 1000;
	static pthread_t master;
	static pthread_t slave;
	static int allExit;
	static mysig_t control = {0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
	static pthread_barrier_t startBarrier, readyBarrier, holdBarrier;
	static int timeoutCount = 0;
	static int signalsTakenCount = 0;
	static int signalsSent = 0;
	static int bias = 0;
	static int timeout = 10; // Must be > 0
	static const long NANOSEC_PER_MILLISEC = 1000000;

	class InnerBlock {
	public:
		static void * masterThread(void * arg)
		{
			int dither = (int)(size_t)arg;
			timeout = (int)(size_t)arg;
			pthread_barrier_wait(&startBarrier);
			do {
				int sleepTime;
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&control.mx) == 0);
				control.value = timeout;
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&control.mx) == 0);
				/*
				 * We are attempting to send the signal close to when the slave
				 * is due to timeout. We feel around by adding some [non-random] dither.
				 *
				 * dither is in the range 2*timeout peak-to-peak
				 * sleep time is the average of timeout plus dither.
				 * e.g.
				 * if timeout = 10 then dither = 20 and
				 * sleep millisecs is: 5 <= ms <= 15
				 *
				 * The bias value attempts to apply some negative feedback to keep
				 * the ratio of timeouts to signals taken close to 1:1.
				 * bias changes more slowly than dither so as to average more.
				 *
				 * Finally, if abs(bias) exceeds timeout then timeout is incremented.
				 */
				if(signalsSent % timeout == 0) {
					if(timeoutCount > signalsTakenCount) {
						bias++;
					}
					else if(timeoutCount < signalsTakenCount) {
						bias--;
					}
					if(bias < -timeout || bias > timeout) {
						timeout++;
					}
				}
				dither = (dither + 1 ) % (timeout * 2);
				sleepTime = (timeout - bias + dither) / 2;
				Sleep(sleepTime);
				PTHR4W_TEST_ASSERT(pthread_cond_signal(&control.cv) == 0);
				signalsSent++;
				pthread_barrier_wait(&holdBarrier);
				pthread_barrier_wait(&readyBarrier);
			} while(!allExit);
			return NULL;
		}
		static void * slaveThread(void * arg)
		{
			pthread_barrier_wait(&startBarrier);
			do {
				struct timespec abstime;
				struct timespec reltime;
				PTHR4W_TEST_ASSERT(pthread_mutex_lock(&control.mx) == 0);
				reltime.tv_sec = (control.value / 1000);
				reltime.tv_nsec = (control.value % 1000) * NANOSEC_PER_MILLISEC;
				if(pthread_cond_timedwait(&control.cv, &control.mx,
					pthread_win32_getabstime_np(&abstime, &reltime)) == ETIMEDOUT) {
					timeoutCount++;
				}
				else {
					signalsTakenCount++;
				}
				PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&control.mx) == 0);
				pthread_barrier_wait(&holdBarrier);
				pthread_barrier_wait(&readyBarrier);
			} while(!allExit);
			return NULL;
		}
	};
	uint i;
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&startBarrier, NULL, 3) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&readyBarrier, NULL, 3) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_init(&holdBarrier, NULL, 3) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&master, NULL, InnerBlock::masterThread, (void*)(size_t)timeout) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&slave, NULL, InnerBlock::slaveThread, NULL) == 0);
	allExit = FALSE;
	pthread_barrier_wait(&startBarrier);
	for(i = 1; !allExit; i++) {
		pthread_barrier_wait(&holdBarrier);
		if(i >= ITERATIONS) {
			allExit = TRUE;
		}
		pthread_barrier_wait(&readyBarrier);
	}
	PTHR4W_TEST_ASSERT(pthread_join(slave, NULL) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(master, NULL) == 0);
	printf("Signals sent = %d\nWait timeouts = %d\nSignals taken = %d\nBias = %d\nTimeout = %d\n",
	    signalsSent, timeoutCount, signalsTakenCount, (int)bias, timeout);
	/* Cleanup */
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&holdBarrier) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&readyBarrier) == 0);
	PTHR4W_TEST_ASSERT(pthread_barrier_destroy(&startBarrier) == 0);
	PTHR4W_TEST_ASSERT(pthread_cond_destroy(&control.cv) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&control.mx) == 0);
	return 0; // Success
}
#if defined(_MSC_VER) || defined(__cplusplus)
// 
// Test Synopsis: Test passing of exceptions back to the application.
// Input:
//  - None.
// Output:
// - File name, Line number, and failed expression on failure.
// - No output on success.
// Assumptions:
// - have working pthread_create, pthread_self, pthread_mutex_lock/unlock
//   pthread_testcancel, pthread_cancel, pthread_join
// Pass Criteria:
// - Process returns zero exit status.
// Fail Criteria:
// - Process returns non-zero exit status.
// 
static int PThr4wTest_Exception1()
{
	class InnerBlock {
	public:
		static void * exceptionedThread(void * arg)
		{
			int dummy = 0;
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			/* Set to async cancelable */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
			Sleep(100);
		#if defined(_MSC_VER) && !defined(__cplusplus)
			__try {
				int zero = (int)(size_t)arg; /* Passed in from arg to avoid compiler error */
				int one = 1;
				/*
				 * The deliberate exception condition (zero divide) is
				 * in an "if" to avoid being optimised out.
				 */
				if(dummy == one/zero)
					Sleep(0);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				/* Should get into here. */
				result = (void*)((int)(size_t)PTHREAD_CANCELED + 2);
			}
		#elif defined(__cplusplus)
			try
			{
				/*
				 * I had a zero divide exception here but it
				 * wasn't being caught by the catch(...)
				 * below under Mingw32. That could be a problem.
				 */
				throw dummy;
			}
		#if defined(__PtW32CatchAll)
			__PtW32CatchAll
		#else
			catch(...)
		#endif
			{
				/* Should get into here. */
				result = (void*)((int)(size_t)PTHREAD_CANCELED + 2);
			}
		#endif
			return (void*)(size_t)result;
		}

		static void * canceledThread(void * arg)
		{
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			int count;
			/* Set to async cancelable */
			PTHR4W_TEST_ASSERT(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
		#if defined(_MSC_VER) && !defined(__cplusplus)
			__try
			{
				/*
				 * We wait up to 10 seconds, waking every 0.1 seconds,
				 * for a cancellation to be applied to us.
				 */
				for(count = 0; count < 100; count++)
					Sleep(100);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				/* Should NOT get into here. */
				result = (void*)((int)(size_t)PTHREAD_CANCELED + 2);
			}
		#elif defined(__cplusplus)
			try
			{
				/*
				 * We wait up to 10 seconds, waking every 0.1 seconds,
				 * for a cancellation to be applied to us.
				 */
				for(count = 0; count < 100; count++)
					Sleep(100);
			}
		#if defined(__PtW32CatchAll)
			__PtW32CatchAll
		#else
			catch(...)
		#endif
			{
				/* Should NOT get into here. */
				result = (void*)((int)(size_t)PTHREAD_CANCELED + 2);
			}
		#endif
			return (void*)(size_t)result;
		}
	};
	const int NUMTHREADS = 4; // Create NUMTHREADS threads in addition to the Main thread.
	int failed = 0;
	int i;
	pthread_t mt;
	pthread_t et[NUMTHREADS];
	pthread_t ct[NUMTHREADS];
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	PTHR4W_TEST_ASSERT((mt = pthread_self()).p != NULL);
	for(i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, (void*)0) == 0);
		PTHR4W_TEST_ASSERT(pthread_create(&ct[i], NULL, InnerBlock::canceledThread, NULL) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(100);
	for(i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_cancel(ct[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 0; i < NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		/* Canceled thread */
		PTHR4W_TEST_ASSERT(pthread_join(ct[i], &result) == 0);
		PTHR4W_TEST_ASSERT(!(fail = (result != PTHREAD_CANCELED)));
		failed = (failed || fail);
		/* Exceptioned thread */
		PTHR4W_TEST_ASSERT(pthread_join(et[i], &result) == 0);
		PTHR4W_TEST_ASSERT(!(fail = (result != (void*)((int)(size_t)PTHREAD_CANCELED + 2))));
		failed = (failed || fail);
	}
	PTHR4W_TEST_ASSERT(!failed);
	return 0; // Success
}
// 
// Test Synopsis: Test passing of exceptions out of thread scope.
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
static int PThr4wTest_Exception2(int argc, char* argv[])
{
	class InnerBlock {
	public:
		static void * exceptionedThread(void * arg)
		{
			int dummy = 0x1;
		#if defined(_MSC_VER) && !defined(__cplusplus)
			RaiseException(dummy, 0, 0, NULL);
		#elif defined(__cplusplus)
			throw dummy;
		#endif
			return (void*)100;
		}
	};
	const int NUMTHREADS = 1; // Create NUMTHREADS threads in addition to the Main thread.
	int i;
	pthread_t mt;
	pthread_t et[NUMTHREADS];
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	if(argc <= 1) {
		int result;
		printf("You should see an \"abnormal termination\" message\n");
		fflush(stdout);
		result = system("exception2.exe die");
		printf("\"exception2.exe die\" returned status %d\n", result);
		/*
		 * result should be 0, 1 or 3 depending on build settings
		 */
		exit((result == 0 || result == 1 || result == 3) ? 0 : 1);
	}
#if defined(NO_ERROR_DIALOGS)
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif
	PTHR4W_TEST_ASSERT((mt = pthread_self()).p != NULL);
	for(i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, NULL) == 0);
	}
	Sleep(100);
	return 0; // Success
}

#else /* defined(_MSC_VER) || defined(__cplusplus) */
	static int PThr4wTest_Exception1()
	{
		slfprintf_stderr("Test N/A for this compiler environment.\n");
		return 0;
	}
	static int PThr4wTest_Exception2(int argc, char* argv[])
	{
		slfprintf_stderr("Test N/A for this compiler environment.\n");
		return 0;
	}
#endif /* defined(_MSC_VER) || defined(__cplusplus) */
// 
// Note: Due to a buggy C++ runtime in Visual Studio 2005, when we are
// built with /MD and an unhandled exception occurs, the runtime does not
// properly call the terminate handler specified by set_terminate().
// 
#if defined(__cplusplus) && !(defined(_MSC_VER) && _MSC_VER == 1400 && defined(_DLL) && !defined(_DEBUG))
// 
// Test Synopsis: Test running of user supplied terminate() function.
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
static int PThr4wTest_Exception3()
{
	static const int NUMTHREADS = 10; // Create NUMTHREADS threads in addition to the Main thread.
	static int caught = 0;
	static pthread_mutex_t caughtLock;

	class InnerBlock {
	public:
		static void terminateFunction()
		{
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&caughtLock) == 0);
			caught++;
		#if 0
			{
				FILE * fp = fopen("pthread.log", "a");
				fprintf(fp, "Caught = %d\n", caught);
				fclose(fp);
			}
		#endif
			assert_e(pthread_mutex_unlock(&caughtLock), ==, 0);
			/*
			 * Notes from the MSVC++ manual:
			 *       1) A term_func() should call exit(), otherwise
			 *          abort() will be called on return to the caller.
			 *          abort() raises SIGABRT. The default signal handler
			 *          for all signals terminates the calling program with
			 *          exit code 3.
			 *       2) A term_func() must not throw an exception. Dev: Therefore
			 *          term_func() should not call pthread_exit() if an
			 *          exception-using version of pthreads-win32 library
			 *          is being used (i.e. either pthreadVCE or pthreadVSE).
			 */
			/*
			 * Allow time for all threads to reach here before exit, otherwise
			 * threads will be terminated while holding the lock and cause
			 * the next unlock to return EPERM (we're using ERRORCHECK mutexes).
			 * Perhaps this would be a good test for robust mutexes.
			 */
			Sleep(20);
			exit(0);
		}
		static void wrongTerminateFunction()
		{
			fputs("This is not the termination routine that should have been called!\n", stderr);
			exit(1);
		}
		static void * exceptionedThread(void * arg)
		{
			int dummy = 0x1;
		#if defined (__PTW32_USES_SEPARATE_CRT) && (defined(__PTW32_CLEANUP_CXX) || defined(__PTW32_CLEANUP_SEH))
			printf("PTW32_USES_SEPARATE_CRT is defined\n");
			pthread_win32_set_terminate_np(&terminateFunction);
			set_terminate(&wrongTerminateFunction);
		#else
			set_terminate(&terminateFunction);
		#endif
			throw dummy;
			return 0;
		}
	};
	pthread_t mt;
	pthread_t et[NUMTHREADS];
	pthread_mutexattr_t ma;
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	PTHR4W_TEST_ASSERT((mt = pthread_self()).p != NULL);
	printf("See the notes inside of exception3.c re term_funcs.\n");
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutex_init(&caughtLock, &ma) == 0);
	PTHR4W_TEST_ASSERT(pthread_mutexattr_destroy(&ma) == 0);
	for(int i = 0; i < NUMTHREADS; i++) {
		PTHR4W_TEST_ASSERT(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, NULL) == 0);
	}
	while(true)
		;
	// Should never be reached.
	return 1;
}
// 
// Test Synopsis: Test running of user supplied terminate() function.
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
static int PThr4wTest_Exception30()
{
	const int NUMTHREADS = 10; // Create NUMTHREADS threads in addition to the Main thread.
	static int caught = 0;
	static CRITICAL_SECTION caughtLock;

	class InnerBlock {
	public:
		static void terminateFunction()
		{
			EnterCriticalSection(&caughtLock);
			caught++;
		#if 0
			{
				FILE * fp = fopen("pthread.log", "a");
				fprintf(fp, "Caught = %d\n", caught);
				fclose(fp);
			}
		#endif
			LeaveCriticalSection(&caughtLock);

			/*
			 * Notes from the MSVC++ manual:
			 *       1) A term_func() should call exit(), otherwise
			 *          abort() will be called on return to the caller.
			 *          abort() raises SIGABRT. The default signal handler
			 *          for all signals terminates the calling program with
			 *          exit code 3.
			 *       2) A term_func() must not throw an exception. Dev: Therefore
			 *          term_func() should not call pthread_exit() if an
			 *          exception-using version of pthreads-win32 library
			 *          is being used (i.e. either pthreadVCE or pthreadVSE).
			 */
			exit(0);
		}
		static void * exceptionedThread(void * arg)
		{
			int dummy = 0x1;
			set_terminate(&terminateFunction);
			PTHR4W_TEST_ASSERT(set_terminate(&terminateFunction) == &terminateFunction);
			throw dummy;
			return (void*)2;
		}
	};
	DWORD et[NUMTHREADS];
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	InitializeCriticalSection(&caughtLock);
	for(int i = 0; i < NUMTHREADS; i++) {
		CreateThread(NULL, //Choose default security
		    0, //Default stack size
		    (LPTHREAD_START_ROUTINE)&InnerBlock::exceptionedThread, //Routine to execute
		    NULL, //Thread parameter
		    0, //Immediately run the thread
		    &et[i] //Thread Id
		    );
	}
	Sleep(NUMTHREADS * 10);
	DeleteCriticalSection(&caughtLock);
	return 1; // Fail. Should never be reached.
}
#else /* defined(__cplusplus) */
	static int PThr4wTest_Exception3()
	{
		slfprintf_stderr("Test N/A for this compiler environment.\n");
		return 0;
	}
	static int PThr4wTest_Exception30()
	{
		slfprintf_stderr("Test N/A for this compiler environment.\n");
		return 0;
	}
#endif /* defined(__cplusplus) */
//
//
//
//
// @sobolev (yet defined at implement.h) #define  __PTW32_OBJECT_AUTO_INIT ((void*)-1)
// 
// Dummy use of j, otherwise the loop may be removed by the optimiser
// when doing the overhead timing with an empty loop.
// 
#define TESTSTART { int i, j = 0, k = 0;  __PTW32_FTIME(&currSysTimeStart); for(i = 0; i < ITERATIONS; i++) { j++;
#define TESTSTOP };  __PTW32_FTIME(&currSysTimeStop); if(j + k == i) j++; }

static BOOL (WINAPI *__ptw32_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
static HINSTANCE __ptw32_h_kernel32;
//
//
//
#define  __PTW32_MUTEX_TYPES

class BenchTestBlock {
public:
	static int old_mutex_use;
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
		if(attr && *attr && (*attr)->pshared == PTHREAD_PROCESS_SHARED) {
			result = ENOSYS;
		}
		else {
			CRITICAL_SECTION cs;
			// 
			// Load KERNEL32 and try to get address of TryEnterCriticalSection
			// 
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
			result = EINVAL;
		}
		else {
			if(*mutex == (old_mutex_t)__PTW32_OBJECT_AUTO_INIT) {
				// Don't use initialisers when benchtesting.
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
		}
		return result;
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
			// Don't use initialisers when benchtesting.
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
};

int BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32CS;
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over lock/unlock for each mutex type.
// 
static int PThr4wTest_Benchtest1()
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
			PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, mType) == 0);
		#endif
			PTHR4W_TEST_ASSERT(pthread_mutex_init(&mx, &ma) == 0);
			TESTSTART PTHR4W_TEST_ASSERT((pthread_mutex_lock(&mx), 1) == one); PTHR4W_TEST_ASSERT((pthread_mutex_unlock(&mx), 2) == two); TESTSTOP PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mx) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	int i = 0;
	CRITICAL_SECTION cs;
	BenchTestBlock::old_mutex_t ox;
	pthread_mutexattr_init(&ma);
	printf("=============================================================================\n");
	printf("\nLock plus unlock on an unlocked mutex.\n%ld iterations\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	TESTSTART PTHR4W_TEST_ASSERT(1 == one); PTHR4W_TEST_ASSERT(2 == two); TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	TESTSTART PTHR4W_TEST_ASSERT((BenchTestBlock::dummy_call(&i), 1) == one); PTHR4W_TEST_ASSERT((BenchTestBlock::dummy_call(&i), 2) == two); TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Dummy call x 2", durationMilliSecs, (float)(durationMilliSecs * 1E3 / ITERATIONS));
	TESTSTART PTHR4W_TEST_ASSERT((BenchTestBlock::interlocked_inc_with_conditionals(&i), 1) == one);
	PTHR4W_TEST_ASSERT((BenchTestBlock::interlocked_dec_with_conditionals(&i), 2) == two);
	TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Dummy call -> Interlocked with cond x 2", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	TESTSTART PTHR4W_TEST_ASSERT((InterlockedIncrement((LPLONG)&i), 1) == (LONG)one);
	PTHR4W_TEST_ASSERT((InterlockedDecrement((LPLONG)&i), 2) == (LONG)two);
	TESTSTOP
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "InterlockedOp x 2", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	InitializeCriticalSection(&cs);
	TESTSTART PTHR4W_TEST_ASSERT((EnterCriticalSection(&cs), 1) == one);
	PTHR4W_TEST_ASSERT((LeaveCriticalSection(&cs), 2) == two);
	TESTSTOP DeleteCriticalSection(&cs);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Simple Critical Section", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32CS;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	TESTSTART PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox) == zero);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox) == zero);
	TESTSTOP PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32MUTEX;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	TESTSTART PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox) == zero);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox) == zero);
	TESTSTOP PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
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
	// End of tests.
	pthread_mutexattr_destroy(&ma);
	one = i; // Dummy assignment to avoid 'variable unused' warning 
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
static int PThr4wTest_Benchtest2()
{
	static const long ITERATIONS = 100000L;
	static pthread_mutex_t gate1;
	static pthread_mutex_t gate2;
	static BenchTestBlock::old_mutex_t ox1;
	static BenchTestBlock::old_mutex_t ox2;
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
				BenchTestBlock::old_mutex_lock(&ox1);
				BenchTestBlock::old_mutex_lock(&ox2);
				BenchTestBlock::old_mutex_unlock(&ox1);
				sched_yield();
				BenchTestBlock::old_mutex_unlock(&ox2);
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
			PTHR4W_TEST_ASSERT(pthread_mutexattr_settype(&ma, mType) == 0);
		#endif
			PTHR4W_TEST_ASSERT(pthread_mutex_init(&gate1, &ma) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_init(&gate2, &ma) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&gate1) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&gate2) == 0);
			running = 1;
			PTHR4W_TEST_ASSERT(pthread_create(&worker, NULL, workerThread, NULL) == 0);
			TESTSTART
				 pthread_mutex_unlock(&gate1);
			sched_yield();
			pthread_mutex_unlock(&gate2);
			pthread_mutex_lock(&gate1);
			pthread_mutex_lock(&gate2);
			TESTSTOP
				running = 0;
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&gate2) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&gate1) == 0);
			PTHR4W_TEST_ASSERT(pthread_join(worker, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&gate2) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&gate1) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4 /* Four locks/unlocks per iteration */);
		}
	};
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
	printf("=============================================================================\n");
	printf("\nLock plus unlock on a locked mutex.\n");
	printf("%ld iterations, four locks/unlocks per iteration.\n\n", ITERATIONS);
	printf("%-45s %15s %15s\n", "Test", "Total(msec)", "average(usec)");
	printf("-----------------------------------------------------------------------------\n");
	// 
	// Time the loop overhead so we can subtract it from the actual test times.
	// 
	running = 1;
	PTHR4W_TEST_ASSERT(pthread_create(&worker, NULL, InnerBlock::overheadThread, NULL) == 0);
	TESTSTART sched_yield();
	sched_yield();
	TESTSTOP
	    running = 0;
	PTHR4W_TEST_ASSERT(pthread_join(worker, NULL) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;

	InitializeCriticalSection(&cs1);
	InitializeCriticalSection(&cs2);
	EnterCriticalSection(&cs1);
	EnterCriticalSection(&cs2);
	running = 1;
	PTHR4W_TEST_ASSERT(pthread_create(&worker, NULL, InnerBlock::CSThread, NULL) == 0);
	TESTSTART LeaveCriticalSection(&cs1);
	sched_yield();
	LeaveCriticalSection(&cs2);
	EnterCriticalSection(&cs1);
	EnterCriticalSection(&cs2);
	TESTSTOP
	    running = 0;
	LeaveCriticalSection(&cs2);
	LeaveCriticalSection(&cs1);
	PTHR4W_TEST_ASSERT(pthread_join(worker, NULL) == 0);
	DeleteCriticalSection(&cs2);
	DeleteCriticalSection(&cs1);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Simple Critical Section", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);

	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32CS;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox1, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox2, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox1) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox2) == 0);
	running = 1;
	PTHR4W_TEST_ASSERT(pthread_create(&worker, NULL, InnerBlock::oldThread, NULL) == 0);
	TESTSTART
		 BenchTestBlock::old_mutex_unlock(&ox1);
	sched_yield();
	BenchTestBlock::old_mutex_unlock(&ox2);
	BenchTestBlock::old_mutex_lock(&ox1);
	BenchTestBlock::old_mutex_lock(&ox2);
	TESTSTOP
	    running = 0;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox1) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox2) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(worker, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox2) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox1) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);

	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32MUTEX;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox1, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox2, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox1) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox2) == 0);
	running = 1;
	PTHR4W_TEST_ASSERT(pthread_create(&worker, NULL, InnerBlock::oldThread, NULL) == 0);
	TESTSTART
		 BenchTestBlock::old_mutex_unlock(&ox1);
	sched_yield();
	BenchTestBlock::old_mutex_unlock(&ox2);
	BenchTestBlock::old_mutex_lock(&ox1);
	BenchTestBlock::old_mutex_lock(&ox2);
	TESTSTOP
	    running = 0;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox1) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox2) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(worker, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox2) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox1) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Win32 Mutex (W9x)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS / 4);
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
	// End of tests.
	pthread_mutexattr_destroy(&ma);
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over a trylock on a locked mutex for each mutex type.
// 
static int PThr4wTest_Benchtest3()
{
	static const long ITERATIONS = 10000000L;
	static pthread_mutex_t mx;
	static BenchTestBlock::old_mutex_t ox;
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
			TESTSTART BenchTestBlock::old_mutex_trylock(&ox); TESTSTOP
			return NULL;
		}
		static void runTest(char * testNameString, int mType)
		{
			pthread_t t;
		#ifdef  __PTW32_MUTEX_TYPES
			pthread_mutexattr_settype(&ma, mType);
		#endif
			PTHR4W_TEST_ASSERT(pthread_mutex_init(&mx, &ma) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_lock(&mx) == 0);
			PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, trylockThread, 0) == 0);
			PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_unlock(&mx) == 0);
			PTHR4W_TEST_ASSERT(pthread_mutex_destroy(&mx) == 0);
			durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
			printf("%-45s %15ld %15.3f\n", testNameString, durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
		}
	};
	pthread_t t;
	PTHR4W_TEST_ASSERT(pthread_mutexattr_init(&ma) == 0);
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
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32CS;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::oldTrylockThread, 0) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32MUTEX;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_lock(&ox) == 0);
	PTHR4W_TEST_ASSERT(pthread_create(&t, NULL, InnerBlock::oldTrylockThread, 0) == 0);
	PTHR4W_TEST_ASSERT(pthread_join(t, NULL) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_unlock(&ox) == 0);
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
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
	// End of tests.
	pthread_mutexattr_destroy(&ma);
	return 0;
}
// 
// Measure time taken to complete an elementary operation.
// - Mutex
//   Single thread iteration over trylock/unlock for each mutex type.
// 
static int PThr4wTest_Benchtest4()
{
	static const long ITERATIONS = 10000000L;
	static pthread_mutex_t mx;
	static BenchTestBlock::old_mutex_t ox;
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
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32CS;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	TESTSTART BenchTestBlock::old_mutex_trylock(&ox); BenchTestBlock::old_mutex_unlock(&ox); TESTSTOP PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
	durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	printf("%-45s %15ld %15.3f\n", "Old PT Mutex using a Critical Section (WNT)", durationMilliSecs, (float)durationMilliSecs * 1E3 / ITERATIONS);
	BenchTestBlock::old_mutex_use = BenchTestBlock::OLD_WIN32MUTEX;
	PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_init(&ox, NULL) == 0);
	TESTSTART BenchTestBlock::old_mutex_trylock(&ox); BenchTestBlock::old_mutex_unlock(&ox); TESTSTOP PTHR4W_TEST_ASSERT(BenchTestBlock::old_mutex_destroy(&ox) == 0);
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
static int PThr4wTest_Benchtest5()
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
	TESTSTART PTHR4W_TEST_ASSERT(1 == one); TESTSTOP
    durationMilliSecs = GetDurationMilliSecs(currSysTimeStart, currSysTimeStop) - overHeadMilliSecs;
	overHeadMilliSecs = durationMilliSecs;
	// 
	// Now we can start the actual tests
	// 
	PTHR4W_TEST_ASSERT((w32sema = CreateSemaphore(NULL, (long)0, (long)ITERATIONS, NULL)) != 0);
	TESTSTART PTHR4W_TEST_ASSERT((ReleaseSemaphore(w32sema, 1, NULL), 1) == one);
	TESTSTOP PTHR4W_TEST_ASSERT(CloseHandle(w32sema) != 0);
	InnerBlock::reportTest("W32 Post with no waiters");
	PTHR4W_TEST_ASSERT((w32sema = CreateSemaphore(NULL, (long)ITERATIONS, (long)ITERATIONS, NULL)) != 0);
	TESTSTART PTHR4W_TEST_ASSERT((WaitForSingleObject(w32sema, INFINITE), 1) == one);
	TESTSTOP PTHR4W_TEST_ASSERT(CloseHandle(w32sema) != 0);
	InnerBlock::reportTest("W32 Wait without blocking");
	PTHR4W_TEST_ASSERT(sem_init(&sema, 0, 0) == 0);
	TESTSTART PTHR4W_TEST_ASSERT((sem_post(&sema), 1) == one);
	TESTSTOP PTHR4W_TEST_ASSERT(sem_destroy(&sema) == 0);
	InnerBlock::reportTest("POSIX Post with no waiters");
	PTHR4W_TEST_ASSERT(sem_init(&sema, 0, ITERATIONS) == 0);
	TESTSTART PTHR4W_TEST_ASSERT((sem_wait(&sema), 1) == one);
	TESTSTOP PTHR4W_TEST_ASSERT(sem_destroy(&sema) == 0);
	InnerBlock::reportTest("POSIX Wait without blocking");
	printf("=============================================================================\n");
	// 
	// End of tests.
	// 
	return 0;
}
// 
// This source code is taken directly from examples in the book
// Windows System Programming, Edition 4 by Johnson (John) Hart
// 
// Session 6, Chapter 10. ThreeStage.c
// 
// Several required additional header and source files from the
// book examples have been included inline to simplify building.
// The only modification to the code has been to provide default
// values when run without arguments.
// 
// Three-stage Producer Consumer system
// Other files required in this project, either directly or
// in the form of libraries (DLLs are preferable)
//   QueueObj.c (inlined here)
//   Messages.c (inlined here)
// 
// Usage: ThreeStage npc goal [display]
// start up "npc" paired producer  and consumer threads.
//   Display messages if "display" is non-zero
// Each producer must produce a total of
// "goal" messages, where each message is tagged
// with the consumer that should receive it
// Messages are sent to a "transmitter thread" which performs
// additional processing before sending message groups to the
// "receiver thread." Finally, the receiver thread sends
// the messages to the consumer threads.
// 
// Transmitter: Receive messages one at a time from producers,
// create a transmission message of up to "TBLOCK_SIZE" messages
// to be sent to the Receiver. (this could be a network xfer
// Receiver: Take message blocks sent by the Transmitter
// and send the individual messages to the designated consumer
// 
#define sleep(i) Sleep(i*1000)

#if (!defined INFINITE)
	#define INFINITE 0xFFFFFFFF
#endif

//int main(int argc, char * argv[])
static int PThr4wTest_ThreeStage(int argc, char * argv[])
{
	//#define DATA_SIZE 256
	//#define DELAY_COUNT 1000
	//#define MAX_THREADS 1024
	static const size_t DATA_SIZE = 256;
	static const int DELAY_COUNT = 1000;
	static const int MAX_THREADS = 1024;
	// 
	// Queue lengths and blocking factors. These numbers are arbitrary and
	// can be adjusted for performance tuning. The current values are
	// not well balanced.
	// 
	static const int TBLOCK_SIZE = 5;  // Transmitter combines this many messages at at time
	static const int Q_TIMEOUT = 2000; // Transmiter and receiver timeout (ms) waiting for messages
	//#define Q_TIMEOUT INFINITE
	static const int MAX_RETRY = 5;  // Number of q_get retries before quitting
	static const int P2T_QLEN = 10;  // Producer to Transmitter queue length
	static const int T2R_QLEN = 4;   // Transmitter to Receiver queue length
	static const int R2C_QLEN = 4;   // Receiver to Consumer queue length - there is one such queue for each consumer
	static const int CV_TIMEOUT = 5; // tunable parameter for the CV model

	struct THARG {
		volatile uint thread_number;
		volatile uint work_goal; // used by producers 
		volatile uint work_done; // Used by producers and consumers 
	};
	// 
	// Definitions of a synchronized, general bounded queue structure.
	// Queues are implemented as arrays with indices to youngest
	// and oldest messages, with wrap around.
	// Each queue also contains a guard mutex and "not empty" and "not full" condition variables.
	// Finally, there is a pointer to an array of messages of arbitrary type
	// 
	struct queue_t { // General purpose queue
		pthread_mutex_t q_guard; // Guard the message block
		pthread_cond_t q_ne; // Event: Queue is not empty
		pthread_cond_t q_nf; // Event: Queue is not full
		// These two events are manual-reset for the broadcast model
		// and auto-reset for the signal model 
		volatile uint q_size; /* Queue max size size          */
		volatile uint q_first; /* Index of oldest message      */
		volatile uint q_last; /* Index of youngest msg        */
		volatile uint q_destroyed;/* Q receiver has terminated  */
		void *  msg_array;/* array of q_size messages     */
	};
	struct msg_block_t { // Message block 
		pthread_mutex_t mguard; // Mutex for  the message block 
		pthread_cond_t mconsumed; // Event: Message consumed;
		// Produce a new one or stop    
		pthread_cond_t mready; // Event: Message ready
		/*
		 * Note: the mutex and events are not used by some programs, such
		 * as Program 10-3, 4, 5 (the multi-stage pipeline) as the messages
		 * are part of a protected queue
		 */
		volatile uint source; /* Creating producer identity     */
		volatile uint destination;/* Identity of receiving thread*/

		volatile uint f_consumed;
		volatile uint f_ready;
		volatile uint f_stop;
		// Consumed & ready state flags, stop flag
		volatile uint sequence; /* Message block sequence number        */
		time_t timestamp;
		uint checksum; /* Message contents checksum             */
		uint data[DATA_SIZE]; /* Message Contents               */
	};
	//
	// Grouped messages sent by the transmitter to receiver
	//
	struct /*T2R_MSG_TYPEag*/T2R_MSG_TYPE {
		volatile uint num_msgs; /* Number of messages contained */
		msg_block_t messages [TBLOCK_SIZE];
	};

	static queue_t p2tq;
	static queue_t t2rq;
	static queue_t * r2cq_array;
	// ShutDown, AllProduced are global flags to shut down the system & transmitter 
	static volatile uint ShutDown = 0;
	static volatile uint AllProduced = 0;
	static uint DisplayMessages = 0;

	class InnerBlock {
	private:
		static uint compute_checksum(void * msg, uint length)
		{
			// Computer an xor checksum on the entire message of "length" integers 
			uint cs = 0;
			uint * pint = (uint *)msg;
			for(uint i = 0; i < length; i++) {
				cs = (cs ^ *pint);
				pint++;
			}
			return cs;
		}
		static void message_fill(msg_block_t * mblock, uint src, uint dest, uint seqno)
		{
			/* Fill the message buffer, and include checksum and timestamp  */
			/* This function is called from the producer thread while it    */
			/* owns the message block mutex                                 */
			uint i;
			mblock->checksum = 0;
			for(i = 0; i < DATA_SIZE; i++) {
				mblock->data[i] = rand();
			}
			mblock->source = src;
			mblock->destination = dest;
			mblock->sequence = seqno;
			mblock->timestamp = time(NULL);
			mblock->checksum = compute_checksum(mblock, sizeof(msg_block_t)/sizeof(uint));
			/*      printf ("Generated message: %d %d %d %d %x %x\n",
						  src, dest, seqno, mblock->timestamp,
						  mblock->data[0], mblock->data[DATA_SIZE-1]);  */
			return;
		}
		static void message_display(msg_block_t * mblock)
		{
			// Display message buffer and timestamp, validate checksum
			// This function is called from the consumer thread while it
			// owns the message block mutex
			uint tcheck = compute_checksum(mblock, sizeof(msg_block_t)/sizeof(uint));
			printf("\nMessage number %d generated at: %s", mblock->sequence, ctime(&(mblock->timestamp)));
			printf("Source and destination: %d %d\n", mblock->source, mblock->destination);
			printf("First and last entries: %x %x\n", mblock->data[0], mblock->data[DATA_SIZE-1]);
			if(tcheck == 0 /*mblock->checksum was 0 when CS first computed */)
				printf("GOOD ->Checksum was validated.\n");
			else
				printf("BAD  ->Checksum failed. message was corrupted\n");
		}
	public:
		//
		// Queue management functions 
		//
		// 
		// Finite bounded queue management functions
		// q_get, q_put timeouts (max_wait) are in ms - convert to sec, rounding up
		// 
		static uint q_get(queue_t * q, void * msg, uint msize, uint MaxWait)
		{
			int tstatus = 0, got_msg = 0, time_inc = (MaxWait + 999) /1000;
			struct timespec timeout;
			timeout.tv_nsec = 0;
			if(q_destroyed(q)) 
				return 1;
			pthread_mutex_lock(&q->q_guard);
			while(q_empty(q) && 0 == tstatus) {
				if(MaxWait != INFINITE) {
					timeout.tv_sec = time(NULL) + time_inc;
					tstatus = pthread_cond_timedwait(&q->q_ne, &q->q_guard, &timeout);
				}
				else {
					tstatus = pthread_cond_wait(&q->q_ne, &q->q_guard);
				}
			}
			/* remove the message, if any, from the queue */
			if(0 == tstatus && !q_empty(q)) {
				q_remove(q, msg, msize);
				got_msg = 1;
				/* Signal that the queue is not full as we've removed a message */
				pthread_cond_broadcast(&q->q_nf);
			}
			pthread_mutex_unlock(&q->q_guard);
			return (0 == tstatus && got_msg == 1 ? 0 : MAX(1, tstatus)); /* 0 indicates success */
		}
		static uint q_put(queue_t * q, void * msg, uint msize, uint MaxWait)
		{
			int tstatus = 0, put_msg = 0, time_inc = (MaxWait + 999) /1000;
			struct timespec timeout;
			timeout.tv_nsec = 0;
			if(q_destroyed(q)) 
				return 1;
			pthread_mutex_lock(&q->q_guard);
			while(q_full(q) && 0 == tstatus) {
				if(MaxWait != INFINITE) {
					timeout.tv_sec = time(NULL) + time_inc;
					tstatus = pthread_cond_timedwait(&q->q_nf, &q->q_guard, &timeout);
				}
				else {
					tstatus = pthread_cond_wait(&q->q_nf, &q->q_guard);
				}
			}
			// Insert the message into the queue if there's room 
			if(0 == tstatus && !q_full(q)) {
				q_insert(q, msg, msize);
				put_msg = 1;
				// Signal that the queue is not empty as we've inserted a message 
				pthread_cond_broadcast(&q->q_ne);
			}
			pthread_mutex_unlock(&q->q_guard);
			return (0 == tstatus && put_msg == 1 ? 0 : MAX(1, tstatus)); /* 0 indictates success */
		}
		static uint q_initialize(queue_t * q, uint msize, uint nmsgs)
		{
			// Initialize queue, including its mutex and events 
			// Allocate storage for all messages. 
			q->q_first = q->q_last = 0;
			q->q_size = nmsgs;
			q->q_destroyed = 0;
			pthread_mutex_init(&q->q_guard, NULL);
			pthread_cond_init(&q->q_ne, NULL);
			pthread_cond_init(&q->q_nf, NULL);
			if((q->msg_array = SAlloc::C(nmsgs, msize)) == NULL) 
				return 1;
			return 0; /* No error */
		}
		static uint q_destroy(queue_t * q)
		{
			if(q_destroyed(q)) 
				return 1;
			// Free all the resources created by q_initialize 
			pthread_mutex_lock(&q->q_guard);
			q->q_destroyed = 1;
			SAlloc::F(q->msg_array);
			pthread_cond_destroy(&q->q_ne);
			pthread_cond_destroy(&q->q_nf);
			pthread_mutex_unlock(&q->q_guard);
			pthread_mutex_destroy(&q->q_guard);
			return 0;
		}
		static uint q_destroyed(queue_t * q)
		{
			return (q->q_destroyed);
		}
		static uint q_empty(queue_t * q)
		{
			return (q->q_first == q->q_last);
		}
		static uint q_full(queue_t * q)
		{
			return ((q->q_first - q->q_last) == 1 || (q->q_last == q->q_size-1 && q->q_first == 0));
		}
		static uint q_remove(queue_t * q, void * msg, uint msize)
		{
			char * pm = (char*)q->msg_array;
			// Remove oldest ("first") message 
			memcpy(msg, pm + (q->q_first * msize), msize);
			// Invalidate the message
			q->q_first = ((q->q_first + 1) % q->q_size);
			return 0; /* no error */
		}
		static uint q_insert(queue_t * q, void * msg, uint msize)
		{
			char * pm = (char*)q->msg_array;
			// Add a new youngest ("last") message 
			if(q_full(q)) 
				return 1; /* Error - Q is full */
			memcpy(pm + (q->q_last * msize), msg, msize);
			q->q_last = ((q->q_last + 1) % q->q_size);
			return 0;
		}
		static void * producer(void * arg)
		{
			THARG * parg;
			uint ithread, tstatus = 0;
			msg_block_t msg;
			parg = (THARG*)arg;
			ithread = parg->thread_number;
			while(parg->work_done < parg->work_goal && !ShutDown) {
				/* Periodically produce work units until the goal is satisfied */
				/* messages receive a source and destination address which are */
				/* the same in this case but could, in general, be different. */
				sleep(rand()/100000000);
				message_fill(&msg, ithread, ithread, parg->work_done);
				/* put the message in the queue - Use an infinite timeout to assure
				 * that the message is inserted, even if consumers are delayed */
				tstatus = InnerBlock::q_put(&p2tq, &msg, sizeof(msg), INFINITE);
				if(0 == tstatus) {
					parg->work_done++;
				}
			}
			return 0;
		}
		static void * consumer(void * arg)
		{
			uint tstatus = 0, Retries = 0;
			msg_block_t msg;
			queue_t * pr2cq;
			THARG * carg = (THARG*)arg;
			uint ithread = carg->thread_number;
			carg = (THARG*)arg;
			pr2cq = &r2cq_array[ithread];
			while(carg->work_done < carg->work_goal && Retries < MAX_RETRY && !ShutDown) {
				/* Receive and display/process messages */
				/* Try to receive the requested number of messages,
				 * but allow for early system shutdown */
				tstatus = InnerBlock::q_get(pr2cq, &msg, sizeof(msg), Q_TIMEOUT);
				if(0 == tstatus) {
					if(DisplayMessages > 0) 
						message_display(&msg);
					carg->work_done++;
					Retries = 0;
				}
				else {
					Retries++;
				}
			}
			return NULL;
		}
		static void * transmitter(void * arg)
		{
			// Obtain multiple producer messages, combining into a single
			// compound message for the receiver
			uint tstatus = 0, im, Retries = 0;
			T2R_MSG_TYPE t2r_msg = {0};
			msg_block_t p2t_msg;
			while(!ShutDown && !AllProduced) {
				t2r_msg.num_msgs = 0;
				// pack the messages for transmission to the receiver 
				im = 0;
				while(im < TBLOCK_SIZE && !ShutDown && Retries < MAX_RETRY && !AllProduced) {
					tstatus = InnerBlock::q_get(&p2tq, &p2t_msg, sizeof(p2t_msg), Q_TIMEOUT);
					if(0 == tstatus) {
						memcpy(&t2r_msg.messages[im], &p2t_msg, sizeof(p2t_msg));
						t2r_msg.num_msgs++;
						im++;
						Retries = 0;
					}
					else { // Timed out
						Retries++;
					}
				}
				tstatus = InnerBlock::q_put(&t2rq, &t2r_msg, sizeof(t2r_msg), INFINITE);
				if(tstatus != 0) 
					return NULL;
			}
			return NULL;
		}
		static void * receiver(void * arg)
		{
			// Obtain compound messages from the transmitter and unblock them
			// and transmit to the designated consumer.
			uint tstatus = 0, im, ic, Retries = 0;
			T2R_MSG_TYPE t2r_msg;
			msg_block_t r2c_msg;
			while(!ShutDown && Retries < MAX_RETRY) {
				tstatus = InnerBlock::q_get(&t2rq, &t2r_msg, sizeof(t2r_msg), Q_TIMEOUT);
				if(tstatus != 0) { /* Timeout - Have the producers shut down? */
					Retries++;
					continue;
				}
				Retries = 0;
				/* Distribute the packaged messages to the proper consumer */
				im = 0;
				while(im < t2r_msg.num_msgs) {
					memcpy(&r2c_msg, &t2r_msg.messages[im], sizeof(r2c_msg));
					ic = r2c_msg.destination; /* Destination consumer */
					tstatus = InnerBlock::q_put(&r2cq_array[ic], &r2c_msg, sizeof(r2c_msg), INFINITE);
					if(0 == tstatus) 
						im++;
				}
			}
			return NULL;
		}
	};
	uint tstatus = 0;
	uint nthread;
	uint ithread;
	uint goal;
	uint thid;
	pthread_t * producer_th, * consumer_th, transmitter_th, receiver_th;
	THARG * producer_arg, * consumer_arg;
	if(argc < 3) {
		nthread = 32;
		goal = 1000;
	}
	else {
		nthread = atoi(argv[1]);
		goal = atoi(argv[2]);
		if(argc >= 4)
			DisplayMessages = atoi(argv[3]);
	}
	srand((int)time(NULL));   /* Seed the RN generator */
	if(nthread > MAX_THREADS) {
		printf("Maximum number of producers or consumers is %d.\n", MAX_THREADS);
		return 2;
	}
	producer_th = (pthread_t *)SAlloc::M(nthread * sizeof(pthread_t));
	producer_arg = (THARG *)SAlloc::C(nthread, sizeof(THARG));
	consumer_th = (pthread_t *)SAlloc::M(nthread * sizeof(pthread_t));
	consumer_arg = (THARG *)SAlloc::C(nthread, sizeof(THARG));
	if(producer_th == NULL || producer_arg == NULL || consumer_th == NULL || consumer_arg == NULL)
		perror("Cannot allocate working memory for threads.");
	InnerBlock::q_initialize(&p2tq, sizeof(msg_block_t), P2T_QLEN);
	InnerBlock::q_initialize(&t2rq, sizeof(T2R_MSG_TYPE), T2R_QLEN);
	// Allocate and initialize Receiver to Consumer queue for each consumer 
	r2cq_array = (queue_t*)SAlloc::C(nthread, sizeof(queue_t));
	if(r2cq_array == NULL) perror("Cannot allocate memory for r2c queues");
	for(ithread = 0; ithread < nthread; ithread++) {
		// Initialize r2c queue for this consumer thread 
		InnerBlock::q_initialize(&r2cq_array[ithread], sizeof(msg_block_t), R2C_QLEN);
		// Fill in the thread arg 
		consumer_arg[ithread].thread_number = ithread;
		consumer_arg[ithread].work_goal = goal;
		consumer_arg[ithread].work_done = 0;
		tstatus = pthread_create(&consumer_th[ithread], NULL, InnerBlock::consumer, (void*)&consumer_arg[ithread]);
		if(tstatus != 0)
			perror("Cannot create consumer thread");
		producer_arg[ithread].thread_number = ithread;
		producer_arg[ithread].work_goal = goal;
		producer_arg[ithread].work_done = 0;
		tstatus = pthread_create(&producer_th[ithread], NULL, InnerBlock::producer, (void*)&producer_arg[ithread]);
		if(tstatus != 0)
			perror("Cannot create producer thread");
	}
	tstatus = pthread_create(&transmitter_th, NULL, InnerBlock::transmitter, &thid);
	if(tstatus != 0)
		perror("Cannot create tranmitter thread");
	tstatus = pthread_create(&receiver_th, NULL, InnerBlock::receiver, &thid);
	if(tstatus != 0)
		perror("Cannot create receiver thread");
	printf("BOSS: All threads are running\n");
	/* Wait for the producers to complete */
	/* The implementation allows too many threads for WaitForMultipleObjects */
	/* although you could call WFMO in a loop */
	for(ithread = 0; ithread < nthread; ithread++) {
		tstatus = pthread_join(producer_th[ithread], NULL);
		if(tstatus != 0)
			perror("Cannot wait for producer thread");
		printf("BOSS: Producer %d produced %d work units\n", ithread, producer_arg[ithread].work_done);
	}
	/* Producers have completed their work. */
	printf("BOSS: All producers have completed their work.\n");
	AllProduced = 1;
	/* Wait for the consumers to complete */
	for(ithread = 0; ithread < nthread; ithread++) {
		tstatus = pthread_join(consumer_th[ithread], NULL);
		if(tstatus != 0)
			perror("Cannot wait for consumer thread");
		printf("BOSS: consumer %d consumed %d work units\n", ithread, consumer_arg[ithread].work_done);
	}
	printf("BOSS: All consumers have completed their work.\n");
	ShutDown = 1; /* Set a shutdown flag - All messages have been consumed */
	// Wait for the transmitter and receiver 
	tstatus = pthread_join(transmitter_th, NULL);
	if(tstatus != 0)
		perror("Failed waiting for transmitter");
	tstatus = pthread_join(receiver_th, NULL);
	if(tstatus != 0)
		perror("Failed waiting for receiver");
	InnerBlock::q_destroy(&p2tq);
	InnerBlock::q_destroy(&t2rq);
	for(ithread = 0; ithread < nthread; ithread++)
		InnerBlock::q_destroy(&r2cq_array[ithread]);
	SAlloc::F(r2cq_array);
	SAlloc::F(producer_th);
	SAlloc::F(consumer_th);
	SAlloc::F(producer_arg);
	SAlloc::F(consumer_arg);
	printf("System has finished. Shutting down\n");
	return 0;
}

int DoTest_PThr4w()
{
	pthread_win32_process_attach_np();
	//PThr4wTest_Robust1();
	//PThr4wTest_Robust2();
	//PThr4wTest_Robust3();
	//PThr4wTest_Robust4();
	//PThr4wTest_Robust5();
	PThr4wTest_Sizes();
	PThr4wTest_Eyal1();
	//PThr4wTest_Sequence1();
	PThr4wTest_TryEnterCs1();
	PThr4wTest_TryEnterCs2();
	PThr4wTest_Timeouts();
	PThr4wTest_Stress1();
	PThr4wTest_Affinity1();
	PThr4wTest_Affinity2();
	PThr4wTest_Affinity3();
	PThr4wTest_Affinity4();
	PThr4wTest_Affinity5();
	PThr4wTest_Affinity6();
	PThr4wTest_Join0();
	PThr4wTest_Join1();
	PThr4wTest_Join2();
	PThr4wTest_Join3();
	PThr4wTest_Join4();
	PThr4wTest_Create1();
	PThr4wTest_Create2();
	PThr4wTest_Create3();
	PThr4wTest_Kill1();
	PThr4wTest_Delay1();
	PThr4wTest_Delay2();
	PThr4wTest_Tsd1();
	PThr4wTest_Tsd2();
	PThr4wTest_Tsd3();
	PThr4wTest_Valid1();
	PThr4wTest_Valid2();
	PThr4wTest_Once1();
	PThr4wTest_Once2();
	PThr4wTest_Once3();
	PThr4wTest_Once4();
	PThr4wTest_Barrier1();
	PThr4wTest_Barrier2();
	PThr4wTest_Barrier3();
	PThr4wTest_Barrier4();
	PThr4wTest_Barrier5();
	PThr4wTest_Barrier6();
	PThr4wTest_Mutex1();
	PThr4wTest_Mutex2();
	PThr4wTest_Mutex2e();
	PThr4wTest_Mutex2r();
	PThr4wTest_Mutex3();
	PThr4wTest_Mutex3e();
	PThr4wTest_Mutex3r();
	PThr4wTest_Mutex4();
	PThr4wTest_Mutex5();
	PThr4wTest_Mutex6();
	PThr4wTest_Mutex6e();
	PThr4wTest_Mutex6es();
	PThr4wTest_Mutex6n();
	PThr4wTest_Mutex6r();
	PThr4wTest_Mutex6rs();
	PThr4wTest_Mutex6s();
	PThr4wTest_Mutex7();
	PThr4wTest_Mutex7e();
	PThr4wTest_Mutex7n();
	PThr4wTest_Mutex7r();
	PThr4wTest_Mutex8();
	PThr4wTest_Mutex8e();
	PThr4wTest_Mutex8n();
	PThr4wTest_Mutex8r();
	PThr4wTest_Mutex1e();
	PThr4wTest_Mutex1n();
	PThr4wTest_Mutex1r();
	PThr4wTest_Semaphore1();
	PThr4wTest_Semaphore2();
	PThr4wTest_Semaphore3();
	PThr4wTest_Semaphore4();
	PThr4wTest_Semaphore4t();
	PThr4wTest_Semaphore5();
	PThr4wTest_RwLock1();
	PThr4wTest_RwLock2();
	PThr4wTest_RwLock2t();
	PThr4wTest_RwLock3();
	PThr4wTest_RwLock3t();
	PThr4wTest_RwLock4();
	PThr4wTest_RwLock4t();
	PThr4wTest_RwLock5();
	PThr4wTest_RwLock5t();
	PThr4wTest_RwLock6();
	PThr4wTest_RwLock6t();
	PThr4wTest_RwLock6t2();
	PThr4wTest_RwLock7();
	PThr4wTest_RwLock71();
	PThr4wTest_RwLock8();
	PThr4wTest_RwLock81();
	PThr4wTest_CondVar1();
	PThr4wTest_CondVar11();
	PThr4wTest_CondVar12();
	PThr4wTest_CondVar2();
	PThr4wTest_CondVar21();
	PThr4wTest_CondVar3();
	PThr4wTest_CondVar31();
	PThr4wTest_CondVar32();
	PThr4wTest_CondVar33();
	PThr4wTest_CondVar4();
	PThr4wTest_CondVar5();
	PThr4wTest_CondVar6();
	PThr4wTest_CondVar7();
	PThr4wTest_CondVar8();
	PThr4wTest_CondVar9();
	PThr4wTest_Spin1();
	PThr4wTest_Spin2();
	PThr4wTest_Spin3();
	PThr4wTest_Spin4();
	PThr4wTest_CleanUp0();
	PThr4wTest_CleanUp1();
	PThr4wTest_CleanUp2();
	PThr4wTest_CleanUp3();
	PThr4wTest_Equal0();
	PThr4wTest_Equal1();
	PThr4wTest_Errno0();
	PThr4wTest_Errno1();
	PThr4wTest_Exception1();
	/*{
		char * pp_argv[] = { "PThr4wTest_Exception2", "dummy" };
		PThr4wTest_Exception2(SIZEOFARRAY(pp_argv), pp_argv);
	}*/
	// (trouble - unhandled exception) PThr4wTest_Exception3();
	// (trouble - unhandled exception) PThr4wTest_Exception30();
	PThr4wTest_Inherit1();
	PThr4wTest_NameNp1();
	PThr4wTest_NameNp2();
	PThr4wTest_Context1();
	PThr4wTest_Context2();
	PThr4wTest_Count1();

	PThr4wTest_Priority1();
	PThr4wTest_Priority2();
	// (trouble - unhandled exception) PThr4wTest_Reinit1();
	// (fail) PThr4wTest_Reuse1();
	PThr4wTest_Reuse2();
	PThr4wTest_Self1();
	PThr4wTest_Self2();

	PThr4wTest_Cancel1();
	PThr4wTest_Cancel2();
	PThr4wTest_Cancel3();
	PThr4wTest_Cancel4();
	PThr4wTest_Cancel5();
	PThr4wTest_Cancel6a();
	PThr4wTest_Cancel6d();
	// (fail) PThr4wTest_Cancel7();
	// (fail) PThr4wTest_Cancel8();
	PThr4wTest_Cancel9();
	// PThr4wTest_Detach1(); // fail
	// (trouble - exit) PThr4wTest_Exit1();
	PThr4wTest_Exit2();
	PThr4wTest_Exit3();
	PThr4wTest_Exit4();
	// (fail) PThr4wTest_Exit5();
	PThr4wTest_Exit6();
	PThr4wTest_Benchtest1();
	PThr4wTest_Benchtest2();
	PThr4wTest_Benchtest3();
	PThr4wTest_Benchtest4();
	PThr4wTest_Benchtest5();
	//PThr4wTest_ThreeStage(int argc, char * argv[]);
	return 0;
}
