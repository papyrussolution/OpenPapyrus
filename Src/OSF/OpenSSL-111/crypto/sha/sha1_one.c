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
//#include <openssl/crypto.h>
#include <openssl/sha.h>

uchar * SHA1(const uchar * d, size_t n, uchar * md)
{
	SHA_CTX c;
	static uchar m[SHA_DIGEST_LENGTH];
	if(md == NULL)
		md = m;
	if(!SHA1_Init(&c))
		return NULL;
	SHA1_Update(&c, d, n);
	SHA1_Final(md, &c);
	OPENSSL_cleanse(&c, sizeof(c));
	return md;
}
