/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include <zbar.h>
#pragma hdrstop
#include "error.h"
#include "symbol.h"

void _zbar_image_refcnt(zbar_image_t * img, int delta)
{
	if(!_zbar_refcnt(&img->refcnt, delta) && delta <= 0) {
		if(img->cleanup)
			img->cleanup(img);
		if(!img->src)
			_zbar_image_free(img);
	}
}

void _zbar_image_swap_symbols(zbar_image_t * a, zbar_image_t * b)
{
	zbar_symbol_set_t * tmp = a->syms;
	a->syms = b->syms;
	b->syms = tmp;
}

void _zbar_image_copy_size(zbar_image_t * dst, const zbar_image_t * src)
{
	dst->width = src->width;
	dst->height = src->height;
	dst->crop_x = src->crop_x;
	dst->crop_y = src->crop_y;
	dst->crop_w = src->crop_w;
	dst->crop_h = src->crop_h;
}

zbar_image_t * zbar_image_create()
{
	zbar_image_t * img = (zbar_image_t *)calloc(1, sizeof(zbar_image_t));
	_zbar_refcnt_init();
	_zbar_image_refcnt(img, 1);
	img->srcidx = -1;
	return(img);
}

void _zbar_image_free(zbar_image_t * img)
{
	if(img) {
		if(img->syms) {
			zbar_symbol_set_ref(img->syms, -1);
			img->syms = NULL;
		}
		free(img);
	}
}

void zbar_image_destroy(zbar_image_t * img)
{
	_zbar_image_refcnt(img, -1);
}

void zbar_image_ref(zbar_image_t * img, int refs)
{
	_zbar_image_refcnt(img, refs);
}

ulong zbar_image_get_format(const zbar_image_t * img)
{
	return img->Format;
}

uint zbar_image_get_sequence(const zbar_image_t * img)
{
	return img->seq;
}

uint zbar_image_get_width(const zbar_image_t * img)
{
	return img->width;
}

uint zbar_image_get_height(const zbar_image_t * img)
{
	return img->height;
}

void zbar_image_get_size(const zbar_image_t * img, uint * w, uint * h)
{
	ASSIGN_PTR(w, img->width);
	ASSIGN_PTR(h, img->height);
}

void zbar_image_get_crop(const zbar_image_t * img, uint * x, uint * y, uint * w, uint * h)
{
	ASSIGN_PTR(x, img->crop_x);
	ASSIGN_PTR(y, img->crop_y);
	ASSIGN_PTR(w, img->crop_w);
	ASSIGN_PTR(h, img->crop_h);
}

const void * zbar_image_get_data(const zbar_image_t * img)
{
	return img->P_Data;
}

ulong zbar_image_get_data_length(const zbar_image_t * img)
{
	return img->datalen;
}

void zbar_image_set_format(zbar_image_t * img, ulong fmt)
{
	img->Format = fmt;
}

void zbar_image_set_sequence(zbar_image_t * img, uint seq)
{
	img->seq = seq;
}

void zbar_image_set_size(zbar_image_t * img, uint w, uint h)
{
	img->crop_x = img->crop_y = 0;
	img->width = img->crop_w = w;
	img->height = img->crop_h = h;
}

void zbar_image_set_crop(zbar_image_t * img, uint x, uint y, uint w, uint h)
{
	uint img_w = img->width;
	SETMIN(x, img_w);
	if((x + w) > img_w) 
		w = (img_w - x);
	img->crop_x = x;
	img->crop_w = w;
	uint img_h = img->height;
	SETMIN(y, img_h);
	if((y + h) > img_h) 
		h = (img_h - y);
	img->crop_y = y;
	img->crop_h = h;
}

void zbar_image_free_data(zbar_image_t * img)
{
	if(img) {
		if(img->src) {
			zbar_image_t * newimg;
			// replace video image w/new copy 
			assert(img->refcnt); /* FIXME needs lock */
			newimg = zbar_image_create();
			memcpy(newimg, img, sizeof(zbar_image_t));
			// recycle video image 
			newimg->cleanup(newimg);
			// detach old image from src 
			img->cleanup = NULL;
			img->src = NULL;
			img->srcidx = -1;
		}
		else if(img->cleanup && img->P_Data) {
			if(img->cleanup != zbar_image_free_data) {
				//
				// using function address to detect this case is a bad idea;
				// windows link libraries add an extra layer of indirection...
				// this works around that problem (bug #2796277)
				//
				zbar_image_cleanup_handler_t cleanup = img->cleanup;
				img->cleanup = zbar_image_free_data;
				cleanup(img);
			}
			else
				free((void*)img->P_Data);
		}
		img->P_Data = NULL;
	}
}

