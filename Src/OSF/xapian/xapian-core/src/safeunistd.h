/** @file
 * @brief <unistd.h>, but with compat. and large file support for MSVC.
 */
/* Copyright (C) 2007,2015 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */
#ifndef XAPIAN_INCLUDED_SAFEUNISTD_H
#define XAPIAN_INCLUDED_SAFEUNISTD_H

#ifndef PACKAGE
	#error config.h must be included first in each C++ source file
#endif
#ifndef _MSC_VER
	#include <unistd.h>
#else
	#include <io.h> // io.h is the nearest equivalent to unistd.h.
	#include <process.h> // process.h is needed for getpid().
	#include <direct.h> // direct.h is needed for rmdir().
#endif

// Under mingw we probably don't need to provide our own sleep().
#if defined __WIN32__ && !defined HAVE_SLEEP

inline uint sleep(uint seconds)
{
	// Use our own little helper function to avoid pulling in <windows.h>.
	// @v11.9.1 (replaced with SDelay) extern void xapian_sleep_milliseconds(uint millisecs);

	// Sleep takes a time interval in milliseconds, whereas POSIX sleep takes
	// a time interval in seconds, so we need to multiply 'seconds' by 1000.
	//
	// But make sure the multiplication won't overflow!  4294967 seconds is
	// nearly 50 days, so just sleep for that long and return the number of
	// seconds left to sleep for.  The common case of sleep(CONSTANT) should
	// optimise to just xapian_sleep_milliseconds(CONSTANT).
	if(seconds > 4294967u) {
		/*xapian_sleep_milliseconds*/SDelay(4294967000u);
		return seconds - 4294967u;
	}
	/*xapian_sleep_milliseconds*/SDelay(seconds * 1000u);
	return 0;
}

#endif
#endif /* XAPIAN_INCLUDED_SAFEUNISTD_H */
