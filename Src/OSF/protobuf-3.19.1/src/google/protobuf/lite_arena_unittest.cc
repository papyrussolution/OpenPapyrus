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
#include <google/protobuf/arena_test_util.h>
#include <google/protobuf/map_lite_test_util.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace {

class LiteArenaTest : public testing::Test {
 protected:
  LiteArenaTest() {
    ArenaOptions options;
    options.start_block_size = 128 * 1024;
    options.max_block_size = 128 * 1024;
    arena_.reset(new Arena(options));
    // Trigger the allocation of the first arena block, so that further use of
    // the arena will not require any heap allocations.
    Arena::CreateArray<char>(arena_.get(), 1);
  }

  std::unique_ptr<Arena> arena_;
};

TEST_F(LiteArenaTest, MapNoHeapAllocation) {
  std::string data;
  data.reserve(128 * 1024);

  {
    // TODO(teboring): Enable no heap check when ArenaStringPtr is used in
    // Map.
    // internal::NoHeapChecker no_heap;

    protobuf_unittest::TestArenaMapLite* from =
        Arena::CreateMessage<protobuf_unittest::TestArenaMapLite>(arena_.get());
    MapLiteTestUtil::SetArenaMapFields(from);
    from->SerializeToString(&data);

    protobuf_unittest::TestArenaMapLite* to =
        Arena::CreateMessage<protobuf_unittest::TestArenaMapLite>(arena_.get());
    to->ParseFromString(data);
    MapLiteTestUtil::ExpectArenaMapFieldsSet(*to);
  }
}

TEST_F(LiteArenaTest, UnknownFieldMemLeak) {
  protobuf_unittest::ForeignMessageArenaLite* message =
      Arena::CreateMessage<protobuf_unittest::ForeignMessageArenaLite>(
          arena_.get());
  std::string data = "\012\000";
  int original_capacity = data.capacity();
  while (data.capacity() <= original_capacity) {
    data.append("a");
  }
  data[1] = data.size() - 2;
  message->ParseFromString(data);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
