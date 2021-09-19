// PTHREAD4W-TEST.CPP
//
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "sltest/test.h"

static const int PTW32TEST_THREAD_INIT_PRIO = 0;
static const int PTW32TEST_MAXPRIORITIES = 512;
#define SEMAPHORE_MAX_COUNT 100

int assertE;

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

void Implement_AssertE(bool condition, int iE, const char * pE, const char * pO, const char * pR, const char * pFile, const int line) 
{
	assertE = iE;
	if(condition) {
		if(ASSERT_TRACE) {
			fprintf(stderr, "Assertion succeeded: (%s), file %s, line %d\n", pE, pFile, line);
			fflush(stderr);
		}
	}
	else {
		if(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) {
			fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %s\n", pE, pO, pR, pFile, line, PThr4wErrorString[assertE]);
		}
		else {
			fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %d\n", pE, pO, pR, pFile, line, assertE);
		}
		exit(1);
	}
	//(((assertE = e) o (r)) ? ((ASSERT_TRACE) ? fprintf(stderr, "Assertion succeeded: (%s), file %s, line %d\n", \
	//#e, __FILE__, (int)__LINE__), fflush(stderr) : 0) : \
	//(assertE <= (int)(sizeof(PThr4wErrorString)/sizeof(PThr4wErrorString[0]))) ? \
	//(fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %s\n", #e,#o,#r, __FILE__, (int) __LINE__, error_string[assertE]), exit(1), 0) :\
	//(fprintf(stderr, "Assertion failed: (%s %s %s), file %s, line %d, error %d\n", #e,#o,#r, __FILE__, (int) __LINE__, assertE), exit(1), 0))
}

