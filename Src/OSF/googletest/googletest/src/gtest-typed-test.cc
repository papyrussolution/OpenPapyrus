// Copyright 2008 Google Inc.
// All Rights Reserved.
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
#include "gtest/internal/gtest-build-internal.h"
#pragma hdrstop

namespace testing {
namespace internal {
// Skips to the first non-space char in str. Returns an empty string if str
// contains only whitespace characters.
static const char* SkipSpaces(const char* str) 
{
	while(IsSpace(*str)) 
		str++;
	return str;
}

static std::vector<std::string> SplitIntoTestNames(const char* src) 
{
	std::vector<std::string> name_vec;
	src = SkipSpaces(src);
	for(; src != nullptr; src = SkipComma(src)) {
		name_vec.push_back(StripTrailingSpaces(GetPrefixUntilComma(src)));
	}
	return name_vec;
}

// Verifies that registered_tests match the test names in
// registered_tests_; returns registered_tests if successful, or
// aborts the program otherwise.
const char* TypedTestSuitePState::VerifyRegisteredTestNames(const char* test_suite_name, const char* file, int line, const char* registered_tests) 
{
	RegisterTypeParameterizedTestSuite(test_suite_name, CodeLocation(file, line));
	typedef RegisteredTestsMap::const_iterator RegisteredTestIter;
	registered_ = true;
	std::vector<std::string> name_vec = SplitIntoTestNames(registered_tests);
	Message errors;
	std::set<std::string> tests;
	for(std::vector<std::string>::const_iterator name_it = name_vec.begin();
	    name_it != name_vec.end(); ++name_it) {
		const std::string& name = *name_it;
		if(tests.count(name) != 0) {
			errors << "Test " << name << " is listed more than once.\n";
			continue;
		}
		if(registered_tests_.count(name) != 0) {
			tests.insert(name);
		}
		else {
			errors << "No test named " << name << " can be found in this test suite.\n";
		}
	}
	for(RegisteredTestIter it = registered_tests_.begin();
	    it != registered_tests_.end(); ++it) {
		if(tests.count(it->first) == 0) {
			errors << "You forgot to list test " << it->first << ".\n";
		}
	}
	const std::string& errors_str = errors.GetString();
	if(errors_str != "") {
		fprintf(stderr, "%s %s", FormatFileLocation(file, line).c_str(), errors_str.c_str());
		fflush(stderr);
		posix::Abort();
	}
	return registered_tests;
}
}  // namespace internal
}  // namespace testing
