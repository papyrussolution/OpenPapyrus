// V_BILL.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
SLAPI BillFilt::FiltExtraParam::FiltExtraParam(long setupValues, BrowseBillsType bbt) : SetupValues(setupValues), Bbt(bbt)
{
}

IMPLEMENT_PPFILT_FACTORY(Bill); SLAPI BillFilt::BillFilt() : PPBaseFilt(PPFILT_BILL, 0, 3), P_SjF(0), P_TagF(0) // @v8.2.9 ver 2-->3
{
	SetFlatChunk(offsetof(BillFilt, ReserveStart),
		offsetof(BillFilt, ReserveEnd)-offsetof(BillFilt, ReserveStart)+sizeof(ReserveEnd));
	SetBranchObjIdListFilt(offsetof(BillFilt, List));
	SetBranchObjIdListFilt(offsetof(BillFilt, LocList));
	SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(BillFilt, P_SjF));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt, P_TagF));
	SetBranchDisplayExtList(offsetof(BillFilt, Dl));
	Init(1, 0);
}

int SLAPI BillFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 2) {
		class BillFilt_v2 : public PPBaseFilt {
		public:
			SLAPI  BillFilt_v2() : PPBaseFilt(PPFILT_BILL, 0, 2), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(BillFilt, ReserveStart),
					offsetof(BillFilt, ReserveEnd)-offsetof(BillFilt, ReserveStart)+sizeof(ReserveEnd));
				SetBranchObjIdListFilt(offsetof(BillFilt, List));
				SetBranchObjIdListFilt(offsetof(BillFilt, LocList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(BillFilt, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt, P_TagF));
				Init(1, 0);
			}
			char   ReserveStart[32]; // @anchor
			long   Tag;
			DateRange DuePeriod;
			uint32 Count;
			uint8  Reserve[20];
			BrowseBillsType Bbt;
			DateRange Period;
			DateRange PaymPeriod;
			PPID   MainOrgID;
			PPID   PoolOpID;
			PPID   OpID;
			PPID   CurID;
			PPID   AccSheetID;
			PPID   ObjectID;
			PPID   Object2ID;
			PPID   PayerID;
			PPID   AgentID;
			PPID   CreatorID;
			PPID   StatusID;
			PPID   AssocID;
			PPID   PoolBillID;
			long   Flags;
			uint   DenyFlags;
			int16  ClientCardMode;
			int16  Ft_STax;
			int16  Ft_ClosedOrder;
			int16  SortOrder;
			PPID   Sel;
			RealRange AmtRange;
			long   ReserveEnd;     // @anchor
			SysJournalFilt * P_SjF;
			ObjIdListFilt List;
			ObjIdListFilt LocList;
			TagFilt * P_TagF;
		};
		BillFilt_v2 fv2;
		THROW(fv2.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
		memzero(Reserve, sizeof(Reserve));
#define CPYFLD(f) f = fv2.f
			CPYFLD(Tag);
			CPYFLD(DuePeriod);
			CPYFLD(Count);
			CPYFLD(Bbt);
			CPYFLD(Period);
			CPYFLD(PaymPeriod);
			CPYFLD(MainOrgID);
			CPYFLD(PoolOpID);
			CPYFLD(OpID);
			CPYFLD(CurID);
			CPYFLD(AccSheetID);
			CPYFLD(ObjectID);
			CPYFLD(Object2ID);
			CPYFLD(PayerID);
			CPYFLD(AgentID);
			CPYFLD(CreatorID);
			CPYFLD(StatusID);
			CPYFLD(AssocID);
			CPYFLD(PoolBillID);
			CPYFLD(Flags);
			CPYFLD(DenyFlags);
			CPYFLD(ClientCardMode);
			CPYFLD(Ft_STax);
			CPYFLD(Ft_ClosedOrder);
			CPYFLD(SortOrder);
			CPYFLD(Sel);
			CPYFLD(AmtRange);
			CPYFLD(ReserveEnd);

			CPYFLD(List);
			CPYFLD(LocList);
#undef CPYFLD
		if(fv2.P_SjF) {
			THROW_MEM(P_SjF = new SysJournalFilt);
			*P_SjF = *fv2.P_SjF;
		}
		else
			ZDELETE(P_SjF);
		if(fv2.P_TagF) {
			THROW_MEM(P_TagF = new TagFilt);
			*P_TagF = *fv2.P_TagF;
		}
		else
			ZDELETE(P_TagF);
		Dl.Clear(); // new field
		ok = 1;
	}
	CATCHZOK
	return ok;
}

// virtual
int SLAPI BillFilt::Describe(long flags, SString & rBuf) const
{
	long   id = 1;
	SString buf;
	StrAssocArray flag_list;
	PutObjMembToBuf(PPOBJ_ACCSHEET,    AccSheetID, STRINGIZE(AccSheetID),  rBuf);
	PutObjMembToBuf(PPOBJ_PERSON,      MainOrgID,  STRINGIZE(MainOrgID),   rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,     ObjectID,   STRINGIZE(ObjectID),    rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,     Object2ID,  STRINGIZE(Object2ID),   rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,     PayerID,    STRINGIZE(PayerID),     rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,     AgentID,    STRINGIZE(AgentID),     rBuf);
	PutObjMembToBuf(PPOBJ_USR,         CreatorID,  STRINGIZE(CreatorID),   rBuf);
	PutObjMembToBuf(PPOBJ_OPRKIND,     OpID,       STRINGIZE(OpID),        rBuf);
	PutObjMembToBuf(PPOBJ_BILL,        PoolBillID, STRINGIZE(PoolOpID),    rBuf);
	PutObjMembToBuf(PPOBJ_CURRENCY,    CurID,      STRINGIZE(CurID),       rBuf);
	PutObjMembToBuf(PPOBJ_STATUS,      StatusID,   STRINGIZE(StatusID),    rBuf);

	PutMembToBuf(&Period,              STRINGIZE(Period),     rBuf);
	PutMembToBuf(&PaymPeriod,          STRINGIZE(PaymPeriod), rBuf);
	PutMembToBuf(&DuePeriod,           STRINGIZE(DuePeriod),  rBuf);
	PutMembToBuf((long)Ft_STax,        STRINGIZE(Ft_STax),   rBuf);
	PutMembToBuf((long)Ft_ClosedOrder, STRINGIZE(Ft_ClosedOrder),       rBuf);

	if(DenyFlags & fDenyAdd)
		flag_list.Add(id++, STRINGIZE(fDenyAdd));
	if(DenyFlags & fDenyUpdate)
		flag_list.Add(id++, STRINGIZE(fDenyAdd));
	if(DenyFlags & fDenyRemove)
		flag_list.Add(id++, STRINGIZE(fDenyAdd));
	PutFlagsMembToBuf(&flag_list, STRINGIZE(DenyFlags), rBuf);

	if(ClientCardMode == ccmDebts)
		PutMembToBuf(STRINGIZE(ccmDebts),     STRINGIZE(ClientCardMode), rBuf);
	else if(ClientCardMode == ccmRPayments)
		PutMembToBuf(STRINGIZE(ccmRPayments), STRINGIZE(ClientCardMode), rBuf);
	if(SortOrder == ordByObject)
		PutMembToBuf(STRINGIZE(ordByObject),  STRINGIZE(SortOrder), rBuf);
	else if(SortOrder == ordByCode)
		PutMembToBuf(STRINGIZE(ordByCode),    STRINGIZE(SortOrder), rBuf);
	else
		PutMembToBuf(STRINGIZE(ordByDate),    STRINGIZE(SortOrder), rBuf);
	PutMembToBuf(&AmtRange, STRINGIZE(AmtRange), rBuf);
	{
		buf.Z();
#define __BBT(f) case f: buf = #f; break
		switch(Bbt) {
			__BBT(bbtUndef);
			__BBT(bbtGoodsBills);
			__BBT(bbtOrderBills);
			__BBT(bbtAccturnBills);
			__BBT(bbtInventoryBills);
			__BBT(bbtPoolBills);
			__BBT(bbtClientDebt);
			__BBT(bbtClientRPayment);
			__BBT(bbtDraftBills);
			__BBT(bbtRealTypes);
			__BBT(bbtWmsBills);
		}
#undef __BBT
		PutMembToBuf(buf,  STRINGIZE(Bbt),  rBuf);
	}
	PutObjMembListToBuf(PPOBJ_LOCATION, &LocList,   STRINGIZE(LocList),   rBuf);
	{
		id = 1;
		flag_list.Z();
#define __ADD_FLAG(f) if(Flags & f) flag_list.Add(id++, #f)
		__ADD_FLAG(fShowDebt);
		__ADD_FLAG(fDebtOnly);
		__ADD_FLAG(fPaymNeeded);
		__ADD_FLAG(fFreightedOnly);
		__ADD_FLAG(fCashOnly);
		__ADD_FLAG(fOrderOnly);
		__ADD_FLAG(fInvOnly);
		__ADD_FLAG(fAsSelector);
		__ADD_FLAG(fLabelOnly);
		__ADD_FLAG(fAllCurrencies);
		__ADD_FLAG(fAccturnOnly);
		__ADD_FLAG(fSetupNewBill);
		__ADD_FLAG(fDraftOnly);
		__ADD_FLAG(fDebtsWithPayments);
		__ADD_FLAG(fPoolOnly);
		__ADD_FLAG(fShowAck);
		__ADD_FLAG(fEditPoolByType);
		__ADD_FLAG(fIgnoreRtPeriod);
		__ADD_FLAG(fShowWoAgent);
		__ADD_FLAG(fBillListOnly);
		__ADD_FLAG(fWmsOnly);
		__ADD_FLAG(fUnshippedOnly);
#undef __ADD_FLAG
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	return 1;
}

BillFilt & FASTCALL BillFilt::operator = (const BillFilt & s)
{
	Copy(&s, 0);
	return *this;
}

void FASTCALL BillFilt::SetupBrowseBillsType(BrowseBillsType bbt)
{
	switch(bbt) {
		case bbtGoodsBills:
			if(Flags & (fOrderOnly | fInvOnly | fAccturnOnly | fPoolOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fInvOnly | fAccturnOnly | fPoolOnly | fWmsOnly);
			break;
		case bbtOrderBills:
			if(!(Flags & fOrderOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fInvOnly | fAccturnOnly | fPoolOnly | fWmsOnly);
			Flags |= fOrderOnly;
			break;
		case bbtAccturnBills:
			if(!(Flags & fAccturnOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fInvOnly | fPoolOnly | fWmsOnly);
			Flags |= fAccturnOnly;
			break;
		case bbtInventoryBills:
			if(!(Flags & fInvOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fAccturnOnly | fPoolOnly | fWmsOnly);
			Flags |= fInvOnly;
			break;
		case bbtPoolBills:
			if(!(Flags & fPoolOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fInvOnly | fAccturnOnly | fWmsOnly);
			Flags |= fPoolOnly;
			break;
		case bbtDraftBills:
			if(!(Flags & fDraftOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fAccturnOnly | fPoolOnly | fInvOnly | fWmsOnly);
			Flags |= fDraftOnly;
			Ft_ClosedOrder = -1;
			break;
		case bbtWmsBills:
			if(!(Flags & fDraftOnly))
				OpID = ObjectID = 0;
			Flags &= ~(fOrderOnly | fAccturnOnly | fPoolOnly | fInvOnly | fDraftOnly);
			Flags |= fWmsOnly;
			break;
	}
}

#define GRP_LOC 1

class BillFiltDialog : public WLDialog {
public:
	BillFiltDialog(uint dlgID, const char * pAddText) : WLDialog(dlgID, CTL_BILLFLT_LABEL)
	{
		Data.Sel = -1;
		setSubTitle(pAddText);
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_BILLFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		SetupCalPeriod(CTLCAL_BILLFLT_PERIOD, CTL_BILLFLT_PERIOD);
		SetupCalPeriod(CTLCAL_BILLFLT_DUEPERIOD, CTL_BILLFLT_DUEPERIOD);
	}
	int    setDTS(const BillFilt *);
	int    getDTS(BillFilt *);
private:
	DECL_HANDLE_EVENT;
	void   setupAccSheet(PPID accSheetID, PPID accSheet2ID);
 	void   extraFilt();
	void   viewOptions();
	void   SetupLocationCombo();
	BillFilt Data;
};

int SLAPI BillFilterDialog(uint dlgID, BillFilt * pFilt, TDialog ** ppDlg, const char * pAddText)
{
	int    r, valid_data = 0;
	BillFiltDialog * dlg = 0;
	if(*ppDlg == 0) {
		if(!CheckDialogPtr(&(dlg = new BillFiltDialog(dlgID, pAddText))))
			return 0;
		*ppDlg = dlg;
	}
	else
		dlg = (BillFiltDialog *)*ppDlg;
	dlg->setDTS(pFilt);
	while(!valid_data && (r = ExecView(dlg)) == cmOK)
		valid_data = dlg->getDTS(pFilt);
	return r;
}

int SLAPI BillFilterDialog(uint dlgID, BillFilt * pFilt, const char * pAddText)
{
	BillFiltDialog * dlg = 0;
	int    r = BillFilterDialog(dlgID, pFilt, (TDialog**)&dlg, pAddText);
	delete dlg;
	return r;
}

void BillFiltDialog::viewOptions()
{
	TDialog * dlg = new TDialog(DLG_BILLFLTVOPT);
	if(CheckDialogPtrErr(&dlg)) {
		long   flags = 0;
		// PPViewDisplayExtList Dl
		if(Data.Dl.GetItemByDataId(BillFilt::dliFreightIssueDate, 0))
			flags |= 0x0001;
		if(Data.Dl.GetItemByDataId(BillFilt::dliFreightArrivalDate, 0))
			flags |= 0x0002;
		if(Data.Dl.GetItemByDataId(BillFilt::dliDueDate, 0))
			flags |= 0x0004;
		if(Data.Dl.GetItemByDataId(BillFilt::dliAgentName, 0))
			flags |= 0x0008;
		if(Data.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0))
			flags |= 0x0010;
		if(Data.Dl.GetItemByDataId(BillFilt::dliDlvrAddr, 0))
			flags |= 0x0020;
		const long preserve_flags = flags;
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 0, 0x01);
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 1, 0x02);
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 2, 0x04);
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 3, 0x08);
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 4, 0x10);
		dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, 5, 0x20);
		dlg->SetClusterData(CTL_BILLFLTVOPT_FLAGS, flags);
		{
			PrcssrAlcReport::Config arp_cfg;
			const int enbl_alclic = BIN(PrcssrAlcReport::ReadConfig(&arp_cfg) > 0 && arp_cfg.AlcLicRegTypeID);
			dlg->DisableClusterItem(CTL_BILLFLTVOPT_FLAGS, 4, !enbl_alclic);
		}
		if(ExecView(dlg) == cmOK) {
			flags = dlg->GetClusterData(CTL_BILLFLTVOPT_FLAGS);
			if(flags != preserve_flags) {
				if(flags & 0x0001)
					Data.Dl.SetItem(BillFilt::dliFreightIssueDate, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliFreightIssueDate);
				if(flags & 0x0002)
					Data.Dl.SetItem(BillFilt::dliFreightArrivalDate, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliFreightArrivalDate);
				if(flags & 0x0004)
					Data.Dl.SetItem(BillFilt::dliDueDate, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliDueDate);
				if(flags & 0x0008)
					Data.Dl.SetItem(BillFilt::dliAgentName, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliAgentName);
				if(flags & 0x0010)
					Data.Dl.SetItem(BillFilt::dliAlcoLic, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliAlcoLic);
				if(flags & 0x0020)
					Data.Dl.SetItem(BillFilt::dliDlvrAddr, 0, 0);
				else
					Data.Dl.RemoveItem(BillFilt::dliDlvrAddr);
			}
		}
	}
}

IMPL_HANDLE_EVENT(BillFiltDialog)
{
	WLDialog::handleEvent(event);
	if(event.isCmd(cmTags)) {
		SETIFZ(Data.P_TagF, new TagFilt());
		if(!Data.P_TagF)
			PPError(PPERR_NOMEM);
		else if(!EditTagFilt(PPOBJ_BILL, Data.P_TagF))
			PPError();
		if(Data.P_TagF->IsEmpty())
			ZDELETE(Data.P_TagF);
	}
	else if(event.isCbSelected(CTLSEL_BILLFLT_OPRKIND)) {
		const PPID prev_op_id = Data.OpID;
		getCtrlData(CTLSEL_BILLFLT_OPRKIND, &Data.OpID);
		if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
			PPID   acc_sheet_id = 0, acc_sheet2_id = 0;
			GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, &acc_sheet2_id);
			setupAccSheet(acc_sheet_id, acc_sheet2_id);
		}
		if(Data.OpID != prev_op_id) {
			PPID   op_type_id = GetOpType(Data.OpID, 0);
			if(op_type_id == PPOPT_DRAFTTRANSIT)
                SetupLocationCombo();
		}
	}
	else if(event.isCmd(cmaMore))
		extraFilt();
	else if(event.isCmd(cmViewOptions))
		viewOptions();
	else if(event.isCmd(cmSysjFilt2)) {
		SysJournalFilt sj_filt;
		RVALUEPTR(sj_filt, Data.P_SjF);
		sj_filt.ObjType = PPOBJ_BILL;
		if(EditSysjFilt2(&sj_filt) > 0) {
			SETIFZ(Data.P_SjF, new SysJournalFilt);
			ASSIGN_PTR(Data.P_SjF, sj_filt);
		}
		if(Data.P_SjF) {
			//
			// Функция SysJournalFilt::IsEmpty считает фильтр, в котором установлен ObjType
			// не пустым. В данном случае это - не верно.
			//
			Data.P_SjF->ObjType = 0;
			if(Data.P_SjF->IsEmpty()) {
				ZDELETE(Data.P_SjF);
			}
			else
				Data.P_SjF->ObjType = PPOBJ_BILL;
		}
	}
	else if(event.isClusterClk(CTL_BILLFLT_PAYM)) {
		ushort v = getCtrlUInt16(CTL_BILLFLT_PAYM);
		if(v & 0x02)
			v |= 0x01;
		if(Data.Bbt == bbtGoodsBills) {
			if(v & 0x04)
				v &= ~0x08;
			DisableClusterItem(CTL_BILLFLT_PAYM, 2, v & 0x08);
			DisableClusterItem(CTL_BILLFLT_PAYM, 3, v & 0x04);
		}
		setCtrlUInt16(CTL_BILLFLT_PAYM, v);
	}
	else
		return;
	clearEvent(event);
}

void BillFiltDialog::extraFilt()
{
	if(GetSellAccSheet() || GetAgentAccSheet()) {
		if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
			GetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod);
		}
		const PPID cur_user_id = LConfig.User;
		PPBillExt ext;
		ext.PayerID = Data.PayerID;
		ext.AgentID = Data.AgentID;
		ext.Ft_STax = Data.Ft_STax;
		ext.Ft_Declined = Data.Ft_Declined;
		ext.EdiRecadvStatus = Data.EdiRecadvStatus; // @v9.1.6
		ext.EdiRecadvConfStatus = Data.EdiRecadvConfStatus; // @v9.1.6
		ext.DuePeriod = Data.DuePeriod;
		ushort v = getCtrlUInt16(CTL_BILLFLT_FLAGS);
		PPAccessRestriction accsr;
		const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
		if(v & 0x02 || own_bill_restr == 1) {
			SETIFZ(Data.CreatorID, cur_user_id);
		}
		else if(own_bill_restr == 2) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(cur_user_id, &sec_rec) > 0)
				SETIFZ(Data.CreatorID, (sec_rec.ParentID | PPObjSecur::maskUserGroup));
		}
		else if(Data.CreatorID == cur_user_id)
			Data.CreatorID = 0;
		ext.CreatorID = Data.CreatorID;
		if(BillExtraDialog(0, &ext, 0, 2) > 0) {
			Data.PayerID = ext.PayerID;
			Data.AgentID = ext.AgentID;
			Data.Ft_STax = ext.Ft_STax;
			Data.Ft_Declined = ext.Ft_Declined;
			Data.EdiRecadvStatus = ext.EdiRecadvStatus; // @v9.1.6
			Data.EdiRecadvConfStatus = ext.EdiRecadvConfStatus; // @v9.1.6
			Data.CreatorID = ext.CreatorID;
			Data.DuePeriod = ext.DuePeriod;
			if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
				SetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod);
			}
			int    disable_own_bill_only_tag = 0;
			v = getCtrlUInt16(CTL_BILLFLT_FLAGS);
			if(Data.CreatorID)
				if(Data.CreatorID != cur_user_id) {
					setCtrlUInt16(CTL_BILLFLT_FLAGS, v & ~0x02);
					disable_own_bill_only_tag = 1;
				}
				else
					setCtrlUInt16(CTL_BILLFLT_FLAGS, v | 0x02);
			else
				setCtrlUInt16(CTL_BILLFLT_FLAGS, v & ~0x02);
			DisableClusterItem(CTL_BILLFLT_FLAGS, 1, disable_own_bill_only_tag);
		}
	}
	else
		enableCommand(cmaMore, 0);
}

void BillFiltDialog::SetupLocationCombo()
{
	LocationCtrlGroup::Rec loc_rec(&Data.LocList);
	PPID   op_type_id = GetOpType(Data.OpID, 0);
	LocationCtrlGroup * p_cgrp = (LocationCtrlGroup *)getGroup(GRP_LOC);
	if(p_cgrp) {
		if(op_type_id == PPOPT_DRAFTTRANSIT) {
			PPIDArray loc_list;
			BillObj->P_Tbl->GetLocListByOp(Data.OpID, loc_list);
			p_cgrp->SetExtLocList(loc_list.getCount() ? &loc_list : 0);
		}
		else
			p_cgrp->SetExtLocList(0);
		setGroupData(GRP_LOC, &loc_rec);
	}
}

int BillFiltDialog::setDTS(const BillFilt * pFilt)
{
	if(!Data.IsEqual(pFilt, 0)) {
		const  PPID cur_user_id = LConfig.User;
		ushort v;
		PPID   acc_sheet_id = 0, acc_sheet2_id = 0;
		int    is_op_kind_list = 0;
		PPIDArray types;
		PPObjOprKind opk_obj;

		Data = *pFilt;
		SetupLocationCombo();
		SetPeriodInput(this, CTL_BILLFLT_PERIOD, &Data.Period);
		SetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod);
		if(Data.Flags & BillFilt::fOrderOnly)
			types.addzlist(PPOPT_GOODSORDER, PPOPT_GENERIC, 0L);
		else if(Data.Flags & BillFilt::fAccturnOnly) {
			types.addzlist(PPOPT_ACCTURN, PPOPT_AGREEMENT, PPOPT_GENERIC, 0L); // @v10.2.2 PPOPT_AGREEMENT
		}
		else if(Data.Flags & BillFilt::fInvOnly)
			types.add(PPOPT_INVENTORY);
		else if(Data.Flags & BillFilt::fPoolOnly)
			types.add(PPOPT_POOL);
		else if(Data.Flags & BillFilt::fDraftOnly)
			types.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_GENERIC, 0L); // @v9.1.4 PPOPT_GENERIC
		else if(Data.Flags & BillFilt::fWmsOnly)
			types.addzlist(PPOPT_WAREHOUSE, 0L);
		else {
			PPBillPoolOpEx bpox;
			if(Data.PoolOpID && opk_obj.GetPoolExData(Data.PoolOpID, &bpox) > 0 && bpox.OpList.getCount()) {
				types.copy(bpox.OpList);
				is_op_kind_list = 1;
			}
			else
				types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL,
					PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK, PPOPT_GENERIC, PPOPT_CORRECTION, 0L);
		}
		SetupOprKindCombo(this, CTLSEL_BILLFLT_OPRKIND, Data.OpID, 0, &types, is_op_kind_list ? OPKLF_OPLIST : 0);
		GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, &acc_sheet2_id);
		setupAccSheet(NZOR(Data.AccSheetID, acc_sheet_id), acc_sheet2_id);
		SetupPPObjCombo(this, CTLSEL_BILLFLT_STATUS, PPOBJ_BILLSTATUS, Data.StatusID, 0, 0);
		SetupCurrencyCombo(this, CTLSEL_BILLFLT_CUR, Data.CurID, 0, 1, 0);
		SetRealRangeInput(this, CTL_BILLFLT_AMOUNT, &Data.AmtRange);
		setCtrlUInt16(CTL_BILLFLT_ALLCUR, BIN(Data.Flags & BillFilt::fAllCurrencies));
		v = 0;
		SETFLAG(v, 0x01, Data.Flags & BillFilt::fShowDebt);
		SETFLAG(v, 0x02, Data.Flags & BillFilt::fDebtOnly);
		if(Data.Flags & (BillFilt::fOrderOnly | BillFilt::fDraftOnly)) {
			SETFLAG(v, 0x04, Data.Ft_ClosedOrder < 0);
		}
		else if(Data.Bbt == bbtGoodsBills) {
			if(Data.Flags & BillFilt::fUnshippedOnly)
				Data.Flags &= ~BillFilt::fShippedOnly;
			SETFLAG(v, 0x04, Data.Flags & BillFilt::fUnshippedOnly);
			SETFLAG(v, 0x08, Data.Flags & BillFilt::fShippedOnly);
			DisableClusterItem(CTL_BILLFLT_PAYM, 2, Data.Flags & BillFilt::fShippedOnly);
			DisableClusterItem(CTL_BILLFLT_PAYM, 3, Data.Flags & BillFilt::fUnshippedOnly);
		}
		else {
			DisableClusterItem(CTL_BILLFLT_PAYM, 2, 1);
			DisableClusterItem(CTL_BILLFLT_PAYM, 3, 1);
		}
		setCtrlData(CTL_BILLFLT_PAYM, &v);
		{
			PPAccessRestriction accsr;
			const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
			if(own_bill_restr == 1) {
				Data.CreatorID = cur_user_id;
				DisableClusterItem(CTL_BILLFLT_FLAGS, 1, 1);
			}
			else if(own_bill_restr == 2) {
				PPObjSecur sec_obj(PPOBJ_USR, 0);
				PPSecur sec_rec;
				if(sec_obj.Fetch(cur_user_id, &sec_rec) > 0) {
					Data.CreatorID = (sec_rec.ParentID | PPObjSecur::maskUserGroup);
					DisableClusterItem(CTL_BILLFLT_FLAGS, 1, 1);
				}
			}
			else if(Data.CreatorID && Data.CreatorID != cur_user_id)
				DisableClusterItem(CTL_BILLFLT_FLAGS, 1, 1);
		}
		v = 0;
		DisableClusterItem(CTL_BILLFLT_FLAGS, 3, Data.Bbt != bbtGoodsBills); // @v9.7.12 @fix 4-->3
		SETFLAG(v, 0x01, Data.Flags & BillFilt::fSetupNewBill);
		SETFLAG(v, 0x02, Data.CreatorID == cur_user_id);
		SETFLAG(v, 0x04, Data.Flags & BillFilt::fShowWoAgent);
		SETFLAG(v, 0x08, Data.Flags & BillFilt::fDiscountOnly);
		SETFLAG(v, 0x10, Data.Flags & BillFilt::fCcPrintedOnly); // @v9.7.12
		setCtrlData(CTL_BILLFLT_FLAGS, &v);
		setWL(BIN(Data.Flags & BillFilt::fLabelOnly));
		AddClusterAssoc(CTL_BILLFLT_ORDER, 0, BillFilt::ordByDate);
		AddClusterAssoc(CTL_BILLFLT_ORDER, 1, BillFilt::ordByCode);
		AddClusterAssoc(CTL_BILLFLT_ORDER, 2, BillFilt::ordByObject);
		SetClusterData(CTL_BILLFLT_ORDER, (long)Data.SortOrder);
	}
	return 1;
}

int BillFiltDialog::getDTS(BillFilt * pFilt)
{
	int    ok = 1;
	const  PPConfig & r_cfg = LConfig;
	uint   sel = 0;
	ushort v;
	DateRange temp_period = Data.Period;
	LocationCtrlGroup::Rec loc_rec;
	getGroupData(GRP_LOC, &loc_rec);
	Data.LocList = loc_rec.LocList;
	THROW(GetPeriodInput(this, CTL_BILLFLT_PERIOD, &temp_period));
	THROW(GetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod));
	getCtrlData(CTLSEL_BILLFLT_OPRKIND, &Data.OpID);
	if(Data.OpID || Data.AccSheetID)
		getCtrlData(CTLSEL_BILLFLT_OBJECT, &Data.ObjectID);
	else
		Data.ObjectID = 0;
	getCtrlData(CTLSEL_BILLFLT_OBJ2, &(Data.Object2ID = 0));
	getCtrlData(CTLSEL_BILLFLT_STATUS, &Data.StatusID);
	getCtrlData(CTLSEL_BILLFLT_CUR, &Data.CurID);
	getCtrlData(CTL_BILLFLT_ALLCUR, &(v = 0));
	SETFLAG(Data.Flags, BillFilt::fAllCurrencies, v);
	if(v)
		Data.CurID = -1L;
	GetRealRangeInput(this, CTL_BILLFLT_AMOUNT, &Data.AmtRange);
	getCtrlData(CTL_BILLFLT_PAYM, &(v = 0));
	SETFLAG(Data.Flags, BillFilt::fDebtOnly, v & 0x02);
	SETFLAG(Data.Flags, BillFilt::fShowDebt, v & (0x01 | 0x02));
	if(Data.Flags & (BillFilt::fOrderOnly | BillFilt::fDraftOnly)) {
		Data.Ft_ClosedOrder = (v & 0x04) ? -1 : 0;
	}
	else if(Data.Bbt == bbtGoodsBills) {
		SETFLAG(Data.Flags, BillFilt::fUnshippedOnly, v & 0x04);
		SETFLAG(Data.Flags, BillFilt::fShippedOnly,   v & 0x08);
	}
	getCtrlData(CTL_BILLFLT_FLAGS, &(v = 0));
	SETFLAG(Data.Flags, BillFilt::fSetupNewBill, v & 0x01);
	if(v & 0x02) {
		SETIFZ(Data.CreatorID, r_cfg.User);
	}
	else if(Data.CreatorID == r_cfg.User)
		Data.CreatorID = 0;
	SETFLAG(Data.Flags, BillFilt::fShowWoAgent, v & 0x04);
	SETFLAG(Data.Flags, BillFilt::fLabelOnly, getWL());
	SETFLAG(Data.Flags, BillFilt::fDiscountOnly, v & 0x08);
	SETFLAG(Data.Flags, BillFilt::fCcPrintedOnly, v & 0x10); // @v9.7.12
	sel = CTL_BILLFLT_PERIOD;
	THROW((Data.Flags & BillFilt::fDebtOnly) || AdjustPeriodToRights(temp_period, 1));
	Data.Period = temp_period;
	Data.SortOrder = (int16)GetClusterData(CTL_BILLFLT_ORDER);
	*pFilt = Data;
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

void BillFiltDialog::setupAccSheet(PPID sheet, PPID accSheet2ID)
{
	if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
		SetupArCombo(this, CTLSEL_BILLFLT_OBJECT, Data.ObjectID, OLW_LOADDEFONOPEN, sheet, sacfDisableIfZeroSheet);
		if(!sheet && isCurrCtlID(CTL_BILLFLT_OBJECT))
			selectNext();
	}
	if(getCtrlView(CTLSEL_BILLFLT_OBJ2)) {
		SetupArCombo(this, CTLSEL_BILLFLT_OBJ2, Data.Object2ID, OLW_LOADDEFONOPEN, accSheet2ID, sacfDisableIfZeroSheet);
		if(!accSheet2ID && isCurrCtlID(CTL_BILLFLT_OBJ2))
			selectNext();
	}
}
//
//
//
SLAPI PPViewBill::PoolInsertionParam::PoolInsertionParam() : Verb(2), AddedBillKind(bbtGoodsBills)
{
}

SLAPI PPViewBill::PPViewBill() : PPView(0, &Filt, PPVIEW_BILL), P_TempTbl(0), P_TempOrd(0), P_BPOX(0), P_Arp(0),
	P_BObj(BillObj), CtrlX(0), P_IterState(0), LastSelID(0)
{
}

SLAPI PPViewBill::~PPViewBill()
{
	delete P_TempTbl;
	delete P_TempOrd;
	delete P_BPOX;
	delete P_Arp;
	SAlloc::F(P_IterState);
	DBRemoveTempFiles();
}

int SLAPI PPViewBill::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	if(Filt.Flags & BillFilt::fShippedOnly && Filt.Flags & BillFilt::fUnshippedOnly)
		Filt.Flags &= ~BillFilt::fShippedOnly;
	Filt.Period.Actualize(ZERODATE);
	Filt.PaymPeriod.Actualize(ZERODATE);
	Filt.DuePeriod.Actualize(ZERODATE);
	ZDELETE(P_TempTbl);
	ZDELETE(P_TempOrd);
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_BPOX);
	{
		int    do_destroy_arp = 1;
		if(Filt.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0)) {
			PrcssrAlcReport::Config arp_cfg;
			if(PrcssrAlcReport::ReadConfig(&arp_cfg) > 0 && arp_cfg.AlcLicRegTypeID) {
				THROW_MEM(P_Arp = new PrcssrAlcReport);
				P_Arp->Init();
				P_Arp->SetConfig(&arp_cfg);
				do_destroy_arp = 0;
			}
		}
		if(do_destroy_arp)
			ZDELETE(P_Arp);
	}
	LastSelID = 0;
	TempOrder = OrdByDefault;
	Counter.Init();
	IdList = Filt.List;
	GetOpList(&Filt, &OpList, &SingleOpID);
	THROW_PP(OpList.getCount(), PPERR_VIEWBYFILTISEMPTY);
	if(SingleOpID) {
		if(Filt.OpID == 0 || IsGenericOp(Filt.OpID) > 0)
			Filt.OpID = SingleOpID;
		else {
			THROW_PP(Filt.OpID == SingleOpID, PPErrCode = PPERR_VIEWBYFILTISEMPTY);
		}
	}
	{
		//
		// Инициализация списка складов LocList_. Если отчет должен строиться по всем складам,
		// то список LocList_ - пустой.
		//
		int    draft_transit_only = 0;
		SingleLocID = 0;
		LocList_.clear();
		if(OpList.getCount()) {
			draft_transit_only = 1;
			for(uint i = 0; draft_transit_only && i < OpList.getCount(); i++)
				if(GetOpType(OpList.get(i)) != PPOPT_DRAFTTRANSIT)
					draft_transit_only = 0;
		}
		if(!draft_transit_only) {
			if(Filt.LocList.IsExists() && Filt.LocList.GetCount()) {
				PPIDArray full_loc_list;
				LocObj.GetWarehouseList(&full_loc_list);
				THROW(LocObj.ResolveWarehouseList(&Filt.LocList.Get(), LocList_));
				LocList_.intersect(&full_loc_list);
			}
			else if(ObjRts.IsLocRights()) {
				PPIDArray full_loc_list;
				LocObj.GetWarehouseList(&full_loc_list);
				THROW(LocObj.ResolveWarehouseList(&full_loc_list, LocList_));
			}
		}
		else if(Filt.LocList.IsExists())
			LocList_ = Filt.LocList.Get();
		SingleLocID = LocList_.getSingle();
		LocList_.sort();
	}
	if(Filt.PoolBillID && PPObjBill::IsPoolOwnedByBill(Filt.AssocID)) {
		if(P_BObj->Search(Filt.PoolBillID) > 0)
			Filt.PoolOpID = P_BObj->P_Tbl->data.OpID;
	}
	if(Filt.PoolOpID) {
		PPObjOprKind opk_obj;
		THROW_MEM(P_BPOX = new PPBillPoolOpEx);
		opk_obj.GetPoolExData(Filt.PoolOpID, P_BPOX);
	}
	if(Filt.P_SjF && !Filt.P_SjF->IsEmpty()) {
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		PPIDArray bill_list;
		Filt.P_SjF->Period.Actualize(ZERODATE);
		THROW(p_sj->GetObjListByEventPeriod(PPOBJ_BILL, Filt.P_SjF->UserID, &Filt.P_SjF->ActionIDList, &Filt.P_SjF->Period, bill_list));
		if(IdList.IsExists())
			bill_list.intersect(&IdList.Get());
		IdList.Set(&bill_list);
	}
	if(!(Filt.Flags & BillFilt::fIgnoreRtPeriod))
		THROW(AdjustPeriodToRights(Filt.Period, 0));
	if(IsTempTblNeeded()) {
		IterOrder ord = OrdByDefault;
		if(Filt.SortOrder == BillFilt::ordByDate)
			ord = OrdByDate;
		else if(Filt.SortOrder == BillFilt::ordByCode)
			ord = OrdByCode;
		else if(Filt.SortOrder == BillFilt::ordByObject)
			ord = OrdByObjectName;
		THROW(CreateTempTable(ord, 0));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1, caption = -1;
	TDialog * d = 0;
	PPID   single_op_id = 0;
	PPIDArray op_list;
	uint   rez_id = DLG_BILLFLT;
	BrowseBillsType bbt = (pFilt) ? ((BillFilt*)pFilt)->Bbt : bbtUndef;
	BillFilt * p_filt = (BillFilt*)pFilt;
	if(oneof3(bbt, bbtOrderBills, bbtInventoryBills, bbtPoolBills)) {
		uint f;
		if(bbt == bbtOrderBills) {
			f = BillFilt::fOrderOnly;
			caption = 1;
			rez_id = DLG_BILLFLT_ORDER;
		}
		else if(bbt == bbtInventoryBills) {
			f = BillFilt::fInvOnly;
			caption = 3;
			rez_id = DLG_INVFLT;
		}
		else if(bbt == bbtPoolBills) {
			f = BillFilt::fPoolOnly;
			caption = 4;
		}
		else
			return -1;
		p_filt->Flags |= f;
	}
	else if(bbt == bbtAccturnBills) {
		p_filt->Flags |= BillFilt::fAccturnOnly;
		caption = 2;
	}
	else if(bbt == bbtDraftBills) {
		p_filt->Flags |= BillFilt::fDraftOnly;
		caption = 5;
		rez_id = DLG_BILLFLT_DRAFT;
	}
	else if(bbt == bbtWmsBills) {
		p_filt->Flags |= BillFilt::fWmsOnly;
		caption = 6;
	}
	else
		caption = 0;
	GetOpList(p_filt, &op_list, &single_op_id);
	SString temp_buf;
	if(op_list.getCount()) {
		if(single_op_id)
			p_filt->OpID = single_op_id;
		if(caption >= 0)
			PPGetSubStr(PPTXT_BILLFLTCAPTIONS, caption, temp_buf);
		else
			temp_buf.Z();
		// @v9.0.6 assert(MemHeapTracer::Check()); // @debug
		int r = BillFilterDialog(rez_id, p_filt, &d, temp_buf);
		assert(!d->H() || ::IsWindow(d->H())); // @debug
		ZDELETE(d);
		// @v9.0.6 assert(MemHeapTracer::Check()); // @debug
		if(r == cmOK)
			ok = 1;
		else if(r == 0)
			ok = 0;
	}
	else {
		int    bill_idx = -1;
		if(bbt == bbtOrderBills)
			bill_idx = PPNOTUNE_ORDER;
		else if(bbt == bbtInventoryBills)
			bill_idx = PPNOTUNE_INVENT;
		else if(bbt == bbtDraftBills)
			bill_idx = PPNOTUNE_DRAFT;
		if(bill_idx >= 0) {
			PPGetSubStr(PPTXT_NOTUNEANYOPS, bill_idx, temp_buf);
			PPOutputMessage(temp_buf, mfInfo);
		}
	}
	return ok;
}

// virtual
PPBaseFilt * SLAPI PPViewBill::CreateFilt(void * extraPtr) const
{
	BillFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_BILL, (PPBaseFilt**)&p_filt)) {
		const PPConfig & r_cfg = LConfig;
		BillFilt::FiltExtraParam * p = extraPtr ? (BillFilt::FiltExtraParam *)extraPtr : 0;
		if(p && p->SetupValues) {
			p_filt->Period.SetDate(r_cfg.OperDate);
			p_filt->LocList.Add(r_cfg.Location);
			p_filt->Bbt   = p->Bbt;
			if(p_filt->Bbt == bbtGoodsBills) {
				if(DS.CheckExtFlag(ECF_GOODSBILLFILTSHOWDEBT))
					p_filt->Flags |= BillFilt::fShowDebt;
			}
			else if(p_filt->Bbt == bbtDraftBills)
				p_filt->Ft_ClosedOrder = -1;
		}
		PPAccessRestriction accsr;
		const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
		if(own_bill_restr == 1)
			p_filt->CreatorID = r_cfg.User;
		else if(own_bill_restr == 2) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(r_cfg.User, &sec_rec) > 0)
				p_filt->CreatorID = (sec_rec.ParentID | PPObjSecur::maskUserGroup);
		}
	}
	else
		PPSetError(PPERR_BASEFILTUNSUPPORTED);
	return (PPBaseFilt*)p_filt;
}

int SLAPI PPViewBill::IsTempTblNeeded() const
{
	if((Filt.P_SjF && !Filt.P_SjF->IsEmpty()) || (Filt.P_TagF && !Filt.P_TagF->IsEmpty()) || IdList.IsExists() ||
		(Filt.PoolBillID && Filt.AssocID) || Filt.PayerID || Filt.AgentID ||
		(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) ||
		!Filt.PaymPeriod.IsZero() || Filt.SortOrder || Filt.Flags & BillFilt::fShowWoAgent || P_Arp || Filt.StatusID) {
		return 1;
	}
	else {
		return 0;
	}
}

int SLAPI PPViewBill::GetOpList(const BillFilt * pFilt, PPIDArray * pList, PPID * pSingleOpID) const
{
	int    ok = 1;
	const PPRights & r_orts = ObjRts;
	ASSIGN_PTR(pSingleOpID, 0);
	pList->freeAll();
	if(pFilt->OpID == 0 || IsGenericOp(pFilt->OpID) > 0) {
		PPID   ot = 0;
		PPIDArray ot_list;
		if(pFilt->Flags & BillFilt::fOrderOnly)
			ot = PPOPT_GOODSORDER;
		else if(pFilt->Flags & BillFilt::fInvOnly)
			ot = PPOPT_INVENTORY;
		else if(pFilt->Flags & BillFilt::fPoolOnly)
			ot = PPOPT_POOL;
		if(ot)
			ot_list.add(ot);
		else if(pFilt->Flags & BillFilt::fAccturnOnly)
			ot_list.addzlist(PPOPT_ACCTURN, PPOPT_AGREEMENT, 0L);
		else if(pFilt->Flags & BillFilt::fDraftOnly)
			ot_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, 0L);
		else if(pFilt->Flags & BillFilt::fWmsOnly)
			ot_list.add(PPOPT_WAREHOUSE);
		else if(pFilt->Flags & BillFilt::fPaymNeeded) {
			PPOprKind op_rec;
			for(PPID op_id = 0; EnumOperations(0L, &op_id, &op_rec) > 0;)
				if(op_rec.Flags & OPKF_NEEDPAYMENT && r_orts.CheckOpID(op_id, PPR_READ))
					pList->add(op_id);
		}
		else {
			if(pFilt->List.GetCount()) {
				ot_list.add(PPOPT_GOODSRECEIPT);
				ot_list.add(PPOPT_GOODSEXPEND);
				ot_list.add(PPOPT_GOODSRETURN);
				ot_list.add(PPOPT_GOODSREVAL);
				ot_list.add(PPOPT_GOODSMODIF);
				ot_list.add(PPOPT_GOODSORDER);
				ot_list.add(PPOPT_GOODSACK);
				ot_list.add(PPOPT_DRAFTRECEIPT);
				ot_list.add(PPOPT_DRAFTEXPEND);
				ot_list.add(PPOPT_DRAFTTRANSIT);
				ot_list.add(PPOPT_ACCTURN);
				ot_list.add(PPOPT_PAYMENT);
				ot_list.add(PPOPT_CHARGE);
				ot_list.add(PPOPT_CORRECTION);
				ot_list.add(PPOPT_AGREEMENT); // @v10.1.12
			}
			else {
				ot_list.add(PPOPT_GOODSRECEIPT);
				ot_list.add(PPOPT_GOODSEXPEND);
				ot_list.add(PPOPT_GOODSRETURN);
				ot_list.add(PPOPT_GOODSREVAL);
				ot_list.add(PPOPT_GOODSMODIF);
				ot_list.add(PPOPT_CORRECTION);
				if(pFilt->Bbt == bbtUndef) {
					ot_list.add(PPOPT_DRAFTRECEIPT);
					ot_list.add(PPOPT_DRAFTEXPEND);
					ot_list.add(PPOPT_DRAFTTRANSIT);
					ot_list.add(PPOPT_GOODSORDER);
					ot_list.add(PPOPT_ACCTURN);
				}
				if(pFilt->Bbt == bbtRealTypes || pFilt->OpID /* Generic op */) {
					ot_list.add(PPOPT_PAYMENT);
					ot_list.add(PPOPT_CHARGE);
					ot_list.add(PPOPT_GOODSACK);
					ot_list.add(PPOPT_ACCTURN);
					ot_list.add(PPOPT_GOODSORDER);
					ot_list.add(PPOPT_DRAFTRECEIPT);
					ot_list.add(PPOPT_DRAFTEXPEND);
					ot_list.add(PPOPT_DRAFTTRANSIT);
				}
			}
		}
		if(ot_list.getCount()) {
			ot_list.sortAndUndup();
			r_orts.MaskOpRightsByTypes(&ot_list, pList);
			if(pFilt->OpID) { // Generic op
				PPIDArray gen_op_list;
				GetGenericOpList(pFilt->OpID, &gen_op_list);
				pList->intersect(&gen_op_list);
			}
		}
	}
	else if(IsGenericOp(pFilt->OpID) > 0) {
		// @todo @v4.6.9 В эту точку программа не попадает (см if выше)
		PPIDArray gen_op_list;
		GetGenericOpList(pFilt->OpID, &gen_op_list);
		r_orts.MaskOpRightsByOps(&gen_op_list, pList);
	}
	else if(pFilt->OpID == PPOPK_INTRRECEIPT) {
		for(PPID op_id = 0; EnumOperations(0L, &op_id, 0) > 0;)
			if(IsIntrOp(op_id) > 0)
				pList->add(op_id);
	}
	else if(r_orts.CheckOpID(pFilt->OpID, PPR_READ))
		pList->add(pFilt->OpID);
	if(pList->getCount() == 1)
		ASSIGN_PTR(pSingleOpID, pList->at(0));
	return ok;
}

int FASTCALL PPViewBill::CheckFlagsForFilt(const BillTbl::Rec * pRec) const
{
	int    ok = 1;
	const  long f = pRec ? pRec->Flags : 0;
	const  long f2 = pRec ? pRec->Flags2 : 0;
	THROW(!(Filt.Flags & BillFilt::fCashOnly)      || (f & BILLF_CASH));
	THROW(!(Filt.Flags & BillFilt::fDebtOnly)      || !(f & BILLF_PAYOUT));
	THROW(!(Filt.Flags & BillFilt::fPaymNeeded)    || (f & BILLF_NEEDPAYMENT));
	THROW(!(Filt.Flags & BillFilt::fLabelOnly)     || (f & BILLF_WHITELABEL));
	THROW(!(Filt.Flags & BillFilt::fFreightedOnly) || (f & BILLF_FREIGHT));
	THROW(!(Filt.Flags & BillFilt::fUnshippedOnly) || !(f & BILLF_SHIPPED));
	THROW(!(Filt.Flags & BillFilt::fShippedOnly)   || (f & BILLF_SHIPPED));
	THROW(!(Filt.Flags & BillFilt::fDiscountOnly)  || (f & BILLF_TOTALDISCOUNT));
	THROW(!(Filt.Flags & BillFilt::fCcPrintedOnly) || (f & BILLF_CHECK)); // @v9.7.12
	if(Filt.Ft_STax > 0)
		{ THROW(f & BILLF_RMVEXCISE); }
	else if(Filt.Ft_STax < 0)
		{ THROW(!(f & BILLF_RMVEXCISE)); }
	if(Filt.Ft_ClosedOrder > 0)
		{ THROW(f & BILLF_CLOSEDORDER); }
	else if(Filt.Ft_ClosedOrder < 0)
		{ THROW(!(f & BILLF_CLOSEDORDER)); }
	if(Filt.Ft_Declined > 0)
		{ THROW(f2 & BILLF2_DECLINED); }
	else if(Filt.Ft_Declined < 0)
		{ THROW(!(f2 & BILLF2_DECLINED)); }
	// @v9.1.6 {
    if(Filt.EdiRecadvStatus) {
		const int recadv_status = pRec ? BillCore::GetRecadvStatus(*pRec) : 0;
		if(Filt.EdiRecadvStatus == -1) {
			THROW(recadv_status == 0);
		}
		else
			THROW(recadv_status == (int)Filt.EdiRecadvStatus);
    }
    if(Filt.EdiRecadvConfStatus) {
		const int recadv_conf_status = pRec ? BillCore::GetRecadvConfStatus(*pRec) : 0;
		if(Filt.EdiRecadvConfStatus == -1) {
			THROW(recadv_conf_status == 0);
		}
		THROW(recadv_conf_status == (int)Filt.EdiRecadvConfStatus);
    }
	// } @v9.1.6
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::CheckIDForFilt(PPID id, const BillTbl::Rec * pRec)
{
	if(IdList.IsExists() && !IdList.CheckID(id))
		return 0;
	if(pRec == 0)
		if(P_BObj->Search(id) > 0)
			pRec = & P_BObj->P_Tbl->data;
		else
			return 0;
	if(!Filt.Period.CheckDate(pRec->Dt))
		return 0;
	if(!Filt.DuePeriod.CheckDate(pRec->DueDate))
		return 0;
	if(Filt.ObjectID && pRec->Object != Filt.ObjectID)
		return 0;
	if(Filt.Object2ID && pRec->Object2 != Filt.Object2ID)
		return 0;
	if(Filt.StatusID && pRec->StatusID != Filt.StatusID)
		return 0;
	if(Filt.CurID >= 0 && !(Filt.Flags & BillFilt::fAllCurrencies))
		if(pRec->CurID != Filt.CurID)
			return 0;
	if(!CheckFlagsForFilt(pRec))
		return 0;
	if(!Filt.AmtRange.CheckVal(pRec->Amount))
		return 0;
	if(Filt.CreatorID) {
		if(Filt.CreatorID & PPObjSecur::maskUserGroup) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur usr_rec;
			if(sec_obj.Fetch(pRec->UserID, &usr_rec) <= 0 || usr_rec.ParentID != (Filt.CreatorID & ~PPObjSecur::maskUserGroup))
				return 0;
		}
		else if(pRec->UserID != Filt.CreatorID)
			return 0;
	}
	if(LocList_.getCount() && !LocList_.bsearch(pRec->LocID))
		return 0;
	if(!OpList.lsearch(pRec->OpID))
		return 0;
	if(Filt.PayerID || Filt.AgentID || Filt.Flags & BillFilt::fShowWoAgent) {
		PPBillExt bext;
		if(P_BObj->FetchExt(id, &bext) > 0) {
			if(bext.AgentID && Filt.Flags & BillFilt::fShowWoAgent)
				return 0;
			else if(!CheckFiltID(Filt.AgentID, bext.AgentID) || !CheckFiltID(Filt.PayerID, bext.PayerID))
				return 0;
		}
		else if(Filt.AgentID || Filt.PayerID)
			return 0;
	}
	if(PPObjTag::CheckForTagFilt(PPOBJ_BILL, pRec->ID, Filt.P_TagF) <= 0)
		return 0;
	else if(Filt.PoolBillID && !IsMemberOfPool(id))
		return 0;
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempBill);

int SLAPI PPViewBill::InitOrderRec(IterOrder ord, const BillTbl::Rec * pBillRec, TempOrderTbl::Rec * pOrdRec)
{
	int    ok = 1;
	SString temp_buf;
	memzero(pOrdRec, sizeof(TempOrderTbl::Rec));
	pOrdRec->ID = pBillRec->ID;
	if(ord == OrdByID) {
		temp_buf.CatLongZ(pBillRec->ID, 10);
	}
	else if(ord == OrdByDate) {
		temp_buf.Cat(pBillRec->Dt, DATF_YMD|DATF_NODIV|DATF_CENTURY).CatLongZ(pBillRec->BillNo, 10);
	}
	else if(ord == OrdByCode) {
		STRNSCPY(pOrdRec->Name, pBillRec->Code);
		BillCore::GetCode(pOrdRec->Name);
		temp_buf = pOrdRec->Name;
	}
	else if(ord == OrdByObjectName) {
		GetArticleName(pBillRec->Object, temp_buf);
	}
	else if(ord == OrdByOpName) {
		GetOpName(pBillRec->OpID, temp_buf);
	}
	else
		ok = -1;
	if(ok > 0) {
		temp_buf.ToUpper();
		if(Filt.Flags & BillFilt::fDescOrder) {
			SString invert_buf;
			for(uint i = 0; i < temp_buf.Len(); i++) {
				invert_buf.CatChar((char)(255-(uchar)temp_buf.C(i)));
			}
			temp_buf = invert_buf;
		}
		temp_buf.CopyTo(pOrdRec->Name, sizeof(pOrdRec->Name));
	}
	return ok;
}

void SLAPI PPViewBill::InitTempRec(BillTbl::Rec * pBillRec, TempBillTbl::Rec * pTmpRec)
{
	memzero(pTmpRec, sizeof(TempBillTbl::Rec));
	pTmpRec->BillID  = pBillRec->ID;
	pTmpRec->Dt      = pBillRec->Dt;
	pTmpRec->BillNo  = pBillRec->BillNo;
	pTmpRec->OpID    = pBillRec->OpID;
	pTmpRec->Object  = pBillRec->Object;
	pTmpRec->DueDate = pBillRec->DueDate;
	CALLPTRMEMB(P_Arp, GetBillLic(pBillRec->ID, &pTmpRec->LicRegID, 0));
}

int SLAPI PPViewBill::CalcDebtCardInSaldo(double * pSaldo)
{
	double saldo = 0.0;
	if(Filt.Period.low != 0) {
		BillFilt  temp_filt = Filt;
		BillTotal total;
		PPViewBill temp_view;
		temp_filt.Period.upp = plusdate(temp_filt.Period.low, -1);
		temp_filt.Period.low = ZERODATE;
		temp_filt.Flags |= BillFilt::fIgnoreRtPeriod;
		temp_view.Init_(&temp_filt);
		temp_view.CalcTotal(&total);
		saldo = total.Debit - total.Credit;
	}
	ASSIGN_PTR(pSaldo, saldo);
	return 1;
}

int SLAPI PPViewBill::EnumerateDebtCard(BillViewEnumProc proc, long param)
{
	int    ok = 1, r;
	uint   i;
	PPID   bill_id;
	BillCore * t = P_BObj->P_Tbl;
	PPIDArray payable_op_list;
	PPIDArray bill_id_list, lookup_list;
	PPIDArray pool_list;
	ObjIdListFilt ext_bill_list; // Список документов, соответствующих расширению фильта (AgentID, PayerID)
	PPObjOprKind op_obj;
	ArticleTbl::Rec ar_rec;
	BillTbl::Rec bill_rec;
	BillViewItem item;
	BillTbl::Key3 k;
	MEMSZERO(k);
	k.Object = Filt.ObjectID;
	BExtQuery q(t, 3);
	DBQ * dbq = 0;

	if(ArObj.Fetch(Filt.ObjectID, &ar_rec) > 0)
		op_obj.GetPayableOpList(ar_rec.AccSheetID, &payable_op_list);
	if((Filt.PayerID || Filt.AgentID) && !(CConfig.Flags & CCFLG_INDIVIDBILLEXTFILT)) {
		PPIDArray temp_list;
		THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, Filt.PayerID, temp_list));
		ext_bill_list.Set(&temp_list);
	}
	q.select(t->ID, t->OpID, t->Flags, t->CurID, t->Amount, t->Dt, 0L);
	dbq = & (t->Object == Filt.ObjectID);
	if(LocList_.getCount())
		dbq = ppcheckfiltidlist(dbq, t->LocID, &LocList_);
	if(Filt.CurID >= 0)
		dbq = & (*dbq && t->CurID == Filt.CurID);
	q.where(*dbq);
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;)
		if(!(Filt.Flags & BillFilt::fLabelOnly) || t->data.Flags & BILLF_WHITELABEL) {
			PPID   op_id = t->data.OpID;
			bill_id = t->data.ID;
			if(payable_op_list.lsearch(op_id) && ext_bill_list.CheckID(bill_id)) {
				if(Filt.Period.CheckDate(t->data.Dt)) {
					if((Filt.PayerID || Filt.AgentID) && !ext_bill_list.IsExists()) {
						PPBillExt bext;
						if(P_BObj->FetchExt(bill_id, &bext) > 0) {
							if(bext.AgentID && Filt.Flags & BillFilt::fShowWoAgent)
								continue;
							else if(!CheckFiltID(Filt.AgentID, bext.AgentID) || !CheckFiltID(Filt.PayerID, bext.PayerID))
								continue;
						}
					}
					THROW(P_BObj->Search(bill_id, &bill_rec) > 0);
					memcpy(&item, &bill_rec, sizeof(bill_rec));
					item.Debit  = BR2(item.Amount);
					item.Credit = 0;
					item.Saldo  = 0;
					THROW(r = proc(&item, param));
					if(r < 0)
						return -1;
				}
				THROW(bill_id_list.add(bill_id));
			}
		}
	bill_id_list.sort();
	for(i = 0; i < bill_id_list.getCount(); i++) {
		bill_id = bill_id_list.at(i);
		PPID   member_id = 0;
		for(DateIter diter(&Filt.Period); t->EnumLinks(bill_id, &diter, BLNK_PAYMRETN, &bill_rec) > 0;)
			if(!bill_id_list.bsearch(bill_rec.ID) && !lookup_list.lsearch(bill_rec.ID)) {
				memcpy(&item, &bill_rec, sizeof(bill_rec));
				item.Debit  = 0;
				item.Credit = BR2(item.Amount);
				item.Saldo  = 0;
				THROW(r = proc(&item, param));
				lookup_list.add(bill_rec.ID);
				if(r < 0)
					return -1;
			}
		{
			pool_list.clear();
			t->GetPoolMembersList(PPASS_PAYMBILLPOOL, bill_id, &pool_list);
			for(uint j = 0; j < pool_list.getCount(); j++) {
				const PPID member_id = pool_list.get(j);
				if(!bill_id_list.bsearch(member_id) && !lookup_list.lsearch(bill_rec.ID))
					if(P_BObj->Search(member_id, &bill_rec) > 0 && Filt.Period.CheckDate(bill_rec.Dt)) {
						memcpy(&item, &bill_rec, sizeof(bill_rec));
						item.Debit  = 0;
						item.Credit = BR2(item.Amount);
						item.Saldo  = 0;
						THROW(r = proc(&item, param));
						lookup_list.add(bill_rec.ID);
						if(r < 0)
							return -1;
					}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::Helper_EnumProc(PPID billID, const BillTbl::Rec * pRec, int checkForFilt, BillViewEnumProc proc, long param)
{
	int    ok = 1;
	BillTbl::Rec rec;
	SETIFZ(pRec, ((P_BObj->Search(billID, &rec) > 0) ? &rec : 0));
	if(pRec && (!checkForFilt || CheckIDForFilt(billID, pRec) > 0)) {
		BillViewItem item;
		MEMSZERO(item);
		memcpy(&item, pRec, sizeof(BillTbl::Rec));
		if(!Filt.PaymPeriod.IsZero()) {
			if(!P_BObj->P_Tbl->CalcPayment(item.ID, 1, &Filt.PaymPeriod, Filt.CurID, &item.Credit))
				return 0;
			item.Debit = item.Amount;
			item.Saldo = item.Debit - item.Credit;
		}
		int    r = proc(&item, param);
		if(r == 0)
			ok = 0;
		else if(r < 0)
			ok = -1;
	}
	return ok;
}

int SLAPI PPViewBill::Enumerator(BillViewEnumProc proc, long param)
{
	int    ok = 1, r = 1;
	const PPConfig & r_cfg = LConfig;
	PPIDArray temp_list;
	const PPIDArray * p_list = 0;
	int    check_list_item_for_filt = 1;
	BillCore * t = P_BObj->P_Tbl;
	BExtQuery * q = 0;
	if(IdList.IsExists()) {
		p_list = &IdList.Get();
		check_list_item_for_filt = (Filt.Flags & BillFilt::fBillListOnly) ? 0 : 1;
	}
	else if(Filt.P_SjF && !Filt.P_SjF->IsEmpty())
		ok = -1;
	else if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)
		ok = EnumerateDebtCard(proc, param);
	else if(Filt.PoolBillID && Filt.AssocID) {
		THROW(t->GetPoolMembersList(Filt.AssocID, Filt.PoolBillID, &temp_list));
		p_list = &temp_list;
	}
	else if((Filt.PayerID || Filt.AgentID) && !(CConfig.Flags & CCFLG_INDIVIDBILLEXTFILT)) {
		THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, Filt.PayerID, temp_list));
		p_list = &temp_list;
	}
	else {
		int    idx;
		union {
			BillTbl::Key1 bk1;
			BillTbl::Key2 bk2;
			BillTbl::Key3 bk3;
			BillTbl::Key5 bk5;
		} k;
		DBQ  * dbq = 0;
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		MEMSZERO(k);
		if(Filt.Flags & BillFilt::fCashOnly) {
			idx = 1;
			k.bk1.Dt = r_cfg.OperDate;
			dbq = & (t->Dt == r_cfg.OperDate && t->UserID == r_cfg.Cash);
		}
		else if(Filt.ObjectID) {
			idx = 3;
			k.bk3.Object = Filt.ObjectID;
			k.bk3.Dt = Filt.Period.low;
			dbq = & (t->Object == Filt.ObjectID && daterange(t->Dt, &Filt.Period));
			dbq = ppcheckfiltid(dbq, t->OpID, SingleOpID);
		}
		else if(SingleOpID) {
			idx = 2;
			k.bk2.OpID = SingleOpID;
			k.bk2.Dt = Filt.Period.low;
			dbq = & (t->OpID == SingleOpID && daterange(t->Dt, &Filt.Period));
		}
		else if(SingleLocID) {
			idx = 5;
			k.bk5.LocID = SingleLocID;
			k.bk5.Dt    = Filt.Period.low;
			dbq = & (t->LocID == SingleLocID && daterange(t->Dt, &Filt.Period));
		}
		else {
			idx = 1;
			k.bk1.Dt = Filt.Period.low;
			dbq = & daterange(t->Dt, &Filt.Period);
		}
		dbq = & (*dbq && daterange(t->DueDate, &Filt.DuePeriod));
		if(SingleLocID && idx != 5)
			dbq = & (*dbq && t->LocID == SingleLocID);
		dbq = ppcheckfiltid(dbq, t->Object2,  Filt.Object2ID);
		dbq = ppcheckfiltid(dbq, t->StatusID, Filt.StatusID);
		if(!(Filt.CreatorID & PPObjSecur::maskUserGroup))
			dbq = ppcheckfiltid(dbq, t->UserID, Filt.CreatorID);
		if(Filt.CurID >= 0 && !(Filt.Flags & BillFilt::fAllCurrencies))
			dbq = & (*dbq && t->CurID == Filt.CurID);
		dbq = & (*dbq && realrange(t->Amount, Filt.AmtRange.low, Filt.AmtRange.upp));
		THROW_MEM(q = new BExtQuery(t, idx));
		q->select(t->ID, t->Code, t->Dt, t->DueDate, t->BillNo, t->Object, t->OpID, /*t->StatusID,*/
			t->CurID, t->Flags, t->Flags2, t->Amount, t->LinkBillID, t->LocID, t->UserID, 0L).where(*dbq);
		for(q->initIteration(0, &k, spGt); ok > 0 && q->nextIteration() > 0;) {
			if(CheckFlagsForFilt(&t->data)) {
				const BillTbl::Rec bill_rec = t->data;
				PPBillExt bext;
				if(!SingleOpID && !OpList.lsearch(bill_rec.OpID))
					continue;
				if(!SingleLocID && LocList_.getCount() && !LocList_.lsearch(bill_rec.LocID))
					continue;
				if(Filt.CreatorID & PPObjSecur::maskUserGroup) {
					PPSecur sec_rec;
					if(sec_obj.Fetch(bill_rec.UserID, &sec_rec) <= 0 || sec_rec.ParentID != (Filt.CreatorID & ~PPObjSecur::maskUserGroup))
						continue;
				}
				if(Filt.PayerID || Filt.AgentID > 0) {
					if(P_BObj->FetchExt(bill_rec.ID, &bext) <= 0 || !CheckFiltID(Filt.AgentID, bext.AgentID) || !CheckFiltID(Filt.PayerID, bext.PayerID))
						continue;
				}
				else if(Filt.Flags & BillFilt::fShowWoAgent) {
					if(P_BObj->FetchExt(bill_rec.ID, &bext) > 0 && bext.AgentID != 0)
						continue;
				}
				else if(PPObjTag::CheckForTagFilt(PPOBJ_BILL, bill_rec.ID, Filt.P_TagF) <= 0)
					continue;
				THROW(ok = Helper_EnumProc(bill_rec.ID, &bill_rec, 0, proc, param));
			}
		}
	}
	if(p_list) {
		for(uint i = 0; ok > 0 && i < p_list->getCount(); i++)
			THROW(ok = Helper_EnumProc(p_list->get(i), 0, check_list_item_for_filt, proc, param));
	}
	CATCHZOK
	delete q;
	return ok;
}

struct IterProcParam_Total {
	int    CashChecks;
	const  BillFilt * P_Filt;
	PPObjBill * P_BObj;
	BillTotal * Data;
};

static int IterProc_Total(BillViewItem * pItem, long p)
{
	IterProcParam_Total * p_param = (IterProcParam_Total*)p;
	PPID   id = pItem->ID;
	if(p_param) {
		double amt  = BR2(pItem->Amount);
		double paym = 0.0;
		p_param->Data->Count++;
		if(p_param->CashChecks) {
			if(pItem->OpID == GetCashOp())
				p_param->Data->Sum += amt;
			else if(pItem->OpID == GetCashRetOp())
				p_param->Data->Sum -= amt;
		}
		else {
			p_param->Data->Sum += amt;
			AmtList temp_amt_list;
			uint   pos = 0;
			p_param->P_BObj->P_Tbl->GetAmountList(id, &temp_amt_list);
			if(!p_param->P_Filt->PaymPeriod.IsZero())
				paym = pItem->Credit;
			else if(temp_amt_list.Search(PPAMT_PAYMENT, pItem->CurID, &pos))
				paym = temp_amt_list.at(pos).Amt;
			p_param->Data->Amounts.Add(&temp_amt_list, 1);
			if((pItem->Flags & BILLF_NEEDPAYMENT) || CheckOpFlags(pItem->OpID, OPKF_RECKON, 0))
				p_param->Data->Debt += (amt - paym);
		}
		p_param->Data->Debit  += pItem->Debit;
		p_param->Data->Credit += pItem->Credit;
		return 1;
	}
	else
		return -1;
}

int SLAPI PPViewBill::CalcTotal(BillTotal * pTotal)
{
	int    ok = 1;
	pTotal->Reset();
	IterProcParam_Total param;
	param.P_BObj = P_BObj;
	param.Data = pTotal;
	param.P_Filt = &Filt;
	param.CashChecks = BIN(Filt.Flags & BillFilt::fCashOnly);
	ok = Enumerator(IterProc_Total, (long)&param);
	if(ok)
		if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) {
			CalcDebtCardInSaldo(&pTotal->InSaldo);
			pTotal->OutSaldo = pTotal->InSaldo + pTotal->Debit - pTotal->Credit;
		}
	return ok;
}

int SLAPI PPViewBill::CalcItemTotal(PPID billID, BillTotalData * pTotal)
{
	PPBillPacket pack;
	return (billID && P_BObj->ExtractPacket(billID, &pack) > 0) ? pack.CalcTotal(pTotal, BTC_CALCSALESTAXES) : -1;
}

static int IterProc_CrList(BillViewItem * pItem, long p)
{
	return (p && ((PPIDArray*)p)->add(pItem->ID)) ? 1 : PPSetErrorSLib();
}

int SLAPI PPViewBill::GetBillIDList(PPIDArray * pList)
{
	return Enumerator(IterProc_CrList, (long)pList);
}

struct IterProcParam_CrTmpTbl {
	PPViewBill::IterOrder Ord;
	int    IsOrdTbl;
	double Saldo;
	BExtInsert * bei;
	PPViewBill * BV;
};

static int IterProc_CrTmpTbl(BillViewItem * pItem, long p)
{
	TempBillTbl::Rec  tbrec;
	TempOrderTbl::Rec torec;
	IterProcParam_CrTmpTbl * param = (IterProcParam_CrTmpTbl*)p;
	if(param->IsOrdTbl) {
		param->BV->InitOrderRec(param->Ord, pItem, &torec);
		if(!param->bei->insert(&torec))
			return PPSetErrorDB();
	}
	else {
		param->BV->InitTempRec(pItem, &tbrec);
		const BillFilt * p_flt = (BillFilt*)param->BV->GetBaseFilt();
		if((p_flt->ObjectID && p_flt->Flags & BillFilt::fDebtsWithPayments) || !p_flt->PaymPeriod.IsZero()) {
			tbrec.Debit   = pItem->Debit;
			tbrec.Credit  = pItem->Credit;
			param->Saldo += (pItem->Debit - pItem->Credit);
			tbrec.Saldo   = param->Saldo;
		}
		if(!param->bei->insert(&tbrec))
			return PPSetErrorDB();
	}
	return 1;
}

int SLAPI PPViewBill::CreateTempTable(IterOrder ord, int * pIsOrdTbl)
{
	int    ok = 1;
	TempBillTbl  * btbl = 0;
	TempOrderTbl * otbl = 0;
	IterProcParam_CrTmpTbl param;
	MEMSZERO(param);
	param.Ord = ord;
	ZDELETE(P_TempOrd);
	ZDELETE(P_TempTbl);
	if(oneof3(ord, OrdByCode, OrdByObjectName, OrdByOpName) || (ord && Filt.Flags & BillFilt::fDescOrder)) {
		param.IsOrdTbl = 1;
		THROW(otbl = CreateTempOrderFile());
		THROW_MEM(param.bei = new BExtInsert(otbl));
	}
	else {
		param.IsOrdTbl = 0;
		THROW(btbl = CreateTempFile());
		THROW_MEM(param.bei = new BExtInsert(btbl));
	}
	param.BV = this;
	param.Saldo = 0;
	{
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		THROW(Enumerator(IterProc_CrTmpTbl, (long)&param));
		THROW_DB(param.bei->flash());
		if(btbl && (Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)) {
			param.Saldo = 0;
			THROW(CalcDebtCardInSaldo(&param.Saldo));
			TempBillTbl::Key1 k;
			MEMSZERO(k);
			while(btbl->searchForUpdate(1, &k, spGt)) {
				param.Saldo += (btbl->data.Debit - btbl->data.Credit);
				btbl->data.Saldo = param.Saldo;
				THROW_DB(btbl->updateRec()); // @sfu
			}
		}
		if(param.IsOrdTbl) {
			P_TempOrd = otbl;
			TempOrder = ord;
		}
		else
			P_TempTbl = btbl;
		THROW(tra.Commit());
		ASSIGN_PTR(pIsOrdTbl, param.IsOrdTbl);
	}
	CATCH
		ok = 0;
		ZDELETE(param.bei);
		delete btbl;
		delete otbl;
	ENDCATCH
	delete param.bei;
	return ok;
}

int SLAPI PPViewBill::InitIteration(IterOrder ord)
{
	int    ok = 1;
	char   key[MAXKEYLEN];
	int    idx = 0, use_ord_tbl = 0;
	_IterC = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	switch(ord) {
		case OrdByDefault:    idx = 1; break;
		case OrdByID:         idx = 0; break;
		case OrdByDate:       idx = 1; break;
		case OrdByCode:       idx = 1; use_ord_tbl = 1; break;
		case OrdByObjectName: idx = 1; use_ord_tbl = 1; break;
		case OrdByOpName:     idx = 1; use_ord_tbl = 1; break;
		default:              idx = 1;
	}
	if(Filt.Flags & BillFilt::fDescOrder)
		use_ord_tbl = 1;
	if(use_ord_tbl) {
		if(ord != TempOrder || !P_TempOrd)
			THROW(CreateTempTable(ord, 0));
		PPInitIterCounter(Counter, P_TempOrd);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempOrd, idx));
	}
	else {
		if(!P_TempTbl)
			THROW(CreateTempTable(ord, 0));
		PPInitIterCounter(Counter, P_TempTbl);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, idx));
	}
	P_IterQuery->selectAll();
	memzero(key, sizeof(key));
	P_IterQuery->initIteration(0, key, spFirst);
	CATCH
		BExtQuery::ZDelete(&P_IterQuery);
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL PPViewBill::NextIteration(BillViewItem * pItem)
{
	int    r = -1;
	if(Filt.Count && _IterC >= Filt.Count)
		r = -1;
	else {
		while((r = P_IterQuery->nextIteration()) > 0) {
			BillTbl::Rec br;
			Counter.Increment();
			const TempBillTbl * p_temp_tbl = P_TempTbl;
			const PPID id = p_temp_tbl ? p_temp_tbl->data.BillID : P_TempOrd->data.ID;
			if(P_BObj->Search(id, &br) > 0) {
				_IterC++;
				if(pItem) {
					memzero(pItem, sizeof(*pItem));
					*((BillTbl::Rec*)pItem) = br;
					if(p_temp_tbl) {
						pItem->Debit  = p_temp_tbl->data.Debit;
						pItem->Credit = p_temp_tbl->data.Credit;
						pItem->Saldo  = p_temp_tbl->data.Saldo;
					}
					if(CheckOpFlags(br.OpID, OPKF_NEEDPAYMENT)) {
						BillTbl::Rec last_paym_rec;
						if(P_BObj->P_Tbl->GetLastPayment(id, &last_paym_rec) > 0)
							pItem->LastPaymDate = last_paym_rec.Dt;
					}
				}
				return 1;
			}
		}
	}
	return r;
}

int SLAPI PPViewBill::SetIterState(const void * pSt, size_t sz)
{
	ZFREE(P_IterState);
	if(pSt && sz) {
		P_IterState = SAlloc::M(sz);
		memcpy(P_IterState, pSt, sz);
		return 1;
	}
	else
		return -1;
}

const void * SLAPI PPViewBill::GetIterState() const
{
	return P_IterState;
}
//
//
//
int SLAPI PPViewBill::WriteOffDraft(PPID id)
{
	int    ok = -1;
	uint   s = 1;
	PPDraftOpEx doe;
	BillTbl::Rec bill_rec;
	if(id && P_BObj->Search(id, &bill_rec) > 0) {
		if(GetOpSubType(bill_rec.OpID) == OPSUBT_DEBTINVENT) {
			THROW(P_BObj->WriteOffDebtInventory(id, 1));
			ok = 1;
		}
		else if(GetOpSubType(bill_rec.OpID) == OPSUBT_TRADEPLAN) {
			s = 0;
			const PPID op_id = bill_rec.OpID;
			PPID ar_id = bill_rec.Object;
			if(Filt.OpID != op_id || SelectorDialog(DLG_SELTRPLANCMP, CTL_SELTRPLANCMP_SEL, &s) > 0) {
				SArray bill_rec_list(sizeof(BillTbl::Rec));
				if(s == 0)
					bill_rec_list.insert(&bill_rec);
				else {
					PPIDArray idlist;
					THROW(GetBillIDList(&idlist));
					for(uint i = 0; i < idlist.getCount(); i++) {
						BillTbl::Rec temp_bill_rec;
						if(P_BObj->Search(idlist.get(i), &temp_bill_rec) > 0) {
							if(ar_id && temp_bill_rec.Object != ar_id)
								ar_id = 0;
							bill_rec_list.insert(&temp_bill_rec);
						}
					}
				}
				//
				PPOprKind op_rec;
				GoodsOpAnalyzeFilt goa_filt;
				PPViewGoodsOpAnalyze goa_view;
				P_BObj->P_OpObj->GetDraftExData(op_id, &doe);
				GetOpData(op_id, &op_rec);
				if(op_rec.AccSheetID == GetSupplAccSheet())
					goa_filt.Flags |= GoodsOpAnalyzeFilt::fTradePlanObjAsSuppl;
				goa_filt.OpGrpID = GoodsOpAnalyzeFilt::ogSelected;
				goa_filt.OpID    = doe.WrOffOpID;
				for(uint i = 0; i < bill_rec_list.getCount(); i++) {
					const BillTbl::Rec & r_bill_rec = *(BillTbl::Rec *)bill_rec_list.at(i);
					PPID   acc_sheet_id = 0;
					goa_filt.AddTradePlanBillID(r_bill_rec.ID);
					if(!goa_filt.Period.low || goa_filt.Period.low > r_bill_rec.Dt)
						goa_filt.Period.low = r_bill_rec.Dt;
					if(!goa_filt.Period.upp || goa_filt.Period.upp < r_bill_rec.DueDate)
						goa_filt.Period.upp = r_bill_rec.DueDate;
					if(GetArticleSheetID(ar_id, &acc_sheet_id) > 0) {
						if(acc_sheet_id == GetAgentAccSheet())
							goa_filt.AgentID = ar_id;
					}
					if(goa_filt.AgentID == 0 && goa_filt.ObjectID == 0)
						goa_filt.LocList.Add(r_bill_rec.LocID);
				}
				if(goa_view.EditBaseFilt(&goa_filt) > 0)
					THROW(ViewGoodsOpAnalyze(&goa_filt));
				ok = 1;
			}
		}
	}
	if(ok < 0 && (!id || SelectorDialog(DLG_SELWROFF, CTL_SELWROFF_SEL, &(s = 0)) > 0)) {
		PPObjMrpTab mrp_obj;
		int    is_deficit = 0;
		PPID   single_op_id_on_deficit = 0;
		PUGL   deficit_list;
		for(int try_again = 1; try_again != 0;) {
			if(s == 0) { // Списать выбранный документ
				if(P_BObj->Search(id, &bill_rec) > 0) {
					if(bill_rec.Flags & BILLF_WRITEDOFF)
						PPMessage(mfInfo|mfOK, PPINF_DRAFTALLREADYWROFF);
					else {
						THROW(ok = P_BObj->WriteOffDraft(id, 0, &deficit_list, 1));
						if(ok == -2) {
							if(!is_deficit)
								single_op_id_on_deficit = bill_rec.OpID;
							else if(single_op_id_on_deficit && bill_rec.OpID != single_op_id_on_deficit)
								single_op_id_on_deficit = 0;
							is_deficit = 1;
						}
					}
				}
			}
			else if(s == 1) { // Списать всю выборку документов
				PPIDArray idlist;
				PPWait(1);
				THROW(GetBillIDList(&idlist));
				{
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < idlist.getCount(); i++) {
						const PPID bill_id = idlist.at(i);
						if(P_BObj->Search(bill_id, &bill_rec) > 0 && !(bill_rec.Flags & BILLF_CLOSEDORDER)) {
							THROW(ok = P_BObj->WriteOffDraft(bill_id, 0, &deficit_list, 0));
							if(ok == -2) {
								if(!is_deficit)
									single_op_id_on_deficit = bill_rec.OpID;
								else if(single_op_id_on_deficit && bill_rec.OpID != single_op_id_on_deficit)
									single_op_id_on_deficit = 0;
								is_deficit++;
							}
						}
						PPWaitPercent(i+1, idlist.getCount());
					}
					THROW(tra.Commit());
				}
				PPWait(0);
			}
			else if(s == 2) { // Перенести теги с драфт-документа на документ списания
				if(P_BObj->Search(id, &bill_rec) > 0) {
					if(!(bill_rec.Flags & BILLF_WRITEDOFF)) {
						PPMessage(mfInfo|mfOK, PPINF_DRAFTNOTWROFF);
					}
					else {
						PPIDArray wroff_bill_list;
						BillTbl::Rec wroff_bill_rec;
						for(DateIter diter; P_BObj->P_Tbl->EnumLinks(bill_rec.ID, &diter, BLNK_WROFFDRAFT, &wroff_bill_rec) > 0;)
							wroff_bill_list.add(wroff_bill_rec.ID);
						if(wroff_bill_list.getCount() == 1) {
							SString temp_buf;
							SString bill_text;
							PPBillPacket _this_bp;
							PPBillPacket _link_bp;
							const PPID   _link_id = wroff_bill_list.get(0);
							// @v10.2.9 StringSet _this_lxc_ss;
							// @v10.2.9 StringSet _link_lxc_ss;
							PPLotExtCodeContainer::MarkSet _this_lxc_set; // @v10.2.9 
							PPLotExtCodeContainer::MarkSet _link_lxc_set; // @v10.2.9 
							int    do_update = 0;
							THROW(P_BObj->ExtractPacketWithFlags(id, &_this_bp, BPLD_FORCESERIALS) > 0);
							THROW(P_BObj->ExtractPacketWithFlags(_link_id, &_link_bp, BPLD_FORCESERIALS) > 0);
							PPObjBill::MakeCodeString(&_link_bp.Rec, PPObjBill::mcsAddOpName, bill_text);
							for(uint tbpi = 0; tbpi < _this_bp.GetTCount(); tbpi++) {
								const PPTransferItem & r_ti = _this_bp.ConstTI(tbpi);
								if(r_ti.RByBill > 0) {
									const ObjTagList * p_tl = _this_bp.LTagL.Get(tbpi);
									uint  _lp = 0;
									if(p_tl && p_tl->GetCount() && _link_bp.SearchTI(r_ti.RByBill, &_lp)) {
										int    do_update_local = 0;
										const  PPTransferItem & r_link_ti = _link_bp.ConstTI(_lp);
										ObjTagList * p_link_tl = _link_bp.LTagL.Get(_lp);
										ObjTagList _link_tl;
										RVALUEPTR(_link_tl, p_link_tl);
										for(uint tli = 0; tli < p_tl->GetCount(); tli++) {
											const ObjTagItem * p_tag = p_tl->GetItemByPos(tli);
											if(p_tag && !p_tag->IsZeroVal()) {
												const PPID tag_id = p_tag->TagID;
												if(tag_id) {
													const ObjTagItem * p_ex_link_tag = _link_tl.GetItem(tag_id);
													if(!p_ex_link_tag || *p_ex_link_tag != *p_tag) {
														_link_tl.PutItem(p_tag->TagID, p_tag);
														do_update_local = 1;
													}
												}
											}
										}
										if(do_update_local) {
											_link_bp.LTagL.Set(_lp, &_link_tl);
											do_update = 1;
										}
									}
									// @v10.2.7 {
									if(_this_bp.XcL.Get(tbpi+1, 0, _this_lxc_set) > 0) {
										_link_bp.XcL.Get(_lp+1, 0, _link_lxc_set);
										if(_link_lxc_set.GetCount() == 0) {
											/* @v10.2.9 for(uint thislxcssp = 0; _this_lxc_ss.get(&thislxcssp, temp_buf);) {
												_link_bp.XcL.Add(_lp+1, temp_buf, 0);
												do_update = 1;
											}*/
											// @v10.2.9 {
											PPLotExtCodeContainer::MarkSet::Entry lxentry;
											for(uint thislxidx = 0; thislxidx < _this_lxc_set.GetCount(); thislxidx++) {
												if(_this_lxc_set.GetByIdx(thislxidx, lxentry)) {
													_link_bp.XcL.Add(_lp+1, lxentry.BoxID, (int16)lxentry.Flags, lxentry.Num, 0);
													do_update = 1; // @v10.2.10 @fix
												}
											}
											// } @v10.2.9 
										}
									}
									// } @v10.2.7 
								}
							}
							if(do_update) {
								THROW(P_BObj->UpdatePacket(&_link_bp, 1));
								PPMessage(mfInfo|mfOK, PPINF_TAGSINWROFFBILLUPD, bill_text);
							}
							else {
								PPMessage(mfInfo|mfOK, PPINF_TAGSINWROFFBILLNUPD, bill_text);
							}
						}
						else if(wroff_bill_list.getCount() > 1) {
							; // Не понятно что делать - не делаем ничего
						}
					}
				}
			}
			try_again = 0;
			if(is_deficit) {
				deficit_list.ClearActions();
				if(s == 0) {
					P_BObj->P_OpObj->GetDraftExData(bill_rec.OpID, &doe);
					deficit_list.AddAction(PCUG_BALANCE);
					deficit_list.AddAction(PCUG_CANCEL);
					PPOprKind wroff_op_rec;
					if(doe.Flags & DROXF_SELSUPPLONCOMPL && GetOpType(doe.WrOffComplOpID, &wroff_op_rec) == PPOPT_GOODSRECEIPT) {
						deficit_list.SupplAccSheetForSubstID = wroff_op_rec.AccSheetID;
					}
				}
				deficit_list.OPcug = PCUG_CANCEL;
				ProcessUnsuffisientList(DLG_MSGNCMPL5, &deficit_list);
				if(deficit_list.OPcug == PCUG_BALANCE) {
					PPID   compens_op_id = doe.WrOffComplOpID;
					if(GetOpType(compens_op_id) == PPOPT_GOODSMODIF)
						compens_op_id = CConfig.ReceiptOp;
					THROW(P_BObj->ProcessDeficit(compens_op_id, 0, &deficit_list, 0, 1));
					deficit_list.Clear();
					is_deficit = 0;
					try_again  = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
struct Bill2MrpParam {
	enum {
		fAllSelection = 0x0001 // Создать MRP-таблицу по всей выборке
	};
	char   MrpName[48];
	long   Flags;
};

static int SLAPI EditBill2MrpParam(Bill2MrpParam * pParam)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_BILL2MRP);
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = BIN(pParam->Flags & Bill2MrpParam::fAllSelection);
		dlg->setCtrlData(CTL_BILL2MRP_WHAT, &v);
		dlg->setCtrlData(CTL_BILL2MRP_MRPNAME, pParam->MrpName);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			v = dlg->getCtrlUInt16(CTL_BILL2MRP_WHAT);
			SETFLAG(pParam->Flags, Bill2MrpParam::fAllSelection, v == 1);
			dlg->getCtrlData(CTL_BILL2MRP_MRPNAME, pParam->MrpName);
			ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PPViewBill::CreateMrpTab(PPID billID)
{
	int    ok = -1;
	Bill2MrpParam param;
	MEMSZERO(param);
	if((Filt.Flags & (BillFilt::fOrderOnly | BillFilt::fDraftOnly)) && EditBill2MrpParam(&param) > 0) {
		PPObjMrpTab mrp_obj;
		MrpTabPacket mrp_pack;

		PPIDArray bill_list;
		PPWait(1);
		if(param.Flags & Bill2MrpParam::fAllSelection) {
			THROW(GetBillIDList(&bill_list));
			mrp_pack.Init(PPOBJ_BILL, 0, param.MrpName);
		}
		else {
			bill_list.add(billID);
			mrp_pack.Init(PPOBJ_BILL, billID, param.MrpName);
		}
		THROW(P_BObj->CreateMrpTab(&bill_list, &mrp_pack, 0, 1));
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::EditFilt(BillFilt * pFilt, long extraParam) const
{
	BrowseBillsType bbt = (BrowseBillsType)extraParam;

	int    ok = -1, caption = -1;
	TDialog * d = 0;
	PPID   single_op_id = 0;
	PPIDArray op_list;
	uint   rez_id = DLG_BILLFLT;
	if(oneof3(bbt, bbtOrderBills, bbtInventoryBills, bbtPoolBills)) {
		uint f;
		if(bbt == bbtOrderBills) {
			f = BillFilt::fOrderOnly;
			caption = 1;
			rez_id = DLG_BILLFLT_ORDER;
		}
		else if(bbt == bbtInventoryBills) {
			f = BillFilt::fInvOnly;
			caption = 3;
			rez_id = DLG_INVFLT;
		}
		else if(bbt == bbtPoolBills) {
			f = BillFilt::fPoolOnly;
			caption = 4;
		}
		else
			return -1;
		pFilt->Flags |= f;
	}
	else if(bbt == bbtAccturnBills) {
		pFilt->Flags |= BillFilt::fAccturnOnly;
		caption = 2;
	}
	else if(bbt == bbtDraftBills) {
		pFilt->Flags |= BillFilt::fDraftOnly;
		caption = 5;
		rez_id = DLG_BILLFLT_DRAFT;
	}
	else
		caption = 0;
	GetOpList(pFilt, &op_list, &single_op_id);
	SString temp_buf;
	if(op_list.getCount()) {
		if(single_op_id)
			pFilt->OpID = single_op_id;
		if(caption >= 0)
			PPGetSubStr(PPTXT_BILLFLTCAPTIONS, caption, temp_buf);
		else
			temp_buf.Z();
		int r = BillFilterDialog(rez_id, pFilt, &d, temp_buf);
		ZDELETE(d);
		if(r == cmOK)
			ok = 1;
		else if(r == 0)
			ok = 0;
	}
	else {
		int    bill_idx = -1;
		if(bbt == bbtOrderBills)
			bill_idx = PPNOTUNE_ORDER;
		else if(bbt == bbtInventoryBills)
			bill_idx = PPNOTUNE_INVENT;
		else if(bbt == bbtDraftBills)
			bill_idx = PPNOTUNE_DRAFT;
		if(bill_idx >= 0) {
			PPGetSubStr(PPTXT_NOTUNEANYOPS, bill_idx, temp_buf);
			PPOutputMessage(temp_buf, mfInfo);
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw) {
		PPViewBill * p_view = (PPViewBill *)p_brw->P_View;
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewBill::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < (long)p_def->getCount()) {
			const BroColumn & r_col = p_def->at(col);
			BillTbl::Rec bill_rec;
			PPViewBill::BrwHdr * p_hdr = (PPViewBill::BrwHdr *)pData;
			if(p_hdr->ID) {
				if(r_col.OrgOffs == 0) { // ID
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0) {
						if(bill_rec.Flags2 & BILLF2_FULLSYNC && PPMaster) {
							pStyle->Color2 = GetColorRef(SClrDodgerblue);
							pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
							ok = 1;
						}
					}
				}
				else if(r_col.OrgOffs == 2) { // BillNo
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0) {
						const TagFilt & r_tag_filt = P_BObj->GetConfig().TagIndFilt;
						if(!r_tag_filt.IsEmpty()) {
							SColor clr;
							if(r_tag_filt.SelectIndicator(p_hdr->ID, clr) > 0) {
								pStyle->Color2 = (COLORREF)clr;
								pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
						}
						if(P_BObj->GetConfig().Flags & BCF_PAINTSHIPPEDBILLS) {
							if(bill_rec.Flags & BILLF_SHIPPED) {
								pStyle->Color = LightenColor(GetColorRef(SClrBlue), 0.7f);
								ok = 1;
							}
						}
						if(bill_rec.Flags2 & BILLF2_BHT) {
							pStyle->Color = GetColorRef(SClrLime);
							pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
							ok = 1;
						}
					}
				}
				else if(r_col.OrgOffs == 4) { // Memo
					SString & r_memos = SLS.AcquireRvlStr(); // @v10.0.01
					if(P_BObj->FetchExtMemo(p_hdr->ID, r_memos) > 0) {
						pStyle->Color = GetColorRef(SClrDarkgreen);
						pStyle->Flags = BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
				else if(r_col.OrgOffs == 6) { // Contragent
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0 && bill_rec.Object) {
						ArticleTbl::Rec ar_rec;
						if(ArObj.Fetch(bill_rec.Object, &ar_rec) > 0 && ar_rec.Flags & ARTRF_STOPBILL) {
							pStyle->Color = GetColorRef(SClrCoral);
							pStyle->Flags = BrowserWindow::CellStyle::fCorner;
							ok = 1;
						}
					}
				}
				else if(r_col.OrgOffs == 10) { // Status
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0) {
						const PPID op_type_id = GetOpType(bill_rec.OpID);
						if(oneof3(op_type_id, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
							if(bill_rec.Flags2 & BILLF2_DECLINED) {
								pStyle->Color2 = GetColorRef(SClrGrey);
								pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
							else if(bill_rec.Flags & BILLF_WRITEDOFF) {
								pStyle->Color2 = GetColorRef(SClrOrange);
								pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
						}
						// @v10.0.01 {
						else if(op_type_id == PPOPT_GOODSORDER) {
							if(bill_rec.Flags & BILLF_CLOSEDORDER) {
								pStyle->Color2 = GetColorRef(SClrOrange);
								pStyle->Flags |= BrowserWindow::CellStyle::fLeftBottomCorner;
								ok = 1;
							}
						}
						// } @v10.0.01
						{
							const int edi_user_status = P_BObj->GetEdiUserStatus(bill_rec);
							if(edi_user_status) {
								COLORREF edi_color = 0;
								switch(edi_user_status) {
									case BEDIUS_DESADV_OUT_SENDED: 	edi_color = GetColorRef(SClrYellow); break;
									case BEDIUS_DESADV_OUT_PROCESSED: edi_color = GetColorRef(SClrOrange); break;
									case BEDIUS_DESADV_OUT_RECADV_ACC: edi_color = GetColorRef(SClrGreen); break;
									case BEDIUS_DESADV_OUT_RECADV_PACC: edi_color = GetColorRef(SClrLightgreen); break;
									case BEDIUS_DESADV_OUT_RECADV_REJ: edi_color = GetColorRef(SClrBrown); break;
									case BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_ACC: edi_color = GetColorRef(SClrBlue); break;
									case BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_PACC: edi_color = GetColorRef(SClrLightblue); break;
									case BEDIUS_DESADV_OUT_RECADV_CONF_ACC_ON_REJ: edi_color = GetColorRef(SClrViolet); break;
									case BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_ACC:
									case BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_PACC:
									case BEDIUS_DESADV_OUT_RECADV_CONF_REJ_ON_REJ: edi_color = GetColorRef(SClrDarkgrey); break;
									case BEDIUS_DESADV_IN_ACCEPTED: edi_color = GetColorRef(SClrOrangered); break;
									case BEDIUS_DESADV_IN_RECADV_ACC: edi_color = GetColorRef(SClrGreen); break;
									case BEDIUS_DESADV_IN_RECADV_PACC: edi_color = GetColorRef(SClrLightgreen); break;
									case BEDIUS_DESADV_IN_RECADV_REJ: edi_color = GetColorRef(SClrBrown); break;
									default: edi_color = GetColorRef(SClrPink); break;
								}
								pStyle->RightFigColor = edi_color;
								pStyle->Flags |= BrowserWindow::CellStyle::fRightFigCircle;
								ok = 1;
							}
						}
						// @v10.2.4 {
						if(bill_rec.StatusID) {
							PPBillStatus bs_rec;
							PPObjBillStatus bs_obj;
							if(bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0 && !bs_rec.IndColor.IsEmpty()) {
								pStyle->Color = bs_rec.IndColor;
								ok = 1;
							}
						}
						// } @v10.2.4
					}
				}
			}
		}
	}
	return ok;
}

//virtual
void SLAPI PPViewBill::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    caption = 0;
	SString title, sub_title, temp_buf;
	if(pBrw) {
		const BrowserDef * p_def = pBrw->getDef();
		int    show_debt = 0;
		PPID   single_loc_id = LocList_.getSingle();
		if(Filt.Flags & BillFilt::fOrderOnly)
			show_debt = BIN(Filt.Flags & BillFilt::fShowDebt);
		else
			show_debt = BIN(Filt.Flags & (BillFilt::fShowDebt | BillFilt::fDebtOnly));
		pBrw->options |= (ofCenterX | ofCenterY);
		if(Filt.Flags & BillFilt::fCashOnly) {
			const PPConfig & r_cfg = LConfig;
			GetObjectName(PPOBJ_CASHNODE, r_cfg.Cash, sub_title, 1);
			sub_title.CatDivIfNotEmpty('-', 1).Cat(r_cfg.OperDate);
		}
		else {
			if(!Filt.Period.IsZero())
				sub_title.CatDivIfNotEmpty('-', 1).Cat(Filt.Period, 1);
			if(single_loc_id) {
				GetLocationName(single_loc_id, temp_buf);
				sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(Filt.OpID) {
				GetOpName(Filt.OpID, temp_buf);
				sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(Filt.AccSheetID) {
				GetObjectName(PPOBJ_ACCSHEET, Filt.AccSheetID, temp_buf);
				sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(Filt.ObjectID) {
				GetArticleName(Filt.ObjectID, temp_buf);
				sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(Filt.CurID > 0 && !(Filt.Flags & BillFilt::fAllCurrencies))
				sub_title.CatDivIfNotEmpty('-', 1).Cat(GetCurSymbText(Filt.CurID, temp_buf));
			if(!(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)) {
				uint   loc_col  = 1;
				uint   bs_col   = 2;
				uint   debt_col = 4;
				if(!single_loc_id) {
					// @v9.0.2 pBrw->InsColumnWord(loc_col, PPWORD_WAREHOUSE, 7, 0, MKSFMT(10, 0), 0);
					pBrw->InsColumn(loc_col, "@warehouse", 7, 0, MKSFMT(10, 0), 0); // @v9.0.2
					debt_col++;
					bs_col++;
				}
				{
					PPObjBillStatus bs_obj;
					PPID   bs_id = 0;
					if(bs_obj.EnumItems(&bs_id, 0) > 0) {
						//
						// Определен по крайней мере один статус документа,
						// следовательно в таблице нужна колонка статуса документа
						//
						// @v9.1.11 pBrw->InsColumnWord(bs_col, PPWORD_STATUS, 10, 0, MKSFMT(6, 0), 0);
						pBrw->InsColumn(bs_col, "@status", 10, 0, MKSFMT(6, 0), 0); // @v9.1.11
						debt_col++;
					}
				}
				if(show_debt)
					pBrw->InsColumn(debt_col, "@debt", 9, 0, MKSFMTD(10, 2, NMBF_NOZERO), 0);
			}
			{
				int  de_col = 1;
				if(PPMaster) {
					pBrw->InsColumn(0, "@id", 0, 0, 0, 0);
					de_col++;
				}
				if(Filt.Dl.GetItemByDataId(BillFilt::dliDueDate, 0)) { // #13
					pBrw->InsColumn(de_col++, "@duedate", 13, 0, MKSFMT(0, DATF_DMY), 0);
				}
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightIssueDate, 0)) { // #11
					pBrw->InsColumn(de_col++, "@issuedate", 11, 0, MKSFMT(0, DATF_DMY), 0);
				}
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightArrivalDate, 0)) { // #12
					pBrw->InsColumn(de_col++, "@arrivaldate", 12, 0, MKSFMT(0, DATF_DMY), 0);
				}
			}
			if(p_def) {
				int    next_pos = -1;
				const int org_offs_ar = 6;
				const int org_offs_memo = 4;
				int    ar_pos = -1;
				int    memo_pos = -1;
				for(uint i = 0; i < p_def->getCount(); i++) {
					if(p_def->at(i).OrgOffs == org_offs_ar)
						ar_pos = i;
					else if(p_def->at(i).OrgOffs == org_offs_memo)
						memo_pos = i;
				}
				if(ar_pos >= 0)
					next_pos = ar_pos+1;
				else if(memo_pos >= 0)
					next_pos = memo_pos;
				if(next_pos >= 0) {
					if(Filt.Dl.GetItemByDataId(BillFilt::dliAgentName, 0)) { // #14
						pBrw->InsColumn(next_pos++, "@agent", 14, 0, 0, 0);
					}
					if(Filt.Dl.GetItemByDataId(BillFilt::dliDlvrAddr, 0)) { // #16
						pBrw->InsColumn(next_pos++, "@daddress", 16, 0, 0, 0);
					}
					if(Filt.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0) && P_Arp) { // #15
						pBrw->InsColumn(next_pos++, "@alcolic", 15, 0, 0, 0);
					}
				}
			}
		}
		pBrw->Advise(PPAdviseBlock::evBillChanged, 0, PPOBJ_BILL, 0);
	}
	SetExtToolbar(0);
	if(Filt.Flags & BillFilt::fAsSelector) {
		PPGetWord(PPWORD_SELBILL, 0, sub_title);
		if(Filt.Sel) {
			PPID   temp_id = Filt.Sel;
			pBrw->search2(&temp_id, CMPF_LONG, srchFirst, 0);
		}
	}
	if(Filt.PoolBillID && Filt.AssocID)
		caption = 6;
	else if(Filt.Flags & BillFilt::fOrderOnly) {
		SetExtToolbar(TOOLBAR_ORDERBILL);
		caption = 1;
	}
	else if(Filt.Flags & BillFilt::fAccturnOnly) {
		SetExtToolbar(TOOLBAR_ACCBILL);
		caption = 2;
	}
	else if(Filt.Flags & BillFilt::fInvOnly) {
		SetExtToolbar(TOOLBAR_INVENTBILL);
		caption = 3;
	}
	else if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)
		caption = 4;
	else if(Filt.Flags & BillFilt::fPoolOnly)
		caption = 5;
	else if(Filt.Flags & BillFilt::fDraftOnly) {
		SetExtToolbar(TOOLBAR_DRAFTBILL);
		caption = 7;
	}
	if(GetOuterTitle(0)) {
		GetOuterTitle(&title);
		pBrw->setOrgTitle(title);
	}
	else {
		if(caption >= 0 && PPGetSubStr(PPTXT_BILLBRWCAPTIONS, caption, title))
			pBrw->setOrgTitle(title);
		pBrw->setSubTitle(sub_title);
	}
	pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
}

DBQuery * SLAPI PPViewBill::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	const int use_omt_paymamt = BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	PPID   single_loc_id = LocList_.getSingle();
	BillTbl       * bll  = 0;
	TempBillTbl   * bllt = 0;
	TempOrderTbl  * ordt = 0;
	BillAmountTbl * t_amt  = 0;
	DBQuery       * q    = 0;
	DBQ  * dbq  = 0;
	DBE    dbe_debt;
	DBE    dbe_status;
	DBE    dbe_oprkind;
	DBE    dbe_loc;
	DBE    dbe_ar;
	DBE    dbe_cur;
	DBE    dbe_chkusr;
	DBE    dbe_issuedate;
	DBE    dbe_arrvldate;
	DBE    dbe_agentname;
	DBE    dbe_licreg;
	DBE    dbe_dlvraddr;
	DBE    dbe_strgloc;
	uint   brw_id = 0;
	int    pool_op = 0;
	int    tbl_count = 0;
	DBTable * tbl_l[12];
	memzero(tbl_l, sizeof(tbl_l));
	CALLPTRMEMB(pSubTitle, Z());
	if(P_TempTbl) {
		bllt = new TempBillTbl(P_TempTbl->GetName());
		THROW(CheckTblPtr(bllt));
	}
	if(P_TempOrd && oneof2(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject)) {
		ordt = new TempOrderTbl(P_TempOrd->GetName());
		THROW(CheckTblPtr(ordt));
	}
	THROW(CheckTblPtr(bll = new BillTbl));
	if(Filt.Flags & BillFilt::fCashOnly) {
		const PPConfig & r_cfg = LConfig;
		q = &select(
			bll->ID,     // #0
			bll->Dt,     // #1
			bll->Code,   // #2
			bll->Amount, // #3
			0L).from(bll, 0L).where(bll->Dt == r_cfg.OperDate && bll->UserID == r_cfg.Cash &&
			(bll->Flags & BILLF_CASH) == BILLF_CASH).orderBy(bll->Dt, bll->BillNo, 0L);
		brw_id = BROWSER_CASHNODEBILL;
	}
	else {
		PPDbqFuncPool::InitObjNameFunc(dbe_oprkind, PPDbqFuncPool::IdObjNameOprKind, bll->OpID);
		if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) {
			q = &select(
				bllt->BillID, // #0
				bllt->Dt,     // #1
				bll->Code,    // #2
				dbe_oprkind,  // #3
				bllt->Debit,  // #4
				bllt->Credit, // #5
				bll->Memo,    // #6
				0L).from(bllt, bll, 0L).where(bll->ID == bllt->BillID).orderBy(bllt->Dt, bllt->BillNo, 0L);
			brw_id = BROWSER_DEBTCARD;
		}
		else {
			dbe_debt.init();
			if(Filt.Flags & BillFilt::fShowDebt) {
				dbe_debt.push(bll->Amount);
				if(Filt.PaymPeriod.IsZero() || bllt == 0) {
					if(use_omt_paymamt) {
						dbe_debt.push(bll->PaymAmount);
					}
					else {
						THROW(CheckTblPtr(t_amt = new BillAmountTbl));
						dbe_debt.push(t_amt->Amount);
					}
				}
				else
					dbe_debt.push(bllt->Credit);
				dbe_debt.push(bll->Flags);
				dbe_debt.push(bll->OpID);
				dbe_debt.push((DBFunc)PPDbqFuncPool::IdBillDebt);
			}
			else
				dbe_debt.push((DBFunc)PPDbqFuncPool::IdEmpty);
			PPDbqFuncPool::InitObjNameFunc(dbe_status, PPDbqFuncPool::IdObjNameBillStatus, bll->StatusID);
			PPDbqFuncPool::InitObjNameFunc(dbe_ar,  PPDbqFuncPool::IdObjNameAr,       bll->Object);
			PPDbqFuncPool::InitObjNameFunc(dbe_cur, PPDbqFuncPool::IdObjSymbCurrency, bll->CurID);
			PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc,      bll->LocID);
			if(ordt)
				tbl_l[tbl_count++] = ordt;
			if(bllt)
				tbl_l[tbl_count++] = bllt;
			tbl_l[tbl_count++] = bll;
			q = &select(bll->ID, 0);  // #0
			q->addField(bll->Dt);     // #1
			q->addField(bll->Code);   // #2
			q->addField(bll->Amount); // #3
			q->addField(bll->Memo);   // #4
			q->addField(dbe_oprkind); // #5
			q->addField(dbe_ar);      // #6
			q->addField(dbe_loc);     // #7
			q->addField(dbe_cur);     // #8
			if(Filt.Bbt == bbtInventoryBills)
				brw_id = BROWSER_INVENTORYBILL;
			else if(Filt.Flags & BillFilt::fAllCurrencies)
				brw_id = BROWSER_GOODSBILLCUR;
			else
				brw_id = BROWSER_GOODSBILL;
			q->addField(dbe_debt);          // #9
			q->addField(dbe_status);        // #10
			{
				dbe_issuedate.init();
				dbe_arrvldate.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightIssueDate, 0)) {
					dbe_issuedate.push(bll->ID);
					dbe_issuedate.push((DBFunc)PPDbqFuncPool::IdBillFrghtIssueDt);
				}
				else {
					dbe_issuedate.push((DBFunc)PPDbqFuncPool::IdEmpty);
				}
				q->addField(dbe_issuedate); // #11
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightArrivalDate, 0)) {
					dbe_arrvldate.push(bll->ID);
					dbe_arrvldate.push((DBFunc)PPDbqFuncPool::IdBillFrghtArrvlDt);
				}
				else {
					dbe_arrvldate.push((DBFunc)PPDbqFuncPool::IdEmpty);
				}
				q->addField(dbe_arrvldate); // #12
			}
			q->addField(bll->DueDate);      // #13
			{
				dbe_agentname.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliAgentName, 0)) {
					dbe_agentname.push(bll->ID);
					dbe_agentname.push((DBFunc)PPDbqFuncPool::IdBillAgentName);
				}
				else
					dbe_agentname.push((DBFunc)PPDbqFuncPool::IdEmpty);
				q->addField(dbe_agentname); // #14
			}
			{
				dbe_licreg.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0) && P_Arp && bllt) {
					dbe_licreg.push(bllt->LicRegID);
					dbe_licreg.push((DBFunc)PPDbqFuncPool::IdRegisterText);
				}
				else
					dbe_licreg.push((DBFunc)PPDbqFuncPool::IdEmpty);
				q->addField(dbe_licreg); // #15
			}
			{
				dbe_dlvraddr.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliDlvrAddr, 0)) {
					dbe_dlvraddr.push(bll->ID);
					dbe_dlvraddr.push((DBFunc)PPDbqFuncPool::IdBillFrghtDlvrAddr);
				}
				else
					dbe_dlvraddr.push((DBFunc)PPDbqFuncPool::IdEmpty);
				q->addField(dbe_dlvraddr); // #16
			}
			{
				dbe_strgloc.init();
				if(Filt.Bbt == bbtInventoryBills) {
					dbe_strgloc.push(bll->ID);
					dbe_strgloc.push((DBFunc)PPDbqFuncPool::IdBillFrghtStrgLoc);
				}
				else
					dbe_strgloc.push((DBFunc)PPDbqFuncPool::IdEmpty);
				q->addField(dbe_strgloc); // #17
			}
			tbl_l[tbl_count++] = t_amt;
			q->from(tbl_l[0], tbl_l[1], tbl_l[2], tbl_l[3], tbl_l[4], tbl_l[5], tbl_l[6], tbl_l[7], 0L);
			//
			// Restrictions {
			//
			dbq = & daterange((bllt ? bllt->Dt : bll->Dt), &Filt.Period);
			dbq = & (*dbq && daterange((bllt ? bllt->DueDate : bll->DueDate), &Filt.DuePeriod));
			if(ordt)
				dbq = & (*dbq && bll->ID == ordt->ID);
			if(bllt)
				dbq = & (*dbq && bll->ID == bllt->BillID);
			if(!Filt.OpID && Filt.PoolOpID && !Filt.PoolBillID && P_BPOX && P_BPOX->OpList.getCount()) {
				dbq = &(*dbq && ppidlist(bll->OpID, &P_BPOX->OpList));
				pool_op = 1;
			}
			if(!pool_op && !IdList.IsExists())
				dbq = & (*dbq && ppidlist(bll->OpID, &OpList));
			if(LocList_.getCount()) {
				PPIDArray temp_loc_list = LocList_;
				if(Filt.Flags & BillFilt::fAddZeroLoc || temp_loc_list.getCount() > 1)
					temp_loc_list.addUnique(0L);
				dbq = & (*dbq && ppidlist(bll->LocID, &temp_loc_list));
			}
			if(Filt.ObjectID && !(Filt.Flags & BillFilt::fDebtsWithPayments))
				dbq = & (*dbq && bll->Object == Filt.ObjectID);
			if(!(Filt.Flags & BillFilt::fAllCurrencies) && Filt.CurID >= 0 && !IdList.IsExists())
				dbq = & (*dbq && bll->CurID == Filt.CurID);
			dbq = & (*dbq && realrange(bll->Amount, Filt.AmtRange.low, Filt.AmtRange.upp));
			if(Filt.Flags & BillFilt::fShowDebt && t_amt) // При use_omt_paymamt t_amt==0 автоматически
				dbq = & (*dbq && (t_amt->BillID += bll->ID) && t_amt->AmtTypeID == PPAMT_PAYMENT);
			dbq = ppcheckfiltid(dbq, bll->Object2, Filt.Object2ID);
			dbq = ppcheckfiltid(dbq, bll->StatusID, Filt.StatusID);
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_PAYOUT,  (Filt.Flags & BillFilt::fDebtOnly) ? -1 : 0);
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_FREIGHT, (Filt.Flags & BillFilt::fFreightedOnly) ? 1 : 0);
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_SHIPPED, (Filt.Flags & BillFilt::fUnshippedOnly) ? -1 : BIN(Filt.Flags & BillFilt::fShippedOnly));
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_RMVEXCISE, Filt.Ft_STax);
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_CLOSEDORDER, Filt.Ft_ClosedOrder);
			dbq = ppcheckflag(dbq, bll->Flags2, BILLF2_DECLINED, Filt.Ft_Declined);
			// @v9.1.6 {
			{
				DBE * p_dbe_1 = 0;
				DBE * p_dbe_2 = 0;
				if(Filt.EdiRecadvStatus) {
					p_dbe_1 = &(bll->Flags2 & BILLF2_RECADV_ACCP);
					p_dbe_2 = &(bll->Flags2 & BILLF2_RECADV_DECL);
					switch(Filt.EdiRecadvStatus) {
						case PPEDI_RECADV_STATUS_ACCEPT:
							dbq = &(*dbq && *p_dbe_1 == BILLF2_RECADV_ACCP && *p_dbe_2 == 0L);
							break;
						case PPEDI_RECADV_STATUS_PARTACCEPT:
							dbq = &(*dbq && *p_dbe_1 == BILLF2_RECADV_ACCP && *p_dbe_2 == BILLF2_RECADV_DECL);
							break;
						case PPEDI_RECADV_STATUS_REJECT:
							dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == BILLF2_RECADV_DECL);
							break;
						case -1:
							dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == 0L);
							break;
					}
				}
				if(Filt.EdiRecadvConfStatus) {
					p_dbe_1 = &(bll->Flags2 & BILLF2_EDIAR_AGR);
					p_dbe_2 = &(bll->Flags2 & BILLF2_EDIAR_DISAGR);
					switch(Filt.EdiRecadvConfStatus) {
						case PPEDI_RECADVCONF_STATUS_ACCEPT:
							dbq = &(*dbq && *p_dbe_1 == BILLF2_EDIAR_AGR && *p_dbe_2 == 0L);
							break;
						case PPEDI_RECADVCONF_STATUS_REJECT:
							dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == BILLF2_EDIAR_DISAGR);
							break;
						case -1:
							dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == 0L);
							break;
					}
				}
				delete p_dbe_1;
				delete p_dbe_2;
			}
			// } @v9.1.6
			if(Filt.CreatorID & PPObjSecur::maskUserGroup) {
				dbe_chkusr.init();
				dbe_chkusr.push(bll->UserID);
				DBConst dbc_long;
				dbc_long.init(Filt.CreatorID);
				dbe_chkusr.push(dbc_long);
				dbe_chkusr.push((DBFunc)PPDbqFuncPool::IdCheckUserID);
				dbq = & (*dbq && dbe_chkusr == (long)1);
			}
			else
				dbq = ppcheckfiltid(dbq, bll->UserID, Filt.CreatorID);
			if(!IdList.IsExists()) {
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_NEEDPAYMENT,   BIN(Filt.Flags & BillFilt::fPaymNeeded));
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_WHITELABEL,    BIN(Filt.Flags & BillFilt::fLabelOnly));
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_TOTALDISCOUNT, BIN(Filt.Flags & BillFilt::fDiscountOnly));
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_CHECK,         BIN(Filt.Flags & BillFilt::fCcPrintedOnly)); // @v9.7.12
			}
			q->where(*dbq);
			//
			// } Restrictions
			//
			if(ordt && oneof2(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject))
				q->orderBy(ordt->Name, 0L);
			else if(bllt)
				q->orderBy(bllt->Dt, bllt->BillNo, 0L);
			else if(Filt.ObjectID)
				q->orderBy(bll->Object, bll->Dt, bll->BillNo, 0L);
			else if(SingleOpID && GetOpType(Filt.OpID) != PPOPT_POOL)
				q->orderBy(bll->OpID, bll->Dt, bll->BillNo, 0L);
			else if(LocList_.getSingle() && !(Filt.Flags & BillFilt::fAddZeroLoc))
				q->orderBy(bll->LocID, bll->Dt, bll->BillNo, 0L);
			else
				q->orderBy(bll->Dt, bll->BillNo, 0L);
		}
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete bllt;
			delete ordt;
			delete bll;
			delete t_amt;
			delete dbq;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}
//
//
//
int SLAPI PPViewBill::SendAddBillMessage(PPID id)
{
	if(id && P_BObj->Cfg.Flags & BCF_WARNADDBILLNOFLT)
		return (CheckIDForFilt(id, 0) > 0) ? -1 : (PPMessage(mfInfo|mfOK, PPINF_ADDBILLNOSHOWBYFLT), 1);
	else
		return -1;
}

int SLAPI PPViewBill::AddItem(PPID * pID, PPID opID)
{
	PPID   id = 0;
	if(Filt.Flags & BillFilt::fEditPoolByType)
		return AddBillToPool();
	else {
		const PPConfig & r_cfg = LConfig;
		int    r = -1;
		PPID   op_id = opID;
		PPID   loc_id = r_cfg.Location;
	 	const  PPID save_loc_id = loc_id;
		if(!(Filt.DenyFlags & BillFilt::fDenyAdd)) {
			PPOprKind op_rec;
			if(Filt.Flags & BillFilt::fSetupNewBill) {
				if(Filt.OpID)
					GetOpData(Filt.OpID, &op_rec);
				if(IsGenericOp(Filt.OpID) == 0 && op_rec.LinkOpID == 0)
					op_id = Filt.OpID;
			}
			if(op_id == 0) {
				if(r_cfg.Cash) {
					if(op_id == 0 || op_id != GetCashRetOp())
						op_id = GetCashOp();
				}
				else {
					PPIDArray op_type_list;
					if(Filt.AccSheetID) {
						while(EnumOperations(0, &op_id, &op_rec) > 0)
							if(Filt.AccSheetID == op_rec.AccSheetID)
								op_type_list.add(op_id);
						op_id = 0;
					}
					else if(Filt.Flags & BillFilt::fAccturnOnly)
						op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_AGREEMENT, 0L); // @v10.1.12 PPOPT_AGREEMENT
					else if(Filt.Flags & BillFilt::fOrderOnly)
						op_type_list.add(PPOPT_GOODSORDER);
					else if(Filt.Flags & BillFilt::fInvOnly)
						op_type_list.add(PPOPT_INVENTORY);
					else if(Filt.Flags & BillFilt::fPoolOnly)
						op_type_list.add(PPOPT_POOL);
					else if(Filt.Flags & BillFilt::fDraftOnly)
						op_type_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, 0L);
					else if(Filt.Flags & BillFilt::fWmsOnly)
						op_type_list.addzlist(PPOPT_WAREHOUSE, 0L);
					else
						op_type_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, 0L);
					if(BillPrelude(&op_type_list, (Filt.AccSheetID ? OPKLF_OPLIST : 0), 0, &op_id, &loc_id) > 0)
						DS.SetLocation(loc_id);
					else
						op_id = 0;
				}
			}
			if(op_id > 0) {
				PPObjBill::AddBlock ab;
				ab.OpID = op_id;
				if(GetOpType(op_id) == PPOPT_ACCTURN && !CheckOpFlags(op_id, OPKF_EXTACCTURN))
					r = P_BObj->AddGenAccturn(&id, op_id, 0);
				else if(Filt.Flags & BillFilt::fSetupNewBill)
					r = P_BObj->AddGoodsBillByFilt(&id, &Filt, op_id);
				else
					r = P_BObj->AddGoodsBill(&id, &ab);
			}
			else
				r = -1;
		}
		DS.SetLocation(save_loc_id);
		ASSIGN_PTR(pID, id);
		return (r == cmOK) ? (SendAddBillMessage(id), 1) : -1;
	}
}

static int SLAPI SelectAddByRcptAction(SelAddBySampleParam * pData)
{
	int    ok = -1; //, what = -1;
	PPID   op_id = 0;
	TDialog * dlg = new TDialog(DLG_SELBBSMPL);
	if(CheckDialogPtrErr(&dlg)) {
		ushort v = 0;
		PPIDArray op_type_list;
		dlg->AddClusterAssocDef(CTL_SELBBSMPL_WHAT, 0, SelAddBySampleParam::acnStd);
		dlg->AddClusterAssoc(CTL_SELBBSMPL_WHAT, 1, SelAddBySampleParam::acnShipmAll);
		dlg->SetClusterData(CTL_SELBBSMPL_WHAT, pData->Action);
		//dlg->setCtrlData(CTL_SELBBSMPL_WHAT, &v);
		//SetupPPObjCombo(dlg, CTLSEL_SELBBSMPL_OP, PPOBJ_OPRKIND, 0, 0, PPOPT_GOODSEXPEND);
		op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, 0L);
		SetupOprKindCombo(dlg, CTLSEL_SELBBSMPL_OP, pData->OpID, 0, &op_type_list, 0);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			//dlg->getCtrlData(CTL_SELBBSMPL_WHAT, &v);
			dlg->GetClusterData(CTL_SELBBSMPL_WHAT, &pData->Action);
			ok = 1;
			if(pData->Action == SelAddBySampleParam::acnShipmAll) {
				dlg->getCtrlData(CTLSEL_SELBBSMPL_OP, &pData->OpID);
				if(!pData->OpID) {
					PPErrorByDialog(dlg, CTLSEL_SELBBSMPL_OP, PPERR_OPRKINDNEEDED);
					ok = -1;
				}
			}
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

static int SLAPI SelectAddByOrderAction(SelAddBySampleParam * pData, int allowBulkMode)
{
	static const char * WrParam_StoreFlags = "SelectAddBillBySampleFlags";

	class SelAddByOrdDialog : public TDialog {
	public:
		SelAddByOrdDialog(int allowBulkMode) : TDialog(DLG_SELOBSMPL), AllowBulkMode(allowBulkMode)
		{
		}
		int    setDTS(const SelAddBySampleParam * pData)
		{
			Data = *pData;
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 0, SelAddBySampleParam::acnStd);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 1, SelAddBySampleParam::acnShipmByOrder);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 2, SelAddBySampleParam::acnDraftExpByOrder);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 3, SelAddBySampleParam::acnDraftExpRestByOrder); // @v9.9.6
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 4, SelAddBySampleParam::acnDraftRcpByOrder); // @v9.9.6 (3, 3)-->(4, 3)
			SetClusterData(CTL_SELBBSMPL_WHAT, Data.Action);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 0, SelAddBySampleParam::fCopyBillCode);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 1, SelAddBySampleParam::fNonInteractive); // @v10.0.02
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 2, SelAddBySampleParam::fAll); // @v10.0.02
			SetClusterData(CTL_SELBBSMPL_FLAGS, Data.Flags);
			DisableClusterItem(CTL_SELBBSMPL_FLAGS, 2, !AllowBulkMode || Data.Action == SelAddBySampleParam::acnStd); // @v10.0.02
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0, 0); // @v10.0.02
			setCtrlDate(CTL_SELBBSMPL_DT, Data.Dt); // @v10.0.02
			setupOpCombo();
			restoreFlags();
			return 1;
		}
		int    getDTS(SelAddBySampleParam * pData)
		{
			int    ok = 1;
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			GetClusterData(CTL_SELBBSMPL_SAMECODE, &Data.Flags);
			getCtrlData(CTLSEL_SELBBSMPL_LOC, &Data.LocID);
			getCtrlData(CTLSEL_SELBBSMPL_OP, &Data.OpID);
			getCtrlData(CTLSEL_SELBBSMPL_QK, &Data.QuotKindID); // @v10.0.02
			Data.Dt = getCtrlDate(CTL_SELBBSMPL_DT); // @v10.0.02
			if(oneof4(Data.Action, SelAddBySampleParam::acnShipmByOrder, SelAddBySampleParam::acnDraftExpByOrder,
				SelAddBySampleParam::acnDraftExpRestByOrder, SelAddBySampleParam::acnDraftRcpByOrder)) {
				if(Data.OpID == 0)
					ok = PPErrorByDialog(this, CTLSEL_SELBBSMPL_OP, PPERR_OPRKINDNEEDED);
				else if(Data.LocID == 0)
					ok = PPErrorByDialog(this, CTLSEL_SELBBSMPL_LOC, PPERR_LOCNEEDED);
			}
			if(ok) {
				storeFlags();
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			int    process_reg = 0;
			if(event.isClusterClk(CTL_SELBBSMPL_WHAT)) {
				setupOpCombo();
				DisableClusterItem(CTL_SELBBSMPL_FLAGS, 2, !AllowBulkMode || Data.Action == SelAddBySampleParam::acnStd); // @v10.0.02
				process_reg = 1;
				clearEvent(event);
			}
			else if(event.isCbSelected(CTLSEL_SELBBSMPL_OP)) {
				process_reg = 1;
				clearEvent(event);
			}
			if(process_reg) {
				PPID   new_op_id = getCtrlLong(CTLSEL_SELBBSMPL_OP);
				long   new_action = 0;
				GetClusterData(CTL_SELBBSMPL_WHAT, &new_action);
				if(new_op_id != Data.OpID || new_action != Data.Action) {
					storeFlags();
					restoreFlags();
				}
				GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
				Data.OpID = getCtrlLong(CTLSEL_SELBBSMPL_OP);
			}
		}
		void   storeFlags()
		{
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::PrefSettings, 0);
			SString param, val;
			long   flags = 0;
			GetClusterData(CTL_SELBBSMPL_SAMECODE, &flags);
			(param = WrParam_StoreFlags).CatChar('-').Cat(Data.OpID).CatChar('-').Cat(Data.Action);
			val.Z().Cat(flags & SelAddBySampleParam::fCopyBillCode);
			reg_key.PutString(param, val);
		}
		void   restoreFlags()
		{
			WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::PrefSettings, 1); // @v9.2.0 readonly 0-->1
			SString param;
			char   val_buf[128];
			PPID   op_id = getCtrlLong(CTLSEL_SELBBSMPL_OP);
			long   action = 0;
			GetClusterData(CTL_SELBBSMPL_WHAT, &action);
			(param = WrParam_StoreFlags).CatChar('-').Cat(op_id).CatChar('-').Cat(action);
			long   flags = 0;
			if(reg_key.GetString(param, val_buf, sizeof(val_buf))) {
				long rf = atoi(val_buf);
				SETFLAGBYSAMPLE(flags, SelAddBySampleParam::fCopyBillCode, rf);
			}
			SetClusterData(CTL_SELBBSMPL_SAMECODE, flags);
		}
		void   setupOpCombo()
		{
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			PPIDArray op_list;
			PPOprKind op_rec;
			if(Data.Action == Data.acnShipmByOrder) {
				for(PPID id = 0; EnumOperations(PPOPT_GOODSEXPEND, &id, &op_rec) > 0;)
					if(op_rec.Flags & OPKF_ONORDER)
						op_list.add(id);
			}
			else if(oneof2(Data.Action, Data.acnDraftExpByOrder, Data.acnDraftExpRestByOrder)) {
				for(PPID id = 0; EnumOperations(PPOPT_DRAFTEXPEND, &id, &op_rec) > 0;)
					op_list.add(id);
			}
			else if(Data.Action == Data.acnDraftRcpByOrder) {
				for(PPID id = 0; EnumOperations(PPOPT_DRAFTRECEIPT, &id, &op_rec) > 0;)
					op_list.add(id);
			}
			SetupOprKindCombo(this, CTLSEL_SELBBSMPL_OP, op_list.getSingle(), 0, &op_list, OPKLF_OPLIST);
		}
		SelAddBySampleParam Data;
		int    AllowBulkMode;
	};
	DIALOG_PROC_BODY_P1(SelAddByOrdDialog, allowBulkMode, pData);
}

int SLAPI PPViewBill::AddItemBySample(PPID * pID, PPID sampleBillID)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	if(sampleBillID && P_BObj->Search(sampleBillID, &bill_rec) > 0) {
		PPID   bill_id = 0;
		const PPID op_type_id = GetOpType(bill_rec.OpID);
		if(Filt.DenyFlags & BillFilt::fDenyAdd)
			ok = -1;
		else if(Filt.Flags & BillFilt::fAccturnOnly && op_type_id != PPOPT_AGREEMENT) // @v10.2.2 (&& op_type_id != PPOPT_AGREEMENT)
			ok = P_BObj->AddAccturnBySample(&bill_id, sampleBillID);
		else {
			SelAddBySampleParam param;
			MEMSZERO(param);
			param.Action = param.acnUndef;
			param.LocID = bill_rec.LocID;
			int    allow_bulk_mode = 0;
			if(op_type_id == PPOPT_GOODSORDER) {
				// @v10.0.02 {
				if(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1) && Filt.OpID && GetOpType(Filt.OpID) == op_type_id)
					allow_bulk_mode = 1;
				// } @v10.0.02
				SelectAddByOrderAction(&param, allow_bulk_mode);
			}
			else if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF))
				SelectAddByRcptAction(&param);
			else
				param.Action = SelAddBySampleParam::acnStd;
			switch(param.Action) {
				case SelAddBySampleParam::acnStd:
					{
						PPObjBill::AddBlock ab;
						ab.SampleBillID = sampleBillID;
						ok = P_BObj->AddGoodsBill(&bill_id, &ab);
					}
					break;
				case SelAddBySampleParam::acnShipmByOrder:
					if(op_type_id == PPOPT_GOODSORDER) {
						if(allow_bulk_mode && param.Flags & param.fAll) {
							param.Flags |= param.fNonInteractive;
							PPIDArray bill_id_list;
							BillViewItem item;
							for(InitIteration(OrdByDefault); NextIteration(&item) > 0;)
								bill_id_list.add(item.ID);
							if(bill_id_list.getCount()) {
								bill_id_list.sortAndUndup();
								PPLogger logger;
								PPWait(1);
								for(uint i = 0; i < bill_id_list.getCount(); i++) {
									const PPID sample_bill_id = bill_id_list.get(i);
									const int  local_result = P_BObj->AddExpendByOrder(&bill_id, sampleBillID, &param);
									if(!local_result)
										logger.LogLastError();
									else if(local_result == cmOK) {
										logger.LogAcceptMsg(PPOBJ_BILL, bill_id, 0);
										ok = cmOK;
									}
									PPWaitPercent(i+1, bill_id_list.getCount());
								}
								PPWait(0);
							}
						}
						else {
							ok = P_BObj->AddExpendByOrder(&bill_id, sampleBillID, &param);
						}
					}
					else
						ok = -1;
					break;
				case SelAddBySampleParam::acnDraftExpByOrder:
				case SelAddBySampleParam::acnDraftExpRestByOrder:
				case SelAddBySampleParam::acnDraftRcpByOrder:
					if(op_type_id == PPOPT_GOODSORDER) {
						if(allow_bulk_mode && param.Flags & param.fAll) {
							param.Flags |= param.fNonInteractive; // !
							PPIDArray bill_id_list;
							BillViewItem item;
							for(InitIteration(OrdByDefault); NextIteration(&item) > 0;)
								bill_id_list.add(item.ID);
							if(bill_id_list.getCount()) {
								bill_id_list.sortAndUndup();
								PPLogger logger;
								PPWait(1);
								for(uint i = 0; i < bill_id_list.getCount(); i++) {
									const PPID sample_bill_id = bill_id_list.get(i);
									const int  local_result = P_BObj->AddDraftByOrder(&bill_id, sample_bill_id, &param);
									if(!local_result)
										logger.LogLastError();
									else if(local_result == cmOK) {
										logger.LogAcceptMsg(PPOBJ_BILL, bill_id, 0);
										ok = cmOK;
									}
									PPWaitPercent(i+1, bill_id_list.getCount());
								}
								PPWait(0);
							}
							else
								ok = cmCancel;
						}
						else {
							ok = P_BObj->AddDraftByOrder(&bill_id, sampleBillID, &param);
						}
					}
					else
						ok = -1;
					break;
				case SelAddBySampleParam::acnShipmAll:
					if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF))
						ok = P_BObj->AddExpendByReceipt(&bill_id, sampleBillID, &param);
					else
						ok = -1;
					break;
				default:
					ok = -1;
					break;
			}
		}
		if(ok == cmOK) {
			if(Filt.Flags & BillFilt::fEditPoolByType) {
				if(InsertIntoPool(bill_id, 1) == 0)
					PPError();
			}
			else
				SendAddBillMessage(bill_id);
			ASSIGN_PTR(pID, bill_id);
		}
		else if(ok)
			ok = -1;
	}
	return ok;
}

int SLAPI PPViewBill::EditItem(PPID billID)
{
	if(billID && !(Filt.DenyFlags & BillFilt::fDenyUpdate))
		return (P_BObj->Edit(&billID, 0) == cmOK) ? 1 : -1;
	else
		return -1;
}
//
// Returns:
//   <0 - Cancel operation
//   0  - Error
//   1  - Rollback writing off draft
//   2  - Remove draft
//
static int SLAPI ConfirmRmvDraft(int all)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(all ? DLG_RMVDRAFTALL : DLG_RMVDRAFT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlUInt16(CTL_RMVDRAFT_WHAT, 0);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			ushort v = dlg->getCtrlUInt16(CTL_RMVDRAFT_WHAT);
			if(v == 0)
				ok = 1;
			else if(v == 1)
				ok = 2;
		}
	}
	delete dlg;
	return ok;
}

int SLAPI PPViewBill::DeleteBillFromPool(PPID billID)
{
	int    ok = -1;
	ushort v = 0;
	TDialog * dlg = new TDialog(DLG_BPOOLRMV);
	THROW(CheckDialogPtrErr(&dlg));
	dlg->setCtrlData(CTL_BPOOLRMV_VERB, &v);
	if(ExecView(dlg) == cmOK) {
		PPIDArray bill_list;
		if(billID)
			bill_list.add(billID);
		else
			P_BObj->P_Tbl->GetPoolMembersList(Filt.AssocID, Filt.PoolBillID, &bill_list);
		v = dlg->getCtrlUInt16(CTL_BPOOLRMV_VERB);
		{
			PPTransaction tra(1);
			THROW(tra);
			bill_list.sort();
			uint i = bill_list.getCount();
			if(i)
				do {
					const PPID bill_id = bill_list.get(--i);
					THROW(RemoveFromPool(bill_id, 0));
					if(v == 0)
						if(P_BObj->Search(bill_id) > 0)
							THROW(P_BObj->RemovePacket(bill_id, 0));
					ok = 1;
				} while(i);
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewBill::DeleteItem(PPID billID)
{
	int    ok = -1;
	int    i, r;
	if(!(Filt.DenyFlags & BillFilt::fDenyRemove)) {
		int    is_bill_found = 0;
		int    is_wrd_draft = 0;
		BillTbl::Rec bill_rec;
		if(billID && P_BObj->Search(billID, &bill_rec) > 0) {
			is_bill_found = 1;
			if(IsDraftOp(bill_rec.OpID) && bill_rec.Flags & BILLF_WRITEDOFF)
				is_wrd_draft = 1;
		}
		if(!is_wrd_draft && ((Filt.Flags & BillFilt::fEditPoolByType) || (Filt.PoolBillID && PPMaster && (CConfig.Flags & CCFLG_DEBUG))))
			ok = DeleteBillFromPool(billID);
		else if(billID) {
			if(is_bill_found) {
				if(is_wrd_draft) {
					r = ConfirmRmvDraft(0);
					if(r == 1) {
						THROW(P_BObj->RollbackWrOffDraft(billID, 1));
						ok = 1;
					}
					else if(r == 2) {
						PPTransaction tra(1);
						THROW(tra);
						THROW(P_BObj->RollbackWrOffDraft(billID, 0));
						THROW(P_BObj->RemovePacket(billID, 0));
						THROW(tra.Commit());
						ok = 1;
					}
				}
				else {
					SString temp_buf;
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName, temp_buf);
					if(CONFIRM_S(PPCFM_DELETE_BILL, temp_buf)) {
						THROW(P_BObj->RemovePacket(billID, 1));
						ok = 1;
					}
				}
			}
		}
		else { // Deleting All Bills By Filter
			PPIDArray id_list;
			THROW(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1));
			if(Filt.Flags & BillFilt::fDraftOnly) {
				if((r = ConfirmRmvDraft(1)) > 0) {
					PPWait(1);
					THROW(GetBillIDList(&id_list));
					if(id_list.getCount()) {
						PPTransaction tra(1);
						THROW(tra);
						for(i = id_list.getCount()-1; i >= 0; i--) {
							PPID bill_id = id_list.at(i);
							if(r == 1) {
								THROW(P_BObj->RollbackWrOffDraft(bill_id, 0));
							}
							else if(r == 2) {
								THROW(P_BObj->RollbackWrOffDraft(bill_id, 0));
								THROW(P_BObj->RemovePacket(bill_id, 0));
							}
							PPWaitPercent(id_list.getCount()-i, id_list.getCount());
						}
						THROW(tra.Commit());
					}
					PPWait(0);
					ok = 1;
				}
			}
			else if(CONFIRMCRIT(PPCFM_RMVALLBILL)) {
				PPWait(1);
				THROW(GetBillIDList(&id_list));
				if(id_list.getCount()) {
					for(i = id_list.getCount()-1; i >= 0; i--) {
		   		    	P_BObj->RemoveObjV(id_list.at(i), 0, PPObject::use_transaction, 0);
						PPWaitPercent(id_list.getCount()-i, id_list.getCount());
					}
					ok = 1;
				}
				PPWait(0);
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::ViewPayments(PPID billID, int kind)
{
	int    ok = -1;
	BillTbl::Rec bill_rec, link_rec;
	PPViewBill * p_v = 0;
	if(P_BObj->Search(billID, &bill_rec) > 0) {
		if(GetOpType(bill_rec.OpID) == PPOPT_INVENTORY /*|| IsDraftOp(bill_rec.OpID)*/) {
			SString bill_code, fmt_buf, title;
			BillFilt flt;
			flt.List.InitEmpty();
			for(DateIter di; P_BObj->P_Tbl->EnumLinks(billID, &di, BLNK_ALL, &link_rec) > 0;)
				flt.List.Add(link_rec.ID);
			THROW_MEM(p_v = new PPViewBill);
			THROW(p_v->Init_(&flt));
			{
				PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName | PPObjBill::mcsAddLocName, bill_code);
				if(IsDraftOp(bill_rec.OpID))
					PPLoadText(PPTXT_PPVTTL_WROFFBILLDRAFT, fmt_buf);
				else if(GetOpType(bill_rec.OpID) == PPOPT_INVENTORY)
					PPLoadText(PPTXT_PPVTTL_WROFFBILLINVNT, fmt_buf);
				if(fmt_buf.NotEmpty()) {
					p_v->SetOuterTitle(title.Printf(fmt_buf, bill_code.cptr()));
				}
			}
			THROW(p_v->Browse(0));
		}
		else if(IsDraftOp(bill_rec.OpID)) {
			ok = P_BObj->ViewPayments(billID, LinkedBillFilt::lkWrOffDraft);
		}
		else
			ok = P_BObj->ViewPayments(billID, kind);
	}
	CATCHZOKPPERR
	delete p_v;
	return ok;
}

int SLAPI PPViewBill::ViewBillsByOrder(PPID billID)
{
	int    ok = -1;
	BillTbl::Rec bill_rec, sh_rec;
	PPViewBill * p_v = 0;
	if(P_BObj->Search(billID, &bill_rec) > 0 && GetOpType(bill_rec.OpID) == PPOPT_GOODSORDER) {
		BillFilt flt;
		flt.List.InitEmpty();
		for(DateIter di; P_BObj->P_Tbl->EnumByObj(billID, &di, &sh_rec) > 0;) {
			if(sh_rec.OpID == 0)
				THROW(flt.List.Add(sh_rec.LinkBillID));
		}
		THROW_MEM(p_v = new PPViewBill);
		THROW(p_v->Init_(&flt));
		THROW(p_v->Browse(0));
	}
	CATCHZOKPPERR
	delete p_v;
	return ok;
}

int SLAPI PPViewBill::AttachBillToOrder(PPID billID)
{
	int    ok = -1, r;
	BillTbl::Rec bill_rec;
	if(P_BObj->Search(billID, &bill_rec) > 0 && CheckOpFlags(bill_rec.OpID, OPKF_ONORDER)) {
		BillFilt flt;
		PPOprKind op_rec;
		GetOpData(bill_rec.OpID, &op_rec);
		PPID   acc_sheet_id = op_rec.AccSheetID;
		flt.Period.upp = bill_rec.Dt;
		PPID   op_id = 0;
		if(op_rec.AccSheetID == LConfig.LocAccSheetID) {
			//
			// Если таблица статей документа - "склады", то заказ подбираем несколько по-иному:
			// выбираем операцию заказа по такой же таблице статей с признаком "Заказ привязан к складу".
			// При этом не ограничиваем выборку контрагентом (из-за межскладских перемещений),
			// но ограничиваем складом.
			//
			for(PPID id = 0; (r = EnumOperations(PPOPT_GOODSORDER, &id, &op_rec)) > 0;)
				if(op_rec.AccSheetID == acc_sheet_id && op_rec.Flags & OPKF_ORDERBYLOC)
					if(op_id == 0)
						op_id = id;
					else {
						op_id = 0;
						break;
					}
			flt.LocList.Add(PPObjLocation::ObjToWarehouse(bill_rec.Object));
			flt.LocList.Add(bill_rec.LocID);
		}
		else {
			for(PPID id = 0; (r = EnumOperations(PPOPT_GOODSORDER, &id, &op_rec)) > 0;)
				if(op_rec.AccSheetID == acc_sheet_id)
					if(op_id == 0)
						op_id = id;
					else {
						op_id = 0;
						break;
					}
			flt.ObjectID = bill_rec.Object;
		}
		flt.OpID = op_id;
		flt.Ft_ClosedOrder = -1;
		flt.Flags   |= (BillFilt::fAsSelector|BillFilt::fOrderOnly);
		if(LastSelID) {
			BillTbl::Rec last_sel_rec;
			if(P_BObj->Fetch(LastSelID, &last_sel_rec) > 0) {
				if(last_sel_rec.Object == flt.ObjectID)
					flt.Sel = LastSelID;
			}
		}
		if(ViewGoodsBills(&flt, 1) > 0) {
			PPID   order_bill_id = flt.Sel;
			LastSelID = flt.Sel;
			PPBillPacket pack, ord_pack;
			THROW(P_BObj->ExtractPacket(billID, &pack) > 0);
			THROW(P_BObj->ExtractPacket(order_bill_id, &ord_pack) > 0);
			THROW(pack.AttachToOrder(&ord_pack));
			THROW(P_BObj->UpdatePacket(&pack, 1));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::AttachBillToDraft(PPID billID, BrowserWindow * pBrw)
{
	int    ok = -1, r;
	BillTbl::Rec bill_rec;
	if(P_BObj->Search(billID, &bill_rec) > 0) {
		uint   msg_id = 0;
		if(bill_rec.LinkBillID) {
			msg_id = PPTXT_BILLALREADYHASLINK;
		}
		else if(!bill_rec.Object) {
			msg_id = PPTXT_BILLHASNTOBJFORLINK;
		}
		else {
			PPObjOprKind op_obj;
			PPOprKind op_rec;
			GetOpData(bill_rec.OpID, &op_rec);
			PPID   acc_sheet_id = op_rec.AccSheetID;
			DateRange period;
			period.upp = bill_rec.Dt;
			period.low = plusdate(bill_rec.Dt, -180);
			PPIDArray op_list;
			LAssocArray doe_flags_list;
			for(PPID id = 0; (r = EnumOperations(0, &id, &op_rec)) > 0;) {
				if(IsDraftOp(id) && op_rec.AccSheetID == acc_sheet_id) {
					PPDraftOpEx doe;
					if(op_obj.GetDraftExData(id, &doe) > 0 && doe.WrOffOpID == bill_rec.OpID) {
						if(op_list.addUnique(id) > 0) {
							doe_flags_list.Add(id, doe.Flags, 0);
						}
					}
				}
			}
			if(op_list.getCount()) {
				PPIDArray bill_list;
				DBQ * dbq = 0;
				BillTbl::Key3 k3;
				BillTbl * t = P_BObj->P_Tbl;
				BExtQuery q(t, 3);
				q.select(t->ID, t->Dt, t->OpID, t->Object, t->Object2, t->StatusID, t->Flags, t->CurID, t->Amount, 0L);
				dbq = & (t->Object == bill_rec.Object && daterange(t->Dt, &period));
				q.where(*dbq);
				k3.Object = bill_rec.Object;
				k3.Dt = period.low;
				k3.BillNo = 0;
				BillTbl::Rec draft_bill_rec;
				for(q.initIteration(0, &k3, spGe); q.nextIteration() > 0;) {
					t->copyBufTo(&draft_bill_rec);
					if(op_list.lsearch(draft_bill_rec.OpID)) {
						int   suited = 0;
						if(draft_bill_rec.Flags & BILLF_WRITEDOFF) {
							long doe_flags = 0;
							if(doe_flags_list.Search(draft_bill_rec.OpID, &doe_flags, 0) && doe_flags & DROXF_MULTWROFF)
								suited = 1;
						}
						else
							suited = 1;
						if(suited)
							bill_list.add(draft_bill_rec.ID);
					}
				}
				if(bill_list.getCount()) {
					BillFilt flt;
					flt.Flags   |= BillFilt::fAsSelector;
					flt.List.Set(&bill_list);
					if(LastSelID && bill_list.lsearch(LastSelID)) {
						flt.Sel = LastSelID;
					}
					if(ViewGoodsBills(&flt, 1) > 0) {
						PPID   draft_bill_id = flt.Sel;
						LastSelID = flt.Sel;
						{
							PPTransaction tra(1);
							THROW(tra);
							THROW(P_BObj->Search(billID, &bill_rec) > 0);
							bill_rec.LinkBillID = draft_bill_id;
							THROW(P_BObj->P_Tbl->EditRec(&billID, &bill_rec, 0));

							THROW(P_BObj->Search(draft_bill_id, &draft_bill_rec) > 0);
							draft_bill_rec.Flags |= BILLF_WRITEDOFF;
							THROW(P_BObj->P_Tbl->EditRec(&draft_bill_id, &draft_bill_rec, 0));
							THROW(tra.Commit());
							ok = 1;
						}
					}
				}
				else
					msg_id = PPTXT_NODRAFTBILLFORLINK;
			}
			else
				msg_id = PPTXT_NODRAFTOPFORLINK;
		}
		if(ok < 0 && msg_id && pBrw) {
			SString fmt_buf, msg_buf, bill_text;
			if(PPLoadText(msg_id, fmt_buf)) {
				PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, bill_text);
				msg_buf.Printf(fmt_buf, bill_text.cptr());
				PPTooltipMessage(msg_buf, 0, pBrw->H(), 10000, GetColorRef(SClrOrange), SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
					SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::UniteInventory()
{
	int    ok = -1;
	ushort v;
	PPObjBill::InvMovSgo sgo;
	int    rmv_src = 1;
	PPIDArray ary;
	TDialog * dlg = new TDialog(DLG_UNITEINVENT);
	THROW(CheckDialogPtr(&dlg));
	dlg->setCtrlUInt16(CTL_UNITEINVENT_SGOPTION, 0);
	dlg->setCtrlUInt16(CTL_UNITEINVENT_FLAGS,    1);
	if(ExecView(dlg) == cmOK) {
		v = dlg->getCtrlUInt16(CTL_UNITEINVENT_SGOPTION);
		if(v == 0)
			sgo = PPObjBill::imsgoAdd;
		else if(v == 1)
			sgo = PPObjBill::imsgoSkip;
		else if(v == 2)
			sgo = PPObjBill::imsgoFail;
		else
			sgo = PPObjBill::imsgoAdd;
		v = dlg->getCtrlUInt16(CTL_UNITEINVENT_FLAGS);
		rmv_src = BIN(v & 0x01);
		PPWait(1);
		THROW(GetBillIDList(&ary));
		if(ary.getCount() > 1) {
			PPID   dest_id = ary.get(0);
			PPIDArray src_ids;
			for(uint i = ary.getCount()-1; i > 0; i--)
				src_ids.add(ary.get(i));
			ary.freeAll();
			THROW(P_BObj->UniteInventory(dest_id, &src_ids, sgo, rmv_src, 1));
			ok = 1;
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewBill::UniteSellBills()
{
	int    ok = -1;
	PPIDArray  src_ids, dest_ids;
	PPBillPacket pack;
	if(PPMessage(mfConf|mfYesNo, PPCFM_UNITESELL) == cmYes) {
		PPWait(1);
		THROW(GetBillIDList(&src_ids));
		if(src_ids.getCount() > 1) {
			uint   i;
			PPBill last_bill;
			PPTransaction tra(1);
			const PPID src_bill_id = src_ids.at(src_ids.getCount()-1);
			THROW(tra);
			THROW(P_BObj->P_Tbl->Extract(src_bill_id, &last_bill) > 0);
			THROW(pack.CreateBlank2(Filt.OpID, last_bill.Rec.Dt, last_bill.Rec.LocID, 0));
			pack.Rec.Object = last_bill.Rec.Object;
			pack.Rec.Object2 = last_bill.Rec.Object2;
			pack.Ext = last_bill.Ext;
			pack.SetFreight(last_bill.P_Freight);
			SETFLAGBYSAMPLE(pack.Rec.Flags, BILLF_WHITELABEL, last_bill.Rec.Flags);
			THROW(P_BObj->TurnPacket(&pack, 0));
			dest_ids.addUnique(pack.Rec.ID);
			for(i = 0; i < src_ids.getCount(); i++) {
				PPWaitPercent(i, src_ids.getCount());
				THROW(P_BObj->UniteGoodsBill(&pack, src_ids.at(i), 0));
				if(pack.CheckLargeBill(0)) {
					THROW(pack.CreateBlank2(Filt.OpID, last_bill.Rec.Dt, last_bill.Rec.LocID, 0));
					pack.Rec.Object = last_bill.Rec.Object;
					pack.Rec.Object2 = last_bill.Rec.Object2;
					pack.Ext = last_bill.Ext;
					pack.SetFreight(last_bill.P_Freight);
					SETFLAGBYSAMPLE(pack.Rec.Flags, BILLF_WHITELABEL, last_bill.Rec.Flags);
					THROW(P_BObj->TurnPacket(&pack, 0));
					dest_ids.addUnique(pack.Rec.ID);
				}
			}
			for(i = 0; i < dest_ids.getCount(); i++) {
				const PPID dest_id = dest_ids.get(i);
				THROW(P_BObj->RecalcPayment(dest_id, 0));
				THROW(P_BObj->RecalcTurns(dest_id, 0, 0));
			}
			THROW(tra.Commit());
			ok = 1;
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::UniteReceiptBills()
{
	int    ok = -1;
	PPIDArray ary;
	if(PPMessage(mfConf|mfYesNo, PPCFM_UNITERCPT) == cmYes) {
		PPWait(1);
		THROW(GetBillIDList(&ary));
		if(ary.getCount() > 1) {
			PPID   dest_id = ary.get(0);
			ary.reverse(0, ary.getCount());
			ary.atFree(ary.getCount()-1);
			THROW(P_BObj->UniteReceiptBill(dest_id, &ary, 1));
			ok = 1;
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::ChangeFlags()
{
	int    ok = -1, r;
	long   set = 0, reset = 0;
	PPID   new_status_id = 0;
	THROW(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1));
	if(ChangeBillFlagsDialog(&set, &reset, &new_status_id) > 0 && (set || reset || new_status_id)) {
		uint   i, p;
	   	PPID * pid;
		PPIDArray ary;
		PPLogger logger;
		THROW(GetBillIDList(&ary));
		{
			PPTransaction tra(1);
			THROW(tra);
			for(i = 0; ary.enumItems(&i, (void**)&pid);) {
				PPBillPacket pack;
				long   sav;
				// @v9.6.8 THROW(P_BObj->ExtractPacketWithFlags(*pid, &pack, BPLD_SKIPTRFR));
				if(P_BObj->ExtractPacket(*pid, &pack) > 0) {
					sav = pack.Rec.Flags;
					for(p = 0; p < 32; p++) {
						const ulong t = (1UL << p);
						if(set & t)
							pack.Rec.Flags |= t;
						if(reset & t)
							pack.Rec.Flags &= ~t;
					}
					if(pack.Rec.Flags != sav) {
						// @v9.6.8 pack.Rec.Flags |= BILLF_NOLOADTRFR;
						if(P_BObj->UpdatePacket(&pack, 0)) {
							ok = 1;
						}
						else
							logger.LogLastError();
					}
					if(new_status_id && P_BObj->CheckRights(BILLOPRT_MODSTATUS, 1)) {
						r = P_BObj->SetStatus(*pid, new_status_id, 0);
						if(r > 0)
							ok = 1;
						else if(r == 0)
							logger.LogLastError();
					}
				}
				else
					logger.LogLastError();
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::InsertIntoPool(PPID billID, int use_ta)
{
	int    ok = 1;
	BillTbl::Rec br;
	TempBillTbl::Rec tbrec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_BObj->Search(billID, &br) > 0);
		if(IsMemberOfPool(billID) <= 0) {
			PPID   assc_id = (Filt.Flags & BillFilt::fEditPoolByType) ? Filt.AssocID : PPASS_OPBILLPOOL;
			THROW(P_BObj->P_Tbl->UpdatePool(billID, assc_id, Filt.PoolBillID, 0));
			if(Filt.AssocID == PPASS_OPBILLPOOL)
				THROW(P_BObj->UpdatePool(Filt.PoolBillID, 0));
		}
		if(P_TempTbl && SearchByID(P_TempTbl, 0, billID, 0) <= 0) {
			InitTempRec(&br, &tbrec);
			THROW_DB(P_TempTbl->insertRecBuf(&tbrec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::RemoveFromPool(PPID billID, int use_ta)
{
	int    ok = 1;
	if(IsMemberOfPool(billID) > 0) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(P_BObj->P_Tbl->RemoveFromPool(billID, Filt.AssocID, Filt.PoolBillID, 0));
		if(Filt.AssocID == PPASS_OPBILLPOOL)
			THROW(P_BObj->UpdatePool(Filt.PoolBillID, 0));
		if(P_TempTbl)
			THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->BillID == billID));
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::UpdateInPool(PPID /*billID*/)
{
	return 1;
}

int SLAPI PPViewBill::IsMemberOfPool(PPID billID)
{
	PPID   pool_id = Filt.PoolBillID;
	return BIN(P_BObj->IsMemberOfPool(billID, Filt.AssocID, &pool_id) > 0);
}

int SLAPI PPViewBill::EnumMembersOfPool(PPID * pBillID)
{
	return P_BObj->P_Tbl->EnumMembersOfPool(Filt.AssocID, Filt.PoolBillID, pBillID);
}

int SLAPI PPViewBill::SetupPoolInsertionFilt(BillFilt * pFilt)
{
	int    ok = -1;
	PPID   first_member_id = 0;
	BillFilt pattern = *pFilt;
	pFilt->PoolOpID = Filt.PoolOpID;

	BillTbl::Rec pool_rec;
	THROW(P_BObj->Search(Filt.PoolBillID, &pool_rec) > 0);
	if(P_BPOX) {
		if(P_BPOX->OpList.getCount() == 1) {
			pFilt->OpID = P_BPOX->OpList.at(0);
			ArticleTbl::Rec ar_rec;
			if(pool_rec.Object && ArObj.Fetch(pool_rec.Object, &ar_rec) > 0) {
				PPOprKind op_rec;
				if(GetOpData(pFilt->OpID, &op_rec) > 0 && op_rec.AccSheetID == ar_rec.AccSheetID)
					pFilt->ObjectID = pool_rec.Object;
			}
			pFilt->LocList.Add(pool_rec.LocID);
		}
		if(EnumMembersOfPool(&first_member_id) > 0) {
			BillTbl::Rec first_member_rec;
			if(P_BObj->Search(first_member_id, &first_member_rec) > 0) {
				if(P_BPOX->Flags & BPOXF_ONEOP)
					pFilt->OpID = first_member_rec.OpID;
				if(P_BPOX->Flags & BPOXF_ONEDATE)
					pFilt->Period.SetDate(first_member_rec.Dt);
				if(P_BPOX->Flags & BPOXF_ONEOBJECT)
					pFilt->ObjectID =  first_member_rec.Object;
			}
		}
	}
	if(pFilt->PoolOpID != pattern.PoolOpID || pFilt->OpID != pattern.OpID ||
		pFilt->Period.low != pattern.Period.low || pFilt->Period.upp != pattern.Period.upp ||
		pFilt->ObjectID != pattern.ObjectID)
		ok = 1;
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::AddBillToPool()
{
	class BillPoolAddDialog : public TDialog {
	public:
		BillPoolAddDialog() : TDialog(DLG_BPOOLADD)
		{
		}
		int    setDTS(const PPViewBill::PoolInsertionParam * pParam)
		{
			Data = *pParam;

			AddClusterAssocDef(CTL_BPOOLADD_VERB,  0, 1);
			AddClusterAssoc(CTL_BPOOLADD_VERB,  1, 2);
			SetClusterData(CTL_BPOOLADD_VERB, Data.Verb);
			enableCommand(cmBillFilt, Data.Verb == 2);

			AddClusterAssocDef(CTL_BPOOLADD_BILLKIND, 0, bbtUndef);
			AddClusterAssoc(CTL_BPOOLADD_BILLKIND, 1, bbtAccturnBills);
			AddClusterAssoc(CTL_BPOOLADD_BILLKIND, 2, bbtGoodsBills);
			AddClusterAssoc(CTL_BPOOLADD_BILLKIND, 3, bbtOrderBills);
			AddClusterAssoc(CTL_BPOOLADD_BILLKIND, 4, bbtDraftBills);
			SetClusterData(CTL_BPOOLADD_BILLKIND, Data.AddedBillKind);
			return 1;
		}
		int    getDTS(PPViewBill::PoolInsertionParam * pParam)
		{
			long   temp_long = 0;
			GetClusterData(CTL_BPOOLADD_VERB, &Data.Verb);
			if(GetClusterData(CTL_BPOOLADD_BILLKIND, &temp_long))
				Data.AddedBillKind = (BrowseBillsType)temp_long;
			ASSIGN_PTR(pParam, Data);
			return 1;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmBillFilt)) {
				getDTS(0);
				if(Data.Verb == 2) {
					PPViewBill bv;
					bv.EditFilt(&Data.Filt, Data.AddedBillKind);
				}
			}
			else if(event.isCmd(cmClusterClk)) {
				getDTS(0);
				if(event.isCtlEvent(CTL_BPOOLADD_VERB))
					enableCommand(cmBillFilt, Data.Verb == 2);
				else if(event.isCtlEvent(CTL_BPOOLADD_BILLKIND))
					Data.Filt.SetupBrowseBillsType(Data.AddedBillKind);
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		PPViewBill::PoolInsertionParam Data;
	};
	int    ok = 1, r;
	const PPConfig & r_cfg = LConfig;
	int    count = 0;
	PPID   loc_id = LocList_.getSingle();
	SETIFZ(loc_id, r_cfg.Location);
 	const  PPID  save_loc_id = r_cfg.Location;
	PPID   ar_id = 0;
	PPID   ar2_id = 0; // @v9.8.1
	BillTbl::Rec bill_rec;
	if(Filt.PoolBillID && Filt.AssocID) {
		//
		// Здесь мы определяем некоторые параметры создаваемого документа на основании информации о владельце пула.
		// Эти параметры в этой точке могут не проверяться слишком тщательно поскольку
		// функции создания документов позаботяться о корректности этих значений самостоятельно.
		//
		if(Filt.AssocID == PPASS_TSESSBILLPOOL) {
			PPObjTSession tses_obj;
			TSessionTbl::Rec tses_rec;
			if(tses_obj.Search(Filt.PoolBillID, &tses_rec) > 0) {
				ProcessorTbl::Rec prc_rec;
				if(tses_obj.GetPrc(tses_rec.PrcID, &prc_rec, 1, 1) > 0)
					loc_id = prc_rec.LocID;
				ar_id = tses_rec.ArID;
				ar2_id = tses_rec.Ar2ID; // @v9.8.1
			}
		}
		else if(P_BObj->Search(Filt.PoolBillID, &bill_rec) > 0)
			ar_id = bill_rec.Object;
	}
	BillPoolAddDialog * dlg = 0;
	PoolInsertionParam param = Pip;
	SetupPoolInsertionFilt(&param.Filt);
	THROW(CheckDialogPtr(&(dlg = new BillPoolAddDialog)));
	while(dlg->setDTS(&param) && ExecView(dlg) == cmOK) {
		PPID   new_bill_id = 0;
		PPID   op_id = 0;
		dlg->getDTS(&param);
		Pip = param;
		dlg->enableCommand(cmBillFilt, 1);
		r = -1;
		if(param.Verb == 1) { // Insert New Bill
			int    is_op_list = 0;
			PPIDArray op_type_list;
			if(P_BPOX && P_BPOX->OpList.getCount()) {
				op_id = P_BPOX->OpList.getSingle();
				if(!op_id) {
					op_type_list.copy(P_BPOX->OpList);
					is_op_list = 1;
				}
			}
			else if(param.AddedBillKind == bbtAccturnBills)
				op_type_list.add(PPOPT_ACCTURN);
			else if(param.AddedBillKind == bbtOrderBills)
				op_type_list.add(PPOPT_GOODSORDER);
			else if(param.AddedBillKind == bbtGoodsBills)
				op_type_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, 0L);
			else if(param.AddedBillKind == bbtDraftBills)
				op_type_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, 0L);
			else
				op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSORDER, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
					PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND,
					PPOPT_DRAFTTRANSIT, PPOPT_CORRECTION, 0L);
			if(op_id || BillPrelude(&op_type_list, (is_op_list ? OPKLF_OPLIST : 0), 0, &op_id, &loc_id) > 0) {
				PPObjBill::AddBlock ab;
				ab.OpID = op_id;
				ab.ObjectID = ar_id;
				ab.Object2ID = ar2_id; // @v9.8.1
				ab.Pk = PPBillPacket::ObjAssocToPoolKind(Filt.AssocID);
				ab.PoolID = Filt.PoolBillID;
				DS.SetLocation(loc_id);
				r = (param.AddedBillKind == bbtAccturnBills) ? P_BObj->AddAccturn(&new_bill_id, &ab) : P_BObj->AddGoodsBill(&new_bill_id, &ab);
				if(r == cmOK)
					r = 1;
				else if(r)
					r = -1;
			}
			if(r > 0 && (r = InsertIntoPool(new_bill_id, 1)) > 0) {
				count++;
				break;
			}
			if(r == 0)
				PPError();
		}
		else if(param.Verb == 2) { // Insert By Filt
			param.Filt.Flags |= BillFilt::fAsSelector;
			PPViewBill temp_bv;
			SetupPoolInsertionFilt(&param.Filt);
			THROW(temp_bv.Init_(&param.Filt));
			while(temp_bv.Browse(0) > 0)
				if((r = InsertIntoPool(temp_bv.Filt.Sel, 1)) == 0)
					PPError();
				else if(r > 0) {
					count++;
					if(SetupPoolInsertionFilt(&param.Filt) > 0)
						THROW(temp_bv.Init_(&param.Filt));
				}
		}
	}
	ok = (count > 0) ? 1 : -1;
	CATCHZOKPPERR
	delete dlg;
	DS.SetLocation(save_loc_id);
	return ok;
}

int SLAPI PPViewBill::GetCommonPoolAttribs(LDATE * pDt, PPID * pLocID, PPID * pOpID, PPID * pObjID)
{
	int    ok = 1;
	PPID   member_id = 0;
	PPID   comm_op_id  = -2;
	PPID   comm_obj_id = -2;
	PPID   comm_loc_id = -2;
	LDATE  comm_dt = MAXDATE;
	while(EnumMembersOfPool(&member_id) > 0) {
		BillTbl::Rec member_rec;
		THROW(P_BObj->Search(member_id, &member_rec) > 0);
		if(comm_op_id == -2)
			comm_op_id = member_rec.OpID;
		else if(comm_op_id != member_rec.OpID)
			comm_op_id = -1;

		if(comm_obj_id == -2)
			comm_obj_id = member_rec.Object;
		else if(comm_obj_id != member_rec.Object)
			comm_obj_id = -1;

		if(comm_loc_id == -2)
			comm_loc_id = member_rec.LocID;
		else if(comm_loc_id != member_rec.LocID)
			comm_loc_id = -1;

		if(comm_dt == MAXDATE)
			comm_dt = member_rec.Dt;
		else if(comm_dt != member_rec.Dt)
			comm_dt = ZERODATE;
	}
	if(comm_dt == MAXDATE)
		comm_dt = ZERODATE;
	ASSIGN_PTR(pDt, comm_dt);
	ASSIGN_PTR(pLocID, comm_loc_id);
	ASSIGN_PTR(pOpID, comm_op_id);
	ASSIGN_PTR(pObjID, comm_obj_id);
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::CreateTempPoolPacket(PPBillPacket * pPack)
{
	int    ok = 1, r_by_bill, found;
	uint   i;
	PPTransferItem ti, * p_ti;
	PPID   member_id = 0;
	PPID   comm_op_id, comm_loc_id, comm_obj_id;
	LDATE  comm_dt;
	PPBillPacket pool_pack;

	THROW(P_BObj->ExtractPacket(Filt.PoolBillID, &pool_pack) > 0);
	THROW(GetCommonPoolAttribs(&comm_dt, &comm_loc_id, &comm_op_id, &comm_obj_id));

	THROW_PP(comm_op_id > 0, PPERR_GETERPOOLMERGE);
	THROW_PP(GetOpType(comm_op_id) != PPOPT_GOODSREVAL, PPERR_REVALPOOLMERGE);

	pPack->CreateBlank(comm_op_id, 0, (comm_loc_id > 0) ? comm_loc_id : pool_pack.Rec.LocID, 0);
	pPack->UngetCounter();
	STRNSCPY(pPack->Rec.Code, pool_pack.Rec.Code);
	STRNSCPY(pPack->Rec.Memo, pool_pack.Rec.Memo);
	pPack->Rec.Dt     = (comm_dt > 0) ? comm_dt : pool_pack.Rec.Dt;
	pPack->Rec.LocID  = (comm_loc_id > 0) ? comm_loc_id : pool_pack.Rec.LocID;
	pPack->Rec.Object = (comm_obj_id >= 0) ? comm_obj_id : pool_pack.Rec.Object;
	pPack->SetFreight(pool_pack.P_Freight);

	while(EnumMembersOfPool(&member_id) > 0) {
		for(r_by_bill = 0; P_BObj->trfr->EnumItems(member_id, &r_by_bill, &ti) > 0;) {
			long sign_mask = (PPTFR_PLUS | PPTFR_MINUS);
			for(i = 0, found = 0; !found && pPack->EnumTItems(&i, &p_ti) > 0;) {
				if(p_ti->GoodsID == ti.GoodsID && (p_ti->Flags & sign_mask) == (ti.Flags & sign_mask)) {
					double sum_qtty = p_ti->Quantity_ + ti.Quantity_;
					p_ti->Cost  = (ti.Cost * ti.Quantity_ + p_ti->Cost * p_ti->Quantity_) / sum_qtty;
					p_ti->Price = ((ti.Price - ti.Discount) * ti.Quantity_ + p_ti->Price * p_ti->Quantity_) / sum_qtty;
					p_ti->Discount = 0.0;
					p_ti->Quantity_ = sum_qtty;
					p_ti->LotID = ti.LotID;
					found = 1;
				}
			}
			if(!found) {
				THROW(ti.Init(&pPack->Rec, 0, ((ti.Flags & sign_mask) == PPTFR_PLUS) ? TISIGN_PLUS : TISIGN_MINUS));
				ti.Price -= ti.Discount;
				ti.Discount = 0.0;
				THROW(pPack->InsertRow(&ti, 0));
			}
		}
	}
	for(i = 0; pPack->EnumTItems(&i, &p_ti) > 0;)
		THROW(p_ti->SetupLot(p_ti->LotID, 0, TISL_IGNCOST|TISL_ADJPRICE));
	CATCHZOK
	return ok;
}

int SLAPI PPViewBill::ShowPoolDetail(PPBillPacket * pBillPack)
{
	int    ok = 1;
	if(pBillPack->Rec.ID)
		if(GetOpType(pBillPack->Rec.OpID) == PPOPT_POOL) {
			BillFilt   flt;
			PPViewBill bv;
			flt.Bbt = bbtUndef;
			flt.PoolBillID = pBillPack->Rec.ID;
			flt.AssocID    = PPASS_OPBILLPOOL;
			flt.Flags |= BillFilt::fEditPoolByType;
			if(bv.Init_(&flt))
				ok = bv.Browse(0);
			else
				ok = PPErrorZ();
		}
	return ok;
}

int SLAPI PPViewBill::ShowDetails(PPID billID)
{
	int    ok = -1, is_lock = 0;
	PPViewInventory * p_v = 0;
	PPBillPacket pack;
	if(billID) {
		THROW(P_BObj->ExtractPacketWithFlags(billID, &pack, BPLD_FORCESERIALS));
		if(GetOpType(pack.Rec.OpID) == PPOPT_INVENTORY) {
			InventoryFilt filt;
			THROW(P_BObj->Lock(billID));
			THROW_MEM(p_v = new PPViewInventory());
			filt.Setup(billID);
			THROW(p_v->Init_(&filt));
			p_v->Browse(0);
			if(p_v->GetUpdateStatus() > 0) {
				p_v->UpdatePacket(billID);
				ok = 1;
			}
			P_BObj->Unlock(billID);
			is_lock = 0;
		}
		else if(GetOpType(pack.Rec.OpID) == PPOPT_POOL) {
			ShowPoolDetail(&pack);
			ok = 1;
		}
		else
			ViewBillDetails(&pack, 2, P_BObj);
	}
	CATCHZOKPPERR
	delete p_v;
	if(is_lock)
		P_BObj->Unlock(billID);
	return ok;
}

static int SLAPI SelectPrintPoolVerb(int * pVerb)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_PRNPOOLBILL);
	if(CheckDialogPtr(&dlg)) {
		ushort v = (*pVerb == 1) ? 0 : 1;
		dlg->setCtrlData(CTL_PRNPOOLBILL_WHAT, &v);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_PRNPOOLBILL_WHAT, &v);
			if(v == 0)
				*pVerb = 1;
			else
				*pVerb = 2;
			ok = 1;
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI PPViewBill::PrintBill(PPID billID /* @v10.0.0, int addCashSummator*/)
{
	int    ok = 1;
	PPBillPacket pack;
	if(billID) {
		THROW(P_BObj->ExtractPacket(billID, &pack));
		if(pack.Rec.Flags & BILLF_CASH) {
			ushort v = 0;
			TDialog * dlg = new TDialog(DLG_SELPRNCHK);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->setCtrlUInt16(CTL_SELPRNCHK_WHAT, 0);
				if(ExecView(dlg) == cmOK)
					v = dlg->getCtrlUInt16(CTL_SELPRNCHK_WHAT) + 1;
				else
					v = 0;
				delete dlg;
				if(v == 1) {
					THROW(P_BObj->PrintCheck(&pack, 0, /*addCashSummator*/1));
				}
				else if(v == 2) {
					PrintGoodsBill(&pack);
				}
				else
					ok = -1;
			}
		}
		else if(pack.OpTypeID == PPOPT_PAYMENT && !CheckOpPrnFlags(pack.Rec.OpID, OPKF_PRT_INVOICE))
			PrintCashOrderByGoodsBill(&pack);
		else if(pack.OpTypeID == PPOPT_POOL) {
			int    prn_verb = 1; // 1 - reestr, 2 - merged bill
			PPID   comm_op_id;
			BillFilt   flt;
			PPViewBill bv;
			flt.PoolBillID = billID;
			flt.AssocID = PPASS_OPBILLPOOL;
			flt.Flags  |= BillFilt::fEditPoolByType;
			THROW(bv.Init_(&flt));
			bv.GetCommonPoolAttribs(0, 0, &comm_op_id, 0);
			if(comm_op_id > 0) {
				THROW(ok = SelectPrintPoolVerb(&prn_verb));
				if(ok > 0)
					if(prn_verb == 1)
						bv.Print();
					else {
						PPBillPacket merged_pack;
						THROW(bv.CreateTempPoolPacket(&merged_pack));
						PrintGoodsBill(&merged_pack);
					}
			}
			else
				bv.Print();
		}
		else
			PrintGoodsBill(&pack);
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::PrintAllBills()
{
	int    ok = -1, is_packet = 0;
	PPID   op_type_id = Filt.OpID ? GetOpType(Filt.OpID) : 0;
	SVector * p_rpt_ary = 0; // @v9.8.6 SArray-->SVector
	if(!oneof3(op_type_id, 0, PPOPT_POOL, PPOPT_GENERIC) && (op_type_id != PPOPT_PAYMENT || CheckOpPrnFlags(Filt.OpID, OPKF_PRT_INVOICE))) {
		int    out_amt_type = 0, r = 1;
		uint   count = 0;
		PPBillPacket pack;
		BillViewItem item;
		IterOrder order = OrdByDefault;
		if(Filt.SortOrder == BillFilt::ordByDate)
			order = OrdByDate;
		else if(Filt.SortOrder == BillFilt::ordByCode)
			order = OrdByCode;
		else if(Filt.SortOrder == BillFilt::ordByObject)
			order = OrdByObjectName;
		for(InitIteration(order); r > 0 && NextIteration(&item) > 0;) {
			if(item.ID) {
				is_packet = 0;
				THROW(P_BObj->ExtractPacket(item.ID, &pack));
				is_packet = 1;
				pack.OutAmtType = out_amt_type;
				THROW(r = PrintGoodsBill(&pack, &p_rpt_ary, 1));
				out_amt_type = pack.OutAmtType;
				count++;
			}
		}
		ok = 1;
	}
	CATCH
		if(!is_packet)
			PPError();
		ok = 0;
	ENDCATCH
	delete p_rpt_ary;
	return ok;
}

int SLAPI PPViewBill::UpdateAttributes()
{
	struct UpdAttr {
		PPID   AgentID;
		PPID   PayerID;
		PPID   ObjectID;
		PPID   Object2ID;
		long   ModCodeStartCounter;
	};
	int    ok = 1, frrl_tag = 0;
	UpdAttr ua;
	MEMSZERO(ua);
	TDialog * dlg = 0;
	PPIDArray ary;
	PPID   obj_sheet_id = Filt.AccSheetID;
	PPID   obj_sheet2_id = 0;
	int    enable_mod_code = 0;
	SString temp_buf;
	SString mod_code_template;
	PPObjOpCounter opc_obj;
	PPOpCounter opc_rec;
	THROW(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1));
	if(Filt.OpID)
		GetOpCommonAccSheet(Filt.OpID, obj_sheet_id ? 0 : &obj_sheet_id, &obj_sheet2_id);
	THROW(GetBillIDList(&ary));
	THROW(CheckDialogPtrErr(&(dlg = new TDialog(DLG_UPDBLIST))));
	SetupArCombo(dlg, CTLSEL_UPDBLIST_PAYER,   ua.PayerID,   OLW_CANINSERT, GetSellAccSheet(),  sacfDisableIfZeroSheet);
	SetupArCombo(dlg, CTLSEL_UPDBLIST_AGENT,   ua.AgentID,   OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
	SetupArCombo(dlg, CTLSEL_UPDBLIST_OBJECT,  ua.ObjectID,  OLW_CANINSERT, obj_sheet_id,       sacfDisableIfZeroSheet);
	SetupArCombo(dlg, CTLSEL_UPDBLIST_OBJECT2, ua.Object2ID, OLW_CANINSERT, obj_sheet2_id,      sacfDisableIfZeroSheet);
	if(Filt.OpID && !IsGenericOp(Filt.OpID)) {
		PPOprKind op_rec;
		GetOpData(Filt.OpID, &op_rec);
		if(op_rec.OpCounterID) {
			if(opc_obj.Search(op_rec.OpCounterID, &opc_rec) > 0 && opc_rec.CodeTemplate[0]) {
				mod_code_template = opc_rec.CodeTemplate;
				dlg->setCtrlLong(CTL_UPDBLIST_MODCODEST, ua.ModCodeStartCounter);
				enable_mod_code = 1;
			}
		}
	}
	dlg->disableCtrl(CTL_UPDBLIST_MODCODEST, !enable_mod_code);
	if(ExecView(dlg) == cmOK) {
		uint   i = 0;
		long   upd_code_counter = 0;
		SString org_code_buf;
		IterCounter counter;
		dlg->getCtrlData(CTLSEL_UPDBLIST_PAYER, &ua.PayerID);
		dlg->getCtrlData(CTLSEL_UPDBLIST_AGENT, &ua.AgentID);
		dlg->getCtrlData(CTLSEL_UPDBLIST_OBJECT, &ua.ObjectID);
		dlg->getCtrlData(CTLSEL_UPDBLIST_OBJECT2, &ua.Object2ID);
		if(enable_mod_code) {
			ua.ModCodeStartCounter = dlg->getCtrlLong(CTL_UPDBLIST_MODCODEST);
			upd_code_counter = ua.ModCodeStartCounter;
		}
		PPWait(1);
		{
			PPTransaction tra(1);
			THROW(tra);
			THROW(P_BObj->atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
			PPWaitPercent(0);
			counter.Init(ary.getCount());
			for(i = 0; i < ary.getCount(); i++) {
				PPID   bill_id = ary.get(i);
				BillTbl::Rec rec;
				THROW(PPCheckUserBreak());
				if(ua.ObjectID || ua.Object2ID) {
					PPBillPacket pack;
					if(P_BObj->ExtractPacket(bill_id, &pack) > 0) {
						int    do_upd = 0;
						const long org_rec_flags = pack.Rec.Flags;
						const long org_rec_flags2 = pack.Rec.Flags2;
						if(ua.ObjectID && pack.Rec.Object != ua.ObjectID) {
							pack.Rec.Object = ua.ObjectID;
							do_upd = 1;
						}
						if(ua.Object2ID && pack.Rec.Object2 != ua.Object2ID) {
							pack.Rec.Object2 = ua.Object2ID;
							do_upd = 1;
						}
						if(ua.ModCodeStartCounter && mod_code_template.NotEmpty()) {
							opc_obj.CodeByTemplate(mod_code_template, upd_code_counter-1, temp_buf);
							upd_code_counter++;
							STRNSCPY(pack.Rec.Code, temp_buf);
							BillCore::SetCode(pack.Rec.Code, pack.Rec.Flags);
							do_upd = 1;
						}
						if(do_upd) {
							THROW(P_BObj->FillTurnList(&pack));
							THROW(P_BObj->UpdatePacket(&pack, 0));
						}
					}
				}
				else {
					PPBillExt ext;
					THROW(P_BObj->P_Tbl->GetExtraData(bill_id, &ext));
					if(P_BObj->Search(bill_id, &rec) > 0) {
						org_code_buf = rec.Code;
						const long org_rec_flags = rec.Flags;
						const long org_rec_flags2 = rec.Flags2;
						int   do_upd = 0;
						if(ua.AgentID && ext.AgentID != ua.AgentID) {
							ext.AgentID = ua.AgentID;
							do_upd = 1;
						}
						if(ua.PayerID && ext.PayerID != ua.PayerID) {
							ext.PayerID = ua.PayerID;
							do_upd = 1;
						}
						SETFLAG(rec.Flags, BILLF_EXTRA, !ext.IsEmpty());
						if(ua.ModCodeStartCounter && mod_code_template.NotEmpty()) {
							opc_obj.CodeByTemplate(mod_code_template, upd_code_counter-1, temp_buf);
							upd_code_counter++;
							STRNSCPY(rec.Code, temp_buf);
							do_upd = 1;
						}
						BillCore::SetCode(rec.Code, rec.Flags);
						if(org_code_buf != rec.Code) {
							do_upd = 1;
						}
						rec.Flags &= ~BILLF_NOLOADTRFR;
						rec.Flags2 &= ~BILLF2_DONTCLOSDRAFT;
						if(rec.Flags != org_rec_flags || rec.Flags2 != org_rec_flags2)
							do_upd = 1;
						if(do_upd) {
							THROW_DB(P_BObj->P_Tbl->updateRecBuf(&rec));
							THROW(P_BObj->P_Tbl->PutExtraData(bill_id, ((rec.Flags & BILLF_EXTRA) ? &ext : 0), 0));
							DS.LogAction(PPACN_UPDBILLEXT, PPOBJ_BILL, bill_id, 0, 0);
						}
					}
				}
				PPWaitPercent(counter.Increment());
			}
			THROW(P_BObj->atobj->P_Tbl->LockingFRR(0, &frrl_tag, 0));
			THROW(tra.Commit());
		}
		PPWait(0);
	}
	CATCH
		P_BObj->atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int SLAPI PPViewBill::CalcBillVATax(BVATAccmArray * dest)
{
	uint   ok = 1, i, c;
	PPIDArray ary;
	THROW(GetBillIDList(&ary));
	dest->freeAll();
	PPWaitPercent(0);
	for(i = 0, c = ary.getCount(); i < c; PPWaitPercent(++i, c))
		THROW(PPCheckUserBreak() && dest->CalcBill(ary.at(i)));
	CATCHZOK
	return ok;
}

static void SLAPI _SetTotal(TDialog * dlg, uint ctl, double s, double vatnf, double vat)
{
	SString buf;
	buf.Space().
		Cat(s, MKSFMTD(19, 2, NMBF_TRICOMMA|ALIGN_RIGHT)).CR().Space().
		Cat(vatnf, MKSFMTD(19, 4, NMBF_TRICOMMA|ALIGN_RIGHT)).CR().Space().
		Cat(vat,   MKSFMTD(19, 4, NMBF_TRICOMMA|ALIGN_RIGHT));
	dlg->setStaticText(ctl, buf);
}

int SLAPI PPViewBill::ViewVATaxList()
{
	int    ok = 1;
	BVATAccmArray dest;
	uint   i;
	double vat_cost   = 0.0, vat_price   = 0.0, tmp;
	double vatnf_cost = 0.0, vatnf_price = 0.0; // Сумма НДС для тех, кто платит
	double s_cost     = 0.0, s_price     = 0.0;
	double snf_cost   = 0.0, snf_price   = 0.0; // Сумма по документам для тех, кто платит
	SString sub;
	BVATAccm     * p_item, total;
	StrAssocArray * p_ary = 0;
	SmartListBox * p_list = 0;
	ListBoxDef   * p_def  = 0;
	TDialog      * dlg    = 0;
	PPWait(1);
	THROW(CalcBillVATax(&dest));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_VATAXLST))));
	p_list = (SmartListBox *)dlg->getCtrlView(CTL_VATAXLST_LIST);
	MEMSZERO(total);
	THROW_MEM(p_ary = new StrAssocArray);
	for(i = 0; dest.enumItems(&i, (void**)&p_item);) {
		StringSet ss(SLBColumnDelim);
		tmp        = SalesTaxMult(p_item->PRate);
		s_cost    += p_item->Cost;
		s_price   += p_item->Price;
		vat_cost  += p_item->Cost  * tmp;
		vat_price += p_item->Price * tmp;
		if(p_item->IsVatFree)
			ss.add(sub.Z().Cat(p_item->PRate, SFMT_QTTY).Space().Cat("(0)"));
		else {
			ss.add(sub.Z().Cat(p_item->PRate, SFMT_QTTY));
			snf_cost    += p_item->Cost;
			snf_price   += p_item->Price;
			vatnf_cost  += p_item->Cost  * tmp;
			vatnf_price += p_item->Price * tmp;
		}
		ss.add(sub.Z().Cat(p_item->Cost,  MKSFMTD(0, 2, NMBF_TRICOMMA)));
		ss.add(sub.Z().Cat(p_item->Price, MKSFMTD(0, 2, NMBF_TRICOMMA)));
		THROW_SL(p_ary->Add(i+1, ss.getBuf()));
	}
	vat_cost    = CalcVATRate(s_cost,    vat_cost);
	vat_price   = CalcVATRate(s_price,   vat_price);
	vatnf_cost  = CalcVATRate(snf_cost,  vatnf_cost);
	vatnf_price = CalcVATRate(snf_price, vatnf_price);
	_SetTotal(dlg, CTL_VATAXLST_COST,  s_cost,  vatnf_cost,  vat_cost);
	_SetTotal(dlg, CTL_VATAXLST_PRICE, s_price, vatnf_price, vat_price);
	if(p_list) {
		THROW_MEM(p_def = new StrAssocListBoxDef(p_ary, lbtDisposeData));
		p_list->setDef(p_def);
		p_list->Draw_();
	}
	PPWait(0);
	ExecView(dlg);
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewBill::ViewTotal()
{
	class BillTotalDialog : public AmtListDialog {
	public:
		BillTotalDialog(uint resID, uint listCtlId, BillTotal * pTotal, PPViewBill * pV) :
			AmtListDialog(resID, listCtlId, 1, &pTotal->Amounts, 0, 0, 0), P_Total(pTotal), P_V(pV)
		{
			setCtrlLong(CTL_BILLTOTAL2_COUNT, pTotal->Count);
			setCtrlReal(CTL_BILLTOTAL2_SUM,   pTotal->Sum);
			setCtrlReal(CTL_BILLTOTAL2_DEBT,  pTotal->Debt);
		}
	protected:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmPrint) && P_V) {
				CALLPTRMEMB(P_V, PrintTotal(P_Total));
			}
			else if(event.isCmd(cmCalcVAT)) {
				CALLPTRMEMB(P_V, ViewVATaxList());
			}
			else
				return;
			clearEvent(event);
		}
		PPViewBill * P_V;
		BillTotal * P_Total;
	};
	int    ok = -1;
	if(Filt.Flags & BillFilt::fInvOnly) {
		PPIDArray id_list;
		BillViewItem item;
		PPWait(1);
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			id_list.add(item.ID);
		id_list.sortAndUndup();
		PPWait(0);
		ok = P_BObj->ViewInventoryTotal(id_list, 0);
	}
	else {
		BillTotal total;
		PPWait(1);
		if(CalcTotal(&total)) {
			PPWait(0);
			if(!P_BObj->CheckRights(BILLRT_ACCSCOST)) {
				total.Amounts.Put(PPAMT_BUYING, 0L /* @curID */, 0, 0, 1);
				if(Filt.OpID && CheckOpFlags(Filt.OpID, OPKF_BUYING))
					total.Sum = 0.0;
			}
			total.Amounts.Remove(PPAMT_PCTDIS, 0L);
			BillTotalDialog * dlg = new BillTotalDialog(DLG_BILLTOTAL2, CTL_BILLTOTAL2_AMTLIST, &total, this);
			if(CheckDialogPtrErr(&dlg))
				ExecViewAndDestroy(dlg);
		}
		PPWait(0);
	}
	return ok;
}

// @vmiller
struct PrvdrDllLink {
	PrvdrDllLink()
	{
		THISZERO();
	}
	int    SessId;
	char   PrvdrSymb[12]; // PPSupplExchangeCfg::PrvdrSymb
	ImpExpDll * P_ExpDll;
	ImpExpParamDllStruct ParamDll;
};

int WriteBill_NalogRu2_Invoice(const PPBillPacket & rBp, SString & rFileName); // @prototype
int WriteBill_NalogRu2_DP_REZRUISP(const PPBillPacket & rBp, SString & rFileName); // @prototype
int WriteBill_NalogRu2_UPD(const PPBillPacket & rBp, SString & rFileName); // @prototype

int SLAPI PPViewBill::ExportGoodsBill(const PPBillImpExpParam * pBillParam, const PPBillImpExpParam * pBRowParam)
{
	int    ok = -1, r = 0;
	int    dll_pos = 0;
	PPID   prev_bill_id = 0;
	PPBillExporter b_e;
	BillViewItem view_item;
	SString msg_buf, fmt_buf, temp_buf;
	STempBuffer result_str(1024);
	char   errmsg_[1024];
	PrvdrDllLink * p_prvd_dll_link = 0;
	TSCollection <PrvdrDllLink> exp_dll_coll;
	SString doc_type;
	PPLogger logger;
	StringSet result_file_list;
	PPIDArray bill_id_list;
	PPID   single_loc_id = -1;
	int    is_there_bnkpaym = 0;
	for(InitIteration(OrdByDefault); NextIteration(&view_item) > 0;) {
		// @v9.4.3 if(view_item.Flags & BILLF_GOODS || IsDraftOp(view_item.OpID) || view_item.Flags & BILLF_BANKING) {
		if(IsGoodsDetailOp(view_item.OpID) || view_item.Flags & BILLF_BANKING) { // @v9.4.3
			bill_id_list.add(view_item.ID);
			if(view_item.LocID) {
				if(single_loc_id < 0)
					single_loc_id = view_item.LocID;
				else if(single_loc_id != view_item.LocID)
					single_loc_id = 0;
			}
			if(view_item.Flags & BILLF_BANKING)
				is_there_bnkpaym = 1;
		}
	}
	if(bill_id_list.getCount()) {
		PPBillPacket pack;
		//
		// Первый документ необходимо извлечь из БД для того, чтобы инициализировать возможные шаблоны переменных
		// в наименовании файла экспорта.
		//
		THROW(P_BObj->ExtractPacketWithFlags(bill_id_list.get(0), &pack, BPLD_FORCESERIALS) > 0);
		if(!is_there_bnkpaym)
			b_e.DisabledOptions |= PPBillImpExpBaseProcessBlock::fPaymOrdersExp;
		THROW(r = b_e.Init(pBillParam, pBRowParam, &pack, &result_file_list));
		if(r > 0) {
			PPBillImpExpParam bill_param;
			PPBillImpExpParam brow_param;
			bill_param = b_e.BillParam;
			brow_param = b_e.BRowParam;
			if(b_e.GetIEBill())
				bill_param.FileName = b_e.GetIEBill()->GetPreservedOrgFileName();
			if(b_e.GetIEBRow())
				brow_param.FileName = b_e.GetIEBRow()->GetPreservedOrgFileName();
			if(b_e.BillParam.PredefFormat) {
				if(oneof3(b_e.BillParam.PredefFormat, PPBillImpExpParam::pfNalogR_Invoice, PPBillImpExpParam::pfNalogR_REZRUISP, PPBillImpExpParam::pfNalogR_SCHFDOPPR)) {
					PPWait(1);
					for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
						const  PPID bill_id = bill_id_list.get(_idx);
						int    err = 0;
						if(P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
							int    r = 0;
							THROW(r = b_e.Init(&bill_param, &brow_param, &pack, 0 /*&result_file_list*/));
							if(b_e.BillParam.PredefFormat == PPBillImpExpParam::pfNalogR_Invoice) {
								r = WriteBill_NalogRu2_Invoice(pack, b_e.BillParam.FileName);
							}
							else if(b_e.BillParam.PredefFormat == PPBillImpExpParam::pfNalogR_REZRUISP) {
								r = WriteBill_NalogRu2_DP_REZRUISP(pack, b_e.BillParam.FileName);
							}
							else if(b_e.BillParam.PredefFormat == PPBillImpExpParam::pfNalogR_SCHFDOPPR) {
								r = WriteBill_NalogRu2_UPD(pack, b_e.BillParam.FileName);
							}
							if(r > 0)
								result_file_list.add(b_e.BillParam.FileName);
						}
						PPWaitPercent(_idx+1, bill_id_list.getCount());
					}
					PPWait(0);
				}
			}
			else if(b_e.Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
				THROW(Helper_ExportBnkOrder(b_e.CfgNameBill, logger));
			}
			else if(b_e.Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp) {
				long   cflags = (b_e.Flags & PPBillImporter::fTestMode) ? PPEgaisProcessor::cfDebugMode : 0;
				if(b_e.Flags & PPBillImporter::fEgaisVer3)
					cflags |= PPEgaisProcessor::cfVer3;
				PPEgaisProcessor ep(cflags, &logger, 0);
				THROW(ep);
				THROW(ep.CheckLic());
				{
					PPBillExportFilt sbp;
					sbp.IdList = bill_id_list;
					sbp.LocID = (single_loc_id > 0) ? single_loc_id : Filt.LocList.GetSingle();
					TSVector <PPEgaisProcessor::UtmEntry> utm_list; // @v9.8.11 TSArray-->TSVector
					THROW(ep.GetUtmList(sbp.LocID, utm_list));
					for(uint i = 0; i < utm_list.getCount(); i++) {
						ep.SetUtmEntry(sbp.LocID, &utm_list.at(i), &sbp.Period);
						ep.SendBillActs(sbp);
						ep.SendBillRepeals(sbp); // @v9.2.8
						ep.SendBills(sbp);
						ep.SetUtmEntry(0, 0, 0);
					}
				}
			}
			else {
				SString edi_prvdr_symb;
				PPWait(1);
				// @vmiller {
				// Запомним начальное значение конфигурации
				{
					const PPImpExp * p_iebill = b_e.GetIEBill();
					doc_type = p_iebill ? p_iebill->GetParamConst().Name.cptr() : 0; // Тип документа (перечисление в PPTXT_EDIEXPCMD)
				}
				if(b_e.BillParam.BaseFlags & PPImpExpParam::bfDLL) {
					SString prev_bill_code;
					SString ini_file_name;
					THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
					PPIniFile ini_file(ini_file_name, 0, 1, 1);
					//
					// Проверка из-за экспорта через job-сервер, ибо там от него не передается флаг fEdiImpExp
					// Если имя doc_type соответствует одной из строк перечисления PPTXT_EDIEXPCMD, то экспорт
					// происходит в режиме EDI
					//
					if(!(b_e.Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)) {
						StringSet ss(';', PPLoadTextS(PPTXT_EDIEXPCMD, temp_buf));
						for(uint i = 0, f_exit = 0; !f_exit && ss.get(&i, temp_buf);) {
							uint j = 0;
							StringSet ss1(',', temp_buf);
							ss1.get(&j, temp_buf.Z());
							ss1.get(&j, temp_buf.Z());
							ss1.get(&j, temp_buf.Z());
							if(temp_buf.CmpNC(doc_type) == 0) {
								b_e.Flags |= PPBillImpExpBaseProcessBlock::fEdiImpExp;
								f_exit = 1;
							}
						}
					}
					for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
						const  PPID bill_id = bill_id_list.get(_idx);
						int    err = 0;
						if(P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
							// Берем начальное значение BillParam
							b_e.BillParam = bill_param;
							PPSupplAgreement suppl_agt;
							//
							// Получаем соглашение поставщика
							// @v8.1.8 (конструктор PPSupplAgreement инициализирует объект) MEMSZERO(suppl_agr);
							//
							PPObjArticle::GetSupplAgreement(pack.Rec.Object, &suppl_agt, 1);
							suppl_agt.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, edi_prvdr_symb);
							//
							// Так как в списке документов могут быть документы с разными поставщиками и, соответственно,
							// провайдерами, то, чтобы 100500 раз не инициализировать и разрушать dll, инициализируем
							// ее для каждого провайдера по одному разу и запомним указатель на библиотеку в коллекцию.
							// Смотрим коллекцию на наличие инициализированной бибилиотеки
							//
							if((b_e.Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) && !edi_prvdr_symb.NotEmptyS()) {
								GetArticleName(pack.Rec.Object, temp_buf.Z());
								PPSetError(PPERR_EMPTY_EDISYMB, temp_buf);
								err = 1;
							}
							else {
								PPLoadTextS(PPTXT_EDIPRVDRSYMBRECEIVED, msg_buf).Space().CatQStr(edi_prvdr_symb); // @vmiller
								logger.Log(msg_buf); // @vmiller new
								uint   dll_found = 0;
								for(uint i = 0; i < exp_dll_coll.getCount(); i++) {
									const PrvdrDllLink * p_item = exp_dll_coll.at(i);
									if(p_item && edi_prvdr_symb.CmpNC(p_item->PrvdrSymb) == 0) {
										dll_found = 1;
										dll_pos = i;
										break;
									}
								}
								//if(!dll_found) {
								b_e.BillParam = bill_param;

								ImpExpDll * p_exp_dll = new ImpExpDll;
								THROW_MEM(p_exp_dll);
								THROW_MEM(p_prvd_dll_link = new PrvdrDllLink);
								edi_prvdr_symb.CopyTo(p_prvd_dll_link->PrvdrSymb, sizeof(p_prvd_dll_link->PrvdrSymb));
								p_prvd_dll_link->P_ExpDll = p_exp_dll;
								//
								// Считываем нужную конфигурацию
								// Причем считываем всегда! Ибо в b_e.BillParam должна быть актуальная для текущего провайдера инфа
								// На следующем круге провайдер может поменяться, и, соответственно, настройки тоже.
								// При этом не важно, втречается нам провайдер первый раз или мы уже с ним работали.
								// Сначала сформруем название конфигурации
								//
								if(b_e.Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
									b_e.BillParam.ProcessName(1, temp_buf.Z());
									temp_buf.Cat("DLL_").Cat(edi_prvdr_symb).CatChar('_').Cat(doc_type);
									// Теперь читаем параметры конфигурации из ini-файла
									if(!b_e.BillParam.ReadIni(&ini_file, temp_buf, 0)) {
										PPSetError(PPERR_IMPEXPCFGRDFAULT, temp_buf);
										err = 1;
									}
									else if(!b_e.BRowParam.ReadIni(&ini_file, temp_buf, 0)) {
										PPSetError(PPERR_IMPEXPCFGRDFAULT, temp_buf);
										err = 1;
									}
									p_prvd_dll_link->ParamDll = b_e.BillParam.ImpExpParamDll;
								}
								if(!dll_found) {
									exp_dll_coll.insert(p_prvd_dll_link);
									dll_pos = exp_dll_coll.getCount() - 1;
									if(!err) {
										b_e.BillParam.ImpExpParamDll.FileName = b_e.BillParam.FileName;
										PrvdrDllLink * p_prvdr_item = exp_dll_coll.at(dll_pos);
										if(p_prvdr_item->P_ExpDll->InitLibrary(b_e.BillParam.ImpExpParamDll.DllPath, 1)) {
											int    sess_id = 0;
											Sdr_ImpExpHeader hdr;
											PPVersionInfo vers_info;
											vers_info.GetProductName(temp_buf.Z());
											temp_buf.CopyTo(hdr.SrcSystemName, sizeof(hdr.SrcSystemName));
											vers_info.GetVersionText(hdr.SrcSystemVer, sizeof(hdr.SrcSystemVer));
											b_e.BillParam.ImpExpParamDll.Login.CopyTo(hdr.EdiLogin, sizeof(hdr.EdiLogin));
											b_e.BillParam.ImpExpParamDll.Password.CopyTo(hdr.EdiPassword, sizeof(hdr.EdiPassword));
											if(p_prvdr_item->P_ExpDll->InitExport(&hdr, b_e.BillParam.ImpExpParamDll.FileName, &sess_id))
												p_prvdr_item->SessId = sess_id;
											else
												err = 1;
										}
										else
											err = 1;
									}
								}
							}
							if(!err) {
								PrvdrDllLink * p_prvdr_item = (dll_pos < (int)exp_dll_coll.getCount()) ? exp_dll_coll.at(dll_pos) : 0;
								ImpExpDll * p_exp_dll = p_prvdr_item ? p_prvdr_item->P_ExpDll : 0;
								if(!b_e.PutPacket(&pack, (p_prvdr_item ? p_prvdr_item->SessId : 0), p_exp_dll)) {
									if(b_e.Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
										// В методе PutPacket() вызывается внешний метод dll SetExportObj(), который
										// начинает формировать новый документ для отправки, но предварительно
										// он отправляет старый. То есть если на этом этапе не отправился документ, то это
										// это не текущий, а предыдущий
										logger.LogMsgCode(mfError, PPERR_IMPEXP_BILL, prev_bill_code);
									}
									else
										logger.LogMsgCode(mfError, PPERR_IMPEXP_BILL, pack.Rec.Code);
								}
								else {
									PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
									PPFormatT(PPTXT_LOG_EXPBILL_ITEM, &msg_buf, temp_buf.cptr());
									logger.Log(msg_buf);
								}
							}
							prev_bill_code = pack.Rec.Code;
						}
						else
							err = 1;
						if(err) {
							logger.LogLastError();
						}
						PPWaitPercent(_idx+1, bill_id_list.getCount());
					}
					for(uint i = 0; i < exp_dll_coll.getCount(); i++) {
						PrvdrDllLink * p_item = exp_dll_coll.at(i);
						if(p_item && p_item->P_ExpDll && p_item->P_ExpDll->IsInited()) {
							b_e.BillParam.ImpExpParamDll = p_item->ParamDll;
							if(!b_e.CheckBillsWasExported(p_item->P_ExpDll)) {
								errmsg_[0] = 0;
								p_item->P_ExpDll->GetErrorMessage(errmsg_, sizeof(errmsg_));
								PPSetError(PPERR_IMPEXP_DLL, msg_buf.Z().Cat(errmsg_).Transf(CTRANSF_OUTER_TO_INNER));
								logger.LogLastError();
							}
							else if(!p_item->P_ExpDll->FinishImpExp()) {
								errmsg_[0] = 0;
								p_item->P_ExpDll->GetErrorMessage(errmsg_, sizeof(errmsg_));
								PPSetError(PPERR_IMPEXP_DLL, msg_buf.Z().Cat(errmsg_).Transf(CTRANSF_OUTER_TO_INNER));
								logger.LogLastError();
							}
						}
					}
					//THROW(b_e.SignBill());
					ok = 1;
				}
				else {
					const PPID inet_acc_id = b_e.Tp.InetAccID;
					const StrAssocArray inet_addr_list = b_e.Tp.AddrList;
					int   use_mail_addr_by_context = 0;
					SString mail_subj = b_e.Tp.Subject;
					mail_subj.SetIfEmpty("No Subject").Transf(CTRANSF_INNER_TO_UTF8);
					if(inet_acc_id && inet_addr_list.getCount()) {
						for(uint ai = 0; !use_mail_addr_by_context && ai < inet_addr_list.getCount(); ai++) {
							temp_buf = inet_addr_list.Get(ai).Txt;
							if(temp_buf.IsEqiAscii("@bycontext") || temp_buf == "@@")
								use_mail_addr_by_context = 1;
						}
					}
					if(b_e.BillParam.Flags & PPBillImpExpParam::fExpOneByOne) {
						StringSet local_result_file_list;
						for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
							const  PPID bill_id = bill_id_list.get(_idx);
							if(P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
								local_result_file_list.clear();
								THROW(r = b_e.Init(&bill_param, &brow_param, &pack, &local_result_file_list));
								result_file_list.add(local_result_file_list);
								{
									PPImpExp * p_iebill = b_e.GetIEBill();
									PPImpExp * p_iebrow = b_e.GetIEBRow();
									if(!b_e.PutPacket(&pack, 0, 0)) {
										logger.LogMsgCode(mfError, PPERR_IMPEXP_BILL, pack.Rec.Code);
									}
									else {
										PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
										PPFormatT(PPTXT_LOG_EXPBILL_ITEM, &msg_buf, temp_buf.cptr());
										logger.Log(msg_buf);
									}
									CALLPTRMEMB(p_iebrow, CloseFile());
									if(p_iebill) {
										p_iebill->CloseFile();
										b_e.BillParam.DistributeFile(&logger);
										if(p_iebrow && fileExists(b_e.BRowParam.FileName) && b_e.BRowParam.FileName.CmpNC(b_e.BillParam.FileName) != 0)
											b_e.BRowParam.DistributeFile(&logger);
									}
								}
								if(use_mail_addr_by_context && pack.GetContextEmailAddr(temp_buf) > 0 && local_result_file_list.getCount()) {
									if(!PutFilesToEmail2(&local_result_file_list, inet_acc_id, temp_buf, mail_subj, 0))
										logger.LogLastError();
								}
							}
							else
								logger.LogLastError();
							PPWaitPercent(_idx+1, bill_id_list.getCount());
						}
					}
					else {
						PPImpExp * p_iebill = b_e.GetIEBill();
						PPImpExp * p_iebrow = b_e.GetIEBRow();
						for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
							const  PPID bill_id = bill_id_list.get(_idx);
							if(P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
								if(!b_e.PutPacket(&pack, 0, 0)) {
									logger.LogMsgCode(mfError, PPERR_IMPEXP_BILL, pack.Rec.Code);
								}
								else {
									PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
									PPFormatT(PPTXT_LOG_EXPBILL_ITEM, &msg_buf, temp_buf.cptr());
									logger.Log(msg_buf);
								}
							}
							else
								logger.LogLastError();
							PPWaitPercent(_idx+1, bill_id_list.getCount());
						}
						CALLPTRMEMB(p_iebrow, CloseFile());
						if(p_iebill) {
							p_iebill->CloseFile();
							b_e.BillParam.DistributeFile(&logger);
							if(p_iebrow && fileExists(b_e.BRowParam.FileName) && b_e.BRowParam.FileName.CmpNC(b_e.BillParam.FileName) != 0)
								b_e.BRowParam.DistributeFile(&logger);
						}
						//THROW(b_e.SignBill());
						ok = 1;
					}
					if(inet_acc_id && inet_addr_list.getCount() && result_file_list.getCount()) {
						temp_buf.Z();
						for(uint ai = 0; !use_mail_addr_by_context && ai < inet_addr_list.getCount(); ai++) {
							temp_buf = inet_addr_list.Get(ai).Txt;
							if(temp_buf.IsEqiAscii("@bycontext") || temp_buf == "@@")
								temp_buf.Z();
							else
								break;
						}
						if(temp_buf.NotEmptyS() && !PutFilesToEmail2(&result_file_list, inet_acc_id, temp_buf, mail_subj, 0))
							logger.LogLastError();
					}
				}
			}
		}
	}
	CATCH
		if(dll_pos < (int)exp_dll_coll.getCount()) {
			ImpExpDll * p_ied = exp_dll_coll.at(dll_pos)->P_ExpDll;
			if(p_ied && p_ied->IsInited()) {
				errmsg_[0] = 0;
				p_ied->GetErrorMessage(errmsg_, sizeof(errmsg_));
				PPSetError(PPERR_IMPEXP_DLL, msg_buf.Z().Cat(errmsg_).Transf(CTRANSF_OUTER_TO_INNER));
			}
		}
		logger.LogLastError(); // @v9.2.10
		ok = PPErrorZ();
	ENDCATCH
	for(uint i = 0; i < exp_dll_coll.getCount(); i++) {
		ImpExpDll * p_ied = exp_dll_coll.at(i)->P_ExpDll;
		b_e.BillParam.ImpExpParamDll = exp_dll_coll.at(i)->ParamDll;
		if(p_ied && p_ied->IsInited())
			b_e.CheckBillsWasExported(p_ied);
		ZDELETE(exp_dll_coll.at(i)->P_ExpDll);
	}
	PPWait(0);
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	return ok;
}

int SLAPI PPViewBill::Helper_ExportBnkOrder(const char * pSection, PPLogger & rLogger)
{
	int    ok = -1;
	SString section = pSection;
	if(section.NotEmptyS()) {
		SString str_fmt, msg, str_dt;
		BillViewItem item;
		PPIDArray id_list;
		DateRange period;
		period.low = MAXDATE;
		period.upp = encodedate(1, 1, 1900);
		PPWait(1);
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			if(item.Flags & BILLF_BANKING) {
				id_list.addUnique(item.ID);
				period.AdjustToDate(item.Dt);
			}
			else {
				str_dt.Z().Cat(item.Dt);
				msg.Printf(PPLoadTextS(PPTXT_BILLNOTBANKING, str_fmt), item.Code, str_dt.cptr());
				rLogger.Log(msg);
			}
		}
		const uint cnt = id_list.getCount();
		if(cnt) {
			ClientBankExportDef cbed(&period);
			THROW(cbed.ReadDefinition(section));
			THROW(cbed.CreateOutputFile());
			THROW(cbed.PutHeader());
			ok = 1;
			for(uint i = 0; i < cnt; i++) {
				PPBillPacket pack;
				const PPID id = id_list.get(i);
				THROW(P_BObj->ExtractPacket(id, &pack) > 0 && cbed.PutRecord(&pack, 0, &rLogger));
				PPWaitPercent(i+1, cnt);
			}
			THROW(cbed.PutEnd());
			THROW(ok);
		}
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PPViewBill::ExportBnkOrder()
{
	int    ok = -1;
	SString section;
	PPLogger logger;
	if(CliBnkSelectCfgDialog(1, section) > 0) {
		ok = Helper_ExportBnkOrder(section, logger);
	}
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	return ok;
}

static int SLAPI SCardNumDlg(PPSCardPacket & rScPack, CCheckTbl::Rec * pChkRec, int isDraft)
{
	int    ok = -1;
	char   sc_code[24];
	PPObjSCard sc_obj;
	CCheckTbl::Rec cc_rec;
	TDialog * p_dlg = new TDialog(isDraft ? DLG_SCARDCHK : DLG_SCARDNUM);
	CCheckCore cc_core;
	LDATE  dt = ZERODATE;
	MEMSZERO(cc_rec);
	THROW(CheckDialogPtr(&p_dlg));
	if(isDraft) {
		SetupCalCtrl(CTLCAL_SCARDNUM_CHKDT, p_dlg, CTL_SCARDNUM_CHKDT, 4);
		p_dlg->setCtrlData(CTL_SCARDNUM_CHKDT, &(dt = getcurdate_()));
	}
	while(ok < 0 && ExecView(p_dlg) == cmOK) {
		ushort sel = 0;
		MEMSZERO(cc_rec);
		p_dlg->getCtrlData(sel = CTL_SCARDNUM_SCARDNUM, sc_code);
		if(strip(sc_code)[0] != 0) {
			SString added_msg;
			SCardTbl::Rec sc_rec_;
			PPSetAddedMsgString(added_msg.CatEq("Code", sc_code).Quot('[', ']'));
			if(cc_core.Cards.SearchCode(0, sc_code, &sc_rec_) != 1)
				PPError(PPERR_SCARDNOTFOUND, sc_code);
			else if(sc_rec_.Flags & SCRDF_CLOSEDSRV)
				PPError(PPERR_SCRDCLOSEDSRV);
			else if(sc_obj.GetPacket(sc_rec_.ID, &rScPack) > 0) {
				if(isDraft) {
					long   chk_no = 0, cash_no = 0;
					p_dlg->getCtrlData(CTL_SCARDNUM_CHKDT,   &dt);
					p_dlg->getCtrlData(CTL_SCARDNUM_CHKNUM,  &chk_no);
					p_dlg->getCtrlData(CTL_SCARDNUM_CASHNUM, &cash_no);
					if(chk_no) {
						if(dt == ZERODATE)
							sel = (PPError(PPERR_CHKDATENEEDED), CTL_SCARDNUM_CHKDT);
						else if(!checkdate(&dt))
							sel = (PPError(PPERR_SLIB), CTL_SCARDNUM_CHKDT);
						else if(chk_no == 0)
							sel = (PPError(PPERR_CHKNUMNEEDED), CTL_SCARDNUM_CHKNUM);
						else {
							int    chks_qtty = 0;
							LDATETIME dttm;
							PPIDArray chk_ary;
							CCheckTbl::Rec tmp_cc_rec;
							MEMSZERO(tmp_cc_rec);
							dttm.Set(dt, ZEROTIME);
							if(cc_core.GetListByCard(sc_rec_.ID, &dttm, &chk_ary) > 0) {
								for(uint i = 0; i < chk_ary.getCount(); i++) {
									THROW(cc_core.Search(chk_ary.at(i), &tmp_cc_rec));
									if(tmp_cc_rec.Code == chk_no && (!cash_no || tmp_cc_rec.CashID == cash_no) && (++chks_qtty == 1))
										cc_rec = tmp_cc_rec;
								}
							}
							if(chks_qtty == 0)
								sel = (PPError(PPERR_CHKBYSCARDNFOUND), CTL_SCARDNUM_SCARDNUM);
							else if(chks_qtty == 1) {
								ok = 2;
							}
							else
								sel = (PPError(PPERR_CASHNUMNEEDED), CTL_SCARDNUM_CASHNUM);
						}
					}
					else
						ok = 1;
				}
				else
					ok = 1;
			}
			else
				PPError();
		}
		else
			PPError(PPERR_INVSCARDNUM);
		if(ok <= 0)
			p_dlg->selectCtrl(sel);
	}
	CATCHZOK
	delete p_dlg;
	ASSIGN_PTR(pChkRec, cc_rec);
	return ok;
}

static int SLAPI SCardInfoDlg(PPSCardPacket & rScPack, PPID * pOpID, long flags, int withoutPsn)
{
	int    without_person = 0;
	int    ok = -1, valid_data = 0;
	SString temp_buf;
	SString added_msg;
	PPID   op_id = 0;
	PPIDArray op_list, op_type_list;
	PPOprKind opr_kind;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	PPObjPerson psn_obj;
	PPPersonPacket psn_pack;
	PPObjSCard sc_obj;
	TDialog * p_dlg = 0;

	if(flags & BillFilt::fDraftOnly)
		op_type_list.add(PPOPT_DRAFTEXPEND);
	else if(flags & BillFilt::fOrderOnly)
		op_type_list.add(PPOPT_GOODSORDER);
	else if(flags & BillFilt::fAccturnOnly)
		op_type_list.add(PPOPT_ACCTURN);
	else
		op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_PAYMENT, 0L);
	if(!withoutPsn) {
		THROW_PP(rScPack.Rec.PersonID, PPERR_SCARDPERSONNODEF);
	}
	PPSetAddedMsgString(ideqvalstr(rScPack.Rec.PersonID, added_msg));
	if(!withoutPsn) {
		THROW(psn_obj.GetPacket(rScPack.Rec.PersonID, &psn_pack, 0) > 0);
	}
	for(op_id = 0; EnumOperations(0, &op_id, &opr_kind) > 0;) {
		if(op_type_list.lsearch(opr_kind.OpTypeID)) {
			if(withoutPsn) {
				PPID sell_accsheet = GetSellAccSheet();
				if(!opr_kind.AccSheetID || opr_kind.AccSheetID == sell_accsheet)
					op_list.add(op_id);
			}
			else if(acs_obj.Fetch(opr_kind.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON) {
				if(psn_pack.Kinds.lsearch(acs_rec.ObjGroup) > 0)
					op_list.add(op_id);
			}
		}
	}
	THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_SCARDINFO))));
	p_dlg->setCtrlData(CTL_SCARDINFO_CODE, rScPack.Rec.Code);
	p_dlg->setCtrlData(CTL_SCARDINFO_NAME, psn_pack.Rec.Name);
	{
		rScPack.GetExtStrData(rScPack.extssPassword, temp_buf);
		p_dlg->setCtrlString(CTL_SCARDINFO_PSW,  temp_buf);
	}
	SetupOprKindCombo(p_dlg, CTLSEL_SCARDINFO_OP, 0, 0, &op_list, OPKLF_OPLIST);
	p_dlg->disableCtrls(1, CTL_SCARDINFO_CODE, CTL_SCARDINFO_NAME, CTL_SCARDINFO_PSW, 0);
	{
		SString info_buf, temp_buf;
		if(sc_obj.IsCreditCard(rScPack.Rec.ID) > 0) {
			double rest = 0.0;
			sc_obj.P_Tbl->GetRest(rScPack.Rec.ID, ZERODATE, &rest);
			PPLoadString("crdcard", temp_buf);
			info_buf.Cat(temp_buf).CatDiv('.', 2);
			PPLoadString("rest", temp_buf);
			info_buf.Cat(temp_buf).CatDiv('=', 1).Cat(rest);
		}
		else {
			PPLoadString("discard", temp_buf);
			info_buf.Cat(temp_buf).CatDiv('.', 2);
			PPLoadString("discount", temp_buf);
			info_buf.Cat(temp_buf).CatDiv('=', 1).Cat(fdiv100i(rScPack.Rec.PDis)).CatChar('%');
		}
		p_dlg->setStaticText(CTL_SCARDINFO_ST_INFO, info_buf);
	}
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		op_id = p_dlg->getCtrlLong(CTLSEL_SCARDINFO_OP);
		if(op_id > 0) {
			ASSIGN_PTR(pOpID, op_id);
			ok = valid_data = 1;
		}
		else
			PPErrorByDialog(p_dlg, CTL_SCARDINFO_OP, PPERR_INVOP);
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

int SLAPI PPViewBill::AddBySCard(PPID * pID)
{
	int    ok = -1, r = 0;
	const  int is_draft = BIN(Filt.Flags & BillFilt::fDraftOnly);
	int    sc_num_ret = 0;
	PPID   op_id = 0;
	PPObjSCard sc_obj;
	//SCardTbl::Rec  sc_rec;
	//MEMSZERO(sc_rec);
	PPSCardPacket sc_pack;
	CCheckTbl::Rec cc_rec;
	MEMSZERO(cc_rec);
	THROW((sc_num_ret = SCardNumDlg(sc_pack, &cc_rec, is_draft)));
	if(sc_num_ret > 0) {
		int    without_psn = BIN(!sc_pack.Rec.PersonID && CONFIRM(PPCFM_SCARDWITHOUTPSN));
		THROW((r = SCardInfoDlg(sc_pack, &op_id, Filt.Flags, without_psn)));
		if(r > 0) {
			PPID   b_id = 0;
			PPOprKind    opr_kind;
			ArticleTbl::Rec ar_rec;
			THROW(GetOpData(op_id, &opr_kind) > 0);
			PPSetAddedMsgObjName(PPOBJ_PERSON, sc_pack.Rec.PersonID);
			if(!without_psn) {
				THROW_PP(ArObj.P_Tbl->SearchObjRef(opr_kind.AccSheetID, sc_pack.Rec.PersonID, &ar_rec), PPERR_OBJNFOUND);
				Filt.ObjectID = ar_rec.ID;
			}
			THROW((r = P_BObj->AddGoodsBillByFilt(&b_id, &Filt, op_id, sc_pack.Rec.ID, (sc_num_ret == 2) ? &cc_rec : 0)));
			ASSIGN_PTR(pID, b_id);
			ok = (r == cmOK) ? 1 : ok;
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::Browse(int modeless)
{
	int    ok = 1;
	const  PPConfig & r_cfg = LConfig;
	// @v9.8.11 const  long save_state = P_BObj->State;
	const  PPID save_loc   = r_cfg.Location;
	PPID   single_loc_id = LocList_.getSingle();
	Filt.Period.Actualize(ZERODATE);
	THROW((Filt.Flags & BillFilt::fDebtOnly) || AdjustPeriodToRights(Filt.Period, 0));
	if(single_loc_id && single_loc_id != r_cfg.Location)
		DS.SetLocation(single_loc_id);
	ok = PPView::Browse(modeless);
	CATCHZOK
	if(!modeless) {
		DS.SetLocation(save_loc);
		// @v9.8.11 P_BObj->State = save_state;
	}
	return ok;
}

int SLAPI PPViewBill::SelectBillListForm(uint * pForm, int * pIsExt, IterOrder * pOrder)
{
	int    ok = -1, r;
	uint   form_id = 0;
	ushort v = 0, o = 0;
	TDialog * dlg = 0;
	PPID   op_id = Filt.OpID;
	if(Filt.PoolBillID && Filt.AssocID) {
		PPID   comm_op_id;
		GetCommonPoolAttribs(0, 0, &comm_op_id, 0);
		if(comm_op_id > 0)
			op_id = comm_op_id;
	}
	if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) {
		form_id = REPORT_DEBTCARD;
		ok = 1;
	}
	else if(op_id == 0 || GetOpType(Filt.OpID) == PPOPT_PAYMENT) {
		form_id = REPORT_BILLLIST;
		ok = 1;
	}
	else if(IsIntrExpndOp(op_id) /**/ || IsIntrOp(op_id) == INTRRCPT /**/) {
		if(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNBLSTI)))) {
			dlg->setCtrlUInt16(CTL_PRNBLST_FLAGS, 0);
			dlg->setCtrlUInt16(CTL_PRNBLST_ORDER, 0);
			if(ExecView(dlg) == cmOK) {
				v = dlg->getCtrlUInt16(CTL_PRNBLST_FLAGS);
				o = dlg->getCtrlUInt16(CTL_PRNBLST_ORDER);
				if(v == 1)
					form_id = REPORT_EXTINTRBILLLIST;
				else
					form_id = REPORT_INTRBILLLIST;
				if(o == 1)
					*pOrder = OrdByCode;
				else if(o == 2)
					*pOrder = OrdByObjectName;
				else
					*pOrder = OrdByDate;
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = 0;
	}
	else {
		if(CheckDialogPtr(&(dlg = new TDialog(DLG_PRNBLST)))) {
			if(Filt.Flags & BillFilt::fShowDebt)
				v = 3;
			dlg->setCtrlData(CTL_PRNBLST_AMTTYPE, &v);
			dlg->setCtrlData(CTL_PRNBLST_ORDER,   &o);
			r = ExecView(dlg);
			dlg->getCtrlData(CTL_PRNBLST_AMTTYPE, &v);
			dlg->getCtrlData(CTL_PRNBLST_ORDER,   &o);
			delete dlg;
			dlg = 0;
			if(r == cmOK) {
				*pIsExt = 0;
				switch(v) {
					case 0: form_id = REPORT_OABILLLIST; break;
					case 1: form_id = REPORT_OBILLLIST;  break;
					case 2: form_id = REPORT_EXTBILLLIST; *pIsExt = 1; break;
					case 3: form_id = REPORT_DBILLLIST;  break;
				}
				if(o == 1)
					*pOrder = OrdByCode;
				else if(o == 2)
					*pOrder = OrdByObjectName;
				else
					*pOrder = OrdByDate;
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = 0;
	}
	delete dlg;
	ASSIGN_PTR(pForm, form_id);
	return ok;
}

int SLAPI PPViewBill::Transmit(PPID id, int transmitKind)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(transmitKind == 0) {
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			BillViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWait(1);
			for(InitIteration(OrdByDefault); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
				objid_ary.Add(PPOBJ_BILL, item.ID);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	else if(transmitKind == 1) {
		PPIDArray id_list;
		id_list.add(id);
		THROW(SendCharryObject(PPDS_CRRBILL, id_list));
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

/*void LogObjToTransmit(PPID objType, PPID objID, const char * pName)
{
	SString msg_buf;
}*/

// static
int SLAPI PPViewBill::TransmitByFilt(const BillFilt * pFilt, const ObjTransmitParam * pParam)
{
	int    ok = -1, r = 1;
	uint   val = 0;
	BillFilt filt;
	ObjTransmitParam param;
	filt.SetupBrowseBillsType(filt.Bbt = bbtUndef);
	RVALUEPTR(filt, pFilt);
	RVALUEPTR(param, pParam);
	if(!pFilt && (r = SelectorDialog(DLG_BBTSEL, CTL_BBTSEL_TYPE, &val)) > 0)
		filt.SetupBrowseBillsType(filt.Bbt = (BrowseBillsType)val);
	if((pFilt && pParam) || (r > 0 && ObjTransmDialogExt(DLG_OBJTRANSM, PPVIEW_BILL, &param, &filt) > 0)) {
		BillViewItem item;
		PPViewBill   view;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		THROW(view.Init_(&filt));
		for(view.InitIteration(OrdByDefault); view.NextIteration(&item) > 0; PPWaitPercent(view.GetCounter())) {
			objid_ary.Add(PPOBJ_BILL, item.ID);
		}
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}
//
// Descr: Блок данных для печати итогов выборки документов, передаваемый объекту PPALDD_BillTotal
//
struct BillTotalPrintData {
	const BillTotal * P_Total;
	const BillFilt  * P_Filt;
};

int SLAPI PPViewBill::PrintTotal(const BillTotal * pTotal)
{
	BillTotalPrintData btpd;
	btpd.P_Total = pTotal;
	btpd.P_Filt = &Filt;
	PView  pv(&btpd);
	return PPAlddPrint(REPORT_BILLTOTAL, &pv, 0);
}

// AHTOXA {
// @<<PPALDD_BillInfoList::NextIteration
int SLAPI PPViewBill::GetPacket(PPID billID, PPBillPacket * pPack) const
{
	return P_BObj ? P_BObj->ExtractPacket(billID, pPack) : 0;
}
//
// Descr: Блок данных для печати специальной информации по списку документов
//
struct BillInfoListPrintData {
	BillInfoListPrintData(PPViewBill * pV, PPBillPacket * pPack) : P_V(pV), P_Pack(pPack)
	{
	}
	PPViewBill * P_V;
	PPBillPacket * P_Pack;
};

int SLAPI PPViewBill::PrintBillInfoList()
{
	int    ok = 1;
	BillInfoListPrintData bilpd(this, 0);
	PView  pv(&bilpd);
	ok = PPAlddPrint(REPORT_BILLINFOLIST, &pv, 0);
	ZDELETE(((BillInfoListPrintData*)pv.Ptr)->P_Pack);
	return ok;
}
// } AHTOXA

int SLAPI PPViewBill::Print()
{
	int    ok = 1, reply, ext = 0;
	uint   form;
	IterOrder order = OrdByDefault;
	if(Filt.SortOrder == BillFilt::ordByDate)
		order = OrdByDate;
	else if(Filt.SortOrder == BillFilt::ordByCode)
		order = OrdByCode;
	else if(Filt.SortOrder == BillFilt::ordByObject)
		order = OrdByObjectName;
	THROW(reply = SelectBillListForm(&form, &ext, &order));
	if(reply > 0) {
		PView pv(this);
		PPReportEnv env;
		env.Sort = order;
		PPAlddPrint(form, &pv, &env);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewBill::UpdateTempTable(PPID id)
{
	int    ok = 1;
	BillTbl::Rec  rec;
	int    check_list_item_for_filt = 1;
	const  PPIDArray * p_list = 0;
	if(IdList.IsExists()) {
		p_list = &IdList.Get();
		check_list_item_for_filt = (Filt.Flags & BillFilt::fBillListOnly) ? 0 : 1;
	}
	int    id_found = 0;
	if(id && P_BObj->Search(id, &rec) > 0) {
		if(check_list_item_for_filt) {
			if(CheckIDForFilt(id, &rec))
				id_found = 1;
		}
		else {
			if(p_list && p_list->lsearch(id))
				id_found = 1;
		}
	}
	else
		MEMSZERO(rec); // @v9.0.8
	if(P_TempOrd && oneof2(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject)) {
		BillTbl::Key0 k0;
		k0.ID = id;
		// @v9.0.8 MEMSZERO(rec);
		if(id) {
			if(id_found) {
				TempOrderTbl::Rec ord_rec;
				MEMSZERO(ord_rec);
				InitOrderRec(TempOrder, &rec, &ord_rec);
				if(SearchByID(P_TempOrd, 0, id, 0) > 0)
					UpdateByID(P_TempOrd, 0, id, &ord_rec, 0);
				else
					AddByID(P_TempOrd, &id, &ord_rec, 0);
			}
			else
				deleteFrom(P_TempOrd, 0, P_TempOrd->ID == id);
		}
		else
			deleteFrom(P_TempOrd, 0, P_TempOrd->ID > id);
		ok = 1;
	}
	else if(P_TempTbl) {
		if(id) {
			if(id_found) {
				TempBillTbl::Rec temp_rec;
				InitTempRec(&rec, &temp_rec);
				if((Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) || !Filt.PaymPeriod.IsZero()) {
					// @todo Debit, Credit, Saldo
				}
				if(SearchByID(P_TempTbl, 0, id, 0) > 0)
					UpdateByID(P_TempTbl, 0, id, &temp_rec, 0);
				else
					AddByID(P_TempTbl, &id, &temp_rec, 0);
			}
			else
				deleteFrom(P_TempTbl, 0, P_TempTbl->BillID == id);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPViewBill::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0;
	if(pEv) {
		if(kind == PPAdviseBlock::evBillChanged) {
			if(pEv->IsFinish() && UpdateBillList.getCount())
				update = 1;
			else
				UpdateBillList.add(pEv->ObjID);
		}
		ok = 1;
	}
	if(ok > 0 && update && pBrw) {
		UpdateBillList.sortAndUndup();
		if(IsTempTblNeeded()) {
			for(uint i = 0; i < UpdateBillList.getCount(); i++) {
				PPID   bill_id = UpdateBillList.get(i);
				UpdateTempTable(bill_id);
			}
			pBrw->refresh();
		}
		else
			pBrw->Update();
		UpdateBillList.clear();
	}
	return ok;
}

// virtual
int SLAPI PPViewBill::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_DETAIL && ppvCmd != PPVCMD_PRINT) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	uint   options = (Filt.Flags & BillFilt::fAsSelector) ? 0 : (OLW_CANEDIT|OLW_CANINSERT|OLW_CANDELETE);
	int    update = 0;
	PPID   id = 0;
	if(ok == -2) {
		const PPConfig & r_cfg = LConfig;
		BillTbl::Rec bill_rec;
		BrwHdr hdr;
		if(pHdr && ppvCmd != PPVCMD_INPUTCHAR && ppvCmd != PPVCMD_RECEIVEDFOCUS)
			hdr = *(PPViewBill::BrwHdr *)pHdr;
		else
			MEMSZERO(hdr);
		id = hdr.ID;
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				if(Filt.Flags & BillFilt::fAsSelector && pBrw->IsInState(sfModal)) {
					Filt.Sel = hdr.ID;
					ok = 1;
					pBrw->endModal(Filt.Sel ? cmOK : cmCancel);
				}
				else if(options & OLW_CANEDIT && (ok = EditItem(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_ADDITEM:
				if(options & OLW_CANINSERT && (ok = AddItem(&id)) > 0)
					update = 2;
				break;
			case PPVCMD_ADDBYSAMPLE:
				{
					PPID   new_bill_id = 0;
					if(options & OLW_CANINSERT && (ok = AddItemBySample(&new_bill_id, hdr.ID)) > 0) {
						// @v9.0.11 {
						if(CheckIDForFilt(new_bill_id, 0)) {
							// Если вновь созданный документ попадает в выборку, то
							// следующее присвоение обеспечит перевод курсора на этот документ.
							id = new_bill_id;
						}
						// } @v9.0.11
						update = 1;
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				if(options & OLW_CANDELETE && hdr.ID && (ok = DeleteItem(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_DELETEALL:
				if(options & OLW_CANDELETE && (ok = DeleteItem(0)) > 0) {
					id = 0;
					update = 1;
				}
				break;
			case PPVCMD_ADDBYSCARD:
				if((ok = AddBySCard(&id)) > 0)
					update = 1;
				break;
			case PPVCMD_EDITBILLEXT:
				ok = P_BObj->EditBillExtData(hdr.ID);
				break;
			case PPVCMD_EDITBILLFREIGHT:
				if((ok = P_BObj->EditBillFreight(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_BROWSE:
				if((ok = ShowDetails(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_DELACKBILL:
				ok = -1;
				if(hdr.ID && !(Filt.DenyFlags & BillFilt::fDenyRemove)) {
					BillTbl::Rec link_rec;
					for(DateIter di; P_BObj->P_Tbl->EnumLinks(hdr.ID, &di, BLNK_ACK, &link_rec) > 0;)
						if(link_rec.ID) {
							P_BObj->RemoveObjV(link_rec.ID, 0, PPObject::use_transaction, 0);
							ok = 1;
						}
				}
				break;
			case PPVCMD_EDITACKBILL:
				ok = -1;
				if(hdr.ID) {
					PPID ack_id = 0;
					for(DateIter di; !ack_id && P_BObj->P_Tbl->EnumLinks(hdr.ID, &di, BLNK_ACK) > 0;)
						ack_id = P_BObj->P_Tbl->data.ID;
					if(ack_id) {
						if(P_BObj->EditGoodsBill(ack_id, 0) == cmOK)
							ok = 1;
					}
					else {
						PPObjBill::AddBlock ab;
						ab.LinkBillID = hdr.ID;
						ab.OpID = SelectOprKind(0, 0, PPOPT_GOODSACK, 0L);
						if(ab.OpID > 0 && P_BObj->AddGoodsBill(&ack_id, &ab) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_ACCTURNSBYBILL:
				ok = P_BObj->ViewAccturns(hdr.ID);
				break;
			case PPVCMD_CHANGESTATUS:
				ok = P_BObj->EditBillStatus(hdr.ID);
				if(ok > 0)
					update = 1;
				break;
			case PPVCMD_SYSINFO:
				ok = P_BObj->ViewBillInfo(hdr.ID);
				if(ok > 0)
					update = 1;
				break;
			case PPVCMD_SYSJ:
				if(hdr.ID)
					ok = ViewSysJournal(PPOBJ_BILL, hdr.ID, 0);
				break;
			case PPVCMD_CHNGFLAGS:
				if((ok = ChangeFlags()) > 0)
					update = 1;
				break;
			case PPVCMD_VIEWOP:
				{
					PPObjOprKind op_obj;
					ok = (P_BObj->Search(hdr.ID, &bill_rec) > 0 && op_obj.Edit(&bill_rec.OpID, 0) == cmOK) ? 1 : -1;
				}
				break;
			case PPVCMD_BILLSBYORDER:
				if((ok = ViewBillsByOrder(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_UNITEBILLS:
				ok = -1;
				{
					PPOprKind op_rec;
					if(!SingleLocID) {
						ok = (PPError(PPERR_NONSINGLELOCFORBILLUN), 0);
					}
					else if(Filt.OpID && (Filt.ObjectID || GetOpType(Filt.OpID, &op_rec) == PPOPT_INVENTORY || !op_rec.AccSheetID))
						if(P_BObj->CheckRights(BILLOPRT_UNITEBILLS, 1)) {
							if(GetOpType(Filt.OpID) == PPOPT_GOODSEXPEND)
								ok = UniteSellBills();
							else if(GetOpType(Filt.OpID) == PPOPT_GOODSRECEIPT)
								ok = UniteReceiptBills();
							else if(GetOpType(Filt.OpID) == PPOPT_INVENTORY)
								ok = UniteInventory();
							if(ok > 0)
								update = 1;
						}
						else
							ok = PPErrorZ();
				}
				break;
			case PPVCMD_WROFFDRAFT:
				if((ok = WriteOffDraft(hdr.ID)) > 0)
					update = 1;
				break;
			case PPVCMD_INPUTCHAR:
				if(((const char*)pHdr)[0] == kbCtrlX)
					CtrlX++;
				else
					CtrlX = 0;
				if(CtrlX == 2) {
					ok = UpdateAttributes();
					CtrlX = 0;
				}
				break;
			case PPVCMD_TRFRANLZ:
				ok = -1;
				{
					TrfrAnlzFilt filt;
					PPViewTrfrAnlz v_trfr;
					if(v_trfr.EditBaseFilt(&filt) > 0) {
						PPWait(1);
						BillViewItem bitem;
						for(InitIteration(OrdByID); NextIteration(&bitem) > 0;)
							filt.RcptBillList.Add(bitem.ID);
						PPWait(0);
						ok = ::ViewTrfrAnlz(&filt);
					}
				}
				break;
			case PPVCMD_PAYMENT:
				if((ok = ViewPayments(hdr.ID, LinkedBillFilt::lkPayments)) > 0)
					update = 1;
				break;
			case PPVCMD_RECKON:
				if((ok = ViewPayments(hdr.ID, LinkedBillFilt::lkReckon)) > 0)
					update = 1;
				break;
			case PPVCMD_RENTA:
				if((ok = ViewPayments(hdr.ID, LinkedBillFilt::lkCharge)) > 0)
					update = 1;
				break;
			case PPVCMD_GOODSRET:
				ok = -1;
				if(r_cfg.Cash) {
					if(Filt.Flags & BillFilt::fCashOnly && GetCashRetOp() > 0) {
						PPObjBill::AddBlock ab;
						ab.OpID = GetCashRetOp();
						if(P_BObj->AddGoodsBill(&id, &ab) == cmOK)
							update = ok = 1;
					}
				}
				else if(P_BObj->AddRetBill(0, hdr.ID, 0) == cmOK)
					update = ok = 1;
				break;
			case PPVCMD_RECKONPAYM:
				ok = -1;
				if(P_BObj->Search(hdr.ID, &bill_rec) > 0 && CheckOpFlags(bill_rec.OpID, OPKF_RECKON)) {
					PPObjBill::ReckonParam rp(0, 0);
					rp.Flags |= rp.fPopupInfo;
					ok = P_BObj->ReckoningPaym(hdr.ID, rp, 1);
				}
				if(ok > 0)
					update = 1;
				break;
			case PPVCMD_RECKONDEBT:
				ok = -1;
				if(P_BObj->Search(hdr.ID, &bill_rec) > 0 && CheckOpFlags(bill_rec.OpID, OPKF_NEEDPAYMENT)) {
					PPObjBill::ReckonParam rp(0, 0);
					rp.Flags |= rp.fPopupInfo;
					ok = P_BObj->ReckoningDebt(hdr.ID, rp, 1);
				}
				if(ok > 0)
					update = 1;
				break;
			case PPVCMD_SETWLABEL:
				ok = -1;
				if(P_BObj->SetWLabel(hdr.ID, -1) > 0)
					update = ok = 1;
				break;
			case PPVCMD_COMPARE:
				ok = -1;
				{
					PPIDArray rh_bill_list;
					if(P_BObj->GetComplementGoodsBillList(hdr.ID, rh_bill_list) > 0)
						ViewGoodsBillCmp(hdr.ID, rh_bill_list, 0);
				}
				break;
			case PPVCMD_ATTACHBILLTOORD:   ok = AttachBillToOrder(hdr.ID); break;
			case PPVCMD_ATTACHBILLTODRAFT: ok = AttachBillToDraft(hdr.ID, pBrw); break;
			case PPVCMD_TRANSMIT:          ok = Transmit(hdr.ID, 0); break;
			case PPVCMD_TRANSMITCHARRY:    ok = Transmit(hdr.ID, 1); break;
			case PPVCMD_EXPORT:            ok = ExportGoodsBill(0, 0); break;
			case PPVCMD_PRINT:             ok = PrintBill(hdr.ID/*@v10.0.0 , 1*/); break;
			case PPVCMD_PRINTLIST:         ok = Print(); break;
			case PPVCMD_PRINTINFOLIST:     ok = PrintBillInfoList(); break;
			case PPVCMD_PRINTALLBILLS:     ok = PrintAllBills(); break;
			case PPVCMD_POSPRINTBYBILL:    ok = P_BObj->PosPrintByBill(hdr.ID); break;
			// @v10.0.0 case PPVCMD_PRINTCHECK:        ok = PrintBill(hdr.ID, 0); break;
			/* @v10.0.0
			case PPVCMD_PRINTZEROCHECK:
				ok = -1;
				if(r_cfg.Cash) {
					P_BObj->PrintCheck(0, 0);
					ok = 1;
				}
				break;
			*/
			case PPVCMD_CREATEMRPTAB:
				if(hdr.ID)
					ok = CreateMrpTab(hdr.ID);
				break;
			/* case PPVCMD_TOTAL:
				ok = ViewTotal();
				break; */
			/* @v9.2.10 case PPVCMD_TPIDTOTAL:
				ok = ViewVATaxList();
				break; */
			case PPVCMD_CHECKSTAT:
				if(r_cfg.Cash) {
					PPCashMachine * cm = PPCashMachine::CreateInstance(r_cfg.Cash);
					if(cm) {
						ok = cm->SyncViewSessionStat(0);
						delete cm;
					}
				}
				break;
			/* @v9.2.10 case PPVCMD_EXPORTBNKORDER:
				if(Filt.Flags & BillFilt::fAccturnOnly)
					ok = ExportBnkOrder();
				break; */
			case PPVCMD_REFRESH:
				ok = update = 1;
				break;
			case PPVCMD_RECEIVEDFOCUS:
				if(!pHdr) {
					const PPID single_loc_id = LocList_.getSingle();
					if(single_loc_id)
						DS.SetLocation(single_loc_id);
					StatusWinChange();
				}
				break;
			case PPVCMD_EDITMEMOS:
				if(hdr.ID) {
					if((ok = EditObjMemos(PPOBJ_BILL, PPPRP_BILLMEMO, hdr.ID)) == 0)
						PPError();
					else
						update = 1;
				}
				break;
			case PPVCMD_BROWSESCOPE:
				{
					InventoryFilt filt;
					BillViewItem bitem;
					for(InitIteration(OrdByID); NextIteration(&bitem) > 0;) {
						filt.BillList.Add(bitem.ID);
					}
					if(filt.BillList.GetCount()) {
						PPViewInventory * p_view = new PPViewInventory();
						if(p_view && p_view->Init_(&filt))
							p_view->Browse(1);
						else
							PPError();
					}
				}
				break;
			case PPVCMD_MOUSEHOVER:
				if(id && pBrw) {
					long   h = 0;
					pBrw->ItemByMousePos(&h, 0);
					if(h >= 0) {
						int    mfn = -1;
						if(!(Filt.Flags & BillFilt::fCashOnly)) {
							BrowserDef * p_def = pBrw->getDef();
							if(p_def && p_def->getCount())
								mfn = p_def->getCount()-1;
						}
						if(h == mfn) {
							SString memos;
							if(id && P_BObj->FetchExtMemo(id, memos) > 0) {
							//if(id && PPRef->GetPropVlrString(PPOBJ_BILL, id, PPPRP_BILLMEMO, memos) > 0 && memos.Len() > 0) {
								const long flags = SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|SMessageWindow::fTextAlignLeft|
									SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow;
								memos.ReplaceChar('\n', ' ');
								memos.ReplaceChar('\r', ' ');
								memos.ReplaceStr(MemosDelim, "\n", 0);
								PPTooltipMessage(memos, 0, pBrw->H(), 10000, 0, flags);
							}
						}
					}
				}
				break;
			default:
				break;
		}
	}
	if(update > 0 && ok > 0) {
		//if(P_TempOrd && oneof2(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject))
		if(IsTempTblNeeded()) {
			ok = UpdateTempTable(id);
			update = 2; // @v9.0.9
		}
		if(update == 2 && pBrw) {
			pBrw->Update();
			if(CheckIDForFilt(id, 0)) { // @v9.0.11
				pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
			}
		}
	}
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}
//
//
//
int FASTCALL ViewGoodsBills(BillFilt * pFilt, int asModeless)
{
	int    ok = -1, r = 0, view_in_use = 0;
	int    modeless = GetModelessStatus(asModeless);
	PPView * p_v = 0;
	PPBaseFilt * p_flt = 0;
    PPViewBrowser * p_prev_win = 0;
	THROW(PPCheckDatabaseChain());
	THROW(PPView::CreateInstance(PPVIEW_BILL, &p_v));
	THROW(p_flt = p_v->CreateFilt(0));
	if(modeless)
		p_prev_win = (PPViewBrowser *)PPFindLastBrowser();
	if(pFilt) {
		THROW(p_flt->Copy(pFilt, 1));
	}
	else if(p_prev_win) {
		THROW(p_flt->Copy(p_prev_win->P_View->GetBaseFilt(), 1));
	}
	while(pFilt || p_v->EditBaseFilt(p_flt) > 0) {
		PPWait(1);
		if(((BillFilt *)p_flt)->Flags & BillFilt::fAsSelector)
			modeless = 0;
		THROW(p_v->Init_(p_flt));
		PPCloseBrowser(p_prev_win);
		THROW(r = p_v->Browse(modeless));
		if(modeless || pFilt) {
			view_in_use = 1;
			break;
		}
	}
	if(!modeless && r > 0) {
		pFilt->Sel = ((BillFilt*)p_v->GetBaseFilt())->Sel;
		ok = 1;
	}
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	delete p_flt;
	return ok;
}

int SLAPI ViewBillsByPool(PPID poolType, PPID poolOwnerID)
{
	int    ok = -1;
	int    pool_bytype = BIN(poolOwnerID && oneof5(poolType, PPASS_OPBILLPOOL, PPASS_TODOBILLPOOL,
		PPASS_PRJBILLPOOL, PPASS_PRJPHASEBILLPOOL, PPASS_TSESSBILLPOOL));
	PPBaseFilt * p_flt = 0;
	PPView * p_v = 0;
	if(poolType && poolOwnerID) {
		THROW(PPView::CreateInstance(PPVIEW_BILL, &p_v));
		THROW(p_flt = p_v->CreateFilt(0));
		{
			BillFilt * p_bfilt = (BillFilt*)p_flt;
			p_bfilt->AssocID = poolType;
			p_bfilt->PoolBillID = poolOwnerID;
			p_bfilt->Bbt = bbtUndef;
			SETFLAG(p_bfilt->Flags, BillFilt::fEditPoolByType, pool_bytype);
			if(poolType == PPASS_TSESSBILLPOOL) {
				PPObjTSession tses_obj;
				TSessionTbl::Rec tses_rec;
				if(tses_obj.Search(poolOwnerID, &tses_rec) > 0) {
					ProcessorTbl::Rec prc_rec;
					if(tses_obj.GetPrc(tses_rec.PrcID, &prc_rec, 1, 1) > 0) {
						if(prc_rec.WrOffOpID && CheckOpFlags(prc_rec.WrOffOpID, OPKF_NEEDPAYMENT)) {
							SETFLAG(p_bfilt->Flags, BillFilt::fShowDebt, 1);
						}
					}
				}
			}
			THROW(p_v->Init_(p_bfilt));
			THROW(p_v->Browse(0));
		}
	}
	CATCHZOKPPERR
	delete p_v;
	delete p_flt;
	return ok;
}

int FASTCALL BrowseBills(BrowseBillsType bbt)
{
	int    ok = -1;
	/* @v9.8.11 if(bbt == bbtDraftBills && !(CConfig.Flags & CCFLG_USEDRAFTBILL))
		ok = (PPMessage(mfInfo, PPINF_NOSETUSEDRAFTBILLFLAG), -1);
	else { */
		THROW(PPCheckDatabaseChain());
		{
			BillFilt::FiltExtraParam p(1, bbt);
			ok = PPView::Execute(PPVIEW_BILL, 0, GetModelessStatus(), &p);
		}
	// @v9.8.1 }
	CATCHZOKPPERR
	return ok;
}
//
// Implementation of PPALDD_GoodsBillBase
//
PPALDD_CONSTRUCTOR(GoodsBillBase)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBillBase)
{
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	CALLPTRMEMB(p_pack, RemoveVirtualTItems());
	Destroy();
}

void PPALDD_GoodsBillBase::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))

	if(pF->Name == "?CalcInSaldo") {
		double saldo = 0.0;
		const PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
		if(p_pack) {
			const PPID goods_id = _ARG_INT(1);
			if(H.ObjectID && goods_id) {
				PPObjBill * p_bobj = BillObj;
				PPTransferItem * p_ti;
				long   oprno = 0;
				double qtty = 0.0;
				for(uint i = 0; p_pack->EnumTItems(&i, &p_ti);) {
					if(labs(p_ti->GoodsID) == goods_id) {
						if(oprno == 0 && p_ti->RByBill) {
							TransferTbl::Rec rec;
							if(p_bobj->trfr->SearchByBill(p_ti->BillID, 0, p_ti->RByBill, &rec) > 0)
								oprno = rec.OprNo;
						}
						qtty += p_ti->SQtty(p_pack->Rec.OpID);
					}
				}
				const PPID dlvr_loc_id = p_pack->P_Freight ? p_pack->P_Freight->DlvrAddrID : 0; // @v9.5.1
				p_bobj->GetGoodsSaldo(goods_id, H.ObjectID, dlvr_loc_id, H.Dt, oprno, &saldo, 0); // @v9.5.1 @fix H.DlvrLocID-->dlvr_loc_id
			}
		}
		_RET_DBL = saldo;
	}
	else if(pF->Name == "?UnlimGoodsOnly") {
		const PPBillPacket * p_pack = (const PPBillPacket *)Extra[0].Ptr;
		_RET_INT = BIN(p_pack && p_pack->ProcessFlags & PPBillPacket::pfAllGoodsUnlim);
	}
}

static void SLAPI setupDiscountText(const PPBillPacket * pPack, int enableSTaxText, char * pBuf, size_t bufSize)
{
	pBuf[0] = 0;
	if(!CheckOpPrnFlags(pPack->Rec.OpID, OPKF_PRT_NDISCNT)) {
		int    isdis = 0;
		SString val;
		SString temp_buf;
		const  int    re = BIN(pPack->Rec.Flags & BILLF_RMVEXCISE);
		const  int    ne = (CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re;
		const  double dis = pPack->Amounts.Get(PPAMT_MANDIS, pPack->Rec.CurID);
		const  double pctdis = pPack->Amounts.Get(PPAMT_PCTDIS, 0L /* @curID */);
		if(dis != 0 || pctdis != 0) {
			PPLoadText(PPTXT_INCLDIS, temp_buf);
			if(pctdis != 0.0)
				temp_buf.Cat(pctdis, MKSFMTD(0, 1, 0)).Strip().CatChar('%');
			else
				temp_buf.Cat(dis, SFMT_MONEY);
			isdis = 1;
		}
		if(!ne && enableSTaxText) {
			PPGetSubStr(PPTXT_INCLEXCISE, isdis, val);
			if(temp_buf.NotEmpty())
				temp_buf.Space();
		}
		strnzcpy(pBuf, temp_buf, bufSize);
	}
}

int PPALDD_GoodsBillBase::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;

	PPOprKind op_rec;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	const  long bill_f = p_pack->Rec.Flags;
	const  PPID optype = p_pack->OpTypeID;
	PPID   main_org_id = 0;
	BillTotalData  total_data;
	PPObjPerson    psn_obj;
	int    exclude_vat = 0;
	GetOpData(p_pack->Rec.OpID, &op_rec);
	const  PPID object_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? p_pack->Rec.Object2 : p_pack->Rec.Object;
	const  PPID person_id = ObjectToPerson(object_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(person_id && psn_obj.GetRelPersonList(person_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
			parent_person_id = rel_list.at(0);
	}
	BillObj->LoadClbList(p_pack, 1);
	MEMSZERO(H);
	if(op_rec.PrnFlags & OPKF_PRT_NBILLN)
		H.Code[0] = 0;
	else if(bill_f & BILLF_PRINTINVOICE && p_pack->Ext.InvoiceCode[0])
		STRNSCPY(H.Code, p_pack->Ext.InvoiceCode);
	else
		STRNSCPY(H.Code, p_pack->Rec.Code);
	strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	H.Dt = (bill_f & BILLF_PRINTINVOICE && p_pack->Ext.InvoiceDate) ? p_pack->Ext.InvoiceDate : p_pack->Rec.Dt;
	H.OprKindID  = p_pack->Rec.OpID;
	H.ObjectID   = object_id;
	H.PayerID    = p_pack->Ext.PayerID;
	H.AgentID    = p_pack->Ext.AgentID;
	H.LocID      = p_pack->Rec.LocID;
	H.BillID     = p_pack->Rec.ID;
	H.LinkBillID = p_pack->Rec.LinkBillID;
	H.CurID      = p_pack->Rec.CurID;
	H.fShortMainOrg = BIN(op_rec.PrnFlags & OPKF_PRT_SHRTORG);
	p_pack->GetMainOrgID_(&main_org_id);
	H.Flags      = bill_f;
	H.ExpendFlag = 0;
	H.fMergeSameGoods = BIN(op_rec.PrnFlags & OPKF_PRT_MERGETI);
	H.RowOrder = (p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel) ? TiIter::ordByPLU : op_rec.PrnOrder;
	if(op_rec.PrnFlags & OPKF_PRT_NEGINVC)
		H.Flags |= BILLF_NEGINVOICE;
	if(oneof2(optype, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND) || (bill_f & (BILLF_GEXPEND|BILLF_GMODIF)) ||
		(oneof3(optype, PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_CHARGE) && !(op_rec.PrnFlags & OPKF_PRT_INCINVC))) {
		H.ExpendFlag = 1;
		H.DlvrID     = main_org_id;
		H.DlvrReq    = main_org_id;
		H.DlvrLocID  = p_pack->Rec.LocID;
		H.ConsignorReq = main_org_id;
		if(object_id)
			if(PPObjLocation::ObjToWarehouse(object_id)) {
				H.RcvrID    = main_org_id;
				H.RcvrReq   = main_org_id;
				H.RcvrLocID = PPObjLocation::ObjToWarehouse(object_id);
				H.ConsigneeReq = main_org_id;
			}
			else {
				H.RcvrLocID = (bill_f & BILLF_FREIGHT && p_pack->P_Freight) ? p_pack->P_Freight->DlvrAddrID : 0;
				if(parent_person_id) {
					H.RcvrID = parent_person_id;
					H.ConsigneeReq = person_id;
				}
				else {
					H.RcvrID = person_id;
					H.ConsigneeReq = H.RcvrID;
				}
				H.RcvrReq = H.RcvrID;
			}
	}
	else if(optype == PPOPT_DRAFTRECEIPT || bill_f & BILLF_GRECEIPT ||
		(oneof3(optype, PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_CHARGE) && op_rec.PrnFlags & OPKF_PRT_INCINVC)) {
		H.ExpendFlag = 2;
		H.RcvrID    = main_org_id;
		H.RcvrReq   = main_org_id;
		H.RcvrLocID = p_pack->Rec.LocID;
		H.ConsigneeReq = main_org_id;
		if(object_id) {
			if(IsIntrOp(p_pack->Rec.OpID) == INTRRCPT) {
				H.DlvrID = main_org_id;
				H.ConsignorReq = H.DlvrID;
			}
			else if(parent_person_id) {
				H.DlvrID = parent_person_id;
				H.ConsignorReq = person_id;
			}
			else {
				H.DlvrID = person_id;
				H.ConsignorReq = H.DlvrID;
			}
			H.DlvrReq = H.DlvrID;
		}
		if(H.DlvrID == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(object_id);
	}
	p_pack->GetLastPayDate(&H.PayDt);
	{
		PersonTbl::Rec psn_rec;
		const PPID suppl_person_id = (optype == PPOPT_GOODSRETURN) ? H.RcvrID : H.DlvrID;
		if(psn_obj.Fetch(suppl_person_id, &psn_rec) > 0 && psn_rec.Flags & PSNF_NOVATAX) {
			H.fSupplIsVatExempt = 1;
			exclude_vat = 1;
		}
		else
			H.fSupplIsVatExempt = 0;
	}
	//
	// Формирование виртуальных строк для бухгалтерских документов, оплат, начислений и некоторых иных случаев.
	// Функция SetupVirtualTItems самостоятельно проверяет необходимые условия.
	//
	p_pack->SetupVirtualTItems();
	{
		long   btc_flags = BTC_CALCOUTAMOUNTS;
		SETFLAG(btc_flags, BTC_EXCLUDEVAT, exclude_vat);
		SETFLAG(btc_flags, BTC_ONLYUNLIMGOODS, p_pack->ProcessFlags & PPBillPacket::pfPrintOnlyUnlimGoods);
		p_pack->CalcTotal(&total_data, btc_flags);
	}
	H.TotalGoodsLines = total_data.LinesCount;
	H.TotalGoodsNames = total_data.GoodsCount;
	if(total_data.VatList.getCount() >= 1) {
		H.VATRate1 = total_data.VatList.at(0).Rate;
		H.VATSum1  = total_data.VatList.at(0).VatSum;
	}
	if(total_data.VatList.getCount() >= 2) {
		H.VATRate2 = total_data.VatList.at(1).Rate;
		H.VATSum2  = total_data.VatList.at(1).VatSum;
	}
	H.TotalSum    = total_data.Amt;
	H.TotalQtty   = fabs(total_data.UnitsCount);
	H.TotalPhQtty = fabs(total_data.PhUnitsCount);
	H.TotalPacks  = total_data.PackCount;
	H.TotalBrutto = total_data.Brutto;
	H.TotalVAT    = total_data.VAT;
	H.TotalSalesTax = total_data.STax;
	setupDiscountText(p_pack, BIN(total_data.STax > 0), H.TxtManualDscnt, sizeof(H.TxtManualDscnt));
	return (DlRtm::InitData(rFilt, rsrv) > 0) ? 1 : -1;
}

int PPALDD_GoodsBillBase::InitIteration(PPIterID iterId, int sortId, long)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
	long   f = H.fMergeSameGoods ? (ETIEF_UNITEBYGOODS|ETIEF_DIFFBYPACK|ETIEF_DIFFBYQCERT|ETIEF_DIFFBYNETPRICE) : 0;
	PPID   filt_grp_id = 0;
	if(p_pack->ProcessFlags & PPBillPacket::pfPrintTareSaldo) {
		PPObjGoods goods_obj;
		if(goods_obj.GetConfig().TareGrpID) {
			filt_grp_id = goods_obj.GetConfig().TareGrpID;
			f |= ETIEF_SALDOFILTGRP;
		}
	}
	p_pack->InitExtTIter(f, filt_grp_id, (TiIter::Order)H.RowOrder);
	return 1;
}

int PPALDD_GoodsBillBase::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPBillPacket   * p_pack = (PPBillPacket *)(Extra[0].Ptr);
	//
	// Возможны два алгоритма расчета налогов по объединенным строкам документа:
	// 1. Объединенная строка обсчитывается сама по себе, как единая
	// 2. Каждая из строк, включенных в объединенную, обсчитываются порознь, а результат складывается.
	//
	int   merge_line_tax_alg = 1;
	const PPCommConfig & r_ccfg = CConfig;
	if(checkdate(r_ccfg._InvcMergeTaxCalcAlg2Since) && p_pack->Rec.Dt >= r_ccfg._InvcMergeTaxCalcAlg2Since) {
		merge_line_tax_alg = 2;
	}
	//
	SString temp_buf;
	PPObjBill * p_bobj = BillObj;
	ReceiptCore * p_rcpt = (p_bobj && p_bobj->trfr) ? &p_bobj->trfr->Rcpt : 0;
	PPTransferItem * p_ti, temp_ti;
	PPBillPacket::TiItemExt tiie;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	PPObjQuotKind qk_obj;
	PPQuotKind qk_rec;
	double ext_price = 0.0;
	double upp = 0.0; // Емкость упаковки
	int    tiamt;
	int    price_chng = 1;
	uint   n = (uint)I.nn;
	const  PPID qk_id = p_pack->Ext.ExtPriceQuotKindID;
	const  long exclude_tax_flags = H.fSupplIsVatExempt ? GTAXVF_VAT : 0L;
	const  int  extprice_by_base = BIN(qk_obj.Fetch(qk_id, &qk_rec) > 0 && qk_rec.Flags & QUOTKF_EXTPRICEBYBASE);
	int    treat_as_unlim = 0;
	do {
		price_chng = 1; // Цена изменилась по отношению к предыдущему лоту. Если не установлен флаг pfPrintChangedPriceOnly, то игнорируется.
		treat_as_unlim = 0;
		if(p_pack->EnumTItemsExt(0, &temp_ti, &tiie) > 0) {
			n++;
			p_ti = &temp_ti;
			if(p_pack->ProcessFlags & PPBillPacket::pfPrintOnlyUnlimGoods && goods_obj.Fetch(p_ti->GoodsID, &goods_rec) > 0 && goods_rec.GoodsTypeID) {
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				if(gt_obj.Fetch(goods_rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.Flags & (GTF_UNLIMITED|GTF_QUASIUNLIM))
					treat_as_unlim = 1;
			}
			if(qk_id) {
				const double base = extprice_by_base ? p_ti->Price : p_ti->NetPrice();
				const QuotIdent qi(p_ti->LocID, qk_id, p_ti->CurID, p_pack->Rec.Object);
				goods_obj.GetQuotExt(p_ti->GoodsID, qi, p_ti->Cost, base, &ext_price, 1);
			}
			if(p_pack->ProcessFlags & PPBillPacket::pfPrintChangedPriceOnly) {
				//
				// Будем печатать только те товары, цены на которые изменились.
				//
				if(p_rcpt) {
					ReceiptTbl::Rec prev_rec, rec;
					if(p_rcpt->Search(p_ti->LotID, &rec) > 0) {
						const int r = p_rcpt->GetPreviousLot(rec.GoodsID, rec.LocID, rec.Dt, rec.OprNo, &prev_rec);
						price_chng = BIN(r <= 0 || rec.Price != prev_rec.Price);
						if(!price_chng) {
							double prev_rest = 0.0;
							p_bobj->trfr->GetRest(prev_rec.ID, rec.Dt, rec.OprNo, &prev_rest, 0);
							if(prev_rest <= 0.0)
								price_chng = 1;
						}
					}
				}
			}
		}
		else
			return -1;
	} while((p_pack->ProcessFlags & PPBillPacket::pfPrintOnlyUnlimGoods && !treat_as_unlim) || !price_chng);
	I.nn      = n;
	I.LotID   = p_ti->LotID;
	I.GoodsID = p_ti->GoodsID;
	I.GoodsGrpName[0] = 0;
	if(H.RowOrder == TiIter::ordByGrpGoods && goods_obj.Fetch(p_ti->GoodsID, &goods_rec) > 0) {
		goods_obj.P_Tbl->MakeFullName(goods_rec.ParentID, 0, temp_buf);
		temp_buf.CopyTo(I.GoodsGrpName, sizeof(I.GoodsGrpName));
	}
	else if(H.RowOrder == TiIter::ordByLocation) {
		PPID   loc_id = 0;
		PPObjLocation loc_obj;
		GoodsToObjAssoc gto_assc(PPASS_GOODS2LOC, PPOBJ_LOCATION);
		gto_assc.Load();
		if(gto_assc.Get(p_ti->GoodsID, &loc_id) > 0 && GetLocationName(loc_id, temp_buf) > 0)
			temp_buf.CopyTo(I.GoodsGrpName, sizeof(I.GoodsGrpName));
	}
	else if(H.RowOrder == TiIter::ordByStorePlaceGrpGoods && goods_obj.Fetch(p_ti->GoodsID, &goods_rec) > 0) {
		PPIDArray loc_list;
		PPObjLocation loc_obj;
		GoodsToObjAssoc gto_assc(PPASS_GOODS2WAREPLACE, PPOBJ_LOCATION, 1);
		gto_assc.Load();
		temp_buf.Z();
		if(gto_assc.GetListByGoods(goods_rec.ID, loc_list) > 0)
			for(uint j = 0; temp_buf.Empty() && j < loc_list.getCount(); j++) {
				const PPID loc_id = loc_list.get(j);
				if(loc_obj.BelongTo(loc_id, p_pack->Rec.LocID, &temp_buf)) {
					break;
				}
			}
		temp_buf.CopyTo(I.GoodsGrpName, sizeof(I.GoodsGrpName));
	}
	I.LotID   = p_ti->LotID;
	I.Cost    = p_ti->Cost;
	I.Price   = (p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel) ? p_ti->NetPrice() : p_ti->Price;
	//
	I.Discount = p_ti->Discount;
	I.CurPrice = p_ti->CurPrice;
	I.CurSum   = p_ti->CurPrice * fabs(p_ti->Qtty());
	if(p_pack->OutAmtType == TIAMT_COST) {
		tiamt = TIAMT_COST;
		I.MainPrice = p_ti->Cost;
	}
	else if(p_pack->OutAmtType == TIAMT_PRICE) {
		tiamt = TIAMT_PRICE;
		I.MainPrice = p_ti->NetPrice();
	}
	else {
		tiamt = TIAMT_AMOUNT;
		I.MainPrice = p_ti->NetPrice();
	}
	I.ExtPrice     = ext_price;
    if(oneof2(H.OprKindID, PPOPK_EDI_STOCK, PPOPK_EDI_SHOPCHARGEON)) // Для документа остатков количество должно быть со знаком "как он есть"
		I.Qtty = p_ti->Quantity_;
	else
		I.Qtty = p_ti->Qtty();
	upp = p_ti->UnitPerPack;
	if(upp <= 0.0) {
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
			upp = gse.Package;
	}
	if(upp <= 0.0 && p_pack->IsDraft()) {
		if(p_rcpt) {
			ReceiptTbl::Rec lot_rec;
			if(p_rcpt->GetLastLot(p_ti->GoodsID, p_pack->Rec.LocID, p_pack->Rec.Dt, &lot_rec) > 0)
				upp = lot_rec.UnitPerPack;
		}
	}
	I.UnitsPerPack = upp;
	I.FullPack = 0;
	if(upp > 0.0)
		I.FullPack = (long)(fabs(I.Qtty) / upp);
	{
		I.VATRate = 0.0;
		I.VATSum = 0.0;
		I.ExcRate = 0.0;
		I.ExcSum = 0.0;
		I.STRate = 0.0;
		I.STSum = 0.0;
		if(merge_line_tax_alg == 2 && tiie.MergePosList.getCount()) { // @v9.2.3 && tiie.MergePosList.getCount()
			double prev_vat_rate = 0.0;
			assert(tiie.MergePosList.getCount());
			for(uint i = 0; i < tiie.MergePosList.getCount(); i++) {
				uint pos_local = tiie.MergePosList.get(i);
				assert(pos_local < p_pack->GetTCount());
				const PPTransferItem & r_ti_local = p_pack->TI(pos_local);
				const double qtty_local = fabs(r_ti_local.Qtty());
				GTaxVect vect;
				vect.CalcTI(&r_ti_local, p_pack->Rec.OpID, tiamt, exclude_tax_flags);
				I.VATRate = vect.GetTaxRate(GTAX_VAT, 0);
				assert(i == 0 || I.VATRate == prev_vat_rate);
				prev_vat_rate = I.VATRate;
				I.VATSum  += vect.GetValue(GTAXVF_VAT);
				I.ExcRate = vect.GetTaxRate(GTAX_EXCISE, 0);
				I.ExcSum  += vect.GetValue(GTAXVF_EXCISE);
				I.STRate  = vect.GetTaxRate(GTAX_SALES, 0);
				if(tiamt == TIAMT_COST) {
					if(r_ti_local.Flags & PPTFR_COSTWOVAT)
						I.MainPrice += I.VATSum / qtty_local;
				}
				else if(r_ti_local.Flags & PPTFR_PRICEWOTAXES) {
					const double _a = vect.GetValue(GTAXVF_BEFORETAXES) / qtty_local;
					if(i == 0)
						I.MainPrice = _a;
					else
						I.MainPrice += _a;
				}
				if(!(p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel)) {
					I.STSum += vect.GetValue(GTAXVF_SALESTAX);
				}
			}
		}
		else {
			const PPTransferItem & r_ti_local = *p_ti;
			const double qtty_local = fabs(r_ti_local.Qtty());
			GTaxVect vect;
			vect.CalcTI(&r_ti_local, p_pack->Rec.OpID, tiamt, exclude_tax_flags);
			I.VATRate = vect.GetTaxRate(GTAX_VAT, 0);
			I.VATSum  = vect.GetValue(GTAXVF_VAT);
			I.ExcRate = vect.GetTaxRate(GTAX_EXCISE, 0);
			I.ExcSum  = vect.GetValue(GTAXVF_EXCISE);
			I.STRate  = vect.GetTaxRate(GTAX_SALES, 0);
			if(tiamt == TIAMT_COST) {
				if(r_ti_local.Flags & PPTFR_COSTWOVAT)
					I.MainPrice += I.VATSum / qtty_local;
			}
			else if(r_ti_local.Flags & PPTFR_PRICEWOTAXES) {
				const double _a = vect.GetValue(GTAXVF_BEFORETAXES) / qtty_local;
				I.MainPrice = _a;
			}
			if(!(p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel)) {
				I.STSum = vect.GetValue(GTAXVF_SALESTAX);
			}
		}
		if(p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel) {
			long   plu = 0;
			if(p_bobj->Cfg.Flags & BCF_RETAILEDPRICEINLABEL) {
				RetailExtrItem rpi;
				RetailPriceExtractor rpe(p_ti->LocID, 0, 0, ZERODATETIME, RTLPF_PRICEBYQUOT);
				rpe.GetPrice(p_ti->GoodsID, 0, 0.0, &rpi);
				I.Price = rpi.Price;
			}
			SString obj_assc_name;
			p_pack->GetNextPLU(&plu, obj_assc_name);
			STRNSCPY(I.CQtty, obj_assc_name);
			I.STSum = plu;
		}
		else {
			QttyToStr(p_ti->Quantity_, upp, ((LConfig.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_COMPLPACK|QTTYF_FRACTION) : QTTYF_FRACTION), I.CQtty);
		}
	}
	if(p_pack->P_PckgList) {
		while(tiie.Pckg.Len() > 1 && isalpha(tiie.Pckg.Last()))
			tiie.Pckg.TrimRight();
		tiie.Pckg.CopyTo(I.CLB, sizeof(I.CLB));
	}
	else
		tiie.Clb.CopyTo(I.CLB, sizeof(I.CLB));
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_GoodsBillDispose
//
struct DlGoodsBillDisposeBlock {
	DlGoodsBillDisposeBlock(void * ptr) : P_Pack((PPBillPacket *)ptr)
	{
	}
	PPBillPacket * P_Pack;
};

PPALDD_CONSTRUCTOR(GoodsBillDispose)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(GoodsBillDispose)
{
	DlGoodsBillDisposeBlock * p_blk = (DlGoodsBillDisposeBlock *)Extra[0].Ptr;
	delete p_blk;
	Destroy();
}

int PPALDD_GoodsBillDispose::InitData(PPFilt & rFilt, long rsrv)
{
	DlGoodsBillDisposeBlock * p_blk = new DlGoodsBillDisposeBlock(rFilt.Ptr);
	Extra[0].Ptr = p_blk;
	PPOprKind op_rec;
	PPBillPacket * p_pack = (PPBillPacket *)p_blk->P_Pack;
	const  long bill_f = p_pack->Rec.Flags;
	const  PPID optype = p_pack->OpTypeID;
	PPID   main_org_id = 0;
	BillTotalData  total_data;
	PPObjPerson    psn_obj;
	int    exclude_vat = 0;
	GetOpData(p_pack->Rec.OpID, &op_rec);
	const  PPID object_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? p_pack->Rec.Object2 : p_pack->Rec.Object;
	const  PPID person_id = ObjectToPerson(object_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(person_id && psn_obj.GetRelPersonList(person_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
			parent_person_id = rel_list.at(0);
	}
	BillObj->LoadClbList(p_pack, 1);
	MEMSZERO(H);
	if(op_rec.PrnFlags & OPKF_PRT_NBILLN)
		H.Code[0] = 0;
	else if(bill_f & BILLF_PRINTINVOICE && p_pack->Ext.InvoiceCode[0])
		STRNSCPY(H.Code, p_pack->Ext.InvoiceCode);
	else
		STRNSCPY(H.Code, p_pack->Rec.Code);
	strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	H.Dt = (bill_f & BILLF_PRINTINVOICE && p_pack->Ext.InvoiceDate) ? p_pack->Ext.InvoiceDate : p_pack->Rec.Dt;
	H.OprKindID  = p_pack->Rec.OpID;
	H.ObjectID   = object_id;
	H.PayerID    = p_pack->Ext.PayerID;
	H.AgentID    = p_pack->Ext.AgentID;
	H.LocID      = p_pack->Rec.LocID;
	H.BillID     = p_pack->Rec.ID;
	H.LinkBillID = p_pack->Rec.LinkBillID;
	H.CurID      = p_pack->Rec.CurID;
	H.fShortMainOrg = BIN(op_rec.PrnFlags & OPKF_PRT_SHRTORG);
	p_pack->GetMainOrgID_(&main_org_id);
	H.Flags      = bill_f;
	H.ExpendFlag = 0;
	H.fMergeSameGoods = BIN(op_rec.PrnFlags & OPKF_PRT_MERGETI);
	H.RowOrder = (p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel) ? TiIter::ordByPLU : op_rec.PrnOrder;
	if(op_rec.PrnFlags & OPKF_PRT_NEGINVC)
		H.Flags |= BILLF_NEGINVOICE;
	if(oneof2(optype, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND) || (bill_f & (BILLF_GEXPEND|BILLF_GMODIF)) ||
		(oneof3(optype, PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_CHARGE) && !(op_rec.PrnFlags & OPKF_PRT_INCINVC))) {
		H.ExpendFlag = 1;
		H.DlvrID     = main_org_id;
		H.DlvrReq    = main_org_id;
		H.DlvrLocID  = p_pack->Rec.LocID;
		H.ConsignorReq = main_org_id;
		if(object_id)
			if(PPObjLocation::ObjToWarehouse(object_id)) {
				H.RcvrID    = main_org_id;
				H.RcvrReq   = main_org_id;
				H.RcvrLocID = PPObjLocation::ObjToWarehouse(object_id);
				H.ConsigneeReq = main_org_id;
			}
			else {
				H.RcvrLocID = (bill_f & BILLF_FREIGHT && p_pack->P_Freight) ? p_pack->P_Freight->DlvrAddrID : 0;
				if(parent_person_id) {
					H.RcvrID = parent_person_id;
					H.ConsigneeReq = person_id;
				}
				else {
					H.RcvrID = person_id;
					H.ConsigneeReq = H.RcvrID;
				}
				H.RcvrReq = H.RcvrID;
			}
	}
	else if(optype == PPOPT_DRAFTRECEIPT || bill_f & BILLF_GRECEIPT ||
		(oneof3(optype, PPOPT_ACCTURN, PPOPT_PAYMENT, PPOPT_CHARGE) && op_rec.PrnFlags & OPKF_PRT_INCINVC)) {
		H.ExpendFlag = 2;
		H.RcvrID    = main_org_id;
		H.RcvrReq   = main_org_id;
		H.RcvrLocID = p_pack->Rec.LocID;
		H.ConsigneeReq = main_org_id;
		if(object_id) {
			if(IsIntrOp(p_pack->Rec.OpID) == INTRRCPT) {
				H.DlvrID = main_org_id;
				H.ConsignorReq = H.DlvrID;
			}
			else if(parent_person_id) {
				H.DlvrID = parent_person_id;
				H.ConsignorReq = person_id;
			}
			else {
				H.DlvrID = person_id;
				H.ConsignorReq = H.DlvrID;
			}
			H.DlvrReq = H.DlvrID;
		}
		if(H.DlvrID == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(object_id);
	}
	p_pack->GetLastPayDate(&H.PayDt);
	{
		PersonTbl::Rec psn_rec;
		PPID   suppl_person_id = (optype == PPOPT_GOODSRETURN) ? H.RcvrID : H.DlvrID;
		if(psn_obj.Fetch(suppl_person_id, &psn_rec) > 0 && psn_rec.Flags & PSNF_NOVATAX) {
			H.fSupplIsVatExempt = 1;
			exclude_vat = 1;
		}
		else
			H.fSupplIsVatExempt = 0;
	}
	long   btc_flags = BTC_CALCOUTAMOUNTS;
	SETFLAG(btc_flags, BTC_EXCLUDEVAT, exclude_vat);
	SETFLAG(btc_flags, BTC_ONLYUNLIMGOODS, p_pack->ProcessFlags & PPBillPacket::pfPrintOnlyUnlimGoods);
	p_pack->CalcTotal(&total_data, btc_flags);
	H.TotalGoodsLines = total_data.LinesCount;
	H.TotalGoodsNames = total_data.GoodsCount;
	H.TotalSum    = total_data.Amt;
	H.TotalQtty   = fabs(total_data.UnitsCount);
	H.TotalPhQtty = fabs(total_data.PhUnitsCount);
	H.TotalPacks  = total_data.PackCount;
	H.TotalBrutto = total_data.Brutto;
	H.TotalVAT    = total_data.VAT;
	H.TotalSalesTax = total_data.STax;
	return (DlRtm::InitData(rFilt, rsrv) > 0) ? 1 : -1;
}

int PPALDD_GoodsBillDispose::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	DlGoodsBillDisposeBlock * p_blk = (DlGoodsBillDisposeBlock *)Extra[0].Ptr;
	PPBillPacket * p_pack = p_blk->P_Pack;
	p_pack->InitExtTIter(ETIEF_DISPOSE, 0, (TiIter::Order)H.RowOrder);
	return 1;
}

int PPALDD_GoodsBillDispose::NextIteration(long iterId)
{
	IterProlog(iterId, 0);
	//
	DlGoodsBillDisposeBlock * p_blk = (DlGoodsBillDisposeBlock *)Extra[0].Ptr;
	PPBillPacket * p_pack = p_blk->P_Pack;
	//
	const PPConfig & r_cfg = LConfig;
	PPTransferItem * p_ti, temp_ti;
	PPBillPacket::TiItemExt tiie;
	GTaxVect vect;
	SString temp_buf;
	double ext_price = 0.0;
	double upp = 0.0; // Емкость упаковки
	long   exclude_tax_flags = H.fSupplIsVatExempt ? GTAXVF_VAT : 0L;
	int    tiamt, price_chng = 1;
	uint   n = (uint)I.nn;
	PPObjGoods goods_obj;
	PPObjQuotKind qk_obj;
	PPQuotKind qk_rec;
	const  PPID qk_id = p_pack->Ext.ExtPriceQuotKindID;
	const  int  extprice_by_base = BIN(qk_obj.Fetch(qk_id, &qk_rec) > 0 && qk_rec.Flags & QUOTKF_EXTPRICEBYBASE);
	do {
		if(p_pack->EnumTItemsExt(0, &temp_ti, &tiie) > 0) {
			n++;
			p_ti = &temp_ti;
			if(qk_id) {
				const double base = extprice_by_base ? p_ti->Price : p_ti->NetPrice();
				QuotIdent qi(p_ti->LocID, qk_id, p_ti->CurID, p_pack->Rec.Object);
				goods_obj.GetQuotExt(p_ti->GoodsID, qi, p_ti->Cost, base, &ext_price, 1);
			}
		}
		else
			return -1;
	} while(0);
	I.nn      = n;
	I.LotID   = p_ti->LotID;
	I.GoodsID = p_ti->GoodsID;
	I.LotID   = p_ti->LotID;
	I.Cost    = p_ti->Cost;
	I.Price   = (p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel) ? p_ti->NetPrice() : p_ti->Price;
	//
	I.Discount = p_ti->Discount;
	I.CurPrice = p_ti->CurPrice;
	I.CurSum   = p_ti->CurPrice * fabs(p_ti->Qtty());
	if(p_pack->OutAmtType == TIAMT_COST) {
		tiamt = TIAMT_COST;
		I.MainPrice = p_ti->Cost;
	}
	else if(p_pack->OutAmtType == TIAMT_PRICE) {
		tiamt = TIAMT_PRICE;
		I.MainPrice = p_ti->NetPrice();
	}
	else {
		tiamt = TIAMT_AMOUNT;
		I.MainPrice = p_ti->NetPrice();
	}
	I.ExtPrice     = ext_price;
	I.Qtty         = p_ti->Qtty();
	upp = p_ti->UnitPerPack;
	if(upp <= 0.0) {
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
			upp = gse.Package;
	}
	if(upp <= 0.0 && p_pack->IsDraft()) {
		PPObjBill * p_bobj = BillObj;
		ReceiptCore * p_rcpt = (p_bobj && p_bobj->trfr) ? &p_bobj->trfr->Rcpt : 0;
		if(p_rcpt) {
			ReceiptTbl::Rec lot_rec;
			if(p_rcpt->GetLastLot(p_ti->GoodsID, p_pack->Rec.LocID, p_pack->Rec.Dt, &lot_rec) > 0)
				upp = lot_rec.UnitPerPack;
		}
	}
	I.UnitsPerPack = upp;
	I.FullPack = 0;
	if(upp > 0.0)
		I.FullPack = (long)(fabs(I.Qtty) / upp);
	//
	I.CellID     = tiie.LctRec.LocID;
	I.DispRByLoc = tiie.LctRec.RByLoc;
	I.DispRByBill = tiie.LctRec.RByBill;
	I.DispDt      = tiie.LctRec.Dt;
	I.DispTm      = tiie.LctRec.Tm;
	I.DispUserID  = tiie.LctRec.UserID;
	I.DispOp      = tiie.LctRec.Op;
	I.DispFlags   = tiie.LctRec.Flags;
	I.PalletTypeID = tiie.LctRec.PalletTypeID;
	I.PalletCount  = tiie.LctRec.PalletCount;
	I.DispQtty     = tiie.LctRec.Qtty;
	I.DispRestByGoods = tiie.LctRec.RestByGoods;
	I.DispRestByLot   = tiie.LctRec.RestByLot;
	QttyToStr(fabs(p_ti->Qtty()),     upp, ((r_cfg.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_COMPLPACK | QTTYF_FRACTION) : QTTYF_FRACTION), I.CQtty);
	QttyToStr(fabs(tiie.LctRec.Qtty), upp, ((r_cfg.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_COMPLPACK | QTTYF_FRACTION) : QTTYF_FRACTION), I.CDispQtty);
	//
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_Bill
//
struct DL600_BillExt {
	DL600_BillExt(BillCore * pT) : P_Bill(pT), CrEventSurID(0)
	{
		MEMSZERO(Rec);
	}
	BillCore * P_Bill;
	long   CrEventSurID;
	BillTbl::Rec Rec;
};

PPALDD_CONSTRUCTOR(Bill)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		DL600_BillExt * p_ext = new DL600_BillExt(BillObj->P_Tbl);
		Extra[0].Ptr = p_ext;
		//Extra[0].Ptr = BillObj->tbl;
	}
}

PPALDD_DESTRUCTOR(Bill)
{
	delete (DL600_BillExt *)(Extra[0].Ptr);
	Extra[0].Ptr = 0;
	Destroy();
}

int PPALDD_Bill::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		DL600_BillExt * p_ext = (DL600_BillExt *)(Extra[0].Ptr);
		BillCore * p_billcore = p_ext->P_Bill;
		p_ext->CrEventSurID = 0;
		BillTbl::Rec rec;
		if(p_billcore->Search(rFilt.ID, &rec) > 0) {
			const  int accs_cost = BillObj->CheckRights(BILLRT_ACCSCOST);
			PPID   freight_bill_id = 0;
			AmtList amt_list;
			PPFreight freight;
			PPRentCondition rent;
			AmtEntry * p_ae = 0;
			PPObjAmountType amtt_obj;
			PPAmountType amtt_rec;
			TaxAmountIDs tai;

			p_ext->Rec = rec;
			H.ID         = rec.ID;
			H.ArticleID  = rec.Object;
			H.Object2ID  = rec.Object2;
			H.LocID      = rec.LocID;
			H.OprKindID  = rec.OpID;
			H.LinkBillID = rec.LinkBillID;
			H.UserID     = rec.UserID;
			STRNSCPY(H.Code, rec.Code);
			p_billcore->GetCode(H.Code);
			STRNSCPY(H.Memo, rec.Memo);
			H.Dt       = rec.Dt;
			H.CurID    = rec.CurID;
			H.StatusID = rec.StatusID;
			H.Flags    = rec.Flags;
			H.fNeedPayment = BIN(rec.Flags & BILLF_NEEDPAYMENT);
			H.fPayout      = BIN(rec.Flags & BILLF_PAYOUT);
			H.fWL          = BIN(rec.Flags & BILLF_WHITELABEL);
			H.DueDate      = rec.DueDate;
			H.Amount   = BR2(rec.Amount);
			amtt_obj.GetTaxAmountIDs(&tai, 1);
			H.Vat1Rate = fdiv100i(tai.VatRate[0]);
			H.Vat2Rate = fdiv100i(tai.VatRate[1]);
			p_billcore->GetAmountList(rec.ID, &amt_list);
			for(uint i = 0; amt_list.enumItems(&i, (void **)&p_ae);) {
				if(p_ae->CurID == 0) {
					switch(p_ae->AmtTypeID) {
						case PPAMT_BUYING:
							if(accs_cost) {
								H.CostSum = p_ae->Amt;
								H.GenCostSum += p_ae->Amt;
							}
							break;
						case PPAMT_SELLING:
							H.PriceSum = p_ae->Amt;
							H.GenPriceSum += p_ae->Amt;
							break;
						case PPAMT_DISCOUNT:
							H.Discount = p_ae->Amt;
							H.GenDiscount += p_ae->Amt;
							break;
						case PPAMT_PCTDIS:
							H.ManPctDscnt = p_ae->Amt;
							break;
						case PPAMT_MANDIS:
							H.ManAbsDscnt = p_ae->Amt;
							break;
						case PPAMT_EXCISE:
							H.Excise = p_ae->Amt;
							break;
						case PPAMT_CVAT:
							if(accs_cost) {
								//
								// Если доступа к ценам поступления нет, то
								// показывать НДС в ценах постуления - значит позволить
								// рассчитать сумму в ценах поступления (ставка обычно известна).
								//
								H.CVat = p_ae->Amt;
							}
							break;
						case PPAMT_PVAT:
							H.PVat = p_ae->Amt;
							break;
						default:
							if(amtt_obj.Fetch(p_ae->AmtTypeID, &amtt_rec) > 0) {
								if(amtt_rec.Flags & PPAmountType::fReplaceCost) {
									if(accs_cost)
										H.GenCostSum += p_ae->Amt;
								}
								else if(amtt_rec.Flags & PPAmountType::fReplacePrice)
									H.GenPriceSum += p_ae->Amt;
								else if(amtt_rec.Flags & PPAmountType::fReplaceDiscount)
									H.GenDiscount += p_ae->Amt;
								if(p_ae->AmtTypeID == tai.VatAmtID[0])
									H.Vat1Sum = p_ae->Amt;
								else if(p_ae->AmtTypeID == tai.VatAmtID[1])
									H.Vat2Sum = p_ae->Amt;
							}
							break;
					}
				}
				if(p_ae->AmtTypeID == PPAMT_PAYMENT && p_ae->CurID == rec.CurID)
					H.Payment = p_ae->Amt;
			}
			if(rec.Flags & BILLF_NEEDPAYMENT || CheckOpFlags(rec.OpID, OPKF_RECKON, 0))
				H.Debt = H.Amount - H.Payment;
			p_billcore->GetLastPayDate(rec.ID, &H.LastPayDate);
			if(rec.Flags & BILLF_EXTRA) {
				PPBillExt ext;
				p_billcore->GetExtraData(rec.ID, &ext);
				H.PayerID = ext.PayerID;
				H.AgentID = ext.AgentID;
				H.ExtPriceQkID = ext.ExtPriceQuotKindID;
				STRNSCPY(H.PaymBillCode, ext.PaymBillCode);
				H.PaymBillDate = ext.PaymBillDate;
				STRNSCPY(H.InvoiceCode, ext.InvoiceCode);
				H.InvoiceDate = ext.InvoiceDate;
			}
			if(rec.Flags & BILLF_FREIGHT)
				freight_bill_id = rec.ID;
			else if(p_billcore->IsMemberOfPool(rec.ID, PPASS_OPBILLPOOL, &freight_bill_id) <= 0)
				freight_bill_id = 0;
			if(freight_bill_id && PPRef->GetProperty(PPOBJ_BILL, freight_bill_id, BILLPRP_FREIGHT, &freight, sizeof(freight)) > 0) {
				STRNSCPY(H.FreightCode, freight.Name);
				H.TranspID       = freight.ShipID;
				H.PortOfLoading  = freight.PortOfLoading;
				H.PortOfDschrg   = freight.PortOfDischarge;
				H.DlvrLocID      = freight.DlvrAddrID;
				H.IssueDate      = freight.IssueDate;
				H.ArrivalDate    = freight.ArrivalDate;
				H.CaptainID      = freight.CaptainID;
				H.VesselsAgentID = freight.AgentID;
				H.NmbOrigsBsL    = freight.NmbOrigsBsL;
			}
			if(rec.Flags & BILLF_RENT && p_billcore->GetRentCondition(rec.ID, &rent) > 0) {
				H.RcStart  = rent.Period.low;
				H.RcFinish = rent.Period.upp;
				H.RcFlags  = rent.Flags;
				H.fRcPctCharge = BIN(rent.Flags & RENTF_PERCENT);
				H.fRcClosed    = BIN(rent.Flags & RENTF_CLOSED);
				H.RcCycle      = rent.Cycle;
				H.RcDayOffs    = (int16)rent.ChargeDayOffs;
				H.RcPercent    = rent.Percent;
				H.RcPartAmount = rent.PartAmount;
				if(H.RcCycle) {
					SString temp_buf, item_buf, id_buf, txt_buf;
					if(PPLoadText(PPTXT_CYCLELIST, temp_buf))
						for(int idx = 0; PPGetSubStr(temp_buf, idx, item_buf) > 0; idx++) {
							long   id = 0;
							if(item_buf.Divide(',', id_buf, txt_buf) > 0)
								id = id_buf.ToLong();
							else {
								id = (idx+1);
								txt_buf = item_buf;
							}
							if(id == H.RcCycle) {
								STRNSCPY(H.RcCycleText, txt_buf);
								break;
							}
						}
				}
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_Bill::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))
	#define _RET_STR     (**(SString **)rS.GetPtr(pApl->Get(0)))

	DL600_BillExt * p_ext = (DL600_BillExt *)(Extra[0].Ptr);
	BillCore * p_billcore = p_ext->P_Bill;
	if(pF->Name == "?GetAmount") {
		double amt = 0.0;
		if(p_billcore) {
			PPObjAmountType amt_obj;
			PPID   amt_id = 0;
			if(amt_obj.SearchSymb(&amt_id, _ARG_STR(1)) > 0)
				p_billcore->GetAmount(H.ID, amt_id, 0 /* curID */, &amt);
		}
		_RET_DBL = amt;
	}
	else if(pF->Name == "?GetLastPaymBill") {
		PPID   last_paym_id = 0;
		BillTbl::Rec bill_rec;
		if(p_billcore && p_billcore->GetLastPayment(H.ID, &bill_rec) > 0)
			last_paym_id = bill_rec.ID;
		_RET_INT = last_paym_id;
	}
	else if(pF->Name == "?GetReckonBill") {
		PPID   paym_pool_owner_id = 0;
		if(p_billcore)
			p_billcore->IsMemberOfPool(H.ID, PPASS_PAYMBILLPOOL, &paym_pool_owner_id);
		_RET_INT = paym_pool_owner_id;
	}
	else if(pF->Name == "?GetOrderBillSingle") {
		PPID   ord_id = 0;
		if(p_billcore) {
			PPIDArray ord_bill_list;
			if(p_billcore->GetListOfOrdersByLading(H.ID, &ord_bill_list) > 0)
				if(ord_bill_list.getCount())
					ord_id = ord_bill_list.get(0);
		}
		_RET_INT = ord_id;
	}
	else if(pF->Name == "?GetTotalTrfrQtty") {
		PPObjBill * p_bobj = BillObj;
		PPBillPacket pack;
		double sum = 0.0;
		int    sign = _ARG_INT(1);
		if(p_bobj && p_bobj->ExtractPacket(H.ID, &pack) > 0) {
			PPTransferItem * p_ti;
			for(uint i = 0; pack.EnumTItems(&i, &p_ti) > 0;) {
				double qtty = p_ti->SQtty(pack.Rec.OpID);
				if(sign == 0 || (sign > 0 && qtty > 0.0) || (sign < 0 && qtty < 0.0))
					sum += qtty;
			}
		}
		_RET_DBL = sum;
	}
	else if(pF->Name == "?GetTotalTrfrLines") {
		PPObjBill * p_bobj = BillObj;
		PPBillPacket pack;
		if(p_bobj && p_bobj->ExtractPacket(H.ID, &pack) > 0)
			_RET_INT = (int)pack.GetTCount();
		else
			_RET_INT = 0;
	}
	else if(pF->Name == "?GetCreationEvent") {
		long   sur_id = ((DL600_BillExt *)(Extra[0].Ptr))->CrEventSurID;
		if(!sur_id) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			SysJournalTbl::Rec sj_rec;
			if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, H.ID, &sj_rec) > 0) {
				DS.GetTLA().SurIdList.Add(&sur_id, &sj_rec, sizeof(sj_rec));
				((DL600_BillExt *)(Extra[0].Ptr))->CrEventSurID = sur_id;
			}
			else
				((DL600_BillExt *)(Extra[0].Ptr))->CrEventSurID = -1;
		}
		else if(sur_id < 0)
			sur_id = 0;
		_RET_INT = sur_id;
	}
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_BILL, H.ID, _ARG_STR(1));
	}
	else if(pF->Name == "?GetMemo") {
		if(p_ext->Rec.ID)
			_RET_STR = p_ext->Rec.Memo;
		else
			_RET_STR.Z();
	}
}
//
// Implementation of PPALDD_BillPool
//
PPALDD_CONSTRUCTOR(BillPool)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(BillPool) { Destroy(); }

int PPALDD_BillPool::InitData(PPFilt & rFilt, long rsrv)
{
	H.ID = rFilt.ID;
	if(rFilt.Ptr) {
		H.AssocType = *(int32 *)rFilt.Ptr;
	}
	else
		H.AssocType = PPASS_OPBILLPOOL;
	switch(H.AssocType) {
		case PPASS_OPBILLPOOL:
			H.Kind = PPBillPacket::bpkOpBill;
			H.BillID = H.ID;
			break;
		case PPASS_PAYMBILLPOOL:
			H.Kind = PPBillPacket::bpkReckon;
			H.BillID = H.ID;
			break;
		case PPASS_CSESSBILLPOOL:
			H.Kind = PPBillPacket::bpkCSess;
			H.CSessID = H.ID;
			break;
		case PPASS_TSESSBILLPOOL:
			H.Kind = PPBillPacket::bpkTSess;
			H.TSessID = H.ID;
			break;
		case PPASS_CSDBILLPOOL:
			H.Kind = PPBillPacket::bpkCSessDfct;
			H.CSessID = H.ID;
			break;
		case PPASS_TSDBILLPOOL:
			H.Kind = PPBillPacket::bpkTSessDfct;
			H.TSessID = H.ID;
			break;
		default:
			H.Kind = PPBillPacket::bpkOpBill;
			H.BillID = H.ID;
			break;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillPool::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	I.Num = 0;
	return -1;
}

int PPALDD_BillPool::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PPID   memb_id = I.InnerBillID;
	if(BillObj->P_Tbl->EnumMembersOfPool(H.AssocType, H.ID, &memb_id) > 0) {
		I.InnerBillID = memb_id;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_BillPool::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))

	_RET_INT = 0;
	if(pF->Name == "?GetMemberByOp") {
		PPObjBill * p_bobj = BillObj;
		if(p_bobj) {
			PPObjOprKind op_obj;
			PPID   op_id = 0;
			if(op_obj.SearchBySymb(_ARG_STR(1), &op_id) > 0) {
				for(PPID memb_id = 0; p_bobj->EnumMembersOfPool(H.AssocType, H.ID, &memb_id) > 0;) {
					BillTbl::Rec bill_rec;
					if(p_bobj->Search(memb_id, &bill_rec) > 0 && bill_rec.OpID == op_id) {
						_RET_INT = memb_id;
						break;
					}
				}
			}
		}
	}
}
//
// Implementation of PPALDD_GoodsBillModif
//
PPALDD_CONSTRUCTOR(GoodsBillModif)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBillModif) { Destroy(); }

int PPALDD_GoodsBillModif::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	MEMSZERO(H);
	if(!CheckOpPrnFlags(p_pack->Rec.OpID, OPKF_PRT_NBILLN))
		STRNSCPY(H.Code, p_pack->Rec.Code);
	strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	H.Dt        = p_pack->Rec.Dt;
	H.OprKindID = p_pack->Rec.OpID;
	H.ObjectID  = p_pack->Rec.Object;
	H.LocID     = p_pack->Rec.LocID;
	H.BillID    = p_pack->Rec.ID;
	H.CurID     = p_pack->Rec.CurID;

	H.fShortMainOrg = CheckOpPrnFlags(p_pack->Rec.OpID, OPKF_PRT_SHRTORG);
	PPID   main_org_id = 0;
	p_pack->GetMainOrgID_(&main_org_id);
	H.Flags = p_pack->Rec.Flags;
	if(p_pack->Rec.Object) {
		H.CntragntID  = ObjectToPerson(p_pack->Rec.Object);
		H.CntragntReq = H.CntragntID;
	}
	const  int accs_cost = BillObj->CheckRights(BILLRT_ACCSCOST);
	long   exclude_tax_flags = GTAXVF_SALESTAX;
	PPTransferItem * p_ti;
	for(uint i = 0; p_pack->EnumTItems(&i, &p_ti);) {
		double amount = 0, vatsum = 0, excisesum = 0;
		GTaxVect vect;
		int    tiamt;
		if(p_pack->OutAmtType == 1) {
			tiamt = TIAMT_COST;
			amount = p_ti->Cost * fabs(p_ti->Qtty());
		}
		else if(p_pack->OutAmtType == 2) {
			tiamt = TIAMT_PRICE;
			amount = p_ti->NetPrice() * fabs(p_ti->Qtty());
		}
		else {
			tiamt = TIAMT_AMOUNT;
			amount = fabs(p_ti->CalcAmount(0));
		}
		vect.CalcTI(p_ti, p_pack->Rec.OpID, tiamt, exclude_tax_flags);
		vatsum    = vect.GetValue(GTAXVF_VAT);
		excisesum = vect.GetValue(GTAXVF_EXCISE);
		if(p_ti->Flags & PPTFR_COSTWOVAT)
			if(p_pack->OutAmtType == 1 || (p_pack->OutAmtType != 2 && !(p_ti->Flags & PPTFR_SELLING)))
				amount += vect.GetValue(GTAXVF_VAT);

		if(p_ti->Flags & PPTFR_PLUS) {
			H.ReceiptQtty   += fabs(p_ti->Quantity_);
			if(p_ti->UnitPerPack > 0)
				H.ReceiptPacks += long(fabs(p_ti->Qtty()) / p_ti->UnitPerPack);
			if(accs_cost)
				H.ReceiptCost   += p_ti->Cost;
			H.ReceiptPrice  += p_ti->Price;
			H.ReceiptSum    += amount;
			H.ReceiptVAT    += vatsum;
			H.ReceiptExcise += excisesum;
		}
		else if(p_ti->Flags & PPTFR_MINUS) {
			H.ExpendQtty   += fabs(p_ti->Quantity_);
			if(p_ti->UnitPerPack > 0)
				H.ExpendPacks += long(fabs(p_ti->Qtty()) / p_ti->UnitPerPack);
			if(accs_cost)
				H.ExpendCost   += p_ti->Cost;
			H.ExpendPrice  += p_ti->Price;
			H.ExpendSum    += amount;
			H.ExpendVAT    += vatsum;
			H.ExpendExcise += excisesum;
		}
	}
	return (DlRtm::InitData(rFilt, rsrv) > 0) ? 1 : -1;
}

int PPALDD_GoodsBillModif::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.Iter_NN = 0;
	I.Sign_NN = 0;
	I.Sign = -1;
	return 1;
}

int PPALDD_GoodsBillModif::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPBillPacket   * p_pack = (PPBillPacket *) (Extra[0].Ptr);
		PPTransferItem * p_ti;
		GTaxVect vect;
		long   exclude_tax_flags = GTAXVF_SALESTAX;
		int    tiamt;
		uint   n = (uint)I.Iter_NN;
		while(1) {
			if(p_pack->EnumTItems(&n, &p_ti) > 0) {
				if(I.Sign < 0) {
					if(p_ti->Flags & PPTFR_MINUS)
						break;
				}
				else if(I.Sign > 0) {
					if(p_ti->Flags & PPTFR_PLUS)
						break;
				}
			}
			else if(I.Sign < 0) {
				n = 0;
				I.Sign = 1;
			}
			else
				return -1;
		}
		I.Sign_NN++;
		I.Iter_NN = n;
		I.GoodsID = p_ti->GoodsID;
		I.LotID   = p_ti->LotID;
		I.Cost    = p_ti->Cost;
		I.Price   = p_ti->Price;
		I.CurPrice = p_ti->CurPrice;
		I.CurSum   = p_ti->CurPrice * fabs(p_ti->Qtty());
		if(p_pack->OutAmtType == 1) {
			tiamt = TIAMT_COST;
			I.MainPrice = p_ti->Cost;
		}
		else if(p_pack->OutAmtType == 2) {
			tiamt = TIAMT_PRICE;
			I.MainPrice = p_ti->NetPrice();
		}
		else {
			tiamt = TIAMT_AMOUNT;
			I.MainPrice = p_ti->NetPrice();
		}
		I.Qtty = p_ti->Qtty();
		I.UnitsPerPack = p_ti->UnitPerPack;
		I.FullPack = 0;
		if(p_ti->UnitPerPack > 0)
			I.FullPack = (long)(fabs(I.Qtty) / p_ti->UnitPerPack);

		vect.CalcTI(p_ti, p_pack->Rec.OpID, tiamt, exclude_tax_flags);
		I.VATRate = vect.GetTaxRate(GTAX_VAT, 0);
		I.VATSum  = vect.GetValue(GTAXVF_VAT);
		I.ExcRate = vect.GetTaxRate(GTAX_EXCISE, 0);
		I.ExcSum  = vect.GetValue(GTAXVF_EXCISE);
		if(p_ti->Flags & PPTFR_COSTWOVAT && tiamt == TIAMT_COST) {
			I.MainPrice += /*round(*/I.VATSum / fabs(p_ti->Qtty())/*, 2)*/;
		}
		QttyToStr(p_ti->Quantity_, p_ti->UnitPerPack, ((LConfig.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_SIMPLPACK|QTTYF_FRACTION) : QTTYF_FRACTION), I.CQtty);
	}
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_GoodsReval
//
PPALDD_CONSTRUCTOR(GoodsReval)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsReval) { Destroy(); }

int PPALDD_GoodsReval::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	BillTbl::Rec rec = p_pack->Rec;

	BillTotalData total_data;
	PPObjPerson psn_obj;
	PPOprKind op_rec;
	GetOpData(rec.OpID, &op_rec);
	PPID   main_org_id = 0;
	const  PPID object_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? rec.Object2 : rec.Object;
	const  PPID person_id = ObjectToPerson(object_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(person_id && psn_obj.GetRelPersonList(person_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
			parent_person_id = rel_list.at(0);
	}
	p_pack->GetMainOrgID_(&main_org_id);
	int    exclude_vat = 0;
	MEMSZERO(H);
	H.BillID     = rec.ID;
	H.LinkBillID = rec.LinkBillID;
	H.Dt         = rec.Dt;
	H.ArticleID  = object_id;
	H.LocID      = rec.LocID;
	H.ExpendFlag = 0; // Приход (под вопросом, но переоценка и корректировка в большинстве случаев трактуются как приход)
	STRNSCPY(H.Code, rec.Code);
	STRNSCPY(H.Memo, rec.Memo);
	{
		H.RcvrID    = main_org_id;
		H.RcvrReq   = main_org_id;
		H.RcvrLocID = rec.LocID;
		H.ConsigneeReq = main_org_id;
		if(object_id) {
			if(IsIntrOp(rec.OpID) == INTRRCPT) {
				H.DlvrID = main_org_id;
				H.ConsignorReq = H.DlvrID;
			}
			else if(parent_person_id) {
				H.DlvrID = parent_person_id;
				H.ConsignorReq = person_id;
			}
			else {
				H.DlvrID = person_id;
				H.ConsignorReq = H.DlvrID;
			}
			H.DlvrReq = H.DlvrID;
		}
		if(H.DlvrID == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(object_id);
	}
	{
		PersonTbl::Rec psn_rec;
		PPID   suppl_person_id = H.DlvrID;
		if(psn_obj.Fetch(suppl_person_id, &psn_rec) > 0 && psn_rec.Flags & PSNF_NOVATAX) {
			H.fSupplIsVatExempt = 1;
			exclude_vat = 1;
		}
		else
			H.fSupplIsVatExempt = 0;
	}
	{
		long   btc_flags = BTC_CALCOUTAMOUNTS;
		SETFLAG(btc_flags, BTC_EXCLUDEVAT, exclude_vat);
		SETFLAG(btc_flags, BTC_ONLYUNLIMGOODS, p_pack->ProcessFlags & PPBillPacket::pfPrintOnlyUnlimGoods);
		p_pack->CalcTotal(&total_data, btc_flags);
		H.TotalGoodsLines = total_data.LinesCount;
		H.TotalGoodsNames = total_data.GoodsCount;
		if(total_data.VatList.getCount() >= 1) {
			H.VATRate1 = total_data.VatList.at(0).Rate;
			H.VATSum1  = total_data.VatList.at(0).VatSum;
		}
		if(total_data.VatList.getCount() >= 2) {
			H.VATRate2 = total_data.VatList.at(1).Rate;
			H.VATSum2  = total_data.VatList.at(1).VatSum;
		}
		H.TotalSum    = total_data.Amt;
		H.TotalQtty   = fabs(total_data.UnitsCount);
		H.TotalPhQtty = fabs(total_data.PhUnitsCount);
		H.TotalPacks  = total_data.PackCount;
		H.TotalBrutto = total_data.Brutto;
		H.TotalVAT    = total_data.VAT;
		H.TotalSalesTax = total_data.STax;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsReval::InitIteration(PPIterID iterId, int sortId, long)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	return 1;
}

int PPALDD_GoodsReval::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPBillPacket   * p_pack = (PPBillPacket *)Extra[0].Ptr;
		PPTransferItem * p_ti;
		uint   nn = (uint)I.nn;
		if(p_pack->EnumTItems(&nn, &p_ti)) {
			int    reval_assets_wo_vat = 1;
			double new_price = p_ti->Price;
			double new_cost  = p_ti->Cost;
			double old_price = p_ti->Discount;
			double old_cost  = p_ti->RevalCost;
			double qtty      = fabs(p_ti->Rest_);
			double vatsum_oldcost = 0.0;
			double vatsum_newcost = 0.0;
			double vatsum_oldprice = 0.0;
			double vatsum_newprice = 0.0;
			PPObjGoods    gobj;
			PPGoodsTaxEntry gtx_cost;
			PPGoodsTaxEntry gtx_price;
			gobj.FetchTax(labs(p_ti->GoodsID), p_ti->LotDate, 0, &gtx_price);
			if(p_ti->LotTaxGrpID) {
				PPObjGoodsTax gtobj;
				gtobj.Fetch(p_ti->LotTaxGrpID, p_ti->LotDate, 0, &gtx_cost);
			}
			else {
				gtx_cost = gtx_price;
			}
			double tax_qtty = qtty;
			gobj.MultTaxFactor(p_ti->GoodsID, &tax_qtty);
			if(p_ti->Flags & PPTFR_CORRECTION) {
				GTaxVect gt_vect;
				{
					long   amt_flags  = ~0L;
					long   excl_flags = 0L;
					if(p_ti->Flags & PPTFR_COSTWOVAT)
						amt_flags &= ~GTAXVF_VAT;
					if(PPObjLocation::CheckWarehouseFlags(p_ti->LocID, LOCF_VATFREE) || IsSupplVATFree(p_ti->Suppl) > 0)
						excl_flags |= GTAXVF_VAT;
					if(gtx_cost.Flags & GTAXF_NOLOTEXCISE)
						excl_flags |= GTAXVF_EXCISE;
					gt_vect.Calc_(&gtx_cost, old_cost, tax_qtty, amt_flags, excl_flags);
					vatsum_oldcost = gt_vect.GetValue(GTAXVF_VAT);
					gt_vect.Calc_(&gtx_cost, new_cost,  tax_qtty, amt_flags, excl_flags);
					vatsum_newcost  = gt_vect.GetValue(GTAXVF_VAT);
				}
				{
					long   amt_flags  = ~0L;
					long   excl_flags = 0L;
					gt_vect.Calc_(&gtx_price, old_price, tax_qtty, amt_flags, excl_flags);
					vatsum_oldprice = gt_vect.GetValue(GTAXVF_VAT);
					gt_vect.Calc_(&gtx_price, new_price,  tax_qtty, amt_flags, excl_flags);
					vatsum_newprice = gt_vect.GetValue(GTAXVF_VAT);
				}
			}
			else if(p_ti->Flags & PPTFR_REVAL && reval_assets_wo_vat && gobj.IsAsset(labs(p_ti->GoodsID)) > 0) {
				GTaxVect gt_vect;
				long   amt_flags  = ~0L;
				long   excl_flags = 0L;
				if(p_ti->Flags & PPTFR_COSTWOVAT)
					amt_flags &= ~GTAXVF_VAT;
				if(PPObjLocation::CheckWarehouseFlags(p_ti->LocID, LOCF_VATFREE) || IsSupplVATFree(p_ti->Suppl) > 0)
					excl_flags |= GTAXVF_VAT;
				if(gtx_cost.Flags & GTAXF_NOLOTEXCISE)
					excl_flags |= GTAXVF_EXCISE;
				gt_vect.Calc_(&gtx_cost, new_price, tax_qtty, amt_flags, excl_flags);
				new_price = gt_vect.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gt_vect.Calc_(&gtx_cost, new_cost,  tax_qtty, amt_flags, excl_flags);
				new_cost  = gt_vect.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gt_vect.Calc_(&gtx_cost, old_price, tax_qtty, amt_flags, excl_flags);
				old_price = gt_vect.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gt_vect.Calc_(&gtx_cost, old_cost,  tax_qtty, amt_flags, excl_flags);
				old_cost  = gt_vect.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
			}
			I.nn       = nn;
			I.GoodsID  = p_ti->GoodsID;
			I.LotID    = p_ti->LotID;
			I.NewPrice = new_price;
			I.NewCost  = new_cost;
			I.OldPrice = old_price;
			I.OldCost  = old_cost;
			I.VATRate  = gtx_cost.GetVatRate();
			I.VATSumOldCost = vatsum_oldcost;
			I.VATSumNewCost = vatsum_newcost;
			I.VATSumOldPrice = vatsum_oldprice;
			I.VATSumNewPrice = vatsum_newprice;
			if(p_ti->Flags & PPTFR_CORRECTION) {
				I.Quantity = fabs(p_ti->Quantity_);
				I.OldQtty  = fabs(p_ti->QuotPrice);
			}
			else {
				I.Quantity = qtty;
			}
		}
		else
			return -1;
	}
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsReval::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))

	if(pF->Name == "?UnlimGoodsOnly") {
		PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
		_RET_INT = BIN(p_pack && p_pack->ProcessFlags & PPBillPacket::pfAllGoodsUnlim);
	}
}
//
// Implementation of PPALDD_BillPayPlan
//
PPALDD_CONSTRUCTOR(BillPayPlan)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(BillPayPlan) { Destroy(); }

int PPALDD_BillPayPlan::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	const BillTbl::Rec * p_rec = &(((PPBillPacket *)Extra[0].Ptr)->Rec);
	MEMSZERO(H);
	H.BillID = p_rec->ID;
	if(CheckOpFlags(p_rec->OpID, OPKF_PROFITABLE))
		H.fProfitable = 1;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillPayPlan::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	const PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
	I.nn = 0;
	I.Rest = p_pack->Rec.Amount;
	return 1;
}

int PPALDD_BillPayPlan::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
		uint   n = (uint)I.nn;
		if(n < p_pack->Pays.getCount()) {
			const PayPlanTbl::Rec & r_item = p_pack->Pays.at(n);
			I.nn       = n+1;
			I.PayDate  = r_item.PayDate;
			I.Amount   = r_item.Amount;
			I.Interest = r_item.Interest;
			if(n)
				I.Rest -= p_pack->Pays.at(n-1).Amount;
		}
		else
			return -1;
	}
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_CashOrder
//
PPALDD_CONSTRUCTOR(CashOrder)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(CashOrder) { Destroy(); }

int PPALDD_CashOrder::InitData(PPFilt & rFilt, long rsrv)
{
	PPObjBill * p_bobj = BillObj;
	const  PPCommConfig & r_ccfg = CConfig;
	uint   pos;
	Acct   corr_acct;
	PPBillPacket * pack = (PPBillPacket *)rFilt.Ptr;
	int    incstax = 0, val = 0;
	PPIniFile ini_file;
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CASHORDINCSTAX, &val))
		incstax = val;
	H.Dt = pack->Rec.Dt;
	STRNSCPY(H.Code, CheckOpPrnFlags(pack->Rec.OpID, OPKF_PRT_NBILLN) ? 0 : pack->Rec.Code);
	STRNSCPY(H.Memo, pack->Rec.Memo);
	H.BillID    = pack->Rec.ID;
	H.ArticleID = pack->Rec.Object;
	H.Article2ID = pack->Rec.Object2;
	H.Amount     = fabs(pack->GetAmount());
	if(p_bobj->atobj->SearchAccturnInPacketByCorrAcc(pack, PPDEBIT, r_ccfg.CashAcct.ac, &corr_acct, &pos) ||
		p_bobj->atobj->SearchAccturnInPacketByCorrAcc(pack, PPCREDIT, r_ccfg.CashAcct.ac, &corr_acct, &pos)) {
		H.CashAc = r_ccfg.CashAcct.ac;
		H.CashSb = r_ccfg.CashAcct.sb;
		H.Ac = corr_acct.ac;
		H.Sb = corr_acct.sb;
		H.Ar = corr_acct.ar;
		PPAccTurn * at = &pack->Turns.at(pos);
		if(H.Amount == 0.0)
			H.Amount = at->Amount;
		if(!H.ArticleID) {
			AcctID acctid;
			p_bobj->atobj->ConvertAcct(&corr_acct, 0, &acctid, 0);
			H.ArticleID = acctid.ar;
		}
	}
	H.PersonReqID  = ObjectToPerson(H.ArticleID);
	H.Person2ReqID = ObjectToPerson(H.Article2ID);
	H.VAT = 0;

	PPBillPacket temp_pack, *p_tax_pack = 0;
	PPObjAmountType amtt_obj;
	TaxAmountIDs tais;
	int    is_vat_exempt = 0;
	PPID   main_org_id = 0;
	double coeff = 1.0;
	if(pack->GetMainOrgID_(&main_org_id)) {
		PersonTbl::Rec prec;
		if(SearchObject(PPOBJ_PERSON, main_org_id, &prec) > 0 && prec.Flags & PSNF_NOVATAX)
			is_vat_exempt = 1;
	}
	if(pack->OpTypeID == PPOPT_PAYMENT && pack->Rec.LinkBillID) {
		if(p_bobj->ExtractPacket(pack->Rec.LinkBillID, &temp_pack)) {
			p_tax_pack = &temp_pack;
			coeff = fdivnz(pack->GetAmount(), temp_pack.GetAmount());
		}
	}
	else if(pack->OpTypeID != PPOPT_ACCTURN)
		p_tax_pack = pack;
	if(p_tax_pack) {
		BillTotalData total_data;
		long   btc_flags = BTC_CALCOUTAMOUNTS;
		if(is_vat_exempt)
			btc_flags |= BTC_EXCLUDEVAT;
		p_tax_pack->CalcTotal(&total_data, btc_flags);
		H.VAT = total_data.VAT * coeff;
		if(total_data.VatList.getCount() >= 1) {
			H.VATRate1 = total_data.VatList.at(0).Rate;
			H.VATSum1  = total_data.VatList.at(0).VatSum * coeff;
		}
		if(total_data.VatList.getCount() >= 2) {
			H.VATRate2 = total_data.VatList.at(1).Rate;
			H.VATSum2  = total_data.VatList.at(1).VatSum * coeff;
		}
		if(incstax) {
			amtt_obj.GetTaxAmountIDs(&tais, 1);
			if(tais.STaxAmtID) {
				H.STaxSum = pack->Amounts.Get(tais.STaxAmtID, 0L);
				if(H.STaxSum) {
					H.STaxRate = fdiv100i(tais.STaxRate);
					H.Amount += H.STaxSum;
				}
			}
		}
		SETIFZ(H.STaxSum, total_data.STax * coeff);
	}
	else if(pack->OpTypeID == PPOPT_ACCTURN) {
		amtt_obj.GetTaxAmountIDs(&tais, 1);
		if(tais.VatAmtID[0]) {
			H.VATSum1 = pack->Amounts.Get(tais.VatAmtID[0], 0L);
			if(H.VATSum1)
				H.VATRate1 = fdiv100i(tais.VatRate[0]);
		}
		if(tais.VatAmtID[1]) {
			H.VATSum2  = pack->Amounts.Get(tais.VatAmtID[1], 0L);
			if(H.VATSum2)
				H.VATRate2 = fdiv100i(tais.VatRate[1]);
		}
		if(tais.STaxAmtID) {
			H.STaxSum = pack->Amounts.Get(tais.STaxAmtID, 0L);
			if(H.STaxSum)
				H.STaxRate = fdiv100i(tais.STaxRate);
		}
	}
	return DlRtm::InitData(rFilt, rsrv);
}
//
// Implementation of PPALDD_GoodsBillQCert
//
PPALDD_CONSTRUCTOR(GoodsBillQCert)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBillQCert) { Destroy(); }

int PPALDD_GoodsBillQCert::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	H.BillID = p_pack->Rec.ID;
	STRNSCPY(H.Code, p_pack->Rec.Code);
	H.Dt = p_pack->Rec.Dt;
	H.fShortMainOrg = CheckOpPrnFlags(p_pack->Rec.OpID, OPKF_PRT_SHRTORG);
	PPID   optype = GetOpType(p_pack->Rec.OpID);
	PPID   main_org_id = 0;
	p_pack->GetMainOrgID_(&main_org_id);
	if(oneof2(optype, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND) || (p_pack->Rec.Flags & (BILLF_GEXPEND | BILLF_GMODIF))) {
		H.ExpendFlag = 1;
		H.DlvrReq    = main_org_id;
		H.DlvrLocID  = p_pack->Rec.LocID;
		if(p_pack->Rec.Object)
			if(PPObjLocation::ObjToWarehouse(p_pack->Rec.Object)) {
				H.RcvrReq   = main_org_id;
				H.RcvrLocID = PPObjLocation::ObjToWarehouse(p_pack->Rec.Object);
			}
			else {
				H.RcvrReq   = ObjectToPerson(p_pack->Rec.Object);
				H.RcvrLocID = 0;
			}
	}
	else if(optype == PPOPT_DRAFTRECEIPT || p_pack->Rec.Flags & BILLF_GRECEIPT) {
		H.ExpendFlag = 2;
		H.RcvrReq   = main_org_id;
		H.RcvrLocID = p_pack->Rec.LocID;
		if(p_pack->Rec.Object)
			H.DlvrReq = (IsIntrOp(p_pack->Rec.OpID) == INTRRCPT) ? main_org_id : ObjectToPerson(p_pack->Rec.Object);
		if(H.DlvrReq == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(p_pack->Rec.Object);
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsBillQCert::InitIteration(PPIterID iterId, int sortId, long)
{
	TiIter::Order o = TiIter::ordDefault;
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	{
		I.nn = 0;
		PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
		long   f = 0;
		PPOprKind op_rec;
		if(GetOpData(p_pack->Rec.OpID, &op_rec) > 0) {
			if(op_rec.PrnFlags & OPKF_PRT_MERGETI)
				f = (ETIEF_UNITEBYGOODS | ETIEF_DIFFBYPACK | ETIEF_DIFFBYQCERT | ETIEF_DIFFBYNETPRICE);
			o = (TiIter::Order)op_rec.PrnOrder;
		}
		if(op_rec.PrnFlags & OPKF_PRT_QCERTLIST) {
			f &= ~(ETIEF_DIFFBYPACK | ETIEF_DIFFBYQCERT|ETIEF_DIFFBYNETPRICE);
			o = TiIter::ordByQCert;
			p_pack->ProcessFlags |= PPBillPacket::pfPrintQCertList;
		}
		p_pack->InitExtTIter(f, 0, o);
	}
	return 1;
}

int PPALDD_GoodsBillQCert::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
		PPTransferItem temp_ti, * p_ti;
		QualityCertTbl::Rec qc_rec;
		do {
			if(p_pack->EnumTItemsExt(0, &temp_ti) <= 0)
				return -1;
			p_ti = &temp_ti;
			I.nn++;
		} while(!p_ti->QCert || SearchObject(PPOBJ_QCERT, p_ti->QCert, &qc_rec) <= 0);
		I.QualityCertID = p_ti->QCert;
		I.GoodsID = p_ti->GoodsID;
		I.LotID   = p_ti->LotID;
		I.Qtty    = fabs(p_ti->Qtty());
		STRNSCPY(I.LExpiry, qc_rec.SPrDate);
	}
	return DlRtm::NextIteration(iterId);
}
//
// Implementation PPALDD_BillList
//
PPALDD_CONSTRUCTOR(BillList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(BillList) { Destroy(); }

int PPALDD_BillList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Bill, rsrv);
	H.FltLocID     = p_filt->LocList.GetSingle();
	H.FltOprKindID = p_filt->OpID;
	H.FltArtID     = p_filt->ObjectID;
	H.FltPayerID   = p_filt->PayerID;
	H.FltAgentID   = p_filt->AgentID;
	H.FltCurID     = p_filt->CurID;
	H.FltBeg       = p_filt->Period.low;
	H.FltEnd       = p_filt->Period.upp;
	H.fAllCurrencies = BIN(p_filt->Flags & BillFilt::fAllCurrencies);
	H.InDebt  = 0;
	H.OutDebt = 0;
	if(p_filt->ObjectID && p_filt->Flags & BillFilt::fDebtsWithPayments) {
		BillTotal total;
		p_v->CalcTotal(&total);
		H.InDebt  = total.InSaldo;
		H.OutDebt = total.OutSaldo;
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewBill * p_v = (PPViewBill*)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration((PPViewBill::IterOrder)SortIdx);
	return 1;
}

int PPALDD_BillList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Bill);
	I.BillID = item.ID;
	I.Debit  = item.Debit;
	I.Credit = item.Credit;
	I.Saldo  = item.Saldo;
	I.LastPaymDate = item.LastPaymDate;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_BillList::Destroy() { DESTROY_PPVIEW_ALDD(Bill); }
//
// Implementation PPALDD_ContentBList
//
PPALDD_CONSTRUCTOR(ContentBList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(ContentBList) { Destroy(); }

int PPALDD_ContentBList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Bill, rsrv);
	H.FltBeg       = p_filt->Period.low;
	H.FltEnd       = p_filt->Period.upp;
	H.FltLocID     = p_filt->LocList.GetSingle();
	H.FltOprKindID = p_filt->OpID;
	H.FltPayerID   = p_filt->PayerID;
	H.FltCurID     = p_filt->CurID;
	H.fAllCurrencies = BIN(p_filt->Flags & BillFilt::fAllCurrencies);
	BillTotalData total;
	BillViewItem item;
	for(p_v->InitIteration(PPViewBill::OrdByDefault); p_v->NextIteration(&item) > 0;)
		p_v->CalcItemTotal(item.ID, &total);
	BillVatEntry * p_vat_entry;
	for(uint i = 0; total.VatList.enumItems(&i, (void **)&p_vat_entry);) {
		if(i == 1) {
			H.TotalVatRate1 = p_vat_entry->Rate;
			H.TotalVat1     = p_vat_entry->VatSum;
		}
		else if(i == 2) {
			H.TotalVatRate2 = p_vat_entry->Rate;
			H.TotalVat2     = p_vat_entry->VatSum;
		}
		else if(i == 3) {
			H.TotalVatRate3 = p_vat_entry->Rate;
			H.TotalVat3     = p_vat_entry->VatSum;
		}
		else if(i == 4) {
			H.TotalVatRate4 = p_vat_entry->Rate;
			H.TotalVat4     = p_vat_entry->VatSum;
		}
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_ContentBList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewBill * p_v = (PPViewBill*)(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration((PPViewBill::IterOrder)SortIdx);
	I.recNo = 0;
	I.grpNo = 0;
	return 1;
}

int PPALDD_ContentBList::NextIteration(PPIterID iterId)
{
	struct VIterState {
		int    IsDraft;
	};
	IterProlog(iterId, 0);
	PPViewBill * p_v = (PPViewBill*)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	PPObjBill * p_bobj = BillObj;
	BillViewItem item;
	PPTransferItem ti;
	CpTrfrExt ext;
	while(I.recNo || p_v->NextIteration(&item) > 0) {
		if(!I.recNo) {
			I.BillID = item.ID;
			I.grpNo++;
			VIterState vis;
			vis.IsDraft = IsDraftOp(item.OpID);
			p_v->SetIterState(&vis, sizeof(vis));
		}
		int    r = 0;
		const  VIterState * p_vis = (const VIterState *)p_v->GetIterState();
		if(p_vis && p_vis->IsDraft) {
			if(p_bobj->P_CpTrfr)
		 		r = p_bobj->P_CpTrfr->EnumItems(I.BillID, (int*)&I.recNo, &ti, &ext);
		}
		else
			r = p_bobj->trfr->EnumItems(I.BillID, (int*)&I.recNo, &ti);
		if(r > 0) {
			double cost = 0.0;
			double old_cost = 0.0;
			double price = 0.0;
			double old_price = 0.0;
			double qtty  = 0.0;
			double old_qtty = 0.0;
			if(ti.Flags & PPTFR_REVAL) {
				old_cost = p_bobj->CheckRights(BILLRT_ACCSCOST) ? ti.RevalCost : 0.0;
				old_price = ti.Discount;
			}
			else {
				cost = old_cost = p_bobj->CheckRights(BILLRT_ACCSCOST) ? ti.Cost : 0.0;
				price = old_price = ti.NetPrice();
			}
			if(ti.Flags & PPTFR_CORRECTION) {
				qtty = fabs(ti.Quantity_);
				old_qtty  = fabs(ti.QuotPrice);
			}
			else {
				qtty = old_qtty = fabs(ti.Qtty());
			}
			GTaxVect vect;
			vect.CalcTI(&ti, item.OpID, TIAMT_AMOUNT);
		   	I.VaTax    = vect.GetTaxRate(GTAX_VAT, 0);
			I.VatSum   = vect.GetValue(GTAXVF_VAT);
			I.ExTax    = vect.GetTaxRate(GTAX_EXCISE, 0); // @v10.0.05 @fix GTAXVF_EXCISE-->GTAX_EXCISE
			I.ExtSum   = vect.GetValue(GTAXVF_EXCISE);
			I.StTax    = vect.GetTaxRate(GTAX_SALES, 0);
			I.StSum    = vect.GetValue(GTAXVF_SALESTAX);
			if(ti.Flags & PPTFR_COSTWOVAT) {
				vect.CalcTI(&ti, item.OpID, TIAMT_COST);
				cost += vect.GetValue(GTAXVF_VAT) / qtty;
				old_cost += vect.GetValue(GTAXVF_VAT) / old_qtty;
			}
			if(ti.Flags & PPTFR_PRICEWOTAXES) {
				vect.CalcTI(&ti, item.OpID, TIAMT_PRICE);
				price = vect.GetValue(GTAXVF_BEFORETAXES) / qtty;
				old_price = vect.GetValue(GTAXVF_BEFORETAXES) / old_qtty;
			}
			I.GoodsID  = ti.GoodsID;
			I.LotID    = ti.LotID;
			I.Cost     = cost;
			I.OldCost  = old_cost;
			I.Price    = price;
			I.OldPrice = old_price;
			I.CurPrice = ti.CurPrice;
			I.Quantity = qtty;
			I.OldQtty  = old_qtty;
			return DlRtm::NextIteration(iterId);
		}
		else {
			I.recNo = 0;
			PPWaitPercent(p_v->GetCounter());
		}
	}
	return -1;
}

void PPALDD_ContentBList::Destroy() { DESTROY_PPVIEW_ALDD(Bill); }
//
// Implementation of PPALDD_AmountType
//
PPALDD_CONSTRUCTOR(AmountType)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(AmountType) { Destroy(); }

int PPALDD_AmountType::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPAmountType amt_t_rec;
		if(PPRef->GetItem(PPOBJ_AMOUNTTYPE, rFilt.ID, &amt_t_rec) > 0) {
			H.ID = amt_t_rec.ID;
			STRNSCPY(H.Name, amt_t_rec.Name);
			STRNSCPY(H.Symb, amt_t_rec.Symb);
			H.Flags   = amt_t_rec.Flags;
			H.Tax     = amt_t_rec.Tax;
			H.TaxRate = amt_t_rec.TaxRate;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_AdvanceRep
//
PPALDD_CONSTRUCTOR(AdvanceRep)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AdvanceRep) { Destroy(); }

int PPALDD_AdvanceRep::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	MEMSZERO(H);
	H.BillID = p_pack->Rec.ID;
	if(p_pack->P_AdvRep) {
		STRNSCPY(H.Rcp1Text, p_pack->P_AdvRep->Rcp[0].Text);
		H.Rcp1Dt  = p_pack->P_AdvRep->Rcp[0].Dt;
		H.Rcp1Amt = p_pack->P_AdvRep->Rcp[0].Amount;
		STRNSCPY(H.Rcp2Text, p_pack->P_AdvRep->Rcp[1].Text);
		H.Rcp2Dt  = p_pack->P_AdvRep->Rcp[1].Dt;
		H.Rcp2Amt = p_pack->P_AdvRep->Rcp[1].Amount;
		H.InRest         = p_pack->P_AdvRep->InRest;
		H.RcpAmount      = p_pack->P_AdvRep->RcpAmount;
		H.ExpAmount      = p_pack->P_AdvRep->ExpAmount;
		H.OutRest        = p_pack->P_AdvRep->OutRest;
		H.NumAddedBills  = p_pack->P_AdvRep->NumAddedBills;
		H.NumAddedSheets = p_pack->P_AdvRep->NumAddedSheets;
	}
	return (DlRtm::InitData(rFilt, rsrv) > 0) ? 1 : -1;
}

int PPALDD_AdvanceRep::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	H.nn = 0;
	return 1;
}

int PPALDD_AdvanceRep::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPBillPacket * p_pack = (PPBillPacket *)(Extra[0].Ptr);
	if(H.nn < (int16)p_pack->AdvList.GetCount()) {
		PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(H.nn);
		STRNSCPY(I.AdvCode, r_item.AdvCode);
		I.AdvDt = r_item.AdvDt;
		I.AdvBillKindID = r_item.AdvBillKindID;
		GetObjectName(PPOBJ_ADVBILLKIND, r_item.AdvBillKindID, I.AdvBillKindName, sizeof(I.AdvBillKindName));
		I.AdvBillID = r_item.AdvBillID;
		I.AccID     = r_item.AccID;
		I.ArID      = r_item.ArID;
		I.Flags     = r_item.Flags;
		I.Amt       = r_item.Amount;
		I.ExtAmt    = r_item.ExtAmt;
		//STRNSCPY(I.Memo, r_item.Memo); // @v7.8.2
		H.nn++;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_BnkPaymOrder
//
PPALDD_CONSTRUCTOR(BnkPaymOrder)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(BnkPaymOrder) { Destroy(); }

int PPALDD_BnkPaymOrder::InitData(PPFilt & rFilt, long rsrv)
{
	BnkAcctData bnk_data;
	PPObjPerson psn_obj;
	PPBillPacket * pack = (PPBillPacket *)rFilt.Ptr;
	if(pack && pack->P_PaymOrder) {
		SString temp_buf;
		H.PayerID = pack->P_PaymOrder->PayerID;
		H.RcvrID  = pack->P_PaymOrder->RcvrID;
		H.BillID  = pack->P_PaymOrder->BillID;
		H.LclDt   = pack->P_PaymOrder->Dt;
		STRNSCPY(H.LclCode, pack->P_PaymOrder->Code);
		STRNSCPY(H.LclMemo, pack->Rec.Memo);
		H.PayerBnkAccID = pack->P_PaymOrder->PayerBnkAccID;
		bnk_data.InitFlags = BADIF_INITALLBR;
		psn_obj.GetBnkAcctData(H.PayerBnkAccID, (const PPBankAccount *)0, &bnk_data);
		H.PayerBnkID = bnk_data.Bnk.ID;
		//STRNSCPY(H.PayerBnkName, bnk_data.Bnk.Name);
		STRNSCPY(H.PayerBnkCity, bnk_data.Bnk.City);
		STRNSCPY(H.PayerBIC,     bnk_data.Bnk.BIC);
		STRNSCPY(H.PayerBnkCorAcc, bnk_data.Bnk.CorrAcc);
		STRNSCPY(H.PayerAcc,     bnk_data.Acct);
		H.RcvrBnkAccID  = pack->P_PaymOrder->RcvrBnkAccID;
		bnk_data.InitFlags = BADIF_INITALLBR;
		psn_obj.GetBnkAcctData(H.RcvrBnkAccID, (const PPBankAccount *)0, &bnk_data);
		H.RcvrBnkID = bnk_data.Bnk.ID;
		//STRNSCPY(H.RcvrBnkName, bnk_data.Bnk.Name);
		STRNSCPY(H.RcvrBnkCity,   bnk_data.Bnk.City);
		STRNSCPY(H.RcvrBIC,       bnk_data.Bnk.BIC);
		STRNSCPY(H.RcvrBnkCorAcc, bnk_data.Bnk.CorrAcc);
		STRNSCPY(H.RcvrAcc,       bnk_data.Acct);
		H.BnkPaymMethod = pack->P_PaymOrder->BnkPaymMethod;
		H.BnkQueueing   = pack->P_PaymOrder->BnkQueueing;
		H.PayerStatus   = pack->P_PaymOrder->PayerStatus;
		{
			temp_buf.Z();
			if(pack->P_PaymOrder->PayerStatus) {
				// @v9.7.0 longfmtz(pack->P_PaymOrder->PayerStatus, 2, H.TxtPayerStatus, sizeof(H.TxtPayerStatus));
				temp_buf.Z().CatLongZ(pack->P_PaymOrder->PayerStatus, 2); // @v9.7.0
			}
			STRNSCPY(H.TxtPayerStatus, temp_buf);
		}
		H.Amount        = pack->P_PaymOrder->Amount;
		H.VATRate       = pack->P_PaymOrder->VATRate;
		H.VATSum        = pack->P_PaymOrder->VATSum;
		STRNSCPY(H.TxmClass, pack->P_PaymOrder->Txm.TaxClass2);
		STRNSCPY(H.TxmOKATO, pack->P_PaymOrder->Txm.OKATO);
		STRNSCPY(H.TxmReason, pack->P_PaymOrder->Txm.Reason);
		pack->P_PaymOrder->Txm.Period.Format(temp_buf).CopyTo(H.TxmPeriod, sizeof(H.TxmPeriod));
		STRNSCPY(H.TxmDocNumber, pack->P_PaymOrder->Txm.DocNumber);
		H.TxmDocDate = pack->P_PaymOrder->Txm.DocDate;
		STRNSCPY(H.TxmPaymType, pack->P_PaymOrder->Txm.PaymType);
		STRNSCPY(H.UIN, pack->P_PaymOrder->Txm.UIN);
	}
	else if(pack) {
		H.BillID  = pack->Rec.ID;
		H.LclDt   = pack->Rec.Dt;
		STRNSCPY(H.LclCode, pack->Rec.Code);
		STRNSCPY(H.LclMemo, pack->Rec.Memo);
		H.Amount = pack->GetAmount();
	}
	return DlRtm::InitData(rFilt, rsrv);
}
//
// Implementation of PPALDD_BillTotal
//
PPALDD_CONSTRUCTOR(BillTotal)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(BillTotal) { Destroy(); }

int PPALDD_BillTotal::InitData(PPFilt & rFilt, long rsrv)
{
	BillTotalPrintData * p_data = 0;
	if(rsrv) {
		p_data = (BillTotalPrintData *)rFilt.Ptr;
		Extra[1].Ptr = p_data;
		H.Count    = p_data->P_Total->Count;
		H.Nominal  = p_data->P_Total->Sum;
		H.Debt     = p_data->P_Total->Debt;
		H.FltBeg   = p_data->P_Filt->Period.low;
		H.FltEnd   = p_data->P_Filt->Period.upp;
		H.FltLocID = p_data->P_Filt->LocList.GetSingle();
		H.FltOpID  = p_data->P_Filt->OpID;
		H.FltObjID = p_data->P_Filt->ObjectID;
		H.Flags    = p_data->P_Filt->Flags;
		return DlRtm::InitData(rFilt, rsrv);
	}
	else
		return 0;
}

int PPALDD_BillTotal::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.LineNo = 0;
	return 1;
}

int PPALDD_BillTotal::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	BillTotalPrintData * p_data = (BillTotalPrintData *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	AmtEntry * p_item;
	uint   n = (uint)I.LineNo;
	if(p_data->P_Total->Amounts.enumItems(&n, (void **)&p_item) > 0) {
		I.LineNo = n;
		I.AmtTypeID = p_item->AmtTypeID;
		I.CurID = p_item->CurID;
		I.Amt = p_item->Amt;
	}
	else if(n == 0) {
		I.LineNo = 1;
		I.AmtTypeID = 1;
		I.CurID = 0;
		I.Amt = p_data->P_Total->Sum;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}

void PPALDD_BillTotal::Destroy()
{
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_AssetReceipt
//
PPALDD_CONSTRUCTOR(AssetReceipt)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AssetReceipt) { Destroy(); }

int PPALDD_AssetReceipt::InitData(PPFilt & rFilt, long rsrv)
{
	AssetCard * p_data = (AssetCard *)rFilt.Ptr;
	Extra[1].Ptr = p_data;
	H.LotID      = p_data->LotID;
	H.OrgLotID   = p_data->OrgLotID;
	H.StartCost  = p_data->OrgCost;
	H.StartPrice = p_data->OrgPrice;
	H.AssetAccID = p_data->AssetAcctID.ac;
	H.AssetArID  = p_data->AssetAcctID.ar;
	H.ExplBillID = p_data->ExplBillID;
	H.MovLineNo  = 0;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_AssetReceipt::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	H.MovLineNo  = 0;
	return 1;
}

int PPALDD_AssetReceipt::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	AssetCard * p_data = (AssetCard *)Extra[1].Ptr;
	AssetCard::MovItem * p_item;
	uint   n = (uint)H.MovLineNo;
	if(p_data && p_data->P_MovList && p_data->P_MovList->enumItems(&n, (void **)&p_item) > 0) {
		H.MovLineNo = (long)n;
		I.MovBillID = p_item->BillID;
		I.MovLotID  = p_item->LotID;
		I.DestLocID = p_item->DestLocID;
		I.MovPrice  = p_item->Price;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}

void PPALDD_AssetReceipt::Destroy()
{
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_Warrant (only WIN32)
//
PPALDD_CONSTRUCTOR(Warrant)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Warrant) { Destroy(); }

int PPALDD_Warrant::InitData(PPFilt & rFilt, long rsrv)
{
	LDATE  dt;
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)Extra[0].Ptr;
	MEMSZERO(H);
	H.BillID = p_pack->Rec.ID;
	STRNSCPY(H.WarrantNo,  p_pack->Rec.Code);
	H.WarrantDt = p_pack->Rec.Dt;
	p_pack->GetLastPayDate(&dt);
	H.ExpiryDt = dt;
	if(p_pack->Rec.Object && p_pack->AccSheetID)
		H.PersonReqID = ObjectToPerson(p_pack->Rec.Object, 0);
	H.SupplID = p_pack->Rec.Object2;
	STRNSCPY(H.Memo, p_pack->Rec.Memo);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Warrant::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.LineNo = 0;
	return 1;
}

int PPALDD_Warrant::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPBillPacket * p_pack = (PPBillPacket *)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	if(I.LineNo < (int16)p_pack->AdvList.GetCount()) {
		char   buf[128];
		const  PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(I.LineNo);
		STRNSCPY(I.GdsName, r_item.Memo);
		GetObjectName(PPOBJ_UNIT, r_item.ArID, I.Unit, sizeof(I.Unit));
		numbertotext(r_item.Amount, NTTF_NOZERO|NTTF_FIRSTCAP|NTTF_DECCURR, buf);
		STRNSCPY(I.Qtty, buf);
		I.LineNo++;
		return DlRtm::NextIteration(iterId);
	}
	else
		return -1;
}

void PPALDD_Warrant::Destroy()
{
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_BillInfo
//
// AHTOXA {
PPALDD_CONSTRUCTOR(BillInfo)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(BillInfo) { Destroy(); }

int PPALDD_BillInfo::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	PPBillPacket * p_pack = (PPBillPacket *)rFilt.Ptr;
	const  long f = p_pack->Rec.Flags;
	H.BillID = p_pack->Rec.ID;
	H.fTotalDiscount     = BIN(f & BILLF_TOTALDISCOUNT);
	H.fGReceipt          = BIN(f & BILLF_GRECEIPT);
	H.fGExpend           = BIN(f & BILLF_GEXPEND);
	H.fGReval            = BIN(f & BILLF_GREVAL);
	H.fGModif            = BIN(f & BILLF_GMODIF);
	H.fClosedOrder       = BIN(f & BILLF_CLOSEDORDER);
	H.fCash              = BIN(f & BILLF_CASH);
	H.fCCheck            = BIN(f & BILLF_CHECK);
	H.fNoAturn           = BIN(f & BILLF_NOATURN);
	H.fRmvExcise         = BIN(f & BILLF_RMVEXCISE);
	H.fFixedAmounts      = BIN(f & BILLF_FIXEDAMOUNTS);
	H.fFreight           = BIN(f & BILLF_FREIGHT);
	H.fRent              = BIN(f & BILLF_RENT);
	H.fRecon             = BIN(f & BILLF_RECKON);
	H.fBanking           = BIN(f & BILLF_BANKING);
	H.fShipped           = BIN(f & BILLF_SHIPPED);
	H.fWritedOff         = BIN(f & BILLF_WRITEDOFF);
	H.fCSessWrOff        = BIN(f & BILLF_CSESSWROFF);
	H.fAdvanceRep        = BIN(f & BILLF_ADVANCEREP);
	H.fTGGLexCsNPrice    = BIN(f & BILLF_TGGLEXCSNPRICE);
	H.fTSessWrOff        = BIN(f & BILLF_TSESSWROFF);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillInfo::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	return 1;
}

int PPALDD_BillInfo::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	uint   n = (uint)I.nn;
	PPBillPacket * p_pack = (PPBillPacket*)Extra[0].Ptr;
	if(n < p_pack->Amounts.getCount()) {
		const AmtEntry & r_amt = p_pack->Amounts.at(n);
		I.nn         = n + 1;
		I.AmountID   = r_amt.AmtTypeID;
		I.CurrencyID = r_amt.CurID;
		I.Amount     = r_amt.Amt;
		return DlRtm::NextIteration(iterId);
	}
	else
		return -1;
}
//
// Implementation of PPALDD_BillInfoList
//
PPALDD_CONSTRUCTOR(BillInfoList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(BillInfoList) { Destroy(); }

int PPALDD_BillInfoList::InitData(PPFilt & rFilt, long rsrv)
{
	Extra[0].Ptr = rFilt.Ptr;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillInfoList::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	BillInfoListPrintData * p_bilpd = (BillInfoListPrintData*)Extra[0].Ptr;
	I.nn = 0;
	ZDELETE(p_bilpd->P_Pack);
	return p_bilpd->P_V->InitIteration(PPViewBill::OrdByDefault);
}

int PPALDD_BillInfoList::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	int    ok = 1;
	uint   n = (uint)I.nn;
	BillViewItem item;
	PPBillPacket * p_pack = 0;
	BillInfoListPrintData * p_bilpd = (BillInfoListPrintData*)Extra[0].Ptr;
	if(!p_bilpd->P_Pack || n >= p_bilpd->P_Pack->Amounts.getCount()) {
		if(p_bilpd->P_V->NextIteration(&item) > 0) {
			p_pack = new PPBillPacket();
			ok = p_bilpd->P_V->GetPacket(item.ID, p_pack);
			ZDELETE(p_bilpd->P_Pack);
			p_bilpd->P_Pack = p_pack;
			n = 0;
			PPWaitPercent(p_bilpd->P_V->GetCounter());
		}
		else
			ok = -1;
	}
	if(ok > 0) {
		const long f = p_bilpd->P_Pack->Rec.Flags;
		I.BillID = p_bilpd->P_Pack->Rec.ID;
		I.fTotalDiscount     = BIN(f & BILLF_TOTALDISCOUNT);
		I.fGReceipt          = BIN(f & BILLF_GRECEIPT);
		I.fGExpend           = BIN(f & BILLF_GEXPEND);
		I.fGReval            = BIN(f & BILLF_GREVAL);
		I.fGModif            = BIN(f & BILLF_GMODIF);
		I.fClosedOrder       = BIN(f & BILLF_CLOSEDORDER);
		I.fCash              = BIN(f & BILLF_CASH);
		I.fCCheck            = BIN(f & BILLF_CHECK);
		I.fNoAturn           = BIN(f & BILLF_NOATURN);
		I.fRmvExcise         = BIN(f & BILLF_RMVEXCISE);
		I.fFixedAmounts      = BIN(f & BILLF_FIXEDAMOUNTS);
		I.fFreight           = BIN(f & BILLF_FREIGHT);
		I.fRent              = BIN(f & BILLF_RENT);
		I.fRecon             = BIN(f & BILLF_RECKON);
		I.fBanking           = BIN(f & BILLF_BANKING);
		I.fShipped           = BIN(f & BILLF_SHIPPED);
		I.fWritedOff         = BIN(f & BILLF_WRITEDOFF);
		I.fCSessWrOff        = BIN(f & BILLF_CSESSWROFF);
		I.fAdvanceRep        = BIN(f & BILLF_ADVANCEREP);
		I.fTGGLexCsNPrice    = BIN(f & BILLF_TGGLEXCSNPRICE);
		I.fTSessWrOff        = BIN(f & BILLF_TSESSWROFF);
		if(n < p_bilpd->P_Pack->Amounts.getCount()) {
			AmtEntry amt = p_bilpd->P_Pack->Amounts.at(n);
			I.nn         = n + 1;
			I.AmountID   = amt.AmtTypeID;
			I.CurrencyID = amt.CurID;
			I.Amount     = amt.Amt;
		}
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
// } AHTOXA

//
// Implementation of PPALDD_UhttBill
//
struct UhttBillBlock {
	UhttBillBlock() : ItemsCounter(0)
	{
	}
	PPBillPacket Pack;
	uint   ItemsCounter;
};

PPALDD_CONSTRUCTOR(UhttBill)
{
	if(Valid) {
		Extra[0].Ptr = new UhttBillBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData("iter@Items", &I_Items, sizeof(I_Items));
	}
}

PPALDD_DESTRUCTOR(UhttBill)
{
	Destroy();
	delete (UhttBillBlock *)Extra[0].Ptr;
}

int PPALDD_UhttBill::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	MEMSZERO(H);
	PPViewBill bv;
	PPBillExt  ext;
	SString temp_buf;
	UhttBillBlock & r_blk = *(UhttBillBlock *)Extra[0].Ptr;
	if(bv.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
		PPObjBill * p_bobj = BillObj;
		H.ID = r_blk.Pack.Rec.ID;
		H.Dt = r_blk.Pack.Rec.Dt;
		H.OprKindID = r_blk.Pack.Rec.OpID;
		STRNSCPY(H.Code, r_blk.Pack.Rec.Code);
		H.LocID = r_blk.Pack.Rec.LocID;
		H.ArticleID = r_blk.Pack.Rec.Object;
		if(r_blk.Pack.P_Freight)
			H.DlvrLocID = r_blk.Pack.P_Freight->DlvrAddrID;
		H.CurID = r_blk.Pack.Rec.CurID;
		if(p_bobj->P_Tbl->GetExtraData(r_blk.Pack.Rec.ID, &ext) > 0)
            H.AgentID = ext.AgentID;
		H.StatusID = r_blk.Pack.Rec.StatusID;
		H.Amount = r_blk.Pack.Rec.Amount;
		H.Discount = r_blk.Pack.Amounts.Get(PPAMT_DISCOUNT, 0);
		{
			S_GUID   guid;
			p_bobj->GetGuid(r_blk.Pack.Rec.ID, &guid);
			guid.ToStr(S_GUID::fmtIDL, temp_buf.Z());
			STRNSCPY(H.GUID, temp_buf);
		}
		H.Flags = 0;
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttBill::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttBillBlock & r_blk = *(UhttBillBlock *)Extra[0].Ptr;
	r_blk.ItemsCounter = 0;
	return -1;
}

int PPALDD_UhttBill::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	if(iterId == GetIterID("iter@Items")) {
		UhttBillBlock & r_blk = *(UhttBillBlock *)Extra[0].Ptr;
		PPTransferItem * p_ti = 0;
		if(r_blk.Pack.EnumTItems(&r_blk.ItemsCounter, &p_ti) > 0) {
			if(p_ti) {
				I_Items.GoodsID = p_ti->GoodsID;
				//I_Items.Serial;
				I_Items.Qtty = p_ti->Quantity_;
				I_Items.Cost = p_ti->Cost;
				I_Items.Price = p_ti->Price;
				I_Items.Discount = p_ti->Discount;
				I_Items.Amount = p_ti->Quantity_ * p_ti->Price;
				I_Items.Flags = p_ti->Flags;
				ok = DlRtm::NextIteration(iterId);
			}
		}
	}
	return ok;
}

//
// Implementation of PPALDD_UhttDraftTransitGoodsRestList
//
PPALDD_CONSTRUCTOR(UhttDraftTransitGoodsRestList)
{
	if(Valid) {
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData(rscDefIter, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(UhttDraftTransitGoodsRestList) { Destroy(); }

int PPALDD_UhttDraftTransitGoodsRestList::InitData(PPFilt & rFilt, long rsrv)
{
	TSVector <UhttGoodsRestVal> * p_list = (TSVector <UhttGoodsRestVal> *)rFilt.Ptr; // @v9.8.11 TSArray-->TSVector
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttDraftTransitGoodsRestList::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	TSVector <UhttGoodsRestVal> * p_list = (TSVector <UhttGoodsRestVal> *)Extra[0].Ptr; // @v9.8.11 TSArray-->TSVector
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_UhttDraftTransitGoodsRestList::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	TSVector <UhttGoodsRestVal> * p_list = (TSVector <UhttGoodsRestVal> *)Extra[0].Ptr; // @v9.8.11 TSArray-->TSVector
	if(p_list && (p_list->getPointer() < p_list->getCount())) {
		UhttGoodsRestVal & r_gr_val = p_list->at(p_list->getPointer());
		I.GoodsID = r_gr_val.GoodsID;
		I.LocID = r_gr_val.LocID;
		I.Rest = r_gr_val.Rest;
		I.RestBillDt = r_gr_val.RestBillDt;
		//
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
