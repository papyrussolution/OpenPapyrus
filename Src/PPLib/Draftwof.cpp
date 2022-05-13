// DRAFTWOF.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>

PPDraftWrOffEntry::PPDraftWrOffEntry() : OpID(0), LocID(0), Flags(0)
{
	Reserve[0] = Reserve[1] = 0;
}

void PPDraftWrOff2::SetLotManufTimeParam(const PUGL::SetLotManufTimeParam * pS)
{
	if(pS) {
		SETFLAG(Flags, DWOF_SETMANUFDATE, pS->Flags & pS->fEnable);
		SetManufDateOffsDays = static_cast<int16>(pS->DateOffsDays);
		SetManufFixedTime = static_cast<int16>(pS->FixedTime);
	}
	else {
		Flags &= ~DWOF_SETMANUFDATE;
		SetManufDateOffsDays = 0;
		SetManufFixedTime = 0;
	}
}

void PPDraftWrOff2::GetLotManufTimeParam(PUGL::SetLotManufTimeParam * pS) const
{
	if(pS) {
		SETFLAG(pS->Flags, pS->fEnable, Flags & DWOF_SETMANUFDATE);
		pS->DateOffsDays = SetManufDateOffsDays;
		pS->FixedTime = SetManufFixedTime;
	}
}

PPDraftWrOff2::PPDraftWrOff2()
{
	THISZERO();
}

PPDraftWrOffPacket::PPDraftWrOffPacket() : P_List(0)
{
	// @v10.5.12 (Rec get ctr now) Init();
}

PPDraftWrOffPacket::~PPDraftWrOffPacket()
{
	delete P_List;
}

void PPDraftWrOffPacket::Init()
{
	ZDELETE(P_List);
	MEMSZERO(Rec);
}

PPDraftWrOffPacket & FASTCALL PPDraftWrOffPacket::operator = (const PPDraftWrOffPacket & s)
{
	Rec = s.Rec;
	ZDELETE(P_List);
	if(s.P_List) {
		P_List = new SArray(sizeof(PPDraftWrOffEntry));
		P_List->copy(*s.P_List);
	}
	return *this;
}
//
//
//
PPObjDraftWrOff::PPObjDraftWrOff(void * extraPtr) : PPObjReference(PPOBJ_DRAFTWROFF, extraPtr)
{
}

