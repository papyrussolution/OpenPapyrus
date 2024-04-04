/* ac-config.win32.h.  Generated from ac-config.h.in by configure.  Patched by [i_a] to make it work for MSVC2005.  */
/* src/common/xlconfig.h.in.  Generated from configure.ac by autoheader.  */


#ifndef WIN32
#define WIN32
#endif

#ifndef _WIN32
#define _WIN32
#endif

#ifndef __WIN32
#define __WIN32
#endif


/* Define if you want to compile with extra debug development checks */
#if defined(_DEBUG) || defined(DEBUG) || defined(__DEBUG__) /* [i_a] this is done in the MSVC2005 project: different builds available */
#undef DEBUG
#define DEBUG 1
#else
#undef DEBUG
#endif


/* Define to 1 if you have the <crtdbg.h> header file. */
#define HAVE_CRTDBG_H 1

/* Define to 1 if you have the <crt_externs.h> header file. */
#define HAVE_CRT_EXTERNS_H 1

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define if you have the iconv() function. */
#undef HAVE_ICONV

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if the system has the type `long long int'. */
#define HAVE_LONG_LONG_INT 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the `memchr' function. */
#define HAVE_MEMCHR 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define if you have cygwin with new paths style */
#define HAVE_NEWSTYLE_CYGWIN 1   /* [i_a] fixup apr/2007 - native Win32 port hack to ensure the drive:/ prefix doesn't need to be fixed EVERYWHERE, leading to code chaos */

/* Define if compiler supports #pragma pack(<size>). */
#define HAVE_PRAGMA_PACK 1

/* Define if compiler does not listen strictly to large <size>s in #pragma
   pack(<size>) but applies further member packing instead when none of the
   (basic) members are <size> bytes or more. */
#define HAVE_PRAGMA_PACK_OVERSMART_COMPILER 1

/* Define if compiler supports #pragma pack(push) / pack(pop) and
   pack(<size>). */
#define HAVE_PRAGMA_PACK_PUSH_POP 1

/* Define to 1 if you have the `snprintf' function. */
#undef HAVE_SNPRINTF

/* Define to 1 if stdbool.h conforms to C99. */
#undef HAVE_STDBOOL_H

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#undef HAVE_STRCASECMP

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strcmp' function. */
#define HAVE_STRCMP 1

/* Define to 1 if you have the `stricmp' function. */
#define HAVE_STRICMP 1

/* Define to 1 if cpp supports the ANSI # stringizing operator. */
#define HAVE_STRINGIZE 1

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `vsnprintf' function. */
#undef HAVE_VSNPRINTF

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

// @v11.9.11 #define HAVE__BOOL 1 /* Define to 1 if the system has the type `_Bool'. */

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF 1

/* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF 1

/* Define if compiler implements __FUNCTION__. */
#define HAVE___FUNCTION__ 1

/* Define if compiler implements __func__. */
#undef HAVE___FUNC__

/* Set host type */
#define HOSTTYPE "Windows-MS"

/* Name of package */
#define PACKAGE "xlslib"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "dhoerl@users.sourceforge.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "xlslib"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "xlslib 2.4.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "xlslib"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.4.0"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT sizeof(int)  /* 4 */

/* The size of `long int', as computed by sizeof. */
#define SIZEOF_LONG_INT sizeof(long int) /* 4 */

/* The size of `long long int', as computed by sizeof. */
#define SIZEOF_LONG_LONG_INT sizeof(long long int) /* 8 */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.4.0"

/* DISable ASSERT/VERIFY checks */
#if !defined(DEBUG)
#define XLSLIB_DONT_ASSERT 1
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
/* # undef __CHAR_UNSIGNED__ */
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline
#endif

typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
// @sobolev typedef __int8 int8_t;

typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;


/* Define to the type of a signed integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int16_t */

/* Define to the type of a signed integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int32_t */

/* Define to the type of a signed integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int64_t */

/* Define to the type of a signed integer type of width exactly 8 bits if such
   a type exists and the standard includes do not define it. */
/* #undef int8_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `long' if <sys/types.h> does not define. */
#if defined(_MSC_VER) /* [i_a] */
#if defined(_WIN64)
#  define ssize_t __int64
#else
#  define ssize_t long
#endif
#else
#  define ssize_t long
#endif

