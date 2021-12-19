// cwchar.c
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *   Copyright (C) 2001, International Business Machines Corporation and others.  All Rights Reserved.
 *   encoding:   UTF-8
 *   created on: 2001may25
 *   created by: Markus W. Scherer
 */
#include <icu-internal.h>
#pragma hdrstop

#if !U_HAVE_WCSCPY

#include "cwchar.h"

U_CAPI wchar_t * uprv_wcscat(wchar_t * dst, const wchar_t * src)
{
	wchar_t * start = dst;
	while(*dst!=0) {
		++dst;
	}
	while((*dst = *src)!=0) {
		++dst;
		++src;
	}
	return start;
}

U_CAPI wchar_t * uprv_wcscpy(wchar_t * dst, const wchar_t * src) 
{
	wchar_t * start = dst;
	while((*dst = *src)!=0) {
		++dst;
		++src;
	}
	return start;
}

U_CAPI size_t uprv_wcslen(const wchar_t * src) 
{
	const wchar_t * start = src;
	while(*src!=0) {
		++src;
	}
	return src-start;
}

#endif
