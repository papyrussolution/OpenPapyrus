// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 * Copyright (C) 2015, International Business Machines Corporation
 * and others. All Rights Reserved.
 *******************************************************************************
 * standardplural.cpp
 *
 * created on: 2015dec14
 * created by: Markus W. Scherer
 */
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_FORMATTING

#include "standardplural.h"

U_NAMESPACE_BEGIN

static const char * gKeywords[StandardPlural::COUNT] = { "zero", "one", "two", "few", "many", "other", "=0", "=1" };

const char * StandardPlural::getKeyword(Form p) 
{
	assert(ZERO <= p && p < COUNT);
	return gKeywords[p];
}

int32_t StandardPlural::indexOrNegativeFromString(const char * keyword) 
{
	switch(*keyword++) {
		case 'f':
		    if(sstreq(keyword, "ew")) {
			    return FEW;
		    }
		    break;
		case 'm':
		    if(sstreq(keyword, "any")) {
			    return MANY;
		    }
		    break;
		case 'o':
		    if(sstreq(keyword, "ther")) {
			    return OTHER;
		    }
		    else if(sstreq(keyword, "ne")) {
			    return ONE;
		    }
		    break;
		case 't':
		    if(sstreq(keyword, "wo")) {
			    return TWO;
		    }
		    break;
		case 'z':
		    if(sstreq(keyword, "ero")) {
			    return ZERO;
		    }
		    break;
		case '=':
		    if(sstreq(keyword, "0")) {
			    return EQ_0;
		    }
		    else if(sstreq(keyword, "1")) {
			    return EQ_1;
		    }
		    break;
		// Also allow "0" and "1"
		case '0':
		    if(*keyword == 0) {
			    return EQ_0;
		    }
		    break;
		case '1':
		    if(*keyword == 0) {
			    return EQ_1;
		    }
		    break;
		default:
		    break;
	}
	return -1;
}

static const char16_t gZero[] = u"zero";
static const char16_t gOne[] = u"one";
static const char16_t gTwo[] = u"two";
static const char16_t gFew[] = u"few";
static const char16_t gMany[] = u"many";
static const char16_t gOther[] = u"other";
static const char16_t gEq0[] = u"=0";
static const char16_t gEq1[] = u"=1";

int32_t StandardPlural::indexOrNegativeFromString(const UnicodeString & keyword) {
	switch(keyword.length()) {
		case 1:
		    if(keyword.charAt(0) == '0') {
			    return EQ_0;
		    }
		    else if(keyword.charAt(0) == '1') {
			    return EQ_1;
		    }
		    break;
		case 2:
		    if(keyword.compare(gEq0, 2) == 0) {
			    return EQ_0;
		    }
		    else if(keyword.compare(gEq1, 2) == 0) {
			    return EQ_1;
		    }
		    break;
		case 3:
		    if(keyword.compare(gOne, 3) == 0) {
			    return ONE;
		    }
		    else if(keyword.compare(gTwo, 3) == 0) {
			    return TWO;
		    }
		    else if(keyword.compare(gFew, 3) == 0) {
			    return FEW;
		    }
		    break;
		case 4:
		    if(keyword.compare(gMany, 4) == 0) {
			    return MANY;
		    }
		    else if(keyword.compare(gZero, 4) == 0) {
			    return ZERO;
		    }
		    break;
		case 5:
		    if(keyword.compare(gOther, 5) == 0) {
			    return OTHER;
		    }
		    break;
		default:
		    break;
	}
	return -1;
}

int32_t StandardPlural::indexFromString(const char * keyword, UErrorCode & errorCode) {
	if(U_FAILURE(errorCode)) {
		return OTHER;
	}
	int32_t i = indexOrNegativeFromString(keyword);
	if(i >= 0) {
		return i;
	}
	else {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return OTHER;
	}
}

int32_t StandardPlural::indexFromString(const UnicodeString & keyword, UErrorCode & errorCode) {
	if(U_FAILURE(errorCode)) {
		return OTHER;
	}
	int32_t i = indexOrNegativeFromString(keyword);
	if(i >= 0) {
		return i;
	}
	else {
		errorCode = U_ILLEGAL_ARGUMENT_ERROR;
		return OTHER;
	}
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_FORMATTING
