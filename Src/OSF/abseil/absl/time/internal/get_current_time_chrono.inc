// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
namespace absl {
	ABSL_NAMESPACE_BEGIN
	namespace time_internal {
		static int64_t GetCurrentTimeNanosFromSystem() {
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - std::chrono::system_clock::from_time_t(0)).count();
		}
	} // namespace time_internal
	ABSL_NAMESPACE_END
}  // namespace absl
