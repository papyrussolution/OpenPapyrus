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
// Generates Kotlin code for a given .proto file.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H__

//#include <string>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// CodeGenerator implementation which generates Kotlin code.  If you create your
// own protocol compiler binary and you want it to support Kotlin output, you
// can do so by registering an instance of this CodeGenerator with the
// CommandLineInterface in your main() function.
class PROTOC_EXPORT KotlinGenerator : public CodeGenerator {
 public:
  KotlinGenerator();
  ~KotlinGenerator() override;

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* context, std::string* error) const override;

  uint64_t GetSupportedFeatures() const override;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(KotlinGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_KOTLIN_GENERATOR_H__
