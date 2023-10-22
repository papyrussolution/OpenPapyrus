/*-
 * Copyright (c) 2011 Tim Kientzle
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
__FBSDID("$FreeBSD$");
//#include "archive_read_private.h"
//#include "archive_options_private.h"

static int archive_set_format_option(Archive * a, const char * m, const char * o, const char * v);
static int archive_set_filter_option(Archive * a, const char * m, const char * o, const char * v);
static int archive_set_option(Archive * a, const char * m, const char * o, const char * v);

int archive_read_set_format_option(Archive * a, const char * m, const char * o, const char * v)
{
	return _archive_set_option(a, m, o, v, ARCHIVE_READ_MAGIC, __FUNCTION__, archive_set_format_option);
}

int archive_read_set_filter_option(Archive * a, const char * m, const char * o, const char * v)
{
	return _archive_set_option(a, m, o, v, ARCHIVE_READ_MAGIC, __FUNCTION__, archive_set_filter_option);
}

int archive_read_set_option(Archive * a, const char * m, const char * o, const char * v)
{
	return _archive_set_option(a, m, o, v, ARCHIVE_READ_MAGIC, __FUNCTION__, archive_set_option);
}

int archive_read_set_options(Archive * a, const char * options)
{
	return _archive_set_options(a, options, ARCHIVE_READ_MAGIC, __FUNCTION__, archive_set_option);
}

static int archive_set_format_option(Archive * _a, const char * m, const char * o, const char * v)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	size_t i;
	int r, rv = ARCHIVE_WARN, matched_modules = 0;
	for(i = 0; i < SIZEOFARRAY(a->formats); i++) {
		ArchiveRead::archive_format_descriptor * format = &a->formats[i];
		if(format->options == NULL || format->name == NULL)
			/* This format does not support option. */
			continue;
		if(m != NULL) {
			if(strcmp(format->name, m) != 0)
				continue;
			++matched_modules;
		}
		a->format = format;
		r = format->options(a, o, v);
		a->format = NULL;
		if(r == ARCHIVE_FATAL)
			return ARCHIVE_FATAL;
		if(r == ARCHIVE_OK)
			rv = ARCHIVE_OK;
	}
	/* If the format name didn't match, return a special code for
	 * _archive_set_option[s]. */
	if(m != NULL && matched_modules == 0)
		return ARCHIVE_WARN - 1;
	return (rv);
}

static int archive_set_filter_option(Archive * _a, const char * m, const char * o, const char * v)
{
	ArchiveRead * a = reinterpret_cast<ArchiveRead *>(_a);
	ArchiveReadFilter * filter;
	ArchiveReadFilterBidder * bidder;
	int r, rv = ARCHIVE_WARN, matched_modules = 0;
	for(filter = a->filter; filter != NULL; filter = filter->upstream) {
		bidder = filter->bidder;
		if(bidder == NULL)
			continue;
		if(!bidder->FnOptions)
			continue; // This bidder does not support option
		if(m != NULL) {
			if(strcmp(filter->name, m) != 0)
				continue;
			++matched_modules;
		}
		r = bidder->FnOptions(bidder, o, v);
		if(r == ARCHIVE_FATAL)
			return ARCHIVE_FATAL;
		if(r == ARCHIVE_OK)
			rv = ARCHIVE_OK;
	}
	// If the filter name didn't match, return a special code for _archive_set_option[s]
	if(m && matched_modules == 0)
		return ARCHIVE_WARN - 1;
	return (rv);
}

static int archive_set_option(Archive * a, const char * m, const char * o, const char * v)
{
	return _archive_set_either_option(a, m, o, v, archive_set_format_option, archive_set_filter_option);
}
