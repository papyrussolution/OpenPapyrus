// OBJOPRK.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

PPInventoryOpEx::PPInventoryOpEx()
{
	THISZERO();
}

/*static*/int FASTCALL PPInventoryOpEx::Helper_GetAccelInputMode(long flags)
{
	int    mode = accsliNo;
	if(flags & (INVOPF_ACCELADDITEMS|INVOPF_ACCELADDITEMSQTTY)) {
		if(flags & INVOPF_ACCELADDITEMS && !(flags & INVOPF_ACCELADDITEMSQTTY))
			mode = accsliCode;
		else if(!(flags & INVOPF_ACCELADDITEMS) && (flags & INVOPF_ACCELADDITEMSQTTY))
			mode = accsliCodeAndQtty;
	}
	return mode;
}

int PPInventoryOpEx::GetAccelInputMode() const
{
	return Helper_GetAccelInputMode(Flags);
}

void PPInventoryOpEx::SetAccelInputMode(int mode)
{
	if(mode == accsliCode) {
		SETUPFLAGS(Flags, INVOPF_ACCELADDITEMS, INVOPF_ACCELADDITEMSQTTY);
	}
	else if(mode == accsliCodeAndQtty) {
		SETUPFLAGS(Flags, INVOPF_ACCELADDITEMSQTTY, INVOPF_ACCELADDITEMS);
	}
	else {
		Flags &= ~(INVOPF_ACCELADDITEMS|INVOPF_ACCELADDITEMSQTTY);
	}
}
//
// PPReckonOpEx
//
PPReckonOpEx::PPReckonOpEx() : Beg(ZERODATE), End(ZERODATE), Flags(0), PersonRelTypeID(0)
{
	memzero(Reserve, sizeof(Reserve));
}

PPReckonOpEx & PPReckonOpEx::Z()
{
	Beg = End = ZERODATE;
	Flags = 0;
	PersonRelTypeID = 0;
	memzero(Reserve, sizeof(Reserve));
	OpList.clear(); // @v10.6.1 freeAll-->clear
	return *this;
}

PPReckonOpEx & FASTCALL PPReckonOpEx::operator = (const PPReckonOpEx & rS)
{
	Beg = rS.Beg;
	End = rS.End;
	Flags = rS.Flags;
	PersonRelTypeID = rS.PersonRelTypeID;
	OpList = rS.OpList;
	return *this;
}

bool PPReckonOpEx::IsEmpty() const { return (!Beg && !End && !Flags && !OpList.getCount()); }

int PPReckonOpEx::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	struct SRD {
		LDATE  Beg;
		LDATE  End;
		long   Flags;
		PPID   PersonRelTypeID;
		long   Reserve[4];
	};
	int    ok = 1;
	uint16 sz = 0;
	if(dir > 0) {
		sz = IsEmpty() ? 0 : sizeof(SRD);
		THROW_SL(rBuf.Write(&sz, sizeof(sz)));
		if(sz) {
			SRD srd;
			MEMSZERO(srd);
			srd.Beg = Beg;
			srd.End = End;
			srd.Flags = Flags;
			srd.PersonRelTypeID = PersonRelTypeID;
			THROW_SL(rBuf.Write(&srd, sizeof(srd)));
			THROW_SL(rBuf.Write(&OpList, 0));
		}
	}
	else if(dir < 0) {
		Z();
		THROW_SL(rBuf.Read(&sz, sizeof(sz)));
		if(sz) {
			if(sz >= sizeof(SRD)) {
				SRD srd;
				THROW_SL(rBuf.Read(&srd, sizeof(srd)));
				Beg = srd.Beg;
				End = srd.End;
				Flags = srd.Flags;
				PersonRelTypeID = srd.PersonRelTypeID;
				if(sz > sizeof(SRD))
					rBuf.SetRdOffs(rBuf.GetRdOffs()+sz-sizeof(SRD));
			}
			else
				rBuf.SetRdOffs(rBuf.GetRdOffs()+sz);
			THROW_SL(rBuf.Read(&OpList, 0));
		}
	}
	CATCHZOK
	return ok;
}

void PPReckonOpEx::PeriodToStr(SString & rBuf) const // @todo Убрать эту функцию и перейти на шаблонизированный период
{
	char   temp[64];
	char * p = temp;
	if(!Beg)
		if(Flags & ROXF_BEGISBILLDT)
			*p++ = '@';
	DateRange period;
	period.Set(Beg, End);
	periodfmt(period, p);
	if(!End) {
		if(Flags & ROXF_ENDISBILLDT) {
			if(*(p-1) != '@') {
				if(Beg == 0 && !(Flags & ROXF_BEGISBILLDT)) {
					*p++ = '.';
					*p++ = '.';
					*p = 0;
				}
				p += sstrlen(p);
				*p++ = '@';
				*p = 0;
			}
		}
		else if(*(p-1) == '@') {
			*p++ = '.';
			*p++ = '.';
			*p = 0;
		}
	}
	rBuf = temp;
}

int PPReckonOpEx::StrToPeriod(const char * pBuf)
{
	char   temp[64];
	DateRange period;
	int    at_beg = 0, at_end = 0;
	STRNSCPY(temp, pBuf);
	strip(temp);
	if(temp[0] == '@') {
		at_beg = 1;
		STRNSCPY(temp, temp+1);
	}
	if(temp[sstrlen(temp)-1] == '@') {
		at_end = 1;
		temp[sstrlen(temp)-1] = 0;
	}
	period.FromStr(temp, 0);
	Beg = period.low;
	End = period.upp;
	SETFLAG(Flags, ROXF_BEGISBILLDT, !period.low && at_beg);
	SETFLAG(Flags, ROXF_ENDISBILLDT, !period.upp && at_end);
	return 1;
}

int PPReckonOpEx::GetReckonPeriod(LDATE debtDate, DateRange & rPeriod) const
{
	if(checkdrange(debtDate, Beg, End)) {
		rPeriod.low = (Beg == 0 && Flags & ROXF_BEGISBILLDT) ? debtDate : ZERODATE;
		rPeriod.upp = (End == 0 && Flags & ROXF_ENDISBILLDT) ? debtDate : ZERODATE;
		return 1;
	}
	else {
		rPeriod.SetDate(MAXDATE);
		return -1;
	}
}

void PPReckonOpEx::GetDebtPeriod(LDATE paymDate, DateRange & rPeriod) const
{
	rPeriod.Set(Beg, End);
	if(rPeriod.low == 0 && Flags & ROXF_BEGISBILLDT)
		rPeriod.low = paymDate;
	if(rPeriod.upp == 0 && Flags & ROXF_ENDISBILLDT)
		rPeriod.upp = paymDate;
}
//
// PPBillPoolOpEx
//
PPBillPoolOpEx::PPBillPoolOpEx()
{
	Init();
}

void PPBillPoolOpEx::Init()
{
	Flags = 0;
	memzero(Reserve, sizeof(Reserve));
	OpList.clear();
}

PPBillPoolOpEx & FASTCALL PPBillPoolOpEx::operator = (const PPBillPoolOpEx & src)
{
	Init();
	Flags = src.Flags;
	OpList.copy(src.OpList);
	return *this;
}
//
//
//
PPOprKind2::PPOprKind2()
{
	THISZERO();
}
//
// PPOprKindPacket
//
PPOprKindPacket::PPOprKindPacket() : P_IOE(0), P_DIOE(0), P_GenList(0), P_ReckonData(0), P_PoolData(0), P_DraftData(0)
{
	// @v12.2.1 Z();
}

PPOprKindPacket::~PPOprKindPacket()
{
	delete P_IOE;
	delete P_DIOE;
	delete P_GenList;
	delete P_ReckonData;
	delete P_PoolData;
	delete P_DraftData;
}

PPOprKindPacket & PPOprKindPacket::Z()
{
	MEMSZERO(Rec);
	Amounts.Z(); // @v12.2.1
	ATTmpls.clear(); // @v12.2.1 freeAll()-->clear()
	ExtString.Z();
	ZDELETE(P_IOE);
	ZDELETE(P_DIOE);
	ZDELETE(P_GenList);
	ZDELETE(P_ReckonData);
	ZDELETE(P_PoolData);
	ZDELETE(P_DraftData);
	OpCntrPack.Init(0, 0); // @v12.2.1
	return *this;
}

PPOprKindPacket & FASTCALL PPOprKindPacket::operator = (const PPOprKindPacket & rS)
{
	Z();
	Rec = rS.Rec;
	Amounts = rS.Amounts; // @v12.2.1
	ATTmpls = rS.ATTmpls; // @v12.2.1
	if(rS.P_IOE) {
		P_IOE = new PPInventoryOpEx;
		ASSIGN_PTR(P_IOE, *rS.P_IOE);
	}
	if(rS.P_DIOE) {
		P_DIOE = new PPDebtInventOpEx;
		ASSIGN_PTR(P_DIOE, *rS.P_DIOE);
	}
	if(rS.P_GenList) {
		P_GenList = new ObjRestrictArray(*rS.P_GenList);
	}
	if(rS.P_ReckonData) {
		P_ReckonData = new PPReckonOpEx;
		ASSIGN_PTR(P_ReckonData, *rS.P_ReckonData);
	}
	if(rS.P_PoolData) {
		P_PoolData = new PPBillPoolOpEx;
		ASSIGN_PTR(P_PoolData, *rS.P_PoolData);
	}
	if(rS.P_DraftData) {
		P_DraftData = new PPDraftOpEx;
		ASSIGN_PTR(P_DraftData, *rS.P_DraftData);
	}
	ExtString = rS.ExtString;
	OpCntrPack = rS.OpCntrPack; // @v12.2.1
	return *this;
}

int PPOprKindPacket::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData_def(fldID, OPKEXSTR_MEMO, ExtString, rBuf); }
int PPOprKindPacket::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }
//
//
//
PPDraftOpEx::PPDraftOpEx()
{
	Init();
}

void PPDraftOpEx::Init()
{
	WrOffOpID = 0;
	WrOffObjID = 0;
	WrOffComplOpID = 0;
	Flags = 0;
	memzero(Reserve, sizeof(Reserve));
}

PPDraftOpEx & FASTCALL PPDraftOpEx::operator = (const PPDraftOpEx & s)
{
	WrOffOpID = s.WrOffOpID;
	WrOffObjID = s.WrOffObjID;
	WrOffComplOpID = s.WrOffComplOpID;
	Flags = s.Flags;
	memcpy(Reserve, s.Reserve, sizeof(Reserve));
	return *this;
}
//
//
//
struct OpkListEntry { // @flat
	PPID   ID;
	char   Name[48];
	int16  Rank;
	uint16 Reserve; // @alignment
};

static IMPL_CMPFUNC(OpkListEntry, i1, i2)
{
	const OpkListEntry * p_i1 = static_cast<const OpkListEntry *>(i1);
	const OpkListEntry * p_i2 = static_cast<const OpkListEntry *>(i2);
	if(p_i1->Rank > p_i2->Rank)
		return -1;
	else if(p_i1->Rank < p_i2->Rank)
		return 1;
	else
		return stricmp866(p_i1->Name, p_i2->Name);
}

