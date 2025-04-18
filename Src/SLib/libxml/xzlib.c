/**
 * xzlib.c: front end for the transparent suport of lzma compression
 *     at the I/O layer, based on an example file from lzma project
 *
 * See Copyright for the status of this software.
 *
 * Anders F Bjorklund <afb@users.sourceforge.net>
 */
#include <slib-internal.h>
#pragma hdrstop

#ifdef HAVE_LZMA_H
//#include "xzlib.h"
//
// values for xz_state how 
//
#define LOOK 0 // look for a gzip/lzma header
#define COPY 1 // copy input directly
#define GZIP 2 // decompress a gzip stream
#define LZMA 3 // decompress a lzma stream
//
// internal lzma file state data structure 
//
typedef struct {
	int    mode; /* see lzma modes above */
	int    fd; /* file descriptor */
	char * path; /* path or fd for error messages */
	uint64 pos; /* current position in uncompressed data */
	uint   size; /* buffer size, zero if not allocated yet */
	uint   want; /* requested buffer size, default is BUFSIZ */
	uchar * in; /* input buffer */
	uchar * out; /* output buffer (double-sized when reading) */
	uchar * next; /* next output data to deliver or write */
	uint   have; /* amount of output data unused at next */
	int    eof; /* true if end of input file reached */
	uint64 start; /* where the lzma data started, for rewinding */
	uint64 raw; /* where the raw data started, for seeking */
	int    how; /* 0: get header, 1: copy, 2: decompress */
	int    direct; /* true if last read direct, false if lzma */
	/* seek request */
	uint64 skip; /* amount to skip (already rewound if backwards) */
	int    seek; /* true if seek request pending */
	/* error information */
	int    err; /* error code */
	char * msg; /* error message */
	/* lzma stream */
	int    init; /* is the iniflate stream initialized */
	lzma_stream strm; /* stream structure in-place (not a pointer) */
	char   padding1[32]; /* padding allowing to cope with possible extensions of above structure without too much side effect */
#ifdef HAVE_ZLIB_H
	/* zlib inflate or deflate stream */
	z_stream zstrm; /* stream structure in-place (not a pointer) */
#endif
	char   padding2[32]; /* padding allowing to cope with possible extensions of above structure without too much side effect */
} xz_state, * xz_statep;

static void xz_error(xz_statep state, int err, const char * msg)
{
	/* free previously allocated message and clear */
	if(state->msg) {
		if(state->err != LZMA_MEM_ERROR)
			SAlloc::F(state->msg);
		state->msg = NULL;
	}
	/* set error code, and if no message, then done */
	state->err = err;
	if(msg) {
		/* for an out of memory error, save as static string */
		if(err == LZMA_MEM_ERROR) {
			state->msg = (char *)msg;
		}
		else {
			/* construct error message with path */
			state->msg = static_cast<char *>(SAlloc::M(sstrlen(state->path) + sstrlen(msg) + 3));
			if(state->msg == NULL) {
				state->err = LZMA_MEM_ERROR;
				state->msg = const_cast<char *>(SlTxtOutOfMem); // @badcast
			}
			else {
				strcpy(state->msg, state->path);
				strcat(state->msg, ": ");
				strcat(state->msg, msg);
			}
		}
	}
}

static void xz_reset(xz_statep state)
{
	state->have = 0; /* no output data available */
	state->eof = 0; /* not at end of file */
	state->how = LOOK; /* look for gzip header */
	state->direct = 1; /* default for empty file */
	state->seek = 0; /* no seek request pending */
	xz_error(state, LZMA_OK, 0); /* clear error */
	state->pos = 0; /* no uncompressed data yet */
	state->strm.avail_in = 0; /* no input data yet */
#ifdef HAVE_ZLIB_H
	state->zstrm.avail_in = 0; /* no input data yet */
#endif
}

