/*-
 * Copyright (c) 2003-2010 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * $FreeBSD: head/lib/libarchive/archive_string.h 201092 2009-12-28 02:26:06Z kientzle $
 */
#ifndef ARCHIVE_STRING_H_INCLUDED
#define ARCHIVE_STRING_H_INCLUDED
#ifndef __LIBARCHIVE_BUILD
	#ifndef __LIBARCHIVE_TEST
		#error This header is only to be used internally to libarchive.
	#endif
#endif
#include "archive.h"
/*
 * Basic resizable/reusable string support similar to Java's "StringBuffer."
 *
 * Unlike sbuf(9), the buffers here are fully reusable and track the
 * length throughout.
 */
struct archive_string {
	char * s; // Pointer to the storage
	size_t length; // Length of 's' in characters 
	size_t buffer_length; // Length of malloc-ed storage in bytes
};

struct archive_wstring {
	wchar_t * s; // Pointer to the storage 
	size_t length; // Length of 's' in characters 
	size_t buffer_length; // Length of malloc-ed storage in bytes.
};

struct archive_string_conv;

/* Initialize an archive_string object on the stack or elsewhere. */
#define	archive_string_init(a)	do { (a)->s = NULL; (a)->length = 0; (a)->buffer_length = 0; } while(0)

/* Append a C char to an archive_string, resizing as necessary. */
archive_string * archive_strappend_char(archive_string *, char);

/* Ditto for a wchar_t and an archive_wstring. */
archive_wstring * archive_wstrappend_wchar(archive_wstring *, wchar_t);
/* Append a raw array to an archive_string, resizing as necessary */
archive_string * archive_array_append(archive_string *, const char *, size_t);

/* Convert a Unicode string to current locale and append the result. */
/* Returns -1 if conversion fails. */
int archive_string_append_from_wcs(archive_string *, const wchar_t *, size_t);

/* Create a string conversion object.
 * Return NULL and set a error message if the conversion is not supported
 * on the platform. */
archive_string_conv * archive_string_conversion_to_charset(Archive *, const char *, int);
archive_string_conv * archive_string_conversion_from_charset(Archive *, const char *, int);
/* Create the default string conversion object for reading/writing an archive.
 * Return NULL if the conversion is unneeded.
 * Note: On non Windows platform this always returns NULL.
 */
archive_string_conv * archive_string_default_conversion_for_read(Archive *);
archive_string_conv * archive_string_default_conversion_for_write(Archive *);
/* Dispose of a string conversion object. */
void archive_string_conversion_free(Archive *);
const char * archive_string_conversion_charset_name(archive_string_conv *);
void archive_string_conversion_set_opt(archive_string_conv *, int);
#define SCONV_SET_OPT_UTF8_LIBARCHIVE2X	1
#define SCONV_SET_OPT_NORMALIZATION_C	2
#define SCONV_SET_OPT_NORMALIZATION_D	4


/* Copy one archive_string to another in locale conversion.
 * Return -1 if conversion fails. */
int archive_strncpy_l(archive_string *, const void *, size_t, archive_string_conv *);
/* Copy one archive_string to another in locale conversion.
 * Return -1 if conversion fails. */
int archive_strncat_l(archive_string *, const void *, size_t, archive_string_conv *);

/* Copy one archive_string to another */
#define	archive_string_copy(dest, src) ((dest)->length = 0, archive_string_concat((dest), (src)))
#define	archive_wstring_copy(dest, src) ((dest)->length = 0, archive_wstring_concat((dest), (src)))

/* Concatenate one archive_string to another */
void FASTCALL archive_string_concat(archive_string *dest, archive_string *src);
void FASTCALL archive_wstring_concat(archive_wstring *dest, archive_wstring *src);

/* Ensure that the underlying buffer is at least as large as the request. */
archive_string * FASTCALL archive_string_ensure(archive_string *, size_t);
archive_wstring * FASTCALL archive_wstring_ensure(archive_wstring *, size_t);

/* Append C string, which may lack trailing \0. */
/* The source is declared void * here because this gets used with
 * "signed char *", "uchar *" and "char *" arguments.
 * Declaring it "char *" as with some of the other functions just
 * leads to a lot of extra casts. */
