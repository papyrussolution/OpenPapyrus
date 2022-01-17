// Copyright 2018 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/container/internal/raw_hash_set.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace container_internal {
alignas(16) ABSL_CONST_INIT ABSL_DLL const ctrl_t kEmptyGroup[16] = {
	ctrl_t::kSentinel, ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty,
	ctrl_t::kEmpty,    ctrl_t::kEmpty, ctrl_t::kEmpty, ctrl_t::kEmpty
};

constexpr size_t Group::kWidth;

// Returns "random" seed.
inline size_t RandomSeed() {
#ifdef ABSL_HAVE_THREAD_LOCAL
	static thread_local size_t counter = 0;
	size_t value = ++counter;
#else   // ABSL_HAVE_THREAD_LOCAL
	static std::atomic<size_t> counter(0);
	size_t value = counter.fetch_add(1, std::memory_order_relaxed);
#endif  // ABSL_HAVE_THREAD_LOCAL
	return value ^ static_cast<size_t>(reinterpret_cast<uintptr_t>(&counter));
}

bool ShouldInsertBackwards(size_t hash, const ctrl_t* ctrl) {
	// To avoid problems with weak hashes and single bit tests, we use % 13.
	// TODO(kfm,sbenza): revisit after we do unconditional mixing
	return (H1(hash, ctrl) ^ RandomSeed()) % 13 > 6;
}

void ConvertDeletedToEmptyAndFullToDeleted(ctrl_t* ctrl, size_t capacity) {
	assert(ctrl[capacity] == ctrl_t::kSentinel);
	assert(IsValidCapacity(capacity));
	for(ctrl_t* pos = ctrl; pos < ctrl + capacity; pos += Group::kWidth) {
		Group{pos}.ConvertSpecialToEmptyAndFullToDeleted(pos);
	}
	// Copy the cloned ctrl bytes.
	memcpy(ctrl + capacity + 1, ctrl, NumClonedBytes());
	ctrl[capacity] = ctrl_t::kSentinel;
}

// Extern template instantiotion for inline function.
template FindInfo find_first_non_full(const ctrl_t*, size_t, size_t);
}  // namespace container_internal
ABSL_NAMESPACE_END
}  // namespace absl
