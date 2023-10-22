/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#ifndef ARCHIVE_OPENSSL_EVP_PRIVATE_H_INCLUDED
#define ARCHIVE_OPENSSL_EVP_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif
#include <slib-ossl.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static inline EVP_MD_CTX *EVP_MD_CTX_new(void)
{
	EVP_MD_CTX *ctx = (EVP_MD_CTX *)SAlloc::C(1, sizeof(EVP_MD_CTX));
	return ctx;
}

static inline void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
	EVP_MD_CTX_cleanup(ctx);
	memzero(ctx, sizeof(*ctx));
	SAlloc::F(ctx);
}
#endif
#endif