/*static*/StrAssocArray * PPObjOprKind::MakeOprKindList(PPID linkOprKind, const PPIDArray * pOpList, uint flags)
{
	int    r;
	PPID   id = 0;
	PPOprKind opk;
	SVector temp_list(sizeof(OpkListEntry));
	StrAssocArray * p_list = 0;
	while((r = EnumOperations(0, &id, &opk)) > 0) {
		int    suit = 0;
		if(!(opk.Flags & OPKF_PASSIVE) || (flags & OPKLF_SHOWPASSIVE)) {
			PPID temp_id = (flags & OPKLF_OPLIST) ? id : opk.OpTypeID;
			if(!pOpList || pOpList->lsearch(temp_id))
				suit = BIN((flags & OPKLF_IGNORERIGHTS) || ObjRts.CheckOpID(id, PPR_READ));
		}
		if(suit && (!linkOprKind || linkOprKind == opk.LinkOpID)) {
			if(PPMaster || !oneof3(id, _PPOPK_SUPPLRET, _PPOPK_SELLRET, _PPOPK_RETAILRET)) {
				OpkListEntry entry;
				MEMSZERO(entry);
				entry.ID = opk.ID;
				entry.Rank = opk.Rank;
				STRNSCPY(entry.Name, opk.Name);
				THROW_SL(temp_list.ordInsert(&entry, 0, PTR_CMPFUNC(OpkListEntry)));
			}
		}
	}
	THROW(r);
	{
		THROW_MEM(p_list = new StrAssocArray);
		for(uint i = 0; i < temp_list.getCount(); i++) {
			const OpkListEntry * p_entry = static_cast<const OpkListEntry *>(temp_list.at(i));
			THROW_SL(p_list->Add(p_entry->ID, p_entry->Name));
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

#if 0 // @v11.1.11 @construction {
/*virtual*/ListBoxDef * PPObjOprKind::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = PPObjReference::Selector(pOrgDef, flags, extraPtr);
	AssignImages(p_def);
	return p_def;
}

int PPObjOprKind::AssignImages(ListBoxDef * pDef)
{
	if(pDef && pDef->valid() && (ImplementFlags & implTreeSelector)) {
		LongArray list;
		pDef->ClearImageAssocList();
		if(pDef->getIdList(list) > 0) {
			PPOprKind rec;
			for(uint i = 0; i < list.getCount(); i++) {
				PPID id = list.at(i);
				if(GetOpData(id, &rec) > 0) {
					long   img_id = 0;
					if(rec.OpTypeID == PPOPT_GOODSRECEIPT)
						img_id = PPDV_OP_GOODSRECEIPT;
					else if(rec.OpTypeID == PPOPT_GOODSEXPEND)
						img_id = PPDV_OP_GOODSEXPEND;
					else if(rec.OpTypeID == PPOPT_GOODSMODIF)
						img_id = PPDV_OP_GOODSMODIF;
					else 
						img_id = PPDV_MAIL01;
					if(img_id)
						pDef->AddVecImageAssoc(id, img_id);
				}
			}
		}
	}
	return 1;	
}
#endif // } @v11.1.11 @construction

int STDCALL SetupOprKindCombo(TDialog * dlg, uint ctl, PPID id, uint /*olwFlags*/, const PPIDArray * pOpList, uint opklFlags)
{
	int    ok = 0;
	ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(p_combo) {
		StrAssocArray * p_list = PPObjOprKind::MakeOprKindList(0, pOpList, opklFlags);
		PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_OPRKIND, p_list, lbtDisposeData|lbtDblClkNotify, 0);
		if(p_lw) {
			p_combo->setListWindow(p_lw, id);
			dlg->SetupWordSelector(ctl, 0, id, 2, 0);
			ok = 1;
		}
	}
	else
		ok = -1;
	return ok;
}

PPID SelectOpKind(PPID linkOpID, const PPIDArray * pOpList, uint opklFlags)
{
	PPID   id = 0;
	ListWindow * p_lw = 0;
	StrAssocArray * p_ary = 0;
	THROW(p_ary = PPObjOprKind::MakeOprKindList(linkOpID, pOpList, opklFlags));
	if(p_ary->getCount() == 1)
		id = p_ary->Get(0).Id;
	else if(p_ary->getCount() == 0)
		id = -1;
	if(id == 0) {
		SString title_buf;
		THROW(p_lw = CreateListWindow(p_ary, lbtDisposeData|lbtDblClkNotify));
		THROW(PPLoadText(PPTXT_SELECTOPRKIND, title_buf));
		p_lw->setTitle(title_buf);
		p_lw->ViewOptions |= (ofCenterX | ofCenterY);
		if(ExecView(p_lw) == cmOK)
			p_lw->getResult(&id);
		else
			id = -1;
	}
	CATCH
		id = PPErrorZ();
	ENDCATCH
	if(p_lw)
		delete p_lw;
	else
		delete p_ary;
	return id;
}

PPID CDECL SelectOprKind(uint opklFlags, PPID linkOpID, ...)
{
	va_list ap;
	PPID   id = 0;
	PPIDArray types;
	va_start(ap, linkOpID);
	while((id = va_arg(ap, PPID)) != 0)
		types.add(id);
	va_end(ap);
	return SelectOpKind(linkOpID, &types, opklFlags);
}
//
//
//
PPObjOprKind::PPObjOprKind(void * extraPtr) : PPObjReference(PPOBJ_OPRKIND, extraPtr)
{
	// @v11.1.11 (@construction) ImplementFlags |= (implStrAssocMakeList | implTreeSelector);
}

/*static*/int PPObjOprKind::GetATTemplList(PPID opID, PPAccTurnTemplArray * pList)
{
	int    ok = -1;
	PPID   p = 0, last = PP_MAXATURNTEMPLATES;
	PPAccTurnTempl t;
	const  size_t sz = sizeof(PPAccTurnTempl);
	CALLPTRMEMB(pList, clear());
	while(PPRef->EnumProperties(PPOBJ_OPRKIND, opID, &p, memzero(&t, sz), sz) > 0 && p <= last) {
		if(pList)
			THROW_SL(pList->insert(&t));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjOprKind::GetPacket(PPID id, PPOprKindPacket * pack)
{
	int    ok = 1;
	if(PPCheckGetObjPacketID(Obj, id)) {
		PPObjOpCounter opc_obj;
		THROW(Search(id, &pack->Rec) > 0);
		THROW(opc_obj.GetPacket(pack->Rec.OpCounterID, &pack->OpCntrPack));
		THROW(GetExAmountList(id, &pack->Amounts));
		THROW(P_Ref->GetPropVlrString(Obj, id, OPKPRP_EXTSTRDATA, pack->ExtString));
		pack->ATTmpls.freeAll();
		ZDELETE(pack->P_GenList);
		ZDELETE(pack->P_ReckonData);
		ZDELETE(pack->P_PoolData);
		if(pack->Rec.OpTypeID == PPOPT_INVENTORY) {
			PPInventoryOpEx ioe;
			ZDELETE(pack->P_IOE);
			if(P_Ref->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_INVENTORY, &ioe, sizeof(ioe)) > 0) {
				THROW_MEM(pack->P_IOE = new PPInventoryOpEx);
				*pack->P_IOE = ioe;
			}
		}
		else if(pack->Rec.OpTypeID == PPOPT_GENERIC) {
			THROW_MEM(pack->P_GenList = new ObjRestrictArray);
			THROW(GetGenericList(id, pack->P_GenList));
		}
		else if(oneof4(pack->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ)) { // @v10.5.7 PPOPT_DRAFTQUOTREQ
			PPDraftOpEx doe;
			ZDELETE(pack->P_DraftData);
			if(GetDraftExData(id, &doe) > 0) {
				THROW_MEM(pack->P_DraftData = new PPDraftOpEx);
				*pack->P_DraftData = doe;
			}
		}
		else {
			if(pack->Rec.OpTypeID == PPOPT_POOL) {
				THROW_MEM(pack->P_PoolData = new PPBillPoolOpEx);
				THROW(GetPoolExData(id, pack->P_PoolData));
			}
			THROW(PPObjOprKind::GetATTemplList(id, &pack->ATTmpls));
		}
		if(pack->Rec.Flags & OPKF_RECKON) {
			THROW_MEM(pack->P_ReckonData = new PPReckonOpEx);
			THROW(GetReckonExData(id, pack->P_ReckonData));
		}
		if(pack->Rec.SubType == OPSUBT_DEBTINVENT) {
			PPDebtInventOpEx dioe;
			MEMSZERO(dioe);
			ZDELETE(pack->P_DIOE);
			if(P_Ref->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_DEBTINVENT, &dioe, sizeof(dioe)) > 0) {
				THROW_MEM(pack->P_DIOE = new PPDebtInventOpEx);
				*pack->P_DIOE = dioe;
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjOprKind::PutPacket(PPID * pID, PPOprKindPacket * pack, int use_ta)
{
	int    ok = 1, r = 0;
	uint   i;
	PPOpCounter opc_rec;
	PPOpCounterPacket opc_pack;
	PPObjOpCounter opc_obj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pack->Rec.OpTypeID == PPOPT_INVENTORY) {
			if(pack->P_IOE && pack->P_IOE->Flags & INVOPF_COSTNOMINAL) {
				pack->Rec.Flags |= OPKF_BUYING;
				pack->Rec.Flags &= ~OPKF_SELLING;
			}
			else {
				pack->Rec.Flags |= OPKF_SELLING;
				pack->Rec.Flags &= ~OPKF_BUYING;
			}
		}
		// AHTOXA
		THROW(r = opc_obj.GetPacket(pack->Rec.OpCounterID, &opc_pack));
		if(r > 0) {
			if(opc_pack.Head.OwnerObjID)
				opc_pack.Head.OwnerObjID = NZOR(*pID, -1L);
		}
		else
			opc_pack.Head.OwnerObjID = *pID ? *pID : -1L;
		STRNSCPY(opc_pack.Head.CodeTemplate, pack->OpCntrPack.Head.CodeTemplate);
		opc_pack.Head.Counter = pack->OpCntrPack.Head.Counter;
		opc_pack.Head.Flags   = pack->OpCntrPack.Head.Flags;
		opc_pack.Init(pack->OpCntrPack.P_Items);
		THROW(opc_obj.PutPacket(&pack->Rec.OpCounterID, &opc_pack, 0));
		pack->OpCntrPack.Head.ID = pack->Rec.OpCounterID;
		// } AHTOXA
		if(*pID) {
			PPOprKind org_rec;
			PPID   prop = 0;
			THROW(P_Ref->GetItem(Obj, *pID, &org_rec) > 0);
			THROW_DB(deleteFrom(&P_Ref->Prop, 0, (P_Ref->Prop.ObjType == Obj && P_Ref->Prop.ObjID == *pID && P_Ref->Prop.Prop <= (long)PP_MAXATURNTEMPLATES)));
			THROW(P_Ref->UpdateItem(Obj, *pID, &pack->Rec, 1, 0));
			if(org_rec.OpCounterID != pack->Rec.OpCounterID)
				if(P_Ref->GetItem(PPOBJ_OPCOUNTER, org_rec.OpCounterID, &opc_rec) > 0)
					if(opc_rec.OwnerObjID)
						THROW(opc_obj.PutPacket(&org_rec.OpCounterID, 0, 0));
			Dirty(*pID);
		}
		else {
			*pID = pack->Rec.ID;
			THROW(P_Ref->AddItem(PPOBJ_OPRKIND, pID, &pack->Rec, 0));
		}
		for(i = 0; i < pack->ATTmpls.getCount(); i++) {
			THROW(P_Ref->PutProp(Obj, *pID, static_cast<PPID>(i+1), &pack->ATTmpls.at(i), sizeof(PPAccTurnTempl), 0));
		}
		if(pack->Rec.OpTypeID == PPOPT_GENERIC) {
			THROW(P_Ref->PutPropArray(PPOBJ_OPRKIND, *pID, OPKPRP_GENLIST2, pack->P_GenList, 0));
		}
		else if(pack->Rec.OpTypeID == PPOPT_POOL) {
			THROW(SetPoolExData(*pID, pack->P_PoolData, 0));
		}
		else if(oneof4(pack->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ)) { // @v10.5.7 PPOPT_DRAFTQUOTREQ
			THROW(SetDraftExData(*pID, pack->P_DraftData));
		}
		if(pack->Rec.Flags & OPKF_RECKON) {
			THROW(SetReckonExData(*pID, pack->P_ReckonData, 0));
		}
		THROW(P_Ref->PutPropVlrString(Obj, *pID, OPKPRP_EXTSTRDATA, pack->ExtString));
		THROW(P_Ref->PutPropArray(Obj, *pID, OPKPRP_EXAMTLIST, &pack->Amounts, 0));
		if(pack->Rec.OpTypeID == PPOPT_INVENTORY)
			THROW(P_Ref->PutProp(Obj, *pID, OPKPRP_INVENTORY, pack->P_IOE));
		if(pack->Rec.SubType == OPSUBT_DEBTINVENT)
			THROW(P_Ref->PutProp(Obj, *pID, OPKPRP_DEBTINVENT, pack->P_DIOE));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjOprKind::GetExtStrData(PPID opID, int fldID, SString & rBuf)
{
	SString line_buf;
	rBuf.Z();
	return (P_Ref->GetPropVlrString(Obj, opID, OPKPRP_EXTSTRDATA, line_buf) > 0) ? PPGetExtStrData_def(fldID, OPKEXSTR_MEMO, line_buf, rBuf) : -1;
}

int PPObjOprKind::GetExAmountList(PPID id, PPIDArray * pList) { return P_Ref->GetPropArray(Obj, id, OPKPRP_EXAMTLIST, pList); }
int PPObjOprKind::GetGenericList(PPID id, ObjRestrictArray * pList) { return P_Ref->GetPropArray(Obj, id, OPKPRP_GENLIST2, pList); }

int PPObjOprKind::GetGenericList(PPID id, PPIDArray * pList)
{
	ObjRestrictArray or_list;
	int    ok = GetGenericList(id, &or_list);
	if(ok > 0)
		for(uint i = 0; i < or_list.getCount(); i++)
			if(!pList->add(or_list.at(i).ObjID)) {
				ok = 0;
				break;
			}
	return ok;
}

int PPObjOprKind::SetReckonExData(PPID id, const PPReckonOpEx * pData, int use_ta)
{
	int    ok = 1;
	PPIDArray temp;
	if(pData) {
		uint   i;
		THROW_SL(temp.add(static_cast<long>(pData->Beg)));
		THROW_SL(temp.add(static_cast<long>(pData->End)));
		THROW_SL(temp.add(pData->Flags));
		THROW_SL(temp.add(pData->PersonRelTypeID));
		for(i = 0; i < SIZEOFARRAY(pData->Reserve); i++)
			THROW_SL(temp.add(pData->Reserve[i]));
		for(i = 0; i < pData->OpList.getCount(); i++)
			THROW_SL(temp.add(pData->OpList.get(i)));
	}
	THROW(P_Ref->PutPropArray(Obj, id, OPKPRP_PAYMOPLIST, (pData ? &temp : static_cast<const PPIDArray *>(0)), use_ta));
	CATCHZOK
	return ok;
}

int PPObjOprKind::GetReckonExData(PPID id, PPReckonOpEx * pData)
{
	int    ok = -1;
	PPIDArray temp;
	if(P_Ref->GetPropArray(Obj, id, OPKPRP_PAYMOPLIST, &temp) > 0) {
		if(temp.getCount() < ROX_HDR_DW_COUNT) {
			CALLPTRMEMB(pData, Z());
		}
		else {
			if(pData) {
				pData->Beg.v = temp.at(0);
				pData->End.v = temp.at(1);
				pData->Flags = temp.at(2);
				pData->PersonRelTypeID = temp.at(3);
				memzero(pData->Reserve, sizeof(pData->Reserve));
				for(uint i = ROX_HDR_DW_COUNT; i < temp.getCount(); i++)
					if(!pData->OpList.add(temp.at(i))) {
						pData->Z();
						return PPSetErrorSLib();
					}
			}
			ok = 1;
		}
	}
	return ok;
}

struct PPDraftOpEx_Strg {
	PPID   Tag;              // Const = PPOBJ_OPRKIND
	PPID   ID;               // -> PPOprKind::ID
	PPID   Prop;             // Const = OPKPRP_DRAFT
	long   ReserveCnt;
	PPID   WrOffOpID;
	long   Flags;
	PPID   WrOffObjID;
	PPID   WrOffComplOpID;
	char   Reserve2[44];
	long   Reserve3;
	long   Reserve4;
};

int PPObjOprKind::SetDraftExData(PPID id, const PPDraftOpEx * pData)
{
	PPDraftOpEx_Strg strg, * p_strg = 0;
	size_t sz = 0;
	if(pData) {
		p_strg = &strg;
		sz = sizeof(strg);
		MEMSZERO(strg);
		strg.WrOffOpID  = pData->WrOffOpID;
		strg.WrOffObjID = pData->WrOffObjID;
		strg.WrOffComplOpID = pData->WrOffComplOpID;
		strg.Flags      = pData->Flags;
	}
	return BIN(P_Ref->PutProp(Obj, id, OPKPRP_DRAFT, p_strg, sz));
}

int PPObjOprKind::GetDraftExData(PPID id, PPDraftOpEx * pData)
{
	int    ok = -1;
	PPDraftOpEx_Strg strg;
	if(P_Ref->GetProperty(Obj, id, OPKPRP_DRAFT, &strg, sizeof(strg)) > 0) {
		if(pData) {
			pData->WrOffOpID  = strg.WrOffOpID;
			pData->WrOffObjID = strg.WrOffObjID;
			pData->WrOffComplOpID = strg.WrOffComplOpID;
			pData->Flags      = strg.Flags;
		}
		ok = 1;
	}
	else if(pData) {
		pData->WrOffOpID  = 0;
		pData->WrOffObjID = 0;
		pData->WrOffComplOpID = 0;
		pData->Flags      = 0;
		ok = -1;
	}
	return ok;
}

int PPObjOprKind::SetPoolExData(PPID id, const PPBillPoolOpEx * pData, int use_ta)
{
	int    ok = 1;
	PPIDArray temp;
	if(pData) {
		uint   i;
		THROW_SL(temp.add(pData->Flags));
		for(i = 0; i < SIZEOFARRAY(pData->Reserve); i++)
			THROW(temp.add(pData->Reserve[i]));
		for(i = 0; i < pData->OpList.getCount(); i++)
			THROW(temp.add(pData->OpList.get(i)));
	}
	THROW(P_Ref->PutPropArray(Obj, id, OPKPRP_BILLPOOL, (pData ? &temp : static_cast<const PPIDArray *>(0)), use_ta));
	CATCHZOK
	return ok;
}

int PPObjOprKind::GetPoolExData(PPID id, PPBillPoolOpEx * pData)
{
	int    ok = -1;
	PPIDArray temp;
	CALLPTRMEMB(pData, Init());
	if(P_Ref->GetPropArray(Obj, id, OPKPRP_BILLPOOL, &temp) > 0 && temp.getCount() >= BPOX_HDR_DW_COUNT) {
		if(pData) {
			pData->Flags = temp.at(0);
			memzero(pData->Reserve, sizeof(pData->Reserve));
			for(uint i = BPOX_HDR_DW_COUNT; i < temp.getCount(); i++) {
				THROW_SL(pData->OpList.add(temp.at(i)));
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjOprKind::Helper_GetOpListByLink(PPID opTypeID, PPID linkOpID, PPIDArray * pList)
{
	int    ok = 1;
	if(opTypeID && pList) {
		PROFILE_START
		PPOprKind op_rec;
		for(SEnum en = P_Ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
			if(op_rec.OpTypeID == opTypeID && (!linkOpID || op_rec.LinkOpID == linkOpID)) {
				if(!pList->add(op_rec.ID))
					ok = PPSetErrorSLib();
			}
		}
		PROFILE_END
	}
	return ok;
}

int PPObjOprKind::GetPaymentOpList(PPID linkOpID, PPIDArray * pList) { return Helper_GetOpListByLink(PPOPT_PAYMENT, linkOpID, pList); }
int PPObjOprKind::GetCorrectionOpList(PPID linkOpID, PPIDArray * pList) { return Helper_GetOpListByLink(PPOPT_CORRECTION, linkOpID, pList); }
int PPObjOprKind::GetQuoteReqSeqOpList(PPID linkOpID, PPIDArray * pList) { return Helper_GetOpListByLink(PPOPT_DRAFTQUOTREQ, linkOpID, pList); }

int FASTCALL GetOpList(PPID opTypeID, PPIDArray * pList)
{
	int    ok = -1;
	PPOprKind op_rec;
	for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); en.Next(&op_rec) > 0;) {
		if(!opTypeID || op_rec.OpTypeID == opTypeID) {
			CALLPTRMEMB(pList, addUnique(op_rec.ID));
			ok = 1;
		}
	}
	return ok;
}

int PPObjOprKind::GetPayableOpList(PPID accSheetID, PPIDArray * pList)
{
	int    ok = 1;
	PPOprKind op_rec;
	if(pList) {
		for(SEnum en = P_Ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
   		    if((accSheetID == -1 || op_rec.AccSheetID == accSheetID) && op_rec.Flags & OPKF_NEEDPAYMENT) {
				if(op_rec.OpTypeID != PPOPT_GOODSORDER || !(CConfig.Flags & CCFLG_IGNOREORDERSDEBT))
					if(!pList->add(op_rec.ID))
						ok = PPSetErrorSLib();
			}
		}
	}
	return ok;
}

int PPObjOprKind::GetProfitableOpList(PPID accSheetID, PPIDArray * pList)
{
	int    ok = 1;
	PPOprKind op_rec;
	if(pList) {
		for(SEnum en = P_Ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
   		    if(op_rec.AccSheetID == accSheetID && op_rec.Flags & OPKF_PROFITABLE)
				if(!pList->add(op_rec.ID))
					ok = PPSetErrorSLib();
		}
	}
	return ok;
}

PPObjOprKind::ReservedOpCreateBlock::ReservedOpCreateBlock()
{
	THISZERO();
}

int PPObjOprKind::Helper_GetReservedOp(PPID * pID, const ReservedOpCreateBlock & rBlk, int use_ta)
{
	int    ok = 1;
	PPID   op_id = 0;
	PPOprKind op_rec;
	if(rBlk.OpID && Search(rBlk.OpID, &op_rec) > 0) {
		op_id = op_rec.ID;
	}
	else {
		assert(rBlk.OpTypeID);
		assert(rBlk.NameTxtId);
		assert(!isempty(rBlk.P_Symb));
		assert(!isempty(rBlk.P_CodeTempl));
		SString temp_buf;
		PPOprKindPacket op_pack;
		op_pack.Rec.ID = rBlk.OpID;
        PPLoadText(rBlk.NameTxtId, temp_buf);
        THROW(temp_buf.NotEmptyS());
        STRNSCPY(op_pack.Rec.Name, temp_buf);
        STRNSCPY(op_pack.Rec.Symb, rBlk.P_Symb);
        op_pack.Rec.AccSheetID = rBlk.AccSheetID;
        op_pack.Rec.OpTypeID = rBlk.OpTypeID;
		op_pack.Rec.SubType = static_cast<int16>(rBlk.SubType); // @v12.1.4
		op_pack.Rec.Flags |= rBlk.Flags;
		op_pack.OpCntrPack.Init(0);
		STRNSCPY(op_pack.OpCntrPack.Head.CodeTemplate, rBlk.P_CodeTempl);
		op_pack.OpCntrPack.Head.Counter = 0;
		THROW(PutPacket(&op_id, &op_pack, use_ta));
	}
	CATCHZOK
	ASSIGN_PTR(pID, op_id);
	return ok;
}

int PPObjOprKind::GetEdiRecadvOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_RECADV;
	blk.OpTypeID = PPOPT_DRAFTEXPEND;
	blk.NameTxtId = PPTXT_OPK_EDI_RECADV;
	blk.Flags = OPKF_PASSIVE;
	blk.P_Symb = "EDIRECADV";
	blk.P_CodeTempl = "RECADV%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiOrdrspOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_ORDERRSP;
	blk.OpTypeID = PPOPT_DRAFTRECEIPT;
	blk.NameTxtId = PPTXT_OPK_EDI_ORDRSP;
	blk.Flags = OPKF_PASSIVE;
	blk.P_Symb = "EDIORDRSP";
	blk.P_CodeTempl = "ORDRSP%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiShopChargeOnOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_SHOPCHARGEON;
	blk.OpTypeID = PPOPT_DRAFTRECEIPT;
	blk.NameTxtId = PPTXT_OPK_EDI_SHOPCHARGEON;
	blk.Flags = OPKF_PASSIVE;
	blk.P_Symb = "EDISHOPCHARGEON";
	blk.P_CodeTempl = "EDISCO%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiStockOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_STOCK;
	blk.OpTypeID = PPOPT_DRAFTRECEIPT;
	blk.NameTxtId = PPTXT_OPK_EDI_STOCK;
	blk.Flags = OPKF_PASSIVE;
	blk.P_Symb = "EDISTOCK";
	blk.P_CodeTempl = "EDISTK%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiWrOffShopOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_WRITEOFFSHOP;
	blk.OpTypeID = PPOPT_DRAFTEXPEND;
	blk.NameTxtId = PPTXT_OPK_EDI_WRITEOFFSHOP;
	blk.Flags = 0;
	blk.P_Symb = "EDISHOPWROFF";
	blk.P_CodeTempl = "EDISWO%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiWrOffWithMarksOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_WROFFWITHMARKS;
	blk.OpTypeID = PPOPT_DRAFTEXPEND;
	blk.NameTxtId = PPTXT_OPK_EDI_WRITEOFFWITHMARKS;
	blk.Flags = 0;
	blk.P_Symb = "EDIWROFFWITHMARKS";
	blk.P_CodeTempl = "EDIWOM%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetEdiChargeOnWithMarksOp(PPID * pID, int use_ta)
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_EDI_CHARGEONWITHMARKS;
	blk.OpTypeID = PPOPT_DRAFTRECEIPT;
	blk.NameTxtId = PPTXT_OPK_EDI_CHARGEONWITHMARKS;
	blk.Flags = 0;
	blk.P_Symb = "EDICHRGONWITHMARKS";
	blk.P_CodeTempl = "EDICOM%05";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

int PPObjOprKind::GetGenericAccTurnForRegisterOp(PPID * pID, int use_ta) // @v12.1.4
{
	ReservedOpCreateBlock blk;
	blk.OpID = PPOPK_GENERICREGACCTURN;
	blk.OpTypeID = PPOPT_ACCTURN;
	blk.NameTxtId = PPTXT_OPK_GENERICREGACCTURN;
	blk.Flags = 0;
	blk.SubType = OPSUBT_REGISTER;
	blk.P_Symb = "GENERICREGACCTURN";
	blk.P_CodeTempl = "%06";
	return Helper_GetReservedOp(pID, blk, use_ta);
}

/*virtual*/int PPObjOprKind::MakeReserved(long flags)
{
    int    ok = -1;
    if(flags & mrfInitializeDb) {
		// PPTXT_OPK_COMM_GENERICACCTURN  "Общая бухгалтерская проводка"
		// PPTXT_OPK_COMM_RECEIPT         "Приход товара от поставщика"
		// PPTXT_OPK_COMM_SALE            "Продажа покупателю"
		// PPTXT_OPK_COMM_RETAIL          "Розничная продажа"
		// PPTXT_OPK_COMM_INTREXPEND      "Внутренняя передача"
		// PPTXT_OPK_COMM_INTRRECEIPT     "Межскладской приход"
		// PPTXT_OPK_COMM_INVENTORY       "Инвентаризация"
		// PPTXT_OPK_COMM_ORDER           "Заказ от покупателя"
		// PPTXT_OPK_COMM_PURCHASE        "Закупка"
		long    _count = 0;
		PPOprKind op_rec;
		{
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&op_rec) > 0;) {
				_count++;
			}
		}
        if(_count == 0) {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			PPIDArray acs_id_list;
			{
				ReservedOpCreateBlock blk;
				PPID   op_id = PPOPK_GENERICACCTURN;
				blk.OpID = PPOPK_GENERICACCTURN;
				blk.OpTypeID = PPOPT_ACCTURN;
				blk.NameTxtId = PPTXT_OPK_COMM_GENERICACCTURN;
				blk.Flags = 0;
				blk.P_Symb = "OP-GENATURN";
				blk.P_CodeTempl = "AT%05";
				THROW(Helper_GetReservedOp(&op_id, blk, 1));
			}
			{
				ReservedOpCreateBlock blk;
				PPID   op_id = PPOPK_RETAIL;
				blk.OpID = PPOPK_RETAIL;
				blk.OpTypeID = PPOPT_GOODSEXPEND;
				blk.NameTxtId = PPTXT_OPK_COMM_RETAIL;
				blk.Flags = OPKF_PROFITABLE|OPKF_GEXPEND|OPKF_SELLING;
				blk.P_Symb = "OP-RETAIL";
				blk.P_CodeTempl = "RTL%05";
				THROW(Helper_GetReservedOp(&op_id, blk, 1));
			}
			{
				acs_id_list.clear();
				{
					for(PPID acs_id = 0; acs_obj.EnumItems(&acs_id, &acs_rec) > 0;) {
						if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_SUPPL)
							acs_id_list.add(acs_rec.ID);
					}
				}
				if(acs_id_list.getCount()) {
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = PPOPK_RECEIPT;
						blk.OpID = PPOPK_RECEIPT;
						blk.OpTypeID = PPOPT_GOODSRECEIPT;
						blk.NameTxtId = PPTXT_OPK_COMM_RECEIPT;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_GRECEIPT|OPKF_BUYING|OPKF_NEEDPAYMENT;
						blk.P_Symb = "OP-RCPT";
						blk.P_CodeTempl = "RC%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = 0;
						blk.OpID = 0;
						blk.OpTypeID = PPOPT_DRAFTRECEIPT;
						blk.NameTxtId = PPTXT_OPK_COMM_PURCHASE;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_BUYING;
						blk.P_Symb = "OP-PURCHASE";
						blk.P_CodeTempl = "PCH%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
				}
			}
			{
				acs_id_list.clear();
				{
					for(PPID acs_id = 0; acs_obj.EnumItems(&acs_id, &acs_rec) > 0;) {
						if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup == PPPRK_CLIENT)
							acs_id_list.add(acs_rec.ID);
					}
				}
				if(acs_id_list.getCount()) {
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = PPOPK_SELL;
						blk.OpID = PPOPK_SELL;
						blk.OpTypeID = PPOPT_GOODSEXPEND;
						blk.NameTxtId = PPTXT_OPK_COMM_SALE;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_GEXPEND|OPKF_SELLING|OPKF_NEEDPAYMENT|OPKF_ONORDER|OPKF_FREIGHT;
						blk.P_Symb = "OP-SALE";
						blk.P_CodeTempl = "SL%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = blk.OpID = 0;
						blk.OpTypeID = PPOPT_GOODSORDER;
						blk.NameTxtId = PPTXT_OPK_COMM_ORDER;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_SELLING;
						blk.P_Symb = "OP-ORDER";
						blk.P_CodeTempl = "ORD%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
				}
			}
			{
				acs_id_list.clear();
				{
					for(PPID acs_id = 0; acs_obj.EnumItems(&acs_id, &acs_rec) > 0;) {
						if(acs_rec.Assoc == PPOBJ_LOCATION)
							acs_id_list.add(acs_rec.ID);
					}
				}
				if(acs_id_list.getCount()) {
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = PPOPK_INTREXPEND;
						blk.OpID = PPOPK_INTREXPEND;
						blk.OpTypeID = PPOPT_GOODSEXPEND;
						blk.NameTxtId = PPTXT_OPK_COMM_INTREXPEND;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_GEXPEND;
						blk.P_Symb = "OP-INTREXPEND";
						blk.P_CodeTempl = "IE%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
					{
						ReservedOpCreateBlock blk;
						PPID   op_id = PPOPK_INTRRECEIPT;
						blk.OpID = PPOPK_INTRRECEIPT;
						blk.OpTypeID = PPOPT_GOODSRECEIPT;
						blk.NameTxtId = PPTXT_OPK_COMM_INTRRECEIPT;
						blk.AccSheetID = acs_id_list.get(0);
						blk.Flags = OPKF_GRECEIPT;
						blk.P_Symb = "OP-INTRRCPT";
						blk.P_CodeTempl = "IR%05";
						THROW(Helper_GetReservedOp(&op_id, blk, 1));
					}
				}
			}
			ok = 1;
        }
    }
    CATCHZOK
    return ok;
}

/*virtual*/StrAssocArray * PPObjOprKind::MakeStrAssocList(void * extraPtr)
{
	const  PPID op_type_id = reinterpret_cast<const  PPID>(extraPtr);
	PPIDArray op_list;
	op_list.add(op_type_id);
	return MakeOprKindList(0, (op_type_id ? &op_list : 0), 0);
}

// Prototype
int EditInventoryOptionsDialog(PPInventoryOpEx *);
//
//
//
class OprKindView : public PPListDialog {
public:
	//
	// ARG(extDataKind IN):
	//   1 - extData as PPObjOprType,
	//   2 - extData as Linked PPObjOprKind
	//   3 - extData as OpCounter
	//
	OprKindView(PPID extData, int extDataKind);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pos, long * id);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	void   addBySample();

	const int ExtDataKind;
	PPID   OpTypeID;
	PPID   LinkOpID_;
	PPID   OpCounterID;
	PPObjOprKind OpObj;
};

OprKindView::OprKindView(PPID extData, int extDataKind) : PPListDialog(((extDataKind == 2) ? DLG_OPLINKSVIEW : DLG_OPKVIEW), CTL_OPKVIEW_LIST),
	ExtDataKind(extDataKind), OpTypeID(0), LinkOpID_(0)
{
	if(ExtDataKind == 3) {
		OpCounterID = extData;
		SetupPPObjCombo(this, CTLSEL_OPKVIEW_TYPE, PPOBJ_OPCOUNTER, OpCounterID, 0, 0);
	}
	else if(ExtDataKind == 2) {
		LinkOpID_ = extData;
		SetupOprKindCombo(this, CTLSEL_OPKVIEW_TYPE, LinkOpID_, 0, 0, OPKLF_SHOWPASSIVE);
	}
	else {
		OpTypeID = extData;
		SetupPPObjCombo(this, CTLSEL_OPKVIEW_TYPE, PPOBJ_OPRTYPE, OpTypeID, 0, 0);
	}
	updateList(-1);
}

IMPL_HANDLE_EVENT(OprKindView)
{
	PPListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_OPKVIEW_TYPE))
		updateList(0);
	else if(event.isCmd(cmOpListLinks)) {
		PPID   op_id = 0;
		if(getCurItem(0, &op_id) && op_id) {
			OprKindView * p_opkview = new OprKindView(op_id, 2);
			ExecViewAndDestroy(p_opkview);
		}
	}
	else if(event.isCmd(cmAddBySample))
		addBySample();
	else if(event.isKeyDown(kbAltF2))
		addBySample();
	else
		return;
	clearEvent(event);
}

