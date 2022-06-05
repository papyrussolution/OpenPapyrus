// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// The Google C++ Testing and Mocking Framework (Google Test)
//
// This file implements just enough of the matcher interface to allow
// EXPECT_DEATH and friends to accept a matcher argument.

#include "gtest/internal/gtest-build-internal.h"
#pragma hdrstop

namespace testing {
// Constructs a matcher that matches a const std::string& whose value is
// equal to s.
Matcher<const std::string&>::Matcher(const std::string& s) { *this = Eq(s); }
// Constructs a matcher that matches a const std::string& whose value is equal to s.
Matcher<const std::string&>::Matcher(const char* s) { *this = Eq(std::string(s)); }
// Constructs a matcher that matches a std::string whose value is equal to s.
Matcher<std::string>::Matcher(const std::string& s) { *this = Eq(s); }
// Constructs a matcher that matches a std::string whose value is equal to s.
Matcher<std::string>::Matcher(const char* s) { *this = Eq(std::string(s)); }
#if GTEST_INTERNAL_HAS_STRING_VIEW
	// Constructs a matcher that matches a const StringView& whose value is
	// equal to s.
	Matcher<const internal::StringView&>::Matcher(const std::string& s) { *this = Eq(s); }

	// Constructs a matcher that matches a const StringView& whose value is
	// equal to s.
	Matcher<const internal::StringView&>::Matcher(const char* s) { *this = Eq(std::string(s)); }

	// Constructs a matcher that matches a const StringView& whose value is
	// equal to s.
	Matcher<const internal::StringView&>::Matcher(internal::StringView s) { *this = Eq(std::string(s)); }

	// Constructs a matcher that matches a StringView whose value is equal to
	// s.
	Matcher<internal::StringView>::Matcher(const std::string& s) { *this = Eq(s); }

	// Constructs a matcher that matches a StringView whose value is equal to
	// s.
	Matcher<internal::StringView>::Matcher(const char* s) { *this = Eq(std::string(s)); }

	// Constructs a matcher that matches a StringView whose value is equal to
	// s.
	Matcher<internal::StringView>::Matcher(internal::StringView s) { *this = Eq(std::string(s)); }
#endif  // GTEST_INTERNAL_HAS_STRING_VIEW
}  // namespace testing
