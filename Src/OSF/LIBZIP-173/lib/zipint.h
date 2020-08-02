#ifndef _HAD_ZIPINT_H
#define _HAD_ZIPINT_H
/*
   zipint.h -- internal declarations.
   Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

   This file is part of libzip, a library to manipulate ZIP archives.
   The authors can be contacted at <libzip@nih.at>

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
   OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
   IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//#include "config.h"
#include <stdint.h>
#ifndef _HAD_ZIPCONF_H
	#include "zipconf.h"
#endif
/* BEGIN DEFINES */
#define HAVE___PROGNAME
#define HAVE__CLOSE
#define HAVE__DUP
#define HAVE__FDOPEN
#define HAVE__FILENO
#define HAVE__SETMODE
#define HAVE__SNPRINTF
#define HAVE__STRDUP
#define HAVE__STRICMP
#define HAVE__STRTOI64
#define HAVE__STRTOUI64
#define HAVE__UMASK
#define HAVE__UNLINK
//#define HAVE_ARC4RANDOM
//#define HAVE_CLONEFILE
//#define HAVE_COMMONCRYPTO
#define HAVE_CRYPTO
//#define HAVE_FICLONERANGE
#define HAVE_FILENO
//#define HAVE_FSEEKO
//#define HAVE_FTELLO
#define HAVE_GETPROGNAME
//#define HAVE_GNUTLS
#define HAVE_LIBBZ2
#define HAVE_LIBLZMA
//#define HAVE_LOCALTIME_R
//#define HAVE_MBEDTLS
#define HAVE_MKSTEMP
#define HAVE_NULLABLE
#define HAVE_OPENSSL
#define HAVE_SETMODE
#define HAVE_SNPRINTF
//#define HAVE_STRCASECMP
#define HAVE_STRDUP
#define HAVE_STRICMP
#define HAVE_STRTOLL
#define HAVE_STRTOULL
#define HAVE_STRUCT_TM_TM_ZONE
#define HAVE_STDBOOL_H
#define HAVE_STRINGS_H
#define HAVE_UNISTD_H
//#define HAVE_WINDOWS_CRYPTO
#define HAVE_OPENSSL
#define SIZEOF_OFF_T  4
#if defined(_WIN64)
	#define SIZEOF_SIZE_T 8
#elif defined(_WIN32)
	#define SIZEOF_SIZE_T 4
#else
	#define SIZEOF_SIZE_T (sizeof(void *))
#endif
#define HAVE_DIRENT_H
#define HAVE_FTS_H
#define HAVE_NDIR_H
#define HAVE_SYS_DIR_H
#define HAVE_SYS_NDIR_H
#define WORDS_BIGENDIAN
#define HAVE_SHARED
/* END DEFINES */
#define PACKAGE "libzip"
#define VERSION "1.7.3"
// } config.h
//#include "compat.h"
//#include "zipconf.h"
//#include "config.h"

// to have *_MAX definitions for all types when compiling with g++ 
#define __STDC_LIMIT_MACROS
#ifdef _WIN32
	#ifndef ZIP_EXTERN
		#ifndef ZIP_STATIC
			#define ZIP_EXTERN __declspec(dllexport)
		#endif
	#endif
	#include <io.h> // for dup(), close(), etc. 
#endif
#ifdef HAVE_STDBOOL_H
	#include <stdbool.h>
#else
	typedef char bool;
	#define true 1
	#define false 0
#endif
//#include <errno.h>
// at least MinGW does not provide EOPNOTSUPP, see http://sourceforge.net/p/mingw/bugs/263/
#ifndef EOPNOTSUPP
	#define EOPNOTSUPP EINVAL
#endif
// at least MinGW does not provide EOVERFLOW, see http://sourceforge.net/p/mingw/bugs/242/
#ifndef EOVERFLOW
	#define EOVERFLOW EFBIG
#endif
// not supported on at least Windows 
#ifndef O_CLOEXEC
	#define O_CLOEXEC 0
#endif
#ifdef _WIN32
	#if defined(HAVE__CLOSE)
		#define close _close
	#endif
	#if defined(HAVE__DUP)
		#define dup _dup
	#endif
	// crashes reported when using fdopen instead of _fdopen on Windows/Visual Studio 10/Win64 
	#if defined(HAVE__FDOPEN)
		#define fdopen _fdopen
	#endif
	#if !defined(HAVE_FILENO) && defined(HAVE__FILENO)
		#define fileno _fileno
	#endif
	//#if defined(HAVE__SNPRINTF)
		//#define snprintf _snprintf
	//#endif
	#if defined(HAVE__STRDUP)
		#if !defined(HAVE_STRDUP) || defined(_WIN32)
			#undef strdup
			#define strdup _strdup
		#endif
	#endif
	#if !defined(HAVE__SETMODE) && defined(HAVE_SETMODE)
		#define _setmode setmode
	#endif
	#if !defined(HAVE_STRTOLL) && defined(HAVE__STRTOI64)
		#define strtoll _strtoi64
	#endif
	#if !defined(HAVE_STRTOULL) && defined(HAVE__STRTOUI64)
		#define strtoull _strtoui64
	#endif
	#if defined(HAVE__UNLINK)
		#define unlink _unlink
	#endif
#endif
#ifndef HAVE_FSEEKO
	#define fseeko(s, o, w) (fseek((s), (long int)(o), (w)))
#endif
#ifndef HAVE_FTELLO
	#define ftello(s) ((long)ftell((s)))
