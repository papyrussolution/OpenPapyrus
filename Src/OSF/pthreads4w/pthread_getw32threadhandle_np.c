/*
 * pthread_getw32threadhandle_np.c
 * Description: This translation unit implements non-portable thread functions.
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
 * pthread_getw32threadhandle_np()
 *
 * Returns the win32 thread handle that the POSIX
 * thread "thread" is running as.
 *
 * Applications can use the win32 handle to set
 * win32 specific attributes of the thread.
 */
HANDLE pthread_getw32threadhandle_np(pthread_t thread) { return (static_cast<__ptw32_thread_t *>(thread.p))->threadH; }
/*
 * pthread_getw32threadid_np()
 *
 * Returns the win32 thread id that the POSIX
 * thread "thread" is running as.
 */
DWORD pthread_getw32threadid_np(pthread_t thread) { return (static_cast<__ptw32_thread_t *>(thread.p))->thread; }
