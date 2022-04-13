/** @file
 * @brief Iteration over terms in a document
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

Xapian::termcount DocumentTermList::get_approx_size() const
{
	// DocumentTermList is only used in a TermIterator wrapper and that never
	// calls this method.
	Assert(false);
	return 0;
}

string DocumentTermList::get_termname() const
{
	Assert(!at_end());
	return it->first;
}

Xapian::termcount DocumentTermList::get_wdf() const
{
	Assert(!at_end());
	return it->second.get_wdf();
}

Xapian::doccount DocumentTermList::get_termfreq() const
{
	throw Xapian::InvalidOperationError("get_termfreq() not valid for a TermIterator from a Document which is not associated with a database");
}

const Xapian::VecCOW<Xapian::termpos>* DocumentTermList::get_vec_termpos() const
{
	return it->second.get_positions();
}

PositionList* DocumentTermList::positionlist_begin() const
{
	return new InMemoryPositionList(*it->second.get_positions());
}

Xapian::termcount DocumentTermList::positionlist_count() const
{
	return it->second.count_positions();
}

TermList* DocumentTermList::next()
{
	if(it == doc->terms->end()) {
		it = doc->terms->begin();
	}
	else {
		++it;
	}
	while(it != doc->terms->end() && it->second.is_deleted()) {
		++it;
	}
	return NULL;
}

TermList* DocumentTermList::skip_to(const string & term)
{
	it = doc->terms->lower_bound(term);
	while(it != doc->terms->end() && it->second.is_deleted()) {
		++it;
	}
	return NULL;
}

bool DocumentTermList::at_end() const
{
	return it == doc->terms->end();
}
