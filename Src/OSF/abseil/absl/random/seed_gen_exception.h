// seed_gen_exception.h
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
// This header defines an exception class which may be thrown if unpredictable
// events prevent the derivation of suitable seed-material for constructing a
// bit generator conforming to [rand.req.urng] (eg. entropy cannot be read from
// /dev/urandom on a Unix-based system).
//
// Note: if exceptions are disabled, `std::terminate()` is called instead.

#ifndef ABSL_RANDOM_SEED_GEN_EXCEPTION_H_
#define ABSL_RANDOM_SEED_GEN_EXCEPTION_H_

#include <exception>
#include "absl/base/config.h"

namespace absl {
ABSL_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// SeedGenException
//------------------------------------------------------------------------------
class SeedGenException : public std::exception {
public:
	SeedGenException() = default;
	~SeedGenException() override;
	const char* what() const noexcept override;
};

namespace random_internal {
// throw delegator
[[noreturn]] void ThrowSeedGenException();
}  // namespace random_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_RANDOM_SEED_GEN_EXCEPTION_H_
