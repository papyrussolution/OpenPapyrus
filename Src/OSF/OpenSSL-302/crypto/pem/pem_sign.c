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
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

int PEM_SignInit(EVP_MD_CTX * ctx, EVP_MD * type)
{
	return EVP_DigestInit_ex(ctx, type, NULL);
}

int PEM_SignUpdate(EVP_MD_CTX * ctx,
    const uchar * data, unsigned int count)
{
	return EVP_DigestUpdate(ctx, data, count);
}

int PEM_SignFinal(EVP_MD_CTX * ctx, uchar * sigret, unsigned int * siglen, EVP_PKEY * pkey)
{
	uchar * m;
	int i, ret = 0;
	unsigned int m_len;
	m = (uchar *)OPENSSL_malloc(EVP_PKEY_get_size(pkey));
	if(m == NULL) {
		ERR_raise(ERR_LIB_PEM, ERR_R_MALLOC_FAILURE);
		goto err;
	}

	if(EVP_SignFinal(ctx, m, &m_len, pkey) <= 0)
		goto err;

	i = EVP_EncodeBlock(sigret, m, m_len);
	*siglen = i;
	ret = 1;
err:
	/* ctx has been zeroed by EVP_SignFinal() */
	OPENSSL_free(m);
	return ret;
}
