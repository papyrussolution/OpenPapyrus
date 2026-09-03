// SNTOK.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
//
// email regexp: (?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])
//
// {9}-[*]
//
SNaturalTokenStat::SNaturalTokenStat() : Len(0), Seq(0)
{
}

SNaturalTokenStat & SNaturalTokenStat::Z()
{
	Len = 0;
	Seq = 0;
	ChrList.clear(); // @v11.9.7
	return *this;
}

bool SNaturalTokenStat::IsChrListAsciiPatternMatched(const char * pPattern) const
{
	bool   result = true;
	const  size_t plen = sstrlen(pPattern);
	if(plen) {
		const  uint clcount = ChrList.getCount();
		for(uint i = 0; result && i < clcount; i++) {
			const  char c = static_cast<char>(ChrList.at(i).Key);
			if(!sstrchr(pPattern, c)) {
				result = false;
			}
		}
	}
	else
		result = false;
	return result;
}

static const SIntToSymbTabEntry SNTokSymb_List[] = {
	{ SNTOK_NATURALWORD, "natural-word" },
	{ SNTOK_DIGITCODE, "digit-code" },
	{ SNTOK_EAN13, "ean13" },
	{ SNTOK_EAN8, "ean8" },
	{ SNTOK_UPCA, "upca" },
	{ SNTOK_UPCE, "upce" },
	{ SNTOK_RU_INN, "ru-inn" },
	{ SNTOK_EGAISWARECODE, "egais-ware-code" }, // @v12.3.3 @fix "egai-ware-code"-->"egais-ware-code"
	{ SNTOK_EGAISMARKCODE, "eags-mark-code" },
	{ SNTOK_LUHN, "luhn" },
	{ SNTOK_DIGLAT, "dig-lat" },
	{ SNTOK_GUID, "guid" },
	{ SNTOK_EMAIL, "email" },
	{ SNTOK_PHONE, "phone" },
	{ SNTOK_IMEI, "imei" },
	{ SNTOK_IP4, "ip4" },
	{ SNTOK_IP6, "ip6" },
	{ SNTOK_MACADDR48, "macaddr48" },
	{ SNTOK_DATE, "date" },
	{ SNTOK_TIME, "time" },
	{ SNTOK_SOFTWAREVER, "software-ver" },
	{ SNTOK_COLORHEX, "color-hex" },
	{ SNTOK_REALNUMBER, "real-number" },
	{ SNTOK_INTNUMBER, "int-number" },
	{ SNTOK_PERCENTAGE, "percentage" },
	{ SNTOK_NUMERIC_DOT, "numeric-dot" },
	{ SNTOK_NUMERIC_COM, "numeric-com" },
	{ SNTOK_CHZN_GS1_GTIN, "chzn-gs1-gtin" },
	{ SNTOK_CHZN_SIGN_SGTIN, "chzn-sign-sgtin" },
	// @v12.6.9 @unused { SNTOK_CHZN_SSCC, "chzn-sscc" },
	{ SNTOK_CHZN_CIGITEM, "chzn-cigitem" },
	{ SNTOK_CHZN_CIGBLOCK, "chzn-cigblock" },
	{ SNTOK_RU_OKPO, "ru-okpo" },
	{ SNTOK_RU_SNILS, "ru-snils" },
	{ SNTOK_RU_BIC, "ru-bic" },
	{ SNTOK_RU_KPP, "ru-kpp" },
	{ SNTOK_LINGUACODE, "lingua-code" },
	{ SNTOK_CHZN_SURROGATE_GTIN, "chzn-surrogate-gtin" },
	{ SNTOK_CHZN_SURROGATE_GTINCOUNT, "chzn-surrogate-gtin-count" },
	{ SNTOK_CL_RUT, "cl-rut" },
	{ SNTOK_CHZN_ALTCIGITEM, "chzn-altcigitem" },
	{ SNTOK_AR_DNI, "ar-dni" },
	{ SNTOK_GENERICTEXT_ASCII, "generictext-ascii" }, // @v12.2.12
	{ SNTOK_GENERICTEXT_UTF8, "generictext-utf8" }, // @v12.2.12
	{ SNTOK_GENERICTEXT_CP1251, "generictext-cp1251"}, // @v12.2.12
	{ SNTOK_GENERICTEXT_CP866, "generictext-cp866" }, // @v12.2.12
	{ SNTOK_JSON, "json" }, // @v12.2.12
	{ SNTOK_PLIDENT, "plident" }, // @v12.3.0
	{ SNTOK_HASH_MD5, "hash-md5" }, // @v12.3.3
	{ SNTOK_BASE32, "enc-base32" }, // @v12.3.3
	{ SNTOK_BASE32_CROCKFORD, "enc-base32-crockford" }, // @v12.3.3
	{ SNTOK_BASE58, "enc-base58" }, // @v12.3.3
	{ SNTOK_BASE64, "enc-base64" }, // @v12.3.3
	{ SNTOK_BASE64_URL, "enc-base32url" }, // @v12.3.3
	{ SNTOK_BASE64_WP, "enc-base32-withpadding" }, // @v12.3.3
	{ SNTOK_BASE64_URL_WP, "enc-base32url-withpadding" }, // @v12.3.3
	{ SNTOK_SSCC, "sscc" }, // @v12.4.5
	{ SNTOK_RU_LICPLATE, "ru-license-plate" }, // @v12.7.4
	{ SNTOK_WININTERNALCMD, "win-internal-cmd" }, // @v12.7.6
};

SNaturalToken::SNaturalToken() : ID(0), Prob(0.0f), Count(0)
{
}

SString & SNaturalToken::GetSymb(SString & rBuf) const
{
	SIntToSymbTab_GetSymb(SNTokSymb_List, SIZEOFARRAY(SNTokSymb_List), ID, rBuf);
	return rBuf;
}

SNaturalTokenArray & SNaturalTokenArray::Z()
{
	clear();
	return *this;
}

float FASTCALL SNaturalTokenArray::Has(uint32 tok) const
{
    uint   pos = 0;
    return lsearch(&tok, &pos, CMPF_LONG) ? at(pos).Prob : 0.0f;
}

int SNaturalTokenArray::AddTok(uint32 tok, float prob, uint flags)
{
	int    ok = 1;
    uint   pos = 0;
	if(prob > 0.0f) {
		if(lsearch(&tok, &pos, CMPF_LONG)) {
			SNaturalToken & r_item = at(pos);
			/*if(r_item.Prob != prob) {
				r_item.Prob = prob;
			}*/
			float new_prob = ((r_item.Prob * r_item.Count) + prob) / static_cast<float>(r_item.Count+1);
			r_item.Prob = new_prob;
			r_item.Count++;
		}
		else {
			SNaturalToken item;
			item.ID = tok;
			item.Prob = prob;
			item.Count = 1;
			ok = insert(&item);
		}
	}
	return ok;
}

int SNaturalTokenArray::Combine(const SNaturalTokenArray & rOther)
{
	int    ok = -1;
	const  uint _c = rOther.getCount();
	if(_c) {
		for(uint i = 0; ok && i < _c; i++) {
			const  SNaturalToken & r_other_item = rOther.at(i);
			if(AddTok(r_other_item.ID, r_other_item.Prob, 0/*flags*/))
				ok = 1;
			else
				ok = 0;
		}
	}
	return ok;
}

uint SNaturalTokenArray::IsThereAnyItemsThatAreNotInOther(const SNaturalTokenArray & rOther) const
{
	uint    result = 0;
	if(getCount()) {
		if(rOther.getCount()) {
			for(uint i = 0; i < getCount(); i++) {
				const SNaturalToken & r_item = at(i);
				if(rOther.Has(r_item.ID) > 0.0f) {
					;
				}
				else {
					result++;
				}
			}
		}
		else
			result = getCount();
	}
	return result;
}

int SNaturalTokenArray::Intersect(const SNaturalTokenArray & rS) // @v12.2.11
{
	int    result = -1;
	uint   c = getCount();
	if(c) do {
		SNaturalToken & r_tok = at(--c);
		float p = rS.Has(r_tok.ID);
		if(r_tok.Prob > 0.0f && p > 0.0f) {
			SETMIN(r_tok.Prob, p);
			result = 1;
		}
		else {
			atFree(c);
		}
	} while(c);
	return result;
}

