// OBJBILST.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2009, 2010, 2011, 2012, 2014, 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
// @ModuleDef(PPObjBillStatus)
//
SLAPI PPObjBillStatus::PPObjBillStatus(void * extraPtr) : PPObjReference(PPOBJ_BILLSTATUS, extraPtr)
{
	ImplementFlags |= implStrAssocMakeList;
}

#define GRP_COLOR 1

int SLAPI PPObjBillStatus::Edit(PPID * pID, void * extraPtr)
{
	int    r = cmCancel, ok = 1, valid_data = 0;
	int    is_new = 0;
	PPBillStatus rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_BILLSTATUS))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(*pID) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->addGroup(GRP_COLOR, new ColorCtrlGroup(CTL_BILLSTATUS_COLOR, CTLSEL_BILLSTATUS_COLOR, cmSelColor, CTL_BILLSTATUS_SELCOLOR));
	dlg->setCtrlData(CTL_BILLSTATUS_NAME, rec.Name);
	dlg->setCtrlData(CTL_BILLSTATUS_SYMB, rec.Symb);
	dlg->setCtrlLong(CTL_BILLSTATUS_ID,   rec.ID);
	dlg->disableCtrl(CTL_BILLSTATUS_ID, (!PPMaster || rec.ID));
	dlg->setCtrlData(CTL_BILLSTATUS_RANK, &rec.Rank);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 0, BILSTF_DENY_MOD);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 1, BILSTF_DENY_DEL);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 2, BILSTF_DENY_TRANSM);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 3, BILSTF_DENY_CHANGELINK);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 4, BILSTF_DENY_RANKDOWN);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 5, BILSTF_LOCK_ACCTURN);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 6, BILSTF_LOCK_PAYMENT);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 7, BILSTF_LOCDISPOSE);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 8, BILSTF_READYFOREDIACK);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_FLAGS, 9, BILSTF_STRICTPRICECONSTRAINS); // @v10.2.5
	dlg->SetClusterData(CTL_BILLSTATUS_FLAGS, rec.Flags);

	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  0, BILCHECKF_AGENT);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  1, BILCHECKF_PAYER);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  2, BILCHECKF_DLVRADDR);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  3, BILCHECKF_PORTOFLOADING);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  4, BILCHECKF_PORTOFDISCHARGE);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  5, BILCHECKF_ISSUEDT);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  6, BILCHECKF_ARRIVALDT);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  7, BILCHECKF_SHIP);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  8, BILCHECKF_FREIGHTCOST);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS,  9, BILCHECKF_CAPTAIN);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS, 10, BILCHECKF_TRBROKER);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS, 11, BILCHECKF_OBJECT); 
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS, 12, BILCHECKF_OBJECT2);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKFLDS, 13, BILCHECKF_DUEDATE);
	dlg->SetClusterData(CTL_BILLSTATUS_CHECKFLDS, rec.CheckFields);
	dlg->AddClusterAssoc(CTL_BILLSTATUS_CHECKF2, 0, BILCHECKF_CODE);
	dlg->SetClusterData(CTL_BILLSTATUS_CHECKF2, rec.CheckFields);
	SetupPPObjCombo(dlg, CTLSEL_BILLSTATUS_OPCNTR, PPOBJ_OPCOUNTER, rec.CounterID, OLW_CANINSERT, 0);
	SetupOprKindCombo(dlg, CTLSEL_BILLSTATUS_OP, rec.RestrictOpID, 0, 0, 0);
	// @v10.2.4 {
	{
		ColorCtrlGroup::Rec color_rec;
		color_rec.SetupStdColorList();
		color_rec.C = (COLORREF)rec.IndColor;
		dlg->setGroupData(GRP_COLOR, &color_rec);
	}
	// } @v10.2.4 
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		THROW(is_new || CheckRights(PPR_MOD));
		dlg->getCtrlData(CTL_BILLSTATUS_NAME, rec.Name);
		dlg->getCtrlData(CTL_BILLSTATUS_SYMB, rec.Symb);
		if(!CheckName(*pID, strip(rec.Name), 0))
			dlg->selectCtrl(CTL_BILLSTATUS_NAME);
		else if(*strip(rec.Symb) && !ref->CheckUniqueSymb(Obj, rec.ID, rec.Symb, offsetof(PPBillStatus, Symb)))
			PPErrorByDialog(dlg, CTL_BILLSTATUS_SYMB);
		else {
			valid_data = 1;
			dlg->getCtrlData(CTL_BILLSTATUS_ID,   &rec.ID);
			dlg->getCtrlData(CTL_BILLSTATUS_RANK, &rec.Rank);
			dlg->GetClusterData(CTL_BILLSTATUS_FLAGS, &rec.Flags);
			dlg->GetClusterData(CTL_BILLSTATUS_CHECKFLDS, &rec.CheckFields);
			dlg->GetClusterData(CTL_BILLSTATUS_CHECKF2,   &rec.CheckFields);
			rec.CounterID = dlg->getCtrlLong(CTLSEL_BILLSTATUS_OPCNTR);
			rec.RestrictOpID = dlg->getCtrlLong(CTLSEL_BILLSTATUS_OP);
			// @v10.2.4 {
			{
				ColorCtrlGroup::Rec color_rec;
				dlg->getGroupData(GRP_COLOR, &color_rec);
				if(color_rec.C == 0)
					rec.IndColor.SetEmpty();
				else
					rec.IndColor = SColor((COLORREF)color_rec.C);
			}
			// } @v10.2.4 
			if(*pID)
				*pID = rec.ID;
			THROW(EditItem(PPOBJ_BILLSTATUS, *pID, &rec, 1));
			*pID = rec.ID;
			Dirty(*pID);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok ? r : 0;
}

