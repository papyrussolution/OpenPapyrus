// cpputils.h
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1997-2011, International Business Machines Corporation and others.  All Rights Reserved.
// @codepage UTF-8
//
#ifndef CPPUTILS_H
#define CPPUTILS_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "cmemory.h"
// 
// Array copy utility functions
// 
static inline void uprv_arrayCopy(const double* src, double* dst, int32_t count)
{
	memcpy(dst, src, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const double* src, int32_t srcStart, double* dst, int32_t dstStart, int32_t count)
{
	memcpy(dst+dstStart, src+srcStart, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int8* src, int8* dst, int32_t count) 
{
	memcpy(dst, src, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const bool * src, bool * dst, int32_t count) // @sobolev (because of UBool removed and replaced with bool)
{
	memcpy(dst, src, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int8* src, int32_t srcStart, int8* dst, int32_t dstStart, int32_t count)
{
	memcpy(dst+dstStart, src+srcStart, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int16* src, int16* dst, int32_t count)
{
	memcpy(dst, src, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int16* src, int32_t srcStart, int16* dst, int32_t dstStart, int32_t count)
{
	memcpy(dst+dstStart, src+srcStart, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int32_t* src, int32_t* dst, int32_t count)
{
	memcpy(dst, src, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const int32_t* src, int32_t srcStart, int32_t* dst, int32_t dstStart, int32_t count)
{
	memcpy(dst+dstStart, src+srcStart, (size_t)count * sizeof(*src));
}

static inline void uprv_arrayCopy(const char16_t * src, int32_t srcStart, char16_t * dst, int32_t dstStart, int32_t count)
{
	memcpy(dst+dstStart, src+srcStart, (size_t)count * sizeof(*src));
}

/**
 * Copy an array of UnicodeString OBJECTS (not pointers).
 * @internal
 */
static inline void uprv_arrayCopy(const icu::UnicodeString * src, icu::UnicodeString * dst, int32_t count)
{
	while(count-- > 0) *dst++ = *src++;
}

/**
 * Copy an array of UnicodeString OBJECTS (not pointers).
 * @internal
 */
static inline void uprv_arrayCopy(const icu::UnicodeString * src, int32_t srcStart, icu::UnicodeString * dst, int32_t dstStart, int32_t count)
{
	uprv_arrayCopy(src+srcStart, dst+dstStart, count);
}

/**
 * Checks that the string is readable and writable.
 * Sets U_ILLEGAL_ARGUMENT_ERROR if the string isBogus() or has an open getBuffer().
 */
inline void uprv_checkCanGetBuffer(const icu::UnicodeString & s, UErrorCode & errorCode) 
{
	if(U_SUCCESS(errorCode) && s.isBogus()) {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
	}
}

#endif /* _CPPUTILS */
