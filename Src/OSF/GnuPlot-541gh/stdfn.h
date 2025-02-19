// GNUPLOT - stdfn.h 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
/* get prototypes or declarations for string and stdlib functions and deal
   with missing functions like sstrchr. */

/* we will assume the ANSI/Posix/whatever situation as default.
   the header file is called string.h and the index functions are called
   sstrchr, strrchr. Exceptions have to be listed explicitly */

#ifndef STDFN_H
#define STDFN_H

#ifdef HAVE_STRING_H
	#include <string.h>
#else
	#include <strings.h>
#endif
#ifndef HAVE_STRCSPN
	size_t gp_strcspn(const char *, const char *);
	#define strcspn gp_strcspn
#endif
#ifndef HAVE_STRSTR
	char * strstr(const char *, const char *);
#endif
/* Deal with varargs functions */
#if defined(HAVE_VFPRINTF) || defined(HAVE_DOPRNT)
	#ifdef STDC_HEADERS
		#include <stdarg.h>
		#define VA_START(args, lastarg) va_start(args, lastarg)
	#else
		#include <varargs.h>
		#define VA_START(args, lastarg) va_start(args)
	#endif /* !STDC_HEADERS */
#else /* HAVE_VFPRINTF || HAVE_DOPRNT */
	#define va_dcl char * a1, char * a2, char * a3, char * a4, char * a5, char * a6, char * a7, char * a8
#endif /* !(HAVE_VFPRINTF || HAVE_DOPRNT) */
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#else
	#ifdef HAVE_LIBC_H
		#include <libc.h>
	#endif
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif
#ifdef EXTERN_ERRNO
	extern int errno;
#endif
#ifndef HAVE_STRERROR
	char * strerror(int);
	extern int sys_nerr;
	extern char * sys_errlist[];
#endif
#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
	// This is all taken from GNU fileutils lib/filemode.h 
	#if !S_IRUSR
		#if S_IREAD
			#define S_IRUSR S_IREAD
		#else
			#define S_IRUSR 00400
		#endif
	#endif
	#if !S_IWUSR
		#if S_IWRITE
			#define S_IWUSR S_IWRITE
		#else
			#define S_IWUSR 00200
		#endif
	#endif
	#if !S_IXUSR
		#if S_IEXEC
			#define S_IXUSR S_IEXEC
		#else
			#define S_IXUSR 00100
		#endif
	#endif
	#ifdef STAT_MACROS_BROKEN
		#undef S_ISBLK
		#undef S_ISCHR
		#undef S_ISDIR
		#undef S_ISFIFO
		#undef S_ISLNK
		#undef S_ISMPB
		#undef S_ISMPC
		#undef S_ISNWK
		#undef S_ISREG
		#undef S_ISSOCK
	#endif
	#if !defined(S_ISBLK) && defined(S_IFBLK)
		#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
	#endif
	#if !defined(S_ISCHR) && defined(S_IFCHR)
		#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
	#endif
	#if !defined(S_ISDIR) && defined(S_IFDIR)
		#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
	#endif
	#if !defined(S_ISREG) && defined(S_IFREG)
		#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
	#endif
	#if !defined(S_ISFIFO) && defined(S_IFIFO)
		#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
	#endif
	#if !defined(S_ISLNK) && defined(S_IFLNK)
		#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
	#endif
	#if !defined(S_ISSOCK) && defined(S_IFSOCK)
		#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
	#endif
	#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
		#define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB)
		#define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC)
	#endif
	#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
		#define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
	#endif
#endif /* HAVE_SYS_STAT_H */
#ifdef HAVE_LIMITS_H
	//#include <limits.h>
#else
	#ifdef HAVE_VALUES_H
		#include <values.h>
	#endif
#endif
// ctime etc, should also define time_t and struct tm 
//#ifdef HAVE_TIME_H
	//#include <time.h>
//#endif
#ifndef HAVE_TIME_T_IN_TIME_H
	#define time_t long
#endif
#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h> /* for gettimeofday() */
#endif
//#ifdef HAVE_FLOAT_H
	//#include <float.h>
//#endif
// Some older platforms, namely SunOS 4.x, don't define this. 
#ifndef DBL_EPSILON
	#define DBL_EPSILON     2.2204460492503131E-16
#endif
#ifdef HAVE_LOCALE_H
	// @v11.7.1 #include <locale.h>
#endif
#ifdef HAVE_MATH_H
	#include <math.h>
#endif
//#ifndef M_LN10
	//#define M_LN10    2.3025850929940456840e0
//#endif
// Use definitions from float.h and math.h if we found them 
#if defined(DBL_MIN_10_EXP)
	#define E_MINEXP (DBL_MIN_10_EXP * SMathConst::Ln10)
