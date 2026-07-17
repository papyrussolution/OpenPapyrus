// UED.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025, 2026
//
#include <pp.h>
#pragma hdrstop
#if(_MSC_VER >= 1900)
	#include <unicode\uclean.h>
	#include <unicode\brkiter.h>
	#include <unicode\measunit.h>
	#include <unicode\measfmt.h>
	#include <unicode\unumberformatter.h>
#endif
//#include <sartre.h>

IMPL_INVARIANT_C(UedSetBase)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(LimbCount * sizeof(uint64) <= SBaseBuffer::Size, pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

UedSetBase::UedSetBase() : LimbCount(0)
{
	SBaseBuffer::Init();
	SInvariantParam invp;
	assert(InvariantC(&invp));
}
	
UedSetBase::UedSetBase(const UedSetBase & rS) : LimbCount(0)
{
	SBaseBuffer::Init();
	Copy(rS);
	SInvariantParam invp;
	assert(InvariantC(&invp));
}

UedSetBase::~UedSetBase()
{
	SInvariantParam invp;
	assert(InvariantC(&invp));
	SBaseBuffer::Destroy();
	LimbCount = 0;
}

UedSetBase & UedSetBase::Z()
{
	LimbCount = 0;
	return *this;
}

UedSetBase & FASTCALL UedSetBase::operator = (const UedSetBase & rS)
{
	Copy(rS);
	return *this;
}

bool FASTCALL UedSetBase::operator == (const UedSetBase & rS) const { return IsEq(rS); }
	
int FASTCALL UedSetBase::Copy(const UedSetBase & rS)
{
	int    ok = 1;
	if(SBaseBuffer::CopyPrefix(rS, rS.LimbCount * sizeof(uint64))) {
		LimbCount = rS.LimbCount;
		SInvariantParam invp;
		assert(InvariantC(&invp));
	}
	else
		ok = 0;
	return ok;
}

bool FASTCALL UedSetBase::IsEq(const UedSetBase & rS) const
{
	bool    eq = true;
	if(LimbCount != rS.LimbCount)
		eq = false;
	else if(!SBaseBuffer::IsEqPrefix(rS, LimbCount * sizeof(uint64)))
		eq = false;
	return eq;
}

uint64 UedSetBase::Get(uint idx) const
{
	return (idx < LimbCount) ? reinterpret_cast<const uint64 *>(P_Buf)[idx] : 0ULL;
}

bool UedSetBase::Add(const uint64 * pUed, uint count, uint * pIdx)
{
	bool   ok = true;
	const  uint current_end_idx = LimbCount;
	if(pUed && count) {
		if(SBaseBuffer::Put(current_end_idx * sizeof(uint64), pUed, count * sizeof(uint64))) {
			LimbCount += count;
			SInvariantParam invp;
			assert(InvariantC(&invp));
		}
		else
			ok = false;
	}
	ASSIGN_PTR(pIdx, current_end_idx);
	return ok;
}

bool UedSetBase::Add(uint64 ued, uint * pIdx)
{
	return Add(&ued, 1, pIdx);
}

int  SrUedContainer_Base::Helper_RecognizeSymb(SStrScan & rScan, uint flags, SString & rMeta, SString & rSymb) const
{
	rMeta.Z();
	rSymb.Z();
	int    ok = 0;
	SString temp_buf;
	if(rScan.GetIdent(temp_buf)) {
		if(rScan.Is('.')) {
			rScan.Incr();
			rMeta = temp_buf;
			if(rScan.GetIdent(temp_buf)) {
				rSymb = temp_buf;
				ok = 2;
			}
			else
				rMeta.Z();
		}
		else {
			rSymb = temp_buf;
			ok = 1;
		}
	}
	return ok;
}

uint64 SrUedContainer_Base::Recognize(SStrScan & rScan, uint64 implicitMeta, uint flags) const
{
	uint64 result = 0;
	SString temp_buf;
	SString _meta_symb;
	SString _symb;
	uint   preserve_offs = 0;
	rScan.Push(&preserve_offs);
	rScan.Skip();
	int    hr = -1;
	if(flags & rfPrefixSharp && rScan.Is('#')) {
		rScan.Incr();
		hr = Helper_RecognizeSymb(rScan, flags, _meta_symb, _symb);
		/*if(rScan.GetIdent(temp_buf)) {
			if(rScan.Is('.')) {
				rScan.Incr();
				uint64 meta = SearchSymb(temp_buf, UED_META_META);
				if(meta && (!implicitMeta || implicitMeta == meta)) {
					if(rScan.GetIdent(temp_buf)) {
						result = SearchSymb(temp_buf, meta);
					}
				}
			}
			else if(implicitMeta) {
				result = SearchSymb(temp_buf, implicitMeta);
			}
		}*/
	}
	else {
		hr = Helper_RecognizeSymb(rScan, flags, _meta_symb, _symb);
	}
	if(hr > 0) {
		if(flags & rfDraft)
			result = UED_PREDEFVALUE_MAYBE;
		else if(hr == 2) {
			uint64 meta = SearchSymb(_meta_symb, UED_META_META);
			if(meta && (!implicitMeta || implicitMeta == meta)) {
				result = SearchSymb(_symb, meta);
			}
		}
		else if(hr == 1) {
			if(implicitMeta) {
				result = SearchSymb(_symb, implicitMeta);
			}
		}
	}
	else if(rScan.GetXDigits(temp_buf)) {
		uint64 ued = sxtou64(temp_buf);
		if(ued) {
			if(flags & rfDraft)
				result = UED_PREDEFVALUE_MAYBE;
			else {
				if(implicitMeta) {
					if(UED::BelongsToMeta(ued, implicitMeta)) {
						result = ued;
					}
				}
				else {
					uint64 meta = UED::GetMeta(ued);
					if(meta && SearchBaseId(ued, temp_buf)) {
						result = ued;
					}
				}
			}
		}
	}
	if(!result)
		rScan.Pop(preserve_offs);
	return result;
}

/*static*/uint64 UED::SetRaw_Oid(SObjID oid)
{
	uint64 result = 0;
	uint64 meta = 0;
	switch(oid.Obj) {
		case PPOBJ_GOODS: meta = UED_META_PRV_WARE; break;
		case PPOBJ_PERSON: meta = UED_META_PRV_PERSON; break;
		case PPOBJ_LOCATION: meta = UED_META_PRV_LOCATION; break;
		case PPOBJ_BILL: meta = UED_META_PRV_DOC; break;
		case PPOBJ_LOT: meta = UED_META_PRV_LOT; break;
		case PPOBJ_PROCESSOR: meta = UED_META_PRV_PROCESSOR; break; // @v12.6.4
		case PPOBJ_TECH: meta = UED_META_PRV_TECH; break; // @v12.6.4
	}
	if(meta != 0ULL) {
		result = ApplyMetaToRawValue(meta, oid.Id);
	}
	return result;
}

/*static*/bool UED::GetRaw_Oid(uint64 ued, SObjID & rOid)
{
	rOid.Z();
	bool   ok = false;
	uint64 meta = GetMeta(ued);
	switch(meta) {
		case UED_META_PRV_WARE: rOid.Obj = PPOBJ_GOODS; break;
		case UED_META_PRV_PERSON: rOid.Obj = PPOBJ_PERSON; break;
		case UED_META_PRV_LOCATION: rOid.Obj = PPOBJ_LOCATION; break;
		case UED_META_PRV_DOC: rOid.Obj = PPOBJ_BILL; break;
		case UED_META_PRV_LOT: rOid.Obj = PPOBJ_LOT; break;
		case UED_META_PRV_PROCESSOR: rOid.Obj = PPOBJ_PROCESSOR; break; // @v12.6.4
		case UED_META_PRV_TECH: rOid.Obj = PPOBJ_TECH; break; // @v12.6.4
	}
	if(rOid.Obj) {
		uint64 raw_value = 0;
		if(GetRawValue(ued, &raw_value)) {
			if(raw_value <= INT_MAX) {
				rOid.Id = static_cast<int32>(raw_value);
				ok = true;
			}
		}
	}
	return ok;
}
//
//
//
SrUedContainer_Base::UedLocaleEntry::UedLocaleEntry() : Ued(0ULL), Locale(0U)
{
}
		
SrUedContainer_Base::UedLocaleEntry::UedLocaleEntry(uint64 ued, uint locale) : Ued(ued), Locale(locale)
{
}

SrUedContainer_Base::UedLocaleEntry::UedLocaleEntry(const UedLocaleEntry & rS) : Ued(rS.Ued), Locale(rS.Locale)
{
}

SrUedContainer_Base::UedLocaleEntry & FASTCALL SrUedContainer_Base::UedLocaleEntry::operator = (const SrUedContainer_Base::UedLocaleEntry & rS)
{
	Ued = rS.Ued;
	Locale = rS.Locale;
	return *this;
}

bool FASTCALL SrUedContainer_Base::UedLocaleEntry::operator == (const SrUedContainer_Base::UedLocaleEntry & rS) const
{
	return (Ued == rS.Ued && Locale == rS.Locale);
}

SrUedContainer_Base::UedLocaleEntry & SrUedContainer_Base::UedLocaleEntry::Set(uint64 ued, uint locale)
{
	Ued = ued;
	Locale = locale;
	return *this;
}

SrUedContainer_Base::UedLocaleEntry & SrUedContainer_Base::UedLocaleEntry::Z()
{
	Ued = 0ULL;
	Locale = 0U;
	return *this;
}
//
//
//
static IMPL_CMPFUNC(SrUedContainer_TextEntry, p1, p2)
{
	const SrUedContainer_Base::TextEntry * p_e1 = static_cast<const SrUedContainer_Base::TextEntry *>(p1);
	const SrUedContainer_Base::TextEntry * p_e2 = static_cast<const SrUedContainer_Base::TextEntry *>(p2);
	RET_CMPCASCADE2(p_e1, p_e2, Locale, Ued);
}

SrUedContainer_Base::SrUedContainer_Base() : LinguaLocusMeta(0), Ht(1024*32, 0), LastSymbHashId(0), PropIdx(4096, 0)
{
}
	
SrUedContainer_Base::~SrUedContainer_Base()
{
}

uint64 SrUedContainer_Base::SearchBaseIdBySymbId(uint symbId, uint64 meta) const
{
	uint64 id = 0;
	for(uint i = 0; !id && i < BL.getCount(); i++) {
		if(BL.at(i).SymbHashId == symbId) {
			uint64 temp_id = BL.at(i).Id;
			if(UED::BelongsToMeta(temp_id, meta))
				id = temp_id;
		}
	}
	return id;
}

uint64 SrUedContainer_Base::SearchBaseSymb(const char * pSymb, uint64 meta) const
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

uint64 SrUedContainer_Base::SearchSymb(const char * pSymb, uint64 meta) const
{
	uint64 result = 0;
	if(!isempty(pSymb)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = pSymb).Strip().Utf8ToLower();
		result = SearchBaseSymb(r_temp_buf, meta);
	}
	return result;
}

