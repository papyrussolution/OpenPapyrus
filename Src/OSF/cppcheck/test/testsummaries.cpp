/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <cppcheck-test-internal.h>
#pragma hdrstop
#include "summaries.h"

class TestSummaries : public TestFixture {
public:
	TestSummaries() : TestFixture("TestSummaries") {
	}

private:
	void run() override {
		TEST_CASE(createSummaries1);
		TEST_CASE(createSummariesGlobal);
		TEST_CASE(createSummariesNoreturn);
	}

#define createSummaries(...) createSummaries_(__FILE__, __LINE__, __VA_ARGS__)
	std::string createSummaries_(const char* file, int line, const char code[], const char filename[] = "test.cpp") {
		// Clear the error buffer..
		errout.str("");

		// tokenize..
		Settings settings;
		Tokenizer tokenizer(&settings, this);
		std::istringstream istr(code);
		ASSERT_LOC(tokenizer.tokenize(istr, filename), file, line);
		return Summaries::create(&tokenizer, "");
	}

	void createSummaries1() {
		ASSERT_EQUALS("foo\n", createSummaries("void foo() {}"));
	}

	void createSummariesGlobal() {
		ASSERT_EQUALS("foo global:[x]\n", createSummaries("int x; void foo() { x=0; }"));
	}

	void createSummariesNoreturn() {
		ASSERT_EQUALS("foo call:[bar] noreturn:[bar]\n", createSummaries("void foo() { bar(); }"));
	}
};

REGISTER_TEST(TestSummaries)
