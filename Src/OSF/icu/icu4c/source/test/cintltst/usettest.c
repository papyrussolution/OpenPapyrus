// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 * Copyright (c) 2002-2016, International Business Machines Corporation and others.  All Rights Reserved.
 */
#include <icu-internal.h>
#pragma hdrstop
#include "cintltst.h"

#define TEST(x) addTest(root, &x, "uset/" # x)

static void TestAPI();
static void Testj2269();
static void TestSerialized();
static void TestNonInvariantPattern();
static void TestBadPattern();
static void TestFreezable();
static void TestSpan();

void addUSetTest(TestNode** root);

static void expect(const USet* set,
    const char * inList,
    const char * outList,
    UErrorCode * ec);
static void expectContainment(const USet* set,
    const char * list,
    bool isIn);
static char oneUCharToChar(UChar32 c);
static void expectItems(const USet* set,
    const char * items);

void addUSetTest(TestNode** root) {
	TEST(TestAPI);
	TEST(Testj2269);
	TEST(TestSerialized);
	TEST(TestNonInvariantPattern);
	TEST(TestBadPattern);
	TEST(TestFreezable);
	TEST(TestSpan);
}

/*------------------------------------------------------------------
 * Tests
 *------------------------------------------------------------------*/

static void Testj2269() {
	UErrorCode status = U_ZERO_ERROR;
	char16_t a[4] = { 0x61, 0x62, 0x63, 0 };
	USet * s = uset_open(1, 0);
	uset_addString(s, a, 3);
	a[0] = 0x63; a[1] = 0x63;
	expect(s, "{abc}", "{ccc}", &status);
	uset_close(s);
}

static const char16_t PAT[] = {91, 97, 45, 99, 123, 97, 98, 125, 93, 0}; /* "[a-c{ab}]" */
static const int32_t PAT_LEN = SIZEOFARRAYi(PAT) - 1;

static const char16_t PAT_lb[] = {0x6C, 0x62, 0}; /* "lb" */
static const int32_t PAT_lb_LEN = SIZEOFARRAYi(PAT_lb) - 1;

static const char16_t VAL_SP[] = {0x53, 0x50, 0}; /* "SP" */
static const int32_t VAL_SP_LEN = SIZEOFARRAYi(VAL_SP) - 1;

static const char16_t STR_bc[] = {98, 99, 0}; /* "bc" */
static const int32_t STR_bc_LEN = SIZEOFARRAYi(STR_bc) - 1;

static const char16_t STR_ab[] = {97, 98, 0}; /* "ab" */
static const int32_t STR_ab_LEN = SIZEOFARRAYi(STR_ab) - 1;

/**
 * Basic API test for uset.x
 */