// virtual
void * SLAPI PPObjBillStatus::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjBillStatusListWindow : public PPObjListWindow {
	public:
		PPObjBillStatusListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_BILLSTATUS);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				getResult(&id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmTransmitCharry:
							{
								PPIDArray id_list;
								ReferenceTbl::Rec rec;
								for(PPID item_id = 0; ((PPObjReference *)P_Obj)->EnumItems(&item_id, &rec) > 0;)
									id_list.add(rec.ObjID);
								if(!SendCharryObject(PPDS_CRRBILLSTATUS, id_list))
									PPError();
								clearEvent(event);
							}
							break;
					}
				}
				PostProcessHandleEvent(update, id);
			}
		}
	};
	return new PPObjBillStatusListWindow(this, flags, extraPtr);
}

int SLAPI PPObjBillStatus::Browse(void * extraPtr)
{
	return RefObjView(this, PPDS_CRRBILLSTATUS, 0);
}

int SLAPI PPObjBillStatus::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	PPBillStatus rec;
	if(msg == DBMSG_OBJDELETE) {
		switch(_obj) {
			case PPOBJ_OPCOUNTER:
				{
					for(SEnum en = ref->Enum(Obj, 0); ok && en.Next(&rec) > 0;) {
						if(rec.CounterID == _id)
							ok = RetRefsExistsErr(Obj, rec.ID);
					}
				}
				break;
			case PPOBJ_OPRKIND:
				{
					for(SEnum en = ref->Enum(Obj, 0); ok && en.Next(&rec) > 0;) {
						if(rec.RestrictOpID == _id)
							ok = RetRefsExistsErr(Obj, rec.ID);
					}
				}
				break;
		}
	}
	return ok;
}

