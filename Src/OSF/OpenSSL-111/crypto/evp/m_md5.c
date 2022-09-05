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

#ifndef OPENSSL_NO_MD5

#include <openssl/md5.h>

static int init(EVP_MD_CTX * ctx)
{
	return MD5_Init(static_cast<MD5_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static int update(EVP_MD_CTX * ctx, const void * data, size_t count)
{
	return MD5_Update(static_cast<MD5_CTX *>(EVP_MD_CTX_md_data(ctx)), data, count);
}

static int final(EVP_MD_CTX * ctx, uchar * md)
{
	return MD5_Final(md, static_cast<MD5_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static const EVP_MD md5_md = {
	NID_md5,
	NID_md5WithRSAEncryption,
	MD5_DIGEST_LENGTH,
	0,
	init,
	update,
	final,
	NULL,
	NULL,
	MD5_CBLOCK,
	sizeof(EVP_MD *) + sizeof(MD5_CTX),
};

const EVP_MD * EVP_md5(void)
{
	return &md5_md;
}

#endif
