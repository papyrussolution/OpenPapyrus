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
#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__

//#include <string>
//#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/csharp/csharp_source_generator_base.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

namespace google {
	namespace protobuf {
		namespace compiler {
			namespace csharp {
				class FieldGeneratorBase : public SourceGeneratorBase {
				public:
					FieldGeneratorBase(const FieldDescriptor* descriptor, int presenceIndex, const Options* options);
					~FieldGeneratorBase();
					FieldGeneratorBase(const FieldGeneratorBase&) = delete;
					FieldGeneratorBase& operator=(const FieldGeneratorBase&) = delete;
					virtual void GenerateCloningCode(io::Printer* printer) = 0;
					virtual void GenerateFreezingCode(io::Printer* printer);
					virtual void GenerateCodecCode(io::Printer* printer);
					virtual void GenerateExtensionCode(io::Printer* printer);
					virtual void GenerateMembers(io::Printer* printer) = 0;
					virtual void GenerateMergingCode(io::Printer* printer) = 0;
					virtual void GenerateParsingCode(io::Printer* printer) = 0;
					virtual void GenerateParsingCode(io::Printer* printer, bool use_parse_context);
					virtual void GenerateSerializationCode(io::Printer* printer) = 0;
					virtual void GenerateSerializationCode(io::Printer* printer, bool use_write_context);
					virtual void GenerateSerializedSizeCode(io::Printer* printer) = 0;
					virtual void WriteHash(io::Printer* printer) = 0;
					virtual void WriteEquals(io::Printer* printer) = 0;
					// Currently unused, as we use reflection to generate JSON
					virtual void WriteToString(io::Printer* printer) = 0;
				protected:
					const FieldDescriptor* descriptor_;
					const int presenceIndex_;
					std::map<std::string, std::string> variables_;
					void AddDeprecatedFlag(io::Printer* printer);
					void AddNullCheck(io::Printer* printer);
					void AddNullCheck(io::Printer* printer, const std::string& name);
					void AddPublicMemberAttributes(io::Printer* printer);
					void SetCommonOneofFieldVariables(std::map<std::string, std::string>* variables);
					std::string oneof_property_name();
					std::string oneof_name();
					std::string property_name();
					std::string name();
					std::string type_name();
					std::string type_name(const FieldDescriptor* descriptor);
					bool has_default_value();
					std::string default_value();
					std::string default_value(const FieldDescriptor* descriptor);
					std::string number();
					std::string capitalized_type_name();
				private:
					void SetCommonFieldVariables(std::map<std::string, std::string>* variables);
					std::string GetStringDefaultValueInternal(const FieldDescriptor* descriptor);
					std::string GetBytesDefaultValueInternal(const FieldDescriptor* descriptor);
				};
			}
		}
	}
}
#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__
