/*
 * pthread_equal.c
 *
 * Description: This translation unit implements miscellaneous thread functions.
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
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function returns nonzero if t1 and t2 are equal, else
 *   returns zero
 *
 * PARAMETERS
 *   t1,
 *   t2
 *     thread IDs
 *
 *
 * DESCRIPTION
 *   This function returns nonzero if t1 and t2 are equal, else
 *   returns zero.
 *
 * RESULTS
 *     non-zero        if t1 and t2 refer to the same thread,
 *     0               t1 and t2 do not refer to the same thread
 *
 * ------------------------------------------------------
 */
int pthread_equal(pthread_t t1, pthread_t t2)
{
	// We also accept NULL == NULL - treating NULL as a thread for this special case, because there is no error that we can return.
	int result = (t1.p == t2.p && t1.x == t2.x);
	return result;
}
