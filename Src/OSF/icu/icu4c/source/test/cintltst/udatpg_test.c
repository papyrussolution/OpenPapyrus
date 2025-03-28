// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 *
 *   Copyright (C) 2007-2016, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  udatpg_test.c
 *   encoding:   UTF-8
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2007aug01
 *   created by: Markus W. Scherer
 *
 *   Test of the C wrapper for the DateTimePatternGenerator.
 *   Calls each C API function and exercises code paths in the wrapper,
 *   but the full functionality is tested in the C++ intltest.
 *
 *   One item to note: C API functions which return a const char16_t *
 *   should return a NUL-terminated string.
 *   (The C++ implementation needs to use getTerminatedBuffer()
 *   on UnicodeString objects which end up being returned this way.)
 */
#include <icu-internal.h>
#pragma hdrstop
#if !UCONFIG_NO_FORMATTING
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "cintltst.h"

void addDateTimePatternGeneratorTest(TestNode** root);

#define TESTCASE(x) addTest(root, &x, "tsformat/udatpg_test/" #x)

static void TestOpenClose();
static void TestUsage();
static void TestBuilder();
static void TestOptions();
static void TestGetFieldDisplayNames();
static void TestGetDefaultHourCycle();
static void TestGetDefaultHourCycleOnEmptyInstance();
static void TestEras();

void addDateTimePatternGeneratorTest(TestNode** root) {
	TESTCASE(TestOpenClose);
	TESTCASE(TestUsage);
	TESTCASE(TestBuilder);
	TESTCASE(TestOptions);
	TESTCASE(TestGetFieldDisplayNames);
	TESTCASE(TestGetDefaultHourCycle);
	TESTCASE(TestGetDefaultHourCycleOnEmptyInstance);
	TESTCASE(TestEras);
}

/*
 * Pipe symbol '|'. We pass only the first char16_t without NUL-termination.
 * The second char16_t is just to verify that the API does not pick that up.
 */
static const char16_t pipeString[] = { 0x7c, 0x0a };

static const char16_t testSkeleton1[] = { 0x48, 0x48, 0x6d, 0x6d, 0 }; /* HHmm */
static const char16_t expectingBestPattern[] = { 0x48, 0x2e, 0x6d, 0x6d, 0 }; /* H.mm */
static const char16_t testPattern[] = { 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0 }; /* HH:mm */
static const char16_t expectingSkeleton[] = { 0x48, 0x48, 0x6d, 0x6d, 0 }; /* HHmm */
static const char16_t expectingBaseSkeleton[] = { 0x48, 0x6d, 0 }; /* HHmm */
static const char16_t redundantPattern[] = { 0x79, 0x79, 0x4d, 0x4d, 0x4d, 0 }; /* yyMMM */
static const char16_t testFormat[] = {0x7B, 0x31, 0x7D, 0x20, 0x7B, 0x30, 0x7D, 0};  /* {1} {0} */
static const char16_t appendItemName[] = {0x68, 0x72, 0};  /* hr */
static const char16_t testPattern2[] = { 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0x20, 0x76, 0 }; /* HH:mm v */
static const char16_t replacedStr[] = { 0x76, 0x76, 0x76, 0x76, 0 }; /* vvvv */
/* results for getBaseSkeletons() - {Hmv}, {yMMM} */
static const char16_t resultBaseSkeletons[2][10] = {{0x48, 0x6d, 0x76, 0}, {0x79, 0x4d, 0x4d, 0x4d, 0 } };
static const char16_t sampleFormatted[] = {0x31, 0x30, 0x20, 0x6A, 0x75, 0x69, 0x6C, 0x2E, 0}; /* 10 juil. */
static const char16_t skeleton[] = {0x4d, 0x4d, 0x4d, 0x64, 0};  /* MMMd */
static const char16_t timeZoneGMT[] = { 0x0047, 0x004d, 0x0054, 0x0000 };  /* "GMT" */

