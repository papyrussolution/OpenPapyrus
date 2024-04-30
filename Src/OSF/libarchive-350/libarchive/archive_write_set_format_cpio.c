/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * Copyright (c) 2011-2012 Michihiro NAKAJIMA
 * All rights reserved.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_set_format_cpio.c 201170 2009-12-29 06:34:23Z kientzle $");

static ssize_t  archive_write_cpio_data(struct archive_write *, const void * buff, size_t s);
static int archive_write_cpio_close(struct archive_write *);
static int archive_write_cpio_free(struct archive_write *);
static int archive_write_cpio_finish_entry(struct archive_write *);
static int archive_write_cpio_header(struct archive_write *, ArchiveEntry *);
static int archive_write_cpio_options(struct archive_write *, const char *, const char *);
static int format_octal(int64, void *, int);
static int64  format_octal_recursive(int64, char *, int);
static int write_header(struct archive_write *, ArchiveEntry *);

struct cpio {
	uint64 entry_bytes_remaining;
	int64 ino_next;
	struct InoList { 
		int64 old; 
		int New;
	} * ino_list;
	size_t ino_list_size;
	size_t ino_list_next;
	archive_string_conv * opt_sconv;
	archive_string_conv * sconv_default;
	int init_default_conversion;
};

#define c_magic_offset 0
#define c_magic_size 6
#define c_dev_offset 6
#define c_dev_size 6
#define c_ino_offset 12
#define c_ino_size 6
#define c_mode_offset 18
#define c_mode_size 6
#define c_uid_offset 24
#define c_uid_size 6
#define c_gid_offset 30
#define c_gid_size 6
#define c_nlink_offset 36
#define c_nlink_size 6
#define c_rdev_offset 42
#define c_rdev_size 6
#define c_mtime_offset 48
#define c_mtime_size 11
#define c_namesize_offset 59
#define c_namesize_size 6
#define c_filesize_offset 65
#define c_filesize_size 11
/*
 * Set output format to 'cpio' format.
 */
int archive_write_set_format_cpio(Archive * _a)
{
	struct archive_write * a = (struct archive_write *)_a;
	struct cpio * cpio;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	/* If someone else was already registered, unregister them. */
	if(a->format_free)
		(a->format_free)(a);
	cpio = (struct cpio *)SAlloc::C(1, sizeof(*cpio));
	if(cpio == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate cpio data");
		return ARCHIVE_FATAL;
	}
	a->format_data = cpio;
	a->format_name = "cpio";
	a->format_options = archive_write_cpio_options;
	a->format_write_header = archive_write_cpio_header;
	a->format_write_data = archive_write_cpio_data;
	a->format_finish_entry = archive_write_cpio_finish_entry;
	a->format_close = archive_write_cpio_close;
	a->format_free = archive_write_cpio_free;
	a->archive.archive_format = ARCHIVE_FORMAT_CPIO_POSIX;
	a->archive.archive_format_name = "POSIX cpio";
	return ARCHIVE_OK;
}

static int archive_write_cpio_options(struct archive_write * a, const char * key, const char * val)
{
	struct cpio * cpio = (struct cpio *)a->format_data;
	int ret = ARCHIVE_FAILED;
	if(sstreq(key, "hdrcharset")) {
		if(isempty(val))
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "%s: hdrcharset option needs a character-set name", a->format_name);
		else {
			cpio->opt_sconv = archive_string_conversion_to_charset(&a->archive, val, 0);
			if(cpio->opt_sconv != NULL)
				ret = ARCHIVE_OK;
			else
				ret = ARCHIVE_FATAL;
		}
		return ret;
	}
	// Note: The "warn" return is just to inform the options supervisor that we didn't handle it.  It will generate a suitable error if no one used this option
	return ARCHIVE_WARN;
}

/*
 * Ino values are as long as 64 bits on some systems; cpio format
 * only allows 18 bits and relies on the ino values to identify hardlinked
 * files.  So, we can't merely "hash" the ino numbers since collisions
 * would corrupt the archive.  Instead, we generate synthetic ino values
 * to store in the archive and maintain a map of original ino values to
 * synthetic ones so we can preserve hardlink information.
 *
 * TODO: Make this more efficient.  It's not as bad as it looks (most
 * files don't have any hardlinks and we don't do any work here for those),
 * but it wouldn't be hard to do better.
 *
 * TODO: Work with dev/ino pairs here instead of just ino values.
 */
