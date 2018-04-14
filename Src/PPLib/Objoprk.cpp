// OBJOPRK.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PPInventoryOpEx::PPInventoryOpEx()
{
	THISZERO();
}

//static
int FASTCALL PPInventoryOpEx::Helper_GetAccelInputMode(long flags)
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

int SLAPI PPInventoryOpEx::GetAccelInputMode() const
{
	return Helper_GetAccelInputMode(Flags);
}

void SLAPI PPInventoryOpEx::SetAccelInputMode(int mode)
{
	if(mode == accsliCode) {
		Flags |= INVOPF_ACCELADDITEMS;
		Flags &= ~INVOPF_ACCELADDITEMSQTTY;
	}
	else if(mode == accsliCodeAndQtty) {
		Flags &= ~INVOPF_ACCELADDITEMS;
		Flags |= INVOPF_ACCELADDITEMSQTTY;
	}
	else {
		Flags &= ~(INVOPF_ACCELADDITEMS|INVOPF_ACCELADDITEMSQTTY);
	}
}
//
// PPReckonOpEx
//
SLAPI PPReckonOpEx::PPReckonOpEx()
{
	Init();
}

void SLAPI PPReckonOpEx::Init()
{
	Beg = End = ZERODATE;
	Flags = 0;
	PersonRelTypeID = 0;
	memzero(Reserve, sizeof(Reserve));
	OpList.freeAll();
}

PPReckonOpEx & FASTCALL PPReckonOpEx::operator = (const PPReckonOpEx & src)
{
	Init();
	Beg = src.Beg;
	End = src.End;
	Flags = src.Flags;
	PersonRelTypeID = src.PersonRelTypeID;
	OpList.copy(src.OpList);
	return *this;
}

int SLAPI PPReckonOpEx::IsEmpty() const
{
	return BIN(!Beg && !End && !Flags && !OpList.getCount());
}

int SLAPI PPReckonOpEx::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
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
		Init();
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

int SLAPI PPReckonOpEx::PeriodToStr(SString & rBuf) const
{
	char   temp[64];
	char * p = temp;
	if(Beg == 0)
		if(Flags & ROXF_BEGISBILLDT)
			*p++ = '@';
	DateRange period;
	period.low = Beg;
	period.upp = End;
	periodfmt(&period, p);
	if(End == 0) {
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
	return 1;
}

int SLAPI PPReckonOpEx::StrToPeriod(const char * pBuf)
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
	period.SetZero();
	strtoperiod(temp, &period, 0);
	Beg = period.low;
	End = period.upp;
	SETFLAG(Flags, ROXF_BEGISBILLDT, !period.low && at_beg);
	SETFLAG(Flags, ROXF_ENDISBILLDT, !period.upp && at_end);
	return 1;
}

int SLAPI PPReckonOpEx::GetReckonPeriod(LDATE debtDate, DateRange * pPeriod) const
{
	if(checkdrange(debtDate, Beg, End)) {
		pPeriod->low = (Beg == 0 && Flags & ROXF_BEGISBILLDT) ? debtDate : ZERODATE;
		pPeriod->upp = (End == 0 && Flags & ROXF_ENDISBILLDT) ? debtDate : ZERODATE;
		return 1;
	}
	else {
		pPeriod->low.v = pPeriod->upp.v = MAXLONG;
		return -1;
	}
}

int SLAPI PPReckonOpEx::GetDebtPeriod(LDATE paymDate, DateRange * pPeriod) const
{
	pPeriod->low = Beg;
	pPeriod->upp = End;
	if(pPeriod->low == 0 && Flags & ROXF_BEGISBILLDT)
		pPeriod->low = paymDate;
	if(pPeriod->upp == 0 && Flags & ROXF_ENDISBILLDT)
		pPeriod->upp = paymDate;
	return 1;
}
//
// PPBillPoolOpEx
//
SLAPI PPBillPoolOpEx::PPBillPoolOpEx()
{
	Init();
}

void SLAPI PPBillPoolOpEx::Init()
{
	Flags = 0;
	memzero(Reserve, sizeof(Reserve));
	OpList.freeAll();
}

PPBillPoolOpEx & FASTCALL PPBillPoolOpEx::operator = (PPBillPoolOpEx & src)
{
	Init();
	Flags = src.Flags;
	OpList.copy(src.OpList);
	return *this;
}
//
// PPOprKindPacket
//
SLAPI PPOprKindPacket::PPOprKindPacket()
{
	P_IOE       = 0;
	P_DIOE      = 0;
	P_GenList   = 0;
	P_ReckonData = 0;
	P_PoolData   = 0;
	P_DraftData  = 0;
	Init();
}

SLAPI PPOprKindPacket::~PPOprKindPacket()
{
	delete P_IOE;
	delete P_DIOE;
	delete P_GenList;
	delete P_ReckonData;
	delete P_PoolData;
	delete P_DraftData;
}

void SLAPI PPOprKindPacket::Init()
{
	MEMSZERO(Rec);
	ATTmpls.freeAll();
	ExtString.Z();
	ZDELETE(P_IOE);
	ZDELETE(P_DIOE);
	ZDELETE(P_GenList);
	ZDELETE(P_ReckonData);
	ZDELETE(P_PoolData);
	ZDELETE(P_DraftData);
}

PPOprKindPacket & FASTCALL PPOprKindPacket::operator = (const PPOprKindPacket & rSrc)
{
	Init();
	Rec = rSrc.Rec;
	if(rSrc.P_IOE) {
		P_IOE = new PPInventoryOpEx;
		ASSIGN_PTR(P_IOE, *rSrc.P_IOE);
	}
	if(rSrc.P_DIOE) {
		P_DIOE = new PPDebtInventOpEx;
		ASSIGN_PTR(P_DIOE, *rSrc.P_DIOE);
	}
	if(rSrc.P_GenList) {
		P_GenList = new ObjRestrictArray;
		ASSIGN_PTR(P_GenList, *rSrc.P_GenList);
	}
	if(rSrc.P_ReckonData) {
		P_ReckonData = new PPReckonOpEx;
		ASSIGN_PTR(P_ReckonData, *rSrc.P_ReckonData);
	}
	if(rSrc.P_PoolData) {
		P_PoolData = new PPBillPoolOpEx;
		ASSIGN_PTR(P_PoolData, *rSrc.P_PoolData);
	}
	if(rSrc.P_DraftData) {
		P_DraftData = new PPDraftOpEx;
		ASSIGN_PTR(P_DraftData, *rSrc.P_DraftData);
	}
	ExtString = rSrc.ExtString;
	return *this;
}

int SLAPI PPOprKindPacket::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, OPKEXSTR_MEMO, ExtString, rBuf); }
int SLAPI PPOprKindPacket::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }
//
//
//
SLAPI PPDraftOpEx::PPDraftOpEx()
{
	Init();
}

void SLAPI PPDraftOpEx::Init()
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
struct OpkListEntry {
	PPID   id;
	char   name[48];
	int16  rank;
};

static IMPL_CMPFUNC(OpkListEntry, i1, i2)
{
	const OpkListEntry * p_i1 = (const OpkListEntry *)i1;
	const OpkListEntry * p_i2 = (const OpkListEntry *)i2;
	if(p_i1->rank > p_i2->rank)
		return -1;
	else if(p_i1->rank < p_i2->rank)
		return 1;
	else
		return stricmp866(p_i1->name, p_i2->name);
}

