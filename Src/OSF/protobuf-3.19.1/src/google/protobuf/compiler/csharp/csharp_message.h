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
#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_MESSAGE_H__

//#include <string>
//#include <vector>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/csharp/csharp_source_generator_base.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>

namespace google {
	namespace protobuf {
		namespace compiler {
			namespace csharp {
				class FieldGeneratorBase;

				class MessageGenerator : public SourceGeneratorBase {
				public:
					MessageGenerator(const Descriptor* descriptor, const Options* options);
					~MessageGenerator();
					MessageGenerator(const MessageGenerator&) = delete;
					MessageGenerator& operator=(const MessageGenerator&) = delete;
					void GenerateCloningCode(io::Printer* printer);
					void GenerateFreezingCode(io::Printer* printer);
					void GenerateFrameworkMethods(io::Printer* printer);
					void Generate(io::Printer* printer);
				private:
					const Descriptor* descriptor_;
					std::vector<const FieldDescriptor*> fields_by_number_;
					int has_bit_field_count_;
					uint end_tag_;
					bool has_extension_ranges_;

					void GenerateMessageSerializationMethods(io::Printer* printer);
					void GenerateWriteToBody(io::Printer* printer, bool use_write_context);
					void GenerateMergingMethods(io::Printer* printer);
					void GenerateMainParseLoop(io::Printer* printer, bool use_parse_context);
					int GetPresenceIndex(const FieldDescriptor* descriptor);
					FieldGeneratorBase* CreateFieldGeneratorInternal(const FieldDescriptor* descriptor);
					bool HasNestedGeneratedTypes();
					void AddDeprecatedFlag(io::Printer* printer);
					void AddSerializableAttribute(io::Printer* printer);
					std::string class_name();
					std::string full_class_name();
					// field descriptors sorted by number
					const std::vector<const FieldDescriptor*>& fields_by_number();
				};
			}
		}
	}
}

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_MESSAGE_H__
