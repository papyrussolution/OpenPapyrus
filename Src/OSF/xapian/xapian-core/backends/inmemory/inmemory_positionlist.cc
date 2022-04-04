/** @file
 * @brief PositionList from an InMemory DB or a Document object
 */
/* Copyright 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

Xapian::termcount InMemoryPositionList::get_approx_size() const
{
	return positions.size();
}

Xapian::termpos InMemoryPositionList::back() const
{
	return positions.back();
}

Xapian::termpos InMemoryPositionList::get_position() const
{
	AssertRel(index, <, positions.size());
	return positions[index];
}

bool InMemoryPositionList::next()
{
	++index;
	AssertRel(index, <=, positions.size());
	return index != positions.size();
}

bool InMemoryPositionList::skip_to(Xapian::termpos termpos)
{
	if(index == size_t(-1))
		index = 0;
	auto begin = positions.begin();
	auto end = positions.end();
	auto it = lower_bound(begin + index, end, termpos);
	index = it - begin;
	return it != end;
}
