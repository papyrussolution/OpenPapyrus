/**********************************************************************
   big5.c -  Oniguruma (regular expression library)
**********************************************************************/
/*-
 * Copyright (c) 2002-2020  K.Kosako
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#include "regint.h"
#pragma hdrstop

static const int EncLen_BIG5[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1
};

static int big5_mbc_enc_len(const uchar * p)
{
	return EncLen_BIG5[*p];
}

static int big5_code_to_mbclen(OnigCodePoint code)
{
	if((code & (~0xffff)) != 0) return ONIGERR_INVALID_CODE_POINT_VALUE;

	if((code & 0xff00) != 0) {
		if(EncLen_BIG5[(int)(code >> 8) & 0xff] == 2)
			return 2;
	}
	else {
		if(EncLen_BIG5[(int)(code & 0xff)] == 1)
			return 1;
	}

	return ONIGERR_INVALID_CODE_POINT_VALUE;
}

static int is_valid_mbc_string(const uchar * p, const uchar * end)
{
	while(p < end) {
		if(*p < 0x80) {
			p++;
		}
		else if(*p < 0xa1) {
			return FALSE;
		}
		else if(*p < 0xff) {
			p++;
			if(p >= end) return FALSE;
			if(*p < 0x40) return FALSE;
			if(*p > 0x7e && *p < 0xa1) return FALSE;
			if(*p == 0xff) return FALSE;
			p++;
		}
		else
			return FALSE;
	}

	return TRUE;
}

static OnigCodePoint big5_mbc_to_code(const uchar * p, const uchar * end)
{
	return onigenc_mbn_mbc_to_code(ONIG_ENCODING_BIG5, p, end);
}

static int big5_code_to_mbc(OnigCodePoint code, uchar * buf)
{
	return onigenc_mb2_code_to_mbc(ONIG_ENCODING_BIG5, code, buf);
}

static int big5_mbc_case_fold(OnigCaseFoldType flag, const uchar ** pp, const uchar * end,
    uchar * lower)
{
	return onigenc_mbn_mbc_case_fold(ONIG_ENCODING_BIG5, flag,
		   pp, end, lower);
}

static int big5_is_code_ctype(OnigCodePoint code, uint ctype)
{
	return onigenc_mb2_is_code_ctype(ONIG_ENCODING_BIG5, code, ctype);
}

static const char BIG5_CAN_BE_TRAIL_TABLE[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

#define BIG5_ISMB_FIRST(byte)  (EncLen_BIG5[byte] > 1)
#define BIG5_ISMB_TRAIL(byte)  BIG5_CAN_BE_TRAIL_TABLE[(byte)]

static uchar * big5_left_adjust_char_head(const uchar * start, const uchar * s)
{
	const uchar * p;
	int len;
	if(s <= start) 
		return (uchar *)s;
	p = s;
	if(BIG5_ISMB_TRAIL(*p)) {
		while(p > start) {
			if(!BIG5_ISMB_FIRST(*--p)) {
				p++;
				break;
			}
		}
	}
	len = enclen(ONIG_ENCODING_BIG5, p);
	if(p + len > s) 
		return (uchar *)p;
	p += len;
	return (uchar *)(p + ((s - p) & ~1));
}

static int big5_is_allowed_reverse_match(const uchar * s, const uchar * end ARG_UNUSED)
{
	const uchar c = *s;
	return (BIG5_ISMB_TRAIL(c) ? FALSE : TRUE);
}

OnigEncodingType OnigEncodingBIG5 = {
	big5_mbc_enc_len,
	"Big5", /* name */
	2,    /* max enc length */
	1,    /* min enc length */
	onigenc_is_mbc_newline_0x0a,
	big5_mbc_to_code,
	big5_code_to_mbclen,
	big5_code_to_mbc,
	big5_mbc_case_fold,
	onigenc_ascii_apply_all_case_fold,
	onigenc_ascii_get_case_fold_codes_by_str,
	onigenc_minimum_property_name_to_ctype,
	big5_is_code_ctype,
	onigenc_not_support_get_ctype_code_range,
	big5_left_adjust_char_head,
	big5_is_allowed_reverse_match,
	NULL, /* init */
	NULL, /* is_initialized */
	is_valid_mbc_string,
	ENC_FLAG_ASCII_COMPATIBLE|ENC_FLAG_SKIP_OFFSET_1,
	0, 0
};
