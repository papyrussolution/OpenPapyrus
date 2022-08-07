/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include "crypto/evp.h"

static int null_init_key(EVP_CIPHER_CTX * ctx, const uchar * key,
    const uchar * iv, int enc);
static int null_cipher(EVP_CIPHER_CTX * ctx, uchar * out,
    const uchar * in, size_t inl);
static const EVP_CIPHER n_cipher = {
	NID_undef,
	1, 0, 0, 0,
	EVP_ORIG_GLOBAL,
	null_init_key,
	null_cipher,
	NULL,
	0,
	NULL,
	NULL,
	NULL,
	NULL
};

const EVP_CIPHER * EVP_enc_null(void)
{
	return &n_cipher;
}

static int null_init_key(EVP_CIPHER_CTX * ctx, const uchar * key,
    const uchar * iv, int enc)
{
	return 1;
}

static int null_cipher(EVP_CIPHER_CTX * ctx, uchar * out,
    const uchar * in, size_t inl)
{
	if(in != out)
		memcpy(out, in, inl);
	return 1;
}
