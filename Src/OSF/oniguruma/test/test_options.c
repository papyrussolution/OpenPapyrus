// test_options.c
// Copyright (c) 2020  K.Kosako
//
#include "regint.h"
#pragma hdrstop
#ifdef ONIG_ESCAPE_UCHAR_COLLISION
	#undef ONIG_ESCAPE_UCHAR_COLLISION
#endif

static OnigTestBlock OnigTB;

#define SLEN(s)  strlen(s)

static void xx(OnigOptionType options, const char * pattern, const char * str, int from, int to, int mem, int _not, int error_no, int line_no)
{
#ifdef __TRUSTINSOFT_ANALYZER__
	if(nall++ % TIS_TEST_CHOOSE_MAX != TIS_TEST_CHOOSE_CURRENT) return;
#endif
	regex_t* reg;
	OnigErrorInfo einfo;
	int r = onig_new(&reg, (uchar *)pattern, (uchar *)(pattern + SLEN(pattern)), options, ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, &einfo);
	if(r) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		if(error_no == 0) {
			onig_error_code_to_str((uchar *)s, r, &einfo);
			OnigTB.OutputError(s, pattern, line_no);
		}
		else if(r == error_no) {
			slfprintf(OnigTB.out_file, "OK(ERROR): /%s/ %d  #%d\n", pattern, r, line_no);
			OnigTB.nsucc++;
		}
		else {
			slfprintf(OnigTB.out_file, "FAIL(ERROR): /%s/ '%s', %d, %d  #%d\n", pattern, str, error_no, r, line_no);
			OnigTB.nfail++;
		}
		return;
	}
	r = onig_search(reg, (uchar *)str, (uchar *)(str + SLEN(str)), (uchar *)str, (uchar *)(str + SLEN(str)), OnigTB.region, options);
	if(r < ONIG_MISMATCH) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		if(error_no == 0) {
			onig_error_code_to_str((uchar *)s, r);
			OnigTB.OutputError(s, pattern, line_no);
		}
		else if(r == error_no) {
			slfprintf(OnigTB.out_file, "OK(ERROR): /%s/ '%s', %d  #%d\n", pattern, str, r, line_no);
			OnigTB.nsucc++;
		}
		else {
			slfprintf(OnigTB.out_file, "FAIL ERROR NO: /%s/ '%s', %d, %d  #%d\n", pattern, str, error_no, r, line_no);
			OnigTB.nfail++;
		}
		return;
	}
	if(r == ONIG_MISMATCH) {
		if(_not)
			OnigTB.OutputOkN(pattern, str, line_no);
		else
			OnigTB.OutputFail(pattern, str, line_no);
	}
	else {
		if(_not)
			OnigTB.OutputFailN(pattern, str, line_no);
		else if(OnigTB.region->beg[mem] == from && OnigTB.region->end[mem] == to) {
			slfprintf(OnigTB.out_file, "OK: /%s/ '%s'  #%d\n", pattern, str, line_no);
			OnigTB.nsucc++;
		}
		else {
			slfprintf(OnigTB.out_file, "FAIL: /%s/ '%s' %d-%d : %d-%d  #%d\n", pattern, str, from, to, OnigTB.region->beg[mem], OnigTB.region->end[mem], line_no);
			OnigTB.nfail++;
		}
	}
	onig_free(reg);
}

static void xx2(OnigOptionType options, const char * pattern, const char * str, int from, int to, int line_no) { xx(options, pattern, str, from, to, 0, 0, 0, line_no); }
static void xx3(OnigOptionType options, const char * pattern, const char * str, int from, int to, int mem, int line_no) { xx(options, pattern, str, from, to, mem, 0, 0, line_no); }
static void xn(OnigOptionType options, const char * pattern, const char * str, int line_no) { xx(options, pattern, str, 0, 0, 0, 1, 0, line_no); }
#if 0
	static void xe(OnigOptionType options, char * pattern, char * str, int error_no, int line_no) { xx(options, pattern, str, 0, 0, 0, 0, error_no, line_no); }
#endif

#define x2(o, p, s, f, t)    xx2(o, p, s, f, t, __LINE__)
#define x3(o, p, s, f, t, m)  xx3(o, p, s, f, t, m, __LINE__)
#define n(o, p, s)          xn(o, p, s,   __LINE__)
#define e(o, p, s, en)       xe(o, p, s, en, __LINE__)
#define OIA  (ONIG_OPTION_IGNORECASE | ONIG_OPTION_IGNORECASE_IS_ASCII)

