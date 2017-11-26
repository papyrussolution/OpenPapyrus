/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010-2011 Ger Hobbelt All Rights Reserved.
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

#include "xlslib-internal.h"
#pragma hdrstop

static void exception_throwing_assertion_reporter(const char *expr, const char *filename, int lineno, const char *funcname)
{
	str_stream s;
	s << "Assertion failed: ";
	s << (expr ? expr : "???");
	s << " at line ";
	s << lineno;
	if(funcname) {
		s << " (" << funcname << ")";
	}
	if(filename) {
		s << " in " << filename;
	} else {
		s << " in [unidentified source file]";
	}
	throw std::string(s);
}

extern "C"
{
	static void xlslib_default_assertion_reporter(const char *expr, const char *fname, int lineno, const char *funcname)
	{
		exception_throwing_assertion_reporter(expr, fname, lineno, funcname);
	}

	static xlslib_userdef_assertion_reporter * callback = &xlslib_default_assertion_reporter;

	void xlslib_report_failed_assertion(const char *expr, const char *fname, int lineno, const char *funcname)
	{
		if(callback) {
			callback(expr, fname, lineno, funcname);
		}
	}
	void xlslib_register_assert_reporter(xlslib_userdef_assertion_reporter *user_func)
	{
		callback = user_func;
	}
};
