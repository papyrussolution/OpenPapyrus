/* $Id: tif_unix.c,v 1.28 2017-01-11 19:02:49 erouault Exp $ */
/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 */
/*
 * TIFF Library UNIX-specific Routines. These are should also work with the
 * Windows Common RunTime Library.
 */
#include <slib-internal.h>
#pragma hdrstop
#include "tiffiop.h"
#ifdef HAVE_FCNTL_H
	#include <fcntl.h>
#endif
#ifdef HAVE_IO_H
	#include <io.h>
#endif

#define TIFF_IO_MAX 2147483647U

typedef union fd_as_handle_union {
	int fd;
	thandle_t h;
} fd_as_handle_union_t;

static tmsize_t _tiffReadProc(thandle_t fd, void * buf, tmsize_t size)
{
	fd_as_handle_union_t fdh;
	const size_t bytes_total = (size_t)size;
	size_t bytes_read;
	tmsize_t count = -1;
	if((tmsize_t)bytes_total != size) {
		errno = EINVAL;
		return (tmsize_t)-1;
	}
	else {
		fdh.h = fd;
		for(bytes_read = 0; bytes_read < bytes_total; bytes_read += count) {
			char * buf_offset = (char *)buf+bytes_read;
			size_t io_size = bytes_total-bytes_read;
			SETMIN(io_size, TIFF_IO_MAX);
			count = _read(fdh.fd, buf_offset, (TIFFIOSize_t)io_size);
			if(count <= 0)
				break;
		}
		return (count >= 0) ? (tmsize_t)bytes_read : (tmsize_t)-1;
	}
}

static tmsize_t _tiffWriteProc(thandle_t fd, void * buf, tmsize_t size)
{
	fd_as_handle_union_t fdh;
	const size_t bytes_total = (size_t)size;
	size_t bytes_written;
	tmsize_t count = -1;
	if((tmsize_t)bytes_total != size) {
		errno = EINVAL;
		return (tmsize_t)-1;
	}
	else {
		fdh.h = fd;
		for(bytes_written = 0; bytes_written < bytes_total; bytes_written += count) {
			const char * buf_offset = (char *)buf+bytes_written;
			size_t io_size = bytes_total-bytes_written;
			SETMIN(io_size, TIFF_IO_MAX);
			count = _write(fdh.fd, buf_offset, (TIFFIOSize_t)io_size);
			if(count <= 0)
				break;
		}
		return (count >= 0) ? (tmsize_t)bytes_written : (tmsize_t)-1;
		/* return ((tmsize_t) write(fdh.fd, buf, bytes_total)); */
	}
}

static uint64 _tiffSeekProc(thandle_t fd, uint64 off, int whence)
{
	fd_as_handle_union_t fdh;
	_TIFF_off_t off_io = (_TIFF_off_t)off;
	if((uint64)off_io != off) {
		errno = EINVAL;
		return (uint64)-1; /* this is really gross */
	}
	fdh.h = fd;
	return ((uint64)_TIFF_lseek_f(fdh.fd, off_io, whence));
}

static int _tiffCloseProc(thandle_t fd)
{
	fd_as_handle_union_t fdh;
	fdh.h = fd;
	return _close(fdh.fd);
}

static uint64 _tiffSizeProc(thandle_t fd)
{
	_TIFF_stat_s sb;
	fd_as_handle_union_t fdh;
	fdh.h = fd;
	return (_TIFF_fstat_f(fdh.fd, &sb) < 0) ? 0 : (uint64)sb.st_size;
}

#ifdef HAVE_MMAP
#include <sys/mman.h>

static int _tiffMapProc(thandle_t fd, void ** pbase, toff_t* psize)
{
	uint64 size64 = _tiffSizeProc(fd);
	tmsize_t sizem = (tmsize_t)size64;
	if((uint64)sizem==size64) {
		fd_as_handle_union_t fdh;
		fdh.h = fd;
		*pbase = (void *)mmap(0, (size_t)sizem, PROT_READ, MAP_SHARED, fdh.fd, 0);
		if(*pbase != (void *)-1) {
			*psize = (tmsize_t)sizem;
			return 1;
		}
	}
	return 0;
}

static void _tiffUnmapProc(thandle_t fd, void * base, toff_t size)
{
	(void)fd;
	(void)munmap(base, (off_t)size);
}

#else /* !HAVE_MMAP */
static int _tiffMapProc(thandle_t fd, void ** pbase, toff_t* psize)
{
	(void)fd; (void)pbase; (void)psize;
	return 0;
}

