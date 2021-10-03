// pthread_attr.c
//
/*
 * Description:
 * This translation unit implements operations on thread attribute objects.
 *
 * --------------------------------------------------------------------------
 *
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
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop

#if defined(_MSC_VER)
	#pragma warning( disable : 4100 ) // ignore warning "unreferenced formal parameter" 
#endif
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a thread attributes object with default attributes.
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 * DESCRIPTION
 *   Initializes a thread attributes object with default attributes.
 *   NOTES:
 *           1)      Used to define thread attributes
 * RESULTS
 *           0               successfully initialized attr,
 *           ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_attr_init(pthread_attr_t * attr)
{
	pthread_attr_t attr_result;
	cpu_set_t cpuset;
	if(!attr) {
		return EINVAL; // This is disallowed. 
	}
	attr_result = (pthread_attr_t)SAlloc::M(sizeof(*attr_result));
	if(attr_result == NULL) {
		return ENOMEM;
	}
#if defined(_POSIX_THREAD_ATTR_STACKSIZE)
	/*
	 * Default to zero size. Unless changed explicitly this
	 * will allow Win32 to set the size to that of the
	 * main thread.
	 */
	attr_result->stacksize = 0;
#endif
#if defined(_POSIX_THREAD_ATTR_STACKADDR)
	/* FIXME: Set this to something sensible when we support it. */
	attr_result->stackaddr = NULL;
#endif
	attr_result->detachstate = PTHREAD_CREATE_JOINABLE;
#if defined(HAVE_SIGSET_T)
	memzero(&(attr_result->sigmask), sizeof(sigset_t));
#endif /* HAVE_SIGSET_T */
	/*
	 * Win32 sets new threads to THREAD_PRIORITY_NORMAL and
	 * not to that of the parent thread. We choose to default to
	 * this arrangement.
	 */
	attr_result->param.sched_priority = THREAD_PRIORITY_NORMAL;
	attr_result->inheritsched = PTHREAD_EXPLICIT_SCHED;
	attr_result->contentionscope = PTHREAD_SCOPE_SYSTEM;
	CPU_ZERO(&cpuset);
	attr_result->cpuset = ((_sched_cpu_set_vector_*)&cpuset)->_cpuset;
	attr_result->thrname = NULL;
	attr_result->valid =  __PTW32_ATTR_VALID;
	*attr = attr_result;
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Destroys a thread attributes object.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *
 *
 * DESCRIPTION
 *   Destroys a thread attributes object.
 *
 *   NOTES:
 *           1)      Does not affect threads created with 'attr'.
 *
 * RESULTS
 *           0               successfully destroyed attr,
 *           EINVAL          'attr' is invalid.
 *
 * ------------------------------------------------------
 */
int pthread_attr_destroy(pthread_attr_t * attr)
{
	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	// Set the attribute object to a specific invalid value.
	(*attr)->valid = 0;
	SAlloc::F(*attr);
	*attr = NULL;
	return 0;
}

int pthread_attr_getaffinity_np(const pthread_attr_t * attr, size_t cpusetsize, cpu_set_t * cpuset)
{
	if(__ptw32_is_attr(attr) != 0 || !cpuset)
		return EINVAL;
	else {
		reinterpret_cast<_sched_cpu_set_vector_ *>(cpuset)->_cpuset = (*attr)->cpuset;
		return 0;
	}
}

int pthread_attr_setaffinity_np(pthread_attr_t * attr, size_t cpusetsize, const cpu_set_t * cpuset)
{
	if(__ptw32_is_attr(attr) != 0 || cpuset == NULL) {
		return EINVAL;
	}
	(*attr)->cpuset = ((_sched_cpu_set_vector_*)cpuset)->_cpuset;
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function determines whether threads created with 'attr' will run detached.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *
 *   detachstate
 *           pointer to an integer into which is returned one of:
 *
 *           PTHREAD_CREATE_JOINABLE
 *                           Thread ID is valid, must be joined
 *
 *           PTHREAD_CREATE_DETACHED
 *                           Thread ID is invalid, cannot be joined, canceled, or modified
 *
 *
 * DESCRIPTION
 *   This function determines whether threads created with 'attr' will run detached.
 *
 *   NOTES:
 *           1)      You cannot join or cancel detached threads.
 *
 * RESULTS
 *           0               successfully retrieved detach state,
 *           EINVAL          'attr' is invalid
 *
 * ------------------------------------------------------
 */
int pthread_attr_getdetachstate(const pthread_attr_t * attr, int * detachstate)
{
	if(__ptw32_is_attr(attr) != 0 || detachstate == NULL) {
		return EINVAL;
	}
	*detachstate = (*attr)->detachstate;
	return 0;
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function specifies whether threads created with
 *   'attr' will run detached.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *
 *   detachstate
 *           an integer containing one of:
 *
 *           PTHREAD_CREATE_JOINABLE
 *                           Thread ID is valid, must be joined
 *
 *           PTHREAD_CREATE_DETACHED
 *                           Thread ID is invalid, cannot be joined,
 *                           canceled, or modified
 *
 *
 * DESCRIPTION
 *   This function specifies whether threads created with
 *   'attr' will run detached.
 *
 *   NOTES:
 *           1)      You cannot join or cancel detached threads.
 *
 * RESULTS
 *           0               successfully set detach state,
 *           EINVAL          'attr' or 'detachstate' is invalid
 *
 * ------------------------------------------------------
 */
