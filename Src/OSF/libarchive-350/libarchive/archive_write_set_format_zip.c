/*-
 * Copyright (c) 2008 Anselm Strauss
 * Copyright (c) 2009 Joerg Sonnenberger
 * Copyright (c) 2011-2012,2014 Michihiro NAKAJIMA
 * All rights reserved.
 */
/*
 * Development supported by Google Summer of Code 2008.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD: head/lib/libarchive/archive_write_set_format_zip.c 201168 2009-12-29 06:15:32Z kientzle $");

#ifdef HAVE_LANGINFO_H
	#include <langinfo.h>
#endif
#include "archive_cryptor_private.h"
#include "archive_hmac_private.h"
#include "archive_random_private.h"
#ifndef HAVE_ZLIB_H
	#include "archive_crc32.h"
#endif

#define ZIP_ENTRY_FLAG_ENCRYPTED        (1<<0)
#define ZIP_ENTRY_FLAG_LENGTH_AT_END    (1<<3)
#define ZIP_ENTRY_FLAG_UTF8_NAME        (1 << 11)

#define ZIP_4GB_MAX ARCHIVE_LITERAL_LL(0xffffffff)
#define ZIP_4GB_MAX_UNCOMPRESSED ARCHIVE_LITERAL_LL(0xff000000)

enum compression {
	COMPRESSION_UNSPECIFIED = -1,
	COMPRESSION_STORE = 0,
	COMPRESSION_DEFLATE = 8
};

#ifdef HAVE_ZLIB_H
	#define COMPRESSION_DEFAULT     COMPRESSION_DEFLATE
#else
	#define COMPRESSION_DEFAULT     COMPRESSION_STORE
#endif

enum encryption {
	ENCRYPTION_NONE = 0,
	ENCRYPTION_TRADITIONAL, /* Traditional PKWARE encryption. */
	ENCRYPTION_WINZIP_AES128, /* WinZIP AES-128 encryption. */
	ENCRYPTION_WINZIP_AES256, /* WinZIP AES-256 encryption. */
};

#define TRAD_HEADER_SIZE        12
/*
 * See "WinZip - AES Encryption Information"
 *     http://www.winzip.com/aes_info.htm
 */
/* Value used in compression method. */
#define WINZIP_AES_ENCRYPTION   99
/* A WinZip AES header size which is stored at the beginning of
 * file contents. */
#define WINZIP_AES128_HEADER_SIZE       (8 + 2)
#define WINZIP_AES256_HEADER_SIZE       (16 + 2)
/* AES vendor version. */
#define AES_VENDOR_AE_1 0x0001
#define AES_VENDOR_AE_2 0x0002
/* Authentication code size. */
#define AUTH_CODE_SIZE          10
/**/
#define MAX_DERIVED_KEY_BUF_SIZE (AES_MAX_KEY_SIZE * 2 + 2)

struct cd_segment {
	struct cd_segment * next;
	size_t buff_size;
	uchar * buff;
	uchar * p;
};

struct trad_enc_ctx {
	uint32 keys[3];
};

struct zip {
	int64 entry_offset;
	int64 entry_compressed_size;
	int64 entry_uncompressed_size;
	int64 entry_compressed_written;
	int64 entry_uncompressed_written;
	int64 entry_uncompressed_limit;
	ArchiveEntry * entry;
	uint32 entry_crc32;
	enum compression entry_compression;
	enum encryption entry_encryption;
	int entry_flags;
	int entry_uses_zip64;
	int experiments;
	struct trad_enc_ctx tctx;
	char tctx_valid;
	uchar trad_chkdat;
	uint  aes_vendor;
	archive_crypto_ctx cctx;
	char cctx_valid;
	archive_hmac_sha1_ctx hctx;
	char hctx_valid;

	uchar * file_header;
	size_t file_header_extra_offset;
	ulong (* crc32func)(ulong crc, const void * buff, size_t len);

	struct cd_segment * central_directory;
	struct cd_segment * central_directory_last;
	size_t central_directory_bytes;
	size_t central_directory_entries;

	int64 written_bytes; /* Overall position in file. */

	archive_string_conv * opt_sconv;
	archive_string_conv * sconv_default;
	enum compression requested_compression;
	int deflate_compression_level;
	int init_default_conversion;
	enum encryption encryption_type;

#define ZIP_FLAG_AVOID_ZIP64 1
#define ZIP_FLAG_FORCE_ZIP64 2
#define ZIP_FLAG_EXPERIMENT_xl 4
	int flags;
#ifdef HAVE_ZLIB_H
	z_stream stream;
#endif
	size_t len_buf;
	uchar * buf;
};

// Don't call this min or MIN, since those are already defined on lots of platforms (but not all)
//#define zipmin_Removed(a, b) ((a) > (b) ? (b) : (a))

static ssize_t archive_write_zip_data(struct archive_write *, const void * buff, size_t s);
static int archive_write_zip_close(struct archive_write *);
static int archive_write_zip_free(struct archive_write *);
static int archive_write_zip_finish_entry(struct archive_write *);
static int archive_write_zip_header(struct archive_write *, ArchiveEntry *);
static int archive_write_zip_options(struct archive_write *, const char *, const char *);
static uint dos_time(const time_t);
static size_t path_length(ArchiveEntry *);
static int write_path(ArchiveEntry *, struct archive_write *);
static void copy_path(ArchiveEntry *, uchar *);
static archive_string_conv * get_sconv(struct archive_write *, struct zip *);
static int trad_enc_init(struct trad_enc_ctx *, const char *, size_t);
static uint trad_enc_encrypt_update(struct trad_enc_ctx *, const uint8 *, size_t, uint8 *, size_t);
static int init_traditional_pkware_encryption(struct archive_write *);
static int is_traditional_pkware_encryption_supported();
static int init_winzip_aes_encryption(struct archive_write *);
static int is_winzip_aes_encryption_supported(int encryption);

static uchar * cd_alloc(struct zip * zip, size_t length)
{
	uchar * p;
	if(zip->central_directory == NULL || (zip->central_directory_last->p + length > zip->central_directory_last->buff + zip->central_directory_last->buff_size)) {
		struct cd_segment * segment = static_cast<struct cd_segment *>(SAlloc::C(1, sizeof(*segment)));
		if(segment == NULL)
			return NULL;
		segment->buff_size = 64 * 1024;
		segment->buff = static_cast<uchar *>(SAlloc::M(segment->buff_size));
		if(segment->buff == NULL) {
			SAlloc::F(segment);
			return NULL;
		}
		segment->p = segment->buff;
		if(zip->central_directory == NULL) {
			zip->central_directory = zip->central_directory_last = segment;
		}
		else {
			zip->central_directory_last->next = segment;
			zip->central_directory_last = segment;
		}
	}
	p = zip->central_directory_last->p;
	zip->central_directory_last->p += length;
	zip->central_directory_bytes += length;
	return (p);
}

static ulong real_crc32(ulong crc, const void * buff, size_t len)
{
	return crc32(crc, static_cast<const Byte *>(buff), (uint)len);
}

static ulong fake_crc32(ulong crc, const void * buff, size_t len)
{
	(void)crc; /* UNUSED */
	(void)buff; /* UNUSED */
	(void)len; /* UNUSED */
	return 0;
}

