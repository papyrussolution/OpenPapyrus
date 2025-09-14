/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_read_support_format_empty.c 191524 2009-04-26 18:24:14Z kientzle $");
//#include "archive_read_private.h"

static int archive_read_format_empty_bid(ArchiveRead *, int);
static int archive_read_format_empty_read_data(ArchiveRead *, const void **, size_t *, int64 *);
static int archive_read_format_empty_read_header(ArchiveRead *, ArchiveEntry *);

int archive_read_support_format_empty(Archive * _a)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	int r = __archive_read_register_format(a, NULL, "empty", archive_read_format_empty_bid,
		NULL, archive_read_format_empty_read_header, archive_read_format_empty_read_data, NULL, NULL, NULL, NULL, NULL);
	return r;
}

static int archive_read_format_empty_bid(ArchiveRead * a, int best_bid)
{
	if(best_bid < 1 && __archive_read_ahead(a, 1, NULL) == NULL)
		return 1;
	return -1;
}

static int archive_read_format_empty_read_header(ArchiveRead * a, ArchiveEntry * entry)
{
	CXX_UNUSED(a);
	CXX_UNUSED(entry);
	a->archive.archive_format = ARCHIVE_FORMAT_EMPTY;
	a->archive.archive_format_name = "Empty file";
	return (ARCHIVE_EOF);
}

static int archive_read_format_empty_read_data(ArchiveRead * a, const void ** buff, size_t * size, int64 * offset)
{
	CXX_UNUSED(a);
	(void)buff; /*unused*/
	(void)size; /*unused*/
	CXX_UNUSED(offset);
	return (ARCHIVE_EOF);
}
