/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef ARCHIVE_OPENSSL_EVP_PRIVATE_H_INCLUDED
#define ARCHIVE_OPENSSL_EVP_PRIVATE_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

#include <openssl/evp.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
//#include <stdlib.h> /* malloc, free */
//#include <string.h> /* memset */
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
