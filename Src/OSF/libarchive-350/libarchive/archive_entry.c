/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * Copyright (c) 2016 Martin Matuska
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_entry.c 201096 2009-12-28 02:41:27Z kientzle $");

#if MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#define HAVE_MAJOR
#elif MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#define HAVE_MAJOR
#endif
#ifdef HAVE_LINUX_FS_H
#include <linux/fs.h>   /* for Linux file flags */
#endif
/*
 * Some Linux distributions have both linux/ext2_fs.h and ext2fs/ext2_fs.h.
 * As the include guards don't agree, the order of include is important.
 */
#ifdef HAVE_LINUX_EXT2_FS_H
#include <linux/ext2_fs.h>      /* for Linux file flags */
#endif
#if defined(HAVE_EXT2FS_EXT2_FS_H) && !defined(__CYGWIN__)
#include <ext2fs/ext2_fs.h>     /* for Linux file flags */
#endif
#include "archive_acl_private.h"
#include "archive_entry_locale.h"
#include "archive_entry_private.h"

#if !defined(HAVE_MAJOR) && !defined(major)
/* Replacement for major/minor/makedev. */
#define major(x) ((int)(0x00ff & ((x) >> 8)))
#define minor(x) ((int)(0xffff00ff & (x)))
#define makedev(maj, min) ((0xff00 & ((maj)<<8)) | (0xffff00ff & (min)))
#endif

/* Play games to come up with a suitable makedev() definition. */
#ifdef __QNXNTO__
/* QNX.  <sigh> */
#include <sys/netmgr.h>
#define ae_makedev(maj, min) makedev(ND_LOCAL_NODE, (maj), (min))
#elif defined makedev
/* There's a "makedev" macro. */
#define ae_makedev(maj, min) makedev((maj), (min))
#elif defined mkdev || ((defined _WIN32 || defined __WIN32__) && !defined(__CYGWIN__))
/* Windows. <sigh> */
#define ae_makedev(maj, min) mkdev((maj), (min))
#else
/* There's a "makedev" function. */
#define ae_makedev(maj, min) makedev((maj), (min))
#endif

/*
 * This adjustment is needed to support the following idiom for adding
 * 1000ns to the stored time:
 * archive_entry_set_atime(archive_entry_atime(),
 *                   archive_entry_atime_nsec() + 1000)
 * The additional if() here compensates for ambiguity in the C standard,
 * which permits two possible interpretations of a % b when a is negative.
 */
#define FIX_NS(t, ns) \
	do {    \
		t += ns / 1000000000; \
		ns %= 1000000000; \
		if(ns < 0) { --t; ns += 1000000000; } \
	} while(0)

static char *    ae_fflagstostr(ulong bitset, ulong bitclear);
static const wchar_t    * ae_wcstofflags(const wchar_t * stringp, ulong * setp, ulong * clrp);
static const char       * ae_strtofflags(const char * stringp, ulong * setp, ulong * clrp);

#ifndef HAVE_WCSCPY
static wchar_t * wcscpy(wchar_t * s1, const wchar_t * s2)
{
	wchar_t * dest = s1;
	while((*s1 = *s2) != L'\0')
		++s1, ++s2;
	return dest;
}

#endif
#ifndef HAVE_WCSLEN
static size_t wcslen(const wchar_t * s)
{
	const wchar_t * p = s;
	while(*p != L'\0')
		++p;
	return p - s;
}

#endif
#ifndef HAVE_WMEMCMP
/* Good enough for simple equality testing, but not for sorting. */
#define wmemcmp(a, b, i)  memcmp((a), (b), (i) * sizeof(wchar_t))
#endif
// 
// Public Interface
// 
ArchiveEntry * archive_entry_clear(ArchiveEntry * entry)                        
{
	if(entry) {
		archive_mstring_clean(&entry->ae_fflags_text);
		archive_mstring_clean(&entry->ae_gname);
		archive_mstring_clean(&entry->ae_hardlink);
		archive_mstring_clean(&entry->ae_pathname);
		archive_mstring_clean(&entry->ae_sourcepath);
		archive_mstring_clean(&entry->ae_symlink);
		archive_mstring_clean(&entry->ae_uname);
		archive_entry_copy_mac_metadata(entry, NULL, 0);
		archive_acl_clear(&entry->acl);
		archive_entry_xattr_clear(entry);
		archive_entry_sparse_clear(entry);
		SAlloc::F(entry->stat);
		entry->ae_symlink_type = AE_SYMLINK_TYPE_UNDEFINED;
		memzero(entry, sizeof(*entry));
	}
	return entry;
}

ArchiveEntry * archive_entry_clone(ArchiveEntry * entry)                        
{
	struct ae_xattr * xp;
	struct ae_sparse * sp;
	size_t s;
	const void * p;
	/* Allocate new structure and copy over all of the fields. */
	/* TODO: Should we copy the archive over?  Or require a new archive
	 * as an argument? */
	ArchiveEntry * entry2 = archive_entry_new2(entry->archive);
	if(entry2) {
		entry2->ae_stat = entry->ae_stat;
		entry2->ae_fflags_set = entry->ae_fflags_set;
		entry2->ae_fflags_clear = entry->ae_fflags_clear;
		// TODO: XXX If clone can have a different archive, what do we do here if character sets are different? XXX 
		archive_mstring_copy(&entry2->ae_fflags_text, &entry->ae_fflags_text);
		archive_mstring_copy(&entry2->ae_gname, &entry->ae_gname);
		archive_mstring_copy(&entry2->ae_hardlink, &entry->ae_hardlink);
		archive_mstring_copy(&entry2->ae_pathname, &entry->ae_pathname);
		archive_mstring_copy(&entry2->ae_sourcepath, &entry->ae_sourcepath);
		archive_mstring_copy(&entry2->ae_symlink, &entry->ae_symlink);
		entry2->ae_set = entry->ae_set;
		archive_mstring_copy(&entry2->ae_uname, &entry->ae_uname);
		// Copy symlink type 
		entry2->ae_symlink_type = entry->ae_symlink_type;
		// Copy encryption status 
		entry2->encryption = entry->encryption;
		// Copy digests 
	#define copy_digest(_e2, _e, _t) memcpy(_e2->digest._t, _e->digest._t, sizeof(_e2->digest._t))
		copy_digest(entry2, entry, md5);
		copy_digest(entry2, entry, rmd160);
		copy_digest(entry2, entry, sha1);
		copy_digest(entry2, entry, sha256);
		copy_digest(entry2, entry, sha384);
		copy_digest(entry2, entry, sha512);
	#undef copy_digest
		// Copy ACL data over
		archive_acl_copy(&entry2->acl, &entry->acl);
		// Copy Mac OS metadata
		p = archive_entry_mac_metadata(entry, &s);
		archive_entry_copy_mac_metadata(entry2, p, s);
		// Copy xattr data over
		for(xp = entry->xattr_head; xp; xp = xp->next) {
			archive_entry_xattr_add_entry(entry2, xp->name, xp->value, xp->size);
		}
		// Copy sparse data over
		for(sp = entry->sparse_head; sp; sp = sp->next) {
			archive_entry_sparse_add_entry(entry2, sp->offset, sp->length);
		}
	}
	return entry2;
}

void FASTCALL archive_entry_free(ArchiveEntry * entry)
{
	if(entry) {
		archive_entry_clear(entry);
		SAlloc::F(entry);
	}
}

