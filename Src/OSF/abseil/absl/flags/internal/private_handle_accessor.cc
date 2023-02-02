//
// Copyright 2020 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/flags/commandlineflag.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace flags_internal {
FlagFastTypeId PrivateHandleAccessor::TypeId(const CommandLineFlag& flag) {
	return flag.TypeId();
}

std::unique_ptr<FlagStateInterface> PrivateHandleAccessor::SaveState(CommandLineFlag& flag) {
	return flag.SaveState();
}

bool PrivateHandleAccessor::IsSpecifiedOnCommandLine(const CommandLineFlag& flag) {
	return flag.IsSpecifiedOnCommandLine();
}

bool PrivateHandleAccessor::ValidateInputValue(const CommandLineFlag& flag,
    absl::string_view value) {
	return flag.ValidateInputValue(value);
}

void PrivateHandleAccessor::CheckDefaultValueParsingRoundtrip(const CommandLineFlag& flag) {
	flag.CheckDefaultValueParsingRoundtrip();
}

bool PrivateHandleAccessor::ParseFrom(CommandLineFlag& flag,
    absl::string_view value,
    flags_internal::FlagSettingMode set_mode,
    flags_internal::ValueSource source,
    std::string & error) {
	return flag.ParseFrom(value, set_mode, source, error);
}
}  // namespace flags_internal
ABSL_NAMESPACE_END
}  // namespace absl
