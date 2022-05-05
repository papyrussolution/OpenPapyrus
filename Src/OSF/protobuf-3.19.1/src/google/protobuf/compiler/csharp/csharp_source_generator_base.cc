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
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/compiler/csharp/csharp_source_generator_base.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_names.h>
#include <google/protobuf/compiler/csharp/csharp_options.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

SourceGeneratorBase::SourceGeneratorBase(
    const Options *options) : options_(options) {
}

SourceGeneratorBase::~SourceGeneratorBase() {
}

void SourceGeneratorBase::WriteGeneratedCodeAttributes(io::Printer* printer) {
  printer->Print("[global::System.Diagnostics.DebuggerNonUserCodeAttribute]\n");
  // The second argument of the [GeneratedCode] attribute could be set to current protoc
  // version, but that would cause excessive code churn in the pre-generated
  // code in the repository every time the protobuf version number is updated.
  printer->Print("[global::System.CodeDom.Compiler.GeneratedCode(\"protoc\", null)]\n");
}

std::string SourceGeneratorBase::class_access_level() {
  return this->options()->internal_access ? "internal" : "public";
}

const Options* SourceGeneratorBase::options() {
  return this->options_;
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
