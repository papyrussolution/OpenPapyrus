// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
* COPYRIGHT:
* Copyright (c) 1997-2016, International Business Machines
* Corporation and others. All Rights Reserved.
********************************************************************/
/********************************************************************************
 *
 * File CFORMTST.C
 *
 * Modification History:
 *        Name                     Description
 *     Madhu Katragadda               Creation
 *********************************************************************************
 */

/* FormatTest is a medium top level test for everything in the  C FORMAT API */
#include <icu-internal.h>
#pragma hdrstop
#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"
#include "cformtst.h"

void addCalTest(TestNode**);
void addDateForTest(TestNode**);
void addDateTimePatternGeneratorTest(TestNode**);
void addDateIntervalFormatTest(TestNode**);
void addRelativeDateFormatTest(TestNode**);
void addNumForTest(TestNode**);
void addMsgForTest(TestNode**);
void addDateForRgrTest(TestNode**);
void addNumFrDepTest(TestNode**);
void addDtFrDepTest(TestNode**);
void addUtmsTest(TestNode**);
void addCurrencyTest(TestNode**);
void addPluralRulesTest(TestNode**);
void addURegionTest(TestNode** root);
void addUListFmtTest(TestNode** root);
void addUNumberFormatterTest(TestNode** root);
void addUFormattedValueTest(TestNode** root);
void addUNumberRangeFormatterTest(TestNode** root);

void addFormatTest(TestNode** root);

void addFormatTest(TestNode** root)
{
	addCalTest(root);
	addDateForTest(root);
	addDateTimePatternGeneratorTest(root);
	addDateIntervalFormatTest(root);
#if !UCONFIG_NO_BREAK_ITERATION
	addRelativeDateFormatTest(root);
#endif /* !UCONFIG_NO_BREAK_ITERATION */
	addNumForTest(root);
	addNumFrDepTest(root);
	addMsgForTest(root);
	addDateForRgrTest(root);
	addDtFrDepTest(root);
	addUtmsTest(root);
	addCurrencyTest(root);
	addPluralRulesTest(root);
	addURegionTest(root);
	addUListFmtTest(root);
	addUNumberFormatterTest(root);
	addUFormattedValueTest(root);
	addUNumberRangeFormatterTest(root);
}

/*Internal functions used*/

char16_t * myDateFormat(UDateFormat* dat, UDate d1)
{
	char16_t * result1 = NULL;
	int32_t resultlength, resultlengthneeded;
	UErrorCode status = U_ZERO_ERROR;

	resultlength = 0;
	resultlengthneeded = udat_format(dat, d1, NULL, resultlength, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		resultlength = resultlengthneeded+1;
		result1 = (char16_t *)ctst_malloc(sizeof(char16_t) * resultlength);
		udat_format(dat, d1, result1, resultlength, NULL, &status);
	}
	if(U_FAILURE(status)) {
		log_err("Error in formatting using udat_format(.....): %s\n", myErrorName(status));
		return 0;
	}
	return result1;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
