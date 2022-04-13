/** @file
 * @brief Helper functions for omassert.h
 */
/* Copyright (C) 2007,2012 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

#ifdef XAPIAN_ASSERTIONS

using namespace std;

namespace Xapian {
	namespace Internal {
		bool within_DBL_EPSILON(double a, double b) { return fabs(a - b) < DBL_EPSILON; }
	}
}

#endif
