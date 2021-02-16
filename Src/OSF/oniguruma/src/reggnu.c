// reggnu.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2019  K.Kosako  All rights reserved.
//
#include "regint.h"
#pragma hdrstop
#include "oniggnu.h"

extern void re_free_registers(OnigRegion* r)
{
	/* 0: don't free self */
	onig_region_free(r, 0);
}

extern int re_adjust_startpos(regex_t* reg, const char* string, int size, int startpos, int range)
{
	if(startpos > 0 && ONIGENC_MBC_MAXLEN(reg->enc) != 1 && startpos < size) {
		uchar * p;
		uchar * s = (uchar *)string + startpos;
		if(range > 0) {
			p = onigenc_get_right_adjust_char_head(reg->enc, (uchar *)string, s);
		}
		else {
			p = ONIGENC_LEFT_ADJUST_CHAR_HEAD(reg->enc, (uchar *)string, s);
		}
		return (int)(p - (uchar *)string);
	}
	return startpos;
}

extern int re_match(regex_t* reg, const char* str, int size, int pos, struct re_registers* regs)
{
	return onig_match(reg, (uchar *)str, (uchar *)(str + size), (uchar *)(str + pos), regs, ONIG_OPTION_NONE);
}

extern int re_search(regex_t* bufp, const char* string, int size, int startpos, int range, struct re_registers* regs)
{
	return onig_search(bufp, (uchar *)string, (uchar *)(string + size), (uchar *)(string + startpos), (uchar *)(string + startpos + range), regs, ONIG_OPTION_NONE);
}

extern int re_compile_pattern(const char* pattern, int size, regex_t* reg, char* ebuf)
{
	OnigErrorInfo einfo;
	int r = onig_compile(reg, (uchar *)pattern, (uchar *)(pattern + size), &einfo);
	if(r != ONIG_NORMAL) {
		if(IS_NOT_NULL(ebuf))
			(void)onig_error_code_to_str((uchar *)ebuf, r, &einfo);
	}
	return r;
}

extern void re_free_pattern(regex_t* reg)
{
	onig_free(reg);
}

extern int re_alloc_pattern(regex_t** reg)
{
	*reg = (regex_t*)SAlloc::M(sizeof(regex_t));
	if(IS_NULL(*reg)) 
		return ONIGERR_MEMORY;
	return onig_reg_init(*reg, ONIG_OPTION_DEFAULT, ONIGENC_CASE_FOLD_DEFAULT, OnigEncDefaultCharEncoding, OnigDefaultSyntax);
}

extern void re_set_casetable(const char* table)
{
	onigenc_set_default_caseconv_table((uchar *)table);
}

extern void re_mbcinit(int mb_code)
{
	OnigEncoding enc;
	switch(mb_code) {
		case RE_MBCTYPE_ASCII: enc = ONIG_ENCODING_ASCII; break;
		case RE_MBCTYPE_EUC: enc = ONIG_ENCODING_EUC_JP; break;
		case RE_MBCTYPE_SJIS: enc = ONIG_ENCODING_SJIS; break;
		case RE_MBCTYPE_UTF8: enc = ONIG_ENCODING_UTF8; break;
		default: return; break;
	}
	onig_initialize(&enc, 1);
	onigenc_set_default_encoding(enc);
}
