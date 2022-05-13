// regenc.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2020  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

#define LARGE_S   0x53
#define SMALL_S   0x73

OnigEncoding OnigEncDefaultCharEncoding = ONIG_ENCODING_INIT_DEFAULT;

#define INITED_LIST_SIZE  20

static int InitedListNum;

static struct {
	OnigEncoding enc;
	int inited;
} InitedList[INITED_LIST_SIZE];

static int enc_inited_entry(OnigEncoding enc)
{
	int i;
	for(i = 0; i < InitedListNum; i++) {
		if(InitedList[i].enc == enc) {
			InitedList[i].inited = 1;
			return i;
		}
	}
	i = InitedListNum;
	if(i < INITED_LIST_SIZE - 1) {
		InitedList[i].enc    = enc;
		InitedList[i].inited = 1;
		InitedListNum++;
		return i;
	}
	return -1;
}

static int FASTCALL enc_is_inited(const OnigEncoding enc)
{
	for(int i = 0; i < InitedListNum; i++) {
		if(InitedList[i].enc == enc) {
			return InitedList[i].inited;
		}
	}
	return 0;
}

static int OnigEncInited;

int onigenc_init(void)
{
	if(!OnigEncInited) 
		OnigEncInited = 1;
	return 0;
}

int onigenc_end(void)
{
	for(int i = 0; i < InitedListNum; i++) {
		InitedList[i].enc    = 0;
		InitedList[i].inited = 0;
	}
	InitedListNum = 0;
	OnigEncInited = 0;
	return ONIG_NORMAL;
}

int onig_initialize_encoding(OnigEncoding enc)
{
	int r;
	if(enc != ONIG_ENCODING_ASCII && ONIGENC_IS_ASCII_COMPATIBLE_ENCODING(enc)) {
		OnigEncoding ascii = ONIG_ENCODING_ASCII;
		if(ascii->init != 0 && enc_is_inited(ascii) == 0) {
			r = ascii->init();
			if(r != ONIG_NORMAL) 
				return r;
			enc_inited_entry(ascii);
		}
	}
	if(enc->init != 0 && enc_is_inited(enc) == 0) {
		r = (enc->init)();
		if(r == ONIG_NORMAL)
			enc_inited_entry(enc);
		return r;
	}
	return 0;
}

OnigEncoding onigenc_get_default_encoding(void)
{
	return OnigEncDefaultCharEncoding;
}

int onigenc_set_default_encoding(OnigEncoding enc)
{
	OnigEncDefaultCharEncoding = enc;
	return 0;
}

uchar * onigenc_strdup(OnigEncoding enc, const uchar * s, const uchar * end)
{
	const int slen = (int)(end - s);
	const int term_len = ONIGENC_MBC_MINLEN(enc);
	uchar * r = (uchar *)SAlloc::M(slen + term_len);
	CHECK_NULL_RETURN(r);
	memcpy(r, s, slen);
	for(int i = 0; i < term_len; i++)
		r[slen + i] = (uchar)0;
	return r;
}

uchar * onigenc_get_right_adjust_char_head(OnigEncoding enc, const uchar * start, const uchar * s)
{
	uchar * p = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);
	if(p < s) {
		p += enclen(enc, p);
	}
	return p;
}

uchar * onigenc_get_right_adjust_char_head_with_prev(OnigEncoding enc, const uchar * start, const uchar * s, const uchar ** prev)
{
	uchar * p = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);
	if(p < s) {
		if(prev) 
			*prev = (const uchar *)p;
		p += enclen(enc, p);
	}
	else {
		if(prev)
			*prev = onigenc_get_prev_char_head(enc, start, p);
	}
	return p;
}

uchar * onigenc_get_prev_char_head(OnigEncoding enc, const uchar * start, const uchar * s)
{
	if(s <= start)
		return (uchar *)NULL;
	return ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s - 1);
}

uchar * onigenc_step_back(OnigEncoding enc, const uchar * start, const uchar * s, int n)
{
	while(ONIG_IS_NOT_NULL(s) && n-- > 0) {
		if(s <= start)
			return (uchar *)NULL;
		s = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s - 1);
	}
	return (uchar *)s;
}

uchar * onigenc_step(OnigEncoding enc, const uchar * p, const uchar * end, int n)
{
	uchar * q = (uchar *)p;
	while(n-- > 0) {
		q += ONIGENC_MBC_ENC_LEN(enc, q);
	}
	return (q <= end ? q : NULL);
}

int onigenc_strlen(OnigEncoding enc, const uchar * p, const uchar * end)
{
	int n = 0;
	const uchar * q = p;
	while(q < end) {
		q += ONIGENC_MBC_ENC_LEN(enc, q);
		n++;
	}
	return n;
}

