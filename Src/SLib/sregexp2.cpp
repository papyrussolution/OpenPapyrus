// SREGEXP2.CPP
// Copyright (c) A.Sobolev 2021, 2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\osf\oniguruma\src\oniguruma.h>

static uint FASTCALL SRE2_FlagsToOnigurumaOptions(uint f)
{
	uint onig_options = 0;
	if(f & SRegExp2::fIgnoreCase) onig_options |= ONIG_OPTION_IGNORECASE;
	if(f & SRegExp2::fExtend) onig_options |= ONIG_OPTION_EXTEND;
	if(f & SRegExp2::fMultiLine) onig_options |= ONIG_OPTION_MULTILINE;
	if(f & SRegExp2::fSingleLine) onig_options |= ONIG_OPTION_SINGLELINE;
	if(f & SRegExp2::fFindLongest) onig_options |= ONIG_OPTION_FIND_LONGEST;
	if(f & SRegExp2::fFindNotEmpty) onig_options |= ONIG_OPTION_FIND_NOT_EMPTY;
	if(f & SRegExp2::fNegateSingleLine) onig_options |= ONIG_OPTION_NEGATE_SINGLELINE;
	if(f & SRegExp2::fDontCaptureGroup) onig_options |= ONIG_OPTION_DONT_CAPTURE_GROUP;
	if(f & SRegExp2::fCaptureGroup) onig_options |= ONIG_OPTION_CAPTURE_GROUP;
	if(f & SRegExp2::fNotBOL) onig_options |= ONIG_OPTION_NOTBOL;
	if(f & SRegExp2::fNotEOL) onig_options |= ONIG_OPTION_NOTEOL;
	if(f & SRegExp2::fPosixRegion) onig_options |= ONIG_OPTION_POSIX_REGION;
	if(f & SRegExp2::fCheckValidityOfString) onig_options |= ONIG_OPTION_CHECK_VALIDITY_OF_STRING;
	if(f & SRegExp2::fNotBeginString) onig_options |= ONIG_OPTION_NOT_BEGIN_STRING;
	if(f & SRegExp2::fNotEndString) onig_options |= ONIG_OPTION_NOT_END_STRING;
	if(f & SRegExp2::fNotBeginPosition) onig_options |= ONIG_OPTION_NOT_BEGIN_POSITION;
	if(f & SRegExp2::fIgnoreCaseIsAscii) onig_options |= ONIG_OPTION_IGNORECASE_IS_ASCII;
	if(f & SRegExp2::fWordIsAscii) onig_options |= ONIG_OPTION_WORD_IS_ASCII;
	if(f & SRegExp2::fDigitIsAscii) onig_options |= ONIG_OPTION_DIGIT_IS_ASCII;
	if(f & SRegExp2::fSpaceIsAscii) onig_options |= ONIG_OPTION_SPACE_IS_ASCII;
	if(f & SRegExp2::fPosixIsAscii) onig_options |= ONIG_OPTION_POSIX_IS_ASCII;
	if(f & SRegExp2::fTextSegmentExtendedGraphemeCluster) onig_options |= ONIG_OPTION_TEXT_SEGMENT_EXTENDED_GRAPHEME_CLUSTER;
	if(f & SRegExp2::fTextSegmentWord) onig_options |= ONIG_OPTION_TEXT_SEGMENT_WORD;
	return onig_options;
}

static OnigEncoding FASTCALL SRE2_EncodingToOnigurumaEncoding(SCodepage cp)
{
	OnigEncoding onig_enc = ONIG_ENCODING_UNDEF;
	switch(cp) {
		case cpANSI:    onig_enc = ONIG_ENCODING_ASCII; break;
		case cpUTF8:    onig_enc = ONIG_ENCODING_UTF8; break;
		case cpUTF16:   onig_enc = ONIG_ENCODING_UTF16_LE; break;
		case cpUTF16BE: onig_enc = ONIG_ENCODING_UTF16_BE; break;
		case cpUTF32:   onig_enc = ONIG_ENCODING_UTF32_LE; break;
		case cpUTF32BE: onig_enc = ONIG_ENCODING_UTF16_BE; break;
		case cpISO_8859_01: onig_enc = ONIG_ENCODING_ISO_8859_1; break;
		case cpISO_8859_02: onig_enc = ONIG_ENCODING_ISO_8859_2; break;
		case cpISO_8859_03: onig_enc = ONIG_ENCODING_ISO_8859_3; break;
		case cpISO_8859_04: onig_enc = ONIG_ENCODING_ISO_8859_4; break;
		case cpISO_8859_05: onig_enc = ONIG_ENCODING_ISO_8859_5; break;
		case cpISO_8859_06: onig_enc = ONIG_ENCODING_ISO_8859_6; break;
		case cpISO_8859_07: onig_enc = ONIG_ENCODING_ISO_8859_7; break;
		case cpISO_8859_08: onig_enc = ONIG_ENCODING_ISO_8859_8; break;
		case cpISO_8859_09: onig_enc = ONIG_ENCODING_ISO_8859_9; break;
		case cpISO_8859_10: onig_enc = ONIG_ENCODING_ISO_8859_10; break;
		case cpISO_8859_11: onig_enc = ONIG_ENCODING_ISO_8859_11; break;
		//case cpISO_8859_12: onig_enc = ONIG_ENCODING_ISO_8859_12; break;
		case cpISO_8859_13: onig_enc = ONIG_ENCODING_ISO_8859_13; break;
		case cpISO_8859_14: onig_enc = ONIG_ENCODING_ISO_8859_14; break;
		case cpISO_8859_15: onig_enc = ONIG_ENCODING_ISO_8859_15; break;
		case cpISO_8859_16: onig_enc = ONIG_ENCODING_ISO_8859_16; break;
		case cp51932: onig_enc = ONIG_ENCODING_EUC_JP; break;
		//case 10001: onig_enc = ONIG_ENCODING_EUC_TW; break;
		case cp51949: onig_enc = ONIG_ENCODING_EUC_KR; break;
		case cp51936: onig_enc = ONIG_ENCODING_EUC_CN; break;
		case cpShiftJIS: onig_enc = ONIG_ENCODING_SJIS; break;
		//case 10001: onig_enc = ONIG_ENCODING_KOI8; break;
		case cpKOI8R: onig_enc = ONIG_ENCODING_KOI8_R; break;
		case cp1251:  onig_enc = ONIG_ENCODING_CP1251; break;
		case cp950:   onig_enc = ONIG_ENCODING_BIG5; break;
		case cp54936: onig_enc = ONIG_ENCODING_GB18030; break;
		default: onig_enc = ONIG_ENCODING_UNDEF;
	}
	return onig_enc;
}

