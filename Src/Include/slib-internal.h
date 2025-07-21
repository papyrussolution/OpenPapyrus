// SLIB-INTERNAL.H
// Copyright (c) A.Sobolev 2020, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#ifndef __SLIB_INTERNAL_H
#define __SLIB_INTERNAL_H
	#define IN_LIBXML
	#define LIBXML_STATIC
	#define ZLIB_INTERNAL
	#define SLIB_INCLUDE_CPPSTDLIBS // @v12.3.9
	#include <slib.h>
	#ifndef SKIP_UED_ID_H // @v12.2.1
		#include <ued-id.h> // @v11.8.9
	#endif
	#include <..\slib\libxml\libxml.h> // @v11.7.9
	#include <slui.h>
	#include <snet.h> // @v11.7.0
	#include <uri.h>  // @v11.7.0

FORCEINLINE uint implement_iseol(const char * pIn, SEOLFormat eolf)
{
	const char c = pIn[0];
	if(oneof2(c, '\x0A', '\x0D')) {
		switch(eolf) {
			case eolAny: // @v11.3.6 Любая комбинация ('\x0D'; '\x0A'; '\x0D\x0A'; '\x0A\x0D') трактуется как конец строки
				return ((c == '\x0D' && pIn[1] == '\x0A') || (c == '\x0A' && pIn[1] == '\x0D')) ? 2 : 1;
			case eolUnix: return BIN(c == '\x0A');
			case eolMac: return BIN(c == '\x0D');
			case eolWindows: return (c == '\x0D' && (pIn[1] == '\x0A')) ? 2 : 0;
			case eolUndef: return (c == '\x0D' && (pIn[1] == '\x0A')) ? 2 : 1;
			case eolSpcICalendar: // @v11.0.3 (если после \xD\xA идет пробел, то это - продолжение текущей строки, но обслуживает ситуацию caller
				return (c == '\x0D' && (pIn[1] == '\x0A')) ? 2 : 0;
		}
	}
	return 0;
}

#endif // __SLIB_INTERNAL_H