#endif
#if !defined(HAVE_STRCASECMP)
	#if defined(HAVE__STRICMP)
		#define strcasecmp _stricmp
	#elif defined(HAVE_STRICMP)
		#define strcasecmp stricmp
	#endif
#endif
#if SIZEOF_OFF_T == 8
	#define ZIP_OFF_MAX ZIP_INT64_MAX
	#define ZIP_OFF_MIN ZIP_INT64_MIN
#elif SIZEOF_OFF_T == 4
	#define ZIP_OFF_MAX ZIP_INT32_MAX
	#define ZIP_OFF_MIN ZIP_INT32_MIN
#elif SIZEOF_OFF_T == 2
	#define ZIP_OFF_MAX ZIP_INT16_MAX
	#define ZIP_OFF_MIN ZIP_INT16_MIN
#else
	#error unsupported size of off_t
#endif
#if defined(HAVE_FTELLO) && defined(HAVE_FSEEKO)
	#define ZIP_FSEEK_MAX ZIP_OFF_MAX
	#define ZIP_FSEEK_MIN ZIP_OFF_MIN
#else
	#include <limits.h>
	#define ZIP_FSEEK_MAX LONG_MAX
	#define ZIP_FSEEK_MIN LONG_MIN
#endif
#ifndef SIZE_MAX
	#if SIZEOF_SIZE_T == 8
		#define SIZE_MAX ZIP_INT64_MAX
	#elif SIZEOF_SIZE_T == 4
		#define SIZE_MAX ZIP_INT32_MAX
	#elif SIZEOF_SIZE_T == 2
		#define SIZE_MAX ZIP_INT16_MAX
	#else
		#error unsupported size of size_t
	#endif
#endif
#ifndef PRId64
	#ifdef _MSC_VER
		#define PRId64 "I64d"
	#else
		#define PRId64 "lld"
	#endif
#endif
#ifndef PRIu64
	#ifdef _MSC_VER
		#define PRIu64 "I64u"
	#else
		#define PRIu64 "llu"
	#endif
#endif
#ifndef S_ISDIR
	#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
	#define S_ISREG(mode) (((mode)&S_IFMT) == S_IFREG)
#endif
// } compat.h
#include <slib.h>

#ifndef _ZIP_COMPILING_DEPRECATED
	#define ZIP_DISABLE_DEPRECATED
#endif
#include "zip.h"
#include <zlib.h>

#define CENTRAL_MAGIC "PK\1\2"
#define LOCAL_MAGIC "PK\3\4"
#define EOCD_MAGIC "PK\5\6"
#define DATADES_MAGIC "PK\7\10"
#define EOCD64LOC_MAGIC "PK\6\7"
#define EOCD64_MAGIC "PK\6\6"
#define CDENTRYSIZE 46u
#define LENTRYSIZE 30
#define MAXCOMLEN 65536
#define MAXEXTLEN 65536
#define EOCDLEN 22
#define EOCD64LOCLEN 20
#define EOCD64LEN 56
#define CDBUFSIZE (MAXCOMLEN + EOCDLEN + EOCD64LOCLEN)
#define BUFSIZE 8192
#define EFZIP64SIZE 28
#define EF_WINZIP_AES_SIZE 7
#define MAX_DATA_DESCRIPTOR_LENGTH 24
#define ZIP_CRYPTO_PKWARE_HEADERLEN 12
#define ZIP_CM_REPLACED_DEFAULT (-2)
#define ZIP_CM_WINZIP_AES 99 /* Winzip AES encrypted */
#define WINZIP_AES_PASSWORD_VERIFY_LENGTH 2
#define WINZIP_AES_MAX_HEADER_LENGTH (16 + WINZIP_AES_PASSWORD_VERIFY_LENGTH)
#define AES_BLOCK_SIZE 16
#define HMAC_LENGTH 10
#define SHA1_LENGTH 20
#define SALT_LENGTH(method) ((method) == ZIP_EM_AES_128 ? 8 : ((method) == ZIP_EM_AES_192 ? 12 : 16))
#define ZIP_CM_IS_DEFAULT(x) ((x) == ZIP_CM_DEFAULT || (x) == ZIP_CM_REPLACED_DEFAULT)
#define ZIP_CM_ACTUAL(x) ((uint16)(ZIP_CM_IS_DEFAULT(x) ? ZIP_CM_DEFLATE : (x)))
#define ZIP_EF_UTF_8_COMMENT 0x6375
#define ZIP_EF_UTF_8_NAME 0x7075
#define ZIP_EF_WINZIP_AES 0x9901
#define ZIP_EF_ZIP64 0x0001

#define ZIP_EF_IS_INTERNAL(id) ((id) == ZIP_EF_UTF_8_COMMENT || (id) == ZIP_EF_UTF_8_NAME || (id) == ZIP_EF_WINZIP_AES || (id) == ZIP_EF_ZIP64)
// according to unzip-6.0's zipinfo.c, this corresponds to a regular file with rw permissions for everyone 
#define ZIP_EXT_ATTRIB_DEFAULT (0100666u << 16)
// according to unzip-6.0's zipinfo.c, this corresponds to a directory with rwx permissions for everyone 
#define ZIP_EXT_ATTRIB_DEFAULT_DIR (0040777u << 16)
#define ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS_ALLOWED_MASK 0x0836
//#define ZIP_MAX_Removed(a, b) ((a) > (b) ? (a) : (b))
//#define ZIP_MIN_Removed(a, b) ((a) < (b) ? (a) : (b))

// This section contains API that won't materialize like this.  It's
// placed in the internal section, pending cleanup. 

