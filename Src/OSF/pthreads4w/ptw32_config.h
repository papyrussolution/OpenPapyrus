/* config.h  */

#ifndef  __PTW32_CONFIG_H
#define __PTW32_CONFIG_H
// 
// Defaults: see target specific redefinitions below.
// 
#define __PTW32_BUILD /* We're building the pthreads-win32 library */
#define HAVE_CPU_AFFINITY /* CPU affinity */
#undef HAVE_SIGSET_T /* Do we know about the C type sigset_t? */
#undef HAVE_SIGNAL_H /* Define if you have the <signal.h> header file.  */
#undef HAVE_TASM32 /* Define if you have the Borland TASM32 or compatible assembler.  */
#undef NEED_DUPLICATEHANDLE /* Define if you don't have Win32 DuplicateHandle. (eg. WinCE) */
#undef NEED_CREATETHREAD /* Define if you don't have Win32 _beginthreadex. (eg. WinCE) */
#undef NEED_ERRNO /* Define if you don't have Win32 errno. (eg. WinCE) */
#undef NEED_CALLOC /* Define if you don't have Win32 calloc. (eg. WinCE)  */
#undef NEED_SEM /* Define if you don't have Win32 semaphores. (eg. WinCE 2.1 or earlier)  */
#undef NEED_UNICODE_CONSTS /* Define if you need to convert string parameters to unicode. (eg. WinCE)  */
#undef HAVE_C_INLINE /* Define if your C (not C++) compiler supports "inline" functions. */
#undef HAVE_MODE_T /* Do we know about type mode_t? */
/*
 * Define if GCC has atomic builtins, i.e. __sync_* intrinsics
 * __sync_lock_* is implemented in mingw32 gcc 4.5.2 at least
 * so this define does not turn those on or off. If you get an
 * error from __sync_lock* then consider upgrading your gcc.
 */
#undef HAVE_GCC_ATOMIC_BUILTINS
#undef HAVE_STRUCT_TIMESPEC /* Define if you have the timespec struct */
#undef NEED_PROCESS_AFFINITY_MASK /* Define if you don't have the GetProcessAffinityMask() */
/* Define if your version of Windows TLSGetValue() clears WSALastError
 * and calling SetLastError() isn't enough restore it. You'll also need to
 * link against wsock32.lib (or libwsock32.a for MinGW).
 */
#undef RETAIN_WSALASTERROR
/*
 # ----------------------------------------------------------------------
 # The library can be built with some alternative behaviour to better
 # facilitate development of applications on Win32 that will be ported
 # to other POSIX systems.
 #
 # Nothing described here will make the library non-compliant and strictly
 # compliant applications will not be affected in any way, but
 # applications that make assumptions that POSIX does not guarantee are
 # not strictly compliant and may fail or misbehave with some settings.
 #
 #  __PTW32_THREAD_ID_REUSE_INCREMENT
 # Purpose:
 # POSIX says that applications should assume that thread IDs can be
 # recycled. However, Solaris (and some other systems) use a [very large]
 # sequence number as the thread ID, which provides virtual uniqueness.
 # This provides a very high but finite level of safety for applications
 # that are not meticulous in tracking thread lifecycles e.g. applications
 # that call functions which target detached threads without some form of
 # thread exit synchronisation.
 #
 # Usage:
 # Set to any value in the range: 0 <= value < 2^wordsize.
 # Set to 0 to emulate reusable thread ID behaviour like Linux or *BSD.
 # Set to 1 for unique thread IDs like Solaris (this is the default).
 # Set to some factor of 2^wordsize to emulate smaller word size types
 # (i.e. will wrap sooner). This might be useful to emulate some embedded
 # systems.
 #
 #define __PTW32_THREAD_ID_REUSE_INCREMENT 0
 #
 # ----------------------------------------------------------------------
 */
#undef  __PTW32_THREAD_ID_REUSE_INCREMENT
// 
// Target specific groups
// 
// If you find that these are incorrect or incomplete please report it
// to the pthreads-win32 maintainer. Thanks.
// 
#if defined(WINCE)
	#undef  HAVE_CPU_AFFINITY
	#define NEED_DUPLICATEHANDLE
	#define NEED_CREATETHREAD
	#define NEED_ERRNO
	#define NEED_CALLOC
	#define NEED_FTIME
	/*#define NEED_SEM */
	#define NEED_UNICODE_CONSTS
	#define NEED_PROCESS_AFFINITY_MASK
	/* This may not be needed */
	#define RETAIN_WSALASTERROR
#endif
#if defined(_MSC_VER)
	#if _MSC_VER >= 1900
		#define HAVE_STRUCT_TIMESPEC
	#elif _MSC_VER < 1300
		#define __PTW32_CONFIG_MSVC6
	#elif _MSC_VER < 1400
		#define __PTW32_CONFIG_MSVC7
	#endif
#endif
#if defined(_UWIN)
	#define HAVE_MODE_T
	#define HAVE_STRUCT_TIMESPEC
	#define HAVE_SIGNAL_H
#endif
#if defined(__GNUC__)
	#define HAVE_C_INLINE
#endif
#if defined(__BORLANDC__)
#endif
#if defined(__WATCOMC__)
#endif
#if defined(__DMC__)
	#define HAVE_SIGNAL_H
	#define HAVE_C_INLINE
#endif
#endif /* __PTW32_CONFIG_H */
