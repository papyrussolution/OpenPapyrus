// Copyright 2005, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
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
// This file defines the AssertionResult type.

#include "gtest/internal/gtest-build-internal.h"
#pragma hdrstop

namespace testing {
// AssertionResult constructors.
// Used in EXPECT_TRUE/FALSE(assertion_result).
AssertionResult::AssertionResult(const AssertionResult& other) : success_(other.success_), 
	message_(other.message_.get() != nullptr ? new ::std::string(*other.message_) : static_cast< ::std::string*>(nullptr)) 
{
}

// Swaps two AssertionResults.
void AssertionResult::swap(AssertionResult& other) 
{
	using std::swap;
	swap(success_, other.success_);
	swap(message_, other.message_);
}

// Returns the assertion's negation. Used with EXPECT/ASSERT_FALSE.
AssertionResult AssertionResult::operator!() const 
{
	AssertionResult negation(!success_);
	if(message_.get() != nullptr) 
		negation << *message_;
	return negation;
}

// Makes a successful assertion result.
AssertionResult AssertionSuccess() { return AssertionResult(true); }
// Makes a failed assertion result.
AssertionResult AssertionFailure() { return AssertionResult(false); }
// Makes a failed assertion result with the given failure message.
// Deprecated; use AssertionFailure() << message.
AssertionResult AssertionFailure(const Message& message) { return AssertionFailure() << message; }
}  // namespace testing
