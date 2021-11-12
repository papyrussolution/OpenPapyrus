// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CRESTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Converted to C
*********************************************************************************
*/
#ifndef _CRESTST
#define _CRESTST
/* C API TEST FOR RESOURCEBUNDLE */
#include "cintltst.h"




    void addTestResourceBundleTest(TestNode**);

    /**
  * Perform several extensive tests using the subtest routine testTag
     */
    void TestResourceBundles();
    /** 
  * Test construction of ResourceBundle accessing a custom test resource-file
     */
    void TestConstruction1();

    void TestConstruction2();

    void TestAliasConflict();

    static void TestGetSize();

    static void TestGetLocaleByType();

    /**
  * extensive subtests called by TestResourceBundles
     **/

    bool testTag(const char * frag, bool in_Root, bool in_te, bool in_te_IN);

    void record_pass();
    void record_fail();


    int32_t pass;
    int32_t fail;

#endif
