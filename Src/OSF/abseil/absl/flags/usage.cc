// Copyright 2019 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/flags/usage.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace flags_internal {
namespace {
ABSL_CONST_INIT absl::Mutex usage_message_guard(absl::kConstInit);
ABSL_CONST_INIT std::string* program_usage_message
    ABSL_GUARDED_BY(usage_message_guard) = nullptr;
}  // namespace
}  // namespace flags_internal

// --------------------------------------------------------------------
// Sets the "usage" message to be used by help reporting routines.
void SetProgramUsageMessage(absl::string_view new_usage_message) {
	absl::MutexLock l(&flags_internal::usage_message_guard);

	if(flags_internal::program_usage_message != nullptr) {
		ABSL_INTERNAL_LOG(FATAL, "SetProgramUsageMessage() called twice.");
		std::exit(1);
	}

	flags_internal::program_usage_message = new std::string(new_usage_message);
}

// --------------------------------------------------------------------
// Returns the usage message set by SetProgramUsageMessage().
// Note: We able to return string_view here only because calling
// SetProgramUsageMessage twice is prohibited.
absl::string_view ProgramUsageMessage() {
	absl::MutexLock l(&flags_internal::usage_message_guard);

	return flags_internal::program_usage_message != nullptr
	       ? absl::string_view(*flags_internal::program_usage_message)
	       : "Warning: SetProgramUsageMessage() never called";
}

ABSL_NAMESPACE_END
}  // namespace absl