int SLAPI PPObjBillStatus::Read(PPObjPack * p, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	THROW_MEM(p->Data = new PPBillStatus);
	if(stream == 0) {
		THROW(Search(id, (PPBillStatus*)p->Data) > 0);
	}
	else {
		THROW(Serialize_(-1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBillStatus::Write(PPObjPack * p, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	if(p && p->Data) {
		if(stream == 0) {
			PPBillStatus * p_rec = (PPBillStatus *)p->Data;
			if(*pID == 0) {
				PPID   same_id = 0;
				if(ref->SearchSymb(Obj, &same_id, p_rec->Symb, offsetof(PPBillStatus, Symb)) > 0) {
					PPBillStatus same_rec;
					if(Search(same_id, &same_rec) > 0) {
						ASSIGN_PTR(pID, same_id);
					}
					else
						same_id = 0;
				}
				if(same_id == 0) {
					p_rec->ID = 0;
					THROW(EditItem(Obj, *pID, p_rec, 1));
					ASSIGN_PTR(pID, ref->data.ObjID);
				}
			}
			else {
				;
			}
		}
		else {
			THROW(Serialize_(+1, static_cast<ReferenceTbl::Rec *>(p->Data), stream, pCtx));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBillStatus::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPID * p_id = 0;
		PPBillStatus * p_pack = (PPBillStatus*)p->Data;
		ProcessObjRefInArray(PPOBJ_OPCOUNTER, &p_pack->CounterID,       ary, replace);
		ProcessObjRefInArray(PPOBJ_OPRKIND,   &p_pack->RestrictOpID,    ary, replace);
		return 1;
	}
	return -1;
}
//
//
//
class BillStatusCache : public ObjCache {
public:
	SLAPI BillStatusCache() : ObjCache(PPOBJ_BILLSTATUS, sizeof(Data)) {}
private:
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct Data : public ObjCacheEntry {
		int16  Rank;
		long   Flags;
		long   CheckFields;
		PPID   CounterID;
		PPID   RestrictOpID;
		SColor IndColor; // @v10.2.4
	};
};

int SLAPI BillStatusCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjBillStatus bs_obj;
	PPBillStatus rec;
	if(bs_obj.Search(id, &rec) > 0) {
		#define FLD(f) p_cache_rec->f = rec.f
		FLD(Flags);
		FLD(Rank);
		FLD(CheckFields);
		FLD(CounterID);
		FLD(RestrictOpID);
		FLD(IndColor); // @v10.2.4
		#undef FLD
		MultTextBlock b;
		b.Add(rec.Name);
		b.Add(rec.Symb);
		ok = PutTextBlock(b, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void SLAPI BillStatusCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPBillStatus * p_data_rec = (PPBillStatus *)pDataRec;
	const Data * p_cache_rec = static_cast<const Data *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
	p_data_rec->Tag   = PPOBJ_BILLSTATUS;
	p_data_rec->ID    = p_cache_rec->ID;
	#define FLD(f) p_data_rec->f = p_cache_rec->f
	FLD(Flags);
	FLD(Rank);
	FLD(CheckFields);
	FLD(CounterID);
	FLD(RestrictOpID);
	FLD(IndColor); // @v10.2.4
	#undef FLD
	MultTextBlock b(this, pEntry);
	b.Get(p_data_rec->Name, sizeof(p_data_rec->Name));
	b.Get(p_data_rec->Symb, sizeof(p_data_rec->Symb));
}

IMPL_OBJ_FETCH(PPObjBillStatus, PPBillStatus, BillStatusCache);
//
// Implementation of PPALDD_BillStatus
//
PPALDD_CONSTRUCTOR(BillStatus)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjBillStatus;
	}
}

PPALDD_DESTRUCTOR(BillStatus)
{
	Destroy();
	delete (PPObjBillStatus *)Extra[0].Ptr;
}

int PPALDD_BillStatus::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjBillStatus * p_obj = (PPObjBillStatus *)(Extra[0].Ptr);
		PPBillStatus rec;
		if(p_obj->Fetch(rFilt.ID, &rec) > 0) {
			H.ID    = rec.ID;
			H.Flags = rec.Flags;
			H.CheckFields = rec.CheckFields;
			H.Rank  = rec.Rank;
			H.OpCntrID = rec.CounterID;
			H.RestrictOpID = rec.RestrictOpID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

