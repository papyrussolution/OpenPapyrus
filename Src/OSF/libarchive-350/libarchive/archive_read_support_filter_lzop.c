/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * Copyright (c) 2012 Michihiro NAKAJIMA
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

//#ifdef HAVE_UNISTD_H
//#include <unistd.h>
//#endif
#ifdef HAVE_LZO_LZOCONF_H
	#include <lzo/lzoconf.h>
#endif
#ifdef HAVE_LZO_LZO1X_H
	#include <lzo/lzo1x.h>
#endif
#if !defined(HAVE_ZLIB_H) && defined(HAVE_LZO_LZOCONF_H) && defined(HAVE_LZO_LZO1X_H)
	#include "archive_crc32.h"
#endif
//#include "archive_read_private.h"

#ifndef HAVE_ZLIB_H
#define adler32 lzo_adler32
#endif

#define LZOP_HEADER_MAGIC "\x89\x4c\x5a\x4f\x00\x0d\x0a\x1a\x0a"
#define LZOP_HEADER_MAGIC_LEN 9

#if defined(HAVE_LZO_LZOCONF_H) && defined(HAVE_LZO_LZO1X_H)
struct read_lzop {
	uchar   * out_block;
	size_t out_block_size;
	int64 total_out;
	int flags;
	uint32 compressed_cksum;
	uint32 uncompressed_cksum;
	size_t compressed_size;
	size_t uncompressed_size;
	size_t unconsumed_bytes;
	char in_stream;
	char eof; /* True = found end of compressed data. */
};

#define FILTER                  0x0800
#define CRC32_HEADER            0x1000
#define EXTRA_FIELD             0x0040
#define ADLER32_UNCOMPRESSED    0x0001
#define ADLER32_COMPRESSED      0x0002
#define CRC32_UNCOMPRESSED      0x0100
#define CRC32_COMPRESSED        0x0200
#define MAX_BLOCK_SIZE          (64 * 1024 * 1024)

static ssize_t  lzop_filter_read(ArchiveReadFilter *, const void **);
static int lzop_filter_close(ArchiveReadFilter *);
#endif

static int lzop_bidder_bid(ArchiveReadFilterBidder *, ArchiveReadFilter *);
static int lzop_bidder_init(ArchiveReadFilter *);

