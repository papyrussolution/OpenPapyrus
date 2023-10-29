// archive_entry_stat.c
// Copyright (c) 2003-2007 Tim Kientzle All rights reserved.
// 
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_entry_stat.c 201100 2009-12-28 03:05:31Z kientzle $");
//#include "archive_entry_private.h"

const struct stat * archive_entry_stat(ArchiveEntry * entry)                     
{
	struct stat * st;
	if(entry->stat == NULL) {
		entry->stat = SAlloc::C(1, sizeof(*st));
		if(entry->stat == NULL)
			return NULL;
		entry->stat_valid = 0;
	}
	/*
	 * If none of the underlying fields have been changed, we
	 * don't need to regenerate.  In theory, we could use a bitmap
	 * here to flag only those items that have changed, but the
	 * extra complexity probably isn't worth it.  It will be very
	 * rare for anyone to change just one field then request a new
	 * stat structure.
	 */
	if(entry->stat_valid)
		return static_cast<const struct stat *>(entry->stat);
	st = (struct stat *)entry->stat;
	/*
	 * Use the public interfaces to extract items, so that
	 * the appropriate conversions get invoked.
	 */
	st->st_atime = archive_entry_atime(entry);
#if HAVE_STRUCT_STAT_ST_BIRTHTIME
	st->st_birthtime = archive_entry_birthtime(entry);
#endif
	st->st_ctime = archive_entry_ctime(entry);
	st->st_mtime = archive_entry_mtime(entry);
	st->st_dev = archive_entry_dev(entry);
	st->st_gid = (gid_t)archive_entry_gid(entry);
	st->st_uid = (uid_t)archive_entry_uid(entry);
	st->st_ino = (ino_t)archive_entry_ino64(entry);
	st->st_nlink = archive_entry_nlink(entry);
	st->st_rdev = archive_entry_rdev(entry);
	st->st_size = (off_t)archive_entry_size(entry);
	st->st_mode = archive_entry_mode(entry);

	/*
	 * On systems that support high-res timestamps, copy that
	 * information into struct stat.
	 */
#if HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC
	st->st_atimespec.tv_nsec = archive_entry_atime_nsec(entry);
	st->st_ctimespec.tv_nsec = archive_entry_ctime_nsec(entry);
	st->st_mtimespec.tv_nsec = archive_entry_mtime_nsec(entry);
#elif HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC
	st->st_atim.tv_nsec = archive_entry_atime_nsec(entry);
	st->st_ctim.tv_nsec = archive_entry_ctime_nsec(entry);
	st->st_mtim.tv_nsec = archive_entry_mtime_nsec(entry);
#elif HAVE_STRUCT_STAT_ST_MTIME_N
	st->st_atime_n = archive_entry_atime_nsec(entry);
	st->st_ctime_n = archive_entry_ctime_nsec(entry);
	st->st_mtime_n = archive_entry_mtime_nsec(entry);
#elif HAVE_STRUCT_STAT_ST_UMTIME
	st->st_uatime = archive_entry_atime_nsec(entry) / 1000;
	st->st_uctime = archive_entry_ctime_nsec(entry) / 1000;
	st->st_umtime = archive_entry_mtime_nsec(entry) / 1000;
#elif HAVE_STRUCT_STAT_ST_MTIME_USEC
	st->st_atime_usec = archive_entry_atime_nsec(entry) / 1000;
	st->st_ctime_usec = archive_entry_ctime_nsec(entry) / 1000;
	st->st_mtime_usec = archive_entry_mtime_nsec(entry) / 1000;
#endif
#if HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC
	st->st_birthtimespec.tv_nsec = archive_entry_birthtime_nsec(entry);
#endif
	/*
	 * TODO: On Linux, store 32 or 64 here depending on whether
	 * the cached stat structure is a stat32 or a stat64.  This
	 * will allow us to support both variants interchangeably.
	 */
	entry->stat_valid = 1;
	return (st);
}
