/*
 * Copyright (C) 2013-2015 Marc Hoersken <info@marc-hoersken.de> All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and the
 * following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names
 * of any other contributors may be used to endorse or
 * promote products derived from this software without
 * specific prior written permission.
 */
#include "libssh2_priv.h"
#pragma hdrstop

#ifdef LIBSSH2_WINCNG /* compile only if we build with wincng */
/* required for cross-compilation against the w64 mingw-runtime package */
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600)
	#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0600
#endif
/* specify the required libraries for dependencies using MSVC */
#ifdef _MSC_VER
	#pragma comment(lib, "bcrypt.lib")
	#ifdef HAVE_LIBCRYPT32
		#pragma comment(lib, "crypt32.lib")
	#endif
#endif
#include <windows.h>
#include <bcrypt.h>
#include <math.h>
#ifdef HAVE_STDLIB_H
	#include <stdlib.h>
#endif
#ifdef HAVE_LIBCRYPT32
	#include <wincrypt.h>
#endif
#define PEM_RSA_HEADER "-----BEGIN RSA PRIVATE KEY-----"
#define PEM_RSA_FOOTER "-----END RSA PRIVATE KEY-----"
#define PEM_DSA_HEADER "-----BEGIN DSA PRIVATE KEY-----"
#define PEM_DSA_FOOTER "-----END DSA PRIVATE KEY-----"

/*******************************************************************/
/*
 * Windows CNG backend: Missing definitions (for MinGW[-w64])
 */
#ifndef BCRYPT_SUCCESS
	#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
#ifndef BCRYPT_RNG_ALGORITHM
	#define BCRYPT_RNG_ALGORITHM L"RNG"
#endif
#ifndef BCRYPT_MD5_ALGORITHM
	#define BCRYPT_MD5_ALGORITHM L"MD5"
#endif
#ifndef BCRYPT_SHA1_ALGORITHM
	#define BCRYPT_SHA1_ALGORITHM L"SHA1"
#endif
#ifndef BCRYPT_SHA256_ALGORITHM
	#define BCRYPT_SHA256_ALGORITHM L"SHA256"
#endif
#ifndef BCRYPT_SHA512_ALGORITHM
	#define BCRYPT_SHA512_ALGORITHM L"SHA512"
#endif
#ifndef BCRYPT_RSA_ALGORITHM
	#define BCRYPT_RSA_ALGORITHM L"RSA"
#endif
#ifndef BCRYPT_DSA_ALGORITHM
	#define BCRYPT_DSA_ALGORITHM L"DSA"
#endif
#ifndef BCRYPT_AES_ALGORITHM
	#define BCRYPT_AES_ALGORITHM L"AES"
#endif
#ifndef BCRYPT_RC4_ALGORITHM
	#define BCRYPT_RC4_ALGORITHM L"RC4"
#endif
#ifndef BCRYPT_3DES_ALGORITHM
	#define BCRYPT_3DES_ALGORITHM L"3DES"
#endif
#ifndef BCRYPT_ALG_HANDLE_HMAC_FLAG
	#define BCRYPT_ALG_HANDLE_HMAC_FLAG 0x00000008
#endif
#ifndef BCRYPT_DSA_PUBLIC_BLOB
	#define BCRYPT_DSA_PUBLIC_BLOB L"DSAPUBLICBLOB"
#endif
#ifndef BCRYPT_DSA_PUBLIC_MAGIC
	#define BCRYPT_DSA_PUBLIC_MAGIC 0x42505344 /* DSPB */
#endif
#ifndef BCRYPT_DSA_PRIVATE_BLOB
	#define BCRYPT_DSA_PRIVATE_BLOB L"DSAPRIVATEBLOB"
#endif
#ifndef BCRYPT_DSA_PRIVATE_MAGIC
	#define BCRYPT_DSA_PRIVATE_MAGIC 0x56505344 /* DSPV */
#endif
#ifndef BCRYPT_RSAPUBLIC_BLOB
	#define BCRYPT_RSAPUBLIC_BLOB L"RSAPUBLICBLOB"
#endif
#ifndef BCRYPT_RSAPUBLIC_MAGIC
	#define BCRYPT_RSAPUBLIC_MAGIC 0x31415352 /* RSA1 */
#endif
#ifndef BCRYPT_RSAFULLPRIVATE_BLOB
	#define BCRYPT_RSAFULLPRIVATE_BLOB L"RSAFULLPRIVATEBLOB"
#endif
#ifndef BCRYPT_RSAFULLPRIVATE_MAGIC
	#define BCRYPT_RSAFULLPRIVATE_MAGIC 0x33415352 /* RSA3 */
#endif
#ifndef BCRYPT_KEY_DATA_BLOB
	#define BCRYPT_KEY_DATA_BLOB L"KeyDataBlob"
#endif
#ifndef BCRYPT_MESSAGE_BLOCK_LENGTH
	#define BCRYPT_MESSAGE_BLOCK_LENGTH L"MessageBlockLength"
#endif

#ifndef BCRYPT_NO_KEY_VALIDATION
#define BCRYPT_NO_KEY_VALIDATION 0x00000008
#endif

#ifndef BCRYPT_BLOCK_PADDING
#define BCRYPT_BLOCK_PADDING 0x00000001
#endif

#ifndef BCRYPT_PAD_NONE
#define BCRYPT_PAD_NONE 0x00000001
#endif

#ifndef BCRYPT_PAD_PKCS1
#define BCRYPT_PAD_PKCS1 0x00000002
#endif

#ifndef BCRYPT_PAD_OAEP
#define BCRYPT_PAD_OAEP 0x00000004
#endif

#ifndef BCRYPT_PAD_PSS
#define BCRYPT_PAD_PSS 0x00000008
#endif

#ifndef CRYPT_STRING_ANY
#define CRYPT_STRING_ANY 0x00000007
#endif

#ifndef LEGACY_RSAPRIVATE_BLOB
#define LEGACY_RSAPRIVATE_BLOB L"CAPIPRIVATEBLOB"
#endif

#ifndef PKCS_RSA_PRIVATE_KEY
#define PKCS_RSA_PRIVATE_KEY (LPCSTR)43
#endif

/*******************************************************************/
/*
 * Windows CNG backend: Generic functions
 */

void _libssh2_wincng_init(void)
{
	int ret;
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRNG, BCRYPT_RNG_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashMD5, BCRYPT_MD5_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA1, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA256, BCRYPT_SHA256_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHashSHA512, BCRYPT_SHA512_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacMD5, BCRYPT_MD5_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA1, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA256, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgHmacSHA512, BCRYPT_SHA512_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRSA, BCRYPT_RSA_ALGORITHM, NULL, 0);
	(void)BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgDSA, BCRYPT_DSA_ALGORITHM, NULL, 0);
	ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgAES_CBC, BCRYPT_AES_ALGORITHM, NULL, 0);
	if(BCRYPT_SUCCESS(ret)) {
		ret = BCryptSetProperty(_libssh2_wincng.hAlgAES_CBC, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC,
		    sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
		if(!BCRYPT_SUCCESS(ret)) {
			(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgAES_CBC, 0);
		}
	}

	ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlgRC4_NA,
	    BCRYPT_RC4_ALGORITHM, NULL, 0);
	if(BCRYPT_SUCCESS(ret)) {
		ret = BCryptSetProperty(_libssh2_wincng.hAlgRC4_NA, BCRYPT_CHAINING_MODE,
		    (PBYTE)BCRYPT_CHAIN_MODE_NA,
		    sizeof(BCRYPT_CHAIN_MODE_NA), 0);
		if(!BCRYPT_SUCCESS(ret)) {
			(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRC4_NA, 0);
		}
	}

	ret = BCryptOpenAlgorithmProvider(&_libssh2_wincng.hAlg3DES_CBC,
	    BCRYPT_3DES_ALGORITHM, NULL, 0);
	if(BCRYPT_SUCCESS(ret)) {
		ret = BCryptSetProperty(_libssh2_wincng.hAlg3DES_CBC, BCRYPT_CHAINING_MODE,
		    (PBYTE)BCRYPT_CHAIN_MODE_CBC,
		    sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
		if(!BCRYPT_SUCCESS(ret)) {
			(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlg3DES_CBC, 0);
		}
	}
}

