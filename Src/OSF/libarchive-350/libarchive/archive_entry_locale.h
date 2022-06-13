/*-
 * Copyright (c) 2011 Michihiro NAKAJIMA
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
 * $FreeBSD$
 */
#ifndef ARCHIVE_ENTRY_LOCALE_H_INCLUDED
#define ARCHIVE_ENTRY_LOCALE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
#error This header is only to be used internally to libarchive.
#endif

struct ArchiveEntry;
struct archive_string_conv;
/*
 * Utility functions to set and get entry attributes by translating
 * character-set. These are designed for use in format readers and writers.
 *
 * The return code and interface of these are quite different from other
 * functions for archive_entry defined in archive_entry.h.
 * Common return code are:
 *   Return 0 if the string conversion succeeded.
 *   Return -1 if the string conversion failed.
 */

#define archive_entry_gname_l	_archive_entry_gname_l
int _archive_entry_gname_l(ArchiveEntry *, const char **, size_t *, archive_string_conv *);
#define archive_entry_hardlink_l	_archive_entry_hardlink_l
int _archive_entry_hardlink_l(ArchiveEntry *, const char **, size_t *, archive_string_conv *);
#define archive_entry_pathname_l	_archive_entry_pathname_l
int _archive_entry_pathname_l(ArchiveEntry *, const char **, size_t *, archive_string_conv *);
#define archive_entry_symlink_l	_archive_entry_symlink_l
int _archive_entry_symlink_l(ArchiveEntry *, const char **, size_t *, archive_string_conv *);
#define archive_entry_uname_l	_archive_entry_uname_l
int _archive_entry_uname_l(ArchiveEntry *, const char **, size_t *, archive_string_conv *);
#define archive_entry_acl_text_l _archive_entry_acl_text_l
int _archive_entry_acl_text_l(ArchiveEntry *, int, const char **, size_t *, archive_string_conv *) __LA_DEPRECATED;
#define archive_entry_acl_to_text_l _archive_entry_acl_to_text_l
char *_archive_entry_acl_to_text_l(ArchiveEntry *, ssize_t *, int, archive_string_conv *);
#define archive_entry_acl_from_text_l _archive_entry_acl_from_text_l
int _archive_entry_acl_from_text_l(ArchiveEntry *, const char* text, int type, archive_string_conv *);
#define archive_entry_copy_gname_l	_archive_entry_copy_gname_l
int _archive_entry_copy_gname_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);
#define archive_entry_copy_hardlink_l	_archive_entry_copy_hardlink_l
int _archive_entry_copy_hardlink_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);
#define archive_entry_copy_link_l	_archive_entry_copy_link_l
int _archive_entry_copy_link_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);
#define archive_entry_copy_pathname_l	_archive_entry_copy_pathname_l
int _archive_entry_copy_pathname_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);
#define archive_entry_copy_symlink_l	_archive_entry_copy_symlink_l
int _archive_entry_copy_symlink_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);
#define archive_entry_copy_uname_l	_archive_entry_copy_uname_l
int _archive_entry_copy_uname_l(ArchiveEntry *, const char *, size_t, archive_string_conv *);

#endif /* ARCHIVE_ENTRY_LOCALE_H_INCLUDED */
