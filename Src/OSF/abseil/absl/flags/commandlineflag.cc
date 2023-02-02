//
// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
	ABSL_NAMESPACE_BEGIN
	bool CommandLineFlag::IsRetired() const { return false; }

	bool CommandLineFlag::ParseFrom(absl::string_view value, std::string* error) 
	{
		return ParseFrom(value, flags_internal::SET_FLAGS_VALUE, flags_internal::kProgrammaticChange, *error);
	}
	ABSL_NAMESPACE_END
}
