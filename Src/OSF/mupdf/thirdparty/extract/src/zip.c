//
//
#include <slib.h>
#include "alloc.h"
#include "mem.h"
#include "memento.h"
#include "outf.h"
#include "zip.h"
#include <zlib.h>
/* For crc32(). */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>

typedef struct {
	int16_t mtime;
	int16_t mdate;
	int32_t crc_sum;
	int32_t size_compressed;
	int32_t size_uncompressed;
	char*       name;
	uint32_t offset;
	uint16_t attr_internal;
	uint32_t attr_external;
} extract_zip_cd_file_t;

struct extract_zip_t {
	extract_buffer_t*       buffer;
	extract_zip_cd_file_t*  cd_files;
	int cd_files_num;

	/* errno_ is set to non-zero if any operation fails; avoids need to check
	   after every small output operation. */
	int errno_;
	int eof;

	/* Defaults for various values in zip file headers etc. */
	uint16_t mtime;
	uint16_t mdate;
	uint16_t version_creator;
	uint16_t version_extract;
	uint16_t general_purpose_bit_flag;
	uint16_t file_attr_internal;
	uint32_t file_attr_external;
	char*                   archive_comment;
};

int extract_zip_open(extract_buffer_t* buffer, extract_zip_t** o_zip)
{
	int e = -1;
	extract_zip_t* zip;
	if(extract_malloc(&zip, sizeof(*zip))) goto end;

	zip->cd_files = NULL;
	zip->cd_files_num = 0;
	zip->buffer = buffer;
	zip->errno_ = 0;
	zip->eof = 0;

	/* We could maybe convert current date/time to the ms-dos format required
	   here, but using zeros doesn't seem to make a difference to Word etc. */
	zip->mtime = 0;
	zip->mdate = 0;

	/* These are all copied from command-line zip on unix. */
	zip->version_creator = (0x3 << 8) + 30; /* 0x3 is unix, 30 means 3.0. */
	zip->version_extract = 10;          /* 10 means 1.0. */
	zip->general_purpose_bit_flag = 0;
	zip->file_attr_internal = 0;

	/* We follow command-line zip which uses 0x81a40000 which is octal
	   0100644:0.  (0100644 is S_IFREG (regular file) plus rw-r-r. See stat(2) for
	   details.) */
	zip->file_attr_external = (0100644 << 16) + 0;
	if(extract_strdup("Artifex", &zip->archive_comment)) goto end;

	e = 0;

end:
	if(e) {
		if(zip) extract_free(&zip->archive_comment);
		extract_free(&zip);
		*o_zip = NULL;
	}
	else {
		*o_zip = zip;
	}
	return e;
}

static int s_native_little_endinesss(void)
{
	static const char a[] = { 1, 2};
	uint16_t b = *(uint16_t*)a;
	if(b == 1 + 2*256) {
		/* Native little-endiness. */
		return 1;
	}
	else if(b == 2 + 1*256) {
		/* Native big-endiness. */
		return 0;
	}
	abort();
}

static int s_write(extract_zip_t* zip, const void* data, size_t data_length)
{
	size_t actual;
	int e;
	if(zip->errno_) return -1;
	if(zip->eof) return +1;
	e = extract_buffer_write(zip->buffer, data, data_length, &actual);
	if(e == -1) zip->errno_ = errno;
	if(e == +1) zip->eof = 1;
	return e;
}

static int s_write_uint32(extract_zip_t* zip, uint32_t value)
{
	if(s_native_little_endinesss()) {
		return s_write(zip, &value, sizeof(value));
	}
	else {
		unsigned char value2[4] = {
			(unsigned char)(value >> 0),
			(unsigned char)(value >> 8),
			(unsigned char)(value >> 16),
			(unsigned char)(value >> 24)
		};
		return s_write(zip, &value2, sizeof(value2));
	}
}

static int s_write_uint16(extract_zip_t* zip, uint16_t value)
{
	if(s_native_little_endinesss()) {
		return s_write(zip, &value, sizeof(value));
	}
	else {
		unsigned char value2[2] = {
			(unsigned char)(value >> 0),
			(unsigned char)(value >> 8)
		};
		return s_write(zip, &value2, sizeof(value2));
	}
}

static int s_write_string(extract_zip_t* zip, const char* text)
{
	return s_write(zip, text, strlen(text));
}

