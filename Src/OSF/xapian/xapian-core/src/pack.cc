/** @file
 * @brief Pack types into strings and unpack them again.
 */
/* Copyright (C) 2019 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <xapian-internal.h>
#pragma hdrstop

[[noreturn]] void unpack_throw_serialisation_error(const char * p)
{
	const char * m;
	if(p == NULL) {
		m = "Insufficient serialised data";
	}
	else {
		m = "Serialised data overflowed type";
	}
	throw Xapian::SerialisationError(m);
}