// flags for compression and encryption sources 
#define ZIP_CODEC_DECODE 0 // decompress/decrypt (encode flag not set) 
#define ZIP_CODEC_ENCODE 1 // compress/encrypt 

typedef zip_source_t *(* zip_encryption_implementation)(zip_t *, zip_source_t *, uint16, int, const char *);

zip_encryption_implementation _zip_get_encryption_implementation(uint16 method, int operation);

/* clang-format off */
enum zip_compression_status {
	ZIP_COMPRESSION_OK,
	ZIP_COMPRESSION_END,
	ZIP_COMPRESSION_ERROR,
	ZIP_COMPRESSION_NEED_DATA
};

/* clang-format on */
typedef enum zip_compression_status zip_compression_status_t;

struct zip_compression_algorithm {
	void *(* allocate)(uint16 method, int compression_flags, zip_error_t * error); // called once to create new context 
	void (* deallocate)(void * ctx); // called once to free context 
	uint16 (* general_purpose_bit_flags)(void * ctx); // get compression specific general purpose bitflags 
	uint8 version_needed; // minimum version needed when using this algorithm 
	bool (* start)(void * ctx); /* start processing */
	bool (* end)(void * ctx); /* stop processing */
	bool (* input)(void * ctx, uint8 * data, uint64 length); /* provide new input data, remains valid until next call to input or end */
	void (* end_of_input)(void * ctx); /* all input data has been provided */
	// process input data, writing to data, which has room for length bytes, update length to number of bytes written 
	zip_compression_status_t (* process)(void * ctx, uint8 * data, uint64 * length);
};

typedef struct zip_compression_algorithm zip_compression_algorithm_t;

extern zip_compression_algorithm_t zip_algorithm_bzip2_compress;
extern zip_compression_algorithm_t zip_algorithm_bzip2_decompress;
extern zip_compression_algorithm_t zip_algorithm_deflate_compress;
extern zip_compression_algorithm_t zip_algorithm_deflate_decompress;
extern zip_compression_algorithm_t zip_algorithm_xz_compress;
extern zip_compression_algorithm_t zip_algorithm_xz_decompress;

/* This API is not final yet, but we need it internally, so it's private for now. */

const uint8 * zip_get_extra_field_by_id(zip_t *, int, int, uint16, int, uint16 *);

/* This section contains API that is of limited use until support for
   user-supplied compression/encryption implementation is finished.
   Thus we will keep it private for now. */

typedef int64 (* zip_source_layered_callback)(zip_source_t *, void *, void *, uint64, enum zip_source_cmd);
zip_source_t * zip_source_compress(zip_t * za, zip_source_t * src, int32 cm, int compression_flags);
zip_source_t * zip_source_crc(zip_t *, zip_source_t *, int);
zip_source_t * zip_source_decompress(zip_t * za, zip_source_t * src, int32 cm);
zip_source_t * zip_source_layered(zip_t *, zip_source_t *, zip_source_layered_callback, void *);
zip_source_t * zip_source_layered_create(zip_source_t * src, zip_source_layered_callback cb, void * ud, zip_error_t * error);
zip_source_t * zip_source_pkware_decode(zip_t *, zip_source_t *, uint16, int, const char *);
zip_source_t * zip_source_pkware_encode(zip_t *, zip_source_t *, uint16, int, const char *);
int zip_source_remove(zip_source_t *);
int64 FASTCALL zip_source_supports(const zip_source_t * src);
zip_source_t * zip_source_window(zip_t *, zip_source_t *, uint64, uint64);
zip_source_t * zip_source_winzip_aes_decode(zip_t *, zip_source_t *, uint16, int, const char *);
zip_source_t * zip_source_winzip_aes_encode(zip_t *, zip_source_t *, uint16, int, const char *);
zip_source_t * zip_source_buffer_with_attributes(zip_t * za, const void * data, uint64 len, int freep, zip_file_attributes_t * attributes);

/* error source for layered sources */

enum zip_les { ZIP_LES_NONE, ZIP_LES_UPPER, ZIP_LES_LOWER, ZIP_LES_INVAL };

/* directory entry: general purpose bit flags */

#define ZIP_GPBF_ENCRYPTED 0x0001u         /* is encrypted */
#define ZIP_GPBF_DATA_DESCRIPTOR 0x0008u   /* crc/size after file data */
#define ZIP_GPBF_STRONG_ENCRYPTION 0x0040u /* uses strong encryption */
#define ZIP_GPBF_ENCODING_UTF_8 0x0800u    /* file name encoding is UTF-8 */

/* extra fields */
#define ZIP_EF_LOCAL ZIP_FL_LOCAL                   /* include in local header */
#define ZIP_EF_CENTRAL ZIP_FL_CENTRAL               /* include in central directory */
#define ZIP_EF_BOTH (ZIP_EF_LOCAL | ZIP_EF_CENTRAL) /* include in both */
#define ZIP_FL_FORCE_ZIP64 1024 /* force zip64 extra field (_zip_dirent_write) */
#define ZIP_FL_ENCODING_ALL (ZIP_FL_ENC_GUESS | ZIP_FL_ENC_CP437 | ZIP_FL_ENC_UTF_8)

/* encoding type */
enum zip_encoding_type {
	ZIP_ENCODING_UNKNOWN,  /* not yet analyzed */
	ZIP_ENCODING_ASCII,    /* plain ASCII */
	ZIP_ENCODING_UTF8_KNOWN, /* is UTF-8 */
	ZIP_ENCODING_UTF8_GUESSED, /* possibly UTF-8 */
	ZIP_ENCODING_CP437,    /* Code Page 437 */
	ZIP_ENCODING_ERROR     /* should be UTF-8 but isn't */
};

