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
#include <google/protobuf/compiler/java/java_extension_lite.h>
#include <google/protobuf/compiler/java/java_context.h>
#include <google/protobuf/compiler/java/java_doc_comment.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/compiler/java/java_name_resolver.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
ImmutableExtensionLiteGenerator::ImmutableExtensionLiteGenerator(const FieldDescriptor* descriptor, Context* context)
	: descriptor_(descriptor), name_resolver_(context->GetNameResolver()) {
	if(descriptor_->extension_scope() != NULL) {
		scope_ =
		    name_resolver_->GetImmutableClassName(descriptor_->extension_scope());
	}
	else {
		scope_ = name_resolver_->GetImmutableClassName(descriptor_->file());
	}
}

ImmutableExtensionLiteGenerator::~ImmutableExtensionLiteGenerator() {
}

void ImmutableExtensionLiteGenerator::Generate(io::Printer* printer) {
	std::map<std::string, std::string> vars;
	const bool kUseImmutableNames = true;
	InitTemplateVars(descriptor_, scope_, kUseImmutableNames, name_resolver_,
	    &vars);
	printer->Print(vars, "public static final int $constant_name$ = $number$;\n");

	WriteFieldDocComment(printer, descriptor_);
	if(descriptor_->is_repeated()) {
		printer->Print(
			vars,
			"public static final\n"
			"  com.google.protobuf.GeneratedMessageLite.GeneratedExtension<\n"
			"    $containing_type$,\n"
			"    $type$> $name$ = com.google.protobuf.GeneratedMessageLite\n"
			"        .newRepeatedGeneratedExtension(\n"
			"      $containing_type$.getDefaultInstance(),\n"
			"      $prototype$,\n"
			"      $enum_map$,\n"
			"      $number$,\n"
			"      com.google.protobuf.WireFormat.FieldType.$type_constant$,\n"
			"      $packed$,\n"
			"      $singular_type$.class);\n");
	}
	else {
		printer->Print(
			vars,
			"public static final\n"
			"  com.google.protobuf.GeneratedMessageLite.GeneratedExtension<\n"
			"    $containing_type$,\n"
			"    $type$> $name$ = com.google.protobuf.GeneratedMessageLite\n"
			"        .newSingularGeneratedExtension(\n"
			"      $containing_type$.getDefaultInstance(),\n"
			"      $default$,\n"
			"      $prototype$,\n"
			"      $enum_map$,\n"
			"      $number$,\n"
			"      com.google.protobuf.WireFormat.FieldType.$type_constant$,\n"
			"      $singular_type$.class);\n");
	}
	printer->Annotate("name", descriptor_);
}

int ImmutableExtensionLiteGenerator::GenerateNonNestedInitializationCode(io::Printer* printer) {
	return 0;
}

int ImmutableExtensionLiteGenerator::GenerateRegistrationCode(io::Printer* printer) {
	printer->Print("registry.add($scope$.$name$);\n", "scope", scope_, "name",
	    UnderscoresToCamelCaseCheckReserved(descriptor_));
	return 7;
}
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
