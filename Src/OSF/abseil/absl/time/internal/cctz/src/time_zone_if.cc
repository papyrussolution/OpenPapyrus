// Copyright 2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "time_zone_if.h"
#include "time_zone_info.h"
#include "time_zone_libc.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace time_internal {
namespace cctz {
std::unique_ptr<TimeZoneIf> TimeZoneIf::Load(const std::string & name) {
	// Support "libc:localtime" and "libc:*" to access the legacy
	// localtime and UTC support respectively from the C library.
	if(name.compare(0, 5, "libc:") == 0) {
		return std::unique_ptr<TimeZoneIf>(new TimeZoneLibC(name.substr(5)));
	}

	// Otherwise use the "zoneinfo" implementation by default.
	std::unique_ptr<TimeZoneInfo> tz(new TimeZoneInfo);
	if(!tz->Load(name)) tz.reset();
	return std::unique_ptr<TimeZoneIf>(tz.release());
}

// Defined out-of-line to avoid emitting a weak vtable in all TUs.
TimeZoneIf::~TimeZoneIf() {
}
}  // namespace cctz
}  // namespace time_internal
ABSL_NAMESPACE_END
}  // namespace absl
