// SUC.CPP
// Copyright (c) A.Sobolev 2017, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
//
//
//
SLAPI SUnicodeTable::Item32::Item32()
{
	THISZERO();
}

SLAPI SUnicodeTable::UPair::UPair(uint32 key, uint32 val) : K(key), V(val)
{
}

SLAPI SUnicodeTable::SUnicodeTable() : LastIdx(0)
{
}

int SLAPI SUnicodeTable::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 sver = 0;
    THROW(pSCtx->Serialize(dir, sver, rBuf));
    THROW(pSCtx->Serialize(dir, &List32, rBuf));
    THROW(pSCtx->Serialize(dir, &Ranges, rBuf));
    THROW(pSCtx->Serialize(dir, &SimpleToUpAndCap, rBuf));
    THROW(pSCtx->Serialize(dir, &SimpleToUp, rBuf));
    THROW(pSCtx->Serialize(dir, &SimpleToCap, rBuf));
    THROW(pSCtx->Serialize(dir, &SimpleToLo, rBuf));
    THROW(pSCtx->Serialize(dir, &MirrorPairs, rBuf));
	CATCHZOK
	return ok;
}

const SUnicodeTable::Item32 * FASTCALL SUnicodeTable::Get(uint32 u) const
{
	uint   pos = 0;
	SUnicodeTable::Item32 * p_result = List32.bsearch(&u, &pos, CMPF_LONG) ? &List32.at(pos) : 0;
	if(!p_result) {
        for(uint i = 0; i < Ranges.getCount(); i++) {
			const UPair & r_range = Ranges.at(i);
			if(u >= r_range.K && u <= r_range.V) {
                p_result = List32.bsearch(&r_range.K, &pos, CMPF_LONG) ? &List32.at(pos) : 0;
                break;
			}
        }
	}
	return p_result;
}

uint32 FASTCALL SUnicodeTable::ToLower(uint32 u) const
{
	uint   p = 0;
    return SimpleToLo.bsearch(&u, &p, CMPF_LONG) ? SimpleToLo.at(p).V : u;
}

uint32 FASTCALL SUnicodeTable::ToUpper(uint32 u) const
{
	uint   p = 0;
	if(SimpleToUpAndCap.bsearch(&u, &p, CMPF_LONG))
		return SimpleToUpAndCap.at(p).V;
	else {
		p = 0;
		return SimpleToUp.bsearch(&u, &p, CMPF_LONG) ? SimpleToLo.at(p).V : u;
	}
}

uint32 FASTCALL SUnicodeTable::ToCapital(uint32 u) const
{
	uint   p = 0;
	if(SimpleToUpAndCap.bsearch(&u, &p, CMPF_LONG))
		return SimpleToUpAndCap.at(p).V;
	else {
		p = 0;
		return SimpleToCap.bsearch(&u, &p, CMPF_LONG) ? SimpleToLo.at(p).V : u;
	}
}

int SLAPI SUnicodeTable::ParseDescription(SString & rDescr, SUnicodeTable::Item32 & rD)
{
    int    ok = 1; // 0 - error, 1 - normal, 2 - lower range bound, 3 - upper range bound
    if(rDescr.CmpNC("<control>") == 0) {
		rDescr.Z();
    }
    else {
		if(rDescr.C(0) == '<' && rDescr.Last() == '>') {
			const char * p_first_suffix = ", first>";
			const char * p_last_suffix = ", last>";
			if(rDescr.CmpSuffix(p_first_suffix, 1) == 0) {
				ok = 2;
				rDescr.ShiftLeft(1);
				rDescr.Trim(rDescr.Len()-sstrlen(p_first_suffix));
				rDescr.Strip();
			}
			else if(rDescr.CmpSuffix(p_last_suffix, 1) == 0) {
				ok = 3;
				rDescr.ShiftLeft(1);
				rDescr.Trim(rDescr.Len()-sstrlen(p_last_suffix));
				rDescr.Strip();
			}
		}
    	size_t start_p = 0;
    	size_t length = 0;
		int    nscript_id = RecognizeSNScriptSymb(rDescr+start_p, &length);
		if(nscript_id) {
            rD.SNScriptId = (uint16)nscript_id;
            rDescr.ShiftLeft(length).Strip();
		}
		if(ok == 2) {
            rD.Flags |= fRange;
		}
    }
    return ok;
}

int SLAPI SUnicodeTable::PreprocessLine(SString & rLine) const
{
	size_t comment_pos = 0;
	rLine.Chomp();
	if(rLine.SearchChar('#', &comment_pos))
		rLine.Trim(comment_pos);
	return rLine.NotEmptyS();
}

uint32 FASTCALL SUnicodeTable::ParseUnicode(SString & rBuf)
{
	uint32 u = 0;
	rBuf.Strip();
	if(rBuf.Len() == 4) {
		if(ishex(rBuf[0]) && ishex(rBuf[1]) && ishex(rBuf[2]) && ishex(rBuf[3]))
			u = (hex(rBuf[0]) << 12) + (hex(rBuf[1]) << 8) + (hex(rBuf[2]) << 4) + hex(rBuf[3]);
	}
	else if(rBuf.Len() == 5) {
		if(ishex(rBuf[0]) && ishex(rBuf[1]) && ishex(rBuf[2]) && ishex(rBuf[3]) && ishex(rBuf[4]))
			u = (hex(rBuf[0]) << 16) + (hex(rBuf[1]) << 12) + (hex(rBuf[2]) << 8) + (hex(rBuf[3]) << 4) + hex(rBuf[4]);
	}
	else if(rBuf.Len() == 6) {
		if(ishex(rBuf[0]) && ishex(rBuf[1]) && ishex(rBuf[2]) && ishex(rBuf[3]) && ishex(rBuf[4]) && ishex(rBuf[5]))
			u = (hex(rBuf[0]) << 20) + (hex(rBuf[1]) << 16) + (hex(rBuf[2]) << 12) + (hex(rBuf[3]) << 8) + (hex(rBuf[4]) << 4) + hex(rBuf[5]);
	}
	return u;
}

