// UED.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop
#if(_MSC_VER >= 1900)
	#include <cmath>
	#include <unicode\uclean.h>
	#include <unicode\brkiter.h>
	#include <unicode\measunit.h>
	#include <unicode\measfmt.h>
	#include <unicode\unumberformatter.h>
#endif
#include <sartre.h>
#include <ued.h>
#include <ued-id.h>

/*static*/uint UED::GetMetaRawDataBits(uint64 meta)
{
	uint   result = 0;
	if(IsMetaId(meta)) {
		const uint32 meta_lodw = LoDWord(meta);
		if(meta_lodw & 0x80000000U)
			result = 56;
		else if(meta_lodw & 0x40000000U)
			result = 48;
		else 
			result = 32;
	}
	return result;
}

/*static*/uint UED::GetRawDataBits(uint64 ued)
{
	uint   result = 0;
	if(!IsMetaId(ued)) {
		const uint32 hi_dword = HiDWord(ued);
		if(hi_dword & 0x80000000U)
			result = 56;
		else if(hi_dword & 0x40000000)
			result = 48;
		else if(hi_dword)
			result = 32;
	}
	return result;
}

/*static*/uint64 UED::GetMeta(uint64 ued)
{
	if(IsMetaId(ued))
		return  0x0000000100000001ULL; // meta
	else {
		const uint32 dw_hi = HiDWord(ued);
		const uint8  b_hi = static_cast<uint8>(dw_hi >> 24);
		if(b_hi & 0x80) {
			return (0x0000000100000000ULL & static_cast<uint64>(dw_hi & 0xff000000U));
		}
		else if(b_hi & 0x40) {
			return (0x0000000100000000ULL & static_cast<uint64>(dw_hi & 0xffff0000U));
		}
		else if(b_hi) {
			return (0x0000000100000000ULL & static_cast<uint64>(dw_hi & 0x0fffffffU));
		}
		else
			return 0ULL;
	}
}

/*static*/bool UED::GetRawValue(uint64 ued, uint64 * pRawValue)
{
	bool   ok = true;
	uint64 raw_value = 0ULL;
	uint   bits = GetRawDataBits(ued);
	assert(oneof4(bits, 56, 48, 32, 0));
	if(bits == 32) {
		raw_value = LoDWord(ued);	
	}
	else if(bits == 48) {
		raw_value = ued & 0x0000ffffffffffffULL;
	}
	else if(bits == 56) {
		raw_value = ued & 0x0000ffffffffffffULL;
	}
	else
		ok = false;
	ASSIGN_PTR(pRawValue, raw_value);
	return ok;
}

/*static*/bool UED::GetRawValue32(uint64 ued, uint32 * pRawValue)
{
	bool   ok = true;
	uint32 raw_value = 0UL;
	uint   bits = GetRawDataBits(ued);
	assert(oneof4(bits, 56, 48, 32, 0));
	if(bits == 32)
		raw_value = LoDWord(ued);	
	else
		ok = false;
	ASSIGN_PTR(pRawValue, raw_value);
	return ok;
}

/*static*/uint64 UED::ApplyMetaToRawValue(uint64 meta, uint64 rawValue)
{
	uint64 result = 0ULL;
	const  uint bits = GetMetaRawDataBits(meta);
	assert(bits < 64);
	THROW(bits > 0);
	THROW((64 - SBits::Clz(rawValue)) <= bits)
	{
		const uint32 meta_lodw = LoDWord(meta);
		if(meta_lodw & 0x80000000) {
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00ffffffffffffffULL);
		}
		else if(meta_lodw & 0x40000000) {
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x0000ffffffffffffULL);
		}
		else {
			result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00000000ffffffffULL);
		}
	}
	CATCH
		result = 0ULL;
	ENDCATCH
	return result;
}

/*static*/uint64 UED::ApplyMetaToRawValue32(uint64 meta, uint32 rawValue)
{
	uint64 result = 0ULL;
	const  uint bits = GetMetaRawDataBits(meta);
	assert(bits < 64);
	THROW(bits > 0);
	const uint _clz_ = SBits::Clz(rawValue);
	THROW((64 - (_clz_+32)) <= bits)
	{
		const uint32 meta_lodw = LoDWord(meta);
		THROW(!(meta_lodw & 0x80000000)); // @todo @err (not applicable)
		THROW(!(meta_lodw & 0x40000000)); // @todo @err (not applicable)
		result = (static_cast<uint64>(meta_lodw) << 32) | (rawValue & 0x00000000ffffffffULL);
	}
	CATCH
		result = 0ULL;
	ENDCATCH
	return result;
}

