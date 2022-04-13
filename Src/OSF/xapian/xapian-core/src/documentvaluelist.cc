/** @file
 * @brief Iteration over values in a document.
 */
/* Copyright (C) 2008,2009,2011,2013 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

Xapian::docid DocumentValueList::get_docid() const
{
	throw Xapian::InvalidOperationError("get_docid() isn't valid when iterating over values in a document");
}

Xapian::valueno DocumentValueList::get_valueno() const
{
	Assert(!at_end());
	return it->first;
}

string DocumentValueList::get_value() const
{
	Assert(!at_end());
	return it->second;
}

bool DocumentValueList::at_end() const
{
	return it == doc->values->end();
}

void DocumentValueList::next()
{
	if(it == doc->values->end()) {
		it = doc->values->begin();
	}
	else {
		++it;
	}
}

void DocumentValueList::skip_to(Xapian::docid slot)
{
	it = doc->values->lower_bound(slot);
}

string DocumentValueList::get_description() const
{
	string desc = "DocumentValueList(";
	if(!at_end()) {
		desc += "slot=";
		desc += str(get_valueno());
		desc += ", value=\"";
		description_append(desc, get_value());
		desc += "\")";
	}
	else {
		desc += "atend)";
	}
	return desc;
}
