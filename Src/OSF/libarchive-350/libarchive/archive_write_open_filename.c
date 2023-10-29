/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_open_filename.c 191165 2009-04-17 00:39:35Z kientzle $");
//#ifdef HAVE_UNISTD_H
	//#include <unistd.h>
//#endif
#ifndef O_BINARY
	#define O_BINARY 0
#endif
#ifndef O_CLOEXEC
	#define O_CLOEXEC       0
#endif

struct write_file_data {
	int fd;
	struct archive_mstring filename;
};

static int file_close(Archive *, void *);
static int file_free(Archive *, void *);
static int file_open(Archive *, void *);
static ssize_t  file_write(Archive *, void *, const void * buff, size_t);
static int open_filename(Archive *, int, const void *);

int archive_write_open_file(Archive * a, const char * filename)
{
	return (archive_write_open_filename(a, filename));
}

int archive_write_open_filename(Archive * a, const char * filename)
{
	return isempty(filename) ? archive_write_open_fd(a, 1) : open_filename(a, 1, filename);
}

int archive_write_open_filename_w(Archive * a, const wchar_t * filename)
{
	return isempty(filename) ? archive_write_open_fd(a, 1) : open_filename(a, 0, filename);
}

static int open_filename(Archive * a, int mbs_fn, const void * filename)
{
	int r;
	struct write_file_data * mine = (struct write_file_data *)SAlloc::C(1, sizeof(*mine));
	if(mine == NULL) {
		archive_set_error(a, ENOMEM, SlTxtOutOfMem);
		return ARCHIVE_FATAL;
	}
	if(mbs_fn)
		r = archive_mstring_copy_mbs(&mine->filename, static_cast<const char *>(filename));
	else
		r = archive_mstring_copy_wcs(&mine->filename, static_cast<const wchar_t *>(filename));
	if(r < 0) {
		if(errno == ENOMEM) {
			archive_set_error(a, ENOMEM, SlTxtOutOfMem);
			return ARCHIVE_FATAL;
		}
		if(mbs_fn)
			archive_set_error(a, ARCHIVE_ERRNO_MISC, "Can't convert '%s' to WCS", (const char *)filename);
		else
			archive_set_error(a, ARCHIVE_ERRNO_MISC, "Can't convert '%S' to MBS", (const wchar_t *)filename);
		return ARCHIVE_FAILED;
	}
	mine->fd = -1;
	return (archive_write_open2(a, mine, file_open, reinterpret_cast<archive_write_callback *>(file_write), file_close, file_free));
}

static int file_open(Archive * a, void * client_data)
{
	int flags;
	struct write_file_data * mine;
	struct stat st;
#if defined(_WIN32) && !defined(__CYGWIN__)
	wchar_t * fullpath;
#endif
	const wchar_t * wcs;
	const char * mbs;

	mine = (struct write_file_data *)client_data;
	flags = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_CLOEXEC;

	/*
	 * Open the file.
	 */
	mbs = NULL; wcs = NULL;
#if defined(_WIN32) && !defined(__CYGWIN__)
	if(archive_mstring_get_wcs(a, &mine->filename, &wcs) != 0) {
		if(errno == ENOMEM)
			archive_set_error(a, errno, SlTxtOutOfMem);
		else {
			archive_mstring_get_mbs(a, &mine->filename, &mbs);
			archive_set_error(a, errno, "Can't convert '%s' to WCS", mbs);
		}
		return ARCHIVE_FATAL;
	}
	fullpath = __la_win_permissive_name_w(wcs);
	if(fullpath != NULL) {
		mine->fd = _wopen(fullpath, flags, 0666);
		SAlloc::F(fullpath);
	}
	else
		mine->fd = _wopen(wcs, flags, 0666);
#else
	if(archive_mstring_get_mbs(a, &mine->filename, &mbs) != 0) {
		if(errno == ENOMEM)
			archive_set_error(a, errno, SlTxtOutOfMem);
		else {
			archive_mstring_get_wcs(a, &mine->filename, &wcs);
			archive_set_error(a, errno, "Can't convert '%S' to MBS", wcs);
		}
		return ARCHIVE_FATAL;
	}
	mine->fd = open(mbs, flags, 0666);
	__archive_ensure_cloexec_flag(mine->fd);
#endif
	if(mine->fd < 0) {
		if(mbs != NULL)
			archive_set_error(a, errno, "Failed to open '%s'", mbs);
		else
			archive_set_error(a, errno, "Failed to open '%S'", wcs);
		return ARCHIVE_FATAL;
	}
	if(fstat(mine->fd, &st) != 0) {
		if(mbs != NULL)
			archive_set_error(a, errno, "Couldn't stat '%s'", mbs);
		else
			archive_set_error(a, errno, "Couldn't stat '%S'", wcs);
		return ARCHIVE_FATAL;
	}
	/*
	 * Set up default last block handling.
	 */
	if(archive_write_get_bytes_in_last_block(a) < 0) {
		if(S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode) ||
		    S_ISFIFO(st.st_mode))
			/* Pad last block when writing to device or FIFO. */
			archive_write_set_bytes_in_last_block(a, 0);
		else
			/* Don't pad last block otherwise. */
			archive_write_set_bytes_in_last_block(a, 1);
	}

	/*
	 * If the output file is a regular file, don't add it to
	 * itself.  If it's a device file, it's okay to add the device
	 * entry to the output archive.
	 */
	if(S_ISREG(st.st_mode))
		archive_write_set_skip_file(a, st.st_dev, st.st_ino);

	return ARCHIVE_OK;
}

static ssize_t file_write(Archive * a, void * client_data, const void * buff,
    size_t length)
{
	struct write_file_data  * mine;
	ssize_t bytesWritten;

	mine = (struct write_file_data *)client_data;
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

static int file_close(Archive * a, void * client_data)
{
	struct write_file_data  * mine = (struct write_file_data *)client_data;
	CXX_UNUSED(a);
	if(mine == NULL)
		return ARCHIVE_FATAL;
	if(mine->fd >= 0)
		close(mine->fd);
	return ARCHIVE_OK;
}

static int file_free(Archive * a, void * client_data)
{
	struct write_file_data  * mine = (struct write_file_data *)client_data;
	CXX_UNUSED(a);
	if(mine == NULL)
		return ARCHIVE_OK;
	archive_mstring_clean(&mine->filename);
	SAlloc::F(mine);
	return ARCHIVE_OK;
}
