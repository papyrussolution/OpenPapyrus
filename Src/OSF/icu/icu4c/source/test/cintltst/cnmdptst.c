// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
* COPYRIGHT:
* Copyright (c) 1997-2016, International Business Machines Corporation
* and others. All Rights Reserved.
********************************************************************/
/*******************************************************************************
 *
 * File CNMDPTST.C
 *
 *  Madhu Katragadda                       Creation
 * Modification History:
 *
 *   Date        Name        Description
 *   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
 *******************************************************************************
 */

/* C DEPTH TEST FOR NUMBER FORMAT */

#include <icu-internal.h>
#pragma hdrstop
#if !UCONFIG_NO_FORMATTING

#include "unicode/ucurr.h"
#include "unicode/unum.h"
#include "cintltst.h"
#include "cnmdptst.h"
#include "ulist.h"

#define CHECK(status, str) if(U_FAILURE(status)) { log_err("FAIL: %s\n", str); return; }

void addNumFrDepTest(TestNode** root);
static void TestCurrencyObject();

void addNumFrDepTest(TestNode** root)
{
	addTest(root, &TestPatterns, "tsformat/cnmdptst/TestPatterns");
	addTest(root, &TestQuotes, "tsformat/cnmdptst/TestQuotes");
	addTest(root, &TestExponential, "tsformat/cnmdptst/TestExponential");
	addTest(root, &TestCurrencySign, "tsformat/cnmdptst/TestCurrencySign");
	addTest(root, &TestCurrency,  "tsformat/cnmdptst/TestCurrency");
	addTest(root, &TestCurrencyObject,  "tsformat/cnmdptst/TestCurrencyObject");
	addTest(root, &TestRounding487, "tsformat/cnmdptst/TestRounding487");
	addTest(root, &TestDoubleAttribute, "tsformat/cnmdptst/TestDoubleAttribute");
	addTest(root, &TestSecondaryGrouping, "tsformat/cnmdptst/TestSecondaryGrouping");
	addTest(root, &TestCurrencyKeywords, "tsformat/cnmdptst/TestCurrencyKeywords");
	addTest(root, &TestRounding5350, "tsformat/cnmdptst/TestRounding5350");
	addTest(root, &TestGetKeywordValuesForLocale, "tsformat/cnmdptst/TestGetKeywordValuesForLocale");
}

/*Test Various format patterns*/
static void TestPatterns(void)
{
	int32_t pat_length, i, lneed;
	UNumberFormat * fmt;
	char16_t upat[5];
	char16_t unewpat[5];
	char16_t unum[5];
	char16_t * unewp = NULL;
	char16_t * str = NULL;
	UErrorCode status = U_ZERO_ERROR;
	const char * pat[] = { "#.#", "#.", ".#", "#" };
	const char * newpat[] = { "0.#", "0.", "#.0", "0" };
	const char * num[] = { "0",   "0.", ".0", "0" };

	log_verbose("\nTesting different format patterns\n");
	pat_length = SIZEOFARRAYi(pat);
	for(i = 0; i < pat_length; ++i) {
		status = U_ZERO_ERROR;
		u_uastrcpy(upat, pat[i]);
		fmt = unum_open(UNUM_IGNORE, upat, u_strlen(upat), "en_US", NULL, &status);
		if(U_FAILURE(status)) {
			log_err_status(status, "FAIL: Number format constructor failed for pattern %s -> %s\n", pat[i],
			    u_errorName(status));
			continue;
		}
		lneed = 0;
		lneed = unum_toPattern(fmt, FALSE, NULL, lneed, &status);
		if(status==U_BUFFER_OVERFLOW_ERROR) {
			status = U_ZERO_ERROR;
			unewp = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
			unum_toPattern(fmt, FALSE, unewp, lneed+1, &status);
		}
		if(U_FAILURE(status)) {
			log_err("FAIL: Number format extracting the pattern failed for %s\n", pat[i]);
		}
		u_uastrcpy(unewpat, newpat[i]);
		if(u_strcmp(unewp, unewpat) != 0)
			log_err("FAIL: Pattern  %s should be transmuted to %s; %s seen instead\n", pat[i], newpat[i],  austrdup(unewp));

		lneed = 0;
		lneed = unum_format(fmt, 0, NULL, lneed, NULL, &status);
		if(status==U_BUFFER_OVERFLOW_ERROR) {
			status = U_ZERO_ERROR;
			str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
			unum_format(fmt, 0, str, lneed+1,  NULL, &status);
		}
		if(U_FAILURE(status)) {
			log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
		}
		u_uastrcpy(unum, num[i]);
		if(u_strcmp(str, unum) != 0) {
			log_err("FAIL: Pattern %s should format zero as %s; %s Seen instead\n", pat[i], num[i], austrdup(str));
		}
		SAlloc::F(unewp);
		SAlloc::F(str);
		unum_close(fmt);
	}
}

