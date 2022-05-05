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
#include <google/protobuf/parse_context.h>
#include <google/protobuf/generated_message_tctable_impl.h>
// clang-format off
#include <google/protobuf/port_def.inc>
// clang-format on

namespace google {
namespace protobuf {
namespace internal {
const char* TcParser::GenericFallback(PROTOBUF_TC_PARAM_DECL) {
	return GenericFallbackImpl<Message, UnknownFieldSet>(PROTOBUF_TC_PARAM_PASS);
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google
