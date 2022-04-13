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
#ifndef XAPIAN_INCLUDED_GLASS_ALLDOCSPOSTLIST_H
#define XAPIAN_INCLUDED_GLASS_ALLDOCSPOSTLIST_H

#include <string>
#include "glass_postlist.h"

class GlassAllDocsPostList : public GlassPostList {
	void operator =(const GlassAllDocsPostList &); /// Don't allow assignment.
	GlassAllDocsPostList(const GlassAllDocsPostList &); /// Don't allow copying.
	Xapian::doccount doccount; /// The number of documents in the database.
public:
	GlassAllDocsPostList(Xapian::Internal::intrusive_ptr<const GlassDatabase> db_, Xapian::doccount doccount_);
	Xapian::doccount get_termfreq() const;
	Xapian::termcount get_wdf() const;
	PositionList * read_position_list();
	PositionList * open_position_list() const;
	std::string get_description() const;
};

#endif // XAPIAN_INCLUDED_GLASS_ALLDOCSPOSTLIST_H
