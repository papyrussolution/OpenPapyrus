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
#include <google/protobuf/compiler/csharp/csharp_doc_comment.h>
#include <google/protobuf/compiler/csharp/csharp_enum.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_options.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {
EnumGenerator::EnumGenerator(const EnumDescriptor* descriptor, const Options* options) :
	SourceGeneratorBase(options),
	descriptor_(descriptor) {
}

EnumGenerator::~EnumGenerator() {
}

void EnumGenerator::Generate(io::Printer* printer) {
	WriteEnumDocComment(printer, descriptor_);
	printer->Print("$access_level$ enum $name$ {\n",
	    "access_level", class_access_level(),
	    "name", descriptor_->name());
	printer->Indent();
	std::set<std::string> used_names;
	std::set<int> used_number;
	for(int i = 0; i < descriptor_->value_count(); i++) {
		WriteEnumValueDocComment(printer, descriptor_->value(i));
		std::string original_name = descriptor_->value(i)->name();
		std::string name =
		    GetEnumValueName(descriptor_->name(), descriptor_->value(i)->name());
		// Make sure we don't get any duplicate names due to prefix removal.
		while(!used_names.insert(name).second) {
			// It's possible we'll end up giving this warning multiple times, but that's better than not at
			// all.
			GOOGLE_LOG(WARNING) << "Duplicate enum value " << name << " (originally " << original_name
					    << ") in " << descriptor_->name() << "; adding underscore to distinguish";
			name += "_";
		}
		int number = descriptor_->value(i)->number();
		if(!used_number.insert(number).second) {
			printer->Print("[pbr::OriginalName(\"$original_name$\", PreferredAlias = false)] $name$ = $number$,\n",
			    "original_name", original_name,
			    "name", name,
			    "number", StrCat(number));
		}
		else {
			printer->Print("[pbr::OriginalName(\"$original_name$\")] $name$ = $number$,\n",
			    "original_name", original_name,
			    "name", name,
			    "number", StrCat(number));
		}
	}
	printer->Outdent();
	printer->Print("}\n");
	printer->Print("\n");
}
}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
