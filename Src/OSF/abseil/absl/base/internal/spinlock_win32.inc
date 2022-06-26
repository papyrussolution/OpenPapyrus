// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// This file is a Win32-specific part of spinlock_wait.cc
//
#include "absl/base/internal/scheduling_mode.h"

extern "C" {
void ABSL_INTERNAL_C_SYMBOL(AbslInternalSpinLockDelay)(
	std::atomic<uint32_t>*/* lock_word */, uint32_t /* value */, int loop,
	absl::base_internal::SchedulingMode /* mode */) {
	if(loop == 0) {
	}
	else if(loop == 1) {
		Sleep(0);
	}
	else {
		Sleep(absl::base_internal::SpinLockSuggestedDelayNS(loop) / 1000000);
	}
}

void ABSL_INTERNAL_C_SYMBOL(AbslInternalSpinLockWake)(
	std::atomic<uint32_t>*/* lock_word */, bool /* all */) {
}
}  // extern "C"
