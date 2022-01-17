// Copyright (C) 2011 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
#ifndef I18N_PHONENUMBERS_ENCODING_UTILS_H_
#define I18N_PHONENUMBERS_ENCODING_UTILS_H_

#include "basictypes.h"
#include "unilib.h"
#include "utf.h"

namespace i18n {
namespace phonenumbers {

class EncodingUtils {
 public:
  // Decodes one Unicode code-point value from a UTF-8 array. Returns the number
  // of bytes read from the array. If the array does not contain valid UTF-8,
  // the function stores 0xFFFD in the output variable and returns 1.
  static inline int DecodeUTF8Char(const char* in, char32* out) {
    Rune r;
    int len = chartorune(&r, in);
    *out = r;
    return len;
  }

  static const char* AdvanceOneUTF8Character(const char* buf_utf8) {
      return buf_utf8 + UniLib::OneCharLen(buf_utf8);
  }

  static const char* BackUpOneUTF8Character(const char* start,
                                            const char* end) {
    while (start < end && UniLib::IsTrailByte(*--end)) {}
    return end;
  }
};

}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_ENCODING_UTILS_H_
