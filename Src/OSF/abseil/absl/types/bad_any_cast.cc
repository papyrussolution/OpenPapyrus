// Copyright 2017 The Abseil Authors.
// @license Apache License 2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/types/bad_any_cast.h"

#ifndef ABSL_USES_STD_ANY

namespace absl {
ABSL_NAMESPACE_BEGIN bad_any_cast::~bad_any_cast() = default;

const char* bad_any_cast::what() const noexcept {
	return "Bad any cast";
}

namespace any_internal {
void ThrowBadAnyCast() {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw bad_any_cast();
#else
	ABSL_RAW_LOG(FATAL, "Bad any cast");
	std::abort();
#endif
}
}  // namespace any_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_USES_STD_ANY
