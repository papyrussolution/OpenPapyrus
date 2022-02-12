/** @file version.h
 * @brief Define preprocessor symbols for the library version
 */
// Copyright (C) 2002,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2015,2016,2017,2018 Olly Betts
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef XAPIAN_INCLUDED_VERSION_H
#define XAPIAN_INCLUDED_VERSION_H

#ifdef __GNUC__
#if __GNUC__ < 3 || (__GNUC__ == 3 && __GNUC_MINOR__ == 0)
#error Xapian no longer supports GCC < 3.1
#else
#if !defined(__GXX_ABI_VERSION) || __GXX_ABI_VERSION != 1011
#if defined __GXX_ABI_VERSION && __GXX_ABI_VERSION >= 1002
#warning The C++ ABI version of compiler you are using does not exactly match
#warning that of the compiler used to build the library. If linking fails
#warning due to missing symbols, this is probably the reason why.
#warning The Xapian library was built with g++ 7.3.0
#else
#error The C++ ABI version of compiler you are using does not match
#error that of the compiler used to build the library. The versions
#error must match or your program will not work correctly.
#error The Xapian library was built with g++ 7.3.0
#endif
#endif

#ifdef _GLIBCXX_DEBUG
#error You are compiling with _GLIBCXX_DEBUG defined, but the library
#error was not compiled with this flag. The settings must match or your
#error program will not work correctly.
#endif
#endif
#endif

/// The library was compiled with GCC's -fvisibility=hidden option.
/* #undef XAPIAN_ENABLE_VISIBILITY */

/// The version of Xapian as a C string literal.
#define XAPIAN_VERSION "1.4.19"

/** The major component of the Xapian version.
 * E.g. for Xapian 1.0.14 this would be: 1
 */
#define XAPIAN_MAJOR_VERSION 1

/** The minor component of the Xapian version.
 * E.g. for Xapian 1.0.14 this would be: 0
 */
#define XAPIAN_MINOR_VERSION 4

/** The revision component of the Xapian version.
 * E.g. for Xapian 1.0.14 this would be: 14
 */
#define XAPIAN_REVISION 19

/// Base (signed) type for Xapian::docid and related types.
#define XAPIAN_DOCID_BASE_TYPE long long 

/// Base (signed) type for Xapian::termcount and related types.
#define XAPIAN_TERMCOUNT_BASE_TYPE long long

/// Base (signed) type for Xapian::termpos.
#define XAPIAN_TERMPOS_BASE_TYPE long long // int

/// Type for returning total document length.
#define XAPIAN_TOTALLENGTH_TYPE unsigned long long

/// Underlying type for Xapian::rev.
#define XAPIAN_REVISION_TYPE unsigned long long

/// XAPIAN_HAS_CHERT_BACKEND Defined if the chert backend is enabled.
#define XAPIAN_HAS_CHERT_BACKEND 

/// XAPIAN_HAS_GLASS_BACKEND Defined if the glass backend is enabled.
#define XAPIAN_HAS_GLASS_BACKEND 

/// XAPIAN_HAS_INMEMORY_BACKEND Defined if the inmemory backend is enabled.
#define XAPIAN_HAS_INMEMORY_BACKEND 

/// XAPIAN_HAS_REMOTE_BACKEND Defined if the remote backend is enabled.
#define XAPIAN_HAS_REMOTE_BACKEND 

/// XAPIAN_AT_LEAST(A,B,C) checks for xapian-core >= A.B.C - use like so:
///
/// @code
/// #if XAPIAN_AT_LEAST(1,4,2)
/// /* Code needing features needing Xapian >= 1.4.2. */
/// #endif
/// @endcode
///
/// Added in Xapian 1.4.2.
#define XAPIAN_AT_LEAST(A,B,C) \
 (XAPIAN_MAJOR_VERSION > (A) || \
 (XAPIAN_MAJOR_VERSION == (A) && \
 (XAPIAN_MINOR_VERSION > (B) || \
 (XAPIAN_MINOR_VERSION == (B) && XAPIAN_REVISION >= (C)))))

/// We support move semantics when we're confident the compiler supports it.
///
/// C++11 move semantics are very useful in threaded code that wants to
/// hand-off Xapian objects to worker threads, but in this case it's very
/// unhelpful for availability of these semantics to vary by compiler as it
/// quietly leads to a build with non-threadsafe behaviour.
///
/// User code can #define XAPIAN_MOVE_SEMANTICS to force this on, and will
/// then get a compilation failure if the compiler lacks suitable support.
#ifndef XAPIAN_MOVE_SEMANTICS
#if __cplusplus >= 201103L || \
 (defined _MSC_VER && _MSC_VER >= 1900) || \
 defined XAPIAN_LIB_BUILD
#define XAPIAN_MOVE_SEMANTICS
#endif
#endif

#endif /* XAPIAN_INCLUDED_VERSION_H */
