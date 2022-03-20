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
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
// clang-format off
#include <google/protobuf/port_def.inc>
// clang-format on

namespace google {
namespace protobuf {
namespace internal {
std::string* InlinedStringField::Mutable(const LazyString& /*default_value*/,
    Arena* arena, bool donated,
    uint32_t* donating_states,
    uint32_t mask) {
	if(arena == nullptr || !donated) {
		return UnsafeMutablePointer();
	}
	return MutableSlow(arena, donated, donating_states, mask);
}

std::string* InlinedStringField::Mutable(ArenaStringPtr::EmptyDefault,
    Arena* arena, bool donated,
    uint32_t* donating_states,
    uint32_t mask) {
	if(arena == nullptr || !donated) {
		return UnsafeMutablePointer();
	}
	return MutableSlow(arena, donated, donating_states, mask);
}

std::string* InlinedStringField::MutableSlow(::google::protobuf::Arena* arena,
    bool donated,
    uint32_t* donating_states,
    uint32_t mask) {
	return UnsafeMutablePointer();
}

void InlinedStringField::SetAllocated(const std::string* default_value,
    std::string* value, Arena* arena,
    bool donated, uint32_t* donating_states,
    uint32_t mask) {
	SetAllocatedNoArena(default_value, value);
}

void InlinedStringField::Set(const std::string* default_value,
    std::string && value, Arena* arena, bool donated,
    uint32_t* donating_states, uint32_t mask) {
	SetNoArena(default_value, std::move(value));
}

std::string* InlinedStringField::Release(const std::string* default_value,
    Arena* arena, bool donated) {
	if(arena == nullptr && !donated) {
		return ReleaseNonDefaultNoArena(default_value);
	}
	return ReleaseNonDefault(default_value, arena);
}

std::string* InlinedStringField::ReleaseNonDefault(const std::string* default_value, Arena* arena) {
	return ReleaseNonDefaultNoArena(default_value);
}

void InlinedStringField::ClearToDefault(const LazyString& default_value,
    Arena* arena, bool donated) {
	(void)arena;
	get_mutable()->assign(default_value.get());
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google
