/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * $FreeBSD$
 */
#ifndef ARCHIVE_PATHMATCH_H
#define ARCHIVE_PATHMATCH_H

#ifndef __LIBARCHIVE_BUILD
#ifndef __LIBARCHIVE_TEST
#error This header is only to be used internally to libarchive.
#endif
#endif

/* Don't anchor at beginning unless the pattern starts with "^" */
#define PATHMATCH_NO_ANCHOR_START	1
/* Don't anchor at end unless the pattern ends with "$" */
#define PATHMATCH_NO_ANCHOR_END 	2

/* Note that "^" and "$" are not special unless you set the corresponding
 * flag above. */

int __archive_pathmatch(const char *p, const char *s, int flags);
int __archive_pathmatch_w(const wchar_t *p, const wchar_t *s, int flags);

#define archive_pathmatch(p, s, f)	__archive_pathmatch(p, s, f)
#define archive_pathmatch_w(p, s, f)	__archive_pathmatch_w(p, s, f)

#endif
