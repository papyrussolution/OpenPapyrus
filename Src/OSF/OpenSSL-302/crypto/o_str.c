/*
 * Copyright 2003-2022 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include "e_os.h"

#define DEFAULT_SEPARATOR ':'
#define CH_ZERO '\0'

char * CRYPTO_strdup(const char * str, const char* file, int line)
{
	char * ret;
	if(!str)
		return NULL;
	ret = (char*)CRYPTO_malloc(strlen(str) + 1, file, line);
	if(ret)
		strcpy(ret, str);
	return ret;
}

char * CRYPTO_strndup(const char * str, size_t s, const char* file, int line)
{
	size_t maxlen;
	char * ret;
	if(!str)
		return NULL;
	maxlen = /*OPENSSL_strnlen*/sstrnlen(str, s);
	ret = (char*)CRYPTO_malloc(maxlen + 1, file, line);
	if(ret) {
		memcpy(ret, str, maxlen);
		ret[maxlen] = CH_ZERO;
	}
	return ret;
}

void * CRYPTO_memdup(const void * data, size_t siz, const char* file, int line)
{
	void * ret = NULL;
	if(data == NULL || siz >= INT_MAX)
		return NULL;
	ret = CRYPTO_malloc(siz, file, line);
	if(!ret) {
		ERR_raise(ERR_LIB_CRYPTO, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	return memcpy(ret, data, siz);
}

/* @v11.9.12 (replaced with sstrnlen) size_t OPENSSL_strnlen(const char * str, size_t maxlen)
{
	const char * p;
	for(p = str; maxlen-- != 0 && *p != CH_ZERO; ++p)
		;
	return p - str;
}*/

size_t OPENSSL_strlcpy(char * dst, const char * src, size_t size)
{
	size_t l = 0;
	for(; size > 1 && *src; size--) {
		*dst++ = *src++;
		l++;
	}
	if(size)
		*dst = CH_ZERO;
	return l + strlen(src);
}

size_t OPENSSL_strlcat(char * dst, const char * src, size_t size)
{
	size_t l = 0;
	for(; size > 0 && *dst; size--, dst++)
		l++;
	return l + OPENSSL_strlcpy(dst, src, size);
}

int OPENSSL_hexchar2int(unsigned char c)
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

static int hexstr2buf_sep(uchar * buf, size_t buf_n, size_t * buflen, const char * str, const char sep)
{
	uchar * q;
	unsigned char ch, cl;
	int chi, cli;
	const uchar * p;
	size_t cnt;

	for(p = (const unsigned char*)str, q = buf, cnt = 0; *p;) {
		ch = *p++;
		/* A separator of CH_ZERO means there is no separator */
		if(ch == sep && sep != CH_ZERO)
			continue;
		cl = *p++;
		if(!cl) {
			ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_ODD_NUMBER_OF_DIGITS);
			return 0;
		}
		cli = OPENSSL_hexchar2int(cl);
		chi = OPENSSL_hexchar2int(ch);
		if(cli < 0 || chi < 0) {
			ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_ILLEGAL_HEX_DIGIT);
			return 0;
		}
		cnt++;
		if(q) {
			if(cnt > buf_n) {
				ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_TOO_SMALL_BUFFER);
				return 0;
			}
			*q++ = (uchar)((chi << 4) | cli);
		}
	}
	if(buflen)
		*buflen = cnt;
	return 1;
}

/*
 * Given a string of hex digits convert to a buffer
 */
int OPENSSL_hexstr2buf_ex(uchar * buf, size_t buf_n, size_t * buflen,
    const char * str, const char sep)
{
	return hexstr2buf_sep(buf, buf_n, buflen, str, sep);
}