static int archive_write_zip_options(struct archive_write * a, const char * key, const char * val)
{
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	int ret = ARCHIVE_FAILED;
	if(sstreq(key, "compression")) {
		/*
		 * Set compression to use on all future entries.
		 * This only affects regular files.
		 */
		if(isempty(val)) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "%s: compression option needs a compression name", a->format_name);
		}
		else if(sstreq(val, "deflate")) {
#ifdef HAVE_ZLIB_H
			zip->requested_compression = COMPRESSION_DEFLATE;
			ret = ARCHIVE_OK;
#else
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "deflate compression not supported");
#endif
		}
		else if(sstreq(val, "store")) {
			zip->requested_compression = COMPRESSION_STORE;
			ret = ARCHIVE_OK;
		}
		return ret;
	}
	else if(sstreq(key, "compression-level")) {
		if(val == NULL || !(val[0] >= '0' && val[0] <= '9') || val[1] != '\0') {
			return ARCHIVE_WARN;
		}
		if(val[0] == '0') {
			zip->requested_compression = COMPRESSION_STORE;
			return ARCHIVE_OK;
		}
		else {
#ifdef HAVE_ZLIB_H
			zip->requested_compression = COMPRESSION_DEFLATE;
			zip->deflate_compression_level = val[0] - '0';
			return ARCHIVE_OK;
#else
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "deflate compression not supported");
#endif
		}
	}
	else if(sstreq(key, "encryption")) {
		if(val == NULL) {
			zip->encryption_type = ENCRYPTION_NONE;
			ret = ARCHIVE_OK;
		}
		else if(val[0] == '1' || sstreq(val, "traditional") || sstreq(val, "zipcrypt") || sstreq(val, "ZipCrypt")) {
			if(is_traditional_pkware_encryption_supported()) {
				zip->encryption_type = ENCRYPTION_TRADITIONAL;
				ret = ARCHIVE_OK;
			}
			else {
				archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "encryption not supported");
			}
		}
		else if(sstreq(val, "aes128")) {
			if(is_winzip_aes_encryption_supported(ENCRYPTION_WINZIP_AES128)) {
				zip->encryption_type = ENCRYPTION_WINZIP_AES128;
				ret = ARCHIVE_OK;
			}
			else {
				archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "encryption not supported");
			}
		}
		else if(sstreq(val, "aes256")) {
			if(is_winzip_aes_encryption_supported(ENCRYPTION_WINZIP_AES256)) {
				zip->encryption_type = ENCRYPTION_WINZIP_AES256;
				ret = ARCHIVE_OK;
			}
			else {
				archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "encryption not supported");
			}
		}
		else {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "%s: unknown encryption '%s'", a->format_name, val);
		}
		return ret;
	}
	else if(sstreq(key, "experimental")) {
		if(isempty(val)) {
			zip->flags &= ~ZIP_FLAG_EXPERIMENT_xl;
		}
		else {
			zip->flags |= ZIP_FLAG_EXPERIMENT_xl;
		}
		return ARCHIVE_OK;
	}
	else if(sstreq(key, "fakecrc32")) {
		//
		// FOR TESTING ONLY:  disable CRC calculation to speed up certain complex tests.
		//
		if(isempty(val)) {
			zip->crc32func = real_crc32;
		}
		else {
			zip->crc32func = fake_crc32;
		}
		return ARCHIVE_OK;
	}
	else if(sstreq(key, "hdrcharset")) {
		/*
		 * Set the character set used in translating filenames.
		 */
		if(isempty(val)) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "%s: hdrcharset option needs a character-set name", a->format_name);
		}
		else {
			zip->opt_sconv = archive_string_conversion_to_charset(&a->archive, val, 0);
			if(zip->opt_sconv)
				ret = ARCHIVE_OK;
			else
				ret = ARCHIVE_FATAL;
		}
		return ret;
	}
	else if(sstreq(key, "zip64")) {
		/*
		 * Bias decisions about Zip64: force them to be
		 * generated in certain cases where they are not
		 * forbidden or avoid them in certain cases where they
		 * are not strictly required.
		 */
		if(val && *val != '\0') {
			zip->flags |= ZIP_FLAG_FORCE_ZIP64;
			zip->flags &= ~ZIP_FLAG_AVOID_ZIP64;
		}
		else {
			zip->flags &= ~ZIP_FLAG_FORCE_ZIP64;
			zip->flags |= ZIP_FLAG_AVOID_ZIP64;
		}
		return ARCHIVE_OK;
	}
	// Note: The "warn" return is just to inform the options supervisor that we didn't handle it.  It will generate a suitable error if no one used this option
	return ARCHIVE_WARN;
}

int archive_write_zip_set_compression_deflate(Archive * _a)
{
	struct archive_write * a = (struct archive_write *)_a;
	int ret = ARCHIVE_FAILED;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW | ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA, __FUNCTION__);
	if(a->archive.archive_format != ARCHIVE_FORMAT_ZIP) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Can only use archive_write_zip_set_compression_deflate with zip format");
		ret = ARCHIVE_FATAL;
	}
	else {
#ifdef HAVE_ZLIB_H
		struct zip * zip = static_cast<struct zip *>(a->format_data);
		zip->requested_compression = COMPRESSION_DEFLATE;
		ret = ARCHIVE_OK;
#else
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "deflate compression not supported");
		ret = ARCHIVE_FAILED;
#endif
	}
	return ret;
}

int archive_write_zip_set_compression_store(Archive * _a)
{
	struct archive_write * a = (struct archive_write *)_a;
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	int ret = ARCHIVE_FAILED;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW | ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA, __FUNCTION__);
	if(a->archive.archive_format != ARCHIVE_FORMAT_ZIP) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Can only use archive_write_zip_set_compression_store with zip format");
		ret = ARCHIVE_FATAL;
	}
	else {
		zip->requested_compression = COMPRESSION_STORE;
		ret = ARCHIVE_OK;
	}
	return ret;
}

int archive_write_set_format_zip(Archive * _a)
{
	struct archive_write * a = (struct archive_write *)_a;
	struct zip * zip;
	archive_check_magic(_a, ARCHIVE_WRITE_MAGIC, ARCHIVE_STATE_NEW, __FUNCTION__);
	/* If another format was already registered, unregister it. */
	if(a->format_free)
		(a->format_free)(a);
	zip = (struct zip *)SAlloc::C(1, sizeof(*zip));
	if(zip == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate zip data");
		return ARCHIVE_FATAL;
	}
	/* "Unspecified" lets us choose the appropriate compression. */
	zip->requested_compression = COMPRESSION_UNSPECIFIED;
#ifdef HAVE_ZLIB_H
	zip->deflate_compression_level = Z_DEFAULT_COMPRESSION;
#endif
	zip->crc32func = real_crc32;
	/* A buffer used for both compression and encryption. */
	zip->len_buf = 65536;
	zip->buf = static_cast<uchar *>(SAlloc::M(zip->len_buf));
	if(zip->buf == NULL) {
		SAlloc::F(zip);
		archive_set_error(&a->archive, ENOMEM, "Can't allocate compression buffer");
		return ARCHIVE_FATAL;
	}
	a->format_data = zip;
	a->format_name = "zip";
	a->format_options = archive_write_zip_options;
	a->format_write_header = archive_write_zip_header;
	a->format_write_data = archive_write_zip_data;
	a->format_finish_entry = archive_write_zip_finish_entry;
	a->format_close = archive_write_zip_close;
	a->format_free = archive_write_zip_free;
	a->archive.archive_format = ARCHIVE_FORMAT_ZIP;
	a->archive.archive_format_name = "ZIP";
	return ARCHIVE_OK;
}

