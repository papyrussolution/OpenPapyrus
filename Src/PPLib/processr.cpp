// PROCESSR.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

#define PRCPLCCODESEQMAX 20

ProcessorPlaceCodeTemplate::ProcessorPlaceCodeTemplate()
{
	Reset();
}

void ProcessorPlaceCodeTemplate::Reset()
{
	SeqList.clear();
}

int ProcessorPlaceCodeTemplate::Parse(const char * pPattern)
{
	SeqList.clear();

	int    ok = 1;
	long   re_hdl = 0;
	SString temp_buf, sub, sub2;
	SString pattern;
	SStrScan scan;
	scan.RegisterRe("^[^[%]*\\%[0-9]+\\[[^.]+\\.\\.[^]]+\\]", &re_hdl);
	scan.Set((pattern = pPattern).Strip(), 0);
	scan.Skip();
	while(scan.GetRe(re_hdl, temp_buf)) {
		Seq seq;
		MEMSZERO(seq);
		uint   p = 0;
		for(; p < temp_buf.Len() && temp_buf.C(p) != '%'; p++) {
			if(p < (sizeof(seq.Prefix)-1)) {
				seq.Prefix[p] = temp_buf.C(p);
			}
		}
		THROW_PP_S(temp_buf.C(p) == '%', PPERR_PRCPLCCODE_EXPPCT, pPattern);
		p++;
		sub.Z();
		while(isdec(temp_buf.C(p))) {
			sub.CatChar(temp_buf.C(p++));
		}
		THROW_PP_S(sub.ToLong() > 0 && sub.ToLong() <= 20, PPERR_PRCPLCCODE_INVLEN, pPattern);
		seq.Len = static_cast<uint8>(sub.ToLong());
		THROW_PP_S(temp_buf.C(p) == '[', PPERR_PRCPLCCODE_EXPLBRK, pPattern);
		p++;
		sub.Z();
		while(p < temp_buf.Len() && temp_buf.C(p) != '.') {
			sub.CatChar(temp_buf.C(p++));
		}
		THROW_PP_S(temp_buf.C(p) == '.' && temp_buf.C(p+1) == '.', PPERR_PRCPLCCODE_EXPDDOT, pPattern);
		p += 2;
		sub2 = 0;
		while(p < temp_buf.Len() && temp_buf.C(p) != ']') {
			sub2.CatChar(temp_buf.C(p++));
		}
		THROW_PP_S(temp_buf.C(p) == ']', PPERR_PRCPLCCODE_EXPRBRK, pPattern);
		if(sub.IsDigit()) {
			THROW_PP_S(sub2.IsDigit(), PPERR_PRCPLCCODE_EXPINT, pPattern);
			seq.Type = 1;
			seq.Start = sub.ToLong();
			seq.End = sub2.ToLong();
			THROW_PP_S(seq.Start < seq.End, PPERR_PRCPLCCODE_RNG, pPattern);
		}
		else {
			THROW_PP_S(sub.Len() == sub2.Len() && sub.Len() == 1, PPERR_PRCPLCCODE_ALRNGLEN, pPattern);
			const char c1 = sub.C(0);
			const char c2 = sub2.C(0);
			// @v10.9.8 THROW_PP_S((c1 >= 'A' && c1 <= 'Z') || (c1 >= 'a' && c1 <= 'z') || IsLetter866(c1), PPERR_PRCPLCCODE_ALRNGSYM, pPattern);
			// @v10.9.8 THROW_PP_S((c2 >= 'A' && c2 <= 'Z') || (c2 >= 'a' && c2 <= 'z') || IsLetter866(c2), PPERR_PRCPLCCODE_ALRNGSYM, pPattern);
			THROW_PP_S(isasciialpha(c1) || IsLetter866(c1), PPERR_PRCPLCCODE_ALRNGSYM, pPattern); // @v10.9.8 
			THROW_PP_S(isasciialpha(c2) || IsLetter866(c2), PPERR_PRCPLCCODE_ALRNGSYM, pPattern); // @v10.9.8 
			seq.Type = 2;
			seq.Start = ToUpper866(c1);
			seq.End = ToUpper866(c2);
			THROW_PP_S(seq.Start < seq.End, PPERR_PRCPLCCODE_RNG, pPattern);
		}
		SeqList.insert(&seq);
	}
	THROW(scan.Offs == pattern.Len());
	THROW_PP_S(SeqList.getCount() > 0 && SeqList.getCount() <= PRCPLCCODESEQMAX, PPERR_PRCPLCCODE_TOOMANYSEG, pPattern);
	CATCHZOK
	return ok;
}

void ProcessorPlaceCodeTemplate::Helper_GenerateLevel(int s, const uint32 pCounters[], SString & rBuf) const
{
	assert(s >= 0 && s < SeqList.getCountI());
	const Seq & r_seq = SeqList.at(s);
	const uint32 c = pCounters[s];
	if(r_seq.Prefix[0]) {
		rBuf.Cat(r_seq.Prefix);
	}
	if(r_seq.Type == 1) {
		rBuf.CatLongZ(c, r_seq.Len);
	}
	else if(r_seq.Type == 2) {
		rBuf.CatChar((char)(c & 0xff));
	}
}

int ProcessorPlaceCodeTemplate::Helper_Generate(int s, int fullRow, uint32 pCounters[], SString & rBuf) const
{
	assert(s >= 0 && s < SeqList.getCountI());
	int   ret_pos = s;
	const Seq & r_seq = SeqList.at(s);
	if(pCounters[s] <= r_seq.End) {
		if(fullRow) {
			for(int i = 0; i < s; i++) {
				Helper_GenerateLevel(i, pCounters, rBuf);
			}
		}
		Helper_GenerateLevel(s, pCounters, rBuf);
		if((s+1) < SeqList.getCountI()) {
			ret_pos = Helper_Generate(s+1, 0, pCounters, rBuf);
		}
		else {
			ret_pos = s;
			while(ret_pos >= 0 && pCounters[ret_pos] == SeqList.at(ret_pos).End)
				ret_pos--;
		}
	}
	else
		ret_pos = s - 1;
	return ret_pos;
}

int ProcessorPlaceCodeTemplate::Generate(StringSet & rSs) const
{
	int    ok = -1;
	uint   i;
	int    first_enum_idx = 0;
	const  uint _c = SeqList.getCount();
	SString temp_buf, code;
	uint32 counters[PRCPLCCODESEQMAX];
	memzero(counters, sizeof(counters));
	THROW_PP_S(SeqList.getCount() > 0 && _c <= PRCPLCCODESEQMAX, PPERR_PRCPLCCODE_TOOMANYSEG, "");
	for(i = 0; i < _c; i++) {
		const Seq & r_seq = SeqList.at(i);
		if(oneof2(r_seq.Type, 1, 2)) {
			SETIFZ(first_enum_idx, i+1);
			counters[i] = r_seq.Start;
		}
	}
	{
		if(first_enum_idx) {
			first_enum_idx--;
			int _p = first_enum_idx;
			do {
				_p = Helper_Generate(_p, 1, counters, code = 0);
				if(_p >= first_enum_idx) {
					rSs.add(code);
					counters[_p]++;
					for(uint i = _p+1; i < _c; i++)
						counters[i] = SeqList.at(i).Start;
				}
				else
					break;
			} while(1);
		}
	}
	CATCHZOK
	return ok;
}

/*static*/SString & FASTCALL ProcessorPlaceCodeTemplate::NormalizeCode(SString & rCode)
{
	return rCode.Strip().ToUpper();
}

int ProcessorPlaceCodeTemplate::HasCode(const char * pCode) const
{
	int    ok = 1;
	SString code(pCode);
	SString sub;
	NormalizeCode(code);
	if(code.Len() == 0)
		ok = 0;
	else {
		for(uint i = 0; ok && i < SeqList.getCount(); i++) {
			const Seq & r_seq = SeqList.at(i);
			if(r_seq.Prefix[0]) {
				THROW(code.CmpPrefix(r_seq.Prefix, 1) == 0);
				code.ShiftLeft(sstrlen(r_seq.Prefix));
			}
			THROW(oneof2(r_seq.Type, 1, 2));
			THROW(r_seq.Len);
			code.Sub(0, r_seq.Len, sub);
			THROW(sub.Len() == r_seq.Len);
			if(r_seq.Type == 1) {
				ulong v = static_cast<ulong>(sub.ToLong());
				THROW(v >= r_seq.Start && v <= r_seq.End);
				code.ShiftLeft(sub.Len());
			}
			else if(r_seq.Type == 2) {
				THROW(sub.Last() >= static_cast<char>(r_seq.Start) && sub.Last() <= static_cast<char>(r_seq.End));
				code.ShiftLeft(sub.Len());
			}
		}
		THROW(code.Len() == 0);
	}
	CATCHZOK
	return ok;
}
//
//
//
PPProcessorPacket::PlaceDescription::PlaceDescription() : GoodsID(0)
{
}

PPProcessorPacket::PlaceDescription & PPProcessorPacket::PlaceDescription::Z()
{
	GoodsID = 0;
	Range.Z();
	Descr.Z();
	return *this;
}

PPProcessorPacket::ExtBlock::ExtBlock()
{
	destroy();
}

PPProcessorPacket::ExtBlock & PPProcessorPacket::ExtBlock::destroy()
{
	Ver = DS.GetVersion();
	CheckInTime = ZEROTIME;
	CheckOutTime = ZEROTIME;
	TimeFlags = 0;
	InitSessStatus = 0;
	ExtStrP = 0;
	MEMSZERO(Fb);
	DestroyS();
	Places.clear();
	return *this;
}

int PPProcessorPacket::ExtBlock::IsEmpty() const
{
	return BIN(CheckInTime == 0 && CheckOutTime == 0 && TimeFlags == 0 && InitSessStatus == 0 && ExtStrP == 0 && !Places.getCount() && !GetOwnerGuaID());
}

int PPProcessorPacket::ExtBlock::GetExtStrData(int fldID, SString & rBuf) const
{
	int    ok = -1;
	if(ExtStrP) {
		SString ext_string;
		GetS(ExtStrP, ext_string);
		ok = PPGetExtStrData(fldID, ext_string, rBuf);
	}
	return ok;
}

int PPProcessorPacket::ExtBlock::PutExtStrData(int fldID, const char * pBuf)
{
	SString ext_string;
	GetS(ExtStrP, ext_string);
	int    ok = PPPutExtStrData(fldID, ext_string, pBuf);
	AddS(ext_string.Strip(), &ExtStrP);
	return ok;
}

int PPProcessorPacket::ExtBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		THROW(Pack());
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW_SL(SerializeS(dir, rBuf, pSCtx));
    THROW_SL(pSCtx->Serialize(dir, CheckInTime, rBuf));
    THROW_SL(pSCtx->Serialize(dir, CheckOutTime, rBuf));
    THROW_SL(pSCtx->Serialize(dir, TimeFlags, rBuf));
    THROW_SL(pSCtx->Serialize(dir, InitSessStatus, rBuf));
    THROW_SL(pSCtx->Serialize(dir, ExtStrP, rBuf));
    THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Fb), &Fb, rBuf, 1));
    THROW_SL(pSCtx->Serialize(dir, &Places, rBuf));
    CATCHZOK
	return ok;
}

