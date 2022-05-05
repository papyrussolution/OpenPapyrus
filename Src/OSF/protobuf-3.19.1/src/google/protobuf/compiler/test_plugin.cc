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
//
// This is a dummy code generator plugin used by
// command_line_interface_unittest.

#include <protobuf-internal.h>
#pragma hdrstop
#include <google/protobuf/compiler/mock_code_generator.h>
#include <google/protobuf/compiler/plugin.h>

namespace google {
namespace protobuf {
namespace compiler {
int ProtobufMain(int argc, char* argv[]) {
	MockCodeGenerator generator("test_plugin");
	return PluginMain(argc, argv, &generator);
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

int main(int argc, char* argv[]) {
#ifdef _MSC_VER
	// Don't print a silly message or stick a modal dialog box in my face,
	// please.
	_set_abort_behavior(0, ~0);
#endif  // !_MSC_VER
	return google::protobuf::compiler::ProtobufMain(argc, argv);
}