int extract_zip_write_file(extract_zip_t* zip,
    const void* data,
    size_t data_length,
    const char* name
    )
{
	int e = -1;
	extract_zip_cd_file_t* cd_file = NULL;

	if(data_length > INT_MAX) {
		assert(0);
		errno = EINVAL;
		return -1;
	}
	/* Create central directory file header for later. */
	if(extract_realloc2(
		    &zip->cd_files,
		    sizeof(extract_zip_cd_file_t) * zip->cd_files_num,
		    sizeof(extract_zip_cd_file_t) * (zip->cd_files_num+1)
		    )) goto end;
	cd_file = &zip->cd_files[zip->cd_files_num];
	cd_file->name = NULL;

	cd_file->mtime = zip->mtime;
	cd_file->mdate = zip->mtime;
	cd_file->crc_sum = (int32_t)crc32(crc32(0, NULL, 0), (const Bytef*)data, (int)data_length);
	cd_file->size_compressed = (int)data_length;
	cd_file->size_uncompressed = (int)data_length;
	if(extract_strdup(name, &cd_file->name)) goto end;
	cd_file->offset = (int)extract_buffer_pos(zip->buffer);
	cd_file->attr_internal = zip->file_attr_internal;
	cd_file->attr_external = zip->file_attr_external;
	if(!cd_file->name) goto end;

	/* Write local file header. */
	{
		const char extra_local[] = ""; /* Modify for testing. */
		s_write_uint32(zip, 0x04034b50);
		s_write_uint16(zip, zip->version_extract);  /* Version needed to extract (minimum). */
		s_write_uint16(zip, zip->general_purpose_bit_flag); /* General purpose bit flag */
		s_write_uint16(zip, 0);                     /* Compression method */
		s_write_uint16(zip, cd_file->mtime);        /* File last modification time */
		s_write_uint16(zip, cd_file->mdate);        /* File last modification date */
		s_write_uint32(zip, cd_file->crc_sum);      /* CRC-32 of uncompressed data */
		s_write_uint32(zip, cd_file->size_compressed); /* Compressed size */
		s_write_uint32(zip, cd_file->size_uncompressed); /* Uncompressed size */
		s_write_uint16(zip, (uint16_t)strlen(name)); /* File name length (n) */
		s_write_uint16(zip, sizeof(extra_local)-1); /* Extra field length (m) */
		s_write_string(zip, cd_file->name);         /* File name */
		s_write(zip, extra_local, sizeof(extra_local)-1); /* Extra field */
	}
	/* Write the (uncompressed) data. */
	s_write(zip, data, data_length);

	if(zip->errno_) e = -1;
	else if(zip->eof) e = +1;
	else e = 0;

end:

	if(e) {
		/* Leave zip->cd_files_num unchanged, so calling extract_zip_close()
		   will write out any earlier files. Free cd_file->name to avoid leak. */
		if(cd_file) extract_free(&cd_file->name);
	}
	else {
		/* cd_files[zip->cd_files_num] is valid. */
		zip->cd_files_num += 1;
	}

	return e;
}

int extract_zip_close(extract_zip_t** pzip)
{
	int e = -1;
	size_t pos;
	size_t len;
	int i;
	extract_zip_t*  zip = *pzip;
	if(!zip) {
		return 0;
	}
	pos = extract_buffer_pos(zip->buffer);
	len = 0;

	/* Write Central directory file headers, freeing data as we go. */
	for(i = 0; i<zip->cd_files_num; ++i) {
		const char extra[] = "";
		size_t pos2 = extract_buffer_pos(zip->buffer);
		extract_zip_cd_file_t* cd_file = &zip->cd_files[i];
		s_write_uint32(zip, 0x02014b50);
		s_write_uint16(zip, zip->version_creator);      /* Version made by, copied from command-line zip. */
		s_write_uint16(zip, zip->version_extract);      /* Version needed to extract (minimum). */
		s_write_uint16(zip, zip->general_purpose_bit_flag); /* General purpose bit flag */
		s_write_uint16(zip, 0);                         /* Compression method */
		s_write_uint16(zip, cd_file->mtime);            /* File last modification time */
		s_write_uint16(zip, cd_file->mdate);            /* File last modification date */
		s_write_uint32(zip, cd_file->crc_sum);          /* CRC-32 of uncompressed data */
		s_write_uint32(zip, cd_file->size_compressed);  /* Compressed size */
		s_write_uint32(zip, cd_file->size_uncompressed); /* Uncompressed size */
		s_write_uint16(zip, (uint16_t)strlen(cd_file->name)); /* File name length (n) */
		s_write_uint16(zip, sizeof(extra)-1);           /* Extra field length (m) */
		s_write_uint16(zip, 0);                         /* File comment length (k) */
		s_write_uint16(zip, 0);                         /* Disk number where file starts */
		s_write_uint16(zip, cd_file->attr_internal);    /* Internal file attributes */
		s_write_uint32(zip, cd_file->attr_external);    /* External file attributes. */
		s_write_uint32(zip, cd_file->offset);           /* Offset of local file header. */
		s_write_string(zip, cd_file->name);             /* File name */
		s_write(zip, extra, sizeof(extra)-1);           /* Extra field */
		len += extract_buffer_pos(zip->buffer) - pos2;
		extract_free(&cd_file->name);
	}
	extract_free(&zip->cd_files);

	/* Write End of central directory record. */
	s_write_uint32(zip, 0x06054b50);
	s_write_uint16(zip, 0);                         /* Number of this disk */
	s_write_uint16(zip, 0);                         /* Disk where central directory starts */
	s_write_uint16(zip, (uint16_t)zip->cd_files_num); /* Number of central directory records on this disk */
	s_write_uint16(zip, (uint16_t)zip->cd_files_num); /* Total number of central directory records */
	s_write_uint32(zip, (int)len);                  /* Size of central directory (bytes) */
	s_write_uint32(zip, (int)pos);                  /* Offset of start of central directory, relative to start of
	                                                   archive */

	s_write_uint16(zip, (uint16_t)strlen(zip->archive_comment)); /* Comment length (n) */
	s_write_string(zip, zip->archive_comment);
	extract_free(&zip->archive_comment);

	if(zip->errno_) e = -1;
	else if(zip->eof) e = +1;
	else e = 0;

	extract_free(pzip);

	return e;
}
