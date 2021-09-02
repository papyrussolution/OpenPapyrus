/*
 * affinity1.c
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
 *
 * --------------------------------------------------------------------------
 *
 * Basic test of CPU_*() support routines.
 * Have the process switch CPUs.
 * Have the thread switch CPUs.
 * Test thread CPU affinity setting.
 * Test thread CPU affinity inheritance.
 * Test thread CPU affinity from thread attributes.
 *
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
#include "test.h"

int PThr4wTest_Affinity1()
{
	uint cpu;
	cpu_set_t newmask;
	cpu_set_t src1mask;
	cpu_set_t src2mask;
	cpu_set_t src3mask;
	CPU_ZERO(&newmask);
	CPU_ZERO(&src1mask);
	memzero(&src2mask, sizeof(cpu_set_t));
	assert(memcmp(&src1mask, &src2mask, sizeof(cpu_set_t)) == 0);
	assert(CPU_EQUAL(&src1mask, &src2mask));
	assert(CPU_COUNT(&src1mask) == 0);
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
	assert(CPU_COUNT(&src1mask) == (sizeof(cpu_set_t)*4));
	assert(CPU_COUNT(&src2mask) == ((sizeof(cpu_set_t)*4 + (sizeof(cpu_set_t)*2))));
	assert(CPU_COUNT(&src3mask) == (sizeof(cpu_set_t)*4));
	CPU_SET(0, &newmask);
	CPU_SET(1, &newmask);
	CPU_SET(3, &newmask);
	assert(CPU_ISSET(1, &newmask));
	CPU_CLR(1, &newmask);
	assert(!CPU_ISSET(1, &newmask));
	CPU_OR(&newmask, &src1mask, &src2mask);
	assert(CPU_EQUAL(&newmask, &src2mask));
	CPU_AND(&newmask, &src1mask, &src2mask);
	assert(CPU_EQUAL(&newmask, &src1mask));
	CPU_XOR(&newmask, &src1mask, &src3mask);
	memzero(&src2mask, sizeof(cpu_set_t));
	assert(memcmp(&newmask, &src2mask, sizeof(cpu_set_t)) == 0);
	/*
	* Need to confirm the bitwise logical right-shift in CpuCount().
	* i.e. zeros inserted into MSB on shift because cpu_set_t is unsigned.
	*/
	CPU_ZERO(&src1mask);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &src1mask);                                /* 0b10101010101010101010101010101010 */
	}
	assert(CPU_ISSET(sizeof(cpu_set_t)*8-1, &src1mask));
	assert(CPU_COUNT(&src1mask) == (sizeof(cpu_set_t)*4));
	return 0;
}

int PThr4wTest_Affinity2()
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
	assert(sched_getaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
	assert(!CPU_EQUAL(&newmask, &mask));
	result = sched_setaffinity(0, sizeof(cpu_set_t), &newmask);
	if(result != 0) {
		int err =
#if defined (__PTW32_USES_SEPARATE_CRT)
		    GetLastError();
#else
		    errno;
#endif
		assert(err != ESRCH);
		assert(err != EFAULT);
		assert(err != EPERM);
		assert(err != EINVAL);
		assert(err != EAGAIN);
		assert(err == ENOSYS);
		assert(CPU_COUNT(&mask) == 1);
	}
	else {
		if(CPU_COUNT(&mask) > 1) {
			CPU_AND(&newmask, &mask, &switchmask); /* Remove every other CPU */
			assert(sched_setaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
			assert(sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0);
			CPU_XOR(&newmask, &mask, &flipmask); /* Switch to all alternative CPUs */
			assert(sched_setaffinity(0, sizeof(cpu_set_t), &newmask) == 0);
			assert(sched_getaffinity(0, sizeof(cpu_set_t), &mask) == 0);
			assert(!CPU_EQUAL(&newmask, &mask));
		}
	}
	return 0;
}

int PThr4wTest_Affinity3()
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
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &processCpus) == 0);
	printf("This thread has a starting affinity with %d CPUs\n", CPU_COUNT(&processCpus));
	assert(!CPU_EQUAL(&mask, &processCpus));
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &switchmask);                      /* 0b01010101010101010101010101010101 */
	}
	for(cpu = 0; cpu < sizeof(cpu_set_t)*8; cpu++) {
		CPU_SET(cpu, &flipmask);                                /* 0b11111111111111111111111111111111 */
	}
	result = pthread_setaffinity_np(self, sizeof(cpu_set_t), &processCpus);
	if(result != 0) {
		assert(result != ESRCH);
		assert(result != EFAULT);
		assert(result != EPERM);
		assert(result != EINVAL);
		assert(result != EAGAIN);
		assert(result == ENOSYS);
		assert(CPU_COUNT(&mask) == 1);
	}
	else {
		if(CPU_COUNT(&mask) > 1) {
			CPU_AND(&newmask, &processCpus, &switchmask); /* Remove every other CPU */
			assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
			assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
			assert(CPU_EQUAL(&mask, &newmask));
			CPU_XOR(&newmask, &mask, &flipmask); /* Switch to all alternative CPUs */
			assert(!CPU_EQUAL(&mask, &newmask));
			assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &newmask) == 0);
			assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &mask) == 0);
			assert(CPU_EQUAL(&mask, &newmask));
		}
	}
	return 0;
}

