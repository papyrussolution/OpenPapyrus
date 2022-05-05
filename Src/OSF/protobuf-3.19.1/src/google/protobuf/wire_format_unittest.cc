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
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <protobuf-internal.h>
#pragma hdrstop
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/unittest_mset.pb.h>
#include <google/protobuf/unittest_mset_wire_format.pb.h>
#include <google/protobuf/unittest_proto3_arena.pb.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"
#define PROTO2_WIREFORMAT_UNITTEST ::proto2_wireformat_unittest
#define PROTO3_ARENA_UNITTEST ::proto3_arena_unittest

// Must include after defining UNITTEST, etc.
// clang-format off
#include <google/protobuf/test_util.inc>
#include <google/protobuf/wire_format_unittest.inc>
// clang-format on

// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {
namespace {


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