#if(_MSC_VER >= 1900) // {
// @construction {
#include <..\OSF\abseil\absl\numeric\int128.h>

static const uint64 __ued_planarangle = 0x90ULL;
static const uint64 __ued_geoloc      = 0x93ULL;

/*static*/uint64 UED::SetRaw_GeoLoc(const SGeoPosLL & rGeoPos)
{
	uint64 result = 0;
	if(rGeoPos.IsValid()) {
		const uint64 ued_width = ((1 << 28) - 1); // 28 bits
		uint64 _lat = static_cast<uint64>((rGeoPos.Lat + 90.0)  * 1000000.0);
		uint64 _lon = static_cast<uint64>((rGeoPos.Lon + 180.0) * 1000000.0);
		absl::uint128 _lat128 = (_lat * ued_width);
		_lat128 /= absl::uint128(180ULL * 1000000ULL);
		assert(_lat128 <= ued_width);
		absl::uint128 _lon128 = (_lon * ued_width);
		_lon128 /= absl::uint128(360ULL * 1000000ULL);
		assert(_lon128 <= ued_width);
		result = (__ued_geoloc << 56) | (static_cast<uint64>(_lat128) << 28) | (static_cast<uint64>(_lon128));
		//
		// test straighten
		//
		SGeoPosLL test_geopos;
		absl::uint128 _lat128_ = ((result >> 28) & 0xfffffffULL) * absl::uint128(180ULL * 1000000ULL);
		absl::uint128 _lon128_ = (result & 0xfffffffULL) * absl::uint128(360ULL * 1000000ULL);
		_lat128_ /= ued_width;
		_lon128_ /= ued_width;
		test_geopos.Lat = (static_cast<double>(_lat128_) / 1000000.0) - 90.0;
		test_geopos.Lon = (static_cast<double>(_lon128_) / 1000000.0) - 180.0;
	}
	return result;
}

/*static*/bool UED::GetRaw_GeoLoc(uint64 ued, SGeoPosLL & rGeoPos)
{
	bool   ok = false;
	if((ued >> 56) == __ued_geoloc) {
		const uint64 ued_width = ((1 << 28) - 1); // 28 bits
		absl::uint128 _lat128 = ((ued >> 28) & 0xfffffffULL) * absl::uint128(180ULL * 1000000ULL);
		absl::uint128 _lon128 = (ued & 0xfffffffULL) * absl::uint128(360ULL * 1000000ULL);
		_lat128 /= ued_width;
		_lon128 /= ued_width;
		rGeoPos.Lat = (static_cast<double>(_lat128) / 1000000.0) - 90.0;
		rGeoPos.Lon = (static_cast<double>(_lon128) / 1000000.0) - 180.0;
		ok = true;
	}
	return ok;
}

static uint64 UedPlanarAngleMask     = 0x007fffffffffffULL;
static uint64 UedPlanarAngleSignMask = 0x00800000000000ULL;

/*static*/uint64 UED::SetRaw_PlanarAngleDeg(double deg)
{
	uint64 result = 0;
	const uint64 _divisor = 360ULL * 3600ULL;
	const bool  minus = (deg < 0);
	const uint64 maxv = (UedPlanarAngleMask / _divisor) * _divisor;
	assert((maxv % 360ULL) == 0ULL);
	double _deg = fmod(deg, 360.0);
	assert(_deg > -360.0 && _deg < 360.0);
	_deg = fabs(_deg);
	if(minus)
		result = (__ued_planarangle << 56) | (uint64)(_deg * (maxv / 360ULL)) | UedPlanarAngleSignMask;
	else
		result = (__ued_planarangle << 56) | (uint64)(_deg * (maxv / 360ULL));
	assert((result & ~(UedPlanarAngleMask | UedPlanarAngleSignMask)) == (__ued_planarangle << 56));
	return result;
}