int PThr4wTest_Sizes()
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
// Test for pthread_equal
// Depends on functions: pthread_self().
//
int PThr4wTest_Equal0()
{
	pthread_t t1 = pthread_self();
	assert(pthread_equal(t1, pthread_self()) != 0);
	return 0; // Success
}
//
// Test for pthread_equal
// Depends on functions: pthread_create().
//
int PThr4wTest_Equal1()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(2000);
			return 0;
		}
	};
	pthread_t t1, t2;
	assert(pthread_create(&t1, NULL, InnerBlock::func, (void*)1) == 0);
	assert(pthread_create(&t2, NULL, InnerBlock::func, (void*)2) == 0);
	assert(pthread_equal(t1, t2) == 0);
	assert(pthread_equal(t1, t1) != 0);
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
int PThr4wTest_Kill1()
{
	assert(pthread_kill(pthread_self(), 1) == EINVAL);
	return 0;
}
//
// Descr: Test some basic assertions about the number of threads at runtime.
//
int PThr4wTest_Count1()
{
	static const int NUMTHREADS = 30;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	static pthread_t threads[NUMTHREADS];
	static uint numThreads = 0;

	class InnerBlock {
	public:
		static void * myfunc(void * arg)
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
		assert(pthread_create(&threads[i], NULL, InnerBlock::myfunc, 0) == 0);
	}
	// Wait for all the threads to exit.
	for(i = 0; i < SIZEOFARRAY(threads); i++) {
		assert(pthread_join(threads[i], NULL) == 0);
	}
	// Check the number of threads created.
	assert((int)numThreads == SIZEOFARRAY(threads));
	return 0; // Success
}
//
// Depends on API functions: pthread_delay_np
//
int PThr4wTest_Delay1()
{
	struct timespec interval = {1L, 500000000L};
	assert(pthread_delay_np(&interval) == 0);
	return 0;
}
//
// Depends on API functions: pthread_delay_np
//
int PThr4wTest_Delay2()
{
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			struct timespec interval = {5, 500000000L};
			assert(pthread_mutex_lock(&mx) == 0);
		#ifdef _MSC_VER
			#pragma inline_depth(0)
		#endif
			pthread_cleanup_push(pthread_mutex_unlock, &mx);
			assert(pthread_delay_np(&interval) == 0);
			pthread_cleanup_pop(1);
		#ifdef _MSC_VER
			#pragma inline_depth()
		#endif
			return (void*)(size_t)1;
		}
	};
	pthread_t t;
	void * result = (void*)0;
	assert(pthread_mutex_lock(&mx) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_cancel(t) == 0);
	assert(pthread_mutex_unlock(&mx) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(result == (void*)PTHREAD_CANCELED);
	return 0;
}
//
// Depends on API functions: pthread_create(), pthread_detach(), pthread_exit().
//
int PThr4wTest_Detach1()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
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
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_create(&id[i], NULL, InnerBlock::func, reinterpret_cast<void *>(i)) == 0);
	}
	// Some threads will finish before they are detached, some after. 
	Sleep(NUMTHREADS/2 * 10 + 50);
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_detach(id[i]) == 0);
	}
	Sleep(NUMTHREADS * 10 + 100);
	// 
	// Check that all threads are now invalid.
	// This relies on unique thread IDs - e.g. works with
	// pthreads-w32 or Solaris, but may not work for Linux, BSD etc.
	// 
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_kill(id[i], 0) == ESRCH);
	}
	return 0; // Success
}
// 
// Test for pthread_self().
// Depends on API functions: pthread_self()
// Implicitly depends on: pthread_getspecific(), pthread_setspecific()
// 
int PThr4wTest_Self1()
{
	// 
	// This should always succeed unless the system has no resources (memory) left.
	// 
	pthread_t self;
#if defined (__PTW32_STATIC_LIB) && !(defined(_MSC_VER) || defined(__MINGW32__))
	pthread_win32_process_attach_np();
#endif
	self = pthread_self();
	assert(self.p != NULL);
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
int PThr4wTest_Self2()
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
	assert(pthread_create(&t, NULL, InnerBlock::entry, NULL) == 0);
	Sleep(100);
	assert(pthread_equal(t, me) != 0);
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
int PThr4wTest_Valid1()
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
int PThr4wTest_Sequence1()
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
		static void * func(void * arg)
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
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
	memzero(seqmap, sizeof(seqmap));
	for(i = 0; i < NUMTHREADS; i++) {
		if(NUMTHREADS/2 == i) {
			// Include this main thread, which will be an implicit pthread_t 
			seqmap[(int)pthread_getunique_np(pthread_self())] = 1;
		}
		assert(pthread_create(&t[i], &attr, InnerBlock::func, NULL) == 0);
	}
	while(NUMTHREADS > InterlockedExchangeAdd((LPLONG)&done, 0L))
		Sleep(100);
	Sleep(100);
	assert(seqmap[0] == 0);
	for(i = 1; i < NUMTHREADS+2; i++) {
		assert(seqmap[i] == 1);
	}
	return 0;
}
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
int PThr4wTest_Context1()
{
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
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
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
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
	assert(washere == 2);
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
int PThr4wTest_Context2()
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
		static void * func(void * arg)
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
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
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
	assert(washere == 2);
	return 0;
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create(), pthread_exit().
//
int PThr4wTest_Join0()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(2000);
			pthread_exit(arg);
			exit(1); // Never reached
		}
	};
	pthread_t id;
	void* result = (void*)0;
	// Create a single thread and wait for it to exit. 
	assert(pthread_create(&id, NULL, InnerBlock::func, (void*)123) == 0);
	assert(pthread_join(id, &result) == 0);
	assert((int)(size_t)result == 123);
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create(), pthread_join(), pthread_exit().
//
int PThr4wTest_Join1()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
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
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	// Some threads will finish before they are joined, some after. 
	Sleep(2 * 100 + 50);
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join2()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
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
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join3()
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
		assert(pthread_create(&id[i], NULL, InnerBlock::func, (void*)(size_t)i) == 0);
	}
	/*
	 * Let threads exit before we join them.
	 * We should still retrieve the exit code for the threads.
	 */
	Sleep(1000);
	for(i = 0; i < 4; i++) {
		assert(pthread_join(id[i], &result) == 0);
		assert((int)(size_t)result == i);
	}
	return 0; // Success
}
//
// Test for pthread_join()
// Depends on API functions: pthread_create().
//
int PThr4wTest_Join4()
{
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			Sleep(1200);
			return arg;
		}
	};
	pthread_t id;
	struct timespec abstime, reltime = { 1, 0 };
	void * result = (void*)-1;
	assert(pthread_create(&id, NULL, InnerBlock::func, (void*)(size_t)999) == 0);
	/*
	 * Let thread start before we attempt to join it.
	 */
	Sleep(100);
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	/* Test for pthread_timedjoin_np timeout */
	assert(pthread_timedjoin_np(id, &result, &abstime) == ETIMEDOUT);
	assert((int)(size_t)result == -1);
	/* Test for pthread_tryjoin_np behaviour before thread has exited */
	assert(pthread_tryjoin_np(id, &result) == EBUSY);
	assert((int)(size_t)result == -1);
	Sleep(500);
	/* Test for pthread_tryjoin_np behaviour after thread has exited */
	assert(pthread_tryjoin_np(id, &result) == 0);
	assert((int)(size_t)result == 999);
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
int PThr4wTest_Reuse1()
{
	const int NUMTHREADS = 100;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			washere = 1;
			return arg;
		}
	};
	pthread_t t, last_t;
	pthread_attr_t attr;
	void * result = NULL;
	int i;
	assert(pthread_attr_init(&attr) == 0);;
	assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0);
	washere = 0;
	assert(pthread_create(&t, &attr, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, &result) == 0);;
	assert((int)(size_t)result == 0);
	assert(washere == 1);
	last_t = t;
	for(i = 1; i < NUMTHREADS; i++) {
		washere = 0;
		assert(pthread_create(&t, &attr, InnerBlock::func, (void*)(size_t)i) == 0);
		pthread_join(t, &result);
		assert((int)(size_t)result == i);
		assert(washere == 1);
		/* thread IDs should be unique */
		assert(!pthread_equal(t, last_t));
		/* thread struct pointers should be the same */
		assert(t.p == last_t.p);
		/* thread handle reuse counter should be different by one */
		assert(t.x == last_t.x+1);
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
int PThr4wTest_Reuse2()
{
	const int NUMTHREADS = 10000;
	static long done = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
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
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);
	for(i = 0; i < NUMTHREADS; i++) {
		while(pthread_create(&t[i], &attr, InnerBlock::func, NULL) != 0)
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
int PThr4wTest_Priority1()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int policy;
			struct sched_param param;
			pthread_t threadID = pthread_self();
			assert(pthread_getschedparam(threadID, &policy, &param) == 0);
			assert(policy == SCHED_OTHER);
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
	assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	assert(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);
	/* Set the thread's priority to a known initial value. */
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	printf("Using pthread_getschedparam\n");
	printf("%10s %10s %10s\n", "Set value", "Get value", "Win priority");
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		int prio;
		assert(pthread_attr_setschedparam(&attr, &param) == 0);
		assert(pthread_create(&t, &attr, InnerBlock::func, (void*)&attr) == 0);
		assert((prio = GetThreadPriority(pthread_getw32threadhandle_np(t))) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		assert(pthread_join(t, &result) == 0);
		assert(param.sched_priority == (int)(size_t)result);
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
int PThr4wTest_Priority2()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];
	static pthread_barrier_t startBarrier;
	static pthread_barrier_t endBarrier;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int policy;
			struct sched_param param;
			int result = pthread_barrier_wait(&startBarrier);
			assert(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
			assert(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
			assert(policy == SCHED_OTHER);
			result = pthread_barrier_wait(&endBarrier);
			assert(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
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
				/*
				 * If prioSet is invalid then the threads priority is unchanged
				 * from the previous value. Make the previous value a known
				 * one so that we can check later.
				 */
				param.sched_priority = prioSet;
				assert(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
				validPriorities[prioSet+(PTW32TEST_MAXPRIORITIES/2)] = GetThreadPriority(threadH);
			}
			return 0;
		}
	};
	pthread_t t;
	void * result = NULL;
	int result2;
	struct sched_param param;
	assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	assert(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(pthread_barrier_init(&startBarrier, NULL, 2) == 0);
	assert(pthread_barrier_init(&endBarrier, NULL, 2) == 0);
	/* Set the thread's priority to a known initial value.
	 * If the new priority is invalid then the threads priority
	 * is unchanged from the previous value.
	 */
	SetThreadPriority(pthread_getw32threadhandle_np(pthread_self()), PTW32TEST_THREAD_INIT_PRIO);
	for(param.sched_priority = minPrio; param.sched_priority <= maxPrio; param.sched_priority++) {
		assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
		assert(pthread_setschedparam(t, SCHED_OTHER, &param) == 0);
		result2 = pthread_barrier_wait(&startBarrier);
		assert(result2 == 0 || result2 == PTHREAD_BARRIER_SERIAL_THREAD);
		result2 = pthread_barrier_wait(&endBarrier);
		assert(result2 == 0 || result2 == PTHREAD_BARRIER_SERIAL_THREAD);
		assert(GetThreadPriority(pthread_getw32threadhandle_np(t)) == validPriorities[param.sched_priority+(PTW32TEST_MAXPRIORITIES/2)]);
		pthread_join(t, &result);
		assert(param.sched_priority == (int)(size_t)result);
	}
	return 0;
}
// 
// Create a static pthread_once and test that it calls myfunc once.
// Depends on API functions: pthread_once(), pthread_create()
// 
int PThr4wTest_Once1()
{
	static pthread_once_t once = PTHREAD_ONCE_INIT;
	static int washere = 0;

	class InnerBlock {
	public:
		static void myfunc(void)
		{
			washere++;
		}
		static void * mythread(void * arg)
		{
			assert(pthread_once(&once, myfunc) == 0);
			return 0;
		}
	};
	pthread_t t1, t2;
	assert(pthread_create(&t1, NULL, InnerBlock::mythread, NULL) == 0);
	assert(pthread_create(&t2, NULL, InnerBlock::mythread, NULL) == 0);
	Sleep(2000);
	assert(washere == 1);
	return 0;
}
// 
// Create several static pthread_once objects and channel several threads through each.
// Depends on API functions: pthread_once(), pthread_create()
//
int PThr4wTest_Once2()
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
			/* Simulate slow once routine so that following threads pile up behind it */
			Sleep(100);
		}
	public:
		static void * mythread(void * arg)
		{
			assert(pthread_once(&once[(int)(size_t)arg], myfunc) == 0);
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
			/* GCC build: create was failing with EAGAIN after 790 threads */
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::mythread, (void*)(size_t)j))
				sched_yield();
		}
	}
	for(j = 0; j < NUM_ONCE; j++)
		for(i = 0; i < NUM_THREADS; i++)
			if(pthread_join(t[i][j], NULL) != 0)
				printf("Join failed for [thread,once] = [%d,%d]\n", i, j);

	assert(numOnce.i == NUM_ONCE);
	assert(numThreads.i == NUM_THREADS * NUM_ONCE);
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
int PThr4wTest_Once3()
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
			assert(numOnce.i > 0);
			LeaveCriticalSection(&numOnce.cs);
			/* Simulate slow once routine so that following threads pile up behind it */
			Sleep(10);
			/* Test for cancellation late so we're sure to have waiters. */
			pthread_testcancel();
		}
	public:
		static void * mythread(void * arg)
		{
			/*
			 * Cancel every thread. These threads are deferred cancelable only, so
			 * this thread will see it only when it performs the once routine (my_func).
			 * The result will be that every thread eventually cancels only when it
			 * becomes the new 'once' thread.
			 */
			assert(pthread_cancel(pthread_self()) == 0);
			/*
			 * Now we block on the 'once' control.
			 */
			assert(pthread_once(&once[(int)(size_t)arg], myfunc) == 0);
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
			/* GCC build: create was failing with EAGAIN after 790 threads */
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::mythread, (void*)(size_t)j))
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
	assert(numOnce.i == NUM_ONCE * NUM_THREADS);
	assert(numThreads.i == 0);
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
int PThr4wTest_Once4()
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
		static void * mythread(void * arg)
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
			assert(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
			pthread_cleanup_pop(1);
		#else
			assert(pthread_once(&once[bag->oncenum], myinitfunc) == 0);
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
			/* GCC build: create was failing with EAGAIN after 790 threads */
			while(0 != pthread_create(&t[i][j], NULL, InnerBlock::mythread, (void*)bag))
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
	assert(numOnce.i == NUM_ONCE * NUM_THREADS);
	assert(numThreads.i == 0);
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
int PThr4wTest_Inherit1()
{
	static int minPrio;
	static int maxPrio;
	static int validPriorities[PTW32TEST_MAXPRIORITIES];

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int policy;
			struct sched_param param;
			assert(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
			return (void*)(size_t)param.sched_priority;
		}
		static void * getValidPriorities(void * arg)
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
				assert(pthread_setschedparam(thread, SCHED_OTHER, &param) == 0);
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
	assert((maxPrio = sched_get_priority_max(SCHED_OTHER)) != -1);
	assert((minPrio = sched_get_priority_min(SCHED_OTHER)) != -1);
	assert(pthread_create(&t, NULL, InnerBlock::getValidPriorities, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert(pthread_attr_init(&attr) == 0);
	assert(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) == 0);
	assert(pthread_attr_getinheritsched(&attr, &inheritsched) == 0);
	assert(inheritsched == PTHREAD_INHERIT_SCHED);
	for(prio = minPrio; prio <= maxPrio; prio++) {
		mainParam.sched_priority = prio;
		// Set the thread's priority to a known initial value. 
		SetThreadPriority(threadH, PTW32TEST_THREAD_INIT_PRIO);
		// Change the main thread priority 
		assert(pthread_setschedparam(mainThread, SCHED_OTHER, &mainParam) == 0);
		assert(pthread_getschedparam(mainThread, &policy, &mainParam) == 0);
		assert(policy == SCHED_OTHER);
		// Priority returned below should be the level set by pthread_setschedparam(). 
		assert(mainParam.sched_priority == prio);
		assert(GetThreadPriority(threadH) == validPriorities[prio+(PTW32TEST_MAXPRIORITIES/2)]);
		for(param.sched_priority = prio;
		    param.sched_priority <= maxPrio;
		    param.sched_priority++) {
			// The new thread create should ignore this new priority 
			assert(pthread_attr_setschedparam(&attr, &param) == 0);
			assert(pthread_create(&t, &attr, InnerBlock::func, NULL) == 0);
			pthread_join(t, &result);
			assert((int)(size_t)result == mainParam.sched_priority);
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
int PThr4wTest_Barrier1()
{
	static pthread_barrier_t barrier = NULL;
	assert(barrier == NULL);
	assert(pthread_barrier_init(&barrier, NULL, 1) == 0);
	assert(barrier != NULL);
	assert(pthread_barrier_destroy(&barrier) == 0);
	assert(barrier == NULL);
	return 0;
}

int PThr4wTest_Barrier2()
{
	static pthread_barrier_t barrier = NULL;
	assert(pthread_barrier_init(&barrier, NULL, 1) == 0);
	assert(pthread_barrier_wait(&barrier) == PTHREAD_BARRIER_SERIAL_THREAD);
	assert(pthread_barrier_destroy(&barrier) == 0);
	return 0;
}

int PThr4wTest_Barrier3()
{
	static pthread_barrier_t barrier = NULL;
	static void * result = (void*)1;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			return (void*)(size_t)pthread_barrier_wait(&barrier);
		}
	};
	pthread_t t;
	pthread_barrierattr_t ba;
	assert(pthread_barrierattr_init(&ba) == 0);
	assert(pthread_barrierattr_setpshared(&ba, PTHREAD_PROCESS_PRIVATE) == 0);
	assert(pthread_barrier_init(&barrier, &ba, 1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, &result) == 0);
	assert((int)(size_t)result == PTHREAD_BARRIER_SERIAL_THREAD);
	assert(pthread_barrier_destroy(&barrier) == 0);
	assert(pthread_barrierattr_destroy(&ba) == 0);
	return 0;
}

int PThr4wTest_Barrier4()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			assert(pthread_mutex_lock(&mx) == 0);
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
			assert(pthread_mutex_unlock(&mx) == 0);
			return NULL;
		}
	};
	const int NUMTHREADS = 16;
	pthread_t t[NUMTHREADS + 1];
	for(int j = 1; j <= NUMTHREADS; j++) {
		int i;
		printf("Barrier height = %d\n", j);
		serialThreadCount = 0;
		assert(pthread_barrier_init(&barrier, NULL, j) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, NULL) == 0);
		}
		for(i = 1; i <= j; i++) {
			assert(pthread_join(t[i], NULL) == 0);
		}
		assert(serialThreadCount == 1);
		assert(pthread_barrier_destroy(&barrier) == 0);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

