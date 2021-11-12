// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef RESOURCEBUNDLETEST_H
#define RESOURCEBUNDLETEST_H

#include "intltest.h"

/**
 * Tests for class ResourceBundle
 **/
class ResourceBundleTest: public IntlTest {
public:
    ResourceBundleTest();
    virtual ~ResourceBundleTest();
    
    void runIndexedTest( int32_t index, bool exec, const char *& name, char * par = NULL ) override;

    /** 
  * Perform several extensive tests using the subtest routine testTag
     **/
    void TestResourceBundles();
    /** 
  * Test construction of ResourceBundle accessing a custom test resource-file
     **/
    void TestConstruction();

    void TestExemplar();

    void TestGetSize();
    void TestGetLocaleByType();

private:
    /**
  * The assignment operator has no real implementation.
  * It is provided to make the compiler happy. Do not call.
     */
    ResourceBundleTest& operator = (const ResourceBundleTest&) { return *this; }

    /**
  * extensive subtests called by TestResourceBundles
     **/
    bool testTag(const char * frag, bool in_Root, bool in_te, bool in_te_IN);

    void record_pass(UnicodeString passMessage);
    void record_fail(UnicodeString errMessage);

    int32_t pass;
    int32_t fail;
};

#endif
