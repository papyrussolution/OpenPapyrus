/*-
 * Copyright (c) 2009-2011 Sean Purcell
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
//#ifdef HAVE_UNISTD_H
	//#include <unistd.h>
//#endif
#if HAVE_ZSTD_H
	#include <..\osf\zstd\lib\include\zstd.h>
#endif
//#include "archive_read_private.h"

#if HAVE_ZSTD_H && HAVE_LIBZSTD

struct private_data {
	ZSTD_DStream    * dstream;
	uchar   * out_block;
	size_t out_block_size;
	int64 total_out;
	char in_frame; /* True = in the middle of a zstd frame. */
	char eof; /* True = found end of compressed data. */
};

/* Zstd Filter. */
static ssize_t  zstd_filter_read(ArchiveReadFilter *, const void**);
static int zstd_filter_close(ArchiveReadFilter *);
#endif

/*
 * Note that we can detect zstd compressed files even if we can't decompress
 * them.  (In fact, we like detecting them because we can give better error
 * messages.)  So the bid framework here gets compiled even if no zstd library
 * is available.
 */
static int zstd_bidder_bid(ArchiveReadFilterBidder *, ArchiveReadFilter *);
static int zstd_bidder_init(ArchiveReadFilter *);

int archive_read_support_filter_zstd(Archive * _a)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	ArchiveReadFilterBidder * bidder;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	bidder->data = NULL;
	bidder->name = "zstd";
	bidder->FnBid = zstd_bidder_bid;
	bidder->FnInit = zstd_bidder_init;
	bidder->FnOptions = NULL;
	bidder->FnFree = NULL;
#if HAVE_ZSTD_H && HAVE_LIBZSTD
	return ARCHIVE_OK;
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external zstd program for zstd decompression");
	return ARCHIVE_WARN;
#endif
}
/*
 * Test whether we can handle this data.
 */
static int zstd_bidder_bid(ArchiveReadFilterBidder * self, ArchiveReadFilter * filter)
{
	const uchar * buffer;
	ssize_t avail;
	uint   prefix;
	/* Zstd frame magic values */
	const uint zstd_magic = 0xFD2FB528U;
	const uint zstd_magic_skippable_start = 0x184D2A50U;
	const uint zstd_magic_skippable_mask = 0xFFFFFFF0;
	CXX_UNUSED(self);
	buffer = (const uchar *)__archive_read_filter_ahead(filter, 4, &avail);
	if(!buffer)
		return 0;
	prefix = archive_le32dec(buffer);
	if(prefix == zstd_magic)
		return (32);
	if((prefix & zstd_magic_skippable_mask) == zstd_magic_skippable_start)
		return (32);
	return 0;
}

#if !(HAVE_ZSTD_H && HAVE_LIBZSTD)
/*
 * If we don't have the library on this system, we can't do the
 * decompression directly.  We can, however, try to run "zstd -d"
 * in case that's available.
 */
static int zstd_bidder_init(ArchiveReadFilter * self)
{
	int r = __archive_read_program(self, "zstd -d -qq");
	/* Note: We set the format here even if __archive_read_program()
	 * above fails.  We do, after all, know what the format is
	 * even if we weren't able to read it. */
	self->code = ARCHIVE_FILTER_ZSTD;
	self->name = "zstd";
	return r;
}
#else
/*
 * Initialize the filter object
 */
static int zstd_bidder_init(ArchiveReadFilter * self)
{
	struct private_data * state;
	const size_t out_block_size = ZSTD_DStreamOutSize();
	void * out_block;
	ZSTD_DStream * dstream;
	self->code = ARCHIVE_FILTER_ZSTD;
	self->name = "zstd";
	state = (struct private_data *)SAlloc::C(sizeof(*state), 1);
	out_block = (uchar *)SAlloc::M(out_block_size);
	dstream = ZSTD_createDStream();
	if(state == NULL || out_block == NULL || dstream == NULL) {
		SAlloc::F(out_block);
		SAlloc::F(state);
		ZSTD_freeDStream(dstream); /* supports free on NULL */
		archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for zstd decompression");
		return ARCHIVE_FATAL;
	}
	self->data = state;
	state->out_block_size = out_block_size;
	state->out_block = (uchar *)out_block;
	state->dstream = dstream;
	self->FnRead = zstd_filter_read;
	self->skip = NULL; /* not supported */
	self->FnClose = zstd_filter_close;
	state->eof = 0;
	state->in_frame = 0;
	return ARCHIVE_OK;
}

static ssize_t zstd_filter_read(ArchiveReadFilter * self, const void ** p)
{
	size_t decompressed;
	ssize_t avail_in;
	ZSTD_inBuffer in;
	struct private_data * state = (struct private_data *)self->data;
	ZSTD_outBuffer out = { state->out_block, state->out_block_size, 0 };
	/* Try to fill the output buffer. */
	while(out.pos < out.size && !state->eof) {
		if(!state->in_frame) {
			const size_t ret = ZSTD_initDStream(state->dstream);
			if(ZSTD_isError(ret)) {
				archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Error initializing zstd decompressor: %s", ZSTD_getErrorName(ret));
				return ARCHIVE_FATAL;
			}
		}
		in.src = __archive_read_filter_ahead(self->upstream, 1, &avail_in);
		if(avail_in < 0) {
			return avail_in;
		}
		if(in.src == NULL && avail_in == 0) {
			if(!state->in_frame) {
				/* end of stream */
				state->eof = 1;
				break;
			}
			else {
				archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Truncated zstd input");
				return ARCHIVE_FATAL;
			}
		}
		in.size = avail_in;
		in.pos = 0;
		{
			const size_t ret = ZSTD_decompressStream(state->dstream, &out, &in);
			if(ZSTD_isError(ret)) {
				archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Zstd decompression failed: %s", ZSTD_getErrorName(ret));
				return ARCHIVE_FATAL;
			}
			/* Decompressor made some progress */
			__archive_read_filter_consume(self->upstream, in.pos);
			/* ret guaranteed to be > 0 if frame isn't done yet */
			state->in_frame = (ret != 0);
		}
	}
	decompressed = out.pos;
	state->total_out += decompressed;
	*p = decompressed ? state->out_block : NULL;
	return (decompressed);
}
/*
 * Clean up the decompressor.
 */
static int zstd_filter_close(ArchiveReadFilter * self)
{
	struct private_data * state = (struct private_data *)self->data;
	ZSTD_freeDStream(state->dstream);
	SAlloc::F(state->out_block);
	SAlloc::F(state);
	return ARCHIVE_OK;
}

#endif /* HAVE_ZLIB_H && HAVE_LIBZSTD */
