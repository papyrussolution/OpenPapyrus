// CRYPTO.CPP
// Copyright (c) A.Sobolev 1996, 2003, 2010, 2016
// @threadsafe
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

static int SLAPI getkey(int key[], ulong * addendum)
{
	key[0] = 5;
	key[1] = 4;
	key[2] = 19;
	key[3] = 2;
	key[4] = 171;
	key[5] = 1;
	key[6] = 88;
	*addendum = 0x11230244UL;
	return 7;
}

static ulong SLAPI mix(ulong v)
{
	int i;
	int key[16];
	ulong addendum;
	int c = getkey(key, &addendum);
	v += addendum;
	for(i = 0; i < c; i++)
		if(key[i] % 3)
			v = _lrotr(v, key[i]);
		else
			v = ~_lrotl(v, key[i]);
	return v;
}

static ulong SLAPI unmix(ulong v)
{
	int i;
	int key[16];
	ulong addendum;
	int c = getkey(key, &addendum);
	for(i = c-1; i >= 0; i--)
		if(key[i] % 3)
			v = _lrotl(v, key[i]);
		else
			v = _lrotr(~v, key[i]);
	v -= addendum;
	return v;
}

void * SLAPI encrypt(void * pBuf, size_t len)
{
	size_t i;
	len = ((len + 3) >> 2);
	for(i = 0; i < len; i++)
		((ulong*)pBuf)[i] = mix(((ulong*)pBuf)[i]);
	return pBuf;
}

void * SLAPI decrypt(void * pBuf, size_t len)
{
	size_t i;
	len >>= 2;
	for(i = 0; i < len; i++)
		((ulong*)pBuf)[i] = unmix(((ulong*)pBuf)[i]);
	return pBuf;
}

ulong SLAPI _checksum__(const char * buf, size_t len)
{
	ulong r = 0xc22cc22cUL;
	size_t i;
	for(i = 0; i < len; i++)
		((uchar*)&r)[i % 4] += (uchar)((uint)buf[i] ^ (uint)((uchar*)&r)[3 - (i % 4)]);
	return r;
}