static int synthesize_ino_value(struct cpio * cpio, ArchiveEntry * entry)
{
	int64 ino = archive_entry_ino64(entry);
	int ino_new;
	size_t i;
	/*
	 * If no index number was given, don't assign one.  In
	 * particular, this handles the end-of-archive marker
	 * correctly by giving it a zero index value.  (This is also
	 * why we start our synthetic index numbers with one below.)
	 */
	if(ino == 0)
		return 0;
	/* Don't store a mapping if we don't need to. */
	if(archive_entry_nlink(entry) < 2) {
		return (int)(++cpio->ino_next);
	}
	/* Look up old ino; if we have it, this is a hardlink
	 * and we reuse the same value. */
	for(i = 0; i < cpio->ino_list_next; ++i) {
		if(cpio->ino_list[i].old == ino)
			return (cpio->ino_list[i].New);
	}
	/* Assign a new index number. */
	ino_new = (int)(++cpio->ino_next);
	/* Ensure space for the new mapping. */
	if(cpio->ino_list_size <= cpio->ino_list_next) {
		size_t newsize = cpio->ino_list_size < 512 ? 512 : cpio->ino_list_size * 2;
		void * newlist = SAlloc::R(cpio->ino_list, sizeof(cpio->ino_list[0]) * newsize);
		if(newlist == NULL)
			return -1;
		cpio->ino_list_size = newsize;
		cpio->ino_list = static_cast<cpio::InoList *>(newlist);
	}
	/* Record and return the new value. */
	cpio->ino_list[cpio->ino_list_next].old = ino;
	cpio->ino_list[cpio->ino_list_next].New = ino_new;
	++cpio->ino_list_next;
	return (ino_new);
}

static archive_string_conv * get_sconv(struct archive_write * a)                                     
{
	struct cpio * cpio;
	archive_string_conv * sconv;
	cpio = (struct cpio *)a->format_data;
	sconv = cpio->opt_sconv;
	if(sconv == NULL) {
		if(!cpio->init_default_conversion) {
			cpio->sconv_default =
			    archive_string_default_conversion_for_write(
				&(a->archive));
			cpio->init_default_conversion = 1;
		}
		sconv = cpio->sconv_default;
	}
	return (sconv);
}

static int archive_write_cpio_header(struct archive_write * a, ArchiveEntry * entry)
{
	const char * path;
	size_t len;
	if(archive_entry_filetype(entry) == 0 && archive_entry_hardlink(entry) == NULL) {
		archive_set_error(&a->archive, -1, "Filetype required");
		return ARCHIVE_FAILED;
	}
	if(archive_entry_pathname_l(entry, &path, &len, get_sconv(a)) != 0 && errno == ENOMEM) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for Pathname");
		return ARCHIVE_FATAL;
	}
	if(len == 0 || isempty(path)) {
		archive_set_error(&a->archive, -1, "Pathname required");
		return ARCHIVE_FAILED;
	}
	if(!archive_entry_size_is_set(entry) || archive_entry_size(entry) < 0) {
		archive_set_error(&a->archive, -1, "Size required");
		return ARCHIVE_FAILED;
	}
	return write_header(a, entry);
}

