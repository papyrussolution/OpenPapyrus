/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * Copyright (c) 2012 Michihiro NAKAJIMA
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

/* A table that maps names to functions. */
static const struct { const char * name; int (* setter)(Archive *); } names[] = {
	{ "b64encode",          archive_write_add_filter_b64encode },
	{ "bzip2",              archive_write_add_filter_bzip2 },
	{ "compress",           archive_write_add_filter_compress },
	{ "grzip",              archive_write_add_filter_grzip },
	{ "gzip",               archive_write_add_filter_gzip },
	{ "lrzip",              archive_write_add_filter_lrzip },
	{ "lz4",                archive_write_add_filter_lz4 },
	{ "lzip",               archive_write_add_filter_lzip },
	{ "lzma",               archive_write_add_filter_lzma },
	{ "lzop",               archive_write_add_filter_lzop },
	{ "uuencode",           archive_write_add_filter_uuencode },
	{ "xz",                 archive_write_add_filter_xz },
	{ "zstd",               archive_write_add_filter_zstd },
	{ NULL,                 NULL }
};

int archive_write_add_filter_by_name(Archive * a, const char * name)
{
	for(int i = 0; names[i].name; i++) {
		if(strcmp(name, names[i].name) == 0)
			return ((names[i].setter)(a));
	}
	archive_set_error(a, EINVAL, "No such filter '%s'", name);
	a->state = ARCHIVE_STATE_FATAL;
	return ARCHIVE_FATAL;
}
