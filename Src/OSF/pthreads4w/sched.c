// SHED.C
//
/*
 * Description:
 * POSIX thread functions that deal with thread scheduling.
 * --------------------------------------------------------------------------
 *   Pthreads4w - POSIX Threads for Windows
 *   Copyright 1998 John E. Bossom
 *   Copyright 1999-2018, Pthreads4w contributors
 *
 *   Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *   The current list of contributors is contained
 *   in the file CONTRIBUTORS included with the source
 *   code distribution. The list can also be seen at the
 *   following World Wide Web location: https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop

int sched_setscheduler(pid_t pid, int policy)
{
	// 
	// Win32 only has one policy which we call SCHED_OTHER.
	// However, we try to provide other valid side-effects
	// such as EPERM and ESRCH errors. Choosing to check
	// for a valid policy last allows us to get the most value out of this function.
	// 
	if(pid) {
		int selfPid = (int)GetCurrentProcessId();
		if(pid != selfPid) {
			HANDLE h = OpenProcess(PROCESS_SET_INFORMATION,  __PTW32_FALSE, (DWORD)pid);
			if(!h) {
				__PTW32_SET_ERRNO((GetLastError() == (0xFF & ERROR_ACCESS_DENIED)) ? EPERM : ESRCH);
				return -1;
			}
			else
				CloseHandle(h);
		}
	}
	if(SCHED_OTHER != policy) {
		__PTW32_SET_ERRNO(ENOSYS);
		return -1;
	}
	// 
	// Don't set anything because there is nothing to set.
	// Just return the current (the only possible) value.
	// 
	return SCHED_OTHER;
}

int sched_getscheduler(pid_t pid)
{
	// 
	// Win32 only has one policy which we call SCHED_OTHER.
	// However, we try to provide other valid side-effects such as EPERM and ESRCH errors.
	// 
	if(pid) {
		int selfPid = (int)GetCurrentProcessId();
		if(pid != selfPid) {
			HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION,  __PTW32_FALSE, (DWORD)pid);
			if(!h) {
				__PTW32_SET_ERRNO(((0xFF & ERROR_ACCESS_DENIED) == GetLastError()) ? EPERM : ESRCH);
				return -1;
			}
			else
				CloseHandle(h);
		}
	}
	return SCHED_OTHER;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Sets the CPU affinity mask of the process whose ID is pid
 *	     to the value specified by mask.  If pid is zero, then the
 *	     calling process is used.  The argument cpusetsize is the
 *	     length (in bytes) of the data pointed to by mask.  Normally
 *	     this argument would be specified as sizeof(cpu_set_t).
 *
 *	     If the process specified by pid is not currently running on
 *	     one of the CPUs specified in mask, then that process is
 *	     migrated to one of the CPUs specified in mask.
 *
 * PARAMETERS
 *   pid
 *                           Process ID
 *
 *   cpusetsize
 *                           Currently ignored in pthreads4w.
 *                           Usually set to sizeof(cpu_set_t)
 *
 *   mask
 *                           Pointer to the CPU mask to set (cpu_set_t).
 *
 * DESCRIPTION
 *   Sets the CPU affinity mask of the process whose ID is pid
 *	     to the value specified by mask.  If pid is zero, then the
 *	     calling process is used.  The argument cpusetsize is the
 *	     length (in bytes) of the data pointed to by mask.  Normally
 *	     this argument would be specified as sizeof(cpu_set_t).
 *
 *	     If the process specified by pid is not currently running on
 *	     one of the CPUs specified in mask, then that process is
 *	     migrated to one of the CPUs specified in mask.
 *
 * RESULTS
 *           0               successfully created semaphore,
 *           EFAULT          'mask' is a NULL pointer.
 *           EINVAL          '*mask' contains no CPUs in the set of available CPUs.
 *           EAGAIN          The system available CPUs could not be obtained.
 *           EPERM           The process referred to by 'pid' is not modifiable by us.
 *           ESRCH           The process referred to by 'pid' was not found.
 *           ENOSYS			 Function not supported.
 *
 * ------------------------------------------------------
 */
int sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t * set)
{
#if !defined(NEED_PROCESS_AFFINITY_MASK)

	DWORD_PTR vProcessMask;
	DWORD_PTR vSystemMask;
	HANDLE h;
	int targetPid = (int)(size_t)pid;
	int result = 0;
	if(!set) {
		result = EFAULT;
	}
	else {
		if(0 == targetPid) {
			targetPid = (int)GetCurrentProcessId();
		}
		h = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION,  __PTW32_FALSE, (DWORD)targetPid);
		if(!h) {
			result = (((0xFF & ERROR_ACCESS_DENIED) == GetLastError()) ? EPERM : ESRCH);
		}
		else {
			if(GetProcessAffinityMask(h, &vProcessMask, &vSystemMask)) {
				/*
				 * Result is the intersection of available CPUs and the mask.
				 */
				DWORD_PTR newMask = vSystemMask & ((_sched_cpu_set_vector_*)set)->_cpuset;
				if(newMask) {
					if(SetProcessAffinityMask(h, newMask) == 0) {
						switch(GetLastError()) {
							case (0xFF & ERROR_ACCESS_DENIED): result = EPERM; break;
							case (0xFF & ERROR_INVALID_PARAMETER): result = EINVAL; break;
							default: result = EAGAIN; break;
						}
					}
				}
				else
					result = EINVAL; // Mask does not contain any CPUs currently available on the system.
			}
			else
				result = EAGAIN;
		}
		CloseHandle(h);
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	else {
		return 0;
	}
#else
	__PTW32_SET_ERRNO(ENOSYS);
	return -1;
#endif
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Gets the CPU affinity mask of the process whose ID is pid
 *	     to the value specified by mask.  If pid is zero, then the
 *	     calling process is used.  The argument cpusetsize is the
 *	     length (in bytes) of the data pointed to by mask.  Normally
 *	     this argument would be specified as sizeof(cpu_set_t).
 *
 * PARAMETERS
 *   pid
 *                           Process ID
 *
 *   cpusetsize
 *                           Currently ignored in pthreads4w.
 *                           Usually set to sizeof(cpu_set_t)
 *
 *   mask
 *                           Pointer to the CPU mask to set (cpu_set_t).
 *
 * DESCRIPTION
 *   Sets the CPU affinity mask of the process whose ID is pid
 *	     to the value specified by mask.  If pid is zero, then the
 *	     calling process is used.  The argument cpusetsize is the
 *	     length (in bytes) of the data pointed to by mask.  Normally
 *	     this argument would be specified as sizeof(cpu_set_t).
 *
 * RESULTS
 *           0               successfully created semaphore,
 *           EFAULT          'mask' is a NULL pointer.
 *           EAGAIN          The system available CPUs could not be obtained.
 *           EPERM           The process referred to by 'pid' is not modifiable by us.
 *           ESRCH           The process referred to by 'pid' was not found.
 *
 * ------------------------------------------------------
 */
int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t * set)
{
	DWORD_PTR vProcessMask;
	DWORD_PTR vSystemMask;
	HANDLE h;
	int targetPid = (int)(size_t)pid;
	int result = 0;
	if(!set) {
		result = EFAULT;
	}
	else {
#if !defined(NEED_PROCESS_AFFINITY_MASK)
		if(0 == targetPid) {
			targetPid = (int)GetCurrentProcessId();
		}
		h = OpenProcess(PROCESS_QUERY_INFORMATION,  __PTW32_FALSE, (DWORD)targetPid);
		if(!h) {
			result = (((0xFF & ERROR_ACCESS_DENIED) == GetLastError()) ? EPERM : ESRCH);
		}
		else {
			if(GetProcessAffinityMask(h, &vProcessMask, &vSystemMask))
				reinterpret_cast<_sched_cpu_set_vector_ *>(set)->_cpuset = vProcessMask;
			else
				result = EAGAIN;
		}
		CloseHandle(h);
#else
		((_sched_cpu_set_vector_*)set)->_cpuset = (size_t)0x1;
#endif
	}
	if(result != 0) {
		__PTW32_SET_ERRNO(result);
		return -1;
	}
	else {
		return 0;
	}
}
// 
// Support routines for cpu_set_t
// 
int _sched_affinitycpucount(const cpu_set_t * set)
{
	size_t tset;
	int count;
	// 
	// Relies on tset being unsigned, otherwise the right-shift will
	// be arithmetic rather than logical and the 'for' will loop forever.
	// 
	for(count = 0, tset = reinterpret_cast<const _sched_cpu_set_vector_ *>(set)->_cpuset; tset; tset >>= 1) {
		if(tset & (size_t)1)
			count++;
	}
	return count;
}