static void TestOpenClose() {
	UErrorCode errorCode = U_ZERO_ERROR;
	UDateTimePatternGenerator * dtpg, * dtpg2;
	const char16_t * s;
	int32_t length;

	/* Open a DateTimePatternGenerator for the default locale. */
	dtpg = udatpg_open(NULL, &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err_status(errorCode, "udatpg_open(NULL) failed - %s\n", u_errorName(errorCode));
		return;
	}
	udatpg_close(dtpg);

	/* Now one for German. */
	dtpg = udatpg_open("de", &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_open(de) failed - %s\n", u_errorName(errorCode));
		return;
	}

	/* Make some modification which we verify gets passed on to the clone. */
	udatpg_setDecimal(dtpg, pipeString, 1);

	/* Clone the generator. */
	dtpg2 = udatpg_clone(dtpg, &errorCode);
	if(U_FAILURE(errorCode) || dtpg2==NULL) {
		log_err("udatpg_clone() failed - %s\n", u_errorName(errorCode));
		return;
	}

	/* Verify that the clone has the custom decimal symbol. */
	s = udatpg_getDecimal(dtpg2, &length);
	if(s==pipeString || length!=1 || 0!=u_memcmp(s, pipeString, length) || s[length]!=0) {
		log_err("udatpg_getDecimal(cloned object) did not return the expected string\n");
		return;
	}

	udatpg_close(dtpg);
	udatpg_close(dtpg2);
}

typedef struct {
	UDateTimePatternField field;
	char16_t name[12];
} AppendItemNameData;

static const AppendItemNameData appendItemNameData[] = { /* for Finnish */
	{ UDATPG_YEAR_FIELD,    {0x0076, 0x0075, 0x006F, 0x0073, 0x0069, 0} },/* "vuosi" */
	{ UDATPG_MONTH_FIELD,   {0x006B, 0x0075, 0x0075, 0x006B, 0x0061, 0x0075, 0x0073, 0x0069, 0} },/* "kuukausi" */
	{ UDATPG_WEEKDAY_FIELD, {0x0076, 0x0069, 0x0069, 0x006B, 0x006F, 0x006E, 0x0070, 0x00E4, 0x0069, 0x0076, 0x00E4, 0} },
	{ UDATPG_DAY_FIELD,     {0x0070, 0x00E4, 0x0069, 0x0076, 0x00E4, 0} },
	{ UDATPG_HOUR_FIELD,    {0x0074, 0x0075, 0x006E, 0x0074, 0x0069, 0} },/* "tunti" */
	{ UDATPG_FIELD_COUNT,   {0}        }/* terminator */
};