ArchiveEntry * archive_entry_new(void)
{
	return archive_entry_new2(NULL);
}

ArchiveEntry * archive_entry_new2(Archive * a)                        
{
	ArchiveEntry * entry = (ArchiveEntry *)SAlloc::C(1, sizeof(*entry));
	if(entry) {
		entry->archive = a;
		entry->ae_symlink_type = AE_SYMLINK_TYPE_UNDEFINED;
	}
	return entry;
}
/*
 * Functions for reading fields from an archive_entry.
 */
time_t FASTCALL archive_entry_atime(const ArchiveEntry * entry) { return static_cast<time_t>(entry->ae_stat.aest_atime); }
long   FASTCALL archive_entry_atime_nsec(const ArchiveEntry * entry) { return (entry->ae_stat.aest_atime_nsec); }
int    FASTCALL archive_entry_atime_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_ATIME); }
time_t FASTCALL archive_entry_birthtime(const ArchiveEntry * entry) { return static_cast<time_t>(entry->ae_stat.aest_birthtime); }
long   FASTCALL archive_entry_birthtime_nsec(const ArchiveEntry * entry) { return (entry->ae_stat.aest_birthtime_nsec); }
int    FASTCALL archive_entry_birthtime_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_BIRTHTIME); }
time_t FASTCALL archive_entry_ctime(const ArchiveEntry * entry) { return static_cast<time_t>(entry->ae_stat.aest_ctime); }
int    FASTCALL archive_entry_ctime_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_CTIME); }
long   FASTCALL archive_entry_ctime_nsec(const ArchiveEntry * entry) { return (entry->ae_stat.aest_ctime_nsec); }
dev_t  FASTCALL archive_entry_dev(const ArchiveEntry * entry) { return (entry->ae_stat.aest_dev_is_broken_down) ? ae_makedev(entry->ae_stat.aest_devmajor, entry->ae_stat.aest_devminor) : (entry->ae_stat.aest_dev); }
int    FASTCALL archive_entry_dev_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_DEV); }
dev_t  FASTCALL archive_entry_devmajor(const ArchiveEntry * entry) { return (entry->ae_stat.aest_dev_is_broken_down) ? (entry->ae_stat.aest_devmajor) : major(entry->ae_stat.aest_dev); }
dev_t  FASTCALL archive_entry_devminor(const ArchiveEntry * entry) { return (entry->ae_stat.aest_dev_is_broken_down) ? (entry->ae_stat.aest_devminor) : minor(entry->ae_stat.aest_dev); }
__LA_MODE_T FASTCALL archive_entry_filetype(const ArchiveEntry * entry) { return (AE_IFMT & entry->acl.mode); }

void archive_entry_fflags(const ArchiveEntry * entry, ulong * set, ulong * clear)
{
	*set = entry->ae_fflags_set;
	*clear = entry->ae_fflags_clear;
}
/*
 * Note: if text was provided, this just returns that text.  If you
 * really need the text to be rebuilt in a canonical form, set the
 * text, ask for the bitmaps, then set the bitmaps.  (Setting the
 * bitmaps clears any stored text.)  This design is deliberate: if
 * we're editing archives, we don't want to discard flags just because
 * they aren't supported on the current system.  The bitmap<->text
 * conversions are platform-specific (see below).
 */
const char * FASTCALL archive_entry_fflags_text(ArchiveEntry * entry)
{
	const char * f = 0;
	char * p;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_fflags_text, &f) == 0) {
		if(f)
			return (f);
	}
	else if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	if(entry->ae_fflags_set == 0 && entry->ae_fflags_clear == 0)
		return NULL;
	p = ae_fflagstostr(entry->ae_fflags_set, entry->ae_fflags_clear);
	if(!p)
		return NULL;
	archive_mstring_copy_mbs(&entry->ae_fflags_text, p);
	SAlloc::F(p);
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_fflags_text, &f) == 0)
		return (f);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

la_int64_t FASTCALL archive_entry_gid(ArchiveEntry * entry)
{
	return (entry->ae_stat.aest_gid);
}

