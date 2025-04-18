// Copyright 2018 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/container/internal/hashtablez_sampler.h"
#include "absl/container/internal/have_sse.h"
#include "absl/debugging/stacktrace.h"
#include "absl/profiling/internal/exponential_biased.h"
#include "absl/profiling/internal/sample_recorder.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace container_internal {
constexpr int HashtablezInfo::kMaxStackDepth;

namespace {
ABSL_CONST_INIT std::atomic<bool> g_hashtablez_enabled{
	false
};
ABSL_CONST_INIT std::atomic<int32_t> g_hashtablez_sample_parameter{1 << 10};
std::atomic<HashtablezConfigListener> g_hashtablez_config_listener{nullptr};

#if defined(ABSL_INTERNAL_HASHTABLEZ_SAMPLE)
ABSL_PER_THREAD_TLS_KEYWORD absl::profiling_internal::ExponentialBiased
    g_exponential_biased_generator;
#endif

void TriggerHashtablezConfigListener() {
	auto* listener = g_hashtablez_config_listener.load(std::memory_order_acquire);
	if(listener != nullptr) listener();
}
}  // namespace

#if defined(ABSL_INTERNAL_HASHTABLEZ_SAMPLE)
ABSL_PER_THREAD_TLS_KEYWORD int64_t global_next_sample = 0;
#endif  // defined(ABSL_INTERNAL_HASHTABLEZ_SAMPLE)

HashtablezSampler& GlobalHashtablezSampler() {
	static auto* sampler = new HashtablezSampler();
	return *sampler;
}

HashtablezInfo::HashtablezInfo() = default;
HashtablezInfo::~HashtablezInfo() = default;

void HashtablezInfo::PrepareForSampling(size_t inline_element_size_value) {
	capacity.store(0, std::memory_order_relaxed);
	size.store(0, std::memory_order_relaxed);
	num_erases.store(0, std::memory_order_relaxed);
	num_rehashes.store(0, std::memory_order_relaxed);
	max_probe_length.store(0, std::memory_order_relaxed);
	total_probe_length.store(0, std::memory_order_relaxed);
	hashes_bitwise_or.store(0, std::memory_order_relaxed);
	hashes_bitwise_and.store(~size_t{}, std::memory_order_relaxed);
	hashes_bitwise_xor.store(0, std::memory_order_relaxed);
	max_reserve.store(0, std::memory_order_relaxed);

	create_time = absl::Now();
	// The inliner makes hardcoded skip_count difficult (especially when combined
	// with LTO).  We use the ability to exclude stacks by regex when encoding
	// instead.
	depth = absl::GetStackTrace(stack, HashtablezInfo::kMaxStackDepth,
	        /* skip_count= */ 0);
	inline_element_size = inline_element_size_value;
}

static bool ShouldForceSampling() {
	enum ForceState {
		kDontForce,
		kForce,
		kUninitialized
	};

	ABSL_CONST_INIT static std::atomic<ForceState> global_state{
		kUninitialized
	};
	ForceState state = global_state.load(std::memory_order_relaxed);
	if(ABSL_PREDICT_TRUE(state == kDontForce)) return false;

	if(state == kUninitialized) {
		state = ABSL_INTERNAL_C_SYMBOL(AbslContainerInternalSampleEverything)()
		    ? kForce
		    : kDontForce;
		global_state.store(state, std::memory_order_relaxed);
	}
	return state == kForce;
}