void _libssh2_wincng_free(void)
{
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRNG, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashMD5, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA1, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA256, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHashSHA512, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacMD5, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA1, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA256, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgHmacSHA512, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRSA, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgDSA, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgAES_CBC, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlgRC4_NA, 0);
	(void)BCryptCloseAlgorithmProvider(_libssh2_wincng.hAlg3DES_CBC, 0);
	memzero(&_libssh2_wincng, sizeof(_libssh2_wincng));
}

int _libssh2_wincng_random(void * buf, int len)
{
	int ret = BCryptGenRandom(_libssh2_wincng.hAlgRNG, (uchar *)buf, len, 0);
	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

static void _libssh2_wincng_safe_free(void * buf, int len)
{
#ifndef LIBSSH2_CLEAR_MEMORY
	(void)len;
#endif
	if(!buf)
		return;
#ifdef LIBSSH2_CLEAR_MEMORY
	if(len > 0)
		SecureZeroMemory(buf, len);
#endif
	SAlloc::F(buf);
}

/*******************************************************************/
/*
 * Windows CNG backend: Hash functions
 */

int _libssh2_wincng_hash_init(_libssh2_wincng_hash_ctx * ctx,
    BCRYPT_ALG_HANDLE hAlg, ulong hashlen,
    uchar * key, ulong keylen)
{
	BCRYPT_HASH_HANDLE hHash;
	uchar * pbHashObject;
	ulong dwHashObject, dwHash, cbData;
	int ret;
	ret = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (uchar *)&dwHash, sizeof(dwHash), &cbData, 0);
	if((!BCRYPT_SUCCESS(ret)) || dwHash != hashlen) {
		return -1;
	}
	ret = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (uchar *)&dwHashObject, sizeof(dwHashObject), &cbData, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		return -1;
	}
	pbHashObject = (uchar *)SAlloc::M(dwHashObject);
	if(!pbHashObject) {
		return -1;
	}
	ret = BCryptCreateHash(hAlg, &hHash, pbHashObject, dwHashObject, key, keylen, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		_libssh2_wincng_safe_free(pbHashObject, dwHashObject);
		return -1;
	}
	ctx->hHash = hHash;
	ctx->pbHashObject = pbHashObject;
	ctx->dwHashObject = dwHashObject;
	ctx->cbHash = dwHash;
	return 0;
}

