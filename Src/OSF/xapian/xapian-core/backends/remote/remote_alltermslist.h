/** @file
 * @brief Iterate all terms in a remote database.
 */
/* Copyright (C) 2007,2008,2011,2018 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_REMOTE_ALLTERMSLIST_H
#define XAPIAN_INCLUDED_REMOTE_ALLTERMSLIST_H

#include "backends/alltermslist.h"

/// Iterate all terms in a remote database.
class RemoteAllTermsList : public AllTermsList {
	void operator =(const RemoteAllTermsList &) = delete; /// Don't allow assignment.
	RemoteAllTermsList(const RemoteAllTermsList &) = delete; /// Don't allow copying.

	std::string current_term;
	Xapian::doccount current_termfreq;
	std::string data;
	const char * p = NULL;
public:
	RemoteAllTermsList(const std::string & prefix, std::string && data_) : current_term(prefix), data(data_) 
	{
	}
	/// Return approximate size of this termlist.
	Xapian::termcount get_approx_size() const;
	/// Return the termname at the current position.
	std::string get_termname() const;
	/// Return the term frequency for the term at the current position.
	Xapian::doccount get_termfreq() const;
	/// Advance the current position to the next term in the termlist.
	TermList* next();
	/** Skip forward to the specified term.
	 *
	 *  If the specified term isn't in the list, position ourselves on the
	 *  first term after @a term (or at_end() if no terms after @a term exist).
	 */
	TermList* skip_to(const std::string & term);
	/// Return true if the current position is past the last term in this list.
	bool at_end() const;
};

#endif // XAPIAN_INCLUDED_REMOTE_ALLTERMSLIST_H
