// GNUPLOT - encoding.c 
// Copyright 2018   Thomas Williams, Colin Kelley
//
#include <gnuplot.h>
#pragma hdrstop
#ifdef HAVE_ICONV
	#include <iconv.h>
#endif
#ifdef HAVE_LANGINFO_H
	#include <langinfo.h>
#endif
#if defined(_WIN32)
	static enum set_encoding_id map_codepage_to_encoding(uint cp);
#endif
//static const char * encoding_micro();
//static const char * encoding_minus();
//static void set_degreesign(char *);
//static bool utf8_getmore(ulong * wch, const char ** str, int nbytes);
//
// encoding functions
//
//void init_encoding()
void GnuPlot::InitEncoding()
{
	GPT._Encoding = encoding_from_locale();
	if(GPT._Encoding == S_ENC_INVALID)
		GPT._Encoding = S_ENC_DEFAULT;
	InitSpecialChars();
}

enum set_encoding_id encoding_from_locale()                     
{
	char * l = NULL;
	enum set_encoding_id encoding = S_ENC_INVALID;
#if defined(_WIN32) || defined(MSDOS) || defined(OS2)
#ifdef HAVE_LOCALE_H
	char * cp_str;
	l = setlocale(LC_CTYPE, "");
	// preserve locale string, skip language information 
	if(l && (cp_str = strchr(l, '.')) != NULL) {
		cp_str++; /* Step past the dot in, e.g., German_Germany.1252 */
		uint cp = strtoul(cp_str, NULL, 10);
		if(cp != 0)
			encoding = map_codepage_to_encoding(cp);
	}
#endif
#ifdef _WIN32
	// get encoding from currently active codepage 
	if(encoding == S_ENC_INVALID) {
#ifndef WGP_CONSOLE
		encoding = map_codepage_to_encoding(GetACP());
#else
		encoding = map_codepage_to_encoding(GetConsoleCP());
#endif
	}
#endif
#elif defined(HAVE_LOCALE_H)
	if(encoding == S_ENC_INVALID) {
		l = setlocale(LC_CTYPE, "");
		if(l && (strstr(l, "utf") || strstr(l, "UTF")))
			encoding = S_ENC_UTF8;
		if(l && (strstr(l, "sjis") || strstr(l, "SJIS") || strstr(l, "932")))
			encoding = S_ENC_SJIS;
		if(l && (strstr(l, "850") || strstr(l, "858")))
			encoding = S_ENC_CP850;
		if(l && (strstr(l, "437")))
			encoding = S_ENC_CP437;
		if(l && (strstr(l, "852")))
			encoding = S_ENC_CP852;
		if(l && (strstr(l, "1250")))
			encoding = S_ENC_CP1250;
		if(l && (strstr(l, "1251")))
			encoding = S_ENC_CP1251;
		if(l && (strstr(l, "1252")))
			encoding = S_ENC_CP1252;
		if(l && (strstr(l, "1254")))
			encoding = S_ENC_CP1254;
		if(l && (strstr(l, "950")))
			encoding = S_ENC_CP950;
		/* FIXME: "set encoding locale" has only limited support on non-Windows systems */
	}
#endif
	return encoding;
}
//
// Encoding-specific character enabled by "set micro" 
//
static const char * encoding_micro()
{
	static const char micro_utf8[4]    = { '\xC2', '\xB5', '\x00', '\x00' };
	static const char micro_437[2]     = { '\xE6', '\x00' };
	static const char micro_latin1[2]  = { '\xB5', '\x00' };
	static const char micro_default[2] = { 'u',  '\x00' };
	switch(GPT._Encoding) {
		case S_ENC_UTF8:        return micro_utf8;
		case S_ENC_CP1250:
		case S_ENC_CP1251:
		case S_ENC_CP1252:
		case S_ENC_CP1254:
		case S_ENC_ISO8859_1:
		case S_ENC_ISO8859_9:
		case S_ENC_ISO8859_15:  return micro_latin1;
		case S_ENC_CP437:
		case S_ENC_CP850:       return micro_437;
		default:                return micro_default;
	}
}
//
// Encoding-specific character enabled by "set minussign" 
//
static const char * encoding_minus()
{
	static const char minus_utf8[4] = { '\xE2', '\x88', '\x92', '\x00' };
	static const char minus_1252[2] = { '\x96', '\x00' };
	// NB: This SJIS character is correct, but produces bad spacing if used
	//  static const char minus_sjis[4] = {0x81, 0x7c, 0x0, 0x0};
	switch(GPT._Encoding) {
		case S_ENC_UTF8:        return minus_utf8;
		case S_ENC_CP1252:      return minus_1252;
		case S_ENC_SJIS:
		default:                return NULL;
	}
}
//void init_special_chars()
void GnuPlot::InitSpecialChars()
{
	// Set degree sign to match encoding 
	char * l = NULL;
#ifdef HAVE_LOCALE_H
	l = setlocale(LC_CTYPE, "");
#endif
	SetDegreeSign(l);
	GpU.minus_sign = encoding_minus(); // Set minus sign to match encoding 
	GpU.micro = encoding_micro(); // Set micro character to match encoding 
}

