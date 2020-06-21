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

#ifndef OPENSSL_NO_RMD160

#include <openssl/ripemd.h>
//#include <openssl/evp.h>
//#include <openssl/objects.h>
//#include <openssl/x509.h>
//#include <openssl/rsa.h>
//#include <evp_int.h>

static int init(EVP_MD_CTX * ctx)
{
	return RIPEMD160_Init(static_cast<RIPEMD160_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static int update(EVP_MD_CTX * ctx, const void * data, size_t count)
{
	return RIPEMD160_Update(static_cast<RIPEMD160_CTX *>(EVP_MD_CTX_md_data(ctx)), data, count);
}

static int final(EVP_MD_CTX * ctx, unsigned char * md)
{
	return RIPEMD160_Final(md, static_cast<RIPEMD160_CTX *>(EVP_MD_CTX_md_data(ctx)));
}

static const EVP_MD ripemd160_md = {
	NID_ripemd160,
	NID_ripemd160WithRSA,
	RIPEMD160_DIGEST_LENGTH,
	0,
	init,
	update,
	final,
	NULL,
	NULL,
	RIPEMD160_CBLOCK,
	sizeof(EVP_MD *) + sizeof(RIPEMD160_CTX),
};

const EVP_MD * EVP_ripemd160(void)
{
	return &ripemd160_md;
}

#endif
