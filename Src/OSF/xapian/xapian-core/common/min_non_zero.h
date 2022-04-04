/** @file
 * @brief Return the smaller of two numbers which isn't zero.
 */
/* Copyright (C) 2018 Olly Betts
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
#ifndef XAPIAN_INCLUDED_MIN_NON_ZERO_H
#define XAPIAN_INCLUDED_MIN_NON_ZERO_H

/** Return the smaller of two unsigned integers which isn't zero.
 *
 *  If both a and b are zero, returns zero.
 */
template <typename T> constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type min_non_zero(const T& a, const T& b)
{
	// To achieve the effect we want, we find the *maximum* after negating each
	// of the values (which for an unsigned type leaves 0 alone but flips the
	// order of all other values), then negate the answer.
	return -std::max(-a, -b);
}

#endif // XAPIAN_INCLUDED_MIN_NON_ZERO_H
