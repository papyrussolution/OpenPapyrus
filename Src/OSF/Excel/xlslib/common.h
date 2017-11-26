/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2013 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef XLSLIB_COMMON_H
#define XLSLIB_COMMON_H

#if defined(HAVE_WORKING_ICONV)
#include <iconv.h>

/* part of fix for PR #3039001 */
#ifndef __GLIBC__
#define UCS_2_INTERNAL "UCS-2-INTERNAL"
#else
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ <= 1)
#error "Too old glibc. This version's iconv() implementation cannot be trusted."
#endif
#define UCS_2_INTERNAL "UCS-2"
#endif
#endif
/* HAVE_WORKING_ICONV */

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	// Some typedefs used only by xlslib core
	class range_t {
	public:
		range_t() : first_row(0), last_row(0), first_col(0), last_col(0) { }
		virtual ~range_t() { }

	public:
		uint32 first_row;
		uint32 last_row;
		uint32 first_col;
		uint32 last_col;
	};

	typedef std::vector<xlslib_core::range_t* XLSLIB_DFLT_ALLOCATOR> Range_Vect_t;
	typedef Range_Vect_t::iterator Range_Vect_Itor_t;

	class range;
	typedef std::vector<xlslib_core::range* XLSLIB_DFLT_ALLOCATOR> RangeObj_Vect_t;
	typedef RangeObj_Vect_t::iterator RangeObj_Vect_Itor_t;
}

// #include "xls_poppack.h"

#endif
