// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *******************************************************************************
 *
 *   Copyright (C) 1998-2011, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *
 * File error.c
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   05/28/99    stephen     Creation.
 *******************************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop
#include "errmsg.h"
#include "toolutil.h"

U_CFUNC void error(uint32_t linenumber, const char * msg, ...)
{
	va_list va;
	va_start(va, msg);
	fprintf(stderr, "%s:%u: ", gCurrentFileName, (int)linenumber);
	vfprintf(stderr, msg, va);
	fprintf(stderr, "\n");
	va_end(va);
}

static bool gShowWarning = TRUE;

U_CFUNC void setShowWarning(bool val)
{
	gShowWarning = val;
}

U_CFUNC bool getShowWarning() { return gShowWarning; }

static bool gStrict = FALSE;
U_CFUNC bool isStrict() { return gStrict; }
U_CFUNC void setStrict(bool val) { gStrict = val; }

static bool gVerbose = FALSE;
U_CFUNC bool isVerbose() { return gVerbose; }
U_CFUNC void setVerbose(bool val) { gVerbose = val; }

U_CFUNC void warning(uint32_t linenumber, const char * msg, ...)
{
	if(gShowWarning) {
		va_list va;
		va_start(va, msg);
		fprintf(stderr, "%s:%u: warning: ", gCurrentFileName, (int)linenumber);
		vfprintf(stderr, msg, va);
		fprintf(stderr, "\n");
		va_end(va);
	}
}