static xzFile xz_open(const char * path, int fd, const char * mode ATTRIBUTE_UNUSED)
{
	// allocate xzFile structure to return
	xz_statep state = static_cast<xz_statep>(SAlloc::M(sizeof(xz_state)));
	if(!state)
		return NULL;
	state->size = 0; // no buffers allocated yet
	state->want = BUFSIZ; // requested buffer size
	state->msg = NULL; // no error message yet
	state->init = 0; // initialization of zlib data
	// save the path name for error messages
	state->path = static_cast<char *>(SAlloc::M(sstrlen(path) + 1));
	if(isempty(state->path)) {
		SAlloc::F(state);
		return NULL;
	}
	else {
		strcpy(state->path, path);
		// open the file with the appropriate mode (or just use fd)
		SString _file_name(path);
		SStringU _file_name_u;
		_file_name.CopyToUnicode(_file_name_u);
		int open_flags = O_RDONLY;
	#ifdef O_LARGEFILE
			open_flags |= O_LARGEFILE;
	#endif
	#ifdef O_BINARY
			open_flags |= O_BINARY;
	#endif
		state->fd = (fd != -1) ? fd : _wopen(_file_name_u, open_flags, 0666); // @v11.9.12 open-->_wopen
		if(state->fd == -1) {
			SAlloc::F(state->path);
			SAlloc::F(state);
			return NULL;
		}
		// save the current position for rewinding (only if reading)
		state->start = lseek(state->fd, 0, SEEK_CUR);
		if(state->start == (uint64_t)-1)
			state->start = 0;
		xz_reset(state); // initialize stream
		return (xzFile)state; // return stream
	}
}

static int xz_compressed(xzFile f) 
{
	if(f) {
		xz_statep state = (xz_statep)f;
		if(state->init <= 0)
			return -1;
		switch(state->how) {
			case COPY:
				return 0;
			case GZIP:
			case LZMA:
				return 1;
		}
	}
	return -1;
}

xzFile __libxml2_xzopen(const char * path, const char * mode) { return xz_open(path, -1, mode); }
int __libxml2_xzcompressed(xzFile f) { return xz_compressed(f); }

xzFile __libxml2_xzdopen(int fd, const char * mode)
{
	char * path; /* identifier for error messages */
	xzFile xz;
	if(fd == -1 || (path = static_cast<char *>(SAlloc::M(7 + 3 * sizeof(int)))) == NULL)
		return NULL;
	sprintf(path, "<fd:%d>", fd); /* for debugging */
	xz = xz_open(path, fd, mode);
	SAlloc::F(path);
	return xz;
}

static int xz_load(xz_statep state, uchar * buf, uint len, uint * have)
{
	int ret;
	*have = 0;
	do {
		ret = read(state->fd, buf + *have, len - *have);
		if(ret <= 0)
			break;
		*have += ret;
	} while(*have < len);
	if(ret < 0) {
		xz_error(state, -1, strerror(errno));
		return -1;
	}
	if(!ret)
		state->eof = 1;
	return 0;
}

static int xz_avail(xz_statep state)
{
	lzma_stream * strm = &(state->strm);
	if(state->err != LZMA_OK)
		return -1;
	if(state->eof == 0) {
		// avail_in is size_t, which is not necessary sizeof(uint) 
		uint tmp = strm->avail_in;
		if(xz_load(state, state->in, state->size, &tmp) == -1) {
			strm->avail_in = tmp;
			return -1;
		}
		strm->avail_in = tmp;
		strm->next_in = state->in;
	}
	return 0;
}

#ifdef HAVE_ZLIB_H
static int xz_avail_zstrm(xz_statep state)
{
	state->strm.avail_in = state->zstrm.avail_in;
	state->strm.next_in = state->zstrm.next_in;
	{
		int ret = xz_avail(state);
		state->zstrm.avail_in = (uInt)state->strm.avail_in;
		state->zstrm.next_in = const_cast<Byte *>(state->strm.next_in); // @badcast
		return ret;
	}
}
#endif

