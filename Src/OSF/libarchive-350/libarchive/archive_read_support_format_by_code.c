/*-
 * Copyright (c) 2003-2011 Tim Kientzle
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

int archive_read_support_format_by_code(struct archive * a, int format_code)
{
	archive_check_magic(a, ARCHIVE_READ_MAGIC, ARCHIVE_STATE_NEW, "archive_read_support_format_by_code");
	switch(format_code & ARCHIVE_FORMAT_BASE_MASK) {
		case ARCHIVE_FORMAT_7ZIP: return archive_read_support_format_7zip(a); break;
		case ARCHIVE_FORMAT_AR: return archive_read_support_format_ar(a); break;
		case ARCHIVE_FORMAT_CAB: return archive_read_support_format_cab(a); break;
		case ARCHIVE_FORMAT_CPIO: return archive_read_support_format_cpio(a); break;
		case ARCHIVE_FORMAT_EMPTY: return archive_read_support_format_empty(a); break;
		case ARCHIVE_FORMAT_ISO9660: return archive_read_support_format_iso9660(a); break;
		case ARCHIVE_FORMAT_LHA: return archive_read_support_format_lha(a); break;
		case ARCHIVE_FORMAT_MTREE: return archive_read_support_format_mtree(a); break;
		case ARCHIVE_FORMAT_RAR: return archive_read_support_format_rar(a); break;
		case ARCHIVE_FORMAT_RAR_V5: return archive_read_support_format_rar5(a); break;
		case ARCHIVE_FORMAT_RAW: return archive_read_support_format_raw(a); break;
		case ARCHIVE_FORMAT_TAR: return archive_read_support_format_tar(a); break;
		case ARCHIVE_FORMAT_WARC: return archive_read_support_format_warc(a); break;
		case ARCHIVE_FORMAT_XAR: return archive_read_support_format_xar(a); break;
		case ARCHIVE_FORMAT_ZIP: return archive_read_support_format_zip(a); break;
	}
	archive_set_error(a, ARCHIVE_ERRNO_PROGRAMMER, "Invalid format code specified");
	return ARCHIVE_FATAL;
}
