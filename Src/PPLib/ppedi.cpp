// PPEDI.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
GtinStruc::GtinStruc() : StrAssocArray(), SpecialNaturalToken(0)
{
	memzero(SpecialStopChars, sizeof(SpecialStopChars)); // @v10.9.9
}

struct GtinFixedLengthToken {
	int16  Id;
	uint8  FxLen;
};

static const GtinFixedLengthToken GtinFixedLengthTokenList[] = {
	{ GtinStruc::fldSscc18,                18 }, // num
	{ GtinStruc::fldGTIN14,                14 }, // num
	{ GtinStruc::fldContainerCt,           14 }, // num
	//{ GtinStruc::fldPart,                  "10" }, // 1..20 alnum
	{ GtinStruc::fldManufDate,             6 }, // num
	{ GtinStruc::fldExpiryPeriod,          6 }, // num
	{ GtinStruc::fldPackDate,              6 }, // num
	{ GtinStruc::fldBestBeforeDate,        6 }, // num
	{ GtinStruc::fldExpiryDate,            6 }, // num
	{ GtinStruc::fldVariant,               2 }, // num
	//{ GtinStruc::fldSerial,                "21" }, // 1..20 num
	//{ GtinStruc::fldHIBCC,                 "22" }, // 1..29 alnum
	//{ GtinStruc::fldQtty,                  "30" }, // 1..8 num  
	//{ GtinStruc::fldCount,                 "37" }, // 1..8 num  
	//{ GtinStruc::fldMutualCode,            "90" }, // 1..30 alnum
	//{ GtinStruc::fldUSPS,                  "91" }, // 1..8 num   
	//{ GtinStruc::fldInner1,                "92" }, // 1..30 alnum
	//{ GtinStruc::fldInner2,                "93" }, // 1..30 alnum
	//{ GtinStruc::fldInner3,                "94" }, // 1..30 alnum
	//{ GtinStruc::fldInner4,                "95" }, // 1..30 alnum
	//{ GtinStruc::fldInner5,                "96" }, // 1..30 alnum
	//{ GtinStruc::fldInner6,                "97" }, // 1..30 alnum
	//{ GtinStruc::fldInner7,                "98" }, // 1..30 alnum
	//{ GtinStruc::fldInner8,                "99" }, // 1..30 alnum
	//{ GtinStruc::fldLot,                   "23x" }, // 1..19 alnum
	//{ GtinStruc::fldAddendumId,            "240" }, // 1..30 alnum
	//{ GtinStruc::fldSerial2,               "250" }, // 1..30 alnum
	//{ GtinStruc::fldCustomerOrderNo,       "400" }, // 1..29 alnum
	{ GtinStruc::fldShipTo,                13 }, // 13 num 
	{ GtinStruc::fldBillTo,                13 }, // 13 num 
	{ GtinStruc::fldPurchaseFrom,          13 }, // 13 num 
	//{ GtinStruc::fldShitToZip,             "420" }, // 1..9 alnum 
	//{ GtinStruc::fldShitToZipInt,          "421" }, // 4..12 alnum
	{ GtinStruc::fldCountry,               3 }, // 3 num            
	{ GtinStruc::fldWtNettKg,              6 }, // 6 num            
	{ GtinStruc::fldLenM,                  6 }, // 6 num            
	{ GtinStruc::fldWidthM,                6 }, // 6 num            
	{ GtinStruc::fldThknM,                 6 }, // 6 num    
	{ GtinStruc::fldAreaM2,                6 }, // 6 num    
	{ GtinStruc::fldVolumeL,               6 }, // 6 num   
	{ GtinStruc::fldVolumeM3,              6 }, // 6 num
	{ GtinStruc::fldWtNettLb,              6 }, // 6 num
	{ GtinStruc::fldLenInch,               6 }, // 6 num
	{ GtinStruc::fldLenFt,                 6 }, // 6 num
	{ GtinStruc::fldLenYr,                 6 }, // 6 num
	{ GtinStruc::fldDiamInch,              6 }, // 6 num
	{ GtinStruc::fldDiamFt,                6 }, // 6 num
	{ GtinStruc::fldDiamYr,                6 }, // 6 num
	{ GtinStruc::fldThknInch,              6 }, // 6 num
	{ GtinStruc::fldThknFt,                6 }, // 6 num
	{ GtinStruc::fldThknYr,                6 }, // 6 num
	{ GtinStruc::fldContainerWtBruttKg,    6 }, // 6 num
	{ GtinStruc::fldContainerLenM,         6 }, // 6 num
	{ GtinStruc::fldContainerDiamM,        6 }, // 6 num
	{ GtinStruc::fldContainerThknM,        6 }, // 6 num
	{ GtinStruc::fldContainerAreaM2,       6 }, // 6 num
	{ GtinStruc::fldContainerVolumeL,      6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeM3,    6 }, // 6 num
	{ GtinStruc::fldContainerMassLb,       6 }, // 6 num
	{ GtinStruc::fldContainerLenInch,      6 }, // 6 num
	{ GtinStruc::fldContainerLenFt,        6 }, // 6 num
	{ GtinStruc::fldContainerLenYr,        6 }, // 6 num
	{ GtinStruc::fldContainerDiamInch,     6 }, // 6 num
	{ GtinStruc::fldContainerDiamFt,       6 }, // 6 num
	{ GtinStruc::fldContainerDiamYr,       6 }, // 6 num
	{ GtinStruc::fldContainerThknInch,     6 }, // 6 num
	{ GtinStruc::fldContainerThknFt,       6 }, // 6 num
	{ GtinStruc::fldContainerThknYr,       6 }, // 6 num
	{ GtinStruc::fldProductAreaInch2,      6 }, // 6 num
	{ GtinStruc::fldProductAreaFt2,        6 }, // 6 num
	{ GtinStruc::fldProductAreaM2,         6 }, // 6 num
	{ GtinStruc::fldContainerAreaInch2,    6 }, // 6 num
	{ GtinStruc::fldContainerAreaFt2,      6 }, // 6 num
	{ GtinStruc::fldContainerAreaYr2,      6 }, // 6 num
	{ GtinStruc::fldWtNettTOz,             6 }, // 6 num
	{ GtinStruc::fldVolumeL_2,             6 }, // 6 num
	{ GtinStruc::fldVolumeL_3,             6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeQt,    6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeGal,   6 }, // 6 num
	{ GtinStruc::fldVolumeInch3,           6 }, // 6 num
	{ GtinStruc::fldVolumeFt3,             6 }, // 6 num
	{ GtinStruc::fldVolumeM3_2,            6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeInch3, 6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeFt3,   6 }, // 6 num
	{ GtinStruc::fldContainerGVolumeM3_2,  6 }, // 6 num
	{ GtinStruc::fldRollDimentions,        14 }, // 14 num
	//{ GtinStruc::fldESN,                   "8002" }, // 1..20 alnum
	//{ GtinStruc::fldGRAI,                  "8003" }, // 14 num + 1..16 alnum
	//{ GtinStruc::fldGIAI,                  "8004" }, // 1..30 alnum
	{ GtinStruc::fldPrice,                 6 }, // 6 num
	{ GtinStruc::fldCouponCode1,           6 }, // 6 num
	{ GtinStruc::fldCouponCode2,           10 }, // 10 num
	{ GtinStruc::fldCouponCode3,           2 }, // 2 num
};

int GtinStruc::GetPrefixSpec(int prefixId, uint * pFixedLen, uint * pMinLen) const
{
	int    ok = 0;
	ASSIGN_PTR(pFixedLen, 0);
	ASSIGN_PTR(pMinLen, 0);
	long   spc_fx_len = 0;
	long   spc_min_len = 0;
	if(SpecialFixedTokens.Search(prefixId, &spc_fx_len, 0)) {
		ASSIGN_PTR(pFixedLen, static_cast<uint>(spc_fx_len));
		ok = 1;
	}
	else {
		for(uint i = 0; !ok && i < SIZEOFARRAY(GtinFixedLengthTokenList); i++) {
			const GtinFixedLengthToken & r_item = GtinFixedLengthTokenList[i];
			if(r_item.Id == prefixId && r_item.FxLen > 0) {
				ASSIGN_PTR(pFixedLen, r_item.FxLen);
				ok = 1;
			}
		}
	}
	if(SpecialMinLenTokens.Search(prefixId, &spc_min_len, 0)) {
		ASSIGN_PTR(pMinLen, static_cast<uint>(spc_min_len));
		ok = 1;
	}
	return ok;
}

static const SIntToSymbTabEntry GtinPrefix[] = {
	{ GtinStruc::fldSscc18,                "00" },
	{ GtinStruc::fldGTIN14,                "01" },
	{ GtinStruc::fldContainerCt,           "02" },
	{ GtinStruc::fldPart,                  "10" },
	{ GtinStruc::fldManufDate,             "11" },
	{ GtinStruc::fldExpiryPeriod,          "12" },
	{ GtinStruc::fldPackDate,              "13" },
	{ GtinStruc::fldBestBeforeDate,        "15" },
	{ GtinStruc::fldExpiryDate,            "17" },
	{ GtinStruc::fldVariant,               "20" },
	{ GtinStruc::fldSerial,                "21" },
	{ GtinStruc::fldHIBCC,                 "22" },
	{ GtinStruc::fldQtty,                  "30" },
	{ GtinStruc::fldCount,                 "37" },
	{ GtinStruc::fldMutualCode,            "90" },
	{ GtinStruc::fldUSPS,                  "91" },
	{ GtinStruc::fldInner1,                "92" },
	{ GtinStruc::fldInner2,                "93" },
	{ GtinStruc::fldInner3,                "94" },
	{ GtinStruc::fldInner4,                "95" },
	{ GtinStruc::fldInner5,                "96" },
	{ GtinStruc::fldInner6,                "97" },
	{ GtinStruc::fldInner7,                "98" },
	{ GtinStruc::fldInner8,                "99" },
	{ GtinStruc::fldLot,                   "23x" },
	{ GtinStruc::fldAddendumId,            "240" },
	{ GtinStruc::fldSerial2,               "250" },
	{ GtinStruc::fldCustomerOrderNo,       "400" },
	{ GtinStruc::fldShipTo,                "410" },
	{ GtinStruc::fldBillTo,                "411" },
	{ GtinStruc::fldPurchaseFrom,          "412" },
	{ GtinStruc::fldShitToZip,             "420" },
	{ GtinStruc::fldShitToZipInt,          "421" },
	{ GtinStruc::fldCountry,               "422" },
	{ GtinStruc::fldWtNettKg,              "310y" },
	{ GtinStruc::fldLenM,                  "311y" },
	{ GtinStruc::fldWidthM,                "312y" },
	{ GtinStruc::fldThknM,                 "313y" },
	{ GtinStruc::fldAreaM2,                "314y" },
	{ GtinStruc::fldVolumeL,               "315y" },
	{ GtinStruc::fldVolumeM3,              "316y" },
	{ GtinStruc::fldWtNettLb,              "320y" },
	{ GtinStruc::fldLenInch,               "321y" },
	{ GtinStruc::fldLenFt,                 "322y" },
	{ GtinStruc::fldLenYr,                 "323y" },
	{ GtinStruc::fldDiamInch,              "324y" },
	{ GtinStruc::fldDiamFt,                "325y" },
	{ GtinStruc::fldDiamYr,                "326y" },
	{ GtinStruc::fldThknInch,              "327y" },
	{ GtinStruc::fldThknFt,                "328y" },
	{ GtinStruc::fldThknYr,                "329y" },
	{ GtinStruc::fldContainerWtBruttKg,    "330Y" },
	{ GtinStruc::fldContainerLenM,         "331y" },
	{ GtinStruc::fldContainerDiamM,        "332y" },
	{ GtinStruc::fldContainerThknM,        "333y" },
	{ GtinStruc::fldContainerAreaM2,       "334y" },
	{ GtinStruc::fldContainerVolumeL,      "335y" },
	{ GtinStruc::fldContainerGVolumeM3,    "336y" },
	{ GtinStruc::fldContainerMassLb,       "340y" },
	{ GtinStruc::fldContainerLenInch,      "341y" },
	{ GtinStruc::fldContainerLenFt,        "342y" },
	{ GtinStruc::fldContainerLenYr,        "343y" },
	{ GtinStruc::fldContainerDiamInch,     "344y" },
	{ GtinStruc::fldContainerDiamFt,       "345y" },
	{ GtinStruc::fldContainerDiamYr,       "346y" },
	{ GtinStruc::fldContainerThknInch,     "347y" },
	{ GtinStruc::fldContainerThknFt,       "348y" },
	{ GtinStruc::fldContainerThknYr,       "349y" },
	{ GtinStruc::fldProductAreaInch2,      "350y" },
	{ GtinStruc::fldProductAreaFt2,        "351y" },
	{ GtinStruc::fldProductAreaM2,         "352y" },
	{ GtinStruc::fldContainerAreaInch2,    "353y" },
	{ GtinStruc::fldContainerAreaFt2,      "354y" },
	{ GtinStruc::fldContainerAreaYr2,      "355y" },
	{ GtinStruc::fldWtNettTOz,             "356y" },
	{ GtinStruc::fldVolumeL_2,             "360y" },
	{ GtinStruc::fldVolumeL_3,             "361y" },
	{ GtinStruc::fldContainerGVolumeQt,    "362y" },
	{ GtinStruc::fldContainerGVolumeGal,   "363y" },
	{ GtinStruc::fldVolumeInch3,           "364y" },
	{ GtinStruc::fldVolumeFt3,             "365y" },
	{ GtinStruc::fldVolumeM3_2,            "366y" },
	{ GtinStruc::fldContainerGVolumeInch3, "367y" },
	{ GtinStruc::fldContainerGVolumeFt3,   "368y" },
	{ GtinStruc::fldContainerGVolumeM3_2,  "369y" },
	{ GtinStruc::fldRollDimentions,        "8001" },
	{ GtinStruc::fldESN,                   "8002" },
	{ GtinStruc::fldGRAI,                  "8003" },
	{ GtinStruc::fldGIAI,                  "8004" },
	{ GtinStruc::fldPrice,                 "8005" },
	{ GtinStruc::fldCouponCode1,           "8100" },
	{ GtinStruc::fldCouponCode2,           "8101" },
	{ GtinStruc::fldCouponCode3,           "8102" },
};

int GtinStruc::DetectPrefix(const char * pSrc, uint flags, int currentId, uint * pPrefixLen, SString & rPrefix) const
{
	int    prefix_id = -1;
	const  size_t src_len = sstrlen(pSrc);
	uint   prefix_len = 0;
	SString temp_buf;
	if(src_len >= 4) {
		temp_buf.Z().CatN(pSrc, 4);
		int _id = SIntToSymbTab_GetId(GtinPrefix, SIZEOFARRAY(GtinPrefix), temp_buf);
		if(_id > 0 && (!OnlyTokenList.getCount() || OnlyTokenList.lsearch(_id)) && _id != currentId && !StrAssocArray::Search(_id)) {
			prefix_len = 4;
			prefix_id = _id;
		}
	}
	if(prefix_id <= 0 && src_len >= 3) {
		temp_buf.Z().CatN(pSrc, 3);
		int _id = SIntToSymbTab_GetId(GtinPrefix, SIZEOFARRAY(GtinPrefix), temp_buf);
		if(_id > 0 && (!OnlyTokenList.getCount() || OnlyTokenList.lsearch(_id)) && _id != currentId && !StrAssocArray::Search(_id)) {
			prefix_len = 3;
			prefix_id = _id;
		}
	}
	if(prefix_id <= 0 && src_len >= 2) {
		temp_buf.Z().CatN(pSrc, 2);
		int _id = SIntToSymbTab_GetId(GtinPrefix, SIZEOFARRAY(GtinPrefix), temp_buf);
		if(_id > 0 && (!OnlyTokenList.getCount() || OnlyTokenList.lsearch(_id)) && _id != currentId && !StrAssocArray::Search(_id)) {
			if((flags & dpfBOL) || !oneof2(_id, fldSscc18, fldGTIN14)) {
				prefix_len = 2;
				prefix_id = _id;
			}
		}
	}
	// @v10.7.7 {
	if(oneof5(prefix_id, GtinStruc::fldManufDate, GtinStruc::fldExpiryPeriod, GtinStruc::fldPackDate, GtinStruc::fldBestBeforeDate, GtinStruc::fldExpiryDate)) {
		// 6 digits
		int   false_prefix = 0;
		if(sstrlen(pSrc+prefix_len) >= 6) {
			for(uint ci = 0; !false_prefix && ci < 6; ci++) {
				if(!isdec(pSrc[prefix_len+ci]))
					false_prefix = 1;
			}
		}
		else
			false_prefix = 1;
		if(false_prefix) {
			prefix_id = -1;
			prefix_len = 0;
		}
	}
	// } @v10.7.7 
	ASSIGN_PTR(pPrefixLen, prefix_len);
	return prefix_id;
}

void GtinStruc::AddOnlyToken(int token)
{
	if(SIntToSymbTab_HasId(GtinPrefix, SIZEOFARRAY(GtinPrefix), token))
		OnlyTokenList.addUnique(token);
}

void GtinStruc::SetSpecialFixedToken(int token, int fixedLen)
{
	if(fixedLen == 1000 || checkirange(fixedLen, 1, 50) && SIntToSymbTab_HasId(GtinPrefix, SIZEOFARRAY(GtinPrefix), token)) { // @v10.9.0 30-->50
		SpecialFixedTokens.Remove(token, 0); // @v10.7.12
		SpecialFixedTokens.AddUnique(token, fixedLen, 0, 0);
	}
}

void GtinStruc::SetSpecialMinLenToken(int token, int minLen)
{
	if(minLen > 0)
		SpecialMinLenTokens.AddUnique(token, minLen, 0, 0);
}

void GtinStruc::AddSpecialStopChar(uchar stopChar)
{
	if(stopChar) {
		const uint slot_size = SIZEOFARRAY(SpecialStopChars);
		for(uint i = 0; i < slot_size; i++) {
			if(SpecialStopChars[i] == stopChar)
				break;
			else if(SpecialStopChars[i] == 0) {
				SpecialStopChars[i] = stopChar;
				break;
			}
		}
	}
}

uint GtinStruc::SetupFixedLenField(const char * pSrc, const uint prefixLen, const uint fixLen, int fldId)
{
	uint   result_offs = 0;
	SString temp_buf;
	if(fixLen == 1000) {
		temp_buf.Cat(pSrc+prefixLen);
	}
	else {
		temp_buf.CatN(pSrc+prefixLen, fixLen);
		THROW(temp_buf.Len() == fixLen);
	}
	const uint result_fix_len = temp_buf.Len();
	if(fldId > 0) {
		THROW(!StrAssocArray::Search(fldId));
		StrAssocArray::Add(fldId, temp_buf);
	}
	result_offs = (prefixLen + result_fix_len);
	CATCH
		result_offs = 0;
	ENDCATCH
	return result_offs;
}

GtinStruc & GtinStruc::Z()
{
	StrAssocArray::Z();
	SpecialNaturalToken = 0;
	return *this;
}

int GtinStruc::Debug_Output(SString & rBuf) const
{
	int    ok = 1;
	rBuf.Z();
	if(StrAssocArray::getCount()) {
		SString temp_buf;
		for(uint i = 0; i < StrAssocArray::getCount(); i++) {
			StrAssocArray::Item item = StrAssocArray::Get(i);
			SIntToSymbTab_GetSymb(GtinPrefix, SIZEOFARRAY(GtinPrefix), item.Id, temp_buf);
			if(rBuf.NotEmpty())
				rBuf.CR();
			rBuf.Cat(item.Id).Space().CatChar('[').Cat(temp_buf).CatChar(']').Space().Cat(item.Txt).Space().CatEq("len", sstrlen(item.Txt));
		}
	}
	else
		rBuf.Cat("empty");
	return ok;
}

/*
Пачки сигарет 29 символов
_____________
GTIN – 14 знаков
Серийный номер – 7 знаков
МРЦ – 4 знака
Криптохвост – 4 знака

00000046209443j+Q'?P5ACZAC8bG
00000046209443|j+Q'?P5|ACZA|C8bG
00000046209443x-8xfgOACZAYGfv
04606203098187o&zWeIyABr8l/nT
*/

int GtinStruc::GetToken(int tokenId, SString * pToken) const
{
	int    ok = 0;
	uint   pos = 0;
	if(StrAssocArray::Search(tokenId, &pos)) {
		if(pToken) {
			StrAssocArray::Item item = StrAssocArray::Get(pos);
			*pToken = item.Txt;
		}
		ok = 1;
	}
	return ok;
}

int GtinStruc::GetSpecialNaturalToken() const
{
	return SpecialNaturalToken;
}

uint GtinStruc::RecognizeFieldLen(const char * pSrc, int currentPrefixID) const
{
	uint   len = 0;
	SString next_prefix_;
	while(*pSrc) {
		uint  next_prefix_len = 0;
		int   next_prefix_id = DetectPrefix(pSrc, 0, currentPrefixID, &next_prefix_len, next_prefix_);
		if(next_prefix_id > 0)
			break;
		else {
			pSrc++;
			len++;
		}
	}
	return len;
}

int GtinStruc::IsSpecialStopChar(const char * pSrc) const
{
	if(SpecialStopChars[0] && pSrc) {
		const uchar c = static_cast<uchar>(*pSrc);
		if(c) {
			for(uint i = 0; i < SIZEOFARRAY(SpecialStopChars); i++) {
				if(SpecialStopChars[i] == c)
					return 1;
				else if(SpecialStopChars[i] == 0)
					break;
			}
		}
	}
	return 0;
}

// dosn't work :(
static int FASTCALL Base80ToTobaccoPrice(const SString & rS, SString & rBuf)
{
	/*
		«Алгоритм кодирования-декодирования МРЦ основан на переводе МРЦ в копейках в 80-чную систему счисления,

		используя следующий алфавит: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!"%&'*+-./_,:;=<>?

		Пример кода для 1С:

		Функция МРЦКодаМаркировкиТабачнойПачки(КодМаркировки) Экспорт

			ДлинаКода = СтрДлина(КодМаркировки);
			Если ДлинаКода <> 29 Тогда
				Возврат Неопределено;
			КонецЕсли;
			СтрокаМРЦ = Сред(КодМаркировки, 22, 4);
			Алфавит = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456­789!\"%&'*+-./_,:;=<>?";
			МРЦ    = 0;
			Индекс = 1;
			Пока Индекс <= 4 Цикл
				Символ = Сред(СтрокаМРЦ, Индекс, 1);
				ИндексСимвола = СтрНайти(Алфавит, Символ) - 1;
				Если ИндексСимвола < 0 Тогда
					Возврат Неопределено;
				КонецЕсли;
				МРЦ = МРЦ + Pow(80, 4 - Индекс) * ИндексСимвола;
				Индекс = Индекс + 1;
			КонецЦикла;
			// Если цена <= 5000 и нет копеек, то высокая вероятность, что это реальное МРЦ.
			Если МРЦ <= 500000
				И МРЦ%100 = 0 Тогда
				Возврат МРЦ / 100;
			Иначе
				Возврат Неопределено;
			КонецЕсли;
		КонецФункции
	*/
	const char * p_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456­789!""%&'*+-./_,:;=<>?";
	int    ok = 1;
	rBuf.Z();
	uint64 result = 0;
	{
		const uint len = rS.Len();
		for(uint i = 0; ok && i < len; i++) {
			const  char c = rS.C(i);
			const  char * p_idxpos = strchr(p_alphabet, c);
			if(p_idxpos) {
				const uint64 m = ui64pow(80, len-i-1);
				result += m * (p_idxpos - p_alphabet);
			}
			else
				ok = 0;
		}
	}
	/*{
		SString temp_buf = rS;
		temp_buf.ShiftLeft();
		const uint len = temp_buf.Len();
		for(uint i = 0; ok && i < len; i++) {
			const  char c = temp_buf.C(i);
			const  char * p_idxpos = strchr(p_alphabet, c);
			if(p_idxpos) {
				const uint64 m = ui64pow(80, len-i-1);
				result2 += m * (p_idxpos - p_alphabet);
			}
			else
				ok = 0;
		}
	}*/
	if(ok)
		rBuf.Cat(result);
	return ok;
}

static int FASTCALL Base36ToTobaccoPrice(const SString & rS, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	uint64 result = 0;
	const uint len = rS.Len();
	for(uint i = 0; ok && i < len; i++) {
		const  char c = toupper(rS.C(i));
		uint64 v = 0;
		if(c >= '0' && c <= '9')
			v = c - '0';
		else if(c >= 'A' && c <= 'Z')
			v = c - 'A' + 10;
		else
			ok = 0;
		const uint64 m = ui64pow(36, len-i-1);
		result += v * m;
	}
	if(ok) {
		rBuf.Cat(result);
		if(rBuf.Len() < 19)
			rBuf.PadLeft(19-rBuf.Len(), '0');
	}
	return ok;
}

int GtinStruc::Parse(const char * pCode)
{
	int    ok = 1;
	SString code_buf(pCode);
	Z();
	if(code_buf.NotEmptyS()) {
		SString temp_buf;
		SString prefix_;
		SString next_prefix_;
		StrAssocArray::Add(fldOriginalText, code_buf);
		STokenRecognizer tr;
		SNaturalTokenArray nta;
		uint tokn = 0;
		tr.Run(code_buf.ucptr(), code_buf.Len(), nta, 0);
		if(nta.Has(SNTOK_CHZN_CIGITEM)) {
			assert(code_buf.Len() == 29);
			size_t offs = 0; 
			temp_buf.Z().CatN(code_buf+offs, 14); offs += 14;
			StrAssocArray::Add(fldGTIN14, temp_buf);
			temp_buf.Z().CatN(code_buf+offs, 7); offs += 7;
			StrAssocArray::Add(fldSerial, temp_buf);
			temp_buf.Z().CatN(code_buf+offs, 4); offs += 4;
			StrAssocArray::Add(fldPriceRuTobacco, temp_buf);
			temp_buf.Z().CatN(code_buf+offs, 4); offs += 4;
			{
				//SString price_buf;
				//Base80ToTobaccoPrice(temp_buf, price_buf);
				//Base36ToTobaccoPrice(temp_buf, price_buf);
				StrAssocArray::Add(fldControlRuTobacco, temp_buf);
			}
			SpecialNaturalToken = SNTOK_CHZN_CIGITEM;
		}
		else if(nta.Has(SNTOK_CHZN_CIGBLOCK)) {
			// 0104600818007879 21t"XzgHU 8005095000 930p2J24014518552
			//assert(oneof2(code_buf.Len(), 52, 35));
			if(code_buf.HasPrefix("01")) {
				code_buf.ShiftLeft(2);
				temp_buf.Z().CatN(code_buf, 14);
				StrAssocArray::Add(fldGTIN14, temp_buf);
				code_buf.ShiftLeft(14);
				if(code_buf.HasPrefix("21")) {
					code_buf.ShiftLeft(2);
					temp_buf.Z().CatN(code_buf, 7);
					StrAssocArray::Add(fldSerial, temp_buf);
					code_buf.ShiftLeft(7);
					if(code_buf.HasPrefix("8005")) {
						code_buf.ShiftLeft(4);
						temp_buf.Z().CatN(code_buf, 6);
						StrAssocArray::Add(fldPrice, temp_buf);
						code_buf.ShiftLeft(6);
						if(code_buf.HasPrefix("93")) {
							code_buf.ShiftLeft(2);
							temp_buf.Z().Cat(code_buf);
							StrAssocArray::Add(fldControlRuTobacco, temp_buf);
							SpecialNaturalToken = SNTOK_CHZN_CIGBLOCK;
						}
					}
				}
			}
		}
		else {
			const char * p = code_buf.cptr();
			uint    dpf = dpfBOL;
			while(*p) {
				uint  prefix_len = 0;
				if(IsSpecialStopChar(p)) { // @v10.9.9
					p++;
				}
				int   prefix_id = DetectPrefix(p, dpf, -1, &prefix_len, prefix_);
				dpf = 0;
				uint  fixed_len = 0;
				uint  min_len = 0;
				if(prefix_id > 0) {
					GetPrefixSpec(prefix_id, &fixed_len, &min_len);
					if(fixed_len) {
						const uint ro = SetupFixedLenField(p, prefix_len, fixed_len, prefix_id);
						THROW(ro);
						p += ro;
					}
					else {
						temp_buf.Z();
						p += prefix_len;
						int   next_prefix_id = -1;
						uint  next_prefix_len = 0;
						while(*p) {
							if(IsSpecialStopChar(p)) { // @v10.9.9
								p++; // Специальный стоп-символ пропускаем и завершаем акцепт токена
								break;
							}
							else if(min_len > 0 && temp_buf.Len() < min_len) {
								temp_buf.CatChar(*p++);
							}
							else {
								next_prefix_id = DetectPrefix(p, dpf, prefix_id, &next_prefix_len, next_prefix_);
								if(next_prefix_id > 0) {
									uint next_fld_len = RecognizeFieldLen(p+next_prefix_len, prefix_id);
									if(next_fld_len == 0) {
										temp_buf.CatChar(*p++);
									}
									else
										break;
								}
								else
									temp_buf.CatChar(*p++);
							}
						}
						THROW(!StrAssocArray::Search(prefix_id));
						StrAssocArray::Add(prefix_id, temp_buf);
					}
				}
				else {
					ok = 2; // unexpected 
					break;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int TestGtinStruc()
{
	int    ok = 1;
	SString temp_buf;
	SString out_buf;
	PPLogger logger;
	PPGetPath(PPPATH_TESTROOT, temp_buf);
	temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("chzn-marks.txt");
	SFile f_in(temp_buf, SFile::mRead);
	if(f_in.IsValid()) {
		PPGetPath(PPPATH_TESTROOT, temp_buf);
		temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("chzn-marks-result.txt");
		SFile f_out(temp_buf, SFile::mWrite);
		if(f_out.IsValid()) {
			while(f_in.ReadLine(temp_buf)) {
				temp_buf.Chomp().Strip();
				GtinStruc gts;
				gts.AddOnlyToken(GtinStruc::fldGTIN14);
				gts.AddOnlyToken(GtinStruc::fldSerial);
				gts.SetSpecialFixedToken(GtinStruc::fldSerial, 13);
				gts.AddOnlyToken(GtinStruc::fldPart);
				// gts.SetSpecialMinLenToken(GtinStruc::fldPart, 5); // @v10.9.6
				gts.AddOnlyToken(GtinStruc::fldAddendumId);
				gts.AddOnlyToken(GtinStruc::fldUSPS);
				gts.SetSpecialFixedToken(GtinStruc::fldUSPS, 4); // @v10.9.10
				gts.AddOnlyToken(GtinStruc::fldInner1);
				gts.SetSpecialFixedToken(GtinStruc::fldInner1, 1000/*UNTIL EOL*/); // @v10.9.0
				gts.AddOnlyToken(GtinStruc::fldInner2);
				gts.AddOnlyToken(GtinStruc::fldSscc18);
				gts.AddOnlyToken(GtinStruc::fldExpiryDate);
				gts.AddOnlyToken(GtinStruc::fldManufDate);
				gts.AddOnlyToken(GtinStruc::fldVariant);
				gts.AddOnlyToken(GtinStruc::fldMutualCode);
				//gts.AddOnlyToken(GtinStruc::fldPriceRuTobacco);
				//gts.AddOnlyToken(GtinStruc::fldPrice);
				gts.AddSpecialStopChar(0x1D); // @v10.9.9
				gts.AddSpecialStopChar(0xE8); // @v10.9.9
				int pr = gts.Parse(temp_buf);
				if(pr != 1 && gts.GetToken(GtinStruc::fldGTIN14, 0)) {
					gts.SetSpecialFixedToken(GtinStruc::fldSerial, 12);
					pr = gts.Parse(temp_buf);
					if(pr != 1 && gts.GetToken(GtinStruc::fldGTIN14, 0)) {
						gts.SetSpecialFixedToken(GtinStruc::fldSerial, 11);
						pr = gts.Parse(temp_buf);
						// @v10.8.2 {
						/*if(pr != 1 && gts.GetToken(GtinStruc::fldGTIN14, 0)) {
							gts.SetSpecialFixedToken(GtinStruc::fldSerial, 8);
							pr = gts.Parse(temp_buf);
						}*/
						// } @v10.8.2 
					}
				}
				out_buf.Z().CR().Cat(temp_buf).Space().CatEq("parse-result", static_cast<long>(pr));
				gts.Debug_Output(temp_buf);
				out_buf.CR().Cat(temp_buf);
				f_out.WriteLine(out_buf);
				logger.Log(out_buf);
			}
		}
	}
	return ok;
}
//
//
//
class PPEanComDocument {
public:
	struct QValue {
		QValue() : Q(0), Unit(0), Currency(0), Value(0.0)
		{
		}
		int    Q;    // Qualifier
		int    Unit; // UNIT_XXX
		int    Currency; // Валюта (для денежных величин)
		double Value; // Значение
	};
	//
	//
	//
	enum {
		/*
			2   = Delivery date/time, requested
			10  = Shipment date/time, requested
			11  = Despatch date and/or time
			15  = Promotion start date/time
			37  = Ship not before date/time
			38  = Ship not later than date/time
			61  = Cancel if not delivered by this date
			63  = Delivery date/time, latest
			64  = Delivery date/time, earliest
			69  = Delivery date/time, promised for
			76  = Delivery date/time, scheduled for
			X14 = Requested for delivery week commencing (GS1 Temporary Code)
			137 = Document/message date/time
			200 = Pick-up/collection date/time of cargo
			235 = Collection date/time, latest
			263 = Invoicing period
			273 = Validity period
			282 = Confirmation date lead time
			383 = Cancel if not shipped by this date
			//
			//
			1 = Service completion date/time, actual. Actual date/time on which the service was completed. 	
			2 = Delivery date/time, requested. Date on which buyer requests goods to be delivered. 	
			3 = Invoice date/time. [2376] Date when a Commercial Invoice is issued. 	
			4 = Order date/time. [2010] Date when an order is issued. 	
			7 = Effective date/time. Date and/or time at which specified event or document becomes effective. 	
			8 = Order received date/time. Date/time when the purchase order is received by the seller. 	
			9 = Processing date/time. Date/time of processing. 	
			10 = Shipment date/time, requested. Date on which goods should be shipped or despatched by the supplier. 	
			11 = Despatch date and/or time. (2170) Date/time on which the goods are or are expected to be despatched or shipped. 	
			12 = Terms discount due date/time. Date by which payment should be made if discount terms are to apply. 	
			13 = Terms net due date. Date by which payment must be made. 	
			14 = Payment date/time, deferred. Date/time when instalments are due. 	
			15 = Promotion start date/time. Date/time when promotion activities begin. 	
			16 = Promotion end date/time. Date/time when promotion activities end. 	
			17 = Delivery date/time, estimated. Date and/or time when the shipper of the goods expects delivery will take place. 	
			18 = Installation date/time/period. The date/time/period of the act, or an instance of installing something or someone. 	
			35 = Delivery date/time, actual. Date/time on which goods or consignment are delivered at their destination. 	
			36 = Expiry date. Date of expiry of the validity of a referenced document, price information or any other referenced data element with a limited validity period. 	
			37 = Ship not before date/time. Goods should not be shipped before given date/time. 	
			38 = Ship not later than date/time. Date/time by which the goods should have been shipped. 	
			44 = Availability. Date/time when received item is available. 	
			50 = Goods receipt date/time. Date/time upon which the goods were received by a given party. 	
			55 = Confirmed date/time. Date/time which has been confirmed. 	
			58 = Clearance date (Customs). (3080) Date on which Customs formalities necessary to allow goods to be exported, to enter home use, or to be placed under another Customs procedure has been accomplished (CCC). 	
			59 = Inbond movement authorization date. Inland movement authorization date. 	
			61 = Cancel if not delivered by this date. The date on which cancellation should take place, if delivery has not occurred. 	
			63 = Delivery date/time, latest. Date identifying a point of time after which goods shall not or will not be delivered. 	
			64 = Delivery date/time, earliest. Date identifying a point in time before which the goods shall not be delivered. 	
			67 = Delivery date/time, current schedule. Delivery Date deriving from actual schedule. 	X15 	 
			69 = Delivery date/time, promised for [2138]. Date by which, or period within which, the merchandise should be delivered to the buyer, as agreed between the seller and the buyer (generic term). 	
			71 = Delivery date/time, requested for (after and including). Delivery is requested to happen after or on given date. 	
			72 = Delivery date/time, promised for (after and including). Delivery might take place earliest at given date. 	
			73 = Guarantee period. The period for which the guarantee is or will be granted. 	
			74 = Delivery date/time, requested for (prior to and including). Delivery is requested to happen prior to or including the given date. 	
			75 = Delivery date/time, promised for (prior to and including). Delivery might take place latest at given date. 	
			76 = Delivery date/time, scheduled for. The date/time for which delivery is scheduled. 	
			79 = Shipment date/time, promised for. Shipment might happen at given date/time. 	
			90 = Report start date. The date on which a report is to begin. 	
			91 = Report end date. The date on which a report is to end. 	
			94 = Production/manufacture date. Date on which goods are produced. 	
			95 = Bill of lading date. Date as specified on the bill of lading. 	
			99 = Quotation opening date. The date on which the quotation has been or may be opened. 	
			100 = Product ageing period before delivery. Period of time before delivery during which the product is ageing. 	
			102 =  Health problem period. Period of time of health problem. GS1 Description: Period of time of exceptional health problems. 	
			117 = Delivery date/time, first. First possible date/time for delivery. 	
			119 = Test completion date. Date when a test has been completed. 	
			124 = Despatch note date. [2218] Date when a Despatch Note is issued. 	
			126 = Contract date. [2326] Date when a Contract is agreed. 	
			128 = Delivery date/time, last. Date when the last delivery should be or has been accomplished. 	
			129 = Exportation date. Date when imported vessel/merchandise last left the country of export for the country of import. 	
			131 = Tax point date. Date on which tax is due or calculated. 	
			132 = Arrival date/time, estimated. (2348) Date/time when carrier estimates that a means of transport should arrive at the port of discharge or place of destination. 	
			133 = Departure date/time, estimated. Date/time when carrier estimates that a means of transport should depart at the place of departure. 	
			134 = Rate of exchange date/time. Date/time on which the exchange rate was fixed. 	
			136 = Departure date/time. [2280] Date (and time) of departure of means of transport. 	
			137 = Document/message date/time. (2006) Date/time when a document/message is issued. This may include authentication. 	
			138 = Payment date. [2034] Date on which an amount due is made available to the creditor, in accordance with the terms of payment. 	
			140 = Payment due date. Date/time at which funds should be made available. 	
			143 = Acceptance date/time of goods. [2126] Date on which the goods are taken over by the carrier at the place of acceptance (CMR 4). 	
			147 = Expiry date of export licence. [2078] Date of expiry of the validity of an Export Licence. 	
			151 = Importation date 	
			Date on which goods are imported, as determined by the governing Customs administration. 	
			152 = Exportation date for textiles 	
			Date when imported textiles last left the country of origin for the country of importation. 	
			154 = Acceptance date of document 	
			The date on which a document was accepted. 	
			GS1 Description:
			Date on which a document was, or will be, accepted. 	
			155 = Accounting period start date 	
			The first date of an accounting period. 	
			156 = Accounting period end date 	
			The last date of an accounting period. 	
			157 = Validity start date 	
			The first date of a period for which something is valid. 	
			162 = Release date of supplier 	
			Date when the supplier released goods. 	
			165 = Tax period start date. Date when a tax period begins. 	
			166 = Tax period end date. Date when a tax period ends. 	
			167 = Charge period start date. The charge period's first date. 	
			168 = Charge period end date. The charge period's last date. 	
			169 = Lead time. Time required between order entry till earliest goods delivery. 	
			171 = Reference date/time. Date/time on which the reference was issued. 	
			174 = Advise after date/time. The information must be advised after the date/time indicated. 	
			175 = Advise before date/time. The information must be advised before the date/time indicated. 	
			176 = Advise completed date/time. The advise has been completed at the date indicated. 	
			177 = Advise on date/time. The information must be advised on the date/time indicated. 	
			178 = Arrival date/time, actual. [2106] Date (and time) of arrival of means of transport. 	
			179 = Booking date/time. Date at which the booking was made. 	
			180 = Closing date/time 	
			Final date for delivering cargo to a liner ship. 	
			182 = Issue date 	
			Date when a document/message has been or will be issued. 	
			186 = Departure date/time, actual 	
			(2280) Date (and time) of departure of means of transport. 	
			189 = Departure date/time, scheduled 	
			Date (and time) of scheduled departure of means of transport. 	
			190 = Transhipment date/time 	
			Date and time of the transfer of the goods from one means of transport to another. 	
			191 = Delivery date/time, expected 	
			Date/time on which goods are expected to be delivered. 	
			192 = Expiration date/time of customs document 	
			Date on which validity of a customs document expires. 	
			193 = Execution date 	
			The date when ordered bank initiated the transaction. 	
			194 = Start date/time 	
			Date/time on which a period starts. 	
			195 = Expiry date of import licence 	
			[2272] Date of expiry of the validity of an Import Licence. 	
			199 = Positioning date/time of goods 	
			The date and/or time the goods have to be or have been positioned. 	
			200 = Pick-up/collection date/time of cargo 	
			Date/time at which the cargo is picked up. 	
			202 = Posting date 	
			The date when an entry is posted to an account. 	
			203 = Execution date/time, requested 	
			The date/time on which the ordered bank is requested to initiate the payment order, as specified by the originator (e.g. the date of the debit). 	
			206 = End date/time 	
			Date/time on which a period (from - to) ends. 	
			209 = Value date 	
			Date on which the funds are at the disposal of the beneficiary or cease to be at the disposal of the ordering customer. 	
			211 = 360/30 	
			Calculation is based on year of 360 days, month of 30 days. 	
			212 = 360/28-31 	
			Calculation is based on year of 360 days, month of 28-31 days. 	
			213 = 365-6/30 	
			Calculation is based on year of 365-6 days, month of 30 days. 	
			214 = 365-6/28-31 	
			Calculation is based on year of 365-6 days, month of 28-31 days. 	
			215 = 365/28-31 	
			Calculation is based on year of 365 days, month of 28-31 days. 	
			216 = 365/30 	
			Calculation is based on year of 365 days, month of 30 days. 	
			218 = Authentication/validation date/time 	
			The date/time of authentication and/or validation. 	
			219 = Crossborder date/time 	
			Date/time at which goods are transferred across a country border. 	
			221 = Interest period 	
			Number of days used for the calculation of interests. 	
			222 = Presentation date, latest 	
			Latest date for presentation of a document. 	
			223 = Delivery date/time, deferred 	
			New date and time of delivery calculated on basis of a consignee's requirement (chargeable). 	
			227 = Beneficiary's banks due date 	
			Date on which funds should be made available to the beneficiary's bank. 	
			231 = Arrival date/time, earliest 	
			Date/time of earliest arrival of means of transport. 	
			232 = Arrival date/time, scheduled 	
			Date (and time) of scheduled arrival of means of transport. 	
			234 = Collection date/time, earliest 	
			The transport order may be issued before the goods are ready for picking up. This date/time indicates from when on the carrier can have access to the consignment. 	
			235 = Collection date/time, latest 	
			In relation with the arrangements agreed between buyer and seller or between sender and main transport it may be necessary to specify the latest collection date/time. 	
			255 = Availability due date 	
			Date when ordered items should be available at a specified location. 	
			257 = Calculation date/time/period 	
			The date/time/period on which a calculation will take, or has taken, place. 	
			260 = Valuation date (Customs) 	
			Date when Customs valuation was made. 	
			261 = Release date/time 	
			Date/time assigned to identify the release of a set of rules, conditions, conventions, productions, etc. 	
			263 = Invoicing period 	
			Period for which an invoice is issued. 	
			265 = Due date 	
			The date on which some action should occur. 	
			266 = Validation date. The date on which something was made valid, ratified or confirmed. GS1 Description: The date on which the validation of a document or message has taken place. 	
			267 = Rate/price date/time. Date/time on which a rate/price is determined. 	
			273 = Validity period. Dates (from/to)/period referenced documents are valid. 	
			282 = Confirmation date lead time. Lead time is referenced to the date of confirmation.
			306 = Work period. Period of execution of works. 	
			310 = Received date/time. Date/time of receipt. 	
			315 = Agreement to pay date. Date on which the debtor agreed to pay. 	
			321 = Promotion date/period. Date/period relevant for specific promotion activities. 	
			322 = Accounting period. Self-explanatory. GS1 Description: Period related to a company's accounting procedures/fiscal calendar. 	
			324 = Processing date/period. Date/period a specific process happened/will happen. 	
			325 = Tax period. Period a tax rate/tax amount etc. is applicable. GS1 Description: A period which is designated by tax authorities, e.g. VAT period. 	
			326 = Charge period. Period a specified charge is valid for.
			328 = Payroll deduction date/time. Date/time of a monetary deduction made from the salary of a person on a payroll.
			334 = Status change date/time. Date/time when a status changes. 	
			349 = Packing end date. Date on which packing completed. 	
			350 = Test start date. Date when a test has been started. 	
			351 = Inspection date. Date of inspection. 	
			356 = Sales date, and or time, and or period. The date, and or time, and or period on which a sale took place. 	
			357 = Cancel if not published by this date. Cancel if not published by this date. 	
			358 = Scheduled for delivery on or after. Scheduled for delivery on or after the specified date, and or time. 	
			359 = Scheduled for delivery on or before. Scheduled for delivery on or before specified date and or time. 	
			360 = Sell by date. The date by which a product should be sold. 	
			361 = Best before date. The best before date. 	
			362 = End availability date. The end date of availability. 	
			363 = Total shelf life period. A period indicating the total shelf life of a product. 	
			364 = Minimum shelf life remaining at time of despatch period. Period indicating the minimum shelf life remaining for a product at the time of leaving the supplier. 	
			365 = Packaging date. The date on which the packaging of a product took place. 	
			366 = Inventory report date. Date on which a inventory report is made. 	
			367 = Previous meter reading date. Date on which the previous reading of a meter took place. 	
			368 = Latest meter reading date. Date on which the latest reading of a meter took place. 	
			369 = Date and or time of handling, estimated. The date and or time when the handling action is estimated to take place. 	
			381 = Product Lifespan at time of production (GS1 Temporary Code). The total lifespan of a product at the time of its production. 	
			382 = Earliest sale date. The earliest date on which the product may be made available for sale. 	
			383 = Cancel if not shipped by this date. Cancel the order if goods not shipped by this date. GS1 Description: Cancel the identified order if it has not been shipped/despatched by this date. 	
			417 = Previous booking date/time. Date/time at which the previous booking was made. 	
			418 = Minimum shelf life remaining at time of receipt. The minimum shelf life remaining at the time of receipt. 	
			419 = Forecast period. A period for which a forecast applies. GS1 Description: A validity period for a forecast. 	
			423 = First date of ordering. The first date on which ordering may take place. 	
			424 = Last date of ordering. The last date on which ordering may take place. 	
			434 = Maturity date. Date at which maturity occurs. 	
			447 = Creditor's requested value date. Date on which the creditor requests to be credited. 	
			448 = Referenced item creation date. Creation date of referenced item. 	
			454 = Accounting value date. Date against which the entry has to be legally allocated. 	
			496 = Reinstatement date. Identifies the date of reinstatement. 	
			506 = Back order delivery date/time/period. The date/time/period during which the delivery of a back order will take, or has taken, place. 	
			530 = Fumigation date and/or time. The date/or time on which fumigation is to occur or has taken place. 	
			531 = Payment period. A period of time in which a payment has been or will be made. 	
			536 = Review date. Date the item was or will be reviewed. 	
			541 = First published date. Date when material was first published. 	
			557 = Returned date. Date return takes place. GS1 Description: Date by which goods must be, or have been, returned. 	
			656 = Age. Length of time that a person or thing has existed. GS1 Description: Length of time that a person or animal has lived or a thing has existed. 	
			684 = Deletion date. The date on which deletion occurs. 	
			685 = First sale date and/or time and/or period. The first date, and/or time, and/or period a product was sold. 	
			686 = Last sale date and/or time and/or period. The last date, and/or time, and/or period a product was sold. 	
			706 = File generation date and/or time. Date and, or time of file generation. 	
			743 = Purchase order latest possible change date. Date identifying a point of time after which a purchase order cannot be changed. 	
			748 = Open period. Code identifying the period during which something is, was or will be open. 	
			750 = Before date. The specified before date.
			751 = After date. The specified after date. 	
			753 = Maturity date, optimal . Date at which optimal maturity occurs. 	
			754 = Product ageing duration, maximum. Maximum period of time during which the product is ageing. 	
			755 = Product ageing duration, minimum. Minimum period of time during which the product is ageing. 	
			758 = Trade item ship date/time, earliest possible. The earliest date/time that the trade item can be shipped. It indicates the earliest date that the trade item can be shipped. This is independent of any specific ship-from location. 	
			759 = Trade item ship date/time, latest possible. The latest date/time that the trade item can be shipped. It indicates the latest date that the trade item can be shipped. This is independent of any specific ship-from location. 	
			760 = Start date/time, maximum buying quantity. The date/time from which the maximum buying quantity may be purchased. The start date for when the maximum buying quantity is available to the trading partner. 	
			761 = Start date/time, minimum buying quantity. The start date for when the minimum buying quantity is available to the trading partner. 	
			762 = Marketing campaign end date/time, suggested. The date and or time suggested for the marketing campaign to end. The date suggested by the supplier for the campaign to end. It indicates the end of a marketing campaign. 	
			763 = Marketing campaign start date/time, suggested. The date and or time suggested for the marketing campaign to start. The date suggested by the supplier for the campaign to start. It indicates the beginning of a marketing campaign. 	
			764 = Start availability date. The start date of availability. Indicates the start date of the trade item's seasonal availability. 	
			765 = Seasonal availabilty calendar year. The calendar year of the season in which the trade item is available. 	
			766 = Goods pickup lead time. Minimum time required between order entry and goods release for pick-up. Time (in weeks, days, hours …) required between order entry and the earliest goods release (use for pick-up, not use for delivery). 	
			767 = Change date/time, latest. Most recent date and/or time that the information has been changed. A system generated value identifying the date and time a record was last updated. 	
			768 = End date/time, maximum buying quantity. The date and/or time until which the maximum buying quantity may be purchased. The end date for when the maximum buying quantity is no longer available to the trading partner. 	
			769 = End dat/time, minimum buying quantity. The date and/or time until which the minimum buying quantity may be purchased. The end date for when the minimum buying quantity is no longer available to the trading partner. 	
			770 = End date/time of exclusivity. The date and/or time until which a product is exclusive. The Date & Time at which a product is no longer exclusive to that trading partner. 	
			772 = Handling start date and/or time, actual . The actual date and/or time when the start of the handling action takes place. 	
			773 = Handling end date and/or time, estimated. The date and/or time when the end of the handling action is estimated to take place. 	
			774 = Handling end date and/or time, actual. The actual date and/or time when the end of the handling action takes place. 	
			775 = Minimum product lifespan for consumer. The minimum life span of the product remaining after selling it to the consumer, i.e. between the "sell by date" and the "use by date" of the product. 	
			789 = Opened trade item life span. The number of days the trade item that had been opened can remain on the shelf and must then be removed. 	
			793 = Reprocessing date/time. Date/time on which goods previously produced are re-processed. 	
			794 = First returnable date/time. The first date/time on or after which items can be returned. 	
			795 = Community visibility date/time. The date/time from which information becomes visible to the target community. 	
			796 = Catch date/time. Date/time of catch. 	
			799 = Validity end date. The last date of a period for which something is valid. 	
			800 = Next status report date. Date of the next status report 	
			801 = Service connection date/time, actual. The date/time on which a service was connected, e.g. telephone, water, etc. 	
			802 = Service disconnection date/time, actual. The date/time on which a service was DISconnected, e.g. telephone, water, etc. 	
			803 = Empty equipment required date/time/period. Date/time/period on which empty equipment is required. 	
			804 = Product sterilisation date. Date on which a product was sterilised. 	
			805 = Stock demand cover period, expected. A period of time when stocks are expected to cover demand for a product. 	
			806 = Shipment date/time, expected. Date and/or time when shipment is expected 	
			807 = Slaughtering date/time. Date/time of slaughtering 	
			808 = Animal birth date/time. Date/time when an animal was born 	
			809 = Seasonal availability end date. Indicates the end date of the trade item's seasonal availability 	
			21E = End validity date (GS1 Temporary Code). Date indicating the end date of a validity period. GS1 Note: Code marked for deletion. Use value 799 instead. 	
			40E = Next status date (GS1 Temporary Code). The next date on which a status report will be provided. GS1 Note: Code marked for deletion. Use value 800 instead. 	
			41E = Product lifespan from time of production (GS1 Temporary Code). An indication of the lifespan of a product from its time of production. 	
			44E = Connection date/time (GS1 Temporary Code). The date/time on which a service was connected, e.g. telephone, water, etc. Connection date/time (GS1 Temporary Code) 	
			45E = Disconnection date/time (GS1 Temporary Code). The date/time on which a service was disconnected, e.g. telephone, water, etc.
			GS1 Note: Code marked for deletion. Use value 802 instead. 	
			46E = Order completion date/time (GS1 Temporary Code) . The date/time on which an order was completed. 	
			50E = Empty equipment required date/time/period (GS1 Temporary Code) 	
				Date on which empty equipment is required. GS1 Note: Code marked for deletion. Use value 803 instead. 	
			54E = Stuffing date/time (GS1 Temporary Code). The date/time on which the stuffing of a container is to take place, or has taken place. 	
			55E = Un-stuffing date/time (GS1 Temporary Code). The date/time on which the un-stuffing of a container is to take place, or has taken place. 	
			61B = Cancel if not delivered by this date unless title not published at time (GS1 Temporary Code) 	
			Cancel the product ordered if it has not been delivered by this date unless it had not been published at the time of order.
			GS1 Note: Code marked for deletion. 	
			63B = Latest availability date if not yet despatched (GS1 Temporary Code) . The latest availability date required for a product or order if it has not already been despatched. 	
			90E = Product sterilisation date (GS1 Temporary Code). Date on which a product was sterilised. GS1 Note: Code marked for deletion. Use value 804 instead. 	
			91E = First freezing date (GS1 Temporary Code) . Date on which a product was first frozen. 	
			92E = Pension deduction payment date (GS1 Temporary Code). Date when the deduction was made from the salary of the employee. 	
			93E = Professional risk deduction payment date (GS1 Temporary Code). Date when the risk deduction was made from the salary of the employee. 	
			94E = Health care deduction payment date (GS1 Temporary Code). Date when health care deduction was made. 	
			96E = Stock cover period (GS1 Temporary Code). A period of time when all stocks are expected to cover demand for a product.
			GS1 Note: Code marked for deletion. Use value 805 instead. 	
			98E = Discharge date/time, start (GS1 Temporary Code) 	
			Date/time when all discharge operations on the transport means have actually been started. 	
			X13 = Expect to ship by (GS1 Temporary Code). The consignement is expected to be shipped by this date and/or time. GS1 Note: Code marked for deletion. Use value 806 instead. 	
			X14 = Requested for delivery week commencing (GS1 Temporary Code). Code requesting delivery of an order or order line during a specified week. 	
			X19 = Material Safety Data Sheet issue date (GS1 Temporary Code) 	
			A date when a Material Safety Data Sheet has been or will be issued. 	
			X20 = Slaughter date/time (GS1 Temporary Code). Date/time of slaughtering of an animal.
			GS1 Note: Code marked for deletion. Use value 807 instead. 	
			X21 = Animal Birth date/time (GS1 Temporary Code). Date/time when an animal was born. For traceability purposes.
			GS1 Note: Code marked for deletion. Use value 808 instead. 	
			X49 = Seasonal availability end date (GS1 Temporary Code). Indicates the end date of the trade item's seasonal availability. GS1 Note: Code marked for deletion. Use value 809 instead. 	
			X50 = Committed quantity announcement date (GS1 Temporary Code). The date when quantity committed by store could be announced to the supply chain of the retailer. 	
			XF2 = Date of issue and maturity (SWIFT Code). Date when a document/message has been issued and becomes mature. 	
			YB9 = Total credits (SWIFT Code). The total value of credits to a financial account.
		*/
		dtmqDlvry            =   2, // Delivery date/time, requested
		dtmqShipment         =  10, // Shipment date/time, requested
		dtmqDespatch         =  11, // Despatch date and/or time
		dtmqPromotionStart   =  15, // Promotion start date/time
		dtmqDlvryEstimated   =  17, // Delivery date/time, estimated
		dtmqShipNotBefore    =  37, // Ship not before date/time
		dtmqShipNotLater     =  38, // Ship not later than date/time
		dtmqInbondMovementAuth    =  59, // Inbond movement authorization date
		dtmqCancelIfNotDlvrd      =  61, // Cancel if not delivered by this date
		dtmqDeliveryLatest        =  63, // Delivery date/time, latest
		dtmqDeliveryEarliest      =  64, // Delivery date/time, earliest
		dtmqDeliveryPromisedFor   =  69, // Delivery date/time, promised for
		dtmqDeliveryScheduledFor  =  76, // Delivery date/time, scheduled for
		//X14 = Requested for delivery week commencing (GS1 Temporary Code)
		dtmqDocument              = 137, // Document/message date/time
		dtmqReleaseDateOfSupplier = 162, // Release date of supplier
		dtmqReference             = 171, // Reference date/time
		dtmqDlvryExpected         = 191, // Delivery date/time, expected
		dtmqStart                 = 194, // Start date/time
		dtmqCollectionOfCargo     = 200, // Pick-up/collection date/time of cargo
		dtmqEnd                   = 206, // End date/time
		dtmqCollectionEarliest    = 234, // Collection date/time, earliest
		dtmqCollectionLatest      = 235, // Collection date/time, latest
		dtmqInvoicingPeriod       = 263, // Invoicing period
		dtmqValidityPeriod        = 273, // Validity period
		dtmqConfirmation          = 282, // Confirmation date lead time
		dtmqScheduledDlvryOnOrAfter  = 358, // Scheduled for delivery on or after
		dtmqScheduledDlvryOnOrBefore = 359, // Scheduled for delivery on or before
		dtmqCancelIfNotShipped       = 383, // Cancel if not shipped by this date
		//54E = Stuffing date/time (GS1 Temporary Code)
	};
	/*
		2           DDMMYY Calendar date: D = Day; M = Month; Y = Year.
		101         YYMMDD Calendar date: Y = Year; M = Month; D = Day.
		102 !       CCYYMMDD Calendar date: C = Century ; Y = Year ; M = Month ; D = Day.
		104         MMWW-MMWW A period of time specified by giving the start week of a month followed by the end week of a month. Data is to be transmitted as consecutive characters without hyphen.
		107         DDD Day's number within a specific year: D = Day.
		108         WW Week's number within a specific year: W = Week.
		109         MM Month's number within a specific year: M = Month.
		110         DD Day's number within is a specific month.
		201         YYMMDDHHMM 	Calendar date including time without seconds: Y = Year; M = Month; D = Day; H = Hour; M = Minute.
		203  !      CCYYMMDDHHMM 	Calendar date including time with minutes: C=Century; Y=Year; M=Month; D=Day; H=Hour; M=Minutes.
		204         CCYYMMDDHHMMSS 	Calendar date including time with seconds: C=Century;Y=Year; M=Month;D=Day;H=Hour;M=Minute;S=Second.
		401         HHMM 	Time without seconds: H = Hour; m = Minute.
		501         HHMMHHMM 	Time span without seconds: H = Hour; m = Minute;.
		502         HHMMSS-HHMMSS 	Format of period to be given without hyphen.
		602         CCYY 	Calendar year including century: C = Century; Y = Year.
		609         YYMM 	Month within a calendar year: Y = Year; M = Month.
		610         CCYYMM 	Month within a calendar year: CC = Century; Y = Year; M = Month.
		615         YYWW 	Week within a calendar year: Y = Year; W = Week 1st week of January = week 01.
		616  !      CCYYWW 	Week within a calendar year: CC = Century; Y = Year; W = Week (1st week of January = week 01).
		713         YYMMDDHHMM-YYMMDDHHMM 	Format of period to be given in actual message without hyphen.
		715         YYWW-YYWW 	A period of time specified by giving the start week of a year followed by the end week of year (both not including century). Data is to be transmitted as consecutive characters without hyphen.
		717         YYMMDD-YYMMDD 	Format of period to be given in actual message without hyphen.
		718  !      CCYYMMDD-CCYYMMDD 	Format of period to be given without hyphen.
		719         CCYYMMDDHHMM-CCYYMMDDHHMM 	A period of time which includes the century, year, month, day, hour and minute. Format of period to be given in actual message without hyphen.
		720         DHHMM-DHHMM 	Format of period to be given without hyphen (D=day of the week, 1=Monday; 2=Tuesday; ... 7=Sunday).
		801         Year 	To indicate a quantity of years.
		802         Month 	To indicate a quantity of months.
		803         Week 	To indicate a quantity of weeks.
		804         Day 	To indicate a quantity of days.
		805         Hour 	To indicate a quantity of hours.
		806         Minute 	To indicate a quantity of minutes.
		810         Trimester 	To indicate a quantity of trimesters (three months).
		811         Half month 	To indicate a quantity of half months.
		21E         DDHHMM-DDHHMM (GS1 Temporary Code) 	Format of period to be given in actual message without hyphen.
	*/
	enum {
		dtmfmtCCYYMMDD  = 102, // CCYYMMDD
		dtmfmtCCYYMMDDHHMM      = 203, // CCYYMMDDHHMM
		dtmfmtCCYYWW    = 616, // CCYYWW
		dtmfmtCCYYMMDD_CCYYMMDD = 718, // CCYYMMDD-CCYYMMDD
	};
	struct DtmValue {
		DtmValue() : Q(0), Dtm(ZERODATETIME), DtmFinish(ZERODATETIME)
		{
		}
		int    Q; // Qualifier
		LDATETIME Dtm;
		LDATETIME DtmFinish;
	};
	enum {
		refqUndef = 0,
		refqAAB = 1, // Proforma invoice number
		refqAAJ, // Delivery order number
		refqAAK, // Despatch advice number
		refqAAM, // Waybill number. Reference number assigned to a waybill, see: 1001 = 700.
		refqAAN, // Delivery schedule number
		refqAAS, // Transport document number
		refqAAU, // Despatch note number
		refqABT, // Customs declaration number [1426] Number, assigned or accepted by Customs, to identify a Goods declaration.
		refqAFO, // Beneficiary's reference
		refqAIZ, // Consolidated invoice number
		refqALL, // Message batch number
		refqAMT, // Goods and Services Tax identification number
		refqAPQ, // Commercial account summary reference number
		refqASI, // Proof of delivery reference number. A reference number identifying a proof of delivery which is generated by the goods recipient.
		refqAWT, // Administrative Reference Code
		refqCD, // Credit note number
		refqCR, // Customer reference number
		refqCT, // Contract number
		refqDL, // Debit note number
		refqDQ, // Delivery note number
		refqFC, // Fiscal number Tax payer's number. Number assigned to individual persons as well as to corporates by a public institution;
			// this number is different from the VAT registration number.
		refqIP, // Import licence number
		refqIV, // Invoice number
		refqON, // Order number (buyer)
		refqPK, // Packing list number
		refqPL, // Price list number
		refqPOR, // Purchase order response number
		refqPP, // Purchase order change number
		refqRF, // Export reference number
		refqVN, // Order number (supplier)
		refqXA, // Company/place registration number. Company registration and place as legally required.

		refqAE, // Authorization for expense (AFE) number
		refqBO, // Blanket order number
		refqPD, // Promotion deal number
		refqUC, // Ultimate customer's reference number
		refqAKO, // Action authorization number
		refqANJ, // Authorization number
	};
	struct RefValue {
		RefValue() : Q(refqUndef)
		{
		}
		int    Q; // Qualifier
		SString Ref;
	};
	//
	// Descr: Варианты "Item type identification code" для PiaValue
	//
	enum {
		iticUndef = 0,
		iticAA, // Product version number. Number assigned by manufacturer or seller to identify the release of a product.
		iticAB, // Assembly. The item number is that of an assembly.
		iticAC, // HIBC (Health Industry Bar Code). Article identifier used within health sector to indicate data used conforms to HIBC.
		iticAD, // Cold roll number. Number assigned to a cold roll.
		iticAE, // Hot roll number. Number assigned to a hot roll.
		iticAF, // Slab number. Number assigned to a slab, which is produced in a particular production step.
		iticAG, // Software revision number. A number assigned to indicate a revision of software.
		iticAH, // UPC (Universal Product Code) Consumer package code (1-5-5). An 11-digit code that uniquely identifies consumer packaging of a product; does not have a check digit.
		iticAI, // UPC (Universal Product Code) Consumer package code (1-5-5-1). A 12-digit code that uniquely identifies the consumer packaging of a product, including a check digit.
		iticAJ, // Sample number. Number assigned to a sample.
		iticAK, // Pack number. Number assigned to a pack containing a stack of items put together (e.g. cold roll sheets (steel product)).
		iticAL, // UPC (Universal Product Code) Shipping container code (1-2-5-5). A 13-digit code that uniquely identifies the manufacturer's shipping unit, including the packaging indicator.
		iticAM, // UPC (Universal Product Code)/EAN (European article number) Shipping container code (1-2-5-5-1). A 14-digit code that uniquely identifies the manufacturer's shipping unit, including the packaging indicator and the check digit.
		iticAN, // UPC (Universal Product Code) suffix. A suffix used in conjunction with a higher level UPC (Universal product code) to define packing variations for a product.
		iticAO, // State label code. A code which specifies the codification of the state's labelling requirements.
		iticAP, // Heat number. Number assigned to the heat (also known as the iron charge) for the production of steel products.
		iticAQ, // Coupon number. A number identifying a coupon.
		iticAR, // Resource number. A number to identify a resource.
		iticAS, // Work task number. A number to identify a work task.
		iticAT, // Price look up number. Identification number on a product allowing a quick electronic retrieval of price information for that product.
		iticAU, // NSN (North Atlantic Treaty Organization Stock Number). Number assigned under the NATO (North Atlantic Treaty Organization) codification system to provide the identification of an approved item of supply.
		iticAV, // Refined product code. A code specifying the product refinement designation.
		iticAW, // Exhibit. A code indicating that the product is identified by an exhibit number.
		iticAX, // End item. A number specifying an end item.
		iticAY, // Federal supply classification. A code to specify a product's Federal supply classification.
		iticAZ, // Engineering data list. A code specifying the product's engineering data list.
		iticBA, // Milestone event number. A number to identify a milestone event.
		iticBB, // Lot number. A number indicating the lot number of a product.
		iticBC, // National drug code 4-4-2 format. A code identifying the product in national drug format 4-4-2.
		iticBD, // National drug code 5-3-2 format. A code identifying the product in national drug format 5-3-2.
		iticBE, // National drug code 5-4-1 format. A code identifying the product in national drug format 5-4-1.
		iticBF, // National drug code 5-4-2 format. A code identifying the product in national drug format 5-4-2.
		iticBG, // National drug code. A code specifying the national drug classification.
		iticBH, // Part number. A number indicating the part.
		iticBI, // Local Stock Number (LSN). A local number assigned to an item of stock.
		iticBJ, // Next higher assembly number. A number specifying the next higher assembly or component into which the product is being incorporated.
		iticBK, // Data category. A code specifying a category of data.
		iticBL, // Control number. To specify the control number.
		iticBM, // Special material identification code. A number to identify the special material code.
		iticBN, // Locally assigned control number. A number assigned locally for control purposes.
		iticBO, // Buyer's colour. Colour assigned by buyer.
		iticBP, // Buyer's part number. Reference number assigned by the buyer to identify an article.
		iticBQ, // Variable measure product code. A code assigned to identify a variable measure item.
		iticBR, // Financial phase. To specify as an item, the financial phase.
		iticBS, // Contract breakdown. To specify as an item, the contract breakdown.
		iticBT, // Technical phase. To specify as an item, the technical phase.
		iticBU, // Dye lot number. Number identifying a dye lot.
		iticBV, // Daily statement of activities. A statement listing activities of one day.
		iticBW, // Periodical statement of activities within a bilaterally agreed time period. Periodical statement listing activities within a bilaterally agreed time period.
		iticBX, // Calendar week statement of activities. A statement listing activities of a calendar week.
		iticBY, // Calendar month statement of activities. A statement listing activities of a calendar month.
		iticBZ, // Original equipment number. Original equipment number allocated to spare parts by the manufacturer.
		iticCC, // Industry commodity code. The codes given to certain commodities by an industry.
		iticCG, // Commodity grouping. Code for a group of articles with common characteristics (e.g. used for statistical purposes).
		iticCL, // Colour number. Code for the colour of an article.
		iticCR, // Contract number. Reference number identifying a contract.
		iticCV, // Customs article number. Code defined by Customs authorities to an article or a group of articles for Customs purposes.
		iticDR, // Drawing revision number. Reference number indicating that a change or revision has been applied to a drawing.
		iticDW, // Drawing. Reference number identifying a drawing of an article.
		iticEC, // Engineering change level. Reference number indicating that a change or revision has been applied to an article's specification.
		iticEF, // Material code. Code defining the material's type, surface, geometric form plus various classifying characteristics.
		iticEN, // International Article Numbering Association (EAN). Number assigned to a manufacturer's product according to the International Article Numbering Association.
		iticGB, // Buyer's internal product group code. Product group code used within a buyer's internal systems.
		iticGN, // National product group code. National product group code. Administered by a national agency.
		iticGS, // General specification number. The item number is a general specification number.
		iticHS, // Harmonised system. The item number is part of, or is generated in the context of the Harmonised Commodity Description and Coding System (Harmonised System), as developed and maintained by the World Customs Organization (WCO).
		iticIB, // ISBN (International Standard Book Number). A unique number identifying a book.
		iticIN, // ! Buyer's item number. The item number has been allocated by the buyer.
		iticIS, // ISSN (International Standard Serial Number). A unique number identifying a serial publication.
		iticIT, // Buyer's style number. Number given by the buyer to a specific style or form of an article, especially used for garments.
		iticIZ, // Buyer's size code. Code given by the buyer to designate the size of an article in textile and shoe industry.
		iticLI, // Line item number (GS1 Temporary Code). Number identifying a specific line within a document/message.
		iticMA, // Machine number. The item number is a machine number.
		iticMF, // Manufacturer's (producer's) article number. The number given to an article by its manufacturer.
		iticMN, // Model number. Reference number assigned by the manufacturer to differentiate variations in similar products in a class or group.
		iticMP, // Product/service identification number. Reference number identifying a product or service.
		iticNB, // Batch number. The item number is a batch number.
		iticON, // Customer order number. Reference number of a customer's order.
		iticPD, // Part number description. Reference number identifying a description associated with a number ultimately used to identify an article.
		iticPL, // Purchaser's order line number. Reference number identifying a line entry in a customer's order for goods or services.
		iticPO, // Purchase order number. Reference number identifying a customer's order.
		iticPV, // Promotional variant number. The item number is a promotional variant number.
		iticQS, // Buyer's qualifier for size. The item number qualifies the size of the buyer.
		iticRC, // Returnable container number. Reference number identifying a returnable container.
		iticRN, // Release number. Reference number identifying a release from a buyer's purchase order.
		iticRU, // Run number. The item number identifies the production or manufacturing run or sequence in which the item was manufactured, processed or assembled.
		iticRY, // Record keeping of model year. The item number relates to the year in which the particular model was kept.
		iticSA, // ! Supplier's article number. Number assigned to an article by the supplier of that article.
		iticSG, // Standard group of products (mixed assortment). The item number relates to a standard group of other items (mixed) which are grouped together as a single item for identification purposes.
		iticSK, // SKU (Stock keeping unit). Reference number of a stock keeping unit.
		iticSN, // Serial number. Identification number of an item which distinguishes this specific item out of a number of identical items.
		iticSRS, // RSK number. Plumbing and heating.
		iticSRT, // IFLS (Institut Francais du Libre Service) 5 digit product classification code. 5 digit code for product classification managed by the Institut Francais du Libre Service.
		iticSRU, // IFLS (Institut Francais du Libre Service) 9 digit product classification code. 9 digit code for product classification managed by the Institut Francais du Libre Service.
		iticSRV, // ! EAN.UCC Global Trade Item Number. A unique number, up to 14-digits, assigned according to the numbering structure of the EAN.UCC system. 'EAN' stands for the 'International Article Numbering Association', and 'UCC' for the 'Uniform Code Council'.
		iticSRW, // EDIS (Energy Data Identification System). European system for identification of meter data.
		iticSRX, // Slaughter number. Unique number given by a slaughterhouse to an animal or a group of animals of the same breed.
		iticSRY, // Official animal number. Unique number given by a national authority to identify an animal individually.
		iticSS, // Supplier's supplier article number. Article number referring to a sales catalogue of supplier's supplier.
		iticST, // Style number. Number given to a specific style or form of an article, especially used for garments.
		iticTG, // Transport group number. (8012) Additional number to form article groups for packing and/or transportation purposes.
		iticUA, // Ultimate customer's article number. Number assigned by ultimate customer to identify relevant article.
		iticUP, // UPC (Universal product code). Number assigned to a manufacturer's product by the Product Code Council.
		iticVN, // Vendor item number. Reference number assigned by a vendor/seller identifying a product/service/article.
		iticVP, // Vendor's (seller's) part number. Reference number assigned by a vendor/seller identifying an article.
		iticVS, // Vendor's supplemental item number. The item number is a specified by the vendor as a supplemental number for the vendor's purposes.
		iticVX, // Vendor specification number. The item number has been allocated by the vendor as a specification number.
		iticZZZ, // Mutually defined. A code assigned within a code list to be used on an interim basis and as defined among trading partners until a precise code can be assigned to the code list.
	};
	enum { // values significat!
		piaqAdditionalIdent = 1, // Additional identification
		piaqSubstitutedBy   = 3, // Substituted by
		piaqSubstitutedFor  = 4, // Substituted for
		piaqProductIdent    = 5, // Product identification
	};
	struct PiaValue { // @flat Additional product id
		PiaValue() : Q(0), Itic(0)
		{
			PTR32(Code)[0] = 0;
		}
		int    Q; // Qualifier
		int    Itic; // Item type identification code
		char   Code[24];
	};
	struct ImdValue { // item description
		ImdValue() : Q(0)
		{
		}
		int    Q; // Qualifier
		SString Text;
	};
	struct PartyValue {
		PartyValue() : PartyQ(0), CountryCode(0)
		{
		}
		PartyValue & operator = (const PartyValue & rS)
		{
			PartyQ = rS.PartyQ;
			CountryCode = rS.CountryCode;
			Code = rS.Code;
			Name = rS.Name;
			Addr = rS.Addr;
			City = rS.City;
			CountrySubEntity = rS.CountrySubEntity;
			ZIP = rS.ZIP;
			TSCollection_Copy(RefL, rS.RefL);
			return *this;
		}
		int    PartyQ;
		int    CountryCode;
		SString Code;
		SString Name;
		SString City;
		SString CountrySubEntity;
		SString ZIP;
		SString Addr;
		TSCollection <RefValue> RefL;
	};
	//
	// (1129) --- Action request/notification description code
	// 1  = Added. The information is to be or has been added. This line item is added to a referenced message or an existing data base.
	// 2  = Deleted. The information is to be or has been deleted. This line item is deleted from a referenced message or an existing data base.
	// 3  = Changed. The information is to be or has been changed. This line item is changed in a referenced message or an existing data base. A change to data on a data base would normally result in the creation of a history entry recording the correction.
	// 4  = No action. This line item is not affected by the actual message.
	// 5  = Accepted without amendment. This line item is entirely accepted by the seller.
	// 6  = Accepted with amendment. This line item is accepted but amended by the seller.
	// 7  = Not accepted. This line item is not accepted by the seller. 	
	// 10 = Not found. This line item is not found in the referenced message.
	// 11 = Not amended. This line is not amended by the buyer.
	// 23 = Proposed amendment. A code used to indicate an amendment suggested by the sender.
	// 24 = Accepted with amendment, no confirmation required. Accepted with changes which require no confirmation. 	
	// 38 = Replaced. Provide a replacement. 	
	// 55 = Referred item, rejected. The item being referred to has been rejected.
	// 74 = Reopened. Opened again. A line item that was marked for deletion is reactivated in the referenced message or existing database. 	
	// 96 = Correction of error. A correction to information previously communicated which contained an error.
	// 115 = Prior addition. This existing line item becomes available at an earlier date. A line item, which has already been added with a certain start validity date is added again with a validity date prior to that of the last addition in the referenced message or existing database. 	
	// 11E = Reporting item details included (SWIFT Code). Reporting item details are included.
	// 12E = No advice (SWIFT Code). No details will be advised.
	// 13E = Reporting item details sent separately (SWIFT Code). Reporting item details have been transmitted separately.
	// 14E = Reporting item details to follow (SWIFT Code). Reporting item details will be sent separately.
	// XB6 = Reporting item details advised just-in-time (SWIFT Code). Reporting item details will be advised just-in-time. 	
	// YF2 = Referral item accepted (SWIFT Code). The referral item being report has been accepted.
	//
	enum {
		linarnAdded            =  1,
		linarnDeleted          =  2,
		linarnChanged          =  3,
		linarnNoAction         =  4,
		linarnAcceptedWithoutAmendment =  5,
		linarnAcceptedWithAmendment    =  6,
		linarnNotAccepted      =  7,
		linarnNotFound         = 10,
		linarnNotAmended       = 11,
		linarnProposedAmendment        = 23,
		linarnAcceptedWithAmendmentNoCfm = 24,
		linarnReplaced         = 38,
		linarnReferredItemRejected     = 55,
		linarnReopened         = 74,
		linarnCorrectionOfError        = 96,
		linarnPriorAddition    = 115 
	};
	struct LinValue {
		LinValue() : LineN(0), Action(0)
		{
		}
		int    LineN;
		int    Action;
		SString GoodsCode;
	};
	//
	// Descr: To specify currencies used in the transaction and relevant details for the rate of exchange.
	//
	struct CuxValue { 
		CuxValue() : UsageQ(0), TypeQ(0), Rate(0.0)
		{
			PTR32(Ident)[0] = 0;
		}
		CuxValue & Z()
		{
			UsageQ = 0;
			TypeQ = 0;
			PTR32(Ident)[0] = 0;
			Rate = 0.0;
			return *this;
		}
		int    UsageQ; // Currency usage code qualifier
			// 2 - reference currency
			// 3 - target currency
		int    TypeQ;  // Currency type code qualifier
			//  4 - Invoicing currency
			// 10 - Pricing currency
			// 11 - Payment currenc
		char   Ident[8]; // ISO 4217 three alpha
		double Rate;

	};
	struct DocumentDetailValue {
		DocumentDetailValue()
		{
		}
		DocumentDetailValue & operator = (const DocumentDetailValue & rS)
		{
			LinV = rS.LinV;
			TSCollection_Copy(ImdL, rS.ImdL);
			PiaL = rS.PiaL;
			MoaL = rS.MoaL;
			QtyL = rS.QtyL;
			PriL = rS.PriL;
			return *this;
		}
		LinValue LinV;
		TSArray <PiaValue> PiaL;
		TSCollection <ImdValue> ImdL;
		TSVector <QValue> MoaL;
		TSVector <QValue> QtyL;
		TSVector <QValue> PriL;
	};
	struct DocumentValue {
		DocumentValue() : FuncMsgCode(0)
		{
		}
		LDATE  GetBillDate() const
		{
			LDATE   result = ZERODATE;
			for(uint i = 0; i < DtmL.getCount(); i++) {
				const DtmValue & r_val = DtmL.at(i);
				if(r_val.Q == dtmqDocument) {
					result = r_val.Dtm.d;
					break;
				}
			}
			return result;
		}
		LDATE  GetBillDueDate() const
		{
			LDATE   result = ZERODATE;
			for(uint i = 0; i < DtmL.getCount(); i++) {
				const DtmValue & r_val = DtmL.at(i);
				if(oneof2(r_val.Q, dtmqDlvry, dtmqDlvryEstimated))
					result = r_val.Dtm.d;
			}
			return result;
		}
		LDATE  GetLinkedOrderDate() const
		{
			LDATE   result = ZERODATE;
			for(uint i = 0; i < DtmL.getCount(); i++) {
				const DtmValue & r_val = DtmL.at(i);
				if(r_val.Q == dtmqReference)
					result = r_val.Dtm.d;
			}
			return result;
		}
		const char * GetFinalBillCode() const
		{
			if(BgmIdent.NotEmpty())
				return BgmIdent;
			else if(UnhIdent.NotEmpty())
				return UnhIdent;
			else
				return "";
		}
		int    FuncMsgCode;
		SString UnhIdent;
		SString BgmIdent;
		CuxValue Cux;
		TSCollection <RefValue> RefL;
		TSVector <DtmValue> DtmL;
		TSVector <QValue> MoaL;
		TSCollection <PartyValue> PartyL;
		TSCollection <DocumentDetailValue> DetailL;
	};

	static int FASTCALL GetMsgSymbByType(int msgType, SString & rSymb);
	static int FASTCALL GetMsgTypeBySymb(const char * pSymb);
	static int FASTCALL GetRefqSymb(int refq, SString & rSymb);
	static int FASTCALL GetRefqBySymb(const char * pSymb);
	static int FASTCALL GetPartyqSymb(int refq, SString & rSymb);
	static int FASTCALL GetPartyqBySymb(const char * pSymb);
	static int FASTCALL GetIticSymb(int refq, SString & rSymb);
	static int FASTCALL GetIticBySymb(const char * pSymb);
	static int FASTCALL GetPriceqSymb(int priceq, SString & rSymb);
	static int FASTCALL GetPriceqBySymb(const char * pSymb);

	explicit PPEanComDocument(PPEdiProcessor::ProviderImplementation * pPi);
	~PPEanComDocument();
	int    Write_MessageHeader(SXml::WDoc & rDoc, int msgType, const char * pMsgId);
	int    Read_MessageHeader(xmlNode * pFirstNode, SString & rMsgType, SString & rMsgId); // @notimplemented
	//
	// Descr: Коды функций сообщения
	// Message function code:
	// 1 Cancellation. Message cancelling a previous transmission for a given transaction.
	// 2 Addition. Message containing items to be added.
	// 3 Deletion. Message containing items to be deleted.
	// 4 Change. Message containing items to be changed.
	// 5 Replace. Message replacing a previous message.
	// 6 Confirmation. Message confirming the details of a previous transmission where such confirmation is required or
	//	recommended under the terms of a trading partner agreement.
	// 7 Duplicate. The message is a duplicate of a previously generated message.
	// 8 Status. Code indicating that the referenced message is a status.
	// 9 Original. Initial transmission related to a given transaction.
	// 10 Not found. Message whose reference number is not filed.
	// 11 Response. Message responding to a previous message or document.
	// 12 Not processed. Message indicating that the referenced message was received but not yet processed.
	// 13 Request. Code indicating that the referenced message is a request.
	// 14 Advance notification. Code indicating that the information contained in the
	//	message is an advance notification of information to follow.
	// 15 Reminder. Repeated message transmission for reminding purposes.
	// 16 Proposal. Message content is a proposal.
	// 17 Cancel, to be reissued. Referenced transaction cancelled, reissued message will follow.
	// 18 Reissue. New issue of a previous message (maybe cancelled).
	// 19 Seller initiated change. Change information submitted by buyer but initiated by seller.
	// 20 Replace heading section only. Message to replace the heading of a previous message.
	// 21 Replace item detail and summary only. Message to replace item detail and summary of a previous message.
	// 22 Final transmission. Final message in a related series of messages together
	//	making up a commercial, administrative or transport transaction.
	// 23 Transaction on hold. Message not to be processed until further release information.
	// 24 Delivery instruction. Delivery schedule message only used to transmit short-term delivery instructions.
	// 25 Forecast. Delivery schedule message only used to transmit long- term schedule information.
	// 26 Delivery instruction and forecast. Combination of codes '24' and '25'.
	// 27 Not accepted. Message to inform that the referenced message is not accepted by the recipient.
	// 28 Accepted, with amendment in heading section. Message accepted but amended in heading section.
	// 29 Accepted without amendment. Referenced message is entirely accepted.
	// 30 Accepted, with amendment in detail section. Referenced message is accepted but amended in detail section.
	// 31 Copy. Indicates that the message is a copy of an original message that has been sent, e.g. for action or information.
	// 32 Approval. A message releasing an existing referenced message for action to the receiver.
	// 33 Change in heading section. Message changing the referenced message heading section.
	// 34 Accepted with amendment. The referenced message is accepted but amended.
	// 35 Retransmission. Change-free transmission of a message previously sent.
	// 36 Change in detail section. Message changing referenced detail section.
	// 37 Reversal of a debit. Reversal of a previously posted debit.
	// 38 Reversal of a credit. Reversal of a previously posted credit.
	// 39 Reversal for cancellation. Code indicating that the referenced message is reversing
	//	a cancellation of a previous transmission for a given transaction.
	// 40 Request for deletion. The message is given to inform the recipient to delete the referenced transaction.
	// 41 Finishing/closing order. Last of series of call-offs.
	// 42 Confirmation via specific means. Message confirming a transaction previously agreed via other means (e.g. phone).
	// 43 Additional transmission. Message already transmitted via another communication
	//	channel. This transmission is to provide electronically processable data only.
	// 44 Accepted without reserves. Message accepted without reserves.
	// 45 Accepted with reserves. Message accepted with reserves.
	// 46 Provisional. Message content is provisional.
	// 47 Definitive. Message content is definitive.
	// 48 Accepted, contents rejected. Message to inform that the previous message is received, but it cannot be processed due to regulations, laws, etc.
	// 49 Settled dispute. The reported dispute is settled.
	// 50 Withdraw. Message withdrawing a previously approved message.
	// 51 Authorisation. Message authorising a message or transaction(s).
	// 52 Proposed amendment. A code used to indicate an amendment suggested by the sender.
	// 53 Test. Code indicating the message is to be considered as a test.
	// 54 Extract. A subset of the original.
	// 55 Notification only. The receiver may use the notification information for analysis only.
	// 56 Advice of ledger booked items. An advice that items have been booked in the ledger.
	// 57 Advice of items pending to be booked in the ledger. An advice that items are pending to be booked in the ledger.
	// 58 Pre-advice of items requiring further information. A pre-advice that items require further information.
	// 59 Pre-adviced items. A pre-advice of items.
	// 60 No action since last message. Code indicating the fact that no action has taken place since the last message.
	// 61 Complete schedule. The message function is a complete schedule.
	// 62 Update schedule. The message function is an update to a schedule.
	// 63 Not accepted, provisional. Not accepted, subject to confirmation.
	// +64 Verification. The message is transmitted to verify information.
	// +65 Unsettled dispute. To report an unsettled dispute.
	//
	enum {
		fmsgcodeReplace      =  5, // Replace
		fmsgcodeConfirmation =  6, // Confirmation
		fmsgcodeDuplicate    =  7, // Duplicate
		fmsgcodeOriginal     =  9, // Original
		fmsgcodeProposal     = 16, // Proposal
		fmsgcodeAcceptedWOA  = 29, // Accepted without amendment
		fmsgcodeCopy = 31, // Copy
		fmsgcodeConfirmationVieSpcMeans = 42, // Confirmation via specific means
		fmsgcodeProvisional  = 46  // Provisional
	};
	//
	// Коды документов в BeginningOfMessage
	// 220 = Order
	// 221 = Blanket order
	// 224 = Rush order
	// 225 = Repair order
	// 226 = Call off order
	// 227 = Consignment order
	// 22E = Manufacturer raised order (GS1 Temporary Code)
	// 23E = Manufacturer raised consignment order (GS1 Temporary Code)
	// 258 = Standing order
	// 237 = Cross docking services order
	// 400 = Exceptional order
	// 401 = Transshipment order
	// 402 = Cross docking order
	//
	int    Write_BeginningOfMessage(SXml::WDoc & rDoc, const char * pDocCode, const char * pDocIdent, int funcMsgCode);
	int    Read_BeginningOfMessage(xmlNode * pFirstNode, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode);
	int    Write_UNT(SXml::WDoc & rDoc, const char * pDocCode, uint segCount);
	int    Read_UNT(xmlNode * pFirstNode, SString & rDocCode, uint * pSegCount);
	//
	int    Write_DTM(SXml::WDoc & rDoc, int dtmKind, int dtmFmt, const LDATETIME & rDtm, const LDATETIME * pFinish);
	int    Read_DTM(xmlNode * pFirstNode, TSVector <DtmValue> & rList);
	int    Write_RFF(SXml::WDoc & rDoc, int refQ, const char * pRef); // reference
	int    Read_RFF(xmlNode * pFirstNode, TSCollection <RefValue> & rList); // reference
	int    Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN);
	int    Read_NAD(xmlNode * pFirstNode, PartyValue & rV);
	int    Write_CUX(SXml::WDoc & rDoc, const char * pCurrencyCode3);
	int    Read_CUX(xmlNode * pFirstNode, SString & rCurrencyCode3); // @notimplemented
	int    Write_CPS(SXml::WDoc & rDoc);
	int    Read_CPS(xmlNode * pFirstNode);
	enum { // values significat!
		amtqAmtDue                =   9, // Amount due/amount payable
		amtqCashDiscount          =  21, // Cash discount
		amtqCashOnDeliveryAmt     =  22, // Cash on delivery amount
		amtqChargeAmt             =  23, // Charge amount
		amtqInvoiceItemAmt        =  38, // Invoice item amount
		amtqInvoiceTotalAmt       =  39, // Invoice total amount
		amtqCustomsValue          =  40, // Customs value
		amtqFreightCharge         =  64, // Freight charge
		amtqTotalLnItemsAmt       =  79, // Total line items amount
		amtqLoadAndHandlCost      =  81, // Loading and handling cost
		amtqMsgTotalMonetaryAmt   =  86, // Message total monetary amount
		amtqOriginalAmt           =  98, // Original amount
		amtqTaxAmt                = 124, // Tax amount
		amtqTaxableAmt            = 125, // Taxable amount
		amtqTotalAmt              = 128, // Total amount (The amount specified is the total amount)
		amtqTotalServiceCharge    = 140, // Total service charge
		amtqUnitPrice             = 146, // Unit price (5110) Reporting monetary amount is a "per unit" amount. (цена без НДС)
		amtqMsgTotalDutyTaxFeeAmt = 176, // Message total duty/tax/fee amount
		amtqExactAmt              = 178, // Exact amount
		amtqLnItemAmt             = 203, // Line item amount
		amtqAllowanceAmt          = 204, // Allowance amount
		amtqTotalCharges          = 259, // Total charges
		amtqTotalAllowances       = 260, // Total allowances
		amtqInstalmentAmt         = 262, // Instalment amount
		amtqGoodsAndServicesTax   = 369, // Goods and services tax
		amtqTotalAmtInclVAT       = 388, // Total amount including Value Added Tax (VAT)
		amtqCalcBasisExclAllTaxes = 528, // Calculation basis excluding all taxes
		amtqUnloadAndHandlCost    = 542, // Unloading and handling cost

		amtqExtBias       = 10000, // Типы сумм, начинающиеся с этого номера представлены алфавитными кодами
			// и трактуются как специализированные (часто справочные) значения.
		amtqExt_XB5       = 10001 // XB5 Information amount (SWIFT Code). A monetary amount provided for information purposes.
	};
	int    Write_MOA(SXml::WDoc & rDoc, int amtQ, double amount);
	//
	// Descr: Считывает суммовую величину и заносит ее в вектор rList.
	//
	int    Read_MOA(xmlNode * pFirstNode, TSVector <QValue> & rList);
	enum {
		/*
			1	Discrete quantity. Individually separated and distinct quantity.
			2	Charge. Quantity relevant for charge.
			3	Cumulative quantity. Quantity accumulated.
			4	Interest for overdrawn account. Interest for overdrawing the account.
			5	Active ingredient dose per unit. The dosage of active ingredient per unit.
			6	Auditor. The number of entities that audit accounts.
			7	Branch locations, leased. The number of branch locations being leased by an entity.
			8	Inventory quantity at supplier's subject to inspection by customer. Quantity of goods which the customer requires the supplier to have in inventory and which may be inspected by the customer if desired.
			9	Branch locations, owned. The number of branch locations owned by an entity.
			10	Judgements registered. The number of judgements registered against an entity.
			11	Split quantity. Part of the whole quantity.
			12	Despatch quantity. Quantity despatched by the seller.
			13	Liens registered. The number of liens registered against an entity.
			14	Livestock. The number of animals kept for use or profit.
			15	Insufficient funds returned cheques. The number of cheques returned due to insufficient funds.
			16	Stolen cheques. The number of stolen cheques.
			17	Quantity on hand. The total quantity of a product on hand at a location. This includes as well units awaiting return to manufacturer, units unavailable due to inspection procedures and undamaged stock available for despatch, resale or use.
			18	Previous quantity. Quantity previously referenced.
			19	Paid-in security shares. The number of security shares issued and for which full payment has been made.
			20	Unusable quantity. Quantity not usable.
			21	Ordered quantity. The quantity which has been ordered.
			22	Quantity at 100%. Equivalent quantity at 100% purity.
			23	Active ingredient. Quantity at 100% active agent content.
			24	Inventory quantity at supplier's not subject to inspection by customer. Quantity of goods which the customer requires the supplier to have in inventory but which will not be checked by the customer.
			25	Retail sales. Quantity of retail point of sale activity.
			26	Promotion quantity. A quantity associated with a promotional event.
			27	On hold for shipment. Article received which cannot be shipped in its present form.
			28	Military sales quantity. Quantity of goods or services sold to a military organization.
			29	On premises sales. Sale of product in restaurants or bars.
			30	Off premises sales. Sale of product directly to a store.
			31	Estimated annual volume. Volume estimated for a year.
			32	Minimum delivery batch. Minimum quantity of goods delivered at one time.
			33	Maximum delivery batch. Maximum quantity of goods delivered at one time.
			34	Pipes. The number of tubes used to convey a substance.
			35	Price break from. The minimum quantity of a quantity range for a specified (unit) price.
			36	Price break to. Maximum quantity to which the price break applies.
			37	Poultry. The number of domestic fowl.
			38	Secured charges registered. The number of secured charges registered against an entity.
			39	Total properties owned. The total number of properties owned by an entity.
			40	Normal delivery. Quantity normally delivered by the seller.
			41	Sales quantity not included in the replenishment calculation. Sales which will not be included in the calculation of replenishment requirements.
			42	Maximum supply quantity, supplier endorsed. Maximum supply quantity endorsed by a supplier.
			43	Buyer. The number of buyers.
			44	Debenture bond. The number of fixed-interest bonds of an entity backed by general credit rather than specified assets.
			45	Debentures filed against directors. The number of notices of indebtedness filed against an entity?s directors.
			46	Pieces delivered. Number of pieces actually received at the final destination.
			47	Invoiced quantity. The quantity as per invoice.
			48	Received quantity. The quantity which has been received.
			49	Chargeable distance. Distance really charged by tariff appliance.
			50	Disposition undetermined quantity. Product quantity that has not yet had its disposition determined.
			51	Inventory category transfer. Inventory that has been moved from one inventory category to another.
			52	Quantity per pack. Quantity for each pack.
			53	Minimum order quantity. Minimum quantity of goods for an order.
			54	Maximum order quantity. Maximum quantity of goods for an order.
			55	Total sales. The summation of total quantity sales.
			56	Wholesaler to wholesaler sales. Sale of product to other wholesalers by a wholesaler.
			57	In transit quantity. A quantity that is en route.
			58	Quantity withdrawn. Quantity withdrawn from a location.
			59	Numbers of consumer units in the traded unit. Number of units for consumer sales in a unit for trading.
			60	Current inventory quantity available for shipment. Current inventory quantity available for shipment.
			61	Return quantity. Quantity of goods returned.
			62	Sorted quantity. The quantity that is sorted.
			63	Sorted quantity rejected. The sorted quantity that is rejected.
			64	Scrap quantity. Remainder of the total quantity after split deliveries.
			65	Destroyed quantity. Quantity of goods destroyed.
			66	Committed quantity. Quantity a party is committed to.
			67	Estimated reading quantity. The value that is estimated to be the reading of a measuring device (e.g. meter).
			68	End quantity. The quantity recorded at the end of an agreement or period.
			69	Start quantity. The quantity recorded at the start of an agreement or period.
			70	Cumulative quantity received. Cumulative quantity of all deliveries of this article received by the buyer.
			71	Cumulative quantity ordered. Cumulative quantity of all deliveries, outstanding and scheduled orders.
			72	Cumulative quantity received end of prior year. Cumulative quantity of all deliveries of the product received by the buyer till end of prior year.
			73	Outstanding quantity. Difference between quantity ordered and quantity received.
			74	Latest cumulative quantity. Cumulative quantity after complete delivery of all scheduled quantities of the product.
			75	Previous highest cumulative quantity. Cumulative quantity after complete delivery of all scheduled quantities of the product from a prior schedule period.
			76	Adjusted corrector reading. A corrector reading after it has been adjusted.
			77	Work days. Number of work days, e.g. per respective period.
			78	Cumulative quantity scheduled. Adding the quantity actually scheduled to previous cumulative quantity.
			79	Previous cumulative quantity. Cumulative quantity prior the actual order.
			80	Unadjusted corrector reading. A corrector reading before it has been adjusted.
			81	Extra unplanned delivery. Non scheduled additional quantity.
			82	Quantity requirement for sample inspection. Required quantity for sample inspection.
			83	Backorder quantity. The quantity of goods that is on back-order.
			84	Urgent delivery quantity. Quantity for urgent delivery.
			85	Previous order quantity to be cancelled. Quantity ordered previously to be cancelled.
			86	Normal reading quantity. The value recorded or read from a measuring device (e.g. meter) in the normal conditions.
			87	Customer reading quantity. The value recorded or read from a measuring device (e.g. meter) by the customer.
			88	Information reading quantity. The value recorded or read from a measuring device (e.g. meter) for information purposes.
			89	Quality control held. Quantity of goods held pending completion of a quality control assessment.
			90	As is quantity. Quantity as it is in the existing circumstances.
			91	Open quantity. Quantity remaining after partial delivery.
			92	Final delivery quantity. Quantity of final delivery to a respective order.
			93	Subsequent delivery quantity. Quantity delivered to a respective order after it's final delivery.
			94	Substitutional quantity. Quantity delivered replacing previous deliveries.
			95	Redelivery after post processing. Quantity redelivered after post processing.
			96	Quality control failed. Quantity of goods which have failed quality control.
			97	Minimum inventory. Minimum stock quantity on which replenishment is based.
			98	Maximum inventory. Maximum stock quantity on which replenishment is based.
			99	Estimated quantity. Quantity estimated.
			100	Chargeable weight. The weight on which charges are based.
			101	Chargeable gross weight. The gross weight on which charges are based.
			102	Chargeable tare weight. The tare weight on which charges are based.
			103	Chargeable number of axles. The number of axles on which charges are based.
			104	Chargeable number of containers. The number of containers on which charges are based.
			105	Chargeable number of rail wagons. The number of rail wagons on which charges are based.
			106	Chargeable number of packages. The number of packages on which charges are based.
			107	Chargeable number of units. The number of units on which charges are based.
			108	Chargeable period. The period of time on which charges are based.
			109	Chargeable volume. The volume on which charges are based.
			110	Chargeable cubic measurements. The cubic measurements on which charges are based.
			111	Chargeable surface. The surface area on which charges are based.
			112	Chargeable length. The length on which charges are based.
			113	Quantity to be delivered. The quantity to be delivered.
			114	Number of passengers. Total number of passengers on the conveyance.
			115	Number of crew. Total number of crew members on the conveyance.
			116	Number of transport documents. Total number of air waybills, bills of lading, etc. being reported for a specific conveyance.
			117	Quantity landed. Quantity of goods actually arrived.
			118	Quantity manifested. Quantity of goods contracted for delivery by the carrier.
			119	Short shipped. Indication that part of the consignment was not shipped.
			120	Split shipment. Indication that the consignment has been split into two or more shipments.
			121	Over shipped. Indication that more goods have been shipped than contracted for delivery.
			122	Short-landed goods. If quantity of goods actually landed is less than the quantity which appears in the documentation. This quantity is the difference between these quantities.
			123	Surplus goods. If quantity of goods actually landed is more than the quantity which appears in the documentation. This quantity is the difference between these quantities.
			124	Damaged goods. Quantity of goods which have deteriorated in transport such that they cannot be used for the purpose for which they were originally intended.
			125	Pilferage goods. Quantity of goods stolen during transport.
			126	Lost goods. Quantity of goods that disappeared in transport.
			127	Report difference. The quantity concerning the same transaction differs between two documents/messages and the source of this difference is a typing error.
			128	Quantity loaded. Quantity of goods loaded onto a means of transport.
			129	Units per unit price. Number of units per unit price.
			130	Allowance. Quantity relevant for allowance.
			131	Delivery quantity. Quantity required by buyer to be delivered.
			132	Cumulative quantity, preceding period, planned. Cumulative quantity originally planned for the preceding period.
			133	Cumulative quantity, preceding period, reached. Cumulative quantity reached in the preceding period.
			134	Cumulative quantity, actual planned. Cumulative quantity planned for now.
			135	Period quantity, planned. Quantity planned for this period.
			136	Period quantity, reached. Quantity reached during this period.
			137	Cumulative quantity, preceding period, estimated. Estimated cumulative quantity reached in the preceding period.
			138	Cumulative quantity, actual estimated. Estimated cumulative quantity reached now.
			139	Cumulative quantity, preceding period, measured. Surveyed cumulative quantity reached in the preceding period.
			140	Cumulative quantity, actual measured. Surveyed cumulative quantity reached now.
			141	Period quantity, measured. Surveyed quantity reached during this period.
			142	Total quantity, planned. Total quantity planned.
			143	Quantity, remaining. Quantity remaining.
			144	Tolerance. Plus or minus tolerance expressed as a monetary amount.
			145	Actual stock. The stock on hand, undamaged, and available for despatch, sale or use.
			146	Model or target stock. The stock quantity required or planned to have on hand, undamaged and available for use.
			147	Direct shipment quantity. Quantity to be shipped directly to a customer from a manufacturing site.
			148	Amortization total quantity. Indication of final quantity for amortization.
			149	Amortization order quantity. Indication of actual share of the order quantity for amortization.
			150	Amortization cumulated quantity. Indication of actual cumulated quantity of previous and actual amortization order quantity.
			151	Quantity advised. Quantity advised by supplier or shipper, in contrast to quantity actually received.
			152	Consignment stock. Quantity of goods with an external customer which is still the property of the supplier.
			153	Statistical sales quantity. Quantity of goods sold in a specified period.
			154	Sales quantity planned. Quantity of goods required to meet future demands. -Market intelligence quantity.
			155	Replenishment quantity. Quantity required to maintain the requisite on-hand stock of goods.
			156	Inventory movement quantity. To specify the quantity of an inventory movement.
			157	Opening stock balance quantity. To specify the quantity of an opening stock balance.
			158	Closing stock balance quantity. To specify the quantity of a closing stock balance.
			159	Number of stops. Number of times a means of transport stops before arriving at destination.
			160	Minimum production batch. The quantity specified is the minimum output from a single production run.
			161	Dimensional sample quantity. The quantity defined is a sample for the purpose of validating dimensions.
			162	Functional sample quantity. The quantity defined is a sample for the purpose of validating function and performance.
			163	Pre-production quantity. Quantity of the referenced item required prior to full production.
			164	Delivery batch. Quantity of the referenced item which constitutes a standard batch for deliver purposes.
			165	Delivery batch multiple. The multiples in which delivery batches can be supplied.
			166	All time buy. The total quantity of the referenced covering all future needs. Further orders of the referenced item are not expected.
			167	Total delivery quantity. The total quantity required by the buyer to be delivered.
			168	Single delivery quantity. The quantity required by the buyer to be delivered in a single shipment.
			169	Supplied quantity. Quantity of the referenced item actually shipped.
			170	Allocated quantity. Quantity of the referenced item allocated from available stock for delivery.
			171	Maximum stackability. The number of pallets/handling units which can be safely stacked one on top of another.
			172	Amortisation quantity. The quantity of the referenced item which has a cost for tooling amortisation included in the item price.
			173	Previously amortised quantity. The cumulative quantity of the referenced item which had a cost for tooling amortisation included in the item price.
			174	Total amortisation quantity. The total quantity of the referenced item which has a cost for tooling amortisation included in the item price.
			175	Number of moulds. The number of pressing moulds contained within a single piece of the referenced tooling.
			176	Concurrent item output of tooling. The number of related items which can be produced simultaneously with a single piece of the referenced tooling.
			177	Periodic capacity of tooling. Maximum production output of the referenced tool over a period of time.
			178	Lifetime capacity of tooling. Maximum production output of the referenced tool over its productive lifetime.
			179	Number of deliveries per despatch period. The number of deliveries normally expected to be despatched within each despatch period.
			180	Provided quantity. The quantity of a referenced component supplied by the buyer for manufacturing of an ordered item.
			181	Maximum production batch. The quantity specified is the maximum output from a single production run.
			182	Cancelled quantity. Quantity of the referenced item which has previously been ordered and is now cancelled.
			183	No delivery requirement in this instruction. This delivery instruction does not contain any delivery requirements.
			184	Quantity of material in ordered time. Quantity of the referenced material within the ordered time.
			185	Rejected quantity. The quantity of received goods rejected for quantity reasons.
			186	Cumulative quantity scheduled up to accumulation start date. The cumulative quantity scheduled up to the accumulation start date.
			187	Quantity scheduled. The quantity scheduled for delivery.
			188	Number of identical handling units. Number of identical handling units in terms of type and contents.
			189	Number of packages in handling unit. The number of packages contained in one handling unit.
			190	Despatch note quantity. The item quantity specified on the despatch note.
			191	Adjustment to inventory quantity. An adjustment to inventory quantity.
			192	Free goods quantity. Quantity of goods which are free of charge.
			193	Free quantity included. Quantity included to which no charge is applicable.
			194	Received and accepted. Quantity which has been received and accepted at a given location.
			195	Received, not accepted, to be returned. Quantity which has been received but not accepted at a given location and which will consequently be returned to the relevant party.
			196	Received, not accepted, to be destroyed. Quantity which has been received but not accepted at a given location and which will consequently be destroyed.
			197	Reordering level. Quantity at which an order may be triggered to replenish.
			198 Quantity in transit. Quantity which is currently in transit. Note: 1. This code value will be removed effective with directory D.04A.
			199	Inventory withdrawal quantity. Quantity which has been withdrawn from inventory since the last inventory report.
			200	Free quantity not included. Free quantity not included in ordered quantity.
			201	Recommended overhaul and repair quantity. To indicate the recommended quantity of an article required to support overhaul and repair activities.
			202	Quantity per next higher assembly. To indicate the quantity required for the next higher assembly.
			203	Quantity per unit of issue. Provides the standard quantity of an article in which one unit can be issued.
			204	Cumulative scrap quantity. Provides the cumulative quantity of an item which has been identified as scrapped.
			205	Publication turn size. The quantity of magazines or newspapers grouped together with the spine facing alternate directions in a bundle.
			206	Recommended maintenance quantity. Recommended quantity of an article which is required to meet an agreed level of maintenance.
			207	Labour hours. Number of labour hours.
			208	Quantity requirement for maintenance and repair of equipment. Quantity of the material needed to maintain and repair equipment.
			209	Additional replenishment demand quantity
				Incremental needs over and above normal replenishment calculations, but not intended to permanently change the model parameters.
			210	Returned by consumer quantity
				Quantity returned by a consumer.
			211	Replenishment override quantity
				Quantity to override the normal replenishment model calculations, but not intended to permanently change the model parameters.
			212	Quantity sold, net
				Net quantity sold which includes returns of saleable inventory and other adjustments.
			213	Transferred out quantity
				Quantity which was transferred out of this location.
			214	Transferred in quantity
				Quantity which was transferred into this location.
			215	Unsaleable quantity
				Quantity of inventory received which cannot be sold in its present condition.
			216	Consumer reserved quantity
				Quantity reserved for consumer delivery or pickup and not yet withdrawn from inventory.
			217	Out of inventory quantity
				Quantity of inventory which was requested but was not available.
			218	Quantity returned, defective or damaged
				Quantity returned in a damaged or defective condition.
			219	Taxable quantity
				Quantity subject to taxation.
			220	Meter reading
				The numeric value of measure units counted by a meter.
			221	Maximum requestable quantity
				The maximum quantity which may be requested.
			222	Minimum requestable quantity
				The minimum quantity which may be requested.
			223	Daily average quantity
				The quantity for a defined period divided by the number of days of the period.
			224	Budgeted hours
				The number of budgeted hours.
			225	Actual hours
				The number of actual hours.
			226	Earned value hours
				The number of earned value hours.
			227	Estimated hours
				The number of estimated hours.
			228	Level resource task quantity
				Quantity of a resource that is level for the duration of the task.
			229	Available resource task quantity
				Quantity of a resource available to complete a task.
			230	Work time units
				Quantity of work units of time.
			231	Daily work shifts
				Quantity of work shifts per day.
			232	Work time units per shift
				Work units of time per work shift.
			233	Work calendar units
				Work calendar units of time.
			234	Elapsed duration
				Quantity representing the elapsed duration.
			235	Remaining duration
				Quantity representing the remaining duration.
			236	Original duration
				Quantity representing the original duration.
			237	Current duration
				Quantity representing the current duration.
			238	Total float time
				Quantity representing the total float time.
			239	Free float time
				Quantity representing the free float time.
			240	Lag time. Quantity representing lag time.
			241	Lead time. Quantity representing lead time.
			242	Number of months. The number of months.
			243	Reserved quantity customer direct delivery sales. Quantity of products reserved for sales delivered direct to the customer.
			244	Reserved quantity retail sales. Quantity of products reserved for retail sales.
			245	Consolidated discount inventory. A quantity of inventory supplied at consolidated discount terms.
			246	Returns replacement quantity. A quantity of goods issued as a replacement for a returned quantity.
			247	Additional promotion sales forecast quantity. A forecast of additional quantity which will be sold during a period of promotional activity.
			248	Reserved quantity. Quantity reserved for specific purposes.
			249	Quantity displayed not available for sale. Quantity displayed within a retail outlet but not available for sale.
			250	Inventory discrepancy. The difference recorded between theoretical and physical inventory.
			251	Incremental order quantity. The incremental quantity by which ordering is carried out.
			252	Quantity requiring manipulation before despatch. A quantity of goods which needs manipulation before despatch.
			253	Quantity in quarantine. A quantity of goods which are held in a restricted area for quarantine purposes.
			254	Quantity withheld by owner of goods. A quantity of goods which has been withheld by the owner of the goods.
			255	Quantity not available for despatch. A quantity of goods not available for despatch.
			256	Quantity awaiting delivery. Quantity of goods which are awaiting delivery.
			257	Quantity in physical inventory. A quantity of goods held in physical inventory.
			258	Quantity held by logistic service provider. Quantity of goods under the control of a logistic service provider.
			259	Optimal quantity. The optimal quantity for a given purpose.
			260	Delivery quantity balance. The difference between the scheduled quantity and the quantity delivered to the consignee at a given date.
			261	Cumulative quantity shipped. Cumulative quantity of all shipments.
			262	Quantity suspended. The quantity of something which is suspended.
			263	Control quantity. The quantity designated for control purposes.
			264	Equipment quantity. A count of a quantity of equipment.
			265	Factor. Number by which the measured unit has to be multiplied to calculate the units used.
			266	Unsold quantity held by wholesaler. Unsold quantity held by the wholesaler.
			267	Quantity held by delivery vehicle. Quantity of goods held by the delivery vehicle.
			268	Quantity held by retail outlet. Quantity held by the retail outlet.
			269	Rejected return quantity. A quantity for return which has been rejected.
			270	Accounts. The number of accounts.
			271	Accounts placed for collection. The number of accounts placed for collection.
			272	Activity codes. The number of activity codes.
			273	Agents. The number of agents.
			274	Airline attendants. The number of airline attendants.
			275	Authorised shares. The number of shares authorised for issue.
			276	Employee average. The average number of employees.
			277	Branch locations. The number of branch locations.
			278	Capital changes. The number of capital changes made.
			279	Clerks. The number of clerks.
			280	Companies in same activity. The number of companies doing business in the same activity category.
			281	Companies included in consolidated financial statement. The number of companies included in a consolidated financial statement.
			282	Cooperative shares. The number of cooperative shares.
			283	Creditors. The number of creditors.
			284	Departments. The number of departments.
			285	Design employees. The number of employees involved in the design process.
			286	Physicians. The number of medical doctors.
			287	Domestic affiliated companies. The number of affiliated companies located within the country.
			288	Drivers. The number of drivers.
			289	Employed at location. The number of employees at the specified location.
			290	Employed by this company. The number of employees at the specified company.
			291	Total employees. The total number of employees.
			292	Employees shared. The number of employees shared among entities.
			293	Engineers. The number of engineers.
			294	Estimated accounts. The estimated number of accounts.
			295	Estimated employees at location. The estimated number of employees at the specified location.
			296	Estimated total employees. The total estimated number of employees.
			297	Executives. The number of executives.
			298	Agricultural workers. The number of agricultural workers.
			299	Financial institutions. The number of financial institutions.
			300	Floors occupied. The number of floors occupied.
			301	Foreign related entities. The number of related entities located outside the country.
			302	Group employees. The number of employees within the group.
			303	Indirect employees. The number of employees not associated with direct production.
			304	Installers. The number of employees involved with the installation process.
			305	Invoices. The number of invoices.
			306	Issued shares. The number of shares actually issued.
			307	Labourers. The number of labourers.
			308	Manufactured units. The number of units manufactured.
			309	Maximum number of employees. The maximum number of people employed.
			310	Maximum number of employees at location. The maximum number of people employed at a location.
			311	Members in group. The number of members within a group.
			312	Minimum number of employees at location. The minimum number of people employed at a location.
			313	Minimum number of employees. The minimum number of people employed.
			314	Non-union employees. The number of employees not belonging to a labour union.
			315	Floors. The number of floors in a building.
			316	Nurses. The number of nurses.
			317	Office workers. The number of workers in an office.
			318	Other employees. The number of employees otherwise categorised.
			319	Part time employees. The number of employees working on a part time basis.
			320	Accounts payable average overdue days. The average number of days accounts payable are overdue.
			321	Pilots. The number of pilots.
			322	Plant workers. The number of workers within a plant.
			323	Previous number of accounts. The number of accounts which preceded the current count.
			324	Previous number of branch locations. The number of branch locations which preceded the current count.
			325	Principals included as employees. The number of principals which are included in the count of employees.
			326	Protested bills. The number of bills which are protested.
			327	Registered brands distributed. The number of registered brands which are being distributed.
			328	Registered brands manufactured. The number of registered brands which are being manufactured.
			329	Related business entities. The number of related business entities.
			330	Relatives employed. The number of relatives which are counted as employees.
			331	Rooms. The number of rooms.
			332	Salespersons. The number of salespersons.
			333	Seats. The number of seats.
			334	Shareholders. The number of shareholders.
			335	Shares of common stock. The number of shares of common stock.
			336	Shares of preferred stock. The number of shares of preferred stock.
			337	Silent partners. The number of silent partners.
			338	Subcontractors. The number of subcontractors.
			339	Subsidiaries. The number of subsidiaries.
			340	Law suits. The number of law suits.
			341	Suppliers. The number of suppliers.
			342	Teachers. The number of teachers.
			343	Technicians. The number of technicians.
			344	Trainees. The number of trainees.
			345	Union employees. The number of employees who are members of a labour union.
			346	Number of units. The quantity of units.
			347	Warehouse employees. The number of employees who work in a warehouse setting.
			348	Shareholders holding remainder of shares. Number of shareholders owning the remainder of shares.
			349	Payment orders filed. Number of payment orders filed.
			350	Uncovered cheques. Number of uncovered cheques.
			351	Auctions. Number of auctions.
			352	Units produced. The number of units produced.
			353	Added employees. Number of employees that were added to the workforce.
			354	Number of added locations. Number of locations that were added.
			355	Total number of foreign subsidiaries not included in financial statement. The total number of foreign subsidiaries not included in the financial statement.
			356	Number of closed locations. Number of locations that were closed.
			357	Counter clerks. The number of clerks that work behind a flat-topped fitment.
			358	Payment experiences in the last 3 months. The number of payment experiences received for an entity over the last 3 months.
			359	Payment experiences in the last 12 months. The number of payment experiences received for an entity over the last 12 months.
			360	Total number of subsidiaries not included in the financial statement. The total number of subsidiaries not included in the financial statement.
			361	Paid-in common shares. The number of paid-in common shares.
			362	Total number of domestic subsidiaries not included in financial statement. The total number of domestic subsidiaries not included in the financial statement.
			363	Total number of foreign subsidiaries included in financial statement. The total number of foreign subsidiaries included in the financial statement.
			364	Total number of domestic subsidiaries included in financial statement. The total number of domestic subsidiaries included in the financial statement.
			365	Total transactions. The total number of transactions.
			366	Paid-in preferred shares. The number of paid-in preferred shares.
			367	Employees. Code specifying the quantity of persons working for a company, whose services are used for pay.
			368	Active ingredient dose per unit, dispensed. The dosage of active ingredient per dispensed unit.
			369	Budget. Budget quantity.
			370	Budget, cumulative to date. Budget quantity, cumulative to date.
			371	Actual units. The number of actual units.
			372	Actual units, cumulative to date. The number of cumulative to date actual units.
			373	Earned value. Earned value quantity.
			374	Earned value, cumulative to date. Earned value quantity accumulated to date.
			375	At completion quantity, estimated. The estimated quantity when a project is complete.
			376	To complete quantity, estimated. The estimated quantity required to complete a project.
			377	Adjusted units. The number of adjusted units.
			378	Number of limited partnership shares. Number of shares held in a limited partnership.
			379	National business failure incidences. Number of firms in a country that discontinued with a loss to creditors.
			380	Industry business failure incidences. Number of firms in a specific industry that discontinued with a loss to creditors.
			381	Business class failure incidences. Number of firms in a specific class that discontinued with a loss to creditors.
			382	Mechanics. Number of mechanics.
			383	Messengers. Number of messengers.
			384	Primary managers. Number of primary managers.
			385	Secretaries. Number of secretaries.
			386	Detrimental legal filings. Number of detrimental legal filings.
			387	Branch office locations, estimated. Estimated number of branch office locations.
			388	Previous number of employees. The number of employees for a previous period.
			389	Asset seizers. Number of entities that seize assets of another entity.
			390	Out-turned quantity. The quantity discharged.
			391	Material on-board quantity, prior to loading. The material in vessel tanks, void spaces, and pipelines prior to loading.
			392	Supplier estimated previous meter reading. Previous meter reading estimated by the supplier.
			393	Supplier estimated latest meter reading. Latest meter reading estimated by the supplier.
			394	Customer estimated previous meter reading. Previous meter reading estimated by the customer.
			395	Customer estimated latest meter reading. Latest meter reading estimated by the customer.
			396	Supplier previous meter reading. Previous meter reading done by the supplier.
			397	Supplier latest meter reading. Latest meter reading recorded by the supplier.
			398	Maximum number of purchase orders allowed. Maximum number of purchase orders that are allowed.
			399	File size before compression. The size of a file before compression.
			400	File size after compression. The size of a file after compression.
			401	Securities shares. Number of shares of securities.
			402	Patients. Number of patients.
			403	Completed projects. Number of completed projects.
			404	Promoters. Number of entities who finance or organize an event or a production.
			405	Administrators. Number of administrators.
			406	Supervisors. Number of supervisors.
			407	Professionals. Number of professionals.
			408	Debt collectors. Number of debt collectors.
			409	Inspectors. Number of individuals who perform inspections.
			410	Operators. Number of operators.
			411	Trainers. Number of trainers.
			412	Active accounts. Number of accounts in a current or active status.
			413	Trademarks used. Number of trademarks used.
			414	Machines. Number of machines.
			415	Fuel pumps. Number of fuel pumps.
			416	Tables available. Number of tables available for use.
			417	Directors. Number of directors.
			418	Freelance debt collectors. Number of debt collectors who work on a freelance basis.
			419	Freelance salespersons. Number of salespersons who work on a freelance basis.
			420	Travelling employees. Number of travelling employees.
			421	Foremen. Number of workers with limited supervisory responsibilities.
			422	Production workers. Number of employees engaged in production.
			423	Employees not including owners. Number of employees excluding business owners.
			424	Beds. Number of beds.
			425	Resting quantity. A quantity of product that is at rest before it can be used.
			426	Production requirements. Quantity needed to meet production requirements.
			427	Corrected quantity. The quantity has been corrected.
			428	Operating divisions. Number of divisions operating.
			429	Quantitative incentive scheme base. Quantity constituting the base for the quantitative incentive scheme.
			430	Petitions filed. Number of petitions that have been filed.
			431	Bankruptcy petitions filed. Number of bankruptcy petitions that have been filed.
			432	Projects in process. Number of projects in process.
			433	Changes in capital structure. Number of modifications made to the capital structure of an entity.
			434	Detrimental legal filings against directors. The number of legal filings that are of a detrimental nature that have been filed against the directors.
			435	Number of failed businesses of directors. The number of failed businesses with which the directors have been associated.
			436	Professor. The number of professors.
			437	Seller. The number of sellers.
			438	Skilled worker. The number of skilled workers.
			439	Trademark represented. The number of trademarks represented.
			440	Number of quantitative incentive scheme units. Number of units allocated to a quantitative incentive scheme.
			441	Quantity in manufacturing process. Quantity currently in the manufacturing process.
			442	Number of units in the width of a layer. Number of units which make up the width of a layer.
			443	Number of units in the depth of a layer. Number of units which make up the depth of a layer.
			444	Return to warehouse. A quantity of products sent back to the warehouse.
			445	Return to the manufacturer. A quantity of products sent back from the manufacturer.
			448	Pre-paid invoice annual consumption, estimated. The estimated annual consumption used for a prepayment invoice.
			449	Total quoted quantity. The sum of quoted quantities.
			450	Requests pertaining to entity in last 12 months. Number of requests received in last 12 months pertaining to the entity.
			451	Total inquiry matches. Number of instances which correspond with the inquiry.
			ZZZ	Mutually defined. As agreed by the trading partners.
		*/
		qtyqDiscrete      =  1, // Discrete quantity
		qtyqSplit = 11, // Split quantity
		qtyqDespatch      = 12, // Despatch quantity
		qtyqOrdered       = 21, // Ordered quantity
		qtyqPerPack       = 52, // Quantity per pack
		qtyqNumOfConsumerUInTradedU = 59, // Number of consumer units in the traded unit
		qtyqReturn        = 61, // Return quantity
		qtyqOverShipped   = 121, // Over shipped
		qtyqDeliveryBatch = 164, // Delivery batch
		qtyqAllocated     = 170, // Allocated quantity. Quantity of the referenced item allocated from available stock for delivery.
		qtyqFreeGoods     = 192, // Free goods quantity
		qtyqFreeIncluded  = 193, // Free quantity included
	};
	int    Write_QTY(SXml::WDoc & rDoc, PPID goodsID, int qtyQ, double qtty);
	int    Read_QTY(xmlNode * pFirstNode, TSVector <QValue> & rList);
	enum {
		cntqQuantity       = 1, // Algebraic total of the quantity values in line items in a message
		cntqNumOfLn        = 2, // Number of line items in message
		cntqNumOfLnAndSub  = 3, // Number of line and sub items in message
		cntqNumOfInvcLn    = 4, // Number of invoice lines
	};
	int    Write_CNT(SXml::WDoc & rDoc, int countQ, double value);
	int    Read_CNT(xmlNode * pFirstNode, int * pCountQ, double * pValue);
	enum {
		// Use the codes AAH, AAQ, ABL, ABM when dealing with CSA (customer specific articles).
		priceqAAA = 1, // Calculation net. The price stated is the net price including all allowances and charges and excluding taxes.
			// Allowances and charges may be stated for information purposes only.
		priceqAAE,     // Information price, excluding allowances or charges, including taxes
		priceqAAF,     // Information price, excluding allowances or charges and taxes
		priceqAAH,     // Subject to escalation and price adjustment
		priceqAAQ,     // Firm price
		priceqABL,     // Base price
		priceqABM      // Base price difference
	};
	int    Write_PRI(SXml::WDoc & rDoc, int priceQ, double amount);
	int    Read_PRI(xmlNode * pFirstNode, TSVector <QValue> & rList);
	enum {
		taxqCustomDuty = 5,
		taxqTax        = 7
	};
	enum {
		taxtGST = 1, // GST = Goods and services tax
		taxtIMP,     // IMP = Import tax
		taxtVAT      // VAT = Value added tax
	};
	//
	// Descr: Записывает сегмент определения налога.
	// Note: Пока функция заточена только на НДС.
	//
	int    Write_TAX(SXml::WDoc & rDoc, int taxQ, int taxT, double value);
	int    Read_TAX(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt); // @notimplemented
	int    Write_CUX(SXml::WDoc & rDoc, const CuxValue & rV);
	int    Read_CUX(xmlNode * pFirstNode, CuxValue & rV);
	int    Write_LIN(SXml::WDoc & rDoc, const LinValue & rV);
	int    Read_LIN(xmlNode * pFirstNode, LinValue & rV);
	int    Write_PIA(SXml::WDoc & rDoc, const PiaValue & rV);
	int    Read_PIA(xmlNode * pFirstNode, TSArray <PiaValue> & rL);
	//
	// Descr: IMD Description format code
	//
	enum {
		imdqFreeFormLongDescr = 1, // A = Free-form long description
		imdqCode,                  // C = Code (from industry code list)
		imdqFreeFormShortDescr,    // E = Free-form short description
		imdqFreeForm,              // F = Free-form
		imdqStructured,            // S = Structured (from industry code list)
		imdqCodeAndText            // B = Code and text
	};
	int    Write_IMD(SXml::WDoc & rDoc, int imdq, const char * pDescription);
	int    Read_IMD(xmlNode * pFirstNode, TSCollection <ImdValue> & rL);

	struct BillGoodsItemsTotal {
		BillGoodsItemsTotal() : Count(0), SegCount(0), Quantity(0.0), AmountWoTax(0.0), AmountWithTax(0.0)
		{
		}
		uint   Count; // Количество строк
		uint   SegCount; // Количество сегментов сообщения
		double Quantity;
		double AmountWoTax;
		double AmountWithTax;
	};
	int    Write_DesadvGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal);
	int    Write_OrderGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal);
	int    Write_DESADV(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    Write_ORDERS(xmlTextWriter * pX, const PPBillPacket & rPack);
	int    Write_ORDERRSP(xmlTextWriter * pX, const PPBillPacket & rPack, const PPBillPacket * pExtPack);
	int    Read_Document(PPEdiProcessor::ProviderImplementation * pProvider, void * pCtx, const char * pFileName, const char * pIdent, TSCollection <PPEdiProcessor::Packet> & rList);
private:
	int    PreprocessGoodsOnReading(const PPBillPacket * pPack, const DocumentDetailValue * pItem, PPID * pGoodsID);
	struct PartyResolveBlock {
		PartyResolveBlock()
		{
			THISZERO();
		}
		long   Flags;
		PPID   BillObjID;
		PPID   MainOrgID;
		PPID   AgentID;
		PPID   PayerID;
		PPID   DlvrLocID;
		PPID   BillLocID;
	};
	int    PreprocessPartiesOnReading(int ediOpID, const DocumentValue * pV, PartyResolveBlock * pResult);
	void   SetupPartyAddedMsg(const PartyValue * pVal, SString & rBuf);
	int    Read_CommonDocumentEntries(xmlNode * pFirstNode, DocumentValue & rVal);

	PPEdiProcessor::ProviderImplementation * P_Pi;
};

/*static*/int FASTCALL PPEdiProcessor::GetEdiMsgTypeByText(const char * pSymb)
{
	int    edi_msg_type = PPEanComDocument::GetMsgTypeBySymb(pSymb);
	if(!edi_msg_type) {
		if(sstreqi_ascii(pSymb, "alcrpt") || sstreqi_ascii(pSymb, "alcdes"))
			edi_msg_type = PPEDIOP_ALCODESADV;
		else if(sstreqi_ascii(pSymb, "order"))
			edi_msg_type = PPEDIOP_ORDER;
	}
	return edi_msg_type;
}

PPEdiProcessor::ProviderImplementation::ProviderImplementation(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags) :
	Epp(rEpp), MainOrgID(mainOrgID), Flags(flags), P_BObj(BillObj)
{
	PPAlbatrosCfgMngr::Get(&ACfg);
	Arp.SetConfig(0);
	Arp.Init();
}

PPEdiProcessor::ProviderImplementation::~ProviderImplementation()
{
}

const SString & FASTCALL PPEdiProcessor::ProviderImplementation::EncXmlText(const char * pS)
{
	EncBuf = pS;
	EncBuf.ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(EncBuf, "&<>\'");
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

const SString & FASTCALL PPEdiProcessor::ProviderImplementation::EncXmlText(const SString & rS)
{
	(EncBuf = rS).ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(EncBuf, "&<>\'");
	return EncBuf.Transf(CTRANSF_INNER_TO_UTF8);
}

int PPEdiProcessor::ProviderImplementation::ValidateGLN(const SString & rGLN)
{
	int    ok = 0;
	if(rGLN.NotEmpty()) {
		SNaturalTokenArray nta;
		SNaturalTokenStat nts;
		TR.Run(rGLN.ucptr(), rGLN.Len(), nta, &nts);
		if(nts.Seq & SNTOKSEQ_DEC)
			ok = 1;
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetOriginOrderBill(const PPBillPacket & rBp, BillTbl::Rec * pOrdBillRec)
{
	int    ok = -1;
	PPID   order_bill_id = 0;
	PPIDArray order_list;
	BillTbl::Rec order_bill_rec;
	PPID   any_order_bill_id = 0;
	rBp.GetOrderList(order_list);
	for(uint i = 0; !order_bill_id && i < order_list.getCount(); i++) {
		const PPID temp_order_bill_id = order_list.get(i);
		if(P_BObj->Search(temp_order_bill_id, &order_bill_rec) > 0) {
			if(order_bill_rec.EdiOp == PPEDIOP_ORDER)
				order_bill_id = order_bill_rec.ID;
			else if(!any_order_bill_id)
				any_order_bill_id = temp_order_bill_id;
		}
	}
	if(order_bill_id)
		ok = 1;
	else if(any_order_bill_id) {
		order_bill_id = any_order_bill_id;
		THROW(P_BObj->Search(order_bill_id, &order_bill_rec) > 0);
		ok = 1;
	}
	if(ok > 0) {
		assert(order_bill_id == order_bill_rec.ID);
	}
	else {
		MEMSZERO(order_bill_rec);
	}
	CATCHZOK
	ASSIGN_PTR(pOrdBillRec, order_bill_rec);
	return ok;
}

int PPEdiProcessor::ProviderImplementation::Helper_GetPersonGLN(PPID psnID, SString & rGLN)
{
	int    ok = 0;
	rGLN.Z();
	if(psnID) {
		PsnObj.GetRegNumber(psnID, PPREGT_GLN, rGLN);
	}
	if(ValidateGLN(rGLN)) {
		ok = 1;
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetPersonGLN(PPID psnID, SString & rGLN)
{
	int    ok = Helper_GetPersonGLN(psnID, rGLN);
	if(!ok) {
		SString temp_buf;
		GetPersonName(psnID, temp_buf);
		PPSetError(PPERR_EDI_PSNHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetArticleGLN(PPID arID, SString & rGLN)
{
	PPID   psn_id = ObjectToPerson(arID, 0);
	int    ok = Helper_GetPersonGLN(psn_id, rGLN);
	if(!ok) {
		SString temp_buf;
		GetArticleName(arID, temp_buf);
		PPSetError(PPERR_EDI_ARHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetMainOrgGLN(SString & rGLN)
{
	PPID   psn_id = MainOrgID;
	int    ok = Helper_GetPersonGLN(psn_id, rGLN);
	if(!ok) {
		SString temp_buf;
		GetPersonName(psn_id, temp_buf);
		PPSetError(PPERR_EDI_MAINORGHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetLocGLN(PPID locID, SString & rGLN)
{
	int    ok = 0;
	rGLN.Z();
	if(locID) {
		RegisterTbl::Rec reg_rec;
		if(PsnObj.LocObj.GetRegister(locID, PPREGT_GLN, ZERODATE, 1, &reg_rec) > 0) {
			rGLN = reg_rec.Num;
		}
	}
	if(ValidateGLN(rGLN)) {
		ok = 1;
	}
	else {
		SString temp_buf;
		GetObjectName(PPOBJ_LOCATION, locID, temp_buf);
		PPSetError(PPERR_EDI_LOCHASNTVALUDGLN, temp_buf);
	}
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetGoodsInfo(PPID goodsID, PPID arID, Goods2Tbl::Rec * pRec, SString & rGtin, SString & rArCode)
{
	rGtin.Z();
	rArCode.Z();
	int    ok = 1;
	SString temp_buf;
	uint   non_strict_bc_pos = 0;
	Goods2Tbl::Rec goods_rec;
	BarcodeArray bc_list;
	THROW(GObj.Search(goodsID, &goods_rec) > 0);
	GObj.P_Tbl->ReadBarcodes(goodsID, bc_list);
	// @v10.3.6 Перемещаем предпочтительный код вверх списка дабы проверить его с приоритетом {
	{
		uint   pref_bc_pos = 0;
		const BarcodeTbl::Rec * p_pref_bc_rec = bc_list.GetPreferredItem(&pref_bc_pos);
		if(p_pref_bc_rec) {
			assert(pref_bc_pos < bc_list.getCount()); // @paranoic
			if(pref_bc_pos > 0)
				bc_list.swap(pref_bc_pos, 0);
		}
	}
	// } @v10.3.6
	for(uint bcidx = 0; rGtin.IsEmpty() && bcidx < bc_list.getCount(); bcidx++) {
		int    d = 0;
		int    std = 0;
		const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
		(temp_buf = r_bc_item.Code).Strip();
		while(temp_buf.Last() == '*' || temp_buf.Last() == ' ')
			temp_buf.TrimRight();
		if(GObj.DiagBarcode(temp_buf, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE))
			rGtin = temp_buf;
		else if(!non_strict_bc_pos) {
			if(temp_buf.Len() <= 14)
				non_strict_bc_pos = bcidx+1;
		}
	}
	if(rGtin.IsEmpty()) {
		if(!(ACfg.Hdr.Flags & PPAlbatrosCfgHdr::fStrictExpGtinCheck) && non_strict_bc_pos) {
			(temp_buf = bc_list.at(non_strict_bc_pos-1).Code).Strip();
			while(temp_buf.Last() == '*' || temp_buf.Last() == ' ')
				temp_buf.TrimRight();
			rGtin = temp_buf;
		}
		THROW_PP_S(rGtin.NotEmpty(), PPERR_EDI_WAREHASNTVALIDCODE, goods_rec.Name);
	}
	if(arID) {
		GObj.P_Tbl->GetArCode(arID, goodsID, rArCode, 0);
	}
	CATCHZOK
	ASSIGN_PTR(pRec, goods_rec);
	return ok;
}

int PPEdiProcessor::ProviderImplementation::GetIntermediatePath(const char * pSub, int docType, SString & rBuf)
{
	rBuf.Z();
	PPGetPath(PPPATH_TEMP, rBuf);
	rBuf.SetLastSlash().Cat("EDI");
	if(Epp.Rec.Symb[0])
		rBuf.SetLastSlash().Cat(Epp.Rec.Symb);
	if(!isempty(pSub))
		rBuf.SetLastSlash().Cat(pSub);
	if(docType) {
		SString temp_buf;
		PPGetSubStrById(PPTXT_EDIOP, docType, temp_buf);
		if(temp_buf.NotEmpty())
			rBuf.SetLastSlash().Cat(temp_buf);
	}
	rBuf.SetLastSlash();
	::createDir(rBuf);
	return 1;
}

int PPEdiProcessor::ProviderImplementation::GetTempOutputPath(int docType, SString & rBuf)
	{ return GetIntermediatePath("OUT", docType, rBuf); }
int PPEdiProcessor::ProviderImplementation::GetTempInputPath(int docType, SString & rBuf)
	{ return GetIntermediatePath("IN", docType, rBuf); }

PPEanComDocument::PPEanComDocument(PPEdiProcessor::ProviderImplementation * pPi) : P_Pi(pPi)
{
}

PPEanComDocument::~PPEanComDocument()
{
}

int PPEanComDocument::Write_MessageHeader(SXml::WDoc & rDoc, int msgType, const char * pMsgId)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_unh(rDoc, "UNH"); // Message header
	n_unh.PutInner("E0062", temp_buf.Z().Cat(pMsgId)); // ИД сообщения
	{
		SXml::WNode n_i(rDoc, "S009");
		THROW(GetMsgSymbByType(msgType, temp_buf));
		n_i.PutInner("E0065", temp_buf); // Тип сообщения
		n_i.PutInner("E0052", "D"); // Версия сообщения
		n_i.PutInner("E0054", "01B"); // Версия выпуска
		{
			const char * p_e0057_code = 0;
			if(msgType == PPEDIOP_ORDER)
				p_e0057_code = "EAN010";
			else if(msgType == PPEDIOP_DESADV)
				p_e0057_code = "EAN007";
			if(p_e0057_code) {
				n_i.PutInner("E0051", "UN"); // Код ведущей организации
				n_i.PutInner("E0057", p_e0057_code); // Код, присвоенный ведущей организацией
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_MessageHeader(xmlNode * pFirstNode, SString & rMsgType, SString & rMsgId) // @notimplemented
{
	rMsgType.Z();
	rMsgId.Z();
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E0062", temp_buf)) {
			rMsgId = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		}
		else if(SXml::IsName(p_n, "S009")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E0065", temp_buf)) {
					rMsgType = temp_buf;
				}
			}
		}
	}
	return ok;
}

int PPEanComDocument::Write_BeginningOfMessage(SXml::WDoc & rDoc, const char * pDocCode, const char * pDocIdent, int funcMsgCode)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_bgm(rDoc, "BGM"); // Beginning of message
	{
		{
			SXml::WNode n_i(rDoc, "C002"); // Имя документа/сообщения
			(temp_buf = pDocCode).Transf(CTRANSF_INNER_TO_UTF8);
			n_i.PutInner("E1001", temp_buf); // Код документа
		}
		{
			SXml::WNode n_i(rDoc, "C106"); // Идентификация документа/сообщения
			temp_buf = pDocIdent;
			THROW_PP_S(temp_buf.Len() > 0 && temp_buf.Len() <= 17, PPERR_EDI_DOCIDENTLEN, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
			n_i.PutInner("E1004", temp_buf); // Номер документа (Должно быть максимум 17 символов)
		}
		n_bgm.PutInner("E1225", temp_buf.Z().Cat(funcMsgCode)); // Код функции сообщения
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_BeginningOfMessage(xmlNode * pFirstNode, SString & rDocCode, SString & rDocIdent, int * pFuncMsgCode)
{
	rDocCode.Z();
	rDocIdent.Z();
	int    ok = 1;
	int    func_msg_code = 0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C002")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1001", temp_buf)) {
					rDocCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
		else if(SXml::IsName(p_n, "C106")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1004", temp_buf)) {
					rDocIdent = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
		else if(SXml::GetContentByName(p_n, "E1225", temp_buf)) {
			func_msg_code = temp_buf.ToLong();
		}
	}
	ASSIGN_PTR(pFuncMsgCode, func_msg_code);
	return ok;
}

int PPEanComDocument::Write_UNT(SXml::WDoc & rDoc, const char * pDocCode, uint segCount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_unt(rDoc, "UNT"); // Окончание сообщения
	n_unt.PutInner("E0074", temp_buf.Z().Cat(segCount)); // Общее число сегментов в сообщении
	n_unt.PutInner("E0062", pDocCode); // Номер электронного сообщения (совпадает с указанным в заголовке)
	return ok;
}

int PPEanComDocument::Read_UNT(xmlNode * pFirstNode, SString & rDocCode, uint * pSegCount)
{
	int    ok = 1;
	uint   seg_count = 0;
	rDocCode.Z();
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E0074", temp_buf)) {
			seg_count = temp_buf.ToLong();
		}
		else if(SXml::GetContentByName(p_n, "E0062", temp_buf)) {
			rDocCode = temp_buf;
		}
	}
	ASSIGN_PTR(pSegCount, seg_count);
	return ok;
}

int PPEanComDocument::Write_DTM(SXml::WDoc & rDoc, int dtmKind, int dtmFmt, const LDATETIME & rDtm, const LDATETIME * pFinish)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_dtm(rDoc, "DTM"); // Дата документа
	THROW(oneof3(dtmFmt, dtmfmtCCYYMMDD, dtmfmtCCYYMMDDHHMM, dtmfmtCCYYMMDD_CCYYMMDD));
	THROW(dtmFmt != dtmfmtCCYYMMDD_CCYYMMDD || pFinish);
	{
		SXml::WNode n_i(rDoc, "C507");
		n_i.PutInner("E2005", temp_buf.Z().Cat(dtmKind)); // Квалификатор функции даты-времени (Дата/время документа/сообщения)
		temp_buf.Z();
		if(dtmFmt == dtmfmtCCYYMMDD) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV);
		}
		else if(dtmFmt == dtmfmtCCYYMMDDHHMM) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(rDtm.t, TIMF_HM);
		}
		else if(dtmFmt == dtmfmtCCYYMMDD_CCYYMMDD) {
			temp_buf.Cat(rDtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).CatChar('-').Cat(pFinish->d, DATF_YMD|DATF_CENTURY|DATF_NODIV);
		}
		n_i.PutInner("E2380", temp_buf); // Дата или время, или период
		n_i.PutInner("E2379", temp_buf.Z().Cat(dtmFmt)); // Формат даты/времени (CCYYMMDD)
	}
	CATCHZOK
	return ok;
}

	/*
		2           DDMMYY Calendar date: D = Day; M = Month; Y = Year.
		101         YYMMDD Calendar date: Y = Year; M = Month; D = Day.
		102 !       CCYYMMDD Calendar date: C = Century ; Y = Year ; M = Month ; D = Day.
		104         MMWW-MMWW A period of time specified by giving the start week of a month followed by the end week of a month. Data is to be transmitted as consecutive characters without hyphen.
		107         DDD Day's number within a specific year: D = Day.
		108         WW Week's number within a specific year: W = Week.
		109         MM Month's number within a specific year: M = Month.
		110         DD Day's number within is a specific month.
		201         YYMMDDHHMM 	Calendar date including time without seconds: Y = Year; M = Month; D = Day; H = Hour; M = Minute.
		203  !      CCYYMMDDHHMM 	Calendar date including time with minutes: C=Century; Y=Year; M=Month; D=Day; H=Hour; M=Minutes.
		204         CCYYMMDDHHMMSS 	Calendar date including time with seconds: C=Century;Y=Year; M=Month;D=Day;H=Hour;M=Minute;S=Second.
		401         HHMM 	Time without seconds: H = Hour; m = Minute.
		501         HHMMHHMM 	Time span without seconds: H = Hour; m = Minute;.
		502         HHMMSS-HHMMSS 	Format of period to be given without hyphen.
		602         CCYY 	Calendar year including century: C = Century; Y = Year.
		609         YYMM 	Month within a calendar year: Y = Year; M = Month.
		610         CCYYMM 	Month within a calendar year: CC = Century; Y = Year; M = Month.
		615         YYWW 	Week within a calendar year: Y = Year; W = Week 1st week of January = week 01.
		616  !      CCYYWW 	Week within a calendar year: CC = Century; Y = Year; W = Week (1st week of January = week 01).
		713         YYMMDDHHMM-YYMMDDHHMM 	Format of period to be given in actual message without hyphen.
		715         YYWW-YYWW 	A period of time specified by giving the start week of a year followed by the end week of year (both not including century). Data is to be transmitted as consecutive characters without hyphen.
		717         YYMMDD-YYMMDD 	Format of period to be given in actual message without hyphen.
		718  !      CCYYMMDD-CCYYMMDD 	Format of period to be given without hyphen.
		719         CCYYMMDDHHMM-CCYYMMDDHHMM 	A period of time which includes the century, year, month, day, hour and minute. Format of period to be given in actual message without hyphen.
		720         DHHMM-DHHMM 	Format of period to be given without hyphen (D=day of the week, 1=Monday; 2=Tuesday; ... 7=Sunday).
		801         Year 	To indicate a quantity of years.
		802         Month 	To indicate a quantity of months.
		803         Week 	To indicate a quantity of weeks.
		804         Day 	To indicate a quantity of days.
		805         Hour 	To indicate a quantity of hours.
		806         Minute 	To indicate a quantity of minutes.
		810         Trimester 	To indicate a quantity of trimesters (three months).
		811         Half month 	To indicate a quantity of half months.
		21E         DDHHMM-DDHHMM (GS1 Temporary Code) 	Format of period to be given in actual message without hyphen.
	*/

static int ParseDTM(int fmt, const SString & rBuf, LDATETIME & rDtm, LDATETIME & rDtmFinish)
{
	int    ok = 0;
	SString temp_buf;
	rDtm.Z();
	rDtmFinish.Z();
	switch(fmt) {
		case 102:
			if(rBuf.Len() == 8) {
				ok = strtodate(rBuf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
			}
			break;
		case 203:
			if(rBuf.Len() == 12) {
				(temp_buf = rBuf).Trim(8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
				(temp_buf = rBuf).Excise(7, 4);
				strtotime(temp_buf, TIMF_HM|TIMF_NODIV, &rDtm.t);
				ok = 1;
			}
			break;
		case 616:
			break;
		case 718:
			if(rBuf.Len() == 16) {
				(temp_buf = rBuf).Trim(8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtm.d);
				(temp_buf = rBuf).Excise(7, 8);
				strtodate(temp_buf, DATF_DMY|DATF_CENTURY|DATF_NODIV, &rDtmFinish.d);
				ok = 1;
			}
			break;
	}
	return ok;
}

int PPEanComDocument::Read_DTM(xmlNode * pFirstNode, TSVector <DtmValue> & rList)
{
	int    ok = 1;
	int    dtm_fmt = 0;
	DtmValue dtm_val;
	SString dtm_text;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C507")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E2005", temp_buf)) {
					dtm_val.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E2379", temp_buf)) {
					dtm_fmt = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E2380", temp_buf)) {
					dtm_text = temp_buf;
				}
			}
		}
	}
	ParseDTM(dtm_fmt, dtm_text, dtm_val.Dtm, dtm_val.DtmFinish);
	if(dtm_val.Q) {
		rList.insert(&dtm_val);
	}
	else
		ok = 0;
	return ok;
}

int PPEanComDocument::Write_RFF(SXml::WDoc & rDoc, int refQ, const char * pRef) // reference
{
	int    ok = 1;
	SString temp_buf;
	{
		SXml::WNode n_rff(rDoc, "RFF");
		{
			SXml::WNode n_c506(rDoc, "C506");
			THROW(GetRefqSymb(refQ, temp_buf));
			n_c506.PutInner("E1153", temp_buf);
			(temp_buf = pRef).Transf(CTRANSF_INNER_TO_UTF8);
			n_c506.PutInner("E1154", temp_buf);
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_RFF(xmlNode * pFirstNode, TSCollection <RefValue> & rList) // reference
{
	int    ok = 1;
	RefValue * p_new_item = 0;
	int    ref_q = 0;
	SString ref_q_text;
	SString ref_buf;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C506")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E1153", temp_buf)) {
					ref_q_text = temp_buf;
					ref_q = GetRefqBySymb(temp_buf);
				}
				else if(SXml::GetContentByName(p_n2, "E1154", temp_buf))
					ref_buf = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
	}
	THROW_PP(ref_q, PPERR_EANCOM_RFFWOQ);
	THROW_SL(p_new_item = rList.CreateNewItem());
	p_new_item->Q = ref_q;
	p_new_item->Ref = ref_buf;
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_NAD(SXml::WDoc & rDoc, int partyQ, const char * pGLN)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_nad(rDoc, "NAD"); // Наименование и адрес
	THROW_PP(!isempty(pGLN), PPERR_EDI_GLNISEMPTY);
	{
		THROW_PP_S(GetPartyqSymb(partyQ, temp_buf), PPERR_EDI_INVPARTYQ, (long)partyQ);
		n_nad.PutInner("E3035", temp_buf);
		{
			SXml::WNode n_i(rDoc, "C082"); // Детали стороны
			n_i.PutInner("E3039", pGLN); // GLN стороны
			n_i.PutInner("E3055", "9"); // Код ведущей организации - EAN (Международная ассоциация товарной нумерации)
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_NAD(xmlNode * pFirstNode, PartyValue & rV)
{
	int    ok = 1;
	int    party_q = 0;
	SString temp_buf;
	SString ident_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E3035", temp_buf)) { // Party function code qualifier
			party_q = GetPartyqBySymb(temp_buf);
		}
		else if(SXml::IsName(p_n, "C082")) { // PARTY IDENTIFICATION DETAILS
			int    agency_code = 0;
			ident_buf.Z();
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E3039", temp_buf)) { // Party identifier
					ident_buf = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_n2, "E3055", temp_buf)) { // Code list responsible agency code (9 = GS1)
					agency_code = temp_buf.ToLong();
				}
			}
			rV.Code = ident_buf;
		}
		else if(SXml::IsName(p_n, "C080")) { // PARTY NAME
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E3036", temp_buf)) {
					rV.Name.Cat(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				}
			}
		}
		else if(SXml::IsName(p_n, "C058")) { // NAME AND ADDRESS
		}
		else if(SXml::IsName(p_n, "C059")) { // STREET
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				// Текст адреса может быть разбит на несколько порций
				if(SXml::GetContentByName(p_n2, "E3042", temp_buf)) {
					rV.Addr.Cat(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				}
			}
		}
		else if(SXml::GetContentByName(p_n, "E3251", temp_buf)) { // Postal identification code
			rV.ZIP = temp_buf;
		}
		else if(SXml::GetContentByName(p_n, "E3207", temp_buf)) { // Country name code
		}
		else if(SXml::GetContentByName(p_n, "E3164", temp_buf)) { // City name
			rV.City = temp_buf;
		}
		else if(SXml::IsName(p_n, "C819")) { // COUNTRY SUB-ENTITY DETAILS
		}
	}
	THROW_PP(party_q, PPERR_EANCOM_NADWOQ);
	rV.PartyQ = party_q;
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_CUX(SXml::WDoc & rDoc, const char * pCurrencyCode3)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_cux(rDoc, "CUX");
	THROW_PP(!isempty(pCurrencyCode3), PPERR_EDI_CURRCODEISEMPTY);
	{
		SXml::WNode n_i(rDoc, "C504");
		n_i.PutInner("E6347", "2");
		n_i.PutInner("E6345", pCurrencyCode3);
		n_i.PutInner("E3055", "9");
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_CUX(xmlNode * pFirstNode, SString & rCurrencyCode3)
{
	int    ok = 0;
	return ok;
}

int PPEanComDocument::Write_MOA(SXml::WDoc & rDoc, int amtQ, double amount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_moa(rDoc, "MOA");
	{
		SXml::WNode n_i(rDoc, "C516");
		n_i.PutInner("E5025", temp_buf.Z().Cat(amtQ)); // Квалификатор суммы товарной позиции
		n_i.PutInner("E5004", temp_buf.Z().Cat(amount, MKSFMTD(0, 2, 0))); // Сумма (Число знаков после запятой - не больше 2)
	}
	return ok;
}

int PPEanComDocument::Read_MOA(xmlNode * pFirstNode, TSVector <QValue> & rList)
{
	int    ok = 1;
	SString temp_buf;
	SString moa_text;
	QValue qv;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C516")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E5025", temp_buf)) {
					moa_text = temp_buf;
					if(temp_buf.IsEqiAscii("XB5"))
						qv.Q = amtqExt_XB5;
					else
						qv.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E5004", temp_buf)) {
					qv.Value = temp_buf.ToReal();
				}
			}
		}
	}
	if(!moa_text.NotEmpty()) {
		ok = -1;
	}
	else {
		THROW_PP(qv.Q, PPERR_EANCOM_MOAWOQ);
		THROW_SL(rList.insert(&qv));
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_QTY(SXml::WDoc & rDoc, PPID goodsID, int qtyQ, double qtty)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_qty(rDoc, "QTY");
	{
		SXml::WNode n_i(rDoc, "C186");
		n_i.PutInner("E6063", temp_buf.Z().Cat(qtyQ)); // Квалификатор количества
		SString unit_buf("PCE");
		double unit_scale = 1.0;
		Goods2Tbl::Rec goods_rec;
		if(goodsID && P_Pi->GObj.Fetch(goodsID, &goods_rec) > 0) {
			PPUnit u_rec;
			if(P_Pi->GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
				if(u_rec.ID == PPUNT_KILOGRAM)
					unit_buf = "KGM";
				else if(u_rec.BaseUnitID == PPUNT_KILOGRAM && u_rec.BaseRatio > 0.0) {
					unit_buf = "KGM";
					unit_scale = u_rec.BaseRatio;
				}
			}
		}
		n_i.PutInner("E6060", temp_buf.Z().Cat(qtty * unit_scale, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
		n_i.PutInner("E6411", unit_buf);
	}
	return ok;
}

int PPEanComDocument::Read_QTY(xmlNode * pFirstNode, TSVector <QValue> & rList)
{
	int    ok = 1;
	SString temp_buf;
	QValue qv;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C186")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E6063", temp_buf)) {
					qv.Q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6060", temp_buf)) {
					qv.Value = temp_buf.ToReal();
				}
				else if(SXml::GetContentByName(p_n2, "E6411", temp_buf)) {
					if(temp_buf.IsEqiAscii("KGM"))
						qv.Unit = UNIT_KILOGRAM;
					else if(temp_buf.IsEqiAscii("PCE"))
						qv.Unit = UNIT_ITEM;
				}
			}
		}
	}
	THROW_PP(qv.Q, PPERR_EANCOM_QTYWOQ);
	THROW_SL(rList.insert(&qv));
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_CNT(SXml::WDoc & rDoc, int countQ, double value)
{
	int    ok = 1;
	SXml::WNode n_cnt(rDoc, "CNT");
	{
		SString temp_buf;
		SXml::WNode n_c270(rDoc, "C270");
		n_c270.PutInner("E6069", temp_buf.Z().Cat(countQ));
		n_c270.PutInner("E6066", temp_buf.Z().Cat(value, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
	}
	return ok;
}

int PPEanComDocument::Read_CNT(xmlNode * pFirstNode, int * pCountQ, double * pValue)
{
	int    ok = 1;
	int    count_q = 0;
	double value = 0.0;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C270")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E6069", temp_buf)) {
					count_q = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6066", temp_buf)) {
					value = temp_buf.ToReal();
				}
			}
		}
	}
	ASSIGN_PTR(pCountQ, count_q);
	ASSIGN_PTR(pValue, value);
	return ok;
}

int PPEanComDocument::Write_PRI(SXml::WDoc & rDoc, int priceQ, double amount)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_pri(rDoc, "PRI");
	{
		SXml::WNode n_i(rDoc, "C509");
		THROW_PP_S(GetPriceqSymb(priceQ, temp_buf), PPERR_EDI_INVPRICEQ, (long)priceQ);
		n_i.PutInner("E5125", temp_buf); // Квалификатор цены
		n_i.PutInner("E5118", temp_buf.Z().Cat(amount, MKSFMTD(0, 2, 0)));
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_PRI(xmlNode * pFirstNode, TSVector <QValue> & rList)
{
	int    ok = 1;
	//int    price_q = 0;
	//double value = 0.0;
	QValue qv;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C509")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E5125", temp_buf)) {
					qv.Q = GetPriceqBySymb(temp_buf);
				}
				else if(SXml::GetContentByName(p_n2, "E5118", temp_buf)) {
					qv.Value = temp_buf.ToReal();
				}
			}
		}		
	}
	if(qv.Q != 0 || qv.Value != 0.0) {
		rList.insert(&qv);
	}
	//ASSIGN_PTR(pPriceQ, price_q);
	//ASSIGN_PTR(pAmt, value);
	return ok;
}

int PPEanComDocument::Write_TAX(SXml::WDoc & rDoc, int taxQ, int taxT, double value)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_tax(rDoc, "TAX");
	THROW_PP_S(oneof2(taxQ, taxqCustomDuty, taxqTax), PPERR_EDI_INVTAXQ, (long)taxQ);
	n_tax.PutInner("E5283", temp_buf.Z().Cat(taxQ)); // Квалификатор налога
	{
		// Тип налога
		switch(taxT) {
			case taxtGST: temp_buf = "GST"; break;
			case taxtIMP: temp_buf = "IMP"; break;
			case taxtVAT: temp_buf = "VAT"; break;
			default:
				CALLEXCEPT_PP_S(PPERR_EDI_INVTAXTYPE, (long)taxT);
		}
		SXml::WNode n_c241(rDoc, "C241");
		n_c241.PutInner("E5153", temp_buf);
	}
	{
		SXml::WNode n_c243(rDoc, "C243");
		n_c243.PutInner("E5278", temp_buf.Z().Cat(value)); // Ставка
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_TAX(SXml::WDoc & rDoc, int * pPriceQ, double * pAmt) // @notimplemented
{
	int    ok = 0;
	return ok;
}

int PPEanComDocument::Write_CUX(SXml::WDoc & rDoc, const CuxValue & rV)
{
	int    ok = 0;
	return ok;
}

int PPEanComDocument::Read_CUX(xmlNode * pFirstNode, CuxValue & rV)
{
	rV.Z();
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "C504")) {
			int   is_srv = 0;
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E6343", temp_buf)) {
					rV.TypeQ = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6345", temp_buf)) {
					STRNSCPY(rV.Ident, temp_buf);
				}
				else if(SXml::GetContentByName(p_n2, "E6347", temp_buf)) {
					rV.UsageQ = temp_buf.ToLong();
				}
				else if(SXml::GetContentByName(p_n2, "E6348", temp_buf)) {
					rV.Rate = temp_buf.ToReal();
				}
			}
		}
	}
	return ok;
}

int PPEanComDocument::Write_LIN(SXml::WDoc & rDoc, /*int lineN, const char * pGoodsCode*/const LinValue & rV)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_lin(rDoc, "LIN");
	n_lin.PutInner("E1082", temp_buf.Z().Cat(rV.LineN));
	{
		SXml::WNode n_c212(rDoc, "C212");
		n_c212.PutInner("E7140", rV.GoodsCode); // Штрих-код товара
		n_c212.PutInner("E7143", "SRV"); // Тип штрихкода EAN.UCC
	}
	return ok;
}

int PPEanComDocument::Read_LIN(xmlNode * pFirstNode, /*int * pLineN, SString & rGoodsCode*/LinValue & rV)
{
	rV.GoodsCode.Z();
	rV.Action = 0;
	rV.LineN = 0;
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E1082", temp_buf))
			rV.LineN = temp_buf.ToLong();
		else if(SXml::GetContentByName(p_n, "E1229", temp_buf)) {
			rV.Action = temp_buf.ToLong();
		}
		else if(SXml::IsName(p_n, "C212")) {
			int   is_srv = 0;
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E7140", temp_buf)) {
					rV.GoodsCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
				else if(SXml::GetContentByName(p_n2, "E7143", temp_buf)) {
					if(temp_buf.IsEqiAscii("SRV"))
						is_srv = 1;
				}
			}
			if(!is_srv) {
				rV.GoodsCode.Z();
			}
		}
	}
	return ok;
}

int PPEanComDocument::Write_PIA(SXml::WDoc & rDoc, const PiaValue & rV)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WNode n_pia(rDoc, "PIA"); // Дополнительный идентификатор товара
	THROW(rV.Code[0]);
	THROW(oneof4(rV.Q, piaqAdditionalIdent, piaqSubstitutedBy, piaqSubstitutedFor, piaqProductIdent));
	n_pia.PutInner("E4347", temp_buf.Z().Cat(rV.Q)); // Дополнительный идентификатор
	{
		SXml::WNode n_c212(rDoc, "C212");
		(temp_buf = rV.Code).Transf(CTRANSF_INNER_TO_UTF8);
		n_c212.PutInner("E7140", temp_buf); // Артикул
		GetIticSymb(rV.Itic, temp_buf);
		n_c212.PutInner("E7143", temp_buf);
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_PIA(xmlNode * pFirstNode, TSArray <PiaValue> & rL)
{
	int    ok = 1;
	SString temp_buf;
	PiaValue value;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E4347", temp_buf)) {
			value.Q = temp_buf.ToLong();
		}
		else if(SXml::IsName(p_n, "C212")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E7140", temp_buf)) {
					value.Itic = GetIticBySymb(temp_buf);
				}
				else if(SXml::GetContentByName(p_n2, "E7143", temp_buf)) {
					STRNSCPY(value.Code, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				}
			}
		}
	}
	THROW_PP(value.Q, PPERR_EANCOM_PIAWOQ);
	THROW_PP(value.Code[0], PPERR_EANCOM_PIAWOCODE);
	THROW_SL(rL.insert(&value));
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_IMD(SXml::WDoc & rDoc, int imdQ, const char * pDescription)
{
	int    ok = 1;
	if(!isempty(pDescription)) {
		SString temp_buf;
		SXml::WNode n_imd(rDoc, "IMD"); // Описание товара
		switch(imdQ) {
			case imdqFreeFormLongDescr: temp_buf = "A"; break;
			case imdqCode: temp_buf = "C"; break;
			case imdqFreeFormShortDescr: temp_buf = "E"; break;
			case imdqFreeForm:    temp_buf = "F"; break;
			case imdqStructured:  temp_buf = "S"; break;
			case imdqCodeAndText: temp_buf = "B"; break;
			default: temp_buf.Z(); break;
		}
		if(temp_buf.NotEmpty()) {
			n_imd.PutInner("E7077", temp_buf); // Код формата описания (текст)
			{
				SXml::WNode n_c273(rDoc, "C273"); // Описание
				n_c273.PutInner("E7008", SXml::WNode::CDATA((temp_buf = pDescription).Transf(CTRANSF_INNER_TO_UTF8)));
			}
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int PPEanComDocument::Read_IMD(xmlNode * pFirstNode, TSCollection <ImdValue> & rL)
{
	int    ok = 1;
	int    imd_q = 0;
	SString imd_text;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; ok > 0 && p_n; p_n = p_n->next) {
		if(SXml::GetContentByName(p_n, "E7077", temp_buf)) {
			if(temp_buf.Len() == 1) {
				switch(temp_buf.C(0)) {
					case 'A': imd_q = imdqFreeFormLongDescr; break;
					case 'C': imd_q = imdqCode; break;
					case 'E': imd_q = imdqFreeFormShortDescr; break;
					case 'F': imd_q = imdqFreeForm; break;
					case 'S': imd_q = imdqStructured; break;
					case 'B': imd_q = imdqCodeAndText; break;
				}
			}
		}
		else if(SXml::IsName(p_n, "C273")) {
			for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
				if(SXml::GetContentByName(p_n2, "E7008", temp_buf)) {
					imd_text = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				}
			}
		}
	}
	THROW_PP(imd_q, PPERR_EANCOM_IMDWOQ);
	{
		ImdValue * p_new_item = rL.CreateNewItem();
		THROW_SL(p_new_item);
		p_new_item->Q = imd_q;
		p_new_item->Text = imd_text;
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_DesadvGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal)
{
	int    ok = 1;
	const  double qtty = fabs(rTi.Qtty());
	SString temp_buf;
	SString goods_code;
	SString goods_ar_code;
	Goods2Tbl::Rec goods_rec;
	THROW(qtty > 0.0); // @todo error (бессмысленная строка с нулевым количеством, но пока не ясно, что с ней делать - возможно лучше просто пропустить с замечанием).
	THROW(P_Pi->GetGoodsInfo(rTi.GoodsID, 0, &goods_rec, goods_code, goods_ar_code));
	{
		LinValue lv;
		lv.Action = 0;
		lv.LineN = rTi.RByBill;
		lv.GoodsCode = goods_code;
		THROW(Write_LIN(rDoc, lv));
		rTotal.SegCount++;
		if(goods_ar_code.NotEmpty()) {
			PiaValue pia;
			pia.Q = piaqAdditionalIdent;
			pia.Itic = iticIN;
			STRNSCPY(pia.Code, goods_ar_code);
			THROW(Write_PIA(rDoc, pia));
			rTotal.SegCount++;
		}
		THROW(Write_IMD(rDoc, imdqFreeForm, goods_rec.Name));
		rTotal.SegCount++;
		THROW(Write_QTY(rDoc, rTi.GoodsID, 21, qtty));
		rTotal.SegCount++;
		rTotal.Quantity += qtty;
		{
			GTaxVect vect;
			vect.CalcTI(rTi, 0 /*opID*/, tiamt);
			const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
			const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
			// @v10.3.0 (never used) const double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
			const double price_with_vat = R5(amount_with_vat / qtty);
			const double price_without_vat = R5(amount_without_vat / qtty);
			rTotal.AmountWithTax += amount_with_vat;
			rTotal.AmountWoTax += amount_without_vat;
			THROW(Write_MOA(rDoc, amtqUnitPrice/*146*/, price_without_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqTotalLnItemsAmt/*79*/, amount_with_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqLnItemAmt/*203*/, amount_without_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqTaxAmt/*124*/, amount_with_vat - amount_without_vat));
			rTotal.SegCount++;
		}
	}
	rTotal.Count++;
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_OrderGoodsItem(SXml::WDoc & rDoc, int ediOp, const PPTransferItem & rTi, int tiamt, BillGoodsItemsTotal & rTotal)
{
	int    ok = 1;
	const  double qtty = fabs(rTi.Qtty());
	SString temp_buf;
	SString goods_code;
	SString goods_ar_code;
	Goods2Tbl::Rec goods_rec;
	BarcodeArray bc_list;
	THROW(qtty > 0.0); // @todo error (бессмысленная строка с нулевым количеством, но пока не ясно, что с ней делать - возможно лучше просто пропустить с замечанием).
	THROW(P_Pi->GObj.Search(rTi.GoodsID, &goods_rec) > 0);
	P_Pi->GObj.P_Tbl->ReadBarcodes(rTi.GoodsID, bc_list);
	for(uint bcidx = 0; goods_code.IsEmpty() && bcidx < bc_list.getCount(); bcidx++) {
		int    d = 0;
		int    std = 0;
		const  BarcodeTbl::Rec & r_bc_item = bc_list.at(bcidx);
		if(P_Pi->GObj.DiagBarcode(r_bc_item.Code, &d, &std, 0) > 0 && oneof4(std, BARCSTD_EAN8, BARCSTD_EAN13, BARCSTD_UPCA, BARCSTD_UPCE)) {
			goods_code = r_bc_item.Code;
		}
	}
	THROW_PP_S(goods_code.NotEmpty(), PPERR_EDI_WAREHASNTVALIDCODE, goods_rec.Name);
	{
		LinValue lv;
		lv.Action = 0;
		lv.LineN = rTi.RByBill;
		lv.GoodsCode = goods_code;
		THROW(Write_LIN(rDoc, lv));
		rTotal.SegCount++;
		if(goods_ar_code.NotEmpty()) {
			PiaValue pia;
			pia.Q = piaqAdditionalIdent;
			pia.Itic = iticSA;
			STRNSCPY(pia.Code, goods_ar_code);
			THROW(Write_PIA(rDoc, pia));
			rTotal.SegCount++;
		}
		THROW(Write_IMD(rDoc, imdqFreeForm, goods_rec.Name));
		rTotal.SegCount++;
		THROW(Write_QTY(rDoc, rTi.GoodsID, 21, qtty));
		rTotal.SegCount++;
		{
			GTaxVect vect;
			vect.CalcTI(rTi, 0 /*opID*/, tiamt);
			const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
			const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
			const double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
			const double price_with_vat = R5(amount_with_vat / qtty);
			const double price_without_vat = R5(amount_without_vat / qtty);
			rTotal.AmountWithTax += amount_with_vat;
			rTotal.AmountWoTax += amount_without_vat;
			THROW(Write_MOA(rDoc, amtqTotalLnItemsAmt, amount_with_vat));
			rTotal.SegCount++;
			THROW(Write_MOA(rDoc, amtqLnItemAmt, amount_without_vat));
			rTotal.SegCount++;
			{
				SXml::WNode n_sg32(rDoc, "SG32"); // Цена товара с НДС
				THROW(Write_PRI(rDoc, priceqAAE, price_with_vat));
				rTotal.SegCount++;
			}
			{
				SXml::WNode n_sg32(rDoc, "SG32"); // Цена товара без НДС
				THROW(Write_PRI(rDoc, priceqAAA, price_without_vat));
				rTotal.SegCount++;
			}
			{
				SXml::WNode n_sg38(rDoc, "SG38"); // Ставка НДС
				THROW(Write_TAX(rDoc, taxqTax, taxtVAT, vat_rate));
				rTotal.SegCount++;
			}
		}
	}
	rTotal.Count++;
	CATCHZOK
	return ok;
}

int PPEanComDocument::PreprocessGoodsOnReading(const PPBillPacket * pPack, const DocumentDetailValue * pItem, PPID * pGoodsID)
{
	int    ok = 1;
	PPID   goods_id = 0;
	SString temp_buf;
	SString addendum_msg_buf;
	BarcodeTbl::Rec bc_rec;
	Goods2Tbl::Rec goods_rec;
	if(pItem->LinV.GoodsCode.NotEmpty() && P_Pi->GObj.SearchByBarcode(pItem->LinV.GoodsCode, &bc_rec, &goods_rec) > 0) {
		goods_id = goods_rec.ID;
	}
	else {
		if(pItem->LinV.GoodsCode.NotEmpty())
			addendum_msg_buf.CatDivIfNotEmpty('/', 1).Cat(pItem->LinV.GoodsCode);
		for(uint j = 0; j < pItem->PiaL.getCount(); j++) {
			const PiaValue & r_pia = pItem->PiaL.at(j);
			if(oneof2(r_pia.Q, piaqAdditionalIdent, piaqProductIdent) && r_pia.Code[0]) {
				if(r_pia.Itic == iticSA) {
					if(P_Pi->GObj.P_Tbl->SearchByArCode(pPack->Rec.Object, r_pia.Code, 0, &goods_rec) > 0)
						goods_id = goods_rec.ID;
					else
						addendum_msg_buf.CatDivIfNotEmpty('/', 1).Cat(r_pia.Code);
				}
				else if(r_pia.Itic == iticSRV) {
					if(P_Pi->GObj.SearchByBarcode(r_pia.Code, &bc_rec, &goods_rec) > 0)
						goods_id = goods_rec.ID;
					else
						addendum_msg_buf.CatDivIfNotEmpty('/', 1).Cat(r_pia.Code);
				}
				else if(r_pia.Itic == iticIN) {
					PPID   temp_id = (temp_buf = r_pia.Code).ToLong();
					if(temp_id && P_Pi->GObj.Search(temp_id, &goods_rec) > 0)
						goods_id = goods_rec.ID;
					else
						addendum_msg_buf.CatDivIfNotEmpty('/', 1).Cat(r_pia.Code);
				}
			}
		}
	}
	if(!goods_id) {
		if(pItem->ImdL.getCount()) {
			for(uint j = 0; j < pItem->ImdL.getCount(); j++) {
				temp_buf = pItem->ImdL.at(j)->Text;
				if(temp_buf.NotEmptyS()) {
					addendum_msg_buf.CatDivIfNotEmpty('/', 1).Cat(temp_buf);
					break;
				}
			}
		}
		addendum_msg_buf.CatDivIfNotEmpty(':', 1).Cat(pPack->Rec.Code).CatDiv('-', 1).Cat(pPack->Rec.Dt, DATF_DMY);
		CALLEXCEPT_PP_S(PPERR_EDI_UNBLRSLV_GOODS, addendum_msg_buf);
	}
	CATCHZOK
	ASSIGN_PTR(pGoodsID, goods_id);
	return ok;
}

void PPEanComDocument::SetupPartyAddedMsg(const PartyValue * pVal, SString & rBuf)
{
	rBuf.CatDivIfNotEmpty('/', 1).Cat(pVal->Code);
	if(pVal->Name.NotEmpty())
		rBuf.CatChar('[').Cat(pVal->Name).CatChar(']');
}

int PPEanComDocument::PreprocessPartiesOnReading(int ediOpID, const DocumentValue * pV, PartyResolveBlock * pResult)
{
	int    ok = 1;
	const PPID reg_type_id = PPREGT_GLN;
	SString temp_buf;
	PPIDArray psn_list;
	PPIDArray ar_list;
	PPIDArray loc_list;
	PPID   consignor_ar_id = 0;
	PPID   consignee_psn_id = 0;
	PPID   consignee_loc_id = 0;
	SString addendum_msg_cli;
	SString addendum_msg_main;
	SString addendum_msg_wh;
	if(ediOpID == PPEDIOP_DESADV) {
		for(uint i = 0; i < pV->PartyL.getCount(); i++) {
			const PartyValue * p_val = pV->PartyL.at(i);
			if(p_val && p_val->Code.NotEmpty()) {
				PPID   ar_id = 0;
				switch(p_val->PartyQ) {
					case EDIPARTYQ_SELLER:
						SetupPartyAddedMsg(p_val, addendum_msg_cli);
						if(P_Pi->PsnObj.ResolveGLN_Article(p_val->Code, GetSupplAccSheet(), &ar_id) > 0)
							pResult->BillObjID = ar_id;
						break;
					case EDIPARTYQ_BUYER:
						{
							SetupPartyAddedMsg(p_val, addendum_msg_main);
							THROW(P_Pi->GetMainOrgGLN(temp_buf));
							//THROW_PP_S(temp_buf == p_val->Code, PPERR_EDI_DESADVBYNEQMAINORG, p_val->Code);
							if(temp_buf == p_val->Code) {
								pResult->MainOrgID = P_Pi->MainOrgID;
							}
							else {
								THROW(P_Pi->PsnObj.GetListByRegNumber(reg_type_id, PPPRK_MAIN, p_val->Code, psn_list.Z()));
								if(psn_list.getCount())
									pResult->MainOrgID = psn_list.get(0);
							}
						}
						break;
					case EDIPARTYQ_CONSIGNOR:
						SetupPartyAddedMsg(p_val, addendum_msg_cli);
						if(P_Pi->PsnObj.ResolveGLN_Article(p_val->Code, GetSupplAccSheet(), &ar_id) > 0)
							consignor_ar_id = ar_id;
						break;
					case EDIPARTYQ_CONSIGNEE:
						SetupPartyAddedMsg(p_val, addendum_msg_wh);
						if(P_Pi->PsnObj.LocObj.ResolveGLN(LOCTYP_WAREHOUSE, p_val->Code, loc_list.Z()) > 0) {
							assert(loc_list.getCount());
							consignee_loc_id = loc_list.get(0);
						}
						else if(P_Pi->PsnObj.LocObj.P_Tbl->GetListByCode(LOCTYP_WAREHOUSE, p_val->Code, &loc_list.Z()) > 0) {
							assert(loc_list.getCount());
							consignee_loc_id = loc_list.get(0);
						}
						break;
					case EDIPARTYQ_DELIVERY: // Точка доставки
						SetupPartyAddedMsg(p_val, addendum_msg_wh);
						if(P_Pi->PsnObj.LocObj.ResolveGLN(LOCTYP_WAREHOUSE, p_val->Code, loc_list.Z()) > 0) {
							assert(loc_list.getCount());
							pResult->BillLocID = loc_list.get(0);
						}
						else if(P_Pi->PsnObj.LocObj.P_Tbl->GetListByCode(LOCTYP_WAREHOUSE, p_val->Code, &loc_list.Z()) > 0) {
							assert(loc_list.getCount());
							pResult->BillLocID = loc_list.get(0);
						}
						break;
				}
			}
		}
		if(!pResult->BillObjID) {
			pResult->BillObjID = consignor_ar_id;
			if(addendum_msg_cli.NotEmpty())
				temp_buf = addendum_msg_cli;
			else
				temp_buf = "*";
			temp_buf.CatDiv('-', 1).Cat(pV->GetFinalBillCode()).CatDiv('-', 1).Cat(pV->GetBillDate(), DATF_DMY);
			THROW_PP_S(pResult->BillObjID, PPERR_EDI_UNBLRSLV_BILLOBJ, temp_buf);
		}
		else if(consignor_ar_id && consignor_ar_id != pResult->BillObjID) {
			// @todo message (Контрагент не тот же, что и грузоотправитель. В общем случае это - нормально,
			// но возможны и проблемы).
		}
		if(!pResult->BillLocID) {
			pResult->BillLocID = consignee_loc_id;
			if(addendum_msg_cli.NotEmpty())
				temp_buf = addendum_msg_wh;
			else
				temp_buf = "*";
			temp_buf.CatDiv('-', 1).Cat(pV->GetFinalBillCode()).CatDiv('-', 1).Cat(pV->GetBillDate(), DATF_DMY);
			THROW_PP_S(pResult->BillLocID, PPERR_EDI_UNBLRSLV_BILLWH, temp_buf);
		}
		{
			if(addendum_msg_cli.NotEmpty())
				temp_buf = addendum_msg_main;
			else
				temp_buf = "*";
			temp_buf.CatDiv('-', 1).Cat(pV->GetFinalBillCode()).CatDiv('-', 1).Cat(pV->GetBillDate(), DATF_DMY);
			THROW_PP_S(pResult->MainOrgID, PPERR_EDI_UNBLRSLV_BILLMAINORG, temp_buf);
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_CommonDocumentEntries(xmlNode * pFirstNode, DocumentValue & rVal)
{
	int    ok = 1;
	SString temp_buf;
	for(xmlNode * p_n = pFirstNode; p_n; p_n = p_n->next) {
		if(SXml::IsName(p_n, "UNH")) {
			THROW(Read_MessageHeader(p_n->children, temp_buf, rVal.UnhIdent));
		}
		else if(SXml::IsName(p_n, "BGM")) {
			THROW(Read_BeginningOfMessage(p_n->children, temp_buf, rVal.BgmIdent, &rVal.FuncMsgCode));
		}
		else if(SXml::IsName(p_n, "DTM")) {
			THROW(Read_DTM(p_n->children, rVal.DtmL));
		}
		else if(SXml::IsName(p_n, "MOA")) {
			THROW(Read_MOA(p_n->children, rVal.MoaL));
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Read_Document(PPEdiProcessor::ProviderImplementation * pProvider, void * pCtx, const char * pFileName, const char * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	assert(pProvider);
	int    ok = -1;
	SString temp_buf;
	SString addendum_msg_buf;
	xmlParserCtxt * p_ctx = static_cast<xmlParserCtxt *>(pCtx);
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	PPEdiProcessor::Packet * p_pack = 0;
	PPBillPacket * p_bpack = 0; // is owned by p_pack
	TSVector <DtmValue> dtm_temp_list;
	DocumentValue document;
	PPObjOprKind op_obj;
	THROW(pProvider);
	THROW_SL(fileExists(pFileName));
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "DESADV")) {
		THROW_PP(P_Pi->ACfg.Hdr.EdiDesadvOpID, PPERR_EDI_OPNDEF_DESADV);
		THROW(Read_CommonDocumentEntries(p_root->children, document));
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "SG1")) {
				dtm_temp_list.clear();
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, "RFF")) {
						THROW(Read_RFF(p_n2->children, document.RefL));
					}
					else if(SXml::IsName(p_n, "DTM")) {
						THROW(Read_DTM(p_n2->children, dtm_temp_list));
					}
				}
			}
			else if(SXml::IsName(p_n, "SG2")) {
				PartyValue local_party;
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, "NAD")) {
						THROW(Read_NAD(p_n2->children, local_party));
					}
					else if(SXml::IsName(p_n2, "LOC")) {
					}
					else if(SXml::IsName(p_n2, "SG3")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "RFF")) {
								THROW(Read_RFF(p_n3->children, local_party.RefL));
							}
						}
					}
					else if(SXml::IsName(p_n2, "SG4")) {
					}
				}
				if(local_party.PartyQ) {
					PartyValue * p_new_party = document.PartyL.CreateNewItem();
					THROW_SL(p_new_party);
					*p_new_party = local_party;
				}
			}
			else if(SXml::IsName(p_n, "SG10")) {
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, "CPS")) {
					}
					else if(SXml::IsName(p_n2, "FTX")) {
					}
					else if(SXml::IsName(p_n2, "SG11")) { //PAC-MEA-QTY-SG12-SG13
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "PAC")) {
							}
							else if(SXml::IsName(p_n3, "MEA")) {
							}
							else if(SXml::IsName(p_n3, "QTY")) {
								// This segment is used to specify the quantity per package specified in the PAC segment.
							}
							else if(SXml::IsName(p_n3, "SG12")) {
							}
							else if(SXml::IsName(p_n3, "SG13")) {
							}
						}
					}
					else if(SXml::IsName(p_n2, "SG17")) { // LIN-PIA-IMD-MEA-QTY-ALI-DLM-DTM-FTX-MOA-SG18-SG20-SG22-SG25
						DocumentDetailValue * p_new_detail_item = document.DetailL.CreateNewItem();
						THROW_SL(p_new_detail_item);
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "LIN")) {
								THROW(Read_LIN(p_n3->children, p_new_detail_item->LinV));
							}
							else if(SXml::IsName(p_n3, "PIA")) {
								THROW(Read_PIA(p_n3->children, p_new_detail_item->PiaL));
							}
							else if(SXml::IsName(p_n3, "IMD")) {
								THROW(Read_IMD(p_n3->children, p_new_detail_item->ImdL));
							}
							else if(SXml::IsName(p_n3, "QTY")) {
								THROW(Read_QTY(p_n3->children, p_new_detail_item->QtyL));
							}
							else if(SXml::IsName(p_n3, "ALI")) {
							}
							else if(SXml::IsName(p_n3, "DLM")) {
							}
							else if(SXml::IsName(p_n3, "DTM")) {
							}
							else if(SXml::IsName(p_n3, "FTX")) {
							}
							else if(SXml::IsName(p_n3, "MOA")) {
								THROW(Read_MOA(p_n3->children, p_new_detail_item->MoaL));
							}
							else if(SXml::IsName(p_n3, "SG18")) {
							}
							else if(SXml::IsName(p_n3, "SG20")) {
							}
							else if(SXml::IsName(p_n3, "SG22")) {
							}
							else if(SXml::IsName(p_n3, "SG25")) {
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "CNT")) {
				int    cnt_q = 0;
				double cnt_value = 0.0;
				THROW(Read_CNT(p_n->children, &cnt_q, &cnt_value));
			}
			else if(SXml::IsName(p_n, "UNT")) {
				uint   seg_count = 0;
				THROW(Read_UNT(p_n->children, temp_buf, &seg_count));
			}
		}
		{
			uint   i;
			PPID   bill_op_id = P_Pi->ACfg.Hdr.EdiDesadvOpID;
			LDATE  bill_dt = ZERODATE;
			LDATE  bill_due_dt = ZERODATE;
			bill_dt = document.GetBillDate();
			bill_due_dt = document.GetBillDueDate();
			for(i = 0; i < document.MoaL.getCount(); i++) {
				const QValue & r_val = document.MoaL.at(i);
				switch(r_val.Q) {
					case amtqAmtDue: break;
					case amtqOriginalAmt: break;
					case amtqTaxAmt: break;
					case amtqTaxableAmt: break;
				}
			}
			{
				PartyResolveBlock parties_blk;
				PPBillPacket::SetupObjectBlock sob;
				THROW_MEM(p_pack = new PPEdiProcessor::Packet(PPEDIOP_DESADV));
				THROW(PreprocessPartiesOnReading(p_pack->DocType, &document, &parties_blk));
				p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
				THROW(p_bpack->CreateBlank_WithoutCode(bill_op_id, 0, parties_blk.BillLocID, 1));
				p_bpack->Rec.EdiOp = p_pack->DocType;
				STRNSCPY(p_bpack->Rec.Code, document.GetFinalBillCode());
				p_bpack->Rec.Dt = bill_dt;
				if(checkdate(bill_due_dt, 0))
					p_bpack->Rec.DueDate = bill_due_dt;
				THROW(p_bpack->SetupObject(parties_blk.BillObjID, sob));
				for(i = 0; i < document.DetailL.getCount(); i++) {
					const DocumentDetailValue * p_item = document.DetailL.at(i);
					if(p_item) {
						PPTransferItem ti;
						PPID   goods_id = 0;
						THROW(ti.Init(&p_bpack->Rec, 1));
						THROW(PreprocessGoodsOnReading(p_bpack, p_item, &goods_id));
						assert(goods_id);
						if(goods_id) {
							ti.SetupGoods(goods_id, 0);
							double line_amount_total = 0.0; // with VAT
							double line_amount = 0.0; // without VAT
							double line_tax_amount = 0.0;
							double line_price = 0.0; // without VAT
							double line_qtty = 0.0;
							double ordered_qtty = 0.0;
							{
								for(uint j = 0; j < p_item->QtyL.getCount(); j++) {
									const QValue & r_qitem = p_item->QtyL.at(j);
									if(r_qitem.Q == qtyqDespatch)
										line_qtty = r_qitem.Value;
									else if(r_qitem.Q == qtyqOrdered)
										ordered_qtty = r_qitem.Value;
								}
							}
							{
								for(uint j = 0; j < p_item->MoaL.getCount(); j++) {
									const QValue & r_qitem = p_item->MoaL.at(j);
									if(r_qitem.Q == amtqTotalLnItemsAmt)
										line_amount_total = r_qitem.Value;
									else if(r_qitem.Q == amtqUnitPrice) // without VAT
										line_price = r_qitem.Value;
									else if(r_qitem.Q == amtqLnItemAmt)
										line_amount = r_qitem.Value;
									else if(r_qitem.Q == amtqTaxAmt)
										line_tax_amount = r_qitem.Value;
								}
							}
							if(line_qtty > 0.0) {
								ti.Quantity_ = R6(fabs(line_qtty));
								if(line_amount_total > 0.0)
									ti.Cost = R5(line_amount_total / ti.Quantity_);
								else if(line_amount > 0.0)
									ti.Cost = R5(line_amount / ti.Quantity_);
								ti.Price = 0.0;
								p_bpack->LoadTItem(&ti, 0, 0);
							}
						}
					}
				}
			}
		}
		ok = 1;
	}
	else if(SXml::IsName(p_root, "ORDERS")) {
		THROW_PP(P_Pi->ACfg.Hdr.OpID, PPERR_EDI_OPNDEF_ORDER);
		THROW(Read_CommonDocumentEntries(p_root->children, document));
		THROW_MEM(p_pack = new PPEdiProcessor::Packet(PPEDIOP_ORDER));
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			//
			// ...
			//
			if(SXml::IsName(p_n, "CNT")) {
				int    cnt_q = 0;
				double cnt_value = 0.0;
				THROW(Read_CNT(p_n->children, &cnt_q, &cnt_value));
			}
			else if(SXml::IsName(p_n, "UNT")) {
				uint   seg_count = 0;
				THROW(Read_UNT(p_n->children, temp_buf, &seg_count));
			}
		}
	}
	else if(SXml::IsName(p_root, "ORDRSP")) { // @v11.2.7
		PPID   ordrsp_op_id = 0;
		//PPBillPacket * p_bp_rsp = 0;
		PPBillPacket * p_bp_ord = 0;
		THROW(op_obj.GetEdiOrdrspOp(&ordrsp_op_id, 1));
		THROW(Read_CommonDocumentEntries(p_root->children, document));
		THROW_MEM(p_pack = new PPEdiProcessor::Packet(PPEDIOP_ORDERRSP));
		p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
		p_bp_ord = static_cast<PPBillPacket *>(p_pack->P_ExtData);
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "SG1")) {
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "NAD")) {
						PartyValue * p_party = document.PartyL.CreateNewItem();
						THROW_SL(p_party);
						THROW(Read_NAD(p_inr->children, *p_party));
					}
					else if(SXml::IsName(p_inr, "DTM")) {
						THROW(Read_DTM(p_inr->children, document.DtmL));
					}
					else if(SXml::IsName(p_inr, "RFF")) {
						THROW(Read_RFF(p_inr->children, document.RefL));
					}
				}
			}
			else if(SXml::IsName(p_n, "SG3")) {
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "NAD")) {
						PartyValue * p_party = document.PartyL.CreateNewItem();
						THROW_SL(p_party);
						THROW(Read_NAD(p_inr->children, *p_party));
					}
					else if(SXml::IsName(p_inr, "SG4")) {
						for(xmlNode * p_inr2 = p_inr->children; p_inr2; p_inr2 = p_inr2->next) {
							if(SXml::IsName(p_inr2, "RFF")) {
								THROW(Read_RFF(p_inr2->children, document.RefL));
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "SG4")) {
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "RFF")) {
						THROW(Read_RFF(p_inr->children, document.RefL));
					}
				}
			}
			else if(SXml::IsName(p_n, "SG8")) {
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "CUX")) {
						THROW(Read_CUX(p_inr->children, document.Cux));
					}
				}
			}
			else if(SXml::IsName(p_n, "SG26")) {
				DocumentDetailValue * p_new_detail_item = document.DetailL.CreateNewItem();
				THROW_SL(p_new_detail_item);
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "LIN")) {
						THROW(Read_LIN(p_inr->children, p_new_detail_item->LinV));
					}
					else if(SXml::IsName(p_inr, "PIA")) {
						THROW(Read_PIA(p_inr->children, p_new_detail_item->PiaL));
					}
					else if(SXml::IsName(p_inr, "QTY")) {
						THROW(Read_QTY(p_inr->children, p_new_detail_item->QtyL));
					}
					else if(SXml::IsName(p_inr, "MOA")) {
						THROW(Read_MOA(p_inr->children, p_new_detail_item->MoaL));
					}
					else if(SXml::IsName(p_inr, "FTX")) {
					}
					else if(SXml::IsName(p_inr, "SG30")) {
						for(xmlNode * p_inr2 = p_inr->children; p_inr2; p_inr2 = p_inr2->next) {
							if(SXml::IsName(p_inr2, "PRI")) {
								THROW(Read_PRI(p_inr2->children, p_new_detail_item->PriL));
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "UNS")) {
				for(xmlNode * p_inr = p_n->children; p_inr; p_inr = p_inr->next) {
					if(SXml::IsName(p_inr, "E0081")) {
					}
				}
			}
			else if(SXml::IsName(p_n, "CNT")) {
				int    cnt_q = 0;
				double cnt_value = 0.0;
				THROW(Read_CNT(p_n->children, &cnt_q, &cnt_value));
			}
			else if(SXml::IsName(p_n, "UNT")) {
				uint   seg_count = 0;
				THROW(Read_UNT(p_n->children, temp_buf, &seg_count));
			}
		}
		{
			Goods2Tbl::Rec goods_rec;
			SString order_number;
			LDATE  order_date = document.GetLinkedOrderDate();
			assert(p_bpack);
			p_bpack->CreateBlank_WithoutCode(ordrsp_op_id, 0/*link_bill_id*/, 0/*loc_id*/, 1);
			p_bpack->Rec.EdiOp = p_pack->DocType;
			p_bpack->Rec.Dt = document.GetBillDate();
			p_bpack->Rec.DueDate = document.GetBillDueDate();
			STRNSCPY(p_bpack->Rec.Code, document.GetFinalBillCode());
			{
				for(uint rffidx = 0; rffidx < document.RefL.getCount(); rffidx++) {
					const RefValue * p_rff = document.RefL.at(rffidx);
					if(p_rff && p_rff->Q == refqON)
						order_number = p_rff->Ref;
				}
			}
			{
				for(uint nadidx = 0; nadidx < document.PartyL.getCount(); nadidx++) {
					const PartyValue * p_nad = document.PartyL.at(nadidx);
					if(p_nad) {
						if(p_nad->PartyQ) {
							pProvider->ResolveContractor(p_nad->Code, p_nad->PartyQ, p_bpack);
						}
					}
				}
			}
			if(order_number.NotEmptyS() && checkdate(order_date)) {
				BillTbl::Rec ord_bill_rec;
				if(pProvider->SearchLinkedBill(order_number, order_date, p_bpack->Rec.Object, PPEDIOP_ORDER, &ord_bill_rec) > 0) {
					p_bpack->Rec.LinkBillID = ord_bill_rec.ID;
					THROW(pProvider->P_BObj->ExtractPacket(p_bpack->Rec.LinkBillID, p_bp_ord) > 0);
				}
			}
			for(uint linidx = 0; linidx < document.DetailL.getCount(); linidx++) {
				const DocumentDetailValue * p_lin = document.DetailL.at(linidx);
				if(p_lin) {
					PPTransferItem ti;
					ti.RByBill = p_lin->LinV.LineN;
					PPID   buyer_id = 0;
					const  char * p_ar_code = 0; // ссылается на поле элемента p_lin->PiaL
					for(uint piaidx = 0; piaidx < p_lin->PiaL.getCount(); piaidx++) {
						const PiaValue & r_pia = p_lin->PiaL.at(piaidx);
						if(r_pia.Itic == iticIN) {
							buyer_id = satoi(r_pia.Code);
						}
						else if(r_pia.Itic == iticSA) {
							p_ar_code = r_pia.Code;
						}
					}
					if(p_lin->LinV.GoodsCode.NotEmpty() && pProvider->GObj.SearchByBarcode(p_lin->LinV.GoodsCode, 0, &goods_rec) > 0) {
						ti.GoodsID = goods_rec.ID;
					}
					else if(buyer_id && pProvider->GObj.Search(buyer_id, &goods_rec) > 0) {
						ti.GoodsID = goods_rec.ID;
					}
					if(ti.GoodsID) {
						double qty_ordered = 0.0;
						double qty_allocated = 0.0;
						double amt_ln_total = 0.0;
						for(uint imdidx = 0; imdidx < p_lin->ImdL.getCount(); imdidx++) {
							const ImdValue * p_imd = p_lin->ImdL.at(imdidx);
						}
						for(uint moaidx = 0; moaidx < p_lin->MoaL.getCount(); moaidx++) {
							const QValue & r_moa = p_lin->MoaL.at(moaidx);
							if(r_moa.Q == amtqTotalLnItemsAmt) // 79 Total line items amount
								amt_ln_total = r_moa.Value;
						}
						for(uint qtyidx = 0; qtyidx < p_lin->QtyL.getCount(); qtyidx++) {
							const QValue & r_qty = p_lin->QtyL.at(qtyidx);
							if(r_qty.Q == qtyqOrdered)
								qty_ordered = r_qty.Value;
							else if(r_qty.Q == qtyqAllocated)
								qty_allocated = r_qty.Value;
						}
						for(uint priidx = 0; priidx < p_lin->PriL.getCount(); priidx++) {
							const QValue & r_pri = p_lin->PriL.at(priidx);
						}
						ti.Quantity_ = qty_allocated;
						if(qty_allocated > 0.0)
							ti.Cost = amt_ln_total / qty_allocated;
						THROW(p_bpack->LoadTItem(&ti, 0, 0));
					}
					else {
						// @err
					}
				}
			}
		}
	}
	if(p_pack) {
		if(p_bpack)
			p_bpack->BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, pIdent);
		rList.insert(p_pack);
		p_pack = 0; // Перед выходом экземпляр разрушается
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	delete p_pack;
	return ok;
}

int PPEanComDocument::Write_DESADV(xmlTextWriter * pX, const PPBillPacket & rPack)
{
	int    ok = 1;
	const  int edi_op = PPEDIOP_DESADV;
	uint   seg_count = 0;
	SString temp_buf;
	SString bill_ident;
	LDATETIME dtm;
	BillGoodsItemsTotal items_total;
	if(rPack.BTagL.GetItemStr(PPTAG_BILL_UUID, temp_buf) > 0)
		bill_ident = temp_buf;
	else
		bill_ident.Z().Cat(rPack.Rec.ID);
	{
		SXml::WDoc _doc(pX, cpUTF8);
		SXml::WNode n_docs(_doc, "DESADV");
		n_docs.PutAttrib("version", "1.07");
		{
			THROW(Write_MessageHeader(_doc, edi_op, bill_ident)); // "UNH" Message header
			seg_count++;
			// @v11.1.12 BillCore::GetCode(temp_buf = rPack.Rec.Code);
			temp_buf = rPack.Rec.Code; // @v11.1.12 
			THROW(Write_BeginningOfMessage(_doc, "351", temp_buf, fmsgcodeOriginal)); // "BGM" Beginning of message
			seg_count++;
			dtm.Set(rPack.Rec.Dt, ZEROTIME);
			THROW(Write_DTM(_doc, dtmqDocument, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
			seg_count++;
			if(rPack.P_Freight) {
				if(checkdate(rPack.P_Freight->IssueDate, 0)) {
					dtm.Set(rPack.P_Freight->IssueDate, ZEROTIME);
					THROW(Write_DTM(_doc, dtmqDespatch, dtmfmtCCYYMMDD, dtm, 0));
					seg_count++;
				}
				if(checkdate(rPack.P_Freight->ArrivalDate, 0)) {
					dtm.Set(rPack.P_Freight->ArrivalDate, ZEROTIME);
					THROW(Write_DTM(_doc, dtmqDlvryEstimated, dtmfmtCCYYMMDD, dtm, 0));
					seg_count++;
				}
			}
			// amtqAmtDue amtqOriginalAmt amtqTaxableAmt amtqTaxAmt
			THROW(Write_MOA(_doc, amtqAmtDue, rPack.Rec.Amount));
			seg_count++;
			THROW(Write_MOA(_doc, amtqOriginalAmt, 0.0));
			seg_count++;
			THROW(Write_MOA(_doc, amtqTaxableAmt, 0.0));
			seg_count++;
			THROW(Write_MOA(_doc, amtqTaxAmt, 0.0));
			seg_count++;
			{
				PPIDArray order_id_list;
				rPack.GetOrderList(order_id_list);
				for(uint ordidx = 0; ordidx < order_id_list.getCount(); ordidx++) {
					const PPID ord_id = order_id_list.get(ordidx);
					BillTbl::Rec ord_rec;
					if(ord_id && BillObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
						// @v11.1.12 BillCore::GetCode(temp_buf = ord_rec.Code);
						temp_buf = ord_rec.Code; // @v11.1.12 
						LDATETIME ord_dtm;
						ord_dtm.Set(ord_rec.Dt, ZEROTIME);
						{
							SXml::WNode n_sg1(_doc, "SG1");
							THROW(Write_RFF(_doc, refqON, temp_buf));
							if(checkdate(ord_dtm.d, 0)) {
								THROW(Write_DTM(_doc, dtmqReference, dtmfmtCCYYMMDD, ord_dtm, 0));
							}
						}
					}
				}
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				THROW(P_Pi->GetMainOrgGLN(temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_SUPPLIER, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_BUYER, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				if(rPack.GetDlvrAddrID()) {
					THROW(P_Pi->GetLocGLN(rPack.GetDlvrAddrID(), temp_buf));
				}
				else {
					THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				}
				THROW(Write_NAD(_doc, EDIPARTYQ_DELIVERY, temp_buf));
			}
			{
				SXml::WNode n_sg2(_doc, "SG2");
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_INVOICEE, temp_buf));
			}
			{
				SXml::WNode n_sg10(_doc, "SG10");
				{
					SXml::WNode n_cps(_doc, "CPS");
					n_cps.PutInner("E7164", "1"); // Номер иерархии по умолчанию - 1
				}
				{
					for(uint i = 0; i < rPack.GetTCount(); i++) {
						const PPTransferItem & r_ti = rPack.ConstTI(i);
						SXml::WNode n_sg17(_doc, "SG17"); // maxOccurs="200000" LIN-PIA-IMD-MEA-QTY-ALI-DTM-MOA-GIN-QVR-FTX-SG32-SG33-SG34-SG37-SG38-SG39-SG43-SG49
						// A group of segments providing details of the individual ordered items. This Segment group may be repeated to give sub-line details.
						THROW(Write_DesadvGoodsItem(_doc, edi_op, r_ti, TIAMT_PRICE, items_total));
					}
					seg_count += items_total.SegCount;
				}
			}
			THROW(Write_CNT(_doc, cntqQuantity, items_total.Quantity));
			seg_count++;
			THROW(Write_CNT(_doc, cntqNumOfLn, items_total.Count));
			seg_count++;
			THROW(Write_UNT(_doc, bill_ident, ++seg_count));
		}
	}
	CATCHZOK
	return ok;
}

int PPEanComDocument::Write_ORDERRSP(xmlTextWriter * pX, const PPBillPacket & rPack, const PPBillPacket * pExtPack)
{
	int    ok = -1;
	return ok;
}

int PPEanComDocument::Write_ORDERS(xmlTextWriter * pX, const PPBillPacket & rPack)
{
	int    ok = 1;
	const  int edi_op = PPEDIOP_ORDER;
	uint   seg_count = 0;
	SString temp_buf;
	SString fmt;
	SString bill_ident;
	LDATETIME dtm;
	//THROW_PP(pX, IEERR_NULLWRIEXMLPTR);
	BillGoodsItemsTotal items_total;
	if(rPack.BTagL.GetItemStr(PPTAG_BILL_UUID, temp_buf) > 0)
		bill_ident = temp_buf;
	else
		bill_ident.Z().Cat(rPack.Rec.ID);
	{
		SXml::WDoc _doc(pX, cpUTF8);
		SXml::WNode n_docs(_doc, "ORDERS");
		n_docs.PutAttrib("version", "1.07");
		{
			THROW(Write_MessageHeader(_doc, edi_op, bill_ident)); // "UNH" Message header
			seg_count++;
			// @v11.1.12 BillCore::GetCode(temp_buf = rPack.Rec.Code);
			temp_buf = rPack.Rec.Code; // @v11.1.12 
			THROW(Write_BeginningOfMessage(_doc, "220", temp_buf, fmsgcodeOriginal)); // "BGM" Beginning of message
			seg_count++;
			dtm.Set(rPack.Rec.Dt, ZEROTIME);
			THROW(Write_DTM(_doc, dtmqDocument, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
			seg_count++;
			if(checkdate(rPack.Rec.DueDate, 0)) {
				dtm.Set(rPack.Rec.DueDate, ZEROTIME);
				THROW(Write_DTM(_doc, dtmqDlvry, dtmfmtCCYYMMDD, dtm, 0)); // "DTM" // Date/time/period // maxOccurs="35"
				seg_count++;
			}
			//SXml::WNode n_sg1(_doc, "SG1"); // RFF-DTM // minOccurs="0" maxOccurs="9999"
			{ // Поставщик
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				THROW(Write_NAD(_doc, EDIPARTYQ_SUPPLIER, temp_buf));
				seg_count++;
			}
			{ // Грузоотправитель
				THROW(P_Pi->GetArticleGLN(rPack.Rec.Object, temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_CONSIGNOR, temp_buf));
				seg_count++;
			}
			{ // Покупатель
				THROW(P_Pi->GetMainOrgGLN(temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_BUYER, temp_buf));
				seg_count++;
			}
			{ // Адрес доставки
				THROW(P_Pi->GetLocGLN(rPack.Rec.LocID, temp_buf));
				SXml::WNode n_sg2(_doc, "SG2"); // NAD-LOC-FII-SG3-SG4-SG5 // maxOccurs="99"
				THROW(Write_NAD(_doc, EDIPARTYQ_DELIVERY, temp_buf));
				seg_count++;
			}
			{
				SXml::WNode n_sg7(_doc, "SG7"); // CUX-DTM // minOccurs="0" maxOccurs="5"
				THROW(Write_CUX(_doc, "RUB"));
				seg_count++;
			}
			{
				for(uint i = 0; i < rPack.GetTCount(); i++) {
					const PPTransferItem & r_ti = rPack.ConstTI(i);
					SXml::WNode n_sg28(_doc, "SG28"); // maxOccurs="200000" LIN-PIA-IMD-MEA-QTY-ALI-DTM-MOA-GIN-QVR-FTX-SG32-SG33-SG34-SG37-SG38-SG39-SG43-SG49
					// A group of segments providing details of the individual ordered items. This Segment group may be repeated to give sub-line details.
					THROW(Write_OrderGoodsItem(_doc, edi_op, r_ti, TIAMT_COST, items_total));
				}
				seg_count += items_total.SegCount;
			}
			{
				SXml::WNode n_uns(_doc, "UNS"); // Разделитель зон
				n_uns.PutInner("E0081", "S"); // Идентификатор секции (Зона итоговой информации)
				seg_count++;
			}
			THROW(Write_MOA(_doc, amtqTotalAmt, items_total.AmountWithTax));
			seg_count++;
			THROW(Write_MOA(_doc, amtqOriginalAmt, items_total.AmountWoTax));
			seg_count++;
			THROW(Write_CNT(_doc, cntqNumOfLn, items_total.Count));
			seg_count++;
			THROW(Write_UNT(_doc, bill_ident, ++seg_count));
		}
	}
	CATCHZOK
	return ok;
}

static const SIntToSymbTabEntry EdiMsgTypeSymbols_EanCom[] = {
	{ PPEDIOP_ORDER,        "ORDERS" },
	{ PPEDIOP_ORDERRSP,     "ORDRSP" },
	{ PPEDIOP_APERAK,       "APERAK" },
	{ PPEDIOP_DESADV,       "DESADV" },
	{ PPEDIOP_DECLINEORDER, "DECLNORDER" },
	{ PPEDIOP_RECADV,       "RECADV" },
	{ PPEDIOP_ALCODESADV,   "ALCODESADV" },
	{ PPEDIOP_PARTIN,       "PARTIN" },
	{ PPEDIOP_INVOIC,       "INVOIC" },
};

/*static*/int FASTCALL PPEanComDocument::GetMsgSymbByType(int msgType, SString & rSymb)
	{ return SIntToSymbTab_GetSymb(EdiMsgTypeSymbols_EanCom, SIZEOFARRAY(EdiMsgTypeSymbols_EanCom), msgType, rSymb); }
/*static*/int FASTCALL PPEanComDocument::GetMsgTypeBySymb(const char * pSymb)
	{ return SIntToSymbTab_GetId(EdiMsgTypeSymbols_EanCom, SIZEOFARRAY(EdiMsgTypeSymbols_EanCom), pSymb); }

static const SIntToSymbTabEntry EanComPriceQSymbList[] = {
	{ PPEanComDocument::priceqAAA, "AAA" },
	{ PPEanComDocument::priceqAAE, "AAE" },
	{ PPEanComDocument::priceqAAF, "AAF" },
	{ PPEanComDocument::priceqAAH, "AAH" },
	{ PPEanComDocument::priceqAAQ, "AAQ" },
	{ PPEanComDocument::priceqABL, "ABL" },
	{ PPEanComDocument::priceqABM, "ABM" },
};

/*static*/int FASTCALL PPEanComDocument::GetPriceqSymb(int priceq, SString & rSymb)
	{ return SIntToSymbTab_GetSymb(EanComPriceQSymbList, SIZEOFARRAY(EanComPriceQSymbList), priceq, rSymb); }
/*static*/int FASTCALL PPEanComDocument::GetPriceqBySymb(const char * pSymb)
	{ return SIntToSymbTab_GetId(EanComPriceQSymbList, SIZEOFARRAY(EanComPriceQSymbList), pSymb); }

static const SIntToSymbTabEntry EanComRefQSymbList[] = {
	{ PPEanComDocument::refqAAB, "AAB" },
	{ PPEanComDocument::refqAAJ, "AAJ" },
	{ PPEanComDocument::refqAAK, "AAK" },
	{ PPEanComDocument::refqAAM, "AAM" },
	{ PPEanComDocument::refqAAN, "AAN" },
	{ PPEanComDocument::refqAAS, "AAS" },
	{ PPEanComDocument::refqAAU, "AAU" },
	{ PPEanComDocument::refqABT, "ABT" },
	{ PPEanComDocument::refqAFO, "AFO" },
	{ PPEanComDocument::refqAIZ, "AIZ" },
	{ PPEanComDocument::refqALL, "ALL" },
	{ PPEanComDocument::refqAMT, "AMT" },
	{ PPEanComDocument::refqAPQ, "APQ" },
	{ PPEanComDocument::refqASI, "ASI" },
	{ PPEanComDocument::refqAWT, "AWT" },
	{ PPEanComDocument::refqCD,  "CD"  },
	{ PPEanComDocument::refqCR,  "CR"  },
	{ PPEanComDocument::refqCT,  "CT"  },
	{ PPEanComDocument::refqDL,  "DL"  },
	{ PPEanComDocument::refqDQ,  "DQ"  },
	{ PPEanComDocument::refqFC,  "FC"  },
	{ PPEanComDocument::refqIP,  "IP"  },
	{ PPEanComDocument::refqIV,  "IV"  },
	{ PPEanComDocument::refqON,  "ON"  },
	{ PPEanComDocument::refqPK,  "PK"  },
	{ PPEanComDocument::refqPL,  "PL"  },
	{ PPEanComDocument::refqPOR, "POR" },
	{ PPEanComDocument::refqPP,  "PP"  },
	{ PPEanComDocument::refqRF,  "RF"  },
	{ PPEanComDocument::refqVN,  "VN"  },
	{ PPEanComDocument::refqXA,  "XA"  },
	{ PPEanComDocument::refqAE,  "AE" },
	{ PPEanComDocument::refqBO,  "BO" },
	{ PPEanComDocument::refqPD,  "PD" },
	{ PPEanComDocument::refqUC,  "UC" },
	{ PPEanComDocument::refqAKO, "AKO" },
	{ PPEanComDocument::refqANJ, "ANJ" },
};

/*static*/int FASTCALL PPEanComDocument::GetRefqSymb(int refq, SString & rSymb)
	{ return SIntToSymbTab_GetSymb(EanComRefQSymbList, SIZEOFARRAY(EanComRefQSymbList), refq, rSymb); }
/*static*/int FASTCALL PPEanComDocument::GetRefqBySymb(const char * pSymb)
	{ return SIntToSymbTab_GetId(EanComRefQSymbList, SIZEOFARRAY(EanComRefQSymbList), pSymb); }

static const SIntToSymbTabEntry EanComPartyQSymbList[] = {
	{ EDIPARTYQ_BUYER, "BY" },
	{ EDIPARTYQ_CORPOFFICE, "CO" },
	{ EDIPARTYQ_DELIVERY, "DP" },
	{ EDIPARTYQ_INVOICEE, "IV" },
	{ EDIPARTYQ_STORENUMBER, "SN" },
	{ EDIPARTYQ_SUPPLAGENT, "SR" },
	{ EDIPARTYQ_SUPPLIER, "SU" },
	{ EDIPARTYQ_WAREHOUSEKEEPER, "WH" },
	{ EDIPARTYQ_CONSIGNOR, "CZ" },
	{ EDIPARTYQ_CONSIGNEE, "CN" },
	{ EDIPARTYQ_SELLER, "SE" },
	{ EDIPARTYQ_PAYEE,  "PE" },
	{ EDIPARTYQ_CONSOLIDATOR,  "CS" },
	{ EDIPARTYQ_ISSUEROFINVOICE,  "II" },
	{ EDIPARTYQ_SHIPTO,  "ST" },
	{ EDIPARTYQ_BILLANDSHIPTO,  "BS" },
	{ EDIPARTYQ_BROKERORSALESOFFICE,  "BO" },
	//LD 		= 	Party recovering the Value Added Tax (VAT)
	//RE 		= 	Party to receive commercial invoice remittance
	//LC 		= 	Party declaring the Value Added Tax (VAT)
};

/*static*/int FASTCALL PPEanComDocument::GetPartyqSymb(int refq, SString & rSymb)
	{ return SIntToSymbTab_GetSymb(EanComPartyQSymbList, SIZEOFARRAY(EanComPartyQSymbList), refq, rSymb); }
/*static*/int FASTCALL PPEanComDocument::GetPartyqBySymb(const char * pSymb)
	{ return SIntToSymbTab_GetId(EanComPartyQSymbList, SIZEOFARRAY(EanComPartyQSymbList), pSymb); }

static const SIntToSymbTabEntry EanComIticSymbList[] = {
	{ PPEanComDocument::iticAA, "AA" }, // Product version number. Number assigned by manufacturer or seller to identify the release of a product.
	{ PPEanComDocument::iticAB, "AB" }, // Assembly. The item number is that of an assembly.
	{ PPEanComDocument::iticAC, "AC" }, // HIBC (Health Industry Bar Code). Article identifier used within health sector to indicate data used conforms to HIBC.
	{ PPEanComDocument::iticAD, "AD" }, // Cold roll number. Number assigned to a cold roll.
	{ PPEanComDocument::iticAE, "AE" }, // Hot roll number. Number assigned to a hot roll.
	{ PPEanComDocument::iticAF, "AF" }, // Slab number. Number assigned to a slab, which is produced in a particular production step.
	{ PPEanComDocument::iticAG, "AG" }, // Software revision number. A number assigned to indicate a revision of software.
	{ PPEanComDocument::iticAH, "AH" }, // UPC (Universal Product Code) Consumer package code (1-5-5). An 11-digit code that uniquely identifies consumer packaging of a product; does not have a check digit.
	{ PPEanComDocument::iticAI, "AI" }, // UPC (Universal Product Code) Consumer package code (1-5-5-1). A 12-digit code that uniquely identifies the consumer packaging of a product, including a check digit.
	{ PPEanComDocument::iticAJ, "AJ" }, // Sample number. Number assigned to a sample.
	{ PPEanComDocument::iticAK, "AK" }, // Pack number. Number assigned to a pack containing a stack of items put together (e.g. cold roll sheets (steel product)).
	{ PPEanComDocument::iticAL, "AL" }, // UPC (Universal Product Code) Shipping container code (1-2-5-5). A 13-digit code that uniquely identifies the manufacturer's shipping unit, including the packaging indicator.
	{ PPEanComDocument::iticAM, "AM" }, // UPC (Universal Product Code)/EAN (European article number) Shipping container code (1-2-5-5-1). A 14-digit code that uniquely identifies the manufacturer's shipping unit, including the packaging indicator and the check digit.
	{ PPEanComDocument::iticAN, "AN" }, // UPC (Universal Product Code) suffix. A suffix used in conjunction with a higher level UPC (Universal product code) to define packing variations for a product.
	{ PPEanComDocument::iticAO, "AO" }, // State label code. A code which specifies the codification of the state's labelling requirements.
	{ PPEanComDocument::iticAP, "AP" }, // Heat number. Number assigned to the heat (also known as the iron charge) for the production of steel products.
	{ PPEanComDocument::iticAQ, "AQ" }, // Coupon number. A number identifying a coupon.
	{ PPEanComDocument::iticAR, "AR" }, // Resource number. A number to identify a resource.
	{ PPEanComDocument::iticAS, "AS" }, // Work task number. A number to identify a work task.
	{ PPEanComDocument::iticAT, "AT" }, // Price look up number. Identification number on a product allowing a quick electronic retrieval of price information for that product.
	{ PPEanComDocument::iticAU, "AU" }, // NSN (North Atlantic Treaty Organization Stock Number). Number assigned under the NATO (North Atlantic Treaty Organization) codification system to provide the identification of an approved item of supply.
	{ PPEanComDocument::iticAV, "AV" }, // Refined product code. A code specifying the product refinement designation.
	{ PPEanComDocument::iticAW, "AW" }, // Exhibit. A code indicating that the product is identified by an exhibit number.
	{ PPEanComDocument::iticAX, "AX" }, // End item. A number specifying an end item.
	{ PPEanComDocument::iticAY, "AY" }, // Federal supply classification. A code to specify a product's Federal supply classification.
	{ PPEanComDocument::iticAZ, "AZ" }, // Engineering data list. A code specifying the product's engineering data list.
	{ PPEanComDocument::iticBA, "BA" }, // Milestone event number. A number to identify a milestone event.
	{ PPEanComDocument::iticBB, "BB" }, // Lot number. A number indicating the lot number of a product.
	{ PPEanComDocument::iticBC, "BC" }, // National drug code 4-4-2 format. A code identifying the product in national drug format 4-4-2.
	{ PPEanComDocument::iticBD, "BD" }, // National drug code 5-3-2 format. A code identifying the product in national drug format 5-3-2.
	{ PPEanComDocument::iticBE, "BE" }, // National drug code 5-4-1 format. A code identifying the product in national drug format 5-4-1.
	{ PPEanComDocument::iticBF, "BF" }, // National drug code 5-4-2 format. A code identifying the product in national drug format 5-4-2.
	{ PPEanComDocument::iticBG, "BG" }, // National drug code. A code specifying the national drug classification.
	{ PPEanComDocument::iticBH, "BH" }, // Part number. A number indicating the part.
	{ PPEanComDocument::iticBI, "BI" }, // Local Stock Number (LSN). A local number assigned to an item of stock.
	{ PPEanComDocument::iticBJ, "BJ" }, // Next higher assembly number. A number specifying the next higher assembly or component into which the product is being incorporated.
	{ PPEanComDocument::iticBK, "BK" }, // Data category. A code specifying a category of data.
	{ PPEanComDocument::iticBL, "BL" }, // Control number. To specify the control number.
	{ PPEanComDocument::iticBM, "BM" }, // Special material identification code. A number to identify the special material code.
	{ PPEanComDocument::iticBN, "BN" }, // Locally assigned control number. A number assigned locally for control purposes.
	{ PPEanComDocument::iticBO, "BO" }, // Buyer's colour. Colour assigned by buyer.
	{ PPEanComDocument::iticBP, "BP" }, // Buyer's part number. Reference number assigned by the buyer to identify an article.
	{ PPEanComDocument::iticBQ, "BQ" }, // Variable measure product code. A code assigned to identify a variable measure item.
	{ PPEanComDocument::iticBR, "BR" }, // Financial phase. To specify as an item, the financial phase.
	{ PPEanComDocument::iticBS, "BS" }, // Contract breakdown. To specify as an item, the contract breakdown.
	{ PPEanComDocument::iticBT, "BT" }, // Technical phase. To specify as an item, the technical phase.
	{ PPEanComDocument::iticBU, "BU" }, // Dye lot number. Number identifying a dye lot.
	{ PPEanComDocument::iticBV, "BV" }, // Daily statement of activities. A statement listing activities of one day.
	{ PPEanComDocument::iticBW, "BW" }, // Periodical statement of activities within a bilaterally agreed time period. Periodical statement listing activities within a bilaterally agreed time period.
	{ PPEanComDocument::iticBX, "BX" }, // Calendar week statement of activities. A statement listing activities of a calendar week.
	{ PPEanComDocument::iticBY, "BY" }, // Calendar month statement of activities. A statement listing activities of a calendar month.
	{ PPEanComDocument::iticBZ, "BZ" }, // Original equipment number. Original equipment number allocated to spare parts by the manufacturer.
	{ PPEanComDocument::iticCC, "CC" }, // Industry commodity code. The codes given to certain commodities by an industry.
	{ PPEanComDocument::iticCG, "CG" }, // Commodity grouping. Code for a group of articles with common characteristics (e.g. used for statistical purposes).
	{ PPEanComDocument::iticCL, "CL" }, // Colour number. Code for the colour of an article.
	{ PPEanComDocument::iticCR, "CR" }, // Contract number. Reference number identifying a contract.
	{ PPEanComDocument::iticCV, "CV" }, // Customs article number. Code defined by Customs authorities to an article or a group of articles for Customs purposes.
	{ PPEanComDocument::iticDR, "DR" }, // Drawing revision number. Reference number indicating that a change or revision has been applied to a drawing.
	{ PPEanComDocument::iticDW, "DW" }, // Drawing. Reference number identifying a drawing of an article.
	{ PPEanComDocument::iticEC, "EC" }, // Engineering change level. Reference number indicating that a change or revision has been applied to an article's specification.
	{ PPEanComDocument::iticEF, "EF" }, // Material code. Code defining the material's type, surface, geometric form plus various classifying characteristics.
	{ PPEanComDocument::iticEN, "EN" }, // International Article Numbering Association (EAN). Number assigned to a manufacturer's product according to the International Article Numbering Association.
	{ PPEanComDocument::iticGB, "GB" }, // Buyer's internal product group code. Product group code used within a buyer's internal systems.
	{ PPEanComDocument::iticGN, "GN" }, // National product group code. National product group code. Administered by a national agency.
	{ PPEanComDocument::iticGS, "GS" }, // General specification number. The item number is a general specification number.
	{ PPEanComDocument::iticHS, "HS" }, // Harmonised system. The item number is part of, or is generated in the context of the Harmonised Commodity Description and Coding System (Harmonised System), as developed and maintained by the World Customs Organization (WCO).
	{ PPEanComDocument::iticIB, "IB" }, // ISBN (International Standard Book Number). A unique number identifying a book.
	{ PPEanComDocument::iticIN, "IN" }, // Buyer's item number. The item number has been allocated by the buyer.
	{ PPEanComDocument::iticIS, "IS" }, // ISSN (International Standard Serial Number). A unique number identifying a serial publication.
	{ PPEanComDocument::iticIT, "IT" }, // Buyer's style number. Number given by the buyer to a specific style or form of an article, especially used for garments.
	{ PPEanComDocument::iticIZ, "IZ" }, // Buyer's size code. Code given by the buyer to designate the size of an article in textile and shoe industry.
	{ PPEanComDocument::iticLI, "LI" }, // Line item number (GS1 Temporary Code). Number identifying a specific line within a document/message.
	{ PPEanComDocument::iticMA, "MA" }, // Machine number. The item number is a machine number.
	{ PPEanComDocument::iticMF, "MF" }, // Manufacturer's (producer's) article number. The number given to an article by its manufacturer.
	{ PPEanComDocument::iticMN, "MN" }, // Model number. Reference number assigned by the manufacturer to differentiate variations in similar products in a class or group.
	{ PPEanComDocument::iticMP, "MP" }, // Product/service identification number. Reference number identifying a product or service.
	{ PPEanComDocument::iticNB, "NB" }, // Batch number. The item number is a batch number.
	{ PPEanComDocument::iticON, "ON" }, // Customer order number. Reference number of a customer's order.
	{ PPEanComDocument::iticPD, "PD" }, // Part number description. Reference number identifying a description associated with a number ultimately used to identify an article.
	{ PPEanComDocument::iticPL, "PL" }, // Purchaser's order line number. Reference number identifying a line entry in a customer's order for goods or services.
	{ PPEanComDocument::iticPO, "PO" }, // Purchase order number. Reference number identifying a customer's order.
	{ PPEanComDocument::iticPV, "PV" }, // Promotional variant number. The item number is a promotional variant number.
	{ PPEanComDocument::iticQS, "QS" }, // Buyer's qualifier for size. The item number qualifies the size of the buyer.
	{ PPEanComDocument::iticRC, "RC" }, // Returnable container number. Reference number identifying a returnable container.
	{ PPEanComDocument::iticRN, "RN" }, // Release number. Reference number identifying a release from a buyer's purchase order.
	{ PPEanComDocument::iticRU, "RU" }, // Run number. The item number identifies the production or manufacturing run or sequence in which the item was manufactured, processed or assembled.
	{ PPEanComDocument::iticRY, "RY" }, // Record keeping of model year. The item number relates to the year in which the particular model was kept.
	{ PPEanComDocument::iticSA, "SA" }, // Supplier's article number. Number assigned to an article by the supplier of that article.
	{ PPEanComDocument::iticSG, "SG" }, // Standard group of products (mixed assortment). The item number relates to a standard group of other items (mixed) which are grouped together as a single item for identification purposes.
	{ PPEanComDocument::iticSK, "SK" }, // SKU (Stock keeping unit). Reference number of a stock keeping unit.
	{ PPEanComDocument::iticSN, "SN" }, // Serial number. Identification number of an item which distinguishes this specific item out of a number of identical items.
	{ PPEanComDocument::iticSRS, "SRS" }, // RSK number. Plumbing and heating.
	{ PPEanComDocument::iticSRT, "SRT" }, // IFLS (Institut Francais du Libre Service) 5 digit product classification code. 5 digit code for product classification managed by the Institut Francais du Libre Service.
	{ PPEanComDocument::iticSRU, "SRU" }, // IFLS (Institut Francais du Libre Service) 9 digit product classification code. 9 digit code for product classification managed by the Institut Francais du Libre Service.
	{ PPEanComDocument::iticSRV, "SRV" }, // EAN.UCC Global Trade Item Number. A unique number, up to 14-digits, assigned according to the numbering structure of the EAN.UCC system. 'EAN' stands for the 'International Article Numbering Association', and 'UCC' for the 'Uniform Code Council'.
	{ PPEanComDocument::iticSRW, "SRW" }, // EDIS (Energy Data Identification System). European system for identification of meter data.
	{ PPEanComDocument::iticSRX, "SRX" }, // Slaughter number. Unique number given by a slaughterhouse to an animal or a group of animals of the same breed.
	{ PPEanComDocument::iticSRY, "SRY" }, // Official animal number. Unique number given by a national authority to identify an animal individually.
	{ PPEanComDocument::iticSS, "SS" }, // Supplier's supplier article number. Article number referring to a sales catalogue of supplier's supplier.
	{ PPEanComDocument::iticST, "ST" }, // Style number. Number given to a specific style or form of an article, especially used for garments.
	{ PPEanComDocument::iticTG, "TG" }, // Transport group number. (8012) Additional number to form article groups for packing and/or transportation purposes.
	{ PPEanComDocument::iticUA, "UA" }, // Ultimate customer's article number. Number assigned by ultimate customer to identify relevant article.
	{ PPEanComDocument::iticUP, "UP" }, // UPC (Universal product code). Number assigned to a manufacturer's product by the Product Code Council.
	{ PPEanComDocument::iticVN, "VN" }, // Vendor item number. Reference number assigned by a vendor/seller identifying a product/service/article.
	{ PPEanComDocument::iticVP, "VP" }, // Vendor's (seller's) part number. Reference number assigned by a vendor/seller identifying an article.
	{ PPEanComDocument::iticVS, "VS" }, // Vendor's supplemental item number. The item number is a specified by the vendor as a supplemental number for the vendor's purposes.
	{ PPEanComDocument::iticVX, "VX" }, // Vendor specification number. The item number has been allocated by the vendor as a specification number.
	{ PPEanComDocument::iticZZZ, "ZZZ" }, // Mutually defined. A code assigned within a code list to be used on an interim basis and as defined among trading partners until a precise code can be assigned to the code list.
};

/*static*/int FASTCALL PPEanComDocument::GetIticSymb(int refq, SString & rSymb)
	{ return SIntToSymbTab_GetSymb(EanComIticSymbList, SIZEOFARRAY(EanComIticSymbList), refq, rSymb); }
/*static*/int FASTCALL PPEanComDocument::GetIticBySymb(const char * pSymb)
	{ return SIntToSymbTab_GetId(EanComIticSymbList, SIZEOFARRAY(EanComIticSymbList), pSymb); }

class EdiProviderImplementation_Kontur : public PPEdiProcessor::ProviderImplementation {
public:
	EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags);
	virtual ~EdiProviderImplementation_Kontur();
	virtual int    GetDocumentList(const PPBillIterchangeFilt & rP, PPEdiProcessor::DocumentInfoList & rList);
	virtual int    ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList);
	virtual int    SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
private:
	struct OwnFormatCommonAttr {
		OwnFormatCommonAttr();
		void    Clear();

		SString Id; // "id"
		SString Num; // "number"
		LDATE  Dt; // "date"
		SString Status; // "status"
		SString Revision; // "revisionNumber"
		SString UOM; // "unitOfMeasure"
	};
	struct OwnFormatAddress {
		OwnFormatAddress();
		OwnFormatAddress & Z();

		char   CountryCode[4]; // 2-letter code
		SString AddressText; // foreign address
		SString RegionIsoCode;
		SString District;
		SString City;       // Город
		SString Settlement; // Населенный пункт
		SString Street;
		SString House;
		SString Flat;
		SString ZIP;
	};
	struct OwnFormatContractor {
		OwnFormatContractor();
		OwnFormatContractor & Z();

		SString Name;
		SString GLN;
		SString INN;
		SString KPP;
		//
		OwnFormatAddress Addr;
	};
	int    ResolveDlvrLoc(const OwnFormatContractor & rC, PPBillPacket * pPack);
	int    ResolveOwnFormatContractor(const OwnFormatContractor & rC, int partyQ, PPBillPacket * pPack);
	int    ReadCommonAttributes(const xmlNode * pNode, OwnFormatCommonAttr & rA);
	int    ReadOwnFormatDocument(void * pCtx, const char * pFileName, const char * pIdent, TSCollection <PPEdiProcessor::Packet> & rList);
	int    WriteOwnFormatContractor(SXml::WDoc & rDoc, PPID personID, PPID locID);
	int    ReadOwnFormatContractor(xmlNode * pNode, OwnFormatContractor & rC);
	int    Write_OwnFormat_ORDERS(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	//
	// Returns:
	//   1 - accepted
	//   2 - rejected
	//   3 - changed
	//
	int    IdentifyOrderRspStatus(const PPBillPacket & rBp, const PPBillPacket * pExtBp);
	int    Write_OwnFormat_ORDERRSP(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp, const PPBillPacket * pExtBp);
	int    Write_OwnFormat_DESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	int    Write_OwnFormat_ALCODESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	int    Write_OwnFormat_INVOIC(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	int    Write_OwnFormat_RECADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPEdiProcessor::RecadvPacket & rRaPack);
	void   Write_OwnFormat_OriginOrder_Tag(SXml::WDoc & rDoc, const BillTbl::Rec & rOrderBillRec);
};

class EdiProviderImplementation_Exite : public PPEdiProcessor::ProviderImplementation {
public:
	EdiProviderImplementation_Exite(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags);
	virtual ~EdiProviderImplementation_Exite();
	virtual int    GetDocumentList(const PPBillIterchangeFilt & rP, PPEdiProcessor::DocumentInfoList & rList);
	virtual int    ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList);
	virtual int    SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack);
private:
	int    Implement_Auth(SString & rToken);
	int    Auth();
	int    Helper_SendDocument(const char * pDocType, const char * pDocName, const char * pDocMime64, SString & rRetIdent);
	int    Write_OwnFormat_ORDERS(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	int    Write_OwnFormat_ORDERRSP(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp, const PPBillPacket * pExtBp);
	int    Write_OwnFormat_DESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);
	int    Write_OwnFormat_RECADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPEdiProcessor::RecadvPacket & rRaPack);
	int    Write_OwnFormat_INVOIC(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp);

	struct AuthToken {
		AuthToken() : Tm(ZERODATETIME)
		{
		}
		SString Token; // Токен авторизации
		LDATETIME Tm;  // Время получения (необходимо для идентификации факта просроченности токена)
	};
	AuthToken AT;
};

EdiProviderImplementation_Kontur::OwnFormatCommonAttr::OwnFormatCommonAttr() : Dt(ZERODATE)
{
}

void EdiProviderImplementation_Kontur::OwnFormatCommonAttr::Clear()
{
	Id.Z();
	Num.Z();
	Dt = ZERODATE;
	Status.Z();
	UOM.Z();
}

EdiProviderImplementation_Kontur::OwnFormatAddress::OwnFormatAddress()
{
	CountryCode[0] = 0;
}

EdiProviderImplementation_Kontur::OwnFormatAddress & EdiProviderImplementation_Kontur::OwnFormatAddress::Z()
{
	CountryCode[0] = 0;
	AddressText.Z();
	RegionIsoCode.Z();
	District.Z();
	City.Z();
	Settlement.Z();
	Street.Z();
	House.Z();
	Flat.Z();
	ZIP.Z();
	return *this;
}

EdiProviderImplementation_Kontur::OwnFormatContractor::OwnFormatContractor()
{
}

EdiProviderImplementation_Kontur::OwnFormatContractor & EdiProviderImplementation_Kontur::OwnFormatContractor::Z()
{
	Name.Z();
	GLN.Z();
	INN.Z();
	KPP.Z();
	Addr.Z();
	return *this;
}

int  EdiProviderImplementation_Kontur::ResolveDlvrLoc(const OwnFormatContractor & rC, PPBillPacket * pPack)
{
	int    ok = -1;
	if(rC.GLN.NotEmpty()) {
		PPIDArray loc_list_by_gln;
		PPID   final_dlvr_loc_id = 0;
		PsnObj.LocObj.ResolveGLN(LOCTYP_ADDRESS, rC.GLN, loc_list_by_gln);
		THROW_PP_S(loc_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLDLVRLOC, rC.GLN);
		for(uint i = 0; !final_dlvr_loc_id && i < loc_list_by_gln.getCount(); i++) {
			const PPID loc_id = loc_list_by_gln.get(i);
			LocationTbl::Rec loc_rec;
			if(PsnObj.LocObj.Fetch(loc_id, &loc_rec) > 0) {
				if(loc_rec.OwnerID) {
					PPID   acs_id = 0;
					PPID   psn_id = ObjectToPerson(pPack->Rec.Object, &acs_id);
					if(psn_id) {
						if(loc_rec.OwnerID == psn_id) {
							final_dlvr_loc_id = loc_id;
						}
					}
					else if(pPack->Rec.Object == 0) {
						final_dlvr_loc_id = loc_id;
					}
				}
			}
		}
		THROW_PP_S(final_dlvr_loc_id, PPERR_EDI_UNBLRSLV_BILLDLVRLOC, rC.GLN);
		{
			PPFreight freight;
			pPack->GetFreight(&freight);
			freight.SetupDlvrAddr(final_dlvr_loc_id);
			pPack->SetFreight(&freight);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::ResolveOwnFormatContractor(const OwnFormatContractor & rC, int partyQ, PPBillPacket * pPack)
{
	int    ok = 1;
	if(pPack) {
		SString msg_buf;
		const PPID reg_type_id = PPREGT_GLN;
		PPIDArray psn_list_by_gln;
		PPIDArray ar_list;
		if(rC.GLN.NotEmpty()) {
			msg_buf.CatDivIfNotEmpty('/', 0).Cat(rC.GLN);
			THROW(PsnObj.GetListByRegNumber(reg_type_id, 0, rC.GLN, psn_list_by_gln));
		}
		if(rC.INN.NotEmpty())
			msg_buf.CatDivIfNotEmpty('/', 0).Cat(rC.INN);
		if(rC.KPP.NotEmpty())
			msg_buf.CatDivIfNotEmpty('/', 0).Cat(rC.KPP);
		if(rC.Name.NotEmpty())
			msg_buf.CatDivIfNotEmpty('/', 0).Cat(rC.Name);
		msg_buf.SetIfEmpty("*");
		msg_buf.CatDiv('-', 1).Cat(pPack->Rec.Code).CatDiv('-', 1).Cat(pPack->Rec.Dt, DATF_DMY);
		if(partyQ == EDIPARTYQ_SELLER) {
			if(pPack->Rec.EdiOp == PPEDIOP_DESADV) {
				THROW_PP_S(psn_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
				THROW(ArObj.GetByPersonList(GetSupplAccSheet(), &psn_list_by_gln, &ar_list));
				THROW_PP_S(ar_list.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
				{
					PPBillPacket::SetupObjectBlock sob;
					THROW(pPack->SetupObject(ar_list.get(0), sob));
				}
			}
			else if(pPack->Rec.EdiOp == PPEDIOP_ORDER) {
				PPID   main_org_id = 0;
				for(uint i = 0; !main_org_id && i < psn_list_by_gln.getCount(); i++) {
					const PPID _id = psn_list_by_gln.get(i);
					if(PsnObj.P_Tbl->IsBelongToKind(_id, PPPRK_MAIN) > 0)
						main_org_id = _id;
				}
				THROW_PP_S(main_org_id, PPERR_EDI_UNBLRSLV_BILLMAINORG, msg_buf);
			}
		}
		else if(partyQ == EDIPARTYQ_BUYER) {
			if(pPack->Rec.EdiOp == PPEDIOP_DESADV) {
				PPID   main_org_id = 0;
				for(uint i = 0; !main_org_id && i < psn_list_by_gln.getCount(); i++) {
					const PPID _id = psn_list_by_gln.get(i);
					if(PsnObj.P_Tbl->IsBelongToKind(_id, PPPRK_MAIN) > 0)
						main_org_id = _id;
				}
				THROW_PP_S(main_org_id, PPERR_EDI_UNBLRSLV_BILLMAINORG, msg_buf);
			}
			else if(pPack->Rec.EdiOp == PPEDIOP_ORDER) {
				THROW_PP_S(psn_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
				THROW(ArObj.GetByPersonList(GetSellAccSheet(), &psn_list_by_gln, &ar_list));
				THROW_PP_S(ar_list.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
				{
					PPBillPacket::SetupObjectBlock sob;
					THROW(pPack->SetupObject(ar_list.get(0), sob));
				}
			}
		}
		else if(partyQ == EDIPARTYQ_INVOICEE) {
		}
		else if(oneof2(partyQ, EDIPARTYQ_SHIPTO, EDIPARTYQ_CONSIGNEE)) {
			if(pPack->Rec.EdiOp == PPEDIOP_DESADV) {
				int lr = ResolveDlvrLoc(rC, pPack);
				if(!lr) {
				}
			}
			else if(pPack->Rec.EdiOp == PPEDIOP_ORDER) {
				int lr = ResolveDlvrLoc(rC, pPack);
				if(!lr) {
				}
			}
		}
		else if(partyQ == EDIPARTYQ_CONSIGNOR) {
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::ReadCommonAttributes(const xmlNode * pNode, OwnFormatCommonAttr & rA)
{
	int    ok = -1;
	SString temp_buf;
	rA.Clear();
	if(SXml::GetAttrib(pNode, "id", rA.Id))
		ok = 1;
	if(SXml::GetAttrib(pNode, "number", rA.Num)) {
		rA.Num.Transf(CTRANSF_UTF8_TO_INNER);
		ok = 1;
	}
	if(SXml::GetAttrib(pNode, "status", rA.Status))
		ok = 1;
	if(SXml::GetAttrib(pNode, "revisionNumber", rA.Revision))
		ok = 1;
	if(SXml::GetAttrib(pNode, "unitOfMeasure", rA.UOM))
		ok = 1;
	if(SXml::GetAttrib(pNode, "date", temp_buf) > 0) {
		rA.Dt = strtodate_(temp_buf, DATF_DMY);
		ok = 1;
	}
	return ok;
}

int EdiProviderImplementation_Kontur::WriteOwnFormatContractor(SXml::WDoc & rDoc, PPID personID, PPID locID)
{
	int    ok = 1;
	SString temp_buf;
	if(locID) {
		THROW(GetLocGLN(locID, temp_buf));
		SXml::WNode n_gln(rDoc, "gln", temp_buf);
	}
	else if(personID) {
		THROW(GetPersonGLN(personID, temp_buf));
		SXml::WNode n_gln(rDoc, "gln", temp_buf);
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::ReadOwnFormatContractor(xmlNode * pNode, OwnFormatContractor & rC)
{
	int    ok = 1;
	SString temp_buf;
	rC.Z();
	for(xmlNode * p_cn = pNode; p_cn; p_cn = p_cn->next) {
		if(SXml::GetContentByName(p_cn, "gln", temp_buf)) {
			rC.GLN = temp_buf;
		}
		else if(SXml::GetContentByName(p_cn, "additionalIdentificator", temp_buf)) {
		}
		else if(SXml::GetContentByName(p_cn, "additionalnfo", temp_buf)) {
			for(xmlNode * p_n = p_cn->children; p_n; p_n = p_n->next) {
			}
		}
		else if(SXml::GetContentByName(p_cn, "taxSystem", temp_buf)) {
		}
		else if(SXml::IsName(p_cn, "organization")) {
			for(xmlNode * p_n = p_cn->children; p_n; p_n = p_n->next) {
				if(SXml::GetContentByName(p_n, "name", temp_buf))
					rC.Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "inn", temp_buf))
					rC.INN = temp_buf;
				else if(SXml::GetContentByName(p_n, "kpp", temp_buf))
					rC.KPP = temp_buf;
			}
		}
		else if(SXml::IsName(p_cn, "foreignAddress")) {
			for(xmlNode * p_n = p_cn->children; p_n; p_n = p_n->next) {
				if(SXml::GetContentByName(p_n, "countryISOCode", temp_buf))
					STRNSCPY(rC.Addr.CountryCode, temp_buf);
				else if(SXml::GetContentByName(p_n, "address", temp_buf))
					rC.Addr.AddressText = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
		else if(SXml::IsName(p_cn, "russianAddress")) {
			for(xmlNode * p_n = p_cn->children; p_n; p_n = p_n->next) {
				if(SXml::GetContentByName(p_n, "regionISOCode", temp_buf))
					rC.Addr.RegionIsoCode = temp_buf;
				else if(SXml::GetContentByName(p_n, "district", temp_buf))
					rC.Addr.District = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "city", temp_buf))
					rC.Addr.City = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "settlement", temp_buf))
					rC.Addr.Settlement = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "street", temp_buf))
					rC.Addr.Street = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "house", temp_buf))
					rC.Addr.House = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "flat", temp_buf))
					rC.Addr.Flat = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				else if(SXml::GetContentByName(p_n, "postalCode", temp_buf))
					rC.Addr.ZIP = temp_buf;
			}
		}
	}
	return ok;
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_ORDERS(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "ORDERS");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "1");
	}
	{
		SXml::WNode n_b(_doc, "order"); // <order number="OR012012552011" date="2014-02-07" status="Replace" revisionNumber="02">
		// @v11.1.12 n_b.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
		n_b.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		n_b.PutAttrib("status", "Original");
		{
			SXml::WNode n_i(_doc, "proposalOrdersIdentificator"); // <proposalOrdersIdentificator number="001" date="2014-02-06"/>
		}
		n_b.PutInnerSkipEmpty("promotionDealNumber", ""); // <!--номер промоакции-->
		{
			SXml::WNode n_i(_doc, "contractIdentificator"); // <contractIdentificator number="357951" date="2012-05-06"/>
		}
		n_b.PutInnerSkipEmpty("blanketOrderIdentificator", ""); // <blanketOrderIdentificator number="11212500345"/> <!--номер серии заказов-->
		{
			SXml::WNode n_i(_doc, "seller");
			THROW(WriteOwnFormatContractor(_doc, ObjectToPerson(rBp.Rec.Object), 0));
		}
		{
			SXml::WNode n_i(_doc, "buyer");
			THROW(WriteOwnFormatContractor(_doc, MainOrgID, 0));
		}
		/*{
			SXml::WNode n_i(_doc, "invoicee");
		}*/
		{
			SXml::WNode n_i(_doc, "deliveryInfo");
			if(checkdate(rBp.Rec.DueDate, 0)) {
				LDATETIME temp_dtm;
				temp_dtm.Set(rBp.Rec.DueDate, ZEROTIME);
				n_i.PutInner("requestedDeliveryDateTime", temp_buf.Z().Cat(temp_dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
			}
			{
				SXml::WNode n_i2(_doc, "shipTo");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.Rec.LocID));
			}
		}
		// @v11.1.12 n_b.PutInnerSkipEmpty("comment", temp_buf.Z().Cat(rBp.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutInnerSkipEmpty("comment", (temp_buf = rBp.SMemo).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12
		{
			SString goods_code;
			SString goods_ar_code;
			Goods2Tbl::Rec goods_rec;
			SXml::WNode n_dtl(_doc, "lineItems");
			n_dtl.PutInner("currencyISOCode", "RUB");
			double total_amount = 0.0;
			for(uint i = 0; i < rBp.GetTCount(); i++) {
				const PPTransferItem & r_ti = rBp.ConstTI(i);
				SXml::WNode n_item(_doc, "lineItem");
				const PPID goods_id = labs(r_ti.GoodsID);
				double qtty = fabs(r_ti.Quantity_);
				double cost = r_ti.Cost;
				double amount = cost * qtty;
				THROW(GetGoodsInfo(goods_id, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
				n_item.PutInner("gtin", goods_code);
				n_item.PutInner("internalBuyerCode", temp_buf.Z().Cat(goods_rec.ID));
				if(goods_ar_code.NotEmpty()) {
					n_item.PutInner("internalSupplierCode", (temp_buf = goods_ar_code).Transf(CTRANSF_INNER_TO_UTF8));
				}
				n_item.PutInner("lineNumber", temp_buf.Z().Cat(r_ti.RByBill));
				//n_item.PutInnerSkipEmpty("comment", "");
				{
					SXml::WNode n_q(_doc, "requestedQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				n_item.PutInnerSkipEmpty("flowType", "Direct"); // Тип поставки, может принимать значения: Stock - сток до РЦ, Transit - транзит в магазин, Direct - прямая поставка, Fresh - свежие продукты
				//n_item.PutInner("netPrice", "");
				n_item.PutInner("netPriceWithVAT", temp_buf.Z().Cat(cost, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				//n_item.PutInner("netAmount", "");
				n_item.PutInner("amount", temp_buf.Z().Cat(amount, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				total_amount += amount;
			}
			n_dtl.PutInner("totalAmount", temp_buf.Z().Cat(total_amount, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::IdentifyOrderRspStatus(const PPBillPacket & rBp, const PPBillPacket * pExtBp)
{
	int    status = 1;
	if(pExtBp && pExtBp != &rBp) {
		int    all_rejected = 1;
		const  PPBillPacket & r_org_pack = DEREFPTROR(pExtBp, rBp);
		for(uint i = 0; i < r_org_pack.GetTCount(); i++) {
			uint   current_ti_pos = 0;
			const PPTransferItem & r_ti = r_org_pack.ConstTI(i);
			const PPTransferItem * p_current_ti = rBp.SearchTI(r_ti.RByBill, &current_ti_pos) ? &rBp.ConstTI(current_ti_pos) : 0;
			const  PPID   goods_id = labs(r_ti.GoodsID);
			PPID   confirm_goods_id = p_current_ti ? labs(p_current_ti->GoodsID) : goods_id;
			double qtty = fabs(r_ti.Quantity_);
			double confirm_qtty = p_current_ti ? fabs(p_current_ti->Quantity_) : 0.0;
			double price = (r_ti.Price - r_ti.Discount);
			double confirm_price = p_current_ti ? (p_current_ti->Price - p_current_ti->Discount) : price;
			if(confirm_qtty != 0.0) {
				all_rejected = 0;
				if(confirm_qtty != qtty || confirm_price != price || confirm_goods_id != goods_id)
					status = 3;
			}
		}
		if(all_rejected) {
			status = 2;
		}
	}
	return status;
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_ORDERRSP(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp, const PPBillPacket * pExtBp)
{
	int    ok = 1;
	const  PPBillPacket & r_org_pack = DEREFPTROR(pExtBp, rBp);
	SString temp_buf;
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "ORDRSP");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "1");
	}
	{
		SXml::WNode n_b(_doc, "orderResponse"); // <order number="OR012012552011" date="2014-02-07" status="Replace" revisionNumber="02">
		// @v11.1.12 n_b.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
		n_b.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		{
			int ordrsp_status = IdentifyOrderRspStatus(rBp, pExtBp);
			const char * p_status_text = 0;
			switch(ordrsp_status) {
				case 1: p_status_text = "Accepted"; break;
				case 2: p_status_text = "Rejected"; break;
				case 3: p_status_text = "Changed"; break;
			}
			assert(!isempty(p_status_text));
			n_b.PutAttrib("status", p_status_text); // [Accepted,Rejected,Changed]
		}
		{
			SXml::WNode n_i(_doc, "originOrder");
			// @v11.1.12 n_i.PutAttrib("number", BillCore::GetCode(temp_buf = r_org_pack.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
			n_i.PutAttrib("number", (temp_buf = r_org_pack.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
			n_i.PutAttrib("date", temp_buf.Z().Cat(r_org_pack.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		}
		{
			//SXml::WNode n_i(_doc, "proposalOrdersIdentificator"); // <proposalOrdersIdentificator number="001" date="2014-02-06"/>
		}
		//n_b.PutInnerSkipEmpty("promotionDealNumber", ""); // <!--номер промоакции-->
		{
			//SXml::WNode n_i(_doc, "contractIdentificator"); // <contractIdentificator number="357951" date="2012-05-06"/>
		}
		n_b.PutInnerSkipEmpty("blanketOrderIdentificator", ""); // <blanketOrderIdentificator number="11212500345"/> <!--номер серии заказов-->
		{
			SXml::WNode n_i(_doc, "seller");
			THROW(WriteOwnFormatContractor(_doc, MainOrgID, 0));
		}
		{
			SXml::WNode n_i(_doc, "buyer");
			THROW(WriteOwnFormatContractor(_doc, ObjectToPerson(rBp.Rec.Object), 0));
		}
		/*{
			SXml::WNode n_i(_doc, "invoicee");
		}*/
		{
			LDATETIME temp_dtm;
			SXml::WNode n_i(_doc, "deliveryInfo");
			if(checkdate(r_org_pack.Rec.DueDate, 0)) {
				temp_dtm.Set(r_org_pack.Rec.DueDate, ZEROTIME);
				n_i.PutInner("requestedDeliveryDateTime", temp_buf.Z().Cat(temp_dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
			}
			if(checkdate(rBp.Rec.DueDate, 0)) {
				temp_dtm.Set(rBp.Rec.DueDate, ZEROTIME);
				n_i.PutInner("estimatedDeliveryDateTime", temp_buf.Z().Cat(temp_dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
			}
			if(r_org_pack.GetDlvrAddrID()) {
				SXml::WNode n_i2(_doc, "shipTo");
				THROW(WriteOwnFormatContractor(_doc, 0, r_org_pack.GetDlvrAddrID()));
			}
			{
				SXml::WNode n_i2(_doc, "shipFrom");
				THROW(WriteOwnFormatContractor(_doc, 0, r_org_pack.Rec.LocID));
			}
		}
		// @v11.1.12 n_b.PutInnerSkipEmpty("comment", temp_buf.Z().Cat(rBp.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutInnerSkipEmpty("comment", (temp_buf = rBp.SMemo).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12
		{
			SString goods_code;
			SString goods_ar_code;
			Goods2Tbl::Rec goods_rec;
			SXml::WNode n_dtl(_doc, "lineItems");
			n_dtl.PutInner("currencyISOCode", "RUB");
			double total_amount = 0.0;
			for(uint i = 0; i < r_org_pack.GetTCount(); i++) {
				uint   current_ti_pos = 0;
				const PPTransferItem & r_ti = r_org_pack.ConstTI(i);
				const PPTransferItem * p_current_ti = rBp.SearchTI(r_ti.RByBill, &current_ti_pos) ? &rBp.ConstTI(current_ti_pos) : 0;
				const  PPID   goods_id = labs(r_ti.GoodsID);
				const  PPID   confirm_goods_id = p_current_ti ? labs(p_current_ti->GoodsID) : goods_id;
				double qtty = fabs(r_ti.Quantity_);
				double confirm_qtty = p_current_ti ? fabs(p_current_ti->Quantity_) : 0.0;
				double price = (r_ti.Price - r_ti.Discount);
				double confirm_price = p_current_ti ? (p_current_ti->Price - p_current_ti->Discount) : price;
				double amount = confirm_price * confirm_qtty;
				SXml::WNode n_item(_doc, "lineItem");
				const char * p_line_status = "Accepted";
				if(confirm_qtty == 0.0)
					p_line_status = "Rejected";
				else if(confirm_qtty != qtty || confirm_price != price || confirm_goods_id != goods_id)
					p_line_status = "Changed";
				n_item.PutAttrib("status", p_line_status);
				THROW(GetGoodsInfo(confirm_goods_id, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
				n_item.PutInner("gtin", goods_code);
				// @debug n_item.PutInner("internalSupplierCode", temp_buf.Z().Cat(goods_rec.ID));
				if(goods_ar_code.NotEmpty()) {
					n_item.PutInner("internalBuyerCode", (temp_buf = goods_ar_code).Transf(CTRANSF_INNER_TO_UTF8));
				}
				n_item.PutInner("lineNumber", temp_buf.Z().Cat(r_ti.RByBill));
				//n_item.PutInnerSkipEmpty("comment", "");
				{
					SXml::WNode n_q(_doc, "orderedQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				{
					SXml::WNode n_q(_doc, "confirmedQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(confirm_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				//n_item.PutInnerSkipEmpty("flowType", "Direct"); // Тип поставки, может принимать значения: Stock - сток до РЦ, Transit - транзит в магазин, Direct - прямая поставка, Fresh - свежие продукты
				//n_item.PutInner("netPrice", "");
				n_item.PutInner("netPriceWithVAT", temp_buf.Z().Cat(confirm_price, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				//n_item.PutInner("netAmount", "");
				n_item.PutInner("amount", temp_buf.Z().Cat(amount, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				total_amount += amount;
			}
			n_dtl.PutInner("totalAmount", temp_buf.Z().Cat(total_amount, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS)));
		}
	}
	CATCHZOK
	return ok;
}

void EdiProviderImplementation_Kontur::Write_OwnFormat_OriginOrder_Tag(SXml::WDoc & rDoc, const BillTbl::Rec & rOrderBillRec)
{
	SString temp_buf;
	SXml::WNode n_i(rDoc, "originOrder"); // <originOrder number="ORSP0012" date="2014-02-07"/>
	// @v11.1.12 n_i.PutAttrib("number", BillCore::GetCode(temp_buf = rOrderBillRec.Code).Transf(CTRANSF_INNER_TO_UTF8));
	n_i.PutAttrib("number", (temp_buf = rOrderBillRec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
	n_i.PutAttrib("date", temp_buf.Z().Cat(rOrderBillRec.Dt, DATF_ISO8601|DATF_CENTURY));
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_DESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "DESADV");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "0");
	}
	{
		BillTbl::Rec order_bill_rec;
		// @v10.6.4 MEMSZERO(order_bill_rec);
		SXml::WNode n_b(_doc, "despatchAdvice"); // <despatchAdvice number="DES003" date="2014-02-07" status="Original">
		// @v11.1.12 n_b.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
		n_b.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		n_b.PutAttrib("status", "Original");
		{
			const int goobr = GetOriginOrderBill(rBp, &order_bill_rec);
			THROW(goobr);
			THROW_PP_S(goobr > 0, PPERR_EDI_DESADV_NOORDER, bill_text);
			Write_OwnFormat_OriginOrder_Tag(_doc, order_bill_rec);
		}
		{
			//SXml::WNode n_i(_doc, "orderResponse"); // <orderResponse number="ORSP0012" date="2014-02-07"/>
		}
		{
			//SXml::WNode n_i(_doc, "blanketOrderIdentificator"); // <blanketOrderIdentificator number="11212500345"/> <!--номер серии заказов-->
		}
		{
			//SXml::WNode n_i(_doc, "egaisRegistrationIdentificator"); // <egaisRegistrationIdentificator number="123_TEST"/>
		}
		{
			//SXml::WNode n_i(_doc, "egaisFixationIdentificator"); // <egaisFixationIdentificator number="12223" date="2016-05-04"/>
		}
		{
			//SXml::WNode n_i(_doc, "deliveryNoteIdentificator"); // <deliveryNoteIdentificator number="13245" date="2014-02-07"/> номер и дата ТОРГ-12
		}
		{
			SXml::WNode n_i(_doc, "seller");
			THROW(WriteOwnFormatContractor(_doc, MainOrgID, 0));
		}
		{
			SXml::WNode n_i(_doc, "buyer");
			THROW(WriteOwnFormatContractor(_doc, ObjectToPerson(rBp.Rec.Object), 0));
		}
		{
			//SXml::WNode n_i(_doc, "invoicee");
		}
		{
			SXml::WNode n_i(_doc, "deliveryInfo");
			// @v10.0.10 {
			if(checkdate(order_bill_rec.DueDate, 0)) {
				LDATETIME temp_dtm;
				temp_dtm.Set(order_bill_rec.DueDate, ZEROTIME);
				n_i.PutInner("estimatedDeliveryDateTime", temp_buf.Z().Cat(temp_dtm, DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
			}
			// } @v10.0.10
			{
				SXml::WNode n_i2(_doc, "shipFrom");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.Rec.LocID));
			}
			if(rBp.GetDlvrAddrID()) {
				SXml::WNode n_i2(_doc, "shipTo");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.GetDlvrAddrID()));
			}
		}
		// @v11.1.12 n_b.PutInnerSkipEmpty("comment", temp_buf.Z().Cat(rBp.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8)); // <!--номер промоакции-->
		n_b.PutInnerSkipEmpty("comment", (temp_buf = rBp.SMemo).Transf(CTRANSF_INNER_TO_UTF8)); // <!--номер промоакции--> // @v11.1.12
		{
			SString goods_code;
			SString goods_ar_code;
			Goods2Tbl::Rec goods_rec;
			double total_amount_with_vat = 0.0;
			double total_amount_without_vat = 0.0;
			SXml::WNode n_dtl(_doc, "lineItems");
			n_dtl.PutInner("currencyISOCode", "RUB");
			for(uint i = 0; i < rBp.GetTCount(); i++) {
				const PPTransferItem & r_ti = rBp.ConstTI(i);
				SXml::WNode n_item(_doc, "lineItem");
				double qtty = fabs(r_ti.Quantity_);
				//
					GTaxVect vect;
					vect.CalcTI(r_ti, rBp.Rec.OpID, TIAMT_PRICE);
					const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
					const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
					const double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
					const double price_with_vat = R5(amount_with_vat / qtty);
					const double price_without_vat = R5(amount_without_vat / qtty);
				//
				THROW(GetGoodsInfo(r_ti.GoodsID, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
				n_item.PutInner("gtin", goods_code);
				n_item.PutInner("internalSupplierCode", temp_buf.Z().Cat(goods_rec.ID));
				//n_item.PutInnerSkipEmpty("codeOfEgais", ""); // <!--код товара в ЕГАИС-->
				//n_item.PutInnerSkipEmpty("lotNumberEgais", ""); // <!--номер товара в ТТН ЕГАИС-->
				//n_item.PutInnerSkipEmpty("orderLineNumber", ""); // <!--порядковый номер товара-->
				// n_item.PutInner("lineNumber", temp_buf.Z().Cat(r_ti.RByBill));
				//n_item.PutInnerSkipEmpty("comment", "");
				{
					double ord_qtty = 0.0;
					if(r_ti.Flags & PPTFR_ONORDER) {
						uint sh_lot_pos = 0;
						if(rBp.SearchShLot(r_ti.OrdLotID, &sh_lot_pos)) {
							ReceiptTbl::Rec ord_lot_rec;
							if(P_BObj->trfr->Rcpt.Search(r_ti.OrdLotID, &ord_lot_rec) > 0)
								ord_qtty = fabs(ord_lot_rec.Quantity);
						}
					}
					if(ord_qtty > 0.0) {
						SXml::WNode n_q(_doc, "orderedQuantity");
						n_q.PutAttrib("unitOfMeasure", "PCE");
						n_q.SetValue(temp_buf.Z().Cat(ord_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
					}
				}
				{
					SXml::WNode n_q(_doc, "despatchedQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				if(r_ti.UnitPerPack > 0.0) {
					SXml::WNode n_q(_doc, "onePlaceQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(fabs(r_ti.UnitPerPack), MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				if(checkdate(r_ti.Expiry, 0))
					n_item.PutInner("expireDate", temp_buf.Z().Cat(r_ti.Expiry, DATF_ISO8601|DATF_CENTURY));
				n_item.PutInner("netPrice", temp_buf.Z().Cat(price_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // цена по позиции без НДС
				n_item.PutInner("netPriceWithVAT", temp_buf.Z().Cat(price_with_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // цена по позиции с НДС
				n_item.PutInner("netAmount", temp_buf.Z().Cat(amount_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // сумма по позиции без НДС
				n_item.PutInner("amount", temp_buf.Z().Cat(amount_with_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // сумма по позиции с НДС
				n_item.PutInner("vATRate", temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // ставка НДС
				total_amount_with_vat += amount_with_vat;
				total_amount_without_vat += amount_without_vat;
			}
			n_dtl.PutInner("totalSumExcludingTaxes", temp_buf.Z().Cat(total_amount_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Общая сумма отгруженных товарных позиций без НДС
			//n_dtl.PutInner("totalVATAmount", ""); //  Общая сумма НДС по всему документу
			n_dtl.PutInner("totalAmount", temp_buf.Z().Cat(total_amount_with_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Общая сумма с НДС по документу
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_ALCODESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "ALCRPT");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "0");
	}
	{
		SXml::WNode n_b(_doc, "alcoholReport"); // <despatchAdvice number="DES003" date="2014-02-07" status="Original">
		// @v11.1.12 n_b.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
		n_b.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
		n_b.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		n_b.PutAttrib("status", "Original");
		{
			BillTbl::Rec order_bill_rec;
			const int goobr = GetOriginOrderBill(rBp, &order_bill_rec);
			THROW(goobr);
			THROW_PP_S(goobr > 0, PPERR_EDI_DESADV_NOORDER, bill_text);
			Write_OwnFormat_OriginOrder_Tag(_doc, order_bill_rec);
		}
		{
			//SXml::WNode n_i(_doc, "orderResponse"); // <orderResponse number="ORSP0012" date="2014-02-07"/>
		}
		{
			//SXml::WNode n_i(_doc, "blanketOrderIdentificator"); // <blanketOrderIdentificator number="11212500345"/> <!--номер серии заказов-->
		}
		{
			//SXml::WNode n_i(_doc, "egaisRegistrationIdentificator"); // <egaisRegistrationIdentificator number="123_TEST"/>
		}
		{
			//SXml::WNode n_i(_doc, "egaisFixationIdentificator"); // <egaisFixationIdentificator number="12223" date="2016-05-04"/>
		}
		{
			//SXml::WNode n_i(_doc, "deliveryNoteIdentificator"); // <deliveryNoteIdentificator number="13245" date="2014-02-07"/> номер и дата ТОРГ-12
		}
		{
			SXml::WNode n_i(_doc, "seller");
			THROW(WriteOwnFormatContractor(_doc, MainOrgID, 0));
		}
		{
			SXml::WNode n_i(_doc, "buyer");
			THROW(WriteOwnFormatContractor(_doc, ObjectToPerson(rBp.Rec.Object), 0));
		}
		{
			//SXml::WNode n_i(_doc, "invoicee");
		}
		{
			SXml::WNode n_i(_doc, "deliveryInfo");
			{
				//<waybill number="382419/40000" date="2013-02-22"/>
				SXml::WNode n_i2(_doc, "waybill");
				// @v11.1.12 n_i2.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
				n_i2.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
				n_i2.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
			}
			{
				SXml::WNode n_i2(_doc, "shipFrom");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.Rec.LocID));
			}
			if(rBp.GetDlvrAddrID()) {
				SXml::WNode n_i2(_doc, "shipTo");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.GetDlvrAddrID()));
			}
		}
		// @v11.1.12 n_b.PutInnerSkipEmpty("comment", temp_buf.Z().Cat(rBp.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8)); // <!--номер промоакции-->
		n_b.PutInnerSkipEmpty("comment", (temp_buf = rBp.SMemo).Transf(CTRANSF_INNER_TO_UTF8)); // <!--номер промоакции--> // @v11.1.12
		{
			const int use_refc_data = Arp.UseOwnEgaisObjects();
			SString goods_code;
			SString goods_ar_code;
			Goods2Tbl::Rec goods_rec;
			PrcssrAlcReport::GoodsItem agi;
			SXml::WNode n_dtl(_doc, "lineItems");
			for(uint i = 0; i < rBp.GetTCount(); i++) {
				const PPTransferItem & r_ti = rBp.ConstTI(i);
				if(Arp.IsAlcGoods(r_ti.GoodsID) && Arp.PreprocessGoodsItem(r_ti.GoodsID, 0, 0, 0, agi) > 0) {
					SXml::WNode n_item(_doc, "lineItem");
					double qtty = fabs(r_ti.Quantity_);
					THROW(GetGoodsInfo(r_ti.GoodsID, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
					n_item.PutInner("gtin", goods_code);
					n_item.PutInner("internalSupplierCode", temp_buf.Z().Cat(goods_rec.ID));
					n_item.PutInnerSkipEmpty("internalBuyerCode", (temp_buf = goods_ar_code).Transf(CTRANSF_INNER_TO_UTF8));
					{
						SXml::WNode n_lot(_doc, "lot");
						n_lot.PutInner("description", temp_buf.Z().Cat(goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
						n_lot.PutInnerSkipEmpty("codeOfEgais", temp_buf.Z().Cat(agi.EgaisCode).Transf(CTRANSF_INNER_TO_UTF8));
						if(agi.UnpackedVolume > 0) {
							SXml::WNode n_vol(_doc, "volume");
							n_vol.PutAttrib("volumeTypeQualifier", "acceptedVolume");
							n_vol.PutAttrib("unitOfMeasure", "DKL");
							{
								const double mult = agi.UnpackedVolume / 10.0;
								n_vol.SetValue(temp_buf.Z().Cat(qtty * mult, MKSFMTD(0, 4, 0)));
							}
						}
						else {
							const double volume = (use_refc_data && agi.RefcVolume) ? agi.RefcVolume : agi.Volume;
							{
								SXml::WNode n_vol(_doc, "volume");
								n_vol.PutAttrib("volumeTypeQualifier", "acceptedVolume");
								n_vol.PutAttrib("unitOfMeasure", "PCE");
								n_vol.SetValue(temp_buf.Z().Cat(qtty, MKSFMTD(0, 4, NMBF_NOTRAILZ)));
							}
							n_lot.PutInner("volumeOfUnitInLiter", temp_buf.Z().Cat(volume, MKSFMTD(0, 4, NMBF_NOTRAILZ)));
						}
						{
							EgaisPersonCore::Item epr_item;
							if(use_refc_data && Arp.GetEgaisPersonByCode(agi.RefcManufCode, epr_item) > 0) {
								SXml::WNode n_mf(_doc, "manufacturer");
								if(epr_item.INN[0]) {
									SXml::WNode n_org(_doc, "organization");
									n_org.PutInner("inn", epr_item.INN);
									n_org.PutInnerSkipEmpty("kpp", epr_item.KPP);
									temp_buf.Z().Cat(epr_item.Name).ReplaceChar('\x07', ' ');
									XMLReplaceSpecSymb(temp_buf, "&<>\'");
									n_org.PutInner("name", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
								}
								else {
									SXml::WNode n_org(_doc, "foreignOrganization");
									n_org.PutAttrib("countryCode", temp_buf.Z().Cat(epr_item.CountryCode));
									temp_buf.Z().Cat(epr_item.Name).ReplaceChar('\x07', ' ');
									XMLReplaceSpecSymb(temp_buf, "&<>\'");
									n_org.PutInner("name", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
								}
							}
							else {
								PPID   manuf_id = 0;
								PersonTbl::Rec manuf_psn_rec;
								if(Arp.GetLotManufID(r_ti.LotID, &manuf_id, 0) > 0 && PsnObj.Fetch(manuf_id, &manuf_psn_rec) > 0) {
									SXml::WNode n_mf(_doc, "manufacturer");
									if(PsnObj.GetRegNumber(manuf_id, PPREGT_TPID, rBp.Rec.Dt, temp_buf) > 0) {
										SXml::WNode n_org(_doc, "organization");
										n_org.PutInner("inn", temp_buf);
										if(PsnObj.GetRegNumber(manuf_id, PPREGT_KPP, rBp.Rec.Dt, temp_buf) > 0) {
											n_org.PutInnerSkipEmpty("kpp", temp_buf);
										}
										temp_buf.Z().Cat(manuf_psn_rec.Name).ReplaceChar('\x07', ' ');
										XMLReplaceSpecSymb(temp_buf, "&<>\'");
										n_org.PutInner("name", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
									}
								}
							}
						}
						{
							RegisterTbl::Rec lic_reg_rec;
							if(Arp.GetWkrRegister(Arp.wkrAlcLic, MainOrgID, rBp.Rec.LocID, rBp.Rec.Dt, &lic_reg_rec) > 0) {
								SXml::WNode n_lic(_doc, "licenseSeller");
								n_lic.PutAttrib("seriesNumber", (temp_buf = lic_reg_rec.Num).Transf(CTRANSF_INNER_TO_UTF8));
								if(checkdate(lic_reg_rec.Dt, 0)) {
									n_lic.PutAttrib("startdate", temp_buf.Z().Cat(lic_reg_rec.Dt, DATF_ISO8601|DATF_CENTURY));
								}
								if(checkdate(lic_reg_rec.Expiry, 0)) {
									n_lic.PutAttrib("enddate", temp_buf.Z().Cat(lic_reg_rec.Expiry, DATF_ISO8601|DATF_CENTURY));
								}
								{
									PersonTbl::Rec lic_org_rec;
									if(lic_reg_rec.RegOrgID && PsnObj.Fetch(lic_reg_rec.RegOrgID, &lic_org_rec) > 0) {
										n_lic.PutAttrib("issuerOf", temp_buf.Z().Cat(lic_org_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
									}
									else {
										n_lic.PutAttrib("issuerOf", temp_buf.Z().Cat("Somebody").Transf(CTRANSF_INNER_TO_UTF8));
									}
								}
							}
						}
						temp_buf = (use_refc_data && agi.RefcCategoryCode.NotEmpty()) ? agi.RefcCategoryCode : agi.CategoryCode;
						n_lot.PutInner("typeOfAlco", temp_buf);
						if(checkdate(agi.BottlingDate, 0)) {
							n_lot.PutInner("bottlingDate", temp_buf.Z().Cat(agi.BottlingDate, DATF_ISO8601|DATF_CENTURY));
						}
						if(rBp.LTagL.GetTagStr(i, PPTAG_LOT_CLB, temp_buf) > 0) {
							n_lot.PutInner("customsDeclarationNumber", temp_buf);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_RECADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPEdiProcessor::RecadvPacket & rRaPack)
{
	int    ok = 0;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rRaPack.RBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rRaPack.RBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "RECADV");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "0");
	}

	//
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::Write_OwnFormat_INVOIC(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "eDIMessage"); // <eDIMessage id="0000015137">
	rIdent.ToStr(S_GUID::fmtIDL, temp_buf);
	n_docs.PutAttrib("id", temp_buf);
	{
		SXml::WNode n_hdr(_doc, "interchangeHeader");
		THROW(GetMainOrgGLN(temp_buf));
		n_hdr.PutInner("sender", temp_buf);
		THROW(GetArticleGLN(rBp.Rec.Object, temp_buf));
		n_hdr.PutInner("recipient", temp_buf);
		n_hdr.PutInner("documentType", "INVOIC");
		n_hdr.PutInner("creationDateTime", temp_buf.Z().CatCurDateTime(DATF_ISO8601|DATF_CENTURY, TIMF_HMS));
		//n_hdr.PutInner("isTest", "0");
	}
	{
		SXml::WNode n_b(_doc, "invoice"); // <despatchAdvice number="DES003" date="2014-02-07" status="Original">
		if(rBp.Ext.InvoiceCode[0])
			temp_buf = rBp.Ext.InvoiceCode;
		else {
			// @v11.1.12 BillCore::GetCode(temp_buf = rBp.Rec.Code);
			temp_buf = rBp.Rec.Code; // @v11.1.12 
		}
		temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
		n_b.PutAttrib("number", temp_buf);
		if(checkdate(rBp.Ext.InvoiceDate, 0))
			temp_buf.Z().Cat(rBp.Ext.InvoiceDate, DATF_ISO8601|DATF_CENTURY);
		else
			temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY);
		n_b.PutAttrib("date", temp_buf);
		n_b.PutAttrib("status", "Original");
		{
			BillTbl::Rec order_bill_rec;
			const int goobr = GetOriginOrderBill(rBp, &order_bill_rec);
			THROW(goobr);
			THROW_PP_S(goobr > 0, PPERR_EDI_DESADV_NOORDER, bill_text);
			Write_OwnFormat_OriginOrder_Tag(_doc, order_bill_rec);
		}
		{
			//<despatchIdentificator number="DES003" date="2014-02-07"/>
			SXml::WNode n_i(_doc, "despatchIdentificator");
			// @v11.1.12 n_i.PutAttrib("number", BillCore::GetCode(temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8));
			n_i.PutAttrib("number", (temp_buf = rBp.Rec.Code).Transf(CTRANSF_INNER_TO_UTF8)); // @v11.1.12 
			n_i.PutAttrib("date", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		}
		if(rBp.BTagL.GetItemStr(PPTAG_BILL_EDIRECADVRCV, temp_buf) > 0 && temp_buf.NotEmpty()) {
			StringSet ss;
			temp_buf.Tokenize(" ", ss);
			if(ss.getCount() >= 3) {
				uint ssp = 0;
				if(ss.get(&ssp, temp_buf)) {
					if(ss.get(&ssp, temp_buf)) {
						SXml::WNode n_i(_doc, "receivingIdentificator");
						n_i.PutAttrib("number", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
						if(ss.get(&ssp, temp_buf))
							n_i.PutAttrib("date", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
					}
				}
			}
		}
		{
			SXml::WNode n_i(_doc, "seller");
			THROW(WriteOwnFormatContractor(_doc, MainOrgID, 0));
		}
		{
			SXml::WNode n_i(_doc, "buyer");
			THROW(WriteOwnFormatContractor(_doc, ObjectToPerson(rBp.Rec.Object), 0));
		}
		{
			//SXml::WNode n_i(_doc, "invoicee");
		}
		{
			SXml::WNode n_i(_doc, "deliveryInfo");
			{
				SXml::WNode n_i2(_doc, "shipFrom");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.Rec.LocID));
			}
			if(rBp.GetDlvrAddrID()) {
				SXml::WNode n_i2(_doc, "shipTo");
				THROW(WriteOwnFormatContractor(_doc, 0, rBp.GetDlvrAddrID()));
			}
		}
		// @v11.1.12 n_b.PutInnerSkipEmpty("comment", EncXmlText(rBp.Rec.Memo)); // <!--номер промоакции-->
		n_b.PutInnerSkipEmpty("comment", EncXmlText(temp_buf = rBp.SMemo)); // <!--номер промоакции--> // @v11.1.12
		{
			SString goods_code;
			SString goods_ar_code;
			Goods2Tbl::Rec goods_rec;
			double total_amount_with_vat = 0.0;
			double total_amount_without_vat = 0.0;
			double total_vat_amount = 0.0;
			SXml::WNode n_dtl(_doc, "lineItems");
			n_dtl.PutInner("currencyISOCode", "RUB");
			for(uint i = 0; i < rBp.GetTCount(); i++) {
				const PPTransferItem & r_ti = rBp.ConstTI(i);
				SXml::WNode n_item(_doc, "lineItem");
				double qtty = fabs(r_ti.Quantity_);
				//
					GTaxVect vect;
					vect.CalcTI(r_ti, rBp.Rec.OpID, TIAMT_PRICE);
					const double amount_with_vat = vect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
					const double amount_without_vat = vect.GetValue(GTAXVF_AFTERTAXES);
					const double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
					const double vat_amount = vect.GetValue(GTAXVF_VAT);
					const double price_with_vat = R5(amount_with_vat / qtty);
					const double price_without_vat = R5(amount_without_vat / qtty);
				//
				THROW(GetGoodsInfo(r_ti.GoodsID, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
				n_item.PutInner("gtin", goods_code);
				n_item.PutInner("internalSupplierCode", temp_buf.Z().Cat(goods_rec.ID));
				n_item.PutInner("description", EncXmlText(goods_rec.Name));
				//n_item.PutInnerSkipEmpty("codeOfEgais", ""); // <!--код товара в ЕГАИС-->
				//n_item.PutInnerSkipEmpty("lotNumberEgais", ""); // <!--номер товара в ТТН ЕГАИС-->
				//n_item.PutInnerSkipEmpty("orderLineNumber", ""); // <!--порядковый номер товара-->
				// n_item.PutInner("lineNumber", temp_buf.Z().Cat(r_ti.RByBill));
				//n_item.PutInnerSkipEmpty("comment", "");
				{
					SXml::WNode n_q(_doc, "quantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				if(r_ti.UnitPerPack > 0.0) {
					SXml::WNode n_q(_doc, "onePlaceQuantity");
					n_q.PutAttrib("unitOfMeasure", "PCE");
					n_q.SetValue(temp_buf.Z().Cat(fabs(r_ti.UnitPerPack), MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
				}
				n_item.PutInner("netPrice", temp_buf.Z().Cat(price_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // цена по позиции без НДС
				n_item.PutInner("netPriceWithVAT", temp_buf.Z().Cat(price_with_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // цена по позиции с НДС
				n_item.PutInner("netAmount", temp_buf.Z().Cat(amount_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // сумма по позиции без НДС
				n_item.PutInner("amount", temp_buf.Z().Cat(amount_with_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // сумма по позиции с НДС
				n_item.PutInner("vATRate", temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // ставка НДС
				n_item.PutInner("vATAmount", temp_buf.Z().Cat(vat_amount, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // сумма НДС
				total_amount_with_vat += amount_with_vat;
				total_amount_without_vat += amount_without_vat;
				total_vat_amount += vat_amount;
				if(rBp.LTagL.GetTagStr(i, PPTAG_LOT_CLB, temp_buf) > 0 && temp_buf.NotEmpty()) {
					n_item.PutInner("customsDeclarationNumber", temp_buf); // ГТД
				}
			}
			n_dtl.PutInner("totalSumExcludingTaxes", temp_buf.Z().Cat(total_amount_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Общая сумма отгруженных товарных позиций без НДС
			n_dtl.PutInner("totalVATAmount", temp_buf.Z().Cat(total_amount_without_vat, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); //  Общая сумма НДС по всему документу
			n_dtl.PutInner("totalAmount", temp_buf.Z().Cat(total_vat_amount, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Общая сумма с НДС по документу
		}
	}
	CATCHZOK
	return ok;
}

PPEdiProcessor::DocumentInfo::DocumentInfo() : ID(0), EdiOp(0), Time(ZERODATETIME), Status(0), Flags(0), PrvFlags(0)
{
}

PPEdiProcessor::DocumentInfoList::DocumentInfoList()
{
}

uint PPEdiProcessor::DocumentInfoList::GetCount() const { return L.getCount(); }

int PPEdiProcessor::DocumentInfoList::GetByIdx(uint idx, DocumentInfo & rItem) const
{
	int    ok = 1;
	if(idx < L.getCount()) {
		const Entry & r_entry = L.at(idx);
		rItem.ID = r_entry.ID;
		rItem.EdiOp = r_entry.EdiOp;
		rItem.Time = r_entry.Dtm;
		rItem.Uuid = r_entry.Uuid;
		rItem.Status = r_entry.Status;
		rItem.Flags = r_entry.Flags;
		rItem.PrvFlags = r_entry.PrvFlags;
		GetS(r_entry.CodeP, rItem.Code);
		GetS(r_entry.SenderCodeP, rItem.SenderCode);
		GetS(r_entry.RcvrCodeP, rItem.RcvrCode);
		GetS(r_entry.BoxP, rItem.Box);
		GetS(r_entry.SIdP, rItem.SId);
	}
	else
		ok = 0;
	return ok;
}

int PPEdiProcessor::DocumentInfoList::Add(const DocumentInfo & rItem, uint * pIdx)
{
	int    ok = 1;
	Entry  new_entry;
	MEMSZERO(new_entry);
	new_entry.ID = rItem.ID;
	new_entry.EdiOp = rItem.EdiOp;
	new_entry.Dtm = rItem.Time;
	new_entry.Uuid = rItem.Uuid;
	new_entry.Status = rItem.Status;
	new_entry.Flags = rItem.Flags;
	new_entry.PrvFlags = rItem.PrvFlags;
	AddS(rItem.Code, &new_entry.CodeP);
	AddS(rItem.SenderCode, &new_entry.SenderCodeP);
	AddS(rItem.RcvrCode, &new_entry.RcvrCodeP);
	AddS(rItem.Box, &new_entry.BoxP);
	AddS(rItem.SId, &new_entry.SIdP);
	L.insert(&new_entry);
	return ok;
}

PPEdiProcessor::RecadvPacket::RecadvPacket() : DesadvBillDate(ZERODATE), AllRowsAccepted(1), WrOffBillID(0), OrderBillID(0)
{
}

PPEdiProcessor::Packet::Packet(int docType) : DocType(docType), Flags(0), P_Data(0), P_ExtData(0)
{
	switch(DocType) {
		case PPEDIOP_ORDER:
		case PPEDIOP_DESADV:
		case PPEDIOP_ALCODESADV:
		case PPEDIOP_INVOIC:
			P_Data = new PPBillPacket;
			break;
		case PPEDIOP_ORDERRSP:
			P_Data = new PPBillPacket;
			P_ExtData = new PPBillPacket;
			break;
		case PPEDIOP_RECADV:
			P_Data = new RecadvPacket;
			break;
	}
}

PPEdiProcessor::Packet::~Packet()
{
	switch(DocType) {
		case PPEDIOP_ORDER:
		case PPEDIOP_DESADV:
		case PPEDIOP_ALCODESADV:
		case PPEDIOP_INVOIC:
			delete static_cast<PPBillPacket *>(P_Data);
			break;
		case PPEDIOP_ORDERRSP:
			delete static_cast<PPBillPacket *>(P_Data);
			delete static_cast<PPBillPacket *>(P_ExtData);
			break;
		case PPEDIOP_RECADV:
			delete (RecadvPacket *)P_Data;
			break;
	}
	P_Data = 0;
	P_ExtData = 0;
}

/*static*/PPEdiProcessor::ProviderImplementation * PPEdiProcessor::CreateProviderImplementation(PPID ediPrvID, PPID mainOrgID, long flags)
{
	ProviderImplementation * p_imp = 0;
	PPObjEdiProvider ep_obj;
	PPEdiProviderPacket ep_pack;
	THROW(ep_obj.GetPacket(ediPrvID, &ep_pack) > 0);
	if(sstreqi_ascii(ep_pack.Rec.Symb, "KONTUR") || sstreqi_ascii(ep_pack.Rec.Symb, "KONTUR-T")) {
		p_imp = new EdiProviderImplementation_Kontur(ep_pack, mainOrgID, flags);
	}
	else if(sstreqi_ascii(ep_pack.Rec.Symb, "EXITE")) { // @v10.2.8
		p_imp = new EdiProviderImplementation_Exite(ep_pack, mainOrgID, flags);
	}
	else {
		CALLEXCEPT_PP_S(PPERR_EDI_THEREISNTPRVIMP, ep_pack.Rec.Symb);
	}
	CATCH
		ZDELETE(p_imp);
	ENDCATCH
	return p_imp;
}

PPEdiProcessor::PPEdiProcessor(ProviderImplementation * pImp, PPLogger * pLogger) : P_Prv(pImp), P_Logger(pLogger), P_BObj(BillObj)
{
	PPAlbatrosCfgMngr::Get(&ACfg);
}

PPEdiProcessor::~PPEdiProcessor()
{
}

int PPEdiProcessor::SendDocument(DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(ok = P_Prv->SendDocument(pIdent, rPack));
	CATCH
		ok = 0;
		CALLPTRMEMB(P_Logger, LogLastError());
	ENDCATCH
	return ok;
}

int PPEdiProcessor::ReceiveDocument(const DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->ReceiveDocument(pIdent, rList));
	CATCHZOK
	return ok;
}

int PPEdiProcessor::GetDocumentList(const PPBillIterchangeFilt & rP, DocumentInfoList & rList)
{
	int    ok = 1;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW(P_Prv->GetDocumentList(rP, rList))
	CATCHZOK
	return ok;
}

int PPEdiProcessor::SendOrders(const PPBillIterchangeFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	BillTbl::Rec bill_rec;
	PPIDArray temp_bill_list;
	PPIDArray op_list;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	{
		PPPredictConfig cfg;
		PrcssrPrediction::GetPredictCfg(&cfg);
		op_list.addnz(ACfg.Hdr.EdiOrderOpID/*OpID*/);
		op_list.addnz(cfg.PurchaseOpID);
		op_list.addnz(CConfig.DraftRcptOp);
		op_list.sortAndUndup();
	}
	for(uint i = 0; i < op_list.getCount(); i++) {
		const PPID op_id = op_list.get(i);
		PPOprKind op_rec;
		GetOpData(op_id, &op_rec);
		if(rP.IdList.getCount()) {
			for(uint j = 0; j < rP.IdList.getCount(); j++) {
				const PPID bill_id = rP.IdList.get(j);
				if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
					if((!rP.LocID || bill_rec.LocID == rP.LocID) && rArList.lsearch(bill_rec.Object))
						temp_bill_list.add(bill_rec.ID);
				}
			}
		}
		else {
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && rArList.lsearch(bill_rec.Object))
					temp_bill_list.add(bill_rec.ID);
			}
		}
	}
	if(temp_bill_list.getCount()) {
		PPObjTag tag_obj;
		PPObjectTag tag_rec;
		SString tag_sent_content;
		temp_bill_list.sortAndUndup();
		for(uint k = 0; k < temp_bill_list.getCount(); k++) {
			const  PPID bill_id = temp_bill_list.get(k);
			int    do_skip = 0;
			PPID   tag_id = PPTAG_BILL_EDIORDERSENT;
			tag_sent_content.Z();
			if(tag_obj.Fetch(tag_id, &tag_rec) > 0) {
				if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, tag_id, tag_sent_content) > 0)
					do_skip = 1;
			}
			else
				tag_id = 0;
			if(!do_skip && P_BObj->Search(bill_id, &bill_rec) > 0 && P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
				PPEdiProcessor::Packet pack(PPEDIOP_ORDER);
				if(P_BObj->ExtractPacket(bill_id, static_cast<PPBillPacket *>(pack.P_Data)) > 0) {
					DocumentInfo di;
					if(SendDocument(&di, pack) > 0) {
						if(tag_id) {
							ObjTagItem tag_item;
							tag_sent_content.Z().Cat(di.Uuid, S_GUID::fmtIDL);
							if(tag_item.SetStr(tag_id, tag_sent_content))
								THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProcessor::CheckBillStatusForRecadvSending(const BillTbl::Rec & rBillRec)
{
	int    ok = -1;
	if(P_BObj->CheckStatusFlag(rBillRec.StatusID, BILSTF_READYFOREDIACK))
		ok = 1;
	else {
		int    wroff_bill_is_ready = -1;
		BillTbl::Rec wroff_bill_rec;
		for(DateIter di2; P_BObj->P_Tbl->EnumLinks(rBillRec.ID, &di2, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;) {
			if(!P_BObj->CheckStatusFlag(wroff_bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
				wroff_bill_is_ready = 0;
				break;
			}
			else
				wroff_bill_is_ready = 1;
		}
		if(wroff_bill_is_ready > 0)
			ok = 1;
	}
	return ok;
}

int PPEdiProcessor::SendRECADV(const PPBillIterchangeFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	SString edi_ack;
	BillTbl::Rec bill_rec;
	PPBillPacket * p_link_bp = 0;
	PPIDArray temp_bill_list;
	PPIDArray op_list;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	THROW_PP(ACfg.Hdr.EdiDesadvOpID, PPERR_EDI_OPNDEF_DESADV);
	op_list.add(ACfg.Hdr.EdiDesadvOpID);
	if(rP.IdList.getCount()) {
		for(uint j = 0; j < rP.IdList.getCount(); j++) {
			const PPID bill_id = rP.IdList.get(j);
			if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.EdiOp == PPEDIOP_DESADV && op_list.bsearch(bill_rec.OpID)) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && rArList.lsearch(bill_rec.Object)) {
					if(CheckBillStatusForRecadvSending(bill_rec) > 0)
						temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	else {
		for(uint i = 0; i < rArList.getCount(); i++) {
			const PPID ar_id = rArList.get(i);
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
				if(bill_rec.EdiOp == PPEDIOP_DESADV && (!rP.LocID || bill_rec.LocID == rP.LocID) && op_list.bsearch(bill_rec.OpID)) {
					if(CheckBillStatusForRecadvSending(bill_rec) > 0) {
						temp_bill_list.add(bill_rec.ID);
					}
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0) {
			if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, PPTAG_BILL_EDIACK, edi_ack) > 0) {
				;
			}
			else {
				PPEdiProcessor::Packet pack(PPEDIOP_RECADV);
				RecadvPacket * p_recadv_pack = static_cast<RecadvPacket *>(pack.P_Data);
				assert(p_recadv_pack);
				THROW(p_recadv_pack);
				if(P_BObj->ExtractPacket(bill_id, &p_recadv_pack->ABp) > 0) {
					/*
					int    cmp_result = 0; // 0 - accepted, -1 - rejected,
						// & 0x01 - есть отличия в меньшую сторону по количеству
						// & 0x02 - есть отличия в большую сторону по количеству
					int    is_uncond_accept = 0;
					*/
					int    is_status_suited = 0;
					PPBillPacket order_bp;
					BillTbl::Rec order_bill_rec;
					PPIDArray wroff_bill_list;
					BillTbl::Rec wroff_bill_rec;
					// @v11.1.12 BillCore::GetCode(p_recadv_pack->DesadvBillCode = p_recadv_pack->ABp.Rec.Code);
					p_recadv_pack->DesadvBillCode = p_recadv_pack->ABp.Rec.Code; // @v11.1.12 
					p_recadv_pack->DesadvBillDate = p_recadv_pack->ABp.Rec.Dt;
					if(p_recadv_pack->ABp.Rec.LinkBillID && P_BObj->Fetch(p_recadv_pack->ABp.Rec.LinkBillID, &order_bill_rec) > 0) {
						ObjTagItem tag_item;
						if(p_ref->Ot.GetTag(PPOBJ_BILL, p_recadv_pack->ABp.Rec.LinkBillID, PPTAG_BILL_EDIORDERSENT, &tag_item) > 0) {
							p_recadv_pack->OrderBillID = p_recadv_pack->ABp.Rec.LinkBillID;
							THROW(P_BObj->ExtractPacket(p_recadv_pack->OrderBillID, &order_bp) > 0);
						}
					}
					for(DateIter diter; P_BObj->P_Tbl->EnumLinks(bill_id, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;)
						wroff_bill_list.add(wroff_bill_rec.ID);
					if(wroff_bill_list.getCount() > 1)
						;//LogTextWithAddendum(PPTXT_EGAIS_BILLWROFFCONFL, bill_text);
					else {
						if(wroff_bill_list.getCount() == 1) {
							THROW(P_BObj->ExtractPacket(wroff_bill_list.get(0), &p_recadv_pack->RBp) > 0);
							p_recadv_pack->WrOffBillID = p_recadv_pack->RBp.Rec.ID;
							p_link_bp = &p_recadv_pack->RBp;
							if(P_BObj->CheckStatusFlag(p_recadv_pack->RBp.Rec.StatusID, BILSTF_READYFOREDIACK)) {
								for(uint i = 0; i < p_recadv_pack->ABp.GetTCount(); i++) {
									const PPTransferItem & r_desadv_ti = p_recadv_pack->ABp.ConstTI(i);
									uint recadv_rbb_pos = 0;
									if(p_recadv_pack->RBp.SearchTI(r_desadv_ti.RByBill, &recadv_rbb_pos)) {
										const PPTransferItem & r_recadv_ti = p_recadv_pack->RBp.ConstTI(recadv_rbb_pos);
										p_recadv_pack->RecadvQttyList.Add(i+1, fabs(r_recadv_ti.Quantity_));
									}
									else
										p_recadv_pack->RecadvQttyList.Add(i+1, 0.0);
								}
								is_status_suited = 1;
							}
						}
						else if(p_recadv_pack->ABp.Rec.Flags2 & BILLF2_DECLINED) {
							for(uint i = 0; i < p_recadv_pack->ABp.GetTCount(); i++) {
								p_recadv_pack->RecadvQttyList.Add(i+1, 0.0);
							}
							is_status_suited = 1;
						}
						if(is_status_suited) {
							{
								for(uint i = 0; i < p_recadv_pack->ABp.GetTCount(); i++) {
									const PPTransferItem & r_desadv_ti = p_recadv_pack->ABp.ConstTI(i);
									p_recadv_pack->DesadvQttyList.Add(i+1, fabs(r_desadv_ti.Quantity_));
								}
							}
							if(order_bp.Rec.ID) {
								for(uint i = 0; i < p_recadv_pack->ABp.GetTCount(); i++) {
									const PPTransferItem & r_desadv_ti = p_recadv_pack->ABp.ConstTI(i);
									double ordqtty = 0.0;
									for(uint ordpos = 0; order_bp.SearchGoods(r_desadv_ti.GoodsID, &ordpos); ordpos++) {
										ordqtty += fabs(order_bp.ConstTI(ordpos).Quantity_);
									}
									p_recadv_pack->OrderedQttyList.Add(i+1, ordqtty);
								}
							}
							else {
								for(uint i = 0; i < p_recadv_pack->ABp.GetTCount(); i++)
									p_recadv_pack->OrderedQttyList.Add(i+1, fabs(p_recadv_pack->ABp.ConstTI(i).Quantity_));
							}
							DocumentInfo di;
							if(SendDocument(&di, pack) > 0) {
								ObjTagItem tag_item;
								if(tag_item.SetStr(PPTAG_BILL_EDIACK, temp_buf.Z().Cat(di.Uuid, S_GUID::fmtIDL)))
									THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
								if(p_recadv_pack->RBp.Rec.ID && di.SId.NotEmpty()) {
									if(tag_item.SetStr(PPTAG_BILL_EDIIDENT, di.SId))
										THROW(p_ref->Ot.PutTag(PPOBJ_BILL, p_recadv_pack->RBp.Rec.ID, &tag_item, 0));
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProcessor::SendOrderRsp(const PPBillIterchangeFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	BillTbl::Rec bill_rec;
	PPIDArray temp_bill_list;
	PPIDArray op_list;
	SString temp_buf;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	{
		PPPredictConfig cfg;
		PrcssrPrediction::GetPredictCfg(&cfg);
		op_list.addnz(ACfg.Hdr.OpID);
		op_list.addnz(cfg.PurchaseOpID);
		op_list.addnz(CConfig.DraftRcptOp);
		op_list.sortAndUndup();
	}
	if(rP.IdList.getCount()) {
		for(uint j = 0; j < rP.IdList.getCount(); j++) {
			const PPID bill_id = rP.IdList.get(j);
			if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.EdiOp == PPEDIOP_ORDER && op_list.bsearch(bill_rec.OpID)) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
					if(rArList.lsearch(bill_rec.Object)) {
						temp_bill_list.add(bill_rec.ID);
					}
				}
			}
		}
	}
	else {
		for(uint i = 0; i < rArList.getCount(); i++) {
			const PPID ar_id = rArList.get(i);
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
				if(bill_rec.EdiOp == PPEDIOP_ORDER && (!rP.LocID || bill_rec.LocID == rP.LocID) && op_list.bsearch(bill_rec.OpID) &&
					P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
					temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	temp_bill_list.sortAndUndup();
	for(uint k = 0; k < temp_bill_list.getCount(); k++) {
		const PPID bill_id = temp_bill_list.get(k);
		if(P_BObj->Search(bill_id, &bill_rec) > 0 && P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
			PPEdiProcessor::Packet pack(PPEDIOP_ORDERRSP);
			if(P_BObj->ExtractPacket(bill_id, static_cast<PPBillPacket *>(pack.P_Data)) > 0) {
				int gopr = P_BObj->GetOriginalPacket(bill_id, 0, static_cast<PPBillPacket *>(pack.P_ExtData));
				if(gopr <= 0 || gopr == 1/*документ не менялся*/) {
					delete static_cast<PPBillPacket *>(pack.P_ExtData);
					pack.P_ExtData = 0;
				}
				DocumentInfo di;
				if(SendDocument(&di, pack) > 0) {
					ObjTagItem tag_item;
					temp_buf.Z().Cat(di.Uuid, S_GUID::fmtIDL);
					if(tag_item.SetStr(PPTAG_BILL_EDIORDRSPSENT, temp_buf))
						THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProcessor::SendDESADV(int ediOp, const PPBillIterchangeFilt & rP, const PPIDArray & rArList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	BillTbl::Rec bill_rec;
	SString temp_buf;
	PPIDArray temp_bill_list;
	THROW_PP(P_Prv, PPERR_EDI_PRVUNDEF);
	assert(oneof3(ediOp, PPEDIOP_DESADV, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC));
	THROW(oneof3(ediOp, PPEDIOP_DESADV, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC));
	if(rP.IdList.getCount()) {
		for(uint j = 0; j < rP.IdList.getCount(); j++) {
			const PPID bill_id = rP.IdList.get(j);
			if(P_BObj->Search(bill_id, &bill_rec) > 0 && GetOpType(bill_rec.OpID) == PPOPT_GOODSEXPEND) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && rArList.lsearch(bill_rec.Object)) {
					temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	else {
		for(uint i = 0; i < rArList.getCount(); i++) {
			const PPID ar_id = rArList.get(i);
			for(DateIter di(&rP.Period); P_BObj->P_Tbl->EnumByObj(ar_id, &di, &bill_rec) > 0;) {
				if((!rP.LocID || bill_rec.LocID == rP.LocID) && GetOpType(bill_rec.OpID) == PPOPT_GOODSEXPEND) {
					temp_bill_list.add(bill_rec.ID);
				}
			}
		}
	}
	if(temp_bill_list.getCount()) {
		PPObjTag tag_obj;
		SString tag_sent_content;
		temp_bill_list.sortAndUndup();
		for(uint k = 0; k < temp_bill_list.getCount(); k++) {
			const  PPID bill_id = temp_bill_list.get(k);
			int    do_skip = 0;
			PPID   tag_id = 0;
			tag_sent_content.Z();
			switch(ediOp) {
				case PPEDIOP_DESADV: tag_id = PPTAG_BILL_EDIDESADVSENT; break;
				case PPEDIOP_ALCODESADV: tag_id = PPTAG_BILL_EDIALCDESADVSENT; break;
				case PPEDIOP_INVOIC:
					tag_id = PPTAG_BILL_EDIINVOICSENT;
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIRECADVRCV, temp_buf) > 0) {
					}
					else
						do_skip = 1; // Если не получен RECADV, то INVOIC не отсылаем
					break;
			}
			if(!do_skip && tag_id) {
				PPObjectTag tag_rec;
				if(tag_obj.Fetch(tag_id, &tag_rec) > 0) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, tag_id, tag_sent_content) > 0)
						do_skip = 1;
				}
				else
					tag_id = 0;
			}
			if(!do_skip && P_BObj->Search(bill_id, &bill_rec) > 0 && P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
				PPEdiProcessor::Packet pack(ediOp);
				PPBillPacket * p_bp = static_cast<PPBillPacket *>(pack.P_Data);
				if(P_BObj->ExtractPacket(bill_id, p_bp) > 0) {
					if(ediOp == PPEDIOP_ALCODESADV) {
						do_skip = 1;
						PrcssrAlcReport & r_apr = P_Prv->Arp;
						for(uint tidx = 0; do_skip && tidx < p_bp->GetTCount(); tidx++) {
							const PPTransferItem & r_ti = p_bp->ConstTI(tidx);
							if(r_apr.IsAlcGoods(r_ti.GoodsID))
								do_skip = 0;
						}
					}
					if(!do_skip) {
						DocumentInfo di;
						if(SendDocument(&di, pack) > 0) {
							if(tag_id) {
								ObjTagItem tag_item;
								tag_sent_content.Z().Cat(di.Uuid, S_GUID::fmtIDL);
								if(tag_item.SetStr(tag_id, tag_sent_content))
									THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
EdiProviderImplementation_Kontur::EdiProviderImplementation_Kontur(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags) :
	PPEdiProcessor::ProviderImplementation(rEpp, mainOrgID, flags)
{
}

EdiProviderImplementation_Kontur::~EdiProviderImplementation_Kontur()
{
}

int EdiProviderImplementation_Kontur::GetDocumentList(const PPBillIterchangeFilt & rP, PPEdiProcessor::DocumentInfoList & rList)
{
	int    ok = -1;
	SString temp_buf;
	SString left, right;
	SPathStruc ps;
	InetUrl url;
	THROW(Epp.MakeUrl(0, url));
	const int prot = url.GetProtocol();
	if(prot == InetUrl::protUnkn) {
		url.SetProtocol(InetUrl::protFtp);
	}
	if(prot == InetUrl::protFtp) {
		const char * p_box = "Inbox";
		int    last_id = 0;
		ScURL  curl;
		url.SetComponent(url.cPath, p_box);
		SFileEntryPool fp;
		SFileEntryPool::Entry fpe;
		THROW_SL(curl.FtpList(url, ScURL::mfVerbose, fp));
		for(uint i = 0; i < fp.GetCount(); i++) {
			if(fp.Get(i, &fpe, 0) > 0) {
				ps.Split(fpe.Name);
				if(ps.Ext.IsEqiAscii("xml") && ps.Nam.Divide('_', left, right) > 0) {
					PPEdiProcessor::DocumentInfo entry;
					entry.Uuid.FromStr(right);
					entry.EdiOp = PPEdiProcessor::GetEdiMsgTypeByText(left);
					entry.Box = p_box;
					entry.ID = ++last_id;
					entry.SId = fpe.Name;
					entry.Time = fpe.WriteTime;
					THROW(rList.Add(entry, 0));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Kontur::ReadOwnFormatDocument(void * pCtx, const char * pFileName, const char * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = -1;
	SString temp_buf;
	SString addendum_msg_buf;
	SString serial;
	SString goods_name;
	SString gtin;
	SString ar_goods_code;
	xmlParserCtxt * p_ctx = static_cast<xmlParserCtxt *>(pCtx);
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	PPEdiProcessor::Packet * p_pack = 0;
	PPBillPacket * p_bpack = 0; // is owned by p_pack
	PPID   goods_id_by_gtin = 0;
	PPID   goods_id_by_arcode = 0;
	Goods2Tbl::Rec goods_rec;
	BarcodeTbl::Rec bc_rec;
	OwnFormatCommonAttr attrs;
	OwnFormatContractor contractor;
	THROW_SL(fileExists(pFileName));
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "eDIMessage")) {
		int    edi_op = 0;
		int    is_test = 0;
		PPID   sender_psn_id = 0;
		PPID   rcvr_psn_id = 0;
		LDATETIME cr_dtm = ZERODATETIME;
		LDATETIME cr_dtm_by_sender = ZERODATETIME;
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "interchangeHeader")) {
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::GetContentByName(p_n2, "sender", temp_buf)) {
						THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &sender_psn_id) > 0, PPERR_EDI_UNBLRSLV_SENDER, temp_buf);
					}
					else if(SXml::GetContentByName(p_n2, "recipient", temp_buf)) {
						THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &rcvr_psn_id) > 0, PPERR_EDI_UNBLRSLV_RCVR, temp_buf);
					}
					else if(SXml::GetContentByName(p_n2, "documentType", temp_buf)) {
						THROW(edi_op == 0); // @todo err
						edi_op = PPEanComDocument::GetMsgTypeBySymb(temp_buf);
					}
					else if(SXml::GetContentByName(p_n2, "creationDateTime", temp_buf))
						strtodatetime(temp_buf, &cr_dtm, DATF_ISO8601, 0);
					else if(SXml::GetContentByName(p_n2, "creationDateTimeBySender", temp_buf))
						strtodatetime(temp_buf, &cr_dtm_by_sender, DATF_ISO8601, 0);
					else if(SXml::GetContentByName(p_n2, "isTest", temp_buf)) {
						if(temp_buf.ToLong() == 1)
							is_test = 1;
					}
				}
			}
			else if(SXml::IsName(p_n, "order")) {
				THROW(edi_op == PPEDIOP_ORDER);
				THROW_PP(ACfg.Hdr.OpID, PPERR_EDI_OPNDEF_ORDER);
				THROW(ReadCommonAttributes(p_n, attrs));
				THROW_MEM(p_pack = new PPEdiProcessor::Packet(edi_op));
				p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
				THROW(p_bpack->CreateBlank_WithoutCode(ACfg.Hdr.OpID, 0, 0, 1));
				STRNSCPY(p_bpack->Rec.Code, attrs.Num);
				p_bpack->Rec.Dt = attrs.Dt;
				p_bpack->Rec.EdiOp = edi_op;
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::GetContentByName(p_n2, "proposalOrdersIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "promotionDealNumber", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "contractIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "blanketOrderIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "comment", temp_buf)) {
						// @v11.1.12 STRNSCPY(p_bpack->Rec.Memo, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
						p_bpack->SMemo = temp_buf.Transf(CTRANSF_UTF8_TO_INNER); // @v11.1.12
					}
					else if(SXml::IsName(p_n2, "seller")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SELLER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "buyer")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_BUYER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "invoicee")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_INVOICEE, p_bpack));
					}
					else if(SXml::IsName(p_n2, "deliveryInfo")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "requestedDeliveryDateTime", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "exportDateTimeFromSupplier", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "transportBy", temp_buf)) {
							}
							else if(SXml::IsName(p_n3, "shipFrom")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_CONSIGNOR, p_bpack));
							}
							else if(SXml::IsName(p_n3, "shipTo")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SHIPTO, p_bpack));
							}
							else if(SXml::IsName(p_n3, "ultimateCustomer")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
							}
							else if(SXml::IsName(p_n3, "transportation")) {
							}
						}
					}
					else if(SXml::IsName(p_n2, "lineItems")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "currencyISOCode", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "totalSumExcludingTaxes", temp_buf)) {
							}
							else if(SXml::IsName(p_n3, "lineItem")) {
								PPTransferItem ti;
								double price_wo_taxes = 0.0;
								double full_price = 0.0;
								double vat_rate = -1.0;
 								goods_id_by_gtin = 0;
								goods_id_by_arcode = 0;
								goods_name.Z();
								gtin.Z();
								ar_goods_code.Z();
								serial.Z();
								ti.Init(&p_bpack->Rec);
								for(xmlNode * p_li = p_n3->children; p_li; p_li = p_li->next) {
									if(SXml::GetContentByName(p_li, "gtin", temp_buf)) {
										gtin = temp_buf;
										if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec, 1) > 0)
											goods_id_by_gtin = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "internalBuyerCode", temp_buf)) {
										ar_goods_code = temp_buf;
										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
											goods_id_by_arcode = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "description", temp_buf)) {
										(goods_name = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
									}
									else if(SXml::GetContentByName(p_li, "lineNumber", temp_buf)) {
										ti.RByBill = static_cast<int16>(temp_buf.ToLong());
										if(ti.RByBill < 0)
											ti.RByBill = 0;
									}
									else if(SXml::GetContentByName(p_li, "comment", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "requestedQuantity", temp_buf)) {
										ti.Quantity_ = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "onePlaceQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "flowType", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netPrice", temp_buf)) {
										price_wo_taxes = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "netPriceWithVAT", temp_buf)) {
										full_price = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "VATRate", temp_buf)) {
										vat_rate = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "netAmount", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "amount", temp_buf)) {
									}
									else if(SXml::IsName(p_li, "ultimateCustomer")) {
										//
									}
								}
								if(goods_id_by_gtin) {
									ti.GoodsID = goods_id_by_gtin;
									if(goods_id_by_arcode && goods_id_by_arcode != goods_id_by_gtin) {
										// @todo message ambiguity
									}
								}
								else if(goods_id_by_arcode)
									ti.GoodsID = goods_id_by_arcode;
								if(ti.GoodsID) {
									if(ti.Quantity_ <= 0.0) {
										addendum_msg_buf.Z().Cat(ti.Quantity_, MKSFMTD(0, 3, 0)).CatDiv(':', 1).
											Cat(p_bpack->Rec.Code).CatDiv('-', 1).Cat(p_bpack->Rec.Dt, DATF_DMY);
										CALLEXCEPT_PP_S(PPERR_EDI_UNBLRSLV_INVQTTY, addendum_msg_buf);
									}
									else {
										if(full_price > 0.0) {
											ti.Price = full_price;
										}
										else if(price_wo_taxes > 0.0) {

										}
										p_bpack->LoadTItem(&ti, 0, 0);
									}
								}
								else {
									addendum_msg_buf.Z();
									if(gtin.NotEmpty())
										addendum_msg_buf.Cat(gtin);
									if(ar_goods_code.NotEmpty())
										addendum_msg_buf.CatDivIfNotEmpty('/', 0).Cat(ar_goods_code);
									if(goods_name.NotEmpty())
										addendum_msg_buf.CatDivIfNotEmpty('/', 0).Cat(goods_name);
									addendum_msg_buf.CatDivIfNotEmpty(':', 1).Cat(p_bpack->Rec.Code).CatDiv('-', 1).Cat(p_bpack->Rec.Dt, DATF_DMY);
									CALLEXCEPT_PP_S(PPERR_EDI_UNBLRSLV_GOODS, addendum_msg_buf);
								}
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "orderResponse")) {
				THROW(edi_op == PPEDIOP_ORDERRSP);
				THROW_PP(ACfg.Hdr.EdiOrderSpOpID, PPERR_EDI_OPNDEF_ORDERRSP);
				THROW(ReadCommonAttributes(p_n, attrs));
				THROW_MEM(p_pack = new PPEdiProcessor::Packet(edi_op));
				p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
				addendum_msg_buf.Z().Cat("ORDERRSP").Space().Cat(attrs.Num).Space().Cat(attrs.Dt, DATF_DMY);
				THROW_PP_S(p_bpack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::GetContentByName(p_n2, "originOrder", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "blanketOrderIdentificator", temp_buf)) {
					}
					else if(SXml::IsName(p_n2, "seller")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SELLER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "buyer")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_BUYER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "invoicee")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_INVOICEE, p_bpack));
					}
					else if(SXml::IsName(p_n2, "deliveryInfo")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "requestedDeliveryDateTime", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "exportDateTimeFromSupplier", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "transportBy", temp_buf)) {
							}
							else if(SXml::IsName(p_n3, "shipFrom")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_CONSIGNOR, p_bpack));
							}
							else if(SXml::IsName(p_n3, "shipTo")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SHIPTO, p_bpack));
							}
							else if(SXml::IsName(p_n3, "ultimateCustomer")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
							}
							else if(SXml::IsName(p_n3, "transportation")) {
							}
						}
					}
					else if(SXml::IsName(p_n2, "lineItems")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "currencyISOCode", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "lineItem", temp_buf)) {
								PPTransferItem ti;
								goods_id_by_gtin = 0;
								goods_id_by_arcode = 0;
								serial.Z();
								goods_name.Z();
								ti.Init(&p_bpack->Rec);
								for(xmlNode * p_li = p_n3->children; p_li; p_li = p_li->next) {
									if(SXml::GetContentByName(p_li, "gtin", temp_buf)) {
										if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec) > 0)
											goods_id_by_gtin = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "internalSupplierCode", temp_buf)) {
										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
											goods_id_by_arcode = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "serialNumber", temp_buf)) {
										serial = temp_buf;
									}
									else if(SXml::GetContentByName(p_li, "orderLineNumber", temp_buf)) {
										ti.RByBill = static_cast<int16>(temp_buf.ToLong());
										if(ti.RByBill < 0)
											ti.RByBill = 0;
									}
									else if(SXml::GetContentByName(p_li, "description", temp_buf)) {
										goods_name = temp_buf;
									}
									else if(SXml::GetContentByName(p_li, "comment", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "orderedQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "confirmedQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "onePlaceQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "expireDate", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "manufactoringDate", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "estimatedDeliveryDate", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netPrice", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netPriceWithVAT", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netAmount", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "amount", temp_buf)) {
									}
									else if(SXml::IsName(p_li, "ultimateCustomer")) {
									}
								}
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "despatchAdvice")) {
				THROW(edi_op == PPEDIOP_DESADV);
				THROW_PP(ACfg.Hdr.EdiDesadvOpID, PPERR_EDI_OPNDEF_DESADV);
				THROW(ReadCommonAttributes(p_n, attrs));
				THROW_MEM(p_pack = new PPEdiProcessor::Packet(edi_op));
				p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
				addendum_msg_buf.Z().Cat("DESADV").Space().Cat(attrs.Num).Space().Cat(attrs.Dt, DATF_DMY);
				THROW_PP_S(p_bpack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
				THROW(p_bpack->CreateBlank_WithoutCode(ACfg.Hdr.EdiDesadvOpID, 0, 0, 1));
				STRNSCPY(p_bpack->Rec.Code, attrs.Num);
				p_bpack->Rec.Dt = attrs.Dt;
				p_bpack->Rec.EdiOp = edi_op;
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::GetContentByName(p_n2, "originOrder", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "orderResponse", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "blanketOrderIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "egaisRegistrationIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "egaisFixationIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "deliveryNoteIdentificator", temp_buf)) {
					}
					else if(SXml::IsName(p_n2, "seller")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SELLER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "buyer")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_BUYER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "invoicee")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_INVOICEE, p_bpack));
					}
					else if(SXml::IsName(p_n2, "deliveryInfo")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "requestedDeliveryDateTime", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "exportDateTimeFromSupplier", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "transportBy", temp_buf)) {
							}
							else if(SXml::IsName(p_n3, "shipFrom")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_CONSIGNOR, p_bpack));
							}
							else if(SXml::IsName(p_n3, "shipTo")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
								THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SHIPTO, p_bpack));
							}
							else if(SXml::IsName(p_n3, "ultimateCustomer")) {
								THROW(ReadOwnFormatContractor(p_n3->children, contractor));
							}
							else if(SXml::IsName(p_n3, "transportation")) {
							}
						}
					}
					else if(SXml::GetContentByName(p_n2, "packages", temp_buf)) {
					}
					else if(SXml::IsName(p_n2, "lineItems")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::GetContentByName(p_n3, "currencyISOCode", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "totalSumExcludingTaxes", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "totalVATAmount", temp_buf)) {
							}
							else if(SXml::GetContentByName(p_n3, "totalAmount", temp_buf)) {
							}
							else if(SXml::IsName(p_n3, "lineItem")) {
								PPTransferItem ti;
								double price_wo_taxes = 0.0;
								double full_price = 0.0;
 								goods_id_by_gtin = 0;
								goods_id_by_arcode = 0;
								serial.Z();
								goods_name.Z();
								ti.Init(&p_bpack->Rec);
								for(xmlNode * p_li = p_n3->children; p_li; p_li = p_li->next) {
									if(SXml::GetContentByName(p_li, "gtin", temp_buf)) {
										if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec) > 0)
											goods_id_by_gtin = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "internalSupplierCode", temp_buf)) {
										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
											goods_id_by_arcode = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "packageLevel", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "codeOfEgais", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "lotNumberEgais", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "orderLineNumber", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "description", temp_buf)) {
										goods_name = temp_buf;
									}
									else if(SXml::GetContentByName(p_li, "orderedQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "despatchedQuantity", temp_buf)) {
										ti.Quantity_ = R6(temp_buf.ToReal());
									}
									else if(SXml::GetContentByName(p_li, "onePlaceQuantity", temp_buf)) {
										ti.UnitPerPack = R6(temp_buf.ToReal());
									}
									else if(SXml::GetContentByName(p_li, "expireDate", temp_buf)) {
										ti.Expiry = strtodate_(temp_buf, DATF_YMD);
									}
									else if(SXml::GetContentByName(p_li, "freshnessDate", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netAmount", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "amount", temp_buf)) {
									}
									else if(SXml::IsName(p_li, "ultimateCustomer")) {
										//
									}
								}
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "receivingAdvice")) {
				PPID   desadv_bill_id = 0;
				int    diff_status = 0; //
				PPObjOprKind op_obj;
				PPEdiProcessor::RecadvPacket * p_recadv_pack = 0;
				OwnFormatCommonAttr desadv_bill_attr;
				PPID   op_id = 0;
				THROW(edi_op == PPEDIOP_RECADV);
				THROW(op_obj.GetEdiRecadvOp(&op_id, 1));
				//THROW_PP(ACfg.Hdr.E, PPERR_EDI_OPNDEF_DESADV);
				THROW(ReadCommonAttributes(p_n, attrs));
				THROW_MEM(p_pack = new PPEdiProcessor::Packet(edi_op));
				p_recadv_pack = (PPEdiProcessor::RecadvPacket *)p_pack->P_Data;
				addendum_msg_buf.Z().Cat("RECADV").Space().Cat(attrs.Num).Space().Cat(attrs.Dt, DATF_DMY);
				THROW_PP_S(p_recadv_pack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
				{
					p_bpack = &p_recadv_pack->RBp;
					THROW(p_bpack->CreateBlank_WithoutCode(op_id, 0, 0, 1));
					STRNSCPY(p_bpack->Rec.Code, attrs.Num);
					p_bpack->Rec.Dt = attrs.Dt;
					p_bpack->Rec.EdiOp = edi_op;
				}
				for(xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::GetContentByName(p_n2, "originOrder", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "despatchIdentificator", temp_buf)) {
						THROW(ReadCommonAttributes(p_n2, desadv_bill_attr));
						p_recadv_pack->DesadvBillCode = desadv_bill_attr.Num;
						p_recadv_pack->DesadvBillDate = desadv_bill_attr.Dt;
					}
					else if(SXml::GetContentByName(p_n2, "blanketOrderIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "receivingAdviceIdentificatorInBuyerSystem", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "egaisRegistrationIdentificator", temp_buf)) {
					}
					else if(SXml::GetContentByName(p_n2, "egaisFixationIdentificator", temp_buf)) {
					}
					else if(SXml::IsName(p_n2, "seller")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_SELLER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "buyer")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_BUYER, p_bpack));
					}
					else if(SXml::IsName(p_n2, "invoicee")) {
						THROW(ReadOwnFormatContractor(p_n2->children, contractor));
						THROW(ResolveOwnFormatContractor(contractor, EDIPARTYQ_INVOICEE, p_bpack));
					}
					else if(SXml::IsName(p_n2, "deliveryInfo")) {
					}
					else if(SXml::IsName(p_n2, "lineItems")) {
						for(xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, "lineItem")) {
								PPTransferItem ti;
								double price_wo_taxes = 0.0;
								double full_price = 0.0;
								double desadv_qtty = 0.0; // Отправленное количество
								double recadv_qtty = 0.0; // Принятое количество
 								goods_id_by_gtin = 0;
								goods_id_by_arcode = 0;
								serial.Z();
								goods_name.Z();
								ti.Init(&p_bpack->Rec);
								for(xmlNode * p_li = p_n3->children; p_li; p_li = p_li->next) {
									if(SXml::GetContentByName(p_li, "gtin", temp_buf)) {
										if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec) > 0)
											goods_id_by_gtin = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "internalBuyerCode", temp_buf)) {
										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
											goods_id_by_arcode = goods_rec.ID;
									}
									else if(SXml::GetContentByName(p_li, "codeOfEgais", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "lotNumberEgais", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "orderLineNumber", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "description", temp_buf)) {
										(goods_name = temp_buf).Transf(CTRANSF_UTF8_TO_INNER);
									}
									else if(SXml::GetContentByName(p_li, "orderedQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "despatchedQuantity", temp_buf)) {
										desadv_qtty = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "deliveredQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "acceptedQuantity", temp_buf)) {
										recadv_qtty = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "onePlaceQuantity", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netPrice", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "netPriceWithVAT", temp_buf)) {
										full_price = temp_buf.ToReal();
									}
									else if(SXml::GetContentByName(p_li, "netAmount", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "exciseDuty", temp_buf)) {
									}
									else if(SXml::GetContentByName(p_li, "amount", temp_buf)) {
									}
								}
								if(goods_id_by_gtin) {
									ti.GoodsID = goods_id_by_gtin;
									if(goods_id_by_arcode && goods_id_by_arcode != goods_id_by_gtin) {
										// @todo message ambiguity
									}
								}
								else if(goods_id_by_arcode)
									ti.GoodsID = goods_id_by_arcode;
								if(ti.GoodsID) {
									if(recadv_qtty <= 0.0) {
										addendum_msg_buf.Z().Cat(ti.Quantity_, MKSFMTD(0, 3, 0)).CatDiv(':', 1).
											Cat(p_bpack->Rec.Code).CatDiv('-', 1).Cat(p_bpack->Rec.Dt, DATF_DMY);
										CALLEXCEPT_PP_S(PPERR_EDI_UNBLRSLV_INVQTTY, addendum_msg_buf);
									}
									else {
										ti.Quantity_ = fabs(recadv_qtty);
										if(full_price > 0.0) {
											ti.Price = full_price;
										}
										else if(price_wo_taxes > 0.0) {
										}
										p_bpack->LoadTItem(&ti, 0, 0);
										p_recadv_pack->DesadvQttyList.Add(p_bpack->GetTCount(), desadv_qtty);
										if(!feqeps(desadv_qtty, recadv_qtty, 1E-7)) {
											p_recadv_pack->AllRowsAccepted = 0;
										}
									}
								}
								else {
									addendum_msg_buf.Z();
									if(gtin.NotEmpty())
										addendum_msg_buf.Cat(gtin);
									if(ar_goods_code.NotEmpty())
										addendum_msg_buf.CatDivIfNotEmpty('/', 0).Cat(ar_goods_code);
									if(goods_name.NotEmpty())
										addendum_msg_buf.CatDivIfNotEmpty('/', 0).Cat(goods_name);
									addendum_msg_buf.CatDivIfNotEmpty(':', 1).Cat(p_bpack->Rec.Code).CatDiv('-', 1).Cat(p_bpack->Rec.Dt, DATF_DMY);
									CALLEXCEPT_PP_S(PPERR_EDI_UNBLRSLV_GOODS, addendum_msg_buf);
								}
							}
						}
					}
				}
			}
		}
	}
	if(p_pack) {
		if(p_bpack)
			p_bpack->BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, pIdent);
		rList.insert(p_pack);
		p_pack = 0; // Перед выходом экземпляр разрушается
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	delete p_pack;
	return ok;
}

int EdiProviderImplementation_Kontur::ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = 1;
	xmlParserCtxt * p_ctx = 0;
	if(pIdent && pIdent->Box.NotEmpty() && pIdent->SId.NotEmpty()) {
		int    skip = 0;
		SString temp_buf;
		if(!pIdent->Uuid.IsZero())
			pIdent->Uuid.ToStr(S_GUID::fmtIDL, temp_buf);
		else {
			SPathStruc ps(pIdent->SId);
			temp_buf = ps.Nam;
		}
		const SString edi_ident_buf(temp_buf);
		if(edi_ident_buf.NotEmpty()) {
			PPIDArray ex_bill_id_list;
			PPRef->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, edi_ident_buf, &ex_bill_id_list);
			if(ex_bill_id_list.getCount()) {
				// Документ уже существует ?@todo message
				skip = 1;
			}
		}
		if(!skip) {
			InetUrl url;
			THROW(Epp.MakeUrl(0, url));
			const int prot = url.GetProtocol();
			if(prot == InetUrl::protUnkn) {
				url.SetProtocol(InetUrl::protFtp);
			}
			if(prot == InetUrl::protFtp) {
				const char * p_box = pIdent->Box;
				ScURL  curl;
				THROW(p_ctx = xmlNewParserCtxt());
				(temp_buf = p_box).SetLastDSlash().Cat(pIdent->SId);
				url.SetComponent(url.cPath, temp_buf);

				GetTempInputPath(pIdent->EdiOp, temp_buf);
				temp_buf.SetLastSlash().Cat(pIdent->SId);
				if(!fileExists(temp_buf)) {
					THROW_SL(curl.FtpGet(url, ScURL::mfVerbose, temp_buf, 0, 0));
				}
				{
					int    format = 0;
					PPXmlFileDetector xfd;
					if(xfd.Run(temp_buf, &format)) {
						if(format == xfd.Eancom) {
							PPEanComDocument s_doc(this);
							const int rdr = s_doc.Read_Document(this, p_ctx, temp_buf, edi_ident_buf, rList);
							THROW(rdr);
						}
						else if(format == xfd.KonturEdi) {
							const int rdr = ReadOwnFormatDocument(p_ctx, temp_buf, edi_ident_buf, rList);
							THROW(rdr);
						}
					}
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int EdiProviderImplementation_Kontur::SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = -1;
	xmlTextWriter * p_x = 0;
	SString path;
	// @v11.2.9 PPEDIOP_RECADV
	if(rPack.P_Data && oneof6(rPack.DocType, PPEDIOP_ORDER, PPEDIOP_ORDERRSP, PPEDIOP_DESADV, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC, PPEDIOP_RECADV)) {
		const S_GUID msg_uuid(SCtrGenerate_);
		const PPEdiProcessor::RecadvPacket * p_recadv_pack = (rPack.DocType == PPEDIOP_RECADV) ? static_cast<const PPEdiProcessor::RecadvPacket *>(rPack.P_Data) : 0;
		const PPBillPacket * p_bp = static_cast<const PPBillPacket *>(rPack.P_Data);
		SString temp_buf;
		SString edi_format_symb;
		GetTempOutputPath(rPack.DocType, path);
		THROW_SL(::createDir(path.RmvLastSlash()));
		//
		//MakeTempFileName(path.SetLastSlash(), "export_", "xml", 0, temp_buf);
		PPEanComDocument::GetMsgSymbByType(rPack.DocType, temp_buf);
		path.SetLastSlash().Cat(temp_buf);
		msg_uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		path.CatChar('_').Cat(temp_buf).DotCat("xml");
		//
		THROW(p_x = xmlNewTextWriterFilename(path, 0));
		Epp.GetExtStrData(Epp.extssFormatSymb, edi_format_symb);
		const int use_own_format = edi_format_symb.IsEqiAscii("eancom") ? 0 : 1;
		if(use_own_format) {
			switch(rPack.DocType) {
				case PPEDIOP_ORDER: THROW(Write_OwnFormat_ORDERS(p_x, msg_uuid, *p_bp)); break;
				case PPEDIOP_ORDERRSP: THROW(Write_OwnFormat_ORDERRSP(p_x, msg_uuid, *p_bp, static_cast<const PPBillPacket *>(rPack.P_ExtData))); break;
				case PPEDIOP_DESADV: THROW(Write_OwnFormat_DESADV(p_x, msg_uuid, *p_bp)); break;
				case PPEDIOP_ALCODESADV: THROW(Write_OwnFormat_ALCODESADV(p_x, msg_uuid, *p_bp)); break;
				case PPEDIOP_INVOIC: THROW(Write_OwnFormat_INVOIC(p_x, msg_uuid, *p_bp)); break;
				case PPEDIOP_RECADV: THROW(Write_OwnFormat_RECADV(p_x, msg_uuid, *p_recadv_pack)); break; // @v11.2.9
			}
		}
		else {
			PPEanComDocument s_doc(this);
			switch(rPack.DocType) {
				case PPEDIOP_ORDER: THROW(s_doc.Write_ORDERS(p_x, *p_bp)); break;
				case PPEDIOP_ORDERRSP: THROW(s_doc.Write_ORDERRSP(p_x, *p_bp, static_cast<const PPBillPacket *>(rPack.P_ExtData))); break;
				case PPEDIOP_DESADV: THROW(s_doc.Write_DESADV(p_x, *p_bp)); break;
				case PPEDIOP_ALCODESADV: /*THROW(s_doc.Write_DESADV(p_x, *p_bp));*/ break; // @todo
				case PPEDIOP_INVOIC: /*THROW(s_doc.Write_DESADV(p_x, *p_bp));*/ break; // @todo
				case PPEDIOP_RECADV: break; // @todo
			}
		}
		xmlFreeTextWriter(p_x);
		p_x = 0;
		if(!(Flags & ctrfTestMode)) {
			InetUrl url;
			THROW(Epp.MakeUrl(0, url));
			const int prot = url.GetProtocol();
			if(prot == InetUrl::protUnkn) {
				url.SetProtocol(InetUrl::protFtp);
			}
			if(prot == InetUrl::protFtp) {
				ScURL curl;
				const char * p_box_name = "Outbox";
				url.SetComponent(url.cPath, p_box_name);
				THROW(curl.FtpPut(url, ScURL::mfVerbose, path, 0, 0));
				pIdent->EdiOp = rPack.DocType;
				pIdent->Uuid = msg_uuid;
				pIdent->Box = p_box_name;
				pIdent->Time = getcurdatetime_();
				ok = 1;
			}
		}
	}
    CATCHZOK
	xmlFreeTextWriter(p_x);
	if(!ok && path.NotEmpty())
		SFile::Remove(path);
    return ok;
}
//
//
//
EdiProviderImplementation_Exite::EdiProviderImplementation_Exite(const PPEdiProviderPacket & rEpp, PPID mainOrgID, long flags) :
	PPEdiProcessor::ProviderImplementation(rEpp, mainOrgID, flags)
{
}

EdiProviderImplementation_Exite::~EdiProviderImplementation_Exite()
{
}

int EdiProviderImplementation_Exite::GetDocumentList(const PPBillIterchangeFilt & rP, PPEdiProcessor::DocumentInfoList & rList)
{
	int    ok = -1;
	SJson * p_query = 0;
	SJson * p_reply = 0;
	SString temp_buf;
	SString left, right;
	SString json_buf;
	SString log_buf;
	SString reply_message;
	int    reply_code = 0;
	SPathStruc ps;
	InetUrl url;
	THROW(Epp.MakeUrl(0, url));
	THROW(Auth());
	{
		ScURL c;
		SBuffer ack_buf;
		SFile wr_stream(ack_buf, SFile::mWrite);
		StrStrAssocArray hdr_flds;
		url.SetComponent(url.cPath, "Api/V1/Edo/Document/GetEdiDocs");
		THROW_SL(p_query = new SJson(SJson::tOBJECT));
		THROW_SL(p_query->InsertString("varToken", AT.Token));
		{
			if(checkdate(rP.Period.low)) {
				temp_buf.Z().Cat(rP.Period.low, DATF_ISO8601|DATF_CENTURY);
				THROW_SL(p_query->InsertString("timefrom", temp_buf));
				if(checkdate(rP.Period.upp)) {
					temp_buf.Z().Cat(rP.Period.upp, DATF_ISO8601|DATF_CENTURY);
					THROW_SL(p_query->InsertString("timeto", temp_buf));
				}
			}
		}
		// unread : отметка о прочтении; 0 - не прочитан, 1 - прочитан
		// doc_type : тип документа
		// timefrom : "ГГГГ-ММ-ДД ЧЧ:ММ:СС" / "ГГГГ-ММ-ДД ЧЧ:ММ" / "ГГГГ-ММ-ДД" дата начала поиска
		// timeto : "ГГГГ-ММ-ДД ЧЧ:ММ:СС" / "ГГГГ-ММ-ДД ЧЧ:ММ" / "ГГГГ-ММ-ДД" дата окончания поиска
		// limit : лимит на отображение документов (=1000 по умолчанию)
		// page : страница

		//THROW_SL(p_query->InsertString("doc_type", pDocType));
		url.Composite(0, temp_buf);
		PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "URL").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
		THROW_SL(p_query->ToStr(temp_buf));
		json_buf = temp_buf;
		//json_buf.EncodeUrl(temp_buf, 0);
		PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "Q").CatDiv(':', 2).Cat(json_buf), LOGMSGF_TIME|LOGMSGF_USER);
		SFileFormat::GetMime(SFileFormat::Json, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
		THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "R").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
				if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
					for(SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Type == SJson::tOBJECT) {
							for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
								if(p_obj->Text.IsEqiAscii("varMessage"))
									reply_message = p_obj->P_Child->Text;
								else if(p_obj->Text.IsEqiAscii("intCode"))
									reply_code = p_obj->P_Child->Text.ToLong();
								else if(p_obj->Text.IsEqiAscii("docs")) {
									if(p_obj->P_Child && p_obj->P_Child->Type == SJson::tARRAY) {
										for(SJson * p_doc = p_obj->P_Child->P_Child; p_doc; p_doc = p_doc->P_Next) {
											if(p_doc->Type == SJson::tOBJECT) {
												PPEdiProcessor::DocumentInfo new_entry;
												for(const SJson * p_df = p_doc->P_Child; p_df; p_df = p_df->P_Next) {
													if(p_df->Text.IsEqiAscii("intDocID"))
														new_entry.SId = p_df->P_Child->Text;
													else if(p_df->Text.IsEqiAscii("doc_type")) 
														new_entry.EdiOp = PPEdiProcessor::GetEdiMsgTypeByText(p_df->P_Child->Text);
													else if(p_df->Text.IsEqiAscii("date"))
														strtodatetime(p_df->P_Child->Text, &new_entry.Time, DATF_YMD, TIMF_HMS);
												}
												if(new_entry.SId.NotEmpty() && new_entry.EdiOp && checkdate(new_entry.Time.d)) {
													rList.Add(new_entry, 0);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			if(reply_code == 200)
				ok = 1;
			else if(reply_code == 404)
				ok = -1;
			else {
				temp_buf.Z().Cat("EXITE").Space().Cat(reply_code).Space().Cat(reply_message.Chomp()).Transf(CTRANSF_UTF8_TO_INNER);
				CALLEXCEPT_PP_S(PPERR_EDI_SVCGETDOCLIST, temp_buf);
			}
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

/*
1. Адрес сайта: https://e-vo.ru/
2. Логин: testTStest
3. Пароль: OVXmgv
*/

int EdiProviderImplementation_Exite::Auth()
{
	int    ok = -1;
	if(AT.Token.IsEmpty() || !AT.Tm || diffdatetimesec(getcurdatetime_(), AT.Tm) >= 3600) {
		THROW(Implement_Auth(AT.Token));
		AT.Tm = getcurdatetime_();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Exite::Implement_Auth(SString & rToken)
{
	int    ok = -1;
	SJson * p_query = 0;
	SJson * p_reply = 0;
	ScURL c;
	SString temp_buf;
	SString json_buf;
	SString log_buf;
	SString reply_message;
	int    reply_code = 0;
	InetUrl url;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	StrStrAssocArray hdr_flds;
	THROW(Epp.MakeUrl(0, url));
	url.SetComponent(url.cPath, "Api/V1/Edo/Index/Authorize");
	THROW_SL(p_query = new SJson(SJson::tOBJECT));
	url.GetComponent(url.cUserName, 0, temp_buf);
	THROW_SL(p_query->InsertString("varLogin", temp_buf.Transf(CTRANSF_INNER_TO_UTF8)));
	url.SetComponent(url.cUserName, 0);
	url.GetComponent(url.cPassword, 0, temp_buf);
	THROW_SL(p_query->InsertString("varPassword", temp_buf.Transf(CTRANSF_INNER_TO_UTF8)));
	url.SetComponent(url.cPassword, 0);
	url.Composite(0, temp_buf);
	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "URL").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
	THROW_SL(p_query->ToStr(temp_buf));
	json_buf = temp_buf;
	//json_buf.EncodeUrl(temp_buf, 0);

	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "Q").CatDiv(':', 2).Cat(json_buf), LOGMSGF_TIME|LOGMSGF_USER);
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
	THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
	THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
	{
		SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
		if(p_ack_buf) {
			temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
			PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "R").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
			if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
				for(SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Type == SJson::tOBJECT) {
						for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->Text.IsEqiAscii("varToken"))
								rToken = p_obj->P_Child->Text;
							else if(p_obj->Text.IsEqiAscii("varMessage"))
								reply_message = p_obj->P_Child->Text;
							else if(p_obj->Text.IsEqiAscii("intCode"))
								reply_code = p_obj->P_Child->Text.ToLong();
						}
					}
				}
			}
		}
		if(reply_code == 200)
			ok = 1;
		else {
			temp_buf.Z().Cat("EXITE").Space().Cat(reply_code).Space().Cat(reply_message.Chomp()).Transf(CTRANSF_UTF8_TO_INNER);
			CALLEXCEPT_PP_S(PPERR_EDI_SVCAUTH, temp_buf);
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

int EdiProviderImplementation_Exite::Helper_SendDocument(const char * pDocType, const char * pDocName, const char * pDocMime64, SString & rRetIdent)
{
	rRetIdent.Z();
	int    ok = -1;
	SJson * p_query = 0;
	SJson * p_reply = 0;
	ScURL c;
	SString temp_buf;
	SString json_buf;
	SString log_buf;
	SString reply_message;
	int    reply_code = 0;
	InetUrl url;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	StrStrAssocArray hdr_flds;
	THROW(Epp.MakeUrl(0, url));
	THROW(Auth());
	url.SetComponent(url.cPath, "Api/V1/Edo/Document/Send");
	THROW_SL(p_query = new SJson(SJson::tOBJECT));
	THROW_SL(p_query->InsertString("varToken", AT.Token));
	THROW_SL(p_query->InsertString("doc_type", pDocType));
	THROW_SL(p_query->InsertString("document_name", pDocName));
	THROW_SL(p_query->InsertString("body", pDocMime64));
	THROW_SL(p_query->Insert("return_id", json_new_number(temp_buf.Z().Cat(1))));
	url.Composite(0, temp_buf);
	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "URL").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
	THROW_SL(p_query->ToStr(temp_buf));
	json_buf = temp_buf;
	//json_buf.EncodeUrl(temp_buf, 0);

	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "Q").CatDiv(':', 2).Cat(json_buf), LOGMSGF_TIME|LOGMSGF_USER);
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
	THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
	THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
	{
		SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
		if(p_ack_buf) {
			temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
			PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "R").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
			if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
				for(SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Type == SJson::tOBJECT) {
						for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
							if(p_obj->Text.IsEqiAscii("varMessage"))
								reply_message = p_obj->P_Child->Text;
							else if(p_obj->Text.IsEqiAscii("intCode"))
								reply_code = p_obj->P_Child->Text.ToLong();
							else if(p_obj->Text.IsEqiAscii("intDocID"))
								rRetIdent = p_obj->P_Child->Text;
						}
					}
				}
			}
		}
		if(reply_code == 200)
			ok = 1;
		else {
			temp_buf.Z().Cat("EXITE").Space().Cat(reply_code).Space().Cat(reply_message.Chomp()).Transf(CTRANSF_UTF8_TO_INNER);
			CALLEXCEPT_PP_S(PPERR_EDI_SVCSENDDOC, temp_buf);
		}
	}
	CATCHZOK
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

int PPEdiProcessor::ProviderImplementation::ResolveDlvrLoc(const char * pText, PPBillPacket * pPack)
{
	int    ok = -1;
	if(!isempty(pText)) {
		PPIDArray loc_list_by_gln;
		PPID   final_dlvr_loc_id = 0;
		int    is_warehouse = 0;
		int    loc_type = 0;
		if(oneof4(pPack->Rec.EdiOp, PPEDIOP_DESADV, PPEDIOP_ORDERRSP, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC)) {
			loc_type = LOCTYP_WAREHOUSE;
		}
		else if(oneof2(pPack->Rec.EdiOp, PPEDIOP_ORDER, PPEDIOP_RECADV)) {
			loc_type = LOCTYP_ADDRESS;
		}
		if(loc_type) {
			PsnObj.LocObj.ResolveGLN(loc_type, pText, loc_list_by_gln);
			THROW_PP_S(loc_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLDLVRLOC, pText);
			for(uint i = 0; !final_dlvr_loc_id && i < loc_list_by_gln.getCount(); i++) {
				const PPID loc_id = loc_list_by_gln.get(i);
				LocationTbl::Rec loc_rec;
				if(PsnObj.LocObj.Fetch(loc_id, &loc_rec) > 0 && loc_rec.Type == loc_type) {
					if(loc_type == LOCTYP_WAREHOUSE) {
						final_dlvr_loc_id = loc_id;
						is_warehouse = 1;
					}
					else if(loc_type == LOCTYP_ADDRESS) {
						if(loc_rec.OwnerID) {
							PPID   acs_id = 0;
							PPID   psn_id = ObjectToPerson(pPack->Rec.Object, &acs_id);
							if(psn_id) {
								if(loc_rec.OwnerID == psn_id) {
									final_dlvr_loc_id = loc_id;
								}
							}
							else if(pPack->Rec.Object == 0) {
								final_dlvr_loc_id = loc_id;
							}
						}
					}
				}
			}
			THROW_PP_S(final_dlvr_loc_id, PPERR_EDI_UNBLRSLV_BILLDLVRLOC, pText);
			if(is_warehouse) {
				pPack->Rec.LocID = final_dlvr_loc_id;
				ok = 1;
			}
			else {
				PPFreight freight;
				pPack->GetFreight(&freight);
				freight.SetupDlvrAddr(final_dlvr_loc_id);
				pPack->SetFreight(&freight);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProcessor::ProviderImplementation::ResolveContractor(const char * pText, int partyQ, PPBillPacket * pPack)
{
	int    ok = 1;
	SString msg_buf;
	if(!isempty(pText) && pPack && pPack->Rec.EdiOp) {
		if(oneof2(partyQ, EDIPARTYQ_SHIPTO, EDIPARTYQ_CONSIGNEE)) {
			THROW(ResolveDlvrLoc(pText, pPack));
		}
		else if(partyQ == EDIPARTYQ_CONSIGNOR) {
		}
		else {
			const PPID reg_type_id = PPREGT_GLN;
			PPIDArray psn_list_by_gln;
			//PPIDArray loc_list_by_gln;
			PPIDArray ar_list;
			msg_buf.CatDivIfNotEmpty('/', 0).Cat(pText);
			THROW(PsnObj.GetListByRegNumber(reg_type_id, 0, pText, psn_list_by_gln));
			if(oneof2(partyQ, EDIPARTYQ_SELLER, EDIPARTYQ_SUPPLIER)) {
				if(oneof4(pPack->Rec.EdiOp, PPEDIOP_DESADV, PPEDIOP_ORDERRSP, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC)) {
					THROW_PP_S(psn_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
					THROW(ArObj.GetByPersonList(GetSupplAccSheet(), &psn_list_by_gln, &ar_list));
					THROW_PP_S(ar_list.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
					{
						PPBillPacket::SetupObjectBlock sob;
						THROW(pPack->SetupObject(ar_list.get(0), sob));
					}
				}
				else if(oneof2(pPack->Rec.EdiOp, PPEDIOP_ORDER, PPEDIOP_RECADV)) {
					PPID   main_org_id = 0;
					for(uint i = 0; !main_org_id && i < psn_list_by_gln.getCount(); i++) {
						const PPID _id = psn_list_by_gln.get(i);
						if(PsnObj.P_Tbl->IsBelongToKind(_id, PPPRK_MAIN) > 0)
							main_org_id = _id;
					}
					THROW_PP_S(main_org_id, PPERR_EDI_UNBLRSLV_BILLMAINORG, msg_buf);
				}
			}
			else if(oneof2(partyQ, EDIPARTYQ_BUYER, EDIPARTYQ_INVOICEE)) {
				if(oneof4(pPack->Rec.EdiOp, PPEDIOP_DESADV, PPEDIOP_ORDERRSP, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC)) {
					PPID   main_org_id = 0;
					for(uint i = 0; !main_org_id && i < psn_list_by_gln.getCount(); i++) {
						const PPID _id = psn_list_by_gln.get(i);
						if(PsnObj.P_Tbl->IsBelongToKind(_id, PPPRK_MAIN) > 0)
							main_org_id = _id;
					}
					THROW_PP_S(main_org_id, PPERR_EDI_UNBLRSLV_BILLMAINORG, msg_buf);
				}
				else if(oneof2(pPack->Rec.EdiOp, PPEDIOP_ORDER, PPEDIOP_RECADV)) {
					THROW_PP_S(psn_list_by_gln.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
					THROW(ArObj.GetByPersonList(GetSellAccSheet(), &psn_list_by_gln, &ar_list));
					THROW_PP_S(ar_list.getCount(), PPERR_EDI_UNBLRSLV_BILLOBJ, msg_buf);
					{
						PPBillPacket::SetupObjectBlock sob;
						THROW(pPack->SetupObject(ar_list.get(0), sob));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPEdiProcessor::ProviderImplementation::SearchLinkedBill(const char * pCode, LDATE dt, PPID arID, int ediOp, BillTbl::Rec * pBillRec)
{
	int    ok = -1;
	char   scode[64]; // @v11.1.12 [32]-->[64]
	BillCore * p_bt = P_BObj->P_Tbl;
	Reference * p_ref = PPRef;
	DBQ  * dbq = 0;
	union {
		BillTbl::Key1 k1;
		BillTbl::Key2 k2;
		BillTbl::Key3 k3;
	} k;
	int    idx = 2;
	STRNSCPY(scode, pCode);
	// @v11.1.12 BillCore::GetCode(scode);
	MEMSZERO(k);
	if(arID) {
		k.k3.Object = arID;
		k.k3.Dt     = dt;
		dbq = & (p_bt->Object == arID && p_bt->Dt == dt);
		idx = 3;
	}
	else {
		k.k1.Dt = dt;
		dbq = & (p_bt->Dt == dt);
		idx = 1;
	}
	SString temp_buf;
	BExtQuery q(p_bt, idx, 256);
	q.select(p_bt->ID, p_bt->Dt, p_bt->Code, p_bt->EdiOp, 0L).where(*dbq);
	for(q.initIteration(0, &k, spGe); ok < 0 && q.nextIteration() > 0;) {
		temp_buf = p_bt->data.Code;
		// @v11.1.12 BillCore::GetCode(temp_buf);
		if(temp_buf.NotEmptyS() && stricmp866(temp_buf, scode) == 0) {
			const PPID bill_id = p_bt->data.ID;
			S_GUID sent_guid;
			sent_guid.Z();
			int    is_suited = 0;
			if(ediOp == PPEDIOP_ORDER) {
				if(p_ref->Ot.GetTagGuid(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIORDERSENT, sent_guid) > 0)
					is_suited = 1;
			}
			else if(ediOp == PPEDIOP_DESADV) {
				if(p_ref->Ot.GetTagGuid(PPOBJ_BILL, bill_id, PPTAG_BILL_EDIDESADVSENT, sent_guid) > 0)
					is_suited = 1;
			}
			else
				is_suited = 1;
			if(is_suited) {
				if(!pBillRec || p_bt->Search(bill_id, pBillRec) > 0) {
					ok = 1;
				}
			}
		}
	}
	return ok;
}

struct Exite_PositionBlock {
	Exite_PositionBlock() : GoodsID_ByGTIN(0), GoodsID_ByArCode(0), OrdQtty(0.0), AccQtty(0.0), DlvrQtty(0.0),
		PriceWithVat(0.0), PriceWithoutVat(0.0), Vat(0.0)
	{
	}
	int    Init(const BillTbl::Rec * pBillRec)
	{
		int    ok = 1;
		GoodsID_ByGTIN = 0;
		GoodsID_ByArCode = 0;
		ArGoodsCode.Z();
		GoodsName.Z();
		GTIN.Z();
		OrdQtty = 0.0;
		AccQtty = 0.0;
		DlvrQtty = 0.0;
		PriceWithVat = 0.0;
		PriceWithoutVat = 0.0;
		Vat = 0.0;
		THROW(Ti.Init(pBillRec, 1, 0));
		CATCHZOK
		return ok;
	}
	PPID   GoodsID_ByGTIN;
	PPID   GoodsID_ByArCode;
	SString ArGoodsCode;
	SString GoodsName;
	SString GTIN;
	double OrdQtty;  // заказанное количество
	double AccQtty;  // количество, принятое к исполнению
	double DlvrQtty; // Доставленное количество (DESADV)
	double PriceWithVat;
	double PriceWithoutVat;
	double Vat; // Ставка НДС в процентах
	PPTransferItem Ti;
};

int EdiProviderImplementation_Exite::ReceiveDocument(const PPEdiProcessor::DocumentInfo * pIdent, TSCollection <PPEdiProcessor::Packet> & rList)
{
	int    ok = -1;
	PPEdiProcessor::Packet * p_pack = 0;
	PPBillPacket * p_bpack = 0; // is owned by p_pack
	SJson * p_query = 0;
	SJson * p_reply = 0;
	ScURL c;
	SString temp_buf;
	SString json_buf;
	SString log_buf;
	SString reply_message;
	SString addendum_msg_buf;
	Exite_PositionBlock pos_blk;
	int    reply_code = 0;
	InetUrl url;
	SBuffer ack_buf;
	SBuffer * p_ack_buf = 0;
	SFile wr_stream(ack_buf, SFile::mWrite);
	StrStrAssocArray hdr_flds;
	PPObjOprKind op_obj;
	//
	// Следующие 2 вида операций являются зарезервированными и создаются автоматически (если отсутствуют) вызовами ниже {
	//
	PPID   ordrsp_op_id = 0;
	PPID   recadv_op_id = 0;
	switch(pIdent->EdiOp) {
		case PPEDIOP_ORDERRSP: THROW(op_obj.GetEdiOrdrspOp(&ordrsp_op_id, 1)); break;
		case PPEDIOP_RECADV: THROW(op_obj.GetEdiRecadvOp(&recadv_op_id, 1)); break;
		case PPEDIOP_ORDER:  THROW_PP(ACfg.Hdr.OpID, PPERR_EDI_OPNDEF_ORDER); break;
		case PPEDIOP_DESADV: THROW_PP(ACfg.Hdr.EdiDesadvOpID, PPERR_EDI_OPNDEF_DESADV); break;
	}
	// }
	THROW(Epp.MakeUrl(0, url));
	THROW(Auth());
	url.SetComponent(url.cPath, "Api/V1/Edo/Document/GetEdiDocBody");
	THROW_SL(p_query = new SJson(SJson::tOBJECT));
	THROW_SL(p_query->InsertString("varToken", AT.Token));
	THROW_SL(p_query->InsertString("intDocID", pIdent->SId));
	url.Composite(0, temp_buf);
	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "URL").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
	THROW_SL(p_query->ToStr(temp_buf));
	json_buf = temp_buf;
	//json_buf.EncodeUrl(temp_buf, 0);
	PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "Q").CatDiv(':', 2).Cat(json_buf), LOGMSGF_TIME|LOGMSGF_USER);
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
	THROW_SL(c.SetupDefaultSslOptions(0, SSystem::sslDefault, 0));
	THROW_SL(c.HttpPost(url, /*ScURL::mfDontVerifySslPeer|*/ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	if(p_ack_buf) {
		temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
		PPLogMessage(PPFILNAM_EDIEXITE_LOG, (log_buf = "R").CatDiv(':', 2).Cat(temp_buf), LOGMSGF_TIME|LOGMSGF_USER);
		if(json_parse_document(&p_reply, temp_buf.cptr()) == JSON_OK) {
			const  int edi_op = pIdent->EdiOp;
			for(SJson * p_cur = p_reply; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("intDocID")) {
							//rRetIdent = p_obj->P_Child->Text;
						}
						else if(p_obj->Text.IsEqiAscii("body") && p_obj->P_Child && p_obj->P_Child->Type == SJson::tOBJECT) {
							int    edipartyq_whoami = 0; // EDIPARTYQ_BUYER || EDIPARTYQ_SUPPLIER
							if(oneof4(edi_op, PPEDIOP_ORDERRSP, PPEDIOP_DESADV, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC))
								edipartyq_whoami = EDIPARTYQ_BUYER;
							else if(oneof2(edi_op, PPEDIOP_ORDER, PPEDIOP_RECADV))
								edipartyq_whoami = EDIPARTYQ_SUPPLIER;
							PPID   link_order_id = 0;
							SString order_number;
							SString delivery_note_number;
							LDATE  delivery_note_date = ZERODATE;
							LDATE  order_date = ZERODATE;
							LDATETIME delivery_dtm = ZERODATETIME;
							PPID   sender_psn_id = 0;
							PPID   rcvr_psn_id = 0;
							PPID   contractor_ar_id = 0;
							Goods2Tbl::Rec goods_rec;
							BarcodeTbl::Rec bc_rec;
							THROW_MEM(p_pack = new PPEdiProcessor::Packet(edi_op));
							//addendum_msg_buf.Z().Cat("ORDERRSP").Space().Cat(attrs.Num).Space().Cat(attrs.Dt, DATF_DMY);
							if(edi_op == PPEDIOP_DESADV) {
								p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
								THROW_PP_S(p_bpack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
								THROW(p_bpack->CreateBlank_WithoutCode(ACfg.Hdr.EdiDesadvOpID, 0, 0, 1));
								p_bpack->Rec.EdiOp = PPEDIOP_DESADV;
								for(SJson * p_bf = p_obj->P_Child->P_Child; p_bf; p_bf = p_bf->P_Next) {
									temp_buf = p_bf->P_Child->Text;
									if(p_bf->Text.IsEqiAscii("HEAD")) {
										if(p_bf->P_Child && p_bf->P_Child->Type == SJson::tARRAY) {
											for(SJson * p_hi = p_bf->P_Child->P_Child; p_hi; p_hi = p_hi->P_Next) {
												if(p_hi->Type == SJson::tOBJECT) {
													for(SJson * p_hf = p_hi->P_Child; p_hf; p_hf = p_hf->P_Next) {
														temp_buf = p_hf->P_Child->Text;
														if(p_hf->Text.IsEqiAscii("SUPPLIER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SUPPLIER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("SUPPLIERNAME")) {
														}
														else if(p_hf->Text.IsEqiAscii("BUYER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_BUYER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("BUYERCODE")) {
														}
														else if(p_hf->Text.IsEqiAscii("DELIVERYPLACE")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SHIPTO, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("FINALRECIPIENT")) {
														}
														else if(p_hf->Text.IsEqiAscii("SENDER")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &sender_psn_id) > 0, PPERR_EDI_UNBLRSLV_SENDER, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("SENDERNAME")) {
														}
														else if(p_hf->Text.IsEqiAscii("SENDERPHONE")) {
														}
														else if(p_hf->Text.IsEqiAscii("SENDERCITY")) {
														}
														else if(p_hf->Text.IsEqiAscii("SENDERADRESS")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENT")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &rcvr_psn_id) > 0, PPERR_EDI_UNBLRSLV_RCVR, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("EDIINTERCHANGEID")) {
														}
														else if(p_hf->Text.IsEqiAscii("PACKINGSEQUENCE")) {
															if(p_hf->P_Child && p_hf->P_Child->Type == SJson::tARRAY) {
																for(SJson * p_psi = p_hf->P_Child->P_Child; p_psi; p_psi = p_psi->P_Next) {
																	if(p_psi->Type == SJson::tOBJECT) {
																		for(SJson * p_psf = p_psi->P_Child; p_psf; p_psf = p_psf->P_Next) {
																			if(p_psf->Text.IsEqiAscii("HIERARCHICALID")) {
																			}
																			else if(p_psf->Text.IsEqiAscii("POSITION")) {
																				if(p_psf->P_Child && p_psf->P_Child->Type == SJson::tARRAY) {
																					for(SJson * p_pli = p_psf->P_Child->P_Child; p_pli; p_pli = p_pli->P_Next) {
																						if(p_pli->Type == SJson::tOBJECT) {
																							THROW(pos_blk.Init(&p_bpack->Rec));
																							for(SJson * p_pf = p_pli->P_Child; p_pf; p_pf = p_pf->P_Next) {
																								temp_buf = p_pf->P_Child->Text;
																								if(p_pf->Text.IsEqiAscii("POSITIONNUMBER")) {
																									pos_blk.Ti.RByBill = static_cast<int16>(temp_buf.ToLong());
																									if(pos_blk.Ti.RByBill < 0)
																										pos_blk.Ti.RByBill = 0;
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODUCT")) {
																									pos_blk.GTIN = temp_buf;
																									if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec, 1) > 0)
																										pos_blk.GoodsID_ByGTIN = goods_rec.ID;
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODUCTIDSUPPLIER")) {
																									if(edipartyq_whoami == EDIPARTYQ_SUPPLIER)
																										pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																									else {
																										pos_blk.ArGoodsCode = temp_buf;
																										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																											pos_blk.GoodsID_ByArCode = goods_rec.ID;
																									}
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODUCTIDBUYER")) {
																									if(edipartyq_whoami == EDIPARTYQ_BUYER)
																										pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																									else {
																										pos_blk.ArGoodsCode = temp_buf;
																										if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																											pos_blk.GoodsID_ByArCode = goods_rec.ID;
																									}
																								}
																								else if(p_pf->Text.IsEqiAscii("DELIVEREDQUANTITY")) {
																									pos_blk.DlvrQtty = temp_buf.ToReal();
																								}
																								else if(p_pf->Text.IsEqiAscii("DELIVEREDUNIT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("ORDEREDQUANTITY")) {
																									pos_blk.OrdQtty = temp_buf.ToReal();
																								}
																								else if(p_pf->Text.IsEqiAscii("ORDERUNIT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("QUANTITYOFCUINTU")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("BOXESQUANTITY")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODINN")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODKPP")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PARTYNAME")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PRODUCTIONCODE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("DISPLACEMENT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("BOXWEIGHT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("IMPORTPARTYNAME")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("LICENSE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("LICGIVEN")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("LICSTART")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("LICEND")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("INVOICEDQUANTITY")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PORTAL_CERT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("MANUFACTUREDATE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CERTSTART")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CERTEND")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CERTS_URL")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("EGAIS")) {
																									// EGAISCODE, EGAISQUANTITY
																								}
																								else if(p_pf->Text.IsEqiAscii("SHELFLIFEDATE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("EXPIREDATE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("INVOICEUNIT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("AMOUNT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("COUNTRYORIGIN")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CUSTOMSTARIFFNUMBER")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PRICE")) { // без НДС
																									pos_blk.PriceWithoutVat = temp_buf.ToReal();
																								}
																								else if(p_pf->Text.IsEqiAscii("PRICEWITHVAT")) {
																									pos_blk.PriceWithVat = temp_buf.ToReal();
																								}
																								else if(p_pf->Text.IsEqiAscii("AMOUNTWITHVAT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("DISCOUNT")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("TAXRATE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CONDITIONSTATUS")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("DESCRIPTION")) {
																									pos_blk.GoodsName = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER);
																								}
																								else if(p_pf->Text.IsEqiAscii("PACKAGEID")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("PARTNUMBER")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("CERTIFICATE")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("MINIMUMORDERQUANTITY")) {
																								}
																								else if(p_pf->Text.IsEqiAscii("VETDOCUMENT")) {
																									// BATCHID, UUID, VOLUME, DATEOFPRODUCTION, TIMEOFPRODUCTION
																								}
																							}
																							if(pos_blk.GoodsID_ByGTIN)
																								pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByGTIN;
																							else if(pos_blk.GoodsID_ByArCode)
																								pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByArCode;
																							if(pos_blk.Ti.GoodsID && pos_blk.DlvrQtty > 0.0 && pos_blk.Ti.RByBill) {
																								pos_blk.Ti.Quantity_ = pos_blk.DlvrQtty;
																								if(pos_blk.PriceWithVat > 0.0)
																									pos_blk.Ti.Cost = pos_blk.PriceWithVat;
																								else if(pos_blk.PriceWithoutVat > 0.0) {
																									double result_price = pos_blk.PriceWithoutVat;
																									PPGoodsTaxEntry gtx;
																									if(GObj.FetchTax(pos_blk.Ti.GoodsID, getcurdate_(), p_bpack->Rec.OpID, &gtx) > 0) {
																										double tax_factor = 1.0;
																										GObj.MultTaxFactor(pos_blk.Ti.GoodsID, &tax_factor);
																										GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &result_price, 1);
																									}
																									pos_blk.Ti.Cost = result_price;
																								}
																								THROW(p_bpack->LoadTItem(&pos_blk.Ti, 0, 0));
																							}
																						}
																					}
																				}
																			}
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
									else if(p_bf->Text.IsEqiAscii("NUMBER")) {
										temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Code, sizeof(p_bpack->Rec.Code));
									}
									else if(p_bf->Text.IsEqiAscii("DATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											p_bpack->Rec.Dt = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											delivery_dtm.d = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYTIME")) {
										LTIME t = ZEROTIME;
										strtotime(temp_buf, TIMF_HMS, &t);
										delivery_dtm.t = t;
									}
									else if(p_bf->Text.IsEqiAscii("ORDERNUMBER")) {
										order_number = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER);
									}
									else if(p_bf->Text.IsEqiAscii("ORDERDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											order_date = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYNOTENUMBER")) {
										delivery_note_number = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER);
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYNOTEDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											delivery_note_date = dt;										
									}
									else if(p_bf->Text.IsEqiAscii("TTNNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("SELFSHIPMENT")) {
									}
									else if(p_bf->Text.IsEqiAscii("WAYBILLNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("WAYBILLDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("INVOICENUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("INVOICEDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("UPDNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("UPDDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("SFAKTNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("SFAKTDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("EGAISNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("SUPPLIERORDENUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("SUPPLIERORDERDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("TOTALPACKAGES")) {
									}
									else if(p_bf->Text.IsEqiAscii("INFO")) {
										// @v11.1.12 temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Memo, sizeof(p_bpack->Rec.Memo));
										p_bpack->SMemo = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER); // @v11.1.12
									}
									else if(p_bf->Text.IsEqiAscii("SHIPMENTS")) {
									}
									else if(p_bf->Text.IsEqiAscii("CAMPAIGNNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTQUANTITY")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTMARK")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTID")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTERNAME")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTERTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("CARRIERNAME")) {
									}
									else if(p_bf->Text.IsEqiAscii("CARRIERINN")) {
									}
									else if(p_bf->Text.IsEqiAscii("CARRIERKPP")) {
									}
									else if(p_bf->Text.IsEqiAscii("PACKAGEWIGHT")) {
									}
								}
								if(order_number.NotEmptyS() && checkdate(order_date)) {
									BillTbl::Rec ord_bill_rec;
									if(SearchLinkedBill(order_number, order_date, p_bpack->Rec.Object, PPEDIOP_ORDER, &ord_bill_rec) > 0)
										p_bpack->Rec.LinkBillID = ord_bill_rec.ID;
								}
								if(delivery_note_number.NotEmptyS()) {
									if(!delivery_note_number.IsEqNC(p_bpack->Rec.Code)) {
										p_bpack->BTagL.PutItemStr(PPTAG_BILL_OUTERCODE, delivery_note_number);
									}
									if(checkdate(delivery_note_date)) {
										ObjTagItem tag_item;
										if(tag_item.SetDate(PPTAG_BILL_OUTERDATE, delivery_note_date) > 0)
											p_bpack->BTagL.PutItem(PPTAG_BILL_OUTERDATE, &tag_item);
									}
								}
							}
							else if(edi_op == PPEDIOP_ORDER) {
								p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
								THROW_PP_S(p_bpack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
								THROW(p_bpack->CreateBlank_WithoutCode(ACfg.Hdr.OpID, 0, 0, 1));
								p_bpack->Rec.EdiOp = PPEDIOP_ORDER;
								for(SJson * p_bf = p_obj->P_Child->P_Child; p_bf; p_bf = p_bf->P_Next) {
									temp_buf = p_bf->P_Child->Text;
									if(p_bf->Text.IsEqiAscii("HEAD")) {
										if(p_bf->P_Child && p_bf->P_Child->Type == SJson::tARRAY) {
											for(SJson * p_hi = p_bf->P_Child->P_Child; p_hi; p_hi = p_hi->P_Next) {
												if(p_hi->Type == SJson::tOBJECT) {
													for(SJson * p_hf = p_hi->P_Child; p_hf; p_hf = p_hf->P_Next) {
														temp_buf = p_hf->P_Child->Text;
														if(p_hf->Text.IsEqiAscii("SUPPLIER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SUPPLIER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("BUYER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_BUYER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("BUYERCODE")) {
														}
														else if(p_hf->Text.IsEqiAscii("DELIVERYPLACE")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SHIPTO, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("FINALRECIPIENT")) {
														}
														else if(p_hf->Text.IsEqiAscii("INVOICEPARTNER")) {
														}
														else if(p_hf->Text.IsEqiAscii("SENDER")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &sender_psn_id) > 0, PPERR_EDI_UNBLRSLV_SENDER, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENT")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &rcvr_psn_id) > 0, PPERR_EDI_UNBLRSLV_RCVR, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTCODE")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTNAME")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTCONTACTFACE")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTPHONE")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTCITY")) {
														}
														else if(p_hf->Text.IsEqiAscii("RECIPIENTADRESS")) {
														}
														else if(p_hf->Text.IsEqiAscii("EDIINTERCHANGEID")) {
														}
														else if(p_hf->Text.IsEqiAscii("POSITION")) {
															if(p_hf->P_Child && p_hf->P_Child->Type == SJson::tARRAY) {
																for(SJson * p_pli = p_hf->P_Child->P_Child; p_pli; p_pli = p_pli->P_Next) {
																	if(p_pli->Type == SJson::tOBJECT) {
																		THROW(pos_blk.Init(&p_bpack->Rec));
																		for(SJson * p_pf = p_pli->P_Child; p_pf; p_pf = p_pf->P_Next) {
																			temp_buf = p_pf->P_Child->Text;
																			if(p_pf->Text.IsEqiAscii("POSITIONNUMBER")) {
																				pos_blk.Ti.RByBill = static_cast<int16>(temp_buf.ToLong());
																				if(pos_blk.Ti.RByBill < 0)
																					pos_blk.Ti.RByBill = 0;
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCT")) {
																				pos_blk.GTIN = temp_buf;
																				if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec, 1) > 0)
																					pos_blk.GoodsID_ByGTIN = goods_rec.ID;
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCTIDSUPPLIER")) {
																				if(edipartyq_whoami == EDIPARTYQ_SUPPLIER)
																					pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																				else {
																					pos_blk.ArGoodsCode = temp_buf;
																					if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																						pos_blk.GoodsID_ByArCode = goods_rec.ID;
																				}
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCTIDBUYER")) {
																				if(edipartyq_whoami == EDIPARTYQ_BUYER)
																					pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																				else {
																					pos_blk.ArGoodsCode = temp_buf;
																					if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																						pos_blk.GoodsID_ByArCode = goods_rec.ID;
																				}
																			}
																			else if(p_pf->Text.IsEqiAscii("BUYERPARTNUMBER")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("ORDEREDQUANTITY")) {
																				pos_blk.OrdQtty = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("QUANTITYOFCUINTU")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("BOXESQUANTITY")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("ORDERUNIT")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("ORDERPRICE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("PRICEWITHVAT")) {
																				pos_blk.PriceWithVat = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("VAT")) {
																				pos_blk.Vat = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("CLAIMEDDELIVERYDATE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("CLAIMEDDELIVERYTIME")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("INFOCODED")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("MINIMUMORDERQUANTITY")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("INFO")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("COMPAIGNNUMBER")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("EARLIESTDELIVERYDATE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("LATESTDELIVERYDATE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("LATESTDELIVERYTIME")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("CONDITIONSTATUS")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("PACKAGEID")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("CHARACTERISTIC")) {
																				if(p_pf->P_Child && p_pf->P_Child->Type == SJson::tOBJECT) {
																					for(SJson * p_cf = p_pf->P_Child; p_cf; p_cf = p_cf->P_Next) {
																						if(p_cf->Text.IsEqiAscii("DESCRIPTION")) {
																						}
																					}
																				}
																			}
																		}
																		if(pos_blk.GoodsID_ByGTIN)
																			pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByGTIN;
																		else if(pos_blk.GoodsID_ByArCode)
																			pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByArCode;
																		if(pos_blk.Ti.GoodsID && pos_blk.OrdQtty > 0.0 && pos_blk.Ti.RByBill) {
																			pos_blk.Ti.Quantity_ = pos_blk.OrdQtty;
																			if(pos_blk.PriceWithVat > 0.0)
																				pos_blk.Ti.Price = pos_blk.PriceWithVat;
																			else if(pos_blk.PriceWithoutVat > 0.0) {
																				double result_price = pos_blk.PriceWithoutVat;
																				PPGoodsTaxEntry gtx;
																				if(GObj.FetchTax(pos_blk.Ti.GoodsID, getcurdate_(), p_bpack->Rec.OpID, &gtx) > 0) {
																					double tax_factor = 1.0;
																					GObj.MultTaxFactor(pos_blk.Ti.GoodsID, &tax_factor);
																					GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &result_price, 1);
																				}
																				pos_blk.Ti.Price = result_price;
																			}
																			THROW(p_bpack->LoadTItem(&pos_blk.Ti, 0, 0));
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
									else if(p_bf->Text.IsEqiAscii("DOCUMENTNAME")) {
									}
									else if(p_bf->Text.IsEqiAscii("NUMBER")) {
										temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Code, sizeof(p_bpack->Rec.Code));
									}
									else if(p_bf->Text.IsEqiAscii("DATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											p_bpack->Rec.Dt = dt;
									}
									else if(p_bf->Text.IsEqiAscii("TIME")) {
									}
									else if(p_bf->Text.IsEqiAscii("PROMO")) {
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											delivery_dtm.d = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYTIME")) {
										LTIME t = ZEROTIME;
										strtotime(temp_buf, TIMF_HMS, &t);
										delivery_dtm.t = t;
									}
									else if(p_bf->Text.IsEqiAscii("SHIPMENTDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("CAMPAIGNNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("CURRENCY")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTQUANTITY")) {
									}
									else if(p_bf->Text.IsEqiAscii("VAT")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTATIONTYPES")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTATIONMEANS")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTATIONPAYMENTTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTATIONROUTE")) {
									}
									else if(p_bf->Text.IsEqiAscii("BLANKETORDERNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("INFOCODED")) {
									}
									else if(p_bf->Text.IsEqiAscii("DOCTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("SUPORDER")) { // Номер заказа поставщика
									}
									else if(p_bf->Text.IsEqiAscii("KDKNUM")) { // Номер общего заказа КДК
									}
									else if(p_bf->Text.IsEqiAscii("ORDRTYPE")) { // Тип заказа: поле O - оригинал R - корректировка, D - отмена
									}
									else if(p_bf->Text.IsEqiAscii("INFO")) {
										// @v11.1.12 temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Memo, sizeof(p_bpack->Rec.Memo));
										p_bpack->SMemo = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER); // @v11.1.12
									}
									else if(p_bf->Text.IsEqiAscii("TYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("EARLIESTDELIVERYDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("PRODUCTTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("LATESTDELIVERYDATE")) {
									}
									else if(p_bf->Text.IsEqiAscii("LIMES")) {
										// []
										/*if(p_bf->Text.IsEqiAscii("LIMESNAME")) {
										}
										else if(p_bf->Text.IsEqiAscii("DATEFROM")) {
										}
										else if(p_bf->Text.IsEqiAscii("TIMEFROM")) {
										}
										else if(p_bf->Text.IsEqiAscii("DATETO")) {
										}
										else if(p_bf->Text.IsEqiAscii("TIMETO")) {
										}*/
									}
								}
							}
							else if(edi_op == PPEDIOP_RECADV) {
								;
							}
							else if(edi_op == PPEDIOP_INVOIC) {
								;
							}
							else if(edi_op == PPEDIOP_ORDERRSP) {
								p_bpack = static_cast<PPBillPacket *>(p_pack->P_Data);
								THROW_PP_S(p_bpack, PPERR_EDI_INBILLNOTINITED, addendum_msg_buf);
								THROW(p_bpack->CreateBlank_WithoutCode(ordrsp_op_id, 0, 0, 1));
								p_bpack->Rec.EdiOp = p_pack->DocType;
								for(SJson * p_bf = p_obj->P_Child->P_Child; p_bf; p_bf = p_bf->P_Next) {
									temp_buf = p_bf->P_Child->Text;
									if(p_bf->Text.IsEqiAscii("HEAD")) {
										if(p_bf->P_Child && p_bf->P_Child->Type == SJson::tARRAY) {
											for(SJson * p_hi = p_bf->P_Child->P_Child; p_hi; p_hi = p_hi->P_Next) {
												if(p_hi->Type == SJson::tOBJECT) {
													for(SJson * p_hf = p_hi->P_Child; p_hf; p_hf = p_hf->P_Next) {
														temp_buf = p_hf->P_Child->Text;
														if(p_hf->Text.IsEqiAscii("RECIPIENT")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &rcvr_psn_id) > 0, PPERR_EDI_UNBLRSLV_RCVR, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("BUYER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_BUYER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("DELIVERYPLACE")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SHIPTO, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("SUPPLIER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_SUPPLIER, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("FINALRECIPIENT")) {
														}
														else if(p_hf->Text.IsEqiAscii("INVOICEPARTNER")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_INVOICEE, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("DELIVERYPOINT")) {
														}
														else if(p_hf->Text.IsEqiAscii("CONSIGNEE")) {
															THROW(ResolveContractor(temp_buf, EDIPARTYQ_CONSIGNEE, p_bpack));
														}
														else if(p_hf->Text.IsEqiAscii("SENDER")) {
															THROW_PP_S(PsnObj.ResolveGLN(temp_buf, &sender_psn_id) > 0, PPERR_EDI_UNBLRSLV_SENDER, temp_buf);
														}
														else if(p_hf->Text.IsEqiAscii("EDIINTERCHANGEID")) {
														}
														else if(p_hf->Text.IsEqiAscii("POSITION")) {
															if(p_hf->P_Child && p_hf->P_Child->Type == SJson::tARRAY) {
																for(SJson * p_pli = p_hf->P_Child->P_Child; p_pli; p_pli = p_pli->P_Next) {
																	if(p_pli->Type == SJson::tOBJECT) {
																		THROW(pos_blk.Init(&p_bpack->Rec));
																		for(SJson * p_pf = p_pli->P_Child; p_pf; p_pf = p_pf->P_Next) {
																			temp_buf = p_pf->P_Child->Text;
																			if(p_pf->Text.IsEqiAscii("PRODUCT")) {
																				pos_blk.GTIN = temp_buf;
																				if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec, 1) > 0)
																					pos_blk.GoodsID_ByGTIN = goods_rec.ID;
																			}
																			else if(p_pf->Text.IsEqiAscii("POSITIONNUMBER")) {
																				pos_blk.Ti.RByBill = static_cast<int16>(temp_buf.ToLong());
																				if(pos_blk.Ti.RByBill < 0)
																					pos_blk.Ti.RByBill = 0;
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCTIDSUPPLIER")) {
																				if(edipartyq_whoami == EDIPARTYQ_SUPPLIER)
																					pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																				else {
																					pos_blk.ArGoodsCode = temp_buf;
																					if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																						pos_blk.GoodsID_ByArCode = goods_rec.ID;
																				}
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCTIDBUYER")) {
																				if(edipartyq_whoami == EDIPARTYQ_BUYER)
																					pos_blk.GoodsID_ByArCode = temp_buf.ToLong();
																				else {
																					pos_blk.ArGoodsCode = temp_buf;
																					if(GObj.P_Tbl->SearchByArCode(p_bpack->Rec.Object, temp_buf, 0, &goods_rec) > 0)
																						pos_blk.GoodsID_ByArCode = goods_rec.ID;
																				}
																			}
																			else if(p_pf->Text.IsEqiAscii("PRICEWITHVAT")) {
																				pos_blk.PriceWithVat = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("VAT")) {
																				pos_blk.Vat = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("PRODUCTTYPE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("AVAILABILITYDATE")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("PRICE")) {
																				pos_blk.PriceWithoutVat = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("ORDRSPUNIT")) {
																			}
																			else if(p_pf->Text.IsEqiAscii("ORDEREDQUANTITY")) {
																				pos_blk.OrdQtty = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("ACCEPTEDQUANTITY")) {
																				pos_blk.AccQtty = temp_buf.ToReal();
																			}
																			else if(p_pf->Text.IsEqiAscii("DESCRIPTION")) {
																				pos_blk.GoodsName = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER);
																			}
																		}
																		if(pos_blk.GoodsID_ByGTIN)
																			pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByGTIN;
																		else if(pos_blk.GoodsID_ByArCode)
																			pos_blk.Ti.GoodsID = pos_blk.GoodsID_ByArCode;
																		if(pos_blk.Ti.GoodsID && (pos_blk.OrdQtty > 0.0 || pos_blk.AccQtty > 0.0) && pos_blk.Ti.RByBill) {
																			pos_blk.Ti.Quantity_ = pos_blk.AccQtty;
																			if(pos_blk.PriceWithVat > 0.0)
																				pos_blk.Ti.Cost = pos_blk.PriceWithVat;
																			else if(pos_blk.PriceWithoutVat > 0.0) {
																				double result_price = pos_blk.PriceWithoutVat;
																				PPGoodsTaxEntry gtx;
																				if(GObj.FetchTax(pos_blk.Ti.GoodsID, getcurdate_(), p_bpack->Rec.OpID, &gtx) > 0) {
																					double tax_factor = 1.0;
																					GObj.MultTaxFactor(pos_blk.Ti.GoodsID, &tax_factor);
																					GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &result_price, 1);
																				}
																				pos_blk.Ti.Cost = result_price;
																			}
																			THROW(p_bpack->LoadTItem(&pos_blk.Ti, 0, 0));
																		}
																	}
																}
															}
														}
													}
												}
											}
										}
									}
									else if(p_bf->Text.IsEqiAscii("NUMBER")) {
										temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Code, sizeof(p_bpack->Rec.Code));
									}
									else if(p_bf->Text.IsEqiAscii("DATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											p_bpack->Rec.Dt = dt;
									}
									else if(p_bf->Text.IsEqiAscii("TIME")) {
									}
									else if(p_bf->Text.IsEqiAscii("ORDERNUMBER")) {
										order_number = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER);
									}
									else if(p_bf->Text.IsEqiAscii("ORDERDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											order_date = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYDATE")) {
										LDATE dt = ZERODATE;
										strtodate(temp_buf, DATF_ISO8601, &dt);
										if(checkdate(dt))
											delivery_dtm.d = dt;
									}
									else if(p_bf->Text.IsEqiAscii("DELIVERYTIME")) {
										LTIME t = ZEROTIME;
										strtotime(temp_buf, TIMF_HMS, &t);
										delivery_dtm.t = t;
									}
									else if(p_bf->Text.IsEqiAscii("CURRENCY")) {
									}
									else if(p_bf->Text.IsEqiAscii("VAT")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTQUANTITY")) {
									}
									else if(p_bf->Text.IsEqiAscii("CAMPAIGNNUMBER")) {
									}
									else if(p_bf->Text.IsEqiAscii("ORDRTYPE")) {
									}
									else if(p_bf->Text.IsEqiAscii("BASED_ON_OTHER")) {
									}
									else if(p_bf->Text.IsEqiAscii("ACTION")) {
									}
									else if(p_bf->Text.IsEqiAscii("TOTALPACKAGES")) {
									}
									else if(p_bf->Text.IsEqiAscii("TOTALPACKAGESSPACE")) {
									}
									else if(p_bf->Text.IsEqiAscii("INFO")) {
										// @v11.1.12 temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER).CopyTo(p_bpack->Rec.Memo, sizeof(p_bpack->Rec.Memo));
										p_bpack->SMemo = temp_buf.Unescape().Transf(CTRANSF_UTF8_TO_INNER); // @v11.1.12
									}
									else if(p_bf->Text.IsEqiAscii("REASONDECREACEQUANTITYALL")) {
									}
									else if(p_bf->Text.IsEqiAscii("TRANSPORTATIONCONDITION")) {
									}
									else if(p_bf->Text.IsEqiAscii("LIMES")) {
										// []
									}
								}
								if(order_number.NotEmptyS() && checkdate(order_date)) {
									BillTbl::Rec ord_bill_rec;
									if(SearchLinkedBill(order_number, order_date, p_bpack->Rec.Object, PPEDIOP_ORDER, &ord_bill_rec) > 0) {
										p_bpack->Rec.LinkBillID = ord_bill_rec.ID;
										THROW(P_BObj->ExtractPacket(p_bpack->Rec.LinkBillID, static_cast<PPBillPacket *>(p_pack->P_ExtData)) > 0);
									}
								}
							}
						}
						else if(p_obj->Text.IsEqiAscii("varMessage"))
							reply_message = p_obj->P_Child->Text;
						else if(p_obj->Text.IsEqiAscii("intCode"))
							reply_code = p_obj->P_Child->Text.ToLong();
					}
				}
			}
		}
	}
	if(reply_code == 200)
		ok = 1;
	else {
		temp_buf.Z().Cat("EXITE").Space().Cat(reply_code).Space().Cat(reply_message.Chomp()).Transf(CTRANSF_UTF8_TO_INNER);
		CALLEXCEPT_PP_S(PPERR_EDI_SVCGETDOC, temp_buf);
	}
	if(p_pack) {
		if(p_bpack)
			p_bpack->BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, pIdent->SId);
		rList.insert(p_pack);
		p_pack = 0; // Перед выходом экземпляр разрушается
	}
	CATCHZOK
	delete p_pack;
	json_free_value(&p_reply);
	json_free_value(&p_query);
	return ok;
}

int EdiProviderImplementation_Exite::SendDocument(PPEdiProcessor::DocumentInfo * pIdent, PPEdiProcessor::Packet & rPack)
{
	int    ok = -1;
	xmlTextWriter * p_x = 0;
	xmlBuffer * p_xml_buf = 0;
	SString path;
	SString ret_doc_ident; // Идент документа, возвращаемый провайдером
	if(rPack.P_Data && oneof6(rPack.DocType, PPEDIOP_ORDER, PPEDIOP_ORDERRSP, PPEDIOP_DESADV, PPEDIOP_RECADV, PPEDIOP_ALCODESADV, PPEDIOP_INVOIC)) {
		const S_GUID msg_uuid(SCtrGenerate_);
		const PPEdiProcessor::RecadvPacket * p_recadv_pack = (rPack.DocType == PPEDIOP_RECADV) ? (PPEdiProcessor::RecadvPacket *)rPack.P_Data : 0;
		const PPBillPacket * p_bp = (rPack.DocType == PPEDIOP_RECADV) ? &p_recadv_pack->ABp : static_cast<const PPBillPacket *>(rPack.P_Data);
		SString temp_buf;
		SString doc_type_buf;
		SString doc_buf_raw;
		SString doc_buf_mime;
		//SString edi_format_symb;
		GetTempOutputPath(rPack.DocType, path);
		THROW_SL(::createDir(path.RmvLastSlash()));
		//
		PPEanComDocument::GetMsgSymbByType(rPack.DocType, temp_buf);
		path.SetLastSlash().Cat(temp_buf);
		msg_uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		path.CatChar('_').Cat(temp_buf).DotCat("xml");
		//
		THROW(p_xml_buf = xmlBufferCreate());
		THROW(p_x = xmlNewTextWriterMemory(p_xml_buf, 0));
		//Epp.GetExtStrData(Epp.extssFormatSymb, edi_format_symb);
		switch(rPack.DocType) {
			case PPEDIOP_ORDER:
				doc_type_buf = "ORDER";
				THROW(Write_OwnFormat_ORDERS(p_x, msg_uuid, *p_bp));
				break;
			case PPEDIOP_ORDERRSP:
				doc_type_buf = "ORDRSP";
				THROW(Write_OwnFormat_ORDERRSP(p_x, msg_uuid, *p_bp, static_cast<const PPBillPacket *>(rPack.P_ExtData)));
				break;
			case PPEDIOP_DESADV:
				doc_type_buf = "DESADV";
				THROW(Write_OwnFormat_DESADV(p_x, msg_uuid, *p_bp));
				break;
			case PPEDIOP_RECADV:
				doc_type_buf = "RECADV";
				THROW(Write_OwnFormat_RECADV(p_x, msg_uuid, *p_recadv_pack));
				break;
			//case PPEDIOP_ALCODESADV: THROW(Write_OwnFormat_ALCODESADV(p_x, msg_uuid, *p_bp)); break;
			case PPEDIOP_INVOIC:
				doc_type_buf = "INVOICE";
				THROW(Write_OwnFormat_INVOIC(p_x, msg_uuid, *p_bp));
				break;
		}
		xmlTextWriterFlush(p_x);
		doc_buf_raw.CopyFromN((char *)p_xml_buf->content, p_xml_buf->use);
		xmlFreeTextWriter(p_x);
		p_x = 0;
		{
			SFile f_out(path, SFile::mWrite);
			f_out.Write(doc_buf_raw, doc_buf_raw.Len());
		}
		if(!(Flags & ctrfTestMode)) {
			temp_buf.Z().Cat(msg_uuid, S_GUID::fmtIDL);
			doc_buf_mime.EncodeMime64(doc_buf_raw, doc_buf_raw.Len());
			THROW(Helper_SendDocument(doc_type_buf, temp_buf, doc_buf_mime, ret_doc_ident));
			pIdent->EdiOp = rPack.DocType;
			pIdent->Uuid = msg_uuid;
			pIdent->Box = 0;//p_box_name;
			pIdent->Time = getcurdatetime_();
			pIdent->SId = ret_doc_ident;
			ok = 1;
		}
	}
    CATCHZOK
	xmlFreeTextWriter(p_x);
	xmlBufferFree(p_xml_buf);
    return ok;
}

int EdiProviderImplementation_Exite::Write_OwnFormat_ORDERS(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString main_org_gln;
	SString contractor_gln;
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "ORDER");
	THROW(GetMainOrgGLN(main_org_gln));
	THROW(GetArticleGLN(rBp.Rec.Object, contractor_gln));
	{
		n_docs.PutInner("DOCUMENTNAME", "220"); // 218 order // во всех документах 220
		// @v11.1.12 n_docs.PutInner("NUMBER", EncXmlText(BillCore::GetCode(temp_buf = rBp.Rec.Code)));
		n_docs.PutInner("NUMBER", EncXmlText(temp_buf = rBp.Rec.Code)); // @v11.1.12 
		n_docs.PutInner("DATE", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		//n_docs.PutInner("PROMO", 0);
		n_docs.PutInner("DELIVERYDATE", temp_buf.Z().Cat(NZOR(rBp.Rec.DueDate, rBp.Rec.Dt), DATF_ISO8601|DATF_CENTURY));
		//n_docs.PutInner("DELIVERYTIME", 0);
		//n_docs.PutInner("SHIPMENTDATE", 0);
		//n_docs.PutInner("CAMPAIGNNUMBER", 0); // Номер договора на поставку
		n_docs.PutInner("CURRENCY", "RUB");
		//n_docs.PutInner("TRANSPORTQUANTITY", 0);
		/*{
			SXml::WNode n_inner(_doc, "LIMES");
			{
				n_inner.PutInner("LIMESNAME", 0);
				n_inner.PutInner("DATEFROM", 0);
				n_inner.PutInner("TIMEFROM", 0);
				n_inner.PutInner("DATETO", 0);
				n_inner.PutInner("TIMETO", 0);
			}
		}*/
		//n_docs.PutInner("VAT", 0);
		//n_docs.PutInner("TRANSPORTATIONTYPES", 0);
		//n_docs.PutInner("TRANSPORTATIONMEANS", 0);
		//n_docs.PutInner("TRANSPORTATIONPAYMENTTYPE", 0);
		//n_docs.PutInner("TRANSPORTATIONROUTE", 0); // Маршрут доставки
		//n_docs.PutInner("BLANKETORDERNUMBER", 0); // Номер бланкового заказа
		//n_docs.PutInner("INFOCODED", 0);
		n_docs.PutInner("DOCTYPE", "O"); // Тип документа: O — оригинал, R — замена, D — удаление
		//n_docs.PutInner("SUPORDER", 0);
		//n_docs.PutInner("KDKNUM", 0);
		//n_docs.PutInner("ORDRTYPE", "O"); // Тип заказа: поле O - оригинал R - корректировка, D - отмена
		// @V11.1.12 n_docs.PutInner("INFO", EncXmlText(rBp.Rec.Memo));
		n_docs.PutInner("INFO", EncXmlText(temp_buf = rBp.SMemo)); // @v11.1.12
		// n_docs.PutInner("TYPE", 0); // 1 - Оборудование, 2 - Расходные материалы, 3 - Оборудование и услуга.
		//n_docs.PutInner("EARLIESTDELIVERYDATE", 0);
		//n_docs.PutInner("PRODUCTTYPE", 0); // 1 - Оборудование, 2 - Услуга
		//n_docs.PutInner("LATESTDELIVERYDATE", 0);
		{
			SXml::WNode n_inner(_doc, "HEAD");
			{
				SString goods_code;
				SString goods_ar_code;
				Goods2Tbl::Rec goods_rec;
				n_inner.PutInner("SUPPLIER", contractor_gln); // GLN поставщика
				n_inner.PutInner("BUYER", main_org_gln); // GLN покупателя
				//n_inner.PutInner("BUYERCODE", 0); // Код покупателя
				THROW(GetLocGLN(rBp.Rec.LocID, temp_buf));
				n_inner.PutInner("DELIVERYPLACE", temp_buf); // GLN места доставки
				//n_inner.PutInner("FINALRECIPIENT", 0); // GLN конечного консигнатора
				//n_inner.PutInner("INVOICEPARTNER", 0); // GLN плательщика
				n_inner.PutInner("SENDER", main_org_gln); // GLN отправителя сообщения
				n_inner.PutInner("RECIPIENT", contractor_gln); // GLN получателя сообщения
				//n_inner.PutInner("RECIPIENTCODE", 0); // Код получателя
				GetArticleName(rBp.Rec.Object, temp_buf);
				if(temp_buf.NotEmptyS())
					n_inner.PutInner("RECIPIENTNAME", EncXmlText(temp_buf)); // Имя получателя
				//n_inner.PutInner("RECIPIENTCONTACTFACE", 0); // Контактное лицо
				//n_inner.PutInner("RECIPIENTPHONE", 0); // Телефон получателя
				//n_inner.PutInner("RECIPIENTCITY", 0); // Город получателя
				//n_inner.PutInner("RECIPIENTADRESS", 0); // Адрес получателя
				//n_inner.PutInner("EDIINTERCHANGEID", 0); // Номер транзакции
				for(uint i = 0; i < rBp.GetTCount(); i++) {
					const PPTransferItem & r_ti = rBp.ConstTI(i);
					SXml::WNode n_inner2(_doc, "POSITION");
					{
						const PPID goods_id = labs(r_ti.GoodsID);
						double qtty = fabs(r_ti.Quantity_);
						double cost = r_ti.Cost;
						double amount = cost * qtty;
						//
							GTaxVect tvect;
							tvect.CalcTI(r_ti, 0 /*opID*/, TIAMT_COST);
							const double amount_with_vat = tvect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
							const double amount_without_vat = tvect.GetValue(GTAXVF_AFTERTAXES);
							const double vat_rate = tvect.GetTaxRate(GTAX_VAT, 0);
							const double price_with_vat = R5(amount_with_vat / qtty);
							const double price_without_vat = R5(amount_without_vat / qtty);
						//
						THROW(GetGoodsInfo(goods_id, rBp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
						n_inner2.PutInner("POSITIONNUMBER", temp_buf.Z().Cat(r_ti.RByBill));
						n_inner2.PutInner("PRODUCT", goods_code);
						if(goods_ar_code.NotEmpty())
							n_inner2.PutInner("PRODUCTIDSUPPLIER", goods_ar_code);
						n_inner2.PutInner("PRODUCTIDBUYER", temp_buf.Z().Cat(goods_rec.ID)); // Внутренний номер в БД покупателя
						//n_inner2.PutInner("BUYERPARTNUMBER", 0); // Внутренний системный номер артикула в БД покупателя
						n_inner2.PutInner("ORDEREDQUANTITY", temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
						//n_inner2.PutInner("QUANTITYOFCUINTU", 0); // Количество в упаковке
						n_inner2.PutInner("BOXESQUANTITY", 0); // Количество упаковок
						n_inner2.PutInner("ORDERUNIT", "PCE"); // Единицы измерения
						n_inner2.PutInner("ORDERPRICE", temp_buf.Z().Cat(price_without_vat, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Цена продукта без НДС
						n_inner2.PutInner("PRICEWITHVAT", temp_buf.Z().Cat(price_with_vat, MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_OMITEPS))); // Цена продукта с НДС
						n_inner2.PutInner("VAT", temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 1, NMBF_NOTRAILZ)));
						//n_inner2.PutInner("CLAIMEDDELIVERYDATE", 0); // Объявленная дата доставки
						//n_inner2.PutInner("CLAIMEDDELIVERYTIME", 0); // Объявленное время доставки
						//n_inner2.PutInner("INFOCODED", 0); // Инфокод
						//n_inner2.PutInner("MINIMUMORDERQUANTITY", 0); // Минимальное заказанное количество
						//n_inner2.PutInner("INFO", 0); // Свободный текст
						//n_inner2.PutInner("COMPAIGNNUMBER", 0); // Номер поставщика
						//n_inner2.PutInner("EARLIESTDELIVERYDATE", 0); // Поставка не раньше указанной даты
						//n_inner2.PutInner("LATESTDELIVERYDATE", 0); // Поставка не позднее указанной даты
						//n_inner2.PutInner("LATESTDELIVERYTIME", 0); // Поставка не позднее указанного времени
						//n_inner2.PutInner("CONDITIONSTATUS", 0); // Статус кондиции
						//n_inner2.PutInner("PACKAGEID", 0); // Идентификатор упаковки
						{
							SXml::WNode n_inner3(_doc, "CHARACTERISTIC");
							n_inner3.PutInner("DESCRIPTION", EncXmlText(goods_rec.Name));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Exite::Write_OwnFormat_ORDERRSP(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp, const PPBillPacket * pExtBp)
{
	int    ok = 1;
	const  PPBillPacket & r_org_pack = DEREFPTROR(pExtBp, rBp);
	SString temp_buf;
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "ORDRSP");
	{
		n_docs.PutInner("NUMBER", 0);
		n_docs.PutInner("DATE", 0);
		n_docs.PutInner("TIME", 0);
		n_docs.PutInner("ORDERNUMBER", 0);
		n_docs.PutInner("ORDERDATE", 0);
		n_docs.PutInner("DELIVERYDATE", 0);
		n_docs.PutInner("DELIVERYTIME", 0);
		n_docs.PutInner("REASONDECREACEQUANTITYALL", 0);
		n_docs.PutInner("CURRENCY", "RUB");
		{
			SXml::WNode n_inner(_doc, "LIMES");
			{
				n_inner.PutInner("LIMESNAME", 0);
				n_inner.PutInner("DATEFROM", 0);
				n_inner.PutInner("TIMEFROM", 0);
				n_inner.PutInner("DATETO", 0);
				n_inner.PutInner("TIMETO", 0);
			}
		}
		n_docs.PutInner("VAT", 0);
		n_docs.PutInner("ACTION", 0);
		n_docs.PutInner("SELFSHIPMENT", 0);
		{
			SXml::WNode n_inner(_doc, "SELFSHIPLIMES");
			{
				n_inner.PutInner("SELFSHIPLIMECODE", 0);
				n_inner.PutInner("SELFSHIPLIMENAME", 0);
				n_inner.PutInner("SELFSHIPLIMEADDRES", 0);
				n_inner.PutInner("TOTALPACKAGESSPACE", 0);
				n_inner.PutInner("TRANSPORTTYPE", 0);
			}
		}
		n_docs.PutInner("TOTALPACKAGES", 0);
		n_docs.PutInner("TOTALPACKAGESSPACE", 0);
		n_docs.PutInner("TRANSPORTQUANTITY", 0);
		n_docs.PutInner("INFO", 0);
		{
			SXml::WNode n_inner(_doc, "HEAD");
			{
				n_inner.PutInner("BUYER", 0);
				n_inner.PutInner("BUYERCODE", 0);
				n_inner.PutInner("SUPPLIER", 0);
				n_inner.PutInner("DELIVERYPLACE", 0);
				n_inner.PutInner("FINALRECIPIENT", 0);
				n_inner.PutInner("INVOICEPARTNER", 0);
				n_inner.PutInner("SENDER", 0);
				n_inner.PutInner("RECIPIENT", 0);
				n_inner.PutInner("RECIPIENTCODE", 0);
				n_inner.PutInner("RECIPIENTNAME", 0);
				n_inner.PutInner("RECIPIENTCONTACTFACE", 0);
				n_inner.PutInner("RECIPIENTPHONE", 0);
				n_inner.PutInner("RECIPIENTCITY", 0);
				n_inner.PutInner("RECIPIENTADRESS", 0);
				n_inner.PutInner("EDIINTERCHANGEID", 0);
				for(uint i = 0; i < rBp.GetTCount(); i++) {
					const PPTransferItem & r_ti = rBp.ConstTI(i);
					SXml::WNode n_inner2(_doc, "POSITION");
					{
						n_inner2.PutInner("POSITIONNUMBER", 0);
						n_inner2.PutInner("PRODUCT", 0);
						n_inner2.PutInner("PRODUCTIDBUYER", 0);
						n_inner2.PutInner("PRODUCTIDSUPPLIER", 0);
						n_inner2.PutInner("ORDRSPUNIT", 0);
						n_inner2.PutInner("DESCRIPTION", 0);
						n_inner2.PutInner("PRICE", 0);
						n_inner2.PutInner("PRICEWITHVAT", 0);
						n_inner2.PutInner("VAT", 0);
						n_inner2.PutInner("PRODUCTTYPE", 0);
						n_inner2.PutInner("ORDEREDQUANTITY", 0);
						n_inner2.PutInner("ACCEPTEDQUANTITY", 0);
						n_inner2.PutInner("REASONDECREACEQUANTITY", 0);
						n_inner2.PutInner("BOXESQUANTITY", 0);
						n_inner2.PutInner("INFO", 0);
						n_inner2.PutInner("CONDITIONSTATUS", 0);
						n_inner2.PutInner("PACKAGEID", 0);
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}

int EdiProviderImplementation_Exite::Write_OwnFormat_DESADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "DESADV");
	{
		n_docs.PutInner("NUMBER", 0);
		n_docs.PutInner("DATE", 0);
		n_docs.PutInner("DELIVERYDATE", 0);
		n_docs.PutInner("DELIVERYTIME", 0);
		n_docs.PutInner("ORDERNUMBER", 0);
		n_docs.PutInner("ORDERDATE", 0);
		n_docs.PutInner("DELIVERYNOTENUMBER", 0);
		n_docs.PutInner("DELIVERYNOTEDATE", 0);
		n_docs.PutInner("TTNNUMBER", 0);
		n_docs.PutInner("SELFSHIPMENT", 0);
		n_docs.PutInner("WAYBILLNUMBER", 0);
		n_docs.PutInner("WAYBILLDATE", 0);
		n_docs.PutInner("INVOICENUMBER", 0);
		n_docs.PutInner("INVOICEDATE", 0);
		n_docs.PutInner("UPDNUMBER", 0);
		n_docs.PutInner("UPDDATE", 0);
		//n_docs.PutInner("SFAKTNUMBER", 0);
		//n_docs.PutInner("SFAKTDATE", 0);
		n_docs.PutInner("EGAISNUMBER", 0);
		n_docs.PutInner("SUPPLIERORDENUMBER", 0);
		n_docs.PutInner("SUPPLIERORDERDATE", 0);
		n_docs.PutInner("TOTALPACKAGES", 0);
		n_docs.PutInner("INFO", 0);
		n_docs.PutInner("SHIPMENTS", 0);
		n_docs.PutInner("CAMPAIGNNUMBER", 0);
		n_docs.PutInner("TRANSPORTQUANTITY", 0);
		n_docs.PutInner("TRANSPORTMARK", 0);
		n_docs.PutInner("TRANSPORTID", 0);
		n_docs.PutInner("TRANSPORTERNAME", 0);
		n_docs.PutInner("TRANSPORTTYPE", 0);
		n_docs.PutInner("TRANSPORTERTYPE", 0);
		n_docs.PutInner("CARRIERNAME", 0);
		n_docs.PutInner("CARRIERINN", 0);
		n_docs.PutInner("CARRIERKPP", 0);
		n_docs.PutInner("PACKAGEWIGHT", 0);
		{
			SXml::WNode n_inner(_doc, "HEAD");
			{
				n_inner.PutInner("SUPPLIER", 0);
				n_inner.PutInner("SUPPLIERNAME", 0);
				n_inner.PutInner("BUYER", 0);
				n_inner.PutInner("BUYERCODE", 0);
				n_inner.PutInner("DELIVERYPLACE", 0);
				n_inner.PutInner("FINALRECIPIENT", 0);
				n_inner.PutInner("SENDER", 0);
				n_inner.PutInner("SENDERNAME", 0);
				n_inner.PutInner("SENDERPHONE", 0);
				n_inner.PutInner("SENDERCITY", 0);
				n_inner.PutInner("SENDERADRESS", 0);
				n_inner.PutInner("RECIPIENT", 0);
				n_inner.PutInner("EDIINTERCHANGEID", 0);
				{
					SXml::WNode n_inner_ps(_doc, "PACKINGSEQUENCE");
					n_inner_ps.PutInner("HIERARCHICALID", 0);
					for(uint i = 0; i < rBp.GetTCount(); i++) {
						const PPTransferItem & r_ti = rBp.ConstTI(i);
						SXml::WNode n_inner2(_doc, "POSITION");
						{
							n_inner2.PutInner("POSITIONNUMBER", 0);
							n_inner2.PutInner("PRODUCT", 0);
							n_inner2.PutInner("PRODUCTIDSUPPLIER", 0);
							n_inner2.PutInner("PRODUCTIDBUYER", 0);
							n_inner2.PutInner("DELIVEREDQUANTITY", 0);
							n_inner2.PutInner("DELIVEREDUNIT", 0);
							n_inner2.PutInner("ORDEREDQUANTITY", 0);
							n_inner2.PutInner("ORDERUNIT", 0);
							n_inner2.PutInner("QUANTITYOFCUINTU", 0);
							n_inner2.PutInner("BOXESQUANTITY", 0);
							n_inner2.PutInner("PRODINN", 0);
							n_inner2.PutInner("PRODKPP", 0);
							n_inner2.PutInner("PARTYNAME", 0);
							n_inner2.PutInner("PRODUCTIONCODE", 0);
							n_inner2.PutInner("DISPLACEMENT", 0);
							n_inner2.PutInner("BOXWEIGHT", 0);
							n_inner2.PutInner("IMPORTPARTYNAME", 0);
							n_inner2.PutInner("LICENSE", 0);
							n_inner2.PutInner("LICGIVEN", 0);
							n_inner2.PutInner("LICSTART", 0);
							n_inner2.PutInner("LICEND", 0);
							n_inner2.PutInner("INVOICEDQUANTITY", 0);
							n_inner2.PutInner("PORTAL_CERT", 0);
							n_inner2.PutInner("MANUFACTUREDATE", 0);
							n_inner2.PutInner("CERTSTART", 0);
							n_inner2.PutInner("CERTEND", 0);
							n_inner2.PutInner("CERTS_URL", 0);
							{
								SXml::WNode n_egais(_doc, "EGAIS");
								{
									n_egais.PutInner("EGAISCODE", 0);
									n_egais.PutInner("EGAISQUANTITY", 0);
								}
							}
							n_inner2.PutInner("SHELFLIFEDATE", 0);
							n_inner2.PutInner("EXPIREDATE", 0);
							n_inner2.PutInner("INVOICEUNIT", 0);
							n_inner2.PutInner("AMOUNT", 0); // без ндс
							n_inner2.PutInner("COUNTRYORIGIN", 0);
							n_inner2.PutInner("CUSTOMSTARIFFNUMBER", 0); // гтд
							n_inner2.PutInner("PRICE", 0); // без ндс
							n_inner2.PutInner("PRICEWITHVAT", 0);
							n_inner2.PutInner("AMOUNTWITHVAT", 0);
							n_inner2.PutInner("DISCOUNT", 0);
							n_inner2.PutInner("TAXRATE", 0);
							n_inner2.PutInner("CONDITIONSTATUS", 0);
							n_inner2.PutInner("DESCRIPTION", 0);
							n_inner2.PutInner("PACKAGEID", 0);
							n_inner2.PutInner("PARTNUMBER", 0);
							n_inner2.PutInner("CERTIFICATE", 0);
							n_inner2.PutInner("MINIMUMORDERQUANTITY", 0);
							{
								SXml::WNode n_vetis(_doc, "VETDOCUMENT");
								{
									n_vetis.PutInner("BATCHID", 0);
									n_vetis.PutInner("UUID", 0);
									n_vetis.PutInner("VOLUME", 0);
									n_vetis.PutInner("DATEOFPRODUCTION", 0);
									n_vetis.PutInner("TIMEOFPRODUCTION", 0);
								}
							}
						}
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}

int EdiProviderImplementation_Exite::Write_OwnFormat_RECADV(xmlTextWriter * pX, const S_GUID & rIdent, const PPEdiProcessor::RecadvPacket & rRaPack)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	BillTbl::Rec order_bill_rec;
	BillTbl::Rec wroff_bill_rec;
	SString main_org_gln;
	SString contractor_gln;
	PersonTbl::Rec contractor_psn_rec;
	PPObjBill::MakeCodeString(&rRaPack.ABp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "RECADV");
	// @v10.6.4 MEMSZERO(order_bill_rec);
	// @v10.6.4 MEMSZERO(wroff_bill_rec);
	const PPID contractor_psn_id = ObjectToPerson(rRaPack.ABp.Rec.Object, 0);
	THROW(PsnObj.Search(contractor_psn_id, &contractor_psn_rec) > 0);
	THROW(GetMainOrgGLN(main_org_gln));
	THROW(GetArticleGLN(rRaPack.ABp.Rec.Object, contractor_gln));
	{
		SString desadv_code;
		// @v11.1.12 BillCore::GetCode(desadv_code = rRaPack.ABp.Rec.Code);
		desadv_code = rRaPack.ABp.Rec.Code; // @v11.1.12 
		if(rRaPack.WrOffBillID && P_BObj->Fetch(rRaPack.WrOffBillID, &wroff_bill_rec) > 0) {
			// @v11.1.12 BillCore::GetCode(temp_buf = wroff_bill_rec.Code);
			temp_buf = wroff_bill_rec.Code; // @v11.1.12 
			n_docs.PutInner("NUMBER", temp_buf);
			n_docs.PutInner("DATE", temp_buf.Z().Cat(wroff_bill_rec.Dt, DATF_ISO8601|DATF_CENTURY));
			n_docs.PutInner("RECEPTIONDATE", temp_buf.Z().Cat(wroff_bill_rec.Dt, DATF_ISO8601|DATF_CENTURY));
		}
		else {
			(temp_buf = desadv_code).CatChar('-').Cat("RA");
			n_docs.PutInner("NUMBER", EncXmlText(temp_buf));
			n_docs.PutInner("DATE", temp_buf.Z().Cat(rRaPack.ABp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
			n_docs.PutInner("RECEPTIONDATE", temp_buf.Z().Cat(rRaPack.ABp.Rec.Dt, DATF_ISO8601|DATF_CENTURY));
		}
		if(rRaPack.OrderBillID && P_BObj->Fetch(rRaPack.OrderBillID, &order_bill_rec) > 0) {
			// @v11.1.12 BillCore::GetCode(temp_buf = order_bill_rec.Code);
			temp_buf = order_bill_rec.Code; // @v11.1.12 
			n_docs.PutInner("ORDERNUMBER", EncXmlText(temp_buf));
			n_docs.PutInner("ORDERDATE", temp_buf.Z().Cat(order_bill_rec.Dt, DATF_ISO8601|DATF_CENTURY));
		}
		n_docs.PutInner("DESADVNUMBER", EncXmlText(desadv_code)); // Номер уведомления об отгрузке
		n_docs.PutInner("DESADVDATE", temp_buf.Z().Cat(rRaPack.ABp.Rec.Dt, DATF_ISO8601|DATF_CENTURY)); // Дата уведомления об отгрузке
		{
			SString delivery_note_number;
			if(rRaPack.ABp.BTagL.GetItemStr(PPTAG_BILL_OUTERCODE, delivery_note_number) <= 0)
				delivery_note_number = desadv_code;
			n_docs.PutInner("DELIVERYNOTENUMBER", EncXmlText(delivery_note_number)); // Номер накладной
		}
		{
			LDATE delivery_note_date = ZERODATE;
			const ObjTagItem * p_tag_item = rRaPack.ABp.BTagL.GetItem(PPTAG_BILL_OUTERDATE);
			if(!p_tag_item || p_tag_item->GetDate(&delivery_note_date) <= 0)
				delivery_note_date = rRaPack.ABp.Rec.Dt;
			n_docs.PutInner("DELIVERYNOTEDATE", temp_buf.Z().Cat(delivery_note_date, DATF_ISO8601|DATF_CENTURY)); // Дата накладной
		}
		//n_docs.PutInner("CAMPAIGNNUMBER", 0);
		//n_docs.PutInner("SUPPLIERORDENUMBER", 0);
		//n_docs.PutInner("SUPPLIERORDERDATE", 0);
		//n_docs.PutInner("TRANSPORTID", 0);
		//n_docs.PutInner("TOTALPACKAGES", 0);
		{
			double total_amount = 0.0;
			double total_amount_with_vat = 0.0;
			for(uint i = 0; i < rRaPack.ABp.GetTCount(); i++) {
				const  PPTransferItem & r_ti = rRaPack.ABp.ConstTI(i);
				uint   recadv_ti_pos = 0;
				const PPTransferItem * p_recadv_ti = rRaPack.RBp.SearchTI(r_ti.RByBill, &recadv_ti_pos) ? &rRaPack.RBp.ConstTI(recadv_ti_pos) : 0;
				GTaxVect tvect;
				tvect.CalcTI(p_recadv_ti ? *p_recadv_ti : r_ti, 0 /*opID*/, TIAMT_COST);
				const double amount_with_vat = tvect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
				const double amount_without_vat = tvect.GetValue(GTAXVF_AFTERTAXES);
				total_amount += amount_without_vat;
				total_amount_with_vat += amount_with_vat;
			}
			n_docs.PutInner("TOTALAMOUNT", temp_buf.Z().Cat(total_amount, MKSFMTD(0, 2, 0)));
			n_docs.PutInner("TOTALAMOUNTWITHVAT", temp_buf.Z().Cat(total_amount_with_vat, MKSFMTD(0, 2, 0)));
		}
		n_docs.PutInner("DOCTYPE", "O");
		// @v11.1.12 n_docs.PutInner("INFO", EncXmlText(rRaPack.RBp.Rec.Memo));
		n_docs.PutInner("INFO", EncXmlText(temp_buf = rRaPack.RBp.SMemo)); // @v11.1.12
		{
			SXml::WNode n_inner(_doc, "HEAD");
			{
				SString goods_code;
				SString goods_ar_code;
				Goods2Tbl::Rec goods_rec;
				n_inner.PutInner("SUPPLIER", contractor_gln);
				n_inner.PutInner("SUPPLIERNAME", EncXmlText(contractor_psn_rec.Name));
				n_inner.PutInner("BUYER", main_org_gln);
				THROW(GetLocGLN(NZOR(rRaPack.RBp.Rec.LocID, rRaPack.ABp.Rec.LocID), temp_buf));
				n_inner.PutInner("DELIVERYPLACE", temp_buf); // GLN места доставки
				n_inner.PutInner("FINALRECIPIENT", 0);
				n_inner.PutInner("SENDER", main_org_gln);
				GetMainOrgName(temp_buf);
				n_inner.PutInner("SENDERNAME", EncXmlText(temp_buf));
				//n_inner.PutInner("SENDERPHONE", 0);
				//n_inner.PutInner("SENDERCITY", 0);
				//n_inner.PutInner("SENDERADRESS", 0);
				n_inner.PutInner("RECIPIENT", contractor_gln);
				//n_inner.PutInner("RECIPIENTCODE", 0);
				n_inner.PutInner("RECIPIENTNAME", EncXmlText(contractor_psn_rec.Name));
				//n_inner.PutInner("RECIPIENTCONTACTFACE", 0);
				//n_inner.PutInner("RECIPIENTPHONE", 0);
				//n_inner.PutInner("RECIPIENTCITY", 0);
				//n_inner.PutInner("RECIPIENTADRESS", 0);
				//n_inner.PutInner("EDIINTERCHANGEID", 0);
				{
					SXml::WNode n_inner_ps(_doc, "PACKINGSEQUENCE");
					n_inner_ps.PutInner("HIERARCHICALID", "1");
					for(uint i = 0; i < rRaPack.ABp.GetTCount(); i++) {
						const PPTransferItem & r_ti = rRaPack.ABp.ConstTI(i);
						SXml::WNode n_inner2(_doc, "POSITION");
						{
							const PPID goods_id = labs(r_ti.GoodsID);
							double dlvr_qtty = fabs(fabs(r_ti.Quantity_));
							assert(dlvr_qtty == rRaPack.DesadvQttyList.Get(i+1));
							double ord_qtty = fabs(rRaPack.OrderedQttyList.Get(i+1));
							double acc_qtty = fabs(rRaPack.RecadvQttyList.Get(i+1));
							uint   recadv_ti_pos = 0;
							const PPTransferItem * p_recadv_ti = rRaPack.RBp.SearchTI(r_ti.RByBill, &recadv_ti_pos) ? &rRaPack.RBp.ConstTI(recadv_ti_pos) : 0;
							//double amount_with_vat = p_recadv_ti ? (p_recadv_ti->Cost * fabs(p_recadv_ti->Quantity_)) : (r_ti.Cost * fabs(r_ti.Quantity_));
							//
								GTaxVect tvect;
								tvect.CalcTI(DEREFPTROR(p_recadv_ti, r_ti), 0 /*opID*/, TIAMT_COST);
								const double amount_with_vat = tvect.GetValue(GTAXVF_AFTERTAXES|GTAXVF_VAT);
								const double amount_without_vat = tvect.GetValue(GTAXVF_AFTERTAXES);
								const double vat_rate = tvect.GetTaxRate(GTAX_VAT, 0);
								const double price_with_vat = R5(amount_with_vat / acc_qtty);
								const double price_without_vat = R5(amount_without_vat / acc_qtty);
							//
							THROW(GetGoodsInfo(goods_id, rRaPack.ABp.Rec.Object, &goods_rec, goods_code, goods_ar_code));
							n_inner2.PutInner("POSITIONNUMBER", temp_buf.Z().Cat(r_ti.RByBill));
							n_inner2.PutInner("PRODUCT", goods_code);
							if(goods_ar_code.NotEmpty())
								n_inner2.PutInner("PRODUCTIDSUPPLIER", goods_ar_code);
							n_inner2.PutInner("PRODUCTIDBUYER", temp_buf.Z().Cat(goods_rec.ID)); // Внутренний номер в БД покупателя
							//n_inner2.PutInner("BUYERPARTNUMBER", 0);
							n_inner2.PutInner("ACCEPTEDQUANTITY", temp_buf.Z().Cat(acc_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
							n_inner2.PutInner("ACCEPTEDUNIT", "PCE");
							//n_inner2.PutInner("NOTACCEPTEDREASON", 0);
							//n_inner2.PutInner("NOTACCEPTEDQUANTITY", 0);
							n_inner2.PutInner("DELIVERQUANTITY", temp_buf.Z().Cat(dlvr_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
							n_inner2.PutInner("ORDERQUANTITY", temp_buf.Z().Cat(ord_qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_OMITEPS)));
							//n_inner2.PutInner("DELTAQUANTITY", 0);
							n_inner2.PutInner("PRICE", temp_buf.Z().Cat(price_without_vat, MKSFMTD(0, 2, 0)));
							//n_inner2.PutInner("PRICEUSD", 0);
							//n_inner2.PutInner("PRICEWITHVATUSD", 0);
							n_inner2.PutInner("AMOUNT", temp_buf.Z().Cat(amount_without_vat, MKSFMTD(0, 2, 0)));
							n_inner2.PutInner("AMOUNTWITHVAT", temp_buf.Z().Cat(amount_with_vat, MKSFMTD(0, 2, 0)));
							n_inner2.PutInner("TAXRATE", temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 1, NMBF_NOTRAILZ)));
							//n_inner2.PutInner("CONDITIONSTATUS", 0);
							n_inner2.PutInner("DESCRIPTION", EncXmlText(goods_rec.Name));
							//n_inner2.PutInner("PACKAGEID", 0);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int EdiProviderImplementation_Exite::Write_OwnFormat_INVOIC(xmlTextWriter * pX, const S_GUID & rIdent, const PPBillPacket & rBp)
{
	int    ok = 1;
	SString temp_buf;
	SString bill_text;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName, bill_text);
	SXml::WDoc _doc(pX, cpUTF8);
	SXml::WNode n_docs(_doc, "INVOICE");
	{
		n_docs.PutInner("DOCUMENTNAME", 0);
		n_docs.PutInner("NUMBER", 0);
		n_docs.PutInner("DATE", 0);
		n_docs.PutInner("DELIVERYDATE", 0);
		n_docs.PutInner("DELIVERYTIME", 0);
		n_docs.PutInner("CURRENCY", 0);
		n_docs.PutInner("ORDERNUMBER", 0);
		n_docs.PutInner("ISPRORIGINALDOCNUMBER", 0);
		n_docs.PutInner("ISPRORIGINALDOCDATE", 0);
		n_docs.PutInner("ORDERDATE", 0);
		n_docs.PutInner("CORRNUMBER", 0);
		n_docs.PutInner("CORRDATE", 0);
		n_docs.PutInner("DELIVERYNOTENUMBER", 0);
		n_docs.PutInner("DELIVERYNOTEDATE", 0);
		n_docs.PutInner("RECADVNUMBER", 0);
		n_docs.PutInner("RECADVDATE", 0);
		n_docs.PutInner("REFERENCEINVOICENUMBER", 0);
		n_docs.PutInner("REFERENCEINVOICEDATE", 0);
		{
			SXml::WNode n_inner(_doc, "DOCUMENTBASE");
			{
				n_inner.PutInner("OPERATION", 0);
				n_inner.PutInner("BASENUMBER", 0);
				n_inner.PutInner("BASEDATE", 0);
				n_inner.PutInner("BASENAME", 0);
			}
		}
		n_docs.PutInner("GOODSTOTALAMOUNT", 0);
		n_docs.PutInner("POSITIONSAMOUNT", 0);
		n_docs.PutInner("VATSUM", 0);
		n_docs.PutInner("INVOICETOTALAMOUNT", 0);
		n_docs.PutInner("TAXABLEAMOUNT", 0);
		n_docs.PutInner("DISCOUNTAMOUNT", 0);
		n_docs.PutInner("PAYMENTORDERNUMBER", 0);
		n_docs.PutInner("FISCALNUMBER", 0);
		n_docs.PutInner("REGISTRATIONNUMBER", 0);
		n_docs.PutInner("VATNUMBER", 0);
		n_docs.PutInner("CAMPAIGNNUMBER", 0);
		n_docs.PutInner("SUPPLIERCODE", 0);
		n_docs.PutInner("MANAGER", 0);
		{
			SXml::WNode n_inner(_doc, "MANAGERDATA");
			{
				n_inner.PutInner("POSITION", 0);
				n_inner.PutInner("AUTHORIZATION", 0);
				n_inner.PutInner("STATUS", 0);
				n_inner.PutInner("CREDENTIALS", 0);
				n_inner.PutInner("INN", 0);
			}
		}
		n_docs.PutInner("ACCOUNTING", 0);
		{
			SXml::WNode n_inner(_doc, "ACCOUNTINGDATA");
			{
				n_inner.PutInner("POSITION", 0);
				n_inner.PutInner("AUTHORIZATION", 0);
				n_inner.PutInner("STATUS", 0);
				n_inner.PutInner("CREDENTIALS", 0);
			}
		}
		n_docs.PutInner("VAT", 0);
		{
			SXml::WNode n_inner(_doc, "HEAD");
			{
				n_inner.PutInner("SUPPLIER", 0);
				n_inner.PutInner("SUPPLIERNAME", 0);
				{
					SXml::WNode n_inner_ps(_doc, "PACKINGSEQUENCE");
					n_inner_ps.PutInner("HIERARCHICALID", 0);
					for(uint i = 0; i < rBp.GetTCount(); i++) {
						const PPTransferItem & r_ti = rBp.ConstTI(i);
						SXml::WNode n_inner2(_doc, "POSITION");
						{
							n_inner2.PutInner("POSITIONNUMBER", 0);
							n_inner2.PutInner("PRODUCT", 0);
							n_inner2.PutInner("PRODUCTIDSUPPLIER", 0);
							n_inner2.PutInner("PRODUCTIDBUYER", 0);
							n_inner2.PutInner("DELIVEREDQUANTITY", 0);
							n_inner2.PutInner("DELIVEREDUNIT", 0);
							n_inner2.PutInner("ORDEREDQUANTITY", 0);
							n_inner2.PutInner("ORDERUNIT", 0);
							n_inner2.PutInner("QUANTITYOFCUINTU", 0);
							n_inner2.PutInner("BOXESQUANTITY", 0);
							n_inner2.PutInner("PRODINN", 0);
							n_inner2.PutInner("PRODKPP", 0);
							n_inner2.PutInner("PARTYNAME", 0);
							n_inner2.PutInner("PRODUCTIONCODE", 0);
							n_inner2.PutInner("DISPLACEMENT", 0);
							n_inner2.PutInner("BOXWEIGHT", 0);
							n_inner2.PutInner("IMPORTPARTYNAME", 0);
							n_inner2.PutInner("LICENSE", 0);
							n_inner2.PutInner("LICGIVEN", 0);
							n_inner2.PutInner("LICSTART", 0);
							n_inner2.PutInner("LICEND", 0);
							n_inner2.PutInner("INVOICEDQUANTITY", 0);
							n_inner2.PutInner("PORTAL_CERT", 0);
							n_inner2.PutInner("MANUFACTUREDATE", 0);
							n_inner2.PutInner("CERTSTART", 0);
							n_inner2.PutInner("CERTEND", 0);
							n_inner2.PutInner("CERTS_URL", 0);
							{
								SXml::WNode n_egais(_doc, "EGAIS");
								{
									n_egais.PutInner("EGAISCODE", 0);
									n_egais.PutInner("EGAISQUANTITY", 0);
								}
							}
							n_inner2.PutInner("SHELFLIFEDATE", 0);
							n_inner2.PutInner("EXPIREDATE", 0);
							n_inner2.PutInner("INVOICEUNIT", 0);
							n_inner2.PutInner("AMOUNT", 0); // без ндс
							n_inner2.PutInner("COUNTRYORIGIN", 0);
							n_inner2.PutInner("CUSTOMSTARIFFNUMBER", 0); // гтд
							n_inner2.PutInner("PRICE", 0); // без ндс
							n_inner2.PutInner("PRICEWITHVAT", 0);
							n_inner2.PutInner("AMOUNTWITHVAT", 0);
							n_inner2.PutInner("DISCOUNT", 0);
							n_inner2.PutInner("TAXRATE", 0);
							n_inner2.PutInner("CONDITIONSTATUS", 0);
							n_inner2.PutInner("DESCRIPTION", 0);
							n_inner2.PutInner("PACKAGEID", 0);
							n_inner2.PutInner("PARTNUMBER", 0);
							n_inner2.PutInner("CERTIFICATE", 0);
							n_inner2.PutInner("MINIMUMORDERQUANTITY", 0);
							{
								SXml::WNode n_vetis(_doc, "VETDOCUMENT");
								{
									n_vetis.PutInner("BATCHID", 0);
									n_vetis.PutInner("UUID", 0);
									n_vetis.PutInner("VOLUME", 0);
									n_vetis.PutInner("DATEOFPRODUCTION", 0);
									n_vetis.PutInner("TIMEOFPRODUCTION", 0);
								}
							}
						}
					}
				}
			}
		}
	}
	//CATCHZOK
	return ok;
}
