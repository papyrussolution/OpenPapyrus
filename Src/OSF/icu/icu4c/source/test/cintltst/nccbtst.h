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
#ifndef _NCCBTST
#define _NCCBTST

#include "unicode/utypes.h"
#include "unicode/ucnv.h"

/* C API TEST FOR CALL BACK ROUTINES OF CODESET CONVERSION COMPONENT */
#include "cintltst.h"


static void TestSkipCallBack();
static void TestStopCallBack();
static void TestSubCallBack();
static void TestSubWithValueCallBack();
static void TestLegalAndOtherCallBack();
static void TestSingleByteCallBack();


static void TestSkip(int32_t inputsize, int32_t outputsize);

static void TestStop(int32_t inputsize, int32_t outputsize);

static void TestSub(int32_t inputsize, int32_t outputsize);

static void TestSubWithValue(int32_t inputsize, int32_t outputsize);

static void TestLegalAndOthers(int32_t inputsize, int32_t outputsize);
static void TestSingleByte(int32_t inputsize, int32_t outputsize);
static void TestEBCDIC_STATEFUL_Sub(int32_t inputsize, int32_t outputsize);

/* Following will return FALSE *only* on a mismatch. They will return TRUE on any other error OR success, because
 * the error would have been emitted to log_err separately. */

bool testConvertFromUnicode(const char16_t *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback, const int32_t *expectOffsets,
                const char *mySubChar, int8_t len);


bool testConvertToUnicode( const uint8_t *source, int sourcelen, const char16_t *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len);

bool testConvertFromUnicodeWithContext(const char16_t *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback , const int32_t *expectOffsets, 
                const char *mySubChar, int8_t len, const void * context, UErrorCode expectedError);

bool testConvertToUnicodeWithContext( const uint8_t *source, int sourcelen, const char16_t *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len, const void * context, UErrorCode expectedError);

static void printSeq(const uint8_t* a, int len);
static void printUSeq(const char16_t * a, int len);
static void printSeqErr(const uint8_t* a, int len);
static void printUSeqErr(const char16_t * a, int len);
static void setNuConvTestName(const char *codepage, const char *direction);


#endif