/*static*/bool UED::GetRaw_PlanarAngleDeg(uint64 ued, double & rDeg)
{
	bool   ok = false;
	if((ued >> 56) == __ued_planarangle) {
		const uint64 _divisor = 360ULL * 3600ULL;
		const uint64 maxv = (UedPlanarAngleMask / _divisor) * _divisor;
		rDeg = static_cast<double>(ued & UedPlanarAngleMask) / (maxv / 360ULL);
		assert(rDeg >= 0.0 && rDeg < 360.0);
		if(ued & UedPlanarAngleSignMask)
			rDeg = -rDeg;
		ok = true;
	}
	return ok;
}

/*static*/uint64 UED::SetRaw_Color(const SColor & rC)
{
	union {
		uint8  Bytes[4];
		uint32 U;
	} rv;
	rv.Bytes[0] = rC.B;
	rv.Bytes[1] = rC.G;
	rv.Bytes[2] = rC.R;
	rv.Bytes[3] = rC.Alpha;
	return ApplyMetaToRawValue32(UED_META_COLORRGB, rv.U);
}

/*static*/bool UED::GetRaw_Color(uint64 ued, SColor & rC)
{
	bool   ok = false;
	union {
		uint8  Bytes[4];
		uint32 U;
	} rv;
	if(GetRawValue32(ued, &rv.U)) {
		rC.Set(rv.Bytes[2], rv.Bytes[1], rv.Bytes[0]);
		rC.SetAlpha(rv.Bytes[3]);
		ok = true;
	}
	return ok;
}
// } @construction
#endif // } (_MSC_VER >= 1900)
//
//
//
static IMPL_CMPFUNC(SrUedContainer_TextEntry, p1, p2)
{
	const SrUedContainer::TextEntry * p_e1 = static_cast<const SrUedContainer::TextEntry *>(p1);
	const SrUedContainer::TextEntry * p_e2 = static_cast<const SrUedContainer::TextEntry *>(p2);
	RET_CMPCASCADE2(p_e1, p_e2, Locale, Id);
}

SrUedContainer::SrUedContainer() : LinguaLocusMeta(0), Ht(1024*32, 0), LastSymbHashId(0)
{
}
	
SrUedContainer::~SrUedContainer()
{
}

uint64 SrUedContainer::SearchBaseIdBySymbId(uint symbId, uint64 meta) const
{
	uint64 id = 0;
	for(uint i = 0; !id && i < BL.getCount(); i++) {
		if(BL.at(i).SymbHashId == symbId) {
			uint64 temp_id = BL.at(i).Id;
			if(UED::BelongToMeta(temp_id, meta))
				id = temp_id;
		}
	}
	return id;
}

uint64 SrUedContainer::SearchBaseSymb(const char * pSymb, uint64 meta) const
{
	uint64 id = 0;
	if(!meta || UED::IsMetaId(meta)) {
		uint symb_hash_id = 0;
		if(Ht.Search(pSymb, &symb_hash_id, 0)) {
			id = SearchBaseIdBySymbId(symb_hash_id, meta); 
		}
	}
	return id;
}

bool   SrUedContainer::SearchBaseId(uint64 id, SString & rSymb) const
{
	rSymb.Z();
	bool   ok = false;
	for(uint i = 0; !ok && i < BL.getCount(); i++) {
		if(BL.at(i).Id == id) {
			if(Ht.GetByAssoc(BL.at(i).SymbHashId, rSymb))
				ok = true;
		}
	}
	return ok;
}

