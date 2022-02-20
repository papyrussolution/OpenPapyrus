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
__FBSDID("$FreeBSD: src/lib/libarchive/archive_read_data_into_fd.c,v 1.16 2008/05/23 05:01:29 cperciva Exp $");

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "archive.h"
#include "archive_private.h"

/* Maximum amount of data to write at one time. */
#define MAX_WRITE       (1024 * 1024)

/*
 * This implementation minimizes copying of data and is sparse-file aware.
 */
static int pad_to(struct archive * a, int fd, int can_lseek,
    size_t nulls_size, const char * nulls,
    int64 target_offset, int64 actual_offset)
{
	size_t to_write;
	ssize_t bytes_written;

	if(can_lseek) {
		actual_offset = lseek(fd,
			target_offset - actual_offset, SEEK_CUR);
		if(actual_offset != target_offset) {
			archive_set_error(a, errno, "Seek error");
			return ARCHIVE_FATAL;
		}
		return ARCHIVE_OK;
	}
	while(target_offset > actual_offset) {
		to_write = nulls_size;
		if(target_offset < actual_offset + (int64)nulls_size)
			to_write = (size_t)(target_offset - actual_offset);
		bytes_written = write(fd, nulls, to_write);
		if(bytes_written < 0) {
			archive_set_error(a, errno, "Write error");
			return ARCHIVE_FATAL;
		}
		actual_offset += bytes_written;
	}
	return ARCHIVE_OK;
}

int archive_read_data_into_fd(struct archive * a, int fd)
{
	struct stat st;
	int r, r2;
	const void * buff;
	size_t size, bytes_to_write;
	ssize_t bytes_written;
	int64 target_offset;
	int64 actual_offset = 0;
	int can_lseek;
	char * nulls = NULL;
	size_t nulls_size = 16384;

	archive_check_magic(a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_DATA,
	    "archive_read_data_into_fd");

	can_lseek = (fstat(fd, &st) == 0) && S_ISREG(st.st_mode);
	if(!can_lseek)
		nulls = static_cast<char *>(SAlloc::C(1, nulls_size));
	while((r = archive_read_data_block(a, &buff, &size, &target_offset)) ==
	    ARCHIVE_OK) {
		const char * p = static_cast<const char *>(buff);
		if(target_offset > actual_offset) {
			r = pad_to(a, fd, can_lseek, nulls_size, nulls,
				target_offset, actual_offset);
			if(r != ARCHIVE_OK)
				break;
			actual_offset = target_offset;
		}
		while(size > 0) {
			bytes_to_write = size;
			if(bytes_to_write > MAX_WRITE)
				bytes_to_write = MAX_WRITE;
			bytes_written = write(fd, p, bytes_to_write);
			if(bytes_written < 0) {
				archive_set_error(a, errno, "Write error");
				r = ARCHIVE_FATAL;
				goto cleanup;
			}
			actual_offset += bytes_written;
			p += bytes_written;
			size -= bytes_written;
		}
	}

	if(r == ARCHIVE_EOF && target_offset > actual_offset) {
		r2 = pad_to(a, fd, can_lseek, nulls_size, nulls,
			target_offset, actual_offset);
		if(r2 != ARCHIVE_OK)
			r = r2;
	}

cleanup:
	SAlloc::F(nulls);
	if(r != ARCHIVE_EOF)
		return r;
	return ARCHIVE_OK;
}
