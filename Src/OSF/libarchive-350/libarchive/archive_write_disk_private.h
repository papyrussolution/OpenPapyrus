/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
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
