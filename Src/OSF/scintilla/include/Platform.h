// Scintilla source code edit control
/** @file Platform.h
** Interface to platform facilities. Also includes some basic utilities.
** Implemented in PlatGTK.cxx for GTK+/Linux, PlatWin.cxx for Windows, and PlatWX.cxx for wxWindows.
**/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef PLATFORM_H
#define PLATFORM_H

#include <slib.h>
#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#define NO_CXX11_REGEX // @sobolev

// PLAT_GTK = GTK+ on Linux or Win32
// PLAT_GTK_WIN32 is defined additionally when running PLAT_GTK under Win32
// PLAT_WIN = Win32 API on Win32 OS
// PLAT_WX is wxWindows on any supported platform
// PLAT_TK = Tcl/TK on Linux or Win32

#define PLAT_GTK 0
#define PLAT_GTK_WIN32 0
#define PLAT_GTK_MACOSX 0
#define PLAT_MACOSX 0
#define PLAT_WIN 0
#define PLAT_WX  0
#define PLAT_QT 0
#define PLAT_FOX 0
#define PLAT_CURSES 0
#define PLAT_TK 0

#if defined(FOX)
	#undef PLAT_FOX
	#define PLAT_FOX 1
#elif defined(__WX__)
	#undef PLAT_WX
	#define PLAT_WX  1
#elif defined(CURSES)
	#undef PLAT_CURSES
	#define PLAT_CURSES 1
#elif defined(SCINTILLA_QT)
	#undef PLAT_QT
	#define PLAT_QT 1
#elif defined(TK)
	#undef PLAT_TK
	#define PLAT_TK 1
#elif defined(GTK)
	#undef PLAT_GTK
	#define PLAT_GTK 1
	#if defined(__WIN32__) || defined(_MSC_VER)
		#undef PLAT_GTK_WIN32
		#define PLAT_GTK_WIN32 1
	#endif
	#if defined(__APPLE__)
		#undef PLAT_GTK_MACOSX
		#define PLAT_GTK_MACOSX 1
	#endif
#elif defined(__APPLE__)
	#undef PLAT_MACOSX
	#define PLAT_MACOSX 1
#else
	#undef PLAT_WIN
	#define PLAT_WIN 1
#endif

#endif