int pthread_attr_setdetachstate(pthread_attr_t * attr, int detachstate)
{
	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	if(detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED) {
		return EINVAL;
	}
	(*attr)->detachstate = detachstate;
	return 0;
}

int pthread_attr_getinheritsched(const pthread_attr_t * attr, int * inheritsched)
{
	if(__ptw32_is_attr(attr) != 0 || inheritsched == NULL) {
		return EINVAL;
	}
	*inheritsched = (*attr)->inheritsched;
	return 0;
}

int pthread_attr_setinheritsched(pthread_attr_t * attr, int inheritsched)
{
	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	if(PTHREAD_INHERIT_SCHED != inheritsched && PTHREAD_EXPLICIT_SCHED != inheritsched) {
		return EINVAL;
	}
	(*attr)->inheritsched = inheritsched;
	return 0;
}

int pthread_attr_getname_np(pthread_attr_t * attr, char * name, int len)
{
	//strncpy_s(name, len - 1, (*attr)->thrname, len - 1);
#if defined(_MSVCRT_)
#pragma warning(suppress:4996)
	strncpy(name, (*attr)->thrname, len - 1);
	(*attr)->thrname[len - 1] = '\0';
#endif
	return 0;
}

#if defined (__PTW32_COMPATIBILITY_BSD) || defined (__PTW32_COMPATIBILITY_TRU64)
	int pthread_attr_setname_np(pthread_attr_t * attr, const char * name, void * arg)
	{
		int len;
		int result;
		char tmpbuf[PTHREAD_MAX_NAMELEN_NP];
		char * newname;
		char * oldname;
		/*
		 * According to the MSDN description for snprintf()
		 * where count is the second parameter:
		 * If len < count, then len characters are stored in buffer, a null-terminator is appended, and len is returned.
		 * If len = count, then len characters are stored in buffer, no null-terminator is appended, and len is
		 *returned.
		 * If len > count, then count characters are stored in buffer, no null-terminator is appended, and a negative
		 *value is returned.
		 *
		 * This is different to the POSIX behaviour which returns the number of characters that would have been written
		 *in all cases.
		 */
		len = snprintf(tmpbuf, PTHREAD_MAX_NAMELEN_NP-1, name, arg);
		tmpbuf[PTHREAD_MAX_NAMELEN_NP-1] = '\0';
		if(len < 0) {
			return EINVAL;
		}
		newname = _strdup(tmpbuf);
		oldname = (*attr)->thrname;
		(*attr)->thrname = newname;
		SAlloc::F(oldname);
		return 0;
	}
#else
	int pthread_attr_setname_np(pthread_attr_t * attr, const char * name)
	{
		char * newname = _strdup(name);
		char * oldname = (*attr)->thrname;
		(*attr)->thrname = newname;
		SAlloc::F(oldname);
		return 0;
	}
#endif

int pthread_attr_getschedparam(const pthread_attr_t * attr, struct sched_param * param)
{
	if(__ptw32_is_attr(attr) != 0 || param == NULL) {
		return EINVAL;
	}
	memcpy(param, &(*attr)->param, sizeof(*param));
	return 0;
}

int pthread_attr_setschedparam(pthread_attr_t * attr, const struct sched_param * param)
{
	int priority;
	if(__ptw32_is_attr(attr) != 0 || param == NULL) {
		return EINVAL;
	}
	priority = param->sched_priority;
	/* Validate priority level. */
	if(priority < sched_get_priority_min(SCHED_OTHER) ||
	    priority > sched_get_priority_max(SCHED_OTHER)) {
		return EINVAL;
	}
	memcpy(&(*attr)->param, param, sizeof(*param));
	return 0;
}

int pthread_attr_getschedpolicy(const pthread_attr_t * attr, int * policy)
{
	if(__ptw32_is_attr(attr) != 0 || policy == NULL) {
		return EINVAL;
	}
	else {
		*policy = SCHED_OTHER;
		return 0;
	}
}

int pthread_attr_setschedpolicy(pthread_attr_t * attr, int policy)
{
	if(__ptw32_is_attr(attr) != 0)
		return EINVAL;
	else if(policy != SCHED_OTHER)
		return ENOTSUP;
	else
		return 0;
}

int pthread_attr_getscope(const pthread_attr_t * attr, int * contentionscope)
{
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
	*contentionscope = (*attr)->contentionscope;
	return 0;
#else
	return ENOSYS;
#endif
}