int SLAPI SUnicodeTable::ParseSource(const char * /*pFileName*/pPath)
{
    int   ok = 1;
    SString file_name;
    SString line_buf, fld_buf;
    StringSet ss(";");
    {
        (file_name = pPath).SetLastSlash().Cat("UnicodeData.txt");
		SFile f_in(file_name, SFile::mRead);
		THROW(f_in.IsValid());
		while(f_in.ReadLine(line_buf)) {
			if(PreprocessLine(line_buf)) {
				ss.setBuf(line_buf.Strip());
				const uint fld_count = ss.getCount();
				int   description_result = 0;
				if(oneof2(fld_count, 14, 15)) {
					uint   u_size = 0;
					uint   descr_p = 0;
					Item32 i32;
					uint32 to_lo = 0;
					uint32 to_up = 0;
					uint32 to_cap = 0;
					for(uint fld_p = 0, fld_no = 0; ss.get(&fld_p, fld_buf); fld_no++) {
						switch(fld_no) {
							case 0: // unicode symbol
								i32.U4 = ParseUnicode(fld_buf);
								break;
							case 1: // description
								{
									fld_buf.Strip();
									description_result = ParseDescription(fld_buf, i32);
									if(!oneof2(description_result, 0, 3)) // Конец диапазона в список не вставляем
										AddS(fld_buf, &descr_p);
								}
								break;
							case 2: // general category
								fld_buf.Strip().ToLower();
								{
									if(fld_buf == "lu") i32.Gc = gcLu;
									else if(fld_buf == "ll") i32.Gc = gcLl;
									else if(fld_buf == "lt") i32.Gc = gcLt;
									else if(fld_buf == "lc") i32.Gc = gcLC;
									else if(fld_buf == "lm") i32.Gc = gcLm;
									else if(fld_buf == "lo") i32.Gc = gcLo;
									else if(fld_buf == "l") i32.Gc = gcL;
									else if(fld_buf == "mn") i32.Gc = gcMn;
									else if(fld_buf == "mc") i32.Gc = gcMc;
									else if(fld_buf == "me") i32.Gc = gcMe;
									else if(fld_buf == "m") i32.Gc = gcM;
									else if(fld_buf == "nd") i32.Gc = gcNd;
									else if(fld_buf == "nl") i32.Gc = gcNl;
									else if(fld_buf == "no") i32.Gc = gcNo;
									else if(fld_buf == "n") i32.Gc = gcN;
									else if(fld_buf == "pc") i32.Gc = gcPc;
									else if(fld_buf == "pd") i32.Gc = gcPd;
									else if(fld_buf == "ps") i32.Gc = gcPs;
									else if(fld_buf == "pe") i32.Gc = gcPe;
									else if(fld_buf == "pi") i32.Gc = gcPi;
									else if(fld_buf == "pf") i32.Gc = gcPf;
									else if(fld_buf == "po") i32.Gc = gcPo;
									else if(fld_buf == "p") i32.Gc = gcP;
									else if(fld_buf == "sm") i32.Gc = gcSm;
									else if(fld_buf == "sc") i32.Gc = gcSc;
									else if(fld_buf == "sk") i32.Gc = gcSk;
									else if(fld_buf == "so") i32.Gc = gcSo;
									else if(fld_buf == "s") i32.Gc = gcS;
									else if(fld_buf == "zs") i32.Gc = gcZs;
									else if(fld_buf == "zl") i32.Gc = gcZl;
									else if(fld_buf == "zp") i32.Gc = gcZp;
									else if(fld_buf == "z") i32.Gc = gcZ;
									else if(fld_buf == "cc") i32.Gc = gcCc;
									else if(fld_buf == "cf") i32.Gc = gcCf;
									else if(fld_buf == "cs") i32.Gc = gcCs;
									else if(fld_buf == "co") i32.Gc = gcCo;
									else if(fld_buf == "cn") i32.Gc = gcCn;
									else if(fld_buf == "c") i32.Gc = gcC;
								}
								break;
							case 3: // Canonical_Combining_Class
								{
									long   temp_val = fld_buf.ToLong();
									if(temp_val >= 0 && temp_val < 256)
										i32.Ccc = (uint8)temp_val;
								}
								break;
							case 4: // Bidi_Class
								fld_buf.Strip().ToLower();
								{
									if(fld_buf ==      "l")   i32.Bidi = bidiL;
									else if(fld_buf == "r")   i32.Bidi = bidiR;
									else if(fld_buf == "al")  i32.Bidi = bidiAL;
									else if(fld_buf == "en")  i32.Bidi = bidiEN;
									else if(fld_buf == "es")  i32.Bidi = bidiES;
									else if(fld_buf == "et")  i32.Bidi = bidiET;
									else if(fld_buf == "an")  i32.Bidi = bidiAN;
									else if(fld_buf == "cs")  i32.Bidi = bidiCS;
									else if(fld_buf == "nsm") i32.Bidi = bidiNSM;
									else if(fld_buf == "bn")  i32.Bidi = bidiBN;
									else if(fld_buf == "b")   i32.Bidi = bidiB;
									else if(fld_buf == "s")   i32.Bidi = bidiS;
									else if(fld_buf == "ws")  i32.Bidi = bidiWS;
									else if(fld_buf == "on")  i32.Bidi = bidiON;
									else if(fld_buf == "lre") i32.Bidi = bidiLRE;
									else if(fld_buf == "lro") i32.Bidi = bidiLRO;
									else if(fld_buf == "rle") i32.Bidi = bidiRLE;
									else if(fld_buf == "rlo") i32.Bidi = bidiRLO;
									else if(fld_buf == "pdf") i32.Bidi = bidiPDF;
									else if(fld_buf == "lri") i32.Bidi = bidiLRI;
									else if(fld_buf == "rli") i32.Bidi = bidiRLI;
									else if(fld_buf == "fsi") i32.Bidi = bidiFSI;
									else if(fld_buf == "pdi") i32.Bidi = bidiPDI;
								}
								break;
							case 5: // Decomposition_Type / Decomposition_Mapping
								break;
							case 6: // Numeric_Type / Numeric_Value
								// If the character has the property value Numeric_Type=Decimal, then the Numeric_Value of that digit
								// is represented with an integer value (limited to the range 0..9) in fields 6, 7, and 8.
								// Characters with the property value Numeric_Type=Decimal are restricted to digits which can be used
								// in a decimal radix positional numeral system and which are encoded in the standard in a contiguous ascending range 0..9.
								break;
							case 7: // Numeric_Type / Numeric_Value
								// If the character has the property value Numeric_Type=Digit, then the Numeric_Value of that digit is
								// represented with an integer value (limited to the range 0..9) in fields 7 and 8, and field 6 is null.
								// This covers digits that need special handling, such as the compatibility superscript digits.
								// Starting with Unicode 6.3.0, no newly encoded numeric characters will be given Numeric_Type=Digit,
								// nor will existing characters with Numeric_Type=Numeric be changed to Numeric_Type=Digit.
								// The distinction between those two types is not considered useful.
								break;
							case 8: // Numeric_Type / Numeric_Value
								// If the character has the property value Numeric_Type=Numeric, then the Numeric_Value of that
								// character is represented with a positive or negative integer or rational number in this field,
								// and fields 6 and 7 are null. This includes fractions such as, for example, "1/5" for U+2155
								// VULGAR FRACTION ONE FIFTH. Some characters have these properties based on values from the Unihan data files.
								break;
							case 9: // Bidi_Mirrored
								fld_buf.Strip().ToLower();
								if(fld_buf == "y") {
									i32.Flags |= fBidiMirrored;
								}
								break;
							case 10: // obsolete
								break;
							case 11: // obsolete
								break;
							case 12: // Simple_Uppercase_Mapping
								to_up = ParseUnicode(fld_buf);
								break;
							case 13: // Simple_Lowercase_Mapping
								to_lo = ParseUnicode(fld_buf);
								break;
							case 14: // Simple_Titlecase_Mapping
								to_cap = ParseUnicode(fld_buf);
								break;
							default:
								break;
						}
					}
					if(description_result == 3) { // Конец диапазона
						assert(List32.getCount());
						const Item32 & r_last = List32.at(List32.getCount()-1);
						assert(r_last.Flags & fRange);
						assert(r_last.U4 < i32.U4);
						UPair pair(r_last.U4, i32.U4);
						THROW(Ranges.insert(&pair));
					}
					else {
						if(to_lo) {
							UPair pair(i32.U4, to_lo);
							THROW(SimpleToLo.insert(&pair));
						}
						if(to_cap == to_up) {
							if(to_up) {
								UPair pair(i32.U4, to_up);
								THROW(SimpleToUpAndCap.insert(&pair));
							}
						}
						else {
							if(to_up) {
								UPair pair(i32.U4, to_up);
								THROW(SimpleToUp.insert(&pair));
							}
							if(to_cap) {
								UPair pair(i32.U4, to_cap);
								THROW(SimpleToCap.insert(&pair));
							}
						}
						i32.DescrP = descr_p;
						List32.insert(&i32);
					}
				}
			}
		}
    }
    SimpleToLo.sort(CMPF_LONG);
    SimpleToUp.sort(CMPF_LONG);
    SimpleToCap.sort(CMPF_LONG);
    SimpleToUpAndCap.sort(CMPF_LONG);
	List32.sort(CMPF_LONG);
    {
    	SString left, right;
        (file_name = pPath).SetLastSlash().Cat("BidiMirroring.txt");
		SFile f_in(file_name, SFile::mRead);
		THROW(f_in.IsValid());
		while(f_in.ReadLine(line_buf)) {
			if(PreprocessLine(line_buf) && line_buf.Divide(';', left, right) > 0) {
				uint32 u1 = ParseUnicode(left);
				uint32 u2 = ParseUnicode(right);
				assert(u1 && u2);
				if(u1 && u2) {
					UPair pair(u1, u2);
					const SUnicodeTable::Item32 * p_item1 = Get(u1);
					const SUnicodeTable::Item32 * p_item2 = Get(u2);
					assert(p_item1 && p_item2);
					assert(p_item1->Flags & fBidiMirrored);
					assert(p_item2->Flags & fBidiMirrored);
					THROW(MirrorPairs.insert(&pair));
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
SLAPI SCodepageMapPool::CpMap::CpUToBHash::CpUToBHash(const SCodepageMapPool::CpMap & rMap) : P_Buf(0), Count(0), EntrySize(rMap.MbMl)
{
	int    ok = -1;
	const uint mc = rMap.MapCount;
	const uint es = rMap.MbMl;
	void * p_temp_hash = 0;
	SCodepageMapPool::MapEntry * p_fallback = 0;
	uint  real_fbc = 0;
	int   is_there_fallback_dup = 0;
	if(rMap.FallbackCount) {
		//
		// Во избежании риска зациклить идентификацию размера хэша, мы должны
		// убедиться, что fallback не содержит элементов, которые уже есть в rMap.P_Map
		// Для этого создадим временный fallback без таких дубликатов
		//
		THROW(p_fallback = new SCodepageMapPool::MapEntry[rMap.FallbackCount]);
		for(uint i = 0; i < rMap.FallbackCount; i++) {
			if(!rMap.SearchU(rMap.P_Fallback[i].U2)) {
				p_fallback[real_fbc++] = rMap.P_Fallback[i];
			}
			else {
				is_there_fallback_dup = 1;
			}
		}
	}
	const uint fbc = real_fbc;
	uint hs = mc+fbc+1;
	do {
		while(!IsPrime(hs))
			hs++;
		p_temp_hash = SAlloc::R(p_temp_hash, hs * es);
		memzero(p_temp_hash, hs * es);
		int    is_dup = 0; // Если !0, то в хэше есть дубликат - надо увеличивать размер
		int    fuhr = FillUpHash(mc, rMap.P_Map, es, hs, p_temp_hash);
		if(fuhr > 0) {
			fuhr = FillUpHash(fbc, p_fallback, es, hs, p_temp_hash);
		}
		if(fuhr < 0) {
			is_dup = 1;
			hs += (mc+fbc); // Увеличиваем приблизительный размер хэша на один шаг, равный сумме элементов основной таблицы и fallback'а
		}
		else if(fuhr > 0)
			ok = 1;
		else  // fuhr == 0
			ok = 0;
	} while(ok < 0);
	if(ok > 0) {
		Count = hs;
		P_Buf = p_temp_hash;
	}
	CATCHZOK
	delete [] p_fallback;
}

int SLAPI SCodepageMapPool::CpMap::CpUToBHash::Translate(const wchar_t * pSrc, size_t srcLen, SString & rResult) const
{
	int    ok = 1;
	const uint hs = Count;
	const uint es = EntrySize;
	size_t p = 0;
	switch(es) {
		case 1:
			{
				uint8 result_buf[16];
				while((srcLen-p) >= 16) {
					const wchar_t * p_s = pSrc+p;
					result_buf[0]  = PTR8(P_Buf)[(p_s[0] % hs)];
					result_buf[1]  = PTR8(P_Buf)[(p_s[1] % hs)];
					result_buf[2]  = PTR8(P_Buf)[(p_s[2] % hs)];
					result_buf[3]  = PTR8(P_Buf)[(p_s[3] % hs)];
					result_buf[4]  = PTR8(P_Buf)[(p_s[4] % hs)];
					result_buf[5]  = PTR8(P_Buf)[(p_s[5] % hs)];
					result_buf[6]  = PTR8(P_Buf)[(p_s[6] % hs)];
					result_buf[7]  = PTR8(P_Buf)[(p_s[7] % hs)];
					result_buf[8]  = PTR8(P_Buf)[(p_s[8] % hs)];
					result_buf[9]  = PTR8(P_Buf)[(p_s[9] % hs)];
					result_buf[10] = PTR8(P_Buf)[(p_s[10] % hs)];
					result_buf[11] = PTR8(P_Buf)[(p_s[11] % hs)];
					result_buf[12] = PTR8(P_Buf)[(p_s[12] % hs)];
					result_buf[13] = PTR8(P_Buf)[(p_s[13] % hs)];
					result_buf[14] = PTR8(P_Buf)[(p_s[14] % hs)];
					result_buf[15] = PTR8(P_Buf)[(p_s[15] % hs)];
					p += 16;
					Helper_TranslateResultToBuf(result_buf, 16, rResult);
				}
				uint i = 0;
				while(p < srcLen) {
					result_buf[i++] = PTR8(P_Buf)[(pSrc[p++] % hs)];
				}
				Helper_TranslateResultToBuf(result_buf, i, rResult);
			}
			break;
		case 2:
			{
				uint16 result_buf[16];
				while((srcLen-p) >= 16) {
					const wchar_t * p_s = pSrc+p;
					result_buf[0]  = PTR16(P_Buf)[(p_s[0] % hs)];
					result_buf[1]  = PTR16(P_Buf)[(p_s[1] % hs)];
					result_buf[2]  = PTR16(P_Buf)[(p_s[2] % hs)];
					result_buf[3]  = PTR16(P_Buf)[(p_s[3] % hs)];
					result_buf[4]  = PTR16(P_Buf)[(p_s[4] % hs)];
					result_buf[5]  = PTR16(P_Buf)[(p_s[5] % hs)];
					result_buf[6]  = PTR16(P_Buf)[(p_s[6] % hs)];
					result_buf[7]  = PTR16(P_Buf)[(p_s[7] % hs)];
					result_buf[8]  = PTR16(P_Buf)[(p_s[8] % hs)];
					result_buf[9]  = PTR16(P_Buf)[(p_s[9] % hs)];
					result_buf[10] = PTR16(P_Buf)[(p_s[10] % hs)];
					result_buf[11] = PTR16(P_Buf)[(p_s[11] % hs)];
					result_buf[12] = PTR16(P_Buf)[(p_s[12] % hs)];
					result_buf[13] = PTR16(P_Buf)[(p_s[13] % hs)];
					result_buf[14] = PTR16(P_Buf)[(p_s[14] % hs)];
					result_buf[15] = PTR16(P_Buf)[(p_s[15] % hs)];
					p += 16;
					Helper_TranslateResultToBuf(result_buf, 16, rResult);
				}
				uint i = 0;
				while(p < srcLen) {
					result_buf[i++] = PTR16(P_Buf)[(pSrc[p++] % hs)];
				}
				Helper_TranslateResultToBuf(result_buf, i, rResult);
			}
			break;
		case 3:
			{
				uint8 result_buf[16*3];
				while((srcLen-p) >= 16) {
					const wchar_t * p_s = pSrc+p;
					uint h;
					h = (p_s[0] % hs) * 3;
					result_buf[0]  = PTR8(P_Buf)[h];
					result_buf[1]  = PTR8(P_Buf)[h+1];
					result_buf[2]  = PTR8(P_Buf)[h+2];
					h = (p_s[1] % hs) * 3;
					result_buf[3]  = PTR8(P_Buf)[h];
					result_buf[4]  = PTR8(P_Buf)[h+1];
					result_buf[5]  = PTR8(P_Buf)[h+2];
					h = (p_s[2] % hs) * 3;
					result_buf[6]  = PTR8(P_Buf)[h];
					result_buf[7]  = PTR8(P_Buf)[h+1];
					result_buf[8]  = PTR8(P_Buf)[h+2];
					h = (p_s[3] % hs) * 3;
					result_buf[9]  = PTR8(P_Buf)[h];
					result_buf[10]  = PTR8(P_Buf)[h+1];
					result_buf[11]  = PTR8(P_Buf)[h+2];
					h = (p_s[4] % hs) * 3;
					result_buf[12]  = PTR8(P_Buf)[h];
					result_buf[13]  = PTR8(P_Buf)[h+1];
					result_buf[14]  = PTR8(P_Buf)[h+2];
					h = (p_s[5] % hs) * 3;
					result_buf[15]  = PTR8(P_Buf)[h];
					result_buf[16]  = PTR8(P_Buf)[h+1];
					result_buf[17]  = PTR8(P_Buf)[h+2];
					h = (p_s[6] % hs) * 3;
					result_buf[18]  = PTR8(P_Buf)[h];
					result_buf[19]  = PTR8(P_Buf)[h+1];
					result_buf[20]  = PTR8(P_Buf)[h+2];
					h = (p_s[7] % hs) * 3;
					result_buf[21]  = PTR8(P_Buf)[h];
					result_buf[22]  = PTR8(P_Buf)[h+1];
					result_buf[23]  = PTR8(P_Buf)[h+2];
					h = (p_s[8] % hs) * 3;
					result_buf[24]  = PTR8(P_Buf)[h];
					result_buf[25]  = PTR8(P_Buf)[h+1];
					result_buf[26]  = PTR8(P_Buf)[h+2];
					h = (p_s[9] % hs) * 3;
					result_buf[27]  = PTR8(P_Buf)[h];
					result_buf[28]  = PTR8(P_Buf)[h+1];
					result_buf[29]  = PTR8(P_Buf)[h+2];
					h = (p_s[10] % hs) * 3;
					result_buf[30]  = PTR8(P_Buf)[h];
					result_buf[31]  = PTR8(P_Buf)[h+1];
					result_buf[32]  = PTR8(P_Buf)[h+2];
					h = (p_s[11] % hs) * 3;
					result_buf[33]  = PTR8(P_Buf)[h];
					result_buf[34]  = PTR8(P_Buf)[h+1];
					result_buf[35]  = PTR8(P_Buf)[h+2];
					h = (p_s[12] % hs) * 3;
					result_buf[36]  = PTR8(P_Buf)[h];
					result_buf[37]  = PTR8(P_Buf)[h+1];
					result_buf[38]  = PTR8(P_Buf)[h+2];
					h = (p_s[13] % hs) * 3;
					result_buf[39]  = PTR8(P_Buf)[h];
					result_buf[40]  = PTR8(P_Buf)[h+1];
					result_buf[41]  = PTR8(P_Buf)[h+2];
					h = (p_s[14] % hs) * 3;
					result_buf[42]  = PTR8(P_Buf)[h];
					result_buf[43]  = PTR8(P_Buf)[h+1];
					result_buf[44]  = PTR8(P_Buf)[h+2];
					h = (p_s[15] % hs) * 3;
					result_buf[45]  = PTR8(P_Buf)[h];
					result_buf[46]  = PTR8(P_Buf)[h+1];
					result_buf[47]  = PTR8(P_Buf)[h+2];
					p += 16;
					Helper_TranslateResultToBuf(result_buf, 16, rResult);
				}
				uint i = 0;
				while(p < srcLen) {
					const uint h = (pSrc[p++] % hs) * 3;
					result_buf[i]   = PTR8(P_Buf)[h];
					result_buf[i+1] = PTR8(P_Buf)[h+1];
					result_buf[i+2] = PTR8(P_Buf)[h+2];
					i += 3;
				}
				Helper_TranslateResultToBuf(result_buf, i / 3, rResult);
			}
			break;
		case 4:
			{
				uint32 result_buf[16];
				while((srcLen-p) >= 16) {
					const wchar_t * p_s = pSrc+p;
					result_buf[0]  = PTR32(P_Buf)[(p_s[0] % hs)];
					result_buf[1]  = PTR32(P_Buf)[(p_s[1] % hs)];
					result_buf[2]  = PTR32(P_Buf)[(p_s[2] % hs)];
					result_buf[3]  = PTR32(P_Buf)[(p_s[3] % hs)];
					result_buf[4]  = PTR32(P_Buf)[(p_s[4] % hs)];
					result_buf[5]  = PTR32(P_Buf)[(p_s[5] % hs)];
					result_buf[6]  = PTR32(P_Buf)[(p_s[6] % hs)];
					result_buf[7]  = PTR32(P_Buf)[(p_s[7] % hs)];
					result_buf[8]  = PTR32(P_Buf)[(p_s[8] % hs)];
					result_buf[9]  = PTR32(P_Buf)[(p_s[9] % hs)];
					result_buf[10] = PTR32(P_Buf)[(p_s[10] % hs)];
					result_buf[11] = PTR32(P_Buf)[(p_s[11] % hs)];
					result_buf[12] = PTR32(P_Buf)[(p_s[12] % hs)];
					result_buf[13] = PTR32(P_Buf)[(p_s[13] % hs)];
					result_buf[14] = PTR32(P_Buf)[(p_s[14] % hs)];
					result_buf[15] = PTR32(P_Buf)[(p_s[15] % hs)];
					p += 16;
					Helper_TranslateResultToBuf(result_buf, 16, rResult);
				}
				uint i = 0;
				while(p < srcLen) {
					result_buf[i++] = PTR32(P_Buf)[(pSrc[p++] % hs)];
				}
				Helper_TranslateResultToBuf(result_buf, i, rResult);
			}
			break;
		default: assert(0);
	}
	return ok;
}

int SLAPI SCodepageMapPool::CpMap::CpUToBHash::Helper_TranslateResultToBuf(const void * pResultBuf, uint resultBufSize, SString & rBuf) const
{
	int    ok = 1;
	switch(EntrySize) {
		case 1:
			{
				for(uint i = 0; i < resultBufSize; i++) {
					uchar c = PTR8C(pResultBuf)[i];
					if(c)
						rBuf.CatChar(c);
					else
						rBuf.CatChar('?');
				}
			}
			break;
		case 2:
			{
				for(uint i = 0; i < resultBufSize; i++) {
					uint8 c0 = PTR8C(pResultBuf)[i*2];
					if(c0) {
						rBuf.CatChar(c0);
						uint8 c1 = PTR8C(pResultBuf)[i*2+1];
						if(c1)
							rBuf.CatChar(c1);
					}
					else
						rBuf.CatChar('?');
				}
			}
			break;
		case 3:
			{
				for(uint i = 0; i < resultBufSize; i++) {
					uint8 c0 = PTR8C(pResultBuf)[i*3];
					if(c0) {
						rBuf.CatChar(c0);
						uint8 c1 = PTR8C(pResultBuf)[i*3+1];
						if(c1) {
							rBuf.CatChar(c1);
							uint8 c2 = PTR8C(pResultBuf)[i*3+2];
							if(c2)
								rBuf.CatChar(c2);
						}
					}
					else
						rBuf.CatChar('?');
				}
			}
			break;
		case 4:
			{
				for(uint i = 0; i < resultBufSize; i++) {
					uint8 c0 = PTR8C(pResultBuf)[i*4];
					if(c0) {
						rBuf.CatChar(c0);
						uint8 c1 = PTR8C(pResultBuf)[i*4+1];
						if(c1) {
							rBuf.CatChar(c1);
							uint8 c2 = PTR8C(pResultBuf)[i*4+2];
							if(c2) {
								rBuf.CatChar(c2);
								uint8 c3 = PTR8C(pResultBuf)[i*4+3];
								if(c3)
									rBuf.CatChar(c3);
							}
						}
					}
					else
						rBuf.CatChar('?');
				}
			}
			break;
		default: assert(0);
	}
	return ok;
}

int SLAPI SCodepageMapPool::CpMap::CpUToBHash::FillUpHash(const uint mapCount, const SCodepageMapPool::MapEntry * pMap, const uint entrySize, const uint hashSize, void * pHash)
{
	int    is_dup = 0;
	for(uint i = 0; !is_dup && i < mapCount; i++) {
		uint16 u2 = pMap[i].U2;
		const uint h = u2 % hashSize;
		const uint8 * p_b = pMap[i].B;
		switch(entrySize) {
			case 1:
				if(PTR8(pHash)[h] == 0)
					PTR8(pHash)[h] = p_b[0];
				else
					is_dup = 1;
				break;
			case 2:
				if(PTR16(pHash)[h] == 0)
					PTR16(pHash)[h] = PTR16C(p_b)[0];
				else
					is_dup = 1;
				break;
			case 3:
				{
					uint8 * p_entry = PTR8(pHash) + h * 3;
					if(!p_entry[0] && !p_entry[1] && !p_entry[2]) {
						p_entry[0] = p_b[0];
						p_entry[1] = p_b[1];
						p_entry[2] = p_b[2];
					}
					else
						is_dup = 1;
				}
				break;
			case 4:
				if(PTR32(pHash)[h] == 0)
					PTR32(pHash)[h] = PTR32C(p_b)[0];
				else
					is_dup = 1;
				break;
			default:
				assert(0);
		}
	}
	return is_dup ? -1 : 1;
}

SLAPI SCodepageMapPool::CpMap::CpMap() : P_U2B_Hash(0)
{
	Clear();
}

SLAPI SCodepageMapPool::CpMap::~CpMap()
{
	delete P_U2B_Hash;
}

void SLAPI SCodepageMapPool::CpMap::Clear()
{
	Id = cpUndef;
	MbMl = 0;
    Solid256Offs = 0;
    StraightCount = 0;
	CpSis = 0;
	Flags = 0;
	MapCount = 0;
	FallbackCount = 0;
	NScriptCount = 0;
	P_Map = 0;
	P_Fallback = 0;
	P_NScript = 0;
	Name.Z();
	Code.Z();
	Version = 0;
	ZDELETE(P_U2B_Hash);
}

const SCodepageMapPool::MapEntry * FASTCALL SCodepageMapPool::CpMap::SearchC(const uint8 b[]) const
{
	uint8 local_b[4];
    PTR32(local_b)[0] = 0;
	uint   bw = 0;
    if(b[0]) {
		local_b[0] = b[0];
		bw++;
		if(b[1]) {
			local_b[1] = b[1];
			bw++;
			if(b[2]) {
				bw++;
				local_b[2] = b[2];
				if(b[3]) {
					bw++;
					local_b[3] = b[3];
				}
			}
		}
		else if(Flags & SCodepageMapPool::fSolid256) {
			return (P_Map + local_b[0] + Solid256Offs);
		}
    }
	if(bw <= MbMl) {
		const uint32 local_b32 = PTR32(local_b)[0];
		for(uint i = 0; i < MapCount; i++) {
			if(PTR32C(P_Map[i].B)[0] == local_b32)
				return (P_Map+i);
		}
	}
	return 0;
}

const SCodepageMapPool::MapEntry * FASTCALL SCodepageMapPool::CpMap::SearchU(uint32 u) const
{
	for(uint i = 0; i < MapCount; i++) {
		if(P_Map[i].U2 == static_cast<uint16>(u))
			return (P_Map+i);
	}
	return 0;
}

const SCodepageMapPool::MapEntry * FASTCALL SCodepageMapPool::CpMap::SearchFallback(uint32 u) const
{
	if(FallbackCount) {
		const MapEntry * p_org = P_Fallback;
		for(uint i = 0, lo = 0, up = FallbackCount-1; lo <= up;) {
			const MapEntry * p = p_org + (i = (lo + up) >> 1);
			const int cmp = CMPSIGN(p->U2, u);
			if(cmp < 0)
				lo = i + 1;
			else if(cmp) {
				if(i)
					up = i - 1;
				else
					return 0;
			}
			else
				return p;
		}
	}
	return 0;
}

int SLAPI SCodepageMapPool::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint32 sver = 0;
    THROW(pSCtx->Serialize(dir, sver, rBuf));
    THROW(SStrGroup::SerializeS(dir, rBuf, pSCtx));
    THROW(pSCtx->Serialize(dir, &CpL, rBuf));
    THROW(pSCtx->Serialize(dir, &MeL, rBuf));
    THROW(pSCtx->Serialize(dir, &FbL, rBuf));
	THROW(pSCtx->Serialize(dir, &NScrL, rBuf));
	CATCHZOK
	return ok;
}

void SLAPI SCodepageMapPool::Clear()
{
	ClearS();
	CpL.freeAll();
	MeL.freeAll();
	FbL.freeAll();
	NScrL.freeAll();
}

uint SLAPI SCodepageMapPool::GetCount() const
{
	return CpL.getCount();
}

void SLAPI SCodepageMapPool::TranslateEntry(const CpEntry & rSrc, CpMap & rDest) const
{
	rDest.Clear();
	rDest.Id = rSrc.Id;
	rDest.MbMl = rSrc.MbMl;
	rDest.Solid256Offs = rSrc.Solid256Offs;
	rDest.StraightCount = rSrc.StraightCount;
	rDest.CpSis = rSrc.CpSis;
	rDest.Flags = rSrc.Flags;
	rDest.MapCount = rSrc.MapCount;
	rDest.FallbackCount = rSrc.FallbackCount;
	rDest.NScriptCount = rSrc.NScriptCount;
	rDest.P_Map = &MeL.at(rSrc.MapStart);
	rDest.P_Fallback = rSrc.FallbackCount ? &FbL.at(rSrc.FallbackStart) : 0;
	rDest.P_NScript = rSrc.NScriptCount ? &NScrL.at(rSrc.NScriptStart) : 0;
	GetS(rSrc.NameP, rDest.Name);
	GetS(rSrc.CodeP, rDest.Code);
	GetS(rSrc.VersionP, rDest.Version);
}

int SLAPI SCodepageMapPool::GetByPos(uint pos, CpMap * pM) const
{
	int    ok = 0;
	if(pos < CpL.getCount()) {
		const CpEntry & r_entry = CpL.at(pos);
		if(pM)
			TranslateEntry(r_entry, *pM);
		ok = 1;
	}
	return ok;
}

int SLAPI SCodepageMapPool::Get(SCodepage cp, SCodepageMapPool::CpMap * pM) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < CpL.getCount(); i++) {
		const CpEntry & r_entry = CpL.at(i);
		if(r_entry.Id == cp) {
			if(pM)
				TranslateEntry(r_entry, *pM);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI SCodepageMapPool::GetByName(const char * pName, SCodepageMapPool::CpMap * pM) const
{
	int    ok = 0;
	SString temp_buf;
	for(uint i = 0; !ok && i < CpL.getCount(); i++) {
		const CpEntry & r_entry = CpL.at(i);
		GetS(r_entry.NameP, temp_buf);
		if(temp_buf.CmpNC(pName) == 0) {
			if(pM)
				TranslateEntry(r_entry, *pM);
			ok = 1;
		}
	}
	return ok;
}

IMPL_CMPCFUNC(CPMCPENTRY, p1, p2)
{
	const SCodepageMapPool::MapEntry * i1 = static_cast<const SCodepageMapPool::MapEntry *>(p1);
	const SCodepageMapPool::MapEntry * i2 = static_cast<const SCodepageMapPool::MapEntry *>(p2);
	return CMPSIGN(PTR32C(i1->B)[0], PTR32C(i2->B)[0]);
}

IMPL_CMPCFUNC(CPMCPENTRY_U, p1, p2)
{
	const SCodepageMapPool::MapEntry * i1 = static_cast<const SCodepageMapPool::MapEntry *>(p1);
	const SCodepageMapPool::MapEntry * i2 = static_cast<const SCodepageMapPool::MapEntry *>(p2);
	return CMPSIGN(i1->U2, i2->U2);
}

IMPL_CMPCFUNC(CPMCPENTRYFULLY, p1, p2)
{
	const SCodepageMapPool::MapEntry * i1 = static_cast<const SCodepageMapPool::MapEntry *>(p1);
	const SCodepageMapPool::MapEntry * i2 = static_cast<const SCodepageMapPool::MapEntry *>(p2);
	int   si = CMPSIGN(PTR32C(i1->B)[0], PTR32C(i2->B)[0]);
	if(si == 0)
		si = CMPSIGN(i1->U2, i2->U2);
	return si;
}

IMPL_CMPCFUNC(CPMCPENTRYUREF, p1, p2)
{
	const long r1 = *static_cast<const long *>(p1);
	const long r2 = *static_cast<const long *>(p2);
	const SCodepageMapPool::MapEntry * p_list = static_cast<const SCodepageMapPool::MapEntry *>(pExtraData);
	return CMPSIGN(p_list[r1].U2, p_list[r2].U2);
}

SLAPI SCodepageMapPool::MapEntry::MapEntry() : U2(0)
{
	PTR32(B)[0] = 0;
}

int FASTCALL SCodepageMapPool::MapEntry::operator == (const MapEntry & rS) const
{
	return BIN(Cmp(rS) == 0);
}

int FASTCALL SCodepageMapPool::MapEntry::operator != (const MapEntry & rS) const
{
	return BIN(Cmp(rS) != 0);
}

int FASTCALL SCodepageMapPool::MapEntry::Cmp(const MapEntry & rS) const
{
	int    si = CMPSIGN(*PTR32C(B), *PTR32C(rS.B));
	return si ? si : CMPSIGN(U2, rS.U2);
}

SLAPI SCodepageMapPool::SCodepageMapPool() : SStrGroup()
{
}

int SLAPI SCodepageMapPool::ParseCpName(const SString & rName, int * pSis, SString & rCode, SString & rVersion) const
{
	int    ret = 0;
	int    sis = 0;
    StringSet ss('-', rName);
    SString temp_buf;
    uint   ss_pos = 0;
    if(ss.get(&ss_pos, temp_buf)) {
		sis = RecognizeSisSymb(temp_buf);
        if(ss.get(&ss_pos, temp_buf)) {
			rCode = temp_buf;
			if(ss.get(&ss_pos, temp_buf)) {
				rVersion = temp_buf;
				ret = -1;
				if(sis == ssisWindows && rCode.ToLong())
					ret = rCode.ToLong();
			}
        }
    }
	ASSIGN_PTR(pSis, sis);
    return ret;
}

int SLAPI SCodepageMapPool::ParseSymbols(SString & rU, const SString & rMb, MapEntry & rEntry, uint8 * pMbMl) const
{
	int    ok = -1;
	uint8  mbml = DEREFPTRORZ(pMbMl);
	const size_t u_len = rU.Len();
	const size_t b_len = rMb.Len();
	assert(oneof4(b_len, 2, 5, 8, 11));
	if(oneof4(b_len, 2, 5, 8, 11)) {
		rEntry.B[0] = (hex(rMb[0]) << 4) + hex(rMb[1]);
		SETMAX(mbml, 1);
		if(b_len >= 5) {
			rEntry.B[1] = (hex(rMb[3]) << 4) + hex(rMb[4]);
			SETMAX(mbml, 2);
		}
		if(b_len >= 8) {
			rEntry.B[2] = (hex(rMb[6]) << 4) + hex(rMb[7]);
			SETMAX(mbml, 3);
		}
		if(b_len == 11) {
			rEntry.B[3] = (hex(rMb[9]) << 4) + hex(rMb[10]);
			SETMAX(mbml, 4);
		}
		ok = 1;
	}
	else {
		; // @todo
	}
	rEntry.U2 = static_cast<uint16>(SUnicodeTable::ParseUnicode(rU));
	ok = 1;
	ASSIGN_PTR(pMbMl, mbml);
	return ok;
}

int SLAPI SCodepageMapPool::SearchMapSeq(const TSVector <MapEntry> & rSeq, uint * pPos) const
{
    int    ok = 0;
    const  uint _c = rSeq.getCount();
	const  uint _mc = MeL.getCount();
    if(_c) {
		for(uint p = 0; !ok && MeL.lsearch(&rSeq.at(0), &p, PTR_CMPCFUNC(CPMCPENTRYFULLY));) {
			ok = 1;
			uint   i = 1;
			for(; ok && i < _c; i++) {
				const uint mp = p+i;
				if(mp >= _mc || rSeq.at(i) != MeL.at(mp))
					ok = 0;
			}
			if(ok) {
				ASSIGN_PTR(pPos, p);
			}
			else
				p += i;
		}
    }
    return ok;
}

int SLAPI SCodepageMapPool::ParseXmlSingle(void * pXmlContext, const char * pFileName, const SUnicodeTable * pUt)
{
    int    ok = 1;
    xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
    SString temp_buf;
    SString u_buf, b_buf;
    LongArray nscript_list;
	uint8  two_c_start[64];
	uint8  four_c_start[64];
	SString cp_name;
	int    has_u4 = 0;
	int    has_b3 = 0;
	uint8  mbml = 0;
	uint8  mbml_fallback = 0;
	TSVector <MapEntry> map_list; // @v9.8.4 TSArray-->TSVector
	TSVector <MapEntry> fallback_list; // @v9.8.4 TSArray-->TSVector
	MEMSZERO(two_c_start);
	MEMSZERO(four_c_start);
	THROW(fileExists(pFileName));
	if(pXmlContext)
		p_ctx = static_cast<xmlParserCtxt *>(pXmlContext);
	else {
		THROW(p_ctx = xmlNewParserCtxt());
	}
	THROW(p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT));
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "characterMapping")) {
		SXml::GetAttrib(p_root, "id", cp_name);
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "assignments")) {
				for(xmlNode * p_a = p_n->children; p_a; p_a = p_a->next) {
					if(SXml::IsName(p_a, "a")) {
						SXml::GetAttrib(p_a, "u", u_buf);
						SXml::GetAttrib(p_a, "b", b_buf);
						MapEntry map_entry;
						if(ParseSymbols(u_buf, b_buf, map_entry, &mbml) > 0) {
							/*
							if(map_entry.U > 0xffffU)
								has_u4 = 1;
							*/
							if(map_entry.B[0]) {
								if(map_entry.B[1]) {
									if(map_entry.B[2] == 0) {
										/*
										int    _2f = 0;
										int    _fz = -1;
                                        for(uint i = 0; !_2f && i < SIZEOFARRAY(two_c_start); i++) {
											if(two_c_start[i] == map_entry.B[0])
												_2f = 1;
											else if(two_c_start[i] == 0 && _fz < 0)
												_fz = static_cast<int>(i);
                                        }
										if(!_2f) {
											assert(_fz >= 0);
											two_c_start[_fz] = map_entry.B[0];
										}
										*/
									}
									else if(map_entry.B[3]) {
										/*
										int    _4f = 0;
										int    _fz = -1;
										for(uint i = 0; !_4f && i < SIZEOFARRAY(four_c_start); i++) {
											if(four_c_start[i] == map_entry.B[0])
												_4f = 1;
											else if(four_c_start[i] == 0 && _fz < 0)
												_fz = static_cast<int>(i);
										}
										if(!_4f) {
											assert(_fz >= 0);
											four_c_start[_fz] = map_entry.B[0];
										}
										*/
									}
									else {
										has_b3 = 1;
									}
								}
							}
							THROW(map_list.insert(&map_entry));
						}
						else {
							; // @todo
						}
					}
					else if(SXml::IsName(p_a, "fub")) {
						SXml::GetAttrib(p_a, "u", u_buf);
						SXml::GetAttrib(p_a, "b", b_buf);
						MapEntry map_entry;
						if(ParseSymbols(u_buf, b_buf, map_entry, &mbml_fallback) > 0) {
							/*
							if(map_entry.U > 0xffffU)
								has_u4 = 1;
							*/
							THROW(fallback_list.insert(&map_entry));
						}
						else {
							; // @todo
						}
					}
				}
			}
		}
	}
	{
		SCodepageMapPool::CpEntry entry;
		MEMSZERO(entry);
		int    cp_sis = 0;
		uint   spp = 0;
		SString cp_symb;
		SString cp_version;
		entry.Id = static_cast<SCodepage>(ParseCpName(cp_name, &cp_sis, cp_symb, cp_version));
		entry.MbMl = mbml;
		if(has_u4)
			entry.Flags |= fHas4U;
		if(has_b3)
			entry.Flags |= fHas3B;
		entry.CpSis = static_cast<uint8>(cp_sis);
		THROW(AddS(cp_name, &spp));
		entry.NameP = spp;
		THROW(AddS(cp_symb, &spp));
		entry.CodeP = spp;
		THROW(AddS(cp_version, &spp));
		entry.VersionP = spp;
		map_list.sort(PTR_CMPCFUNC(CPMCPENTRY));
		fallback_list.sort(PTR_CMPCFUNC(CPMCPENTRY_U));
		{
			entry.MapCount = map_list.getCount();
			if(entry.MapCount <= 256 && mbml == 1) {
				int    is_solid = 1;
				uint8  first = 0;
                for(uint i = 0; is_solid && i < map_list.getCount(); i++) {
                	const uint8 _c = map_list.at(i).B[0];
					if(i == 0)
						first = _c;
                    else if(_c != (map_list.at(i-1).B[0]+1))
						is_solid = 0;
                }
				if(is_solid) {
					entry.Flags |= fSolid256;
					entry.Solid256Offs = first;
				}
			}
			{
				const uint _ct = MIN(256, map_list.getCount());
				for(uint i = 0; i < _ct; i++) {
					const uint8 * p_b = map_list.at(i).B;
					if(p_b[1] == 0) {
						const uint8 _c = p_b[0];
						if(_c == static_cast<uint8>(i))
							entry.StraightCount++;
						else
							break;
					}
				}
			}
			assert(!(entry.Flags & fSolid256 && entry.Solid256Offs == 0) || entry.StraightCount == map_list.getCount());
			uint   sp = 0;
			if(SearchMapSeq(map_list, &sp)) {
                entry.MapStart = sp;
			}
			else {
				entry.MapStart = MeL.getCount();
				THROW(MeL.insertChunk(entry.MapCount, map_list.dataPtr()));
			}
		}
        entry.FallbackStart = FbL.getCount();
        entry.FallbackCount = fallback_list.getCount();
        THROW(FbL.insertChunk(entry.FallbackCount, fallback_list.dataPtr()));
        if(pUt) {
			for(uint i = 0; i < map_list.getCount(); i++) {
                const MapEntry & r_entry = map_list.at(i);
                const SUnicodeTable::Item32 * p_ui = pUt->Get(r_entry.U2);
                if(p_ui && p_ui->SNScriptId) {
					nscript_list.addUnique(static_cast<long>(p_ui->SNScriptId));
                }
			}
			nscript_list.sortAndUndup();
			entry.NScriptCount = nscript_list.getCount();
			if(entry.NScriptCount) {
				entry.NScriptStart = NScrL.getCount();
				for(uint i = 0; i < entry.NScriptCount; i++) {
					const uint16 s = static_cast<uint16>(nscript_list.at(i));
					THROW(NScrL.insert(&s));
				}
			}
        }
        THROW(CpL.insert(&entry));
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	if(!pXmlContext)
		xmlFreeParserCtxt(p_ctx);
    return ok;
}

