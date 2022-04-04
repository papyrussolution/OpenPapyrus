/** @file
 *  @brief Class for wrapping type returned by an input_iterator.
 */
/* Copyright (C) 2004,2008,2009,2013,2014 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_DEREFWRAPPER_H
#define XAPIAN_INCLUDED_DEREFWRAPPER_H

#if !defined XAPIAN_IN_XAPIAN_H && !defined XAPIAN_LIB_BUILD
	#error Never use <xapian/derefwraper.h> directly; include <xapian.h> instead.
#endif

namespace Xapian {
	/** @private @internal Class which returns a value when dereferenced with
	 *  operator*.
	 *
	 *  We need this wrapper class to implement input_iterator semantics for the
	 *  postfix operator++ methods of some of our iterator classes.
	 */
	template <typename T> class DerefWrapper_ {
		void operator = (const DerefWrapper_&) = delete; /// Don't allow assignment.
		T res; /// The value.
	public:
		DerefWrapper_(const DerefWrapper_&) = default; /// Default copy constructor.
		explicit DerefWrapper_(const T &res_) : res(res_) 
		{
		}
		const T & operator*() const { return res; }
	};
}

#endif // XAPIAN_INCLUDED_DEREFWRAPPER_H
