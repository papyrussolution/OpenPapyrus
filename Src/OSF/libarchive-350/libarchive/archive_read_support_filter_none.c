/*-
 * Copyright (c) 2003-2007 Tim Kientzle All rights reserved.
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

#if ARCHIVE_VERSION_NUMBER < 4000000
// Deprecated; remove in libarchive 4.0 
int archive_read_support_compression_none(struct archive * a)
{
	return archive_read_support_filter_none(a);
}
#endif
/*
 * Uncompressed streams are handled implicitly by the read core,
 * so this is now a no-op.
 */
int archive_read_support_filter_none(struct archive * a)
{
	archive_check_magic(a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, "archive_read_support_filter_none");
	return ARCHIVE_OK;
}
