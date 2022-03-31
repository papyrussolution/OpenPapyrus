// Copyright (C) 2011 The Libphonenumber Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
#ifndef I18N_PHONENUMBERS_REGION_CODE_H_
#define I18N_PHONENUMBERS_REGION_CODE_H_

namespace i18n {
	namespace phonenumbers {
		using std::string;

		class RegionCode {
		public:
			// Returns a region code string representing the "unknown" region.
			static const char* GetUnknown() { return ZZ(); }
			static const char* ZZ() { return "ZZ"; }
		};
	}
}

#endif  // I18N_PHONENUMBERS_REGION_CODE_H_
