// Copyright (C) 2012 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Patrick Mezard
//
#include <libphonenumber-internal.h>
#pragma hdrstop

namespace i18n {
	namespace phonenumbers {
		DefaultMapStorage::DefaultMapStorage() 
		{
		}
		DefaultMapStorage::~DefaultMapStorage() 
		{
		}
		int32 DefaultMapStorage::GetPrefix(int index) const 
		{
			DCHECK_GE(index, 0);
			DCHECK_LT(index, prefixes_size_);
			return prefixes_[index];
		}
		const char* DefaultMapStorage::GetDescription(int index) const 
		{
			DCHECK_GE(index, 0);
			DCHECK_LT(index, prefixes_size_);
			return descriptions_[index];
		}
		void DefaultMapStorage::ReadFromMap(const PrefixDescriptions* descriptions) 
		{
			prefixes_ = descriptions->prefixes;
			prefixes_size_ = descriptions->prefixes_size;
			descriptions_ = descriptions->descriptions;
			possible_lengths_ = descriptions->possible_lengths;
			possible_lengths_size_ = descriptions->possible_lengths_size;
		}
		int DefaultMapStorage::GetNumOfEntries() const { return prefixes_size_; }
		const int* DefaultMapStorage::GetPossibleLengths() const { return possible_lengths_; }
		int DefaultMapStorage::GetPossibleLengthsSize() const { return possible_lengths_size_; }
	}
}
