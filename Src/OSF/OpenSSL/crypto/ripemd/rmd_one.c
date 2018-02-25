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
//#include <openssl/ripemd.h>

uchar * RIPEMD160(const uchar * d, size_t n, uchar * md)
{
	RIPEMD160_CTX c;
	static uchar m[RIPEMD160_DIGEST_LENGTH];
	SETIFZ(md, m);
	if(!RIPEMD160_Init(&c))
		return NULL;
	else {
		RIPEMD160_Update(&c, d, n);
		RIPEMD160_Final(md, &c);
		OPENSSL_cleanse(&c, sizeof(c)); /* security consideration */
		return (md);
	}
}

