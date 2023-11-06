/*-
 * Copyright (c) 2002 Thomas Moestl <tmm@FreeBSD.org>
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
 * $FreeBSD: head/lib/libarchive/archive_endian.h 201085 2009-12-28 02:17:15Z kientzle $
 *
 * Borrowed from FreeBSD's <sys/endian.h>
 */
#ifndef ARCHIVE_ENDIAN_H_INCLUDED
#define ARCHIVE_ENDIAN_H_INCLUDED

/* Note:  This is a purely internal header! */
/* Do not use this outside of libarchive internal code! */

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

/*
 * Disabling inline keyword for compilers known to choke on it:
 * - Watcom C++ in C code.  (For any version?)
 * - SGI MIPSpro
 * - Microsoft Visual C++ 6.0 (supposedly newer versions too)
 * - IBM VisualAge 6 (XL v6)
 * - Sun WorkShop C (SunPro) before 5.9
 */
#if defined(__WATCOMC__) || defined(__sgi) || defined(__hpux) || defined(__BORLANDC__)
	#define	inline
#elif defined(__IBMC__) && __IBMC__ < 700
	#define	inline
#elif defined(__SUNPRO_C) && __SUNPRO_C < 0x590
	#define inline
#elif defined(_MSC_VER) || defined(__osf__)
	#define inline __inline
#endif

/* Alignment-agnostic encode/decode bytestream to/from little/big endian. */

static inline uint16 archive_be16dec(const void *pp)
{
	uchar const * p = (uchar const *)pp;
	/* Store into unsigned temporaries before left shifting, to avoid
	promotion to signed int and then left shifting into the sign bit,
	which is undefined behaviour. */
	uint p1 = p[1];
	uint p0 = p[0];
	return ((p0 << 8) | p1);
}

static inline uint32 archive_be32dec(const void *pp)
{
	uchar const * p = (uchar const *)pp;
	/* Store into unsigned temporaries before left shifting, to avoid
	promotion to signed int and then left shifting into the sign bit,
	which is undefined behaviour. */
	uint p3 = p[3];
	uint p2 = p[2];
	uint p1 = p[1];
	uint p0 = p[0];
	return ((p0 << 24) | (p1 << 16) | (p2 << 8) | p3);
}

static inline uint64 archive_be64dec(const void *pp)
{
	uchar const * p = (uchar const *)pp;
	return (((uint64)archive_be32dec(p) << 32) | archive_be32dec(p + 4));
}

static inline uint16 archive_le16dec(const void *pp)
{
	uchar const * p = (uchar const *)pp;
	/* Store into unsigned temporaries before left shifting, to avoid
	promotion to signed int and then left shifting into the sign bit,
	which is undefined behaviour. */
	uint p1 = p[1];
	uint p0 = p[0];
	return ((p1 << 8) | p0);
}

static inline uint32 archive_le32dec(const void *pp)
{
	uchar const * p = (uchar const *)pp;
	/* Store into unsigned temporaries before left shifting, to avoid
	promotion to signed int and then left shifting into the sign bit,
	which is undefined behaviour. */
	uint p3 = p[3];
	uint p2 = p[2];
	uint p1 = p[1];
	uint p0 = p[0];
	return ((p3 << 24) | (p2 << 16) | (p1 << 8) | p0);
}

static inline uint64 archive_le64dec(const void *pp)
{
	const uchar * p = (const uchar *)pp;
	return (((uint64)archive_le32dec(p + 4) << 32) | archive_le32dec(p));
}

static inline void archive_be16enc(void * pp, uint16 u)
{
	uchar * p = static_cast<uchar *>(pp);
	p[0] = (u >> 8) & 0xff;
	p[1] = u & 0xff;
}

static inline void archive_be32enc(void * pp, uint32 u)
{
	uchar * p = static_cast<uchar *>(pp);
	p[0] = static_cast<uchar>((u >> 24) & 0xff);
	p[1] = static_cast<uchar>((u >> 16) & 0xff);
	p[2] = static_cast<uchar>((u >> 8) & 0xff);
	p[3] = static_cast<uchar>(u & 0xff);
}

static inline void archive_be64enc(void *pp, uint64 u)
{
	uchar * p = static_cast<uchar *>(pp);
	archive_be32enc(p, (uint32)(u >> 32));
	archive_be32enc(p + 4, (uint32)(u & 0xffffffff));
}

static inline void archive_le16enc(void *pp, uint16 u)
{
	uchar * p = static_cast<uchar *>(pp);
	p[0] = u & 0xff;
	p[1] = (u >> 8) & 0xff;
}

static inline void archive_le32enc(void *pp, uint32 u)
{
	uchar * p = static_cast<uchar *>(pp);
	p[0] = static_cast<uchar>(u & 0xff);
	p[1] = static_cast<uchar>((u >> 8) & 0xff);
	p[2] = static_cast<uchar>((u >> 16) & 0xff);
	p[3] = static_cast<uchar>((u >> 24) & 0xff);
}

static inline void archive_le64enc(void *pp, uint64 u)
{
	uchar * p = static_cast<uchar *>(pp);
	archive_le32enc(p, (uint32)(u & 0xffffffff));
	archive_le32enc(p + 4, (uint32)(u >> 32));
}

#endif