static int is_format_xz(xz_statep state)
{
	lzma_stream * strm = &(state->strm);
	return strm->avail_in >= 6 && memcmp(state->in, "\3757zXZ", 6) == 0;
}

static int is_format_lzma(xz_statep state)
{
	lzma_stream * strm = &(state->strm);
	lzma_filter filter;
	lzma_options_lzma * opt;
	uint32_t dict_size;
	uint64_t uncompressed_size;
	size_t i;
	if(strm->avail_in < 13)
		return 0;
	filter.id = LZMA_FILTER_LZMA1;
	if(lzma_properties_decode(&filter, NULL, state->in, 5) != LZMA_OK)
		return 0;
	opt = static_cast<lzma_options_lzma *>(filter.options);
	dict_size = opt->dict_size;
	SAlloc::F(opt); /* we can't use free on a string returned by zlib */
	/* A hack to ditch tons of false positives: We allow only dictionary
	 * sizes that are 2^n or 2^n + 2^(n-1) or UINT32_MAX. LZMA_Alone
	 * created only files with 2^n, but accepts any dictionary size.
	 * If someone complains, this will be reconsidered.
	 */
	if(dict_size != UINT32_MAX) {
		uint32_t d = dict_size - 1;
		d |= d >> 2;
		d |= d >> 3;
		d |= d >> 4;
		d |= d >> 8;
		d |= d >> 16;
		++d;
		if(d != dict_size || dict_size == 0)
			return 0;
	}
	/* Another hack to ditch false positives: Assume that if the
	 * uncompressed size is known, it must be less than 256 GiB.
	 * Again, if someone complains, this will be reconsidered.
	 */
	uncompressed_size = 0;
	for(i = 0; i < 8; ++i)
		uncompressed_size |= (uint64_t)(state->in[5 + i]) << (i * 8);
	if(uncompressed_size != UINT64_MAX && uncompressed_size > (UINT64_C(1) << 38))
		return 0;
	return 1;
}

#ifdef HAVE_ZLIB_H

// Get next byte from input, or -1 if end or error
#define NEXT() ((strm->avail_in == 0 && xz_avail(state) == -1) ? -1 : (strm->avail_in == 0 ? -1 :	(strm->avail_in--, *(strm->next_in)++)))
// Same thing, but from zstrm
#define NEXTZ() ((strm->avail_in == 0 && xz_avail_zstrm(state) == -1) ? -1 : (strm->avail_in == 0 ? -1 :	(strm->avail_in--, *(strm->next_in)++)))

/* Get a four-byte little-endian integer and return 0 on success and the value
   in *ret.  Otherwise -1 is returned and *ret is not modified. */
static int gz_next4(xz_statep state, unsigned long * ret)
{
	int ch;
	ulong val;
	z_streamp strm = &(state->zstrm);
	val = NEXTZ();
	val += (uint)NEXTZ() << 8;
	val += (ulong)NEXTZ() << 16;
	ch = NEXTZ();
	if(ch == -1)
		return -1;
	val += static_cast<ulong>(ch) << 24;
	*ret = val;
	return 0;
}

#endif

