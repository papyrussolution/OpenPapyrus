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
#include <protobuf-internal.h>
#pragma hdrstop
#include <google/protobuf/unittest_well_known_types.pb.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {
namespace {
// This test only checks whether well-known types are included in protobuf
// runtime library. The test passes if it compiles.
TEST(WellKnownTypesTest, AllKnownTypesAreIncluded) {
	protobuf_unittest::TestWellKnownTypes message;
	EXPECT_EQ(0, message.any_field().ByteSize());
	EXPECT_EQ(0, message.api_field().ByteSize());
	EXPECT_EQ(0, message.duration_field().ByteSize());
	EXPECT_EQ(0, message.empty_field().ByteSize());
	EXPECT_EQ(0, message.field_mask_field().ByteSize());
	EXPECT_EQ(0, message.source_context_field().ByteSize());
	EXPECT_EQ(0, message.struct_field().ByteSize());
	EXPECT_EQ(0, message.timestamp_field().ByteSize());
	EXPECT_EQ(0, message.type_field().ByteSize());
	EXPECT_EQ(0, message.int32_field().ByteSize());
}
}  // namespace
}  // namespace protobuf
}  // namespace google
