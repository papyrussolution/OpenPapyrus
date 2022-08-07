/*
 * Copyright 2007-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * Copyright (c) 2007 KISA(Korea Information Security Agency). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of author nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 */
#ifndef OSSL_CRYPTO_SEED_LOCAL_H
#define OSSL_CRYPTO_SEED_LOCAL_H

#include <openssl/e_os2.h>
#include <openssl/seed.h>

#ifdef SEED_LONG               /* need 32-bit type */
typedef unsigned long seed_word;
#else
typedef unsigned int seed_word;
#endif


#define char2word(c, i)  \
        (i) = ((((seed_word)(c)[0]) << 24) | (((seed_word)(c)[1]) << 16) | (((seed_word)(c)[2]) << 8) | ((seed_word)(c)[3]))

#define word2char(l, c)  \
        *((c)+0) = (uchar)((l)>>24) & 0xff; \
        *((c)+1) = (uchar)((l)>>16) & 0xff; \
        *((c)+2) = (uchar)((l)>> 8) & 0xff; \
        *((c)+3) = (uchar)((l))     & 0xff

#define KEYSCHEDULE_UPDATE0(T0, T1, X1, X2, X3, X4, KC)  \
        (T0) = (X3);                                     \
        (X3) = (((X3)<<8) ^ ((X4)>>24)) & 0xffffffff;    \
        (X4) = (((X4)<<8) ^ ((T0)>>24)) & 0xffffffff;    \
        (T0) = ((X1) + (X3) - (KC))     & 0xffffffff;    \
        (T1) = ((X2) + (KC) - (X4))     & 0xffffffff

#define KEYSCHEDULE_UPDATE1(T0, T1, X1, X2, X3, X4, KC)  \
        (T0) = (X1);                                     \
        (X1) = (((X1)>>8) ^ ((X2)<<24)) & 0xffffffff;    \
        (X2) = (((X2)>>8) ^ ((T0)<<24)) & 0xffffffff;    \
        (T0) = ((X1) + (X3) - (KC))     & 0xffffffff;     \
        (T1) = ((X2) + (KC) - (X4))     & 0xffffffff

#define KEYUPDATE_TEMP(T0, T1, K)   \
        (K)[0] = G_FUNC((T0));      \
        (K)[1] = G_FUNC((T1))

#define XOR_SEEDBLOCK(DST, SRC)      \
        ((DST))[0] ^= ((SRC))[0];    \
        ((DST))[1] ^= ((SRC))[1];    \
        ((DST))[2] ^= ((SRC))[2];    \
        ((DST))[3] ^= ((SRC))[3]

#define MOV_SEEDBLOCK(DST, SRC)      \
        ((DST))[0] = ((SRC))[0];     \
        ((DST))[1] = ((SRC))[1];     \
        ((DST))[2] = ((SRC))[2];     \
        ((DST))[3] = ((SRC))[3]

#define CHAR2WORD(C, I)              \
        char2word((C),    (I)[0]);    \
        char2word((C+4),  (I)[1]);    \
        char2word((C+8),  (I)[2]);    \
        char2word((C+12), (I)[3])

#define WORD2CHAR(I, C)              \
        word2char((I)[0], (C));       \
        word2char((I)[1], (C+4));     \
        word2char((I)[2], (C+8));     \
        word2char((I)[3], (C+12))

#define E_SEED(T0, T1, X1, X2, X3, X4, rbase)   \
        (T0) = (X3) ^ (ks->data)[(rbase)];       \
        (T1) = (X4) ^ (ks->data)[(rbase)+1];     \
        (T1) ^= (T0);                            \
        (T1) = G_FUNC((T1));                     \
        (T0) = ((T0) + (T1)) & 0xffffffff;       \
        (T0) = G_FUNC((T0));                     \
        (T1) = ((T1) + (T0)) & 0xffffffff;       \
        (T1) = G_FUNC((T1));                     \
        (T0) = ((T0) + (T1)) & 0xffffffff;       \
        (X1) ^= (T0);                            \
        (X2) ^= (T1)

#endif                          /* OSSL_CRYPTO_SEED_LOCAL_H */
