// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2014, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CNUMTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda              Creation
*********************************************************************************
*/
/* C API TEST FOR NUMBER FORMAT */
#ifndef _CNUMFRMTST
#define _CNUMFRMTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"


/**
 * The function used to test the Number format API
 **/
static void TestNumberFormat();

/**
 * The function used to test parsing of numbers in UNUM_SPELLOUT style
 **/
static void TestSpelloutNumberParse();

/**
 * The function used to test significant digits in the Number format API
 **/
static void TestSignificantDigits();

/**
 * The function used to test Number format API rounding with significant digits
 **/
static void TestSigDigRounding();

/**
 * The function used to test the Number format API with padding
 **/
static void TestNumberFormatPadding();

/**
 * The function used to test the Number format API with padding
 **/
static void TestInt64Format();

static void TestNonExistentCurrency();

/**
 * Test RBNF access through unumfmt APIs.
 **/
static void TestRBNFFormat();

/**
 * Test some Currency stuff
 **/
static void TestCurrencyRegression();

/**
 * Test strict parsing of "0"
 **/
static void TestParseZero();

/**
 * Test cloning formatter with RBNF
 **/
static void TestCloneWithRBNF();

/**
 * Test the Currency Usage Implementations
 **/
static void TestCurrencyUsage();
#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