static int xz_head(xz_statep state)
{
	lzma_stream * strm = &(state->strm);
	lzma_stream init = LZMA_STREAM_INIT;
	int flags;
	unsigned len;
	/* allocate read buffers and inflate memory */
	if(state->size == 0) {
		/* allocate buffers */
		state->in = static_cast<uchar *>(SAlloc::M(state->want));
		state->out = static_cast<uchar *>(SAlloc::M(state->want << 1));
		if(state->in == NULL || state->out == NULL) {
			SAlloc::F(state->out);
			SAlloc::F(state->in);
			xz_error(state, LZMA_MEM_ERROR, SlTxtOutOfMem);
			return -1;
		}
		state->size = state->want;
		/* allocate decoder memory */
		state->strm = init;
		state->strm.avail_in = 0;
		state->strm.next_in = NULL;
		if(lzma_auto_decoder(&state->strm, UINT64_MAX, 0) != LZMA_OK) {
			SAlloc::F(state->out);
			SAlloc::F(state->in);
			state->size = 0;
			xz_error(state, LZMA_MEM_ERROR, SlTxtOutOfMem);
			return -1;
		}
#ifdef HAVE_ZLIB_H
		/* allocate inflate memory */
		state->zstrm.zalloc = Z_NULL;
		state->zstrm.zfree = Z_NULL;
		state->zstrm.opaque = Z_NULL;
		state->zstrm.avail_in = 0;
		state->zstrm.next_in = Z_NULL;
		if(state->init == 0) {
			if(inflateInit2(&(state->zstrm), -15) != Z_OK) { /* raw inflate */
				SAlloc::F(state->out);
				SAlloc::F(state->in);
				state->size = 0;
				xz_error(state, LZMA_MEM_ERROR, SlTxtOutOfMem);
				return -1;
			}
			state->init = 1;
		}
#endif
	}
	/* get some data in the input buffer */
	if(strm->avail_in == 0) {
		if(xz_avail(state) == -1)
			return -1;
		if(strm->avail_in == 0)
			return 0;
	}
	/* look for the xz magic header bytes */
	if(is_format_xz(state) || is_format_lzma(state)) {
		state->how = LZMA;
		state->direct = 0;
		return 0;
	}
#ifdef HAVE_ZLIB_H
	/* look for the gzip magic header bytes 31 and 139 */
	if(strm->next_in[0] == 31) {
		strm->avail_in--;
		strm->next_in++;
		if(strm->avail_in == 0 && xz_avail(state) == -1)
			return -1;
		if(strm->avail_in && strm->next_in[0] == 139) {
			/* we have a gzip header, woo hoo! */
			strm->avail_in--;
			strm->next_in++;

			/* skip rest of header */
			if(NEXT() != 8) { /* compression method */
				xz_error(state, LZMA_DATA_ERROR,
				    "unknown compression method");
				return -1;
			}
			flags = NEXT();
			if(flags & 0xe0) { /* reserved flag bits */
				xz_error(state, LZMA_DATA_ERROR,
				    "unknown header flags set");
				return -1;
			}
			NEXT(); /* modification time */
			NEXT();
			NEXT();
			NEXT();
			NEXT(); /* extra flags */
			NEXT(); /* operating system */
			if(flags & 4) { /* extra field */
				len = (uint)NEXT();
				len += (uint)NEXT() << 8;
				while(len--)
					if(NEXT() < 0)
						break;
			}
			if(flags & 8) /* file name */
				while(NEXT() > 0) ;
			if(flags & 16) /* comment */
				while(NEXT() > 0) ;
			if(flags & 2) { /* header crc */
				NEXT();
				NEXT();
			}
			/* an unexpected end of file is not checked for here -- it will be
			 * noticed on the first request for uncompressed data */

			/* set up for decompression */
			inflateReset(&state->zstrm);
			state->zstrm.adler = crc32(0L, Z_NULL, 0);
			state->how = GZIP;
			state->direct = 0;
			return 0;
		}
		else {
			/* not a gzip file -- save first byte (31) and fall to raw i/o */
			state->out[0] = 31;
			state->have = 1;
		}
	}
#endif

	/* doing raw i/o, save start of raw data for seeking, copy any leftover
	 * input to output -- this assumes that the output buffer is larger than
	 * the input buffer, which also assures space for gzungetc() */
	state->raw = state->pos;
	state->next = state->out;
	if(strm->avail_in) {
		memcpy(state->next + state->have, strm->next_in, strm->avail_in);
		state->have += strm->avail_in;
		strm->avail_in = 0;
	}
	state->how = COPY;
	state->direct = 1;
	return 0;
}

