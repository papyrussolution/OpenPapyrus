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
__FBSDID("$FreeBSD$");

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#include "archive_read_private.h"

#define LRZIP_HEADER_MAGIC "LRZI"
#define LRZIP_HEADER_MAGIC_LEN 4

static int lrzip_bidder_bid(struct archive_read_filter_bidder *, struct archive_read_filter *);
static int lrzip_bidder_init(struct archive_read_filter *);

static int lrzip_reader_free(struct archive_read_filter_bidder * self)
{
	CXX_UNUSED(self);
	return ARCHIVE_OK;
}

int archive_read_support_filter_lrzip(struct archive * _a)
{
	struct archive_read * a = (struct archive_read *)_a;
	struct archive_read_filter_bidder * reader;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, "archive_read_support_filter_lrzip");
	if(__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	reader->data = NULL;
	reader->name = "lrzip";
	reader->bid = lrzip_bidder_bid;
	reader->init = lrzip_bidder_init;
	reader->options = NULL;
	reader->free = lrzip_reader_free;
	/* This filter always uses an external program. */
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external lrzip program for lrzip decompression");
	return ARCHIVE_WARN;
}

/*
 * Bidder just verifies the header and returns the number of verified bits.
 */
static int lrzip_bidder_bid(struct archive_read_filter_bidder * self,
    struct archive_read_filter * filter)
{
	const uchar * p;
	ssize_t avail, len;
	int i;

	CXX_UNUSED(self);
	/* Start by looking at the first six bytes of the header, which
	 * is all fixed layout. */
	len = 6;
	p = (const uchar *)__archive_read_filter_ahead(filter, len, &avail);
	if(p == NULL || avail == 0)
		return 0;

	if(memcmp(p, LRZIP_HEADER_MAGIC, LRZIP_HEADER_MAGIC_LEN))
		return 0;

	/* current major version is always 0, verify this */
	if(p[LRZIP_HEADER_MAGIC_LEN])
		return 0;
	/* support only v0.6+ lrzip for sanity */
	i = p[LRZIP_HEADER_MAGIC_LEN + 1];
	if((i < 6) || (i > 10))
		return 0;

	return (int)len;
}

static int lrzip_bidder_init(struct archive_read_filter * self)
{
	int r = __archive_read_program(self, "lrzip -d -q");
	/* Note: We set the format here even if __archive_read_program()
	 * above fails.  We do, after all, know what the format is
	 * even if we weren't able to read it. */
	self->code = ARCHIVE_FILTER_LRZIP;
	self->name = "lrzip";
	return r;
}
