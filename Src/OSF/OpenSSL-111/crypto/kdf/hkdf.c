/*
 * Copyright 2016-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

#define HKDF_MAXBUF 1024

static uchar * HKDF(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len);
static uchar * HKDF_Extract(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, uchar * prk, size_t * prk_len);
static uchar * HKDF_Expand(const EVP_MD * evp_md, const uchar * prk, size_t prk_len, const uchar * info, size_t info_len, uchar * okm, size_t okm_len);

typedef struct {
	int mode;
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
	HKDF_PKEY_CTX * kctx;
	if((kctx = static_cast<HKDF_PKEY_CTX *>(OPENSSL_zalloc(sizeof(*kctx)))) == NULL) {
		KDFerr(KDF_F_PKEY_HKDF_INIT, ERR_R_MALLOC_FAILURE);
		return 0;
	}
	ctx->data = kctx;
	return 1;
}

static void pkey_hkdf_cleanup(EVP_PKEY_CTX * ctx)
{
	HKDF_PKEY_CTX * kctx = static_cast<HKDF_PKEY_CTX *>(ctx->data);
	OPENSSL_clear_free(kctx->salt, kctx->salt_len);
	OPENSSL_clear_free(kctx->key, kctx->key_len);
	OPENSSL_cleanse(kctx->info, kctx->info_len);
	OPENSSL_free(kctx);
}

static int pkey_hkdf_ctrl(EVP_PKEY_CTX * ctx, int type, int p1, void * p2)
{
	HKDF_PKEY_CTX * kctx = static_cast<HKDF_PKEY_CTX *>(ctx->data);
	switch(type) {
		case EVP_PKEY_CTRL_HKDF_MD:
		    if(p2 == NULL)
			    return 0;
		    kctx->md = static_cast<const EVP_MD *>(p2);
		    return 1;
		case EVP_PKEY_CTRL_HKDF_MODE:
		    kctx->mode = p1;
		    return 1;
		case EVP_PKEY_CTRL_HKDF_SALT:
		    if(p1 == 0 || p2 == NULL)
			    return 1;

		    if(p1 < 0)
			    return 0;

		    if(kctx->salt != NULL)
			    OPENSSL_clear_free(kctx->salt, kctx->salt_len);

		    kctx->salt = static_cast<uchar *>(OPENSSL_memdup(p2, p1));
		    if(kctx->salt == NULL)
			    return 0;

		    kctx->salt_len = p1;
		    return 1;

		case EVP_PKEY_CTRL_HKDF_KEY:
		    if(p1 < 0)
			    return 0;
		    if(kctx->key != NULL)
			    OPENSSL_clear_free(kctx->key, kctx->key_len);
		    kctx->key = static_cast<uchar *>(OPENSSL_memdup(p2, p1));
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
	if(sstreq(type, "mode")) {
		int mode;
		if(sstreq(value, "EXTRACT_AND_EXPAND"))
			mode = EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND;
		else if(sstreq(value, "EXTRACT_ONLY"))
			mode = EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY;
		else if(sstreq(value, "EXPAND_ONLY"))
			mode = EVP_PKEY_HKDEF_MODE_EXPAND_ONLY;
		else
			return 0;
		return EVP_PKEY_CTX_hkdf_mode(ctx, mode);
	}
	if(sstreq(type, "md"))
		return EVP_PKEY_CTX_md(ctx, EVP_PKEY_OP_DERIVE, EVP_PKEY_CTRL_HKDF_MD, value);
	if(sstreq(type, "salt"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_SALT, value);
	if(sstreq(type, "hexsalt"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_SALT, value);
	if(sstreq(type, "key"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_KEY, value);
	if(sstreq(type, "hexkey"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_KEY, value);
	if(sstreq(type, "info"))
		return EVP_PKEY_CTX_str2ctrl(ctx, EVP_PKEY_CTRL_HKDF_INFO, value);
	if(sstreq(type, "hexinfo"))
		return EVP_PKEY_CTX_hex2ctrl(ctx, EVP_PKEY_CTRL_HKDF_INFO, value);
	KDFerr(KDF_F_PKEY_HKDF_CTRL_STR, KDF_R_UNKNOWN_PARAMETER_TYPE);
	return -2;
}

static int pkey_hkdf_derive_init(EVP_PKEY_CTX * ctx)
{
	HKDF_PKEY_CTX * kctx = static_cast<HKDF_PKEY_CTX *>(ctx->data);
	OPENSSL_clear_free(kctx->key, kctx->key_len);
	OPENSSL_clear_free(kctx->salt, kctx->salt_len);
	OPENSSL_cleanse(kctx->info, kctx->info_len);
	memzero(kctx, sizeof(*kctx));
	return 1;
}

static int pkey_hkdf_derive(EVP_PKEY_CTX * ctx, uchar * key, size_t * keylen)
{
	HKDF_PKEY_CTX * kctx = static_cast<HKDF_PKEY_CTX *>(ctx->data);
	if(kctx->md == NULL) {
		KDFerr(KDF_F_PKEY_HKDF_DERIVE, KDF_R_MISSING_MESSAGE_DIGEST);
		return 0;
	}
	if(kctx->key == NULL) {
		KDFerr(KDF_F_PKEY_HKDF_DERIVE, KDF_R_MISSING_KEY);
		return 0;
	}
	switch(kctx->mode) {
		case EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND:
		    return HKDF(kctx->md, kctx->salt, kctx->salt_len, kctx->key, kctx->key_len, kctx->info, kctx->info_len, key, *keylen) != NULL;
		case EVP_PKEY_HKDEF_MODE_EXTRACT_ONLY:
		    if(!key) {
			    *keylen = EVP_MD_size(kctx->md);
			    return 1;
		    }
		    return HKDF_Extract(kctx->md, kctx->salt, kctx->salt_len, kctx->key, kctx->key_len, key, keylen) != NULL;
		case EVP_PKEY_HKDEF_MODE_EXPAND_ONLY:
		    return HKDF_Expand(kctx->md, kctx->key, kctx->key_len, kctx->info, kctx->info_len, key, *keylen) != NULL;
		default:
		    return 0;
	}
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

	pkey_hkdf_derive_init,
	pkey_hkdf_derive,
	pkey_hkdf_ctrl,
	pkey_hkdf_ctrl_str
};

static uchar * HKDF(const EVP_MD * evp_md, const uchar * salt, size_t salt_len, const uchar * key, size_t key_len, 
	const uchar * info, size_t info_len, uchar * okm, size_t okm_len)
{
	uchar prk[EVP_MAX_MD_SIZE];
	uchar * ret;
	size_t prk_len;
	if(!HKDF_Extract(evp_md, salt, salt_len, key, key_len, prk, &prk_len))
		return NULL;
	ret = HKDF_Expand(evp_md, prk, prk_len, info, info_len, okm, okm_len);
	OPENSSL_cleanse(prk, sizeof(prk));
	return ret;
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
	uchar * ret = NULL;
	uint i;
	uchar prev[EVP_MAX_MD_SIZE];
	size_t done_len = 0, dig_len = EVP_MD_size(evp_md);
	size_t n = okm_len / dig_len;
	if(okm_len % dig_len)
		n++;
	if(n > 255 || okm == NULL)
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
	ret = okm;

err:
	OPENSSL_cleanse(prev, sizeof(prev));
	HMAC_CTX_free(hmac);
	return ret;
}