int _libssh2_wincng_hash_update(_libssh2_wincng_hash_ctx * ctx, const uchar * data, ulong datalen)
{
	int ret = BCryptHashData(ctx->hHash, (uchar *)data, datalen, 0);
	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int _libssh2_wincng_hash_final(_libssh2_wincng_hash_ctx * ctx,
    uchar * hash)
{
	int ret;

	ret = BCryptFinishHash(ctx->hHash, hash, ctx->cbHash, 0);

	BCryptDestroyHash(ctx->hHash);
	ctx->hHash = NULL;

	_libssh2_wincng_safe_free(ctx->pbHashObject, ctx->dwHashObject);
	ctx->pbHashObject = NULL;
	ctx->dwHashObject = 0;

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int _libssh2_wincng_hash(uchar * data, ulong datalen, BCRYPT_ALG_HANDLE hAlg, uchar * hash, ulong hashlen)
{
	_libssh2_wincng_hash_ctx ctx;
	int ret = _libssh2_wincng_hash_init(&ctx, hAlg, hashlen, NULL, 0);
	if(!ret) {
		ret = _libssh2_wincng_hash_update(&ctx, data, datalen);
		ret |= _libssh2_wincng_hash_final(&ctx, hash);
	}

	return ret;
}

/*******************************************************************/
/*
 * Windows CNG backend: HMAC functions
 */

int _libssh2_wincng_hmac_final(_libssh2_wincng_hash_ctx * ctx,
    uchar * hash)
{
	int ret;

	ret = BCryptFinishHash(ctx->hHash, hash, ctx->cbHash, 0);

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void _libssh2_wincng_hmac_cleanup(_libssh2_wincng_hash_ctx * ctx)
{
	BCryptDestroyHash(ctx->hHash);
	ctx->hHash = NULL;

	_libssh2_wincng_safe_free(ctx->pbHashObject, ctx->dwHashObject);
	ctx->pbHashObject = NULL;
	ctx->dwHashObject = 0;
}

/*******************************************************************/
/*
 * Windows CNG backend: Key functions
 */

int _libssh2_wincng_key_sha1_verify(_libssh2_wincng_key_ctx * ctx,
    const uchar * sig,
    ulong sig_len,
    const uchar * m,
    ulong m_len,
    ulong flags)
{
	BCRYPT_PKCS1_PADDING_INFO paddingInfoPKCS1;
	void * pPaddingInfo;
	uchar * data, * hash;
	ulong datalen, hashlen;
	int ret;
	datalen = m_len;
	data = (uchar *)SAlloc::M(datalen);
	if(!data) {
		return -1;
	}
	hashlen = SHA_DIGEST_LENGTH;
	hash = (uchar *)SAlloc::M(hashlen);
	if(!hash) {
		SAlloc::F(data);
		return -1;
	}
	memcpy(data, m, datalen);
	ret = _libssh2_wincng_hash(data, datalen, _libssh2_wincng.hAlgHashSHA1, hash, hashlen);
	_libssh2_wincng_safe_free(data, datalen);
	if(ret) {
		_libssh2_wincng_safe_free(hash, hashlen);
		return -1;
	}
	datalen = sig_len;
	data = (uchar *)SAlloc::M(datalen);
	if(!data) {
		_libssh2_wincng_safe_free(hash, hashlen);
		return -1;
	}
	if(flags & BCRYPT_PAD_PKCS1) {
		paddingInfoPKCS1.pszAlgId = BCRYPT_SHA1_ALGORITHM;
		pPaddingInfo = &paddingInfoPKCS1;
	}
	else
		pPaddingInfo = NULL;

	memcpy(data, sig, datalen);

	ret = BCryptVerifySignature(ctx->hKey, pPaddingInfo,
	    hash, hashlen, data, datalen, flags);

	_libssh2_wincng_safe_free(hash, hashlen);
	_libssh2_wincng_safe_free(data, datalen);

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

#ifdef HAVE_LIBCRYPT32
static int _libssh2_wincng_load_pem(LIBSSH2_SESSION * session,
    const char * filename,
    const char * passphrase,
    const char * headerbegin,
    const char * headerend,
    uchar ** data,
    uint * datalen)
{
	FILE * fp;
	int ret;

	(void)passphrase;

	fp = fopen(filename, "r");
	if(!fp) {
		return -1;
	}

	ret = _libssh2_pem_parse(session, headerbegin, headerend,
	    fp, data, datalen);

	fclose(fp);

	return ret;
}

static int _libssh2_wincng_load_private(LIBSSH2_SESSION * session,
    const char * filename,
    const char * passphrase,
    uchar ** ppbEncoded,
    ulong * pcbEncoded,
    int tryLoadRSA, int tryLoadDSA)
{
	uchar * data = NULL;
	uint datalen = 0;
	int ret = -1;

	if(ret && tryLoadRSA) {
		ret = _libssh2_wincng_load_pem(session, filename, passphrase,
		    PEM_RSA_HEADER, PEM_RSA_FOOTER,
		    &data, &datalen);
	}

	if(ret && tryLoadDSA) {
		ret = _libssh2_wincng_load_pem(session, filename, passphrase,
		    PEM_DSA_HEADER, PEM_DSA_FOOTER,
		    &data, &datalen);
	}

	if(!ret) {
		*ppbEncoded = data;
		*pcbEncoded = datalen;
	}

	return ret;
}

static int _libssh2_wincng_load_private_memory(LIBSSH2_SESSION * session,
    const char * privatekeydata,
    size_t privatekeydata_len,
    const char * passphrase,
    uchar ** ppbEncoded,
    ulong * pcbEncoded,
    int tryLoadRSA, int tryLoadDSA)
{
	uchar * data = NULL;
	uint datalen = 0;
	int ret = -1;

	(void)passphrase;

	if(ret && tryLoadRSA) {
		ret = _libssh2_pem_parse_memory(session,
		    PEM_RSA_HEADER, PEM_RSA_FOOTER,
		    privatekeydata, privatekeydata_len,
		    &data, &datalen);
	}

	if(ret && tryLoadDSA) {
		ret = _libssh2_pem_parse_memory(session,
		    PEM_DSA_HEADER, PEM_DSA_FOOTER,
		    privatekeydata, privatekeydata_len,
		    &data, &datalen);
	}

	if(!ret) {
		*ppbEncoded = data;
		*pcbEncoded = datalen;
	}
	return ret;
}

static int _libssh2_wincng_asn_decode(uchar * pbEncoded, ulong cbEncoded,
    LPCSTR lpszStructType, uchar ** ppbDecoded, ulong * pcbDecoded)
{
	uchar * pbDecoded = NULL;
	ulong cbDecoded = 0;
	int ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, lpszStructType, pbEncoded, cbEncoded, 0, NULL, NULL, &cbDecoded);
	if(!ret) {
		return -1;
	}
	pbDecoded = (uchar *)SAlloc::M(cbDecoded);
	if(!pbDecoded) {
		return -1;
	}
	ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, lpszStructType, pbEncoded, cbEncoded, 0, NULL, pbDecoded, &cbDecoded);
	if(!ret) {
		_libssh2_wincng_safe_free(pbDecoded, cbDecoded);
		return -1;
	}
	*ppbDecoded = pbDecoded;
	*pcbDecoded = cbDecoded;
	return 0;
}

static int _libssh2_wincng_bn_ltob(uchar * pbInput,
    ulong cbInput,
    uchar ** ppbOutput,
    ulong * pcbOutput)
{
	uchar * pbOutput;
	ulong cbOutput, index, offset, length;

	if(cbInput < 1) {
		return 0;
	}

	offset = 0;
	length = cbInput - 1;
	cbOutput = cbInput;
	if(pbInput[length] & (1 << 7)) {
		offset++;
		cbOutput += offset;
	}
	pbOutput = (uchar *)SAlloc::M(cbOutput);
	if(!pbOutput) {
		return -1;
	}
	pbOutput[0] = 0;
	for(index = 0; ((index + offset) < cbOutput) && (index < cbInput); index++) {
		pbOutput[index + offset] = pbInput[length - index];
	}
	*ppbOutput = pbOutput;
	*pcbOutput = cbOutput;
	return 0;
}

static int _libssh2_wincng_asn_decode_bn(uchar * pbEncoded,
    ulong cbEncoded,
    uchar ** ppbDecoded,
    ulong * pcbDecoded)
{
	uchar * pbDecoded = NULL, * pbInteger;
	ulong cbDecoded = 0, cbInteger;
	int ret;

	ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded,
	    X509_MULTI_BYTE_UINT,
	    &pbInteger, &cbInteger);
	if(!ret) {
		ret = _libssh2_wincng_bn_ltob(((PCRYPT_DATA_BLOB)pbInteger)->pbData,
		    ((PCRYPT_DATA_BLOB)pbInteger)->cbData,
		    &pbDecoded, &cbDecoded);
		if(!ret) {
			*ppbDecoded = pbDecoded;
			*pcbDecoded = cbDecoded;
		}
		_libssh2_wincng_safe_free(pbInteger, cbInteger);
	}

	return ret;
}

static int _libssh2_wincng_asn_decode_bns(uchar * pbEncoded,
    ulong cbEncoded,
    uchar *** prpbDecoded,
    ulong ** prcbDecoded,
    ulong * pcbCount)
{
	PCRYPT_DER_BLOB pBlob;
	uchar * pbDecoded;
	uchar ** rpbDecoded;
	ulong cbDecoded, * rcbDecoded, index, length;
	int ret;
	ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded, X509_SEQUENCE_OF_ANY, &pbDecoded, &cbDecoded);
	if(!ret) {
		length = ((PCRYPT_DATA_BLOB)pbDecoded)->cbData;
		rpbDecoded = (uchar **)SAlloc::M(sizeof(PBYTE) * length);
		if(rpbDecoded) {
			rcbDecoded = (ulong *)SAlloc::M(sizeof(DWORD) * length);
			if(rcbDecoded) {
				for(index = 0; index < length; index++) {
					pBlob = &((PCRYPT_DER_BLOB)((PCRYPT_DATA_BLOB)pbDecoded)->pbData)[index];
					ret = _libssh2_wincng_asn_decode_bn(pBlob->pbData, pBlob->cbData, &rpbDecoded[index], &rcbDecoded[index]);
					if(ret)
						break;
				}
				if(!ret) {
					*prpbDecoded = rpbDecoded;
					*prcbDecoded = rcbDecoded;
					*pcbCount = length;
				}
				else {
					for(length = 0; length < index; length++) {
						_libssh2_wincng_safe_free(rpbDecoded[length], rcbDecoded[length]);
						rpbDecoded[length] = NULL;
						rcbDecoded[length] = 0;
					}
					SAlloc::F(rpbDecoded);
					SAlloc::F(rcbDecoded);
				}
			}
			else {
				SAlloc::F(rpbDecoded);
				ret = -1;
			}
		}
		else {
			ret = -1;
		}
		_libssh2_wincng_safe_free(pbDecoded, cbDecoded);
	}

	return ret;
}

#endif /* HAVE_LIBCRYPT32 */