int onigenc_strlen_null(OnigEncoding enc, const uchar * s)
{
	int n = 0;
	uchar * p = (uchar *)s;
	while(1) {
		if(*p == '\0') {
			uchar * q;
			int len = ONIGENC_MBC_MINLEN(enc);
			if(len == 1) 
				return n;
			q = p + 1;
			while(len > 1) {
				if(*q != '\0') 
					break;
				q++;
				len--;
			}
			if(len == 1) return n;
		}
		p += ONIGENC_MBC_ENC_LEN(enc, p);
		n++;
	}
}

int onigenc_str_bytelen_null(OnigEncoding enc, const uchar * s)
{
	const uchar * start = s;
	const uchar * p = s;
	while(1) {
		if(*p == '\0') {
			const uchar * q;
			int len = ONIGENC_MBC_MINLEN(enc);
			if(len == 1) 
				return (int)(p - start);
			q = p + 1;
			while(len > 1) {
				if(*q != '\0') 
					break;
				q++;
				len--;
			}
			if(len == 1) 
				return (int)(p - start);
		}
		p += ONIGENC_MBC_ENC_LEN(enc, p);
	}
}

const uchar OnigEncAsciiToLowerCaseTable[] = {
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
	(uchar)'\260', (uchar)'\261', (uchar)'\262', (uchar)'\263', (uchar)'\264', (uchar)'\265', (uchar)'\266', (uchar)'\267',
	(uchar)'\270', (uchar)'\271', (uchar)'\272', (uchar)'\273', (uchar)'\274', (uchar)'\275', (uchar)'\276', (uchar)'\277',
	(uchar)'\300', (uchar)'\301', (uchar)'\302', (uchar)'\303', (uchar)'\304', (uchar)'\305', (uchar)'\306', (uchar)'\307',
	(uchar)'\310', (uchar)'\311', (uchar)'\312', (uchar)'\313', (uchar)'\314', (uchar)'\315', (uchar)'\316', (uchar)'\317',
	(uchar)'\320', (uchar)'\321', (uchar)'\322', (uchar)'\323', (uchar)'\324', (uchar)'\325', (uchar)'\326', (uchar)'\327',
	(uchar)'\330', (uchar)'\331', (uchar)'\332', (uchar)'\333', (uchar)'\334', (uchar)'\335', (uchar)'\336', (uchar)'\337',
	(uchar)'\340', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\362', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\367',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\374', (uchar)'\375', (uchar)'\376', (uchar)'\377',
};

