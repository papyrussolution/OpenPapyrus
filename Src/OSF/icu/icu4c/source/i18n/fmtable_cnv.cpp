// FMTABLE.CPP
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1997-2010, International Business Machines Corporation and others. All Rights Reserved.
// Modification History:
// Date        Name        Description
// 03/25/97    clhuang     Initial Implementation.
// 
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_CONVERSION

#include "unicode/fmtable.h"
//
// class Formattable
//
U_NAMESPACE_BEGIN
	// -------------------------------------
	// Creates a formattable object with a char * string.
	// This API is useless. The API that takes a UnicodeString is actually just as good.
	Formattable::Formattable(const char * stringToCopy)
	{
		init();
		fType = kString;
		fValue.fString = new UnicodeString(stringToCopy);
	}
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING || !UCONFIG_NO_CONVERSION */
