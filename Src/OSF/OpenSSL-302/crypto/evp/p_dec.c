/*
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include "crypto/evp.h"

int EVP_PKEY_decrypt_old(uchar * key, const uchar * ek, int ekl, EVP_PKEY * priv)
{
	int ret = -1;
	if(EVP_PKEY_get_id(priv) != EVP_PKEY_RSA) {
		ERR_raise(ERR_LIB_EVP, EVP_R_PUBLIC_KEY_NOT_RSA);
		goto err;
	}
	ret = RSA_private_decrypt(ekl, ek, key, evp_pkey_get0_RSA_int(priv), RSA_PKCS1_PADDING);
err:
	return ret;
}