static int write_header(struct archive_write * a, ArchiveEntry * entry)
{
	const char * p, * path;
	int pathlength, ret, ret_final;
	int64 ino;
	char h[76];
	archive_string_conv * sconv;
	ArchiveEntry * entry_main;
	size_t len;
	struct cpio * cpio = (struct cpio *)a->format_data;
	ret_final = ARCHIVE_OK;
	sconv = get_sconv(a);
#if defined(_WIN32) && !defined(__CYGWIN__)
	/* Make sure the path separators in pathname, hardlink and symlink
	 * are all slash '/', not the Windows path separator '\'. */
	entry_main = __la_win_entry_in_posix_pathseparator(entry);
	if(entry_main == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate ustar data");
		return(ARCHIVE_FATAL);
	}
	if(entry != entry_main)
		entry = entry_main;
	else
		entry_main = NULL;
#else
	entry_main = NULL;
#endif
	ret = archive_entry_pathname_l(entry, &path, &len, sconv);
	if(ret) {
		if(errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for Pathname");
			ret_final = ARCHIVE_FATAL;
			goto exit_write_header;
		}
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Can't translate pathname '%s' to %s", archive_entry_pathname(entry), archive_string_conversion_charset_name(sconv));
		ret_final = ARCHIVE_WARN;
	}
	/* Include trailing null. */
	pathlength = (int)len + 1;
	memzero(h, sizeof(h));
	format_octal(070707, h + c_magic_offset, c_magic_size);
	format_octal(archive_entry_dev(entry), h + c_dev_offset, c_dev_size);
	ino = synthesize_ino_value(cpio, entry);
	if(ino < 0) {
		archive_set_error(&a->archive, ENOMEM, "No memory for ino translation table");
		ret_final = ARCHIVE_FATAL;
		goto exit_write_header;
	}
	else if(ino > 0777777) {
		archive_set_error(&a->archive, ERANGE, "Too many files for this cpio format");
		ret_final = ARCHIVE_FATAL;
		goto exit_write_header;
	}
	format_octal(ino & 0777777, h + c_ino_offset, c_ino_size);

	/* @todo Set ret_final to ARCHIVE_WARN if any of these overflow. */
	format_octal(archive_entry_mode(entry), h + c_mode_offset, c_mode_size);
	format_octal(archive_entry_uid(entry), h + c_uid_offset, c_uid_size);
	format_octal(archive_entry_gid(entry), h + c_gid_offset, c_gid_size);
	format_octal(archive_entry_nlink(entry), h + c_nlink_offset, c_nlink_size);
	if(archive_entry_filetype(entry) == AE_IFBLK || archive_entry_filetype(entry) == AE_IFCHR)
		format_octal(archive_entry_rdev(entry), h + c_rdev_offset, c_rdev_size);
	else
		format_octal(0, h + c_rdev_offset, c_rdev_size);
	format_octal(archive_entry_mtime(entry), h + c_mtime_offset, c_mtime_size);
	format_octal(pathlength, h + c_namesize_offset, c_namesize_size);
	/* Non-regular files don't store bodies. */
	if(archive_entry_filetype(entry) != AE_IFREG)
		archive_entry_set_size(entry, 0);
	/* Symlinks get the link written as the body of the entry. */
	ret = archive_entry_symlink_l(entry, &p, &len, sconv);
	if(ret) {
		if(errno == ENOMEM) {
			archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for Linkname");
			ret_final = ARCHIVE_FATAL;
			goto exit_write_header;
		}
		archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Can't translate linkname '%s' to %s", archive_entry_symlink(entry), archive_string_conversion_charset_name(sconv));
		ret_final = ARCHIVE_WARN;
	}
	if(len > 0 && p != NULL && *p != '\0')
		ret = format_octal(strlen(p), h + c_filesize_offset, c_filesize_size);
	else
		ret = format_octal(archive_entry_size(entry), h + c_filesize_offset, c_filesize_size);
	if(ret) {
		archive_set_error(&a->archive, ERANGE, "File is too large for cpio format.");
		ret_final = ARCHIVE_FAILED;
		goto exit_write_header;
	}

	ret = __archive_write_output(a, h, sizeof(h));
	if(ret != ARCHIVE_OK) {
		ret_final = ARCHIVE_FATAL;
		goto exit_write_header;
	}

	ret = __archive_write_output(a, path, pathlength);
	if(ret != ARCHIVE_OK) {
		ret_final = ARCHIVE_FATAL;
		goto exit_write_header;
	}
	cpio->entry_bytes_remaining = archive_entry_size(entry);
	/* Write the symlink now. */
	if(p && *p != '\0') {
		ret = __archive_write_output(a, p, strlen(p));
		if(ret != ARCHIVE_OK) {
			ret_final = ARCHIVE_FATAL;
			goto exit_write_header;
		}
	}
exit_write_header:
	archive_entry_free(entry_main);
	return (ret_final);
}

static ssize_t archive_write_cpio_data(struct archive_write * a, const void * buff, size_t s)
{
	int ret;
	struct cpio * cpio = (struct cpio *)a->format_data;
	if(s > cpio->entry_bytes_remaining)
		s = (size_t)cpio->entry_bytes_remaining;
	ret = __archive_write_output(a, buff, s);
	cpio->entry_bytes_remaining -= s;
	if(ret >= 0)
		return (s);
	else
		return ret;
}
/*
 * Format a number into the specified field.
 */
static int format_octal(int64 v, void * p, int digits)
{
	int ret;
	int64 max = (((int64)1) << (digits * 3)) - 1;
	if(v >= 0 && v <= max) {
		format_octal_recursive(v, (char *)p, digits);
		ret = 0;
	}
	else {
		format_octal_recursive(max, (char *)p, digits);
		ret = -1;
	}
	return ret;
}

static int64 format_octal_recursive(int64 v, char * p, int s)
{
	if(s == 0)
		return (v);
	v = format_octal_recursive(v, p+1, s-1);
	*p = '0' + ((char)v & 7);
	return (v >> 3);
}

static int archive_write_cpio_close(struct archive_write * a)
{
	int er;
	ArchiveEntry * trailer = archive_entry_new2(NULL);
	/* nlink = 1 here for GNU cpio compat. */
	archive_entry_set_nlink(trailer, 1);
	archive_entry_set_size(trailer, 0);
	archive_entry_set_pathname(trailer, "TRAILER!!!");
	er = write_header(a, trailer);
	archive_entry_free(trailer);
	return (er);
}

static int archive_write_cpio_free(struct archive_write * a)
{
	struct cpio * cpio = (struct cpio *)a->format_data;
	SAlloc::F(cpio->ino_list);
	SAlloc::F(cpio);
	a->format_data = NULL;
	return ARCHIVE_OK;
}

static int archive_write_cpio_finish_entry(struct archive_write * a)
{
	struct cpio * cpio = (struct cpio *)a->format_data;
	return (__archive_write_nulls(a, (size_t)cpio->entry_bytes_remaining));
}
