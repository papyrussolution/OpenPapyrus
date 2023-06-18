// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File NCCBTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda           creation
*********************************************************************************
*/
#ifndef _NCNVFBTS
#define _NCNVFBTS
/* C API TEST FOR FALL BACK ROUTINES OF CODESET CONVERSION COMPONENT */
#include "cintltst.h"
#include "unicode/utypes.h"

static void TestConverterFallBack();
static void TestConvertFallBackWithBufferSizes(int32_t outsize, int32_t insize );
static bool testConvertFromUnicode(const char16_t *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, bool fallback, const int32_t *expectOffsets);
static bool testConvertToUnicode( const uint8_t *source, int sourcelen, const char16_t *expect, int expectlen, 
               const char *codepage, bool fallback, const int32_t *expectOffsets);


static void printSeq(const unsigned char * a, int len);
static void printUSeq(const char16_t * a, int len);
static void printSeqErr(const unsigned char * a, int len);
static void printUSeqErr(const char16_t * a, int len);
static void setNuConvTestName(const char *codepage, const char *direction);


#endif