void _sched_affinitycpuzero(cpu_set_t * pset)
{
	reinterpret_cast<_sched_cpu_set_vector_ *>(pset)->_cpuset = (size_t)0;
}

void _sched_affinitycpuset(int cpu, cpu_set_t * pset)
{
	reinterpret_cast<_sched_cpu_set_vector_ *>(pset)->_cpuset |= ((size_t)1 << cpu);
}

void _sched_affinitycpuclr(int cpu, cpu_set_t * pset)
{
	reinterpret_cast<_sched_cpu_set_vector_ *>(pset)->_cpuset &= ~((size_t)1 << cpu);
}

int _sched_affinitycpuisset(int cpu, const cpu_set_t * pset)
{
	return ((((_sched_cpu_set_vector_*)pset)->_cpuset & ((size_t)1 << cpu)) != (size_t)0);
}

void _sched_affinitycpuand(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2)
{
	((_sched_cpu_set_vector_*)pdestset)->_cpuset = (((_sched_cpu_set_vector_*)psrcset1)->_cpuset & ((_sched_cpu_set_vector_*)psrcset2)->_cpuset);
}

void _sched_affinitycpuor(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2)
{
	((_sched_cpu_set_vector_*)pdestset)->_cpuset = (((_sched_cpu_set_vector_*)psrcset1)->_cpuset | ((_sched_cpu_set_vector_*)psrcset2)->_cpuset);
}

void _sched_affinitycpuxor(cpu_set_t * pdestset, const cpu_set_t * psrcset1, const cpu_set_t * psrcset2)
{
	((_sched_cpu_set_vector_*)pdestset)->_cpuset = (((_sched_cpu_set_vector_*)psrcset1)->_cpuset ^ ((_sched_cpu_set_vector_*)psrcset2)->_cpuset);
}