void OprKindView::addBySample()
{
	if(P_Box) {
		PPID   id = 0, sample_id = 0;
		if(P_Box->getCurID(&sample_id) && sample_id)
			if(OpObj.AddBySample(&id, sample_id) == cmOK)
				updateList(-1);
	}
}

int OprKindView::setupList()
{
	int    ok = -1;
	PPIDArray op_list;
	StrAssocArray * p_ary = 0;
	PPID   ext_id = getCtrlLong(CTLSEL_OPKVIEW_TYPE);
	if(ExtDataKind == 3) {
		OpCounterID = ext_id;
		PPOprKind op_rec;
		for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;)
			if(op_rec.OpCounterID == OpCounterID)
				op_list.add(op_id);
		p_ary = PPObjOprKind::MakeOprKindList(LinkOpID_, &op_list, OPKLF_OPLIST | OPKLF_SHOWPASSIVE);
	}
	else if(ExtDataKind == 2) {
		LinkOpID_ = ext_id;
		if(LinkOpID_)
			p_ary = PPObjOprKind::MakeOprKindList(LinkOpID_, 0, OPKLF_SHOWPASSIVE);
		else
			ok = 1;
	}
	else {
		OpTypeID = ext_id;
		op_list.add(OpTypeID);
		p_ary = PPObjOprKind::MakeOprKindList(0, OpTypeID ? &op_list : 0, OPKLF_SHOWPASSIVE);
	}
	if(p_ary) {
		ok = 1;
		for(uint i = 0; i < p_ary->getCount(); i++) {
			StrAssocArray::Item item = p_ary->Get(i);
			if(!addStringToList(item.Id, item.Txt)) {
				ok = 0;
				break;
			}
		}
	}
	else if(ok > 0)
		ok = 0;
	delete p_ary;
	return ok;
}

int OprKindView::addItem(long *, long * pID)
{
	int    ok = -1;
	if(ExtDataKind == 3) {
		PPID      op_id = 0;
		PPOprKind op_rec;
		PPOprKindPacket pack;
		PPIDArray op_list;
		while(EnumOperations(0, &op_id, &op_rec) > 0)
			if(op_rec.OpCounterID != OpCounterID)
				op_list.add(op_id);
		op_id = SelectOpKind(0L, &op_list, OPKLF_OPLIST | OPKLF_SHOWPASSIVE);
		if(op_id > 0 && OpObj.GetPacket(op_id, &pack) > 0) {
			pack.Rec.OpCounterID = OpCounterID;
			if(OpObj.PutPacket(&op_id, &pack, 1)) {
				ASSIGN_PTR(pID, op_id);
				ok = 1;
			}
			else
				ok = PPErrorZ();
		}
	}
	else {
		PPID   obj_id = 0;
		int    r = OpObj.Edit(&obj_id, OpTypeID, LinkOpID_);
		if(r == cmOK) {
			ASSIGN_PTR(pID, obj_id);
			ok = 1;
		}
		else if(!r)
			ok = 0;
	}
	return ok;
}

int OprKindView::editItem(long, long id)
{
	if(ExtDataKind != 3) {
		int    r = id ? OpObj.Edit(&id, 0) : -1;
		if(r == cmOK)
			return 1;
		if(!r)
			return 0;
	}
	return -1;
}

int OprKindView::delItem(long, long id)
{
	int    ok = -1;
	if(ExtDataKind == 3) {
		PPOprKindPacket pack;
		if(id && OpObj.GetPacket(id, &pack) > 0) {
			pack.Rec.OpCounterID = 0;
			ok = OpObj.PutPacket(&id, &pack, 1) ? 1 : PPErrorZ();
		}
	}
	else
		ok = (id && OpObj.RemoveObjV(id, 0, PPObject::rmv_default, 0)) ? 1 : -1;
	return ok;
}
//
//
//
int PPObjOpCounter::Browse(void * extraPtr)
{
	class OpCounterView : public ObjViewDialog {
	public:
		explicit OpCounterView(PPObjOpCounter * pObj) : ObjViewDialog(DLG_OPCNTRVIEW, pObj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			if(id)
				ExecViewAndDestroy(new OprKindView(id, 3));
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new OpCounterView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}
//
//
//
class OprKindDialog : public TDialog {
public:
	OprKindDialog(uint rezID, PPOprKindPacket * pData) : TDialog(rezID), P_Data(pData), P_AtObj(BillObj->atobj)
	{
		P_ListBox = static_cast<SmartListBox *>(getCtrlView(CTL_OPRKIND_LIST));
		IsGeneric = BIN(P_Data->Rec.OpTypeID == PPOPT_GENERIC);
		IsDraft   = BIN(oneof4(P_Data->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ)); // @v10.5.7 PPOPT_DRAFTQUOTREQ
		if(P_ListBox) {
			if(!SetupStrListBox(P_ListBox))
				PPError();
		}
		setup();
	}
	int    getDTS(PPOprKindPacket *);
private:
	DECL_HANDLE_EVENT;
	void   addTempl();
	void   editTempl();
	void   delTempl();
	void   addGenOp();
	void   editGenOp();
	void   delGenOp();
	void   setup();
	void   setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID);
	void   updateList();
	void   moreDialog();
	void   editPaymList();
	void   editCounter();
	void   EditOptions2(uint dlgID, int useMainAmt, const PPIDArray * pSubTypeList, PPIDArray * pFlagsList, PPIDArray * pExtFlagsList);
	void   editInventoryOptions();
	void   editPoolOptions();
	void   editExtension();
	void   prnOptDialog();
	void   exAmountList();
	int    setAccTextToList(const AcctID & rAci, long, long, long, SString & rBuf);

	SmartListBox * P_ListBox;
	PPObjOprKind OpObj;
	PPObjAccTurn * P_AtObj;
	PPOprKindPacket * P_Data;
	int    IsGeneric;
	int    IsDraft;
};

void OprKindDialog::setup()
{
	PPIDArray types;
	SString temp_buf;
	const  int  modatt = OpObj.CheckRights(OPKRT_MODIFYATT);
	const  PPID op_type_id = P_Data->Rec.OpTypeID;
	setCtrlData(CTL_OPRKIND_NAME, P_Data->Rec.Name);
	setCtrlData(CTL_OPRKIND_SYMB, P_Data->Rec.Symb);
	{
		P_Data->GetExtStrData(OPKEXSTR_EXPSYMB, temp_buf);
		setCtrlString(CTL_OPRKIND_EXPSYMB, temp_buf);
	}
	setCtrlData(CTL_OPRKIND_ID,   &P_Data->Rec.ID);
	setCtrlData(CTL_OPRKIND_RANK, &P_Data->Rec.Rank);
	setCtrlUInt16(CTL_OPRKIND_PASSIVE, BIN(P_Data->Rec.Flags & OPKF_PASSIVE));
	setCtrlUInt16(CTL_OPRKIND_PAYMF, BIN(P_Data->Rec.Flags & OPKF_RECKON));
	disableCtrl(CTL_OPRKIND_ID,      (!PPMaster || P_Data->Rec.ID));
	disableCtrl(CTLSEL_OPRKIND_TYPE, P_Data->Rec.ID || op_type_id);
	enableCommand(cmOprKindPaymList, BIN(P_Data->Rec.Flags & OPKF_RECKON));
	if(!IsGeneric) {
		enableCommand(cmaInsert, modatt);
		enableCommand(cmaEdit,   modatt);
		enableCommand(cmaDelete, modatt);
	}
	enableCommand(cmaMore,   OpObj.CheckRights(OPKRT_MODIFYOPTIONS));
	enableCommand(cmOprKindPrnOpt, OpObj.CheckRights(OPKRT_MODIFYOPTIONS));
	disableCtrl(CTL_OPRKIND_CVAL, !OpObj.CheckRights(OPKRT_MODIFYCOUNTER));
	SetupPPObjCombo(this, CTLSEL_OPRKIND_TYPE, PPOBJ_OPRTYPE, op_type_id, OLW_CANINSERT, 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_ACCSHEET, PPOBJ_ACCSHEET, P_Data->Rec.AccSheetID, OLW_CANINSERT, 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_ACCSHEET2, PPOBJ_ACCSHEET, P_Data->Rec.AccSheet2ID, OLW_CANINSERT, 0);
	if(op_type_id == PPOPT_CORRECTION) {
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, 0L);
	}
	else if(op_type_id == PPOPT_DRAFTQUOTREQ) {
		types.addzlist(PPOPT_DRAFTQUOTREQ, 0L);
	}
	else if(oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
		; // nothing
	}
	else if(op_type_id == PPOPT_WAREHOUSE) { // @v12.4.1
		types.addzlist(PPOPT_WAREHOUSE, 0L);
	}
	else {
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, 0L);
		if(op_type_id == PPOPT_PAYMENT) {
			types.addzlist(PPOPT_ACCTURN, PPOPT_CHARGE, PPOPT_GOODSACK, PPOPT_POOL, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT,
				PPOPT_DRAFTEXPEND, PPOPT_CORRECTION, 0L);
		}
		else if(op_type_id == PPOPT_CHARGE)
			types.add(PPOPT_ACCTURN);
	}
	SetupOprKindCombo(this, CTLSEL_OPRKIND_LINK, P_Data->Rec.LinkOpID, 0, &types, P_Data->Rec.ID ? OPKLF_SHOWPASSIVE : 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_INITST, PPOBJ_BILLSTATUS, P_Data->Rec.InitStatusID, 0, 0);
	if(IsDraft) {
		PPDraftOpEx opex_rec;
		if(P_Data->P_DraftData)
			opex_rec = *P_Data->P_DraftData;
		types.clear();
		if(op_type_id == PPOPT_DRAFTRECEIPT)
			types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, 0L);
		else if(op_type_id == PPOPT_DRAFTEXPEND)
			types.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSMODIF, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, 0L);
		if(P_Data->Rec.SubType == OPSUBT_TRADEPLAN)
			types.add(PPOPT_GENERIC);
		types.add(PPOPT_ACCTURN);
		SetupOprKindCombo(this, CTLSEL_OPRKIND_WROFFOP, opex_rec.WrOffOpID, 0, &types, 0);
		setupAccSheet(CTLSEL_OPRKIND_WROFFOP, CTLSEL_OPRKIND_WROFFOBJ, opex_rec.WrOffObjID);
		types.freeAll();
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, 0L);
		SetupOprKindCombo(this, CTLSEL_OPRKIND_WROFFCOP, opex_rec.WrOffComplOpID, 0, &types, 0);
		disableCtrls(opex_rec.WrOffOpID ? 0 : 1, CTLSEL_OPRKIND_WROFFOBJ, 0);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 0, DROXF_CREMPTYBILL);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 1, DROXF_USEPARTSTRUC);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 2, DROXF_WROFFCURDATE);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 3, DROXF_DONTINHEXPIRY);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 4, DROXF_MULTWROFF);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 5, DROXF_MULTDRFTWROFF);
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 6, DROXF_SELSUPPLONCOMPL);
		SetClusterData(CTL_OPRKIND_DRFTOPTION, opex_rec.Flags);
	}
	enableCommand(cmOprKindExt, op_type_id == PPOPT_ACCTURN && P_Data->Rec.SubType == OPSUBT_DEBTINVENT);
	updateList();
}

