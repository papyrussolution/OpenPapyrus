// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2014, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CLOCTST.H
*
* Modification History:
*        Name                     Description
*     Madhu Katragadda            Converted to C
*********************************************************************************
*/
#ifndef _CLOCTEST
#define _CLOCTEST

#include "cintltst.h"
/*C API TEST FOR LOCALE */

/**
 * Test functions to set and get data fields
 **/
static void TestBasicGetters();
static void TestPrefixes();
/**
 * Use Locale to access Resource file data and compare against expected values
 **/
static void TestSimpleResourceInfo();
/**
 * Use Locale to access Resource file display names and compare against expected values
 **/
static  void TestDisplayNames();
static  void TestGetDisplayScriptPreFlighting21160();

/**
 * Test getAvailableLocales
 **/
static void TestGetAvailableLocales();
static void TestGetAvailableLocalesByType();
/**
 * Test functions to set and access a custom data directory
 **/
 static void TestDataDirectory();
/**
 * Test functions to test get ISO countries and Languages
 **/
 static void TestISOFunctions();
/**
 * Test functions to test get ISO3 countries and Languages Fallback
 **/
 static void TestISO3Fallback();
/**
 * Test functions to test get ISO3 countries and Languages for Uninstalled locales
 **/
 static void TestUninstalledISO3Names();
 static void TestObsoleteNames();
/**
 * Test functions uloc_getDisplaynames()
 **/
 static void TestSimpleDisplayNames();
/**
 * Test functions uloc_getDisplaynames()
 **/
 static void TestVariantParsing();

 /* Test getting keyword enumeratin */
 static void TestKeywordVariants();

 static void TestKeywordSet();
 static void TestKeywordSetError();

 /* Test getting keyword values */
 static void TestKeywordVariantParsing();
 
 /* Test warning for no data in getDisplay* */
 static void TestDisplayNameWarning();

 /* Test uloc_getLocaleForLCID */
 static void TestGetLocaleForLCID();

/**
 * routine to perform subtests, used by TestDisplayNames
 */
 static void doTestDisplayNames(const char * inLocale, int32_t compareIndex);

 static void TestCanonicalization();
 static void TestCanonicalizationBuffer();
static  void TestCanonicalization21749StackUseAfterScope();

 static void TestDisplayKeywords();

 static void TestDisplayKeywordValues();

 static void TestGetBaseName();

static void TestTrailingNull();

static void TestGetLocale();

/**
 * additional initialization for datatables storing expected values
 */
static void setUpDataTable();
static void cleanUpDataTable();
/*static void displayDataTable();*/
static void TestAcceptLanguage();

/**
 * test locale aliases 
*/
static void TestCalendar(); 
static void TestDateFormat();
static void TestCollation();
static void TestULocale();
static void TestUResourceBundle();
static void TestDisplayName();

static void TestAcceptLanguage();

static void TestOrientation();

static void TestLikelySubtags();

/**
 * test terminate correctly.
 */
static void Test21157CorrectTerminating();

/**
 * language tag
 */
static void TestForLanguageTag();
static void TestToLanguageTag();
static void TestBug20132();
static void TestLangAndRegionCanonicalize();

static void TestToUnicodeLocaleKey();
static void TestToLegacyKey();
static void TestToUnicodeLocaleType();
static void TestToLegacyType();
static void TestBug20149();
static void TestCDefaultLocale();
static void TestBug21449InfiniteLoop();


/**
 * U_USING_DEFAULT_WARNING
 */
static void TestUsingDefaultWarning();

/**
 * locale data
 */
static void TestEnglishExemplarCharacters();

#endif
