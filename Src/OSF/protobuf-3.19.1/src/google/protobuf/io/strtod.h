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
// A locale-independent version of strtod(), used to parse floating
// point default values in .proto files, where the decimal separator
// is always a dot.

#ifndef GOOGLE_PROTOBUF_IO_STRTOD_H__
#define GOOGLE_PROTOBUF_IO_STRTOD_H__

namespace google {
namespace protobuf {
namespace io {

// A locale-independent version of the standard strtod(), which always
// uses a dot as the decimal separator.
double NoLocaleStrtod(const char* str, char** endptr);

// Casts a double value to a float value. If the value is outside of the
// representable range of float, it will be converted to positive or negative
// infinity.
float SafeDoubleToFloat(double value);

}  // namespace io
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_IO_STRTOD_H__
