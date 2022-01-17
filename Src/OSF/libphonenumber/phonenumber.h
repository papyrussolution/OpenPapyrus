// Copyright (C) 2009 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_PHONENUMBER_H_
#define I18N_PHONENUMBERS_PHONENUMBER_H_

// Helper functions dealing with PhoneNumber and PhoneNumberDesc comparisons.

namespace i18n {
	namespace phonenumbers {
		class PhoneNumber;
		class PhoneNumberDesc;
		// Compares two phone numbers.
		bool ExactlySameAs(const PhoneNumber& first_number, const PhoneNumber& second_number);
		// Compares two phone number descriptions.
		bool ExactlySameAs(const PhoneNumberDesc& first_number_desc, const PhoneNumberDesc& second_number_desc);
	}
}

#endif  // I18N_PHONENUMBERS_PHONENUMBER_H_
