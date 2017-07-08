/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_MD2_H
# define HEADER_MD2_H

# include <openssl/opensslconf.h>

# ifndef OPENSSL_NO_MD2
# include <stddef.h>
# ifdef  __cplusplus
extern "C" {
# endif

typedef uchar MD2_INT;

# define MD2_DIGEST_LENGTH       16
# define MD2_BLOCK               16

typedef struct MD2state_st {
    uint num;
    uchar data[MD2_BLOCK];
    MD2_INT cksm[MD2_BLOCK];
    MD2_INT state[MD2_BLOCK];
} MD2_CTX;

const char *MD2_options(void);
int MD2_Init(MD2_CTX *c);
int MD2_Update(MD2_CTX *c, const uchar *data, size_t len);
int MD2_Final(uchar *md, MD2_CTX *c);
uchar *MD2(const uchar *d, size_t n, uchar *md);

# ifdef  __cplusplus
}
# endif
# endif

#endif
