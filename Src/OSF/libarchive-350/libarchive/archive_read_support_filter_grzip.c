/*-
 * Copyright (c) 2012 Michihiro NAKAJIMA
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
__FBSDID("$FreeBSD$");

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "archive_read_private.h"

static const uchar grzip_magic[] = {
	0x47, 0x52, 0x5a, 0x69, 0x70, 0x49, 0x49, 0x00,
	0x02, 0x04, 0x3a, 0x29
};

static int grzip_bidder_bid(ArchiveReadFilterBidder *, ArchiveReadFilter *);
static int grzip_bidder_init(ArchiveReadFilter *);

static int grzip_reader_free(ArchiveReadFilterBidder * self)
{
	CXX_UNUSED(self);
	return ARCHIVE_OK;
}

int archive_read_support_filter_grzip(Archive * _a)
{
	ArchiveRead * a = (ArchiveRead *)_a;
	ArchiveReadFilterBidder * reader;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	reader->data = NULL;
	reader->FnBid = grzip_bidder_bid;
	reader->FnInit = grzip_bidder_init;
	reader->FnOptions = NULL;
	reader->FnFree = grzip_reader_free;
	// This filter always uses an external program
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external grzip program for grzip decompression");
	return ARCHIVE_WARN;
}

/*
 * Bidder just verifies the header and returns the number of verified bits.
 */
static int grzip_bidder_bid(ArchiveReadFilterBidder * self, ArchiveReadFilter * filter)
{
	const uchar * p;
	ssize_t avail;
	CXX_UNUSED(self);
	p = (const uchar *)__archive_read_filter_ahead(filter, sizeof(grzip_magic), &avail);
	if(p == NULL || avail == 0)
		return 0;
	if(memcmp(p, grzip_magic, sizeof(grzip_magic)))
		return 0;
	return (sizeof(grzip_magic) * 8);
}

static int grzip_bidder_init(ArchiveReadFilter * self)
{
	int r = __archive_read_program(self, "grzip -d");
	// Note: We set the format here even if __archive_read_program()
	// above fails.  We do, after all, know what the format is
	// even if we weren't able to read it. */
	self->code = ARCHIVE_FILTER_GRZIP;
	self->name = "grzip";
	return r;
}
