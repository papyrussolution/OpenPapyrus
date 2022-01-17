// Copyright (C) 2020 The Libphonenumber Authors
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: Philippe Liard
//
#ifndef I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_BOOST_H_
#define I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_BOOST_H_

#include <boost/thread/mutex.hpp>

namespace i18n {
namespace phonenumbers {

typedef boost::mutex Lock;
typedef boost::mutex::scoped_lock AutoLock;

}  // namespace phonenumbers
}  // namespace i18n

#endif  // I18N_PHONENUMBERS_BASE_SYNCHRONIZATION_LOCK_BOOST_H_