#ifdef USE_UPPER_CASE_TABLE
const uchar OnigEncAsciiToUpperCaseTable[256] = {
	(uchar)'\000', (uchar)'\001', (uchar)'\002', (uchar)'\003', (uchar)'\004', (uchar)'\005', (uchar)'\006', (uchar)'\007',
	(uchar)'\010', (uchar)'\011', (uchar)'\012', (uchar)'\013', (uchar)'\014', (uchar)'\015', (uchar)'\016', (uchar)'\017',
	(uchar)'\020', (uchar)'\021', (uchar)'\022', (uchar)'\023', (uchar)'\024', (uchar)'\025', (uchar)'\026', (uchar)'\027',
	(uchar)'\030', (uchar)'\031', (uchar)'\032', (uchar)'\033', (uchar)'\034', (uchar)'\035', (uchar)'\036', (uchar)'\037',
	(uchar)'\040', (uchar)'\041', (uchar)'\042', (uchar)'\043', (uchar)'\044', (uchar)'\045', (uchar)'\046', (uchar)'\047',
	(uchar)'\050', (uchar)'\051', (uchar)'\052', (uchar)'\053', (uchar)'\054', (uchar)'\055', (uchar)'\056', (uchar)'\057',
	(uchar)'\060', (uchar)'\061', (uchar)'\062', (uchar)'\063', (uchar)'\064', (uchar)'\065', (uchar)'\066', (uchar)'\067',
	(uchar)'\070', (uchar)'\071', (uchar)'\072', (uchar)'\073', (uchar)'\074', (uchar)'\075', (uchar)'\076', (uchar)'\077',
	(uchar)'\100', (uchar)'\101', (uchar)'\102', (uchar)'\103', (uchar)'\104', (uchar)'\105', (uchar)'\106', (uchar)'\107',
	(uchar)'\110', (uchar)'\111', (uchar)'\112', (uchar)'\113', (uchar)'\114', (uchar)'\115', (uchar)'\116', (uchar)'\117',
	(uchar)'\120', (uchar)'\121', (uchar)'\122', (uchar)'\123', (uchar)'\124', (uchar)'\125', (uchar)'\126', (uchar)'\127',
	(uchar)'\130', (uchar)'\131', (uchar)'\132', (uchar)'\133', (uchar)'\134', (uchar)'\135', (uchar)'\136', (uchar)'\137',
	(uchar)'\140', (uchar)'\101', (uchar)'\102', (uchar)'\103', (uchar)'\104', (uchar)'\105', (uchar)'\106', (uchar)'\107',
	(uchar)'\110', (uchar)'\111', (uchar)'\112', (uchar)'\113', (uchar)'\114', (uchar)'\115', (uchar)'\116', (uchar)'\117',
	(uchar)'\120', (uchar)'\121', (uchar)'\122', (uchar)'\123', (uchar)'\124', (uchar)'\125', (uchar)'\126', (uchar)'\127',
	(uchar)'\130', (uchar)'\131', (uchar)'\132', (uchar)'\173', (uchar)'\174', (uchar)'\175', (uchar)'\176', (uchar)'\177',
	(uchar)'\200', (uchar)'\201', (uchar)'\202', (uchar)'\203', (uchar)'\204', (uchar)'\205', (uchar)'\206', (uchar)'\207',
	(uchar)'\210', (uchar)'\211', (uchar)'\212', (uchar)'\213', (uchar)'\214', (uchar)'\215', (uchar)'\216', (uchar)'\217',
	(uchar)'\220', (uchar)'\221', (uchar)'\222', (uchar)'\223', (uchar)'\224', (uchar)'\225', (uchar)'\226', (uchar)'\227',
	(uchar)'\230', (uchar)'\231', (uchar)'\232', (uchar)'\233', (uchar)'\234', (uchar)'\235', (uchar)'\236', (uchar)'\237',
	(uchar)'\240', (uchar)'\241', (uchar)'\242', (uchar)'\243', (uchar)'\244', (uchar)'\245', (uchar)'\246', (uchar)'\247',
	(uchar)'\250', (uchar)'\251', (uchar)'\252', (uchar)'\253', (uchar)'\254', (uchar)'\255', (uchar)'\256', (uchar)'\257',
	(uchar)'\260', (uchar)'\261', (uchar)'\262', (uchar)'\263', (uchar)'\264', (uchar)'\265', (uchar)'\266', (uchar)'\267',
	(uchar)'\270', (uchar)'\271', (uchar)'\272', (uchar)'\273', (uchar)'\274', (uchar)'\275', (uchar)'\276', (uchar)'\277',
	(uchar)'\300', (uchar)'\301', (uchar)'\302', (uchar)'\303', (uchar)'\304', (uchar)'\305', (uchar)'\306', (uchar)'\307',
	(uchar)'\310', (uchar)'\311', (uchar)'\312', (uchar)'\313', (uchar)'\314', (uchar)'\315', (uchar)'\316', (uchar)'\317',
	(uchar)'\320', (uchar)'\321', (uchar)'\322', (uchar)'\323', (uchar)'\324', (uchar)'\325', (uchar)'\326', (uchar)'\327',
	(uchar)'\330', (uchar)'\331', (uchar)'\332', (uchar)'\333', (uchar)'\334', (uchar)'\335', (uchar)'\336', (uchar)'\337',
	(uchar)'\340', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\362', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\367',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\374', (uchar)'\375', (uchar)'\376', (uchar)'\377',
};
#endif

