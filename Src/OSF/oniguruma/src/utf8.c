// utf8.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2019  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

#define USE_RFC3629_RANGE /* U+0000 - U+10FFFF */

/* #define USE_INVALID_CODE_SCHEME */

#ifdef USE_INVALID_CODE_SCHEME
/* virtual codepoint values for invalid encoding byte 0xfe and 0xff */
#define INVALID_CODE_FE   0xfffffffe
#define INVALID_CODE_FF   0xffffffff
#define VALID_CODE_LIMIT  0x7fffffff
#endif

#define utf8_islead(c)     ((uchar)((c) & 0xc0) != 0x80)
#define utf8_istail(c)     ((uchar)((c) & 0xc0) == 0x80)

// (replaced with SUtfConst::Utf8EncLen_RFC3629/SUtfConst::Utf8EncLen_) static const int EncLen_UTF8[] = {
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
//#ifdef USE_RFC3629_RANGE
//	4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
//#else
//	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
//#endif
//};

static int mbc_enc_len(const uchar * p)
{
	//return EncLen_UTF8[*p];
#ifdef USE_RFC3629_RANGE
	return SUtfConst::Utf8EncLen_RFC3629[*p];
#else
	return SUtfConst::Utf8EncLen_[*p];
#endif
}

static int is_valid_mbc_string(const uchar * p, const uchar * end)
{
	int i, len;
	while(p < end) {
		if(!utf8_islead(*p))
			return FALSE;
		len = mbc_enc_len(p++);
		if(len > 1) {
			for(i = 1; i < len; i++) {
				if(p == end)
					return FALSE;
				if(!utf8_istail(*p++))
					return FALSE;
			}
		}
	}
	return TRUE;
}

static OnigCodePoint mbc_to_code(const uchar * p, const uchar * end)
{
	int c;
	OnigCodePoint n;
	int len = mbc_enc_len(p);
	if(len > (int)(end - p)) 
		len = (int)(end - p);
	c = *p++;
	if(len > 1) {
		len--;
		n = c & ((1 << (6 - len)) - 1);
		while(len--) {
			c = *p++;
			n = (n << 6) | (c & ((1 << 6) - 1));
		}
		return n;
	}
	else {
#ifdef USE_INVALID_CODE_SCHEME
		if(c > 0xfd) {
			return ((c == 0xfe) ? INVALID_CODE_FE : INVALID_CODE_FF);
		}
#endif
		return (OnigCodePoint)c;
	}
}

static int code_to_mbclen(OnigCodePoint code)
{
	if((code & 0xffffff80) == 0) return 1;
	else if((code & 0xfffff800) == 0) return 2;
	else if((code & 0xffff0000) == 0) return 3;
	else if((code & 0xffe00000) == 0) return 4;
#ifndef USE_RFC3629_RANGE
	else if((code & 0xfc000000) == 0) return 5;
	else if((code & 0x80000000) == 0) return 6;
#endif
#ifdef USE_INVALID_CODE_SCHEME
	else if(code == INVALID_CODE_FE) return 1;
	else if(code == INVALID_CODE_FF) return 1;
#endif
	else
		return ONIGERR_INVALID_CODE_POINT_VALUE;
}

