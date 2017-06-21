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
#include <openssl/idea.h>
#include "idea_lcl.h"
/*
 * The input and output encrypted as though 64bit cfb mode is being used.
 * The extra state information to record how much of the 64bit block we have
 * used is contained in *num;
 */

void IDEA_cfb64_encrypt(const uchar * in, uchar * out, long length, IDEA_KEY_SCHEDULE * schedule, uchar * ivec, int * num, int encrypt)
{
	register ulong v0, v1, t;
	register int n = *num;
	register long l = length;
	ulong ti[2];
	uchar c, cc;
	uchar * iv = (uchar*)ivec;
	if(encrypt) {
		while(l--) {
			if(n == 0) {
				n2l(iv, v0);
				ti[0] = v0;
				n2l(iv, v1);
				ti[1] = v1;
				IDEA_encrypt((ulong*)ti, schedule);
				iv = (uchar*)ivec;
				t = ti[0];
				l2n(t, iv);
				t = ti[1];
				l2n(t, iv);
				iv = (uchar*)ivec;
			}
			c = *(in++) ^ iv[n];
			*(out++) = c;
			iv[n] = c;
			n = (n + 1) & 0x07;
		}
	}
	else {
		while(l--) {
			if(n == 0) {
				n2l(iv, v0);
				ti[0] = v0;
				n2l(iv, v1);
				ti[1] = v1;
				IDEA_encrypt((ulong*)ti, schedule);
				iv = (uchar*)ivec;
				t = ti[0];
				l2n(t, iv);
				t = ti[1];
				l2n(t, iv);
				iv = (uchar*)ivec;
			}
			cc = *(in++);
			c = iv[n];
			iv[n] = cc;
			*(out++) = c ^ cc;
			n = (n + 1) & 0x07;
		}
	}
	v0 = v1 = ti[0] = ti[1] = t = c = cc = 0;
	*num = n;
}

