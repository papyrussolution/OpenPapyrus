// Copyright (C) 2011 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Tao Huang
//
// Implementation of a mutable match of a phone number within a piece of
// text. Matches may be found using PhoneNumberUtil::FindNumbers.
//
#include <libphonenumber-internal.h>
#pragma hdrstop
//#include "phonenumber.pb.h"

namespace i18n {
	namespace phonenumbers {
		PhoneNumberMatch::PhoneNumberMatch(int start, const string & raw_string, const PhoneNumber& number) : start_(start), raw_string_(raw_string), number_(number) 
		{
		}

		PhoneNumberMatch::PhoneNumberMatch() : start_(-1), raw_string_(""), number_(PhoneNumber::default_instance()) 
		{
		}

		const PhoneNumber& PhoneNumberMatch::number() const { return number_; }
		int PhoneNumberMatch::start() const { return start_; }
		int PhoneNumberMatch::end() const { return static_cast<int>(start_ + raw_string_.length()); }
		int PhoneNumberMatch::length() const { return static_cast<int>(raw_string_.length()); }
		const string & PhoneNumberMatch::raw_string() const { return raw_string_; }
		void PhoneNumberMatch::set_start(int start) { start_ = start; }
		void PhoneNumberMatch::set_raw_string(const string & raw_string) { raw_string_ = raw_string; }
		void PhoneNumberMatch::set_number(const PhoneNumber& number) { number_.CopyFrom(number); }
		string PhoneNumberMatch::ToString() const { return StrCat("PhoneNumberMatch [", start(), ",", end(), ") ", raw_string_.c_str()); }
		bool PhoneNumberMatch::Equals(const PhoneNumberMatch& match) const 
		{
			return ExactlySameAs(match.number_, number_) && match.raw_string_.compare(raw_string_) == 0 && match.start_ == start_;
		}

		void PhoneNumberMatch::CopyFrom(const PhoneNumberMatch& match) 
		{
			raw_string_ = match.raw_string();
			start_ = match.start();
			number_ = match.number();
		}
	}  // namespace phonenumbers
}  // namespace i18n
