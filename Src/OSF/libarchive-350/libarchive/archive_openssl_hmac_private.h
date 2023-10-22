/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 */
#ifndef ARCHIVE_OPENSSL_HMAC_PRIVATE_H_INCLUDED
#define ARCHIVE_OPENSSL_HMAC_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

#include <slib-ossl.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
	static inline HMAC_CTX *HMAC_CTX_new(void)
	{
		HMAC_CTX *ctx = (HMAC_CTX *)SAlloc::C(1, sizeof(HMAC_CTX));
		return ctx;
	}

	static inline void HMAC_CTX_free(HMAC_CTX *ctx)
	{
		HMAC_CTX_cleanup(ctx);
		memzero(ctx, sizeof(*ctx));
		SAlloc::F(ctx);
	}
#endif

#endif
