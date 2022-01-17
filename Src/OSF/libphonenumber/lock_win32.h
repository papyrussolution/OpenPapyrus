// Copyright (C) 2020 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_WINDOWS_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_WINDOWS_H_

#include <windows.h>
#include <synchapi.h>
#include "basictypes.h"

namespace i18n {
namespace phonenumbers {
class Lock {
public:
	Lock() {
		InitializeCriticalSection(&cs_);
	}

	~Lock() {
		DeleteCriticalSection(&cs_);
	}

	void Acquire() {
		EnterCriticalSection(&cs_);
	}

	void Release() {
		LeaveCriticalSection(&cs_);
	}

private:
	DISALLOW_COPY_AND_ASSIGN(Lock);
	CRITICAL_SECTION cs_;
};
}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_POSIX_H_
