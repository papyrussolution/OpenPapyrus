/*
   zip_buffer.c -- bounds checked access to memory buffer
   Copyright (C) 2014 Dieter Baron and Thomas Klausner

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

#include <stdlib.h>
#include <string.h>

#include "zipint.h"

uint8 * _zip_buffer_data(zip_buffer_t * buffer)
{
	return buffer->data;
}

void _zip_buffer_free(zip_buffer_t * buffer)
{
	if(buffer == NULL) {
		return;
	}

	if(buffer->free_data) {
		free(buffer->data);
	}

	free(buffer);
}

bool _zip_buffer_eof(zip_buffer_t * buffer)
{
	return buffer->ok && buffer->offset == buffer->size;
}

uint8 * _zip_buffer_get(zip_buffer_t * buffer, uint64 length)
{
	uint8 * data;

	if(!buffer->ok || buffer->offset + length < length || buffer->offset + length > buffer->size) {
		buffer->ok = false;
		return NULL;
	}

	data = buffer->data + buffer->offset;
	buffer->offset += length;
	return data;
}

uint16 _zip_buffer_get_16(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 2);

	if(data == NULL) {
		return 0;
	}

	return (uint16)(data[0] + (data[1] << 8));
}

uint32 _zip_buffer_get_32(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 4);

	if(data == NULL) {
		return 0;
	}

	return ((((((uint32)data[3] << 8) + data[2]) << 8) + data[1]) << 8) + data[0];
}

uint64 _zip_buffer_get_64(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 8);

	if(data == NULL) {
		return 0;
	}

	return ((uint64)data[7] <<
	    56) +
	       ((uint64)data[6] <<
	    48) +
	       ((uint64)data[5] <<
	    40) + ((uint64)data[4] << 32) + ((uint64)data[3] << 24) + ((uint64)data[2] << 16) + ((uint64)data[1] << 8) + (uint64)data[0];
}

uint8 _zip_buffer_get_8(zip_buffer_t * buffer)
{
	uint8 * data = _zip_buffer_get(buffer, 1);

	if(data == NULL) {
		return 0;
	}

	return data[0];
}

uint64 _zip_buffer_left(zip_buffer_t * buffer)
{
	return buffer->ok ? buffer->size - buffer->offset : 0;
}

zip_buffer_t * _zip_buffer_new(uint8 * data, uint64 size)
{
	bool free_data = (data == NULL);
	zip_buffer_t * buffer;

	if(data == NULL) {
		if((data = (uint8*)malloc(size)) == NULL) {
			return NULL;
		}
	}

	if((buffer = (zip_buffer_t*)malloc(sizeof(*buffer))) == NULL) {
		if(free_data) {
			free(data);
		}
		return NULL;
	}

	buffer->ok = true;
	buffer->data = data;
	buffer->size = size;
	buffer->offset = 0;
	buffer->free_data = free_data;

	return buffer;
}

zip_buffer_t * _zip_buffer_new_from_source(zip_source_t * src, uint64 size, uint8 * buf, zip_error_t * error)
{
	zip_buffer_t * buffer;

	if((buffer = _zip_buffer_new(buf, size)) == NULL) {
		zip_error_set(error, ZIP_ER_MEMORY, 0);
		return NULL;
	}

	if(_zip_read(src, buffer->data, size, error) < 0) {
		_zip_buffer_free(buffer);
		return NULL;
	}

	return buffer;
}

uint64 _zip_buffer_offset(zip_buffer_t * buffer)
{
	return buffer->ok ? buffer->offset : 0;
}

bool _zip_buffer_ok(zip_buffer_t * buffer)
{
	return buffer->ok;
}

int _zip_buffer_put(zip_buffer_t * buffer, const void * src, size_t length)
{
	uint8 * dst = _zip_buffer_get(buffer, length);

	if(dst == NULL) {
		return -1;
	}

	memcpy(dst, src, length);
	return 0;
}

int _zip_buffer_put_16(zip_buffer_t * buffer, uint16 i)
{
	uint8 * data = _zip_buffer_get(buffer, 2);

	if(data == NULL) {
		return -1;
	}

	data[0] = (uint8)(i & 0xff);
	data[1] = (uint8)((i >> 8) & 0xff);

	return 0;
}

int _zip_buffer_put_32(zip_buffer_t * buffer, uint32 i)
{
	uint8 * data = _zip_buffer_get(buffer, 4);

	if(data == NULL) {
		return -1;
	}

	data[0] = (uint8)(i & 0xff);
	data[1] = (uint8)((i >> 8) & 0xff);
	data[2] = (uint8)((i >> 16) & 0xff);
	data[3] = (uint8)((i >> 24) & 0xff);

	return 0;
}

int _zip_buffer_put_64(zip_buffer_t * buffer, uint64 i)
{
	uint8 * data = _zip_buffer_get(buffer, 8);

	if(data == NULL) {
		return -1;
	}

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

int _zip_buffer_put_8(zip_buffer_t * buffer, uint8 i)
{
	uint8 * data = _zip_buffer_get(buffer, 1);

	if(data == NULL) {
		return -1;
	}

	data[0] = i;

	return 0;
}

int _zip_buffer_set_offset(zip_buffer_t * buffer, uint64 offset)
{
	if(offset > buffer->size) {
		buffer->ok = false;
		return -1;
	}

	buffer->ok = true;
	buffer->offset = offset;

	return 0;
}

int _zip_buffer_skip(zip_buffer_t * buffer, uint64 length)
{
	uint64 offset = buffer->offset + length;

	if(offset < buffer->offset) {
		buffer->ok = false;
		return -1;
	}
	return _zip_buffer_set_offset(buffer, offset);
}

uint64 _zip_buffer_size(zip_buffer_t * buffer)
{
	return buffer->size;
}

