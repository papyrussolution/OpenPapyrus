/**********************************************************************
   iso8859_7.c -  Oniguruma (regular expression library)
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

#define ENC_ISO_8859_7_TO_LOWER_CASE(c) EncISO_8859_7_ToLowerCaseTable[c]
#define ENC_IS_ISO_8859_7_CTYPE(code, ctype) ((EncISO_8859_7_CtypeTable[code] & CTYPE_TO_BIT(ctype)) != 0)

static const uchar EncISO_8859_7_ToLowerCaseTable[256] = {
	(uchar)'\000', (uchar)'\001', (uchar)'\002', (uchar)'\003', (uchar)'\004', (uchar)'\005', (uchar)'\006', (uchar)'\007',
	(uchar)'\010', (uchar)'\011', (uchar)'\012', (uchar)'\013', (uchar)'\014', (uchar)'\015', (uchar)'\016', (uchar)'\017',
	(uchar)'\020', (uchar)'\021', (uchar)'\022', (uchar)'\023', (uchar)'\024', (uchar)'\025', (uchar)'\026', (uchar)'\027',
	(uchar)'\030', (uchar)'\031', (uchar)'\032', (uchar)'\033', (uchar)'\034', (uchar)'\035', (uchar)'\036', (uchar)'\037',
	(uchar)'\040', (uchar)'\041', (uchar)'\042', (uchar)'\043', (uchar)'\044', (uchar)'\045', (uchar)'\046', (uchar)'\047',
	(uchar)'\050', (uchar)'\051', (uchar)'\052', (uchar)'\053', (uchar)'\054', (uchar)'\055', (uchar)'\056', (uchar)'\057',
	(uchar)'\060', (uchar)'\061', (uchar)'\062', (uchar)'\063', (uchar)'\064', (uchar)'\065', (uchar)'\066', (uchar)'\067',
	(uchar)'\070', (uchar)'\071', (uchar)'\072', (uchar)'\073', (uchar)'\074', (uchar)'\075', (uchar)'\076', (uchar)'\077',
	(uchar)'\100', (uchar)'\141', (uchar)'\142', (uchar)'\143', (uchar)'\144', (uchar)'\145', (uchar)'\146', (uchar)'\147',
	(uchar)'\150', (uchar)'\151', (uchar)'\152', (uchar)'\153', (uchar)'\154', (uchar)'\155', (uchar)'\156', (uchar)'\157',
	(uchar)'\160', (uchar)'\161', (uchar)'\162', (uchar)'\163', (uchar)'\164', (uchar)'\165', (uchar)'\166', (uchar)'\167',
	(uchar)'\170', (uchar)'\171', (uchar)'\172', (uchar)'\133', (uchar)'\134', (uchar)'\135', (uchar)'\136', (uchar)'\137',
	(uchar)'\140', (uchar)'\141', (uchar)'\142', (uchar)'\143', (uchar)'\144', (uchar)'\145', (uchar)'\146', (uchar)'\147',
	(uchar)'\150', (uchar)'\151', (uchar)'\152', (uchar)'\153', (uchar)'\154', (uchar)'\155', (uchar)'\156', (uchar)'\157',
	(uchar)'\160', (uchar)'\161', (uchar)'\162', (uchar)'\163', (uchar)'\164', (uchar)'\165', (uchar)'\166', (uchar)'\167',
	(uchar)'\170', (uchar)'\171', (uchar)'\172', (uchar)'\173', (uchar)'\174', (uchar)'\175', (uchar)'\176', (uchar)'\177',
	(uchar)'\200', (uchar)'\201', (uchar)'\202', (uchar)'\203', (uchar)'\204', (uchar)'\205', (uchar)'\206', (uchar)'\207',
	(uchar)'\210', (uchar)'\211', (uchar)'\212', (uchar)'\213', (uchar)'\214', (uchar)'\215', (uchar)'\216', (uchar)'\217',
	(uchar)'\220', (uchar)'\221', (uchar)'\222', (uchar)'\223', (uchar)'\224', (uchar)'\225', (uchar)'\226', (uchar)'\227',
	(uchar)'\230', (uchar)'\231', (uchar)'\232', (uchar)'\233', (uchar)'\234', (uchar)'\235', (uchar)'\236', (uchar)'\237',
	(uchar)'\240', (uchar)'\241', (uchar)'\242', (uchar)'\243', (uchar)'\244', (uchar)'\245', (uchar)'\246', (uchar)'\247',
	(uchar)'\250', (uchar)'\251', (uchar)'\252', (uchar)'\253', (uchar)'\254', (uchar)'\255', (uchar)'\256', (uchar)'\257',
	(uchar)'\260', (uchar)'\261', (uchar)'\262', (uchar)'\263', (uchar)'\264', (uchar)'\265', (uchar)'\334', (uchar)'\267',
	(uchar)'\335', (uchar)'\336', (uchar)'\337', (uchar)'\273', (uchar)'\374', (uchar)'\275', (uchar)'\375', (uchar)'\376',
	(uchar)'\300', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\322', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\367',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\334', (uchar)'\335', (uchar)'\336', (uchar)'\337',
	(uchar)'\340', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\362', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\367',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\374', (uchar)'\375', (uchar)'\376', (uchar)'\377'
};

static const ushort EncISO_8859_7_CtypeTable[256] = {
	0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
	0x4008, 0x420c, 0x4209, 0x4208, 0x4208, 0x4208, 0x4008, 0x4008,
	0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
	0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
	0x4284, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
	0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
	0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0,
	0x78b0, 0x78b0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
	0x41a0, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x74a2,
	0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2,
	0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2,
	0x74a2, 0x74a2, 0x74a2, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x51a0,
	0x41a0, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x70e2,
	0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2,
	0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2,
	0x70e2, 0x70e2, 0x70e2, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x4008,
	0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
	0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
	0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
	0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008,
	0x0284, 0x01a0, 0x01a0, 0x00a0, 0x0000, 0x0000, 0x00a0, 0x00a0,
	0x00a0, 0x00a0, 0x0000, 0x01a0, 0x00a0, 0x01a0, 0x0000, 0x01a0,
	0x00a0, 0x00a0, 0x10a0, 0x10a0, 0x00a0, 0x00a0, 0x34a2, 0x01a0,
	0x34a2, 0x34a2, 0x34a2, 0x01a0, 0x34a2, 0x10a0, 0x34a2, 0x34a2,
	0x30e2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2,
	0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2,
	0x34a2, 0x34a2, 0x0000, 0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x34a2,
	0x34a2, 0x34a2, 0x34a2, 0x34a2, 0x30e2, 0x30e2, 0x30e2, 0x30e2,
	0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2,
	0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2,
	0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2,
	0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x30e2, 0x0000
};

static int mbc_case_fold(OnigCaseFoldType flag,
    const uchar ** pp, const uchar * end ARG_UNUSED, uchar * lower)
{
	const uchar * p = *pp;

	if(CASE_FOLD_IS_NOT_ASCII_ONLY(flag) || ONIGENC_IS_ASCII_CODE(*p))
		*lower = ENC_ISO_8859_7_TO_LOWER_CASE(*p);
	else
		*lower = *p;

	(*pp)++;
	return 1;
}

static int is_code_ctype(OnigCodePoint code, uint ctype)
{
	if(code < 256)
		return ENC_IS_ISO_8859_7_CTYPE(code, ctype);
	else
		return FALSE;
}

static const OnigPairCaseFoldCodes CaseFoldMap[] = {
	{ 0xb6, 0xdc },
	{ 0xb8, 0xdd },
	{ 0xb9, 0xde },
	{ 0xba, 0xdf },
	{ 0xbc, 0xfc },
	{ 0xbe, 0xfd },
	{ 0xbf, 0xfe },

	{ 0xc1, 0xe1 },
	{ 0xc2, 0xe2 },
	{ 0xc3, 0xe3 },
	{ 0xc4, 0xe4 },
	{ 0xc5, 0xe5 },
	{ 0xc6, 0xe6 },
	{ 0xc7, 0xe7 },
	{ 0xc8, 0xe8 },
	{ 0xc9, 0xe9 },
	{ 0xca, 0xea },
	{ 0xcb, 0xeb },
	{ 0xcc, 0xec },
	{ 0xcd, 0xed },
	{ 0xce, 0xee },
	{ 0xcf, 0xef },

	{ 0xd0, 0xf0 },
	{ 0xd1, 0xf1 },
	{ 0xd2, 0xf2 },
	{ 0xd3, 0xf3 },
	{ 0xd4, 0xf4 },
	{ 0xd5, 0xf5 },
	{ 0xd6, 0xf6 },
	{ 0xd7, 0xf7 },
	{ 0xd8, 0xf8 },
	{ 0xd9, 0xf9 },
	{ 0xda, 0xfa },
	{ 0xdb, 0xfb }
};

static int apply_all_case_fold(OnigCaseFoldType flag, OnigApplyAllCaseFoldFunc f, void * arg)
{
	return onigenc_apply_all_case_fold_with_map(SIZEOFARRAY(CaseFoldMap), CaseFoldMap, 0, flag, f, arg);
}

static int get_case_fold_codes_by_str(OnigCaseFoldType flag, const uchar * p, const uchar * end, OnigCaseFoldCodeItem items[])
{
	return onigenc_get_case_fold_codes_by_str_with_map(SIZEOFARRAY(CaseFoldMap), CaseFoldMap, 0, flag, p, end, items);
}

OnigEncodingType OnigEncodingISO_8859_7 = {
	onigenc_single_byte_mbc_enc_len,
	"ISO-8859-7", /* name */
	1,       /* max enc length */
	1,       /* min enc length */
	onigenc_is_mbc_newline_0x0a,
	onigenc_single_byte_mbc_to_code,
	onigenc_single_byte_code_to_mbclen,
	onigenc_single_byte_code_to_mbc,
	mbc_case_fold,
	apply_all_case_fold,
	get_case_fold_codes_by_str,
	onigenc_minimum_property_name_to_ctype,
	is_code_ctype,
	onigenc_not_support_get_ctype_code_range,
	onigenc_single_byte_left_adjust_char_head,
	onigenc_always_true_is_allowed_reverse_match,
	NULL, /* init */
	NULL, /* is_initialized */
	onigenc_always_true_is_valid_mbc_string,
	ENC_FLAG_ASCII_COMPATIBLE|ENC_FLAG_SKIP_OFFSET_1,
	0, 0
};
