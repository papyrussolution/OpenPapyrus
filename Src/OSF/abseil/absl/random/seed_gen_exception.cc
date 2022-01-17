// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN

static constexpr const char kExceptionMessage[] = "Failed generating seed-material for URBG.";

SeedGenException::~SeedGenException() = default;

const char* SeedGenException::what() const noexcept {
	return kExceptionMessage;
}

namespace random_internal {
void ThrowSeedGenException() {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw absl::SeedGenException();
#else
	std::cerr << kExceptionMessage << std::endl;
	std::terminate();
#endif
}
}  // namespace random_internal
ABSL_NAMESPACE_END
}  // namespace absl
