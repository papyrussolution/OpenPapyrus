// ucln_io.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2001-2014, International Business Machines Corporation and others. All Rights Reserved.
// encoding:   UTF-8
// created on: 2006August11
// created by: George Rhoten
// 
#include <icu-internal.h>
#pragma hdrstop
#include "ucln.h"
#include "ucln_io.h"

#ifndef U_IO_IMPLEMENTATION
#error U_IO_IMPLEMENTATION not set - must be set for all ICU source files in io/ - see https://unicode-org.github.io/icu/userguide/howtouseicu
#endif

/**  Auto-client */
#define UCLN_TYPE UCLN_IO
#include "ucln_imp.h"

/* Leave this copyright notice here! It needs to go somewhere in this library. */
static const char copyright[] = U_COPYRIGHT_STRING;
static cleanupFunc * gCleanupFunctions[UCLN_IO_COUNT];

static bool U_CALLCONV io_cleanup(void)
{
	int32_t libType = UCLN_IO_START;
	(void)copyright; // Suppress unused variable warning.
	while(++libType<UCLN_IO_COUNT) {
		if(gCleanupFunctions[libType]) {
			gCleanupFunctions[libType]();
			gCleanupFunctions[libType] = NULL;
		}
	}
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
	ucln_unRegisterAutomaticCleanup();
#endif
	return TRUE;
}

void ucln_io_registerCleanup(ECleanupIOType type, cleanupFunc * func) 
{
	assert(UCLN_IO_START < type && type < UCLN_IO_COUNT);
	{
		icu::Mutex m; // See ticket 10295 for discussion.
		ucln_registerCleanup(UCLN_IO, io_cleanup);
		if(UCLN_IO_START < type && type < UCLN_IO_COUNT) {
			gCleanupFunctions[type] = func;
		}
	}
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
	ucln_registerAutomaticCleanup();
#endif
}