uchar * ossl_hexstr2buf_sep(const char * str, long * buflen,
    const char sep)
{
	uchar * buf;
	size_t buf_n, tmp_buflen;

	buf_n = strlen(str);
	if(buf_n <= 1) {
		ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_HEX_STRING_TOO_SHORT);
		return NULL;
	}
	buf_n /= 2;
	if((buf = (uchar *)OPENSSL_malloc(buf_n)) == NULL) {
		ERR_raise(ERR_LIB_CRYPTO, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	if(buflen)
		*buflen = 0;
	tmp_buflen = 0;
	if(hexstr2buf_sep(buf, buf_n, &tmp_buflen, str, sep)) {
		if(buflen)
			*buflen = (long)tmp_buflen;
		return buf;
	}
	OPENSSL_free(buf);
	return NULL;
}

uchar * OPENSSL_hexstr2buf(const char * str, long * buflen)
{
	return ossl_hexstr2buf_sep(str, buflen, DEFAULT_SEPARATOR);
}

static int buf2hexstr_sep(char * str, size_t str_n, size_t * strlength, const uchar * buf, size_t buflen, const char sep)
{
	// @sobolev static const char hexdig[] = "0123456789ABCDEF";
	const uchar * p;
	char * q;
	size_t i;
	int has_sep = (sep != CH_ZERO);
	size_t len = has_sep ? buflen * 3 : 1 + buflen * 2;
	if(strlength)
		*strlength = len;
	if(!str)
		return 1;
	if(str_n < (ulong)len) {
		ERR_raise(ERR_LIB_CRYPTO, CRYPTO_R_TOO_SMALL_BUFFER);
		return 0;
	}
	q = str;
	for(i = 0, p = buf; i < buflen; i++, p++) {
		*q++ = SlConst::P_HxDigU[(*p >> 4) & 0xf];
		*q++ = SlConst::P_HxDigU[*p & 0xf];
		if(has_sep)
			*q++ = sep;
	}
	if(has_sep)
		--q;
	*q = CH_ZERO;

#ifdef CHARSET_EBCDIC
	ebcdic2ascii(str, str, q - str - 1);
#endif
	return 1;
}

int OPENSSL_buf2hexstr_ex(char * str, size_t str_n, size_t * strlength, const uchar * buf, size_t buflen, const char sep)
{
	return buf2hexstr_sep(str, str_n, strlength, buf, buflen, sep);
}

char * ossl_buf2hexstr_sep(const uchar * buf, long buflen, char sep)
{
	char * tmp;
	size_t tmp_n;
	if(buflen == 0)
		return (char *)OPENSSL_zalloc(1);
	tmp_n = (sep != CH_ZERO) ? buflen * 3 : 1 + buflen * 2;
	if((tmp = (char*)OPENSSL_malloc(tmp_n)) == NULL) {
		ERR_raise(ERR_LIB_CRYPTO, ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	if(buf2hexstr_sep(tmp, tmp_n, NULL, buf, buflen, sep))
		return tmp;
	OPENSSL_free(tmp);
	return NULL;
}
/*
 * Given a buffer of length 'len' return a OPENSSL_malloc'ed string with its
 * hex representation @@@ (Contents of buffer are always kept in ASCII, also
 * on EBCDIC machines)
 */
char * OPENSSL_buf2hexstr(const uchar * buf, long buflen)
{
	return ossl_buf2hexstr_sep(buf, buflen, ':');
}

int openssl_strerror_r(int errnum, char * buf, size_t buflen)
{
#if defined(_MSC_VER) && _MSC_VER>=1400 && !defined(_WIN32_WCE)
	return !strerror_s(buf, buflen, errnum);
#elif defined(_GNU_SOURCE)
	char * err;

	/*
	 * GNU strerror_r may not actually set buf.
	 * It can return a pointer to some (immutable) static string in which case
	 * buf is left unused.
	 */
	err = strerror_r(errnum, buf, buflen);
	if(err == NULL || buflen == 0)
		return 0;
	/*
	 * If err is statically allocated, err != buf and we need to copy the data.
	 * If err points somewhere inside buf, OPENSSL_strlcpy can handle this,
	 * since src and dest are not annotated with __restrict and the function
	 * reads src byte for byte and writes to dest.
	 * If err == buf we do not have to copy anything.
	 */
	if(err != buf)
		OPENSSL_strlcpy(buf, err, buflen);
	return 1;
#elif (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) || \
	(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
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
	OPENSSL_strlcpy(buf, err, buflen);
	return 1;
#endif
}