int PPProcessorPacket::ExtBlock::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		if(p_pack_handle) {
			const uint c = Places.getCount();
			for(uint i = 0; ok && i < c; i++) {
				InnerPlaceDescription & r_item = Places.at(i);
				Pack_Replace(p_pack_handle, r_item.RangeP);
				Pack_Replace(p_pack_handle, r_item.DescrP);
			}
			Pack_Replace(p_pack_handle, ExtStrP);
			Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

PPID PPProcessorPacket::ExtBlock::GetOwnerGuaID() const
{
	return Fb.OwnerGuaID;
}

void PPProcessorPacket::ExtBlock::SetOwnerGuaID(PPID id)
{
	Fb.OwnerGuaID = id;
}

long PPProcessorPacket::ExtBlock::GetCipCancelTimeout() const
{
	return Fb.CipCancelTimeout;
}

int  PPProcessorPacket::ExtBlock::SetCipCancelTimeout(long t)
{
	if(t >= 0) {
		Fb.CipCancelTimeout = t;
		return 1;
	}
	else
		return 0;
}

long PPProcessorPacket::ExtBlock::GetCipLockTimeout() const
{
	return Fb.CipLockTimeout;
}

int  PPProcessorPacket::ExtBlock::SetCipLockTimeout(long t)
{
	if(t >= 0) {
		Fb.CipLockTimeout = t;
		return 1;
	}
	else
		return 0;
}

uint PPProcessorPacket::ExtBlock::GetPlaceDescriptionCount() const
{
    return Places.getCount();
}

int PPProcessorPacket::ExtBlock::GetPlaceDescription(uint pos, PPProcessorPacket::PlaceDescription & rItem) const
{
	int    ok = 0;
	rItem.Z();
    if(pos < Places.getCount()) {
        const InnerPlaceDescription & r_item = Places.at(pos);
        rItem.GoodsID = r_item.GoodsID;
        GetS(r_item.RangeP, rItem.Range);
        GetS(r_item.DescrP, rItem.Descr);
        ok = 1;
    }
	return ok;
}

int PPProcessorPacket::ExtBlock::GetPlaceDescriptionByCode(const char * pCode, PPProcessorPacket::PlaceDescription & rItem) const
{
	int    ok = 0;
    if(Places.getCount()) {
		ProcessorPlaceCodeTemplate ppct;
		SString temp_buf;
        for(uint i = 0; !ok && i < Places.getCount(); i++) {
			const InnerPlaceDescription & r_item = Places.at(i);
			GetS(r_item.RangeP, temp_buf);
			if(temp_buf.NotEmptyS()) {
				if(ppct.Parse(temp_buf) && ppct.HasCode(pCode)) {
                    rItem.GoodsID = r_item.GoodsID;
                    rItem.Range = temp_buf;
                    GetS(r_item.DescrP, rItem.Descr);
                    ok = 1;
				}
			}
        }
    }
    return ok;
}

int PPProcessorPacket::ExtBlock::PutPlaceDescription(uint pos, const PPProcessorPacket::PlaceDescription * pItem)
{
	int    ok = 1;
	if(pos < Places.getCount()) {
		if(pItem) {
            InnerPlaceDescription new_item;
            new_item.GoodsID = pItem->GoodsID;
            THROW_SL(AddS(pItem->Range, &new_item.RangeP));
            THROW_SL(AddS(pItem->Descr, &new_item.DescrP));
            Places.at(pos) = new_item;
            ok = 2;
		}
		else {
			THROW_SL(Places.atFree(pos));
			ok = 3;
		}
	}
	else if(pos == Places.getCount()) {
        if(pItem) {
            InnerPlaceDescription new_item;
            new_item.GoodsID = pItem->GoodsID;
            THROW_SL(AddS(pItem->Range, &new_item.RangeP));
            THROW_SL(AddS(pItem->Descr, &new_item.DescrP));
            THROW_SL(Places.insert(&new_item));
            ok = 1;
        }
        else
			ok = 0;
	}
	CATCHZOK
	return ok;
}

PPProcessorPacket::PPProcessorPacket()
{
	// @v10.7.1 destroy();
}

PPProcessorPacket & PPProcessorPacket::destroy()
{
	MEMSZERO(Rec);
	Ext.destroy();
	return *this;
}

TLP_IMPL(PPObjProcessor, ProcessorTbl, P_Tbl);

/*static*/int FASTCALL PPObjProcessor::ReadConfig(PPProcessorConfig * pCfg)
{
	int    r = PPRef->GetPropMainConfig(PPPRP_PRCCFG, pCfg, sizeof(*pCfg));
	if(r <= 0)
		memzero(pCfg, sizeof(*pCfg));
	return r;
}

PPObjProcessor::PPObjProcessor(void * extraPtr) : PPObject(PPOBJ_PROCESSOR), ExtraPtr(extraPtr)
{
	TLP_OPEN(P_Tbl);
	ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

PPObjProcessor::~PPObjProcessor()
{
	TLP_CLOSE(P_Tbl);
}

int PPObjProcessor::Search(PPID id, void * b)
{
	return SearchByID(P_Tbl, Obj, id, b);
}

/*virtual*/const char * PPObjProcessor::GetNamePtr()
{
	return P_Tbl->data.Name;
}

int PPObjProcessor::SearchByName(int kind, const char * pName, PPID * pID, ProcessorTbl::Rec * pRec)
{
	int    ok = -1;
	PPID   id = 0;
	if(kind == 0) {
		if(SearchByName(PPPRCK_PROCESSOR, pName, &id, pRec) > 0) // @recursion
			ok = 1;
		else if(SearchByName(PPPRCK_GROUP, pName, &id, pRec) > 0) // @recursion
			ok = 1;
	}
	else {
		ProcessorTbl::Key2 k2;
		MEMSZERO(k2);
		k2.Kind = kind;
		STRNSCPY(k2.Name, pName);
		ok = SearchByKey(P_Tbl, 2, &k2, pRec);
		if(ok > 0)
			id = P_Tbl->data.ID;
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPObjProcessor::SearchByCode(const char * pCode, PPID * pID, ProcessorTbl::Rec * pRec)
{
	if(pCode) {
		BExtQuery q(P_Tbl, 0);
		q.selectAll().where(P_Tbl->Code == pCode);
		ProcessorTbl::Key0 k0;
		MEMSZERO(k0);
		for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;)
			if(stricmp(P_Tbl->data.Code, pCode) == 0) {
				ASSIGN_PTR(pID, P_Tbl->data.ID);
				ASSIGN_PTR(pRec, P_Tbl->data);
				return 1;
			}
	}
	return -1;
}

int PPObjProcessor::SearchByLinkObj(PPID objType, PPID objID, PPID * pID, ProcessorTbl::Rec * pRec)
{
	int    ok = -1;
	ProcessorTbl * p_t = P_Tbl;
	ProcessorTbl::Key1 k1;
	MEMSZERO(k1);
	k1.Kind = PPPRCK_PROCESSOR;
	BExtQuery q(p_t, 0, 1);
	q.selectAll().where(p_t->Kind == (long)PPPRCK_PROCESSOR && p_t->LinkObjType == objType && p_t->LinkObjID == objID);
	for(q.initIteration(0, &k1, spGe); ok < 0 && q.nextIteration() > 0;) {
		ASSIGN_PTR(pID, p_t->data.ID);
		ASSIGN_PTR(pRec, p_t->data);
		ok = 1;
	}
	return ok;
}

int PPObjProcessor::GetChildIDList(PPID prcID, int recur, PPIDArray * pList)
{
	int    ok = -1;
	ProcessorTbl * p_t = P_Tbl;
	ProcessorTbl::Key1 k1;
	if(recur) {
		MEMSZERO(k1);
		k1.Kind = PPPRCK_GROUP;
		k1.ParentID = prcID;
		BExtQuery q(p_t, 1);
		q.select(p_t->ID, 0L).where(p_t->Kind == (long)PPPRCK_GROUP && p_t->ParentID == prcID);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			ok = 1;
			if(pList)
				THROW_SL(pList->addUnique(p_t->data.ID));
			THROW(GetChildIDList(p_t->data.ID, 1, pList)); // @recursion
		}
	}
	{
		MEMSZERO(k1);
		k1.Kind = PPPRCK_PROCESSOR;
		k1.ParentID = prcID;
		BExtQuery q(p_t, 1);
		q.select(p_t->ID, 0L).where(p_t->Kind == (long)PPPRCK_PROCESSOR && p_t->ParentID == prcID);
		for(q.initIteration(0, &k1, spGe); q.nextIteration() > 0;) {
			ok = 1;
			if(pList)
				THROW_SL(pList->addUnique(p_t->data.ID));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjProcessor::GetListByOwnerGuaID(PPID guaID, PPIDArray & rList)
{
	rList.clear();
	int    ok = -1;
	if(guaID) {
		Reference * p_ref = PPRef;
		SBuffer buffer;
		SSerializeContext sctx;
		PPProcessorPacket::ExtBlock ext;
		PropertyTbl::Key1 k1;
		MEMSZERO(k1);
		k1.ObjType = Obj;
		k1.Prop = PRCPRP_EXT2;
		if(p_ref->Prop.search(1, &k1, spGt) && k1.ObjType == Obj && k1.Prop == PRCPRP_EXT2) do {
			const PPID prc_id = p_ref->Prop.data.ObjID;
			buffer.Z();
			if(p_ref->GetPropSBuffer_Current(buffer) > 0) {
				ext.destroy();
				if(ext.Serialize(-1, buffer, &sctx) && ext.GetOwnerGuaID() == guaID) {
					rList.add(prc_id);
					ok = 1;
				}
			}
		} while(p_ref->Prop.search(1, &k1, spNext) && k1.ObjType == Obj && k1.Prop == PRCPRP_EXT2);
	}
	return ok;
}

int PPObjProcessor::GetParentsList(PPID prcID, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray id_list;
	for(PPID id = prcID; id != 0;) {
		ProcessorTbl::Rec rec;
		if(Search(id, &rec) > 0 && rec.ParentID) {
			int    r = id_list.addUnique(rec.ParentID);
			THROW_SL(r);
			// Проверка на циклические ссылки {
			THROW_PP_S(r > 0, PPERR_CYCLELINKPRC, rec.Name);
			// }
			id = rec.ParentID;
			ok = 1;
		}
		else
			id = 0;
	}
	CATCHZOK
	ASSIGN_PTR(pList, id_list);
	return ok;
}

int PPObjProcessor::GetExtWithInheritance(PPID prcID, PPProcessorPacket::ExtBlock * pExt)
{
	int    ok = -1;
	ProcessorTbl::Rec rec, parent_rec;
	PPProcessorPacket::ExtBlock ext, parent_ext;
	PPIDArray id_list;
	int    init_status = 0;
	if(Fetch(prcID, &rec) > 0) {
		if(GetExtention(prcID, &ext) > 0) {
			if(PPObjTSession::ValidateStatus(ext.InitSessStatus))
				init_status = ext.InitSessStatus;
			ASSIGN_PTR(pExt, ext);
			ok = 1;
		}
		for(PPID parent_id = rec.ParentID; (ok < 0 || !init_status) && parent_id != 0; parent_id = parent_rec.ParentID) {
			THROW(Fetch(parent_id, &parent_rec) > 0);
			THROW_PP_S(id_list.lsearch(parent_rec.ParentID) == 0, PPERR_CYCLELINKPRC, parent_rec.Name);
			id_list.add(parent_rec.ParentID);
			if(GetExtention(parent_id, &parent_ext) > 0) {
				if(!init_status && PPObjTSession::ValidateStatus(parent_ext.InitSessStatus))
					init_status = parent_ext.InitSessStatus;
				if(ok < 0) {
					ASSIGN_PTR(pExt, parent_ext);
					ok = 1;
				}
			}
		}
	}
	if(ok > 0 && pExt)
		pExt->InitSessStatus = init_status;
	CATCHZOK
	return ok;
}

int PPObjProcessor::GetRecWithInheritance(PPID prcID, ProcessorTbl::Rec * pRec, int useCache)
{
	int    ok = -1;
	ProcessorTbl::Rec rec, parent_rec;
	PPIDArray id_list;
	if((useCache ? Fetch(prcID, &rec) : Search(prcID, &rec)) > 0) {
		for(PPID parent_id = rec.ParentID; parent_id != 0; parent_id = parent_rec.ParentID)
			if(!rec.LocID || !rec.TimeUnitID || !rec.LinkObjType || !rec.WrOffOpID ||
				!rec.WrOffArID || !(rec.Flags & PRCF_LOCKWROFF) || !(rec.Flags & PRCF_TURNINCOMPLBILL)) {
				THROW((useCache ? Fetch(parent_id, &parent_rec) : Search(parent_id, &parent_rec)) > 0);
				// Проверка на циклические ссылки {
				THROW_PP_S(id_list.lsearch(parent_rec.ParentID) == 0, PPERR_CYCLELINKPRC, parent_rec.Name);
				id_list.add(parent_rec.ParentID); // @v8.1.6 addUnique-->add
				// }
				SETIFZ(rec.LocID, parent_rec.LocID);
				SETIFZ(rec.TimeUnitID, parent_rec.TimeUnitID);
				SETIFZ(rec.LinkObjType, parent_rec.LinkObjType);
				if(rec.Kind == PPPRCK_GROUP) {
					//
					// Для групп наследуем подгруппу связанного объекта
					//
					SETIFZ(rec.LinkObjID, parent_rec.LinkObjID);
				}
				if(!rec.WrOffOpID) {
					rec.WrOffOpID = parent_rec.WrOffOpID;
					if(parent_rec.WrOffOpID) {
						SETFLAGBYSAMPLE(rec.Flags, PRCF_WROFFDT_START,   parent_rec.Flags);
						SETFLAGBYSAMPLE(rec.Flags, PRCF_WROFFDT_BYSUPER, parent_rec.Flags);
					}
				}
				SETIFZ(rec.WrOffArID, parent_rec.WrOffArID);
				//
				// Любая блокировка списания вверх по иерархии
				// блокирует списание для всех нижестоящих процессоров
				//
				if(parent_rec.Flags & PRCF_LOCKWROFF)
					rec.Flags |= PRCF_LOCKWROFF;
				//
				// Любое разрешение списания дефицитных документов
				// разрешает такое списание для всех нижестоящих процессоров
				//
				if(parent_rec.Flags & PRCF_TURNINCOMPLBILL)
					rec.Flags |= PRCF_TURNINCOMPLBILL;
				if(parent_rec.Flags & PRCF_ACCDUPSERIALINSESS)
					rec.Flags |= PRCF_ACCDUPSERIALINSESS;
				//
				// Любое разрешение регистрации персоналий разрешает такую регистрацию для всех
				// нижестоящих процессоров.
				//
				if(parent_rec.Flags & PRCF_ALLOWCIP)
					rec.Flags |= PRCF_ALLOWCIP;
				if(rec.Flags & PRCF_ALLOWCIP) {
					//
					// Вид персоналий для регистрации также наследуется вниз по иерархии
					//
					SETIFZ(rec.CipPersonKindID, parent_rec.CipPersonKindID);
				}
				//
				// Любое предписание использования упрощенного диалога сессии наследуется для всех нижестоящих процессоров.
				//
				if(parent_rec.Flags & PRCF_USETSESSSIMPLEDLG)
					rec.Flags |= PRCF_USETSESSSIMPLEDLG;
				//
				// Любое разрешение перехода статуса ЗАКРЫТА->ОТМЕНЕНА
				// разрешает такой переход для всех нижестоящих процессоров
				//
				if(parent_rec.Flags & PRCF_ALLOWCANCELAFTERCLOSE)
					rec.Flags |= PRCF_ALLOWCANCELAFTERCLOSE;
				//
				// Квант временной диаграммы наследуется в случае, если в нижнем по иерархии процессоре он не определен.
				//
				if(parent_rec.TcbQuant > 0 && rec.TcbQuant <= 0)
					rec.TcbQuant = parent_rec.TcbQuant;
				SETFLAGBYSAMPLE(rec.Flags, PRCF_ADDEDOBJASAGENT, parent_rec.Flags);
			}
			else
				break;
		ASSIGN_PTR(pRec, rec);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjProcessor::IsSwitchable(PPID prcID, PPIDArray * pSwitchPrcList)
{
	int    ok = -1;
	ProcessorTbl::Rec prc_rec, parent_rec;
	CALLPTRMEMB(pSwitchPrcList, clear()); // @v10.7.1 freeAll()-->clear()
	if(Fetch(prcID, &prc_rec) > 0 && Fetch(prc_rec.ParentID, &parent_rec) > 0)
		if(parent_rec.Flags & PRCF_CANSWITCHPAN) {
			ok = 1;
			if(pSwitchPrcList)
				if(!GetChildIDList(parent_rec.ID, 0, pSwitchPrcList))
					return 0;
		}
	return ok;
}

int PPObjProcessor::SearchAnyRef(PPID objType, PPID objID, PPID * pID)
{
	ProcessorTbl * p_t = P_Tbl;
	if(objType == PPOBJ_PROCESSOR) {
		ProcessorTbl::Key1 k;
		MEMSZERO(k);
		k.Kind = PPPRCK_GROUP;
		k.ParentID = objID;
		if(p_t->search(1, &k, spGe) && k.Kind == PPPRCK_GROUP && k.ParentID == objID) {
			ASSIGN_PTR(pID, p_t->data.ID);
			return 1;
		}
		else {
			MEMSZERO(k);
			k.Kind = PPPRCK_PROCESSOR;
			k.ParentID = objID;
			if(p_t->search(1, &k, spGe) && k.Kind == PPPRCK_PROCESSOR && k.ParentID == objID) {
				ASSIGN_PTR(pID, p_t->data.ID);
				return 1;
			}
		}
	}
	else {
		DBQ * dbq = 0;
		if(objType == PPOBJ_UNIT)
			dbq = & (p_t->TimeUnitID == objID);
		else if(objType == PPOBJ_LOCATION)
			dbq = & (p_t->LocID == objID);
		else if(objType == PPOBJ_OPRKIND)
			dbq = & (p_t->WrOffOpID == objID);
		else if(objType == PPOBJ_ARTICLE)
			dbq = & (p_t->WrOffArID == objID);
		else if(objType == PPOBJ_BCODEPRINTER)
			dbq = & (p_t->PrinterID == objID);
		else
			dbq = & (p_t->LinkObjType == objType && p_t->LinkObjID == objID);
		{
			ProcessorTbl::Key0 k0;
			BExtQuery q(p_t, 0);
			q.select(p_t->ID, 0L).where(*dbq);
			k0.ID = 0;
			if(q.fetchFirst(&k0, spGt) > 0) {
				ASSIGN_PTR(pID, p_t->data.ID);
				return 1;
			}
		}
	}
	return -1;
}

int PPObjProcessor::DeleteObj(PPID id)
{
	ProcessorTbl::Rec rec;
	if(Search(id, &rec) > 0) {
		if(rec.Kind == PPPRCK_GROUP) {
			PPID   branch_id = 0;
			if(SearchAnyRef(PPOBJ_PROCESSOR, id, &branch_id) > 0)
				return RetRefsExistsErr(Obj, branch_id);
		}
		return RemoveByID(P_Tbl, id, 0);
	}
	else
		return 0;
}

int PPObjProcessor::PutPacket(PPID * pID, PPProcessorPacket * pPack, int use_ta)
{
	int    ok = 1, ta = 0;
	PPID   log_action_id = 0;
	const  int has_ext = pPack ? BIN(!pPack->Ext.IsEmpty()) : 0;
	ProcessorTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			THROW(Search(*pID, &rec) > 0);
			if(pPack == 0) {
				//
				// Удаление пакета
				//
				THROW(RemoveObjV(*pID, 0, 0, 0));
				THROW(PutExtention(*pID, 0, 0));
			}
			else {
				//
				// Изменение пакета
				//
				SETFLAG(pPack->Rec.Flags, PRCF_HASEXT, has_ext);
				if(rec.Kind == PPPRCK_GROUP) {
					if(rec.LinkObjType)
						if(pPack->Rec.LinkObjType != rec.LinkObjType || pPack->Rec.LinkObjID != rec.LinkObjID)
							THROW_PP(GetChildIDList(*pID, 1, 0) > 0, PPERR_UPDLINKNONEMPTYPRCGRP);
				}
				THROW(UpdateByID(P_Tbl, Obj, *pID, &pPack->Rec, 0));
				THROW(PutExtention(*pID, &pPack->Ext, 0));
				log_action_id = PPACN_OBJUPD;
			}
			Dirty(*pID);
		}
		else if(pPack) {
			//
			// Добавление пакета
			//
			SETFLAG(pPack->Rec.Flags, PRCF_HASEXT, has_ext);
			THROW(AddObjRecByID(P_Tbl, Obj, pID, &pPack->Rec, 0));
			pPack->Rec.ID = *pID;
			THROW(PutExtention(*pID, &pPack->Ext, 0));
			log_action_id = PPACN_OBJADD;
		}
		DS.LogAction(log_action_id, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

struct Strg_ProcessorExt { // @persistent
	long   ObjType;      // const PPOBJ_PROCESSOR
	long   ObjID;        // ->Processor.ID
	long   Prop;         // const PRCPRP_EXT
	SVerT  Ver;
	LTIME  CheckInTime;
	LTIME  CheckOutTime;
	long   TimeFlags;
	uint32 ExtStrLen;
	long   InitSessStatus; // @v8.2.9
	uint8  Reserve[40];    // @v8.2.9 [44]-->[40]
	long   ReserveVal1;
	long   ReserveVal2;
	// ExtString
};

#if 0 // @v8.7.0 {
int PPObjProcessor::PutExtention(PPID id, PPProcessorPacket::ExtBlock * pExt, int use_ta)
{
	int    ok = 1;
	Strg_ProcessorExt * p_strg = 0;
	size_t sz = 0;
	if(pExt && !pExt->IsEmpty()) {
		SString stub_extsting;
		const size_t ext_str_len = stub_extsting.NotEmpty() ? (stub_extsting.Len() + 1) : 0;
		sz = sizeof(Strg_ProcessorExt) + ext_str_len;
		THROW_MEM(p_strg = (Strg_ProcessorExt *)SAlloc::M(sz));
		memzero(p_strg, sz);
		p_strg->Ver = DS.GetVersion();
		p_strg->CheckInTime = pExt->CheckInTime;
		p_strg->CheckOutTime = pExt->CheckOutTime;
		p_strg->TimeFlags = pExt->TimeFlags;
		p_strg->InitSessStatus = pExt->InitSessStatus;
		p_strg->ExtStrLen = ext_str_len;
		if(ext_str_len) {
			memcpy((p_strg+1), (const char *)stub_extsting, ext_str_len);
		}
	}
	THROW(PPRef->PutProp(PPOBJ_PROCESSOR, id, PRCPRP_EXT, p_strg, sz, use_ta));
	CATCHZOK
	return ok;
}
#endif // } @v8.7.0

int PPObjProcessor::PutExtention(PPID id, PPProcessorPacket::ExtBlock * pExt, int use_ta)
{
	int    ok = 1;
	SBuffer buffer;
	SSerializeContext sctx;
	if(pExt && !pExt->IsEmpty()) {
		THROW(pExt->Serialize(+1, buffer, &sctx));
	}
	THROW(PPRef->PutPropSBuffer(Obj, id, PRCPRP_EXT2, buffer, use_ta));
	CATCHZOK
	return ok;
}

int PPObjProcessor::GetExtention(PPID id, PPProcessorPacket::ExtBlock * pExt)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	Strg_ProcessorExt * p_strg = 0;
	size_t sz = 0;
	SBuffer buffer;
	if(p_ref->GetPropSBuffer(Obj, id, PRCPRP_EXT2, buffer) > 0) {
		if(pExt) {
			SSerializeContext sctx;
			THROW(pExt->Serialize(-1, buffer, &sctx));
		}
		ok = 1;
	}
	else if(p_ref->GetPropActualSize(Obj, id, PRCPRP_EXT, &sz) > 0) {
		if(pExt) {
			SString stub_extsting;
			p_strg = static_cast<Strg_ProcessorExt *>(SAlloc::M(sz));
			ok = p_ref->GetProperty(Obj, id, PRCPRP_EXT, p_strg, sz);
			assert(ok > 0); // Раз нам удалось считать размер буфера, то последующая ошибка чтения - критична
			THROW(ok > 0);
			pExt->destroy();
			pExt->CheckInTime = p_strg->CheckInTime;
			pExt->CheckOutTime = p_strg->CheckOutTime;
			pExt->TimeFlags = p_strg->TimeFlags;
			pExt->InitSessStatus = p_strg->InitSessStatus;
			if(p_strg->ExtStrLen > 0) {
				stub_extsting = reinterpret_cast<const char *>(p_strg+1);
			}
		}
		ok = 2;
	}
	CATCHZOK
	SAlloc::F(p_strg);
	return ok;
}

int PPObjProcessor::GetPacket(PPID id, PPProcessorPacket * pPack)
{
	int    ok = -1;
	pPack->destroy();
	if(PPCheckGetObjPacketID(Obj, id)) { // @v10.3.6
		ok = Search(id, &pPack->Rec);
		if(ok > 0) {
			THROW(GetExtention(id, &pPack->Ext));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class ProcessorCache : public ObjCache {
public:
	ProcessorCache() : ObjCache(PPOBJ_PROCESSOR, sizeof(Data))
	{
	}
private:
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		long   ParentID;
		long   Kind;
		long   LocID;
		long   TimeUnitID;
		long   Flags;
		long   LinkObjType;
		long   LinkObjID;
		long   WrOffOpID;
		long   WrOffArID;
		long   SuperSessTiming;
		long   RestAltGrpID;
		long   PrinterID;
		int16  CipMax;
		int16  TcbQuant;
		long   CipPersonKindID;
	};
};

int ProcessorCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjProcessor prc_obj;
	ProcessorTbl::Rec rec;
	if(prc_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(ParentID);
		CPY_FLD(Kind);
		CPY_FLD(LocID);
		CPY_FLD(TimeUnitID);
		CPY_FLD(Flags);
		CPY_FLD(LinkObjType);
		CPY_FLD(LinkObjID);
		CPY_FLD(WrOffOpID);
		CPY_FLD(WrOffArID);
		CPY_FLD(SuperSessTiming);
		CPY_FLD(RestAltGrpID);
		CPY_FLD(PrinterID);
		CPY_FLD(CipMax);
		CPY_FLD(CipPersonKindID);
		CPY_FLD(TcbQuant);
#undef CPY_FLD
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Code);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void ProcessorCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	ProcessorTbl::Rec * p_data_rec = static_cast<ProcessorTbl::Rec *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	CPY_FLD(ID);
	CPY_FLD(ParentID);
	CPY_FLD(Kind);
	CPY_FLD(LocID);
	CPY_FLD(TimeUnitID);
	CPY_FLD(Flags);
	CPY_FLD(LinkObjType);
	CPY_FLD(LinkObjID);
	CPY_FLD(WrOffOpID);
	CPY_FLD(WrOffArID);
	CPY_FLD(SuperSessTiming);
	CPY_FLD(RestAltGrpID);
	CPY_FLD(PrinterID);
	CPY_FLD(CipMax);
	CPY_FLD(CipPersonKindID);
	CPY_FLD(TcbQuant);
#undef CPY_FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Code, sizeof(p_data_rec->Code));
}

int PPObjProcessor::Fetch(PPID id, ProcessorTbl::Rec * pRec)
{
	ProcessorCache * p_cache = GetDbLocalCachePtr <ProcessorCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int PPObjProcessor::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE) {
		PPID   prc_id = 0;
		if(SearchAnyRef(_obj, _id, &prc_id) > 0)
			ok = RetRefsExistsErr(Obj, prc_id);
	}
	else if(msg == DBMSG_PERSONACQUIREKIND) {
		const PPID person_id = _id;
		const PPID kind_id = reinterpret_cast<long>(extraPtr);
		SString name_buf;
		PPIDArray grp_id_list;
		ProcessorTbl::Rec grp_rec;
		ProcessorTbl::Key1 k1;
		MEMSZERO(k1);
		k1.Kind = PPPRCK_GROUP;
		if(P_Tbl->search(1, &k1, spGe) && P_Tbl->data.Kind == PPPRCK_GROUP) do {
			P_Tbl->copyBufTo(&grp_rec);
			if(grp_rec.LinkObjType == PPOBJ_PERSON && grp_rec.LinkObjID == kind_id && grp_rec.Flags & PRCF_AUTOCREATE) {
				grp_id_list.add(grp_rec.ID);
			}
		} while(P_Tbl->search(1, &k1, spNext) && P_Tbl->data.Kind == PPPRCK_GROUP);
		for(uint i = 0; i < grp_id_list.getCount(); i++) {
			PPID   prc_id = 0;
			int    r;
			THROW(r = SearchByLinkObj(PPOBJ_PERSON, person_id, &prc_id, 0));
			if(r < 0) {
				prc_id = 0;
				PPProcessorPacket pack;
				pack.Rec.Kind = PPPRCK_PROCESSOR;
				pack.Rec.ParentID = grp_id_list.get(i);
				THROW(GetRecWithInheritance(pack.Rec.ParentID, &grp_rec, 1) > 0);
				assert(grp_rec.LinkObjType == PPOBJ_PERSON);
				pack.Rec.LinkObjType = grp_rec.LinkObjType;
				pack.Rec.LinkObjID = person_id;
				pack.Rec.LocID = grp_rec.LocID;
				THROW(GetPersonName(person_id, name_buf) > 0);
				name_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
				SETFLAGBYSAMPLE(pack.Rec.Flags, PRCF_USETSESSSIMPLEDLG, grp_rec.Flags);
				THROW(PutPacket(&prc_id, &pack, 0));
			}
		}
	}
	else if(msg == DBMSG_PERSONLOSEKIND) {
		const  PPID person_id = _obj;
		const  PPID kind_id = _id;
		PPID   prc_id = 0;
		if(SearchByLinkObj(PPOBJ_PERSON, person_id, &prc_id, 0) > 0)
			return RetRefsExistsErr(Obj, prc_id);
	}
	CATCHZOK
	return ok;
}
//
//
//
PrcCtrlGroup::Rec::Rec(PPID prcID) : PrcID(prcID)
{
}

PrcCtrlGroup::PrcCtrlGroup(uint ctlSelPrc) : CtrlGroup(), CtlSelPrc(ctlSelPrc)
{
	// @v10.6.4 MEMSZERO(Data);
}

int PrcCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupPPObjCombo(pDlg, CtlSelPrc, PPOBJ_PROCESSOR, Data.PrcID, OLW_CANSELUPLEVEL, 0);
	return 1;
}

int PrcCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	pDlg->getCtrlData(CtlSelPrc, &Data.PrcID);
	if(pData)
		*static_cast<Rec *>(pData) = Data;
	return 1;
}
//
//
//
class ProcessorDialog : public TDialog {
public:
	enum {
		ctlgroupGoods = 1
	};
	explicit ProcessorDialog(uint dlgID) : TDialog(dlgID), InheritedFlags(0)
	{
	}
	int    setDTS(const PPProcessorPacket *);
	int    getDTS(PPProcessorPacket *);
private:
	DECL_HANDLE_EVENT;
	void   setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID);
	int    setupParent();
	int    setupAssoc();
	void   setupLinkName(int force);
	PPID   groupObjType() const { return (Data.Rec.LinkObjType == PPOBJ_PERSON) ? PPOBJ_PERSONKIND : 0; }
	int    EditExt();

	PPProcessorPacket Data;
	PPObjProcessor PrcObj;
	long   InheritedFlags;
};

/*static*/int PPObjProcessor::EditPrcPlaceItem(PPProcessorPacket::PlaceDescription * pItem)
{
	//#define GRP_GOODS 1
    int    ok = -1;
    uint   sel = 0;
    ProcessorPlaceCodeTemplate ppct;
    TDialog * dlg = new TDialog(DLG_PRCPLACE);
    THROW(CheckDialogPtrErr(&dlg));
	dlg->addGroup(ProcessorDialog::ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_PRCPLACE_GGRP, CTLSEL_PRCPLACE_GOODS));
	{
		GoodsCtrlGroup::Rec grp_rec(0, pItem->GoodsID, GoodsCtrlGroup::disableEmptyGoods|GoodsCtrlGroup::enableInsertGoods);
		dlg->setGroupData(ProcessorDialog::ctlgroupGoods, &grp_rec);
	}
    dlg->setCtrlString(CTL_PRCPLACE_PLACES, pItem->Range);
    dlg->setCtrlString(CTL_PRCPLACE_DESCR, pItem->Descr);
    while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(sel = CTL_PRCPLACE_PLACES, pItem->Range);
        if(!pItem->Range.NotEmptyS()) {
			PPErrorByDialog(dlg, sel, PPERR_USERINPUT);
        }
        else if(!ppct.Parse(pItem->Range)) {
            PPErrorByDialog(dlg, sel);
        }
        else {
			sel = CTL_PRCPLACE_GOODS;
			GoodsCtrlGroup::Rec grp_rec;
			dlg->getGroupData(ProcessorDialog::ctlgroupGoods, &grp_rec);
			pItem->GoodsID = grp_rec.GoodsID;
			if(!pItem->GoodsID) {
				PPErrorByDialog(dlg, sel, PPERR_GOODSNOTSEL);
			}
			else {
				dlg->getCtrlString(CTL_PRCPLACE_DESCR, pItem->Descr);
				{
					// @debug
					/*
					StringSet ss;
                    ppct.Generate(ss);
                    SString temp_buf;
                    for(uint p = 0; ss.get(&p, temp_buf);) {
						if(ppct.HasCode(temp_buf))
							temp_buf.Space().Cat("OK");
						else
							temp_buf.Space().Cat("ERR");
						PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, 0);
                    }
					assert(ppct.HasCode("101D08Z") == 0);
					*/
				}
				ok = 1;
			}
		}
    }
    CATCHZOK
    delete dlg;
    return ok;
    //#undef GRP_GOODS
}

int ProcessorDialog::EditExt()
{
	class PrcExtDialog : public PPListDialog {
		DECL_DIALOG_DATA(PPProcessorPacket);
	public:
		PrcExtDialog() : PPListDialog(DLG_PRCEXT, CTL_PRCEXT_PLACELIST)
		{
			SetupTimePicker(this, CTL_PRCEXT_CHKINTIME, CTLTM_PRCEXT_CHKINTIME);
			SetupTimePicker(this, CTL_PRCEXT_CHKOUTTIME, CTLTM_PRCEXT_CHKOUTTIME);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			// round direction: 1 - forward, 2 - backward
			long   check_in_round  = (Data.Ext.TimeFlags & Data.Ext.tfCheckInRoundForward) ? 1 : 2;
			long   check_out_round = (Data.Ext.TimeFlags & Data.Ext.tfCheckOutRoundBackward) ? 2 : 1;
			setCtrlTime(CTL_PRCEXT_CHKINTIME, Data.Ext.CheckInTime);
			setCtrlTime(CTL_PRCEXT_CHKOUTTIME, Data.Ext.CheckOutTime);
			AddClusterAssocDef(CTL_PRCEXT_CHKINROUND,  0, 2);
			AddClusterAssoc(CTL_PRCEXT_CHKINROUND,  1, 1);
			SetClusterData(CTL_PRCEXT_CHKINROUND, check_in_round);

			AddClusterAssocDef(CTL_PRCEXT_CHKOUTROUND,  0, 1);
			AddClusterAssoc(CTL_PRCEXT_CHKOUTROUND,  1, 2);
			SetClusterData(CTL_PRCEXT_CHKOUTROUND, check_out_round);

			AddClusterAssoc(CTL_PRCEXT_PLUSONE, 0, Data.Ext.tfPlusOneDay);
			SetClusterData(CTL_PRCEXT_PLUSONE, Data.Ext.TimeFlags);
			AddClusterAssocDef(CTL_PRCEXT_INITSTATUS, 0, 0);
			AddClusterAssoc(CTL_PRCEXT_INITSTATUS, 1, TSESST_PLANNED);
			AddClusterAssoc(CTL_PRCEXT_INITSTATUS, 2, TSESST_PENDING);
			AddClusterAssoc(CTL_PRCEXT_INITSTATUS, 3, TSESST_INPROCESS);
			AddClusterAssoc(CTL_PRCEXT_INITSTATUS, 4, TSESST_CLOSED);
			AddClusterAssoc(CTL_PRCEXT_INITSTATUS, 5, TSESST_CANCELED);
			SetClusterData(CTL_PRCEXT_INITSTATUS, Data.Ext.InitSessStatus);
			SetupPPObjCombo(this, CTLSEL_PRCEXT_GUA, PPOBJ_GLOBALUSERACC, Data.Ext.GetOwnerGuaID(), 0, 0);
			{
				long   cipcto_min = labs(Data.Ext.GetCipCancelTimeout()) / 60;
				setCtrlLong(CTL_PRCEXT_CIPCTO, cipcto_min);
			}
			{
				long   ciplto_min = labs(Data.Ext.GetCipLockTimeout()) / 60;
				setCtrlLong(CTL_PRCEXT_CIPLTO, ciplto_min);
			}
			updateList(-1);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Data.Ext.CheckInTime  = getCtrlTime(CTL_PRCEXT_CHKINTIME);
			Data.Ext.CheckOutTime = getCtrlTime(CTL_PRCEXT_CHKOUTTIME);
			long   check_in_round = GetClusterData(CTL_PRCEXT_CHKINROUND);
			long   check_out_round = GetClusterData(CTL_PRCEXT_CHKOUTROUND);
			SETFLAG(Data.Ext.TimeFlags, Data.Ext.tfCheckInRoundForward,   check_in_round == 1);
			SETFLAG(Data.Ext.TimeFlags, Data.Ext.tfCheckOutRoundBackward, check_out_round == 2);
			GetClusterData(CTL_PRCEXT_PLUSONE, &Data.Ext.TimeFlags);
			GetClusterData(CTL_PRCEXT_INITSTATUS, &Data.Ext.InitSessStatus);
			PPID   owner_gua_id = getCtrlLong(CTLSEL_PRCEXT_GUA);
			Data.Ext.SetOwnerGuaID(owner_gua_id);
			{
				long   cipcto_min = getCtrlLong(CTL_PRCEXT_CIPCTO);
                long   cipcto = labs(cipcto_min * 60);
				Data.Ext.SetCipCancelTimeout(cipcto);
			}
			{
				long   ciplto_min = getCtrlLong(CTL_PRCEXT_CIPLTO);
                long   ciplto = labs(ciplto_min * 60);
				Data.Ext.SetCipLockTimeout(ciplto);
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		virtual int setupList()
		{
			int    ok = 1;
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			PPProcessorPacket::PlaceDescription item;
			for(uint i = 0; i < Data.Ext.GetPlaceDescriptionCount(); i++) {
				if(!Data.Ext.GetPlaceDescription(i, item)) {
					item.Z();
					item.Range = "#ERROR";
				}
				ss.clear();
				ss.add(item.Range);
				ss.add(GetGoodsName(item.GoodsID, temp_buf.Z()));
				ss.add(item.Descr);
				THROW(addStringToList(i+1, ss.getBuf()));
			}
			CATCHZOKPPERR
			return 1;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int    ok = -1;
			PPProcessorPacket::PlaceDescription new_item;
			if(PPObjProcessor::EditPrcPlaceItem(&new_item) > 0) {
				uint   new_pos = Data.Ext.GetPlaceDescriptionCount();
				if(!Data.Ext.PutPlaceDescription(new_pos, &new_item))
					ok = PPErrorZ();
				else {
					ASSIGN_PTR(pPos, new_pos);
					ASSIGN_PTR(pID, new_pos+1);
					ok = 1;
				}
			}
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			const  long c = static_cast<long>(Data.Ext.GetPlaceDescriptionCount());
			if(pos < c) {
				PPProcessorPacket::PlaceDescription item;
				if(Data.Ext.GetPlaceDescription(pos, item)) {
					if(PPObjProcessor::EditPrcPlaceItem(&item) > 0) {
						if(!Data.Ext.PutPlaceDescription(pos, &item))
							ok = PPErrorZ();
						else
							ok = 1;
					}
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			const  long c = static_cast<long>(Data.Ext.GetPlaceDescriptionCount());
			if(pos < c) {
				Data.Ext.PutPlaceDescription(pos, 0);
				ok = 1;
			}
			return ok;
		}
	};
	DIALOG_PROC_BODY(PrcExtDialog, &Data);
}

int ProcessorDialog::setupParent()
{
	int    ok = 1;
	PPID   parent_id = getCtrlLong(CTLSEL_PRC_PARENT);
	ProcessorTbl::Rec grp_rec;
	InheritedFlags = 0;
	if(PrcObj.GetRecWithInheritance(parent_id, &grp_rec, 1) > 0) {
		if(grp_rec.Flags & PRCF_ALLOWCIP && !(Data.Rec.Flags & PRCF_ALLOWCIP))
			InheritedFlags |= PRCF_ALLOWCIP;
		if(grp_rec.LocID) {
			if(getCtrlLong(CTLSEL_PRC_LOC) == 0)
				setCtrlLong(CTLSEL_PRC_LOC, grp_rec.LocID);
		}
		if(Data.Rec.Kind == PPPRCK_PROCESSOR) {
			PPID   prev_link_obj_type = Data.Rec.LinkObjType;
			if(grp_rec.LinkObjType) {
				long   link_extra = 0;
				if(grp_rec.LinkObjType == PPOBJ_TRANSPORT)
					link_extra = PPTRTYP_CAR;
				else
					link_extra = grp_rec.LinkObjID;
				SString obj_title;
				GetObjectTitle(grp_rec.LinkObjType, obj_title);
				setLabelText(CTL_PRC_LINKOBJ, obj_title);
				SetupPPObjCombo(this, CTLSEL_PRC_LINKOBJ, grp_rec.LinkObjType, Data.Rec.LinkObjID, OLW_CANINSERT, reinterpret_cast<void *>(link_extra));
			}
			else
				setLabelText(CTL_PRC_LINKOBJ, 0);
			disableCtrl(CTLSEL_PRC_LINKOBJ, !grp_rec.LinkObjType);
			// @v9.3.2 disableCtrl(CTL_PRC_NAME, BIN(grp_rec.LinkObjType));
			if(prev_link_obj_type != grp_rec.LinkObjType)
				setCtrlLong(CTLSEL_PRC_LINKOBJ, 0);
		}
		Data.Rec.LinkObjType = grp_rec.LinkObjType;
	}
	if(InheritedFlags & PRCF_ALLOWCIP) {
		Data.Rec.Flags |= PRCF_ALLOWCIP;
		DisableClusterItem((Data.Rec.Kind == PPPRCK_PROCESSOR) ? CTL_PRC_FLAGS : CTL_PRC_GRP_FLAGS, (Data.Rec.Kind == PPPRCK_PROCESSOR) ? 6 : 8, 1);
	}
	else {
		DisableClusterItem((Data.Rec.Kind == PPPRCK_PROCESSOR) ? CTL_PRC_FLAGS : CTL_PRC_GRP_FLAGS, (Data.Rec.Kind == PPPRCK_PROCESSOR) ? 6 : 8, 0);
	}
	return ok;
}

IMPL_HANDLE_EVENT(ProcessorDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_PRC_WROFFOP))
		setupAccSheet(CTLSEL_PRC_WROFFOP, CTLSEL_PRC_WROFFAR, 0);
	else if(event.isCbSelected(CTLSEL_PRC_PARENT)) {
		setupParent();
	}
	else if(event.isCbSelected(CTLSEL_PRC_LINKOBJ)) {
		if(Data.Rec.Kind == PPPRCK_PROCESSOR) {
			const PPID prev_link_id = Data.Rec.LinkObjID;
			Data.Rec.LinkObjID = getCtrlLong(CTLSEL_PRC_LINKOBJ);
			setupLinkName(prev_link_id != Data.Rec.LinkObjID);
		}
	}
	else if(event.isClusterClk(CTL_PRC_GRP_FLAGS)) {
		GetClusterData(CTL_PRC_GRP_FLAGS, &Data.Rec.Flags);
		disableCtrl(CTLSEL_PRC_RESTGGRP, !(Data.Rec.Flags & PRCF_STOREGOODSREST));
	}
	else if(event.isClusterClk(CTL_PRC_FLAGS)) {
		GetClusterData(CTL_PRC_FLAGS, &Data.Rec.Flags);
		disableCtrl(CTL_PRC_CIPMAX, !(Data.Rec.Flags & PRCF_ALLOWCIP));
		disableCtrl(CTLSEL_PRC_CIPKIND, !(Data.Rec.Flags & PRCF_ALLOWCIP));
	}
	else if(event.isClusterClk(CTL_PRC_ASSOC)) {
		if(Data.Rec.Kind == PPPRCK_GROUP) {
			PPID   old_assoc = Data.Rec.LinkObjType;
			GetClusterData(CTL_PRC_ASSOC, &Data.Rec.LinkObjType);
			if(Data.Rec.LinkObjType != old_assoc) {
				Data.Rec.LinkObjID = 0;
				setupAssoc();
			}
		}
	}
	else if(event.isCmd(cmPrcExt)) {
		EditExt();
	}
	else
		return;
	clearEvent(event);
}

void ProcessorDialog::setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID)
{
	PPOprKind op_rec;
	GetOpData(getCtrlLong(opSelCtl), &op_rec);
	SetupArCombo(this, objSelCtl, arID, OLW_LOADDEFONOPEN, op_rec.AccSheetID, sacfDisableIfZeroSheet|sacfNonGeneric);
	DisableClusterItem(CTL_PRC_GRP_FLAGS, 6, BIN(op_rec.AccSheet2ID));
}

int ProcessorDialog::setupAssoc()
{
	if(Data.Rec.Kind == PPPRCK_GROUP) {
		AddClusterAssocDef(CTL_PRC_ASSOC,  0, 0);
		AddClusterAssoc(CTL_PRC_ASSOC,  1, PPOBJ_PERSON);
		AddClusterAssoc(CTL_PRC_ASSOC,  2, PPOBJ_TRANSPORT);
		SetClusterData(CTL_PRC_ASSOC, Data.Rec.LinkObjType);
		disableCtrl(CTLSEL_PRC_ASSOCGROUP, !(Data.Rec.LinkObjType == PPOBJ_PERSON));
		if(Data.Rec.LinkObjType == PPOBJ_PERSON)
			SetupPPObjCombo(this, CTLSEL_PRC_ASSOCGROUP, groupObjType(), Data.Rec.LinkObjID, OLW_CANINSERT, 0);
		else
			setCtrlLong(CTL_PRC_ASSOCGROUP, 0);
	}
	return 1;
}

void ProcessorDialog::setupLinkName(int force)
{
	if(Data.Rec.LinkObjType && Data.Rec.LinkObjID) {
		SString link_obj_name;
		SString prev_name;
		getCtrlString(CTL_PRC_NAME, prev_name);
		if(GetObjectName(Data.Rec.LinkObjType, Data.Rec.LinkObjID, link_obj_name) > 0) {
			if(force || !prev_name.NotEmptyS())
				setCtrlString(CTL_PRC_NAME, link_obj_name);
		}
	}
}

int ProcessorDialog::setDTS(const PPProcessorPacket * pData)
{
	RVALUEPTR(Data, pData);
	setCtrlLong(CTL_PRC_ID, Data.Rec.ID);
	disableCtrl(CTL_PRC_ID, 1);
	setCtrlData(CTL_PRC_NAME, Data.Rec.Name);
	setCtrlData(CTL_PRC_SYMB, Data.Rec.Code);
	SetupPPObjCombo(this, CTLSEL_PRC_PARENT, PPOBJ_PROCESSOR, Data.Rec.ParentID, OLW_CANINSERT|OLW_CANSELUPLEVEL, reinterpret_cast<void *>(PRCEXDF_GROUP));
	SetupPPObjCombo(this, CTLSEL_PRC_LOC, PPOBJ_LOCATION, Data.Rec.LocID, 0);
	SetupPPObjCombo(this, CTLSEL_PRC_RESTGGRP, PPOBJ_GOODSGROUP, Data.Rec.RestAltGrpID, OLW_CANINSERT, reinterpret_cast<void *>(GGRTYP_SEL_ALT));
	setupParent();
	if(Data.Rec.Kind == PPPRCK_PROCESSOR) {
		SetupPPObjCombo(this, CTLSEL_PRC_PRINTER, PPOBJ_BCODEPRINTER, Data.Rec.PrinterID, OLW_CANINSERT, 0);
		AddClusterAssoc(CTL_PRC_PANE_FLAGS, 0, PRCF_PRINTNEWLINE_PANE);
		AddClusterAssoc(CTL_PRC_PANE_FLAGS, 1, PRCF_ONECLICKTURN_PANE);
		SetClusterData(CTL_PRC_PANE_FLAGS, Data.Rec.Flags);
		AddClusterAssoc(CTL_PRC_FLAGS, 0, PRCF_LOCKWROFF);
		AddClusterAssoc(CTL_PRC_FLAGS, 1, PRCF_TURNINCOMPLBILL);
		AddClusterAssoc(CTL_PRC_FLAGS, 2, PRCF_PASSIVE);
		AddClusterAssoc(CTL_PRC_FLAGS, 3, PRCF_CLOSEBYJOBSRV);
		AddClusterAssoc(CTL_PRC_FLAGS, 4, PRCF_USETSESSSIMPLEDLG);
		AddClusterAssoc(CTL_PRC_FLAGS, 5, PRCF_NEEDCCHECK);
		AddClusterAssoc(CTL_PRC_FLAGS, 6, PRCF_ALLOWCIP);
		AddClusterAssoc(CTL_PRC_FLAGS, 7, PRCF_ALLOWCANCELAFTERCLOSE);
		AddClusterAssoc(CTL_PRC_FLAGS, 8, PRCF_ALLOWREPEATING); // @v11.0.4
		SetClusterData(CTL_PRC_FLAGS, Data.Rec.Flags);
		setCtrlData(CTL_PRC_LABELCOUNT, &Data.Rec.LabelCount);
		setCtrlData(CTL_PRC_CIPMAX, &Data.Rec.CipMax);
		setupLinkName(0);
	}
	else {
		long   wr_off_dt_sel = (Data.Rec.Flags & PRCF_WROFFDT_START) ? 0 : 1;
		AddClusterAssoc(CTL_PRC_WROFFDT,  0, 0);
		AddClusterAssocDef(CTL_PRC_WROFFDT,  1, 1);
		SetClusterData(CTL_PRC_WROFFDT, wr_off_dt_sel);
		AddClusterAssoc(CTL_PRC_WROFFDT_BYSUPER, 0, PRCF_WROFFDT_BYSUPER);
		SetClusterData(CTL_PRC_WROFFDT_BYSUPER, Data.Rec.Flags);
		setupAssoc();
		if(Data.Rec.LinkObjType && Data.Rec.ID) {
			if(PrcObj.GetChildIDList(Data.Rec.ID, 1, 0) > 0)
				disableCtrls(1, CTL_PRC_ASSOC, CTLSEL_PRC_ASSOCGROUP, 0);
		}
	}
	setCtrlData(CTL_PRC_SRVJOBSYMB, Data.Rec.SrvJobSymb);
	PPIDArray op_types;
	op_types.addzlist(PPOPT_GOODSMODIF, PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, 0);
	SetupOprKindCombo(this, CTLSEL_PRC_WROFFOP, Data.Rec.WrOffOpID, 0, &op_types, 0);
	setupAccSheet(CTLSEL_PRC_WROFFOP, CTLSEL_PRC_WROFFAR, Data.Rec.WrOffArID);
	SetupPPObjCombo(this, CTLSEL_PRC_CIPKIND, PPOBJ_PERSONKIND, Data.Rec.CipPersonKindID, 0, 0);

	LTIME  tm;
	tm.settotalsec(Data.Rec.SuperSessTiming);
	setCtrlData(CTL_PRC_SUPERSESSCONT, &tm);

	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  0, PRCF_INDUCTSUPERSESSTATUS);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  1, PRCF_STOREGOODSREST);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  2, PRCF_LOCKWROFF);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  3, PRCF_CANSWITCHPAN);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  4, PRCF_ACCDUPSERIALINSESS);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  5, PRCF_TURNINCOMPLBILL);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  6, PRCF_ADDEDOBJASAGENT);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  7, PRCF_CLOSEBYJOBSRV);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  8, PRCF_ALLOWCIP);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS,  9, PRCF_USETSESSSIMPLEDLG);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS, 10, PRCF_AUTOCREATE);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS, 11, PRCF_ALLOWCANCELAFTERCLOSE);
	AddClusterAssoc(CTL_PRC_GRP_FLAGS, 12, PRCF_ALLOWREPEATING); // @v11.0.4
	SetClusterData(CTL_PRC_GRP_FLAGS, Data.Rec.Flags);
	{
		long  tcbquant = Data.Rec.TcbQuant * 5;
		setCtrlLong(CTL_PRC_TCBQUANT, tcbquant);
	}
	disableCtrl(CTLSEL_PRC_RESTGGRP, !(Data.Rec.Flags & PRCF_STOREGOODSREST));
	disableCtrl(CTL_PRC_CIPMAX, !(Data.Rec.Flags & PRCF_ALLOWCIP));
	disableCtrl(CTLSEL_PRC_CIPKIND, !(Data.Rec.Flags & PRCF_ALLOWCIP));
	return 1;
}

