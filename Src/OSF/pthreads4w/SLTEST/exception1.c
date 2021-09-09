/*
 * File: exception1.c
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

#if defined(_MSC_VER) || defined(__cplusplus)
#if defined(_MSC_VER) && defined(__cplusplus)
	#include <eh.h>
#elif defined(__cplusplus)
	#include <exception>
#endif
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
int PThr4wTest_Exception1()
{
	class InnerBlock {
	public:
		static void * exceptionedThread(void * arg)
		{
			int dummy = 0;
			void* result = (void*)((int)(size_t)PTHREAD_CANCELED + 1);
			/* Set to async cancelable */
			assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
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
			assert(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) == 0);
			assert(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) == 0);
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
	assert((mt = pthread_self()).p != NULL);
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, (void*)0) == 0);
		assert(pthread_create(&ct[i], NULL, InnerBlock::canceledThread, NULL) == 0);
	}
	/*
	 * Code to control or manipulate child threads should probably go here.
	 */
	Sleep(100);
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_cancel(ct[i]) == 0);
	}
	Sleep(NUMTHREADS * 100); // Give threads time to run.
	// Check any results here. Set "failed" and only print output on failure.
	failed = 0;
	for(i = 0; i < NUMTHREADS; i++) {
		int fail = 0;
		void* result = (void*)0;
		/* Canceled thread */
		assert(pthread_join(ct[i], &result) == 0);
		assert(!(fail = (result != PTHREAD_CANCELED)));
		failed = (failed || fail);
		/* Exceptioned thread */
		assert(pthread_join(et[i], &result) == 0);
		assert(!(fail = (result != (void*)((int)(size_t)PTHREAD_CANCELED + 2))));
		failed = (failed || fail);
	}
	assert(!failed);
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
//int main(int argc, char* argv[])
int PThr4wTest_Exception2(int argc, char* argv[])
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
	assert((mt = pthread_self()).p != NULL);
	for(i = 0; i < NUMTHREADS; i++) {
		assert(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, NULL) == 0);
	}
	Sleep(100);
	return 0; // Success
}

#else /* defined(_MSC_VER) || defined(__cplusplus) */
	int PThr4wTest_Exception1()
	{
		fprintf(stderr, "Test N/A for this compiler environment.\n");
		return 0;
	}
	int PThr4wTest_Exception2(int argc, char* argv[])
	{
		fprintf(stderr, "Test N/A for this compiler environment.\n");
		return 0;
	}
#endif /* defined(_MSC_VER) || defined(__cplusplus) */
// 
// Note: Due to a buggy C++ runtime in Visual Studio 2005, when we are
// built with /MD and an unhandled exception occurs, the runtime does not
// properly call the terminate handler specified by set_terminate().
// 
#if defined(__cplusplus) && !(defined(_MSC_VER) && _MSC_VER == 1400 && defined(_DLL) && !defined(_DEBUG))
#if defined(_MSC_VER)
	#include <eh.h>
#else
	#if defined(__GNUC__) && __GNUC__ < 3
		#include <new.h>
	#else
		#include <new>
		using std::set_terminate;
	#endif
#endif
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
int PThr4wTest_Exception3()
{
	static const int NUMTHREADS = 10; // Create NUMTHREADS threads in addition to the Main thread.
	static int caught = 0;
	static pthread_mutex_t caughtLock;

	class InnerBlock {
	public:
		static void terminateFunction()
		{
			assert(pthread_mutex_lock(&caughtLock) == 0);
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
			return (void*)0;
		}
	};
	pthread_t mt;
	pthread_t et[NUMTHREADS];
	pthread_mutexattr_t ma;
	DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	assert((mt = pthread_self()).p != NULL);
	printf("See the notes inside of exception3.c re term_funcs.\n");
	assert(pthread_mutexattr_init(&ma) == 0);
	assert(pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_ERRORCHECK) == 0);
	assert(pthread_mutex_init(&caughtLock, &ma) == 0);
	assert(pthread_mutexattr_destroy(&ma) == 0);
	for(int i = 0; i < NUMTHREADS; i++) {
		assert(pthread_create(&et[i], NULL, InnerBlock::exceptionedThread, NULL) == 0);
	}
	while(true);
	/*
	 * Should never be reached.
	 */
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
int PThr4wTest_Exception30()
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
			assert(set_terminate(&terminateFunction) == &terminateFunction);
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
	int PThr4wTest_Exception3()
	{
		fprintf(stderr, "Test N/A for this compiler environment.\n");
		return 0;
	}
	int PThr4wTest_Exception30()
	{
		fprintf(stderr, "Test N/A for this compiler environment.\n");
		return 0;
	}
#endif /* defined(__cplusplus) */
