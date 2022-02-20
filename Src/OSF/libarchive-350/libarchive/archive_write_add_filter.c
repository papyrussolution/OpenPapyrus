/*-
 * Copyright (c) 2012 Ondrej Holy
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
__FBSDID("$FreeBSD$");

#include "archive.h"
#include "archive_private.h"

/* A table that maps filter codes to functions. */
static const
struct { int code; int (* setter)(struct archive *); } codes[] =
{
	{ ARCHIVE_FILTER_NONE,          archive_write_add_filter_none },
	{ ARCHIVE_FILTER_GZIP,          archive_write_add_filter_gzip },
	{ ARCHIVE_FILTER_BZIP2,         archive_write_add_filter_bzip2 },
	{ ARCHIVE_FILTER_COMPRESS,      archive_write_add_filter_compress },
	{ ARCHIVE_FILTER_GRZIP,         archive_write_add_filter_grzip },
	{ ARCHIVE_FILTER_LRZIP,         archive_write_add_filter_lrzip },
	{ ARCHIVE_FILTER_LZ4,           archive_write_add_filter_lz4 },
	{ ARCHIVE_FILTER_LZIP,          archive_write_add_filter_lzip },
	{ ARCHIVE_FILTER_LZMA,          archive_write_add_filter_lzma },
	{ ARCHIVE_FILTER_LZOP,          archive_write_add_filter_lzip },
	{ ARCHIVE_FILTER_UU,            archive_write_add_filter_uuencode },
	{ ARCHIVE_FILTER_XZ,            archive_write_add_filter_xz },
	{ ARCHIVE_FILTER_ZSTD,          archive_write_add_filter_zstd },
	{ -1,                   NULL }
};

int archive_write_add_filter(struct archive * a, int code)
{
	int i;
	for(i = 0; codes[i].code != -1; i++) {
		if(code == codes[i].code)
			return ((codes[i].setter)(a));
	}
	archive_set_error(a, EINVAL, "No such filter");
	return ARCHIVE_FATAL;
}