int ProcessorDialog::getDTS(PPProcessorPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	LTIME  tm = ZEROTIME;
	getCtrlData(sel = CTL_PRC_NAME, Data.Rec.Name);
	THROW_PP(*strip(Data.Rec.Name), PPERR_NAMENEEDED);
	getCtrlData(CTL_PRC_SYMB, Data.Rec.Code);
	getCtrlData(CTLSEL_PRC_PARENT, &Data.Rec.ParentID);
	THROW_PP_S(!Data.Rec.ParentID || Data.Rec.ParentID != Data.Rec.ID, PPERR_PROCESSORSELFPAR, Data.Rec.Name); // @v7.7.7
	getCtrlData(sel = CTLSEL_PRC_LOC, &Data.Rec.LocID);
	THROW_PP(Data.Rec.LocID, PPERR_LOCNEEDED);
	getCtrlData(CTLSEL_PRC_WROFFOP, &Data.Rec.WrOffOpID);
	getCtrlData(CTLSEL_PRC_WROFFAR, &Data.Rec.WrOffArID);

	getCtrlData(CTL_PRC_SUPERSESSCONT, &tm);
	Data.Rec.SuperSessTiming = tm.totalsec();
	GetClusterData(CTL_PRC_GRP_FLAGS, &Data.Rec.Flags);
	getCtrlData(CTLSEL_PRC_RESTGGRP, &Data.Rec.RestAltGrpID);
	if(Data.Rec.Kind == PPPRCK_PROCESSOR) {
		getCtrlData(CTLSEL_PRC_PRINTER, &Data.Rec.PrinterID);
		GetClusterData(CTL_PRC_PANE_FLAGS, &Data.Rec.Flags);
		GetClusterData(CTL_PRC_FLAGS,      &Data.Rec.Flags);
		getCtrlData(CTL_PRC_LABELCOUNT, &Data.Rec.LabelCount);
		getCtrlData(CTL_PRC_CIPMAX, &Data.Rec.CipMax); // @v7.7.2
		sel = CTL_PRC_LINKOBJ;
		THROW_PP(!Data.Rec.LinkObjType || Data.Rec.LinkObjID, PPERR_LINKOBJNEEDED);
	}
	else {
		long   wr_off_dt_sel = 0;
		GetClusterData(CTL_PRC_WROFFDT, &wr_off_dt_sel);
		SETFLAG(Data.Rec.Flags, PRCF_WROFFDT_START, wr_off_dt_sel == 0);
		GetClusterData(CTL_PRC_WROFFDT_BYSUPER, &Data.Rec.Flags);
		GetClusterData(CTL_PRC_ASSOC, &Data.Rec.LinkObjType);
		if(Data.Rec.LinkObjType)
			if(Data.Rec.LinkObjType == PPOBJ_PERSON)
				Data.Rec.LinkObjID = getCtrlLong(CTLSEL_PRC_ASSOCGROUP);
			else
				Data.Rec.LinkObjID = 0;
		else
			Data.Rec.LinkObjID = 0;
	}
	getCtrlData(CTL_PRC_SRVJOBSYMB, Data.Rec.SrvJobSymb);
	getCtrlData(CTLSEL_PRC_CIPKIND, &Data.Rec.CipPersonKindID);
	if(InheritedFlags & PRCF_ALLOWCIP)
		Data.Rec.Flags &= ~PRCF_ALLOWCIP;
	{
		long  tcbquant = 0;
		if(getCtrlData(CTL_PRC_TCBQUANT, &tcbquant))
			Data.Rec.TcbQuant = (int16)(tcbquant / 5);
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int PPObjProcessor::Edit(PPID * pID, void * extraPtr /*parentID*/)
{
	const  PPID extra_parent_id = reinterpret_cast<PPID>(extraPtr);
	int    ok = cmCancel;
	int    valid_data = 0;
	uint   dlg_id = 0;
	ProcessorDialog * dlg = 0;
	PPProcessorPacket pack;
	ProcessorTbl::Rec grp_rec;
	if(*pID) {
		THROW(GetPacket(*pID, &pack) > 0);
		if(pack.Rec.Kind == PPPRCK_PROCESSOR) {
			if(pack.Rec.ParentID && GetRecWithInheritance(pack.Rec.ParentID, &grp_rec, 1) > 0) {
				if(grp_rec.LinkObjType && pack.Rec.LinkObjType == 0)
					pack.Rec.LinkObjType = grp_rec.LinkObjType;
			}
		}
	}
	else {
		if(extra_parent_id & PRCEXDF_GROUP)
			pack.Rec.Kind = PPPRCK_GROUP;
		else {
			pack.Rec.Kind = PPPRCK_PROCESSOR;
			pack.Rec.ParentID = (extra_parent_id & ~PRCEXDF_GROUP);
			if(pack.Rec.ParentID) {
				THROW(GetRecWithInheritance(pack.Rec.ParentID, &grp_rec, 1) > 0);
				pack.Rec.LinkObjType = grp_rec.LinkObjType;
				pack.Rec.LocID = grp_rec.LocID;
			}
			if(!pack.Rec.LocID) {
				PPObjLocation loc_obj;
				PPIDArray wh_list;
				loc_obj.GetWarehouseList(&wh_list, 0);
				if(wh_list.getCount() == 1)
					pack.Rec.LocID = wh_list.get(0);
			}
		}
	}
	if(pack.Rec.Kind == PPPRCK_GROUP)
		dlg_id = DLG_PRCGROUP;
	else if(pack.Rec.Kind == PPPRCK_PROCESSOR)
		dlg_id = DLG_PRC;
	else {
		char   kind_buf[32];
		CALLEXCEPT_PP_S(PPERR_INVPRCKIND, ltoa(pack.Rec.Kind, kind_buf, 10));
	}
	THROW(CheckDialogPtr(&(dlg = new ProcessorDialog(dlg_id))));
	dlg->setDTS(&pack);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&pack)) {
			ProcessorTbl::Rec _rec;
			if(strip(pack.Rec.Code)[0] && SearchByCode(pack.Rec.Code, 0, &_rec) > 0 && _rec.ID != pack.Rec.ID) {
				PPObject::SetLastErrObj(Obj, _rec.ID);
				PPError(PPERR_DUPSYMB, 0);
			}
			else if(PutPacket(pID, &pack, 1))
				ok = valid_data = cmOK;
			else
				PPError();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjProcessor::AddListItem(StrAssocArray * pList, ProcessorTbl::Rec * pRec, PPIDArray * pRecurTrace)
{
	int    ok = 1, r;
	PPIDArray local_recur_trace;
	if(pList->Search(pRec->ID))
		ok = -1;
	else {
		PPID   par_id = pRec->ParentID;
		ProcessorTbl::Rec par_rec;
		if(par_id && par_id == pRec->ID) {
			PPSetError(PPERR_PROCESSORSELFPAR, pRec->Name);
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
			par_id = 0;
		}
		else {
			if(par_id && Fetch(par_id, &par_rec) > 0) {
				SETIFZ(pRecurTrace, &local_recur_trace);
				THROW(r = pRecurTrace->addUnique(par_id));
				if(r > 0) {
					THROW(AddListItem(pList, &par_rec, pRecurTrace)); // @recursion
				}
				else {
					PPSetError(PPERR_PROCESSORRECUR, pRec->Name);
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_LASTERR);
					par_id = 0;
				}
			}
		}
		THROW_SL(pList->Add(pRec->ID, par_id, pRec->Name));
	}
	CATCHZOK
	return ok;
}

StrAssocArray * PPObjProcessor::MakeStrAssocList(void * extraPtr /*parentID*/)
{
	const PPID outer_parent_id = reinterpret_cast<PPID>(extraPtr);
	union {
		ProcessorTbl::Key1 k1;
		ProcessorTbl::Key2 k2;
	} k;
	const  PPID  parent_id = (outer_parent_id & ~PRCEXDF_GROUP);
	const  long  kind = (outer_parent_id & PRCEXDF_GROUP) ? PPPRCK_GROUP : PPPRCK_PROCESSOR;
	int    idx = parent_id ? 1 : 2;
	StrAssocArray * p_list = new StrAssocArray;
	ProcessorTbl * p_t = P_Tbl;
	DBQ  * dbq = 0;
	BExtQuery q(p_t, idx);

	THROW_MEM(p_list);
	dbq = &(*dbq && p_t->Kind == kind);
	dbq = ppcheckfiltid(dbq, p_t->ParentID, parent_id);
	q.select(p_t->ID, p_t->ParentID, p_t->Name, p_t->Flags, 0L).where(*dbq);
	MEMSZERO(k);
	k.k1.Kind = kind;
	if(parent_id)
		k.k1.ParentID = parent_id;
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
		ProcessorTbl::Rec rec;
		p_t->copyBufTo(&rec);
		if(!(rec.Flags & PRCF_PASSIVE)) {
			//THROW_SL(p_list->Add(rec.ID, rec.Name));
			THROW(AddListItem(p_list, &rec, 0));
		}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	if(p_list) {
		LongArray recur_bad_array;
		p_list->RemoveRecursion(&recur_bad_array);
	}
	return p_list;
}

int PPObjProcessor::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_PROCESSOR, 0, 1, extraPtr); }
//
//
//
IMPL_DESTROY_OBJ_PACK(PPObjProcessor, PPProcessorPacket);

