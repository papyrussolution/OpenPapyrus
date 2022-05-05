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
#ifndef GOOGLE_PROTOBUF_STUBS_ONCE_H__
#define GOOGLE_PROTOBUF_STUBS_ONCE_H__

#include <mutex>
#include <utility>
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace internal {
using once_flag = std::once_flag;
template <typename ... Args>
void call_once(Args&& ... args) {
	std::call_once(std::forward<Args>(args) ...);
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_STUBS_ONCE_H__
