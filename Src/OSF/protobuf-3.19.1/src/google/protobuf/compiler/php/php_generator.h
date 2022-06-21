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
#ifndef GOOGLE_PROTOBUF_COMPILER_PHP_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_PHP_GENERATOR_H__

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
//#include <string>
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace compiler {
namespace php {
struct Options;

class PROTOC_EXPORT Generator : public CodeGenerator {
public:
	virtual bool Generate(const FileDescriptor* file, const std::string& parameter, GeneratorContext* generator_context, std::string* error) const override;
	bool GenerateAll(const std::vector<const FileDescriptor*>& files, const std::string& parameter,
	    GeneratorContext* generator_context, std::string* error) const override;
	uint64_t GetSupportedFeatures() const override { return FEATURE_PROTO3_OPTIONAL; }
private:
	bool Generate(const FileDescriptor* file, const Options& options, GeneratorContext* generator_context, std::string* error) const;
};

// To skip reserved keywords in php, some generated classname are prefixed.
// Other code generators may need following API to figure out the actual
// classname.
PROTOC_EXPORT std::string GeneratedClassName(const Descriptor* desc);
PROTOC_EXPORT std::string GeneratedClassName(const EnumDescriptor* desc);
PROTOC_EXPORT std::string GeneratedClassName(const ServiceDescriptor* desc);

inline bool IsWrapperType(const FieldDescriptor* descriptor) {
	return descriptor->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
	       descriptor->message_type()->file()->name() == "google/protobuf/wrappers.proto";
}
}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_COMPILER_PHP_GENERATOR_H__
