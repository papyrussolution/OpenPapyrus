/** @file
 * @brief Allow rejection of terms during ESet generation.
 */
/* Copyright (C) 2007,2016 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

namespace Xapian {
ExpandDecider::~ExpandDecider() {
}

bool ExpandDeciderAnd::operator()(const string &term) const
{
	return (*first)(term) && (*second)(term);
}

bool ExpandDeciderFilterTerms::operator()(const string &term) const
{
	/* Some older compilers (such as Sun's CC) return an iterator from find()
	 * and a const_iterator from end() in this situation, and then can't
	 * compare the two!  We workaround this problem by explicitly assigning the
	 * result of find() to a const_iterator. */
	set<string>::const_iterator i = rejects.find(term);
	return i == rejects.end();
}

bool ExpandDeciderFilterPrefix::operator()(const string &term) const
{
	return startswith(term, prefix);
}
}
