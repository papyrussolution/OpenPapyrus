// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, 2007-2009 International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CNMDPTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
/* C DEPTH TEST FOR NUMBER FORMAT */

#ifndef _CNUMDEPTST
#define _CNUMDEPTST

#include "unicode/utypes.h"
#include "unicode/unum.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"

/* The function used to test different format patterns*/
static void TestPatterns();

/*  Test the handling of quotes*/
static void TestQuotes();

/* Test patterns with exponential representation*/
static void TestExponential();

/* Test the handling of the currency symbol in patterns. */
static void TestCurrencySign(); 

/* Test proper rounding by the format method.*/
static void TestRounding487();

/* Test proper handling of rounding modes. */
static void TestRounding5350();

/* Test localized currency patterns. */
static void TestCurrency();

/* Test getDoubleAttribute and getDoubleAttribute */
static void TestDoubleAttribute();

static void TestSecondaryGrouping();

/*Internal functions used*/
static void roundingTest(UNumberFormat*, double,  int32_t, const char *);
static void roundingTest2(UNumberFormat*, double, int32_t, const char *);

static void TestCurrencyKeywords();

static void TestGetKeywordValuesForLocale();

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