static void TestAPI() {
	USet* set;
	USet* set2;
	UErrorCode ec;

	/* [] */
	set = uset_openEmpty();
	expect(set, "", "abc{ab}", NULL);
	uset_close(set);

	set = uset_open(1, 0);
	expect(set, "", "abc{ab}", NULL);
	uset_close(set);

	set = uset_open(1, 1);
	uset_clear(set);
	expect(set, "", "abc{ab}", NULL);
	uset_close(set);

	/* [ABC] */
	set = uset_open(0x0041, 0x0043);
	expect(set, "ABC", "DEF{ab}", NULL);
	if(uset_hasStrings(set)) {
		log_err("uset_hasStrings([ABC]) = true");
	}
	uset_close(set);

	/* [a-c{ab}] */
	ec = U_ZERO_ERROR;
	set = uset_openPattern(PAT, PAT_LEN, &ec);
	if(U_FAILURE(ec)) {
		log_err("uset_openPattern([a-c{ab}]) failed - %s\n", u_errorName(ec));
		return;
	}
	if(!uset_resemblesPattern(PAT, PAT_LEN, 0)) {
		log_err("uset_resemblesPattern of PAT failed\n");
	}
	if(!uset_hasStrings(set)) {
		log_err("uset_hasStrings([a-c{ab}]) = false");
	}
	expect(set, "abc{ab}", "def{bc}", &ec);

	/* [a-d{ab}] */
	uset_add(set, 0x64);
	expect(set, "abcd{ab}", "ef{bc}", NULL);

	/* [acd{ab}{bc}] */
	uset_remove(set, 0x62);
	uset_addString(set, STR_bc, STR_bc_LEN);
	expect(set, "acd{ab}{bc}", "bef{cd}", NULL);

	/* [acd{bc}] */
	uset_removeString(set, STR_ab, STR_ab_LEN);
	expect(set, "acd{bc}", "bfg{ab}", NULL);

	/* [^acd{bc}] */
	uset_complement(set);
	expect(set, "bef{bc}", "acd{ac}", NULL);

	/* [a-e{bc}] */
	uset_complement(set);
	uset_addRange(set, 0x0062, 0x0065);
	expect(set, "abcde{bc}", "fg{ab}", NULL);

	/* [de{bc}] */
	uset_removeRange(set, 0x0050, 0x0063);
	expect(set, "de{bc}", "bcfg{ab}", NULL);

	/* [g-l] */
	uset_set(set, 0x0067, 0x006C);
	expect(set, "ghijkl", "de{bc}", NULL);

	if(uset_indexOf(set, 0x0067) != 0) {
		log_err("uset_indexOf failed finding correct index of 'g'\n");
	}

	if(uset_charAt(set, 0) != 0x0067) {
		log_err("uset_charAt failed finding correct char 'g' at index 0\n");
	}

	/* How to test this one...? */
	uset_compact(set);

	/* [g-i] */
	uset_retain(set, 0x0067, 0x0069);
	expect(set, "ghi", "dejkl{bc}", NULL);

	/* UCHAR_ASCII_HEX_DIGIT */
	uset_applyIntPropertyValue(set, UCHAR_ASCII_HEX_DIGIT, 1, &ec);
	if(U_FAILURE(ec)) {
		log_err("uset_applyIntPropertyValue([UCHAR_ASCII_HEX_DIGIT]) failed - %s\n", u_errorName(ec));
		return;
	}
	expect(set, "0123456789ABCDEFabcdef", "GHIjkl{bc}", NULL);
	if(uset_size(set) != 22 || uset_getRangeCount(set) != 3 || uset_getItemCount(set) != 3) {
		log_err("line %d: uset_size()/uset_getRangeCount()/uset_getItemCount() wrong", __LINE__);
	}

	/* [ab] */
	uset_clear(set);
	uset_addAllCodePoints(set, STR_ab, STR_ab_LEN);
	expect(set, "ab", "def{ab}", NULL);
	if(uset_containsAllCodePoints(set, STR_bc, STR_bc_LEN)) {
		log_err("set should not contain all characters of \"bc\" \n");
	}

	/* [] */
	set2 = uset_open(1, 1);
	uset_clear(set2);

	/* space */
	uset_applyPropertyAlias(set2, PAT_lb, PAT_lb_LEN, VAL_SP, VAL_SP_LEN, &ec);
	expect(set2, " ", "abcdefghi{bc}", NULL);

	/* [a-c] */
	uset_set(set2, 0x0061, 0x0063);
	/* [g-i] */
	uset_set(set, 0x0067, 0x0069);

	/* [a-c g-i] */
	if(uset_containsSome(set, set2)) {
		log_err("set should not contain some of set2 yet\n");
	}
	uset_complementAll(set, set2);
	if(!uset_containsSome(set, set2)) {
		log_err("set should contain some of set2\n");
	}
	expect(set, "abcghi", "def{bc}", NULL);

	/* [g-i] */
	uset_removeAll(set, set2);
	expect(set, "ghi", "abcdef{bc}", NULL);

	/* [a-c g-i] */
	uset_addAll(set2, set);
	expect(set2, "abcghi", "def{bc}", NULL);
	/* [g-i] */
	uset_retainAll(set2, set);
	expect(set2, "ghi", "abcdef{bc}", NULL);
	// ICU 69 added some missing functions for parity with C++ and Java.
	uset_applyPattern(set, u"[abcdef{ch}{sch}]", -1, 0, &ec);
	if(U_FAILURE(ec)) {
		log_err("uset_openPattern([abcdef{ch}{sch}]) failed - %s\n", u_errorName(ec));
		return;
	}
	expect(set, "abcdef{ch}{sch}", "", NULL);
	uset_removeAllCodePoints(set, u"ce", 2);
	expect(set, "abdf{ch}{sch}", "ce", NULL);
	uset_complementRange(set, u'b', u'f');
	expect(set, "ace{ch}{sch}", "bdf", NULL);
	uset_complementString(set, u"ch", -1);
	expect(set, "ace{sch}", "bdf{ch}", NULL);
	uset_complementString(set, u"xy", -1);
	expect(set, "ace{sch}{xy}", "bdf{ch}", NULL);
	uset_complementAllCodePoints(set, u"abef", 4);
	expect(set, "bcf{sch}{xy}", "ade{ch}", NULL);
	uset_retainAllCodePoints(set, u"abef", -1);
	expect(set, "bf", "acde{ch}{sch}{xy}", NULL);
	uset_applyPattern(set, u"[abcdef{ch}{sch}]", -1, 0, &ec);
	if(U_FAILURE(ec)) {
		log_err("uset_openPattern([abcdef{ch}{sch}]) failed - %s\n", u_errorName(ec));
		return;
	}
	expect(set, "abcdef{ch}{sch}", "", NULL);
	if(uset_size(set) != 8 || uset_getRangeCount(set) != 1 || uset_getItemCount(set) != 3) {
		log_err("line %d: uset_size()/uset_getRangeCount()/uset_getItemCount() wrong", __LINE__);
	}
	uset_retainString(set, u"sch", 3);
	expect(set, "{sch}", "abcdef{ch}", NULL);
	uset_retainString(set, u"ch", 3);
	expect(set, "", "abcdef{ch}{sch}", NULL);
	uset_close(set);
	uset_close(set2);
}

