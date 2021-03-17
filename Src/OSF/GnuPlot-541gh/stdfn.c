// GNUPLOT - stdfn.c 
// Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
//
// This module collects various functions, which were previously scattered
// all over the place. In a future implementation of gnuplot, each of
// these functions will probably reside in their own file in a subdirectory.
//   - Lars Hecking
// 
#include <gnuplot.h>
#pragma hdrstop
#if defined(_WIN32) && defined(__WATCOMC__)
	#include <direct.h>
#endif
#ifdef _WIN32
	// the WIN32 API has a Sleep function that does not consume CPU cycles 
	#define WIN32_LEAN_AND_MEAN
	#include "win/winmain.h"
	#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#endif
#ifdef NEED_CEXP
	#include <math.h>
	#include <complex.h>
#endif
/*
 * ANSI C functions
 */
// strerror() 
#ifndef HAVE_STRERROR
char * strerror(int no)
{
	static char res_str[30];
	if(no > sys_nerr) {
		sprintf(res_str, "unknown errno %d", no);
		return res_str;
	}
	else
		return sys_errlist[no];
}
#endif
// 
// POSIX functions
//
#ifndef HAVE_SLEEP
/* The implementation below does not even come close
 * to what is required by POSIX.1, but I suppose
 * it doesn't really matter on these systems. lh
 */
uint sleep(uint delay)
{
#if defined(_WIN32)
	Sleep((DWORD)delay * 1000);
#endif
	return 0;
}
#endif /* HAVE_SLEEP */
//
// Other common functions
//
#if 0 // {
	// 
	// portable implementation of strnicmp (hopefully)
	// 
	#ifndef HAVE_STRCASECMP
		#ifndef HAVE_STRICMP
			// 
			// return (see MSVC documentation and strcasecmp()):
			// -1  if str1 < str2
			//  0  if str1 == str2
			//  1  if str1 > str2
			// 
			int gp_stricmp(const char * s1, const char * s2)
			{
				uchar c1, c2;
				do {
					c1 = *s1++;
					if(islower(c1))
						c1 = toupper(c1);
					c2 = *s2++;
					if(islower(c2))
						c2 = toupper(c2);
				} while(c1 == c2 && c1 && c2);
				if(c1 == c2)
					return 0;
				else if(c1 == '\0' || c1 > c2)
					return 1;
				else
					return -1;
			}
		#endif
	#endif
	//
	// portable implementation of strnicmp (hopefully)
	//
	#ifndef HAVE_STRNCASECMP
		#ifndef HAVE_STRNICMP
			int gp_strnicmp(const char * s1, const char * s2, size_t n)
			{
				uchar c1, c2;
				if(n == 0)
					return 0;
				do {
					c1 = *s1++;
					if(islower(c1))
						c1 = toupper(c1);
					c2 = *s2++;
					if(islower(c2))
						c2 = toupper(c2);
				} while(c1 == c2 && c1 && c2 && --n > 0);
				if(n == 0 || c1 == c2)
					return 0;
				else if(c1 == '\0' || c1 > c2)
					return 1;
				else
					return -1;
			}
		#endif
	#endif
	// Safe, '\0'-terminated version of strncpy()
	// strnzcpy(dest, src, n), where n = sizeof(dest)
	char * safe_strncpy_Removed(char * d, const char * s, size_t n)
	{
		char * ret = strncpy(d, s, n);
		if(strlen(s) >= n)
			d[n > 0 ? n - 1 : 0] = NUL;
		return ret;
	}
#endif // } 0
#ifndef HAVE_STRNLEN
size_t strnlen(const char * str, size_t n)
{
	const char * stop = (char *)memchr(str, '\0', n);
	return stop ? stop - str : n;
}
#endif
#ifndef HAVE_STRNDUP
char * strndup(const char * str, size_t n)
{
	size_t len = strnlen(str, n);
	char * ret = (char *)SAlloc::M(len + 1);
	if(ret == NULL) 
		return NULL;
	ret[len] = '\0';
	return (char *)memcpy(ret, str, len);
}
#endif
#ifndef HAVE_STRCSPN
/*
 * our own substitute for strcspn()
 * return the length of the initial segment of str1
 * consisting of characters not in str2
 * returns strlen(str1) if none of the characters
 * from str2 are in str1
 * based in misc.c(instring) */