typedef enum zip_encoding_type zip_encoding_type_t;
struct zip_hash;
struct zip_progress;
typedef struct zip_cdir zip_cdir_t;
typedef struct zip_dirent zip_dirent_t;
typedef struct zip_entry zip_entry_t;
typedef struct zip_extra_field zip_extra_field_t;
typedef struct zip_string zip_string_t;
typedef struct zip_buffer zip_buffer_t;
typedef struct zip_hash zip_hash_t;
typedef struct zip_progress zip_progress_t;

/* zip archive, part of API */

struct zip {
	zip_source_t * src;  /* data source for archive */
	uint open_flags; /* flags passed to zip_open */
	zip_error_t error;   /* error information */
	uint flags; /* archive global flags */
	uint ch_flags; /* changed archive global flags */
	char * default_password; /* password used when no other supplied */
	zip_string_t * comment_orig; /* archive comment */
	zip_string_t * comment_changes; /* changed archive comment */
	bool comment_changed;      /* whether archive comment was changed */
	uint64 nentry;   /* number of entries */
	uint64 nentry_alloc; /* number of entries allocated */
	zip_entry_t * entry;   /* entries */
	uint nopen_source;   /* number of open sources using archive */
	uint nopen_source_alloc; /* number of sources allocated */
	zip_source_t ** open_source; /* open sources using archive */
	zip_hash_t * names; /* hash table for name lookup */
	zip_progress_t * progress; /* progress callback for zip_close() */
};

/* file in zip archive, part of API */

struct zip_file {
	zip_t * za;    /* zip archive containing this file */
	zip_error_t error; /* error information */
	bool eof;
	zip_source_t * src; /* data source */
};
//
// zip archive directory entry (central or local) 
//
#define ZIP_DIRENT_COMP_METHOD 0x0001u
#define ZIP_DIRENT_FILENAME 0x0002u
#define ZIP_DIRENT_COMMENT 0x0004u
#define ZIP_DIRENT_EXTRA_FIELD 0x0008u
#define ZIP_DIRENT_ATTRIBUTES 0x0010u
#define ZIP_DIRENT_LAST_MOD 0x0020u
#define ZIP_DIRENT_ENCRYPTION_METHOD 0x0040u
#define ZIP_DIRENT_PASSWORD 0x0080u
#define ZIP_DIRENT_ALL ZIP_UINT32_MAX

struct zip_dirent {
	uint32 changed;
	bool local_extra_fields_read; /*      whether we already read in local header extra fields */
	bool cloned;              /*      whether this instance is cloned, and thus shares non-changed strings */
	bool crc_valid; /*      if CRC is valid (sometimes not for encrypted archives) */
	uint16 version_madeby; /* (c)  version of creator */
	uint16 version_needed; /* (cl) version needed to extract */
	uint16 bitflags;       /* (cl) general purpose bit flag */
	int32 comp_method;     /* (cl) compression method used (uint16 and ZIP_CM_DEFAULT (-1)) */
	time_t last_mod;             /* (cl) time of last modification */
	uint32 crc;            /* (cl) CRC-32 of uncompressed data */
	uint64 comp_size;      /* (cl) size of compressed data */
	uint64 uncomp_size;    /* (cl) size of uncompressed data */
	zip_string_t * filename;     /* (cl) file name (NUL-terminated) */
	zip_extra_field_t * extra_fields; /* (cl) extra fields, parsed */
	zip_string_t * comment;      /* (c)  file comment */
	uint32 disk_number;    /* (c)  disk number start */
	uint16 int_attrib;     /* (c)  internal file attributes */
	uint32 ext_attrib;     /* (c)  external file attributes */
	uint64 offset;         /* (c)  offset of local header */
	uint16 compression_level; /*      level of compression to use (never valid in orig) */
	uint16 encryption_method; /*      encryption method, computed from other fields */
	char * password;            /*      file specific encryption password */
};

/* zip archive central directory */

struct zip_cdir {
	zip_entry_t * entry;   /* directory entries */
	uint64 nentry;   /* number of entries */
	uint64 nentry_alloc; /* number of entries allocated */
	uint64 size; /* size of central directory */
	uint64 offset; /* offset of central directory in file */
	zip_string_t * comment; /* zip archive comment */
	bool is_zip64;     /* central directory in zip64 format */
};

struct zip_extra_field {
	zip_extra_field_t * next;
	zip_flags_t flags; /* in local/central header */
	uint16 id; /* header id */
	uint16 size; /* data size */
	uint8 * data;
};

enum zip_source_write_state {
	ZIP_SOURCE_WRITE_CLOSED, /* write is not in progress */
	ZIP_SOURCE_WRITE_OPEN, /* write is in progress */
	ZIP_SOURCE_WRITE_FAILED, /* commit failed, only rollback allowed */
	ZIP_SOURCE_WRITE_REMOVED /* file was removed */
};

typedef enum zip_source_write_state zip_source_write_state_t;

struct zip_source {
	zip_source_t * src;
	union {
		zip_source_callback f;
		zip_source_layered_callback l;
	} cb;

