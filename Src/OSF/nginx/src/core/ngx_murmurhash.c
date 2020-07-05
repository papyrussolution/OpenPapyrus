/*
 * Copyright (C) Austin Appleby
 */
#include <ngx_config.h>
#include <ngx_core.h>
#pragma hdrstop

/* @v10.8.0 (replaced with SlHash::Murmur2_32(a, b, 0)) 
uint32_t ngx_murmur_hash2(u_char * data, size_t len)
{
	uint32_t k;
	uint32_t h = 0 ^ len;
	while(len >= 4) {
		k  = data[0];
		k |= data[1] << 8;
		k |= data[2] << 16;
		k |= data[3] << 24;
		k *= 0x5bd1e995;
		k ^= k >> 24;
		k *= 0x5bd1e995;
		h *= 0x5bd1e995;
		h ^= k;
		data += 4;
		len -= 4;
	}
	switch(len) {
		case 3: h ^= data[2] << 16;
		// @fallthrough
		case 2: h ^= data[1] << 8;
		// @fallthrough
		case 1:
		    h ^= data[0];
		    h *= 0x5bd1e995;
	}
	h ^= h >> 13;
	h *= 0x5bd1e995;
	h ^= h >> 15;
	return h;
}*/
