/*-
 * Copyright (c) 2003-2008 Tim Kientzle
 * Copyright (c) 2016 Martin Matuska
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
 * $FreeBSD: head/lib/libarchive/archive_entry.h 201096 2009-12-28 02:41:27Z kientzle $
 */
#ifndef ARCHIVE_ENTRY_H_INCLUDED
#define	ARCHIVE_ENTRY_H_INCLUDED
/* Note: Compiler will complain if this does not match archive.h! */
#define	ARCHIVE_VERSION_NUMBER 3005000
/*
 * Note: archive_entry.h is for use outside of libarchive; the
 * configuration headers (config.h, archive_platform.h, etc.) are
 * purely internal.  Do NOT use HAVE_XXX configuration macros to
 * control the behavior of this header!  If you must conditionalize,
 * use predefined compiler and/or platform macros.
 */
#include <sys/types.h>
#include <stddef.h> // for wchar_t 
// Get a suitable 64-bit integer type
#if !defined(__LA_INT64_T_DEFINED)
#if ARCHIVE_VERSION_NUMBER < 4000000
#define __LA_INT64_T la_int64_t
#endif
#define __LA_INT64_T_DEFINED
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
typedef __int64 la_int64_t;
# else
#include <unistd.h>
#if defined(_SCO_DS) || defined(__osf__)
typedef long long la_int64_t;
#else
typedef int64 la_int64_t;
#endif
#endif
#endif

/* The la_ssize_t should match the type used in 'struct stat' */
#if !defined(__LA_SSIZE_T_DEFINED)
/* Older code relied on the __LA_SSIZE_T macro; after 4.0 we'll switch to the typedef exclusively. */
#if ARCHIVE_VERSION_NUMBER < 4000000
#define __LA_SSIZE_T la_ssize_t
#endif
#define __LA_SSIZE_T_DEFINED
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
#if defined(_SSIZE_T_DEFINED) || defined(_SSIZE_T_)
typedef ssize_t la_ssize_t;
#  elif defined(_WIN64)
typedef __int64 la_ssize_t;
#else
typedef long la_ssize_t;
#endif
# else
#include <unistd.h>  /* ssize_t */
typedef ssize_t la_ssize_t;
#endif
#endif

/* Get a suitable definition for mode_t */
#if ARCHIVE_VERSION_NUMBER >= 3999000
/* Switch to plain 'int' for libarchive 4.0.  It's less broken than 'mode_t' */
#define	__LA_MODE_T	int
#elif defined(_WIN32) && !defined(__CYGWIN__) && !defined(__BORLANDC__) && !defined(__WATCOMC__)
#define	__LA_MODE_T	unsigned short
#else
#define	__LA_MODE_T	mode_t
#endif

/* Large file support for Android */
#ifdef __ANDROID__
#include "android_lf.h"
#endif
/*
 * On Windows, define LIBARCHIVE_STATIC if you're building or using a
 * .lib.  The default here assumes you're building a DLL.  Only
 * libarchive source should ever define __LIBARCHIVE_BUILD.
 */
#if ((defined __WIN32__) || (defined _WIN32) || defined(__CYGWIN__)) && (!defined LIBARCHIVE_STATIC)
#ifdef __LIBARCHIVE_BUILD
#ifdef __GNUC__
#define __LA_DECL	__attribute__((dllexport)) extern
#else
#define __LA_DECL	__declspec(dllexport)
#endif
# else
#ifdef __GNUC__
#define __LA_DECL
#else
#define __LA_DECL	__declspec(dllimport)
#endif
#endif
#else
#define __LA_DECL // Static libraries on all platforms and shared libraries on non-Windows
#endif

#if defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 1
#define __LA_DEPRECATED __attribute__((deprecated))
#else
#define __LA_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Description of an archive entry.
 *
 * You can think of this as "struct stat" with some text fields added in.
 *
 * TODO: Add "comment", "charset", and possibly other entries that are
 * supported by "pax interchange" format.  However, GNU, ustar, cpio,
 * and other variants don't support these features, so they're not an
 * excruciatingly high priority right now.
 *
 * TODO: "pax interchange" format allows essentially arbitrary
 * key/value attributes to be attached to any entry.  Supporting
 * such extensions may make this library useful for special
 * applications (e.g., a package manager could attach special
 * package-management attributes to each entry).
 */
