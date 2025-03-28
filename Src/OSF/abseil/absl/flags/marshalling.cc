//
// Copyright 2019 The Abseil Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
//
#include "absl/absl-internal.h"
#pragma hdrstop

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace flags_internal {
//
// AbslParseFlag specializations for boolean type.
//
bool AbslParseFlag(absl::string_view text, bool* dst, std::string*) 
{
	const char* kTrue[] = {"1", "t", "true", "y", "yes"};
	const char* kFalse[] = {"0", "f", "false", "n", "no"};
	static_assert(sizeof(kTrue) == sizeof(kFalse), "true_false_equal");
	text = absl::StripAsciiWhitespace(text);
	for(size_t i = 0; i < ABSL_ARRAYSIZE(kTrue); ++i) {
		if(absl::EqualsIgnoreCase(text, kTrue[i])) {
			*dst = true;
			return true;
		}
		else if(absl::EqualsIgnoreCase(text, kFalse[i])) {
			*dst = false;
			return true;
		}
	}
	return false; // didn't match a legal input
}

// --------------------------------------------------------------------
// AbslParseFlag for integral types.

// Return the base to use for parsing text as an integer.  Leading 0x
// puts us in base 16.  But leading 0 does not put us in base 8. It
// caused too many bugs when we had that behavior.
static int NumericBase(absl::string_view text) 
{
	const bool hex = (text.size() >= 2 && text[0] == '0' && oneof2(text[1], 'x', 'X'));
	return hex ? 16 : 10;
}

template <typename IntType> inline bool ParseFlagImpl(absl::string_view text, IntType& dst) 
{
	text = absl::StripAsciiWhitespace(text);
	return absl::numbers_internal::safe_strtoi_base(text, &dst, NumericBase(text));
}

bool AbslParseFlag(absl::string_view text, short* dst, std::string*) 
{
	int val;
	if(!ParseFlagImpl(text, val)) return false;
	if(static_cast<short>(val) != val) // worked, but number out of range
		return false;
	*dst = static_cast<short>(val);
	return true;
}

bool AbslParseFlag(absl::string_view text, unsigned short* dst, std::string*) 
{
	unsigned int val;
	if(!ParseFlagImpl(text, val)) return false;
	if(static_cast<unsigned short>(val) != val) // worked, but number out of range
		return false;
	*dst = static_cast<unsigned short>(val);
	return true;
}

bool AbslParseFlag(absl::string_view text, int* dst, std::string*) { return ParseFlagImpl(text, *dst); }
bool AbslParseFlag(absl::string_view text, unsigned int* dst, std::string*) { return ParseFlagImpl(text, *dst); }
bool AbslParseFlag(absl::string_view text, long* dst, std::string*) { return ParseFlagImpl(text, *dst); }
bool AbslParseFlag(absl::string_view text, unsigned long* dst, std::string*) { return ParseFlagImpl(text, *dst); }
bool AbslParseFlag(absl::string_view text, long long* dst, std::string*) { return ParseFlagImpl(text, *dst); }
bool AbslParseFlag(absl::string_view text, unsigned long long* dst, std::string*) { return ParseFlagImpl(text, *dst); }
//
// AbslParseFlag for floating point types.
//
bool AbslParseFlag(absl::string_view text, float* dst, std::string*) { return absl::SimpleAtof(text, dst); }
bool AbslParseFlag(absl::string_view text, double* dst, std::string*) { return absl::SimpleAtod(text, dst); }
//
// AbslParseFlag for strings.
//
bool AbslParseFlag(absl::string_view text, std::string* dst, std::string*) 
{ 
	dst->assign(text.data(), text.size());
	return true;
}
//
// AbslParseFlag for vector of strings.
//
bool AbslParseFlag(absl::string_view text, std::vector <std::string>* dst, std::string*) 
{
	// An empty flag value corresponds to an empty vector, not a vector
	// with a single, empty std::string.
	if(text.empty()) {
		dst->clear();
		return true;
	}
	*dst = absl::StrSplit(text, ',', absl::AllowEmpty());
	return true;
}
//
// AbslUnparseFlag specializations for various builtin flag types.
//
std::string Unparse(bool v) { return STextConst::GetBool(v); }
std::string Unparse(short v) { return absl::StrCat(v); }
std::string Unparse(unsigned short v) { return absl::StrCat(v); }
std::string Unparse(int v) { return absl::StrCat(v); }
std::string Unparse(unsigned int v) { return absl::StrCat(v); }
std::string Unparse(long v) { return absl::StrCat(v); }
std::string Unparse(unsigned long v) { return absl::StrCat(v); }
std::string Unparse(long long v) { return absl::StrCat(v); }
std::string Unparse(unsigned long long v) { return absl::StrCat(v); }

template <typename T> std::string UnparseFloatingPointVal(T v) 
{
	// digits10 is guaranteed to roundtrip correctly in string -> value -> string
	// conversions, but may not be enough to represent all the values correctly.
	std::string digit10_str = absl::StrFormat("%.*g", std::numeric_limits<T>::digits10, v);
	if(std::isnan(v) || std::isinf(v)) 
		return digit10_str;
	T roundtrip_val = 0;
	std::string err;
	if(absl::ParseFlag(digit10_str, &roundtrip_val, &err) && roundtrip_val == v) {
		return digit10_str;
	}
	// max_digits10 is the number of base-10 digits that are necessary to uniquely
	// represent all distinct values.
	return absl::StrFormat("%.*g", std::numeric_limits<T>::max_digits10, v);
}

std::string Unparse(float v) { return UnparseFloatingPointVal(v); }
std::string Unparse(double v) { return UnparseFloatingPointVal(v); }
std::string AbslUnparseFlag(absl::string_view v) { return std::string(v); }
std::string AbslUnparseFlag(const std::vector <std::string>& v) { return absl::StrJoin(v, ","); }
}  // namespace flags_internal

bool AbslParseFlag(absl::string_view text, absl::LogSeverity* dst, std::string* err) 
{
	text = absl::StripAsciiWhitespace(text);
	if(text.empty()) {
		*err = "no value provided";
		return false;
	}
	if(text.front() == 'k' || text.front() == 'K') text.remove_prefix(1);
	if(absl::EqualsIgnoreCase(text, "info")) {
		*dst = absl::LogSeverity::kInfo;
		return true;
	}
	if(absl::EqualsIgnoreCase(text, "warning")) {
		*dst = absl::LogSeverity::kWarning;
		return true;
	}
	if(absl::EqualsIgnoreCase(text, "error")) {
		*dst = absl::LogSeverity::kError;
		return true;
	}
	if(absl::EqualsIgnoreCase(text, "fatal")) {
		*dst = absl::LogSeverity::kFatal;
		return true;
	}
	std::underlying_type<absl::LogSeverity>::type numeric_value;
	if(absl::ParseFlag(text, &numeric_value, err)) {
		*dst = static_cast<absl::LogSeverity>(numeric_value);
		return true;
	}
	*err = "only integers and absl::LogSeverity enumerators are accepted";
	return false;
}

std::string AbslUnparseFlag(absl::LogSeverity v) 
{
	if(v == absl::NormalizeLogSeverity(v)) 
		return absl::LogSeverityName(v);
	return absl::UnparseFlag(static_cast<int>(v));
}

ABSL_NAMESPACE_END
}  // namespace absl