int _sched_affinitycpuequal(const cpu_set_t * pset1, const cpu_set_t * pset2)
{
	return (reinterpret_cast<const _sched_cpu_set_vector_*>(pset1)->_cpuset == reinterpret_cast<const _sched_cpu_set_vector_*>(pset2)->_cpuset);
}
/*
 * On Windows98, THREAD_PRIORITY_LOWEST is (-2) and THREAD_PRIORITY_HIGHEST is 2, and everything works just fine.
 *
 * On WinCE 3.0, it so happen that THREAD_PRIORITY_LOWEST is 5
 * and THREAD_PRIORITY_HIGHEST is 1 (yes, I know, it is funny:
 * highest priority use smaller numbers) and the following happens:
 *
 * sched_get_priority_min() returns 5
 * sched_get_priority_max() returns 1
 *
 * The following table shows the base priority levels for combinations
 * of priority class and priority value in Win32.
 *
 *   Process Priority Class               Thread Priority Level
 *   -----------------------------------------------------------------
 *   1 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_IDLE
 *   1 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_IDLE
 *   1 NORMAL_PRIORITY_CLASS              THREAD_PRIORITY_IDLE
 *   1 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_IDLE
 *   1 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_IDLE
 *   2 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_LOWEST
 *   3 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_BELOW_NORMAL
 *   4 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_NORMAL
 *   4 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_LOWEST
 *   5 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_ABOVE_NORMAL
 *   5 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_BELOW_NORMAL
 *   5 Background NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_LOWEST
 *   6 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_HIGHEST
 *   6 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_NORMAL
 *   6 Background NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_BELOW_NORMAL
 *   7 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_ABOVE_NORMAL
 *   7 Background NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_NORMAL
 *   7 Foreground NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_LOWEST
 *   8 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_HIGHEST
 *   8 NORMAL_PRIORITY_CLASS              THREAD_PRIORITY_ABOVE_NORMAL
 *   8 Foreground NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_BELOW_NORMAL
 *   8 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_LOWEST
 *   9 NORMAL_PRIORITY_CLASS              THREAD_PRIORITY_HIGHEST
 *   9 Foreground NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_NORMAL
 *   9 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_BELOW_NORMAL
 *  10 Foreground NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_ABOVE_NORMAL
 *  10 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_NORMAL
 *  11 Foreground NORMAL_PRIORITY_CLASS   THREAD_PRIORITY_HIGHEST
 *  11 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_ABOVE_NORMAL
 *  11 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_LOWEST
 *  12 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_HIGHEST
 *  12 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_BELOW_NORMAL
 *  13 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_NORMAL
 *  14 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_ABOVE_NORMAL
 *  15 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_HIGHEST
 *  15 HIGH_PRIORITY_CLASS                THREAD_PRIORITY_TIME_CRITICAL
 *  15 IDLE_PRIORITY_CLASS                THREAD_PRIORITY_TIME_CRITICAL
 *  15 BELOW_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_TIME_CRITICAL
 *  15 NORMAL_PRIORITY_CLASS              THREAD_PRIORITY_TIME_CRITICAL
 *  15 ABOVE_NORMAL_PRIORITY_CLASS        THREAD_PRIORITY_TIME_CRITICAL
 *  16 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_IDLE
 *  17 REALTIME_PRIORITY_CLASS            -7
 *  18 REALTIME_PRIORITY_CLASS            -6
 *  19 REALTIME_PRIORITY_CLASS            -5
 *  20 REALTIME_PRIORITY_CLASS            -4
 *  21 REALTIME_PRIORITY_CLASS            -3
 *  22 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_LOWEST
 *  23 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_BELOW_NORMAL
 *  24 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_NORMAL
 *  25 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_ABOVE_NORMAL
 *  26 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_HIGHEST
 *  27 REALTIME_PRIORITY_CLASS             3
 *  28 REALTIME_PRIORITY_CLASS             4
 *  29 REALTIME_PRIORITY_CLASS             5
 *  30 REALTIME_PRIORITY_CLASS             6
 *  31 REALTIME_PRIORITY_CLASS            THREAD_PRIORITY_TIME_CRITICAL
 *
 * Windows NT:  Values -7, -6, -5, -4, -3, 3, 4, 5, and 6 are not supported.
 *
 */
int sched_get_priority_min(int policy)
{
	if(policy < SCHED_MIN || policy > SCHED_MAX) {
		__PTW32_SET_ERRNO(EINVAL);
		return -1;
	}
#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
	// WinCE? 
	return __PTW32_MIN(THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#else
	// This is independent of scheduling policy in Win32. 
	return __PTW32_MIN(THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#endif
}

int sched_get_priority_max(int policy)
{
	if(policy < SCHED_MIN || policy > SCHED_MAX) {
		__PTW32_SET_ERRNO(EINVAL);
		return -1;
	}
#if (THREAD_PRIORITY_LOWEST > THREAD_PRIORITY_NORMAL)
	// WinCE? 
	return __PTW32_MAX(THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#else
	// This is independent of scheduling policy in Win32. 
	return __PTW32_MAX(THREAD_PRIORITY_IDLE, THREAD_PRIORITY_TIME_CRITICAL);
#endif
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function indicates that the calling thread is
 *   willing to give up some time slices to other threads.
 *
 * PARAMETERS
 *   N/A
 *
 * DESCRIPTION
 *   This function indicates that the calling thread is
 *   willing to give up some time slices to other threads.
 *   NOTE: Since this is part of POSIX 1003.1b
 *             (realtime extensions), it is defined as returning
 *             -1 if an error occurs and sets errno to the actual error.
 *
 * RESULTS
 *           0               successfully created semaphore,
 *           ENOSYS          sched_yield not supported,
 *
 * ------------------------------------------------------
 */
int sched_yield()
{
	Sleep(0);
	return 0;
}
