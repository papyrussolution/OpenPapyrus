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
__FBSDID("$FreeBSD: src/lib/libarchive/archive_write_open_file.c,v 1.19 2007/01/09 08:05:56 kientzle Exp $");

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

struct write_FILE_data {
	FILE            * f;
};

static int file_free(struct archive *, void *);
static int file_open(struct archive *, void *);
static ssize_t  file_write(struct archive *, void *, const void * buff, size_t);

int archive_write_open_FILE(struct archive * a, FILE * f)
{
	struct write_FILE_data * mine = (struct write_FILE_data *)SAlloc::M(sizeof(*mine));
	if(mine == NULL) {
		archive_set_error(a, ENOMEM, "No memory");
		return ARCHIVE_FATAL;
	}
	mine->f = f;
	return (archive_write_open2(a, mine, file_open, reinterpret_cast<archive_write_callback *>(file_write), NULL, file_free));
}

static int file_open(struct archive * a, void * client_data)
{
	CXX_UNUSED(a);
	(void)client_data; /* UNUSED */
	return ARCHIVE_OK;
}

static ssize_t file_write(struct archive * a, void * client_data, const void * buff, size_t length)
{
	size_t bytesWritten;
	struct write_FILE_data * mine = static_cast<struct write_FILE_data *>(client_data);
	for(;;) {
		bytesWritten = fwrite(buff, 1, length, mine->f);
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
	struct write_FILE_data  * mine = static_cast<struct write_FILE_data *>(client_data);
	CXX_UNUSED(a);
	if(mine == NULL)
		return ARCHIVE_OK;
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