const ushort OnigEncAsciiCtypeTable[256] = {
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
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

const uchar OnigEncISO_8859_1_ToLowerCaseTable[256] = {
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
	(uchar)'\260', (uchar)'\261', (uchar)'\262', (uchar)'\263', (uchar)'\264', (uchar)'\265', (uchar)'\266', (uchar)'\267',
	(uchar)'\270', (uchar)'\271', (uchar)'\272', (uchar)'\273', (uchar)'\274', (uchar)'\275', (uchar)'\276', (uchar)'\277',
	(uchar)'\340', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\362', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\327',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\374', (uchar)'\375', (uchar)'\376', (uchar)'\337',
	(uchar)'\340', (uchar)'\341', (uchar)'\342', (uchar)'\343', (uchar)'\344', (uchar)'\345', (uchar)'\346', (uchar)'\347',
	(uchar)'\350', (uchar)'\351', (uchar)'\352', (uchar)'\353', (uchar)'\354', (uchar)'\355', (uchar)'\356', (uchar)'\357',
	(uchar)'\360', (uchar)'\361', (uchar)'\362', (uchar)'\363', (uchar)'\364', (uchar)'\365', (uchar)'\366', (uchar)'\367',
	(uchar)'\370', (uchar)'\371', (uchar)'\372', (uchar)'\373', (uchar)'\374', (uchar)'\375', (uchar)'\376', (uchar)'\377'
};

#ifdef USE_UPPER_CASE_TABLE
const uchar OnigEncISO_8859_1_ToUpperCaseTable[256] = {
	(uchar)'\000', (uchar)'\001', (uchar)'\002', (uchar)'\003', (uchar)'\004', (uchar)'\005', (uchar)'\006', (uchar)'\007',
	(uchar)'\010', (uchar)'\011', (uchar)'\012', (uchar)'\013', (uchar)'\014', (uchar)'\015', (uchar)'\016', (uchar)'\017',
	(uchar)'\020', (uchar)'\021', (uchar)'\022', (uchar)'\023', (uchar)'\024', (uchar)'\025', (uchar)'\026', (uchar)'\027',
	(uchar)'\030', (uchar)'\031', (uchar)'\032', (uchar)'\033', (uchar)'\034', (uchar)'\035', (uchar)'\036', (uchar)'\037',
	(uchar)'\040', (uchar)'\041', (uchar)'\042', (uchar)'\043', (uchar)'\044', (uchar)'\045', (uchar)'\046', (uchar)'\047',
	(uchar)'\050', (uchar)'\051', (uchar)'\052', (uchar)'\053', (uchar)'\054', (uchar)'\055', (uchar)'\056', (uchar)'\057',
	(uchar)'\060', (uchar)'\061', (uchar)'\062', (uchar)'\063', (uchar)'\064', (uchar)'\065', (uchar)'\066', (uchar)'\067',
	(uchar)'\070', (uchar)'\071', (uchar)'\072', (uchar)'\073', (uchar)'\074', (uchar)'\075', (uchar)'\076', (uchar)'\077',
	(uchar)'\100', (uchar)'\101', (uchar)'\102', (uchar)'\103', (uchar)'\104', (uchar)'\105', (uchar)'\106', (uchar)'\107',
	(uchar)'\110', (uchar)'\111', (uchar)'\112', (uchar)'\113', (uchar)'\114', (uchar)'\115', (uchar)'\116', (uchar)'\117',
	(uchar)'\120', (uchar)'\121', (uchar)'\122', (uchar)'\123', (uchar)'\124', (uchar)'\125', (uchar)'\126', (uchar)'\127',
	(uchar)'\130', (uchar)'\131', (uchar)'\132', (uchar)'\133', (uchar)'\134', (uchar)'\135', (uchar)'\136', (uchar)'\137',
	(uchar)'\140', (uchar)'\101', (uchar)'\102', (uchar)'\103', (uchar)'\104', (uchar)'\105', (uchar)'\106', (uchar)'\107',
	(uchar)'\110', (uchar)'\111', (uchar)'\112', (uchar)'\113', (uchar)'\114', (uchar)'\115', (uchar)'\116', (uchar)'\117',
	(uchar)'\120', (uchar)'\121', (uchar)'\122', (uchar)'\123', (uchar)'\124', (uchar)'\125', (uchar)'\126', (uchar)'\127',
	(uchar)'\130', (uchar)'\131', (uchar)'\132', (uchar)'\173', (uchar)'\174', (uchar)'\175', (uchar)'\176', (uchar)'\177',
	(uchar)'\200', (uchar)'\201', (uchar)'\202', (uchar)'\203', (uchar)'\204', (uchar)'\205', (uchar)'\206', (uchar)'\207',
	(uchar)'\210', (uchar)'\211', (uchar)'\212', (uchar)'\213', (uchar)'\214', (uchar)'\215', (uchar)'\216', (uchar)'\217',
	(uchar)'\220', (uchar)'\221', (uchar)'\222', (uchar)'\223', (uchar)'\224', (uchar)'\225', (uchar)'\226', (uchar)'\227',
	(uchar)'\230', (uchar)'\231', (uchar)'\232', (uchar)'\233', (uchar)'\234', (uchar)'\235', (uchar)'\236', (uchar)'\237',
	(uchar)'\240', (uchar)'\241', (uchar)'\242', (uchar)'\243', (uchar)'\244', (uchar)'\245', (uchar)'\246', (uchar)'\247',
	(uchar)'\250', (uchar)'\251', (uchar)'\252', (uchar)'\253', (uchar)'\254', (uchar)'\255', (uchar)'\256', (uchar)'\257',
	(uchar)'\260', (uchar)'\261', (uchar)'\262', (uchar)'\263', (uchar)'\264', (uchar)'\265', (uchar)'\266', (uchar)'\267',
	(uchar)'\270', (uchar)'\271', (uchar)'\272', (uchar)'\273', (uchar)'\274', (uchar)'\275', (uchar)'\276', (uchar)'\277',
	(uchar)'\300', (uchar)'\301', (uchar)'\302', (uchar)'\303', (uchar)'\304', (uchar)'\305', (uchar)'\306', (uchar)'\307',
	(uchar)'\310', (uchar)'\311', (uchar)'\312', (uchar)'\313', (uchar)'\314', (uchar)'\315', (uchar)'\316', (uchar)'\317',
	(uchar)'\320', (uchar)'\321', (uchar)'\322', (uchar)'\323', (uchar)'\324', (uchar)'\325', (uchar)'\326', (uchar)'\327',
	(uchar)'\330', (uchar)'\331', (uchar)'\332', (uchar)'\333', (uchar)'\334', (uchar)'\335', (uchar)'\336', (uchar)'\337',
	(uchar)'\300', (uchar)'\301', (uchar)'\302', (uchar)'\303', (uchar)'\304', (uchar)'\305', (uchar)'\306', (uchar)'\307',
	(uchar)'\310', (uchar)'\311', (uchar)'\312', (uchar)'\313', (uchar)'\314', (uchar)'\315', (uchar)'\316', (uchar)'\317',
	(uchar)'\320', (uchar)'\321', (uchar)'\322', (uchar)'\323', (uchar)'\324', (uchar)'\325', (uchar)'\326', (uchar)'\367',
	(uchar)'\330', (uchar)'\331', (uchar)'\332', (uchar)'\333', (uchar)'\334', (uchar)'\335', (uchar)'\336', (uchar)'\377',
};
#endif

void onigenc_set_default_caseconv_table(const uchar * table ARG_UNUSED)
{
	/* nothing */
	/* obsoleted. */
}

uchar * onigenc_get_left_adjust_char_head(OnigEncoding enc, const uchar * start, const uchar * s)
{
	return ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);
}

