/*
 * Copyright 2002-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
#include <openssl/aes.h>
#include <openssl/modes.h>

void AES_cbc_encrypt(const uchar * in, uchar * out, size_t len, const AES_KEY * key, uchar * ivec, const int enc)
{
	if(enc)
		CRYPTO_cbc128_encrypt(in, out, len, key, ivec, (block128_f)AES_encrypt);
	else
		CRYPTO_cbc128_decrypt(in, out, len, key, ivec, (block128_f)AES_decrypt);
}