int SrUedContainer::ReplaceSurrogateLocaleIds(const SymbHashTable & rT, PPLogger * pLogger)
{
	int    ok = 1;
	SString temp_buf;
	SString locale_buf;
	THROW(LinguaLocusMeta);
	for(uint i = 0; i < TL.getCount(); i++) {
		TextEntry & r_e = TL.at(i);
		if(r_e.Locale) {
			THROW(rT.GetByAssoc(r_e.Locale, locale_buf));
			{
				uint64 locale_id = SearchBaseSymb(locale_buf, LinguaLocusMeta);
				if(!locale_id) {
					ok = PPSetError(PPERR_UED_SYMBFORMETANOTFOUND, 
						temp_buf.Z().Cat("line").CatDiv(':', 2).Cat(r_e.LineNo).Space().
						Cat(locale_buf).Space().Cat("->").Space().CatHex(LinguaLocusMeta));
					if(pLogger)
						pLogger->LogLastError();
					else
						CALLEXCEPT();
				}
				else {
					const bool btm_result = UED::BelongToMeta(locale_id, LinguaLocusMeta);
					if(!btm_result) {
						ok = PPSetError(PPERR_UED_VALUENOTBELONGTOMETA, 
							temp_buf.Z().Cat("line").CatDiv(':', 2).Cat(r_e.LineNo).Space().
							CatHex(locale_id).Space().Cat("->").Space().CatHex(LinguaLocusMeta));
						if(pLogger)
							pLogger->LogLastError();
						else
							CALLEXCEPT();
					}
					{
						//r_e.Locale = UED::MakeShort(locale_id, LinguaLocusMeta);
						assert(UED::GetMeta(locale_id) == LinguaLocusMeta);
						bool grvr = UED::GetRawValue32(locale_id, &r_e.Locale);
						assert(grvr);
						assert(r_e.Locale);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
	
int SrUedContainer::ReadSource(const char * pFileName, PPLogger * pLogger)
{
	int    ok = 1;
	uint   line_no = 0;
	SString line_buf;
	SString temp_buf;
	SString lang_buf;
	SString text_buf;
	SString log_buf;
	StringSet ss;
	uint   last_linglocus_temp_id = 0;
	SymbHashTable temporary_linglocus_tab(512);
	SStrScan scan;
	SFile f_in(pFileName, SFile::mRead);
	THROW(f_in.IsValid());
	while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
		line_no++;
		while(line_buf.C(0) == '\t')
			line_buf.ShiftLeft();
		if(line_buf.HasPrefix("//")) {
			; // comment
		}
		else if(line_buf.IsEmpty()) {
			; // empty line
		}
		else {
			size_t comment_pos = 0;
			if(line_buf.Search("//", 0, 0, &comment_pos)) {
				line_buf.Trim(comment_pos).Strip();
			}
			if(line_buf.NotEmpty()) {
				//line_buf.Tokenize(" \t", ss.Z());
				ss.Z();
				bool scan_ok = true;
				{
					scan.Set(line_buf, 0);
					if(scan.GetXDigits(temp_buf.Z())) {
						ss.add(temp_buf);
						scan.Skip();
						if(scan.GetQuotedString(temp_buf) || scan.GetIdent(temp_buf)) {
							ss.add(temp_buf);
							scan.Skip();
							if(!scan.IsEnd()) {
								if(scan.GetQuotedString(temp_buf)) {
									ss.add(temp_buf);
								}
								else
									scan_ok = false;
							}
						}
						else
							scan_ok = false;
					}
					else
						scan_ok = false;
				}
				uint ssc = ss.getCount();
				if(scan_ok && oneof2(ssc, 2, 3)) {
					uint64 id = 0;
					int   lang_id = 0;
					text_buf.Z();
					lang_buf.Z();
					uint   token_n = 0;
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						token_n++;
						temp_buf.Utf8ToLower();
						if(token_n == 1) {
							id = sxtou64(temp_buf);
						}
						else if(token_n == 2) {
							if(ssc == 2) {
								text_buf = temp_buf.StripQuotes();
							}
							else {
								assert(ssc == 3);
								lang_buf = temp_buf;
								uint llid = 0;
								if(!temporary_linglocus_tab.Search(lang_buf, &llid, 0)) {
									llid = ++last_linglocus_temp_id;
									temporary_linglocus_tab.Add(lang_buf, llid, 0);
								}
								lang_id = llid;
								//lang_id = RecognizeLinguaSymb(lang_buf, 1);
							}
						}
						else if(token_n == 3) {
							assert(ssc == 3);
							text_buf = temp_buf.StripQuotes();
						}
					}
					{
						log_buf.Z().Cat(id).Tab().Cat(lang_buf).Tab().Cat(text_buf);
						//PPLogMessage("ued-import.log", log_buf, LOGMSGF_TIME);
					}
					if(id) {
						if(ssc == 2) {
							if(text_buf.IsEqiAscii("lingualocus")) {
								if(!LinguaLocusMeta)
									LinguaLocusMeta = id;
								else {
									; // @error
								}
							}
							BaseEntry new_entry;
							new_entry.LineNo = line_no; // @v11.7.8
							new_entry.Id = id;
							uint   symb_hash_id = 0;
							if(!Ht.Search(text_buf, &symb_hash_id, 0)) {
								symb_hash_id = ++LastSymbHashId;
								Ht.Add(text_buf, symb_hash_id);
							}
							new_entry.SymbHashId = symb_hash_id;
							//AddS(text_buf, &new_entry.SymbP);
							BL.insert(&new_entry);
						}
						else if(ssc == 3) {
							if(lang_id) {
								TextEntry new_entry;
								new_entry.LineNo = line_no; // @v11.7.8
								new_entry.Id = id;
								new_entry.Locale = lang_id;
								AddS(text_buf, &new_entry.TextP);
								TL.insert(&new_entry);
							}
						}
					}
				}
				else {
					// invalid line
				}
			}
		}
	}
	Ht.BuildAssoc();
	THROW_SL(temporary_linglocus_tab.BuildAssoc());
	THROW(ReplaceSurrogateLocaleIds(temporary_linglocus_tab, pLogger));
	CATCHZOK
	return ok;
}

/*static*/void SrUedContainer::MakeUedCanonicalName(SString & rResult, long ver)
{
	rResult.Z();
	rResult.Cat("ued-id");
	if(ver > 0)
		rResult.CatChar('-').CatLongZ(ver, 4);
	else if(ver == 0)
		rResult.CatChar('-').Cat("????");
	else { // ver < 0 - file-name for programming interface source
		//
	}
}

/*static*/long SrUedContainer::SearchLastCanonicalFile(const char * pPath, SString & rFileName)
{
	long   result = 0; // version
	long   max_ver = 0;
	SString max_ver_filename;
	SString temp_buf;
	SString fn_dat;
	SString fn_hash;
	SString fn_wc;
	StringSet ss;
	SDirEntry de;
	SPathStruc ps;
	MakeUedCanonicalName(fn_wc, 0);
	(temp_buf = pPath).Strip().SetLastSlash().Cat(fn_wc).Dot().Cat("dat");
	for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
		if(de.IsFile()) {
			ss.Z();
			de.GetNameA(temp_buf);
			ps.Split(temp_buf);
			ps.Nam.Tokenize("-", ss);
			if(ss.getCount() == 3 && ss.getByIdx(2, temp_buf)) { // ued;id;version ("ued-id-0004")
				long iter_ver = temp_buf.ToLong();
				if(iter_ver > 0 && iter_ver > max_ver) {
					//
					// Необходимо убедиться что рядом с основным файлом присутствует файл с контрольной суммой (ext .sha256)
					//
					de.GetNameA(pPath, temp_buf);
					ps.Split(temp_buf);
					ps.Ext = "sha256";
					ps.Merge(temp_buf);
					if(fileExists(temp_buf)) {
						max_ver = iter_ver;
						de.GetNameA(pPath, max_ver_filename);
					}
				}
			}
		}
	}
	if(max_ver > 0) {
		assert(max_ver_filename.NotEmpty());
		assert(fileExists(max_ver_filename));
		rFileName = max_ver_filename;
		result = max_ver;
	}
	return result;
}
	
int SrUedContainer::WriteSource(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash)
{
	int    ok = 1;
	SString line_buf;
	SString temp_buf;
	THROW(!isempty(pFileName)); // @err
	THROW(LinguaLocusMeta); // @err
	{
		SFile f_out(pFileName, SFile::mWrite|SFile::mBinary);
		THROW_SL(f_out.IsValid());
		BL.sort(PTR_CMPFUNC(int64));
		TL.sort(PTR_CMPFUNC(SrUedContainer_TextEntry));
		{
			for(uint i = 0; i < BL.getCount(); i++) {
				const BaseEntry & r_e = BL.at(i);
				SearchBaseId(r_e.Id, temp_buf);
				assert(temp_buf.NotEmpty());
				line_buf.Z().CatHex(r_e.Id).Space().CatQStr(temp_buf).CRB();
				f_out.WriteLine(line_buf);
			}
		}
		{
			for(uint i = 0; i < TL.getCount(); i++) {
				const TextEntry & r_e = TL.at(i);
				line_buf.Z().CatHex(r_e.Id).Space();
				assert(r_e.Locale);
				uint64 ued_locus = UED::ApplyMetaToRawValue32(LinguaLocusMeta, r_e.Locale);
				assert(ued_locus);
				SearchBaseId(ued_locus, temp_buf);
				assert(temp_buf.NotEmpty());
				line_buf.Cat(temp_buf).Space(); // locale without quotes!
				GetS(r_e.TextP, temp_buf);
				assert(temp_buf.NotEmpty());
				line_buf.CatQStr(temp_buf);
				line_buf.CRB();
				f_out.WriteLine(line_buf);
			}
		}
	}
	{
		SFile f_in(pFileName, SFile::mRead|SFile::mBinary);
		SBinaryChunk bc_hash;
		THROW_SL(f_in.IsValid());
		THROW_SL(f_in.CalcHash(0, SHASHF_SHA256, bc_hash));
		ASSIGN_PTR(pHash, bc_hash);
		if(pPrevHash && bc_hash == *pPrevHash) {
			f_in.Close();
			SFile::Remove(pFileName);
			ok = -1;
		}
		else {
			SPathStruc ps(pFileName);
			ps.Ext = "sha256";
			ps.Merge(temp_buf);
			SFile f_hash(temp_buf, SFile::mWrite);
			THROW_SL(f_hash.IsValid());
			bc_hash.Hex(temp_buf);
			f_hash.WriteLine(temp_buf);
		}
	}
	CATCHZOK
	return ok;
}

bool SrUedContainer::GenerateSourceDecl_C(const char * pFileName, uint versionN, const SBinaryChunk & rHash)
{
	bool   ok = true;
	SString temp_buf;
	SString def_symb;
	SString def_value;
	SString meta_symb;
	SString h_sentinel_def;
	Generator_CPP gen(pFileName);
	const  SPathStruc ps(pFileName);
	uint   max_symb_len = 0;
	{
		ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
		gen.Wr_Comment(temp_buf.ToUpper());
		temp_buf.Z().Cat("version").CatDiv(':', 2).Cat(versionN);
		gen.Wr_Comment(temp_buf);
		temp_buf.Z().Cat("sha256").CatDiv(':', 2).CatHex(rHash.PtrC(), rHash.Len());
		gen.Wr_Comment(temp_buf);
		gen.Wr_Comment(temp_buf.Z());
	}
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				Ht.GetByAssoc(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				SETMAX(max_symb_len, def_symb.Len());
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongToMeta(r_be_inner.Id, r_be.Id)) {
							Ht.GetByAssoc(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							SETMAX(max_symb_len, def_symb.Len());
						}
					}
					gen.IndentDec();
				}
			}
		}
	}
	h_sentinel_def.Z().Cat("__").Cat(ps.Nam).CatChar('_').Cat("h").ToUpper();
	h_sentinel_def.ReplaceChar('-', '_');
	gen.Wr_IfDef(h_sentinel_def, 1);
	gen.Wr_Define(h_sentinel_def, 0);
	gen.WriteBlancLine();
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				Ht.GetByAssoc(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				assert(max_symb_len >= def_symb.Len());
				def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
				def_value.Z().Cat("0x").CatHex(r_be.Id).Cat("ULL");
				gen.Wr_Define(def_symb, def_value);
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					gen.IndentInc();
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongToMeta(r_be_inner.Id, r_be.Id)) {
							Ht.GetByAssoc(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							assert(max_symb_len >= def_symb.Len());
							def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
							def_value.Z().Cat("0x").CatHex(r_be_inner.Id).Cat("ULL");
							gen.Wr_Indent();
							gen.Wr_Define(def_symb, def_value);						
						}
					}
					gen.IndentDec();
				}
			}
		}
	}
	gen.WriteBlancLine();
	gen.Wr_EndIf(h_sentinel_def);
	return ok;
}

