// imgui-slib-internal.h
//
#ifndef __IMGUI_SLIB_INTERNAL_H
#define __IMGUI_SLIB_INTERNAL_H

#include <slib.h>
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
	#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef IMGUI_DEFINE_MATH_OPERATORS
	#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
	#include "misc/freetype/imgui_freetype.h"
#endif
//#include <stdio.h>      // vsnprintf, sscanf, printf

#endif // __IMGUI_SLIB_INTERNAL_H

