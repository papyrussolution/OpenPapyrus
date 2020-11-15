/*
 * Copyright 2017 The OpenSSL Project Authors. All Rights Reserved.
 * Copyright 2017 Ribose Inc. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at  https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
#ifndef OPENSSL_NO_SM3
//#include <openssl/evp.h>
//#include <internal/crypto/evp_int.h>
#include <internal/crypto/sm3.h>

static int init(EVP_MD_CTX * ctx)
{
	return sm3_init(static_cast<SM3_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static int update(EVP_MD_CTX * ctx, const void * data, size_t count)
{
	return sm3_update(static_cast<SM3_CTX *>(EVP_MD_CTX_md_data(ctx)), data, count);
}

static int final(EVP_MD_CTX * ctx, uchar * md)
{
	return sm3_final(md, static_cast<SM3_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static const EVP_MD sm3_md = {
	NID_sm3,
	NID_sm3WithRSAEncryption,
	SM3_DIGEST_LENGTH,
	0,
	init,
	update,
	final,
	NULL,
	NULL,
	SM3_CBLOCK,
	sizeof(EVP_MD *) + sizeof(SM3_CTX),
};

const EVP_MD * EVP_sm3(void)
{
	return &sm3_md;
}

#endif
