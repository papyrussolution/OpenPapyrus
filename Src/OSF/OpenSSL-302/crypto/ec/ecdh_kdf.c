/*
 * Copyright 2015-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * ECDH low level APIs are deprecated for public use, but still ok for internal use.
 */
#include "ec_local.h"

/* Key derivation function from X9.63/SECG */
int ossl_ecdh_kdf_X9_63(uchar * out, size_t outlen, const uchar * Z, size_t Zlen,
    const uchar * sinfo, size_t sinfolen, const EVP_MD * md, OSSL_LIB_CTX * libctx, const char * propq)
{
	int ret = 0;
	EVP_KDF_CTX * kctx = NULL;
	OSSL_PARAM params[4], * p = params;
	const char * mdname = EVP_MD_get0_name(md);
	EVP_KDF * kdf = EVP_KDF_fetch(libctx, OSSL_KDF_NAME_X963KDF, propq);
	if((kctx = EVP_KDF_CTX_new(kdf)) != NULL) {
		*p++ = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST,
			(char*)mdname, 0);
		*p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY,
			(void*)Z, Zlen);
		*p++ = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO,
			(void*)sinfo, sinfolen);
		*p = OSSL_PARAM_construct_end();

		ret = EVP_KDF_derive(kctx, out, outlen, params) > 0;
		EVP_KDF_CTX_free(kctx);
	}
	EVP_KDF_free(kdf);
	return ret;
}

/*-
 * The old name for ecdh_KDF_X9_63
 * Retained for ABI compatibility
 */
#ifndef OPENSSL_NO_DEPRECATED_3_0
int ECDH_KDF_X9_62(uchar * out, size_t outlen,
    const uchar * Z, size_t Zlen,
    const uchar * sinfo, size_t sinfolen,
    const EVP_MD * md)
{
	return ossl_ecdh_kdf_X9_63(out, outlen, Z, Zlen, sinfo, sinfolen, md, NULL, NULL);
}

#endif
