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
//
// Provides a mechanism for mapping a descriptor to the
// fully-qualified name of the corresponding Java class.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__

//#include <string>

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class FileDescriptor;
class FieldDescriptor;
class ServiceDescriptor;

namespace compiler {
namespace java {

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const Descriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const EnumDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const FileDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   The fully-qualified Java class name.
std::string ClassName(const ServiceDescriptor* descriptor);

// Requires:
//   descriptor != NULL
//
// Returns:
//   Java package name.
std::string FileJavaPackage(const FileDescriptor* descriptor);

// Requires:
//   descriptor != NULL
// Returns:
//   Capitalized camel case name field name.
std::string CapitalizedFieldName(const FieldDescriptor* descriptor);

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_H__
