// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
* COPYRIGHT:
* Copyright (c) 2005-2013, International Business Machines Corporation and
* others. All Rights Reserved.
********************************************************************/
/*
 * File utexttst.c
 *
 * Modification History:
 *
 *   Date          Name               Description
 *   06/13/2005    Andy Heninger      Creation
 *******************************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop
#include "cintltst.h"

static void TestAPI();

void addUTextTest(TestNode** root)
{
	addTest(root, &TestAPI,    "tsutil/UTextTest/TestAPI");
}

#define TEST_ASSERT(x) UPRV_BLOCK_MACRO_BEGIN { \
		if((x)==FALSE) { \
			log_err("Test failure in file %s at line %d\n", __FILE__, __LINE__); \
			gFailed = TRUE; \
		} \
} UPRV_BLOCK_MACRO_END

#define TEST_SUCCESS(status) UPRV_BLOCK_MACRO_BEGIN { \
		if(U_FAILURE(status)) { \
			log_err("Test failure in file %s at line %d. Error = \"%s\"\n", \
			    __FILE__, __LINE__, u_errorName(status)); \
			gFailed = TRUE; \
		} \
} UPRV_BLOCK_MACRO_END

/*
 *  TestAPI   verify that the UText API is accessible from C programs.
 *            This is not intended to be a complete test of the API functionality.  That is
 *            in the C++ intltest program.
 *            This test is intended to check that everything can be accessed and built in
 *            a pure C environment.
 */

