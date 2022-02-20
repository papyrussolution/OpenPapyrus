/*-
 * Copyright (c) 2007 Joerg Sonnenberger
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
 * $FreeBSD: head/lib/libarchive/filter_fork.h 201087 2009-12-28 02:18:26Z kientzle $
 */
#ifndef FILTER_FORK_H
#define FILTER_FORK_H

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

int __archive_create_child(const char *cmd, int *child_stdin, int *child_stdout,
#if defined(_WIN32) && !defined(__CYGWIN__)
	HANDLE *out_child);
#else
	pid_t *out_child);
#endif
void __archive_check_child(int in, int out);

#endif