static int xz_decomp(xz_statep state)
{
	int ret;
	unsigned had;
	unsigned long crc, len;
	lzma_stream * strm = &(state->strm);

	lzma_action action = LZMA_RUN;

	/* fill output buffer up to end of deflate stream */
	had = strm->avail_out;
	do {
		/* get more input for inflate() */
		if(strm->avail_in == 0 && xz_avail(state) == -1)
			return -1;
		if(strm->avail_in == 0) {
			xz_error(state, LZMA_DATA_ERROR, "unexpected end of file");
			return -1;
		}
		if(state->eof)
			action = LZMA_FINISH;

		/* decompress and handle errors */
#ifdef HAVE_ZLIB_H
		if(state->how == GZIP) {
			state->zstrm.avail_in = (uInt)state->strm.avail_in;
			state->zstrm.next_in = (Byte *)state->strm.next_in;
			state->zstrm.avail_out = (uInt)state->strm.avail_out;
			state->zstrm.next_out = (Byte *)state->strm.next_out;
			ret = inflate(&state->zstrm, Z_NO_FLUSH);
			if(ret == Z_STREAM_ERROR || ret == Z_NEED_DICT) {
				xz_error(state, Z_STREAM_ERROR, "internal error: inflate stream corrupt");
				return -1;
			}
			if(ret == Z_MEM_ERROR)
				ret = LZMA_MEM_ERROR;
			if(ret == Z_DATA_ERROR)
				ret = LZMA_DATA_ERROR;
			if(ret == Z_STREAM_END)
				ret = LZMA_STREAM_END;
			state->strm.avail_in = state->zstrm.avail_in;
			state->strm.next_in = state->zstrm.next_in;
			state->strm.avail_out = state->zstrm.avail_out;
			state->strm.next_out = state->zstrm.next_out;
		}
		else            /* state->how == LZMA */
#endif
		ret = lzma_code(strm, action);
		if(ret == LZMA_MEM_ERROR) {
			xz_error(state, LZMA_MEM_ERROR, SlTxtOutOfMem);
			return -1;
		}
		if(ret == LZMA_DATA_ERROR) {
			xz_error(state, LZMA_DATA_ERROR, "compressed data error");
			return -1;
		}
	} while(strm->avail_out && ret != LZMA_STREAM_END);

	/* update available output and crc check value */
	state->have = had - strm->avail_out;
	state->next = strm->next_out - state->have;
#ifdef HAVE_ZLIB_H
	state->zstrm.adler =
	    crc32(state->zstrm.adler, state->next, state->have);
#endif

	if(ret == LZMA_STREAM_END) {
#ifdef HAVE_ZLIB_H
		if(state->how == GZIP) {
			if(gz_next4(state, &crc) == -1 || gz_next4(state, &len) == -1) {
				xz_error(state, LZMA_DATA_ERROR, "unexpected end of file");
				return -1;
			}
			if(crc != state->zstrm.adler) {
				xz_error(state, LZMA_DATA_ERROR, "incorrect data check");
				return -1;
			}
			if(len != (state->zstrm.total_out & 0xffffffffL)) {
				xz_error(state, LZMA_DATA_ERROR, "incorrect length check");
				return -1;
			}
			state->strm.avail_in = 0;
			state->strm.next_in = NULL;
			state->strm.avail_out = 0;
			state->strm.next_out = NULL;
		}
		else
#endif
		if(strm->avail_in != 0 || !state->eof) {
			xz_error(state, LZMA_DATA_ERROR, "trailing garbage");
			return -1;
		}
		state->how = LOOK; /* ready for next stream, once have is 0 (leave
		    * state->direct unchanged to remember how) */
	}

	/* good decompression */
	return 0;
}

static int xz_make(xz_statep state)
{
	lzma_stream * strm = &(state->strm);
	if(state->how == LOOK) { /* look for lzma / gzip header */
		if(xz_head(state) == -1)
			return -1;
		if(state->have) /* got some data from xz_head() */
			return 0;
	}
	if(state->how == COPY) { /* straight copy */
		if(xz_load(state, state->out, state->size << 1, &(state->have)) == -1)
			return -1;
		state->next = state->out;
	}
	else if(state->how == LZMA || state->how == GZIP) { /* decompress */
		strm->avail_out = state->size << 1;
		strm->next_out = state->out;
		if(xz_decomp(state) == -1)
			return -1;
	}
	return 0;
}