bool SrUedContainer::GenerateSourceDecl_Java(const char * pFileName, uint versionN, const SBinaryChunk & rHash)
{
	bool   ok = true;
	SString temp_buf;
	SString def_symb;
	SString def_value;
	SString meta_symb;
	SString h_sentinel_def;
	SFile  genf(pFileName, SFile::mWrite);
	const  SPathStruc ps(pFileName);
	uint   max_symb_len = 0;
	{
		ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf.ToUpper()).CR());
		temp_buf.Z().Cat("version").CatDiv(':', 2).Cat(versionN);
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf).CR());
		temp_buf.Z().Cat("sha256").CatDiv(':', 2).CatHex(rHash.PtrC(), rHash.Len());
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf).CR());
		genf.WriteLine(meta_symb.Z().Cat("//").CR());
	}
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				Ht.GetByAssoc(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				SETMAX(max_symb_len, def_symb.Len());
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongToMeta(r_be_inner.Id, r_be.Id)) {
							Ht.GetByAssoc(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							SETMAX(max_symb_len, def_symb.Len());
						}
					}
				}
			}
		}
	}
	genf.WriteBlancLine();
	temp_buf.Z().Cat("class").Space().Cat("UED_ID").Space().CatChar('{').CR();
	genf.WriteLine(temp_buf);
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				Ht.GetByAssoc(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				assert(max_symb_len >= def_symb.Len());
				def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
				def_value.Z().Cat("0x").CatHex(r_be.Id).Cat("L");
				temp_buf.Z().Tab().Cat("public static final long").Space().Cat(def_symb).CatChar('=').Space().Cat(def_value).Semicol().CR();
				genf.WriteLine(temp_buf);
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					//gen.IndentInc();
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongToMeta(r_be_inner.Id, r_be.Id)) {
							Ht.GetByAssoc(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							assert(max_symb_len >= def_symb.Len());
							def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
							def_value.Z().Cat("0x").CatHex(r_be_inner.Id).Cat("L");
							temp_buf.Z().Tab(2).Cat("public static final long").Space().Cat(def_symb).CatChar('=').Space().Cat(def_value).Semicol().CR();
							genf.WriteLine(temp_buf);
						}
					}
					//gen.IndentDec();
				}
			}
		}
	}
	temp_buf.Z().CatChar('}').CR();
	genf.WriteLine(temp_buf);
	genf.WriteBlancLine();
	//gen.Wr_EndIf(h_sentinel_def);
	return ok;
}

