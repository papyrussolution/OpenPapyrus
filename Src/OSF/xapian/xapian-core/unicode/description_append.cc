/** @file
 *  @brief Append a string to an object description, escaping invalid UTF-8
 */
// Copyright (C) 2013 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop
#include "description_append.h"

using namespace std;

void description_append(std::string & desc, const std::string & s)
{
	desc.reserve(desc.size() + s.size());
	for(Xapian::Utf8Iterator i(s); i != Xapian::Utf8Iterator(); ++i) {
		uint ch = i.strict_deref();
		if((ch & 0x80000000) == 0 && ch >= ' ' && ch != '\\' && ch != 127) {
			Xapian::Unicode::append_utf8(desc, ch);
		}
		else {
			char buf[8];
			sprintf(buf, "\\x%02x", ch & 0xff);
			desc += buf;
		}
	}
}