static ulong _libssh2_wincng_bn_size(const uchar * bignum,
    ulong length)
{
	ulong offset;

	if(!bignum)
		return 0;

	length--;

	offset = 0;
	while(!(*(bignum + offset)) && (offset < length))
		offset++;

	length++;

	return length - offset;
}

/*******************************************************************/
/*
 * Windows CNG backend: RSA functions
 */

int _libssh2_wincng_rsa_new(libssh2_rsa_ctx ** rsa,
    const uchar * edata,
    ulong elen,
    const uchar * ndata,
    ulong nlen,
    const uchar * ddata,
    ulong dlen,
    const uchar * pdata,
    ulong plen,
    const uchar * qdata,
    ulong qlen,
    const uchar * e1data,
    ulong e1len,
    const uchar * e2data,
    ulong e2len,
    const uchar * coeffdata,
    ulong coefflen)
{
	BCRYPT_KEY_HANDLE hKey;
	BCRYPT_RSAKEY_BLOB * rsakey;
	LPCWSTR lpszBlobType;
	uchar * key;
	ulong keylen, offset, mlen, p1len = 0, p2len = 0;
	int ret;

	mlen = max(_libssh2_wincng_bn_size(ndata, nlen),
	    _libssh2_wincng_bn_size(ddata, dlen));
	offset = sizeof(BCRYPT_RSAKEY_BLOB);
	keylen = offset + elen + mlen;
	if(ddata && dlen > 0) {
		p1len = max(_libssh2_wincng_bn_size(pdata, plen), _libssh2_wincng_bn_size(e1data, e1len));
		p2len = max(_libssh2_wincng_bn_size(qdata, qlen), _libssh2_wincng_bn_size(e2data, e2len));
		keylen += p1len * 3 + p2len * 2 + mlen;
	}
	key = (uchar *)SAlloc::M(keylen);
	if(!key) {
		return -1;
	}
	memzero(key, keylen);

	/* https://msdn.microsoft.com/library/windows/desktop/aa375531.aspx */
	rsakey = (BCRYPT_RSAKEY_BLOB*)key;
	rsakey->BitLength = mlen * 8;
	rsakey->cbPublicExp = elen;
	rsakey->cbModulus = mlen;
	memcpy(key + offset, edata, elen);
	offset += elen;
	if(nlen < mlen)
		memcpy(key + offset + mlen - nlen, ndata, nlen);
	else
		memcpy(key + offset, ndata + nlen - mlen, mlen);
	if(ddata && dlen > 0) {
		offset += mlen;
		if(plen < p1len)
			memcpy(key + offset + p1len - plen, pdata, plen);
		else
			memcpy(key + offset, pdata + plen - p1len, p1len);
		offset += p1len;
		if(qlen < p2len)
			memcpy(key + offset + p2len - qlen, qdata, qlen);
		else
			memcpy(key + offset, qdata + qlen - p2len, p2len);
		offset += p2len;
		if(e1len < p1len)
			memcpy(key + offset + p1len - e1len, e1data, e1len);
		else
			memcpy(key + offset, e1data + e1len - p1len, p1len);
		offset += p1len;
		if(e2len < p2len)
			memcpy(key + offset + p2len - e2len, e2data, e2len);
		else
			memcpy(key + offset, e2data + e2len - p2len, p2len);
		offset += p2len;
		if(coefflen < p1len)
			memcpy(key + offset + p1len - coefflen, coeffdata, coefflen);
		else
			memcpy(key + offset, coeffdata + coefflen - p1len, p1len);
		offset += p1len;
		if(dlen < mlen)
			memcpy(key + offset + mlen - dlen, ddata, dlen);
		else
			memcpy(key + offset, ddata + dlen - mlen, mlen);
		lpszBlobType = BCRYPT_RSAFULLPRIVATE_BLOB;
		rsakey->Magic = BCRYPT_RSAFULLPRIVATE_MAGIC;
		rsakey->cbPrime1 = p1len;
		rsakey->cbPrime2 = p2len;
	}
	else {
		lpszBlobType = BCRYPT_RSAPUBLIC_BLOB;
		rsakey->Magic = BCRYPT_RSAPUBLIC_MAGIC;
		rsakey->cbPrime1 = 0;
		rsakey->cbPrime2 = 0;
	}
	ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL, lpszBlobType, &hKey, key, keylen, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		_libssh2_wincng_safe_free(key, keylen);
		return -1;
	}
	*rsa = (libssh2_rsa_ctx *)SAlloc::M(sizeof(libssh2_rsa_ctx));
	if(!(*rsa)) {
		BCryptDestroyKey(hKey);
		_libssh2_wincng_safe_free(key, keylen);
		return -1;
	}
	(*rsa)->hKey = hKey;
	(*rsa)->pbKeyObject = key;
	(*rsa)->cbKeyObject = keylen;
	return 0;
}

#ifdef HAVE_LIBCRYPT32
static int _libssh2_wincng_rsa_new_private_parse(libssh2_rsa_ctx ** rsa, LIBSSH2_SESSION * session, uchar * pbEncoded, ulong cbEncoded)
{
	BCRYPT_KEY_HANDLE hKey;
	uchar * pbStructInfo;
	ulong cbStructInfo;
	int ret;
	(void)session;
	ret = _libssh2_wincng_asn_decode(pbEncoded, cbEncoded, PKCS_RSA_PRIVATE_KEY, &pbStructInfo, &cbStructInfo);
	_libssh2_wincng_safe_free(pbEncoded, cbEncoded);
	if(ret) {
		return -1;
	}
	ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL, LEGACY_RSAPRIVATE_BLOB, &hKey, pbStructInfo, cbStructInfo, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		_libssh2_wincng_safe_free(pbStructInfo, cbStructInfo);
		return -1;
	}
	*rsa = (libssh2_rsa_ctx *)SAlloc::M(sizeof(libssh2_rsa_ctx));
	if(!(*rsa)) {
		BCryptDestroyKey(hKey);
		_libssh2_wincng_safe_free(pbStructInfo, cbStructInfo);
		return -1;
	}
	(*rsa)->hKey = hKey;
	(*rsa)->pbKeyObject = pbStructInfo;
	(*rsa)->cbKeyObject = cbStructInfo;
	return 0;
}

#endif /* HAVE_LIBCRYPT32 */

