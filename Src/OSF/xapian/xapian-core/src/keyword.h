/** @file
 * @brief Efficient keyword to enum lookup
 */
/* Copyright (C) 2012,2016 Olly Betts
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

#ifndef XAPIAN_INCLUDED_KEYWORD_H
#define XAPIAN_INCLUDED_KEYWORD_H

#include <string.h>

int keyword(const uchar * tab, const char * s, size_t len);

/// 2 byte offset variant.
int keyword2(const uchar * tab, const char * s, size_t len);

#endif
