/*	$NetBSD: pack_dev.c,v 1.12 2013/06/14 16:28:20 tsutsui Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
/* Originally from NetBSD's mknod(8) source. */
#include "archive_platform.h"
#pragma hdrstop
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if !defined(lint)
__RCSID("$NetBSD$");
#endif /* not lint */
#if MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#define HAVE_MAJOR
#elif MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#define HAVE_MAJOR
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "archive_pack_dev.h"

static pack_t pack_netbsd;
static pack_t pack_freebsd;
static pack_t pack_8_8;
static pack_t pack_12_20;
static pack_t pack_14_18;
static pack_t pack_8_24;
static pack_t pack_bsdos;
static int     compare_format(const void *, const void *);

static const char iMajorError[] = "invalid major number";
static const char iMinorError[] = "invalid minor number";
static const char tooManyFields[] = "too many fields for format";

/* This is blatantly stolen from libarchive/archive_entry.c,
 * in an attempt to get this to play nice on MinGW... */
#if !defined(HAVE_MAJOR) && !defined(major)
/* Replacement for major/minor/makedev. */
#define major(x) ((int)(0x00ff & ((x) >> 8)))
#define minor(x) ((int)(0xffff00ff & (x)))
#define makedev(maj, min) ((0xff00 & ((maj)<<8)) | (0xffff00ff & (min)))
#endif

/* Play games to come up with a suitable makedev() definition. */
#ifdef __QNXNTO__
/* QNX.  <sigh> */
#include <sys/netmgr.h>
#define apd_makedev(maj, min) makedev(ND_LOCAL_NODE, (maj), (min))
#elif defined makedev
/* There's a "makedev" macro. */
#define apd_makedev(maj, min) makedev((maj), (min))
#elif defined mkdev || ((defined _WIN32 || defined __WIN32__) && !defined(__CYGWIN__))
/* Windows. <sigh> */
#define apd_makedev(maj, min) mkdev((maj), (min))
#else
/* There's a "makedev" function. */
#define apd_makedev(maj, min) makedev((maj), (min))
#endif