const SCodepageMapPool::MapEntry * SCodepageMapPool::CpMap::Test_ToLower(const SCodepageMapPool::MapEntry * pSrc, const SUnicodeTable * pUt) const
{
	if(pUt && pSrc && pSrc->U2) {
		uint32 u = pUt->ToLower(pSrc->U2);
		if(u)
			return SearchU(u);
	}
	return 0;
}

const SCodepageMapPool::MapEntry * SCodepageMapPool::CpMap::Test_ToUpper(const SCodepageMapPool::MapEntry * pSrc, const SUnicodeTable * pUt) const
{
	if(pUt && pSrc && pSrc->U2) {
		uint32 u = pUt->ToUpper(pSrc->U2);
		if(u)
			return SearchU(u);
	}
	return 0;
}

const SCodepageMapPool::MapEntry * SCodepageMapPool::CpMap::Test_ToCapital(const SCodepageMapPool::MapEntry * pSrc, const SUnicodeTable * pUt) const
{
	if(pUt && pSrc && pSrc->U2) {
		uint32 u = pUt->ToCapital(pSrc->U2);
		if(u)
			return SearchU(u);
	}
	return 0;
}

int SLAPI SCodepageMapPool::CpMap::TranslateToB2(const wchar_t * pSrc, size_t srcLen, SString & rBuf)
{
	int    ok = 1;
	if(SETIFZ(P_U2B_Hash, new CpUToBHash(*this)))
		P_U2B_Hash->Translate(pSrc, srcLen, rBuf);
	else
		ok = 0;
	return ok;
}