archive_string  * STDCALL archive_strncat(archive_string *, const void *, size_t);
archive_wstring * STDCALL archive_wstrncat(archive_wstring *, const wchar_t *, size_t);

/* Append a C string to an archive_string, resizing as necessary. */
archive_string  * FASTCALL archive_strcat(archive_string *, const void *);
archive_wstring * FASTCALL archive_wstrcat(archive_wstring *, const wchar_t *);

/* Copy a C string to an archive_string, resizing as necessary. */
#define	archive_strcpy(as,p) archive_strncpy((as), (p), ((p) == NULL ? 0 : strlen(p)))
#define	archive_wstrcpy(as,p) archive_wstrncpy((as), (p), ((p) == NULL ? 0 : wcslen(p)))
#define	archive_strcpy_l(as,p,lo) archive_strncpy_l((as), (p), ((p) == NULL ? 0 : strlen(p)), (lo))

/* Copy a C string to an archive_string with limit, resizing as necessary. */
#define	archive_strncpy(as,p,l) ((as)->length=0, archive_strncat((as), (p), (l)))
#define	archive_wstrncpy(as,p,l) ((as)->length = 0, archive_wstrncat((as), (p), (l)))

/* Return length of string. */
#define	archive_strlen(a) ((a)->length)

/* Set string length to zero. */
#define	archive_string_empty(a) ((a)->length = 0)
#define	archive_wstring_empty(a) ((a)->length = 0)

/* Release any allocated storage resources. */
void	FASTCALL archive_string_free(archive_string *);
void	FASTCALL archive_wstring_free(archive_wstring *);
/* Like 'vsprintf', but resizes the underlying string as necessary. */
/* Note: This only implements a small subset of standard printf functionality. */
void	archive_string_vsprintf(archive_string *, const char *, va_list) __LA_PRINTF(2, 0);
void	archive_string_sprintf(archive_string *, const char *, ...) __LA_PRINTF(2, 3);

/* Translates from MBS to Unicode. */
/* Returns non-zero if conversion failed in any way. */
int archive_wstring_append_from_mbs(archive_wstring *dest, const char *, size_t);

/* A "multistring" can hold Unicode, UTF8, or MBS versions of
 * the string.  If you set and read the same version, no translation
 * is done.  If you set and read different versions, the library
 * will attempt to transparently convert.
 */
struct archive_mstring {
	archive_string aes_mbs;
	archive_string aes_utf8;
	archive_wstring aes_wcs;
	archive_string aes_mbs_in_locale;
	/* Bitmap of which of the above are valid.  Because we're lazy
	 * about malloc-ing and reusing the underlying storage, we
	 * can't rely on NULL pointers to indicate whether a string
	 * has been set. */
	int aes_set;
#define	AES_SET_MBS 1
#define	AES_SET_UTF8 2
#define	AES_SET_WCS 4
};

void   FASTCALL archive_mstring_clean(struct archive_mstring *);
void   FASTCALL archive_mstring_copy(struct archive_mstring *dest, struct archive_mstring *src);
int    STDCALL archive_mstring_get_mbs(Archive *, struct archive_mstring *, const char **);
int    archive_mstring_get_utf8(Archive *, struct archive_mstring *, const char **);
int    STDCALL archive_mstring_get_wcs(Archive *, struct archive_mstring *, const wchar_t **);
int    archive_mstring_get_mbs_l(Archive *, struct archive_mstring *, const char **, size_t *, archive_string_conv *);
int    archive_mstring_copy_mbs(struct archive_mstring *, const char *mbs);
int    archive_mstring_copy_mbs_len(struct archive_mstring *, const char *mbs, size_t);
int    archive_mstring_copy_utf8(struct archive_mstring *, const char *utf8);
int    archive_mstring_copy_wcs(struct archive_mstring *, const wchar_t *wcs);
int    archive_mstring_copy_wcs_len(struct archive_mstring *, const wchar_t *wcs, size_t);
int    archive_mstring_copy_mbs_len_l(struct archive_mstring *, const char *mbs, size_t, archive_string_conv *);
int    archive_mstring_update_utf8(Archive *, struct archive_mstring *aes, const char *utf8);

#endif
