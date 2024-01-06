/** @file
 * @brief Count the number of set bits in an integer type
 */
/* Copyright (C) 2014-2020 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef XAPIAN_INCLUDED_POPCOUNT_H
#define XAPIAN_INCLUDED_POPCOUNT_H

#ifndef PACKAGE
	#error config.h must be included first in each C++ source file
#endif

#if !HAVE_DECL___BUILTIN_POPCOUNT
// Only include <intrin.h> if we have to as it can result in warnings about
// duplicate declarations of builtin functions under mingw.
#if HAVE_DECL___POPCNT || HAVE_DECL___POPCNT64
#include <intrin.h>
#endif
#endif
/* @sobolev (replaced with SBits::Cpop) 
/// Add the number of set bits in value to accumulator.
template <typename A, typename V> static inline void add_popcount(A& accumulator, V value)
{
	if(false) {
#if HAVE_DECL___BUILTIN_POPCOUNT
	}
	else if(sizeof(V) == sizeof(unsigned)) {
		accumulator += __builtin_popcount(value);
#elif HAVE_DECL___POPCNT
	}
	else if(sizeof(V) == sizeof(unsigned)) {
		accumulator += static_cast<A>(__popcnt(value));
#endif
#if HAVE_DECL___BUILTIN_POPCOUNTL
	}
	else if(sizeof(V) == sizeof(ulong)) {
		accumulator += __builtin_popcountl(value);
#endif
#if HAVE_DECL___BUILTIN_POPCOUNTLL
	}
	else if(sizeof(V) == sizeof(ulong long)) {
		accumulator += __builtin_popcountll(value);
#elif HAVE_DECL___POPCNT64
	}
	else if(sizeof(V) == sizeof(ulong long)) {
		accumulator += static_cast<A>(__popcnt64(value));
#endif
	}
	else {
		while(value) {
			++accumulator;
			value &= value - 1;
		}
	}
}*/

/// Count the number of set bits in value.
/* @sobolev (replaced with SBits::Cpop) template <typename V> static unsigned popcount(V value)
{
	unsigned accumulator = 0;
	add_popcount(accumulator, value);
	return accumulator;
}*/

#endif // XAPIAN_INCLUDED_POPCOUNT_H
