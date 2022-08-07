/*
 * Copyright 2002-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * AES_encrypt is deprecated - but we need to use it to implement AES_ofb128_encrypt
 */
void AES_ofb128_encrypt(const uchar * in, uchar * out, size_t length, const AES_KEY * key, uchar * ivec, int * num)
{
	CRYPTO_ofb128_encrypt(in, out, length, key, ivec, num, (block128_f)AES_encrypt);
}
