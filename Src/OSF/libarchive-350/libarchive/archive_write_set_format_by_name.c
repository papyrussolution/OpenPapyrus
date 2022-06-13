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
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_set_format_by_name.c 201168 2009-12-29 06:15:32Z kientzle $");

/* A table that maps names to functions. */
static const
struct { const char * name; int (* setter)(Archive *); } names[] =
{
	{ "7zip",       archive_write_set_format_7zip },
	{ "ar",         archive_write_set_format_ar_bsd },
	{ "arbsd",      archive_write_set_format_ar_bsd },
	{ "argnu",      archive_write_set_format_ar_svr4 },
	{ "arsvr4",     archive_write_set_format_ar_svr4 },
	{ "bsdtar",     archive_write_set_format_pax_restricted },
	{ "cd9660",     archive_write_set_format_iso9660 },
	{ "cpio",       archive_write_set_format_cpio },
	{ "gnutar",     archive_write_set_format_gnutar },
	{ "iso",        archive_write_set_format_iso9660 },
	{ "iso9660",    archive_write_set_format_iso9660 },
	{ "mtree",      archive_write_set_format_mtree },
	{ "mtree-classic",      archive_write_set_format_mtree_classic },
	{ "newc",       archive_write_set_format_cpio_newc },
	{ "odc",        archive_write_set_format_cpio },
	{ "oldtar",     archive_write_set_format_v7tar },
	{ "pax",        archive_write_set_format_pax },
	{ "paxr",       archive_write_set_format_pax_restricted },
	{ "posix",      archive_write_set_format_pax },
	{ "raw",        archive_write_set_format_raw },
	{ "rpax",       archive_write_set_format_pax_restricted },
	{ "shar",       archive_write_set_format_shar },
	{ "shardump",   archive_write_set_format_shar_dump },
	{ "ustar",      archive_write_set_format_ustar },
	{ "v7tar",      archive_write_set_format_v7tar },
	{ "v7",         archive_write_set_format_v7tar },
	{ "warc",       archive_write_set_format_warc },
	{ "xar",        archive_write_set_format_xar },
	{ "zip",        archive_write_set_format_zip },
	{ NULL,         NULL }
};

int archive_write_set_format_by_name(Archive * a, const char * name)
{
	for(int i = 0; names[i].name; i++) {
		if(sstreq(name, names[i].name))
			return ((names[i].setter)(a));
	}
	archive_set_error(a, EINVAL, "No such format '%s'", name);
	a->state = ARCHIVE_STATE_FATAL;
	return ARCHIVE_FATAL;
}
