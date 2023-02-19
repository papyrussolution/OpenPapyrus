// Copyright (C) 2011 The Libphonenumber Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Author: Philippe Liard

#ifndef I18N_PHONENUMBERS_REGEXP_ADAPTER_FACTORY_H_
#define I18N_PHONENUMBERS_REGEXP_ADAPTER_FACTORY_H_

// This file selects the right implementation of the abstract regexp factory at
// compile time depending on the compilation flags (I18N_PHONENUMBERS_USE_RE2).
// The default abstract regexp factory implementation can be obtained using the
// type RegExpFactory. This will be set to RE2RegExpFactory if RE2 is used or
// ICURegExpFactory otherwise.

#ifdef I18N_PHONENUMBERS_USE_RE2
	#include "regexp_adapter_re2.h"
#else
	#include "regexp_adapter_icu.h"
#endif  // I18N_PHONENUMBERS_USE_RE2

namespace i18n {
namespace phonenumbers {

#ifdef I18N_PHONENUMBERS_USE_RE2
typedef RE2RegExpFactory RegExpFactory;
#else
typedef ICURegExpFactory RegExpFactory;
#endif  // I18N_PHONENUMBERS_USE_RE2

}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_REGEXP_ADAPTER_FACTORY_H_
