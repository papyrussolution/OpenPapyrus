// -*- coding: utf-8 -*-
// Copyright 2002-2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// TODO: Test extractions for PartialMatch/Consume

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>

#if !defined(_MSC_VER) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#include <sys/mman.h>
#include <unistd.h>  /* for sysconf */
#endif

#include "util/test.h"
#include "util/flags.h"
#include "util/logging.h"
#include "util/malloc_counter.h"
#include "util/strutil.h"
#include "util/utf.h"
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/regexp.h"
#include "re2/testing/regexp_generator.h"
#include "re2/testing/string_generator.h"
#include "re2/testing/exhaustive_tester.h"

namespace re2 {
	TEST(RE2, HexTests) 
	{
	#define ASSERT_HEX(type, value)                                         \
		do {                                                                  \
			type v;                                                             \
			ASSERT_TRUE(RE2::FullMatch(#value, "([0-9a-fA-F]+)[uUlL]*", RE2::Hex(&v))); \
			ASSERT_EQ(v, 0x ## value);                                            \
			ASSERT_TRUE(RE2::FullMatch("0x" #value, "([0-9a-fA-FxX]+)[uUlL]*", RE2::CRadix(&v))); \
			ASSERT_EQ(v, 0x ## value);                                            \
		} while(0)

		ASSERT_HEX(short,              2bad);
		ASSERT_HEX(unsigned short,     2badU);
		ASSERT_HEX(int,                dead);
		ASSERT_HEX(unsigned int,       deadU);
		ASSERT_HEX(long,               7eadbeefL);
		ASSERT_HEX(unsigned long,      deadbeefUL);
		ASSERT_HEX(long long,          12345678deadbeefLL);
		ASSERT_HEX(unsigned long long, cafebabedeadbeefULL);

	#undef ASSERT_HEX
	}

	TEST(RE2, OctalTests) 
	{
	#define ASSERT_OCTAL(type, value)                                           \
		do {                                                                      \
			type v;                                                                 \
			ASSERT_TRUE(RE2::FullMatch(#value, "([0-7]+)[uUlL]*", RE2::Octal(&v))); \
			ASSERT_EQ(v, 0 ## value);                                                 \
			ASSERT_TRUE(RE2::FullMatch("0" #value, "([0-9a-fA-FxX]+)[uUlL]*", RE2::CRadix(&v))); \
			ASSERT_EQ(v, 0 ## value);                                                 \
		} while(0)

		ASSERT_OCTAL(short,              77777);
		ASSERT_OCTAL(unsigned short,     177777U);
		ASSERT_OCTAL(int,                17777777777);
		ASSERT_OCTAL(unsigned int,       37777777777U);
		ASSERT_OCTAL(long,               17777777777L);
		ASSERT_OCTAL(unsigned long,      37777777777UL);
		ASSERT_OCTAL(long long,          777777777777777777777LL);
		ASSERT_OCTAL(unsigned long long, 1777777777777777777777ULL);

	#undef ASSERT_OCTAL
	}

	TEST(RE2, DecimalTests) 
	{
	#define ASSERT_DECIMAL(type, value)                                            \
		do {                                                                         \
			type v;                                                                    \
			ASSERT_TRUE(RE2::FullMatch(#value, "(-?[0-9]+)[uUlL]*", &v));              \
			ASSERT_EQ(v, value);                                                       \
			ASSERT_TRUE(RE2::FullMatch(#value, "(-?[0-9a-fA-FxX]+)[uUlL]*", RE2::CRadix(&v))); \
			ASSERT_EQ(v, value);                                                       \
		} while(0)

		ASSERT_DECIMAL(short,              -1);
		ASSERT_DECIMAL(unsigned short,     9999);
		ASSERT_DECIMAL(int,                -1000);
		ASSERT_DECIMAL(unsigned int,       12345U);
		ASSERT_DECIMAL(long,               -10000000L);
		ASSERT_DECIMAL(unsigned long,      3083324652U);
		ASSERT_DECIMAL(long long,          -100000000000000LL);
		ASSERT_DECIMAL(unsigned long long, 1234567890987654321ULL);

	#undef ASSERT_DECIMAL
	}

	TEST(RE2, Replace) 
	{
		struct ReplaceTest {
			const char * regexp;
			const char * rewrite;
			const char * original;
			const char * single;
			const char * global;
			int greplace_count;
		};

		static const ReplaceTest tests[] = {
			{ "(qu|[b-df-hj-np-tv-z]*)([a-z]+)", "\\2\\1ay", "the quick brown fox jumps over the lazy dogs.",
			  "ethay quick brown fox jumps over the lazy dogs.", "ethay ickquay ownbray oxfay umpsjay overay ethay azylay ogsday.", 9 },
			{ "\\w+", "\\0-NOSPAM", "abcd.efghi@google.com", "abcd-NOSPAM.efghi@google.com", "abcd-NOSPAM.efghi-NOSPAM@google-NOSPAM.com-NOSPAM", 4 },
			{ "^", "(START)", "foo", "(START)foo", "(START)foo", 1 },
			{ "^", "(START)", "", "(START)", "(START)", 1 },
			{ "$", "(END)", "", "(END)", "(END)", 1 },
			{ "b", "bb", "ababababab", "abbabababab", "abbabbabbabbabb", 5 },
			{ "b", "bb", "bbbbbb", "bbbbbbb", "bbbbbbbbbbbb", 6 },
			{ "b+", "bb", "bbbbbb", "bb", "bb", 1 },
			{ "b*", "bb", "bbbbbb", "bb", "bb", 1 },
			{ "b*", "bb", "aaaaa", "bbaaaaa", "bbabbabbabbabbabb", 6 },
			// Check newline handling
			{ "a.*a", "(\\0)", "aba\naba", "(aba)\naba", "(aba)\n(aba)", 2 },
			{ "", NULL, NULL, NULL, NULL, 0 }
		};

		for(const ReplaceTest* t = tests; t->original != NULL; t++) {
			std::string one(t->original);
			ASSERT_TRUE(RE2::Replace(&one, t->regexp, t->rewrite));
			ASSERT_EQ(one, t->single);
			std::string all(t->original);
			ASSERT_EQ(RE2::GlobalReplace(&all, t->regexp, t->rewrite), t->greplace_count)
				<< "Got: " << all;
			ASSERT_EQ(all, t->global);
		}
	}

	static void TestCheckRewriteString(const char* regexp, const char* rewrite, bool expect_ok) 
	{
		std::string error;
		RE2 exp(regexp);
		bool actual_ok = exp.CheckRewriteString(rewrite, &error);
		EXPECT_EQ(expect_ok, actual_ok) << " for " << rewrite << " error: " << error;
	}

	TEST(CheckRewriteString, all) 
	{
		TestCheckRewriteString("abc", "foo", true);
		TestCheckRewriteString("abc", "foo\\", false);
		TestCheckRewriteString("abc", "foo\\0bar", true);

		TestCheckRewriteString("a(b)c", "foo", true);
		TestCheckRewriteString("a(b)c", "foo\\0bar", true);
		TestCheckRewriteString("a(b)c", "foo\\1bar", true);
		TestCheckRewriteString("a(b)c", "foo\\2bar", false);
		TestCheckRewriteString("a(b)c", "f\\\\2o\\1o", true);

		TestCheckRewriteString("a(b)(c)", "foo\\12", true);
		TestCheckRewriteString("a(b)(c)", "f\\2o\\1o", true);
		TestCheckRewriteString("a(b)(c)", "f\\oo\\1", false);
	}

	TEST(RE2, Extract) 
	{
		std::string s;
		ASSERT_TRUE(RE2::Extract("boris@kremvax.ru", "(.*)@([^.]*)", "\\2!\\1", &s));
		ASSERT_EQ(s, "kremvax!boris");
		ASSERT_TRUE(RE2::Extract("foo", ".*", "'\\0'", &s));
		ASSERT_EQ(s, "'foo'");
		// check that false match doesn't overwrite
		ASSERT_FALSE(RE2::Extract("baz", "bar", "'\\0'", &s));
		ASSERT_EQ(s, "'foo'");
	}

	TEST(RE2, MaxSubmatchTooLarge) 
	{
		std::string s;
		ASSERT_FALSE(RE2::Extract("foo", "f(o+)", "\\1\\2", &s));
		s = "foo";
		ASSERT_FALSE(RE2::Replace(&s, "f(o+)", "\\1\\2"));
		s = "foo";
		ASSERT_FALSE(RE2::GlobalReplace(&s, "f(o+)", "\\1\\2"));
	}

	TEST(RE2, Consume) 
	{
		RE2 r("\\s*(\\w+)"); // matches a word, possibly proceeded by whitespace
		std::string word;

		std::string s("   aaa b!@#$@#$cccc");
		StringPiece input(s);

		ASSERT_TRUE(RE2::Consume(&input, r, &word));
		ASSERT_EQ(word, "aaa") << " input: " << input;
		ASSERT_TRUE(RE2::Consume(&input, r, &word));
		ASSERT_EQ(word, "b") << " input: " << input;
		ASSERT_FALSE(RE2::Consume(&input, r, &word)) << " input: " << input;
	}

	TEST(RE2, ConsumeN) 
	{
		const std::string s(" one two three 4");
		StringPiece input(s);

		RE2::Arg argv[2];
		const RE2::Arg* const args[2] = { &argv[0], &argv[1] };

		// 0 arg
		EXPECT_TRUE(RE2::ConsumeN(&input, "\\s*(\\w+)", args, 0)); // Skips "one".

		// 1 arg
		std::string word;
		argv[0] = &word;
		EXPECT_TRUE(RE2::ConsumeN(&input, "\\s*(\\w+)", args, 1));
		EXPECT_EQ("two", word);

		// Multi-args
		int n;
		argv[1] = &n;
		EXPECT_TRUE(RE2::ConsumeN(&input, "\\s*(\\w+)\\s*(\\d+)", args, 2));
		EXPECT_EQ("three", word);
		EXPECT_EQ(4, n);
	}

	TEST(RE2, FindAndConsume) 
	{
		RE2 r("(\\w+)"); // matches a word
		std::string word;

		std::string s("   aaa b!@#$@#$cccc");
		StringPiece input(s);

		ASSERT_TRUE(RE2::FindAndConsume(&input, r, &word));
		ASSERT_EQ(word, "aaa");
		ASSERT_TRUE(RE2::FindAndConsume(&input, r, &word));
		ASSERT_EQ(word, "b");
		ASSERT_TRUE(RE2::FindAndConsume(&input, r, &word));
		ASSERT_EQ(word, "cccc");
		ASSERT_FALSE(RE2::FindAndConsume(&input, r, &word));

		// Check that FindAndConsume works without any submatches.
		// Earlier version used uninitialized data for
		// length to consume.
		input = "aaa";
		ASSERT_TRUE(RE2::FindAndConsume(&input, "aaa"));
		ASSERT_EQ(input, "");
	}

	TEST(RE2, FindAndConsumeN) 
	{
		const std::string s(" one two three 4");
		StringPiece input(s);

		RE2::Arg argv[2];
		const RE2::Arg* const args[2] = { &argv[0], &argv[1] };

		// 0 arg
		EXPECT_TRUE(RE2::FindAndConsumeN(&input, "(\\w+)", args, 0)); // Skips "one".

		// 1 arg
		std::string word;
		argv[0] = &word;
		EXPECT_TRUE(RE2::FindAndConsumeN(&input, "(\\w+)", args, 1));
		EXPECT_EQ("two", word);

		// Multi-args
		int n;
		argv[1] = &n;
		EXPECT_TRUE(RE2::FindAndConsumeN(&input, "(\\w+)\\s*(\\d+)", args, 2));
		EXPECT_EQ("three", word);
		EXPECT_EQ(4, n);
	}

	TEST(RE2, MatchNumberPeculiarity) 
	{
		RE2 r("(foo)|(bar)|(baz)");
		std::string word1;
		std::string word2;
		std::string word3;

		ASSERT_TRUE(RE2::PartialMatch("foo", r, &word1, &word2, &word3));
		ASSERT_EQ(word1, "foo");
		ASSERT_EQ(word2, "");
		ASSERT_EQ(word3, "");
		ASSERT_TRUE(RE2::PartialMatch("bar", r, &word1, &word2, &word3));
		ASSERT_EQ(word1, "");
		ASSERT_EQ(word2, "bar");
		ASSERT_EQ(word3, "");
		ASSERT_TRUE(RE2::PartialMatch("baz", r, &word1, &word2, &word3));
		ASSERT_EQ(word1, "");
		ASSERT_EQ(word2, "");
		ASSERT_EQ(word3, "baz");
		ASSERT_FALSE(RE2::PartialMatch("f", r, &word1, &word2, &word3));

		std::string a;
		ASSERT_TRUE(RE2::FullMatch("hello", "(foo)|hello", &a));
		ASSERT_EQ(a, "");
	}

	TEST(RE2, Match) 
	{
		RE2 re("((\\w+):([0-9]+))"); // extracts host and port
		StringPiece group[4];

		// No match.
		StringPiece s = "zyzzyva";
		ASSERT_FALSE(
			re.Match(s, 0, s.size(), RE2::UNANCHORED, group, arraysize(group)));

		// Matches and extracts.
		s = "a chrisr:9000 here";
		ASSERT_TRUE(
			re.Match(s, 0, s.size(), RE2::UNANCHORED, group, arraysize(group)));
		ASSERT_EQ(group[0], "chrisr:9000");
		ASSERT_EQ(group[1], "chrisr:9000");
		ASSERT_EQ(group[2], "chrisr");
		ASSERT_EQ(group[3], "9000");

		std::string all, host;
		int port;
		ASSERT_TRUE(RE2::PartialMatch("a chrisr:9000 here", re, &all, &host, &port));
		ASSERT_EQ(all, "chrisr:9000");
		ASSERT_EQ(host, "chrisr");
		ASSERT_EQ(port, 9000);
	}

	static void TestRecursion(int size, const char* pattern) {
		// Fill up a string repeating the pattern given
		std::string domain;
		domain.resize(size);
		size_t patlen = strlen(pattern);
		for(int i = 0; i < size; i++) {
			domain[i] = pattern[i % patlen];
		}
		// Just make sure it doesn't crash due to too much recursion.
		RE2 re("([a-zA-Z0-9]|-)+(\\.([a-zA-Z0-9]|-)+)*(\\.)?", RE2::Quiet);
		RE2::FullMatch(domain, re);
	}

	// A meta-quoted string, interpreted as a pattern, should always match
	// the original unquoted string.
	static void TestQuoteMeta(const std::string& unquoted, const RE2::Options& options = RE2::DefaultOptions) 
	{
		std::string quoted = RE2::QuoteMeta(unquoted);
		RE2 re(quoted, options);
		EXPECT_TRUE(RE2::FullMatch(unquoted, re)) << "Unquoted='" << unquoted << "', quoted='" << quoted << "'.";
	}
	// A meta-quoted string, interpreted as a pattern, should always match
	// the original unquoted string.
	static void NegativeTestQuoteMeta(const std::string& unquoted, const std::string& should_not_match, const RE2::Options& options = RE2::DefaultOptions) 
	{
		std::string quoted = RE2::QuoteMeta(unquoted);
		RE2 re(quoted, options);
		EXPECT_FALSE(RE2::FullMatch(should_not_match, re)) << "Unquoted='" << unquoted << "', quoted='" << quoted << "'.";
	}

	// Tests that quoted meta characters match their original strings,
	// and that a few things that shouldn't match indeed do not.
	TEST(QuoteMeta, Simple) 
	{
		TestQuoteMeta("foo");
		TestQuoteMeta("foo.bar");
		TestQuoteMeta("foo\\.bar");
		TestQuoteMeta("[1-9]");
		TestQuoteMeta("1.5-2.0?");
		TestQuoteMeta("\\d");
		TestQuoteMeta("Who doesn't like ice cream?");
		TestQuoteMeta("((a|b)c?d*e+[f-h]i)");
		TestQuoteMeta("((?!)xxx).*yyy");
		TestQuoteMeta("([");
	}
	TEST(QuoteMeta, SimpleNegative) 
	{
		NegativeTestQuoteMeta("foo", "bar");
		NegativeTestQuoteMeta("...", "bar");
		NegativeTestQuoteMeta("\\.", ".");
		NegativeTestQuoteMeta("\\.", "..");
		NegativeTestQuoteMeta("(a)", "a");
		NegativeTestQuoteMeta("(a|b)", "a");
		NegativeTestQuoteMeta("(a|b)", "(a)");
		NegativeTestQuoteMeta("(a|b)", "a|b");
		NegativeTestQuoteMeta("[0-9]", "0");
		NegativeTestQuoteMeta("[0-9]", "0-9");
		NegativeTestQuoteMeta("[0-9]", "[9]");
		NegativeTestQuoteMeta("((?!)xxx)", "xxx");
	}
	TEST(QuoteMeta, Latin1) 
	{
		TestQuoteMeta("3\xb2 = 9", RE2::Latin1);
	}
	TEST(QuoteMeta, UTF8) 
	{
		TestQuoteMeta("Plácido Domingo");
		TestQuoteMeta("xyz"); // No fancy utf8.
		TestQuoteMeta("\xc2\xb0"); // 2-byte utf8 -- a degree symbol.
		TestQuoteMeta("27\xc2\xb0 degrees"); // As a middle character.
		TestQuoteMeta("\xe2\x80\xb3"); // 3-byte utf8 -- a double prime.
		TestQuoteMeta("\xf0\x9d\x85\x9f"); // 4-byte utf8 -- a music note.
		TestQuoteMeta("27\xc2\xb0"); // Interpreted as Latin-1, this should still work.
		NegativeTestQuoteMeta("27\xc2\xb0", "27\\\xc2\\\xb0");              // 2-byte utf8 -- a degree symbol.
	}

	TEST(QuoteMeta, HasNull) 
	{
		std::string has_null;
		// string with one null character
		has_null += '\0';
		TestQuoteMeta(has_null);
		NegativeTestQuoteMeta(has_null, "");
		// Don't want null-followed-by-'1' to be interpreted as '\01'.
		has_null += '1';
		TestQuoteMeta(has_null);
		NegativeTestQuoteMeta(has_null, "\1");
	}

	TEST(ProgramSize, BigProgram) 
	{
		RE2 re_simple("simple regexp");
		RE2 re_medium("medium.*regexp");
		RE2 re_complex("complex.{1,128}regexp");

		ASSERT_GT(re_simple.ProgramSize(), 0);
		ASSERT_GT(re_medium.ProgramSize(), re_simple.ProgramSize());
		ASSERT_GT(re_complex.ProgramSize(), re_medium.ProgramSize());

		ASSERT_GT(re_simple.ReverseProgramSize(), 0);
		ASSERT_GT(re_medium.ReverseProgramSize(), re_simple.ReverseProgramSize());
		ASSERT_GT(re_complex.ReverseProgramSize(), re_medium.ReverseProgramSize());
	}

	TEST(ProgramFanout, BigProgram) 
	{
		RE2 re1("(?:(?:(?:(?:(?:.)?){1})*)+)");
		RE2 re10("(?:(?:(?:(?:(?:.)?){10})*)+)");
		RE2 re100("(?:(?:(?:(?:(?:.)?){100})*)+)");
		RE2 re1000("(?:(?:(?:(?:(?:.)?){1000})*)+)");
		std::vector<int> histogram;
		// 3 is the largest non-empty bucket and has 2 element.
		ASSERT_EQ(3, re1.ProgramFanout(&histogram));
		ASSERT_EQ(2, histogram[3]);

		// 6 is the largest non-empty bucket and has 11 elements.
		ASSERT_EQ(6, re10.ProgramFanout(&histogram));
		ASSERT_EQ(11, histogram[6]);

		// 9 is the largest non-empty bucket and has 101 elements.
		ASSERT_EQ(9, re100.ProgramFanout(&histogram));
		ASSERT_EQ(101, histogram[9]);

		// 13 is the largest non-empty bucket and has 1001 elements.
		ASSERT_EQ(13, re1000.ProgramFanout(&histogram));
		ASSERT_EQ(1001, histogram[13]);

		// 2 is the largest non-empty bucket and has 2 element.
		ASSERT_EQ(2, re1.ReverseProgramFanout(&histogram));
		ASSERT_EQ(2, histogram[2]);

		// 5 is the largest non-empty bucket and has 11 elements.
		ASSERT_EQ(5, re10.ReverseProgramFanout(&histogram));
		ASSERT_EQ(11, histogram[5]);

		// 9 is the largest non-empty bucket and has 101 elements.
		ASSERT_EQ(9, re100.ReverseProgramFanout(&histogram));
		ASSERT_EQ(101, histogram[9]);

		// 12 is the largest non-empty bucket and has 1001 elements.
		ASSERT_EQ(12, re1000.ReverseProgramFanout(&histogram));
		ASSERT_EQ(1001, histogram[12]);
	}

	// Issue 956519: handling empty character sets was
	// causing NULL dereference.  This tests a few empty character sets.
	// (The way to get an empty character set is to negate a full one.)
	TEST(EmptyCharset, Fuzz) 
	{
		static const char * empties[] = { "[^\\S\\s]", "[^\\S[:space:]]", "[^\\D\\d]", "[^\\D[:digit:]]" };
		for(size_t i = 0; i < arraysize(empties); i++)
			ASSERT_FALSE(RE2(empties[i]).Match("abc", 0, 3, RE2::UNANCHORED, NULL, 0));
	}

	// Bitstate assumes that kInstFail instructions in
	// alternations or capture groups have been "compiled away".
	TEST(EmptyCharset, BitstateAssumptions) 
	{
		// Captures trigger use of Bitstate.
		static const char * nop_empties[] = { "((((()))))" "[^\\S\\s]?", "((((()))))" "([^\\S\\s])?", "((((()))))" "([^\\S\\s]|[^\\S\\s])?", "((((()))))" "(([^\\S\\s]|[^\\S\\s])|)" };
		StringPiece group[6];
		for(size_t i = 0; i < arraysize(nop_empties); i++)
			ASSERT_TRUE(RE2(nop_empties[i]).Match("", 0, 0, RE2::UNANCHORED, group, 6));
	}

	// Test that named groups work correctly.
	TEST(Capture, NamedGroups) 
	{
		{
			RE2 re("(hello world)");
			ASSERT_EQ(re.NumberOfCapturingGroups(), 1);
			const std::map<std::string, int>& m = re.NamedCapturingGroups();
			ASSERT_EQ(m.size(), 0);
		}
		{
			RE2 re("(?P<A>expr(?P<B>expr)(?P<C>expr))((expr)(?P<D>expr))");
			ASSERT_EQ(re.NumberOfCapturingGroups(), 6);
			const std::map<std::string, int>& m = re.NamedCapturingGroups();
			ASSERT_EQ(m.size(), 4);
			ASSERT_EQ(m.find("A")->second, 1);
			ASSERT_EQ(m.find("B")->second, 2);
			ASSERT_EQ(m.find("C")->second, 3);
			ASSERT_EQ(m.find("D")->second, 6); // $4 and $5 are anonymous
		}
	}

	TEST(RE2, CapturedGroupTest) {
		RE2 re("directions from (?P<S>.*) to (?P<D>.*)");
		int num_groups = re.NumberOfCapturingGroups();
		EXPECT_EQ(2, num_groups);
		std::string args[4];
		RE2::Arg arg0(&args[0]);
		RE2::Arg arg1(&args[1]);
		RE2::Arg arg2(&args[2]);
		RE2::Arg arg3(&args[3]);

		const RE2::Arg* const matches[4] = {&arg0, &arg1, &arg2, &arg3};
		EXPECT_TRUE(RE2::FullMatchN("directions from mountain view to san jose",
			re, matches, num_groups));
		const std::map<std::string, int>& named_groups = re.NamedCapturingGroups();
		EXPECT_TRUE(named_groups.find("S") != named_groups.end());
		EXPECT_TRUE(named_groups.find("D") != named_groups.end());

		// The named group index is 1-based.
		int source_group_index = named_groups.find("S")->second;
		int destination_group_index = named_groups.find("D")->second;
		EXPECT_EQ(1, source_group_index);
		EXPECT_EQ(2, destination_group_index);

		// The args is zero-based.
		EXPECT_EQ("mountain view", args[source_group_index - 1]);
		EXPECT_EQ("san jose", args[destination_group_index - 1]);
	}

	TEST(RE2, FullMatchWithNoArgs) {
		ASSERT_TRUE(RE2::FullMatch("h", "h"));
		ASSERT_TRUE(RE2::FullMatch("hello", "hello"));
		ASSERT_TRUE(RE2::FullMatch("hello", "h.*o"));
		ASSERT_FALSE(RE2::FullMatch("othello", "h.*o")); // Must be anchored at front
		ASSERT_FALSE(RE2::FullMatch("hello!", "h.*o")); // Must be anchored at end
	}

	TEST(RE2, PartialMatch) 
	{
		ASSERT_TRUE(RE2::PartialMatch("x", "x"));
		ASSERT_TRUE(RE2::PartialMatch("hello", "h.*o"));
		ASSERT_TRUE(RE2::PartialMatch("othello", "h.*o"));
		ASSERT_TRUE(RE2::PartialMatch("hello!", "h.*o"));
		ASSERT_TRUE(RE2::PartialMatch("x", "((((((((((((((((((((x))))))))))))))))))))"));
	}

	TEST(RE2, PartialMatchN) 
	{
		RE2::Arg argv[2];
		const RE2::Arg* const args[2] = { &argv[0], &argv[1] };

		// 0 arg
		EXPECT_TRUE(RE2::PartialMatchN("hello", "e.*o", args, 0));
		EXPECT_FALSE(RE2::PartialMatchN("othello", "a.*o", args, 0));

		// 1 arg
		int i;
		argv[0] = &i;
		EXPECT_TRUE(RE2::PartialMatchN("1001 nights", "(\\d+)", args, 1));
		EXPECT_EQ(1001, i);
		EXPECT_FALSE(RE2::PartialMatchN("three", "(\\d+)", args, 1));

		// Multi-arg
		std::string s;
		argv[1] = &s;
		EXPECT_TRUE(RE2::PartialMatchN("answer: 42:life", "(\\d+):(\\w+)", args, 2));
		EXPECT_EQ(42, i);
		EXPECT_EQ("life", s);
		EXPECT_FALSE(RE2::PartialMatchN("hi1", "(\\w+)(1)", args, 2));
	}

	TEST(RE2, FullMatchZeroArg) 
	{
		// Zero-arg
		ASSERT_TRUE(RE2::FullMatch("1001", "\\d+"));
	}

	TEST(RE2, FullMatchOneArg) 
	{
		int i;
		// Single-arg
		ASSERT_TRUE(RE2::FullMatch("1001", "(\\d+)",   &i));
		ASSERT_EQ(i, 1001);
		ASSERT_TRUE(RE2::FullMatch("-123", "(-?\\d+)", &i));
		ASSERT_EQ(i, -123);
		ASSERT_FALSE(RE2::FullMatch("10", "()\\d+", &i));
		ASSERT_FALSE(RE2::FullMatch("1234567890123456789012345678901234567890", "(\\d+)", &i));
	}

	TEST(RE2, FullMatchIntegerArg) 
	{
		int i;
		// Digits surrounding integer-arg
		ASSERT_TRUE(RE2::FullMatch("1234", "1(\\d*)4", &i));
		ASSERT_EQ(i, 23);
		ASSERT_TRUE(RE2::FullMatch("1234", "(\\d)\\d+", &i));
		ASSERT_EQ(i, 1);
		ASSERT_TRUE(RE2::FullMatch("-1234", "(-\\d)\\d+", &i));
		ASSERT_EQ(i, -1);
		ASSERT_TRUE(RE2::PartialMatch("1234", "(\\d)", &i));
		ASSERT_EQ(i, 1);
		ASSERT_TRUE(RE2::PartialMatch("-1234", "(-\\d)", &i));
		ASSERT_EQ(i, -1);
	}

	TEST(RE2, FullMatchStringArg) 
	{
		std::string s;
		// String-arg
		ASSERT_TRUE(RE2::FullMatch("hello", "h(.*)o", &s));
		ASSERT_EQ(s, std::string("ell"));
	}

	TEST(RE2, FullMatchStringPieceArg) 
	{
		int i;
		// StringPiece-arg
		StringPiece sp;
		ASSERT_TRUE(RE2::FullMatch("ruby:1234", "(\\w+):(\\d+)", &sp, &i));
		ASSERT_EQ(sp.size(), 4);
		ASSERT_TRUE(memcmp(sp.data(), "ruby", 4) == 0);
		ASSERT_EQ(i, 1234);
	}

	TEST(RE2, FullMatchMultiArg) {
		int i;
		std::string s;
		// Multi-arg
		ASSERT_TRUE(RE2::FullMatch("ruby:1234", "(\\w+):(\\d+)", &s, &i));
		ASSERT_EQ(s, std::string("ruby"));
		ASSERT_EQ(i, 1234);
	}

	TEST(RE2, FullMatchN) {
		RE2::Arg argv[2];
		const RE2::Arg* const args[2] = { &argv[0], &argv[1] };

		// 0 arg
		EXPECT_TRUE(RE2::FullMatchN("hello", "h.*o", args, 0));
		EXPECT_FALSE(RE2::FullMatchN("othello", "h.*o", args, 0));

		// 1 arg
		int i;
		argv[0] = &i;
		EXPECT_TRUE(RE2::FullMatchN("1001", "(\\d+)", args, 1));
		EXPECT_EQ(1001, i);
		EXPECT_FALSE(RE2::FullMatchN("three", "(\\d+)", args, 1));

		// Multi-arg
		std::string s;
		argv[1] = &s;
		EXPECT_TRUE(RE2::FullMatchN("42:life", "(\\d+):(\\w+)", args, 2));
		EXPECT_EQ(42, i);
		EXPECT_EQ("life", s);
		EXPECT_FALSE(RE2::FullMatchN("hi1", "(\\w+)(1)", args, 2));
	}

	TEST(RE2, FullMatchIgnoredArg) {
		int i;
		std::string s;

		// Old-school NULL should be ignored.
		ASSERT_TRUE(
			RE2::FullMatch("ruby:1234", "(\\w+)(:)(\\d+)", &s, (void*)NULL, &i));
		ASSERT_EQ(s, std::string("ruby"));
		ASSERT_EQ(i, 1234);

		// C++11 nullptr should also be ignored.
		ASSERT_TRUE(RE2::FullMatch("rubz:1235", "(\\w+)(:)(\\d+)", &s, nullptr, &i));
		ASSERT_EQ(s, std::string("rubz"));
		ASSERT_EQ(i, 1235);
	}

	TEST(RE2, FullMatchTypedNullArg) {
		std::string s;

		// Ignore non-void* NULL arg
		ASSERT_TRUE(RE2::FullMatch("hello", "he(.*)lo", (char*)NULL));
		ASSERT_TRUE(RE2::FullMatch("hello", "h(.*)o", (std::string*)NULL));
		ASSERT_TRUE(RE2::FullMatch("hello", "h(.*)o", (StringPiece*)NULL));
		ASSERT_TRUE(RE2::FullMatch("1234", "(.*)", (int*)NULL));
		ASSERT_TRUE(RE2::FullMatch("1234567890123456", "(.*)", (long long*)NULL));
		ASSERT_TRUE(RE2::FullMatch("123.4567890123456", "(.*)", (double*)NULL));
		ASSERT_TRUE(RE2::FullMatch("123.4567890123456", "(.*)", (float*)NULL));

		// Fail on non-void* NULL arg if the match doesn't parse for the given type.
		ASSERT_FALSE(RE2::FullMatch("hello", "h(.*)lo", &s, (char*)NULL));
		ASSERT_FALSE(RE2::FullMatch("hello", "(.*)", (int*)NULL));
		ASSERT_FALSE(RE2::FullMatch("1234567890123456", "(.*)", (int*)NULL));
		ASSERT_FALSE(RE2::FullMatch("hello", "(.*)", (double*)NULL));
		ASSERT_FALSE(RE2::FullMatch("hello", "(.*)", (float*)NULL));
	}

	// Check that numeric parsing code does not read past the end of
	// the number being parsed.
	// This implementation requires mmap(2) et al. and thus cannot
	// be used unless they are available.
	TEST(RE2, NULTerminated) {
	#if defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES > 0
		char * v;
		int x;
		long pagesize = sysconf(_SC_PAGE_SIZE);

	#ifndef MAP_ANONYMOUS
	#define MAP_ANONYMOUS MAP_ANON
	#endif
		v = static_cast<char*>(mmap(NULL, 2*pagesize, PROT_READ|PROT_WRITE,
			MAP_ANONYMOUS|MAP_PRIVATE, -1, 0));
		ASSERT_TRUE(v != reinterpret_cast<char*>(-1));
		LOG(INFO) << "Memory at " << (void*)v;
		ASSERT_EQ(munmap(v + pagesize, pagesize), 0) << " error " << errno;
		v[pagesize - 1] = '1';

		x = 0;
		ASSERT_TRUE(RE2::FullMatch(StringPiece(v + pagesize - 1, 1), "(.*)", &x));
		ASSERT_EQ(x, 1);
	#endif
	}

	TEST(RE2, FullMatchTypeTests) {
		// Type tests
		std::string zeros(1000, '0');
		{
			char c;
			ASSERT_TRUE(RE2::FullMatch("Hello", "(H)ello", &c));
			ASSERT_EQ(c, 'H');
		}
		{
			unsigned char c;
			ASSERT_TRUE(RE2::FullMatch("Hello", "(H)ello", &c));
			ASSERT_EQ(c, static_cast<unsigned char>('H'));
		}
		{
			int16_t v;
			ASSERT_TRUE(RE2::FullMatch("100",     "(-?\\d+)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100",    "(-?\\d+)", &v)); ASSERT_EQ(v, -100);
			ASSERT_TRUE(RE2::FullMatch("32767",   "(-?\\d+)", &v)); ASSERT_EQ(v, 32767);
			ASSERT_TRUE(RE2::FullMatch("-32768",  "(-?\\d+)", &v)); ASSERT_EQ(v, -32768);
			ASSERT_FALSE(RE2::FullMatch("-32769", "(-?\\d+)", &v));
			ASSERT_FALSE(RE2::FullMatch("32768",  "(-?\\d+)", &v));
		}
		{
			uint16_t v;
			ASSERT_TRUE(RE2::FullMatch("100",    "(\\d+)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("32767",  "(\\d+)", &v)); ASSERT_EQ(v, 32767);
			ASSERT_TRUE(RE2::FullMatch("65535",  "(\\d+)", &v)); ASSERT_EQ(v, 65535);
			ASSERT_FALSE(RE2::FullMatch("65536", "(\\d+)", &v));
		}
		{
			int32_t v;
			static const int32_t max = INT32_C(0x7fffffff);
			static const int32_t min = -max - 1;
			ASSERT_TRUE(RE2::FullMatch("100",          "(-?\\d+)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100",         "(-?\\d+)", &v)); ASSERT_EQ(v, -100);
			ASSERT_TRUE(RE2::FullMatch("2147483647",   "(-?\\d+)", &v)); ASSERT_EQ(v, max);
			ASSERT_TRUE(RE2::FullMatch("-2147483648",  "(-?\\d+)", &v)); ASSERT_EQ(v, min);
			ASSERT_FALSE(RE2::FullMatch("-2147483649", "(-?\\d+)", &v));
			ASSERT_FALSE(RE2::FullMatch("2147483648",  "(-?\\d+)", &v));

			ASSERT_TRUE(RE2::FullMatch(zeros + "2147483647", "(-?\\d+)", &v));
			ASSERT_EQ(v, max);
			ASSERT_TRUE(RE2::FullMatch("-" + zeros + "2147483648", "(-?\\d+)", &v));
			ASSERT_EQ(v, min);

			ASSERT_FALSE(RE2::FullMatch("-" + zeros + "2147483649", "(-?\\d+)", &v));
			ASSERT_TRUE(RE2::FullMatch("0x7fffffff", "(.*)", RE2::CRadix(&v)));
			ASSERT_EQ(v, max);
			ASSERT_FALSE(RE2::FullMatch("000x7fffffff", "(.*)", RE2::CRadix(&v)));
		}
		{
			uint32_t v;
			static const uint32_t max = UINT32_C(0xffffffff);
			ASSERT_TRUE(RE2::FullMatch("100",         "(\\d+)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("4294967295",  "(\\d+)", &v)); ASSERT_EQ(v, max);
			ASSERT_FALSE(RE2::FullMatch("4294967296", "(\\d+)", &v));
			ASSERT_FALSE(RE2::FullMatch("-1",         "(\\d+)", &v));

			ASSERT_TRUE(RE2::FullMatch(zeros + "4294967295", "(\\d+)", &v)); ASSERT_EQ(v, max);
		}
		{
			int64_t v;
			static const int64_t max = INT64_C(0x7fffffffffffffff);
			static const int64_t min = -max - 1;
			std::string str;

			ASSERT_TRUE(RE2::FullMatch("100",  "(-?\\d+)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100", "(-?\\d+)", &v)); ASSERT_EQ(v, -100);

			str = std::to_string(max);
			ASSERT_TRUE(RE2::FullMatch(str,    "(-?\\d+)", &v)); ASSERT_EQ(v, max);

			str = std::to_string(min);
			ASSERT_TRUE(RE2::FullMatch(str,    "(-?\\d+)", &v)); ASSERT_EQ(v, min);

			str = std::to_string(max);
			ASSERT_NE(str.back(), '9');
			str.back()++;
			ASSERT_FALSE(RE2::FullMatch(str,   "(-?\\d+)", &v));

			str = std::to_string(min);
			ASSERT_NE(str.back(), '9');
			str.back()++;
			ASSERT_FALSE(RE2::FullMatch(str,   "(-?\\d+)", &v));
		}
		{
			uint64_t v;
			int64_t v2;
			static const uint64_t max = UINT64_C(0xffffffffffffffff);
			std::string str;

			ASSERT_TRUE(RE2::FullMatch("100",  "(-?\\d+)", &v));  ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100", "(-?\\d+)", &v2)); ASSERT_EQ(v2, -100);

			str = std::to_string(max);
			ASSERT_TRUE(RE2::FullMatch(str,    "(-?\\d+)", &v)); ASSERT_EQ(v, max);

			ASSERT_NE(str.back(), '9');
			str.back()++;
			ASSERT_FALSE(RE2::FullMatch(str,   "(-?\\d+)", &v));
		}
	}

	TEST(RE2, FloatingPointFullMatchTypes) {
		std::string zeros(1000, '0');
		{
			float v;
			ASSERT_TRUE(RE2::FullMatch("100",   "(.*)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100.", "(.*)", &v)); ASSERT_EQ(v, -100);
			ASSERT_TRUE(RE2::FullMatch("1e23",  "(.*)", &v)); ASSERT_EQ(v, float(1e23));
			ASSERT_TRUE(RE2::FullMatch(" 100",  "(.*)", &v)); ASSERT_EQ(v, 100);

			ASSERT_TRUE(RE2::FullMatch(zeros + "1e23",  "(.*)", &v));
			ASSERT_EQ(v, float(1e23));

			// 6700000000081920.1 is an edge case.
			// 6700000000081920 is exactly halfway between
			// two float32s, so the .1 should make it round up.
			// However, the .1 is outside the precision possible with
			// a float64: the nearest float64 is 6700000000081920.
			// So if the code uses strtod and then converts to float32,
			// round-to-even will make it round down instead of up.
			// To pass the test, the parser must call strtof directly.
			// This test case is carefully chosen to use only a 17-digit
			// number, since C does not guarantee to get the correctly
			// rounded answer for strtod and strtof unless the input is
			// short.
			//
			// This is known to fail on Cygwin and MinGW due to a broken
			// implementation of strtof(3). And apparently MSVC too. Sigh.
	#if !defined(_MSC_VER) && !defined(__CYGWIN__) && !defined(__MINGW32__)
			ASSERT_TRUE(RE2::FullMatch("0.1", "(.*)", &v));
			ASSERT_EQ(v, 0.1f) << StringPrintf("%.8g != %.8g", v, 0.1f);
			ASSERT_TRUE(RE2::FullMatch("6700000000081920.1", "(.*)", &v));
			ASSERT_EQ(v, 6700000000081920.1f)
				<< StringPrintf("%.8g != %.8g", v, 6700000000081920.1f);
	#endif
		}
		{
			double v;
			ASSERT_TRUE(RE2::FullMatch("100",   "(.*)", &v)); ASSERT_EQ(v, 100);
			ASSERT_TRUE(RE2::FullMatch("-100.", "(.*)", &v)); ASSERT_EQ(v, -100);
			ASSERT_TRUE(RE2::FullMatch("1e23",  "(.*)", &v)); ASSERT_EQ(v, 1e23);
			ASSERT_TRUE(RE2::FullMatch(zeros + "1e23", "(.*)", &v));
			ASSERT_EQ(v, double(1e23));

			ASSERT_TRUE(RE2::FullMatch("0.1", "(.*)", &v));
			ASSERT_EQ(v, 0.1) << StringPrintf("%.17g != %.17g", v, 0.1);
			ASSERT_TRUE(RE2::FullMatch("1.00000005960464485", "(.*)", &v));
			ASSERT_EQ(v, 1.0000000596046448)
				<< StringPrintf("%.17g != %.17g", v, 1.0000000596046448);
		}
	}

	TEST(RE2, FullMatchAnchored) {
		int i;
		// Check that matching is fully anchored
		ASSERT_FALSE(RE2::FullMatch("x1001", "(\\d+)",  &i));
		ASSERT_FALSE(RE2::FullMatch("1001x", "(\\d+)",  &i));
		ASSERT_TRUE(RE2::FullMatch("x1001",  "x(\\d+)", &i)); ASSERT_EQ(i, 1001);
		ASSERT_TRUE(RE2::FullMatch("1001x",  "(\\d+)x", &i)); ASSERT_EQ(i, 1001);
	}

	TEST(RE2, FullMatchBraces) {
		// Braces
		ASSERT_TRUE(RE2::FullMatch("0abcd",  "[0-9a-f+.-]{5,}"));
		ASSERT_TRUE(RE2::FullMatch("0abcde", "[0-9a-f+.-]{5,}"));
		ASSERT_FALSE(RE2::FullMatch("0abc",  "[0-9a-f+.-]{5,}"));
	}

	TEST(RE2, Complicated) {
		// Complicated RE2
		ASSERT_TRUE(RE2::FullMatch("foo", "foo|bar|[A-Z]"));
		ASSERT_TRUE(RE2::FullMatch("bar", "foo|bar|[A-Z]"));
		ASSERT_TRUE(RE2::FullMatch("X",   "foo|bar|[A-Z]"));
		ASSERT_FALSE(RE2::FullMatch("XY", "foo|bar|[A-Z]"));
	}

	TEST(RE2, FullMatchEnd) {
		// Check full-match handling (needs '$' tacked on internally)
		ASSERT_TRUE(RE2::FullMatch("fo", "fo|foo"));
		ASSERT_TRUE(RE2::FullMatch("foo", "fo|foo"));
		ASSERT_TRUE(RE2::FullMatch("fo", "fo|foo$"));
		ASSERT_TRUE(RE2::FullMatch("foo", "fo|foo$"));
		ASSERT_TRUE(RE2::FullMatch("foo", "foo$"));
		ASSERT_FALSE(RE2::FullMatch("foo$bar", "foo\\$"));
		ASSERT_FALSE(RE2::FullMatch("fox", "fo|bar"));

		// Uncomment the following if we change the handling of '$' to
		// prevent it from matching a trailing newline
		if(false) {
			// Check that we don't get bitten by pcre's special handling of a
			// '\n' at the end of the string matching '$'
			ASSERT_FALSE(RE2::PartialMatch("foo\n", "foo$"));
		}
	}

	TEST(RE2, FullMatchArgCount) {
		// Number of args
		int a[16];
		ASSERT_TRUE(RE2::FullMatch("", ""));

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("1", "(\\d){1}", &a[0]));
		ASSERT_EQ(a[0], 1);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("12", "(\\d)(\\d)", &a[0], &a[1]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("123", "(\\d)(\\d)(\\d)", &a[0], &a[1], &a[2]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("1234", "(\\d)(\\d)(\\d)(\\d)", &a[0], &a[1], &a[2], &a[3]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);
		ASSERT_EQ(a[3], 4);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("12345", "(\\d)(\\d)(\\d)(\\d)(\\d)", &a[0], &a[1], &a[2], &a[3], &a[4]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);
		ASSERT_EQ(a[3], 4);
		ASSERT_EQ(a[4], 5);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("123456", "(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);
		ASSERT_EQ(a[3], 4);
		ASSERT_EQ(a[4], 5);
		ASSERT_EQ(a[5], 6);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("1234567", "(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);
		ASSERT_EQ(a[3], 4);
		ASSERT_EQ(a[4], 5);
		ASSERT_EQ(a[5], 6);
		ASSERT_EQ(a[6], 7);

		memset(a, 0, sizeof(0));
		ASSERT_TRUE(RE2::FullMatch("1234567890123456", "(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)(\\d)",
			&a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &a[9], &a[10], &a[11], &a[12], &a[13], &a[14], &a[15]));
		ASSERT_EQ(a[0], 1);
		ASSERT_EQ(a[1], 2);
		ASSERT_EQ(a[2], 3);
		ASSERT_EQ(a[3], 4);
		ASSERT_EQ(a[4], 5);
		ASSERT_EQ(a[5], 6);
		ASSERT_EQ(a[6], 7);
		ASSERT_EQ(a[7], 8);
		ASSERT_EQ(a[8], 9);
		ASSERT_EQ(a[9], 0);
		ASSERT_EQ(a[10], 1);
		ASSERT_EQ(a[11], 2);
		ASSERT_EQ(a[12], 3);
		ASSERT_EQ(a[13], 4);
		ASSERT_EQ(a[14], 5);
		ASSERT_EQ(a[15], 6);
	}

	TEST(RE2, Accessors) {
		// Check the pattern() accessor
		{
			const std::string kPattern = "http://([^/]+)/.*";
			const RE2 re(kPattern);
			ASSERT_EQ(kPattern, re.pattern());
		}

		// Check RE2 error field.
		{
			RE2 re("foo");
			ASSERT_TRUE(re.error().empty()); // Must have no error
			ASSERT_TRUE(re.ok());
			ASSERT_EQ(re.error_code(), RE2::NoError);
		}
	}

	TEST(RE2, UTF8) {
		// Check UTF-8 handling
		// Three Japanese characters (nihongo)
		const char utf8_string[] = {
			(char)0xe6, (char)0x97, (char)0xa5, // 65e5
			(char)0xe6, (char)0x9c, (char)0xac, // 627c
			(char)0xe8, (char)0xaa, (char)0x9e, // 8a9e
			0
		};
		const char utf8_pattern[] = {
			'.',
			(char)0xe6, (char)0x9c, (char)0xac, // 627c
			'.',
			0
		};

		// Both should match in either mode, bytes or UTF-8
		RE2 re_test1(".........", RE2::Latin1);
		ASSERT_TRUE(RE2::FullMatch(utf8_string, re_test1));
		RE2 re_test2("...");
		ASSERT_TRUE(RE2::FullMatch(utf8_string, re_test2));

		// Check that '.' matches one byte or UTF-8 character
		// according to the mode.
		std::string s;
		RE2 re_test3("(.)", RE2::Latin1);
		ASSERT_TRUE(RE2::PartialMatch(utf8_string, re_test3, &s));
		ASSERT_EQ(s, std::string("\xe6"));
		RE2 re_test4("(.)");
		ASSERT_TRUE(RE2::PartialMatch(utf8_string, re_test4, &s));
		ASSERT_EQ(s, std::string("\xe6\x97\xa5"));

		// Check that string matches itself in either mode
		RE2 re_test5(utf8_string, RE2::Latin1);
		ASSERT_TRUE(RE2::FullMatch(utf8_string, re_test5));
		RE2 re_test6(utf8_string);
		ASSERT_TRUE(RE2::FullMatch(utf8_string, re_test6));

		// Check that pattern matches string only in UTF8 mode
		RE2 re_test7(utf8_pattern, RE2::Latin1);
		ASSERT_FALSE(RE2::FullMatch(utf8_string, re_test7));
		RE2 re_test8(utf8_pattern);
		ASSERT_TRUE(RE2::FullMatch(utf8_string, re_test8));
	}

	TEST(RE2, UngreedyUTF8) {
		// Check that ungreedy, UTF8 regular expressions don't match when they
		// oughtn't -- see bug 82246.
		{
			// This code always worked.
			const char* pattern = "\\w+X";
			const std::string target = "a aX";
			RE2 match_sentence(pattern, RE2::Latin1);
			RE2 match_sentence_re(pattern);

			ASSERT_FALSE(RE2::FullMatch(target, match_sentence));
			ASSERT_FALSE(RE2::FullMatch(target, match_sentence_re));
		}
		{
			const char* pattern = "(?U)\\w+X";
			const std::string target = "a aX";
			RE2 match_sentence(pattern, RE2::Latin1);
			ASSERT_EQ(match_sentence.error(), "");
			RE2 match_sentence_re(pattern);

			ASSERT_FALSE(RE2::FullMatch(target, match_sentence));
			ASSERT_FALSE(RE2::FullMatch(target, match_sentence_re));
		}
	}

	TEST(RE2, Rejects) {
		{
			RE2 re("a\\1", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
		{
			RE2 re("a[x", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
		{
			RE2 re("a[z-a]", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
		{
			RE2 re("a[[:foobar:]]", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
		{
			RE2 re("a(b", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
		{
			RE2 re("a\\", RE2::Quiet);
			ASSERT_FALSE(re.ok());
		}
	}

	TEST(RE2, NoCrash) {
		// Test that using a bad regexp doesn't crash.
		{
			RE2 re("a\\", RE2::Quiet);
			ASSERT_FALSE(re.ok());
			ASSERT_FALSE(RE2::PartialMatch("a\\b", re));
		}

		// Test that using an enormous regexp doesn't crash
		{
			RE2 re("(((.{100}){100}){100}){100}", RE2::Quiet);
			ASSERT_FALSE(re.ok());
			ASSERT_FALSE(RE2::PartialMatch("aaa", re));
		}

		// Test that a crazy regexp still compiles and runs.
		{
			RE2 re(".{512}x", RE2::Quiet);
			ASSERT_TRUE(re.ok());
			std::string s;
			s.append(515, 'c');
			s.append("x");
			ASSERT_TRUE(RE2::PartialMatch(s, re));
		}
	}

	TEST(RE2, Recursion) {
		// Test that recursion is stopped.
		// This test is PCRE-legacy -- there's no recursion in RE2.
		int bytes = 15 * 1024; // enough to crash PCRE
		TestRecursion(bytes, ".");
		TestRecursion(bytes, "a");
		TestRecursion(bytes, "a.");
		TestRecursion(bytes, "ab.");
		TestRecursion(bytes, "abc.");
	}

	TEST(RE2, BigCountedRepetition) {
		// Test that counted repetition works, given tons of memory.
		RE2::Options opt;
		opt.set_max_mem(256<<20);

		RE2 re(".{512}x", opt);
		ASSERT_TRUE(re.ok());
		std::string s;
		s.append(515, 'c');
		s.append("x");
		ASSERT_TRUE(RE2::PartialMatch(s, re));
	}

	TEST(RE2, DeepRecursion) {
		// Test for deep stack recursion.  This would fail with a
		// segmentation violation due to stack overflow before pcre was
		// patched.
		// Again, a PCRE legacy test.  RE2 doesn't recurse.
		std::string comment("x*");
		std::string a(131072, 'a');
		comment += a;
		comment += "*x";
		RE2 re("((?:\\s|xx.*\n|x[*](?:\n|.)*?[*]x)*)");
		ASSERT_TRUE(RE2::FullMatch(comment, re));
	}

	// Suggested by Josh Hyman.  Failed when SearchOnePass was
	// not implementing case-folding.
	TEST(CaseInsensitive, MatchAndConsume) {
		std::string text = "A fish named *Wanda*";
		StringPiece sp(text);
		StringPiece result;
		EXPECT_TRUE(RE2::PartialMatch(text, "(?i)([wand]{5})", &result));
		EXPECT_TRUE(RE2::FindAndConsume(&sp, "(?i)([wand]{5})", &result));
	}

	// RE2 should permit implicit conversions from string, StringPiece, const char*,
	// and C string literals.
	TEST(RE2, ImplicitConversions) {
		std::string re_string(".");
		StringPiece re_stringpiece(".");
		const char* re_cstring = ".";
		EXPECT_TRUE(RE2::PartialMatch("e", re_string));
		EXPECT_TRUE(RE2::PartialMatch("e", re_stringpiece));
		EXPECT_TRUE(RE2::PartialMatch("e", re_cstring));
		EXPECT_TRUE(RE2::PartialMatch("e", "."));
	}

	// Bugs introduced by 8622304
	TEST(RE2, CL8622304) {
		// reported by ingow
		std::string dir;
		EXPECT_TRUE(RE2::FullMatch("D", "([^\\\\])")); // ok
		EXPECT_TRUE(RE2::FullMatch("D", "([^\\\\])", &dir)); // fails
		// reported by jacobsa
		std::string key, val;
		EXPECT_TRUE(RE2::PartialMatch("bar:1,0x2F,030,4,5;baz:true;fooby:false,true", "(\\w+)(?::((?:[^;\\\\]|\\\\.)*))?;?", &key, &val));
		EXPECT_EQ(key, "bar");
		EXPECT_EQ(val, "1,0x2F,030,4,5");
	}

	// Check that RE2 returns correct regexp pieces on error.
	// In particular, make sure it returns whole runes
	// and that it always reports invalid UTF-8.
	// Also check that Perl error flag piece is big enough.
	static struct ErrorTest {
		const char * regexp;
		RE2::ErrorCode error_code;
		const char * error_arg;
	} error_tests[] = {
		{ "ab\\αcd", RE2::ErrorBadEscape, "\\α" },
		{ "ef\\x☺01", RE2::ErrorBadEscape, "\\x☺0" },
		{ "gh\\x1☺01", RE2::ErrorBadEscape, "\\x1☺" },
		{ "ij\\x1", RE2::ErrorBadEscape, "\\x1" },
		{ "kl\\x", RE2::ErrorBadEscape, "\\x" },
		{ "uv\\x{0000☺}", RE2::ErrorBadEscape, "\\x{0000☺" },
		{ "wx\\p{ABC", RE2::ErrorBadCharRange, "\\p{ABC" },
		// used to return (?s but the error is X
		{ "yz(?smiUX:abc)", RE2::ErrorBadPerlOp, "(?smiUX" },
		{ "aa(?sm☺i", RE2::ErrorBadPerlOp, "(?sm☺" },
		{ "bb[abc", RE2::ErrorMissingBracket, "[abc" },
		{ "abc(def", RE2::ErrorMissingParen, "abc(def" },
		{ "abc)def", RE2::ErrorUnexpectedParen, "abc)def" },

		// no argument string returned for invalid UTF-8
		{ "mn\\x1\377", RE2::ErrorBadUTF8, "" },
		{ "op\377qr", RE2::ErrorBadUTF8, "" },
		{ "st\\x{00000\377", RE2::ErrorBadUTF8, "" },
		{ "zz\\p{\377}", RE2::ErrorBadUTF8, "" },
		{ "zz\\x{00\377}", RE2::ErrorBadUTF8, "" },
		{ "zz(?P<name\377>abc)", RE2::ErrorBadUTF8, "" },
	};
	TEST(RE2, ErrorCodeAndArg) {
		for(size_t i = 0; i < arraysize(error_tests); i++) {
			RE2 re(error_tests[i].regexp, RE2::Quiet);
			EXPECT_FALSE(re.ok());
			EXPECT_EQ(re.error_code(), error_tests[i].error_code) << re.error();
			EXPECT_EQ(re.error_arg(), error_tests[i].error_arg) << re.error();
		}
	}

	// Check that "never match \n" mode never matches \n.
	static struct NeverTest {
		const char* regexp;
		const char* text;
		const char* match;
	} never_tests[] = {
		{ "(.*)", "abc\ndef\nghi\n", "abc" },
		{ "(?s)(abc.*def)", "abc\ndef\n", NULL },
		{ "(abc(.|\n)*def)", "abc\ndef\n", NULL },
		{ "(abc[^x]*def)", "abc\ndef\n", NULL },
		{ "(abc[^x]*def)", "abczzzdef\ndef\n", "abczzzdef" },
	};
	TEST(RE2, NeverNewline) {
		RE2::Options opt;
		opt.set_never_nl(true);
		for(size_t i = 0; i < arraysize(never_tests); i++) {
			const NeverTest& t = never_tests[i];
			RE2 re(t.regexp, opt);
			if(t.match == NULL) {
				EXPECT_FALSE(re.PartialMatch(t.text, re));
			}
			else {
				StringPiece m;
				EXPECT_TRUE(re.PartialMatch(t.text, re, &m));
				EXPECT_EQ(m, t.match);
			}
		}
	}

	// Check that dot_nl option works.
	TEST(RE2, DotNL) {
		RE2::Options opt;
		opt.set_dot_nl(true);
		EXPECT_TRUE(RE2::PartialMatch("\n", RE2(".", opt)));
		EXPECT_FALSE(RE2::PartialMatch("\n", RE2("(?-s).", opt)));
		opt.set_never_nl(true);
		EXPECT_FALSE(RE2::PartialMatch("\n", RE2(".", opt)));
	}

	// Check that there are no capturing groups in "never capture" mode.
	TEST(RE2, NeverCapture) {
		RE2::Options opt;
		opt.set_never_capture(true);
		RE2 re("(r)(e)", opt);
		EXPECT_EQ(0, re.NumberOfCapturingGroups());
	}

	// Bitstate bug was looking at submatch[0] even if nsubmatch == 0.
	// Triggered by a failed DFA search falling back to Bitstate when
	// using Match with a NULL submatch set.  Bitstate tried to read
	// the submatch[0] entry even if nsubmatch was 0.
	TEST(RE2, BitstateCaptureBug) {
		RE2::Options opt;
		opt.set_max_mem(20000);
		RE2 re("(_________$)", opt);
		StringPiece s = "xxxxxxxxxxxxxxxxxxxxxxxxxx_________x";
		EXPECT_FALSE(re.Match(s, 0, s.size(), RE2::UNANCHORED, NULL, 0));
	}

	// C++ version of bug 609710.
	TEST(RE2, UnicodeClasses) {
		const std::string str = "ABCDEFGHI譚永鋒";
		std::string a, b, c;

		EXPECT_TRUE(RE2::FullMatch("A", "\\p{L}"));
		EXPECT_TRUE(RE2::FullMatch("A", "\\p{Lu}"));
		EXPECT_FALSE(RE2::FullMatch("A", "\\p{Ll}"));
		EXPECT_FALSE(RE2::FullMatch("A", "\\P{L}"));
		EXPECT_FALSE(RE2::FullMatch("A", "\\P{Lu}"));
		EXPECT_TRUE(RE2::FullMatch("A", "\\P{Ll}"));

		EXPECT_TRUE(RE2::FullMatch("譚", "\\p{L}"));
		EXPECT_FALSE(RE2::FullMatch("譚", "\\p{Lu}"));
		EXPECT_FALSE(RE2::FullMatch("譚", "\\p{Ll}"));
		EXPECT_FALSE(RE2::FullMatch("譚", "\\P{L}"));
		EXPECT_TRUE(RE2::FullMatch("譚", "\\P{Lu}"));
		EXPECT_TRUE(RE2::FullMatch("譚", "\\P{Ll}"));

		EXPECT_TRUE(RE2::FullMatch("永", "\\p{L}"));
		EXPECT_FALSE(RE2::FullMatch("永", "\\p{Lu}"));
		EXPECT_FALSE(RE2::FullMatch("永", "\\p{Ll}"));
		EXPECT_FALSE(RE2::FullMatch("永", "\\P{L}"));
		EXPECT_TRUE(RE2::FullMatch("永", "\\P{Lu}"));
		EXPECT_TRUE(RE2::FullMatch("永", "\\P{Ll}"));

		EXPECT_TRUE(RE2::FullMatch("鋒", "\\p{L}"));
		EXPECT_FALSE(RE2::FullMatch("鋒", "\\p{Lu}"));
		EXPECT_FALSE(RE2::FullMatch("鋒", "\\p{Ll}"));
		EXPECT_FALSE(RE2::FullMatch("鋒", "\\P{L}"));
		EXPECT_TRUE(RE2::FullMatch("鋒", "\\P{Lu}"));
		EXPECT_TRUE(RE2::FullMatch("鋒", "\\P{Ll}"));

		EXPECT_TRUE(RE2::PartialMatch(str, "(.).*?(.).*?(.)", &a, &b, &c));
		EXPECT_EQ("A", a);
		EXPECT_EQ("B", b);
		EXPECT_EQ("C", c);

		EXPECT_TRUE(RE2::PartialMatch(str, "(.).*?([\\p{L}]).*?(.)", &a, &b, &c));
		EXPECT_EQ("A", a);
		EXPECT_EQ("B", b);
		EXPECT_EQ("C", c);

		EXPECT_FALSE(RE2::PartialMatch(str, "\\P{L}"));

		EXPECT_TRUE(RE2::PartialMatch(str, "(.).*?([\\p{Lu}]).*?(.)", &a, &b, &c));
		EXPECT_EQ("A", a);
		EXPECT_EQ("B", b);
		EXPECT_EQ("C", c);

		EXPECT_FALSE(RE2::PartialMatch(str, "[^\\p{Lu}\\p{Lo}]"));

		EXPECT_TRUE(RE2::PartialMatch(str, ".*(.).*?([\\p{Lu}\\p{Lo}]).*?(.)", &a, &b, &c));
		EXPECT_EQ("譚", a);
		EXPECT_EQ("永", b);
		EXPECT_EQ("鋒", c);
	}

	TEST(RE2, LazyRE2) {
		// Test with and without options.
		static LazyRE2 a = {"a"};
		static LazyRE2 b = {"b", RE2::Latin1};

		EXPECT_EQ("a", a->pattern());
		EXPECT_EQ(RE2::Options::EncodingUTF8, a->options().encoding());

		EXPECT_EQ("b", b->pattern());
		EXPECT_EQ(RE2::Options::EncodingLatin1, b->options().encoding());
	}

	// Bug reported by saito. 2009/02/17
	TEST(RE2, NullVsEmptyString) {
		RE2 re(".*");
		EXPECT_TRUE(re.ok());

		StringPiece null;
		EXPECT_TRUE(RE2::FullMatch(null, re));

		StringPiece empty("");
		EXPECT_TRUE(RE2::FullMatch(empty, re));
	}

	// Similar to the previous test, check that the null string and the empty
	// string both match, but also that the null string can only provide null
	// submatches whereas the empty string can also provide empty submatches.
	TEST(RE2, NullVsEmptyStringSubmatches) {
		RE2 re("()|(foo)");
		EXPECT_TRUE(re.ok());

		// matches[0] is overall match, [1] is (), [2] is (foo), [3] is nonexistent.
		StringPiece matches[4];

		for(size_t i = 0; i < arraysize(matches); i++)
			matches[i] = "bar";

		StringPiece null;
		EXPECT_TRUE(re.Match(null, 0, null.size(), RE2::UNANCHORED,
			matches, arraysize(matches)));
		for(size_t i = 0; i < arraysize(matches); i++) {
			EXPECT_TRUE(matches[i].data() == NULL); // always null
			EXPECT_TRUE(matches[i].empty());
		}

		for(size_t i = 0; i < arraysize(matches); i++)
			matches[i] = "bar";

		StringPiece empty("");
		EXPECT_TRUE(re.Match(empty, 0, empty.size(), RE2::UNANCHORED,
			matches, arraysize(matches)));
		EXPECT_TRUE(matches[0].data() != NULL); // empty, not null
		EXPECT_TRUE(matches[0].empty());
		EXPECT_TRUE(matches[1].data() != NULL); // empty, not null
		EXPECT_TRUE(matches[1].empty());
		EXPECT_TRUE(matches[2].data() == NULL);
		EXPECT_TRUE(matches[2].empty());
		EXPECT_TRUE(matches[3].data() == NULL);
		EXPECT_TRUE(matches[3].empty());
	}

	// Issue 1816809
	TEST(RE2, Bug1816809) {
		RE2 re("(((((llx((-3)|(4)))(;(llx((-3)|(4))))*))))");
		StringPiece piece("llx-3;llx4");
		std::string x;
		EXPECT_TRUE(RE2::Consume(&piece, re, &x));
	}

	// Issue 3061120
	TEST(RE2, Bug3061120) {
		RE2 re("(?i)\\W");
		EXPECT_FALSE(RE2::PartialMatch("x", re)); // always worked
		EXPECT_FALSE(RE2::PartialMatch("k", re)); // broke because of kelvin
		EXPECT_FALSE(RE2::PartialMatch("s", re)); // broke because of latin long s
	}

	TEST(RE2, CapturingGroupNames) {
		// Opening parentheses annotated with group IDs:
		//      12    3        45   6         7
		RE2 re("((abc)(?P<G2>)|((e+)(?P<G2>.*)(?P<G1>u+)))");
		EXPECT_TRUE(re.ok());
		const std::map<int, std::string>& have = re.CapturingGroupNames();
		std::map<int, std::string> want;
		want[3] = "G2";
		want[6] = "G2";
		want[7] = "G1";
		EXPECT_EQ(want, have);
	}

	TEST(RE2, RegexpToStringLossOfAnchor) {
		EXPECT_EQ(RE2("^[a-c]at", RE2::POSIX).Regexp()->ToString(), "^[a-c]at");
		EXPECT_EQ(RE2("^[a-c]at").Regexp()->ToString(), "(?-m:^)[a-c]at");
		EXPECT_EQ(RE2("ca[t-z]$", RE2::POSIX).Regexp()->ToString(), "ca[t-z]$");
		EXPECT_EQ(RE2("ca[t-z]$").Regexp()->ToString(), "ca[t-z](?-m:$)");
	}

	// Issue 10131674
	TEST(RE2, Bug10131674) {
		// Some of these escapes describe values that do not fit in a byte.
		RE2 re("\\140\\440\\174\\271\\150\\656\\106\\201\\004\\332", RE2::Latin1);
		EXPECT_FALSE(re.ok());
		EXPECT_FALSE(RE2::FullMatch("hello world", re));
	}

	TEST(RE2, Bug18391750) {
		// Stray write past end of match_ in nfa.cc, caught by fuzzing + address sanitizer.
		const char t[] = {
			(char)0x28, (char)0x28, (char)0xfc, (char)0xfc, (char)0x08, (char)0x08,
			(char)0x26, (char)0x26, (char)0x28, (char)0xc2, (char)0x9b, (char)0xc5,
			(char)0xc5, (char)0xd4, (char)0x8f, (char)0x8f, (char)0x69, (char)0x69,
			(char)0xe7, (char)0x29, (char)0x7b, (char)0x37, (char)0x31, (char)0x31,
			(char)0x7d, (char)0xae, (char)0x7c, (char)0x7c, (char)0xf3, (char)0x29,
			(char)0xae, (char)0xae, (char)0x2e, (char)0x2a, (char)0x29, (char)0x00,
		};
		RE2::Options opt;
		opt.set_encoding(RE2::Options::EncodingLatin1);
		opt.set_longest_match(true);
		opt.set_dot_nl(true);
		opt.set_case_sensitive(false);
		RE2 re(t, opt);
		ASSERT_TRUE(re.ok());
		RE2::PartialMatch(t, re);
	}

	TEST(RE2, Bug18458852) {
		// Bug in parser accepting invalid (too large) rune,
		// causing compiler to fail in DCHECK in UTF-8
		// character class code.
		const char b[] = {
			(char)0x28, (char)0x05, (char)0x05, (char)0x41, (char)0x41, (char)0x28,
			(char)0x24, (char)0x5b, (char)0x5e, (char)0xf5, (char)0x87, (char)0x87,
			(char)0x90, (char)0x29, (char)0x5d, (char)0x29, (char)0x29, (char)0x00,
		};
		RE2 re(b);
		ASSERT_FALSE(re.ok());
	}

	TEST(RE2, Bug18523943) {
		// Bug in BitState: case kFailInst failed the match entirely.

		RE2::Options opt;
		const char a[] = {
			(char)0x29, (char)0x29, (char)0x24, (char)0x00,
		};
		const char b[] = {
			(char)0x28, (char)0x0a, (char)0x2a, (char)0x2a, (char)0x29, (char)0x00,
		};
		opt.set_log_errors(false);
		opt.set_encoding(RE2::Options::EncodingLatin1);
		opt.set_posix_syntax(true);
		opt.set_longest_match(true);
		opt.set_literal(false);
		opt.set_never_nl(true);

		RE2 re((const char*)b, opt);
		ASSERT_TRUE(re.ok());
		std::string s1;
		ASSERT_TRUE(RE2::PartialMatch((const char*)a, re, &s1));
	}

	TEST(RE2, Bug21371806) {
		// Bug in parser accepting Unicode groups in Latin-1 mode,
		// causing compiler to fail in DCHECK in prog.cc.

		RE2::Options opt;
		opt.set_encoding(RE2::Options::EncodingLatin1);

		RE2 re("g\\p{Zl}]", opt);
		ASSERT_TRUE(re.ok());
	}

	TEST(RE2, Bug26356109) {
		// Bug in parser caused by factoring of common prefixes in alternations.

		// In the past, this was factored to "a\\C*?[bc]". Thus, the automaton would
		// consume "ab" and then stop (when unanchored) whereas it should consume all
		// of "abc" as per first-match semantics.
		RE2 re("a\\C*?c|a\\C*?b");
		ASSERT_TRUE(re.ok());

		std::string s = "abc";
		StringPiece m;

		ASSERT_TRUE(re.Match(s, 0, s.size(), RE2::UNANCHORED, &m, 1));
		ASSERT_EQ(m, s) << " (UNANCHORED) got m='" << m << "', want '" << s << "'";

		ASSERT_TRUE(re.Match(s, 0, s.size(), RE2::ANCHOR_BOTH, &m, 1));
		ASSERT_EQ(m, s) << " (ANCHOR_BOTH) got m='" << m << "', want '" << s << "'";
	}

	TEST(RE2, Issue104) 
	{
		// RE2::GlobalReplace always advanced by one byte when the empty string was
		// matched, which would clobber any rune that is longer than one byte.

		std::string s = "bc";
		ASSERT_EQ(3, RE2::GlobalReplace(&s, "a*", "d"));
		ASSERT_EQ("dbdcd", s);

		s = "ąć";
		ASSERT_EQ(3, RE2::GlobalReplace(&s, "Ć*", "Ĉ"));
		ASSERT_EQ("ĈąĈćĈ", s);

		s = "人类";
		ASSERT_EQ(3, RE2::GlobalReplace(&s, "大*", "小"));
		ASSERT_EQ("小人小类小", s);
	}

	TEST(RE2, Issue310) 
	{
		// (?:|a)* matched more text than (?:|a)+ did.
		std::string s = "aaa";
		StringPiece m;
		RE2 star("(?:|a)*");
		ASSERT_TRUE(star.Match(s, 0, s.size(), RE2::UNANCHORED, &m, 1));
		ASSERT_EQ(m, "") << " got m='" << m << "', want ''";
		RE2 plus("(?:|a)+");
		ASSERT_TRUE(plus.Match(s, 0, s.size(), RE2::UNANCHORED, &m, 1));
		ASSERT_EQ(m, "") << " got m='" << m << "', want ''";
	}
}  // namespace re2
//
// RE2_ARG_TEST
//
namespace re2 {
	struct SuccessTable {
		const char * value_string;
		int64_t value;
		bool success[6];
	};

	// Test boundary cases for different integral sizes.
	// Specifically I want to make sure that values outside the boundries
	// of an integral type will fail and that negative numbers will fail
	// for unsigned types. The following table contains the boundaries for
	// the various integral types and has entries for whether or not each
	// type can contain the given value.
	const SuccessTable kSuccessTable[] = {
	// string       integer value     i16    u16    i32    u32    i64    u64
	// 0 to 2^7-1
		{ "0",          0,              { true,  true,  true,  true,  true,  true  }},
		{ "127",        127,            { true,  true,  true,  true,  true,  true  }},

	// -1 to -2^7
		{ "-1",         -1,             { true,  false, true,  false, true,  false }},
		{ "-128",       -128,           { true,  false, true,  false, true,  false }},

	// 2^7 to 2^8-1
		{ "128",        128,            { true,  true,  true,  true,  true,  true  }},
		{ "255",        255,            { true,  true,  true,  true,  true,  true  }},

	// 2^8 to 2^15-1
		{ "256",        256,            { true,  true,  true,  true,  true,  true  }},
		{ "32767",      32767,          { true,  true,  true,  true,  true,  true  }},

	// -2^7-1 to -2^15
		{ "-129",       -129,           { true,  false, true,  false, true,  false }},
		{ "-32768",     -32768,         { true,  false, true,  false, true,  false }},

	// 2^15 to 2^16-1
		{ "32768",      32768,          { false, true,  true,  true,  true,  true  }},
		{ "65535",      65535,          { false, true,  true,  true,  true,  true  }},

	// 2^16 to 2^31-1
		{ "65536",      65536,          { false, false, true,  true,  true,  true  }},
		{ "2147483647", 2147483647,     { false, false, true,  true,  true,  true  }},

	// -2^15-1 to -2^31
		{ "-32769",     -32769,         { false, false, true,  false, true,  false }},
		{ "-2147483648", static_cast<int64_t>(0xFFFFFFFF80000000LL), { false, false, true,  false, true,  false }},

	// 2^31 to 2^32-1
		{ "2147483648", 2147483648U,    { false, false, false, true,  true,  true  }},
		{ "4294967295", 4294967295U,    { false, false, false, true,  true,  true  }},

	// 2^32 to 2^63-1
		{ "4294967296", 4294967296LL,   { false, false, false, false, true,  true  }},
		{ "9223372036854775807", 9223372036854775807LL,        { false, false, false, false, true,  true  }},

	// -2^31-1 to -2^63
		{ "-2147483649", -2147483649LL, { false, false, false, false, true,  false }},
		{ "-9223372036854775808", static_cast<int64_t>(0x8000000000000000LL),
		  { false, false, false, false, true,  false }},

	// 2^63 to 2^64-1
		{ "9223372036854775808", static_cast<int64_t>(9223372036854775808ULL), { false, false, false, false, false, true  }},
		{ "18446744073709551615", static_cast<int64_t>(18446744073709551615ULL), { false, false, false, false, false, true  }},

	// >= 2^64
		{ "18446744073709551616", 0,    { false, false, false, false, false, false }},
	};

	const int kNumStrings = arraysize(kSuccessTable);

	// It's ugly to use a macro, but we apparently can't use the EXPECT_EQ
	// macro outside of a TEST block and this seems to be the only way to
	// avoid code duplication.  I can also pull off a couple nice tricks
	// using concatenation for the type I'm checking against.
	#define PARSE_FOR_TYPE(type, column) {                                   \
			type r;                                                                \
			for(int i = 0; i < kNumStrings; ++i) {                                \
				RE2::Arg arg(&r);                                                    \
				const char* const p = kSuccessTable[i].value_string;                 \
				bool retval = arg.Parse(p, strlen(p));                               \
				bool success = kSuccessTable[i].success[column];                     \
				EXPECT_EQ(retval, success)                                           \
					<< "Parsing '" << p << "' for type " #type " should return "     \
					<< success;                                                      \
				if(success) {                                                       \
					EXPECT_EQ(r, (type)kSuccessTable[i].value);                        \
				}                                                                    \
			}                                                                      \
	}

	TEST(RE2ArgTest, Int16Test) { PARSE_FOR_TYPE(int16_t, 0); }
	TEST(RE2ArgTest, Uint16Test) { PARSE_FOR_TYPE(uint16_t, 1); }
	TEST(RE2ArgTest, Int32Test) { PARSE_FOR_TYPE(int32_t, 2); }
	TEST(RE2ArgTest, Uint32Test) { PARSE_FOR_TYPE(uint32_t, 3); }
	TEST(RE2ArgTest, Int64Test) { PARSE_FOR_TYPE(int64_t, 4); }
	TEST(RE2ArgTest, Uint64Test) { PARSE_FOR_TYPE(uint64_t, 5); }

	TEST(RE2ArgTest, ParseFromTest) 
	{
	#if !defined(_MSC_VER)
		struct {
			bool ParseFrom(const char* str, size_t n) {
				LOG(INFO) << "str = " << str << ", n = " << n;
				return true;
			}
		} obj1;
		RE2::Arg arg1(&obj1);
		EXPECT_TRUE(arg1.Parse("one", 3));

		struct {
			bool ParseFrom(const char* str, size_t n) {
				LOG(INFO) << "str = " << str << ", n = " << n;
				return false;
			}

			// Ensure that RE2::Arg works even with overloaded ParseFrom().
			void ParseFrom(const char* str) {}
		} obj2;
		RE2::Arg arg2(&obj2);
		EXPECT_FALSE(arg2.Parse("two", 3));
	#endif
	}
}  // namespace re2
//
// CHARCLASS_TEST
//
namespace re2 {
	struct CCTest {
		struct {
			Rune lo;
			Rune hi;
		} add[10];
		int remove;
		struct {
			Rune lo;
			Rune hi;
		} final[10];
	};

	static CCTest tests[] = {
		{ { { 10, 20 }, {-1} }, -1, { { 10, 20 }, {-1} } },
		{ { { 10, 20 }, { 20, 30 }, {-1} }, -1, { { 10, 30 }, {-1} } },
		{ { { 10, 20 }, { 30, 40 }, { 20, 30 }, {-1} }, -1, { { 10, 40 }, {-1} } },
		{ { { 0, 50 }, { 20, 30 }, {-1} }, -1, { { 0, 50 }, {-1} } },
		{ { { 10, 11 }, { 13, 14 }, { 16, 17 }, { 19, 20 }, { 22, 23 }, {-1} }, -1, { { 10, 11 }, { 13, 14 }, { 16, 17 }, { 19, 20 }, { 22, 23 }, {-1} } },
		{ { { 13, 14 }, { 10, 11 }, { 22, 23 }, { 19, 20 }, { 16, 17 }, {-1} }, -1, { { 10, 11 }, { 13, 14 }, { 16, 17 }, { 19, 20 }, { 22, 23 }, {-1} } },
		{ { { 13, 14 }, { 10, 11 }, { 22, 23 }, { 19, 20 }, { 16, 17 }, {-1} }, -1, { { 10, 11 }, { 13, 14 }, { 16, 17 }, { 19, 20 }, { 22, 23 }, {-1} } },
		{ { { 13, 14 }, { 10, 11 }, { 22, 23 }, { 19, 20 }, { 16, 17 }, { 5, 25 }, {-1} }, -1, { { 5, 25 }, {-1} } },
		{ { { 13, 14 }, { 10, 11 }, { 22, 23 }, { 19, 20 }, { 16, 17 }, { 12, 21 }, {-1} }, -1, { { 10, 23 }, {-1} } },
		// These check boundary cases during negation.
		{ { { 0, Runemax }, {-1} }, -1, { { 0, Runemax }, {-1} } },
		{ { { 0, 50 }, {-1} }, -1, { { 0, 50 }, {-1} } },
		{ { { 50, Runemax }, {-1} }, -1, { { 50, Runemax }, {-1} } },
		// Check RemoveAbove.
		{ { { 50, Runemax }, {-1} }, 255, { { 50, 255 }, {-1} } },
		{ { { 50, Runemax }, {-1} }, 65535, { { 50, 65535 }, {-1} } },
		{ { { 50, Runemax }, {-1} }, Runemax, { { 50, Runemax }, {-1} } },
		{ { { 50, 60 }, { 250, 260 }, { 350, 360 }, {-1} }, 255, { { 50, 60 }, { 250, 255 }, {-1} } },
		{ { { 50, 60 }, {-1} }, 255, { { 50, 60 }, {-1} } },
		{ { { 350, 360 }, {-1} }, 255, { {-1} } },
		{ { {-1} }, 255, { {-1} } },
	};

	template <typename CharClass> static void Broke(const char * desc, const CCTest* t, CharClass* cc) 
	{
		if(t == NULL) {
			printf("\t%s:", desc);
		}
		else {
			printf("\n");
			printf("CharClass added: [%s]", desc);
			for(int k = 0; t->add[k].lo >= 0; k++)
				printf(" %d-%d", t->add[k].lo, t->add[k].hi);
			printf("\n");
			if(t->remove >= 0)
				printf("Removed > %d\n", t->remove);
			printf("\twant:");
			for(int k = 0; t->final[k].lo >= 0; k++)
				printf(" %d-%d", t->final[k].lo, t->final[k].hi);
			printf("\n");
			printf("\thave:");
		}
		for(typename CharClass::iterator it = cc->begin(); it != cc->end(); ++it)
			printf(" %d-%d", it->lo, it->hi);
		printf("\n");
	}

	bool ShouldContain(CCTest * t, int x) 
	{
		for(int j = 0; t->final[j].lo >= 0; j++)
			if(t->final[j].lo <= x && x <= t->final[j].hi)
				return true;
		return false;
	}

	// Helpers to make templated CorrectCC work with both CharClass and CharClassBuilder.

	CharClass* Negate(CharClass * cc) { return cc->Negate(); }
	void Delete(CharClass* cc) { cc->Delete(); }
	void Delete(CharClassBuilder* cc) { delete cc; }

	CharClassBuilder* Negate(CharClassBuilder* cc) 
	{
		CharClassBuilder* ncc = cc->Copy();
		ncc->Negate();
		return ncc;
	}

	template <typename CharClass> bool CorrectCC(CharClass * cc, CCTest * t, const char * desc) 
	{
		typename CharClass::iterator it = cc->begin();
		int size = 0;
		for(int j = 0; t->final[j].lo >= 0; j++, ++it) {
			if(it == cc->end() ||
				it->lo != t->final[j].lo ||
				it->hi != t->final[j].hi) {
				Broke(desc, t, cc);
				return false;
			}
			size += it->hi - it->lo + 1;
		}
		if(it != cc->end()) {
			Broke(desc, t, cc);
			return false;
		}
		if(cc->size() != size) {
			Broke(desc, t, cc);
			printf("wrong size: want %d have %d\n", size, cc->size());
			return false;
		}
		for(int j = 0; j < 101; j++) {
			if(j == 100)
				j = Runemax;
			if(ShouldContain(t, j) != cc->Contains(j)) {
				Broke(desc, t, cc);
				printf("want contains(%d)=%d, got %d\n",
					j, ShouldContain(t, j), cc->Contains(j));
				return false;
			}
		}
		CharClass* ncc = Negate(cc);
		for(int j = 0; j < 101; j++) {
			if(j == 100)
				j = Runemax;
			if(ShouldContain(t, j) == ncc->Contains(j)) {
				Broke(desc, t, cc);
				Broke("ncc", NULL, ncc);
				printf("want ncc contains(%d)!=%d, got %d\n",
					j, ShouldContain(t, j), ncc->Contains(j));
				Delete(ncc);
				return false;
			}
			if(ncc->size() != Runemax+1 - cc->size()) {
				Broke(desc, t, cc);
				Broke("ncc", NULL, ncc);
				printf("ncc size should be %d is %d\n",
					Runemax+1 - cc->size(), ncc->size());
				Delete(ncc);
				return false;
			}
		}
		Delete(ncc);
		return true;
	}

	TEST(TestCharClassBuilder, Adds) 
	{
		int nfail = 0;
		for(size_t i = 0; i < arraysize(tests); i++) {
			CharClassBuilder ccb;
			CCTest* t = &tests[i];
			for(int j = 0; t->add[j].lo >= 0; j++)
				ccb.AddRange(t->add[j].lo, t->add[j].hi);
			if(t->remove >= 0)
				ccb.RemoveAbove(t->remove);
			if(!CorrectCC(&ccb, t, "before copy (CharClassBuilder)"))
				nfail++;
			CharClass* cc = ccb.GetCharClass();
			if(!CorrectCC(cc, t, "before copy (CharClass)"))
				nfail++;
			cc->Delete();

			CharClassBuilder * ccb1 = ccb.Copy();
			if(!CorrectCC(ccb1, t, "after copy (CharClassBuilder)"))
				nfail++;
			cc = ccb.GetCharClass();
			if(!CorrectCC(cc, t, "after copy (CharClass)"))
				nfail++;
			cc->Delete();
			delete ccb1;
		}
		EXPECT_EQ(nfail, 0);
	}
}  // namespace re2
//
// COMPILE_TEST
//
namespace re2 {
	// Simple input/output tests checking that
	// the regexp compiles to the expected code.
	// These are just to sanity check the basic implementation.
	// The real confidence tests happen by testing the NFA/DFA
	// that run the compiled code.

	struct Test_Compile {
		const char* regexp;
		const char* code;
	};

	static Test_Compile tests_compile[] = {
		{ "a",
		  "3. byte [61-61] 0 -> 4\n"
		  "4. match! 0\n" },
		{ "ab",
		  "3. byte [61-61] 0 -> 4\n"
		  "4. byte [62-62] 0 -> 5\n"
		  "5. match! 0\n" },
		{ "a|c",
		  "3+ byte [61-61] 0 -> 5\n"
		  "4. byte [63-63] 0 -> 5\n"
		  "5. match! 0\n" },
		{ "a|b",
		  "3. byte [61-62] 0 -> 4\n"
		  "4. match! 0\n" },
		{ "[ab]",
		  "3. byte [61-62] 0 -> 4\n"
		  "4. match! 0\n" },
		{ "a+",
		  "3. byte [61-61] 0 -> 4\n"
		  "4+ nop -> 3\n"
		  "5. match! 0\n" },
		{ "a+?",
		  "3. byte [61-61] 0 -> 4\n"
		  "4+ match! 0\n"
		  "5. nop -> 3\n" },
		{ "a*",
		  "3+ byte [61-61] 1 -> 3\n"
		  "4. match! 0\n" },
		{ "a*?",
		  "3+ match! 0\n"
		  "4. byte [61-61] 0 -> 3\n" },
		{ "a?",
		  "3+ byte [61-61] 1 -> 5\n"
		  "4. nop -> 5\n"
		  "5. match! 0\n" },
		{ "a??",
		  "3+ nop -> 5\n"
		  "4. byte [61-61] 0 -> 5\n"
		  "5. match! 0\n" },
		{ "a{4}",
		  "3. byte [61-61] 0 -> 4\n"
		  "4. byte [61-61] 0 -> 5\n"
		  "5. byte [61-61] 0 -> 6\n"
		  "6. byte [61-61] 0 -> 7\n"
		  "7. match! 0\n" },
		{ "(a)",
		  "3. capture 2 -> 4\n"
		  "4. byte [61-61] 0 -> 5\n"
		  "5. capture 3 -> 6\n"
		  "6. match! 0\n" },
		{ "(?:a)",
		  "3. byte [61-61] 0 -> 4\n"
		  "4. match! 0\n" },
		{ "",
		  "3. match! 0\n" },
		{ ".",
		  "3+ byte [00-09] 0 -> 5\n"
		  "4. byte [0b-ff] 0 -> 5\n"
		  "5. match! 0\n" },
		{ "[^ab]",
		  "3+ byte [00-09] 0 -> 6\n"
		  "4+ byte [0b-60] 0 -> 6\n"
		  "5. byte [63-ff] 0 -> 6\n"
		  "6. match! 0\n" },
		{ "[Aa]",
		  "3. byte/i [61-61] 0 -> 4\n"
		  "4. match! 0\n" },
		{ "\\C+",
		  "3. byte [00-ff] 0 -> 4\n"
		  "4+ altmatch -> 5 | 6\n"
		  "5+ nop -> 3\n"
		  "6. match! 0\n" },
		{ "\\C*",
		  "3+ altmatch -> 4 | 5\n"
		  "4+ byte [00-ff] 1 -> 3\n"
		  "5. match! 0\n" },
		{ "\\C?",
		  "3+ byte [00-ff] 1 -> 5\n"
		  "4. nop -> 5\n"
		  "5. match! 0\n" },
		// Issue 20992936
		{ "[[-`]",
		  "3. byte [5b-60] 0 -> 4\n"
		  "4. match! 0\n" },
		// Issue 310
		{ "(?:|a)*",
		  "3+ nop -> 7\n"
		  "4. nop -> 9\n"
		  "5+ nop -> 7\n"
		  "6. nop -> 9\n"
		  "7+ nop -> 5\n"
		  "8. byte [61-61] 0 -> 5\n"
		  "9. match! 0\n" },
		{ "(?:|a)+",
		  "3+ nop -> 5\n"
		  "4. byte [61-61] 0 -> 5\n"
		  "5+ nop -> 3\n"
		  "6. match! 0\n" },
	};

	TEST(TestRegexpCompileToProg, Simple) 
	{
		int failed = 0;
		for(size_t i = 0; i < arraysize(tests); i++) {
			const re2::Test_Compile & t = tests_compile[i];
			Regexp* re = Regexp::Parse(t.regexp, Regexp::PerlX|Regexp::Latin1, NULL);
			if(re == NULL) {
				LOG(ERROR) << "Cannot parse: " << t.regexp;
				failed++;
				continue;
			}
			Prog* prog = re->CompileToProg(0);
			if(prog == NULL) {
				LOG(ERROR) << "Cannot compile: " << t.regexp;
				re->Decref();
				failed++;
				continue;
			}
			ASSERT_TRUE(re->CompileToProg(1) == NULL);
			std::string s = prog->Dump();
			if(s != t.code) {
				LOG(ERROR) << "Incorrect compiled code for: " << t.regexp;
				LOG(ERROR) << "Want:\n" << t.code;
				LOG(ERROR) << "Got:\n" << s;
				failed++;
			}
			delete prog;
			re->Decref();
		}
		EXPECT_EQ(failed, 0);
	}

	static void DumpByteMap(StringPiece pattern, Regexp::ParseFlags flags, std::string* bytemap) 
	{
		Regexp* re = Regexp::Parse(pattern, flags, NULL);
		EXPECT_TRUE(re != NULL);
		{
			Prog* prog = re->CompileToProg(0);
			EXPECT_TRUE(prog != NULL);
			*bytemap = prog->DumpByteMap();
			delete prog;
		}
		{
			Prog* prog = re->CompileToReverseProg(0);
			EXPECT_TRUE(prog != NULL);
			EXPECT_EQ(*bytemap, prog->DumpByteMap());
			delete prog;
		}
		re->Decref();
	}

	TEST(TestCompile, Latin1Ranges) 
	{
		// The distinct byte ranges involved in the Latin-1 dot ([^\n]).
		std::string bytemap;
		DumpByteMap(".", Regexp::PerlX|Regexp::Latin1, &bytemap);
		EXPECT_EQ("[00-09] -> 0\n"
			"[0a-0a] -> 1\n"
			"[0b-ff] -> 0\n",
			bytemap);
	}

	TEST(TestCompile, OtherByteMapTests) 
	{
		std::string bytemap;
		// Test that "absent" ranges are mapped to the same byte class.
		DumpByteMap("[0-9A-Fa-f]+", Regexp::PerlX|Regexp::Latin1, &bytemap);
		EXPECT_EQ("[00-2f] -> 0\n"
			"[30-39] -> 1\n"
			"[3a-40] -> 0\n"
			"[41-46] -> 1\n"
			"[47-60] -> 0\n"
			"[61-66] -> 1\n"
			"[67-ff] -> 0\n",
			bytemap);

		// Test the byte classes for \b.
		DumpByteMap("\\b", Regexp::LikePerl|Regexp::Latin1, &bytemap);
		EXPECT_EQ("[00-2f] -> 0\n"
			"[30-39] -> 1\n"
			"[3a-40] -> 0\n"
			"[41-5a] -> 1\n"
			"[5b-5e] -> 0\n"
			"[5f-5f] -> 1\n"
			"[60-60] -> 0\n"
			"[61-7a] -> 1\n"
			"[7b-ff] -> 0\n",
			bytemap);

		// Bug in the ASCII case-folding optimization created too many byte classes.
		DumpByteMap("[^_]", Regexp::LikePerl|Regexp::Latin1, &bytemap);
		EXPECT_EQ("[00-5e] -> 0\n"
			"[5f-5f] -> 1\n"
			"[60-ff] -> 0\n",
			bytemap);
	}

	TEST(TestCompile, UTF8Ranges) {
		// The distinct byte ranges involved in the UTF-8 dot ([^\n]).
		// Once, erroneously split between 0x3f and 0x40 because it is
		// a 6-bit boundary.

		std::string bytemap;

		DumpByteMap(".", Regexp::PerlX, &bytemap);
		EXPECT_EQ("[00-09] -> 0\n"
			"[0a-0a] -> 1\n"
			"[0b-7f] -> 0\n"
			"[80-bf] -> 2\n"
			"[c0-c1] -> 1\n"
			"[c2-df] -> 3\n"
			"[e0-ef] -> 4\n"
			"[f0-f4] -> 5\n"
			"[f5-ff] -> 1\n",
			bytemap);
	}

	TEST(TestCompile, InsufficientMemory) {
		Regexp* re = Regexp::Parse(
			"^(?P<name1>[^\\s]+)\\s+(?P<name2>[^\\s]+)\\s+(?P<name3>.+)$",
			Regexp::LikePerl, NULL);
		EXPECT_TRUE(re != NULL);
		Prog* prog = re->CompileToProg(850);
		// If the memory budget has been exhausted, compilation should fail
		// and return NULL instead of trying to do anything with NoMatch().
		EXPECT_TRUE(prog == NULL);
		re->Decref();
	}

	static void Dump(StringPiece pattern, Regexp::ParseFlags flags,
		std::string* forward, std::string* reverse) {
		Regexp* re = Regexp::Parse(pattern, flags, NULL);
		EXPECT_TRUE(re != NULL);

		if(forward != NULL) {
			Prog* prog = re->CompileToProg(0);
			EXPECT_TRUE(prog != NULL);
			*forward = prog->Dump();
			delete prog;
		}

		if(reverse != NULL) {
			Prog* prog = re->CompileToReverseProg(0);
			EXPECT_TRUE(prog != NULL);
			*reverse = prog->Dump();
			delete prog;
		}

		re->Decref();
	}

	TEST(TestCompile, Bug26705922) {
		// Bug in the compiler caused inefficient bytecode to be generated for Unicode
		// groups: common suffixes were cached, but common prefixes were not factored.

		std::string forward, reverse;

		Dump("[\\x{10000}\\x{10010}]", Regexp::LikePerl, &forward, &reverse);
		EXPECT_EQ("3. byte [f0-f0] 0 -> 4\n"
			"4. byte [90-90] 0 -> 5\n"
			"5. byte [80-80] 0 -> 6\n"
			"6+ byte [80-80] 0 -> 8\n"
			"7. byte [90-90] 0 -> 8\n"
			"8. match! 0\n",
			forward);
		EXPECT_EQ("3+ byte [80-80] 0 -> 5\n"
			"4. byte [90-90] 0 -> 5\n"
			"5. byte [80-80] 0 -> 6\n"
			"6. byte [90-90] 0 -> 7\n"
			"7. byte [f0-f0] 0 -> 8\n"
			"8. match! 0\n",
			reverse);

		Dump("[\\x{8000}-\\x{10FFF}]", Regexp::LikePerl, &forward, &reverse);
		EXPECT_EQ("3+ byte [e8-ef] 0 -> 5\n"
			"4. byte [f0-f0] 0 -> 8\n"
			"5. byte [80-bf] 0 -> 6\n"
			"6. byte [80-bf] 0 -> 7\n"
			"7. match! 0\n"
			"8. byte [90-90] 0 -> 5\n",
			forward);
		EXPECT_EQ("3. byte [80-bf] 0 -> 4\n"
			"4. byte [80-bf] 0 -> 5\n"
			"5+ byte [e8-ef] 0 -> 7\n"
			"6. byte [90-90] 0 -> 8\n"
			"7. match! 0\n"
			"8. byte [f0-f0] 0 -> 7\n",
			reverse);

		Dump("[\\x{80}-\\x{10FFFF}]", Regexp::LikePerl, &forward, &reverse);
		EXPECT_EQ("3+ byte [c2-df] 0 -> 6\n"
			"4+ byte [e0-ef] 0 -> 8\n"
			"5. byte [f0-f4] 0 -> 9\n"
			"6. byte [80-bf] 0 -> 7\n"
			"7. match! 0\n"
			"8. byte [80-bf] 0 -> 6\n"
			"9. byte [80-bf] 0 -> 8\n",
			forward);
		EXPECT_EQ("3. byte [80-bf] 0 -> 4\n"
			"4+ byte [c2-df] 0 -> 6\n"
			"5. byte [80-bf] 0 -> 7\n"
			"6. match! 0\n"
			"7+ byte [e0-ef] 0 -> 6\n"
			"8. byte [80-bf] 0 -> 9\n"
			"9. byte [f0-f4] 0 -> 6\n",
			reverse);
	}

	TEST(TestCompile, Bug35237384) {
		// Bug in the compiler caused inefficient bytecode to be generated for
		// nested nullable subexpressions.

		std::string forward;

		Dump("a**{3,}", Regexp::Latin1|Regexp::NeverCapture, &forward, NULL);
		EXPECT_EQ("3+ byte [61-61] 1 -> 3\n"
			"4. nop -> 5\n"
			"5+ byte [61-61] 1 -> 5\n"
			"6. nop -> 7\n"
			"7+ byte [61-61] 1 -> 7\n"
			"8. match! 0\n",
			forward);

		Dump("(a*|b*)*{3,}", Regexp::Latin1|Regexp::NeverCapture, &forward, NULL);
		EXPECT_EQ("3+ nop -> 28\n"
			"4. nop -> 30\n"
			"5+ byte [61-61] 1 -> 5\n"
			"6. nop -> 32\n"
			"7+ byte [61-61] 1 -> 7\n"
			"8. nop -> 26\n"
			"9+ byte [61-61] 1 -> 9\n"
			"10. nop -> 20\n"
			"11+ byte [62-62] 1 -> 11\n"
			"12. nop -> 20\n"
			"13+ byte [62-62] 1 -> 13\n"
			"14. nop -> 26\n"
			"15+ byte [62-62] 1 -> 15\n"
			"16. nop -> 32\n"
			"17+ nop -> 9\n"
			"18. nop -> 11\n"
			"19. match! 0\n"
			"20+ nop -> 17\n"
			"21. nop -> 19\n"
			"22+ nop -> 7\n"
			"23. nop -> 13\n"
			"24+ nop -> 17\n"
			"25. nop -> 19\n"
			"26+ nop -> 22\n"
			"27. nop -> 24\n"
			"28+ nop -> 5\n"
			"29. nop -> 15\n"
			"30+ nop -> 22\n"
			"31. nop -> 24\n"
			"32+ nop -> 28\n"
			"33. nop -> 30\n",
			forward);

		Dump("((|S.+)+|(|S.+)+|){2}", Regexp::Latin1|Regexp::NeverCapture, &forward, NULL);
		EXPECT_EQ("3+ nop -> 36\n"
			"4+ nop -> 31\n"
			"5. nop -> 33\n"
			"6+ byte [00-09] 0 -> 8\n"
			"7. byte [0b-ff] 0 -> 8\n"
			"8+ nop -> 6\n"
			"9+ nop -> 29\n"
			"10. nop -> 28\n"
			"11+ byte [00-09] 0 -> 13\n"
			"12. byte [0b-ff] 0 -> 13\n"
			"13+ nop -> 11\n"
			"14+ nop -> 26\n"
			"15. nop -> 28\n"
			"16+ byte [00-09] 0 -> 18\n"
			"17. byte [0b-ff] 0 -> 18\n"
			"18+ nop -> 16\n"
			"19+ nop -> 36\n"
			"20. nop -> 33\n"
			"21+ byte [00-09] 0 -> 23\n"
			"22. byte [0b-ff] 0 -> 23\n"
			"23+ nop -> 21\n"
			"24+ nop -> 31\n"
			"25. nop -> 33\n"
			"26+ nop -> 28\n"
			"27. byte [53-53] 0 -> 11\n"
			"28. match! 0\n"
			"29+ nop -> 28\n"
			"30. byte [53-53] 0 -> 6\n"
			"31+ nop -> 33\n"
			"32. byte [53-53] 0 -> 21\n"
			"33+ nop -> 29\n"
			"34+ nop -> 26\n"
			"35. nop -> 28\n"
			"36+ nop -> 33\n"
			"37. byte [53-53] 0 -> 16\n",
			forward);
	}
}  // namespace re2
//
// DFA_TEST
//
static const bool UsingMallocCounter = false;

DEFINE_FLAG(int, size, 8, "log2(number of DFA nodes)");
DEFINE_FLAG(int, repeat, 2, "Repetition count.");
DEFINE_FLAG(int, threads, 4, "number of threads");

namespace re2 {
	static int state_cache_resets = 0;
	static int search_failures = 0;

	struct SetHooks {
		SetHooks() 
		{
			hooks::SetDFAStateCacheResetHook([](const hooks::DFAStateCacheReset&) { ++state_cache_resets; });
			hooks::SetDFASearchFailureHook([](const hooks::DFASearchFailure&) { ++search_failures; });
		}
	} set_hooks;

	// Check that multithreaded access to DFA class works.

	// Helper function: builds entire DFA for prog.
	static void DoBuild(Prog* prog) { ASSERT_TRUE(prog->BuildEntireDFA(Prog::kFirstMatch, nullptr)); }

	TEST(Multithreaded, BuildEntireDFA) 
	{
		// Create regexp with 2^FLAGS_size states in DFA.
		std::string s = "a";
		for(int i = 0; i < GetFlag(FLAGS_size); i++)
			s += "[ab]";
		s += "b";
		Regexp* re = Regexp::Parse(s, Regexp::LikePerl, NULL);
		ASSERT_TRUE(re != NULL);

		// Check that single-threaded code works.
		{
			Prog* prog = re->CompileToProg(0);
			ASSERT_TRUE(prog != NULL);

			std::thread t(DoBuild, prog);
			t.join();

			delete prog;
		}

		// Build the DFA simultaneously in a bunch of threads.
		for(int i = 0; i < GetFlag(FLAGS_repeat); i++) {
			Prog* prog = re->CompileToProg(0);
			ASSERT_TRUE(prog != NULL);

			std::vector<std::thread> threads;
			for(int j = 0; j < GetFlag(FLAGS_threads); j++)
				threads.emplace_back(DoBuild, prog);
			for(int j = 0; j < GetFlag(FLAGS_threads); j++)
				threads[j].join();

			// One more compile, to make sure everything is okay.
			prog->BuildEntireDFA(Prog::kFirstMatch, nullptr);
			delete prog;
		}

		re->Decref();
	}

	// Check that DFA size requirements are followed.
	// BuildEntireDFA will, like SearchDFA, stop building out
	// the DFA once the memory limits are reached.
	TEST(SingleThreaded, BuildEntireDFA) 
	{
		// Create regexp with 2^30 states in DFA.
		Regexp* re = Regexp::Parse("a[ab]{30}b", Regexp::LikePerl, NULL);
		ASSERT_TRUE(re != NULL);
		for(int i = 17; i < 24; i++) {
			int64_t limit = int64_t{1}<<i;
			int64_t usage;
			//int64_t progusage, dfamem;
			{
				testing::MallocCounter m(testing::MallocCounter::THIS_THREAD_ONLY);
				Prog* prog = re->CompileToProg(limit);
				ASSERT_TRUE(prog != NULL);
				//progusage = m.HeapGrowth();
				//dfamem = prog->dfa_mem();
				prog->BuildEntireDFA(Prog::kFirstMatch, nullptr);
				prog->BuildEntireDFA(Prog::kLongestMatch, nullptr);
				usage = m.HeapGrowth();
				delete prog;
			}
			if(UsingMallocCounter) {
				//LOG(INFO) << "limit " << limit << ", "
				//          << "prog usage " << progusage << ", "
				//          << "DFA budget " << dfamem << ", "
				//          << "total " << usage;
				// Tolerate +/- 10%.
				ASSERT_GT(usage, limit*9/10);
				ASSERT_LT(usage, limit*11/10);
			}
		}
		re->Decref();
	}

	// Test that the DFA gets the right result even if it runs
	// out of memory during a search.  The regular expression
	// 0[01]{n}$ matches a binary string of 0s and 1s only if
	// the (n+1)th-to-last character is a 0.  Matching this in
	// a single forward pass (as done by the DFA) requires
	// keeping one bit for each of the last n+1 characters
	// (whether each was a 0), or 2^(n+1) possible states.
	// If we run this regexp to search in a string that contains
	// every possible n-character binary string as a substring,
	// then it will have to run through at least 2^n states.
	// States are big data structures -- certainly more than 1 byte --
	// so if the DFA can search correctly while staying within a
	// 2^n byte limit, it must be handling out-of-memory conditions
	// gracefully.
	TEST(SingleThreaded, SearchDFA) {
		// The De Bruijn string is the worst case input for this regexp.
		// By default, the DFA will notice that it is flushing its cache
		// too frequently and will bail out early, so that RE2 can use the
		// NFA implementation instead.  (The DFA loses its speed advantage
		// if it can't get a good cache hit rate.)
		// Tell the DFA to trudge along instead.
		Prog::TESTING_ONLY_set_dfa_should_bail_when_slow(false);
		state_cache_resets = 0;
		search_failures = 0;

		// Choice of n is mostly arbitrary, except that:
		//   * making n too big makes the test run for too long.
		//   * making n too small makes the DFA refuse to run,
		//     because it has so little memory compared to the program size.
		// Empirically, n = 18 is a good compromise between the two.
		const int n = 18;

		Regexp* re = Regexp::Parse(StringPrintf("0[01]{%d}$", n),
			Regexp::LikePerl, NULL);
		ASSERT_TRUE(re != NULL);

		// The De Bruijn string for n ends with a 1 followed by n 0s in a row,
		// which is not a match for 0[01]{n}$.  Adding one more 0 is a match.
		std::string no_match = DeBruijnString(n);
		std::string match = no_match + "0";

		int64_t usage;
		int64_t peak_usage;
		{
			testing::MallocCounter m(testing::MallocCounter::THIS_THREAD_ONLY);
			Prog* prog = re->CompileToProg(1<<n);
			ASSERT_TRUE(prog != NULL);
			for(int i = 0; i < 10; i++) {
				bool matched = false;
				bool failed = false;
				matched = prog->SearchDFA(match, StringPiece(), Prog::kUnanchored,
					Prog::kFirstMatch, NULL, &failed, NULL);
				ASSERT_FALSE(failed);
				ASSERT_TRUE(matched);
				matched = prog->SearchDFA(no_match, StringPiece(), Prog::kUnanchored,
					Prog::kFirstMatch, NULL, &failed, NULL);
				ASSERT_FALSE(failed);
				ASSERT_FALSE(matched);
			}
			usage = m.HeapGrowth();
			peak_usage = m.PeakHeapGrowth();
			delete prog;
		}
		if(UsingMallocCounter) {
			//LOG(INFO) << "usage " << usage << ", "
			//          << "peak usage " << peak_usage;
			ASSERT_LT(usage, 1<<n);
			ASSERT_LT(peak_usage, 1<<n);
		}
		re->Decref();

		// Reset to original behaviour.
		Prog::TESTING_ONLY_set_dfa_should_bail_when_slow(true);
		ASSERT_GT(state_cache_resets, 0);
		ASSERT_EQ(search_failures, 0);
	}

	// Helper function: searches for match, which should match,
	// and no_match, which should not.
	static void DoSearch(Prog* prog, const StringPiece& match,
		const StringPiece& no_match) {
		for(int i = 0; i < 2; i++) {
			bool matched = false;
			bool failed = false;
			matched = prog->SearchDFA(match, StringPiece(), Prog::kUnanchored,
				Prog::kFirstMatch, NULL, &failed, NULL);
			ASSERT_FALSE(failed);
			ASSERT_TRUE(matched);
			matched = prog->SearchDFA(no_match, StringPiece(), Prog::kUnanchored,
				Prog::kFirstMatch, NULL, &failed, NULL);
			ASSERT_FALSE(failed);
			ASSERT_FALSE(matched);
		}
	}

	TEST(Multithreaded, SearchDFA) 
	{
		Prog::TESTING_ONLY_set_dfa_should_bail_when_slow(false);
		state_cache_resets = 0;
		search_failures = 0;

		// Same as single-threaded test above.
		const int n = 18;
		Regexp* re = Regexp::Parse(StringPrintf("0[01]{%d}$", n), Regexp::LikePerl, NULL);
		ASSERT_TRUE(re != NULL);
		std::string no_match = DeBruijnString(n);
		std::string match = no_match + "0";
		// Check that single-threaded code works.
		{
			Prog* prog = re->CompileToProg(1<<n);
			ASSERT_TRUE(prog != NULL);
			std::thread t(DoSearch, prog, match, no_match);
			t.join();
			delete prog;
		}
		// Run the search simultaneously in a bunch of threads.
		// Reuse same flags for Multithreaded.BuildDFA above.
		for(int i = 0; i < GetFlag(FLAGS_repeat); i++) {
			Prog* prog = re->CompileToProg(1<<n);
			ASSERT_TRUE(prog != NULL);
			std::vector<std::thread> threads;
			for(int j = 0; j < GetFlag(FLAGS_threads); j++)
				threads.emplace_back(DoSearch, prog, match, no_match);
			for(int j = 0; j < GetFlag(FLAGS_threads); j++)
				threads[j].join();
			delete prog;
		}
		re->Decref();
		// Reset to original behaviour.
		Prog::TESTING_ONLY_set_dfa_should_bail_when_slow(true);
		ASSERT_GT(state_cache_resets, 0);
		ASSERT_EQ(search_failures, 0);
	}

	struct ReverseTest {
		const char* regexp;
		const char* text;
		bool match;
	};

	// Test that reverse DFA handles anchored/unanchored correctly.
	// It's in the DFA interface but not used by RE2.
	ReverseTest reverse_tests[] = {
		{ "\\A(a|b)", "abc", true },
		{ "(a|b)\\z", "cba", true },
		{ "\\A(a|b)", "cba", false },
		{ "(a|b)\\z", "abc", false },
	};

	TEST(DFA, ReverseMatch) 
	{
		int nfail = 0;
		for(size_t i = 0; i < arraysize(reverse_tests); i++) {
			const ReverseTest& t = reverse_tests[i];
			Regexp* re = Regexp::Parse(t.regexp, Regexp::LikePerl, NULL);
			ASSERT_TRUE(re != NULL);
			Prog* prog = re->CompileToReverseProg(0);
			ASSERT_TRUE(prog != NULL);
			bool failed = false;
			bool matched = prog->SearchDFA(t.text, StringPiece(), Prog::kUnanchored,
				Prog::kFirstMatch, NULL, &failed, NULL);
			if(matched != t.match) {
				LOG(ERROR) << t.regexp << " on " << t.text << ": want " << t.match;
				nfail++;
			}
			delete prog;
			re->Decref();
		}
		EXPECT_EQ(nfail, 0);
	}

	struct CallbackTest {
		const char* regexp;
		const char* dump;
	};

	// Test that DFA::BuildAllStates() builds the expected DFA states
	// and issues the expected callbacks. These test cases reflect the
	// very compact encoding of the callbacks, but that also makes them
	// very difficult to understand, so let's work through "\\Aa\\z".
	// There are three slots per DFA state because the bytemap has two
	// equivalence classes and there is a third slot for kByteEndText:
	//   0: all bytes that are not 'a'
	//   1: the byte 'a'
	//   2: kByteEndText
	// -1 means that there is no transition from that DFA state to any
	// other DFA state for that slot. The valid transitions are thus:
	//   state 0 --slot 1--> state 1
	//   state 1 --slot 2--> state 2
	// The double brackets indicate that state 2 is a matching state.
	// Putting it together, this means that the DFA must consume the
	// byte 'a' and then hit end of text. Q.E.D.
	CallbackTest callback_tests[] = {
		{ "\\Aa\\z", "[-1,1,-1] [-1,-1,2] [[-1,-1,-1]]" },
		{ "\\Aab\\z", "[-1,1,-1,-1] [-1,-1,2,-1] [-1,-1,-1,3] [[-1,-1,-1,-1]]" },
		{ "\\Aa*b\\z", "[-1,0,1,-1] [-1,-1,-1,2] [[-1,-1,-1,-1]]" },
		{ "\\Aa+b\\z", "[-1,1,-1,-1] [-1,1,2,-1] [-1,-1,-1,3] [[-1,-1,-1,-1]]" },
		{ "\\Aa?b\\z", "[-1,1,2,-1] [-1,-1,2,-1] [-1,-1,-1,3] [[-1,-1,-1,-1]]" },
		{ "\\Aa\\C*\\z", "[-1,1,-1] [1,1,2] [[-1,-1,-1]]" },
		{ "\\Aa\\C*", "[-1,1,-1] [2,2,3] [[2,2,2]] [[-1,-1,-1]]" },
		{ "a\\C*", "[0,1,-1] [2,2,3] [[2,2,2]] [[-1,-1,-1]]" },
		{ "\\C*", "[1,2] [[1,1]] [[-1,-1]]" },
		{ "a", "[0,1,-1] [2,2,2] [[-1,-1,-1]]"},
	};

	TEST(DFA, Callback) {
		int nfail = 0;
		for(size_t i = 0; i < arraysize(callback_tests); i++) {
			const CallbackTest& t = callback_tests[i];
			Regexp* re = Regexp::Parse(t.regexp, Regexp::LikePerl, NULL);
			ASSERT_TRUE(re != NULL);
			Prog* prog = re->CompileToProg(0);
			ASSERT_TRUE(prog != NULL);
			std::string dump;
			prog->BuildEntireDFA(Prog::kLongestMatch, [&](const int* next, bool match) {
					ASSERT_TRUE(next != NULL);
					if(!dump.empty())
						dump += " ";
					dump += match ? "[[" : "[";
					for(int b = 0; b < prog->bytemap_range() + 1; b++)
						dump += StringPrintf("%d,", next[b]);
					dump.pop_back();
					dump += match ? "]]" : "]";
				});
			if(dump != t.dump) {
				LOG(ERROR) << t.regexp << " bytemap:\n" << prog->DumpByteMap();
				LOG(ERROR) << t.regexp << " dump:\ngot " << dump << "\nwant " << t.dump;
				nfail++;
			}
			delete prog;
			re->Decref();
		}
		EXPECT_EQ(nfail, 0);
	}
}  // namespace re2
//
// SIMPLIFY_TEST
//
namespace re2 {
	struct Test_Simplify {
		const char* regexp;
		const char* simplified;
	};

	static Test_Simplify tests_simplify[] = {
		// Already-simple constructs
		{ "a", "a" },
		{ "ab", "ab" },
		{ "a|b", "[a-b]" },
		{ "ab|cd", "ab|cd" },
		{ "(ab)*", "(ab)*" },
		{ "(ab)+", "(ab)+" },
		{ "(ab)?", "(ab)?" },
		{ ".", "." },
		{ "^", "^" },
		{ "$", "$" },
		{ "[ac]", "[ac]" },
		{ "[^ac]", "[^ac]" },

		// Posix character classes
		{ "[[:alnum:]]", "[0-9A-Za-z]" },
		{ "[[:alpha:]]", "[A-Za-z]" },
		{ "[[:blank:]]", "[\\t ]" },
		{ "[[:cntrl:]]", "[\\x00-\\x1f\\x7f]" },
		{ "[[:digit:]]", "[0-9]" },
		{ "[[:graph:]]", "[!-~]" },
		{ "[[:lower:]]", "[a-z]" },
		{ "[[:print:]]", "[ -~]" },
		{ "[[:punct:]]", "[!-/:-@\\[-`{-~]" },
		{ "[[:space:]]", "[\\t-\\r ]" },
		{ "[[:upper:]]", "[A-Z]" },
		{ "[[:xdigit:]]", "[0-9A-Fa-f]" },

		// Perl character classes
		{ "\\d", "[0-9]" },
		{ "\\s", "[\\t-\\n\\f-\\r ]" },
		{ "\\w", "[0-9A-Z_a-z]" },
		{ "\\D", "[^0-9]" },
		{ "\\S", "[^\\t-\\n\\f-\\r ]" },
		{ "\\W", "[^0-9A-Z_a-z]" },
		{ "[\\d]", "[0-9]" },
		{ "[\\s]", "[\\t-\\n\\f-\\r ]" },
		{ "[\\w]", "[0-9A-Z_a-z]" },
		{ "[\\D]", "[^0-9]" },
		{ "[\\S]", "[^\\t-\\n\\f-\\r ]" },
		{ "[\\W]", "[^0-9A-Z_a-z]" },

		// Posix repetitions
		{ "a{1}", "a" },
		{ "a{2}", "aa" },
		{ "a{5}", "aaaaa" },
		{ "a{0,1}", "a?" },
		// The next three are illegible because Simplify inserts (?:)
		// parens instead of () parens to avoid creating extra
		// captured subexpressions.  The comments show a version fewer parens.
		{ "(a){0,2}",                   "(?:(a)(a)?)?"     },//       (aa?)?
		{ "(a){0,4}",       "(?:(a)(?:(a)(?:(a)(a)?)?)?)?" },//   (a(a(aa?)?)?)?
		{ "(a){2,6}", "(a)(a)(?:(a)(?:(a)(?:(a)(a)?)?)?)?" }, // aa(a(a(aa?)?)?)?
		{ "a{0,2}",           "(?:aa?)?"     },//       (aa?)?
		{ "a{0,4}",   "(?:a(?:a(?:aa?)?)?)?" },//   (a(a(aa?)?)?)?
		{ "a{2,6}", "aa(?:a(?:a(?:aa?)?)?)?" }, // aa(a(a(aa?)?)?)?
		{ "a{0,}", "a*" },
		{ "a{1,}", "a+" },
		{ "a{2,}", "aa+" },
		{ "a{5,}", "aaaaa+" },

		// Test that operators simplify their arguments.
		// (Simplify used to not simplify arguments to a {} repeat.)
		{ "(?:a{1,}){1,}", "a+" },
		{ "(a{1,}b{1,})", "(a+b+)" },
		{ "a{1,}|b{1,}", "a+|b+" },
		{ "(?:a{1,})*", "(?:a+)*" },
		{ "(?:a{1,})+", "a+" },
		{ "(?:a{1,})?", "(?:a+)?" },
		{ "a{0}", "" },

		// Character class simplification
		{ "[ab]", "[a-b]" },
		{ "[a-za-za-z]", "[a-z]" },
		{ "[A-Za-zA-Za-z]", "[A-Za-z]" },
		{ "[ABCDEFGH]", "[A-H]" },
		{ "[AB-CD-EF-GH]", "[A-H]" },
		{ "[W-ZP-XE-R]", "[E-Z]" },
		{ "[a-ee-gg-m]", "[a-m]" },
		{ "[a-ea-ha-m]", "[a-m]" },
		{ "[a-ma-ha-e]", "[a-m]" },
		{ "[a-zA-Z0-9 -~]", "[ -~]" },

		// Empty character classes
		{ "[^[:cntrl:][:^cntrl:]]", "[^\\x00-\\x{10ffff}]" },

		// Full character classes
		{ "[[:cntrl:][:^cntrl:]]", "." },

		// Unicode case folding.
		{ "(?i)A", "[Aa]" },
		{ "(?i)a", "[Aa]" },
		{ "(?i)K", "[Kk\\x{212a}]" },
		{ "(?i)k", "[Kk\\x{212a}]" },
		{ "(?i)\\x{212a}", "[Kk\\x{212a}]" },
		{ "(?i)[a-z]", "[A-Za-z\\x{17f}\\x{212a}]" },
		{ "(?i)[\\x00-\\x{FFFD}]", "[\\x00-\\x{fffd}]" },
		{ "(?i)[\\x00-\\x{10ffff}]", "." },

		// Empty string as a regular expression.
		// Empty string must be preserved inside parens in order
		// to make submatches work right, so these are less
		// interesting than they used to be.  ToString inserts
		// explicit (?:) in place of non-parenthesized empty strings,
		// to make them easier to spot for other parsers.
		{ "(a|b|)", "([a-b]|(?:))" },
		{ "(|)", "((?:)|(?:))" },
		{ "a()", "a()" },
		{ "(()|())", "(()|())" },
		{ "(a|)", "(a|(?:))" },
		{ "ab()cd()", "ab()cd()" },
		{ "()", "()" },
		{ "()*", "()*" },
		{ "()+", "()+" },
		{ "()?", "()?" },
		{ "(){0}", "" },
		{ "(){1}", "()" },
		{ "(){1,}", "()+" },
		{ "(){0,2}", "(?:()()?)?" },

		// Test that coalescing occurs and that the resulting repeats are simplified.
		// Two-op combinations of *, +, ?, {n}, {n,} and {n,m} with a literal:
		{ "a*a*", "a*" },
		{ "a*a+", "a+" },
		{ "a*a?", "a*" },
		{ "a*a{2}", "aa+" },
		{ "a*a{2,}", "aa+" },
		{ "a*a{2,3}", "aa+" },
		{ "a+a*", "a+" },
		{ "a+a+", "aa+" },
		{ "a+a?", "a+" },
		{ "a+a{2}", "aaa+" },
		{ "a+a{2,}", "aaa+" },
		{ "a+a{2,3}", "aaa+" },
		{ "a?a*", "a*" },
		{ "a?a+", "a+" },
		{ "a?a?", "(?:aa?)?" },
		{ "a?a{2}", "aaa?" },
		{ "a?a{2,}", "aa+" },
		{ "a?a{2,3}", "aa(?:aa?)?" },
		{ "a{2}a*", "aa+" },
		{ "a{2}a+", "aaa+" },
		{ "a{2}a?", "aaa?" },
		{ "a{2}a{2}", "aaaa" },
		{ "a{2}a{2,}", "aaaa+" },
		{ "a{2}a{2,3}", "aaaaa?" },
		{ "a{2,}a*", "aa+" },
		{ "a{2,}a+", "aaa+" },
		{ "a{2,}a?", "aa+" },
		{ "a{2,}a{2}", "aaaa+" },
		{ "a{2,}a{2,}", "aaaa+" },
		{ "a{2,}a{2,3}", "aaaa+" },
		{ "a{2,3}a*", "aa+" },
		{ "a{2,3}a+", "aaa+" },
		{ "a{2,3}a?", "aa(?:aa?)?" },
		{ "a{2,3}a{2}", "aaaaa?" },
		{ "a{2,3}a{2,}", "aaaa+" },
		{ "a{2,3}a{2,3}", "aaaa(?:aa?)?" },
		// With a char class, any char and any byte:
		{ "\\d*\\d*", "[0-9]*" },
		{ ".*.*", ".*" },
		{ "\\C*\\C*", "\\C*" },
		// FoldCase works, but must be consistent:
		{ "(?i)A*a*", "[Aa]*" },
		{ "(?i)a+A+", "[Aa][Aa]+" },
		{ "(?i)A*(?-i)a*", "[Aa]*a*" },
		{ "(?i)a+(?-i)A+", "[Aa]+A+" },
		// NonGreedy works, but must be consistent:
		{ "a*?a*?", "a*?" },
		{ "a+?a+?", "aa+?" },
		{ "a*?a*", "a*?a*" },
		{ "a+a+?", "a+a+?" },
		// The second element is the literal, char class, any char or any byte:
		{ "a*a", "a+" },
		{ "\\d*\\d", "[0-9]+" },
		{ ".*.", ".+" },
		{ "\\C*\\C", "\\C+" },
		// FoldCase works, but must be consistent:
		{ "(?i)A*a", "[Aa]+" },
		{ "(?i)a+A", "[Aa][Aa]+" },
		{ "(?i)A*(?-i)a", "[Aa]*a" },
		{ "(?i)a+(?-i)A", "[Aa]+A" },
		// The second element is a literal string that begins with the literal:
		{ "a*aa", "aa+" },
		{ "a*aab", "aa+b" },
		// FoldCase works, but must be consistent:
		{ "(?i)a*aa", "[Aa][Aa]+" },
		{ "(?i)a*aab", "[Aa][Aa]+[Bb]" },
		{ "(?i)a*(?-i)aa", "[Aa]*aa" },
		{ "(?i)a*(?-i)aab", "[Aa]*aab" },
		// Negative tests with mismatching ops:
		{ "a*b*", "a*b*" },
		{ "\\d*\\D*", "[0-9]*[^0-9]*" },
		{ "a+b", "a+b" },
		{ "\\d+\\D", "[0-9]+[^0-9]" },
		{ "a?bb", "a?bb" },
		// Negative tests with capturing groups:
		{ "(a*)a*", "(a*)a*" },
		{ "a+(a)", "a+(a)" },
		{ "(a?)(aa)", "(a?)(aa)" },
		// Just for fun:
		{ "aa*aa+aa?aa{2}aaa{2,}aaa{2,3}a", "aaaaaaaaaaaaaaaa+" },

		// During coalescing, the child of the repeat changes, so we build a new
		// repeat. The new repeat must have the min and max of the old repeat.
		// Failure to copy them results in min=0 and max=0 -> empty match.
		{ "(?:a*aab){2}", "aa+baa+b" },

		// During coalescing, the child of the capture changes, so we build a new
		// capture. The new capture must have the cap of the old capture.
		// Failure to copy it results in cap=0 -> ToString() logs a fatal error.
		{ "(a*aab)", "(aa+b)" },

		// Test squashing of **, ++, ?? et cetera.
		{ "(?:(?:a){0,}){0,}", "a*" },
		{ "(?:(?:a){1,}){1,}", "a+" },
		{ "(?:(?:a){0,1}){0,1}", "a?" },
		{ "(?:(?:a){0,}){1,}", "a*" },
		{ "(?:(?:a){0,}){0,1}", "a*" },
		{ "(?:(?:a){1,}){0,}", "a*" },
		{ "(?:(?:a){1,}){0,1}", "a*" },
		{ "(?:(?:a){0,1}){0,}", "a*" },
		{ "(?:(?:a){0,1}){1,}", "a*" },
	};

	TEST(TestSimplify, SimpleRegexps) 
	{
		for(size_t i = 0; i < arraysize(tests_simplify); i++) {
			RegexpStatus status;
			VLOG(1) << "Testing " << tests_simplify[i].regexp;
			Regexp* re = Regexp::Parse(tests_simplify[i].regexp, Regexp::MatchNL | (Regexp::LikePerl & ~Regexp::OneLine), &status);
			ASSERT_TRUE(re != NULL) << " " << tests_simplify[i].regexp << " " << status.Text();
			Regexp* sre = re->Simplify();
			ASSERT_TRUE(sre != NULL);
			// Check that already-simple regexps don't allocate new ones.
			if(strcmp(tests_simplify[i].regexp, tests_simplify[i].simplified) == 0) {
				ASSERT_TRUE(re == sre) << " " << tests_simplify[i].regexp << " " << re->ToString() << " " << sre->ToString();
			}
			EXPECT_EQ(tests_simplify[i].simplified, sre->ToString()) << " " << tests_simplify[i].regexp << " " << sre->Dump();
			re->Decref();
			sre->Decref();
		}
	}
}  // namespace re2
//
// RANDOM_TEST
//
DEFINE_FLAG(int, regexpseed, 404, "Random regexp seed.");
DEFINE_FLAG(int, regexpcount, 100, "How many random regexps to generate.");
DEFINE_FLAG(int, stringseed, 200, "Random string seed.");
DEFINE_FLAG(int, stringcount, 100, "How many random strings to generate.");

namespace re2 {
	// Runs a random test on the given parameters.
	// (Always uses the same random seeds for reproducibility.
	// Can give different seeds on command line.)
	static void RandomTest(int maxatoms, int maxops, const std::vector<std::string>& alphabet, const std::vector<std::string>& ops,
		int maxstrlen, const std::vector<std::string>& stralphabet, const std::string& wrapper) 
	{
		// Limit to smaller test cases in debug mode,
		// because everything is so much slower.
		if(RE2_DEBUG_MODE) {
			maxatoms--;
			maxops--;
			maxstrlen /= 2;
		}
		ExhaustiveTester t(maxatoms, maxops, alphabet, ops, maxstrlen, stralphabet, wrapper, "");
		t.RandomStrings(GetFlag(FLAGS_stringseed), GetFlag(FLAGS_stringcount));
		t.GenerateRandom(GetFlag(FLAGS_regexpseed), GetFlag(FLAGS_regexpcount));
		printf("%d regexps, %d tests, %d failures [%d/%d str]\n", t.regexps(), t.tests(), t.failures(), maxstrlen, (int)stralphabet.size());
		EXPECT_EQ(0, t.failures());
	}

	// Tests random small regexps involving literals and egrep operators.
	TEST(Random, SmallEgrepLiterals) { RandomTest(5, 5, Explode("abc."), RegexpGenerator::EgrepOps(), 15, Explode("abc"), ""); }

	// Tests random bigger regexps involving literals and egrep operators.
	TEST(Random, BigEgrepLiterals) { RandomTest(10, 10, Explode("abc."), RegexpGenerator::EgrepOps(), 15, Explode("abc"), ""); }

	// Tests random small regexps involving literals, capturing parens,
	// and egrep operators.
	TEST(Random, SmallEgrepCaptures) { RandomTest(5, 5, Split(" ", "a (b) ."), RegexpGenerator::EgrepOps(), 15, Explode("abc"), ""); }

	// Tests random bigger regexps involving literals, capturing parens,
	// and egrep operators.
	TEST(Random, BigEgrepCaptures) { RandomTest(10, 10, Split(" ", "a (b) ."), RegexpGenerator::EgrepOps(), 15, Explode("abc"), ""); }

	// Tests random large complicated expressions, using all the possible
	// operators, some literals, some parenthesized literals, and predefined
	// character classes like \d.  (Adding larger character classes would
	// make for too many possibilities.)
	TEST(Random, Complicated) 
	{
		std::vector<std::string> ops = Split(" ",
			"%s%s %s|%s %s* %s*? %s+ %s+? %s? %s?? "
			"%s{0} %s{0,} %s{1} %s{1,} %s{0,1} %s{0,2} %s{1,2} "
			"%s{2} %s{2,} %s{3,4} %s{4,5}");

		// Use (?:\b) and (?:\B) instead of \b and \B,
		// because PCRE rejects \b* but accepts (?:\b)*.
		// Ditto ^ and $.
		std::vector<std::string> atoms = Split(" ",
			". (?:^) (?:$) \\a \\f \\n \\r \\t \\v "
			"\\d \\D \\s \\S \\w \\W (?:\\b) (?:\\B) "
			"a (a) b c - \\\\");
		std::vector<std::string> alphabet = Explode("abc123\001\002\003\t\r\n\v\f\a");
		RandomTest(10, 10, atoms, ops, 20, alphabet, "");
	}
}  // namespace re2
