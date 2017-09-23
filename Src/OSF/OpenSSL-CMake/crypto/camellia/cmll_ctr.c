/*
 * Copyright 2006-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
//#include <openssl/camellia.h>
//#include <openssl/modes.h>

void Camellia_ctr128_encrypt(const uchar * in, uchar * out, size_t length, const CAMELLIA_KEY * key,
    uchar ivec[CAMELLIA_BLOCK_SIZE], uchar ecount_buf[CAMELLIA_BLOCK_SIZE], uint * num)
{
	CRYPTO_ctr128_encrypt(in, out, length, key, ivec, ecount_buf, num, (block128_f)Camellia_encrypt);
}