static int is_all_ascii(const char * p)
{
	const uchar * pp = (const uchar *)p;
	while(*pp) {
		if(*pp++ > 127)
			return 0;
	}
	return 1;
}

static int archive_write_zip_header(struct archive_write * a, ArchiveEntry * entry)
{
	uchar local_header[32];
	uchar local_extra[144];
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	uchar * e;
	uchar * cd_extra;
	size_t filename_length;
	const char * slink = NULL;
	size_t slink_size = 0;
	archive_string_conv * sconv = get_sconv(a, zip);
	int ret, ret2 = ARCHIVE_OK;
	int version_needed = 10;
	// Ignore types of entries that we don't support
	mode_t type = archive_entry_filetype(entry);
	if(type != AE_IFREG && type != AE_IFDIR && type != AE_IFLNK) {
		__archive_write_entry_filetype_unsupported(&a->archive, entry, "zip");
		return ARCHIVE_FAILED;
	}
	// If we're not using Zip64, reject large files
	if(zip->flags & ZIP_FLAG_AVOID_ZIP64) {
		// Reject entries over 4GB
		if(archive_entry_size_is_set(entry) && (archive_entry_size(entry) > ZIP_4GB_MAX)) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Files > 4GB require Zip64 extensions");
			return ARCHIVE_FAILED;
		}
		// Reject entries if archive is > 4GB
		if(zip->written_bytes > ZIP_4GB_MAX) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Archives > 4GB require Zip64 extensions");
			return ARCHIVE_FAILED;
		}
	}
	// Only regular files can have size > 0
	if(type != AE_IFREG)
		archive_entry_set_size(entry, 0);
	// Reset information from last entry
	zip->entry_offset = zip->written_bytes;
	zip->entry_uncompressed_limit = INT64_MAX;
	zip->entry_compressed_size = 0;
	zip->entry_uncompressed_size = 0;
	zip->entry_compressed_written = 0;
	zip->entry_uncompressed_written = 0;
	zip->entry_flags = 0;
	zip->entry_uses_zip64 = 0;
	zip->entry_crc32 = zip->crc32func(0, NULL, 0);
	zip->entry_encryption = static_cast<encryption>(0);
	archive_entry_free(zip->entry);
	zip->entry = NULL;
	if(zip->cctx_valid)
		archive_encrypto_aes_ctr_release(&zip->cctx);
	if(zip->hctx_valid)
		archive_hmac_sha1_cleanup(&zip->hctx);
	zip->tctx_valid = zip->cctx_valid = zip->hctx_valid = 0;
	if(type == AE_IFREG &&(!archive_entry_size_is_set(entry) || archive_entry_size(entry) > 0)) {
		switch(zip->encryption_type) {
			case ENCRYPTION_TRADITIONAL:
			case ENCRYPTION_WINZIP_AES128:
			case ENCRYPTION_WINZIP_AES256:
			    zip->entry_flags |= ZIP_ENTRY_FLAG_ENCRYPTED;
			    zip->entry_encryption = zip->encryption_type;
			    break;
			case ENCRYPTION_NONE:
			default:
			    break;
		}
	}
#if defined(_WIN32) && !defined(__CYGWIN__)
	// Make sure the path separators in pathname, hardlink and symlink are all slash '/', not the Windows path separator '\'
	zip->entry = __la_win_entry_in_posix_pathseparator(entry);
	if(zip->entry == entry)
		zip->entry = archive_entry_clone(entry);
#else
	zip->entry = archive_entry_clone(entry);
