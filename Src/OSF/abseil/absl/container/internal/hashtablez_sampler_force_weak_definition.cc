// Copyright 2018 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/container/internal/hashtablez_sampler.h"

namespace absl {
	ABSL_NAMESPACE_BEGIN
		namespace container_internal {
			// See hashtablez_sampler.h for details.
			extern "C" ABSL_ATTRIBUTE_WEAK bool ABSL_INTERNAL_C_SYMBOL(AbslContainerInternalSampleEverything)() { return false; }
		}
	ABSL_NAMESPACE_END
}
