/** @file
 * @brief class wrapper around zlib
 */
/* Copyright (C) 2012 Dan Colish
 * Copyright (C) 2012,2013,2014,2016 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef XAPIAN_INCLUDED_COMPRESSION_STREAM_H
#define XAPIAN_INCLUDED_COMPRESSION_STREAM_H

#include <zlib.h>

class CompressionStream {
	int compress_strategy;
	size_t out_len;
	char * out;
	z_stream* deflate_zstream; /// Zlib state object for deflating
	z_stream* inflate_zstream; /// Zlib state object for inflating
	void lazy_alloc_deflate_zstream(); /// Allocate the zstream for deflating, if not already allocated.
	void lazy_alloc_inflate_zstream(); /// Allocate the zstream for inflating, if not already allocated.
public:
	/* Create a new CompressionStream object.
	 *
	 *  @param compress_strategy_	Z_DEFAULT_STRATEGY,
	 *					Z_FILTERED, Z_HUFFMAN_ONLY, or Z_RLE.
	 */
	explicit CompressionStream(int compress_strategy_ = Z_DEFAULT_STRATEGY) : compress_strategy(compress_strategy_),
		out_len(0), out(NULL), deflate_zstream(NULL), inflate_zstream(NULL)
	{
	}
	~CompressionStream();
	const char * compress(const char * buf, size_t* p_size);
	void decompress_start() 
	{
		lazy_alloc_inflate_zstream();
	}
	/** Returns true if this was the final chunk. */
	bool decompress_chunk(const char * p, int len, std::string & buf);
};

#endif // XAPIAN_INCLUDED_COMPRESSION_STREAM_H
