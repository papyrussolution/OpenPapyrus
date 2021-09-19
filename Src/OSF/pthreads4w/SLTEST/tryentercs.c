/*
 * tryentercs.c
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
//
// See if we have the TryEnterCriticalSection function.
// Does not use any part of pthreads.
// 
int PThr4wTest_TryEnterCs1()
{
	// Function pointer to TryEnterCriticalSection if it exists - otherwise NULL
	static BOOL (WINAPI *_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
	static HINSTANCE _h_kernel32; // Handle to kernel32.dll

	CRITICAL_SECTION cs;
	SetLastError(0);
	printf("Last Error [main enter] %ld\n", (long)GetLastError());
	/*
	 * Load KERNEL32 and try to get address of TryEnterCriticalSection
	 */
	_h_kernel32 = LoadLibrary(_T("KERNEL32.DLL"));
	_try_enter_critical_section = (BOOL (WINAPI *)(LPCRITICAL_SECTION))GetProcAddress(_h_kernel32, (LPCSTR)"TryEnterCriticalSection");
	if(_try_enter_critical_section != NULL) {
		InitializeCriticalSection(&cs);
		SetLastError(0);
		if((*_try_enter_critical_section)(&cs) != 0) {
			LeaveCriticalSection(&cs);
		}
		else {
			printf("Last Error [try enter] %ld\n", (long)GetLastError());
			_try_enter_critical_section = NULL;
		}
		DeleteCriticalSection(&cs);
	}
	(void)FreeLibrary(_h_kernel32);
	printf("This system %s TryEnterCriticalSection.\n", (_try_enter_critical_section == NULL) ? "DOES NOT SUPPORT" : "SUPPORTS");
	printf("POSIX Mutexes will be based on Win32 %s.\n", (_try_enter_critical_section == NULL) ? "Mutexes" : "Critical Sections");
	return 0;
}
// 
// See if we have the TryEnterCriticalSection function.
// Does not use any part of pthreads.
// 
int PThr4wTest_TryEnterCs2()
{
	// Function pointer to TryEnterCriticalSection if it exists - otherwise NULL
	static BOOL (WINAPI *_try_enter_critical_section)(LPCRITICAL_SECTION) = NULL;
	// @sobolev LPCRITICAL_SECTION lpcs = NULL;
	SetLastError(0);
	printf("Last Error [main enter] %ld\n", (long)GetLastError());
	{
		CRITICAL_SECTION cs; // @sobolev
		InitializeCriticalSection(&cs); // @sobolev
		// Load KERNEL32 and try to get address of TryEnterCriticalSection
		HINSTANCE _h_kernel32 = LoadLibrary(_T("KERNEL32.DLL"));
		_try_enter_critical_section = (BOOL (WINAPI *)(LPCRITICAL_SECTION))GetProcAddress(_h_kernel32, (LPCSTR)"TryEnterCriticalSection");
		if(_try_enter_critical_section != NULL) {
			SetLastError(0);
			(*_try_enter_critical_section)(&cs); // @sobolev lpcs-->&cs
			printf("Last Error [try enter] %ld\n", (long)GetLastError());
		}
		FreeLibrary(_h_kernel32);
		DeleteCriticalSection(&cs); // @sobolev
	}
	printf("This system %s TryEnterCriticalSection.\n", (_try_enter_critical_section == NULL) ? "DOES NOT SUPPORT" : "SUPPORTS");
	printf("POSIX Mutexes will be based on Win32 %s.\n", (_try_enter_critical_section == NULL) ? "Mutexes" : "Critical Sections");
	return 0;
}
