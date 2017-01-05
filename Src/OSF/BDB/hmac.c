/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * Some parts of this code originally written by Adam Stubblefield,
 * -- astubble@rice.edu.
 *
 * $Id$
 */
#include "db_config.h"
#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/crypto.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#pragma hdrstop
#include "dbinc/hmac.h"
#include "dbinc/log.h"

#define HMAC_OUTPUT_SIZE        20
#define HMAC_BLOCK_SIZE 64

static void __db_hmac __P((uint8*, uint8*, size_t, uint8 *));

/*
 * !!!
 * All of these functions use a ctx structure on the stack.  The __db_SHA1Init
 * call does not initialize the 64-byte buffer portion of it.  The
 * underlying SHA1 functions will properly pad the buffer if the data length
 * is less than 64-bytes, so there isn't a chance of reading uninitialized
 * memory.  Although it would be cleaner to do a memset(ctx.buffer, 0, 64)
 * we do not want to incur that penalty if we don't have to for performance.
 */

/*
 * __db_hmac --
 *	Do a hashed MAC.
 */
static void __db_hmac(uint8 * k, uint8 * data, size_t data_len, uint8 * mac)
{
	SHA1_CTX ctx;
	uint8 key[HMAC_BLOCK_SIZE];
	uint8 ipad[HMAC_BLOCK_SIZE];
	uint8 opad[HMAC_BLOCK_SIZE];
	uint8 tmp[HMAC_OUTPUT_SIZE];
	int i;
	memset(key,  0x00, HMAC_BLOCK_SIZE);
	memset(ipad, 0x36, HMAC_BLOCK_SIZE);
	memset(opad, 0x5C, HMAC_BLOCK_SIZE);
	memcpy(key, k, HMAC_OUTPUT_SIZE);
	for(i = 0; i < HMAC_BLOCK_SIZE; i++) {
		ipad[i] ^= key[i];
		opad[i] ^= key[i];
	}
	__db_SHA1Init(&ctx);
	__db_SHA1Update(&ctx, ipad, HMAC_BLOCK_SIZE);
	__db_SHA1Update(&ctx, data, data_len);
	__db_SHA1Final(tmp, &ctx);
	__db_SHA1Init(&ctx);
	__db_SHA1Update(&ctx, opad, HMAC_BLOCK_SIZE);
	__db_SHA1Update(&ctx, tmp, HMAC_OUTPUT_SIZE);
	__db_SHA1Final(mac, &ctx);
	return;
}
/*
 * __db_chksum --
 *	Create a MAC/SHA1 checksum.
 *
 * PUBLIC: void __db_chksum __P((void *,
 * PUBLIC:     uint8 *, size_t, uint8 *, uint8 *));
 */
void __db_chksum(void * hdr, uint8 * data, size_t data_len, uint8 * mac_key, uint8 * store)
{
	int sumlen;
	uint32 hash4;
	/*
	 * Since the checksum might be on a page of data we are checksumming
	 * we might be overwriting after checksumming, we zero-out the
	 * checksum value so that we can have a known value there when
	 * we verify the checksum.
	 * If we are passed a log header XOR in prev and len so we have
	 * some redundancy on these fields.  Mostly we need to be sure that
	 * we detect a race when doing hot backups and reading a live log
	 * file.
	 */
	if(mac_key == NULL)
		sumlen = sizeof(uint32);
	else
		sumlen = DB_MAC_KEY;
	if(hdr == NULL)
		memzero(store, sumlen);
	else
		store = ((HDR *)hdr)->chksum;
	if(mac_key == NULL) {
		/* Just a hash, no MAC */
		hash4 = __ham_func4(NULL, data, (uint32)data_len);
		if(hdr != NULL)
			hash4 ^= ((HDR *)hdr)->prev^((HDR *)hdr)->len;
		memcpy(store, &hash4, sumlen);
	}
	else {
		__db_hmac(mac_key, data, data_len, store);
		if(hdr != 0) {
			((int *)store)[0] ^= ((HDR *)hdr)->prev;
			((int *)store)[1] ^= ((HDR *)hdr)->len;
		}
	}
	return;
}
/*
 * __db_derive_mac --
 *	Create a MAC/SHA1 key.
 *
 * PUBLIC: void __db_derive_mac __P((uint8 *, size_t, uint8 *));
 */
void __db_derive_mac(uint8 * passwd, size_t plen, uint8 * mac_key)
{
	SHA1_CTX ctx;
	/* Compute the MAC key. mac_key must be 20 bytes. */
	__db_SHA1Init(&ctx);
	__db_SHA1Update(&ctx, passwd, plen);
	__db_SHA1Update(&ctx, (uint8 *)DB_MAC_MAGIC, strlen(DB_MAC_MAGIC));
	__db_SHA1Update(&ctx, passwd, plen);
	__db_SHA1Final(mac_key, &ctx);
	return;
}
/*
 * __db_check_chksum --
 *	Verify a checksum.
 *
 *	Return 0 on success, >0 (errno) on error, -1 on checksum mismatch.
 *
 * PUBLIC: int __db_check_chksum __P((ENV *,
 * PUBLIC:     void *, DB_CIPHER *, uint8 *, void *, size_t, int));
 */
int __db_check_chksum(ENV * env, void * hdr, DB_CIPHER * db_cipher, uint8 * chksum, void * data, size_t data_len, int is_hmac)
{
	int ret;
	size_t sum_len;
	uint32 hash4;
	uint8 * mac_key, old[DB_MAC_KEY], new_key[DB_MAC_KEY];
	/*
	 * If we are just doing checksumming and not encryption, then checksum
	 * is 4 bytes.  Otherwise, it is DB_MAC_KEY size.  Check for illegal
	 * combinations of crypto/non-crypto checksums.
	 */
	if(is_hmac == 0) {
		if(db_cipher != NULL) {
			__db_errx(env, DB_STR("0195", "Unencrypted checksum with a supplied encryption key"));
			return EINVAL;
		}
		sum_len = sizeof(uint32);
		mac_key = NULL;
	}
	else {
		if(db_cipher == NULL) {
			__db_errx(env, DB_STR("0196", "Encrypted checksum: no encryption key specified"));
			return EINVAL;
		}
		sum_len = DB_MAC_KEY;
		mac_key = db_cipher->mac_key;
	}
	/*
	 * !!!
	 * Since the checksum might be on the page, we need to have known data
	 * there so that we can generate the same original checksum.  We zero
	 * it out, just like we do in __db_chksum above.
	 * If there is a log header, XOR the prev and len fields.
	 */
retry:
	if(hdr == NULL) {
		memcpy(old, chksum, sum_len);
		memzero(chksum, sum_len);
		chksum = old;
	}
	if(mac_key == NULL) {
		/* Just a hash, no MAC */
		hash4 = __ham_func4(NULL, data, (uint32)data_len);
		if(hdr != NULL)
			LOG_HDR_SUM(0, hdr, &hash4);
		ret = memcmp((uint32 *)chksum, &hash4, sum_len) ? -1 : 0;
	}
	else {
		__db_hmac(mac_key, (uint8 *)data, data_len, new_key);
		if(hdr != NULL)
			LOG_HDR_SUM(1, hdr, new_key);
		ret = memcmp(chksum, new_key, sum_len) ? -1 : 0;
	}
	/*
	 * !!!
	 * We might be looking at an old log even with the new_key
	 * code.  So, if we have a hdr, and the checksum doesn't
	 * match, try again without a hdr.
	 */
	if(hdr != NULL && ret != 0) {
		hdr = NULL;
		goto retry;
	}
	return ret;
}