int PPObjProcessor::SerializePacket(int dir, PPProcessorPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString stub_extsting;
	THROW_SL(P_Tbl->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->Ext.CheckInTime, rBuf));
	THROW_SL(pSCtx->Serialize(dir, pPack->Ext.CheckOutTime, rBuf));
	THROW_SL(pSCtx->Serialize(dir, pPack->Ext.TimeFlags, rBuf));
	THROW_SL(pSCtx->Serialize(dir, stub_extsting, rBuf));
	CATCHZOK
	return ok;
}

int PPObjProcessor::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjProcessor, PPProcessorPacket>(this, p, id, stream, pCtx); }

int  PPObjProcessor::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPProcessorPacket * p_pack = p ? static_cast<PPProcessorPacket *>(p->Data) : 0;
	if(p_pack) {
		if(stream == 0) {
			PPID   same_id = 0;
			ProcessorTbl::Rec same_rec;
			if(*pID == 0 && SearchByName(p_pack->Rec.Kind, p_pack->Rec.Name, &same_id, &same_rec) > 0) {
				*pID = same_id;
			}
			else if(*pID && Search(*pID, &same_rec) > 0) {
				same_id = *pID;
			}
			if(same_id) {
				p_pack->Rec.RestAltGrpID = same_rec.RestAltGrpID;
				p_pack->Rec.PrinterID = same_rec.PrinterID;
				p_pack->Rec.WrOffGenOpID = same_rec.WrOffGenOpID;
			}
			else {
				p_pack->Rec.RestAltGrpID = 0;
				p_pack->Rec.PrinterID = 0;
				p_pack->Rec.WrOffGenOpID = 0;
			}
			const int    is_new = BIN(*pID == 0);
			int     r = PutPacket(pID, p_pack, 1);
			if(!r) {
				pCtx->OutputAcceptObjErrMsg(Obj, p_pack->Rec.ID, p_pack->Rec.Name);
				ok = -1;
			}
			else if(r > 0)
				ok = is_new ? 101 : 102; // @ObjectCreated : @ObjectUpdated
		}
		else {
			SBuffer buffer;
			THROW_SL(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int  PPObjProcessor::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = 1;
	if(p && p->Data) {
		PPProcessorPacket * p_pack = static_cast<PPProcessorPacket *>(p->Data);
		THROW(ProcessObjRefInArray(PPOBJ_PROCESSOR, &p_pack->Rec.ParentID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_LOCATION, &p_pack->Rec.LocID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_UNIT, &p_pack->Rec.TimeUnitID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_OPRKIND, &p_pack->Rec.WrOffOpID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_ARTICLE, &p_pack->Rec.WrOffArID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_PERSONKIND, &p_pack->Rec.CipPersonKindID, ary, replace));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}
//
// @ModuleDef(PPViewProcessor)
//
IMPLEMENT_PPFILT_FACTORY(Processor); ProcessorFilt::ProcessorFilt() : PPBaseFilt(PPFILT_PROCESSOR, 0, 0)
{
	SetFlatChunk(offsetof(ProcessorFilt, ReserveStart),
		offsetof(ProcessorFilt, Reserve)-offsetof(ProcessorFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

int ProcessorFilt::Init(int fullyDestroy, long extraData)
{
	PPBaseFilt::Init(fullyDestroy, extraData);
	Kind = PPPRCK_PROCESSOR;
	return 1;
}

PPViewProcessor::PPViewProcessor() : PPView(&PrcObj, &Filt, PPVIEW_PROCESSOR, implDontEditNullFilter, REPORT_PROCESSOR)
{
}

PPBaseFilt * PPViewProcessor::CreateFilt(void * extraPtr) const
{
	ProcessorFilt * p_filt = new ProcessorFilt;
	if(extraPtr) {
		p_filt->Kind = ((reinterpret_cast<long>(extraPtr)) & PRCEXDF_GROUP) ? PPPRCK_GROUP : PPPRCK_PROCESSOR;
		p_filt->ParentID = (reinterpret_cast<long>(extraPtr)) & ~PRCEXDF_GROUP;
	}
	return p_filt;
}

int PPViewProcessor::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(Filt.IsA(pBaseFilt)) {
		ProcessorFilt * p_filt = static_cast<ProcessorFilt *>(pBaseFilt);
		if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_PRCFILT)))) {
			dlg->AddClusterAssoc(CTL_PRCFILT_KIND, 0, PPPRCK_GROUP);
			dlg->AddClusterAssoc(CTL_PRCFILT_KIND, 1, PPPRCK_PROCESSOR);
			dlg->SetClusterData(CTL_PRCFILT_KIND, p_filt->Kind);
			SetupPPObjCombo(dlg, CTLSEL_PRCFILT_PARENT, PPOBJ_PROCESSOR, p_filt->ParentID, OLW_CANINSERT|OLW_CANSELUPLEVEL,
				reinterpret_cast<void *>(PRCEXDF_GROUP));
			SetupPPObjCombo(dlg, CTLSEL_PRCFILT_LOC, PPOBJ_LOCATION, p_filt->LocID, 0);
			while(ok < 0 && ExecView(dlg) == cmOK) {
				dlg->GetClusterData(CTL_PRCFILT_KIND, &p_filt->Kind);
				dlg->getCtrlData(CTLSEL_PRCFILT_PARENT, &p_filt->ParentID);
				dlg->getCtrlData(CTLSEL_PRCFILT_LOC, &p_filt->LocID);
				ok = 1;
			}
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewProcessor::Init_(const PPBaseFilt * pFilt)
{
	Counter.Init();
	BExtQuery::ZDelete(&P_IterQuery);
	return BIN(Helper_InitBaseFilt(pFilt));
}

int PPViewProcessor::InitIteration()
{
	int    ok = 1, idx = 0;
	union {
		ProcessorTbl::Key1 k1;
		ProcessorTbl::Key2 k2;
	} k, k_;
	DBQ *  dbq = 0;
	ProcessorTbl * t = PrcObj.P_Tbl;
	BExtQuery::ZDelete(&P_IterQuery);
	if(Filt.ParentID)
		idx = 1;
	else
		idx = 2;
	P_IterQuery = new BExtQuery(t, idx);
	dbq = &(*dbq && t->Kind == Filt.Kind);
	dbq = ppcheckfiltid(dbq, t->ParentID, Filt.ParentID);
	dbq = ppcheckfiltid(dbq, t->LocID, Filt.LocID);
	P_IterQuery->selectAll().where(*dbq);
	MEMSZERO(k);
	k.k1.Kind = Filt.Kind;
	if(Filt.ParentID)
		k.k1.ParentID = Filt.ParentID;
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
	P_IterQuery->initIteration(0, &k, spGe);
	return ok;
}

int FASTCALL PPViewProcessor::NextIteration(ProcessorViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		PrcObj.P_Tbl->copyBufTo(pItem);
		Counter.Increment();
		return 1;
	}
	else
		return -1;
}

void * PPViewProcessor::GetEditExtraParam()
{
	return (Filt.Kind == PPPRCK_GROUP) ? reinterpret_cast<void *>(Filt.ParentID | PRCEXDF_GROUP) : reinterpret_cast<void *>(Filt.ParentID);
}

int PPViewProcessor::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	ProcessorTbl::Rec rec;
	PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	if(PrcObj.Search(id, &rec) > 0 && rec.Kind == PPPRCK_GROUP) {
		ProcessorFilt filt;
		filt.Kind = 0; //PPPRCK_PROCESSOR;
		filt.ParentID = id;
		PPView::Execute(PPVIEW_PROCESSOR, &filt, 1, 0);
	}
	return -1;
}

int PPViewProcessor::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		ProcessorViewItem item;
		const  PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_PROCESSOR, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		PPWaitStop();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

DBQuery * PPViewProcessor::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_PROCESSOR;
	ProcessorTbl * p_prct = 0;
	DBQuery * q  = 0;
	DBQ  * dbq = 0;
	DBE    dbe_loc, dbe_parent;
	THROW(CheckTblPtr(p_prct = new ProcessorTbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc,    p_prct->LocID);
	PPDbqFuncPool::InitObjNameFunc(dbe_parent, PPDbqFuncPool::IdObjNamePrc, p_prct->ParentID);
	q = & select(p_prct->ID, p_prct->Name, p_prct->Code, dbe_parent, dbe_loc, 0L).from(p_prct, 0L);
	dbq = ppcheckfiltid(dbq, p_prct->Kind, Filt.Kind);
	dbq = ppcheckfiltid(dbq, p_prct->ParentID, Filt.ParentID);
	dbq = ppcheckfiltid(dbq, p_prct->LocID, Filt.LocID);
	q->where(*dbq);
	if(Filt.ParentID)
		q->orderBy(p_prct->Kind, p_prct->ParentID, p_prct->Name, 0L);
	else
		q->orderBy(p_prct->Kind, p_prct->Name, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		SString fmt_buf;
		uint   title_id = (Filt.Kind == PPPRCK_GROUP) ? 1 : 0;
		if(PPGetSubStr(PPTXT_PRCTITLES, title_id, fmt_buf)) {
			PPID   parent_id = (Filt.ParentID & ~PRCEXDF_GROUP);
			ProcessorTbl::Rec rec;
			if(!parent_id || PrcObj.Fetch(parent_id, &rec) <= 0)
				rec.Name[0] = 0;
			pSubTitle->Printf(fmt_buf, rec.Name);
		}
		else
			*pSubTitle = 0;
	}
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_prct;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewProcessor::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   prc_id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_VIEWTECH:
				ok = -1;
				if(prc_id) {
					TechFilt filt;
					filt.PrcID = prc_id;
					::ViewTech(&filt);
				}
				break;
			case PPVCMD_VIEWTSESS:
				ok = -1;
				if(prc_id) {
					TSessionFilt filt;
					filt.PrcID = prc_id;
					::ViewTSession(&filt);
				}
				break;
			case PPVCMD_TRANSMIT:
				ok = Transmit(0);
				break;
			case PPVCMD_EXPORTUHTT:
				ok = -1;
				ExportUhtt();
				break;
		}
	}
	return ok;
}