void OprKindDialog::setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID)
{
	PPOprKind op_rec;
	GetOpData(getCtrlLong(opSelCtl), &op_rec);
	SetupArCombo(this, objSelCtl, arID, OLW_LOADDEFONOPEN, op_rec.AccSheetID, sacfDisableIfZeroSheet|sacfNonGeneric);
}

void OprKindDialog::addTempl()
{
	if(P_ListBox && OpObj.CheckRights(OPKRT_MODIFYATT)) {
		PPAccTurnTempl tmpl;
		MEMSZERO(tmpl);
		if(EditAccTurnTemplate(P_AtObj, &tmpl) > 0) {
			if(!P_Data->ATTmpls.insert(&tmpl))
				PPError(PPERR_SLIB, 0);
			else
				updateList();
		}
	}
}

void OprKindDialog::editTempl()
{
	if(P_ListBox && OpObj.CheckRights(OPKRT_MODIFYATT)) {
		PPID   id;
		if(P_ListBox->getCurID(&id) && id) {
			PPAccTurnTempl & r_tmpl = P_Data->ATTmpls.at((uint)(id-1));
			PPAccTurnTempl tmp(r_tmpl);
			if(EditAccTurnTemplate(P_AtObj, &tmp) > 0) {
				r_tmpl = tmp;
				updateList();
			}
		}
	}
}

void OprKindDialog::delTempl()
{
	PPID   id;
	if(P_ListBox && OpObj.CheckRights(OPKRT_MODIFYATT) && P_ListBox->getCurID(&id) && id)
		if(CONFIRM(PPCFM_DELETE)) {
			if(P_Data->ATTmpls.atFree((uint)(id-1)))
				updateList();
		}
}

static void MakeOpTypeListForGeneric(PPIDArray & rList)
{
	rList.clear();
	rList.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
		PPOPT_GOODSREVAL, PPOPT_GOODSORDER, PPOPT_GOODSMODIF, PPOPT_GOODSACK, PPOPT_PAYMENT,
		PPOPT_CHARGE, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_CORRECTION, PPOPT_DRAFTQUOTREQ, 0L);
}

static int AddGenOpItems(ObjRestrictArray & rList)
{
	int    ok = -1;
	PPIDArray dd_list;
	PPIDArray optype_list;
	MakeOpTypeListForGeneric(optype_list);
	StrAssocArray * p_src_list = PPObjOprKind::MakeOprKindList(0, &optype_list, 0);
	THROW(p_src_list);
	{
		uint   i;
		for(i = 0; i < rList.getCount(); i++) {
			dd_list.add(rList.at(i).ObjID);
		}
		ListToListData  ll_data(p_src_list, PPOBJ_OPRKIND, &dd_list);
		ll_data.TitleStrID = PPTXT_SELECTOPRKIND;
		THROW(ListToListDialog(&ll_data));
		dd_list.sortAndUndup();
		{
			i = rList.getCount();
			if(i) do {
				const  PPID id_to_remove = rList.at(--i).ObjID;
				if(!dd_list.bsearch(id_to_remove)) {
					rList.atFree(i);
					ok = 1;
				}
			} while(i);
		}
		for(i = 0; i < dd_list.getCount(); i++) {
			ObjRestrictItem new_item(dd_list.get(i), 0);
			if(rList.CheckUniqueID(new_item.ObjID)) {
				rList.insert(&new_item);
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete p_src_list;
	return ok;
}

static int EditGenOpItem(ObjRestrictItem * pItem)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GENOPITEM);
	if(CheckDialogPtrErr(&dlg)) {
		uint   fl = 0;
		PPIDArray optype_list;
		MakeOpTypeListForGeneric(optype_list);
		if(pItem->ObjID)
			fl |= OPKLF_SHOWPASSIVE;
		SetupOprKindCombo(dlg, CTLSEL_GENOPITEM_OP, pItem->ObjID, 0, &optype_list, fl);
		if(pItem->ObjID) {
			dlg->disableCtrl(CTLSEL_GENOPITEM_OP, true);
			dlg->selectCtrl(CTL_GENOPITEM_FLAGS);
		}
		dlg->AddClusterAssoc(CTL_GENOPITEM_FLAGS, 0, GOIF_NEGATIVE);
		dlg->SetClusterData(CTL_GENOPITEM_FLAGS, pItem->Flags);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->GetClusterData(CTL_GENOPITEM_FLAGS, &pItem->Flags);
			dlg->getCtrlData(CTLSEL_GENOPITEM_OP, &pItem->ObjID);
			if(pItem->ObjID == 0)
				PPErrorByDialog(dlg, CTLSEL_GENOPITEM_OP, PPERR_OPRKINDNEEDED);
			else
				ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

void OprKindDialog::addGenOp()
{
	if(IsGeneric && P_ListBox) {
		if(AddGenOpItems(*P_Data->P_GenList) > 0)
			updateList();
	}
}

void OprKindDialog::editGenOp()
{
	long   p = 0;
	if(IsGeneric && P_ListBox && P_Data->P_GenList && P_ListBox->getCurID(&p) && p > 0)
		if(EditGenOpItem(&P_Data->P_GenList->at((uint)(p-1))) > 0)
			updateList();
}

void OprKindDialog::delGenOp()
{
	long   p = 0;
	if(IsGeneric && P_ListBox && P_Data->P_GenList && P_ListBox->getCurID(&p) && p > 0) {
		P_Data->P_GenList->atFree((uint)(p-1));
		updateList();
	}
}

int OprKindDialog::setAccTextToList(const AcctID & rAci, long flgs, long accFixMask, long artFixMask, SString & rBuf)
{
	int    ok = 1;
	Acct   acct;
	PPID   cur_id = 0;
	if(P_AtObj->ConvertAcctID(rAci, &acct, &cur_id, 1 /* useCache */)) {
		acct.ToStr(MKSFMT(0, ACCF_DEFAULT | ALIGN_LEFT), rBuf);
		rBuf.Space().CatChar((flgs & accFixMask) ? 'X' : ' ').CatChar((flgs & artFixMask) ? 'X' : ' ');
	}
	else {
		ok = 0;
		rBuf.Z();
	}
	return ok;
}

void OprKindDialog::updateList()
{
	SString sub;
	if(P_ListBox) {
		P_ListBox->freeAll();
		if(IsGeneric) {
			if(P_Data->P_GenList) {
				ObjRestrictItem * p_ori;
				for(uint i = 0; P_Data->P_GenList->enumItems(&i, reinterpret_cast<void **>(&p_ori));) {
					StringSet ss(SLBColumnDelim);
					GetOpName(p_ori->ObjID, sub);
					ss.add(sub);
					sub.Z();
					if(p_ori->Flags & GOIF_NEGATIVE)
						sub.CatChar('-');
					ss.add(sub);
					THROW_SL(P_ListBox->addItem(i, ss.getBuf()));
				}
			}
		}
		else {
			PPAccTurnTempl * e;
			for(uint i = 0; P_Data->ATTmpls.enumItems(&i, reinterpret_cast<void **>(&e));) {
				StringSet ss(SLBColumnDelim);
				THROW(setAccTextToList(e->DbtID, e->Flags, ATTF_DACCFIX, ATTF_DARTFIX, sub));
				ss.add(sub);
				THROW(setAccTextToList(e->CrdID, e->Flags, ATTF_CACCFIX, ATTF_CARTFIX, sub));
				ss.add(sub);
				ss.add(e->Expr);
				THROW_SL(P_ListBox->addItem(i, ss.getBuf()));
			}
		}
		P_ListBox->Draw_();
	}
	CATCH
		PPError();
	ENDCATCH
}

int OprKindDialog::getDTS(PPOprKindPacket * /*_data*/)
{
	int    ok  = 1;
	uint   sel = 0;
	ushort v;
	SString temp_buf;
	getCtrlData(sel = CTL_OPRKIND_NAME,    P_Data->Rec.Name);
	THROW_PP(*strip(P_Data->Rec.Name), PPERR_NAMENEEDED);
	getCtrlData(CTL_OPRKIND_SYMB, P_Data->Rec.Symb);
	{
		getCtrlString(CTL_OPRKIND_EXPSYMB, temp_buf);
		P_Data->PutExtStrData(OPKEXSTR_EXPSYMB, temp_buf.Strip());
	}
	getCtrlData(CTL_OPRKIND_ID,            &P_Data->Rec.ID);
	getCtrlData(CTL_OPRKIND_RANK,          &P_Data->Rec.Rank);
	getCtrlData(sel = CTLSEL_OPRKIND_TYPE, &P_Data->Rec.OpTypeID);
	getCtrlData(CTLSEL_OPRKIND_ACCSHEET,   &P_Data->Rec.AccSheetID);
	getCtrlData(CTLSEL_OPRKIND_ACCSHEET2,  &P_Data->Rec.AccSheet2ID);
	getCtrlData(CTLSEL_OPRKIND_LINK,       &P_Data->Rec.LinkOpID);
	// @v11.0.0 {
	if(P_Data->Rec.LinkOpID) {
		GetOpName(P_Data->Rec.LinkOpID, temp_buf);
		THROW_PP_S(P_Data->Rec.LinkOpID != P_Data->Rec.ID, PPERR_OPRLINKRECUR, temp_buf);
	}
	// } @v11.0.0 
	getCtrlData(CTLSEL_OPRKIND_INITST,     &P_Data->Rec.InitStatusID);
	THROW_PP(P_Data->Rec.OpTypeID, PPERR_OPRTYPENEEDED);
	if(oneof3(P_Data->Rec.OpTypeID, PPOPT_PAYMENT, PPOPT_GOODSRETURN, PPOPT_CORRECTION))
		THROW_PP((sel = CTL_OPRKIND_LINK, P_Data->Rec.LinkOpID), PPERR_OPRLINKNEEDED);
	getCtrlData(CTL_OPRKIND_PAYMF, &(v = 0));
	SETFLAG(P_Data->Rec.Flags, OPKF_RECKON, (v & 1));
	getCtrlData(CTL_OPRKIND_PASSIVE, &(v = 0));
	SETFLAG(P_Data->Rec.Flags, OPKF_PASSIVE, (v & 1));
	if(IsDraft) {
		PPDraftOpEx opex_rec;
		getCtrlData(CTLSEL_OPRKIND_WROFFOP,  &opex_rec.WrOffOpID);
		getCtrlData(CTLSEL_OPRKIND_WROFFOBJ, &opex_rec.WrOffObjID);
		getCtrlData(CTLSEL_OPRKIND_WROFFCOP, &opex_rec.WrOffComplOpID);
		GetClusterData(CTL_OPRKIND_DRFTOPTION, &opex_rec.Flags);
		if(opex_rec.WrOffOpID || opex_rec.WrOffComplOpID) {
			if(P_Data->P_DraftData == 0)
				THROW_MEM(P_Data->P_DraftData = new PPDraftOpEx);
			*P_Data->P_DraftData = opex_rec;
		}
	}
	CATCHZOKPPERRBYDLG
	return ok;
}

void OprKindDialog::prnOptDialog()
{
	ushort v = 0;
	long   f = P_Data->Rec.PrnFlags;
	SString prn_form_name;
	if(OpObj.CheckRights(OPKRT_MODIFYOPTIONS)) {
		TDialog * dlg = new TDialog(DLG_OPKPRNOPT);
		if(CheckDialogPtrErr(&dlg)) {
			if(f & OPKF_PRT_BUYING)   v |= 0x0001;
			if(f & OPKF_PRT_SELLING)  v |= 0x0002;
			dlg->setCtrlData(CTL_OPKMORE_PRTAMT, &v);

			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 0, OPKF_PRT_QCERT);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 1, OPKF_PRT_INVOICE);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 2, OPKF_PRT_CASHORD);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 3, OPKF_PRT_LADING);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 4, OPKF_PRT_CHECK);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 5, OPKF_PRT_SRVACT);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 6, OPKF_PRT_PLABEL);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 7, OPKF_PRT_PAYPLAN);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 8, OPKF_PRT_TARESALDO);
			dlg->AddClusterAssoc(CTL_OPKMORE_WHAT, 9, OPKF_PRT_LOTTAGIMG);
			dlg->SetClusterData(CTL_OPKMORE_WHAT, f);

			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  0, OPKF_PRT_NBILLN);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  1, OPKF_PRT_VATAX);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  2, OPKF_PRT_QCG);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  3, OPKF_PRT_SHRTORG);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  4, OPKF_PRT_SELPRICE);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  5, OPKF_PRT_NDISCNT);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  6, OPKF_PRT_INCINVC);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  7, OPKF_PRT_NEGINVC);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  8, OPKF_PRT_MERGETI);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC,  9, OPKF_PRT_CHECKTI);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC, 10, OPKF_PRT_EXTOBJ2OBJ);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC, 11, OPKF_PRT_BCODELIST);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTQC, 12, OPKF_PRT_QCERTLIST);
			dlg->SetClusterData(CTL_OPKMORE_PRTQC, f);

			P_Data->GetExtStrData(OPKEXSTR_DEFPRNFORM, prn_form_name);
			dlg->setCtrlString(CTL_OPKMORE_PRNFORM, prn_form_name);

			dlg->AddClusterAssocDef(CTL_OPKMORE_PRTORD,  0, TiIter::ordDefault);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  1, TiIter::ordByGoods);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  2, TiIter::ordByGrpGoods);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  3, TiIter::ordByBarcode);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  4, TiIter::ordBySuppl);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  5, TiIter::ordByLocation);
			dlg->AddClusterAssoc(CTL_OPKMORE_PRTORD,  6, TiIter::ordByStorePlaceGrpGoods);
			dlg->SetClusterData(CTL_OPKMORE_PRTORD, P_Data->Rec.PrnOrder);
			if(ExecView(dlg) == cmOK) {
				long   temp_long = 0;
				dlg->getCtrlData(CTL_OPKMORE_PRTAMT, &v);
				f &= ~(OPKF_PRT_BUYING | OPKF_PRT_SELLING);
				if(v & 1)
					f |= OPKF_PRT_BUYING;
				if(v & 2)
					f |= OPKF_PRT_SELLING;
				dlg->GetClusterData(CTL_OPKMORE_WHAT,  &f);
				dlg->GetClusterData(CTL_OPKMORE_PRTQC, &f);
				P_Data->Rec.PrnFlags = f;
				if(dlg->GetClusterData(CTL_OPKMORE_PRTORD, &temp_long))
					P_Data->Rec.PrnOrder = static_cast<int16>(temp_long);
				dlg->getCtrlString(CTL_OPKMORE_PRNFORM, prn_form_name);
				P_Data->PutExtStrData(OPKEXSTR_DEFPRNFORM, prn_form_name.Strip());
			}
			delete dlg;
		}
	}
}
//
//
//
void OprKindDialog::EditOptions2(uint dlgID, int useMainAmt, const PPIDArray * pSubTypeList, PPIDArray * pFlagsList, PPIDArray * pExtFlagsList)
{
	PPIDArray options;
	PPIDArray ext_options;
	int    valid_data = 0;
	uint   i;
	int    warn_noupdlotrestflagdisabled = 0;
	ushort v = 0;
	long   s;
	long   b;
	long   f = P_Data->Rec.Flags;
	long   ext_f = P_Data->Rec.ExtFlags;
	ulong  o;
	ulong  ext_o;
	TDialog * dlg = 0;
	SString memo_tmpl;
	SString obj2name;
	SString amt_formula;
	RVALUEPTR(options, pFlagsList);
	RVALUEPTR(ext_options, pExtFlagsList);
	if(CheckDialogPtrErr(&(dlg = new TDialog(dlgID)))) {
		v = 0;
		for(i = 0; i < options.getCount(); i++) {
			o = static_cast<ulong>(options.at(i));
			SETFLAG(v, (1 << i), f & o);
		}
		dlg->setCtrlUInt16(CTL_OPKMORE_FLAGS, v);
		//@erik v10.5.9 {
		long paym_type_flg = 0;
		const bool is_cash_f = ext_options.lsearch(OPKFX_PAYMENT_CASH);
		const bool is_bank_f = ext_options.lsearch(OPKFX_PAYMENT_NONCASH);
		assert((is_cash_f && is_bank_f) || (!is_cash_f && !is_bank_f));
		if(is_cash_f && is_bank_f) {
			paym_type_flg = 1; // если флаги есть, то не придется выполнять поиск еще раз. Далее просто проверим значение этой переменной
			dlg->AddClusterAssocDef(CTL_OPKMORE_PAYMTYPE, 0, 0);
			dlg->AddClusterAssoc(CTL_OPKMORE_PAYMTYPE, 1, OPKFX_PAYMENT_CASH);
			dlg->AddClusterAssoc(CTL_OPKMORE_PAYMTYPE, 2, OPKFX_PAYMENT_NONCASH);
			const long __p = CHKXORFLAGS(ext_f, OPKFX_PAYMENT_CASH, OPKFX_PAYMENT_NONCASH);
			dlg->SetClusterData(CTL_OPKMORE_PAYMTYPE, __p);
		}
		// } @erik
		v = 0;
		for(i = 0; i < ext_options.getCount(); i++) {
			ext_o = (ulong)ext_options.at(i);
			if(!oneof2(ext_o, OPKFX_PAYMENT_CASH, OPKFX_PAYMENT_NONCASH)) //@erik v10.5.9
				SETFLAG(v, (1 << i), ext_f & ext_o);
		}

		dlg->setCtrlUInt16(CTL_OPKMORE_EXTFLAGS, v);
		if(useMainAmt) {
			s = (f & OPKF_SELLING);
			b = (f & OPKF_BUYING);
			if(useMainAmt == 1)
				v = (s ^ b) ? (b ? 1 : 2) : 0;
			else if(s)
				v = 1;
			else
				v = 0;
			dlg->setCtrlData(CTL_OPKMORE_AMOUNT, &v);
			// ahtoxa {
			P_Data->GetExtStrData(OPKEXSTR_AMTFORMULA, amt_formula);
			dlg->setCtrlString(CTL_OPKMORE_AMTFORMULA, amt_formula);
			// } ahtoxa
		}
		P_Data->GetExtStrData(OPKEXSTR_MEMO, memo_tmpl);
		P_Data->GetExtStrData(OPKEXSTR_OBJ2NAME, obj2name);
		dlg->setCtrlString(CTL_OPKMORE_MEMO,     memo_tmpl);
		dlg->setCtrlString(CTL_OPKMORE_OBJ2NAME, obj2name);
		if(dlg->getCtrlView(CTLSEL_OPKMORE_DEFLOC))
			SetupPPObjCombo(dlg, CTLSEL_OPKMORE_DEFLOC, PPOBJ_LOCATION, P_Data->Rec.DefLocID, 0, 0);
		if(dlg->getCtrlView(CTL_OPKMORE_SUBTYPE) && pSubTypeList) {
			v = 0;
			if(pSubTypeList->lsearch(P_Data->Rec.SubType, &(i = 0)))
				v = i;
			dlg->setCtrlData(CTL_OPKMORE_SUBTYPE, &v);
		}
		if(dlg->getCtrlView(CTL_OPKMORE_MCR)) {
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 0, OPKFX_MCR_GROUP);
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 1, OPKFX_MCR_SUBSTSTRUC);
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 2, OPKFX_MCR_EQQTTY);
			dlg->SetClusterData(CTL_OPKMORE_MCR, ext_f);
		}
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_OPKMORE_FLAGS,  &(v = 0));
			for(i = 0; i < options.getCount(); i++) {
				o = static_cast<ulong>(options.at(i));
				SETFLAG(f, o, v & (1 << i));
			}
			f &= ~(OPKF_BUYING | OPKF_SELLING);
			dlg->getCtrlData(CTL_OPKMORE_EXTFLAGS,  &(v = 0));
			for(i = 0; i < ext_options.getCount(); i++) {
				ext_o = static_cast<ulong>(ext_options.at(i));
				if(oneof2(ext_o, OPKFX_PAYMENT_CASH, OPKFX_PAYMENT_NONCASH)) {
					continue;
				}
				SETFLAG(ext_f, ext_o, v & (1 << i));
			}
			if(dlg->getCtrlView(CTL_OPKMORE_MCR)) {
				dlg->GetClusterData(CTL_OPKMORE_MCR, &ext_f);
			}
			//@erik v10.5.9 { 
			if(paym_type_flg > 0) {
				ext_f &= ~(OPKFX_PAYMENT_CASH|OPKFX_PAYMENT_NONCASH);
				ext_f |= dlg->GetClusterData(CTL_OPKMORE_PAYMTYPE);
			}				
			// } @erik
			if(useMainAmt) {
				// ahtoxa {
				PPBillPacket bill_pack;
				SString temp_formula;
				dlg->getCtrlString(CTL_OPKMORE_AMTFORMULA, amt_formula);
				amt_formula.Strip();
				if(amt_formula.HasPrefixIAscii("ignfix"))
					(temp_formula = amt_formula).ShiftLeft(sstrlen("ignfix")).Strip();
				else
					temp_formula = amt_formula;
				if(amt_formula.Strip().IsEmpty() || PPCalcExpression(temp_formula, 0, &bill_pack, 0, 0) > 0) {
					P_Data->PutExtStrData(OPKEXSTR_AMTFORMULA, amt_formula);
					valid_data = 1;
					dlg->getCtrlData(CTL_OPKMORE_AMOUNT, &v);
					if(useMainAmt == 1) {
						if(v == 1)
							f |= OPKF_BUYING;
						else if(v == 2)
							f |= OPKF_SELLING;
					}
					else if(f & OPKF_CURTRANSIT)
						if(v == 1)
							f |= OPKF_SELLING;
						else
							f |= OPKF_BUYING;
				}
				else
					PPErrorByDialog(dlg, CTL_OPKMORE_AMTFORMULA);
				// } ahtoxa
			}
			else
				valid_data = 1;
			if(valid_data) {
				if(!(CConfig.Flags & CCFLG_USENOUPDRESTOPFLAG)) {
					if(f & OPKF_NOUPDLOTREST) {
						warn_noupdlotrestflagdisabled = 1;
						f &= ~OPKF_NOUPDLOTREST;
					}
				}
				P_Data->Rec.Flags = f;
				P_Data->Rec.ExtFlags = ext_f;
				if(dlg->getCtrlView(CTLSEL_OPKMORE_DEFLOC))
					dlg->getCtrlData(CTLSEL_OPKMORE_DEFLOC, &P_Data->Rec.DefLocID);
				dlg->getCtrlString(CTL_OPKMORE_MEMO, memo_tmpl);
				dlg->getCtrlString(CTL_OPKMORE_OBJ2NAME, obj2name);
				if(dlg->getCtrlView(CTL_OPKMORE_SUBTYPE) && pSubTypeList) {
					dlg->getCtrlData(CTL_OPKMORE_SUBTYPE, &(v = 0));
					P_Data->Rec.SubType = (v < pSubTypeList->getCount()) ? (int16)pSubTypeList->at(v) : 0;
				}
				P_Data->PutExtStrData(OPKEXSTR_MEMO, memo_tmpl.Strip());
				P_Data->PutExtStrData(OPKEXSTR_OBJ2NAME, obj2name.Strip());
			}
		}
		delete dlg;
	}
	if(warn_noupdlotrestflagdisabled)
		PPMessage(mfInfo, PPINF_NOUPDRESTOPFLAGDISABLED);
	enableCommand(cmOprKindExt, P_Data->Rec.OpTypeID == PPOPT_ACCTURN && P_Data->Rec.SubType == OPSUBT_DEBTINVENT);
}