/* Test the handling of quotes*/
static void TestQuotes(void)
{
	int32_t lneed;
	UErrorCode status = U_ZERO_ERROR;
	char16_t pat[15];
	char16_t res[15];
	char16_t * str = NULL;
	UNumberFormat * fmt;
	char tempBuf[256];
	log_verbose("\nTestting the handling of quotes in number format\n");
	u_uastrcpy(pat, "a'fo''o'b#");
	fmt = unum_open(UNUM_IGNORE, pat, u_strlen(pat), "en_US", NULL, &status);
	if(U_FAILURE(status)) {
		log_err_status(status, "Error in number format construction using pattern \"a'fo''o'b#\" -> %s\n", u_errorName(status));
	}
	lneed = 0;
	lneed = unum_format(fmt, 123, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		unum_format(fmt, 123, str, lneed+1,  NULL, &status);
	}
	if(U_FAILURE(status) || !str) {
		log_err_status(status, "Error in formatting using unum_format(.....): %s\n", myErrorName(status));
		return;
	}
	log_verbose("Pattern \"%s\" \n", u_austrcpy(tempBuf, pat));
	log_verbose("Format 123 -> %s\n", u_austrcpy(tempBuf, str));
	u_uastrcpy(res, "afo'ob123");
	if(u_strcmp(str, res) != 0)
		log_err("FAIL: Expected afo'ob123");

	SAlloc::F(str);
	unum_close(fmt);

	u_uastrcpy(pat, "");
	u_uastrcpy(pat, "a''b#");

	fmt = unum_open(UNUM_IGNORE, pat, u_strlen(pat), "en_US", NULL, &status);
	if(U_FAILURE(status)) {
		log_err("Error in number format construction using pattern \"a''b#\"\n");
	}
	lneed = 0;
	lneed = unum_format(fmt, 123, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		unum_format(fmt, 123, str, lneed+1,  NULL, &status);
	}
	if(U_FAILURE(status)) {
		log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
	}
	log_verbose("Pattern \"%s\" \n", u_austrcpy(tempBuf, pat));
	log_verbose("Format 123 -> %s\n", u_austrcpy(tempBuf, str));
	u_uastrcpy(res, "");
	u_uastrcpy(res, "a'b123");
	if(u_strcmp(str, res) != 0)
		log_err("FAIL: Expected a'b123\n");

	SAlloc::F(str);
	unum_close(fmt);
}

/* Test exponential pattern*/
static void TestExponential(void)
{
	int32_t pat_length, val_length, lval_length;
	int32_t ival, ilval, p, v, lneed;
	UNumberFormat * fmt;
	int32_t ppos;
	char16_t * upat;
	char16_t pattern[20];
	char16_t * str = NULL;
	char16_t uvalfor[20], ulvalfor[20];
	char tempMsgBug[256];
	double a;
	UErrorCode status = U_ZERO_ERROR;
#if U_PLATFORM == U_PF_OS390
	static const double val[] = { 0.01234, 123456789, 1.23e75, -3.141592653e-78 };
#else
	static const double val[] = { 0.01234, 123456789, 1.23e300, -3.141592653e-271 };
#endif
	static const char * pat[] = { "0.####E0", "00.000E00", "##0.######E000", "0.###E0;[0.###E0]" };
	static const int32_t lval[] = { 0, -1, 1, 123456789 };

	static const char * valFormat[] =
	{
		"1.234E-2", "1.2346E8", "1.23E300", "-3.1416E-271",
		"12.340E-03", "12.346E07", "12.300E299", "-31.416E-272",
		"12.34E-003", "123.4568E006", "1.23E300", "-314.1593E-273",
		"1.234E-2", "1.235E8", "1.23E300", "[3.142E-271]"
	};
	static const char * lvalFormat[] =
	{
		"0E0", "-1E0", "1E0", "1.2346E8",
		"00.000E00", "-10.000E-01", "10.000E-01", "12.346E07",
		"0E000", "-1E000", "1E000", "123.4568E006",
		"0E0", "[1E0]", "1E0", "1.235E8"
	};
	static const double valParse[] =
	{
#if U_PLATFORM == U_PF_OS390
		0.01234, 123460000, 1.23E75, -3.1416E-78,
		0.01234, 123460000, 1.23E75, -3.1416E-78,
		0.01234, 123456800, 1.23E75, -3.141593E-78,
		0.01234, 123500000, 1.23E75, -3.142E-78
#else
		/* We define the whole IEEE 754 number in the 4th column because
		   Visual Age 7 has a bug in rounding numbers. */
		0.01234, 123460000, 1.23E300, -3.1415999999999999E-271,
		0.01234, 123460000, 1.23E300, -3.1415999999999999E-271,
		0.01234, 123456800, 1.23E300, -3.1415929999999999E-271,
		0.01234, 123500000, 1.23E300, -3.1420000000000001E-271
#endif
	};
	static const int32_t lvalParse[] =
	{
		0, -1, 1, 123460000,
		0, -1, 1, 123460000,
		0, -1, 1, 123456800,
		0, -1, 1, 123500000
	};

	pat_length = SIZEOFARRAYi(pat);
	val_length = SIZEOFARRAYi(val);
	lval_length = SIZEOFARRAYi(lval);
	ival = 0;
	ilval = 0;
	for(p = 0; p < pat_length; ++p) {
		upat = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen(pat[p])+1));
		u_uastrcpy(upat, pat[p]);
		fmt = unum_open(UNUM_IGNORE, upat, u_strlen(upat), "en_US", NULL, &status);
		if(U_FAILURE(status)) {
			log_err_status(status,
			    "FAIL: Bad status returned by Number format construction with pattern %s -> %s\n",
			    pat[p],
			    u_errorName(status));
			SAlloc::F(upat);
			upat = NULL;
			continue;
		}
		lneed = u_strlen(upat) + 1;
		unum_toPattern(fmt, FALSE, pattern, lneed, &status);
		log_verbose("Pattern \" %s \" -toPattern-> \" %s \" \n", upat, u_austrcpy(tempMsgBug, pattern));
		for(v = 0; v<val_length; ++v) {
			/*format*/
			lneed = 0;
			lneed = unum_formatDouble(fmt, val[v], NULL, lneed, NULL, &status);
			if(status==U_BUFFER_OVERFLOW_ERROR) {
				status = U_ZERO_ERROR;
				str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
				unum_formatDouble(fmt, val[v], str, lneed+1,  NULL, &status);
			}
			if(U_FAILURE(status)) {
				log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
			}

			u_uastrcpy(uvalfor, valFormat[v+ival]);
			if(u_strcmp(str, uvalfor) != 0)
				log_verbose("FAIL: Expected %s ( %s )\n", valFormat[v+ival], u_austrcpy(tempMsgBug, uvalfor));

			/*parsing*/
			ppos = 0;
			a = unum_parseDouble(fmt, str, u_strlen(str), &ppos, &status);
			if(ppos== u_strlen(str)) {
				if(a != valParse[v+ival])
					log_err("FAIL: Expected: %e, Got: %g\n", valParse[v+ival], a);
			}
			else
				log_err(" FAIL: Partial parse (  %d  chars ) ->  %e\n",  ppos, a);

			SAlloc::F(str);
		}
		for(v = 0; v<lval_length; ++v) {
			/*format*/
			lneed = 0;
			lneed = unum_formatDouble(fmt, lval[v], NULL, lneed, NULL, &status);
			if(status==U_BUFFER_OVERFLOW_ERROR) {
				status = U_ZERO_ERROR;
				str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
				unum_formatDouble(fmt, lval[v], str, lneed+1,  NULL, &status);
			}
			if(U_FAILURE(status)) {
				log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
			}
			/*printf(" Format %e -> %s\n",  lval[v], austrdup(str));*/
			u_uastrcpy(ulvalfor, lvalFormat[v+ilval]);
			if(u_strcmp(str, ulvalfor) != 0)
				log_err("FAIL: Expected %s ( %s )\n", valFormat[v+ilval], austrdup(ulvalfor));

			/*parsing*/
			ppos = 0;
			a = unum_parseDouble(fmt, str, u_strlen(str), &ppos, &status);
			if(ppos== u_strlen(str)) {
				/*printf(" Parse -> %e\n",  a);*/
				if(a != lvalParse[v+ilval])
					log_err("FAIL: Expected : %e\n", valParse[v+ival]);
			}
			else
				log_err(" FAIL: Partial parse (  %d  chars ) ->  %e\n",  ppos, a);

			SAlloc::F(str);
		}
		ival += val_length;
		ilval += lval_length;
		unum_close(fmt);
		SAlloc::F(upat);
	}
}

