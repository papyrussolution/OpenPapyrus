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

#ifdef HAVE_ZLIB_H
struct private_data {
	z_stream stream;
	char in_stream;
	uchar   * out_block;
	size_t out_block_size;
	int64 total_out;
	ulong crc;
	uint32 mtime;
	char            * name;
	char eof; /* True = found end of compressed data. */
};

/* Gzip Filter. */
static ssize_t  gzip_filter_read(struct archive_read_filter *, const void **);
static int gzip_filter_close(struct archive_read_filter *);
#endif

/*
 * Note that we can detect gzip archives even if we can't decompress
 * them.  (In fact, we like detecting them because we can give better
 * error messages.)  So the bid framework here gets compiled even
 * if zlib is unavailable.
 *
 * TODO: If zlib is unavailable, gzip_bidder_init() should
 * use the compress_program framework to try to fire up an external
 * gzip program.
 */
static int gzip_bidder_bid(struct archive_read_filter_bidder *,
    struct archive_read_filter *);
static int gzip_bidder_init(struct archive_read_filter *);

#if ARCHIVE_VERSION_NUMBER < 4000000
/* Deprecated; remove in libarchive 4.0 */
int archive_read_support_compression_gzip(struct archive * a)
{
	return archive_read_support_filter_gzip(a);
}

#endif