//static void set_degreesign(char * locale)
void GnuPlot::SetDegreeSign(char * locale)
{
#if defined(HAVE_ICONV) && !(defined _WIN32)
	char degree_utf8[3] = {'\302', '\260', '\0'};
	size_t lengthin = 3;
	size_t lengthout = 8;
	char * in = degree_utf8;
	char * out = GpU.degree_sign;
	iconv_t cd;
	if(locale) {
		// This should work even if gnuplot doesn't understand the encoding 
#ifdef HAVE_LANGINFO_H
		char * cencoding = nl_langinfo(CODESET);
#else
		char * cencoding = strchr(locale, '.');
		if(cencoding)
			cencoding++; /* Step past the dot in, e.g., ja_JP.EUC-JP */
#endif
		if(cencoding) {
			if(sstreq(cencoding, "UTF-8"))
				strcpy(GpU.degree_sign, degree_utf8);
			else if((cd = iconv_open(cencoding, "UTF-8")) == (iconv_t)(-1))
				IntWarn(NO_CARET, "iconv_open failed for %s", cencoding);
			else {
				if(iconv(cd, &in, &lengthin, &out, &lengthout) == (size_t)(-1))
					IntWarn(NO_CARET, "iconv failed to convert degree sign");
				iconv_close(cd);
			}
		}
		return;
	}
#endif
	// These are the internally-known encodings 
	memzero(GpU.degree_sign, sizeof(GpU.degree_sign));
	switch(GPT._Encoding) {
		case S_ENC_UTF8:    GpU.degree_sign[0] = '\302'; GpU.degree_sign[1] = '\260'; break;
		case S_ENC_KOI8_R:
		case S_ENC_KOI8_U:  GpU.degree_sign[0] = '\234'; break;
		case S_ENC_CP437:
		case S_ENC_CP850:
		case S_ENC_CP852:   GpU.degree_sign[0] = '\370'; break;
		case S_ENC_SJIS:    break;/* should be 0x818B */
		case S_ENC_CP950:   break;/* should be 0xA258 */
		// default applies at least to: ISO8859-1, -2, -9, -15, CP1250, CP1251, CP1252, CP1254
		default: GpU.degree_sign[0] = '\260'; break;
	}
}

#if defined(_WIN32) || defined(MSDOS) || defined(OS2)
static enum set_encoding_id map_codepage_to_encoding(uint cp)                            
{
	enum set_encoding_id encoding;
	// The code below is the inverse to the code found in WinGetCodepage().
	// For a list of code page identifiers see
	// http://msdn.microsoft.com/en-us/library/dd317756%28v=vs.85%29.aspx
	switch(cp) {
		case 437:   encoding = S_ENC_CP437; break;
		case 850:
		case 858:   encoding = S_ENC_CP850; break;/* 850 with Euro sign */
		case 852:   encoding = S_ENC_CP852; break;
		case 932:   encoding = S_ENC_SJIS; break;
		case 950:   encoding = S_ENC_CP950; break;
		case 1250:  encoding = S_ENC_CP1250; break;
		case 1251:  encoding = S_ENC_CP1251; break;
		case 1252:  encoding = S_ENC_CP1252; break;
		case 1254:  encoding = S_ENC_CP1254; break;
		case 20866: encoding = S_ENC_KOI8_R; break;
		case 21866: encoding = S_ENC_KOI8_U; break;
		case 28591: encoding = S_ENC_ISO8859_1; break;
		case 28592: encoding = S_ENC_ISO8859_2; break;
		case 28599: encoding = S_ENC_ISO8859_9; break;
		case 28605: encoding = S_ENC_ISO8859_15; break;
		case 65001: encoding = S_ENC_UTF8; break;
		default:
		    encoding = S_ENC_DEFAULT;
	}
	return encoding;
}
#endif