	void * ud;
	zip_error_t error;
	int64 supports;             /* supported commands */
	uint open_count;          /* number of times source was opened (directly or as lower layer) */
	zip_source_write_state_t write_state; /* whether source is open for writing */
	bool source_closed;               /* set if source archive is closed */
	zip_t * source_archive;           /* zip archive we're reading from, NULL if not from archive */
	uint refcount;
	bool eof;        /* EOF reached */
	bool had_read_error; /* a previous ZIP_SOURCE_READ reported an error */
};

#define ZIP_SOURCE_IS_OPEN_READING(src) ((src)->open_count > 0)
#define ZIP_SOURCE_IS_OPEN_WRITING(src) ((src)->write_state == ZIP_SOURCE_WRITE_OPEN)
#define ZIP_SOURCE_IS_LAYERED(src) ((src)->src != NULL)
//
// entry in zip archive directory 
//
struct zip_entry {
	zip_dirent_t * orig;
	zip_dirent_t * changes;
	zip_source_t * source;
	bool deleted;
};
//
// file or archive comment, or filename
//
struct zip_string {
	uint8 * raw;           /* raw string */
	uint16 length;         /* length of raw string */
	enum zip_encoding_type encoding; /* autorecognized encoding */
	uint8 * converted;     /* autoconverted string */
	uint32 converted_length; /* length of converted */
};

/* byte array */
// 
// For performance, we usually keep 8k byte arrays on the stack.
// However, there are (embedded) systems with a stack size of 12k;
// for those, use malloc()/free() 
// 
#ifdef ZIP_ALLOCATE_BUFFER
	#define DEFINE_BYTE_ARRAY(buf, size) uint8 * buf
	#define byte_array_init(buf, size) (((buf) = static_cast<uint8 *>(SAlloc::M(size))) != NULL)
	#define byte_array_fini(buf) (SAlloc::F(buf))
#else
	#define DEFINE_BYTE_ARRAY(buf, size) uint8 buf[size]
	#define byte_array_init(buf, size) (1)
	#define byte_array_fini(buf) ((void)0)
#endif
//
// bounds checked access to memory buffer 
//
struct zip_buffer {
	bool ok;
	bool free_data;
	uint8 * data;
	uint64 size;
	uint64 offset;
};
//
// which files to write in which order
//
struct zip_filelist {
	uint64 idx;
	// TODO const char *name;
};

typedef struct zip_filelist zip_filelist_t;
struct _zip_winzip_aes;
typedef struct _zip_winzip_aes zip_winzip_aes_t;

struct _zip_pkware_keys {
	uint32 key[3];
};

typedef struct _zip_pkware_keys zip_pkware_keys_t;

// @sobolev extern const char * const _zip_err_str[];
// @sobolev extern const int _zip_nerr_str;
// @sobolev extern const int _zip_err_type[];

//#define MAX(a, b) ((a) > (b) ? (a) : (b))
//#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ZIP_ENTRY_CHANGED(e, f) ((e)->changes && ((e)->changes->changed & (f)))
#define ZIP_ENTRY_DATA_CHANGED(x) ((x)->source != NULL)
#define ZIP_ENTRY_HAS_CHANGES(e) (ZIP_ENTRY_DATA_CHANGED(e) || (e)->deleted || ZIP_ENTRY_CHANGED((e), ZIP_DIRENT_ALL))

#define ZIP_IS_RDONLY(za) ((za)->ch_flags & ZIP_AFL_RDONLY)
#ifdef HAVE_EXPLICIT_MEMSET
	#define _zip_crypto_clear(b, l) explicit_memset((b), 0, (l))
#else
	#ifdef HAVE_EXPLICIT_BZERO
		#define _zip_crypto_clear(b, l) explicit_bzero((b), (l))
	#else
		#define _zip_crypto_clear(b, l) memzero((b), (l))
	#endif
#endif

