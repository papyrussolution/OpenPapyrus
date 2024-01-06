/** @file
 * @brief Set of documents judged as relevant
 */
/* Copyright (C) 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
RSet::RSet(const RSet&) = default;

RSet& RSet::operator = (const RSet&) = default;
RSet::RSet(RSet &&) = default;
RSet& RSet::operator = (RSet &&) = default;

RSet::RSet() 
{
}

RSet::RSet(Internal* internal_) : internal(internal_) 
{
}

RSet::~RSet() 
{
}

Xapian::doccount RSet::size() const
{
	return internal.get() ? internal->docs.size() : 0;
}

void RSet::add_document(Xapian::docid did)
{
	if(UNLIKELY(did == 0))
		throw Xapian::InvalidArgumentError("Docid 0 not valid in an RSet");
	if(!internal.get())
		internal = new RSet::Internal;
	internal->docs.insert(did);
}

void RSet::remove_document(Xapian::docid did)
{
	if(UNLIKELY(did == 0))
		throw Xapian::InvalidArgumentError("Docid 0 not valid in an RSet");
	if(internal.get())
		internal->docs.erase(did);
}

bool RSet::contains(Xapian::docid did) const
{
	return internal.get() && internal->docs.find(did) != internal->docs.end();
}

string RSet::get_description() const
{
	string desc = "RSet(";
	if(!internal.get() || internal->docs.empty()) {
		desc += ')';
	}
	else {
		for(auto && did : internal->docs) {
			desc += str(did);
			desc += ',';
		}
		desc.back() = ')';
	}
	return desc;
}
}
