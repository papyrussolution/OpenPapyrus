/* Copyright (c) 2004-2007, Sara Golemon <sarag@libssh2.org>
 * Copyright (c) 2010-2014, Daniel Stenberg <daniel@haxx.se>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the
 * following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names
 * of any other contributors may be used to endorse or
 * promote products derived from this software without
 * specific prior written permission.
 */
#include "libssh2_priv.h"
#pragma hdrstop
#ifdef LIBSSH2_HAVE_ZLIB
	#include <zlib.h>
#endif
//#include "comp.h"

/* ********
* none *
******** */

/*
 * comp_method_none_comp
 *
 * Minimalist compression: Absolutely none
 */
static int comp_method_none_comp(LIBSSH2_SESSION * session, uchar * dest, size_t * dest_len, const uchar * src, size_t src_len, void ** abstract)
{
	(void)session;
	(void)abstract;
	(void)dest;
	(void)dest_len;
	(void)src;
	(void)src_len;
	return 0;
}
/*
 * comp_method_none_decomp
 *
 * Minimalist decompression: Absolutely none
 */
static int comp_method_none_decomp(LIBSSH2_SESSION * session, uchar ** dest, size_t * dest_len, size_t payload_limit, const uchar * src, size_t src_len, void ** abstract)
{
	(void)session;
	(void)payload_limit;
	(void)abstract;
	*dest = (uchar *)src;
	*dest_len = src_len;
	return 0;
}

static const LIBSSH2_COMP_METHOD comp_method_none = {
	"none",
	0, /* not really compressing */
	0, /* isn't used in userauth, go figure */
	NULL,
	comp_method_none_comp,
	comp_method_none_decomp,
	NULL
};

#ifdef LIBSSH2_HAVE_ZLIB
/* ********
* zlib *
******** */

/* Memory management wrappers
 * Yes, I realize we're doing a callback to a callback,
 * Deal...
 */

static voidpf comp_method_zlib_alloc(voidpf opaque, uInt items, uInt size)
{
	LIBSSH2_SESSION * session = (LIBSSH2_SESSION*)opaque;
	return (voidpf)LIBSSH2_ALLOC(session, items * size);
}

static void comp_method_zlib_free(voidpf opaque, voidpf address)
{
	LIBSSH2_SESSION * session = (LIBSSH2_SESSION*)opaque;
	LIBSSH2_FREE(session, address);
}

/* libssh2_comp_method_zlib_init
 * All your bandwidth are belong to us (so save some)
 */
static int comp_method_zlib_init(LIBSSH2_SESSION * session, int compr, void ** abstract)
{
	int status;
	z_stream * strm = (z_stream *)LIBSSH2_CALLOC(session, sizeof(z_stream));
	if(!strm) {
		return _libssh2_error(session, LIBSSH2_ERROR_ALLOC, "Unable to allocate memory for zlib compression/decompression");
	}
	strm->opaque = (voidpf)session;
	strm->zalloc = (alloc_func)comp_method_zlib_alloc;
	strm->zfree = (free_func)comp_method_zlib_free;
	if(compr) {
		status = deflateInit(strm, Z_DEFAULT_COMPRESSION); /* deflate */
	}
	else {
		status = inflateInit(strm); /* inflate */
	}
	if(status != Z_OK) {
		LIBSSH2_FREE(session, strm);
		_libssh2_debug(session, LIBSSH2_TRACE_TRANS, "unhandled zlib error %d", status);
		return LIBSSH2_ERROR_COMPRESS;
	}
	*abstract = strm;
	return LIBSSH2_ERROR_NONE;
}
/*
 * libssh2_comp_method_zlib_comp
 *
 * Compresses source to destination. Without allocation.
 */
static int comp_method_zlib_comp(LIBSSH2_SESSION * session, uchar * dest,
    /* dest_len is a pointer to allow this function to update it with the final actual size used */
    size_t * dest_len, const uchar * src, size_t src_len, void ** abstract)
{
	z_stream * strm = (z_stream *)*abstract;
	int out_maxlen = *dest_len;
	int status;
	strm->next_in = (uchar *)src;
	strm->avail_in = src_len;
	strm->next_out = dest;
	strm->avail_out = out_maxlen;
	status = deflate(strm, Z_PARTIAL_FLUSH);
	if((status == Z_OK) && (strm->avail_out > 0)) {
		*dest_len = out_maxlen - strm->avail_out;
		return 0;
	}
	_libssh2_debug(session, LIBSSH2_TRACE_TRANS, "unhandled zlib compression error %d, avail_out", status, strm->avail_out);
	return _libssh2_error(session, LIBSSH2_ERROR_ZLIB, "compression failure");
}
/*
 * libssh2_comp_method_zlib_decomp
 *
 * Decompresses source to destination. Allocates the output memory.
 */
