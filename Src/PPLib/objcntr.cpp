// OBJCNTR.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2010, 2013, 2014, 2015, 2016, 2017, 2019
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPObjOpCounter)
//
PPOpCounterPacket::PPOpCounterPacket() : P_Items(0)
{
	Init(0, 0);
	//DontLogUpdAction = 0;
	Flags = 0;
}

PPOpCounterPacket::~PPOpCounterPacket()
{
	ZDELETE(P_Items);
}

int SLAPI PPOpCounterPacket::Init(const PPOpCounter * pHead, const LAssocArray * pItems)
{
    int    ok = 1;
	if(!RVALUEPTR(Head, pHead))
		MEMSZERO(Head);
	if(*strip(Head.CodeTemplate) == 0) {
		Head.CodeTemplate[0] = '%';
		Head.CodeTemplate[1] = '0';
		Head.CodeTemplate[2] = '5';
	}
	THROW(Init(pItems));
	CATCHZOK
	return ok;
}

int SLAPI PPOpCounterPacket::Init(const LAssocArray * pItems)
{
	int    ok = 1;
	ZDELETE(P_Items);
	if(Head.Flags & OPCNTF_DIFFBYLOC) {
		P_Items = new LAssocArray();
		if(pItems)
			P_Items->copy(*pItems);
		if(P_Items->getCount() == 0) {
			PPObjLocation loc_obj;
			PPIDArray wh_list;
			loc_obj.GetWarehouseList(&wh_list);
			for(uint i = 0; i < wh_list.getCount(); i++)
				THROW_SL(P_Items->AddUnique(wh_list.at(i), 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPOpCounterPacket::GetCounter(PPID locID, long * pCounter)
{
	if(!P_Items || !(Head.Flags & OPCNTF_DIFFBYLOC) || !P_Items->Search(locID, pCounter, 0))
		ASSIGN_PTR(pCounter, Head.Counter);
	return 1;
}

int SLAPI PPOpCounterPacket::SetCounter(PPID locID, long counter)
{
	int    r = -1;
	counter = (counter < 0) ? 0 : counter;
	if(P_Items && locID && Head.Flags & OPCNTF_DIFFBYLOC) {
		if(P_Items->Search(locID, 0, 0))
			r = P_Items->Update(locID, counter);
		else
			r = P_Items->AddUnique(locID, counter, 0);
	}
	else {
		Head.Counter = counter;
		r = 1;
	}
	return r;
}

int SLAPI PPOpCounterPacket::CounterIncr(PPID locID, long * pCounter, int incr)
{
	long   counter = 0;
	GetCounter(locID, &counter);
	if(!(Head.Flags & OPCNTF_LOCKINCR)) {
		if(incr)
			counter++;
		else
			counter--;
	}
	ASSIGN_PTR(pCounter, counter);
	return SetCounter(locID, counter);
}

int SLAPI PPOpCounterPacket::ResetAll()
{
	int    ok = 1;
	Head.Counter = 0L;
	if(P_Items)
		for(uint i = 0; i < P_Items->getCount(); i++)
			THROW(P_Items->Update(P_Items->at(i).Key, 0L));
	CATCHZOK
	return ok;
}

int SLAPI PPOpCounterPacket::UngetCounter(PPID locID, long counter)
{
	int    r = -1;
	long   cntr = 0;
	GetCounter(locID, &cntr);
	if(!(Head.Flags & OPCNTF_LOCKINCR))
		if(--cntr == counter)
			r = SetCounter(locID, cntr);
	return r;
}

PPOpCounterPacket & FASTCALL PPOpCounterPacket::operator = (const PPOpCounterPacket &aPack)
{
	Init(&aPack.Head, aPack.P_Items);
	return *this;
}

static int SLAPI OpCounterListFilt(void * rec, void * extraPtr)
{
	return (((PPOpCounter*)rec)->OwnerObjID == 0);
}

SLAPI PPObjOpCounter::PPObjOpCounter(void * extraPtr) : PPObjReference(PPOBJ_OPCOUNTER, extraPtr)
{
	FiltProc = OpCounterListFilt;
}

int SLAPI PPObjOpCounter::Edit(PPID * pID, void * extraPtr)
{
	int    ok = -1, valid_data = 0, r = 0;
	PPOpCounterPacket pack;
	THROW(CheckRightsModByID(pID));
	if(*pID)
		THROW(GetPacket(*pID, &pack) > 0);
	THROW(r = EditCounter(&pack, DLG_OPCNTR));
	if(r > 0) {
		THROW(PutPacket(pID, &pack, 1));
		r = cmOK;
		ok = 1;
	}
	else
		r = cmCancel;
	CATCHZOKPPERR
	return ok ? r : 0;
}

int SLAPI PPObjOpCounter::CodeByTemplate(const char * pTempl, long counter, SString & rBuf)
{
	int    wasnumber = 0;
	char   temp_buf[64];
	char * d = temp_buf;
	for(const char * p = pTempl; *p;) {
		if(*p == '%' && !wasnumber) {
			char fmt[16];
			char * n = fmt;
			*n++ = *p++;
			if(*p == '-')
				*n++ = *p++;
			while(isdec(*p))
				*n++ = *p++;
			*n++ = 'l';
			*n++ = 'd';
			*n   = 0;
			sprintf(d, fmt, counter + 1);
			d += sstrlen(d);
			wasnumber = 1;
		}
		else
			*d++ = *p++;
	}
	*d = 0;
	rBuf = temp_buf;
	return 1;
}

int SLAPI PPObjOpCounter::CheckCodeTemplate(const char * pTempl, size_t bufLen)
{
	SString temp_buf;
	CodeByTemplate(pTempl, 9999L, temp_buf);
	return (temp_buf.Len() <= (bufLen - 1));
}

int SLAPI PPObjOpCounter::UpdateCounter(PPID id, long counter, long flags, PPID locID, int use_ta)
{
	int    ok = 1;
	if(id) {
		long   prev_val = 0;
		PPOpCounterPacket pack;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(GetPacket(id, &pack) > 0);
			prev_val = pack.Head.Counter;
			THROW(pack.SetCounter(locID, counter));
			pack.Head.Flags = flags;
			THROW(PutPacket(&id, &pack, 0));
			DS.LogAction(PPACN_OPCNTRUPD, Obj, id, counter-prev_val, 0);
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPObjOpCounter::Helper_GetCounter(PPID id, PPID locID, long * pCounter, SString * pCodeBuf, int use_ta)
{
	int    ok = -1;
	long   counter = 0;
	if(id) {
		PPOpCounterPacket pack;
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(GetPacket(id, &pack) > 0);
		pack.GetCounter(locID, &counter);
		THROW(pack.CounterIncr(locID, 0));
		pack.Flags |= PPOpCounterPacket::fDontLogUpdAction;
		THROW(PutPacket(&id, &pack, 0));
		THROW(tra.Commit());
		if(pCodeBuf) {
			CodeByTemplate(pack.Head.CodeTemplate, counter, *pCodeBuf);
		}
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pCounter, counter);
	return ok;
}

int SLAPI PPObjOpCounter::GetCounter(PPID id, PPID locID, long * pCounter, int use_ta)
{
	return Helper_GetCounter(id, locID, pCounter, 0, use_ta);
}

int SLAPI PPObjOpCounter::GetCode(PPID id, long * pCounter, char * pBuf, size_t bufLen, PPID locID, int use_ta)
{
	SString temp_buf;
	int    ok = Helper_GetCounter(id, locID, pCounter, &temp_buf, use_ta);
	temp_buf.CopyTo(pBuf, bufLen);
	return ok;
}

int SLAPI PPObjOpCounter::UngetCounter(PPID id, long counter, PPID locID, int use_ta)
{
	int    ok = 1, r = 0;
	if(id) {
		PPOpCounterPacket pack;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(GetPacket(id, &pack) > 0);
			THROW(r = pack.UngetCounter(locID, counter))
			if(r > 0) {
				pack.Flags |= PPOpCounterPacket::fDontLogUpdAction;
				THROW(PutPacket(&id, &pack, 0));
			}
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjOpCounter::ResetAll()
{
	int    ok = 1;
	if(PPMessage(mfConf|mfYesNo, PPCFM_RESETOPCNTRS) == cmYes) {
		PPID   id = 0;
		PPObjOpCounter opc_obj;
		PPOpCounterPacket pack;
		{
			PPTransaction tra(1);
			THROW(tra);
			while(opc_obj.EnumItems(&id, 0) > 0) {
				THROW(opc_obj.GetPacket(id, &pack) > 0);
				if(pack.Head.ObjType == 0 || pack.Head.ObjType == PPOBJ_OPRKIND) {
					pack.ResetAll();
					THROW(opc_obj.PutPacket(&id, &pack, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOpCounter::GetPacket(PPID opCntrID, PPOpCounterPacket * pPack)
{
	int    ok = -1, r = 0;
	if(pPack) {
		LAssocArray cntrs_ary;
		PPOpCounter opc_rec;
		MEMSZERO(opc_rec);
		THROW(r = ref->GetItem(PPOBJ_OPCOUNTER, opCntrID, &opc_rec));
		if(r > 0) {
			if(opc_rec.Flags & OPCNTF_DIFFBYLOC)
				THROW(ref->GetPropArray(PPOBJ_OPRKIND, opCntrID, OPKPRP_DIFFCNTRS, &cntrs_ary));
			THROW(pPack->Init(&opc_rec, &cntrs_ary));
			ok = 1;
		}
		else {
			pPack->Init(0, 0);
			memzero(pPack->Head.CodeTemplate, sizeof(pPack->Head.CodeTemplate));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOpCounter::PutPacket(PPID * pOpCntrID, const PPOpCounterPacket * pPack, int use_ta)
{
	int    ok = -1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			PPOpCounter opc_rec;
			if(ref->GetItem(PPOBJ_OPCOUNTER, *pOpCntrID, &opc_rec) > 0) {
				int    log_action = (pPack->Flags & PPOpCounterPacket::fDontLogUpdAction) ? 0 : 1;
				long   prev_cntr = opc_rec.Counter;
				opc_rec = pPack->Head;
				if(pPack->Flags & PPOpCounterPacket::fDontUpdCounter)
					opc_rec.Counter = prev_cntr;
	   			THROW(ref->UpdateItem(PPOBJ_OPCOUNTER, *pOpCntrID, &opc_rec, log_action, 0));
			}
			else {
				opc_rec = pPack->Head;
				THROW(ref->AddItem(PPOBJ_OPCOUNTER, pOpCntrID, &opc_rec, 0));
			}
			if(opc_rec.Flags & OPCNTF_DIFFBYLOC) {
				THROW(ref->PutPropArray(PPOBJ_OPRKIND, *pOpCntrID, OPKPRP_DIFFCNTRS, pPack->P_Items, 0));
			}
			/* @v8.2.9 else
				THROW(ref->PutPropArray(PPOBJ_OPRKIND, *pOpCntrID, OPKPRP_DIFFCNTRS, 0, 0)); */
			ok = 1;
		}
		else if(*pOpCntrID) {
			THROW(ref->RemoveItem(Obj, *pOpCntrID, use_ta) && RemoveSync(*pOpCntrID));
			THROW(ref->PutPropArray(PPOBJ_OPRKIND, *pOpCntrID, OPKPRP_DIFFCNTRS, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjOpCounter::MakeReserved(long flags)
{
	// {ID, Name, Symb, CodeTemplate }
	int    ok = 1;
	uint   num_recs, i;
	SString name, symb, templ;
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	THROW_PP(p_rez->findResource(ROD_OPCOUNTER, PP_RCDATA), PPERR_RESFAULT);
	THROW_PP(num_recs = p_rez->getUINT(), PPERR_RESFAULT);
	for(i = 0; i < num_recs; i++) {
		PPOpCounterPacket pack;
		PPOpCounter temp_rec;
		PPID   id = p_rez->getUINT();
		p_rez->getString(name, 2);
		p_rez->getString(symb, 2);
		p_rez->getString(templ, 2);
		pack.Head.ID = id;
		name.CopyTo(pack.Head.Name, sizeof(pack.Head.Name));
		symb.CopyTo(pack.Head.Symb, sizeof(pack.Head.Symb));
		templ.CopyTo(pack.Head.CodeTemplate, sizeof(pack.Head.CodeTemplate));
		if(id && Search(id, &temp_rec) <= 0 && SearchBySymb(symb, 0, &temp_rec) <= 0) {
			//
			// «десь нельз€ использовать PutPacket поскольку добавл€етс€ запись
			// с предопределенным идентификатором.
			//
			THROW(EditItem(Obj, 0, &pack.Head, 1));
		}
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_OpCounter
//
PPALDD_CONSTRUCTOR(OpCounter) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(OpCounter) { Destroy(); }

int PPALDD_OpCounter::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPOpCounter rec;
		if(SearchObject(PPOBJ_OPCOUNTER, rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			STRNSCPY(H.CodeTemplate, rec.CodeTemplate);
			H.ObjType = rec.ObjType;
			H.Flags = rec.Flags;
			H.Counter = rec.Counter;
			H.OwnerObjID = rec.OwnerObjID;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
