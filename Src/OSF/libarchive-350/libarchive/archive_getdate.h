/*-
 * Copyright (c) 2003-2015 Tim Kientzle
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
#ifndef ARCHIVE_GETDATE_H_INCLUDED
#define ARCHIVE_GETDATE_H_INCLUDED
#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif
//#include <time.h>

time_t __archive_get_date(time_t now, const char *);

#endif
