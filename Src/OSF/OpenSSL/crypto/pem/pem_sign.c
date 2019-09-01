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

int PEM_SignInit(EVP_MD_CTX * ctx, EVP_MD * type)
{
	return EVP_DigestInit_ex(ctx, type, 0);
}

int PEM_SignUpdate(EVP_MD_CTX * ctx, uchar * data, uint count)
{
	return EVP_DigestUpdate(ctx, data, count);
}

int PEM_SignFinal(EVP_MD_CTX * ctx, uchar * sigret, uint * siglen, EVP_PKEY * pkey)
{
	int i, ret = 0;
	uint m_len;
	uchar * m = (uchar *)OPENSSL_malloc(EVP_PKEY_size(pkey) + 2);
	if(!m) {
		PEMerr(PEM_F_PEM_SIGNFINAL, ERR_R_MALLOC_FAILURE);
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