int archive_read_support_filter_gzip(struct archive * _a)
{
	struct archive_read * a = (struct archive_read *)_a;
	struct archive_read_filter_bidder * bidder;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(__archive_read_get_bidder(a, &bidder) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	bidder->data = NULL;
	bidder->name = "gzip";
	bidder->bid = gzip_bidder_bid;
	bidder->init = gzip_bidder_init;
	bidder->options = NULL;
	bidder->free = NULL; /* No data, so no cleanup necessary. */
	/* Signal the extent of gzip support with the return value here. */
#ifdef HAVE_ZLIB_H
	return ARCHIVE_OK;
#else
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external gzip program");
	return ARCHIVE_WARN;
#endif
}

/*
 * Read and verify the header.
 *
 * Returns zero if the header couldn't be validated, else returns
 * number of bytes in header.  If pbits is non-NULL, it receives a
 * count of bits verified, suitable for use by bidder.
 */
static ssize_t peek_at_header(struct archive_read_filter * filter, int * pbits,
#ifdef HAVE_ZLIB_H
    struct private_data * state
#else
    void * state
#endif
    )
{
	const uchar * p;
	ssize_t avail, len;
	int bits = 0;
	int header_flags;
#ifndef HAVE_ZLIB_H
	(void)state; /* UNUSED */
#endif

	/* Start by looking at the first ten bytes of the header, which
	 * is all fixed layout. */
	len = 10;
	p = static_cast<const uchar *>(__archive_read_filter_ahead(filter, len, &avail));
	if(p == NULL || avail == 0)
		return 0;
	/* We only support deflation- third byte must be 0x08. */
	if(memcmp(p, "\x1F\x8B\x08", 3) != 0)
		return 0;
	bits += 24;
	if((p[3] & 0xE0)!= 0)   /* No reserved flags set. */
		return 0;
	bits += 3;
	header_flags = p[3];
	/* Bytes 4-7 are mod time in little endian. */
#ifdef HAVE_ZLIB_H
	if(state)
		state->mtime = archive_le32dec(p + 4);
#endif
	/* Byte 8 is deflate flags. */
	/* XXXX TODO: return deflate flags back to consume_header for use
	   in initializing the decompressor. */
	/* Byte 9 is OS. */

	/* Optional extra data:  2 byte length plus variable body. */
	if(header_flags & 4) {
		p = static_cast<const uchar *>(__archive_read_filter_ahead(filter, len + 2, &avail));
		if(!p)
			return 0;
		len += ((int)p[len + 1] << 8) | (int)p[len];
		len += 2;
	}

	/* Null-terminated optional filename. */
	if(header_flags & 8) {
#ifdef HAVE_ZLIB_H
		ssize_t file_start = len;
#endif
		do {
			++len;
			if(avail < len)
				p = static_cast<const uchar *>(__archive_read_filter_ahead(filter, len, &avail));
			if(!p)
				return 0;
		} while(p[len - 1] != 0);

#ifdef HAVE_ZLIB_H
		if(state) {
			/* Reset the name in case of repeat header reads. */
			SAlloc::F(state->name);
			state->name = sstrdup((const char *)&p[file_start]);
		}
#endif
	}

	/* Null-terminated optional comment. */
	if(header_flags & 16) {
		do {
			++len;
			if(avail < len)
				p = static_cast<const uchar *>(__archive_read_filter_ahead(filter, len, &avail));
			if(!p)
				return 0;
		} while(p[len - 1] != 0);
	}
	/* Optional header CRC */
	if((header_flags & 2)) {
		p = static_cast<const uchar *>(__archive_read_filter_ahead(filter, len + 2, &avail));
		if(!p)
			return 0;
#if 0
		int hcrc = ((int)p[len + 1] << 8) | (int)p[len];
		int crc = /* XXX TODO: Compute header CRC. */;
		if(crc != hcrc)
			return 0;
		bits += 16;
#endif
		len += 2;
	}

	if(pbits != NULL)
		*pbits = bits;
	return (len);
}

/*
 * Bidder just verifies the header and returns the number of verified bits.
 */
static int gzip_bidder_bid(struct archive_read_filter_bidder * self,
    struct archive_read_filter * filter)
{
	int bits_checked;

	CXX_UNUSED(self);

	if(peek_at_header(filter, &bits_checked, NULL))
		return (bits_checked);
	return 0;
}

#ifndef HAVE_ZLIB_H

/*
 * If we don't have the library on this system, we can't do the
 * decompression directly.  We can, however, try to run "gzip -d"
 * in case that's available.
 */
static int gzip_bidder_init(struct archive_read_filter * self)
{
	int r;

	r = __archive_read_program(self, "gzip -d");
	/* Note: We set the format here even if __archive_read_program()
	 * above fails.  We do, after all, know what the format is
	 * even if we weren't able to read it. */
	self->code = ARCHIVE_FILTER_GZIP;
	self->name = "gzip";
	return r;
}

#else

static int gzip_read_header(struct archive_read_filter * self, struct archive_entry * entry)
{
	struct private_data * state = (struct private_data *)self->data;
	/* A mtime of 0 is considered invalid/missing. */
	if(state->mtime != 0)
		archive_entry_set_mtime(entry, state->mtime, 0);
	/* If the name is available, extract it. */
	if(state->name)
		archive_entry_set_pathname(entry, state->name);
	return ARCHIVE_OK;
}
/*
 * Initialize the filter object.
 */
static int gzip_bidder_init(struct archive_read_filter * self)
{
	struct private_data * state;
	static const size_t out_block_size = 64 * 1024;
	void * out_block;
	self->code = ARCHIVE_FILTER_GZIP;
	self->name = "gzip";
	state = (struct private_data *)SAlloc::C(sizeof(*state), 1);
	out_block = (uchar *)SAlloc::M(out_block_size);
	if(state == NULL || out_block == NULL) {
		SAlloc::F(out_block);
		SAlloc::F(state);
		archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for gzip decompression");
		return ARCHIVE_FATAL;
	}
	self->data = state;
	state->out_block_size = out_block_size;
	state->out_block = static_cast<uchar *>(out_block);
	self->FnRead = gzip_filter_read;
	self->skip = NULL; /* not supported */
	self->FnClose = gzip_filter_close;
#ifdef HAVE_ZLIB_H
	self->read_header = gzip_read_header;
#endif
	state->in_stream = 0; /* We're not actually within a stream yet. */
	return ARCHIVE_OK;
}

static int consume_header(struct archive_read_filter * self)
{
	ssize_t avail;
	int ret;
	struct private_data * state = (struct private_data *)self->data;
	/* If this is a real header, consume it. */
	size_t len = peek_at_header(self->upstream, NULL, state);
	if(len == 0)
		return (ARCHIVE_EOF);
	__archive_read_filter_consume(self->upstream, len);
	/* Initialize CRC accumulator. */
	state->crc = crc32(0L, NULL, 0);
	/* Initialize compression library. */
	state->stream.next_in = (uchar *)(uintptr_t)__archive_read_filter_ahead(self->upstream, 1, &avail);
	state->stream.avail_in = (uInt)avail;
	ret = inflateInit2(&(state->stream), -15 /* Don't check for zlib header */);

	/* Decipher the error code. */
	switch(ret) {
		case Z_OK:
		    state->in_stream = 1;
		    return ARCHIVE_OK;
		case Z_STREAM_ERROR:
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Internal error initializing compression library: invalid setup parameter");
		    break;
		case Z_MEM_ERROR:
		    archive_set_error(&self->archive->archive, ENOMEM, "Internal error initializing compression library: out of memory");
		    break;
		case Z_VERSION_ERROR:
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Internal error initializing compression library: invalid library version");
		    break;
		default:
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Internal error initializing compression library: Zlib error %d", ret);
		    break;
	}
	return ARCHIVE_FATAL;
}

static int consume_trailer(struct archive_read_filter * self)
{
	const uchar * p;
	ssize_t avail;
	struct private_data * state = (struct private_data *)self->data;
	state->in_stream = 0;
	switch(inflateEnd(&(state->stream))) {
		case Z_OK:
		    break;
		default:
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Failed to clean up gzip decompressor");
		    return ARCHIVE_FATAL;
	}

	/* GZip trailer is a fixed 8 byte structure. */
	p = static_cast<const uchar *>(__archive_read_filter_ahead(self->upstream, 8, &avail));
	if(p == NULL || avail == 0)
		return ARCHIVE_FATAL;
	/* XXX TODO: Verify the length and CRC. */
	/* We've verified the trailer, so consume it now. */
	__archive_read_filter_consume(self->upstream, 8);
	return ARCHIVE_OK;
}

static ssize_t gzip_filter_read(struct archive_read_filter * self, const void ** p)
{
	size_t decompressed;
	ssize_t avail_in, max_in;
	int ret;
	struct private_data * state = (struct private_data *)self->data;
	// Empty our output buffer. 
	state->stream.next_out = state->out_block;
	state->stream.avail_out = (uInt)state->out_block_size;
	/* Try to fill the output buffer. */
	while(state->stream.avail_out > 0 && !state->eof) {
		// If we're not in a stream, read a header and initialize the decompression library.
		if(!state->in_stream) {
			ret = consume_header(self);
			if(ret == ARCHIVE_EOF) {
				state->eof = 1;
				break;
			}
			if(ret < ARCHIVE_OK)
				return ret;
		}
		/* Peek at the next available data. */
		/* ZLib treats stream.next_in as const but doesn't declare
		 * it so, hence this ugly cast. */
		state->stream.next_in = (uchar *)(uintptr_t)__archive_read_filter_ahead(self->upstream, 1, &avail_in);
		if(state->stream.next_in == NULL) {
			archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "truncated gzip input");
			return ARCHIVE_FATAL;
		}
		if(UINT_MAX >= SSIZE_MAX)
			max_in = SSIZE_MAX;
		else
			max_in = UINT_MAX;
		if(avail_in > max_in)
			avail_in = max_in;
		state->stream.avail_in = (uInt)avail_in;
		/* Decompress and consume some of that data. */
		ret = inflate(&(state->stream), 0);
		switch(ret) {
			case Z_OK: /* Decompressor made some progress. */
			    __archive_read_filter_consume(self->upstream,
				avail_in - state->stream.avail_in);
			    break;
			case Z_STREAM_END: /* Found end of stream. */
			    __archive_read_filter_consume(self->upstream,
				avail_in - state->stream.avail_in);
			    /* Consume the stream trailer; release the
			     * decompression library. */
			    ret = consume_trailer(self);
			    if(ret < ARCHIVE_OK)
				    return ret;
			    break;
			default:
			    /* Return an error. */
			    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "gzip decompression failed");
			    return ARCHIVE_FATAL;
		}
	}
	/* We've read as much as we can. */
	decompressed = state->stream.next_out - state->out_block;
	state->total_out += decompressed;
	*p = decompressed ? state->out_block : NULL;
	return (decompressed);
}
/*
 * Clean up the decompressor.
 */
static int gzip_filter_close(struct archive_read_filter * self)
{
	struct private_data * state = (struct private_data *)self->data;
	int ret = ARCHIVE_OK;
	if(state->in_stream) {
		switch(inflateEnd(&(state->stream))) {
			case Z_OK:
			    break;
			default:
			    archive_set_error(&(self->archive->archive), ARCHIVE_ERRNO_MISC, "Failed to clean up gzip compressor"); 
				ret = ARCHIVE_FATAL;
		}
	}
	SAlloc::F(state->name);
	SAlloc::F(state->out_block);
	SAlloc::F(state);
	return ret;
}

#endif /* HAVE_ZLIB_H */
