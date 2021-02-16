// test_syntax.c
// Copyright (c) 2019-2020  K.Kosako
//
#include "regint.h"
#pragma hdrstop
#ifdef ONIG_ESCAPE_UCHAR_COLLISION
	#undef ONIG_ESCAPE_UCHAR_COLLISION
#endif

#define SLEN(s)  strlen(s)

static OnigTestBlock OnigTB;

static void xx(char* pattern, char* str, int from, int to, int mem, int not, int error_no)
{
#ifdef __TRUSTINSOFT_ANALYZER__
	if(nall++ % TIS_TEST_CHOOSE_MAX != TIS_TEST_CHOOSE_CURRENT) return;
#endif
	regex_t* reg;
	OnigErrorInfo einfo;
	int r = onig_new(&reg, (uchar *)pattern, (uchar *)(pattern + SLEN(pattern)), ONIG_OPTION_DEFAULT, ONIG_ENCODING_UTF8, OnigTB.Syntax, &einfo);
	if(r) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		if(error_no == 0) {
			onig_error_code_to_str((uchar *)s, r, &einfo);
			slfprintf(OnigTB.err_file, "ERROR: %s  /%s/\n", s, pattern);
			OnigTB.nerror++;
		}
		else if(r == error_no) {
			slfprintf(OnigTB.out_file, "OK(ERROR): /%s/ %d\n", pattern, r);
			OnigTB.nsucc++;
		}
		else {
			slfprintf(OnigTB.out_file, "FAIL(ERROR): /%s/ '%s', %d, %d\n", pattern, str, error_no, r);
			OnigTB.nfail++;
		}
		return;
	}
	r = onig_search(reg, (uchar *)str, (uchar *)(str + SLEN(str)), (uchar *)str, (uchar *)(str + SLEN(str)), OnigTB.region, ONIG_OPTION_NONE);
	if(r < ONIG_MISMATCH) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		if(error_no == 0) {
			onig_error_code_to_str((uchar *)s, r);
			slfprintf(OnigTB.err_file, "ERROR: %s  /%s/\n", s, pattern);
			OnigTB.nerror++;
		}
		else if(r == error_no) {
			slfprintf(OnigTB.out_file, "OK(ERROR): /%s/ '%s', %d\n", pattern, str, r);
			OnigTB.nsucc++;
		}
		else {
			slfprintf(OnigTB.out_file, "FAIL ERROR NO: /%s/ '%s', %d, %d\n", pattern, str, error_no, r);
			OnigTB.nfail++;
		}
		return;
	}
	if(r == ONIG_MISMATCH) {
		if(not)
			OnigTB.OutputOkN(pattern, str);
		else
			OnigTB.OutputFail(pattern, str);
	}
	else {
		if(not)
			OnigTB.OutputFailN(pattern, str);
		else if(OnigTB.region->beg[mem] == from && OnigTB.region->end[mem] == to)
			OnigTB.OutputOk(pattern, str);
		else {
			slfprintf(OnigTB.out_file, "FAIL: /%s/ '%s' %d-%d : %d-%d\n", pattern, str, from, to, OnigTB.region->beg[mem], OnigTB.region->end[mem]);
			OnigTB.nfail++;
		}
	}
	onig_free(reg);
}

static void x2(char* pattern, char* str, int from, int to) { xx(pattern, str, from, to, 0, 0, 0); }
static void x3(char* pattern, char* str, int from, int to, int mem) { xx(pattern, str, from, to, mem, 0, 0); }
static void n(char* pattern, char* str) { xx(pattern, str, 0, 0, 0, 1, 0); }
static void e(char* pattern, char* str, int error_no) { xx(pattern, str, 0, 0, 0, 0, error_no); }

static int test_fixed_interval()
{
	x2("a{1,3}?", "aaa", 0, 1);
	x2("a{3}", "aaa", 0, 3);
	x2("a{3}?", "aaa", 0, 3);
	n("a{3}?", "aa");
	x2("a{3,3}?", "aaa", 0, 3);
	n("a{3,3}?", "aa");
	x2("a{1,3}+", "aaaaaa", 0, 3);
	x2("a{3}+", "aaaaaa", 0, 3);
	x2("a{3,3}+", "aaaaaa", 0, 3);
	return 0;
}

