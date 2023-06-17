// ures_cnv.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *   Copyright (C) 1997-2006, International Business Machines Corporation and others.  All Rights Reserved.
 *   encoding:   UTF-8
 *   created on: 2004aug25
 *   created by: Markus W. Scherer
 *   Character conversion functions moved here from uresbund.c
 */
#include <icu-internal.h>
#pragma hdrstop
#include "unicode/ucnv.h"
#include "unicode/ures.h"
#include "ustr_cnv.h"

U_CAPI UResourceBundle * U_EXPORT2 ures_openU(const char16_t * myPath, const char * localeID, UErrorCode * status)
{
	char pathBuffer[1024];
	char * path = pathBuffer;
	if(!status || U_FAILURE(*status)) {
		return NULL;
	}
	if(myPath==NULL) {
		path = NULL;
	}
	else {
		int32_t length = u_strlen(myPath);
		if(length>=(int32_t)sizeof(pathBuffer)) {
			*status = U_ILLEGAL_ARGUMENT_ERROR;
			return NULL;
		}
		else if(uprv_isInvariantUString(myPath, length)) {
			/*
			 * the invariant converter is sufficient for package and tree names
			 * and is more efficient
			 */
			u_UCharsToChars(myPath, path, length+1); /* length+1 to include the NUL */
		}
		else {
#if !UCONFIG_NO_CONVERSION
			/* use the default converter to support variant-character paths */
			UConverter * cnv = u_getDefaultConverter(status);
			length = ucnv_fromUChars(cnv, path, (int32_t)sizeof(pathBuffer), myPath, length, status);
			u_releaseDefaultConverter(cnv);
			if(U_FAILURE(*status)) {
				return NULL;
			}
			if(length>=(int32_t)sizeof(pathBuffer)) {
				/* not NUL-terminated - path too long */
				*status = U_ILLEGAL_ARGUMENT_ERROR;
				return NULL;
			}
#else
			/* the default converter is not available */
			*status = U_UNSUPPORTED_ERROR;
			return NULL;
#endif
		}
	}
	return ures_open(path, localeID, status);
}
