/*-
 * Copyright (c) 2017 Martin Matuska
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
/* !!ONLY FOR USE INTERNALLY TO LIBARCHIVE!! */

#ifndef ARCHIVE_PLATFORM_XATTR_H_INCLUDED
#define ARCHIVE_PLATFORM_XATTR_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
#ifndef __LIBARCHIVE_TEST_COMMON
#error This header is only to be used internally to libarchive.
#endif
#endif

/*
 * Determine if we support extended attributes
 */
#if ARCHIVE_XATTR_LINUX || ARCHIVE_XATTR_DARWIN || ARCHIVE_XATTR_FREEBSD || \
    ARCHIVE_XATTR_AIX
#define ARCHIVE_XATTR_SUPPORT     1
#endif

#endif	/* ARCHIVE_PLATFORM_XATTR_H_INCLUDED */
