/** @file
 * @brief functions to serialise and unserialise a double
 */
/* Copyright (C) 2006,2012 Olly Betts
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#ifndef XAPIAN_INCLUDED_SERIALISE_DOUBLE_H
#define XAPIAN_INCLUDED_SERIALISE_DOUBLE_H

/** Serialise a double to a string.
 *
 *  @param v	The double to serialise.
 *
 *  @return	Serialisation of @a v.
 */
std::string serialise_double(double v);

/** Unserialise a double serialised by serialise_double.
 *
 *  @param p	Pointer to a pointer to the string, which will be advanced past
 *		the serialised double.
 *  @param end	Pointer to the end of the string.
 *
 *  @return	The unserialised double.
 */
double unserialise_double(const char ** p, const char *end);

#endif