/**
 * Test the handling of the currency symbol in patterns.
 */
static void TestCurrencySign(void)
{
	int32_t lneed;
	UNumberFormat * fmt;
	char16_t * pattern = NULL;
	char16_t * str = NULL;
	char16_t * pat = NULL;
	char16_t * res = NULL;
	UErrorCode status = U_ZERO_ERROR;
	char tempBuf[256];

	pattern = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen("*#,##0.00;-*#,##0.00") + 1));
	u_uastrcpy(pattern, "*#,##0.00;-*#,##0.00");
	pattern[0] = pattern[11] = 0xa4; /* insert latin-1 currency symbol */
	fmt = unum_open(UNUM_IGNORE, pattern, u_strlen(pattern), "en_US", NULL, &status);
	if(U_FAILURE(status)) {
		log_err_status(status,
		    "Error in number format construction with pattern  \"\\xA4#,##0.00;-\\xA4#,##0.00\\\" -> %s\n",
		    u_errorName(status));
	}
	lneed = 0;
	lneed = unum_formatDouble(fmt, 1234.56, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		unum_formatDouble(fmt, 1234.56, str, lneed+1, NULL, &status);
	}
	if(U_FAILURE(status)) {
		log_err_status(status, "Error in formatting using unum_format(.....): %s\n", myErrorName(status));
	}
	lneed = 0;
	lneed = unum_toPattern(fmt, FALSE, NULL, lneed, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		pat = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		unum_formatDouble(fmt, FALSE, pat, lneed+1, NULL, &status);
	}
	log_verbose("Pattern \" %s \" \n", u_austrcpy(tempBuf, pat));
	log_verbose("Format 1234.56 -> %s\n", u_austrcpy(tempBuf, str));
	if(U_SUCCESS(status) && str) {
		res = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen("$1,234.56")+1));
		u_uastrcpy(res, "$1,234.56");
		if(u_strcmp(str, res) !=0) log_data_err("FAIL: Expected $1,234.56\n");
	}
	else {
		log_err_status(status, "Error formatting -> %s\n", u_errorName(status));
	}
	SAlloc::F(str);
	SAlloc::F(res);
	SAlloc::F(pat);

	lneed = 0;
	lneed = unum_formatDouble(fmt, -1234.56, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		unum_formatDouble(fmt, -1234.56, str, lneed+1, NULL, &status);
	}
	if(U_FAILURE(status)) {
		log_err_status(status, "Error in formatting using unum_format(.....): %s\n", myErrorName(status));
	}
	if(str) {
		res = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen("-$1,234.56")+1));
		u_uastrcpy(res, "-$1,234.56");
		if(u_strcmp(str, res) != 0) log_data_err("FAIL: Expected -$1,234.56\n");
		SAlloc::F(str);
		SAlloc::F(res);
	}

	unum_close(fmt);
	SAlloc::F(pattern);
}

/**
 * Test localized currency patterns.
 */
