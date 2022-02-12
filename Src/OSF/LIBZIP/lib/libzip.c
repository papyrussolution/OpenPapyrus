// LIBZIP.C
//
//#define _ZIP_COMPILING_DEPRECATED
#include "zipint.h"
// Windows sys/types.h does not provide these 
#ifndef S_ISREG
	#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if defined(S_IXUSR) && defined(S_IRWXG) && defined(S_IRWXO)
	#define _SAFE_MASK (S_IXUSR | S_IRWXG | S_IRWXO)
#elif defined(_S_IWRITE)
	#define _SAFE_MASK (_S_IWRITE)
#else
	#error do not know safe values for umask, please report this
#endif
#ifdef _MSC_VER
	typedef int mode_t; // MSVC doesn't have mode_t 
#endif
//
// _zip_new:
// creates a new zipfile struct, and sets the contents to zero; returns
// the new struct
//
zip_t * _zip_new(zip_error_t * error)
{
	zip_t * za = static_cast<zip_t *>(SAlloc::M(sizeof(zip_t)));
	if(!za) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	}
	else {
		za->names = _zip_hash_new(ZIP_HASH_TABLE_SIZE, error);
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
			za->tempdir = NULL;
		}
	}
	return za;
}
//
//
//
/*const char * const _zip_err_str[] = {
    "No error",
    "Multi-disk zip archives not supported",
    "Renaming temporary file failed",
    "Closing zip archive failed",
    "Seek error",
    "Read error",
    "Write error",
    "CRC error",
    "Containing zip archive was closed",
    "No such file",
    "File already exists",
    "Can't open file",
    "Failure to create temporary file",
    "Zlib error",
    "Malloc failure",
    "Entry has been changed",
    "Compression method not supported",
    "Premature end of file",
    "Invalid argument",
    "Not a zip archive",
    "Internal error",
    "Zip archive inconsistent",
    "Can't remove file",
    "Entry has been deleted",
    "Encryption method not supported",
    "Read-only archive", 
    "No password provided",
    "Wrong password provided",
    "Operation not supported",
    "Resource still in use",
    "Tell error",
};*/

//const int _zip_nerr_str = sizeof(_zip_err_str)/sizeof(_zip_err_str[0]);

#define N ZIP_ET_NONE
#define S ZIP_ET_SYS
#define Z ZIP_ET_ZLIB

static int GetZipErrType(int errCode)
{
	if(oneof9(errCode, SLERR_ZIP_RENAME, SLERR_ZIP_CLOSE, SLERR_ZIP_SEEK, SLERR_ZIP_READ, SLERR_ZIP_WRITE, SLERR_ZIP_OPEN, 
		SLERR_ZIP_TMPOPEN, SLERR_ZIP_REMOVE, SLERR_ZIP_TELL))
		return ZIP_ET_SYS;
	else if(errCode == SLERR_ZIP_ZLIB)
		return ZIP_ET_ZLIB;
	else
		return ZIP_ET_NONE;
}

/*static const int _zip_err_type[] = { N, N, S, S, S, S, S, N, N, N, N, S, S, Z, N, N, N, N, N, N, N, N, S, N, N, N, N, N, N, N, S, };*/
//
// ZIPERROR
//
ZIP_EXTERN int zip_error_code_system(const zip_error_t * error) { return error->sys_err; }
ZIP_EXTERN int zip_error_code_zip(const zip_error_t * error) { return error->zip_err; }
ZIP_EXTERN void zip_error_fini(zip_error_t * err) { ZFREE(err->str); }

ZIP_EXTERN void FASTCALL zip_error_init(zip_error_t * err)
{
	err->zip_err = SLERR_SUCCESS;
	err->sys_err = 0;
	err->str = NULL;
}

/*ZIP_EXTERN void zip_error_init_with_code(zip_error_t * error, int ze)
{
	zip_error_init(error);
	error->zip_err = ze;
	switch(zip_error_system_type(error)) {
		case ZIP_ET_SYS:
		    error->sys_err = errno;
		    break;
		default:
		    error->sys_err = 0;
		    break;
	}
}*/

/*ZIP_EXTERN const char * zip_error_strerror(zip_error_t * err)
{
	const char * zs, * ss;
	char buf[128], * s;
	zip_error_fini(err);
	if(err->zip_err < 0 || err->zip_err >= SIZEOFARRAY(_zip_err_str)) {
		sprintf(buf, "Unknown error %d", err->zip_err);
		zs = NULL;
		ss = buf;
	}
	else {
		zs = _zip_err_str[err->zip_err];
		switch(_zip_err_type[err->zip_err]) {
			case ZIP_ET_SYS:
			    ss = strerror(err->sys_err);
			    break;
			case ZIP_ET_ZLIB:
			    ss = zError(err->sys_err);
			    break;
			default:
			    ss = NULL;
		}
	}
	if(ss == NULL)
		return zs;
	else {
		if((s = (char *)SAlloc::M(strlen(ss) + (zs ? strlen(zs)+2 : 0) + 1)) == NULL)
			return _zip_err_str[SLERR_ZIP_MEMORY];
		sprintf(s, "%s%s%s", (zs ? zs : ""), (zs ? ": " : ""), ss);
		err->str = s;
		return s;
	}
}*/

ZIP_EXTERN int zip_error_system_type(const zip_error_t * error)
{
	//return (error->zip_err >= SLERR_ZIP_FIRSTERROR && error->zip_err <= SLERR_ZIP_LASTERROR) ? _zip_err_type[error->zip_err-SLERR_ZIP_FIRSTERROR+1] : ZIP_ET_NONE;
	return error ? GetZipErrType(error->zip_err) : ZIP_ET_NONE;
}

// ZIP_EXTERN int zip_error_get_sys_type(int ze) { return (ze >= 0 && ze < SIZEOFARRAY(_zip_err_str)) ? _zip_err_type[ze] : 0; }

static void FASTCALL _zip_error_clear(zip_error_t * err)
{
	if(err) {
		err->zip_err = SLERR_SUCCESS;
		err->sys_err = 0;
	}
}

static void FASTCALL _zip_error_copy(zip_error_t * dst, const zip_error_t * src)
{
	dst->zip_err = src->zip_err;
	dst->sys_err = src->sys_err;
}

/*void _zip_error_get(const zip_error_t * err, int * zep, int * sep)
{
	ASSIGN_PTR(zep, err->zip_err);
	if(sep)
		*sep = (zip_error_system_type(err) != ZIP_ET_NONE) ? err->sys_err : 0;
}*/

int FASTCALL zip_error_set(zip_error_t * err, int ze, int se)
{
	if(err) {
		err->zip_err = ze;
		err->sys_err = se;
	}
	return -1; // strictly -1 (callers relys on it)
}

static void FASTCALL _zip_error_set_from_source(zip_error_t * err, zip_source_t * src)
{
	_zip_error_copy(err, zip_source_error(src));
}

int64 FASTCALL zip_error_to_data(const zip_error_t * error, void * data, uint64 length)
{
	int * e = static_cast<int *>(data);
	if(length < sizeof(int)*2) {
		return -1;
	}
	else {
		e[0] = zip_error_code_zip(error);
		e[1] = zip_error_code_system(error);
		return sizeof(int)*2;
	}
}

ZIP_EXTERN void zip_error_clear(zip_t * za)
{
	if(za)
		_zip_error_clear(&za->error);
}

// ZIP_EXTERN void zip_error_get(zip_t * za, int * zep, int * sep) { _zip_error_get(&za->error, zep, sep); }
ZIP_EXTERN zip_error_t * zip_get_error(zip_t * za) { return &za->error; }
ZIP_EXTERN zip_error_t * zip_file_get_error(zip_file_t * f) { return &f->error; }
// ZIP_EXTERN void zip_file_error_get(zip_file_t * zf, int * zep, int * sep) { _zip_error_get(&zf->error, zep, sep); }
// ZIP_EXTERN const char * zip_file_strerror(zip_file_t * zf) { return zip_error_strerror(&zf->error); }

ZIP_EXTERN void zip_file_error_clear(zip_file_t * zf)
{
	if(zf)
		_zip_error_clear(&zf->error);
}

/*ZIP_EXTERN int zip_error_to_str(char * buf, size_t len, int ze, int se)
{
	const char * zs, * ss;
	if(ze < 0 || ze >= SIZEOFARRAY(_zip_err_str))
		return _snprintf(buf, len, "Unknown error %d", ze);
	zs = _zip_err_str[ze];
	switch(_zip_err_type[ze]) {
		case ZIP_ET_SYS:
		    ss = strerror(se);
		    break;
		case ZIP_ET_ZLIB:
		    ss = zError(se);
		    break;
		default:
		    ss = NULL;
	}
	return _snprintf(buf, len, "%s%s%s", zs, (ss ? ": " : ""), (ss ? ss : ""));
}*/
//
// NOTE: Return type is signed so we can return -1 on error.
//   The index can not be larger than ZIP_INT64_MAX since the size
//   of the central directory cannot be larger than
//   ZIP_UINT64_MAX, and each entry is larger than 2 bytes.
//
ZIP_EXTERN int64 zip_add(zip_t * za, const char * name, zip_source_t * source) { return zip_file_add(za, name, source, 0); }

// NOTE: Signed due to -1 on error.  See zip_add.c for more details. 
ZIP_EXTERN int64 zip_add_dir(zip_t * za, const char * name) { return zip_dir_add(za, name, 0); }

// NOTE: Signed due to -1 on error.  See zip_add.c for more details.
int64 FASTCALL _zip_add_entry(zip_t * za)
{
	uint64 idx;
	if(za->nentry+1 >= za->nentry_alloc) {
		zip_entry_t * rentries;
		uint64 nalloc = za->nentry_alloc;
		uint64 realloc_size;
		const uint64 additional_entries = sclamp(2 * nalloc, 16ULL, 1024ULL);
		//uint64 additional_entries = 2 * nalloc;
		//if(additional_entries < 16) { additional_entries = 16; } else if(additional_entries > 1024) { additional_entries = 1024; }
		// neither + nor * overflows can happen: nentry_alloc * sizeof(struct zip_entry) < UINT64_MAX 
		nalloc += additional_entries;
		realloc_size = sizeof(struct zip_entry) * (size_t)nalloc;
		if(sizeof(struct zip_entry) * (size_t)za->nentry_alloc > realloc_size) {
			return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		rentries = static_cast<zip_entry_t *>(SAlloc::R(za->entry, sizeof(struct zip_entry) * (size_t)nalloc));
		if(!rentries) {
			return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		za->entry = rentries;
		za->nentry_alloc = nalloc;
	}
	idx = za->nentry++;
	_zip_entry_init(za->entry+idx);
	return (int64)idx;
}
//
// ZIPBUFFER
//
uint8 * FASTCALL _zip_buffer_data(zip_buffer_t * buffer) { return buffer->data; }
bool FASTCALL _zip_buffer_eof(const zip_buffer_t * buffer) { return buffer->ok && buffer->offset == buffer->size; }

void FASTCALL _zip_buffer_free(zip_buffer_t * buffer)
{
	if(buffer) {
		if(buffer->free_data)
			SAlloc::F(buffer->data);
		SAlloc::F(buffer);
	}
}

uint8 * FASTCALL _zip_buffer_get(zip_buffer_t * buffer, uint64 length)
{
	if(!buffer->ok || buffer->offset + length < length || buffer->offset + length > buffer->size) {
		buffer->ok = false;
		return NULL;
	}
	else {
		uint8 * data = buffer->data + buffer->offset;
		buffer->offset += length;
		return data;
	}
}

uint16 FASTCALL _zip_buffer_get_16(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 2);
	return data ? static_cast<uint16>(data[0] + (data[1] << 8)) : 0;
}

uint32 FASTCALL _zip_buffer_get_32(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 4);
	return data ? (((((((uint32)data[3] << 8) + data[2]) << 8) + data[1]) << 8) + data[0]) : 0;
}

uint64 FASTCALL _zip_buffer_get_64(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 8);
	if(data == NULL) {
		return 0;
	}
	return ((uint64)data[7] << 56) + ((uint64)data[6] << 48) + ((uint64)data[5] << 40) + 
		((uint64)data[4] << 32) + ((uint64)data[3] << 24) + ((uint64)data[2] << 16) + ((uint64)data[1] << 8) + (uint64)data[0];
}

uint8 FASTCALL _zip_buffer_get_8(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 1);
	return data ? data[0] : 0;
}

uint64 FASTCALL _zip_buffer_left(const zip_buffer_t * buffer)
{
	return buffer->ok ? (buffer->size - buffer->offset) : 0;
}

