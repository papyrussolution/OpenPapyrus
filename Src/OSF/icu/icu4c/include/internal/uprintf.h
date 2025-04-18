// uprintf.h
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 1998-2006, International Business Machines Corporation and others.  All Rights Reserved.
// Modification History:
// Date        Name        Description
// 11/19/98    stephen        Creation.
// 03/12/99    stephen     Modified for new C API.
//
#ifndef UPRINTF_H
#define UPRINTF_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustdio.h"
#include "ufmt_cmn.h"
#include "locbund.h"

/**
 * Struct encapsulating a single uprintf format specification.
 */
typedef struct u_printf_spec_info {
	int32_t fPrecision; /* Precision  */
	int32_t fWidth; /* Width  */
	char16_t fOrigSpec; /* Conversion specification */
	char16_t fSpec; /* Conversion specification */
	char16_t fPadChar; /* Padding character  */
	bool fAlt; /* # flag  */
	bool fSpace; /* Space flag  */
	bool fLeft; /* - flag  */
	bool fShowSign; /* + flag  */
	bool fZero; /* 0 flag  */
	bool fIsLongDouble; /* L flag  */
	bool fIsShort; /* h flag  */
	bool fIsLong; /* l flag  */
	bool fIsLongLong; /* ll flag  */
} u_printf_spec_info;

typedef int32_t U_EXPORT2 u_printf_write_stream (void   * context, const char16_t * str, int32_t count);
typedef int32_t U_EXPORT2 u_printf_pad_and_justify_stream (void   * context, const u_printf_spec_info    * info, const char16_t                 * result, int32_t resultLen);

typedef struct u_printf_stream_handler {
	u_printf_write_stream * write;
	u_printf_pad_and_justify_stream * pad_and_justify;
} u_printf_stream_handler;

/* Used by sprintf */
typedef struct u_localized_print_string {
	char16_t * str; /* Place to write the string */
	int32_t available; /* Number of codeunits available to write to */
	int32_t len; /* Maximum number of code units that can be written to output */

	ULocaleBundle fBundle; /* formatters */
} u_localized_print_string;

#define UP_PERCENT 0x0025

/**
 * Parse a single u_printf format string.
 * @param fmt A pointer to a '%' character in a u_printf format specification.
 * @param spec A pointer to a <TT>u_printf_spec</TT> to receive the parsed
 * format specifier.
 * @param locStringContext If present, will make sure that it will only write
 *          to the buffer when space is available. It's done this way because
 *          va_list sometimes can't be passed by pointer.
 * @return The number of characters contained in this specifier.
 */
U_CFUNC int32_t u_printf_parse(const u_printf_stream_handler * streamHandler,
    const char16_t * fmt,
    void   * context,
    u_localized_print_string * locStringContext,
    ULocaleBundle   * formatBundle,
    int32_t * written,
    va_list ap);

#endif /* #if !UCONFIG_NO_FORMATTING */
#endif