struct Archive;
struct ArchiveEntry;
/*
 * File-type constants.  These are returned from archive_entry_filetype()
 * and passed to archive_entry_set_filetype().
 *
 * These values match S_XXX defines on every platform I've checked,
 * including Windows, AIX, Linux, Solaris, and BSD.  They're
 * (re)defined here because platforms generally don't define the ones
 * they don't support.  For example, Windows doesn't define S_IFLNK or
 * S_IFBLK.  Instead of having a mass of conditional logic and system
 * checks to define any S_XXX values that aren't supported locally,
 * I've just defined a new set of such constants so that
 * libarchive-based applications can manipulate and identify archive
 * entries properly even if the hosting platform can't store them on
 * disk.
 *
 * These values are also used directly within some portable formats,
 * such as cpio.  If you find a platform that varies from these, the
 * correct solution is to leave these alone and translate from these
 * portable values to platform-native values when entries are read from
 * or written to disk.
 */
/*
 * In libarchive 4.0, we can drop the casts here.
 * They're needed to work around Borland C's broken mode_t.
 */
#define AE_IFMT		((__LA_MODE_T)0170000)
#define AE_IFREG	((__LA_MODE_T)0100000)
#define AE_IFLNK	((__LA_MODE_T)0120000)
#define AE_IFSOCK	((__LA_MODE_T)0140000)
#define AE_IFCHR	((__LA_MODE_T)0020000)
#define AE_IFBLK	((__LA_MODE_T)0060000)
#define AE_IFDIR	((__LA_MODE_T)0040000)
#define AE_IFIFO	((__LA_MODE_T)0010000)

/*
 * Symlink types
 */
#define AE_SYMLINK_TYPE_UNDEFINED	0
#define AE_SYMLINK_TYPE_FILE		1
#define AE_SYMLINK_TYPE_DIRECTORY	2
/*
 * Basic object manipulation
 */
__LA_DECL ArchiveEntry * archive_entry_clear(ArchiveEntry *);
/* The 'clone' function does a deep copy; all of the strings are copied too. */
__LA_DECL ArchiveEntry * archive_entry_clone(ArchiveEntry *);
__LA_DECL void FASTCALL archive_entry_free(ArchiveEntry *);
__LA_DECL ArchiveEntry * archive_entry_new(void);

/*
 * This form of archive_entry_new2() will pull character-set
 * conversion information from the specified archive handle.  The
 * older archive_entry_new(void) form is equivalent to calling
 * archive_entry_new2(NULL) and will result in the use of an internal
 * default character-set conversion.
 */
__LA_DECL ArchiveEntry * archive_entry_new2(Archive *);
/*
 * Retrieve fields from an archive_entry.
 *
 * There are a number of implicit conversions among these fields.  For
 * example, if a regular string field is set and you read the _w wide
 * character field, the entry will implicitly convert narrow-to-wide
 * using the current locale.  Similarly, dev values are automatically
 * updated when you write devmajor or devminor and vice versa.
 *
 * In addition, fields can be "set" or "unset."  Unset string fields
 * return NULL, non-string fields have _is_set() functions to test
 * whether they've been set.  You can "unset" a string field by
 * assigning NULL; non-string fields have _unset() functions to
 * unset them.
 *
 * Note: There is one ambiguity in the above; string fields will
 * also return NULL when implicit character set conversions fail.
 * This is usually what you want.
 */