/*static*/int STokenRecognizer::EncodeChZn1162(uint16 productTypeBytes, const char * pGTIN, const char * pSerial, void * pResultBuf, size_t resultBufSize)
{
	int   ret = 0;
	uint8  _buf[256];
	size_t _bp = 0;
	PTR8(_buf)[_bp++] = PTR8C(&productTypeBytes)[1];
	PTR8(_buf)[_bp++] = PTR8C(&productTypeBytes)[0];
	SString temp_buf(pGTIN);
	if(temp_buf.Len() == 14 && temp_buf.IsDec()) {
		int64 n = temp_buf.ToInt64();
		PTR8(_buf)[_bp++] = PTR8C(&n)[5];
		PTR8(_buf)[_bp++] = PTR8C(&n)[4];
		PTR8(_buf)[_bp++] = PTR8C(&n)[3];
		PTR8(_buf)[_bp++] = PTR8C(&n)[2];
		PTR8(_buf)[_bp++] = PTR8C(&n)[1];
		PTR8(_buf)[_bp++] = PTR8C(&n)[0];
		//
		{
			const size_t sl = sstrlen(pSerial);
			for(uint si = 0; si < sl; si++) {
				_buf[_bp++] = static_cast<uint8>(pSerial[si]);
			}
		}
		if(resultBufSize >= _bp && pResultBuf)
			memcpy(pResultBuf, _buf, _bp);
		ret = static_cast<int>(_bp);
	}
	return ret;
}

STokenRecognizer::STokenRecognizer() : SRegExpSet(), ReBase32(0), ReBase32_Crockford(0), ReBase58(0), ReBase64(0), ReBase64_Wp(0), ReBase64_Url(0), ReBase64_Url_Wp(0)
{
}

STokenRecognizer::~STokenRecognizer()
{
}

/*static*/int FASTCALL STokenRecognizer::IsUtf8(const uchar * p, size_t restLen)
{
	const int8 extra = SUtfConst::TrailingBytesForUTF8[*p];
	return (extra == 0) ? 1 : ((static_cast<int>(restLen) > extra && SUnicode::IsLegalUtf8Char(p, 2)) ? (extra+1) : 0);
}

/*
Возможные номера телефонов:
	99-99-99
	999-999
	999-9999
	russia: 8(999)999-99-99 (8 9999)99-99-99

Числа:
	9'999'999.99
	9'999'999,99
	99,999.99
	[0-9],. '-+
*/

static int FASTCALL _ProbeDate(const SString & rText)
{
	int    ok = 0;
	LDATE  probe_dt = strtodate_(rText, DATF_DMY);
	if(checkdate(probe_dt))
		ok = 1;
	else {
		probe_dt = strtodate_(rText, DATF_MDY);
		if(checkdate(probe_dt))
			ok = 1;
		else {
			probe_dt = strtodate_(rText, DATF_YMD);
			if(checkdate(probe_dt))
				ok = 1;
		}
	}
	return ok;
}

STokenRecognizer::ImplementBlock::ImplementBlock() : F(0), DecCount(0)
{
}

STokenRecognizer::ImplementBlock & STokenRecognizer::ImplementBlock::Z()
{
	F = 0;
	DecCount = 0;
	Temp.Z();
	Stat.Z();
	return *this;
}

void STokenRecognizer::ImplementBlock::Init(const uchar * pToken, int len)
{
	Z();
	Stat.Len = static_cast<uint32>((len >= 0) ? len : sstrlen(pToken));
}

static constexpr char * P_RuLicPlateUtf8Symbs = "АВЕКМНОРСТУХавекмнорстухABEKMHOPCTYXabekmhopctyx";

template <class T> bool MayBeRuLicPlate(const LAssocArray & rChrList, const T & rSet)
{
	bool   result = true;
	uint   dec_count = 0;
	uint   lett_count = 0;
	const  uint clc = rChrList.getCount();
	for(uint ci = 0; result && ci < clc; ci++) {
		const uint c = static_cast<uint>(rChrList.at(ci).Key);
		const uint ccnt = static_cast<uint>(rChrList.at(ci).Val);
		if(isdec(c)) {
			dec_count += ccnt;
			if(dec_count > 6)
				result = false;
		}
		else if(oneof2(c, ' ', '-')) {
			;
		}
		else if(rSet.HasChr(c)) {
			lett_count += ccnt;
			if(lett_count > 3)
				result = false;
		}
		else
			result = false;
	}
	if(result && (!oneof2(dec_count, 5, 6) || lett_count != 3)) {
		result = false;
	}
	return result;
}

