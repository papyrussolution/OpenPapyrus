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
// This file contains helpers for generated code.
//
//  Nothing in this file should be directly referenced by users of protobufs.

#ifndef GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__
#define GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__

#include <google/protobuf/parse_context.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/port_def.inc> // Must come last:

namespace google {
namespace protobuf {
namespace internal {
// To save code size, protos without any fields are derived from ZeroFieldsBase
// rather than Message.
class PROTOBUF_EXPORT ZeroFieldsBase : public Message {
public:
	PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
	bool IsInitialized() const final {
		return true;
	}

	size_t ByteSizeLong() const final;
	int GetCachedSize() const final {
		return _cached_size_.Get();
	}

	const char* _InternalParse(const char* ptr,
	    internal::ParseContext* ctx) final;
	::uint8_t* _InternalSerialize(::uint8_t* target,
	    io::EpsCopyOutputStream* stream) const final;

protected:
	constexpr ZeroFieldsBase() {
	}

	explicit ZeroFieldsBase(Arena* arena, bool is_message_owned)
		: Message(arena, is_message_owned) {
	}

	ZeroFieldsBase(const ZeroFieldsBase&) = delete;
	ZeroFieldsBase& operator=(const ZeroFieldsBase&) = delete;
	~ZeroFieldsBase() override;

	void SetCachedSize(int size) const final {
		_cached_size_.Set(size);
	}

	static void MergeImpl(Message* to, const Message& from);
	static void CopyImpl(Message* to, const Message& from);
	void InternalSwap(ZeroFieldsBase* other);

	mutable internal::CachedSize _cached_size_;
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_GENERATED_MESSAGE_BASES_H__
