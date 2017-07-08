/*
 * Copyright 2002-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_AES_H
# define HEADER_AES_H

# include <openssl/opensslconf.h>

# include <stddef.h>
# ifdef  __cplusplus
extern "C" {
# endif

# define AES_ENCRYPT     1
# define AES_DECRYPT     0

/*
 * Because array size can't be a const in C, the following two are macros.
 * Both sizes are in bytes.
 */
# define AES_MAXNR 14
# define AES_BLOCK_SIZE 16

/* This should be a hidden type, but EVP requires that the size be known */
struct aes_key_st {
# ifdef AES_LONG
    unsigned long rd_key[4 * (AES_MAXNR + 1)];
# else
    uint rd_key[4 * (AES_MAXNR + 1)];
# endif
    int rounds;
};
typedef struct aes_key_st AES_KEY;

const char *AES_options(void);

int AES_set_encrypt_key(const uchar *userKey, const int bits,
                        AES_KEY *key);
int AES_set_decrypt_key(const uchar *userKey, const int bits,
                        AES_KEY *key);

void AES_encrypt(const uchar *in, uchar *out,
                 const AES_KEY *key);
void AES_decrypt(const uchar *in, uchar *out,
                 const AES_KEY *key);

void AES_ecb_encrypt(const uchar *in, uchar *out,
                     const AES_KEY *key, const int enc);
void AES_cbc_encrypt(const uchar *in, uchar *out,
                     size_t length, const AES_KEY *key,
                     uchar *ivec, const int enc);
void AES_cfb128_encrypt(const uchar *in, uchar *out,
                        size_t length, const AES_KEY *key,
                        uchar *ivec, int *num, const int enc);
void AES_cfb1_encrypt(const uchar *in, uchar *out,
                      size_t length, const AES_KEY *key,
                      uchar *ivec, int *num, const int enc);
void AES_cfb8_encrypt(const uchar *in, uchar *out, size_t length, const AES_KEY *key, uchar *ivec, int *num, const int enc);
void AES_ofb128_encrypt(const uchar *in, uchar *out, size_t length, const AES_KEY *key, uchar *ivec, int *num);
/* NB: the IV is _two_ blocks long */
void AES_ige_encrypt(const uchar *in, uchar *out, size_t length, const AES_KEY *key, uchar *ivec, const int enc);
/* NB: the IV is _four_ blocks long */
void AES_bi_ige_encrypt(const uchar *in, uchar *out, size_t length, const AES_KEY *key, const AES_KEY *key2, const uchar *ivec, const int enc);
int AES_wrap_key(AES_KEY *key, const uchar *iv, uchar *out, const uchar *in, uint inlen);
int AES_unwrap_key(AES_KEY *key, const uchar *iv, uchar *out, const uchar *in, uint inlen);


# ifdef  __cplusplus
}
# endif

#endif