template <class T> bool IsRuLicPlate(const T & rText, const T & rSet)
{
	bool   result = false;
	uint   ci = 0;
	auto   c = rText.C(ci);
	if(rSet.HasChr(c)) { // первый символ - буква
		c = rText.C(++ci);
		if(oneof2(c, ' ', '-')) { // возможен разделитель
			c = rText.C(++ci);
		}
		if(isdec(c)) { // три цифры подряд
			c = rText.C(++ci);
			if(isdec(c)) {
				c = rText.C(++ci);
				if(isdec(c)) {
					c = rText.C(++ci);
					if(oneof2(c, ' ', '-')) { // возможен разделитель
						c = rText.C(++ci);
					}
					if(rSet.HasChr(c)) { // две буквы подряд
						c = rText.C(++ci);
						if(rSet.HasChr(c)) {
							c = rText.C(++ci);
							if(oneof2(c, ' ', '-')) { // возможен разделитель
								c = rText.C(++ci);
							}
							if(isdec(c)) { // две или три цифры подряд
								c = rText.C(++ci);
								if(isdec(c)) {
									c = rText.C(++ci);
									if(c == 0 || isdec(c)) {
										result = true;	
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return result;
}

int STokenRecognizer::Implement(ImplementBlock & rIb, const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat)
{
	//Temp
	int    ok = 1;
	uint32 h = 0;
	rIb.Init(pToken, len);
	const  uint toklen = rIb.Stat.Len;
	LAssocArray & r_chr_list = rIb.Stat.ChrList;
    if(toklen) {
		uint   i;
		uchar  num_potential_frac_delim = 0;
		uchar  num_potential_tri_delim = 0;
		bool   is_there_illegal_utf8 = false;
		bool   is_there_multib_utf8 = false; // true если в тексте содержатся utf8 символы с длиной более одного байта
		const  char the_first_chr = pToken[0];
		h = 0xffffffffU & ~(SNTOKSEQ_LEADSHARP|SNTOKSEQ_LEADMINUS|SNTOKSEQ_LEADDOLLAR|SNTOKSEQ_BACKPCT);
		if(toklen >= 5) {
			rIb.F |= ImplementBlock::fPhoneSet;
			if(toklen >= 8) {
				rIb.F |= ImplementBlock::fClRut;
				if(toklen <= 16) { // @v12.7.4
					rIb.F |= ImplementBlock::fRuLicPlateSet; 
				}
			}
		}
		for(i = 0; i < toklen; i++) {
			const  uint   preserve_idx = i;
            const  uchar  c = pToken[i];
			const  uint16 utf8_extra = SUtfConst::TrailingBytesForUTF8[c];
			const  bool   is_legal_utf8 = ((utf8_extra < (toklen-i)) && SUnicode::IsLegalUtf8Char(pToken+i, utf8_extra+1)); // @v12.7.4 ((utf8_extra < (toklen-i)) &&)
			if(is_legal_utf8) {
                h |= SNTOKSEQ_UTF8;
                i += utf8_extra;
				if(utf8_extra > 0)
					is_there_multib_utf8 = true;
			}
			else
				is_there_illegal_utf8 = true;
			{
				uint  pos = 0;
				if(is_legal_utf8 && utf8_extra) {
					// @v12.7.4 {
					const  long uc = static_cast<long>(SUnicode::Helper_Utf8ToUtf32(PTRCHRC_(pToken)+preserve_idx, utf8_extra+1));
					if(r_chr_list.Search(uc, &pos))
						r_chr_list.at(pos).Val++;
					else
						r_chr_list.Add(uc, 1, 0);
					// } @v12.7.4 
				}
				else {
					if(r_chr_list.Search(static_cast<long>(c), &pos))
						r_chr_list.at(pos).Val++;
					else
						r_chr_list.Add(static_cast<long>(c), 1, 0);
				}
			}
		}
		if(is_there_illegal_utf8) {
			h &= ~SNTOKSEQ_UTF8;
			is_there_multib_utf8 = false;
		}
		r_chr_list.Sort();
		const  uint clc = r_chr_list.getCount();
		// @v12.3.0 if(/*rIb.F*/h & /*ImplementBlock::fUtf8*/SNTOKSEQ_UTF8) { // @v12.3.0 @fix ImplementBlock::fUtf8-->SNTOKSEQ_UTF8
		if(is_there_multib_utf8) { // @v12.3.0
			h &= ~(SNTOKSEQ_DEC|SNTOKSEQ_HEX|SNTOKSEQ_LATLWR|SNTOKSEQ_LATUPR|SNTOKSEQ_LAT|SNTOKSEQ_DECLAT|
				SNTOKSEQ_ASCII|SNTOKSEQ_866|SNTOKSEQ_1251|SNTOKSEQ_HEXHYPHEN|SNTOKSEQ_DECHYPHEN|SNTOKSEQ_HEXCOLON|
				SNTOKSEQ_DECCOLON|SNTOKSEQ_HEXDOT|SNTOKSEQ_DECDOT|SNTOKSEQ_DECSLASH|SNTOKSEQ_NUMERIC|SNTOKSEQ_PLIDENT);
		}
		else {
			bool   is_lead_plus = false;
			bool   has_dec = false;
			bool   has_cp866 = false;
			bool   has_cp1251 = false;
			i = 0;
			if(the_first_chr == '#') {
				h |= SNTOKSEQ_LEADSHARP;
				i++;
			}
			else if(the_first_chr == '-')
				h |= SNTOKSEQ_LEADMINUS;
			else if(the_first_chr == '$')
				h |= SNTOKSEQ_LEADDOLLAR;
			else if(the_first_chr == '+')
				is_lead_plus = true;
			// @v12.3.0 {
			if((h & SNTOKSEQ_PLIDENT) && !(the_first_chr == '_' || isasciialpha(the_first_chr)))
				h &= ~SNTOKSEQ_PLIDENT;
			// } @v12.3.0 
			for(; i < clc; i++) {
				const uchar c = static_cast<uchar>(r_chr_list.at(i).Key);
				const uint  ccnt = static_cast<uint>(r_chr_list.at(i).Val);
				if(h & SNTOKSEQ_ASCII && !(c >= 1 && c <= 127)) {
					h &= ~(SNTOKSEQ_ASCII|SNTOKSEQ_PLIDENT);
				}
				else {
					const bool is_hex_c = ishex(c);
					const bool is_dec_c = isdec(c);
					const bool is_asciialpha = isasciialpha(c);
					if(is_dec_c) {
						has_dec = true;
						rIb.DecCount += ccnt;
					}
					if(h & SNTOKSEQ_LAT && !is_asciialpha) {
						h &= ~SNTOKSEQ_LAT;
					}
					else {
						if(h & SNTOKSEQ_LATLWR && !(c >= 'a' && c <= 'z'))
							h &= ~SNTOKSEQ_LATLWR;
						if(h & SNTOKSEQ_LATUPR && !(c >= 'A' && c <= 'Z'))
							h &= ~SNTOKSEQ_LATUPR;
					}
					if(h & SNTOKSEQ_LATHYPHENORUSCORE && !is_asciialpha && !oneof2(c, '-', '_'))
						h &= ~SNTOKSEQ_LATHYPHENORUSCORE;
					if(h & SNTOKSEQ_HEX && !is_hex_c)
						h &= ~SNTOKSEQ_HEX;
					else if(h & SNTOKSEQ_DEC && !is_dec_c)
						h &= ~SNTOKSEQ_DEC;
					if(h & SNTOKSEQ_DECHYPHEN && !(c == '-' || is_dec_c))
						h &= ~SNTOKSEQ_DECHYPHEN;
					if(h & SNTOKSEQ_HEXHYPHEN && !(c == '-' || is_hex_c))
						h &= ~SNTOKSEQ_HEXHYPHEN;
					if(h & SNTOKSEQ_DECLAT && !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_dec_c))
						h &= ~SNTOKSEQ_DECLAT;
					if(h & SNTOKSEQ_DECCOLON && !(c == ':' || is_dec_c))
						h &= ~SNTOKSEQ_DECCOLON;
					if(h & SNTOKSEQ_HEXCOLON && !(c == ':' || is_hex_c))
						h &= ~SNTOKSEQ_HEXCOLON;
					if(h & SNTOKSEQ_DECDOT && !(c == '.' || is_dec_c))
						h &= ~SNTOKSEQ_DECDOT;
					if(h & SNTOKSEQ_HEXDOT && !(c == '.' || is_hex_c))
						h &= ~SNTOKSEQ_HEXDOT;
					if(h & SNTOKSEQ_DECSLASH && !(c == '/' || is_dec_c))
						h &= ~SNTOKSEQ_DECSLASH;
					if(h & SNTOKSEQ_NUMERIC) {
						if(!is_dec_c && !oneof7(c, ',', '.', '\'', '`', ' ', '+', '-'))
							h &= ~SNTOKSEQ_NUMERIC;
						else if(the_first_chr == ' ')
							h &= ~SNTOKSEQ_NUMERIC;
						else if(oneof2(c, '+', '-')) {
							if(r_chr_list.at(i).Val > 1)
								h &= ~SNTOKSEQ_NUMERIC;
							else if(c == '+' && !is_lead_plus)
								h &= ~SNTOKSEQ_NUMERIC;
							else if(c == '-' && !(h & SNTOKSEQ_LEADMINUS))
								h &= ~SNTOKSEQ_NUMERIC;
						}
					}
					if(rIb.F & ImplementBlock::fPhoneSet) {
						if(!is_dec_c && !oneof5(c, '+', '-', '(', ')', ' '))
							rIb.F &= ~ImplementBlock::fPhoneSet;
						else if(oneof3(c, '+', '(', ')') && ccnt > 1)
							rIb.F &= ~ImplementBlock::fPhoneSet;
					}
					if(rIb.F & ImplementBlock::fClRut) {
						if(!(h & SNTOKSEQ_ASCII))
							rIb.F &= ~ImplementBlock::fClRut;
						else if(!is_dec_c && !oneof4(c, 'k', 'K', '-', '.'))
							rIb.F &= ~ImplementBlock::fClRut;
						else if(oneof2(c, 'k', 'K') && ccnt > 1)  {
							rIb.F &= ~ImplementBlock::fClRut;
						}
						// @todo Надо еще проверить на отсутствие дубликатов 'k'-'K'; 'K'-'k'
					}
					// @v12.3.0 {
					if(h & SNTOKSEQ_PLIDENT && !(is_asciialpha || is_dec_c || c == '_')) { // Первый символ не может быть цифрой, но это мы уже проверили выше 
						h &= ~SNTOKSEQ_PLIDENT;
					}
					// } @v12.3.0 
				}
				{
					const bool is_letter_cp866 = IsLetter866(c);
					const bool is_letter_cp1251 = IsLetter1251(c);
					if(is_letter_cp866) {
						has_cp866 = true;
					}
					else if(h & SNTOKSEQ_866 && !(c >= 1 && c <= 127)) {
						h &= ~SNTOKSEQ_866;
					}
					if(is_letter_cp1251) {
						has_cp1251 = true;
					}
					else if(h & SNTOKSEQ_1251 && !(c >= 1 && c <= 127)) {
						h &= ~SNTOKSEQ_1251;
					}
				}
			}
			if(h & SNTOKSEQ_866 && !has_cp866)
				h &= ~SNTOKSEQ_866;
			if(h & SNTOKSEQ_1251 && !has_cp1251)
				h &= ~SNTOKSEQ_1251;
			if(!(h & SNTOKSEQ_ASCII)) {
				h &= ~(SNTOKSEQ_LAT|SNTOKSEQ_LATUPR|SNTOKSEQ_LATLWR|SNTOKSEQ_HEX|SNTOKSEQ_DEC|SNTOKSEQ_DECLAT|
					SNTOKSEQ_HEXHYPHEN|SNTOKSEQ_DECHYPHEN|SNTOKSEQ_HEXCOLON|SNTOKSEQ_DECCOLON|SNTOKSEQ_HEXDOT|
					SNTOKSEQ_DECDOT|SNTOKSEQ_DECSLASH|SNTOKSEQ_NUMERIC|SNTOKSEQ_LATHYPHENORUSCORE);
			}
			else {
				if(!(h & SNTOKSEQ_HEX))
					h &= ~SNTOKSEQ_DEC;
				if(!(h & SNTOKSEQ_LAT))
					h &= ~(SNTOKSEQ_LATLWR|SNTOKSEQ_LATUPR);
			}
			{
				const uint32 tf = SNTOKSEQ_HEXHYPHEN;
				if(h & tf) {
					if(h & SNTOKSEQ_HEX)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == '-');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == '-'); // '#' < '-'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_DECHYPHEN;
				if(h & tf) {
					if(h & SNTOKSEQ_DEC)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == '-');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == '-'); // '#' < '-'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_HEXCOLON;
				if(h & tf) {
					if(h & SNTOKSEQ_HEX)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == ':');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == ':'); // '#' < ':'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_DECCOLON;
				if(h & tf) {
					if(h & SNTOKSEQ_DEC)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == ':');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == ':'); // '#' < ':'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_HEXDOT;
				if(h & tf) {
					if(h & SNTOKSEQ_HEX)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == '.');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == '.'); // '#' < '.'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_DECDOT;
				if(h & tf) {
					if(h & SNTOKSEQ_DEC)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == '.');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == '.'); // '#' < '.'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_DECSLASH;
				if(h & tf) {
					if(h & SNTOKSEQ_DEC)
						h &= ~tf;
					else if(clc == 1) {
						assert(r_chr_list.at(0).Key == '/');
						h &= ~tf;
					}
					else if(clc == 2 && h & SNTOKSEQ_LEADSHARP) {
						assert(r_chr_list.at(1).Key == '/'); // '#' < '/'
						h &= ~tf;
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_LATHYPHENORUSCORE;
				if(h & tf) {
					if(h & SNTOKSEQ_LAT) {
						h &= ~tf;
					}
					else if(clc == 1) {
						assert(oneof2(r_chr_list.at(0).Key, '-', '_'));
						h &= ~tf;
					}
					else {
						if(toklen >= 2 && toklen <= 7) {
							//RecognizeLinguaSymb(reinterpret_cast<const char *>(pToken), 1);
						}
					}
				}
			}
			{
				const uint32 tf = SNTOKSEQ_NUMERIC;
				if(h & tf) {
					if(!has_dec)
						h &= ~tf;
					else {
						uint   comma_chr_pos = 0;
						const  uint comma_count = r_chr_list.Search(static_cast<long>(','), (long *)0, &comma_chr_pos) ? r_chr_list.at(comma_chr_pos).Val : 0;
						uint   last_dec_ser = 0;
						uint   j = toklen;
						if(j) do {
							const uchar lc = pToken[--j];
							if(isdec(lc))
								last_dec_ser++;
							else {
								if(lc == '.') {
									if(!num_potential_frac_delim)
										num_potential_frac_delim = lc;
									else {
										h &= ~tf;
										break; // Две точки в разбираемом формате невозможны
									}
								}
								else if(lc == ',' && comma_count == 1) {
									// 999,99
									// 999,999 - самый плохой случай. 
									if(last_dec_ser == 3) {
										num_potential_tri_delim = lc;
									}
									else if(num_potential_frac_delim) {
										h &= ~tf;
										break; // Роль десятичного разделителя занята, а на роль разделителя разрядов запятая здесь не годится - уходим.
									}
									if(!num_potential_frac_delim)
										num_potential_frac_delim = lc;
								}
								else if((comma_count > 1) ? oneof4(lc, ',', '\'', '`', ' ') : oneof3(lc, '\'', '`', ' ')) {
									if(last_dec_ser != 3) {
										h &= ~tf;
										break; // Потенциальный резделитель должен иметь точно 3 цифры справа. Это не так - уходим.
									}
									else {
										if(!num_potential_tri_delim) {
											num_potential_tri_delim = lc;
										}
										else if(num_potential_tri_delim != lc) {
											h &= ~tf;
											break; // Более одного символа могут претендовать на роль разделителя разрядов - уходим
										}
									}
								}
								else if(lc == '+') {
									if(j) {
										h &= ~tf;
										break; // + может быть только в первой позиции
									}
								}
								else if(lc == '-') {
									if(j) {
										h &= ~tf;
										break; // - может быть только в первой позиции
									}
								}
								last_dec_ser = 0;
							}
						} while(j);
					}
				}
			}
		}
		if(h & SNTOKSEQ_LEADSHARP) {
			if(h & SNTOKSEQ_HEX && toklen == 7) {
				rResultList.AddTok(SNTOK_COLORHEX, 0.9f, 0/*flags*/);
			}
		}
		/*else if(h & SNTOKSEQ_LEADMINUS) {
		}*/
		else if(h & SNTOKSEQ_LEADDOLLAR) {
		}
		else if(h & SNTOKSEQ_BACKPCT) {
		}
		else {
			if(h & SNTOKSEQ_PLIDENT) { // @v12.3.0
				rResultList.AddTok(SNTOK_PLIDENT, 0.3f, 0/*flags*/);
			}
			if(h & SNTOKSEQ_DECCOLON) { // @v12.2.12
				LTIME temp_tm = ZEROTIME;
				if(strtotime(reinterpret_cast<const char *>(pToken), TIMF_HMS, &temp_tm)) {
					const int _tm_h = temp_tm.hour();
					const int _tm_m = temp_tm.minut();
					const int _tm_s = temp_tm.sec();
					if(_tm_h >= 0 && _tm_h < 24 && _tm_m >= 0 && _tm_m < 60 && _tm_s >= 0 && _tm_s < 60) {
						rResultList.AddTok(SNTOK_TIME, 0.9f, 0/*flags*/);
					}
				}
			}
			if(h & SNTOKSEQ_LAT) { // @v12.7.6
				if(toklen >= 2 && toklen <= 10) {
					// @fixme Это способ идентификации токена очень медленный (облагает "налогом" все ascii-токены) - надо как-то оптимизировать
					static const char * p_win_internal_cmd_list =
						",ASSOC,ATTRIB,BREAK,CALL,CD,CHCP,CHDIR,CLS,CMD,COLOR,COPY,DATE,DEL,DIR,DPATH,ECHO,ENDLOCAL,ERASE,EXIT,FOR,FTYPE,"
						"GOTO,GRAFTABL,HELP,IF,LABEL,MD,MKDIR,MKLINK,MODE,MORE,MOVE,PATH,PAUSE,POPD,PROMPT,PUSHD,RD,REM,REN,RENAME,RMDIR,SET,SETLOCAL,SHIFT,SORT,START,SUBST,TIME,"
						"TITLE,TYPE,VER,VERIFY,VOL,XCOPY,";
					rIb.Temp.Z().CatChar(',').Cat(reinterpret_cast<const char *>(pToken)).ToUpperAscii().CatChar(',');
					const char * p = strstr(p_win_internal_cmd_list, rIb.Temp.cptr());
					if(p) {
						rResultList.AddTok(SNTOK_WININTERNALCMD, 0.4f, 0);
					}
				}
			}
			if(h & SNTOKSEQ_DEC) {
				rIb.F &= ~ImplementBlock::fRuLicPlateSet; // @v12.7.4
				// @v12.2.12 {
				{
					//SNTOKSEQ_LEADMINUS
					float intnum_prob = 1.0f;
					rResultList.AddTok(SNTOK_INTNUMBER, intnum_prob, 0/*flags*/);
				}
				// } @v12.2.12 
				if(!(h & SNTOKSEQ_LEADMINUS)) {
					const uchar last = pToken[toklen-1];
					int   cd = 0;
					{
						float digcod_prob = 1.0f;
						if(toklen >= 2) {
							if(pToken[0] == '0')
								digcod_prob = 0.9f;
							else
								digcod_prob = 0.8f;
						}
						else
							digcod_prob = 0.5f;
						rResultList.AddTok(SNTOK_DIGITCODE, digcod_prob, 0/*flags*/);
					}
					switch(toklen) {
						case 6:
							if(_ProbeDate(rIb.Temp.Z().CatN(reinterpret_cast<const char *>(pToken), toklen))) {
								rResultList.AddTok(SNTOK_DATE, 0.5f, 0/*flags*/);
							}
							break;
						case 8:
							// RU_OKPO
							{
								// 
								// Проверка правильности указания ОКПО:
								// 
								// Алгоритм проверки ОКПО:
								// 1. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (1,2,3,4,5,6,7).
								// 2. Вычисляется контрольное число(1) как остаток от деления контрольной суммы на 11.
								// 3. Вычисляется контрольная сумма по 7-и цифрам со следующими весовыми коэффициентами: (3,4,5,6,7,8,9).
								// 4. Вычисляется контрольное число(2) как остаток от деления контрольной суммы на 11.
								//   Если остаток от деления равен 10-ти, то контрольному числу(2) присваивается ноль.
								// 5. Если контрольное число(1) больше девяти, то восьмой знак ОКПО сравнивается с контрольным числом(2),
								//   иначе восьмой знак ОКПО сравнивается с контрольным числом(1). В случае их равенства ОКПО считается правильным.
								// 
								int is_ru_okpo = 0;
								static const int8 ru_okpo_w1[] = {1,2,3,4,5,6,7};
								static const int8 ru_okpo_w2[] = {3,4,5,6,7,8,9};
								ulong  sum1 = 0, sum2 = 0;
								for(i = 0; i < 7; i++) {
									sum1 += (ru_okpo_w1[i] * (pToken[i]-'0'));
									sum2 += (ru_okpo_w2[i] * (pToken[i]-'0'));
								}
								int    cd1 = (sum1 % 11);
								int    cd2 = (sum2 % 11);
								if(cd2 == 10)
									cd2 = 0;
								is_ru_okpo = (cd1 > 9) ? BIN((last-'0') == cd2) : BIN((last-'0') == cd1);							
								if(is_ru_okpo)
									rResultList.AddTok(SNTOK_RU_OKPO, 0.95f, 0/*flags*/);
							}
							cd = SCalcBarcodeCheckDigitL(reinterpret_cast<const char *>(pToken), toklen-1);
							if(static_cast<uchar>(cd) == (last-'0')) {
								if(pToken[0] == '0')
									rResultList.AddTok(SNTOK_UPCE, 0.9f, 0/*flags*/);
								else
									rResultList.AddTok(SNTOK_EAN8, 0.9f, 0/*flags*/);
							}
							if(_ProbeDate(rIb.Temp.Z().CatN(reinterpret_cast<const char *>(pToken), toklen))) {
								rResultList.AddTok(SNTOK_DATE, 0.8f, 0/*flags*/);
							}
							break;
						case 9:
							if(pToken[0] == '0' && pToken[1] == '4') {
								rResultList.AddTok(SNTOK_RU_BIC, 0.6f, 0/*flags*/);
							}
							rResultList.AddTok(SNTOK_RU_KPP, 0.1f, 0/*flags*/);
							break;
						case 10:
							if(SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_RU_INN, 1.0f, 0/*flags*/);
							}
							else {
								rResultList.AddTok(SNTOK_RU_INN, 0.01f, 0/*flags*/); // @v12.4.7
							}
							break;
						case 11:
							{
								// СНИЛС (страховой номер индивидуального лицевого счета) состоит из 11 цифр:
								//	1-9-я цифры — любые цифры;
								//	10-11-я цифры — контрольное число.
								// Маски ввода
								//	XXXXXXXXXXX — маска ввода без разделителей.
								//	XXX-XXX-XXX-XX — маска ввода с разделителями.
								//	XXX-XXX-XXX XX — маска ввода с разделителями и с отделением контрольного числа.
								// Алгоритм проверки контрольного числа
								//	Вычислить сумму произведений цифр СНИЛС (с 1-й по 9-ю) на следующие коэффициенты — 9, 8, 7, 6, 5, 4, 3, 2, 1 (т.е. номера цифр в обратном порядке).
								//	Вычислить контрольное число от полученной суммы следующим образом:
								//		если она меньше 100, то контрольное число равно этой сумме;
								//		если равна 100, то контрольное число равно 0;
								//		если больше 100, то вычислить остаток от деления на 101 и далее:
								//			если остаток от деления равен 100, то контольное число равно 0;
								//			в противном случае контрольное число равно вычисленному остатку от деления.
								//	Сравнить полученное контрольное число с двумя младшими разрядами СНИЛС. Если они равны, то СНИЛС верный.
								static const int8 ru_snils_w[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
								ulong  sum = 0;
								uint   cn = 0; // check number
								for(i = 0; i < 9; i++) {
									sum += (ru_snils_w[i] * (pToken[i]-'0'));
								}
								if(sum < 100)
									cn = sum;
								else if(sum == 100)
									cn = 0;
								else {
									cn = sum % 101;
									if(cn == 100)
										cn = 0;
								}
								if(cn == ((pToken[9]-'0') * 10 + (pToken[10]-'0'))) {
									rResultList.AddTok(SNTOK_RU_SNILS, 0.95f, 0/*flags*/);
								}
							}
							break;
						case 12:
							cd = SCalcBarcodeCheckDigitL(reinterpret_cast<const char *>(pToken), toklen-1);
							if(static_cast<uchar>(cd) == (last-'0')) {
								if(pToken[0] == '0')
									rResultList.AddTok(SNTOK_UPCE, 1.0f, 0/*flags*/);
								else
									rResultList.AddTok(SNTOK_EAN8, 1.0f, 0/*flags*/);
							}
							if(SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_RU_INN, 1.0f, 0/*flags*/);
							}
							else {
								rResultList.AddTok(SNTOK_RU_INN, 0.01f, 0/*flags*/); // @v12.4.7
							}
							break;
						case 13:
							cd = SCalcBarcodeCheckDigitL(reinterpret_cast<const char *>(pToken), toklen-1);
							if((uchar)cd == (last-'0')) {
								rResultList.AddTok(SNTOK_EAN13, 1.0f, 0/*flags*/);
							}
							break;
						case 15:
							if(SCalcCheckDigit(SCHKDIGALG_LUHN|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_IMEI, 0.9f, 0/*flags*/);
								rResultList.AddTok(SNTOK_DIGITCODE, 0.1f, 0/*flags*/);
							}
							break;
						case 18:
							// SSCC without two fixed leading zeros
							if(SCalcCheckDigit(SCHKDIGALG_SSCC|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_SSCC, 0.9f, 0/*flags*/);
							}
							break;
						case 19:
							if(SCalcCheckDigit(SCHKDIGALG_LUHN|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_LUHN, 0.9f, 0/*flags*/);
								rResultList.AddTok(SNTOK_EGAISWARECODE, 0.1f, 0/*flags*/);
							}
							else {
								rResultList.AddTok(SNTOK_EGAISWARECODE, 1.0f, 0/*flags*/);
							}
							break;
						case 20:
							// SSCC with two fixed leading zeros. Контрольная цифра остается верной даже с учетом префикса, поскольку там два нуля (чет/нечет)
							if(pToken[0] == '0' && pToken[1] == '0' && SCalcCheckDigit(SCHKDIGALG_SSCC|SCHKDIGALG_TEST, reinterpret_cast<const char *>(pToken), toklen)) {
								rResultList.AddTok(SNTOK_SSCC, 0.9f, 0/*flags*/);
							}
							break;
					}
				}
			}
			if(h & SNTOKSEQ_DECLAT) {
				rResultList.AddTok(SNTOK_DIGLAT, 1.0f, 0/*flags*/);
				if(oneof2(toklen, 68, 150)) {
					rResultList.AddTok(SNTOK_EGAISMARKCODE, 0.8f, 0/*flags*/);
				}
				else if(toklen == 9) {
					int   is_ru_kpp = 1;
					for(i = 0; is_ru_kpp && i < toklen; i++) {
						if(!isdec(pToken[i])) {
							if(!(oneof2(i, 4, 5) && checkirange(pToken[i], static_cast<uchar>('A'), static_cast<uchar>('Z')))) // 5, 6 знаки в КПП могут быть прописной латинской буквой
								is_ru_kpp = 0;
						}
					}
					if(is_ru_kpp)
						rResultList.AddTok(SNTOK_RU_KPP, 0.1f, 0/*flags*/); 
				}
			}
			if(h & SNTOKSEQ_HEXHYPHEN) {
				if(oneof2(toklen, 36, 39)) {
					//DC81EB17-2D15-4C1E-866F-FE89599761AC
					//2AE8-90BC-0DB9-4032-9649-270B-DA6D-089D
					uint   pos = 0;
					long   val = 0;
					if(r_chr_list.BSearch((long)'-', &val, &pos) && (val, 4, 7)) {
						float prob = (toklen == 36) ? 1.0f : 0.5f;
						rResultList.AddTok(SNTOK_GUID, prob, 0/*flags*/);
					}
				}
			}
			if(h & (SNTOKSEQ_DECHYPHEN|SNTOKSEQ_DECSLASH|SNTOKSEQ_DECDOT)) {
				rIb.F &= ~ImplementBlock::fRuLicPlateSet; // @v12.7.4
				// 1-1-1 17-12-2016
				if(toklen >= 5 && toklen <= 10) {
					rIb.Temp.Z().CatN(reinterpret_cast<const char *>(pToken), toklen);
					StringSet ss;
					const char * p_div = 0;
					if(h & SNTOKSEQ_DECHYPHEN)
						p_div = "-";
					else if(h & SNTOKSEQ_DECSLASH)
						p_div = "/";
					else if(h & SNTOKSEQ_DECDOT)
						p_div = ".";
					rIb.Temp.Tokenize(p_div, ss);
					if(ss.IsCountEq(3)) {
						if(_ProbeDate(rIb.Temp.Z().CatN(reinterpret_cast<const char *>(pToken), toklen))) {
							rResultList.AddTok(SNTOK_DATE, 0.8f, 0/*flags*/);
						}
					}
				}
			}
			if(h & SNTOKSEQ_DECDOT) {
				rIb.F &= ~ImplementBlock::fRuLicPlateSet; // @v12.7.4
				// 1.1.1.1 255.255.255.255
				rIb.Temp.Z().CatN(reinterpret_cast<const char *>(pToken), toklen);
				StringSet ss('.', rIb.Temp);
				const uint ss_count = ss.getCount();
				if(ss_count == 2) {
					rResultList.AddTok(SNTOK_REALNUMBER, 0.9f, 0/*flags*/);
				}
				if(toklen >= 3 && toklen <= 15) {
					if(ss_count == 4) {
						int   is_ip4 = 1;
						for(uint ssp = 0; is_ip4 && ss.get(&ssp, rIb.Temp);) {
							if(rIb.Temp.IsEmpty())
								is_ip4 = 0;
							else {
								const long v = rIb.Temp.ToLong();
								if(v < 0 || v > 255)
									is_ip4 = 0;
							}
						}
						if(is_ip4) {
							const float prob = (memcmp(pToken, "127.0.0.1", toklen) == 0) ? 1.0f : 0.95f;
							rResultList.AddTok(SNTOK_IP4, prob, 0/*flags*/);
						}
					}
					else if(oneof2(ss_count, 2, 3)) {
						int   is_ver = 1;
						for(uint ssp = 0; is_ver && ss.get(&ssp, rIb.Temp);) {
							if(rIb.Temp.IsEmpty())
								is_ver = 0;
							else {
								const long v = rIb.Temp.ToLong();
								if(v < 0 || v > 100)
									is_ver = 0;
							}
						}
						if(is_ver) {
							rResultList.AddTok(SNTOK_SOFTWAREVER, (ss_count == 3) ? 0.5f : 0.1f, 0/*flags*/);
						}
					}
				}
			}
			if(h & SNTOKSEQ_NUMERIC) {
				rIb.F &= ~ImplementBlock::fRuLicPlateSet; // @v12.7.4
				if(num_potential_frac_delim && num_potential_frac_delim == num_potential_tri_delim) {
					rResultList.AddTok(SNTOK_NUMERIC_COM, 0.6f, 0/*flags*/);
					rResultList.AddTok(SNTOK_NUMERIC_DOT, 0.6f, 0/*flags*/);
				}
				else if(num_potential_frac_delim == ',') {
					rResultList.AddTok(SNTOK_NUMERIC_COM, num_potential_tri_delim ? 0.7f : 0.95f, 0/*flags*/);
				}
				else {
					rResultList.AddTok(SNTOK_NUMERIC_DOT, num_potential_tri_delim ? 0.8f : 0.99f, 0/*flags*/);
				}
			}
			if(rIb.F & ImplementBlock::fPhoneSet) {
				if(rIb.DecCount >= 5 && rIb.DecCount <= 14) {
					// "^([+]?[\\s0-9]+)?(\\d{3}|[(]?[0-9]+[)])?([-]?[\\s]?[0-9])+"
					if(InitRePhone()) {
						SRegExp2::FindResult reresult;
						if(P_RePhone->Find(reinterpret_cast<const char *>(pToken), toklen, 0, &reresult)) {
							const uint f_pos = 0;
							size_t _offs = reresult.at(f_pos).low;
							size_t _len = reresult.at(f_pos).upp - reresult.at(f_pos).low;
							if(_offs == 0 && _len == toklen)
								rResultList.AddTok(SNTOK_PHONE, 0.8f, 0/*flags*/);
						}
					}
				}
			}
			if(rIb.F & ImplementBlock::fClRut) {
				if(rIb.DecCount >= 7 && rIb.DecCount <= 13) {
					const char control = toupper(pToken[toklen-1]);
					if(isdec(control) || control == 'K') {
						char cctrl = 0;
						static const uint8 cl_rut_w[] = {2,3,4,5,6,7,2,3,4,5,6,7};
						char   raw_code[32];
						size_t raw_code_len = 0;
						for(uint i = 0; i < toklen-1; i++) {
							if(isdec(pToken[i]))
								raw_code[raw_code_len++] = pToken[i];
						}
						assert(raw_code_len == (isdec(control) ? (rIb.DecCount-1) : rIb.DecCount));
						size_t ri = raw_code_len;
						uint   idx = 0;
						uint   sum = 0;
						if(ri) do {
							const uint d = raw_code[--ri]-'0';
							assert(idx < SIZEOFARRAY(cl_rut_w));
							sum += d * cl_rut_w[idx++];
						} while(ri);
						const uint cv = 11 - (sum % 11);
						if(cv == 11)
							cctrl = '0';
						else if(cv == 10)
							cctrl = 'K';
						else
							cctrl = '0' + cv;
						if(cctrl == control) {
							rResultList.AddTok(SNTOK_CL_RUT, 0.95f, 0/*flags*/);
						}
						/*
						{"9007920-4", "21620312-7", "13621690-2", "9329827-6", 
						"5946647-k", "7425273-7", "17694763-2", "23212441-5", "21485432-5", 
						"15459172-9", "10218932-9", "11316657-6", "24130358-6", 
						"11377848-2", "18609823-4", "18004377-2", "8784472-2", "12357399-4",
						"12391279-9", "8304218-4"}
						*/
					}
				}
			}
			if(h & SNTOKSEQ_ASCII) {
				{ // @v12.3.3
					if(sstreq(pToken, "===")) {
						rResultList.AddTok(SNTOK_BASE64_WP, 0.3f, 0/*flags*/);
						rResultList.AddTok(SNTOK_BASE64_URL_WP, 0.3f, 0/*flags*/);
					}
					else {
						static constexpr char p_chrset_base32[]           = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";
						static constexpr char p_chrset_base32_crockford[] = "ABCDEFGHJKMNPQRSTVWXYZ0123456789";
						static constexpr char p_chrset_base58[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz123456789";
						static constexpr char p_chrset_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
						static constexpr char p_chrset_base64_wp[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
						static constexpr char p_chrset_base64_url[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
						static constexpr char p_chrset_base64_url_wp[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";
						static_assert(sizeof(p_chrset_base32) == 32+1+1);
						static_assert(sizeof(p_chrset_base32_crockford) == 32+1);
						static_assert(sizeof(p_chrset_base58) == 58+1);
						static_assert(sizeof(p_chrset_base64) == 64+1);
						static_assert(sizeof(p_chrset_base64_wp) == 64+1+1);
						static_assert(sizeof(p_chrset_base64_url) == 64+1);
						static_assert(sizeof(p_chrset_base64_url_wp) == 64+1+1);

						struct TokRegExpTabEntry {
							uint32 Token;
							const  char * P_ChrSet;
							const  char * P_RegExp;
							long & R_RegExpHandler;
							float  Prob;
						};

						TokRegExpTabEntry tok_re_tab[] = {
							{ SNTOK_BASE32, p_chrset_base32, "^[A-Z2-7]+=*", ReBase32, 0.7f },
							{ SNTOK_BASE32_CROCKFORD, p_chrset_base32_crockford, "^[A-HJKMNP-TV-Z0-9]+", ReBase32_Crockford, 0.7f },
							{ SNTOK_BASE58, p_chrset_base58, "^[A-HJ-NP-Za-km-z1-9]*", ReBase58, 0.7f },
							{ SNTOK_BASE64_WP, p_chrset_base64_wp, "^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{4})", ReBase64_Wp, 0.95f },
							{ SNTOK_BASE64, p_chrset_base64, "^[A-Za-z0-9+/]+", ReBase64, 0.7f },
							{ SNTOK_BASE64_URL_WP, p_chrset_base64_url_wp, "^(?:[A-Za-z0-9_-]{4})*(?:[A-Za-z0-9_-]{2}==|[A-Za-z0-9_-]{3}=|[A-Za-z0-9_-]{4})", ReBase64_Url_Wp, 0.95f },
							{ SNTOK_BASE64_URL, p_chrset_base64_url, "^[A-Za-z0-9_-]+", ReBase64_Url, 0.7f },
						};

						for(uint rti = 0; rti < SIZEOFARRAY(tok_re_tab); rti++) {
							const TokRegExpTabEntry & r_entry = tok_re_tab[rti];
							if(rIb.Stat.IsChrListAsciiPatternMatched(r_entry.P_ChrSet)) {
								if(r_entry.R_RegExpHandler || RegisterRe(r_entry.P_RegExp, &r_entry.R_RegExpHandler)) {
									SRegExp2::FindResult reresult;
									const  SRegExp2 * p_re = ReList.at(r_entry.R_RegExpHandler-1);
									if(p_re && p_re->Find(reinterpret_cast<const char *>(pToken), toklen, 0, &reresult)) {
										assert(reresult.getCount());
										if(reresult.getCount()) {
											const uint f_pos = 0;
											size_t _offs = reresult.at(f_pos).low;
											size_t _len = reresult.at(f_pos).upp - reresult.at(f_pos).low;
											if(_offs == 0 && _len == toklen)
												rResultList.AddTok(r_entry.Token, r_entry.Prob, 0/*flags*/);
										}
									}
								}
							}
						}
					}
				}
				uint   pos = 0;
				long   val = 0;
				if(r_chr_list.BSearch(static_cast<long>('@'), &val, &pos) && val == 1 && InitReEmail()) {
					SRegExp2::FindResult reresult;
					if(P_ReEMail->Find(reinterpret_cast<const char *>(pToken), toklen, 0, &reresult)) {
						assert(reresult.getCount());
						if(reresult.getCount()) {
							const uint f_pos = 0;
							size_t _offs = reresult.at(f_pos).low;
							size_t _len = reresult.at(f_pos).upp - reresult.at(f_pos).low;
							if(_offs == 0 && _len == toklen)
								rResultList.AddTok(SNTOK_EMAIL, 1.0f, 0/*flags*/);
						}
					}
				}
				//
				// Проверка на маркировки сигаретных пачек (SNTOK_CHZN_CIGITEM)
				//
				if(oneof2(toklen, 25, 29)) { // Может встретится марка без криптохвоста (toklen==25)
					size_t _offs = 0;
					if(pToken[_offs++] == '0') {
						bool   is_chzn_cigitem = true;
						while(_offs < 14) {
							if(!isdec(pToken[_offs]))
								is_chzn_cigitem = false;
							_offs++;
						}
						// @v12.6.11 {
						// Если после 01GTIN идет "21" то это - не сигареты, а, скорее всего, суррогатный код чзн содержащий GTIN(01) и SERIAL(21)
						if(is_chzn_cigitem && (pToken[16] == '2' && pToken[17] == '1')) {
							is_chzn_cigitem = false;
						}
						// } @v12.6.11 
						if(is_chzn_cigitem) {
							if(memcmp(pToken+21, "AAAA", 4) == 0) { // @v11.9.0
								rResultList.AddTok(SNTOK_CHZN_ALTCIGITEM, (toklen == 29) ? 0.8f : 0.4f, 0/*flags*/);
								rResultList.AddTok(SNTOK_CHZN_CIGITEM, (toklen == 29) ? 0.4f : 0.2f, 0/*flags*/);
							}
							else {
								rResultList.AddTok(SNTOK_CHZN_CIGITEM, (toklen == 29) ? 0.8f : 0.4f, 0/*flags*/);
							}
							rResultList.AddTok(SNTOK_CHZN_GENERAL, 0.5f, 0); // @v12.6.9
						}
					}
				}
				if(oneof5(toklen, 25, 35, 41, 52, 55) || (toklen == 43 && r_chr_list.BSearch(static_cast<long>('\x1D'), &val, &pos) && val == 2)) {
					int    sig_prefix = 0; // 0 - no, 1 - '0', 2 - '(01)'
					size_t _offs = 0;
					if(pToken[_offs] == '0') {
						sig_prefix = 1;
						_offs++;
					}
					else if(pToken[_offs] == '(' && pToken[_offs+1] == '0' && pToken[_offs+2] == '1' && pToken[_offs+3] == ')') {
						sig_prefix = 2;
						_offs += 4;
					}
					if(sig_prefix) {
						int    is_chzn_cigblock = 1;
						if(sig_prefix == 1) {
							while(_offs < 16) {
								if(!isdec(pToken[_offs]))
									is_chzn_cigblock = 0;
								_offs++;
							}
						}
						else if(sig_prefix == 2) {
							while(_offs < 18) {
								if(!isdec(pToken[_offs]))
									is_chzn_cigblock = 0;
								_offs++;
							}
						}
						if(is_chzn_cigblock) {
							assert(_offs == 16 || _offs == 18);
							if(strstr(PTRCHRC_(pToken)+_offs, "8005")) { // код сигаретного блока может содержать тег цены с префиксом 80005
								rResultList.AddTok(SNTOK_CHZN_CIGBLOCK, 0.8f, 0/*flags*/);
								rResultList.AddTok(SNTOK_CHZN_GENERAL, 0.8f, 0); // @v12.6.9
							}
							else if(toklen == 25) {
								rResultList.AddTok(SNTOK_CHZN_CIGBLOCK, 0.5f, 0/*flags*/);
								rResultList.AddTok(SNTOK_CHZN_GENERAL, 0.5f, 0); // @v12.6.9
							}
						}
					}
				}
			}
		}
		if(rIb.F & ImplementBlock::fRuLicPlateSet) { // @v12.7.4
			// К142ХВ196
			//rus  АВЕКМНОРСТУХ
			//lat  ABEKMHOPCTYX
			if(h & (SNTOKSEQ_UTF8|SNTOKSEQ_1251|SNTOKSEQ_866|SNTOKSEQ_ASCII)) {
				bool   mu8 = false;
				bool   m1251 = false;
				bool   m866 = false;
				bool   mascii = false;
				assert((SLS.AcquireRvlStr() = P_RuLicPlateUtf8Symbs).IsLegalUtf8());
				if(h & SNTOKSEQ_UTF8) {
					rIb.TempU.Z().CopyFromUtf8Strict(P_RuLicPlateUtf8Symbs, strlen(P_RuLicPlateUtf8Symbs));
					if(MayBeRuLicPlate(r_chr_list, rIb.TempU)) {
						SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
						const  bool cfusr = r_temp_buf_u.CopyFromUtf8Strict(PTRCHRC_(pToken), toklen);
						assert(cfusr); // Мы выше убедились, что исходный токен в кодировке utf8. Если здесь сбой, то надо перепроверять код выше.
						if(IsRuLicPlate(r_temp_buf_u, rIb.TempU)) {
							mu8 = true;
							rResultList.AddTok(SNTOK_RU_LICPLATE, 0.8f, 0/*flags*/);
						}
					}
				}
				else {
					if(h & SNTOKSEQ_1251) {
						(rIb.Temp = P_RuLicPlateUtf8Symbs).Transf(CTRANSF_UTF8_TO_OUTER);
						if(MayBeRuLicPlate(r_chr_list, rIb.Temp)) {
							SString & r_temp_buf = SLS.AcquireRvlStr();
							r_temp_buf.CatN(PTRCHRC_(pToken), toklen);
							if(IsRuLicPlate(r_temp_buf, rIb.Temp)) {
								m1251 = true;
								rResultList.AddTok(SNTOK_RU_LICPLATE, 0.8f, 0/*flags*/);
							}
						}
					}
					if(!m1251) {
						if(h & SNTOKSEQ_866) {
							(rIb.Temp = P_RuLicPlateUtf8Symbs).Transf(CTRANSF_UTF8_TO_INNER);
							if(MayBeRuLicPlate(r_chr_list, rIb.Temp)) {
								SString & r_temp_buf = SLS.AcquireRvlStr();
								r_temp_buf.CatN(PTRCHRC_(pToken), toklen);
								if(IsRuLicPlate(r_temp_buf, rIb.Temp)) {
									m866 = true;
									rResultList.AddTok(SNTOK_RU_LICPLATE, 0.8f, 0/*flags*/);
								}
							}
						}
						if(!m866) {
							if(h & SNTOKSEQ_ASCII) {
								(rIb.Temp = P_RuLicPlateUtf8Symbs).Transf(CTRANSF_UTF8_TO_OUTER);
								if(MayBeRuLicPlate(r_chr_list, rIb.Temp)) {
									SString & r_temp_buf = SLS.AcquireRvlStr();
									r_temp_buf.CatN(PTRCHRC_(pToken), toklen);
									if(IsRuLicPlate(r_temp_buf, rIb.Temp)) {
										mascii = true;
										rResultList.AddTok(SNTOK_RU_LICPLATE, 0.7f, 0/*flags*/);
									}
								}
							}
						}
					}
				}
			}
		}
		// @v12.2.12 {
		{
			if(h & (SNTOKSEQ_ASCII|SNTOKSEQ_UTF8)) {
				if((pToken[0] == '[' && pToken[toklen-1] == ']') || (pToken[0] == '{' && pToken[toklen-1] == '}')) {
					// Возможно, json
					SString temp_buf;
					temp_buf.CatN(reinterpret_cast<const char *>(pToken), toklen);
					SJson * p_js_probe = SJson::Parse(temp_buf);
					if(p_js_probe) {
						rResultList.AddTok(SNTOK_JSON, 1.0f, 0/*flags*/);
						delete p_js_probe;
					}
				}
			}
		}
		{
			if(!rResultList.getCount()) { 
				// Если нам не удалось распознать ни одного годного токена, то попробуем трактовать переданную строку как некий текст
				if(h & SNTOKSEQ_1251) {
					rResultList.AddTok(SNTOK_GENERICTEXT_CP1251, 0.9f, 0/*flags*/);
				}
				if(h & SNTOKSEQ_866) {
					rResultList.AddTok(SNTOK_GENERICTEXT_CP866, 0.9f, 0/*flags*/);
				}
				if(h & SNTOKSEQ_ASCII) {
					rResultList.AddTok(SNTOK_GENERICTEXT_ASCII, 0.3f, 0/*flags*/);
				}
				else if(h & SNTOKSEQ_UTF8) {
					rResultList.AddTok(SNTOK_GENERICTEXT_UTF8, 0.9f, 0/*flags*/);
				}
			}
		}
		// } @v12.2.12 
    }
	rIb.Stat.Seq = h;
	ASSIGN_PTR(pStat, rIb.Stat);
	return ok;
}

/*virtual*/int STokenRecognizer::PostImplement(ImplementBlock & rIb, const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat)
{
	return 1;	
}

int STokenRecognizer::Run(const uchar * pToken, int len, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat)
{
	ImplementBlock ib;
	int ok = Implement(ib, pToken, len, rResultList, pStat);
	return ok ? PostImplement(ib, pToken, len, rResultList, pStat) : 0;
}

int STokenRecognizer::Run(const SString & rToken, SNaturalTokenArray & rResultList, SNaturalTokenStat * pStat)
{
	ImplementBlock ib;
	int ok = Implement(ib, rToken.ucptr(), rToken.LenI(), rResultList, pStat);
	return ok ? PostImplement(ib, rToken.ucptr(), rToken.LenI(), rResultList, pStat) : 0;
}

/*virtual*/int STokenRecognizer::NormalizeToken(const uchar * pToken, int len, const SNaturalTokenStat & rStat, uint32 tok, SString & rResult) // @construction
{
	rResult.Z();
	int    ok = -1;
	const  uint32 toklen = pToken ? static_cast<uint32>((len >= 0) ? len : sstrlen(pToken)) : 0;
	if(toklen) {
		if(tok == SNTOK_RU_LICPLATE) {
			//static constexpr char * P_RuLicPlateUtf8Symbs = "АВЕКМНОРСТУХавекмнорстухABEKMHOPCTYXabekmhopctyx";
			if(rStat.Seq & SNTOKSEQ_UTF8) {
				SStringU & r_symb_u = SLS.AcquireRvlStrU();
				if(r_symb_u.CopyFromUtf8R(P_RuLicPlateUtf8Symbs, sstrlen(P_RuLicPlateUtf8Symbs), 0)) {
					assert(r_symb_u.Len() == 48);
					SStringU & r_temp_u = SLS.AcquireRvlStrU();
					if(r_symb_u.Len() && r_temp_u.CopyFromUtf8R(PTRCHRC_(pToken), toklen, 0)) {
						SStringU & r_temp2_u = SLS.AcquireRvlStrU();
						bool   local_fault = false;
						for(uint i = 0; !local_fault && i < r_temp_u.Len(); i++) {
							const  wchar_t c = r_temp_u.C(i);
							size_t cpos = 0;
							if(isdec(c)) {
								r_temp2_u.CatChar(c);
							}
							else if(r_symb_u.SearchChar(c, &cpos)) {
								assert(cpos < r_symb_u.Len());
								if(cpos < 12) {
									r_temp2_u.CatChar(c);
								}
								else if(cpos < 24) {
									r_temp2_u.CatChar(r_symb_u.C(cpos-12));
								}
								else if(cpos < 36) {
									r_temp2_u.CatChar(r_symb_u.C(cpos-24));
								}
								else {
									assert(cpos >= 36);
									r_temp2_u.CatChar(r_symb_u.C(cpos-36));
								}
							}
							else if(oneof2(c, ' ', '-')) {
								; // Разделители пропускаем
							}
							else {
								local_fault = true; // неожиданный символ. Вероятно, нам подсунули строку, которая не может трактоваться как целевая - уходим!
							}
						}
						ok = local_fault ? 0 : r_temp2_u.CopyToUtf8(rResult, 1);
					}
					else
						ok = 0;
				}
				else
					ok = 0;
			}
			else {
				int    result_ascii = -1;
				int    result_1251 = -1;
				int    result_866 = -1;
				SString tok_ascii;
				SString tok_1251;
				SString tok_866;
				struct InternalTabEntry {
					uint   NtSeqFlag;
					int    CpTransfMode;
					int  & R_ResultVar;
					SString & R_Tok;
				};
				/*non-static*/ InternalTabEntry _tab[] = {
					{SNTOKSEQ_ASCII, CTRANSF_UTF8_TO_OUTER, result_ascii, tok_ascii},
					{SNTOKSEQ_1251, CTRANSF_UTF8_TO_OUTER, result_1251, tok_1251},
					{SNTOKSEQ_866, CTRANSF_UTF8_TO_INNER, result_866, tok_866},
				};
				uint   good_result_count = 0;
				{
					for(uint ti = 0; ti < SIZEOFARRAY(_tab); ti++) {
						InternalTabEntry & r_entry = _tab[ti];
						if(rStat.Seq & r_entry.NtSeqFlag) {
							SString & r_symb = SLS.AcquireRvlStr();
							(r_symb = P_RuLicPlateUtf8Symbs).Transf(r_entry.CpTransfMode);
							assert(r_symb.Len() == 48);
							SString & r_temp2 = SLS.AcquireRvlStr();
							bool  local_fault = false;
							for(uint i = 0; !local_fault && i < toklen; i++) {
								const  char c = pToken[i];
								size_t cpos = 0;
								if(isdec(c)) {
									r_temp2.CatChar(c);
								}
								else if(r_symb.SearchChar(c, &cpos)) {
									assert(cpos < r_symb.Len());
									if(cpos < 12) {
										r_temp2.CatChar(c);
									}
									else if(cpos < 24) {
										r_temp2.CatChar(r_symb.C(cpos-12));
									}
									else if(cpos < 36) {
										r_temp2.CatChar(r_symb.C(cpos-24));
									}
									else {
										assert(cpos >= 36);
										r_temp2.CatChar(r_symb.C(cpos-36));
									}
								}
								else if(oneof2(c, ' ', '-')) {
									; // Разделители пропускаем
								}
								else {
									local_fault = true; // неожиданный символ. Вероятно, нам подсунули строку, которая не может трактоваться как целевая - уходим!
								}
							}
							if(local_fault)
								r_entry.R_ResultVar = 0;
							else {
								r_entry.R_Tok = r_temp2;
								r_entry.R_ResultVar = 1;
								good_result_count++;
							}
						}
					}
				}
				if(good_result_count == 1) {
					for(uint ti = 0; ti < SIZEOFARRAY(_tab); ti++) {
						InternalTabEntry & r_entry = _tab[ti];
						if(r_entry.R_ResultVar > 0) {
							rResult = r_entry.R_Tok;
							ok = 1;
						}
					}
				}
				else if(good_result_count > 1) {
					if(result_866 > 0) { 
						assert(tok_866.Len());
						rResult = tok_866;
						ok = 1;
					}
					else if(result_1251 > 0) { 
						assert(tok_1251.Len());
						rResult = tok_1251;
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}
