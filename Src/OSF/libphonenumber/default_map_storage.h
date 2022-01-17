// Copyright (C) 2012 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Patrick Mezard
// Default class for storing area codes.
//
#ifndef I18N_PHONENUMBERS_DEFAULT_MAP_STORAGE_H_
#define I18N_PHONENUMBERS_DEFAULT_MAP_STORAGE_H_

namespace i18n {
	namespace phonenumbers {
		struct PrefixDescriptions;

		// Default area code map storage strategy that is used for data not
		// containing description duplications. It is mainly intended to avoid
		// the overhead of the string table management when it is actually
		// unnecessary (i.e no string duplication).
		class DefaultMapStorage {
		public:
			DefaultMapStorage();
			virtual ~DefaultMapStorage();
			// Returns the phone number prefix located at the provided index.
			int32 GetPrefix(int index) const;
			// Gets the description corresponding to the phone number prefix located
			// at the provided index. If the description is not available in the current
			// language an empty string is returned.
			const char* GetDescription(int index) const;
			// Sets the internal state of the underlying storage implementation from the
			// provided area_codes that maps phone number prefixes to description strings.
			void ReadFromMap(const PrefixDescriptions* descriptions);
			// Returns the number of entries contained in the area code map.
			int GetNumOfEntries() const;
			// Returns an array containing the possible lengths of prefixes sorted in
			// ascending order.
			const int* GetPossibleLengths() const;
			// Returns the number of elements in GetPossibleLengths() array.
			int GetPossibleLengthsSize() const;
		private:
			// Sorted sequence of phone number prefixes.
			const int32* prefixes_;
			int prefixes_size_;
			// Sequence of prefix descriptions, in the same order than prefixes_.
			const char** descriptions_;
			// Sequence of unique possible lengths in ascending order.
			const int32* possible_lengths_;
			int possible_lengths_size_;

			DISALLOW_COPY_AND_ASSIGN(DefaultMapStorage);
		};
	}
}

#endif /* I18N_PHONENUMBERS_DEFAULT_MAP_STORAGE_H_ */