/*------------------------------------------------------------------
 * Support
 *------------------------------------------------------------------*/

/**
 * Verifies that the given set contains the characters and strings in
 * inList, and does not contain those in outList.  Also verifies that
 * 'set' is not NULL and that 'ec' succeeds.
 * @param set the set to test, or NULL (on error)
 * @param inList list of set contents, in iteration order.  Format is
 * list of individual strings, in iteration order, followed by sorted
 * list of strings, delimited by {}.  This means we do not test
 * characters '{' or '}' and we do not test strings containing those
 * characters either.
 * @param outList list of things not in the set.  Same format as
 * inList.
 * @param ec an error code, checked for success.  May be NULL in which
 * case it is ignored.
 */
static void expect(const USet* set, const char * inList, const char * outList, UErrorCode * ec) 
{
	if(ec!=NULL && U_FAILURE(*ec)) {
		log_err("FAIL: %s\n", u_errorName(*ec));
		return;
	}
	if(set == NULL) {
		log_err("FAIL: USet is NULL\n");
		return;
	}
	expectContainment(set, inList, TRUE);
	expectContainment(set, outList, FALSE);
	expectItems(set, inList);
}

static void expectContainment(const USet* set, const char * list, bool isIn) 
{
	const char * p = list;
	char16_t ustr[4096];
	char * pat;
	UErrorCode ec;
	int32_t rangeStart = -1, rangeEnd = -1, length;

	ec = U_ZERO_ERROR;
	length = uset_toPattern(set, ustr, sizeof(ustr), TRUE, &ec);
	if(U_FAILURE(ec)) {
		log_err("FAIL: uset_toPattern() fails in expectContainment() - %s\n", u_errorName(ec));
		return;
	}
	pat = aescstrdup(ustr, length);

	while(*p) {
		if(*p=='{') {
			const char * stringStart = ++p;
			int32_t stringLength = 0;
			char strCopy[64];

			while(*p++ != '}') {
			}
			stringLength = (int32_t)(p - stringStart - 1);
			strncpy(strCopy, stringStart, stringLength);
			strCopy[stringLength] = 0;

			u_charsToUChars(stringStart, ustr, stringLength);

			if(uset_containsString(set, ustr, stringLength) == isIn) {
				log_verbose("Ok: %s %s \"%s\"\n", pat,
				    (isIn ? "contains" : "does not contain"),
				    strCopy);
			}
			else {
				log_data_err("FAIL: %s %s \"%s\" (Are you missing data?)\n", pat,
				    (isIn ? "does not contain" : "contains"),
				    strCopy);
			}
		}

		else {
			UChar32 c;

			u_charsToUChars(p, ustr, 1);
			c = ustr[0];

			if(uset_contains(set, c) == isIn) {
				log_verbose("Ok: %s %s '%c'\n", pat,
				    (isIn ? "contains" : "does not contain"),
				    *p);
			}
			else {
				log_data_err("FAIL: %s %s '%c' (Are you missing data?)\n", pat,
				    (isIn ? "does not contain" : "contains"),
				    *p);
			}

			/* Test the range API too by looking for ranges */
			if(c == rangeEnd+1) {
				rangeEnd = c;
			}
			else {
				if(rangeStart >= 0) {
					if(uset_containsRange(set, rangeStart, rangeEnd) == isIn) {
						log_verbose("Ok: %s %s U+%04X-U+%04X\n", pat,
						    (isIn ? "contains" : "does not contain"),
						    rangeStart, rangeEnd);
					}
					else {
						log_data_err("FAIL: %s %s U+%04X-U+%04X (Are you missing data?)\n", pat,
						    (isIn ? "does not contain" : "contains"),
						    rangeStart, rangeEnd);
					}
				}
				rangeStart = rangeEnd = c;
			}

			++p;
		}
	}

	if(rangeStart >= 0) {
		if(uset_containsRange(set, rangeStart, rangeEnd) == isIn) {
			log_verbose("Ok: %s %s U+%04X-U+%04X\n", pat,
			    (isIn ? "contains" : "does not contain"),
			    rangeStart, rangeEnd);
		}
		else {
			log_data_err("FAIL: %s %s U+%04X-U+%04X (Are you missing data?)\n", pat,
			    (isIn ? "does not contain" : "contains"),
			    rangeStart, rangeEnd);
		}
	}
}