__LA_DECL time_t	 FASTCALL archive_entry_atime(const ArchiveEntry *);
__LA_DECL long		 FASTCALL archive_entry_atime_nsec(const ArchiveEntry *);
__LA_DECL int		 FASTCALL archive_entry_atime_is_set(const ArchiveEntry *);
__LA_DECL time_t	 FASTCALL archive_entry_birthtime(const ArchiveEntry *);
__LA_DECL long		 FASTCALL archive_entry_birthtime_nsec(const ArchiveEntry *);
__LA_DECL int		 FASTCALL archive_entry_birthtime_is_set(const ArchiveEntry *);
__LA_DECL time_t	 FASTCALL archive_entry_ctime(const ArchiveEntry *);
__LA_DECL long		 FASTCALL archive_entry_ctime_nsec(const ArchiveEntry *);
__LA_DECL int		 FASTCALL archive_entry_ctime_is_set(const ArchiveEntry *);
__LA_DECL dev_t		 FASTCALL archive_entry_dev(const ArchiveEntry *);
__LA_DECL int		 FASTCALL archive_entry_dev_is_set(const ArchiveEntry *);
__LA_DECL dev_t		 FASTCALL archive_entry_devmajor(const ArchiveEntry *);
__LA_DECL dev_t		 FASTCALL archive_entry_devminor(const ArchiveEntry *);
__LA_DECL __LA_MODE_T FASTCALL archive_entry_filetype(const ArchiveEntry *);
__LA_DECL void		 archive_entry_fflags(const ArchiveEntry *, ulong * /* set */, ulong * /* clear */);
__LA_DECL const char	* FASTCALL archive_entry_fflags_text(ArchiveEntry *);
__LA_DECL la_int64_t	  FASTCALL archive_entry_gid(ArchiveEntry *);
__LA_DECL const char	* FASTCALL archive_entry_gname(ArchiveEntry *);
__LA_DECL const char	* FASTCALL archive_entry_gname_utf8(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_gname_w(ArchiveEntry *);
__LA_DECL const char	* FASTCALL archive_entry_hardlink(ArchiveEntry *);
__LA_DECL const char	* FASTCALL archive_entry_hardlink_utf8(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_hardlink_w(ArchiveEntry *);
__LA_DECL la_int64_t FASTCALL archive_entry_ino(const ArchiveEntry *);
__LA_DECL la_int64_t FASTCALL archive_entry_ino64(const ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_ino_is_set(const ArchiveEntry *);
__LA_DECL __LA_MODE_T  FASTCALL archive_entry_mode(const ArchiveEntry *);
__LA_DECL time_t FASTCALL archive_entry_mtime(const ArchiveEntry *);
__LA_DECL long   FASTCALL archive_entry_mtime_nsec(const ArchiveEntry *);
__LA_DECL int    FASTCALL archive_entry_mtime_is_set(const ArchiveEntry *);
__LA_DECL uint   FASTCALL archive_entry_nlink(const ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_pathname(ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_pathname_utf8(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_pathname_w(ArchiveEntry *);
__LA_DECL __LA_MODE_T FASTCALL archive_entry_perm(ArchiveEntry *);
__LA_DECL dev_t FASTCALL archive_entry_rdev(ArchiveEntry *);
__LA_DECL dev_t FASTCALL archive_entry_rdevmajor(ArchiveEntry *);
__LA_DECL dev_t FASTCALL archive_entry_rdevminor(ArchiveEntry *);
__LA_DECL const char	* FASTCALL archive_entry_sourcepath(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_sourcepath_w(ArchiveEntry *);
__LA_DECL la_int64_t FASTCALL archive_entry_size(ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_size_is_set(ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_strmode(ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_symlink(ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_symlink_utf8(ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_symlink_type(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_symlink_w(ArchiveEntry *);
__LA_DECL la_int64_t FASTCALL archive_entry_uid(const ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_uname(ArchiveEntry *);
__LA_DECL const char * FASTCALL archive_entry_uname_utf8(ArchiveEntry *);
__LA_DECL const wchar_t	* FASTCALL archive_entry_uname_w(ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_is_data_encrypted(ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_is_metadata_encrypted(ArchiveEntry *);
__LA_DECL int FASTCALL archive_entry_is_encrypted(ArchiveEntry *);
/*
 * Set fields in an archive_entry.
 *
 * Note: Before libarchive 2.4, there were 'set' and 'copy' versions
 * of the string setters.  'copy' copied the actual string, 'set' just
 * stored the pointer.  In libarchive 2.4 and later, strings are
 * always copied.
 */

__LA_DECL void	archive_entry_set_atime(ArchiveEntry *, time_t, long);
__LA_DECL void  archive_entry_unset_atime(ArchiveEntry *);
#if defined(_WIN32) && !defined(__CYGWIN__)
__LA_DECL void archive_entry_copy_bhfi(ArchiveEntry *, BY_HANDLE_FILE_INFORMATION *);
#endif
__LA_DECL void	archive_entry_set_birthtime(ArchiveEntry *, time_t, long);
__LA_DECL void  archive_entry_unset_birthtime(ArchiveEntry *);
__LA_DECL void	archive_entry_set_ctime(ArchiveEntry *, time_t, long);
__LA_DECL void  archive_entry_unset_ctime(ArchiveEntry *);
__LA_DECL void	archive_entry_set_dev(ArchiveEntry *, dev_t);
__LA_DECL void	archive_entry_set_devmajor(ArchiveEntry *, dev_t);
__LA_DECL void	archive_entry_set_devminor(ArchiveEntry *, dev_t);
__LA_DECL void	FASTCALL archive_entry_set_filetype(ArchiveEntry *, uint);
__LA_DECL void	archive_entry_set_fflags(ArchiveEntry *, ulong /* set */, ulong /* clear */);
/* Returns pointer to start of first invalid token, or NULL if none. */
/* Note that all recognized tokens are processed, regardless. */
__LA_DECL const char *archive_entry_copy_fflags_text(ArchiveEntry *, const char *);
__LA_DECL const wchar_t *archive_entry_copy_fflags_text_w(ArchiveEntry *, const wchar_t *);
__LA_DECL void	archive_entry_set_gid(ArchiveEntry *, la_int64_t);
__LA_DECL void	archive_entry_set_gname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_gname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_gname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_gname_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_gname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_hardlink(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_hardlink_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_hardlink(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_hardlink_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_hardlink_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_ino(ArchiveEntry *, la_int64_t);
__LA_DECL void	archive_entry_set_ino64(ArchiveEntry *, la_int64_t);
__LA_DECL void	archive_entry_set_link(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_link_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_link(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_link_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_link_utf8(ArchiveEntry *, const char *);
__LA_DECL void	FASTCALL archive_entry_set_mode(ArchiveEntry *, __LA_MODE_T);
__LA_DECL void	archive_entry_set_mtime(ArchiveEntry *, time_t, long);
__LA_DECL void  archive_entry_unset_mtime(ArchiveEntry *);
__LA_DECL void	FASTCALL archive_entry_set_nlink(ArchiveEntry *, uint);
__LA_DECL void	FASTCALL archive_entry_set_pathname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_pathname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_pathname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_pathname_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_pathname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_perm(ArchiveEntry *, __LA_MODE_T);
__LA_DECL void	archive_entry_set_rdev(ArchiveEntry *, dev_t);
__LA_DECL void	archive_entry_set_rdevmajor(ArchiveEntry *, dev_t);
__LA_DECL void	archive_entry_set_rdevminor(ArchiveEntry *, dev_t);
__LA_DECL void	STDCALL archive_entry_set_size(ArchiveEntry *, la_int64_t);
__LA_DECL void	FASTCALL archive_entry_unset_size(ArchiveEntry *);
__LA_DECL void	archive_entry_copy_sourcepath(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_sourcepath_w(ArchiveEntry *, const wchar_t *);
__LA_DECL void	archive_entry_set_symlink(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_symlink_type(ArchiveEntry *, int);
__LA_DECL void	archive_entry_set_symlink_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_symlink(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_symlink_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_symlink_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_uid(ArchiveEntry *, la_int64_t);
__LA_DECL void	archive_entry_set_uname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_uname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_uname(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_copy_uname_w(ArchiveEntry *, const wchar_t *);
__LA_DECL int	archive_entry_update_uname_utf8(ArchiveEntry *, const char *);
__LA_DECL void	archive_entry_set_is_data_encrypted(ArchiveEntry *, char is_encrypted);
__LA_DECL void	archive_entry_set_is_metadata_encrypted(ArchiveEntry *, char is_encrypted);
/*
 * Routines to bulk copy fields to/from a platform-native "struct
 * stat."  Libarchive used to just store a struct stat inside of each
 * archive_entry object, but this created issues when trying to
 * manipulate archives on systems different than the ones they were
 * created on.
 *
 * TODO: On Linux and other LFS systems, provide both stat32 and
 * stat64 versions of these functions and all of the macro glue so
 * that archive_entry_stat is magically defined to
 * archive_entry_stat32 or archive_entry_stat64 as appropriate.
 */
__LA_DECL const struct stat	*archive_entry_stat(ArchiveEntry *);
__LA_DECL void	archive_entry_copy_stat(ArchiveEntry *, const struct stat *);

/*
 * Storage for Mac OS-specific AppleDouble metadata information.
 * Apple-format tar files store a separate binary blob containing
 * encoded metadata with ACL, extended attributes, etc.
 * This provides a place to store that blob.
 */

__LA_DECL const void * archive_entry_mac_metadata(ArchiveEntry *, size_t *);
__LA_DECL void archive_entry_copy_mac_metadata(ArchiveEntry *, const void *, size_t);

/*
 * Digest routine. This is used to query the raw hex digest for the
 * given entry. The type of digest is provided as an argument.
 */
#define ARCHIVE_ENTRY_DIGEST_MD5              0x00000001
#define ARCHIVE_ENTRY_DIGEST_RMD160           0x00000002
#define ARCHIVE_ENTRY_DIGEST_SHA1             0x00000003
#define ARCHIVE_ENTRY_DIGEST_SHA256           0x00000004
#define ARCHIVE_ENTRY_DIGEST_SHA384           0x00000005
#define ARCHIVE_ENTRY_DIGEST_SHA512           0x00000006

__LA_DECL const uchar * archive_entry_digest(ArchiveEntry *, int /* type */);

/*
 * ACL routines.  This used to simply store and return text-format ACL
 * strings, but that proved insufficient for a number of reasons:
 *   = clients need control over uname/uid and gname/gid mappings
 *   = there are many different ACL text formats
 *   = would like to be able to read/convert archives containing ACLs
 *     on platforms that lack ACL libraries
 *
 *  This last point, in particular, forces me to implement a reasonably
 *  complete set of ACL support routines.
 */

/*
 * Permission bits.
 */
#define	ARCHIVE_ENTRY_ACL_EXECUTE             0x00000001
#define	ARCHIVE_ENTRY_ACL_WRITE               0x00000002
#define	ARCHIVE_ENTRY_ACL_READ                0x00000004
#define	ARCHIVE_ENTRY_ACL_READ_DATA           0x00000008
#define	ARCHIVE_ENTRY_ACL_LIST_DIRECTORY      0x00000008
#define	ARCHIVE_ENTRY_ACL_WRITE_DATA          0x00000010
#define	ARCHIVE_ENTRY_ACL_ADD_FILE            0x00000010
#define	ARCHIVE_ENTRY_ACL_APPEND_DATA         0x00000020
#define	ARCHIVE_ENTRY_ACL_ADD_SUBDIRECTORY    0x00000020
#define	ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS    0x00000040
#define	ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS   0x00000080
#define	ARCHIVE_ENTRY_ACL_DELETE_CHILD        0x00000100
#define	ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES     0x00000200
#define	ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES    0x00000400
#define	ARCHIVE_ENTRY_ACL_DELETE              0x00000800
#define	ARCHIVE_ENTRY_ACL_READ_ACL            0x00001000
#define	ARCHIVE_ENTRY_ACL_WRITE_ACL           0x00002000
#define	ARCHIVE_ENTRY_ACL_WRITE_OWNER         0x00004000
#define	ARCHIVE_ENTRY_ACL_SYNCHRONIZE         0x00008000

#define	ARCHIVE_ENTRY_ACL_PERMS_POSIX1E			\
	(ARCHIVE_ENTRY_ACL_EXECUTE			\
	    | ARCHIVE_ENTRY_ACL_WRITE			\
	    | ARCHIVE_ENTRY_ACL_READ)

#define ARCHIVE_ENTRY_ACL_PERMS_NFS4			\
	(ARCHIVE_ENTRY_ACL_EXECUTE			\
	    | ARCHIVE_ENTRY_ACL_READ_DATA		\
	    | ARCHIVE_ENTRY_ACL_LIST_DIRECTORY 		\
	    | ARCHIVE_ENTRY_ACL_WRITE_DATA		\
	    | ARCHIVE_ENTRY_ACL_ADD_FILE		\
	    | ARCHIVE_ENTRY_ACL_APPEND_DATA		\
	    | ARCHIVE_ENTRY_ACL_ADD_SUBDIRECTORY	\
	    | ARCHIVE_ENTRY_ACL_READ_NAMED_ATTRS	\
	    | ARCHIVE_ENTRY_ACL_WRITE_NAMED_ATTRS	\
	    | ARCHIVE_ENTRY_ACL_DELETE_CHILD		\
	    | ARCHIVE_ENTRY_ACL_READ_ATTRIBUTES		\
	    | ARCHIVE_ENTRY_ACL_WRITE_ATTRIBUTES	\
	    | ARCHIVE_ENTRY_ACL_DELETE			\
	    | ARCHIVE_ENTRY_ACL_READ_ACL		\
	    | ARCHIVE_ENTRY_ACL_WRITE_ACL		\
	    | ARCHIVE_ENTRY_ACL_WRITE_OWNER		\
	    | ARCHIVE_ENTRY_ACL_SYNCHRONIZE)

/*
 * Inheritance values (NFS4 ACLs only); included in permset.
 */
#define	ARCHIVE_ENTRY_ACL_ENTRY_INHERITED                   0x01000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_FILE_INHERIT                0x02000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_DIRECTORY_INHERIT           0x04000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_NO_PROPAGATE_INHERIT        0x08000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_INHERIT_ONLY                0x10000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_SUCCESSFUL_ACCESS           0x20000000
#define	ARCHIVE_ENTRY_ACL_ENTRY_FAILED_ACCESS               0x40000000

#define	ARCHIVE_ENTRY_ACL_INHERITANCE_NFS4			\
	(ARCHIVE_ENTRY_ACL_ENTRY_FILE_INHERIT			\
	    | ARCHIVE_ENTRY_ACL_ENTRY_DIRECTORY_INHERIT		\
	    | ARCHIVE_ENTRY_ACL_ENTRY_NO_PROPAGATE_INHERIT	\
	    | ARCHIVE_ENTRY_ACL_ENTRY_INHERIT_ONLY		\
	    | ARCHIVE_ENTRY_ACL_ENTRY_SUCCESSFUL_ACCESS		\
	    | ARCHIVE_ENTRY_ACL_ENTRY_FAILED_ACCESS		\
	    | ARCHIVE_ENTRY_ACL_ENTRY_INHERITED)

/* We need to be able to specify combinations of these. */
#define	ARCHIVE_ENTRY_ACL_TYPE_ACCESS	0x00000100  /* POSIX.1e only */
#define	ARCHIVE_ENTRY_ACL_TYPE_DEFAULT	0x00000200  /* POSIX.1e only */
#define	ARCHIVE_ENTRY_ACL_TYPE_ALLOW	0x00000400 /* NFS4 only */
#define	ARCHIVE_ENTRY_ACL_TYPE_DENY	0x00000800 /* NFS4 only */
#define	ARCHIVE_ENTRY_ACL_TYPE_AUDIT	0x00001000 /* NFS4 only */
#define	ARCHIVE_ENTRY_ACL_TYPE_ALARM	0x00002000 /* NFS4 only */
#define	ARCHIVE_ENTRY_ACL_TYPE_POSIX1E	(ARCHIVE_ENTRY_ACL_TYPE_ACCESS | ARCHIVE_ENTRY_ACL_TYPE_DEFAULT)
#define	ARCHIVE_ENTRY_ACL_TYPE_NFS4	(ARCHIVE_ENTRY_ACL_TYPE_ALLOW | ARCHIVE_ENTRY_ACL_TYPE_DENY | ARCHIVE_ENTRY_ACL_TYPE_AUDIT | ARCHIVE_ENTRY_ACL_TYPE_ALARM)

/* Tag values mimic POSIX.1e */
#define	ARCHIVE_ENTRY_ACL_USER		10001	/* Specified user. */
#define	ARCHIVE_ENTRY_ACL_USER_OBJ 	10002	/* User who owns the file. */
#define	ARCHIVE_ENTRY_ACL_GROUP		10003	/* Specified group. */
#define	ARCHIVE_ENTRY_ACL_GROUP_OBJ	10004	/* Group who owns the file. */
#define	ARCHIVE_ENTRY_ACL_MASK		10005	/* Modify group access (POSIX.1e only) */
#define	ARCHIVE_ENTRY_ACL_OTHER		10006	/* Public (POSIX.1e only) */
#define	ARCHIVE_ENTRY_ACL_EVERYONE	10107   /* Everyone (NFS4 only) */

/*
 * Set the ACL by clearing it and adding entries one at a time.
 * Unlike the POSIX.1e ACL routines, you must specify the type
 * (access/default) for each entry.  Internally, the ACL data is just
 * a soup of entries.  API calls here allow you to retrieve just the
 * entries of interest.  This design (which goes against the spirit of
 * POSIX.1e) is useful for handling archive formats that combine
 * default and access information in a single ACL list.
 */
__LA_DECL void	 archive_entry_acl_clear(ArchiveEntry *);
__LA_DECL int	 archive_entry_acl_add_entry(ArchiveEntry *, int /* type */, int /* permset */, int /* tag */, int /* qual */, const char * /* name */);
__LA_DECL int	 archive_entry_acl_add_entry_w(ArchiveEntry *, int /* type */, int /* permset */, int /* tag */, int /* qual */, const wchar_t * /* name */);

/*
 * To retrieve the ACL, first "reset", then repeatedly ask for the
 * "next" entry.  The want_type parameter allows you to request only
 * certain types of entries.
 */
__LA_DECL int	 archive_entry_acl_reset(ArchiveEntry *, int /* want_type */);
__LA_DECL int	 archive_entry_acl_next(ArchiveEntry *, int /* want_type */, int * /* type */, int * /* permset */, int * /* tag */, int * /* qual */, const char ** /* name */);

/*
 * Construct a text-format ACL.  The flags argument is a bitmask that
 * can include any of the following:
 *
 * Flags only for archive entries with POSIX.1e ACL:
 * ARCHIVE_ENTRY_ACL_TYPE_ACCESS - Include POSIX.1e "access" entries.
 * ARCHIVE_ENTRY_ACL_TYPE_DEFAULT - Include POSIX.1e "default" entries.
 * ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT - Include "default:" before each
 *    default ACL entry.
 * ARCHIVE_ENTRY_ACL_STYLE_SOLARIS - Output only one colon after "other" and
 *    "mask" entries.
 *
 * Flags only for archive entries with NFSv4 ACL:
 * ARCHIVE_ENTRY_ACL_STYLE_COMPACT - Do not output the minus character for
 *    unset permissions and flags in NFSv4 ACL permission and flag fields
 *
 * Flags for for archive entries with POSIX.1e ACL or NFSv4 ACL:
 * ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID - Include extra numeric ID field in
 *    each ACL entry.
 * ARCHIVE_ENTRY_ACL_STYLE_SEPARATOR_COMMA - Separate entries with comma
 *    instead of newline.
 */
#define	ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID	0x00000001
#define	ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT	0x00000002
#define	ARCHIVE_ENTRY_ACL_STYLE_SOLARIS		0x00000004
#define	ARCHIVE_ENTRY_ACL_STYLE_SEPARATOR_COMMA	0x00000008
#define	ARCHIVE_ENTRY_ACL_STYLE_COMPACT		0x00000010

__LA_DECL wchar_t *archive_entry_acl_to_text_w(ArchiveEntry *, la_ssize_t * /* len */, int /* flags */);
__LA_DECL char *archive_entry_acl_to_text(ArchiveEntry *, la_ssize_t * /* len */, int /* flags */);
__LA_DECL int archive_entry_acl_from_text_w(ArchiveEntry *, const wchar_t * /* wtext */, int /* type */);
__LA_DECL int archive_entry_acl_from_text(ArchiveEntry *, const char * /* text */, int /* type */);

/* Deprecated constants */
#define	OLD_ARCHIVE_ENTRY_ACL_STYLE_EXTRA_ID		1024
#define	OLD_ARCHIVE_ENTRY_ACL_STYLE_MARK_DEFAULT	2048

/* Deprecated functions */
__LA_DECL const wchar_t	*archive_entry_acl_text_w(ArchiveEntry *, int /* flags */) __LA_DEPRECATED;
__LA_DECL const char *archive_entry_acl_text(ArchiveEntry *, int /* flags */) __LA_DEPRECATED;

/* Return bitmask of ACL types in an archive entry */
__LA_DECL int	 archive_entry_acl_types(ArchiveEntry *);

/* Return a count of entries matching 'want_type' */
__LA_DECL int	 archive_entry_acl_count(ArchiveEntry *, int /* want_type */);

/* Return an opaque ACL object. */
/* There's not yet anything clients can actually do with this... */
struct archive_acl;
__LA_DECL archive_acl *archive_entry_acl(ArchiveEntry *);
/*
 * extended attributes
 */

__LA_DECL void	 archive_entry_xattr_clear(ArchiveEntry *);
__LA_DECL void	 archive_entry_xattr_add_entry(ArchiveEntry *, const char * /* name */, const void * /* value */, size_t /* size */);
/*
 * To retrieve the xattr list, first "reset", then repeatedly ask for the
 * "next" entry.
 */

__LA_DECL int	archive_entry_xattr_count(ArchiveEntry *);
__LA_DECL int	archive_entry_xattr_reset(ArchiveEntry *);
__LA_DECL int	archive_entry_xattr_next(ArchiveEntry *, const char ** /* name */, const void ** /* value */, size_t *);
/*
 * sparse
 */
__LA_DECL void	 archive_entry_sparse_clear(ArchiveEntry *);
__LA_DECL void	 archive_entry_sparse_add_entry(ArchiveEntry *, la_int64_t /* offset */, la_int64_t /* length */);
/*
 * To retrieve the xattr list, first "reset", then repeatedly ask for the
 * "next" entry.
 */

__LA_DECL int	archive_entry_sparse_count(ArchiveEntry *);
__LA_DECL int	archive_entry_sparse_reset(ArchiveEntry *);
__LA_DECL int	archive_entry_sparse_next(ArchiveEntry *, la_int64_t * /* offset */, la_int64_t * /* length */);

/*
 * Utility to match up hardlinks.
 *
 * The 'struct archive_entry_linkresolver' is a cache of archive entries
 * for files with multiple links.  Here's how to use it:
 *   1. Create a lookup object with archive_entry_linkresolver_new()
 *   2. Tell it the archive format you're using.
 *   3. Hand each archive_entry to archive_entry_linkify().
 *      That function will return 0, 1, or 2 entries that should
 *      be written.
 *   4. Call archive_entry_linkify(resolver, NULL) until
 *      no more entries are returned.
 *   5. Call archive_entry_linkresolver_free(resolver) to free resources.
 *
 * The entries returned have their hardlink and size fields updated
 * appropriately.  If an entry is passed in that does not refer to
 * a file with multiple links, it is returned unchanged.  The intention
 * is that you should be able to simply filter all entries through
 * this machine.
 *
 * To make things more efficient, be sure that each entry has a valid
 * nlinks value.  The hardlink cache uses this to track when all links
 * have been found.  If the nlinks value is zero, it will keep every
 * name in the cache indefinitely, which can use a lot of memory.
 *
 * Note that archive_entry_size() is reset to zero if the file
 * body should not be written to the archive.  Pay attention!
 */
struct archive_entry_linkresolver;
/*
 * There are three different strategies for marking hardlinks.
 * The descriptions below name them after the best-known
 * formats that rely on each strategy:
 *
 * "Old cpio" is the simplest, it always returns any entry unmodified.
 *    As far as I know, only cpio formats use this.  Old cpio archives
 *    store every link with the full body; the onus is on the dearchiver
 *    to detect and properly link the files as they are restored.
 * "tar" is also pretty simple; it caches a copy the first time it sees
 *    any link.  Subsequent appearances are modified to be hardlink
 *    references to the first one without any body.  Used by all tar
 *    formats, although the newest tar formats permit the "old cpio" strategy
 *    as well.  This strategy is very simple for the dearchiver,
 *    and reasonably straightforward for the archiver.
 * "new cpio" is trickier.  It stores the body only with the last
 *    occurrence.  The complication is that we might not
 *    see every link to a particular file in a single session, so
 *    there's no easy way to know when we've seen the last occurrence.
 *    The solution here is to queue one link until we see the next.
 *    At the end of the session, you can enumerate any remaining
 *    entries by calling archive_entry_linkify(NULL) and store those
 *    bodies.  If you have a file with three links l1, l2, and l3,
 *    you'll get the following behavior if you see all three links:
 *     linkify(l1) => NULL   (the resolver stores l1 internally)
 *     linkify(l2) => l1     (resolver stores l2, you write l1)
 *     linkify(l3) => l2, l3 (all links seen, you can write both).
 *    If you only see l1 and l2, you'll get this behavior:
 *     linkify(l1) => NULL
 *     linkify(l2) => l1
 *     linkify(NULL) => l2   (at end, you retrieve remaining links)
 *    As the name suggests, this strategy is used by newer cpio variants.
 *    It's noticeably more complex for the archiver, slightly more complex
 *    for the dearchiver than the tar strategy, but makes it straightforward
 *    to restore a file using any link by simply continuing to scan until
 *    you see a link that is stored with a body.  In contrast, the tar
 *    strategy requires you to rescan the archive from the beginning to
 *    correctly extract an arbitrary link.
 */

__LA_DECL struct archive_entry_linkresolver *archive_entry_linkresolver_new(void);
__LA_DECL void archive_entry_linkresolver_set_strategy(struct archive_entry_linkresolver *, int /* format_code */);
__LA_DECL void archive_entry_linkresolver_free(struct archive_entry_linkresolver *);
__LA_DECL void archive_entry_linkify(struct archive_entry_linkresolver *, ArchiveEntry **, ArchiveEntry **);
__LA_DECL ArchiveEntry *archive_entry_partial_links(struct archive_entry_linkresolver *res, uint *links);
#ifdef __cplusplus
}
#endif

/* This is meaningless outside of this header. */
#undef __LA_DECL

#endif /* !ARCHIVE_ENTRY_H_INCLUDED */
