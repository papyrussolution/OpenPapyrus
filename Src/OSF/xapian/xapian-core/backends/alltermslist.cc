/** @file
 * @brief Abstract base class for iterating all terms in a database.
 */
/* Copyright (C) 2007,2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

Xapian::termcount AllTermsList::get_wdf() const
{
	throw Xapian::InvalidOperationError("AllTermsList::get_wdf() isn't meaningful");
}

Xapian::termcount AllTermsList::positionlist_count() const
{
	throw Xapian::InvalidOperationError("AllTermsList::positionlist_count() isn't meaningful");
}

PositionList* AllTermsList::positionlist_begin() const
{
	throw Xapian::InvalidOperationError("AllTermsList::positionlist_begin() isn't meaningful");
}