static int code_to_mbc(OnigCodePoint code, uchar * buf)
{
#define UTF8_TRAILS(code, shift) (uchar)((((code) >> (shift)) & 0x3f) | 0x80)
#define UTF8_TRAIL0(code)        (uchar)(((code) & 0x3f) | 0x80)

	if((code & 0xffffff80) == 0) {
		*buf = (uchar)code;
		return 1;
	}
	else {
		uchar * p = buf;
		if((code & 0xfffff800) == 0) {
			*p++ = (uchar)(((code>>6)& 0x1f) | 0xc0);
		}
		else if((code & 0xffff0000) == 0) {
			*p++ = (uchar)(((code>>12) & 0x0f) | 0xe0);
			*p++ = UTF8_TRAILS(code, 6);
		}
		else if((code & 0xffe00000) == 0) {
			*p++ = (uchar)(((code>>18) & 0x07) | 0xf0);
			*p++ = UTF8_TRAILS(code, 12);
			*p++ = UTF8_TRAILS(code,  6);
		}
#ifndef USE_RFC3629_RANGE
		else if((code & 0xfc000000) == 0) {
			*p++ = (uchar)(((code>>24) & 0x03) | 0xf8);
			*p++ = UTF8_TRAILS(code, 18);
			*p++ = UTF8_TRAILS(code, 12);
			*p++ = UTF8_TRAILS(code,  6);
		}
		else if((code & 0x80000000) == 0) {
			*p++ = (uchar)(((code>>30) & 0x01) | 0xfc);
			*p++ = UTF8_TRAILS(code, 24);
			*p++ = UTF8_TRAILS(code, 18);
			*p++ = UTF8_TRAILS(code, 12);
			*p++ = UTF8_TRAILS(code,  6);
		}
#endif
#ifdef USE_INVALID_CODE_SCHEME
		else if(code == INVALID_CODE_FE) {
			*p = 0xfe;
			return 1;
		}
		else if(code == INVALID_CODE_FF) {
			*p = 0xff;
			return 1;
		}
#endif
		else {
			return ONIGERR_TOO_BIG_WIDE_CHAR_VALUE;
		}

		*p++ = UTF8_TRAIL0(code);
		return (int)(p - buf);
	}
}

static int mbc_case_fold(OnigCaseFoldType flag, const uchar ** pp, const uchar * end, uchar * fold)
{
	const uchar * p = *pp;
	if(ONIGENC_IS_MBC_ASCII(p)) {
#ifdef USE_UNICODE_CASE_FOLD_TURKISH_AZERI
		if((flag & ONIGENC_CASE_FOLD_TURKISH_AZERI) != 0) {
			if(*p == 0x49) {
				*fold++ = 0xc4;
				*fold   = 0xb1;
				(*pp)++;
				return 2;
			}
		}
#endif
		*fold = ONIGENC_ASCII_CODE_TO_LOWER_CASE(*p);
		(*pp)++;
		return 1; /* return byte length of converted char to lower */
	}
	else {
		return onigenc_unicode_mbc_case_fold(ONIG_ENCODING_UTF8, flag, pp, end, fold);
	}
}

static int get_ctype_code_range(OnigCtype ctype, OnigCodePoint * sb_out,
    const OnigCodePoint* ranges[])
{
	*sb_out = 0x80;
	return onigenc_unicode_ctype_code_range(ctype, ranges);
}

static uchar * left_adjust_char_head(const uchar * start, const uchar * s)
{
	const uchar * p;
	if(s <= start) 
		return (uchar *)s;
	p = s;
	while(!utf8_islead(*p) && p > start) p--;
	return (uchar *)p;
}

static int get_case_fold_codes_by_str(OnigCaseFoldType flag, const uchar * p, const uchar * end, OnigCaseFoldCodeItem items[])
{
	return onigenc_unicode_get_case_fold_codes_by_str(ONIG_ENCODING_UTF8, flag, p, end, items);
}

OnigEncodingType OnigEncodingUTF8 = {
	mbc_enc_len,
	"UTF-8", /* name */
#ifdef USE_RFC3629_RANGE
	4,     /* max enc length */
#else
	6,
#endif
	1,     /* min enc length */
	onigenc_is_mbc_newline_0x0a,
	mbc_to_code,
	code_to_mbclen,
	code_to_mbc,
	mbc_case_fold,
	onigenc_unicode_apply_all_case_fold,
	get_case_fold_codes_by_str,
	onigenc_unicode_property_name_to_ctype,
	onigenc_unicode_is_code_ctype,
	get_ctype_code_range,
	left_adjust_char_head,
	onigenc_always_true_is_allowed_reverse_match,
	NULL, /* init */
	NULL, /* is_initialized */
	is_valid_mbc_string,
	ENC_FLAG_ASCII_COMPATIBLE|ENC_FLAG_UNICODE|ENC_FLAG_SKIP_OFFSET_1_OR_0,
	0, 0
};