int SrUedContainer::VerifyByPreviousVersion(const SrUedContainer * pPrevC, PPLogger * pLogger)
{
	int    ok = -1;
	//
	// Здесь мы должны проверить следующие факты:
	// 1. Ни одна концепция из предыдущего релиза не должна поменять символа или значения
	// 2. Ни одна концепция из предыдущего релиза не должна исчезнуть
	// 3. Не должно быть дублирующихся значений
	// 4. Не должно быть дублирующихся символов в рамках одной концепции
	// 5. Наименования на натуральных языках могут меняться, но желательно проверить чтобы наименования 
	//   принадлежали соответствующему натуральному язык (планируем использовать icu).
	if(pPrevC) { // 1, 2
		ok = 1;
		SString temp_buf;
		SString this_symb;
		SString prev_symb;
		for(uint i = 0; i < pPrevC->BL.getCount(); i++) {
			const BaseEntry & r_be = pPrevC->BL.at(i);
			pPrevC->Ht.GetByAssoc(r_be.SymbHashId, prev_symb);
			if(SearchBaseId(r_be.Id, this_symb)) {
				if(prev_symb != this_symb) {
					temp_buf.Z().Cat(prev_symb).Cat("-->").Cat(this_symb);
					ok = PPSetError(PPERR_UED_PREVVERV_PREVSYMBMODIFIED, temp_buf);
					if(pLogger)
						pLogger->LogLastError();
				}
			}
			else if(prev_symb.NotEmpty()) {
				//PPERR_UED_PREVVERV_SYMBREMOVED      "Символ, существовавший в предудущей версии, удален в новой версии (%s)"
				ok = PPSetError(PPERR_UED_PREVVERV_SYMBREMOVED, prev_symb);
				if(pLogger)
					pLogger->LogLastError();
			}
		}
	}
	return ok;
}
	