/* exported */
dev_t pack_native(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;
	if(n == 2) {
		dev = apd_makedev(numbers[0], numbers[1]);
		if((ulong)major(dev) != numbers[0])
			*error = iMajorError;
		else if((ulong)minor(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

static dev_t pack_netbsd(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;

	if(n == 2) {
		dev = makedev_netbsd(numbers[0], numbers[1]);
		if((ulong)major_netbsd(dev) != numbers[0])
			*error = iMajorError;
		else if((ulong)minor_netbsd(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_freebsd(x)        ((int32_t)(((x) & 0x0000ff00) >> 8))
#define minor_freebsd(x)        ((int32_t)(((x) & 0xffff00ff) >> 0))
#define makedev_freebsd(x, y)    ((dev_t)((((x) << 8) & 0x0000ff00) | \
	(((y) << 0) & 0xffff00ff)))

static dev_t pack_freebsd(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;

	if(n == 2) {
		dev = makedev_freebsd(numbers[0], numbers[1]);
		if((ulong)major_freebsd(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_freebsd(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_8_8(x)            ((int32_t)(((x) & 0x0000ff00) >> 8))
#define minor_8_8(x)            ((int32_t)(((x) & 0x000000ff) >> 0))
#define makedev_8_8(x, y)        ((dev_t)((((x) << 8) & 0x0000ff00) | \
	(((y) << 0) & 0x000000ff)))

static dev_t pack_8_8(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;

	if(n == 2) {
		dev = makedev_8_8(numbers[0], numbers[1]);
		if((ulong)major_8_8(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_8_8(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_12_20(x)          ((int32_t)(((x) & 0xfff00000) >> 20))
#define minor_12_20(x)          ((int32_t)(((x) & 0x000fffff) >>  0))
#define makedev_12_20(x, y)      ((dev_t)((((x) << 20) & 0xfff00000) | \
	(((y) <<  0) & 0x000fffff)))

static dev_t pack_12_20(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;

	if(n == 2) {
		dev = makedev_12_20(numbers[0], numbers[1]);
		if((ulong)major_12_20(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_12_20(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_14_18(x)          ((int32_t)(((x) & 0xfffc0000) >> 18))
#define minor_14_18(x)          ((int32_t)(((x) & 0x0003ffff) >>  0))
#define makedev_14_18(x, y)      ((dev_t)((((x) << 18) & 0xfffc0000) | \
	(((y) <<  0) & 0x0003ffff)))

static dev_t pack_14_18(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;

	if(n == 2) {
		dev = makedev_14_18(numbers[0], numbers[1]);
		if((ulong)major_14_18(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_14_18(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_8_24(x)           ((int32_t)(((x) & 0xff000000) >> 24))
#define minor_8_24(x)           ((int32_t)(((x) & 0x00ffffff) >>  0))
#define makedev_8_24(x, y)       ((dev_t)((((x) << 24) & 0xff000000) | \
	(((y) <<  0) & 0x00ffffff)))

static dev_t pack_8_24(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;
	if(n == 2) {
		dev = makedev_8_24(numbers[0], numbers[1]);
		if((ulong)major_8_24(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_8_24(dev) != numbers[1])
			*error = iMinorError;
	}
	else
		*error = tooManyFields;
	return (dev);
}

#define major_12_12_8(x)        ((int32_t)(((x) & 0xfff00000) >> 20))
#define unit_12_12_8(x)         ((int32_t)(((x) & 0x000fff00) >>  8))
#define subunit_12_12_8(x)      ((int32_t)(((x) & 0x000000ff) >>  0))
#define makedev_12_12_8(x, y, z)  ((dev_t)((((x) << 20) & 0xfff00000) | (((y) <<  8) & 0x000fff00) | (((z) <<  0) & 0x000000ff)))

static dev_t pack_bsdos(int n, ulong numbers[], const char ** error)
{
	dev_t dev = 0;
	if(n == 2) {
		dev = makedev_12_20(numbers[0], numbers[1]);
		if((ulong)major_12_20(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)minor_12_20(dev) != numbers[1])
			*error = iMinorError;
	}
	else if(n == 3) {
		dev = makedev_12_12_8(numbers[0], numbers[1], numbers[2]);
		if((ulong)major_12_12_8(dev) != numbers[0])
			*error = iMajorError;
		if((ulong)unit_12_12_8(dev) != numbers[1])
			*error = "invalid unit number";
		if((ulong)subunit_12_12_8(dev) != numbers[2])
			*error = "invalid subunit number";
	}
	else
		*error = tooManyFields;
	return (dev);
}

/* list of formats and pack functions */
/* this list must be sorted lexically */
static const struct format {
	const char      * name;
	pack_t          * pack;
} formats[] = {
	{"386bsd",  pack_8_8},
	{"4bsd",    pack_8_8},
	{"bsdos",   pack_bsdos},
	{"freebsd", pack_freebsd},
	{"hpux",    pack_8_24},
	{"isc",     pack_8_8},
	{"linux",   pack_8_8},
	{"native",  pack_native},
	{"netbsd",  pack_netbsd},
	{"osf1",    pack_12_20},
	{"sco",     pack_8_8},
	{"solaris", pack_14_18},
	{"sunos",   pack_8_8},
	{"svr3",    pack_8_8},
	{"svr4",    pack_14_18},
	{"ultrix",  pack_8_8},
};

static int compare_format(const void * key, const void * element)
{
	const char * name = static_cast<const char *>(key);
	const struct format * format = static_cast<const struct format *>(element);
	return (strcmp(name, format->name));
}

pack_t * pack_find(const char * name)
{
	struct format * format = static_cast<struct format *>(bsearch(name, formats, SIZEOFARRAY(formats), sizeof(formats[0]), compare_format));
	return format ? format->pack : 0;
}
