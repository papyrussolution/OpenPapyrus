// Copyright (C) 2011 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_

#if defined(I18N_PHONENUMBERS_USE_BOOST)
	#include "synchronization/lock_boost.h"
#elif (__cplusplus >= 201103L) && defined(I18N_PHONENUMBERS_USE_STDMUTEX)
// C++11 Lock implementation based on std::mutex.
	#include "synchronization/lock_stdmutex.h"
#elif defined(__linux__) || defined(__APPLE__) || defined(I18N_PHONENUMBERS_HAVE_POSIX_THREAD)
	#include "lock_posix.h"
#elif defined(WIN32)
	#include "lock_win32.h"
#else
	#include "lock_unsafe.h"
#endif

// lock_boost.h comes with its own AutoLock.
#if !defined(I18N_PHONENUMBERS_USE_BOOST)
namespace i18n {
namespace phonenumbers {
class AutoLock {
public:
	AutoLock(Lock& lock) : lock_(lock) {
		lock_.Acquire();
	}

	~AutoLock() {
		lock_.Release();
	}

private:
	Lock& lock_;
};
}  // namespace phonenumbers
}  // namespace i18n
#endif  // !I18N_PHONENUMBERS_USE_BOOST

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_
