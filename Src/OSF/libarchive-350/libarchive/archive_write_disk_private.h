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
 * $FreeBSD: head/lib/libarchive/archive_write_disk_private.h 201086 2009-12-28 02:17:53Z kientzle $
 */

#ifndef ARCHIVE_WRITE_DISK_PRIVATE_H_INCLUDED
#define ARCHIVE_WRITE_DISK_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
#error This header is only to be used internally to libarchive.
#endif

#include "archive_platform_acl.h"
#include "archive_acl_private.h"
#include "archive_entry.h"

struct archive_write_disk;

int archive_write_disk_set_acls(Archive *, int, const char *, archive_acl *, __LA_MODE_T);

#endif