static void TestCurrency(void)
{
	UNumberFormat * currencyFmt;
	char16_t * str;
	int32_t lneed, i;
	UFieldPosition pos;
	char16_t res[100];
	UErrorCode status = U_ZERO_ERROR;
	const char * locale[] = {"fr_CA", "de_DE@currency=DEM", "fr_FR@currency=FRF"};
	const char * result[] = {"1,50\\u00a0$", "1,50\\u00a0DM", "1,50\\u00a0F"};
	log_verbose("\nTesting the number format with different currency patterns\n");
	for(i = 0; i < 3; i++) {
		str = NULL;
		currencyFmt = unum_open(UNUM_CURRENCY, NULL, 0, locale[i], NULL, &status);

		if(U_FAILURE(status)) {
			log_data_err("Error in the construction of number format with style currency: %s (Are you missing data?)\n",
			    myErrorName(status));
		}
		else {
			lneed = 0;
			lneed = unum_formatDouble(currencyFmt, 1.50, NULL, lneed, NULL, &status);
			if(status==U_BUFFER_OVERFLOW_ERROR) {
				status = U_ZERO_ERROR;
				str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
				pos.field = 0;
				unum_formatDouble(currencyFmt, 1.50, str, lneed+1, &pos, &status);
			}

			if(U_FAILURE(status)) {
				log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
			}
			else {
				u_unescape(result[i], res, (int32_t)strlen(result[i])+1);

				if(u_strcmp(str, res) != 0) {
					log_err("FAIL: Expected %s Got: %s for locale: %s\n", result[i], aescstrdup(str, -1), locale[i]);
				}
			}
		}

		unum_close(currencyFmt);
		SAlloc::F(str);
	}
}

/**
 * Test currency "object" (we use this name to match the other C++
 * test name and the Jave name).  Actually, test ISO currency code
 * support in the C API.
 */
static void TestCurrencyObject(void)
{
	UNumberFormat * currencyFmt;
	char16_t * str = NULL, * res = NULL;
	int32_t lneed, i;
	UFieldPosition pos;
	UErrorCode status = U_ZERO_ERROR;

	const char * locale[] = {
		"fr_FR",
		"fr_FR",
	};

	const char * currency[] = {
		"",
		"JPY",
	};

	const char * result[] = {
		"1\\u202F234,56\\u00A0\\u20AC",
		"1\\u202F235\\u00A0JPY",
	};

	log_verbose("\nTesting the number format with different currency codes\n");
	for(i = 0; i < 2; i++) {
		char cStr[20] = {0};
		char16_t isoCode[16] = {0};
		currencyFmt = unum_open(UNUM_CURRENCY, NULL, 0, locale[i], NULL, &status);
		if(U_FAILURE(status)) {
			log_data_err("Error in the construction of number format with style currency: %s (Are you missing data?)\n",
			    myErrorName(status));
		}
		else {
			if(*currency[i]) {
				u_uastrcpy(isoCode, currency[i]);
				unum_setTextAttribute(currencyFmt, UNUM_CURRENCY_CODE,
				    isoCode, u_strlen(isoCode), &status);

				if(U_FAILURE(status)) {
					log_err("FAIL: can't set currency code %s\n", myErrorName(status));
				}
			}

			unum_getTextAttribute(currencyFmt, UNUM_CURRENCY_CODE,
			    isoCode, sizeof(isoCode), &status);

			if(U_FAILURE(status)) {
				log_err("FAIL: can't get currency code %s\n", myErrorName(status));
			}

			u_UCharsToChars(isoCode, cStr, u_strlen(isoCode));
			log_verbose("ISO code %s\n", cStr);
			if(*currency[i] && strcmp(cStr, currency[i])) {
				log_err("FAIL: currency should be %s, but is %s\n", currency[i], cStr);
			}

			lneed = 0;
			lneed = unum_formatDouble(currencyFmt, 1234.56, NULL, lneed, NULL, &status);
			if(status==U_BUFFER_OVERFLOW_ERROR) {
				status = U_ZERO_ERROR;
				str = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
				pos.field = 0;
				unum_formatDouble(currencyFmt, 1234.56, str, lneed+1, &pos, &status);
			}
			if(U_FAILURE(status)) {
				log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
			}
			else {
				res = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen(result[i])+1));
				u_unescape(result[i], res, (int32_t)(strlen(result[i])+1));
				if(u_strcmp(str, res) != 0) {
					log_err("FAIL: Expected %s Got: %s for locale: %s\n", result[i], aescstrdup(str, -1), locale[i]);
				}
			}
		}

		unum_close(currencyFmt);
		SAlloc::F(str);
		SAlloc::F(res);
	}
}

/**
 * Test proper rounding by the format method.
 */
static void TestRounding487(void)
{
	UNumberFormat * nnf;
	UErrorCode status = U_ZERO_ERROR;
	/* this is supposed to open default date format, but later on it treats it like it is "en_US"
	   - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
	/* nnf = unum_open(UNUM_DEFAULT, NULL, &status); */
	nnf = unum_open(UNUM_DEFAULT, NULL, 0, "en_US", NULL, &status);

	if(U_FAILURE(status)) {
		log_data_err("FAIL: failure in the construction of number format: %s (Are you missing data?)\n", myErrorName(status));
	}
	else {
		roundingTest(nnf, 0.00159999, 4, "0.0016");
		roundingTest(nnf, 0.00995, 4, "0.01");

		roundingTest(nnf, 12.3995, 3, "12.4");

		roundingTest(nnf, 12.4999, 0, "12");
		roundingTest(nnf, -19.5, 0, "-20");
	}

	unum_close(nnf);
}

/*-------------------------------------*/

static void roundingTest(UNumberFormat * nf, double x, int32_t maxFractionDigits, const char * expected)
{
	char16_t * out = NULL;
	char16_t * res;
	UFieldPosition pos;
	UErrorCode status;
	int32_t lneed;
	status = U_ZERO_ERROR;
	unum_setAttribute(nf, UNUM_MAX_FRACTION_DIGITS, maxFractionDigits);
	lneed = 0;
	lneed = unum_formatDouble(nf, x, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		out = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		pos.field = 0;
		unum_formatDouble(nf, x, out, lneed+1, &pos, &status);
	}
	if(U_FAILURE(status)) {
		log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
	}
	/*Need to use log_verbose here. Problem with the float*/
	/*printf("%f format with %d fraction digits to %s\n", x, maxFractionDigits, austrdup(out));*/
	res = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen(expected)+1));
	u_uastrcpy(res, expected);
	if(u_strcmp(out, res) != 0)
		log_err("FAIL: Expected: %s or %s\n", expected, austrdup(res));
	SAlloc::F(res);
	if(out != NULL) {
		SAlloc::F(out);
	}
}