size_t gp_strcspn(const char * str1, const char * str2)
{
	size_t pos = 0;
	if(str1 && str2) {
		pos = strlen(str1);
		while(*str2++) {
			const char * s = strchr(str1, *str2);
			if(s)
				if((s - str1) < pos)
					pos = (s - str1);
		}
	}
	return pos;
}
#endif /* !HAVE_STRCSPN */

// Standard compliant replacement functions for MSVC 
#if defined(_MSC_VER) && (_MSC_VER < 1900)
int ms_vsnprintf(char * str, size_t size, const char * format, va_list ap)
{
	int count = -1;
	if(size && str)
		count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
	if(count == -1)
		count = _vscprintf(format, ap);
	return count;
}

int ms_snprintf(char * str, size_t size, const char * format, ...)
{
	int result;
	va_list ap;
	va_start(ap, format);
	result = ms_vsnprintf(str, size, format, ap);
	va_end(ap);
	return result;
}

#endif
#if 0 // (replaced with fgetnan())
// 
// Implement portable generation of a NaN value. 
// NB: Supposedly DJGPP V2.04 can use atof("NaN"), but... 
// 
double not_a_number_Removed()
{
#if defined (_MSC_VER) || defined (DJGPP) || defined(__DJGPP__) || defined(__MINGW32__)
	ulong lnan[2] = {0xffffffff, 0x7fffffff};
	return *(double*)lnan;
#else
	return atof("NaN");
#endif
}
#endif 

#ifdef NEED_CEXP
double _Complex cexp(double _Complex z)
{
	double x = creal(z);
	double y = cimag(z);
	return exp(x) * (cos(y) + I*sin(y));
}
#endif
// 
// Version of basename, which does take two possible
// separators into account and does not modify its argument.
// 
char * gp_basename(char * path)
{
	char * basename = strrchr(path, DIRSEP1);
	if(basename) {
		basename++;
		return basename;
	}
#if DIRSEP2 != NUL
	basename = strrchr(path, DIRSEP2);
	if(basename) {
		basename++;
		return basename;
	}
#endif
	return path; // no path separator found 
}

#ifdef HAVE_ATEXIT
	#define GP_ATEXIT(x) atexit((x))
#elif defined(HAVE_ON_EXIT)
	#define GP_ATEXIT(x) on_exit((x), 0)
#else
	#define GP_ATEXIT(x) /* you lose */
#endif

struct EXIT_HANDLER {
	void (* function)();
	struct EXIT_HANDLER* next;
};

static struct EXIT_HANDLER * exit_handlers = NULL;
// 
// Calls the cleanup functions registered using gp_atexit().
// Normally gnuplot should be exited using gp_exit(). In some cases, this is not
// possible (notably when returning from main(), where some compilers get
// confused because they expect a return statement at the very end. In that
// case, gp_exit_cleanup() should be called before the return statement.
// 
void gp_exit_cleanup()
{
	// Call exit handlers registered using gp_atexit(). This is used instead of
	// normal atexit-handlers, because some libraries (notably Qt) seem to have
	// problems with the destruction order when some objects are only destructed
	// on atexit(3). Circumvent this problem by calling the gnuplot
	// atexit-handlers, before any global destructors are run.
	while(exit_handlers) {
		struct EXIT_HANDLER * handler = exit_handlers;
		(*handler->function)();
		// note: assumes that function above has not called gp_atexit() 
		exit_handlers = handler->next;
		SAlloc::F(handler);
	}
}
//
// Called from exit(3). Verifies that all exit handlers have already been called.
//
static void debug_exit_handler()
{
	if(exit_handlers) {
		fprintf(stderr, "Gnuplot exiting abnormally. Trying to execute exit handlers anyway.\n");
		gp_exit_cleanup();
	}
}
//
// Gnuplot replacement for atexit(3) 
//
void gp_atexit(void (*function)())
{
	// Register new handler 
	static bool debug_exit_handler_registered = false;
	EXIT_HANDLER * new_handler = (EXIT_HANDLER *)SAlloc::M(sizeof(EXIT_HANDLER));
	new_handler->function = function;
	new_handler->next = exit_handlers;
	exit_handlers = new_handler;
	if(!debug_exit_handler_registered) {
		GP_ATEXIT(debug_exit_handler);
		debug_exit_handler_registered = true;
	}
}
//
// Gnuplot replacement for exit(3). Calls the functions registered using
// gp_atexit(). Always use this function instead of exit(3)!
//
void gp_exit(int status)
{
	gp_exit_cleanup();
	exit(status);
}