const OnigPairCaseFoldCodes OnigAsciiLowerMap[] = {
	{ 0x41, 0x61 },
	{ 0x42, 0x62 },
	{ 0x43, 0x63 },
	{ 0x44, 0x64 },
	{ 0x45, 0x65 },
	{ 0x46, 0x66 },
	{ 0x47, 0x67 },
	{ 0x48, 0x68 },
	{ 0x49, 0x69 },
	{ 0x4a, 0x6a },
	{ 0x4b, 0x6b },
	{ 0x4c, 0x6c },
	{ 0x4d, 0x6d },
	{ 0x4e, 0x6e },
	{ 0x4f, 0x6f },
	{ 0x50, 0x70 },
	{ 0x51, 0x71 },
	{ 0x52, 0x72 },
	{ 0x53, 0x73 },
	{ 0x54, 0x74 },
	{ 0x55, 0x75 },
	{ 0x56, 0x76 },
	{ 0x57, 0x77 },
	{ 0x58, 0x78 },
	{ 0x59, 0x79 },
	{ 0x5a, 0x7a }
};

int onigenc_ascii_apply_all_case_fold(OnigCaseFoldType flag ARG_UNUSED, OnigApplyAllCaseFoldFunc f, void * arg)
{
	for(int i = 0; i < (int)(sizeof(OnigAsciiLowerMap)/sizeof(OnigPairCaseFoldCodes)); i++) {
		OnigCodePoint code = OnigAsciiLowerMap[i].to;
		int r = (*f)(OnigAsciiLowerMap[i].from, &code, 1, arg);
		if(r) 
			return r;
		code = OnigAsciiLowerMap[i].from;
		r = (*f)(OnigAsciiLowerMap[i].to, &code, 1, arg);
		if(r) 
			return r;
	}
	return 0;
}

int onigenc_ascii_get_case_fold_codes_by_str(OnigCaseFoldType flag ARG_UNUSED, const uchar * p, const uchar * end ARG_UNUSED, OnigCaseFoldCodeItem items[])
{
	if(0x41 <= *p && *p <= 0x5a) {
		items[0].byte_len = 1;
		items[0].code_len = 1;
		items[0].code[0] = (OnigCodePoint)(*p + 0x20);
		return 1;
	}
	else if(0x61 <= *p && *p <= 0x7a) {
		items[0].byte_len = 1;
		items[0].code_len = 1;
		items[0].code[0] = (OnigCodePoint)(*p - 0x20);
		return 1;
	}
	else
		return 0;
}

