// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// Copyright 2008 Google Inc. All Rights Reserved.
// Author: xpeng@google.com (Peter Peng)

#include <protobuf-internal.h>
#pragma hdrstop
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(StructurallyValidTest, ValidUTF8String) {
  // On GCC, this string can be written as:
  //   "abcd 1234 - \u2014\u2013\u2212"
  // MSVC seems to interpret \u differently.
  std::string valid_str(
      "abcd 1234 - \342\200\224\342\200\223\342\210\222 - xyz789");
  EXPECT_TRUE(IsStructurallyValidUTF8(valid_str.data(),
                                      valid_str.size()));
  // Additional check for pointer alignment
  for (int i = 1; i < 8; ++i) {
    EXPECT_TRUE(IsStructurallyValidUTF8(valid_str.data() + i,
                                        valid_str.size() - i));
  }
}

TEST(StructurallyValidTest, InvalidUTF8String) {
  const std::string invalid_str("abcd\xA0\xB0\xA0\xB0\xA0\xB0 - xyz789");
  EXPECT_FALSE(IsStructurallyValidUTF8(invalid_str.data(),
                                       invalid_str.size()));
  // Additional check for pointer alignment
  for (int i = 1; i < 8; ++i) {
    EXPECT_FALSE(IsStructurallyValidUTF8(invalid_str.data() + i,
                                         invalid_str.size() - i));
  }
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
