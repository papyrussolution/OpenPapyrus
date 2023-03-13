/* $Id: libport.h,v 1.5 2015-08-19 02:31:04 bfriesen Exp $ */
/*
 * Copyright (c) 2009 Frank Warmerdam
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 */
#ifndef _LIBPORT_
#define _LIBPORT_

/* @v10.9.12 include\getopt.h
int getopt(int argc, char * const argv[], const char * optstring);
extern char * optarg;
extern int opterr;
extern int optind;
extern int optopt;
*/
int strcasecmp(const char * s1, const char * s2);

#ifndef HAVE_GETOPT
	#define HAVE_GETOPT 1
#endif
#if 0
	ulong strtoul(const char * nptr, char ** endptr, int base);
#endif
#if 0
	void * lfind(const void * key, const void * base, size_t * nmemb, size_t size, int (* compar)(const void *, const void *));
#endif
#if !defined(HAVE_SNPRINTF)
	//#undef vsnprintf
	//#define vsnprintf _TIFF_vsnprintf_f

	//#undef snprintf
	//#define snprintf _TIFF_snprintf_f
	//int snprintf(char * str, size_t size, const char * format, ...);
#endif

#endif /* ndef _LIBPORT_ */