static void TestUsage() {
	UErrorCode errorCode = U_ZERO_ERROR;
	UDateTimePatternGenerator * dtpg;
	const AppendItemNameData * appItemNameDataPtr;
	char16_t bestPattern[20];
	char16_t result[20];
	int32_t length;
	char16_t * s;
	const char16_t * r;

	dtpg = udatpg_open("fi", &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err_status(errorCode, "udatpg_open(fi) failed - %s\n", u_errorName(errorCode));
		return;
	}
	length = udatpg_getBestPattern(dtpg, testSkeleton1, 4,
		bestPattern, 20, &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_getBestPattern failed - %s\n", u_errorName(errorCode));
		return;
	}
	if((u_memcmp(bestPattern, expectingBestPattern, length)!=0) || bestPattern[length]!=0) {
		log_err("udatpg_getBestPattern did not return the expected string\n");
		return;
	}

	/* Test skeleton == NULL */
	s = NULL;
	length = udatpg_getBestPattern(dtpg, s, 0, bestPattern, 20, &errorCode);
	if(!U_FAILURE(errorCode)&&(length!=0)) {
		log_err("udatpg_getBestPattern failed in illegal argument - skeleton is NULL.\n");
		return;
	}

	/* Test udatpg_getSkeleton */
	length = udatpg_getSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_getSkeleton failed - %s\n", u_errorName(errorCode));
		return;
	}
	if((u_memcmp(result, expectingSkeleton, length)!=0) || result[length]!=0) {
		log_err("udatpg_getSkeleton did not return the expected string\n");
		return;
	}

	/* Test pattern == NULL */
	s = NULL;
	length = udatpg_getSkeleton(dtpg, s, 0, result, 20, &errorCode);
	if(!U_FAILURE(errorCode)&&(length!=0)) {
		log_err("udatpg_getSkeleton failed in illegal argument - pattern is NULL.\n");
		return;
	}

	/* Test udatpg_getBaseSkeleton */
	length = udatpg_getBaseSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_getBaseSkeleton failed - %s\n", u_errorName(errorCode));
		return;
	}
	if((u_memcmp(result, expectingBaseSkeleton, length)!=0) || result[length]!=0) {
		log_err("udatpg_getBaseSkeleton did not return the expected string\n");
		return;
	}

	/* Test pattern == NULL */
	s = NULL;
	length = udatpg_getBaseSkeleton(dtpg, s, 0, result, 20, &errorCode);
	if(!U_FAILURE(errorCode)&&(length!=0)) {
		log_err("udatpg_getBaseSkeleton failed in illegal argument - pattern is NULL.\n");
		return;
	}

	/* set append format to {1}{0} */
	udatpg_setAppendItemFormat(dtpg, UDATPG_MONTH_FIELD, testFormat, 7);
	r = udatpg_getAppendItemFormat(dtpg, UDATPG_MONTH_FIELD, &length);

	if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) {
		log_err("udatpg_setAppendItemFormat did not return the expected string\n");
		return;
	}

	for(appItemNameDataPtr = appendItemNameData; appItemNameDataPtr->field <  UDATPG_FIELD_COUNT; appItemNameDataPtr++) {
		int32_t nameLength;
		const char16_t * namePtr = udatpg_getAppendItemName(dtpg, appItemNameDataPtr->field, &nameLength);
		if(namePtr == NULL || u_strncmp(appItemNameDataPtr->name, namePtr, nameLength) != 0) {
			log_err("udatpg_getAppendItemName returns invalid name for field %d\n", (int)appItemNameDataPtr->field);
		}
	}

	/* set append name to hr */
	udatpg_setAppendItemName(dtpg, UDATPG_HOUR_FIELD, appendItemName, 2);
	r = udatpg_getAppendItemName(dtpg, UDATPG_HOUR_FIELD, &length);

	if(length!=2 || 0!=u_memcmp(r, appendItemName, length) || r[length]!=0) {
		log_err("udatpg_setAppendItemName did not return the expected string\n");
		return;
	}

	/* set date time format to {1}{0} */
	udatpg_setDateTimeFormat(dtpg, testFormat, 7);
	r = udatpg_getDateTimeFormat(dtpg, &length);

	if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) {
		log_err("udatpg_setDateTimeFormat did not return the expected string\n");
		return;
	}
	udatpg_close(dtpg);
}