int SrUedContainer::Verify(const char * pPath, long ver, SBinaryChunk * pHash)
{
	int    ok = 1;
	SString temp_buf;
	THROW(ver > 0);
	MakeUedCanonicalName(temp_buf, ver);
	{
		SString file_path;
		SString hash_file_path;
		binary256 hash;
		(file_path = pPath).SetLastSlash().Cat(temp_buf).Dot().Cat("dat");
		THROW_SL(fileExists(file_path));
		(hash_file_path = pPath).SetLastSlash().Cat(temp_buf).Dot().Cat("sha256");
		THROW_SL(fileExists(hash_file_path));
		{
			SFile f_in(file_path, SFile::mRead|SFile::mBinary);
			SFile f_hash(hash_file_path, SFile::mRead);
			SBinaryChunk bc_hash_stored;
			SBinaryChunk bc_hash_evaluated;
			THROW_SL(f_in.IsValid());
			THROW_SL(f_hash.IsValid());
			THROW_SL(f_in.CalcHash(0, SHASHF_SHA256, bc_hash_evaluated));
			THROW_SL(f_hash.ReadLine(temp_buf));
			THROW_SL(bc_hash_stored.FromHex(temp_buf.Chomp().Strip()));
			THROW(bc_hash_stored == bc_hash_evaluated);
			if(pHash) {
				*pHash = bc_hash_stored;
			}
		}
	}
	CATCHZOK
	return ok;
}

