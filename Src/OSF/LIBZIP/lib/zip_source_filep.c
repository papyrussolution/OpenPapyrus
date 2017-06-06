/*
   zip_source_filep.c -- create data source from FILE *
   Copyright (C) 1999-2015 Dieter Baron and Thomas Klausner

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
#include "zipint.h"
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef _WIN32
	#include <fcntl.h> // WIN32 needs <fcntl.h> for _O_BINARY 
#endif
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

struct ZipReadFileBlock {
	zip_error_t error;  /* last error information */
	int64 supports;
	/* reading */
	char * fname;       /* name of file to read from */
	FILE * f;           /* file to read from */
	struct zip_stat st; /* stat information passed in */
	uint64 start; /* start offset of data to read */
	uint64 end;   /* end offset of data to read, 0 for up to EOF */
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
		zip_error_set(error, ZIP_ER_INVAL, 0);
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
		zip_error_set(error, ZIP_ER_INVAL, 0);
		return NULL;
	}
	if((ctx = (ZipReadFileBlock *)SAlloc::M(sizeof(ZipReadFileBlock))) == NULL) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}
	ctx->fname = NULL;
	if(fname) {
		ctx->fname = sstrdup(fname);
		if(!ctx->fname) {
			zip_error_set(error, ZIP_ER_MEMORY, 0);
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
	if((zs = zip_source_function_create(read_file, ctx, error)) == NULL) {
		SAlloc::F(ctx->fname);
		SAlloc::F(ctx);
		return NULL;
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
				return (0);
			if(!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return (0);
			}
			*trv = '/';
			break;
		}
	}
	for(;; ) {
		if((fd = open(path, O_CREAT|O_EXCL|O_RDWR|O_BINARY, 0600)) >= 0)
			return (fd);
		if(errno != EEXIST)
			return (0);
		// tricky little algorithm for backward compatibility 
		for(trv = start;; ) {
			if(!*trv)
				return (0);
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
	char * temp = (char*)SAlloc::M(strlen(ctx->fname)+8);
	if(!temp)
		return zip_error_set(&ctx->error, ZIP_ER_MEMORY, 0);
	sprintf(temp, "%s.XXXXXX", ctx->fname);
	mask = _umask(_SAFE_MASK);
	if((tfd = _zip_mkstemp(temp)) == -1) {
		zip_error_set(&ctx->error, ZIP_ER_TMPOPEN, errno);
		_umask(mask);
		SAlloc::F(temp);
		return -1;
	}
	_umask(mask);
	if((tfp = _fdopen(tfd, "r+b")) == NULL) {
		zip_error_set(&ctx->error, ZIP_ER_TMPOPEN, errno);
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
	char * buf = (char*)data;
	switch(cmd) {
		case ZIP_SOURCE_BEGIN_WRITE:
		    if(ctx->fname == NULL)
			    return zip_error_set(&ctx->error, ZIP_ER_OPNOTSUPP, 0);
			else
				return create_temp_output(ctx);
		case ZIP_SOURCE_COMMIT_WRITE: {
		    mode_t mask;
		    if(fclose(ctx->fout) < 0) {
			    ctx->fout = NULL;
			    zip_error_set(&ctx->error, ZIP_ER_WRITE, errno);
		    }
		    ctx->fout = NULL;
		    if(rename(ctx->tmpname, ctx->fname) < 0)
			    return zip_error_set(&ctx->error, ZIP_ER_RENAME, errno);
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
				    return zip_error_set(&ctx->error, ZIP_ER_OPEN, errno);
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
			    if(n > len) {
				    n = len;
			    }
		    }
		    else {
			    n = len;
		    }
		    SETMIN(n, SIZE_MAX);
		    if((i = fread(buf, 1, (size_t)n, ctx->f)) == 0) {
			    if(ferror(ctx->f))
				    return zip_error_set(&ctx->error, ZIP_ER_READ, errno);
		    }
		    ctx->current += i;
		    return (int64)i;
		case ZIP_SOURCE_REMOVE:
		    if(remove(ctx->fname) < 0)
			    return zip_error_set(&ctx->error, ZIP_ER_REMOVE, errno);
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
							return zip_error_set(&ctx->error, ZIP_ER_SEEK, errno);
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
					zip_error_set(&ctx->error, ZIP_ER_INVAL, 0);
					return -1;
		    }
		    if(new_current < 0 || (uint64)new_current < ctx->start || (ctx->end != 0 && (uint64)new_current > ctx->end)) {
			    zip_error_set(&ctx->error, ZIP_ER_INVAL, 0);
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
		    if(args == NULL) {
			    return -1;
		    }
		    if(_zip_fseek(ctx->fout, args->offset, args->whence, &ctx->error) < 0) {
			    return -1;
		    }
		    return 0;
	    }
		case ZIP_SOURCE_STAT: {
		    if(len < sizeof(ctx->st))
			    return -1;
		    if(ctx->st.valid != 0)
			    memcpy(data, &ctx->st, sizeof(ctx->st));
		    else {
			    zip_stat_t * st;
			    struct stat fst;
				int    err = ctx->f ? fstat(_fileno(ctx->f), &fst) : stat(ctx->fname, &fst);
			    if(err != 0)
				    return zip_error_set(&ctx->error, ZIP_ER_READ, errno);
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
		case ZIP_SOURCE_SUPPORTS:
		    return ctx->supports;
		case ZIP_SOURCE_TELL:
		    return (int64)ctx->current;
		case ZIP_SOURCE_TELL_WRITE:
	    {
		    off_t ret = ftello(ctx->fout);
		    if(ret < 0)
			    return zip_error_set(&ctx->error, ZIP_ER_TELL, errno);
			else
				return ret;
	    }
		case ZIP_SOURCE_WRITE:
	    {
		    size_t ret;
		    clearerr(ctx->fout);
		    ret = fwrite(data, 1, (size_t)len, ctx->fout);
		    if(ret != len || ferror(ctx->fout))
			    return zip_error_set(&ctx->error, ZIP_ER_WRITE, errno);
			else
				return (int64)ret;
	    }
		default:
		    return zip_error_set(&ctx->error, ZIP_ER_OPNOTSUPP, 0);
	}
}

static int _zip_fseek_u(FILE * f, uint64 offset, int whence, zip_error_t * error)
{
	if(offset > ZIP_INT64_MAX)
		return zip_error_set(error, ZIP_ER_SEEK, EOVERFLOW);
	else
		return _zip_fseek(f, (int64)offset, whence, error);
}

static int _zip_fseek(FILE * f, int64 offset, int whence, zip_error_t * error)
{
	int    result = 0;
	if(offset > ZIP_FSEEK_MAX || offset < ZIP_FSEEK_MIN) {
		result = zip_error_set(error, ZIP_ER_SEEK, EOVERFLOW);
	}
	else if(fseeko(f, (off_t)offset, whence) < 0) {
		result = zip_error_set(error, ZIP_ER_SEEK, errno);
	}
	return result;
}