static int xz_skip(xz_statep state, uint64_t len)
{
	/* skip over len bytes or reach end-of-file, whichever comes first */
	while(len) {
		/* skip over whatever is in output buffer */
		if(state->have) {
			const unsigned n = ((uint64_t)state->have > len) ? (uint)len : state->have;
			state->have -= n;
			state->next += n;
			state->pos += n;
			len -= n;
		}
		/* output buffer empty -- return if we're at the end of the input */
		else if(state->eof && state->strm.avail_in == 0)
			break;
		/* need more data to skip -- load up output buffer */
		else {
			/* get more output, looking for header if required */
			if(xz_make(state) == -1)
				return -1;
		}
	}
	return 0;
}

int __libxml2_xzread(xzFile file, void * buf, unsigned len)
{
	unsigned got, n;
	xz_statep state;
	lzma_stream * strm;
	/* get internal structure */
	if(!file)
		return -1;
	state = (xz_statep)file;
	strm = &(state->strm);
	/* check that we're reading and that there's no error */
	if(state->err != LZMA_OK)
		return -1;
	/* since an int is returned, make sure len fits in one, otherwise return
	 * with an error (this avoids the flaw in the interface) */
	if((int)len < 0) {
		xz_error(state, LZMA_BUF_ERROR, "requested length does not fit in int");
		return -1;
	}
	/* if len is zero, avoid unnecessary operations */
	if(!len)
		return 0;
	/* process a skip request */
	if(state->seek) {
		state->seek = 0;
		if(xz_skip(state, state->skip) == -1)
			return -1;
	}
	// get len bytes to buf, or less than len if at the end 
	got = 0;
	do {
		// first just try copying data from the output buffer 
		if(state->have) {
			n = state->have > len ? len : state->have;
			memcpy(buf, state->next, n);
			state->next += n;
			state->have -= n;
		}
		// output buffer empty -- return if we're at the end of the input 
		else if(state->eof && strm->avail_in == 0)
			break;
		// need output data -- for small len or new stream load up our output buffer 
		else if(state->how == LOOK || len < (state->size << 1)) {
			// get more output, looking for header if required 
			if(xz_make(state) == -1)
				return -1;
			continue; /* no progress yet -- go back to memcpy() above */
			/* the copy above assures that we will leave with space in the
			 * output buffer, allowing at least one gzungetc() to succeed */
		}
		// large len -- read directly into user buffer 
		else if(state->how == COPY) { /* read directly */
			if(xz_load(state, static_cast<uchar *>(buf), len, &n) == -1)
				return -1;
		}
		/* large len -- decompress directly into user buffer */
		else {          /* state->how == LZMA */
			strm->avail_out = len;
			strm->next_out = static_cast<uint8_t *>(buf);
			if(xz_decomp(state) == -1)
				return -1;
			n = state->have;
			state->have = 0;
		}
		/* update progress */
		len -= n;
		buf = (char *)buf + n;
		got += n;
		state->pos += n;
	} while(len);
	return (int)got; // return number of bytes read into user buffer (will fit in int) 
}

int __libxml2_xzclose(xzFile file)
{
	int ret;
	xz_statep state;
	/* get internal structure */
	if(!file)
		return LZMA_DATA_ERROR;
	state = (xz_statep)file;
	/* free memory and close file */
	if(state->size) {
		lzma_end(&(state->strm));
#ifdef HAVE_ZLIB_H
		if(state->init == 1)
			inflateEnd(&(state->zstrm));
		state->init = 0;
#endif
		SAlloc::F(state->out);
		SAlloc::F(state->in);
	}
	SAlloc::F(state->path);
	ret = close(state->fd);
	SAlloc::F(state);
	return ret ? ret : LZMA_OK;
}

#endif /* HAVE_LZMA_H */
