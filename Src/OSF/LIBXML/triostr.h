/*************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2001 Bjorn Reese and Daniel Stenberg.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 ************************************************************************/

#ifndef TRIO_TRIOSTR_H
#define TRIO_TRIOSTR_H

//#include <assert.h>
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
#include "triodef.h"
#include "triop.h"

enum {
	TRIO_HASH_NONE = 0,
	TRIO_HASH_PLAIN,
	TRIO_HASH_TWOSIGNED
};

#if !defined(TRIO_STRING_PUBLIC)
	#if !defined(TRIO_PUBLIC)
		#define TRIO_PUBLIC
	#endif
	#define TRIO_STRING_PUBLIC TRIO_PUBLIC
#endif

/*************************************************************************
 * String functions
 */
TRIO_STRING_PUBLIC int trio_copy_max(char *target, size_t max, const char *source);
TRIO_STRING_PUBLIC char *trio_create(size_t size);
TRIO_STRING_PUBLIC void trio_destroy(char *string);
TRIO_STRING_PUBLIC char *trio_duplicate(const char *source);
TRIO_STRING_PUBLIC int trio_equal(const char *first, const char *second);
TRIO_STRING_PUBLIC int trio_equal_case(const char *first, const char *second);
TRIO_STRING_PUBLIC int trio_equal_locale(const char *first, const char *second);
TRIO_STRING_PUBLIC int trio_equal_max(const char *first, size_t max, const char *second);
TRIO_STRING_PUBLIC const char * trio_error(int);
//TRIO_STRING_PUBLIC size_t trio_length_Removed(const char *string);
TRIO_STRING_PUBLIC double trio_to_double(const char *source, char **endp);
TRIO_STRING_PUBLIC long trio_to_long(const char *source, char **endp, int base);
TRIO_STRING_PUBLIC trio_long_double_t trio_to_long_double(const char *source, char **endp);
TRIO_STRING_PUBLIC int trio_to_upper(int source);

#if !defined(TRIO_MINIMAL)
	TRIO_STRING_PUBLIC int trio_append(char *target, const char *source);
	TRIO_STRING_PUBLIC int trio_append_max(char *target, size_t max, const char *source);
	TRIO_STRING_PUBLIC int trio_contains(const char *string, const char *substring);
	TRIO_STRING_PUBLIC int trio_copy(char *target, const char *source);
	TRIO_STRING_PUBLIC char *trio_duplicate_max(const char *source, size_t max);
	TRIO_STRING_PUBLIC int trio_equal_case_max(const char *first, size_t max, const char *second);
	#if !defined(_WIN32_WCE)
		TRIO_STRING_PUBLIC size_t trio_format_date_max(char *target, size_t max, const char *format, const struct tm *datetime);
	#endif
	TRIO_STRING_PUBLIC unsigned long trio_hash(const char *string, int type);
	TRIO_STRING_PUBLIC char *trio_index(const char *string, int character);
	TRIO_STRING_PUBLIC char *trio_index_last(const char *string, int character);
	TRIO_STRING_PUBLIC int trio_lower(char *target);
	TRIO_STRING_PUBLIC int trio_match(const char *string, const char *pattern);
	TRIO_STRING_PUBLIC int trio_match_case(const char *string, const char *pattern);
	TRIO_STRING_PUBLIC size_t trio_span_function(char *target, const char *source, int (*Function)(int));
	TRIO_STRING_PUBLIC char *trio_substring(const char *string, const char *substring);
	TRIO_STRING_PUBLIC char *trio_substring_max(const char *string, size_t max, const char *substring);
	TRIO_STRING_PUBLIC float trio_to_float(const char *source, char **endp);
	TRIO_STRING_PUBLIC int trio_to_lower(int source);
	TRIO_STRING_PUBLIC unsigned long trio_to_unsigned_long(const char *source, char **endp, int base);
	TRIO_STRING_PUBLIC char *trio_tokenize(char *string, const char *delimiters);
	TRIO_STRING_PUBLIC int trio_upper(char *target);
#endif

/*************************************************************************
 * Dynamic string functions
 */
/*
 * Opaque type for dynamic strings
 */
typedef struct _trio_string_t trio_string_t;

TRIO_STRING_PUBLIC void trio_string_destroy(trio_string_t *self);
TRIO_STRING_PUBLIC char *trio_string_extract(trio_string_t *self);
TRIO_STRING_PUBLIC int trio_string_size(trio_string_t *self);
TRIO_STRING_PUBLIC void trio_string_terminate(trio_string_t *self);
TRIO_STRING_PUBLIC int trio_xstring_append_char(trio_string_t *self, char character);
TRIO_STRING_PUBLIC trio_string_t *trio_xstring_duplicate(const char *other);

#if !defined(TRIO_MINIMAL)
	TRIO_STRING_PUBLIC trio_string_t *trio_string_create(int initial_size);
	TRIO_STRING_PUBLIC char *trio_string_get(trio_string_t *self, int offset);
	TRIO_STRING_PUBLIC void trio_xstring_set(trio_string_t *self, char *buffer);
	TRIO_STRING_PUBLIC int trio_string_append(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_contains(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_copy(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC trio_string_t *trio_string_duplicate(trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_equal(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_equal_max(trio_string_t *self, size_t max, trio_string_t *second);
	TRIO_STRING_PUBLIC int trio_string_equal_case(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_equal_case_max(trio_string_t *self, size_t max, trio_string_t *other);
	#if !defined(_WIN32_WCE)
		TRIO_STRING_PUBLIC size_t trio_string_format_date_max(trio_string_t *self, size_t max, const char *format, const struct tm *datetime);
	#endif
	TRIO_STRING_PUBLIC char *trio_string_index(trio_string_t *self, int character);
	TRIO_STRING_PUBLIC char *trio_string_index_last(trio_string_t *self, int character);
	TRIO_STRING_PUBLIC int trio_string_length(trio_string_t *self);
	TRIO_STRING_PUBLIC int trio_string_lower(trio_string_t *self);
	TRIO_STRING_PUBLIC int trio_string_match(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_match_case(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC char *trio_string_substring(trio_string_t *self, trio_string_t *other);
	TRIO_STRING_PUBLIC int trio_string_upper(trio_string_t *self);
	TRIO_STRING_PUBLIC int trio_xstring_append(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_contains(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_copy(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_equal(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_equal_max(trio_string_t *self, size_t max, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_equal_case(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_equal_case_max(trio_string_t *self, size_t max, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_match(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC int trio_xstring_match_case(trio_string_t *self, const char *other);
	TRIO_STRING_PUBLIC char *trio_xstring_substring(trio_string_t *self, const char *other);
#endif

#endif /* TRIO_TRIOSTR_H */
