/*
 * sched_getschedparam.c
 *
 * Description:
 * POSIX thread functions that deal with thread scheduling.
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
 */
#include <sl_pthreads4w.h>
#pragma hdrstop

int pthread_getschedparam(pthread_t thread, int * policy, struct sched_param * param)
{
	/*
	 * Validate the thread id. This method works for pthreads-win32 because
	 * pthread_kill and pthread_t are designed to accommodate it, but the
	 * method is not portable.
	 */
	int result = pthread_kill(thread, 0);
	if(result) {
		return result;
	}
	if(policy == NULL) {
		return EINVAL;
	}
	/* Fill out the policy. */
	*policy = SCHED_OTHER;
	/*
	 * This function must return the priority value set by
	 * the most recent pthread_setschedparam() or pthread_create()
	 * for the target thread. It must not return the actual thread
	 * priority as altered by any system priority adjustments etc.
	 */
	param->sched_priority = (static_cast<__ptw32_thread_t *>(thread.p))->sched_priority;
	return 0;
}