#endif
#if defined(DBL_MAX_10_EXP)
	#define E_MAXEXP (DBL_MAX_10_EXP * SMathConst::Ln10)
#endif
#ifndef HAVE_STRCASECMP
	#ifdef HAVE_STRICMP
		#define strcasecmp stricmp
	#else
		//int gp_stricmp(const char *, const char *);
		//#define strcasecmp gp_stricmp
	#endif
#endif
#ifndef HAVE_STRNCASECMP
	#ifdef HAVE_STRNICMP
		#define strncasecmp strnicmp
	#else
		//int gp_strnicmp(const char *, const char *, size_t);
		//#define strncasecmp gp_strnicmp
	#endif
#endif
#ifndef HAVE_STRNDUP
	char * strndup(const char * str, size_t n);
#endif
#if defined(_MSC_VER) && (_MSC_VER < 1900)
	int ms_vsnprintf(char * str, size_t size, const char * format, va_list ap);
	int ms_snprintf(char * str, size_t size, const char * format, ...);
#endif
#ifndef GP_GETCWD
#if defined(_WIN32)
#define GP_GETCWD(path, len) gp_getcwd(path, len)
char * gp_getcwd(char * path, size_t len);
# elif defined(HAVE_GETCWD)
#   if defined(__EMX__)
#     define GP_GETCWD(path, len) _getcwd2(path, len)
#   else
#     define GP_GETCWD(path, len) getcwd(path, len)
#endif /* __EMX__ */
#else
#define GP_GETCWD(path, len) getwd(path)
#endif
#endif
// sleep delay time, where delay is a double value 
#if defined(HAVE_USLEEP) && !defined(_WIN32)
	#define GP_SLEEP(delay) usleep((uint)((delay)*1e6))
	#ifndef HAVE_SLEEP
		#define HAVE_SLEEP
	#endif
#elif defined(__EMX__)
	#define GP_SLEEP(_pTerm, delay) _sleep2(_pTerm, (uint)((delay)*1e3))
	#ifndef HAVE_SLEEP
		#define HAVE_SLEEP
	#endif
#elif defined(_WIN32)
	#define GP_SLEEP(_pTerm, delay) win_sleep(_pTerm, (DWORD)1000*delay)
	#ifndef HAVE_SLEEP
		#define HAVE_SLEEP
	#endif
#endif
#ifndef GP_SLEEP
	#define GP_SLEEP(delay) sleep((uint)(delay+0.5))
#endif
// Gnuplot replacement for atexit(3) 
void gp_atexit(void (* function)());

/* Gnuplot replacement for exit(3). Calls the functions registered using
 * gp_atexit(). Always use this function instead of exit(3)!
 */
void gp_exit(int status);

/* Calls the cleanup functions registered using gp_atexit().
 * Normally gnuplot should be exited using gp_exit(). In some cases, this is not
 * possible (notably when returning from main(), where some compilers get
 * confused because they expect a return statement at the very end. In that
 * case, gp_exit_cleanup() should be called before the return statement.
 */
void gp_exit_cleanup();
char * gp_basename(char * path);

#ifdef _WIN32
/*

    Declaration of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
 */
/*

    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.

    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

 */
typedef struct GPDIR GPDIR;

struct gp_dirent {
	char * d_name;
};

GPDIR * gp_opendir(const char *);
int gp_closedir(GPDIR *);
struct gp_dirent * gp_readdir(GPDIR *);
void gp_rewinddir(GPDIR *);
#define opendir(p) gp_opendir(p)
#define closedir(d) gp_closedir(d)
#define readdir(d) gp_readdir(d)
#define rewinddir(d) gp_rewinddir(d)
#define dirent gp_dirent
#define DIR GPDIR
#elif defined(HAVE_DIRENT_H)
	#include <sys/types.h>
	#include <dirent.h>
#elif defined(__WATCOMC__)
	#include <direct.h>
#endif /* !HAVE_DIRENT_H && _WIN32 */
#if defined(HAVE_DIRENT_H) || defined(_WIN32) || defined(__WATCOMC__)
	#define HAVE_DIRENT
#endif
//
// Misc. defines
//
#define NUL ('\0') /* Null character */

// Definitions for debugging 
//#define NDEBUG 
#ifdef DEBUG
	#define DEBUG_WHERE do { fprintf(stderr, "%s:%d ", __FILE__, __LINE__); } while(0)
	#define FPRINTF(a) do { DEBUG_WHERE; fprintf a; } while(0)
#else
	#define DEBUG_WHERE     do { /* nought */ } while(0)
	#define FPRINTF(a)      do { /* nought */ } while(0)
#endif

#define INT_STR_LEN (3* sizeof(int))