extern int OnigTestOptions_main(FILE * fOut)
{
	OnigTB.out_file = NZOR(fOut, stdout);
	OnigTB.err_file = NZOR(fOut, stdout);
	OnigEncoding use_encs[1];
	use_encs[0] = ONIG_ENCODING_UTF8;
	onig_initialize(use_encs, sizeof(use_encs)/sizeof(use_encs[0]));
	OnigTB.region = onig_region_new();
	x2(ONIG_OPTION_IGNORECASE, "a", "A", 0, 1);
	n(ONIG_OPTION_IGNORECASE_IS_ASCII, "a", "A");
	/* KELVIN SIGN */
	x2(ONIG_OPTION_IGNORECASE, "\xe2\x84\xaa", "k", 0, 1);
	x2(ONIG_OPTION_IGNORECASE, "k", "\xe2\x84\xaa", 0, 3);
	n(OIA, "\xe2\x84\xaa", "k");
	n(OIA, "k", "\xe2\x84\xaa");
	x2(OIA, "a", "a", 0, 1);
	x2(OIA, "A", "A", 0, 1);
	x2(OIA, "a", "A", 0, 1);
	x2(OIA, "A", "a", 0, 1);
	x2(OIA, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz", 0, 26);
	x2(OIA, "abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 26);
	x2(OIA, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCabcdefghijklmnopqrstuvwxyz", 3, 29);
	x2(OIA, "abcdefghijklmnopqrstuvwxyz", "abcABCDEFGHIJKLMNOPQRSTUVWXYZ", 3, 29);
	x3(OIA, "#%(a!;)(b&)", "#%A!;B&", 5, 7, 2);

	x2(ONIG_OPTION_IGNORECASE, "ss", "\xc3\x9f", 0, 2);
	x2(ONIG_OPTION_IGNORECASE, "\xc3\x9f", "SS", 0, 2);
	n(OIA, "ss", "\xc3\x9f");
	n(OIA, "\xc3\x9f", "ss");
	x2(OIA, "ss", "SS", 0, 2);
	x2(OIA, "Ss", "sS", 0, 2);

	n(ONIG_OPTION_NOTBOL, "^ab", "ab");
	n(ONIG_OPTION_NOTBOL, "\\Aab", "ab");
	n(ONIG_OPTION_NOTEOL, "ab$", "ab");
	n(ONIG_OPTION_NOTEOL, "ab\\z", "ab");
	n(ONIG_OPTION_NOTEOL, "ab\\Z", "ab");
	n(ONIG_OPTION_NOTEOL, "ab\\Z", "ab\n");
	n(ONIG_OPTION_NOT_BEGIN_STRING, "\\Aab", "ab");
	n(ONIG_OPTION_NOT_END_STRING, "ab\\z", "ab");
	n(ONIG_OPTION_NOT_END_STRING, "ab\\Z", "ab");
	n(ONIG_OPTION_NOT_END_STRING, "ab\\Z", "ab\n");
	x2(ONIG_OPTION_WORD_IS_ASCII, "\\w", "@g", 1, 2);
	n(ONIG_OPTION_WORD_IS_ASCII, "\\w", "あ");
	x2(ONIG_OPTION_NONE, "\\d", "１", 0, 3);
	n(ONIG_OPTION_DIGIT_IS_ASCII, "\\d", "１");
	x2(ONIG_OPTION_SPACE_IS_ASCII, "\\s", " ", 0, 1);
	x2(ONIG_OPTION_NONE, "\\s", "　", 0, 3);
	n(ONIG_OPTION_SPACE_IS_ASCII, "\\s", "　");
	x2(ONIG_OPTION_POSIX_IS_ASCII, "\\w\\d\\s", "c3 ", 0, 3);
	n(ONIG_OPTION_POSIX_IS_ASCII, "\\w|\\d|\\s", "あ４　");
	x2(ONIG_OPTION_EXTEND, " abc  \n def", "abcdef", 0, 6);
	x2(ONIG_OPTION_FIND_LONGEST, "\\w+", "abc defg hij", 4, 8);
	x2(ONIG_OPTION_FIND_NOT_EMPTY, "\\w*", "@@@ abc defg hij", 4, 7);
	//slfprintf(out_file, "\nRESULT   SUCC: %4d,  FAIL: %d,  ERROR: %d      (by Oniguruma %s)\n", nsucc, nfail, nerror, onig_version());
	OnigTB.OutputResult();
	onig_region_free(OnigTB.region, 1);
	onig_end();
	return OnigTB.GetResult();
}
