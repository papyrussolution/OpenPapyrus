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
#include <google/protobuf/implicit_weak_message.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {
const char* ImplicitWeakMessage::_InternalParse(const char* ptr, ParseContext* ctx) 
{
	return ctx->AppendString(ptr, &data_);
}

ExplicitlyConstructed<ImplicitWeakMessage> implicit_weak_message_default_instance;
internal::once_flag implicit_weak_message_once_init_;

void InitImplicitWeakMessageDefaultInstance() 
{
	implicit_weak_message_default_instance.DefaultConstruct();
}

const ImplicitWeakMessage* ImplicitWeakMessage::default_instance() 
{
	internal::call_once(implicit_weak_message_once_init_, InitImplicitWeakMessageDefaultInstance);
	return &implicit_weak_message_default_instance.get();
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