int64 _zip_add_entry(zip_t *);
uint8 * _zip_buffer_data(zip_buffer_t * buffer);
// @sobolev (static) bool FASTCALL _zip_buffer_eof(const zip_buffer_t * buffer);
// @sobolev (static) void FASTCALL _zip_buffer_free(zip_buffer_t * buffer);
// @sobolev (static) uint8 * FASTCALL _zip_buffer_get(zip_buffer_t * buffer, uint64 length);
uint16 _zip_buffer_get_16(zip_buffer_t * buffer);
uint32 _zip_buffer_get_32(zip_buffer_t * buffer);
uint64 _zip_buffer_get_64(zip_buffer_t * buffer);
uint8 _zip_buffer_get_8(zip_buffer_t * buffer);
// @sobolev (static) uint64 FASTCALL _zip_buffer_left(const zip_buffer_t * buffer);
// @sobolev (static) zip_buffer_t * FASTCALL _zip_buffer_new(uint8 * data, uint64 size);
// @sobolev (static) zip_buffer_t * _zip_buffer_new_from_source(zip_source_t * src, uint64 size, uint8 * buf, zip_error_t * error);
// @sobolev (static) uint64 FASTCALL _zip_buffer_offset(const zip_buffer_t * buffer);
// @sobolev (static) bool FASTCALL _zip_buffer_ok(const zip_buffer_t * buffer);
// @sobolev (static) uint8 * FASTCALL _zip_buffer_peek(zip_buffer_t * buffer, uint64 length);
// @sobolev (static) int FASTCALL _zip_buffer_put(zip_buffer_t * buffer, const void * src, size_t length);
// @sobolev (static) int FASTCALL _zip_buffer_put_16(zip_buffer_t * buffer, uint16 i);
// @sobolev (static) int FASTCALL _zip_buffer_put_32(zip_buffer_t * buffer, uint32 i);
// @sobolev (static) int FASTCALL _zip_buffer_put_64(zip_buffer_t * buffer, uint64 i);
// @sobolev (static) int FASTCALL _zip_buffer_put_8(zip_buffer_t * buffer, uint8 i);
// @sobolev (static) uint64 _zip_buffer_read(zip_buffer_t * buffer, uint8 * data, uint64 length);
// @sobolev (static) int _zip_buffer_skip(zip_buffer_t * buffer, uint64 length);
// @sobolev (static) int FASTCALL _zip_buffer_set_offset(zip_buffer_t * buffer, uint64 offset);
// @sobolev (static) uint64 FASTCALL _zip_buffer_size(const zip_buffer_t * buffer);
void _zip_cdir_free(zip_cdir_t *);
bool _zip_cdir_grow(zip_cdir_t * cd, uint64 additional_entries, zip_error_t * error);
zip_cdir_t * _zip_cdir_new(uint64, zip_error_t *);
int64 _zip_cdir_write(zip_t * za, const zip_filelist_t * filelist, uint64 survivors);
time_t _zip_d2u_time(uint16, uint16);
void _zip_deregister_source(zip_t * za, zip_source_t * src);
void _zip_dirent_apply_attributes(zip_dirent_t *, zip_file_attributes_t *, bool, uint32);
zip_dirent_t * _zip_dirent_clone(const zip_dirent_t *);
void _zip_dirent_free(zip_dirent_t *);
void _zip_dirent_finalize(zip_dirent_t *);
void _zip_dirent_init(zip_dirent_t *);
bool _zip_dirent_needs_zip64(const zip_dirent_t *, zip_flags_t);
zip_dirent_t * _zip_dirent_new(void);
int64 _zip_dirent_read(zip_dirent_t * zde, zip_source_t * src, zip_buffer_t * buffer, bool local, zip_error_t * error);
void _zip_dirent_set_version_needed(zip_dirent_t * de, bool force_zip64);
int32 _zip_dirent_size(zip_source_t * src, uint16, zip_error_t *);
int _zip_dirent_write(zip_t * za, zip_dirent_t * dirent, zip_flags_t flags);
zip_extra_field_t * _zip_ef_clone(const zip_extra_field_t *, zip_error_t *);
zip_extra_field_t * _zip_ef_delete_by_id(zip_extra_field_t *, uint16, uint16, zip_flags_t);
void _zip_ef_free(zip_extra_field_t *);
const uint8 * _zip_ef_get_by_id(const zip_extra_field_t *, uint16 *, uint16, uint16, zip_flags_t, zip_error_t *);
zip_extra_field_t * _zip_ef_merge(zip_extra_field_t *, zip_extra_field_t *);
zip_extra_field_t * _zip_ef_new(uint16, uint16, const uint8 *, zip_flags_t);
bool _zip_ef_parse(const uint8 *, uint16, zip_flags_t, zip_extra_field_t **, zip_error_t *);
zip_extra_field_t * _zip_ef_remove_internal(zip_extra_field_t *);
uint16 _zip_ef_size(const zip_extra_field_t *, zip_flags_t);
int _zip_ef_write(zip_t * za, const zip_extra_field_t * ef, zip_flags_t flags);
void _zip_entry_finalize(zip_entry_t *);
void _zip_entry_init(zip_entry_t *);
void _zip_error_clear(zip_error_t *);
void _zip_error_get(const zip_error_t *, int *, int *);
void _zip_error_copy(zip_error_t * dst, const zip_error_t * src);
void _zip_error_set_from_source(zip_error_t *, zip_source_t *);
const uint8 * _zip_extract_extra_field_by_id(zip_error_t *, uint16, int, const uint8 *, uint16, uint16 *);
int _zip_file_extra_field_prepare_for_change(zip_t *, uint64);
int _zip_file_fillbuf(void *, size_t, zip_file_t *);
uint64 _zip_file_get_end(const zip_t * za, uint64 index, zip_error_t * error);
uint64 _zip_file_get_offset(const zip_t *, uint64, zip_error_t *);
zip_dirent_t * _zip_get_dirent(zip_t *, uint64, zip_flags_t, zip_error_t *);
enum zip_encoding_type _zip_guess_encoding(zip_string_t *, enum zip_encoding_type);
uint8 * _zip_cp437_to_utf8(const uint8 *const, uint32, uint32 *, zip_error_t *);
bool _zip_hash_add(zip_hash_t * hash, const uint8 * name, uint64 index, zip_flags_t flags, zip_error_t * error);
bool _zip_hash_delete(zip_hash_t * hash, const uint8 * key, zip_error_t * error);
void _zip_hash_free(zip_hash_t * hash);
int64 _zip_hash_lookup(zip_hash_t * hash, const uint8 * name, zip_flags_t flags, zip_error_t * error);
zip_hash_t * _zip_hash_new(zip_error_t * error);
bool _zip_hash_reserve_capacity(zip_hash_t * hash, uint64 capacity, zip_error_t * error);
bool _zip_hash_revert(zip_hash_t * hash, zip_error_t * error);
int _zip_mkstempm(char * path, int mode);
zip_t * _zip_open(zip_source_t *, uint, zip_error_t *);
void _zip_progress_end(zip_progress_t * progress);
void _zip_progress_free(zip_progress_t * progress);
int _zip_progress_start(zip_progress_t * progress);
int _zip_progress_subrange(zip_progress_t * progress, double start, double end);
int _zip_progress_update(zip_progress_t * progress, double value);
/* this symbol is extern so it can be overridden for regression testing */
ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length);
uint32 zip_random_uint32(void);
int _zip_read(zip_source_t * src, uint8 * data, uint64 length, zip_error_t * error);
int _zip_read_at_offset(zip_source_t * src, uint64 offset, unsigned char * b, size_t length, zip_error_t * error);
uint8 * _zip_read_data(zip_buffer_t * buffer, zip_source_t * src, size_t length, bool nulp, zip_error_t * error);
int _zip_read_local_ef(zip_t *, uint64);
zip_string_t * _zip_read_string(zip_buffer_t * buffer, zip_source_t * src, uint16 length, bool nulp, zip_error_t * error);
int _zip_register_source(zip_t * za, zip_source_t * src);
void _zip_set_open_error(int * zep, const zip_error_t * err, int ze);
bool zip_source_accept_empty(zip_source_t * src);
int64 _zip_source_call(zip_source_t * src, void * data, uint64 length, zip_source_cmd_t command);
bool _zip_source_eof(zip_source_t *);
zip_source_t * _zip_source_file_or_p(const char *, FILE *, uint64, int64, const zip_stat_t *, zip_error_t * error);
bool _zip_source_had_error(zip_source_t *);
void _zip_source_invalidate(zip_source_t * src);
zip_source_t * _zip_source_new(zip_error_t * error);
int _zip_source_set_source_archive(zip_source_t *, zip_t *);
zip_source_t * _zip_source_window_new(zip_source_t * src, uint64 start, uint64 length, zip_stat_t * st,
    zip_file_attributes_t * attributes, zip_t * source_archive, uint64 source_index, zip_error_t * error);
