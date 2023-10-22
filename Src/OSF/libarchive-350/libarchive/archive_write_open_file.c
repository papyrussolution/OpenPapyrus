/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
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

static int file_free(Archive *, void *);
static int file_open(Archive *, void *);
static ssize_t  file_write(Archive *, void *, const void * buff, size_t);

int archive_write_open_FILE(Archive * a, FILE * f)
{
	struct write_FILE_data * mine = (struct write_FILE_data *)SAlloc::M(sizeof(*mine));
	if(mine == NULL) {
		archive_set_error(a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	mine->f = f;
	return (archive_write_open2(a, mine, file_open, reinterpret_cast<archive_write_callback *>(file_write), NULL, file_free));
}

static int file_open(Archive * a, void * client_data)
{
	CXX_UNUSED(a);
	(void)client_data; /* UNUSED */
	return ARCHIVE_OK;
}

static ssize_t file_write(Archive * a, void * client_data, const void * buff, size_t length)
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

static int file_free(Archive * a, void * client_data)
{
	struct write_FILE_data  * mine = static_cast<struct write_FILE_data *>(client_data);
	CXX_UNUSED(a);
	if(mine == NULL)
		return ARCHIVE_OK;
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
