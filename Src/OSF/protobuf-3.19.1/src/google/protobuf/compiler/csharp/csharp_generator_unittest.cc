// Protocol Buffers - Google's data interchange format
// Copyright 2014 Google Inc.  All rights reserved.
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
#include <protobuf-internal.h>
#pragma hdrstop
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>
#include <google/protobuf/testing/file.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {
namespace {

TEST(CSharpEnumValue, PascalCasedPrefixStripping) {
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "FOO__BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "FOO_BAR_BAZ"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "Foo_BarBaz"));
  EXPECT_EQ("Bar", GetEnumValueName("FO_O", "FOO_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("FOO", "F_O_O_BAR"));
  EXPECT_EQ("Bar", GetEnumValueName("Foo", "BAR"));
  EXPECT_EQ("BarBaz", GetEnumValueName("Foo", "BAR_BAZ"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO"));
  EXPECT_EQ("Foo", GetEnumValueName("Foo", "FOO___"));
  // Identifiers can't start with digits
  EXPECT_EQ("_2Bar", GetEnumValueName("Foo", "FOO_2_BAR"));
  EXPECT_EQ("_2", GetEnumValueName("Foo", "FOO___2"));
}

}  // namespace
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
