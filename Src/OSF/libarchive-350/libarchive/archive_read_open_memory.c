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
__FBSDID("$FreeBSD: src/lib/libarchive/archive_read_open_memory.c,v 1.6 2007/07/06 15:51:59 kientzle Exp $");
/*
 * Glue to read an archive from a block of memory.
 *
 * This is mostly a huge help in building test harnesses;
 * test programs can build archives in memory and read them
 * back again without having to mess with files on disk.
 */

struct read_memory_data {
	const uchar     * start;
	const uchar     * p;
	const uchar     * end;
	ssize_t read_size;
};

static int memory_read_close(Archive *, void *);
static int memory_read_open(Archive *, void *);
static int64  memory_read_seek(Archive *, void *, int64 offset, int whence);
static int64  memory_read_skip(Archive *, void *, int64 request);
static ssize_t  memory_read(Archive *, void *, const void ** buff);

int archive_read_open_memory(Archive * a, const void * buff, size_t size)
{
	return archive_read_open_memory2(a, buff, size, size);
}

/*
 * Don't use _open_memory2() in production code; the archive_read_open_memory()
 * version is the one you really want.  This is just here so that
 * test harnesses can exercise block operations inside the library.
 */
int archive_read_open_memory2(Archive * a, const void * buff, size_t size, size_t read_size)
{
	struct read_memory_data * mine = (struct read_memory_data *)SAlloc::C(1, sizeof(*mine));
	if(mine == NULL) {
		archive_set_error(a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	mine->start = mine->p = (const uchar *)buff;
	mine->end = mine->start + size;
	mine->read_size = read_size;
	archive_read_set_open_callback(a, memory_read_open);
	archive_read_set_read_callback(a, reinterpret_cast<archive_read_callback *>(memory_read));
	archive_read_set_seek_callback(a, memory_read_seek);
	archive_read_set_skip_callback(a, memory_read_skip);
	archive_read_set_close_callback(a, memory_read_close);
	archive_read_set_callback_data(a, mine);
	return (archive_read_open1(a));
}

/*
 * There's nothing to open.
 */
static int memory_read_open(Archive * a, void * client_data)
{
	CXX_UNUSED(a);
	(void)client_data; /* UNUSED */
	return ARCHIVE_OK;
}

/*
 * This is scary simple:  Just advance a pointer.  Limiting
 * to read_size is not technically necessary, but it exercises
 * more of the internal logic when used with a small block size
 * in a test harness.  Production use should not specify a block
 * size; then this is much faster.
 */
static ssize_t memory_read(Archive * a, void * client_data, const void ** buff)
{
	struct read_memory_data * mine = (struct read_memory_data *)client_data;
	ssize_t size;

	CXX_UNUSED(a);
	*buff = mine->p;
	size = mine->end - mine->p;
	if(size > mine->read_size)
		size = mine->read_size;
	mine->p += size;
	return (size);
}

/*
 * Advancing is just as simple.  Again, this is doing more than
 * necessary in order to better exercise internal code when used
 * as a test harness.
 */
static int64 memory_read_skip(Archive * a, void * client_data, int64 skip)
{
	struct read_memory_data * mine = (struct read_memory_data *)client_data;

	CXX_UNUSED(a);
	if((int64)skip > (int64)(mine->end - mine->p))
		skip = mine->end - mine->p;
	/* Round down to block size. */
	skip /= mine->read_size;
	skip *= mine->read_size;
	mine->p += skip;
	return (skip);
}

/*
 * Seeking.
 */
static int64 memory_read_seek(Archive * a, void * client_data, int64 offset, int whence)
{
	struct read_memory_data * mine = (struct read_memory_data *)client_data;

	CXX_UNUSED(a);
	switch(whence) {
		case SEEK_SET:
		    mine->p = mine->start + offset;
		    break;
		case SEEK_CUR:
		    mine->p += offset;
		    break;
		case SEEK_END:
		    mine->p = mine->end + offset;
		    break;
		default:
		    return ARCHIVE_FATAL;
	}
	if(mine->p < mine->start) {
		mine->p = mine->start;
		return ARCHIVE_FAILED;
	}
	if(mine->p > mine->end) {
		mine->p = mine->end;
		return ARCHIVE_FAILED;
	}
	return (mine->p - mine->start);
}

/*
 * Close is just cleaning up our one small bit of data.
 */
static int memory_read_close(Archive * a, void * client_data)
{
	struct read_memory_data * mine = (struct read_memory_data *)client_data;
	CXX_UNUSED(a);
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
