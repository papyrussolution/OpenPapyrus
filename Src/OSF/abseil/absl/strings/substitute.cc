// Copyright 2017 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace substitute_internal {
void SubstituteAndAppendArray(std::string* output, absl::string_view format, const absl::string_view* args_array, size_t num_args) 
{
	// Determine total size needed.
	size_t size = 0;
	for(size_t i = 0; i < format.size(); i++) {
		if(format[i] == '$') {
			if(i + 1 >= format.size()) {
#ifndef NDEBUG
				ABSL_RAW_LOG(FATAL, "Invalid absl::Substitute() format string: \"%s\".", absl::CEscape(format).c_str());
#endif
				return;
			}
			else if(absl::ascii_isdigit(format[i + 1])) {
				int index = format[i + 1] - '0';
				if(static_cast<size_t>(index) >= num_args) {
#ifndef NDEBUG
					ABSL_RAW_LOG(FATAL,
						"Invalid absl::Substitute() format string: asked for \"$"
						"%d\", but only %d args were given.  Full format string was: "
						"\"%s\".",
						index, static_cast<int>(num_args), absl::CEscape(format).c_str());
#endif
					return;
				}
				size += args_array[index].size();
				++i; // Skip next char.
			}
			else if(format[i + 1] == '$') {
				++size;
				++i; // Skip next char.
			}
			else {
#ifndef NDEBUG
				ABSL_RAW_LOG(FATAL, "Invalid absl::Substitute() format string: \"%s\".", absl::CEscape(format).c_str());
#endif
				return;
			}
		}
		else {
			++size;
		}
	}
	if(size == 0) 
		return;
	// Build the string.
	size_t original_size = output->size();
	strings_internal::STLStringResizeUninitializedAmortized(output, original_size + size);
	char* target = &(*output)[original_size];
	for(size_t i = 0; i < format.size(); i++) {
		if(format[i] == '$') {
			if(absl::ascii_isdigit(format[i + 1])) {
				const absl::string_view src = args_array[format[i + 1] - '0'];
				target = std::copy(src.begin(), src.end(), target);
				++i; // Skip next char.
			}
			else if(format[i + 1] == '$') {
				*target++ = '$';
				++i; // Skip next char.
			}
		}
		else {
			*target++ = format[i];
		}
	}

	assert(target == output->data() + output->size());
}

Arg::Arg(const void* value) 
{
	static_assert(sizeof(scratch_) >= sizeof(value) * 2 + 2, "fix sizeof(scratch_)");
	if(value == nullptr) {
		piece_ = "NULL";
	}
	else {
		char* ptr = scratch_ + sizeof(scratch_);
		uintptr_t num = reinterpret_cast<uintptr_t>(value);
		do {
			*--ptr = SlConst::P_HxDigL[num & 0xf];
			num >>= 4;
		} while(num != 0);
		*--ptr = 'x';
		*--ptr = '0';
		piece_ = absl::string_view(ptr, scratch_ + sizeof(scratch_) - ptr);
	}
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(Hex hex) 
{
	char* const end = &scratch_[numbers_internal::kFastToBufferSize];
	char* writer = end;
	uint64_t value = hex.value;
	do {
		*--writer = SlConst::P_HxDigL[value & 0xF];
		value >>= 4;
	} while(value != 0);

	char* beg;
	if(end - writer < hex.width) {
		beg = end - hex.width;
		std::fill_n(beg, writer - beg, hex.fill);
	}
	else {
		beg = writer;
	}
	piece_ = absl::string_view(beg, end - beg);
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(Dec dec) 
{
	assert(dec.width <= numbers_internal::kFastToBufferSize);
	char* const end = &scratch_[numbers_internal::kFastToBufferSize];
	char* const minfill = end - dec.width;
	char* writer = end;
	uint64_t value = dec.value;
	bool neg = dec.neg;
	while(value > 9) {
		*--writer = '0' + (value % 10);
		value /= 10;
	}
	*--writer = static_cast<char>('0' + value);
	if(neg) *--writer = '-';

	ptrdiff_t fillers = writer - minfill;
	if(fillers > 0) {
		// Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
		// But...: if the fill character is '0', then it's <+/-><fill><digits>
		bool add_sign_again = false;
		if(neg && dec.fill == '0') { // If filling with '0',
			++writer;  // ignore the sign we just added
			add_sign_again = true; // and re-add the sign later.
		}
		writer -= fillers;
		std::fill_n(writer, fillers, dec.fill);
		if(add_sign_again) *--writer = '-';
	}

	piece_ = absl::string_view(writer, end - writer);
}
}  // namespace substitute_internal
ABSL_NAMESPACE_END
}  // namespace absl