int PPViewProcessor::ExportUhtt()
{
	int    ok = -1;
	SString msg_buf, fmt_buf, temp_buf, loc_text_buf;
	SString phone_buf, contact_buf, addr_buf;
	PPLogger logger;
	PPUhttClient uhtt_cli;
	PPWaitStart();
	THROW(uhtt_cli.Auth());
	{
		ProcessorViewItem item;
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			const PPID _id = item.ID;
			PPProcessorPacket pack;
			if(PrcObj.GetPacket(_id, &pack) > 0) {
				long    uhtt_prc_id = 0;
				UhttProcessorPacket uhtt_pack, ret_pack;
				int   dont_update = 0;
				if(pack.Rec.Code[0] != 0) {
					if(uhtt_cli.GetProcessorByCode(pack.Rec.Code, ret_pack) > 0) {
						uhtt_prc_id = ret_pack.ID;
						dont_update = 1;
					}
					if(dont_update) {
						uhtt_prc_id = ret_pack.ID;
						//
						PPFormatT(PPTXT_UHTTEXPPRC_FOUND, &msg_buf, (const char *)pack.Rec.Name);
						logger.Log(msg_buf);
					}
					else {
						uhtt_pack.ID = uhtt_prc_id;
						#define CPYFLD(f) uhtt_pack.f = pack.Rec.f
						CPYFLD(Kind);
						CPYFLD(Flags);
						CPYFLD(LocID);
						//CPYFLD(LinkObjType);
						//CPYFLD(LinkObjID);
						CPYFLD(CipPersonKindID);
						CPYFLD(CipMax);
						#undef CPYFLD
						uhtt_pack.SetName(pack.Rec.Name);
						uhtt_pack.SetSymb(pack.Rec.Code);
						for(uint i = 0; i < pack.Ext.GetPlaceDescriptionCount(); i++) {
							PPProcessorPacket::PlaceDescription src_item;
							if(pack.Ext.GetPlaceDescription(i, src_item)) {
								int    uhtt_goods_id = 0;
								if(uhtt_cli.GetUhttGoods(src_item.GoodsID, 0, &uhtt_goods_id, 0) > 0) {
									UhttPrcPlaceDescription * p_new_item = new UhttPrcPlaceDescription;
									THROW_MEM(p_new_item);
									p_new_item->GoodsID = uhtt_goods_id;
									p_new_item->SetRange(src_item.Range);
									p_new_item->SetDescr(src_item.Descr);
                                    THROW_SL(uhtt_pack.Places.insert(p_new_item));
								}
								else {
									PPGetLastErrorMessage(1, msg_buf);
									logger.Log(msg_buf);
								}
							}
						}
						int    cr = uhtt_cli.CreateProcessor(&uhtt_prc_id, uhtt_pack);
						if(cr) {
							logger.Log(PPFormatT(PPTXT_UHTTEXPPRC_EXPORTED, &msg_buf, (const char *)pack.Rec.Name));
						}
						else {
							(temp_buf = uhtt_cli.GetLastMessage()).ToOem();
							logger.Log(PPFormatT(PPTXT_UHTTEXPPRC_EEXPORT, &msg_buf, (const char *)pack.Rec.Name, temp_buf.cptr()));
						}
					}
				}
				else {
					logger.Log(PPFormatT(PPTXT_UHTTEXPPRC_NOSYMB, &msg_buf, (const char *)pack.Rec.Name, temp_buf.cptr()));
				}
			}
		}
	}
	PPWaitStop();
	CATCH
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}
//
// Implementation of PPALDD_Processor
//
PPALDD_CONSTRUCTOR(Processor)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjProcessor;
	}
}

