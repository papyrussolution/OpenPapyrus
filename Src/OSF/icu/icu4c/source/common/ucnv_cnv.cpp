// uconv_cnv.c:
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1999-2004, International Business Machines Corporation and others.  All Rights Reserved.
// Implements all the low level conversion functions
// T_UnicodeConverter_{to,from}Unicode_$ConversionType
// Change history:
// 06/29/2000  helena      Major rewrite of the callback APIs.
// 
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ucnv_cnv.h"
#include "ucnv_bld.h"

U_CFUNC void ucnv_getCompleteUnicodeSet(const UConverter * cnv, const USetAdder * sa, UConverterUnicodeSet which, UErrorCode * pErrorCode) 
{
	(void)cnv;
	(void)which;
	(void)pErrorCode;
	sa->addRange(sa->set, 0, 0x10ffff);
}

U_CFUNC void ucnv_getNonSurrogateUnicodeSet(const UConverter * cnv, const USetAdder * sa, UConverterUnicodeSet which, UErrorCode * pErrorCode) 
{
	(void)cnv;
	(void)which;
	(void)pErrorCode;
	sa->addRange(sa->set, 0, 0xd7ff);
	sa->addRange(sa->set, 0xe000, 0x10ffff);
}

U_CFUNC void ucnv_fromUWriteBytes(UConverter * cnv,
    const char * bytes, int32_t length,
    char ** target, const char * targetLimit,
    int32_t ** offsets,
    int32_t sourceIndex,
    UErrorCode * pErrorCode) {
	char * t = *target;
	int32_t * o;

	/* write bytes */
	if(offsets==NULL || (o = *offsets)==NULL) {
		while(length>0 && t<targetLimit) {
			*t++ = *bytes++;
			--length;
		}
	}
	else {
		/* output with offsets */
		while(length>0 && t<targetLimit) {
			*t++ = *bytes++;
			*o++ = sourceIndex;
			--length;
		}
		*offsets = o;
	}
	*target = t;
	/* write overflow */
	if(length > 0) {
		if(cnv) {
			t = (char *)cnv->charErrorBuffer;
			cnv->charErrorBufferLength = (int8)length;
			do {
				*t++ = (uint8)*bytes++;
			} while(--length>0);
		}
		*pErrorCode = U_BUFFER_OVERFLOW_ERROR;
	}
}

U_CFUNC void ucnv_toUWriteUChars(UConverter * cnv, const char16_t * uchars, int32_t length, char16_t ** target, const char16_t * targetLimit,
    int32_t ** offsets, int32_t sourceIndex, UErrorCode * pErrorCode) 
{
	char16_t * t = *target;
	int32_t * o;
	/* write UChars */
	if(offsets==NULL || (o = *offsets)==NULL) {
		while(length>0 && t<targetLimit) {
			*t++ = *uchars++;
			--length;
		}
	}
	else {
		/* output with offsets */
		while(length>0 && t<targetLimit) {
			*t++ = *uchars++;
			*o++ = sourceIndex;
			--length;
		}
		*offsets = o;
	}
	*target = t;
	/* write overflow */
	if(length > 0) {
		if(cnv) {
			t = cnv->UCharErrorBuffer;
			cnv->UCharErrorBufferLength = (int8)length;
			do {
				*t++ = *uchars++;
			} while(--length>0);
		}
		*pErrorCode = U_BUFFER_OVERFLOW_ERROR;
	}
}

U_CFUNC void ucnv_toUWriteCodePoint(UConverter * cnv, UChar32 c, char16_t ** target, const char16_t * targetLimit,
    int32_t ** offsets, int32_t sourceIndex, UErrorCode * pErrorCode) 
{
	int32_t * o;
	char16_t * t = *target;
	if(t<targetLimit) {
		if(c<=0xffff) {
			*t++ = (char16_t)c;
			c = U_SENTINEL;
		}
		else { /* c is a supplementary code point */
			*t++ = U16_LEAD(c);
			c = U16_TRAIL(c);
			if(t<targetLimit) {
				*t++ = (char16_t)c;
				c = U_SENTINEL;
			}
		}
		/* write offsets */
		if(offsets && (o = *offsets) != NULL) {
			*o++ = sourceIndex;
			if((*target+1)<t) {
				*o++ = sourceIndex;
			}
			*offsets = o;
		}
	}
	*target = t;
	/* write overflow from c */
	if(c>=0) {
		if(cnv) {
			int8 i = 0;
			U16_APPEND_UNSAFE(cnv->UCharErrorBuffer, i, c);
			cnv->UCharErrorBufferLength = i;
		}
		*pErrorCode = U_BUFFER_OVERFLOW_ERROR;
	}
}

#endif
