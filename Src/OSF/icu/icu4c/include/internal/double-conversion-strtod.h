// © 2018 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
//
// From the double-conversion library. Original license:
//
// Copyright 2010 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//  * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// ICU PATCH: ifdef around UCONFIG_NO_FORMATTING
#include "unicode/utypes.h"
#if !UCONFIG_NO_FORMATTING

#ifndef DOUBLE_CONVERSION_STRTOD_H_
#define DOUBLE_CONVERSION_STRTOD_H_

// ICU PATCH: Customize header file paths for ICU.

#include "double-conversion-utils.h"

// ICU PATCH: Wrap in ICU namespace
U_NAMESPACE_BEGIN

namespace double_conversion {
// The buffer must only contain digits in the range [0-9]. It must not
// contain a dot or a sign. It must not start with '0', and must not be empty.
double Strtod(Vector<const char> buffer, int exponent);

// The buffer must only contain digits in the range [0-9]. It must not
// contain a dot or a sign. It must not start with '0', and must not be empty.
float Strtof(Vector<const char> buffer, int exponent);

// Same as Strtod, but assumes that 'trimmed' is already trimmed, as if run
// through TrimAndCut. That is, 'trimmed' must have no leading or trailing
// zeros, must not be a lone zero, and must not have 'too many' digits.
double StrtodTrimmed(Vector<const char> trimmed, int exponent);

// Same as Strtof, but assumes that 'trimmed' is already trimmed, as if run
// through TrimAndCut. That is, 'trimmed' must have no leading or trailing
// zeros, must not be a lone zero, and must not have 'too many' digits.
float StrtofTrimmed(Vector<const char> trimmed, int exponent);

inline Vector<const char> TrimTrailingZeros(Vector<const char> buffer) 
{
	for(int i = buffer.length() - 1; i >= 0; --i) {
		if(buffer[i] != '0') {
			return buffer.SubVector(0, i + 1);
		}
	}
	return Vector<const char>(buffer.start(), 0);
}
}  // namespace double_conversion

// ICU PATCH: Close ICU namespace
U_NAMESPACE_END

#endif  // DOUBLE_CONVERSION_STRTOD_H_
#endif // ICU PATCH: close #if !UCONFIG_NO_FORMATTING