#endif
	if(zip->entry == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate zip header data");
		return ARCHIVE_FATAL;
	}
	if(sconv) {
		const char * p;
		size_t len;
		if(archive_entry_pathname_l(entry, &p, &len, sconv) != 0) {
			if(errno == ENOMEM) {
				archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for Pathname");
				return ARCHIVE_FATAL;
			}
			archive_set_error(&a->archive, ARCHIVE_ERRNO_FILE_FORMAT, "Can't translate Pathname '%s' to %s", archive_entry_pathname(entry), archive_string_conversion_charset_name(sconv));
			ret2 = ARCHIVE_WARN;
		}
		if(len > 0)
			archive_entry_set_pathname(zip->entry, p);
		// 
		// There is no standard for symlink handling; we convert
		// it using the same character-set translation that we use for filename.
		// 
		if(type == AE_IFLNK) {
			if(archive_entry_symlink_l(entry, &p, &len, sconv)) {
				if(errno == ENOMEM) {
					archive_set_error(&a->archive, ENOMEM, "Can't allocate memory for Symlink");
					return ARCHIVE_FATAL;
				}
				// No error if we can't convert
			}
			else if(len > 0)
				archive_entry_set_symlink(zip->entry, p);
		}
	}
	// If filename isn't ASCII and we can use UTF-8, set the UTF-8 flag
	if(!is_all_ascii(archive_entry_pathname(zip->entry))) {
		if(zip->opt_sconv) {
			if(sstreq(archive_string_conversion_charset_name(zip->opt_sconv), "UTF-8"))
				zip->entry_flags |= ZIP_ENTRY_FLAG_UTF8_NAME;
#if HAVE_NL_LANGINFO
		}
		else if(sstreq(nl_langinfo(CODESET), "UTF-8")) {
			zip->entry_flags |= ZIP_ENTRY_FLAG_UTF8_NAME;
#endif
		}
	}
	filename_length = path_length(zip->entry);
	// Determine appropriate compression and size for this entry
	if(type == AE_IFLNK) {
		slink = archive_entry_symlink(zip->entry);
		slink_size = sstrlen(slink);
		zip->entry_uncompressed_limit = slink_size;
		zip->entry_compressed_size = slink_size;
		zip->entry_uncompressed_size = slink_size;
		zip->entry_crc32 = zip->crc32func(zip->entry_crc32, (const uchar *)slink, slink_size);
		zip->entry_compression = COMPRESSION_STORE;
		version_needed = 20;
	}
	else if(type != AE_IFREG) {
		zip->entry_compression = COMPRESSION_STORE;
		zip->entry_uncompressed_limit = 0;
		version_needed = 20;
	}
	else if(archive_entry_size_is_set(zip->entry)) {
		int64 size = archive_entry_size(zip->entry);
		int64 additional_size = 0;
		zip->entry_uncompressed_limit = size;
		zip->entry_compression = zip->requested_compression;
		if(zip->entry_compression == COMPRESSION_UNSPECIFIED) {
			zip->entry_compression = COMPRESSION_DEFAULT;
		}
		if(zip->entry_compression == COMPRESSION_STORE) {
			zip->entry_compressed_size = size;
			zip->entry_uncompressed_size = size;
			version_needed = 10;
		}
		else {
			zip->entry_uncompressed_size = size;
			version_needed = 20;
		}

		if(zip->entry_flags & ZIP_ENTRY_FLAG_ENCRYPTED) {
			switch(zip->entry_encryption) {
				case ENCRYPTION_TRADITIONAL:
				    additional_size = TRAD_HEADER_SIZE;
				    version_needed = 20;
				    break;
				case ENCRYPTION_WINZIP_AES128:
				    additional_size = WINZIP_AES128_HEADER_SIZE + AUTH_CODE_SIZE;
				    version_needed = 20;
				    break;
				case ENCRYPTION_WINZIP_AES256:
				    additional_size = WINZIP_AES256_HEADER_SIZE + AUTH_CODE_SIZE;
				    version_needed = 20;
				    break;
				case ENCRYPTION_NONE:
				default:
				    break;
			}
			if(zip->entry_compression == COMPRESSION_STORE)
				zip->entry_compressed_size += additional_size;
		}
		// 
		// Set Zip64 extension in any of the following cases
		// (this was suggested by discussion on info-zip-dev mailing list):
		//   = Zip64 is being forced by user
		//   = File is over 4GiB uncompressed (including encryption header, if any)
		//   = File is close to 4GiB and is being compressed (compression might make file larger)
		// 
		if((zip->flags & ZIP_FLAG_FORCE_ZIP64) || (zip->entry_uncompressed_size + additional_size > ZIP_4GB_MAX) || 
			(zip->entry_uncompressed_size > ZIP_4GB_MAX_UNCOMPRESSED && zip->entry_compression != COMPRESSION_STORE)) {
			zip->entry_uses_zip64 = 1;
			version_needed = 45;
		}
		/* We may know the size, but never the CRC. */
		zip->entry_flags |= ZIP_ENTRY_FLAG_LENGTH_AT_END;
	}
	else {
		/* We don't know the size.  In this case, we prefer
		 * deflate (it has a clear end-of-data marker which
		 * makes length-at-end more reliable) and will
		 * enable Zip64 extensions unless we're told not to.
		 */
		zip->entry_compression = COMPRESSION_DEFAULT;
		zip->entry_flags |= ZIP_ENTRY_FLAG_LENGTH_AT_END;
		if((zip->flags & ZIP_FLAG_AVOID_ZIP64) == 0) {
			zip->entry_uses_zip64 = 1;
			version_needed = 45;
		}
		else if(zip->entry_compression == COMPRESSION_STORE) {
			version_needed = 10;
		}
		else {
			version_needed = 20;
		}
		if(zip->entry_flags & ZIP_ENTRY_FLAG_ENCRYPTED) {
			switch(zip->entry_encryption) {
				case ENCRYPTION_TRADITIONAL:
				case ENCRYPTION_WINZIP_AES128:
				case ENCRYPTION_WINZIP_AES256:
				    if(version_needed < 20)
					    version_needed = 20;
				    break;
				case ENCRYPTION_NONE:
				default:
				    break;
			}
		}
	}
	// Format the local header
	memzero(local_header, sizeof(local_header));
	memcpy(local_header, "PK\003\004", 4);
	archive_le16enc(local_header + 4, version_needed);
	archive_le16enc(local_header + 6, zip->entry_flags);
	if(zip->entry_encryption == ENCRYPTION_WINZIP_AES128 || zip->entry_encryption == ENCRYPTION_WINZIP_AES256)
		archive_le16enc(local_header + 8, WINZIP_AES_ENCRYPTION);
	else
		archive_le16enc(local_header + 8, zip->entry_compression);
	archive_le32enc(local_header + 10, dos_time(archive_entry_mtime(zip->entry)));
	archive_le32enc(local_header + 14, zip->entry_crc32);
	if(zip->entry_uses_zip64) {
		/* Zip64 data in the local header "must" include both
		 * compressed and uncompressed sizes AND those fields
		 * are included only if these are 0xffffffff;
		 * THEREFORE these must be set this way, even if we
		 * know one of them is smaller. */
		archive_le32enc(local_header + 18, ZIP_4GB_MAX);
		archive_le32enc(local_header + 22, ZIP_4GB_MAX);
	}
	else {
		archive_le32enc(local_header + 18, (uint32)zip->entry_compressed_size);
		archive_le32enc(local_header + 22, (uint32)zip->entry_uncompressed_size);
	}
	archive_le16enc(local_header + 26, (uint16)filename_length);
	if(zip->entry_encryption == ENCRYPTION_TRADITIONAL) {
		if(zip->entry_flags & ZIP_ENTRY_FLAG_LENGTH_AT_END)
			zip->trad_chkdat = local_header[11];
		else
			zip->trad_chkdat = local_header[17];
	}
	// Format as much of central directory file header as we can:
	zip->file_header = cd_alloc(zip, 46);
	// If (zip->file_header == NULL) XXXX
	++zip->central_directory_entries;
	memzero(zip->file_header, 46);
	memcpy(zip->file_header, "PK\001\002", 4);
	// "Made by PKZip 2.0 on Unix."
	archive_le16enc(zip->file_header + 4, 3 * 256 + version_needed);
	archive_le16enc(zip->file_header + 6, version_needed);
	archive_le16enc(zip->file_header + 8, zip->entry_flags);
	if(zip->entry_encryption == ENCRYPTION_WINZIP_AES128 || zip->entry_encryption == ENCRYPTION_WINZIP_AES256)
		archive_le16enc(zip->file_header + 10, WINZIP_AES_ENCRYPTION);
	else
		archive_le16enc(zip->file_header + 10, zip->entry_compression);
	archive_le32enc(zip->file_header + 12, dos_time(archive_entry_mtime(zip->entry)));
	archive_le16enc(zip->file_header + 28, (uint16)filename_length);
	// Following Info-Zip, store mode in the "external attributes" field
	archive_le32enc(zip->file_header + 38, ((uint32)archive_entry_mode(zip->entry)) << 16);
	e = cd_alloc(zip, filename_length);
	/* If (e == NULL) XXXX */
	copy_path(zip->entry, e);
	/* Format extra data. */
	memzero(local_extra, sizeof(local_extra));
	e = local_extra;
	/* First, extra blocks that are the same between
	 * the local file header and the central directory.
	 * We format them once and then duplicate them. */
	/* UT timestamp, length depends on what timestamps are set. */
	memcpy(e, "UT", 2);
	archive_le16enc(e + 2, 1 + (archive_entry_mtime_is_set(entry) ? 4 : 0) + (archive_entry_atime_is_set(entry) ? 4 : 0) + (archive_entry_ctime_is_set(entry) ? 4 : 0));
	e += 4;
	*e++ = (archive_entry_mtime_is_set(entry) ? 1 : 0) | (archive_entry_atime_is_set(entry) ? 2 : 0) | (archive_entry_ctime_is_set(entry) ? 4 : 0);
	if(archive_entry_mtime_is_set(entry)) {
		archive_le32enc(e, (uint32)archive_entry_mtime(entry));
		e += 4;
	}
	if(archive_entry_atime_is_set(entry)) {
		archive_le32enc(e, (uint32)archive_entry_atime(entry));
		e += 4;
	}
	if(archive_entry_ctime_is_set(entry)) {
		archive_le32enc(e, (uint32)archive_entry_ctime(entry));
		e += 4;
	}
	/* ux Unix extra data, length 11, version 1 */
	/* @todo If uid < 64k, use 2 bytes, ditto for gid. */
	memcpy(e, "ux\013\000\001", 5);
	e += 5;
	*e++ = 4; /* Length of following UID */
	archive_le32enc(e, (uint32)archive_entry_uid(entry));
	e += 4;
	*e++ = 4; /* Length of following GID */
	archive_le32enc(e, (uint32)archive_entry_gid(entry));
	e += 4;

	/* AES extra data field: WinZIP AES information, ID=0x9901 */
	if((zip->entry_flags & ZIP_ENTRY_FLAG_ENCRYPTED) && (zip->entry_encryption == ENCRYPTION_WINZIP_AES128 || zip->entry_encryption == ENCRYPTION_WINZIP_AES256)) {
		memcpy(e, "\001\231\007\000\001\000AE", 8);
		/* AES vendor version AE-2 does not store a CRC.
		 * WinZip 11 uses AE-1, which does store the CRC,
		 * but it does not store the CRC when the file size
		 * is less than 20 bytes. So we simulate what
		 * WinZip 11 does.
		 * NOTE: WinZip 9.0 and 10.0 uses AE-2 by default. */
		if(archive_entry_size_is_set(zip->entry) && archive_entry_size(zip->entry) < 20) {
			archive_le16enc(e+4, AES_VENDOR_AE_2);
			zip->aes_vendor = AES_VENDOR_AE_2; /* no CRC. */
		}
		else
			zip->aes_vendor = AES_VENDOR_AE_1;
		e += 8;
		/* AES encryption strength. */
		*e++ = (zip->entry_encryption == ENCRYPTION_WINZIP_AES128) ? 1 : 3;
		/* Actual compression method. */
		archive_le16enc(e, zip->entry_compression);
		e += 2;
	}

	/* Copy UT ,ux, and AES-extra into central directory as well. */
	zip->file_header_extra_offset = zip->central_directory_bytes;
	cd_extra = cd_alloc(zip, e - local_extra);
	memcpy(cd_extra, local_extra, e - local_extra);

	/*
	 * Following extra blocks vary between local header and
	 * central directory. These are the local header versions.
	 * Central directory versions get formatted in
	 * archive_write_zip_finish_entry() below.
	 */

	/* "[Zip64 entry] in the local header MUST include BOTH
	 * original [uncompressed] and compressed size fields." */
	if(zip->entry_uses_zip64) {
		uchar * zip64_start = e;
		memcpy(e, "\001\000\020\000", 4);
		e += 4;
		archive_le64enc(e, zip->entry_uncompressed_size);
		e += 8;
		archive_le64enc(e, zip->entry_compressed_size);
		e += 8;
		archive_le16enc(zip64_start + 2, (uint16)(e - (zip64_start + 4)));
	}

	if(zip->flags & ZIP_FLAG_EXPERIMENT_xl) {
		/* Experimental 'xl' extension to improve streaming. */
		uchar * external_info = e;
		int included = 7;
		memcpy(e, "xl\000\000", 4); // 0x6c65 + 2-byte length
		e += 4;
		e[0] = included; /* bitmap of included fields */
		e += 1;
		if(included & 1) {
			archive_le16enc(e, /* "Version created by" */
			    3 * 256 + version_needed);
			e += 2;
		}
		if(included & 2) {
			archive_le16enc(e, 0); /* internal file attributes */
			e += 2;
		}
		if(included & 4) {
			archive_le32enc(e,  /* external file attributes */
			    ((uint32)archive_entry_mode(zip->entry)) << 16);
			e += 4;
		}
		if(included & 8) {
			// Libarchive does not currently support file comments.
		}
		archive_le16enc(external_info + 2, (uint16)(e - (external_info + 4)));
	}

	/* Update local header with size of extra data and write it all out: */
	archive_le16enc(local_header + 28, (uint16)(e - local_extra));

	ret = __archive_write_output(a, local_header, 30);
	if(ret != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	zip->written_bytes += 30;

	ret = write_path(zip->entry, a);
	if(ret <= ARCHIVE_OK)
		return ARCHIVE_FATAL;
	zip->written_bytes += ret;

	ret = __archive_write_output(a, local_extra, e - local_extra);
	if(ret != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	zip->written_bytes += e - local_extra;

	/* For symlinks, write the body now. */
	if(slink) {
		ret = __archive_write_output(a, slink, slink_size);
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		zip->entry_compressed_written += slink_size;
		zip->entry_uncompressed_written += slink_size;
		zip->written_bytes += slink_size;
	}

#ifdef HAVE_ZLIB_H
	if(zip->entry_compression == COMPRESSION_DEFLATE) {
		zip->stream.zalloc = Z_NULL;
		zip->stream.zfree = Z_NULL;
		zip->stream.opaque = Z_NULL;
		zip->stream.next_out = zip->buf;
		zip->stream.avail_out = (uInt)zip->len_buf;
		if(deflateInit2(&zip->stream, zip->deflate_compression_level,
		    Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
			archive_set_error(&a->archive, ENOMEM, "Can't init deflate compressor");
			return ARCHIVE_FATAL;
		}
	}
#endif

	return (ret2);
}

static ssize_t archive_write_zip_data(struct archive_write * a, const void * buff, size_t s)
{
	int ret;
	struct zip * zip = static_cast<struct zip *>(a->format_data);

	if((int64)s > zip->entry_uncompressed_limit)
		s = (size_t)zip->entry_uncompressed_limit;
	zip->entry_uncompressed_written += s;

	if(s == 0) return 0;

	if(zip->entry_flags & ZIP_ENTRY_FLAG_ENCRYPTED) {
		switch(zip->entry_encryption) {
			case ENCRYPTION_TRADITIONAL:
			    /* Initialize traditional PKWARE encryption context. */
			    if(!zip->tctx_valid) {
				    ret = init_traditional_pkware_encryption(a);
				    if(ret != ARCHIVE_OK)
					    return ret;
				    zip->tctx_valid = 1;
			    }
			    break;
			case ENCRYPTION_WINZIP_AES128:
			case ENCRYPTION_WINZIP_AES256:
			    if(!zip->cctx_valid) {
				    ret = init_winzip_aes_encryption(a);
				    if(ret != ARCHIVE_OK)
					    return ret;
				    zip->cctx_valid = zip->hctx_valid = 1;
			    }
			    break;
			case ENCRYPTION_NONE:
			default:
			    break;
		}
	}

	switch(zip->entry_compression) {
		case COMPRESSION_STORE:
		    if(zip->tctx_valid || zip->cctx_valid) {
			    const uint8 * rb = (const uint8 *)buff;
			    const uint8 * const re = rb + s;

			    while(rb < re) {
				    size_t l;

				    if(zip->tctx_valid) {
					    l = trad_enc_encrypt_update(&zip->tctx,
						    rb, re - rb,
						    zip->buf, zip->len_buf);
				    }
				    else {
					    l = zip->len_buf;
					    ret = archive_encrypto_aes_ctr_update(
						    &zip->cctx,
						    rb, re - rb, zip->buf, &l);
					    if(ret < 0) {
						    archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Failed to encrypt file");
						    return ARCHIVE_FAILED;
					    }
					    archive_hmac_sha1_update(&zip->hctx,
						zip->buf, l);
				    }
				    ret = __archive_write_output(a, zip->buf, l);
				    if(ret != ARCHIVE_OK)
					    return ret;
				    zip->entry_compressed_written += l;
				    zip->written_bytes += l;
				    rb += l;
			    }
		    }
		    else {
			    ret = __archive_write_output(a, buff, s);
			    if(ret != ARCHIVE_OK)
				    return ret;
			    zip->written_bytes += s;
			    zip->entry_compressed_written += s;
		    }
		    break;
#ifdef HAVE_ZLIB_H
		case COMPRESSION_DEFLATE:
		    zip->stream.next_in = (uchar *)(uintptr_t)buff;
		    zip->stream.avail_in = (uInt)s;
		    do {
			    ret = deflate(&zip->stream, Z_NO_FLUSH);
			    if(ret == Z_STREAM_ERROR)
				    return ARCHIVE_FATAL;
			    if(zip->stream.avail_out == 0) {
				    if(zip->tctx_valid) {
					    trad_enc_encrypt_update(&zip->tctx,
						zip->buf, zip->len_buf,
						zip->buf, zip->len_buf);
				    }
				    else if(zip->cctx_valid) {
					    size_t outl = zip->len_buf;
					    ret = archive_encrypto_aes_ctr_update(
						    &zip->cctx,
						    zip->buf, zip->len_buf,
						    zip->buf, &outl);
					    if(ret < 0) {
						    archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Failed to encrypt file");
						    return ARCHIVE_FAILED;
					    }
					    archive_hmac_sha1_update(&zip->hctx,
						zip->buf, zip->len_buf);
				    }
				    ret = __archive_write_output(a, zip->buf,
					    zip->len_buf);
				    if(ret != ARCHIVE_OK)
					    return ret;
				    zip->entry_compressed_written += zip->len_buf;
				    zip->written_bytes += zip->len_buf;
				    zip->stream.next_out = zip->buf;
				    zip->stream.avail_out = (uInt)zip->len_buf;
			    }
		    } while(zip->stream.avail_in != 0);
		    break;
#endif
		case COMPRESSION_UNSPECIFIED:
		default:
		    archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Invalid ZIP compression type");
		    return ARCHIVE_FATAL;
	}
	zip->entry_uncompressed_limit -= s;
	if(!zip->cctx_valid || zip->aes_vendor != AES_VENDOR_AE_2)
		zip->entry_crc32 = zip->crc32func(zip->entry_crc32, buff, (uint)s);
	return (s);
}

static int archive_write_zip_finish_entry(struct archive_write * a)
{
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	int ret;
#ifdef HAVE_ZLIB_H
	if(zip->entry_compression == COMPRESSION_DEFLATE) {
		for(;;) {
			size_t remainder;
			ret = deflate(&zip->stream, Z_FINISH);
			if(ret == Z_STREAM_ERROR)
				return ARCHIVE_FATAL;
			remainder = zip->len_buf - zip->stream.avail_out;
			if(zip->tctx_valid) {
				trad_enc_encrypt_update(&zip->tctx,
				    zip->buf, remainder, zip->buf, remainder);
			}
			else if(zip->cctx_valid) {
				size_t outl = remainder;
				ret = archive_encrypto_aes_ctr_update(
					&zip->cctx, zip->buf, remainder,
					zip->buf, &outl);
				if(ret < 0) {
					archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Failed to encrypt file");
					return ARCHIVE_FAILED;
				}
				archive_hmac_sha1_update(&zip->hctx,
				    zip->buf, remainder);
			}
			ret = __archive_write_output(a, zip->buf, remainder);
			if(ret != ARCHIVE_OK)
				return ret;
			zip->entry_compressed_written += remainder;
			zip->written_bytes += remainder;
			zip->stream.next_out = zip->buf;
			if(zip->stream.avail_out != 0)
				break;
			zip->stream.avail_out = (uInt)zip->len_buf;
		}
		deflateEnd(&zip->stream);
	}
#endif
	if(zip->hctx_valid) {
		uint8 hmac[20];
		size_t hmac_len = 20;

		archive_hmac_sha1_final(&zip->hctx, hmac, &hmac_len);
		ret = __archive_write_output(a, hmac, AUTH_CODE_SIZE);
		if(ret != ARCHIVE_OK)
			return ret;
		zip->entry_compressed_written += AUTH_CODE_SIZE;
		zip->written_bytes += AUTH_CODE_SIZE;
	}

	/* Write trailing data descriptor. */
	if((zip->entry_flags & ZIP_ENTRY_FLAG_LENGTH_AT_END) != 0) {
		char d[24];
		memcpy(d, "PK\007\010", 4);
		if(zip->cctx_valid && zip->aes_vendor == AES_VENDOR_AE_2)
			archive_le32enc(d + 4, 0); /* no CRC.*/
		else
			archive_le32enc(d + 4, zip->entry_crc32);
		if(zip->entry_uses_zip64) {
			archive_le64enc(d + 8,
			    (uint64)zip->entry_compressed_written);
			archive_le64enc(d + 16,
			    (uint64)zip->entry_uncompressed_written);
			ret = __archive_write_output(a, d, 24);
			zip->written_bytes += 24;
		}
		else {
			archive_le32enc(d + 8,
			    (uint32)zip->entry_compressed_written);
			archive_le32enc(d + 12,
			    (uint32)zip->entry_uncompressed_written);
			ret = __archive_write_output(a, d, 16);
			zip->written_bytes += 16;
		}
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
	}

	/* Append Zip64 extra data to central directory information. */
	if(zip->entry_compressed_written > ZIP_4GB_MAX || zip->entry_uncompressed_written > ZIP_4GB_MAX || zip->entry_offset > ZIP_4GB_MAX) {
		uchar zip64[32];
		uchar * z = zip64, * zd;
		memcpy(z, "\001\000\000\000", 4);
		z += 4;
		if(zip->entry_uncompressed_written >= ZIP_4GB_MAX) {
			archive_le64enc(z, zip->entry_uncompressed_written);
			z += 8;
		}
		if(zip->entry_compressed_written >= ZIP_4GB_MAX) {
			archive_le64enc(z, zip->entry_compressed_written);
			z += 8;
		}
		if(zip->entry_offset >= ZIP_4GB_MAX) {
			archive_le64enc(z, zip->entry_offset);
			z += 8;
		}
		archive_le16enc(zip64 + 2, (uint16)(z - (zip64 + 4)));
		zd = cd_alloc(zip, z - zip64);
		if(zd == NULL) {
			archive_set_error(&a->archive, ENOMEM, "Can't allocate zip data");
			return ARCHIVE_FATAL;
		}
		memcpy(zd, zip64, z - zip64);
		/* Zip64 means version needs to be set to at least 4.5 */
		if(archive_le16dec(zip->file_header + 6) < 45)
			archive_le16enc(zip->file_header + 6, 45);
	}
	/* Fix up central directory file header. */
	if(zip->cctx_valid && zip->aes_vendor == AES_VENDOR_AE_2)
		archive_le32enc(zip->file_header + 16, 0); /* no CRC.*/
	else
		archive_le32enc(zip->file_header + 16, zip->entry_crc32);
	archive_le32enc(zip->file_header + 20, (uint32)MIN(zip->entry_compressed_written, ZIP_4GB_MAX));
	archive_le32enc(zip->file_header + 24, (uint32)MIN(zip->entry_uncompressed_written, ZIP_4GB_MAX));
	archive_le16enc(zip->file_header + 30, (uint16)(zip->central_directory_bytes - zip->file_header_extra_offset));
	archive_le32enc(zip->file_header + 42, (uint32)MIN(zip->entry_offset, ZIP_4GB_MAX));
	return ARCHIVE_OK;
}

static int archive_write_zip_close(struct archive_write * a)
{
	uint8 buff[64];
	int64 offset_start, offset_end;
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	struct cd_segment * segment;
	int ret;
	offset_start = zip->written_bytes;
	segment = zip->central_directory;
	while(segment) {
		ret = __archive_write_output(a,
			segment->buff, segment->p - segment->buff);
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		zip->written_bytes += segment->p - segment->buff;
		segment = segment->next;
	}
	offset_end = zip->written_bytes;

	/* If central dir info is too large, write Zip64 end-of-cd */
	if(offset_end - offset_start > ZIP_4GB_MAX
	   || offset_start > ZIP_4GB_MAX
	   || zip->central_directory_entries > 0xffffUL
	   || (zip->flags & ZIP_FLAG_FORCE_ZIP64)) {
		/* Zip64 end-of-cd record */
		memzero(buff, 56);
		memcpy(buff, "PK\006\006", 4);
		archive_le64enc(buff + 4, 44);
		archive_le16enc(buff + 12, 45);
		archive_le16enc(buff + 14, 45);
		/* This is disk 0 of 0. */
		archive_le64enc(buff + 24, zip->central_directory_entries);
		archive_le64enc(buff + 32, zip->central_directory_entries);
		archive_le64enc(buff + 40, offset_end - offset_start);
		archive_le64enc(buff + 48, offset_start);
		ret = __archive_write_output(a, buff, 56);
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		zip->written_bytes += 56;

		/* Zip64 end-of-cd locator record. */
		memzero(buff, 20);
		memcpy(buff, "PK\006\007", 4);
		archive_le32enc(buff + 4, 0);
		archive_le64enc(buff + 8, offset_end);
		archive_le32enc(buff + 16, 1);
		ret = __archive_write_output(a, buff, 20);
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		zip->written_bytes += 20;
	}
	/* Format and write end of central directory. */
	memzero(buff, sizeof(buff));
	memcpy(buff, "PK\005\006", 4);
	archive_le16enc(buff + 8, (uint16)MIN(0xffffU, zip->central_directory_entries));
	archive_le16enc(buff + 10, (uint16)MIN(0xffffU, zip->central_directory_entries));
	archive_le32enc(buff + 12, (uint32)MIN(ZIP_4GB_MAX, (offset_end - offset_start)));
	archive_le32enc(buff + 16, (uint32)MIN(ZIP_4GB_MAX, offset_start));
	ret = __archive_write_output(a, buff, 22);
	if(ret != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	zip->written_bytes += 22;
	return ARCHIVE_OK;
}

static int archive_write_zip_free(struct archive_write * a)
{
	struct cd_segment * segment;
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	while(zip->central_directory) {
		segment = zip->central_directory;
		zip->central_directory = segment->next;
		SAlloc::F(segment->buff);
		SAlloc::F(segment);
	}
	SAlloc::F(zip->buf);
	archive_entry_free(zip->entry);
	if(zip->cctx_valid)
		archive_encrypto_aes_ctr_release(&zip->cctx);
	if(zip->hctx_valid)
		archive_hmac_sha1_cleanup(&zip->hctx);
	/* @todo Free opt_sconv, sconv_default */

	SAlloc::F(zip);
	a->format_data = NULL;
	return ARCHIVE_OK;
}

/* Convert into MSDOS-style date/time. */
static uint dos_time(const time_t unix_time)
{
	struct tm * t;
	uint dt;
#if defined(HAVE_LOCALTIME_R) || defined(HAVE__LOCALTIME64_S)
	struct tm tmbuf;
#endif
#if defined(HAVE__LOCALTIME64_S)
	errno_t terr;
	__time64_t tmptime;
#endif

	/* This will not preserve time when creating/extracting the archive
	 * on two systems with different time zones. */
#if defined(HAVE_LOCALTIME_R)
	t = localtime_r(&unix_time, &tmbuf);
#elif defined(HAVE__LOCALTIME64_S)
	tmptime = unix_time;
	terr = _localtime64_s(&tmbuf, &tmptime);
	if(terr)
		t = NULL;
	else
		t = &tmbuf;
#else
	t = localtime(&unix_time);
#endif

	/* MSDOS-style date/time is only between 1980-01-01 and 2107-12-31 */
	if(t->tm_year < 1980 - 1900)
		/* Set minimum date/time '1980-01-01 00:00:00'. */
		dt = 0x00210000U;
	else if(t->tm_year > 2107 - 1900)
		/* Set maximum date/time '2107-12-31 23:59:58'. */
		dt = 0xff9fbf7dU;
	else {
		dt = 0;
		dt += ((t->tm_year - 80) & 0x7f) << 9;
		dt += ((t->tm_mon + 1) & 0x0f) << 5;
		dt += (t->tm_mday & 0x1f);
		dt <<= 16;
		dt += (t->tm_hour & 0x1f) << 11;
		dt += (t->tm_min & 0x3f) << 5;
		dt += (t->tm_sec & 0x3e) >> 1; /* Only counting every 2 seconds. */
	}
	return dt;
}

static size_t path_length(ArchiveEntry * entry)
{
	size_t len = 0;
	mode_t type = archive_entry_filetype(entry);
	const char * path = archive_entry_pathname(entry);
	if(path) {
		len = strlen(path);
		if(type == AE_IFDIR && (path[0] == '\0' || path[len - 1] != '/'))
			++len; /* Space for the trailing / */
	}
	return len;
}

static int write_path(ArchiveEntry * entry, struct archive_write * archive)
{
	int ret;
	const char * path = archive_entry_pathname(entry);
	mode_t type = archive_entry_filetype(entry);
	size_t written_bytes = 0;
	if(!path)
		return ARCHIVE_FATAL;
	ret = __archive_write_output(archive, path, strlen(path));
	if(ret != ARCHIVE_OK)
		return ARCHIVE_FATAL;
	written_bytes += strlen(path);
	/* Folders are recognized by a trailing slash. */
	if((type == AE_IFDIR) & (path[strlen(path) - 1] != '/')) {
		ret = __archive_write_output(archive, "/", 1);
		if(ret != ARCHIVE_OK)
			return ARCHIVE_FATAL;
		written_bytes += 1;
	}
	return ((int)written_bytes);
}

static void copy_path(ArchiveEntry * entry, uchar * p)
{
	const char * path = archive_entry_pathname(entry);
	size_t pathlen = strlen(path);
	mode_t type = archive_entry_filetype(entry);
	memcpy(p, path, pathlen);
	/* Folders are recognized by a trailing slash. */
	if((type == AE_IFDIR) && (path[pathlen - 1] != '/'))
		p[pathlen] = '/';
}

static archive_string_conv * get_sconv(struct archive_write * a, struct zip * zip)                                      
{
	if(zip->opt_sconv)
		return (zip->opt_sconv);
	if(!zip->init_default_conversion) {
		zip->sconv_default = archive_string_default_conversion_for_write(&(a->archive));
		zip->init_default_conversion = 1;
	}
	return (zip->sconv_default);
}
/*
   Traditional PKWARE Decryption functions.
 */
static void trad_enc_update_keys(struct trad_enc_ctx * ctx, uint8 c)
{
	uint8 t;
#define CRC32(c, b) (crc32(c ^ 0xffffffffUL, &b, 1) ^ 0xffffffffUL)
	ctx->keys[0] = CRC32(ctx->keys[0], c);
	ctx->keys[1] = (ctx->keys[1] + (ctx->keys[0] & 0xff)) * 134775813L + 1;
	t = static_cast<uint8>((ctx->keys[1] >> 24) & 0xff);
	ctx->keys[2] = CRC32(ctx->keys[2], t);
#undef CRC32
}

static uint8 trad_enc_decrypt_byte(struct trad_enc_ctx * ctx)
{
	uint temp = ctx->keys[2] | 2;
	return (uint8)((temp * (temp ^ 1)) >> 8) & 0xff;
}

static uint trad_enc_encrypt_update(struct trad_enc_ctx * ctx, const uint8 * in, size_t in_len, uint8 * out, size_t out_len)
{
	uint i;
	const uint max = (uint)((in_len < out_len) ? in_len : out_len);
	for(i = 0; i < max; i++) {
		uint8 t = in[i];
		out[i] = t ^ trad_enc_decrypt_byte(ctx);
		trad_enc_update_keys(ctx, t);
	}
	return i;
}

static int trad_enc_init(struct trad_enc_ctx * ctx, const char * pw, size_t pw_len)
{
	ctx->keys[0] = 305419896L;
	ctx->keys[1] = 591751049L;
	ctx->keys[2] = 878082192L;
	for(; pw_len; --pw_len)
		trad_enc_update_keys(ctx, *pw++);
	return 0;
}

static int is_traditional_pkware_encryption_supported(void)
{
	uint8 key[TRAD_HEADER_SIZE];
	if(archive_random(key, sizeof(key)-1) != ARCHIVE_OK)
		return 0;
	return 1;
}

static int init_traditional_pkware_encryption(struct archive_write * a)
{
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	uint8 key[TRAD_HEADER_SIZE];
	uint8 key_encrypted[TRAD_HEADER_SIZE];
	int ret;
	const char * passphrase = __archive_write_get_passphrase(a);
	if(passphrase == NULL) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Encryption needs passphrase");
		return ARCHIVE_FAILED;
	}
	if(archive_random(key, sizeof(key)-1) != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Can't generate random number for encryption");
		return ARCHIVE_FATAL;
	}
	trad_enc_init(&zip->tctx, passphrase, strlen(passphrase));
	/* Set the last key code which will be used as a check code
	 * for verifying passphrase in decryption. */
	key[TRAD_HEADER_SIZE-1] = zip->trad_chkdat;
	trad_enc_encrypt_update(&zip->tctx, key, TRAD_HEADER_SIZE,
	    key_encrypted, TRAD_HEADER_SIZE);
	/* Write encrypted keys in the top of the file content. */
	ret = __archive_write_output(a, key_encrypted, TRAD_HEADER_SIZE);
	if(ret != ARCHIVE_OK)
		return ret;
	zip->written_bytes += TRAD_HEADER_SIZE;
	zip->entry_compressed_written += TRAD_HEADER_SIZE;
	return ret;
}

static int init_winzip_aes_encryption(struct archive_write * a)
{
	struct zip * zip = static_cast<struct zip *>(a->format_data);
	size_t key_len, salt_len;
	uint8 salt[16 + 2];
	uint8 derived_key[MAX_DERIVED_KEY_BUF_SIZE];
	int ret;
	const char * passphrase = __archive_write_get_passphrase(a);
	if(passphrase == NULL) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Encryption needs passphrase");
		return ARCHIVE_FAILED;
	}
	if(zip->entry_encryption == ENCRYPTION_WINZIP_AES128) {
		salt_len = 8;
		key_len = 16;
	}
	else {
		/* AES 256 */
		salt_len = 16;
		key_len = 32;
	}
	if(archive_random(salt, salt_len) != ARCHIVE_OK) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Can't generate random number for encryption");
		return ARCHIVE_FATAL;
	}
	archive_pbkdf2_sha1(passphrase, strlen(passphrase), salt, salt_len, 1000, derived_key, key_len * 2 + 2);
	ret = archive_encrypto_aes_ctr_init(&zip->cctx, derived_key, key_len);
	if(ret) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Decryption is unsupported due to lack of crypto library");
		return ARCHIVE_FAILED;
	}
	ret = archive_hmac_sha1_init(&zip->hctx, derived_key + key_len, key_len);
	if(ret) {
		archive_encrypto_aes_ctr_release(&zip->cctx);
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Failed to initialize HMAC-SHA1");
		return ARCHIVE_FAILED;
	}
	/* Set a password verification value after the 'salt'. */
	salt[salt_len] = derived_key[key_len * 2];
	salt[salt_len + 1] = derived_key[key_len * 2 + 1];
	/* Write encrypted keys in the top of the file content. */
	ret = __archive_write_output(a, salt, salt_len + 2);
	if(ret != ARCHIVE_OK)
		return ret;
	zip->written_bytes += salt_len + 2;
	zip->entry_compressed_written += salt_len + 2;
	return ARCHIVE_OK;
}

static int is_winzip_aes_encryption_supported(int encryption)
{
	size_t key_len, salt_len;
	uint8 salt[16 + 2];
	uint8 derived_key[MAX_DERIVED_KEY_BUF_SIZE];
	archive_crypto_ctx cctx;
	archive_hmac_sha1_ctx hctx;
	int ret;
	if(encryption == ENCRYPTION_WINZIP_AES128) {
		salt_len = 8;
		key_len = 16;
	}
	else {
		/* AES 256 */
		salt_len = 16;
		key_len = 32;
	}
	if(archive_random(salt, salt_len) != ARCHIVE_OK)
		return 0;
	ret = archive_pbkdf2_sha1("p", 1, salt, salt_len, 1000, derived_key, key_len * 2 + 2);
	if(ret)
		return 0;
	ret = archive_encrypto_aes_ctr_init(&cctx, derived_key, key_len);
	if(ret)
		return 0;
	ret = archive_hmac_sha1_init(&hctx, derived_key + key_len,
		key_len);
	archive_encrypto_aes_ctr_release(&cctx);
	if(ret)
		return 0;
	archive_hmac_sha1_cleanup(&hctx);
	return 1;
}
