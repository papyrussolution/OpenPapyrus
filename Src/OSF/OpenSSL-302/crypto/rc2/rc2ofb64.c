/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
/*
 * RC2 low level APIs are deprecated for public use, but still ok for internal use.
 */
#include <openssl/rc2.h>
#include "rc2_local.h"
/*
 * The input and output encrypted as though 64bit ofb mode is being used.
 * The extra state information to record how much of the 64bit block we have
 * used is contained in *num;
 */
void RC2_ofb64_encrypt(const uchar * in, uchar * out, long length, RC2_KEY * schedule, uchar * ivec, int * num)
{
	unsigned long v0, v1, t;
	int n = *num;
	long l = length;
	unsigned char d[8];
	char * dp;
	unsigned long ti[2];
	uchar * iv;
	int save = 0;
	iv = (uchar *)ivec;
	c2l(iv, v0);
	c2l(iv, v1);
	ti[0] = v0;
	ti[1] = v1;
	dp = (char*)d;
	l2c(v0, dp);
	l2c(v1, dp);
	while(l--) {
		if(n == 0) {
			RC2_encrypt((ulong *)ti, schedule);
			dp = (char*)d;
			t = ti[0];
			l2c(t, dp);
			t = ti[1];
			l2c(t, dp);
			save++;
		}
		*(out++) = *(in++) ^ d[n];
		n = (n + 1) & 0x07;
	}
	if(save) {
		v0 = ti[0];
		v1 = ti[1];
		iv = (uchar *)ivec;
		l2c(v0, iv);
		l2c(v1, iv);
	}
	t = v0 = v1 = ti[0] = ti[1] = 0;
	*num = n;
}
