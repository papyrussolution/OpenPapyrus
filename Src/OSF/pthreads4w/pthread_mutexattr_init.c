/*
 * pthread_mutexattr_init.c
 *
 * Description:
 * This translation unit implements mutual exclusion (mutex) primitives.
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
 *   following World Wide Web location:
 *
 *   https://sourceforge.net/p/pthreads4w/wiki/Contributors/
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
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   Initializes a mutex attributes object with default
 *   attributes.
 *
 * PARAMETERS
 *   attr
 *           pointer to an instance of pthread_mutexattr_t
 *
 *
 * DESCRIPTION
 *   Initializes a mutex attributes object with default
 *   attributes.
 *
 *   NOTES:
 *           1)      Used to define mutex types
 *
 * RESULTS
 *           0               successfully initialized attr,
 *           ENOMEM          insufficient memory for attr.
 *
 * ------------------------------------------------------
 */
int pthread_mutexattr_init(pthread_mutexattr_t * attr)
{
	int result = 0;
	pthread_mutexattr_t ma = static_cast<pthread_mutexattr_t>(SAlloc::C(1, sizeof(*ma)));
	if(ma == NULL)
		result = ENOMEM;
	else {
		ma->pshared = PTHREAD_PROCESS_PRIVATE;
		ma->kind = PTHREAD_MUTEX_DEFAULT;
		ma->robustness = PTHREAD_MUTEX_STALLED;
	}
	*attr = ma;
	return result;
}
