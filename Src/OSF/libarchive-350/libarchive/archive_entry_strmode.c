// archive_entry_strmode.c
// Copyright (c) 2003-2007 Tim Kientzle All rights reserved.
// 
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: src/lib/libarchive/archive_entry_strmode.c,v 1.4 2008/06/15 05:14:01 kientzle Exp $");
//#include "archive_entry_private.h"

const char * FASTCALL archive_entry_strmode(ArchiveEntry * entry)
{
	static const mode_t permbits[] = { 0400, 0200, 0100, 0040, 0020, 0010, 0004, 0002, 0001 };
	char * bp = entry->strmode;
	mode_t mode;
	int i;
	/* Fill in a default string, then selectively override. */
	strcpy(bp, "?rwxrwxrwx ");
	mode = archive_entry_mode(entry);
	switch(archive_entry_filetype(entry)) {
		case AE_IFREG:  bp[0] = '-'; break;
		case AE_IFBLK:  bp[0] = 'b'; break;
		case AE_IFCHR:  bp[0] = 'c'; break;
		case AE_IFDIR:  bp[0] = 'd'; break;
		case AE_IFLNK:  bp[0] = 'l'; break;
		case AE_IFSOCK: bp[0] = 's'; break;
		case AE_IFIFO:  bp[0] = 'p'; break;
		default:
		    if(archive_entry_hardlink(entry)) {
			    bp[0] = 'h';
			    break;
		    }
	}
	for(i = 0; i < 9; i++)
		if(!(mode & permbits[i]))
			bp[i+1] = '-';
	if(mode & S_ISUID) {
		if(mode & 0100) 
			bp[3] = 's';
		else 
			bp[3] = 'S';
	}
	if(mode & S_ISGID) {
		if(mode & 0010) 
			bp[6] = 's';
		else 
			bp[6] = 'S';
	}
	if(mode & S_ISVTX) {
		if(mode & 0001) 
			bp[9] = 't';
		else 
			bp[9] = 'T';
	}
	if(archive_entry_acl_types(entry) != 0)
		bp[10] = '+';
	return (bp);
}
