// ustrcase_locale.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2011, International Business Machines Corporation and others.  All Rights Reserved.
// encoding:   UTF-8
// created on: 2011may31
// created by: Markus W. Scherer
// Locale-sensitive case mapping functions (ones that call uloc_getDefault())
// were moved here to break dependency cycles among parts of the common library.
// 
#include <icu-internal.h>
#pragma hdrstop
#include "unicode/casemap.h"
#include "unicode/ucasemap.h"
#include "ucase.h"
#include "ucasemap_imp.h"

U_CFUNC int32_t ustrcase_getCaseLocale(const char * locale) 
{
	SETIFZQ(locale, uloc_getDefault());
	return (*locale == 0) ? UCASE_LOC_ROOT : ucase_getCaseLocale(locale);
}

/* public API functions */

U_CAPI int32_t U_EXPORT2 u_strToLower(char16_t * dest, int32_t destCapacity, const char16_t * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) 
{
	return ustrcase_mapWithOverlap(ustrcase_getCaseLocale(locale), 0, UCASEMAP_BREAK_ITERATOR_NULL dest, destCapacity,
		src, srcLength, ustrcase_internalToLower, *pErrorCode);
}

U_CAPI int32_t U_EXPORT2 u_strToUpper(char16_t * dest, int32_t destCapacity, const char16_t * src, int32_t srcLength, const char * locale, UErrorCode * pErrorCode) 
{
	return ustrcase_mapWithOverlap(ustrcase_getCaseLocale(locale), 0, UCASEMAP_BREAK_ITERATOR_NULL dest, destCapacity, src, srcLength,
		ustrcase_internalToUpper, *pErrorCode);
}

U_NAMESPACE_BEGIN

int32_t CaseMap::toLower(const char * locale, uint32_t options, const char16_t * src, int32_t srcLength, char16_t * dest, int32_t destCapacity, Edits * edits, UErrorCode & errorCode) 
{
	return ustrcase_map(ustrcase_getCaseLocale(locale), options, UCASEMAP_BREAK_ITERATOR_NULL dest, destCapacity, src, srcLength,
		ustrcase_internalToLower, edits, errorCode);
}

int32_t CaseMap::toUpper(const char * locale, uint32_t options, const char16_t * src, int32_t srcLength, char16_t * dest, int32_t destCapacity, Edits * edits, UErrorCode & errorCode) 
{
	return ustrcase_map(ustrcase_getCaseLocale(locale), options, UCASEMAP_BREAK_ITERATOR_NULL dest, destCapacity, src, srcLength,
		ustrcase_internalToUpper, edits, errorCode);
}

U_NAMESPACE_END