const char * FASTCALL archive_entry_gname(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_gname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const char * FASTCALL archive_entry_gname_utf8(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_utf8(entry->archive, &entry->ae_gname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const wchar_t * FASTCALL archive_entry_gname_w(ArchiveEntry * entry)
{
	const wchar_t * p;
	if(archive_mstring_get_wcs(entry->archive, &entry->ae_gname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

int _archive_entry_gname_l(ArchiveEntry * entry, const char ** p, size_t * len, archive_string_conv * sc)
{
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_gname, p, len, sc));
}

const char * FASTCALL archive_entry_hardlink(ArchiveEntry * entry)
{
	const char * p;
	if((entry->ae_set & AE_SET_HARDLINK) == 0)
		return NULL;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_hardlink, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const char * FASTCALL archive_entry_hardlink_utf8(ArchiveEntry * entry)
{
	const char * p = 0;
	if(entry->ae_set & AE_SET_HARDLINK) {
		if(archive_mstring_get_utf8(entry->archive, &entry->ae_hardlink, &p) != 0) {
			if(errno == ENOMEM)
				__archive_errx(1, SlTxtOutOfMem);
			p = 0;
		}
	}
	return p;
}

const wchar_t * FASTCALL archive_entry_hardlink_w(ArchiveEntry * entry)
{
	const wchar_t * p = 0;
	if(entry->ae_set & AE_SET_HARDLINK) {
		if(archive_mstring_get_wcs(entry->archive, &entry->ae_hardlink, &p) != 0) {
			if(errno == ENOMEM)
				__archive_errx(1, SlTxtOutOfMem);
			p = 0;
		}
	}
	return p;
}

int _archive_entry_hardlink_l(ArchiveEntry * entry, const char ** p, size_t * len, archive_string_conv * sc)
{
	if((entry->ae_set & AE_SET_HARDLINK) == 0) {
		*p = NULL;
		*len = 0;
		return 0;
	}
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_hardlink, p, len, sc));
}

la_int64_t FASTCALL archive_entry_ino(const ArchiveEntry * entry) { return (entry->ae_stat.aest_ino); }
int    FASTCALL archive_entry_ino_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_INO); }
la_int64_t FASTCALL archive_entry_ino64(const ArchiveEntry * entry) { return (entry->ae_stat.aest_ino); }
__LA_MODE_T FASTCALL archive_entry_mode(const ArchiveEntry * entry) { return (entry->acl.mode); }
time_t FASTCALL archive_entry_mtime(const ArchiveEntry * entry) { return static_cast<time_t>(entry->ae_stat.aest_mtime); }
long   FASTCALL archive_entry_mtime_nsec(const ArchiveEntry * entry) { return (entry->ae_stat.aest_mtime_nsec); }
int    FASTCALL archive_entry_mtime_is_set(const ArchiveEntry * entry) { return (entry->ae_set & AE_SET_MTIME); }
uint   FASTCALL archive_entry_nlink(const ArchiveEntry * entry) { return (entry->ae_stat.aest_nlink); }

const char * FASTCALL archive_entry_pathname(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_pathname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const char * FASTCALL archive_entry_pathname_utf8(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_utf8(entry->archive, &entry->ae_pathname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const wchar_t * FASTCALL archive_entry_pathname_w(ArchiveEntry * entry)
{
	const wchar_t * p;
	if(archive_mstring_get_wcs(entry->archive, &entry->ae_pathname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

int _archive_entry_pathname_l(ArchiveEntry * entry, const char ** p, size_t * len, archive_string_conv * sc)
{
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_pathname, p, len, sc));
}

__LA_MODE_T FASTCALL archive_entry_perm(ArchiveEntry * entry)
{
	return (~AE_IFMT & entry->acl.mode);
}

dev_t FASTCALL archive_entry_rdev(ArchiveEntry * entry)
{
	if(entry->ae_stat.aest_rdev_is_broken_down)
		return ae_makedev(entry->ae_stat.aest_rdevmajor, entry->ae_stat.aest_rdevminor);
	else
		return (entry->ae_stat.aest_rdev);
}

dev_t FASTCALL archive_entry_rdevmajor(ArchiveEntry * entry)
{
	if(entry->ae_stat.aest_rdev_is_broken_down)
		return (entry->ae_stat.aest_rdevmajor);
	else
		return major(entry->ae_stat.aest_rdev);
}

dev_t FASTCALL archive_entry_rdevminor(ArchiveEntry * entry)
{
	if(entry->ae_stat.aest_rdev_is_broken_down)
		return (entry->ae_stat.aest_rdevminor);
	else
		return minor(entry->ae_stat.aest_rdev);
}

la_int64_t FASTCALL archive_entry_size(ArchiveEntry * entry)
{
	return (entry->ae_stat.aest_size);
}

int FASTCALL archive_entry_size_is_set(ArchiveEntry * entry)
{
	return (entry->ae_set & AE_SET_SIZE);
}

const char * FASTCALL archive_entry_sourcepath(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_sourcepath, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const wchar_t * FASTCALL archive_entry_sourcepath_w(ArchiveEntry * entry)
{
	const wchar_t * p;
	if(archive_mstring_get_wcs(entry->archive, &entry->ae_sourcepath, &p) == 0)
		return (p);
	return NULL;
}

const char * FASTCALL archive_entry_symlink(ArchiveEntry * entry)
{
	const char * p;
	if((entry->ae_set & AE_SET_SYMLINK) == 0)
		return NULL;
	if(archive_mstring_get_mbs(
		    entry->archive, &entry->ae_symlink, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

int FASTCALL archive_entry_symlink_type(ArchiveEntry * entry)
{
	return (entry->ae_symlink_type);
}

const char * FASTCALL archive_entry_symlink_utf8(ArchiveEntry * entry)
{
	const char * p;
	if((entry->ae_set & AE_SET_SYMLINK) == 0)
		return NULL;
	if(archive_mstring_get_utf8(entry->archive, &entry->ae_symlink, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const wchar_t * FASTCALL archive_entry_symlink_w(ArchiveEntry * entry)
{
	const wchar_t * p;
	if((entry->ae_set & AE_SET_SYMLINK) == 0)
		return NULL;
	if(archive_mstring_get_wcs(entry->archive, &entry->ae_symlink, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

int _archive_entry_symlink_l(ArchiveEntry * entry, const char ** p, size_t * len, archive_string_conv * sc)
{
	if((entry->ae_set & AE_SET_SYMLINK) == 0) {
		*p = NULL;
		*len = 0;
		return 0;
	}
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_symlink, p, len, sc));
}

la_int64_t FASTCALL archive_entry_uid(const ArchiveEntry * entry)
{
	return (entry->ae_stat.aest_uid);
}

const char * FASTCALL archive_entry_uname(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_mbs(entry->archive, &entry->ae_uname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const char * FASTCALL archive_entry_uname_utf8(ArchiveEntry * entry)
{
	const char * p;
	if(archive_mstring_get_utf8(entry->archive, &entry->ae_uname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

const wchar_t * FASTCALL archive_entry_uname_w(ArchiveEntry * entry)
{
	const wchar_t * p;
	if(archive_mstring_get_wcs(entry->archive, &entry->ae_uname, &p) == 0)
		return (p);
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return NULL;
}

int _archive_entry_uname_l(ArchiveEntry * entry, const char ** p, size_t * len, archive_string_conv * sc)
{
	return (archive_mstring_get_mbs_l(entry->archive, &entry->ae_uname, p, len, sc));
}

int FASTCALL archive_entry_is_data_encrypted(ArchiveEntry * entry)
{
	return ((entry->encryption & AE_ENCRYPTION_DATA) == AE_ENCRYPTION_DATA);
}

int FASTCALL archive_entry_is_metadata_encrypted(ArchiveEntry * entry)
{
	return ((entry->encryption & AE_ENCRYPTION_METADATA) == AE_ENCRYPTION_METADATA);
}

int FASTCALL archive_entry_is_encrypted(ArchiveEntry * entry)
{
	return (entry->encryption & (AE_ENCRYPTION_DATA|AE_ENCRYPTION_METADATA));
}
/*
 * Functions to set archive_entry properties.
 */
void FASTCALL archive_entry_set_filetype(ArchiveEntry * entry, uint type)
{
	entry->stat_valid = 0;
	entry->acl.mode &= ~AE_IFMT;
	entry->acl.mode |= AE_IFMT & type;
}

void archive_entry_set_fflags(ArchiveEntry * entry, ulong set, ulong clear)
{
	archive_mstring_clean(&entry->ae_fflags_text);
	entry->ae_fflags_set = set;
	entry->ae_fflags_clear = clear;
}

const char * archive_entry_copy_fflags_text(ArchiveEntry * entry, const char * flags)
{
	archive_mstring_copy_mbs(&entry->ae_fflags_text, flags);
	return (ae_strtofflags(flags, &entry->ae_fflags_set, &entry->ae_fflags_clear));
}

const wchar_t * archive_entry_copy_fflags_text_w(ArchiveEntry * entry, const wchar_t * flags)
{
	archive_mstring_copy_wcs(&entry->ae_fflags_text, flags);
	return (ae_wcstofflags(flags, &entry->ae_fflags_set, &entry->ae_fflags_clear));
}

void archive_entry_set_gid(ArchiveEntry * entry, la_int64_t g)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_gid = g;
}

void archive_entry_set_gname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_gname, name);
}

void archive_entry_set_gname_utf8(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_utf8(&entry->ae_gname, name);
}

void archive_entry_copy_gname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_gname, name);
}

void archive_entry_copy_gname_w(ArchiveEntry * entry, const wchar_t * name)
{
	archive_mstring_copy_wcs(&entry->ae_gname, name);
}

int archive_entry_update_gname_utf8(ArchiveEntry * entry, const char * name)
{
	if(archive_mstring_update_utf8(entry->archive,
	    &entry->ae_gname, name) == 0)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

int _archive_entry_copy_gname_l(ArchiveEntry * entry, const char * name, size_t len, archive_string_conv * sc)
{
	return (archive_mstring_copy_mbs_len_l(&entry->ae_gname, name, len, sc));
}

void archive_entry_set_ino(ArchiveEntry * entry, la_int64_t ino)
{
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_INO;
	entry->ae_stat.aest_ino = ino;
}

void archive_entry_set_ino64(ArchiveEntry * entry, la_int64_t ino)
{
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_INO;
	entry->ae_stat.aest_ino = ino;
}

void archive_entry_set_hardlink(ArchiveEntry * entry, const char * target)
{
	archive_mstring_copy_mbs(&entry->ae_hardlink, target);
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, target);
}

void archive_entry_set_hardlink_utf8(ArchiveEntry * entry, const char * target)
{
	archive_mstring_copy_utf8(&entry->ae_hardlink, target);
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, target);
}

void archive_entry_copy_hardlink(ArchiveEntry * entry, const char * target)
{
	archive_mstring_copy_mbs(&entry->ae_hardlink, target);
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, target);
}

void archive_entry_copy_hardlink_w(ArchiveEntry * entry, const wchar_t * target)
{
	archive_mstring_copy_wcs(&entry->ae_hardlink, target);
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, target);
}

int archive_entry_update_hardlink_utf8(ArchiveEntry * entry, const char * target)
{
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, target);
	if(archive_mstring_update_utf8(entry->archive, &entry->ae_hardlink, target) == 0)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

int _archive_entry_copy_hardlink_l(ArchiveEntry * entry, const char * target, size_t len, archive_string_conv * sc)
{
	int r = archive_mstring_copy_mbs_len_l(&entry->ae_hardlink, target, len, sc);
	SETFLAG(entry->ae_set, AE_SET_HARDLINK, (target && r == 0));
	return r;
}

void archive_entry_set_atime(ArchiveEntry * entry, time_t t, long ns)
{
	FIX_NS(t, ns);
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_ATIME;
	entry->ae_stat.aest_atime = t;
	entry->ae_stat.aest_atime_nsec = ns;
}

void archive_entry_unset_atime(ArchiveEntry * entry)
{
	archive_entry_set_atime(entry, 0, 0);
	entry->ae_set &= ~AE_SET_ATIME;
}

void archive_entry_set_birthtime(ArchiveEntry * entry, time_t t, long ns)
{
	FIX_NS(t, ns);
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_BIRTHTIME;
	entry->ae_stat.aest_birthtime = t;
	entry->ae_stat.aest_birthtime_nsec = ns;
}

void archive_entry_unset_birthtime(ArchiveEntry * entry)
{
	archive_entry_set_birthtime(entry, 0, 0);
	entry->ae_set &= ~AE_SET_BIRTHTIME;
}

void archive_entry_set_ctime(ArchiveEntry * entry, time_t t, long ns)
{
	FIX_NS(t, ns);
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_CTIME;
	entry->ae_stat.aest_ctime = t;
	entry->ae_stat.aest_ctime_nsec = ns;
}

void archive_entry_unset_ctime(ArchiveEntry * entry)
{
	archive_entry_set_ctime(entry, 0, 0);
	entry->ae_set &= ~AE_SET_CTIME;
}

void archive_entry_set_dev(ArchiveEntry * entry, dev_t d)
{
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_DEV;
	entry->ae_stat.aest_dev_is_broken_down = 0;
	entry->ae_stat.aest_dev = d;
}

void archive_entry_set_devmajor(ArchiveEntry * entry, dev_t m)
{
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_DEV;
	entry->ae_stat.aest_dev_is_broken_down = 1;
	entry->ae_stat.aest_devmajor = m;
}

void archive_entry_set_devminor(ArchiveEntry * entry, dev_t m)
{
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_DEV;
	entry->ae_stat.aest_dev_is_broken_down = 1;
	entry->ae_stat.aest_devminor = m;
}

/* Set symlink if symlink is already set, else set hardlink. */
void archive_entry_set_link(ArchiveEntry * entry, const char * target)
{
	if(entry->ae_set & AE_SET_SYMLINK)
		archive_mstring_copy_mbs(&entry->ae_symlink, target);
	else
		archive_mstring_copy_mbs(&entry->ae_hardlink, target);
}

void archive_entry_set_link_utf8(ArchiveEntry * entry, const char * target)
{
	if(entry->ae_set & AE_SET_SYMLINK)
		archive_mstring_copy_utf8(&entry->ae_symlink, target);
	else
		archive_mstring_copy_utf8(&entry->ae_hardlink, target);
}

/* Set symlink if symlink is already set, else set hardlink. */
void archive_entry_copy_link(ArchiveEntry * entry, const char * target)
{
	if(entry->ae_set & AE_SET_SYMLINK)
		archive_mstring_copy_mbs(&entry->ae_symlink, target);
	else
		archive_mstring_copy_mbs(&entry->ae_hardlink, target);
}

/* Set symlink if symlink is already set, else set hardlink. */
void archive_entry_copy_link_w(ArchiveEntry * entry, const wchar_t * target)
{
	if(entry->ae_set & AE_SET_SYMLINK)
		archive_mstring_copy_wcs(&entry->ae_symlink, target);
	else
		archive_mstring_copy_wcs(&entry->ae_hardlink, target);
}

int archive_entry_update_link_utf8(ArchiveEntry * entry, const char * target)
{
	int r;
	if(entry->ae_set & AE_SET_SYMLINK)
		r = archive_mstring_update_utf8(entry->archive, &entry->ae_symlink, target);
	else
		r = archive_mstring_update_utf8(entry->archive, &entry->ae_hardlink, target);
	if(!r)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

int _archive_entry_copy_link_l(ArchiveEntry * entry, const char * target, size_t len, archive_string_conv * sc)
{
	int r;
	if(entry->ae_set & AE_SET_SYMLINK)
		r = archive_mstring_copy_mbs_len_l(&entry->ae_symlink, target, len, sc);
	else
		r = archive_mstring_copy_mbs_len_l(&entry->ae_hardlink, target, len, sc);
	return r;
}

void FASTCALL archive_entry_set_mode(ArchiveEntry * entry, mode_t m)
{
	entry->stat_valid = 0;
	entry->acl.mode = m;
}

void archive_entry_set_mtime(ArchiveEntry * entry, time_t t, long ns)
{
	FIX_NS(t, ns);
	entry->stat_valid = 0;
	entry->ae_set |= AE_SET_MTIME;
	entry->ae_stat.aest_mtime = t;
	entry->ae_stat.aest_mtime_nsec = ns;
}

void archive_entry_unset_mtime(ArchiveEntry * entry)
{
	archive_entry_set_mtime(entry, 0, 0);
	entry->ae_set &= ~AE_SET_MTIME;
}

void FASTCALL archive_entry_set_nlink(ArchiveEntry * entry, uint nlink)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_nlink = nlink;
}

void FASTCALL archive_entry_set_pathname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_pathname, name);
}

void archive_entry_set_pathname_utf8(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_utf8(&entry->ae_pathname, name);
}

void archive_entry_copy_pathname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_pathname, name);
}

void archive_entry_copy_pathname_w(ArchiveEntry * entry, const wchar_t * name)
{
	archive_mstring_copy_wcs(&entry->ae_pathname, name);
}

int archive_entry_update_pathname_utf8(ArchiveEntry * entry, const char * name)
{
	if(archive_mstring_update_utf8(entry->archive, &entry->ae_pathname, name) == 0)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

int _archive_entry_copy_pathname_l(ArchiveEntry * entry, const char * name, size_t len, archive_string_conv * sc)
{
	return (archive_mstring_copy_mbs_len_l(&entry->ae_pathname, name, len, sc));
}

void archive_entry_set_perm(ArchiveEntry * entry, mode_t p)
{
	entry->stat_valid = 0;
	entry->acl.mode &= AE_IFMT;
	entry->acl.mode |= ~AE_IFMT & p;
}

void archive_entry_set_rdev(ArchiveEntry * entry, dev_t m)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_rdev = m;
	entry->ae_stat.aest_rdev_is_broken_down = 0;
}

void archive_entry_set_rdevmajor(ArchiveEntry * entry, dev_t m)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_rdev_is_broken_down = 1;
	entry->ae_stat.aest_rdevmajor = m;
}

void archive_entry_set_rdevminor(ArchiveEntry * entry, dev_t m)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_rdev_is_broken_down = 1;
	entry->ae_stat.aest_rdevminor = m;
}

void STDCALL archive_entry_set_size(ArchiveEntry * entry, la_int64_t s)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_size = s;
	entry->ae_set |= AE_SET_SIZE;
}

void FASTCALL archive_entry_unset_size(ArchiveEntry * entry)
{
	archive_entry_set_size(entry, 0);
	entry->ae_set &= ~AE_SET_SIZE;
}

void archive_entry_copy_sourcepath(ArchiveEntry * entry, const char * path)
{
	archive_mstring_copy_mbs(&entry->ae_sourcepath, path);
}

void archive_entry_copy_sourcepath_w(ArchiveEntry * entry, const wchar_t * path)
{
	archive_mstring_copy_wcs(&entry->ae_sourcepath, path);
}

void archive_entry_set_symlink(ArchiveEntry * entry, const char * linkname)
{
	archive_mstring_copy_mbs(&entry->ae_symlink, linkname);
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, linkname);
}

void archive_entry_set_symlink_type(ArchiveEntry * entry, int type)
{
	entry->ae_symlink_type = type;
}

void archive_entry_set_symlink_utf8(ArchiveEntry * entry, const char * linkname)
{
	archive_mstring_copy_utf8(&entry->ae_symlink, linkname);
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, linkname);
}

void archive_entry_copy_symlink(ArchiveEntry * entry, const char * linkname)
{
	archive_mstring_copy_mbs(&entry->ae_symlink, linkname);
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, linkname);
}

