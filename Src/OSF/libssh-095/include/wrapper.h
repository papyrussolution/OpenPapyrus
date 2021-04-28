/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef WRAPPER_H_
#define WRAPPER_H_

enum ssh_kdf_digest {
	SSH_KDF_SHA1 = 1,
	SSH_KDF_SHA256,
	SSH_KDF_SHA384,
	SSH_KDF_SHA512
};

enum ssh_hmac_e {
	SSH_HMAC_SHA1 = 1,
	SSH_HMAC_SHA256,
	SSH_HMAC_SHA512,
	SSH_HMAC_MD5,
	SSH_HMAC_AEAD_POLY1305,
	SSH_HMAC_AEAD_GCM
};

enum ssh_des_e {
	SSH_3DES,
	SSH_DES
};

struct ssh_hmac_struct {
	const char* name;
	enum ssh_hmac_e hmac_type;
	bool etm;
};

enum ssh_crypto_direction_e {
	SSH_DIRECTION_IN = 1,
	SSH_DIRECTION_OUT = 2,
	SSH_DIRECTION_BOTH = 3,
};

struct ssh_cipher_struct;
struct ssh_crypto_struct;

typedef struct ssh_mac_ctx_struct * ssh_mac_ctx;
MD5CTX md5_init();
void md5_update(MD5CTX c, const void * data, ulong len);
void md5_final(uchar * md, MD5CTX c);
SHACTX sha1_init();
void sha1_update(SHACTX c, const void * data, ulong len);
void sha1_final(uchar * md, SHACTX c);
void sha1(const uchar * digest, int len, uchar * hash);
SHA256CTX sha256_init();
void sha256_update(SHA256CTX c, const void * data, ulong len);
void sha256_final(uchar * md, SHA256CTX c);
void sha256(const uchar * digest, int len, uchar * hash);
SHA384CTX sha384_init();
void sha384_update(SHA384CTX c, const void * data, ulong len);
void sha384_final(uchar * md, SHA384CTX c);
void sha384(const uchar * digest, int len, uchar * hash);
SHA512CTX sha512_init();
void sha512_update(SHA512CTX c, const void * data, ulong len);
void sha512_final(uchar * md, SHA512CTX c);
void sha512(const uchar * digest, int len, uchar * hash);
void evp(int nid, uchar * digest, int len, uchar * hash, uint * hlen);
EVPCTX evp_init(int nid);
void evp_update(EVPCTX ctx, const void * data, ulong len);
void evp_final(EVPCTX ctx, uchar * md, uint * mdlen);
HMACCTX hmac_init(const void * key, int len, enum ssh_hmac_e type);
void hmac_update(HMACCTX c, const void * data, ulong len);
void hmac_final(HMACCTX ctx, uchar * hashmacbuf, uint * len);
size_t hmac_digest_len(enum ssh_hmac_e type);
int ssh_kdf(struct ssh_crypto_struct * crypto, uchar * key, size_t key_len, int key_type, uchar * output, size_t requested_len);
int crypt_set_algorithms_client(ssh_session session);
int crypt_set_algorithms_server(ssh_session session);
struct ssh_crypto_struct * crypto_new();
void crypto_free(struct ssh_crypto_struct * crypto);
void ssh_reseed();
int ssh_crypto_init();
void ssh_crypto_finalize();
void ssh_cipher_clear(struct ssh_cipher_struct * cipher);
struct ssh_hmac_struct * ssh_get_hmactab();
struct ssh_cipher_struct * ssh_get_ciphertab();
const char * ssh_hmac_type_to_string(enum ssh_hmac_e hmac_type, bool etm);

#endif /* WRAPPER_H_ */
