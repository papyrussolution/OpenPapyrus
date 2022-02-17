// Copyright (C) 2012 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Patrick Mezard
//
#include <libphonenumber-internal.h>
#pragma hdrstop
//#include "phonenumber.pb.h"

namespace i18n {
	namespace phonenumbers {
		AreaCodeMap::AreaCodeMap() : phone_util_(*PhoneNumberUtil::GetInstance()) 
		{
		}

		AreaCodeMap::~AreaCodeMap() 
		{
		}

		void AreaCodeMap::ReadAreaCodeMap(const PrefixDescriptions* descriptions) 
		{
			DefaultMapStorage* storage = new DefaultMapStorage();
			storage->ReadFromMap(descriptions);
			storage_.reset(storage);
		}

		const char* AreaCodeMap::Lookup(const PhoneNumber& number) const 
		{
			const int entries = storage_->GetNumOfEntries();
			if(!entries) {
				return NULL;
			}
			string national_number;
			phone_util_.GetNationalSignificantNumber(number, &national_number);
			int64 phone_prefix;
			safe_strto64(SimpleItoa(number.country_code()) + national_number, &phone_prefix);
			const int* const lengths = storage_->GetPossibleLengths();
			const int lengths_size = storage_->GetPossibleLengthsSize();
			int current_index = entries - 1;
			for(int lengths_index = lengths_size - 1; lengths_index >= 0; --lengths_index) {
				const int possible_length = lengths[lengths_index];
				string phone_prefix_str = SimpleItoa(phone_prefix);
				if(static_cast<int>(phone_prefix_str.length()) > possible_length) {
					safe_strto64(phone_prefix_str.substr(0, possible_length), &phone_prefix);
				}
				current_index = BinarySearch(0, current_index, phone_prefix);
				if(current_index < 0) {
					return NULL;
				}
				const int32 current_prefix = storage_->GetPrefix(current_index);
				if(phone_prefix == current_prefix) {
					return storage_->GetDescription(current_index);
				}
			}
			return NULL;
		}
		int AreaCodeMap::BinarySearch(int start, int end, int64 value) const 
		{
			int current = 0;
			while(start <= end) {
				current = (start + end) / 2;
				int32 current_value = storage_->GetPrefix(current);
				if(current_value == value) {
					return current;
				}
				else if(current_value > value) {
					--current;
					end = current;
				}
				else {
					start = current + 1;
				}
			}
			return current;
		}
	}
}
