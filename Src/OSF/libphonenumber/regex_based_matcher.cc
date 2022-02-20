// Copyright (C) 2014 The Libphonenumber Authors
// @license Apache License 2.0
//
#include <libphonenumber-internal.h>
#pragma hdrstop
#include "phonemetadata.pb.h"

namespace i18n {
	namespace phonenumbers {
		// Same implementations of AbstractRegExpFactory and RegExpCache in
		// PhoneNumberUtil (copy from phonenumberutil.cc).
		RegexBasedMatcher::RegexBasedMatcher() : regexp_factory_(new RegExpFactory()), regexp_cache_(new RegExpCache(*regexp_factory_, 128))
		{
		}
		RegexBasedMatcher::~RegexBasedMatcher() 
		{
		}
		bool RegexBasedMatcher::MatchNationalNumber(const string & number, const PhoneNumberDesc& number_desc, bool allow_prefix_match) const 
		{
			const string & national_number_pattern = number_desc.national_number_pattern();
			// We don't want to consider it a prefix match when matching non-empty input
			// against an empty pattern.
			if(national_number_pattern.empty()) {
				return false;
			}
			return Match(number, national_number_pattern, allow_prefix_match);
		}

		bool RegexBasedMatcher::Match(const string & number, const string & number_pattern, bool allow_prefix_match) const 
		{
			const RegExp& regexp(regexp_cache_->GetRegExp(number_pattern));
			if(regexp.FullMatch(number)) {
				return true;
			}
			const scoped_ptr<RegExpInput> normalized_number_input(regexp_factory_->CreateInput(number));
			return regexp.Consume(normalized_number_input.get()) ? allow_prefix_match : false;
		}
	}  // namespace phonenumbers
}  // namespace i18n
