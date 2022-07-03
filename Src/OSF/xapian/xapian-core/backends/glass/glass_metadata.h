/** @file
 * @brief Access to metadata for a glass database.
 */
/* Copyright (C) 2005,2007,2008,2009,2011,2017 Olly Betts
 * Copyright (C) 2008 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_GLASS_METADATA_H
#define XAPIAN_INCLUDED_GLASS_METADATA_H

class GlassCursor;

class GlassMetadataTermList : public AllTermsList {
	GlassMetadataTermList(const GlassMetadataTermList &); /// Copying is not allowed.
	void operator =(const GlassMetadataTermList &); /// Assignment is not allowed.
	Xapian::Internal::intrusive_ptr<const Xapian::Database::Internal> database; /// Keep a reference to our database to stop it being deleted.
	GlassCursor * cursor; /// A cursor which runs through the postlist table reading metadata keys.
	std::string prefix; /// The prefix that all returned keys must have.
public:
	GlassMetadataTermList(Xapian::Internal::intrusive_ptr<const Xapian::Database::Internal> database_, GlassCursor * cursor_, const std::string &prefix_);
	~GlassMetadataTermList();
	Xapian::termcount get_approx_size() const;
	/** Returns the current termname.
	 *
	 *  Either next() or skip_to() must have been called before this
	 *  method can be called.
	 */
	std::string get_termname() const;
	/** Return the term frequency for the term at the current position.
	 *
	 *  Not meaningful for a GlassMetadataTermList.
	 */
	Xapian::doccount get_termfreq() const;
	TermList * next(); /// Advance to the next term in the list.
	TermList * skip_to(const std::string &key); /// Advance to the first key which is >= @a key.
	bool at_end() const; /// True if we're off the end of the list
};

#endif // XAPIAN_INCLUDED_GLASS_METADATA_H
