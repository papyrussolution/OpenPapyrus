/** @file
 * @brief std::ostream operator<< template for Xapian objects
 */
/* Copyright (C) 2016 Olly Betts
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
#ifndef XAPIAN_INCLUDED_COMMON_OUTPUT_H
#define XAPIAN_INCLUDED_COMMON_OUTPUT_H

#include <ostream>

// The decltype() of a comma expression is a trick to leverage SFINAE to
// provide this template for classes with a "get_description() const" method,
// while providing the expected return type for the function.
template<class T> auto operator<<(std::ostream & os, const T& t) -> decltype(t.get_description(), os)
{
    return os << t.get_description();
}

#endif
