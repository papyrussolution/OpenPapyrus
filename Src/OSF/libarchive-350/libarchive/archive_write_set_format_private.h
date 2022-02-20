/*-
 * Copyright (c) 2020 Martin Matuska
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
 * $FreeBSD$
 */
#ifndef ARCHIVE_WRITE_SET_FORMAT_PRIVATE_H_INCLUDED
#define ARCHIVE_WRITE_SET_FORMAT_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#ifndef __LIBARCHIVE_TEST
		#error This header is only to be used internally to libarchive.
	#endif
#endif

#include "archive.h"
#include "archive_entry.h"

void __archive_write_entry_filetype_unsupported(struct archive *a, struct archive_entry *entry, const char *format);
#endif
