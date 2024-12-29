/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.
   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

#ifndef __CYRUS_MD5_H
#define __CYRUS_MD5_H

typedef struct {
	UINT4 state[4]; // state (ABCD) 
	UINT4 count[2]; // number of bits, modulo 2^64 (lsb first) 
	uchar buffer[64]; // input buffer 
} Cyrus_MD5_CTX;

#ifdef __cplusplus
extern "C" {
#endif

void _sasl_MD5Init(Cyrus_MD5_CTX *);
void _sasl_MD5Update(Cyrus_MD5_CTX *, const uchar *, uint);
void _sasl_MD5Final(uchar [16], Cyrus_MD5_CTX *);

#ifdef __cplusplus
}
#endif

#endif // __CYRUS_MD5_H
