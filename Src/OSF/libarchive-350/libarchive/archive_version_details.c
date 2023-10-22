/*-
 * Copyright (c) 2009-2012,2014 Michihiro NAKAJIMA
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_util.c 201098 2009-12-28 02:58:14Z kientzle $");
#ifdef HAVE_LZ4_H
	#include <..\slib\lz4\lz4.h>
#endif
#ifdef HAVE_ZSTD_H
	#include <..\osf\zstd\lib\include\zstd.h>
#endif

const char * archive_version_details(void)
{
	static archive_string str;
	static int init = 0;
	const char * zlib = archive_zlib_version();
	const char * liblzma = archive_liblzma_version();
	const char * bzlib = archive_bzlib_version();
	const char * liblz4 = archive_liblz4_version();
	const char * libzstd = archive_libzstd_version();

	if(!init) {
		archive_string_init(&str);

		archive_strcat(&str, ARCHIVE_VERSION_STRING);
		if(zlib != NULL) {
			archive_strcat(&str, " zlib/");
			archive_strcat(&str, zlib);
		}
		if(liblzma) {
			archive_strcat(&str, " liblzma/");
			archive_strcat(&str, liblzma);
		}
		if(bzlib) {
			const char * p = bzlib;
			const char * sep = strchr(p, ',');
			if(sep == NULL)
				sep = p + strlen(p);
			archive_strcat(&str, " bz2lib/");
			archive_strncat(&str, p, sep - p);
		}
		if(liblz4) {
			archive_strcat(&str, " liblz4/");
			archive_strcat(&str, liblz4);
		}
		if(libzstd) {
			archive_strcat(&str, " libzstd/");
			archive_strcat(&str, libzstd);
		}
	}
	return str.s;
}

const char * archive_zlib_version(void)
{
#ifdef HAVE_ZLIB_H
	return ZLIB_VERSION;
#else
	return NULL;
#endif
}

const char * archive_liblzma_version(void)
{
#ifdef HAVE_LZMA_H
	return LZMA_VERSION_STRING;
#else
	return NULL;
#endif
}

const char * archive_bzlib_version(void)
{
#ifdef HAVE_BZLIB_H
	return BZ2_bzlibVersion();
#else
	return NULL;
#endif
}

const char * archive_liblz4_version(void)
{
#if defined(HAVE_LZ4_H) && defined(HAVE_LIBLZ4)
#define str(s) #s // @todo replacewith(STRINGIZE)
#define NUMBER(x) str(x)
	return NUMBER(LZ4_VERSION_MAJOR) "." NUMBER(LZ4_VERSION_MINOR) "." NUMBER(LZ4_VERSION_RELEASE);
#undef NUMBER
#undef str
#else
	return NULL;
#endif
}

const char * archive_libzstd_version(void)
{
#if HAVE_ZSTD_H && HAVE_LIBZSTD
	return ZSTD_VERSION_STRING;
#else
	return NULL;
#endif
}
