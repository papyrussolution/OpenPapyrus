// PORT.H
// Copyright (c) V.Antonov, A.Sobolev 1999-2001, 2002, 2003, 2007, 2008, 2009, 2010, 2011, 2015
//
#ifdef _WIN32
	#ifndef __WIN32__
		#define __WIN32__ 1 // (__WIN32__)-->(__WIN32__ 1)
	#endif
	#define STRICT
	#define OEMRESOURCE
	#define _WIN32_WINDOWS 0x0410 // @papyrus @v4.1.12
	#define SECURITY_WIN32        // @v6.5.12
	// @papyrus @v5.9.3 {
	#ifndef _WIN32_IE
		#define _WIN32_IE 0x0500
	#endif
	// } @papyrus @v5.9.3
	#include <winsock2.h>
	#include <windows.h>
	#define _Windows
	// @v8.9.8 #define asm __asm
	#define far
	#define _FARFUNC
	#define farcalloc calloc
	#define farmalloc malloc
	#define farrealloc realloc
	#define farfree free

	#define pow10l(x) pow(10,(x))
	#define pow10(x)  pow(10,(x))

	#define delay(n) ::Sleep(n)

	#define MAXDRIVE 3
	#define MAXFILE 257
	#define MAXEXT 5
	#define MAXDIR 257
	#define MAXPATH 261
#endif // _WIN32

// @v5.0.4 AHTOXA {
#ifdef _WIN32_WCE
	#define OEMRESOURCE
	#define _WIN32_WINDOWS 0x0410 // Papyrus @v4.1.12
	#include <winsock2.h>
	#include <windows.h>
	#define _Windows
	#define asm __asm
	#define far
	#define _FARFUNC
	#define farcalloc calloc
	#define farmalloc malloc
	#define farrealloc realloc
	#define farfree free

	#define stricmp         _stricmp
	#define strnicmp        _strnicmp
	#define stpcpy(s1, s2) (strcpy(s1, s2), s1 + strlen(s2))
	#define strcmpi         stricmp
	#define strset          _strset
	#define strupr          _strupr
	#define strlwr          _strlwr
	#define strdup          _strdup
	#define fcvt _fcvt

	#define itoa            _itoa
	#define ltoa            _ltoa
	#define ultoa           _ultoa

	#define pow10l(x) pow(10,(x))
	#define pow10(x) pow(10,(x))

	#define delay(n) ::Sleep(n)

	#define MAXDRIVE 3
	#define MAXFILE 257
	#define MAXEXT 5
	#define MAXDIR 257
	#define MAXPATH 261
#endif
// } @v5.0.4 AHTOXA

#ifdef __WATCOMC__
	#define MAXDRIVE _MAX_DRIVE
	#define MAXPATH  _MAX_PATH
	#define MAXDIR   _MAX_DIR
	#define MAXFILE  _MAX_FNAME
	#define MAXEXT   _MAX_EXT

	#define _argc __argc
	#define _argv __argv
#endif

#define FNF_WILDCARDS 0x01
#define FNF_EXTENSION 0x02
#define FNF_FILENAME  0x04
#define FNF_DIRECTORY 0x08
#define FNF_DRIVE     0x10

#define DMAXEXP     308
#define FMAXEXP     38
#define DMINEXP     -307
#define FMINEXP     -37
#if (__BORLANDC__ < 0x500)
	#define MAXDOUBLE   1.797693E+308
	#define MAXFLOAT    3.37E+38f
	#define MINDOUBLE   2.225074E-308
	#define MINFLOAT    8.43E-37
	// @v8.9.8 #define MAXINT      0x7FFF
#endif

#define DSIGNIF     53
#define FSIGNIF     24

#define DMAXPOWTWO  0x3FF
#define FMAXPOWTWO  0x7F
#define _DEXPLEN    11
#define _FEXPLEN    8
#define _EXPBASE    2
#define _IEEE       1
#define _LENBASE    1
#define HIDDENBIT   1
#define LN_MAXDOUBLE    7.0978E+2
#define LN_MINDOUBLE    -7.0840E+2

#ifdef _MSC_VER
	#define _argc __argc
	#define _argv __argv
	#pragma warning(disable : 4068)
#endif

#ifdef __WIN32__

struct date {
	int    da_year; /* Year - 1980      */
	char   da_day;  /* Day of the month */
	char   da_mon;  /* Month (1 = Jan)  */
};

#ifdef __cplusplus
extern "C" {
#endif
// short  bioscom(int cmd, char abyte, int port);
int    getdisk();
#ifndef  _WIN32_WCE // {
char * stpcpy(char *to, const char *from);
#endif // _WIN32_WCE {
int    fnsplit(const char *, char *, char *, char *, char *);
void   fnmerge(register char *, const char *, const char *, const char *nameP, const char *);
#ifdef __cplusplus
}
#endif

#endif  /* __WIN32__ */

#ifdef __WATCOMC__

#ifdef __cplusplus
extern "C" {
#endif
int    fnsplit(const char *, char *, char *, char *, char *);
void   fnmerge(register char *, const char *, const char *, const char *nameP, const char *);
#ifdef __cplusplus
}
#endif

#endif /* __WATCOMC__ */
