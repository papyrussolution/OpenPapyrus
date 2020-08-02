// SZIP_WIN32.C
// Copyright (C) 1999-2016 Dieter Baron and Thomas Klausner
// 
// This file is part of libzip, a library to manipulate ZIP archives.
// The authors can be contacted at <libzip@nih.at>
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the distribution.
// 3. The names of the authors may not be used to endorse or promote
// products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#include "zipint.h"

// 0x0501 => Windows XP; needs to be at least this value because of GetFileSizeEx 
// @sobolev #define _WIN32_WINNT 0x0501
// @sobolev #include <windows.h>
//
// context for Win32 source 
//
struct _zip_source_win32_file_ops;

struct _zip_source_win32_read_file {
    zip_error_t error;      /* last error information */
    int64 supports;
	struct _zip_source_win32_file_ops * ops; // operations 
    /* reading */
    void * fname;   /* name of file to read from - ANSI (char *) or Unicode (wchar_t *) */
    void * h;       /* HANDLE for file to read from */
    int    closep;  /* whether to close f on ZIP_CMD_FREE */
    zip_stat_t st;  /* stat information passed in */
    uint64 start;   /* start offset of data to read */
    uint64 end;     /* end offset of data to read, 0 for up to EOF */
    uint64 current; /* current offset */
    /* writing */
    void * tmpname; /* name of temp file - ANSI (char *) or Unicode (wchar_t *) */
    void * hout;    /* HANDLE for output file */
};

typedef struct _zip_source_win32_read_file _zip_source_win32_read_file_t;

/* internal operations for Win32 source */

struct _zip_source_win32_file_ops {
    void *(*op_strdup)(const void *);
    void *(*op_open)(_zip_source_win32_read_file_t *);
    void *(*op_create_temp)(_zip_source_win32_read_file_t *, void **, uint32, PSECURITY_ATTRIBUTES);
    int (*op_rename_temp)(_zip_source_win32_read_file_t *);
    int (*op_remove)(const void *);
};

typedef struct _zip_source_win32_file_ops _zip_source_win32_file_ops_t;

zip_source_t *_zip_source_win32_handle_or_name(const void *, void *, uint64, int64, int, const zip_stat_t *, _zip_source_win32_file_ops_t *, zip_error_t *);
//
//
//
static void * _win32_strdup_a(const void * str);
static HANDLE _win32_open_a(_zip_source_win32_read_file_t * ctx);
static HANDLE _win32_create_temp_a(_zip_source_win32_read_file_t * ctx, void ** temp, uint32 value, PSECURITY_ATTRIBUTES sa);
static int _win32_rename_temp_a(_zip_source_win32_read_file_t * ctx);
static int _win32_remove_a(const void * fname);
static int64 _win32_read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd);
static int _win32_create_temp_file(_zip_source_win32_read_file_t * ctx);
//static int _zip_filetime_to_time_t(FILETIME ft, time_t * t);
//static int _zip_seek_win32_u(void * h, uint64 offset, int whence, zip_error_t * error);
//static int _zip_seek_win32(void * h, int64 offset, int whence, zip_error_t * error);
//static int _zip_win32_error_to_errno(ulong win32err);
//static int _zip_stat_win32(void * h, zip_stat_t * st, _zip_source_win32_read_file_t * ctx);
static void * _win32_strdup_w(const void * str);
static HANDLE _win32_open_w(_zip_source_win32_read_file_t * ctx);
static HANDLE _win32_create_temp_w(_zip_source_win32_read_file_t * ctx, void ** temp, uint32 value, PSECURITY_ATTRIBUTES sa);
static int _win32_rename_temp_w(_zip_source_win32_read_file_t * ctx);
static int _win32_remove_w(const void * fname);

static _zip_source_win32_file_ops_t win32_ops_a = { _win32_strdup_a, _win32_open_a, _win32_create_temp_a, _win32_rename_temp_a, _win32_remove_a };

