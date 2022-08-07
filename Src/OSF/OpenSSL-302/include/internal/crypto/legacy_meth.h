/*
 * Copyright 2019-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#define IMPLEMENT_LEGACY_EVP_MD_METH(CTX, nm, fn) \
	static int nm ## _init(EVP_MD_CTX *ctx) { return fn ## _Init((CTX*)EVP_MD_CTX_get0_md_data(ctx)); } \
	static int nm ## _update(EVP_MD_CTX *ctx, const void * data, size_t count) { return fn ## _Update((CTX*)EVP_MD_CTX_get0_md_data(ctx), (const unsigned char*)data, count); } \
	static int nm ## _final(EVP_MD_CTX *ctx, uchar * md) { return fn ## _Final(md, (CTX*)EVP_MD_CTX_get0_md_data(ctx)); }

// @sobolev (CTX arg added)
#define IMPLEMENT_LEGACY_EVP_MD_METH_LC(CTX, nm, fn) \
	static int nm ## _init(EVP_MD_CTX *ctx) { return fn ## _init((CTX *)EVP_MD_CTX_get0_md_data(ctx)); } \
	static int nm ## _update(EVP_MD_CTX *ctx, const void * data, size_t count) { return fn ## _update((CTX *)EVP_MD_CTX_get0_md_data(ctx), data, count); } \
	static int nm ## _final(EVP_MD_CTX *ctx, uchar * md) { return fn ## _final(md, (CTX *)EVP_MD_CTX_get0_md_data(ctx)); }

#define LEGACY_EVP_MD_METH_TABLE(init, update, final, ctrl, blksz) init, update, final, NULL, NULL, blksz, 0, ctrl
