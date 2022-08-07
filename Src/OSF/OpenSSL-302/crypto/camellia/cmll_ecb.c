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
#include "cmll_local.h"

void Camellia_ecb_encrypt(const uchar * in, uchar * out, const CAMELLIA_KEY * key, const int enc)
{
	if(CAMELLIA_ENCRYPT == enc)
		Camellia_encrypt(in, out, key);
	else
		Camellia_decrypt(in, out, key);
}