int SLAPI SCodepageMapPool::CpMap::MakeUIndex(LongArray & rIdx) const
{
	int    ok = 1;
    rIdx.clear();
	for(uint i = 0; i < MapCount; i++) {
		THROW(rIdx.add(static_cast<long>(i)));
	}
	rIdx.SVector::sort(PTR_CMPCFUNC(CPMCPENTRYUREF), const_cast<MapEntry *>(P_Map)); // @badcast
	CATCHZOK
	return ok;
}

const SCodepageMapPool::MapEntry * FASTCALL SCodepageMapPool::CpMap::SearchU(wchar_t u, const LongArray * pIdx) const
{
	if(pIdx) {
		const  MapEntry * p_org = P_Map;
		for(uint i = 0, lo = 0, up = pIdx->getCount()-1; lo <= up;) {
			const uint _pos = static_cast<uint>(pIdx->get(i = (lo + up) >> 1));
			const int cmp = CMPSIGN(p_org[_pos].U2, u);
			if(cmp < 0)
				lo = i + 1;
			else if(cmp) {
				if(i)
					up = i - 1;
				else
					return 0;
			}
			else
				return (p_org + _pos);
		}
		return 0;
	}
	else
		return SearchU(u);
}

int FASTCALL SCodepageMapPool::CpMap::Helper_BSearchB(uint32 b4, wchar_t * pU) const
{
	const  MapEntry * p_org = P_Map;
	for(uint i = StraightCount, lo = StraightCount, up = MapCount-1; lo <= up;) {
		const MapEntry * p = p_org + (i = (lo + up) >> 1);
		const int cmp = CMPSIGN(PTR32C(p->B)[0], b4);
		if(cmp < 0)
			lo = i + 1;
		else if(cmp) {
			if(i)
				up = i - 1;
			else
				return 0;
		}
		else {
			*pU = p->U2;
			return 1;
		}
	}
	return 0;
}

