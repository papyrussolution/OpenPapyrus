// Copyright (C) 2011 The Libphonenumber Authors
// @license Apache License 2.0
// Author: Philippe Liard
// This file provides a minimalist implementation of common macros.
//
#ifndef I18N_PHONENUMBERS_BASE_LOGGING_H_
#define I18N_PHONENUMBERS_BASE_LOGGING_H_

#if !defined(CHECK_EQ)
	#define CHECK_EQ(X, Y) assert((X) == (Y))
#endif
#if !defined(DCHECK)
	#define DCHECK(X) assert(X)
	#define DCHECK_EQ(X, Y) CHECK_EQ((X), (Y))
	#define DCHECK_GE(X, Y) assert((X) >= (Y))
	#define DCHECK_GT(X, Y) assert((X) > (Y))
	#define DCHECK_LT(X, Y) assert((X) < (Y))
#endif

template <typename T> T* CHECK_NOTNULL(T* ptr) 
{
	assert(ptr);
	return ptr;
}

#if !defined(IGNORE_UNUSED)
	#define IGNORE_UNUSED(X) (void)(X)
#endif

#endif  // I18N_PHONENUMBERS_BASE_LOGGING_H_