int pthread_attr_setscope(pthread_attr_t * attr, int contentionscope)
{
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
	switch(contentionscope) {
		case PTHREAD_SCOPE_SYSTEM:
		    (*attr)->contentionscope = contentionscope;
		    return 0;
		case PTHREAD_SCOPE_PROCESS:
		    return ENOTSUP;
		default:
		    return EINVAL;
	}
#else
	return ENOSYS;
#endif
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function determines the address of the stack
 *   on which threads created with 'attr' will run.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *
 *   stackaddr
 *           pointer into which is returned the stack address.
 *
 *
 * DESCRIPTION
 *   This function determines the address of the stack
 *   on which threads created with 'attr' will run.
 *
 *   NOTES:
 *           1)      Function supported only if this macro is defined:
 *
 *                           _POSIX_THREAD_ATTR_STACKADDR
 *
 *           2)      Create only one thread for each stack address..
 *
 * RESULTS
 *           0               successfully retrieved stack address,
 *           EINVAL          'attr' is invalid
 *           ENOSYS          function not supported
 *
 * ------------------------------------------------------
 */
int pthread_attr_getstackaddr(const pthread_attr_t * attr, void ** stackaddr)
{
#if defined( _POSIX_THREAD_ATTR_STACKADDR ) && _POSIX_THREAD_ATTR_STACKADDR != -1

	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	*stackaddr = (*attr)->stackaddr;
	return 0;
#else
	return ENOSYS;
#endif
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Threads created with 'attr' will run on the stack
 *   starting at 'stackaddr'.
 *   Stack must be at least PTHREAD_STACK_MIN bytes.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *   stackaddr
 *           the address of the stack to use
 *
 * DESCRIPTION
 *   Threads created with 'attr' will run on the stack starting at 'stackaddr'.
 *   Stack must be at least PTHREAD_STACK_MIN bytes.
 *
 *   NOTES:
 *           1)      Function supported only if this macro is defined: _POSIX_THREAD_ATTR_STACKADDR
  *           2)      Create only one thread for each stack address..
  *           3)      Ensure that stackaddr is aligned.
 *
 * RESULTS
 *           0               successfully set stack address,
 *           EINVAL          'attr' is invalid
 *           ENOSYS          function not supported
 *
 * ------------------------------------------------------
 */
int pthread_attr_setstackaddr(pthread_attr_t * attr, void * stackaddr)
{
#if defined( _POSIX_THREAD_ATTR_STACKADDR ) && _POSIX_THREAD_ATTR_STACKADDR != -1

	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	(*attr)->stackaddr = stackaddr;
	return 0;
#else
	return ENOSYS;
#endif /* _POSIX_THREAD_ATTR_STACKADDR */
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function determines the size of the stack on which threads created with 'attr' will run.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *
 *   stacksize
 *           pointer to size_t into which is returned the stack size, in bytes.
 *
 *
 * DESCRIPTION
 *   This function determines the size of the stack on
 *   which threads created with 'attr' will run.
 *
 *   NOTES:
 *           1)      Function supported only if this macro is defined:
 *
 *                           _POSIX_THREAD_ATTR_STACKSIZE
 *
 *           2)      Use on newly created attributes object to find the default stack size.
 *
 * RESULTS
 *           0               successfully retrieved stack size,
 *           EINVAL          'attr' is invalid
 *           ENOSYS          function not supported
 *
 * ------------------------------------------------------
 */
int pthread_attr_getstacksize(const pthread_attr_t * attr, size_t * stacksize)
{
#if defined(_POSIX_THREAD_ATTR_STACKSIZE) && _POSIX_THREAD_ATTR_STACKSIZE != -1

	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	// Everything is okay.
	*stacksize = (*attr)->stacksize;
	return 0;
#else
	return ENOSYS;
#endif
}
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function specifies the size of the stack on
 *   which threads created with 'attr' will run.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_attr_t
 *   stacksize
 *           stack size, in bytes.
 *
 * DESCRIPTION
 *   This function specifies the size of the stack on
 *   which threads created with 'attr' will run.
 *
 *   NOTES:
 *           1)      Function supported only if this macro is defined:
 *
 *                           _POSIX_THREAD_ATTR_STACKSIZE
 *
 *           2)      Find the default first (using pthread_attr_getstacksize), then increase by multiplying.
 *
 *           3)      Only use if thread needs more than the default.
 *
 * RESULTS
 *           0               successfully set stack size,
 *           EINVAL          'attr' is invalid or stacksize too small or too big.
 *           ENOSYS          function not supported
 *
 * ------------------------------------------------------
 */
int pthread_attr_setstacksize(pthread_attr_t * attr, size_t stacksize)
{
#if defined(_POSIX_THREAD_ATTR_STACKSIZE) && _POSIX_THREAD_ATTR_STACKSIZE != -1
#if PTHREAD_STACK_MIN > 0
	// Verify that the stack size is within range.
	if(stacksize < PTHREAD_STACK_MIN) {
		return EINVAL;
	}
#endif
	if(__ptw32_is_attr(attr) != 0) {
		return EINVAL;
	}
	// Everything is okay. 
	(*attr)->stacksize = stacksize;
	return 0;
#else
	return ENOSYS;
#endif
}