PPALDD_DESTRUCTOR(Processor)
{
	Destroy();
	delete static_cast<PPObjProcessor *>(Extra[0].Ptr);
}

int PPALDD_Processor::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		ProcessorTbl::Rec rec;
		PPObjProcessor * p_prc_obj = static_cast<PPObjProcessor *>(Extra[0].Ptr);
		if(p_prc_obj->Search(rFilt.ID, &rec) > 0) {
			H.ID       = rec.ID;
			H.ParentID = rec.ParentID;
			H.Kind     = rec.Kind;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Code, rec.Code);
			H.LocID       = rec.LocID;
			H.TimeUnitID  = rec.TimeUnitID;
			H.Flags       = rec.Flags;
			H.LinkObjType = rec.LinkObjType;
			H.LinkObjID   = rec.LinkObjID;
			H.WrOffOpID   = rec.WrOffOpID;
			H.WrOffArID   = rec.WrOffArID;
			H.LinkTransp = (rec.LinkObjType == PPOBJ_TRANSPORT) ? rec.LinkObjID : 0;
			H.LinkPerson = (rec.LinkObjType == PPOBJ_PERSON) ? rec.LinkObjID : 0;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_ProcessorView
//
PPALDD_CONSTRUCTOR(ProcessorView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ProcessorView) { Destroy(); }

