/*
 * Copyright 2008-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop
//#include <openssl/aes.h>
//#include <openssl/modes.h>

int AES_wrap_key(AES_KEY * key, const uchar * iv, uchar * out, const uchar * in, uint inlen)
{
	return CRYPTO_128_wrap(key, iv, out, in, inlen, (block128_f)AES_encrypt);
}

int AES_unwrap_key(AES_KEY * key, const uchar * iv, uchar * out, const uchar * in, uint inlen)
{
	return CRYPTO_128_unwrap(key, iv, out, in, inlen, (block128_f)AES_decrypt);
}
