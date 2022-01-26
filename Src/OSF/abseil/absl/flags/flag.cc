//
//  Copyright 2019 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/flags/flag.h"

namespace absl {
ABSL_NAMESPACE_BEGIN

// This global mutex protects on-demand construction of flag objects in MSVC
// builds.
#if defined(_MSC_VER) && !defined(__clang__)

namespace flags_internal {
ABSL_CONST_INIT static absl::Mutex construction_guard(absl::kConstInit);

absl::Mutex* GetGlobalConstructionGuard() {
	return &construction_guard;
}
}  // namespace flags_internal

#endif

ABSL_NAMESPACE_END
}  // namespace absl