#ifdef _WIN32
char * gp_getcwd(char * path, size_t len)
{
	wchar_t wpath[MAX_PATH + 1];
	if(_wgetcwd(wpath, MAX_PATH)) {
		WideCharToMultiByte(WinGetCodepage(encoding), 0, wpath, -1, path, len, NULL, 0);
		return path;
	}
	return NULL;
}
#endif

#ifdef _WIN32
/* Note: OpenWatcom has dirent functions in direct.h but we use our functions
                 since they support encodings. */
/*

    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
    Rights:  See end of section.

 */

struct DIR {
	intptr_t handle;        /* -1 for failed rewind */
	struct _wfinddata_t info;
	struct gp_dirent result; /* d_name null iff first time */
	WCHAR * name;/* null-terminated string */
	char info_mbname[4*260];
};

DIR * gp_opendir(const char * name)
{
	DIR * dir = 0;
	char * mbname;
	if(!isempty(name)) {
		size_t base_length = strlen(name);
		// search pattern must end with suitable wildcard 
		const char * all = strchr("/\\", name[base_length-1]) ? "*" : "/*";
		dir = (DIR*)SAlloc::M(sizeof(*dir));
		if(dir && (mbname = (char *)SAlloc::M(base_length + strlen(all) + 1)) != NULL) {
			strcat(strcpy(mbname, name), all);
			dir->name = UnicodeText(mbname, encoding);
			SAlloc::F(mbname);
			if(dir->name && ((dir->handle = (long)_wfindfirst(dir->name, &dir->info)) != -1)) {
				dir->result.d_name = NULL;
			}
			else { // rollback 
				SAlloc::F(dir->name);
				ZFREE(dir);
			}
		}
		else { // rollback 
			ZFREE(dir);
			errno = ENOMEM;
		}
	}
	else {
		errno = EINVAL;
	}
	return dir;
}

int gp_closedir(DIR * dir)
{
	int result = -1;
	if(dir) {
		if(dir->handle != -1) {
			result = _findclose(dir->handle);
		}
		SAlloc::F(dir->name);
		SAlloc::F(dir);
	}
	if(result == -1) { /* map all errors to EBADF */
		errno = EBADF;
	}
	return result;
}

struct gp_dirent * gp_readdir(DIR * dir)                    
{
	struct gp_dirent * result = 0;
	if(dir && dir->handle != -1) {
		if(!dir->result.d_name || _wfindnext(dir->handle, &dir->info) != -1) {
			result         = &dir->result;
			WideCharToMultiByte(WinGetCodepage(encoding), 0, dir->info.name, sizeof(dir->info.name) / sizeof(wchar_t),
			    dir->info_mbname, sizeof(dir->info_mbname) / sizeof(char), NULL, 0);
			result->d_name = dir->info_mbname;
		}
	}
	else {
		errno = EBADF;
	}
	return result;
}

void gp_rewinddir(DIR * dir)
{
	if(dir && dir->handle != -1) {
		_findclose(dir->handle);
		dir->handle = (long)_wfindfirst(dir->name, &dir->info);
		dir->result.d_name = NULL;
	}
	else {
		errno = EBADF;
	}
}
/*
    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.

    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

 */
#endif /* _WIN32 */
