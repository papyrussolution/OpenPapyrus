/*
 * Copyright 2003-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

int OPENSSL_memcmp(const void * v1, const void * v2, size_t n)
{
	const uchar * c1 = (const uchar*)v1, * c2 = (const uchar*)v2;
	int ret = 0;
	while(n && (ret = *c1 - *c2) == 0)
		n--, c1++, c2++;
	return ret;
}

char * CRYPTO_strdup(const char * str, const char* file, int line)
{
	char * ret = 0;
	if(str) {
		size_t size = strlen(str) + 1;
		ret = (char*)CRYPTO_malloc(size, file, line);
		if(ret)
			memcpy(ret, str, size);
	}
	return ret;
}

char * CRYPTO_strndup(const char * str, size_t s, const char* file, int line)
{
	char * ret = NULL;
	if(str) {
		size_t maxlen = OPENSSL_strnlen(str, s);
		ret = (char*)CRYPTO_malloc(maxlen + 1, file, line);
		if(ret) {
			memcpy(ret, str, maxlen);
			ret[maxlen] = '\0';
		}
	}
	return ret;
}

void * CRYPTO_memdup(const void * data, size_t siz, const char* file, int line)
{
	void * ret;
	if(data == NULL || siz >= INT_MAX)
		return NULL;
	ret = CRYPTO_malloc(siz, file, line);
	if(!ret) {
		CRYPTOerr(CRYPTO_F_CRYPTO_MEMDUP, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	return memcpy(ret, data, siz);
}

size_t OPENSSL_strnlen(const char * str, size_t maxlen)
{
	const char * p;
	for(p = str; maxlen-- != 0 && *p != '\0'; ++p) 
		;
	return p - str;
}

size_t OPENSSL_strlcpy(char * dst, const char * src, size_t size)
{
	size_t l = 0;
	for(; size > 1 && *src; size--) {
		*dst++ = *src++;
		l++;
	}
	if(size)
		*dst = '\0';
	return l + strlen(src);
}

size_t OPENSSL_strlcat(char * dst, const char * src, size_t size)
{
	size_t l = 0;
	for(; size > 0 && *dst; size--, dst++)
		l++;
	return l + OPENSSL_strlcpy(dst, src, size);
}

int OPENSSL_hexchar2int(uchar c)
{
#ifdef CHARSET_EBCDIC
	c = os_toebcdic[c];
#endif
	switch(c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': case 'A': return 0x0A;
		case 'b': case 'B': return 0x0B;
		case 'c': case 'C': return 0x0C;
		case 'd': case 'D': return 0x0D;
		case 'e': case 'E': return 0x0E;
		case 'f': case 'F': return 0x0F;
	}
	return -1;
}
/*
 * Give a string of hex digits convert to a buffer
 */
uchar * OPENSSL_hexstr2buf(const char * str, long * len)
{
	uchar * hexbuf, * q;
	uchar ch, cl;
	int chi, cli;
	const uchar * p;
	size_t s = strlen(str);
	if((hexbuf = (uchar*)OPENSSL_malloc(s >> 1)) == NULL) {
		CRYPTOerr(CRYPTO_F_OPENSSL_HEXSTR2BUF, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	for(p = (const uchar*)str, q = hexbuf; *p; ) {
		ch = *p++;
		if(ch == ':')
			continue;
		cl = *p++;
		if(!cl) {
			CRYPTOerr(CRYPTO_F_OPENSSL_HEXSTR2BUF,
			    CRYPTO_R_ODD_NUMBER_OF_DIGITS);
			OPENSSL_free(hexbuf);
			return NULL;
		}
		cli = OPENSSL_hexchar2int(cl);
		chi = OPENSSL_hexchar2int(ch);
		if(cli < 0 || chi < 0) {
			OPENSSL_free(hexbuf);
			CRYPTOerr(CRYPTO_F_OPENSSL_HEXSTR2BUF, CRYPTO_R_ILLEGAL_HEX_DIGIT);
			return NULL;
		}
		*q++ = (uchar)((chi << 4) | cli);
	}
	if(len)
		*len = q - hexbuf;
	return hexbuf;
}

/*
 * Given a buffer of length 'len' return a OPENSSL_malloc'ed string with its
 * hex representation @@@ (Contents of buffer are always kept in ASCII, also
 * on EBCDIC machines)
 */
char * OPENSSL_buf2hexstr(const uchar * buffer, long len)
{
	const static char hexdig[] = "0123456789ABCDEF";
	char * tmp, * q;
	const uchar * p;
	int i;
	if(len == 0) {
		return (char*)OPENSSL_zalloc(1);
	}
	if((tmp = (char*)OPENSSL_malloc(len * 3)) == NULL) {
		CRYPTOerr(CRYPTO_F_OPENSSL_BUF2HEXSTR, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	q = tmp;
	for(i = 0, p = buffer; i < len; i++, p++) {
		*q++ = hexdig[(*p >> 4) & 0xf];
		*q++ = hexdig[*p & 0xf];
		*q++ = ':';
	}
	q[-1] = 0;
#ifdef CHARSET_EBCDIC
	ebcdic2ascii(tmp, tmp, q - tmp - 1);
#endif
	return tmp;
}

int openssl_strerror_r(int errnum, char * buf, size_t buflen)
{
#if defined(_MSC_VER) && _MSC_VER>=1400
	return !strerror_s(buf, buflen, errnum);
#elif defined(_GNU_SOURCE)
	return strerror_r(errnum, buf, buflen) != NULL;
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600)
	/*
	 * We can use "real" strerror_r. The OpenSSL version differs in that it
	 * gives 1 on success and 0 on failure for consistency with other OpenSSL
	 * functions. Real strerror_r does it the other way around
	 */
	return !strerror_r(errnum, buf, buflen);
#else
	char * err;
	/* Fall back to non-thread safe strerror()...its all we can do */
	if(buflen < 2)
		return 0;
	err = strerror(errnum);
	/* Can this ever happen? */
	if(err == NULL)
		return 0;
	strncpy(buf, err, buflen - 1);
	buf[buflen - 1] = '\0';
	return 1;
#endif
}

