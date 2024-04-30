/*-
 * Copyright (c) 2003-2007 Tim Kientzle All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: src/lib/libarchive/archive_read_extract.c,v 1.61 2008/05/26 17:00:22 kientzle Exp $");

int archive_read_extract(Archive * _a, ArchiveEntry * entry, int flags)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	struct archive_read_extract * extract = __archive_read_get_extract(a);
	if(extract == NULL)
		return ARCHIVE_FATAL;
	/* If we haven't initialized the archive_write_disk object, do it now. */
	if(extract->ad == NULL) {
		extract->ad = archive_write_disk_new();
		if(extract->ad == NULL) {
			archive_set_error(&a->archive, ENOMEM, "Can't extract");
			return ARCHIVE_FATAL;
		}
		archive_write_disk_set_standard_lookup(extract->ad);
	}
	archive_write_disk_set_options(extract->ad, flags);
	return (archive_read_extract2(&a->archive, entry, extract->ad));
}