int SLAPI SCodepageMapPool::CpMap::TranslateToU(const uint8 * pSrc, size_t srcSize, SStringU & rBuf) const
{
	const uint _count = MapCount;
	if(_count) {
		if(Flags & SCodepageMapPool::fSolid256) {
			assert(MbMl == 1);
			if(Solid256Offs == 0) {
				for(uint i = 0; i < srcSize; i++) {
					rBuf.CatChar(P_Map[pSrc[i]].U2);
				}
			}
			else {
				for(uint i = 0; i < srcSize; i++) {
					const uint8 c = pSrc[i];
					if(c < Solid256Offs)
						rBuf.CatChar(static_cast<wchar_t>(c));
					else
						rBuf.CatChar(P_Map[c+Solid256Offs].U2);
				}
			}
		}
		else {
			const  MapEntry * p_org = P_Map;
			if(MbMl == 1) {
				uint8 local_b[4];
				PTR32(local_b)[0] = 0;
				for(uint k = 0; k < srcSize; k++) {
					local_b[0] = pSrc[k];
					wchar_t u = 0;
					if(local_b[0] < StraightCount) {
						u = p_org[local_b[0]].U2;
					}
					else {
						Helper_BSearchB(*PTR32(local_b), &u);
					}
					rBuf.CatChar(u);
				}
			}
			else if(MbMl == 2) {
				uint8 local_b[4];
				uint8 local_b2[4];
				PTR32(local_b)[0] = 0;
				PTR32(local_b2)[0] = 0;
				for(uint k = 0; k < srcSize; k++) {
					const uint8 _cf = pSrc[k];
					wchar_t u = 0;
					if(_cf < StraightCount) {
						u = p_org[_cf].U2;
						assert(p_org[_cf].B[1] == 0);
					}
					else {
						int    _found = 0;
						if(k < (srcSize-1)) {
							local_b2[0] = _cf;
							local_b2[1] = pSrc[k+1];
							if(Helper_BSearchB(*PTR32(local_b2), &u)) {
								k++; // Использовали 2 байта входного потока
								_found = 1;
							}
						}
						if(!_found) {
							local_b[0] = _cf;
							if(Helper_BSearchB(*PTR32(local_b), &u)) {
								_found = 1;
							}
						}
					}
					rBuf.CatChar(u);
				}
			}
			else if(MbMl == 3) {
				uint8 local_b[4];
				uint8 local_b2[4];
				uint8 local_b3[4];
				PTR32(local_b)[0] = 0;
				PTR32(local_b2)[0] = 0;
				PTR32(local_b3)[0] = 0;
				for(uint k = 0; k < srcSize; k++) {
					const uint8 _cf = pSrc[k];
					wchar_t u = 0;
					if(_cf < StraightCount) {
						u = p_org[_cf].U2;
						assert(p_org[_cf].B[1] == 0);
					}
					else {
						int    _found = 0;
						if(k < (srcSize-2)) {
							local_b3[0] = _cf;
							local_b3[1] = pSrc[k+1];
							local_b3[2] = pSrc[k+2];
							if(Helper_BSearchB(*PTR32(local_b3), &u)) {
								k += 2; // Использовали 3 байта входного потока
								_found = 1;
							}
						}
						if(!_found) {
							if(k < (srcSize-1)) {
								local_b2[0] = _cf;
								local_b2[1] = pSrc[k+1];
								if(Helper_BSearchB(*PTR32(local_b2), &u)) {
									k++; // Использовали 2 байта входного потока
									_found = 1;
								}
							}
						}
						if(!_found) {
							local_b[0] = _cf;
							if(Helper_BSearchB(*PTR32(local_b), &u)) {
								_found = 1;
							}
						}
					}
					rBuf.CatChar(u);
				}
			}
			else if(MbMl == 4) {
				uint8 local_b[4];
				uint8 local_b2[4];
				uint8 local_b3[4];
				uint8 local_b4[4];
				PTR32(local_b)[0] = 0;
				PTR32(local_b2)[0] = 0;
				PTR32(local_b3)[0] = 0;
				PTR32(local_b4)[0] = 0;
				for(uint k = 0; k < srcSize; k++) {
					const uint8 _cf = pSrc[k];
					wchar_t u = 0;
					if(_cf < StraightCount) {
						u = p_org[_cf].U2;
						assert(p_org[_cf].B[1] == 0);
					}
					else {
						int    _found = 0;
						if(k < (srcSize-3)) {
							local_b4[0] = _cf;
							local_b4[1] = pSrc[k+1];
							local_b4[2] = pSrc[k+2];
							local_b4[3] = pSrc[k+3];
							if(Helper_BSearchB(*PTR32(local_b4), &u)) {
								k += 3; // Использовали 4 байта входного потока
								_found = 1;
							}
						}
						if(!_found) {
							if(k < (srcSize-2)) {
								local_b3[0] = _cf;
								local_b3[1] = pSrc[k+1];
								local_b3[2] = pSrc[k+2];
								if(Helper_BSearchB(*PTR32(local_b3), &u)) {
									k += 2; // Использовали 3 байта входного потока
									_found = 1;
								}
							}
						}
						if(!_found) {
							if(k < (srcSize-1)) {
								local_b2[0] = _cf;
								local_b2[1] = pSrc[k+1];
								if(Helper_BSearchB(*PTR32(local_b2), &u)) {
									k++; // Использовали 2 байта входного потока
									_found = 1;
								}
							}
						}
						if(!_found) {
							local_b[0] = _cf;
							if(Helper_BSearchB(*PTR32(local_b), &u)) {
								_found = 1;
							}
						}
					}
					rBuf.CatChar(u);
				}
			}
			else {
				assert(0);
			}
		}
	}
	return 0;
}

SCodepageMapPool::CMapTranslIndexTest::CMapTranslIndexTest() : TSVector <CMapTranslEntry>() // @v9.8.4 TSArray-->TSVector
{
	Reset();
}

