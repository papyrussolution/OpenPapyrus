// utf16_le.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2020  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

static int init(void)
{
#ifdef USE_CALLOUT
	int id;
	char * name;
	uint args[4];
	OnigValue opts[4];
	OnigEncoding enc = ONIG_ENCODING_UTF16_LE;
	name = "F\000A\000I\000L\000\000\000";            BC0_P(name, fail);
	name = "M\000I\000S\000M\000A\000T\000C\000H\000\000\000"; BC0_P(name, mismatch);
	name = "M\000A\000X\000\000\000";
	args[0] = ONIG_TYPE_TAG | ONIG_TYPE_LONG;
	args[1] = ONIG_TYPE_CHAR;
	opts[0].c = 'X';
	BC_B_O(name, max, 2, args, 1, opts);

	name = "E\000R\000R\000O\000R\000\000\000";
	args[0] = ONIG_TYPE_LONG; opts[0].l = ONIG_ABORT;
	BC_P_O(name, error, 1, args, 1, opts);

	name = "C\000O\000U\000N\000T\000\000\000";
	args[0] = ONIG_TYPE_CHAR; opts[0].c = '>';
	BC_B_O(name, count, 1, args, 1, opts);

	name = "T\000O\000T\000A\000L\000_\000C\000O\000U\000N\000T\000\000\000";
	args[0] = ONIG_TYPE_CHAR; opts[0].c = '>';
	BC_B_O(name, total_count, 1, args, 1, opts);

	name = "C\000M\000P\000\000\000";
	args[0] = ONIG_TYPE_TAG | ONIG_TYPE_LONG;
	args[1] = ONIG_TYPE_STRING;
	args[2] = ONIG_TYPE_TAG | ONIG_TYPE_LONG;
	BC_P(name, cmp, 3, args);

#endif /* USE_CALLOUT */

	return ONIG_NORMAL;
}

static const int EncLen_UTF16[] = {
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

static int utf16le_code_to_mbclen(OnigCodePoint code)
{
	if(code > 0xffff) {
		if(code > 0x10ffff)
			return ONIGERR_INVALID_CODE_POINT_VALUE;
		else
			return 4;
	}
	else {
		return 2;
	}
}

static int utf16le_mbc_enc_len(const uchar * p)
{
	return EncLen_UTF16[*(p+1)];
}

static int is_valid_mbc_string(const uchar * p, const uchar * end)
{
	const uchar * end1 = end - 1;
	while(p < end1) {
		int len = utf16le_mbc_enc_len(p);
		if(len == 4) {
			if(p + 3 < end && !UTF16_IS_SURROGATE_SECOND(*(p + 3)))
				return FALSE;
		}
		else if(UTF16_IS_SURROGATE_SECOND(*(p + 1)))
			return FALSE;

		p += len;
	}
	if(p != end)
		return FALSE;
	else
		return TRUE;
}

static int utf16le_is_mbc_newline(const uchar * p, const uchar * end)
{
	if(p + 1 < end) {
		if(*p == NEWLINE_CODE && *(p+1) == 0x00)
			return 1;
#ifdef USE_UNICODE_ALL_LINE_TERMINATORS
		if((
#ifndef USE_CRNL_AS_LINE_TERMINATOR
			    *p == 0x0d ||
#endif
			    *p == 0x85) && *(p+1) == 0x00)
			return 1;

		if(*(p+1) == 0x20 && (*p == 0x29 || *p == 0x28))
			return 1;
#endif
	}
	return 0;
}

static OnigCodePoint utf16le_mbc_to_code(const uchar * p, const uchar * end ARG_UNUSED)
{
	OnigCodePoint code;
	uchar c0 = *p;
	uchar c1 = *(p+1);
	if(UTF16_IS_SURROGATE_FIRST(c1)) {
		code = ((((c1 - 0xd8) << 2) + ((c0  & 0xc0) >> 6) + 1) << 16) + ((((c0 & 0x3f) << 2) + (p[3] - 0xdc)) << 8) + p[2];
	}
	else {
		code = c1 * 256 + p[0];
	}
	return code;
}

static int utf16le_code_to_mbc(OnigCodePoint code, uchar * buf)
{
	uchar * p = buf;
	if(code > 0xffff) {
		uint plane = (code >> 16) - 1;
		uint high = (code & 0xff00) >> 8;
		*p++ = ((plane & 0x03) << 6) + (high >> 2);
		*p++ = (plane >> 2) + 0xd8;
		*p++ = (uchar)(code & 0xff);
		*p   = (high & 0x03) + 0xdc;
		return 4;
	}
	else {
		*p++ = (uchar)(code & 0xff);
		*p   = (uchar)((code & 0xff00) >> 8);
		return 2;
	}
}

static int utf16le_mbc_case_fold(OnigCaseFoldType flag, const uchar ** pp, const uchar * end, uchar * fold)
{
	const uchar * p = *pp;
	if(ONIGENC_IS_ASCII_CODE(*p) && *(p+1) == 0) {
#ifdef USE_UNICODE_CASE_FOLD_TURKISH_AZERI
		if((flag & ONIGENC_CASE_FOLD_TURKISH_AZERI) != 0) {
			if(*p == 0x49) {
				*fold++ = 0x31;
				*fold   = 0x01;
				(*pp) += 2;
				return 2;
			}
		}
#endif
		*fold++ = ONIGENC_ASCII_CODE_TO_LOWER_CASE(*p);
		*fold   = 0;
		*pp += 2;
		return 2;
	}
	else
		return onigenc_unicode_mbc_case_fold(ONIG_ENCODING_UTF16_LE, flag, pp, end, fold);
}

static uchar * utf16le_left_adjust_char_head(const uchar * start, const uchar * s)
{
	if(s <= start) 
		return (uchar *)s;
	if((s - start) % 2 == 1) {
		s--;
	}
	if(UTF16_IS_SURROGATE_SECOND(*(s+1)) && s > start + 1 && UTF16_IS_SURROGATE_FIRST(*(s-1)))
		s -= 2;
	return (uchar *)s;
}

static int utf16le_get_case_fold_codes_by_str(OnigCaseFoldType flag, const uchar * p, const uchar * end, OnigCaseFoldCodeItem items[])
{
	return onigenc_unicode_get_case_fold_codes_by_str(ONIG_ENCODING_UTF16_LE, flag, p, end, items);
}

OnigEncodingType OnigEncodingUTF16_LE = {
	utf16le_mbc_enc_len,
	"UTF-16LE", /* name */
	4,      /* max enc length */
	2,      /* min enc length */
	utf16le_is_mbc_newline,
	utf16le_mbc_to_code,
	utf16le_code_to_mbclen,
	utf16le_code_to_mbc,
	utf16le_mbc_case_fold,
	onigenc_unicode_apply_all_case_fold,
	utf16le_get_case_fold_codes_by_str,
	onigenc_unicode_property_name_to_ctype,
	onigenc_unicode_is_code_ctype,
	onigenc_utf16_32_get_ctype_code_range,
	utf16le_left_adjust_char_head,
	onigenc_always_false_is_allowed_reverse_match,
	init,
	0, /* is_initialized */
	is_valid_mbc_string,
	ENC_FLAG_UNICODE|ENC_FLAG_SKIP_OFFSET_1,
	0, 0
};
