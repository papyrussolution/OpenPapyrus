/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_BF_LOCL_H
# define HEADER_BF_LOCL_H
# include <openssl/opensslconf.h>

/* NOTE - c is not incremented as per n2l */
# define n2ln(c,l1,l2,n) { \
                        c+=n; \
                        l1=l2=0; \
                        switch (n) { \
                        case 8: l2 =((ulong)(*(--(c))))    ; \
                        case 7: l2|=((ulong)(*(--(c))))<< 8; \
                        case 6: l2|=((ulong)(*(--(c))))<<16; \
                        case 5: l2|=((ulong)(*(--(c))))<<24; \
                        case 4: l1 =((ulong)(*(--(c))))    ; \
                        case 3: l1|=((ulong)(*(--(c))))<< 8; \
                        case 2: l1|=((ulong)(*(--(c))))<<16; \
                        case 1: l1|=((ulong)(*(--(c))))<<24; \
                                } \
                        }

/* NOTE - c is not incremented as per l2n */
# define l2nn(l1,l2,c,n) { \
                        c+=n; \
                        switch (n) { \
                        case 8: *(--(c))=(uchar)(((l2)    )&0xff); \
                        case 7: *(--(c))=(uchar)(((l2)>> 8)&0xff); \
                        case 6: *(--(c))=(uchar)(((l2)>>16)&0xff); \
                        case 5: *(--(c))=(uchar)(((l2)>>24)&0xff); \
                        case 4: *(--(c))=(uchar)(((l1)    )&0xff); \
                        case 3: *(--(c))=(uchar)(((l1)>> 8)&0xff); \
                        case 2: *(--(c))=(uchar)(((l1)>>16)&0xff); \
                        case 1: *(--(c))=(uchar)(((l1)>>24)&0xff); \
                                } \
                        }

# undef n2l
# define n2l(c,l)        (l =((ulong)(*((c)++)))<<24L, \
                         l|=((ulong)(*((c)++)))<<16L, \
                         l|=((ulong)(*((c)++)))<< 8L, \
                         l|=((ulong)(*((c)++))))

# undef l2n
# define l2n(l,c)        (*((c)++)=(uchar)(((l)>>24L)&0xff), \
                         *((c)++)=(uchar)(((l)>>16L)&0xff), \
                         *((c)++)=(uchar)(((l)>> 8L)&0xff), \
                         *((c)++)=(uchar)(((l)     )&0xff))

/*
 * This is actually a big endian algorithm, the most significant byte is used
 * to lookup array 0
 */

# define BF_ENC(LL,R,S,P) ( \
        LL^=P, \
        LL^=((( S[       ((R>>24)&0xff)] + \
                S[0x0100+((R>>16)&0xff)])^ \
                S[0x0200+((R>> 8)&0xff)])+ \
                S[0x0300+((R    )&0xff)])&0xffffffffU \
        )

#endif
