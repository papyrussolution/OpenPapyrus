// Copyright (C) 2020 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_UNSAFE_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_UNSAFE_H_

#include "logging.h"
#include "thread_checker.h"

// Dummy lock implementation on non-POSIX platforms. If you are running on a
// different platform and care about thread-safety, please compile with
// -DI18N_PHONENUMBERS_USE_BOOST.
namespace i18n {
namespace phonenumbers {
class Lock {
public:
	Lock() 
	{
	}
	void Acquire() const 
	{
		DCHECK(thread_checker_.CalledOnValidThread());
		IGNORE_UNUSED(thread_checker_);
	}
	void Release() const 
	{
		DCHECK(thread_checker_.CalledOnValidThread());
		IGNORE_UNUSED(thread_checker_);
	}
private:
	DISALLOW_COPY_AND_ASSIGN(Lock);
	const ThreadChecker thread_checker_;
};
}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_UNSAFE_H_
