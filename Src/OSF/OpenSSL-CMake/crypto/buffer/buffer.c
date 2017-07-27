/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
/*
 * LIMIT_BEFORE_EXPANSION is the maximum n such that (n+3)/3*4 < 2**31. That
 * function is applied in several functions in this file and this limit
 * ensures that the result fits in an int.
 */
#define LIMIT_BEFORE_EXPANSION 0x5ffffffc

BUF_MEM * BUF_MEM_new_ex(ulong flags)
{
	BUF_MEM * ret = BUF_MEM_new();
	if(ret)
		ret->flags = flags;
	return ret;
}

BUF_MEM * BUF_MEM_new(void)
{
	BUF_MEM * ret;
	ret = (BUF_MEM*)OPENSSL_zalloc(sizeof(*ret));
	if(!ret) {
		BUFerr(BUF_F_BUF_MEM_NEW, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	return ret;
}

void BUF_MEM_free(BUF_MEM * a)
{
	if(!a)
		return;

	if(a->data != NULL) {
		if(a->flags & BUF_MEM_FLAG_SECURE)
			OPENSSL_secure_free(a->data);
		else
			OPENSSL_clear_free(a->data, a->max);
	}
	OPENSSL_free(a);
}

/* Allocate a block of secure memory; copy over old data if there
 * was any, and then free it. */
static char * sec_alloc_realloc(BUF_MEM * str, size_t len)
{
	char * ret = (char*)OPENSSL_secure_malloc(len);
	if(str->data != NULL) {
		if(ret)
			memcpy(ret, str->data, str->length);
		OPENSSL_secure_free(str->data);
	}
	return ret;
}

size_t BUF_MEM_grow(BUF_MEM * str, size_t len)
{
	char * ret;
	size_t n;
	if(str->length >= len) {
		str->length = len;
		return (len);
	}
	if(str->max >= len) {
		if(str->data != NULL)
			memzero(&str->data[str->length], len - str->length);
		str->length = len;
		return (len);
	}
	/* This limit is sufficient to ensure (len+3)/3*4 < 2**31 */
	if(len > LIMIT_BEFORE_EXPANSION) {
		BUFerr(BUF_F_BUF_MEM_GROW, ERR_R_MALLOC_FAILURE);
		return 0;
	}
	n = (len + 3) / 3 * 4;
	if((str->flags & BUF_MEM_FLAG_SECURE))
		ret = sec_alloc_realloc(str, n);
	else
		ret = (char*)OPENSSL_realloc(str->data, n);
	if(!ret) {
		BUFerr(BUF_F_BUF_MEM_GROW, ERR_R_MALLOC_FAILURE);
		len = 0;
	}
	else {
		str->data = ret;
		str->max = n;
		memzero(&str->data[str->length], len - str->length);
		str->length = len;
	}
	return (len);
}

size_t BUF_MEM_grow_clean(BUF_MEM * str, size_t len)
{
	char * ret;
	size_t n;
	if(str->length >= len) {
		if(str->data != NULL)
			memzero(&str->data[len], str->length - len);
		str->length = len;
		return (len);
	}
	if(str->max >= len) {
		memzero(&str->data[str->length], len - str->length);
		str->length = len;
		return (len);
	}
	/* This limit is sufficient to ensure (len+3)/3*4 < 2**31 */
	if(len > LIMIT_BEFORE_EXPANSION) {
		BUFerr(BUF_F_BUF_MEM_GROW_CLEAN, ERR_R_MALLOC_FAILURE);
		return 0;
	}
	n = (len + 3) / 3 * 4;
	if((str->flags & BUF_MEM_FLAG_SECURE))
		ret = sec_alloc_realloc(str, n);
	else
		ret = (char*)OPENSSL_clear_realloc(str->data, str->max, n);
	if(!ret) {
		BUFerr(BUF_F_BUF_MEM_GROW_CLEAN, ERR_R_MALLOC_FAILURE);
		len = 0;
	}
	else {
		str->data = ret;
		str->max = n;
		memzero(&str->data[str->length], len - str->length);
		str->length = len;
	}
	return (len);
}

void BUF_reverse(uchar * out, const uchar * in, size_t size)
{
	size_t i;
	if(in) {
		out += size - 1;
		for(i = 0; i < size; i++)
			*out-- = *in++;
	}
	else {
		char c;
		uchar * q = out + size - 1;
		for(i = 0; i < size / 2; i++) {
			c = *q;
			*q-- = *out;
			*out++ = c;
		}
	}
}

