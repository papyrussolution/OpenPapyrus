// LOCK.H
// Copyright (C) 2011 The Libphonenumber Authors
// Author: Philippe Liard
// @licence Apache License 2.0
//
// @sobolev: Have united lock_boost.h lock_posix.h lock_stdmutex.h lock_unsafe.h lock_win32.h in one file
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_

#include "basictypes.h"

#if defined(I18N_PHONENUMBERS_USE_BOOST)
	//#include "synchronization/lock_boost.h"
	#include <boost/thread/mutex.hpp>

	namespace i18n {
		namespace phonenumbers {
			typedef boost::mutex Lock;
			typedef boost::mutex::scoped_lock AutoLock;
		}
	}
#elif (__cplusplus >= 201103L) && defined(I18N_PHONENUMBERS_USE_STDMUTEX)
	// C++11 Lock implementation based on std::mutex.
	//#include "synchronization/lock_stdmutex.h"
	#include <mutex>

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
#elif defined(__linux__) || defined(__APPLE__) || defined(I18N_PHONENUMBERS_HAVE_POSIX_THREAD)
	//#include "lock_posix.h"
	#include <pthread.h>
	#include "logging.h"

	namespace i18n {
		namespace phonenumbers {
			class Lock {
			public:
				Lock() 
				{
					const int ret = pthread_mutex_init(&mutex_, NULL);
					(void)ret;
					DCHECK_EQ(0, ret);
				}
				~Lock() 
				{
					const int ret = pthread_mutex_destroy(&mutex_);
					(void)ret;
					DCHECK_EQ(0, ret);
				}
				void Acquire() const 
				{
					int ret = pthread_mutex_lock(&mutex_);
					(void)ret;
					DCHECK_EQ(0, ret);
				}
				void Release() const 
				{
					int ret = pthread_mutex_unlock(&mutex_);
					(void)ret;
					DCHECK_EQ(0, ret);
				}
			private:
				DISALLOW_COPY_AND_ASSIGN(Lock);
				mutable pthread_mutex_t mutex_;
			};
		}
	}
#elif defined(WIN32)
	//#include "lock_win32.h"
	#include <windows.h>
	#include <synchapi.h>

	namespace i18n {
		namespace phonenumbers {
			class Lock {
			public:
				Lock() 
				{
					InitializeCriticalSection(&cs_);
				}
				~Lock() 
				{
					DeleteCriticalSection(&cs_);
				}
				void Acquire() 
				{
					EnterCriticalSection(&cs_);
				}
				void Release() 
				{
					LeaveCriticalSection(&cs_);
				}
			private:
				DISALLOW_COPY_AND_ASSIGN(Lock);
				CRITICAL_SECTION cs_;
			};
		}
	}
#else
	//#include "lock_unsafe.h"
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
		}
	}
#endif
// lock_boost.h comes with its own AutoLock.
#if !defined(I18N_PHONENUMBERS_USE_BOOST)
	namespace i18n {
		namespace phonenumbers {
			class AutoLock {
			public:
				AutoLock(Lock& lock) : lock_(lock) 
				{
					lock_.Acquire();
				}
				~AutoLock() 
				{
					lock_.Release();
				}
			private:
				Lock & lock_;
			};
		}  // namespace phonenumbers
	}  // namespace i18n
#endif  // !I18N_PHONENUMBERS_USE_BOOST
#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_H_