static int ss_apply_all_case_fold(OnigCaseFoldType flag ARG_UNUSED, OnigApplyAllCaseFoldFunc f, void * arg)
{
	static OnigCodePoint ss[] = { SMALL_S, SMALL_S };
	return (*f)((OnigCodePoint)0xdf, ss, 2, arg);
}

int onigenc_apply_all_case_fold_with_map(int map_size, const OnigPairCaseFoldCodes map[], int ess_tsett_flag, OnigCaseFoldType flag, OnigApplyAllCaseFoldFunc f, void * arg)
{
	OnigCodePoint code;
	int i;
	int r = onigenc_ascii_apply_all_case_fold(flag, f, arg);
	if(r) 
		return r;
	if(CASE_FOLD_IS_ASCII_ONLY(flag))
		return 0;
	for(i = 0; i < map_size; i++) {
		code = map[i].to;
		r = (*f)(map[i].from, &code, 1, arg);
		if(r) return r;
		code = map[i].from;
		r = (*f)(map[i].to, &code, 1, arg);
		if(r) return r;
	}
	if(ess_tsett_flag != 0)
		return ss_apply_all_case_fold(flag, f, arg);
	return 0;
}

int onigenc_get_case_fold_codes_by_str_with_map(int map_size, const OnigPairCaseFoldCodes map[],
    int ess_tsett_flag, OnigCaseFoldType flag, const uchar * p, const uchar * end, OnigCaseFoldCodeItem items[])
{
	int i, j, n;
	static uchar sa[] = { LARGE_S, SMALL_S };
	if(0x41 <= *p && *p <= 0x5a) { /* A - Z */
		if(*p == LARGE_S && ess_tsett_flag != 0 && end > p + 1 && (*(p+1) == LARGE_S || *(p+1) == SMALL_S) /* SS */ && CASE_FOLD_IS_NOT_ASCII_ONLY(flag)) {
ss_combination:
			items[0].byte_len = 2;
			items[0].code_len = 1;
			items[0].code[0] = (OnigCodePoint)0xdf;
			n = 1;
			for(i = 0; i < 2; i++) {
				for(j = 0; j < 2; j++) {
					if(sa[i] == *p && sa[j] == *(p+1))
						continue;

					items[n].byte_len = 2;
					items[n].code_len = 2;
					items[n].code[0] = (OnigCodePoint)sa[i];
					items[n].code[1] = (OnigCodePoint)sa[j];
					n++;
				}
			}
			return 4;
		}

		items[0].byte_len = 1;
		items[0].code_len = 1;
		items[0].code[0] = (OnigCodePoint)(*p + 0x20);
		return 1;
	}
	else if(0x61 <= *p && *p <= 0x7a) { /* a - z */
		if(*p == SMALL_S && ess_tsett_flag != 0 && end > p + 1
		 && (*(p+1) == SMALL_S || *(p+1) == LARGE_S)
		 && CASE_FOLD_IS_NOT_ASCII_ONLY(flag)) {
			goto ss_combination;
		}

		items[0].byte_len = 1;
		items[0].code_len = 1;
		items[0].code[0] = (OnigCodePoint)(*p - 0x20);
		return 1;
	}
	else if(*p == 0xdf && ess_tsett_flag != 0
	 && CASE_FOLD_IS_NOT_ASCII_ONLY(flag)) {
		items[0].byte_len = 1;
		items[0].code_len = 2;
		items[0].code[0] = (OnigCodePoint)'s';
		items[0].code[1] = (OnigCodePoint)'s';

		items[1].byte_len = 1;
		items[1].code_len = 2;
		items[1].code[0] = (OnigCodePoint)'S';
		items[1].code[1] = (OnigCodePoint)'S';

		items[2].byte_len = 1;
		items[2].code_len = 2;
		items[2].code[0] = (OnigCodePoint)'s';
		items[2].code[1] = (OnigCodePoint)'S';

		items[3].byte_len = 1;
		items[3].code_len = 2;
		items[3].code[0] = (OnigCodePoint)'S';
		items[3].code[1] = (OnigCodePoint)'s';

		return 4;
	}
	else {
		int i;

		if(CASE_FOLD_IS_ASCII_ONLY(flag))
			return 0;

		for(i = 0; i < map_size; i++) {
			if(*p == map[i].from) {
				items[0].byte_len = 1;
				items[0].code_len = 1;
				items[0].code[0] = map[i].to;
				return 1;
			}
			else if(*p == map[i].to) {
				items[0].byte_len = 1;
				items[0].code_len = 1;
				items[0].code[0] = map[i].from;
				return 1;
			}
		}
	}

	return 0;
}

