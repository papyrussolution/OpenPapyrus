// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// UTF8 utilities, implemented to reduce dependencies.
//
#ifndef ABSL_STRINGS_INTERNAL_UTF8_H_
#define ABSL_STRINGS_INTERNAL_UTF8_H_

//#include <cstddef>
//#include <cstdint>
#include "absl/base/config.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace strings_internal {

// For Unicode code points 0 through 0x10FFFF, EncodeUTF8Char writes
// out the UTF-8 encoding into buffer, and returns the number of chars
// it wrote.
//
// As described in https://tools.ietf.org/html/rfc3629#section-3 , the encodings
// are:
//    00 -     7F : 0xxxxxxx
//    80 -    7FF : 110xxxxx 10xxxxxx
//   800 -   FFFF : 1110xxxx 10xxxxxx 10xxxxxx
// 10000 - 10FFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
// Values greater than 0x10FFFF are not supported and may or may not write
// characters into buffer, however never will more than kMaxEncodedUTF8Size
// bytes be written, regardless of the value of utf8_char.
enum { kMaxEncodedUTF8Size = 4 };
size_t EncodeUTF8Char(char *buffer, char32_t utf8_char);

}  // namespace strings_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_STRINGS_INTERNAL_UTF8_H_
