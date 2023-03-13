//
//
#include "mupdf/fitz.h"
#pragma hdrstop
#include "z-imp.h"

void fz_deflate(fz_context * ctx, uchar * dest, size_t * destLen, const uchar * source, size_t sourceLen, fz_deflate_level level)
{
	z_stream stream;
	int err;
	size_t left = *destLen;
	*destLen = 0;
	stream.zalloc = fz_zlib_alloc;
	stream.zfree = fz_zlib_free;
	stream.opaque = ctx;
	err = deflateInit(&stream, (int)level);
	if(err != Z_OK)
		fz_throw(ctx, FZ_ERROR_GENERIC, "zlib compression failed: %d", err);
	stream.next_out = dest;
	stream.avail_out = 0;
	stream.next_in = (const Bytef*)source;
	stream.avail_in = 0;
	do {
		if(stream.avail_out == 0) {
			stream.avail_out = left > UINT_MAX ? UINT_MAX : (uInt)left;
			left -= stream.avail_out;
		}
		if(stream.avail_in == 0) {
			stream.avail_in = sourceLen > UINT_MAX ? UINT_MAX : (uInt)sourceLen;
			sourceLen -= stream.avail_in;
		}
		err = deflate(&stream, sourceLen ? Z_NO_FLUSH : Z_FINISH);
	} while(err == Z_OK);

	/* We might have problems if the compressed length > uLong sized. Tough, for now. */
	*destLen = stream.total_out;
	deflateEnd(&stream);
	if(err != Z_STREAM_END)
		fz_throw(ctx, FZ_ERROR_GENERIC, "Zlib failure: %d", err);
}

uchar * fz_new_deflated_data(fz_context * ctx, size_t * compressed_length, const uchar * source, size_t source_length, fz_deflate_level level)
{
	size_t bound = fz_deflate_bound(ctx, source_length);
	uchar * cdata = (uchar *)Memento_label(fz_malloc(ctx, bound), "deflated_data");
	*compressed_length = 0;
	fz_try(ctx)
	fz_deflate(ctx, cdata, &bound, source, source_length, level);
	fz_catch(ctx)
	{
		fz_free(ctx, cdata);
		fz_rethrow(ctx);
	}
	*compressed_length = bound;
	return cdata;
}

uchar * fz_new_deflated_data_from_buffer(fz_context * ctx, size_t * compressed_length, fz_buffer * buffer, fz_deflate_level level)
{
	uchar * data;
	size_t size = fz_buffer_storage(ctx, buffer, &data);
	if(size == 0 || data == NULL) {
		*compressed_length = 0;
		return NULL;
	}
	return fz_new_deflated_data(ctx, compressed_length, data, size, level);
}

size_t fz_deflate_bound(fz_context * ctx, size_t size)
{
	/* Copied from zlib to account for size_t vs uLong */
	return size + (size >> 12) + (size >> 14) + (size >> 25) + 13;
}
