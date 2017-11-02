/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
#include <openssl/hmac.h>
#include <openssl/kdf.h>
//#include <openssl/evp.h>
//#include <internal/evp_int.h>

#define HKDF_MAXBUF 1024

static uchar * HKDF(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len);
static uchar * HKDF_Extract(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, uchar * prk, size_t * prk_len);
static uchar * HKDF_Expand(const EVP_MD * evp_md, const uchar * prk, size_t prk_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len);

typedef struct {
	const EVP_MD * md;
	uchar * salt;
	size_t salt_len;
	uchar * key;
	size_t key_len;
	uchar info[HKDF_MAXBUF];
	size_t info_len;
} HKDF_PKEY_CTX;

static int pkey_hkdf_init(EVP_PKEY_CTX * ctx)
{
	HKDF_PKEY_CTX * kctx = (HKDF_PKEY_CTX *)OPENSSL_zalloc(sizeof(*kctx));
	if(kctx == NULL)
		return 0;
	ctx->data = kctx;
	return 1;
}

static void pkey_hkdf_cleanup(EVP_PKEY_CTX * ctx)
{
	HKDF_PKEY_CTX * kctx = (HKDF_PKEY_CTX *)ctx->data;
	OPENSSL_clear_free(kctx->salt, kctx->salt_len);
	OPENSSL_clear_free(kctx->key, kctx->key_len);
	OPENSSL_cleanse(kctx->info, kctx->info_len);
	OPENSSL_free(kctx);
}

static int pkey_hkdf_ctrl(EVP_PKEY_CTX * ctx, int type, int p1, void * p2)
{
	HKDF_PKEY_CTX * kctx = (HKDF_PKEY_CTX *)ctx->data;
	switch(type) {
		case EVP_PKEY_CTRL_HKDF_MD:
		    if(p2 == NULL)
			    return 0;
		    kctx->md = (EVP_MD *)p2;
		    return 1;
		case EVP_PKEY_CTRL_HKDF_SALT:
		    if(p1 == 0 || p2 == NULL)
			    return 1;
		    if(p1 < 0)
			    return 0;
		    if(kctx->salt != NULL)
			    OPENSSL_clear_free(kctx->salt, kctx->salt_len);
		    kctx->salt = (uchar *)OPENSSL_memdup(p2, p1);
		    if(kctx->salt == NULL)
			    return 0;

		    kctx->salt_len = p1;
		    return 1;

		case EVP_PKEY_CTRL_HKDF_KEY:
		    if(p1 < 0)
			    return 0;
		    if(kctx->key != NULL)
			    OPENSSL_clear_free(kctx->key, kctx->key_len);
		    kctx->key = (uchar *)OPENSSL_memdup(p2, p1);
		    if(kctx->key == NULL)
			    return 0;
		    kctx->key_len  = p1;
		    return 1;
		case EVP_PKEY_CTRL_HKDF_INFO:
		    if(p1 == 0 || p2 == NULL)
			    return 1;
		    if(p1 < 0 || p1 > (int)(HKDF_MAXBUF - kctx->info_len))
			    return 0;
		    memcpy(kctx->info + kctx->info_len, p2, p1);
		    kctx->info_len += p1;
		    return 1;
		default:
		    return -2;
	}
}

static int pkey_hkdf_ctrl_str(EVP_PKEY_CTX * ctx, const char * type, const char * value)
{
	if(sstreq(type, "md"))
		return EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_get_digestbyname(value));
	else if(sstreq(type, "salt"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_SALT, value);
	else if(sstreq(type, "hexsalt"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_SALT, value);
	else if(sstreq(type, "key"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_KEY, value);
	else if(sstreq(type, "hexkey"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_KEY, value);
	else if(sstreq(type, "info"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_INFO, value);
	else if(sstreq(type, "hexinfo"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_INFO, value);
	else 
		return -2;
}

static int pkey_hkdf_derive(EVP_PKEY_CTX * ctx, uchar * key, size_t * keylen)
{
	HKDF_PKEY_CTX * kctx = (HKDF_PKEY_CTX *)ctx->data;
	if(kctx->md == NULL || kctx->key == NULL)
		return 0;
	if(HKDF(kctx->md, kctx->salt, kctx->salt_len, kctx->key, kctx->key_len, kctx->info, kctx->info_len, key, *keylen) == NULL) {
		return 0;
	}
	return 1;
}

const EVP_PKEY_METHOD hkdf_pkey_meth = {
	EVP_PKEY_HKDF,
	0,
	pkey_hkdf_init,
	0,
	pkey_hkdf_cleanup,

	0, 0,
	0, 0,

	0,
	0,

	0,
	0,

	0, 0,

	0, 0, 0, 0,

	0, 0,

	0, 0,

	0,
	pkey_hkdf_derive,
	pkey_hkdf_ctrl,
	pkey_hkdf_ctrl_str
};

static uchar * HKDF(const EVP_MD * evp_md, const uchar * salt, size_t salt_len,
    const uchar * key, size_t key_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len)
{
	uchar prk[EVP_MAX_MD_SIZE];
	size_t prk_len;
	if(!HKDF_Extract(evp_md, salt, salt_len, key, key_len, prk, &prk_len))
		return NULL;
	return HKDF_Expand(evp_md, prk, prk_len, info, info_len, okm, okm_len);
}

static uchar * HKDF_Extract(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, uchar * prk, size_t * prk_len)
{
	uint tmp_len;
	if(!HMAC(evp_md, salt, salt_len, key, key_len, prk, &tmp_len))
		return NULL;
	*prk_len = tmp_len;
	return prk;
}

static uchar * HKDF_Expand(const EVP_MD * evp_md, const uchar * prk, size_t prk_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len)
{
	HMAC_CTX * hmac;
	uint i;
	uchar prev[EVP_MAX_MD_SIZE];
	size_t done_len = 0, dig_len = EVP_MD_size(evp_md);
	size_t n = okm_len / dig_len;
	if(okm_len % dig_len)
		n++;
	if(n > 255)
		return NULL;
	if((hmac = HMAC_CTX_new()) == NULL)
		return NULL;
	if(!HMAC_Init_ex(hmac, prk, prk_len, evp_md, NULL))
		goto err;
	for(i = 1; i <= n; i++) {
		size_t copy_len;
		const uchar ctr = i;
		if(i > 1) {
			if(!HMAC_Init_ex(hmac, NULL, 0, NULL, NULL))
				goto err;
			if(!HMAC_Update(hmac, prev, dig_len))
				goto err;
		}
		if(!HMAC_Update(hmac, info, info_len))
			goto err;
		if(!HMAC_Update(hmac, &ctr, 1))
			goto err;
		if(!HMAC_Final(hmac, prev, NULL))
			goto err;
		copy_len = (done_len + dig_len > okm_len) ? okm_len - done_len : dig_len;
		memcpy(okm + done_len, prev, copy_len);
		done_len += copy_len;
	}
	HMAC_CTX_free(hmac);
	return okm;
err:
	HMAC_CTX_free(hmac);
	return NULL;
}

