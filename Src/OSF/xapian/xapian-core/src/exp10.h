/** @file
 * @brief Define exp10() if not provided by <cmath>
 */
/* Copyright (C) 2017 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_EXP10_H
#define XAPIAN_INCLUDED_EXP10_H

#ifndef PACKAGE
#error config.h must be included first in each C++ source file
#endif

#include <cmath>
#if !HAVE_DECL_EXP10
#if HAVE_DECL___EXP10
inline double exp10(double x) { return __exp10(x); }
# elif defined HAVE___BUILTIN_EXP10
inline double exp10(double x) { return __builtin_exp10(x); }
# else
inline double exp10(double x) { return std::pow(10.0, x); }
#endif
#endif

#endif // XAPIAN_INCLUDED_EXP10_H