/*
 * Testing unum_getDoubleAttribute and  unum_setDoubleAttribute()
 */
static void TestDoubleAttribute(void)
{
	double mydata[] = { 1.11, 22.22, 333.33, 4444.44, 55555.55, 666666.66, 7777777.77, 88888888.88, 999999999.99};
	double dvalue;
	int i;
	UErrorCode status = U_ZERO_ERROR;
	UNumberFormatAttribute attr;
	UNumberFormatStyle style = UNUM_DEFAULT;
	UNumberFormat * def;

	log_verbose("\nTesting get and set DoubleAttributes\n");
	def = unum_open(style, NULL, 0, NULL, NULL, &status);

	if(U_FAILURE(status)) {
		log_data_err("Fail: error creating a default number formatter -> %s (Are you missing data?)\n", u_errorName(status));
	}
	else {
		attr = UNUM_ROUNDING_INCREMENT;
		dvalue = unum_getDoubleAttribute(def, attr);
		for(i = 0; i<9; i++) {
			dvalue = mydata[i];
			unum_setDoubleAttribute(def, attr, dvalue);
			if(unum_getDoubleAttribute(def, attr)!=mydata[i])
				log_err("Fail: error in setting and getting double attributes for UNUM_ROUNDING_INCREMENT\n");
			else
				log_verbose("Pass: setting and getting double attributes for UNUM_ROUNDING_INCREMENT works fine\n");
		}
	}

	unum_close(def);
}

/**
 * Test the functioning of the secondary grouping value.
 */
static void TestSecondaryGrouping() {
	UErrorCode status = U_ZERO_ERROR;
	UNumberFormat * f = NULL, * g = NULL;
	UNumberFormat * us = unum_open(UNUM_DECIMAL, NULL, 0, "en_US", NULL, &status);
	UFieldPosition pos;
	char16_t resultBuffer[512];
	int32_t l = 1876543210L;
	bool ok = TRUE;
	char16_t buffer[512];
	int32_t i;
	bool expectGroup = FALSE, isGroup = FALSE;

	u_uastrcpy(buffer, "#,##,###");
	f = unum_open(UNUM_IGNORE, buffer, -1, "en_US", NULL, &status);
	if(U_FAILURE(status)) {
		log_data_err("Error DecimalFormat ct -> %s (Are you missing data?)\n", u_errorName(status));
		return;
	}

	pos.field = 0;
	unum_format(f, (int32_t)123456789L, resultBuffer, 512, &pos, &status);
	u_uastrcpy(buffer, "12,34,56,789");
	if((u_strcmp(resultBuffer, buffer) != 0) || U_FAILURE(status)) {
		log_err("Fail: Formatting \"#,##,###\" pattern with 123456789 got %s, expected %s\n", austrdup(resultBuffer),
		    "12,34,56,789");
	}
	if(pos.beginIndex != 0 && pos.endIndex != 12) {
		log_err("Fail: Formatting \"#,##,###\" pattern pos = (%d, %d) expected pos = (0, 12)\n", pos.beginIndex, pos.endIndex);
	}
	memzero(resultBuffer, sizeof(char16_t)*512);
	unum_toPattern(f, FALSE, resultBuffer, 512, &status);
	u_uastrcpy(buffer, "#,##,##0");
	if((u_strcmp(resultBuffer, buffer) != 0) || U_FAILURE(status)) {
		log_err("Fail: toPattern() got %s, expected %s\n", austrdup(resultBuffer), "#,##,##0");
	}
	memzero(resultBuffer, sizeof(char16_t)*512);
	u_uastrcpy(buffer, "#,###");
	unum_applyPattern(f, FALSE, buffer, -1, NULL, NULL);
	if(U_FAILURE(status)) {
		log_err("Fail: applyPattern call failed\n");
	}
	unum_setAttribute(f, UNUM_SECONDARY_GROUPING_SIZE, 4);
	unum_format(f, (int32_t)123456789L, resultBuffer, 512, &pos, &status);
	u_uastrcpy(buffer, "12,3456,789");
	if((u_strcmp(resultBuffer, buffer) != 0) || U_FAILURE(status)) {
		log_err("Fail: Formatting \"#,###\" pattern with 123456789 got %s, expected %s\n", austrdup(resultBuffer), "12,3456,789");
	}
	memzero(resultBuffer, sizeof(char16_t)*512);
	unum_toPattern(f, FALSE, resultBuffer, 512, &status);
	u_uastrcpy(buffer, "#,####,##0");
	if((u_strcmp(resultBuffer, buffer) != 0) || U_FAILURE(status)) {
		log_err("Fail: toPattern() got %s, expected %s\n", austrdup(resultBuffer), "#,####,##0");
	}
	memzero(resultBuffer, sizeof(char16_t)*512);
	g = unum_open(UNUM_DECIMAL, NULL, 0, "hi_IN", NULL, &status);
	if(U_FAILURE(status)) {
		log_err("Fail: Cannot create UNumberFormat for \"hi_IN\" locale.\n");
	}
	unum_format(g, l, resultBuffer, 512, &pos, &status);
	unum_close(g);
	/* expect "1,87,65,43,210", but with Hindi digits */
	/*         01234567890123       */
	if(u_strlen(resultBuffer) != 14) {
		ok = FALSE;
	}
	else {
		for(i = 0; i<u_strlen(resultBuffer); ++i) {
			expectGroup = FALSE;
			switch(i) {
				case 1:
				case 4:
				case 7:
				case 10:
				    expectGroup = TRUE;
				    break;
			}
			/* Later -- fix this to get the actual grouping */
			/* character from the resource bundle. */
			isGroup = (bool)(resultBuffer[i] == 0x002C);
			if(isGroup != expectGroup) {
				ok = FALSE;
				break;
			}
		}
	}
	if(!ok) {
		log_err("FAIL  Expected %s x hi_IN -> \"1,87,65,43,210\" (with Hindi digits), got %s\n", "1876543210L", resultBuffer);
	}
	unum_close(f);
	unum_close(us);
}

