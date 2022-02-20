// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// Defines utilities for the TypeResolver.

#ifndef GOOGLE_PROTOBUF_UTIL_TYPE_RESOLVER_UTIL_H__
#define GOOGLE_PROTOBUF_UTIL_TYPE_RESOLVER_UTIL_H__

#include <string>

namespace google {
namespace protobuf {
class DescriptorPool;
namespace util {
class TypeResolver;

#include <google/protobuf/port_def.inc>

// Creates a TypeResolver that serves type information in the given descriptor
// pool. Caller takes ownership of the returned TypeResolver.
PROTOBUF_EXPORT TypeResolver* NewTypeResolverForDescriptorPool(
    const std::string & url_prefix, const DescriptorPool* pool);

}  // namespace util
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_UTIL_TYPE_RESOLVER_UTIL_H__
