// Copyright (C) 2013 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_POSIX_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_POSIX_H_

#include <pthread.h>
#include "basictypes.h"
#include "logging.h"

namespace i18n {
namespace phonenumbers {
class Lock {
public:
	Lock() {
		const int ret = pthread_mutex_init(&mutex_, NULL);
		(void)ret;
		DCHECK_EQ(0, ret);
	}

	~Lock() {
		const int ret = pthread_mutex_destroy(&mutex_);
		(void)ret;
		DCHECK_EQ(0, ret);
	}

	void Acquire() const {
		int ret = pthread_mutex_lock(&mutex_);
		(void)ret;
		DCHECK_EQ(0, ret);
	}

	void Release() const {
		int ret = pthread_mutex_unlock(&mutex_);
		(void)ret;
		DCHECK_EQ(0, ret);
	}

private:
	DISALLOW_COPY_AND_ASSIGN(Lock);

	mutable pthread_mutex_t mutex_;
};
}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_POSIX_H_
