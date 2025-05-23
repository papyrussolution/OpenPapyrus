// uenum.c
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2002-2012, International Business Machines Corporation and others.  All Rights Reserved.
// encoding:   UTF-8
// created on: 2002jul08
// created by: Vladimir Weinstein
// 
#include <icu-internal.h>
#pragma hdrstop
#include "uenumimp.h"

/* Layout of the baseContext buffer. */
typedef struct {
	int32_t len; /* number of bytes available starting at 'data' */
	char data; /* actual data starts here */
} _UEnumBuffer;

/* Extra bytes to allocate in the baseContext buffer. */
static const int32_t PAD = 8;

/* Return a pointer to the baseContext buffer, possibly allocating
   or reallocating it if at least 'capacity' bytes are not available. */
static void * _getBuffer(UEnumeration* en, int32_t capacity) 
{
	if(en->baseContext != NULL) {
		if(((_UEnumBuffer*)en->baseContext)->len < capacity) {
			capacity += PAD;
			en->baseContext = uprv_realloc(en->baseContext,
				sizeof(int32_t) + capacity);
			if(en->baseContext == NULL) {
				return NULL;
			}
			((_UEnumBuffer*)en->baseContext)->len = capacity;
		}
	}
	else {
		capacity += PAD;
		en->baseContext = uprv_malloc(sizeof(int32_t) + capacity);
		if(en->baseContext == NULL) {
			return NULL;
		}
		((_UEnumBuffer*)en->baseContext)->len = capacity;
	}

	return (void *)&((_UEnumBuffer*)en->baseContext)->data;
}

U_CAPI void U_EXPORT2 uenum_close(UEnumeration* en)
{
	if(en) {
		if(en->close) {
			if(en->baseContext) {
				uprv_free(en->baseContext);
			}
			en->close(en);
		}
		else // this seems dangerous, but we better kill the object
			uprv_free(en);
	}
}

U_CAPI int32_t U_EXPORT2 uenum_count(UEnumeration* en, UErrorCode * status)
{
	if(!en || U_FAILURE(*status))
		return -1;
	else if(en->count)
		return en->count(en, status);
	else {
		*status = U_UNSUPPORTED_ERROR;
		return -1;
	}
}

/* Don't call this directly. Only uenum_unext should be calling this. */
U_CAPI const char16_t * U_EXPORT2 uenum_unextDefault(UEnumeration* en, int32_t* resultLength, UErrorCode * status)
{
	char16_t * ustr = NULL;
	int32_t len = 0;
	if(en->next) {
		const char * cstr = en->next(en, &len, status);
		if(cstr) {
			ustr = (char16_t *)_getBuffer(en, (len+1) * sizeof(char16_t));
			if(!ustr)
				*status = U_MEMORY_ALLOCATION_ERROR;
			else
				u_charsToUChars(cstr, ustr, len+1);
		}
	}
	else {
		*status = U_UNSUPPORTED_ERROR;
	}
	ASSIGN_PTR(resultLength, len);
	return ustr;
}

/* Don't call this directly. Only uenum_next should be calling this. */
U_CAPI const char * U_EXPORT2 uenum_nextDefault(UEnumeration* en, int32_t* resultLength, UErrorCode * status)
{
	if(en->uNext) {
		char * tempCharVal;
		const char16_t * tempUCharVal = en->uNext(en, resultLength, status);
		if(tempUCharVal == NULL) {
			return NULL;
		}
		tempCharVal = (char *)_getBuffer(en, (*resultLength+1) * sizeof(char));
		if(!tempCharVal) {
			*status = U_MEMORY_ALLOCATION_ERROR;
			return NULL;
		}
		u_UCharsToChars(tempUCharVal, tempCharVal, *resultLength + 1);
		return tempCharVal;
	}
	else {
		*status = U_UNSUPPORTED_ERROR;
		return NULL;
	}
}

U_CAPI const char16_t * U_EXPORT2 uenum_unext(UEnumeration* en, int32_t* resultLength, UErrorCode * status)
{
	if(!en || U_FAILURE(*status))
		return NULL;
	if(en->uNext)
		return en->uNext(en, resultLength, status);
	else {
		*status = U_UNSUPPORTED_ERROR;
		return NULL;
	}
}

U_CAPI const char * U_EXPORT2 uenum_next(UEnumeration* en, int32_t* resultLength, UErrorCode * status)
{
	if(!en || U_FAILURE(*status)) {
		return NULL;
	}
	if(en->next) {
		if(resultLength) {
			return en->next(en, resultLength, status);
		}
		else {
			int32_t dummyLength = 0;
			return en->next(en, &dummyLength, status);
		}
	}
	else {
		*status = U_UNSUPPORTED_ERROR;
		return NULL;
	}
}

U_CAPI void U_EXPORT2 uenum_reset(UEnumeration* en, UErrorCode * status)
{
	if(!en || U_FAILURE(*status))
		return;
	if(en->reset)
		en->reset(en, status);
	else
		*status = U_UNSUPPORTED_ERROR;
}