int PPALDD_ProcessorView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Processor, rsrv);
	H.FltKind  = p_filt->Kind;
	H.FltGrpID = p_filt->ParentID;
	H.FltLocID = p_filt->LocID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ProcessorView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(Processor);
}

int PPALDD_ProcessorView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Processor);
	I.PrcID    = item.ID;
	I.ParentID = item.ParentID;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_ProcessorView::Destroy() { DESTROY_PPVIEW_ALDD(Processor); }
//
// Implementation of PPALDD_UhttProcessor
//
struct UhttProcessorBlock {
	enum {
		stFetch             = 0,
		stSet               = 0x0001,
		stTSessConfigInited = 0x0002
	};
	UhttProcessorBlock() : PlacePos(0), State(0)
	{
		// @v10.9.9 Clear();
	}
	void Clear()
	{
		Pack.destroy();
		PlacePos = 0;
		State = 0;
	}
	const PPTSessConfig & GetTSessConfig()
	{
		if(!(State & stTSessConfigInited)) {
			PPObjTSession::ReadConfig(&TSesCfg);
			State |= stTSessConfigInited;
		}
		return TSesCfg;
	}
	PPObjProcessor PrcObj;
	PPProcessorPacket Pack;
	PPTSessConfig TSesCfg;
	uint   PlacePos;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttProcessor)
{
	Extra[0].Ptr = new UhttProcessorBlock();
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData("iter@Places", &I_Places, sizeof(I_Places));
}

