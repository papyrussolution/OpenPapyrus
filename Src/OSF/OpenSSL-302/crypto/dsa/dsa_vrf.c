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
 * DSA low level APIs are deprecated for public use, but still ok for internal use.
 */
#include "dsa_local.h"

int DSA_do_verify(const uchar * dgst, int dgst_len, DSA_SIG * sig, DSA * dsa)
{
	return dsa->meth->dsa_do_verify(dgst, dgst_len, sig, dsa);
}
