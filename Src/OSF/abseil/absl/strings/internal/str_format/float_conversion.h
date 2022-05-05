// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
#ifndef ABSL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
#define ABSL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_

#include "absl/strings/internal/str_format/extension.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace str_format_internal {
bool ConvertFloatImpl(float v, const FormatConversionSpecImpl &conv, FormatSinkImpl * sink);
bool ConvertFloatImpl(double v, const FormatConversionSpecImpl &conv, FormatSinkImpl * sink);
bool ConvertFloatImpl(long double v, const FormatConversionSpecImpl &conv, FormatSinkImpl * sink);
}  // namespace str_format_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
