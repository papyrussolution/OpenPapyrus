/*-
 * Copyright (c) 2003-2007 Tim Kientzle
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
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_entry_xattr.c 201096 2009-12-28 02:41:27Z kientzle $");

#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>   /* for Linux file flags */
#endif
/*
 * Some Linux distributions have both linux/ext2_fs.h and ext2fs/ext2_fs.h.
 * As the include guards don't agree, the order of include is important.
 */
#ifdef HAVE_LINUX_EXT2_FS_H
#include <linux/ext2_fs.h>      /* for Linux file flags */
#endif
#if defined(HAVE_EXT2FS_EXT2_FS_H) && !defined(__CYGWIN__)
#include <ext2fs/ext2_fs.h>     /* for Linux file flags */
#endif
#include "archive_entry_private.h"

/*
 * extended attribute handling
 */

void archive_entry_xattr_clear(ArchiveEntry * entry)
{
	while(entry->xattr_head) {
		struct ae_xattr * xp = entry->xattr_head->next;
		SAlloc::F(entry->xattr_head->name);
		SAlloc::F(entry->xattr_head->value);
		SAlloc::F(entry->xattr_head);
		entry->xattr_head = xp;
	}
	entry->xattr_head = NULL;
}

void archive_entry_xattr_add_entry(ArchiveEntry * entry, const char * name, const void * value, size_t size)
{
	struct ae_xattr * xp;
	if((xp = (struct ae_xattr *)SAlloc::M(sizeof(struct ae_xattr))) == NULL)
		__archive_errx(1, SlTxtOutOfMem);
	if((xp->name = sstrdup(name)) == NULL)
		__archive_errx(1, SlTxtOutOfMem);
	if((xp->value = SAlloc::M(size)) != NULL) {
		memcpy(xp->value, value, size);
		xp->size = size;
	}
	else
		xp->size = 0;
	xp->next = entry->xattr_head;
	entry->xattr_head = xp;
}
/*
 * returns number of the extended attribute entries
 */
int archive_entry_xattr_count(ArchiveEntry * entry)
{
	int count = 0;
	for(struct ae_xattr * xp = entry->xattr_head; xp; xp = xp->next)
		count++;
	return count;
}

int archive_entry_xattr_reset(ArchiveEntry * entry)
{
	entry->xattr_p = entry->xattr_head;
	return archive_entry_xattr_count(entry);
}

int archive_entry_xattr_next(ArchiveEntry * entry, const char ** name, const void ** value, size_t * size)
{
	if(entry->xattr_p) {
		*name = entry->xattr_p->name;
		*value = entry->xattr_p->value;
		*size = entry->xattr_p->size;
		entry->xattr_p = entry->xattr_p->next;
		return ARCHIVE_OK;
	}
	else {
		*name = NULL;
		*value = NULL;
		*size = (size_t)0;
		return ARCHIVE_WARN;
	}
}