void archive_entry_copy_symlink_w(ArchiveEntry * entry, const wchar_t * linkname)
{
	archive_mstring_copy_wcs(&entry->ae_symlink, linkname);
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, linkname);
}

int archive_entry_update_symlink_utf8(ArchiveEntry * entry, const char * linkname)
{
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, linkname);
	if(archive_mstring_update_utf8(entry->archive, &entry->ae_symlink, linkname) == 0)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

int _archive_entry_copy_symlink_l(ArchiveEntry * entry, const char * linkname, size_t len, archive_string_conv * sc)
{
	int r = archive_mstring_copy_mbs_len_l(&entry->ae_symlink, linkname, len, sc);
	SETFLAG(entry->ae_set, AE_SET_SYMLINK, (linkname && r == 0));
	return r;
}

void archive_entry_set_uid(ArchiveEntry * entry, la_int64_t u)
{
	entry->stat_valid = 0;
	entry->ae_stat.aest_uid = u;
}

void archive_entry_set_uname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_uname, name);
}

void archive_entry_set_uname_utf8(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_utf8(&entry->ae_uname, name);
}

void archive_entry_copy_uname(ArchiveEntry * entry, const char * name)
{
	archive_mstring_copy_mbs(&entry->ae_uname, name);
}

void archive_entry_copy_uname_w(ArchiveEntry * entry, const wchar_t * name)
{
	archive_mstring_copy_wcs(&entry->ae_uname, name);
}

