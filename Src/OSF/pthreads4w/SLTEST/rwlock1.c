/*
 * rwlock1.c
 *      Pthreads4w - POSIX Threads for Windows
 *      Copyright 1998 John E. Bossom
 *      Copyright 1999-2018, Pthreads4w contributors
 *      Homepage: https://sourceforge.net/projects/pthreads4w/
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
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
// Keep statistics for each thread.
// 
struct thread_t {
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
struct data_t {
	pthread_rwlock_t lock;
	int data;
	int updates;
};
//
//
//
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
	(void)pthread_win32_getabstime_np(&abstime, &reltime);
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

	static thread_t threads[THREADS];
	static data_t data[DATASIZE];

	class InnerBlock {
	public:
		//
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			thread_t * self = static_cast<thread_t *>(arg);
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

	static thread_t threads[THREADS];
	static data_t data[DATASIZE];
	static cpu_set_t processCpus;
	static int cpu_count;

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			thread_t * self = static_cast<thread_t *>(arg);
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

	static thread_t threads[THREADS];
	static data_t data[DATASIZE];

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		// 
		static void * thread_routine(void * arg)
		{
			thread_t * self = static_cast<thread_t *>(arg);
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

	static thread_t threads[THREADS];
	static data_t data[DATASIZE];
	static cpu_set_t processCpus;
	static int cpu_count;

	class InnerBlock {
	public:
		// 
		// Thread start routine that uses read-write locks
		// 
		static void * thread_routine(void * arg)
		{
			thread_t * self = static_cast<thread_t *>(arg);
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
				 * update operation (write lock instead of read
				 * lock).
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

	static thread_t threads[THREADS];
	static data_t data[DATASIZE];

	class InnerBlock {
	public:
		//
		// Thread start routine that uses read-write locks
		//
		static void * thread_routine(void * arg)
		{
			thread_t * self = static_cast<thread_t *>(arg);
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
		/*
		 * Initialize the shared data.
		 */
		for(data_count = 0; data_count < DATASIZE; data_count++) {
			data[data_count].data = 0;
			data[data_count].updates = 0;
			assert(pthread_rwlock_init(&data[data_count].lock, NULL) == 0);
		}
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
		 * Wait for all threads to complete, and collect
		 * statistics.
		 */
		for(count = 0; count < THREADS; count++) {
			assert(pthread_join(threads[count].thread_id, NULL) == 0);
		}
		pthread_win32_process_detach_np();
		pthread_win32_process_attach_np();
	}
	return 0;
}