void SCodepageMapPool::CMapTranslIndexTest::Reset()
{
	SVector::clear(); // @v9.8.4 SArray-->SVector
	MaxSLen = 0;
	MaxDLen = 0;
	IdenticalCount = 0;
	SuccCount = 0;
	FallbackCount = 0;
}

IMPL_CMPCFUNC(CMapTranslEntry, p1, p2) 
	{ RET_CMPCASCADE4(static_cast<const SCodepageMapPool::CMapTranslEntry *>(p1), static_cast<const SCodepageMapPool::CMapTranslEntry *>(p2), S[0], S[1], S[2], S[3]); }

SLAPI SCodepageMapPool::CMapTranslEntry::CMapTranslEntry()
{
	THISZERO();
}

void SCodepageMapPool::CMapTranslIndexTest::Sort()
{
	sort(PTR_CMPCFUNC(CMapTranslEntry));
}

SLAPI SCodepageMapPool::TranslIndex::TranslIndex() : P_Tab(0)
{
	Reset();
}

SLAPI SCodepageMapPool::TranslIndex::TranslIndex(const SCodepageMapPool::TranslIndex & rS)
{
	Copy(rS);
}

SLAPI SCodepageMapPool::TranslIndex::~TranslIndex()
{
	Reset();
}

int FASTCALL SCodepageMapPool::TranslIndex::Copy(const SCodepageMapPool::TranslIndex & rS)
{
	int    ok = 1;
	Reset();
	SL = rS.SL;
	DL = rS.DL;
	IdenticalCount = rS.IdenticalCount;
	Flags = rS.Flags;
	Count = rS.Count;
	const size_t entry_size = GetEntrySize();
	P_Tab = SAlloc::M(Count * entry_size);
	if(P_Tab) {
		memcpy(P_Tab, rS.P_Tab, Count * entry_size);
	}
	else {
		Reset();
		ok = 0;
	}
	return ok;
}

SCodepageMapPool::TranslIndex & FASTCALL SCodepageMapPool::TranslIndex::operator = (const SCodepageMapPool::TranslIndex & rS)
{
	Copy(rS);
	return *this;
}

void SLAPI SCodepageMapPool::TranslIndex::Reset()
{
	ZFREE(P_Tab);
	Count = 0;
	SL = 0;
	DL = 0;
	IdenticalCount = 0;
	Flags = 0;
}

int SLAPI SCodepageMapPool::TranslIndex::Setup(const SCodepageMapPool::CMapTranslIndexTest & rIdx)
{
    int    ok = 1;
	ZFREE(P_Tab);
	Count = 0;
	SL = rIdx.MaxSLen;
	DL = rIdx.MaxDLen;
	IdenticalCount = rIdx.IdenticalCount;
	const size_t entry_size = GetEntrySize();
	if(SL == 1) {
		const size_t maxdlen = DL;
		Count = 256 - IdenticalCount;
		assert(rIdx.getCount() <= 256);
		THROW(P_Tab = SAlloc::C(Count, entry_size));
		for(uint j = 0; j < rIdx.getCount(); j++) {
			const CMapTranslEntry & r_tcm_entry = rIdx.at(j);
			const uint _p = r_tcm_entry.S[0];
			assert(r_tcm_entry.S[1] == 0 && r_tcm_entry.S[2] == 0 && r_tcm_entry.S[3] == 0);
			assert(_p < 256);
			if(_p >= IdenticalCount) {
				const uint _p2 = (_p - IdenticalCount);
				assert(_p2 < 256);
				switch(maxdlen) {
					case 1:
						PTR8(P_Tab)[_p2] = r_tcm_entry.D[0];
						break;
					case 2:
						PTR8(P_Tab)[_p2*maxdlen]   = r_tcm_entry.D[0];
						PTR8(P_Tab)[_p2*maxdlen+1] = r_tcm_entry.D[1];
						break;
					case 3:
						PTR8(P_Tab)[_p2*maxdlen]   = r_tcm_entry.D[0];
						PTR8(P_Tab)[_p2*maxdlen+1] = r_tcm_entry.D[1];
						PTR8(P_Tab)[_p2*maxdlen+2] = r_tcm_entry.D[2];
						break;
					case 4:
						PTR32(P_Tab)[_p2] = PTR32C(r_tcm_entry.D)[0];
						break;
				}
			}
		}
	}
	else {
		Count = (rIdx.SuccCount + rIdx.FallbackCount)/* - IdenticalCount*/;
		size_t tab_ptr = 0;
		uint   running_count = 0;
		THROW(P_Tab = SAlloc::C(Count, entry_size));
		for(uint j = 0; j < rIdx.getCount(); j++) {
			const CMapTranslEntry & r_tcm_entry = rIdx.at(j);
			assert(!(r_tcm_entry.F & r_tcm_entry.fEqual) || (j == 0 || rIdx.at(j).F & CMapTranslEntry::fEqual));
			if(!(r_tcm_entry.F & (r_tcm_entry.fNone/*|r_tcm_entry.fEqual*/))) {
                uint8 * t = PTR8(P_Tab)+tab_ptr;
                switch(rIdx.MaxSLen) {
                    case 2:
                        *PTR16(t) = *PTR16C(r_tcm_entry.S);
                        break;
					case 3:
                        t[0] = r_tcm_entry.S[0];
                        t[1] = r_tcm_entry.S[1];
                        t[2] = r_tcm_entry.S[2];
                        break;
					case 4:
						*PTR32(t) = *PTR32C(r_tcm_entry.S);
                        break;
                }
                t += rIdx.MaxSLen;
                switch(rIdx.MaxDLen) {
					case 1:
						t[0] = r_tcm_entry.D[0];
						break;
					case 2:
						t[0] = r_tcm_entry.D[0];
						t[1] = r_tcm_entry.D[1];
						break;
					case 3:
						t[0] = r_tcm_entry.D[0];
						t[1] = r_tcm_entry.D[1];
						t[2] = r_tcm_entry.D[2];
						break;
					case 4:
						*PTR32(t) = *PTR32C(r_tcm_entry.D);
						break;
                }
                tab_ptr += entry_size;
				running_count++;
				assert(running_count <= Count);
			}
		}
	}
	CATCHZOK
	return ok;
}

size_t SLAPI SCodepageMapPool::TranslIndex::GetEntrySize() const
{
	assert(oneof4(SL, 1, 2, 3, 4));
	assert(oneof4(DL, 1, 2, 3, 4));
	if(SL == 1)
		return DL;
	else
		return (SL+DL);
}

/*
int FASTCALL SCodepageMapPool::CpMap::Helper_BSearchB(uint32 b4, wchar_t * pU) const
{
	const  MapEntry * p_org = P_Map;
	for(uint i = StraightCount, lo = StraightCount, up = MapCount-1; lo <= up;) {
		const MapEntry * p = p_org + (i = (lo + up) >> 1);
		const int cmp = CMPSIGN(PTR32(p->B)[0], b4);
		if(cmp < 0)
			lo = i + 1;
		else if(cmp) {
			if(i)
				up = i - 1;
			else
				return 0;
		}
		else {
			*pU = p->U2;
			return 1;
		}
	}
	return 0;
}
*/

static size_t FASTCALL _GetMbQuantLen(const void * pS)
{
	size_t src_len = 0;
	if(pS && PTR8C(pS)[0]) {
		src_len++;
		if(PTR8C(pS)[1]) {
			src_len++;
			if(PTR8C(pS)[2]) {
				src_len++;
				if(PTR8C(pS)[3])
					src_len++;
			}
		}
	}
	return src_len;
}

const uint8 * FASTCALL SCodepageMapPool::TranslIndex::Search(const uint8 * pSrc) const
{
	const uint8 * p_result = 0;
	if(Flags & fIdentical)
		p_result = pSrc;
	else if(!(Flags & fEmpty)) {
		const size_t src_len = _GetMbQuantLen(pSrc);
		if(src_len == 0)
			p_result = pSrc;
		else if(src_len <= SL) {
			if(SL == 1) {
				assert(Count == (256 - IdenticalCount));
				if(*pSrc < IdenticalCount)
					p_result = pSrc;
				else
					p_result = PTR8(P_Tab) + (*pSrc - IdenticalCount) * DL;
			}
			else {
				assert(SL > 1);
				const size_t cs = MIN(src_len, SL);
				const size_t entry_size = GetEntrySize();
				const uint8 * p_org = static_cast<const uint8 *>(P_Tab);
				if(0) {
					//
					// test
					//
					const uint8 * p_prev = 0;
					for(uint i = 0; i < Count; i++) {
						const uint8 * p = p_org + i * entry_size;
						if(p_prev) {
							int   cmp = 0;
							switch(SL) {
								case 1:
									cmp = CMPSIGN(p[0], p_prev[0]);
									break;
								case 2:
									CMPARRAY2(cmp, p, p_prev);
									break;
								case 3:
									CMPARRAY3(cmp, p, p_prev);
									break;
								case 4:
									CMPARRAY4(cmp, p, p_prev);
									break;
							}
							assert(cmp > 0);
						}
						p_prev = p;
					}
				}
				for(uint i = 0, lo = 0, up = Count-1; lo <= up;) {
					i = (lo + up) >> 1;
					const uint8 * p = p_org + i * entry_size;
					int   cmp = 0;
					switch(cs) {
						case 1:
							cmp = CMPSIGN(p[0], pSrc[0]);
							if(cmp == 0 && SL > src_len && p[1])
								cmp = +1;
							break;
						case 2:
							CMPARRAY2(cmp, p, pSrc);
							if(cmp == 0 && SL > src_len && p[2])
								cmp = +1;
							break;
						case 3:
							CMPARRAY3(cmp, p, pSrc);
							if(cmp == 0 && SL > src_len && p[3])
								cmp = +1;
							break;
						case 4:
							CMPARRAY4(cmp, p, pSrc);
							break;
					}
					if(cmp < 0)
						lo = i + 1;
					else if(cmp) {
						if(i)
							up = i - 1;
						else
							return 0;
					}
					else {
						p_result = (p + SL);
						return p_result;
					}
				}
			}
		}
	}
	return p_result;
}

int SLAPI SCodepageMapPool::Helper_MakeTranslIndex(const CpMap & rFrom, const CpMap & rTo, CMapTranslIndexTest & rIdx, TranslIndex & rFinalIdx)
{
	rIdx.Reset();
	rFinalIdx.Reset();

	int    ok = 1;
	LongArray to_idx;
	rTo.MakeUIndex(to_idx);
	for(uint i = 0; i < rFrom.MapCount; i++) {
		const wchar_t u = rFrom.P_Map[i].U2;
		const  MapEntry * p_to_entry = rTo.SearchU(u, &to_idx);
		CMapTranslEntry tcm_entry;
		*PTR32(tcm_entry.S) = *PTR32C(rFrom.P_Map[i].B);
		if(p_to_entry) {
			*PTR32(tcm_entry.D) = *PTR32C(p_to_entry->B);
			rIdx.SuccCount++;
		}
		else {
			const SCodepageMapPool::MapEntry * p_fb = rTo.SearchFallback(u);
			if(p_fb) {
				*PTR32(tcm_entry.D) = *PTR32C(p_fb->B);
				tcm_entry.F |= tcm_entry.fFallback;
				rIdx.FallbackCount++;
			}
			else
				tcm_entry.F |= tcm_entry.fNone;
		}
		if(!(tcm_entry.F & tcm_entry.fNone)) {
			const uint src_len  = _GetMbQuantLen(tcm_entry.S);
			const uint dest_len = _GetMbQuantLen(tcm_entry.D);
			SETMAX(rIdx.MaxSLen, src_len);
			SETMAX(rIdx.MaxDLen, dest_len);
			if(rIdx.IdenticalCount == i && src_len == dest_len && *PTR32(tcm_entry.S) == *PTR32(tcm_entry.D)) {
				tcm_entry.F |= tcm_entry.fEqual;
				rIdx.IdenticalCount++;
			}
			rIdx.insert(&tcm_entry);
		}
	}
	rFinalIdx.SL = rIdx.MaxSLen;
	rFinalIdx.DL = rIdx.MaxDLen;
	rFinalIdx.IdenticalCount = rIdx.IdenticalCount;
	if(rIdx.IdenticalCount == rFrom.MapCount) {
        // tables are equal
		rFinalIdx.Flags |= TranslIndex::fIdentical;
	}
	else if(rIdx.getCount() == 0) {
		rFinalIdx.Flags |= TranslIndex::fEmpty;
	}
	else {
		rIdx.Sort();
		THROW(rFinalIdx.Setup(rIdx));
	}
	CATCHZOK
	return ok;
}

