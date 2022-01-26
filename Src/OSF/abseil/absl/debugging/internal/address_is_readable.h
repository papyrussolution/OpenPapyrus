// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#ifndef ABSL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
#define ABSL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_

#include "absl/base/config.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace debugging_internal {

// Return whether the byte at *addr is readable, without faulting.
// Save and restores errno.
bool AddressIsReadable(const void *addr);

}  // namespace debugging_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