void OprKindDialog::editInventoryOptions()
{
	PPInventoryOpEx ioe;
	RVALUEPTR(ioe, P_Data->P_IOE);
	if(EditInventoryOptionsDialog(&ioe) > 0) {
		SETIFZ(P_Data->P_IOE, new PPInventoryOpEx);
		*P_Data->P_IOE = ioe;
	}
}

void OprKindDialog::moreDialog()
{
	PPIDArray subtypelist, options_list, ext_options_list;
	if(OpObj.CheckRights(OPKRT_MODIFYOPTIONS)) {
		switch(P_Data->Rec.OpTypeID) {
			case PPOPT_ACCTURN:
			case PPOPT_AGREEMENT:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_ADVANCEREP, OPSUBT_REGISTER,
					OPSUBT_WARRANT, OPSUBT_DEBTINVENT, OPSUBT_ACCWROFF, OPSUBT_POSCORRECTION, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_AUTOWL, OPKF_EXTACCTURN,
					OPKF_RENT, OPKF_BANKING, OPKF_CURTRANSIT,
					OPKF_PROFITABLE, OPKF_OUTBALACCTURN, OPKF_ADVACC, OPKF_ATTACHFILES, OPKF_FREIGHT, OPKF_USEEXT, 0L); // @v12.3.4 OPKF_USEEXT
				ext_options_list.addzlist(OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, OPKFX_ACCAUTOVAT, OPKFX_SETCTXAGENT, 0L); // @v11.6.6 OPKFX_ACCAUTOVAT // @v12.3.4 OPKFX_SETCTXAGENT
				EditOptions2(DLG_OPKMORE_AT, 2, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSEXPEND:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES, OPKF_AUTOWL,
					OPKF_USEEXT, OPKF_RENT, OPKF_FREIGHT, OPKF_NOCALCTIORD, OPKF_NOUPDLOTREST, OPKF_PCKGMOUNTING,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L); 
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, 
					OPKFX_IGNORECLISTOP, OPKFX_SETCTXAGENT, 0L); // @v12.3.4 OPKFX_SETCTXAGENT
				EditOptions2(DLG_OPKMORE_GEX, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSRECEIPT:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_ASSETRCV, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES,
					OPKF_AUTOWL, OPKF_USEEXT, OPKF_RENT, OPKF_FREIGHT, OPKF_NOUPDLOTREST,
					OPKF_ATTACHFILES, OPKF_NEEDVALUATION, OPKF_RESTRICTBYMTX, OPKF_NOCALCTIORD, 0L); // @v11.0.10 OPKF_NOCALCTIORD
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_UNLINKRET,
					OPKFX_DLVRLOCASWH, OPKFX_AUTOGENUUID, OPKFX_SETCTXAGENT, 0L); // @v12.3.4
				EditOptions2(DLG_OPKMORE_GRC, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSMODIF:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_ASSETMODIF, 0L);
				options_list.addzlist(OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT, OPKF_NOCALCTIORD,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_DSBLHALFMODIF, OPKFX_SOURCESERIAL, OPKFX_AUTOGENUUID, 0L);
				EditOptions2(DLG_OPKMORE_MDF, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSRETURN:
				options_list.addzlist(OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_FREIGHT, OPKF_NOUPDLOTREST,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, 0L); 
				EditOptions2(DLG_OPKMORE_RET, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSREVAL:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_ASSETEXPL, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_ATTACHFILES,
					OPKF_DENYREVALCOST, OPKF_RESTRICTBYMTX, 0L);
				EditOptions2(DLG_OPKMORE_GRV, 1, &subtypelist, &options_list, 0);
				break;
			case PPOPT_GOODSORDER:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT, OPKF_RENT,
					OPKF_FREIGHT, OPKF_ORDEXSTONLY, OPKF_ORDRESERVE, OPKF_ORDERBYLOC, OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, OPKF_NOCALCTIORD, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_IGNORECLISTOP,
					OPKFX_AUTOGENUUID, OPKFX_WROFFTODRAFTORD, OPKFX_CANBEDECLINED, OPKFX_MNGPREFSUPPL, OPKFX_SETCTXAGENT, 0L); // @v12.0.8 OPKFX_MNGPREFSUPPL // @v12.3.4 OPKFX_SETCTXAGENT
				EditOptions2(DLG_OPKMORE_ORD, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_PAYMENT:
				options_list.addzlist(OPKF_PROFITABLE, OPKF_AUTOWL, OPKF_BANKING, OPKF_ATTACHFILES, 0L);
				// @attention: опции OPKFX_PAYMENT_CASH и OPKFX_PAYMENT_NONCASH обрабатываются специальным образом и
				// должны быть строго в конце списка!
				ext_options_list.addzlist(OPKFX_SETCTXAGENT, OPKFX_PAYMENT_CASH, OPKFX_PAYMENT_NONCASH, 0L); //@erik v10.5.9 // @v12.3.6 OPKFX_SETCTXAGENT
				EditOptions2(DLG_OPKMORE_PAY, 0, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_CHARGE:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_AUTOWL, OPKF_USEEXT, OPKF_CHARGENEGPAYM, OPKF_ATTACHFILES, 0L);
				EditOptions2(DLG_OPKMORE_CHG, 0, 0, &options_list, 0);
				break;
			case PPOPT_INVENTORY:
				editInventoryOptions();
				break;
			case PPOPT_POOL:
				editPoolOptions();
				break;
			case PPOPT_DRAFTRECEIPT:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_TRADEPLAN, OPSUBT_RETURNREQ, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT,
					OPKF_FREIGHT, OPKF_ATTACHFILES, OPKF_NEEDVALUATION, OPKF_RESTRICTBYMTX, OPKF_NOCALCTIORD, 0L); // @v11.0.10 OPKF_NOCALCTIORD
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_USESUPPLDEAL, OPKFX_CANBEDECLINED,
					OPKFX_DLVRLOCASWH, OPKFX_SOURCESERIAL, OPKFX_AUTOGENUUID, 0L);
				EditOptions2(DLG_OPKMORE_DRC, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_DRAFTEXPEND:
				subtypelist.addzlist(static_cast<long>(OPSUBT_COMMON), OPSUBT_TRADEPLAN, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT,
					OPKF_FREIGHT, OPKF_NOCALCTIORD, OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, OPKFX_IGNORECLISTOP, 0L);
				EditOptions2(DLG_OPKMORE_DEX, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_WAREHOUSE:
				subtypelist.addzlist(OPSUBT_GENERALWMSOP, OPSUBT_BAILMENT_ORDER, OPSUBT_BAILMENT_PUT, OPSUBT_BAILMENT_GET, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT,
					OPKF_EXTACCTURN, OPKF_RENT, OPKF_NOCALCTIORD, OPKF_FREIGHT, OPKF_ATTACHFILES, 0L);
				EditOptions2(DLG_OPKMORE_WH, 1, &subtypelist, &options_list, 0);
				break;
			default:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEEXT,
					OPKF_EXTACCTURN, OPKF_RENT, OPKF_NOCALCTIORD, OPKF_FREIGHT, OPKF_ATTACHFILES, 0L);
				EditOptions2(DLG_OPKMORE, 1, 0, &options_list, 0);
				break;
		}
	}
}

void OprKindDialog::exAmountList()
{
	ListToListData lld(PPOBJ_AMOUNTTYPE, 0, &P_Data->Amounts);
	lld.Flags  |= ListToListData::fCanInsertNewItem;
	lld.TitleStrID = PPTXT_SELOPAMOUNTS;
	if(!ListToListDialog(&lld))
		PPError();
}

void OprKindDialog::editExtension()
{
	TDialog * dlg = 0;
	if(P_Data->Rec.OpTypeID == PPOPT_ACCTURN && P_Data->Rec.SubType == OPSUBT_DEBTINVENT)
		if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_OPKDINVE)))) {
			PPDebtInventOpEx rec;
			if(!RVALUEPTR(rec, P_Data->P_DIOE))
				MEMSZERO(rec);
			PPIDArray op_type_list;
			GoodsCtrlGroup::Rec wd_goods_rec(0, rec.WrDnGoodsID, 0, GoodsCtrlGroup::enableInsertGoods);
			GoodsCtrlGroup::Rec wu_goods_rec(0, rec.WrUpGoodsID, 0, GoodsCtrlGroup::enableInsertGoods);
			dlg->addGroup(1, new GoodsCtrlGroup(CTLSEL_OPKDINVE_WD_GRP, CTLSEL_OPKDINVE_WD_GOODS));
			dlg->addGroup(2, new GoodsCtrlGroup(CTLSEL_OPKDINVE_WU_GRP, CTLSEL_OPKDINVE_WU_GOODS));
			op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, 0);
			SetupOprKindCombo(dlg, CTLSEL_OPKDINVE_WRDNOP, rec.WrDnOp, 0, &op_type_list, 0);
			dlg->setGroupData(1, &wd_goods_rec);
			dlg->setGroupData(2, &wu_goods_rec);
			SetupPPObjCombo(dlg, CTLSEL_OPKDINVE_WRUPOP, PPOBJ_OPRKIND, rec.WrUpOp, 0, reinterpret_cast<void *>(PPOPT_ACCTURN));
			if(ExecView(dlg) == cmOK) {
				dlg->getCtrlData(CTLSEL_OPKDINVE_WRDNOP, &rec.WrDnOp);
				dlg->getGroupData(1, &wd_goods_rec);
				rec.WrDnGoodsID = wd_goods_rec.GoodsID;
				dlg->getCtrlData(CTLSEL_OPKDINVE_WRUPOP, &rec.WrUpOp);
				dlg->getGroupData(2, &wu_goods_rec);
				rec.WrUpGoodsID = wu_goods_rec.GoodsID;
				SETIFZ(P_Data->P_DIOE, new PPDebtInventOpEx);
				*P_Data->P_DIOE = rec;
			}
		}
	delete dlg;
}

