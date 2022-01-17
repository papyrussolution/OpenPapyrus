// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/random/seed_sequences.h"

namespace absl {
ABSL_NAMESPACE_BEGIN

SeedSeq MakeSeedSeq() {
	SeedSeq::result_type seed_material[8];
	random_internal::RandenPool<uint32_t>::Fill(absl::MakeSpan(seed_material));
	return SeedSeq(std::begin(seed_material), std::end(seed_material));
}

ABSL_NAMESPACE_END
}  // namespace absl
