// OBJREGT.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2009, 2010, 2012, 2013, 2014, 2016, 2018, 2019, 2020
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// @ModuleDef(PPObjRegisterType)
//
SLAPI PPRegisterTypePacket::PPRegisterTypePacket()
{
	MEMSZERO(Rec);
	CntrPack.Head.ObjType = PPOBJ_REGISTERTYPE;
	memzero(CntrPack.Head.CodeTemplate, sizeof(CntrPack.Head.CodeTemplate));
}

PPRegisterTypePacket & FASTCALL PPRegisterTypePacket::operator = (const PPRegisterTypePacket & s)
{
	Rec = s.Rec;
	Format = s.Format;
	CntrPack = s.CntrPack;
	return *this;
}
//
//
//
int FASTCALL GetRegisterTypeName(PPID rtID, SString & rBuf)
{
	int    ok = -1;
	PPObjRegisterType rt_obj;
	PPRegisterType rt_rec;
	if(rt_obj.Fetch(rtID, &rt_rec) > 0) {
		rBuf = rt_rec.Name;
		ok = 1;
	}
	else
		ideqvalstr(rtID, rBuf);
	return ok;
}

// static
int SLAPI PPObjRegisterType::IsDupRegType(PPID regTypeID)
{
	PPObjRegisterType rt_obj;
	PPRegisterType rt_rec;
	return BIN(rt_obj.Fetch(regTypeID, &rt_rec) > 0 && rt_rec.Flags & REGTF_DUPNUMBER);
}

// static
int FASTCALL PPObjRegisterType::GetByCode(const char * pRegTypeCode, PPID * pRegTypeID)
{
	int    ok = -1;
	PPID   reg_type_id = 0;
	if(pRegTypeCode)
		if(PPRef->SearchSymb(PPOBJ_REGISTERTYPE, &reg_type_id, pRegTypeCode, offsetof(PPRegisterType, Symb)) > 0)
			ok = 1;
		else
			reg_type_id = 0;
	ASSIGN_PTR(pRegTypeID, reg_type_id);
	return ok;
}

SLAPI PPObjRegisterType::PPObjRegisterType() : PPObjReference(PPOBJ_REGISTERTYPE, 0)
{
}

int SLAPI PPObjRegisterType::GetCode(PPID id, long * pCounter, char * pBuf, size_t bufLen)
{
	int    ok = -1;
	PPRegisterType rt_rec;
	ASSIGN_PTR(pBuf, 0);
	ASSIGN_PTR(pCounter, 0);
	if(Search(id, &rt_rec) > 0 && rt_rec.CounterID) {
		PPObjOpCounter opc_obj;
		ok = opc_obj.GetCode(rt_rec.CounterID, pCounter, pBuf, bufLen, 0, 1);
	}
	return ok;
}

int SLAPI PPObjRegisterType::UngetCounter(PPID id, long counter)
{
	int    ok = -1;
	PPRegisterType rt_rec;
	if(id && counter && Search(id, &rt_rec) > 0 && rt_rec.CounterID) {
		PPObjOpCounter opc_obj;
		ok = opc_obj.UngetCounter(rt_rec.CounterID, counter, 0, 1);
	}
	return ok;
}

int SLAPI PPObjRegisterType::PutPacket(PPID * pID, PPRegisterTypePacket * pPack, int use_ta)
{
	int    ok = 1;
	PPObjOpCounter opc_obj;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack) {
			if(*pID) {
				if(pPack->CntrPack.Head.CodeTemplate[0]) {
					pPack->CntrPack.Head.ObjType = Obj;
					pPack->CntrPack.Head.OwnerObjID = *pID;
					THROW(opc_obj.PutPacket(&pPack->Rec.CounterID, &pPack->CntrPack, 0));
				}
				else {
					THROW(opc_obj.PutPacket(&pPack->Rec.CounterID, 0, 0));
					pPack->Rec.CounterID = 0;
				}
				THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(ref->AddItem(Obj, pID, &pPack->Rec, 0));
				pPack->Rec.ID = *pID;
				if(pPack->CntrPack.Head.CodeTemplate[0]) {
					pPack->CntrPack.Head.ObjType = Obj;
					pPack->CntrPack.Head.OwnerObjID = *pID;
					THROW(opc_obj.PutPacket(&pPack->Rec.CounterID, &pPack->CntrPack, 0));
					//
					// Для того, чтобы запись типа регистрационного документа знала идентификатор
					// счетчика, обновляем ее
					//
					THROW(ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
				}
			}
			THROW(PutFormat(*pID, pPack->Format.Strip()));
		}
		else if(*pID) {
			PPRegisterType rt_rec;
			THROW(Search(*pID, &rt_rec) > 0);
			THROW(opc_obj.PutPacket(&rt_rec.CounterID, 0, 0));
			THROW(ref->RemoveItem(Obj, *pID, 0));
		}
		THROW(tra.Commit());
	}
	if(*pID)
		Dirty(*pID);
	CATCHZOK
	return ok;
}

