/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_read_open_file.c 201093 2009-12-28 02:28:44Z kientzle $");
//#ifdef HAVE_UNISTD_H
	//#include <unistd.h>
//#endif

struct read_FILE_data {
	FILE * f;
	size_t block_size;
	void * buffer;
	char can_skip;
};

static int file_close(Archive *, void *);
static ssize_t  file_read(Archive *, void *, const void ** buff);
static int64  file_skip(Archive *, void *, int64 request);

int archive_read_open_FILE(Archive * a, FILE * f)
{
	struct stat st;
	struct read_FILE_data * mine;
	size_t block_size = 128 * 1024;
	void * b;
	archive_clear_error(a);
	mine = (struct read_FILE_data *)SAlloc::M(sizeof(*mine));
	b = SAlloc::M(block_size);
	if(mine == NULL || b == NULL) {
		archive_set_error(a, ENOMEM, SlTxtOutOfMem);
		SAlloc::F(mine);
		SAlloc::F(b);
		return ARCHIVE_FATAL;
	}
	mine->block_size = block_size;
	mine->buffer = b;
	mine->f = f;
	/*
	 * If we can't fstat() the file, it may just be that it's not
	 * a file.  (On some platforms, FILE * objects can wrap I/O
	 * streams that don't support fileno()).  As a result, fileno()
	 * should be used cautiously.)
	 */
	if(fstat(fileno(mine->f), &st) == 0 && S_ISREG(st.st_mode)) {
		archive_read_extract_set_skip_file(a, st.st_dev, st.st_ino);
		/* Enable the seek optimization only for regular files. */
		mine->can_skip = 1;
	}
	else
		mine->can_skip = 0;

#if defined(__CYGWIN__) || defined(_WIN32)
	setmode(fileno(mine->f), O_BINARY);
#endif
	archive_read_set_read_callback(a, reinterpret_cast<archive_read_callback *>(file_read));
	archive_read_set_skip_callback(a, file_skip);
	archive_read_set_close_callback(a, file_close);
	archive_read_set_callback_data(a, mine);
	return (archive_read_open1(a));
}

static ssize_t file_read(Archive * a, void * client_data, const void ** buff)
{
	struct read_FILE_data * mine = (struct read_FILE_data *)client_data;
	size_t bytes_read;
	*buff = mine->buffer;
	bytes_read = fread(mine->buffer, 1, mine->block_size, mine->f);
	if(bytes_read < mine->block_size && ferror(mine->f)) {
		archive_set_error(a, errno, "Error reading file");
	}
	return (bytes_read);
}

static int64 file_skip(Archive * a, void * client_data, int64 request)
{
	struct read_FILE_data * mine = (struct read_FILE_data *)client_data;
#if HAVE_FSEEKO
	off_t skip = (off_t)request;
#elif HAVE__FSEEKI64
	int64 skip = request;
#else
	long skip = (long)request;
#endif
	int skip_bits = sizeof(skip) * 8 - 1;
	CXX_UNUSED(a);
	/*
	 * If we can't skip, return 0 as the amount we did step and
	 * the caller will work around by reading and discarding.
	 */
	if(!mine->can_skip)
		return 0;
	if(request == 0)
		return 0;
	// If request is too big for a long or an off_t, reduce it. 
	if(sizeof(request) > sizeof(skip)) {
		int64 max_skip = (((int64)1 << (skip_bits - 1)) - 1) * 2 + 1;
		if(request > max_skip)
			skip = static_cast<long>(max_skip);
	}
#ifdef __ANDROID__
	// fileno() isn't safe on all platforms ... see above. 
	if(lseek(fileno(mine->f), skip, SEEK_CUR) < 0)
#elif HAVE_FSEEKO
	if(fseeko(mine->f, skip, SEEK_CUR) != 0)
#elif HAVE__FSEEKI64
	if(_fseeki64(mine->f, skip, SEEK_CUR) != 0)
#else
	if(fseek(mine->f, skip, SEEK_CUR) != 0)
#endif
	{
		mine->can_skip = 0;
		return 0;
	}
	return (request);
}

static int file_close(Archive * a, void * client_data)
{
	struct read_FILE_data * mine = (struct read_FILE_data *)client_data;
	CXX_UNUSED(a);
	SAlloc::F(mine->buffer);
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
