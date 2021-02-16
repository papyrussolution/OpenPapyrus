// regext.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2019  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

#if 0
static void conv_ext0be32(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = '\0';
		*conv++ = '\0';
		*conv++ = '\0';
		*conv++ = *s++;
	}
}

static void conv_ext0le32(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = *s++;
		*conv++ = '\0';
		*conv++ = '\0';
		*conv++ = '\0';
	}
}

static void conv_ext0be(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = '\0';
		*conv++ = *s++;
	}
}

static void conv_ext0le(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = *s++;
		*conv++ = '\0';
	}
}

static void conv_swap4bytes(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = s[3];
		*conv++ = s[2];
		*conv++ = s[1];
		*conv++ = s[0];
		s += 4;
	}
}

static void conv_swap2bytes(const uchar * s, const uchar * end, uchar * conv)
{
	while(s < end) {
		*conv++ = s[1];
		*conv++ = s[0];
		s += 2;
	}
}

static int conv_encoding(OnigEncoding from, OnigEncoding to, const uchar * s, const uchar * end,
    uchar ** conv, uchar ** conv_end)
{
	int len = (int)(end - s);
	if(to == ONIG_ENCODING_UTF16_BE) {
		if(from == ONIG_ENCODING_ASCII || from == ONIG_ENCODING_ISO_8859_1) {
			*conv = (uchar *)SAlloc::M(len * 2);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + (len * 2);
			conv_ext0be(s, end, *conv);
			return 0;
		}
		else if(from == ONIG_ENCODING_UTF16_LE) {
swap16:
			*conv = (uchar *)SAlloc::M(len);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + len;
			conv_swap2bytes(s, end, *conv);
			return 0;
		}
	}
	else if(to == ONIG_ENCODING_UTF16_LE) {
		if(from == ONIG_ENCODING_ASCII || from == ONIG_ENCODING_ISO_8859_1) {
			*conv = (uchar *)SAlloc::M(len * 2);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + (len * 2);
			conv_ext0le(s, end, *conv);
			return 0;
		}
		else if(from == ONIG_ENCODING_UTF16_BE) {
			goto swap16;
		}
	}
	if(to == ONIG_ENCODING_UTF32_BE) {
		if(from == ONIG_ENCODING_ASCII || from == ONIG_ENCODING_ISO_8859_1) {
			*conv = (uchar *)SAlloc::M(len * 4);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + (len * 4);
			conv_ext0be32(s, end, *conv);
			return 0;
		}
		else if(from == ONIG_ENCODING_UTF32_LE) {
swap32:
			*conv = (uchar *)SAlloc::M(len);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + len;
			conv_swap4bytes(s, end, *conv);
			return 0;
		}
	}
	else if(to == ONIG_ENCODING_UTF32_LE) {
		if(from == ONIG_ENCODING_ASCII || from == ONIG_ENCODING_ISO_8859_1) {
			*conv = (uchar *)SAlloc::M(len * 4);
			CHECK_NULL_RETURN_MEMERR(*conv);
			*conv_end = *conv + (len * 4);
			conv_ext0le32(s, end, *conv);
			return 0;
		}
		else if(from == ONIG_ENCODING_UTF32_BE) {
			goto swap32;
		}
	}
	return ONIGERR_NOT_SUPPORTED_ENCODING_COMBINATION;
}

#endif

extern int onig_new_deluxe(regex_t** reg, const uchar * pattern, const uchar * pattern_end, OnigCompileInfo* ci, OnigErrorInfo* einfo)
{
	int r;
	uchar * cpat, * cpat_end;
	if(IS_NOT_NULL(einfo)) 
		einfo->par = (uchar *)NULL;
	if(ci->pattern_enc != ci->target_enc) {
		return ONIGERR_NOT_SUPPORTED_ENCODING_COMBINATION;
	}
	else {
		cpat     = (uchar *)pattern;
		cpat_end = (uchar *)pattern_end;
	}
	*reg = (regex_t*)SAlloc::M(sizeof(regex_t));
	if(IS_NULL(*reg)) {
		r = ONIGERR_MEMORY;
		goto err2;
	}
	r = onig_reg_init(*reg, ci->option, ci->case_fold_flag, ci->target_enc, ci->syntax);
	if(r != 0) 
		goto err;
	r = onig_compile(*reg, cpat, cpat_end, einfo);
	if(r != 0) {
err:
		onig_free(*reg);
		*reg = NULL;
	}
err2:
	if(cpat != pattern) SAlloc::F(cpat);
	return r;
}