//const char * latex_input_encoding(enum set_encoding_id encoding)
const char * GnuPlot::LatexInputEncoding(enum set_encoding_id encoding)
{
	const char * inputenc = NULL;
	switch(encoding) {
		case S_ENC_DEFAULT: break;
		case S_ENC_ISO8859_1: inputenc = "latin1"; break;
		case S_ENC_ISO8859_2: inputenc = "latin2"; break;
		case S_ENC_ISO8859_9: inputenc = "latin5"; break; /* ISO8859-9 is Latin5 */
		case S_ENC_ISO8859_15: inputenc = "latin9"; break; /* ISO8859-15 is Latin9 */
		case S_ENC_CP437: inputenc = "cp437de"; break;
		case S_ENC_CP850: inputenc = "cp850"; break;
		case S_ENC_CP852: inputenc = "cp852"; break;
		case S_ENC_CP1250: inputenc = "cp1250"; break;
		case S_ENC_CP1251: inputenc = "cp1251"; break;
		case S_ENC_CP1252: inputenc = "cp1252"; break;
		case S_ENC_KOI8_R: inputenc = "koi8-r"; break;
		case S_ENC_KOI8_U: inputenc = "koi8-u"; break;
		case S_ENC_UTF8: inputenc = "utf8x"; break; /* utf8x (not utf8) is needed to pick up degree and micro signs */
		case S_ENC_INVALID: IntError(NO_CARET, "invalid input encoding used"); break;
		default: break; /* do nothing */
	}
	return inputenc;
}

bool contains8bit(const char * s)
{
	while(*s) {
		if((*s++ & 0x80))
			return TRUE;
	}
	return FALSE;
}
/*
 * UTF-8 functions
 */
#define INVALID_UTF8 0xFFFDul
//
// Read from second byte to end of UTF-8 sequence.
// used by utf8toulong()
//
static bool utf8_getmore(ulong * wch, const char ** str, int nbytes)
{
	const ulong minvalue[] = {0x80, 0x800, 0x10000, 0x200000, 0x4000000};
	for(int i = 0; i < nbytes; i++) {
		uchar c = (uchar)**str;
		if((c & 0xc0) != 0x80) {
			*wch = INVALID_UTF8;
			return FALSE;
		}
		*wch = (*wch << 6) | (c & 0x3f);
		(*str)++;
	}
	// check for overlong UTF-8 sequences 
	if(*wch < minvalue[nbytes-1]) {
		*wch = INVALID_UTF8;
		return FALSE;
	}
	return TRUE;
}

/* Convert UTF-8 multibyte sequence from string to unsigned long character.
 * Returns TRUE on success.
 */
bool utf8toulong(ulong * wch, const char ** str)
{
	uchar c =  (uchar)*(*str)++;
	if((c & 0x80) == 0) {
		*wch = (ulong)c;
		return TRUE;
	}
	if((c & 0xe0) == 0xc0) {
		*wch = c & 0x1f;
		return utf8_getmore(wch, str, 1);
	}
	if((c & 0xf0) == 0xe0) {
		*wch = c & 0x0f;
		return utf8_getmore(wch, str, 2);
	}
	if((c & 0xf8) == 0xf0) {
		*wch = c & 0x07;
		return utf8_getmore(wch, str, 3);
	}
	/* Note: 5 and 6 byte UTF8 sequences are no longer valid
	 *       according to RFC 3629 (Nov 2003)
	 */
#if (0)
	if((c & 0xfc) == 0xf8) {
		*wch = c & 0x03;
		return utf8_getmore(wch, str, 4);
	}
	if((c & 0xfe) == 0xfc) {
		*wch = c & 0x01;
		return utf8_getmore(wch, str, 5);
	}
#endif
	*wch = INVALID_UTF8;
	return FALSE;
}

