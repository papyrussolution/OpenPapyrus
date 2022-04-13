/** @file
 * @brief Iterate all terms in a remote database.
 */
/* Copyright (C) 2007,2008,2018 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop
#include "remote_alltermslist.h"

using namespace std;

Xapian::termcount RemoteAllTermsList::get_approx_size() const
{
	// RemoteAllTermsList is only used in a TermIterator wrapper and that never
	// calls this method.
	Assert(false);
	return 0;
}

string RemoteAllTermsList::get_termname() const
{
	return current_term;
}

Xapian::doccount RemoteAllTermsList::get_termfreq() const
{
	return current_termfreq;
}

TermList* RemoteAllTermsList::next()
{
	if(!p) {
		p = data.data();
	}
	const char * p_end = data.data() + data.size();
	if(p == p_end) {
		data.resize(0);
		return NULL;
	}
	current_term.resize(size_t(static_cast<uchar>(*p++)));
	if(!unpack_string_append(&p, p_end, current_term) ||
	    !unpack_uint(&p, p_end, &current_termfreq)) {
		unpack_throw_serialisation_error(p);
	}
	return NULL;
}

TermList* RemoteAllTermsList::skip_to(const std::string & term)
{
	if(!p) {
		RemoteAllTermsList::next();
	}
	while(!RemoteAllTermsList::at_end() && current_term < term) {
		RemoteAllTermsList::next();
	}
	return NULL;
}

bool RemoteAllTermsList::at_end() const
{
	return data.empty();
}