/* This only works for invariant BMP chars */
static char oneUCharToChar(UChar32 c) {
	char16_t ubuf[1];
	char buf[1];
	ubuf[0] = (char16_t)c;
	u_UCharsToChars(ubuf, buf, 1);
	return buf[0];
}

static void expectItems(const USet* set,
    const char * items) {
	const char * p = items;
	char16_t ustr[4096], itemStr[4096];
	char buf[4096];
	char * pat;
	UErrorCode ec;
	int32_t expectedSize = 0;
	int32_t rangeCount = uset_getRangeCount(set);
	int32_t itemCount = uset_getItemCount(set);
	int32_t itemIndex = 0;
	UChar32 start = 1, end = 0;
	int32_t itemLen = 0, length;
	bool isString = false;

	ec = U_ZERO_ERROR;
	length = uset_toPattern(set, ustr, sizeof(ustr), TRUE, &ec);
	if(U_FAILURE(ec)) {
		log_err("FAIL: uset_toPattern => %s\n", u_errorName(ec));
		return;
	}
	pat = aescstrdup(ustr, length);

	if(uset_isEmpty(set) != (strlen(items)==0)) {
		log_data_err("FAIL: %s should return %s from isEmpty (Are you missing data?)\n",
		    pat,
		    strlen(items)==0 ? "TRUE" : "FALSE");
	}

	/* Don't test patterns starting with "[^" */
	if(u_strlen(ustr) > 2 && ustr[1] == 0x5e /*'^'*/) {
		return;
	}

	while(*p) {
		++expectedSize;

		if(start > end || start == -1) {
			/* Fetch our next item */
			if(itemIndex >= itemCount) {
				log_data_err("FAIL: ran out of items iterating %s (Are you missing data?)\n", pat);
				return;
			}

			// Pass in NULL pointers where we expect them to be ok.
			if(itemIndex < rangeCount) {
				itemLen = uset_getItem(set, itemIndex, &start, &end, NULL, 0, &ec);
			}
			else {
				itemLen = uset_getItem(set, itemIndex, NULL, NULL,
					itemStr, SIZEOFARRAYi(itemStr), &ec);
				isString = true;
			}
			if(U_FAILURE(ec) || itemLen < 0) {
				log_err("FAIL: uset_getItem => %s\n", u_errorName(ec));
				return;
			}

			if(!isString) {
				log_verbose("Ok: %s item %d is %c-%c\n", pat,
				    itemIndex, oneUCharToChar(start),
				    oneUCharToChar(end));
				if(itemLen != 0) {
					log_err("FAIL: uset_getItem(%d) => length %d\n", itemIndex, itemLen);
				}
			}
			else {
				itemStr[itemLen] = 0;
				u_UCharsToChars(itemStr, buf, itemLen+1);
				log_verbose("Ok: %s item %d is \"%s\"\n", pat, itemIndex, buf);
			}

			++itemIndex;
		}

		if(*p=='{') {
			const char * stringStart = ++p;
			int32_t stringLength = 0;
			char strCopy[64];

			while(*p++ != '}') {
			}
			stringLength = (int32_t)(p - stringStart - 1);
			strncpy(strCopy, stringStart, stringLength);
			strCopy[stringLength] = 0;

			u_charsToUChars(stringStart, ustr, stringLength);
			ustr[stringLength] = 0;

			if(!isString) {
				log_err("FAIL: for %s expect \"%s\" next, but got a char\n",
				    pat, strCopy);
				return;
			}

			if(u_strcmp(ustr, itemStr) != 0) {
				log_err("FAIL: for %s expect \"%s\" next\n",
				    pat, strCopy);
				return;
			}
		}
		else {
			UChar32 c;
			u_charsToUChars(p, ustr, 1);
			c = ustr[0];
			if(isString) {
				log_err("FAIL: for %s expect '%c' next, but got a string\n",
				    pat, *p);
				return;
			}
			if(c != start) {
				log_err("FAIL: for %s expect '%c' next\n",
				    pat, *p);
				return;
			}
			++start;
			++p;
		}
	}
	if(uset_size(set) == expectedSize) {
		log_verbose("Ok: %s size is %d\n", pat, expectedSize);
	}
	else {
		log_err("FAIL: %s size is %d, expected %d\n",
		    pat, uset_size(set), expectedSize);
	}
}

