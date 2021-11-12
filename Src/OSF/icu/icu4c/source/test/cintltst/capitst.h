// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * Copyright (c) 1997-2013 International Business Machines 
 * Corporation and others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CAPITEST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Converted to C
*     Brian Rower                 Added TestOpenVsOpenRules
*********************************************************************************
*//* C API TEST For COLLATOR */

#ifndef _CCOLLAPITST
#define _CCOLLAPITST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cintltst.h"
#include "callcoll.h"
#define MAX_TOKEN_LEN 16


    /**
  * error reporting utility method
     **/

    static void doAssert(int condition, const char *message);
    /**
  * Collator Class Properties
  * ctor, dtor, createInstance, compare, getStrength/setStrength
  * getDecomposition/setDecomposition, getDisplayName
     */
    void TestProperty();
    /**
  * Test RuleBasedCollator and getRules
     **/
    void TestRuleBasedColl();
    
    /**
  * Test compare
     **/
    void TestCompare();
    /**
  * Test hashCode functionality
     **/
    void TestHashCode();
    /**
  * Tests the constructor and numerous other methods for CollationKey
     **/
   void TestSortKey();
    /**
  * test the CollationElementIterator methods
     **/
   void TestElemIter();
    /**
  * Test ucol_getAvailable and ucol_countAvailable()
     **/
    void TestGetAll();
    /**
  * Test ucol_GetDefaultRules ()
    void TestGetDefaultRules();
     **/

    void TestDecomposition();
    /**
  * Test ucol_safeClone ()
     **/    
    void TestSafeClone();

    /**
  * Test ucol_cloneBinary(), ucol_openBinary()
     **/
    void TestCloneBinary();

    /**
  * Test ucol_open() vs. ucol_openRules()
     **/
    void TestOpenVsOpenRules();

    /**
  * Test getting bounds for a sortkey
     */
    void TestBounds();

    /**
  * Test ucol_getLocale function
     */
    void TestGetLocale();

    /**
  * Test buffer overrun while having smaller buffer for sortkey (j1865)
     */
    void TestSortKeyBufferOverrun();
    /**
  * Test getting and setting of attributes
     */
    void TestGetSetAttr();
    /**
  * Test getTailoredSet
     */
    void TestGetTailoredSet();

    /**
  * Test mergeSortKeys
     */
    void TestMergeSortKeys();

    /** 
  * test short string and collator identifier functions
     */
    static void TestShortString();

    /** 
  * test getContractions and getUnsafeSet
     */
    static void TestGetContractionsAndUnsafes();

    /**
  * Test funny stuff with open binary
     */
    static void TestOpenBinary();

    /**
  * Test getKeywordValuesForLocale API
     */
    static void TestGetKeywordValuesForLocale();

    /**
  * test strcoll with null arg
     */
    static void TestStrcollNull();
 
    /**
  * Simple test for ICU-21460.  The issue affects all components, but was originally reported against collation.
     */
    static void TestLocaleIDWithUnderscoreAndExtension();

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