int SLAPI PPObjRegisterType::GetPacket(PPID id, PPRegisterTypePacket * pPack)
{
	int    ok = 1;
	if(Search(id, &pPack->Rec) > 0) {
		PPObjOpCounter opc_obj;
		THROW(opc_obj.GetPacket(pPack->Rec.CounterID, &pPack->CntrPack));
		THROW(GetFormat(id, pPack->Format));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

class RegisterTypeDialog : public TDialog {
	DECL_DIALOG_DATA(PPRegisterTypePacket);
public:
	RegisterTypeDialog() : TDialog(DLG_PREGTYP)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		long   st = 0;
		RVALUEPTR(Data, pData);
		setCtrlData(CTL_PREGTYP_NAME, Data.Rec.Name);
		setCtrlData(CTL_PREGTYP_ID,   &Data.Rec.ID);
		setCtrlData(CTL_PREGTYP_SYMB, Data.Rec.Symb);
		setCtrlData(CTL_PREGTYP_CNTRTEMPL, Data.CntrPack.Head.CodeTemplate);
		setCtrlData(CTL_PREGTYP_CNTR,      &Data.CntrPack.Head.Counter);
		setCtrlData(CTL_PREGTYP_EXPIRYPERIOD, &Data.Rec.ExpiryPeriod);
		setCtrlString(CTL_PREGTYP_FORMAT, Data.Format);
		disableCtrl(CTL_PREGTYP_ID, (int)Data.Rec.ID || !PPMaster);
		SetupPPObjCombo(this, CTLSEL_PREGTYP_REGPKIND, PPOBJ_PRSNKIND, Data.Rec.RegOrgKind, OLW_CANINSERT, 0);
		SetupPPObjCombo(this, CTLSEL_PREGTYP_PKIND, PPOBJ_PRSNKIND, Data.Rec.PersonKindID, 0, 0);

		AddClusterAssoc(CTL_PREGTYP_ST, 0, REGTF_PRIVATE);
		AddClusterAssoc(CTL_PREGTYP_ST, 1, REGTF_LEGAL);
		AddClusterAssoc(CTL_PREGTYP_ST, 2, REGTF_LOCATION);
		SetClusterData(CTL_PREGTYP_ST, Data.Rec.Flags);

		AddClusterAssoc(CTL_PREGTYP_FLAGS, 0, REGTF_UNIQUE);
		AddClusterAssoc(CTL_PREGTYP_FLAGS, 1, REGTF_INSERT);
		AddClusterAssoc(CTL_PREGTYP_FLAGS, 2, REGTF_WARNEXPIRY);
		AddClusterAssoc(CTL_PREGTYP_FLAGS, 3, REGTF_WARNABSENCE);
		AddClusterAssoc(CTL_PREGTYP_FLAGS, 4, REGTF_DUPNUMBER);
		AddClusterAssoc(CTL_PREGTYP_FLAGS, 5, REGTF_ONLYNUMBER);
		SetClusterData(CTL_PREGTYP_FLAGS, Data.Rec.Flags);

		SetupPPObjCombo(this, CTLSEL_PREGTYP_GGRP, PPOBJ_GOODSGROUP, Data.Rec.RestrictGoodsGrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN, 0);
		AddClusterAssocDef(CTL_PREGTYP_GGRPALLOW, 0, PPRegisterType::ggrpaOnlyGroup);
		AddClusterAssoc(CTL_PREGTYP_GGRPALLOW,  1, PPRegisterType::ggrpaDenyGroup);
		AddClusterAssoc(CTL_PREGTYP_GGRPALLOW,  2, PPRegisterType::ggrpaAllowByReg);
		SetClusterData(CTL_PREGTYP_GGRPALLOW, (long)Data.Rec.RestrictGoodsKind);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		int    err = 0, sel = 0;
		long   st = 0, ggrp_allow = 0;
		getCtrlData(sel = CTL_PREGTYP_NAME, Data.Rec.Name);
		THROW_PP(*strip(Data.Rec.Name) != 0, PPERR_NAMENEEDED);
		getCtrlData(sel = CTL_PREGTYP_SYMB, Data.Rec.Symb);
		THROW(PPRef->CheckUniqueSymb(PPOBJ_REGISTERTYPE, Data.Rec.ID, Data.Rec.Symb, offsetof(PPRegisterType, Symb)));
		getCtrlData(CTL_PREGTYP_ID,   &Data.Rec.ID);
		getCtrlData(sel = CTLSEL_PREGTYP_REGPKIND, &Data.Rec.RegOrgKind);
		THROW_PP(Data.Rec.RegOrgKind != 0, PPERR_REGPKINDNEEDED);
		getCtrlData(CTLSEL_PREGTYP_PKIND,  &Data.Rec.PersonKindID);
		GetClusterData(CTL_PREGTYP_ST,     &Data.Rec.Flags);
		GetClusterData(CTL_PREGTYP_FLAGS,  &Data.Rec.Flags);
		getCtrlData(CTL_PREGTYP_CNTRTEMPL, Data.CntrPack.Head.CodeTemplate);
		getCtrlData(CTL_PREGTYP_CNTR,      &Data.CntrPack.Head.Counter);
		getCtrlData(CTL_PREGTYP_EXPIRYPERIOD, &Data.Rec.ExpiryPeriod);
		getCtrlString(CTL_PREGTYP_FORMAT,  Data.Format);
		Data.Format.Strip();
		getCtrlData(CTLSEL_PREGTYP_GGRP,      &Data.Rec.RestrictGoodsGrpID);
		GetClusterData(CTL_PREGTYP_GGRPALLOW, &ggrp_allow);
		Data.Rec.RestrictGoodsKind = (int16)ggrp_allow;
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = (selectCtrl(sel), 0);
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_PREGTYP_GGRP)) {
			SetupCtrls();
			clearEvent(event);
		}
	}
	void   SetupCtrls()
	{
		const PPID grp_id = getCtrlLong(CTLSEL_PREGTYP_GGRP);
		disableCtrl(CTL_PREGTYP_GGRPALLOW, BIN(!grp_id));
	}
};

