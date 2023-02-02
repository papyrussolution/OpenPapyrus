// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
#ifndef ABSL_RANDOM_INTERNAL_RANDEN_DETECT_H_
#define ABSL_RANDOM_INTERNAL_RANDEN_DETECT_H_

#include "absl/base/config.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace random_internal {

// Returns whether the current CPU supports RandenHwAes implementation.
// This typically involves supporting cryptographic extensions on whichever
// platform is currently running.
bool CPUSupportsRandenHwAes();

}  // namespace random_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_RANDOM_INTERNAL_RANDEN_DETECT_H_