int PThr4wTest_Affinity4()
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
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), (*(PDWORD_PTR)&threadCpus) /* Violating Opacity*/);
		assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), vThreadMask);
		assert(vThreadMask != 0);
		assert(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
	}
	return 0;
}

typedef union {
	// Violates opacity 
	cpu_set_t cpuset;
	ulong bits;  /* To stop GCC complaining about %lx args to printf */
} cpuset_to_ulint;

int PThr4wTest_Affinity5()
{
	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			HANDLE threadH = GetCurrentThread();
			cpu_set_t * parentCpus = (cpu_set_t*)arg;
			cpu_set_t threadCpus;
			DWORD_PTR vThreadMask;
			cpuset_to_ulint a, b;
			assert(pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &threadCpus) == 0);
			assert(CPU_EQUAL(parentCpus, &threadCpus));
			vThreadMask = SetThreadAffinityMask(threadH, (*(PDWORD_PTR)&threadCpus) /* Violating Opacity */);
			assert(vThreadMask != 0);
			assert(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
			a.cpuset = *parentCpus;
			b.cpuset = threadCpus;
			/* Violates opacity */
			printf("CPU affinity: Parent/Thread = 0x%lx/0x%lx\n", a.bits, b.bits);
			return (void*)0;
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
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		assert(pthread_create(&tid, NULL, InnerBlock::mythread, (void*)&threadCpus) == 0);
		assert(pthread_join(tid, NULL) == 0);
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		assert(pthread_setaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
		vThreadMask = SetThreadAffinityMask(GetCurrentThread(), (*(PDWORD_PTR)&threadCpus) /* Violating Opacity*/);
		assert(vThreadMask != 0);
		assert(memcmp(&vThreadMask, &threadCpus, sizeof(DWORD_PTR)) == 0);
		assert(pthread_create(&tid, NULL, InnerBlock::mythread, (void*)&threadCpus) == 0);
		assert(pthread_join(tid, NULL) == 0);
	}
	return 0;
}

int PThr4wTest_Affinity6()
{
	class InnerBlock {
	public:
		static void * mythread(void * arg)
		{
			pthread_attr_t * attrPtr = (pthread_attr_t*)arg;
			cpu_set_t threadCpus, attrCpus;
			assert(pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &threadCpus) == 0);
			assert(pthread_attr_getaffinity_np(attrPtr, sizeof(cpu_set_t), &attrCpus) == 0);
			assert(CPU_EQUAL(&attrCpus, &threadCpus));
			return (void*)0;
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
	assert(pthread_attr_init(&attr1) == 0);
	assert(pthread_attr_init(&attr2) == 0);
	CPU_ZERO(&keepCpus);
	for(cpu = 1; cpu < sizeof(cpu_set_t)*8; cpu += 2) {
		CPU_SET(cpu, &keepCpus);                                /* 0b10101010101010101010101010101010 */
	}
	assert(pthread_getaffinity_np(self, sizeof(cpu_set_t), &threadCpus) == 0);
	if(CPU_COUNT(&threadCpus) > 1) {
		assert(pthread_attr_setaffinity_np(&attr1, sizeof(cpu_set_t), &threadCpus) == 0);
		CPU_AND(&threadCpus, &threadCpus, &keepCpus);
		assert(pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t), &threadCpus) == 0);
		assert(pthread_create(&tid, &attr1, InnerBlock::mythread, (void*)&attr1) == 0);
		assert(pthread_join(tid, NULL) == 0);
		assert(pthread_create(&tid, &attr2, InnerBlock::mythread, (void*)&attr2) == 0);
		assert(pthread_join(tid, NULL) == 0);
	}
	assert(pthread_attr_destroy(&attr1) == 0);
	assert(pthread_attr_destroy(&attr2) == 0);
	return 0;
}