IMPL_HANDLE_EVENT(OprKindDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_OPRKIND_PAYMF)) {
		enableCommand(cmOprKindPaymList, (getCtrlUInt16(CTL_OPRKIND_PAYMF) & 1));
		clearEvent(event);
	}
	else if(event.isCbSelected(CTLSEL_OPRKIND_WROFFOP)) {
		PPID   op_id = getCtrlLong(CTLSEL_OPRKIND_WROFFOP);
		setupAccSheet(CTLSEL_OPRKIND_WROFFOP, CTLSEL_OPRKIND_WROFFOBJ, 0);
		disableCtrls(op_id ? 0 : 1, CTLSEL_OPRKIND_WROFFOBJ, 0);
		clearEvent(event);
	}
	else if(TVCOMMAND) {
		switch(TVCMD) {
			case cmaInsert: IsGeneric ? addGenOp() : addTempl(); break;
			case cmaDelete: IsGeneric ? delGenOp() : delTempl(); break;
			case cmLBDblClk:
			case cmaEdit:   IsGeneric ? editGenOp() : editTempl(); break;
			case cmaMore:   moreDialog(); break;
			case cmOprKindPrnOpt: prnOptDialog(); break;
			case cmOprKindExAmt: exAmountList(); break;
			case cmOprKindPaymList: editPaymList(); break;
			case cmOprKindCounter: editCounter(); break;
			case cmOprKindExt: editExtension(); break;
			case cmEditOp:
				{
					long   p = 0;
					if(IsGeneric && P_ListBox && P_Data->P_GenList && P_ListBox->getCurID(&p) && p > 0) {
						ObjRestrictItem item = P_Data->P_GenList->at((uint)(p-1));
						if(OpObj.Edit(&item.ObjID, 0, 0) == cmOK)
							updateList();
					}
				}
				break;
			default:
				return;
		}
		clearEvent(event);
	}
	else if(TVBROADCAST) {
		if(TVINFOVIEW && TVINFOVIEW == P_ListBox && OpObj.CheckRights(OPKRT_MODIFYATT)) {
			enum { ok_button, edit_button } toggle;
			if(TVCMD == cmReceivedFocus)
				toggle = edit_button;
			else if(TVCMD == cmReleasedFocus)
				toggle = ok_button;
			else
				return;
			SetDefaultButton(STDCTL_OKBUTTON,   toggle == ok_button);
			SetDefaultButton(STDCTL_EDITBUTTON, toggle == edit_button);
		}
	}
}
//
//
//
class OpListDialog : public PPListDialog {
public:
	OpListDialog(uint dlgID, uint listCtlID, PPIDArray * pOpList, PPIDArray * pOpTypesList) : PPListDialog(dlgID, listCtlID)
	{
		RVALUEPTR(OpTypesList, pOpTypesList);
		if(SmartListBox::IsValidS(P_Box))
			P_Box->P_Def->SetOption(lbtFocNotify, 1);
		setOpList(pOpList);
	}
protected:
	void   setOpList(const PPIDArray *);
	void   getOpList(PPIDArray *);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int addItem(long * pos, long * id);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	PPIDArray OpListData;
	PPIDArray OpTypesList;
};

IMPL_HANDLE_EVENT(OpListDialog)
{
	if(event.isCmd(cmaEdit)) {
		long p, id;
		if(getCurItem(&p, &id) && id > 0 && editItem(p, id) > 0)
			updateList(p);
	}
	else {
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmaAltInsert)) {
			if(editItem(0, 0) > 0)
				updateList(-1);
		}
		else
			return;
	}
	clearEvent(event);
}

void OpListDialog::setOpList(const PPIDArray * pOpList)
{
	if(!RVALUEPTR(OpListData, pOpList))
		OpListData.freeAll();
	updateList(-1);
}

void OpListDialog::getOpList(PPIDArray * pOpList)
{
	CALLPTRMEMB(pOpList, copy(OpListData));
}

int OpListDialog::setupList()
{
	SString name_buf;
	for(uint i = 0; i < OpListData.getCount(); i++) {
		const  PPID id = OpListData.at(i);
		if(GetObjectName(PPOBJ_OPRKIND, id, name_buf) <= 0)
			ideqvalstr(id, name_buf);
		if(!addStringToList(id, name_buf))
			return 0;
	}
	return 1;
}

int OpListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	if(P_Box) {
		PPID   op_id = SelectOpKind(0L, &OpTypesList, 0);
		if(op_id > 0) {
			int    dup = 0;
			for(uint i = 0; !dup && i < OpListData.getCount(); i++)
				if(OpListData.at(i) == op_id)
					dup = 1;
			if(!dup) {
				OpListData.add(op_id);
				ASSIGN_PTR(pPos, OpListData.getCount());
				ASSIGN_PTR(pID, op_id);
				ok = 1;
			}
		}
	}
	return ok;
}

int OpListDialog::editItem(long /*pos*/, long id)
{
	if(P_Box) {
		int    r = 0;
		const  bool is_new = (id == 0);
		PPObjOprKind op_obj;
		if((r = op_obj.Edit(&id, 0, 0)) == cmCancel)
			r = -1;
		else if(r == cmOK) {
			r = 1;
			if(is_new)
				OpListData.add(id);
		}
		return r;
	}
	return -1;
}

int OpListDialog::delItem(long pos, long /*id*/)
{
	int    ok = 0;
	if(pos >= 0 && pos < static_cast<long>(OpListData.getCount())) {
		OpListData.atFree(static_cast<uint>(pos));
		ok = 1;
	}
	return ok;
}
//
//
//
void OprKindDialog::editPaymList()
{
	class OpkPaymListDialog : public OpListDialog {
	public:
		OpkPaymListDialog(PPReckonOpEx * pData, PPIDArray * pOpTypesList) : OpListDialog(DLG_OPRPOP, CTL_OPRPOP_LIST, &pData->OpList, pOpTypesList)
		{
			setDTS(pData);
		}
		int    setDTS(const PPReckonOpEx * pData)
		{
			SString temp_buf;
			data = *pData;
			data.PeriodToStr(temp_buf);
			setCtrlString(CTL_OPRPOP_PERIOD, temp_buf);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 0, ROXF_AUTOPAYM);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 1, ROXF_CFM_PAYM);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 2, ROXF_AUTODEBT);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 3, ROXF_CFM_DEBT);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 4, ROXF_THISLOCONLY);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 5, ROXF_BYEXTOBJ);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 6, ROXF_REQALTOBJ);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 7, ROXF_THISALTOBJONLY);
			AddClusterAssoc(CTL_OPRPOP_FLAGS, 8, ROXF_RECKONNEGONLY);
			SetClusterData(CTL_OPRPOP_FLAGS, data.Flags);
			SetupPPObjCombo(this, CTLSEL_OPRPOP_PSNRELTYPE, PPOBJ_PERSONRELTYPE, data.PersonRelTypeID, 0, 0);
			setOpList(&data.OpList);
			return 1;
		}
		int    getDTS(PPReckonOpEx * pData)
		{
			char   temp[64];
			getOpList(&data.OpList);
			GetClusterData(CTL_OPRPOP_FLAGS, &data.Flags);
			getCtrlData(CTL_OPRPOP_PERIOD, temp);
			data.StrToPeriod(temp);
			getCtrlData(CTLSEL_OPRPOP_PSNRELTYPE, &data.PersonRelTypeID);
			ASSIGN_PTR(pData, data);
			return 1;
		}
	private:
		PPReckonOpEx data;
	};
	OpkPaymListDialog * dlg = 0;
	ushort v = 0;
	getCtrlData(CTL_OPRKIND_PAYMF, &v);
	if(v & 1/*data->Rec.Flags & OPKF_RECKON*/) {
		PPID   op_id = PPOPT_PAYMENT;
		PPIDArray op_types_list;
		op_types_list.add(op_id);
		SETIFZ(P_Data->P_ReckonData, new PPReckonOpEx);
		dlg = new OpkPaymListDialog(P_Data->P_ReckonData, &op_types_list);
		if(CheckDialogPtrErr(&dlg)) {
			if(ExecView(dlg) == cmOK)
				dlg->getDTS(P_Data->P_ReckonData);
			delete dlg;
		}
	}
}

class DiffByLocCntrDlg : public PPListDialog {
	DECL_DIALOG_DATA(LAssocArray);
public:
	DiffByLocCntrDlg() : PPListDialog(DLG_DIFFCNTR, CTL_DIFFCNTR_LOCLIST), LocID(0)
	{
		/*updateList(-1);*/
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.freeAll();
		setupList();
		if(SmartListBox::IsValidS(P_Box)) {
			long   val = 0;
			Data.Search(LocID = LConfig.Location, &val, 0);
			P_Box->Search_(&LocID, CMPF_LONG, lbSrchByID);
			setCtrlData(CTL_DIFFCNTR_VAL, &val);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		setupCounter(0);
		CALLPTRMEMB(pData, copy(Data));
		return 1;
	}
	void   setupCounter(PPID locID);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	PPID   getCurrID()
	{
		PPID   id = 0;
		return (P_Box && P_Box->getCurID(&id)) ? id : 0;
	}
	PPObjLocation LObj;
	PPID   LocID;
};

IMPL_HANDLE_EVENT(DiffByLocCntrDlg)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmLBItemFocused)) {
		const  PPID loc_id = getCurrID();
		if(loc_id)
			setupCounter(loc_id);
	}
	else if(event.isClusterClk(CTL_DIFFCNTR_LOCLIST)) {
		const  PPID loc_id = getCurrID();
		updateList(loc_id);
		if(SmartListBox::IsValidS(P_Box))
			P_Box->P_Def->top();
	}
	else
		return;
	clearEvent(event);
}

void DiffByLocCntrDlg::setupCounter(PPID locID)
{
	if(LocID != locID) {
		// запоминаем значение счетчика предыдущего склада
		long   val = getCtrlLong(CTL_DIFFCNTR_VAL);
		val = (val < 0) ? 0 : val;
		if(!Data.Search(LocID, 0))
			Data.AddUnique(LocID, val, 0);
		else
			Data.Update(LocID, val);
		// выставляем значение счетчика нового склада
		if(locID) {
			Data.Search(LocID = locID, &(val = 0), 0);
			setCtrlData(CTL_DIFFCNTR_VAL, &val);
		}
	}
}

int DiffByLocCntrDlg::setupList()
{
	if(P_Box) {
		P_Box->setDef(LObj.Selector(0, 0, 0));
		P_Box->Draw_();
	}
	return 1;
}

class OpCntrDialog : public TDialog {
	DECL_DIALOG_DATA(PPOpCounterPacket);
public:
	explicit OpCntrDialog(uint resID) : TDialog(resID)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		PPOpCounter opc_rec = Data.Head;
		setCtrlData(CTL_OPKCOUNTER_TEMPL, opc_rec.CodeTemplate);
		setCtrlData(CTL_OPKCOUNTER_COUNTER, &opc_rec.Counter);
		AddClusterAssoc(CTL_OPKCOUNTER_FLAGS, 0, OPCNTF_LOCKINCR);
		AddClusterAssoc(CTL_OPKCOUNTER_FLAGS, 1, OPCNTF_DIFFBYLOC);
		AddClusterAssoc(CTL_OPKCOUNTER_FLAGS, 2, OPCNTF_VERIFYUNIQ); // @v11.9.2
		SetClusterData(CTL_OPKCOUNTER_FLAGS, opc_rec.Flags);
		enableCommand(cmDiffByLoc, BIN(opc_rec.Flags & OPCNTF_DIFFBYLOC));
		if(resourceID == DLG_OPKCOUNTER)
			SetupPPObjCombo(this, CTLSEL_OPKCOUNTER_CNTR, PPOBJ_OPCOUNTER, opc_rec.OwnerObjID ? 0 : opc_rec.ID, OLW_CANINSERT, 0);
		else {
			setCtrlData(CTL_OPKCOUNTER_NAME, opc_rec.Name);
			setCtrlData(CTL_OPKCOUNTER_SYMB, opc_rec.Symb);
			setCtrlData(CTL_OPKCOUNTER_ID,   &opc_rec.ID);
			disableCtrl(CTL_OPKCOUNTER_ID, (!PPMaster || opc_rec.ID));
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		char   temp_buf[64];
		getCtrlData(sel = CTL_OPKCOUNTER_TEMPL, temp_buf);
		THROW_PP(OpcObj.CheckCodeTemplate(temp_buf, sizeof(((BillTbl::Rec*)0)->Code)), PPERR_INVCODETEMPLATE)
		STRNSCPY(Data.Head.CodeTemplate, temp_buf);
		getCtrlData(CTL_OPKCOUNTER_COUNTER, &Data.Head.Counter);
		if(resourceID == DLG_OPCNTR) {
			getCtrlData(sel = CTL_OPKCOUNTER_NAME, Data.Head.Name);
			THROW_PP(*strip(Data.Head.Name), PPERR_NAMENEEDED);
			getCtrlData(CTL_OPKCOUNTER_SYMB, Data.Head.Symb);
		}
		GetClusterData(CTL_OPKCOUNTER_FLAGS, &Data.Head.Flags);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    editDiffCounters(LAssocArray * pData)
	{
		DIALOG_PROC_BODY(DiffByLocCntrDlg, pData);
	}
	PPObjOpCounter OpcObj;
};

IMPL_HANDLE_EVENT(OpCntrDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_OPKCOUNTER_CNTR)) {
		PPID   id = getCtrlLong(CTLSEL_OPKCOUNTER_CNTR);
		if(OpcObj.GetPacket(id, &Data) > 0) {
			PPOpCounter * p_opc_rec = &Data.Head;
			SetClusterData(CTL_OPKCOUNTER_FLAGS, p_opc_rec->Flags);
			setCtrlData(CTL_OPKCOUNTER_TEMPL,   p_opc_rec->CodeTemplate);
			setCtrlData(CTL_OPKCOUNTER_COUNTER, &p_opc_rec->Counter);
			enableCommand(cmDiffByLoc, p_opc_rec->Flags & OPCNTF_DIFFBYLOC);
		}
	}
	else if(event.isClusterClk(CTL_OPKCOUNTER_FLAGS)) {
		long   old_flags = Data.Head.Flags;
		ushort v = getCtrlUInt16(CTL_OPKCOUNTER_FLAGS);
		SETFLAG(Data.Head.Flags, OPCNTF_DIFFBYLOC, v & 0x02);
		if((old_flags & OPCNTF_DIFFBYLOC) != (Data.Head.Flags & OPCNTF_DIFFBYLOC))
			Data.Init(&Data.Head, 0);
		enableCommand(cmDiffByLoc, v & 0x02); // OPCNTF_DIFFBYLOC
	}
	else if(event.isCmd(cmDiffByLoc)) {
		if(!editDiffCounters(Data.P_Items))
			PPError();
	}
	else
		return;
	clearEvent(event);
}

int EditCounter(PPOpCounterPacket * pPack, uint resID, PPID * pOpcID)
{
	int    ok = -1;
	OpCntrDialog * dlg = new OpCntrDialog(resID);
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(pPack);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(pPack)) {
			PPID   opc_id = (dlg->resourceID == DLG_OPKCOUNTER) ? dlg->getCtrlLong(CTLSEL_OPKCOUNTER_CNTR) : 0;
			ASSIGN_PTR(pOpcID, opc_id);
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

void OprKindDialog::editCounter()
{
	if(!EditCounter(&P_Data->OpCntrPack, DLG_OPKCOUNTER, &P_Data->Rec.OpCounterID))
		PPError();
}
//
//
//
void OprKindDialog::editPoolOptions()
{
	class OpkPoolDialog : public OpListDialog {
	public:
		explicit OpkPoolDialog(PPIDArray * pOpTypesList) : OpListDialog(DLG_OPRPOOL, CTL_OPRPOOL_LIST, 0, pOpTypesList), P_Data(0)
		{
		}
		int    setDTS(PPOprKindPacket * pData)
		{
			P_Data = pData;
			PPBillPoolOpEx * p_bpox = P_Data->P_PoolData;
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 0, BPOXF_ONEOP);
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 1, BPOXF_ONEDATE);
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 2, BPOXF_ONEOBJECT);
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 3, BPOXF_UNITEACCTURNS);
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 4, BPOXF_UNITEPAYMENTS);
			AddClusterAssoc(CTL_OPRPOOL_FLAGS, 5, BPOXF_AUTOAMOUNT); // @v11.8.0
			SetClusterData(CTL_OPRPOOL_FLAGS, p_bpox->Flags);
			AddClusterAssoc(CTL_OPRPOOL_COMMF, 0, OPKF_NEEDPAYMENT);
			AddClusterAssoc(CTL_OPRPOOL_COMMF, 1, OPKF_FREIGHT);
			SetClusterData(CTL_OPRPOOL_COMMF, P_Data->Rec.Flags);
			setOpList(&p_bpox->OpList);
			return 1;
		}
		int    getDTS(PPOprKindPacket * /*pData*/)
		{
			PPBillPoolOpEx * p_bpox = P_Data->P_PoolData;
			getOpList(&p_bpox->OpList);
			GetClusterData(CTL_OPRPOOL_FLAGS, &p_bpox->Flags);
			GetClusterData(CTL_OPRPOOL_COMMF, &P_Data->Rec.Flags);
			return 1;
		}
	private:
		PPOprKindPacket * P_Data;
	};
	OpkPoolDialog * dlg = 0;
	if(P_Data->Rec.OpTypeID == PPOPT_POOL) {
		PPIDArray op_type_list;
		THROW_MEM(SETIFZ(P_Data->P_PoolData, new PPBillPoolOpEx));
		op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
			PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSORDER, PPOPT_GOODSMODIF, PPOPT_GOODSACK,
			PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTQUOTREQ, 0L); // @v10.5.7 PPOPT_DRAFTQUOTREQ
		THROW(CheckDialogPtr(&(dlg = new OpkPoolDialog(&op_type_list))));
		dlg->setDTS(P_Data);
		if(ExecView(dlg) == cmOK)
			dlg->getDTS(P_Data);
	}
	CATCH
		PPError();
	ENDCATCH
	delete dlg;
}
//
//
//
int PPObjOprKind::Browse(void * extraPtr) { return PPView::Execute(PPVIEW_OPRKIND, 0, 1, 0); }
int PPObjOprKind::Edit(PPID * pID, void * extraPtr /*opTypeID*/) { return Edit(pID, reinterpret_cast<PPID>(extraPtr), 0); }