int _libssh2_wincng_rsa_new_private(libssh2_rsa_ctx ** rsa, LIBSSH2_SESSION * session, const char * filename, const uchar * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;
	(void)session;
	ret = _libssh2_wincng_load_private(session, filename, (const char *)passphrase, &pbEncoded, &cbEncoded, 1, 0);
	if(ret) {
		return -1;
	}
	return _libssh2_wincng_rsa_new_private_parse(rsa, session, pbEncoded, cbEncoded);
#else
	(void)rsa;
	(void)filename;
	(void)passphrase;
	return _libssh2_error(session, LIBSSH2_ERROR_FILE, "Unable to load RSA key from private key file: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int _libssh2_wincng_rsa_new_private_frommemory(libssh2_rsa_ctx ** rsa, LIBSSH2_SESSION * session,
    const char * filedata, size_t filedata_len, const uchar * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;
	(void)session;
	ret = _libssh2_wincng_load_private_memory(session, filedata, filedata_len, (const char *)passphrase, &pbEncoded, &cbEncoded, 1, 0);
	if(ret) {
		return -1;
	}
	return _libssh2_wincng_rsa_new_private_parse(rsa, session, pbEncoded, cbEncoded);
#else
	(void)rsa;
	(void)filedata;
	(void)filedata_len;
	(void)passphrase;
	return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, "Unable to extract private key from memory: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int _libssh2_wincng_rsa_sha1_verify(libssh2_rsa_ctx * rsa, const uchar * sig, ulong sig_len, const uchar * m, ulong m_len)
{
	return _libssh2_wincng_key_sha1_verify(rsa, sig, sig_len, m, m_len, BCRYPT_PAD_PKCS1);
}

int _libssh2_wincng_rsa_sha1_sign(LIBSSH2_SESSION * session,
    libssh2_rsa_ctx * rsa,
    const uchar * hash,
    size_t hash_len,
    uchar ** signature,
    size_t * signature_len)
{
	BCRYPT_PKCS1_PADDING_INFO paddingInfo;
	uchar * data, * sig;
	ulong cbData, datalen, siglen;
	int ret;
	datalen = (ulong)hash_len;
	data = (uchar *)SAlloc::M(datalen);
	if(!data) {
		return -1;
	}
	paddingInfo.pszAlgId = BCRYPT_SHA1_ALGORITHM;
	memcpy(data, hash, datalen);
	ret = BCryptSignHash(rsa->hKey, &paddingInfo, data, datalen, NULL, 0, &cbData, BCRYPT_PAD_PKCS1);
	if(BCRYPT_SUCCESS(ret)) {
		siglen = cbData;
		sig = (uchar *)LIBSSH2_ALLOC(session, siglen);
		if(sig) {
			ret = BCryptSignHash(rsa->hKey, &paddingInfo, data, datalen, sig, siglen, &cbData, BCRYPT_PAD_PKCS1);
			if(BCRYPT_SUCCESS(ret)) {
				*signature_len = siglen;
				*signature = sig;
			}
			else {
				LIBSSH2_FREE(session, sig);
			}
		}
		else
			ret = STATUS_NO_MEMORY;
	}

	_libssh2_wincng_safe_free(data, datalen);

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void _libssh2_wincng_rsa_free(libssh2_rsa_ctx * rsa)
{
	if(!rsa)
		return;

	BCryptDestroyKey(rsa->hKey);
	rsa->hKey = NULL;

	_libssh2_wincng_safe_free(rsa->pbKeyObject, rsa->cbKeyObject);
	_libssh2_wincng_safe_free(rsa, sizeof(libssh2_rsa_ctx));
}

/*******************************************************************/
/*
 * Windows CNG backend: DSA functions
 */

#if LIBSSH2_DSA
int _libssh2_wincng_dsa_new(libssh2_dsa_ctx ** dsa,
    const uchar * pdata,
    ulong plen,
    const uchar * qdata,
    ulong qlen,
    const uchar * gdata,
    ulong glen,
    const uchar * ydata,
    ulong ylen,
    const uchar * xdata,
    ulong xlen)
{
	BCRYPT_KEY_HANDLE hKey;
	BCRYPT_DSA_KEY_BLOB * dsakey;
	LPCWSTR lpszBlobType;
	uchar * key;
	ulong keylen, offset, length;
	int ret;
	length = max(max(_libssh2_wincng_bn_size(pdata, plen), _libssh2_wincng_bn_size(gdata, glen)), _libssh2_wincng_bn_size(ydata, ylen));
	offset = sizeof(BCRYPT_DSA_KEY_BLOB);
	keylen = offset + length * 3;
	if(xdata && xlen > 0)
		keylen += 20;
	key = (uchar *)SAlloc::M(keylen);
	if(!key) {
		return -1;
	}
	memzero(key, keylen);
	/* https://msdn.microsoft.com/library/windows/desktop/aa833126.aspx */
	dsakey = (BCRYPT_DSA_KEY_BLOB*)key;
	dsakey->cbKey = length;
	memset(dsakey->Count, -1, sizeof(dsakey->Count));
	memset(dsakey->Seed, -1, sizeof(dsakey->Seed));
	if(qlen < 20)
		memcpy(dsakey->q + 20 - qlen, qdata, qlen);
	else
		memcpy(dsakey->q, qdata + qlen - 20, 20);
	if(plen < length)
		memcpy(key + offset + length - plen, pdata, plen);
	else
		memcpy(key + offset, pdata + plen - length, length);
	offset += length;
	if(glen < length)
		memcpy(key + offset + length - glen, gdata, glen);
	else
		memcpy(key + offset, gdata + glen - length, length);
	offset += length;
	if(ylen < length)
		memcpy(key + offset + length - ylen, ydata, ylen);
	else
		memcpy(key + offset, ydata + ylen - length, length);
	if(xdata && xlen > 0) {
		offset += length;
		if(xlen < 20)
			memcpy(key + offset + 20 - xlen, xdata, xlen);
		else
			memcpy(key + offset, xdata + xlen - 20, 20);
		lpszBlobType = BCRYPT_DSA_PRIVATE_BLOB;
		dsakey->dwMagic = BCRYPT_DSA_PRIVATE_MAGIC;
	}
	else {
		lpszBlobType = BCRYPT_DSA_PUBLIC_BLOB;
		dsakey->dwMagic = BCRYPT_DSA_PUBLIC_MAGIC;
	}
	ret = BCryptImportKeyPair(_libssh2_wincng.hAlgDSA, NULL, lpszBlobType, &hKey, key, keylen, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		_libssh2_wincng_safe_free(key, keylen);
		return -1;
	}
	*dsa = (libssh2_rsa_ctx *)SAlloc::M(sizeof(libssh2_dsa_ctx));
	if(!(*dsa)) {
		BCryptDestroyKey(hKey);
		_libssh2_wincng_safe_free(key, keylen);
		return -1;
	}
	(*dsa)->hKey = hKey;
	(*dsa)->pbKeyObject = key;
	(*dsa)->cbKeyObject = keylen;
	return 0;
}

#ifdef HAVE_LIBCRYPT32
static int _libssh2_wincng_dsa_new_private_parse(libssh2_dsa_ctx ** dsa, LIBSSH2_SESSION * session, uchar * pbEncoded, ulong cbEncoded)
{
	uchar ** rpbDecoded;
	ulong * rcbDecoded, index, length;
	int ret;
	(void)session;
	ret = _libssh2_wincng_asn_decode_bns(pbEncoded, cbEncoded, &rpbDecoded, &rcbDecoded, &length);
	_libssh2_wincng_safe_free(pbEncoded, cbEncoded);
	if(ret) {
		return -1;
	}
	if(length == 6) {
		ret = _libssh2_wincng_dsa_new(dsa, rpbDecoded[1], rcbDecoded[1], rpbDecoded[2], rcbDecoded[2], rpbDecoded[3], 
			rcbDecoded[3], rpbDecoded[4], rcbDecoded[4], rpbDecoded[5], rcbDecoded[5]);
	}
	else {
		ret = -1;
	}
	for(index = 0; index < length; index++) {
		_libssh2_wincng_safe_free(rpbDecoded[index], rcbDecoded[index]);
		rpbDecoded[index] = NULL;
		rcbDecoded[index] = 0;
	}
	SAlloc::F(rpbDecoded);
	SAlloc::F(rcbDecoded);
	return ret;
}

#endif /* HAVE_LIBCRYPT32 */

int _libssh2_wincng_dsa_new_private(libssh2_dsa_ctx ** dsa,
    LIBSSH2_SESSION * session,
    const char * filename,
    const uchar * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;

	ret = _libssh2_wincng_load_private(session, filename,
	    (const char *)passphrase,
	    &pbEncoded, &cbEncoded, 0, 1);
	if(ret) {
		return -1;
	}

	return _libssh2_wincng_dsa_new_private_parse(dsa, session,
	    pbEncoded, cbEncoded);
#else
	(void)dsa;
	(void)filename;
	(void)passphrase;
	return _libssh2_error(session, LIBSSH2_ERROR_FILE, "Unable to load DSA key from private key file: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int _libssh2_wincng_dsa_new_private_frommemory(libssh2_dsa_ctx ** dsa,
    LIBSSH2_SESSION * session,
    const char * filedata,
    size_t filedata_len,
    const uchar * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;

	ret = _libssh2_wincng_load_private_memory(session, filedata, filedata_len,
	    (const char *)passphrase,
	    &pbEncoded, &cbEncoded, 0, 1);
	if(ret) {
		return -1;
	}

	return _libssh2_wincng_dsa_new_private_parse(dsa, session,
	    pbEncoded, cbEncoded);
#else
	(void)dsa;
	(void)filedata;
	(void)filedata_len;
	(void)passphrase;

	return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, "Unable to extract private key from memory: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int _libssh2_wincng_dsa_sha1_verify(libssh2_dsa_ctx * dsa,
    const uchar * sig_fixed,
    const uchar * m,
    ulong m_len)
{
	return _libssh2_wincng_key_sha1_verify(dsa, sig_fixed, 40, m, m_len, 0);
}

int _libssh2_wincng_dsa_sha1_sign(libssh2_dsa_ctx * dsa, const uchar * hash, ulong hash_len, uchar * sig_fixed)
{
	uchar * data, * sig;
	ulong cbData, datalen, siglen;
	int ret;
	datalen = hash_len;
	data = (uchar *)SAlloc::M(datalen);
	if(!data) {
		return -1;
	}
	memcpy(data, hash, datalen);
	ret = BCryptSignHash(dsa->hKey, NULL, data, datalen, NULL, 0, &cbData, 0);
	if(BCRYPT_SUCCESS(ret)) {
		siglen = cbData;
		if(siglen == 40) {
			sig = (uchar *)SAlloc::M(siglen);
			if(sig) {
				ret = BCryptSignHash(dsa->hKey, NULL, data, datalen, sig, siglen, &cbData, 0);
				if(BCRYPT_SUCCESS(ret)) {
					memcpy(sig_fixed, sig, siglen);
				}
				_libssh2_wincng_safe_free(sig, siglen);
			}
			else
				ret = STATUS_NO_MEMORY;
		}
		else
			ret = STATUS_NO_MEMORY;
	}

	_libssh2_wincng_safe_free(data, datalen);

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void _libssh2_wincng_dsa_free(libssh2_dsa_ctx * dsa)
{
	if(!dsa)
		return;

	BCryptDestroyKey(dsa->hKey);
	dsa->hKey = NULL;

	_libssh2_wincng_safe_free(dsa->pbKeyObject, dsa->cbKeyObject);
	_libssh2_wincng_safe_free(dsa, sizeof(libssh2_dsa_ctx));
}

#endif

/*******************************************************************/
/*
 * Windows CNG backend: Key functions
 */

#ifdef HAVE_LIBCRYPT32
static ulong _libssh2_wincng_pub_priv_write(uchar * key,
    ulong offset,
    const uchar * bignum,
    const ulong length)
{
	_libssh2_htonu32(key + offset, length);
	offset += 4;

	memcpy(key + offset, bignum, length);
	offset += length;

	return offset;
}

static int _libssh2_wincng_pub_priv_keyfile_parse(LIBSSH2_SESSION * session,
    uchar ** method,
    size_t * method_len,
    uchar ** pubkeydata,
    size_t * pubkeydata_len,
    uchar * pbEncoded,
    ulong cbEncoded)
{
	uchar ** rpbDecoded;
	ulong * rcbDecoded;
	uchar * key = NULL, * mth = NULL;
	ulong keylen = 0, mthlen = 0;
	ulong index, offset, length;
	int ret;

	ret = _libssh2_wincng_asn_decode_bns(pbEncoded, cbEncoded,
	    &rpbDecoded, &rcbDecoded, &length);

	_libssh2_wincng_safe_free(pbEncoded, cbEncoded);

	if(ret) {
		return -1;
	}

	if(length == 9) { /* private RSA key */
		mthlen = 7;
		mth = (uchar *)LIBSSH2_ALLOC(session, mthlen);
		if(mth) {
			memcpy(mth, "ssh-rsa", mthlen);
		}
		else {
			ret = -1;
		}
		keylen = 4 + mthlen + 4 + rcbDecoded[2] + 4 + rcbDecoded[1];
		key = (uchar *)LIBSSH2_ALLOC(session, keylen);
		if(key) {
			offset = _libssh2_wincng_pub_priv_write(key, 0, mth, mthlen);
			offset = _libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[2], rcbDecoded[2]);
			_libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[1], rcbDecoded[1]);
		}
		else {
			ret = -1;
		}
	}
	else if(length == 6) { /* private DSA key */
		mthlen = 7;
		mth = (uchar *)LIBSSH2_ALLOC(session, mthlen);
		if(mth) {
			memcpy(mth, "ssh-dss", mthlen);
		}
		else {
			ret = -1;
		}
		keylen = 4 + mthlen + 4 + rcbDecoded[1] + 4 + rcbDecoded[2] + 4 + rcbDecoded[3] + 4 + rcbDecoded[4];
		key = (uchar *)LIBSSH2_ALLOC(session, keylen);
		if(key) {
			offset = _libssh2_wincng_pub_priv_write(key, 0, mth, mthlen);
			offset = _libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[1], rcbDecoded[1]);
			offset = _libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[2], rcbDecoded[2]);
			offset = _libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[3], rcbDecoded[3]);
			_libssh2_wincng_pub_priv_write(key, offset, rpbDecoded[4], rcbDecoded[4]);
		}
		else {
			ret = -1;
		}
	}
	else {
		ret = -1;
	}

	for(index = 0; index < length; index++) {
		_libssh2_wincng_safe_free(rpbDecoded[index], rcbDecoded[index]);
		rpbDecoded[index] = NULL;
		rcbDecoded[index] = 0;
	}
	SAlloc::F(rpbDecoded);
	SAlloc::F(rcbDecoded);
	if(ret) {
		if(mth)
			LIBSSH2_FREE(session, mth);
		if(key)
			LIBSSH2_FREE(session, key);
	}
	else {
		*method = mth;
		*method_len = mthlen;
		*pubkeydata = key;
		*pubkeydata_len = keylen;
	}
	return ret;
}

