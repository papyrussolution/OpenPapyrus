// Copyright 2019 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/strings/internal/cordz_sample_token.h"
#include "absl/strings/internal/cordz_handle.h"
#include "absl/strings/internal/cordz_info.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace cord_internal {
CordzSampleToken::Iterator& CordzSampleToken::Iterator::operator++() {
	if(current_) {
		current_ = current_->Next(*token_);
	}
	return *this;
}

CordzSampleToken::Iterator CordzSampleToken::Iterator::operator++(int) {
	Iterator it(*this);
	operator++();
	return it;
}

bool operator==(const CordzSampleToken::Iterator& lhs,
    const CordzSampleToken::Iterator& rhs) {
	return lhs.current_ == rhs.current_ &&
	       (lhs.current_ == nullptr || lhs.token_ == rhs.token_);
}

bool operator!=(const CordzSampleToken::Iterator& lhs,
    const CordzSampleToken::Iterator& rhs) {
	return !(lhs == rhs);
}

CordzSampleToken::Iterator::reference CordzSampleToken::Iterator::operator*()
const {
	return *current_;
}

CordzSampleToken::Iterator::pointer CordzSampleToken::Iterator::operator->()
const {
	return current_;
}

CordzSampleToken::Iterator::Iterator(const CordzSampleToken* token)
	: token_(token), current_(CordzInfo::Head(*token)) {
}
}  // namespace cord_internal
ABSL_NAMESPACE_END
}  // namespace absl
