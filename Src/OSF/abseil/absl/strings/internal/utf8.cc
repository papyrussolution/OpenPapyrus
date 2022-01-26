// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
// UTF8 utilities, implemented to reduce dependencies.
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/strings/internal/utf8.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace strings_internal {
size_t EncodeUTF8Char(char * buffer, char32_t utf8_char) 
{
	if(utf8_char <= 0x7F) {
		*buffer = static_cast<char>(utf8_char);
		return 1;
	}
	else if(utf8_char <= 0x7FF) {
		buffer[1] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xC0 | utf8_char);
		return 2;
	}
	else if(utf8_char <= 0xFFFF) {
		buffer[2] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[1] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xE0 | utf8_char);
		return 3;
	}
	else {
		buffer[3] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[2] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[1] = 0x80 | (utf8_char & 0x3F);
		utf8_char >>= 6;
		buffer[0] = static_cast<char>(0xF0 | utf8_char);
		return 4;
	}
}
}  // namespace strings_internal
ABSL_NAMESPACE_END
}  // namespace absl
