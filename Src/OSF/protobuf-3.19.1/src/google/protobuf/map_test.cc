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
#include <google/protobuf/map_proto2_unittest.pb.h>
#include <google/protobuf/map_unittest.pb.h>
#include <google/protobuf/reflection_tester.h>
#include <google/protobuf/test_util2.h>

#define BRIDGE_UNITTEST ::google::protobuf::bridge_unittest
#define UNITTEST ::protobuf_unittest
#define UNITTEST_IMPORT ::protobuf_unittest_import
#define UNITTEST_PACKAGE_NAME "protobuf_unittest"

// Must include after defining UNITTEST, etc.
// clang-format off
#include <google/protobuf/test_util.inc>
#include <google/protobuf/map_test_util.inc>
#include <google/protobuf/map_test.inc>
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