static void _tiffUnmapProc(thandle_t fd, void * base, toff_t size)
{
	(void)fd; (void)base; (void)size;
}
#endif /* !HAVE_MMAP */
/*
 * Open a TIFF file descriptor for read/writing.
 */
TIFF * TIFFFdOpen(int fd, const char * name, const char * mode)
{
	TIFF * tif;
	fd_as_handle_union_t fdh;
	fdh.fd = fd;
	tif = TIFFClientOpen(name, mode, fdh.h, _tiffReadProc, _tiffWriteProc, _tiffSeekProc, _tiffCloseProc, _tiffSizeProc, _tiffMapProc, _tiffUnmapProc);
	if(tif)
		tif->tif_fd = fd;
	return tif;
}
/*
 * Open a TIFF file for read/writing.
 */
TIFF * TIFFOpen(const char * name, const char * mode)
{
	static const char module[] = __FUNCTION__;
	int fd;
	TIFF * tif;
	int m = _TIFFgetMode(mode, module);
	if(m == -1)
		return 0;
/* for cygwin and mingw */
#ifdef O_BINARY
	m |= O_BINARY;
#endif
	fd = _open(name, m, 0666);
	if(fd < 0) {
		if(errno > 0 && strerror(errno) != NULL) {
			TIFFErrorExt(0, module, "%s: %s", name, strerror(errno));
		}
		else {
			TIFFErrorExt(0, module, "%s: Cannot open", name);
		}
		return 0;
	}
	tif = TIFFFdOpen((int)fd, name, mode);
	if(!tif)
		_close(fd);
	return tif;
}

#ifdef __WIN32__
//#include <windows.h>
/*
 * Open a TIFF file with a Unicode filename, for read/writing.
 */
TIFF * TIFFOpenW(const wchar_t * name, const char * mode)
{
	static const char module[] = __FUNCTION__;
	int fd;
	int mbsize;
	char * mbname;
	TIFF * tif;
	int m = _TIFFgetMode(mode, module);
	if(m == -1)
		return 0;
/* for cygwin and mingw */
#ifdef O_BINARY
	m |= O_BINARY;
#endif
	fd = _wopen(name, m, 0666);
	if(fd < 0) {
		TIFFErrorExt(0, module, "%ls: Cannot open", name);
		return 0;
	}
	mbname = NULL;
	mbsize = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
	if(mbsize > 0) {
		mbname = (char *)SAlloc::M(mbsize);
		if(!mbname) {
			TIFFErrorExt(0, module, "Can't allocate space for filename conversion buffer");
			return 0;
		}
		WideCharToMultiByte(CP_ACP, 0, name, -1, mbname, mbsize, NULL, NULL);
	}
	tif = TIFFFdOpen((int)fd, (mbname != NULL) ? mbname : "<unknown>", mode);
	SAlloc::F(mbname);
	if(!tif)
		_close(fd);
	return tif;
}

#endif

/*void * _TIFFmalloc_Removed(tmsize_t s)
{
	if(s == 0)
		return ((void *)NULL);
	return (malloc((size_t)s));
}*/

/*void * _TIFFcalloc_Removed(tmsize_t nmemb, tmsize_t siz)
{
	if(nmemb == 0 || siz == 0)
		return ((void *)NULL);
	return calloc((size_t)nmemb, (size_t)siz);
}*/

//void _TIFFfree_Removed(void * p) { free(p); }
//void * _TIFFrealloc_Removed(void * p, tmsize_t s) { return (realloc(p, (size_t)s)); }
//void _TIFFmemset_Removed(void * p, int v, tmsize_t c) { memset(p, v, (size_t)c); }
//void _TIFFmemcpy_Removed(void * d, const void * s, tmsize_t c) { memcpy(d, s, (size_t)c); }
//int _TIFFmemcmp(const void * p1, const void * p2, tmsize_t c) { return (memcmp(p1, p2, (size_t)c)); }

static void unixWarningHandler(const char * module, const char * fmt, va_list ap)
{
	if(module)
		slfprintf_stderr("%s: ", module);
	slfprintf_stderr("Warning, ");
	vfprintf(stderr, fmt, ap);
	slfprintf_stderr(".\n");
}

TIFFErrorHandler _TIFFwarningHandler = unixWarningHandler;

static void unixErrorHandler(const char * module, const char * fmt, va_list ap)
{
	if(module != NULL)
		slfprintf_stderr("%s: ", module);
	vfprintf(stderr, fmt, ap);
	slfprintf_stderr(".\n");
}

TIFFErrorHandler _TIFFerrorHandler = unixErrorHandler;