/*
 * Note about VERYLARGE:  This is the upper bound double (or float, if PC)
 * numbers. This flag indicates very large numbers. It doesn't have to
 * be the absolutely biggest number on the machine.
 * If your machine doesn't have HUGE, or float.h,
 * define VERYLARGE here.
 *
 * example:
 #define VERYLARGE 1e37
 *
 * To get an appropriate value for VERYLARGE, we can use DBL_MAX (or
 * FLT_MAX on PCs), HUGE or HUGE_VAL. DBL_MAX is usually defined in
 * float.h and is the largest possible double value. HUGE and HUGE_VAL
 * are either DBL_MAX or +Inf (IEEE special number), depending on the
 * compiler. +Inf may cause problems with some buggy fp
 * implementations, so we better avoid that. The following should work
 * better than the previous setup (which used HUGE in preference to
 * DBL_MAX).
 */
/* Now define VERYLARGE. This is usually DBL_MAX/2 - 1. On MS-DOS however
 * we use floats for memory considerations and thus use FLT_MAX.
 */

#ifndef COORDVAL_FLOAT
	#ifdef DBL_MAX
		#define VERYLARGE (DBL_MAX/2-1)
	#endif
#else /* COORDVAL_FLOAT */
	#ifdef FLT_MAX
		#define VERYLARGE (FLT_MAX/2-1)
	#endif
#endif /* COORDVAL_FLOAT */
#ifndef VERYLARGE
	#ifdef HUGE
		#define VERYLARGE (HUGE/2-1)
	#elif defined(HUGE_VAL)
		#define VERYLARGE (HUGE_VAL/2-1)
	#else
		// as a last resort 
		#define VERYLARGE (1e37)
		/* #  warning "using last resort 1e37 as VERYLARGE define, please check your headers" */
		/* Maybe add a note somewhere in the install docs instead */
	#endif /* HUGE */
#endif /* VERYLARGE */
/* _POSIX_PATH_MAX is too small for practical purposes */
#ifdef HAVE_SYS_PARAM_H
	#include <sys/param.h>
#endif
/* HBB 2012-03-18: clang brings its own <limits.h>, which lacks PATH_MAX,
 * and on top of that, Cygwin's MAXPATHLEN is defined by reference to PATH_MAX.
 * So even though clang sees a #defined MAXPATHLEN, there's still no
 * definition available OA*/
#ifndef PATH_MAX
	#if !defined(MAXPATHLEN) || (MAXPATHLEN <= 0)
		#define PATH_MAX 1024
	#else
		#define PATH_MAX MAXPATHLEN
	#endif
#endif
// Concatenate a path name and a file name. The file name
// may or may not end with a "directory separation" character.
// Path must not be NULL, but can be empty
#define PATH_CONCAT(path, file) \
	{ char * p = path; \
	  p += strlen(path); \
	  if(p!=path) p --; \
	  if(*p && (*p != DIRSEP1) && (*p != DIRSEP2)) { \
		  if(*p) p++; *p++ = DIRSEP1; *p = '\0'; \
	  } \
	  strcat(path, file); \
	}
#ifndef inrange
	#define inrange(z, min, max) (((min)<(max)) ? (((z)>=(min)) && ((z)<=(max))) : (((z)>=(max)) && ((z)<=(min))))
#endif
// 
// HBB 20030117: new macro to simplify clipping operations in the
// presence of possibly reversed axes 
// 
#ifndef cliptorange
/*#define cliptorange(z, min, max)                 \
	do {                                        \
		if((min) < (max)) {                     \
			if((z) > (max))                     \
				(z) = (max);                     \
			else if((z) < (min))                \
				(z) = (min);                     \
		} else {                                 \
			if((z) > (min))                     \
				(z) = (min);                     \
			else if((z) < (max))                \
				(z) = (max);                     \
		}                                        \
	} while(0)*/

	#define cliptorange(z, min, max) (z) = sclampx((z), (min), (max))
#endif

#ifndef clip_to_01
	#define clip_to_01(val) ((val) < 0 ? 0 : (val) > 1 ? 1 : (val))
#endif
// 
// both min/max and MIN/MAX are defined by some compilers.
// we are now on GPMIN / GPMAX
// 
//#define GPMAX_Removed(a, b) ( (a) > (b) ? (a) : (b) )
//#define GPMIN_Removed(a, b) ( (a) < (b) ? (a) : (b) )
// 
// Do any supported platforms already have a sgn function?
// 
#define sgn(x) (((x) > 0) ? 1 : (((x) < 0) ? -1 : 0))
//
// Prototypes from "stdfn.c" 
//
//char * safe_strncpy_Removed(char *, const char *, size_t);
#ifndef HAVE_SLEEP
	uint sleep(uint);
#endif

// (replaced with fgetnan()) double not_a_number_Removed();

#endif /* STDFN_H */