zip_source_t * _zip_source_zip_new(zip_t *, zip_t *, uint64, zip_flags_t, uint64, uint64, const char *);
int _zip_stat_merge(zip_stat_t * dst, const zip_stat_t * src, zip_error_t * error);
int _zip_string_equal(const zip_string_t *, const zip_string_t *);
void _zip_string_free(zip_string_t *);
uint32 _zip_string_crc32(const zip_string_t *);
const uint8 * _zip_string_get(zip_string_t *, uint32 *, zip_flags_t, zip_error_t *);
// @sobolev (static) uint16 FASTCALL _zip_string_length(const zip_string_t *);
zip_string_t * _zip_string_new(const uint8 *, uint16, zip_flags_t, zip_error_t *);
int _zip_string_write(zip_t * za, const zip_string_t * string);
bool _zip_winzip_aes_decrypt(zip_winzip_aes_t * ctx, uint8 * data, uint64 length);
bool _zip_winzip_aes_encrypt(zip_winzip_aes_t * ctx, uint8 * data, uint64 length);
bool _zip_winzip_aes_finish(zip_winzip_aes_t * ctx, uint8 * hmac);
void _zip_winzip_aes_free(zip_winzip_aes_t * ctx);
zip_winzip_aes_t * _zip_winzip_aes_new(const uint8 * password, uint64 password_length, const uint8 * salt,
    uint16 key_size, uint8 * password_verify, zip_error_t * error);
void _zip_pkware_encrypt(zip_pkware_keys_t * keys, uint8 * out, const uint8 * in, uint64 len);
void _zip_pkware_decrypt(zip_pkware_keys_t * keys, uint8 * out, const uint8 * in, uint64 len);
zip_pkware_keys_t * _zip_pkware_keys_new(zip_error_t * error);
void _zip_pkware_keys_free(zip_pkware_keys_t * keys);
void _zip_pkware_keys_reset(zip_pkware_keys_t * keys);
int _zip_changed(const zip_t *, uint64 *);
const char * _zip_get_name(zip_t *, uint64, zip_flags_t, zip_error_t *);
int _zip_local_header_read(zip_t *, int);
void * _zip_memdup(const void *, size_t, zip_error_t *);
int64 _zip_name_locate(zip_t *, const char *, zip_flags_t, zip_error_t *);
zip_t * _zip_new(zip_error_t *);
int64 _zip_file_replace(zip_t *, uint64, const char *, zip_source_t *, zip_flags_t);
int _zip_set_name(zip_t *, uint64, const char *, zip_flags_t);
void _zip_u2d_time(time_t, uint16 *, uint16 *);
int _zip_unchange(zip_t *, uint64, int);
void _zip_unchange_data(zip_entry_t *);
int _zip_write(zip_t * za, const void * data, uint64 length);
//
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef _WIN32
	#include <fcntl.h>
	#include <io.h>
#endif
#ifdef HAVE_CRYPTO
	//#include "zip_crypto.h"
	#define ZIP_CRYPTO_SHA1_LENGTH 20
	#define ZIP_CRYPTO_AES_BLOCK_LENGTH 16

	#if defined(HAVE_WINDOWS_CRYPTO)
		#include "zip_crypto_win.h"
	#elif defined(HAVE_COMMONCRYPTO)
		#include "zip_crypto_commoncrypto.h"
	#elif defined(HAVE_GNUTLS)
		#include "zip_crypto_gnutls.h"
	#elif defined(HAVE_OPENSSL)
		#include "zip_crypto_openssl.h"
	#elif defined(HAVE_MBEDTLS)
		#include "zip_crypto_mbedtls.h"
	#else
		#error "no crypto backend found"
	#endif
	//
#endif
//#include "zip_source_file.h"
struct zip_source_file_stat {
    uint64 size; /* must be valid for regular files */
    time_t mtime;      /* must always be valid, is initialized to current time */
    bool exists;       /* must always be vaild */
    bool regular_file; /* must always be valid */
};