int archive_read_support_filter_lzop(Archive * _a)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	ArchiveReadFilterBidder * reader;
	archive_check_magic(_a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	if(__archive_read_get_bidder(a, &reader) != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	reader->data = NULL;
	reader->FnBid = lzop_bidder_bid;
	reader->FnInit = lzop_bidder_init;
	reader->FnOptions = NULL;
	reader->FnFree = NULL;
	// Signal the extent of lzop support with the return value here
#if defined(HAVE_LZO_LZOCONF_H) && defined(HAVE_LZO_LZO1X_H)
	return ARCHIVE_OK;
#else
	// Return ARCHIVE_WARN since this always uses an external program
	archive_set_error(_a, ARCHIVE_ERRNO_MISC, "Using external lzop program for lzop decompression");
	return ARCHIVE_WARN;
#endif
}
/*
 * Bidder just verifies the header and returns the number of verified bits.
 */
static int lzop_bidder_bid(ArchiveReadFilterBidder * self, ArchiveReadFilter * filter)
{
	const uchar * p;
	ssize_t avail;
	CXX_UNUSED(self);
	p = (const uchar *)__archive_read_filter_ahead(filter, LZOP_HEADER_MAGIC_LEN, &avail);
	if(p == NULL || avail == 0)
		return 0;
	if(memcmp(p, LZOP_HEADER_MAGIC, LZOP_HEADER_MAGIC_LEN))
		return 0;
	return (LZOP_HEADER_MAGIC_LEN * 8);
}

#if !defined(HAVE_LZO_LZOCONF_H) || !defined(HAVE_LZO_LZO1X_H)
/*
 * If we don't have the library on this system, we can't do the
 * decompression directly.  We can, however, try to run "lzop -d"
 * in case that's available.
 */
static int lzop_bidder_init(ArchiveReadFilter * self)
{
	int r = __archive_read_program(self, "lzop -d");
	/* Note: We set the format here even if __archive_read_program()
	 * above fails.  We do, after all, know what the format is
	 * even if we weren't able to read it. */
	self->code = ARCHIVE_FILTER_LZOP;
	self->name = "lzop";
	return r;
}

#else
/*
 * Initialize the filter object.
 */
static int lzop_bidder_init(ArchiveReadFilter * self)
{
	struct read_lzop * state;
	self->code = ARCHIVE_FILTER_LZOP;
	self->name = "lzop";
	state = (struct read_lzop *)SAlloc::C(sizeof(*state), 1);
	if(state == NULL) {
		archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for lzop decompression");
		return ARCHIVE_FATAL;
	}
	self->data = state;
	self->read = lzop_filter_read;
	self->skip = NULL; /* not supported */
	self->close = lzop_filter_close;
	return ARCHIVE_OK;
}

static int consume_header(ArchiveReadFilter * self)
{
	struct read_lzop * state = (struct read_lzop *)self->data;
	const uchar * p, * _p;
	unsigned checksum, flags, len, method, version;
	/*
	 * Check LZOP magic code.
	 */
	p = __archive_read_filter_ahead(self->upstream, LZOP_HEADER_MAGIC_LEN, NULL);
	if(!p)
		return (ARCHIVE_EOF);
	if(memcmp(p, LZOP_HEADER_MAGIC, LZOP_HEADER_MAGIC_LEN))
		return (ARCHIVE_EOF);
	__archive_read_filter_consume(self->upstream, LZOP_HEADER_MAGIC_LEN);
	p = __archive_read_filter_ahead(self->upstream, 29, NULL);
	if(!p)
		goto truncated;
	_p = p;
	version = archive_be16dec(p);
	p += 4; /* version(2 bytes) + library version(2 bytes) */
	if(version >= 0x940) {
		unsigned reqversion = archive_be16dec(p); p += 2;
		if(reqversion < 0x900) {
			archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Invalid required version");
			return ARCHIVE_FAILED;
		}
	}
	method = *p++;
	if(method < 1 || method > 3) {
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Unsupported method");
		return ARCHIVE_FAILED;
	}
	if(version >= 0x940) {
		unsigned level = *p++;
#if 0
		unsigned default_level[] = {0, 3, 1, 9};
#endif
		if(level == 0)
		/* Method is 1..3 here due to check above. */
#if 0   /* Avoid an error Clang Static Analyzer claims
			"Value stored to 'level' is never read". */
			level = default_level[method];
#else
			; /* NOP */
#endif
		else if(level > 9) {
			archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Invalid level");
			return ARCHIVE_FAILED;
		}
	}
	flags = archive_be32dec(p); p += 4;
	if(flags & FILTER)
		p += 4; /* Skip filter */
	p += 4; /* Skip mode */
	if(version >= 0x940)
		p += 8; /* Skip mtime */
	else
		p += 4; /* Skip mtime */
	len = *p++; /* Read filename length */
	len += p - _p;
	/* Make sure we have all bytes we need to calculate checksum. */
	p = __archive_read_filter_ahead(self->upstream, len + 4, NULL);
	if(!p)
		goto truncated;
	if(flags & CRC32_HEADER)
		checksum = crc32(crc32(0, NULL, 0), p, len);
	else
		checksum = adler32(adler32(0, NULL, 0), p, len);
	if(archive_be32dec(p + len) != checksum)
		goto corrupted;
	__archive_read_filter_consume(self->upstream, len + 4);
	if(flags & EXTRA_FIELD) {
		/* Skip extra field */
		p = __archive_read_filter_ahead(self->upstream, 4, NULL);
		if(!p)
			goto truncated;
		len = archive_be32dec(p);
		__archive_read_filter_consume(self->upstream, len + 4 + 4);
	}
	state->flags = flags;
	state->in_stream = 1;
	return ARCHIVE_OK;
truncated:
	archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Truncated lzop data");
	return ARCHIVE_FAILED;
corrupted:
	archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Corrupted lzop header");
	return ARCHIVE_FAILED;
}

static int consume_block_info(ArchiveReadFilter * self)
{
	struct read_lzop * state = (struct read_lzop *)self->data;
	const uchar * p;
	unsigned flags = state->flags;

	p = __archive_read_filter_ahead(self->upstream, 4, NULL);
	if(!p)
		goto truncated;
	state->uncompressed_size = archive_be32dec(p);
	__archive_read_filter_consume(self->upstream, 4);
	if(state->uncompressed_size == 0)
		return (ARCHIVE_EOF);
	if(state->uncompressed_size > MAX_BLOCK_SIZE)
		goto corrupted;

	p = __archive_read_filter_ahead(self->upstream, 4, NULL);
	if(!p)
		goto truncated;
	state->compressed_size = archive_be32dec(p);
	__archive_read_filter_consume(self->upstream, 4);
	if(state->compressed_size > state->uncompressed_size)
		goto corrupted;

	if(flags & (CRC32_UNCOMPRESSED | ADLER32_UNCOMPRESSED)) {
		p = __archive_read_filter_ahead(self->upstream, 4, NULL);
		if(!p)
			goto truncated;
		state->compressed_cksum = state->uncompressed_cksum =
			archive_be32dec(p);
		__archive_read_filter_consume(self->upstream, 4);
	}
	if((flags & (CRC32_COMPRESSED | ADLER32_COMPRESSED)) &&
	    state->compressed_size < state->uncompressed_size) {
		p = __archive_read_filter_ahead(self->upstream, 4, NULL);
		if(!p)
			goto truncated;
		state->compressed_cksum = archive_be32dec(p);
		__archive_read_filter_consume(self->upstream, 4);
	}
	return ARCHIVE_OK;
truncated:
	archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Truncated lzop data");
	return ARCHIVE_FAILED;
corrupted:
	archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Corrupted lzop header");
	return ARCHIVE_FAILED;
}

static ssize_t lzop_filter_read(ArchiveReadFilter * self, const void ** p)
{
	struct read_lzop * state = (struct read_lzop *)self->data;
	const void * b;
	lzo_uint out_size;
	uint32 cksum;
	int ret, r;

	if(state->unconsumed_bytes) {
		__archive_read_filter_consume(self->upstream,
		    state->unconsumed_bytes);
		state->unconsumed_bytes = 0;
	}
	if(state->eof)
		return 0;

	for(;;) {
		if(!state->in_stream) {
			ret = consume_header(self);
			if(ret < ARCHIVE_OK)
				return ret;
			if(ret == ARCHIVE_EOF) {
				state->eof = 1;
				return 0;
			}
		}
		ret = consume_block_info(self);
		if(ret < ARCHIVE_OK)
			return ret;
		if(ret == ARCHIVE_EOF)
			state->in_stream = 0;
		else
			break;
	}
	if(state->out_block == NULL || state->out_block_size < state->uncompressed_size) {
		void * new_block = SAlloc::R(state->out_block, state->uncompressed_size);
		if(new_block == NULL) {
			archive_set_error(&self->archive->archive, ENOMEM, "Can't allocate data for lzop decompression");
			return ARCHIVE_FATAL;
		}
		state->out_block = new_block;
		state->out_block_size = state->uncompressed_size;
	}
	b = __archive_read_filter_ahead(self->upstream, state->compressed_size, NULL);
	if(!b) {
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Truncated lzop data");
		return ARCHIVE_FATAL;
	}
	if(state->flags & CRC32_COMPRESSED)
		cksum = crc32(crc32(0, NULL, 0), b, state->compressed_size);
	else if(state->flags & ADLER32_COMPRESSED)
		cksum = adler32(adler32(0, NULL, 0), b, state->compressed_size);
	else
		cksum = state->compressed_cksum;
	if(cksum != state->compressed_cksum) {
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Corrupted data");
		return ARCHIVE_FATAL;
	}
	/*
	 * If the both uncompressed size and compressed size are the same,
	 * we do not decompress this block.
	 */
	if(state->uncompressed_size == state->compressed_size) {
		*p = b;
		state->total_out += state->compressed_size;
		state->unconsumed_bytes = state->compressed_size;
		return ((ssize_t)state->uncompressed_size);
	}

	/*
	 * Drive lzo uncompression.
	 */
	out_size = (lzo_uint)state->uncompressed_size;
	r = lzo1x_decompress_safe(b, (lzo_uint)state->compressed_size,
		state->out_block, &out_size, NULL);
	switch(r) {
		case LZO_E_OK:
		    if(out_size == state->uncompressed_size)
			    break;
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Corrupted data");
		    return ARCHIVE_FATAL;
		case LZO_E_OUT_OF_MEMORY:
		    archive_set_error(&self->archive->archive, ENOMEM, "lzop decompression failed: out of memory");
		    return ARCHIVE_FATAL;
		default:
		    archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "lzop decompression failed: %d", r);
		    return ARCHIVE_FATAL;
	}

	if(state->flags & CRC32_UNCOMPRESSED)
		cksum = crc32(crc32(0, NULL, 0), state->out_block,
			state->uncompressed_size);
	else if(state->flags & ADLER32_UNCOMPRESSED)
		cksum = adler32(adler32(0, NULL, 0), state->out_block,
			state->uncompressed_size);
	else
		cksum = state->uncompressed_cksum;
	if(cksum != state->uncompressed_cksum) {
		archive_set_error(&self->archive->archive, ARCHIVE_ERRNO_MISC, "Corrupted data");
		return ARCHIVE_FATAL;
	}

	__archive_read_filter_consume(self->upstream, state->compressed_size);
	*p = state->out_block;
	state->total_out += out_size;
	return ((ssize_t)out_size);
}

/*
 * Clean up the decompressor.
 */
static int lzop_filter_close(ArchiveReadFilter * self)
{
	struct read_lzop * state = (struct read_lzop *)self->data;

	SAlloc::F(state->out_block);
	SAlloc::F(state);
	return ARCHIVE_OK;
}

#endif