int SLAPI PPObjRegisterType::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1;
	int    r = cmCancel;
	int    valid_data = 0;
	int    is_new = 0;
	ushort flags = 0;
	PPRegisterTypePacket pack;
	RegisterTypeDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new RegisterTypeDialog())));
	THROW(EditPrereq(pID, p_dlg, &is_new));
	if(!is_new)
		THROW(GetPacket(*pID, &pack) > 0);
	p_dlg->setDTS(&pack);
	while(!valid_data && (r = ExecView(p_dlg)) == cmOK) {
		if(!p_dlg->getDTS(&pack) || !PutPacket(pID, &pack, 1))
			PPError();
		else
			valid_data = 1;
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok ? r : 0;
}

int SLAPI PPObjRegisterType::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE && _obj == PPOBJ_PRSNKIND) {
		int    r;
		PPRegisterType rec;
		for(PPID id = 0; ok == DBRPL_OK && (r = ref->EnumItems(Obj, &id, &rec)) > 0;)
			if(rec.RegOrgKind == _id)
				ok = RetRefsExistsErr(Obj, id);
		if(ok == DBRPL_OK && !r)
			ok = DBRPL_ERROR;
	}
	return ok;
}

int SLAPI PPObjRegisterType::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRREGISTERTYPE, 0); }
int SLAPI PPObjRegisterType::GetFormat(PPID id, SString & rBuf) { return ref->GetPropVlrString(PPOBJ_REGISTERTYPE, id, REGTPRP_FORMAT, rBuf); }
int SLAPI PPObjRegisterType::PutFormat(PPID id, const char * pBuf) { return ref->PutPropVlrString(PPOBJ_REGISTERTYPE, id, REGTPRP_FORMAT, pBuf); }
int SLAPI PPObjRegisterType::SearchSymb(PPID * pID, const char * pSymb) { return ref->SearchSymb(Obj, pID, pSymb, offsetof(PPRegisterType, Symb)); }