int PThr4wTest_Barrier5()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static LONG totalThreadCrossings;
	class InnerBlock {
	public:
		static void * func(void * crossings)
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
		assert(pthread_barrier_init(&barrier, NULL, height) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, (void*)(size_t)Crossings) == 0);
		}
		serialThreadsTotal = 0;
		for(i = 1; i <= j; i++) {
			assert(pthread_join(t[i], &result) == 0);
			serialThreadsTotal += (int)(size_t)result;
		}
		assert(serialThreadsTotal == BARRIERMULTIPLE);
		assert(pthread_barrier_destroy(&barrier) == 0);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
	return 0;
}

int PThr4wTest_Barrier6()
{
	static pthread_barrier_t barrier = NULL;
	static pthread_mutex_t mx = PTHREAD_MUTEX_INITIALIZER;
	static int serialThreadCount = 0;
	static int otherThreadCount = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			int result = pthread_barrier_wait(&barrier);
			assert(pthread_mutex_lock(&mx) == 0);
			if(result == PTHREAD_BARRIER_SERIAL_THREAD) {
				serialThreadCount++;
			}
			else if(0 == result) {
				otherThreadCount++;
			}
			assert(pthread_mutex_unlock(&mx) == 0);
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
		assert(pthread_barrier_init(&barrier, NULL, howHigh) == 0);
		for(i = 1; i <= j; i++) {
			assert(pthread_create(&t[i], NULL, InnerBlock::func, NULL) == 0);
			if(i == howHigh) {
				for(int k = 1; k <= howHigh; k++) {
					assert(pthread_join(t[k], NULL) == 0);
				}
				assert(pthread_barrier_destroy(&barrier) == 0);
			}
		}
		for(i = howHigh+1; i <= j; i++) {
			assert(pthread_join(t[i], NULL) == 0);
		}
		assert(serialThreadCount == 1);
		assert(otherThreadCount == (howHigh - 1));
		assert(pthread_barrier_destroy(&barrier) == EINVAL);
	}
	assert(pthread_mutex_destroy(&mx) == 0);
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
int PThr4wTest_Semaphore1()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			sem_t s;
			int result;
			assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert((result = sem_trywait(&s)) == -1);
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
				assert(err == EAGAIN);
			}
			else {
				printf("thread: ok 1\n");
			}
			assert((result = sem_post(&s)) == 0);
			assert((result = sem_trywait(&s)) == 0);
			assert(sem_post(&s) == 0);
			return NULL;
		}
	};
	pthread_t t;
	sem_t s;
	void* result1 = (void*)-1;
	int result2;
	assert(pthread_create(&t, NULL, InnerBlock::thr, NULL) == 0);
	assert(pthread_join(t, &result1) == 0);
	assert((int)(size_t)result1 == 0);
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert((result2 = sem_trywait(&s)) == -1);
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
		assert(err == EAGAIN);
	}
	else {
		printf("main: ok 1\n");
	}
	assert((result2 = sem_post(&s)) == 0);
	assert((result2 = sem_trywait(&s)) == 0);
	assert(sem_post(&s) == 0);
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
int PThr4wTest_Semaphore2()
{
	sem_t s;
	int value = 0;
	int i;
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, SEMAPHORE_MAX_COUNT) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == SEMAPHORE_MAX_COUNT);
//	  printf("Value = %ld\n", value);
	for(i = SEMAPHORE_MAX_COUNT - 1; i >= 0; i--) {
		assert(sem_wait(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		assert(value == i);
	}
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
//			  printf("Value = %ld\n", value);
		assert(value == i);
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
int PThr4wTest_Semaphore3()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[SEMAPHORE_MAX_COUNT+1];
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	//printf("Value = %d\n", value);	fflush(stdout);
	assert(value == 0);
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			assert(sem_getvalue(&s, &value) == 0);
		}
		while(-value != i);
		//printf("1:Value = %d\n", value); fflush(stdout);
		assert(-value == i);
	}
	for(i = SEMAPHORE_MAX_COUNT - 1; i >= 0; i--) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
		//printf("2:Value = %d\n", value);	fflush(stdout);
		assert(-value == i);
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
int PThr4wTest_Semaphore4()
{
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_wait(&s) == 0);
			return NULL;
		}
	};
	int value = 0;
	int i;
	pthread_t t[SEMAPHORE_MAX_COUNT+1];
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(sem_getvalue(&s, &value) == 0);
	assert(value == 0);
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
		assert(pthread_create(&t[i], NULL, InnerBlock::thr, NULL) == 0);
		do {
			sched_yield();
			assert(sem_getvalue(&s, &value) == 0);
		} while(value != -i);
		assert(-value == i);
	}
	assert(sem_getvalue(&s, &value) == 0);
	assert(-value == SEMAPHORE_MAX_COUNT);
	assert(pthread_cancel(t[50]) == 0);
	{
		void* result;
		assert(pthread_join(t[50], &result) == 0);
	}
	assert(sem_getvalue(&s, &value) == 0);
	assert(-value == (SEMAPHORE_MAX_COUNT - 1));
	for(i = SEMAPHORE_MAX_COUNT - 2; i >= 0; i--) {
		assert(sem_post(&s) == 0);
		assert(sem_getvalue(&s, &value) == 0);
		assert(-value == i);
	}
	for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++)
		if(i != 50)
			assert(pthread_join(t[i], NULL) == 0);
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
int PThr4wTest_Semaphore4t()
{
	static const long NANOSEC_PER_SEC = 1000000000L;
	static sem_t s;

	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_timedwait(&s, NULL) == 0);
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
			assert(rc != 0);
			assert(errno == ETIMEDOUT);
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
			assert(sem_init(&s2, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert(sem_getvalue(&s2, &value) == 0);
			assert(value == 0);
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
			assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
			assert(sem_getvalue(&s, &value) == 0);
			assert(value == 0);
			for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
				assert(pthread_create(&t[i], NULL, thr, NULL) == 0);
				do {
					sched_yield();
					assert(sem_getvalue(&s, &value) == 0);
				} while(value != -i);
				assert(-value == i);
			}
			assert(sem_getvalue(&s, &value) == 0);
			assert(-value == SEMAPHORE_MAX_COUNT);
			assert(pthread_cancel(t[50]) == 0);
			assert(pthread_join(t[50], NULL) == 0);
			assert(sem_getvalue(&s, &value) == 0);
			assert(-value == SEMAPHORE_MAX_COUNT - 1);
			for(i = SEMAPHORE_MAX_COUNT - 2; i >= 0; i--) {
				assert(sem_post(&s) == 0);
				assert(sem_getvalue(&s, &value) == 0);
				assert(-value == i);
			}
			for(i = 1; i <= SEMAPHORE_MAX_COUNT; i++) {
				if(i != 50) {
					assert(pthread_join(t[i], NULL) == 0);
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
int PThr4wTest_Semaphore5()
{
	class InnerBlock {
	public:
		static void * thr(void * arg)
		{
			assert(sem_post((sem_t*)arg) == 0);
			return 0;
		}
	};
	pthread_t t;
	sem_t s;
	assert(sem_init(&s, PTHREAD_PROCESS_PRIVATE, 0) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::thr, (void*)&s) == 0);
	assert(sem_wait(&s) == 0);
	/*
	 * Normally we would retry this next, but we're only
	 * interested in unexpected results in this test.
	 */
	assert(sem_destroy(&s) == 0 || errno == EBUSY);
	assert(pthread_join(t, NULL) == 0);
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
int PThr4wTest_Eyal1()
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
				assert(pthread_mutex_lock(&mutex_stdout) == 0);
				// do our job
				printf("%c", "0123456789abcdefghijklmnopqrstuvwxyz"[who]);
				if(!(++nchars % 50))
					printf("\n");
				fflush(stdout);
				// release lock on stdout
				assert(pthread_mutex_unlock(&mutex_stdout) == 0);
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
			assert(pthread_mutex_lock(&tc->mutex_started) == 0);
			for(;;) {
				assert(pthread_mutex_lock(&tc->mutex_start) == 0);
				assert(pthread_mutex_unlock(&tc->mutex_start) == 0);
				assert(pthread_mutex_lock(&tc->mutex_ended) == 0);
				assert(pthread_mutex_unlock(&tc->mutex_started) == 0);
				for(;;) {
					/*
					 * get lock on todo list
					 */
					assert(pthread_mutex_lock(&mutex_todo) == 0);
					mywork = todo;
					if(todo >= 0) {
						++todo;
						if(todo >= nwork)
							todo = -1;
					}
					assert(pthread_mutex_unlock(&mutex_todo) == 0);
					if(mywork < 0)
						break;
					assert((n = do_work_unit(tc->id, mywork)) >= 0);
					tc->work += n;
				}
				assert(pthread_mutex_lock(&tc->mutex_end) == 0);
				assert(pthread_mutex_unlock(&tc->mutex_end) == 0);
				assert(pthread_mutex_lock(&tc->mutex_started) == 0);
				assert(pthread_mutex_unlock(&tc->mutex_ended) == 0);
				if(-2 == mywork)
					break;
			}
			assert(pthread_mutex_unlock(&tc->mutex_started) == 0);
			return (0);
		}
		static void dosync(void)
		{
			int i;
			for(i = 0; i < nthreads; ++i) {
				assert(pthread_mutex_lock(&tcs[i].mutex_end) == 0);
				assert(pthread_mutex_unlock(&tcs[i].mutex_start) == 0);
				assert(pthread_mutex_lock(&tcs[i].mutex_started) == 0);
				assert(pthread_mutex_unlock(&tcs[i].mutex_started) == 0);
			}
			/*
			 * Now threads do their work
			 */
			for(i = 0; i < nthreads; ++i) {
				assert(pthread_mutex_lock(&tcs[i].mutex_start) == 0);
				assert(pthread_mutex_unlock(&tcs[i].mutex_end) == 0);
				assert(pthread_mutex_lock(&tcs[i].mutex_ended) == 0);
				assert(pthread_mutex_unlock(&tcs[i].mutex_ended) == 0);
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
	assert(NULL != (tcs = (TC *)SAlloc::C(nthreads, sizeof(*tcs))));
	/*
	 * Launch threads
	 */
	for(i = 0; i < nthreads; ++i) {
		tcs[i].id = i;
		assert(pthread_mutex_init(&tcs[i].mutex_start, NULL) == 0);
		assert(pthread_mutex_init(&tcs[i].mutex_started, NULL) == 0);
		assert(pthread_mutex_init(&tcs[i].mutex_end, NULL) == 0);
		assert(pthread_mutex_init(&tcs[i].mutex_ended, NULL) == 0);
		tcs[i].work = 0;
		assert(pthread_mutex_lock(&tcs[i].mutex_start) == 0);
		assert((tcs[i].stat = pthread_create(&tcs[i].thread, NULL, (void *(*)(void *))InnerBlock::print_server, (void*)&tcs[i])) == 0);
		/*
		 * Wait for thread initialisation
		 */
		{
			int trylock = 0;
			while(trylock == 0) {
				trylock = pthread_mutex_trylock(&tcs[i].mutex_started);
				assert(trylock == 0 || trylock == EBUSY);
				if(trylock == 0) {
					assert(pthread_mutex_unlock(&tcs[i].mutex_started) == 0);
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
			assert(pthread_join(tcs[i].thread, NULL) == 0);
	}
	/*
	 * destroy locks
	 */
	assert(pthread_mutex_destroy(&mutex_stdout) == 0);
	assert(pthread_mutex_destroy(&mutex_todo) == 0);
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
		assert(pthread_mutex_unlock(&tcs[i].mutex_start) == 0);
		assert(pthread_mutex_destroy(&tcs[i].mutex_start) == 0);
		assert(pthread_mutex_destroy(&tcs[i].mutex_started) == 0);
		assert(pthread_mutex_destroy(&tcs[i].mutex_end) == 0);
		assert(pthread_mutex_destroy(&tcs[i].mutex_ended) == 0);
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
int PThr4wTest_Timeouts()
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
			assert(0 == pthread_mutexattr_init(&mattr_));
			assert(0 == pthread_mutex_init(&mutex_, &mattr_));
			assert(0 == pthread_condattr_init(&cattr_));
			assert(0 == pthread_cond_init(&cv_, &cattr_));
			return 0;
		}
		static int Destroy()
		{
			assert(0 == pthread_cond_destroy(&cv_));
			assert(0 == pthread_mutex_destroy(&mutex_));
			assert(0 == pthread_mutexattr_destroy(&mattr_));
			assert(0 == pthread_condattr_destroy(&cattr_));
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
			assert(0 == pthread_mutex_lock(&mutex_));
			// 
			// We don't need to check the CV.
			// 
			result = pthread_cond_timedwait(&cv_, &mutex_, &abstime);
			assert(result != 0);
			assert(errno == ETIMEDOUT);
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
int PThr4wTest_NameNp1()
{
	static int washere = 0;
	static pthread_barrier_t sync;
	#if defined (__PTW32_COMPATIBILITY_BSD)
		static int seqno = 0;
	#endif

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			char buf[32];
			pthread_t self = pthread_self();
			washere = 1;
			pthread_barrier_wait(&sync);
			assert(pthread_getname_np(self, buf, 32) == 0);
			printf("Thread name: %s\n", buf);
			pthread_barrier_wait(&sync);
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_barrier_init(&sync, NULL, 2) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
#if defined (__PTW32_COMPATIBILITY_BSD)
	seqno++;
	assert(pthread_setname_np(t, "MyThread%d", (void*)&seqno) == 0);
#elif defined (__PTW32_COMPATIBILITY_TRU64)
	assert(pthread_setname_np(t, "MyThread1", NULL) == 0);
#else
	assert(pthread_setname_np(t, "MyThread1") == 0);
#endif
	pthread_barrier_wait(&sync);
	pthread_barrier_wait(&sync);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_barrier_destroy(&sync) == 0);
	assert(washere == 1);
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
int PThr4wTest_NameNp2()
{
	static int washere = 0;
	static pthread_attr_t attr;
	static pthread_barrier_t sync;
	#if defined (__PTW32_COMPATIBILITY_BSD)
		static int seqno = 0;
	#endif

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			char buf[32];
			pthread_t self = pthread_self();
			washere = 1;
			pthread_barrier_wait(&sync);
			assert(pthread_getname_np(self, buf, 32) == 0);
			printf("Thread name: %s\n", buf);
			pthread_barrier_wait(&sync);
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_attr_init(&attr) == 0);
#if defined (__PTW32_COMPATIBILITY_BSD)
	seqno++;
	assert(pthread_attr_setname_np(&attr, "MyThread%d", (void*)&seqno) == 0);
#elif defined (__PTW32_COMPATIBILITY_TRU64)
	assert(pthread_attr_setname_np(&attr, "MyThread1", NULL) == 0);
#else
	assert(pthread_attr_setname_np(&attr, "MyThread1") == 0);
#endif
	assert(pthread_barrier_init(&sync, NULL, 2) == 0);
	assert(pthread_create(&t, &attr, InnerBlock::func, NULL) == 0);
	pthread_barrier_wait(&sync);
	pthread_barrier_wait(&sync);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_barrier_destroy(&sync) == 0);
	assert(pthread_attr_destroy(&attr) == 0);
	assert(washere == 1);
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
int PThr4wTest_RwLock1()
{
	static pthread_rwlock_t rwlock = NULL;
	assert(rwlock == NULL);
	assert(pthread_rwlock_init(&rwlock, NULL) == 0);
	assert(rwlock != NULL);
	assert(pthread_rwlock_destroy(&rwlock) == 0);
	assert(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, lock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_rdlock(), pthread_rwlock_unlock()
//
int PThr4wTest_RwLock2()
{
	static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
	assert(rwlock == PTHREAD_RWLOCK_INITIALIZER);
	assert(pthread_rwlock_rdlock(&rwlock) == 0);
	assert(rwlock != PTHREAD_RWLOCK_INITIALIZER);
	assert(rwlock != NULL);
	assert(pthread_rwlock_unlock(&rwlock) == 0);
	assert(pthread_rwlock_destroy(&rwlock) == 0);
	assert(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, timed-lock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_unlock()
//
int PThr4wTest_RwLock2t()
{
	static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
	struct timespec abstime;
	struct timespec reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert(rwlock == PTHREAD_RWLOCK_INITIALIZER);
	assert(pthread_rwlock_timedrdlock(&rwlock, &abstime) == 0);
	assert(rwlock != PTHREAD_RWLOCK_INITIALIZER);
	assert(rwlock != NULL);
	assert(pthread_rwlock_unlock(&rwlock) == 0);
	assert(pthread_rwlock_destroy(&rwlock) == 0);
	assert(rwlock == NULL);
	return 0;
}
// 
// Declare a static rwlock object, wrlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_wrlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
int PThr4wTest_RwLock3()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_rwlock_wrlock(&rwlock1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-wrlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedwrlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
int PThr4wTest_RwLock3t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;
	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime, reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_rwlock_timedwrlock(&rwlock1, &abstime) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	Sleep(2000);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, rdlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_rdlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
//
int PThr4wTest_RwLock4()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_rwlock_rdlock(&rwlock1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-rdlock it, trywrlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_trywrlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock4t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_trywrlock(&rwlock1) == EBUSY);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime = { 0, 0 };
	struct timespec reltime = { 1, 0 };
	assert(pthread_rwlock_timedrdlock(&rwlock1, pthread_win32_getabstime_np(&abstime, &reltime)) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	Sleep(2000);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, rdlock it, tryrdlock it, and then unlock it again.
// Depends on API functions: pthread_create(), pthread_join(), pthread_rwlock_rdlock(), pthread_rwlock_tryrdlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock5()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_tryrdlock(&rwlock1) == 0);
			assert(pthread_rwlock_unlock(&rwlock1) == 0);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	assert(pthread_rwlock_rdlock(&rwlock1) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	assert(pthread_join(t, NULL) == 0);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Declare a static rwlock object, timed-rdlock it, tryrdlock it, and then unlock it again.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_tryrdlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock5t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int washere = 0;

	class InnerBlock {
	public:
		static void * func(void * arg)
		{
			assert(pthread_rwlock_tryrdlock(&rwlock1) == 0);
			assert(pthread_rwlock_unlock(&rwlock1) == 0);
			washere = 1;
			return 0;
		}
	};
	pthread_t t;
	struct timespec abstime, reltime = { 1, 0 };
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
	assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == 0);
	assert(pthread_create(&t, NULL, InnerBlock::func, NULL) == 0);
	Sleep(2000);
	assert(pthread_rwlock_unlock(&rwlock1) == 0);
	assert(washere == 1);
	return 0;
}
// 
// Check writer and reader locking
// Depends on API functions: pthread_rwlock_rdlock(), pthread_rwlock_wrlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock6()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int bankAccount = 0;

	class InnerBlock {
	public:
		static void * wrfunc(void * arg)
		{
			int ba;
			assert(pthread_rwlock_wrlock(&rwlock1) == 0);
			Sleep(200);
			bankAccount += 10;
			ba = bankAccount;
			assert(pthread_rwlock_unlock(&rwlock1) == 0);
			return ((void*)(size_t)ba);
		}
		static void * rdfunc(void * arg)
		{
			int ba;
			assert(pthread_rwlock_rdlock(&rwlock1) == 0);
			ba = bankAccount;
			assert(pthread_rwlock_unlock(&rwlock1) == 0);
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
	assert(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(50);
	assert(pthread_create(&rdt, NULL, InnerBlock::rdfunc, NULL) == 0);
	Sleep(50);
	assert(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, NULL) == 0);
	assert(pthread_join(wrt1, &wr1Result) == 0);
	assert(pthread_join(rdt, &rdResult) == 0);
	assert(pthread_join(wrt2, &wr2Result) == 0);
	assert((int)(size_t)wr1Result == 10);
	assert((int)(size_t)rdResult == 10);
	assert((int)(size_t)wr2Result == 20);
	return 0;
}
// 
// Check writer and reader locking with reader timeouts
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_wrlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock6t()
{
	static pthread_rwlock_t rwlock1 = PTHREAD_RWLOCK_INITIALIZER;
	static int bankAccount = 0;

	class InnerBlock {
	public:
		static void * wrfunc(void * arg)
		{
			assert(pthread_rwlock_wrlock(&rwlock1) == 0);
			Sleep(2000);
			bankAccount += 10;
			assert(pthread_rwlock_unlock(&rwlock1) == 0);
			return ((void*)(size_t)bankAccount);
		}
		static void * rdfunc(void * arg)
		{
			int ba = -1;
			struct timespec abstime;
			(void)pthread_win32_getabstime_np(&abstime, NULL);
			if((int)(size_t)arg == 1) {
				abstime.tv_sec += 1;
				assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == ETIMEDOUT);
				ba = 0;
			}
			else if((int)(size_t)arg == 2) {
				abstime.tv_sec += 3;
				assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == 0);
				ba = bankAccount;
				assert(pthread_rwlock_unlock(&rwlock1) == 0);
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
	assert(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(500);
	assert(pthread_create(&rdt1, NULL, InnerBlock::rdfunc, (void*)(size_t)1) == 0);
	Sleep(500);
	assert(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, NULL) == 0);
	Sleep(500);
	assert(pthread_create(&rdt2, NULL, InnerBlock::rdfunc, (void*)(size_t)2) == 0);
	assert(pthread_join(wrt1, &wr1Result) == 0);
	assert(pthread_join(rdt1, &rd1Result) == 0);
	assert(pthread_join(wrt2, &wr2Result) == 0);
	assert(pthread_join(rdt2, &rd2Result) == 0);
	assert((int)(size_t)wr1Result == 10);
	assert((int)(size_t)rd1Result == 0);
	assert((int)(size_t)wr2Result == 20);
	assert((int)(size_t)rd2Result == 20);
	return 0;
}
// 
// Check writer and reader timeouts.
// Depends on API functions: pthread_rwlock_timedrdlock(), pthread_rwlock_timedwrlock(), pthread_rwlock_unlock()
// 
int PThr4wTest_RwLock6t2()
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
				assert(result == 0);
				Sleep(2000);
				bankAccount += 10;
				assert(pthread_rwlock_unlock(&rwlock1) == 0);
				return ((void*)(size_t)bankAccount);
			}
			else if((int)(size_t)arg == 2) {
				assert(result == ETIMEDOUT);
				return ((void*)100);
			}
			return ((void*)(size_t)-1);
		}
		static void * rdfunc(void * arg)
		{
			int ba = 0;
			assert(pthread_rwlock_timedrdlock(&rwlock1, &abstime) == ETIMEDOUT);
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
	assert(pthread_create(&wrt1, NULL, InnerBlock::wrfunc, (void*)(size_t)1) == 0);
	Sleep(100);
	assert(pthread_create(&rdt, NULL, InnerBlock::rdfunc, NULL) == 0);
	Sleep(100);
	assert(pthread_create(&wrt2, NULL, InnerBlock::wrfunc, (void*)(size_t)2) == 0);
	assert(pthread_join(wrt1, &wr1Result) == 0);
	assert(pthread_join(rdt, &rdResult) == 0);
	assert(pthread_join(wrt2, &wr2Result) == 0);
	assert((int)(size_t)wr1Result == 10);
	assert((int)(size_t)rdResult == 0);
	assert((int)(size_t)wr2Result == 100);
	return 0;
}
// 
// Hammer on a bunch of rwlocks to test robustness and fairness.
// Printed stats should be roughly even for each thread.
// 
int PThr4wTest_RwLock7(/*int argc, char * argv[]*/)
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
					assert(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the times, to report later.
					 */
					assert(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
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
		assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
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
		assert(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	/*
	 * Wait for all threads to complete, and collect statistics.
	 */
	for(count = 0; count < THREADS; count++) {
		assert(pthread_join(threads[count].thread_id, NULL) == 0);
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
		assert(pthread_rwlock_destroy(&data[data_count].lock) == 0);
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
int PThr4wTest_RwLock71(/*int argc, char * argv[]*/)
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
			assert(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &self->threadCpus) == 0);
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
					assert(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the times, to report later.
					 */
					assert(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
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
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	assert((cpu_count = CPU_COUNT(&processCpus)) > 0);
	printf("CPUs: %d\n", cpu_count);
	/*
	 * Initialize the shared data.
	 */
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;
		assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
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
		assert(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	/*
	 * Wait for all threads to complete, and collect statistics.
	 */
	for(count = 0; count < THREADS; count++) {
		assert(pthread_join(threads[count].thread_id, NULL) == 0);
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
		assert(pthread_rwlock_destroy(&data[data_count].lock) == 0);
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
int PThr4wTest_RwLock8(/*int argc, char * argv[]*/)
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
					assert(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					sched_yield();
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the
					 * times, to report later.
					 */
					assert(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					sched_yield();
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
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
		assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
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
		assert(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	// 
	// Wait for all threads to complete, and collect statistics.
	// 
	for(count = 0; count < THREADS; count++) {
		assert(pthread_join(threads[count].thread_id, NULL) == 0);
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
		assert(pthread_rwlock_destroy(&data[data_count].lock) == 0);
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
int PThr4wTest_RwLock81(/*int argc, char * argv[]*/)
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
			assert(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &self->threadCpus) == 0);
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
					assert(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					sched_yield();
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					// 
					// Look at the current data element to see whether
					// the current thread last updated it. Count the times, to report later.
					// 
					assert(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					sched_yield();
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
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
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	assert((cpu_count = CPU_COUNT(&processCpus)) > 0);
	printf("CPUs: %d\n", cpu_count);
	// 
	// Initialize the shared data.
	// 
	for(data_count = 0; data_count < DATASIZE; data_count++) {
		data[data_count].data = 0;
		data[data_count].updates = 0;

		assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
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
		assert(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
	}
	// 
	// Wait for all threads to complete, and collect statistics.
	// 
	for(count = 0; count < THREADS; count++) {
		assert(pthread_join(threads[count].thread_id, NULL) == 0);
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
		assert(pthread_rwlock_destroy(&data[data_count].lock) == 0);
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
int PThr4wTest_Reinit1()
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
			assert(pthread_getunique_np(self->thread_id) == (unsigned __int64)(self->thread_num + 2));
			for(iteration = 0; iteration < ITERATIONS; iteration++) {
				/*
				 * Each "self->interval" iterations, perform an
				 * update operation (write lock instead of read lock).
				 */
				if((iteration % interval) == 0) {
					assert(pthread_rwlock_wrlock(&data[element].lock) == 0);
					data[element].data = self->thread_num;
					data[element].updates++;
					self->updates++;
					interval = 1 + rand_r(&seed) % 71;
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
				}
				else {
					/*
					 * Look at the current data element to see whether
					 * the current thread last updated it. Count the
					 * times, to report later.
					 */
					assert(pthread_rwlock_rdlock(&data[element].lock) == 0);
					self->reads++;
					if(data[element].data != self->thread_num) {
						self->changed++;
						interval = 1 + self->changed % 71;
					}
					assert(pthread_rwlock_unlock(&data[element].lock) == 0);
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
			assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
		}
		// 
		// Create THREADS threads to access shared data.
		// 
		for(count = 0; count < THREADS; count++) {
			threads[count].thread_num = count;
			threads[count].updates = 0;
			threads[count].reads = 0;
			threads[count].seed = 1 + rand_r(&seed) % 71;
			assert(pthread_create(&threads[count].thread_id, NULL, InnerBlock::thread_routine, (void*)(size_t)&threads[count]) == 0);
		}
		// 
		// Wait for all threads to complete, and collect statistics.
		// 
		for(count = 0; count < THREADS; count++) {
			assert(pthread_join(threads[count].thread_id, NULL) == 0);
		}
		pthread_win32_process_detach_np();
		pthread_win32_process_attach_np();
	}
	return 0;
}

int PThr4wTest_Affinity1();
int PThr4wTest_Affinity2();
int PThr4wTest_Affinity3();
int PThr4wTest_Affinity4();
int PThr4wTest_Affinity5();
int PThr4wTest_Affinity6();
int PThr4wTest_Mutex1();
int PThr4wTest_Mutex1e();
int PThr4wTest_Mutex1n();
int PThr4wTest_Mutex1r();
int PThr4wTest_Mutex2();
int PThr4wTest_Mutex2e();
int PThr4wTest_Mutex2r();
int PThr4wTest_Mutex3();
int PThr4wTest_Mutex3e();
int PThr4wTest_Mutex3r();
int PThr4wTest_Mutex4();
int PThr4wTest_Mutex5();
int PThr4wTest_Mutex6();
int PThr4wTest_Mutex6e();
int PThr4wTest_Mutex6es();
int PThr4wTest_Mutex6n();
int PThr4wTest_Mutex6r();
int PThr4wTest_Mutex6rs();
int PThr4wTest_Mutex6s();
int PThr4wTest_Mutex7();
int PThr4wTest_Mutex7e();
int PThr4wTest_Mutex7n();
int PThr4wTest_Mutex7r();
int PThr4wTest_Mutex8();
int PThr4wTest_Mutex8e();
int PThr4wTest_Mutex8n();
int PThr4wTest_Mutex8r();
int PThr4wTest_CleanUp0();
int PThr4wTest_CleanUp1();
int PThr4wTest_CleanUp2();
int PThr4wTest_CleanUp3();
int PThr4wTest_Cancel1();
int PThr4wTest_Cancel2();
int PThr4wTest_Cancel3();
int PThr4wTest_Cancel4();
int PThr4wTest_Cancel5();
int PThr4wTest_Cancel6a();
int PThr4wTest_Cancel6d();
int PThr4wTest_Cancel7();
int PThr4wTest_Cancel8();
int PThr4wTest_Cancel9();
int PThr4wTest_Tsd1();
int PThr4wTest_Tsd2();
int PThr4wTest_Tsd3();
int PThr4wTest_CondVar1();
int PThr4wTest_CondVar11();
int PThr4wTest_CondVar12();
int PThr4wTest_CondVar2();
int PThr4wTest_CondVar21();
int PThr4wTest_CondVar3();
int PThr4wTest_CondVar31();
int PThr4wTest_CondVar32();
int PThr4wTest_CondVar33();
int PThr4wTest_CondVar4();
int PThr4wTest_CondVar5();
int PThr4wTest_CondVar6();
int PThr4wTest_CondVar7();
int PThr4wTest_CondVar8();
int PThr4wTest_CondVar9();
int PThr4wTest_Spin1();
int PThr4wTest_Spin2();
int PThr4wTest_Spin3();
int PThr4wTest_Spin4();
// (definition above) int PThr4wTest_RwLock1();
// (definition above) int PThr4wTest_RwLock2();
// (definition above) int PThr4wTest_RwLock2t();
// (definition above) int PThr4wTest_RwLock3();
// (definition above) int PThr4wTest_RwLock3t();
// (definition above) int PThr4wTest_RwLock4();
// (definition above) int PThr4wTest_RwLock4t();
// (definition above) int PThr4wTest_RwLock5();
// (definition above) int PThr4wTest_RwLock5t();
// (definition above) int PThr4wTest_RwLock6();
// (definition above) int PThr4wTest_RwLock6t();
// (definition above) int PThr4wTest_RwLock6t2();
// (definition above) int PThr4wTest_RwLock7();
// (definition above) int PThr4wTest_RwLock71();
// (definition above) int PThr4wTest_RwLock8();
// (definition above) int PThr4wTest_RwLock81();
// (definition above) int PThr4wTest_Semaphore1();
// (definition above) int PThr4wTest_Semaphore2();
// (definition above) int PThr4wTest_Semaphore3();
// (definition above) int PThr4wTest_Semaphore4();
// (definition above) int PThr4wTest_Semaphore4t();
// (definition above) int PThr4wTest_Semaphore5();
// (definition above) int PThr4wTest_Barrier1();
// (definition above) int PThr4wTest_Barrier2();
// (definition above) int PThr4wTest_Barrier3();
// (definition above) int PThr4wTest_Barrier4();
// (definition above) int PThr4wTest_Barrier5();
// (definition above) int PThr4wTest_Barrier6();
// (definition above) int PThr4wTest_Join0();
// (definition above) int PThr4wTest_Join1();
// (definition above) int PThr4wTest_Join2();
// (definition above) int PThr4wTest_Join3();
// (definition above) int PThr4wTest_Join4();
// (definition above) int PThr4wTest_Create1();
// (definition above) int PThr4wTest_Create2();
// (definition above) int PThr4wTest_Create3();
// (definition above) int PThr4wTest_Valid1();
// (definition above) int PThr4wTest_Valid2();
// (definition above) int PThr4wTest_Detach1();
// (definition above) int PThr4wTest_Delay1();
// (definition above) int PThr4wTest_Delay2();
// (definition above) int PThr4wTest_Equal0();
// (definition above) int PThr4wTest_Equal1();
// (definition above) int PThr4wTest_Kill1();
// (definition above) int PThr4wTest_Count1();
// (definition above) int PThr4wTest_Self1();
// (definition above) int PThr4wTest_Self2();
// (definition above) int PThr4wTest_Sequence1();
// (definition above) int PThr4wTest_Errno0();
// (definition above) int PThr4wTest_Errno1();
// (definition above) int PThr4wTest_Context1();
// (definition above) int PThr4wTest_Context2();
// (definition above) int PThr4wTest_Reuse1();
// (definition above) int PThr4wTest_Reuse2();
// (definition above) int PThr4wTest_Priority1();
// (definition above) int PThr4wTest_Priority2();
// (definition above) int PThr4wTest_Once1();
// (definition above) int PThr4wTest_Once2();
// (definition above) int PThr4wTest_Once3();
// (definition above) int PThr4wTest_Once4();
// (definition above) int PThr4wTest_Inherit1();
// (definition above) int PThr4wTest_Eyal1();
// (definition above) int PThr4wTest_Timeouts();
// (definition above) int PThr4wTest_NameNp1();
// (definition above) int PThr4wTest_NameNp2();
int PThr4wTest_Exception1();
int PThr4wTest_Exception2(int argc, char* argv[]);
int PThr4wTest_Exception3();
int PThr4wTest_Exception30();
int PThr4wTest_Exit1();
int PThr4wTest_Exit2();
int PThr4wTest_Exit3();
int PThr4wTest_Exit4();
int PThr4wTest_Exit5();
int PThr4wTest_Exit6();
int PThr4wTest_Robust1();
int PThr4wTest_Robust2();
int PThr4wTest_Robust3();
int PThr4wTest_Robust4();
int PThr4wTest_Robust5();
int PThr4wTest_TryEnterCs1();
int PThr4wTest_TryEnterCs2();
int PThr4wTest_ThreeStage(int argc, char * argv[]);
int PThr4wTest_Stress1();
int PThr4wTest_Reinit1();
int PThr4wTest_Benchtest1();
int PThr4wTest_Benchtest2();
int PThr4wTest_Benchtest3();
int PThr4wTest_Benchtest4();
int PThr4wTest_Benchtest5();

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
	return 0;
}
