/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * MD2 low level APIs are deprecated for public use, but still ok for internal use.
 */
uchar * MDC2(const uchar * d, size_t n, uchar * md)
{
	MDC2_CTX c;
	static unsigned char m[MDC2_DIGEST_LENGTH];
	if(!md)
		md = m;
	if(!MDC2_Init(&c))
		return NULL;
	MDC2_Update(&c, d, n);
	MDC2_Final(md, &c);
	OPENSSL_cleanse(&c, sizeof(c)); /* security consideration */
	return md;
}