int archive_entry_update_uname_utf8(ArchiveEntry * entry, const char * name)
{
	if(archive_mstring_update_utf8(entry->archive, &entry->ae_uname, name) == 0)
		return 1;
	if(errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return 0;
}

void archive_entry_set_is_data_encrypted(ArchiveEntry * entry, char is_encrypted)
{
	SETFLAG(entry->encryption, AE_ENCRYPTION_DATA, is_encrypted);
}

void archive_entry_set_is_metadata_encrypted(ArchiveEntry * entry, char is_encrypted)
{
	SETFLAG(entry->encryption, AE_ENCRYPTION_METADATA, is_encrypted);
}

int _archive_entry_copy_uname_l(ArchiveEntry * entry, const char * name, size_t len, archive_string_conv * sc)
{
	return (archive_mstring_copy_mbs_len_l(&entry->ae_uname, name, len, sc));
}

const void * archive_entry_mac_metadata(ArchiveEntry * entry, size_t * s)
{
	*s = entry->mac_metadata_size;
	return entry->mac_metadata;
}

void archive_entry_copy_mac_metadata(ArchiveEntry * entry, const void * p, size_t s)
{
	SAlloc::F(entry->mac_metadata);
	if(!p || !s) {
		entry->mac_metadata = NULL;
		entry->mac_metadata_size = 0;
	}
	else {
		entry->mac_metadata_size = s;
		entry->mac_metadata = SAlloc::M(s);
		if(entry->mac_metadata == NULL)
			abort();
		memcpy(entry->mac_metadata, p, s);
	}
}

/* Digest handling */
const uchar * archive_entry_digest(ArchiveEntry * entry, int type)
{
	switch(type) {
		case ARCHIVE_ENTRY_DIGEST_MD5: return entry->digest.md5;
		case ARCHIVE_ENTRY_DIGEST_RMD160: return entry->digest.rmd160;
		case ARCHIVE_ENTRY_DIGEST_SHA1: return entry->digest.sha1;
		case ARCHIVE_ENTRY_DIGEST_SHA256: return entry->digest.sha256;
		case ARCHIVE_ENTRY_DIGEST_SHA384: return entry->digest.sha384;
		case ARCHIVE_ENTRY_DIGEST_SHA512: return entry->digest.sha512;
		default: return NULL;
	}
}

int archive_entry_set_digest(ArchiveEntry * entry, int type, const uchar * digest)
{
#define copy_digest(_e, _t, _d) memcpy(_e->digest._t, _d, sizeof(_e->digest._t))
	switch(type) {
		case ARCHIVE_ENTRY_DIGEST_MD5: copy_digest(entry, md5, digest); break;
		case ARCHIVE_ENTRY_DIGEST_RMD160: copy_digest(entry, rmd160, digest); break;
		case ARCHIVE_ENTRY_DIGEST_SHA1: copy_digest(entry, sha1, digest); break;
		case ARCHIVE_ENTRY_DIGEST_SHA256: copy_digest(entry, sha256, digest); break;
		case ARCHIVE_ENTRY_DIGEST_SHA384: copy_digest(entry, sha384, digest); break;
		case ARCHIVE_ENTRY_DIGEST_SHA512: copy_digest(entry, sha512, digest); break;
		default: return ARCHIVE_WARN;
	}
	return ARCHIVE_OK;
#undef copy_digest
}
/*
 * ACL management.  The following would, of course, be a lot simpler
 * if: 1) the last draft of POSIX.1e were a really thorough and
 * complete standard that addressed the needs of ACL archiving and 2)
 * everyone followed it faithfully.  Alas, neither is true, so the
 * following is a lot more complex than might seem necessary to the
 * uninitiated.
 */
archive_acl * archive_entry_acl(ArchiveEntry * entry)                      
{
	return &entry->acl;
}

void archive_entry_acl_clear(ArchiveEntry * entry)
{
	archive_acl_clear(&entry->acl);
}
/*
 * Add a single ACL entry to the internal list of ACL data.
 */
int archive_entry_acl_add_entry(ArchiveEntry * entry, int type, int permset, int tag, int id, const char * name)
{
	return archive_acl_add_entry(&entry->acl, type, permset, tag, id, name);
}
/*
 * As above, but with a wide-character name.
 */
int archive_entry_acl_add_entry_w(ArchiveEntry * entry, int type, int permset, int tag, int id, const wchar_t * name)
{
	return archive_acl_add_entry_w_len(&entry->acl, type, permset, tag, id, name, wcslen(name));
}
/*
 * Return a bitmask of ACL types in an archive entry ACL list
 */
int archive_entry_acl_types(ArchiveEntry * entry)
{
	return (archive_acl_types(&entry->acl));
}
/*
 * Return a count of entries matching "want_type".
 */
int archive_entry_acl_count(ArchiveEntry * entry, int want_type)
{
	return archive_acl_count(&entry->acl, want_type);
}
/*
 * Prepare for reading entries from the ACL data.  Returns a count
 * of entries matching "want_type", or zero if there are no
 * non-extended ACL entries of that type.
 */
int archive_entry_acl_reset(ArchiveEntry * entry, int want_type)
{
	return archive_acl_reset(&entry->acl, want_type);
}
/*
 * Return the next ACL entry in the list.  Fake entries for the
 * standard permissions and include them in the returned list.
 */
int archive_entry_acl_next(ArchiveEntry * entry, int want_type, int * type, int * permset, int * tag, int * id, const char ** name)
{
	int r = archive_acl_next(entry->archive, &entry->acl, want_type, type, permset, tag, id, name);
	if(r == ARCHIVE_FATAL && errno == ENOMEM)
		__archive_errx(1, SlTxtOutOfMem);
	return r;
}
/*
 * Generate a text version of the ACL. The flags parameter controls
 * the style of the generated ACL.
 */
wchar_t * archive_entry_acl_to_text_w(ArchiveEntry * entry, la_ssize_t * len, int flags)
{
	return (archive_acl_to_text_w(&entry->acl, reinterpret_cast<ssize_t *>(len), flags, entry->archive));
}

char * archive_entry_acl_to_text(ArchiveEntry * entry, la_ssize_t * len, int flags)
{
	return (archive_acl_to_text_l(&entry->acl, reinterpret_cast<ssize_t *>(len), flags, NULL));
}

char * _archive_entry_acl_to_text_l(ArchiveEntry * entry, ssize_t * len, int flags, archive_string_conv * sc)
{
	return (archive_acl_to_text_l(&entry->acl, len, flags, sc));
}
/*
 * ACL text parser.
 */
int archive_entry_acl_from_text_w(ArchiveEntry * entry, const wchar_t * wtext, int type)
{
	return (archive_acl_from_text_w(&entry->acl, wtext, type));
}

int archive_entry_acl_from_text(ArchiveEntry * entry, const char * text, int type)
{
	return (archive_acl_from_text_l(&entry->acl, text, type, NULL));
}

int _archive_entry_acl_from_text_l(ArchiveEntry * entry, const char * text, int type, archive_string_conv * sc)
{
	return (archive_acl_from_text_l(&entry->acl, text, type, sc));
}

/* Deprecated */
static int archive_entry_acl_text_compat(int * flags)
{
	if((*flags & ARCHIVE_ENTRY_ACL_TYPE_POSIX1E) == 0)
		return 1;
	/* ABI compat with old ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID */
	if((*flags & OLD_ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID) != 0)
		*flags |= ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID;
	/* ABI compat with old ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT */
	if((*flags & OLD_ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT) != 0)
		*flags |=  ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT;
	*flags |= ARCHIVE_ENTRY_ACL_STYLE_SEPARATOR_COMMA;
	return 0;
}

/* Deprecated */
const wchar_t * archive_entry_acl_text_w(ArchiveEntry * entry, int flags)
{
	ZFREE(entry->acl.acl_text_w);
	if(archive_entry_acl_text_compat(&flags) == 0)
		entry->acl.acl_text_w = archive_acl_to_text_w(&entry->acl, NULL, flags, entry->archive);
	return (entry->acl.acl_text_w);
}

/* Deprecated */
const char * archive_entry_acl_text(ArchiveEntry * entry, int flags)
{
	ZFREE(entry->acl.acl_text);
	if(archive_entry_acl_text_compat(&flags) == 0)
		entry->acl.acl_text = archive_acl_to_text_l(&entry->acl, NULL, flags, NULL);
	return (entry->acl.acl_text);
}

/* Deprecated */
int _archive_entry_acl_text_l(ArchiveEntry * entry, int flags,
    const char ** acl_text, size_t * len, archive_string_conv * sc)
{
	SAlloc::F(entry->acl.acl_text);
	entry->acl.acl_text = NULL;
	if(archive_entry_acl_text_compat(&flags) == 0)
		entry->acl.acl_text = archive_acl_to_text_l(&entry->acl, (ssize_t*)len, flags, sc);
	*acl_text = entry->acl.acl_text;
	return 0;
}
/*
 * Following code is modified from UC Berkeley sources, and
 * is subject to the following copyright notice.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 */
/*
 * Supported file flags on FreeBSD and Mac OS:
 * sappnd,sappend		SF_APPEND
 * arch,archived		SF_ARCHIVED
 * schg,schange,simmutable	SF_IMMUTABLE
 * sunlnk,sunlink		SF_NOUNLINK	(FreeBSD only)
 * uappnd,uappend		UF_APPEND
 * compressed			UF_COMPRESSED	(Mac OS only)
 * hidden,uhidden		UF_HIDDEN
 * uchg,uchange,uimmutable	UF_IMMUTABLE
 * nodump			UF_NODUMP
 * uunlnk,uunlink		UF_NOUNLINK	(FreeBSD only)
 * offline,uoffline		UF_OFFLINE	(FreeBSD only)
 * opaque			UF_OPAQUE
 * rdonly,urdonly,readonly	UF_READONLY	(FreeBSD only)
 * reparse,ureparse		UF_REPARSE	(FreeBSD only)
 * sparse,usparse		UF_SPARSE	(FreeBSD only)
 * system,usystem		UF_SYSTEM	(FreeBSD only)
 *
 * See chflags(2) for more information
 *
 * Supported file attributes on Linux:
 * a	append only			FS_APPEND_FL		sappnd
 * A	no atime updates		FS_NOATIME_FL		atime
 * c	compress			FS_COMPR_FL		compress
 * C	no copy on write		FS_NOCOW_FL		cow
 * d	no dump				FS_NODUMP_FL		dump
 * D	synchronous directory updates	FS_DIRSYNC_FL		dirsync
 * i	immutable			FS_IMMUTABLE_FL		schg
 * j	data journalling		FS_JOURNAL_DATA_FL	journal
 * P	project hierarchy		FS_PROJINHERIT_FL	projinherit
 * s	secure deletion			FS_SECRM_FL		securedeletion
 * S	synchronous updates		FS_SYNC_FL		sync
 * t	no tail-merging			FS_NOTAIL_FL		tail
 * T	top of directory hierarchy	FS_TOPDIR_FL		topdir
 * u	undeletable			FS_UNRM_FL		undel
 *
 * See ioctl_iflags(2) for more information
 *
 * Equivalent file flags supported on FreeBSD / Mac OS and Linux:
 * SF_APPEND		FS_APPEND_FL		sappnd
 * SF_IMMUTABLE		FS_IMMUTABLE_FL		schg
 * UF_NODUMP		FS_NODUMP_FL		nodump
 */

static const struct flag {
	const char      * name;
	const wchar_t   * wname;
	ulong set;
	ulong clear;
} fileflags[] = {
	/* Preferred (shorter) names per flag first, all prefixed by "no" */
#ifdef SF_APPEND
	{ "nosappnd",   L"nosappnd",            SF_APPEND,      0},
	{ "nosappend",  L"nosappend",           SF_APPEND,      0},
#endif
#if defined(FS_APPEND_FL)                       /* 'a' */
	{ "nosappnd",   L"nosappnd",            FS_APPEND_FL,   0},
	{ "nosappend",  L"nosappend",           FS_APPEND_FL,   0},
#elif defined(EXT2_APPEND_FL)                   /* 'a' */
	{ "nosappnd",   L"nosappnd",            EXT2_APPEND_FL, 0},
	{ "nosappend",  L"nosappend",           EXT2_APPEND_FL, 0},
#endif
#ifdef SF_ARCHIVED
	{ "noarch",     L"noarch",              SF_ARCHIVED,    0},
	{ "noarchived", L"noarchived",          SF_ARCHIVED,    0},
#endif
#ifdef SF_IMMUTABLE
	{ "noschg",     L"noschg",              SF_IMMUTABLE,   0},
	{ "noschange",  L"noschange",           SF_IMMUTABLE,   0},
	{ "nosimmutable",       L"nosimmutable",        SF_IMMUTABLE,   0},
#endif
#if defined(FS_IMMUTABLE_FL)                    /* 'i' */
	{ "noschg",     L"noschg",              FS_IMMUTABLE_FL,        0},
	{ "noschange",  L"noschange",           FS_IMMUTABLE_FL,        0},
	{ "nosimmutable",       L"nosimmutable",        FS_IMMUTABLE_FL,        0},
#elif defined(EXT2_IMMUTABLE_FL)                /* 'i' */
	{ "noschg",     L"noschg",              EXT2_IMMUTABLE_FL,      0},
	{ "noschange",  L"noschange",           EXT2_IMMUTABLE_FL,      0},
	{ "nosimmutable",       L"nosimmutable",        EXT2_IMMUTABLE_FL,      0},
#endif
#ifdef SF_NOUNLINK
	{ "nosunlnk",   L"nosunlnk",            SF_NOUNLINK,    0},
	{ "nosunlink",  L"nosunlink",           SF_NOUNLINK,    0},
#endif
#ifdef UF_APPEND
	{ "nouappnd",   L"nouappnd",            UF_APPEND,      0},
	{ "nouappend",  L"nouappend",           UF_APPEND,      0},
#endif
#ifdef UF_IMMUTABLE
	{ "nouchg",     L"nouchg",              UF_IMMUTABLE,   0},
	{ "nouchange",  L"nouchange",           UF_IMMUTABLE,   0},
	{ "nouimmutable",       L"nouimmutable",        UF_IMMUTABLE,   0},
#endif
#ifdef UF_NODUMP
	{ "nodump",     L"nodump",              0,              UF_NODUMP},
#endif
#if defined(FS_NODUMP_FL)       /* 'd' */
	{ "nodump",     L"nodump",              0,              FS_NODUMP_FL},
#elif defined(EXT2_NODUMP_FL)
	{ "nodump",     L"nodump",              0,              EXT2_NODUMP_FL},
#endif
#ifdef UF_OPAQUE
	{ "noopaque",   L"noopaque",            UF_OPAQUE,      0},
#endif
#ifdef UF_NOUNLINK
	{ "nouunlnk",   L"nouunlnk",            UF_NOUNLINK,    0},
	{ "nouunlink",  L"nouunlink",           UF_NOUNLINK,    0},
#endif
#ifdef UF_COMPRESSED
	/* Mac OS */
	{ "nocompressed",       L"nocompressed",        UF_COMPRESSED,  0},
#endif
#ifdef UF_HIDDEN
	{ "nohidden",   L"nohidden",            UF_HIDDEN,      0},
	{ "nouhidden",  L"nouhidden",           UF_HIDDEN,      0},
#endif
#ifdef FILE_ATTRIBUTE_HIDDEN
	{ "nohidden",   L"nohidden",    FILE_ATTRIBUTE_HIDDEN,  0},
	{ "nouhidden",  L"nouhidden",   FILE_ATTRIBUTE_HIDDEN,  0},
#endif
#ifdef UF_OFFLINE
	{ "nooffline",  L"nooffline",           UF_OFFLINE,     0},
	{ "nouoffline", L"nouoffline",          UF_OFFLINE,     0},
#endif
#ifdef UF_READONLY
	{ "nordonly",   L"nordonly",            UF_READONLY,    0},
	{ "nourdonly",  L"nourdonly",           UF_READONLY,    0},
	{ "noreadonly", L"noreadonly",          UF_READONLY,    0},
#endif
#ifdef FILE_ATTRIBUTE_READONLY
	{ "nordonly",   L"nordonly",    FILE_ATTRIBUTE_READONLY,        0},
	{ "nourdonly",  L"nourdonly",   FILE_ATTRIBUTE_READONLY,        0},
	{ "noreadonly", L"noreadonly",  FILE_ATTRIBUTE_READONLY,        0},
#endif
#ifdef UF_SPARSE
	{ "nosparse",   L"nosparse",            UF_SPARSE,      0},
	{ "nousparse",  L"nousparse",           UF_SPARSE,      0},
#endif
#ifdef UF_REPARSE
	{ "noreparse",  L"noreparse",           UF_REPARSE,     0},
	{ "noureparse", L"noureparse",          UF_REPARSE,     0},
#endif
#ifdef UF_SYSTEM
	{ "nosystem",   L"nosystem",            UF_SYSTEM,      0},
	{ "nousystem",  L"nousystem",           UF_SYSTEM,      0},
#endif
#ifdef FILE_ATTRIBUTE_SYSTEM
	{ "nosystem",   L"nosystem",    FILE_ATTRIBUTE_SYSTEM,  0},
	{ "nousystem",  L"nousystem",   FILE_ATTRIBUTE_SYSTEM,  0},
#endif
#if defined(FS_UNRM_FL)         /* 'u' */
	{ "noundel",    L"noundel",             FS_UNRM_FL,     0},
#elif defined(EXT2_UNRM_FL)
	{ "noundel",    L"noundel",             EXT2_UNRM_FL,   0},
#endif

#if defined(FS_COMPR_FL)        /* 'c' */
	{ "nocompress", L"nocompress",          FS_COMPR_FL,    0},
#elif defined(EXT2_COMPR_FL)
	{ "nocompress", L"nocompress",          EXT2_COMPR_FL,  0},
#endif

#if defined(FS_NOATIME_FL)      /* 'A' */
	{ "noatime",    L"noatime",             0,              FS_NOATIME_FL},
#elif defined(EXT2_NOATIME_FL)
	{ "noatime",    L"noatime",             0,              EXT2_NOATIME_FL},
#endif
#if defined(FS_DIRSYNC_FL)      /* 'D' */
	{ "nodirsync",  L"nodirsync",           FS_DIRSYNC_FL,          0},
#elif defined(EXT2_DIRSYNC_FL)
	{ "nodirsync",  L"nodirsync",           EXT2_DIRSYNC_FL,        0},
#endif
#if defined(FS_JOURNAL_DATA_FL) /* 'j' */
	{ "nojournal-data", L"nojournal-data",   FS_JOURNAL_DATA_FL,     0},
	{ "nojournal",  L"nojournal",           FS_JOURNAL_DATA_FL,     0},
#elif defined(EXT3_JOURNAL_DATA_FL)
	{ "nojournal-data", L"nojournal-data",   EXT3_JOURNAL_DATA_FL,   0},
	{ "nojournal",  L"nojournal",           EXT3_JOURNAL_DATA_FL,   0},
#endif
#if defined(FS_SECRM_FL)        /* 's' */
	{ "nosecdel",   L"nosecdel",            FS_SECRM_FL,            0},
	{ "nosecuredeletion", L"nosecuredeletion", FS_SECRM_FL,           0},
#elif defined(EXT2_SECRM_FL)
	{ "nosecdel",   L"nosecdel",            EXT2_SECRM_FL,          0},
	{ "nosecuredeletion", L"nosecuredeletion", EXT2_SECRM_FL,         0},
#endif
#if defined(FS_SYNC_FL)         /* 'S' */
	{ "nosync",     L"nosync",              FS_SYNC_FL,             0},
#elif defined(EXT2_SYNC_FL)
	{ "nosync",     L"nosync",              EXT2_SYNC_FL,           0},
#endif
#if defined(FS_NOTAIL_FL)       /* 't' */
	{ "notail",     L"notail",              0,              FS_NOTAIL_FL},
#elif defined(EXT2_NOTAIL_FL)
	{ "notail",     L"notail",              0,              EXT2_NOTAIL_FL},
#endif
#if defined(FS_TOPDIR_FL)       /* 'T' */
	{ "notopdir",   L"notopdir",            FS_TOPDIR_FL,           0},
#elif defined(EXT2_TOPDIR_FL)
	{ "notopdir",   L"notopdir",            EXT2_TOPDIR_FL,         0},
#endif
#ifdef FS_NOCOW_FL      /* 'C' */
	{ "nocow",      L"nocow",               0,      FS_NOCOW_FL},
#endif
#ifdef FS_PROJINHERIT_FL        /* 'P' */
	{ "noprojinherit", L"noprojinherit", FS_PROJINHERIT_FL,      0},
#endif
	{ NULL, NULL, 0, 0}
};

/*
 * fflagstostr --
 *	Convert file flags to a comma-separated string.  If no flags
 *	are set, return the empty string.
 */
static char * ae_fflagstostr(ulong bitset, ulong bitclear)
{
	char * string, * dp;
	const char * sp;
	const struct flag * flag;
	size_t length;
	ulong bits = bitset | bitclear;
	length = 0;
	for(flag = fileflags; flag->name; flag++)
		if(bits & (flag->set | flag->clear)) {
			length += strlen(flag->name) + 1;
			bits &= ~(flag->set | flag->clear);
		}
	if(length == 0)
		return NULL;
	string = (char *)SAlloc::M(length);
	if(string == NULL)
		return NULL;
	dp = string;
	for(flag = fileflags; flag->name; flag++) {
		if(bitset & flag->set || bitclear & flag->clear) {
			sp = flag->name + 2;
		}
		else if(bitset & flag->clear || bitclear & flag->set) {
			sp = flag->name;
		}
		else
			continue;
		bitset &= ~(flag->set | flag->clear);
		bitclear &= ~(flag->set | flag->clear);
		if(dp > string)
			*dp++ = ',';
		while((*dp++ = *sp++) != '\0')
			;
		dp--;
	}

	*dp = '\0';
	return (string);
}
/*
 * strtofflags --
 *	Take string of arguments and return file flags.  This
 *	version works a little differently than strtofflags(3).
 *	In particular, it always tests every token, skipping any
 *	unrecognized tokens.  It returns a pointer to the first
 *	unrecognized token, or NULL if every token was recognized.
 *	This version is also const-correct and does not modify the
 *	provided string.
 */
static const char * ae_strtofflags(const char * s, ulong * setp, ulong * clrp)
{
	const char * start, * end;
	const struct flag * flag;
	ulong set = 0;
	ulong clear = 0;
	const char * failed = 0;
	start = s;
	/* Find start of first token. */
	while(*start == '\t' || *start == ' ' || *start == ',')
		start++;
	while(*start != '\0') {
		size_t length;
		/* Locate end of token. */
		end = start;
		while(*end != '\0' && *end != '\t' && *end != ' ' && *end != ',')
			end++;
		length = end - start;
		for(flag = fileflags; flag->name; flag++) {
			size_t flag_length = strlen(flag->name);
			if(length == flag_length && memcmp(start, flag->name, length) == 0) {
				/* Matched "noXXXX", so reverse the sense. */
				clear |= flag->set;
				set |= flag->clear;
				break;
			}
			else if(length == flag_length - 2 && memcmp(start, flag->name + 2, length) == 0) {
				/* Matched "XXXX", so don't reverse. */
				set |= flag->set;
				clear |= flag->clear;
				break;
			}
		}
		/* Ignore unknown flag names. */
		if(flag->name == NULL && failed == NULL)
			failed = start;
		/* Find start of next token. */
		start = end;
		while(*start == '\t' || *start == ' ' || *start == ',')
			start++;
	}
	if(setp)
		*setp = set;
	if(clrp)
		*clrp = clear;
	// Return location of first failure. 
	return (failed);
}
/*
 * wcstofflags --
 *	Take string of arguments and return file flags.  This
 *	version works a little differently than strtofflags(3).
 *	In particular, it always tests every token, skipping any
 *	unrecognized tokens.  It returns a pointer to the first
 *	unrecognized token, or NULL if every token was recognized.
 *	This version is also const-correct and does not modify the
 *	provided string.
 */
static const wchar_t * ae_wcstofflags(const wchar_t * s, ulong * setp, ulong * clrp)
{
	const wchar_t * start, * end;
	const struct flag * flag;
	ulong set, clear;
	const wchar_t * failed;
	set = clear = 0;
	start = s;
	failed = NULL;
	/* Find start of first token. */
	while(*start == L' ' || *start == L'\t' || *start == L',')
		start++;
	while(*start != L'\0') {
		size_t length;
		/* Locate end of token. */
		end = start;
		while(*end != L'\0' && *end != L'\t' && *end != L' ' && *end != L',')
			end++;
		length = end - start;
		for(flag = fileflags; flag->wname; flag++) {
			size_t flag_length = wcslen(flag->wname);
			if(length == flag_length && wmemcmp(start, flag->wname, length) == 0) {
				/* Matched "noXXXX", so reverse the sense. */
				clear |= flag->set;
				set |= flag->clear;
				break;
			}
			else if(length == flag_length - 2 && wmemcmp(start, flag->wname + 2, length) == 0) {
				/* Matched "XXXX", so don't reverse. */
				set |= flag->set;
				clear |= flag->clear;
				break;
			}
		}
		/* Ignore unknown flag names. */
		if(flag->wname == NULL && failed == NULL)
			failed = start;
		/* Find start of next token. */
		start = end;
		while(*start == L' ' || *start == L'\t' || *start == L',')
			start++;
	}
	ASSIGN_PTR(setp, set);
	ASSIGN_PTR(clrp, clear);
	/* Return location of first failure. */
	return (failed);
}

#ifdef TEST
#include <stdio.h>
int main(int argc, char ** argv)
{
	ArchiveEntry * entry = archive_entry_new();
	ulong set, clear;
	const wchar_t * remainder;
	remainder = archive_entry_copy_fflags_text_w(entry, L"nosappnd dump archive,,,,,,,");
	archive_entry_fflags(entry, &set, &clear);
	wprintf(L"set=0x%lX clear=0x%lX remainder='%ls'\n", set, clear, remainder);
	wprintf(L"new flags='%s'\n", archive_entry_fflags_text(entry));
	return 0;
}

#endif
