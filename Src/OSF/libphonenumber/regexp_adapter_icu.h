// Copyright (C) 2011 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_REGEXP_ADAPTER_ICU_H_
#define I18N_PHONENUMBERS_REGEXP_ADAPTER_ICU_H_

#include <string>
#include "regexp_adapter.h"

namespace i18n {
	namespace phonenumbers {
		// ICU regexp factory that lets the user instantiate the underlying
		// implementation of RegExp and RegExpInput classes based on the ICU regexp
		// engine.
		class ICURegExpFactory : public AbstractRegExpFactory {
		public:
			virtual ~ICURegExpFactory() 
			{
			}
			virtual RegExpInput* CreateInput(const string & utf8_input) const;
			virtual RegExp* CreateRegExp(const string & utf8_regexp) const;
		};
	}
}

#endif  // I18N_PHONENUMBERS_REGEXP_ADAPTER_ICU_H_