#if(_MSC_VER >= 1900)
	using namespace U_ICU_NAMESPACE;
#endif

int ProcessUed(const char * pSrcFileName, const char * pOutPath, const char * pCPath, const char * pJavaPath, bool forceUpdatePlDecl, PPLogger * pLogger)
{
	int    ok = 1;
	bool   unchanged = false; // Если хэш нового результатного файла не отличается от предыдущей версии, то true.
	//const char * p_file_name = "\\Papyrus\\Src\\Rsrc\\Data\\Sartre\\UED.txt";
	SString temp_buf;
	SStringU temp_buf_u;
	//PPLogger logger;
	//
	SBinaryChunk new_hash;
	SBinaryChunk prev_hash;
	SrUedContainer uedc;
	SrUedContainer uedc_prev;
	SString last_file_name;
	SString path;
	SString out_path;
	SString c_path;
	SString java_path;
	long   new_version = 0;
	THROW(!isempty(pSrcFileName)); // @todo @err
	{
		SPathStruc ps(pSrcFileName);
		SPathStruc ps_out;
		ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, path);
		path.RmvLastSlash();
		if(!isempty(pOutPath)) {
			ps_out.Split(pOutPath);
			ps_out.Merge(SPathStruc::fDrv|SPathStruc::fDir, out_path);
			out_path.RmvLastSlash();
		}
		else {
			ps_out = ps;
			out_path = path;
		}
		const long prev_version = SrUedContainer::SearchLastCanonicalFile(out_path, last_file_name);
		if(prev_version > 0) {
			THROW(uedc_prev.ReadSource(last_file_name, pLogger));
			THROW(uedc.Verify(out_path, prev_version, &prev_hash));
			new_version = prev_version+1;
		}
		THROW(uedc.ReadSource(pSrcFileName, pLogger));
		if(prev_version > 0) {
			THROW(uedc.VerifyByPreviousVersion(&uedc_prev, pLogger));
		}
		{
			SString result_file_name;
			SETIFZQ(new_version, 1);
			SrUedContainer::MakeUedCanonicalName(ps_out.Nam, new_version);
			ps_out.Ext = "dat";
			ps_out.Merge(result_file_name);
			int wsr = uedc.WriteSource(result_file_name, (prev_version > 0) ? &prev_hash : 0, &new_hash);
			THROW(wsr);
			if(wsr > 0) {
				ps_out.Merge(SPathStruc::fDir|SPathStruc::fDrv, temp_buf);
				THROW(uedc.Verify(temp_buf, new_version, 0));
			}
			else {
				new_version = prev_version;
				unchanged = true;
			}
		}
		if(!unchanged || forceUpdatePlDecl) {
			{
				SPathStruc ps_src;
				if(!isempty(pCPath))
					ps_src.Split(pCPath);
				else
					ps_src = ps;
				SrUedContainer::MakeUedCanonicalName(ps_src.Nam, -1);
				ps_src.Ext = "h";
				ps_src.Merge(temp_buf);
				THROW(uedc.GenerateSourceDecl_C(temp_buf, new_version, new_hash));
			}
			{
				SPathStruc ps_src;
				if(!isempty(pJavaPath))
					ps_src.Split(pJavaPath);
				else
					ps_src = ps;
				SrUedContainer::MakeUedCanonicalName(ps_src.Nam, -1);
				ps_src.Ext = "java";
				ps_src.Merge(temp_buf);
				THROW(uedc.GenerateSourceDecl_Java(temp_buf, new_version, new_hash));
			}
		}
	}
	CATCH
		CALLPTRMEMB(pLogger, LogLastError());
		ok = 0;
	ENDCATCH
	return ok;
}
