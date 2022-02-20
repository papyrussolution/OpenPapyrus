/*-
 * Copyright (c) 2012 Michihiro NAKAJIMA
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
#ifndef ARCHIVE_CMDLINE_PRIVATE_H
#define ARCHIVE_CMDLINE_PRIVATE_H

#ifndef __LIBARCHIVE_BUILD
#ifndef __LIBARCHIVE_TEST
#error This header is only to be used internally to libarchive.
#endif
#endif

struct archive_cmdline {
	char * path;
	char ** argv;
	int argc;
};

struct archive_cmdline * __archive_cmdline_allocate(void);
int __archive_cmdline_parse(struct archive_cmdline *, const char *);
int __archive_cmdline_free(struct archive_cmdline *);

#endif
