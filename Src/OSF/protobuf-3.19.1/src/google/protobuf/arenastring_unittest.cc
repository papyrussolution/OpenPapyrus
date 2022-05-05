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
#include <google/protobuf/arenastring.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/generated_message_util.h>
#include <gtest/gtest.h>
// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
using internal::ArenaStringPtr;

using EmptyDefault = ArenaStringPtr::EmptyDefault;

const internal::LazyString nonempty_default{{{"default", 7}}, {nullptr}};
const std::string* empty_default = &internal::GetEmptyString();

class SingleArena : public testing::TestWithParam<bool> {
public:
	std::unique_ptr<Arena> GetArena() {
		if(this->GetParam()) return nullptr;
		return std::unique_ptr<Arena>(new Arena());
	}
};

INSTANTIATE_TEST_SUITE_P(ArenaString, SingleArena, testing::Bool());

TEST_P(SingleArena, GetSet) {
	auto arena = GetArena();
	ArenaStringPtr field;
	field.UnsafeSetDefault(empty_default);
	EXPECT_EQ("", field.Get());
	field.Set(empty_default, "Test short", arena.get());
	EXPECT_EQ("Test short", field.Get());
	field.Set(empty_default, "Test long long long long value", arena.get());
	EXPECT_EQ("Test long long long long value", field.Get());
	field.Set(empty_default, "", arena.get());
	field.Destroy(empty_default, arena.get());
}

TEST_P(SingleArena, MutableAccessor) {
	auto arena = GetArena();
	ArenaStringPtr field;
	const std::string* empty_default = &internal::GetEmptyString();
	field.UnsafeSetDefault(empty_default);

	std::string* mut = field.Mutable(EmptyDefault{}, arena.get());
	EXPECT_EQ(mut, field.Mutable(EmptyDefault{}, arena.get()));
	EXPECT_EQ(mut, &field.Get());
	EXPECT_NE(empty_default, mut);
	EXPECT_EQ("", *mut);
	*mut = "Test long long long long value"; // ensure string allocates storage
	EXPECT_EQ("Test long long long long value", field.Get());
	field.Destroy(empty_default, arena.get());
}

TEST_P(SingleArena, NullDefault) {
	auto arena = GetArena();

	ArenaStringPtr field;
	field.UnsafeSetDefault(nullptr);
	std::string* mut = field.Mutable(nonempty_default, arena.get());
	EXPECT_EQ(mut, field.Mutable(nonempty_default, arena.get()));
	EXPECT_EQ(mut, &field.Get());
	EXPECT_NE(nullptr, mut);
	EXPECT_EQ("default", *mut);
	*mut = "Test long long long long value"; // ensure string allocates storage
	EXPECT_EQ("Test long long long long value", field.Get());
	field.Destroy(nullptr, arena.get());
}

class DualArena : public testing::TestWithParam<std::tuple<bool, bool> > {
public:
	std::unique_ptr<Arena> GetLhsArena() {
		if(std::get<0>(this->GetParam())) return nullptr;
		return std::unique_ptr<Arena>(new Arena());
	}

	std::unique_ptr<Arena> GetRhsArena() {
		if(std::get<1>(this->GetParam())) return nullptr;
		return std::unique_ptr<Arena>(new Arena());
	}
};

INSTANTIATE_TEST_SUITE_P(ArenaString, DualArena,
    testing::Combine(testing::Bool(), testing::Bool()));

TEST_P(DualArena, Swap) {
	auto lhs_arena = GetLhsArena();
	ArenaStringPtr lhs;
	lhs.UnsafeSetDefault(empty_default);
	ArenaStringPtr rhs;
	rhs.UnsafeSetDefault(empty_default);

	{
		auto rhs_arena = GetRhsArena();
		lhs.Set(empty_default, "lhs value that has some heft", lhs_arena.get());
		rhs.Set(empty_default, "rhs value that has some heft", rhs_arena.get());
		ArenaStringPtr::InternalSwap(empty_default, //
		    &lhs, lhs_arena.get(),               //
		    &rhs, rhs_arena.get());
		EXPECT_EQ("rhs value that has some heft", lhs.Get());
		EXPECT_EQ("lhs value that has some heft", rhs.Get());
		lhs.Destroy(empty_default, rhs_arena.get());
	}
	EXPECT_EQ("lhs value that has some heft", rhs.Get());
	rhs.Destroy(empty_default, lhs_arena.get());
}
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