static int FASTCALL _zip_win32_error_to_errno(DWORD win32err)
{
	// Note: This list isn't exhaustive, but should cover common cases.
	switch(win32err) {
		case ERROR_INVALID_PARAMETER: return EINVAL;
		case ERROR_FILE_NOT_FOUND: return ENOENT;
		case ERROR_INVALID_HANDLE: return EBADF;
		case ERROR_ACCESS_DENIED: return EACCES;
		case ERROR_FILE_EXISTS: return EEXIST;
		case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
		case ERROR_DISK_FULL: return ENOSPC;
		default: return 0;
	}
}

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
	else
		return _zip_source_win32_handle_or_name(fname, INVALID_HANDLE_VALUE, start, length, 1, NULL, &win32_ops_a, error);
}

static void * _win32_strdup_a(const void * str)
{
	return sstrdup(static_cast<const char *>(str));
}

static HANDLE _win32_open_a(_zip_source_win32_read_file_t * ctx)
{
	return CreateFileA((LPCSTR)ctx->fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

static HANDLE _win32_create_temp_a(_zip_source_win32_read_file_t * ctx, void ** temp, uint32 value, PSECURITY_ATTRIBUTES sa)
{
	size_t len = strlen(static_cast<const char *>(ctx->fname)) + 10;
	if(*temp == NULL) {
		if((*temp = SAlloc::M(sizeof(char) * len)) == NULL) {
			zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
			return INVALID_HANDLE_VALUE;
		}
	}
	if(sprintf(static_cast<char *>(*temp), "%s.%08x", static_cast<const char *>(ctx->fname), value) != len - 1) {
		return INVALID_HANDLE_VALUE;
	}
	return CreateFileA((const char *)*temp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, sa, CREATE_NEW,
	    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, 0);
}

static int _win32_rename_temp_a(_zip_source_win32_read_file_t * ctx)
{
	return MoveFileExA((LPCSTR)ctx->tmpname, (LPCSTR)ctx->fname, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

static int _win32_remove_a(const void * fname)
{
	DeleteFileA(static_cast<const char *>(fname));
	return 0;
}

static int _zip_seek_win32(HANDLE h, int64 offset, int whence, zip_error_t * error)
{
	LARGE_INTEGER li;
	DWORD method;
	switch(whence) {
		case SEEK_SET: method = FILE_BEGIN; break;
		case SEEK_END: method = FILE_END; break;
		case SEEK_CUR: method = FILE_CURRENT; break;
		default: return zip_error_set(error, SLERR_ZIP_SEEK, EINVAL);
	}
	li.QuadPart = (LONGLONG)offset;
	if(!SetFilePointerEx(h, li, NULL, method))
		return zip_error_set(error, SLERR_ZIP_SEEK, _zip_win32_error_to_errno(GetLastError()));
	else
		return 0;
}

static int _zip_seek_win32_u(HANDLE h, uint64 offset, int whence, zip_error_t * error)
{
	if(offset > ZIP_INT64_MAX)
		return zip_error_set(error, SLERR_ZIP_SEEK, EOVERFLOW);
	else
		return _zip_seek_win32(h, (int64)offset, whence, error);
}

static int _zip_filetime_to_time_t(FILETIME ft, time_t * t)
{
	// Inspired by http://stackoverflow.com/questions/6161776/convert-windows-filetime-to-second-in-unix-linux
	const int64 WINDOWS_TICK = 10000000LL;
	const int64 SEC_TO_UNIX_EPOCH = 11644473600LL;
	ULARGE_INTEGER li;
	int64 secs;
	time_t temp;
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	secs = (li.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
	temp = (time_t)secs;
	if(secs != (int64)temp)
		return -1;
	else {
		*t = temp;
		return 0;
	}
}

static int _zip_stat_win32(HANDLE h, zip_stat_t * st, _zip_source_win32_read_file_t * ctx)
{
	FILETIME mtimeft;
	time_t mtime;
	LARGE_INTEGER size;
	int regularp;
	if(!GetFileTime(h, NULL, NULL, &mtimeft))
		return zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
	if(_zip_filetime_to_time_t(mtimeft, &mtime) < 0)
		return zip_error_set(&ctx->error, SLERR_ZIP_READ, ERANGE);
	regularp = 0;
	if(GetFileType(h) == FILE_TYPE_DISK) {
		regularp = 1;
	}
	if(!GetFileSizeEx(h, &size))
		return zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
	zip_stat_init(st);
	st->mtime = mtime;
	st->valid |= ZIP_STAT_MTIME;
	if(ctx->end != 0) {
		st->size = ctx->end - ctx->start;
		st->valid |= ZIP_STAT_SIZE;
	}
	else if(regularp) {
		st->size = (uint64)size.QuadPart;
		st->valid |= ZIP_STAT_SIZE;
	}
	return 0;
}
//
//
//
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
		return _zip_source_win32_handle_or_name(NULL, h, start, length, 1, NULL, NULL, error);
}

zip_source_t * _zip_source_win32_handle_or_name(const void * fname, HANDLE h, uint64 start, int64 len, 
	int closep, const zip_stat_t * st, _zip_source_win32_file_ops_t * ops, zip_error_t * error)
{
	_zip_source_win32_read_file_t * ctx;
	zip_source_t * zs;
	if(h == INVALID_HANDLE_VALUE && fname == NULL) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	if((ctx = (_zip_source_win32_read_file_t*)SAlloc::M(sizeof(_zip_source_win32_read_file_t))) == NULL) {
		zip_error_set(error, SLERR_ZIP_MEMORY, 0);
		return NULL;
	}
	ctx->fname = NULL;
	if(fname) {
		if((ctx->fname = ops->op_strdup(fname)) == NULL) {
			zip_error_set(error, SLERR_ZIP_MEMORY, 0);
			SAlloc::F(ctx);
			return NULL;
		}
	}
	ctx->ops = ops;
	ctx->h = h;
	ctx->start = start;
	ctx->end = (len < 0 ? 0 : start + (uint64)len);
	ctx->closep = ctx->fname ? 1 : closep;
	if(st) {
		memcpy(&ctx->st, st, sizeof(ctx->st));
		ctx->st.name = NULL;
		ctx->st.valid &= ~ZIP_STAT_NAME;
	}
	else {
		zip_stat_init(&ctx->st);
	}
	ctx->tmpname = NULL;
	ctx->hout = INVALID_HANDLE_VALUE;
	zip_error_init(&ctx->error);
	ctx->supports = ZIP_SOURCE_SUPPORTS_READABLE | zip_source_make_command_bitmap(ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_TELL, -1);
	if(ctx->fname) {
		HANDLE th = ops->op_open(ctx);
		if(th == INVALID_HANDLE_VALUE || GetFileType(th) == FILE_TYPE_DISK) {
			ctx->supports = ZIP_SOURCE_SUPPORTS_WRITABLE;
		}
		if(th != INVALID_HANDLE_VALUE) {
			CloseHandle(th);
		}
	}
	else if(GetFileType(ctx->h) == FILE_TYPE_DISK) {
		ctx->supports = ZIP_SOURCE_SUPPORTS_SEEKABLE;
	}
	if((zs = zip_source_function_create(_win32_read_file, ctx, error)) == NULL) {
		SAlloc::F(ctx->fname);
		SAlloc::F(ctx);
		return NULL;
	}
	return zs;
}

static int64 _win32_read_file(void * state, void * data, uint64 len, zip_source_cmd_t cmd)
{
	uint64 n;
	DWORD i;
	_zip_source_win32_read_file_t * ctx = (_zip_source_win32_read_file_t*)state;
	char * buf = (char *)data;
	switch(cmd) {
		case ZIP_SOURCE_BEGIN_WRITE:
		    if(ctx->fname == NULL)
			    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
			else
				return _win32_create_temp_file(ctx);
		case ZIP_SOURCE_COMMIT_WRITE: {
		    if(!CloseHandle(ctx->hout)) {
			    ctx->hout = INVALID_HANDLE_VALUE;
			    zip_error_set(&ctx->error, SLERR_ZIP_WRITE, _zip_win32_error_to_errno(GetLastError()));
		    }
		    ctx->hout = INVALID_HANDLE_VALUE;
		    if(ctx->ops->op_rename_temp(ctx) < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_RENAME, _zip_win32_error_to_errno(GetLastError()));
			else {
				ZFREE(ctx->tmpname);
				return 0;
			}
	    }
		case ZIP_SOURCE_CLOSE:
		    if(ctx->fname) {
			    CloseHandle(ctx->h);
			    ctx->h = INVALID_HANDLE_VALUE;
		    }
		    return 0;
		case ZIP_SOURCE_ERROR:
		    return zip_error_to_data(&ctx->error, data, len);
		case ZIP_SOURCE_FREE:
		    SAlloc::F(ctx->fname);
		    SAlloc::F(ctx->tmpname);
		    if(ctx->closep && ctx->h != INVALID_HANDLE_VALUE)
			    CloseHandle(ctx->h);
		    SAlloc::F(ctx);
		    return 0;
		case ZIP_SOURCE_OPEN:
		    if(ctx->fname) {
			    if((ctx->h = ctx->ops->op_open(ctx)) == INVALID_HANDLE_VALUE)
				    return zip_error_set(&ctx->error, SLERR_ZIP_OPEN, _zip_win32_error_to_errno(GetLastError()));
		    }
		    if(ctx->closep && ctx->start > 0) {
			    if(_zip_seek_win32_u(ctx->h, ctx->start, SEEK_SET, &ctx->error) < 0) {
				    return -1;
			    }
		    }
		    ctx->current = ctx->start;
		    return 0;
		case ZIP_SOURCE_READ:
		    if(ctx->end > 0) {
			    n = ctx->end - ctx->current;
			    if(n > len) {
				    n = len;
			    }
		    }
		    else {
			    n = len;
		    }
		    if(n > SIZE_MAX)
			    n = SIZE_MAX;
		    if(!ctx->closep) {
			    if(_zip_seek_win32_u(ctx->h, ctx->current, SEEK_SET, &ctx->error) < 0) {
				    return -1;
			    }
		    }
		    if(!ReadFile(ctx->h, buf, (DWORD)n, &i, NULL))
			    return zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(GetLastError()));
			else {
				ctx->current += i;
				return (int64)i;
			}
		case ZIP_SOURCE_REMOVE:
		    if(ctx->ops->op_remove(ctx->fname) < 0)
			    return zip_error_set(&ctx->error, SLERR_ZIP_REMOVE, _zip_win32_error_to_errno(GetLastError()));
			else
				return 0;
		case ZIP_SOURCE_ROLLBACK_WRITE:
		    if(ctx->hout) {
			    CloseHandle(ctx->hout);
			    ctx->hout = INVALID_HANDLE_VALUE;
		    }
		    ctx->ops->op_remove(ctx->tmpname);
		    ZFREE(ctx->tmpname);
		    return 0;
		case ZIP_SOURCE_SEEK: {
		    int64 new_current;
		    int need_seek;
		    zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
		    if(args == NULL)
			    return -1;
		    need_seek = ctx->closep;
		    switch(args->whence) {
			    case SEEK_SET:
				new_current = args->offset;
				break;
			    case SEEK_END:
				if(ctx->end == 0) {
					LARGE_INTEGER zero;
					LARGE_INTEGER new_offset;
					if(_zip_seek_win32(ctx->h, args->offset, SEEK_END, &ctx->error) < 0) {
						return -1;
					}
					else {
						zero.QuadPart = 0;
						if(!SetFilePointerEx(ctx->h, zero, &new_offset, FILE_CURRENT))
							return zip_error_set(&ctx->error, SLERR_ZIP_SEEK, _zip_win32_error_to_errno(GetLastError()));
						else {
							new_current = new_offset.QuadPart;
							need_seek = 0;
						}
					}
				}
				else {
					new_current = (int64)ctx->end + args->offset;
				}
				break;
			    case SEEK_CUR:
					new_current = (int64)ctx->current + args->offset;
					break;
			    default:
					return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
		    }
		    if(new_current < 0 || (uint64)new_current < ctx->start || (ctx->end != 0 && (uint64)new_current > ctx->end))
			    return zip_error_set(&ctx->error, SLERR_ZIP_INVAL, 0);
			else {
				ctx->current = (uint64)new_current;
				if(need_seek) {
					if(_zip_seek_win32_u(ctx->h, ctx->current, SEEK_SET, &ctx->error) < 0) {
						return -1;
					}
				}
				return 0;
			}
	    }
		case ZIP_SOURCE_SEEK_WRITE: {
		    zip_source_args_seek_t * args = ZIP_SOURCE_GET_ARGS(zip_source_args_seek_t, data, len, &ctx->error);
		    if(args == NULL)
			    return -1;
		    else if(_zip_seek_win32(ctx->hout, args->offset, args->whence, &ctx->error) < 0)
			    return -1;
			else
				return 0;
	    }
		case ZIP_SOURCE_STAT: {
		    if(len < sizeof(ctx->st))
			    return -1;
		    if(ctx->st.valid != 0)
			    memcpy(data, &ctx->st, sizeof(ctx->st));
		    else {
			    DWORD  win32err;
			    HANDLE h;
			    int    success;
			    zip_stat_t * st = static_cast<zip_stat_t *>(data);
			    if(ctx->h != INVALID_HANDLE_VALUE) {
				    h = ctx->h;
			    }
			    else {
				    h = ctx->ops->op_open(ctx);
				    if(h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
					    return zip_error_set(&ctx->error, SLERR_ZIP_READ, ENOENT);
			    }
			    success = _zip_stat_win32(h, st, ctx);
			    win32err = GetLastError();
			    /* We're done with the handle, so close it if we just opened it. */
			    if(h != ctx->h) {
				    CloseHandle(h);
			    }
			    if(success < 0) {
				    // @todo Is this the correct error to return in all cases? 
				    return zip_error_set(&ctx->error, SLERR_ZIP_READ, _zip_win32_error_to_errno(win32err));
			    }
		    }
		    return sizeof(ctx->st);
	    }
		case ZIP_SOURCE_SUPPORTS: return ctx->supports;
		case ZIP_SOURCE_TELL:     return (int64)ctx->current;
		case ZIP_SOURCE_TELL_WRITE:
	    {
		    LARGE_INTEGER zero;
		    LARGE_INTEGER offset;
		    zero.QuadPart = 0;
		    if(!SetFilePointerEx(ctx->hout, zero, &offset, FILE_CURRENT))
			    return zip_error_set(&ctx->error, SLERR_ZIP_TELL, _zip_win32_error_to_errno(GetLastError()));
			else
				return offset.QuadPart;
	    }
		case ZIP_SOURCE_WRITE:
	    {
		    DWORD ret;
		    if(!WriteFile(ctx->hout, data, (DWORD)len, &ret, NULL) || ret != len)
			    return zip_error_set(&ctx->error, SLERR_ZIP_WRITE, _zip_win32_error_to_errno(GetLastError()));
			else
				return (int64)ret;
	    }
		default:
		    return zip_error_set(&ctx->error, SLERR_ZIP_OPNOTSUPP, 0);
	}
}

static int _win32_create_temp_file(_zip_source_win32_read_file_t * ctx)
{
	uint32 value;
	//
	// Windows has GetTempFileName(), but it closes the file after
	// creation, leaving it open to a horrible race condition. So we reinvent the wheel.
	//
	int i;
	HANDLE th = INVALID_HANDLE_VALUE;
	void * temp = NULL;
	SECURITY_INFORMATION si;
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR psd = NULL;
	PSECURITY_ATTRIBUTES psa = NULL;
	DWORD len;
	BOOL success;
	/*
	   Read the DACL from the original file, so we can copy it to the temp file.
	   If there is no original file, or if we can't read the DACL, we'll use the
	   default security descriptor.
	 */
	if(ctx->h != INVALID_HANDLE_VALUE && GetFileType(ctx->h) == FILE_TYPE_DISK) {
		si = DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION;
		len = 0;
		success = GetUserObjectSecurity(ctx->h, &si, NULL, len, &len);
		if(!success && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
			if((psd = (PSECURITY_DESCRIPTOR)SAlloc::M(len)) == NULL)
				return zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
			success = GetUserObjectSecurity(ctx->h, &si, psd, len, &len);
		}
		if(success) {
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.bInheritHandle = FALSE;
			sa.lpSecurityDescriptor = psd;
			psa = &sa;
		}
	}
	value = GetTickCount();
	for(i = 0; i < 1024 && th == INVALID_HANDLE_VALUE; i++) {
		th = ctx->ops->op_create_temp(ctx, &temp, value + i, psa);
		if(th == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS)
			break;
	}
	if(th == INVALID_HANDLE_VALUE) {
		SAlloc::F(temp);
		SAlloc::F(psd);
		return zip_error_set(&ctx->error, SLERR_ZIP_TMPOPEN, _zip_win32_error_to_errno(GetLastError()));
	}
	else {
		SAlloc::F(psd);
		ctx->hout = th;
		ctx->tmpname = temp;
		return 0;
	}
}
//
//
//
ZIP_EXTERN zip_source_t * zip_source_file(zip_t * za, const char * fname, uint64 start, int64 len)
{
	return za ? zip_source_file_create(fname, start, len, &za->error) : 0;
}

ZIP_EXTERN zip_source_t * zip_source_file_create(const char * fname, uint64 start, int64 length, zip_error_t * error)
{
	zip_source_t * source = 0;
	if(!fname || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
	}
	else {
		// Convert fname from UTF-8 to Windows-friendly UTF-16
		int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fname, -1, NULL, 0);
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
//
//
//
ZIP_EXTERN zip_source_t * zip_source_win32w(zip_t * za, const wchar_t * fname, uint64 start, int64 len)
{
	return za ? zip_source_win32w_create(fname, start, len, &za->error) : 0;
}

static void * _win32_strdup_w(const void * str)
{
	return _wcsdup(static_cast<const wchar_t *>(str));
}

static HANDLE _win32_open_w(_zip_source_win32_read_file_t * ctx)
{
	return CreateFileW((LPCWSTR)ctx->fname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

static HANDLE _win32_create_temp_w(_zip_source_win32_read_file_t * ctx, void ** temp, uint32 value, PSECURITY_ATTRIBUTES sa)
{
	int len = (int)wcslen((const wchar_t *)ctx->fname) + 10;
	if(*temp == NULL) {
		if((*temp = SAlloc::M(sizeof(wchar_t) * len)) == NULL) {
			zip_error_set(&ctx->error, SLERR_ZIP_MEMORY, 0);
			return INVALID_HANDLE_VALUE;
		}
	}
	if(_snwprintf((wchar_t *)*temp, len, L"%s.%08x", (const wchar_t *)ctx->fname, value) != len - 1) {
		return INVALID_HANDLE_VALUE;
	}
	return CreateFileW((const wchar_t *)*temp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
	    sa, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, 0);
}

static int _win32_rename_temp_w(_zip_source_win32_read_file_t * ctx)
{
	return MoveFileExW((LPCWSTR)ctx->tmpname, (LPCWSTR)ctx->fname, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

static int _win32_remove_w(const void * fname)
{
	DeleteFileW((const wchar_t *)fname);
	return 0;
}

ZIP_EXTERN zip_source_t * zip_source_win32w_create(const wchar_t * fname, uint64 start, int64 length, zip_error_t * error)
{
	static _zip_source_win32_file_ops_t win32_ops_w = {
		_win32_strdup_w,
		_win32_open_w,
		_win32_create_temp_w,
		_win32_rename_temp_w,
		_win32_remove_w
	};
	if(fname == NULL || length < -1) {
		zip_error_set(error, SLERR_ZIP_INVAL, 0);
		return NULL;
	}
	else
		return _zip_source_win32_handle_or_name(fname, INVALID_HANDLE_VALUE, start, length, 1, NULL, &win32_ops_w, error);
}