static void TestSerialized() {
	uint16_t buffer[1000];
	USerializedSet sset;
	USet * set;
	UErrorCode errorCode;
	UChar32 c;
	int32_t length;

	/* use a pattern that generates both BMP and supplementary code points */
	U_STRING_DECL(pattern, "[:Cf:]", 6);
	U_STRING_INIT(pattern, "[:Cf:]", 6);

	errorCode = U_ZERO_ERROR;
	set = uset_openPattern(pattern, -1, &errorCode);
	if(U_FAILURE(errorCode)) {
		log_data_err("uset_openPattern([:Cf:]) failed - %s (Are you missing data?)\n", u_errorName(errorCode));
		return;
	}

	length = uset_serialize(set, buffer, SIZEOFARRAYi(buffer), &errorCode);
	if(U_FAILURE(errorCode)) {
		log_err("unable to uset_serialize([:Cf:]) - %s\n", u_errorName(errorCode));
		uset_close(set);
		return;
	}

	uset_getSerializedSet(&sset, buffer, length);
	for(c = 0; c<=0x10ffff; ++c) {
		if(uset_contains(set, c)!=uset_serializedContains(&sset, c)) {
			log_err("uset_contains(U+%04x)!=uset_serializedContains(U+%04x)\n", c);
			break;
		}
	}

	uset_close(set);
}

/**
 * Make sure that when non-invariant chars are passed to uset_openPattern
 * they do not cause an ugly failure mode (e.g. assertion failure).
 * JB#3795.
 */
static void TestNonInvariantPattern() {
	UErrorCode ec = U_ZERO_ERROR;
	/* The critical part of this test is that the following pattern
	   must contain a non-invariant character. */
	static const char * pattern = "[:ccc!=0:]";
	char16_t buf[256];
	int32_t len = u_unescape(pattern, buf, 256);
	/* This test 'fails' by having an assertion failure within the
	   following call.  It passes by running to completion with no
	   assertion failure. */
	USet * set = uset_openPattern(buf, len, &ec);
	uset_close(set);
}

static void TestBadPattern() {
	UErrorCode status = U_ZERO_ERROR;
	USet * pat;
	U_STRING_DECL(pattern, "[", 1);
	U_STRING_INIT(pattern, "[", 1);
	pat = uset_openPatternOptions(pattern, u_strlen(pattern), 0, &status);
	if(pat != NULL || U_SUCCESS(status)) {
		log_err("uset_openPatternOptions did not fail as expected %s\n", u_errorName(status));
	}
}

