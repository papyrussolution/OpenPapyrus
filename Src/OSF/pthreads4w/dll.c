/*
 * dll.c
 * Description: This translation unit implements DLL initialisation.
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

#if !defined (__PTW32_STATIC_LIB)
#if defined(_MSC_VER)
	#pragma warning( disable : 4100 ) // lpvReserved yields an unreferenced formal parameter; ignore it
#endif

__PTW32_BEGIN_C_DECLS

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
	BOOL result =  TRUE;
	switch(fdwReason) {
		case DLL_PROCESS_ATTACH: result = pthread_win32_process_attach_np(); break;
		case DLL_THREAD_ATTACH:  result = pthread_win32_thread_attach_np(); break; // A thread is being created
		case DLL_THREAD_DETACH:  result = pthread_win32_thread_detach_np(); break; // A thread is exiting cleanly
		case DLL_PROCESS_DETACH: (void)pthread_win32_thread_detach_np(); result = pthread_win32_process_detach_np(); break;
	}
	return result;
}

__PTW32_END_C_DECLS

#endif /* !PTW32_STATIC_LIB */

#if !defined (__PTW32_BUILD_INLINED)
	typedef int foo; // Avoid "translation unit is empty" warnings
#endif

#if defined(__PTW32_STATIC_LIB)
/*
 * Note: MSVC 8 and higher use code in dll.c, which enables TLS cleanup
 * on thread exit. Code here can only do process init and exit functions.
 */
#if defined(__MINGW32__) || defined(_MSC_VER)

/* For an explanation of this code (at least the MSVC parts), refer to
 *
 * http://www.codeguru.com/cpp/misc/misc/threadsprocesses/article.php/c6945/
 * ("Running Code Before and After Main")
 *
 * Compatibility with MSVC8 was cribbed from Boost:
 *
 * http://svn.boost.org/svn/boost/trunk/libs/thread/src/win32/tss_pe.cpp
 *
 * In addition to that, because we are in a static library, and the linker
 * can't tell that the constructor/destructor functions are actually
 * needed, we need a way to prevent the linker from optimizing away this
 * module. The pthread_win32_autostatic_anchor() hack below (and in
 * implement.h) does the job in a portable manner.
 *
 * Make everything "extern" to evade being optimized away.
 * Yes, "extern" is implied if not "static" but we are indicating we are
 * doing this deliberately.
 */

extern int __ptw32_on_process_init(void)
{
	pthread_win32_process_attach_np();
	return 0;
}

extern int __ptw32_on_process_exit(void)
{
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
	return 0;
}

#if defined(__GNUC__)
__attribute__((section(".ctors"), used)) extern int (*gcc_ctor)(void) = __ptw32_on_process_init;
__attribute__((section(".dtors"), used)) extern int (*gcc_dtor)(void) = __ptw32_on_process_exit;
#elif defined(_MSC_VER)
#if _MSC_VER >= 1400 /* MSVC8+ */
#pragma section(".CRT$XCU", long, read)
#pragma section(".CRT$XPU", long, read)
__declspec(allocate(".CRT$XCU")) extern int (*msc_ctor)(void) = __ptw32_on_process_init;
__declspec(allocate(".CRT$XPU")) extern int (*msc_dtor)(void) = __ptw32_on_process_exit;
#else
#pragma data_seg(".CRT$XCU")
extern int (*msc_ctor)(void) = __ptw32_on_process_init;
#pragma data_seg(".CRT$XPU")
extern int (*msc_dtor)(void) = __ptw32_on_process_exit;
#pragma data_seg() /* reset data segment */
#endif
#endif

#endif /* defined(__MINGW32__) || defined(_MSC_VER) */

__PTW32_BEGIN_C_DECLS

/* This dummy function exists solely to be referenced by other modules
 * (specifically, in implement.h), so that the linker can't optimize away
 * this module. Don't call it.
 *
 * Shouldn't work if we are compiling via pthreads.c
 * (whole library single translation unit)
 * Leaving it here in case it affects small-static builds.
 */
void __ptw32_autostatic_anchor(void) 
{
	abort();
}

__PTW32_END_C_DECLS

#endif /* __PTW32_STATIC_LIB */
