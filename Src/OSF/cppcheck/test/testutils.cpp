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

const Settings givenACodeSampleToTokenize::settings;

class TestUtils : public TestFixture {
public:
	TestUtils() : TestFixture("TestUtils") {
	}

private:
	void run() override {
		TEST_CASE(isValidGlobPattern);
		TEST_CASE(matchglob);
	}

	void isValidGlobPattern() const {
		ASSERT_EQUALS(true, ::isValidGlobPattern("*"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("*x"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("x*"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("*/x/*"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("x/*/z"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("**"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("**x"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("x**"));

		ASSERT_EQUALS(true, ::isValidGlobPattern("?"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("?x"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("x?"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("?/x/?"));
		ASSERT_EQUALS(true, ::isValidGlobPattern("x/?/z"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("??"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("??x"));
		ASSERT_EQUALS(false, ::isValidGlobPattern("x??"));
	}

	void matchglob() const {
		ASSERT_EQUALS(true, ::matchglob("*", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("x*", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("*z", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("*y*", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("*y*", "yz"));
		ASSERT_EQUALS(false, ::matchglob("*y*", "abc"));
		ASSERT_EQUALS(true, ::matchglob("*", "x/y/z"));
		ASSERT_EQUALS(true, ::matchglob("*/y/z", "x/y/z"));

		ASSERT_EQUALS(false, ::matchglob("?", "xyz"));
		ASSERT_EQUALS(false, ::matchglob("x?", "xyz"));
		ASSERT_EQUALS(false, ::matchglob("?z", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("?y?", "xyz"));
		ASSERT_EQUALS(true, ::matchglob("?/?/?", "x/y/z"));
	}
};

REGISTER_TEST(TestUtils)
