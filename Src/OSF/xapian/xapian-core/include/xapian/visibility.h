/** @file
 * @brief Define XAPIAN_VISIBILITY_* macros.
 */
// Copyright (C) 2007,2010,2014,2017,2019 Olly Betts
// @licence GNU GPL
//
#ifndef XAPIAN_INCLUDED_VISIBILITY_H
	#define XAPIAN_INCLUDED_VISIBILITY_H
	// See https://gcc.gnu.org/wiki/Visibility for more information about GCC's symbol visibility support.
	#include "xapian/version.h"
	#ifdef XAPIAN_ENABLE_VISIBILITY
		#define XAPIAN_VISIBILITY_DEFAULT __attribute__((visibility("default")))
		#define XAPIAN_VISIBILITY_INTERNAL __attribute__((visibility("internal")))
	#else
		#define XAPIAN_VISIBILITY_DEFAULT
		#define XAPIAN_VISIBILITY_INTERNAL
	#endif
#endif