static OnigSyntaxType * FASTCALL SRE2_SyntaxToOnigurumaSyntax(int syntx)
{
	OnigSyntaxType * onig_syntx = ONIG_SYNTAX_DEFAULT;
	switch(syntx) {
		case SRegExp2::syntaxAsis: onig_syntx = ONIG_SYNTAX_ASIS; break;
		case SRegExp2::syntaxPosixBasic: onig_syntx = ONIG_SYNTAX_POSIX_BASIC; break;
		case SRegExp2::syntaxPosixExtended: onig_syntx = ONIG_SYNTAX_POSIX_EXTENDED; break;
		case SRegExp2::syntaxEmacs: onig_syntx = ONIG_SYNTAX_EMACS; break;
		case SRegExp2::syntaxGrep: onig_syntx = ONIG_SYNTAX_GREP; break;
		case SRegExp2::syntaxGnuRegex: onig_syntx = ONIG_SYNTAX_GNU_REGEX; break;
		case SRegExp2::syntaxJava: onig_syntx = ONIG_SYNTAX_JAVA; break;
		case SRegExp2::syntaxPerl: onig_syntx = ONIG_SYNTAX_PERL; break;
		case SRegExp2::syntaxPerlNg: onig_syntx = ONIG_SYNTAX_PERL_NG; break;
		case SRegExp2::syntaxRuby: onig_syntx = ONIG_SYNTAX_RUBY; break;
		case SRegExp2::syntaxOniguruma: onig_syntx = ONIG_SYNTAX_ONIGURUMA; break;
		case SRegExp2::syntaxDefault: onig_syntx = ONIG_SYNTAX_DEFAULT; break;
		default: onig_syntx = ONIG_SYNTAX_DEFAULT; break;
	}
	return onig_syntx;
}

SRegExp2::FindResult & SRegExp2::FindResult::Z()
{
	TSVector <IntRange>::clear();
	return *this;
}

SRegExp2::Error::Error() : Code(0)
{
	SrcTextR.Set(-1);
}

SRegExp2::Error & SRegExp2::Error::Z()
{
	Code = 0;
	SrcTextR.Set(-1);
	return *this;
}

class OnigurumaInitializer {
public:
	OnigurumaInitializer()
	{
		static bool _done = false;
		if(!_done) {
			ENTER_CRITICAL_SECTION
			if(!_done) {
				OnigEncoding enc_list[] = {
					ONIG_ENCODING_ASCII,      
					ONIG_ENCODING_ISO_8859_1,
					ONIG_ENCODING_ISO_8859_2,
					ONIG_ENCODING_ISO_8859_3,
					ONIG_ENCODING_ISO_8859_4,
					ONIG_ENCODING_ISO_8859_5,
					ONIG_ENCODING_ISO_8859_6,
					ONIG_ENCODING_ISO_8859_7,
					ONIG_ENCODING_ISO_8859_8,
					ONIG_ENCODING_ISO_8859_9,
					ONIG_ENCODING_ISO_8859_10,
					ONIG_ENCODING_ISO_8859_11,
					ONIG_ENCODING_ISO_8859_13,
					ONIG_ENCODING_ISO_8859_14,
					ONIG_ENCODING_ISO_8859_15,
					ONIG_ENCODING_ISO_8859_16,
					ONIG_ENCODING_UTF8,
					ONIG_ENCODING_UTF16_BE,
					ONIG_ENCODING_UTF16_LE,   
					ONIG_ENCODING_UTF32_BE,   
					ONIG_ENCODING_UTF32_LE,   
					ONIG_ENCODING_EUC_JP,     
					ONIG_ENCODING_EUC_TW,     
					ONIG_ENCODING_EUC_KR,     
					ONIG_ENCODING_EUC_CN,     
					ONIG_ENCODING_SJIS,       
					ONIG_ENCODING_KOI8,       
					ONIG_ENCODING_KOI8_R,     
					ONIG_ENCODING_CP1251,     
					ONIG_ENCODING_BIG5,       
					ONIG_ENCODING_GB18030
				};
				onig_initialize(enc_list, SIZEOFARRAY(enc_list));
				_done = true;
			}
			LEAVE_CRITICAL_SECTION
		}
	}
};

