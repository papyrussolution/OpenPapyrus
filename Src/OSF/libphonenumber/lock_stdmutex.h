// Copyright (C) 2020 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_STDMUTEX_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_STDMUTEX_H_

#include <mutex>
#include "basictypes.h"

namespace i18n {
	namespace phonenumbers {
		class Lock {
		public:
			Lock() = default;
			void Acquire() const { mutex_.lock(); }
			void Release() const { mutex_.unlock(); }
		private:
			DISALLOW_COPY_AND_ASSIGN(Lock);
			mutable std::mutex mutex_;
		};
	}
}

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_STDMUTEX_H_