#endif /* HAVE_LIBCRYPT32 */

int _libssh2_wincng_pub_priv_keyfile(LIBSSH2_SESSION * session,
    uchar ** method,
    size_t * method_len,
    uchar ** pubkeydata,
    size_t * pubkeydata_len,
    const char * privatekey,
    const char * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;

	ret = _libssh2_wincng_load_private(session, privatekey, passphrase,
	    &pbEncoded, &cbEncoded, 1, 1);
	if(ret) {
		return -1;
	}

	return _libssh2_wincng_pub_priv_keyfile_parse(session, method, method_len,
	    pubkeydata, pubkeydata_len,
	    pbEncoded, cbEncoded);
#else
	(void)method;
	(void)method_len;
	(void)pubkeydata;
	(void)pubkeydata_len;
	(void)privatekey;
	(void)passphrase;

	return _libssh2_error(session, LIBSSH2_ERROR_FILE, "Unable to load public key from private key file: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

int _libssh2_wincng_pub_priv_keyfilememory(LIBSSH2_SESSION * session,
    uchar ** method,
    size_t * method_len,
    uchar ** pubkeydata,
    size_t * pubkeydata_len,
    const char * privatekeydata,
    size_t privatekeydata_len,
    const char * passphrase)
{
#ifdef HAVE_LIBCRYPT32
	uchar * pbEncoded;
	ulong cbEncoded;
	int ret;

	ret = _libssh2_wincng_load_private_memory(session, privatekeydata,
	    privatekeydata_len, passphrase,
	    &pbEncoded, &cbEncoded, 1, 1);
	if(ret) {
		return -1;
	}

	return _libssh2_wincng_pub_priv_keyfile_parse(session, method, method_len,
	    pubkeydata, pubkeydata_len,
	    pbEncoded, cbEncoded);
#else
	(void)method;
	(void)method_len;
	(void)pubkeydata_len;
	(void)pubkeydata;
	(void)privatekeydata;
	(void)privatekeydata_len;
	(void)passphrase;

	return _libssh2_error(session, LIBSSH2_ERROR_METHOD_NOT_SUPPORTED, "Unable to extract public key from private key in memory: Method unsupported in Windows CNG backend");
#endif /* HAVE_LIBCRYPT32 */
}

/*******************************************************************/
/*
 * Windows CNG backend: Cipher functions
 */

int _libssh2_wincng_cipher_init(_libssh2_cipher_ctx * ctx,
    _libssh2_cipher_type(type),
    uchar * iv,
    uchar * secret,
    int encrypt)
{
	BCRYPT_KEY_HANDLE hKey;
	BCRYPT_KEY_DATA_BLOB_HEADER * header;
	uchar * pbKeyObject, * pbIV, * key;
	ulong dwKeyObject, dwIV, dwBlockLength, cbData, keylen;
	int ret;

	(void)encrypt;

	ret = BCryptGetProperty(*type.phAlg, BCRYPT_OBJECT_LENGTH,
	    (uchar *)&dwKeyObject,
	    sizeof(dwKeyObject),
	    &cbData, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		return -1;
	}

	ret = BCryptGetProperty(*type.phAlg, BCRYPT_BLOCK_LENGTH,
	    (uchar *)&dwBlockLength,
	    sizeof(dwBlockLength),
	    &cbData, 0);
	if(!BCRYPT_SUCCESS(ret)) {
		return -1;
	}
	pbKeyObject = (uchar *)SAlloc::M(dwKeyObject);
	if(!pbKeyObject) {
		return -1;
	}
	keylen = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + type.dwKeyLength;
	key = (uchar *)SAlloc::M(keylen);
	if(!key) {
		SAlloc::F(pbKeyObject);
		return -1;
	}
	header = (BCRYPT_KEY_DATA_BLOB_HEADER*)key;
	header->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
	header->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
	header->cbKeyData = type.dwKeyLength;
	memcpy(key + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), secret, type.dwKeyLength);
	ret = BCryptImportKey(*type.phAlg, NULL, BCRYPT_KEY_DATA_BLOB, &hKey, pbKeyObject, dwKeyObject, key, keylen, 0);
	_libssh2_wincng_safe_free(key, keylen);
	if(!BCRYPT_SUCCESS(ret)) {
		_libssh2_wincng_safe_free(pbKeyObject, dwKeyObject);
		return -1;
	}
	if(type.dwUseIV) {
		pbIV = (uchar *)SAlloc::M(dwBlockLength);
		if(!pbIV) {
			BCryptDestroyKey(hKey);
			_libssh2_wincng_safe_free(pbKeyObject, dwKeyObject);
			return -1;
		}
		dwIV = dwBlockLength;
		memcpy(pbIV, iv, dwIV);
	}
	else {
		pbIV = NULL;
		dwIV = 0;
	}

	ctx->hKey = hKey;
	ctx->pbKeyObject = pbKeyObject;
	ctx->pbIV = pbIV;
	ctx->dwKeyObject = dwKeyObject;
	ctx->dwIV = dwIV;
	ctx->dwBlockLength = dwBlockLength;

	return 0;
}

int _libssh2_wincng_cipher_crypt(_libssh2_cipher_ctx * ctx,
    _libssh2_cipher_type(type),
    int encrypt,
    uchar * block,
    size_t blocklen)
{
	uchar * pbOutput;
	ulong cbOutput, cbInput;
	int ret;
	(void)type;
	cbInput = (ulong)blocklen;
	if(encrypt) {
		ret = BCryptEncrypt(ctx->hKey, block, cbInput, NULL, ctx->pbIV, ctx->dwIV, NULL, 0, &cbOutput, 0);
	}
	else {
		ret = BCryptDecrypt(ctx->hKey, block, cbInput, NULL, ctx->pbIV, ctx->dwIV, NULL, 0, &cbOutput, 0);
	}
	if(BCRYPT_SUCCESS(ret)) {
		pbOutput = (uchar *)SAlloc::M(cbOutput);
		if(pbOutput) {
			if(encrypt) {
				ret = BCryptEncrypt(ctx->hKey, block, cbInput, NULL,
				    ctx->pbIV, ctx->dwIV,
				    pbOutput, cbOutput, &cbOutput, 0);
			}
			else {
				ret = BCryptDecrypt(ctx->hKey, block, cbInput, NULL,
				    ctx->pbIV, ctx->dwIV,
				    pbOutput, cbOutput, &cbOutput, 0);
			}
			if(BCRYPT_SUCCESS(ret)) {
				memcpy(block, pbOutput, cbOutput);
			}

			_libssh2_wincng_safe_free(pbOutput, cbOutput);
		}
		else
			ret = STATUS_NO_MEMORY;
	}

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

void _libssh2_wincng_cipher_dtor(_libssh2_cipher_ctx * ctx)
{
	BCryptDestroyKey(ctx->hKey);
	ctx->hKey = NULL;

	_libssh2_wincng_safe_free(ctx->pbKeyObject, ctx->dwKeyObject);
	ctx->pbKeyObject = NULL;
	ctx->dwKeyObject = 0;

	_libssh2_wincng_safe_free(ctx->pbIV, ctx->dwBlockLength);
	ctx->pbIV = NULL;
	ctx->dwBlockLength = 0;
}

/*******************************************************************/
/*
 * Windows CNG backend: BigNumber functions
 */

_libssh2_bn * _libssh2_wincng_bignum_init(void)
{
	_libssh2_bn * bignum = (_libssh2_bn*)SAlloc::M(sizeof(_libssh2_bn));
	if(bignum) {
		bignum->bignum = NULL;
		bignum->length = 0;
	}
	return bignum;
}

static int _libssh2_wincng_bignum_resize(_libssh2_bn * bn, ulong length)
{
	uchar * bignum;
	if(!bn)
		return -1;
	if(length == bn->length)
		return 0;
#ifdef LIBSSH2_CLEAR_MEMORY
	if(bn->bignum && bn->length > 0 && length < bn->length) {
		SecureZeroMemory(bn->bignum + length, bn->length - length);
	}
#endif
	bignum = (uchar *)SAlloc::R(bn->bignum, length);
	if(!bignum)
		return -1;
	bn->bignum = bignum;
	bn->length = length;
	return 0;
}

int _libssh2_wincng_bignum_rand(_libssh2_bn * rnd, int bits, int top, int bottom)
{
	uchar * bignum;
	ulong length;

	if(!rnd)
		return -1;

	length = (ulong)(ceil((float)bits / 8) * sizeof(uchar));
	if(_libssh2_wincng_bignum_resize(rnd, length))
		return -1;

	bignum = rnd->bignum;

	if(_libssh2_wincng_random(bignum, length))
		return -1;

	/* calculate significant bits in most significant byte */
	bits %= 8;

	/* fill most significant byte with zero padding */
	bignum[0] &= (1 << (8 - bits)) - 1;
	/* set some special last bits in most significant byte */
	if(top == 0)
		bignum[0] |= (1 << (7 - bits));
	else if(top == 1)
		bignum[0] |= (3 << (6 - bits));
	/* make odd by setting first bit in least significant byte */
	if(bottom)
		bignum[length - 1] |= 1;
	return 0;
}

int _libssh2_wincng_bignum_mod_exp(_libssh2_bn * r, _libssh2_bn * a, _libssh2_bn * p, _libssh2_bn * m, _libssh2_bn_ctx * bnctx)
{
	BCRYPT_KEY_HANDLE hKey;
	BCRYPT_RSAKEY_BLOB * rsakey;
	uchar * key, * bignum;
	ulong keylen, offset, length;
	int ret;
	(void)bnctx;
	if(!r || !a || !p || !m)
		return -1;
	offset = sizeof(BCRYPT_RSAKEY_BLOB);
	keylen = offset + p->length + m->length;
	key = (uchar *)SAlloc::M(keylen);
	if(!key)
		return -1;
	/* https://msdn.microsoft.com/library/windows/desktop/aa375531.aspx */
	rsakey = (BCRYPT_RSAKEY_BLOB*)key;
	rsakey->Magic = BCRYPT_RSAPUBLIC_MAGIC;
	rsakey->BitLength = m->length * 8;
	rsakey->cbPublicExp = p->length;
	rsakey->cbModulus = m->length;
	rsakey->cbPrime1 = 0;
	rsakey->cbPrime2 = 0;

	memcpy(key + offset, p->bignum, p->length);
	offset += p->length;

	memcpy(key + offset, m->bignum, m->length);
	ret = BCryptImportKeyPair(_libssh2_wincng.hAlgRSA, NULL, BCRYPT_RSAPUBLIC_BLOB, &hKey, key, keylen, BCRYPT_NO_KEY_VALIDATION);
	if(BCRYPT_SUCCESS(ret)) {
		ret = BCryptEncrypt(hKey, a->bignum, a->length, NULL, NULL, 0, NULL, 0, &length, BCRYPT_PAD_NONE);
		if(BCRYPT_SUCCESS(ret)) {
			if(!_libssh2_wincng_bignum_resize(r, length)) {
				length = max(a->length, length);
				bignum = (uchar *)SAlloc::M(length);
				if(bignum) {
					offset = length - a->length;
					memzero(bignum, offset);
					memcpy(bignum + offset, a->bignum, a->length);
					ret = BCryptEncrypt(hKey, bignum, length, NULL, NULL, 0, r->bignum, r->length, &offset, BCRYPT_PAD_NONE);
					_libssh2_wincng_safe_free(bignum, length);
					if(BCRYPT_SUCCESS(ret)) {
						_libssh2_wincng_bignum_resize(r, offset);
					}
				}
				else
					ret = STATUS_NO_MEMORY;
			}
			else
				ret = STATUS_NO_MEMORY;
		}

		BCryptDestroyKey(hKey);
	}

	_libssh2_wincng_safe_free(key, keylen);

	return BCRYPT_SUCCESS(ret) ? 0 : -1;
}

int _libssh2_wincng_bignum_set_word(_libssh2_bn * bn, ulong word)
{
	ulong offset, number, bits, length;

	if(!bn)
		return -1;

	bits = 0;
	number = word;
	while(number >>= 1)
		bits++;

	length = (ulong)(ceil(((double)(bits + 1)) / 8.0) *
	    sizeof(uchar));
	if(_libssh2_wincng_bignum_resize(bn, length))
		return -1;

	for(offset = 0; offset < length; offset++)
		bn->bignum[offset] = (word >> (offset * 8)) & 0xff;

	return 0;
}

ulong _libssh2_wincng_bignum_bits(const _libssh2_bn * bn)
{
	uchar number;
	ulong offset, length, bits;

	if(!bn)
		return 0;

	length = bn->length - 1;

	offset = 0;
	while(!(*(bn->bignum + offset)) && (offset < length))
		offset++;

	bits = (length - offset) * 8;
	number = bn->bignum[offset];

	while(number >>= 1)
		bits++;

	bits++;

	return bits;
}

void _libssh2_wincng_bignum_from_bin(_libssh2_bn * bn, ulong len, const uchar * bin)
{
	uchar * bignum;
	ulong offset, length, bits;
	if(!bn || !bin || !len)
		return;
	if(_libssh2_wincng_bignum_resize(bn, len))
		return;
	memcpy(bn->bignum, bin, len);
	bits = _libssh2_wincng_bignum_bits(bn);
	length = (ulong)(ceil(((double)bits) / 8.0) * sizeof(uchar));
	offset = bn->length - length;
	if(offset > 0) {
		memmove(bn->bignum, bn->bignum + offset, length);
#ifdef LIBSSH2_CLEAR_MEMORY
		SecureZeroMemory(bn->bignum + length, offset);
#endif
		bignum = (uchar *)SAlloc::R(bn->bignum, length);
		if(bignum) {
			bn->bignum = bignum;
			bn->length = length;
		}
	}
}

void _libssh2_wincng_bignum_to_bin(const _libssh2_bn * bn, uchar * bin)
{
	if(bin && bn && bn->bignum && bn->length > 0) {
		memcpy(bin, bn->bignum, bn->length);
	}
}

void _libssh2_wincng_bignum_free(_libssh2_bn * bn)
{
	if(bn) {
		if(bn->bignum) {
			_libssh2_wincng_safe_free(bn->bignum, bn->length);
			bn->bignum = NULL;
		}
		bn->length = 0;
		_libssh2_wincng_safe_free(bn, sizeof(_libssh2_bn));
	}
}

/*
 * Windows CNG backend: other functions
 */

void _libssh2_init_aes_ctr(void)
{
	/* no implementation */
	(void)0;
}

#endif /* LIBSSH2_WINCNG */