SRegExp2::SRegExp2() : H(0)
{
	OnigurumaInitializer();
}

SRegExp2::SRegExp2(const char * pPattern, SCodepage cp, int syntax, uint flags) : H(0)
{
	OnigurumaInitializer();
	Compile(pPattern, cp, syntax, flags);
}
	
SRegExp2::~SRegExp2()
{
	Z();
}

SRegExp2 & SRegExp2::Z()
{
	if(H) {
		onig_free(static_cast<OnigRegexType *>(H));
		H = 0;
	}
	return *this;
}
	
int SRegExp2::Compile(const char * pPattern, SCodepage cp, int syntax, uint flags) // Compiles char * --> regexp
{
	int    ok = 1;
	Z();
	LastErr.Z();
	const  size_t pattern_len = sstrlen(pPattern);
	if(pattern_len) {
		OnigErrorInfo einfo;
		OnigRegexType * p_reg = 0;
		OnigOptionType onig_options = SRE2_FlagsToOnigurumaOptions(flags);
		OnigEncoding onig_enc = SRE2_EncodingToOnigurumaEncoding(cp);
		OnigSyntaxType * onig_syntx = SRE2_SyntaxToOnigurumaSyntax(syntax);
		int r = onig_new(&p_reg, (const uchar *)pPattern, (const uchar *)(pPattern + pattern_len), onig_options, onig_enc, onig_syntx, &einfo);
		if(r == ONIG_NORMAL) {
			H = p_reg;
			ok = 1;
		}
		else {
			LastErr.Code = r;
			LastErr.SrcTextR.low = (einfo.par - reinterpret_cast<const uchar *>(pPattern));
			LastErr.SrcTextR.upp = (einfo.par_end - reinterpret_cast<const uchar *>(pPattern));
			ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}
	
bool SRegExp2::Find(const char * pText, size_t textLen, uint flags, FindResult * pResult) const // TRUE if regexp in char * arg
{
	bool   ok = false;
	CALLPTRMEMB(pResult, Z());
	if(H) {
		OnigRegion region;
		onig_region_init(&region);
		//fNotBOL, fNotEOL, fNotBeginString,  fNotEndString, fNotBeginPosition
		// @v11.0.3 remove (fNotBOL|fNotEOL|fNotBeginString|fNotEndString|fNotBeginPosition)
		const uint onig_options = SRE2_FlagsToOnigurumaOptions(flags & (/*fNotBOL|fNotEOL|fNotBeginString|fNotEndString|fNotBeginPosition*/0));
		int r = onig_search(static_cast<regex_t *>(H), reinterpret_cast<const uchar *>(pText), reinterpret_cast<const uchar *>(pText+textLen), 
			reinterpret_cast<const uchar *>(pText), reinterpret_cast<const uchar *>(pText+textLen), &region, onig_options);
		if(r >= 0) {
			if(pResult) {
				for(int i = 0; i < region.num_regs; i++) {
					IntRange ritem(region.beg[i], region.end[i]);
					pResult->insert(&ritem);
				}
			}
			ok = true;
		}
		onig_region_free(&region, 0);
	}
	return ok;
}

bool SRegExp2::Find(SStrScan * pScan, uint flags) const
{
	bool   ok = false;
	const  char * p = pScan ? static_cast<const char *>(*pScan) : 0;
	if(!isempty(p)) {
		const size_t plen = strlen(p);
		FindResult fr;
		if(Find(p, plen, flags, &fr)) {
			assert(fr.getCount());
			if(fr.getCount()) {
				int start = fr.at(0).low;
				int end = fr.at(0).upp;
				assert(start >= 0 && end >= start);
				//int start = fr.at(fr.getCount()-1).low;
				//int end = fr.at(fr.getCount()-1).upp;
				pScan->Incr(start);
				pScan->SetLen(end - start);
			}
			ok = true;
		}
	}
	return ok;
}

bool SRegExp2::Find(const char * pText) const { return Find(pText, sstrlen(pText), 0, 0); }
bool SRegExp2::Find(SStrScan * pScan) const { return Find(pScan, 0); }
SRegExp2::Error SRegExp2::GetLastErr() const { return LastErr; }
bool SRegExp2::IsValid() const { return (H != 0); }
