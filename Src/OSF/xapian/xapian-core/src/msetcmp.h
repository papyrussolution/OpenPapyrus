/** @file
 * @brief Result comparison functions.
 */
/* Copyright (C) 2006,2007,2011,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_MSETCMP_H
#define XAPIAN_INCLUDED_MSETCMP_H

class Result;

// typedef for Result comparison function.
typedef bool (* MSetCmp)(const Result&, const Result&);

/// Select the appropriate msetcmp function.
MSetCmp get_msetcmp_function(Xapian::Enquire::Internal::sort_setting sort_by, bool sort_forward, bool sort_value_forward);

#endif // XAPIAN_INCLUDED_MSETCMP_H
