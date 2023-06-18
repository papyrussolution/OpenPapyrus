// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2013, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/***************************************************************************
*
* File CRESTST.H
*
* Modification History:
*        Name               Date               Description
*   Madhu Katragadda    05/09/2000   Ported Tests for New ResourceBundle API
*   Madhu Katragadda    05/24/2000   Added new tests to test RES_BINARY for collationElements
*************************************************************************************************
*/
#ifndef _CRESTSTN
#define _CRESTSTN

#include "unicode/utypes.h"
#include "unicode/ures.h"

/* C TEST FOR NEW RESOURCEBUNDLE API*/
#include "cintltst.h"

/*
 * Test wrapper for ures_getStringXYZ(), for testing other variants of
 * these functions as well.
 * If index>=0, calls ures_getStringByIndex().
 * If key!=NULL, calls ures_getStringByKey().
 */
extern const char16_t *
tres_getString(const UResourceBundle *resB,
               int32_t index, const char *key,
               int32_t *length,
               UErrorCode *status);

void addNEWResourceBundleTest(TestNode**);

/**
*Perform several extensive tests using the subtest routine testTag
*/
static void TestResourceBundles();
/** 
* Test construction of ResourceBundle accessing a custom test resource-file
**/
static void TestConstruction1();

static void TestAliasConflict();

static void TestFallback();

static void TestPreventFallback();

static void TestBinaryCollationData();

static void TestNewTypes();

static void TestEmptyTypes();

static void TestAPI();

static void TestErrorConditions();

static void TestGetVersion();

static void TestGetVersionColl();

static void TestEmptyBundle();

static void TestDirectAccess();

static void TestTicket9804();

static void TestResourceLevelAliasing();

static void TestErrorCodes();

static void TestJB3763();

static void TestXPath();

/**
* extensive subtests called by TestResourceBundles
**/
static bool testTag(const char * frag, bool in_Root, bool in_te, bool in_te_IN);

static void record_pass();
static void record_fail();


#endif
