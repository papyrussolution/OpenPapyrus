/*
 * pthread_getspecific.c
 *
 * Description:
 * POSIX thread functions which implement thread-specific data (TSD).
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
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *   This function returns the current value of key in the
 *   calling thread. If no value has been set for 'key' in
 *   the thread, NULL is returned.
 *
 * PARAMETERS
 *   key
 *     an instance of pthread_key_t
 *
 *
 * DESCRIPTION
 *   This function returns the current value of key in the
 *   calling thread. If no value has been set for 'key' in
 *   the thread, NULL is returned.
 *
 * RESULTS
 *     key value or NULL on failure
 *
 * ------------------------------------------------------
 */
void * pthread_getspecific(pthread_key_t key)
{
	void * ptr;
	if(!key) {
		ptr = NULL;
	}
	else {
		int lasterror = GetLastError();
#if defined(RETAIN_WSALASTERROR)
		int lastWSAerror = WSAGetLastError();
#endif
		ptr = TlsGetValue(key->key);
		SetLastError(lasterror);
#if defined(RETAIN_WSALASTERROR)
		WSASetLastError(lastWSAerror);
#endif
	}
	return ptr;
}
