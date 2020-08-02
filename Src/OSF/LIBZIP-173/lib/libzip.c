// LIBZIP.C
// Copyright (C) 1999-2019 Dieter Baron and Thomas Klausner
// 
// This file is part of libzip, a library to manipulate ZIP archives.
// The authors can be contacted at <libzip@nih.at>
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the distribution.
// 3. The names of the authors may not be used to endorse or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#include "zipint.h"
#pragma hdrstop
//
// return run-time version of library
//
ZIP_EXTERN const char * zip_libzip_version(void) { return LIBZIP_VERSION; }

void * _zip_memdup(const void * mem, size_t len, zip_error_t * error) 
{
	void * ret = 0;
	if(len) {
		ret = SAlloc::M(len);
		if(!ret)
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		else
			memcpy(ret, mem, len);
	}
	return ret;
}
// 
// Descr: create temporary file with same permissions as previous one;
//   or default permissions if there is no previous file
// 
int _zip_mkstempm(char * path, int mode) 
{
	int fd;
	int xcnt = 0;
	char * end = path + strlen(path);
	char * start = end - 1;
	while(start >= path && *start == 'X') {
		xcnt++;
		start--;
	}
	if(xcnt == 0) {
		errno = EINVAL;
		return -1;
	}
	start++;
	for(;;) {
		uint32 value = zip_random_uint32();
		char * xs = start;
		while(xs < end) {
			char digit = static_cast<char>(value % 36);
			if(digit < 10) {
				*(xs++) = digit + '0';
			}
			else {
				*(xs++) = digit - 10 + 'a';
			}
			value /= 36;
		}
		fd = _open(path, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, mode == -1 ? 0666 : (mode_t)mode);
		if(fd >= 0) {
			if(mode != -1)
				(void)_chmod(path, (mode_t)mode); // open() honors umask(), which we don't want in this case 
			return fd;
		}
		else if(errno != EEXIST) {
			return -1;
		}
	}
}
//
// zip-entry
//
void _zip_entry_finalize(zip_entry_t * e) 
{
	if(e) {
		_zip_unchange_data(e);
		_zip_dirent_free(e->orig);
		_zip_dirent_free(e->changes);
	}
}

void _zip_entry_init(zip_entry_t * e) 
{
	if(e) {
		e->orig = NULL;
		e->changes = NULL;
		e->source = NULL;
		e->deleted = 0;
	}
}
//
//
//
int64 _zip_name_locate(zip_t * za, const char * fname, zip_flags_t flags, zip_error_t * error) 
{
	int (* cmp)(const char *, const char *);
	const char * fn, * p;
	uint64 i;
	if(!za)
		return -1;
	else if(!fname) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else if(flags & (ZIP_FL_NOCASE | ZIP_FL_NODIR | ZIP_FL_ENC_CP437)) {
		// can't use hash table 
		cmp = (flags & ZIP_FL_NOCASE) ? strcasecmp : strcmp;
		for(i = 0; i < za->nentry; i++) {
			fn = _zip_get_name(za, i, flags, error);
			// newly added (partially filled) entry or error 
			if(fn == NULL)
				continue;
			if(flags & ZIP_FL_NODIR) {
				p = strrchr(fn, '/');
				if(p)
					fn = p + 1;
			}
			if(cmp(fname, fn) == 0) {
				_zip_error_clear(error);
				return (int64)i;
			}
		}
		zip_error_set(error, SLERR_ZIP_NOENT, 0);
		return -1;
	}
	else
		return _zip_hash_lookup(za->names, (const uint8*)fname, flags, error);
}
//
// Descr: get index by name
//
ZIP_EXTERN int64 zip_name_locate(zip_t * za, const char * fname, zip_flags_t flags) 
{
	return _zip_name_locate(za, fname, flags, &za->error);
}
//
// FILE
//
/*
   NOTE: Return type is signed so we can return -1 on error.
        The index can not be larger than ZIP_INT64_MAX since the size
        of the central directory cannot be larger than
        ZIP_UINT64_MAX, and each entry is larger than 2 bytes.
 */
//
// add file via callback function
//
ZIP_EXTERN int64 zip_file_add(zip_t * za, const char * name, zip_source_t * source, zip_flags_t flags) 
{
	if(!name || !source) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else
		return _zip_file_replace(za, ZIP_UINT64_MAX, name, source, flags);
}
//
// replace file via callback function
//
ZIP_EXTERN int zip_file_replace(zip_t * za, uint64 idx, zip_source_t * source, zip_flags_t flags) 
{
	if(idx >= za->nentry || source == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_file_replace(za, idx, NULL, source, flags) == -1)
		return -1;
	return 0;
}

// NOTE: Signed due to -1 on error.  See zip_add.c for more details. 
int64 _zip_file_replace(zip_t * za, uint64 idx, const char * name, zip_source_t * source, zip_flags_t flags) 
{
	uint64 za_nentry_prev;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	za_nentry_prev = za->nentry;
	if(idx == ZIP_UINT64_MAX) {
		int64 i = -1;
		if(flags & ZIP_FL_OVERWRITE)
			i = _zip_name_locate(za, name, flags, NULL);
		if(i == -1) {
			// create and use new entry, used by zip_add 
			if((i = _zip_add_entry(za)) < 0)
				return -1;
		}
		idx = (uint64)i;
	}
	if(name && _zip_set_name(za, idx, name, flags) != 0) {
		if(za->nentry != za_nentry_prev) {
			_zip_entry_finalize(za->entry + idx);
			za->nentry = za_nentry_prev;
		}
		return -1;
	}
	// does not change any name related data, so we can do it here;
	// needed for a double add of the same file name 
	_zip_unchange_data(za->entry + idx);
	if(za->entry[idx].orig && (za->entry[idx].changes == NULL || (za->entry[idx].changes->changed & ZIP_DIRENT_COMP_METHOD) == 0)) {
		if(za->entry[idx].changes == NULL) {
			if((za->entry[idx].changes = _zip_dirent_clone(za->entry[idx].orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		za->entry[idx].changes->comp_method = ZIP_CM_REPLACED_DEFAULT;
		za->entry[idx].changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}
	za->entry[idx].source = source;
	return (int64)idx;
}
//
// rename file in zip archive
//
ZIP_EXTERN int zip_file_rename(zip_t * za, uint64 idx, const char * name, zip_flags_t flags) 
{
	if(idx >= za->nentry || (sstrlen(name) > ZIP_UINT16_MAX)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	else {
		const char * old_name = zip_get_name(za, idx, 0);
		if(!old_name)
			return -1;
		else {
			int new_is_dir = (name != NULL && name[strlen(name) - 1] == '/');
			int old_is_dir = (old_name[strlen(old_name) - 1] == '/');
			if(new_is_dir != old_is_dir) {
				zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
				return -1;
			}
			else
				return _zip_set_name(za, idx, name, flags);
		}
	}
}
//
// get file comment
// lenp is 32 bit because converted comment can be longer than ZIP_UINT16_MAX 
//
ZIP_EXTERN const char * zip_file_get_comment(zip_t * za, uint64 idx, uint32 * lenp, zip_flags_t flags) 
{
	zip_dirent_t * de;
	uint32 len;
	const uint8 * str;
	if((de = _zip_get_dirent(za, idx, flags, NULL)) == NULL)
		return NULL;
	if((str = _zip_string_get(de->comment, &len, flags, &za->error)) == NULL)
		return NULL;
	ASSIGN_PTR(lenp, len);
	return reinterpret_cast<const char *>(str);
}
//
// set comment for file in archive
//
ZIP_EXTERN int zip_file_set_comment(zip_t * za, uint64 idx, const char * comment, uint16 len, zip_flags_t flags) 
{
	zip_entry_t * e;
	zip_string_t * cstr;
	int changed;
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(len > 0 && comment == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(len > 0) {
		if((cstr = _zip_string_new((const uint8*)comment, len, flags, &za->error)) == NULL)
			return -1;
		if((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS && _zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			cstr->encoding = ZIP_ENCODING_UTF8_KNOWN;
	}
	else
		cstr = NULL;
	e = za->entry + idx;
	if(e->changes) {
		_zip_string_free(e->changes->comment);
		e->changes->comment = NULL;
		e->changes->changed &= ~ZIP_DIRENT_COMMENT;
	}
	if(e->orig && e->orig->comment)
		changed = !_zip_string_equal(e->orig->comment, cstr);
	else
		changed = (cstr != NULL);
	if(changed) {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				_zip_string_free(cstr);
				return -1;
			}
		}
		e->changes->comment = cstr;
		e->changes->changed |= ZIP_DIRENT_COMMENT;
	}
	else {
		_zip_string_free(cstr);
		if(e->changes && e->changes->changed == 0) {
			_zip_dirent_free(e->changes);
			e->changes = NULL;
		}
	}
	return 0;
}
//
// get opsys/external attributes
//
int zip_file_get_external_attributes(zip_t * za, uint64 idx, zip_flags_t flags, uint8 * opsys, uint32 * attributes) 
{
	zip_dirent_t * de;
	if((de = _zip_get_dirent(za, idx, flags, NULL)) == NULL)
		return -1;
	ASSIGN_PTR(opsys, static_cast<uint8>((de->version_madeby >> 8) & 0xff));
	ASSIGN_PTR(attributes, de->ext_attrib);
	return 0;
}
//
// set external attributes for entry
//
ZIP_EXTERN int zip_file_set_external_attributes(zip_t * za, uint64 idx, zip_flags_t flags, uint8 opsys, uint32 attributes) 
{
	zip_entry_t * e;
	int changed;
	uint8 unchanged_opsys;
	uint32 unchanged_attributes;
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	e = za->entry + idx;
	unchanged_opsys = (e->orig ? (uint8)(e->orig->version_madeby >> 8) : (uint8)ZIP_OPSYS_DEFAULT);
	unchanged_attributes = e->orig ? e->orig->ext_attrib : ZIP_EXT_ATTRIB_DEFAULT;
	changed = (opsys != unchanged_opsys || attributes != unchanged_attributes);
	if(changed) {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		e->changes->version_madeby = (uint16)((opsys << 8) | (e->changes->version_madeby & 0xff));
		e->changes->ext_attrib = attributes;
		e->changes->changed |= ZIP_DIRENT_ATTRIBUTES;
	}
	else if(e->changes) {
		e->changes->changed &= ~ZIP_DIRENT_ATTRIBUTES;
		if(e->changes->changed == 0) {
			_zip_dirent_free(e->changes);
			e->changes = NULL;
		}
		else {
			e->changes->version_madeby = (uint16)((unchanged_opsys << 8) | (e->changes->version_madeby & 0xff));
			e->changes->ext_attrib = unchanged_attributes;
		}
	}
	return 0;
}
// 
// get offset of file data in archive.
//
// _zip_file_get_offset(za, ze):
// Returns the offset of the file data for entry ze.
// 
// On error, fills in za->error and returns 0.
// 
uint64 _zip_file_get_offset(const zip_t * za, uint64 idx, zip_error_t * error) 
{
	uint64 offset;
	int32 size;
	if(za->entry[idx].orig == NULL) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return 0;
	}
	offset = za->entry[idx].orig->offset;
	if(zip_source_seek(za->src, (int64)offset, SEEK_SET) < 0) {
		_zip_error_set_from_source(error, za->src);
		return 0;
	}
	// TODO: cache? 
	if((size = _zip_dirent_size(za->src, ZIP_EF_LOCAL, error)) < 0)
		return 0;
	if(offset + (uint32)size > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return 0;
	}
	return offset + (uint32)size;
}

uint64 _zip_file_get_end(const zip_t * za, uint64 index, zip_error_t * error) 
{
	uint64 offset;
	zip_dirent_t * entry;
	if((offset = _zip_file_get_offset(za, index, error)) == 0) {
		return 0;
	}
	entry = za->entry[index].orig;
	if(offset + entry->comp_size < offset || offset + entry->comp_size > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return 0;
	}
	offset += entry->comp_size;
	if(entry->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
		uint8 buf[4];
		if(zip_source_seek(za->src, (int64)offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, za->src);
			return 0;
		}
		if(zip_source_read(za->src, buf, 4) != 4) {
			_zip_error_set_from_source(error, za->src);
			return 0;
		}
		if(memcmp(buf, DATADES_MAGIC, 4) == 0) {
			offset += 4;
		}
		offset += 12;
		if(_zip_dirent_needs_zip64(entry, 0)) {
			offset += 8;
		}
		if(offset > ZIP_INT64_MAX) {
			zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
			return 0;
		}
	}
	return offset;
}
//
// set encryption for file in archive
//
ZIP_EXTERN int zip_file_set_encryption(zip_t * za, uint64 idx, uint16 method, const char * password) 
{
	zip_entry_t * e;
	uint16 old_method;
	if(idx >= za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(method != ZIP_EM_NONE && _zip_get_encryption_implementation(method, ZIP_CODEC_ENCODE) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
		return -1;
	}
	e = za->entry + idx;
	old_method = (e->orig == NULL ? ZIP_EM_NONE : e->orig->encryption_method);
	if(method == old_method && password == NULL) {
		if(e->changes) {
			if(e->changes->changed & ZIP_DIRENT_PASSWORD) {
				_zip_crypto_clear(e->changes->password, strlen(e->changes->password));
				SAlloc::F(e->changes->password);
				e->changes->password = (e->orig ? e->orig->password : 0);
			}
			e->changes->changed &= ~(ZIP_DIRENT_ENCRYPTION_METHOD | ZIP_DIRENT_PASSWORD);
			if(e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	}
	else {
		char * our_password = NULL;
		if(password) {
			if((our_password = strdup(password)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				if(our_password) {
					_zip_crypto_clear(our_password, strlen(our_password));
				}
				SAlloc::F(our_password);
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		e->changes->encryption_method = method;
		e->changes->changed |= ZIP_DIRENT_ENCRYPTION_METHOD;
		if(password) {
			e->changes->password = our_password;
			e->changes->changed |= ZIP_DIRENT_PASSWORD;
		}
		else {
			if(e->changes->changed & ZIP_DIRENT_PASSWORD) {
				_zip_crypto_clear(e->changes->password, strlen(e->changes->password));
				SAlloc::F(e->changes->password);
				e->changes->password = e->orig ? e->orig->password : NULL;
				e->changes->changed &= ~ZIP_DIRENT_PASSWORD;
			}
		}
	}
	return 0;
}
//
// set modification time of entry.
//
ZIP_EXTERN int zip_file_set_dostime(zip_t * za, uint64 idx, uint16 dtime, uint16 ddate, zip_flags_t flags) 
{
	time_t mtime = _zip_d2u_time(dtime, ddate);
	return zip_file_set_mtime(za, idx, mtime, flags);
}

ZIP_EXTERN int zip_file_set_mtime(zip_t * za, uint64 idx, time_t mtime, zip_flags_t flags) 
{
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	else if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	else {
		zip_entry_t * e = za->entry + idx;
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		e->changes->last_mod = mtime;
		e->changes->changed |= ZIP_DIRENT_LAST_MOD;
		return 0;
	}
}
//
// get zip file error
//
ZIP_EXTERN void zip_file_error_get(zip_file_t * zf, int * zep, int * sep) 
{
	_zip_error_get(&zf->error, zep, sep);
}
//
// clear zip file error
//
ZIP_EXTERN void zip_file_error_clear(zip_file_t * zf) 
{
	if(zf)
		_zip_error_clear(&zf->error);
}
//
// zip-buffer
// bounds checked access to memory buffer
//
uint8 * _zip_buffer_data(zip_buffer_t * buffer) { return buffer->data; }
static uint64 FASTCALL _zip_buffer_size(const zip_buffer_t * buffer) { return buffer->size; }
static bool   FASTCALL _zip_buffer_eof(const zip_buffer_t * buffer) { return buffer->ok && buffer->offset == buffer->size; }
static uint64 FASTCALL _zip_buffer_left(const zip_buffer_t * buffer) { return buffer->ok ? (buffer->size - buffer->offset) : 0; }
static uint64 FASTCALL _zip_buffer_offset(const zip_buffer_t * buffer) { return buffer->ok ? buffer->offset : 0; }
static bool FASTCALL _zip_buffer_ok(const zip_buffer_t * buffer) { return buffer->ok; }

static void FASTCALL _zip_buffer_free(zip_buffer_t * buffer) 
{
	if(buffer) {
		if(buffer->free_data)
			SAlloc::F(buffer->data);
		SAlloc::F(buffer);
	}
}

static uint8 * FASTCALL _zip_buffer_peek(zip_buffer_t * buffer, uint64 length) 
{
	uint8 * data = 0;
	if(!buffer->ok || buffer->offset + length < length || buffer->offset + length > buffer->size)
		buffer->ok = false;
	else
		data = buffer->data + buffer->offset;
	return data;
}

static uint8 * FASTCALL _zip_buffer_get(zip_buffer_t * buffer, uint64 length) 
{
	uint8 * data = _zip_buffer_peek(buffer, length);
	if(data)
		buffer->offset += length;
	return data;
}

uint16 _zip_buffer_get_16(zip_buffer_t * buffer) 
{
	uint8 * data = _zip_buffer_get(buffer, 2);
	return data ? (uint16)(data[0] + (data[1] << 8)) : 0;
}

uint32 _zip_buffer_get_32(zip_buffer_t * buffer) 
{
	uint8 * data = _zip_buffer_get(buffer, 4);
	return data ? (((((((uint32)data[3] << 8) + data[2]) << 8) + data[1]) << 8) + data[0]) : 0;
}

uint64 _zip_buffer_get_64(zip_buffer_t * buffer) 
{
	uint8 * data = _zip_buffer_get(buffer, 8);
	if(!data) {
		return 0;
	}
	return ((uint64)data[7] << 56) + ((uint64)data[6] << 48) + ((uint64)data[5] << 40) +
	       ((uint64)data[4] << 32) + ((uint64)data[3] << 24) + ((uint64)data[2] << 16) +
	       ((uint64)data[1] << 8) +
	       (uint64)data[0];
}

uint8 _zip_buffer_get_8(zip_buffer_t * buffer) 
{
	uint8 * data = _zip_buffer_get(buffer, 1);
	return data ? data[0] : 0;
}

static uint64 _zip_buffer_read(zip_buffer_t * buffer, uint8 * data, uint64 length) 
{
	if(_zip_buffer_left(buffer) < length)
		length = _zip_buffer_left(buffer);
	memcpy(data, _zip_buffer_get(buffer, length), static_cast<size_t>(length));
	return length;
}

static zip_buffer_t * FASTCALL _zip_buffer_new(uint8 * data, uint64 size) 
{
	const bool free_data = (data == NULL);
	zip_buffer_t * buffer = 0;
	SETIFZ(data, static_cast<uint8 *>(SAlloc::M(static_cast<size_t>(size))));
	if(data) {
		buffer = static_cast<zip_buffer_t *>(SAlloc::M(sizeof(*buffer)));
		if(!buffer) {
			if(free_data)
				SAlloc::F(data);
		}
		else {
			buffer->ok = true;
			buffer->data = data;
			buffer->size = size;
			buffer->offset = 0;
			buffer->free_data = free_data;
		}
	}
	return buffer;
}

static zip_buffer_t * _zip_buffer_new_from_source(zip_source_t * src, uint64 size, uint8 * buf, zip_error_t * error) 
{
	zip_buffer_t * buffer = _zip_buffer_new(buf, size);
	if(!buffer)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		if(_zip_read(src, buffer->data, size, error) < 0) {
			_zip_buffer_free(buffer);
			buffer = 0;
		}
	}
	return buffer;
}

static int FASTCALL _zip_buffer_put(zip_buffer_t * buffer, const void * src, size_t length) 
{
	uint8 * dst = _zip_buffer_get(buffer, length);
	if(dst == NULL)
		return -1;
	else {
		memcpy(dst, src, length);
		return 0;
	}
}

static int FASTCALL _zip_buffer_put_16(zip_buffer_t * buffer, uint16 i) 
{
	uint8 * data = _zip_buffer_get(buffer, 2);
	if(!data)
		return -1;
	else {
		data[0] = (uint8)(i & 0xff);
		data[1] = (uint8)((i >> 8) & 0xff);
		return 0;
	}
}

static int FASTCALL _zip_buffer_put_32(zip_buffer_t * buffer, uint32 i) 
{
	uint8 * data = _zip_buffer_get(buffer, 4);
	if(!data) {
		return -1;
	}
	else {
		data[0] = (uint8)(i & 0xff);
		data[1] = (uint8)((i >> 8) & 0xff);
		data[2] = (uint8)((i >> 16) & 0xff);
		data[3] = (uint8)((i >> 24) & 0xff);
		return 0;
	}
}

static int FASTCALL _zip_buffer_put_64(zip_buffer_t * buffer, uint64 i) 
{
	uint8 * data = _zip_buffer_get(buffer, 8);
	if(!data)
		return -1;
	else {
		data[0] = (uint8)(i & 0xff);
		data[1] = (uint8)((i >> 8) & 0xff);
		data[2] = (uint8)((i >> 16) & 0xff);
		data[3] = (uint8)((i >> 24) & 0xff);
		data[4] = (uint8)((i >> 32) & 0xff);
		data[5] = (uint8)((i >> 40) & 0xff);
		data[6] = (uint8)((i >> 48) & 0xff);
		data[7] = (uint8)((i >> 56) & 0xff);
		return 0;
	}
}

static int FASTCALL _zip_buffer_put_8(zip_buffer_t * buffer, uint8 i) 
{
	uint8 * data = _zip_buffer_get(buffer, 1);
	if(!data)
		return -1;
	else {
		data[0] = i;
		return 0;
	}
}

static int FASTCALL _zip_buffer_set_offset(zip_buffer_t * buffer, uint64 offset) 
{
	if(offset > buffer->size) {
		buffer->ok = false;
		return -1;
	}
	else {
		buffer->ok = true;
		buffer->offset = offset;
		return 0;
	}
}

static int _zip_buffer_skip(zip_buffer_t * buffer, uint64 length) 
{
	uint64 offset = buffer->offset + length;
	if(offset < buffer->offset) {
		buffer->ok = false;
		return -1;
	}
	else
		return _zip_buffer_set_offset(buffer, offset);
}
//
// zip-io-util
// I/O helper functions
//
int _zip_read(zip_source_t * src, uint8 * b, uint64 length, zip_error_t * error) 
{
	if(length > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	else {
		const int64 n = zip_source_read(src, b, length);
		if(n < 0) {
			_zip_error_set_from_source(error, src);
			return -1;
		}
		else if(n < (int64)length) {
			zip_error_set(error, SLERR_ZIP_EOF, 0);
			return -1;
		}
		else
			return 0;
	}
}

uint8 * _zip_read_data(zip_buffer_t * buffer, zip_source_t * src, size_t length, bool nulp, zip_error_t * error) 
{
	uint8 * r = 0;
	if(length || nulp) {
		r = (uint8 *)SAlloc::M(length + (nulp ? 1 : 0));
		if(!r)
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		else {
			if(buffer) {
				uint8 * data = _zip_buffer_get(buffer, length);
				if(!data) {
					zip_error_set(error, SLERR_ZIP_MEMORY, 0);
					SAlloc::F(r);
					return NULL;
				}
				memcpy(r, data, length);
			}
			else {
				if(_zip_read(src, r, length, error) < 0) {
					SAlloc::F(r);
					return NULL;
				}
			}
			if(nulp) {
				// replace any in-string NUL characters with spaces 
				r[length] = 0;
				for(uint8 * o = r; o < r + length; o++)
					if(*o == '\0')
						*o = ' ';
			}
		}
	}
	return r;
}

zip_string_t * _zip_read_string(zip_buffer_t * buffer, zip_source_t * src, uint16 len, bool nulp, zip_error_t * error) 
{
	zip_string_t * s = 0;
	uint8 * raw = _zip_read_data(buffer, src, len, nulp, error);
	if(raw) {
		s = _zip_string_new(raw, len, ZIP_FL_ENC_GUESS, error);
		SAlloc::F(raw);
	}
	return s;
}

int _zip_write(zip_t * za, const void * data, uint64 length) 
{
	int64 n;
	if((n = zip_source_write(za->src, data, length)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if((uint64)n != length) {
		zip_error_set(&za->error, SLERR_ZIP_WRITE, EINTR);
		return -1;
	}
	return 0;
}
//
// zip_hash
// hash table string -> uint64
//
// parameter for the string hash function 
#define HASH_MULTIPLIER 33
#define HASH_START 5381
// hash table's fill ratio is kept between these by doubling/halfing its size as necessary 
#define HASH_MAX_FILL .75
#define HASH_MIN_FILL .01
// but hash table size is kept between these 
#define HASH_MIN_SIZE 256
#define HASH_MAX_SIZE 0x80000000ul

struct zip_hash_entry {
	const uint8 * name;
	int64 orig_index;
	int64 current_index;
	struct zip_hash_entry * next;
	uint32 hash_value;
};

typedef struct zip_hash_entry zip_hash_entry_t;

struct zip_hash {
	uint32 table_size;
	uint64 nentries;
	zip_hash_entry_t ** table;
};
//
// Descr: free list of entries 
//
static void free_list(zip_hash_entry_t * entry) 
{
	while(entry) {
		zip_hash_entry_t * next = entry->next;
		SAlloc::F(entry);
		entry = next;
	}
}
//
// Descr: compute hash of string, full 32 bit value 
//
static uint32 hash_string(const uint8 * name) 
{
	uint64 value = HASH_START;
	if(name == NULL) {
		return 0;
	}
	while(*name != 0) {
		value = (uint64)(((value * HASH_MULTIPLIER) + (uint8)*name) % 0x100000000ul);
		name++;
	}
	return (uint32)value;
}
//
// resize hash table; new_size must be a power of 2, can be larger or smaller than current size 
//
static bool hash_resize(zip_hash_t * hash, uint32 new_size, zip_error_t * error) 
{
	if(new_size != hash->table_size) {
		zip_hash_entry_t ** new_table = (zip_hash_entry_t **)SAlloc::C(new_size, sizeof(zip_hash_entry_t *));
		if(!new_table) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return false;
		}
		else {
			if(hash->nentries > 0) {
				for(uint32 i = 0; i < hash->table_size; i++) {
					zip_hash_entry_t * entry = hash->table[i];
					while(entry) {
						zip_hash_entry_t * next = entry->next;
						uint32 new_index = entry->hash_value % new_size;
						entry->next = new_table[new_index];
						new_table[new_index] = entry;
						entry = next;
					}
				}
			}
			SAlloc::F(hash->table);
			hash->table = new_table;
			hash->table_size = new_size;
		}
	}
	return true;
}

static uint32 size_for_capacity(uint64 capacity) 
{
	double needed_size = capacity / HASH_MAX_FILL;
	uint32 v = (needed_size > ZIP_UINT32_MAX) ? ZIP_UINT32_MAX : static_cast<uint32>(needed_size);
	if(v > HASH_MAX_SIZE) {
		return HASH_MAX_SIZE;
	}
	/* From Bit Twiddling Hacks by Sean Eron Anderson <seander@cs.stanford.edu>
	   (http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2). */
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

zip_hash_t * _zip_hash_new(zip_error_t * error) 
{
	zip_hash_t * hash = (zip_hash_t*)SAlloc::M(sizeof(zip_hash_t));
	if(!hash)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		hash->table_size = 0;
		hash->nentries = 0;
		hash->table = NULL;
	}
	return hash;
}

void _zip_hash_free(zip_hash_t * hash) 
{
	if(hash) {
		if(hash->table != NULL) {
			for(uint32 i = 0; i < hash->table_size; i++) {
				if(hash->table[i])
					free_list(hash->table[i]);
			}
			SAlloc::F(hash->table);
		}
		SAlloc::F(hash);
	}
}
//
// Descr: insert into hash, return error on existence or memory issues 
//
bool _zip_hash_add(zip_hash_t * hash, const uint8 * name, uint64 index, zip_flags_t flags, zip_error_t * error) 
{
	uint32 hash_value, table_index;
	zip_hash_entry_t * entry;
	if(!hash || !name || index > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	if(hash->table_size == 0) {
		if(!hash_resize(hash, HASH_MIN_SIZE, error)) {
			return false;
		}
	}
	hash_value = hash_string(name);
	table_index = hash_value % hash->table_size;
	for(entry = hash->table[table_index]; entry != NULL; entry = entry->next) {
		if(entry->hash_value == hash_value && strcmp((const char*)name, (const char*)entry->name) == 0) {
			if(((flags & ZIP_FL_UNCHANGED) && entry->orig_index != -1) || entry->current_index != -1) {
				zip_error_set(error, SLERR_ZIP_EXISTS, 0);
				return false;
			}
			else {
				break;
			}
		}
	}
	if(!entry) {
		entry = static_cast<zip_hash_entry_t *>(SAlloc::M(sizeof(zip_hash_entry_t)));
		if(!entry) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return false;
		}
		entry->name = name;
		entry->next = hash->table[table_index];
		hash->table[table_index] = entry;
		entry->hash_value = hash_value;
		entry->orig_index = -1;
		hash->nentries++;
		if(hash->nentries > hash->table_size * HASH_MAX_FILL && hash->table_size < HASH_MAX_SIZE) {
			if(!hash_resize(hash, hash->table_size * 2, error)) {
				return false;
			}
		}
	}
	if(flags & ZIP_FL_UNCHANGED) {
		entry->orig_index = (int64)index;
	}
	entry->current_index = (int64)index;
	return true;
}
//
// Descr: remove entry from hash, error if not found 
//
bool _zip_hash_delete(zip_hash_t * hash, const uint8 * name, zip_error_t * error) 
{
	uint32 hash_value, index;
	zip_hash_entry_t * entry, * previous;
	if(!hash || !name) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	if(hash->nentries > 0) {
		hash_value = hash_string(name);
		index = hash_value % hash->table_size;
		previous = NULL;
		entry = hash->table[index];
		while(entry) {
			if(entry->hash_value == hash_value && strcmp((const char*)name, (const char*)entry->name) == 0) {
				if(entry->orig_index == -1) {
					if(previous) {
						previous->next = entry->next;
					}
					else {
						hash->table[index] = entry->next;
					}
					SAlloc::F(entry);
					hash->nentries--;
					if(hash->nentries < hash->table_size * HASH_MIN_FILL && hash->table_size > HASH_MIN_SIZE) {
						if(!hash_resize(hash, hash->table_size / 2, error)) {
							return false;
						}
					}
				}
				else {
					entry->current_index = -1;
				}
				return true;
			}
			previous = entry;
			entry = entry->next;
		}
	}
	zip_error_set(error, SLERR_ZIP_NOENT, 0);
	return false;
}
//
// Descr: find value for entry in hash, -1 if not found 
//
int64 _zip_hash_lookup(zip_hash_t * hash, const uint8 * name, zip_flags_t flags, zip_error_t * error) 
{
	uint32 hash_value, index;
	zip_hash_entry_t * entry;
	if(!hash || !name) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(hash->nentries > 0) {
		hash_value = hash_string(name);
		index = hash_value % hash->table_size;
		for(entry = hash->table[index]; entry != NULL; entry = entry->next) {
			if(strcmp((const char*)name, (const char*)entry->name) == 0) {
				if(flags & ZIP_FL_UNCHANGED) {
					if(entry->orig_index != -1) {
						return entry->orig_index;
					}
				}
				else {
					if(entry->current_index != -1) {
						return entry->current_index;
					}
				}
				break;
			}
		}
	}
	zip_error_set(error, SLERR_ZIP_NOENT, 0);
	return -1;
}

bool _zip_hash_reserve_capacity(zip_hash_t * hash, uint64 capacity, zip_error_t * error) 
{
	if(capacity) {
		uint32 new_size = size_for_capacity(capacity);
		if(new_size <= hash->table_size)
			return true;
		else if(!hash_resize(hash, new_size, error))
			return false;
	}
	return true;
}

bool _zip_hash_revert(zip_hash_t * hash, zip_error_t * error) 
{
	for(uint32 i = 0; i < hash->table_size; i++) {
		zip_hash_entry_t * previous = NULL;
		zip_hash_entry_t * entry = hash->table[i];
		while(entry) {
			if(entry->orig_index == -1) {
				if(previous)
					previous->next = entry->next;
				else
					hash->table[i] = entry->next;
				zip_hash_entry_t * p = entry;
				entry = entry->next;
				// previous does not change 
				SAlloc::F(p);
				hash->nentries--;
			}
			else {
				entry->current_index = entry->orig_index;
				previous = entry;
				entry = entry->next;
			}
		}
	}
	if(hash->nentries < hash->table_size * HASH_MIN_FILL && hash->table_size > HASH_MIN_SIZE) {
		uint32 new_size = hash->table_size / 2;
		while(hash->nentries < new_size * HASH_MIN_FILL && new_size > HASH_MIN_SIZE) {
			new_size /= 2;
		}
		if(!hash_resize(hash, new_size, error))
			return false;
	}
	return true;
}
//
// zip_string
// string handling (with encoding)
//
uint32 _zip_string_crc32(const zip_string_t * s) 
{
	uint32 crc = (uint32)crc32(0L, Z_NULL, 0);
	if(s)
		crc = (uint32)crc32(crc, s->raw, s->length);
	return crc;
}

int _zip_string_equal(const zip_string_t * a, const zip_string_t * b) 
{
	if(a == NULL || b == NULL)
		return a == b;
	if(a->length != b->length)
		return 0;
	/* TODO: encoding */
	return (memcmp(a->raw, b->raw, a->length) == 0);
}

void _zip_string_free(zip_string_t * s) 
{
	if(s) {
		SAlloc::F(s->raw);
		SAlloc::F(s->converted);
		SAlloc::F(s);
	}
}

const uint8 * _zip_string_get(zip_string_t * string, uint32 * lenp, zip_flags_t flags, zip_error_t * error) 
{
	static const uint8 empty[1] = "";
	if(string == NULL) {
		ASSIGN_PTR(lenp, 0);
		return empty;
	}
	else {
		if((flags & ZIP_FL_ENC_RAW) == 0) {
			// start guessing 
			if(string->encoding == ZIP_ENCODING_UNKNOWN)
				_zip_guess_encoding(string, ZIP_ENCODING_UNKNOWN);
			if(((flags & ZIP_FL_ENC_STRICT) && string->encoding != ZIP_ENCODING_ASCII && string->encoding != ZIP_ENCODING_UTF8_KNOWN) ||
				(string->encoding == ZIP_ENCODING_CP437)) {
				if(string->converted == NULL) {
					if((string->converted = _zip_cp437_to_utf8(string->raw, string->length, &string->converted_length, error)) == NULL)
						return NULL;
				}
				ASSIGN_PTR(lenp, string->converted_length);
				return string->converted;
			}
		}
		ASSIGN_PTR(lenp, string->length);
		return string->raw;
	}
}

static uint16 FASTCALL _zip_string_length(const zip_string_t * s) 
{
	return s ? s->length : 0;
}

zip_string_t * _zip_string_new(const uint8 * raw, uint16 length, zip_flags_t flags, zip_error_t * error) 
{
	zip_string_t * s;
	zip_encoding_type_t expected_encoding;
	if(length == 0)
		return NULL;
	switch(flags & ZIP_FL_ENCODING_ALL) {
		case ZIP_FL_ENC_GUESS: expected_encoding = ZIP_ENCODING_UNKNOWN; break;
		case ZIP_FL_ENC_UTF_8: expected_encoding = ZIP_ENCODING_UTF8_KNOWN; break;
		case ZIP_FL_ENC_CP437: expected_encoding = ZIP_ENCODING_CP437; break;
		default: zip_error_set(error, SLERR_ZIP_INVAL, 0); return NULL;
	}
	if((s = (zip_string_t*)SAlloc::M(sizeof(*s))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if((s->raw = (uint8 *)SAlloc::M((size_t)length + 1)) == NULL) {
		SAlloc::F(s);
		return NULL;
	}
	memcpy(s->raw, raw, length);
	s->raw[length] = '\0';
	s->length = length;
	s->encoding = ZIP_ENCODING_UNKNOWN;
	s->converted = NULL;
	s->converted_length = 0;
	if(expected_encoding != ZIP_ENCODING_UNKNOWN) {
		if(_zip_guess_encoding(s, expected_encoding) == ZIP_ENCODING_ERROR) {
			_zip_string_free(s);
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
			return NULL;
		}
	}
	return s;
}

int _zip_string_write(zip_t * za, const zip_string_t * s) 
{
	return s ? _zip_write(za, s->raw, s->length) : 0;
}
//
// zip_utf-8
// UTF-8 support functions for libzip
//
static const uint16 _cp437_to_unicode[256] = {
	/* 0x00 - 0x0F */
	0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	/* 0x10 - 0x1F */
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
	/* 0x20 - 0x2F */
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	/* 0x30 - 0x3F */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	/* 0x40 - 0x4F */
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	/* 0x50 - 0x5F */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	/* 0x60 - 0x6F */
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	/* 0x70 - 0x7F */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
	/* 0x80 - 0x8F */
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	/* 0x90 - 0x9F */
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	/* 0xA0 - 0xAF */
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	/* 0xB0 - 0xBF */
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	/* 0xC0 - 0xCF */
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	/* 0xD0 - 0xDF */
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	/* 0xE0 - 0xEF */
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	/* 0xF0 - 0xFF */
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

#define UTF_8_LEN_2_MASK 0xe0
#define UTF_8_LEN_2_MATCH 0xc0
#define UTF_8_LEN_3_MASK 0xf0
#define UTF_8_LEN_3_MATCH 0xe0
#define UTF_8_LEN_4_MASK 0xf8
#define UTF_8_LEN_4_MATCH 0xf0
#define UTF_8_CONTINUE_MASK 0xc0
#define UTF_8_CONTINUE_MATCH 0x80

zip_encoding_type_t _zip_guess_encoding(zip_string_t * str, zip_encoding_type_t expected_encoding) 
{
	zip_encoding_type_t enc;
	const uint8 * name;
	uint32 i, j, ulen;
	if(str == NULL)
		return ZIP_ENCODING_ASCII;
	name = str->raw;
	if(str->encoding != ZIP_ENCODING_UNKNOWN)
		enc = str->encoding;
	else {
		enc = ZIP_ENCODING_ASCII;
		for(i = 0; i < str->length; i++) {
			if((name[i] > 31 && name[i] < 128) || name[i] == '\r' || name[i] == '\n' || name[i] == '\t')
				continue;
			enc = ZIP_ENCODING_UTF8_GUESSED;
			if((name[i] & UTF_8_LEN_2_MASK) == UTF_8_LEN_2_MATCH)
				ulen = 1;
			else if((name[i] & UTF_8_LEN_3_MASK) == UTF_8_LEN_3_MATCH)
				ulen = 2;
			else if((name[i] & UTF_8_LEN_4_MASK) == UTF_8_LEN_4_MATCH)
				ulen = 3;
			else {
				enc = ZIP_ENCODING_CP437;
				break;
			}
			if((i + ulen) >= str->length) {
				enc = ZIP_ENCODING_CP437;
				break;
			}
			for(j = 1; j <= ulen; j++) {
				if((name[i + j] & UTF_8_CONTINUE_MASK) != UTF_8_CONTINUE_MATCH) {
					enc = ZIP_ENCODING_CP437;
					goto done;
				}
			}
			i += ulen;
		}
	}
done:
	str->encoding = enc;
	if(expected_encoding != ZIP_ENCODING_UNKNOWN) {
		if(expected_encoding == ZIP_ENCODING_UTF8_KNOWN && enc == ZIP_ENCODING_UTF8_GUESSED)
			str->encoding = enc = ZIP_ENCODING_UTF8_KNOWN;
		if(expected_encoding != enc && enc != ZIP_ENCODING_ASCII)
			return ZIP_ENCODING_ERROR;
	}
	return enc;
}

static uint32 _zip_unicode_to_utf8_len(uint32 codepoint) 
{
	return (codepoint < 0x0080) ?  1 : ((codepoint < 0x0800) ? 2 : ((codepoint < 0x10000) ? 3 : 4));
}

static uint32 _zip_unicode_to_utf8(uint32 codepoint, uint8 * buf) 
{
	if(codepoint < 0x0080) {
		buf[0] = static_cast<uint8>(codepoint & 0xff);
		return 1;
	}
	if(codepoint < 0x0800) {
		buf[0] = (uint8)(UTF_8_LEN_2_MATCH | ((codepoint >> 6) & 0x1f));
		buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
		return 2;
	}
	if(codepoint < 0x10000) {
		buf[0] = (uint8)(UTF_8_LEN_3_MATCH | ((codepoint >> 12) & 0x0f));
		buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 6) & 0x3f));
		buf[2] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
		return 3;
	}
	buf[0] = (uint8)(UTF_8_LEN_4_MATCH | ((codepoint >> 18) & 0x07));
	buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 12) & 0x3f));
	buf[2] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 6) & 0x3f));
	buf[3] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
	return 4;
}

uint8 * _zip_cp437_to_utf8(const uint8 * const _cp437buf, uint32 len, uint32 * utf8_lenp, zip_error_t * error) 
{
	uint8 * cp437buf = (uint8 *)_cp437buf;
	uint8 * utf8buf;
	uint32 buflen, i, offset;
	if(len == 0) {
		ASSIGN_PTR(utf8_lenp, 0);
		return NULL;
	}
	buflen = 1;
	for(i = 0; i < len; i++)
		buflen += _zip_unicode_to_utf8_len(_cp437_to_unicode[cp437buf[i]]);
	if((utf8buf = (uint8 *)SAlloc::M(buflen)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	offset = 0;
	for(i = 0; i < len; i++)
		offset += _zip_unicode_to_utf8(_cp437_to_unicode[cp437buf[i]], utf8buf + offset);
	utf8buf[buflen - 1] = 0;
	ASSIGN_PTR(utf8_lenp, buflen - 1);
	return utf8buf;
}
//
// zip_error
// zip_error_t helper functions
//
#define N ZIP_ET_NONE
#define S ZIP_ET_SYS
#define Z ZIP_ET_ZLIB

static int FASTCALL GetZipErrType(int errCode)
{
	if(oneof9(errCode, SLERR_ZIP_RENAME, SLERR_ZIP_CLOSE, SLERR_ZIP_SEEK, SLERR_ZIP_READ, SLERR_ZIP_WRITE, SLERR_ZIP_OPEN, 
		SLERR_ZIP_TMPOPEN, SLERR_ZIP_REMOVE, SLERR_ZIP_TELL))
		return ZIP_ET_SYS;
	else if(errCode == SLERR_ZIP_ZLIB)
		return ZIP_ET_ZLIB;
	else
		return ZIP_ET_NONE;
}

ZIP_EXTERN int zip_error_code_system(const zip_error_t * error) { return error->sys_err; }
ZIP_EXTERN int zip_error_code_zip(const zip_error_t * error) { return error->zip_err; }

ZIP_EXTERN void zip_error_fini(zip_error_t * pErr) 
{
	ZFREE(pErr->str);
}

ZIP_EXTERN void FASTCALL zip_error_init(zip_error_t * pErr) 
{
	if(pErr) {
		pErr->zip_err = SLERR_SUCCESS;
		pErr->sys_err = 0;
		pErr->str = NULL;
	}
}
//
// get string representation of zip file error
//
// @sobolev ZIP_EXTERN const char * zip_file_strerror(zip_file_t * zf) { return zip_error_strerror(&zf->error); }
//
// Descr: get string representation of zip error
//
// @sobolev ZIP_EXTERN const char * zip_strerror(zip_t * za) { return zip_error_strerror(&za->error); }

ZIP_EXTERN void zip_error_init_with_code(zip_error_t * error, int ze) 
{
	zip_error_init(error);
	error->zip_err = ze;
	switch(zip_error_system_type(error)) {
		case ZIP_ET_SYS: error->sys_err = errno; break;
		default: error->sys_err = 0; break;
	}
}

ZIP_EXTERN int zip_error_system_type(const zip_error_t * pError) 
{
	// @sobolev return (pError->zip_err < 0 || pError->zip_err >= _zip_nerr_str) ? ZIP_ET_NONE : GetZipErrType(pError->zip_err);
	return pError ? GetZipErrType(pError->zip_err) : ZIP_ET_NONE; // @sobolev 
}

// @sobolev ZIP_EXTERN int zip_error_get_sys_type(int ze) { return (ze < 0 || ze >= _zip_nerr_str) ? 0 : GetZipErrType(ze); }
#if 0 // @sobolev {
ZIP_EXTERN const char * zip_error_strerror(zip_error_t * err) 
{
	const char * zs, * ss;
	char buf[128], * s;
	zip_error_fini(err);
	if(err->zip_err < 0 || err->zip_err >= _zip_nerr_str) {
		sprintf(buf, "Unknown error %d", err->zip_err);
		zs = NULL;
		ss = buf;
	}
	else {
		zs = _zip_err_str[err->zip_err];
		switch(GetZipErrType(err->zip_err)) {
			case ZIP_ET_SYS: ss = strerror(err->sys_err); break;
			case ZIP_ET_ZLIB: ss = zError(err->sys_err); break;
			default: ss = NULL;
		}
	}
	if(ss == NULL)
		return zs;
	else {
		if((s = (char *)SAlloc::M(strlen(ss) + (zs ? strlen(zs) + 2 : 0) + 1)) == NULL)
			return _zip_err_str[SLERR_ZIP_MEMORY];
		sprintf(s, "%s%s%s", (zs ? zs : ""), (zs ? ": " : ""), ss);
		err->str = s;
		return s;
	}
}
#endif // } 0 @sobolev
#if 0 // @sobolev {
#define _ZIP_COMPILING_DEPRECATED
ZIP_EXTERN int zip_error_to_str(char * buf, uint64 len, int ze, int se) 
{
	const char * zs, * ss;
	if(ze < 0 || ze >= _zip_nerr_str)
		return snprintf(buf, static_cast<size_t>(len), "Unknown error %d", ze);
	zs = _zip_err_str[ze];
	switch(GetZipErrType(ze)) {
		case ZIP_ET_SYS: ss = strerror(se); break;
		case ZIP_ET_ZLIB: ss = zError(se); break;
		default: ss = NULL;
	}
	return snprintf(buf, static_cast<size_t>(len), "%s%s%s", zs, (ss ? ": " : ""), (ss ? ss : ""));
}
#undef _ZIP_COMPILING_DEPRECATED
#endif // } 0 @sobolev

void _zip_error_clear(zip_error_t * err) 
{
	if(err) {
		err->zip_err = SLERR_SUCCESS;
		err->sys_err = 0;
	}
}

void _zip_error_copy(zip_error_t * dst, const zip_error_t * src) 
{
	if(dst) {
		dst->zip_err = src->zip_err;
		dst->sys_err = src->sys_err;
	}
}

void _zip_error_get(const zip_error_t * err, int * zep, int * sep) 
{
	ASSIGN_PTR(zep, err->zip_err);
	if(sep)
		*sep = (zip_error_system_type(err) != ZIP_ET_NONE) ? err->sys_err : 0;
}

void FASTCALL zip_error_set(zip_error_t * err, int ze, int se) 
{
	if(err) {
		err->zip_err = ze;
		err->sys_err = se;
	}
}

void _zip_error_set_from_source(zip_error_t * err, zip_source_t * src) { _zip_error_copy(err, zip_source_error(src)); }

int64 zip_error_to_data(const zip_error_t * error, void * data, uint64 length) 
{
	int * e = static_cast<int *>(data);
	if(length < sizeof(int) * 2) {
		return -1;
	}
	else {
		e[0] = zip_error_code_zip(error);
		e[1] = zip_error_code_system(error);
		return sizeof(int) * 2;
	}
}

ZIP_EXTERN void zip_error_get(zip_t * za, int * zep, int * sep) { _zip_error_get(&za->error, zep, sep); }
ZIP_EXTERN zip_error_t * zip_get_error(zip_t * za) { return &za->error; }
ZIP_EXTERN zip_error_t * zip_file_get_error(zip_file_t * f) { return &f->error; }

ZIP_EXTERN void zip_error_clear(zip_t * za) 
{
	if(za)
		_zip_error_clear(&za->error);
}
//
// zip f*-operations
//
ZIP_EXTERN zip_file_t * zip_fopen(zip_t * za, const char * fname, zip_flags_t flags) 
{
	int64 idx;
	if((idx = zip_name_locate(za, fname, flags)) < 0)
		return NULL;
	return zip_fopen_index_encrypted(za, (uint64)idx, flags, za->default_password);
}

ZIP_EXTERN zip_t * zip_fdopen(int fd_orig, int _flags, int * zep) 
{
	int fd;
	FILE * fp;
	zip_t * za;
	zip_source_t * src;
	struct zip_error error;
	if(_flags < 0 || (_flags & ~(ZIP_CHECKCONS | ZIP_RDONLY))) {
		_zip_set_open_error(zep, NULL, SLERR_ZIP_INVAL);
		return NULL;
	}
	// We dup() here to avoid messing with the passed in fd.
	// We could not restore it to the original state in case of error. 
	if((fd = dup(fd_orig)) < 0) {
		_zip_set_open_error(zep, NULL, SLERR_ZIP_OPEN);
		return NULL;
	}
	if((fp = fdopen(fd, "rb")) == NULL) {
		close(fd);
		_zip_set_open_error(zep, NULL, SLERR_ZIP_OPEN);
		return NULL;
	}
	zip_error_init(&error);
	if((src = zip_source_filep_create(fp, 0, -1, &error)) == NULL) {
		fclose(fp);
		_zip_set_open_error(zep, &error, 0);
		zip_error_fini(&error);
		return NULL;
	}
	if((za = zip_open_from_source(src, _flags, &error)) == NULL) {
		zip_source_free(src);
		_zip_set_open_error(zep, &error, 0);
		zip_error_fini(&error);
		return NULL;
	}
	zip_error_fini(&error);
	close(fd_orig);
	return za;
}

ZIP_EXTERN zip_file_t * zip_fopen_encrypted(zip_t * za, const char * fname, zip_flags_t flags, const char * password) 
{
	int64 idx;
	if((idx = zip_name_locate(za, fname, flags)) < 0)
		return NULL;
	return zip_fopen_index_encrypted(za, (uint64)idx, flags, password);
}

ZIP_EXTERN zip_file_t * zip_fopen_index(zip_t * za, uint64 index, zip_flags_t flags) 
{
	return zip_fopen_index_encrypted(za, index, flags, za->default_password);
}

static zip_file_t * _zip_file_new(zip_t * za) 
{
	zip_file_t * zf;
	if((zf = (zip_file_t*)SAlloc::M(sizeof(struct zip_file))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	zf->za = za;
	zip_error_init(&zf->error);
	zf->eof = 0;
	zf->src = NULL;
	return zf;
}

ZIP_EXTERN zip_file_t * zip_fopen_index_encrypted(zip_t * za, uint64 index, zip_flags_t flags, const char * password) 
{
	zip_file_t * zf;
	zip_source_t * src;
	if((src = _zip_source_zip_new(za, za, index, flags, 0, 0, password)) == NULL)
		return NULL;
	if(zip_source_open(src) < 0) {
		_zip_error_set_from_source(&za->error, src);
		zip_source_free(src);
		return NULL;
	}
	if((zf = _zip_file_new(za)) == NULL) {
		zip_source_free(src);
		return NULL;
	}
	zf->src = src;
	return zf;
}

ZIP_EXTERN int zip_fclose(zip_file_t * zf) 
{
	int ret;
	if(zf->src)
		zip_source_free(zf->src);
	ret = 0;
	if(zf->error.zip_err)
		ret = zf->error.zip_err;
	zip_error_fini(&zf->error);
	SAlloc::F(zf);
	return ret;
}

ZIP_EXTERN int64 zip_fread(zip_file_t * zf, void * outbuf, uint64 toread) 
{
	int64 n;
	if(!zf)
		return -1;
	if(zf->error.zip_err != 0)
		return -1;
	if(toread > ZIP_INT64_MAX) {
		zip_error_set(&zf->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if((zf->eof) || (toread == 0))
		return 0;
	if((n = zip_source_read(zf->src, outbuf, toread)) < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}
	return n;
}

ZIP_EXTERN int8 zip_fseek(zip_file_t * zf, int64 offset, int whence) 
{
	if(!zf)
		return -1;
	if(zf->error.zip_err != 0)
		return -1;
	if(zip_source_seek(zf->src, offset, whence) < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}
	return 0;
}

ZIP_EXTERN int64 zip_ftell(zip_file_t * zf) 
{
	int64 res;
	if(!zf)
		return -1;
	if(zf->error.zip_err != 0)
		return -1;
	res = zip_source_tell(zf->src);
	if(res < 0) {
		_zip_error_set_from_source(&zf->error, zf->src);
		return -1;
	}
	return res;
}
//
// zip_stat
//
ZIP_EXTERN void zip_stat_init(zip_stat_t * st) 
{
	st->valid = 0;
	st->name = NULL;
	st->index = ZIP_UINT64_MAX;
	st->crc = 0;
	st->mtime = (time_t)-1;
	st->size = 0;
	st->comp_size = 0;
	st->comp_method = ZIP_CM_STORE;
	st->encryption_method = ZIP_EM_NONE;
}

int _zip_stat_merge(zip_stat_t * dst, const zip_stat_t * src, zip_error_t * error) 
{
	/* name is not merged, since zip_stat_t doesn't own it, and src may not be valid as long as dst */
	if(src->valid & ZIP_STAT_INDEX) {
		dst->index = src->index;
	}
	if(src->valid & ZIP_STAT_SIZE) {
		dst->size = src->size;
	}
	if(src->valid & ZIP_STAT_COMP_SIZE) {
		dst->comp_size = src->comp_size;
	}
	if(src->valid & ZIP_STAT_MTIME) {
		dst->mtime = src->mtime;
	}
	if(src->valid & ZIP_STAT_CRC) {
		dst->crc = src->crc;
	}
	if(src->valid & ZIP_STAT_COMP_METHOD) {
		dst->comp_method = src->comp_method;
	}
	if(src->valid & ZIP_STAT_ENCRYPTION_METHOD) {
		dst->encryption_method = src->encryption_method;
	}
	if(src->valid & ZIP_STAT_FLAGS) {
		dst->flags = src->flags;
	}
	dst->valid |= src->valid;
	return 0;
}

ZIP_EXTERN int zip_stat_index(zip_t * za, uint64 index, zip_flags_t flags, zip_stat_t * st) 
{
	const char * name;
	zip_dirent_t * de;
	if((de = _zip_get_dirent(za, index, flags, NULL)) == NULL)
		return -1;
	if((name = zip_get_name(za, index, flags)) == NULL)
		return -1;
	if((flags & ZIP_FL_UNCHANGED) == 0 && ZIP_ENTRY_DATA_CHANGED(za->entry + index)) {
		zip_entry_t * entry = za->entry + index;
		if(zip_source_stat(entry->source, st) < 0) {
			zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
			return -1;
		}
		if(entry->changes->changed & ZIP_DIRENT_LAST_MOD) {
			st->mtime = de->last_mod;
			st->valid |= ZIP_STAT_MTIME;
		}
	}
	else {
		zip_stat_init(st);
		st->crc = de->crc;
		st->size = de->uncomp_size;
		st->mtime = de->last_mod;
		st->comp_size = de->comp_size;
		st->comp_method = (uint16)de->comp_method;
		st->encryption_method = de->encryption_method;
		st->valid = (de->crc_valid ? ZIP_STAT_CRC : 0) | ZIP_STAT_SIZE | ZIP_STAT_MTIME | ZIP_STAT_COMP_SIZE |
		    ZIP_STAT_COMP_METHOD | ZIP_STAT_ENCRYPTION_METHOD;
	}
	st->index = index;
	st->name = name;
	st->valid |= ZIP_STAT_INDEX | ZIP_STAT_NAME;
	return 0;
}

ZIP_EXTERN int zip_stat(zip_t * za, const char * fname, zip_flags_t flags, zip_stat_t * st) 
{
	int64 idx = zip_name_locate(za, fname, flags);
	return (idx < 0) ? -1 : zip_stat_index(za, (uint64)idx, flags, st);
}
//
// zip getter/setter
//
ZIP_EXTERN int zip_set_archive_comment(zip_t * za, const char * comment, uint16 len) 
{
	zip_string_t * cstr;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(len > 0 && comment == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(len > 0) {
		if((cstr = _zip_string_new((const uint8*)comment, len, ZIP_FL_ENC_GUESS, &za->error)) == NULL)
			return -1;
		if(_zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_CP437) {
			_zip_string_free(cstr);
			zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
			return -1;
		}
	}
	else
		cstr = NULL;
	_zip_string_free(za->comment_changes);
	za->comment_changes = NULL;
	if(((za->comment_orig && _zip_string_equal(za->comment_orig, cstr)) || (za->comment_orig == NULL && cstr == NULL))) {
		_zip_string_free(cstr);
		za->comment_changed = 0;
	}
	else {
		za->comment_changes = cstr;
		za->comment_changed = 1;
	}
	return 0;
}

ZIP_EXTERN const char * zip_get_archive_comment(zip_t * za, int * lenp, zip_flags_t flags) 
{
	zip_string_t * comment;
	uint32 len;
	const uint8 * str;
	if((flags & ZIP_FL_UNCHANGED) || (za->comment_changes == NULL))
		comment = za->comment_orig;
	else
		comment = za->comment_changes;
	if((str = _zip_string_get(comment, &len, flags, &za->error)) == NULL)
		return NULL;
	ASSIGN_PTR(lenp, (int)len);
	return (const char*)str;
}

ZIP_EXTERN int zip_set_archive_flag(zip_t * za, zip_flags_t flag, int value) 
{
	uint new_flags;
	if(value)
		new_flags = za->ch_flags | flag;
	else
		new_flags = za->ch_flags & ~flag;
	if(new_flags == za->ch_flags)
		return 0;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if((flag & ZIP_AFL_RDONLY) && value && (za->ch_flags & ZIP_AFL_RDONLY) == 0) {
		if(_zip_changed(za, NULL)) {
			zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
			return -1;
		}
	}
	za->ch_flags = new_flags;
	return 0;
}

ZIP_EXTERN int zip_get_archive_flag(zip_t * za, zip_flags_t flag, zip_flags_t flags) 
{
	uint fl = (flags & ZIP_FL_UNCHANGED) ? za->flags : za->ch_flags;
	return BIN(fl & flag);
}

ZIP_EXTERN int zip_set_default_password(zip_t * za, const char * passwd) 
{
	if(za == NULL)
		return -1;
	else {
		SAlloc::F(za->default_password);
		if(passwd) {
			if((za->default_password = strdup(passwd)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		else
			za->default_password = NULL;
		return 0;
	}
}

ZIP_EXTERN int zip_set_file_comment(zip_t * za, uint64 idx, const char * comment, int len) 
{
	if(len < 0 || len > ZIP_UINT16_MAX) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	return zip_file_set_comment(za, idx, comment, (uint16)len, 0);
}

ZIP_EXTERN const char * zip_get_file_comment(zip_t * za, uint64 idx, int * lenp, int flags) 
{
	uint32 len;
	const char * s;
	if((s = zip_file_get_comment(za, idx, &len, (zip_flags_t)flags)) != NULL) {
		ASSIGN_PTR(lenp, (int)len);
	}
	return s;
}

int _zip_set_name(zip_t * za, uint64 idx, const char * name, zip_flags_t flags) 
{
	zip_entry_t * e;
	zip_string_t * str;
	bool same_as_orig;
	int64 i;
	const uint8 * old_name, * new_name;
	zip_string_t * old_str;
	if(idx >= za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(name && name[0] != '\0') {
		/* TODO: check for string too long */
		if((str = _zip_string_new((const uint8*)name, (uint16)strlen(name), flags, &za->error)) == NULL)
			return -1;
		if((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS &&
		    _zip_guess_encoding(str, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			str->encoding = ZIP_ENCODING_UTF8_KNOWN;
	}
	else
		str = NULL;
	/* TODO: encoding flags needed for CP437? */
	if((i = _zip_name_locate(za, name, 0, NULL)) >= 0 && (uint64)i != idx) {
		_zip_string_free(str);
		zip_error_set(&za->error, SLERR_ZIP_EXISTS, 0);
		return -1;
	}
	/* no effective name change */
	if(i >= 0 && (uint64)i == idx) {
		_zip_string_free(str);
		return 0;
	}
	e = za->entry + idx;
	if(e->orig)
		same_as_orig = LOGIC(_zip_string_equal(e->orig->filename, str));
	else
		same_as_orig = false;
	if(!same_as_orig && e->changes == NULL) {
		if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			_zip_string_free(str);
			return -1;
		}
	}
	if((new_name = _zip_string_get(same_as_orig ? e->orig->filename : str, NULL, 0, &za->error)) == NULL) {
		_zip_string_free(str);
		return -1;
	}
	if(e->changes) {
		old_str = e->changes->filename;
	}
	else if(e->orig) {
		old_str = e->orig->filename;
	}
	else {
		old_str = NULL;
	}
	if(old_str) {
		if((old_name = _zip_string_get(old_str, NULL, 0, &za->error)) == NULL) {
			_zip_string_free(str);
			return -1;
		}
	}
	else {
		old_name = NULL;
	}
	if(_zip_hash_add(za->names, new_name, idx, 0, &za->error) == false) {
		_zip_string_free(str);
		return -1;
	}
	if(old_name) {
		_zip_hash_delete(za->names, old_name, NULL);
	}
	if(same_as_orig) {
		if(e->changes) {
			if(e->changes->changed & ZIP_DIRENT_FILENAME) {
				_zip_string_free(e->changes->filename);
				e->changes->changed &= ~ZIP_DIRENT_FILENAME;
				if(e->changes->changed == 0) {
					_zip_dirent_free(e->changes);
					e->changes = NULL;
				}
				else {
					/* TODO: what if not cloned? can that happen? */
					e->changes->filename = e->orig->filename;
				}
			}
		}
		_zip_string_free(str);
	}
	else {
		if(e->changes->changed & ZIP_DIRENT_FILENAME) {
			_zip_string_free(e->changes->filename);
		}
		e->changes->changed |= ZIP_DIRENT_FILENAME;
		e->changes->filename = str;
	}
	return 0;
}

ZIP_EXTERN const char * zip_get_name(zip_t * za, uint64 idx, zip_flags_t flags) 
{
	return _zip_get_name(za, idx, flags, &za->error);
}

const char * _zip_get_name(zip_t * za, uint64 idx, zip_flags_t flags, zip_error_t * error) 
{
	zip_dirent_t * de = _zip_get_dirent(za, idx, flags, error);
	return de ? reinterpret_cast<const char *>(_zip_string_get(de->filename, NULL, flags, error)) : 0;
}

ZIP_EXTERN int64 zip_get_num_entries(const zip_t * za, zip_flags_t flags) 
{
	uint64 n;
	if(za == NULL)
		return -1;
	else {
		if(flags & ZIP_FL_UNCHANGED) {
			n = za->nentry;
			while(n > 0 && za->entry[n - 1].orig == NULL)
				--n;
			return (int64)n;
		}
		return (int64)za->nentry;
	}
}

ZIP_EXTERN int zip_get_num_files(zip_t * za) 
{
	if(za == NULL)
		return -1;
	else if(za->nentry > INT_MAX) {
		zip_error_set(&za->error, SLERR_ZIP_OPNOTSUPP, 0);
		return -1;
	}
	else
		return (int)za->nentry;
}

zip_encryption_implementation _zip_get_encryption_implementation(uint16 em, int operation) 
{
	switch(em) {
		case ZIP_EM_TRAD_PKWARE:
		    return operation == ZIP_CODEC_DECODE ? zip_source_pkware_decode : zip_source_pkware_encode;
#if defined(HAVE_CRYPTO)
		case ZIP_EM_AES_128:
		case ZIP_EM_AES_192:
		case ZIP_EM_AES_256:
		    return operation == ZIP_CODEC_DECODE ? zip_source_winzip_aes_decode : zip_source_winzip_aes_encode;
#endif
		default:
		    return NULL;
	}
}

ZIP_EXTERN int zip_encryption_method_supported(uint16 method, int encode) 
{
	return (method == ZIP_EM_NONE) ? 1 : _zip_get_encryption_implementation(method, encode ? ZIP_CODEC_ENCODE : ZIP_CODEC_DECODE) != NULL;
}

ZIP_EXTERN int zip_set_file_compression(zip_t * za, uint64 idx, int32 method, uint32 flags) 
{
	zip_entry_t * e;
	int32 old_method;
	if(idx >= za->nentry || flags > 9) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(!zip_compression_method_supported(method, true)) {
		zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
		return -1;
	}
	e = za->entry + idx;
	old_method = (e->orig == NULL ? ZIP_CM_DEFAULT : e->orig->comp_method);
	/* TODO: do we want to recompress if level is set? Only if it's
	 * different than what bit flags tell us, but those are not
	 * defined for all compression methods, or not directly mappable
	 * to levels */
	if(method == old_method) {
		if(e->changes) {
			e->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
			e->changes->compression_level = 0;
			if(e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	}
	else {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		e->changes->comp_method = method;
		e->changes->compression_level = (uint16)flags;
		e->changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}
	return 0;
}
//
// zip extra field
// manipulate extra fields
//
zip_extra_field_t * _zip_ef_clone(const zip_extra_field_t * ef, zip_error_t * error) 
{
	zip_extra_field_t * head = 0;
	zip_extra_field_t * prev = 0;
	zip_extra_field_t * def;
	while(ef) {
		if((def = _zip_ef_new(ef->id, ef->size, ef->data, ef->flags)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			_zip_ef_free(head);
			return NULL;
		}
		SETIFZ(head, def);
		if(prev)
			prev->next = def;
		prev = def;
		ef = ef->next;
	}
	return head;
}

zip_extra_field_t * _zip_ef_delete_by_id(zip_extra_field_t * ef, uint16 id, uint16 id_idx, zip_flags_t flags) 
{
	int i = 0;
	zip_extra_field_t * head = ef;
	zip_extra_field_t * prev = NULL;
	for(; ef; ef = (prev ? prev->next : head)) {
		if((ef->flags & flags & ZIP_EF_BOTH) && ((ef->id == id) || (id == ZIP_EXTRA_FIELD_ALL))) {
			if(id_idx == ZIP_EXTRA_FIELD_ALL || i == id_idx) {
				ef->flags &= ~(flags & ZIP_EF_BOTH);
				if((ef->flags & ZIP_EF_BOTH) == 0) {
					if(prev)
						prev->next = ef->next;
					else
						head = ef->next;
					ef->next = NULL;
					_zip_ef_free(ef);
					if(id_idx == ZIP_EXTRA_FIELD_ALL)
						continue;
				}
			}
			i++;
			if(i > id_idx)
				break;
		}
		prev = ef;
	}
	return head;
}

void _zip_ef_free(zip_extra_field_t * ef) 
{
	while(ef) {
		zip_extra_field_t * ef2 = ef->next;
		SAlloc::F(ef->data);
		SAlloc::F(ef);
		ef = ef2;
	}
}

const uint8 * _zip_ef_get_by_id(const zip_extra_field_t * ef, uint16 * lenp, uint16 id,
    uint16 id_idx, zip_flags_t flags, zip_error_t * error) 
{
	static const uint8 empty[1] = {'\0'};
	int i = 0;
	for(; ef; ef = ef->next) {
		if(ef->id == id && (ef->flags & flags & ZIP_EF_BOTH)) {
			if(i < id_idx) {
				i++;
				continue;
			}
			if(lenp)
				*lenp = ef->size;
			if(ef->size > 0)
				return ef->data;
			else
				return empty;
		}
	}
	zip_error_set(error, SLERR_ZIP_NOENT, 0);
	return NULL;
}

zip_extra_field_t * _zip_ef_merge(zip_extra_field_t * to, zip_extra_field_t * from) 
{
	zip_extra_field_t * ef2, * tt, * tail;
	int duplicate;
	if(to == NULL)
		return from;
	for(tail = to; tail->next; tail = tail->next)
		;
	for(; from; from = ef2) {
		ef2 = from->next;
		duplicate = 0;
		for(tt = to; tt; tt = tt->next) {
			if(tt->id == from->id && tt->size == from->size && (tt->size == 0 || memcmp(tt->data, from->data, tt->size) == 0)) {
				tt->flags |= (from->flags & ZIP_EF_BOTH);
				duplicate = 1;
				break;
			}
		}
		from->next = NULL;
		if(duplicate)
			_zip_ef_free(from);
		else
			tail = tail->next = from;
	}
	return to;
}

zip_extra_field_t * _zip_ef_new(uint16 id, uint16 size, const uint8 * data, zip_flags_t flags) 
{
	zip_extra_field_t * ef = static_cast<zip_extra_field_t *>(SAlloc::M(sizeof(*ef)));
	if(ef) {
		ef->next = NULL;
		ef->flags = flags;
		ef->id = id;
		ef->size = size;
		if(size > 0) {
			if((ef->data = (uint8 *)_zip_memdup(data, size, NULL)) == NULL) {
				SAlloc::F(ef);
				return NULL;
			}
		}
		else
			ef->data = NULL;
	}
	return ef;
}

bool _zip_ef_parse(const uint8 * data, uint16 len, zip_flags_t flags, zip_extra_field_t ** ef_head_p, zip_error_t * error) 
{
	zip_buffer_t * buffer;
	zip_extra_field_t * ef, * ef2, * ef_head;
	if((buffer = _zip_buffer_new((uint8 *)data, len)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	ef_head = ef = NULL;
	while(_zip_buffer_ok(buffer) && _zip_buffer_left(buffer) >= 4) {
		uint16 fid, flen;
		uint8 * ef_data;
		fid = _zip_buffer_get_16(buffer);
		flen = _zip_buffer_get_16(buffer);
		ef_data = _zip_buffer_get(buffer, flen);
		if(ef_data == NULL) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_buffer_free(buffer);
			_zip_ef_free(ef_head);
			return false;
		}
		if((ef2 = _zip_ef_new(fid, flen, ef_data, flags)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			_zip_buffer_free(buffer);
			_zip_ef_free(ef_head);
			return false;
		}
		if(ef_head) {
			ef->next = ef2;
			ef = ef2;
		}
		else
			ef_head = ef = ef2;
	}
	if(!_zip_buffer_eof(buffer)) {
		// Android APK files align stored file data with padding in extra fields; ignore. 
		// see https://android.googlesource.com/platform/build/+/master/tools/zipalign/ZipAlign.cpp 
		size_t glen = static_cast<size_t>(_zip_buffer_left(buffer));
		uint8 * garbage = _zip_buffer_get(buffer, glen);
		if(glen >= 4 || garbage == NULL || memcmp(garbage, "\0\0\0", glen) != 0) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_buffer_free(buffer);
			_zip_ef_free(ef_head);
			return false;
		}
	}
	_zip_buffer_free(buffer);
	if(ef_head_p) {
		*ef_head_p = ef_head;
	}
	else {
		_zip_ef_free(ef_head);
	}
	return true;
}

zip_extra_field_t * _zip_ef_remove_internal(zip_extra_field_t * ef) 
{
	zip_extra_field_t * ef_head = ef;
	zip_extra_field_t * prev = NULL;
	while(ef) {
		if(ZIP_EF_IS_INTERNAL(ef->id)) {
			zip_extra_field_t * next = ef->next;
			if(ef_head == ef)
				ef_head = next;
			ef->next = NULL;
			_zip_ef_free(ef);
			if(prev)
				prev->next = next;
			ef = next;
		}
		else {
			prev = ef;
			ef = ef->next;
		}
	}
	return ef_head;
}

uint16 _zip_ef_size(const zip_extra_field_t * ef, zip_flags_t flags) 
{
	uint16 size = 0;
	for(; ef; ef = ef->next) {
		if(ef->flags & flags & ZIP_EF_BOTH)
			size = (uint16)(size + 4 + ef->size);
	}
	return size;
}

int _zip_ef_write(zip_t * za, const zip_extra_field_t * ef, zip_flags_t flags) 
{
	uint8 b[4];
	zip_buffer_t * buffer = _zip_buffer_new(b, sizeof(b));
	if(buffer == NULL) {
		return -1;
	}
	for(; ef; ef = ef->next) {
		if(ef->flags & flags & ZIP_EF_BOTH) {
			_zip_buffer_set_offset(buffer, 0);
			_zip_buffer_put_16(buffer, ef->id);
			_zip_buffer_put_16(buffer, ef->size);
			if(!_zip_buffer_ok(buffer)) {
				zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
				_zip_buffer_free(buffer);
				return -1;
			}
			if(_zip_write(za, b, 4) < 0) {
				_zip_buffer_free(buffer);
				return -1;
			}
			if(ef->size > 0) {
				if(_zip_write(za, ef->data, ef->size) < 0) {
					_zip_buffer_free(buffer);
					return -1;
				}
			}
		}
	}
	_zip_buffer_free(buffer);
	return 0;
}

int _zip_read_local_ef(zip_t * za, uint64 idx) 
{
	zip_entry_t * e;
	unsigned char b[4];
	zip_buffer_t * buffer;
	uint16 fname_len, ef_len;
	if(idx >= za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	e = za->entry + idx;
	if(e->orig == NULL || e->orig->local_extra_fields_read)
		return 0;
	if(e->orig->offset + 26 > ZIP_INT64_MAX) {
		zip_error_set(&za->error, SLERR_ZIP_SEEK, EFBIG);
		return -1;
	}
	if(zip_source_seek(za->src, (int64)(e->orig->offset + 26), SEEK_SET) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if((buffer = _zip_buffer_new_from_source(za->src, sizeof(b), b, &za->error)) == NULL) {
		return -1;
	}
	fname_len = _zip_buffer_get_16(buffer);
	ef_len = _zip_buffer_get_16(buffer);
	if(!_zip_buffer_eof(buffer)) {
		_zip_buffer_free(buffer);
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	_zip_buffer_free(buffer);
	if(ef_len > 0) {
		zip_extra_field_t * ef;
		uint8 * ef_raw;
		if(zip_source_seek(za->src, fname_len, SEEK_CUR) < 0) {
			zip_error_set(&za->error, SLERR_ZIP_SEEK, errno);
			return -1;
		}
		ef_raw = _zip_read_data(NULL, za->src, ef_len, 0, &za->error);
		if(ef_raw == NULL)
			return -1;
		if(!_zip_ef_parse(ef_raw, ef_len, ZIP_EF_LOCAL, &ef, &za->error)) {
			SAlloc::F(ef_raw);
			return -1;
		}
		SAlloc::F(ef_raw);
		if(ef) {
			ef = _zip_ef_remove_internal(ef);
			e->orig->extra_fields = _zip_ef_merge(e->orig->extra_fields, ef);
		}
	}
	e->orig->local_extra_fields_read = 1;
	if(e->changes && e->changes->local_extra_fields_read == 0) {
		e->changes->extra_fields = e->orig->extra_fields;
		e->changes->local_extra_fields_read = 1;
	}
	return 0;
}

ZIP_EXTERN int zip_file_extra_field_delete(zip_t * za, uint64 idx, uint16 ef_idx, zip_flags_t flags) 
{
	zip_dirent_t * de;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(((flags & ZIP_EF_BOTH) == ZIP_EF_BOTH) && (ef_idx != ZIP_EXTRA_FIELD_ALL)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(_zip_file_extra_field_prepare_for_change(za, idx) < 0)
		return -1;
	de = za->entry[idx].changes;
	de->extra_fields = _zip_ef_delete_by_id(de->extra_fields, ZIP_EXTRA_FIELD_ALL, ef_idx, flags);
	return 0;
}

ZIP_EXTERN int zip_file_extra_field_delete_by_id(zip_t * za, uint64 idx, uint16 ef_id, uint16 ef_idx, zip_flags_t flags) 
{
	zip_dirent_t * de;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(((flags & ZIP_EF_BOTH) == ZIP_EF_BOTH) && (ef_idx != ZIP_EXTRA_FIELD_ALL)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(_zip_file_extra_field_prepare_for_change(za, idx) < 0)
		return -1;
	de = za->entry[idx].changes;
	de->extra_fields = _zip_ef_delete_by_id(de->extra_fields, ef_id, ef_idx, flags);
	return 0;
}

ZIP_EXTERN const uint8 * zip_file_extra_field_get(zip_t * za, uint64 idx, uint16 ef_idx, uint16 * idp, uint16 * lenp, zip_flags_t flags) 
{
	static const uint8 empty[1] = {'\0'};
	zip_dirent_t * de;
	zip_extra_field_t * ef;
	int i;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((de = _zip_get_dirent(za, idx, flags, &za->error)) == NULL)
		return NULL;
	if(flags & ZIP_FL_LOCAL)
		if(_zip_read_local_ef(za, idx) < 0)
			return NULL;
	i = 0;
	for(ef = de->extra_fields; ef; ef = ef->next) {
		if(ef->flags & flags & ZIP_EF_BOTH) {
			if(i < ef_idx) {
				i++;
				continue;
			}
			ASSIGN_PTR(idp, ef->id);
			ASSIGN_PTR(lenp, ef->size);
			if(ef->size > 0)
				return ef->data;
			else
				return empty;
		}
	}
	zip_error_set(&za->error, SLERR_ZIP_NOENT, 0);
	return NULL;
}

ZIP_EXTERN const uint8 * zip_file_extra_field_get_by_id(zip_t * za, uint64 idx, uint16 ef_id, uint16 ef_idx,
    uint16 * lenp, zip_flags_t flags) 
{
	zip_dirent_t * de;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((de = _zip_get_dirent(za, idx, flags, &za->error)) == NULL)
		return NULL;
	if(flags & ZIP_FL_LOCAL)
		if(_zip_read_local_ef(za, idx) < 0)
			return NULL;
	return _zip_ef_get_by_id(de->extra_fields, lenp, ef_id, ef_idx, flags, &za->error);
}

ZIP_EXTERN int16 zip_file_extra_fields_count(zip_t * za, uint64 idx, zip_flags_t flags) 
{
	zip_dirent_t * de;
	zip_extra_field_t * ef;
	uint16 n;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if((de = _zip_get_dirent(za, idx, flags, &za->error)) == NULL)
		return -1;
	if(flags & ZIP_FL_LOCAL)
		if(_zip_read_local_ef(za, idx) < 0)
			return -1;
	n = 0;
	for(ef = de->extra_fields; ef; ef = ef->next)
		if(ef->flags & flags & ZIP_EF_BOTH)
			n++;
	return (int16)n;
}

ZIP_EXTERN int16 zip_file_extra_fields_count_by_id(zip_t * za, uint64 idx, uint16 ef_id, zip_flags_t flags) 
{
	zip_dirent_t * de;
	zip_extra_field_t * ef;
	uint16 n;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if((de = _zip_get_dirent(za, idx, flags, &za->error)) == NULL)
		return -1;
	if(flags & ZIP_FL_LOCAL)
		if(_zip_read_local_ef(za, idx) < 0)
			return -1;
	n = 0;
	for(ef = de->extra_fields; ef; ef = ef->next)
		if(ef->id == ef_id && (ef->flags & flags & ZIP_EF_BOTH))
			n++;
	return (int16)n;
}

ZIP_EXTERN int zip_file_extra_field_set(zip_t * za, uint64 idx, uint16 ef_id, uint16 ef_idx,
    const uint8 * data, uint16 len, zip_flags_t flags) 
{
	zip_dirent_t * de;
	uint16 ls, cs;
	zip_extra_field_t * ef, * ef_prev, * ef_new;
	int i, found, new_len;
	if((flags & ZIP_EF_BOTH) == 0) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(ZIP_EF_IS_INTERNAL(ef_id)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_file_extra_field_prepare_for_change(za, idx) < 0)
		return -1;
	de = za->entry[idx].changes;
	ef = de->extra_fields;
	ef_prev = NULL;
	i = 0;
	found = 0;
	for(; ef; ef = ef->next) {
		if(ef->id == ef_id && (ef->flags & flags & ZIP_EF_BOTH)) {
			if(i == ef_idx) {
				found = 1;
				break;
			}
			i++;
		}
		ef_prev = ef;
	}
	if(i < ef_idx && ef_idx != ZIP_EXTRA_FIELD_NEW) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(flags & ZIP_EF_LOCAL)
		ls = _zip_ef_size(de->extra_fields, ZIP_EF_LOCAL);
	else
		ls = 0;
	if(flags & ZIP_EF_CENTRAL)
		cs = _zip_ef_size(de->extra_fields, ZIP_EF_CENTRAL);
	else
		cs = 0;
	new_len = ls > cs ? ls : cs;
	if(found)
		new_len -= ef->size + 4;
	new_len += len + 4;
	if(new_len > ZIP_UINT16_MAX) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if((ef_new = _zip_ef_new(ef_id, len, data, flags)) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	if(found) {
		if((ef->flags & ZIP_EF_BOTH) == (flags & ZIP_EF_BOTH)) {
			ef_new->next = ef->next;
			ef->next = NULL;
			_zip_ef_free(ef);
			if(ef_prev)
				ef_prev->next = ef_new;
			else
				de->extra_fields = ef_new;
		}
		else {
			ef->flags &= ~(flags & ZIP_EF_BOTH);
			ef_new->next = ef->next;
			ef->next = ef_new;
		}
	}
	else if(ef_prev) {
		ef_new->next = ef_prev->next;
		ef_prev->next = ef_new;
	}
	else
		de->extra_fields = ef_new;
	return 0;
}

int _zip_file_extra_field_prepare_for_change(zip_t * za, uint64 idx) 
{
	zip_entry_t * e;
	if(idx >= za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	e = za->entry + idx;
	if(e->changes && (e->changes->changed & ZIP_DIRENT_EXTRA_FIELD))
		return 0;
	if(e->orig) {
		if(_zip_read_local_ef(za, idx) < 0)
			return -1;
	}
	if(e->changes == NULL) {
		if((e->changes = _zip_dirent_clone(e->orig)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			return -1;
		}
	}
	if(e->orig && e->orig->extra_fields) {
		if((e->changes->extra_fields = _zip_ef_clone(e->orig->extra_fields, &za->error)) == NULL)
			return -1;
	}
	e->changes->changed |= ZIP_DIRENT_EXTRA_FIELD;
	return 0;
}
//
//
//
//
// zip-dirent
// read directory entry (local or central), clean dirent
//
static zip_extra_field_t * _zip_ef_utf8(uint16 id, zip_string_t * str, zip_error_t * error) 
{
	const uint8 * raw;
	uint32 len;
	zip_buffer_t * buffer;
	zip_extra_field_t * ef;
	if((raw = _zip_string_get(str, &len, ZIP_FL_ENC_RAW, NULL)) == NULL) {
		return NULL; // error already set 
	}
	if(len + 5 > ZIP_UINT16_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0); /* TODO: better error code? */
		return NULL;
	}
	if((buffer = _zip_buffer_new(NULL, len + 5)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	_zip_buffer_put_8(buffer, 1);
	_zip_buffer_put_32(buffer, _zip_string_crc32(str));
	_zip_buffer_put(buffer, raw, len);
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		_zip_buffer_free(buffer);
		return NULL;
	}
	ef = _zip_ef_new(id, (uint16)(_zip_buffer_offset(buffer)), _zip_buffer_data(buffer), ZIP_EF_BOTH);
	_zip_buffer_free(buffer);
	return ef;
}

static zip_string_t * _zip_dirent_process_ef_utf_8(const zip_dirent_t * de, uint16 id, zip_string_t * str) 
{
	uint16 ef_len;
	uint32 ef_crc;
	zip_buffer_t * buffer;
	const uint8 * ef = _zip_ef_get_by_id(de->extra_fields, &ef_len, id, 0, ZIP_EF_BOTH, NULL);
	if(ef == NULL || ef_len < 5 || ef[0] != 1) {
		return str;
	}
	if((buffer = _zip_buffer_new((uint8 *)ef, ef_len)) == NULL) {
		return str;
	}
	_zip_buffer_get_8(buffer);
	ef_crc = _zip_buffer_get_32(buffer);
	if(_zip_string_crc32(str) == ef_crc) {
		uint16 len = (uint16)_zip_buffer_left(buffer);
		zip_string_t * ef_str = _zip_string_new(_zip_buffer_get(buffer, len), len, ZIP_FL_ENC_UTF_8, NULL);
		if(ef_str != NULL) {
			_zip_string_free(str);
			str = ef_str;
		}
	}
	_zip_buffer_free(buffer);
	return str;
}

static bool _zip_dirent_process_winzip_aes(zip_dirent_t * de, zip_error_t * error) 
{
	uint16 ef_len;
	zip_buffer_t * buffer;
	const uint8 * ef;
	bool crc_valid;
	uint16 enc_method;
	if(de->comp_method != ZIP_CM_WINZIP_AES) {
		return true;
	}
	ef = _zip_ef_get_by_id(de->extra_fields, &ef_len, ZIP_EF_WINZIP_AES, 0, ZIP_EF_BOTH, NULL);
	if(ef == NULL || ef_len < 7) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return false;
	}
	if((buffer = _zip_buffer_new((uint8 *)ef, ef_len)) == NULL) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return false;
	}
	/* version */
	crc_valid = true;
	switch(_zip_buffer_get_16(buffer)) {
		case 1:
		    break;
		case 2:
		    if(de->uncomp_size < 20 /* TODO: constant */) {
			    crc_valid = false;
		    }
		    break;
		default:
		    zip_error_set(error, SLERR_ZIP_ENCRNOTSUPP, 0);
		    _zip_buffer_free(buffer);
		    return false;
	}
	/* vendor */
	if(memcmp(_zip_buffer_get(buffer, 2), "AE", 2) != 0) {
		zip_error_set(error, SLERR_ZIP_ENCRNOTSUPP, 0);
		_zip_buffer_free(buffer);
		return false;
	}
	/* mode */
	switch(_zip_buffer_get_8(buffer)) {
		case 1: enc_method = ZIP_EM_AES_128; break;
		case 2: enc_method = ZIP_EM_AES_192; break;
		case 3: enc_method = ZIP_EM_AES_256; break;
		default:
		    zip_error_set(error, SLERR_ZIP_ENCRNOTSUPP, 0);
		    _zip_buffer_free(buffer);
		    return false;
	}
	if(ef_len != 7) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		_zip_buffer_free(buffer);
		return false;
	}
	de->crc_valid = crc_valid;
	de->encryption_method = enc_method;
	de->comp_method = _zip_buffer_get_16(buffer);
	_zip_buffer_free(buffer);
	return true;
}

void _zip_cdir_free(zip_cdir_t * cd) 
{
	if(cd) {
		for(uint64 i = 0; i < cd->nentry; i++)
			_zip_entry_finalize(cd->entry + i);
		SAlloc::F(cd->entry);
		_zip_string_free(cd->comment);
		SAlloc::F(cd);
	}
}

zip_cdir_t * _zip_cdir_new(uint64 nentry, zip_error_t * error) 
{
	zip_cdir_t * cd = (zip_cdir_t*)SAlloc::M(sizeof(*cd));
	if(!cd)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		cd->entry = NULL;
		cd->nentry = cd->nentry_alloc = 0;
		cd->size = cd->offset = 0;
		cd->comment = NULL;
		cd->is_zip64 = false;
		if(!_zip_cdir_grow(cd, nentry, error)) {
			_zip_cdir_free(cd);
			return NULL;
		}
	}
	return cd;
}

bool _zip_cdir_grow(zip_cdir_t * cd, uint64 additional_entries, zip_error_t * error) 
{
	uint64 i, new_alloc;
	zip_entry_t * new_entry;
	if(additional_entries == 0) {
		return true;
	}
	new_alloc = cd->nentry_alloc + additional_entries;
	if(new_alloc < additional_entries || new_alloc > SIZE_MAX / sizeof(*(cd->entry))) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	if((new_entry = (zip_entry_t*)SAlloc::R(cd->entry, sizeof(*(cd->entry)) * (size_t)new_alloc)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	cd->entry = new_entry;
	for(i = cd->nentry; i < new_alloc; i++) {
		_zip_entry_init(cd->entry + i);
	}
	cd->nentry = cd->nentry_alloc = new_alloc;
	return true;
}

int64 _zip_cdir_write(zip_t * za, const zip_filelist_t * filelist, uint64 survivors) 
{
	uint64 offset, size;
	zip_string_t * comment;
	uint8 buf[EOCDLEN + EOCD64LEN + EOCD64LOCLEN];
	zip_buffer_t * buffer;
	int64 off;
	uint64 i;
	bool is_zip64;
	int ret;
	if((off = zip_source_tell_write(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	offset = (uint64)off;
	is_zip64 = false;
	for(i = 0; i < survivors; i++) {
		zip_entry_t * entry = za->entry + filelist[i].idx;
		if((ret = _zip_dirent_write(za, entry->changes ? entry->changes : entry->orig, ZIP_FL_CENTRAL)) < 0)
			return -1;
		if(ret)
			is_zip64 = true;
	}
	if((off = zip_source_tell_write(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	size = (uint64)off - offset;
	if(offset > ZIP_UINT32_MAX || survivors > ZIP_UINT16_MAX)
		is_zip64 = true;
	if((buffer = _zip_buffer_new(buf, sizeof(buf))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	if(is_zip64) {
		_zip_buffer_put(buffer, EOCD64_MAGIC, 4);
		_zip_buffer_put_64(buffer, EOCD64LEN - 12);
		_zip_buffer_put_16(buffer, 45);
		_zip_buffer_put_16(buffer, 45);
		_zip_buffer_put_32(buffer, 0);
		_zip_buffer_put_32(buffer, 0);
		_zip_buffer_put_64(buffer, survivors);
		_zip_buffer_put_64(buffer, survivors);
		_zip_buffer_put_64(buffer, size);
		_zip_buffer_put_64(buffer, offset);
		_zip_buffer_put(buffer, EOCD64LOC_MAGIC, 4);
		_zip_buffer_put_32(buffer, 0);
		_zip_buffer_put_64(buffer, offset + size);
		_zip_buffer_put_32(buffer, 1);
	}
	_zip_buffer_put(buffer, EOCD_MAGIC, 4);
	_zip_buffer_put_32(buffer, 0);
	_zip_buffer_put_16(buffer, (uint16)(survivors >= ZIP_UINT16_MAX ? ZIP_UINT16_MAX : survivors));
	_zip_buffer_put_16(buffer, (uint16)(survivors >= ZIP_UINT16_MAX ? ZIP_UINT16_MAX : survivors));
	_zip_buffer_put_32(buffer, size >= ZIP_UINT32_MAX ? ZIP_UINT32_MAX : (uint32)size);
	_zip_buffer_put_32(buffer, offset >= ZIP_UINT32_MAX ? ZIP_UINT32_MAX : (uint32)offset);
	comment = za->comment_changed ? za->comment_changes : za->comment_orig;
	_zip_buffer_put_16(buffer, (uint16)(comment ? comment->length : 0));
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		_zip_buffer_free(buffer);
		return -1;
	}
	if(_zip_write(za, _zip_buffer_data(buffer), _zip_buffer_offset(buffer)) < 0) {
		_zip_buffer_free(buffer);
		return -1;
	}
	_zip_buffer_free(buffer);
	if(comment) {
		if(_zip_write(za, comment->raw, comment->length) < 0) {
			return -1;
		}
	}
	return (int64)size;
}

zip_dirent_t * _zip_dirent_clone(const zip_dirent_t * sde) 
{
	zip_dirent_t * tde;
	if((tde = (zip_dirent_t*)SAlloc::M(sizeof(*tde))) == NULL)
		return NULL;
	if(sde)
		memcpy(tde, sde, sizeof(*sde));
	else
		_zip_dirent_init(tde);
	tde->changed = 0;
	tde->cloned = 1;
	return tde;
}

void _zip_dirent_finalize(zip_dirent_t * zde) 
{
	if(!zde->cloned || zde->changed & ZIP_DIRENT_FILENAME) {
		_zip_string_free(zde->filename);
		zde->filename = NULL;
	}
	if(!zde->cloned || zde->changed & ZIP_DIRENT_EXTRA_FIELD) {
		_zip_ef_free(zde->extra_fields);
		zde->extra_fields = NULL;
	}
	if(!zde->cloned || zde->changed & ZIP_DIRENT_COMMENT) {
		_zip_string_free(zde->comment);
		zde->comment = NULL;
	}
	if(!zde->cloned || zde->changed & ZIP_DIRENT_PASSWORD) {
		if(zde->password) {
			_zip_crypto_clear(zde->password, strlen(zde->password));
		}
		SAlloc::F(zde->password);
		zde->password = NULL;
	}
}

void _zip_dirent_free(zip_dirent_t * zde) 
{
	if(zde) {
		_zip_dirent_finalize(zde);
		SAlloc::F(zde);
	}
}

void _zip_dirent_init(zip_dirent_t * de) 
{
	de->changed = 0;
	de->local_extra_fields_read = 0;
	de->cloned = 0;
	de->crc_valid = true;
	de->version_madeby = 63 | (ZIP_OPSYS_DEFAULT << 8);
	de->version_needed = 10; /* 1.0 */
	de->bitflags = 0;
	de->comp_method = ZIP_CM_DEFAULT;
	de->last_mod = 0;
	de->crc = 0;
	de->comp_size = 0;
	de->uncomp_size = 0;
	de->filename = NULL;
	de->extra_fields = NULL;
	de->comment = NULL;
	de->disk_number = 0;
	de->int_attrib = 0;
	de->ext_attrib = ZIP_EXT_ATTRIB_DEFAULT;
	de->offset = 0;
	de->compression_level = 0;
	de->encryption_method = ZIP_EM_NONE;
	de->password = NULL;
}

bool _zip_dirent_needs_zip64(const zip_dirent_t * de, zip_flags_t flags) 
{
	if(de->uncomp_size >= ZIP_UINT32_MAX || de->comp_size >= ZIP_UINT32_MAX ||
	    ((flags & ZIP_FL_CENTRAL) && de->offset >= ZIP_UINT32_MAX))
		return true;
	return false;
}

zip_dirent_t * _zip_dirent_new(void) 
{
	zip_dirent_t * de;
	if((de = (zip_dirent_t*)SAlloc::M(sizeof(*de))) == NULL)
		return NULL;
	_zip_dirent_init(de);
	return de;
}
//
// _zip_dirent_read(zde, fp, bufp, left, localp, error):
// Fills the zip directory entry zde.
// If buffer is non-NULL, data is taken from there; otherwise data is read from fp as needed.
// If local is true, it reads a local header instead of a central directory entry.
// Returns size of dirent read if successful. On error, error is filled in and -1 is returned.
//
int64 _zip_dirent_read(zip_dirent_t * zde, zip_source_t * src, zip_buffer_t * buffer, bool local, zip_error_t * error) 
{
	uint8 buf[CDENTRYSIZE];
	uint16 dostime, dosdate;
	uint32 size, variable_size;
	uint16 filename_len, comment_len, ef_len;
	bool from_buffer = (buffer != NULL);
	size = local ? LENTRYSIZE : CDENTRYSIZE;
	if(buffer) {
		if(_zip_buffer_left(buffer) < size) {
			zip_error_set(error, SLERR_ZIP_NOZIP, 0);
			return -1;
		}
	}
	else {
		if((buffer = _zip_buffer_new_from_source(src, size, buf, error)) == NULL) {
			return -1;
		}
	}
	if(memcmp(_zip_buffer_get(buffer, 4), (local ? LOCAL_MAGIC : CENTRAL_MAGIC), 4) != 0) {
		zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		if(!from_buffer) {
			_zip_buffer_free(buffer);
		}
		return -1;
	}
	/* convert buffercontents to zip_dirent */
	_zip_dirent_init(zde);
	if(!local)
		zde->version_madeby = _zip_buffer_get_16(buffer);
	else
		zde->version_madeby = 0;
	zde->version_needed = _zip_buffer_get_16(buffer);
	zde->bitflags = _zip_buffer_get_16(buffer);
	zde->comp_method = _zip_buffer_get_16(buffer);
	/* convert to time_t */
	dostime = _zip_buffer_get_16(buffer);
	dosdate = _zip_buffer_get_16(buffer);
	zde->last_mod = _zip_d2u_time(dostime, dosdate);
	zde->crc = _zip_buffer_get_32(buffer);
	zde->comp_size = _zip_buffer_get_32(buffer);
	zde->uncomp_size = _zip_buffer_get_32(buffer);
	filename_len = _zip_buffer_get_16(buffer);
	ef_len = _zip_buffer_get_16(buffer);
	if(local) {
		comment_len = 0;
		zde->disk_number = 0;
		zde->int_attrib = 0;
		zde->ext_attrib = 0;
		zde->offset = 0;
	}
	else {
		comment_len = _zip_buffer_get_16(buffer);
		zde->disk_number = _zip_buffer_get_16(buffer);
		zde->int_attrib = _zip_buffer_get_16(buffer);
		zde->ext_attrib = _zip_buffer_get_32(buffer);
		zde->offset = _zip_buffer_get_32(buffer);
	}
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		if(!from_buffer)
			_zip_buffer_free(buffer);
		return -1;
	}
	if(zde->bitflags & ZIP_GPBF_ENCRYPTED) {
		if(zde->bitflags & ZIP_GPBF_STRONG_ENCRYPTION) {
			// TODO 
			zde->encryption_method = ZIP_EM_UNKNOWN;
		}
		else {
			zde->encryption_method = ZIP_EM_TRAD_PKWARE;
		}
	}
	else {
		zde->encryption_method = ZIP_EM_NONE;
	}
	zde->filename = NULL;
	zde->extra_fields = NULL;
	zde->comment = NULL;
	variable_size = (uint32)filename_len + (uint32)ef_len + (uint32)comment_len;
	if(from_buffer) {
		if(_zip_buffer_left(buffer) < variable_size) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			return -1;
		}
	}
	else {
		_zip_buffer_free(buffer);
		if((buffer = _zip_buffer_new_from_source(src, variable_size, NULL, error)) == NULL) {
			return -1;
		}
	}
	if(filename_len) {
		zde->filename = _zip_read_string(buffer, src, filename_len, 1, error);
		if(!zde->filename) {
			if(zip_error_code_zip(error) == SLERR_ZIP_EOF) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
			}
			if(!from_buffer) {
				_zip_buffer_free(buffer);
			}
			return -1;
		}
		if(zde->bitflags & ZIP_GPBF_ENCODING_UTF_8) {
			if(_zip_guess_encoding(zde->filename, ZIP_ENCODING_UTF8_KNOWN) == ZIP_ENCODING_ERROR) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				if(!from_buffer) {
					_zip_buffer_free(buffer);
				}
				return -1;
			}
		}
	}
	if(ef_len) {
		uint8 * ef = _zip_read_data(buffer, src, ef_len, 0, error);
		if(!ef) {
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		if(!_zip_ef_parse(ef, ef_len, local ? ZIP_EF_LOCAL : ZIP_EF_CENTRAL, &zde->extra_fields, error)) {
			SAlloc::F(ef);
			if(!from_buffer) {
				_zip_buffer_free(buffer);
			}
			return -1;
		}
		SAlloc::F(ef);
		if(local)
			zde->local_extra_fields_read = 1;
	}
	if(comment_len) {
		zde->comment = _zip_read_string(buffer, src, comment_len, 0, error);
		if(!zde->comment) {
			if(!from_buffer) {
				_zip_buffer_free(buffer);
			}
			return -1;
		}
		if(zde->bitflags & ZIP_GPBF_ENCODING_UTF_8) {
			if(_zip_guess_encoding(zde->comment, ZIP_ENCODING_UTF8_KNOWN) == ZIP_ENCODING_ERROR) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				if(!from_buffer) {
					_zip_buffer_free(buffer);
				}
				return -1;
			}
		}
	}
	zde->filename = _zip_dirent_process_ef_utf_8(zde, ZIP_EF_UTF_8_NAME, zde->filename);
	zde->comment = _zip_dirent_process_ef_utf_8(zde, ZIP_EF_UTF_8_COMMENT, zde->comment);
	/* Zip64 */
	if(zde->uncomp_size == ZIP_UINT32_MAX || zde->comp_size == ZIP_UINT32_MAX || zde->offset == ZIP_UINT32_MAX) {
		uint16 got_len;
		zip_buffer_t * ef_buffer;
		const uint8 * ef = _zip_ef_get_by_id(zde->extra_fields, &got_len, ZIP_EF_ZIP64, 0, local ? ZIP_EF_LOCAL : ZIP_EF_CENTRAL, error);
		// TODO: if got_len == 0 && !ZIP64_EOCD: no error, 0xffffffff is valid value 
		if(ef == NULL) {
			if(!from_buffer) {
				_zip_buffer_free(buffer);
			}
			return -1;
		}
		if((ef_buffer = _zip_buffer_new((uint8 *)ef, got_len)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			if(!from_buffer) {
				_zip_buffer_free(buffer);
			}
			return -1;
		}
		if(zde->uncomp_size == ZIP_UINT32_MAX) {
			zde->uncomp_size = _zip_buffer_get_64(ef_buffer);
		}
		else if(local) {
			/* From appnote.txt: This entry in the Local header MUST
			   include BOTH original and compressed file size fields. */
			(void)_zip_buffer_skip(ef_buffer, 8); /* error is caught by _zip_buffer_eof() call */
		}
		if(zde->comp_size == ZIP_UINT32_MAX) {
			zde->comp_size = _zip_buffer_get_64(ef_buffer);
		}
		if(!local) {
			if(zde->offset == ZIP_UINT32_MAX) {
				zde->offset = _zip_buffer_get_64(ef_buffer);
			}
			if(zde->disk_number == ZIP_UINT16_MAX) {
				zde->disk_number = _zip_buffer_get_32(ef_buffer);
			}
		}
		if(!_zip_buffer_eof(ef_buffer)) {
			/* accept additional fields if values match */
			bool ok = true;
			switch(got_len) {
				case 28:
				    _zip_buffer_set_offset(ef_buffer, 24);
				    if(zde->disk_number != _zip_buffer_get_32(ef_buffer)) {
					    ok = false;
				    }
				/* fallthrough */
				case 24:
				    _zip_buffer_set_offset(ef_buffer, 0);
				    if((zde->uncomp_size != _zip_buffer_get_64(ef_buffer)) || (zde->comp_size != _zip_buffer_get_64(ef_buffer)) || (zde->offset != _zip_buffer_get_64(ef_buffer))) {
					    ok = false;
				    }
				    break;

				default:
				    ok = false;
			}
			if(!ok) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				_zip_buffer_free(ef_buffer);
				if(!from_buffer) {
					_zip_buffer_free(buffer);
				}
				return -1;
			}
		}
		_zip_buffer_free(ef_buffer);
	}
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		if(!from_buffer)
			_zip_buffer_free(buffer);
		return -1;
	}
	if(!from_buffer) {
		_zip_buffer_free(buffer);
	}
	// zip_source_seek / zip_source_tell don't support values > ZIP_INT64_MAX 
	if(zde->offset > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return -1;
	}
	if(!_zip_dirent_process_winzip_aes(zde, error)) {
		return -1;
	}
	zde->extra_fields = _zip_ef_remove_internal(zde->extra_fields);
	return (int64)size + (int64)variable_size;
}

int32 _zip_dirent_size(zip_source_t * src, uint16 flags, zip_error_t * error) 
{
	bool local = (flags & ZIP_EF_LOCAL) != 0;
	int i;
	uint8 b[6];
	zip_buffer_t * buffer;
	int32 size = local ? LENTRYSIZE : CDENTRYSIZE;
	if(zip_source_seek(src, local ? 26 : 28, SEEK_CUR) < 0) {
		_zip_error_set_from_source(error, src);
		return -1;
	}
	if((buffer = _zip_buffer_new_from_source(src, local ? 4 : 6, b, error)) == NULL) {
		return -1;
	}
	for(i = 0; i < (local ? 2 : 3); i++) {
		size += _zip_buffer_get_16(buffer);
	}
	if(!_zip_buffer_eof(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		_zip_buffer_free(buffer);
		return -1;
	}
	_zip_buffer_free(buffer);
	return size;
}
// 
// _zip_dirent_write
// Writes zip directory entry.
// If flags & ZIP_EF_LOCAL, it writes a local header instead of a central
// directory entry.  If flags & ZIP_EF_FORCE_ZIP64, a ZIP64 extra field is written, even if not needed.
// Returns 0 if successful, 1 if successful and wrote ZIP64 extra field. On error, error is filled in and -1 is returned.
// 
int _zip_dirent_write(zip_t * za, zip_dirent_t * de, zip_flags_t flags) 
{
	uint16 dostime, dosdate;
	zip_encoding_type_t com_enc, name_enc;
	zip_extra_field_t * ef;
	zip_extra_field_t * ef64;
	uint32 ef_total_size;
	bool is_zip64;
	bool is_really_zip64;
	bool is_winzip_aes;
	uint8 buf[CDENTRYSIZE];
	zip_buffer_t * buffer;
	ef = NULL;
	name_enc = _zip_guess_encoding(de->filename, ZIP_ENCODING_UNKNOWN);
	com_enc = _zip_guess_encoding(de->comment, ZIP_ENCODING_UNKNOWN);
	if((name_enc == ZIP_ENCODING_UTF8_KNOWN && com_enc == ZIP_ENCODING_ASCII) ||
	    (name_enc == ZIP_ENCODING_ASCII && com_enc == ZIP_ENCODING_UTF8_KNOWN) ||
	    (name_enc == ZIP_ENCODING_UTF8_KNOWN && com_enc == ZIP_ENCODING_UTF8_KNOWN))
		de->bitflags |= ZIP_GPBF_ENCODING_UTF_8;
	else {
		de->bitflags &= (uint16) ~ZIP_GPBF_ENCODING_UTF_8;
		if(name_enc == ZIP_ENCODING_UTF8_KNOWN) {
			ef = _zip_ef_utf8(ZIP_EF_UTF_8_NAME, de->filename, &za->error);
			if(ef == NULL)
				return -1;
		}
		if((flags & ZIP_FL_LOCAL) == 0 && com_enc == ZIP_ENCODING_UTF8_KNOWN) {
			zip_extra_field_t * ef2 = _zip_ef_utf8(ZIP_EF_UTF_8_COMMENT, de->comment, &za->error);
			if(ef2 == NULL) {
				_zip_ef_free(ef);
				return -1;
			}
			ef2->next = ef;
			ef = ef2;
		}
	}
	if(de->encryption_method == ZIP_EM_NONE) {
		de->bitflags &= (uint16) ~ZIP_GPBF_ENCRYPTED;
	}
	else {
		de->bitflags |= (uint16)ZIP_GPBF_ENCRYPTED;
	}
	is_really_zip64 = _zip_dirent_needs_zip64(de, flags);
	is_zip64 = (flags & (ZIP_FL_LOCAL | ZIP_FL_FORCE_ZIP64)) == (ZIP_FL_LOCAL | ZIP_FL_FORCE_ZIP64) || is_really_zip64;
	is_winzip_aes = de->encryption_method == ZIP_EM_AES_128 || de->encryption_method == ZIP_EM_AES_192 ||
	    de->encryption_method == ZIP_EM_AES_256;
	if(is_zip64) {
		uint8 ef_zip64[EFZIP64SIZE];
		zip_buffer_t * ef_buffer = _zip_buffer_new(ef_zip64, sizeof(ef_zip64));
		if(ef_buffer == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			_zip_ef_free(ef);
			return -1;
		}
		if(flags & ZIP_FL_LOCAL) {
			if((flags & ZIP_FL_FORCE_ZIP64) || de->comp_size > ZIP_UINT32_MAX || de->uncomp_size > ZIP_UINT32_MAX) {
				_zip_buffer_put_64(ef_buffer, de->uncomp_size);
				_zip_buffer_put_64(ef_buffer, de->comp_size);
			}
		}
		else {
			if((flags & ZIP_FL_FORCE_ZIP64) || de->comp_size > ZIP_UINT32_MAX || de->uncomp_size > ZIP_UINT32_MAX ||
			    de->offset > ZIP_UINT32_MAX) {
				if(de->uncomp_size >= ZIP_UINT32_MAX) {
					_zip_buffer_put_64(ef_buffer, de->uncomp_size);
				}
				if(de->comp_size >= ZIP_UINT32_MAX) {
					_zip_buffer_put_64(ef_buffer, de->comp_size);
				}
				if(de->offset >= ZIP_UINT32_MAX) {
					_zip_buffer_put_64(ef_buffer, de->offset);
				}
			}
		}
		if(!_zip_buffer_ok(ef_buffer)) {
			zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			_zip_buffer_free(ef_buffer);
			_zip_ef_free(ef);
			return -1;
		}
		ef64 = _zip_ef_new(ZIP_EF_ZIP64, (uint16)(_zip_buffer_offset(ef_buffer)), ef_zip64, ZIP_EF_BOTH);
		_zip_buffer_free(ef_buffer);
		ef64->next = ef;
		ef = ef64;
	}
	if(is_winzip_aes) {
		uint8 data[EF_WINZIP_AES_SIZE];
		zip_buffer_t * ef_buffer = _zip_buffer_new(data, sizeof(data));
		zip_extra_field_t * ef_winzip;
		if(ef_buffer == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			_zip_ef_free(ef);
			return -1;
		}
		_zip_buffer_put_16(ef_buffer, 2);
		_zip_buffer_put(ef_buffer, "AE", 2);
		_zip_buffer_put_8(ef_buffer, (uint8)(de->encryption_method & 0xff));
		_zip_buffer_put_16(ef_buffer, (uint16)de->comp_method);
		if(!_zip_buffer_ok(ef_buffer)) {
			zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			_zip_buffer_free(ef_buffer);
			_zip_ef_free(ef);
			return -1;
		}
		ef_winzip = _zip_ef_new(ZIP_EF_WINZIP_AES, EF_WINZIP_AES_SIZE, data, ZIP_EF_BOTH);
		_zip_buffer_free(ef_buffer);
		ef_winzip->next = ef;
		ef = ef_winzip;
	}
	if((buffer = _zip_buffer_new(buf, sizeof(buf))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		_zip_ef_free(ef);
		return -1;
	}
	_zip_buffer_put(buffer, (flags & ZIP_FL_LOCAL) ? LOCAL_MAGIC : CENTRAL_MAGIC, 4);
	if((flags & ZIP_FL_LOCAL) == 0) {
		_zip_buffer_put_16(buffer, de->version_madeby);
	}
	_zip_buffer_put_16(buffer, MAX(is_really_zip64 ? 45 : 0, de->version_needed));
	_zip_buffer_put_16(buffer, de->bitflags);
	if(is_winzip_aes) {
		_zip_buffer_put_16(buffer, ZIP_CM_WINZIP_AES);
	}
	else {
		_zip_buffer_put_16(buffer, (uint16)de->comp_method);
	}
	_zip_u2d_time(de->last_mod, &dostime, &dosdate);
	_zip_buffer_put_16(buffer, dostime);
	_zip_buffer_put_16(buffer, dosdate);
	if(is_winzip_aes && de->uncomp_size < 20) {
		_zip_buffer_put_32(buffer, 0);
	}
	else {
		_zip_buffer_put_32(buffer, de->crc);
	}
	if(((flags & ZIP_FL_LOCAL) == ZIP_FL_LOCAL) && ((de->comp_size >= ZIP_UINT32_MAX) || (de->uncomp_size >= ZIP_UINT32_MAX))) {
		/* In local headers, if a ZIP64 EF is written, it MUST contain
		 * both compressed and uncompressed sizes (even if one of the
		 * two is smaller than 0xFFFFFFFF); on the other hand, those
		 * may only appear when the corresponding standard entry is
		 * 0xFFFFFFFF.  (appnote.txt 4.5.3) */
		_zip_buffer_put_32(buffer, ZIP_UINT32_MAX);
		_zip_buffer_put_32(buffer, ZIP_UINT32_MAX);
	}
	else {
		if(de->comp_size < ZIP_UINT32_MAX) {
			_zip_buffer_put_32(buffer, (uint32)de->comp_size);
		}
		else {
			_zip_buffer_put_32(buffer, ZIP_UINT32_MAX);
		}
		if(de->uncomp_size < ZIP_UINT32_MAX) {
			_zip_buffer_put_32(buffer, (uint32)de->uncomp_size);
		}
		else {
			_zip_buffer_put_32(buffer, ZIP_UINT32_MAX);
		}
	}
	_zip_buffer_put_16(buffer, _zip_string_length(de->filename));
	/* TODO: check for overflow */
	ef_total_size = (uint32)_zip_ef_size(de->extra_fields, flags) + (uint32)_zip_ef_size(ef, ZIP_EF_BOTH);
	_zip_buffer_put_16(buffer, (uint16)ef_total_size);
	if((flags & ZIP_FL_LOCAL) == 0) {
		_zip_buffer_put_16(buffer, _zip_string_length(de->comment));
		_zip_buffer_put_16(buffer, (uint16)de->disk_number);
		_zip_buffer_put_16(buffer, de->int_attrib);
		_zip_buffer_put_32(buffer, de->ext_attrib);
		if(de->offset < ZIP_UINT32_MAX)
			_zip_buffer_put_32(buffer, (uint32)de->offset);
		else
			_zip_buffer_put_32(buffer, ZIP_UINT32_MAX);
	}
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		_zip_buffer_free(buffer);
		_zip_ef_free(ef);
		return -1;
	}
	if(_zip_write(za, buf, _zip_buffer_offset(buffer)) < 0) {
		_zip_buffer_free(buffer);
		_zip_ef_free(ef);
		return -1;
	}
	_zip_buffer_free(buffer);
	if(de->filename) {
		if(_zip_string_write(za, de->filename) < 0) {
			_zip_ef_free(ef);
			return -1;
		}
	}
	if(ef) {
		if(_zip_ef_write(za, ef, ZIP_EF_BOTH) < 0) {
			_zip_ef_free(ef);
			return -1;
		}
	}
	_zip_ef_free(ef);
	if(de->extra_fields) {
		if(_zip_ef_write(za, de->extra_fields, flags) < 0) {
			return -1;
		}
	}
	if((flags & ZIP_FL_LOCAL) == 0) {
		if(de->comment) {
			if(_zip_string_write(za, de->comment) < 0) {
				return -1;
			}
		}
	}
	return is_zip64;
}

time_t _zip_d2u_time(uint16 dtime, uint16 ddate) 
{
	struct tm tm;
	memzero(&tm, sizeof(tm));
	// let mktime decide if DST is in effect 
	tm.tm_isdst = -1;
	tm.tm_year = ((ddate >> 9) & 127) + 1980 - 1900;
	tm.tm_mon = ((ddate >> 5) & 15) - 1;
	tm.tm_mday = ddate & 31;
	tm.tm_hour = (dtime >> 11) & 31;
	tm.tm_min = (dtime >> 5) & 63;
	tm.tm_sec = (dtime << 1) & 62;
	return mktime(&tm);
}

zip_dirent_t * _zip_get_dirent(zip_t * za, uint64 idx, zip_flags_t flags, zip_error_t * error) 
{
	SETIFZ(error, &za->error);
	if(idx >= za->nentry) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((flags & ZIP_FL_UNCHANGED) || za->entry[idx].changes == NULL) {
		if(za->entry[idx].orig == NULL) {
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
			return NULL;
		}
		if(za->entry[idx].deleted && (flags & ZIP_FL_UNCHANGED) == 0) {
			zip_error_set(error, SLERR_ZIP_DELETED, 0);
			return NULL;
		}
		return za->entry[idx].orig;
	}
	else
		return za->entry[idx].changes;
}

void _zip_u2d_time(time_t intime, uint16 * dtime, uint16 * ddate) 
{
	struct tm * tpm;
#ifdef HAVE_LOCALTIME_R
	struct tm tm;
	tpm = localtime_r(&intime, &tm);
#else
	tpm = localtime(&intime);
#endif
	if(!tpm) {
		// if localtime() fails, return an arbitrary date (1980-01-01 00:00:00) 
		*ddate = (1 << 5) + 1;
		*dtime = 0;
	}
	else {
		SETMAX(tpm->tm_year, 80);
		*ddate = (uint16)(((tpm->tm_year + 1900 - 1980) << 9) + ((tpm->tm_mon + 1) << 5) + tpm->tm_mday);
		*dtime = (uint16)(((tpm->tm_hour) << 11) + ((tpm->tm_min) << 5) + ((tpm->tm_sec) >> 1));
	}
}

void _zip_dirent_apply_attributes(zip_dirent_t * de, zip_file_attributes_t * attributes, bool force_zip64, uint32 changed) 
{
	uint16 length;
	if(attributes->valid & ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS) {
		uint16 mask = attributes->general_purpose_bit_mask & ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS_ALLOWED_MASK;
		de->bitflags = (de->bitflags & ~mask) | (attributes->general_purpose_bit_flags & mask);
	}
	if(attributes->valid & ZIP_FILE_ATTRIBUTES_ASCII) {
		de->int_attrib = (de->int_attrib & ~0x1) | (attributes->ascii ? 1 : 0);
	}
	// manually set attributes are preferred over attributes provided by source 
	if((changed & ZIP_DIRENT_ATTRIBUTES) == 0 && (attributes->valid & ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES)) {
		de->ext_attrib = attributes->external_file_attributes;
	}
	if(de->comp_method == ZIP_CM_LZMA) {
		de->version_needed = 63;
	}
	else if(de->encryption_method == ZIP_EM_AES_128 || de->encryption_method == ZIP_EM_AES_192 ||
	    de->encryption_method == ZIP_EM_AES_256) {
		de->version_needed = 51;
	}
	else if(de->comp_method == ZIP_CM_BZIP2) {
		de->version_needed = 46;
	}
	else if(force_zip64 || _zip_dirent_needs_zip64(de, 0)) {
		de->version_needed = 45;
	}
	else if(de->comp_method == ZIP_CM_DEFLATE || de->encryption_method == ZIP_EM_TRAD_PKWARE) {
		de->version_needed = 20;
	}
	else if((length = _zip_string_length(de->filename)) > 0 && de->filename->raw[length - 1] == '/') {
		de->version_needed = 20;
	}
	else {
		de->version_needed = 10;
	}
	if(attributes->valid & ZIP_FILE_ATTRIBUTES_VERSION_NEEDED) {
		de->version_needed = MAX(de->version_needed, attributes->version_needed);
	}
	de->version_madeby = 63 | (de->version_madeby & 0xff00);
	if((changed & ZIP_DIRENT_ATTRIBUTES) == 0 && (attributes->valid & ZIP_FILE_ATTRIBUTES_HOST_SYSTEM)) {
		de->version_madeby = (de->version_madeby & 0xff) | (uint16)(attributes->host_system << 8);
	}
}
// 
// creates a new zipfile struct, and sets the contents to zero; 
// returns the new struct
// 
zip_t * _zip_new(zip_error_t * error) 
{
	zip_t * za = static_cast<zip_t *>(SAlloc::M(sizeof(struct zip)));
	if(!za) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	}
	else {
		za->names = _zip_hash_new(error);
		if(!za->names) {
			ZFREE(za);
		}
		else {
			za->src = NULL;
			za->open_flags = 0;
			zip_error_init(&za->error);
			za->flags = za->ch_flags = 0;
			za->default_password = NULL;
			za->comment_orig = za->comment_changes = NULL;
			za->comment_changed = 0;
			za->nentry = za->nentry_alloc = 0;
			za->entry = NULL;
			za->nopen_source = za->nopen_source_alloc = 0;
			za->open_source = NULL;
			za->progress = NULL;
		}
	}
	return za;
}

typedef enum { EXISTS_ERROR = -1, EXISTS_NOT = 0, EXISTS_OK } exists_t;
static zip_t * _zip_allocate_new(zip_source_t * src, uint flags, zip_error_t * error);
static int64 _zip_checkcons(zip_t * za, zip_cdir_t * cdir, zip_error_t * error);
static zip_cdir_t * _zip_find_central_dir(zip_t * za, uint64 len);
static exists_t _zip_file_exists(zip_source_t * src, zip_error_t * error);
static int _zip_headercomp(const zip_dirent_t *, const zip_dirent_t *);
static unsigned char * _zip_memmem(const unsigned char *, size_t, const unsigned char *, size_t);
static zip_cdir_t * _zip_read_cdir(zip_t * za, zip_buffer_t * buffer, uint64 buf_offset, zip_error_t * error);
static zip_cdir_t * _zip_read_eocd(zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error);
static zip_cdir_t * _zip_read_eocd64(zip_source_t * src, zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error);

ZIP_EXTERN zip_t * zip_open(const char * fn, int _flags, int * zep) 
{
	zip_t * za = 0;
	struct zip_error error;
	zip_error_init(&error);
	zip_source_t * src = zip_source_file_create(fn, 0, -1, &error);
	if(!src) {
		_zip_set_open_error(zep, &error, 0);
		zip_error_fini(&error);
	}
	else {
		za = zip_open_from_source(src, _flags, &error);
		if(!za) {
			zip_source_free(src);
			_zip_set_open_error(zep, &error, 0);
			zip_error_fini(&error);
		}
		else
			zip_error_fini(&error);
	}
	return za;
}

ZIP_EXTERN zip_t * zip_open_from_source(zip_source_t * src, int _flags, zip_error_t * error) 
{
	static int64 needed_support_read = -1;
	static int64 needed_support_write = -1;
	uint flags;
	int64 supported;
	exists_t exists;

	if(_flags < 0 || src == NULL) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	flags = (uint)_flags;

	supported = zip_source_supports(src);
	if(needed_support_read == -1) {
		needed_support_read = zip_source_make_command_bitmap(ZIP_SOURCE_OPEN,
			ZIP_SOURCE_READ,
			ZIP_SOURCE_CLOSE,
			ZIP_SOURCE_SEEK,
			ZIP_SOURCE_TELL,
			ZIP_SOURCE_STAT,
			-1);
		needed_support_write = zip_source_make_command_bitmap(ZIP_SOURCE_BEGIN_WRITE,
			ZIP_SOURCE_COMMIT_WRITE,
			ZIP_SOURCE_ROLLBACK_WRITE,
			ZIP_SOURCE_SEEK_WRITE,
			ZIP_SOURCE_TELL_WRITE,
			ZIP_SOURCE_REMOVE,
			-1);
	}
	if((supported & needed_support_read) != needed_support_read) {
		zip_error_set(error, SLERR_ZIP_OPNOTSUPP, 0);
		return NULL;
	}
	if((supported & needed_support_write) != needed_support_write) {
		flags |= ZIP_RDONLY;
	}
	if((flags & (ZIP_RDONLY | ZIP_TRUNCATE)) == (ZIP_RDONLY | ZIP_TRUNCATE)) {
		zip_error_set(error, SLERR_ZIP_RDONLY, 0);
		return NULL;
	}
	exists = _zip_file_exists(src, error);
	switch(exists) {
		case EXISTS_ERROR:
		    return NULL;
		case EXISTS_NOT:
		    if((flags & ZIP_CREATE) == 0) {
			    zip_error_set(error, SLERR_ZIP_NOENT, 0);
			    return NULL;
		    }
		    return _zip_allocate_new(src, flags, error);

		default: {
		    zip_t * za;
		    if(flags & ZIP_EXCL) {
			    zip_error_set(error, SLERR_ZIP_EXISTS, 0);
			    return NULL;
		    }
		    if(zip_source_open(src) < 0) {
			    _zip_error_set_from_source(error, src);
			    return NULL;
		    }

		    if(flags & ZIP_TRUNCATE) {
			    za = _zip_allocate_new(src, flags, error);
		    }
		    else {
			    /* ZIP_CREATE gets ignored if file exists and not ZIP_EXCL, just like open() */
			    za = _zip_open(src, flags, error);
		    }
		    if(za == NULL) {
			    zip_source_close(src);
			    return NULL;
		    }
		    return za;
	    }
	}
}

zip_t * _zip_open(zip_source_t * src, uint flags, zip_error_t * error) 
{
	zip_t * za;
	zip_cdir_t * cdir;
	struct zip_stat st;
	uint64 len, idx;
	zip_stat_init(&st);
	if(zip_source_stat(src, &st) < 0) {
		_zip_error_set_from_source(error, src);
		return NULL;
	}
	if((st.valid & ZIP_STAT_SIZE) == 0) {
		zip_error_set(error, SLERR_ZIP_SEEK, EOPNOTSUPP);
		return NULL;
	}
	len = st.size;

	if((za = _zip_allocate_new(src, flags, error)) == NULL) {
		return NULL;
	}
	/* treat empty files as empty archives */
	if(len == 0 && zip_source_accept_empty(src)) {
		return za;
	}
	if((cdir = _zip_find_central_dir(za, len)) == NULL) {
		_zip_error_copy(error, &za->error);
		/* keep src so discard does not get rid of it */
		zip_source_keep(src);
		zip_discard(za);
		return NULL;
	}
	za->entry = cdir->entry;
	za->nentry = cdir->nentry;
	za->nentry_alloc = cdir->nentry_alloc;
	za->comment_orig = cdir->comment;
	SAlloc::F(cdir);
	_zip_hash_reserve_capacity(za->names, za->nentry, &za->error);
	for(idx = 0; idx < za->nentry; idx++) {
		const uint8 * name = _zip_string_get(za->entry[idx].orig->filename, NULL, 0, error);
		if(name == NULL) {
			/* keep src so discard does not get rid of it */
			zip_source_keep(src);
			zip_discard(za);
			return NULL;
		}
		if(_zip_hash_add(za->names, name, idx, ZIP_FL_UNCHANGED, &za->error) == false) {
			if(za->error.zip_err != SLERR_ZIP_EXISTS || (flags & ZIP_CHECKCONS)) {
				_zip_error_copy(error, &za->error);
				/* keep src so discard does not get rid of it */
				zip_source_keep(src);
				zip_discard(za);
				return NULL;
			}
		}
	}
	za->ch_flags = za->flags;
	return za;
}

void _zip_set_open_error(int * zep, const zip_error_t * err, int ze) 
{
	if(err) {
		ze = zip_error_code_zip(err);
		if(zip_error_system_type(err) == ZIP_ET_SYS) {
			errno = zip_error_code_system(err);
		}
	}
	if(zep)
		*zep = ze;
}
// 
// _zip_readcdir:
// tries to find a valid end-of-central-directory at the beginning of
// buf, and then the corresponding central directory entries.
// Returns a struct zip_cdir which contains the central directory
// entries, or NULL if unsuccessful. 
// 
static zip_cdir_t * _zip_read_cdir(zip_t * za, zip_buffer_t * buffer, uint64 buf_offset, zip_error_t * error) 
{
	zip_cdir_t * cd;
	uint16 comment_len;
	uint64 i, left;
	uint64 eocd_offset = _zip_buffer_offset(buffer);
	zip_buffer_t * cd_buffer;
	if(_zip_buffer_left(buffer) < EOCDLEN) {
		/* not enough bytes left for comment */
		zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		return NULL;
	}
	/* check for end-of-central-dir magic */
	if(memcmp(_zip_buffer_get(buffer, 4), EOCD_MAGIC, 4) != 0) {
		zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		return NULL;
	}
	if(eocd_offset >= EOCD64LOCLEN && memcmp(_zip_buffer_data(buffer) + eocd_offset - EOCD64LOCLEN, EOCD64LOC_MAGIC, 4) == 0) {
		_zip_buffer_set_offset(buffer, eocd_offset - EOCD64LOCLEN);
		cd = _zip_read_eocd64(za->src, buffer, buf_offset, za->flags, error);
	}
	else {
		_zip_buffer_set_offset(buffer, eocd_offset);
		cd = _zip_read_eocd(buffer, buf_offset, za->flags, error);
	}
	if(cd == NULL)
		return NULL;
	_zip_buffer_set_offset(buffer, eocd_offset + 20);
	comment_len = _zip_buffer_get_16(buffer);
	if(cd->offset + cd->size > buf_offset + eocd_offset) {
		/* cdir spans past EOCD record */
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		_zip_cdir_free(cd);
		return NULL;
	}
	if(comment_len || (za->open_flags & ZIP_CHECKCONS)) {
		uint64 tail_len;
		_zip_buffer_set_offset(buffer, eocd_offset + EOCDLEN);
		tail_len = _zip_buffer_left(buffer);
		if(tail_len < comment_len || ((za->open_flags & ZIP_CHECKCONS) && tail_len != comment_len)) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_cdir_free(cd);
			return NULL;
		}

		if(comment_len) {
			if((cd->comment =
			    _zip_string_new(_zip_buffer_get(buffer, comment_len), comment_len, ZIP_FL_ENC_GUESS, error)) == NULL) {
				_zip_cdir_free(cd);
				return NULL;
			}
		}
	}

	if(cd->offset >= buf_offset) {
		uint8 * data;
		/* if buffer already read in, use it */
		_zip_buffer_set_offset(buffer, cd->offset - buf_offset);

		if((data = _zip_buffer_get(buffer, cd->size)) == NULL) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_cdir_free(cd);
			return NULL;
		}
		if((cd_buffer = _zip_buffer_new(data, cd->size)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			_zip_cdir_free(cd);
			return NULL;
		}
	}
	else {
		cd_buffer = NULL;

		if(zip_source_seek(za->src, (int64)cd->offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, za->src);
			_zip_cdir_free(cd);
			return NULL;
		}

		/* possible consistency check: cd->offset = len-(cd->size+cd->comment_len+EOCDLEN) ? */
		if(zip_source_tell(za->src) != (int64)cd->offset) {
			zip_error_set(error, SLERR_ZIP_NOZIP, 0);
			_zip_cdir_free(cd);
			return NULL;
		}
	}

	left = (uint64)cd->size;
	i = 0;
	while(left > 0) {
		bool grown = false;
		int64 entry_size;

		if(i == cd->nentry) {
			/* InfoZIP has a hack to avoid using Zip64: it stores nentries % 0x10000 */
			/* This hack isn't applicable if we're using Zip64, or if there is no central directory entry
			   following. */

			if(cd->is_zip64 || left < CDENTRYSIZE) {
				break;
			}

			if(!_zip_cdir_grow(cd, 0x10000, error)) {
				_zip_cdir_free(cd);
				_zip_buffer_free(cd_buffer);
				return NULL;
			}
			grown = true;
		}

		if((cd->entry[i].orig = _zip_dirent_new()) == NULL ||
		    (entry_size = _zip_dirent_read(cd->entry[i].orig, za->src, cd_buffer, false, error)) < 0) {
			if(grown && zip_error_code_zip(error) == SLERR_ZIP_NOZIP) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
			}
			_zip_cdir_free(cd);
			_zip_buffer_free(cd_buffer);
			return NULL;
		}
		i++;
		left -= (uint64)entry_size;
	}
	if(i != cd->nentry || left > 0) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		_zip_buffer_free(cd_buffer);
		_zip_cdir_free(cd);
		return NULL;
	}
	if(za->open_flags & ZIP_CHECKCONS) {
		bool ok;
		if(cd_buffer) {
			ok = _zip_buffer_eof(cd_buffer);
		}
		else {
			int64 offset = zip_source_tell(za->src);
			if(offset < 0) {
				_zip_error_set_from_source(error, za->src);
				_zip_cdir_free(cd);
				return NULL;
			}
			ok = ((uint64)offset == cd->offset + cd->size);
		}
		if(!ok) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_buffer_free(cd_buffer);
			_zip_cdir_free(cd);
			return NULL;
		}
	}
	_zip_buffer_free(cd_buffer);
	return cd;
}
//
// _zip_checkcons:
// Checks the consistency of the central directory by comparing central
// directory entries with local headers and checking for plausible
// file and header offsets. Returns -1 if not plausible, else the
// difference between the lowest and the highest fileposition reached */
// 
static int64 _zip_checkcons(zip_t * za, zip_cdir_t * cd, zip_error_t * error) 
{
	uint64 i;
	uint64 min, max, j;
	struct zip_dirent temp;
	_zip_dirent_init(&temp);
	if(cd->nentry) {
		max = cd->entry[0].orig->offset;
		min = cd->entry[0].orig->offset;
	}
	else
		min = max = 0;

	for(i = 0; i < cd->nentry; i++) {
		if(cd->entry[i].orig->offset < min)
			min = cd->entry[i].orig->offset;
		if(min > (uint64)cd->offset) {
			zip_error_set(error, SLERR_ZIP_NOZIP, 0);
			return -1;
		}
		j = cd->entry[i].orig->offset + cd->entry[i].orig->comp_size + _zip_string_length(cd->entry[i].orig->filename) + LENTRYSIZE;
		if(j > max)
			max = j;
		if(max > (uint64)cd->offset) {
			zip_error_set(error, SLERR_ZIP_NOZIP, 0);
			return -1;
		}
		if(zip_source_seek(za->src, (int64)cd->entry[i].orig->offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, za->src);
			return -1;
		}
		if(_zip_dirent_read(&temp, za->src, NULL, true, error) == -1) {
			_zip_dirent_finalize(&temp);
			return -1;
		}
		if(_zip_headercomp(cd->entry[i].orig, &temp) != 0) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_dirent_finalize(&temp);
			return -1;
		}
		cd->entry[i].orig->extra_fields = _zip_ef_merge(cd->entry[i].orig->extra_fields, temp.extra_fields);
		cd->entry[i].orig->local_extra_fields_read = 1;
		temp.extra_fields = NULL;
		_zip_dirent_finalize(&temp);
	}
	return (max - min) < ZIP_INT64_MAX ? (int64)(max - min) : ZIP_INT64_MAX;
}
// 
// _zip_headercomp:
// compares a central directory entry and a local file header
// Return 0 if they are consistent, -1 if not. */
// 
static int _zip_headercomp(const zip_dirent_t * central, const zip_dirent_t * local) 
{
	if((central->version_needed < local->version_needed)
#if 0
	    /* some zip-files have different values in local
	       and global headers for the bitflags */
	    || (central->bitflags != local->bitflags)
#endif
	    || (central->comp_method != local->comp_method) || (central->last_mod != local->last_mod) ||
	    !_zip_string_equal(central->filename, local->filename))
		return -1;
	if((central->crc != local->crc) || (central->comp_size != local->comp_size) || (central->uncomp_size != local->uncomp_size)) {
		/* InfoZip stores valid values in local header even when data descriptor is used.
		   This is in violation of the appnote. */
		if(((local->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) == 0 || local->crc != 0 || local->comp_size != 0 ||
		    local->uncomp_size != 0))
			return -1;
	}
	return 0;
}

static zip_t * _zip_allocate_new(zip_source_t * src, uint flags, zip_error_t * error) 
{
	zip_t * za;
	if((za = _zip_new(error)) == NULL) {
		return NULL;
	}
	za->src = src;
	za->open_flags = flags;
	if(flags & ZIP_RDONLY) {
		za->flags |= ZIP_AFL_RDONLY;
		za->ch_flags |= ZIP_AFL_RDONLY;
	}
	return za;
}
/*
 * tests for file existence
 */
static exists_t _zip_file_exists(zip_source_t * src, zip_error_t * error) 
{
	struct zip_stat st;
	zip_stat_init(&st);
	if(zip_source_stat(src, &st) != 0) {
		zip_error_t * src_error = zip_source_error(src);
		if(zip_error_code_zip(src_error) == SLERR_ZIP_READ && zip_error_code_system(src_error) == ENOENT) {
			return EXISTS_NOT;
		}
		_zip_error_copy(error, src_error);
		return EXISTS_ERROR;
	}
	return EXISTS_OK;
}

static zip_cdir_t * _zip_find_central_dir(zip_t * za, uint64 len) 
{
	zip_cdir_t * cdir, * cdirnew;
	uint8 * match;
	int64 buf_offset;
	uint64 buflen;
	int64 a;
	int64 best;
	zip_error_t error;
	zip_buffer_t * buffer;
	if(len < EOCDLEN) {
		zip_error_set(&za->error, SLERR_ZIP_NOZIP, 0);
		return NULL;
	}
	buflen = (len < CDBUFSIZE ? len : CDBUFSIZE);
	if(zip_source_seek(za->src, -(int64)buflen, SEEK_END) < 0) {
		zip_error_t * src_error = zip_source_error(za->src);
		if(zip_error_code_zip(src_error) != SLERR_ZIP_SEEK || zip_error_code_system(src_error) != EFBIG) {
			/* seek before start of file on my machine */
			_zip_error_copy(&za->error, src_error);
			return NULL;
		}
	}
	if((buf_offset = zip_source_tell(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return NULL;
	}
	if((buffer = _zip_buffer_new_from_source(za->src, buflen, NULL, &za->error)) == NULL) {
		return NULL;
	}
	best = -1;
	cdir = NULL;
	if(buflen >= CDBUFSIZE) {
		/* EOCD64 locator is before EOCD, so leave place for it */
		_zip_buffer_set_offset(buffer, EOCD64LOCLEN);
	}
	zip_error_set(&error, SLERR_ZIP_NOZIP, 0);
	match = _zip_buffer_get(buffer, 0);
	while((match = _zip_memmem(match, static_cast<size_t>(_zip_buffer_left(buffer) - (EOCDLEN - 4)), (const unsigned char*)EOCD_MAGIC, 4)) != NULL) {
		_zip_buffer_set_offset(buffer, (uint64)(match - _zip_buffer_data(buffer)));
		if((cdirnew = _zip_read_cdir(za, buffer, (uint64)buf_offset, &error)) != NULL) {
			if(cdir) {
				if(best <= 0) {
					best = _zip_checkcons(za, cdir, &error);
				}

				a = _zip_checkcons(za, cdirnew, &error);
				if(best < a) {
					_zip_cdir_free(cdir);
					cdir = cdirnew;
					best = a;
				}
				else {
					_zip_cdir_free(cdirnew);
				}
			}
			else {
				cdir = cdirnew;
				if(za->open_flags & ZIP_CHECKCONS)
					best = _zip_checkcons(za, cdir, &error);
				else {
					best = 0;
				}
			}
			cdirnew = NULL;
		}
		match++;
		_zip_buffer_set_offset(buffer, (uint64)(match - _zip_buffer_data(buffer)));
	}
	_zip_buffer_free(buffer);
	if(best < 0) {
		_zip_error_copy(&za->error, &error);
		_zip_cdir_free(cdir);
		return NULL;
	}
	return cdir;
}

static unsigned char * _zip_memmem(const unsigned char * big, size_t biglen, const unsigned char * little, size_t littlelen) 
{
	const unsigned char * p;
	if((biglen < littlelen) || (littlelen == 0))
		return NULL;
	p = big - 1;
	while((p = (const unsigned char*)memchr(p + 1, little[0], (size_t)(big - (p + 1)) + (size_t)(biglen - littlelen) + 1)) != NULL) {
		if(memcmp(p + 1, little + 1, littlelen - 1) == 0)
			return (unsigned char*)p;
	}
	return NULL;
}

static zip_cdir_t * _zip_read_eocd(zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error) 
{
	zip_cdir_t * cd;
	uint64 i, nentry, size, offset, eocd_offset;
	if(_zip_buffer_left(buffer) < EOCDLEN) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	eocd_offset = _zip_buffer_offset(buffer);
	_zip_buffer_get(buffer, 4); /* magic already verified */
	if(_zip_buffer_get_32(buffer) != 0) {
		zip_error_set(error, SLERR_ZIP_MULTIDISK, 0);
		return NULL;
	}
	/* number of cdir-entries on this disk */
	i = _zip_buffer_get_16(buffer);
	/* number of cdir-entries */
	nentry = _zip_buffer_get_16(buffer);
	if(nentry != i) {
		zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		return NULL;
	}
	size = _zip_buffer_get_32(buffer);
	offset = _zip_buffer_get_32(buffer);
	if(offset + size < offset) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	if(offset + size > buf_offset + eocd_offset) {
		/* cdir spans past EOCD record */
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((flags & ZIP_CHECKCONS) && offset + size != buf_offset + eocd_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((cd = _zip_cdir_new(nentry, error)) == NULL)
		return NULL;
	cd->is_zip64 = false;
	cd->size = size;
	cd->offset = offset;
	return cd;
}

static zip_cdir_t * _zip_read_eocd64(zip_source_t * src, zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error) 
{
	zip_cdir_t * cd;
	uint64 offset;
	uint8 eocd[EOCD64LEN];
	uint64 eocd_offset;
	uint64 size, nentry, i, eocdloc_offset;
	bool free_buffer;
	uint32 num_disks, num_disks64, eocd_disk, eocd_disk64;
	eocdloc_offset = _zip_buffer_offset(buffer);
	_zip_buffer_get(buffer, 4); /* magic already verified */
	num_disks = _zip_buffer_get_16(buffer);
	eocd_disk = _zip_buffer_get_16(buffer);
	eocd_offset = _zip_buffer_get_64(buffer);
	/* valid seek value for start of EOCD */
	if(eocd_offset > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	/* does EOCD fit before EOCD locator? */
	if(eocd_offset + EOCD64LEN > eocdloc_offset + buf_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	/* make sure current position of buffer is beginning of EOCD */
	if(eocd_offset >= buf_offset && eocd_offset + EOCD64LEN <= buf_offset + _zip_buffer_size(buffer)) {
		_zip_buffer_set_offset(buffer, eocd_offset - buf_offset);
		free_buffer = false;
	}
	else {
		if(zip_source_seek(src, (int64)eocd_offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, src);
			return NULL;
		}
		if((buffer = _zip_buffer_new_from_source(src, EOCD64LEN, eocd, error)) == NULL) {
			return NULL;
		}
		free_buffer = true;
	}
	if(memcmp(_zip_buffer_get(buffer, 4), EOCD64_MAGIC, 4) != 0) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	/* size of EOCD */
	size = _zip_buffer_get_64(buffer);
	/* is there a hole between EOCD and EOCD locator, or do they overlap? */
	if((flags & ZIP_CHECKCONS) && size + eocd_offset + 12 != buf_offset + eocdloc_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	_zip_buffer_get(buffer, 4); /* skip version made by/needed */
	num_disks64 = _zip_buffer_get_32(buffer);
	eocd_disk64 = _zip_buffer_get_32(buffer);
	/* if eocd values are 0xffff, we have to use eocd64 values.
	   otherwise, if the values are not the same, it's inconsistent;
	   in any case, if the value is not 0, we don't support it */
	if(num_disks == 0xffff) {
		num_disks = num_disks64;
	}
	if(eocd_disk == 0xffff) {
		eocd_disk = eocd_disk64;
	}
	if((flags & ZIP_CHECKCONS) && (eocd_disk != eocd_disk64 || num_disks != num_disks64)) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	if(num_disks != 0 || eocd_disk != 0) {
		zip_error_set(error, SLERR_ZIP_MULTIDISK, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	nentry = _zip_buffer_get_64(buffer);
	i = _zip_buffer_get_64(buffer);
	if(nentry != i) {
		zip_error_set(error, SLERR_ZIP_MULTIDISK, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	size = _zip_buffer_get_64(buffer);
	offset = _zip_buffer_get_64(buffer);
	/* did we read past the end of the buffer? */
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		if(free_buffer) {
			_zip_buffer_free(buffer);
		}
		return NULL;
	}
	if(free_buffer) {
		_zip_buffer_free(buffer);
	}
	if(offset > ZIP_INT64_MAX || offset + size < offset) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	if(offset + size > buf_offset + eocd_offset) {
		// cdir spans past EOCD record 
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((flags & ZIP_CHECKCONS) && offset + size != buf_offset + eocd_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if(nentry > size / CDENTRYSIZE) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((cd = _zip_cdir_new(nentry, error)) == NULL)
		return NULL;
	cd->is_zip64 = true;
	cd->size = size;
	cd->offset = offset;
	return cd;
}
// 
// Descr: frees the space allocated to a zipfile struct, and closes the corresponding file.
// 
void zip_discard(zip_t * za) 
{
	uint64 i;
	if(za) {
		if(za->src) {
			zip_source_close(za->src);
			zip_source_free(za->src);
		}
		SAlloc::F(za->default_password);
		_zip_string_free(za->comment_orig);
		_zip_string_free(za->comment_changes);
		_zip_hash_free(za->names);
		if(za->entry) {
			for(i = 0; i < za->nentry; i++)
				_zip_entry_finalize(za->entry + i);
			SAlloc::F(za->entry);
		}
		for(i = 0; i < za->nopen_source; i++) {
			_zip_source_invalidate(za->open_source[i]);
		}
		SAlloc::F(za->open_source);
		_zip_progress_free(za->progress);
		zip_error_fini(&za->error);
		SAlloc::F(za);
	}
}
//
// zip close
//
static int add_data(zip_t *, zip_source_t *, zip_dirent_t *, uint32);
static int copy_data(zip_t *, uint64);
static int copy_source(zip_t *, zip_source_t *, int64);
static int write_cdir(zip_t *, const zip_filelist_t *, uint64);
static int write_data_descriptor(zip_t * za, const zip_dirent_t * dirent, int is_zip64);

ZIP_EXTERN int zip_close(zip_t * za) 
{
	uint64 i, j, survivors, unchanged_offset;
	int64 off;
	int error;
	zip_filelist_t * filelist;
	int changed;
	if(za == NULL)
		return -1;
	changed = _zip_changed(za, &survivors);
	// don't create zip files with no entries 
	if(survivors == 0) {
		if((za->open_flags & ZIP_TRUNCATE) || changed) {
			if(zip_source_remove(za->src) < 0) {
				if(!((zip_error_code_zip(zip_source_error(za->src)) == SLERR_ZIP_REMOVE) && (zip_error_code_system(zip_source_error(za->src)) == ENOENT))) {
					_zip_error_set_from_source(&za->error, za->src);
					return -1;
				}
			}
		}
		zip_discard(za);
		return 0;
	}
	if(!changed) {
		zip_discard(za);
		return 0;
	}
	if(survivors > za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	if((filelist = (zip_filelist_t*)SAlloc::M(sizeof(filelist[0]) * (size_t)survivors)) == NULL)
		return -1;
	unchanged_offset = ZIP_UINT64_MAX;
	// create list of files with index into original archive 
	for(i = j = 0; i < za->nentry; i++) {
		if(za->entry[i].orig && ZIP_ENTRY_HAS_CHANGES(&za->entry[i])) {
			unchanged_offset = MIN(unchanged_offset, za->entry[i].orig->offset);
		}
		if(za->entry[i].deleted) {
			continue;
		}
		if(j >= survivors) {
			SAlloc::F(filelist);
			zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			return -1;
		}
		filelist[j].idx = i;
		j++;
	}
	if(j < survivors) {
		SAlloc::F(filelist);
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	if((zip_source_supports(za->src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_BEGIN_WRITE_CLONING)) == 0) {
		unchanged_offset = 0;
	}
	else {
		if(unchanged_offset == ZIP_UINT64_MAX) {
			// we're keeping all file data, find the end of the last one 
			uint64 last_index = ZIP_UINT64_MAX;
			unchanged_offset = 0;
			for(i = 0; i < za->nentry; i++) {
				if(za->entry[i].orig != NULL) {
					if(za->entry[i].orig->offset >= unchanged_offset) {
						unchanged_offset = za->entry[i].orig->offset;
						last_index = i;
					}
				}
			}
			if(last_index != ZIP_UINT64_MAX) {
				if((unchanged_offset = _zip_file_get_end(za, last_index, &za->error)) == 0) {
					SAlloc::F(filelist);
					return -1;
				}
			}
		}
		if(unchanged_offset > 0) {
			if(zip_source_begin_write_cloning(za->src, unchanged_offset) < 0) {
				// cloning not supported, need to copy everything 
				unchanged_offset = 0;
			}
		}
	}
	if(unchanged_offset == 0) {
		if(zip_source_begin_write(za->src) < 0) {
			_zip_error_set_from_source(&za->error, za->src);
			SAlloc::F(filelist);
			return -1;
		}
	}
	if(_zip_progress_start(za->progress) != 0) {
		zip_error_set(&za->error, SLERR_ZIP_CANCELLED, 0);
		zip_source_rollback_write(za->src);
		SAlloc::F(filelist);
		return -1;
	}
	error = 0;
	for(j = 0; j < survivors; j++) {
		int new_data;
		zip_entry_t * entry;
		zip_dirent_t * de;
		if(_zip_progress_subrange(za->progress, (double)j / (double)survivors, (double)(j + 1) / (double)survivors) != 0) {
			zip_error_set(&za->error, SLERR_ZIP_CANCELLED, 0);
			error = 1;
			break;
		}
		i = filelist[j].idx;
		entry = za->entry + i;
		if(entry->orig != NULL && entry->orig->offset < unchanged_offset) {
			// already implicitly copied by cloning 
			continue;
		}
		new_data = (ZIP_ENTRY_DATA_CHANGED(entry) || ZIP_ENTRY_CHANGED(entry, ZIP_DIRENT_COMP_METHOD) || ZIP_ENTRY_CHANGED(entry, ZIP_DIRENT_ENCRYPTION_METHOD));
		// create new local directory entry 
		if(!entry->changes) {
			if((entry->changes = _zip_dirent_clone(entry->orig)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
				error = 1;
				break;
			}
		}
		de = entry->changes;
		if(_zip_read_local_ef(za, i) < 0) {
			error = 1;
			break;
		}
		if((off = zip_source_tell_write(za->src)) < 0) {
			_zip_error_set_from_source(&za->error, za->src);
			error = 1;
			break;
		}
		de->offset = static_cast<uint64>(off);
		if(new_data) {
			zip_source_t * zs = NULL;
			if(!ZIP_ENTRY_DATA_CHANGED(entry)) {
				if((zs = _zip_source_zip_new(za, za, i, ZIP_FL_UNCHANGED, 0, 0, NULL)) == NULL) {
					error = 1;
					break;
				}
			}
			// add_data writes dirent 
			if(add_data(za, zs ? zs : entry->source, de, entry->changes ? entry->changes->changed : 0) < 0) {
				error = 1;
				if(zs)
					zip_source_free(zs);
				break;
			}
			if(zs)
				zip_source_free(zs);
		}
		else {
			uint64 offset;
			if(de->encryption_method != ZIP_EM_TRAD_PKWARE) {
				// when copying data, all sizes are known -> no data descriptor needed */
				// except for PKWare encryption, where removing the data descriptor breaks password validation 
				de->bitflags &= ~ZIP_GPBF_DATA_DESCRIPTOR;
			}
			if(_zip_dirent_write(za, de, ZIP_FL_LOCAL) < 0) {
				error = 1;
				break;
			}
			if((offset = _zip_file_get_offset(za, i, &za->error)) == 0) {
				error = 1;
				break;
			}
			if(zip_source_seek(za->src, static_cast<int64>(offset), SEEK_SET) < 0) {
				_zip_error_set_from_source(&za->error, za->src);
				error = 1;
				break;
			}
			if(copy_data(za, de->comp_size) < 0) {
				error = 1;
				break;
			}
			if(de->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
				if(write_data_descriptor(za, de, _zip_dirent_needs_zip64(de, 0)) < 0) {
					error = 1;
					break;
				}
			}
		}
	}
	if(!error) {
		if(write_cdir(za, filelist, survivors) < 0)
			error = 1;
	}
	SAlloc::F(filelist);
	if(!error) {
		if(zip_source_commit_write(za->src) != 0) {
			_zip_error_set_from_source(&za->error, za->src);
			error = 1;
		}
		_zip_progress_end(za->progress);
	}
	if(error) {
		zip_source_rollback_write(za->src);
		return -1;
	}
	zip_discard(za);
	return 0;
}

static int add_data(zip_t * za, zip_source_t * src, zip_dirent_t * de, uint32 changed) 
{
	int64 offstart, offdata, offend, data_length;
	zip_stat_t st;
	zip_file_attributes_t attributes;
	zip_source_t * src_final, * src_tmp;
	int ret;
	int is_zip64;
	zip_flags_t flags;
	bool needs_recompress;
	bool needs_decompress;
	bool needs_crc;
	bool needs_compress;
	bool needs_reencrypt;
	bool needs_decrypt;
	bool needs_encrypt;
	if(zip_source_stat(src, &st) < 0) {
		_zip_error_set_from_source(&za->error, src);
		return -1;
	}
	if(!(st.valid & ZIP_STAT_COMP_METHOD)) {
		st.valid |= ZIP_STAT_COMP_METHOD;
		st.comp_method = ZIP_CM_STORE;
	}
	if(ZIP_CM_IS_DEFAULT(de->comp_method) && st.comp_method != ZIP_CM_STORE)
		de->comp_method = st.comp_method;
	else if(de->comp_method == ZIP_CM_STORE && (st.valid & ZIP_STAT_SIZE)) {
		st.valid |= ZIP_STAT_COMP_SIZE;
		st.comp_size = st.size;
	}
	else {
		// we'll recompress 
		st.valid &= ~ZIP_STAT_COMP_SIZE;
	}
	if(!(st.valid & ZIP_STAT_ENCRYPTION_METHOD)) {
		st.valid |= ZIP_STAT_ENCRYPTION_METHOD;
		st.encryption_method = ZIP_EM_NONE;
	}
	flags = ZIP_EF_LOCAL;
	if(!(st.valid & ZIP_STAT_SIZE)) {
		flags |= ZIP_FL_FORCE_ZIP64;
		data_length = -1;
	}
	else {
		de->uncomp_size = st.size;
		// this is technically incorrect (copy_source counts compressed data), but it's the best we have 
		data_length = (int64)st.size;
		if(!(st.valid & ZIP_STAT_COMP_SIZE)) {
			uint64 max_size;
			switch(ZIP_CM_ACTUAL(de->comp_method)) {
				case ZIP_CM_BZIP2:
				    // computed by looking at increase of 10 random files of size 1MB when compressed with bzip2, rounded up: 1.006 
				    max_size = 4269351188u;
				    break;
				case ZIP_CM_DEFLATE:
				    // max deflate size increase: size + ceil(size/16k)*5+6 
				    max_size = 4293656963u;
				    break;
				case ZIP_CM_STORE:
				    max_size = 0xffffffffu;
				    break;
				default:
				    max_size = 0;
			}
			if(st.size > max_size) {
				flags |= ZIP_FL_FORCE_ZIP64;
			}
		}
		else
			de->comp_size = st.comp_size;
	}
	if((offstart = zip_source_tell_write(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	// as long as we don't support non-seekable output, clear data descriptor bit 
	de->bitflags &= ~ZIP_GPBF_DATA_DESCRIPTOR;
	if((is_zip64 = _zip_dirent_write(za, de, flags)) < 0) {
		return -1;
	}
	needs_recompress = st.comp_method != ZIP_CM_ACTUAL(de->comp_method);
	needs_decompress = needs_recompress && (st.comp_method != ZIP_CM_STORE);
	needs_crc = (st.comp_method == ZIP_CM_STORE) || needs_decompress;
	needs_compress = needs_recompress && (de->comp_method != ZIP_CM_STORE);
	needs_reencrypt = needs_recompress || (de->changed & ZIP_DIRENT_PASSWORD) || (de->encryption_method != st.encryption_method);
	needs_decrypt = needs_reencrypt && (st.encryption_method != ZIP_EM_NONE);
	needs_encrypt = needs_reencrypt && (de->encryption_method != ZIP_EM_NONE);
	src_final = src;
	zip_source_keep(src_final);
	if(needs_decrypt) {
		zip_encryption_implementation impl = _zip_get_encryption_implementation(st.encryption_method, ZIP_CODEC_DECODE);
		if(!impl) {
			zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
			zip_source_free(src_final);
			return -1;
		}
		if((src_tmp = impl(za, src_final, st.encryption_method, ZIP_CODEC_DECODE, za->default_password)) == NULL) {
			// error set by impl 
			zip_source_free(src_final);
			return -1;
		}
		zip_source_free(src_final);
		src_final = src_tmp;
	}
	if(needs_decompress) {
		if((src_tmp = zip_source_decompress(za, src_final, st.comp_method)) == NULL) {
			zip_source_free(src_final);
			return -1;
		}
		zip_source_free(src_final);
		src_final = src_tmp;
	}
	if(needs_crc) {
		if((src_tmp = zip_source_crc(za, src_final, 0)) == NULL) {
			zip_source_free(src_final);
			return -1;
		}
		zip_source_free(src_final);
		src_final = src_tmp;
	}
	if(needs_compress) {
		if((src_tmp = zip_source_compress(za, src_final, de->comp_method, de->compression_level)) == NULL) {
			zip_source_free(src_final);
			return -1;
		}
		zip_source_free(src_final);
		src_final = src_tmp;
	}
	if(needs_encrypt) {
		zip_encryption_implementation impl;
		const char * password = NULL;
		if(de->password) {
			password = de->password;
		}
		else if(za->default_password) {
			password = za->default_password;
		}
		if((impl = _zip_get_encryption_implementation(de->encryption_method, ZIP_CODEC_ENCODE)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
			zip_source_free(src_final);
			return -1;
		}
		if((src_tmp = impl(za, src_final, de->encryption_method, ZIP_CODEC_ENCODE, password)) == NULL) {
			// error set by impl 
			zip_source_free(src_final);
			return -1;
		}
		if(de->encryption_method == ZIP_EM_TRAD_PKWARE) {
			de->bitflags |= ZIP_GPBF_DATA_DESCRIPTOR;
		}
		zip_source_free(src_final);
		src_final = src_tmp;
	}
	if((offdata = zip_source_tell_write(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	ret = copy_source(za, src_final, data_length);
	if(zip_source_stat(src_final, &st) < 0) {
		_zip_error_set_from_source(&za->error, src_final);
		ret = -1;
	}
	if(zip_source_get_file_attributes(src_final, &attributes) != 0) {
		_zip_error_set_from_source(&za->error, src_final);
		ret = -1;
	}
	zip_source_free(src_final);
	if(ret < 0) {
		return -1;
	}
	if((offend = zip_source_tell_write(za->src)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if(zip_source_seek_write(za->src, offstart, SEEK_SET) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if((st.valid & (ZIP_STAT_COMP_METHOD | ZIP_STAT_CRC | ZIP_STAT_SIZE)) != (ZIP_STAT_COMP_METHOD | ZIP_STAT_CRC | ZIP_STAT_SIZE)) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	if((de->changed & ZIP_DIRENT_LAST_MOD) == 0) {
		if(st.valid & ZIP_STAT_MTIME)
			de->last_mod = st.mtime;
		else
			time(&de->last_mod);
	}
	de->comp_method = st.comp_method;
	de->crc = st.crc;
	de->uncomp_size = st.size;
	de->comp_size = (uint64)(offend - offdata);
	_zip_dirent_apply_attributes(de, &attributes, (flags & ZIP_FL_FORCE_ZIP64) != 0, changed);
	if((ret = _zip_dirent_write(za, de, flags)) < 0)
		return -1;
	if(is_zip64 != ret) {
		// Zip64 mismatch between preliminary file header written before data and final file header written afterwards 
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	if(zip_source_seek_write(za->src, offend, SEEK_SET) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if(de->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) {
		if(write_data_descriptor(za, de, is_zip64) < 0) {
			return -1;
		}
	}
	return 0;
}

static int copy_data(zip_t * za, uint64 len) 
{
	DEFINE_BYTE_ARRAY(buf, BUFSIZE);
	size_t n;
	double total = (double)len;
	if(!byte_array_init(buf, BUFSIZE)) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	while(len > 0) {
		n = (len > BUFSIZE) ? BUFSIZE : static_cast<size_t>(len);
		if(_zip_read(za->src, buf, n, &za->error) < 0) {
			byte_array_fini(buf);
			return -1;
		}
		if(_zip_write(za, buf, n) < 0) {
			byte_array_fini(buf);
			return -1;
		}
		len -= n;
		if(_zip_progress_update(za->progress, (total - (double)len) / total) != 0) {
			zip_error_set(&za->error, SLERR_ZIP_CANCELLED, 0);
			return -1;
		}
	}
	byte_array_fini(buf);
	return 0;
}

static int copy_source(zip_t * za, zip_source_t * src, int64 data_length) 
{
	DEFINE_BYTE_ARRAY(buf, BUFSIZE);
	int64 n, current;
	int ret;
	if(zip_source_open(src) < 0) {
		_zip_error_set_from_source(&za->error, src);
		return -1;
	}
	if(!byte_array_init(buf, BUFSIZE)) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	ret = 0;
	current = 0;
	while((n = zip_source_read(src, buf, BUFSIZE)) > 0) {
		if(_zip_write(za, buf, (uint64)n) < 0) {
			ret = -1;
			break;
		}
		if(n == BUFSIZE && za->progress && data_length > 0) {
			current += n;
			if(_zip_progress_update(za->progress, (double)current / (double)data_length) != 0) {
				zip_error_set(&za->error, SLERR_ZIP_CANCELLED, 0);
				ret = -1;
				break;
			}
		}
	}
	if(n < 0) {
		_zip_error_set_from_source(&za->error, src);
		ret = -1;
	}
	byte_array_fini(buf);
	zip_source_close(src);
	return ret;
}

static int write_cdir(zip_t * za, const zip_filelist_t * filelist, uint64 survivors) 
{
	int64 cd_start, end, size;
	if((cd_start = zip_source_tell_write(za->src)) < 0) {
		return -1;
	}
	if((size = _zip_cdir_write(za, filelist, survivors)) < 0) {
		return -1;
	}
	if((end = zip_source_tell_write(za->src)) < 0) {
		return -1;
	}
	return 0;
}

int _zip_changed(const zip_t * za, uint64 * survivorsp) 
{
	int changed = 0;
	uint64 i;
	uint64 survivors = 0;
	if(za->comment_changed || za->ch_flags != za->flags) {
		changed = 1;
	}
	for(i = 0; i < za->nentry; i++) {
		if(ZIP_ENTRY_HAS_CHANGES(&za->entry[i])) {
			changed = 1;
		}
		if(!za->entry[i].deleted) {
			survivors++;
		}
	}
	if(survivorsp) {
		*survivorsp = survivors;
	}
	return changed;
}

static int write_data_descriptor(zip_t * za, const zip_dirent_t * de, int is_zip64) 
{
	zip_buffer_t * buffer = _zip_buffer_new(NULL, MAX_DATA_DESCRIPTOR_LENGTH);
	int ret = 0;
	if(buffer == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	_zip_buffer_put(buffer, DATADES_MAGIC, 4);
	_zip_buffer_put_32(buffer, de->crc);
	if(is_zip64) {
		_zip_buffer_put_64(buffer, de->comp_size);
		_zip_buffer_put_64(buffer, de->uncomp_size);
	}
	else {
		_zip_buffer_put_32(buffer, (uint32)de->comp_size);
		_zip_buffer_put_32(buffer, (uint32)de->uncomp_size);
	}
	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		ret = -1;
	}
	else {
		ret = _zip_write(za, _zip_buffer_data(buffer), _zip_buffer_offset(buffer));
	}
	_zip_buffer_free(buffer);
	return ret;
}
//
// Descr: create and init struct zip_entry
// NOTE: Signed due to -1 on error.  See zip_add.c for more details. 
//
int64 _zip_add_entry(zip_t * za) 
{
	uint64 idx;
	if(za->nentry + 1 >= za->nentry_alloc) {
		zip_entry_t * rentries;
		uint64 nalloc = za->nentry_alloc;
		uint64 additional_entries = 2 * nalloc;
		uint64 realloc_size;
		if(additional_entries < 16) {
			additional_entries = 16;
		}
		else if(additional_entries > 1024) {
			additional_entries = 1024;
		}
		// neither + nor * overflows can happen: nentry_alloc * sizeof(struct zip_entry) < UINT64_MAX 
		nalloc += additional_entries;
		realloc_size = sizeof(struct zip_entry) * (size_t)nalloc;
		if(sizeof(struct zip_entry) * (size_t)za->nentry_alloc > realloc_size) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			return -1;
		}
		rentries = (zip_entry_t*)SAlloc::R(za->entry, sizeof(struct zip_entry) * (size_t)nalloc);
		if(!rentries) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			return -1;
		}
		za->entry = rentries;
		za->nentry_alloc = nalloc;
	}
	idx = za->nentry++;
	_zip_entry_init(za->entry + idx);
	return (int64)idx;
}
// 
// Descr: add file via callback function
// NOTE: Return type is signed so we can return -1 on error.
//   The index can not be larger than ZIP_INT64_MAX since the size
//   of the central directory cannot be larger than
//   ZIP_UINT64_MAX, and each entry is larger than 2 bytes.
// 
ZIP_EXTERN int64 zip_add(zip_t * za, const char * name, zip_source_t * source) 
{
	return zip_file_add(za, name, source, 0);
}
//
// Descr: add directory
// NOTE: Signed due to -1 on error.  See zip_add.c for more details. 
//
ZIP_EXTERN int64 zip_dir_add(zip_t * za, const char * name, zip_flags_t flags) 
{
	size_t len;
	int64 idx;
	char * s;
	zip_source_t * source;
	if(ZIP_IS_RDONLY(za)) {
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
		return -1;
	}
	if(name == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	s = NULL;
	len = strlen(name);
	if(name[len - 1] != '/') {
		if((s = (char *)SAlloc::M(len + 2)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			return -1;
		}
		strcpy(s, name);
		s[len] = '/';
		s[len + 1] = '\0';
	}
	if((source = zip_source_buffer(za, NULL, 0, 0)) == NULL) {
		SAlloc::F(s);
		return -1;
	}
	idx = _zip_file_replace(za, ZIP_UINT64_MAX, s ? s : name, source, flags);
	SAlloc::F(s);
	if(idx < 0)
		zip_source_free(source);
	else {
		if(zip_file_set_external_attributes(za, (uint64)idx, 0, ZIP_OPSYS_DEFAULT, ZIP_EXT_ATTRIB_DEFAULT_DIR) < 0) {
			zip_delete(za, (uint64)idx);
			return -1;
		}
	}
	return idx;
}
//
// Descr: add directory
// NOTE: Signed due to -1 on error.  See zip_add.c for more details. 
//
ZIP_EXTERN int64 zip_add_dir(zip_t * za, const char * name) { return zip_dir_add(za, name, 0); }
//
// Descr: delete file from zip archive
//
ZIP_EXTERN int zip_delete(zip_t * za, uint64 idx) 
{
	int    result = -1;
	if(idx >= za->nentry)
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else if(ZIP_IS_RDONLY(za))
		zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	else {
		const char * name = _zip_get_name(za, idx, 0, &za->error);
		if(name) {
			if(_zip_hash_delete(za->names, (const uint8*)name, &za->error)) {
				if(_zip_unchange(za, idx, 1) == 0) { // allow duplicate file names, because the file will be removed directly afterwards 
					za->entry[idx].deleted = 1;
					result = 0;
				}
			}
		}
	}
	return result;
}
//
// Descr: rename file in zip archive
//
ZIP_EXTERN int zip_rename(zip_t * za, uint64 idx, const char * name) { return zip_file_rename(za, idx, name, 0); }
//
// Descr: replace file via callback function
//
ZIP_EXTERN int zip_replace(zip_t * za, uint64 idx, zip_source_t * source) { return zip_file_replace(za, idx, source, 0); }
//
// zip random
//
// zip-random-win32 {
#ifndef HAVE_SECURE_RANDOM
	#include <wincrypt.h>

	ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
	{
		HCRYPTPROV hprov;
		if(!CryptAcquireContext(&hprov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
			return false;
		}
		if(!CryptGenRandom(hprov, length, buffer)) {
			return false;
		}
		if(!CryptReleaseContext(hprov, 0)) {
			return false;
		}
		return true;
	}
#endif
#ifndef HAVE_RANDOM_UINT32
	uint32 zip_random_uint32(void) 
	{
		static bool seeded = false;
		uint32 value;
		if(zip_secure_random((uint8 *)&value, sizeof(value))) {
			return value;
		}
		else {
			if(!seeded)
				srand((uint)time(NULL));
			return (uint32)rand();
		}
	}
#endif
// } zip-random-win32
// zip-random-unix {
#ifdef HAVE_ARC4RANDOM
	#ifndef HAVE_SECURE_RANDOM
		ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length)
		{
			arc4random_buf(buffer, length);
			return true;
		}
	#endif
	#ifndef HAVE_RANDOM_UINT32
		uint32 zip_random_uint32(void) 
		{
			return arc4random();
		}
	#endif
#else /* HAVE_ARC4RANDOM */
	#ifndef HAVE_SECURE_RANDOM
		ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
		{
			int fd;
			if((fd = open("/dev/urandom", O_RDONLY)) < 0) {
				return false;
			}
			if(read(fd, buffer, length) != length) {
				close(fd);
				return false;
			}
			close(fd);
			return true;
		}
	#endif
	#ifndef HAVE_RANDOM_UINT32
		/* @sobolev
		uint32 zip_random_uint32(void) 
		{
			static bool seeded = false;
			uint32 value;
			if(zip_secure_random((uint8 *)&value, sizeof(value))) {
				return value;
			}
			if(!seeded) {
				srandom((uint)time(NULL));
			}
			return (uint32)random();
		}*/
	#endif
#endif /* HAVE_ARC4RANDOM */
// } zip-random-unix 
// zip-random-uwp {
	#ifndef HAVE_SECURE_RANDOM
	#include <bcrypt.h>
	#include <ntstatus.h>

	ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
	{
		BCRYPT_ALG_HANDLE hAlg = NULL;
		NTSTATUS hr = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
		if(hr != STATUS_SUCCESS || hAlg == NULL) {
			return false;
		}
		hr = BCryptGenRandom(&hAlg, buffer, length, 0);
		BCryptCloseAlgorithmProvider(&hAlg, 0);
		if(hr != STATUS_SUCCESS) {
			return false;
		}
		return true;
	}
#endif
// } zip-random-uwp
//
// zip-source
//
//
// Descr: get last error from zip_source
//
zip_error_t * zip_source_error(zip_source_t * src) { return &src->error; }
bool _zip_source_had_error(zip_source_t * src) { return zip_source_error(src)->zip_err != SLERR_SUCCESS; }
//
// Descr: open zip_source (prepare for reading)
//
ZIP_EXTERN int zip_source_open(zip_source_t * src) 
{
	if(src->source_closed) {
		return -1;
	}
	if(src->write_state == ZIP_SOURCE_WRITE_REMOVED) {
		zip_error_set(&src->error, SLERR_ZIP_DELETED, 0);
		return -1;
	}
	if(ZIP_SOURCE_IS_OPEN_READING(src)) {
		if((zip_source_supports(src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK)) == 0) {
			zip_error_set(&src->error, SLERR_ZIP_INUSE, 0);
			return -1;
		}
	}
	else {
		if(ZIP_SOURCE_IS_LAYERED(src)) {
			if(zip_source_open(src->src) < 0) {
				_zip_error_set_from_source(&src->error, src->src);
				return -1;
			}
		}
		if(_zip_source_call(src, NULL, 0, ZIP_SOURCE_OPEN) < 0) {
			if(ZIP_SOURCE_IS_LAYERED(src)) {
				zip_source_close(src->src);
			}
			return -1;
		}
	}
	src->eof = false;
	src->had_read_error = false;
	_zip_error_clear(&src->error);
	src->open_count++;
	return 0;
}
//
// Descr: close zip_source (stop reading)
//
int zip_source_close(zip_source_t * src) 
{
	if(!ZIP_SOURCE_IS_OPEN_READING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	src->open_count--;
	if(src->open_count == 0) {
		_zip_source_call(src, NULL, 0, ZIP_SOURCE_CLOSE);
		if(ZIP_SOURCE_IS_LAYERED(src)) {
			if(zip_source_close(src->src) < 0) {
				zip_error_set(&src->error, SLERR_ZIP_INTERNAL, 0);
			}
		}
	}
	return 0;
}
//
// Descr: free zip data source
//
ZIP_EXTERN void FASTCALL zip_source_free(zip_source_t * src) 
{
	if(src) {
		if(src->refcount > 0) {
			src->refcount--;
		}
		if(src->refcount > 0) {
			return;
		}
		else {
			if(ZIP_SOURCE_IS_OPEN_READING(src)) {
				src->open_count = 1; /* force close */
				zip_source_close(src);
			}
			if(ZIP_SOURCE_IS_OPEN_WRITING(src)) {
				zip_source_rollback_write(src);
			}
			if(src->source_archive && !src->source_closed) {
				_zip_deregister_source(src->source_archive, src);
			}
			(void)_zip_source_call(src, NULL, 0, ZIP_SOURCE_FREE);
			if(src->src) {
				zip_source_free(src->src); // @recursion
			}
			SAlloc::F(src);
		}
	}
}
//
// Descr: seek to offset
//
ZIP_EXTERN int zip_source_seek(zip_source_t * src, int64 offset, int whence) 
{
	zip_source_args_seek_t args;
	if(src->source_closed) {
		return -1;
	}
	if(!ZIP_SOURCE_IS_OPEN_READING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	args.offset = offset;
	args.whence = whence;
	if(_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK) < 0) {
		return -1;
	}
	src->eof = 0;
	return 0;
}

int64 zip_source_seek_compute_offset(uint64 offset, uint64 length, void * data, uint64 data_length, zip_error_t * error) 
{
	int64 new_offset = -1;
	zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, data_length, error);
	if(args) {
		switch(args->whence) {
			case SEEK_CUR: new_offset = (int64)offset + args->offset; break;
			case SEEK_END: new_offset = (int64)length + args->offset; break;
			case SEEK_SET: new_offset = args->offset; break;
			default: zip_error_set(error, SLERR_ZIP_INVAL, 0); return -1;
		}
		if(new_offset < 0 || (uint64)new_offset > length) {
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
			return -1;
		}
	}
	return new_offset;
}
//
// Descr: seek to offset for writing
//
ZIP_EXTERN int zip_source_seek_write(zip_source_t * src, int64 offset, int whence) 
{
	zip_source_args_seek_t args;
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else {
		args.offset = offset;
		args.whence = whence;
		return (_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK_WRITE) < 0 ? -1 : 0);
	}
}
//
// Descr: report current offset
//
ZIP_EXTERN int64 zip_source_tell(zip_source_t * src) 
{
	if(src->source_closed) {
		return -1;
	}
	if(!ZIP_SOURCE_IS_OPEN_READING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL);
}
//
// Descr: report current offset for writing
//
ZIP_EXTERN int64 zip_source_tell_write(zip_source_t * src) 
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else
		return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL_WRITE);
}
//
// Descr: read data from zip_source
//
int64 zip_source_read(zip_source_t * src, void * data, uint64 len) 
{
	uint64 bytes_read;
	int64 n;
	if(src->source_closed) {
		return -1;
	}
	if(!ZIP_SOURCE_IS_OPEN_READING(src) || len > ZIP_INT64_MAX || (len > 0 && data == NULL)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(src->had_read_error) {
		return -1;
	}
	if(_zip_source_eof(src)) {
		return 0;
	}
	if(len == 0) {
		return 0;
	}
	bytes_read = 0;
	while(bytes_read < len) {
		if((n = _zip_source_call(src, (uint8 *)data + bytes_read, len - bytes_read, ZIP_SOURCE_READ)) < 0) {
			src->had_read_error = true;
			if(bytes_read == 0) {
				return -1;
			}
			else {
				return (int64)bytes_read;
			}
		}
		if(n == 0) {
			src->eof = 1;
			break;
		}
		bytes_read += (uint64)n;
	}
	return (int64)bytes_read;
}

bool _zip_source_eof(zip_source_t * src) { return src->eof; }
//
// Descr: start a new file for writing
//
ZIP_EXTERN int zip_source_begin_write(zip_source_t * src) 
{
	if(ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_source_call(src, NULL, 0, ZIP_SOURCE_BEGIN_WRITE) < 0) {
		return -1;
	}
	src->write_state = ZIP_SOURCE_WRITE_OPEN;
	return 0;
}
//
// Descr: clone part of file for writing
//
ZIP_EXTERN int zip_source_begin_write_cloning(zip_source_t * src, uint64 offset) 
{
	if(ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(_zip_source_call(src, NULL, offset, ZIP_SOURCE_BEGIN_WRITE_CLONING) < 0) {
		return -1;
	}
	src->write_state = ZIP_SOURCE_WRITE_OPEN;
	return 0;
}
//
// Descr: start a new file for writing
//
ZIP_EXTERN int64 zip_source_write(zip_source_t * src, const void * data, uint64 length) 
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src) || length > ZIP_INT64_MAX) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	else
		return _zip_source_call(src, (void*)data, length, ZIP_SOURCE_WRITE);
}
//
// Descr: commit changes to file
//
ZIP_EXTERN int zip_source_commit_write(zip_source_t * src) 
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src)) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(src->open_count > 1) {
		zip_error_set(&src->error, SLERR_ZIP_INUSE, 0);
		return -1;
	}
	else if(ZIP_SOURCE_IS_OPEN_READING(src)) {
		if(zip_source_close(src) < 0) {
			return -1;
		}
	}
	if(_zip_source_call(src, NULL, 0, ZIP_SOURCE_COMMIT_WRITE) < 0) {
		src->write_state = ZIP_SOURCE_WRITE_FAILED;
		return -1;
	}
	src->write_state = ZIP_SOURCE_WRITE_CLOSED;
	return 0;
}
//
// Descr: discard changes
//
ZIP_EXTERN void zip_source_rollback_write(zip_source_t * src) 
{
	if(oneof2(src->write_state, ZIP_SOURCE_WRITE_OPEN, ZIP_SOURCE_WRITE_FAILED)) {
		_zip_source_call(src, NULL, 0, ZIP_SOURCE_ROLLBACK_WRITE);
		src->write_state = ZIP_SOURCE_WRITE_CLOSED;
	}
}
//
// Descr: remove empty archive
//
int zip_source_remove(zip_source_t * src) 
{
	if(src->write_state == ZIP_SOURCE_WRITE_REMOVED) {
		return 0;
	}
	if(ZIP_SOURCE_IS_OPEN_READING(src)) {
		if(zip_source_close(src) < 0) {
			return -1;
		}
	}
	if(src->write_state != ZIP_SOURCE_WRITE_CLOSED) {
		zip_source_rollback_write(src);
	}
	if(_zip_source_call(src, NULL, 0, ZIP_SOURCE_REMOVE) < 0) {
		return -1;
	}
	src->write_state = ZIP_SOURCE_WRITE_REMOVED;
	return 0;
}
//
// Descr: was archive was removed?
//
ZIP_EXTERN int zip_source_is_deleted(zip_source_t * src) { return src->write_state == ZIP_SOURCE_WRITE_REMOVED; }
//
// Descr: get meta information from zip_source
//
ZIP_EXTERN int zip_source_stat(zip_source_t * src, zip_stat_t * st) 
{
	if(src->source_closed) {
		return -1;
	}
	if(st == NULL) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	zip_stat_init(st);
	if(ZIP_SOURCE_IS_LAYERED(src)) {
		if(zip_source_stat(src->src, st) < 0) {
			_zip_error_set_from_source(&src->error, src->src);
			return -1;
		}
	}
	if(_zip_source_call(src, st, sizeof(*st), ZIP_SOURCE_STAT) < 0) {
		return -1;
	}
	return 0;
}
//
// zip-source-buffer
//
#ifndef WRITE_FRAGMENT_SIZE
	#define WRITE_FRAGMENT_SIZE (64 * 1024)
#endif

struct buffer {
	zip_buffer_fragment_t * fragments; /* fragments */
	uint64 * fragment_offsets; /* offset of each fragment from start of buffer, nfragments+1 entries */
	uint64 nfragments;      /* number of allocated fragments */
	uint64 fragments_capacity; /* size of fragments (number of pointers) */
	uint64 first_owned_fragment; /* first fragment to free data from */
	uint64 shared_fragments; /* number of shared fragments */
	struct buffer * shared_buffer; /* buffer fragments are shared with */
	uint64 size;         /* size of buffer */
	uint64 offset;       /* current offset in buffer */
	uint64 current_fragment; /* fragment current offset is in */
};

typedef struct buffer buffer_t;

struct read_data {
	zip_error_t error;
	time_t mtime;
	zip_file_attributes_t attributes;
	buffer_t * in;
	buffer_t * out;
};

#define buffer_capacity(buffer) ((buffer)->fragment_offsets[(buffer)->nfragments])
#define buffer_size(buffer) ((buffer)->size)

static buffer_t * buffer_clone(buffer_t * buffer, uint64 length, zip_error_t * error);
static uint64 buffer_find_fragment(const buffer_t * buffer, uint64 offset);
static void buffer_free(buffer_t * buffer);
static bool buffer_grow_fragments(buffer_t * buffer, uint64 capacity, zip_error_t * error);
static buffer_t * buffer_new(const zip_buffer_fragment_t * fragments, uint64 nfragments, int free_data, zip_error_t * error);
static int64 buffer_read(buffer_t * buffer, uint8 * data, uint64 length);
static int buffer_seek(buffer_t * buffer, void * data, uint64 len, zip_error_t * error);
static int64 buffer_write(buffer_t * buffer, const uint8 * data, uint64 length, zip_error_t *);
static int64 read_data(void *, void *, uint64, zip_source_cmd_t);
zip_source_t * zip_source_buffer_with_attributes_create(const void * data, uint64 len, int freep,
    zip_file_attributes_t * attributes, zip_error_t * error);
zip_source_t * zip_source_buffer_fragment_with_attributes_create(const zip_buffer_fragment_t * fragments, uint64 nfragments,
    int freep, zip_file_attributes_t * attributes, zip_error_t * error);

ZIP_EXTERN zip_source_t * zip_source_buffer(zip_t * za, const void * data, uint64 len, int freep) 
{
	return za ? zip_source_buffer_with_attributes_create(data, len, freep, NULL, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_buffer_create(const void * data, uint64 len, int freep, zip_error_t * error) 
{
	return zip_source_buffer_with_attributes_create(data, len, freep, NULL, error);
}

zip_source_t * zip_source_buffer_with_attributes_create(const void * data, uint64 len, int freep,
    zip_file_attributes_t * attributes, zip_error_t * error) 
{
	zip_buffer_fragment_t fragment;
	if(!data) {
		if(len > 0) {
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
			return NULL;
		}
		return zip_source_buffer_fragment_with_attributes_create(NULL, 0, freep, attributes, error);
	}
	fragment.data = (uint8 *)data;
	fragment.length = len;
	return zip_source_buffer_fragment_with_attributes_create(&fragment, 1, freep, attributes, error);
}

ZIP_EXTERN zip_source_t * zip_source_buffer_fragment(zip_t * za, const zip_buffer_fragment_t * fragments, uint64 nfragments, int freep) 
{
	return za ? zip_source_buffer_fragment_with_attributes_create(fragments, nfragments, freep, NULL, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_buffer_fragment_create(const zip_buffer_fragment_t * fragments,
    uint64 nfragments, int freep, zip_error_t * error) 
{
	return zip_source_buffer_fragment_with_attributes_create(fragments, nfragments, freep, NULL, error);
}

zip_source_t * zip_source_buffer_fragment_with_attributes_create(const zip_buffer_fragment_t * fragments, uint64 nfragments,
    int freep, zip_file_attributes_t * attributes, zip_error_t * error) 
{
	struct read_data * ctx;
	zip_source_t * zs;
	buffer_t * buffer;
	if(fragments == NULL && nfragments > 0) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((buffer = buffer_new(fragments, nfragments, freep, error)) == NULL) {
		return NULL;
	}
	if((ctx = (struct read_data *)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		buffer_free(buffer);
		return NULL;
	}
	ctx->in = buffer;
	ctx->out = NULL;
	ctx->mtime = time(NULL);
	if(attributes) {
		memcpy(&ctx->attributes, attributes, sizeof(ctx->attributes));
	}
	else {
		zip_file_attributes_init(&ctx->attributes);
	}
	zip_error_init(&ctx->error);
	if((zs = zip_source_function_create(read_data, ctx, error)) == NULL) {
		buffer_free(ctx->in);
		SAlloc::F(ctx);
		return NULL;
	}
	return zs;
}

zip_source_t * zip_source_buffer_with_attributes(zip_t * za, const void * data, uint64 len,
    int freep, zip_file_attributes_t * attributes) 
{
	return zip_source_buffer_with_attributes_create(data, len, freep, attributes, &za->error);
}

static int64 read_data(void * state, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	struct read_data * ctx = (struct read_data *)state;
	switch(cmd) {
		case ZIP_SOURCE_BEGIN_WRITE:
		    if((ctx->out = buffer_new(NULL, 0, 0, &ctx->error)) == NULL) {
			    return -1;
		    }
		    ctx->out->offset = 0;
		    ctx->out->current_fragment = 0;
		    return 0;
		case ZIP_SOURCE_BEGIN_WRITE_CLONING:
		    if((ctx->out = buffer_clone(ctx->in, len, &ctx->error)) == NULL) {
			    return -1;
		    }
		    ctx->out->offset = len;
		    ctx->out->current_fragment = ctx->out->nfragments;
		    return 0;
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_COMMIT_WRITE:
		    buffer_free(ctx->in);
		    ctx->in = ctx->out;
		    ctx->out = NULL;
		    return 0;
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    buffer_free(ctx->in);
		    buffer_free(ctx->out);
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES: {
		    if(len < sizeof(ctx->attributes)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    memcpy(data, &ctx->attributes, sizeof(ctx->attributes));
		    return sizeof(ctx->attributes);
	    }
		case ZIP_SOURCE_OPEN:
		    ctx->in->offset = 0;
		    ctx->in->current_fragment = 0;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(len > ZIP_INT64_MAX) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    return buffer_read(ctx->in, static_cast<uint8 *>(data), len);
		case ZIP_SOURCE_REMOVE: {
		    buffer_t * empty = buffer_new(NULL, 0, 0, &ctx->error);
		    if(empty == NULL) {
			    return -1;
		    }
		    buffer_free(ctx->in);
		    ctx->in = empty;
		    return 0;
	    }
		case ZIP_SOURCE_ROLLBACK_WRITE:
		    buffer_free(ctx->out);
		    ctx->out = NULL;
		    return 0;
		case ZIP_SOURCE_SEEK: return buffer_seek(ctx->in, data, len, &ctx->error);
		case ZIP_SOURCE_SEEK_WRITE: return buffer_seek(ctx->out, data, len, &ctx->error);
		case ZIP_SOURCE_STAT: {
		    zip_stat_t * st;
		    if(len < sizeof(*st)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    st = (zip_stat_t*)data;
		    zip_stat_init(st);
		    st->mtime = ctx->mtime;
		    st->size = ctx->in->size;
		    st->comp_size = st->size;
		    st->comp_method = ZIP_CM_STORE;
		    st->encryption_method = ZIP_EM_NONE;
		    st->valid = ZIP_STAT_MTIME | ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD | ZIP_STAT_ENCRYPTION_METHOD;
		    return sizeof(*st);
	    }
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_GET_FILE_ATTRIBUTES,
			       ZIP_SOURCE_OPEN,
			       ZIP_SOURCE_READ,
			       ZIP_SOURCE_CLOSE,
			       ZIP_SOURCE_STAT,
			       ZIP_SOURCE_ERROR,
			       ZIP_SOURCE_FREE,
			       ZIP_SOURCE_SEEK,
			       ZIP_SOURCE_TELL,
			       ZIP_SOURCE_BEGIN_WRITE,
			       ZIP_SOURCE_BEGIN_WRITE_CLONING,
			       ZIP_SOURCE_COMMIT_WRITE,
			       ZIP_SOURCE_REMOVE,
			       ZIP_SOURCE_ROLLBACK_WRITE,
			       ZIP_SOURCE_SEEK_WRITE,
			       ZIP_SOURCE_TELL_WRITE,
			       ZIP_SOURCE_WRITE,
			       -1);

		case ZIP_SOURCE_TELL:
		    if(ctx->in->offset > ZIP_INT64_MAX) {
			    zip_error_set(&ctx->error, SLERR_ZIP_TELL, EOVERFLOW);
			    return -1;
		    }
		    return (int64)ctx->in->offset;

		case ZIP_SOURCE_TELL_WRITE:
		    if(ctx->out->offset > ZIP_INT64_MAX) {
			    zip_error_set(&ctx->error, SLERR_ZIP_TELL, EOVERFLOW);
			    return -1;
		    }
		    return (int64)ctx->out->offset;

		case ZIP_SOURCE_WRITE:
		    if(len > ZIP_INT64_MAX) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    return buffer_write(ctx->out, static_cast<uint8 *>(data), len, &ctx->error);
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
		    return -1;
	}
}

static buffer_t * buffer_clone(buffer_t * buffer, uint64 offset, zip_error_t * error) 
{
	uint64 fragment, fragment_offset, waste;
	buffer_t * clone;
	if(offset == 0) {
		return buffer_new(NULL, 0, 1, error);
	}
	if(offset > buffer->size) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(buffer->shared_buffer != NULL) {
		zip_error_set(error, SLERR_ZIP_INUSE, 0);
		return NULL;
	}
	fragment = buffer_find_fragment(buffer, offset);
	fragment_offset = offset - buffer->fragment_offsets[fragment];
	if(fragment_offset == 0) {
		fragment--;
		fragment_offset = buffer->fragments[fragment].length;
	}
	waste = buffer->fragments[fragment].length - fragment_offset;
	if(waste > offset) {
		zip_error_set(error, SLERR_ZIP_OPNOTSUPP, 0);
		return NULL;
	}
	if((clone = buffer_new(buffer->fragments, fragment + 1, 0, error)) == NULL) {
		return NULL;
	}
#ifndef __clang_analyzer__
	/* clone->fragments can't be null, since it was created with at least one fragment */
	clone->fragments[clone->nfragments - 1].length = fragment_offset;
#endif
	clone->fragment_offsets[clone->nfragments] = offset;
	clone->size = offset;
	clone->first_owned_fragment = MIN(buffer->first_owned_fragment, clone->nfragments - 1);
	buffer->shared_buffer = clone;
	clone->shared_buffer = buffer;
	buffer->shared_fragments = clone->nfragments;
	clone->shared_fragments = fragment + 1;
	return clone;
}

static uint64 buffer_find_fragment(const buffer_t * buffer, uint64 offset) 
{
	uint64 mid;
	uint64 low = 0;
	uint64 high = buffer->nfragments - 1;
	while(low < high) {
		mid = (high - low) / 2 + low;
		if(buffer->fragment_offsets[mid] > offset) {
			high = mid - 1;
		}
		else if(mid == buffer->nfragments || buffer->fragment_offsets[mid + 1] > offset) {
			return mid;
		}
		else {
			low = mid + 1;
		}
	}
	return low;
}

static void buffer_free(buffer_t * buffer) 
{
	if(buffer) {
		if(buffer->shared_buffer != NULL) {
			buffer->shared_buffer->shared_buffer = NULL;
			buffer->shared_buffer->shared_fragments = 0;
			buffer->first_owned_fragment = MAX(buffer->first_owned_fragment, buffer->shared_fragments);
		}
		for(uint64 i = buffer->first_owned_fragment; i < buffer->nfragments; i++) {
			SAlloc::F(buffer->fragments[i].data);
		}
		SAlloc::F(buffer->fragments);
		SAlloc::F(buffer->fragment_offsets);
		SAlloc::F(buffer);
	}
}

static bool buffer_grow_fragments(buffer_t * buffer, uint64 capacity, zip_error_t * error) 
{
	zip_buffer_fragment_t * fragments;
	uint64 * offsets;
	if(capacity < buffer->fragments_capacity) {
		return true;
	}
	if((fragments = static_cast<zip_buffer_fragment_t *>(SAlloc::R(buffer->fragments, sizeof(buffer->fragments[0]) * static_cast<size_t>(capacity)))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	buffer->fragments = fragments;
	if((offsets = static_cast<uint64 *>(SAlloc::R(buffer->fragment_offsets, sizeof(buffer->fragment_offsets[0]) * static_cast<size_t>(capacity + 1)))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	buffer->fragment_offsets = offsets;
	buffer->fragments_capacity = capacity;
	return true;
}

static buffer_t * buffer_new(const zip_buffer_fragment_t * fragments, uint64 nfragments, int free_data, zip_error_t * error) 
{
	buffer_t * buffer = static_cast<buffer_t *>(SAlloc::M(sizeof(*buffer)));
	if(buffer == NULL) {
		return NULL;
	}
	buffer->offset = 0;
	buffer->first_owned_fragment = 0;
	buffer->size = 0;
	buffer->fragments = NULL;
	buffer->fragment_offsets = NULL;
	buffer->nfragments = 0;
	buffer->fragments_capacity = 0;
	buffer->shared_buffer = NULL;
	buffer->shared_fragments = 0;
	if(nfragments == 0) {
		if((buffer->fragment_offsets = static_cast<uint64 *>(SAlloc::M(sizeof(buffer->fragment_offsets[0])))) == NULL) {
			SAlloc::F(buffer);
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		buffer->fragment_offsets[0] = 0;
	}
	else {
		uint64 i, j, offset;
		if(!buffer_grow_fragments(buffer, nfragments, NULL)) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			buffer_free(buffer);
			return NULL;
		}
		offset = 0;
		for(i = 0, j = 0; i < nfragments; i++) {
			if(fragments[i].length == 0) {
				continue;
			}
			if(fragments[i].data == NULL) {
				zip_error_set(error, SLERR_ZIP_INVAL, 0);
				buffer_free(buffer);
				return NULL;
			}
			buffer->fragments[j].data = fragments[i].data;
			buffer->fragments[j].length = fragments[i].length;
			buffer->fragment_offsets[i] = offset;
			offset += fragments[i].length;
			j++;
		}
		buffer->nfragments = j;
		buffer->first_owned_fragment = free_data ? 0 : buffer->nfragments;
		buffer->fragment_offsets[buffer->nfragments] = offset;
		buffer->size = offset;
	}
	return buffer;
}

static int64 buffer_read(buffer_t * buffer, uint8 * data, uint64 length) 
{
	uint64 n, i, fragment_offset;
	length = MIN(length, buffer->size - buffer->offset);
	if(length == 0) {
		return 0;
	}
	if(length > ZIP_INT64_MAX) {
		return -1;
	}
	i = buffer->current_fragment;
	fragment_offset = buffer->offset - buffer->fragment_offsets[i];
	n = 0;
	while(n < length) {
		uint64 left = MIN(length - n, buffer->fragments[i].length - fragment_offset);
		memcpy(data + n, buffer->fragments[i].data + fragment_offset, static_cast<size_t>(left));
		if(left == buffer->fragments[i].length - fragment_offset) {
			i++;
		}
		n += left;
		fragment_offset = 0;
	}
	buffer->offset += n;
	buffer->current_fragment = i;
	return (int64)n;
}

static int buffer_seek(buffer_t * buffer, void * data, uint64 len, zip_error_t * error) 
{
	const int64 new_offset = zip_source_seek_compute_offset(buffer->offset, buffer->size, data, len, error);
	if(new_offset < 0) {
		return -1;
	}
	else {
		buffer->offset = (uint64)new_offset;
		buffer->current_fragment = buffer_find_fragment(buffer, buffer->offset);
		return 0;
	}
}

static int64 buffer_write(buffer_t * buffer, const uint8 * data, uint64 length, zip_error_t * error) 
{
	uint64 n, i, fragment_offset, capacity;
	if(buffer->offset + length + WRITE_FRAGMENT_SIZE - 1 < length) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	// grow buffer if needed 
	capacity = buffer_capacity(buffer);
	if(buffer->offset + length > capacity) {
		uint64 needed_fragments = buffer->nfragments + (length - (capacity - buffer->offset) + WRITE_FRAGMENT_SIZE - 1) / WRITE_FRAGMENT_SIZE;
		if(needed_fragments > buffer->fragments_capacity) {
			uint64 new_capacity = buffer->fragments_capacity;
			SETIFZ(new_capacity, 16);
			while(new_capacity < needed_fragments) {
				new_capacity *= 2;
			}
			if(!buffer_grow_fragments(buffer, new_capacity, error)) {
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
		}
		while(buffer->nfragments < needed_fragments) {
			if((buffer->fragments[buffer->nfragments].data = static_cast<uint8 *>(SAlloc::M(WRITE_FRAGMENT_SIZE))) == NULL) {
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
				return -1;
			}
			buffer->fragments[buffer->nfragments].length = WRITE_FRAGMENT_SIZE;
			buffer->nfragments++;
			capacity += WRITE_FRAGMENT_SIZE;
			buffer->fragment_offsets[buffer->nfragments] = capacity;
		}
	}
	i = buffer->current_fragment;
	fragment_offset = buffer->offset - buffer->fragment_offsets[i];
	n = 0;
	while(n < length) {
		uint64 left = MIN(length - n, buffer->fragments[i].length - fragment_offset);
		memcpy(buffer->fragments[i].data + fragment_offset, data + n, static_cast<size_t>(left));
		if(left == buffer->fragments[i].length - fragment_offset) {
			i++;
		}
		n += left;
		fragment_offset = 0;
	}
	buffer->offset += n;
	buffer->current_fragment = i;
	if(buffer->offset > buffer->size)
		buffer->size = buffer->offset;
	return (int64)n;
}
//
// zip-source-function
// create zip data source from callback function
//
ZIP_EXTERN zip_source_t * zip_source_function(zip_t * za, zip_source_callback zcb, void * ud) 
{
	return za ? zip_source_function_create(zcb, ud, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_function_create(zip_source_callback zcb, void * ud, zip_error_t * error) 
{
	zip_source_t * zs = _zip_source_new(error);
	if(zs) {
		zs->cb.f = zcb;
		zs->ud = ud;
		zs->supports = zcb(ud, NULL, 0, ZIP_SOURCE_SUPPORTS);
		if(zs->supports < 0)
			zs->supports = ZIP_SOURCE_SUPPORTS_READABLE;
	}
	return zs;
}

ZIP_EXTERN void zip_source_keep(zip_source_t * src) { src->refcount++; }

zip_source_t * _zip_source_new(zip_error_t * error) 
{
	zip_source_t * src = (zip_source_t*)SAlloc::M(sizeof(*src));
	if(!src)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		src->src = NULL;
		src->cb.f = NULL;
		src->ud = NULL;
		src->open_count = 0;
		src->write_state = ZIP_SOURCE_WRITE_CLOSED;
		src->source_closed = false;
		src->source_archive = NULL;
		src->refcount = 1;
		zip_error_init(&src->error);
		src->eof = false;
		src->had_read_error = false;
	}
	return src;
}
//
// zip-source-layered
// create layered source
//
zip_source_t * zip_source_layered(zip_t * za, zip_source_t * src, zip_source_layered_callback cb, void * ud) 
{
	return za ? zip_source_layered_create(src, cb, ud, &za->error) : 0;
}

zip_source_t * zip_source_layered_create(zip_source_t * src, zip_source_layered_callback cb, void * ud, zip_error_t * error) 
{
	zip_source_t * zs = _zip_source_new(error);
	if(zs) {
		zip_source_keep(src);
		zs->src = src;
		zs->cb.l = cb;
		zs->ud = ud;
		zs->supports = cb(src, ud, NULL, 0, ZIP_SOURCE_SUPPORTS);
		if(zs->supports < 0)
			zs->supports = ZIP_SOURCE_SUPPORTS_READABLE;
	}
	return zs;
}
//
// Descr: invoke callback command on zip_source
//
int64 _zip_source_call(zip_source_t * src, void * data, uint64 length, zip_source_cmd_t command) 
{
	int64 ret;
	if((src->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(command)) == 0) {
		zip_error_set(&src->error, SLERR_ZIP_OPNOTSUPP, 0);
		return -1;
	}
	else {
		if(!src->src)
			ret = src->cb.f(src->ud, data, length, command);
		else
			ret = src->cb.l(src->src, src->ud, data, length, command);
		if(ret < 0) {
			if(command != ZIP_SOURCE_ERROR && command != ZIP_SOURCE_SUPPORTS) {
				int e[2];
				if(_zip_source_call(src, e, sizeof(e), ZIP_SOURCE_ERROR) < 0)
					zip_error_set(&src->error, SLERR_ZIP_INTERNAL, 0);
				else
					zip_error_set(&src->error, e[0], e[1]);
			}
		}
		return ret;
	}
}
//
// Descr: if empty source is a valid archive
//
bool zip_source_accept_empty(zip_source_t * src) 
{
	int ret;
	if((zip_source_supports(src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_ACCEPT_EMPTY)) == 0) {
		if(ZIP_SOURCE_IS_LAYERED(src)) {
			return zip_source_accept_empty(src->src);
		}
		return true;
	}
	ret = (int)_zip_source_call(src, NULL, 0, ZIP_SOURCE_ACCEPT_EMPTY);
	return ret != 0;
}
//
// check for supported functions
//
int64 FASTCALL zip_source_supports(const zip_source_t * src) { return src->supports; }

ZIP_EXTERN int64 zip_source_make_command_bitmap(zip_source_cmd_t cmd0, ...) 
{
	va_list ap;
	int64 bitmap = ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd0);
	va_start(ap, cmd0);
	for(;;) {
		int cmd = va_arg(ap, int);
		if(cmd < 0) {
			break;
		}
		bitmap |= ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd);
	}
	va_end(ap);
	return bitmap;
}
//
// zip-source-window
// return part of lower source
//
struct window {
	uint64 start; /* where in file we start reading */
	uint64 end; /* where in file we stop reading */
	/* if not NULL, read file data for this file */
	zip_t * source_archive;
	uint64 source_index;
	uint64 offset; /* offset in src for next read */
	zip_stat_t stat;
	zip_file_attributes_t attributes;
	zip_error_t error;
	int64 supports;
	bool needs_seek;
};

static int64 window_read(zip_source_t *, void *, void *, uint64, zip_source_cmd_t);

zip_source_t * zip_source_window(zip_t * za, zip_source_t * src, uint64 start, uint64 len) 
{
	return _zip_source_window_new(src, start, len, NULL, 0, NULL, 0, &za->error);
}

zip_source_t * _zip_source_window_new(zip_source_t * src, uint64 start, uint64 length,
    zip_stat_t * st, zip_file_attributes_t * attributes, zip_t * source_archive, uint64 source_index, zip_error_t * error) 
{
	struct window * ctx;
	if(src == NULL || start + length < start || (source_archive == NULL && source_index != 0)) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = (struct window *)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->start = start;
	ctx->end = start + length;
	zip_stat_init(&ctx->stat);
	if(attributes != NULL) {
		memcpy(&ctx->attributes, attributes, sizeof(ctx->attributes));
	}
	else {
		zip_file_attributes_init(&ctx->attributes);
	}
	ctx->source_archive = source_archive;
	ctx->source_index = source_index;
	zip_error_init(&ctx->error);
	ctx->supports = (zip_source_supports(src) & ZIP_SOURCE_SUPPORTS_SEEKABLE) |
	    (zip_source_make_command_bitmap(ZIP_SOURCE_GET_FILE_ATTRIBUTES, ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1));
	ctx->needs_seek = (ctx->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK)) ? true : false;
	if(st) {
		if(_zip_stat_merge(&ctx->stat, st, error) < 0) {
			SAlloc::F(ctx);
			return NULL;
		}
	}
	return zip_source_layered_create(src, window_read, ctx, error);
}

int _zip_source_set_source_archive(zip_source_t * src, zip_t * za) 
{
	src->source_archive = za;
	return _zip_register_source(za, src);
}

// called by zip_discard to avoid operating on file from closed archive 
void _zip_source_invalidate(zip_source_t * src) 
{
	src->source_closed = 1;
	if(zip_error_code_zip(&src->error) == SLERR_SUCCESS)
		zip_error_set(&src->error, SLERR_ZIP_ZIPCLOSED, 0);
}

static int64 window_read(zip_source_t * src, void * _ctx, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	int64 ret;
	uint64 n, i;
	struct window * ctx = (struct window *)_ctx;
	switch(cmd) {
		case ZIP_SOURCE_CLOSE: return 0;
		case ZIP_SOURCE_ERROR: return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE: SAlloc::F(ctx); return 0;
		case ZIP_SOURCE_OPEN:
		    if(ctx->source_archive) {
			    uint64 offset;
			    if((offset = _zip_file_get_offset(ctx->source_archive, ctx->source_index, &ctx->error)) == 0) {
				    return -1;
			    }
			    if(ctx->end + offset < ctx->end) {
				    /* zip archive data claims end of data past zip64 limits */
				    zip_error_set(&ctx->error, SLERR_ZIP_INCONS, 0);
				    return -1;
			    }
			    ctx->start += offset;
			    ctx->end += offset;
			    ctx->source_archive = NULL;
		    }
		    if(!ctx->needs_seek) {
			    DEFINE_BYTE_ARRAY(b, BUFSIZE);
			    if(!byte_array_init(b, BUFSIZE)) {
				    zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
				    return -1;
			    }
			    for(n = 0; n < ctx->start; n += (uint64)ret) {
				    i = (ctx->start - n > BUFSIZE ? BUFSIZE : ctx->start - n);
				    if((ret = zip_source_read(src, b, i)) < 0) {
					    _zip_error_set_from_source(&ctx->error, src);
					    byte_array_fini(b);
					    return -1;
				    }
				    if(ret == 0) {
					    zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
					    byte_array_fini(b);
					    return -1;
				    }
			    }
			    byte_array_fini(b);
		    }
		    ctx->offset = ctx->start;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(len > ctx->end - ctx->offset)
			    len = ctx->end - ctx->offset;
		    if(len == 0)
			    return 0;
		    if(ctx->needs_seek) {
			    if(zip_source_seek(src, (int64)ctx->offset, SEEK_SET) < 0) {
				    _zip_error_set_from_source(&ctx->error, src);
				    return -1;
			    }
		    }
		    if((ret = zip_source_read(src, data, len)) < 0) {
			    zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
			    return -1;
		    }
		    ctx->offset += (uint64)ret;
		    if(ret == 0) {
			    if(ctx->offset < ctx->end) {
				    zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
				    return -1;
			    }
		    }
		    return ret;
		case ZIP_SOURCE_SEEK: {
		    int64 new_offset = zip_source_seek_compute_offset(ctx->offset - ctx->start, ctx->end - ctx->start, data, len, &ctx->error);
		    if(new_offset < 0) {
			    return -1;
		    }
		    ctx->offset = (uint64)new_offset + ctx->start;
		    return 0;
	    }
		case ZIP_SOURCE_STAT: {
		    zip_stat_t * st = (zip_stat_t*)data;
		    if(_zip_stat_merge(st, &ctx->stat, &ctx->error) < 0) {
			    return -1;
		    }
		    return 0;
	    }
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES:
		    if(len < sizeof(ctx->attributes)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    memcpy(data, &ctx->attributes, sizeof(ctx->attributes));
		    return sizeof(ctx->attributes);
		case ZIP_SOURCE_SUPPORTS: return ctx->supports;
		case ZIP_SOURCE_TELL: return (int64)(ctx->offset - ctx->start);
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
		    return -1;
	}
}

void _zip_deregister_source(zip_t * za, zip_source_t * src) 
{
	for(uint i = 0; i < za->nopen_source; i++) {
		if(za->open_source[i] == src) {
			za->open_source[i] = za->open_source[za->nopen_source - 1];
			za->nopen_source--;
			break;
		}
	}
}

int _zip_register_source(zip_t * za, zip_source_t * src) 
{
	zip_source_t ** open_source;
	if(za->nopen_source + 1 >= za->nopen_source_alloc) {
		uint n = za->nopen_source_alloc + 10;
		open_source = (zip_source_t**)SAlloc::R(za->open_source, n * sizeof(zip_source_t *));
		if(open_source == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			return -1;
		}
		za->nopen_source_alloc = n;
		za->open_source = open_source;
	}
	za->open_source[za->nopen_source++] = src;
	return 0;
}
//
// 
//
ZIP_EXTERN void zip_file_attributes_init(zip_file_attributes_t * attributes) 
{
	attributes->valid = 0;
	attributes->version = 1;
}
//
// Descr: get attributes for file from source
//
int zip_source_get_file_attributes(zip_source_t * src, zip_file_attributes_t * attributes) 
{
	if(src->source_closed) {
		return -1;
	}
	if(attributes == NULL) {
		zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	zip_file_attributes_init(attributes);
	if(src->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_GET_FILE_ATTRIBUTES)) {
		if(_zip_source_call(src, attributes, sizeof(*attributes), ZIP_SOURCE_GET_FILE_ATTRIBUTES) < 0) {
			return -1;
		}
	}
	if(ZIP_SOURCE_IS_LAYERED(src)) {
		zip_file_attributes_t lower_attributes;
		if(zip_source_get_file_attributes(src->src, &lower_attributes) < 0) {
			_zip_error_set_from_source(&src->error, src->src);
			return -1;
		}
		if((lower_attributes.valid & ZIP_FILE_ATTRIBUTES_HOST_SYSTEM) && (attributes->valid & ZIP_FILE_ATTRIBUTES_HOST_SYSTEM) == 0) {
			attributes->host_system = lower_attributes.host_system;
			attributes->valid |= ZIP_FILE_ATTRIBUTES_HOST_SYSTEM;
		}
		if((lower_attributes.valid & ZIP_FILE_ATTRIBUTES_ASCII) && (attributes->valid & ZIP_FILE_ATTRIBUTES_ASCII) == 0) {
			attributes->ascii = lower_attributes.ascii;
			attributes->valid |= ZIP_FILE_ATTRIBUTES_ASCII;
		}
		if((lower_attributes.valid & ZIP_FILE_ATTRIBUTES_VERSION_NEEDED)) {
			if(attributes->valid & ZIP_FILE_ATTRIBUTES_VERSION_NEEDED) {
				attributes->version_needed = MAX(lower_attributes.version_needed, attributes->version_needed);
			}
			else {
				attributes->version_needed = lower_attributes.version_needed;
				attributes->valid |= ZIP_FILE_ATTRIBUTES_VERSION_NEEDED;
			}
		}
		if((lower_attributes.valid & ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES) &&
		    (attributes->valid & ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES) == 0) {
			attributes->external_file_attributes = lower_attributes.external_file_attributes;
			attributes->valid |= ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES;
		}
		if((lower_attributes.valid & ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS)) {
			if(attributes->valid & ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS) {
				attributes->general_purpose_bit_flags &= ~lower_attributes.general_purpose_bit_mask;
				attributes->general_purpose_bit_flags |= lower_attributes.general_purpose_bit_flags &
				    lower_attributes.general_purpose_bit_mask;
				attributes->general_purpose_bit_mask |= lower_attributes.general_purpose_bit_mask;
			}
			else {
				attributes->valid |= ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS;
				attributes->general_purpose_bit_flags = lower_attributes.general_purpose_bit_flags;
				attributes->general_purpose_bit_mask = lower_attributes.general_purpose_bit_mask;
			}
		}
	}
	return 0;
}
//
// zip-source-crc
// pass-through source that calculates CRC32 and size
//
struct crc_context {
	int validate; /* whether to check CRC on EOF and return error on mismatch */
	int crc_complete; /* whether CRC was computed for complete file */
	zip_error_t error;
	uint64 size;
	uint64 position; /* current reading position */
	uint64 crc_position; /* how far we've computed the CRC */
	uint32 crc;
};

static int64 crc_read(zip_source_t *, void *, void *, uint64, zip_source_cmd_t);

zip_source_t * zip_source_crc(zip_t * za, zip_source_t * src, int validate) 
{
	struct crc_context * ctx;
	if(src == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = (struct crc_context *)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	zip_error_init(&ctx->error);
	ctx->validate = validate;
	ctx->crc_complete = 0;
	ctx->crc_position = 0;
	ctx->crc = (uint32)crc32(0, NULL, 0);
	ctx->size = 0;
	return zip_source_layered(za, src, crc_read, ctx);
}

static int64 crc_read(zip_source_t * src, void * _ctx, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	int64 n;
	struct crc_context * ctx = (struct crc_context *)_ctx;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    ctx->position = 0;
		    return 0;
		case ZIP_SOURCE_READ:
		    if((n = zip_source_read(src, data, len)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    if(n == 0) {
			    if(ctx->crc_position == ctx->position) {
				    ctx->crc_complete = 1;
				    ctx->size = ctx->position;
				    if(ctx->validate) {
					    struct zip_stat st;
					    if(zip_source_stat(src, &st) < 0) {
						    _zip_error_set_from_source(&ctx->error, src);
						    return -1;
					    }
					    if((st.valid & ZIP_STAT_CRC) && st.crc != ctx->crc) {
						    zip_error_set(&ctx->error, SLERR_ZIP_CRC, 0);
						    return -1;
					    }
					    if((st.valid & ZIP_STAT_SIZE) && st.size != ctx->size) {
						    zip_error_set(&ctx->error, SLERR_ZIP_INCONS, 0);
						    return -1;
					    }
				    }
			    }
		    }
		    else if(!ctx->crc_complete && ctx->position <= ctx->crc_position) {
			    uint64 i, nn;
			    for(i = ctx->crc_position - ctx->position; i < (uint64)n; i += nn) {
				    nn = MIN(UINT_MAX, (uint64)n - i);
				    ctx->crc = (uint32)crc32(ctx->crc, (const Bytef*)data + i, (uInt)nn);
				    ctx->crc_position += nn;
			    }
		    }
		    ctx->position += (uint64)n;
		    return n;
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT: {
		    zip_stat_t * st = (zip_stat_t*)data;
		    if(ctx->crc_complete) {
			    /* TODO: Set comp_size, comp_method, encryption_method?
			            After all, this only works for uncompressed data. */
			    st->size = ctx->size;
			    st->crc = ctx->crc;
			    st->comp_size = ctx->size;
			    st->comp_method = ZIP_CM_STORE;
			    st->encryption_method = ZIP_EM_NONE;
			    st->valid |= ZIP_STAT_SIZE | ZIP_STAT_CRC | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD |
				ZIP_STAT_ENCRYPTION_METHOD;
		    }
		    return 0;
	    }
		case ZIP_SOURCE_ERROR: return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_SUPPORTS: {
		    int64 mask = zip_source_supports(src);
		    if(mask < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    return mask & ~zip_source_make_command_bitmap(ZIP_SOURCE_BEGIN_WRITE,
			       ZIP_SOURCE_COMMIT_WRITE, ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE,
			       ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE, ZIP_SOURCE_GET_FILE_ATTRIBUTES, -1);
	    }
		case ZIP_SOURCE_SEEK: {
		    int64 new_position;
		    zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
		    if(args == NULL) {
			    return -1;
		    }
		    if(zip_source_seek(src, args->offset, args->whence) < 0 || (new_position = zip_source_tell(src)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    ctx->position = (uint64)new_position;
		    return 0;
	    }
		case ZIP_SOURCE_TELL:
		    return (int64)ctx->position;
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
		    return -1;
	}
}
//
// zip-source-compress
// (de)compression routines
//
struct context {
	zip_error_t error;
	bool end_of_input;
	bool end_of_stream;
	bool can_store;
	bool is_stored; /* only valid if end_of_stream is true */
	bool compress;
	int32 method;
	uint64 size;
	int64 first_read;
	uint8 buffer[BUFSIZE];
	zip_compression_algorithm_t * algorithm;
	void * ud;
};

struct implementation {
	uint16 method;
	zip_compression_algorithm_t * compress;
	zip_compression_algorithm_t * decompress;
};

static struct implementation implementations[] = {
	{ZIP_CM_DEFLATE, &zip_algorithm_deflate_compress, &zip_algorithm_deflate_decompress},
#if defined(HAVE_LIBBZ2)
	{ZIP_CM_BZIP2, &zip_algorithm_bzip2_compress, &zip_algorithm_bzip2_decompress},
#endif
#if defined(HAVE_LIBLZMA)
	/*  Disabled - because 7z isn't able to unpack ZIP+LZMA ZIP+LZMA2
	    archives made this way - and vice versa.

	    {ZIP_CM_LZMA, &zip_algorithm_xz_compress, &zip_algorithm_xz_decompress},
	    {ZIP_CM_LZMA2, &zip_algorithm_xz_compress, &zip_algorithm_xz_decompress},
	 */
	{ZIP_CM_XZ, &zip_algorithm_xz_compress, &zip_algorithm_xz_decompress},
#endif
};

static size_t implementations_size = sizeof(implementations) / sizeof(implementations[0]);

static zip_source_t * compression_source_new(zip_t * za, zip_source_t * src, int32 method, bool compress, int compression_flags);
static int64 compress_callback(zip_source_t *, void *, void *, uint64, zip_source_cmd_t);
static void context_free(struct context * ctx);
static struct context * context_new(int32 method, bool compress, int compression_flags, zip_compression_algorithm_t * algorithm);
static int64 compress_read(zip_source_t *, struct context *, void *, uint64);

static zip_compression_algorithm_t * get_algorithm(int32 method, bool compress) 
{
	const uint16 real_method = ZIP_CM_ACTUAL(method);
	for(size_t i = 0; i < implementations_size; i++) {
		if(implementations[i].method == real_method) {
			if(compress) {
				return implementations[i].compress;
			}
			else {
				return implementations[i].decompress;
			}
		}
	}
	return NULL;
}

ZIP_EXTERN int zip_compression_method_supported(int32 method, int compress) 
{
	return (method == ZIP_CM_STORE) ? 1 : BIN(get_algorithm(method, LOGIC(compress)));
}

zip_source_t * zip_source_compress(zip_t * za, zip_source_t * src, int32 method, int compression_flags) 
{
	return compression_source_new(za, src, method, true, compression_flags);
}

zip_source_t * zip_source_decompress(zip_t * za, zip_source_t * src, int32 method) 
{
	return compression_source_new(za, src, method, false, 0);
}

static zip_source_t * compression_source_new(zip_t * za, zip_source_t * src, int32 method, bool compress, int compression_flags) 
{
	struct context * ctx;
	zip_source_t * s2;
	zip_compression_algorithm_t * algorithm = NULL;
	if(src == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((algorithm = get_algorithm(method, compress)) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
		return NULL;
	}
	if((ctx = context_new(method, compress, compression_flags, algorithm)) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if((s2 = zip_source_layered(za, src, compress_callback, ctx)) == NULL) {
		context_free(ctx);
		return NULL;
	}
	return s2;
}

static struct context * context_new(int32 method, bool compress, int compression_flags, zip_compression_algorithm_t * algorithm) 
{
	struct context * ctx;
	if((ctx = (struct context *)SAlloc::M(sizeof(*ctx))) == NULL) {
		return NULL;
	}
	zip_error_init(&ctx->error);
	ctx->can_store = compress ? ZIP_CM_IS_DEFAULT(method) : false;
	ctx->algorithm = algorithm;
	ctx->method = method;
	ctx->compress = compress;
	ctx->end_of_input = false;
	ctx->end_of_stream = false;
	ctx->is_stored = false;
	if((ctx->ud = ctx->algorithm->allocate(ZIP_CM_ACTUAL(method), compression_flags, &ctx->error)) == NULL) {
		zip_error_fini(&ctx->error);
		SAlloc::F(ctx);
		return NULL;
	}

	return ctx;
}

static void context_free(struct context * ctx) 
{
	if(ctx) {
		ctx->algorithm->deallocate(ctx->ud);
		zip_error_fini(&ctx->error);
		SAlloc::F(ctx);
	}
}

static int64 compress_read(zip_source_t * src, struct context * ctx, void * data, uint64 len) 
{
	zip_compression_status_t ret;
	bool end;
	int64 n;
	uint64 out_offset;
	uint64 out_len;
	if(zip_error_code_zip(&ctx->error) != SLERR_SUCCESS) {
		return -1;
	}
	if(len == 0 || ctx->end_of_stream) {
		return 0;
	}
	out_offset = 0;
	end = false;
	while(!end && out_offset < len) {
		out_len = len - out_offset;
		ret = ctx->algorithm->process(ctx->ud, (uint8 *)data + out_offset, &out_len);
		if(ret != ZIP_COMPRESSION_ERROR) {
			out_offset += out_len;
		}
		switch(ret) {
			case ZIP_COMPRESSION_END:
			    ctx->end_of_stream = true;
			    if(!ctx->end_of_input) {
				    /* TODO: garbage after stream, or compression ended before all data read */
			    }
			    if(ctx->first_read < 0) {
				    /* we got end of processed stream before reading any input data */
				    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
				    end = true;
				    break;
			    }
			    if(ctx->can_store && (uint64)ctx->first_read <= out_offset) {
				    ctx->is_stored = true;
				    ctx->size = (uint64)ctx->first_read;
				    memcpy(data, ctx->buffer, static_cast<size_t>(ctx->size));
				    return (int64)ctx->size;
			    }
			    end = true;
			    break;
			case ZIP_COMPRESSION_OK:
			    break;
			case ZIP_COMPRESSION_NEED_DATA:
			    if(ctx->end_of_input) {
				    /* TODO: error: stream not ended, but no more input */
				    end = true;
				    break;
			    }
			    if((n = zip_source_read(src, ctx->buffer, sizeof(ctx->buffer))) < 0) {
				    _zip_error_set_from_source(&ctx->error, src);
				    end = true;
				    break;
			    }
			    else if(n == 0) {
				    ctx->end_of_input = true;
				    ctx->algorithm->end_of_input(ctx->ud);
				    if(ctx->first_read < 0) {
					    ctx->first_read = 0;
				    }
			    }
			    else {
				    if(ctx->first_read >= 0) {
					    /* we overwrote a previously filled ctx->buffer */
					    ctx->can_store = false;
				    }
				    else {
					    ctx->first_read = n;
				    }
				    ctx->algorithm->input(ctx->ud, ctx->buffer, (uint64)n);
			    }
			    break;

			case ZIP_COMPRESSION_ERROR:
			    /* error set by algorithm */
			    if(zip_error_code_zip(&ctx->error) == SLERR_SUCCESS) {
				    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
			    }
			    end = true;
			    break;
		}
	}
	if(out_offset > 0) {
		ctx->can_store = false;
		ctx->size += out_offset;
		return (int64)out_offset;
	}
	return (zip_error_code_zip(&ctx->error) == SLERR_SUCCESS) ? 0 : -1;
}

static int64 compress_callback(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	struct context * ctx = (struct context *)ud;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    ctx->size = 0;
		    ctx->end_of_input = false;
		    ctx->end_of_stream = false;
		    ctx->is_stored = false;
		    ctx->first_read = -1;
		    return ctx->algorithm->start(ctx->ud) ? 0 : -1;
		case ZIP_SOURCE_READ: return compress_read(src, ctx, data, len);
		case ZIP_SOURCE_CLOSE: return ctx->algorithm->end(ctx->ud) ? 0 : -1;
		case ZIP_SOURCE_STAT: {
		    zip_stat_t * st = static_cast<zip_stat_t *>(data);
		    if(ctx->compress) {
			    if(ctx->end_of_stream) {
				    st->comp_method = ctx->is_stored ? ZIP_CM_STORE : ZIP_CM_ACTUAL(ctx->method);
				    st->comp_size = ctx->size;
				    st->valid |= ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD;
			    }
			    else {
				    st->valid &= ~(ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD);
			    }
		    }
		    else {
			    st->comp_method = ZIP_CM_STORE;
			    st->valid |= ZIP_STAT_COMP_METHOD;
			    if(ctx->end_of_stream) {
				    st->size = ctx->size;
				    st->valid |= ZIP_STAT_SIZE;
			    }
			    else {
				    st->valid &= ~ZIP_STAT_SIZE;
			    }
		    }
	    }
		    return 0;
		case ZIP_SOURCE_ERROR: return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE: context_free(ctx); return 0;
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES: {
		    zip_file_attributes_t * attributes = (zip_file_attributes_t*)data;
		    if(len < sizeof(*attributes)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    attributes->valid |= ZIP_FILE_ATTRIBUTES_VERSION_NEEDED | ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS;
		    attributes->version_needed = ctx->algorithm->version_needed;
		    attributes->general_purpose_bit_mask = ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS_ALLOWED_MASK;
		    attributes->general_purpose_bit_flags = (ctx->is_stored ? 0 : ctx->algorithm->general_purpose_bit_flags(ctx->ud));
		    return sizeof(*attributes);
	    }
		case ZIP_SOURCE_SUPPORTS:
		    return ZIP_SOURCE_SUPPORTS_READABLE | zip_source_make_command_bitmap(ZIP_SOURCE_GET_FILE_ATTRIBUTES, -1);
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
		    return -1;
	}
}
//
// zip-source-file-common
//
static int64 read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd);

static void zip_source_file_stat_init(zip_source_file_stat_t * st) 
{
	st->size = 0;
	st->mtime = time(NULL);
	st->exists = false;
	st->regular_file = false;
}

zip_source_t * zip_source_file_common_new(const char * fname, void * file, uint64 start, int64 len,
    const zip_stat_t * st, zip_source_file_operations_t * ops, void * ops_userdata, zip_error_t * error) 
{
	zip_source_file_context_t * ctx;
	zip_source_t * zs;
	zip_source_file_stat_t sb;
	if(!ops) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(!ops->close || !ops->read || !ops->seek || !ops->stat) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return NULL;
	}
	if(ops->write && (!ops->commit_write || !ops->create_temp_output || !ops->remove || !ops->rollback_write || !ops->tell)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return NULL;
	}
	if(fname) {
		if(!ops->open || !ops->string_duplicate) {
			zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
			return NULL;
		}
	}
	else if(!file) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(len < 0) {
		len = 0;
	}
	if(start > ZIP_INT64_MAX || start + (uint64)len < start) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	ctx = (zip_source_file_context_t*)SAlloc::M(sizeof(zip_source_file_context_t));
	if(!ctx) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->ops = ops;
	ctx->ops_userdata = ops_userdata;
	ctx->fname = NULL;
	if(fname) {
		ctx->fname = ops->string_duplicate(ctx, fname);
		if(!ctx->fname) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			SAlloc::F(ctx);
			return NULL;
		}
	}
	ctx->f = file;
	ctx->start = start;
	ctx->len = (uint64)len;
	if(st) {
		memcpy(&ctx->st, st, sizeof(ctx->st));
		ctx->st.name = NULL;
		ctx->st.valid &= ~ZIP_STAT_NAME;
	}
	else {
		zip_stat_init(&ctx->st);
	}
	if(ctx->len > 0) {
		ctx->st.size = ctx->len;
		ctx->st.valid |= ZIP_STAT_SIZE;
	}
	zip_error_init(&ctx->stat_error);
	ctx->tmpname = NULL;
	ctx->fout = NULL;
	zip_error_init(&ctx->error);
	zip_file_attributes_init(&ctx->attributes);
	ctx->supports = ZIP_SOURCE_SUPPORTS_READABLE | zip_source_make_command_bitmap(ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1);
	zip_source_file_stat_init(&sb);
	if(!ops->stat(ctx, &sb)) {
		_zip_error_copy(error, &ctx->error);
		SAlloc::F(ctx->fname);
		SAlloc::F(ctx);
		return NULL;
	}
	if(!sb.exists) {
		if(ctx->fname && ctx->start == 0 && ctx->len == 0 && ops->write != NULL) {
			ctx->supports = ZIP_SOURCE_SUPPORTS_WRITABLE;
			// zip_open_from_source checks for this to detect non-existing files 
			zip_error_set(&ctx->stat_error, SLERR_ZIP_READ, ENOENT);
		}
		else {
			zip_error_set(&ctx->stat_error, SLERR_ZIP_READ, ENOENT);
			SAlloc::F(ctx->fname);
			SAlloc::F(ctx);
			return NULL;
		}
	}
	else {
		if((ctx->st.valid & ZIP_STAT_MTIME) == 0) {
			ctx->st.mtime = sb.mtime;
			ctx->st.valid |= ZIP_STAT_MTIME;
		}
		if(sb.regular_file) {
			ctx->supports = ZIP_SOURCE_SUPPORTS_SEEKABLE;
			if(ctx->start + ctx->len > sb.size) {
				zip_error_set(error, SLERR_ZIP_INVAL, 0);
				SAlloc::F(ctx->fname);
				SAlloc::F(ctx);
				return NULL;
			}
			if(ctx->len == 0) {
				ctx->len = sb.size - ctx->start;
				ctx->st.size = ctx->len;
				ctx->st.valid |= ZIP_STAT_SIZE;
				/* when using a partial file, don't allow writing */
				if(ctx->fname && start == 0 && ops->write != NULL) {
					ctx->supports = ZIP_SOURCE_SUPPORTS_WRITABLE;
				}
			}
		}
		ctx->supports |= ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_GET_FILE_ATTRIBUTES);
	}
	ctx->supports |= ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_ACCEPT_EMPTY);
	if(ops->create_temp_output_cloning != NULL) {
		if(ctx->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_BEGIN_WRITE)) {
			ctx->supports |= ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_BEGIN_WRITE_CLONING);
		}
	}
	if((zs = zip_source_function_create(read_file, ctx, error)) == NULL) {
		SAlloc::F(ctx->fname);
		SAlloc::F(ctx);
		return NULL;
	}
	return zs;
}

static int64 read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	zip_source_file_context_t * ctx = static_cast<zip_source_file_context_t *>(state);
	char * buf = static_cast<char *>(data);
	switch(cmd) {
		case ZIP_SOURCE_ACCEPT_EMPTY:
		    return 0;
		case ZIP_SOURCE_BEGIN_WRITE:
		    // write support should not be set if fname is NULL 
		    if(!ctx->fname) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
			    return -1;
		    }
			else
				return ctx->ops->create_temp_output(ctx);
		case ZIP_SOURCE_BEGIN_WRITE_CLONING:
		    // write support should not be set if fname is NULL 
		    if(!ctx->fname) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
			    return -1;
		    }
			else
				return ctx->ops->create_temp_output_cloning(ctx, len);
		case ZIP_SOURCE_CLOSE:
		    if(ctx->fname) {
			    ctx->ops->close(ctx);
			    ctx->f = NULL;
		    }
		    return 0;
		case ZIP_SOURCE_COMMIT_WRITE: 
			{
				int64 ret = ctx->ops->commit_write(ctx);
				ctx->fout = NULL;
				if(ret == 0) {
					SAlloc::F(ctx->tmpname);
					ctx->tmpname = NULL;
				}
				return ret;
			}
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx->fname);
		    SAlloc::F(ctx->tmpname);
		    if(ctx->f)
			    ctx->ops->close(ctx);
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES:
		    if(len < sizeof(ctx->attributes)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
			else {
				memcpy(data, &ctx->attributes, sizeof(ctx->attributes));
				return sizeof(ctx->attributes);
			}
		case ZIP_SOURCE_OPEN:
		    if(ctx->fname) {
			    if(ctx->ops->open(ctx) == false) {
				    return -1;
			    }
		    }
		    if(ctx->start > 0) { // TODO: rewind on re-open
			    if(ctx->ops->seek(ctx, ctx->f, (int64)ctx->start, SEEK_SET) == false) {
				    // TODO: skip by reading 
				    return -1;
			    }
		    }
		    ctx->offset = 0;
		    return 0;
		case ZIP_SOURCE_READ: 
			{
				int64 i;
				uint64 n;
				if(ctx->len > 0) {
					n = MIN(ctx->len - ctx->offset, len);
				}
				else {
					n = len;
				}
				if((i = ctx->ops->read(ctx, buf, n)) < 0) {
					zip_error_set(&ctx->error, SLERR_ZIP_READ, errno);
					return -1;
				}
				ctx->offset += (uint64)i;
				return i;
			}
		case ZIP_SOURCE_REMOVE: return ctx->ops->remove(ctx);
		case ZIP_SOURCE_ROLLBACK_WRITE:
		    ctx->ops->rollback_write(ctx);
		    ctx->fout = NULL;
		    SAlloc::F(ctx->tmpname);
		    ctx->tmpname = NULL;
		    return 0;
		case ZIP_SOURCE_SEEK: {
		    int64 new_offset = zip_source_seek_compute_offset(ctx->offset, ctx->len, data, len, &ctx->error);
		    if(new_offset < 0) {
			    return -1;
		    }
		    // The actual offset inside the file must be representable as int64. 
		    if(new_offset > ZIP_INT64_MAX - (int64)ctx->start) {
			    zip_error_set(&ctx->error, SLERR_ZIP_SEEK, EOVERFLOW);
			    return -1;
		    }
		    ctx->offset = (uint64)new_offset;
		    if(ctx->ops->seek(ctx, ctx->f, (int64)(ctx->offset + ctx->start), SEEK_SET) == false) {
			    return -1;
		    }
		    return 0;
	    }
		case ZIP_SOURCE_SEEK_WRITE: 
			{
				zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
				if(!args)
					return -1;
				else if(ctx->ops->seek(ctx, ctx->fout, args->offset, args->whence) == false)
					return -1;
				else
					return 0;
			}
		case ZIP_SOURCE_STAT: 
			{
				if(len < sizeof(ctx->st))
					return -1;
				else if(zip_error_code_zip(&ctx->stat_error) != 0) {
					zip_error_set(&ctx->error, zip_error_code_zip(&ctx->stat_error), zip_error_code_system(&ctx->stat_error));
					return -1;
				}
				else {
					memcpy(data, &ctx->st, sizeof(ctx->st));
					return sizeof(ctx->st);
				}
			}
		case ZIP_SOURCE_SUPPORTS: return ctx->supports;
		case ZIP_SOURCE_TELL: return (int64)ctx->offset;
		case ZIP_SOURCE_TELL_WRITE: return ctx->ops->tell(ctx, ctx->fout);
		case ZIP_SOURCE_WRITE: return ctx->ops->write(ctx, data, len);
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
		    return -1;
	}
}
//
// zip-source-file-stdio
// read-only stdio file source implementation
//
#ifdef _WIN32
	#ifndef S_IWUSR
		#define S_IWUSR _S_IWRITE
	#endif
#endif

// clang-format off 
static zip_source_file_operations_t ops_stdio_read = {
	_zip_stdio_op_close,
	NULL,
	NULL,
	NULL,
	NULL,
	_zip_stdio_op_read,
	NULL,
	NULL,
	_zip_stdio_op_seek,
	_zip_stdio_op_stat,
	NULL,
	_zip_stdio_op_tell,
	NULL
};
/* clang-format on */

ZIP_EXTERN zip_source_t * zip_source_filep(zip_t * za, FILE * file, uint64 start, int64 len) 
{
	return za ? zip_source_filep_create(file, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_filep_create(FILE * file, uint64 start, int64 length, zip_error_t * error) 
{
	if(!file || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else
		return zip_source_file_common_new(NULL, file, start, length, NULL, &ops_stdio_read, NULL, error);
}

void _zip_stdio_op_close(zip_source_file_context_t * ctx) 
{
	fclose((FILE*)ctx->f);
}

int64 _zip_stdio_op_read(zip_source_file_context_t * ctx, void * buf, uint64 len) 
{
	SETMIN(len, SIZE_MAX);
	size_t i = fread(buf, 1, (size_t)len, static_cast<FILE *>(ctx->f));
	if(!i) {
		if(ferror((FILE*)ctx->f)) {
			zip_error_set(&ctx->error, SLERR_ZIP_READ, errno);
			return -1;
		}
	}
	return (int64)i;
}

bool _zip_stdio_op_seek(zip_source_file_context_t * ctx, void * f, int64 offset, int whence) 
{
#if ZIP_FSEEK_MAX > ZIP_INT64_MAX
	if(offset > ZIP_FSEEK_MAX || offset < ZIP_FSEEK_MIN) {
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, EOVERFLOW);
		return false;
	}
#endif
	if(fseeko((FILE*)f, (off_t)offset, whence) < 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, errno);
		return false;
	}
	return true;
}

bool _zip_stdio_op_stat(zip_source_file_context_t * ctx, zip_source_file_stat_t * st) 
{
	struct stat sb;
	int ret = ctx->fname ? stat(ctx->fname, &sb) : fstat(_fileno((FILE*)ctx->f), &sb);
	if(ret < 0) {
		if(errno == ENOENT) {
			st->exists = false;
			return true;
		}
		else {
			zip_error_set(&ctx->error, SLERR_ZIP_READ, errno);
			return false;
		}
	}
	else {
		st->size = (uint64)sb.st_size;
		st->mtime = sb.st_mtime;
		st->regular_file = S_ISREG(sb.st_mode);
		st->exists = true;
		// We're using UNIX file API, even on Windows; thus, we supply external file attributes with Unix values. 
		// TODO: This could be improved on Windows by providing Windows-specific file attributes 
		ctx->attributes.valid = ZIP_FILE_ATTRIBUTES_HOST_SYSTEM | ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES;
		ctx->attributes.host_system = ZIP_OPSYS_UNIX;
		ctx->attributes.external_file_attributes = (((uint32)sb.st_mode) << 16) | ((sb.st_mode & S_IWUSR) ? 0 : 1);
		return true;
	}
}

int64 _zip_stdio_op_tell(zip_source_file_context_t * ctx, void * f) 
{
	const off_t offset = ftello((FILE*)f);
	if(offset < 0)
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, errno);
	return offset;
}
// 
// fopen replacement that sets the close-on-exec flag
// some implementations support an fopen 'e' flag for that, but e.g. macOS doesn't.
// 
FILE * _zip_fopen_close_on_exec(const char * name, bool writeable) 
{
	int flags = O_CLOEXEC;
	if(writeable)
		flags |= O_RDWR;
	else
		flags |= O_RDONLY;
	// mode argument needed on Windows 
	int fd = _open(name, flags, 0666);
	return (fd >= 0) ? _fdopen(fd, writeable ? "r+b" : "rb") : 0;
}
//
// zip-source-file-stdio-named
// source for stdio file opened by name
//
static int64 _zip_stdio_op_commit_write(zip_source_file_context_t * ctx);
static int64 _zip_stdio_op_create_temp_output(zip_source_file_context_t * ctx);
#ifdef CAN_CLONE
	static int64 _zip_stdio_op_create_temp_output_cloning(zip_source_file_context_t * ctx, uint64 offset);
#endif
static bool _zip_stdio_op_open(zip_source_file_context_t * ctx);
static int64 _zip_stdio_op_remove(zip_source_file_context_t * ctx);
static void _zip_stdio_op_rollback_write(zip_source_file_context_t * ctx);
static char * _zip_stdio_op_strdup(zip_source_file_context_t * ctx, const char * string);
static int64 _zip_stdio_op_write(zip_source_file_context_t * ctx, const void * data, uint64 len);

// clang-format off 
static zip_source_file_operations_t ops_stdio_named = {
	_zip_stdio_op_close,
	_zip_stdio_op_commit_write,
	_zip_stdio_op_create_temp_output,
#ifdef CAN_CLONE
	_zip_stdio_op_create_temp_output_cloning,
#else
	NULL,
#endif
	_zip_stdio_op_open,
	_zip_stdio_op_read,
	_zip_stdio_op_remove,
	_zip_stdio_op_rollback_write,
	_zip_stdio_op_seek,
	_zip_stdio_op_stat,
	_zip_stdio_op_strdup,
	_zip_stdio_op_tell,
	_zip_stdio_op_write
};
/* clang-format on */

ZIP_EXTERN zip_source_t * zip_source_file_create(const char * fname, uint64 start, int64 length, zip_error_t * error) 
{
	if(!fname || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else
		return zip_source_file_common_new(fname, NULL, start, length, NULL, &ops_stdio_named, NULL, error);
}

ZIP_EXTERN zip_source_t * zip_source_file(zip_t * za, const char * fname, uint64 start, int64 len) 
{
	return za ? zip_source_file_create(fname, start, len, &za->error) : 0;
}

static int64 _zip_stdio_op_commit_write(zip_source_file_context_t * ctx) 
{
	if(fclose(static_cast<FILE *>(ctx->fout)) < 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_WRITE, errno);
		return -1;
	}
	if(rename(ctx->tmpname, ctx->fname) < 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_RENAME, errno);
		return -1;
	}
	return 0;
}

static int64 _zip_stdio_op_create_temp_output(zip_source_file_context_t * ctx) 
{
	int tfd;
	int mode;
	FILE * tfp;
	struct stat st;
	char * temp = static_cast<char *>(SAlloc::M(strlen(ctx->fname) + 8));
	if(!temp) {
		zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	if(stat(ctx->fname, &st) == 0)
		mode = st.st_mode;
	else
		mode = -1;
	sprintf(temp, "%s.XXXXXX", ctx->fname);
	if((tfd = _zip_mkstempm(temp, mode)) == -1) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		SAlloc::F(temp);
		return -1;
	}
	if((tfp = fdopen(tfd, "r+b")) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		close(tfd);
		(void)remove(temp);
		SAlloc::F(temp);
		return -1;
	}
	ctx->fout = tfp;
	ctx->tmpname = temp;
	return 0;
}

#ifdef CAN_CLONE
static int64 _zip_stdio_op_create_temp_output_cloning(zip_source_file_context_t * ctx, uint64 offset) 
{
	char * temp;
	FILE * tfp;
	if(offset > ZIP_OFF_MAX) {
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, E2BIG);
		return -1;
	}
	if((temp = (char *)SAlloc::M(strlen(ctx->fname) + 8)) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	sprintf(temp, "%s.XXXXXX", ctx->fname);
#ifdef HAVE_CLONEFILE
#ifndef __clang_analyzer__
	/* we can't use mkstemp, since clonefile insists on creating the file */
	if(mktemp(temp) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		SAlloc::F(temp);
		return -1;
	}
#endif
	if(clonefile(ctx->fname, temp, 0) < 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		SAlloc::F(temp);
		return -1;
	}
	if((tfp = _zip_fopen_close_on_exec(temp, true)) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		(void)remove(temp);
		SAlloc::F(temp);
		return -1;
	}
#else
	{
		int fd;
		struct file_clone_range range;
		struct stat st;
		if(fstat(fileno(ctx->f), &st) < 0) {
			zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
			SAlloc::F(temp);
			return -1;
		}
		if((fd = mkstemp(temp)) < 0) {
			zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
			SAlloc::F(temp);
			return -1;
		}
		range.src_fd = fileno(ctx->f);
		range.src_offset = 0;
		range.src_length = ((offset + st.st_blksize - 1) / st.st_blksize) * st.st_blksize;
		if(range.src_length > st.st_size) {
			range.src_length = 0;
		}
		range.dest_offset = 0;
		if(ioctl(fd, FICLONERANGE, &range) < 0) {
			zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
			(void)close(fd);
			(void)remove(temp);
			SAlloc::F(temp);
			return -1;
		}
		if((tfp = fdopen(fd, "r+b")) == NULL) {
			zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
			(void)close(fd);
			(void)remove(temp);
			SAlloc::F(temp);
			return -1;
		}
	}
#endif
	if(ftruncate(fileno(tfp), (off_t)offset) < 0) {
		(void)fclose(tfp);
		(void)remove(temp);
		SAlloc::F(temp);
		return -1;
	}
	if(fseeko(tfp, (off_t)offset, SEEK_SET) < 0) {
		(void)fclose(tfp);
		(void)remove(temp);
		SAlloc::F(temp);
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
	}
	ctx->fout = tfp;
	ctx->tmpname = temp;
	return 0;
}
#endif

static bool _zip_stdio_op_open(zip_source_file_context_t * ctx) 
{
	if((ctx->f = _zip_fopen_close_on_exec(ctx->fname, false)) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_OPEN, errno);
		return false;
	}
	else
		return true;
}

static int64 _zip_stdio_op_remove(zip_source_file_context_t * ctx) 
{
	if(remove(ctx->fname) < 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_REMOVE, errno);
		return -1;
	}
	else
		return 0;
}

static void _zip_stdio_op_rollback_write(zip_source_file_context_t * ctx) 
{
	if(ctx->fout)
		fclose(static_cast<FILE *>(ctx->fout));
	remove(ctx->tmpname);
}

static char * _zip_stdio_op_strdup(zip_source_file_context_t * ctx, const char * string) 
{
	return strdup(string);
}

static int64 _zip_stdio_op_write(zip_source_file_context_t * ctx, const void * data, uint64 len) 
{
	clearerr((FILE*)ctx->fout);
	size_t ret = fwrite(data, 1, static_cast<size_t>(len), (FILE*)ctx->fout);
	if(ret != len || ferror((FILE*)ctx->fout)) {
		zip_error_set(&ctx->error, SLERR_ZIP_WRITE, errno);
		return -1;
	}
	else
		return (int64)ret;
}
//
// zip-source-file-win32
// read-only Windows file source implementation
//
static bool _zip_win32_op_stat(zip_source_file_context_t * ctx, zip_source_file_stat_t * st);
static bool _zip_stat_win32(zip_source_file_context_t * ctx, zip_source_file_stat_t * st, HANDLE h);

static zip_source_file_operations_t ops_win32_read = {
	_zip_win32_op_close,
	NULL,
	NULL,
	NULL,
	NULL,
	_zip_win32_op_read,
	NULL,
	NULL,
	_zip_win32_op_seek,
	_zip_win32_op_stat,
	NULL,
	_zip_win32_op_tell,
	NULL
};

ZIP_EXTERN zip_source_t * zip_source_win32handle(zip_t * za, HANDLE h, uint64 start, int64 len) 
{
	return za ? zip_source_win32handle_create(h, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_win32handle_create(HANDLE h, uint64 start, int64 length, zip_error_t * error) 
{
	if(h == INVALID_HANDLE_VALUE || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else
		return zip_source_file_common_new(NULL, h, start, length, NULL, &ops_win32_read, NULL, error);
}

void _zip_win32_op_close(zip_source_file_context_t * ctx) 
{
	CloseHandle((HANDLE)ctx->f);
}

int64 _zip_win32_op_read(zip_source_file_context_t * ctx, void * buf, uint64 len) 
{
	DWORD i;
	// TODO: cap len to "DWORD_MAX" 
	if(!ReadFile((HANDLE)ctx->f, buf, (DWORD)len, &i, NULL)) {
		zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	return (int64)i;
}

bool _zip_win32_op_seek(zip_source_file_context_t * ctx, void * f, int64 offset, int whence) 
{
	LARGE_INTEGER li;
	DWORD method;
	switch(whence) {
		case SEEK_SET: method = FILE_BEGIN; break;
		case SEEK_END: method = FILE_END; break;
		case SEEK_CUR: method = FILE_CURRENT; break;
		default: 
			zip_error_set(&ctx->error, SLERR_ZIP_SEEK, EINVAL); 
			return false; // @sobolev return -1 --> return false
	}
	li.QuadPart = (LONGLONG)offset;
	if(!SetFilePointerEx((HANDLE)f, li, NULL, method)) {
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, _zip_win32_error_to_errno(GetLastError()));
		return false;
	}
	return true;
}

static bool _zip_win32_op_stat(zip_source_file_context_t * ctx, zip_source_file_stat_t * st) 
{
	return _zip_stat_win32(ctx, st, (HANDLE)ctx->f);
}

int64 _zip_win32_op_tell(zip_source_file_context_t * ctx, void * f) 
{
	LARGE_INTEGER zero;
	LARGE_INTEGER new_offset;
	zero.QuadPart = 0;
	if(!SetFilePointerEx((HANDLE)f, zero, &new_offset, FILE_CURRENT)) {
		zip_error_set(&ctx->error, SLERR_ZIP_SEEK, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	return (int64)new_offset.QuadPart;
}

int _zip_win32_error_to_errno(DWORD win32err) 
{
	/* Note: This list isn't exhaustive, but should cover common cases. */
	switch(win32err) {
		case ERROR_INVALID_PARAMETER: return EINVAL;
		case ERROR_FILE_NOT_FOUND: return ENOENT;
		case ERROR_INVALID_HANDLE: return EBADF;
		case ERROR_ACCESS_DENIED: return EACCES;
		case ERROR_FILE_EXISTS: return EEXIST;
		case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
		case ERROR_DISK_FULL: return ENOSPC;
		default: return 10000 + win32err;
	}
}

static bool _zip_stat_win32(zip_source_file_context_t * ctx, zip_source_file_stat_t * st, HANDLE h) 
{
	FILETIME mtimeft;
	time_t mtime;
	if(!GetFileTime(h, NULL, NULL, &mtimeft)) {
		zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
		return false;
	}
	else if(!_zip_filetime_to_time_t(mtimeft, &mtime)) {
		zip_error_set(&ctx->error, SLERR_ZIP_READ, ERANGE);
		return false;
	}
	else {
		LARGE_INTEGER size;
		st->exists = true;
		st->mtime = mtime;
		if(GetFileType(h) == FILE_TYPE_DISK) {
			st->regular_file = 1;
			if(!GetFileSizeEx(h, &size)) {
				zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
				return false;
			}
			st->size = (uint64)size.QuadPart;
		}
		// TODO: fill in ctx->attributes 
		return true;
	}
}

bool _zip_filetime_to_time_t(FILETIME ft, time_t * t) 
{
	//
	// Inspired by http://stackoverflow.com/questions/6161776/convert-windows-filetime-to-second-in-unix-linux
	//
	const int64 WINDOWS_TICK = 10000000LL;
	const int64 SEC_TO_UNIX_EPOCH = 11644473600LL;
	ULARGE_INTEGER li;
	int64 secs;
	time_t temp;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	secs = (li.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
	temp = (time_t)secs;
	if(secs != (int64)temp) {
		return false;
	}
	else {
		*t = temp;
		return true;
	}
}
//
// zip-source-file-win32-ansi
// source for Windows file opened by ANSI name
//
static char * ansi_allocate_tempname(const char * name, size_t extra_chars, size_t * lengthp);
static void ansi_make_tempname(char * buf, size_t len, const char * name, uint32 i);

zip_win32_file_operations_t ops_ansi = {
	ansi_allocate_tempname,
	reinterpret_cast<HANDLE (__stdcall *)(const void *,DWORD,DWORD,PSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)>(CreateFileA),
	reinterpret_cast<BOOL (__stdcall *)(const void *)>(DeleteFileA),
	reinterpret_cast<DWORD (__stdcall *)(const void *)>(GetFileAttributesA),
	reinterpret_cast<BOOL (__stdcall *)(const void *,GET_FILEEX_INFO_LEVELS,void *)>(GetFileAttributesExA),
	ansi_make_tempname,
	reinterpret_cast<BOOL (__stdcall *)(const void *,const void *,DWORD)>(MoveFileExA),
	reinterpret_cast<BOOL (__stdcall *)(const void *,DWORD)>(SetFileAttributesA),
	strdup
};

ZIP_EXTERN zip_source_t * zip_source_win32a(zip_t * za, const char * fname, uint64 start, int64 len) 
{
	return za ? zip_source_win32a_create(fname, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_win32a_create(const char * fname, uint64 start, int64 length, zip_error_t * error) 
{
	if(!fname || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	return zip_source_file_common_new(fname, NULL, start, length, NULL, &_zip_source_file_win32_named_ops, &ops_ansi, error);
}

static char * ansi_allocate_tempname(const char * name, size_t extra_chars, size_t * lengthp) 
{
	*lengthp = strlen(name) + extra_chars;
	return static_cast<char *>(SAlloc::M(*lengthp));
}

static void ansi_make_tempname(char * buf, size_t len, const char * name, uint32 i) 
{
	_snprintf(buf, len, "%s.%08x", name, i);
}
//
// zip-source-file-win32-utf8
// source for Windows file opened by UTF-8 name
//
ZIP_EXTERN zip_source_t * zip_source_file_create_utf8(const char * fname, uint64 start, int64 length, zip_error_t * error) 
{
	zip_source_t * source = 0;
	if(!fname || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	else {
		// Convert fname from UTF-8 to Windows-friendly UTF-16
		const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fname, -1, NULL, 0);
		if(!size)
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
		else {
			wchar_t * wfname = static_cast<wchar_t *>(SAlloc::M(sizeof(wchar_t) * size));
			if(!wfname)
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			else {
				MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fname, -1, wfname, size);
				source = zip_source_win32w_create(wfname, start, length, error);
				SAlloc::F(wfname);
			}
		}
	}
	return source;
}

ZIP_EXTERN zip_source_t * zip_source_file_utf8(zip_t * za, const char * fname, uint64 start, int64 len) 
{
	return za ? zip_source_file_create(fname, start, len, &za->error) : 0;
}
//
// zip-source-file-win32-utf16
// source for Windows file opened by UTF-16 name
//
static char * utf16_allocate_tempname(const char * name, size_t extra_chars, size_t * lengthp) 
{
	*lengthp = wcslen((const wchar_t*)name) + extra_chars;
	return (char *)SAlloc::M(*lengthp * sizeof(wchar_t));
}

static void utf16_make_tempname(char * buf, size_t len, const char * name, uint32 i) 
{
	_snwprintf((wchar_t*)buf, len, L"%s.%08x", (const wchar_t*)name, i);
}

static char * utf16_strdup(const char * string) 
{
	return (char *)_wcsdup((const wchar_t*)string);
}

static HANDLE __stdcall utf16_create_file(const char * name, DWORD access, DWORD share_mode,
    PSECURITY_ATTRIBUTES security_attributes, DWORD creation_disposition, DWORD file_attributes, HANDLE template_file) 
{
#ifdef MS_UWP
	CREATEFILE2_EXTENDED_PARAMETERS extParams = {0};
	extParams.dwFileAttributes = file_attributes;
	extParams.dwFileFlags = FILE_FLAG_RANDOM_ACCESS;
	extParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extParams.dwSize = sizeof(extParams);
	extParams.hTemplateFile = template_file;
	extParams.lpSecurityAttributes = security_attributes;
	return CreateFile2((const wchar_t*)name, access, share_mode, creation_disposition, &extParams);
#else
	return CreateFileW((const wchar_t*)name, access, share_mode, security_attributes, creation_disposition, file_attributes, template_file);
#endif
}

zip_win32_file_operations_t ops_utf16 = {
	utf16_allocate_tempname,
	reinterpret_cast<HANDLE (__stdcall *)(const void *,DWORD,DWORD,PSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)>(utf16_create_file),
	reinterpret_cast<BOOL (__stdcall *)(const void *)>(DeleteFileW),
	reinterpret_cast<DWORD (__stdcall *)(const void *)>(GetFileAttributesW),
	reinterpret_cast<BOOL (__stdcall *)(const void *,GET_FILEEX_INFO_LEVELS,void *)>(GetFileAttributesExW),
	utf16_make_tempname,
	reinterpret_cast<BOOL (__stdcall *)(const void *,const void *,DWORD)>(MoveFileExW),
	reinterpret_cast<BOOL (__stdcall *)(const void *,DWORD)>(SetFileAttributesW),
	utf16_strdup
};

ZIP_EXTERN zip_source_t * zip_source_win32w(zip_t * za, const wchar_t * fname, uint64 start, int64 len) 
{
	return za ? zip_source_win32w_create(fname, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_win32w_create(const wchar_t * fname, uint64 start, int64 length, zip_error_t * error) 
{
	if(fname == NULL || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	return zip_source_file_common_new((const char*)fname, NULL, start, length, NULL, &_zip_source_file_win32_named_ops, &ops_utf16, error);
}
//
// zip-source-file-win32-named
// source for Windows file opened by name
//
static int64 _zip_win32_named_op_commit_write(zip_source_file_context_t * ctx);
static int64 _zip_win32_named_op_create_temp_output(zip_source_file_context_t * ctx);
static bool _zip_win32_named_op_open(zip_source_file_context_t * ctx);
static int64 _zip_win32_named_op_remove(zip_source_file_context_t * ctx);
static void _zip_win32_named_op_rollback_write(zip_source_file_context_t * ctx);
static bool _zip_win32_named_op_stat(zip_source_file_context_t * ctx, zip_source_file_stat_t * st);
static char * _zip_win32_named_op_string_duplicate(zip_source_file_context_t * ctx, const char * string);
static int64 _zip_win32_named_op_write(zip_source_file_context_t * ctx, const void * data, uint64 len);

static HANDLE win32_named_open(zip_source_file_context_t * ctx, const char * name, bool temporary, PSECURITY_ATTRIBUTES security_attributes);

/* clang-format off */
zip_source_file_operations_t _zip_source_file_win32_named_ops = {
	_zip_win32_op_close,
	_zip_win32_named_op_commit_write,
	_zip_win32_named_op_create_temp_output,
	NULL,
	_zip_win32_named_op_open,
	_zip_win32_op_read,
	_zip_win32_named_op_remove,
	_zip_win32_named_op_rollback_write,
	_zip_win32_op_seek,
	_zip_win32_named_op_stat,
	_zip_win32_named_op_string_duplicate,
	_zip_win32_op_tell,
	_zip_win32_named_op_write
};
/* clang-format on */

static int64 _zip_win32_named_op_commit_write(zip_source_file_context_t * ctx) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	if(!CloseHandle((HANDLE)ctx->fout)) {
		zip_error_set(&ctx->error, SLERR_ZIP_WRITE, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	DWORD attributes = file_ops->get_file_attributes(ctx->tmpname);
	if(attributes == INVALID_FILE_ATTRIBUTES) {
		zip_error_set(&ctx->error, SLERR_ZIP_RENAME, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	if(attributes & FILE_ATTRIBUTE_TEMPORARY) {
		if(!file_ops->set_file_attributes(ctx->tmpname, attributes & ~FILE_ATTRIBUTE_TEMPORARY)) {
			zip_error_set(&ctx->error, SLERR_ZIP_RENAME, _zip_win32_error_to_errno(GetLastError()));
			return -1;
		}
	}
	if(!file_ops->move_file(ctx->tmpname, ctx->fname, MOVEFILE_REPLACE_EXISTING)) {
		zip_error_set(&ctx->error, SLERR_ZIP_RENAME, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	return 0;
}

static int64 _zip_win32_named_op_create_temp_output(zip_source_file_context_t * ctx) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	uint32 value, i;
	HANDLE th = INVALID_HANDLE_VALUE;
	void * temp = NULL;
	PSECURITY_DESCRIPTOR psd = NULL;
	PSECURITY_ATTRIBUTES psa = NULL;
	SECURITY_ATTRIBUTES sa;
	SECURITY_INFORMATION si;
	DWORD success;
	PACL dacl = NULL;
	char * tempname = NULL;
	size_t tempname_size = 0;
	if((HANDLE)ctx->f != INVALID_HANDLE_VALUE && GetFileType((HANDLE)ctx->f) == FILE_TYPE_DISK) {
		si = DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION;
		success = GetSecurityInfo((HANDLE)ctx->f, SE_FILE_OBJECT, si, NULL, NULL, &dacl, NULL, &psd);
		if(success == ERROR_SUCCESS) {
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = psd;
			psa = &sa;
		}
	}
 #ifndef MS_UWP
	value = GetTickCount();
#else
	value = (uint32)(GetTickCount64() & 0xffffffff);
#endif
	if((tempname = file_ops->allocate_tempname(ctx->fname, 10, &tempname_size)) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	for(i = 0; i < 1024 && th == INVALID_HANDLE_VALUE; i++) {
		file_ops->make_tempname(tempname, tempname_size, ctx->fname, value + i);

		th = win32_named_open(ctx, tempname, true, psa);
		if(th == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS)
			break;
	}
	if(th == INVALID_HANDLE_VALUE) {
		SAlloc::F(tempname);
		LocalFree(psd);
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	LocalFree(psd);
	ctx->fout = th;
	ctx->tmpname = tempname;
	return 0;
}

static bool _zip_win32_named_op_open(zip_source_file_context_t * ctx) 
{
	HANDLE h = win32_named_open(ctx, ctx->fname, false, NULL);
	if(h == INVALID_HANDLE_VALUE) {
		return false;
	}
	ctx->f = h;
	return true;
}

static int64 _zip_win32_named_op_remove(zip_source_file_context_t * ctx) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	if(!file_ops->delete_file(ctx->fname)) {
		zip_error_set(&ctx->error, SLERR_ZIP_REMOVE, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	return 0;
}

static void _zip_win32_named_op_rollback_write(zip_source_file_context_t * ctx) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	if(ctx->fout) {
		CloseHandle((HANDLE)ctx->fout);
	}
	file_ops->delete_file(ctx->tmpname);
}

static bool _zip_win32_named_op_stat(zip_source_file_context_t * ctx, zip_source_file_stat_t * st) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	WIN32_FILE_ATTRIBUTE_DATA file_attributes;
	if(!file_ops->get_file_attributes_ex(ctx->fname, GetFileExInfoStandard, &file_attributes)) {
		DWORD error = GetLastError();
		if(error == ERROR_FILE_NOT_FOUND) {
			st->exists = false;
			return true;
		}
		zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(error));
		return false;
	}
	st->exists = true;
	st->regular_file = true; /* TODO: Is this always right? How to determine without a HANDLE? */
	if(!_zip_filetime_to_time_t(file_attributes.ftLastWriteTime, &st->mtime)) {
		zip_error_set(&ctx->error, SLERR_ZIP_READ, ERANGE);
		return false;
	}
	st->size = ((uint64)file_attributes.nFileSizeHigh << 32) | file_attributes.nFileSizeLow;
	// TODO: fill in ctx->attributes 
	return true;
}

static char * _zip_win32_named_op_string_duplicate(zip_source_file_context_t * ctx, const char * string) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	return file_ops->string_duplicate(string);
}

static int64 _zip_win32_named_op_write(zip_source_file_context_t * ctx, const void * data, uint64 len) 
{
	DWORD ret;
	if(!WriteFile((HANDLE)ctx->fout, data, (DWORD)len, &ret, NULL) || ret != len) {
		zip_error_set(&ctx->error, SLERR_ZIP_WRITE, _zip_win32_error_to_errno(GetLastError()));
		return -1;
	}
	else
		return (int64)ret;
}

static HANDLE win32_named_open(zip_source_file_context_t * ctx, const char * name, bool temporary, PSECURITY_ATTRIBUTES security_attributes) 
{
	zip_win32_file_operations_t * file_ops = (zip_win32_file_operations_t*)ctx->ops_userdata;
	DWORD access = GENERIC_READ;
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD creation_disposition = OPEN_EXISTING;
	DWORD file_attributes = FILE_ATTRIBUTE_NORMAL;
	if(temporary) {
		access = GENERIC_READ | GENERIC_WRITE;
		share_mode = FILE_SHARE_READ;
		creation_disposition = CREATE_NEW;
		file_attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY;
	}
	HANDLE h = file_ops->create_file(name, access, share_mode, security_attributes, creation_disposition, file_attributes, NULL);
	if(h == INVALID_HANDLE_VALUE) {
		zip_error_set(&ctx->error, SLERR_ZIP_OPEN, _zip_win32_error_to_errno(GetLastError()));
	}
	return h;
}
//
// zip-pkware
// Traditional PKWARE de/encryption backend routines
//
#define PKWARE_KEY0 305419896
#define PKWARE_KEY1 591751049
#define PKWARE_KEY2 878082192

static void update_keys(zip_pkware_keys_t * keys, uint8 b) 
{
	keys->key[0] = (uint32)crc32(keys->key[0] ^ 0xffffffffUL, &b, 1) ^ 0xffffffffUL;
	keys->key[1] = (keys->key[1] + (keys->key[0] & 0xff)) * 134775813 + 1;
	b = (uint8)(keys->key[1] >> 24);
	keys->key[2] = (uint32)crc32(keys->key[2] ^ 0xffffffffUL, &b, 1) ^ 0xffffffffUL;
}

static uint8 crypt_byte(zip_pkware_keys_t * keys) 
{
	uint16 tmp = (uint16)(keys->key[2] | 2);
	tmp = (uint16)(((uint32)tmp * (tmp ^ 1)) >> 8);
	return (uint8)tmp;
}

void _zip_pkware_keys_reset(zip_pkware_keys_t * keys) 
{
	keys->key[0] = PKWARE_KEY0;
	keys->key[1] = PKWARE_KEY1;
	keys->key[2] = PKWARE_KEY2;
}

void _zip_pkware_encrypt(zip_pkware_keys_t * keys, uint8 * out, const uint8 * in, uint64 len) 
{
	for(uint64 i = 0; i < len; i++) {
		uint8 b = in[i];
		if(out) {
			const uint8 tmp = crypt_byte(keys);
			update_keys(keys, b);
			b ^= tmp;
			out[i] = b;
		}
		else
			update_keys(keys, b); // during initialization, we're only interested in key updates 
	}
}

void _zip_pkware_decrypt(zip_pkware_keys_t * keys, uint8 * out, const uint8 * in, uint64 len) 
{
	for(uint64 i = 0; i < len; i++) {
		uint8 b = in[i];
		// during initialization, we're only interested in key updates 
		if(out) {
			const uint8 tmp = crypt_byte(keys);
			b ^= tmp;
			out[i] = b;
		}
		update_keys(keys, b);
	}
}
//
// zip-source-pkware-encode
// Traditional PKWARE encryption routines
//
struct trad_pkware_encode {
	char * password;
	zip_pkware_keys_t keys;
	zip_buffer_t * buffer;
	bool eof;
	zip_error_t error;
};

struct trad_pkware_decode {
	char * password;
	zip_pkware_keys_t keys;
	zip_error_t error;
};

static struct trad_pkware_encode * trad_pkware_encode_new(const char * password, zip_error_t * error) 
{
	struct trad_pkware_encode * ctx = (struct trad_pkware_encode *)SAlloc::M(sizeof(*ctx));
	if(!ctx)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		if((ctx->password = strdup(password)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			SAlloc::F(ctx);
			return NULL;
		}
		ctx->buffer = NULL;
		zip_error_init(&ctx->error);
	}
	return ctx;
}

static void trad_pkware_encode_free(struct trad_pkware_encode * ctx) 
{
	if(ctx) {
		SAlloc::F(ctx->password);
		_zip_buffer_free(ctx->buffer);
		zip_error_fini(&ctx->error);
		SAlloc::F(ctx);
	}
}

static int pkware_encrypt_header(zip_source_t * src, struct trad_pkware_encode * ctx) 
{
	struct zip_stat st;
	unsigned short dostime, dosdate;
	uint8 * header;
	if(zip_source_stat(src, &st) != 0) {
		_zip_error_set_from_source(&ctx->error, src);
		return -1;
	}
	_zip_u2d_time(st.mtime, &dostime, &dosdate);
	if((ctx->buffer = _zip_buffer_new(NULL, ZIP_CRYPTO_PKWARE_HEADERLEN)) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	header = _zip_buffer_data(ctx->buffer);
	/* generate header from random bytes and mtime
	   see appnote.iz, XIII. Decryption, Step 2, last paragraph */
	if(!zip_secure_random(header, ZIP_CRYPTO_PKWARE_HEADERLEN - 1)) {
		zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
		_zip_buffer_free(ctx->buffer);
		ctx->buffer = NULL;
		return -1;
	}
	header[ZIP_CRYPTO_PKWARE_HEADERLEN - 1] = (uint8)((dostime >> 8) & 0xff);
	_zip_pkware_encrypt(&ctx->keys, header, header, ZIP_CRYPTO_PKWARE_HEADERLEN);
	return 0;
}

static int64 pkware_encrypt(zip_source_t * src, void * ud, void * data, uint64 length, zip_source_cmd_t cmd) 
{
	int64 n;
	uint64 buffer_n;
	struct trad_pkware_encode * ctx = (struct trad_pkware_encode *)ud;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    ctx->eof = false;
		    /* initialize keys */
		    _zip_pkware_keys_reset(&ctx->keys);
		    _zip_pkware_encrypt(&ctx->keys, NULL, (const uint8*)ctx->password, strlen(ctx->password));
		    if(pkware_encrypt_header(src, ctx) < 0) {
			    return -1;
		    }
		    return 0;
		case ZIP_SOURCE_READ:
		    buffer_n = 0;
		    if(ctx->buffer) {
			    /* write header values to data */
			    buffer_n = _zip_buffer_read(ctx->buffer, static_cast<uint8 *>(data), length);
			    data = (uint8 *)data + buffer_n;
			    length -= buffer_n;
			    if(_zip_buffer_eof(ctx->buffer)) {
				    _zip_buffer_free(ctx->buffer);
				    ctx->buffer = NULL;
			    }
		    }
		    if(ctx->eof) {
			    return (int64)buffer_n;
		    }
		    if((n = zip_source_read(src, data, length)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    _zip_pkware_encrypt(&ctx->keys, (uint8 *)data, (uint8 *)data, (uint64)n);
		    if((uint64)n < length) {
			    ctx->eof = true;
		    }
		    return (int64)buffer_n + n;
		case ZIP_SOURCE_CLOSE:
		    _zip_buffer_free(ctx->buffer);
		    ctx->buffer = NULL;
		    return 0;
		case ZIP_SOURCE_STAT: 
			{
				zip_stat_t * st = (zip_stat_t*)data;
				st->encryption_method = ZIP_EM_TRAD_PKWARE;
				st->valid |= ZIP_STAT_ENCRYPTION_METHOD;
				if(st->valid & ZIP_STAT_COMP_SIZE) {
					st->comp_size += ZIP_CRYPTO_PKWARE_HEADERLEN;
				}
				return 0;
			}
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES: 
			{
				zip_file_attributes_t * attributes = (zip_file_attributes_t*)data;
				if(length < sizeof(*attributes)) {
					zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
					return -1;
				}
				attributes->valid |= ZIP_FILE_ATTRIBUTES_VERSION_NEEDED;
				attributes->version_needed = 20;
				return 0;
			}
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, 
				ZIP_SOURCE_GET_FILE_ATTRIBUTES, -1);
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, length);
		case ZIP_SOURCE_FREE:
		    trad_pkware_encode_free(ctx);
		    return 0;
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
		    return -1;
	}
}

zip_source_t * zip_source_pkware_encode(zip_t * za, zip_source_t * src, uint16 em, int flags, const char * password) 
{
	struct trad_pkware_encode * ctx;
	zip_source_t * s2;
	if(password == NULL || src == NULL || em != ZIP_EM_TRAD_PKWARE) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(!(flags & ZIP_CODEC_ENCODE)) {
		zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
		return NULL;
	}
	if((ctx = trad_pkware_encode_new(password, &za->error)) == NULL) {
		return NULL;
	}
	if((s2 = zip_source_layered(za, src, pkware_encrypt, ctx)) == NULL) {
		trad_pkware_encode_free(ctx);
		return NULL;
	}
	return s2;
}
//
// zip-source-pkware-decode
// Traditional PKWARE decryption routines
//
static struct trad_pkware_decode * trad_pkware_decode_new(const char * password, zip_error_t * error) 
{
	struct trad_pkware_decode * ctx = (struct trad_pkware_decode *)SAlloc::M(sizeof(*ctx));
	if(!ctx)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		ctx->password = strdup(password);
		if(!ctx->password) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			ZFREE(ctx);
		}
		else
			zip_error_init(&ctx->error);
	}
	return ctx;
}

static void trad_pkware_decode_free(struct trad_pkware_decode * ctx) 
{
	if(ctx) {
		SAlloc::F(ctx->password);
		SAlloc::F(ctx);
	}
}

static int decrypt_header(zip_source_t * src, struct trad_pkware_decode * ctx) 
{
	uint8 header[ZIP_CRYPTO_PKWARE_HEADERLEN];
	struct zip_stat st;
	int64 n;
	if((n = zip_source_read(src, header, ZIP_CRYPTO_PKWARE_HEADERLEN)) < 0) {
		_zip_error_set_from_source(&ctx->error, src);
		return -1;
	}
	if(n != ZIP_CRYPTO_PKWARE_HEADERLEN) {
		zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
		return -1;
	}
	_zip_pkware_decrypt(&ctx->keys, header, header, ZIP_CRYPTO_PKWARE_HEADERLEN);
	if(zip_source_stat(src, &st)) {
		/* stat failed, skip password validation */
		return 0;
	}
	/* password verification - two ways:
	 *  mtime - InfoZIP way, to avoid computing complete CRC before encrypting data
	 *  CRC - old PKWare way
	 */
	bool ok = false;
	if(st.valid & ZIP_STAT_MTIME) {
		unsigned short dostime, dosdate;
		_zip_u2d_time(st.mtime, &dostime, &dosdate);
		if(header[ZIP_CRYPTO_PKWARE_HEADERLEN - 1] == dostime >> 8) {
			ok = true;
		}
	}
	if(st.valid & ZIP_STAT_CRC) {
		if(header[ZIP_CRYPTO_PKWARE_HEADERLEN - 1] == st.crc >> 24) {
			ok = true;
		}
	}
	if(!ok && ((st.valid & (ZIP_STAT_MTIME | ZIP_STAT_CRC)) != 0)) {
		zip_error_set(&ctx->error, SLERR_ZIP_WRONGPASSWD, 0);
		return -1;
	}
	return 0;
}

static int64 pkware_decrypt(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	int64 n;
	struct trad_pkware_decode * ctx = (struct trad_pkware_decode *)ud;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    _zip_pkware_keys_reset(&ctx->keys);
		    _zip_pkware_decrypt(&ctx->keys, NULL, (const uint8*)ctx->password, strlen(ctx->password));
		    if(decrypt_header(src, ctx) < 0) {
			    return -1;
		    }
		    return 0;
		case ZIP_SOURCE_READ:
		    if((n = zip_source_read(src, data, len)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    _zip_pkware_decrypt(&ctx->keys, (uint8 *)data, (uint8 *)data, (uint64)n);
		    return n;
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT: 
			{
				zip_stat_t * st = (zip_stat_t*)data;
				st->encryption_method = ZIP_EM_NONE;
				st->valid |= ZIP_STAT_ENCRYPTION_METHOD;
				if(st->valid & ZIP_STAT_COMP_SIZE) {
					st->comp_size -= ZIP_CRYPTO_PKWARE_HEADERLEN;
				}
				return 0;
			}
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, -1);
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    trad_pkware_decode_free(ctx);
		    return 0;
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
		    return -1;
	}
}

zip_source_t * zip_source_pkware_decode(zip_t * za, zip_source_t * src, uint16 em, int flags, const char * password) 
{
	zip_source_t * s2 = 0;
	if(!password || !src || em != ZIP_EM_TRAD_PKWARE)
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else if(flags & ZIP_CODEC_ENCODE)
		zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
	else {
		struct trad_pkware_decode * ctx = trad_pkware_decode_new(password, &za->error);
		if(ctx) {
			s2 = zip_source_layered(za, src, pkware_decrypt, ctx);
			if(!s2)
				trad_pkware_decode_free(ctx);
		}
	}
	return s2;
}
//
// zip-winzip-aes
// Winzip AES de/encryption backend routines
//
#define MAX_KEY_LENGTH 256
#define PBKDF2_ITERATIONS 1000

struct _zip_winzip_aes {
	_zip_crypto_aes_t * aes;
	_zip_crypto_hmac_t * hmac;
	uint8 counter[ZIP_CRYPTO_AES_BLOCK_LENGTH];
	uint8 pad[ZIP_CRYPTO_AES_BLOCK_LENGTH];
	int pad_offset;
};

static bool aes_crypt(zip_winzip_aes_t * ctx, uint8 * data, uint64 length) 
{
	for(uint64 i = 0; i < length; i++) {
		if(ctx->pad_offset == AES_BLOCK_SIZE) {
			for(uint j = 0; j < 8; j++) {
				ctx->counter[j]++;
				if(ctx->counter[j] != 0) {
					break;
				}
			}
			if(!_zip_crypto_aes_encrypt_block(ctx->aes, ctx->counter, ctx->pad)) {
				return false;
			}
			ctx->pad_offset = 0;
		}
		data[i] ^= ctx->pad[ctx->pad_offset++];
	}
	return true;
}

zip_winzip_aes_t * _zip_winzip_aes_new(const uint8 * password, uint64 password_length, const uint8 * salt,
    uint16 encryption_method, uint8 * password_verify, zip_error_t * error) 
{
	zip_winzip_aes_t * ctx;
	uint8 buffer[2 * (MAX_KEY_LENGTH / 8) + WINZIP_AES_PASSWORD_VERIFY_LENGTH];
	uint16 key_size = 0; /* in bits */
	uint16 key_length; /* in bytes */
	switch(encryption_method) {
		case ZIP_EM_AES_128: key_size = 128; break;
		case ZIP_EM_AES_192: key_size = 192; break;
		case ZIP_EM_AES_256: key_size = 256; break;
	}
	if(!key_size || !salt || !password || !password_length) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	key_length = key_size / 8;
	if((ctx = (zip_winzip_aes_t*)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	memzero(ctx->counter, sizeof(ctx->counter));
	ctx->pad_offset = ZIP_CRYPTO_AES_BLOCK_LENGTH;
	if(!_zip_crypto_pbkdf2(password, static_cast<int>(password_length), salt, key_length / 2, PBKDF2_ITERATIONS, buffer, 2 * key_length + WINZIP_AES_PASSWORD_VERIFY_LENGTH)) {
		SAlloc::F(ctx);
		return NULL;
	}
	if((ctx->aes = _zip_crypto_aes_new(buffer, key_size, error)) == NULL) {
		_zip_crypto_clear(ctx, sizeof(*ctx));
		SAlloc::F(ctx);
		return NULL;
	}
	if((ctx->hmac = _zip_crypto_hmac_new(buffer + key_length, key_length, error)) == NULL) {
		_zip_crypto_aes_free(ctx->aes);
		SAlloc::F(ctx);
		return NULL;
	}
	if(password_verify) {
		memcpy(password_verify, buffer + (2 * key_size / 8), WINZIP_AES_PASSWORD_VERIFY_LENGTH);
	}
	return ctx;
}

bool _zip_winzip_aes_encrypt(zip_winzip_aes_t * ctx, uint8 * data, uint64 length) 
{
	return aes_crypt(ctx, data, length) && _zip_crypto_hmac(ctx->hmac, data, static_cast<size_t>(length));
}

bool _zip_winzip_aes_decrypt(zip_winzip_aes_t * ctx, uint8 * data, uint64 length) 
{
	return _zip_crypto_hmac(ctx->hmac, data, static_cast<size_t>(length)) && aes_crypt(ctx, data, length);
}

bool _zip_winzip_aes_finish(zip_winzip_aes_t * ctx, uint8 * hmac) 
{
	return _zip_crypto_hmac_output(ctx->hmac, hmac);
}

void _zip_winzip_aes_free(zip_winzip_aes_t * ctx) 
{
	if(ctx) {
		_zip_crypto_aes_free(ctx->aes);
		_zip_crypto_hmac_free(ctx->hmac);
		SAlloc::F(ctx);
	}
}
//
// zip-source-winzip-aes-encode
// Winzip AES encryption routines
//
struct winzip_aes_encode {
	char * password;
	uint16 encryption_method;
	uint8 data[MAX(WINZIP_AES_MAX_HEADER_LENGTH, SHA1_LENGTH)];
	zip_buffer_t * buffer;
	zip_winzip_aes_t * aes_ctx;
	bool eof;
	zip_error_t error;
};

static int encrypt_header(zip_source_t * src, struct winzip_aes_encode * ctx) 
{
	uint16 salt_length = SALT_LENGTH(ctx->encryption_method);
	if(!zip_secure_random(ctx->data, salt_length)) {
		zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
		return -1;
	}
	if((ctx->aes_ctx = _zip_winzip_aes_new((uint8 *)ctx->password, strlen(ctx->password), ctx->data, ctx->encryption_method,
	    ctx->data + salt_length, &ctx->error)) == NULL) {
		return -1;
	}
	if((ctx->buffer = _zip_buffer_new(ctx->data, salt_length + WINZIP_AES_PASSWORD_VERIFY_LENGTH)) == NULL) {
		_zip_winzip_aes_free(ctx->aes_ctx);
		ctx->aes_ctx = NULL;
		zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
		return -1;
	}
	return 0;
}

static void winzip_aes_encode_free(struct winzip_aes_encode * ctx) 
{
	if(ctx) {
		_zip_crypto_clear(ctx->password, strlen(ctx->password));
		SAlloc::F(ctx->password);
		zip_error_fini(&ctx->error);
		_zip_buffer_free(ctx->buffer);
		_zip_winzip_aes_free(ctx->aes_ctx);
		SAlloc::F(ctx);
	}
}

static int64 winzip_aes_encrypt(zip_source_t * src, void * ud, void * data, uint64 length, zip_source_cmd_t cmd) 
{
	int64 ret;
	uint64 buffer_n;
	struct winzip_aes_encode * ctx = (struct winzip_aes_encode *)ud;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    ctx->eof = false;
		    if(encrypt_header(src, ctx) < 0) {
			    return -1;
		    }
		    return 0;
		case ZIP_SOURCE_READ:
		    buffer_n = 0;
		    if(ctx->buffer) {
			    buffer_n = _zip_buffer_read(ctx->buffer, static_cast<uint8 *>(data), length);
			    data = (uint8 *)data + buffer_n;
			    length -= buffer_n;
			    if(_zip_buffer_eof(ctx->buffer)) {
				    _zip_buffer_free(ctx->buffer);
				    ctx->buffer = NULL;
			    }
		    }
		    if(ctx->eof) {
			    return (int64)buffer_n;
		    }
		    if((ret = zip_source_read(src, data, length)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    if(!_zip_winzip_aes_encrypt(ctx->aes_ctx, static_cast<uint8 *>(data), (uint64)ret)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
			    /* TODO: return partial read? */
			    return -1;
		    }
		    if((uint64)ret < length) {
			    ctx->eof = true;
			    if(!_zip_winzip_aes_finish(ctx->aes_ctx, ctx->data)) {
				    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
				    /* TODO: return partial read? */
				    return -1;
			    }
			    _zip_winzip_aes_free(ctx->aes_ctx);
			    ctx->aes_ctx = NULL;
			    if((ctx->buffer = _zip_buffer_new(ctx->data, HMAC_LENGTH)) == NULL) {
				    zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
				    /* TODO: return partial read? */
				    return -1;
			    }
			    buffer_n += _zip_buffer_read(ctx->buffer, (uint8 *)data + ret, length - (uint64)ret);
		    }
		    return (int64)(buffer_n + (uint64)ret);
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT: 
			{
				zip_stat_t * st = (zip_stat_t*)data;
				st->encryption_method = ctx->encryption_method;
				st->valid |= ZIP_STAT_ENCRYPTION_METHOD;
				if(st->valid & ZIP_STAT_COMP_SIZE) {
					st->comp_size += 12 + SALT_LENGTH(ctx->encryption_method);
				}
				return 0;
			}
		case ZIP_SOURCE_GET_FILE_ATTRIBUTES: 
			{
				zip_file_attributes_t * attributes = (zip_file_attributes_t*)data;
				if(length < sizeof(*attributes)) {
					zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
					return -1;
				}
				attributes->valid |= ZIP_FILE_ATTRIBUTES_VERSION_NEEDED;
				attributes->version_needed = 51;
				return 0;
			}
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, 
				ZIP_SOURCE_GET_FILE_ATTRIBUTES, -1);
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, length);
		case ZIP_SOURCE_FREE:
		    winzip_aes_encode_free(ctx);
		    return 0;
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
		    return -1;
	}
}

static struct winzip_aes_encode * winzip_aes_encode_new(uint16 encryption_method, const char * password, zip_error_t * error) 
{
	struct winzip_aes_encode * ctx;
	if((ctx = (struct winzip_aes_encode *)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if((ctx->password = strdup(password)) == NULL) {
		SAlloc::F(ctx);
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->encryption_method = encryption_method;
	ctx->buffer = NULL;
	ctx->aes_ctx = NULL;
	zip_error_init(&ctx->error);
	ctx->eof = false;
	return ctx;
}

zip_source_t * zip_source_winzip_aes_encode(zip_t * za, zip_source_t * src, uint16 encryption_method, int flags, const char * password) 
{
	zip_source_t * s2;
	struct winzip_aes_encode * ctx;
	if((encryption_method != ZIP_EM_AES_128 && encryption_method != ZIP_EM_AES_192 && encryption_method != ZIP_EM_AES_256) ||
	    password == NULL || src == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = winzip_aes_encode_new(encryption_method, password, &za->error)) == NULL) {
		return NULL;
	}
	if((s2 = zip_source_layered(za, src, winzip_aes_encrypt, ctx)) == NULL) {
		winzip_aes_encode_free(ctx);
		return NULL;
	}
	return s2;
}
//
// zip-source-winzip-aes-decode
// Winzip AES decryption routines
//
struct winzip_aes_decode {
	char * password;
	uint16 encryption_method;
	uint64 data_length;
	uint64 current_position;
	zip_winzip_aes_t * aes_ctx;
	zip_error_t error;
};

static int decrypt_header(zip_source_t * src, struct winzip_aes_decode * ctx) 
{
	uint8 header[WINZIP_AES_MAX_HEADER_LENGTH];
	uint8 password_verification[WINZIP_AES_PASSWORD_VERIFY_LENGTH];
	int64 n;
	uint headerlen = WINZIP_AES_PASSWORD_VERIFY_LENGTH + SALT_LENGTH(ctx->encryption_method);
	if((n = zip_source_read(src, header, headerlen)) < 0) {
		_zip_error_set_from_source(&ctx->error, src);
		return -1;
	}
	if(n != headerlen) {
		zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
		return -1;
	}
	if((ctx->aes_ctx = _zip_winzip_aes_new((uint8 *)ctx->password, strlen(ctx->password), header, ctx->encryption_method, password_verification,
	    &ctx->error)) == NULL) {
		return -1;
	}
	if(memcmp(password_verification, header + SALT_LENGTH(ctx->encryption_method), WINZIP_AES_PASSWORD_VERIFY_LENGTH) != 0) {
		_zip_winzip_aes_free(ctx->aes_ctx);
		ctx->aes_ctx = NULL;
		zip_error_set(&ctx->error, SLERR_ZIP_WRONGPASSWD, 0);
		return -1;
	}
	return 0;
}

static bool verify_hmac(zip_source_t * src, struct winzip_aes_decode * ctx) 
{
	unsigned char computed[SHA1_LENGTH], from_file[HMAC_LENGTH];
	if(zip_source_read(src, from_file, HMAC_LENGTH) < HMAC_LENGTH) {
		_zip_error_set_from_source(&ctx->error, src);
		return false;
	}
	if(!_zip_winzip_aes_finish(ctx->aes_ctx, computed)) {
		zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
		return false;
	}
	_zip_winzip_aes_free(ctx->aes_ctx);
	ctx->aes_ctx = NULL;
	if(memcmp(from_file, computed, HMAC_LENGTH) != 0) {
		zip_error_set(&ctx->error, SLERR_ZIP_CRC, 0);
		return false;
	}
	return true;
}

static void winzip_aes_decode_free(struct winzip_aes_decode * ctx) 
{
	if(ctx) {
		_zip_crypto_clear(ctx->password, strlen(ctx->password));
		SAlloc::F(ctx->password);
		zip_error_fini(&ctx->error);
		_zip_winzip_aes_free(ctx->aes_ctx);
		SAlloc::F(ctx);
	}
}

static int64 winzip_aes_decrypt(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd) 
{
	int64 n;
	struct winzip_aes_decode * ctx = (struct winzip_aes_decode *)ud;
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    if(decrypt_header(src, ctx) < 0) {
			    return -1;
		    }
		    ctx->current_position = 0;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(len > ctx->data_length - ctx->current_position) {
			    len = ctx->data_length - ctx->current_position;
		    }
		    if(len == 0) {
			    if(!verify_hmac(src, ctx)) {
				    return -1;
			    }
			    return 0;
		    }
		    if((n = zip_source_read(src, data, len)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    ctx->current_position += (uint64)n;
		    if(!_zip_winzip_aes_decrypt(ctx->aes_ctx, (uint8 *)data, (uint64)n)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
			    return -1;
		    }
		    return n;
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT: 
			{
				zip_stat_t * st = (zip_stat_t*)data;
				st->encryption_method = ZIP_EM_NONE;
				st->valid |= ZIP_STAT_ENCRYPTION_METHOD;
				if(st->valid & ZIP_STAT_COMP_SIZE) {
					st->comp_size -= 12 + SALT_LENGTH(ctx->encryption_method);
				}
				return 0;
			}
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, -1);
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    winzip_aes_decode_free(ctx);
		    return 0;
		default:
		    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
		    return -1;
	}
}

static struct winzip_aes_decode * winzip_aes_decode_new(uint16 encryption_method, const char * password, zip_error_t * error) 
{
	struct winzip_aes_decode * ctx = (struct winzip_aes_decode *)SAlloc::M(sizeof(*ctx));
	if(!ctx)
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	else {
		if((ctx->password = strdup(password)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			SAlloc::F(ctx);
			return NULL;
		}
		else {
			ctx->encryption_method = encryption_method;
			ctx->aes_ctx = NULL;
			zip_error_init(&ctx->error);
		}
	}
	return ctx;
}

zip_source_t * zip_source_winzip_aes_decode(zip_t * za, zip_source_t * src, uint16 encryption_method, int flags, const char * password) 
{
	zip_source_t * s2;
	zip_stat_t st;
	uint64 aux_length;
	struct winzip_aes_decode * ctx;
	if((encryption_method != ZIP_EM_AES_128 && encryption_method != ZIP_EM_AES_192 && encryption_method != ZIP_EM_AES_256) ||
	    password == NULL || src == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(flags & ZIP_CODEC_ENCODE) {
		zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
		return NULL;
	}
	if(zip_source_stat(src, &st) != 0) {
		_zip_error_set_from_source(&za->error, src);
		return NULL;
	}
	aux_length = WINZIP_AES_PASSWORD_VERIFY_LENGTH + SALT_LENGTH(encryption_method) + HMAC_LENGTH;
	if((st.valid & ZIP_STAT_COMP_SIZE) == 0 || st.comp_size < aux_length) {
		zip_error_set(&za->error, SLERR_ZIP_OPNOTSUPP, 0);
		return NULL;
	}
	if((ctx = winzip_aes_decode_new(encryption_method, password, &za->error)) == NULL) {
		return NULL;
	}
	ctx->data_length = st.comp_size - aux_length;
	if((s2 = zip_source_layered(za, src, winzip_aes_decrypt, ctx)) == NULL) {
		winzip_aes_decode_free(ctx);
		return NULL;
	}
	return s2;
}
static void _zip_file_attributes_from_dirent(zip_file_attributes_t * attributes, zip_dirent_t * de) 
{
	zip_file_attributes_init(attributes);
	attributes->valid = ZIP_FILE_ATTRIBUTES_ASCII | ZIP_FILE_ATTRIBUTES_HOST_SYSTEM | ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES |
	    ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS;
	attributes->ascii = de->int_attrib & 1;
	attributes->host_system = de->version_madeby >> 8;
	attributes->external_file_attributes = de->ext_attrib;
	attributes->general_purpose_bit_flags = de->bitflags;
	attributes->general_purpose_bit_mask = ZIP_FILE_ATTRIBUTES_GENERAL_PURPOSE_BIT_FLAGS_ALLOWED_MASK;
}
//
// Descr: prepare data structures for zip_fopen/zip_source_zip
//
zip_source_t * _zip_source_zip_new(zip_t * za, zip_t * srcza, uint64 srcidx, zip_flags_t flags, uint64 start, uint64 len, const char * password) 
{
	zip_source_t * src, * s2;
	zip_stat_t st;
	zip_file_attributes_t attributes;
	zip_dirent_t * de;
	bool partial_data, needs_crc, needs_decrypt, needs_decompress;
	if(za == NULL) {
		return NULL;
	}
	if(srcza == NULL || srcidx >= srcza->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((flags & ZIP_FL_UNCHANGED) == 0 && (ZIP_ENTRY_DATA_CHANGED(srcza->entry + srcidx) || srcza->entry[srcidx].deleted)) {
		zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
		return NULL;
	}
	if(zip_stat_index(srcza, srcidx, flags | ZIP_FL_UNCHANGED, &st) < 0) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return NULL;
	}
	if(flags & ZIP_FL_ENCRYPTED) {
		flags |= ZIP_FL_COMPRESSED;
	}
	if((start > 0 || len > 0) && (flags & ZIP_FL_COMPRESSED)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	// overflow or past end of file 
	if((start > 0 || len > 0) && (start + len < start || start + len > st.size)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(len == 0) {
		len = st.size - start;
	}
	partial_data = len < st.size;
	needs_decrypt = ((flags & ZIP_FL_ENCRYPTED) == 0) && (st.encryption_method != ZIP_EM_NONE);
	needs_decompress = ((flags & ZIP_FL_COMPRESSED) == 0) && (st.comp_method != ZIP_CM_STORE);
	// when reading the whole file, check for CRC errors 
	needs_crc = ((flags & ZIP_FL_COMPRESSED) == 0 || st.comp_method == ZIP_CM_STORE) && !partial_data;
	if(needs_decrypt) {
		if(password == NULL) {
			password = za->default_password;
		}
		if(password == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_NOPASSWD, 0);
			return NULL;
		}
	}
	if((de = _zip_get_dirent(srcza, srcidx, flags, &za->error)) == NULL) {
		return NULL;
	}
	_zip_file_attributes_from_dirent(&attributes, de);
	if(st.comp_size == 0) {
		return zip_source_buffer_with_attributes(za, NULL, 0, 0, &attributes);
	}
	if(partial_data && !needs_decrypt && !needs_decompress) {
		struct zip_stat st2;
		st2.size = len;
		st2.comp_size = len;
		st2.comp_method = ZIP_CM_STORE;
		st2.mtime = st.mtime;
		st2.valid = ZIP_STAT_SIZE | ZIP_STAT_COMP_SIZE | ZIP_STAT_COMP_METHOD | ZIP_STAT_MTIME;
		if((src = _zip_source_window_new(srcza->src, start, len, &st2, &attributes, srcza, srcidx, &za->error)) == NULL) {
			return NULL;
		}
	}
	else {
		if((src = _zip_source_window_new(srcza->src, 0, st.comp_size, &st, &attributes, srcza, srcidx, &za->error)) == NULL) {
			return NULL;
		}
	}
	if(_zip_source_set_source_archive(src, srcza) < 0) {
		zip_source_free(src);
		return NULL;
	}
	// creating a layered source calls zip_keep() on the lower layer, so we free it 
	if(needs_decrypt) {
		zip_encryption_implementation enc_impl;
		if((enc_impl = _zip_get_encryption_implementation(st.encryption_method, ZIP_CODEC_DECODE)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
			return NULL;
		}
		s2 = enc_impl(za, src, st.encryption_method, 0, password);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(needs_decompress) {
		s2 = zip_source_decompress(za, src, st.comp_method);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(needs_crc) {
		s2 = zip_source_crc(za, src, 1);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(partial_data && (needs_decrypt || needs_decompress)) {
		s2 = zip_source_window(za, src, start, len);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	return src;
}
//
// Descr: create data source from zip file
//
ZIP_EXTERN zip_source_t * zip_source_zip(zip_t * za, zip_t * srcza, uint64 srcidx, zip_flags_t flags, uint64 start, int64 len) 
{
	if(len < -1) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else {
		if(len == -1)
			len = 0;
		if(start == 0 && len == 0)
			flags |= ZIP_FL_COMPRESSED;
		else
			flags &= ~ZIP_FL_COMPRESSED;
		return _zip_source_zip_new(za, srcza, srcidx, flags, start, (uint64)len, NULL);
	}
}
//
// Descr: undo helper function
//
void _zip_unchange_data(zip_entry_t * ze) 
{
	if(ze->source) {
		zip_source_free(ze->source);
		ze->source = NULL;
	}
	if(ze->changes != NULL && (ze->changes->changed & ZIP_DIRENT_COMP_METHOD) && ze->changes->comp_method == ZIP_CM_REPLACED_DEFAULT) {
		ze->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
		if(ze->changes->changed == 0) {
			_zip_dirent_free(ze->changes);
			ze->changes = NULL;
		}
	}
	ze->deleted = 0;
}
//
// Descr: undo global changes to ZIP archive
//
ZIP_EXTERN int zip_unchange_archive(zip_t * za) 
{
	if(za->comment_changed) {
		_zip_string_free(za->comment_changes);
		za->comment_changes = NULL;
		za->comment_changed = 0;
	}
	za->ch_flags = za->flags;
	return 0;
}
//
// Descr: undo changes to file in zip archive
//
ZIP_EXTERN int zip_unchange(zip_t * za, uint64 idx) { return _zip_unchange(za, idx, 0); }

int _zip_unchange(zip_t * za, uint64 idx, int allow_duplicates) 
{
	int64 i;
	const char * orig_name, * changed_name;
	if(idx >= za->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return -1;
	}
	if(!allow_duplicates && za->entry[idx].changes && (za->entry[idx].changes->changed & ZIP_DIRENT_FILENAME)) {
		if(za->entry[idx].orig != NULL) {
			if((orig_name = _zip_get_name(za, idx, ZIP_FL_UNCHANGED, &za->error)) == NULL) {
				return -1;
			}
			i = _zip_name_locate(za, orig_name, 0, NULL);
			if(i >= 0 && (uint64)i != idx) {
				zip_error_set(&za->error, SLERR_ZIP_EXISTS, 0);
				return -1;
			}
		}
		else {
			orig_name = NULL;
		}
		if((changed_name = _zip_get_name(za, idx, 0, &za->error)) == NULL) {
			return -1;
		}
		if(orig_name) {
			if(_zip_hash_add(za->names, (const uint8*)orig_name, idx, 0, &za->error) == false) {
				return -1;
			}
		}
		if(_zip_hash_delete(za->names, (const uint8*)changed_name, &za->error) == false) {
			_zip_hash_delete(za->names, (const uint8*)orig_name, NULL);
			return -1;
		}
	}
	_zip_dirent_free(za->entry[idx].changes);
	za->entry[idx].changes = NULL;
	_zip_unchange_data(za->entry + idx);
	return 0;
}
//
// Descr: undo changes to all files in zip archive
//
ZIP_EXTERN int zip_unchange_all(zip_t * za) 
{
	int ret;
	if(!_zip_hash_revert(za->names, &za->error)) {
		return -1;
	}
	else {
		ret = 0;
		for(uint64 i = 0; i < za->nentry; i++)
			ret |= _zip_unchange(za, i, 1);
		ret |= zip_unchange_archive(za);
		return ret;
	}
}
//
// zip-progress
// progress reporting
//
struct zip_progress {
	zip_t * za;
	zip_progress_callback callback_progress;
	void (* ud_progress_free)(void *);
	void * ud_progress;
	zip_cancel_callback callback_cancel;
	void (* ud_cancel_free)(void *);
	void * ud_cancel;
	double precision;
	/* state */
	double last_update; /* last value callback function was called with */
	double start; /* start of sub-progress section */
	double end; /* end of sub-progress section */
};

static void _zip_progress_free_cancel_callback(zip_progress_t * progress);
static void _zip_progress_free_progress_callback(zip_progress_t * progress);
static zip_progress_t * _zip_progress_new(zip_t * za);
static void _zip_progress_set_cancel_callback(zip_progress_t * progress, zip_cancel_callback callback, void (*ud_free)(void *), void * ud);
static void _zip_progress_set_progress_callback(zip_progress_t * progress, double precision, zip_progress_callback callback, void (*ud_free)(void *), void * ud);

void _zip_progress_end(zip_progress_t * progress) { _zip_progress_update(progress, 1.0); }

void _zip_progress_free(zip_progress_t * progress) 
{
	if(progress) {
		_zip_progress_free_progress_callback(progress);
		_zip_progress_free_cancel_callback(progress);
		SAlloc::F(progress);
	}
}

static zip_progress_t * _zip_progress_new(zip_t * za) 
{
	zip_progress_t * progress = (zip_progress_t*)SAlloc::M(sizeof(*progress));
	if(progress == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	progress->za = za;
	progress->callback_progress = NULL;
	progress->ud_progress_free = NULL;
	progress->ud_progress = NULL;
	progress->precision = 0.0;
	progress->callback_cancel = NULL;
	progress->ud_cancel_free = NULL;
	progress->ud_cancel = NULL;
	return progress;
}

static void _zip_progress_free_progress_callback(zip_progress_t * progress) 
{
	if(progress->ud_progress_free) {
		progress->ud_progress_free(progress->ud_progress);
	}
	progress->callback_progress = NULL;
	progress->ud_progress = NULL;
	progress->ud_progress_free = NULL;
}

static void _zip_progress_free_cancel_callback(zip_progress_t * progress) 
{
	if(progress->ud_cancel_free) {
		progress->ud_cancel_free(progress->ud_cancel);
	}
	progress->callback_cancel = NULL;
	progress->ud_cancel = NULL;
	progress->ud_cancel_free = NULL;
}

static void _zip_progress_set_progress_callback(zip_progress_t * progress,
    double precision, zip_progress_callback callback, void (*ud_free)(void *), void * ud) 
{
	_zip_progress_free_progress_callback(progress);
	progress->callback_progress = callback;
	progress->ud_progress_free = ud_free;
	progress->ud_progress = ud;
	progress->precision = precision;
}

void _zip_progress_set_cancel_callback(zip_progress_t * progress, zip_cancel_callback callback, void (*ud_free)(void *), void * ud) 
{
	_zip_progress_free_cancel_callback(progress);
	progress->callback_cancel = callback;
	progress->ud_cancel_free = ud_free;
	progress->ud_cancel = ud;
}

int _zip_progress_start(zip_progress_t * progress) 
{
	if(progress == NULL) {
		return 0;
	}
	if(progress->callback_progress != NULL) {
		progress->last_update = 0.0;
		progress->callback_progress(progress->za, 0.0, progress->ud_progress);
	}
	if(progress->callback_cancel != NULL) {
		if(progress->callback_cancel(progress->za, progress->ud_cancel)) {
			return -1;
		}
	}
	return 0;
}

int _zip_progress_subrange(zip_progress_t * progress, double start, double end) 
{
	if(progress == NULL) {
		return 0;
	}
	else {
		progress->start = start;
		progress->end = end;
		return _zip_progress_update(progress, 0.0);
	}
}

int _zip_progress_update(zip_progress_t * progress, double sub_current) 
{
	double current;
	if(progress == NULL) {
		return 0;
	}
	if(progress->callback_progress != NULL) {
		current = MIN(MAX(sub_current, 0.0), 1.0) * (progress->end - progress->start) + progress->start;
		if(current - progress->last_update > progress->precision) {
			progress->callback_progress(progress->za, current, progress->ud_progress);
			progress->last_update = current;
		}
	}
	if(progress->callback_cancel != NULL) {
		if(progress->callback_cancel(progress->za, progress->ud_cancel)) {
			return -1;
		}
	}
	return 0;
}

ZIP_EXTERN int zip_register_progress_callback_with_state(zip_t * za, double precision, zip_progress_callback callback, void (*ud_free)(void *), void * ud) 
{
	if(callback != NULL) {
		if(za->progress == NULL) {
			if((za->progress = _zip_progress_new(za)) == NULL) {
				return -1;
			}
		}
		_zip_progress_set_progress_callback(za->progress, precision, callback, ud_free, ud);
	}
	else {
		if(za->progress != NULL) {
			if(za->progress->callback_cancel == NULL) {
				_zip_progress_free(za->progress);
				za->progress = NULL;
			}
			else {
				_zip_progress_free_progress_callback(za->progress);
			}
		}
	}
	return 0;
}

ZIP_EXTERN int zip_register_cancel_callback_with_state(zip_t * za, zip_cancel_callback callback, void (*ud_free)(void *), void * ud) 
{
	if(callback != NULL) {
		if(za->progress == NULL) {
			if((za->progress = _zip_progress_new(za)) == NULL) {
				return -1;
			}
		}
		_zip_progress_set_cancel_callback(za->progress, callback, ud_free, ud);
	}
	else {
		if(za->progress != NULL) {
			if(za->progress->callback_progress == NULL) {
				_zip_progress_free(za->progress);
				za->progress = NULL;
			}
			else {
				_zip_progress_free_cancel_callback(za->progress);
			}
		}
	}
	return 0;
}

struct legacy_ud {
	zip_progress_callback_t callback;
};

static void _zip_legacy_progress_callback(zip_t * za, double progress, void * vud) 
{
	struct legacy_ud * ud = (struct legacy_ud *)vud;
	ud->callback(progress);
}

ZIP_EXTERN void zip_register_progress_callback(zip_t * za, zip_progress_callback_t progress_callback) 
{
	struct legacy_ud * ud;
	if(progress_callback == NULL) {
		zip_register_progress_callback_with_state(za, 0, NULL, NULL, NULL);
	}
	if((ud = (struct legacy_ud *)SAlloc::M(sizeof(*ud))) == NULL) {
		return;
	}
	ud->callback = progress_callback;
	if(zip_register_progress_callback_with_state(za, 0.001, _zip_legacy_progress_callback, free, ud) < 0) {
		SAlloc::F(ud);
	}
}
//
// zip-crypto
//
#ifdef HAVE_COMMONCRYPTO
	void _zip_crypto_aes_free(_zip_crypto_aes_t * aes) 
	{
		if(aes)
			CCCryptorRelease(aes);
	}
	bool _zip_crypto_aes_encrypt_block(_zip_crypto_aes_t * aes, const uint8 * in, uint8 * out) 
	{
		size_t len;
		CCCryptorUpdate(aes, in, ZIP_CRYPTO_AES_BLOCK_LENGTH, out, ZIP_CRYPTO_AES_BLOCK_LENGTH, &len);
		return true;
	}
	_zip_crypto_aes_t * _zip_crypto_aes_new(const uint8 * key, uint16 key_size, zip_error_t * error) 
	{
		_zip_crypto_aes_t * aes;
		CCCryptorStatus ret = CCCryptorCreate(kCCEncrypt, kCCAlgorithmAES, kCCOptionECBMode, key, key_size / 8, NULL, &aes);
		switch(ret) {
			case kCCSuccess:
				return aes;
			case kCCMemoryFailure:
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
				return NULL;
			case kCCParamError:
				zip_error_set(error, SLERR_ZIP_INVAL, 0);
				return NULL;
			default:
				zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
				return NULL;
		}
	}
	void _zip_crypto_hmac_free(_zip_crypto_hmac_t * hmac) 
	{
		if(hmac) {
			_zip_crypto_clear(hmac, sizeof(*hmac));
			SAlloc::F(hmac);
		}
	}
	_zip_crypto_hmac_t * _zip_crypto_hmac_new(const uint8 * secret, uint64 secret_length, zip_error_t * error) 
	{
		_zip_crypto_hmac_t * hmac;
		if((hmac = (_zip_crypto_hmac_t*)SAlloc::M(sizeof(*hmac))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		CCHmacInit(hmac, kCCHmacAlgSHA1, secret, secret_length);
		return hmac;
	}
#endif
//
// zip-crypto-openssl
//
#if OPENSSL_VERSION_NUMBER < 0x1010000fL || defined(LIBRESSL_VERSION_NUMBER)
	#define USE_OPENSSL_1_0_API
#endif

_zip_crypto_aes_t * _zip_crypto_aes_new(const uint8 * key, uint16 key_size, zip_error_t * error) 
{
	_zip_crypto_aes_t * aes;
	if((aes = (_zip_crypto_aes_t *)SAlloc::M(sizeof(*aes))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	AES_set_encrypt_key(key, key_size, reinterpret_cast<AES_KEY *>(aes));
	return aes;
}

void _zip_crypto_aes_free(_zip_crypto_aes_t * aes) 
{
	if(aes) {
		_zip_crypto_clear(aes, sizeof(*aes));
		SAlloc::F(aes);
	}
}

_zip_crypto_hmac_t * _zip_crypto_hmac_new(const uint8 * secret, uint64 secret_length, zip_error_t * error) 
{
	_zip_crypto_hmac_t * hmac;
	if(secret_length > INT_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
#ifdef USE_OPENSSL_1_0_API
	if((hmac = (_zip_crypto_hmac_t*)SAlloc::M(sizeof(*hmac))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	HMAC_CTX_init(hmac);
#else
	if((hmac = reinterpret_cast<_zip_crypto_hmac_t *>(HMAC_CTX_new())) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
#endif
	if(HMAC_Init_ex(reinterpret_cast<HMAC_CTX *>(hmac), secret, (int)secret_length, EVP_sha1(), NULL) != 1) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
#ifdef USE_OPENSSL_1_0_API
		SAlloc::F(hmac);
#else
		HMAC_CTX_free(reinterpret_cast<HMAC_CTX *>(hmac));
#endif
		return NULL;
	}
	return hmac;
}

void _zip_crypto_hmac_free(_zip_crypto_hmac_t * hmac) 
{
	if(hmac) {
#ifdef USE_OPENSSL_1_0_API
		HMAC_CTX_cleanup(hmac);
		_zip_crypto_clear(hmac, sizeof(*hmac));
		SAlloc::F(hmac);
#else
		HMAC_CTX_free(reinterpret_cast<HMAC_CTX *>(hmac));
#endif
	}
}

bool _zip_crypto_hmac_output(_zip_crypto_hmac_t * hmac, uint8 * data) 
{
	uint length;
	return HMAC_Final(reinterpret_cast<HMAC_CTX *>(hmac), data, &length) == 1;
}

ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
{
	return RAND_bytes(buffer, length) == 1;
}
//
// zip-crypto-gnutls
//
#ifdef HAVE_GNUTLS
	_zip_crypto_aes_t * _zip_crypto_aes_new(const uint8 * key, uint16 key_size, zip_error_t * error)
	{
		_zip_crypto_aes_t * aes;
		if((aes = (_zip_crypto_aes_t*)SAlloc::M(sizeof(*aes))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		aes->key_size = key_size;
		switch(aes->key_size) {
			case 128: nettle_aes128_set_encrypt_key(&aes->ctx.ctx_128, key); break;
			case 192: nettle_aes192_set_encrypt_key(&aes->ctx.ctx_192, key); break;
			case 256: nettle_aes256_set_encrypt_key(&aes->ctx.ctx_256, key); break;
			default:
				zip_error_set(error, SLERR_ZIP_INVAL, 0);
				SAlloc::F(aes);
				return NULL;
		}
		return aes;
	}

	bool _zip_crypto_aes_encrypt_block(_zip_crypto_aes_t * aes, const uint8 * in, uint8 * out)
	{
		switch(aes->key_size) {
			case 128: nettle_aes128_encrypt(&aes->ctx.ctx_128, ZIP_CRYPTO_AES_BLOCK_LENGTH, out, in); break;
			case 192: nettle_aes192_encrypt(&aes->ctx.ctx_192, ZIP_CRYPTO_AES_BLOCK_LENGTH, out, in); break;
			case 256: nettle_aes256_encrypt(&aes->ctx.ctx_256, ZIP_CRYPTO_AES_BLOCK_LENGTH, out, in); break;
		}
		return true;
	}

	void _zip_crypto_aes_free(_zip_crypto_aes_t * aes)
	{
		if(aes) {
			_zip_crypto_clear(aes, sizeof(*aes));
			SAlloc::F(aes);
		}
	}

	_zip_crypto_hmac_t * _zip_crypto_hmac_new(const uint8 * secret, uint64 secret_length, zip_error_t * error)
	{
		_zip_crypto_hmac_t * hmac;
		int ret;
		if((hmac = (_zip_crypto_hmac_t*)SAlloc::M(sizeof(*hmac))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		if((ret = gnutls_hmac_init(hmac, GNUTLS_MAC_SHA1, secret, secret_length)) < 0) {
			// TODO: set error 
			SAlloc::F(hmac);
			return NULL;
		}
		return hmac;
	}

	void _zip_crypto_hmac_free(_zip_crypto_hmac_t * hmac) 
	{
		uint8 buf[ZIP_CRYPTO_SHA1_LENGTH];
		if(hmac) {
			gnutls_hmac_deinit(*hmac, buf);
			_zip_crypto_clear(hmac, sizeof(*hmac));
			SAlloc::F(hmac);
		}
	}

	ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
	{
		return gnutls_rnd(GNUTLS_RND_KEY, buffer, length) == 0;
	}
#endif
//
// zip-crypto-mbedtls
//
#ifdef HAVE_MBEDTLS
	#include <mbedtls/ctr_drbg.h>
	#include <mbedtls/entropy.h>
	#include <mbedtls/pkcs5.h>

	_zip_crypto_aes_t * _zip_crypto_aes_new(const uint8 * key, uint16 key_size, zip_error_t * error) 
	{
		_zip_crypto_aes_t * aes;
		if((aes = (_zip_crypto_aes_t*)SAlloc::M(sizeof(*aes))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		mbedtls_aes_init(aes);
		mbedtls_aes_setkey_enc(aes, (const unsigned char*)key, (uint)key_size);
		return aes;
	}

	void _zip_crypto_aes_free(_zip_crypto_aes_t * aes) 
	{
		if(aes) {
			mbedtls_aes_free(aes);
			SAlloc::F(aes);
		}
	}

	_zip_crypto_hmac_t * _zip_crypto_hmac_new(const uint8 * secret, uint64 secret_length, zip_error_t * error) 
	{
		_zip_crypto_hmac_t * hmac;
		if(secret_length > INT_MAX) {
			zip_error_set(error, SLERR_ZIP_INVAL, 0);
			return NULL;
		}
		if((hmac = (_zip_crypto_hmac_t*)SAlloc::M(sizeof(*hmac))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		mbedtls_md_init(hmac);
		if(mbedtls_md_setup(hmac, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1) != 0) {
			zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
			SAlloc::F(hmac);
			return NULL;
		}
		if(mbedtls_md_hmac_starts(hmac, (const unsigned char*)secret, (size_t)secret_length) != 0) {
			zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
			SAlloc::F(hmac);
			return NULL;
		}
		return hmac;
	}

	void _zip_crypto_hmac_free(_zip_crypto_hmac_t * hmac) 
	{
		if(hmac) {
			mbedtls_md_free(hmac);
			SAlloc::F(hmac);
		}
	}

	bool _zip_crypto_pbkdf2(const uint8 * key, uint64 key_length, const uint8 * salt,
		uint16 salt_length, int iterations, uint8 * output, uint64 output_length) 
	{
		mbedtls_md_context_t sha1_ctx;
		bool ok = true;
		mbedtls_md_init(&sha1_ctx);
		if(mbedtls_md_setup(&sha1_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1) != 0) {
			ok = false;
		}
		if(ok && mbedtls_pkcs5_pbkdf2_hmac(&sha1_ctx, (const unsigned char*)key, (size_t)key_length, (const unsigned char*)salt,
			(size_t)salt_length, (uint)iterations, (uint32_t)output_length, (unsigned char*)output) != 0) {
			ok = false;
		}
		mbedtls_md_free(&sha1_ctx);
		return ok;
	}

	typedef struct {
		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context ctr_drbg;
	} zip_random_context_t;

	ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
	{
		static zip_random_context_t * ctx = NULL;
		const unsigned char * pers = "zip_crypto_mbedtls";
		if(!ctx) {
			ctx = (zip_random_context_t*)SAlloc::M(sizeof(zip_random_context_t));
			if(!ctx) {
				return false;
			}
			mbedtls_entropy_init(&ctx->entropy);
			mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
			if(mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy, pers, strlen(pers)) != 0) {
				mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
				mbedtls_entropy_free(&ctx->entropy);
				SAlloc::F(ctx);
				ctx = NULL;
				return false;
			}
		}
		return mbedtls_ctr_drbg_random(&ctx->ctr_drbg, (unsigned char*)buffer, (size_t)length) == 0;
	}
#endif
//
// zip-crypto-win
//
#ifdef HAVE_WINDOWS_CRYPTO
#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")
/*

   This code is using the Cryptography API: Next Generation (CNG)
   https://docs.microsoft.com/en-us/windows/desktop/seccng/cng-portal

   This API is supported on
   - Windows Vista or later (client OS)
   - Windows Server 2008 (server OS)
   - Windows Embedded Compact 2013 (don't know about Windows Embedded Compact 7)

   The code was developed for Windows Embedded Compact 2013 (WEC2013),
   but should be working for all of the above mentioned OSes.

   There are 2 restrictions for WEC2013, Windows Vista and Windows Server 2008:

   1.) The function "BCryptDeriveKeyPBKDF2" is not available

   I found some code which is implementing this function using the deprecated Crypto API here:
   https://www.idrix.fr/Root/content/view/37/54/

   I took this code and converted it to the newer CNG API. The original code was more
   flexible, but this is not needed here so i refactored it a bit and just kept what is needed.

   The define "HAS_BCRYPTDERIVEKEYPBKDF2" controls whether "BCryptDeriveKeyPBKDF2"
   of the CNG API is used or not. This define must not be set if you are compiling for WEC2013 or Windows Vista.


   2.) "BCryptCreateHash" can't manage the memory needed for the hash object internally

   On Windows 7 or later it is possible to pass NULL for the hash object buffer.
   This is not supported on WEC2013, so we have to handle the memory allocation/deallocation ourselves.
   There is no #ifdef to control that, because this is working for all supported OSes.

 */
#if !defined(WINCE) && !defined(__MINGW32__)
	#define HAS_BCRYPTDERIVEKEYPBKDF2
#endif

#ifdef HAS_BCRYPTDERIVEKEYPBKDF2
	bool _zip_crypto_pbkdf2(const uint8 * key, uint64 key_length, const uint8 * salt,
		uint16 salt_length, uint16 iterations, uint8 * output, uint16 output_length) 
	{
		BCRYPT_ALG_HANDLE hAlgorithm = NULL;
		if(!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG))) {
			return false;
		}
		bool result = BCRYPT_SUCCESS(BCryptDeriveKeyPBKDF2(hAlgorithm, (PUCHAR)key, (ULONG)key_length, (PUCHAR)salt, salt_length, iterations, output,
			output_length, 0));
		BCryptCloseAlgorithmProvider(hAlgorithm, 0);
		return result;
	}
#else

#define DIGEST_SIZE 20
#define BLOCK_SIZE 64

struct PRF_CTX {
	BCRYPT_ALG_HANDLE hAlgorithm;
	BCRYPT_HASH_HANDLE hInnerHash;
	BCRYPT_HASH_HANDLE hOuterHash;
	ULONG cbHashObject;
	PUCHAR pbInnerHash;
	PUCHAR pbOuterHash;
};

static void hmacFree(PRF_CTX * pContext) 
{
	if(pContext->hOuterHash)
		BCryptDestroyHash(pContext->hOuterHash);
	if(pContext->hInnerHash)
		BCryptDestroyHash(pContext->hInnerHash);
	SAlloc::F(pContext->pbOuterHash);
	SAlloc::F(pContext->pbInnerHash);
	if(pContext->hAlgorithm)
		BCryptCloseAlgorithmProvider(pContext->hAlgorithm, 0);
}

static BOOL hmacPrecomputeDigest(BCRYPT_HASH_HANDLE hHash, PUCHAR pbPassword, DWORD cbPassword, BYTE mask) 
{
	BYTE buffer[BLOCK_SIZE];
	DWORD i;
	if(cbPassword > BLOCK_SIZE) {
		return FALSE;
	}
	memset(buffer, mask, sizeof(buffer));
	for(i = 0; i < cbPassword; ++i) {
		buffer[i] = (char)(pbPassword[i] ^ mask);
	}
	return BCRYPT_SUCCESS(BCryptHashData(hHash, buffer, sizeof(buffer), 0));
}

static BOOL hmacInit(PRF_CTX * pContext, PUCHAR pbPassword, DWORD cbPassword) 
{
	BOOL bStatus = FALSE;
	ULONG cbResult;
	BYTE key[DIGEST_SIZE];
	if(!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&pContext->hAlgorithm, BCRYPT_SHA1_ALGORITHM, NULL, 0)) ||
	    !BCRYPT_SUCCESS(BCryptGetProperty(pContext->hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&pContext->cbHashObject,
	    sizeof(pContext->cbHashObject), &cbResult,
	    0)) || ((pContext->pbInnerHash = SAlloc::M(pContext->cbHashObject)) == NULL) ||
	    ((pContext->pbOuterHash = SAlloc::M(pContext->cbHashObject)) == NULL) ||
	    !BCRYPT_SUCCESS(BCryptCreateHash(pContext->hAlgorithm, &pContext->hInnerHash, pContext->pbInnerHash, pContext->cbHashObject,
	    NULL, 0, 0)) ||
	    !BCRYPT_SUCCESS(BCryptCreateHash(pContext->hAlgorithm, &pContext->hOuterHash, pContext->pbOuterHash, pContext->cbHashObject,
	    NULL, 0, 0))) {
		goto hmacInit_end;
	}
	if(cbPassword > BLOCK_SIZE) {
		BCRYPT_HASH_HANDLE hHash = NULL;
		PUCHAR pbHashObject = SAlloc::M(pContext->cbHashObject);
		if(pbHashObject == NULL) {
			goto hmacInit_end;
		}
		bStatus = BCRYPT_SUCCESS(BCryptCreateHash(pContext->hAlgorithm, &hHash, pbHashObject, pContext->cbHashObject, NULL, 0, 0)) &&
		    BCRYPT_SUCCESS(BCryptHashData(hHash, pbPassword, cbPassword, 0)) &&
		    BCRYPT_SUCCESS(BCryptGetProperty(hHash, BCRYPT_HASH_LENGTH, (PUCHAR)&cbPassword, sizeof(cbPassword), &cbResult, 0)) && 
			BCRYPT_SUCCESS(BCryptFinishHash(hHash, key, cbPassword, 0));
		if(hHash)
			BCryptDestroyHash(hHash);
		SAlloc::F(pbHashObject);
		if(!bStatus) {
			goto hmacInit_end;
		}
		pbPassword = key;
	}
	bStatus = hmacPrecomputeDigest(pContext->hInnerHash, pbPassword, cbPassword, 0x36) && hmacPrecomputeDigest(pContext->hOuterHash, pbPassword, cbPassword, 0x5C);
hmacInit_end:
	if(bStatus == FALSE)
		hmacFree(pContext);
	return bStatus;
}

static BOOL hmacCalculateInternal(BCRYPT_HASH_HANDLE hHashTemplate, PUCHAR pbData, DWORD cbData, PUCHAR pbOutput, DWORD cbOutput, DWORD cbHashObject) 
{
	BOOL success = FALSE;
	BCRYPT_HASH_HANDLE hHash = NULL;
	PUCHAR pbHashObject = SAlloc::M(cbHashObject);
	if(pbHashObject == NULL) {
		return FALSE;
	}
	if(BCRYPT_SUCCESS(BCryptDuplicateHash(hHashTemplate, &hHash, pbHashObject, cbHashObject, 0))) {
		success = BCRYPT_SUCCESS(BCryptHashData(hHash, pbData, cbData, 0)) && BCRYPT_SUCCESS(BCryptFinishHash(hHash, pbOutput, cbOutput, 0));
		BCryptDestroyHash(hHash);
	}
	SAlloc::F(pbHashObject);
	return success;
}

static BOOL hmacCalculate(PRF_CTX * pContext, PUCHAR pbData, DWORD cbData, PUCHAR pbDigest) 
{
	DWORD cbResult;
	DWORD cbHashObject;
	return BCRYPT_SUCCESS(BCryptGetProperty(pContext->hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&cbHashObject, sizeof(cbHashObject),
		   &cbResult,
		   0)) &&
	       hmacCalculateInternal(pContext->hInnerHash, pbData, cbData, pbDigest, DIGEST_SIZE, cbHashObject) && hmacCalculateInternal(
		pContext->hOuterHash,
		pbDigest,
		DIGEST_SIZE,
		pbDigest,
		DIGEST_SIZE,
		cbHashObject);
}

static void myxor(LPBYTE ptr1, LPBYTE ptr2, DWORD dwLen) 
{
	while(dwLen--)
		*ptr1++ ^= *ptr2++;
}

BOOL pbkdf2(PUCHAR pbPassword, ULONG cbPassword, PUCHAR pbSalt, ULONG cbSalt, DWORD cIterations, PUCHAR pbDerivedKey, ULONG cbDerivedKey) 
{
	BOOL bStatus = FALSE;
	DWORD l, r, dwULen, i, j;
	BYTE Ti[DIGEST_SIZE];
	BYTE V[DIGEST_SIZE];
	LPBYTE U = SAlloc::M(max((cbSalt + 4), DIGEST_SIZE));
	PRF_CTX prfCtx = {0};
	if(U == NULL) {
		return FALSE;
	}
	if(pbPassword == NULL || cbPassword == 0 || pbSalt == NULL || cbSalt == 0 || cIterations == 0 || pbDerivedKey == NULL ||
	    cbDerivedKey == 0) {
		SAlloc::F(U);
		return FALSE;
	}
	if(!hmacInit(&prfCtx, pbPassword, cbPassword)) {
		goto PBKDF2_end;
	}
	l = (DWORD)ceil((double)cbDerivedKey / (double)DIGEST_SIZE);
	r = cbDerivedKey - (l - 1) * DIGEST_SIZE;
	for(i = 1; i <= l; i++) {
		ZeroMemory(Ti, DIGEST_SIZE);
		for(j = 0; j < cIterations; j++) {
			if(j == 0) {
				/* construct first input for PRF */
				memcpy(U, pbSalt, cbSalt);
				U[cbSalt] = (BYTE)((i & 0xFF000000) >> 24);
				U[cbSalt + 1] = (BYTE)((i & 0x00FF0000) >> 16);
				U[cbSalt + 2] = (BYTE)((i & 0x0000FF00) >> 8);
				U[cbSalt + 3] = (BYTE)((i & 0x000000FF));
				dwULen = cbSalt + 4;
			}
			else {
				memcpy(U, V, DIGEST_SIZE);
				dwULen = DIGEST_SIZE;
			}
			if(!hmacCalculate(&prfCtx, U, dwULen, V)) {
				goto PBKDF2_end;
			}
			myxor(Ti, V, DIGEST_SIZE);
		}
		if(i != l) {
			memcpy(&pbDerivedKey[(i - 1) * DIGEST_SIZE], Ti, DIGEST_SIZE);
		}
		else {
			/* Take only the first r bytes */
			memcpy(&pbDerivedKey[(i - 1) * DIGEST_SIZE], Ti, r);
		}
	}
	bStatus = TRUE;
PBKDF2_end:
	hmacFree(&prfCtx);
	SAlloc::F(U);
	return bStatus;
}

bool _zip_crypto_pbkdf2(const uint8 * key, uint64 key_length, const uint8 * salt, uint16 salt_length,
    uint16 iterations, uint8 * output, uint16 output_length) 
{
	return (key_length <= ZIP_UINT32_MAX) && pbkdf2((PUCHAR)key, (ULONG)key_length, (PUCHAR)salt, salt_length,
		   iterations, output, output_length);
}

#endif

struct _zip_crypto_aes_s {
	BCRYPT_ALG_HANDLE hAlgorithm;
	BCRYPT_KEY_HANDLE hKey;
	ULONG cbKeyObject;
	PUCHAR pbKeyObject;
};

_zip_crypto_aes_t * _zip_crypto_aes_new(const uint8 * key, uint16 key_size, zip_error_t * error) 
{
	_zip_crypto_aes_t * aes = (_zip_crypto_aes_t*)SAlloc::C(1, sizeof(*aes));
	ULONG cbResult;
	ULONG key_length = key_size / 8;
	if(aes == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if(!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&aes->hAlgorithm, BCRYPT_AES_ALGORITHM, NULL, 0))) {
		_zip_crypto_aes_free(aes);
		return NULL;
	}
	if(!BCRYPT_SUCCESS(BCryptSetProperty(aes->hAlgorithm, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB,
	    sizeof(BCRYPT_CHAIN_MODE_ECB), 0))) {
		_zip_crypto_aes_free(aes);
		return NULL;
	}
	if(!BCRYPT_SUCCESS(BCryptGetProperty(aes->hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&aes->cbKeyObject, sizeof(aes->cbKeyObject),
	    &cbResult, 0))) {
		_zip_crypto_aes_free(aes);
		return NULL;
	}
	aes->pbKeyObject = static_cast<PUCHAR>(SAlloc::M(aes->cbKeyObject));
	if(aes->pbKeyObject == NULL) {
		_zip_crypto_aes_free(aes);
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if(!BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(aes->hAlgorithm, &aes->hKey, aes->pbKeyObject, aes->cbKeyObject, (PUCHAR)key,
	    key_length, 0))) {
		_zip_crypto_aes_free(aes);
		return NULL;
	}
	return aes;
}

void _zip_crypto_aes_free(_zip_crypto_aes_t * aes) 
{
	if(aes) {
		if(aes->hKey != NULL) {
			BCryptDestroyKey(aes->hKey);
		}
		if(aes->pbKeyObject != NULL) {
			SAlloc::F(aes->pbKeyObject);
		}
		if(aes->hAlgorithm != NULL) {
			BCryptCloseAlgorithmProvider(aes->hAlgorithm, 0);
		}
		SAlloc::F(aes);
	}
}

bool _zip_crypto_aes_encrypt_block(_zip_crypto_aes_t * aes, const uint8 * in, uint8 * out) 
{
	ULONG cbResult;
	NTSTATUS status = BCryptEncrypt(aes->hKey, (PUCHAR)in, ZIP_CRYPTO_AES_BLOCK_LENGTH, NULL,
		NULL, 0, (PUCHAR)out, ZIP_CRYPTO_AES_BLOCK_LENGTH, &cbResult, 0);
	return BCRYPT_SUCCESS(status);
}

struct _zip_crypto_hmac_s {
	BCRYPT_ALG_HANDLE hAlgorithm;
	BCRYPT_HASH_HANDLE hHash;
	DWORD cbHashObject;
	PUCHAR pbHashObject;
	DWORD cbHash;
	PUCHAR pbHash;
};
/*
   https://code.msdn.microsoft.com/windowsdesktop/Hmac-Computation-Sample-11fe8ec1/sourcecode?fileId=42820&pathId=283874677
   */
_zip_crypto_hmac_t * _zip_crypto_hmac_new(const uint8 * secret, uint64 secret_length, zip_error_t * error) 
{
	NTSTATUS status;
	ULONG cbResult;
	_zip_crypto_hmac_t * hmac;
	if(secret_length > INT_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	hmac = (_zip_crypto_hmac_t*)SAlloc::C(1, sizeof(*hmac));
	if(hmac == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	status = BCryptOpenAlgorithmProvider(&hmac->hAlgorithm, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
	if(!BCRYPT_SUCCESS(status)) {
		_zip_crypto_hmac_free(hmac);
		return NULL;
	}
	status = BCryptGetProperty(hmac->hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&hmac->cbHashObject, sizeof(hmac->cbHashObject), &cbResult, 0);
	if(!BCRYPT_SUCCESS(status)) {
		_zip_crypto_hmac_free(hmac);
		return NULL;
	}
	hmac->pbHashObject = static_cast<PUCHAR>(SAlloc::M(hmac->cbHashObject));
	if(hmac->pbHashObject == NULL) {
		_zip_crypto_hmac_free(hmac);
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	status = BCryptGetProperty(hmac->hAlgorithm, BCRYPT_HASH_LENGTH, (PUCHAR)&hmac->cbHash, sizeof(hmac->cbHash), &cbResult, 0);
	if(!BCRYPT_SUCCESS(status)) {
		_zip_crypto_hmac_free(hmac);
		return NULL;
	}
	hmac->pbHash = static_cast<PUCHAR>(SAlloc::M(hmac->cbHash));
	if(hmac->pbHash == NULL) {
		_zip_crypto_hmac_free(hmac);
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	status = BCryptCreateHash(hmac->hAlgorithm, &hmac->hHash, hmac->pbHashObject, hmac->cbHashObject,
		(PUCHAR)secret, (ULONG)secret_length, 0);
	if(!BCRYPT_SUCCESS(status)) {
		_zip_crypto_hmac_free(hmac);
		return NULL;
	}
	return hmac;
}

void _zip_crypto_hmac_free(_zip_crypto_hmac_t * hmac)
{
	if(hmac) {
		if(hmac->hHash != NULL) {
			BCryptDestroyHash(hmac->hHash);
		}
		SAlloc::F(hmac->pbHash);
		SAlloc::F(hmac->pbHashObject);
		BCryptCloseAlgorithmProvider(hmac->hAlgorithm, 0);
		SAlloc::F(hmac);
	}
}

bool _zip_crypto_hmac(_zip_crypto_hmac_t * hmac, uint8 * data, uint64 length) 
	{ return (hmac && length <= ULONG_MAX) ? BCRYPT_SUCCESS(BCryptHashData(hmac->hHash, data, (ULONG)length, 0)) : false; }
bool _zip_crypto_hmac_output(_zip_crypto_hmac_t * hmac, uint8 * data) 
	{ return hmac ? BCRYPT_SUCCESS(BCryptFinishHash(hmac->hHash, data, hmac->cbHash, 0)) : false; }
ZIP_EXTERN bool zip_secure_random(uint8 * buffer, uint16 length) 
	{ return BCRYPT_SUCCESS(BCryptGenRandom(NULL, buffer, length, BCRYPT_USE_SYSTEM_PREFERRED_RNG)); }
#endif
//
// zip-algorithm-deflate
// deflate (de)compression routines
//
struct zip_deflate_alg_ctx {
	zip_error_t * error;
	bool   compress;
	int    compression_flags;
	bool   end_of_input;
	z_stream zstr;
};

static void * deflate_allocate(bool compress, int compression_flags, zip_error_t * error) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)SAlloc::M(sizeof(*ctx));
	if(!ctx) {
		zip_error_set(error, ZIP_ET_SYS, errno);
	}
	else {
		ctx->error = error;
		ctx->compress = compress;
		ctx->compression_flags = compression_flags;
		if(ctx->compression_flags < 1 || ctx->compression_flags > 9) {
			ctx->compression_flags = Z_BEST_COMPRESSION;
		}
		ctx->end_of_input = false;
		ctx->zstr.zalloc = Z_NULL;
		ctx->zstr.zfree = Z_NULL;
		ctx->zstr.opaque = NULL;
	}
	return ctx;
}

static void * deflate_compress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return deflate_allocate(true, compression_flags, error); }
static void * deflate_decompress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return deflate_allocate(false, compression_flags, error); }

static void deflate_deallocate(void * ud) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	SAlloc::F(ctx);
}

static uint16 deflate_general_purpose_bit_flags(void * ud) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	if(!ctx->compress)
		return 0;
	if(ctx->compression_flags < 3)
		return 2 << 1;
	else if(ctx->compression_flags > 7)
		return 1 << 1;
	else
		return 0;
}

static bool deflate_start(void * ud) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	int ret;
	ctx->zstr.avail_in = 0;
	ctx->zstr.next_in = NULL;
	ctx->zstr.avail_out = 0;
	ctx->zstr.next_out = NULL;
	if(ctx->compress) {
		// negative value to tell zlib not to write a header 
		ret = deflateInit2(&ctx->zstr, ctx->compression_flags, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	}
	else {
		ret = inflateInit2(&ctx->zstr, -MAX_WBITS);
	}
	if(ret != Z_OK) {
		zip_error_set(ctx->error, SLERR_ZIP_ZLIB, ret);
		return false;
	}
	else
		return true;
}

static bool deflate_end(void * ud) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	int err = ctx->compress ? deflateEnd(&ctx->zstr) : inflateEnd(&ctx->zstr);
	if(err != Z_OK) {
		zip_error_set(ctx->error, SLERR_ZIP_ZLIB, err);
		return false;
	}
	else
		return true;
}

static bool deflate_input(void * ud, uint8 * data, uint64 length) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	if(length > UINT_MAX || ctx->zstr.avail_in > 0) {
		zip_error_set(ctx->error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	else {
		ctx->zstr.avail_in = (uInt)length;
		ctx->zstr.next_in = (Bytef*)data;
		return true;
	}
}

static void deflate_end_of_input(void * ud) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	ctx->end_of_input = true;
}

static zip_compression_status_t deflate_process(void * ud, uint8 * data, uint64 * length) 
{
	struct zip_deflate_alg_ctx * ctx = (struct zip_deflate_alg_ctx *)ud;
	int ret;
	ctx->zstr.avail_out = (uInt)MIN(UINT_MAX, *length);
	ctx->zstr.next_out = (Bytef*)data;
	if(ctx->compress) {
		ret = deflate(&ctx->zstr, ctx->end_of_input ? Z_FINISH : 0);
	}
	else {
		ret = inflate(&ctx->zstr, Z_SYNC_FLUSH);
	}
	*length = *length - ctx->zstr.avail_out;
	switch(ret) {
		case Z_OK: return ZIP_COMPRESSION_OK;
		case Z_STREAM_END: return ZIP_COMPRESSION_END;
		case Z_BUF_ERROR:
		    if(ctx->zstr.avail_in == 0) {
			    return ZIP_COMPRESSION_NEED_DATA;
		    }
		/* fallthrough */
		default:
		    zip_error_set(ctx->error, SLERR_ZIP_ZLIB, ret);
		    return ZIP_COMPRESSION_ERROR;
	}
}

/* clang-format off */

zip_compression_algorithm_t zip_algorithm_deflate_compress = {
	deflate_compress_allocate,
	deflate_deallocate,
	deflate_general_purpose_bit_flags,
	20,
	deflate_start,
	deflate_end,
	deflate_input,
	deflate_end_of_input,
	deflate_process
};

zip_compression_algorithm_t zip_algorithm_deflate_decompress = {
	deflate_decompress_allocate,
	deflate_deallocate,
	deflate_general_purpose_bit_flags,
	20,
	deflate_start,
	deflate_end,
	deflate_input,
	deflate_end_of_input,
	deflate_process
};

/* clang-format on */
//
// zip-algorithm-bzip2
// bzip2 (de)compression routines
//
struct zip_bzip2_alg_ctx {
	zip_error_t * error;
	bool compress;
	int compression_flags;
	bool end_of_input;
	bz_stream zstr;
};

static void * bzip2_allocate(bool compress, int compression_flags, zip_error_t * error) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)SAlloc::M(sizeof(*ctx));
	if(ctx) {
		ctx->error = error;
		ctx->compress = compress;
		ctx->compression_flags = inrangeordefault(compression_flags, 1, 9, 9);
		ctx->end_of_input = false;
		ctx->zstr.bzalloc = NULL;
		ctx->zstr.bzfree = NULL;
		ctx->zstr.opaque = NULL;
	}
	return ctx;
}

static void * bzip2_compress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return bzip2_allocate(true, compression_flags, error); }
static void * bzip2_decompress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return bzip2_allocate(false, compression_flags, error); }

static void bzip2_deallocate(void * ud) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	SAlloc::F(ctx);
}

static uint16 bzip2_general_purpose_bit_flags(void * ud) 
	{ return 0; }

static int bzip2_map_error(int ret) 
{
	switch(ret) {
		case BZ_FINISH_OK:
		case BZ_FLUSH_OK:
		case BZ_OK:
		case BZ_RUN_OK:
		case BZ_STREAM_END: return SLERR_SUCCESS;
		case BZ_DATA_ERROR:
		case BZ_DATA_ERROR_MAGIC:
		case BZ_UNEXPECTED_EOF: return SLERR_ZIP_COMPRESSED_DATA;
		case BZ_MEM_ERROR: return SLERR_ZIP_MEMORY;
		case BZ_PARAM_ERROR: return SLERR_ZIP_INVAL;
		case BZ_CONFIG_ERROR: /* actually, bzip2 miscompiled */
		case BZ_IO_ERROR:
		case BZ_OUTBUFF_FULL:
		case BZ_SEQUENCE_ERROR: return SLERR_ZIP_INTERNAL;
		default: return SLERR_ZIP_INTERNAL;
	}
}

static bool bzip2_start(void * ud) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	int ret;
	ctx->zstr.avail_in = 0;
	ctx->zstr.next_in = NULL;
	ctx->zstr.avail_out = 0;
	ctx->zstr.next_out = NULL;
	if(ctx->compress) {
		ret = BZ2_bzCompressInit(&ctx->zstr, ctx->compression_flags, 0, 30);
	}
	else {
		ret = BZ2_bzDecompressInit(&ctx->zstr, 0, 0);
	}
	if(ret != BZ_OK) {
		zip_error_set(ctx->error, bzip2_map_error(ret), 0);
		return false;
	}
	else
		return true;
}

static bool bzip2_end(void * ud) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	int err = ctx->compress ? BZ2_bzCompressEnd(&ctx->zstr) : BZ2_bzDecompressEnd(&ctx->zstr);
	if(err != BZ_OK) {
		zip_error_set(ctx->error, bzip2_map_error(err), 0);
		return false;
	}
	else
		return true;
}

static bool bzip2_input(void * ud, uint8 * data, uint64 length) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	if(length > UINT_MAX || ctx->zstr.avail_in > 0) {
		zip_error_set(ctx->error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	else {
		ctx->zstr.avail_in = (uint)length;
		ctx->zstr.next_in = (char *)data;
		return true;
	}
}

static void bzip2_end_of_input(void * ud) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	ctx->end_of_input = true;
}

static zip_compression_status_t bzip2_process(void * ud, uint8 * data, uint64 * length) 
{
	struct zip_bzip2_alg_ctx * ctx = (struct zip_bzip2_alg_ctx *)ud;
	int ret;
	if(ctx->zstr.avail_in == 0 && !ctx->end_of_input) {
		*length = 0;
		return ZIP_COMPRESSION_NEED_DATA;
	}
	ctx->zstr.avail_out = (uint)MIN(UINT_MAX, *length);
	ctx->zstr.next_out = (char *)data;
	if(ctx->compress) {
		ret = BZ2_bzCompress(&ctx->zstr, ctx->end_of_input ? BZ_FINISH : BZ_RUN);
	}
	else {
		ret = BZ2_bzDecompress(&ctx->zstr);
	}
	*length = *length - ctx->zstr.avail_out;
	switch(ret) {
		case BZ_FINISH_OK: /* compression */
		    return ZIP_COMPRESSION_OK;
		case BZ_OK: /* decompression */
		case BZ_RUN_OK: /* compression */
		    if(ctx->zstr.avail_in == 0) {
			    return ZIP_COMPRESSION_NEED_DATA;
		    }
		    return ZIP_COMPRESSION_OK;
		case BZ_STREAM_END:
		    return ZIP_COMPRESSION_END;
		default:
		    zip_error_set(ctx->error, bzip2_map_error(ret), 0);
		    return ZIP_COMPRESSION_ERROR;
	}
}

/* clang-format off */

zip_compression_algorithm_t zip_algorithm_bzip2_compress = {
	bzip2_compress_allocate,
	bzip2_deallocate,
	bzip2_general_purpose_bit_flags,
	46,
	bzip2_start,
	bzip2_end,
	bzip2_input,
	bzip2_end_of_input,
	bzip2_process
};

zip_compression_algorithm_t zip_algorithm_bzip2_decompress = {
	bzip2_decompress_allocate,
	bzip2_deallocate,
	bzip2_general_purpose_bit_flags,
	46,
	bzip2_start,
	bzip2_end,
	bzip2_input,
	bzip2_end_of_input,
	bzip2_process
};

/* clang-format on */
//
// zip-algorithm-xz
// XZ (de)compression routines
//
struct zip_xz_alg_ctx {
	zip_error_t * error;
	bool compress;
	uint32 compression_flags;
	bool end_of_input;
	lzma_stream zstr;
	uint16 method;
};

static void * xz_allocate(bool compress, int compression_flags, zip_error_t * error, uint16 method) 
{
	struct zip_xz_alg_ctx * ctx;
	if(compression_flags < 0) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = (struct zip_xz_alg_ctx *)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->error = error;
	ctx->compress = compress;
	ctx->compression_flags = (uint32)compression_flags;
	ctx->compression_flags |= LZMA_PRESET_EXTREME;
	ctx->end_of_input = false;
	memzero(&ctx->zstr, sizeof(ctx->zstr));
	ctx->method = method;
	return ctx;
}

static void * xz_compress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return xz_allocate(true, compression_flags, error, method); }
static void * xz_decompress_allocate(uint16 method, int compression_flags, zip_error_t * error) 
	{ return xz_allocate(false, compression_flags, error, method); }

static void xz_deallocate(void * ud) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	SAlloc::F(ctx);
}

static uint16 xz_general_purpose_bit_flags(void * ud) 
{
	// struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud; 
	return 0;
}

static int xz_map_error(lzma_ret ret) 
{
	switch(ret) {
		case LZMA_UNSUPPORTED_CHECK: return SLERR_ZIP_COMPRESSED_DATA;
		case LZMA_MEM_ERROR: return SLERR_ZIP_MEMORY;
		case LZMA_OPTIONS_ERROR: return SLERR_ZIP_INVAL;
		default: return SLERR_ZIP_INTERNAL;
	}
}

static bool xz_start(void * ud) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	lzma_ret ret;
	lzma_options_lzma opt_lzma;
	lzma_lzma_preset(&opt_lzma, ctx->compression_flags);
	lzma_filter filters[] = {
		{ (ctx->method == ZIP_CM_LZMA ? LZMA_FILTER_LZMA1 : LZMA_FILTER_LZMA2), &opt_lzma},
		{ LZMA_VLI_UNKNOWN, NULL},
	};
	ctx->zstr.avail_in = 0;
	ctx->zstr.next_in = NULL;
	ctx->zstr.avail_out = 0;
	ctx->zstr.next_out = NULL;
	if(ctx->compress) {
		if(ctx->method == ZIP_CM_LZMA)
			ret = lzma_alone_encoder(&ctx->zstr, (const lzma_options_lzma *)filters[0].options);
		else
			ret = lzma_stream_encoder(&ctx->zstr, filters, LZMA_CHECK_CRC64);
	}
	else {
		if(ctx->method == ZIP_CM_LZMA)
			ret = lzma_alone_decoder(&ctx->zstr, UINT64_MAX);
		else
			ret = lzma_stream_decoder(&ctx->zstr, UINT64_MAX, LZMA_CONCATENATED);
	}
	if(ret != LZMA_OK) {
		zip_error_set(ctx->error, xz_map_error(ret), 0);
		return false;
	}
	return true;
}

static bool xz_end(void * ud) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	lzma_end(&ctx->zstr);
	return true;
}

static bool xz_input(void * ud, uint8 * data, uint64 length) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	if(length > UINT_MAX || ctx->zstr.avail_in > 0) {
		zip_error_set(ctx->error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	else {
		ctx->zstr.avail_in = (uInt)length;
		ctx->zstr.next_in = (Bytef*)data;
		return true;
	}
}

static void xz_end_of_input(void * ud) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	ctx->end_of_input = true;
}

static zip_compression_status_t xz_process(void * ud, uint8 * data, uint64 * length) 
{
	struct zip_xz_alg_ctx * ctx = (struct zip_xz_alg_ctx *)ud;
	lzma_ret ret;
	ctx->zstr.avail_out = (uInt)MIN(UINT_MAX, *length);
	ctx->zstr.next_out = (Bytef*)data;
	ret = lzma_code(&ctx->zstr, ctx->end_of_input ? LZMA_FINISH : LZMA_RUN);
	*length = *length - ctx->zstr.avail_out;
	switch(ret) {
		case LZMA_OK: return ZIP_COMPRESSION_OK;
		case LZMA_STREAM_END: return ZIP_COMPRESSION_END;
		case LZMA_BUF_ERROR:
		    if(ctx->zstr.avail_in == 0) {
			    return ZIP_COMPRESSION_NEED_DATA;
		    }
		/* fallthrough */
		default:
		    zip_error_set(ctx->error, xz_map_error(ret), 0);
		    return ZIP_COMPRESSION_ERROR;
	}
}

/* clang-format off */

zip_compression_algorithm_t zip_algorithm_xz_compress = {
	xz_compress_allocate,
	xz_deallocate,
	xz_general_purpose_bit_flags,
	63,
	xz_start,
	xz_end,
	xz_input,
	xz_end_of_input,
	xz_process
};

zip_compression_algorithm_t zip_algorithm_xz_decompress = {
	xz_decompress_allocate,
	xz_deallocate,
	xz_general_purpose_bit_flags,
	63,
	xz_start,
	xz_end,
	xz_input,
	xz_end_of_input,
	xz_process
};

/* clang-format on */