static void TestCurrencyKeywords(void)
{
	static const char * const currencies[] = {
		"ADD", "ADP", "AED", "AFA", "AFN", "AIF", "ALK", "ALL", "ALV", "ALX", "AMD",
		"ANG", "AOA", "AOK", "AON", "AOR", "AOS", "ARA", "ARM", "ARP", "ARS", "ATS",
		"AUD", "AUP", "AWG", "AZM", "BAD", "BAM", "BAN", "BBD", "BDT", "BEC", "BEF",
		"BEL", "BGL", "BGM", "BGN", "BGO", "BGX", "BHD", "BIF", "BMD", "BMP", "BND",
		"BOB", "BOL", "BOP", "BOV", "BRB", "BRC", "BRE", "BRL", "BRN", "BRR", "BRZ",
		"BSD", "BSP", "BTN", "BTR", "BUK", "BUR", "BWP", "BYB", "BYL", "BYR", "BZD",
		"BZH", "CAD", "CDF", "CDG", "CDL", "CFF", "CHF", "CKD", "CLC", "CLE", "CLF",
		"CLP", "CMF", "CNP", "CNX", "CNY", "COB", "COF", "COP", "CRC", "CSC", "CSK",
		"CUP", "CUX", "CVE", "CWG", "CYP", "CZK", "DDM", "DEM", "DES", "DJF", "DKK",
		"DOP", "DZD", "DZF", "DZG", "ECS", "ECV", "EEK", "EGP", "ERN", "ESP", "ETB",
		"ETD", "EUR", "FIM", "FIN", "FJD", "FJP", "FKP", "FOK", "FRF", "FRG", "GAF",
		"GBP", "GEK", "GEL", "GHC", "GHO", "GHP", "GHR", "GIP", "GLK", "GMD", "GMP",
		"GNF", "GNI", "GNS", "GPF", "GQE", "GQF", "GQP", "GRD", "GRN", "GTQ", "GUF",
		"GWE", "GWM", "GWP", "GYD", "HKD", "HNL", "HRD", "HRK", "HTG", "HUF", "IBP",
		"IDG", "IDJ", "IDN", "IDR", "IEP", "ILL", "ILP", "ILS", "IMP", "INR", "IQD",
		"IRR", "ISK", "ITL", "JEP", "JMD", "JMP", "JOD", "JPY", "KES", "KGS", "KHO",
		"KHR", "KID", "KMF", "KPP", "KPW", "KRH", "KRO", "KRW", "KWD", "KYD", "KZR",
		"KZT", "LAK", "LBP", "LIF", "LKR", "LNR", "LRD", "LSL", "LTL", "LTT", "LUF",
		"LVL", "LVR", "LYB", "LYD", "LYP", "MAD", "MAF", "MCF", "MCG", "MDC", "MDL",
		"MDR", "MGA", "MGF", "MHD", "MKD", "MKN", "MLF", "MMK", "MMX", "MNT", "MOP",
		"MQF", "MRO", "MTL", "MTP", "MUR", "MVP", "MVR", "MWK", "MWP", "MXN", "MXP",
		"MXV", "MYR", "MZE", "MZM", "NAD", "NCF", "NGN", "NGP", "NHF", "NIC", "NIG",
		"NIO", "NLG", "NOK", "NPR", "NZD", "NZP", "OMR", "OMS", "PAB", "PDK", "PDN",
		"PDR", "PEI", "PEN", "PES", "PGK", "PHP", "PKR", "PLN", "PLX", "PLZ", "PSP",
		"PTC", "PTE", "PYG", "QAR", "REF", "ROL", "RON", "RUB", "RUR", "RWF", "SAR",
		"SAS", "SBD", "SCR", "SDD", "SDP", "SEK", "SGD", "SHP", "SIB", "SIT", "SKK",
		"SLL", "SML", "SOS", "SQS", "SRG", "SSP", "STD", "STE", "SUN", "SUR", "SVC",
		"SYP", "SZL", "TCC", "TDF", "THB", "TJR", "TJS", "TMM", "TND", "TOP", "TOS",
		"TPE", "TPP", "TRL", "TTD", "TTO", "TVD", "TWD", "TZS", "UAH", "UAK", "UGS",
		"UGX", "USD", "USN", "USS", "UYF", "UYP", "UYU", "UZC", "UZS", "VAL", "VDD",
		"VDN", "VDP", "VEB", "VGD", "VND", "VNN", "VNR", "VNS", "VUV", "WSP", "WST",
		"XAD", "XAF", "XAM", "XAU", "XBA", "XBB", "XBC", "XBD", "XCD", "XCF", "XDR",
		"XEF", "XEU", "XFO", "XFU", "XID", "XMF", "XNF", "XOF", "XPF", "XPS", "XSS",
		"XTR", "YDD", "YEI", "YER", "YUD", "YUF", "YUG", "YUM", "YUN", "YUO", "YUR",
		"ZAL", "ZAP", "ZAR", "ZMK", "ZMP", "ZRN", "ZRZ", "ZWD"
	};

	UErrorCode status = U_ZERO_ERROR;
	int32_t i = 0, j = 0;
	int32_t noLocales = uloc_countAvailable();
	char locale[256];
	char currLoc[256];
	char16_t result[4];
	char16_t currBuffer[256];

	for(i = 0; i < noLocales; i++) {
		strcpy(currLoc, uloc_getAvailable(i));
		for(j = 0; j < SIZEOFARRAYi(currencies); j++) {
			strcpy(locale, currLoc);
			strcat(locale, "@currency=");
			strcat(locale, currencies[j]);
			ucurr_forLocale(locale, result, 4, &status);
			u_charsToUChars(currencies[j], currBuffer, 3);
			currBuffer[3] = 0;
			if(u_strcmp(currBuffer, result) != 0) {
				log_err("Didn't get the right currency for %s\n", locale);
			}
		}
	}
}