int SLAPI PPObjRegisterType::ProcessReservedItem(TVRez & rez)
{
	int    ok = 1, r;
	SString name;
	char   flags[128];
	PPID   id = (PPID)rez.getUINT();
	rez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER); // @v9.4.5
	rez.getString(flags);
	THROW(r = Search(id));
	if(r < 0) {
		PPRegisterType rec;
		MEMSZERO(rec);
		rec.ID = id;
		STRNSCPY(rec.Name, name);
		rec.RegOrgKind = PPPRK_BUSADMIN;
		for(int i = 0; flags[i]; i++) {
			switch(toupper(flags[i])) {
				case 'U': rec.Flags |= REGTF_UNIQUE;     break;
				case 'P': rec.Flags |= REGTF_PRIVATE;    break;
				case 'L': rec.Flags |= REGTF_LEGAL;      break;
				case 'A': rec.Flags |= REGTF_LOCATION;   break; // @v8.3.6
				case 'D': rec.Flags |= REGTF_DUPNUMBER;  break; // @v9.0.4
				case 'N': rec.Flags |= REGTF_ONLYNUMBER; break; // @v9.0.4
			}
		}
		THROW(EditItem(Obj, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}
//
//
//
class RegisterTypeCache : public ObjCache {
public:
	SLAPI  RegisterTypeCache() : ObjCache(PPOBJ_REGISTERTYPE, sizeof(RegisterTypeData)) {}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct RegisterTypeData : public ObjCacheEntry {
		PPID   PersonKindID;
		PPID   RegOrgKind;
		PPID   CounterID;
		long   Flags;
		int16  ExpiryPeriod;       // Период истечения срока годности в днях от даты ввода регистра
		int16  RestrictGoodsKind;
		PPID   RestrictGoodsGrpID;
	};
};

int SLAPI RegisterTypeCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	RegisterTypeData * p_cache_rec = static_cast<RegisterTypeData *>(pEntry);
	PPRegisterType data_rec;
	if((ok = PPRef->GetItem(PPOBJ_REGISTERTYPE, id, &data_rec)) > 0) {
		#define CPY(f) p_cache_rec->f = data_rec.f
	   	CPY(PersonKindID);
		CPY(RegOrgKind);
		CPY(CounterID);
		CPY(Flags);
		CPY(ExpiryPeriod);
		CPY(RestrictGoodsKind);
		CPY(RestrictGoodsGrpID);
		#undef CPY
		MultTextBlock b;
		b.Add(data_rec.Name);
		b.Add(data_rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	return ok;
}

void SLAPI RegisterTypeCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPRegisterType2 * p_data_rec = static_cast<PPRegisterType *>(pDataRec);
	const RegisterTypeData * p_cache_rec = static_cast<const RegisterTypeData *>(pEntry);
	memzero(p_data_rec, sizeof(PPRegisterType));
	p_data_rec->Tag = PPOBJ_REGISTERTYPE;
	#define CPY(f) p_data_rec->f = p_cache_rec->f
	CPY(ID);
	CPY(PersonKindID);
	CPY(RegOrgKind);
	CPY(CounterID);
	CPY(Flags);
	CPY(ExpiryPeriod);
	CPY(RestrictGoodsKind);
	CPY(RestrictGoodsGrpID);
	#undef CPY
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

int SLAPI PPObjRegisterType::Fetch(PPID id, PPRegisterType * pRec)
{
	RegisterTypeCache * p_cache = GetDbLocalCachePtr <RegisterTypeCache> (Obj);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