PPALDD_DESTRUCTOR(UhttProcessor)
{
	Destroy();
	delete static_cast<UhttProcessorBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttProcessor::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		UhttProcessorBlock & r_blk = *static_cast<UhttProcessorBlock *>(Extra[0].Ptr);
		if(r_blk.PrcObj.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
			H.ID       = r_blk.Pack.Rec.ID;
			H.ParentID = r_blk.Pack.Rec.ParentID;
			H.Kind     = r_blk.Pack.Rec.Kind;
			H.Flags       = r_blk.Pack.Rec.Flags;
			H.LocID       = r_blk.Pack.Rec.LocID;
			H.LinkObjType = r_blk.Pack.Rec.LinkObjType;
			H.LinkObjID   = r_blk.Pack.Rec.LinkObjID;
			H.CipPersonKindID = r_blk.Pack.Rec.CipPersonKindID;
			H.CipMax = r_blk.Pack.Rec.CipMax;
			H.OwnerGuaID = r_blk.Pack.Ext.GetOwnerGuaID();
			STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
			STRNSCPY(H.Symb, r_blk.Pack.Rec.Code);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

int PPALDD_UhttProcessor::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttProcessorBlock & r_blk = *static_cast<UhttProcessorBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@Places"))
		r_blk.PlacePos = 0;
	return 1;
}

int PPALDD_UhttProcessor::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttProcessorBlock & r_blk = *static_cast<UhttProcessorBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@Places")) {
		PPProcessorPacket::PlaceDescription pd_item;
		if(r_blk.Pack.Ext.GetPlaceDescription(r_blk.PlacePos++, pd_item)) {
            I_Places.GoodsID = pd_item.GoodsID;
            STRNSCPY(I_Places.Range, pd_item.Range);
            STRNSCPY(I_Places.Descr, pd_item.Descr);
			ok = DlRtm::NextIteration(iterId);
		}
	}
	return ok;
}

int PPALDD_UhttProcessor::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttProcessorBlock & r_blk = *static_cast<UhttProcessorBlock *>(Extra[0].Ptr);
	if(!(r_blk.State & UhttProcessorBlock::stSet)) {
		r_blk.Clear();
		r_blk.State |= UhttProcessorBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			ProcessorTbl::Rec parent_rec;
			r_blk.Pack.Rec.ID = H.ID;
			if(H.ParentID && r_blk.PrcObj.Fetch(H.ParentID, &parent_rec) > 0 && parent_rec.Kind == PPPRCK_GROUP)
				r_blk.Pack.Rec.ParentID = parent_rec.ID;
			else if(H.Kind == PPPRCK_PROCESSOR) {
				const PPID def_time_tech_id = r_blk.GetTSessConfig().DefTimeTechID;
                if(def_time_tech_id) {
                    PPObjTech tec_obj;
					TechTbl::Rec tech_rec;
                    if(tec_obj.Fetch(def_time_tech_id, &tech_rec) > 0) {
                        if(tech_rec.PrcID && r_blk.PrcObj.Fetch(tech_rec.PrcID, &parent_rec) > 0 && parent_rec.Kind == PPPRCK_GROUP)
							r_blk.Pack.Rec.ParentID = parent_rec.ID;
                    }
                }
			}
			r_blk.Pack.Rec.Kind = H.Kind;
			r_blk.Pack.Rec.Flags = H.Flags;
			r_blk.Pack.Rec.LocID = H.LocID;
			r_blk.Pack.Rec.LinkObjType = H.LinkObjType;
			r_blk.Pack.Rec.LinkObjID = H.LinkObjID;
			r_blk.Pack.Rec.CipPersonKindID = H.CipPersonKindID;
			r_blk.Pack.Rec.CipMax = (int16)H.CipMax;
			r_blk.Pack.Ext.SetOwnerGuaID(H.OwnerGuaID);
			STRNSCPY(r_blk.Pack.Rec.Name, H.Name);
			STRNSCPY(r_blk.Pack.Rec.Code, H.Symb);
		}
		else if(iterId == GetIterID("iter@Places")) {
			PPProcessorPacket::PlaceDescription pd_item;
			pd_item.GoodsID = I_Places.GoodsID;
			pd_item.Range = I_Places.Range;
			pd_item.Descr = I_Places.Descr;
			uint   new_pos = r_blk.Pack.Ext.GetPlaceDescriptionCount();
			THROW(r_blk.Pack.Ext.PutPlaceDescription(new_pos, &pd_item));
		}
	}
	else {
		PPID  id = r_blk.Pack.Rec.ID;
		THROW(r_blk.PrcObj.PutPacket(&id, &r_blk.Pack, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}