static USet * openIDSet() {
	UErrorCode errorCode = U_ZERO_ERROR;
	U_STRING_DECL(pattern, "[:ID_Continue:]", 15);
	U_STRING_INIT(pattern, "[:ID_Continue:]", 15);
	return uset_openPattern(pattern, 15, &errorCode);
}

static void TestFreezable() {
	USet * idSet;
	USet * frozen;
	USet * thawed;

	idSet = openIDSet();

	if(idSet == NULL) {
		log_data_err("openIDSet() returned NULL. (Are you missing data?)\n");
		uset_close(idSet);
		return;
	}

	frozen = uset_clone(idSet);

	if(frozen == NULL) {
		log_err("uset_Clone() returned NULL\n");
		return;
	}

	if(!uset_equals(frozen, idSet)) {
		log_err("uset_clone() did not make an equal copy\n");
	}

	uset_freeze(frozen);
	uset_addRange(frozen, 0xd802, 0xd805);

	if(uset_isFrozen(idSet) || !uset_isFrozen(frozen) || !uset_equals(frozen, idSet)) {
		log_err("uset_freeze() or uset_isFrozen() does not work\n");
	}

	thawed = uset_cloneAsThawed(frozen);

	if(thawed == NULL) {
		log_err("uset_cloneAsThawed(frozen) returned NULL");
		uset_close(frozen);
		uset_close(idSet);
		return;
	}

	uset_addRange(thawed, 0xd802, 0xd805);

	if(uset_isFrozen(thawed) || uset_equals(thawed, idSet) || !uset_containsRange(thawed, 0xd802, 0xd805)) {
		log_err("uset_cloneAsThawed() does not work\n");
	}

	uset_close(idSet);
	uset_close(frozen);
	uset_close(thawed);
}

static void TestSpan() {
	static const char16_t s16[2] = { 0xe01, 0x3000 };
	static const char * s8 = "\xE0\xB8\x81\xE3\x80\x80";

	USet * idSet = openIDSet();

	if(idSet == NULL) {
		log_data_err("openIDSet() returned NULL (Are you missing data?)\n");
		return;
	}

	if(
		1!=uset_span(idSet, s16, 2, USET_SPAN_CONTAINED) ||
		0!=uset_span(idSet, s16, 2, USET_SPAN_NOT_CONTAINED) ||
		2!=uset_spanBack(idSet, s16, 2, USET_SPAN_CONTAINED) ||
		1!=uset_spanBack(idSet, s16, 2, USET_SPAN_NOT_CONTAINED)
		) {
		log_err("uset_span() or uset_spanBack() does not work\n");
	}

	if(
		3!=uset_spanUTF8(idSet, s8, 6, USET_SPAN_CONTAINED) ||
		0!=uset_spanUTF8(idSet, s8, 6, USET_SPAN_NOT_CONTAINED) ||
		6!=uset_spanBackUTF8(idSet, s8, 6, USET_SPAN_CONTAINED) ||
		3!=uset_spanBackUTF8(idSet, s8, 6, USET_SPAN_NOT_CONTAINED)
		) {
		log_err("uset_spanUTF8() or uset_spanBackUTF8() does not work\n");
	}

	uset_freeze(idSet);

	if(
		1!=uset_span(idSet, s16, 2, USET_SPAN_CONTAINED) ||
		0!=uset_span(idSet, s16, 2, USET_SPAN_NOT_CONTAINED) ||
		2!=uset_spanBack(idSet, s16, 2, USET_SPAN_CONTAINED) ||
		1!=uset_spanBack(idSet, s16, 2, USET_SPAN_NOT_CONTAINED)
		) {
		log_err("uset_span(frozen) or uset_spanBack(frozen) does not work\n");
	}

	if(
		3!=uset_spanUTF8(idSet, s8, 6, USET_SPAN_CONTAINED) ||
		0!=uset_spanUTF8(idSet, s8, 6, USET_SPAN_NOT_CONTAINED) ||
		6!=uset_spanBackUTF8(idSet, s8, 6, USET_SPAN_CONTAINED) ||
		3!=uset_spanBackUTF8(idSet, s8, 6, USET_SPAN_NOT_CONTAINED)
		) {
		log_err("uset_spanUTF8(frozen) or uset_spanBackUTF8(frozen) does not work\n");
	}

	uset_close(idSet);
}

/*eof*/
