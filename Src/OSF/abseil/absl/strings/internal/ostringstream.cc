// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop
#include "absl/strings/internal/ostringstream.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace strings_internal {
OStringStream::Buf::int_type OStringStream::overflow(int c) {
	assert(s_);
	if(!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof()))
		s_->push_back(static_cast<char>(c));
	return 1;
}

std::streamsize OStringStream::xsputn(const char* s, std::streamsize n) 
{
	assert(s_);
	s_->append(s, n);
	return n;
}
}  // namespace strings_internal
ABSL_NAMESPACE_END
}  // namespace absl
