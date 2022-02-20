// Copyright (C) 2011 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
namespace i18n {
	namespace phonenumbers {
		struct NormalizeUTF8 {
			// Put a UTF-8 string in ASCII digits: All decimal digits (Nd) replaced by
			// their ASCII counterparts; all other characters are copied from input to
			// output.
			static string NormalizeDecimalDigits(const string & number) 
			{
				string normalized;
				UnicodeText number_as_unicode;
				number_as_unicode.PointToUTF8(number.data(), static_cast<int>(number.size()));
				for(UnicodeText::const_iterator it = number_as_unicode.begin(); it != number_as_unicode.end(); ++it) {
					int32_t digitValue = u_charDigitValue(*it);
					if(digitValue == -1) {
						// Not a decimal digit.
						char utf8[4];
						int len = it.get_utf8(utf8);
						normalized.append(utf8, len);
					}
					else {
						normalized.push_back('0' + digitValue);
					}
				}
				return normalized;
			}
		};
	}
}