static void TestGetKeywordValuesForLocale() {
#define PREFERRED_SIZE 15
#define MAX_NUMBER_OF_KEYWORDS 4
	const char * PREFERRED[PREFERRED_SIZE][MAX_NUMBER_OF_KEYWORDS] = {
		{ "root",               "USD", "USN", NULL },
		{ "und",                "USD", "USN", NULL },
		/*           { "und_ZZ",             "USD", NULL, NULL }, -- temporarily remove as this locale now has
		   15 entries */
		{ "en_US",              "USD", "USN", NULL },
		{ "en_029",             "USD", "USN", NULL },
		{ "en_TH",              "THB", NULL, NULL },
		{ "de",                 "EUR", NULL, NULL },
		{ "de_DE",              "EUR", NULL, NULL },
		{ "ar",                 "EGP", NULL, NULL },
		{ "ar_PS",              "ILS", "JOD", NULL },
		{ "en@currency=CAD",    "USD", "USN", NULL },
		{ "fr@currency=zzz",    "EUR", NULL, NULL },
		{ "de_DE@currency=DEM", "EUR", NULL, NULL },
		{ "en_US@rg=THZZZZ",    "THB", NULL, NULL },
		{ "de@rg=USZZZZ",       "USD", "USN", NULL },
		{ "en_US@currency=CAD;rg=THZZZZ", "THB", NULL, NULL },
	};
	const int32_t EXPECTED_SIZE[PREFERRED_SIZE] = {
		2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 1
	};
	/* ucurr_forLocale results for same locales; "" if no result expected */
	const char * FORLOCALE[PREFERRED_SIZE] = {
		"",    "",    "USD", "",
		"THB", "",    "EUR", "",
		"ILS", "CAD", "ZZZ", "DEM",
		"THB", "USD", "CAD"
	};
	UErrorCode status = U_ZERO_ERROR;
	int32_t i, j, size;
	UEnumeration * pref, * all;
	const char * loc = NULL;
	bool matchPref, matchAll;
	const char * value = NULL;
	int32_t valueLength = 0;

	UList * ALLList = NULL;

	UEnumeration * ALL = ucurr_getKeywordValuesForLocale("currency", uloc_getDefault(), FALSE, &status);
	if(ALL == NULL) {
		log_err_status(status, "ERROR getting keyword value for default locale. -> %s\n", u_errorName(status));
		return;
	}

	for(i = 0; i < PREFERRED_SIZE; i++) {
		char16_t getCurrU[4];
		int32_t getCurrLen;

		status = U_ZERO_ERROR;
		pref = NULL;
		all = NULL;
		loc = PREFERRED[i][0];
		pref = ucurr_getKeywordValuesForLocale("currency", loc, TRUE, &status);
		matchPref = FALSE;
		matchAll = FALSE;

		size = uenum_count(pref, &status);

		if(size == EXPECTED_SIZE[i]) {
			matchPref = TRUE;
			for(j = 0; j < size; j++) {
				if((value = uenum_next(pref, &valueLength, &status)) != NULL && U_SUCCESS(status)) {
					if(strcmp(value, PREFERRED[i][j+1]) != 0) {
						log_err("ERROR: locale %s got keywords #%d %s expected %s\n", loc, j, value,
						    PREFERRED[i][j+1]);

						matchPref = FALSE;
						break;
					}
				}
				else {
					matchPref = FALSE;
					log_err("ERROR getting keyword value for locale \"%s\"\n", loc);
					break;
				}
			}
		}
		else {
			log_err("FAIL: size of locale \"%s\" %d does not match expected size %d\n", loc, size, EXPECTED_SIZE[i]);
		}

		if(!matchPref) {
			log_err("FAIL: Preferred values for locale \"%s\" does not match expected.\n", loc);
			break;
		}
		uenum_close(pref);

		all = ucurr_getKeywordValuesForLocale("currency", loc, FALSE, &status);

		size = uenum_count(all, &status);

		if(U_SUCCESS(status) && size == uenum_count(ALL, &status)) {
			matchAll = TRUE;
			ALLList = ulist_getListFromEnum(ALL);
			for(j = 0; j < size; j++) {
				if((value = uenum_next(all, &valueLength, &status)) != NULL && U_SUCCESS(status)) {
					if(!ulist_containsString(ALLList, value, (int32_t)strlen(value))) {
						log_err("Locale %s have %s not in ALL\n", loc, value);
						matchAll = FALSE;
						break;
					}
				}
				else {
					matchAll = FALSE;
					log_err("ERROR getting \"all\" keyword value for locale \"%s\"\n", loc);
					break;
				}
			}
			if(!matchAll) {
				log_err("FAIL: All values for locale \"%s\" does not match expected.\n", loc);
			}
		}
		else {
			if(U_FAILURE(status)) {
				log_err("ERROR: %s\n", u_errorName(status));
			}
			else if(size!=uenum_count(ALL, &status)) {
				log_err("ERROR: got size of %d, wanted %d\n", size, uenum_count(ALL, &status));
			}
		}

		uenum_close(all);

		status = U_ZERO_ERROR;
		getCurrLen = ucurr_forLocale(loc, getCurrU, 4, &status);
		if(U_FAILURE(status)) {
			if(FORLOCALE[i][0] != 0) {
				log_err("ERROR: ucurr_forLocale %s, status %s\n", loc, u_errorName(status));
			}
		}
		else if(getCurrLen != 3) {
			if(FORLOCALE[i][0] != 0 || getCurrLen != -1) {
				log_err("ERROR: ucurr_forLocale %s, returned len %d\n", loc, getCurrLen);
			}
		}
		else {
			char getCurrB[4];
			u_UCharsToChars(getCurrU, getCurrB, 4);
			if(uprv_strncmp(getCurrB, FORLOCALE[i], 4) != 0) {
				log_err("ERROR: ucurr_forLocale %s, expected %s, got %s\n", loc, FORLOCALE[i], getCurrB);
			}
		}
	}

	uenum_close(ALL);
}

