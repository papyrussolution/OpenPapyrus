/*
 * Copyright 2008-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * AES_encrypt/AES_decrypt are deprecated - but we need to use them to implement these functions
 */
int AES_wrap_key(AES_KEY * key, const uchar * iv, uchar * out, const uchar * in, unsigned int inlen)
{
	return CRYPTO_128_wrap(key, iv, out, in, inlen, (block128_f)AES_encrypt);
}

int AES_unwrap_key(AES_KEY * key, const uchar * iv, uchar * out, const uchar * in, unsigned int inlen)
{
	return CRYPTO_128_unwrap(key, iv, out, in, inlen, (block128_f)AES_decrypt);
}