zip_buffer_t * FASTCALL _zip_buffer_new(uint8 * data, size_t size)
{
	const bool free_data = (data == NULL);
	zip_buffer_t * buffer = 0;
	if(SETIFZ(data, static_cast<uint8 *>(SAlloc::M(size)))) {
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

zip_buffer_t * _zip_buffer_new_from_source(zip_source_t * src, size_t size, uint8 * buf, zip_error_t * error)
{
	zip_buffer_t * buffer = _zip_buffer_new(buf, size);
	if(!buffer) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	else if(_zip_read(src, buffer->data, size, error) < 0) {
		_zip_buffer_free(buffer);
		return NULL;
	}
	else
		return buffer;
}

static uint64 FASTCALL _zip_buffer_offset(const zip_buffer_t * buffer) { return buffer->ok ? buffer->offset : 0; }
static bool   FASTCALL _zip_buffer_ok(const zip_buffer_t * buffer) { return buffer->ok; }

int FASTCALL _zip_buffer_put(zip_buffer_t * buffer, const void * src, size_t length)
{
	uint8 * dst = _zip_buffer_get(buffer, length);
	if(!dst)
		return -1;
	else {
		memcpy(dst, src, length);
		return 0;
	}
}

int FASTCALL _zip_buffer_put_16(zip_buffer_t * buffer, uint16 i)
{
	uint8 * data = _zip_buffer_get(buffer, 2);
	if(data) {
		data[0] = (uint8)(i & 0xff);
		data[1] = (uint8)((i >> 8) & 0xff);
		return 0;
	}
	else
		return -1;
}

int FASTCALL _zip_buffer_put_32(zip_buffer_t * buffer, uint32 i)
{
	uint8 * data = _zip_buffer_get(buffer, 4);
	if(data) {
		data[0] = (uint8)(i & 0xff);
		data[1] = (uint8)((i >> 8) & 0xff);
		data[2] = (uint8)((i >> 16) & 0xff);
		data[3] = (uint8)((i >> 24) & 0xff);
		return 0;
	}
	else
		return -1;
}

int FASTCALL _zip_buffer_put_64(zip_buffer_t * buffer, uint64 i)
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

int FASTCALL _zip_buffer_put_8(zip_buffer_t * buffer, uint8 i)
{
	uint8 * data = _zip_buffer_get(buffer, 1);
	if(data == NULL) {
		return -1;
	}
	else {
		data[0] = i;
		return 0;
	}
}

int FASTCALL _zip_buffer_set_offset(zip_buffer_t * buffer, uint64 offset)
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

int FASTCALL _zip_buffer_skip(zip_buffer_t * buffer, uint64 length)
{
	uint64 offset = buffer->offset + length;
	if(offset < buffer->offset) {
		buffer->ok = false;
		return -1;
	}
	else
		return _zip_buffer_set_offset(buffer, offset);
}

uint64 FASTCALL _zip_buffer_size(const zip_buffer_t * buffer)
{
	return buffer->size;
}
//
// ZIPDELETE
//
ZIP_EXTERN int zip_delete(zip_t * za, uint64 idx)
{
	const char * name;
	if(idx >= za->nentry) {
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	}
	if(ZIP_IS_RDONLY(za)) {
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	}
	if((name = _zip_get_name(za, idx, 0, &za->error)) == NULL) {
		return -1;
	}
	if(!_zip_hash_delete(za->names, (const uint8 *)name, &za->error)) {
		return -1;
	}
	// allow duplicate file names, because the file will
	// be removed directly afterwards 
	if(_zip_unchange(za, idx, 1) != 0)
		return -1;
	else {
		za->entry[idx].deleted = 1;
		return 0;
	}
}
//
// ZIP_DIR_ADD
//
// NOTE: Signed due to -1 on error.  See zip_add.c for more details
ZIP_EXTERN int64 zip_dir_add(zip_t * za, const char * name, zip_flags_t flags)
{
	size_t len;
	int64 idx;
	char * s;
	zip_source_t * source;
	if(ZIP_IS_RDONLY(za)) {
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	}
	if(!name) {
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	}
	s = NULL;
	len = strlen(name);
	if(name[len-1] != '/') {
		if((s = (char *)SAlloc::M(len+2)) == NULL) {
			return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		strcpy(s, name);
		s[len] = '/';
		s[len+1] = '\0';
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
// zip_discard:
// frees the space allocated to a zipfile struct, and closes the
// corresponding file
//
void zip_discard(zip_t * za)
{
	if(za) {
		uint64 i;
		if(za->src) {
			zip_source_close(za->src);
			zip_source_free(za->src);
		}
		SAlloc::F(za->default_password);
		_zip_string_free(za->comment_orig);
		_zip_string_free(za->comment_changes);
		_zip_hash_free(za->names);
		if(za->entry) {
			for(i = 0; i<za->nentry; i++)
				_zip_entry_finalize(za->entry+i);
			SAlloc::F(za->entry);
		}
		for(i = 0; i<za->nopen_source; i++) {
			_zip_source_invalidate(za->open_source[i]);
		}
		SAlloc::F(za->open_source);
		zip_error_fini(&za->error);
		SAlloc::F(za);
	}
}
//
// ZIPENTRY
//
static void _zip_unchange_data(zip_entry_t * ze)
{
	if(ze->source) {
		zip_source_free(ze->source);
		ze->source = NULL;
	}
	if(ze->changes && (ze->changes->changed & ZIP_DIRENT_COMP_METHOD) && ze->changes->comp_method == ZIP_CM_REPLACED_DEFAULT) {
		ze->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
		if(ze->changes->changed == 0) {
			_zip_dirent_free(ze->changes);
			ze->changes = NULL;
		}
	}
	ze->deleted = 0;
}

void _zip_entry_finalize(zip_entry_t * e)
{
	_zip_unchange_data(e);
	_zip_dirent_free(e->orig);
	_zip_dirent_free(e->changes);
}

void _zip_entry_init(zip_entry_t * e)
{
	e->orig = NULL;
	e->changes = NULL;
	e->source = NULL;
	e->deleted = 0;
}
//
//
//
ZIP_EXTERN int64 zip_get_num_entries(const zip_t * za, zip_flags_t flags)
{
	uint64 n;
	if(za == NULL)
		return -1;
	if(flags & ZIP_FL_UNCHANGED) {
		n = za->nentry;
		while(n > 0 && za->entry[n-1].orig == NULL)
			--n;
		return (int64)n;
	}
	return (int64)za->nentry;
}
//
//
//
ZIP_EXTERN const char * zip_get_name(zip_t * za, uint64 idx, zip_flags_t flags)
{
	return _zip_get_name(za, idx, flags, &za->error);
}

const char * _zip_get_name(zip_t * za, uint64 idx, zip_flags_t flags, zip_error_t * error)
{
	zip_dirent_t * de = _zip_get_dirent(za, idx, flags, error);
	const uint8 * str = de ? _zip_string_get(de->filename, NULL, flags, error) : 0;
	return reinterpret_cast<const char *>(str);
}

ZIP_EXTERN int zip_get_num_files(zip_t * za)
{
	if(za == NULL)
		return -1;
	else if(za->nentry > INT_MAX)
		return zip_error_set(&za->error, SLERR_ZIP_OPNOTSUPP, 0);
	else
		return (int)za->nentry;
}

static void * _zip_memdup(const void * mem, size_t len, zip_error_t * error)
{
	void * ret = 0;
	if(len) {
		ret = SAlloc::M(len);
		if(!ret) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return NULL;
		}
		memcpy(ret, mem, len);
	}
	return ret;
}
//
//
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

static int _zip_stat_merge(zip_stat_t * dst, const zip_stat_t * src, zip_error_t * error)
{
	// name is not merged, since zip_stat_t doesn't own it, and src may not be valid as long as dst 
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

ZIP_EXTERN int zip_stat(zip_t * za, const char * fname, zip_flags_t flags, zip_stat_t * st)
{
	const int64 idx = zip_name_locate(za, fname, flags);
	return (idx < 0) ? -1 : zip_stat_index(za, (uint64)idx, flags, st);
}

ZIP_EXTERN int zip_stat_index(zip_t * za, uint64 index, zip_flags_t flags, zip_stat_t * st)
{
	const char * name;
	zip_dirent_t * de;
	if((de = _zip_get_dirent(za, index, flags, NULL)) == NULL)
		return -1;
	if((name = zip_get_name(za, index, flags)) == NULL)
		return -1;
	if((flags & ZIP_FL_UNCHANGED) == 0 && ZIP_ENTRY_DATA_CHANGED(za->entry+index)) {
		if(zip_source_stat(za->entry[index].source, st) < 0) {
			return zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
		}
	}
	else {
		zip_stat_init(st);
		st->crc = de->crc;
		st->size = de->uncomp_size;
		st->mtime = de->last_mod;
		st->comp_size = de->comp_size;
		st->comp_method = (uint16)de->comp_method;
		if(de->bitflags & ZIP_GPBF_ENCRYPTED) {
			if(de->bitflags & ZIP_GPBF_STRONG_ENCRYPTION) {
				/* @todo */
				st->encryption_method = ZIP_EM_UNKNOWN;
			}
			else
				st->encryption_method = ZIP_EM_TRAD_PKWARE;
		}
		else
			st->encryption_method = ZIP_EM_NONE;
		st->valid = ZIP_STAT_CRC|ZIP_STAT_SIZE|ZIP_STAT_MTIME
		    |ZIP_STAT_COMP_SIZE|ZIP_STAT_COMP_METHOD|ZIP_STAT_ENCRYPTION_METHOD;
	}
	st->index = index;
	st->name = name;
	st->valid |= ZIP_STAT_INDEX|ZIP_STAT_NAME;
	return 0;
}

ZIP_EXTERN int zip_source_stat(zip_source_t * src, zip_stat_t * st)
{
	if(src->source_closed) {
		return -1;
	}
	if(st == NULL) {
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
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

ZIP_EXTERN void zip_source_keep(zip_source_t * src)
{
	src->refcount++;
}

zip_source_t * _zip_source_new(zip_error_t * error)
{
	zip_source_t * src = static_cast<zip_source_t *>(SAlloc::M(sizeof(*src)));
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
	}
	return src;
}

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

ZIP_EXTERN void zip_source_rollback_write(zip_source_t * src)
{
	if(oneof2(src->write_state, ZIP_SOURCE_WRITE_OPEN, ZIP_SOURCE_WRITE_FAILED)) {
		_zip_source_call(src, NULL, 0, ZIP_SOURCE_ROLLBACK_WRITE);
		src->write_state = ZIP_SOURCE_WRITE_CLOSED;
	}
}

int64 zip_source_supports(const zip_source_t * src)
{
	return src->supports;
}

ZIP_EXTERN int64 zip_source_make_command_bitmap(zip_source_cmd_t cmd0, ...)
{
	va_list ap;
	int64 bitmap = ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd0);
	va_start(ap, cmd0);
	for(;; ) {
		int cmd = va_arg(ap, int);
		if(cmd < 0) {
			break;
		}
		bitmap |= (int64)ZIP_SOURCE_MAKE_COMMAND_BITMASK(cmd);
	}
	va_end(ap);
	return bitmap;
}

struct ZipSourceWindow {
	uint64 start;
	uint64 end;
	uint64 offset;
	zip_stat_t stat;
	zip_error_t error;
	int64 supports;
	bool needs_seek;
};

static int64 window_read(zip_source_t * src, void * _ctx, void * data, uint64 len, zip_source_cmd_t cmd)
{
	int64 ret;
	uint64 n, i;
	char b[8192];
	struct ZipSourceWindow * ctx = static_cast<struct ZipSourceWindow *>(_ctx);
	switch(cmd) {
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_OPEN:
		    if(!ctx->needs_seek) {
			    for(n = 0; n<ctx->start; n += (uint64)ret) {
				    i = (ctx->start-n > sizeof(b) ? sizeof(b) : ctx->start-n);
				    if((ret = zip_source_read(src, b, i)) < 0) {
					    _zip_error_set_from_source(&ctx->error, src);
					    return -1;
				    }
				    if(!ret)
					    return zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
			    }
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
			ret = zip_source_read(src, data, len);
		    if(ret < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
		    ctx->offset += (uint64)ret;
		    if(!ret) {
			    if(ctx->offset < ctx->end)
				    return zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
		    }
		    return ret;
		case ZIP_SOURCE_SEEK:
	    {
		    int64 new_offset = zip_source_seek_compute_offset(ctx->offset - ctx->start, ctx->end - ctx->start, data, len, &ctx->error);
		    if(new_offset < 0) {
			    return -1;
		    }
		    ctx->offset = (uint64)new_offset + ctx->start;
		    return 0;
	    }
		case ZIP_SOURCE_STAT:
	    {
		    zip_stat_t * st = static_cast<zip_stat_t *>(data);
			return (_zip_stat_merge(st, &ctx->stat, &ctx->error) >= 0) ? 0 : -1;
	    }
		case ZIP_SOURCE_SUPPORTS:
		    return ctx->supports;
		case ZIP_SOURCE_TELL:
		    return (int64)(ctx->offset - ctx->start);
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}

zip_source_t * zip_source_window(zip_t * za, zip_source_t * src, uint64 start, uint64 len)
{
	return _zip_source_window_new(src, start, len, NULL, &za->error);
}

zip_source_t * _zip_source_window_new(zip_source_t * src, uint64 start, uint64 length, zip_stat_t * st, zip_error_t * error)
{
	struct ZipSourceWindow * ctx;
	if(src == NULL || start + length < start) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = static_cast<struct ZipSourceWindow *>(SAlloc::M(sizeof(*ctx)))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->start = start;
	ctx->end = start + length;
	zip_stat_init(&ctx->stat);
	zip_error_init(&ctx->error);
	ctx->supports = (zip_source_supports(src) & ZIP_SOURCE_SUPPORTS_SEEKABLE) | (zip_source_make_command_bitmap(ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1));
	ctx->needs_seek = LOGIC(ctx->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK));
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
//
// called by zip_discard to avoid operating on file from closed archive 
//
void _zip_source_invalidate(zip_source_t * src)
{
	src->source_closed = 1;
	if(zip_error_code_zip(&src->error) == SLERR_SUCCESS) {
		zip_error_set(&src->error, SLERR_ZIP_ZIPCLOSED, 0);
	}
}

void _zip_deregister_source(zip_t * za, zip_source_t * src)
{
	for(uint i = 0; i<za->nopen_source; i++) {
		if(za->open_source[i] == src) {
			za->open_source[i] = za->open_source[za->nopen_source-1];
			za->nopen_source--;
			break;
		}
	}
}

int _zip_register_source(zip_t * za, zip_source_t * src)
{
	zip_source_t ** open_source;
	if(za->nopen_source+1 >= za->nopen_source_alloc) {
		uint n = za->nopen_source_alloc + 10;
		open_source = static_cast<zip_source_t **>(SAlloc::R(za->open_source, n*sizeof(zip_source_t *)));
		if(!open_source)
			return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		za->nopen_source_alloc = n;
		za->open_source = open_source;
	}
	za->open_source[za->nopen_source++] = src;
	return 0;
}
//
//
//
struct trad_pkware {
	zip_error_t error;
	uint32 key[3];
};

#define HEADERLEN       12
#define KEY0            305419896
#define KEY1            591751049
#define KEY2            878082192

static void decrypt(struct trad_pkware *, uint8 *, const uint8 *, uint64, int);
static int64 pkware_decrypt(zip_source_t *, void *, void *, uint64, zip_source_cmd_t);

zip_source_t * zip_source_pkware(zip_t * za, zip_source_t * src, uint16 em, int flags, const char * password)
{
	struct trad_pkware * ctx;
	zip_source_t * s2 = 0;
	if(!password || !src || em != ZIP_EM_TRAD_PKWARE) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	}
	else if(flags & ZIP_CODEC_ENCODE) {
		zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
	}
	else if((ctx = (struct trad_pkware*)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
	}
	else {
		zip_error_init(&ctx->error);
		ctx->key[0] = KEY0;
		ctx->key[1] = KEY1;
		ctx->key[2] = KEY2;
		decrypt(ctx, NULL, reinterpret_cast<const uint8 *>(password), strlen(password), 1);
		if((s2 = zip_source_layered(za, src, pkware_decrypt, ctx)) == NULL)
			SAlloc::F(ctx);
	}
	return s2;
}

static void decrypt(struct trad_pkware * ctx, uint8 * out, const uint8 * in, uint64 len, int update_only)
{
	for(uint64 i = 0; i < len; i++) {
		Bytef b = in[i];
		if(!update_only) {
			// decrypt next byte 
			uint16 tmp = static_cast<uint16>(ctx->key[2] | 2);
			tmp = static_cast<uint16>(((uint32)tmp * (tmp ^ 1)) >> 8);
			b ^= (Bytef)tmp;
		}
		// store cleartext 
		if(out)
			out[i] = b;
		// update keys 
		ctx->key[0] = (uint32)crc32(ctx->key[0] ^ 0xffffffffUL, &b, 1) ^ 0xffffffffUL;
		ctx->key[1] = (ctx->key[1] + (ctx->key[0] & 0xff)) * 134775813 + 1;
		b = (Bytef)(ctx->key[1] >> 24);
		ctx->key[2] = (uint32)crc32(ctx->key[2] ^ 0xffffffffUL, &b, 1) ^ 0xffffffffUL;
	}
}

static int FASTCALL decrypt_header(zip_source_t * src, struct trad_pkware * ctx)
{
	uint8 header[HEADERLEN];
	int64 n;
	if((n = zip_source_read(src, header, HEADERLEN)) < 0) {
		_zip_error_set_from_source(&ctx->error, src);
		return -1;
	}
	else if(n != HEADERLEN)
		return zip_error_set(&ctx->error, SLERR_ZIP_EOF, 0);
	else {
		zip_stat_t st;
		decrypt(ctx, header, header, HEADERLEN, 0);
		if(zip_source_stat(src, &st) < 0) {
			return 0; // stat failed, skip password validation 
		}
		else {
			ushort dostime, dosdate;
			_zip_u2d_time(st.mtime, &dostime, &dosdate);
			if(header[HEADERLEN-1] != st.crc>>24 && header[HEADERLEN-1] != dostime>>8)
				return zip_error_set(&ctx->error, SLERR_ZIP_WRONGPASSWD, 0);
			else
				return 0;
		}
	}
}

static int64 pkware_decrypt(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd)
{
	int64 n;
	struct trad_pkware * ctx = static_cast<struct trad_pkware *>(ud);
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    return (decrypt_header(src, ctx) < 0) ? -1 : 0;
		case ZIP_SOURCE_READ:
		    if((n = zip_source_read(src, data, len)) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
			else {
				decrypt((struct trad_pkware*)ud, static_cast<uint8 *>(data), static_cast<uint8 *>(data), (uint64)n, 0);
				return n;
			}
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT:
	    {
		    zip_stat_t * st = static_cast<zip_stat_t *>(data);
		    st->encryption_method = ZIP_EM_NONE;
		    st->valid |= ZIP_STAT_ENCRYPTION_METHOD;
		    // @todo deduce HEADERLEN from size for uncompressed 
		    if(st->valid & ZIP_STAT_COMP_SIZE)
			    st->comp_size -= HEADERLEN;
		    return 0;
	    }
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, -1);
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx);
		    return 0;
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
	}
}
//
//
//
ZIP_EXTERN zip_file_t * zip_fopen(zip_t * za, const char * fname, zip_flags_t flags)
{
	int64 idx = zip_name_locate(za, fname, flags);
	return (idx >= 0) ? zip_fopen_index_encrypted(za, (uint64)idx, flags, za->default_password) : 0;
}

ZIP_EXTERN int zip_fclose(zip_file_t * zf)
{
	int ret = 0;
	if(zf) {
		zip_source_free(zf->src);
		if(zf->error.zip_err)
			ret = zf->error.zip_err;
		zip_error_fini(&zf->error);
		SAlloc::F(zf);
	}
	return ret;
}

ZIP_EXTERN zip_t * zip_fdopen(int fd_orig, int _flags, int * zep)
{
	int fd;
	FILE * fp;
	zip_t * za;
	zip_source_t * src;
	zip_error_t error;
	if(_flags < 0 || (_flags & ZIP_TRUNCATE)) {
		_zip_set_open_error(zep, NULL, SLERR_ZIP_INVAL);
		return NULL;
	}
	/* We dup() here to avoid messing with the passed in fd.
	   We could not restore it to the original state in case of error. */
	if((fd = _dup(fd_orig)) < 0) {
		_zip_set_open_error(zep, NULL, SLERR_ZIP_OPEN);
		return NULL;
	}
	if((fp = _fdopen(fd, "rb")) == NULL) {
		_close(fd);
		_zip_set_open_error(zep, NULL, SLERR_ZIP_OPEN);
		return NULL;
	}
	zip_error_init(&error);
	if((src = zip_source_filep_create(fp, 0, -1, &error)) == NULL) {
		_zip_set_open_error(zep, &error, 0);
		zip_error_fini(&error);
		return NULL;
	}
	if((za = zip_open_from_source(src, _flags, &error)) == NULL) {
		_zip_set_open_error(zep, &error, 0);
		zip_error_fini(&error);
		return NULL;
	}
	zip_error_fini(&error);
	_close(fd_orig);
	return za;
}

ZIP_EXTERN zip_file_t * zip_fopen_encrypted(zip_t * za, const char * fname, zip_flags_t flags, const char * password)
{
	int64 idx = zip_name_locate(za, fname, flags);
	return (idx >= 0) ? zip_fopen_index_encrypted(za, (uint64)idx, flags, password) : 0;
}

ZIP_EXTERN zip_file_t * zip_fopen_index(zip_t * za, uint64 index, zip_flags_t flags)
{
	return zip_fopen_index_encrypted(za, index, flags, za->default_password);
}

static zip_file_t * _zip_file_new(zip_t * za)
{
	zip_file_t * zf = static_cast<zip_file_t *>(SAlloc::M(sizeof(zip_file_t)));
	if(!zf) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
	}
	else {
		zf->za = za;
		zip_error_init(&zf->error);
		zf->eof = 0;
		zf->src = NULL;
	}
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

ZIP_EXTERN int64 zip_file_add(zip_t * za, const char * name, zip_source_t * source, zip_flags_t flags)
{
	if(name == NULL || source == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else
		return _zip_file_replace(za, ZIP_UINT64_MAX, name, source, flags);
}

ZIP_EXTERN int64 zip_fread(zip_file_t * zf, void * outbuf, uint64 toread)
{
	int64 n = -1;
	if(zf && zf->error.zip_err == 0) {
		if(toread > ZIP_INT64_MAX) {
			zip_error_set(&zf->error, SLERR_ZIP_INVAL, 0);
		}
		else {
			n = (zf->eof || !toread) ? 0 : zip_source_read(zf->src, outbuf, toread);
			if(n < 0)
				_zip_error_set_from_source(&zf->error, zf->src);
		}
	}
	return n;
}
//
// lenp is 32 bit because converted comment can be longer than ZIP_UINT16_MAX 
//
ZIP_EXTERN const char * zip_file_get_comment(zip_t * za, uint64 idx, uint32 * lenp, zip_flags_t flags)
{
	zip_dirent_t * de;
	uint32 len;
	const uint8 * str;
	if((de = _zip_get_dirent(za, idx, flags, NULL)) == NULL)
		return NULL;
	else if((str = _zip_string_get(de->comment, &len, flags, &za->error)) == NULL)
		return NULL;
	else {
		ASSIGN_PTR(lenp, len);
		return (const char *)str;
	}
}

int zip_file_get_external_attributes(zip_t * za, uint64 idx, zip_flags_t flags, uint8 * opsys, uint32 * attributes)
{
	zip_dirent_t * de = _zip_get_dirent(za, idx, flags, 0);
	if(de == NULL)
		return -1;
	else {
		ASSIGN_PTR(opsys, (uint8)((de->version_madeby >> 8) & 0xff));
		ASSIGN_PTR(attributes, de->ext_attrib);
		return 0;
	}
}
//
// _zip_file_get_offset(za, ze):
// Returns the offset of the file data for entry ze.
//
// On error, fills in za->error and returns 0.
//
uint64 _zip_file_get_offset(const zip_t * za, uint64 idx, zip_error_t * error)
{
	int32 size;
	uint64 offset = za->entry[idx].orig->offset;
	if(zip_source_seek(za->src, (int64)offset, SEEK_SET) < 0) {
		_zip_error_set_from_source(error, za->src);
		return 0;
	}
	/* @todo cache? */
	if((size = _zip_dirent_size(za->src, ZIP_EF_LOCAL, error)) < 0)
		return 0;
	if(offset+(uint32)size > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return 0;
	}
	return offset + (uint32)size;
}
//
//
//
ZIP_EXTERN int zip_file_rename(zip_t * za, uint64 idx, const char * name, zip_flags_t flags)
{
	const char * old_name;
	int old_is_dir, new_is_dir;
	if(idx >= za->nentry || (name != NULL && strlen(name) > ZIP_UINT16_MAX))
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if((old_name = zip_get_name(za, idx, 0)) == NULL)
		return -1;
	new_is_dir = (name != NULL && name[strlen(name)-1] == '/');
	old_is_dir = (old_name[strlen(old_name)-1] == '/');
	if(new_is_dir != old_is_dir)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	return _zip_set_name(za, idx, name, flags);
}

ZIP_EXTERN int zip_rename(zip_t * za, uint64 idx, const char * name)
{
	return zip_file_rename(za, idx, name, 0);
}

ZIP_EXTERN int zip_file_replace(zip_t * za, uint64 idx, zip_source_t * source, zip_flags_t flags)
{
	if(idx >= za->nentry || source == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else if(_zip_file_replace(za, idx, NULL, source, flags) == -1)
		return -1;
	else
		return 0;
}
//
// NOTE: Signed due to -1 on error.  See zip_add.c for more details
//
int64 _zip_file_replace(zip_t * za, uint64 idx, const char * name, zip_source_t * source, zip_flags_t flags)
{
	uint64 za_nentry_prev;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	za_nentry_prev = za->nentry;
	if(idx == ZIP_UINT64_MAX) {
		int64 i = -1;
		if(flags & ZIP_FL_OVERWRITE)
			i = _zip_name_locate(za, name, flags, 0);
		if(i == -1) {
			// create and use new entry, used by zip_add 
			if((i = _zip_add_entry(za)) < 0)
				return -1;
		}
		idx = (uint64)i;
	}
	if(name && _zip_set_name(za, idx, name, flags) != 0) {
		if(za->nentry != za_nentry_prev) {
			_zip_entry_finalize(za->entry+idx);
			za->nentry = za_nentry_prev;
		}
		return -1;
	}
	/* does not change any name related data, so we can do it here;
	 * needed for a double add of the same file name */
	_zip_unchange_data(za->entry+idx);
	if(za->entry[idx].orig && (za->entry[idx].changes == NULL || (za->entry[idx].changes->changed & ZIP_DIRENT_COMP_METHOD) == 0)) {
		if(za->entry[idx].changes == NULL) {
			if((za->entry[idx].changes = _zip_dirent_clone(za->entry[idx].orig)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		za->entry[idx].changes->comp_method = ZIP_CM_REPLACED_DEFAULT;
		za->entry[idx].changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}
	za->entry[idx].source = source;
	return (int64)idx;
}

ZIP_EXTERN int zip_replace(zip_t * za, uint64 idx, zip_source_t * source)
{
	return zip_file_replace(za, idx, source, 0);
}
//
// SOURCE
//
#ifndef WRITE_FRAGMENT_SIZE
	#define WRITE_FRAGMENT_SIZE 64*1024
#endif

struct ZipSourceBuffer {
	uint64 fragment_size; /* size of each fragment */
	uint8 ** fragments; /* pointers to fragments */
	uint64 nfragments; /* number of allocated fragments */
	uint64 fragments_capacity; /* size of fragments (number of pointers) */
	uint64 size;                    /* size of data in bytes */
	uint64 offset; /* current offset */
	int    free_data;
};

struct read_data {
	zip_error_t error;
	time_t mtime;
	ZipSourceBuffer * in;
	ZipSourceBuffer * out;
};

static void FASTCALL buffer_free(ZipSourceBuffer * buffer)
{
	if(buffer) {
		if(buffer->free_data) {
			for(uint64 i = 0; i < buffer->nfragments; i++) {
				SAlloc::F(buffer->fragments[i]);
			}
		}
		SAlloc::F(buffer->fragments);
		SAlloc::F(buffer);
	}
}

static ZipSourceBuffer * FASTCALL buffer_new(uint64 fragment_size)
{
	ZipSourceBuffer * buffer = (ZipSourceBuffer *)SAlloc::M(sizeof(*buffer));
	if(buffer) {
		buffer->fragment_size = fragment_size;
		buffer->offset = 0;
		buffer->free_data = 0;
		buffer->nfragments = 0;
		buffer->fragments_capacity = 0;
		buffer->fragments = NULL;
		buffer->size = 0;
	}
	return buffer;
}

static ZipSourceBuffer * buffer_new_read(const void * data, uint64 length, int free_data)
{
	ZipSourceBuffer * buffer = buffer_new(length);
	if(buffer) {
		buffer->size = length;
		if(length > 0) {
			if((buffer->fragments = (uint8 **)SAlloc::M(sizeof(*(buffer->fragments)))) == NULL) {
				buffer_free(buffer);
				return NULL;
			}
			buffer->fragments_capacity = 1;
			buffer->nfragments = 1;
			buffer->fragments[0] = (uint8 *)data;
			buffer->free_data = free_data;
		}
	}
	return buffer;
}

static ZipSourceBuffer * buffer_new_write(uint64 fragment_size)
{
	ZipSourceBuffer * buffer = buffer_new(fragment_size);
	if(buffer) {
		buffer->fragments = (uint8 **)SAlloc::M(sizeof(*(buffer->fragments)));
		if(buffer->fragments == NULL) {
			buffer_free(buffer);
			buffer = 0;
		}
		else {
			buffer->fragments_capacity = 1;
			buffer->nfragments = 0;
			buffer->free_data = 1;
		}
	}
	return buffer;
}

static int64 buffer_read(ZipSourceBuffer * buffer, uint8 * data, uint64 length)
{
	uint64 n, i, fragment_offset;
	length = MIN(length, buffer->size - buffer->offset);
	if(length == 0) {
		return 0;
	}
	if(length > ZIP_INT64_MAX) {
		return -1;
	}
	i = buffer->offset / buffer->fragment_size;
	fragment_offset = buffer->offset % buffer->fragment_size;
	n = 0;
	while(n < length) {
		uint64 left = MIN(length - n, buffer->fragment_size - fragment_offset);
		memcpy(data + n, buffer->fragments[i] + fragment_offset, (size_t)left);
		n += left;
		i++;
		fragment_offset = 0;
	}
	buffer->offset += n;
	return (int64)n;
}

static int buffer_seek(ZipSourceBuffer * buffer, void * data, uint64 len, zip_error_t * error)
{
	int64 new_offset = zip_source_seek_compute_offset(buffer->offset, buffer->size, data, len, error);
	if(new_offset < 0) {
		return -1;
	}
	else {
		buffer->offset = (uint64)new_offset;
		return 0;
	}
}

static int64 buffer_write(ZipSourceBuffer * buffer, const uint8 * data, uint64 length, zip_error_t * error)
{
	uint64 n, i, fragment_offset;
	uint8 ** fragments;
	if(buffer->offset + length + buffer->fragment_size - 1 < length)
		return zip_error_set(error, SLERR_ZIP_INVAL, 0);
	// grow buffer if needed 
	if(buffer->offset + length > buffer->nfragments * buffer->fragment_size) {
		uint64 needed_fragments = (buffer->offset + length + buffer->fragment_size - 1) / buffer->fragment_size;
		if(needed_fragments > buffer->fragments_capacity) {
			uint64 new_capacity = buffer->fragments_capacity;
			while(new_capacity < needed_fragments) {
				new_capacity *= 2;
			}
			fragments = (uint8 **)SAlloc::R(buffer->fragments, (size_t)(new_capacity * sizeof(*fragments)));
			if(fragments == NULL)
				return zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			buffer->fragments = fragments;
			buffer->fragments_capacity = new_capacity;
		}
		while(buffer->nfragments < needed_fragments) {
			if((buffer->fragments[buffer->nfragments] = (uint8 *)SAlloc::M((size_t)buffer->fragment_size)) == NULL)
				return zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			buffer->nfragments++;
		}
	}
	i = buffer->offset / buffer->fragment_size;
	fragment_offset = buffer->offset % buffer->fragment_size;
	n = 0;
	while(n < length) {
		uint64 left = MIN(length - n, buffer->fragment_size - fragment_offset);
		memcpy(buffer->fragments[i] + fragment_offset, data + n, (size_t)left);
		n += left;
		i++;
		fragment_offset = 0;
	}
	buffer->offset += n;
	if(buffer->offset > buffer->size) {
		buffer->size = buffer->offset;
	}
	return (int64)n;
}

static int64 read_data(void * state, void * data, uint64 len, zip_source_cmd_t cmd)
{
	struct read_data * ctx = (struct read_data*)state;
	switch(cmd) {
		case ZIP_SOURCE_BEGIN_WRITE:
		    if((ctx->out = buffer_new_write(WRITE_FRAGMENT_SIZE)) == NULL)
			    return zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
			else
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
		case ZIP_SOURCE_OPEN:
		    ctx->in->offset = 0;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(len > ZIP_INT64_MAX)
			    return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			else
				return buffer_read(ctx->in, static_cast<uint8 *>(data), len);
		case ZIP_SOURCE_REMOVE:
	    {
		    ZipSourceBuffer * empty = buffer_new_read(NULL, 0, 0);
		    if(empty == 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
			else {
				buffer_free(ctx->in);
				ctx->in = empty;
				return 0;
			}
	    }
		case ZIP_SOURCE_ROLLBACK_WRITE:
		    buffer_free(ctx->out);
		    ctx->out = NULL;
		    return 0;
		case ZIP_SOURCE_SEEK:       return buffer_seek(ctx->in, data, len, &ctx->error);
		case ZIP_SOURCE_SEEK_WRITE: return buffer_seek(ctx->out, data, len, &ctx->error);
		case ZIP_SOURCE_STAT:
		    if(len < sizeof(zip_stat_t))
			    return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			else {
				zip_stat_t * st = (zip_stat_t *)data;
				zip_stat_init(st);
				st->mtime = ctx->mtime;
				st->size = ctx->in->size;
				st->comp_size = st->size;
				st->comp_method = ZIP_CM_STORE;
				st->encryption_method = ZIP_EM_NONE;
				st->valid = ZIP_STAT_MTIME|ZIP_STAT_SIZE|ZIP_STAT_COMP_SIZE|ZIP_STAT_COMP_METHOD|ZIP_STAT_ENCRYPTION_METHOD;
				return sizeof(*st);
			}
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, 
				ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, ZIP_SOURCE_SEEK, ZIP_SOURCE_TELL, ZIP_SOURCE_BEGIN_WRITE, ZIP_SOURCE_COMMIT_WRITE,
				ZIP_SOURCE_REMOVE, ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_WRITE, -1);
		case ZIP_SOURCE_TELL:
		    if(ctx->in->offset > ZIP_INT64_MAX)
			    return zip_error_set(&ctx->error, SLERR_ZIP_TELL, EOVERFLOW);
			else
				return (int64)ctx->in->offset;
		case ZIP_SOURCE_TELL_WRITE:
		    if(ctx->out->offset > ZIP_INT64_MAX)
			    return zip_error_set(&ctx->error, SLERR_ZIP_TELL, EOVERFLOW);
			else
				return (int64)ctx->out->offset;
		case ZIP_SOURCE_WRITE:
		    if(len > ZIP_INT64_MAX)
			    return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			else
				return buffer_write(ctx->out, (const uint8 *)data, len, &ctx->error);
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}

ZIP_EXTERN zip_source_t * zip_source_buffer(zip_t * za, const void * data, uint64 len, int freep)
{
	return za ? zip_source_buffer_create(data, len, freep, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_buffer_create(const void * data, uint64 len, int freep, zip_error_t * error)
{
	zip_source_t * zs = 0;
	struct read_data * ctx;
	if(data == NULL && len > 0) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	else if((ctx = (struct read_data*)SAlloc::M(sizeof(*ctx))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
	}
	else if((ctx->in = buffer_new_read(data, len, freep)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		SAlloc::F(ctx);
	}
	else {
		ctx->out = NULL;
		ctx->mtime = time(NULL);
		zip_error_init(&ctx->error);
		if((zs = zip_source_function_create(read_data, ctx, error)) == NULL) {
			buffer_free(ctx->in);
			SAlloc::F(ctx);
		}
	}
	return zs;
}

zip_error_t * zip_source_error(zip_source_t * src)
{
	return &src->error;
}

ZIP_EXTERN int zip_source_is_deleted(zip_source_t * src)
{
	return src->write_state == ZIP_SOURCE_WRITE_REMOVED;
}

zip_source_t * zip_source_layered(zip_t * za, zip_source_t * src, zip_source_layered_callback cb, void * ud)
{
	return za ? zip_source_layered_create(src, cb, ud, &za->error) : 0;
}

zip_source_t * zip_source_layered_create(zip_source_t * src, zip_source_layered_callback cb, void * ud, zip_error_t * error)
{
	zip_source_t * zs;
	if((zs = _zip_source_new(error)) == NULL)
		return NULL;
	zip_source_keep(src);
	zs->src = src;
	zs->cb.l = cb;
	zs->ud = ud;
	zs->supports = cb(src, ud, NULL, 0, ZIP_SOURCE_SUPPORTS);
	if(zs->supports < 0) {
		zs->supports = ZIP_SOURCE_SUPPORTS_READABLE;
	}
	return zs;
}

ZIP_EXTERN int zip_source_open(zip_source_t * src)
{
	if(src->source_closed) {
		return -1;
	}
	if(src->write_state == ZIP_SOURCE_WRITE_REMOVED)
		return zip_error_set(&src->error, SLERR_ZIP_DELETED, 0);
	if(ZIP_SOURCE_IS_OPEN_READING(src)) {
		if((zip_source_supports(src) & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_SEEK)) == 0)
			return zip_error_set(&src->error, SLERR_ZIP_INUSE, 0);
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
	src->open_count++;
	return 0;
}

int zip_source_close(zip_source_t * src)
{
	if(!ZIP_SOURCE_IS_OPEN_READING(src))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
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

ZIP_EXTERN void FASTCALL zip_source_free(zip_source_t * src)
{
	if(src) {
		if(src->refcount > 0) {
			src->refcount--;
		}
		if(src->refcount > 0) {
			return;
		}
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
		zip_source_free(src->src);
		SAlloc::F(src);
	}
}

ZIP_EXTERN int64 zip_source_tell(zip_source_t * src)
{
	if(src->source_closed)
		return -1;
	else if(!ZIP_SOURCE_IS_OPEN_READING(src))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else
		return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL);
}

ZIP_EXTERN int64 zip_source_tell_write(zip_source_t * src)
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else
		return _zip_source_call(src, NULL, 0, ZIP_SOURCE_TELL_WRITE);
}

ZIP_EXTERN int zip_source_seek(zip_source_t * src, int64 offset, int whence)
{
	zip_source_args_seek_t args;
	if(src->source_closed)
		return -1;
	else if(!ZIP_SOURCE_IS_OPEN_READING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else {
		args.offset = offset;
		args.whence = whence;
		return (_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK) < 0 ? -1 : 0);
	}
}

int64 zip_source_seek_compute_offset(uint64 offset, uint64 length, void * data, uint64 data_length, zip_error_t * error)
{
	int64 new_offset;
	zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, data_length, error);
	if(args == NULL) {
		return -1;
	}
	switch(args->whence) {
		case SEEK_CUR:
		    new_offset = (int64)offset + args->offset;
		    break;
		case SEEK_END:
		    new_offset = (int64)length + args->offset;
		    break;
		case SEEK_SET:
		    new_offset = args->offset;
		    break;
		default:
		    return zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	if(new_offset < 0 || (uint64)new_offset > length)
		return zip_error_set(error, SLERR_ZIP_INVAL, 0);
	else
		return new_offset;
}

ZIP_EXTERN int zip_source_seek_write(zip_source_t * src, int64 offset, int whence)
{
	zip_source_args_seek_t args;
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src) || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else {
		args.offset = offset;
		args.whence = whence;
		return (_zip_source_call(src, &args, sizeof(args), ZIP_SOURCE_SEEK_WRITE) < 0 ? -1 : 0);
	}
}

int64 zip_source_read(zip_source_t * src, void * data, uint64 len)
{
	if(src->source_closed)
		return -1;
	else if(!ZIP_SOURCE_IS_OPEN_READING(src) || len > ZIP_INT64_MAX || (len > 0 && data == NULL))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else
		return _zip_source_call(src, data, len, ZIP_SOURCE_READ);
}

ZIP_EXTERN int64 zip_source_write(zip_source_t * src, const void * data, uint64 length)
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src) || length > ZIP_INT64_MAX)
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else
		return _zip_source_call(src, (void *)data, length, ZIP_SOURCE_WRITE);
}

ZIP_EXTERN int zip_source_commit_write(zip_source_t * src)
{
	if(!ZIP_SOURCE_IS_OPEN_WRITING(src))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	if(src->open_count > 1)
		return zip_error_set(&src->error, SLERR_ZIP_INUSE, 0);
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

int64 _zip_source_call(zip_source_t * src, void * data, uint64 length, zip_source_cmd_t command)
{
	int64 ret;
	if((src->supports & (int64)ZIP_SOURCE_MAKE_COMMAND_BITMASK(command)) == 0)
		return zip_error_set(&src->error, SLERR_ZIP_OPNOTSUPP, 0);
	if(src->src == NULL) {
		ret = src->cb.f(src->ud, data, length, command);
	}
	else {
		ret = src->cb.l(src->src, src->ud, data, length, command);
	}
	if(ret < 0) {
		if(command != ZIP_SOURCE_ERROR && command != ZIP_SOURCE_SUPPORTS) {
			int e[2];
			if(_zip_source_call(src, e, sizeof(e), ZIP_SOURCE_ERROR) < 0) {
				zip_error_set(&src->error, SLERR_ZIP_INTERNAL, 0);
			}
			else {
				zip_error_set(&src->error, e[0], e[1]);
			}
		}
	}
	return ret;
}

zip_source_t * _zip_source_zip_new(zip_t * za, zip_t * srcza, uint64 srcidx, zip_flags_t flags, uint64 start, uint64 len, const char * password)
{
	zip_compression_implementation comp_impl;
	zip_encryption_implementation enc_impl;
	zip_source_t * src, * s2;
	uint64 offset;
	zip_stat_t st;
	if(za == NULL)
		return NULL;
	if(srcza == NULL || srcidx >= srcza->nentry) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(!(flags & ZIP_FL_UNCHANGED) && (ZIP_ENTRY_DATA_CHANGED(srcza->entry+srcidx) || srcza->entry[srcidx].deleted)) {
		zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
		return NULL;
	}
	if(zip_stat_index(srcza, srcidx, flags|ZIP_FL_UNCHANGED, &st) < 0) {
		zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
		return NULL;
	}
	if(flags & ZIP_FL_ENCRYPTED)
		flags |= ZIP_FL_COMPRESSED;
	if((start > 0 || len > 0) && (flags & ZIP_FL_COMPRESSED)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	/* overflow or past end of file */
	if((start > 0 || len > 0) && (start+len < start || start+len > st.size)) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	enc_impl = NULL;
	if(((flags & ZIP_FL_ENCRYPTED) == 0) && (st.encryption_method != ZIP_EM_NONE)) {
		if(password == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_NOPASSWD, 0);
			return NULL;
		}
		if((enc_impl = _zip_get_encryption_implementation(st.encryption_method)) == NULL) {
			zip_error_set(&za->error, SLERR_ZIP_ENCRNOTSUPP, 0);
			return NULL;
		}
	}
	comp_impl = NULL;
	if((flags & ZIP_FL_COMPRESSED) == 0) {
		if(st.comp_method != ZIP_CM_STORE) {
			if((comp_impl = _zip_get_compression_implementation(st.comp_method)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
				return NULL;
			}
		}
	}
	if((offset = _zip_file_get_offset(srcza, srcidx, &za->error)) == 0)
		return NULL;
	if(st.comp_size == 0) {
		return zip_source_buffer(za, NULL, 0, 0);
	}
	if(start+len > 0 && enc_impl == NULL && comp_impl == NULL) {
		zip_stat_t st2;
		st2.size = len ? len : st.size-start;
		st2.comp_size = st2.size;
		st2.comp_method = ZIP_CM_STORE;
		st2.mtime = st.mtime;
		st2.valid = ZIP_STAT_SIZE|ZIP_STAT_COMP_SIZE|ZIP_STAT_COMP_METHOD|ZIP_STAT_MTIME;
		if((src = _zip_source_window_new(srcza->src, offset+start, st2.size, &st2, &za->error)) == NULL) {
			return NULL;
		}
	}
	else {
		if((src = _zip_source_window_new(srcza->src, offset, st.comp_size, &st, &za->error)) == NULL) {
			return NULL;
		}
	}
	if(_zip_source_set_source_archive(src, srcza) < 0) {
		zip_source_free(src);
		return NULL;
	}
	/* creating a layered source calls zip_keep() on the lower layer, so we free it */
	if(enc_impl) {
		s2 = enc_impl(za, src, st.encryption_method, 0, password);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(comp_impl) {
		s2 = comp_impl(za, src, st.comp_method, 0);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(((flags & ZIP_FL_COMPRESSED) == 0 || st.comp_method == ZIP_CM_STORE) && (len == 0 || len == st.comp_size)) {
		/* when reading the whole file, check for CRC errors */
		s2 = zip_source_crc(za, src, 1);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	if(start+len > 0 && (comp_impl || enc_impl)) {
		s2 = zip_source_window(za, src, start, len ? len : st.size-start);
		zip_source_free(src);
		if(s2 == NULL) {
			return NULL;
		}
		src = s2;
	}
	return src;
}

ZIP_EXTERN zip_source_t * zip_source_zip(zip_t * za, zip_t * srcza, uint64 srcidx, zip_flags_t flags, uint64 start, int64 len)
{
	if(len < -1) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if(len == -1)
		len = 0;
	SETFLAG(flags, ZIP_FL_COMPRESSED, (start == 0 && len == 0));
	return _zip_source_zip_new(za, srcza, srcidx, flags, start, (uint64)len, 0);
}

ZIP_EXTERN int zip_source_begin_write(zip_source_t * src)
{
	if(ZIP_SOURCE_IS_OPEN_WRITING(src))
		return zip_error_set(&src->error, SLERR_ZIP_INVAL, 0);
	else if(_zip_source_call(src, NULL, 0, ZIP_SOURCE_BEGIN_WRITE) < 0)
		return -1;
	else {
		src->write_state = ZIP_SOURCE_WRITE_OPEN;
		return 0;
	}
}
//
// ZIP_SET
//
ZIP_EXTERN int zip_set_archive_comment(zip_t * za, const char * comment, uint16 len)
{
	zip_string_t * cstr;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(len > 0 && comment == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(len > 0) {
		if((cstr = _zip_string_new((const uint8 *)comment, len, ZIP_FL_ENC_GUESS, &za->error)) == NULL)
			return -1;
		if(_zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_CP437) {
			_zip_string_free(cstr);
			return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
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
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	}
	if((flag & ZIP_AFL_RDONLY) && value && (za->ch_flags & ZIP_AFL_RDONLY) == 0) {
		if(_zip_changed(za, NULL)) {
			return zip_error_set(&za->error, SLERR_ZIP_CHANGED, 0);
		}
	}
	za->ch_flags = new_flags;
	return 0;
}

ZIP_EXTERN int zip_set_default_password(zip_t * za, const char * passwd)
{
	if(za == NULL)
		return -1;
	else {
		SAlloc::F(za->default_password);
		if(passwd) {
			za->default_password = sstrdup(passwd);
			if(!za->default_password) {
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
			}
		}
		else
			za->default_password = NULL;
		return 0;
	}
}

ZIP_EXTERN int zip_set_file_comment(zip_t * za, uint64 idx, const char * comment, int len)
{
	if(len < 0 || len > ZIP_UINT16_MAX)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else
		return zip_file_set_comment(za, idx, comment, (uint16)len, 0);
}

ZIP_EXTERN int zip_set_file_compression(zip_t * za, uint64 idx, int32 method, uint32 flags)
{
	zip_entry_t * e;
	int32 old_method;
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(method != ZIP_CM_DEFAULT && method != ZIP_CM_STORE && method != ZIP_CM_DEFLATE)
		return zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
	e = za->entry+idx;
	old_method = (e->orig == NULL ? ZIP_CM_DEFAULT : e->orig->comp_method);
	/* @todo revisit this when flags are supported, since they may require a recompression */
	if(method == old_method) {
		if(e->changes) {
			e->changes->changed &= ~ZIP_DIRENT_COMP_METHOD;
			if(e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	}
	else {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		e->changes->comp_method = method;
		e->changes->changed |= ZIP_DIRENT_COMP_METHOD;
	}
	return 0;
}

int _zip_set_name(zip_t * za, uint64 idx, const char * name, zip_flags_t flags)
{
	zip_entry_t * e;
	zip_string_t * str;
	bool same_as_orig;
	int64 i;
	const uint8 * old_name, * new_name;
	zip_string_t * old_str;
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(name && name[0] != '\0') {
		/* @todo check for string too long */
		if((str = _zip_string_new((const uint8 *)name, (uint16)strlen(name), flags, &za->error)) == NULL)
			return -1;
		if((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS &&
		    _zip_guess_encoding(str, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			str->encoding = ZIP_ENCODING_UTF8_KNOWN;
	}
	else
		str = NULL;
	/* @todo encoding flags needed for CP437? */
	if((i = _zip_name_locate(za, name, 0, NULL)) >= 0 && (uint64)i != idx) {
		_zip_string_free(str);
		return zip_error_set(&za->error, SLERR_ZIP_EXISTS, 0);
	}
	/* no effective name change */
	if(i>=0 && (uint64)i == idx) {
		_zip_string_free(str);
		return 0;
	}
	e = za->entry+idx;
	same_as_orig = LOGIC(e->orig && _zip_string_equal(e->orig->filename, str));
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
		_zip_hash_delete(za->names, old_name, 0);
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
					/* @todo what if not cloned? can that happen? */
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
//
// ZIP_GET
//
ZIP_EXTERN const char * zip_get_archive_comment(zip_t * za, int * lenp, zip_flags_t flags)
{
	uint32 len;
	const uint8 * str;
	zip_string_t * comment = ((flags & ZIP_FL_UNCHANGED) || (za->comment_changes == NULL)) ? za->comment_orig : za->comment_changes;
	if((str = _zip_string_get(comment, &len, flags, &za->error)) == NULL)
		return NULL;
	ASSIGN_PTR(lenp, (int)len);
	return (const char *)str;
}

ZIP_EXTERN int zip_get_archive_flag(zip_t * za, zip_flags_t flag, zip_flags_t flags)
{
	uint fl = (flags & ZIP_FL_UNCHANGED) ? za->flags : za->ch_flags;
	return (fl & flag) ? 1 : 0;
}

zip_compression_implementation _zip_get_compression_implementation(int32 cm)
{
	return (cm == ZIP_CM_DEFLATE || ZIP_CM_IS_DEFAULT(cm)) ? zip_source_deflate : 0;
}

zip_encryption_implementation _zip_get_encryption_implementation(uint16 em)
{
	return (em == ZIP_EM_TRAD_PKWARE) ? zip_source_pkware : 0;
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
//
// IO UTIL
//
int _zip_read(zip_source_t * src, uint8 * b, uint64 length, zip_error_t * error)
{
	int64 n;
	if(length > ZIP_INT64_MAX)
		return zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
	else if((n = zip_source_read(src, b, length)) < 0) {
		_zip_error_set_from_source(error, src);
		return -1;
	}
	else if(n < (int64)length)
		return zip_error_set(error, SLERR_ZIP_EOF, 0);
	else
		return 0;
}

uint8 * _zip_read_data(zip_buffer_t * buffer, zip_source_t * src, size_t length, bool nulp, zip_error_t * error)
{
	uint8 * r;
	if(length == 0 && !nulp) {
		return NULL;
	}
	r = static_cast<uint8 *>(SAlloc::M(length + (nulp ? 1 : 0)));
	if(!r) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if(buffer) {
		uint8 * data = _zip_buffer_get(buffer, length);
		if(data == NULL) {
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
		uint8 * o;
		/* replace any in-string NUL characters with spaces */
		r[length] = 0;
		for(o = r; o<r+length; o++)
			if(*o == '\0')
				*o = ' ';
	}
	return r;
}

zip_string_t * _zip_read_string(zip_buffer_t * buffer, zip_source_t * src, uint16 len, bool nulp, zip_error_t * error)
{
	uint8 * raw;
	zip_string_t * s;
	if((raw = _zip_read_data(buffer, src, len, nulp, error)) == NULL)
		return NULL;
	s = _zip_string_new(raw, len, ZIP_FL_ENC_GUESS, error);
	SAlloc::F(raw);
	return s;
}

int FASTCALL _zip_write(zip_t * za, const void * data, uint64 length)
{
	int64 n;
	if((n = zip_source_write(za->src, data, length)) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if((uint64)n != length)
		return zip_error_set(&za->error, SLERR_ZIP_WRITE, EINTR);
	return 0;
}
//
//
//
ZIP_EXTERN int64 zip_name_locate(zip_t * za, const char * fname, zip_flags_t flags)
{
	return _zip_name_locate(za, fname, flags, &za->error);
}

int64 _zip_name_locate(zip_t * za, const char * fname, zip_flags_t flags, zip_error_t * error)
{
	int (* cmp)(const char *, const char *);
	const char * p;
	if(za == NULL)
		return -1;
	if(fname == NULL)
		return zip_error_set(error, SLERR_ZIP_INVAL, 0);
	if(flags & (ZIP_FL_NOCASE|ZIP_FL_NODIR|ZIP_FL_ENC_CP437)) {
		// can't use hash table 
		cmp = (flags & ZIP_FL_NOCASE) ? /*strcasecmp*/_stricmp : strcmp;
		for(uint64 i = 0; i < za->nentry; i++) {
			const char * fn = _zip_get_name(za, i, flags, error);
			// newly added (partially filled) entry or error 
			if(fn) {
				if(flags & ZIP_FL_NODIR) {
					p = strrchr(fn, '/');
					if(p)
						fn = p+1;
				}
				if(cmp(fname, fn) == 0) {
					_zip_error_clear(error);
					return (int64)i;
				}
			}
		}
		return zip_error_set(error, SLERR_ZIP_NOENT, 0);
	}
	else {
		return _zip_hash_lookup(za->names, (const uint8 *)fname, flags, error);
	}
}
//
// STRING
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
	/* @todo encoding */
	return (memcmp(a->raw, b->raw, a->length) == 0);
}

void FASTCALL _zip_string_free(zip_string_t * s)
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
	if((flags & ZIP_FL_ENC_RAW) == 0) {
		/* start guessing */
		if(string->encoding == ZIP_ENCODING_UNKNOWN)
			_zip_guess_encoding(string, ZIP_ENCODING_UNKNOWN);
		if(((flags & ZIP_FL_ENC_STRICT) && string->encoding != ZIP_ENCODING_ASCII && string->encoding != ZIP_ENCODING_UTF8_KNOWN)
		   || (string->encoding == ZIP_ENCODING_CP437)) {
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

uint16 _zip_string_length(const zip_string_t * s)
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
	if((s->raw = (uint8 *)SAlloc::M((size_t)(length+1))) == NULL) {
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

int _zip_string_write(zip_t * za, const zip_string_t * s) { return s ? _zip_write(za, s->raw, s->length) : 0; }

// ZIP_EXTERN const char * zip_strerror(zip_t *za) { return zip_error_strerror(&za->error); }
//
// UNCHANGE
//
ZIP_EXTERN int zip_unchange(zip_t * za, uint64 idx)
{
	return _zip_unchange(za, idx, 0);
}

int _zip_unchange(zip_t * za, uint64 idx, int allow_duplicates)
{
	int64 i;
	const char * orig_name, * changed_name;
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(!allow_duplicates && za->entry[idx].changes && (za->entry[idx].changes->changed & ZIP_DIRENT_FILENAME)) {
		if(za->entry[idx].orig != NULL) {
			if((orig_name = _zip_get_name(za, idx, ZIP_FL_UNCHANGED, &za->error)) == NULL) {
				return -1;
			}
			i = _zip_name_locate(za, orig_name, 0, 0);
			if(i >= 0 && (uint64)i != idx)
				return zip_error_set(&za->error, SLERR_ZIP_EXISTS, 0);
		}
		else {
			orig_name = NULL;
		}
		if((changed_name = _zip_get_name(za, idx, 0, &za->error)) == NULL) {
			return -1;
		}
		if(orig_name) {
			if(_zip_hash_add(za->names, (const uint8 *)orig_name, idx, 0, &za->error) == false) {
				return -1;
			}
		}
		if(_zip_hash_delete(za->names, (const uint8 *)changed_name, &za->error) == false) {
			_zip_hash_delete(za->names, (const uint8 *)orig_name, 0);
			return -1;
		}
	}
	_zip_dirent_free(za->entry[idx].changes);
	za->entry[idx].changes = NULL;
	_zip_unchange_data(za->entry+idx);
	return 0;
}

ZIP_EXTERN int zip_unchange_all(zip_t * za)
{
	int ret = 0;
	_zip_hash_revert(za->names);
	for(uint64 i = 0; i < za->nentry; i++)
		ret |= _zip_unchange(za, i, 1);
	ret |= zip_unchange_archive(za);
	return ret;
}

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
static void FASTCALL _zip_ef_free(zip_extra_field_t * ef)
{
	while(ef) {
		zip_extra_field_t * ef2 = ef->next;
		SAlloc::F(ef->data);
		SAlloc::F(ef);
		ef = ef2;
	}
}
//
// DIRENT
//
static time_t _zip_d2u_time(uint16, uint16);
static zip_string_t * _zip_dirent_process_ef_utf_8(const zip_dirent_t * de, uint16 id, zip_string_t * str);
static zip_extra_field_t * _zip_ef_utf8(uint16, zip_string_t *, zip_error_t *);

void FASTCALL _zip_cdir_free(zip_cdir_t * cd)
{
	if(cd) {
		for(uint64 i = 0; i < cd->nentry; i++)
			_zip_entry_finalize(cd->entry+i);
		SAlloc::F(cd->entry);
		_zip_string_free(cd->comment);
		SAlloc::F(cd);
	}
}

zip_cdir_t * _zip_cdir_new(uint64 nentry, zip_error_t * error)
{
	zip_cdir_t * cd;
	uint64 i;
	if((cd = (zip_cdir_t*)SAlloc::M(sizeof(*cd))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	if(nentry == 0)
		cd->entry = NULL;
	else if((nentry > SIZE_MAX/sizeof(*(cd->entry))) || (cd->entry = (zip_entry_t*)SAlloc::M(sizeof(*(cd->entry))*(size_t)nentry)) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		SAlloc::F(cd);
		return NULL;
	}
	for(i = 0; i<nentry; i++)
		_zip_entry_init(cd->entry+i);
	cd->nentry = cd->nentry_alloc = nentry;
	cd->size = cd->offset = 0;
	cd->comment = NULL;
	return cd;
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
	for(i = 0; i<survivors; i++) {
		zip_entry_t * entry = za->entry+filelist[i].idx;
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
	if((buffer = _zip_buffer_new(buf, sizeof(buf))) == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
	if(is_zip64) {
		_zip_buffer_put(buffer, EOCD64_MAGIC, 4);
		_zip_buffer_put_64(buffer, EOCD64LEN-12);
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
		_zip_buffer_put_64(buffer, offset+size);
		_zip_buffer_put_32(buffer, 1);
	}
	_zip_buffer_put(buffer, EOCD_MAGIC, 4);
	_zip_buffer_put_32(buffer, 0);
	_zip_buffer_put_16(buffer, static_cast<uint16>(survivors >= ZIP_UINT16_MAX ? ZIP_UINT16_MAX : survivors));
	_zip_buffer_put_16(buffer, static_cast<uint16>(survivors >= ZIP_UINT16_MAX ? ZIP_UINT16_MAX : survivors));
	_zip_buffer_put_32(buffer, size >= ZIP_UINT32_MAX ? ZIP_UINT32_MAX : (uint32)size);
	_zip_buffer_put_32(buffer, offset >= ZIP_UINT32_MAX ? ZIP_UINT32_MAX : (uint32)offset);
	comment = za->comment_changed ? za->comment_changes : za->comment_orig;
	_zip_buffer_put_16(buffer, static_cast<uint16>(comment ? comment->length : 0));
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
	zip_dirent_t * tde = (zip_dirent_t*)SAlloc::M(sizeof(*tde));
	if(tde) {
		if(sde)
			memcpy(tde, sde, sizeof(*sde));
		else
			_zip_dirent_init(tde);
		tde->changed = 0;
		tde->cloned = 1;
	}
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
	de->version_madeby = 20 | (ZIP_OPSYS_DEFAULT << 8);
	de->version_needed = 20; /* 2.0 */
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
}

bool _zip_dirent_needs_zip64(const zip_dirent_t * de, zip_flags_t flags)
{
	return LOGIC(de->uncomp_size >= ZIP_UINT32_MAX || de->comp_size >= ZIP_UINT32_MAX || ((flags & ZIP_FL_CENTRAL) && de->offset >= ZIP_UINT32_MAX));
}

zip_dirent_t * _zip_dirent_new(void)
{
	zip_dirent_t * de = static_cast<zip_dirent_t *>(SAlloc::M(sizeof(*de)));
	if(de)
		_zip_dirent_init(de);
	return de;
}

/* _zip_dirent_read(zde, fp, bufp, left, localp, error):
   Fills the zip directory entry zde.

   If buffer is non-NULL, data is taken from there; otherwise data is read from fp as needed.

   If local is true, it reads a local header instead of a central directory entry.

   Returns size of dirent read if successful. On error, error is filled in and -1 is returned.
 */

int64 _zip_dirent_read(zip_dirent_t * zde, zip_source_t * src, zip_buffer_t * buffer, bool local, zip_error_t * error)
{
	uint8  buf[CDENTRYSIZE];
	uint16 dostime, dosdate;
	uint32 variable_size;
	uint16 filename_len, comment_len, ef_len;
	bool   from_buffer = (buffer != NULL);
	uint32 size = local ? LENTRYSIZE : CDENTRYSIZE;
	if(buffer) {
		if(_zip_buffer_left(buffer) < size)
			return zip_error_set(error, SLERR_ZIP_NOZIP, 0);
	}
	else {
		if((buffer = _zip_buffer_new_from_source(src, size, buf, error)) == NULL) {
			return -1;
		}
	}
	if(memcmp(_zip_buffer_get(buffer, 4), (local ? LOCAL_MAGIC : CENTRAL_MAGIC), 4) != 0) {
		zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		if(!from_buffer)
			_zip_buffer_free(buffer);
		return -1;
	}
	// convert buffercontents to zip_dirent 
	_zip_dirent_init(zde);
	zde->version_madeby = local ? 0 : _zip_buffer_get_16(buffer);
	zde->version_needed = _zip_buffer_get_16(buffer);
	zde->bitflags = _zip_buffer_get_16(buffer);
	zde->comp_method = _zip_buffer_get_16(buffer);
	// convert to time_t 
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
	zde->filename = NULL;
	zde->extra_fields = NULL;
	zde->comment = NULL;
	variable_size = (uint32)filename_len+(uint32)ef_len+(uint32)comment_len;
	if(from_buffer) {
		if(_zip_buffer_left(buffer) < variable_size)
			return zip_error_set(error, SLERR_ZIP_INCONS, 0);
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
			if(zip_error_code_zip(error) == SLERR_ZIP_EOF)
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		if(zde->bitflags & ZIP_GPBF_ENCODING_UTF_8) {
			if(_zip_guess_encoding(zde->filename, ZIP_ENCODING_UTF8_KNOWN) == ZIP_ENCODING_ERROR) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				if(!from_buffer)
					_zip_buffer_free(buffer);
				return -1;
			}
		}
	}
	if(ef_len) {
		uint8 * ef = _zip_read_data(buffer, src, ef_len, 0, error);
		if(ef == NULL) {
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		if(!_zip_ef_parse(ef, ef_len, local ? ZIP_EF_LOCAL : ZIP_EF_CENTRAL, &zde->extra_fields, error)) {
			SAlloc::F(ef);
			if(!from_buffer)
				_zip_buffer_free(buffer);
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
		/* @todo if got_len == 0 && !ZIP64_EOCD: no error, 0xffffffff is valid value */
		if(ef == NULL) {
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		if((ef_buffer = _zip_buffer_new((uint8 *)ef, got_len)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		if(zde->uncomp_size == ZIP_UINT32_MAX)
			zde->uncomp_size = _zip_buffer_get_64(ef_buffer);
		else if(local) {
			// From appnote.txt: This entry in the Local header MUST
			// include BOTH original and compressed file size fields. 
			(void)_zip_buffer_skip(ef_buffer, 8); // error is caught by _zip_buffer_eof() call 
		}
		if(zde->comp_size == ZIP_UINT32_MAX)
			zde->comp_size = _zip_buffer_get_64(ef_buffer);
		if(!local) {
			if(zde->offset == ZIP_UINT32_MAX)
				zde->offset = _zip_buffer_get_64(ef_buffer);
			if(zde->disk_number == ZIP_UINT16_MAX)
				zde->disk_number = _zip_buffer_get_32(buffer);
		}
		if(!_zip_buffer_eof(ef_buffer)) {
			zip_error_set(error, SLERR_ZIP_INCONS, 0);
			_zip_buffer_free(ef_buffer);
			if(!from_buffer)
				_zip_buffer_free(buffer);
			return -1;
		}
		_zip_buffer_free(ef_buffer);
	}

	if(!_zip_buffer_ok(buffer)) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		if(!from_buffer) {
			_zip_buffer_free(buffer);
		}
		return -1;
	}
	if(!from_buffer) {
		_zip_buffer_free(buffer);
	}
	// zip_source_seek / zip_source_tell don't support values > ZIP_INT64_MAX 
	if(zde->offset > ZIP_INT64_MAX)
		return zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
	zde->extra_fields = _zip_ef_remove_internal(zde->extra_fields);
	return (int64)(size + variable_size);
}

static zip_string_t * _zip_dirent_process_ef_utf_8(const zip_dirent_t * de, uint16 id, zip_string_t * str)
{
	uint16 ef_len;
	uint32 ef_crc;
	zip_buffer_t * buffer;
	const uint8 * ef = _zip_ef_get_by_id(de->extra_fields, &ef_len, id, 0, ZIP_EF_BOTH, 0);
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
		zip_string_t * ef_str = _zip_string_new(_zip_buffer_get(buffer, len), len, ZIP_FL_ENC_UTF_8, 0);
		if(ef_str != NULL) {
			_zip_string_free(str);
			str = ef_str;
		}
	}
	_zip_buffer_free(buffer);
	return str;
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
	for(i = 0; i<(local ? 2 : 3); i++) {
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
// Descr: Writes zip directory entry.
//   If flags & ZIP_EF_LOCAL, it writes a local header instead of a central
//   directory entry.  If flags & ZIP_EF_FORCE_ZIP64, a ZIP64 extra field is written, even if not needed.
// Returns:
//   0 if successful, 1 if successful and wrote ZIP64 extra field. On error, error is filled in and -1 is returned.
// 
int _zip_dirent_write(zip_t * za, zip_dirent_t * de, zip_flags_t flags)
{
	uint16 dostime, dosdate;
	zip_extra_field_t * ef = 0;
	zip_extra_field_t * ef64;
	uint32 ef_total_size;
	bool is_zip64;
	bool is_really_zip64;
	uint8 buf[CDENTRYSIZE];
	zip_buffer_t * buffer;
	zip_encoding_type_t name_enc = _zip_guess_encoding(de->filename, ZIP_ENCODING_UNKNOWN);
	zip_encoding_type_t com_enc = _zip_guess_encoding(de->comment, ZIP_ENCODING_UNKNOWN);
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
	is_really_zip64 = _zip_dirent_needs_zip64(de, flags);
	is_zip64 = (flags & (ZIP_FL_LOCAL|ZIP_FL_FORCE_ZIP64)) == (ZIP_FL_LOCAL|ZIP_FL_FORCE_ZIP64) || is_really_zip64;
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
		ef64 = _zip_ef_new(ZIP_EF_ZIP64, static_cast<uint16>(_zip_buffer_offset(ef_buffer)), ef_zip64, ZIP_EF_BOTH);
		_zip_buffer_free(ef_buffer);
		ef64->next = ef;
		ef = ef64;
	}
	if((buffer = _zip_buffer_new(buf, sizeof(buf))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		_zip_ef_free(ef);
		return -1;
	}
	_zip_buffer_put(buffer, (flags & ZIP_FL_LOCAL) ? LOCAL_MAGIC : CENTRAL_MAGIC, 4);
	if((flags & ZIP_FL_LOCAL) == 0) {
		_zip_buffer_put_16(buffer, static_cast<uint16>(is_really_zip64 ? 45 : de->version_madeby));
	}
	_zip_buffer_put_16(buffer, static_cast<uint16>(is_really_zip64 ? 45 : de->version_needed));
	_zip_buffer_put_16(buffer, de->bitflags&0xfff9); /* clear compression method specific flags */
	_zip_buffer_put_16(buffer, (uint16)de->comp_method);
	_zip_u2d_time(de->last_mod, &dostime, &dosdate);
	_zip_buffer_put_16(buffer, dostime);
	_zip_buffer_put_16(buffer, dosdate);
	_zip_buffer_put_32(buffer, de->crc);
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
	/* @todo check for overflow */
	ef_total_size = (uint32)_zip_ef_size(de->extra_fields, flags) + (uint32)_zip_ef_size(ef, ZIP_EF_BOTH);
	_zip_buffer_put_16(buffer, (uint16)ef_total_size);
	if((flags & ZIP_FL_LOCAL) == 0) {
		_zip_buffer_put_16(buffer, _zip_string_length(de->comment));
		_zip_buffer_put_16(buffer, (uint16)de->disk_number);
		_zip_buffer_put_16(buffer, de->int_attrib);
		_zip_buffer_put_32(buffer, de->ext_attrib);
		_zip_buffer_put_32(buffer, (de->offset < ZIP_UINT32_MAX) ? (uint32)de->offset : ZIP_UINT32_MAX);
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

static time_t _zip_d2u_time(uint16 dtime, uint16 ddate)
{
	struct tm tm;
	memzero(&tm, sizeof(tm));
	/* let mktime decide if DST is in effect */
	tm.tm_isdst = -1;
	tm.tm_year = ((ddate>>9)&127) + 1980 - 1900;
	tm.tm_mon = ((ddate>>5)&15) - 1;
	tm.tm_mday = ddate&31;
	tm.tm_hour = (dtime>>11)&31;
	tm.tm_min = (dtime>>5)&63;
	tm.tm_sec = (dtime<<1)&62;
	return mktime(&tm);
}

static zip_extra_field_t * _zip_ef_utf8(uint16 id, zip_string_t * str, zip_error_t * error)
{
	const uint8 * raw;
	uint32 len;
	zip_buffer_t * buffer;
	zip_extra_field_t * ef;
	if((raw = _zip_string_get(str, &len, ZIP_FL_ENC_RAW, NULL)) == NULL) {
		/* error already set */
		return NULL;
	}
	if(len+5 > ZIP_UINT16_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0); /* @todo better error code? */
		return NULL;
	}
	if((buffer = _zip_buffer_new(NULL, len+5)) == NULL) {
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
	ef = _zip_ef_new(id, static_cast<uint16>(_zip_buffer_offset(buffer)), _zip_buffer_data(buffer), ZIP_EF_BOTH);
	_zip_buffer_free(buffer);
	return ef;
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
	struct tm * tm = localtime(&intime);
	SETMAX(tm->tm_year, 80);
	*ddate = static_cast<uint16>(((tm->tm_year+1900-1980)<<9) + ((tm->tm_mon+1)<<5) + tm->tm_mday);
	*dtime = static_cast<uint16>(((tm->tm_hour)<<11) + ((tm->tm_min)<<5) + ((tm->tm_sec)>>1));
}

int _zip_filerange_crc(zip_source_t * src, uint64 start, uint64 len, uLong * crcp, zip_error_t * error)
{
	Bytef buf[BUFSIZE];
	int64 n;
	*crcp = crc32(0L, Z_NULL, 0);
	if(start > ZIP_INT64_MAX)
		return zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
	if(zip_source_seek(src, (int64)start, SEEK_SET) != 0) {
		_zip_error_set_from_source(error, src);
		return -1;
	}
	while(len > 0) {
		n = (int64)(len > BUFSIZE ? BUFSIZE : len);
		if((n = zip_source_read(src, buf, (uint64)n)) < 0) {
			_zip_error_set_from_source(error, src);
			return -1;
		}
		if(n == 0)
			return zip_error_set(error, SLERR_ZIP_EOF, 0);
		*crcp = crc32(*crcp, buf, (uInt)n);
		len -= (uint64)n;
	}
	return 0;
}

struct crc_context {
	int    validate; /* whether to check CRC on EOF and return error on mismatch */
	int    crc_complete; /* whether CRC was computed for complete file */
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
	if((ctx = static_cast<struct crc_context *>(SAlloc::M(sizeof(*ctx)))) == NULL) {
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
	struct crc_context * ctx = static_cast<struct crc_context *>(_ctx);
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
					    zip_stat_t st;
					    if(zip_source_stat(src, &st) < 0) {
						    _zip_error_set_from_source(&ctx->error, src);
						    return -1;
					    }
					    if((st.valid & ZIP_STAT_CRC) && st.crc != ctx->crc)
						    return zip_error_set(&ctx->error, SLERR_ZIP_CRC, 0);
					    if((st.valid & ZIP_STAT_SIZE) && st.size != ctx->size)
						    return zip_error_set(&ctx->error, SLERR_ZIP_INCONS, 0);
				    }
			    }
		    }
		    else if(!ctx->crc_complete && ctx->position <= ctx->crc_position) {
			    uint64 nn;
			    for(uint64 i = ctx->crc_position - ctx->position; i < (uint64)n; i += nn) {
				    nn = MIN(UINT_MAX, (uint64)n-i);
				    ctx->crc = (uint32)crc32(ctx->crc, (const Bytef*)data+i, (uInt)nn);
				    ctx->crc_position += nn;
			    }
		    }
		    ctx->position += (uint64)n;
		    return n;
		case ZIP_SOURCE_CLOSE:
		    return 0;
		case ZIP_SOURCE_STAT:
	    {
		    zip_stat_t * st = (zip_stat_t*)data;
		    if(ctx->crc_complete) {
			    /* @todo Set comp_size, comp_method, encryption_method?
			            After all, this only works for uncompressed data. */
			    st->size = ctx->size;
			    st->crc = ctx->crc;
			    st->comp_size = ctx->size;
			    st->comp_method = ZIP_CM_STORE;
			    st->encryption_method = ZIP_EM_NONE;
			    st->valid |= ZIP_STAT_SIZE|ZIP_STAT_CRC|ZIP_STAT_COMP_SIZE|ZIP_STAT_COMP_METHOD|ZIP_STAT_ENCRYPTION_METHOD;
		    }
		    return 0;
	    }
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_SUPPORTS:
	    {
		    int64 mask = zip_source_supports(src);
		    if(mask < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    return mask & ~zip_source_make_command_bitmap(ZIP_SOURCE_BEGIN_WRITE,
			    ZIP_SOURCE_COMMIT_WRITE, ZIP_SOURCE_ROLLBACK_WRITE,
			    ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE, -1);
	    }
		case ZIP_SOURCE_SEEK:
	    {
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
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}
//
// ZIP CLOSE
//
#define MAX_DEFLATE_SIZE_32     4293656963u // max deflate size increase: size + ceil(size/16k)*5+6 

static int FASTCALL copy_data(zip_t * za, uint64 len)
{
	uint8 buf[BUFSIZE];
	while(len > 0) {
		const size_t n = (len > sizeof(buf)) ? sizeof(buf) : (size_t)len;
		if(_zip_read(za->src, buf, n, &za->error) < 0) {
			return -1;
		}
		if(_zip_write(za, buf, n) < 0) {
			return -1;
		}
		len -= n;
	}
	return 0;
}

static int FASTCALL copy_source(zip_t * za, zip_source_t * src)
{
	uint8 buf[BUFSIZE];
	int64 n;
	int ret;
	if(zip_source_open(src) < 0) {
		_zip_error_set_from_source(&za->error, src);
		return -1;
	}
	ret = 0;
	while((n = zip_source_read(src, buf, sizeof(buf))) > 0) {
		if(_zip_write(za, buf, (uint64)n) < 0) {
			ret = -1;
			break;
		}
	}
	if(n < 0) {
		_zip_error_set_from_source(&za->error, src);
		ret = -1;
	}
	zip_source_close(src);
	return ret;
}

static int add_data(zip_t * za, zip_source_t * src, zip_dirent_t * de)
{
	int64 offstart, offdata, offend;
	zip_stat_t st;
	zip_source_t * s2;
	int ret;
	int is_zip64;
	zip_flags_t flags;
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
		/* we'll recompress */
		st.valid &= ~ZIP_STAT_COMP_SIZE;
	}
	flags = ZIP_EF_LOCAL;
	if((st.valid & ZIP_STAT_SIZE) == 0)
		flags |= ZIP_FL_FORCE_ZIP64;
	else {
		de->uncomp_size = st.size;
		if((st.valid & ZIP_STAT_COMP_SIZE) == 0) {
			if((((de->comp_method == ZIP_CM_DEFLATE || ZIP_CM_IS_DEFAULT(de->comp_method)) && st.size > MAX_DEFLATE_SIZE_32)
				   || (de->comp_method != ZIP_CM_STORE && de->comp_method != ZIP_CM_DEFLATE &&
					    !ZIP_CM_IS_DEFAULT(de->comp_method))))
				flags |= ZIP_FL_FORCE_ZIP64;
		}
		else
			de->comp_size = st.comp_size;
	}
	if((offstart = zip_source_tell_write(za->src)) < 0) {
		return -1;
	}
	// as long as we don't support non-seekable output, clear data descriptor bit 
	de->bitflags &= (uint16) ~ZIP_GPBF_DATA_DESCRIPTOR;
	if((is_zip64 = _zip_dirent_write(za, de, flags)) < 0)
		return -1;
	if(st.comp_method == ZIP_CM_STORE || (ZIP_CM_IS_DEFAULT(de->comp_method) && st.comp_method != de->comp_method)) {
		zip_source_t * s_store, * s_crc;
		zip_compression_implementation comp_impl;
		if(st.comp_method != ZIP_CM_STORE) {
			if((comp_impl = _zip_get_compression_implementation(st.comp_method)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
			if((s_store = comp_impl(za, src, st.comp_method, ZIP_CODEC_DECODE)) == NULL) {
				return -1; // error set by comp_impl 
			}
		}
		else {
			// to have the same reference count to src as in the case where it's not stored 
			zip_source_keep(src);
			s_store = src;
		}
		s_crc = zip_source_crc(za, s_store, 0);
		zip_source_free(s_store);
		if(s_crc == NULL) {
			return -1;
		}
		if(de->comp_method != ZIP_CM_STORE && (!(st.valid & ZIP_STAT_SIZE) || st.size)) {
			if((comp_impl = _zip_get_compression_implementation(de->comp_method)) == NULL) {
				zip_error_set(&za->error, SLERR_ZIP_COMPNOTSUPP, 0);
				zip_source_free(s_crc);
				return -1;
			}
			s2 = comp_impl(za, s_crc, de->comp_method, ZIP_CODEC_ENCODE);
			zip_source_free(s_crc);
			if(s2 == NULL) {
				return -1;
			}
		}
		else {
			s2 = s_crc;
		}
	}
	else {
		zip_source_keep(src);
		s2 = src;
	}
	if((offdata = zip_source_tell_write(za->src)) < 0) {
		return -1;
	}
	ret = copy_source(za, s2);
	if(zip_source_stat(s2, &st) < 0)
		ret = -1;
	zip_source_free(s2);
	if(ret < 0)
		return -1;
	if((offend = zip_source_tell_write(za->src)) < 0) {
		return -1;
	}
	if(zip_source_seek_write(za->src, offstart, SEEK_SET) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	if((st.valid & (ZIP_STAT_COMP_METHOD|ZIP_STAT_CRC|ZIP_STAT_SIZE)) != (ZIP_STAT_COMP_METHOD|ZIP_STAT_CRC|ZIP_STAT_SIZE))
		return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
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
	if((ret = _zip_dirent_write(za, de, flags)) < 0)
		return -1;
	if(is_zip64 != ret) {
		// Zip64 mismatch between preliminary file header written before data and final file header written afterwards 
		return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
	}
	if(zip_source_seek_write(za->src, offend, SEEK_SET) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		return -1;
	}
	return 0;
}

static int write_cdir(zip_t * za, const zip_filelist_t * filelist, uint64 survivors)
{
	int    result = 0;
	int64  cd_start = zip_source_tell_write(za->src);
	if(cd_start < 0)
		result = -1;
	else {
		int64 size = _zip_cdir_write(za, filelist, survivors);
		if(size < 0)
			result = -1;
		else {
			int64 end = zip_source_tell_write(za->src);
			if(end < 0)
				result = -1;
		}
	}
	return result;
}

ZIP_EXTERN int zip_close(zip_t * za)
{
	uint64 i, j, survivors;
	int64  off;
	int    error;
	zip_filelist_t * filelist;
	int    changed;
	if(za == NULL)
		return -1;
	changed = _zip_changed(za, &survivors);
	// don't create zip files with no entries 
	if(survivors == 0) {
		if((za->open_flags & ZIP_TRUNCATE) || changed) {
			if(zip_source_remove(za->src) < 0) {
				_zip_error_set_from_source(&za->error, za->src);
				return -1;
			}
		}
		zip_discard(za);
		return 0;
	}
	if(!changed) {
		zip_discard(za);
		return 0;
	}
	if(survivors > za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
	if((filelist = (zip_filelist_t*)SAlloc::M(sizeof(filelist[0])*(size_t)survivors)) == NULL)
		return -1;
	// create list of files with index into original archive 
	for(i = j = 0; i<za->nentry; i++) {
		if(!za->entry[i].deleted) {
			if(j >= survivors) {
				SAlloc::F(filelist);
				return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			}
			filelist[j].idx = i;
			j++;
		}
	}
	if(j < survivors) {
		SAlloc::F(filelist);
		return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
	}
	if(zip_source_begin_write(za->src) < 0) {
		_zip_error_set_from_source(&za->error, za->src);
		SAlloc::F(filelist);
		return -1;
	}
	error = 0;
	for(j = 0; j < survivors; j++) {
		int new_data;
		zip_entry_t * entry;
		zip_dirent_t * de;
		i = filelist[j].idx;
		entry = za->entry+i;
		new_data = (ZIP_ENTRY_DATA_CHANGED(entry) || ZIP_ENTRY_CHANGED(entry, ZIP_DIRENT_COMP_METHOD));
		// create new local directory entry 
		if(entry->changes == NULL) {
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
			error = 1;
			break;
		}
		de->offset = (uint64)off;
		if(new_data) {
			zip_source_t * zs = NULL;
			if(!ZIP_ENTRY_DATA_CHANGED(entry)) {
				if((zs = _zip_source_zip_new(za, za, i, ZIP_FL_UNCHANGED, 0, 0, NULL)) == NULL) {
					error = 1;
					break;
				}
			}
			// add_data writes dirent
			if(add_data(za, zs ? zs : entry->source, de) < 0) {
				error = 1;
				zip_source_free(zs);
				break;
			}
			zip_source_free(zs);
		}
		else {
			uint64 offset;
			// when copying data, all sizes are known -> no data descriptor needed 
			de->bitflags &= (uint16) ~ZIP_GPBF_DATA_DESCRIPTOR;
			if(_zip_dirent_write(za, de, ZIP_FL_LOCAL) < 0) {
				error = 1;
				break;
			}
			if((offset = _zip_file_get_offset(za, i, &za->error)) == 0) {
				error = 1;
				break;
			}
			if(zip_source_seek(za->src, (int64)offset, SEEK_SET) < 0) {
				_zip_error_set_from_source(&za->error, za->src);
				error = 1;
				break;
			}
			if(copy_data(za, de->comp_size) < 0) {
				error = 1;
				break;
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
	}
	if(error) {
		zip_source_rollback_write(za->src);
		return -1;
	}
	zip_discard(za);
	return 0;
}

int _zip_changed(const zip_t * za, uint64 * survivorsp)
{
	uint64 survivors = 0;
	int    changed = (za->comment_changed || za->ch_flags != za->flags) ? 1 : 0;
	for(uint64 i = 0; i < za->nentry; i++) {
		if(za->entry[i].deleted || za->entry[i].source || (za->entry[i].changes && za->entry[i].changes->changed != 0))
			changed = 1;
		if(!za->entry[i].deleted)
			survivors++;
	}
	ASSIGN_PTR(survivorsp, survivors);
	return changed;
}
//
// SOURCE DEFLATE
//
struct ZipDeflate {
	zip_error_t error;
	bool   eof;
	bool   can_store;
	bool   is_stored;
	int    mem_level;
	uint64 size;
	uint8  buffer[BUFSIZE];
	z_stream zstr;
};

static void FASTCALL deflate_free(ZipDeflate * ctx)
{
	SAlloc::F(ctx);
}

static int64 compress_read(zip_source_t * src, ZipDeflate * ctx, void * data, uint64 len)
{
	int end, ret;
	int64 n;
	uint64 out_offset;
	uInt out_len;
	if(zip_error_code_zip(&ctx->error) != SLERR_SUCCESS)
		return -1;
	if(len == 0 || ctx->is_stored) {
		return 0;
	}
	out_offset = 0;
	out_len = (uInt)MIN(UINT_MAX, len);
	ctx->zstr.next_out = static_cast<Bytef *>(data);
	ctx->zstr.avail_out = out_len;
	end = 0;
	while(!end) {
		ret = deflate(&ctx->zstr, ctx->eof ? Z_FINISH : 0);
		switch(ret) {
			case Z_STREAM_END:
			    if(ctx->can_store && ctx->zstr.total_in <= ctx->zstr.total_out) {
				    ctx->is_stored = true;
				    ctx->size = ctx->zstr.total_in;
				    memcpy(data, ctx->buffer, (size_t)ctx->size);
				    return (int64)ctx->size;
			    }
			// @fallthrough
			case Z_OK:
			    /* all ok */
			    if(ctx->zstr.avail_out == 0) {
				    out_offset += out_len;
				    if(out_offset < len) {
					    out_len = (uInt)MIN(UINT_MAX, len-out_offset);
					    ctx->zstr.next_out = (Bytef *)data+out_offset;
					    ctx->zstr.avail_out = out_len;
				    }
				    else {
					    ctx->can_store = false;
					    end = 1;
				    }
			    }
			    else if(ctx->eof && ctx->zstr.avail_in == 0)
				    end = 1;
			    break;
			case Z_BUF_ERROR:
			    if(ctx->zstr.avail_in == 0) {
				    if(ctx->eof) {
					    end = 1;
					    break;
				    }
				    if((n = zip_source_read(src, ctx->buffer, sizeof(ctx->buffer))) < 0) {
					    _zip_error_set_from_source(&ctx->error, src);
					    end = 1;
					    break;
				    }
				    else if(n == 0) {
					    ctx->eof = true;
					    // @todo check against stat of src? 
					    ctx->size = ctx->zstr.total_in;
				    }
				    else {
					    if(ctx->zstr.total_in > 0)
						    ctx->can_store = false; // we overwrote a previously filled ctx->buffer 
					    ctx->zstr.next_in = static_cast<Bytef *>(ctx->buffer);
					    ctx->zstr.avail_in = (uInt)n;
				    }
				    continue;
			    }
			// @fallthrough
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_STREAM_ERROR:
			case Z_MEM_ERROR:
			    zip_error_set(&ctx->error, SLERR_ZIP_ZLIB, ret);
			    end = 1;
			    break;
		}
	}
	if(ctx->zstr.avail_out < len) {
		ctx->can_store = false;
		return (int64)(len - ctx->zstr.avail_out);
	}
	return (zip_error_code_zip(&ctx->error) == SLERR_SUCCESS) ? 0 : -1;
}

static int64 decompress_read(zip_source_t * src, ZipDeflate * ctx, void * data, uint64 len)
{
	int end, ret;
	int64 n;
	uint64 out_offset;
	uInt out_len;
	if(zip_error_code_zip(&ctx->error) != SLERR_SUCCESS)
		return -1;
	if(len == 0)
		return 0;
	out_offset = 0;
	out_len = (uInt)MIN(UINT_MAX, len);
	ctx->zstr.next_out = static_cast<Bytef *>(data);
	ctx->zstr.avail_out = out_len;
	end = 0;
	while(!end) {
		ret = inflate(&ctx->zstr, Z_SYNC_FLUSH);
		switch(ret) {
			case Z_OK:
			    if(ctx->zstr.avail_out == 0) {
				    out_offset += out_len;
				    if(out_offset < len) {
					    out_len = (uInt)MIN(UINT_MAX, len-out_offset);
					    ctx->zstr.next_out = (Bytef *)data+out_offset;
					    ctx->zstr.avail_out = out_len;
				    }
				    else {
					    end = 1;
				    }
			    }
			    break;

			case Z_STREAM_END:
			    ctx->eof = 1;
			    end = 1;
			    break;

			case Z_BUF_ERROR:
			    if(ctx->zstr.avail_in == 0) {
				    if(ctx->eof) {
					    end = 1;
					    break;
				    }
				    if((n = zip_source_read(src, ctx->buffer, sizeof(ctx->buffer))) < 0) {
					    _zip_error_set_from_source(&ctx->error, src);
					    end = 1;
					    break;
				    }
				    else if(n == 0) {
					    ctx->eof = 1;
				    }
				    else {
					    ctx->zstr.next_in = (Bytef *)ctx->buffer;
					    ctx->zstr.avail_in = (uInt)n;
				    }
				    continue;
			    }
			// @fallthrough
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_STREAM_ERROR:
			case Z_MEM_ERROR:
			    zip_error_set(&ctx->error, SLERR_ZIP_ZLIB, ret);
			    end = 1;
			    break;
		}
	}
	if(ctx->zstr.avail_out < len)
		return (int64)(len - ctx->zstr.avail_out);
	return (zip_error_code_zip(&ctx->error) == SLERR_SUCCESS) ? 0 : -1;
}

static int64 deflate_compress(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd)
{
	int ret;
	ZipDeflate * ctx = static_cast<ZipDeflate *>(ud);
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    ctx->zstr.zalloc = Z_NULL;
		    ctx->zstr.zfree = Z_NULL;
		    ctx->zstr.opaque = NULL;
		    ctx->zstr.avail_in = 0;
		    ctx->zstr.next_in = NULL;
		    ctx->zstr.avail_out = 0;
		    ctx->zstr.next_out = NULL;
		    // negative value to tell zlib not to write a header 
		    if((ret = deflateInit2(&ctx->zstr, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, ctx->mem_level, Z_DEFAULT_STRATEGY)) != Z_OK)
			    return zip_error_set(&ctx->error, SLERR_ZIP_ZLIB, ret);
			else
				return 0;
		case ZIP_SOURCE_READ:
		    return compress_read(src, ctx, data, len);
		case ZIP_SOURCE_CLOSE:
		    deflateEnd(&ctx->zstr);
		    return 0;
		case ZIP_SOURCE_STAT:
			{
				zip_stat_t * st = (zip_stat_t*)data;
				st->comp_method = ctx->is_stored ? ZIP_CM_STORE : ZIP_CM_DEFLATE;
				st->valid |= ZIP_STAT_COMP_METHOD;
				if(ctx->eof) {
					st->comp_size = ctx->size;
					st->valid |= ZIP_STAT_COMP_SIZE;
				}
				else
					st->valid &= ~ZIP_STAT_COMP_SIZE;
			}
		    return 0;
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    deflate_free(ctx);
		    return 0;
		case ZIP_SOURCE_SUPPORTS:
		    return ZIP_SOURCE_SUPPORTS_READABLE;
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_INTERNAL, 0);
	}
}

static int64 deflate_decompress(zip_source_t * src, void * ud, void * data, uint64 len, zip_source_cmd_t cmd)
{
	int64 n;
	int ret;
	ZipDeflate * ctx = static_cast<ZipDeflate *>(ud);
	switch(cmd) {
		case ZIP_SOURCE_OPEN:
		    if((n = zip_source_read(src, ctx->buffer, sizeof(ctx->buffer))) < 0) {
			    _zip_error_set_from_source(&ctx->error, src);
			    return -1;
		    }
		    ctx->zstr.zalloc = Z_NULL;
		    ctx->zstr.zfree = Z_NULL;
		    ctx->zstr.opaque = NULL;
		    ctx->zstr.next_in = static_cast<Bytef *>(ctx->buffer);
		    ctx->zstr.avail_in = (uInt)n;
		    // negative value to tell zlib that there is no header 
		    if((ret = inflateInit2(&ctx->zstr, -MAX_WBITS)) != Z_OK)
			    return zip_error_set(&ctx->error, SLERR_ZIP_ZLIB, ret);
			else
				return 0;
		case ZIP_SOURCE_READ:
		    return decompress_read(src, ctx, data, len);
		case ZIP_SOURCE_CLOSE:
		    inflateEnd(&ctx->zstr);
		    return 0;
		case ZIP_SOURCE_STAT:
	    {
		    zip_stat_t * st = static_cast<zip_stat_t *>(data);
		    st->comp_method = ZIP_CM_STORE;
		    if(st->comp_size > 0 && st->size > 0)
			    st->comp_size = st->size;
		    return 0;
	    }
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_SUPPORTS:
		    return zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR, ZIP_SOURCE_FREE, -1);
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}

zip_source_t * zip_source_deflate(zip_t * za, zip_source_t * src, int32 cm, int flags)
{
	ZipDeflate * ctx;
	zip_source_t * s2;
	if(src == NULL || (cm != ZIP_CM_DEFLATE && !ZIP_CM_IS_DEFAULT(cm))) {
		zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = static_cast<ZipDeflate *>(SAlloc::M(sizeof(*ctx)))) == NULL) {
		zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	zip_error_init(&ctx->error);
	ctx->eof = false;
	ctx->is_stored = false;
	ctx->can_store = ZIP_CM_IS_DEFAULT(cm);
	if(flags & ZIP_CODEC_ENCODE) {
		ctx->mem_level = MAX_MEM_LEVEL;
	}
	if((s2 = zip_source_layered(za, src, ((flags & ZIP_CODEC_ENCODE) ? deflate_compress : deflate_decompress), ctx)) == NULL) {
		deflate_free(ctx);
		return NULL;
	}
	return s2;
}
//
// ZIP HASH
//
struct zip_hash_entry {
	const uint8 * name;
	int64 orig_index;
	int64 current_index;
	struct zip_hash_entry * next;
};

typedef struct zip_hash_entry zip_hash_entry_t;

struct zip_hash {
	uint16 table_size;
	zip_hash_entry_t ** table;
};

zip_hash_t * _zip_hash_new(uint16 table_size, zip_error_t * error)
{
	zip_hash_t * hash;
	if(table_size == 0) {
		zip_error_set(error, SLERR_ZIP_INTERNAL, 0);
		return NULL;
	}
	if((hash = static_cast<zip_hash_t *>(SAlloc::M(sizeof(zip_hash_t)))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	hash->table_size = table_size;
	if((hash->table = static_cast<zip_hash_entry_t **>(SAlloc::C(table_size, sizeof(zip_hash_entry_t *)))) == NULL) {
		SAlloc::F(hash);
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	return hash;
}

static void FASTCALL _free_list(zip_hash_entry_t * entry)
{
	do {
		zip_hash_entry_t * next = entry->next;
		SAlloc::F(entry);
		entry = next;
	} while(entry);
}

void _zip_hash_free(zip_hash_t * hash)
{
	if(hash) {
		for(uint16 i = 0; i<hash->table_size; i++) {
			if(hash->table[i] != NULL) {
				_free_list(hash->table[i]);
			}
		}
		SAlloc::F(hash->table);
		SAlloc::F(hash);
	}
}

static uint16 FASTCALL _hash_string(const uint8 * name, uint16 size)
{
#define HASH_MULTIPLIER 33
	uint16 value = 5381;
	if(!name)
		return 0;
	while(*name != 0) {
		value = static_cast<uint16>(((value * HASH_MULTIPLIER) + (uint8)*name) % size);
		name++;
	}
	return value;
}
//
// insert into hash, return error on existence or memory issues 
//
bool _zip_hash_add(zip_hash_t * hash, const uint8 * name, uint64 index, zip_flags_t flags, zip_error_t * error)
{
	uint16 hash_value;
	zip_hash_entry_t * entry;
	if(hash == NULL || name == NULL || index > ZIP_INT64_MAX) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return false;
	}
	hash_value = _hash_string(name, hash->table_size);
	for(entry = hash->table[hash_value]; entry != NULL; entry = entry->next) {
		if(strcmp((const char *)name, (const char *)entry->name) == 0) {
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
		if((entry = static_cast<zip_hash_entry_t *>(SAlloc::M(sizeof(zip_hash_entry_t)))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			return false;
		}
		entry->name = name;
		entry->next = hash->table[hash_value];
		hash->table[hash_value] = entry;
		entry->orig_index = -1;
	}
	if(flags & ZIP_FL_UNCHANGED) {
		entry->orig_index = (int64)index;
	}
	entry->current_index = (int64)index;
	return true;
}
//
// remove entry from hash, error if not found 
//
bool _zip_hash_delete(zip_hash_t * hash, const uint8 * name, zip_error_t * error)
{
	if(hash == NULL || name == NULL) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	else {
		uint16 hash_value = _hash_string(name, hash->table_size);
		zip_hash_entry_t * previous = NULL;
		zip_hash_entry_t * entry = hash->table[hash_value];
		while(entry) {
			if(strcmp((const char *)name, (const char *)entry->name) == 0) {
				if(entry->orig_index == -1) {
					if(previous) {
						previous->next = entry->next;
					}
					else {
						hash->table[hash_value] = entry->next;
					}
					SAlloc::F(entry);
				}
				else {
					entry->current_index = -1;
				}
				return true;
			}
			previous = entry;
			entry = entry->next;
		}
		zip_error_set(error, SLERR_ZIP_NOENT, 0);
	}
	return false;
}
//
// find value for entry in hash, -1 if not found 
//
int64 _zip_hash_lookup(zip_hash_t * hash, const uint8 * name, zip_flags_t flags, zip_error_t * error)
{
	if(!hash || !name) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	else {
		uint16 hash_value = _hash_string(name, hash->table_size);
		for(zip_hash_entry_t * entry = hash->table[hash_value]; entry != NULL; entry = entry->next) {
			if(strcmp((const char *)name, (const char *)entry->name) == 0) {
				if(flags & ZIP_FL_UNCHANGED) {
					if(entry->orig_index != -1)
						return entry->orig_index;
				}
				else {
					if(entry->current_index != -1)
						return entry->current_index;
				}
				break;
			}
		}
		zip_error_set(error, SLERR_ZIP_NOENT, 0);
	}
	return -1;
}

void _zip_hash_revert(zip_hash_t * hash)
{
	for(uint16 i = 0; i < hash->table_size; i++) {
		zip_hash_entry_t * previous = NULL;
		zip_hash_entry_t * entry = hash->table[i];
		while(entry) {
			if(entry->orig_index == -1) {
				zip_hash_entry_t * p;
				if(previous) {
					previous->next = entry->next;
				}
				else {
					hash->table[i] = entry->next;
				}
				p = entry;
				entry = entry->next;
				// previous does not change 
				SAlloc::F(p);
			}
			else {
				entry->current_index = entry->orig_index;
				previous = entry;
				entry = entry->next;
			}
		}
	}
}
//
//
//
ZIP_EXTERN int zip_file_set_mtime(zip_t * za, uint64 idx, time_t mtime, zip_flags_t flags)
{
	zip_entry_t * e;
	int changed;
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	e = za->entry+idx;
	changed = (e->orig == NULL || mtime != e->orig->last_mod);
	if(changed) {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		e->changes->last_mod = mtime;
		e->changes->changed |= ZIP_DIRENT_LAST_MOD;
	}
	else {
		if(e->changes) {
			e->changes->changed &= ~ZIP_DIRENT_LAST_MOD;
			if(e->changes->changed == 0) {
				_zip_dirent_free(e->changes);
				e->changes = NULL;
			}
		}
	}
	return 0;
}
//
//
//
ZIP_EXTERN int zip_file_set_external_attributes(zip_t * za, uint64 idx, zip_flags_t flags, uint8 opsys, uint32 attributes)
{
	zip_entry_t * e;
	int changed;
	uint8 unchanged_opsys;
	uint32 unchanged_attributes;
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	e = za->entry+idx;
	unchanged_opsys = (e->orig ? (uint8)(e->orig->version_madeby>>8) : (uint8)ZIP_OPSYS_DEFAULT);
	unchanged_attributes = e->orig ? e->orig->ext_attrib : ZIP_EXT_ATTRIB_DEFAULT;
	changed = (opsys != unchanged_opsys || attributes != unchanged_attributes);
	if(changed) {
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		e->changes->version_madeby = static_cast<uint16>((opsys << 8) | (e->changes->version_madeby & 0xff));
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
			e->changes->version_madeby = static_cast<uint16>((unchanged_opsys << 8) | (e->changes->version_madeby & 0xff));
			e->changes->ext_attrib = unchanged_attributes;
		}
	}
	return 0;
}
//
//
//
ZIP_EXTERN int zip_file_set_comment(zip_t * za, uint64 idx, const char * comment, uint16 len, zip_flags_t flags)
{
	zip_entry_t * e;
	zip_string_t * cstr;
	int changed;
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(len > 0 && comment == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(len > 0) {
		if((cstr = _zip_string_new((const uint8 *)comment, len, flags, &za->error)) == NULL)
			return -1;
		if((flags & ZIP_FL_ENCODING_ALL) == ZIP_FL_ENC_GUESS &&
		    _zip_guess_encoding(cstr, ZIP_ENCODING_UNKNOWN) == ZIP_ENCODING_UTF8_GUESSED)
			cstr->encoding = ZIP_ENCODING_UTF8_KNOWN;
	}
	else
		cstr = NULL;
	e = za->entry+idx;
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
// ZIP EXTRA FIELD API
//
ZIP_EXTERN int zip_file_extra_field_delete(zip_t * za, uint64 idx, uint16 ef_idx, zip_flags_t flags)
{
	zip_dirent_t * de;
	if((flags & ZIP_EF_BOTH) == 0)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(((flags & ZIP_EF_BOTH) == ZIP_EF_BOTH) && (ef_idx != ZIP_EXTRA_FIELD_ALL))
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(_zip_file_extra_field_prepare_for_change(za, idx) < 0)
		return -1;
	de = za->entry[idx].changes;
	de->extra_fields = _zip_ef_delete_by_id(de->extra_fields, ZIP_EXTRA_FIELD_ALL, ef_idx, flags);
	return 0;
}

ZIP_EXTERN int zip_file_extra_field_delete_by_id(zip_t * za, uint64 idx, uint16 ef_id, uint16 ef_idx, zip_flags_t flags)
{
	if((flags & ZIP_EF_BOTH) == 0)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else if(((flags & ZIP_EF_BOTH) == ZIP_EF_BOTH) && (ef_idx != ZIP_EXTRA_FIELD_ALL))
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	else if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	else if(_zip_file_extra_field_prepare_for_change(za, idx) < 0)
		return -1;
	else {
		zip_dirent_t * de = za->entry[idx].changes;
		de->extra_fields = _zip_ef_delete_by_id(de->extra_fields, ef_id, ef_idx, flags);
		return 0;
	}
}

ZIP_EXTERN const uint8 * zip_file_extra_field_get(zip_t * za, uint64 idx, uint16 ef_idx, uint16 * idp, uint16 * lenp, zip_flags_t flags)
{
	static const uint8 empty[1] = { '\0' };
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
			return (ef->size > 0) ? ef->data : empty;
		}
	}
	zip_error_set(&za->error, SLERR_ZIP_NOENT, 0);
	return NULL;
}

ZIP_EXTERN const uint8 * zip_file_extra_field_get_by_id(zip_t * za,
    uint64 idx, uint16 ef_id, uint16 ef_idx, uint16 * lenp, zip_flags_t flags)
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
	if((flags & ZIP_EF_BOTH) == 0)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
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
	if((flags & ZIP_EF_BOTH) == 0)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
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

ZIP_EXTERN int zip_file_extra_field_set(zip_t * za, uint64 idx, uint16 ef_id, uint16 ef_idx, const uint8 * data, uint16 len, zip_flags_t flags)
{
	zip_dirent_t * de;
	uint16 ls, cs;
	zip_extra_field_t * ef, * ef_prev, * ef_new;
	int i, found, new_len;
	if((flags & ZIP_EF_BOTH) == 0)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if(_zip_get_dirent(za, idx, 0, NULL) == NULL)
		return -1;
	if(ZIP_IS_RDONLY(za))
		return zip_error_set(&za->error, SLERR_ZIP_RDONLY, 0);
	if(ZIP_EF_IS_INTERNAL(ef_id))
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
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
	if(i < ef_idx && ef_idx != ZIP_EXTRA_FIELD_NEW)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	ls = (flags & ZIP_EF_LOCAL) ? _zip_ef_size(de->extra_fields, ZIP_EF_LOCAL) :  0;
	cs = (flags & ZIP_EF_CENTRAL) ? _zip_ef_size(de->extra_fields, ZIP_EF_CENTRAL) : 0;
	new_len = ls > cs ? ls : cs;
	if(found)
		new_len -= ef->size + 4;
	new_len += len + 4;
	if(new_len > ZIP_UINT16_MAX)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	if((ef_new = _zip_ef_new(ef_id, len, data, flags)) == NULL)
		return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
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
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else {
		zip_entry_t * e = za->entry+idx;
		if(e->changes && (e->changes->changed & ZIP_DIRENT_EXTRA_FIELD))
			return 0;
		if(e->orig) {
			if(_zip_read_local_ef(za, idx) < 0)
				return -1;
		}
		if(e->changes == NULL) {
			if((e->changes = _zip_dirent_clone(e->orig)) == NULL)
				return zip_error_set(&za->error, SLERR_ZIP_MEMORY, 0);
		}
		if(e->orig && e->orig->extra_fields) {
			if((e->changes->extra_fields = _zip_ef_clone(e->orig->extra_fields, &za->error)) == NULL)
				return -1;
		}
		e->changes->changed |= ZIP_DIRENT_EXTRA_FIELD;
		return 0;
	}
}
//
// ZIP_EXTRA_FIELD
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
		else {
			SETIFZ(head, def);
			if(prev)
				prev->next = def;
			prev = def;
			ef = ef->next;
		}
	}
	return head;
}

zip_extra_field_t * _zip_ef_delete_by_id(zip_extra_field_t * ef, uint16 id, uint16 id_idx, zip_flags_t flags)
{
	int    i = 0;
	zip_extra_field_t * head = ef;
	zip_extra_field_t * prev = NULL;
	for(; ef; ef = (prev ? prev->next : head)) {
		if((ef->flags & flags & ZIP_EF_BOTH) && ((ef->id == id) || (id == ZIP_EXTRA_FIELD_ALL))) {
			if(id_idx == ZIP_EXTRA_FIELD_ALL || i == id_idx) {
				ef->flags &= ~(flags & ZIP_EF_BOTH);
				if(!(ef->flags & ZIP_EF_BOTH)) {
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

const uint8 * _zip_ef_get_by_id(const zip_extra_field_t * ef, uint16 * lenp, uint16 id, uint16 id_idx, zip_flags_t flags, zip_error_t * error)
{
	static const uint8 empty[1] = { '\0' };
	int i = 0;
	for(; ef; ef = ef->next) {
		if(ef->id == id && (ef->flags & flags & ZIP_EF_BOTH)) {
			if(i < id_idx)
				i++;
			else {
				ASSIGN_PTR(lenp, ef->size);
				return (ef->size > 0) ? ef->data : empty;
			}
		}
	}
	zip_error_set(error, SLERR_ZIP_NOENT, 0);
	return NULL;
}

zip_extra_field_t * _zip_ef_merge(zip_extra_field_t * to, zip_extra_field_t * from)
{
	if(to == NULL)
		return from;
	else {
		zip_extra_field_t * ef2, * tt, * tail;
		for(tail = to; tail->next; tail = tail->next)
			;
		for(; from; from = ef2) {
			int duplicate = 0;
			ef2 = from->next;
			for(tt = to; tt; tt = tt->next) {
				if(tt->id == from->id && tt->size == from->size && memcmp(tt->data, from->data, tt->size) == 0) {
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
			ef->data = (uint8 *)_zip_memdup(data, size, 0);
			if(!ef->data) {
				ZFREE(ef);
			}
		}
		else
			ef->data = NULL;
	}
	return ef;
}

bool _zip_ef_parse(uint8 * data, uint16 len, zip_flags_t flags, zip_extra_field_t ** ef_head_p, zip_error_t * error)
{
	zip_extra_field_t * ef2;
	zip_buffer_t * buffer = _zip_buffer_new(static_cast<uint8 *>(data), len);
	if(!buffer) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return false;
	}
	else {
		zip_extra_field_t * ef_head = 0;
		zip_extra_field_t * ef = 0;
		while(_zip_buffer_ok(buffer) && _zip_buffer_left(buffer) >= 4) {
			uint16 fid = _zip_buffer_get_16(buffer);
			uint16 flen = _zip_buffer_get_16(buffer);
			uint8 * ef_data = _zip_buffer_get(buffer, flen);
			if(ef_data == NULL) {
				zip_error_set(error, SLERR_ZIP_INCONS, 0);
				_zip_buffer_free(buffer);
				_zip_ef_free(ef_head);
				return false;
			}
			else if((ef2 = _zip_ef_new(fid, flen, ef_data, flags)) == NULL) {
				zip_error_set(error, SLERR_ZIP_MEMORY, 0);
				_zip_buffer_free(buffer);
				_zip_ef_free(ef_head);
				return false;
			}
			else { 
				if(ef_head) {
					ef->next = ef2;
					ef = ef2;
				}
				else
					ef_head = ef = ef2;
			}
		}
		if(!_zip_buffer_eof(buffer)) {
			/* Android APK files align stored file data with padding in extra fields; ignore. */
			/* see https://android.googlesource.com/platform/build/+/master/tools/zipalign/ZipAlign.cpp */
			size_t glen = (size_t)_zip_buffer_left(buffer);
			uint8 * garbage;
			garbage = _zip_buffer_get(buffer, glen);
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
			size = static_cast<uint16>(size+4+ef->size);
	}
	return size;
}

int _zip_ef_write(zip_t * za, const zip_extra_field_t * ef, zip_flags_t flags)
{
	uint8 b[4];
	zip_buffer_t * buffer = _zip_buffer_new(b, sizeof(b));
	if(!buffer) {
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
	uchar b[4];
	zip_buffer_t * buffer;
	if(idx >= za->nentry)
		return zip_error_set(&za->error, SLERR_ZIP_INVAL, 0);
	else {
		zip_entry_t * e = za->entry+idx;
		if(e->orig == NULL || e->orig->local_extra_fields_read)
			return 0;
		else if(e->orig->offset + 26 > ZIP_INT64_MAX)
			return zip_error_set(&za->error, SLERR_ZIP_SEEK, EFBIG);
		else if(zip_source_seek(za->src, (int64)(e->orig->offset + 26), SEEK_SET) < 0) {
			_zip_error_set_from_source(&za->error, za->src);
			return -1;
		}
		else if((buffer = _zip_buffer_new_from_source(za->src, sizeof(b), b, &za->error)) == NULL) {
			return -1;
		}
		else {
			uint16 fname_len = _zip_buffer_get_16(buffer);
			uint16 ef_len = _zip_buffer_get_16(buffer);
			if(!_zip_buffer_eof(buffer)) {
				_zip_buffer_free(buffer);
				return zip_error_set(&za->error, SLERR_ZIP_INTERNAL, 0);
			}
			else {
				_zip_buffer_free(buffer);
				if(ef_len > 0) {
					if(zip_source_seek(za->src, fname_len, SEEK_CUR) < 0)
						return zip_error_set(&za->error, SLERR_ZIP_SEEK, errno);
					else {
						zip_extra_field_t * ef;
						uint8 * ef_raw = _zip_read_data(NULL, za->src, ef_len, 0, &za->error);
						if(ef_raw == NULL)
							return -1;
						else if(!_zip_ef_parse(ef_raw, ef_len, ZIP_EF_LOCAL, &ef, &za->error)) {
							SAlloc::F(ef_raw);
							return -1;
						}
						else {
							SAlloc::F(ef_raw);
							if(ef) {
								ef = _zip_ef_remove_internal(ef);
								e->orig->extra_fields = _zip_ef_merge(e->orig->extra_fields, ef);
							}
						}
					}
				}
				e->orig->local_extra_fields_read = 1;
				if(e->changes && e->changes->local_extra_fields_read == 0) {
					e->changes->extra_fields = e->orig->extra_fields;
					e->changes->local_extra_fields_read = 1;
				}
				return 0;
			}
		}
	}
}
//
// ZIP_OPEN
//
typedef enum {
	EXISTS_ERROR = -1,
	EXISTS_NOT = 0,
	EXISTS_EMPTY,
	EXISTS_NONEMPTY,
} exists_t;

static zip_t * _zip_allocate_new(zip_source_t * src, uint flags, zip_error_t * error);
static int64 _zip_checkcons(zip_t * za, zip_cdir_t * cdir, zip_error_t * error);
static zip_cdir_t * _zip_find_central_dir(zip_t * za, uint64 len);
static exists_t _zip_file_exists(zip_source_t * src, zip_error_t * error);
static int _zip_headercomp(const zip_dirent_t *, const zip_dirent_t *);
static uchar * _zip_memmem(const uchar *, size_t, const uchar *, size_t);
static zip_cdir_t * _zip_read_cdir(zip_t * za, zip_buffer_t * buffer, uint64 buf_offset, zip_error_t * error);
static zip_cdir_t * _zip_read_eocd(zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error);
static zip_cdir_t * _zip_read_eocd64(zip_source_t * src, zip_buffer_t * buffer, uint64 buf_offset, uint flags, zip_error_t * error);

ZIP_EXTERN zip_t * zip_open(const char * fn, int _flags, int * zep)
{
	zip_t * za;
	zip_source_t * src;
	zip_error_t error;
	zip_error_init(&error);
	if((src = zip_source_file_create(fn, 0, -1, &error)) == NULL) {
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
		    ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_SEEK, ZIP_SOURCE_TELL, ZIP_SOURCE_STAT, -1);
		needed_support_write = zip_source_make_command_bitmap(ZIP_SOURCE_BEGIN_WRITE,
		    ZIP_SOURCE_COMMIT_WRITE, ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE, -1);
	}
	if((supported & needed_support_read) != needed_support_read) {
		zip_error_set(error, SLERR_ZIP_OPNOTSUPP, 0);
		return NULL;
	}
	if((supported & needed_support_write) != needed_support_write) {
		flags |= ZIP_RDONLY;
	}
	if((flags & (ZIP_RDONLY|ZIP_TRUNCATE)) == (ZIP_RDONLY|ZIP_TRUNCATE)) {
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
		    zip_t * za = 0;
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
			    // ZIP_CREATE gets ignored if file exists and not ZIP_EXCL, just like open() 
			    za = _zip_open(src, flags, error);
		    }
		    if(za == NULL)
			    zip_source_close(src);
		    return za;
	    }
	}
}

ZIP_EXTERN int zip_archive_set_tempdir(zip_t * za, const char * tempdir)
{
	char * new_tempdir = 0;
	if(tempdir) {
		new_tempdir = sstrdup(tempdir);
		if(!new_tempdir)
			return zip_error_set(&za->error, SLERR_ZIP_MEMORY, errno);
	}
	SAlloc::F(za->tempdir);
	za->tempdir = new_tempdir;
	return 0;
}

zip_t * _zip_open(zip_source_t * src, uint flags, zip_error_t * error)
{
	zip_t * za;
	zip_cdir_t * cdir;
	zip_stat_t st;
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
	/* treat empty files as empty archives */
	if(len == 0) {
		if((za = _zip_allocate_new(src, flags, error)) == NULL) {
			zip_source_free(src);
			return NULL;
		}
		return za;
	}
	if((za = _zip_allocate_new(src, flags, error)) == NULL) {
		return NULL;
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
	for(idx = 0; idx < za->nentry; idx++) {
		const uint8 * name = _zip_string_get(za->entry[idx].orig->filename, NULL, 0, error);
		if(!name) {
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
// Descr: tries to find a valid end-of-central-directory at the beginning of
// buf, and then the corresponding central directory entries.
// Returns: a struct zip_cdir which contains the central directory entries, or NULL if unsuccessful. 
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
			if((cd->comment = _zip_string_new(_zip_buffer_get(buffer, comment_len), comment_len, ZIP_FL_ENC_GUESS, error)) == NULL) {
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
		if((cd_buffer = _zip_buffer_new(data, (size_t)cd->size)) == NULL) {
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
		// possible consistency check: cd->offset = len-(cd->size+cd->comment_len+EOCDLEN) ? 
		if(zip_source_tell(za->src) != (int64)cd->offset) {
			zip_error_set(error, SLERR_ZIP_NOZIP, 0);
			_zip_cdir_free(cd);
			return NULL;
		}
	}
	left = (uint64)cd->size;
	i = 0;
	while(i<cd->nentry && left > 0) {
		int64 entry_size;
		if((cd->entry[i].orig = _zip_dirent_new()) == NULL ||
		    (entry_size = _zip_dirent_read(cd->entry[i].orig, za->src, cd_buffer, false, error)) < 0) {
			_zip_cdir_free(cd);
			_zip_buffer_free(cd_buffer);
			return NULL;
		}
		i++;
		left -= (uint64)entry_size;
	}
	if(i != cd->nentry) {
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
				_zip_buffer_free(cd_buffer);
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
// Descr: Checks the consistency of the central directory by comparing central
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
	for(i = 0; i<cd->nentry; i++) {
		if(cd->entry[i].orig->offset < min)
			min = cd->entry[i].orig->offset;
		if(min > (uint64)cd->offset)
			return zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		j = cd->entry[i].orig->offset + cd->entry[i].orig->comp_size + _zip_string_length(cd->entry[i].orig->filename) + LENTRYSIZE;
		if(j > max)
			max = j;
		if(max > (uint64)cd->offset)
			return zip_error_set(error, SLERR_ZIP_NOZIP, 0);
		if(zip_source_seek(za->src, (int64)cd->entry[i].orig->offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, za->src);
			return -1;
		}
		if(_zip_dirent_read(&temp, za->src, NULL, true, error) == -1) {
			_zip_dirent_finalize(&temp);
			return -1;
		}
		if(_zip_headercomp(cd->entry[i].orig, &temp) != 0) {
			_zip_dirent_finalize(&temp);
			return zip_error_set(error, SLERR_ZIP_INCONS, 0);
		}
		cd->entry[i].orig->extra_fields = _zip_ef_merge(cd->entry[i].orig->extra_fields, temp.extra_fields);
		cd->entry[i].orig->local_extra_fields_read = 1;
		temp.extra_fields = NULL;
		_zip_dirent_finalize(&temp);
	}
	return (max-min) < ZIP_INT64_MAX ? (int64)(max-min) : ZIP_INT64_MAX;
}
// 
// Descr: compares a central directory entry and a local file header
// Return: 0 if they are consistent, -1 if not. */
// 
static int _zip_headercomp(const zip_dirent_t * central, const zip_dirent_t * local)
{
	if((central->version_needed != local->version_needed)
#if 0
	    /* some zip-files have different values in local
	       and global headers for the bitflags */
	   || (central->bitflags != local->bitflags)
#endif
	   || (central->comp_method != local->comp_method)
	   || (central->last_mod != local->last_mod)
	   || !_zip_string_equal(central->filename, local->filename))
		return -1;
	if((central->crc != local->crc) || (central->comp_size != local->comp_size) || (central->uncomp_size != local->uncomp_size)) {
		/* InfoZip stores valid values in local header even when data descriptor is used.
		   This is in violation of the appnote. */
		if(((local->bitflags & ZIP_GPBF_DATA_DESCRIPTOR) == 0 || local->crc != 0 || local->comp_size != 0 || local->uncomp_size != 0))
			return -1;
	}
	return 0;
}

static zip_t * _zip_allocate_new(zip_source_t * src, uint flags, zip_error_t * error)
{
	zip_t * za = _zip_new(error);
	if(za) {
		za->src = src;
		za->open_flags = flags;
		if(flags & ZIP_RDONLY) {
			za->flags |= ZIP_AFL_RDONLY;
			za->ch_flags |= ZIP_AFL_RDONLY;
		}
	}
	return za;
}

/*
 * tests for file existence
 */
static exists_t _zip_file_exists(zip_source_t * src, zip_error_t * error)
{
	zip_stat_t st;
	zip_stat_init(&st);
	if(zip_source_stat(src, &st) != 0) {
		zip_error_t * src_error = zip_source_error(src);
		if(zip_error_code_zip(src_error) == SLERR_ZIP_READ && zip_error_code_system(src_error) == ENOENT) {
			return EXISTS_NOT;
		}
		else {
			_zip_error_copy(error, src_error);
			return EXISTS_ERROR;
		}
	}
	else
		return (st.valid & ZIP_STAT_SIZE) && st.size == 0 ? EXISTS_EMPTY : EXISTS_NONEMPTY;
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
	if((buffer = _zip_buffer_new_from_source(za->src, (size_t)buflen, NULL, &za->error)) == NULL) {
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
	while((match = _zip_memmem(match, (size_t)(_zip_buffer_left(buffer)-(EOCDLEN-4)), (const uchar *)EOCD_MAGIC, 4)) != NULL) {
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
				best = (za->open_flags & ZIP_CHECKCONS) ? _zip_checkcons(za, cdir, &error) : 0;
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

static uchar * _zip_memmem(const uchar * big, size_t biglen, const uchar * little, size_t littlelen)
{
	const uchar * p;
	if((biglen < littlelen) || (littlelen == 0))
		return NULL;
	p = big-1;
	while((p = (const uchar *)memchr(p+1, little[0], (size_t)(big-(p+1))+(size_t)(biglen-littlelen)+1)) != NULL) {
		if(memcmp(p+1, little+1, littlelen-1)==0)
			return (uchar *)p;
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
	if(offset+size < offset) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	if(offset+size > buf_offset + eocd_offset) {
		/* cdir spans past EOCD record */
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((flags & ZIP_CHECKCONS) && offset+size != buf_offset + eocd_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((cd = _zip_cdir_new(nentry, error)) == NULL)
		return NULL;
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
	uint64 size, nentry, i;
	bool free_buffer;
	uint32 num_disks, num_disks64, eocd_disk, eocd_disk64;
	uint64 eocdloc_offset = _zip_buffer_offset(buffer);
	_zip_buffer_get(buffer, 4); /* magic already verified */
	num_disks = _zip_buffer_get_16(buffer);
	eocd_disk = _zip_buffer_get_16(buffer);
	eocd_offset = _zip_buffer_get_64(buffer);
	if(eocd_offset > ZIP_INT64_MAX || eocd_offset + EOCD64LEN < eocd_offset) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	if(eocd_offset + EOCD64LEN > eocdloc_offset + buf_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if(eocd_offset >= buf_offset && eocd_offset + EOCD64LEN <= buf_offset + _zip_buffer_size(buffer)) {
		_zip_buffer_set_offset(buffer, eocd_offset - buf_offset);
		free_buffer = false;
	}
	else {
		if(zip_source_seek(src, (int64)eocd_offset, SEEK_SET) < 0) {
			_zip_error_set_from_source(error, src);
			return NULL;
		}
		else if((buffer = _zip_buffer_new_from_source(src, EOCD64LEN, eocd, error)) == NULL)
			return NULL;
		else
			free_buffer = true;
	}
	if(memcmp(_zip_buffer_get(buffer, 4), EOCD64_MAGIC, 4) != 0) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		if(free_buffer)
			_zip_buffer_free(buffer);
		return NULL;
	}
	size = _zip_buffer_get_64(buffer);
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
		return NULL;
	}
	if(num_disks != 0 || eocd_disk != 0) {
		zip_error_set(error, SLERR_ZIP_MULTIDISK, 0);
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
	if(offset > ZIP_INT64_MAX || offset+size < offset) {
		zip_error_set(error, SLERR_ZIP_SEEK, EFBIG);
		return NULL;
	}
	if((flags & ZIP_CHECKCONS) && offset+size != eocd_offset) {
		zip_error_set(error, SLERR_ZIP_INCONS, 0);
		return NULL;
	}
	if((cd = _zip_cdir_new(nentry, error)) == NULL)
		return NULL;
	cd->size = size;
	cd->offset = offset;
	return cd;
}
//
// ZIP_UTF-8
//
static const uint16 _cp437_to_unicode[256] = {
	/* 0x00 - 0x0F */
	0x2007, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
	0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
	/* 0x10 - 0x1F */
	0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
	0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
	/* 0x20 - 0x2F */
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	/* 0x30 - 0x3F */
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	/* 0x40 - 0x4F */
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	/* 0x50 - 0x5F */
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	/* 0x60 - 0x6F */
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	/* 0x70 - 0x7F */
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
	/* 0x80 - 0x8F */
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	/* 0x90 - 0x9F */
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	/* 0xA0 - 0xAF */
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	/* 0xB0 - 0xBF */
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	/* 0xC0 - 0xCF */
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	/* 0xD0 - 0xDF */
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	/* 0xE0 - 0xEF */
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	/* 0xF0 - 0xFF */
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

#define UTF_8_LEN_2_MASK     0xe0
#define UTF_8_LEN_2_MATCH    0xc0
#define UTF_8_LEN_3_MASK     0xf0
#define UTF_8_LEN_3_MATCH    0xe0
#define UTF_8_LEN_4_MASK     0xf8
#define UTF_8_LEN_4_MATCH    0xf0
#define UTF_8_CONTINUE_MASK  0xc0
#define UTF_8_CONTINUE_MATCH 0x80

zip_encoding_type_t _zip_guess_encoding(zip_string_t * str, zip_encoding_type_t expected_encoding)
{
	zip_encoding_type_t enc;
	const uint8 * name;
	uint32 i, j, ulen;
	if(!str)
		return ZIP_ENCODING_ASCII;
	name = str->raw;
	if(str->encoding != ZIP_ENCODING_UNKNOWN)
		enc = str->encoding;
	else {
		enc = ZIP_ENCODING_ASCII;
		for(i = 0; i<str->length; i++) {
			if((name[i] > 31 && name[i] < 128) || oneof3(name[i], '\r', '\n', '\t'))
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
			if(i + ulen >= str->length) {
				enc = ZIP_ENCODING_CP437;
				break;
			}
			for(j = 1; j<=ulen; j++) {
				if((name[i+j] & UTF_8_CONTINUE_MASK) != UTF_8_CONTINUE_MATCH) {
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
	if(codepoint < 0x0080)
		return 1;
	else if(codepoint < 0x0800)
		return 2;
	else if(codepoint < 0x10000)
		return 3;
	else
		return 4;
}

static uint32 _zip_unicode_to_utf8(uint32 codepoint, uint8 * buf)
{
	if(codepoint < 0x0080) {
		buf[0] = (uint8)(codepoint & 0xff);
		return 1;
	}
	else if(codepoint < 0x0800) {
		buf[0] = (uint8)(UTF_8_LEN_2_MATCH | ((codepoint >> 6) & 0x1f));
		buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
		return 2;
	}
	else if(codepoint < 0x10000) {
		buf[0] = (uint8)(UTF_8_LEN_3_MATCH | ((codepoint >> 12) & 0x0f));
		buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 6) & 0x3f));
		buf[2] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
		return 3;
	}
	else {
		buf[0] = (uint8)(UTF_8_LEN_4_MATCH | ((codepoint >> 18) & 0x07));
		buf[1] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 12) & 0x3f));
		buf[2] = (uint8)(UTF_8_CONTINUE_MATCH | ((codepoint >> 6) & 0x3f));
		buf[3] = (uint8)(UTF_8_CONTINUE_MATCH | (codepoint & 0x3f));
		return 4;
	}
}

uint8 * _zip_cp437_to_utf8(const uint8 * const _cp437buf, uint32 len, uint32 * utf8_lenp, zip_error_t * error)
{
	uint8 * utf8buf = 0;
	if(len == 0) {
		ASSIGN_PTR(utf8_lenp, 0);
	}
	else {
		const uint8 * cp437buf = static_cast<const uint8 *>(_cp437buf);
		uint32 buflen = 1;
		uint32 i;
		for(i = 0; i < len; i++)
			buflen += _zip_unicode_to_utf8_len(_cp437_to_unicode[cp437buf[i]]);
		if((utf8buf = static_cast<uint8 *>(SAlloc::M(buflen))) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		}
		else {
			uint32 offset = 0;
			for(i = 0; i < len; i++)
				offset += _zip_unicode_to_utf8(_cp437_to_unicode[cp437buf[i]], utf8buf+offset);
			utf8buf[buflen-1] = 0;
			ASSIGN_PTR(utf8_lenp, buflen-1);
		}
	}
	return utf8buf;
}
//
// ZIP_SOURCE_FILEP
//
struct ZipReadFileBlock {
	zip_error_t error; /* last error information */
	int64 supports;
	/* reading */
	char * fname; /* name of file to read from */
	FILE * f; /* file to read from */
	zip_stat_t st; /* stat information passed in */
	uint64 start; /* start offset of data to read */
	uint64 end; /* end offset of data to read, 0 for up to EOF */
	uint64 current; /* current offset */
	/* writing */
	char * tmpname;
	FILE * fout;
};

static int64 read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd);
static int create_temp_output(ZipReadFileBlock * ctx);
static int _zip_fseek_u(FILE * f, uint64 offset, int whence, zip_error_t * error);
static int _zip_fseek(FILE * f, int64 offset, int whence, zip_error_t * error);

ZIP_EXTERN zip_source_t * zip_source_filep(zip_t * za, FILE * file, uint64 start, int64 len)
{
	return za ? zip_source_filep_create(file, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_filep_create(FILE * file, uint64 start, int64 length, zip_error_t * error)
{
	if(file == NULL || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else
		return _zip_source_file_or_p(NULL, file, start, length, NULL, error);
}

zip_source_t * _zip_source_file_or_p(const char * fname, FILE * file, uint64 start, int64 len, const zip_stat_t * st, zip_error_t * error)
{
	ZipReadFileBlock * ctx;
	zip_source_t * zs;
	if(file == NULL && fname == NULL) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = static_cast<ZipReadFileBlock *>(SAlloc::M(sizeof(ZipReadFileBlock)))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->fname = NULL;
	if(fname) {
		ctx->fname = sstrdup(fname);
		if(!ctx->fname) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			SAlloc::F(ctx);
			return NULL;
		}
	}
	ctx->f = file;
	ctx->start = start;
	ctx->end = (len < 0 ? 0 : start+(uint64)len);
	if(st) {
		memcpy(&ctx->st, st, sizeof(ctx->st));
		ctx->st.name = NULL;
		ctx->st.valid &= ~ZIP_STAT_NAME;
	}
	else {
		zip_stat_init(&ctx->st);
	}
	ctx->tmpname = NULL;
	ctx->fout = NULL;
	zip_error_init(&ctx->error);
	ctx->supports = ZIP_SOURCE_SUPPORTS_READABLE | zip_source_make_command_bitmap(ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1);
	if(ctx->fname) {
		struct stat sb;
		if(stat(ctx->fname, &sb) < 0 || S_ISREG(sb.st_mode)) {
			ctx->supports = ZIP_SOURCE_SUPPORTS_WRITABLE;
		}
	}
	else if(fseeko(ctx->f, 0, SEEK_CUR) == 0) {
		ctx->supports = ZIP_SOURCE_SUPPORTS_SEEKABLE;
	}
	zs = zip_source_function_create(read_file, ctx, error);
	if(!zs) {
		SAlloc::F(ctx->fname);
		SAlloc::F(ctx);
	}
	return zs;
}

#ifndef O_BINARY
	#define O_BINARY 0
#endif

static int _zip_mkstemp(char * path)
{
#ifdef _WIN32
	int ret = _creat(_mktemp(path), _S_IREAD|_S_IWRITE);
	return (ret == -1) ? 0 : ret;
#else
	int fd;
	char * start, * trv;
	struct stat sbuf;
	// To guarantee multiple calls generate unique names even if
	// the file is not created. 676 different possibilities with 7 or more X's, 26 with 6 or less.
	static char xtra[2] = "aa";
	int xcnt = 0;
	pid_t pid = getpid();
	// Move to end of path and count trailing X's
	for(trv = path; *trv; ++trv)
		if(*trv == 'X')
			xcnt++;
		else
			xcnt = 0;
	// Use at least one from xtra.  Use 2 if more than 6 X's
	if(*(trv - 1) == 'X')
		*--trv = xtra[0];
	if(xcnt > 6 && *(trv - 1) == 'X')
		*--trv = xtra[1];
	// Set remaining X's to pid digits with 0's to the left
	while(*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}
	// update xtra for next call
	if(xtra[0] != 'z')
		xtra[0]++;
	else {
		xtra[0] = 'a';
		if(xtra[1] != 'z')
			xtra[1]++;
		else
			xtra[1] = 'a';
	}
	// 
	// check the target directory; if you have six X's and it
	// doesn't exist this runs for a *very* long time.
	// 
	for(start = trv + 1;; --trv) {
		if(trv <= path)
			break;
		if(*trv == '/') {
			*trv = '\0';
			if(stat(path, &sbuf))
				return 0;
			if(!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return 0;
			}
			*trv = '/';
			break;
		}
	}
	for(;; ) {
		if((fd = open(path, O_CREAT|O_EXCL|O_RDWR|O_BINARY, 0600)) >= 0)
			return (fd);
		if(errno != EEXIST)
			return 0;
		// tricky little algorithm for backward compatibility 
		for(trv = start;; ) {
			if(!*trv)
				return 0;
			if(*trv == 'z')
				*trv++ = 'a';
			else {
				if(isdigit((uchar)*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	//NOTREACHED
#endif
}

static int create_temp_output(ZipReadFileBlock * ctx)
{
	int tfd;
	mode_t mask;
	FILE * tfp;
	char * temp = static_cast<char *>(SAlloc::M(strlen(ctx->fname)+8));
	if(!temp)
		return zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
	sprintf(temp, "%s.XXXXXX", ctx->fname);
	mask = _umask(_SAFE_MASK);
	if((tfd = _zip_mkstemp(temp)) == -1) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		_umask(mask);
		SAlloc::F(temp);
		return -1;
	}
	_umask(mask);
	if((tfp = _fdopen(tfd, "r+b")) == NULL) {
		zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, errno);
		_close(tfd);
		remove(temp);
		SAlloc::F(temp);
		return -1;
	}
#ifdef _WIN32
	// According to Pierre Joye, Windows in some environments per
	// default creates text files, so force binary mode.
	_setmode(_fileno(tfp), _O_BINARY);
#endif
	ctx->fout = tfp;
	ctx->tmpname = temp;
	return 0;
}

static int64 read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd)
{
	uint64 n;
	size_t i;
	ZipReadFileBlock * ctx = (ZipReadFileBlock*)state;
	char * buf = (char *)data;
	switch(cmd) {
		case ZIP_SOURCE_BEGIN_WRITE:
		    if(ctx->fname == NULL)
			    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
			else
				return create_temp_output(ctx);
		case ZIP_SOURCE_COMMIT_WRITE: {
		    mode_t mask;
		    if(fclose(ctx->fout) < 0) {
			    ctx->fout = NULL;
			    zip_error_set(&ctx->error, SLERR_ZIP_WRITE, errno);
		    }
		    ctx->fout = NULL;
		    if(rename(ctx->tmpname, ctx->fname) < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_RENAME, errno);
			else {
				mask = _umask(022);
				_umask(mask);
				// not much we can do if chmod fails except make the whole commit fail 
				_chmod(ctx->fname, 0666&~mask);
				ZFREE(ctx->tmpname);
				return 0;
			}
	    }
		case ZIP_SOURCE_CLOSE:
		    if(ctx->fname)
				SFile::ZClose(&ctx->f);
		    return 0;
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx->fname);
		    SAlloc::F(ctx->tmpname);
			SFile::ZClose(&ctx->f);
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_OPEN:
		    if(ctx->fname) {
			    if((ctx->f = fopen(ctx->fname, "rb")) == NULL)
				    return zip_error_set(&ctx->error, SLERR_ZIP_OPEN, errno);
		    }
		    if(ctx->start > 0) {
			    if(_zip_fseek_u(ctx->f, ctx->start, SEEK_SET, &ctx->error) < 0) {
				    return -1;
			    }
		    }
		    ctx->current = ctx->start;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(ctx->end > 0) {
			    n = ctx->end-ctx->current;
				SETMIN(n, len);
		    }
		    else {
			    n = len;
		    }
		    SETMIN(n, SIZE_MAX);
		    if((i = fread(buf, 1, (size_t)n, ctx->f)) == 0) {
			    if(ferror(ctx->f))
				    return zip_error_set(&ctx->error, SLERR_ZIP_READ, errno);
		    }
		    ctx->current += i;
		    return (int64)i;
		case ZIP_SOURCE_REMOVE:
		    if(remove(ctx->fname) < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_REMOVE, errno);
			else
				return 0;
		case ZIP_SOURCE_ROLLBACK_WRITE:
			SFile::ZClose(&ctx->fout);
		    remove(ctx->tmpname);
		    ZFREE(ctx->tmpname);
		    return 0;
		case ZIP_SOURCE_SEEK: {
		    int64 new_current;
		    int need_seek;
		    zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
		    if(args == NULL)
			    return -1;
		    need_seek = 1;
		    switch(args->whence) {
			    case SEEK_SET: new_current = args->offset; break;
			    case SEEK_END:
					if(ctx->end == 0) {
						if(_zip_fseek(ctx->f, args->offset, SEEK_END, &ctx->error) < 0)
							return -1;
						else if((new_current = ftello(ctx->f)) < 0)
							return zip_error_set(&ctx->error, SLERR_ZIP_SEEK, errno);
						else
							need_seek = 0;
					}
					else {
						new_current = (int64)ctx->end + args->offset;
					}
					break;
			    case SEEK_CUR:
					new_current = (int64)ctx->current + args->offset;
					break;
			    default:
					zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
					return -1;
		    }
		    if(new_current < 0 || (uint64)new_current < ctx->start || (ctx->end != 0 && (uint64)new_current > ctx->end)) {
			    zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			    return -1;
		    }
		    ctx->current = (uint64)new_current;
		    if(need_seek) {
			    if(_zip_fseek_u(ctx->f, ctx->current, SEEK_SET, &ctx->error) < 0) {
				    return -1;
			    }
		    }
		    return 0;
	    }
		case ZIP_SOURCE_SEEK_WRITE: {
		    zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
		    if(args == NULL)
			    return -1;
		    else if(_zip_fseek(ctx->fout, args->offset, args->whence, &ctx->error) < 0)
			    return -1;
			else
				return 0;
	    }
		case ZIP_SOURCE_STAT: {
		    if(len < sizeof(ctx->st))
			    return -1;
			else {
				if(ctx->st.valid != 0)
					memcpy(data, &ctx->st, sizeof(ctx->st));
				else {
					zip_stat_t * st;
					struct stat fst;
					int    err = ctx->f ? fstat(_fileno(ctx->f), &fst) : stat(ctx->fname, &fst);
					if(err != 0)
						return zip_error_set(&ctx->error, SLERR_ZIP_READ, errno);
					else {
						st = (zip_stat_t*)data;
						zip_stat_init(st);
						st->mtime = fst.st_mtime;
						st->valid |= ZIP_STAT_MTIME;
						if(ctx->end != 0) {
							st->size = ctx->end - ctx->start;
							st->valid |= ZIP_STAT_SIZE;
						}
						else if((fst.st_mode&S_IFMT) == S_IFREG) {
							st->size = (uint64)fst.st_size;
							st->valid |= ZIP_STAT_SIZE;
						}
					}
				}
				return sizeof(ctx->st);
			}
	    }
		case ZIP_SOURCE_SUPPORTS:
		    return ctx->supports;
		case ZIP_SOURCE_TELL:
		    return (int64)ctx->current;
		case ZIP_SOURCE_TELL_WRITE:
	    {
		    off_t ret = ftello(ctx->fout);
		    if(ret < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_TELL, errno);
			else
				return ret;
	    }
		case ZIP_SOURCE_WRITE:
	    {
		    size_t ret;
		    clearerr(ctx->fout);
		    ret = fwrite(data, 1, (size_t)len, ctx->fout);
		    if(ret != len || ferror(ctx->fout))
			    return zip_error_set(&ctx->error, SLERR_ZIP_WRITE, errno);
			else
				return (int64)ret;
	    }
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}

static int _zip_fseek_u(FILE * f, uint64 offset, int whence, zip_error_t * error)
{
	if(offset > ZIP_INT64_MAX)
		return zip_error_set(error, SLERR_ZIP_SEEK, EOVERFLOW);
	else
		return _zip_fseek(f, (int64)offset, whence, error);
}

static int _zip_fseek(FILE * f, int64 offset, int whence, zip_error_t * error)
{
	int    result = 0;
	if(offset > ZIP_FSEEK_MAX || offset < ZIP_FSEEK_MIN)
		result = zip_error_set(error, SLERR_ZIP_SEEK, EOVERFLOW);
	else if(fseeko(f, (off_t)offset, whence) < 0)
		result = zip_error_set(error, SLERR_ZIP_SEEK, errno);
	return result;
}