int SLAPI SCodepageMapPool::MakeTranslIndex(const CpMap & rFrom, const CpMap & rTo, TranslIndex & rIdx)
{
	CMapTranslIndexTest intr_idx;
	return Helper_MakeTranslIndex(rFrom, rTo, intr_idx, rIdx);
}

uint SLAPI SCodepageMapPool::Translate(const CpMap & rFrom, const CpMap & rTo, const uint8 * pSrc, size_t srcLen, SString & rDest)
{
    for(size_t i = 0; i < srcLen; i++) {
        const uint8 c = pSrc[i];

    }
	return 0;
}

uint SLAPI SCodepageMapPool::Compare(const CpMap & rS1, const CpMap & rS2) const
{
    uint   result = 0;
    {
    	//
    	// Сначала сравниваем первые 128 символов [0..0x7f]
    	//
    	int    is_eq128 = 1;
		for(uint i = 0; is_eq128 && i < rS1.MapCount; i++) {
			const uint8 c1 = rS1.P_Map[i].B[0];
			if(rS1.P_Map[i].B[1] == 0) {
				if(c1 >= 0 && c1 <= 0x7f) {
					int    found = 0;
                    for(uint j = 0; !found && j < rS2.MapCount; j++) {
						const uint8 c2 = rS2.P_Map[j].B[0];
						if(c2 > c1)
							break;
						else if(rS2.P_Map[i].B[1] == 0) {
                            if(c2 == c1) {
								if(rS1.P_Map[i].U2 != rS2.P_Map[j].U2)
									is_eq128 = 0;
								found = 1;
                            }
						}
                    }
                    if(!found)
						is_eq128 = 0;
				}
			}
		}
		if(is_eq128)
			result |= 0x01;
    }
    {
    	//
    	// Сравниваем всю таблицу
    	//
		const CpMap * p1 = 0;
		const CpMap * p2 = 0;
    	if(rS1.MapCount > rS2.MapCount) {
			p1 = &rS2;
			p2 = &rS1;
    	}
    	else {
			p1 = &rS1;
			p2 = &rS2;
    	}
    	int    is_eq = 1;
    	for(uint i = 0; is_eq && i < p1->MapCount; i++) {
			const uint32 c1 = *PTR32C(p1->P_Map[i].B);
			const uint16 u1 = p1->P_Map[i].U2;
			int    found = 0;
			for(uint j = 0; !found && j < p2->MapCount; j++) {
				const uint32 c2 = *PTR32C(p2->P_Map[j].B);
				if(c2 == c1) {
					if(p2->P_Map[j].U2 != u1)
						is_eq = 0;
					found = 1;
				}
				else if(c2 > c1)
					break;
			}
			if(!found)
				is_eq = 0;
    	}
    	if(is_eq) {
			result |= 0x02;
			if(rS1.MapCount == rS2.MapCount)
				result |= 0x04;
		}
    }
    return result;
}

static int Test_TranslateFile(const char * pSrcFileName, const char * pDestFileName, const SCodepageMapPool::TranslIndex & rIdx)
{
	int    ok = 1;
	SString line_buf;
	SString dest_line_buf;
	SFile  f_in(pSrcFileName, SFile::mRead);
	SFile  f_out(pDestFileName, SFile::mWrite);
	THROW(f_in.IsValid());
	THROW(f_out.IsValid());
	while(f_in.ReadLine(line_buf)) {
		dest_line_buf.Z();
		for(uint i = 0; i < line_buf.Len(); i++) {
			union {
				uint8  Inp[4];
				uint32 InpDw;
			} u;
			u.InpDw = 0;
			u.Inp[0] = line_buf.C(i);
			const uint8 * p_outp = rIdx.Search(u.Inp);
			if(p_outp)
				dest_line_buf.CatChar(p_outp[0]);
			else
				dest_line_buf.CatChar(u.Inp[0]);
		}
		THROW(f_out.WriteLine(dest_line_buf));
	}
	CATCHZOK
	return ok;
}

struct _TestTranslIndexEntry {
	uint   SrcCpIdx;
	uint   DestCpIdx;
	SString ResultFileName;
	SCodepageMapPool::TranslIndex Ti;
};