// static
StrAssocArray * SLAPI PPObjOprKind::MakeOprKindList(PPID linkOprKind, const PPIDArray * pOpList, uint flags)
{
	int    r;
	PPID   id = 0;
	PPOprKind opk;
	SArray temp_list(sizeof(OpkListEntry));
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
				entry.id = opk.ID;
				entry.rank = opk.Rank;
				STRNSCPY(entry.name, opk.Name);
				THROW_SL(temp_list.ordInsert(&entry, 0, PTR_CMPFUNC(OpkListEntry)));
			}
		}
	}
	THROW(r);
	{
		THROW_MEM(p_list = new StrAssocArray);
		for(uint i = 0; i < temp_list.getCount(); i++) {
			const OpkListEntry * p_entry = (const OpkListEntry *)temp_list.at(i);
			THROW_SL(p_list->Add(p_entry->id, p_entry->name));
		}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI SetupOprKindCombo(TDialog * dlg, uint ctl, PPID id, uint /*olwFlags*/, const PPIDArray * pOpList, uint opklFlags)
{
	int    ok = 0;
	ComboBox * p_combo = (ComboBox *)dlg->getCtrlView(ctl);
	if(p_combo) {
		// @v8.4.4 ListWindow * p_lw = CreateListWindow(PPObjOprKind::MakeOprKindList(0, pOpList, opklFlags), lbtDisposeData | lbtDblClkNotify);
		// @v8.4.4 {
		StrAssocArray * p_list = PPObjOprKind::MakeOprKindList(0, pOpList, opklFlags);
		PPObjListWindow * p_lw = new PPObjListWindow(PPOBJ_OPRKIND, p_list, lbtDisposeData|lbtDblClkNotify, 0);
		// } @v8.4.4
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

PPID SLAPI SelectOpKind(PPID linkOpID, PPIDArray * pOpList, uint opklFlags)
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
		p_lw->options |= (ofCenterX | ofCenterY);
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

PPID SLAPIV SelectOprKind(uint opklFlags, PPID linkOpID, ...)
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
SLAPI PPObjOprKind::PPObjOprKind(void * extraPtr) : PPObjReference(PPOBJ_OPRKIND, extraPtr)
{
}

//static
int SLAPI PPObjOprKind::GetATTemplList(PPID opID, PPAccTurnTemplArray * pList)
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

int SLAPI FetchInvOpEx(PPID opID, PPInventoryOpEx * pInvOpEx); // Prototype @defined(ppkernel\oputil.cpp)

int SLAPI PPObjOprKind::FetchInventoryData(PPID id, PPInventoryOpEx * pInvOpEx)
{
	return ::FetchInvOpEx(id, pInvOpEx);
}

int SLAPI PPObjOprKind::GetPacket(PPID id, PPOprKindPacket * pack)
{
	int    ok = 1;
	PPObjOpCounter opc_obj; // AHTOXA
	THROW(Search(id, &pack->Rec) > 0);
	THROW(opc_obj.GetPacket(pack->Rec.OpCounterID, &pack->OpCntrPack)); // AHTOXA
	THROW(GetExAmountList(id, &pack->Amounts));
	THROW(ref->GetPropVlrString(Obj, id, OPKPRP_EXTSTRDATA, pack->ExtString));
	pack->ATTmpls.freeAll();
	ZDELETE(pack->P_GenList);
	ZDELETE(pack->P_ReckonData);
	ZDELETE(pack->P_PoolData);
	if(pack->Rec.OpTypeID == PPOPT_INVENTORY) {
		PPInventoryOpEx ioe;
		ZDELETE(pack->P_IOE);
		if(ref->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_INVENTORY, &ioe, sizeof(ioe)) > 0) {
			THROW_MEM(pack->P_IOE = new PPInventoryOpEx);
			*pack->P_IOE = ioe;
		}
	}
	else if(pack->Rec.OpTypeID == PPOPT_GENERIC) {
		THROW_MEM(pack->P_GenList = new ObjRestrictArray);
		THROW(GetGenericList(id, pack->P_GenList));
	}
	else if(oneof3(pack->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
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
		if(ref->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_DEBTINVENT, &dioe, sizeof(dioe)) > 0) {
			THROW_MEM(pack->P_DIOE = new PPDebtInventOpEx);
			*pack->P_DIOE = dioe;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::PutPacket(PPID * pID, PPOprKindPacket * pack, int use_ta)
{
	int    ok = 1, r = 0;
	uint   i;
	PPOpCounter opc_rec;
	PPOpCounterPacket opc_pack;
	PPObjOpCounter opc_obj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pack->Rec.OpTypeID == PPOPT_INVENTORY)
			if(pack->P_IOE && pack->P_IOE->Flags & INVOPF_COSTNOMINAL) {
				pack->Rec.Flags |= OPKF_BUYING;
				pack->Rec.Flags &= ~OPKF_SELLING;
			}
			else {
				pack->Rec.Flags |= OPKF_SELLING;
				pack->Rec.Flags &= ~OPKF_BUYING;
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
			THROW(ref->GetItem(Obj, *pID, &org_rec) > 0);
			THROW_DB(deleteFrom(&ref->Prop, 0, (ref->Prop.ObjType == Obj && ref->Prop.ObjID == *pID && ref->Prop.Prop <= (long)PP_MAXATURNTEMPLATES)));
			THROW(ref->UpdateItem(Obj, *pID, &pack->Rec, 1, 0));
			if(org_rec.OpCounterID != pack->Rec.OpCounterID)
				if(ref->GetItem(PPOBJ_OPCOUNTER, org_rec.OpCounterID, &opc_rec) > 0)
					if(opc_rec.OwnerObjID)
						THROW(opc_obj.PutPacket(&org_rec.OpCounterID, 0, 0));
			Dirty(*pID);
		}
		else {
			*pID = pack->Rec.ID;
			THROW(ref->AddItem(PPOBJ_OPRKIND, pID, &pack->Rec, 0));
		}
		for(i = 0; i < pack->ATTmpls.getCount(); i++) {
			THROW(ref->PutProp(Obj, *pID, (PPID)(i+1), &pack->ATTmpls.at(i), sizeof(PPAccTurnTempl), 0));
		}
		if(pack->Rec.OpTypeID == PPOPT_GENERIC) {
			THROW(ref->PutPropArray(PPOBJ_OPRKIND, *pID, OPKPRP_GENLIST2, pack->P_GenList, 0));
		}
		else if(pack->Rec.OpTypeID == PPOPT_POOL) {
			THROW(SetPoolExData(*pID, pack->P_PoolData, 0));
		}
		else if(oneof3(pack->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
			THROW(SetDraftExData(*pID, pack->P_DraftData));
		}
		if(pack->Rec.Flags & OPKF_RECKON) {
			THROW(SetReckonExData(*pID, pack->P_ReckonData, 0));
		}
		THROW(ref->PutPropVlrString(Obj, *pID, OPKPRP_EXTSTRDATA, pack->ExtString));
		THROW(ref->PutPropArray(Obj, *pID, OPKPRP_EXAMTLIST, &pack->Amounts, 0));
		if(pack->Rec.OpTypeID == PPOPT_INVENTORY)
			THROW(ref->PutProp(Obj, *pID, OPKPRP_INVENTORY, pack->P_IOE));
		if(pack->Rec.SubType == OPSUBT_DEBTINVENT)
			THROW(ref->PutProp(Obj, *pID, OPKPRP_DEBTINVENT, pack->P_DIOE));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::GetExtStrData(PPID opID, int fldID, SString & rBuf)
{
	SString line_buf;
	rBuf.Z();
	if(ref->GetPropVlrString(Obj, opID, OPKPRP_EXTSTRDATA, line_buf) > 0)
		return PPGetExtStrData(fldID, OPKEXSTR_MEMO, line_buf, rBuf);
	return -1;
}

int SLAPI PPObjOprKind::GetExAmountList(PPID id, PPIDArray * pList) { return ref->GetPropArray(Obj, id, OPKPRP_EXAMTLIST, pList); }
int SLAPI PPObjOprKind::GetGenericList(PPID id, ObjRestrictArray * pList) { return ref->GetPropArray(Obj, id, OPKPRP_GENLIST2, pList); }

int SLAPI PPObjOprKind::GetGenericList(PPID id, PPIDArray * pList)
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

int SLAPI PPObjOprKind::SetReckonExData(PPID id, PPReckonOpEx * pData, int use_ta)
{
	int    ok = 1;
	PPIDArray temp;
	if(pData) {
		uint   i;
		THROW_SL(temp.add((long)pData->Beg));
		THROW_SL(temp.add((long)pData->End));
		THROW_SL(temp.add(pData->Flags));
		THROW_SL(temp.add(pData->PersonRelTypeID));
		for(i = 0; i < sizeof(pData->Reserve) / sizeof(long); i++)
			THROW_SL(temp.add(pData->Reserve[i]));
		for(i = 0; i < pData->OpList.getCount(); i++)
			THROW_SL(temp.add(pData->OpList.at(i)));
	}
	THROW(ref->PutPropArray(Obj, id, OPKPRP_PAYMOPLIST, (pData ? &temp : (PPIDArray*)0), use_ta));
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::GetReckonExData(PPID id, PPReckonOpEx * pData)
{
	int    ok = -1;
	PPIDArray temp;
	if(ref->GetPropArray(Obj, id, OPKPRP_PAYMOPLIST, &temp) > 0) {
		if(temp.getCount() < ROX_HDR_DW_COUNT) {
			CALLPTRMEMB(pData, Init());
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
						pData->Init();
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

int SLAPI PPObjOprKind::SetDraftExData(PPID id, const PPDraftOpEx * pData)
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
	return BIN(ref->PutProp(Obj, id, OPKPRP_DRAFT, p_strg, sz));
}

int SLAPI PPObjOprKind::GetDraftExData(PPID id, PPDraftOpEx * pData)
{
	int    ok = -1;
	PPDraftOpEx_Strg strg;
	if(ref->GetProperty(Obj, id, OPKPRP_DRAFT, &strg, sizeof(strg)) > 0) {
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

int SLAPI PPObjOprKind::SetPoolExData(PPID id, PPBillPoolOpEx * pData, int use_ta)
{
	int    ok = 1;
	PPIDArray temp;
	if(pData) {
		uint   i;
		THROW_SL(temp.add(pData->Flags));
		for(i = 0; i < sizeof(pData->Reserve) / sizeof(long); i++)
			THROW(temp.add(pData->Reserve[i]));
		for(i = 0; i < pData->OpList.getCount(); i++)
			THROW(temp.add(pData->OpList.at(i)));
	}
	THROW(ref->PutPropArray(Obj, id, OPKPRP_BILLPOOL, (pData ? &temp : (PPIDArray*)0), use_ta));
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::GetPoolExData(PPID id, PPBillPoolOpEx * pData)
{
	PPIDArray temp;
	if(ref->GetPropArray(Obj, id, OPKPRP_BILLPOOL, &temp) > 0) {
		if(temp.getCount() < BPOX_HDR_DW_COUNT) {
			CALLPTRMEMB(pData, Init());
	   	    return -1;
		}
		if(pData) {
			pData->Flags = temp.at(0);
			memzero(pData->Reserve, sizeof(pData->Reserve));
			for(uint i = BPOX_HDR_DW_COUNT; i < temp.getCount(); i++)
				if(!pData->OpList.add(temp.at(i))) {
					pData->Init();
					return PPSetErrorSLib();
				}
		}
		return 1;
	}
	return -1;
}

int SLAPI PPObjOprKind::GetPaymentOpList(PPID linkOpID, PPIDArray * pList)
{
	int    ok = 1;
	PROFILE_START
	if(pList) {
		PPOprKind op_rec;
		for(SEnum en = ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
			if(op_rec.OpTypeID == PPOPT_PAYMENT && (!linkOpID || op_rec.LinkOpID == linkOpID)) {
				if(!pList->add(op_rec.ID))
					ok = PPSetErrorSLib();
			}
		}
	}
	PROFILE_END
	return ok;
}

int FASTCALL GetOpList(PPID opTypeID, PPIDArray * pList)
{
	int    ok = -1;
	PPOprKind op_rec;
	for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); en.Next(&op_rec) > 0;) {
		if(!opTypeID || op_rec.OpTypeID == opTypeID) {
			if(pList)
				pList->addUnique(op_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPObjOprKind::GetPayableOpList(PPID accSheetID, PPIDArray * pList)
{
	int    ok = 1;
	PPOprKind op_rec;
	if(pList) {
		for(SEnum en = ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
   		    if((accSheetID == -1 || op_rec.AccSheetID == accSheetID) && op_rec.Flags & OPKF_NEEDPAYMENT) {
				if(op_rec.OpTypeID != PPOPT_GOODSORDER || !(CConfig.Flags & CCFLG_IGNOREORDERSDEBT))
					if(!pList->add(op_rec.ID))
						ok = PPSetErrorSLib();
			}
		}
	}
	return ok;
}

int SLAPI PPObjOprKind::GetProfitableOpList(PPID accSheetID, PPIDArray * pList)
{
	int    ok = 1;
	PPOprKind op_rec;
	if(pList) {
		for(SEnum en = ref->Enum(PPOBJ_OPRKIND, 0); ok && en.Next(&op_rec) > 0;) {
   		    if(op_rec.AccSheetID == accSheetID && op_rec.Flags & OPKF_PROFITABLE)
				if(!pList->add(op_rec.ID))
					ok = PPSetErrorSLib();
		}
	}
	return ok;
}

SLAPI PPObjOprKind::ReservedOpCreateBlock::ReservedOpCreateBlock()
{
	THISZERO();
}

int SLAPI PPObjOprKind::Helper_GetReservedOp(PPID * pID, const ReservedOpCreateBlock & rBlk, int use_ta)
{
	// @v9.4.8 assert(rBlk.OpID);
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
        MEMSZERO(op_rec);
		op_pack.Rec.ID = rBlk.OpID;
        PPLoadText(rBlk.NameTxtId, temp_buf);
        THROW(temp_buf.NotEmptyS());
        STRNSCPY(op_pack.Rec.Name, temp_buf);
        STRNSCPY(op_pack.Rec.Symb, rBlk.P_Symb);
        op_pack.Rec.AccSheetID = rBlk.AccSheetID; // @v9.4.8
        op_pack.Rec.OpTypeID = rBlk.OpTypeID;
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

int SLAPI PPObjOprKind::GetEdiRecadvOp(PPID * pID, int use_ta)
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

int SLAPI PPObjOprKind::GetEdiShopChargeOnOp(PPID * pID, int use_ta)
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

int SLAPI PPObjOprKind::GetEdiStockOp(PPID * pID, int use_ta)
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

int SLAPI PPObjOprKind::GetEdiWrOffShopOp(PPID * pID, int use_ta)
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

//virtual
int SLAPI PPObjOprKind::MakeReserved(long flags)
{
    int    ok = -1;
    if(flags & mrfInitializeDb) {
		/*
// PPTXT_OPK_COMM_GENERICACCTURN  "Общая бухгалтерская проводка"
// PPTXT_OPK_COMM_RECEIPT         "Приход товара от поставщика"
// PPTXT_OPK_COMM_SALE            "Продажа покупателю"
// PPTXT_OPK_COMM_RETAIL          "Розничная продажа"
// PPTXT_OPK_COMM_INTREXPEND      "Внутренняя передача"
// PPTXT_OPK_COMM_INTRRECEIPT     "Межскладской приход"
PPTXT_OPK_COMM_INVENTORY       "Инвентаризация"
// PPTXT_OPK_COMM_ORDER           "Заказ от покупателя"
// PPTXT_OPK_COMM_PURCHASE        "Закупка"
		*/
		long    _count = 0;
		PPOprKind op_rec;
		{
			for(SEnum en = ref->Enum(Obj, 0); en.Next(&op_rec) > 0;) {
				_count++;
			}
		}
        if(_count == 0) {
			PPObjAccSheet acs_obj;
			PPAccSheet acs_rec;
			PPIDArray acs_id_list;
			{
				ReservedOpCreateBlock blk;
				PPID   op_id = blk.OpID = PPOPK_GENERICACCTURN;
				blk.OpTypeID = PPOPT_ACCTURN;
				blk.NameTxtId = PPTXT_OPK_COMM_GENERICACCTURN;
				blk.Flags = 0;
				blk.P_Symb = "OP-GENATURN";
				blk.P_CodeTempl = "AT%05";
				THROW(Helper_GetReservedOp(&op_id, blk, 1));
			}
			{
				ReservedOpCreateBlock blk;
				PPID   op_id = blk.OpID = PPOPK_RETAIL;
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
						PPID   op_id = blk.OpID = PPOPK_RECEIPT;
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
						PPID   op_id = blk.OpID = 0;
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
						PPID   op_id = blk.OpID = PPOPK_SELL;
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
						PPID   op_id = blk.OpID = PPOPK_INTREXPEND;
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
						PPID   op_id = blk.OpID = PPOPK_INTRRECEIPT;
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

// virtual
StrAssocArray * SLAPI PPObjOprKind::MakeStrAssocList(void * extraPtr)
{
	const PPID op_type_id = (PPID)extraPtr;
	PPIDArray op_list;
	op_list.add(op_type_id);
	return MakeOprKindList(0, (op_type_id ? &op_list : 0), 0);
}

// Prototype
int SLAPI EditInventoryOptionsDialog(PPInventoryOpEx *);
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

	int    ExtDataKind;
	PPID   OpTypeID;
	PPID   LinkOpID_;
	PPID   OpCounterID;
	PPObjOprKind OpObj;
};

OprKindView::OprKindView(PPID extData, int extDataKind) :
	PPListDialog(((extDataKind == 2) ? DLG_OPLINKSVIEW : DLG_OPKVIEW), CTL_OPKVIEW_LIST), 
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
		else if(r == 0)
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
		if(r == 0)
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
int SLAPI PPObjOpCounter::Browse(void * extraPtr)
{
	class OpCounterView : public ObjViewDialog {
	public:
		OpCounterView(PPObjOpCounter * pObj) : ObjViewDialog(DLG_OPCNTRVIEW, pObj, 0)
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
	OprKindDialog(uint rezID, PPOprKindPacket * pData) : TDialog(rezID), P_Data(pData)
	{
		P_AtObj = BillObj->atobj;
		P_ListBox = (SmartListBox*)getCtrlView(CTL_OPRKIND_LIST);
		IsGeneric = BIN(P_Data->Rec.OpTypeID == PPOPT_GENERIC);
		IsDraft   = BIN(oneof3(P_Data->Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT));
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

	//void   cdecl editOptions(uint dlgID, int useMainAmt, const PPIDArray *, ...);
	void   editOptions2(uint dlgID, int useMainAmt, const PPIDArray * pSubTypeList, PPIDArray * pFlagsList, PPIDArray * pExtFlagsList);
	void   editInventoryOptions();
	void   editPoolOptions();
	void   editExtension();

	void   prnOptDialog();
	void   exAmountList();
	int    setAccTextToList(AcctID *, long, long, long, SString & rBuf);

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
	int    modatt = OpObj.CheckRights(OPKRT_MODIFYATT);
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
	disableCtrl(CTLSEL_OPRKIND_TYPE, P_Data->Rec.ID || P_Data->Rec.OpTypeID);
	enableCommand(cmOprKindPaymList, BIN(P_Data->Rec.Flags & OPKF_RECKON));
	if(!IsGeneric) {
		enableCommand(cmaInsert, modatt);
		enableCommand(cmaEdit,   modatt);
		enableCommand(cmaDelete, modatt);
	}
	enableCommand(cmaMore,   OpObj.CheckRights(OPKRT_MODIFYOPTIONS));
	enableCommand(cmOprKindPrnOpt, OpObj.CheckRights(OPKRT_MODIFYOPTIONS));
	disableCtrl(CTL_OPRKIND_CVAL, !OpObj.CheckRights(OPKRT_MODIFYCOUNTER));
	SetupPPObjCombo(this, CTLSEL_OPRKIND_TYPE, PPOBJ_OPRTYPE, P_Data->Rec.OpTypeID, OLW_CANINSERT, 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_ACCSHEET, PPOBJ_ACCSHEET, P_Data->Rec.AccSheetID, OLW_CANINSERT, 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_ACCSHEET2, PPOBJ_ACCSHEET, P_Data->Rec.AccSheet2ID, OLW_CANINSERT, 0);
	if(P_Data->Rec.OpTypeID == PPOPT_CORRECTION) {
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, 0L); // @v9.4.3 PPOPT_GOODSEXPEND
	}
	else {
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, 0L);
		if(P_Data->Rec.OpTypeID == PPOPT_PAYMENT) {
			types.addzlist(PPOPT_ACCTURN, PPOPT_CHARGE, PPOPT_GOODSACK, PPOPT_POOL, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
		}
		else if(P_Data->Rec.OpTypeID == PPOPT_CHARGE)
			types.add(PPOPT_ACCTURN);
	}
	SetupOprKindCombo(this, CTLSEL_OPRKIND_LINK, P_Data->Rec.LinkOpID, 0, &types, P_Data->Rec.ID ? OPKLF_SHOWPASSIVE : 0);
	SetupPPObjCombo(this, CTLSEL_OPRKIND_INITST, PPOBJ_BILLSTATUS, P_Data->Rec.InitStatusID, 0, 0);
	if(IsDraft) {
		PPDraftOpEx opex_rec;
		if(P_Data->P_DraftData)
			opex_rec = *P_Data->P_DraftData;
		types.freeAll();
		if(P_Data->Rec.OpTypeID == PPOPT_DRAFTRECEIPT)
			types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_GOODSORDER, PPOPT_DRAFTRECEIPT, 0L);
		else if(P_Data->Rec.OpTypeID == PPOPT_DRAFTEXPEND)
			types.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSMODIF, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, 0L); // @v8.6.2 PPOPT_DRAFTRECEIPT
		if(P_Data->Rec.SubType == OPSUBT_TRADEPLAN)
			types.add(PPOPT_GENERIC);
		types.add(PPOPT_ACCTURN); // @v9.7.10
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
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 5, DROXF_MULTDRFTWROFF); // @v8.8.11
		AddClusterAssoc(CTL_OPRKIND_DRFTOPTION, 6, DROXF_SELSUPPLONCOMPL); // @v9.1.12
		SetClusterData(CTL_OPRKIND_DRFTOPTION, opex_rec.Flags);
	}
	enableCommand(cmOprKindExt, P_Data->Rec.OpTypeID == PPOPT_ACCTURN && P_Data->Rec.SubType == OPSUBT_DEBTINVENT);
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
			if(P_Data->ATTmpls.insert(&tmpl) == 0)
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
			PPAccTurnTempl tmp;
			PPAccTurnTempl & r_tmpl = P_Data->ATTmpls.at((uint)(id-1));
			tmp = r_tmpl;
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

static void SLAPI MakeOpTypeListForGeneric(PPIDArray & rList)
{
	rList.clear();
	rList.addzlist(PPOPT_ACCTURN, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN,
		PPOPT_GOODSREVAL, PPOPT_GOODSORDER, PPOPT_GOODSMODIF, PPOPT_GOODSACK, PPOPT_PAYMENT,
		PPOPT_CHARGE, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_CORRECTION, 0L);
}

static int SLAPI AddGenOpItems(ObjRestrictArray & rList)
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
				const PPID id_to_remove = rList.at(--i).ObjID;
				if(!dd_list.bsearch(id_to_remove)) {
					rList.atFree(i);
					ok = 1;
				}
			} while(i);
		}
		for(i = 0; i < dd_list.getCount(); i++) {
			ObjRestrictItem new_item;
			new_item.Flags = 0;
			new_item.ObjID = dd_list.get(i);
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

static int SLAPI EditGenOpItem(ObjRestrictItem * pItem)
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
			dlg->disableCtrl(CTLSEL_GENOPITEM_OP, 1);
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
		/* @v9.8.6 
		ObjRestrictItem item;
		MEMSZERO(item);
		while(EditGenOpItem(&item) > 0)
			if(P_Data->P_GenList->CheckUniqueID(item.ObjID)) {
				P_Data->P_GenList->insert(&item);
				updateList();
				return;
			}
			else
				PPError(PPERR_DUPGENOPITEM, 0);
		*/
		// @v9.8.6 {
		if(AddGenOpItems(*P_Data->P_GenList) > 0)
			updateList();
		// } @v9.8.6 
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

int OprKindDialog::setAccTextToList(AcctID * acctid, long flgs, long accFixMask, long artFixMask, SString & rBuf)
{
	int    ok = 1;
	Acct   acct;
	PPID   cur_id = 0;
	if(P_AtObj->ConvertAcctID(acctid, &acct, &cur_id, 1 /* useCache */)) {
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
				for(uint i = 0; P_Data->P_GenList->enumItems(&i, (void**)&p_ori);) {
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
			for(uint i = 0; P_Data->ATTmpls.enumItems(&i, (void**)&e);) {
				StringSet ss(SLBColumnDelim);
				THROW(setAccTextToList(&e->DbtID, e->Flags, ATTF_DACCFIX, ATTF_DARTFIX, sub));
				ss.add(sub);
				THROW(setAccTextToList(&e->CrdID, e->Flags, ATTF_CACCFIX, ATTF_CARTFIX, sub));
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
	getCtrlData(CTLSEL_OPRKIND_INITST,     &P_Data->Rec.InitStatusID);
	THROW_PP(P_Data->Rec.OpTypeID, PPERR_OPRTYPENEEDED);
	if(oneof3(P_Data->Rec.OpTypeID, PPOPT_PAYMENT, PPOPT_GOODSRETURN, PPOPT_CORRECTION)) // @v7.8.10 PPOPT_CORRECTION
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
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
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
					P_Data->Rec.PrnOrder = (int16)temp_long;
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
#if 0 // @v9.3.6 {
void OprKindDialog::editOptions(uint dlgID, int useMainAmt, const PPIDArray * pSubTypeList, ...)
{
	va_list ap;
	PPIDArray options;
	int    valid_data = 0;
	uint   i;
	int    warn_noupdlotrestflagdisabled = 0;
	ushort v = 0;
	long   _o, s, b, f = P_Data->Rec.Flags;
	ulong  o;
	TDialog * dlg = 0;
	SString memo_tmpl;
	SString obj2name;
	SString amt_formula;

	va_start(ap, pSubTypeList);
	while((_o = va_arg(ap, long)) != 0)
		options.add(_o);
	if(CheckDialogPtrErr(&(dlg = new TDialog(dlgID)))) {
		for(i = 0; i < options.getCount(); i++) {
			o = (ulong)options.at(i);
			SETFLAG(v, (1 << i), f & o);
		}
		dlg->setCtrlData(CTL_OPKMORE_FLAGS, &v);
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
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_OPKMORE_FLAGS,  &v);
			for(i = 0; i < options.getCount(); i++) {
				o = (ulong)options.at(i);
				SETFLAG(f, o, v & (1 << i));
			}
			f &= ~(OPKF_BUYING | OPKF_SELLING);
			if(useMainAmt) {
				// ahtoxa {
				PPBillPacket bill_pack;
				SString temp_formula;
				dlg->getCtrlString(CTL_OPKMORE_AMTFORMULA, amt_formula);
				amt_formula.Strip();
				if(amt_formula.CmpPrefix("ignfix", 1) == 0)
					(temp_formula = amt_formula).ShiftLeft(sstrlen("ignfix")).Strip();
				else
					temp_formula = amt_formula;
				if(amt_formula.Strip().Empty() || PPCalcExpression(temp_formula, 0, &bill_pack, 0, 0) > 0) {
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
	va_end(ap);
	enableCommand(cmOprKindExt, P_Data->Rec.OpTypeID == PPOPT_ACCTURN && P_Data->Rec.SubType == OPSUBT_DEBTINVENT);
}
#endif // } 0 @v9.3.6

void OprKindDialog::editOptions2(uint dlgID, int useMainAmt, const PPIDArray * pSubTypeList, PPIDArray * pFlagsList, PPIDArray * pExtFlagsList)
{
	PPIDArray options, ext_options;
	int    valid_data = 0;
	uint   i;
	int    warn_noupdlotrestflagdisabled = 0;
	ushort v = 0;
	long   s, b, f = P_Data->Rec.Flags;
	long   ext_f = P_Data->Rec.ExtFlags;
	ulong  o, ext_o;
	TDialog * dlg = 0;
	SString memo_tmpl;
	SString obj2name;
	SString amt_formula;
	RVALUEPTR(options, pFlagsList);
	RVALUEPTR(ext_options, pExtFlagsList);
	if(CheckDialogPtrErr(&(dlg = new TDialog(dlgID)))) {
		v = 0;
		for(i = 0; i < options.getCount(); i++) {
			o = (ulong)options.at(i);
			SETFLAG(v, (1 << i), f & o);
		}
		dlg->setCtrlUInt16(CTL_OPKMORE_FLAGS, v);
		v = 0;
		for(i = 0; i < ext_options.getCount(); i++) {
			ext_o = (ulong)ext_options.at(i);
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
		// @v8.8.6 {
		if(dlg->getCtrlView(CTL_OPKMORE_MCR)) {
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 0, OPKFX_MCR_GROUP);
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 1, OPKFX_MCR_SUBSTSTRUC);
			dlg->AddClusterAssoc(CTL_OPKMORE_MCR, 2, OPKFX_MCR_EQQTTY);
			dlg->SetClusterData(CTL_OPKMORE_MCR, ext_f);
		}
		// } @v8.8.6
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_OPKMORE_FLAGS,  &(v = 0));
			for(i = 0; i < options.getCount(); i++) {
				o = (ulong)options.at(i);
				SETFLAG(f, o, v & (1 << i));
			}
			f &= ~(OPKF_BUYING | OPKF_SELLING);
			dlg->getCtrlData(CTL_OPKMORE_EXTFLAGS,  &(v = 0));
			for(i = 0; i < ext_options.getCount(); i++) {
				ext_o = (ulong)ext_options.at(i);
				SETFLAG(ext_f, ext_o, v & (1 << i));
			}
			// @v8.8.6 {
			if(dlg->getCtrlView(CTL_OPKMORE_MCR)) {
				dlg->GetClusterData(CTL_OPKMORE_MCR, &ext_f);
			}
			// } @v8.8.6
			if(useMainAmt) {
				// ahtoxa {
				PPBillPacket bill_pack;
				SString temp_formula;
				dlg->getCtrlString(CTL_OPKMORE_AMTFORMULA, amt_formula);
				amt_formula.Strip();
				if(amt_formula.CmpPrefix("ignfix", 1) == 0)
					(temp_formula = amt_formula).ShiftLeft(sstrlen("ignfix")).Strip();
				else
					temp_formula = amt_formula;
				if(amt_formula.Strip().Empty() || PPCalcExpression(temp_formula, 0, &bill_pack, 0, 0) > 0) {
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
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_ADVANCEREP, OPSUBT_REGISTER,
					OPSUBT_WARRANT, OPSUBT_DEBTINVENT, OPSUBT_ACCWROFF, OPSUBT_POSCORRECTION, 0L); // @v10.0.0 OPSUBT_POSCORRECTION
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_AUTOWL, OPKF_EXTACCTURN,
					OPKF_RENT, OPKF_BANKING, OPKF_CURTRANSIT,
					OPKF_PROFITABLE, OPKF_OUTBALACCTURN, OPKF_ADVACC, OPKF_ATTACHFILES, OPKF_FREIGHT, 0L);
				editOptions2(DLG_OPKMORE_AT, 2, &subtypelist, &options_list, 0);
				break;
			case PPOPT_GOODSEXPEND:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES, OPKF_AUTOWL,
					OPKF_USEPAYER, OPKF_RENT, OPKF_FREIGHT, OPKF_NOCALCTIORD, OPKF_NOUPDLOTREST, OPKF_PCKGMOUNTING,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				// @v8.9.6 OPKFX_CANBEDECLINED // @v10.0.0 OPKFX_AUTOGENUUID
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, 0L); 
				editOptions2(DLG_OPKMORE_GEX, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSRECEIPT:
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_ASSETRCV, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_ONORDER, OPKF_CALCSTAXES,
					OPKF_AUTOWL, OPKF_USEPAYER, OPKF_RENT, OPKF_FREIGHT, OPKF_NOUPDLOTREST,
					OPKF_ATTACHFILES, OPKF_NEEDVALUATION, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_UNLINKRET, 
					OPKFX_DLVRLOCASWH, OPKFX_AUTOGENUUID, 0L); // @v9.1.10 OPKFX_DLVRLOCASWH // @v10.0.0 OPKFX_AUTOGENUUID
				editOptions2(DLG_OPKMORE_GRC, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSMODIF:
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_ASSETMODIF, 0L);
				options_list.addzlist(OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEPAYER, OPKF_NOCALCTIORD,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				// @v9.0.0 // @v9.3.6 OPKFX_SOURCESERIAL // @v10.0.0 OPKFX_AUTOGENUUID
				ext_options_list.addzlist(OPKFX_DSBLHALFMODIF, OPKFX_SOURCESERIAL, OPKFX_AUTOGENUUID, 0L); 
				editOptions2(DLG_OPKMORE_MDF, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSRETURN:
				options_list.addzlist(OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_FREIGHT, OPKF_NOUPDLOTREST,
					OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, 
					OPKFX_AUTOGENUUID, 0L); // @v9.2.0 // @v10.0.0 OPKFX_AUTOGENUUID
				editOptions2(DLG_OPKMORE_RET, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_GOODSREVAL:
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_ASSETEXPL, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_ATTACHFILES,
					OPKF_DENYREVALCOST, OPKF_RESTRICTBYMTX, 0L);
				editOptions2(DLG_OPKMORE_GRV, 1, &subtypelist, &options_list, 0);
				break;
			case PPOPT_GOODSORDER:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEPAYER, OPKF_RENT,
					OPKF_FREIGHT, OPKF_ORDEXSTONLY, OPKF_ORDRESERVE, OPKF_ORDERBYLOC, OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, OPKF_NOCALCTIORD, 0L); // @v8.2.6 OPKF_NOCALCTIORD
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_IGNORECLISTOP, 
					OPKFX_AUTOGENUUID, 0L); // @v9.8.4 OPKFX_IGNORECLISTOP // @v10.0.0 OPKFX_AUTOGENUUID
				editOptions2(DLG_OPKMORE_ORD, 1, 0, &options_list, &ext_options_list);
				break;
			case PPOPT_PAYMENT:
				options_list.addzlist(OPKF_PROFITABLE, OPKF_AUTOWL, OPKF_BANKING, OPKF_ATTACHFILES, 0L);
				editOptions2(DLG_OPKMORE_PAY, 0, 0, &options_list, 0);
				break;
			case PPOPT_CHARGE:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_AUTOWL, OPKF_USEPAYER,
					OPKF_CHARGENEGPAYM, OPKF_ATTACHFILES, 0L);
				editOptions2(DLG_OPKMORE_CHG, 0, 0, &options_list, 0);
				break;
			case PPOPT_INVENTORY:
				editInventoryOptions();
				break;
			case PPOPT_POOL:
				editPoolOptions();
				break;
			case PPOPT_DRAFTRECEIPT:
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_TRADEPLAN, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEPAYER,
					OPKF_FREIGHT, OPKF_ATTACHFILES, OPKF_NEEDVALUATION, OPKF_RESTRICTBYMTX, 0L);
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_USESUPPLDEAL, OPKFX_CANBEDECLINED, 
					OPKFX_DLVRLOCASWH, OPKFX_SOURCESERIAL, OPKFX_AUTOGENUUID, 0L);
					// @v8.3.3 OPKFX_CANBEDECLINED // @v9.1.10 OPKFX_DLVRLOCASWH // @v9.3.6 OPKFX_SOURCESERIAL // @v10.0.0 OPKFX_AUTOGENUUID
				editOptions2(DLG_OPKMORE_DRC, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			case PPOPT_DRAFTEXPEND:
				subtypelist.addzlist((long)OPSUBT_COMMON, OPSUBT_TRADEPLAN, 0L);
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEPAYER,
					OPKF_FREIGHT, OPKF_NOCALCTIORD, OPKF_ATTACHFILES, OPKF_RESTRICTBYMTX, 0L);
				// @v8.3.3 OPKFX_CANBEDECLINED @v10.0.0 OPKFX_AUTOGENUUID
				ext_options_list.addzlist(OPKFX_ALLOWPARTSTR, OPKFX_RESTRICTPRICE, OPKFX_CANBEDECLINED, OPKFX_AUTOGENUUID, 0L); 
				editOptions2(DLG_OPKMORE_DEX, 1, &subtypelist, &options_list, &ext_options_list);
				break;
			default:
				options_list.addzlist(OPKF_NEEDPAYMENT, OPKF_PROFITABLE,
					OPKF_ONORDER, OPKF_CALCSTAXES, OPKF_AUTOWL, OPKF_USEPAYER,
					OPKF_EXTACCTURN, OPKF_RENT, OPKF_NOCALCTIORD, OPKF_FREIGHT, OPKF_ATTACHFILES, 0L);
				editOptions2(DLG_OPKMORE, 1, 0, &options_list, 0);
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
			op_type_list.add(PPOPT_ACCTURN);
			op_type_list.add(PPOPT_GOODSRECEIPT);
			op_type_list.add(PPOPT_GOODSEXPEND);
			SetupOprKindCombo(dlg, CTLSEL_OPKDINVE_WRDNOP, rec.WrDnOp, 0, &op_type_list, 0);
			dlg->setGroupData(1, &wd_goods_rec);
			dlg->setGroupData(2, &wu_goods_rec);
			SetupPPObjCombo(dlg, CTLSEL_OPKDINVE_WRUPOP, PPOBJ_OPRKIND, rec.WrUpOp, 0, (void *)PPOPT_ACCTURN);
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
	OpListDialog(uint dlgID, uint listCtlID, PPIDArray * pOpList, PPIDArray * pOpTypesList) :
		PPListDialog(dlgID, listCtlID)
	{
		RVALUEPTR(OpTypesList, pOpTypesList);
		if(P_Box && P_Box->def)
			P_Box->def->SetOption(lbtFocNotify, 1);
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
		if(TVCOMMAND && TVCMD == cmaAltInsert) {
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
		PPID   id = OpListData.at(i);
		if(GetObjectName(PPOBJ_OPRKIND, id, name_buf = 0, 0) <= 0)
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
		int    is_new = (id == 0);
		PPObjOprKind opk_obj;
		if((r = opk_obj.Edit(&id, 0, 0)) == cmCancel)
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
	if(pos >= 0 && pos < (long)OpListData.getCount()) {
		OpListData.atFree((uint)pos);
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
		OpkPaymListDialog(PPReckonOpEx * pData, PPIDArray * pOpTypesList) :
			OpListDialog(DLG_OPRPOP, CTL_OPRPOP_LIST, &pData->OpList, pOpTypesList)
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
		if(!P_Data->P_ReckonData)
			P_Data->P_ReckonData = new PPReckonOpEx;
		dlg = new OpkPaymListDialog(P_Data->P_ReckonData, &op_types_list);
		if(CheckDialogPtrErr(&dlg)) {
			if(ExecView(dlg) == cmOK)
				dlg->getDTS(P_Data->P_ReckonData);
			delete dlg;
		}
	}
}

class DiffByLocCntrDlg : public PPListDialog {
public:
	DiffByLocCntrDlg() : PPListDialog(DLG_DIFFCNTR, CTL_DIFFCNTR_LOCLIST)
	{
		/*updateList(-1);*/
	}
	int    setupCounter(PPID locID);
	int    setDTS(const LAssocArray * pData);
	int    getDTS(LAssocArray * pData);
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	PPID   getCurrID()
	{
		PPID   id = 0;
		return (P_Box && P_Box->getCurID(&id)) ? id : 0;
	}
	LAssocArray Data;
	PPObjLocation LObj;
	PPID   LocID;
};

IMPL_HANDLE_EVENT(DiffByLocCntrDlg)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmLBItemFocused)) {
		const PPID loc_id = getCurrID();
		if(loc_id)
			if(!setupCounter(loc_id))
				PPError();
	}
	else if(event.isClusterClk(CTL_DIFFCNTR_LOCLIST)) {
		const PPID loc_id = getCurrID();
		updateList(loc_id);
		if(P_Box && P_Box->def)
			P_Box->def->top();
	}
	else
		return;
	clearEvent(event);
}

int DiffByLocCntrDlg::setupCounter(PPID locID)
{
	if(LocID != locID) {
		// запоминаем значение счетчика предыдущего склада
		long   val = getCtrlLong(CTL_DIFFCNTR_VAL);
		val = (val < 0) ? 0 : val;
		if(!Data.Search(LocID, 0, 0))
			Data.AddUnique(LocID, val, 0);
		else
			Data.Update(LocID, val);
		// выставляем значение счетчика нового склада
		if(locID) {
			Data.Search(LocID = locID, &(val = 0), 0);
			setCtrlData(CTL_DIFFCNTR_VAL, &val);
		}
	}
	return 1;
}

int DiffByLocCntrDlg::setupList()
{
	if(P_Box) {
		P_Box->setDef(LObj.Selector(0));
		P_Box->Draw_();
	}
	return 1;
}

int DiffByLocCntrDlg::setDTS(const LAssocArray * pData)
{
	if(!RVALUEPTR(Data, pData))
		Data.freeAll();
	setupList();
	if(P_Box && P_Box->def) {
		long   val = 0;
		Data.Search(LocID = LConfig.Location, &val, 0);
		P_Box->search(&LocID, CMPF_LONG, lbSrchByID);
		setCtrlData(CTL_DIFFCNTR_VAL, &val);
	}
	return 1;
}

int DiffByLocCntrDlg::getDTS(LAssocArray * pData)
{
	setupCounter(0);
	CALLPTRMEMB(pData, copy(Data));
	return 1;
}

class OpCntrDialog : public TDialog {
public:
	OpCntrDialog(uint resID) : TDialog(resID)
	{
	}
	int    setDTS(const PPOpCounterPacket *);
	int    getDTS(PPOpCounterPacket *);
private:
	DECL_HANDLE_EVENT;
	int    editDiffCounters(LAssocArray *);
	PPObjOpCounter OpcObj;
	PPOpCounterPacket Data;
};

int OpCntrDialog::editDiffCounters(LAssocArray * pData) { DIALOG_PROC_BODY(DiffByLocCntrDlg, pData); }

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

int OpCntrDialog::setDTS(const PPOpCounterPacket * pData)
{
	Data = *pData;

	PPOpCounter opc_rec;
	opc_rec =  Data.Head;
	setCtrlData(CTL_OPKCOUNTER_TEMPL, opc_rec.CodeTemplate);
	setCtrlData(CTL_OPKCOUNTER_COUNTER, &opc_rec.Counter);
	AddClusterAssoc(CTL_OPKCOUNTER_FLAGS, 0, OPCNTF_LOCKINCR);
	AddClusterAssoc(CTL_OPKCOUNTER_FLAGS, 1, OPCNTF_DIFFBYLOC);
	SetClusterData(CTL_OPKCOUNTER_FLAGS, opc_rec.Flags);
	enableCommand(cmDiffByLoc, BIN(opc_rec.Flags & OPCNTF_DIFFBYLOC));
	if(resourceID == DLG_OPKCOUNTER)
		SetupPPObjCombo(this, CTLSEL_OPKCOUNTER_CNTR, PPOBJ_OPCOUNTER, opc_rec.OwnerObjID ? 0 : opc_rec.ID, OLW_CANINSERT, 0);
	else {
		setCtrlData(CTL_OPKCOUNTER_NAME, opc_rec.Name);
		setCtrlData(CTL_OPKCOUNTER_SYMB, opc_rec.Symb); // @v8.8.11
		setCtrlData(CTL_OPKCOUNTER_ID,   &opc_rec.ID);
		disableCtrl(CTL_OPKCOUNTER_ID, (!PPMaster || opc_rec.ID));
	}
	return 1;
}

int OpCntrDialog::getDTS(PPOpCounterPacket * pData)
{
	int    ok = 1;
	uint   sel = 0;
	char   temp_buf[64];
	getCtrlData(sel = CTL_OPKCOUNTER_TEMPL, temp_buf);
	THROW_PP(OpcObj.CheckCodeTemplate(temp_buf, sizeof(((BillTbl::Rec*)0)->Code)), PPERR_INVCODETEMPLATE)
	STRNSCPY(Data.Head.CodeTemplate, temp_buf);
	getCtrlData(CTL_OPKCOUNTER_COUNTER, &Data.Head.Counter);
	if(resourceID == DLG_OPCNTR) {
		// @v8.8.11 getCtrlData(CTL_OPKCOUNTER_ID, &Data.Head.ID);
		getCtrlData(sel = CTL_OPKCOUNTER_NAME, Data.Head.Name);
		THROW_PP(*strip(Data.Head.Name), PPERR_NAMENEEDED);
		getCtrlData(CTL_OPKCOUNTER_SYMB, Data.Head.Symb); // @v8.8.11
	}
	GetClusterData(CTL_OPKCOUNTER_FLAGS, &Data.Head.Flags);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI EditCounter(PPOpCounterPacket * pPack, uint resID, PPID * pOpcID)
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
		OpkPoolDialog(PPIDArray * pOpTypesList) : OpListDialog(DLG_OPRPOOL, CTL_OPRPOOL_LIST, 0, pOpTypesList)
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
			PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
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
int SLAPI PPObjOprKind::Browse(void * extraPtr)
{
	return ViewOprKind(0);
}

// virtual
int SLAPI PPObjOprKind::Edit(PPID * pID, void * extraPtr /*opTypeID*/)
{
	return Edit(pID, (PPID)extraPtr, 0);
}

// non-virtual
int SLAPI PPObjOprKind::Edit(PPID * pID, long opTypeID, PPID linkOpID)
{
	int    ok = 1, done = 0, is_new = 0;
	int    r = cmCancel;
	PPOprKindPacket pack;
	THROW(EditPrereq(pID, 0, &is_new));
	if(!is_new) {
		THROW(GetPacket(*pID, &pack) > 0);
	}
	else if(opTypeID > 0 || PPSelectObject(PPOBJ_OPRTYPE, &(opTypeID = 0), PPTXT_SELECTOPRTYPE, 0) > 0) {
		pack.Rec.OpTypeID = opTypeID;
		pack.Rec.LinkOpID = linkOpID;
	}
	else
		done = 1;
	if(!done) {
		for(int valid_data = 0; !valid_data && EditPacket(&pack) > 0;) {
			THROW(PutPacket(pID, &pack, 1));
			valid_data = 1;
			r = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok ? r : 0;
}

int SLAPI PPObjOprKind::EditPacket(PPOprKindPacket * pPack)
{
	int    ok = -1;
	uint   dlg_id = DLG_OPRKIND;
	OprKindDialog * dlg = 0;
	if(pPack->Rec.OpTypeID == PPOPT_WAREHOUSE)
		dlg_id = DLG_OPRWMS;
	else if(pPack->Rec.OpTypeID == PPOPT_INVENTORY)
		dlg_id = DLG_OPRINV;
	else if(pPack->Rec.OpTypeID == PPOPT_GENERIC) {
		dlg_id = DLG_GENOPRKIND;
		SETIFZ(pPack->P_GenList, new ObjRestrictArray);
	}
	else if(oneof3(pPack->Rec.OpTypeID, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT)) {
		dlg_id = DLG_OPRDRAFT;
	}
	else if(pPack->Rec.OpTypeID == PPOPT_ACCTURN) {
		if(pPack->Rec.ID == 0)
			pPack->Rec.Flags |= OPKF_EXTACCTURN;
	}
	if(CheckDialogPtr(&(dlg = new OprKindDialog(dlg_id, pPack)))) {
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

int SLAPI PPObjOprKind::AddBySample(PPID * pID, PPID sampleID)
{
	int    ok = cmCancel, valid_data = 0;
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

int SLAPI PPObjOprKind::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	if(msg == DBMSG_OBJDELETE) {
		if(_id && oneof4(_obj, PPOBJ_OPRTYPE, PPOBJ_ACCSHEET, PPOBJ_ACCOUNT2, PPOBJ_ARTICLE)) {
			PPOprKind op_rec;
			for(SEnum en = ref->Enum(Obj, 0); en.Next(&op_rec) > 0;) {
				if(_obj == PPOBJ_ACCSHEET && op_rec.AccSheetID == _id)
					return RetRefsExistsErr(Obj, op_rec.ID);
				else if(_obj == PPOBJ_OPRTYPE && op_rec.OpTypeID == _id)
					return RetRefsExistsErr(Obj, op_rec.ID);
				else if(oneof2(_obj, PPOBJ_ACCOUNT2, PPOBJ_ARTICLE)) {
					PPAccTurnTempl att;
					for(PPID prop = 0; ref->EnumProperties(Obj, op_rec.ID, &prop, &att, sizeof(att)) > 0 && prop <= PP_MAXATURNTEMPLATES;)
						if((_obj == PPOBJ_ACCOUNT2 && (att.DbtID.ac == _id || att.CrdID.ac == _id)) ||
							(_obj == PPOBJ_ARTICLE && (att.DbtID.ar == _id || att.CrdID.ar == _id)))
							return RetRefsExistsErr(Obj, op_rec.ID);
				}
			}
		}
	}
	return DBRPL_OK;
}

int SLAPI PPObjOprKind::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	return EditSpcRightFlags(DLG_RTOPRK, CTL_RTOPRK_FLAGS, CTL_RTOPRK_SFLAGS, bufSize, rt, pDlg);
}

IMPL_DESTROY_OBJ_PACK(PPObjOprKind, PPOprKindPacket);

int SLAPI PPObjOprKind::SerializePacket(int dir, PPOprKindPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	PPReckonOpEx roe;
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
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

int SLAPI PPObjOprKind::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPOprKindPacket);
	if(stream == 0) {
		THROW(GetPacket(id, (PPOprKindPacket*)p->Data) > 0);
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, (PPOprKindPacket *)p->Data, buffer, &pCtx->SCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		PPOprKindPacket * p_pack = (PPOprKindPacket*)p->Data;
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
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjOprKind::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		uint   i;
		PPID * p_id = 0;
		PPOprKindPacket * p_pack = (PPOprKindPacket*)p->Data;
		ProcessObjRefInArray(PPOBJ_OPRKIND,   &p_pack->Rec.LinkOpID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_OPRTYPE,   &p_pack->Rec.OpTypeID,    ary, replace);
		ProcessObjRefInArray(PPOBJ_ACCSHEET,  &p_pack->Rec.AccSheetID,  ary, replace);
		ProcessObjRefInArray(PPOBJ_ACCSHEET,  &p_pack->Rec.AccSheet2ID, ary, replace);
		ProcessObjRefInArray(PPOBJ_OPCOUNTER, &p_pack->Rec.OpCounterID, ary, replace);
		ProcessObjRefInArray(PPOBJ_LOCATION,  &p_pack->Rec.DefLocID,    ary, replace);
		for(i = 0; p_pack->Amounts.enumItems(&i, (void**)&p_id);)
			ProcessObjRefInArray(PPOBJ_AMOUNTTYPE, p_id, ary, replace);
		if(p_pack->P_ReckonData) {
			ProcessObjRefInArray(PPOBJ_PERSONRELTYPE, &p_pack->P_ReckonData->PersonRelTypeID, ary, replace);
			for(i = 0; p_pack->P_ReckonData->OpList.enumItems(&i, (void**)&p_id);)
				ProcessObjRefInArray(PPOBJ_OPRKIND, p_id, ary, replace);
		}
		return 1;
	}
	return -1;
}
//
//
//
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
	};
	SLAPI InvOpExCache() : ObjCache(PPOBJ_OPRKIND, sizeof(Data))
	{
	}
	virtual int  SLAPI FetchEntry(PPID id, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
};

int SLAPI InvOpExCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = (Data *)pEntry;
	PPInventoryOpEx rec;
	if(PPRef->GetProperty(PPOBJ_OPRKIND, id, OPKPRP_INVENTORY, &rec, sizeof(rec)) > 0) {
	   	p_cache_rec->WrDnOp  = rec.WrDnOp;
		p_cache_rec->WrDnObj = rec.WrDnObj;
	   	p_cache_rec->WrUpOp  = rec.WrUpOp;
		p_cache_rec->WrUpObj = rec.WrUpObj;
		p_cache_rec->AmountCalcMethod = rec.AmountCalcMethod;
		p_cache_rec->AutoFillMethod = rec.AutoFillMethod;
		p_cache_rec->Flags   = rec.Flags;
	}
	else
		ok = -1;
	return ok;
}

void SLAPI InvOpExCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPInventoryOpEx * p_data_rec = (PPInventoryOpEx *)pDataRec;
	const Data * p_cache_rec = (const Data *)pEntry;
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
	SLAPI  OpCache() : ObjCache(PPOBJ_OPRKIND, sizeof(OpCache::OpData)), P_ReckonOpList(0), State(0)
	{
	}
	SLAPI ~OpCache()
	{
		delete P_ReckonOpList;
	}
	virtual int FASTCALL Dirty(PPID); // @sync_w
	int    SLAPI GetReckonOpList(PPIDArray *); // @sync_rw
	int    SLAPI GetInventoryOpEx(PPID, PPInventoryOpEx *); // @>>IoeC.Get()
	PPID   FASTCALL GetBySymb(const char * pSymb);
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * entry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * entry, void * pDataRec) const;
	int    SLAPI FetchReckonOpList();

	int    IsReckonListInited;
	enum {
		stReckonListInited = 0x0001,
		stOpSymbListInited = 0x0002
	};
	long   State;
	InvOpExCache IoeC;
	PPIDArray * P_ReckonOpList;
	StrAssocArray OpSymbList;
};

int FASTCALL OpCache::Dirty(PPID opID)
{
	{
		//RwL.WriteLock();
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		Helper_Dirty(opID);
		if(P_ReckonOpList && P_ReckonOpList->lsearch(opID)) {
			State &= ~stReckonListInited;
			ZDELETE(P_ReckonOpList);
		}
		if(State & stOpSymbListInited) {
			OpSymbList.Clear();
			State &= ~stOpSymbListInited;
		}
		IoeC.Dirty(opID);
		//RwL.Unlock();
	}
	return 1;
}

PPID FASTCALL OpCache::GetBySymb(const char * pSymb)
{
	PPID   op_id = 0;
	{
		//RwL.ReadLock();
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(!(State & stOpSymbListInited)) {
			//RwL.Unlock();
			//RwL.WriteLock();
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(State & stOpSymbListInited)) {
				OpSymbList.Clear();
				PPOprKind op_rec;
				for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, Reference::eoIdSymb); en.Next(&op_rec) > 0;) {
					if(op_rec.Symb[0]) {
						OpSymbList.Add(op_rec.ID, strip(op_rec.Symb));
					}
				}
				State |= stOpSymbListInited;
			}
			//RwL.Unlock();
			//RwL.ReadLock();
			SRWLOCKER_TOGGLE(SReadWriteLocker::Read);
		}
		if(State & stOpSymbListInited) {
			uint pos = 0;
			if(OpSymbList.SearchByText(pSymb, 1, &pos)) {
				StrAssocArray::Item item = OpSymbList.at_WithoutParent(pos);
				op_id = item.Id;
			}
		}
		//RwL.Unlock();
	}
	return op_id;
}

int SLAPI OpCache::GetInventoryOpEx(PPID opID, PPInventoryOpEx * pInvOpEx)
{
	return IoeC.Get(opID, pInvOpEx);
}

int SLAPI OpCache::GetReckonOpList(PPIDArray * pList)
{
	int    ok = -1;
	if(pList) {
		pList->clear();
		//RwL.ReadLock();
		SRWLOCKER(RwL, SReadWriteLocker::Read);
		if(State & stReckonListInited) {
			if(P_ReckonOpList)
				pList->copy(*P_ReckonOpList);
			ok = 1;
		}
		if(ok < 0) {
			//RwL.Unlock();
			//RwL.WriteLock();
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			ok = FetchReckonOpList();
			if(State & stReckonListInited) {
				if(P_ReckonOpList)
					pList->copy(*P_ReckonOpList);
				ok = 1;
			}
		}
		//RwL.Unlock();
	}
	return ok;
}

int SLAPI OpCache::FetchReckonOpList()
{
	int    ok = 1;
	if(!(State & stReckonListInited)) {
		Reference * p_ref = PPRef;
		PPIDArray temp_list;
		PropertyTbl::Key0 k;
		BExtQuery q(&p_ref->Prop, 0);
		q.select(p_ref->Prop.ObjType, p_ref->Prop.ObjID, p_ref->Prop.Prop, 0L).
			where(p_ref->Prop.ObjType == PPOBJ_OPRKIND && p_ref->Prop.Prop == (long)OPKPRP_PAYMOPLIST);
		MEMSZERO(k);
		k.ObjType = PPOBJ_OPRKIND;
		for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;)
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

int SLAPI OpCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	OpData * p_cache_rec = (OpData *)pEntry;
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

void SLAPI OpCache::EntryToData(const ObjCacheEntry * pEntry, void * dataRec) const
{
	PPOprKind * p_data_rec = (PPOprKind*)dataRec;
	const OpData * p_cache_rec  = (const OpData *)pEntry;
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

int SLAPI FetchInvOpEx(PPID opID, PPInventoryOpEx * pInvOpEx)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	return p_cache ? p_cache->GetInventoryOpEx(opID, pInvOpEx) : 0;
}

int FASTCALL GetReckonOpList(PPIDArray * pList)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	return p_cache ? p_cache->GetReckonOpList(pList) : 0;
}

int FASTCALL GetOpData(PPID op, PPOprKind * pData)
{
	OpCache * p_cache = GetDbLocalCachePtr <OpCache> (PPOBJ_OPRKIND);
	int    r = p_cache ? p_cache->Get(op, pData) : 0;
	if(r < 0)
		r = 0;
	return r;
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
	PPOprKind op_rec;
	return (opID && GetOpData(opID, &op_rec)) ? ((rBuf = op_rec.Name), 1) : (rBuf.Z(), 0);
}

int FASTCALL GetOpName(PPID op, char * buf, size_t buflen)
{
	PPOprKind opk;
	if(op && GetOpData(op, &opk)) {
		strnzcpy(buf, opk.Name, buflen);
		return 1;
	}
	else {
		ASSIGN_PTR(buf, 0);
		return 0;
	}
}

int FASTCALL CheckOpFlags(PPID op, long andF, long notF)
{
	PPOprKind opk;
	if(op && GetOpData(op, &opk))
		if(andF && (opk.Flags & andF) != andF)
			return 0;
		else if(notF && (opk.Flags & notF) == notF)
			return 0;
		else
			return 1;
	else
		return 0;
}

int FASTCALL CheckOpPrnFlags(PPID op, long andF)
{
	PPOprKind opk;
	if(op && GetOpData(op, &opk))
		return (andF && (opk.PrnFlags & andF) != andF) ? 0 : 1;
	else
		return 0;
}

int SLAPI EnumOperations(PPID opTypeID, PPID * pID, PPOprKind * pOpData)
{
	int    r;
	PPOprKind rec;
	while((r = PPRef->EnumItems(PPOBJ_OPRKIND, pID, &rec)) > 0)
		if(!opTypeID || rec.OpTypeID == opTypeID) {
			if(pOpData)
				memcpy(pOpData, &rec, sizeof(PPOprKind));
			return 1;
		}
	return r ? -1 : 0;
}

PPID FASTCALL GetOpType(PPID opID, PPOprKind * pOpData)
{
	PPID   op_type_id = 0;
	PPOprKind rec;
	if(opID && GetOpData(opID, &rec))
		op_type_id = rec.OpTypeID;
	else
		MEMSZERO(rec);
	ASSIGN_PTR(pOpData, rec);
	return op_type_id;
}

int FASTCALL GetOpSubType(PPID opID)
{
	PPOprKind rec;
	return GetOpData(opID, &rec) ? rec.SubType : 0;
}

PPID FASTCALL IsOpPaymOrRetn(PPID opID)
{
	if(opID) {
		PPOprKind op_rec;
		const PPID t = GetOpType(opID, &op_rec);
		return (oneof2(t, PPOPT_PAYMENT, PPOPT_GOODSRETURN) || (t == PPOPT_CHARGE && op_rec.Flags & OPKF_CHARGENEGPAYM)) ? t : 0;
	}
	else
		return 0;
}

int FASTCALL IsOpPaym(PPID opID)
{
	if(opID) {
		PPOprKind op_rec;
		PPID   t = GetOpType(opID, &op_rec);
		return ((t == PPOPT_PAYMENT) || (t == PPOPT_CHARGE && op_rec.Flags & OPKF_CHARGENEGPAYM));
	}
	else
		return 0;
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
	// @v7.5.3 {
	else
		return -1;
	// } @v7.5.3
	// @v7.5.3 return 0;
}

int FASTCALL IsIntrOp(PPID opID)
{
	int    r = 0;
	PPOprKind opk;
	if(opID == PPOPK_INTREXPEND)
		r = INTREXPND;
	else if(opID == PPOPK_INTRRECEIPT)
		r = INTRRCPT;
	else if(opID && GetOpData(opID, &opk))
		if(opk.AccSheetID == LConfig.LocAccSheetID)
			if(opk.OpTypeID == PPOPT_GOODSEXPEND)
				r = INTREXPND;
			else if(opk.OpTypeID == PPOPT_GOODSRECEIPT)
				r = INTRRCPT;
	return r;
}

int FASTCALL IsIntrExpndOp(PPID opID)
{
	return BIN(IsIntrOp(opID) == INTREXPND);
}

int FASTCALL IsDraftOp(PPID opID)
{
	const PPID op_type_id = GetOpType(opID);
	return BIN(oneof3(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT));
}

int FASTCALL IsGoodsDetailOp(PPID opID)
{
	const PPID op_type_id = GetOpType(opID);
	return BIN(oneof11(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTTRANSIT, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
		PPOPT_GOODSREVAL, PPOPT_CORRECTION, PPOPT_GOODSACK, PPOPT_GOODSMODIF, PPOPT_GOODSRETURN, PPOPT_GOODSORDER));
}

int FASTCALL IsSellingOp(PPID opID)
{
	int    ret = -1;
	if(opID) {
		PPOprKind op_rec;
		GetOpData(opID, &op_rec);
		long   s = (op_rec.Flags & OPKF_SELLING);
		ret = (s ^ (op_rec.Flags & OPKF_BUYING)) ? BIN(s) : _IsSellingOp(opID);
	}
	return ret;
}

int FASTCALL IsExpendOp(PPID opID)
{
	int    ret = -1;
	if(opID) {
		PPOprKind opk;
		const PPID t = GetOpType(opID, &opk);
		if(t == PPOPT_GOODSEXPEND)
			ret = 1;
		else if(oneof2(t, PPOPT_GOODSRECEIPT, PPOPT_GOODSORDER))
			ret = 0;
		else if(t == PPOPT_GOODSRETURN) {
			const int r = IsExpendOp(opk.LinkOpID);
			ret = (r >= 0) ? (r ? 0 : 1) : -1;
		}
		else if(t == PPOPT_GOODSACK)
			ret = IsExpendOp(opk.LinkOpID);
	}
	return ret;
}

int FASTCALL IsGenericOp(PPID opID)
{
	return opID ? BIN(GetOpType(opID) == PPOPT_GENERIC) : -1;
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

int FASTCALL IsOpBelongTo(PPID testOpID, PPID anotherOpID)
{
	int    ok = 0;
	if(anotherOpID > 0) {
		if(testOpID == anotherOpID)
			ok = 1;
		else {
			PPIDArray op_list;
			if(GetGenericOpList(anotherOpID, &op_list) > 0 && op_list.lsearch(testOpID))
				ok = 1;
		}
	}
	return ok;
}

int SLAPI GetOpCommonAccSheet(PPID opID, PPID * pAccSheetID, PPID * pAccSheet2ID)
{
	int    ok = -1;
	PPID   acc_sheet_id = 0, acc_sheet2_id = 0;
	PPOprKind opk;
	if(GetOpData(opID, &opk)) {
		if(opk.AccSheet2ID)
			acc_sheet2_id = opk.AccSheet2ID;
		if(opk.AccSheetID || GetOpData(opk.LinkOpID, &opk) > 0)
			acc_sheet_id = opk.AccSheetID;
		else if(opk.OpTypeID == PPOPT_GENERIC) {
		   	PPIDArray op_list;
			GetGenericOpList(opID, &op_list);
			for(uint i = 0; i < op_list.getCount(); i++) {
				PPID tmp_acc_sheet_id = 0, tmp_acc_sheet2_id = 0;
				GetOpCommonAccSheet(op_list.at(i), &tmp_acc_sheet_id, &tmp_acc_sheet2_id); // @recursion
				if(acc_sheet2_id >= 0)
					if(acc_sheet2_id == 0)
						acc_sheet2_id = tmp_acc_sheet2_id;
					else if(tmp_acc_sheet2_id && tmp_acc_sheet2_id != acc_sheet2_id)
						acc_sheet2_id = -1;
				if(acc_sheet_id >= 0)
					if(acc_sheet_id == 0)
						acc_sheet_id = tmp_acc_sheet_id;
					else if(tmp_acc_sheet_id && tmp_acc_sheet_id != acc_sheet_id)
						acc_sheet_id = -1;
			}
		}
	}
	if(acc_sheet_id > 0)
		ok = 1;
	ASSIGN_PTR(pAccSheetID,  (acc_sheet_id > 0  ? acc_sheet_id  : 0L));
	ASSIGN_PTR(pAccSheet2ID, (acc_sheet2_id > 0 ? acc_sheet2_id : 0L));
	return ok;
}

PPID SLAPI GetCashOp()
{
	const PPCommConfig & r_ccfg = CConfig;
	return NZOR(r_ccfg.RetailOp, PPOPK_RETAIL);
}

PPID SLAPI GetCashRetOp()
{
	const  PPCommConfig & r_ccfg = CConfig;
	PPID   op_id_ = -1;
	if(r_ccfg.RetailRetOp)
		op_id_ = r_ccfg.RetailRetOp;
	else {
		const PPID cash_op_id = GetCashOp();
		PPOprKind opk;
		for(PPID iter_op_id = 0; op_id_ < 0 && EnumOperations(PPOPT_GOODSRETURN, &iter_op_id, &opk) > 0;)
			if(opk.LinkOpID == cash_op_id)
				op_id_ = iter_op_id;
	}
	return op_id_;
}

PPID SLAPI GetReceiptOp()
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
			H.OpType = rec.OpTypeID; // @v8.7.7
			H.Flags = rec.Flags;
			H.fNeedPayment = BIN(rec.Flags & OPKF_NEEDPAYMENT);
			H.fProfitable  = BIN(rec.Flags & OPKF_PROFITABLE);
			H.fReckon      = BIN(rec.Flags & OPKF_RECKON);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