// non-virtual
int PPObjOprKind::Edit(PPID * pID, long opTypeID, PPID linkOpID)
{
	int    ok = 1;
	int    done = 0;
	bool   is_new = false;
	int    r = cmCancel;
	PPOprKindPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else {
		if(opTypeID > 0 || PPSelectObject(PPOBJ_OPRTYPE, &(opTypeID = 0), PPTXT_SELECTOPRTYPE, 0) > 0) {
			pack.Rec.OpTypeID = opTypeID;
			pack.Rec.LinkOpID = linkOpID;
		}
		else
			done = 1;
	}
	if(!done) {
		for(bool valid_data = false; !valid_data && EditPacket(&pack) > 0;) {
			THROW(PutPacket(pID, &pack, 1));
			valid_data = true;
			r = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int PPObjOprKind::EditPacket(PPOprKindPacket * pPack)
{
	int    ok = -1;
	uint   dlg_id = DLG_OPRKIND;
	OprKindDialog * dlg = 0;
	switch(pPack->Rec.OpTypeID) {
		case PPOPT_WAREHOUSE: dlg_id = DLG_OPRWMS; break;
		case PPOPT_INVENTORY: dlg_id = DLG_OPRINV; break;
		case PPOPT_GENERIC:
			dlg_id = DLG_GENOPRKIND;
			SETIFZ(pPack->P_GenList, new ObjRestrictArray);
			break;
		case PPOPT_DRAFTEXPEND:
		case PPOPT_DRAFTRECEIPT:
		case PPOPT_DRAFTTRANSIT:
		case PPOPT_DRAFTQUOTREQ: dlg_id = DLG_OPRDRAFT; break;
		case PPOPT_ACCTURN:
			dlg_id = DLG_OPRKIND;
			if(pPack->Rec.ID == 0)
				pPack->Rec.Flags |= OPKF_EXTACCTURN;
			break;
		default: dlg_id = DLG_OPRKIND; break;
	}
	dlg = new OprKindDialog(dlg_id, pPack);
	if(CheckDialogPtr(&dlg)) {
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			if(dlg->getDTS(pPack)) {
				if(!CheckName(pPack->Rec.ID, pPack->Rec.Name, 0))
					dlg->selectCtrl(CTL_OPRKIND_NAME);
				else if(!CheckDupSymb(pPack->Rec.ID, pPack->Rec.Symb))
					PPErrorByDialog(dlg, CTL_OPRKIND_SYMB);
				else
					ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPObjOprKind::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = cmCancel;
	int    valid_data = 0;
	uint   i = 0;
	SString temp_buf;
	PPOprKindPacket pack;
	THROW(CheckRights(PPR_INS));
	THROW(GetPacket(sampleID, &pack) > 0);
	pack.Rec.ID = 0;
	//
	// Подстановка уникального имени
	//
	for(i = 1; i < 999; i++) {
		(temp_buf = pack.Rec.Name).Space().CatChar('#').CatLongZ(i, 3);
		if(CheckDupName(0, temp_buf)) {
			temp_buf.CopyTo(pack.Rec.Name, sizeof(pack.Rec.Name));
			break;
		}
	}
	if(pack.Rec.OpCounterID) {
		PPOpCounter cntr_rec;
		if(SearchObject(PPOBJ_OPCOUNTER, pack.Rec.OpCounterID, &cntr_rec) > 0)
			if(cntr_rec.OwnerObjID)
				pack.Rec.OpCounterID = 0;
	}
	for(valid_data = 0; !valid_data && EditPacket(&pack) > 0;) {
		THROW(PutPacket(pID, &pack, 1));
		valid_data = 1;
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjOprKind::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		if(_id && oneof4(_obj, PPOBJ_OPRTYPE, PPOBJ_ACCSHEET, PPOBJ_ACCOUNT2, PPOBJ_ARTICLE)) {
			PPOprKind op_rec;
			for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&op_rec) > 0;) {
				if(_obj == PPOBJ_ACCSHEET && op_rec.AccSheetID == _id)
					return RetRefsExistsErr(Obj, op_rec.ID);
				else if(_obj == PPOBJ_OPRTYPE && op_rec.OpTypeID == _id)
					return RetRefsExistsErr(Obj, op_rec.ID);
				else if(oneof2(_obj, PPOBJ_ACCOUNT2, PPOBJ_ARTICLE)) {
					PPAccTurnTempl att;
					for(PPID prop = 0; P_Ref->EnumProperties(Obj, op_rec.ID, &prop, &att, sizeof(att)) > 0 && prop <= PP_MAXATURNTEMPLATES;)
						if((_obj == PPOBJ_ACCOUNT2 && (att.DbtID.ac == _id || att.CrdID.ac == _id)) || (_obj == PPOBJ_ARTICLE && (att.DbtID.ar == _id || att.CrdID.ar == _id)))
							return RetRefsExistsErr(Obj, op_rec.ID);
				}
			}
		}
	}
	return DBRPL_OK;
}

int PPObjOprKind::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTOPRK, CTL_RTOPRK_FLAGS, CTL_RTOPRK_SFLAGS, bufSize, rt, pDlg);
}

IMPL_DESTROY_OBJ_PACK(PPObjOprKind, PPOprKindPacket);

int PPObjOprKind::SerializePacket(int dir, PPOprKindPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	PPReckonOpEx roe;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, pPack->ExtString, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &pPack->Amounts, rBuf));
	if(dir > 0) {
		if(pPack->P_ReckonData) {
			THROW(pPack->P_ReckonData->Serialize(dir, rBuf, pSCtx));
		}
		else {
			THROW(roe.Serialize(dir, rBuf, pSCtx));
		}
	}
	else if(dir < 0) {
		THROW(roe.Serialize(dir, rBuf, pSCtx));
		if(!roe.IsEmpty()) {
			THROW_MEM(pPack->P_ReckonData = new PPReckonOpEx);
			*pPack->P_ReckonData = roe;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjOprKind::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjOprKind, PPOprKindPacket>(this, p, id, stream, pCtx); }

int PPObjOprKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPOprKindPacket * p_pack = static_cast<PPOprKindPacket *>(p->Data);
		if(stream == 0) {
			PPID   same_id = 0;
			PPOprKind same_rec;
			if(*pID == 0) {
				if(p_pack->Rec.ID < PP_FIRSTUSRREF) {
					*pID = p_pack->Rec.ID;
				}
				else if(pCtx->Cfg.Flags & DBDXF_IMPOPSYNC && SearchByName(p_pack->Rec.Name, &same_id, &same_rec) > 0 &&
					same_rec.OpTypeID == p_pack->Rec.OpTypeID && same_rec.AccSheetID == p_pack->Rec.AccSheetID) {
					*pID = same_id;
				}
				else {
					p_pack->Rec.ID = 0;
					THROW(PutPacket(pID, p_pack, 1));
				}
			}
			/*
			else {
				//THROW(ref->UpdateItem(Obj, *pID, &p_pack->Rec));
				THROW(PutPacket(pID, p_pack, 1));
			}
			*/
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjOprKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		uint   i;
		PPID * p_id = 0;
		PPOprKindPacket * p_pack = static_cast<PPOprKindPacket *>(p->Data);
		ProcessObjRefInArray(PPOBJ_OPRKIND,   &p_pack->Rec.LinkOpID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_OPRTYPE,   &p_pack->Rec.OpTypeID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_ACCSHEET,  &p_pack->Rec.AccSheetID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_ACCSHEET,  &p_pack->Rec.AccSheet2ID, ary, replace);
		ProcessObjRefInArray(PPOBJ_OPCOUNTER, &p_pack->Rec.OpCounterID, ary, replace);
		ProcessObjRefInArray(PPOBJ_LOCATION,  &p_pack->Rec.DefLocID,    ary, replace);
		for(i = 0; p_pack->Amounts.enumItems(&i, reinterpret_cast<void **>(&p_id));)
			ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, p_id, ary, replace);
		if(p_pack->P_ReckonData) {
			ProcessObjRefInArray(PPOBJ_PERSONRELTYPE, &p_pack->P_ReckonData->PersonRelTypeID, ary, replace);
			for(i = 0; p_pack->P_ReckonData->OpList.enumItems(&i, reinterpret_cast<void **>(&p_id));)
				ProcessObjRefInArray(PPOBJ_OPRKIND, p_id, ary, replace);
		}
		return 1;
	}
	return -1;
}
//
//
//
class ReckonOpExCache : public ObjCache { // @v11.7.11
public:
	struct Data : public ObjCacheEntry {
		LDATE  Beg;
		LDATE  End;
		long   Flags;
		PPID   PersonRelTypeID;
		uint   OpCount;
		PPID   OpList[128]; // PPReckonOpEx содержит список зачитывающих операций. Так как здесь нам нужна "плоская" структура, то
			// предположим, что количество зачитывающих операций не может превышать 128.
			// Переменная OpCount содержит фактическое количество операций в массиве.
			// И да, я знаю, что сделал криво, однако, работать это будет.
	};
	ReckonOpExCache() : ObjCache(PPOBJ_OPRKIND, sizeof(Data))
	{
	}
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

/*virtual*/int ReckonOpExCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPReckonOpEx rec;
	PPObjOprKind op_obj;
	if(op_obj.GetReckonExData(id, &rec) > 0) {
	   	p_cache_rec->Beg  = rec.Beg;
		p_cache_rec->End = rec.End;
	   	p_cache_rec->Flags = rec.Flags;
		p_cache_rec->PersonRelTypeID = rec.PersonRelTypeID;
		p_cache_rec->OpCount = 0;
		memzero(p_cache_rec->OpList, sizeof(p_cache_rec->OpList));
		for(uint i = 0; i < rec.OpList.getCount() && i < SIZEOFARRAY(p_cache_rec->OpList); i++) {
			p_cache_rec->OpList[i] = rec.OpList.get(i);
			p_cache_rec->OpCount++;
		}
		if(rec.OpList.getCount() > SIZEOFARRAY(p_cache_rec->OpList)) {
			// @todo @log (надо как-то сигнализировать, что не удалось загрузить все зачитывающие операции в массив)
		}
	}
	else
		ok = -1;
	return ok;
}

/*virtual*/void ReckonOpExCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPReckonOpEx * p_data_rec = static_cast<PPReckonOpEx *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	p_data_rec->Z();
	p_data_rec->Beg = p_cache_rec->Beg;
	p_data_rec->End = p_cache_rec->End;
	p_data_rec->Flags = p_cache_rec->Flags;
	p_data_rec->PersonRelTypeID = p_cache_rec->PersonRelTypeID;
	for(uint i = 0; i < p_cache_rec->OpCount; i++) {
		p_data_rec->OpList.add(p_cache_rec->OpList[i]);
	}
}

class InvOpExCache : public ObjCache {
public:
	struct Data : public ObjCacheEntry {
		PPID   WrDnOp;           // Операция списания недостач
		PPID   WrDnObj;          // Контрагент списания недостач
		PPID   WrUpOp;           // Операция списания излишков
		PPID   WrUpObj;          // Контрагент списания излишков
		int16  AmountCalcMethod; // Метод расчета цен
		int16  AutoFillMethod;   // Метод автозаполнени
		long   Flags;            // INVOPF_XXX
		long   OnWrOffStatusID;  // @v10.5.9 Статус, устанавливаемый при списании документа
	};
	InvOpExCache() : ObjCache(PPOBJ_OPRKIND, sizeof(Data))
	{
	}
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int InvOpExCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPInventoryOpEx rec;
	if(PPRef->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_INVENTORY, &rec, sizeof(rec)) > 0) {
	   	p_cache_rec->WrDnOp  = rec.WrDnOp;
		p_cache_rec->WrDnObj = rec.WrDnObj;
	   	p_cache_rec->WrUpOp  = rec.WrUpOp;
		p_cache_rec->WrUpObj = rec.WrUpObj;
		p_cache_rec->AmountCalcMethod = rec.AmountCalcMethod;
		p_cache_rec->AutoFillMethod = rec.AutoFillMethod;
		p_cache_rec->Flags   = rec.Flags;
		p_cache_rec->OnWrOffStatusID = rec.OnWrOffStatusID; // @v10.5.9
	}
	else
		ok = -1;
	return ok;
}

void InvOpExCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPInventoryOpEx * p_data_rec = static_cast<PPInventoryOpEx *>(pDataRec);
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag = PPOBJ_OPRKIND;
	p_data_rec->ID  = p_cache_rec->ID;
	p_data_rec->Prop = OPKPRP_INVENTORY;
	p_data_rec->WrDnOp  = p_cache_rec->WrDnOp;
	p_data_rec->WrDnObj = p_cache_rec->WrDnObj;
	p_data_rec->WrUpOp  = p_cache_rec->WrUpOp;
	p_data_rec->WrUpObj = p_cache_rec->WrUpObj;
	p_data_rec->AmountCalcMethod = p_cache_rec->AmountCalcMethod;
	p_data_rec->AutoFillMethod = p_cache_rec->AutoFillMethod;
	p_data_rec->Flags   = p_cache_rec->Flags;
	p_data_rec->OnWrOffStatusID = p_cache_rec->OnWrOffStatusID; // @v10.5.9
}
//
//
//
class OpCache : public ObjCache {
public:
	struct OpData : public ObjCacheEntry {
		PPID   InitStatusID;
		long   ExtFlags;
		int16  Rank;
		int16  Reserve2;
		PPID   LinkOpID;
		PPID   AccSheet2ID;
		PPID   OpCounterID;
		long   PrnFlags;
		PPID   DefLocID;
		int16  PrnOrder;
		int16  SubType;
		long   Flags;
		PPID   OpTypeID;
		PPID   AccSheetID;
	};
	OpCache() : ObjCache(PPOBJ_OPRKIND, sizeof(OpCache::OpData)), P_ReckonOpList(0), State(0)
	{
	}
	~OpCache()
	{
		delete P_ReckonOpList;
	}
	virtual void FASTCALL Dirty(PPID); // @sync_w
	int    GetReckonOpList(PPIDArray *); // @sync_rw
	int    GetInventoryOpEx(PPID, PPInventoryOpEx *); // @>>IoeC.Get()
	int    GetReckonExData(PPID, PPReckonOpEx *); // @v11.7.11
	PPID   FASTCALL GetBySymb(const char * pSymb);
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * entry, void * pDataRec) const;
	int    FetchReckonOpList();

	enum {
		stReckonListInited = 0x0001,
		stOpSymbListInited = 0x0002
	};
	long   State;
	InvOpExCache IoeC;
	ReckonOpExCache RoxC;
	PPIDArray * P_ReckonOpList;
	StrAssocArray OpSymbList;
};

void FASTCALL OpCache::Dirty(PPID opID)
{
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		Helper_Dirty(opID);
		if(P_ReckonOpList && P_ReckonOpList->lsearch(opID)) {
			State &= ~stReckonListInited;
			ZDELETE(P_ReckonOpList);
		}
		if(State & stOpSymbListInited) {
			OpSymbList.Z();
			State &= ~stOpSymbListInited;
		}
		IoeC.Dirty(opID);
	}
}

PPID FASTCALL OpCache::GetBySymb(const char * pSymb)
{
	PPID   op_id = 0;
	{
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(!(State & stOpSymbListInited)) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(State & stOpSymbListInited)) {
				OpSymbList.Z();
				PPOprKind op_rec;
				for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, Reference::eoIdSymb); en.Next(&op_rec) > 0;) {
					if(op_rec.Symb[0]) {
						OpSymbList.Add(op_rec.ID, strip(op_rec.Symb));
					}
				}
				State |= stOpSymbListInited;
			}
			SRWLOCKER_TOGGLE(SReadWriteLocker::Read);
		}
		if(State & stOpSymbListInited) {
			uint pos = 0;
			if(OpSymbList.SearchByTextNc(pSymb, &pos)) {
				StrAssocArray::Item item = OpSymbList.at_WithoutParent(pos);
				op_id = item.Id;
			}
		}
	}
	return op_id;
}

int OpCache::GetInventoryOpEx(PPID opID, PPInventoryOpEx * pInvOpEx) { return IoeC.Get(opID, pInvOpEx); }
int OpCache::GetReckonExData(PPID opID, PPReckonOpEx * pData) { return RoxC.Get(opID, pData); } // @v11.7.11

int OpCache::GetReckonOpList(PPIDArray * pList)
{
	int    ok = -1;
	if(pList) {
		pList->clear();
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(State & stReckonListInited) {
			if(P_ReckonOpList)
				pList->copy(*P_ReckonOpList);
			ok = 1;
		}
		if(ok < 0) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			ok = FetchReckonOpList();
			if(State & stReckonListInited) {
				if(P_ReckonOpList)
					pList->copy(*P_ReckonOpList);
				ok = 1;
			}
		}
	}
	return ok;
}

