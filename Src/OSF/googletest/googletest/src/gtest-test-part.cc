// Copyright 2008, Google Inc. All rights reserved.
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
#include "gtest/internal/gtest-build-internal.h"
#pragma hdrstop

namespace testing {
using internal::GetUnitTestImpl;

// Gets the summary of the failure message by omitting the stack trace
// in it.
std::string TestPartResult::ExtractSummary(const char* message) {
	const char* const stack_trace = strstr(message, internal::kStackTraceMarker);
	return stack_trace == nullptr ? message : std::string(message, stack_trace);
}

// Prints a TestPartResult object.
std::ostream& operator<<(std::ostream& os, const TestPartResult& result) {
	return os << internal::FormatFileLocation(result.file_name(),
		   result.line_number())
		  << " "
		  << (result.type() == TestPartResult::kSuccess ? "Success"
	       : result.type() == TestPartResult::kSkip  ? "Skipped"
	       : result.type() == TestPartResult::kFatalFailure
	       ? "Fatal failure"
	       : "Non-fatal failure")
		  << ":\n"
		  << result.message() << std::endl;
}

// Appends a TestPartResult to the array.
void TestPartResultArray::Append(const TestPartResult& result) {
	array_.push_back(result);
}

// Returns the TestPartResult at the given index (0-based).
const TestPartResult& TestPartResultArray::GetTestPartResult(int index) const {
	if(index < 0 || index >= size()) {
		printf("\nInvalid index (%d) into TestPartResultArray.\n", index);
		internal::posix::Abort();
	}

	return array_[static_cast<size_t>(index)];
}

// Returns the number of TestPartResult objects in the array.
int TestPartResultArray::size() const {
	return static_cast<int>(array_.size());
}

namespace internal {
HasNewFatalFailureHelper::HasNewFatalFailureHelper()
	: has_new_fatal_failure_(false),
	original_reporter_(
		GetUnitTestImpl()->GetTestPartResultReporterForCurrentThread()) {
	GetUnitTestImpl()->SetTestPartResultReporterForCurrentThread(this);
}

HasNewFatalFailureHelper::~HasNewFatalFailureHelper() {
	GetUnitTestImpl()->SetTestPartResultReporterForCurrentThread(
		original_reporter_);
}

void HasNewFatalFailureHelper::ReportTestPartResult(const TestPartResult& result) {
	if(result.fatally_failed()) has_new_fatal_failure_ = true;
	original_reporter_->ReportTestPartResult(result);
}
}  // namespace internal
}  // namespace testing
