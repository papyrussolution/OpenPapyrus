// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace base_internal {
// NOTE: The various STL exception throwing functions are placed within the
// #ifdef blocks so the symbols aren't exposed on platforms that don't support
// them, such as the Android NDK. For example, ANGLE fails to link when building
// within AOSP without them, since the STL functions don't exist.
namespace {
#ifdef ABSL_HAVE_EXCEPTIONS
template <typename T>
[[noreturn]] void Throw(const T& error) {
	throw error;
}

#endif
}  // namespace

void ThrowStdLogicError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::logic_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdLogicError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::logic_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdInvalidArgument(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::invalid_argument(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdInvalidArgument(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::invalid_argument(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdDomainError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::domain_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdDomainError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::domain_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdLengthError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::length_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdLengthError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::length_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdOutOfRange(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::out_of_range(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdOutOfRange(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::out_of_range(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdRuntimeError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::runtime_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdRuntimeError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::runtime_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdRangeError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::range_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdRangeError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::range_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdOverflowError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::overflow_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdOverflowError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::overflow_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdUnderflowError(const std::string & what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::underflow_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg.c_str());
	std::abort();
#endif
}

void ThrowStdUnderflowError(const char* what_arg) {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::underflow_error(what_arg));
#else
	ABSL_RAW_LOG(FATAL, "%s", what_arg);
	std::abort();
#endif
}

void ThrowStdBadFunctionCall() {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::bad_function_call());
#else
	std::abort();
#endif
}

void ThrowStdBadAlloc() {
#ifdef ABSL_HAVE_EXCEPTIONS
	Throw(std::bad_alloc());
#else
	std::abort();
#endif
}
}  // namespace base_internal
ABSL_NAMESPACE_END
}  // namespace absl
