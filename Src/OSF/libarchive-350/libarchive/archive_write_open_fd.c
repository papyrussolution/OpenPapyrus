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
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_open_fd.c 201093 2009-12-28 02:28:44Z kientzle $");
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

struct write_fd_data {
	int fd;
};

static int file_free(struct archive *, void *);
static int file_open(struct archive *, void *);
static ssize_t  file_write(struct archive *, void *, const void * buff, size_t);

int archive_write_open_fd(struct archive * a, int fd)
{
	struct write_fd_data * mine = (struct write_fd_data *)SAlloc::M(sizeof(*mine));
	if(mine == NULL) {
		archive_set_error(a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	mine->fd = fd;
#if defined(__CYGWIN__) || defined(_WIN32)
	setmode(mine->fd, O_BINARY);
#endif
	return (archive_write_open2(a, mine, file_open, reinterpret_cast<archive_write_callback *>(file_write), NULL, file_free));
}

static int file_open(struct archive * a, void * client_data)
{
	struct stat st;
	struct write_fd_data * mine = (struct write_fd_data *)client_data;
	if(fstat(mine->fd, &st) != 0) {
		archive_set_error(a, errno, "Couldn't stat fd %d", mine->fd);
		return ARCHIVE_FATAL;
	}
	/*
	 * If this is a regular file, don't add it to itself.
	 */
	if(S_ISREG(st.st_mode))
		archive_write_set_skip_file(a, st.st_dev, st.st_ino);
	/*
	 * If client hasn't explicitly set the last block handling,
	 * then set it here.
	 */
	if(archive_write_get_bytes_in_last_block(a) < 0) {
		/* If the output is a block or character device, fifo,
		 * or stdout, pad the last block, otherwise leave it
		 * unpadded. */
		if(S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) || S_ISFIFO(st.st_mode) || (mine->fd == 1))
			/* Last block will be fully padded. */
			archive_write_set_bytes_in_last_block(a, 0);
		else
			archive_write_set_bytes_in_last_block(a, 1);
	}
	return ARCHIVE_OK;
}

static ssize_t file_write(struct archive * a, void * client_data, const void * buff, size_t length)
{
	ssize_t bytesWritten;
	struct write_fd_data * mine = (struct write_fd_data *)client_data;
	for(;;) {
		bytesWritten = write(mine->fd, buff, length);
		if(bytesWritten <= 0) {
			if(errno == EINTR)
				continue;
			archive_set_error(a, errno, "Write error");
			return -1;
		}
		return (bytesWritten);
	}
}

static int file_free(struct archive * a, void * client_data)
{
	struct write_fd_data * mine = (struct write_fd_data *)client_data;
	CXX_UNUSED(a);
	if(mine == NULL)
		return ARCHIVE_OK;
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
