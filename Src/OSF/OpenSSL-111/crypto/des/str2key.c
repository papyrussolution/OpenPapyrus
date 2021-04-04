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
//#include <openssl/crypto.h>
#include "des_locl.h"

void DES_string_to_key(const char * str, DES_cblock * key)
{
	DES_key_schedule ks;
	int i, length;
	memzero(key, 8);
	length = strlen(str);
	for(i = 0; i < length; i++) {
		uchar j = str[i];
		if((i % 16) < 8)
			(*key)[i % 8] ^= (j << 1);
		else {
			/* Reverse the bit order 05/05/92 eay */
			j = ((j << 4) & 0xf0) | ((j >> 4) & 0x0f);
			j = ((j << 2) & 0xcc) | ((j >> 2) & 0x33);
			j = ((j << 1) & 0xaa) | ((j >> 1) & 0x55);
			(*key)[7 - (i % 8)] ^= j;
		}
	}
	DES_set_odd_parity(key);
	DES_set_key_unchecked(key, &ks);
	DES_cbc_cksum((const uchar *)str, key, length, &ks, key);
	OPENSSL_cleanse(&ks, sizeof(ks));
	DES_set_odd_parity(key);
}

void DES_string_to_2keys(const char * str, DES_cblock * key1, DES_cblock * key2)
{
	DES_key_schedule ks;
	int i, length;
	memzero(key1, 8);
	memzero(key2, 8);
	length = strlen(str);
	for(i = 0; i < length; i++) {
		uchar j = str[i];
		if((i % 32) < 16) {
			if((i % 16) < 8)
				(*key1)[i % 8] ^= (j << 1);
			else
				(*key2)[i % 8] ^= (j << 1);
		}
		else {
			j = ((j << 4) & 0xf0) | ((j >> 4) & 0x0f);
			j = ((j << 2) & 0xcc) | ((j >> 2) & 0x33);
			j = ((j << 1) & 0xaa) | ((j >> 1) & 0x55);
			if((i % 16) < 8)
				(*key1)[7 - (i % 8)] ^= j;
			else
				(*key2)[7 - (i % 8)] ^= j;
		}
	}
	if(length <= 8)
		memcpy(key2, key1, 8);
	DES_set_odd_parity(key1);
	DES_set_odd_parity(key2);
	DES_set_key_unchecked(key1, &ks);
	DES_cbc_cksum((const uchar *)str, key1, length, &ks, key1);
	DES_set_key_unchecked(key2, &ks);
	DES_cbc_cksum((const uchar *)str, key2, length, &ks, key2);
	OPENSSL_cleanse(&ks, sizeof(ks));
	DES_set_odd_parity(key1);
	DES_set_odd_parity(key2);
}
