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
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {
bool ZeroCopyOutputStream::WriteAliasedRaw(const void* /* data */,
    int /* size */) {
	GOOGLE_LOG(FATAL) << "This ZeroCopyOutputStream doesn't support aliasing. "
		"Reaching here usually means a ZeroCopyOutputStream "
		"implementation bug.";
	return false;
}
}  // namespace io
}  // namespace protobuf
}  // namespace google