bool SrUedContainer_Base::GetText(uint64 ued, uint64 uedLinguaLocus, SString & rText) const
{
	rText.Z();
	bool   ok = false;
	if(ued && uedLinguaLocus) {
		if(UED::BelongsToMeta(uedLinguaLocus, UED_META_LINGUALOCUS)) {
			const uint32 ll32 = UED::GetRawValue32(uedLinguaLocus);
			UedLocaleEntry key;
			key.Ued = ued;
			key.Locale = ll32;
			auto it = TextHt.find(key);
			uint   text_pos = 0;
			if(it != TextHt.end()) {
				text_pos = it->second;
				if(GetS(text_pos, rText)) {
					ok = true;
				}
			}
		}
	}
	return ok;
}

bool SrUedContainer_Base::SearchBaseId(uint64 id, SString & rSymb) const
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

bool SrUedContainer_Base::SearchSymbHashId(uint32 symbHashId, SString & rSymb) const
{
	return Ht.GetByAssoc(symbHashId, rSymb);
}

int SrUedContainer_Base::GetLocaleIdBySymb(const char * pSymb, uint32 * pLocaleId, uint srcLineNo, PPLogger * pLogger) const
{
	int    ok = 1;
	assert(pLocaleId);
	THROW(pLocaleId); // @todo @err
	{
		SString temp_buf;
		SString locale_buf;
		(locale_buf = pSymb).ToLower();
		uint64 locale_id = SearchBaseSymb(locale_buf, LinguaLocusMeta);
		if(!locale_id) {
			ok = PPSetError(PPERR_UED_SYMBFORMETANOTFOUND, 
				temp_buf.Z().Cat("line").CatDiv(':', 2).Cat(srcLineNo).Space().
				Cat(locale_buf).Space().Cat("->").Space().CatHex(LinguaLocusMeta));
			if(pLogger)
				pLogger->LogLastError();
			else
				CALLEXCEPT();
		}
		else {
			const bool btm_result = UED::BelongsToMeta(locale_id, LinguaLocusMeta);
			if(!btm_result) {
				ok = PPSetError(PPERR_UED_VALUENOTBELONGTOMETA, 
					temp_buf.Z().Cat("line").CatDiv(':', 2).Cat(srcLineNo).Space().
					CatHex(locale_id).Space().Cat("->").Space().CatHex(LinguaLocusMeta));
				if(pLogger)
					pLogger->LogLastError();
				else
					CALLEXCEPT();
			}
			{
				//r_e.Locale = UED::MakeShort(locale_id, LinguaLocusMeta);
				assert(UED::GetMeta(locale_id) == LinguaLocusMeta);
				bool grvr = UED::GetRawValue32(locale_id, pLocaleId);
				assert(grvr);
				assert(*pLocaleId);
				ok = 1;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::ReplaceSurrogateLocaleId(const SymbHashTable & rT, uint32 surrLocaleId, uint32 * pRealLocaleId, uint lineNo, PPLogger * pLogger)
{
	int    ok = 1;
	assert(pRealLocaleId);
	THROW(pRealLocaleId); // @todo @err
	if(surrLocaleId) {
		SString locale_buf;
		THROW(rT.GetByAssoc(surrLocaleId, locale_buf));
		THROW(GetLocaleIdBySymb(locale_buf, pRealLocaleId, lineNo, pLogger));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::ReplaceSurrogateLocaleIds(const SymbHashTable & rT, PPLogger * pLogger)
{
	int    ok = 1;
	THROW(LinguaLocusMeta);
	{
		for(uint teidx = 0; teidx < TL.getCount(); teidx++) {
			TextEntry & r_e = TL.at(teidx);
			uint32 real_locale_id = 0;
			const int rslr = ReplaceSurrogateLocaleId(rT, r_e.Locale, &real_locale_id, r_e.LineNo, pLogger);
			if(rslr > 0) {
				assert(real_locale_id);
				r_e.Locale = real_locale_id;
			}
			else if(rslr < 0) {
				assert(real_locale_id == 0);
				r_e.Locale = real_locale_id;
			}
			else {
				ok = 0;
				if(!pLogger) {
					CALLEXCEPT();
				}
			}
		}
	}
	{
		for(uint ppidx = 0; ppidx < ProtoPropList.getCount(); ppidx++) {
			ProtoProp * p_pp = ProtoPropList.at(ppidx);
			assert(p_pp);
			if(p_pp) {
				uint32 real_locale_id = 0;
				const int rslr = ReplaceSurrogateLocaleId(rT, p_pp->Locale, &real_locale_id, p_pp->LineNo, pLogger);
				if(rslr > 0) {
					assert(real_locale_id);
					p_pp->Locale = real_locale_id;
				}
				else if(rslr < 0) {
					assert(real_locale_id == 0);
					p_pp->Locale = real_locale_id;
				}
				else {
					ok = 0;
					if(!pLogger) {
						CALLEXCEPT();
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

SrUedContainer_Base::PropertySet::PropertySet() : UedSetBase()
{
}

uint SrUedContainer_Base::PropertySet::GetCount() const { return PosIdx.getCount(); }
		
int SrUedContainer_Base::PropertySet::Add(const uint64 * pPropChunk, uint count, uint * pPos)
{
	int    ok = -1;
	if(pPropChunk && count) {
		uint   pos = 0;
		ok = UedSetBase::Add(pPropChunk, count, &pos);
		if(ok) {
			PosIdx.Add(static_cast<long>(pos), static_cast<long>(count));
			ASSIGN_PTR(pPos, pos);
		}
	}
	return ok;
}

int SrUedContainer_Base::PropertySet::Get(uint idx, uint count, UedSetBase & rList) const
{
	rList.Z();
	int    ok = 0;
	if(idx < GetLimbCount() && (idx+count) <= GetLimbCount()) {
		uint idx_pos = 0;
		if(PosIdx.Search(idx, &idx_pos) && PosIdx.at(idx_pos).Val == (long)count) {
			for(uint i = idx; i < idx+count; i++) {
				uint64 u = UedSetBase::Get(i);
				rList.Add(u, 0);
			}
			ok = 1;
		}
	}	
	return ok;
}

SrUedContainer_Base::ProtoPropList_SingleUed::ProtoPropList_SingleUed(uint64 ued, uint localeId) : 
	UedLocaleEntry(ued, localeId), TSCollection <ProtoProp>()
{
}
		
void SrUedContainer_Base::ProtoPropList_SingleUed::Init(uint64 ued, uint localeId)
{
	Ued = ued;
	Locale = localeId;
	freeAll();
}

SrUedContainer_Base::ProtoProp * SrUedContainer_Base::ProtoPropList_SingleUed::Create()
{
	ProtoProp * p_new_entry = CreateNewItem();
	if(p_new_entry) {
		p_new_entry->Ued = Ued;
		p_new_entry->Locale = Locale;
	}
	return p_new_entry;
}

SrUedContainer_Base::PropertyListParsingBlock::PropertyListParsingBlock() : Status(0), State(0), PL(0, 0)
{
}
	
bool SrUedContainer_Base::PropertyListParsingBlock::Start(uint64 ued, int localeId)
{
	bool ok = true;
	if(Status)
		ok = false;
	else if(ued != 0) {
		Status = 1;
		State = 0;
		PL.Init(ued, localeId);
	}
	else
		ok = false;
	return ok;
}

enum { 
	stUndef = 0,
	stLeftBrace,
	stPropIdent,
	stColonAfterPropIdent,
	stPropValue,
	stEndOfProp, // ';'
	stEndOfPropList // '}'
};

int SrUedContainer_Base::PropertyListParsingBlock::ScanProp(SrUedContainer_Base & rC, SStrScan & rScan)
{
	int    ok = 0;
	SString temp_buf;
	if(rScan.GetIdent(temp_buf)) {
		ProtoProp * p_new_prop = PL.Create();
		THROW(p_new_prop);
		p_new_prop->Prop.add(temp_buf);
		State = stPropIdent;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::PropertyListParsingBlock::ScanArg(SrUedContainer_Base & rC, SStrScan & rScan, bool isFirst/*@debug*/)
{
	int    ok = 0;
	SString temp_buf;
	uint64 rued = rC.Recognize(rScan, 0, rfDraft);
	if(rued == UED_PREDEFVALUE_MAYBE) {
	//if(rScan.GetIdent(temp_buf)) {
		State = stPropValue;
		THROW(PL.getCount());
		{
			ProtoProp * p_prop = PL.at(PL.getCount()-1);
			THROW(p_prop);
			if(isFirst) {
				assert(p_prop->Prop.getCount() == 1);
			}
			else {
				assert(p_prop->Prop.getCount() > 1);
			}
			p_prop->Prop.add(temp_buf);
		}
		ok = 1;
	}
	else if(rScan.GetNumber(temp_buf)) {
		State = stPropValue;
		THROW(PL.getCount());
		{
			ProtoProp * p_prop = PL.at(PL.getCount()-1);
			THROW(p_prop);
			if(isFirst) {
				assert(p_prop->Prop.getCount() == 1);
			}
			else {
				assert(p_prop->Prop.getCount() > 1);
			}
			p_prop->Prop.add(temp_buf);
		}
		ok = 1;
	}
	else if(rScan.GetQuotedString(SFileFormat::Json, temp_buf)) {
		State = stPropValue;
		THROW(PL.getCount());
		{
			ProtoProp * p_prop = PL.at(PL.getCount()-1);
			THROW(p_prop);
			if(isFirst) {
				assert(p_prop->Prop.getCount() == 1);
			}
			else {
				assert(p_prop->Prop.getCount() > 1);
			}
			p_prop->Prop.add(temp_buf.Quot('\"', '\"'));
		}
		ok = 1;
	}
	/*
		@todo Здесь еще нужно распознавать дату/время, но это сделаю позже
	*/
	CATCHZOK
	return ok;
}
//
// Returns:
//   >0 - разбор завершен (встретился завершающий символ '}')
//   <0 - разбор не завершен
//    0 - error
//
int SrUedContainer_Base::PropertyListParsingBlock::Do(SrUedContainer_Base & rC, SStrScan & rScan)
{
	int    ok = -1;
	SString temp_buf;
	if(Status == 1) {
		THROW(rScan.Is('{'));
		State = stLeftBrace;
		Status = 2;
		rScan.Incr();
	}
	THROW(Status == 2);
	while(Status == 2 && !rScan.Skip().IsEnd()) {
		switch(State) {
			case stLeftBrace:
				THROW(ScanProp(rC, rScan));
				break;
			case stEndOfProp:
				if(rScan.Is('}')) {
					rScan.Incr();
					State = stEndOfPropList;
					Status = 0;
					ok = 1;
				}
				else {
					THROW(ScanProp(rC, rScan));
				}
				break;
			case stPropIdent:
				if(rScan.Is(':')) {
					rScan.Incr();
					State = stColonAfterPropIdent;
				}
				else if(rScan.Is(';')) {
					rScan.Incr();
					State = stEndOfProp;
				}
				else if(ScanArg(rC, rScan, true)) {
					;
				}
				else {
					CALLEXCEPT();
				}
				break;
			case stColonAfterPropIdent:
				if(ScanArg(rC, rScan, true)) {
					State = stPropValue;
				}
				else {
					CALLEXCEPT();
				}
				break;
			case stPropValue:
				if(rScan.Is(';')) {
					rScan.Incr();
					State = stEndOfProp;
				}
				else if(ScanArg(rC, rScan, false)) {
					;
				}
				else {
					CALLEXCEPT();
				}
				break;
			default:
				ok = 0;
		}
	}
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::RegisterProtoPropList(const ProtoPropList_SingleUed & rList)
{
	int    ok = 1;
	for(uint i = 0; i < rList.getCount(); i++) {
		const ProtoProp * p_item = rList.at(i);
		if(p_item && p_item->Ued) {
			ProtoProp * p_dup = new ProtoProp(*p_item);
			if(p_dup)
				ProtoPropList.insert(p_dup);
		}
	}
	return ok;
}

int SrUedContainer_Base::ReadSource(const char * pFileName, uint flags, const StringSet * pSsLang, PPLogger * pLogger)
{
	int    ok = 1;
	bool   debug_mark = false; // @v12.6.6 @debug
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
	UedLocaleEntry last_key;
	PropertyListParsingBlock plp_blk;
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
			size_t _temp_pos = 0;
			if(line_buf.Search("//", 0, 0, &comment_pos)) {
				line_buf.Trim(comment_pos).Strip();
			}
			// @v12.6.6 {
			if(line_buf.SearchChar('\x13', &_temp_pos)) {
				temp_buf.Z().Cat(pFileName).CatChar('(').Cat(line_no).Comma().Cat(_temp_pos).CatChar(')');
				PPSetError(PPERR_UED_INVCHARINSRCLINE, temp_buf);
				if(pLogger) {
					pLogger->LogLastError();
				}
				else {
					CALLEXCEPT();
				}
			} // } @v12.6.6 
			else if(line_buf.NotEmpty()) {
				//line_buf.Tokenize(" \t", ss.Z());
				ss.Z();
				scan.Set(line_buf, 0);
				if(plp_blk.IsStarted()) {
					// Если строка включает описание свойств, то новая сущность на этой строке
					// начинаться не может, потому и else после этого блока.
					int plpr = plp_blk.Do(*this, scan);
					if(plpr > 0) {
						RegisterProtoPropList(plp_blk.GetResult());
					}
				}
				else {
					bool   curly_bracket_left = false;
					bool   scan_ok = true;
					if(scan.GetXDigits(temp_buf.Z())) {
						ss.add(temp_buf);
						scan.Skip();
						if(scan.GetQuotedString(temp_buf) || scan.GetIdent(temp_buf)) {
							ss.add(temp_buf);
							scan.Skip();
							if(scan.Is('{')) {
								curly_bracket_left = true;
							}
							else if(!scan.IsEnd()) {
								if(scan.GetQuotedString(temp_buf)) {
									ss.add(temp_buf);
									scan.Skip();
									if(scan.Is('{')) {
										curly_bracket_left = true;
									}
								}
								else
									scan_ok = false;
							}
						}
						else
							scan_ok = false;
					}
					else if(scan.Is('{'))
						curly_bracket_left = true;
					else
						scan_ok = false;
					if(scan_ok) {
						const uint ssc = ss.getCount();
						if(oneof2(ssc, 2, 3)) {
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
										// @v12.6.6 @debug {
										/*if(text_buf.IsEqiAscii("argon2id")) {
											debug_mark = true;
										}*/
										// } @v12.6.6 @debug 
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
										if(!LinguaLocusMeta) {
											LinguaLocusMeta = id;
	#ifdef UED_META_LINGUALOCUS
											assert(LinguaLocusMeta == UED_META_LINGUALOCUS);
	#endif
										}
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
									last_key.Set(new_entry.Id, 0);
								}
								else if(ssc == 3) {
									if(lang_id) {
										const bool skip_because_lang = (pSsLang && !pSsLang->searchNcAscii(lang_buf, 0));
										if(!skip_because_lang) {
											TextEntry new_entry;
											new_entry.LineNo = line_no; // @v11.7.8
											new_entry.Ued = id;
											new_entry.Locale = lang_id;
											AddS(text_buf, &new_entry.TextP);
											TL.insert(&new_entry);
											last_key.Set(new_entry.Ued, new_entry.Locale);
										}
									}
								}
							}
						}
						else if(!curly_bracket_left) {
							// @todo @err
						}
						if(curly_bracket_left) {
							if(plp_blk.Start(last_key.Ued, last_key.Locale)) {
								int plpr = plp_blk.Do(*this, scan);
								if(plpr > 0) {
									RegisterProtoPropList(plp_blk.GetResult());
								}
							}
							else {
								; // @todo @err
							}
						}
					}
					else {
						// invalid line
						last_key.Set(0ULL, 0);
					}
				}
			}
		}
	}
	Ht.BuildAssoc();
	THROW_SL(temporary_linglocus_tab.BuildAssoc());
	THROW(ReplaceSurrogateLocaleIds(temporary_linglocus_tab, pLogger));
	// @v12.3.9 {
	if(!(flags & rsfCompileTime)) {
		for(uint i = 0; i < TL.getCount(); i++) {
			const TextEntry & r_entry = TL.at(i);
			TextHt.emplace(r_entry, r_entry.TextP); 
		}
	}
	// } @v12.3.9 
	if(flags & rsfCompileTime) {
		THROW(ProcessProperties());
	}
	else {
		// THROW(ReadProps(0));
	}
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::GetPropList(TSCollection <PropIdxEntry> & rList) const
{
	rList.freeAll();
	int    ok = -1;
	PropIdxEntry * p_entry = 0;
	for(uint i = 0; PropIdx.Enum(&i, &p_entry);) {
		assert(p_entry);
		if(p_entry) {
			PropIdxEntry * p_new_entry = rList.CreateNewItem();
			if(p_new_entry) {
				*p_new_entry = *p_entry;
				ok = 1;
			}
		}
	}
	return ok;
}

int SrUedContainer_Base::Helper_PutProperties(const UedLocaleEntry & rUedEntry, const Uint64Array & rRawPropList)
{
	int    ok = 1;
	if(rUedEntry.Ued != 0ULL && rRawPropList.getCount()) {
		uint prop_idx = 0;
		const uint prop_limb_count = rRawPropList.getCount();
		PropS.Add(static_cast<uint64 *>(rRawPropList.dataPtr()), prop_limb_count, &prop_idx);
		{
			PropIdxEntry key;
			key.Ued = rUedEntry.Ued;
			key.Locale = rUedEntry.Locale;
			uint ks = 0;
			const void * p_key = key.GetHashKey(0, &ks);
			PropIdxEntry * p_idx_entry = PropIdx.Get(p_key, ks);
			if(p_idx_entry) {
				p_idx_entry->RefList.Add(prop_idx, prop_limb_count);
			}
			else {
				PropIdxEntry * p_new_idx_entry = new PropIdxEntry;
				p_new_idx_entry->Ued = rUedEntry.Ued;
				p_new_idx_entry->Locale = rUedEntry.Locale;
				p_new_idx_entry->RefList.Add(prop_idx, prop_limb_count);
				PropIdx.Put(p_new_idx_entry, true);
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int SrUedContainer_Base::ProcessProperties()
{
	int    ok = 1;
	SString temp_buf;
	SString prop_item_text;
	SStrScan scan;
	Uint64Array raw_prop_list;
	for(uint i = 0; i < ProtoPropList.getCount(); i++) {
		const ProtoProp * p_pp = ProtoPropList.at(i);
		if(p_pp) {
			//UedSetBase line;
			uint   ssidx = 0;
			raw_prop_list.clear();
			bool local_fault = false;
			for(uint ssp = 0; !local_fault && p_pp->Prop.get(&ssp, prop_item_text); ssidx++) {
				scan.Set(prop_item_text, 0);
				scan.Skip();
				if(ssidx == 0) { // property
					uint64 ued_prop = Recognize(scan, UED_META_PROP, 0);
					if(ued_prop && ued_prop != UED_PREDEFVALUE_MAYBE)
						raw_prop_list.insert(&ued_prop);
					else
						local_fault = true;
				}
				else { // args
					uint64 ued = 0;
					if(scan.GetNumber(temp_buf)) {
						double v = temp_buf.ToReal_Plain();
						if(ffrac(v) == 0.0) {
							// int
						}
						else {
							// real
							SDecimal dcml(temp_buf);
							ued = dcml.ToUed_(UED_META_DECIMAL);
							if(ued) {
								raw_prop_list.insert(&ued);
							}
							else
								local_fault = true;
						}
					}
					else if(scan.GetQuotedString(SFileFormat::Json, temp_buf)) {
						uint   sp = 0;
						AddS(temp_buf, &sp);
						ued = UED::ApplyMetaToRawValue(UED_META_STRINGREF, sp);
						if(ued) {
							raw_prop_list.insert(&ued);
						}
						else
							local_fault = true;
					}
					else {
						ued = Recognize(scan, 0, 0);
						if(ued && ued != UED_PREDEFVALUE_MAYBE) {
							raw_prop_list.insert(&ued);
						}
						else
							local_fault = true;
					}
				}
			}
			if(!local_fault) {
				Helper_PutProperties(*p_pp, raw_prop_list);
			}
		}
	}
	return ok;
}

/*static*/void SrUedContainer_Base::MakeUedCanonicalName(int canonfn, SString & rResult, long ver)
{
	rResult.Z();
	assert(oneof2(canonfn, canonfnIds, canonfnProps));
	if(oneof2(canonfn, canonfnIds, canonfnProps)) {
		if(canonfn == canonfnIds)
			rResult.Cat("ued-id");
		else if(canonfn == canonfnProps)
			rResult.Cat("ued-prop");
		if(ver > 0)
			rResult.CatChar('-').CatLongZ(ver, 4);
		else if(ver == 0)
			rResult.CatChar('-').Cat("????");
		else { // ver < 0 - file-name for programming interface source
			//
		}
	}
}

/*static*/long SrUedContainer_Base::SearchLastCanonicalFile(int canonfn, const char * pPath, SString & rFileName)
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
	SFsPath ps;
	MakeUedCanonicalName(canonfn, fn_wc, 0);
	(temp_buf = pPath).Strip().SetLastSlash().Cat(fn_wc).DotCat("dat");
	for(SDirec sd(temp_buf, 0); sd.Next(&de) > 0;) {
		if(de.IsFile()) {
			ss.Z();
			de.GetNameA(temp_buf);
			ps.Split(temp_buf);
			ps.Nam.Tokenize("-", ss);
			if(ss.getCount() == 3 && ss.getByIdx(2, temp_buf)) { // ued;(id|prop);version ("ued-id-0004")
				long iter_ver = temp_buf.ToLong();
				if(iter_ver > 0 && iter_ver > max_ver) {
					//
					// Необходимо убедиться что рядом с основным файлом присутствует файл с контрольной суммой (ext .sha256)
					//
					de.GetNameA(pPath, temp_buf);
					ps.Split(temp_buf);
					SlHash::GetAlgorithmSymb(SHASHF_SHA256, ps.Ext);
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

bool SrUedContainer_Base::ReadSingleProp(SStrScan & rScan)
{
	bool   ok = true;
	UedLocaleEntry ued_entry;
	Uint64Array raw_prop_list;
	SString temp_buf;
	SString lingua_buf;
	rScan.Skip();
	THROW(rScan.GetXDigits(temp_buf));
	ued_entry.Ued = temp_buf.HexToUInt64();
	rScan.Skip();
	if(rScan.GetIdent(temp_buf)) {
		lingua_buf = temp_buf;
		rScan.Skip();
		THROW(GetLocaleIdBySymb(lingua_buf, &ued_entry.Locale, 0/*srcLineNo*/, 0/*pLogger*/));
	}
	THROW(rScan.IncrChr(':'));
	rScan.Skip();
	while(!rScan.IsEnd()) {
		if(rScan.GetQuotedString(temp_buf)) {
			uint   sp = 0;
			AddS(temp_buf, &sp);
			uint64 ued = UED::ApplyMetaToRawValue(UED_META_STRINGREF, sp);
			if(ued) {
				raw_prop_list.insert(&ued);
			}
			else {
				ok = false;
			}
		}
		else if(rScan.GetXDigits(temp_buf)) {
			uint64 ued = temp_buf.HexToUInt64();
			raw_prop_list.insert(&ued);
		}
		else {
			ok = false;
			break;
		}
		rScan.Skip();
	}
	if(ok) {
		Helper_PutProperties(ued_entry, raw_prop_list);
	}
	CATCHZOK
	return ok;
}

int  SrUedContainer_Base::ReadProps(const char * pFileName)
{
	int    ok = 1;
	SString line_buf;
	SStrScan scan;
	SFile  f_in(pFileName, SFile::mRead);
	THROW(f_in.IsValid());
	while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
		size_t comment_pos = 0;
		if(line_buf.Search("//", 0, 0, &comment_pos)) {
			line_buf.Trim(comment_pos).Strip();
		}
		if(line_buf.NotEmpty()) {
			scan.Set(line_buf, 0U);
			if(ReadSingleProp(scan)) {
				;
			}
			else {
				// @todo @err
				CALLEXCEPT();
			}
		}
	}
	CATCHZOK
	return ok;
}

int SrUedContainer_Base::WriteProps(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash)
{
	int    ok = 1;
	if(PropS.GetCount()) {
		TSCollection <PropIdxEntry> prop_list;
		if(GetPropList(prop_list) > 0) {
			SString line_buf;
			SString comment_buf;
			SString temp_buf;
			THROW_PP_S(!isempty(pFileName), PPERR_INVPARAM_EXT, __FUNCTION__"/pFileName");
			THROW(LinguaLocusMeta); // @todo @err
			{
				UedSetBase local_prop_set;
				SFile f_out(pFileName, SFile::mWrite|SFile::mBinary);
				THROW_SL(f_out.IsValid());
				for(uint i = 0; i < prop_list.getCount(); i++) {
					const PropIdxEntry * p_entry = prop_list.at(i);
					assert(p_entry);
					if(p_entry) {
						for(uint refidx = 0; refidx < p_entry->RefList.getCount(); refidx++) {
							const LAssoc & r_ref = p_entry->RefList.at(refidx);
							if(PropS.Get(r_ref.Key, r_ref.Val, local_prop_set)) {
								line_buf.Z();
								comment_buf.Z();
								line_buf.CatHex(p_entry->Ued);
								if(SearchBaseId(p_entry->Ued, temp_buf)) {
									comment_buf.Cat(temp_buf);
								}
								else {
									comment_buf.CatChar('%').CatHex(p_entry->Ued); // @todo @err
								}
								if(p_entry->Locale) {
									uint64 ued_locus = UED::ApplyMetaToRawValue32(LinguaLocusMeta, p_entry->Locale);
									if(SearchBaseId(ued_locus, temp_buf)) {
										line_buf.Space().Cat(temp_buf);
										comment_buf.Space().Cat(temp_buf);
									}
									else {
										line_buf.Space().CatChar('%').CatHex(ued_locus); // @todo @err
										comment_buf.Space().CatChar('%').CatHex(ued_locus); // @todo @err
									}
								}
								line_buf.Colon();
								comment_buf.Colon();
								//
								for(uint j = 0; j < local_prop_set.GetLimbCount(); j++) {
									uint64 ued_prop = local_prop_set.Get(j);
									if(SearchBaseId(ued_prop, temp_buf)) {
										line_buf.Space().CatHex(ued_prop);
										comment_buf.Space().Cat(temp_buf);
									}
									else if(UED::BelongsToMeta(ued_prop, UED_META_STRINGREF)) {
										uint64 raw_value = 0;
										if(UED::GetRawValue(ued_prop, &raw_value)) {
											GetS((uint)raw_value, temp_buf);
											line_buf.Space().CatQStr(temp_buf);
											comment_buf.Space().CatQStr(temp_buf);
										}
									}
									else if(UED::BelongsToMeta(ued_prop, UED_META_DECIMAL)) {
										uint bits = UED::GetRawDataBits(ued_prop);
										SDecimal dcml;
										dcml.FromUed(ued_prop, bits);
										dcml.ToStr(0, temp_buf);
										line_buf.Space().CatHex(ued_prop);
										comment_buf.Space().Cat(temp_buf);
									}
									else {
										line_buf.Space().CatHex(ued_prop);
										comment_buf.Space().CatChar('%').CatHex(ued_prop); // @todo @err
									}
								}
							}
							temp_buf.Z().Cat(line_buf).Space().Cat("//").Space().Cat(comment_buf).CR();
							f_out.WriteLine(temp_buf);
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
	
int SrUedContainer_Base::WriteSource(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash)
{
	int    ok = 1;
	SString line_buf;
	SString temp_buf;
	THROW_PP_S(!isempty(pFileName), PPERR_INVPARAM_EXT, __FUNCTION__"/pFileName");
	THROW(LinguaLocusMeta); // @todo @err
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
				line_buf.Z().CatHex(r_e.Ued).Space();
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
			SFsPath ps(pFileName);
			SlHash::GetAlgorithmSymb(SHASHF_SHA256, ps.Ext);
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

bool SrUedContainer_Ct::GenerateSourceDecl_C(const char * pFileName, const char * pMultiInclProtectDef, uint versionN, const SBinaryChunk & rHash)
{
	bool   ok = true;
	SString temp_buf;
	SString def_symb;
	SString def_value;
	SString meta_symb;
	SString h_sentinel_def;
	Generator_CPP gen(pFileName);
	const  SFsPath ps(pFileName);
	uint   max_symb_len = 0;
	{
		ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		gen.Wr_Comment(temp_buf.ToUpper());
		temp_buf.Z().Cat("version").CatDiv(':', 2).Cat(versionN);
		gen.Wr_Comment(temp_buf);
		SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
		temp_buf.CatDiv(':', 2).CatHex(rHash.PtrC(), rHash.Len());
		gen.Wr_Comment(temp_buf);
		gen.Wr_Comment(temp_buf.Z());
	}
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				SearchSymbHashId(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				SETMAX(max_symb_len, def_symb.Len32());
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongsToMeta(r_be_inner.Id, r_be.Id)) {
							SearchSymbHashId(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							SETMAX(max_symb_len, def_symb.Len32());
						}
					}
					gen.IndentDec();
				}
			}
		}
	}
	if(isempty(pMultiInclProtectDef)) {
		h_sentinel_def.Z().Cat("__").Cat(ps.Nam).CatChar('_').Cat("h").ToUpper();
		h_sentinel_def.ReplaceChar('-', '_');
	}
	else {
		h_sentinel_def = pMultiInclProtectDef;
	}
	gen.Wr_IfDef(h_sentinel_def, 1);
	gen.Wr_Define(h_sentinel_def, 0);
	gen.WriteBlancLine();
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				SearchSymbHashId(r_be.SymbHashId, meta_symb);
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
						const  BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongsToMeta(r_be_inner.Id, r_be.Id)) {
							SearchSymbHashId(r_be_inner.SymbHashId, temp_buf);
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

bool SrUedContainer_Ct::GenerateSourceDecl_Java(const char * pFileName, uint versionN, const SBinaryChunk & rHash)
{
	bool   ok = true;
	SString temp_buf;
	SString def_symb;
	SString def_value;
	SString meta_symb;
	SString h_sentinel_def;
	SFile  genf(pFileName, SFile::mWrite);
	const  SFsPath ps(pFileName);
	uint   max_symb_len = 0;
	{
		ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf.ToUpper()).CR());
		temp_buf.Z().Cat("version").CatDiv(':', 2).Cat(versionN);
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf).CR());
		SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
		temp_buf.CatDiv(':', 2).CatHex(rHash.PtrC(), rHash.Len());
		genf.WriteLine(meta_symb.Z().Cat("//").Space().Cat(temp_buf).CR());
		genf.WriteLine(meta_symb.Z().Cat("//").CR());
	}
	{
		for(uint i = 0; i < BL.getCount(); i++) {
			const BaseEntry & r_be = BL.at(i);
			if(UED::IsMetaId(r_be.Id)) {
				SearchSymbHashId(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				SETMAX(max_symb_len, def_symb.Len32());
				if(r_be.Id != 0x0000000100000001ULL) { // super-meta wich identifies other meta's
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongsToMeta(r_be_inner.Id, r_be.Id)) {
							SearchSymbHashId(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							SETMAX(max_symb_len, def_symb.Len32());
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
				SearchSymbHashId(r_be.SymbHashId, meta_symb);
				assert(meta_symb.NotEmpty());
				meta_symb.ToUpper();
				(def_symb = "UED").CatChar('_').Cat("META").CatChar('_').Cat(meta_symb);
				assert(max_symb_len >= def_symb.Len());
				def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
				def_value.Z().Cat("0x").CatHex(r_be.Id).Cat("L");
				temp_buf.Z().Tab().Cat("public static final long").Space().Cat(def_symb).Eq().Space().Cat(def_value).Semicol().CR();
				genf.WriteLine(temp_buf);
				if(r_be.Id != /*0x0000000100000001ULL*/UED_META_META) { // super-meta wich identifies other meta's
					//gen.IndentInc();
					for(uint j = 0; j < BL.getCount(); j++) {
						const BaseEntry & r_be_inner = BL.at(j);
						if(UED::BelongsToMeta(r_be_inner.Id, r_be.Id)) {
							SearchSymbHashId(r_be_inner.SymbHashId, temp_buf);
							assert(temp_buf.NotEmpty());
							temp_buf.ToUpper();
							(def_symb = "UED").CatChar('_').Cat(meta_symb).CatChar('_').Cat(temp_buf);
							assert(max_symb_len >= def_symb.Len());
							def_symb.CatCharN(' ', (max_symb_len+1)-def_symb.Len());
							def_value.Z().Cat("0x").CatHex(r_be_inner.Id).Cat("L");
							temp_buf.Z().Tab_(2).Cat("public static final long").Space().Cat(def_symb).Eq().Space().Cat(def_value).Semicol().CR();
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

int SrUedContainer_Ct::VerifyByPreviousVersion(const SrUedContainer_Ct * pPrevC, bool tolerant, PPLogger * pLogger)
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
			pPrevC->SearchSymbHashId(r_be.SymbHashId, prev_symb);
			if(SearchBaseId(r_be.Id, this_symb)) {
				if(prev_symb != this_symb) {
					temp_buf.Z().Cat(prev_symb).Cat("-->").Cat(this_symb);
					if(!tolerant)
						ok = 0;
					PPSetError(PPERR_UED_PREVVERV_PREVSYMBMODIFIED, temp_buf);
					if(pLogger)
						pLogger->LogLastError();
				}
			}
			else if(prev_symb.NotEmpty()) {
				//PPERR_UED_PREVVERV_SYMBREMOVED      "Символ, существовавший в предудущей версии, удален в новой версии (%s)"
				if(!tolerant)
					ok = 0;
				PPSetError(PPERR_UED_PREVVERV_SYMBREMOVED, prev_symb);
				if(pLogger)
					pLogger->LogLastError();
			}
		}
	}
	return ok;
}
	
int SrUedContainer_Base::Verify(const char * pPath, long ver, SBinaryChunk * pHash) const
{
	int    ok = 1;
	SString temp_buf;
	THROW(ver > 0);
	MakeUedCanonicalName(canonfnIds, temp_buf, ver);
	{
		SString file_path;
		SString hash_file_path;
		binary256 hash;
		(file_path = pPath).SetLastSlash().Cat(temp_buf).DotCat("dat");
		THROW_SL(fileExists(file_path));
		(hash_file_path = pPath).SetLastSlash().Cat(temp_buf);
		SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
		hash_file_path.Dot().Cat(temp_buf);
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

int ProcessUed(const char * pSrcFileName, const char * pOutPath, const char * pRtOutPath, const char * pCPath, const char * pJavaPath, uint flags, PPLogger * pLogger)
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
	SrUedContainer_Ct uedc;
	SrUedContainer_Ct uedc_prev;
	SString last_file_name;
	SString path;
	SString out_path;
	SString c_path;
	SString java_path;
	bool   dont_log_last_err = false;
	long   new_version = 0;
	THROW_PP_S(!isempty(pSrcFileName), PPERR_INVPARAM_EXT, __FUNCTION__"/pSrcFileName");
	{
		SFsPath ps(pSrcFileName);
		SFsPath ps_out;
		SFsPath ps_rt_out;
		ps.Merge(SFsPath::fDrv|SFsPath::fDir, path);
		path.RmvLastSlash();
		if(!isempty(pOutPath)) {
			ps_out.Split(pOutPath);
			ps_out.Merge(SFsPath::fDrv|SFsPath::fDir, out_path);
			out_path.RmvLastSlash();
		}
		else {
			ps_out = ps;
			out_path = path;
		}
		if(!isempty(pRtOutPath)) {
			ps_rt_out.Split(pRtOutPath);
		}
		else {
			ps_rt_out = ps;
		}
		const long prev_version = SrUedContainer_Ct::SearchLastCanonicalFile(SrUedContainer_Base::canonfnIds, out_path, last_file_name);
		if(prev_version > 0) {
			THROW(uedc_prev.Read(last_file_name, pLogger));
			THROW(uedc.Verify(out_path, prev_version, &prev_hash));
			new_version = prev_version+1;
		}
		THROW(uedc.Read(pSrcFileName, pLogger));
		if(prev_version > 0) {
			int vr = uedc.VerifyByPreviousVersion(&uedc_prev, LOGIC(flags & prcssuedfTolerant), pLogger);
			if(!vr) {
				dont_log_last_err = true;
				CALLEXCEPT();
			}
		}
		{
			SString result_file_name;
			SString props_file_name;
			SETIFZQ(new_version, 1);
			{
				SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnIds, ps_out.Nam, new_version);
				ps_out.Ext = "dat";
				ps_out.Merge(result_file_name);
				slfprintf_stderr((temp_buf = result_file_name).CR());
				int wsr = uedc.Write(result_file_name, (prev_version > 0) ? &prev_hash : 0, &new_hash);
				THROW(wsr);
				if(wsr > 0) {
					ps_out.Merge(SFsPath::fDir|SFsPath::fDrv, temp_buf);
					THROW(uedc.Verify(temp_buf, new_version, 0));
				}
				else {
					SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnIds, ps_out.Nam, prev_version);
					ps_out.Ext = "dat";
					ps_out.Merge(result_file_name);
					new_version = prev_version;
					unchanged = true;
				}
				{
					// Записываем файл без номера версии (будет использоваться в run-time'е)
					SString result_file_name_wo_sfx;
					SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnIds, ps_rt_out.Nam, -1);
					ps_rt_out.Ext = "dat";
					ps_rt_out.Merge(temp_buf);
					SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, result_file_name_wo_sfx);
					if(!unchanged || SFile::Compare(result_file_name_wo_sfx, result_file_name, 0) <= 0) {
						slfprintf_stderr(temp_buf.Z().Cat(result_file_name).Cat(" -> ").Cat(result_file_name_wo_sfx).CR());
						THROW_SL(copyFileByName(result_file_name, result_file_name_wo_sfx));
						//
						SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
						SFsPath::ReplaceExt(result_file_name, temp_buf, 1);
						SFsPath::ReplaceExt(result_file_name_wo_sfx, temp_buf, 1);
						slfprintf_stderr(temp_buf.Z().Cat(result_file_name).Cat(" -> ").Cat(result_file_name_wo_sfx).CR());
						THROW_SL(copyFileByName(result_file_name, result_file_name_wo_sfx));
					}
				}
			}
			{
				SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnProps, ps_out.Nam, new_version);
				ps_out.Ext = "dat";
				ps_out.Merge(props_file_name);
				slfprintf_stderr((temp_buf = props_file_name).CR());
				int wsr = uedc.WriteProps(props_file_name, (prev_version > 0) ? &prev_hash : 0, &new_hash);
				THROW(wsr);
			}
		}
		if(!unchanged || (flags & prcssuedfForceUpdatePlDecl)) {
			SString out_file_name;
			SString temp_file_name;
			{
				SFsPath ps_src;
				if(!isempty(pCPath))
					ps_src.Split(pCPath);
				else
					ps_src = ps;
				SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnIds, ps_src.Nam, -1);
				ps_src.Ext = "h";
				ps_src.Merge(out_file_name);
				// @v12.4.5 {
				SString h_sentinel_def;
				h_sentinel_def.Z().Cat("__").Cat(ps_src.Nam).CatChar('_').Cat(ps_src.Ext).ToUpper();
				h_sentinel_def.ReplaceChar('-', '_');
				// @v12.4.5 {
				if(!fileExists(out_file_name)) {
					THROW(uedc.GenerateSourceDecl_C(out_file_name, h_sentinel_def, new_version, new_hash));
				}
				else {
					ps_src.Nam.CatChar('-').Cat("temp");
					ps_src.Merge(temp_file_name);
					THROW(uedc.GenerateSourceDecl_C(temp_file_name, h_sentinel_def, new_version, new_hash));
					if(SFile::Compare(out_file_name, temp_file_name, 0) > 0) {
						; // nothing-to-do
						SFile::Remove(temp_file_name);
					}
					else {
						SFile::Remove(out_file_name);
						SFile::Rename(temp_file_name, out_file_name);
					}
				}
			}
			{
				SFsPath ps_src;
				if(!isempty(pJavaPath))
					ps_src.Split(pJavaPath);
				else
					ps_src = ps;
				SrUedContainer_Base::MakeUedCanonicalName(SrUedContainer_Base::canonfnIds, ps_src.Nam, -1);
				ps_src.Ext = "java";
				ps_src.Merge(out_file_name);
				if(!fileExists(out_file_name)) {
					THROW(uedc.GenerateSourceDecl_Java(out_file_name, new_version, new_hash));
				}
				else {
					ps_src.Nam.CatChar('-').Cat("temp");
					ps_src.Merge(temp_file_name);
					THROW(uedc.GenerateSourceDecl_Java(temp_file_name, new_version, new_hash));
					if(SFile::Compare(out_file_name, temp_file_name, 0) > 0) {
						; // nothing-to-do
						SFile::Remove(temp_file_name);
					}
					else {
						SFile::Remove(out_file_name);
						SFile::Rename(temp_file_name, out_file_name);
					}
				}
			}
		}
	}
	CATCH
		if(!dont_log_last_err) {
			CALLPTRMEMB(pLogger, LogLastError());
		}
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
SrUedContainer_Ct::SrUedContainer_Ct() : SrUedContainer_Base()
{
}

SrUedContainer_Ct::~SrUedContainer_Ct()
{
}

int SrUedContainer_Ct::Read(const char * pFileName, PPLogger * pLogger)
{
	return SrUedContainer_Base::ReadSource(pFileName, rsfCompileTime, 0, pLogger);
}

int SrUedContainer_Ct::Write(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash)
{
	return SrUedContainer_Base::WriteSource(pFileName, pPrevHash, pHash);
}

int SrUedContainer_Ct::WriteProps(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash)
{
	return SrUedContainer_Base::WriteProps(pFileName, pPrevHash, pHash);
}
//
//
//
SrUedContainer_Rt::SrUedContainer_Rt(const StringSet * pSsLang) : SrUedContainer_Base()
{
	RVALUEPTR(SsLang, pSsLang);
}

SrUedContainer_Rt::~SrUedContainer_Rt()
{
}
	
int SrUedContainer_Rt::Read(const char * pFileName)
{
	return SrUedContainer_Base::ReadSource(pFileName, 0, (SsLang.getCount() ? &SsLang : 0), 0);
}

bool SrUedContainer_Rt::GetSymb(uint64 ued, SString & rSymb) const
{
	return SearchBaseId(ued, rSymb);
}

uint64 SrUedContainer_Rt::GetBySymb(uint64 meta, const char * pSymb) const // @v12.6.3
{
	return SearchSymb(pSymb, meta);
}

bool SrUedContainer_Rt::GetListByMeta(uint64 meta, Uint64Array & rList) const // @v12.6.9
{
	bool    ok = false;
	if(UED::IsMetaId(meta)) {
		uint64 prev_meta = 0;
		for(uint i = 0; i < BL.getCount(); i++) {
			const  BaseEntry & r_be = BL.at(i);
			if(UED::BelongsToMeta(r_be.Id, meta)) {
				rList.add(r_be.Id);
				ok = true;
			}
			else {
				if(ok) { // !Закладываюсь на то, что список BL отсортирован по значению Id!
					break;
				}
			}
		}
	}
	return ok;
}