typedef struct zip_source_file_context zip_source_file_context_t;
typedef struct zip_source_file_operations zip_source_file_operations_t;
typedef struct zip_source_file_stat zip_source_file_stat_t;

struct zip_source_file_context {
    zip_error_t error; /* last error information */
    int64 supports;
    /* reading */
    char *fname;                      /* name of file to read from */
    void *f;                          /* file to read from */
    zip_stat_t st;                    /* stat information passed in */
    zip_file_attributes_t attributes; /* additional file attributes */
    zip_error_t stat_error;           /* error returned for stat */
    uint64 start;               /* start offset of data to read */
    uint64 len;                 /* length of the file, 0 for up to EOF */
    uint64 offset;              /* current offset relative to start (0 is beginning of part we read) */
    /* writing */
    char *tmpname;
    void *fout;
    zip_source_file_operations_t *ops;
    void *ops_userdata;
};
// 
// The following methods must be implemented to support each feature:
// - close, read, seek, and stat must always be implemented.
// - To support specifying the file by name, open, and strdup must be implemented.
// - For write support, the file must be specified by name and close, commit_write, create_temp_output, remove, rollback_write, and tell must be implemented.
// - create_temp_output_cloning is always optional. */
// 
struct zip_source_file_operations {
    void (*close)(zip_source_file_context_t *ctx);
    int64 (*commit_write)(zip_source_file_context_t *ctx);
    int64 (*create_temp_output)(zip_source_file_context_t *ctx);
    int64 (*create_temp_output_cloning)(zip_source_file_context_t *ctx, uint64 len);
    bool (*open)(zip_source_file_context_t *ctx);
    int64 (*read)(zip_source_file_context_t *ctx, void *buf, uint64 len);
    int64 (*remove)(zip_source_file_context_t *ctx);
    void (*rollback_write)(zip_source_file_context_t *ctx);
    bool (*seek)(zip_source_file_context_t *ctx, void *f, int64 offset, int whence);
    bool (*stat)(zip_source_file_context_t *ctx, zip_source_file_stat_t *st);
    char *(*string_duplicate)(zip_source_file_context_t *ctx, const char *);
    int64 (*tell)(zip_source_file_context_t *ctx, void *f);
    int64 (*write)(zip_source_file_context_t *ctx, const void *data, uint64 len);
};

zip_source_t *zip_source_file_common_new(const char *fname, void *file, uint64 start, int64 len, const zip_stat_t *st, zip_source_file_operations_t *ops, void *ops_userdata, zip_error_t *error);
//
//#include "zip_source_file_stdio.h"
void _zip_stdio_op_close(zip_source_file_context_t *ctx);
int64 _zip_stdio_op_read(zip_source_file_context_t *ctx, void *buf, uint64 len);
bool _zip_stdio_op_seek(zip_source_file_context_t *ctx, void *f, int64 offset, int whence);
bool _zip_stdio_op_stat(zip_source_file_context_t *ctx, zip_source_file_stat_t *st);
int64 _zip_stdio_op_tell(zip_source_file_context_t *ctx, void *f);
FILE *_zip_fopen_close_on_exec(const char *name, bool writeable);
//
#ifdef HAVE_CLONEFILE
	#include <sys/attr.h>
	#include <sys/clonefile.h>
	#define CAN_CLONE
#endif
#ifdef HAVE_FICLONERANGE
	#include <linux/fs.h>
	#include <sys/ioctl.h>
	#define CAN_CLONE
#endif
//#include "zip_source_file_win32.h"
// 0x0501 => Windows XP; needs to be at least this value because of GetFileSizeEx 
#if !defined(MS_UWP) && !defined(_WIN32_WINNT)
	#define _WIN32_WINNT 0x0501
#endif
//#include <windows.h>
#include <aclapi.h>
//#include "zipint.h"
//#include "zip_source_file.h"

struct zip_win32_file_operations {
    char *(*allocate_tempname)(const char *name, size_t extra_chars, size_t *lengthp);
    HANDLE (__stdcall *create_file)(const void *name, DWORD access, DWORD share_mode, PSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD file_attributes, HANDLE template_file);
    BOOL (__stdcall *delete_file)(const void *name);
    DWORD (__stdcall *get_file_attributes)(const void *name);
    BOOL (__stdcall *get_file_attributes_ex)(const void *name, GET_FILEEX_INFO_LEVELS info_level, void *information);
    void (*make_tempname)(char *buf, size_t len, const char *name, uint32 i);
    BOOL (__stdcall *move_file)(const void *from, const void *to, DWORD flags);
    BOOL (__stdcall *set_file_attributes)(const void *name, DWORD attributes);
    char *(*string_duplicate)(const char *string);
};

typedef struct zip_win32_file_operations zip_win32_file_operations_t;
extern zip_source_file_operations_t _zip_source_file_win32_named_ops;
void _zip_win32_op_close(zip_source_file_context_t *ctx);
int64 _zip_win32_op_read(zip_source_file_context_t *ctx, void *buf, uint64 len);
bool _zip_win32_op_seek(zip_source_file_context_t *ctx, void *f, int64 offset, int whence);
int64 _zip_win32_op_tell(zip_source_file_context_t *ctx, void *f);
bool _zip_filetime_to_time_t(FILETIME ft, time_t *t);
int _zip_win32_error_to_errno(DWORD win32err);
//
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <..\..\bzip2\bzlib.h>
#include <..\OSF\liblzma\api\lzma.h>

#endif /* zipint.h */