static void TestBuilder() {
	UErrorCode errorCode = U_ZERO_ERROR;
	UDateTimePatternGenerator * dtpg;
	UDateTimePatternConflict conflict;
	UEnumeration * en;
	char16_t result[20];
	int32_t length, pLength;
	const char16_t * s, * p;
	const char16_t * ptrResult[2];
	int32_t count = 0;
	UDateTimePatternGenerator * generator;
	int32_t formattedCapacity, resultLen, patternCapacity;
	char16_t pattern[40], formatted[40];
	UDateFormat * formatter;
	UDate sampleDate = 837039928046.0;
	static const char locale[] = "fr";
	UErrorCode status = U_ZERO_ERROR;

	/* test create an empty DateTimePatternGenerator */
	dtpg = udatpg_openEmpty(&errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_openEmpty() failed - %s\n", u_errorName(errorCode));
		return;
	}

	/* Add a pattern */
	conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20,
		&length, &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("udatpg_addPattern() failed - %s\n", u_errorName(errorCode));
		return;
	}
	/* Add a redundant pattern */
	conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20,
		&length, &errorCode);
	if(conflict == UDATPG_NO_CONFLICT) {
		log_err("udatpg_addPattern() failed to find the duplicate pattern.\n");
		return;
	}
	/* Test pattern == NULL */
	s = NULL;
	length = udatpg_addPattern(dtpg, s, 0, FALSE, result, 20,
		&length, &errorCode);
	if(!U_FAILURE(errorCode)&&(length!=0)) {
		log_err("udatpg_addPattern failed in illegal argument - pattern is NULL.\n");
		return;
	}

	/* replace field type */
	errorCode = U_ZERO_ERROR;
	conflict = udatpg_addPattern(dtpg, testPattern2, 7, FALSE, result, 20,
		&length, &errorCode);
	if((conflict != UDATPG_NO_CONFLICT)||U_FAILURE(errorCode)) {
		log_err("udatpg_addPattern() failed to add HH:mm v. - %s\n", u_errorName(errorCode));
		return;
	}
	length = udatpg_replaceFieldTypes(dtpg, testPattern2, 7, replacedStr, 4,
		result, 20, &errorCode);
	if(U_FAILURE(errorCode) || (length==0)) {
		log_err("udatpg_replaceFieldTypes failed!\n");
		return;
	}

	/* Get all skeletons and the crroespong pattern for each skeleton. */
	ptrResult[0] = testPattern2;
	ptrResult[1] = redundantPattern;
	count = 0;
	en = udatpg_openSkeletons(dtpg, &errorCode);
	if(U_FAILURE(errorCode) || (length==0)) {
		log_err("udatpg_openSkeletons failed!\n");
		return;
	}
	while((s = uenum_unext(en, &length, &errorCode))!= NULL) {
		p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
		if(U_FAILURE(errorCode) || p==NULL || u_memcmp(p, ptrResult[count], pLength)!=0) {
			log_err("udatpg_getPatternForSkeleton failed!\n");
			return;
		}
		count++;
	}
	uenum_close(en);

	/* Get all baseSkeletons */
	en = udatpg_openBaseSkeletons(dtpg, &errorCode);
	count = 0;
	while((s = uenum_unext(en, &length, &errorCode))!= NULL) {
		p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
		if(U_FAILURE(errorCode) || p==NULL || u_memcmp(p, resultBaseSkeletons[count], pLength)!=0) {
			log_err("udatpg_getPatternForSkeleton failed!\n");
			return;
		}
		count++;
	}
	if(U_FAILURE(errorCode) || (length==0)) {
		log_err("udatpg_openSkeletons failed!\n");
		return;
	}
	uenum_close(en);

	udatpg_close(dtpg);

	/* sample code in Userguide */
	patternCapacity = SIZEOFARRAYi(pattern);
	status = U_ZERO_ERROR;
	generator = udatpg_open(locale, &status);
	if(U_FAILURE(status)) {
		return;
	}

	/* get a pattern for an abbreviated month and day */
	length = udatpg_getBestPattern(generator, skeleton, 4,
		pattern, patternCapacity, &status);
	formatter = udat_open(UDAT_PATTERN, UDAT_PATTERN, locale, timeZoneGMT, -1,
		pattern, length, &status);
	if(formatter==NULL) {
		log_err("Failed to initialize the UDateFormat of the sample code in Userguide.\n");
		udatpg_close(generator);
		return;
	}

	/* use it to format (or parse) */
	formattedCapacity = SIZEOFARRAYi(formatted);
	resultLen = udat_format(formatter, ucal_getNow(), formatted, formattedCapacity,
		NULL, &status);
	/* for French, the result is "13 sept." */

	/* cannot use the result from ucal_getNow() because the value change evreyday. */
	resultLen = udat_format(formatter, sampleDate, formatted, formattedCapacity,
		NULL, &status);
	if(u_memcmp(sampleFormatted, formatted, resultLen) != 0) {
		log_err("Failed udat_format() of sample code in Userguide.\n");
	}
	udatpg_close(generator);
	udat_close(formatter);
}

typedef struct DTPtnGenOptionsData {
	const char * locale;
	const char16_t * skel;
	UDateTimePatternMatchOptions options;
	const char16_t * expectedPattern;
} DTPtnGenOptionsData;
enum { kTestOptionsPatLenMax = 32 };