static void TestAPI() {
	UErrorCode status = U_ZERO_ERROR;
	bool gFailed = FALSE;
	(void)gFailed; /* Suppress set but not used warning. */

	/* Open    */
	{
		UText utLoc = UTEXT_INITIALIZER;
		const char * cString = "\x61\x62\x63\x64";
		char16_t uString[]  = {0x41, 0x42, 0x43, 0};
		UText          * uta;
		UText          * utb;
		char16_t c;

		uta = utext_openUChars(NULL, uString, -1, &status);
		TEST_SUCCESS(status);
		c = utext_next32(uta);
		TEST_ASSERT(c == 0x41);
		utb = utext_close(uta);
		TEST_ASSERT(utb == NULL);

		uta = utext_openUTF8(&utLoc, cString, -1, &status);
		TEST_SUCCESS(status);
		TEST_ASSERT(uta == &utLoc);

		uta = utext_close(&utLoc);
		TEST_ASSERT(uta == &utLoc);
	}

	/* utext_clone()  */
	{
		char16_t uString[]  = {0x41, 0x42, 0x43, 0};
		int64_t len;
		UText   * uta;
		UText   * utb;

		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, uString, -1, &status);
		TEST_SUCCESS(status);
		utb = utext_clone(NULL, uta, FALSE, FALSE, &status);
		TEST_SUCCESS(status);
		TEST_ASSERT(utb != NULL);
		TEST_ASSERT(utb != uta);
		len = utext_nativeLength(uta);
		TEST_ASSERT(len == u_strlen(uString));
		utext_close(uta);
		utext_close(utb);
	}

	/* basic access functions  */
	{
		char16_t uString[]  = {0x41, 0x42, 0x43, 0};
		UText     * uta;
		UChar32 c;
		int64_t len;
		bool b;
		int64_t i;

		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, uString, -1, &status);
		TEST_ASSERT(uta!=NULL);
		TEST_SUCCESS(status);
		b = utext_isLengthExpensive(uta);
		TEST_ASSERT(b==TRUE);
		len = utext_nativeLength(uta);
		TEST_ASSERT(len == u_strlen(uString));
		b = utext_isLengthExpensive(uta);
		TEST_ASSERT(b==FALSE);

		c = utext_char32At(uta, 0);
		TEST_ASSERT(c==uString[0]);

		c = utext_current32(uta);
		TEST_ASSERT(c==uString[0]);

		c = utext_next32(uta);
		TEST_ASSERT(c==uString[0]);
		c = utext_current32(uta);
		TEST_ASSERT(c==uString[1]);

		c = utext_previous32(uta);
		TEST_ASSERT(c==uString[0]);
		c = utext_current32(uta);
		TEST_ASSERT(c==uString[0]);

		c = utext_next32From(uta, 1);
		TEST_ASSERT(c==uString[1]);
		c = utext_next32From(uta, u_strlen(uString));
		TEST_ASSERT(c==U_SENTINEL);

		c = utext_previous32From(uta, 2);
		TEST_ASSERT(c==uString[1]);
		i = utext_getNativeIndex(uta);
		TEST_ASSERT(i == 1);

		utext_setNativeIndex(uta, 0);
		b = utext_moveIndex32(uta, 1);
		TEST_ASSERT(b==TRUE);
		i = utext_getNativeIndex(uta);
		TEST_ASSERT(i==1);

		b = utext_moveIndex32(uta, u_strlen(uString)-1);
		TEST_ASSERT(b==TRUE);
		i = utext_getNativeIndex(uta);
		TEST_ASSERT(i==u_strlen(uString));

		b = utext_moveIndex32(uta, 1);
		TEST_ASSERT(b==FALSE);
		i = utext_getNativeIndex(uta);
		TEST_ASSERT(i==u_strlen(uString));

		utext_setNativeIndex(uta, 0);
		c = UTEXT_NEXT32(uta);
		TEST_ASSERT(c==uString[0]);
		c = utext_current32(uta);
		TEST_ASSERT(c==uString[1]);

		c = UTEXT_PREVIOUS32(uta);
		TEST_ASSERT(c==uString[0]);
		c = UTEXT_PREVIOUS32(uta);
		TEST_ASSERT(c==U_SENTINEL);

		utext_close(uta);
	}

	{
		/*
		 * UText opened on a NULL string with zero length
		 */
		UText    * uta;
		UChar32 c;

		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, NULL, 0, &status);
		TEST_SUCCESS(status);
		c = UTEXT_NEXT32(uta);
		TEST_ASSERT(c == U_SENTINEL);
		utext_close(uta);

		uta = utext_openUTF8(NULL, NULL, 0, &status);
		TEST_SUCCESS(status);
		c = UTEXT_NEXT32(uta);
		TEST_ASSERT(c == U_SENTINEL);
		utext_close(uta);
	}

	{
		/*
		 * extract
		 */
		UText     * uta;
		char16_t uString[]  = {0x41, 0x42, 0x43, 0};
		char16_t buf[100];
		int32_t i;
		/* Test pinning of input bounds */
		char16_t uString2[]  = {0x41, 0x42, 0x43, 0x44, 0x45,
				     0x46, 0x47, 0x48, 0x49, 0x4A, 0};
		char16_t * uString2Ptr = uString2 + 5;

		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, uString, -1, &status);
		TEST_SUCCESS(status);

		status = U_ZERO_ERROR;
		i = utext_extract(uta, 0, 100, NULL, 0, &status);
		TEST_ASSERT(status==U_BUFFER_OVERFLOW_ERROR);
		TEST_ASSERT(i == u_strlen(uString));

		status = U_ZERO_ERROR;
		memset(buf, 0, sizeof(buf));
		i = utext_extract(uta, 0, 100, buf, 100, &status);
		TEST_SUCCESS(status);
		TEST_ASSERT(i == u_strlen(uString));
		i = u_strcmp(uString, buf);
		TEST_ASSERT(i == 0);
		utext_close(uta);

		/* Test pinning of input bounds */
		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, uString2Ptr, -1, &status);
		TEST_SUCCESS(status);

		status = U_ZERO_ERROR;
		memset(buf, 0, sizeof(buf));
		i = utext_extract(uta, -3, 20, buf, 100, &status);
		TEST_SUCCESS(status);
		TEST_ASSERT(i == u_strlen(uString2Ptr));
		i = u_strcmp(uString2Ptr, buf);
		TEST_ASSERT(i == 0);
		utext_close(uta);
	}

	{
		/*
		 *  Copy, Replace, isWritable
		 *    Can't create an editable UText from plain C, so all we
		 *    can easily do is check that errors returned.
		 */
		UText     * uta;
		char16_t uString[]  = {0x41, 0x42, 0x43, 0};
		bool b;

		status = U_ZERO_ERROR;
		uta = utext_openUChars(NULL, uString, -1, &status);
		TEST_SUCCESS(status);

		b = utext_isWritable(uta);
		TEST_ASSERT(b == FALSE);

		b = utext_hasMetaData(uta);
		TEST_ASSERT(b == FALSE);

		utext_replace(uta,
		    0, 1,       /* start, limit */
		    uString, -1,    /* replacement, replacement length */
		    &status);
		TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

		utext_copy(uta,
		    0, 1,        /* start, limit      */
		    2,           /* destination index */
		    FALSE,       /* move flag         */
		    &status);
		TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

		utext_close(uta);
	}
}