HashtablezInfo* SampleSlow(int64_t* next_sample, size_t inline_element_size) {
	if(ABSL_PREDICT_FALSE(ShouldForceSampling())) {
		*next_sample = 1;
		HashtablezInfo* result =
		    GlobalHashtablezSampler().Register(inline_element_size);
		return result;
	}

#if !defined(ABSL_INTERNAL_HASHTABLEZ_SAMPLE)
	*next_sample = std::numeric_limits<int64_t>::max();
	return nullptr;
#else
	bool first = *next_sample < 0;
	*next_sample = g_exponential_biased_generator.GetStride(
		g_hashtablez_sample_parameter.load(std::memory_order_relaxed));
	// Small values of interval are equivalent to just sampling next time.
	ABSL_ASSERT(*next_sample >= 1);

	// g_hashtablez_enabled can be dynamically flipped, we need to set a threshold
	// low enough that we will start sampling in a reasonable time, so we just use
	// the default sampling rate.
	if(!g_hashtablez_enabled.load(std::memory_order_relaxed)) return nullptr;

	// We will only be negative on our first count, so we should just retry in
	// that case.
	if(first) {
		if(ABSL_PREDICT_TRUE(--*next_sample > 0)) return nullptr;
		return SampleSlow(next_sample, inline_element_size);
	}

	return GlobalHashtablezSampler().Register(inline_element_size);
#endif
}

void UnsampleSlow(HashtablezInfo* info) {
	GlobalHashtablezSampler().Unregister(info);
}

void RecordInsertSlow(HashtablezInfo* info, size_t hash, size_t distance_from_desired) 
{
	// SwissTables probe in groups of 16, so scale this to count items probes and
	// not offset from desired.
	size_t probe_length = distance_from_desired;
#if ABSL_INTERNAL_RAW_HASH_SET_HAVE_SSE2
	probe_length /= 16;
#else
	probe_length /= 8;
#endif
	info->hashes_bitwise_and.fetch_and(hash, std::memory_order_relaxed);
	info->hashes_bitwise_or.fetch_or(hash, std::memory_order_relaxed);
	info->hashes_bitwise_xor.fetch_xor(hash, std::memory_order_relaxed);
	info->max_probe_length.store(std::max(info->max_probe_length.load(std::memory_order_relaxed), probe_length), std::memory_order_relaxed);
	info->total_probe_length.fetch_add(probe_length, std::memory_order_relaxed);
	info->size.fetch_add(1, std::memory_order_relaxed);
}

void SetHashtablezConfigListener(HashtablezConfigListener l) {
	g_hashtablez_config_listener.store(l, std::memory_order_release);
}

bool IsHashtablezEnabled() { return g_hashtablez_enabled.load(std::memory_order_acquire); }

void SetHashtablezEnabled(bool enabled) 
{
	SetHashtablezEnabledInternal(enabled);
	TriggerHashtablezConfigListener();
}

void SetHashtablezEnabledInternal(bool enabled) { g_hashtablez_enabled.store(enabled, std::memory_order_release); }
int32_t GetHashtablezSampleParameter() { return g_hashtablez_sample_parameter.load(std::memory_order_acquire); }

void SetHashtablezSampleParameter(int32_t rate) 
{
	SetHashtablezSampleParameterInternal(rate);
	TriggerHashtablezConfigListener();
}

void SetHashtablezSampleParameterInternal(int32_t rate) 
{
	if(rate > 0) {
		g_hashtablez_sample_parameter.store(rate, std::memory_order_release);
	}
	else {
		ABSL_RAW_LOG(ERROR, "Invalid hashtablez sample rate: %lld", static_cast<long long>(rate)); // NOLINT(runtime/int)
	}
}

int32_t GetHashtablezMaxSamples() { return GlobalHashtablezSampler().GetMaxSamples(); }

void SetHashtablezMaxSamples(int32_t max) 
{
	SetHashtablezMaxSamplesInternal(max);
	TriggerHashtablezConfigListener();
}

void SetHashtablezMaxSamplesInternal(int32_t max) 
{
	if(max > 0) {
		GlobalHashtablezSampler().SetMaxSamples(max);
	}
	else {
		ABSL_RAW_LOG(ERROR, "Invalid hashtablez max samples: %lld", static_cast<long long>(max)); // NOLINT(runtime/int)
	}
}
}  // namespace container_internal
ABSL_NAMESPACE_END
}  // namespace absl
