// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
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
#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_ENUM_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_ENUM_H__

//#include <string>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/csharp/csharp_source_generator_base.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

namespace google {
	namespace protobuf {
		namespace compiler {
			namespace csharp {
				class EnumGenerator : public SourceGeneratorBase {
				public:
					EnumGenerator(const EnumDescriptor* descriptor, const Options* options);
					~EnumGenerator();
					EnumGenerator(const EnumGenerator&) = delete;
					EnumGenerator& operator=(const EnumGenerator&) = delete;
					void Generate(io::Printer* printer);
				private:
					const EnumDescriptor* descriptor_;
				};
			}
		}
	}
}

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_ENUM_H__