int OpCache::FetchReckonOpList()
{
	int    ok = 1;
	if(!(State & stReckonListInited)) {
		Reference * p_ref = PPRef;
		PPIDArray temp_list;
		PropertyTbl::Key0 k;
		BExtQuery q(&p_ref->Prop, 0);
		q.select(p_ref->Prop.ObjType, p_ref->Prop.ObjID, p_ref->Prop.Prop, 0L).
			where(p_ref->Prop.ObjType == PPOBJ_OPRKIND && p_ref->Prop.Prop == static_cast<long>(OPKPRP_PAYMOPLIST));
		MEMSZERO(k);
		k.ObjType = PPOBJ_OPRKIND;
		for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;)
			THROW(temp_list.addUnique(p_ref->Prop.data.ObjID));
		if(temp_list.getCount()) {
			if(P_ReckonOpList == 0)
				THROW_MEM(P_ReckonOpList = new PPIDArray);
			P_ReckonOpList->copy(temp_list);
		}
		else
			ZDELETE(P_ReckonOpList);
		State |= stReckonListInited;
	}
	CATCH
		ok = 0;
		State &= ~stReckonListInited;
		ZDELETE(P_ReckonOpList);
	ENDCATCH
	return ok;
}

int OpCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	OpData * p_cache_rec = static_cast<OpData *>(pEntry);
	PPOprKind op_rec;
	if((ok = PPRef->GetItem(PPOBJ_OPRKIND, id, &op_rec)) > 0) {
		#define FLD(f) p_cache_rec->f = op_rec.f
		FLD(InitStatusID);
		FLD(ExtFlags);
		FLD(Rank);
		FLD(Reserve2);
		FLD(LinkOpID);
		FLD(AccSheet2ID);
		FLD(OpCounterID);
		FLD(PrnFlags);
		FLD(DefLocID);
		FLD(PrnOrder);
		FLD(SubType);
		FLD(Flags);
		FLD(OpTypeID);
		FLD(AccSheetID);
		#undef FLD
		MultTextBlock b;
		b.Add(op_rec.Name);
		b.Add(op_rec.Symb);
		ok = PutTextBlock(b, pEntry);
	}
	return ok;
}

void OpCache::EntryToData(const ObjCacheEntry * pEntry, void * dataRec) const
{
	PPOprKind * p_data_rec = static_cast<PPOprKind *>(dataRec);
	const OpData * p_cache_rec  = static_cast<const OpData *>(pEntry);
	memzero(p_data_rec, sizeof(PPOprKind));
	p_data_rec->Tag = PPOBJ_OPRKIND;
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(ID);
	FLD(InitStatusID);
	FLD(ExtFlags);
	FLD(Rank);
	FLD(Reserve2);
	FLD(LinkOpID);
	FLD(AccSheet2ID);
	FLD(OpCounterID);
	FLD(PrnFlags);
	FLD(DefLocID);
	FLD(PrnOrder);
	FLD(SubType);
	FLD(Flags);
	FLD(OpTypeID);
	FLD(AccSheetID);
	#undef FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

int FetchInvOpEx(PPID opID, PPInventoryOpEx * pInvOpEx)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	return p_cache ? p_cache->GetInventoryOpEx(opID, pInvOpEx) : 0;
}

int FetchReckonExData(PPID opID, PPReckonOpEx * pData) // @v11.7.11
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	return p_cache ? p_cache->GetReckonExData(opID, pData) : 0;
}

int PPObjOprKind::FetchInventoryData(PPID id, PPInventoryOpEx * pInvOpEx) { return ::FetchInvOpEx(id, pInvOpEx); }
int PPObjOprKind::FetchReckonExData(PPID opID, PPReckonOpEx * pData) { return ::FetchReckonExData(opID, pData); } // @v11.7.11

int FASTCALL GetReckonOpList(PPIDArray * pList)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND); 
	return p_cache ? p_cache->GetReckonOpList(pList) : 0;
}

int FASTCALL GetOpData(PPID op, PPOprKind * pData)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	PPOprKind stub_data; // @v11.4.9
	SETIFZQ(pData, &stub_data); // @v11.4.9
	int    r = p_cache ? p_cache->Get(op, pData) : 0;
	if(r < 0)
		r = 0;
	return r;
}

/*static*/int FASTCALL PPObjOprKind::ExpandOpList(const PPIDArray & rBaseOpList, PPIDArray & rResultList)
{
	int    ok = -1;
	rResultList.clear();
	PPObjOprKind op_obj;
	ObjRestrictArray or_list;
	for(uint i = 0; i < rBaseOpList.getCount(); i++) {
		const  PPID base_op_id = rBaseOpList.get(i);
		if(base_op_id) {
			if(IsGenericOp(base_op_id) > 0) {
				or_list.clear();
				ObjRestrictItem * p_or_item;
				op_obj.GetGenericList(base_op_id, &or_list);
				for(uint oppos = 0; or_list.enumItems(&oppos, reinterpret_cast<void **>(&p_or_item));)
					rResultList.add(p_or_item->ObjID);
			}
			else
				rResultList.add(base_op_id);
		}
	}
	rResultList.sortAndUndup();
	if(rResultList.getCount())
		ok = 1;
	return ok;
}

/*static*/int FASTCALL PPObjOprKind::ExpandOp(PPID opID, PPIDArray & rResultList)
{
	int    ok = -1;
	rResultList.clear();
	PPObjOprKind op_obj;
	ObjRestrictArray or_list;
	if(IsGenericOp(opID) > 0) {
		or_list.clear();
		ObjRestrictItem * p_or_item;
		op_obj.GetGenericList(opID, &or_list);
		for(uint oppos = 0; or_list.enumItems(&oppos, reinterpret_cast<void **>(&p_or_item));)
			rResultList.add(p_or_item->ObjID);
	}
	else
		rResultList.add(opID);
	rResultList.sortAndUndup();
	if(rResultList.getCount())
		ok = 1;
	return ok;
}

int FASTCALL GetOpBySymb(const char * pSymb, PPOprKind * pData)
{
	PPID   op_id = 0;
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	if(p_cache) {
		op_id = p_cache->GetBySymb(pSymb);
		if(op_id && pData) {
			p_cache->Get(op_id, pData);
		}
	}
	return op_id ? 1 : -1;
}

int FASTCALL GetOpName(PPID opID, SString & rBuf)
{
	rBuf.Z();
	PPOprKind op_rec;
	return (opID && GetOpData(opID, &op_rec)) ? ((rBuf = op_rec.Name), 1) : 0;
}

int STDCALL GetOpName(PPID op, char * buf, size_t buflen)
{
	PPOprKind op_rec;
	if(op && GetOpData(op, &op_rec)) {
		strnzcpy(buf, op_rec.Name, buflen);
		return 1;
	}
	else {
		ASSIGN_PTR(buf, 0);
		return 0;
	}
}

int STDCALL CheckOpFlags(PPID op, long andF, long notF)
{
	PPOprKind op_rec;
	if(op && GetOpData(op, &op_rec))
		if(andF && (op_rec.Flags & andF) != andF)
			return 0;
		else if(notF && (op_rec.Flags & notF) == notF)
			return 0;
		else
			return 1;
	else
		return 0;
}

int FASTCALL CheckOpPrnFlags(PPID op, long andF)
{
	PPOprKind op_rec;
	if(op && GetOpData(op, &op_rec))
		return (andF && (op_rec.PrnFlags & andF) != andF) ? 0 : 1;
	else
		return 0;
}

int STDCALL EnumOperations(PPID opTypeID, PPID * pID, PPOprKind * pOpData)
{
	int    r;
	Reference * p_ref = PPRef;
	PPOprKind op_rec;
	while((r = p_ref->EnumItems(PPOBJ_OPRKIND, pID, &op_rec)) > 0) {
		if(!opTypeID || op_rec.OpTypeID == opTypeID) {
			if(pOpData)
				memcpy(pOpData, &op_rec, sizeof(PPOprKind));
			return 1;
		}
	}
	return r ? -1 : 0;
}

PPID FASTCALL GetOpType(PPID opID, PPOprKind * pOpData)
{
	PPID   op_type_id = 0;
	PPOprKind op_rec;
	if(opID && GetOpData(opID, &op_rec))
		op_type_id = op_rec.OpTypeID;
	else
		MEMSZERO(op_rec);
	ASSIGN_PTR(pOpData, op_rec);
	return op_type_id;
}

int FASTCALL GetOpSubType(PPID opID)
{
	PPOprKind op_rec;
	return GetOpData(opID, &op_rec) ? op_rec.SubType : 0;
}

PPID FASTCALL IsOpPaymOrRetn(PPID opID)
{
	if(opID) {
		PPOprKind op_rec;
		const  PPID op_type_id = GetOpType(opID, &op_rec);
		return (oneof2(op_type_id, PPOPT_PAYMENT, PPOPT_GOODSRETURN) || (op_type_id == PPOPT_CHARGE && op_rec.Flags & OPKF_CHARGENEGPAYM)) ? op_type_id : 0;
	}
	else
		return 0;
}

bool FASTCALL IsOpPaym(PPID opID)
{
	if(opID) {
		PPOprKind op_rec;
		const  PPID op_type_id = GetOpType(opID, &op_rec);
		return ((op_type_id == PPOPT_PAYMENT) || (op_type_id == PPOPT_CHARGE && op_rec.Flags & OPKF_CHARGENEGPAYM));
	}
	else
		return false;
}

int FASTCALL _IsSellingOp(PPID opID)
{
	PPOprKind opk;
	if(opID == 0)
		return -1;
	PPID   t = GetOpType(opID, &opk);
	if(IsIntrOp(opID) || opID == _PPOPK_SUPPLRET || t == PPOPT_GOODSRECEIPT)
		return 0;
	else if(oneof4(opID, PPOPK_SELL, PPOPK_RETAIL, _PPOPK_SELLRET, _PPOPK_RETAILRET))
		return 1;
	else if(oneof4(t, PPOPT_GOODSRETURN, PPOPT_PAYMENT, PPOPT_GOODSACK, PPOPT_CHARGE))
		return IsSellingOp(opk.LinkOpID);
	else if(oneof2(t, PPOPT_GOODSREVAL, PPOPT_GOODSORDER))
		return 1;
	else
		return -1;
}

int FASTCALL IsIntrOp(PPID opID)
{
	int    r = 0;
	if(opID) {
		PPOprKind opk;
		if(opID == PPOPK_INTREXPEND)
			r = INTREXPND;
		else if(opID == PPOPK_INTRRECEIPT)
			r = INTRRCPT;
		else if(GetOpData(opID, &opk)) {
			if(opk.AccSheetID == LConfig.LocAccSheetID) {
				if(opk.OpTypeID == PPOPT_GOODSEXPEND)
					r = INTREXPND;
				else if(opk.OpTypeID == PPOPT_GOODSRECEIPT)
					r = INTRRCPT;
			}
		}
	}
	return r;
}

bool FASTCALL IsIntrExpndOp(PPID opID) { return (IsIntrOp(opID) == INTREXPND); }
int  FASTCALL IsGenericOp(PPID opID) { return opID ? BIN(GetOpType(opID) == PPOPT_GENERIC) : -1; }

bool FASTCALL IsDraftOp(PPID opID)
{
	const  PPID op_type_id = GetOpType(opID);
	return oneof4(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ); // @v10.5.7 PPOPT_DRAFTQUOTREQ
}

bool FASTCALL IsGoodsDetailOp(PPID opID)
{
	const  PPID op_type_id = GetOpType(opID);
	return oneof12(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
		PPOPT_GOODSREVAL, PPOPT_CORRECTION, PPOPT_GOODSACK, PPOPT_GOODSMODIF, PPOPT_GOODSRETURN, PPOPT_GOODSORDER, PPOPT_DRAFTQUOTREQ); // @v10.5.7 PPOPT_DRAFTQUOTREQ
}

int FASTCALL IsSellingOp(PPID opID)
{
	int    ret = -1;
	if(opID) {
		PPOprKind op_rec;
		GetOpData(opID, &op_rec);
		const long s = (op_rec.Flags & OPKF_SELLING);
		ret = (s ^ (op_rec.Flags & OPKF_BUYING)) ? BIN(s) : _IsSellingOp(opID);
	}
	return ret;
}

int FASTCALL IsExpendOp(PPID opID)
{
	int    ret = -1;
	if(opID) {
		PPOprKind op_rec;
		const  PPID op_type_id = GetOpType(opID, &op_rec);
		if(op_type_id == PPOPT_GOODSEXPEND)
			ret = 1;
		else if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSORDER))
			ret = 0;
		else if(op_type_id == PPOPT_GOODSRETURN) {
			const int r = IsExpendOp(op_rec.LinkOpID);
			ret = (r >= 0) ? (r ? 0 : 1) : -1;
		}
		else if(op_type_id == PPOPT_GOODSACK)
			ret = IsExpendOp(op_rec.LinkOpID);
	}
	return ret;
}

int FASTCALL GetGenericOpList(PPID opID, ObjRestrictArray * pList)
{
	int    ok = -1;
	if(opID && GetOpType(opID) == PPOPT_GENERIC) {
		PPObjOprKind op_obj;
		ok = op_obj.GetGenericList(opID, pList);
	}
	return ok;
}

int FASTCALL GetGenericOpList(PPID opID, PPIDArray * pList)
{
	int    ok = -1;
	if(opID && GetOpType(opID) == PPOPT_GENERIC) {
		PPObjOprKind op_obj;
		ok = op_obj.GetGenericList(opID, pList);
	}
	return ok;
}

bool FASTCALL IsOpBelongTo(PPID testOpID, PPID anotherOpID)
{
	bool   ok = false;
	if(anotherOpID > 0) {
		if(testOpID == anotherOpID)
			ok = true;
		else {
			PPIDArray op_list;
			if(GetGenericOpList(anotherOpID, &op_list) > 0 && op_list.lsearch(testOpID))
				ok = true;
		}
	}
	return ok;
}

int STDCALL GetOpCommonAccSheet(PPID opID, PPID * pAccSheetID, PPID * pAccSheet2ID)
{
	int    ok = -1;
	PPID   acc_sheet_id = 0;
	PPID   acc_sheet2_id = 0;
	PPOprKind op_rec;
	if(GetOpData(opID, &op_rec)) {
		if(op_rec.AccSheet2ID)
			acc_sheet2_id = op_rec.AccSheet2ID;
		if(op_rec.AccSheetID || GetOpData(op_rec.LinkOpID, &op_rec) > 0)
			acc_sheet_id = op_rec.AccSheetID;
		else if(op_rec.OpTypeID == PPOPT_GENERIC) {
		   	PPIDArray op_list;
			GetGenericOpList(opID, &op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				PPID   tmp_acc_sheet_id = 0;
				PPID   tmp_acc_sheet2_id = 0;
				GetOpCommonAccSheet(op_list.at(i), &tmp_acc_sheet_id, &tmp_acc_sheet2_id); // @recursion
				if(acc_sheet2_id >= 0) {
					if(acc_sheet2_id == 0)
						acc_sheet2_id = tmp_acc_sheet2_id;
					else if(tmp_acc_sheet2_id && tmp_acc_sheet2_id != acc_sheet2_id)
						acc_sheet2_id = -1;
				}
				if(acc_sheet_id >= 0) {
					if(acc_sheet_id == 0)
						acc_sheet_id = tmp_acc_sheet_id;
					else if(tmp_acc_sheet_id && tmp_acc_sheet_id != acc_sheet_id)
						acc_sheet_id = -1;
				}
			}
		}
	}
	if(acc_sheet_id > 0)
		ok = 1;
	ASSIGN_PTR(pAccSheetID,  (acc_sheet_id > 0  ? acc_sheet_id  : 0L));
	ASSIGN_PTR(pAccSheet2ID, (acc_sheet2_id > 0 ? acc_sheet2_id : 0L));
	return ok;
}

PPID GetCashOp()
{
	const  PPCommConfig & r_ccfg = CConfig;
	return NZOR(r_ccfg.RetailOp, PPOPK_RETAIL);
}

PPID GetCashRetOp()
{
	const  PPCommConfig & r_ccfg = CConfig;
	PPID   op_id_ = -1;
	if(r_ccfg.RetailRetOp)
		op_id_ = r_ccfg.RetailRetOp;
	else {
		const  PPID cash_op_id = GetCashOp();
		PPOprKind opk;
		for(PPID iter_op_id = 0; op_id_ < 0 && EnumOperations(PPOPT_GOODSRETURN, &iter_op_id, &opk) > 0;)
			if(opk.LinkOpID == cash_op_id)
				op_id_ = iter_op_id;
	}
	return op_id_;
}

PPID GetReceiptOp()
{
	return NZOR(CConfig.ReceiptOp, PPOPK_RECEIPT);
}

PPID FASTCALL GetSingleOp(PPID opTypeID)
{
	int    count = 0;
	PPID   ret_op_id = 0;
	PPOprKind op_rec;
	for(PPID op_id = 0; EnumOperations(opTypeID, &op_id, &op_rec) > 0;) {
		if(++count > 1) {
			ret_op_id = -1;
			break;
		}
		ret_op_id = op_id;
	}
	return ret_op_id;
}
//
// Implementation of PPALDD_OprKind
//
PPALDD_CONSTRUCTOR(OprKind)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(OprKind) { Destroy(); }

int PPALDD_OprKind::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPOprKind rec;
		PPObjOprKind op_obj;
		if(op_obj.Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			op_obj.GetExtStrData(rFilt.ID, OPKEXSTR_EXPSYMB, temp_buf);
			temp_buf.CopyTo(H.ExpSymb, sizeof(H.ExpSymb));
			H.ID = rec.ID;
			H.AccSheetID  = rec.AccSheetID;
			H.AccSheet2ID = rec.AccSheet2ID;
			H.LinkOpID    = rec.LinkOpID;
			H.OpCntrID    = rec.OpCounterID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			H.OpType = rec.OpTypeID;
			H.Flags = rec.Flags;
			H.fNeedPayment = BIN(rec.Flags & OPKF_NEEDPAYMENT);
			H.fProfitable  = BIN(rec.Flags & OPKF_PROFITABLE);
			H.fReckon      = BIN(rec.Flags & OPKF_RECKON);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