int PPObjDraftWrOff::PutPacket(PPID * pID, PPDraftWrOffPacket * pPack, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(*pID) {
			if(pPack) {
				THROW(CheckDupName(*pID, pPack->Rec.Name));
				THROW(P_Ref->UpdateItem(Obj, *pID, &pPack->Rec, 1, 0));
			}
			else {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			}
		}
		else {
			*pID = pPack->Rec.ID;
			THROW(P_Ref->AddItem(Obj, pID, &pPack->Rec, 0));
		}
		if(*pID) {
			PPIDArray list, * p_list = 0;
			if(pPack && pPack->P_List) {
				PPDraftWrOffEntry * p_entry;
				for(uint i = 0; pPack->P_List->enumItems(&i, (void **)&p_entry);)
					THROW(list.add(p_entry->OpID) && list.add(p_entry->LocID) &&
						list.add(p_entry->Reserve[0]) && list.add(p_entry->Reserve[1]) &&
						list.add(p_entry->Flags));
				p_list = &list;
			}
			THROW(P_Ref->PutPropArray(Obj, *pID, DWOPRP_ORDER, p_list, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjDraftWrOff::GetPacket(PPID id, PPDraftWrOffPacket * pPack)
{
	int    ok = -1;
	if(Search(id, &pPack->Rec) > 0) {
		PPIDArray list;
		P_Ref->GetPropArray(Obj, id, DWOPRP_ORDER, &list);
		PPDraftWrOffEntry entry;
		for(uint i = 0; i < list.getCount() / 5; i++) {
			entry.OpID  = list.at(i*5);
			entry.LocID = list.at(i*5+1);
			entry.Reserve[0] = 0L; //list.at(i*5+2);
			entry.Reserve[1] = 0L; //list.at(i*5+3);
			entry.Flags = list.at(i*5+4);
			if(pPack->P_List == 0)
				THROW_MEM(pPack->P_List = new SArray(sizeof(entry)));
			THROW_SL(pPack->P_List->insert(&entry));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
class DraftWrOffDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPDraftWrOffPacket);
public:
	DraftWrOffDialog() : PPListDialog(DLG_DRAFTWROFF, CTL_DRAFTWROFF_LIST)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		PPIDArray types;
		PPOprKind op_rec;
		setCtrlData(CTL_DRAFTWROFF_NAME, Data.Rec.Name);
		setCtrlData(CTL_DRAFTWROFF_ID, &Data.Rec.ID);
		AddClusterAssoc(CTL_DRAFTWROFF_FLAGS, 0, DWOF_USEMRPTAB);
		SetClusterData(CTL_DRAFTWROFF_FLAGS, Data.Rec.Flags);
		SetupPPObjCombo(this, CTLSEL_DRAFTWROFF_POOLOP, PPOBJ_OPRKIND, Data.Rec.PoolOpID, 0, reinterpret_cast<void *>(PPOPT_POOL));
		for(PPID op_id = 0; EnumOperations(0, &op_id, &op_rec) > 0;) {
			if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT)
				types.add(op_id);
			else if(IsIntrExpndOp(op_id))
				types.add(op_id);
		}
		SetupOprKindCombo(this, CTLSEL_DRAFTWROFF_DFCTOP, Data.Rec.DfctCompensOpID, 0, &types, OPKLF_OPLIST);
		// @v10.5.12 {
		{
			PUGL::SetLotManufTimeParam slmtp;
			Data.Rec.GetLotManufTimeParam(&slmtp);
			SString temp_buf;
			AddClusterAssoc(CTL_DRAFTWROFF_SETMT, 0, slmtp.fEnable);
			SetClusterData(CTL_DRAFTWROFF_SETMT, slmtp.Flags);
			setCtrlData(CTL_DRAFTWROFF_SETMTDO, &slmtp.DateOffsDays);
			slmtp.FixedTimeToString(temp_buf);
			setCtrlString(CTL_DRAFTWROFF_SETMTFT, temp_buf);
		}
		// } @v10.5.12
		setupDfctSelectors(Data.Rec.DfctCompensArID);
		SetupManufTimeCtrls(); // @v10.5.12
		updateList(-1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok  = 1;
		uint   sel = 0;
		getCtrlData(sel = CTL_DRAFTWROFF_NAME, Data.Rec.Name);
		if(*strip(Data.Rec.Name) == 0)
			ok = PPErrorByDialog(this, sel, PPERR_NAMENEEDED);
		//getCtrlData(CTL_DRAFTWROFF_ID, &Data.Rec.ID);
		GetClusterData(CTL_DRAFTWROFF_FLAGS, &Data.Rec.Flags);
		getCtrlData(CTLSEL_DRAFTWROFF_POOLOP, &Data.Rec.PoolOpID);
		getCtrlData(CTLSEL_DRAFTWROFF_DFCTOP,  &Data.Rec.DfctCompensOpID);
		getCtrlData(sel = CTLSEL_DRAFTWROFF_DFCTAR,  &Data.Rec.DfctCompensArID);
		if(IsIntrExpndOp(Data.Rec.DfctCompensOpID))
			if(!Data.Rec.DfctCompensArID)
				ok = PPErrorByDialog(this, sel, PPERR_UNDEFDFCTSRCLOC);
			else
				Data.Rec.Flags |= DWOF_DFCTARISLOC;
		// @v10.5.12 {
		{
			SString temp_buf;
			PUGL::SetLotManufTimeParam slmtp;
			GetClusterData(CTL_DRAFTWROFF_SETMT, &slmtp.Flags);
			getCtrlData(CTL_DRAFTWROFF_SETMTDO, &slmtp.DateOffsDays);
			getCtrlString(CTL_DRAFTWROFF_SETMTFT, temp_buf);
			slmtp.FixedTimeFromString(temp_buf);
			Data.Rec.SetLotManufTimeParam(&slmtp);
		}
		// } @v10.5.12
		if(ok > 0)
			ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	void   setupDfctSelectors(PPID arID);
	void   SetupManufTimeCtrls()
	{
		const PPID dfct_compens_op_id = getCtrlLong(CTLSEL_DRAFTWROFF_DFCTOP);
		DisableClusterItem(CTL_DRAFTWROFF_SETMT, 0, dfct_compens_op_id ? 0 : 1);
		if(dfct_compens_op_id) {
			PUGL::SetLotManufTimeParam slmtp;
			GetClusterData(CTL_DRAFTWROFF_SETMT, &slmtp.Flags);
			disableCtrls((slmtp.Flags & slmtp.fEnable) ? 0 : 1, CTL_DRAFTWROFF_SETMTDO, CTL_DRAFTWROFF_SETMTFT, 0);
		}
		else
			disableCtrls(1, CTL_DRAFTWROFF_SETMTDO, CTL_DRAFTWROFF_SETMTFT, 0);
	}
};

IMPL_HANDLE_EVENT(DraftWrOffDialog)
{
	long   p, i;
	PPListDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_DRAFTWROFF_DFCTOP)) {
		setupDfctSelectors(0);
		SetupManufTimeCtrls(); // @v10.5.12
	}
	else if(event.isCmd(cmaLevelUp)) {
		if(getCurItem(&p, &i) && Data.P_List && p > 0 && p < (long)Data.P_List->getCount()) {
			Data.P_List->swap(p, p-1);
			updateList(p-1);
		}
	}
	else if(event.isCmd(cmaLevelDown)) {
		if(getCurItem(&p, &i) && Data.P_List && p >= 0 && p < (long)Data.P_List->getCount()-1) {
			Data.P_List->swap(p, p+1);
			updateList(p+1);
		}
	}
	else if(event.isClusterClk(CTL_DRAFTWROFF_SETMT)) { // @v10.5.12
		SetupManufTimeCtrls(); 
	}
	else
		return;
	clearEvent(event);
}

int DraftWrOffDialog::setupList()
{
	PPDraftWrOffEntry * p_item = 0;
	SString sub;
	if(Data.P_List) {
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; Data.P_List->enumItems(&i, (void **)&p_item);) {
			ss.clear();
			GetOpName(p_item->OpID, sub);
			ss.add(sub);
			GetLocationName(p_item->LocID, sub);
			ss.add(sub);
			if(!addStringToList(i, ss.getBuf()))
				return 0;
		}
	}
	return 1;
}

int DraftWrOffDialog::delItem(long pos, long)
{
	return (Data.P_List && Data.P_List->atFree(static_cast<uint>(pos))) ? 1 : -1;
}

void DraftWrOffDialog::setupDfctSelectors(PPID arID)
{
	int    ok = -1;
	PPOprKind op_rec;
	const  PPID op_id = getCtrlLong(CTLSEL_DRAFTWROFF_DFCTOP);
	if(op_id) {
		if(IsIntrExpndOp(op_id)) {
			SetupPPObjCombo(this, CTLSEL_DRAFTWROFF_DFCTAR, PPOBJ_LOCATION, arID, 0, 0);
			ok = 1;
		}
		else if(GetOpData(op_id, &op_rec) > 0) {
			SetupArCombo(this, CTLSEL_DRAFTWROFF_DFCTAR, arID, OLW_LOADDEFONOPEN|OLW_CANINSERT, op_rec.AccSheetID, sacfDisableIfZeroSheet|sacfNonGeneric);
			ok = 1;
		}
	}
	if(ok < 0) {
		setCtrlLong(CTLSEL_DRAFTWROFF_DFCTAR, 0);
		disableCtrl(CTLSEL_DRAFTWROFF_DFCTAR, 1);
	}
	else
		disableCtrl(CTLSEL_DRAFTWROFF_DFCTAR, 0);
}

static int EditDraftWrOffItem(PPDraftWrOffEntry * pItem)
{
	class DraftWrOffEntryDialog : public TDialog {
		DECL_DIALOG_DATA(PPDraftWrOffEntry);
	public:
		DraftWrOffEntryDialog() : TDialog(DLG_DWOITEM)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPIDArray types;
			types.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, 0L);
			SetupOprKindCombo(this, CTLSEL_DWOITEM_OP, Data.OpID, 0, &types, 0);
			SetupPPObjCombo(this, CTLSEL_DWOITEM_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			getCtrlData(CTLSEL_DWOITEM_OP, &Data.OpID);
			getCtrlData(CTLSEL_DWOITEM_LOC, &Data.LocID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	};
	DIALOG_PROC_BODY(DraftWrOffEntryDialog, pItem);
}

int DraftWrOffDialog::addItem(long * pPos, long * pID)
{
	int    ok = 1;
	PPDraftWrOffEntry item;
	if(EditDraftWrOffItem(&item) > 0) {
		if(!Data.P_List)
			THROW_MEM(Data.P_List = new SArray(sizeof(PPDraftWrOffEntry)));
		THROW_SL(Data.P_List->insert(&item));
		{
			const long c = Data.P_List->getCount();
			ASSIGN_PTR(pPos, c-1);
			ASSIGN_PTR(pID, c);
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int DraftWrOffDialog::editItem(long pos, long /*id*/)
{
	if(Data.P_List && pos >= 0 && pos < Data.P_List->getCountI() &&
		EditDraftWrOffItem(static_cast<PPDraftWrOffEntry *>(Data.P_List->at(static_cast<uint>(pos)))) > 0)
		return 1;
	return -1;
}
//
//
//
int PPObjDraftWrOff::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel, valid_data = 0;
	DraftWrOffDialog * dlg = 0;
	PPDraftWrOffPacket data;
	if(*pID)
		THROW(GetPacket(*pID, &data));
	THROW(CheckDialogPtr(&(dlg = new DraftWrOffDialog)));
	dlg->setDTS(&data);
	while(!valid_data && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&data) && PutPacket(pID, &data, 1)) {
			valid_data = 1;
			ok = cmOK;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPObjDraftWrOff::Browse(void * extraPtr) { return RefObjView(this, PPDS_CRRDRAFTWROFF, 0); }
//
// @ModuleDef(PrcssrWrOffDraft)
//
IMPLEMENT_PPFILT_FACTORY(PrcssrWrOffDraft); PrcssrWrOffDraftFilt::PrcssrWrOffDraftFilt() : PPBaseFilt(PPFILT_PRCSSRWROFFDRAFTPARAM, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrWrOffDraftFilt, ReserveStart),
		offsetof(PrcssrWrOffDraftFilt, MrpTabName)-offsetof(PrcssrWrOffDraftFilt, ReserveStart));
	SetBranchSString(offsetof(PrcssrWrOffDraftFilt, MrpTabName));
	SetBranchSVector(offsetof(PrcssrWrOffDraftFilt, CSessList)); // @v9.8.4 SetBranchSArray-->SetBranchSVector
	Init(1, 0);
}

void PrcssrWrOffDraftFilt::SetLotManufTimeParam(const PUGL::SetLotManufTimeParam * pS)
{
	if(pS) {
		SETFLAG(Flags, fSetManufDate, pS->Flags & pS->fEnable);
		SetManufDateOffsDays = static_cast<int16>(pS->DateOffsDays);
		SetManufFixedTime = static_cast<int16>(pS->FixedTime);
	}
	else {
		Flags &= ~fSetManufDate;
		SetManufDateOffsDays = 0;
		SetManufFixedTime = 0;
	}
}

void PrcssrWrOffDraftFilt::GetLotManufTimeParam(PUGL::SetLotManufTimeParam * pS) const
{
	if(pS) {
		SETFLAG(pS->Flags, pS->fEnable, Flags & fSetManufDate);
		pS->DateOffsDays = SetManufDateOffsDays;
		pS->FixedTime = SetManufFixedTime;
	}
}

PrcssrWrOffDraft::PrcssrWrOffDraft() : P_BObj(BillObj)
{
}

int PrcssrWrOffDraft::InitParam(PrcssrWrOffDraftFilt * pP)
{
	if(pP) {
		PPObjLocation loc_obj;
		pP->Init(1, 0);
		pP->Period.SetDate(getcurdate_()); // @v10.8.10 LConfig.OperDate-->getcurdate_()
		pP->DwoID = DwoObj.GetSingle();
		pP->PoolLocID = loc_obj.GetSingleWarehouse();
	}
	return 1;
}

int PrcssrWrOffDraft::Init(const PrcssrWrOffDraftFilt * pP)
{
	RVALUEPTR(P, pP);
	P.Period.Actualize(ZERODATE);
	return 1;
}

class WrOffDraftParamDialog : public TDialog {
	DECL_DIALOG_DATA(PrcssrWrOffDraftFilt);
public:
	WrOffDraftParamDialog() : TDialog(DLG_DWOFILT)
	{
		SetupCalPeriod(CTLCAL_DWOFILT_PERIOD, CTL_DWOFILT_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		if(Data.CSessList.getCount()) {
			PPLoadText(PPTXT_WROFFDRAFTBYCSESSLIST, temp_buf);
			setStaticText(CTL_DWOFILT_ST_INFO, temp_buf);
			disableCtrl(CTL_DWOFILT_PERIOD, 1);
		}
		else
			disableCtrl(CTL_DWOFILT_PERIOD, 0);
		SetPeriodInput(this, CTL_DWOFILT_PERIOD, &Data.Period);
		SetupPPObjCombo(this, CTLSEL_DWOFILT_DWO, PPOBJ_DRAFTWROFF, Data.DwoID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_DWOFILT_LOC, PPOBJ_LOCATION, Data.PoolLocID, 0, 0);
		AddClusterAssoc(CTL_DWOFILT_FLAGS, 0, PrcssrWrOffDraftFilt::fCreateMrpTab);
		SetClusterData(CTL_DWOFILT_FLAGS, Data.Flags);
		setCtrlString(CTL_DWOFILT_MRPNAME, Data.MrpTabName);
		SetManufTimeParam(); // @v10.5.12
		SetupSetManufTimeParams(0);
		// } @v10.5.12 
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		THROW(GetPeriodInput(this, sel = CTL_DWOFILT_PERIOD, &Data.Period));
		getCtrlData(sel = CTLSEL_DWOFILT_DWO, &Data.DwoID);
		THROW_PP(Data.DwoID, PPERR_DWONEEDED);
		getCtrlData(CTLSEL_DWOFILT_LOC, &Data.PoolLocID);
		GetClusterData(CTL_DWOFILT_FLAGS, &Data.Flags);
		getCtrlString(CTL_DWOFILT_MRPNAME, Data.MrpTabName);
		GetManufTimeParam(); // @v10.5.12 
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_DWOFILT_DWO))
			SetupCtrls();
		else if(event.isClusterClk(CTL_DWOFILT_FLAGS))
			SetupCtrls();
		else if(event.isClusterClk(CTL_DWOFILT_SETMT)) {
			GetManufTimeParam();
			SetupSetManufTimeParams(0);
		}
		else
			return;
		clearEvent(event);
	}
	void   SetManufTimeParam()
	{
		SString temp_buf;
		PUGL::SetLotManufTimeParam slmtp;
		Data.GetLotManufTimeParam(&slmtp);
		AddClusterAssoc(CTL_DWOFILT_SETMT, 0, slmtp.fEnable);
		SetClusterData(CTL_DWOFILT_SETMT, slmtp.Flags);
		setCtrlData(CTL_DWOFILT_SETMTDO, &slmtp.DateOffsDays);
		slmtp.FixedTimeToString(temp_buf);
		setCtrlString(CTL_DWOFILT_SETMTFT, temp_buf);
	}
	void   GetManufTimeParam()
	{
		SString temp_buf;
		PUGL::SetLotManufTimeParam slmtp;
		GetClusterData(CTL_DWOFILT_SETMT, &slmtp.Flags);
		getCtrlData(CTL_DWOFILT_SETMTDO, &slmtp.DateOffsDays);
		getCtrlString(CTL_DWOFILT_SETMTFT, temp_buf);
		slmtp.FixedTimeFromString(temp_buf);
		Data.SetLotManufTimeParam(&slmtp);
	}
	void   SetupSetManufTimeParams(const PPDraftWrOff * pDwoRec)
	{
		SString temp_buf;
		int    setmt_enabled = 0;
		int    setmt_param_enabled = 0;
		PPDraftWrOff local_dwo_rec;
		if(!pDwoRec) {
			const PPID dwo_id = getCtrlLong(CTLSEL_DWOFILT_DWO);
			if(dwo_id && DwoObj.Search(dwo_id, &local_dwo_rec) > 0)
				pDwoRec = &local_dwo_rec;
		}
		if(pDwoRec && pDwoRec->ID && pDwoRec->DfctCompensOpID) {
			setmt_enabled = 1;
		}
		if(setmt_enabled) {
			SetManufTimeParam();
			setmt_param_enabled = BIN(Data.Flags & Data.fSetManufDate);
		}
		DisableClusterItem(CTL_DWOFILT_SETMT, 0, !setmt_enabled);
		disableCtrls(!setmt_param_enabled, CTL_DWOFILT_SETMTDO, CTL_DWOFILT_SETMTFT, 0);
	}
	void   SetupCtrls();
	PPObjDraftWrOff DwoObj;
};

void WrOffDraftParamDialog::SetupCtrls()
{
	PPID   prev_dwo_id = Data.DwoID;
	int    prev_crmrp_flag = BIN(Data.Flags & PrcssrWrOffDraftFilt::fCreateMrpTab);
	getCtrlData(CTLSEL_DWOFILT_DWO, &Data.DwoID);
	GetClusterData(CTL_DWOFILT_FLAGS, &Data.Flags);
	getCtrlString(CTL_DWOFILT_MRPNAME, Data.MrpTabName);
	Data.MrpTabName.Strip();
	PPDraftWrOff dwo_rec;
	if(DwoObj.Search(Data.DwoID, &dwo_rec) > 0) {
		if(Data.DwoID != prev_dwo_id || !prev_crmrp_flag) {
			if(!Data.MrpTabName[0] || sstrchr(Data.MrpTabName, '#')) {
				if(dwo_rec.Flags & DWOF_USEMRPTAB || Data.Flags & PrcssrWrOffDraftFilt::fCreateMrpTab) {
					SString mrp_name;
					PPObjMrpTab::GenerateName(PPOBJ_DRAFTWROFF, Data.DwoID, &mrp_name, 1);
					setCtrlString(CTL_DWOFILT_MRPNAME, mrp_name);
				}
			}
			if(dwo_rec.DfctCompensOpID) {
				SETFLAG(Data.Flags, Data.fSetManufDate, dwo_rec.Flags & DWOF_SETMANUFDATE);
				Data.SetManufDateOffsDays = dwo_rec.SetManufDateOffsDays;
				Data.SetManufFixedTime = dwo_rec.SetManufFixedTime;
			}
			else {
				Data.Flags &= ~Data.fSetManufDate;
				Data.SetManufDateOffsDays = 0;
				Data.SetManufFixedTime = 0;
			}
		}
	}
	else
		dwo_rec.ID = 0;
	SetupSetManufTimeParams(&dwo_rec);
}

int PrcssrWrOffDraft::EditParam(PrcssrWrOffDraftFilt * pParam) { DIALOG_PROC_BODY(WrOffDraftParamDialog, pParam); }

static int TestBillRec(const PPDraftWrOffEntry * pDwoEntry, const BillTbl::Rec & rRec, const PPIDArray * pDfctList)
{
	return BIN(!(rRec.Flags & BILLF_WRITEDOFF) && (rRec.OpID == pDwoEntry->OpID) &&
		(!pDwoEntry->LocID || pDwoEntry->LocID == rRec.LocID) && (!pDfctList || !pDfctList->lsearch(rRec.ID)));
}

struct DwoBillEntry { // @flat
	LDATE  Dt;
	PPID   LocID;
	PPID   ID;
};

static IMPL_CMPFUNC(DwoBillEntry, i1, i2) { RET_CMPCASCADE3(static_cast<const DwoBillEntry *>(i1), static_cast<const DwoBillEntry *>(i2), Dt, LocID, ID); }

int PrcssrWrOffDraft::ArrangeBillList(PPIDArray * pList)
{
	int    ok = 1;
	uint   i;
	SVector temp_list(sizeof(DwoBillEntry));
	for(i = 0; i < pList->getCount(); i++) {
		const PPID bill_id = pList->get(i);
		BillTbl::Rec bill_rec;
		if(P_BObj->Fetch(bill_id, &bill_rec) > 0) {
			DwoBillEntry entry;
			entry.Dt = bill_rec.Dt;
			entry.LocID = bill_rec.LocID;
			entry.ID = bill_rec.ID;
			THROW_SL(temp_list.insert(&entry));
		}
	}
	temp_list.sort(PTR_CMPFUNC(DwoBillEntry));
	pList->clear();
	for(i = 0; i < temp_list.getCount(); i++) {
		pList->addUnique(static_cast<const DwoBillEntry *>(temp_list.at(i))->ID);
	}
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::GetWrOffBillList(const PPDraftWrOffEntry * pDwoEntry, PPIDArray * pDfctList, PPIDArray * pList)
{
	int    ok = 1;
	const PPID op_id = pDwoEntry->OpID;
	if(op_id) {
		BillTbl::Rec bill_rec;
		PPIDArray local_list;
		const uint cs_c = P.CSessList.getCount();
		if(cs_c) {
			for(uint i = 0; i < cs_c; i++) {
				const PPID csess_id = P.CSessList.get(i);
				PPIDArray temp_bill_list;
				THROW(P_BObj->P_Tbl->GetPoolMembersList(PPASS_CSDBILLPOOL, csess_id, &temp_bill_list));
				for(uint j = 0; j < temp_bill_list.getCount(); j++) {
					const PPID bill_id = temp_bill_list.get(j);
					if(P_BObj->Fetch(bill_id, &bill_rec) > 0 && TestBillRec(pDwoEntry, bill_rec, pDfctList))
						THROW_SL(local_list.addUnique(bill_id));
				}
			}
		}
		else {
			for(DateIter di(&P.Period); P_BObj->P_Tbl->EnumByOpr(op_id, &di, &bill_rec) > 0;)
				if(TestBillRec(pDwoEntry, bill_rec, pDfctList))
					THROW_SL(local_list.addUnique(bill_rec.ID));
		}
		THROW(ArrangeBillList(&local_list));
		THROW_SL(pList->addUnique(&local_list));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::GetWrOffBillList(const PPDraftWrOffPacket * pPack, PPIDArray * pDfctList, PPIDArray * pList)
{
	int    ok = 1;
	PPDraftWrOffEntry * p_item;
	for(uint i = 0; pPack->P_List->enumItems(&i, (void **)&p_item);) {
		THROW(GetWrOffBillList(p_item, pDfctList, pList));
	}
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::WriteOff(const PPDraftWrOffPacket * pPack,
	PPIDArray * pWrOffBillList, PPIDArray * pDfctList, PUGL * pPugl, PPID * pErrBillID, int use_ta)
{
	int    ok = 1;
	SString msg_buf;
	PPIDArray bill_list;
	GetWrOffBillList(pPack, pDfctList, &bill_list);
	IterCounter cntr;
	cntr.Init(bill_list.getCount());
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < bill_list.getCount(); i++) {
			const PPID bill_id = bill_list.get(i);
			BillTbl::Rec bill_rec;
			if(P_BObj->Search(bill_id, &bill_rec) > 0) {
				PPWaitPercent(cntr.Increment(), PPObjBill::MakeCodeString(&bill_rec, 1, msg_buf));
				const int  r = P_BObj->WriteOffDraft(bill_id, pWrOffBillList, pPugl, 0);
				if(!r) {
					ASSIGN_PTR(pErrBillID, bill_id);
					CALLEXCEPT();
				}
				else if(r == -2) {
					pDfctList->addUnique(bill_id);
					ok = -2;
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::WriteOffMrp(const PPDraftWrOffPacket * pPack, PUGL * pPugl)
{
	//
	// 1. Строим MRP-таблицу по всему списанию для расчета полного дефицита
	//    по терминальной номенклатуре
	// Если итогового дефицита нет, то
	// для каждого элемента списка списания:
	//   2. Строим MRP-таблицу
	//   3. Сортируем листы MRP-таблицы {TabID, Dt, LocID}
	//   Для каждого листа MRP-таблицы
	//     4. Комплектуем недостающие позиции (рекурсивно)
	//     5. Списываем драфт-документы соответствующие MRP-листу по дате и складу
	//   6. После комплектации всех MRP-листов списываем документы окончательно
	//      Документы списанные на шаге 5 помечены как списанные и функция PPObjBill::WriteOffDraft
	//      пропускае их (returns -1), по этому двойного списания не будет
	//
	int    ok = -1, r;
	PPID   mrp_id = 0;
	MrpTabPacket mrp_pack;
	PUGL   pugl;
	PPIDArray wroff_bill_list;
	PPWaitStart();
	{
		THROW(CreateMrpTab(pPack, &mrp_id, &mrp_pack, 1 /*use_ta*/));
		THROW(r = MrpObj.GetDeficitList(&mrp_pack, MRPSRCV_TOTAL, 1, 0, &pugl)); // replacePassiveGoods=0
		if(r > 0) {
			pugl.ClearActions();
			pugl.AddAction(PCUG_BALANCE);
			pugl.AddAction(PCUG_CANCEL);
			pugl.OPcug = PCUG_CANCEL;
			//
			// Перед просмотром дефицита останавливаем транзакцию с сохранением изменений
			// (нам надо сохранить созданную MRP-таблицу)
			//
			PPWaitStop();
			//
			// Интерактивная функция просмотра дефицита
			//
			ProcessUnsuffisientList(DLG_MSGNCMPL4, &pugl);
			PPWaitStart();
			if(pugl.OPcug == PCUG_BALANCE) {
				THROW_PP(pPack->Rec.DfctCompensOpID, PPERR_UNDEFDWODFCTOP);
				PPWaitStart();
				if(mrp_pack.IsTree()) {
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < mrp_pack.getCount(); i++) {
						const MrpTabLeaf & r_leaf = mrp_pack.at(i);
						pugl.freeAll();
						THROW(r = MrpObj.P_Tbl->GetDeficitList_(r_leaf.TabID, MRPSRCV_TOTAL, 1, 1, &pugl)); // replacePassiveGoods=1
						THROW(ProcessDeficit(pPack, &pugl, 0));
					}
					THROW(tra.Commit());
				}
				else {
					//
					// Нам необходимо получить список дефицитных позиций для приходования //
					// с учетом подмены пассивных позиций (#).
					//
					pugl.freeAll();
					THROW(r = MrpObj.P_Tbl->GetDeficitList_(mrp_pack.GetBaseID(), MRPSRCV_TOTAL, 1, 1 /* # */, &pugl));
					THROW(ProcessDeficit(pPack, &pugl, 1));
				}
				ok = -2;
			}
			else
				ok = -1;
		}
		if(r < 0 || ok == -2) {
			ok = 1;
			SString fmt_buf, msg_buf, bill_name;
			PPDraftWrOffEntry * p_item;
			for(uint i = 0; pPack->P_List->enumItems(&i, (void **)&p_item);) {
				PPIDArray bill_list;
				if(GetWrOffBillList(p_item, 0, &bill_list) > 0) {
					PPDraftOpEx dox;
					//
					// Пытаемся компенсировать дефицит за счет комплектации по выстроенной MRP-таблице
					// Сразу вслед за компенсацией списываем драфт-документы день-за-днем
					// Иначе компенсация дефицита за следующий день может перехватить
					// скомплектованную номенклатуру.
					//
					if(OpObj.GetDraftExData(p_item->OpID, &dox) > 0) {
						PPID   wroff_compl_op_id = dox.WrOffComplOpID;
						PPID   mrp_src_id = MRPSRCV_TOTAL; // Источник возникновения дефицита.
						if(!wroff_compl_op_id && GetOpType(dox.WrOffOpID) == PPOPT_GOODSMODIF) {
							wroff_compl_op_id = dox.WrOffOpID;
							mrp_src_id = MRPSRCV_DEP; // Будем комплектовать только зависимый дефицит - независимый сформируется при списании
						}
						if(wroff_compl_op_id) {
							MrpTabLeaf * p_leaf = 0;
							PPTransaction tra(1);
							THROW(tra);
							mrp_pack.Destroy();
							mrp_pack.Init(PPOBJ_DRAFTWROFF, pPack->Rec.ID, pPack->Rec.Name);
							THROW(P_BObj->CreateMrpTab(&bill_list, &mrp_pack, &Logger, 0));
							mrp_pack.Sort();
							for(uint j = 0; mrp_pack.enumItems(&j, (void **)&p_leaf);) {
								if(!mrp_pack.IsTree() || p_leaf->TabID != mrp_pack.GetBaseID()) {
									PUGL::SetLotManufTimeParam slmt; // @v10.5.12
									P.GetLotManufTimeParam(&slmt); // @v10.5.12
									THROW(MrpObj.CreateModif(p_leaf, mrp_src_id, wroff_compl_op_id, &slmt, &wroff_bill_list, &Logger, 0));
									for(uint k = 0; k < bill_list.getCount(); k++) {
										const PPID bill_id = bill_list.at(k);
										BillTbl::Rec bill_rec;
										if(P_BObj->Search(bill_id, &bill_rec) > 0 && bill_rec.LocID == p_leaf->LocID && bill_rec.Dt == p_leaf->Dt) {
											PUGL local_pugl;
											THROW(r = P_BObj->WriteOffDraft(bill_id, &wroff_bill_list, &local_pugl, 0));
											if(pPugl) {
												local_pugl.Dt = bill_rec.Dt;
												pPugl->Add__(&local_pugl);
											}
											PPObjBill::MakeCodeString(&bill_rec, 1, bill_name);
											if(r == -2) {
												ok = -2;
												Logger.LogString(PPTXT_DRAFTWROFFFAILED, bill_name);
												local_pugl.Log(&Logger);
											}
											else if(r > 0)
												Logger.LogString(PPTXT_DRAFTWROFFSUCCESS, bill_name);
										}
									}
								}
							}
							THROW(MrpObj.DestroyPacket(&mrp_pack, 0));
							THROW(tra.Commit());
						}
					}
					//
					// Окончательно списываем draft-документы
					//
					{
						PPTransaction tra(1);
						THROW(tra);
						for(uint j = 0; j < bill_list.getCount(); j++) {
							PPID bill_id = bill_list.at(j);
							BillTbl::Rec bill_rec;
							if(P_BObj->Search(bill_id, &bill_rec) > 0 && !(bill_rec.Flags & BILLF_WRITEDOFF)) {
								PUGL local_pugl;
								THROW(r = P_BObj->WriteOffDraft(bill_id, &wroff_bill_list, &local_pugl, 0));
								if(pPugl) {
									local_pugl.Dt = bill_rec.Dt;
									pPugl->Add__(&local_pugl);
								}
								PPObjBill::MakeCodeString(&bill_rec, 1, bill_name);
								if(r == -2) {
									ok = -2;
									Logger.LogString(PPTXT_DRAFTWROFFFAILED, bill_name);
									local_pugl.Log(&Logger);
								}
								else if(r > 0)
									Logger.LogString(PPTXT_DRAFTWROFFSUCCESS, bill_name);
							}
						}
						THROW(tra.Commit());
					}
				}
			}
		}
		THROW(UniteToPool(pPack->Rec.PoolOpID, &wroff_bill_list, 1));
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PrcssrWrOffDraft::UniteToPool(PPID poolOpID, const PPIDArray * pBillList, int use_ta)
{
	int    ok = 1;
	const  uint bcount = SVectorBase::GetCount(pBillList);
	if(poolOpID && bcount) {
		TSVector <ObjAssocTbl::Rec> pool_list;
		PPBillPacket pool_pack;
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			THROW(pool_pack.CreateBlank(poolOpID, 0, P.PoolLocID, 0));
			if(P.PoolLocID)
				pool_pack.Rec.LocID = P.PoolLocID;
			pool_pack.Rec.Dt = ZERODATE;
			for(uint i = 0; i < bcount; i++) {
				const PPID bill_id = pBillList->get(i);
				BillTbl::Rec bill_rec;
				if(P_BObj->Search(bill_id, &bill_rec) > 0) {
					ObjAssocTbl::Rec assc_rec;
					// @v10.6.4 MEMSZERO(assc_rec);
					assc_rec.ScndObjID = bill_id;
					THROW_SL(pool_list.insert(&assc_rec));
					if(bill_rec.Dt > pool_pack.Rec.Dt)
						pool_pack.Rec.Dt = bill_rec.Dt;
				}
			}
			THROW(P_BObj->TurnPacket(&pool_pack, 0));
			{
				const PPID pool_id = pool_pack.Rec.ID;
				THROW(PPRef->Assc.AddArray(PPASS_OPBILLPOOL, pool_id, &pool_list, 1, 0));
				THROW(P_BObj->UpdatePool(pool_id, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::ProcessDeficit(const PPDraftWrOffPacket * pPack, const PUGL * pPugl, int use_ta)
{
	return P_BObj->ProcessDeficit(pPack->Rec.DfctCompensOpID, pPack->Rec.DfctCompensArID, pPugl, &Logger, use_ta);
}

int PrcssrWrOffDraft::CreateMrpTab(const PPDraftWrOffPacket * pPack, PPID * pMrpTabID, MrpTabPacket * pMrpPack, int use_ta)
{
	int    ok = 1;
	MrpTabPacket mrp_pack;
	PPIDArray bill_list;
	GetWrOffBillList(pPack, 0, &bill_list);
	ASSIGN_PTR(pMrpTabID, 0);
	mrp_pack.Init(PPOBJ_DRAFTWROFF, pPack->Rec.ID, P.MrpTabName);
	THROW(P_BObj->CreateMrpTab(&bill_list, &mrp_pack, &Logger, use_ta));
	ASSIGN_PTR(pMrpTabID, mrp_pack.GetBaseID());
	ASSIGN_PTR(pMrpPack, mrp_pack);
	CATCHZOK
	return ok;
}

int PrcssrWrOffDraft::Run()
{
	int    ok = -1, r;
	PPID   err_bill_id = 0;
	PPID   mrp_tab_id = 0;
	SString msg_buf;
	PPIDArray wroff_bill_list;
	PPDraftWrOffPacket dwo_pack;
	if(P.DwoID && DwoObj.GetPacket(P.DwoID, &dwo_pack) > 0 && dwo_pack.P_List) {
		if(P.Flags & PrcssrWrOffDraftFilt::fCreateMrpTab) {
			PPWaitStart();
			THROW(CreateMrpTab(&dwo_pack, &mrp_tab_id, 0, 1));
			ok = 1;
		}
		else if(dwo_pack.Rec.Flags & DWOF_USEMRPTAB) {
			PUGL   pugl;
			THROW(WriteOffMrp(&dwo_pack, &pugl));
			// @v9.6.7 {
			if(pugl.getCount()) {
				Logger.LogString(PPTXT_TOTALDRAFTWROFFDEFICIT, 0);
				pugl.Log(&Logger); 
			}
			// } @v9.6.7 
			ok = 1;
		}
		else {
			PPIDArray dfct_bill_list;
			do {
				PUGL   pugl;
				PPWaitStart();
				THROW(r = WriteOff(&dwo_pack, &wroff_bill_list, &dfct_bill_list, &pugl, &err_bill_id, 1));
				err_bill_id = 0;
				if(r == -2) { // deficit occured
					PPWaitStop();
					pugl.ClearActions();
					pugl.AddAction(PCUG_BALANCE);
					pugl.AddAction(PCUG_EXCLUDE);
					pugl.AddAction(PCUG_CANCEL);
					pugl.OPcug = PCUG_CANCEL;
					ProcessUnsuffisientList(DLG_MSGNCMPL4, &pugl);
					if(pugl.OPcug == PCUG_BALANCE) {
						P.GetLotManufTimeParam(&pugl.Slmt); // @v10.5.12
						THROW_PP(dwo_pack.Rec.DfctCompensOpID, PPERR_UNDEFDWODFCTOP);
						THROW(ProcessDeficit(&dwo_pack, &pugl, 1));
						dfct_bill_list.freeAll();
						ok = -2;
					}
					else if(pugl.OPcug == PCUG_EXCLUDE) {
						ok = -2;
					}
					else
						ok = -1;
				}
				else {
					THROW(UniteToPool(dwo_pack.Rec.PoolOpID, &wroff_bill_list, 1));
					ok = 1;
				}
			} while(ok < -1);
		}
		PPWaitStop();
	}
	CATCH
		ok = PPErrorZ();
		if(err_bill_id) {
			BillTbl::Rec bill_rec;
			if(P_BObj->Search(err_bill_id, &bill_rec) > 0) {
				PPOutputMessage(PPObjBill::MakeCodeString(&bill_rec, 1, msg_buf), mfInfo | mfOK);
			}
		}
	ENDCATCH
	return ok;
}

int WriteOffDrafts(const PPIDArray * pCSessList)
{
	int    ok = -1;
	PrcssrWrOffDraftFilt p;
	PrcssrWrOffDraft proc;
	proc.InitParam(&p);
	if(SVectorBase::GetCount(pCSessList)) {
		p.CSessList = *pCSessList;
		PPObjCSession cs_obj;
		PPObjCashNode cn_obj;
		CSessionTbl::Rec cs_rec;
		PPCashNode cn_rec;
		PPID   single_loc_id = 0;
		for(uint i = 0; i < p.CSessList.getCount(); i++) {
			if(cs_obj.Search(p.CSessList.get(i), &cs_rec) > 0) {
				if(cn_obj.Fetch(cs_rec.CashNodeID, &cn_rec) > 0) {
					if(!single_loc_id)
						single_loc_id = cn_rec.LocID;
					else if(cn_rec.LocID != single_loc_id) {
						single_loc_id = 0;
						break;
					}
				}
			}
		}
		if(single_loc_id)
			p.PoolLocID = single_loc_id;
	}
	while(proc.EditParam(&p) > 0) {
		if(!proc.Init(&p))
			PPError();
		if(proc.Run()) {
			ok = 1;
			break;
		}
	}
	return ok;
}