static int test_isolated_option()
{
	x2("", "", 0, 0);
	x2("^", "", 0, 0);
	n("^a", "\na");
	n(".", "\n");
	x2("(?s:.)", "\n", 0, 1);
	x2("(?s).", "\n", 0, 1);
	x2("(?s)a|.", "\n", 0, 1);
	n("(?s:a)|.", "\n");
	x2("b(?s)a|.", "\n", 0, 1);
	n("((?s)a)|.", "\n");
	n("b(?:(?s)a)|z|.", "\n");
	n(".|b(?s)a", "\n");
	n(".(?s)", "\n");
	n("(?s)(?-s)a|.", "\n");
	x2("(?s)a|.(?-s)", "\n", 0, 1);
	x2("(?s)a|((?-s)).", "\n", 0, 1);
	x2("(?s)a|(?:(?-s)).", "\n", 0, 1); // !!! Perl 5.26.1 returns empty match
	x2("(?s)a|(?:).", "\n", 0, 1); // !!! Perl 5.26.1 returns empty match
	x2("(?s)a|(?:.)", "\n", 0, 1);
	x2("(?s)a|(?:a*).", "\n", 0, 1);
	n("a|(?:).", "\n");           // !!! Perl 5.26.1 returns empty match
	n("a|(?:)(.)", "\n");
	x2("(?s)a|(?:)(.)", "\n", 0, 1);
	x2("b(?s)a|(?:)(.)", "\n", 0, 1);
	n("b((?s)a)|(?:)(.)", "\n");
	return 0;
}

static int test_prec_read()
{
	x2("(?=a).b", "ab", 0, 2);
	x2("(?=ab|(.))\\1", "ab", 1, 2); // doesn't backtrack if success once in prec-read
	n("(?!(.)z)a\\1", "aa"); // ! Perl 5.26.1 match with "aa"
	return 0;
}

static int test_look_behind()
{
	x2("(?<=a)b", "ab", 1, 2);
	x2("(?<=a|b)c", "abc", 2, 3);
	x2("(?<=a|(.))\\1", "abcc", 3, 4);
	// following is not match in Perl and Java
	//x2("(?<=a|(.))\\1", "aa", 1, 2);
	n("(?<!c|c)a", "ca");
	return 0;
}

extern int OnigTestSyntax_main(FILE * fOut)
{
	OnigTB.out_file = NZOR(fOut, stdout); // @sobolev
	OnigTB.err_file = NZOR(fOut, stdout);
	OnigEncoding use_encs[1];
	use_encs[0] = ONIG_ENCODING_UTF8;
	onig_initialize(use_encs, sizeof(use_encs)/sizeof(use_encs[0]));
	OnigTB.region = onig_region_new();
	OnigTB.Syntax = ONIG_SYNTAX_PERL;
	test_fixed_interval();
	test_isolated_option();
	test_prec_read();
	test_look_behind();
	e("(?<=ab|(.))\\1", "abb", ONIGERR_INVALID_LOOK_BEHIND_PATTERN); // Variable length lookbehind not implemented
	                                                                 // in Perl 5.26.1
	x3("()", "abc", 0, 0, 1);
	e("(", "", ONIGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS);
	// different spec.
	// e("\\x{7fffffff}", "", ONIGERR_TOO_BIG_WIDE_CHAR_VALUE);
	OnigTB.Syntax = ONIG_SYNTAX_JAVA;
	test_fixed_interval();
	test_isolated_option();
	test_prec_read();
	test_look_behind();
	x2("(?<=ab|(.))\\1", "abb", 2, 3);
	n("(?<!ab|b)c", "bbc");
	n("(?<!b|ab)c", "bbc");
	//slfprintf(out_file, "\nRESULT   SUCC: %4d,  FAIL: %d,  ERROR: %d      (by Oniguruma %s)\n", nsucc, nfail, nerror, onig_version());
	OnigTB.OutputResult();
	onig_region_free(OnigTB.region, 1);
	onig_end();
	return OnigTB.GetResult();
}
