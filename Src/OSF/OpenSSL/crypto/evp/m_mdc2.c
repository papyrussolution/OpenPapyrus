/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

#ifndef OPENSSL_NO_MDC2

#include <openssl/mdc2.h>

static int init(EVP_MD_CTX *ctx)
{
    return MDC2_Init((MDC2_CTX *)EVP_MD_CTX_md_data(ctx));
}

static int update(EVP_MD_CTX *ctx, const void *data, size_t count)
{
    return MDC2_Update((MDC2_CTX *)EVP_MD_CTX_md_data(ctx), (const uchar *)data, count);
}

static int final(EVP_MD_CTX *ctx, uchar *md)
{
    return MDC2_Final(md, (MDC2_CTX *)EVP_MD_CTX_md_data(ctx));
}

static const EVP_MD mdc2_md = {
    NID_mdc2,
    NID_mdc2WithRSA,
    MDC2_DIGEST_LENGTH,
    0,
    init,
    update,
    final,
    NULL,
    NULL,
    MDC2_BLOCK,
    sizeof(EVP_MD *) + sizeof(MDC2_CTX),
};

const EVP_MD *EVP_mdc2(void)
{
    return (&mdc2_md);
}
#endif
