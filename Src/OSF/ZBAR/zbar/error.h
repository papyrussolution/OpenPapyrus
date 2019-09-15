/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/
#ifndef _ERROR_H_
#define _ERROR_H_

//#include <config.h>
#include <slib.h>
#include <zbar.h>
#ifdef HAVE_INTTYPES_H
	#include <inttypes.h>
#endif
#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif
#ifdef _WIN32
	#include <windows.h>
#endif
#if __STDC_VERSION__ < 199901L
	#if __GNUC__ >= 2
		#define __func__ __FUNCTION__
	#else
		#define __func__ "<unknown>"
	#endif
#endif

extern int _zbar_verbosity;

// FIXME don't we need varargs hacks here? 

#ifdef _WIN32
	#define ZFLUSH fflush(stderr);
#else
	#define ZFLUSH
#endif

#if 0 // @sobolev {
	#ifdef ZNO_MESSAGES
		#ifdef __GNUC__
			/* older versions of gcc (< 2.95) require a named varargs parameter */
			#define zprintf(args ...)
		#else
			/* unfortunately named vararg parameter is a gcc-specific extension */
			#define zprintf(...)
		#endif
	#else
		#ifdef __GNUC__
			#define zprintf(level, format, args ...) do {				 \
					if(_zbar_verbosity >= level) {					\
						fprintf(stderr, "%s: " format, __func__, ## args);	    \
						ZFLUSH							    \
					}								\
			} while(0)
		#else
			#define zprintf(level, format, ...) do {				\
					if(_zbar_verbosity >= level) {					\
						fprintf(stderr, "%s: " format, __func__, ## __VA_ARGS__);   \
						ZFLUSH							    \
					}								\
			} while(0)
		#endif
	#endif
#endif // } 0 @sobolev

void cdecl zprintf(int level, const char * pFormat, ...);
int err_copy(void * dst_c, void * src_c);
int err_capture(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail);
int err_capture_str(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, const char * arg);
int err_capture_int(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int arg);
int err_capture_num(const void * container, errsev_t sev, zbar_error_t type, const char * func, const char * detail, int num);
void err_init(errinfo_t * err, errmodule_t module);
void err_cleanup(errinfo_t * err);

#endif
