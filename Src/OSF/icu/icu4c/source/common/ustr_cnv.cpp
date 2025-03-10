// ustr_cnv.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1998-2014, International Business Machines Corporation and others.  All Rights Reserved.
// encoding:   UTF-8
// created on: 2004aug24
// created by: Markus W. Scherer
// 
// Character conversion functions moved here from ustring.c
// 
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ustr_cnv.h"
#include "ucnv_bld.h"

/* mutexed access to a shared default converter ----------------------------- */

static UConverter * gDefaultConverter = NULL;

U_CAPI UConverter * U_EXPORT2 u_getDefaultConverter(UErrorCode * status)
{
	UConverter * converter = NULL;
	if(gDefaultConverter) {
		icu::umtx_lock(NULL);
		/* need to check to make sure it wasn't taken out from under us */
		if(gDefaultConverter) {
			converter = gDefaultConverter;
			gDefaultConverter = NULL;
		}
		icu::umtx_unlock(NULL);
	}
	/* if the cache was empty, create a converter */
	if(!converter) {
		converter = ucnv_open(NULL, status);
		if(U_FAILURE(*status)) {
			ucnv_close(converter);
			converter = NULL;
		}
	}
	return converter;
}

U_CAPI void U_EXPORT2 u_releaseDefaultConverter(UConverter * converter)
{
	if(gDefaultConverter == NULL) {
		if(converter)
			ucnv_reset(converter);
		ucnv_enableCleanup();
		icu::umtx_lock(NULL);
		if(gDefaultConverter == NULL) {
			gDefaultConverter = converter;
			converter = NULL;
		}
		icu::umtx_unlock(NULL);
	}
	if(converter)
		ucnv_close(converter);
}

U_CAPI void U_EXPORT2 u_flushDefaultConverter()
{
	UConverter * converter = NULL;
	if(gDefaultConverter != NULL) {
		icu::umtx_lock(NULL);
		/* need to check to make sure it wasn't taken out from under us */
		if(gDefaultConverter != NULL) {
			converter = gDefaultConverter;
			gDefaultConverter = NULL;
		}
		icu::umtx_unlock(NULL);
	}
	// if the cache was populated, flush it
	ucnv_close(converter);
}

/* conversions between char * and char16_t * ------------------------------------- */

#define MAX_STRLEN 0x0FFFFFFF // maximum string length for u_uastrcpy() and u_austrcpy() implementations
/*
   returns the minimum of (the length of the null-terminated string) and n.
 */
/* @v11.9.12 (replaced with sstrnlen) static int32_t u_astrnlen(const char * s1, int32_t n)
{
	int32_t len = 0;
	if(s1) {
		while(n-- && *(s1++)) {
			len++;
		}
	}
	return len;
}*/

U_CAPI char16_t * U_EXPORT2 u_uastrncpy(char16_t * ucs1, const char * s2, int32_t n)
{
	char16_t * target = ucs1;
	UErrorCode err = U_ZERO_ERROR;
	UConverter * cnv = u_getDefaultConverter(&err);
	if(U_SUCCESS(err) && cnv != NULL) {
		ucnv_reset(cnv);
		ucnv_toUnicode(cnv, &target, ucs1+n, &s2, s2 + /*u_astrnlen*/sstrnlen(s2, n), NULL, TRUE, &err);
		ucnv_reset(cnv); /* be good citizens */
		u_releaseDefaultConverter(cnv);
		if(U_FAILURE(err) && (err != U_BUFFER_OVERFLOW_ERROR)) {
			*ucs1 = 0; /* failure */
		}
		if(target < (ucs1+n)) { // U_BUFFER_OVERFLOW_ERROR isn't an err, just means no termination will happen.
			*target = 0; /* terminate */
		}
	}
	else {
		*ucs1 = 0;
	}
	return ucs1;
}

U_CAPI char16_t * U_EXPORT2 u_uastrcpy(char16_t * ucs1, const char * s2)
{
	UErrorCode err = U_ZERO_ERROR;
	UConverter * cnv = u_getDefaultConverter(&err);
	if(U_SUCCESS(err) && cnv != NULL) {
		ucnv_toUChars(cnv, ucs1, MAX_STRLEN, s2, (int32_t)strlen(s2), &err);
		u_releaseDefaultConverter(cnv);
		if(U_FAILURE(err)) {
			*ucs1 = 0;
		}
	}
	else {
		*ucs1 = 0;
	}
	return ucs1;
}

/*
   returns the minimum of (the length of the null-terminated string) and n.
 */
static int32_t u_ustrnlen(const char16_t * ucs1, int32_t n)
{
	int32_t len = 0;
	if(ucs1) {
		while(n-- && *(ucs1++)) {
			len++;
		}
	}
	return len;
}

U_CAPI char * U_EXPORT2 u_austrncpy(char * s1, const char16_t * ucs2, int32_t n)
{
	char * target = s1;
	UErrorCode err = U_ZERO_ERROR;
	UConverter * cnv = u_getDefaultConverter(&err);
	if(U_SUCCESS(err) && cnv != NULL) {
		ucnv_reset(cnv);
		ucnv_fromUnicode(cnv, &target, s1+n, &ucs2, ucs2+u_ustrnlen(ucs2, n), NULL, TRUE, &err);
		ucnv_reset(cnv); /* be good citizens */
		u_releaseDefaultConverter(cnv);
		if(U_FAILURE(err) && (err != U_BUFFER_OVERFLOW_ERROR)) {
			*s1 = 0; /* failure */
		}
		if(target < (s1+n)) { /* U_BUFFER_OVERFLOW_ERROR isn't an err, just means no termination will happen. */
			*target = 0; /* terminate */
		}
	}
	else {
		*s1 = 0;
	}
	return s1;
}

U_CAPI char * U_EXPORT2 u_austrcpy(char * s1, const char16_t * ucs2)
{
	UErrorCode err = U_ZERO_ERROR;
	UConverter * cnv = u_getDefaultConverter(&err);
	if(U_SUCCESS(err) && cnv != NULL) {
		int32_t len = ucnv_fromUChars(cnv, s1, MAX_STRLEN, ucs2, -1, &err);
		u_releaseDefaultConverter(cnv);
		s1[len] = 0;
	}
	else {
		*s1 = 0;
	}
	return s1;
}

#endif