static int comp_method_zlib_decomp(LIBSSH2_SESSION * session, uchar ** dest, size_t * dest_len,
    size_t payload_limit, const uchar * src, size_t src_len, void ** abstract)
{
	z_stream * strm = (z_stream *)*abstract;
	// A short-term alloc of a full data chunk is better than a series of reallocs 
	char * out;
	int out_maxlen = 4 * src_len;
	// If strm is null, then we have not yet been initialized. 
	if(strm == NULL)
		return _libssh2_error(session, LIBSSH2_ERROR_COMPRESS, "decompression uninitialized"); ;
	// In practice they never come smaller than this 
	SETMAX(out_maxlen, 25);
	SETMIN(out_maxlen, (int)payload_limit);
	strm->next_in = (uchar *)src;
	strm->avail_in = src_len;
	strm->next_out = (uchar *)LIBSSH2_ALLOC(session, out_maxlen);
	out = (char *)strm->next_out;
	strm->avail_out = out_maxlen;
	if(!strm->next_out)
		return _libssh2_error(session, LIBSSH2_ERROR_ALLOC, "Unable to allocate decompression buffer");
	// Loop until it's all inflated or hit error 
	for(;;) {
		size_t out_ofs;
		char * newout;
		int status = inflate(strm, Z_PARTIAL_FLUSH);
		if(status == Z_OK) {
			if(strm->avail_out > 0)
				break; /* status is OK and the output buffer has not been exhausted so we're done */
		}
		else if(status == Z_BUF_ERROR) {
			break; /* the input data has been exhausted so we are done */
		}
		else {
			/* error state */
			LIBSSH2_FREE(session, out);
			_libssh2_debug(session, LIBSSH2_TRACE_TRANS, "unhandled zlib error %d", status);
			return _libssh2_error(session, LIBSSH2_ERROR_ZLIB, "decompression failure");
		}
		if(out_maxlen >= (int)payload_limit) {
			LIBSSH2_FREE(session, out);
			return _libssh2_error(session, LIBSSH2_ERROR_ZLIB, "Excessive growth in decompression phase");
		}
		/* If we get here we need to grow the output buffer and try again */
		out_ofs = out_maxlen - strm->avail_out;
		out_maxlen *= 2;
		newout = (char *)LIBSSH2_REALLOC(session, out, out_maxlen);
		if(!newout) {
			LIBSSH2_FREE(session, out);
			return _libssh2_error(session, LIBSSH2_ERROR_ALLOC, "Unable to expand decompression buffer");
		}
		out = newout;
		strm->next_out = reinterpret_cast<uchar *>(out) + out_ofs;
		strm->avail_out = out_maxlen - out_ofs;
	}
	*dest = reinterpret_cast<uchar *>(out);
	*dest_len = out_maxlen - strm->avail_out;
	return 0;
}

/* libssh2_comp_method_zlib_dtor
 * All done, no more compression for you
 */
static int comp_method_zlib_dtor(LIBSSH2_SESSION * session, int compr, void ** abstract)
{
	z_stream * strm = (z_stream *)*abstract;
	if(strm) {
		if(compr)
			deflateEnd(strm);
		else
			inflateEnd(strm);
		LIBSSH2_FREE(session, strm);
	}
	*abstract = NULL;
	return 0;
}

static const LIBSSH2_COMP_METHOD comp_method_zlib = {
	"zlib",
	1, /* yes, this compresses */
	1, /* do compression during userauth */
	comp_method_zlib_init,
	comp_method_zlib_comp,
	comp_method_zlib_decomp,
	comp_method_zlib_dtor,
};

static const LIBSSH2_COMP_METHOD comp_method_zlib_openssh = {
	"zlib@openssh.com",
	1, /* yes, this compresses */
	0, /* don't use compression during userauth */
	comp_method_zlib_init,
	comp_method_zlib_comp,
	comp_method_zlib_decomp,
	comp_method_zlib_dtor,
};
#endif /* LIBSSH2_HAVE_ZLIB */

/* If compression is enabled by the API, then this array is used which then
   may allow compression if zlib is available at build time */
static const LIBSSH2_COMP_METHOD * comp_methods[] = {
#ifdef LIBSSH2_HAVE_ZLIB
	&comp_method_zlib,
	&comp_method_zlib_openssh,
#endif /* LIBSSH2_HAVE_ZLIB */
	&comp_method_none,
	NULL
};

/* If compression is disabled by the API, then this array is used */
static const LIBSSH2_COMP_METHOD * no_comp_methods[] = {
	&comp_method_none,
	NULL
};

const LIBSSH2_COMP_METHOD ** _libssh2_comp_methods(LIBSSH2_SESSION * session)
{
	return session->flag.compress ? comp_methods : no_comp_methods;
}
