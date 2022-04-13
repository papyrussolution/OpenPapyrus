/** @file
 *  @brief Postlists for remote databases
 */
/* Copyright (C) 2007 Lemur Consulting Ltd
 * Copyright (C) 2007,2008,2009,2011,2012,2013,2015,2019 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop
#include "net_postlist.h"

using namespace std;

Xapian::doccount NetworkPostList::get_termfreq() const
{
	return termfreq;
}

Xapian::docid NetworkPostList::get_docid() const
{
	return lastdocid;
}

Xapian::termcount NetworkPostList::get_wdf() const
{
	return lastwdf;
}

PositionList * NetworkPostList::open_position_list() const
{
	return db->open_position_list(lastdocid, term);
}

PostList * NetworkPostList::next(double)
{
	if(!started) {
		started = true;
		pos = postings.data();
		pos_end = pos + postings.size();
		lastdocid = 0;
	}

	if(pos == pos_end) {
		pos = NULL;
	}
	else {
		Xapian::docid inc;
		if(!unpack_uint(&pos, pos_end, &inc) ||
		    !unpack_uint(&pos, pos_end, &lastwdf)) {
			unpack_throw_serialisation_error(pos);
		}
		lastdocid += inc + 1;
	}

	return NULL;
}

PostList * NetworkPostList::skip_to(Xapian::docid did, double min_weight)
{
	if(!started)
		next(min_weight);
	while(pos && lastdocid < did)
		next(min_weight);
	return NULL;
}

bool NetworkPostList::at_end() const
{
	return (pos == NULL && started);
}

Xapian::termcount NetworkPostList::get_wdf_upper_bound() const
{
	// This is only called when setting weights on PostList objects before
	// a match, which shouldn't happen to NetworkPostList objects (as remote
	// matching happens like a local match on the server).
	Assert(false);
	return 0;
}

string NetworkPostList::get_description() const
{
	string desc = "NetworkPostList(";
	description_append(desc, term);
	desc += ')';
	return desc;
}
