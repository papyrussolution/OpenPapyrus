/** @file
 * @brief Append only vector of Xapian PIMPL objects
 */
/* Copyright (C) 2012,2013,2014,2017 Olly Betts
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
#include <xapian-internal.h>
#pragma hdrstop

void Xapian::SmallVector_::do_reserve(std::size_t n)
{
	// Logic error or size_t wrapping.
	if(UNLIKELY(n <= c))
		throw std::bad_alloc();
	void ** blk = new void* [n];
	if(is_external()) {
		std::copy(static_cast<void **>(p[0]), static_cast<void **>(p[1]), blk);
		p[1] = blk + (static_cast<void**>(p[1]) - static_cast<void**>(p[0]));
		delete [] static_cast<void**>(p[0]);
	}
	else {
		std::copy(p, p + c, blk);
		p[1] = blk + c;
	}
	p[0] = blk;
	c = n;
}

void Xapian::SmallVector_::do_free()
{
	delete [] static_cast<void**>(p[0]);
}
