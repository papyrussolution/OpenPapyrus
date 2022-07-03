/** @file
 * @brief A PostList which iterates over all documents in a GlassDatabase.
 */
/* Copyright (C) 2006,2007,2008,2009 Olly Betts
 * Copyright (C) 2008 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;
using Xapian::Internal::intrusive_ptr;

GlassAllDocsPostList::GlassAllDocsPostList(intrusive_ptr<const GlassDatabase> db_, Xapian::doccount doccount_) : 
	GlassPostList(db_, string(), true), doccount(doccount_)
{
	LOGCALL_CTOR(DB, "GlassAllDocsPostList", db_.get() | doccount_);
}

Xapian::doccount GlassAllDocsPostList::get_termfreq() const
{
	LOGCALL(DB, Xapian::doccount, "GlassAllDocsPostList::get_termfreq", NO_ARGS);
	RETURN(doccount);
}

Xapian::termcount GlassAllDocsPostList::get_wdf() const
{
	LOGCALL(DB, Xapian::termcount, "GlassAllDocsPostList::get_wdf", NO_ARGS);
	AssertParanoid(!at_end());
	RETURN(1);
}

PositionList * GlassAllDocsPostList::read_position_list()
{
	LOGCALL(DB, PositionList *, "GlassAllDocsPostList::read_position_list", NO_ARGS);
	throw Xapian::InvalidOperationError("GlassAllDocsPostList::read_position_list() not meaningful");
}

PositionList * GlassAllDocsPostList::open_position_list() const
{
	LOGCALL(DB, PositionList *, "GlassAllDocsPostList::open_position_list", NO_ARGS);
	throw Xapian::InvalidOperationError("GlassAllDocsPostList::open_position_list() not meaningful");
}

string GlassAllDocsPostList::get_description() const
{
	string desc = "GlassAllDocsPostList(doccount=";
	desc += str(doccount);
	desc += ')';
	return desc;
}