/*
 * Convert unicode codepoint to UTF-8
 * returns number of bytes in the UTF-8 representation
 */
int ucs4toutf8(uint32_t codepoint, uchar * utf8char)
{
	int length = 0;

	if(codepoint <= 0x7F) {
		utf8char[0] = codepoint;
		length = 1;
	}
	else if(codepoint <= 0x7FF) {
		utf8char[0] = 0xC0 | (codepoint >> 6);    /* 110xxxxx */
		utf8char[1] = 0x80 | (codepoint & 0x3F);  /* 10xxxxxx */
		length = 2;
	}
	else if(codepoint <= 0xFFFF) {
		utf8char[0] = 0xE0 | (codepoint >> 12);   /* 1110xxxx */
		utf8char[1] = 0x80 | ((codepoint >> 6) & 0x3F); /* 10xxxxxx */
		utf8char[2] = 0x80 | (codepoint & 0x3F);  /* 10xxxxxx */
		length = 3;
	}
	else if(codepoint <= 0x10FFFF) {
		utf8char[0] = 0xF0 | (codepoint >> 18);   /* 11110xxx */
		utf8char[1] = 0x80 | ((codepoint >> 12) & 0x3F); /* 10xxxxxx */
		utf8char[2] = 0x80 | ((codepoint >> 6) & 0x3F); /* 10xxxxxx */
		utf8char[3] = 0x80 | (codepoint & 0x3F);  /* 10xxxxxx */
		length = 4;
	}

	return length;
}

/*
 * Returns number of (possibly multi-byte) characters in a UTF-8 string
 * FIXME: reject/ignore/warn on invalid byte sequences?
 */
size_t strlen_utf8(const char * s)
{
	int i = 0, j = 0;
	while(s[i]) {
		if((s[i] & 0xc0) != 0x80) j++;
		i++;
	}
	return j;
}
/*
 * This operation is used in several places to reduce 8 chars of
 * input to a single utf8 character used for PT_CHARACTER.
 * Replaces content of original string.
 * "AB" -> "A"
 * "\U+0041" -> "A"
 * "\316\261" -> utf8 "ɑ" (alpha) (unchanged from input)
 *               the scanner already replaced string "\316" with octal byte 0316
 */
void truncate_to_one_utf8_char(char * orig)
{
	uint32_t codepoint;
	char newchar[8];
	int length = 0;
	strnzcpy(newchar, orig, sizeof(newchar));
	// Check for unicode escape 
	if(!strncmp("\\U+", newchar, 3)) {
		if(sscanf(&newchar[3], "%4x", &codepoint) == 1)
			length = ucs4toutf8(codepoint, (uchar *)newchar);
		newchar[length] = '\0';
	}
	// Truncate ascii text to single character 
	else if((newchar[0] & 0x80) == 0)
		newchar[1] = '\0';
	// Some other 8-bit sequence (we don't check for valid utf8) 
	else {
		newchar[7] = '\0';
		for(length = 1; length<7; length++)
			if((newchar[length] & 0xC0) != 0x80) {
				newchar[length] = '\0';
				break;
			}
	}

	/* overwrite original string */
	strcpy(orig, newchar);
}
/*
 * S-JIS functions
 */
bool is_sjis_lead_byte(char c)
{
	uint ch = (uchar)c;
	return ((ch >= 0x81) && (ch <= 0x9f)) || ((ch >= 0xe1) && (ch <= 0xee));
}

size_t strlen_sjis(const char * s)
{
	int i = 0, j = 0;
	while(s[i]) {
		if(is_sjis_lead_byte(s[i])) i++; /* skip */
		j++;
		i++;
	}
	return j;
}
