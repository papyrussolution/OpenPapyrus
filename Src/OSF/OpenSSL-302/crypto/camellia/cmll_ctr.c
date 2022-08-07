/*
 * Copyright 2006-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * Camellia low level APIs are deprecated for public use, but still ok for internal use.
 */
#include <openssl/camellia.h>

void Camellia_ctr128_encrypt(const uchar * in, uchar * out, size_t length, const CAMELLIA_KEY * key,
    unsigned char ivec[CAMELLIA_BLOCK_SIZE], unsigned char ecount_buf[CAMELLIA_BLOCK_SIZE], unsigned int * num)
{
	CRYPTO_ctr128_encrypt(in, out, length, key, ivec, ecount_buf, num, (block128_f)Camellia_encrypt);
}
