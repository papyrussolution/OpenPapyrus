// Copyright 2017 The Abseil Authors.
// @license Apache License 2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/types/bad_variant_access.h"

#ifndef ABSL_USES_STD_VARIANT

namespace absl {
ABSL_NAMESPACE_BEGIN
//
// [variant.bad.access] //
//
bad_variant_access::~bad_variant_access() = default;

const char* bad_variant_access::what() const noexcept { return "Bad variant access"; }

namespace variant_internal {
void ThrowBadVariantAccess() {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw bad_variant_access();
#else
	ABSL_RAW_LOG(FATAL, "Bad variant access");
	abort(); // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
}

void Rethrow() {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw;
#else
	ABSL_RAW_LOG(FATAL,
	    "Internal error in absl::variant implementation. Attempted to "
	    "rethrow an exception when building with exceptions disabled.");
	abort(); // TODO(calabrese) Remove once RAW_LOG FATAL is noreturn.
#endif
}
}  // namespace variant_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_USES_STD_VARIANT
