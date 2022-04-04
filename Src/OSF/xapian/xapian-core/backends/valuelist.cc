/** @file
 * @brief Abstract base class for value streams.
 */
/* Copyright (C) 2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

namespace Xapian {
	ValueIterator::Internal::~Internal() 
	{
	}
	bool ValueIterator::Internal::check(Xapian::docid did)
	{
		skip_to(did);
		return true;
	}
}
