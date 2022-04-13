/** @file
 * @brief MatchDecider subclass for filtering results by value.
 */
/* Copyright 2008 Lemur Consulting Ltd
 * Copyright 2009 Olly Betts
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
	bool ValueSetMatchDecider::operator()(const Xapian::Document& doc) const
	{
		string value = doc.get_value(valuenum);
		set<string>::const_iterator it = testset.find(value);
		if(inclusive)
			return it != testset.end();
		else
			return it == testset.end();
	}
}
