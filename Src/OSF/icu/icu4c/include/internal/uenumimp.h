// uenumimp.h
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2002-2006, International Business Machines Corporation and others.  All Rights Reserved.
// encoding:   UTF-8
// created on: 2002jul08
// created by: Vladimir Weinstein
// 
#ifndef __UENUMIMP_H
#define __UENUMIMP_H

#include "unicode/uenum.h"

U_CDECL_BEGIN

/**
 * following are the type declarations for
 * implementations of APIs. If any of these
 * functions are NULL, U_UNSUPPORTED_ERROR
 * is returned. If close is NULL, the enumeration
 * object is going to be released.
 * Initial error checking is done in the body
 * of API function, so the implementations
 * need not to check the initial error condition.
 */

/**
 * Function type declaration for uenum_close().
 *
 * This function should cleanup the enumerator object
 *
 * @param en enumeration to be closed
 */
typedef void U_CALLCONV UEnumClose (UEnumeration * en);
/**
 * Function type declaration for uenum_count().
 *
 * This function should count the number of elements
 * in this enumeration
 *
 * @param en enumeration to be counted
 * @param status pointer to UErrorCode variable
 * @return number of elements in enumeration
 */
typedef int32_t U_CALLCONV UEnumCount (UEnumeration * en, UErrorCode * status);

/**
 * Function type declaration for uenum_unext().
 *
 * This function returns the next element as a char16_t *,
 * or NULL after all elements haven been enumerated.
 *
 * @param en enumeration
 * @param resultLength pointer to result length
 * @param status pointer to UErrorCode variable
 * @return next element as char16_t *,
 *         or NULL after all elements haven been enumerated
 */
typedef const char16_t * U_CALLCONV UEnumUNext (UEnumeration* en, int32_t* resultLength, UErrorCode * status);
/**
 * Function type declaration for uenum_next().
 *
 * This function returns the next element as a char *,
 * or NULL after all elements haven been enumerated.
 *
 * @param en enumeration
 * @param resultLength pointer to result length
 * @param status pointer to UErrorCode variable
 * @return next element as char *,
 *         or NULL after all elements haven been enumerated
 */
typedef const char * U_CALLCONV UEnumNext (UEnumeration* en, int32_t* resultLength, UErrorCode * status);
/**
 * Function type declaration for uenum_reset().
 *
 * This function should reset the enumeration
 * object
 *
 * @param en enumeration
 * @param status pointer to UErrorCode variable
 */
typedef void U_CALLCONV UEnumReset (UEnumeration* en, UErrorCode * status);

struct UEnumeration {
	void * baseContext; /* baseContext. For the base class only. Don't touch! */
	void * context; /* context. Use it for what you need */
	/**
	 * these are functions that will
	 * be used for APIs
	 */
	UEnumClose * close; /* called from uenum_close */
	UEnumCount * count; /* called from uenum_count */
	UEnumUNext * uNext; /* called from uenum_unext */
	UEnumNext  * next; /* called from uenum_next */
	UEnumReset * reset; /* called from uenum_reset */
};

U_CDECL_END

/* This is the default implementation for uenum_unext().
 * It automatically converts the char * string to char16_t *.
 * Don't call this directly.  This is called internally by uenum_unext
 * when a UEnumeration is defined with 'uNext' pointing to this
 * function.
 */
U_CAPI const char16_t * U_EXPORT2 uenum_unextDefault(UEnumeration* en, int32_t* resultLength, UErrorCode * status);

/* This is the default implementation for uenum_next().
 * It automatically converts the char16_t * string to char *.
 * Don't call this directly.  This is called internally by uenum_next
 * when a UEnumeration is defined with 'next' pointing to this
 * function.
 */
U_CAPI const char * U_EXPORT2 uenum_nextDefault(UEnumeration* en, int32_t* resultLength, UErrorCode * status);

#endif