/**
 * Test proper handling of rounding modes.
 */
static void TestRounding5350(void)
{
	UNumberFormat * nnf;
	UErrorCode status = U_ZERO_ERROR;
	/* this is supposed to open default date format, but later on it treats it like it is "en_US"
	   - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
	/* nnf = unum_open(UNUM_DEFAULT, NULL, &status); */
	nnf = unum_open(UNUM_DEFAULT, NULL, 0, "en_US", NULL, &status);

	if(U_FAILURE(status)) {
		log_data_err("FAIL: failure in the construction of number format: %s (Are you missing data?)\n", myErrorName(status));
		return;
	}

	unum_setAttribute(nnf, UNUM_MAX_FRACTION_DIGITS, 2);
	roundingTest2(nnf, -0.125, UNUM_ROUND_CEILING, "-0.12");
	roundingTest2(nnf, -0.125, UNUM_ROUND_FLOOR, "-0.13");
	roundingTest2(nnf, -0.125, UNUM_ROUND_DOWN, "-0.12");
	roundingTest2(nnf, -0.125, UNUM_ROUND_UP, "-0.13");
	roundingTest2(nnf, 0.125, UNUM_ROUND_HALFEVEN, "0.12");
	roundingTest2(nnf, 0.135, UNUM_ROUND_HALF_ODD, "0.13");
	roundingTest2(nnf, 0.135, UNUM_ROUND_HALF_CEILING, "0.14");
	roundingTest2(nnf, -0.135, UNUM_ROUND_HALF_CEILING, "-0.13");
	roundingTest2(nnf, 0.135, UNUM_ROUND_HALF_FLOOR, "0.13");
	roundingTest2(nnf, -0.135, UNUM_ROUND_HALF_FLOOR, "-0.14");
	roundingTest2(nnf, 0.135, UNUM_ROUND_HALFDOWN, "0.13");
	roundingTest2(nnf, 0.125, UNUM_ROUND_HALFUP, "0.13");
	roundingTest2(nnf, 0.135, UNUM_FOUND_HALFEVEN, "0.14");
	/* The following are exactly represented, and shouldn't round */
	roundingTest2(nnf, 1.00, UNUM_ROUND_UP, "1");
	roundingTest2(nnf, 24.25, UNUM_ROUND_UP, "24.25");
	roundingTest2(nnf, 24.25, UNUM_ROUND_CEILING, "24.25");
	roundingTest2(nnf, -24.25, UNUM_ROUND_UP, "-24.25");

	/* Differences pretty far out there */
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_CEILING, "1.01");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_FLOOR, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_DOWN, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_UP, "1.01");
	roundingTest2(nnf, 1.0000001, UNUM_FOUND_HALFEVEN, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_HALF_ODD, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_HALF_CEILING, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_HALF_FLOOR, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_HALFDOWN, "1");
	roundingTest2(nnf, 1.0000001, UNUM_ROUND_HALFUP, "1");

	roundingTest2(nnf, -1.0000001, UNUM_ROUND_CEILING, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_FLOOR, "-1.01");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_DOWN, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_UP, "-1.01");
	roundingTest2(nnf, -1.0000001, UNUM_FOUND_HALFEVEN, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_HALF_ODD, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_HALF_CEILING, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_HALF_FLOOR, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_HALFDOWN, "-1");
	roundingTest2(nnf, -1.0000001, UNUM_ROUND_HALFUP, "-1");

	unum_close(nnf);
}

/*-------------------------------------*/

static void roundingTest2(UNumberFormat * nf, double x, int32_t roundingMode, const char * expected)
{
	char16_t * out = NULL;
	char16_t * res;
	UFieldPosition pos;
	UErrorCode status;
	int32_t lneed;
	status = U_ZERO_ERROR;
	unum_setAttribute(nf, UNUM_ROUNDING_MODE, roundingMode);
	lneed = 0;
	lneed = unum_formatDouble(nf, x, NULL, lneed, NULL, &status);
	if(status==U_BUFFER_OVERFLOW_ERROR) {
		status = U_ZERO_ERROR;
		out = (char16_t *)SAlloc::M(sizeof(char16_t) * (lneed+1));
		pos.field = 0;
		unum_formatDouble(nf, x, out, lneed+1, &pos, &status);
	}
	if(U_FAILURE(status)) {
		log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
	}
	/*Need to use log_verbose here. Problem with the float*/
	/*printf("%f format with %d fraction digits to %s\n", x, maxFractionDigits, austrdup(out));*/
	res = (char16_t *)SAlloc::M(sizeof(char16_t) * (strlen(expected)+1));
	u_uastrcpy(res, expected);
	if(u_strcmp(out, res) != 0)
		log_err("FAIL: Expected: \"%s\"  Got: \"%s\"\n", expected, austrdup(out));
	SAlloc::F(res);
	if(out != NULL) {
		SAlloc::F(out);
	}
}

#endif /* #if !UCONFIG_NO_FORMATTING */
