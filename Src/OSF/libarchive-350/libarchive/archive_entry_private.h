/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#ifndef ARCHIVE_ENTRY_PRIVATE_H_INCLUDED
#define ARCHIVE_ENTRY_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif
//#include "archive_acl_private.h"
//#include "archive_string.h"

struct ae_xattr {
	struct ae_xattr *next;
	char	*name;
	void	*value;
	size_t	size;
};

struct ae_sparse {
	struct ae_sparse *next;
	int64	 offset;
	int64	 length;
};

struct ae_digest {
	uchar md5[16];
	uchar rmd160[20];
	uchar sha1[20];
	uchar sha256[32];
	uchar sha384[48];
	uchar sha512[64];
};

/*
 * Description of an archive entry.
 *
 * Basically, this is a "struct stat" with a few text fields added in.
 *
 * TODO: Add "comment", "charset", and possibly other entries
 * that are supported by "pax interchange" format.  However, GNU, ustar,
 * cpio, and other variants don't support these features, so they're not an
 * excruciatingly high priority right now.
 *
 * TODO: "pax interchange" format allows essentially arbitrary
 * key/value attributes to be attached to any entry.  Supporting
 * such extensions may make this library useful for special
 * applications (e.g., a package manager could attach special
 * package-management attributes to each entry).  There are tricky
 * API issues involved, so this is not going to happen until
 * there's a real demand for it.
 *
 * TODO: Design a good API for handling sparse files.
 */
struct ArchiveEntry {
	Archive * archive;
	/*
	 * Note that ae_stat.st_mode & AE_IFMT  can be  0!
	 *
	 * This occurs when the actual file type of the object is not
	 * in the archive.  For example, 'tar' archives store
	 * hardlinks without marking the type of the underlying object.
	 */
	/*
	 * We have a "struct aest" for holding file metadata rather than just
	 * a "struct stat" because on some platforms the "struct stat" has
	 * fields which are too narrow to hold the range of possible values;
	 * we don't want to lose information if we read an archive and write
	 * out another (e.g., in "tar -cf new.tar @old.tar").
	 *
	 * The "stat" pointer points to some form of platform-specific struct
	 * stat; it is declared as a void * rather than a struct stat * as
	 * some platforms have multiple varieties of stat structures.
	 */
	void *stat;
	int  stat_valid; /* Set to 0 whenever a field in aest changes. */

	struct aest {
		int64  aest_atime;
		uint32 aest_atime_nsec;
		int64  aest_ctime;
		uint32 aest_ctime_nsec;
		int64  aest_mtime;
		uint32 aest_mtime_nsec;
		int64  aest_birthtime;
		uint32 aest_birthtime_nsec;
		int64  aest_gid;
		int64  aest_ino;
		uint32 aest_nlink;
		uint64 aest_size;
		int64  aest_uid;
		/*
		 * Because converting between device codes and
		 * major/minor values is platform-specific and
		 * inherently a bit risky, we only do that conversion
		 * lazily.  That way, we will do a better job of
		 * preserving information in those cases where no
		 * conversion is actually required.
		 */
		int    aest_dev_is_broken_down;
		dev_t  aest_dev;
		dev_t  aest_devmajor;
		dev_t  aest_devminor;
		int    aest_rdev_is_broken_down;
		dev_t  aest_rdev;
		dev_t  aest_rdevmajor;
		dev_t  aest_rdevminor;
	} ae_stat;

	int ae_set; /* bitmap of fields that are currently set */
#define	AE_SET_HARDLINK	1
#define	AE_SET_SYMLINK	2
#define	AE_SET_ATIME	4
#define	AE_SET_CTIME	8
#define	AE_SET_MTIME	16
#define	AE_SET_BIRTHTIME 32
#define	AE_SET_SIZE	64
#define	AE_SET_INO	128
#define	AE_SET_DEV	256
	/*
	 * Use aes here so that we get transparent mbs<->wcs conversions.
	 */
	struct archive_mstring ae_fflags_text;	/* Text fflags per fflagstostr(3) */
	ulong ae_fflags_set;		/* Bitmap fflags */
	ulong ae_fflags_clear;
	struct archive_mstring ae_gname;    // Name of owning group
	struct archive_mstring ae_hardlink; // Name of target for hardlink
	struct archive_mstring ae_pathname; // Name of entry
	struct archive_mstring ae_symlink;  // symlink contents
	struct archive_mstring ae_uname;    // Name of owner
	// Not used within libarchive; useful for some clients
	struct archive_mstring ae_sourcepath; // Path this entry is sourced from
#define AE_ENCRYPTION_NONE 0
#define AE_ENCRYPTION_DATA 1
#define AE_ENCRYPTION_METADATA 2
	char   encryption;
	void * mac_metadata;
	size_t mac_metadata_size;
	struct ae_digest digest; /* Digest support. */
	archive_acl    acl; /* ACL support. */
	/* extattr support. */
	struct ae_xattr *xattr_head;
	struct ae_xattr *xattr_p;
	/* sparse support. */
	struct ae_sparse *sparse_head;
	struct ae_sparse *sparse_tail;
	struct ae_sparse *sparse_p;
	char   strmode[12]; /* Miscellaneous. */
	int    ae_symlink_type; /* Symlink type support */
};

int archive_entry_set_digest(ArchiveEntry *entry, int type, const uchar *digest);

#endif /* ARCHIVE_ENTRY_PRIVATE_H_INCLUDED */
