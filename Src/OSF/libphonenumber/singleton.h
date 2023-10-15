// SINGLETON.H
// Copyright (C) 2011 The Libphonenumber Authors
// Author: Philippe Liard
// @license Apache License 2.0
//
// @sobolev: Have united singleton_boost.h singleton_posix.h singleton_stdmutex.h singleton_unsafe.h singleton_win32.h in one file
//
#ifndef I18N_PHONENUMBERS_BASE_MEMORY_SINGLETON_H_
#define I18N_PHONENUMBERS_BASE_MEMORY_SINGLETON_H_

#if defined(I18N_PHONENUMBERS_USE_BOOST)
	//#include "singleton_boost.h"
	#include <boost/scoped_ptr.hpp>
	#include <boost/thread/once.hpp>
	#include <boost/utility.hpp>

	namespace i18n {
		namespace phonenumbers {
			template <class T> class Singleton : private boost::noncopyable {
			public:
				Singleton() 
				{
				}
				virtual ~Singleton() 
				{
				}
				static T * GetInstance() 
				{
					boost::call_once(Init, flag_);
					return instance_.get();
				}
			private:
				static void Init() 
				{
					instance_.reset(new T());
				}
				static boost::scoped_ptr<T> instance_;
				static boost::once_flag flag_;
			};

			template <class T> boost::scoped_ptr<T> Singleton<T>::instance_;
			template <class T> boost::once_flag Singleton<T>::flag_ = BOOST_ONCE_INIT;
		}
	}
#elif (__cplusplus >= 201103L) && defined(I18N_PHONENUMBERS_USE_STDMUTEX)
	// C++11 Lock implementation based on std::mutex.
	//#include "singleton_stdmutex.h"
	#include <mutex>
	#include "basictypes.h"

	namespace i18n {
		namespace phonenumbers {
			template <class T> class Singleton {
			public:
				Singleton() 
				{
				}
				virtual ~Singleton() 
				{
				}
				static T* GetInstance() 
				{
					if(once_init_) {
						singleton_mutex_.lock();
						if(once_init_) {
							Init();
							once_init_ = false;
						}
						singleton_mutex_.unlock();
					}
					return instance_;
				}
			private:
				DISALLOW_COPY_AND_ASSIGN(Singleton);

				static void Init() 
				{
					instance_ = new T();
				}
				static T* instance_; // Leaky singleton.
				static std::mutex singleton_mutex_;
				static bool once_init_;
			};

			template <class T> T* Singleton<T>::instance_;
			template <class T> std::mutex Singleton<T>::singleton_mutex_;
			template <class T> bool Singleton<T>::once_init_ = true;
		}
	}
#elif defined(__linux__) || defined(__APPLE__) || defined(I18N_PHONENUMBERS_HAVE_POSIX_THREAD)
	//#include "singleton_posix.h"
	#include <pthread.h>
	#include "logging.h"

	namespace i18n {
		namespace phonenumbers {
			template <class T> class Singleton {
			public:
				virtual ~Singleton() 
				{
				}
				static T* GetInstance() 
				{
					const int ret = pthread_once(&once_control_, &Init);
					(void)ret;
					DCHECK_EQ(0, ret);
					return instance_;
				}
			private:
				static void Init() 
				{
					instance_ = new T();
				}
				static T* instance_; // Leaky singleton.
				static pthread_once_t once_control_;
			};

			template <class T> T* Singleton<T>::instance_;
			template <class T> pthread_once_t Singleton<T>::once_control_ = PTHREAD_ONCE_INIT;
		}
	}
#elif defined(WIN32)
	//#include "singleton_win32.h"
	#include <windows.h>
	#include <synchapi.h>
	#include "basictypes.h"

	namespace i18n {
		namespace phonenumbers {
			template <class T> class Singleton {
			public:
				Singleton() 
				{
				}
				virtual ~Singleton() 
				{
				}
				static T* GetInstance() 
				{
					if(once_init_) {
						EnterCriticalSection(&critical_section_);
						if(once_init_) {
							Init();
							once_init_ = false;
						}
						LeaveCriticalSection(&critical_section_);
					}
					return instance_;
				}
			private:
				static void Init() 
				{
					instance_ = new T();
				}
				static T* instance_; // Leaky singleton.
				static CRITICAL_SECTION critical_section_;
				static bool once_init_;
			};

			static bool perform_init_crit(CRITICAL_SECTION& cs)
			{
				InitializeCriticalSection(&cs);
				return true;
			}
			template <class T> T* Singleton<T>::instance_;
			template <class T> CRITICAL_SECTION Singleton<T>::critical_section_;
			template <class T> bool Singleton<T>::once_init_ = perform_init_crit(Singleton<T>::critical_section_);
		}
	}
#else
	//#include "singleton_unsafe.h"
	#include "logging.h"
	#include "thread_checker.h"

	namespace i18n {
		namespace phonenumbers {
			// Note that this implementation is not thread-safe. For a thread-safe
			// implementation on non-POSIX platforms, please compile with
			// -DI18N_PHONENUMBERS_USE_BOOST.
			template <class T> class Singleton {
			public:
				Singleton() : thread_checker_() 
				{
				}
				virtual ~Singleton() 
				{
				}
				static T* GetInstance() 
				{
					static T * instance = NULL;
					if(!instance)
						instance = new T();
					DCHECK(instance->thread_checker_.CalledOnValidThread());
					return instance;
				}
			private:
				const ThreadChecker thread_checker_;
			};
		}
	}
#endif  // !I18N_PHONENUMBERS_USE_BOOST
#endif  // I18N_PHONENUMBERS_BASE_MEMORY_SINGLETON_H_
