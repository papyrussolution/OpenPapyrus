/** @file
 * @brief functions to convert Xapian::Error objects to strings and back
 */
// Copyright (C) 2006,2007,2008,2009,2010,2011,2014,2015,2016 Olly Betts
// @license GNU GPL
//
#include <xapian-internal.h>
#pragma hdrstop

using namespace std;

string serialise_error(const Xapian::Error &e)
{
	// The byte before the type name is the type code.
	string result(1, (e.get_type())[-1]);
	pack_string(result, e.get_context());
	pack_string(result, e.get_msg());
	// The "error string" goes last so we don't need to store its length.
	const char * err = e.get_error_string();
	if(err) result += err;
	return result;
}

void unserialise_error(const string &serialised_error, const string &prefix, const string &new_context)
{
	// Use c_str() so last string is nul-terminated.
	const char * p = serialised_error.c_str();
	const char * end = p + serialised_error.size();
	if(p != end) {
		char type = *p++;
		string context;
		string msg(prefix);
		if(!unpack_string(&p, end, context) || !unpack_string_append(&p, end, msg)) {
			unpack_throw_serialisation_error(p);
		}
		const char * error_string = (p == end) ? NULL : p;
		if(!new_context.empty()) {
			if(!context.empty()) {
				msg += "; context was: ";
				msg += context;
			}
			context = new_context;
		}
		switch(type) {
#include "xapian/errordispatch.h"
		}
	}
	throw Xapian::InternalError("Unknown remote exception type", new_context);
}