int SLAPI SCodepageMapPool::Test(const SUnicodeTable * pUt, const char * pMapPoolFileName, const char * pMapTranslFileName)
{
	int    ok = 1;
	SString temp_buf;
	{
		//
		// Тестовый блок - проверяет таблицы windows стандартными windows-же функциями
		//
		SString line_buf;
		SString line_buf2;
		//PPGetFilePath(PPPATH_OUT, "SCodepageMapPool.txt", temp_buf);
		temp_buf = pMapPoolFileName;
		SFile  f_out(temp_buf, SFile::mWrite);
		//PPGetFilePath(PPPATH_OUT, "SCodepageMapTransl.txt", temp_buf);
		temp_buf = pMapTranslFileName;
		SFile  f_out_transl(temp_buf, SFile::mWrite);
		uint   out_line_no = 0;
		CMapTranslIndexTest cmt_list;
		TSCollection <_TestTranslIndexEntry> test_tidx_list;
		for(uint cpidx = 0; cpidx < GetCount(); cpidx++) {
			CpMap map;
			if(GetByPos(cpidx, &map)) {
				if(f_out.IsValid()) {
					if(out_line_no == 0) {
						line_buf.Z();
						line_buf.Cat("Name").Tab();
						line_buf.Cat("CpSis").Tab();
						line_buf.Cat("Code").Tab();
						line_buf.Cat("Version").Tab();
						line_buf.Cat("Id").Tab();
						line_buf.Cat("MbMl").Tab();
						line_buf.Cat("Flags").Tab();
						line_buf.Cat("Solid256Offs").Tab();
						line_buf.Cat("StraightCount").Tab();
						line_buf.Cat("MapCount").Tab();
						line_buf.Cat("FallbackCount").Tab();
						line_buf.Cat("NScriptCount").Tab();
						line_buf.Cat("NScriptList").Tab();
						f_out.WriteLine(line_buf.CR());
					}
					//
					line_buf.Z();
					line_buf.Cat(map.Name).Tab();
					GetSisCode(map.CpSis, temp_buf);
					line_buf.Cat(temp_buf).Tab();
					line_buf.Cat(map.Code).Tab();
					line_buf.Cat(map.Version).Tab();
					line_buf.Cat((long)map.Id).Tab();
					line_buf.Cat((long)map.MbMl).Tab();
					line_buf.CatHex((long)map.Flags).Tab();
					line_buf.Cat((long)map.Solid256Offs).Tab();
					line_buf.Cat((long)map.StraightCount).Tab();
					line_buf.Cat(map.MapCount).Tab();
					line_buf.Cat(map.FallbackCount).Tab();
					line_buf.Cat(map.NScriptCount).Tab();
					if(map.P_NScript) {
						for(uint si = 0; si < map.NScriptCount; si++) {
							GetSNScriptCode(map.P_NScript[si], temp_buf);
							if(si)
								line_buf.CatDiv(',', 2);
							line_buf.Cat(temp_buf);
						}
					}
					f_out.WriteLine(line_buf.CR());
					out_line_no++;
				}
                if(map.CpSis == ssisWindows && map.Id != cpUndef && map.P_Map) {
					int    result_len = 0;
					int    _err = 0;
					for(uint i = 0; i < map.MapCount; i++) {
						wchar_t test_wbuf[64];
						uchar   test_cbuf[64];
						const   MapEntry & r_entry = map.P_Map[i];
						memcpy(test_cbuf, r_entry.B, 4);
						test_cbuf[4] = 0;
						const int _in_clen = sstrlen(test_cbuf);
						if(_in_clen) {
							result_len = MultiByteToWideChar(map.Id, 0, (char *)test_cbuf, _in_clen, test_wbuf, SIZEOFARRAY(test_wbuf));
							//
							assert(result_len > 0);
							assert(test_wbuf[0] == static_cast<wchar_t>(r_entry.U2));
							THROW(result_len > 0);
							THROW(test_wbuf[0] == static_cast<wchar_t>(r_entry.U2));
							//
							test_wbuf[0] = static_cast<wchar_t>(r_entry.U2);
							test_wbuf[1] = 0;
							result_len = WideCharToMultiByte(map.Id, 0, test_wbuf, 1, (char *)test_cbuf, sizeof(test_cbuf), 0, 0);
							assert(result_len > 0);
							assert(memcmp(test_cbuf, r_entry.B, result_len) == 0);
							THROW(result_len > 0);
							THROW(memcmp(test_cbuf, r_entry.B, result_len) == 0)
						}
					}
					if(oneof10(map.Id, cp1250, cp1251, cp1252, cp1253, cp1254, cp1255, cp1256, cp866, cp437, cp737)) {
						(line_buf = "Case testing").CatDiv(':', 0);
						uint  case_err_count = 0;
						const SCodepage preserve_cp = SLS.GetCodepage();
						SLS.SetCodepage(map.Id);
						for(uint ci = 0; ci < map.MapCount; ci++) {
							{
								const MapEntry * p_ce = map.Test_ToLower(map.P_Map+ci, pUt);
								if(p_ce) {
									const uint8 mc = p_ce->B[0];
									const int crtc = tolower(map.P_Map[ci].B[0]);
									if(mc != crtc) {
										case_err_count++;
										line_buf.Space().CatHex(mc).Cat("!=").CatHex((long)crtc).Space().CatChar('(').CatHex(map.P_Map[ci].B[0]).CatChar(')');
									}
								}
							}
							{
								const MapEntry * p_ce = map.Test_ToUpper(map.P_Map+ci, pUt);
								if(p_ce) {
									const uint8 mc = p_ce->B[0];
									const int crtc = toupper(map.P_Map[ci].B[0]);
									if(mc != crtc) {
										case_err_count++;
										line_buf.Space().CatHex(mc).Cat("!=").CatHex((long)crtc).Space().CatChar('(').CatHex(map.P_Map[ci].B[0]).CatChar(')');
									}
								}
							}
						}
						if(!case_err_count)
							line_buf.Space().Cat("ok");
						f_out.WriteLine(line_buf.CR());
						out_line_no++;
						SLS.SetCodepage(preserve_cp);
					}
                }
                {
					for(uint cpj = 0; cpj < GetCount(); cpj++) {
						CpMap map2;
						if(cpj != cpidx && GetByPos(cpj, &map2)) {
							uint cmpr = Compare(map, map2);
							if((cmpr & 0x02) /*|| !(cmpr & 0x01)*/) {
								line_buf.Z().Tab().Cat("compare").Space().Cat(map2.Name).CatDiv('=', 1).CatHex((ulong)cmpr);
								f_out.WriteLine(line_buf.CR());
								out_line_no++;
							}
							{
								const size_t prefix_len = MAX(map.Name.Len()+1, map2.Name.Len()+1);
								TranslIndex final_ctm_idx;
								Helper_MakeTranslIndex(map, map2, cmt_list, final_ctm_idx);
								for(uint cmti = 0; cmti < cmt_list.getCount(); cmti++) {
									const CMapTranslEntry & r_cmt_item = cmt_list.at(cmti);
									if(!(r_cmt_item.F & CMapTranslEntry::fNone)) {
										const uint8 * p_fctm = final_ctm_idx.Search(r_cmt_item.S);
										assert(p_fctm);
										if(r_cmt_item.D[0]) {
											assert(r_cmt_item.D[0] == p_fctm[0]);
											if(r_cmt_item.D[1]) {
												assert(r_cmt_item.D[1] == p_fctm[1]);
												if(r_cmt_item.D[2]) {
													assert(r_cmt_item.D[2] == p_fctm[2]);
													if(r_cmt_item.D[3]) {
														assert(r_cmt_item.D[3] == p_fctm[3]);
													}
												}
											}
										}
									}
								}
								//static int Test_TranslateFile(const char * pSrcFileName, const char * pDestFileName, const SCodepageMapPool::TranslIndex & rIdx)
								if((oneof2(map.Id, cp1251, cp866) || (map.Code == "KOI8_R")) && (oneof2(map2.Id, cp1251, cp866) || (map2.Code == "KOI8_R"))) {
									{
										_TestTranslIndexEntry * p_new_tti = test_tidx_list.CreateNewItem();
										THROW(p_new_tti);
										p_new_tti->SrcCpIdx = cpidx;
										p_new_tti->DestCpIdx = cpj;
										p_new_tti->Ti = final_ctm_idx;
									}
									const uint ml = MAX(cmt_list.MaxSLen, cmt_list.MaxDLen);
									if(/*map.Id == cp1251 && map2.Id == cp866*/0) {
										uint8 _sb[4];
										char  _test_str[32];
										for(uint si = 0; si < 256; si++) {
											_sb[0] = si;
											_sb[1] = 0;
											_sb[2] = 0;
											_sb[3] = 0;
											const uint8 * p_db = final_ctm_idx.Search(_sb);
											assert(p_db);
											_test_str[0] = (char)_sb[0];
											_test_str[1] = 0;
											if(map.Id == cp1251 && map2.Id == cp866) {
												SCharToOem(_test_str);
												assert(_test_str[0] == p_db[0]);
											}
											else if(map.Id == cp866 && map2.Id == cp1251) {
												SOemToChar(_test_str);
												assert(_test_str[0] == p_db[0]);
											}
											/*
											else if(map.Id == cp866 && map2.Code == "KOI8_R") {
												assert(PTR8(_s_866_to_koi7(_test_str))[0] == p_db[0]);
											}
											*/
										}
									}
									line_buf.Z().Cat(map.Name).Align(prefix_len, ADJ_LEFT);
									line_buf2.Z().Cat(map2.Name).Align(prefix_len, ADJ_LEFT);
									for(uint ti = 0; ti < cmt_list.getCount(); ti++) {
										const CMapTranslEntry & r_cmt = cmt_list.at(ti);
										if(!(r_cmt.F & CMapTranslEntry::fNone)) {
											line_buf.CatHex(r_cmt.S[0]);
											line_buf2.CatHex(r_cmt.D[0]);
											if(ml > 1) {
												line_buf.CatHex(r_cmt.S[1]);
												line_buf2.CatHex(r_cmt.D[1]);
												if(ml > 2) {
													line_buf.CatHex(r_cmt.S[2]);
													line_buf2.CatHex(r_cmt.D[2]);
													if(ml > 3) {
														line_buf.CatHex(r_cmt.S[3]);
														line_buf2.CatHex(r_cmt.D[3]);
													}
												}
											}
											line_buf.Space();
											line_buf2.Space();
										}
									}
									f_out_transl.WriteLine(line_buf.CR());
									f_out_transl.WriteLine(line_buf2.CR());
								}
								/*
								if(cmt_list.MaxSLen && cmt_list.MaxDLen) {
									const uint ml = MAX(cmt_list.MaxSLen, cmt_list.MaxDLen);
									line_buf.Z().Cat(map.Name).Align(prefix_len, ADJ_LEFT);
									line_buf2.Z().Cat(map2.Name).Align(prefix_len, ADJ_LEFT);
									for(uint ti = 0; ti < cmt_list.getCount(); ti++) {
										const CMapTranslEntry & r_cmt = cmt_list.at(ti);
										if(!(r_cmt.F & CMapTranslEntry::fNone)) {
											line_buf.CatHex(r_cmt.S[0]);
											line_buf2.CatHex(r_cmt.D[0]);
											if(ml > 1) {
												line_buf.CatHex(r_cmt.S[1]);
												line_buf2.CatHex(r_cmt.D[1]);
												if(ml > 2) {
													line_buf.CatHex(r_cmt.S[2]);
													line_buf2.CatHex(r_cmt.D[2]);
													if(ml > 3) {
														line_buf.CatHex(r_cmt.S[3]);
														line_buf2.CatHex(r_cmt.D[3]);
													}
												}
											}
											line_buf.Space();
											line_buf2.Space();
										}
									}
									f_out_transl.WriteLine(line_buf.CR());
									f_out_transl.WriteLine(line_buf2.CR());
								}
								*/
							}
						}
					}
                }
				{
					//
					// Тест обратной трансляции
					//
					SStringU org_ustr;
					SString org_cstr;
					SStringU test_ustr;
					SString test_cstr;
					for(uint i = 0; i < map.MapCount; i++) {
						const   MapEntry & r_entry = map.P_Map[i];
						if(i != 0 || r_entry.B[0]) {
							assert(r_entry.B[0]);
							THROW(r_entry.B[0]);
							if(r_entry.B[0]) {
								org_cstr.CatChar(r_entry.B[0]);
								if(r_entry.B[1]) {
									org_cstr.CatChar(r_entry.B[1]);
									if(r_entry.B[2]) {
										org_cstr.CatChar(r_entry.B[2]);
										//assert(r_entry.B[3] != 0);
										if(r_entry.B[3]) {
											org_cstr.CatChar(r_entry.B[3]);
										}
									}
								}
							}
							assert(r_entry.U2 != 0);
							THROW(r_entry.U2 != 0);
							org_ustr.CatChar(r_entry.U2);
						}
					}
					map.TranslateToU(org_cstr.ucptr(), org_cstr.Len(), test_ustr);
					assert(test_ustr.Len() == org_ustr.Len());
					assert(test_ustr.IsEqual(org_ustr));
					THROW(test_ustr.IsEqual(org_ustr));
					//
					map.TranslateToB2(test_ustr, test_ustr.Len(), test_cstr.Z());
					assert(test_cstr.Len() == org_cstr.Len());
					assert(test_cstr.IsEqual(org_cstr));
					THROW(test_cstr.IsEqual(org_cstr));
				}
				/* Тест построения ACS-таблиц для btrieve
				if(map.MapCount <= 256 && map.MbMl == 1) {
					uint8  acs[512];
					memzero(acs, sizeof(acs));

                    size_t acs_ptr = 0;
					acs[acs_ptr++] = 0xAC;
                    strnzcpy((char *)(acs+acs_ptr), map.Code, 8);
                    acs_ptr += 8;

					uint8   kb[4];
					PTR32(kb)[0] = 0;
                    for(uint i = 0; i < 256; i++) {
						kb[0] = (uint8)i;
						const MapEntry * p_ce = map.SearchC(kb);
						if(p_ce) {
							const MapEntry * p_ue = map.Test_ToUpper(p_ce, pUt);
							if(p_ue && p_ue->B[1] == 0)
								acs[acs_ptr++] = p_ue->B[0];
							else
								acs[acs_ptr++] = (uint8)i;
						}
						else
							acs[acs_ptr++] = (uint8)i;
                    }
                    assert(acs_ptr == 265);
                    {
                    	(temp_buf = map.Name).Dot().Cat("acs");
                    	PPGetFilePath(PPPATH_OUT, temp_buf, line_buf);
                    	SFile f_acs(line_buf, SFile::mWrite|SFile::mBinary);
                    	if(f_acs.IsValid()) {
							f_acs.Write(acs, acs_ptr);
						}
                    }
				}
				*/
			}
		}
		{
			const SString test_src_1251_file_name = "D:/Papyrus/Src/PPTEST/DATA/rustext.txt";
			SString test_dest_file_name;
			SPathStruc ps;
			{
				for(uint i = 0; i < test_tidx_list.getCount(); i++) {
					_TestTranslIndexEntry * p_tti = test_tidx_list.at(i);
					CpMap src_map;
					CpMap dest_map;
					assert(GetByPos(p_tti->SrcCpIdx, &src_map));
					assert(GetByPos(p_tti->DestCpIdx, &dest_map));
					if(src_map.Id == 1251) {
						test_dest_file_name.Z();
						ps.Split(test_src_1251_file_name);
						ps.Nam.CatChar('-').Cat(dest_map.Name);
						ps.Merge(test_dest_file_name);
						Test_TranslateFile(test_src_1251_file_name, test_dest_file_name, p_tti->Ti);
						p_tti->ResultFileName = test_dest_file_name;
					}
				}
			}
			{
				for(uint i = 0; i < test_tidx_list.getCount(); i++) {
					_TestTranslIndexEntry * p_tti = test_tidx_list.at(i);
					if(p_tti->ResultFileName.Empty()) {
						CpMap src_map;
						CpMap dest_map;
						assert(GetByPos(p_tti->SrcCpIdx, &src_map));
						assert(GetByPos(p_tti->DestCpIdx, &dest_map));
						for(uint j = 0; j < test_tidx_list.getCount(); j++) {
							const _TestTranslIndexEntry * p_tti2 = test_tidx_list.at(j);
							if(p_tti2->DestCpIdx == p_tti->SrcCpIdx && p_tti2->ResultFileName.NotEmpty()) {
								test_dest_file_name.Z();
								ps.Split(p_tti2->ResultFileName);
								ps.Nam.CatChar('-').Cat(dest_map.Name);
								ps.Merge(test_dest_file_name);
								Test_TranslateFile(p_tti2->ResultFileName, test_dest_file_name, p_tti->Ti);
								p_tti->ResultFileName = test_dest_file_name;
								break;
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

SLAPI SUnicodeBlock::SUnicodeBlock()
{
}

SLAPI SUnicodeBlock::~SUnicodeBlock()
{
}

/* use PPReadUnicodeBlockRawData instead
int SLAPI SUnicodeBlock::ReadRaw(const char * pUnicodePath, const char * pCpPath)
{
	int    ok = 1;
	THROW(Ut.ParseSource(pUnicodePath));
	THROW(Cpmp.ParseXml(pCpPath, &Ut));
	CATCHZOK
	return ok;
}
*/

int SLAPI SUnicodeBlock::Read(const char * pFileName)
{
    int    ok = 1;
    SSerializeContext sctx;
    SBuffer buffer;
    {
    	StrgHeader hdr;
    	SFile f_in(pFileName, SFile::mRead|SFile::mBinary);
    	THROW(f_in.IsValid());
        THROW(f_in.Read(&hdr, sizeof(hdr)));
        THROW(hdr.Signature == 0x50435455U);
        THROW(hdr.Version == 0);
		THROW(f_in.Read(buffer));
		{
			SCRC32 cc;
			uint32 _crc = cc.Calc(0, buffer.GetBuf(0), buffer.GetAvailableSize());
			THROW(_crc == hdr.CRC);
		}
		THROW(Ut.Serialize(-1, buffer, &sctx));
		THROW(Cpmp.Serialize(-1, buffer, &sctx));
    }
    CATCHZOK
    return ok;
}

int SLAPI SUnicodeBlock::Write(const char * pFileName)
{
    int    ok = 1;
    SSerializeContext sctx;
    SBuffer buffer;
    THROW(Ut.Serialize(+1, buffer, &sctx));
    THROW(Cpmp.Serialize(+1, buffer, &sctx));
    {
    	const  size_t bsize = buffer.GetAvailableSize();
    	StrgHeader hdr;
        MEMSZERO(hdr);
        hdr.Signature = 0x50435455U;
        hdr.Version = 0;
        hdr.Flags = 0;

        SCRC32 cc;
        hdr.CRC = cc.Calc(0, buffer.GetBuf(0), bsize);
        //
        SFile f_out(pFileName, SFile::mWrite|SFile::mBinary);
        THROW(f_out.IsValid());
        THROW(f_out.Write(&hdr, sizeof(hdr)));
        THROW(f_out.Write(buffer));
    }
    CATCHZOK
    return ok;
}

