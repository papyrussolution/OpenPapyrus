/*
 * Copyright 2005-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

#ifndef OPENSSL_NO_WHIRLPOOL

#include <openssl/whrlpool.h>

static int init(EVP_MD_CTX * ctx)
{
	return WHIRLPOOL_Init(static_cast<WHIRLPOOL_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static int update(EVP_MD_CTX * ctx, const void * data, size_t count)
{
	return WHIRLPOOL_Update(static_cast<WHIRLPOOL_CTX *>(EVP_MD_CTX_md_data(ctx)), data, count);
}

static int final(EVP_MD_CTX * ctx, uchar * md)
{
	return WHIRLPOOL_Final(md, static_cast<WHIRLPOOL_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static const EVP_MD whirlpool_md = {
	NID_whirlpool,
	0,
	WHIRLPOOL_DIGEST_LENGTH,
	0,
	init,
	update,
	final,
	NULL,
	NULL,
	WHIRLPOOL_BBLOCK / 8,
	sizeof(EVP_MD *) + sizeof(WHIRLPOOL_CTX),
};

const EVP_MD * EVP_whirlpool(void)
{
	return &whirlpool_md;
}

#endif