void zbar_image_set_data(zbar_image_t * img, const void * data, ulong len, zbar_image_cleanup_handler_t cleanup)
{
	zbar_image_free_data(img);
	img->P_Data = data;
	img->datalen = len;
	img->cleanup = cleanup;
}

void zbar_image_set_userdata(zbar_image_t * img, void * userdata)
{
	img->userdata = userdata;
}

void * zbar_image_get_userdata(const zbar_image_t * img)
{
	return(img->userdata);
}

zbar_image_t * zbar_image_copy(const zbar_image_t * src)
{
	zbar_image_t * dst = zbar_image_create();
	dst->Format = src->Format;
	_zbar_image_copy_size(dst, src);
	dst->datalen = src->datalen;
	dst->P_Data = SAlloc::M(src->datalen);
	assert(dst->P_Data);
	memcpy((void*)dst->P_Data, src->P_Data, src->datalen);
	dst->cleanup = zbar_image_free_data;
	return(dst);
}

const zbar_symbol_set_t * zbar_image_get_symbols(const zbar_image_t * img)
{
	return(img->syms);
}

void zbar_image_set_symbols(zbar_image_t * img, const zbar_symbol_set_t * syms)
{
	if(syms)
		zbar_symbol_set_ref(syms, 1);
	if(img->syms)
		zbar_symbol_set_ref(img->syms, -1);
	img->syms = (zbar_symbol_set_t*)syms;
}

const zbar_symbol_t * zbar_image_first_symbol(const zbar_image_t * img)
{
	return(img->syms) ? img->syms->head : NULL;
}

typedef struct zimg_hdr_s {
	uint32 magic, format;
	uint16 width, height;
	uint32 size;
} zimg_hdr_t;

int zbar_image_write(const zbar_image_t * img, const char * filebase)
{
	int len = strlen(filebase) + 16;
	char * filename = (char *)SAlloc::M(len);
	int n = 0, rc = 0;
	FILE * f;
	zimg_hdr_t hdr;
	strcpy(filename, filebase);
	if((img->Format & 0xff) >= ' ')
		n = _snprintf(filename, len, "%s.%.4s.zimg", filebase, (char*)&img->Format);
	else
		n = _snprintf(filename, len, "%s.%08" PRIx32 ".zimg", filebase, img->Format);
	assert(n < len - 1);
	filename[len - 1] = '\0';
	zprintf(1, "dumping %.4s(%08" PRIx32 ") image to %s\n", (char*)&img->Format, img->Format, filename);
	f = fopen(filename, "w");
	if(!f) {
#ifdef HAVE_ERRNO_H
		rc = errno;
		zprintf(1, "ERROR opening %s: %s\n", filename, strerror(rc));
#else
		rc = 1;
#endif
		goto error;
	}
	hdr.magic = 0x676d697a;
	hdr.format = img->Format;
	hdr.width = img->width;
	hdr.height = img->height;
	hdr.size = img->datalen;
	if(fwrite(&hdr, sizeof(hdr), 1, f) != 1 || fwrite(img->P_Data, 1, img->datalen, f) != img->datalen) {
#ifdef HAVE_ERRNO_H
		rc = errno;
		zprintf(1, "ERROR writing %s: %s\n", filename, strerror(rc));
#else
		rc = 1;
#endif
		fclose(f);
		goto error;
	}

	rc = fclose(f);

error:
	free(filename);
	return(rc);
}

#ifdef DEBUG_SVG
	#include <png.h>

int zbar_image_write_png(const zbar_image_t * img, const char * filename)
{
	int rc = -1;
	FILE * file = NULL;
	png_struct * png = NULL;
	png_info * info = NULL;
	const uint8 ** rows = NULL;
	rows = SAlloc::M(img->height * sizeof(*rows));
	if(!rows)
		goto done;
	rows[0] = img->data;
	int y;
	for(y = 1; y < img->height; y++)
		rows[y] = rows[y - 1] + img->width;
	file = fopen(filename, "wb");
	if(!file)
		goto done;
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png)
		goto done;
	info = png_create_info_struct(png);
	if(!info)
		goto done;
	if(setjmp(png_jmpbuf(png)))
		goto done;
	png_init_io(png, file);
	png_set_compression_level(png, 9);
	png_set_IHDR(png, info, img->width, img->height, 8, PNG_COLOR_TYPE_GRAY,
	    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_rows(png, info, (void*)rows);
	png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
	png_write_end(png, info);
	rc = 0;
done:
	if(png)
		png_destroy_write_struct(&png, &info);
	free(rows);
	if(file)
		fclose(file);
	return(rc);
}

#endif
