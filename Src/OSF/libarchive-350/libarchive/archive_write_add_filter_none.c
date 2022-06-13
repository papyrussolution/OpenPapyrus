/*-
 * Copyright (c) 2003-2010 Tim Kientzle
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
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_set_compression_none.c 201080 2009-12-28 02:03:54Z kientzle $");

int archive_write_set_compression_none(Archive * a)
{
	CXX_UNUSED(a);
	return ARCHIVE_OK;
}

int archive_write_add_filter_none(Archive * a)
{
	CXX_UNUSED(a);
	return ARCHIVE_OK;
}
