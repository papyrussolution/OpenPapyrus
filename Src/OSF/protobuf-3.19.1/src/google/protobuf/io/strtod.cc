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
#include <google/protobuf/io/strtod.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace io {
// This approximately 0x1.ffffffp127, but we don't use 0x1.ffffffp127 because
// it won't compile in MSVC.
const double MAX_FLOAT_AS_DOUBLE_ROUNDED = 3.4028235677973366e+38;

float SafeDoubleToFloat(double value) {
	// static_cast<float> on a number larger than float can result in illegal
	// instruction error, so we need to manually convert it to infinity or max.
	if(value > std::numeric_limits<float>::max()) {
		// Max float value is about 3.4028234664E38 when represented as a double.
		// However, when printing float as text, it will be rounded as
		// 3.4028235e+38. If we parse the value of 3.4028235e+38 from text and
		// compare it to 3.4028234664E38, we may think that it is larger, but
		// actually, any number between these two numbers could only be represented
		// as the same max float number in float, so we should treat them the same
		// as max float.
		if(value <= MAX_FLOAT_AS_DOUBLE_ROUNDED) {
			return std::numeric_limits<float>::max();
		}
		return std::numeric_limits<float>::infinity();
	}
	else if(value < -std::numeric_limits<float>::max()) {
		if(value >= -MAX_FLOAT_AS_DOUBLE_ROUNDED) {
			return -std::numeric_limits<float>::max();
		}
		return -std::numeric_limits<float>::infinity();
	}
	else {
		return static_cast<float>(value);
	}
}

double NoLocaleStrtod(const char* str, char** endptr) {
	return google::protobuf::internal::NoLocaleStrtod(str, endptr);
}
}  // namespace io
}  // namespace protobuf
}  // namespace google
