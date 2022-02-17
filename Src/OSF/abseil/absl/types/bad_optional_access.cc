// Copyright 2017 The Abseil Authors.
// @license Apache License 2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/types/bad_optional_access.h"

#ifndef ABSL_USES_STD_OPTIONAL

namespace absl {
ABSL_NAMESPACE_BEGIN bad_optional_access::~bad_optional_access() = default;

const char* bad_optional_access::what() const noexcept {
	return "optional has no value";
}

namespace optional_internal {
void throw_bad_optional_access() {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw bad_optional_access();
#else
	ABSL_RAW_LOG(FATAL, "Bad optional access");
	abort();
#endif
}
}  // namespace optional_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_USES_STD_OPTIONAL
