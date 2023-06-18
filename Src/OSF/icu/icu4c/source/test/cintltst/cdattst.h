// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2014, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CDATTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
/* C API TEST FOR DATE FORMAT */
#ifndef _CDATFRMTST
#define _CDATFRMTST

#include "unicode/utypes.h"
#include "unicode/udat.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"

    /**
  * The functions used to test the Date format API
     **/
    static void TestDateFormat();
    static void TestRelativeDateFormat();

    /**
  * The function used to test API  udat_getSymbols(), udat_setSymbols() and udat_countSymbols()
     **/
    static void TestSymbols();

    /**
  * Test DateFormat(Calendar) API
     */
    static void TestDateFormatCalendar();

    /**
  * test subroutines used by TestSymbols
     **/
    static void VerifygetSymbols(UDateFormat*, UDateFormatSymbolType, int32_t, const char *);
    static void VerifysetSymbols(UDateFormat*, UDateFormatSymbolType, int32_t, const char *);
    static void VerifygetsetSymbols(UDateFormat*, UDateFormat*, UDateFormatSymbolType, int32_t);
    
    /**
  * test subroutine used by the testing functions
     **/
    static char16_t * myNumformat(const UNumberFormat * numfor, double d);
    static int getCurrentYear();

    /**
  * Test DateFormat override number format API
     */
     static void TestOverrideNumberFormat();


#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