static const char16_t skel_Hmm[] = { 0x0048, 0x006D, 0x006D, 0 };
static const char16_t skel_HHmm[] = { 0x0048, 0x0048, 0x006D, 0x006D, 0 };
static const char16_t skel_hhmm[] = { 0x0068, 0x0068, 0x006D, 0x006D, 0 };
static const char16_t patn_hcmm_a[]  = { 0x0068, 0x003A, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; /* h:mm a */
static const char16_t patn_HHcmm[] = { 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0 }; /* HH:mm */
static const char16_t patn_hhcmm_a[] = { 0x0068, 0x0068, 0x003A, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; /* hh:mm a */
static const char16_t patn_HHpmm[] = { 0x0048, 0x0048, 0x002E, 0x006D, 0x006D, 0 }; /* HH.mm */
static const char16_t patn_hpmm_a[]  = { 0x0068, 0x002E, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; /* h.mm a */
static const char16_t patn_Hpmm[] = { 0x0048, 0x002E, 0x006D, 0x006D, 0 }; /* H.mm */
static const char16_t patn_hhpmm_a[] = { 0x0068, 0x0068, 0x002E, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; /* hh.mm a */

static void TestOptions() {
	const DTPtnGenOptionsData testData[] = {
		/*loc   skel       options                       expectedPattern */
		{ "en", skel_Hmm,  UDATPG_MATCH_NO_OPTIONS,        patn_HHcmm   },
		{ "en", skel_HHmm, UDATPG_MATCH_NO_OPTIONS,        patn_HHcmm   },
		{ "en", skel_hhmm, UDATPG_MATCH_NO_OPTIONS,        patn_hcmm_a  },
		{ "en", skel_Hmm,  UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHcmm   },
		{ "en", skel_HHmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHcmm   },
		{ "en", skel_hhmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_hhcmm_a },
		{ "da", skel_Hmm,  UDATPG_MATCH_NO_OPTIONS,        patn_HHpmm   },
		{ "da", skel_HHmm, UDATPG_MATCH_NO_OPTIONS,        patn_HHpmm   },
		{ "da", skel_hhmm, UDATPG_MATCH_NO_OPTIONS,        patn_hpmm_a  },
		{ "da", skel_Hmm,  UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_Hpmm    },
		{ "da", skel_HHmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHpmm   },
		{ "da", skel_hhmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_hhpmm_a },
	};

	int count = SIZEOFARRAYi(testData);
	const DTPtnGenOptionsData * testDataPtr = testData;

	for(; count-- > 0; ++testDataPtr) {
		UErrorCode status = U_ZERO_ERROR;
		UDateTimePatternGenerator * dtpgen = udatpg_open(testDataPtr->locale, &status);
		if(U_SUCCESS(status)) {
			char16_t pattern[kTestOptionsPatLenMax];
			int32_t patLen = udatpg_getBestPatternWithOptions(dtpgen, testDataPtr->skel, -1,
				testDataPtr->options, pattern,
				kTestOptionsPatLenMax, &status);
			if(U_FAILURE(status) || u_strncmp(pattern, testDataPtr->expectedPattern, patLen+1) != 0) {
				char skelBytes[kTestOptionsPatLenMax];
				char expectedPatternBytes[kTestOptionsPatLenMax];
				char patternBytes[kTestOptionsPatLenMax];
				log_err(
					"ERROR udatpg_getBestPatternWithOptions, locale %s, skeleton %s, options 0x%04X, expected pattern %s, got %s, status %d\n",
					testDataPtr->locale,
					u_austrncpy(skelBytes, testDataPtr->skel, kTestOptionsPatLenMax),
					testDataPtr->options,
					u_austrncpy(expectedPatternBytes, testDataPtr->expectedPattern, kTestOptionsPatLenMax),
					u_austrncpy(patternBytes, pattern, kTestOptionsPatLenMax),
					status);
			}
			udatpg_close(dtpgen);
		}
		else {
			log_data_err("ERROR udatpg_open failed for locale %s : %s - (Are you missing data?)\n",
			    testDataPtr->locale,
			    myErrorName(status));
		}
	}
}

typedef struct FieldDisplayNameData {
	const char * locale;
	UDateTimePatternField field;
	UDateTimePGDisplayWidth width;
	const char * expected;
} FieldDisplayNameData;
enum { kFieldDisplayNameMax = 32, kFieldDisplayNameBytesMax  = 64};

static void TestGetFieldDisplayNames() {
	const FieldDisplayNameData testData[] = {
		/*loc      field                              width               expectedName */
		{ "de",    UDATPG_QUARTER_FIELD,              UDATPG_WIDE,        "Quartal" },
		{ "de",    UDATPG_QUARTER_FIELD,              UDATPG_ABBREVIATED, "Quart." },
		{ "de",    UDATPG_QUARTER_FIELD,              UDATPG_NARROW,      "Q" },
		{ "en",    UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_WIDE,        "weekday of the month" },
		{ "en",    UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_ABBREVIATED, "wkday. of mo." },
		{ "en",    UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_NARROW,      "wkday. of mo." },// fallback
		{ "en_GB", UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_WIDE,        "weekday of the month" },
		{ "en_GB", UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_ABBREVIATED, "wkday of mo" }, // override
		{ "en_GB", UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, UDATPG_NARROW,      "wkday of mo" },
		{ "it",    UDATPG_SECOND_FIELD,               UDATPG_WIDE,        "secondo" },
		{ "it",    UDATPG_SECOND_FIELD,               UDATPG_ABBREVIATED, "s" },
		{ "it",    UDATPG_SECOND_FIELD,               UDATPG_NARROW,      "s" },
	};

	int count = SIZEOFARRAYi(testData);
	const FieldDisplayNameData * testDataPtr = testData;
	for(; count-- > 0; ++testDataPtr) {
		UErrorCode status = U_ZERO_ERROR;
		UDateTimePatternGenerator * dtpgen = udatpg_open(testDataPtr->locale, &status);
		if(U_FAILURE(status)) {
			log_data_err("ERROR udatpg_open failed for locale %s : %s - (Are you missing data?)\n",
			    testDataPtr->locale,
			    myErrorName(status));
		}
		else {
			char16_t expName[kFieldDisplayNameMax];
			char16_t getName[kFieldDisplayNameMax];
			u_unescape(testDataPtr->expected, expName, kFieldDisplayNameMax);

			int32_t getLen = udatpg_getFieldDisplayName(dtpgen, testDataPtr->field, testDataPtr->width,
				getName, kFieldDisplayNameMax, &status);
			if(U_FAILURE(status)) {
				log_err("ERROR udatpg_getFieldDisplayName locale %s field %d width %d, got status %s, len %d\n",
				    testDataPtr->locale, testDataPtr->field, testDataPtr->width, u_errorName(status), getLen);
			}
			else if(u_strncmp(expName, getName, kFieldDisplayNameMax) != 0) {
				char expNameB[kFieldDisplayNameBytesMax];
				char getNameB[kFieldDisplayNameBytesMax];
				log_err("ERROR udatpg_getFieldDisplayName locale %s field %d width %d, expected %s, got %s, status %s\n",
				    testDataPtr->locale, testDataPtr->field, testDataPtr->width,
				    u_austrncpy(expNameB, expName, kFieldDisplayNameBytesMax),
				    u_austrncpy(getNameB, getName, kFieldDisplayNameBytesMax), u_errorName(status));
			}
			else if(testDataPtr->width == UDATPG_WIDE && getLen > 1) {
				// test preflight & inadequate buffer
				int32_t getNewLen;
				status = U_ZERO_ERROR;
				getNewLen = udatpg_getFieldDisplayName(dtpgen, testDataPtr->field, UDATPG_WIDE, NULL, 0, &status);
				if(U_FAILURE(status) || getNewLen != getLen) {
					log_err(
						"ERROR udatpg_getFieldDisplayName locale %s field %d width %d, preflight expected len %d, got %d, status %s\n",
						testDataPtr->locale,
						testDataPtr->field,
						testDataPtr->width,
						getLen,
						getNewLen,
						u_errorName(status));
				}
				status = U_ZERO_ERROR;
				getNewLen = udatpg_getFieldDisplayName(dtpgen, testDataPtr->field, UDATPG_WIDE, getName, getLen-1, &status);
				if(status!=U_BUFFER_OVERFLOW_ERROR || getNewLen != getLen) {
					log_err(
						"ERROR udatpg_getFieldDisplayName locale %s field %d width %d, overflow expected len %d & BUFFER_OVERFLOW_ERROR, got %d & status %s\n",
						testDataPtr->locale,
						testDataPtr->field,
						testDataPtr->width,
						getLen,
						getNewLen,
						u_errorName(status));
				}
			}
			udatpg_close(dtpgen);
		}
	}
}

typedef struct HourCycleData {
	const char * locale;
	UDateFormatHourCycle expected;
} HourCycleData;

static void TestGetDefaultHourCycle() 
{
	const HourCycleData testData[] = {
		/*loc      expected */
		{ "ar_EG",    UDAT_HOUR_CYCLE_12 },
		{ "de_DE",    UDAT_HOUR_CYCLE_23 },
		{ "en_AU",    UDAT_HOUR_CYCLE_12 },
		{ "en_CA",    UDAT_HOUR_CYCLE_12 },
		{ "en_US",    UDAT_HOUR_CYCLE_12 },
		{ "es_ES",    UDAT_HOUR_CYCLE_23 },
		{ "fi",       UDAT_HOUR_CYCLE_23 },
		{ "fr",       UDAT_HOUR_CYCLE_23 },
		{ "ja_JP",    UDAT_HOUR_CYCLE_23 },
		{ "zh_CN",    UDAT_HOUR_CYCLE_23 },
		{ "zh_HK",    UDAT_HOUR_CYCLE_12 },
		{ "zh_TW",    UDAT_HOUR_CYCLE_12 },
		{ "ko_KR",    UDAT_HOUR_CYCLE_12 },
	};
	int count = SIZEOFARRAYi(testData);
	const HourCycleData * testDataPtr = testData;
	for(; count-- > 0; ++testDataPtr) {
		UErrorCode status = U_ZERO_ERROR;
		UDateTimePatternGenerator * dtpgen = udatpg_open(testDataPtr->locale, &status);
		if(U_FAILURE(status)) {
			log_data_err("ERROR udatpg_open failed for locale %s : %s - (Are you missing data?)\n", testDataPtr->locale, myErrorName(status));
		}
		else {
			UDateFormatHourCycle actual = udatpg_getDefaultHourCycle(dtpgen, &status);
			if(U_FAILURE(status) || testDataPtr->expected != actual) {
				log_err("ERROR dtpgen locale %s udatpg_getDefaultHourCycle expected to get %d but get %d\n", testDataPtr->locale, testDataPtr->expected, actual);
			}
			udatpg_close(dtpgen);
		}
	}
}

// Ensure that calling udatpg_getDefaultHourCycle on an empty instance doesn't call UPRV_UNREACHABLE_EXIT/abort.
static void TestGetDefaultHourCycleOnEmptyInstance() 
{
	UErrorCode status = U_ZERO_ERROR;
	UDateTimePatternGenerator * dtpgen = udatpg_openEmpty(&status);
	if(U_FAILURE(status)) {
		log_data_err("ERROR udatpg_openEmpty failed, status: %s \n", myErrorName(status));
		return;
	}
	(void)udatpg_getDefaultHourCycle(dtpgen, &status);
	if(!U_FAILURE(status)) {
		log_data_err("ERROR expected udatpg_getDefaultHourCycle on an empty instance to fail, status: %s", myErrorName(status));
	}
	status = U_USELESS_COLLATOR_ERROR;
	(void)udatpg_getDefaultHourCycle(dtpgen, &status);
	if(status != U_USELESS_COLLATOR_ERROR) {
		log_data_err("ERROR udatpg_getDefaultHourCycle shouldn't modify status if it is already failed, status: %s", myErrorName(status));
	}
	udatpg_close(dtpgen);
}

// Test for ICU-21202: Make sure DateTimePatternGenerator supplies an era field for year formats using the
// Buddhist and Japanese calendars for all English-speaking locales.
static void TestEras() {
	const char * localeIDs[] = {
		"en_US@calendar=japanese",
		"en_GB@calendar=japanese",
		"en_150@calendar=japanese",
		"en_001@calendar=japanese",
		"en@calendar=japanese",
		"en_US@calendar=buddhist",
		"en_GB@calendar=buddhist",
		"en_150@calendar=buddhist",
		"en_001@calendar=buddhist",
		"en@calendar=buddhist",
	};
	UErrorCode err = U_ZERO_ERROR;
	for(int32_t i = 0; i < SIZEOFARRAYi(localeIDs); i++) {
		const char * locale = localeIDs[i];
		UDateTimePatternGenerator* dtpg = udatpg_open(locale, &err);
		if(U_SUCCESS(err)) {
			char16_t pattern[200];
			udatpg_getBestPattern(dtpg, u"y", 1, pattern, 200, &err);
			if(u_strchr(pattern, u'G') == NULL) {
				log_err("missing era field for locale %s\n", locale);
			}
		}
		udatpg_close(dtpg);
	}
}

#endif
