/*-
 * Copyright (c) 2003-2012 Tim Kientzle
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
#include "archive_read_private.h"

int archive_read_set_format(Archive * _a, int code)
{
	int r1, r2, i;
	//char str[10];
	const char * p_str = 0;
	ArchiveRead * a = (ArchiveRead *)_a;
	if((r1 = archive_read_support_format_by_code(_a, code)) < (ARCHIVE_OK))
		return r1;
	r1 = r2 = (ARCHIVE_OK);
	if(a->format)
		r2 = (ARCHIVE_WARN);
	switch(code & ARCHIVE_FORMAT_BASE_MASK) {
		case ARCHIVE_FORMAT_7ZIP: p_str = "7zip"; break;
		case ARCHIVE_FORMAT_AR: p_str = "ar"; break;
		case ARCHIVE_FORMAT_CAB: p_str = "cab"; break;
		case ARCHIVE_FORMAT_CPIO: p_str = "cpio"; break;
		case ARCHIVE_FORMAT_EMPTY: p_str = "empty"; break;
		case ARCHIVE_FORMAT_ISO9660: p_str = "iso9660"; break;
		case ARCHIVE_FORMAT_LHA: p_str = "lha"; break;
		case ARCHIVE_FORMAT_MTREE: p_str = "mtree"; break;
		case ARCHIVE_FORMAT_RAR: p_str = "rar"; break;
		case ARCHIVE_FORMAT_RAR_V5: p_str = "rar5"; break;
		case ARCHIVE_FORMAT_RAW: p_str = "raw"; break;
		case ARCHIVE_FORMAT_TAR: p_str = "tar"; break;
		case ARCHIVE_FORMAT_WARC: p_str = "warc"; break;
		case ARCHIVE_FORMAT_XAR: p_str = "xar"; break;
		case ARCHIVE_FORMAT_ZIP: p_str = "zip"; break;
		default:
		    archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER, "Invalid format code specified");
		    return ARCHIVE_FATAL;
	}
	assert(p_str);
	a->format = &(a->formats[0]);
	for(i = 0; i < SIZEOFARRAY(a->formats); i++, a->format++) {
		if(!a->format->name || sstreq(a->format->name, p_str))
			break;
	}
	if(!a->format->name || !sstreq(a->format->name, p_str)) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_PROGRAMMER, "Internal error: Unable to set format");
		r1 = (ARCHIVE_FATAL);
	}
	return (r1 < r2) ? r1 : r2;
}