int onigenc_not_support_get_ctype_code_range(OnigCtype ctype ARG_UNUSED, OnigCodePoint* sb_out ARG_UNUSED, const OnigCodePoint* ranges[] ARG_UNUSED)
{
	return ONIG_NO_SUPPORT_CONFIG;
}

int onigenc_is_mbc_newline_0x0a(const uchar * p, const uchar * end)
{
	if(p < end) {
		if(*p == NEWLINE_CODE) return 1;
	}
	return 0;
}

/* for single byte encodings */
int onigenc_ascii_mbc_case_fold(OnigCaseFoldType flag ARG_UNUSED, const uchar ** p,
    const uchar * end ARG_UNUSED, uchar * lower)
{
	*lower = ONIGENC_ASCII_CODE_TO_LOWER_CASE(**p);

	(*p)++;
	return 1; /* return byte length of converted char to lower */
}

int onigenc_single_byte_mbc_enc_len(const uchar * p ARG_UNUSED)
{
	return 1;
}

OnigCodePoint onigenc_single_byte_mbc_to_code(const uchar * p, const uchar * end ARG_UNUSED)
{
	return (OnigCodePoint)(*p);
}

int onigenc_single_byte_code_to_mbclen(OnigCodePoint code ARG_UNUSED)
{
	return (code < 0x100 ? 1 : ONIGERR_INVALID_CODE_POINT_VALUE);
}

int onigenc_single_byte_code_to_mbc(OnigCodePoint code, uchar * buf)
{
	*buf = (uchar)(code & 0xff);
	return 1;
}

uchar * onigenc_single_byte_left_adjust_char_head(const uchar * start ARG_UNUSED, const uchar * s)
{
	return (uchar *)s;
}

int onigenc_always_true_is_allowed_reverse_match(const uchar * s ARG_UNUSED, const uchar * end ARG_UNUSED)
{
	return TRUE;
}

int onigenc_always_false_is_allowed_reverse_match(const uchar * s ARG_UNUSED, const uchar * end ARG_UNUSED)
{
	return FALSE;
}

int onigenc_always_true_is_valid_mbc_string(const uchar * s ARG_UNUSED, const uchar * end ARG_UNUSED)
{
	return TRUE;
}

int onigenc_length_check_is_valid_mbc_string(OnigEncoding enc, const uchar * p, const uchar * end)
{
	while(p < end) {
		p += enclen(enc, p);
	}
	if(p != end)
		return FALSE;
	else
		return TRUE;
}

int onigenc_is_valid_mbc_string(OnigEncoding enc, const uchar * s, const uchar * end)
{
	return ONIGENC_IS_VALID_MBC_STRING(enc, s, end);
}

OnigCodePoint onigenc_mbn_mbc_to_code(OnigEncoding enc, const uchar * p, const uchar * end)
{
	int c, i;
	int len = enclen(enc, p);
	OnigCodePoint n = (OnigCodePoint)(*p++);
	if(len == 1) 
		return n;
	for(i = 1; i < len; i++) {
		if(p >= end) break;
		c = *p++;
		n <<= 8;  n += c;
	}
	return n;
}

int onigenc_mbn_mbc_case_fold(OnigEncoding enc, OnigCaseFoldType flag ARG_UNUSED,
    const uchar ** pp, const uchar * end ARG_UNUSED, uchar * lower)
{
	int len;
	const uchar * p = *pp;
	if(ONIGENC_IS_MBC_ASCII(p)) {
		*lower = ONIGENC_ASCII_CODE_TO_LOWER_CASE(*p);
		(*pp)++;
		return 1;
	}
	else {
		int i;
		len = enclen(enc, p);
		for(i = 0; i < len; i++) {
			*lower++ = *p++;
		}
		(*pp) += len;
		return len; /* return byte length of converted to lower char */
	}
}

int onigenc_mb2_code_to_mbc(OnigEncoding enc, OnigCodePoint code, uchar * buf)
{
	uchar * p = buf;
	if((code & 0xff00) != 0) {
		*p++ = (uchar)((code >>  8) & 0xff);
	}
	*p++ = (uchar)(code & 0xff);
#if 1
	if(enclen(enc, buf) != (p - buf))
		return ONIGERR_INVALID_CODE_POINT_VALUE;
#endif
	return (int)(p - buf);
}

