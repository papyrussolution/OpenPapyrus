// Copyright 2020 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/status/statusor.h"
#include "absl/status/status.h"

namespace absl {
ABSL_NAMESPACE_BEGIN

BadStatusOrAccess::BadStatusOrAccess(absl::Status status)
	: status_(std::move(status)) {
}

BadStatusOrAccess::BadStatusOrAccess(const BadStatusOrAccess& other)
	: status_(other.status_) {
}

BadStatusOrAccess& BadStatusOrAccess::operator = (const BadStatusOrAccess& other) {
	// Ensure assignment is correct regardless of whether this->InitWhat() has
	// already been called.
	other.InitWhat();
	status_ = other.status_;
	what_ = other.what_;
	return *this;
}

BadStatusOrAccess& BadStatusOrAccess::operator = (BadStatusOrAccess&& other) {
	// Ensure assignment is correct regardless of whether this->InitWhat() has
	// already been called.
	other.InitWhat();
	status_ = std::move(other.status_);
	what_ = std::move(other.what_);
	return *this;
}

BadStatusOrAccess::BadStatusOrAccess(BadStatusOrAccess&& other)
	: status_(std::move(other.status_)) {
}

const char* BadStatusOrAccess::what() const noexcept {
	InitWhat();
	return what_.c_str();
}

const absl::Status& BadStatusOrAccess::status() const {
	return status_;
}

void BadStatusOrAccess::InitWhat() const {
	absl::call_once(init_what_, [this] {
			what_ = absl::StrCat("Bad StatusOr access: ", status_.ToString());
		});
}

namespace internal_statusor {
void Helper::HandleInvalidStatusCtorArg(absl::Status* status) {
	const char* kMessage =
	    "An OK status is not a valid constructor argument to StatusOr<T>";
#ifdef NDEBUG
	ABSL_INTERNAL_LOG(ERROR, kMessage);
#else
	ABSL_INTERNAL_LOG(FATAL, kMessage);
#endif
	// In optimized builds, we will fall back to InternalError.
	*status = absl::InternalError(kMessage);
}

void Helper::Crash(const absl::Status& status) {
	ABSL_INTERNAL_LOG(
		FATAL,
		absl::StrCat("Attempting to fetch value instead of handling error ",
		status.ToString()));
}

void ThrowBadStatusOrAccess(absl::Status status) {
#ifdef ABSL_HAVE_EXCEPTIONS
	throw absl::BadStatusOrAccess(std::move(status));
#else
	ABSL_INTERNAL_LOG(
		FATAL,
		absl::StrCat("Attempting to fetch value instead of handling error ",
		status.ToString()));
	std::abort();
#endif
}
}  // namespace internal_statusor
ABSL_NAMESPACE_END
}  // namespace absl
