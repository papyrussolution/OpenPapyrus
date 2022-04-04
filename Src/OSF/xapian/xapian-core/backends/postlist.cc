/** @file
 * @brief Abstract base class for postlists.
 */
/* Copyright (C) 2007,2009,2011,2015,2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

PostList::~PostList() 
{
}

TermFreqs PostList::get_termfreq_est_using_stats(const Xapian::Weight::Internal &) const
{
	throw Xapian::InvalidOperationError("get_termfreq_est_using_stats() not meaningful for this PostingIterator");
}

Xapian::termcount PostList::get_wdf() const
{
	throw Xapian::InvalidOperationError("get_wdf() not meaningful for this PostingIterator");
}

PositionList * PostList::read_position_list()
{
	throw Xapian::UnimplementedError("OP_NEAR and OP_PHRASE only currently support leaf subqueries");
}

PositionList * PostList::open_position_list() const
{
	throw Xapian::InvalidOperationError("open_position_list() not meaningful for this PostingIterator");
}

PostList * PostList::check(Xapian::docid did, double w_min, bool &valid)
{
	valid = true;
	return skip_to(did, w_min);
}

Xapian::termcount PostList::count_matching_subqs() const
{
	Assert(false);
	return 0;
}

void PostList::gather_position_lists(OrPositionList*)
{
	Assert(false);
}
