/*-
 * Copyright (c) 2020 Martin Matuska All rights reserved.
 */
#ifndef ARCHIVE_WRITE_SET_FORMAT_PRIVATE_H_INCLUDED
#define ARCHIVE_WRITE_SET_FORMAT_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#ifndef __LIBARCHIVE_TEST
		#error This header is only to be used internally to libarchive.
	#endif
#endif
//#include "archive.h"
//#include "archive_entry.h"
void __archive_write_entry_filetype_unsupported(Archive *a, ArchiveEntry *entry, const char *format);
#endif