int onigenc_mb4_code_to_mbc(OnigEncoding enc, OnigCodePoint code, uchar * buf)
{
	uchar * p = buf;
	if((code & 0xff000000) != 0) {
		*p++ = (uchar)((code >> 24) & 0xff);
	}
	if((code & 0xff0000) != 0 || p != buf) {
		*p++ = (uchar)((code >> 16) & 0xff);
	}
	if((code & 0xff00) != 0 || p != buf) {
		*p++ = (uchar)((code >> 8) & 0xff);
	}
	*p++ = (uchar)(code & 0xff);

#if 1
	if(enclen(enc, buf) != (p - buf))
		return ONIGERR_INVALID_CODE_POINT_VALUE;
#endif
	return (int)(p - buf);
}

int onigenc_minimum_property_name_to_ctype(OnigEncoding enc, uchar * p, uchar * end)
{
	static PosixBracketEntryType PBS[] = {
		{ (uchar *)"Alnum",  ONIGENC_CTYPE_ALNUM,  5 },
		{ (uchar *)"Alpha",  ONIGENC_CTYPE_ALPHA,  5 },
		{ (uchar *)"Blank",  ONIGENC_CTYPE_BLANK,  5 },
		{ (uchar *)"Cntrl",  ONIGENC_CTYPE_CNTRL,  5 },
		{ (uchar *)"Digit",  ONIGENC_CTYPE_DIGIT,  5 },
		{ (uchar *)"Graph",  ONIGENC_CTYPE_GRAPH,  5 },
		{ (uchar *)"Lower",  ONIGENC_CTYPE_LOWER,  5 },
		{ (uchar *)"Print",  ONIGENC_CTYPE_PRINT,  5 },
		{ (uchar *)"Punct",  ONIGENC_CTYPE_PUNCT,  5 },
		{ (uchar *)"Space",  ONIGENC_CTYPE_SPACE,  5 },
		{ (uchar *)"Upper",  ONIGENC_CTYPE_UPPER,  5 },
		{ (uchar *)"XDigit", ONIGENC_CTYPE_XDIGIT, 6 },
		{ (uchar *)"ASCII",  ONIGENC_CTYPE_ASCII,  5 },
		{ (uchar *)"Word",   ONIGENC_CTYPE_WORD,   4 },
		{ (uchar *)NULL, -1, 0 }
	};
	int len = onigenc_strlen(enc, p, end);
	for(PosixBracketEntryType * pb = PBS; IS_NOT_NULL(pb->name); pb++) {
		if(len == pb->len &&
		    onigenc_with_ascii_strncmp(enc, p, end, pb->name, pb->len) == 0)
			return pb->ctype;
	}
	return ONIGERR_INVALID_CHAR_PROPERTY_NAME;
}

int onigenc_is_mbc_word_ascii(OnigEncoding enc, uchar * s, const uchar * end)
{
	OnigCodePoint code = ONIGENC_MBC_TO_CODE(enc, s, end);
	if(code > ASCII_LIMIT) 
		return 0;
	return ONIGENC_IS_ASCII_CODE_WORD(code);
}

int onigenc_mb2_is_code_ctype(OnigEncoding enc, OnigCodePoint code, uint ctype)
{
	if(code < 128)
		return ONIGENC_IS_ASCII_CODE_CTYPE(code, ctype);
	else {
		if(CTYPE_IS_WORD_GRAPH_PRINT(ctype)) {
			return (ONIGENC_CODE_TO_MBCLEN(enc, code) > 1 ? TRUE : FALSE);
		}
	}
	return FALSE;
}

int onigenc_mb4_is_code_ctype(OnigEncoding enc, OnigCodePoint code, uint ctype)
{
	if(code < 128)
		return ONIGENC_IS_ASCII_CODE_CTYPE(code, ctype);
	else {
		if(CTYPE_IS_WORD_GRAPH_PRINT(ctype)) {
			return (ONIGENC_CODE_TO_MBCLEN(enc, code) > 1 ? TRUE : FALSE);
		}
	}
	return FALSE;
}

int onigenc_with_ascii_strncmp(OnigEncoding enc, const uchar * p, const uchar * end, const uchar * sascii /* ascii */, int n)
{
	int x, c;
	while(n-- > 0) {
		if(p >= end) 
			return (int)(*sascii);
		c = (int)ONIGENC_MBC_TO_CODE(enc, p, end);
		x = *sascii - c;
		if(x) 
			return x;
		sascii++;
		p += enclen(enc, p);
	}
	return 0;
}

int onig_codes_cmp(OnigCodePoint a[], OnigCodePoint b[], int n)
{
	for(int i = 0; i < n; i++) {
		if(a[i] != b[i])
			return -1;
	}
	return 0;
}

int onig_codes_byte_at(OnigCodePoint codes[], int at)
{
	int index = at / 3;
	int b     = at % 3;
	OnigCodePoint code = codes[index];
	return ((code >> ((2 - b) * 8)) & 0xff);
}
