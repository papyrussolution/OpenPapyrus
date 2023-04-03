/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <libssh-internal.h>
#pragma hdrstop
#if 0 // @sobolev (это, по-видимому, дубликаты openssl-функций для совместимости) {
#include "libcrypto-compat.h"
#ifndef OPENSSL_NO_ENGINE
	#include <slib-ossl.h>
#endif

static void * OPENSSL_zalloc(size_t num)
{
	void * ret = OPENSSL_malloc(num);
	if(ret)
		memzero(ret, num);
	return ret;
}

int RSA_set0_key(RSA * r, BIGNUM * n, BIGNUM * e, BIGNUM * d)
{
	/* If the fields n and e in r are NULL, the corresponding input
	 * parameters MUST be non-NULL for n and e.  d may be
	 * left NULL (in case only the public key is used).
	 */
	if((r->n == NULL && n == NULL) || (r->e == NULL && e == NULL))
		return 0;
	if(n) {
		BN_free(r->n);
		r->n = n;
	}
	if(e) {
		BN_free(r->e);
		r->e = e;
	}
	if(d) {
		BN_free(r->d);
		r->d = d;
	}
	return 1;
}

int RSA_set0_factors(RSA * r, BIGNUM * p, BIGNUM * q)
{
	/* If the fields p and q in r are NULL, the corresponding input
	 * parameters MUST be non-NULL.
	 */
	if((r->p == NULL && p == NULL) || (r->q == NULL && q == NULL))
		return 0;
	if(p) {
		BN_free(r->p);
		r->p = p;
	}
	if(q) {
		BN_free(r->q);
		r->q = q;
	}
	return 1;
}

int RSA_set0_crt_params(RSA * r, BIGNUM * dmp1, BIGNUM * dmq1, BIGNUM * iqmp)
{
	/* If the fields dmp1, dmq1 and iqmp in r are NULL, the corresponding input
	 * parameters MUST be non-NULL.
	 */
	if((r->dmp1 == NULL && dmp1 == NULL) || (r->dmq1 == NULL && dmq1 == NULL) || (r->iqmp == NULL && iqmp == NULL))
		return 0;
	if(dmp1) {
		BN_free(r->dmp1);
		r->dmp1 = dmp1;
	}
	if(dmq1 != NULL) {
		BN_free(r->dmq1);
		r->dmq1 = dmq1;
	}
	if(iqmp != NULL) {
		BN_free(r->iqmp);
		r->iqmp = iqmp;
	}
	return 1;
}

void RSA_get0_key(const RSA * r, const BIGNUM ** n, const BIGNUM ** e, const BIGNUM ** d)
{
	ASSIGN_PTR(n, r->n);
	ASSIGN_PTR(e, r->e);
	ASSIGN_PTR(d, r->d);
}

void RSA_get0_factors(const RSA * r, const BIGNUM ** p, const BIGNUM ** q)
{
	ASSIGN_PTR(p, r->p);
	ASSIGN_PTR(q, r->q);
}

void RSA_get0_crt_params(const RSA * r, const BIGNUM ** dmp1, const BIGNUM ** dmq1, const BIGNUM ** iqmp)
{
	ASSIGN_PTR(dmp1, r->dmp1);
	ASSIGN_PTR(dmq1, r->dmq1);
	ASSIGN_PTR(iqmp, r->iqmp);
}

void DSA_get0_pqg(const DSA * d, const BIGNUM ** p, const BIGNUM ** q, const BIGNUM ** g)
{
	if(p)
		*p = d->p;
	if(q)
		*q = d->q;
	if(g)
		*g = d->g;
}

int DSA_set0_pqg(DSA * d, BIGNUM * p, BIGNUM * q, BIGNUM * g)
{
	/* If the fields p, q and g in d are NULL, the corresponding input
	 * parameters MUST be non-NULL.
	 */
	if((d->p == NULL && p == NULL) || (d->q == NULL && q == NULL) || (d->g == NULL && g == NULL))
		return 0;
	if(p) {
		BN_free(d->p);
		d->p = p;
	}
	if(q) {
		BN_free(d->q);
		d->q = q;
	}
	if(g) {
		BN_free(d->g);
		d->g = g;
	}

	return 1;
}

void DSA_get0_key(const DSA * d, const BIGNUM ** pub_key, const BIGNUM ** priv_key)
{
	if(pub_key)
		*pub_key = d->pub_key;
	if(priv_key)
		*priv_key = d->priv_key;
}

int DSA_set0_key(DSA * d, BIGNUM * pub_key, BIGNUM * priv_key)
{
	/* If the field pub_key in d is NULL, the corresponding input
	 * parameters MUST be non-NULL.  The priv_key field may
	 * be left NULL.
	 */
	if(d->pub_key == NULL && pub_key == NULL)
		return 0;

	if(pub_key) {
		BN_free(d->pub_key);
		d->pub_key = pub_key;
	}
	if(priv_key) {
		BN_free(d->priv_key);
		d->priv_key = priv_key;
	}

	return 1;
}

void DSA_SIG_get0(const DSA_SIG * sig, const BIGNUM ** pr, const BIGNUM ** ps)
{
	if(pr != NULL)
		*pr = sig->r;
	if(ps != NULL)
		*ps = sig->s;
}

int DSA_SIG_set0(DSA_SIG * sig, BIGNUM * r, BIGNUM * s)
{
	if(r == NULL || s == NULL)
		return 0;
	BN_clear_free(sig->r);
	BN_clear_free(sig->s);
	sig->r = r;
	sig->s = s;
	return 1;
}

void ECDSA_SIG_get0(const ECDSA_SIG * sig, const BIGNUM ** pr, const BIGNUM ** ps)
{
	if(pr != NULL)
		*pr = sig->r;
	if(ps != NULL)
		*ps = sig->s;
}

int ECDSA_SIG_set0(ECDSA_SIG * sig, BIGNUM * r, BIGNUM * s)
{
	if(r == NULL || s == NULL)
		return 0;
	BN_clear_free(sig->r);
	BN_clear_free(sig->s);
	sig->r = r;
	sig->s = s;
	return 1;
}

EVP_MD_CTX * EVP_MD_CTX_new()
{
	return OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

static void OPENSSL_clear_free(void * str, size_t num)
{
	if(str) {
		if(num)
			OPENSSL_cleanse(str, num);
		OPENSSL_free(str);
	}
}

/* This call frees resources associated with the context */
int EVP_MD_CTX_reset(EVP_MD_CTX * ctx)
{
	if(!ctx)
		return 1;
	/*
	 * Don't assume ctx->md_data was cleaned in EVP_Digest_Final, because
	 * sometimes only copies of the context are ever finalised.
	 */
	if(ctx->digest && ctx->digest->cleanup && !EVP_MD_CTX_test_flags(ctx, EVP_MD_CTX_FLAG_CLEANED))
		ctx->digest->cleanup(ctx);
	if(ctx->digest && ctx->digest->ctx_size && ctx->md_data && !EVP_MD_CTX_test_flags(ctx, EVP_MD_CTX_FLAG_REUSE)) {
		OPENSSL_clear_free(ctx->md_data, ctx->digest->ctx_size);
	}
	EVP_PKEY_CTX_free(ctx->pctx);
#ifndef OPENSSL_NO_ENGINE
	ENGINE_finish(ctx->engine);
#endif
	OPENSSL_cleanse(ctx, sizeof(*ctx));

	return 1;
}

void EVP_MD_CTX_free(EVP_MD_CTX * ctx)
{
	EVP_MD_CTX_reset(ctx);
	OPENSSL_free(ctx);
}

int EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX * ctx)
{
	EVP_CIPHER_CTX_init(ctx);
	return 1;
}

HMAC_CTX * HMAC_CTX_new()
{
	HMAC_CTX * ctx = (HMAC_CTX *)OPENSSL_zalloc(sizeof(HMAC_CTX));
	if(ctx) {
		if(!HMAC_CTX_reset(ctx)) {
			HMAC_CTX_free(ctx);
			return NULL;
		}
	}
	return ctx;
}

static void hmac_ctx_cleanup(HMAC_CTX * ctx)
{
	EVP_MD_CTX_reset(&ctx->i_ctx);
	EVP_MD_CTX_reset(&ctx->o_ctx);
	EVP_MD_CTX_reset(&ctx->md_ctx);
	ctx->md = NULL;
	ctx->key_length = 0;
	OPENSSL_cleanse(ctx->key, sizeof(ctx->key));
}

void HMAC_CTX_free(HMAC_CTX * ctx)
{
	if(ctx) {
		hmac_ctx_cleanup(ctx);
#if OPENSSL_VERSION_NUMBER > 0x10100000L
		EVP_MD_CTX_free(&ctx->i_ctx);
		EVP_MD_CTX_free(&ctx->o_ctx);
		EVP_MD_CTX_free(&ctx->md_ctx);
#endif
		OPENSSL_free(ctx);
	}
}

int HMAC_CTX_reset(HMAC_CTX * ctx)
{
	HMAC_CTX_init(ctx);
	return 1;
}

#ifndef HAVE_OPENSSL_EVP_CIPHER_CTX_NEW
	EVP_CIPHER_CTX * EVP_CIPHER_CTX_new()
	{
		return OPENSSL_zalloc(sizeof(EVP_CIPHER_CTX));
	}

	void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX * ctx)
	{
		/* EVP_CIPHER_CTX_reset(ctx); alias */
		EVP_CIPHER_CTX_init(ctx);
		OPENSSL_free(ctx);
	}
#endif

void DH_get0_pqg(const DH * dh, const BIGNUM ** p, const BIGNUM ** q, const BIGNUM ** g)
{
	if(p) {
		*p = dh->p;
	}
	if(q) {
		*q = NULL;
	}
	if(g) {
		*g = dh->g;
	}
}

int DH_set0_pqg(DH * dh, BIGNUM * p, BIGNUM * q, BIGNUM * g)
{
	if(p) {
		if(dh->p) {
			BN_free(dh->p);
		}
		dh->p = p;
	}
	if(g) {
		if(dh->g) {
			BN_free(dh->g);
		}
		dh->g = g;
	}
	return 1;
}

void DH_get0_key(const DH * dh,
    const BIGNUM ** pub_key, const BIGNUM ** priv_key)
{
	if(pub_key) {
		*pub_key = dh->pub_key;
	}
	if(priv_key) {
		*priv_key = dh->priv_key;
	}
}

int DH_set0_key(DH * dh, BIGNUM * pub_key, BIGNUM * priv_key)
{
	if(pub_key) {
		if(dh->pub_key) {
			BN_free(dh->pub_key);
		}
		dh->pub_key = pub_key;
	}
	if(priv_key) {
		if(dh->priv_key) {
			BN_free(dh->priv_key);
		}
		dh->priv_key = priv_key;
	}
	return 1;
}

const char * OpenSSL_version(int type)
{
	return SSLeay_version(type);
}

ulong OpenSSL_version_num()
{
	return SSLeay();
}
#endif // } 0 @sobolev