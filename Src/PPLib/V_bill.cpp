// V_BILL.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
BillFilt::FiltExtraParam::FiltExtraParam(long setupValues, BrowseBillsType bbt) : SetupValues(setupValues), Bbt(bbt)
{
}

IMPLEMENT_PPFILT_FACTORY(Bill); BillFilt::BillFilt() : PPBaseFilt(PPFILT_BILL, 0, 5), P_SjF(0), P_TagF(0), P_ContractorPsnTagF(0) // @v11.7.4 ver 3-->4 // @v11.9.6 ver 4-->5
{
	SetFlatChunk(offsetof(BillFilt, ReserveStart),
		offsetof(BillFilt, ReserveEnd)-offsetof(BillFilt, ReserveStart)+sizeof(ReserveEnd));
	SetBranchObjIdListFilt(offsetof(BillFilt, List));
	SetBranchObjIdListFilt(offsetof(BillFilt, LocList));
	SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(BillFilt, P_SjF));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt, P_TagF));
	SetBranchDisplayExtList(offsetof(BillFilt, Dl));
	//
	SetBranchSString(offsetof(BillFilt, ExtString));          // @v11.7.4
	SetBranchObjIdListFilt(offsetof(BillFilt, ObjList));      // @v11.7.4 @reserve
	SetBranchObjIdListFilt(offsetof(BillFilt, Obj2List));     // @v11.7.4 @reserve 
	SetBranchObjIdListFilt(offsetof(BillFilt, AgentList));    // @v11.7.4 @reserve
	SetBranchObjIdListFilt(offsetof(BillFilt, ReservedList)); // @v11.7.4 @reserve
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt, P_ContractorPsnTagF)); // @v11.9.6
	//
	Init(1, 0);
	Bbt = bbtUndef;
}

int BillFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 4) {
		class BillFilt_v4 : public PPBaseFilt, public PPExtStrContainer {
		public:
			BillFilt_v4() : PPBaseFilt(PPFILT_BILL, 0, 4), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(BillFilt_v4, ReserveStart),
					offsetof(BillFilt_v4, ReserveEnd)-offsetof(BillFilt_v4, ReserveStart)+sizeof(ReserveEnd));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, List));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, LocList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(BillFilt_v4, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt_v4, P_TagF));
				SetBranchDisplayExtList(offsetof(BillFilt_v4, Dl));
				SetBranchSString(offsetof(BillFilt_v4, ExtString));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, ObjList));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, Obj2List));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, AgentList));
				SetBranchObjIdListFilt(offsetof(BillFilt_v4, ReservedList));
				Init(1, 0);
				Bbt = bbtUndef;
			}
			char   ReserveStart[20];
			PPID   FreightPortOfDischarge;
			PPID   CliPsnCategoryID;
			PPID   GoodsGroupID;
			long   Tag;
			DateRange DuePeriod;
			uint32 Count;
			int16  Ft_Declined;
			int16  Ft_CheckPrintStatus;
			PPID   StorageLocID;
			int16  EdiRecadvStatus;
			int16  EdiRecadvConfStatus;
			int16  OrderFulfillmentStatus;
			uint8  Reserve[6];
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
			long   ReserveEnd;
			SysJournalFilt * P_SjF;
			ObjIdListFilt List;
			ObjIdListFilt LocList;
			ObjIdListFilt ObjList;
			ObjIdListFilt Obj2List;
			ObjIdListFilt AgentList;
			ObjIdListFilt ReservedList;
			TagFilt * P_TagF;
			PPViewDisplayExtList Dl;
		};
		BillFilt_v4 fv4;
		THROW(fv4.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
		memzero(Reserve, sizeof(Reserve));
#define CPYFLD(f) f = fv4.f
			CPYFLD(FreightPortOfDischarge);
			CPYFLD(CliPsnCategoryID);
			CPYFLD(GoodsGroupID);
			CPYFLD(Tag);
			CPYFLD(DuePeriod);
			CPYFLD(Count);
			CPYFLD(Ft_Declined);
			CPYFLD(Ft_CheckPrintStatus);
			CPYFLD(StorageLocID);
			CPYFLD(EdiRecadvStatus);
			CPYFLD(EdiRecadvConfStatus);
			CPYFLD(OrderFulfillmentStatus);
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
			CPYFLD(ObjList);
			CPYFLD(Obj2List);
			CPYFLD(AgentList);
			CPYFLD(ReservedList);
			CPYFLD(Dl);
#undef CPYFLD
		if(fv4.P_SjF) {
			THROW_MEM(P_SjF = new SysJournalFilt);
			*P_SjF = *fv4.P_SjF;
		}
		else
			ZDELETE(P_SjF);
		if(fv4.P_TagF) {
			THROW_MEM(P_TagF = new TagFilt);
			*P_TagF = *fv4.P_TagF;
		}
		else
			ZDELETE(P_TagF);
		P_ContractorPsnTagF = 0; // new field
		ok = 1;
	}
	else if(ver == 3) {
		class BillFilt_v3 : public PPBaseFilt {
		public:
			BillFilt_v3() : PPBaseFilt(PPFILT_BILL, 0, 3), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(BillFilt_v3, ReserveStart),
					offsetof(BillFilt_v3, ReserveEnd)-offsetof(BillFilt_v3, ReserveStart)+sizeof(ReserveEnd));
				SetBranchObjIdListFilt(offsetof(BillFilt_v3, List));
				SetBranchObjIdListFilt(offsetof(BillFilt_v3, LocList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(BillFilt_v3, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(BillFilt_v3, P_TagF));
				SetBranchDisplayExtList(offsetof(BillFilt_v3, Dl));
				Init(1, 0);
				Bbt = bbtUndef;
			}
			char   ReserveStart[24];
			PPID   CliPsnCategoryID;
			PPID   GoodsGroupID;
			long   Tag;
			DateRange DuePeriod;
			uint32 Count;
			int16  Ft_Declined;
			int16  Ft_CheckPrintStatus;
			PPID   StorageLocID;
			int16  EdiRecadvStatus;
			int16  EdiRecadvConfStatus;
			int16  OrderFulfillmentStatus;
			uint8  Reserve[6];
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
			PPViewDisplayExtList Dl;
		};		
		BillFilt_v3 fv3;
		THROW(fv3.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
		memzero(Reserve, sizeof(Reserve));
#define CPYFLD(f) f = fv3.f
			CPYFLD(CliPsnCategoryID);
			CPYFLD(GoodsGroupID);
			CPYFLD(Tag);
			CPYFLD(DuePeriod);
			CPYFLD(Count);
			CPYFLD(Ft_Declined);
			CPYFLD(Ft_CheckPrintStatus);
			CPYFLD(StorageLocID);
			CPYFLD(EdiRecadvStatus);
			CPYFLD(EdiRecadvConfStatus);
			CPYFLD(OrderFulfillmentStatus);
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
			CPYFLD(ReserveEnd); // @anchor
#undef CPYFLD
		if(fv3.P_SjF) {
			THROW_MEM(P_SjF = new SysJournalFilt);
			*P_SjF = *fv3.P_SjF;
		}
		else
			ZDELETE(P_SjF);
		if(fv3.P_TagF) {
			THROW_MEM(P_TagF = new TagFilt);
			*P_TagF = *fv3.P_TagF;
		}
		else
			ZDELETE(P_TagF);
		PPExtStrContainer::Z(); // new field
		ObjList.Z(); // new field
		Obj2List.Z(); // new field
		AgentList.Z(); // new field
		ReservedList.Z(); // new field
		ok = 1;
	}
	else if(ver == 2) {
		class BillFilt_v2 : public PPBaseFilt {
		public:
			BillFilt_v2() : PPBaseFilt(PPFILT_BILL, 0, 2), P_SjF(0), P_TagF(0)
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
		Dl.Z(); // new field
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*virtual*/int BillFilt::Describe(long flags, SString & rBuf) const
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
	PutObjMembToBuf(PPOBJ_BILLSTATUS,  StatusID,   STRINGIZE(StatusID),    rBuf);
	PutMembToBuf(&Period,              STRINGIZE(Period),     rBuf);
	PutMembToBuf(&PaymPeriod,          STRINGIZE(PaymPeriod), rBuf);
	PutMembToBuf(&DuePeriod,           STRINGIZE(DuePeriod),  rBuf);
	PutMembToBuf(static_cast<long>(Ft_STax),        STRINGIZE(Ft_STax),   rBuf);
	PutMembToBuf(static_cast<long>(Ft_ClosedOrder), STRINGIZE(Ft_ClosedOrder),       rBuf);
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
	else if(SortOrder == ordByDateCode)
		PutMembToBuf(STRINGIZE(ordByDateCode), STRINGIZE(SortOrder), rBuf);
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
	PPBaseFilt::Copy(&s, 0);
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
		default:
			bbt = bbtUndef;
			break;
	}
	Bbt = bbt;
}

class BillFiltDialog : public WLDialog {
	enum {
		ctlgroupLoc = 1
	};
public:
	BillFiltDialog(uint dlgID, const char * pAddText) : WLDialog(dlgID, CTL_BILLFLT_LABEL)
	{
		Data.Sel = -1;
		setSubTitle(pAddText);
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_BILLFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		SetupCalPeriod(CTLCAL_BILLFLT_PERIOD, CTL_BILLFLT_PERIOD);
		SetupCalPeriod(CTLCAL_BILLFLT_DUEPERIOD, CTL_BILLFLT_DUEPERIOD);
	}
	int    setDTS(const BillFilt *);
	int    getDTS(BillFilt *);
private:
	DECL_HANDLE_EVENT;
	void   setupAccSheet(PPID accSheetID, PPID accSheet2ID);
 	void   extraFilt();
	void   ExtraFilt2(); // @v11.9.4 @construction (will replace the extraFilt)
	void   viewOptions();
	void   SetupLocationCombo();
	BillFilt Data;
};

int BillFilterDialog(uint dlgID, BillFilt * pFilt, TDialog ** ppDlg, const char * pAddText)
{
	int    r;
	int    valid_data = 0;
	BillFiltDialog * dlg = 0;
	if(*ppDlg == 0) {
		if(!CheckDialogPtr(&(dlg = new BillFiltDialog(dlgID, pAddText))))
			return 0;
		*ppDlg = dlg;
	}
	else
		dlg = static_cast<BillFiltDialog *>(*ppDlg);
	dlg->setDTS(pFilt);
	while(!valid_data && (r = ExecView(dlg)) == cmOK)
		valid_data = dlg->getDTS(pFilt);
	return r;
}

int BillFilterDialog(uint dlgID, BillFilt * pFilt, const char * pAddText)
{
	BillFiltDialog * dlg = 0;
	const int r = BillFilterDialog(dlgID, pFilt, reinterpret_cast<TDialog**>(&dlg), pAddText);
	delete dlg;
	return r;
}

void BillFiltDialog::viewOptions()
{
	TDialog * dlg = new TDialog(DLG_BILLFLTVOPT);
	if(CheckDialogPtrErr(&dlg)) {
		long   flags = 0;
		const LAssoc sftab[] = {
			{ BillFilt::dliFreightIssueDate,   0x0001 },
			{ BillFilt::dliFreightArrivalDate, 0x0002 },
			{ BillFilt::dliDueDate,            0x0004 },
			{ BillFilt::dliAgentName,          0x0008 },
			{ BillFilt::dliAlcoLic,            0x0010 },
			{ BillFilt::dliDlvrAddr,           0x0020 },
			{ BillFilt::dliTSessLinkTo,        0x0040 }, // @v11.6.12
			{ BillFilt::dliStdAmtCost,         0x0080 }, // @v12.1.0 Стандартная сумма в ценах поступления // 
			{ BillFilt::dliStdAmtPrice,        0x0100 }, // @v12.1.0 Стандартная сумма в ценах реализации //
		};
		// PPViewDisplayExtList Dl
		uint i;
		for(i = 0; i < SIZEOFARRAY(sftab); i++) {
			if(Data.Dl.GetItemByDataId(sftab[i].Key, 0))
				flags |= sftab[i].Val;
			dlg->AddClusterAssoc(CTL_BILLFLTVOPT_FLAGS, i, sftab[i].Val);
		}
		const long preserve_flags = flags;
		dlg->SetClusterData(CTL_BILLFLTVOPT_FLAGS, flags);
		{
			PrcssrAlcReport::Config arp_cfg;
			const bool enbl_alclic = (PrcssrAlcReport::ReadConfig(&arp_cfg) > 0 && arp_cfg.AlcLicRegTypeID);
			dlg->DisableClusterItem(CTL_BILLFLTVOPT_FLAGS, 4, !enbl_alclic);
		}
		if(ExecView(dlg) == cmOK) {
			flags = dlg->GetClusterData(CTL_BILLFLTVOPT_FLAGS);
			if(flags != preserve_flags) {
				for(i = 0; i < SIZEOFARRAY(sftab); i++) {
					if(flags & sftab[i].Val)
						Data.Dl.SetItem(sftab[i].Key, 0, 0);
					else
						Data.Dl.RemoveItem(sftab[i].Key);
				}
			}
		}
	}
}

IMPL_HANDLE_EVENT(BillFiltDialog)
{
	WLDialog::handleEvent(event);
	if(event.isCmd(cmTags)) {
		if(!SETIFZ(Data.P_TagF, new TagFilt()))
			PPError(PPERR_NOMEM);
		else if(!EditTagFilt(PPOBJ_BILL, Data.P_TagF))
			PPError();
		if(Data.P_TagF->IsEmpty())
			ZDELETE(Data.P_TagF);
	}
	// @v11.9.6 {
	else if(event.isCmd(cmContractorTags)) {
		if(!SETIFZ(Data.P_ContractorPsnTagF, new TagFilt()))
			PPError(PPERR_NOMEM);
		else if(!EditTagFilt(PPOBJ_PERSON, Data.P_ContractorPsnTagF))
			PPError();
		if(Data.P_ContractorPsnTagF->IsEmpty())
			ZDELETE(Data.P_ContractorPsnTagF);
	}
	// } @v11.9.6 
	else if(event.isCbSelected(CTLSEL_BILLFLT_OPRKIND)) {
		const  PPID prev_op_id = Data.OpID;
		getCtrlData(CTLSEL_BILLFLT_OPRKIND, &Data.OpID);
		if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
			PPID   acc_sheet_id = 0;
			PPID   acc_sheet2_id = 0;
			GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, &acc_sheet2_id);
			setupAccSheet(acc_sheet_id, acc_sheet2_id);
		}
		if(Data.OpID != prev_op_id) {
			const  PPID op_type_id = GetOpType(Data.OpID, 0);
			if(op_type_id == PPOPT_DRAFTTRANSIT)
                SetupLocationCombo();
		}
	}
	else if(event.isCmd(cmaMore)) {
		// @v11.9.4 extraFilt();
		ExtraFilt2(); // @v11.9.4
	}
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

void BillFiltDialog::ExtraFilt2()
{
	class BillFiltExtDialog : public TDialog {
		DECL_DIALOG_DATA(BillFilt);
		const PPID AgentAcsID;
		const PPID PayerAcsID;
	public:
		BillFiltExtDialog() : TDialog(DLG_BILLEXTFLT2), AgentAcsID(GetAgentAccSheet()), PayerAcsID(GetSellAccSheet())
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			SetupArCombo(this, CTLSEL_BILLEXT_PAYER, Data.PayerID, OLW_CANINSERT|OLW_LOADDEFONOPEN, PayerAcsID, sacfDisableIfZeroSheet|sacfNonGeneric);
			SetupArCombo(this, CTLSEL_BILLEXT_AGENT, Data.AgentID, OLW_CANINSERT|OLW_LOADDEFONOPEN, AgentAcsID, sacfDisableIfZeroSheet|sacfNonGeneric);
			if(Data.OrderFulfillmentStatus < 0) 
				showCtrl(CTL_BILLEXTFLT_ORDFFST, false);
			PPAccessRestriction accsr;
			const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
			setCtrlUInt16(CTL_BILLEXTFLT_STAXTGGL, (Data.Ft_STax > 0) ? 1 : ((Data.Ft_STax < 0) ? 2 : 0));
			setCtrlUInt16(CTL_BILLEXTFLT_DCLTGGL,  (Data.Ft_Declined > 0) ? 1 : ((Data.Ft_Declined < 0) ? 2 : 0));
			setCtrlUInt16(CTL_BILLEXTFLT_CHECKPRST, (Data.Ft_CheckPrintStatus > 0) ? 1 : ((Data.Ft_CheckPrintStatus < 0) ? 2 : 0)); // @erik
            {
                AddClusterAssocDef(CTL_BILLEXTFLT_RECADV, 0, PPEDI_RECADV_STATUS_UNDEF);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 1, PPEDI_RECADV_STATUS_ACCEPT);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 2, PPEDI_RECADV_STATUS_PARTACCEPT);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 3, PPEDI_RECADV_STATUS_REJECT);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADV, 4, -1);
                SetClusterData(CTL_BILLEXTFLT_RECADV, Data.EdiRecadvStatus);
            }
            {
                AddClusterAssocDef(CTL_BILLEXTFLT_RECADVCFM, 0, PPEDI_RECADVCONF_STATUS_UNDEF);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 1, PPEDI_RECADVCONF_STATUS_ACCEPT);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 2, PPEDI_RECADVCONF_STATUS_REJECT);
                AddClusterAssoc(CTL_BILLEXTFLT_RECADVCFM, 3, -1);
                SetClusterData(CTL_BILLEXTFLT_RECADVCFM, Data.EdiRecadvConfStatus);
            }
			disableCtrl(CTLSEL_BILLEXT_CREATOR, own_bill_restr);
			SetupPPObjCombo(this, CTLSEL_BILLEXT_CREATOR, PPOBJ_USR, Data.CreatorID, OLW_CANSELUPLEVEL);
			SetupCalPeriod(CTLCAL_BILLEXT_DUEPERIOD, CTL_BILLEXT_DUEPERIOD);
			SetPeriodInput(this, CTL_BILLEXT_DUEPERIOD, Data.DuePeriod);
			SetupPPObjCombo(this, CTLSEL_BILLEXTFLT_GGRP, PPOBJ_GOODSGROUP, Data.GoodsGroupID, OLW_CANSELUPLEVEL|OLW_WORDSELECTOR); // @v11.0.11
			SetupPPObjCombo(this, CTLSEL_BILLEXTFLT_CLICAT, PPOBJ_PRSNCATEGORY, Data.CliPsnCategoryID, 0); // @v11.1.9
			 // @v11.9.4 {
			{
				PPIDArray worldobj_kind_list;
				worldobj_kind_list.addzlist(WORLDOBJ_CITY, WORLDOBJ_CITYAREA, 0L);
				SetupPPObjCombo(this, CTLSEL_BILLEXTFLT_DPORT, PPOBJ_WORLD, Data.FreightPortOfDischarge, OLW_CANINSERT|OLW_CANSELUPLEVEL|OLW_WORDSELECTOR,
					PPObjWorld::MakeExtraParam(worldobj_kind_list, 0, 0));
			}
			// } @v11.9.4 
			if(Data.Bbt == bbtOrderBills && Data.OrderFulfillmentStatus >= 0) {
				AddClusterAssocDef(CTL_BILLEXTFLT_ORDFFST, 0, 0);
				AddClusterAssoc(CTL_BILLEXTFLT_ORDFFST, 1, 1);
				AddClusterAssoc(CTL_BILLEXTFLT_ORDFFST, 2, 2);
				AddClusterAssoc(CTL_BILLEXTFLT_ORDFFST, 3, 3);
				SetClusterData(CTL_BILLEXTFLT_ORDFFST, Data.OrderFulfillmentStatus);
			}
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v;
			Data.PayerID = PayerAcsID ? getCtrlLong(CTLSEL_BILLEXT_PAYER) : 0;
			Data.AgentID = AgentAcsID ? getCtrlLong(CTLSEL_BILLEXT_AGENT) : 0;
			{
				v = getCtrlUInt16(CTL_BILLEXTFLT_STAXTGGL);
				Data.Ft_STax = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
			}
			{
				v = getCtrlUInt16(CTL_BILLEXTFLT_DCLTGGL);
				Data.Ft_Declined = (v == 1) ? 1 : ((v == 2) ? -1 : 0);
			}
			// @erik {
			{
				v = getCtrlUInt16(CTL_BILLEXTFLT_CHECKPRST);
				Data.Ft_CheckPrintStatus = (v==1) ? 1 : ((v==2) ? -1 : 0);
			}
			// } @erik
			Data.EdiRecadvStatus = static_cast<int16>(GetClusterData(CTL_BILLEXTFLT_RECADV));
			Data.EdiRecadvConfStatus = static_cast<int16>(GetClusterData(CTL_BILLEXTFLT_RECADVCFM));
			getCtrlData(CTLSEL_BILLEXT_CREATOR, &Data.CreatorID);
			getCtrlData(CTLSEL_BILLEXTFLT_GGRP, &Data.GoodsGroupID); // @v11.0.11
			getCtrlData(CTLSEL_BILLEXTFLT_CLICAT, &Data.CliPsnCategoryID); // @v11.1.9
			getCtrlData(CTLSEL_BILLEXTFLT_DPORT, &Data.FreightPortOfDischarge); // @v11.9.4
			// @v11.1.8 {
			if(Data.Bbt == bbtOrderBills) {
				if(Data.OrderFulfillmentStatus >= 0 && getCtrlView(CTL_BILLEXTFLT_ORDFFST))
					Data.OrderFulfillmentStatus = static_cast<int16>(GetClusterData(CTL_BILLEXTFLT_ORDFFST));
			}
			// } @v11.1.8 
			THROW_PP(GetPeriodInput(this, sel = CTL_BILLEXT_DUEPERIOD, &Data.DuePeriod), CTL_BILLEXT_DUEPERIOD);
			//
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
		}
	};
	if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
		GetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod);
	}
	const  PPID cur_user_id = LConfig.UserID;
	//
	ushort v = getCtrlUInt16(CTL_BILLFLT_FLAGS);
	PPAccessRestriction accsr;
	const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
	if(v & 0x02 || own_bill_restr == 1) {
		SETIFZQ(Data.CreatorID, cur_user_id);
	}
	else if(own_bill_restr == 2) {
		PPObjSecur sec_obj(PPOBJ_USR, 0);
		PPSecur sec_rec;
		if(sec_obj.Fetch(cur_user_id, &sec_rec) > 0)
			SETIFZQ(Data.CreatorID, (sec_rec.ParentID | PPObjSecur::maskUserGroup));
	}
	else if(Data.CreatorID == cur_user_id)
		Data.CreatorID = 0;
	//
	if(PPDialogProcBody<BillFiltExtDialog, BillFilt>(&Data)) {
		if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
			SetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, Data.DuePeriod);
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

void BillFiltDialog::extraFilt()
{
	if(GetSellAccSheet() || GetAgentAccSheet()) {
		if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
			GetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, &Data.DuePeriod);
		}
		const  PPID cur_user_id = LConfig.UserID;
		PPBillExt ext;
		ext.PayerID = Data.PayerID;
		ext.AgentID = Data.AgentID;
		ext.Ft_STax = Data.Ft_STax;
		ext.Ft_Declined = Data.Ft_Declined;
		ext.Ft_CheckPrintStatus = Data.Ft_CheckPrintStatus;
		ext.EdiRecadvStatus = Data.EdiRecadvStatus;
		ext.EdiRecadvConfStatus = Data.EdiRecadvConfStatus;
		ext.DuePeriod = Data.DuePeriod;
		ext.GoodsGroupID = Data.GoodsGroupID; // @v11.0.11
		ext.OrderFulfillmentStatus = (Data.Bbt == bbtOrderBills) ? Data.OrderFulfillmentStatus : -1; // @v11.1.8
		ext.CliPsnCategoryID = Data.CliPsnCategoryID; // @v11.1.9
		ushort v = getCtrlUInt16(CTL_BILLFLT_FLAGS);
		PPAccessRestriction accsr;
		const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
		if(v & 0x02 || own_bill_restr == 1) {
			SETIFZQ(Data.CreatorID, cur_user_id);
		}
		else if(own_bill_restr == 2) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(cur_user_id, &sec_rec) > 0)
				SETIFZQ(Data.CreatorID, (sec_rec.ParentID | PPObjSecur::maskUserGroup));
		}
		else if(Data.CreatorID == cur_user_id)
			Data.CreatorID = 0;
		ext.CreatorID = Data.CreatorID;
		if(BillExtraDialog(0, &ext, 0, 2) > 0) {
			Data.PayerID = ext.PayerID;
			Data.AgentID = ext.AgentID;
			Data.Ft_STax = ext.Ft_STax;
			Data.Ft_Declined = ext.Ft_Declined;
			Data.Ft_CheckPrintStatus = ext.Ft_CheckPrintStatus;
			Data.EdiRecadvStatus = ext.EdiRecadvStatus;
			Data.EdiRecadvConfStatus = ext.EdiRecadvConfStatus;
			Data.CreatorID = ext.CreatorID;
			Data.DuePeriod = ext.DuePeriod;
			Data.GoodsGroupID = ext.GoodsGroupID; // @v11.0.11
			Data.OrderFulfillmentStatus = (Data.Bbt == bbtOrderBills) ? ext.OrderFulfillmentStatus : -1; // @v11.1.8
			Data.CliPsnCategoryID = ext.CliPsnCategoryID; // @v11.1.9
			if(getCtrlView(CTL_BILLFLT_DUEPERIOD)) {
				SetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, Data.DuePeriod);
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
	const  PPID op_type_id = GetOpType(Data.OpID, 0);
	LocationCtrlGroup * p_cgrp = static_cast<LocationCtrlGroup *>(getGroup(ctlgroupLoc));
	if(p_cgrp) {
		if(op_type_id == PPOPT_DRAFTTRANSIT) {
			PPIDArray loc_list;
			BillObj->P_Tbl->GetLocListByOp(Data.OpID, loc_list);
			p_cgrp->SetExtLocList(loc_list.getCount() ? &loc_list : 0);
		}
		else
			p_cgrp->SetExtLocList(0);
		setGroupData(ctlgroupLoc, &loc_rec);
	}
}

int BillFiltDialog::setDTS(const BillFilt * pFilt)
{
	if(!Data.PPBaseFilt::IsEq(pFilt, 0)) {
		const  PPID cur_user_id = LConfig.UserID;
		ushort v;
		PPID   acc_sheet_id = 0;
		PPID   acc_sheet2_id = 0;
		int    is_op_kind_list = 0;
		PPIDArray types;
		PPObjOprKind op_obj;
		Data = *pFilt;
		SetupLocationCombo();
		SetPeriodInput(this, CTL_BILLFLT_PERIOD, Data.Period);
		SetPeriodInput(this, CTL_BILLFLT_DUEPERIOD, Data.DuePeriod);
		if(Data.Flags & BillFilt::fOrderOnly)
			types.addzlist(PPOPT_GOODSORDER, PPOPT_GENERIC, 0L);
		else if(Data.Flags & BillFilt::fAccturnOnly) {
			types.addzlist(PPOPT_ACCTURN, PPOPT_AGREEMENT, PPOPT_GENERIC, 0L);
		}
		else if(Data.Flags & BillFilt::fInvOnly)
			types.add(PPOPT_INVENTORY);
		else if(Data.Flags & BillFilt::fPoolOnly)
			types.add(PPOPT_POOL);
		else if(Data.Flags & BillFilt::fDraftOnly)
			types.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, PPOPT_GENERIC, 0L);
		else if(Data.Flags & BillFilt::fWmsOnly)
			types.addzlist(PPOPT_WAREHOUSE, 0L);
		else {
			PPBillPoolOpEx bpox;
			if(Data.PoolOpID && op_obj.GetPoolExData(Data.PoolOpID, &bpox) > 0 && bpox.OpList.getCount()) {
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
		DisableClusterItem(CTL_BILLFLT_FLAGS, 3, Data.Bbt != bbtGoodsBills);
		SETFLAG(v, 0x01, Data.Flags & BillFilt::fSetupNewBill);
		SETFLAG(v, 0x02, Data.CreatorID == cur_user_id);
		SETFLAG(v, 0x04, Data.Flags & BillFilt::fShowWoAgent);
		SETFLAG(v, 0x08, Data.Flags & BillFilt::fDiscountOnly);
		//SETFLAG(v, 0x10, Data.Flags & BillFilt::fCcPrintedOnly); // @v9.7.12 @erikTMP
		setCtrlData(CTL_BILLFLT_FLAGS, &v);
		setWL(BIN(Data.Flags & BillFilt::fLabelOnly));
		AddClusterAssoc(CTL_BILLFLT_ORDER, 0, BillFilt::ordByDate);
		AddClusterAssoc(CTL_BILLFLT_ORDER, 1, BillFilt::ordByDateCode); // @v11.0.11
		AddClusterAssoc(CTL_BILLFLT_ORDER, 2, BillFilt::ordByCode);     // @v11.0.11 1-->2
		AddClusterAssoc(CTL_BILLFLT_ORDER, 3, BillFilt::ordByObject);   // @v11.0.11 2-->3
		SetClusterData(CTL_BILLFLT_ORDER, static_cast<long>(Data.SortOrder));
		// @v11.7.4 {
		if(getCtrlView(CTL_BILLFLT_MEMOTEXT)) {
			SString temp_buf;
			Data.GetExtStrData(BillFilt::extssMemoText, temp_buf);
			setCtrlString(CTL_BILLFLT_MEMOTEXT, temp_buf);
		}
		// } @v11.7.4 
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
	getGroupData(ctlgroupLoc, &loc_rec);
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
		SETIFZ(Data.CreatorID, r_cfg.UserID);
	}
	else if(Data.CreatorID == r_cfg.UserID)
		Data.CreatorID = 0;
	SETFLAG(Data.Flags, BillFilt::fShowWoAgent, v & 0x04);
	SETFLAG(Data.Flags, BillFilt::fLabelOnly, getWL());
	SETFLAG(Data.Flags, BillFilt::fDiscountOnly, v & 0x08);
	//SETFLAG(Data.Flags, BillFilt::fCcPrintedOnly, v & 0x10); // @v9.7.12 @erikTMP
	sel = CTL_BILLFLT_PERIOD;
	THROW((Data.Flags & BillFilt::fDebtOnly) || AdjustPeriodToRights(temp_period, 1));
	Data.Period = temp_period;
	Data.SortOrder = (int16)GetClusterData(CTL_BILLFLT_ORDER);
	// @v11.7.4 {
	if(getCtrlView(CTL_BILLFLT_MEMOTEXT)) {
		SString temp_buf;
		getCtrlString(CTL_BILLFLT_MEMOTEXT, temp_buf);
		Data.PutExtStrData(BillFilt::extssMemoText, temp_buf);
	}
	// } @v11.7.4 
	ASSIGN_PTR(pFilt, Data);
	CATCHZOKPPERRBYDLG
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
PPViewBill::ArFilterBlock::ArFilterBlock() : IsActual(false)
{
}
		
PPViewBill::ArFilterBlock & PPViewBill::ArFilterBlock::Z()
{
	IsActual = false;
	InclList.Z();
	ExclList.Z();
	return *this;
}

PPViewBill::PoolInsertionParam::PoolInsertionParam() : Verb(2), AddedBillKind(bbtGoodsBills)
{
}

PPViewBill::PPViewBill() : PPView(0, &Filt, PPVIEW_BILL, implUseQuickTagEditFunc, 0), P_TempTbl(0), P_TempOrd(0), P_BPOX(0), P_Arp(0),
	P_BObj(BillObj), State(0), P_IterState(0), LastSelID(0), P_Dl600BPackCache(0)
{
}

PPViewBill::~PPViewBill()
{
	delete P_TempTbl;
	delete P_TempOrd;
	delete P_BPOX;
	delete P_Arp;
	delete P_Dl600BPackCache; // @v12.2.0
	SAlloc::F(P_IterState);
	DBRemoveTempFiles();
}

static int IterProc_CrList(const BillViewItem * pItem, void * pExtraPtr)
{
	return (pExtraPtr && static_cast<PPIDArray *>(pExtraPtr)->add(pItem->ID)) ? 1 : PPSetErrorSLib();
}

int PPViewBill::GetBillIDList(PPIDArray * pList)
{
	return Enumerator(0, IterProc_CrList, pList);
}

int PPViewBill::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	if(Filt.Flags & BillFilt::fShippedOnly && Filt.Flags & BillFilt::fUnshippedOnly)
		Filt.Flags &= ~BillFilt::fShippedOnly;
	Filt.Period.Actualize(ZERODATE);
	Filt.PaymPeriod.Actualize(ZERODATE);
	Filt.DuePeriod.Actualize(ZERODATE);
	GoodsList.Z(); // @v11.0.11
	ArFBlk.Z(); // @v11.9.6
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
	SETFLAG(State, stNoTempTbl, BIN(Filt.Flags & BillFilt::fNoTempTable)); // @v11.8.10 @fix LotFilt::fNoTempTable-->BillFilt::BillFilt
	IdList = Filt.List;
	GetOpList(&Filt, &OpList, &SingleOpID);
	THROW_PP(OpList.getCount(), PPERR_VIEWBYFILTISEMPTY);
	if(SingleOpID) {
		if(!Filt.OpID || IsGenericOp(Filt.OpID) > 0)
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
			if(Filt.LocList.GetCount()) {
				PPIDArray full_loc_list;
				LocObj.GetWarehouseList(&full_loc_list, 0);
				THROW(LocObj.ResolveWarehouseList(&Filt.LocList.Get(), LocList_));
				LocList_.intersect(&full_loc_list);
			}
			else if(ObjRts.IsLocRights()) {
				PPIDArray full_loc_list;
				LocObj.GetWarehouseList(&full_loc_list, 0);
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
		PPObjOprKind op_obj;
		THROW_MEM(P_BPOX = new PPBillPoolOpEx);
		op_obj.GetPoolExData(Filt.PoolOpID, P_BPOX);
	}
	// @v11.9.6 {
	if(Filt.HasMultiArRestriction()) {
		bool do_intersect = false;
		if(Filt.ObjList.GetCount()) {
			Filt.ObjList.Get(ArFBlk.InclList);
			do_intersect = true;
			ArFBlk.IsActual = true;
		}
		if(Filt.P_ContractorPsnTagF && !Filt.P_ContractorPsnTagF->IsEmpty()) {
			PPObjTag tag_obj;
			UintHashTable selection_list;
			UintHashTable exclude_list;
			tag_obj.GetObjListByFilt(PPOBJ_PERSON, Filt.P_ContractorPsnTagF, selection_list, exclude_list);
			{
				if(do_intersect) {
					ArFBlk.InclList.intersect(&selection_list);
				}
				else {
					for(ulong iter_id = 0; selection_list.Enum(&iter_id) > 0;) {
						ArFBlk.InclList.add(static_cast<long>(iter_id));
					}
				}
			}
			{
				for(ulong iter_id = 0; exclude_list.Enum(&iter_id) > 0;) {
					ArFBlk.ExclList.add(static_cast<long>(iter_id));
				}
			}
			ArFBlk.IsActual = true;
		}
		ArFBlk.InclList.sortAndUndup();
		ArFBlk.ExclList.sortAndUndup();
	}
	// } @v11.9.6 
	{
		SysJournalFilt * p_sjf = Filt.P_SjF;
		if(p_sjf && !p_sjf->IsEmpty()) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			PPIDArray local_id_list;
			p_sjf->Period.Actualize(ZERODATE);
			THROW(p_sj->GetObjListByEventPeriod(PPOBJ_BILL, p_sjf->UserID, &p_sjf->ActionIDList, &p_sjf->Period, local_id_list));
			if(IdList.IsExists())
				local_id_list.intersect(&IdList.Get());
			IdList.Set(&local_id_list);
		}
	}
	if(!(Filt.Flags & BillFilt::fIgnoreRtPeriod))
		THROW(AdjustPeriodToRights(Filt.Period, 0));
	// @v11.0.11 {
	if(Filt.GoodsGroupID) {
		GoodsIterator::GetListByGroup(Filt.GoodsGroupID, &GoodsList);
		// Если выборка товаров по группе пуста, то это значит, что и вся выборка документов будет пустой.
		THROW_PP(GoodsList.getCount(), PPERR_VIEWBYFILTISEMPTY);
		GoodsList.sortAndUndup();
	}
	// } @v11.0.11 
	// @v11.7.4 {
	{
		SString memo_pattern;
		Filt.GetExtStrData(BillFilt::extssMemoText, memo_pattern); 
		if(memo_pattern.NotEmptyS()) {
			Reference * p_ref = PPRef;
			PPIDArray id_list;
			PPIDArray local_id_list;
			if(Enumerator(enfSkipExtssMemo, IterProc_CrList, &id_list)) {
				id_list.sortAndUndup();
				p_ref->UtrC.FilterIdList(PPOBJ_BILL, PPTRPROP_MEMO, memo_pattern, &id_list, local_id_list);
			}
			if(IdList.IsExists())
				local_id_list.intersect(&IdList.Get());
			IdList.Set(&local_id_list);
		}
	}
	// } @v11.7.4 
	if(IsTempTblNeeded()) {
		IterOrder ord = OrdByDefault;
		switch(Filt.SortOrder) {
			case BillFilt::ordByDate: ord = OrdByDate; break;
			case BillFilt::ordByCode: ord = OrdByCode; break;
			case BillFilt::ordByObject: ord = OrdByObjectName; break;
			case BillFilt::ordByDateCode: ord = OrdByDateCode; break; // @v11.0.11
		}
		THROW(CreateTempTable(ord, 0));
	}
	CATCHZOK
	return ok;
}

int PPViewBill::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	int    caption = -1;
	uint   rez_id = DLG_BILLFLT;
	if(pFilt) {
		const BrowseBillsType bbt = static_cast<const BillFilt *>(pFilt)->Bbt;
		BillFilt * p_filt = static_cast<BillFilt *>(pFilt);
		switch(bbt) {
			case bbtOrderBills:
				p_filt->Flags |= BillFilt::fOrderOnly;
				caption = 1;
				rez_id = DLG_BILLFLT_ORDER;
				break;
			case bbtInventoryBills:
				p_filt->Flags |= BillFilt::fInvOnly;
				caption = 3;
				rez_id = DLG_INVFLT;
				break;
			case bbtPoolBills:
				p_filt->Flags |= BillFilt::fPoolOnly;
				caption = 4;
				break;
			case bbtAccturnBills:
				p_filt->Flags |= BillFilt::fAccturnOnly;
				caption = 2;
				break;
			case bbtDraftBills:
				p_filt->Flags |= BillFilt::fDraftOnly;
				caption = 5;
				rez_id = DLG_BILLFLT_DRAFT;
				break;
			case bbtWmsBills:
				p_filt->Flags |= BillFilt::fWmsOnly;
				caption = 6;
				break;
			default:
				caption = 0;
				break;
		}
		{
			PPID   single_op_id = 0;
			PPIDArray op_list;
			GetOpList(p_filt, &op_list, &single_op_id);
			SString temp_buf;
			if(op_list.getCount()) {
				TDialog * dlg = 0;
				if(single_op_id)
					p_filt->OpID = single_op_id;
				if(caption >= 0)
					PPGetSubStr(PPTXT_BILLFLTCAPTIONS, caption, temp_buf);
				else
					temp_buf.Z();
				int r = BillFilterDialog(rez_id, p_filt, &dlg, temp_buf);
				assert(!dlg->H() || ::IsWindow(dlg->H())); // @debug
				ZDELETE(dlg);
				if(r == cmOK)
					ok = 1;
				else if(!r)
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
		}
	}
	return ok;
}

/*virtual*/PPBaseFilt * PPViewBill::CreateFilt(const void * extraPtr) const
{
	BillFilt * p_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_BILL, reinterpret_cast<PPBaseFilt **>(&p_filt))) {
		const PPConfig & r_cfg = LConfig;
		const BillFilt::FiltExtraParam * p = extraPtr ? static_cast<const BillFilt::FiltExtraParam *>(extraPtr) : 0;
		if(p) {
			p_filt->Bbt = p->Bbt;
			if(p->SetupValues) {
				p_filt->Period.SetDate(r_cfg.OperDate);
				p_filt->LocList.Add(r_cfg.Location);
				if(p_filt->Bbt == bbtGoodsBills) {
					if(DS.CheckExtFlag(ECF_GOODSBILLFILTSHOWDEBT))
						p_filt->Flags |= BillFilt::fShowDebt;
				}
				else if(p_filt->Bbt == bbtDraftBills)
					p_filt->Ft_ClosedOrder = -1;
			}
		}
		PPAccessRestriction accsr;
		const int own_bill_restr = ObjRts.GetAccessRestriction(accsr).GetOwnBillRestrict();
		if(own_bill_restr == 1)
			p_filt->CreatorID = r_cfg.UserID;
		else if(own_bill_restr == 2) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(r_cfg.UserID, &sec_rec) > 0)
				p_filt->CreatorID = (sec_rec.ParentID | PPObjSecur::maskUserGroup);
		}
	}
	else
		PPSetError(PPERR_BASEFILTUNSUPPORTED);
	return static_cast<PPBaseFilt *>(p_filt);
}

bool PPViewBill::IsTempTblNeeded() const
{
	if(State & stNoTempTbl)
		return false;
	else if((Filt.P_SjF && !Filt.P_SjF->IsEmpty()) || (Filt.P_TagF && !Filt.P_TagF->IsEmpty()) || IdList.IsExists() ||
		(Filt.PoolBillID && Filt.AssocID) || Filt.PayerID || Filt.AgentID || Filt.FreightPortOfDischarge ||
		(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) ||
		!Filt.PaymPeriod.IsZero() || Filt.SortOrder || Filt.Flags & BillFilt::fShowWoAgent || P_Arp || Filt.StatusID || Filt.GoodsGroupID ||
		(Filt.Bbt == bbtOrderBills && Filt.OrderFulfillmentStatus > 0) || ArFBlk.IsActual) { 
		// @v11.0.11 Filt.GoodsGroupID // @v11.1.8 OrderFulfillmentStatus // @v11.9.4 Filt.FreightPortOfDischarge // @v11.9.6 ArFBlk.IsActual
		return true;
	}
	else
		return false;
}

int PPViewBill::GetOpList(const BillFilt * pFilt, PPIDArray * pList, PPID * pSingleOpID) const
{
	int    ok = 1;
	const PPRights & r_orts = ObjRts;
	ASSIGN_PTR(pSingleOpID, 0);
	pList->freeAll();
	if(!pFilt->OpID || IsGenericOp(pFilt->OpID) > 0) {
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
			ot_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, 0L);
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
				ot_list.add(PPOPT_AGREEMENT);
				ot_list.add(PPOPT_DRAFTQUOTREQ);
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
					ot_list.add(PPOPT_DRAFTQUOTREQ);
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
	THROW(!(Filt.Flags & BillFilt::fCashOnly)     || (f & BILLF_CASH));
	THROW(!(Filt.Flags & BillFilt::fDebtOnly)     || !(f & BILLF_PAYOUT));
	THROW(!(Filt.Flags & BillFilt::fPaymNeeded)   || (f & BILLF_NEEDPAYMENT));
	THROW(!(Filt.Flags & BillFilt::fLabelOnly)    || (f & BILLF_WHITELABEL));
	THROW(!(Filt.Flags & BillFilt::fFreightedOnly) || (f & BILLF_FREIGHT));
	THROW(!(Filt.Flags & BillFilt::fUnshippedOnly) || !(f & BILLF_SHIPPED));
	THROW(!(Filt.Flags & BillFilt::fShippedOnly)  || (f & BILLF_SHIPPED));
	THROW(!(Filt.Flags & BillFilt::fDiscountOnly) || (f & BILLF_TOTALDISCOUNT));
	// @v10.7.0 THROW(!(Filt.Flags & BillFilt::fCcNotPrintedOnly) || !(f & BILLF_CHECK)); // @erik v10.6.13
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
	if(Filt.Ft_CheckPrintStatus > 0)
		{ THROW(f & BILLF_CHECK); }
	else if(Filt.Ft_CheckPrintStatus < 0)
		{ THROW(!(f & BILLF_CHECK)); }
    if(Filt.EdiRecadvStatus) {
		const int recadv_status = pRec ? BillCore::GetRecadvStatus(*pRec) : 0;
		if(Filt.EdiRecadvStatus == -1) {
			THROW(recadv_status == 0);
		}
		else
			THROW(recadv_status == static_cast<int>(Filt.EdiRecadvStatus));
    }
    if(Filt.EdiRecadvConfStatus) {
		const int recadv_conf_status = pRec ? BillCore::GetRecadvConfStatus(*pRec) : 0;
		if(Filt.EdiRecadvConfStatus == -1) {
			THROW(recadv_conf_status == 0);
		}
		THROW(recadv_conf_status == static_cast<int>(Filt.EdiRecadvConfStatus));
    }
	CATCHZOK
	return ok;
}

int PPViewBill::CheckIDForFilt(PPID id, const BillTbl::Rec * pRec)
{
	return Helper_CheckIDForFilt(0, id, pRec);
}

int PPViewBill::Helper_CheckIDForFilt(uint flags, PPID id, const BillTbl::Rec * pRec)
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
	// @v11.9.6 {
	if(ArFBlk.IsActual) {
		if(!pRec->Object)
			return 0;
		else {
			const PPID psn_id = ObjectToPerson(pRec->Object, 0);
			if(!psn_id || !ArFBlk.CheckID(psn_id))
				return 0;
		}
	}
	// } @v11.9.6 
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
	// @v11.9.4 {
	if(Filt.FreightPortOfDischarge) {
		PPFreight freight;
		if(P_BObj->FetchFreight(id, &freight) > 0 && freight.PortOfDischarge == Filt.FreightPortOfDischarge) {
			; // ok
		}
		else
			return 0;
	}
	// } @v11.9.4 
	// @v11.7.4 {
	if(!(flags & enfSkipExtssMemo)) {
		SString memo_pattern;
		if(Filt.GetExtStrData(BillFilt::extssMemoText, memo_pattern) > 0) {
			assert(memo_pattern.NotEmpty());
			SString memo;
			P_BObj->P_Tbl->GetItemMemo(id, memo);
			if(!memo.NotEmptyS())
				return 0;
			else if(!ExtStrSrch(memo, memo_pattern, 0))
				return 0;
		}
		else
			assert(memo_pattern.IsEmpty());
	}
	// } @v11.7.4 
	if(PPObjTag::CheckForTagFilt(PPOBJ_BILL, pRec->ID, Filt.P_TagF) <= 0)
		return 0;
	else if(Filt.PoolBillID && !IsMemberOfPool(id))
		return 0;
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempBill);

int PPViewBill::InitOrderRec(IterOrder ord, const BillTbl::Rec * pBillRec, TempOrderTbl::Rec * pOrdRec)
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
		// @v11.1.12 BillCore::GetCode(pOrdRec->Name);
		temp_buf = pOrdRec->Name;
	}
	else if(ord == OrdByDateCode) { // @v11.0.11
		// @v11.1.12 SString & r_code_buf = SLS.AcquireRvlStr();
		// @v11.1.12 BillCore::GetCode(r_code_buf = pBillRec->Code);
		temp_buf.Cat(pBillRec->Dt, DATF_YMD|DATF_NODIV|DATF_CENTURY).Cat(/*r_code_buf*/pBillRec->Code);
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

void PPViewBill::InitTempRec(const BillTbl::Rec * pBillRec, TempBillTbl::Rec * pTmpRec)
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

int PPViewBill::CalcDebtCardInSaldo(double * pSaldo)
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

int PPViewBill::EnumerateDebtCard(BillViewEnumProc proc, void * pExtraPtr)
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
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
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
					THROW(r = proc(&item, pExtraPtr));
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
				THROW(r = proc(&item, pExtraPtr));
				lookup_list.add(bill_rec.ID);
				if(r < 0)
					return -1;
			}
		{
			pool_list.clear();
			t->GetPoolMembersList(PPASS_PAYMBILLPOOL, bill_id, &pool_list);
			for(uint j = 0; j < pool_list.getCount(); j++) {
				const  PPID member_id = pool_list.get(j);
				if(!bill_id_list.bsearch(member_id) && !lookup_list.lsearch(bill_rec.ID))
					if(P_BObj->Search(member_id, &bill_rec) > 0 && Filt.Period.CheckDate(bill_rec.Dt)) {
						memcpy(&item, &bill_rec, sizeof(bill_rec));
						item.Debit  = 0;
						item.Credit = BR2(item.Amount);
						item.Saldo  = 0;
						THROW(r = proc(&item, pExtraPtr));
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

int PPViewBill::Helper_EnumProc(PPID billID, const BillTbl::Rec * pRec, int checkForFilt, uint flags, BillViewEnumProc proc, void * pExtraPtr)
{
	int    ok = 1;
	BillTbl::Rec rec;
	SETIFZ(pRec, ((P_BObj->Search(billID, &rec) > 0) ? &rec : 0));
	if(pRec && (!checkForFilt || Helper_CheckIDForFilt(flags, billID, pRec) > 0)) {
		BillViewItem item;
		memcpy(&item, pRec, sizeof(BillTbl::Rec));
		if(!Filt.PaymPeriod.IsZero()) {
			if(!P_BObj->P_Tbl->CalcPayment(item.ID, 1, &Filt.PaymPeriod, Filt.CurID, &item.Credit))
				return 0;
			if(item.Amount < 0.0)
				item.Credit = -item.Credit;
			item.Debit = item.Amount;
			item.Saldo = item.Debit - item.Credit;
		}
		int    r = proc(&item, pExtraPtr);
		if(!r)
			ok = 0;
		else if(r < 0)
			ok = -1;
	}
	return ok;
}
//
//
//
int PPViewBill::EvaluateOrderFulfillmentStatus(PPID billID)
{
	int    status = -1;
	BillTbl::Rec bill_rec;
	if(P_BObj->Fetch(billID, &bill_rec) > 0 && GetOpType(bill_rec.OpID) == PPOPT_GOODSORDER) {
		P_BObj->trfr->GetOrderFulfillmentStatus(billID, &status);
	}
	return status;
}

int PPViewBill::Enumerator(uint flags, BillViewEnumProc proc, void * pExtraPtr)
{
	int    ok = 1;
	int    r = 1;
	const PPConfig & r_cfg = LConfig;
	PersonTbl::Rec psn_rec; // @v11.1.9
	PPIDArray temp_list;
	const PPIDArray * p_list = 0;
	int    check_list_item_for_filt = 1;
	BillCore * t = P_BObj->P_Tbl;
	BExtQuery * q = 0;
	// @v11.7.4 {
	SString memo_pattern;
	SString memo_buf;
	if(!(flags & enfSkipExtssMemo))
		Filt.GetExtStrData(BillFilt::extssMemoText, memo_pattern); 
	// } @v11.7.4
	if(IdList.IsExists()) {
		p_list = &IdList.Get();
		check_list_item_for_filt = (Filt.Flags & BillFilt::fBillListOnly) ? 0 : 1;
	}
	else if(Filt.P_SjF && !Filt.P_SjF->IsEmpty())
		ok = -1;
	else if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)
		ok = EnumerateDebtCard(proc, pExtraPtr);
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
		for(q->initIteration(false, &k, spGt); ok > 0 && q->nextIteration() > 0;) {
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
				if(Filt.CliPsnCategoryID || ArFBlk.IsActual) {
					if(!bill_rec.Object)
						continue;
					else {
						const  PPID psn_id = ObjectToPerson(bill_rec.Object, 0);
						if(!psn_id)
							continue;
						else {
							// @v11.1.9 {
							if(Filt.CliPsnCategoryID && (!(PsnObj.Fetch(psn_id, &psn_rec) > 0 && psn_rec.CatID == Filt.CliPsnCategoryID)))
								continue;
							// } @v11.1.9 
							// @v11.9.6 {
							if(ArFBlk.IsActual && !ArFBlk.CheckID(psn_id))
								continue;
							// } @v11.9.6
						}
					}
				}
				// @v11.7.4 {
				if(!(flags & enfSkipExtssMemo) && memo_pattern.NotEmpty()) {
					P_BObj->P_Tbl->GetItemMemo(bill_rec.ID, memo_buf);
					if(!memo_buf.NotEmptyS() || !ExtStrSrch(memo_buf, memo_pattern, 0))
						continue;
				}
				// } @v11.7.4 
				// @v11.0.11 {
				if(Filt.GoodsGroupID && P_BObj->DoesContainGoods(bill_rec.ID, GoodsList) <= 0)
					continue;
				// } @v11.0.11
				// @v11.9.4 {
				if(Filt.FreightPortOfDischarge) {
					PPFreight freight;
					if(P_BObj->FetchFreight(bill_rec.ID, &freight) > 0 && freight.PortOfDischarge == Filt.FreightPortOfDischarge) {
						; // ok
					}
					else
						continue;
				}
				// } @v11.9.4
				// @v11.1.8 {
				if(Filt.Bbt == bbtOrderBills && oneof3(Filt.OrderFulfillmentStatus, 1, 2, 3)) {
					int ordffs = EvaluateOrderFulfillmentStatus(bill_rec.ID);
					if(ordffs != Filt.OrderFulfillmentStatus)
						continue;
				}
				// } @v11.1.8 
				THROW(ok = Helper_EnumProc(bill_rec.ID, &bill_rec, 0, flags, proc, pExtraPtr));
			}
		}
	}
	if(p_list) {
		for(uint i = 0; ok > 0 && i < p_list->getCount(); i++) {
			const PPID bill_id = p_list->get(i);
			if(!Filt.GoodsGroupID || P_BObj->DoesContainGoods(bill_id, GoodsList) > 0) // @v11.9.4 @fix (это условие не проверялось)
				THROW(ok = Helper_EnumProc(bill_id, 0, check_list_item_for_filt, flags, proc, pExtraPtr));
		}
	}
	CATCHZOK
	delete q;
	return ok;
}

struct IterProcParam_Total {
	IterProcParam_Total(PPObjBill * pBObj, const BillFilt * pFilt, int byCashCheck, BillTotal * pTotal) :
		P_BObj(pBObj), P_Filt(pFilt), CashChecks(byCashCheck), Data(pTotal)
	{
	}
	int    CashChecks;
	const  BillFilt * P_Filt;
	PPObjBill * P_BObj;
	BillTotal * Data;
};

static int IterProc_Total(const BillViewItem * pItem, void * pExtraPtr)
{
	IterProcParam_Total * p_param = static_cast<IterProcParam_Total *>(pExtraPtr);
	const  PPID id = pItem->ID;
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

int PPViewBill::CalcTotal(BillTotal * pTotal)
{
	int    ok = -1;
	if(pTotal) {
		pTotal->Z();
		IterProcParam_Total param(P_BObj, &Filt, BIN(Filt.Flags & BillFilt::fCashOnly), pTotal);
		ok = Enumerator(0, IterProc_Total, &param);
		if(ok && Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) {
			CalcDebtCardInSaldo(&pTotal->InSaldo);
			pTotal->OutSaldo = pTotal->InSaldo + pTotal->Debit - pTotal->Credit;
		}
	}
	return ok;
}

int PPViewBill::CalcItemTotal(PPID billID, BillTotalData & rTotal)
{
	PPBillPacket pack;
	return (billID && P_BObj->ExtractPacket(billID, &pack) > 0) ? pack.CalcTotal(rTotal, BTC_CALCSALESTAXES) : -1;
}

struct IterProcParam_CrTmpTbl {
	PPViewBill::IterOrder Ord;
	int    IsOrdTbl;
	double Saldo;
	BExtInsert * bei;
	PPViewBill * BV;
};

static int IterProc_CrTmpTbl(const BillViewItem * pItem, void * pExtraPtr)
{
	TempBillTbl::Rec  tbrec;
	TempOrderTbl::Rec torec;
	IterProcParam_CrTmpTbl * param = static_cast<IterProcParam_CrTmpTbl *>(pExtraPtr);
	if(param->IsOrdTbl) {
		param->BV->InitOrderRec(param->Ord, pItem, &torec);
		if(!param->bei->insert(&torec))
			return PPSetErrorDB();
	}
	else {
		param->BV->InitTempRec(pItem, &tbrec);
		const BillFilt * p_flt = static_cast<const BillFilt *>(param->BV->GetBaseFilt());
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

int PPViewBill::CreateTempTable(IterOrder ord, int * pIsOrdTbl)
{
	int    ok = 1;
	TempBillTbl  * btbl = 0;
	TempOrderTbl * otbl = 0;
	IterProcParam_CrTmpTbl param;
	MEMSZERO(param);
	param.Ord = ord;
	ZDELETE(P_TempOrd);
	ZDELETE(P_TempTbl);
	if(oneof4(ord, OrdByCode, OrdByObjectName, OrdByOpName, OrdByDateCode) || (ord && Filt.Flags & BillFilt::fDescOrder)) { // @v11.0.11 OrdByDateCode
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
	param.Saldo = 0.0;
	{
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		THROW(Enumerator(0, IterProc_CrTmpTbl, &param));
		THROW_DB(param.bei->flash());
		if(btbl && (Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments)) {
			param.Saldo = 0.0;
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

int PPViewBill::InitIteration(IterOrder ord)
{
	int    ok = 1;
	BtrDbKey key_;
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
		case OrdByDateCode:   idx = 1; use_ord_tbl = 1; break; // @v11.0.11
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
	P_IterQuery->initIteration(false, key_, spFirst);
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
			const  PPID id = p_temp_tbl ? p_temp_tbl->data.BillID : P_TempOrd->data.ID;
			if(P_BObj->Search(id, &br) > 0) {
				_IterC++;
				if(pItem) {
					memzero(pItem, sizeof(*pItem));
					*static_cast<BillTbl::Rec *>(pItem) = br;
					if(p_temp_tbl) {
						pItem->Debit  = p_temp_tbl->data.Debit;
						pItem->Credit = p_temp_tbl->data.Credit;
						pItem->Saldo  = p_temp_tbl->data.Saldo;
					}
					// @v11.1.12 {
					{
						SString & r_temp_buf = SLS.AcquireRvlStr();
						P_BObj->P_Tbl->GetItemMemo(id, r_temp_buf);
						STRNSCPY(pItem->SMemo, r_temp_buf);
					}
					// } @v11.1.12 
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

int PPViewBill::SetIterState(const void * pSt, size_t sz)
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

void * PPViewBill::GetIterState() { return P_IterState; }

const PPBillPacket * PPViewBill::GetIterCachedBillPack(PPID billID)
{
	const PPBillPacket * p_result = 0;
	if(!P_Dl600BPackCache) {
		if(billID) {
			P_Dl600BPackCache = new PPBillPacket;
			if(P_BObj->ExtractPacket(billID, P_Dl600BPackCache) > 0) {
				p_result = P_Dl600BPackCache;
			}
			else {
				ZDELETE(P_Dl600BPackCache);
			}
		}
	}
	else {
		if(P_Dl600BPackCache->Rec.ID == billID) {
			p_result = P_Dl600BPackCache;
		}
		else {
			P_Dl600BPackCache->destroy();
			if(P_BObj->ExtractPacket(billID, P_Dl600BPackCache) > 0) {
				p_result = P_Dl600BPackCache;
			}
			else {
				ZDELETE(P_Dl600BPackCache);
			}
		}
	}
	return p_result;
}
//
//
//
int PPViewBill::WriteOffDraft(PPID id)
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
			const  PPID op_id = bill_rec.OpID;
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
					const BillTbl::Rec & r_bill_rec = *static_cast<const BillTbl::Rec *>(bill_rec_list.at(i));
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
					if(goa_filt.AgentID == 0 && goa_filt.ObjectID == 0) {
						PPID   loc_id = 0;
						PPBillExt bext;
						if(P_BObj->P_Tbl->GetExtraData(r_bill_rec.ID, &bext) > 0 && bext.TradePlanLocID) {
							loc_id = bext.TradePlanLocID;
						}
						else
							loc_id = r_bill_rec.LocID;
						goa_filt.LocList.Add(loc_id);
					}
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
				PPWaitStart();
				THROW(GetBillIDList(&idlist));
				{
					PPTransaction tra(1);
					THROW(tra);
					for(uint i = 0; i < idlist.getCount(); i++) {
						const  PPID bill_id = idlist.at(i);
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
				PPWaitStop();
			}
			else if(s == 2) { // Перенести теги с драфт-документа на документ списания
				THROW(P_BObj->MoveLotTagsFromDraftBillToWrOffBill(id, 0, 1));
			}
			else if(s == 3) { // Перенести теги со строк всех драфт-документов на документы списания
				PPLogger logger;
				PPIDArray idlist;
				PPWaitStart();
				THROW(GetBillIDList(&idlist));
				for(uint i = 0; i < idlist.getCount(); i++) {
					const  PPID bill_id = idlist.at(i);
					P_BObj->MoveLotTagsFromDraftBillToWrOffBill(bill_id, 0, 1);
				}
				PPWaitStop();
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

static int EditBill2MrpParam(Bill2MrpParam * pParam)
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

int PPViewBill::CreateMrpTab(PPID billID)
{
	int    ok = -1;
	Bill2MrpParam param;
	MEMSZERO(param);
	if((Filt.Flags & (BillFilt::fOrderOnly | BillFilt::fDraftOnly)) && EditBill2MrpParam(&param) > 0) {
		PPObjMrpTab mrp_obj;
		MrpTabPacket mrp_pack;

		PPIDArray bill_list;
		PPWaitStart();
		if(param.Flags & Bill2MrpParam::fAllSelection) {
			THROW(GetBillIDList(&bill_list));
			mrp_pack.Init(PPOBJ_BILL, 0, param.MrpName);
		}
		else {
			bill_list.add(billID);
			mrp_pack.Init(PPOBJ_BILL, billID, param.MrpName);
		}
		THROW(P_BObj->CreateMrpTab(&bill_list, &mrp_pack, 0, 1));
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::EditFilt(BillFilt * pFilt, long extraParam) const
{
	const  BrowseBillsType bbt = static_cast<BrowseBillsType>(extraParam);
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
		else if(!r)
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
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewBill * p_view = static_cast<PPViewBill *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewBill::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			BillTbl::Rec bill_rec;
			const PPViewBill::BrwHdr * p_hdr = static_cast<const PPViewBill::BrwHdr *>(pData);
			if(p_hdr->ID) {
				if(r_col.OrgOffs == 0) { // ID
					if(PPMaster && P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0 && bill_rec.Flags2 & BILLF2_FULLSYNC)
						ok = pStyle->SetLeftBottomCornerColor(GetColorRef(SClrDodgerblue));
				}
				else if(r_col.OrgOffs == 2) { // BillNo
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0) {
						const TagFilt & r_tag_filt = P_BObj->GetConfig().TagIndFilt;
						if(!r_tag_filt.IsEmpty()) {
							SColor clr;
							if(r_tag_filt.SelectIndicator(p_hdr->ID, clr))
								ok = pStyle->SetLeftBottomCornerColor(static_cast<COLORREF>(clr));
						}
						if(P_BObj->GetConfig().Flags & BCF_PAINTSHIPPEDBILLS) {
							if(bill_rec.Flags & BILLF_SHIPPED)
								ok = pStyle->SetFullCellColor(LightenColor(GetColorRef(SClrBlue), 0.7f));
						}
						if(bill_rec.Flags2 & BILLF2_BHT)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrLime));
						if(bill_rec.Flags & BILLF_WHITELABEL) // @v11.1.12
							ok = pStyle->SetRightFigTriangleColor(SClrHotpink);
					}
				}
				else if(r_col.OrgOffs == 4) { // Memo
					SString & r_memos = SLS.AcquireRvlStr();
					if(P_BObj->FetchExtMemo(p_hdr->ID, r_memos) > 0)
						ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrDarkgreen));
					// @v11.2.3 {
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0 && bill_rec.Flags2 & BILLF2_FORCEDRECEIPT) { // @v11.2.12 @fix Fetch
						if(pStyle->SetRightFigCircleColor(GetColorRef(SClrHotpink)) > 0) {
							ok = 1;
						}
					}
					// } @v11.2.3 
				}
				else if(r_col.OrgOffs == 6) { // Contragent
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0 && bill_rec.Object) {
						ArticleTbl::Rec ar_rec;
						if(ArObj.Fetch(bill_rec.Object, &ar_rec) > 0 && ar_rec.Flags & ARTRF_STOPBILL)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrCoral));
					}
				}
				else if(r_col.OrgOffs == 10) { // Status
					if(P_BObj->Fetch(p_hdr->ID, &bill_rec) > 0) {
						const  PPID op_type_id = GetOpType(bill_rec.OpID);
						if(oneof4(op_type_id, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ)) {
							if(bill_rec.Flags2 & BILLF2_DECLINED)
								ok = pStyle->SetLeftBottomCornerColor(GetColorRef(SClrGrey));
							else if(bill_rec.Flags & BILLF_WRITEDOFF)
								ok = pStyle->SetLeftBottomCornerColor(GetColorRef(SClrOrange));
						}
						else if(op_type_id == PPOPT_GOODSORDER) {
							if(bill_rec.Flags2 & BILLF2_DECLINED) // @v12.1.6
								ok = pStyle->SetLeftBottomCornerColor(GetColorRef(SClrGrey));
							else if(bill_rec.Flags & BILLF_CLOSEDORDER)
								ok = pStyle->SetLeftBottomCornerColor(GetColorRef(SClrOrange));
						}
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
								ok = pStyle->SetRightFigCircleColor(edi_color);
							}
						}
						if(bill_rec.StatusID) {
							PPBillStatus bs_rec;
							PPObjBillStatus bs_obj;
							if(bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0 && !bs_rec.IndColor.IsEmpty())
								ok = pStyle->SetFullCellColor(bs_rec.IndColor);
						}
					}
				}
			}
		}
	}
	return ok;
}

/*virtual*/void PPViewBill::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int    caption = 0;
	SString title, sub_title, temp_buf;
	if(pBrw) {
		const BrowserDef * p_def = pBrw->getDef();
		const  PPID single_loc_id = LocList_.getSingle();
		const int  show_debt = (Filt.Flags & BillFilt::fOrderOnly) ? BIN(Filt.Flags & BillFilt::fShowDebt) : BIN(Filt.Flags & (BillFilt::fShowDebt|BillFilt::fDebtOnly));
		pBrw->ViewOptions |= (ofCenterX | ofCenterY);
		if(Filt.Flags & BillFilt::fCashOnly) {
			const PPConfig & r_cfg = LConfig;
			CatObjectName(PPOBJ_CASHNODE, r_cfg.Cash, sub_title);
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
					pBrw->InsColumn(loc_col, "@warehouse", 7, 0, MKSFMT(10, 0), 0);
					debt_col++;
					bs_col++;
				}
				{
					PPObjBillStatus bs_obj;
					PPID   bs_id = 0;
					if(bs_obj.EnumItems(&bs_id, 0) > 0) {
						//
						// Определен по крайней мере один статус документа, следовательно в таблице нужна колонка статуса документа
						//
						pBrw->InsColumn(bs_col, "@status", 10, 0, MKSFMT(6, 0), 0);
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
				if(Filt.Dl.GetItemByDataId(BillFilt::dliDueDate, 0)) // #13
					pBrw->InsColumn(de_col++, "@duedate", 13, 0, MKSFMT(0, DATF_DMY), 0);
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightIssueDate, 0)) // #11
					pBrw->InsColumn(de_col++, "@issuedate", 11, 0, MKSFMT(0, DATF_DMY), 0);
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightArrivalDate, 0)) // #12
					pBrw->InsColumn(de_col++, "@arrivaldate", 12, 0, MKSFMT(0, DATF_DMY), 0);
				//
			}
			if(p_def) {
				int    next_pos = -1;
				constexpr int org_offs_ar = 6;
				constexpr int org_offs_memo = 4;
				constexpr int org_offs_amt = 3; // @v12.1.0
				int    ar_pos = -1;
				int    memo_pos = -1;
				int    amt_pos = -1;
				for(uint i = 0; i < p_def->getCount(); i++) {
					switch(p_def->at(i).OrgOffs) {
						case org_offs_ar: ar_pos = i; break;
						case org_offs_memo: memo_pos = i; break;
						case org_offs_amt: amt_pos = i; break;
					}
				}
				// @v12.1.0 {
				if(amt_pos >= 0) {
					int    amt_next_pos = amt_pos;
					if(Filt.Dl.GetItemByDataId(BillFilt::dliStdAmtCost, 0)) {
						pBrw->InsColumn(amt_next_pos++, "@sumcost", 19, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
					}
					if(Filt.Dl.GetItemByDataId(BillFilt::dliStdAmtPrice, 0)) {
						pBrw->InsColumn(amt_next_pos++, "@sumprice", 20, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
					}
				}
				// } @v12.1.0 
				if(ar_pos >= 0)
					next_pos = ar_pos+1;
				else if(memo_pos >= 0)
					next_pos = memo_pos;
				if(next_pos >= 0) {
					if(Filt.Dl.GetItemByDataId(BillFilt::dliAgentName, 0)) // #14
						pBrw->InsColumn(next_pos++, "@agent", 14, 0, 0, 0);
					if(Filt.Dl.GetItemByDataId(BillFilt::dliDlvrAddr, 0)) // #16
						pBrw->InsColumn(next_pos++, "@daddress", 16, 0, 0, 0);
					if(Filt.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0) && P_Arp) // #15
						pBrw->InsColumn(next_pos++, "@alcolic", 15, 0, 0, 0);
					// @v11.6.12 {
					if(Filt.Dl.GetItemByDataId(BillFilt::dliTSessLinkTo, 0)) // #18
						pBrw->InsColumn(next_pos++, "@billfilt_dlitsesslinkto", 18, 0, 0, 0);
					// } @v11.6.12 
				}
			}
		}
		pBrw->Advise(PPAdviseBlock::evBillChanged, 0, PPOBJ_BILL, 0);
	}
	SetExtToolbar(0);
	if(Filt.Flags & BillFilt::fAsSelector) {
		PPGetWord(PPWORD_SELBILL, 0, sub_title);
		if(Filt.Sel)
			pBrw->search2(&Filt.Sel, CMPF_LONG, srchFirst, 0);
	}
	else if(Filt.Sel) {
		if(CheckIDForFilt(Filt.Sel, 0))
			pBrw->search2(&Filt.Sel, CMPF_LONG, srchFirst, 0);
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

DBQuery * PPViewBill::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	const int  use_omt_paymamt = BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	const PPID single_loc_id = LocList_.getSingle();
	BillTbl  * bll  = 0;
	TempBillTbl   * bllt = 0;
	TempOrderTbl  * ordt = 0;
	BillAmountTbl * t_amt  = 0;
	DBQuery  * q    = 0;
	DBQ  * dbq  = 0;
	DBE    dbe_debt;
	DBE    dbe_status;
	DBE    dbe_oprkind;
	DBE    dbe_loc;
	DBE    dbe_ar;
	DBE    dbe_cur;
	DBE    dbe_chkusr;
	DBE    dbe_chkpsncat; // @v11.1.9
	DBE    dbe_chkmemosubstr; // @v11.7.4
	DBE    dbe_issuedate;
	DBE    dbe_arrvldate;
	DBE    dbe_agentname;
	DBE    dbe_licreg;
	DBE    dbe_dlvraddr;
	DBE    dbe_strgloc;
	DBE    dbe_bill_memo; // @v11.1.12
	DBE    dbe_tsess_linkto; // @v11.6.12
	DBE    dbe_amt_cost; // @v12.1.0
	DBE    dbe_amt_price; // @v12.1.0
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
	if(P_TempOrd && oneof3(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject, BillFilt::ordByDateCode)) { // @v11.0.11 BillFilt::ordByDateCode
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
		PPDbqFuncPool::InitObjNameFunc(dbe_bill_memo, PPDbqFuncPool::IdObjMemoBill, bll->ID);
		if(Filt.ObjectID && Filt.Flags & BillFilt::fDebtsWithPayments) {
			q = &select(
				bllt->BillID, // #0
				bllt->Dt,     // #1
				bll->Code,    // #2
				dbe_oprkind,  // #3
				bllt->Debit,  // #4
				bllt->Credit, // #5
				// @v11.1.12 bll->Memo,    // #6
				dbe_bill_memo, // #6 @v11.1.12
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
				dbe_debt.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillDebt));
			}
			else
				dbe_debt.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
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
			// @v11.1.12 q->addField(bll->Memo);   // #4
			q->addField(dbe_bill_memo); // #4 // @v11.1.12
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
					dbe_issuedate.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillFrghtIssueDt));
				}
				else {
					dbe_issuedate.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				}
				q->addField(dbe_issuedate); // #11
				if(Filt.Dl.GetItemByDataId(BillFilt::dliFreightArrivalDate, 0)) {
					dbe_arrvldate.push(bll->ID);
					dbe_arrvldate.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillFrghtArrvlDt));
				}
				else {
					dbe_arrvldate.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				}
				q->addField(dbe_arrvldate); // #12
			}
			q->addField(bll->DueDate);      // #13
			{
				dbe_agentname.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliAgentName, 0)) {
					dbe_agentname.push(bll->ID);
					dbe_agentname.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillAgentName));
				}
				else
					dbe_agentname.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				q->addField(dbe_agentname); // #14
			}
			{
				dbe_licreg.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliAlcoLic, 0) && P_Arp && bllt) {
					dbe_licreg.push(bllt->LicRegID);
					dbe_licreg.push(static_cast<DBFunc>(PPDbqFuncPool::IdRegisterText));
				}
				else
					dbe_licreg.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				q->addField(dbe_licreg); // #15
			}
			{
				dbe_dlvraddr.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliDlvrAddr, 0)) {
					dbe_dlvraddr.push(bll->ID);
					dbe_dlvraddr.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillFrghtDlvrAddr));
				}
				else
					dbe_dlvraddr.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				q->addField(dbe_dlvraddr); // #16
			}
			{
				dbe_strgloc.init();
				if(Filt.Bbt == bbtInventoryBills) {
					dbe_strgloc.push(bll->ID);
					dbe_strgloc.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillFrghtStrgLoc));
				}
				else
					dbe_strgloc.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				q->addField(dbe_strgloc); // #17
			}
			// @v11.6.12 {
			{
				dbe_tsess_linkto.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliTSessLinkTo, 0)) {
					dbe_tsess_linkto.push(bll->ID);
					dbe_tsess_linkto.push(static_cast<DBFunc>(PPDbqFuncPool::IdTSessBillLinkTo_Text));
				}
				else
					dbe_tsess_linkto.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				q->addField(dbe_tsess_linkto); // #18
			}
			// } @v11.6.12
			// @v12.1.0 {
			{
				dbe_amt_cost.init();
				dbe_amt_price.init();
				if(Filt.Dl.GetItemByDataId(BillFilt::dliStdAmtCost, 0)) {
					dbe_amt_cost.push(bll->ID);
					dbe_amt_cost.push(dbconst(PPAMT_BUYING));
					dbe_amt_cost.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillAmount));
				}
				else {
					dbe_amt_cost.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				}
				if(Filt.Dl.GetItemByDataId(BillFilt::dliStdAmtPrice, 0)) {
					dbe_amt_price.push(bll->ID);
					dbe_amt_price.push(dbconst(PPAMT_SELLING));
					dbe_amt_price.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillAmount));
				}
				else {
					dbe_amt_price.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
				}
				q->addField(dbe_amt_cost); // #19
				q->addField(dbe_amt_price); // #20
			}
			// } @v12.1.0
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
			dbq = ppcheckflag(dbq, bll->Flags, BILLF_CHECK, Filt.Ft_CheckPrintStatus);
			{
				DBE * p_dbe_1 = 0;
				DBE * p_dbe_2 = 0;
				if(Filt.EdiRecadvStatus) {
					p_dbe_1 = &(bll->Flags2 & BILLF2_EDI_ACCP);
					p_dbe_2 = &(bll->Flags2 & BILLF2_EDI_DECL);
					switch(Filt.EdiRecadvStatus) {
						case PPEDI_RECADV_STATUS_ACCEPT:     dbq = &(*dbq && *p_dbe_1 == BILLF2_EDI_ACCP && *p_dbe_2 == 0L); break;
						case PPEDI_RECADV_STATUS_PARTACCEPT: dbq = &(*dbq && *p_dbe_1 == BILLF2_EDI_ACCP && *p_dbe_2 == BILLF2_EDI_DECL); break;
						case PPEDI_RECADV_STATUS_REJECT:     dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == BILLF2_EDI_DECL); break;
						case -1:                             dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == 0L); break;
					}
				}
				if(Filt.EdiRecadvConfStatus) {
					p_dbe_1 = &(bll->Flags2 & BILLF2_EDIAR_AGR);
					p_dbe_2 = &(bll->Flags2 & BILLF2_EDIAR_DISAGR);
					switch(Filt.EdiRecadvConfStatus) {
						case PPEDI_RECADVCONF_STATUS_ACCEPT: dbq = &(*dbq && *p_dbe_1 == BILLF2_EDIAR_AGR && *p_dbe_2 == 0L); break;
						case PPEDI_RECADVCONF_STATUS_REJECT: dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == BILLF2_EDIAR_DISAGR); break;
						case -1:                             dbq = &(*dbq && *p_dbe_1 == 0L && *p_dbe_2 == 0L); break;
					}
				}
				delete p_dbe_1;
				delete p_dbe_2;
			}
			if(Filt.CreatorID & PPObjSecur::maskUserGroup) {
				dbe_chkusr.init();
				dbe_chkusr.push(bll->UserID);
				DBConst dbc_long;
				dbc_long.init(Filt.CreatorID);
				dbe_chkusr.push(dbc_long);
				dbe_chkusr.push(static_cast<DBFunc>(PPDbqFuncPool::IdCheckUserID));
				dbq = & (*dbq && dbe_chkusr == 1L);
			}
			else
				dbq = ppcheckfiltid(dbq, bll->UserID, Filt.CreatorID);
			// @v11.1.9 {
			if(Filt.CliPsnCategoryID) {
				dbe_chkpsncat.init();
				dbe_chkpsncat.push(bll->Object);
				DBConst dbc_long;
				dbc_long.init(Filt.CliPsnCategoryID);
				dbe_chkpsncat.push(dbc_long);
				dbe_chkpsncat.push(static_cast<DBFunc>(PPDbqFuncPool::IdArIsCatPerson));
				dbq = & (*dbq && dbe_chkpsncat == 1L);
			}
			// } @v11.1.9 
			// @v11.7.4 {
			{
				SString memo_pattern;
				Filt.GetExtStrData(BillFilt::extssMemoText, memo_pattern);
				if(memo_pattern.NotEmptyS()) {
					dbe_chkmemosubstr.init();
					dbe_chkmemosubstr.push(bll->ID);
					dbe_chkmemosubstr.push(dbconst(memo_pattern));
					dbe_chkmemosubstr.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillMemoSubStr));
					dbq = & (*dbq && dbe_chkmemosubstr == 1L);
				}
			}
			// } @v11.7.4 
			if(!IdList.IsExists()) {
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_NEEDPAYMENT,   BIN(Filt.Flags & BillFilt::fPaymNeeded));
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_WHITELABEL,    BIN(Filt.Flags & BillFilt::fLabelOnly));
				dbq = ppcheckflag(dbq, bll->Flags, BILLF_TOTALDISCOUNT, BIN(Filt.Flags & BillFilt::fDiscountOnly));
				// @v10.7.0 dbq = ppcheckflag(dbq, bll->Flags, BILLF_CHECK, (Filt.Flags & BillFilt::fCcNotPrintedOnly) ? -1 : BIN(Filt.Flags & BillFilt::fCcPrintedOnly)); // @erik v10.6.13
			}
			q->where(*dbq);
			//
			// } Restrictions
			//
			if(ordt && oneof3(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject, BillFilt::ordByDateCode)) // @v11.0.11 BillFilt::ordByDateCode
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
int PPViewBill::SendAddBillMessage(PPID id)
{
	if(id && P_BObj->Cfg.Flags & BCF_WARNADDBILLNOFLT)
		return (CheckIDForFilt(id, 0) > 0) ? -1 : (PPMessage(mfInfo|mfOK, PPINF_ADDBILLNOSHOWBYFLT), 1);
	else
		return -1;
}

int PPViewBill::AddItem(PPID * pID, PPID opID)
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
						op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_AGREEMENT, 0L);
					else if(Filt.Flags & BillFilt::fOrderOnly)
						op_type_list.add(PPOPT_GOODSORDER);
					else if(Filt.Flags & BillFilt::fInvOnly)
						op_type_list.add(PPOPT_INVENTORY);
					else if(Filt.Flags & BillFilt::fPoolOnly)
						op_type_list.add(PPOPT_POOL);
					else if(Filt.Flags & BillFilt::fDraftOnly)
						op_type_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, 0L);
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

static int SelectAddByRcptAction(SelAddBySampleParam * pData)
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
		op_type_list.addzlist(PPOPT_GOODSEXPEND, PPOPT_GOODSRECEIPT, PPOPT_DRAFTEXPEND, 0L);
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

static int SelectAddByDraftAction(SelAddBySampleParam * pData, const BillTbl::Rec & rSrcBillRec) // @v11.0.2
{
	class SelAddByDraftDialog : public TDialog {
		DECL_DIALOG_DATA(SelAddBySampleParam);
	public:
		SelAddByDraftDialog(const BillTbl::Rec & rSrcBillRec) : TDialog(DLG_SELDBSMPL), R_SrcBillRec(rSrcBillRec)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 0, SelAddBySampleParam::acnStd);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 1, SelAddBySampleParam::acnDraftExpByDraftRcpt);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 2, SelAddBySampleParam::acnDraftRcptByDraftExp);
			SetClusterData(CTL_SELBBSMPL_WHAT, Data.Action);
			DisableClusterItem(CTL_SELBBSMPL_WHAT, 1, GetOpType(R_SrcBillRec.OpID) == PPOPT_DRAFTEXPEND);
			DisableClusterItem(CTL_SELBBSMPL_WHAT, 2, GetOpType(R_SrcBillRec.OpID) == PPOPT_DRAFTRECEIPT);
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0, 0);
			SetupOpCombo();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			getCtrlData(CTLSEL_SELBBSMPL_LOC, &Data.LocID);
			getCtrlData(CTLSEL_SELBBSMPL_OP, &Data.OpID);
			if(oneof2(Data.Action, SelAddBySampleParam::acnDraftExpByDraftRcpt, SelAddBySampleParam::acnDraftRcptByDraftExp)) {
				if(Data.OpID == 0)
					ok = PPErrorByDialog(this, CTLSEL_SELBBSMPL_OP, PPERR_OPRKINDNEEDED);
				else if(Data.LocID == 0)
					ok = PPErrorByDialog(this, CTLSEL_SELBBSMPL_LOC, PPERR_LOCNEEDED);
			}
			if(ok) {
				ASSIGN_PTR(pData, Data);
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SELBBSMPL_WHAT)) {
				SetupOpCombo();
				clearEvent(event);
			}
		}
		void   SetupOpCombo()
		{
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			PPIDArray op_list;
			PPOprKind op_rec;
			if(Data.Action == Data.acnDraftExpByDraftRcpt) {
				for(PPID id = 0; EnumOperations(PPOPT_DRAFTEXPEND, &id, &op_rec) > 0;)
					op_list.add(id);
			}
			else if(Data.Action == Data.acnDraftRcptByDraftExp) {
				for(PPID id = 0; EnumOperations(PPOPT_DRAFTRECEIPT, &id, &op_rec) > 0;)
					op_list.add(id);
			}
			SetupOprKindCombo(this, CTLSEL_SELBBSMPL_OP, op_list.getSingle(), 0, &op_list, OPKLF_OPLIST);
		}
		const BillTbl::Rec & R_SrcBillRec;
	};
	DIALOG_PROC_BODY_P1(SelAddByDraftDialog, rSrcBillRec, pData);
}

static int SelectAddByOrderAction(SelAddBySampleParam * pData, int allowBulkMode)
{
	static const char * WrParam_StoreFlags = "SelectAddBillBySampleFlags";

	class SelAddByOrdDialog : public TDialog {
		DECL_DIALOG_DATA(SelAddBySampleParam);
	public:
		SelAddByOrdDialog(bool allowBulkMode) : TDialog(DLG_SELOBSMPL), AllowBulkMode(allowBulkMode)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 0, SelAddBySampleParam::acnStd);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 1, SelAddBySampleParam::acnShipmByOrder);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 2, SelAddBySampleParam::acnDraftExpByOrder);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 3, SelAddBySampleParam::acnDraftExpRestByOrder);
			AddClusterAssoc(CTL_SELBBSMPL_WHAT, 4, SelAddBySampleParam::acnDraftRcpByOrder);
			SetClusterData(CTL_SELBBSMPL_WHAT, Data.Action);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 0, SelAddBySampleParam::fCopyBillCode);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 1, SelAddBySampleParam::fNonInteractive);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 2, SelAddBySampleParam::fAll);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 3, SelAddBySampleParam::fRcptAllOnShipm);
			AddClusterAssoc(CTL_SELBBSMPL_FLAGS, 4, SelAddBySampleParam::fRcptDfctOnShipm); // @v12.0.7
			SetClusterData(CTL_SELBBSMPL_FLAGS, Data.Flags);
			DisableClusterItem(CTL_SELBBSMPL_FLAGS, 2, !AllowBulkMode || Data.Action == SelAddBySampleParam::acnStd);
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			SetupPPObjCombo(this, CTLSEL_SELBBSMPL_QK, PPOBJ_QUOTKIND, Data.QuotKindID, 0, 0);
			setCtrlDate(CTL_SELBBSMPL_DT, Data.Dt);
			SetupOpCombo();
			restoreFlags();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			GetClusterData(CTL_SELBBSMPL_SAMECODE, &Data.Flags);
			getCtrlData(CTLSEL_SELBBSMPL_LOC, &Data.LocID);
			getCtrlData(CTLSEL_SELBBSMPL_OP, &Data.OpID);
			getCtrlData(CTLSEL_SELBBSMPL_QK, &Data.QuotKindID);
			Data.Dt = getCtrlDate(CTL_SELBBSMPL_DT);
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
				SetupOpCombo();
				DisableClusterItem(CTL_SELBBSMPL_FLAGS, 2, !AllowBulkMode || Data.Action == SelAddBySampleParam::acnStd);
				DisableClusterItem(CTL_SELBBSMPL_FLAGS, 3, !(Data.Action == SelAddBySampleParam::acnShipmByOrder));
				DisableClusterItem(CTL_SELBBSMPL_FLAGS, 4, !(Data.Action == SelAddBySampleParam::acnShipmByOrder)); // @v12.0.7
				process_reg = 1;
				clearEvent(event);
			}
			if(event.isClusterClk(CTL_SELBBSMPL_FLAGS)) { // @v12.0.7
				const long preserve_flags = Data.Flags;
				GetClusterData(CTL_SELBBSMPL_FLAGS, &Data.Flags);
				if((Data.Flags & SelAddBySampleParam::fRcptAllOnShipm) && !(preserve_flags & SelAddBySampleParam::fRcptAllOnShipm))
					Data.Flags &= ~SelAddBySampleParam::fRcptDfctOnShipm;
				else if((Data.Flags & SelAddBySampleParam::fRcptDfctOnShipm) && !(preserve_flags & SelAddBySampleParam::fRcptDfctOnShipm))
					Data.Flags &= ~SelAddBySampleParam::fRcptAllOnShipm;
				SetClusterData(CTL_SELBBSMPL_FLAGS, Data.Flags);
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
			WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 0);
			SString param, val;
			long   flags = 0;
			GetClusterData(CTL_SELBBSMPL_SAMECODE, &flags);
			(param = WrParam_StoreFlags).CatChar('-').Cat(Data.OpID).CatChar('-').Cat(Data.Action);
			val.Z().Cat(flags & (SelAddBySampleParam::fCopyBillCode|SelAddBySampleParam::fRcptAllOnShipm|SelAddBySampleParam::fRcptDfctOnShipm));
			reg_key.PutString(param, val);
		}
		void   restoreFlags()
		{
			WinRegKey reg_key(HKEY_CURRENT_USER, PPConst::WrKey_PrefSettings, 1);
			SString param;
			SString temp_buf;
			const  PPID op_id = getCtrlLong(CTLSEL_SELBBSMPL_OP);
			long   action = 0;
			GetClusterData(CTL_SELBBSMPL_WHAT, &action);
			(param = WrParam_StoreFlags).CatChar('-').Cat(op_id).CatChar('-').Cat(action);
			if(reg_key.GetString(param, temp_buf)) {
				const long rf = satoi(temp_buf);
				SETFLAGBYSAMPLE(Data.Flags, SelAddBySampleParam::fCopyBillCode, rf);
				SETFLAGBYSAMPLE(Data.Flags, SelAddBySampleParam::fRcptAllOnShipm, rf);
				SETFLAGBYSAMPLE(Data.Flags, SelAddBySampleParam::fRcptDfctOnShipm, rf); // @v12.0.7
			}
			SetClusterData(CTL_SELBBSMPL_SAMECODE, Data.Flags);
		}
		void   SetupOpCombo()
		{
			GetClusterData(CTL_SELBBSMPL_WHAT, &Data.Action);
			PPIDArray op_list;
			PPOprKind op_rec;
			SString hint_buf; // @v11.9.12
			if(Data.Action == Data.acnShipmByOrder) {
				// @v11.9.12 {
				if(checkdate(Data.SampleBillRec.Dt) && Data.SampleBillRec.Dt > getcurdate_()) {
					PPLoadText(PPTXT_CREATEBILL_HINT_ORDDTGTNOW, hint_buf);
				}
				// } @v11.9.12 
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
			setStaticText(CTL_SELBBSMPL_INFO, hint_buf); // @v11.9.12
		}
		const bool AllowBulkMode;
	};
	DIALOG_PROC_BODY_P1(SelAddByOrdDialog, allowBulkMode, pData);
}

int PPViewBill::AddItemBySample(PPID * pID, PPID sampleBillID)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	if(sampleBillID && P_BObj->Search(sampleBillID, &bill_rec) > 0) {
		PPID   bill_id = 0;
		const  PPID op_type_id = GetOpType(bill_rec.OpID);
		if(Filt.DenyFlags & BillFilt::fDenyAdd)
			ok = -1;
		else if(Filt.Flags & BillFilt::fAccturnOnly && op_type_id != PPOPT_AGREEMENT)
			ok = P_BObj->AddAccturnBySample(&bill_id, sampleBillID);
		else {
			SelAddBySampleParam param;
			param.SampleBillRec = bill_rec;
			param.Action = param.acnUndef;
			param.LocID = bill_rec.LocID;
			bool   allow_bulk_mode = false;
			switch(op_type_id) {
				case PPOPT_GOODSORDER:
					if(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1) && Filt.OpID && GetOpType(Filt.OpID) == op_type_id)
						allow_bulk_mode = true;
					SelectAddByOrderAction(&param, allow_bulk_mode);
					break;
				case PPOPT_GOODSRECEIPT:
				case PPOPT_GOODSMODIF: SelectAddByRcptAction(&param); break;
				case PPOPT_DRAFTEXPEND:
				case PPOPT_DRAFTRECEIPT: SelectAddByDraftAction(&param, bill_rec); break;
				default: param.Action = SelAddBySampleParam::acnStd; break;
			}
			switch(param.Action) {
				case SelAddBySampleParam::acnStd:
					{
						PPObjBill::AddBlock ab;
						ab.SampleBillID = sampleBillID;
						ok = P_BObj->AddGoodsBill(&bill_id, &ab);
					}
					break;
				case SelAddBySampleParam::acnDraftExpByDraftRcpt:
					{
						assert(op_type_id == PPOPT_DRAFTRECEIPT);
						if(op_type_id == PPOPT_DRAFTRECEIPT) {
							ok = P_BObj->AddDraftBySample(&bill_id, sampleBillID, &param);
						}
					}
					break;
				case SelAddBySampleParam::acnDraftRcptByDraftExp: // @v11.0.2
					{
						assert(op_type_id == PPOPT_DRAFTEXPEND);
						if(op_type_id == PPOPT_DRAFTEXPEND) {
							ok = P_BObj->AddDraftBySample(&bill_id, sampleBillID, &param);
						}
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
								PPWaitStart();
								for(uint i = 0; i < bill_id_list.getCount(); i++) {
									const  PPID sample_bill_id = bill_id_list.get(i);
									const int  local_result = P_BObj->AddExpendByOrder(&bill_id, sample_bill_id, &param);
									if(!local_result)
										logger.LogLastError();
									else if(local_result == cmOK) {
										logger.LogAcceptMsg(PPOBJ_BILL, bill_id, 0);
										ok = cmOK;
									}
									PPWaitPercent(i+1, bill_id_list.getCount());
								}
								PPWaitStop();
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
								PPWaitStart();
								for(uint i = 0; i < bill_id_list.getCount(); i++) {
									const  PPID sample_bill_id = bill_id_list.get(i);
									const int  local_result = P_BObj->AddDraftBySample(&bill_id, sample_bill_id, &param);
									if(!local_result)
										logger.LogLastError();
									else if(local_result == cmOK) {
										logger.LogAcceptMsg(PPOBJ_BILL, bill_id, 0);
										ok = cmOK;
									}
									PPWaitPercent(i+1, bill_id_list.getCount());
								}
								PPWaitStop();
							}
							else
								ok = cmCancel;
						}
						else {
							ok = P_BObj->AddDraftBySample(&bill_id, sampleBillID, &param);
						}
					}
					else
						ok = -1;
					break;
				case SelAddBySampleParam::acnShipmAll:
					if(oneof3(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSMODIF, PPOPT_DRAFTEXPEND))
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

int PPViewBill::EditItem(PPID billID)
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
static int ConfirmRmvDraft(int all)
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

int PPViewBill::DeleteBillFromPool(PPID billID)
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
					const  PPID bill_id = bill_list.get(--i);
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

int PPViewBill::DeleteItem(PPID billID)
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
					PPWaitStart();
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
					PPWaitStop();
					ok = 1;
				}
			}
			else if(CONFIRMCRIT(PPCFM_RMVALLBILL)) {
				PPWaitStart();
				THROW(GetBillIDList(&id_list));
				if(id_list.getCount()) {
					for(i = id_list.getCount()-1; i >= 0; i--) {
		   		    	P_BObj->RemoveObjV(id_list.at(i), 0, PPObject::use_transaction, 0);
						PPWaitPercent(id_list.getCount()-i, id_list.getCount());
					}
					ok = 1;
				}
				PPWaitStop();
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::ViewPayments(PPID billID, int kind)
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
		else if(GetOpType(bill_rec.OpID) == PPOPT_GOODSORDER) { // @v12.0.11
			ok = P_BObj->ViewPayments(billID, LinkedBillFilt::lkOrdAccomplish);
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

int PPViewBill::ViewBillsByOrder(PPID billID)
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

int PPViewBill::AttachBill(PPID billID, const BrowserWindow * pBrw)
{
	int    ok = -1;
	TDialog * dlg = 0;
	SString temp_buf;
	BillTbl::Rec bill_rec;
	if(P_BObj->Search(billID, &bill_rec) > 0) {
		dlg = new TDialog(DLG_ATTACHBILL);
		if(CheckDialogPtrErr(&dlg)) {
			int    to_what = 0; // 1 - to order, 2 - to draft
			PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName, temp_buf);
			dlg->setCtrlString(CTL_ATTACHBILL_INFO, temp_buf);
			if(CheckOpFlags(bill_rec.OpID, OPKF_ONORDER)) {
				to_what = 1;
			}
			else {
				to_what = 2;
				dlg->DisableClusterItem(CTL_ATTACHBILL_SEL, 0, 1);
			}
			dlg->AddClusterAssocDef(CTL_ATTACHBILL_SEL, 0, 1);
			dlg->AddClusterAssocDef(CTL_ATTACHBILL_SEL, 1, 2);
			dlg->SetClusterData(CTL_ATTACHBILL_SEL, to_what);
			if(ExecView(dlg) == cmOK) {
				to_what = dlg->GetClusterData(CTL_ATTACHBILL_SEL);
				if(to_what == 1) {
					ok = AttachBillToOrder(billID);
				}
				else if(to_what == 2) {
					ok = AttachBillToDraft(billID, pBrw);
				}
			}
		}
	}
	delete dlg;
	return ok;
}

int PPViewBill::AttachBillToOrder(PPID billID)
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
			for(PPID id = 0; (r = EnumOperations(PPOPT_GOODSORDER, &id, &op_rec)) > 0;) {
				if(op_rec.AccSheetID == acc_sheet_id && op_rec.Flags & OPKF_ORDERBYLOC) {
					if(op_id == 0)
						op_id = id;
					else {
						op_id = 0;
						break;
					}
				}
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
		if(ViewGoodsBills(&flt, true/*modeless*/) > 0) {
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

int PPViewBill::AttachBillToDraft(PPID billID, const BrowserWindow * pBrw)
{
	int    ok = -1, r;
	BillTbl::Rec bill_rec;
	if(P_BObj->Search(billID, &bill_rec) > 0) {
		uint   msg_id = 0;
		SString temp_buf;
		if(bill_rec.LinkBillID)
			msg_id = PPTXT_BILLALREADYHASLINK;
		else if(!bill_rec.Object)
			msg_id = PPTXT_BILLHASNTOBJFORLINK;
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
			const  int is_intrrcpt = (IsIntrOp(bill_rec.OpID) == INTRRCPT);
			PPID   egais_rcpt_op_id = 0;
			if(is_intrrcpt) {
				PPAlbatrossConfig acfg;
				PPObjGlobalUserAcc gua_obj;
				if(gua_obj.FetchAlbatossConfig(&acfg) > 0)
					egais_rcpt_op_id = acfg.Hdr.EgaisRcptOpID;
			}
			for(PPID id = 0; (r = EnumOperations(0, &id, &op_rec)) > 0;) {
				if(IsDraftOp(id)) {
					PPDraftOpEx doe;
					if(op_rec.AccSheetID == acc_sheet_id && op_obj.GetDraftExData(id, &doe) > 0 && doe.WrOffOpID == bill_rec.OpID) {
						if(op_list.addUnique(id) > 0) {
							doe_flags_list.Add(id, doe.Flags, 0);
						}
					}
					else if(id == egais_rcpt_op_id) { // Специальный случай - привязка драфт-прихода егаис
						if(op_list.addUnique(id) > 0) {
							doe_flags_list.Add(id, 0, 0); // Множественное списание точно нельзя допустить
						}
					}
				}
			}
			if(op_list.getCount()) {
				PPIDArray bill_list;
				DBQ * dbq = 0;
				union {;
					BillTbl::Key1 k1;
					BillTbl::Key3 k3;
				} k;
				BillTbl * t = P_BObj->P_Tbl;
				BExtQuery q(t, egais_rcpt_op_id ? 1 : 3);
				q.select(t->ID, t->Dt, t->OpID, t->Object, t->Object2, t->StatusID, t->Flags, t->CurID, t->Amount, t->EdiOp, 0L);
				if(egais_rcpt_op_id) {
					dbq = & (daterange(t->Dt, &period));
					k.k1.Dt = period.low;
					k.k1.BillNo = 0;
				}
				else {
					dbq = & (t->Object == bill_rec.Object && daterange(t->Dt, &period));
					k.k3.Object = bill_rec.Object;
					k.k3.Dt = period.low;
					k.k3.BillNo = 0;
				}
				q.where(*dbq);
				BillTbl::Rec draft_bill_rec;
				for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
					t->copyBufTo(&draft_bill_rec);
					if(op_list.lsearch(draft_bill_rec.OpID) && (egais_rcpt_op_id == draft_bill_rec.OpID || draft_bill_rec.Object == bill_rec.Object)) {
						int   suited = 0;
						if(draft_bill_rec.Flags & BILLF_WRITEDOFF) {
							long doe_flags = 0;
							if(doe_flags_list.Search(draft_bill_rec.OpID, &doe_flags, 0) && doe_flags & DROXF_MULTWROFF)
								suited = 1;
						}
						else if(draft_bill_rec.OpID == egais_rcpt_op_id) {
							suited = 0;
							if(oneof4(draft_bill_rec.EdiOp, PPEDIOP_EGAIS_WAYBILL, PPEDIOP_EGAIS_WAYBILL_V2, PPEDIOP_EGAIS_WAYBILL_V3, PPEDIOP_EGAIS_WAYBILL_V4)) {
								/* @v11.1.12 
								BillTbl::Rec temp_bill_rec; // В итерационном запросе примечания нет - здесь получим полную запись
								if(P_BObj->Search(draft_bill_rec.ID, &temp_bill_rec) > 0) {
									temp_buf = temp_bill_rec.Memo;
									if(temp_buf.HasPrefixIAscii(PPConst::P_BillNotePrefix_IntrExpnd))
										suited = 1;
								}
								*/
								// @v11.1.12 {
								P_BObj->P_Tbl->GetItemMemo(draft_bill_rec.ID, temp_buf);
								if(temp_buf.HasPrefixIAscii(PPConst::P_BillNotePrefix_IntrExpnd))
									suited = 1;
								// } @v11.1.12 
							}
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
					if(ViewGoodsBills(&flt, true/*modeless*/) > 0) {
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
				PPTooltipMessage(msg_buf, 0, pBrw->H(), 10000, GetColorRef(SClrOrange), SMessageWindow::fShowOnCursor/*|SMessageWindow::fCloseOnMouseLeave*/|
					SMessageWindow::fTextAlignLeft|SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::UniteInventory()
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
		PPWaitStart();
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
		PPWaitStop();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewBill::UniteSellBills()
{
	int    ok = -1;
	PPIDArray  src_ids, dest_ids;
	PPBillPacket pack;
	if(PPMessage(mfConf|mfYesNo, PPCFM_UNITESELL) == cmYes) {
		PPWaitStart();
		THROW(GetBillIDList(&src_ids));
		if(src_ids.getCount() > 1) {
			uint   i;
			PPBill last_bill;
			PPTransaction tra(1);
			const  PPID src_bill_id = src_ids.at(src_ids.getCount()-1);
			THROW(tra);
			THROW(P_BObj->P_Tbl->Extract(src_bill_id, &last_bill) > 0);
			THROW(pack.CreateBlank2(Filt.OpID, last_bill.Rec.Dt, last_bill.Rec.LocID, 0));
			pack.Rec.Object  = last_bill.Rec.Object;
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
					if(P_BObj->P_LotXcT) { // @v11.0.0
						THROW(P_BObj->P_LotXcT->PutContainer(pack.Rec.ID, &pack.XcL, 0)); // Здесь надо сохранить расширенные коды лотов для pack
					}
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
			if(P_BObj->P_LotXcT) { // @v11.0.0
				THROW(P_BObj->P_LotXcT->PutContainer(pack.Rec.ID, &pack.XcL, 0)); // Здесь надо сохранить расширенные коды лотов для pack
			}
			for(i = 0; i < dest_ids.getCount(); i++) {
				const  PPID dest_id = dest_ids.get(i);
				THROW(P_BObj->RecalcPayment(dest_id, 0));
				THROW(P_BObj->RecalcTurns(dest_id, 0, 0));
			}
			THROW(tra.Commit());
			ok = 1;
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::UniteReceiptBills()
{
	int    ok = -1;
	PPIDArray ary;
	if(PPMessage(mfConf|mfYesNo, PPCFM_UNITERCPT) == cmYes) {
		PPWaitStart();
		THROW(GetBillIDList(&ary));
		if(ary.getCount() > 1) {
			PPID   dest_id = ary.get(0);
			ary.reverse(0, ary.getCount());
			ary.atFree(ary.getCount()-1);
			THROW(P_BObj->UniteReceiptBill(dest_id, ary, 1));
			ok = 1;
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::ChangeFlags()
{
	int    ok = -1, r;
	long   set = 0, reset = 0;
	PPID   new_status_id = 0;
	THROW(P_BObj->CheckRights(BILLOPRT_MULTUPD, 1));
	if(ChangeBillFlagsDialog(&set, &reset, &new_status_id) > 0 && (set || reset || new_status_id)) {
		PPIDArray ary;
		PPLogger logger;
		THROW(GetBillIDList(&ary));
		{
			PPTransaction tra(1);
			THROW(tra);
			for(uint ididx = 0; ididx < ary.getCount(); ididx++) {
				const  PPID bill_id = ary.get(ididx);
				PPBillPacket pack;
				if(P_BObj->ExtractPacket(bill_id, &pack) > 0) {
					const long preserve_flags = pack.Rec.Flags;
					for(uint p = 0; p < 32; p++) {
						const ulong t = (1UL << p);
						if(set & t)
							pack.Rec.Flags |= t;
						if(reset & t)
							pack.Rec.Flags &= ~t;
					}
					if(pack.Rec.Flags != preserve_flags) {
						if(P_BObj->UpdatePacket(&pack, 0))
							ok = 1;
						else
							logger.LogLastError();
					}
					if(new_status_id && P_BObj->CheckRights(BILLOPRT_MODSTATUS, 1)) {
						r = P_BObj->SetStatus(bill_id, new_status_id, 0);
						if(r > 0)
							ok = 1;
						else if(!r)
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

int PPViewBill::InsertIntoPool(PPID billID, int use_ta)
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

int PPViewBill::RemoveFromPool(PPID billID, int use_ta)
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

int PPViewBill::UpdateInPool(PPID /*billID*/)
{
	return 1;
}

int PPViewBill::IsMemberOfPool(PPID billID)
{
	PPID   pool_id = Filt.PoolBillID;
	return BIN(P_BObj->IsMemberOfPool(billID, Filt.AssocID, &pool_id) > 0);
}

int PPViewBill::EnumMembersOfPool(PPID * pBillID)
{
	return P_BObj->P_Tbl->EnumMembersOfPool(Filt.AssocID, Filt.PoolBillID, pBillID);
}

int PPViewBill::SetupPoolInsertionFilt(BillFilt * pFilt)
{
	int    ok = -1;
	PPID   first_member_id = 0;
	const  BillFilt pattern = *pFilt;
	BillTbl::Rec pool_rec;
	pFilt->PoolOpID = Filt.PoolOpID;
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

int PPViewBill::AddBillToPool()
{
	class BillPoolAddDialog : public TDialog {
		DECL_DIALOG_DATA(PPViewBill::PoolInsertionParam);
	public:
		BillPoolAddDialog() : TDialog(DLG_BPOOLADD)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
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
		DECL_DIALOG_GETDTS()
		{
			long   temp_long = 0;
			GetClusterData(CTL_BPOOLADD_VERB, &Data.Verb);
			if(GetClusterData(CTL_BPOOLADD_BILLKIND, &temp_long))
				Data.AddedBillKind = static_cast<BrowseBillsType>(temp_long);
			ASSIGN_PTR(pData, Data);
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
	};
	int    ok = 1;
	int    r;
	const  PPConfig & r_cfg = LConfig;
	int    count = 0;
	PPID   loc_id = LocList_.getSingle();
	SETIFZ(loc_id, r_cfg.Location);
 	const  PPID  save_loc_id = r_cfg.Location;
	PPID   ar_id = 0;
	PPID   ar2_id = 0;
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
				ar2_id = tses_rec.Ar2ID;
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
				op_type_list.addzlist(PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, 0L);
			else
				op_type_list.addzlist(PPOPT_ACCTURN, PPOPT_GOODSORDER, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND,
					PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND,
					PPOPT_DRAFTTRANSIT, PPOPT_CORRECTION, 0L);
			if(op_id || BillPrelude(&op_type_list, (is_op_list ? OPKLF_OPLIST : 0), 0, &op_id, &loc_id) > 0) {
				PPObjBill::AddBlock ab;
				ab.OpID = op_id;
				ab.ObjectID = ar_id;
				ab.Object2ID = ar2_id;
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
			if(!r)
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

int PPViewBill::GetCommonPoolAttribs(LDATE * pDt, PPID * pLocID, PPID * pOpID, PPID * pObjID)
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

int PPViewBill::CreateTempPoolPacket(PPBillPacket * pPack)
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
	// @v11.1.12 STRNSCPY(pPack->Rec.Memo, pool_pack.Rec.Memo);
	pPack->SMemo = pool_pack.SMemo; // @v11.1.12
	pPack->Rec.Dt     = (comm_dt > 0) ? comm_dt : pool_pack.Rec.Dt;
	pPack->Rec.LocID  = (comm_loc_id > 0) ? comm_loc_id : pool_pack.Rec.LocID;
	pPack->Rec.Object = (comm_obj_id >= 0) ? comm_obj_id : pool_pack.Rec.Object;
	pPack->SetFreight(pool_pack.P_Freight);
	const long sign_mask = (PPTFR_PLUS | PPTFR_MINUS);
	while(EnumMembersOfPool(&member_id) > 0) {
		for(r_by_bill = 0; P_BObj->trfr->EnumItems(member_id, &r_by_bill, &ti) > 0;) {
			for(i = 0, found = 0; !found && pPack->EnumTItems(&i, &p_ti) > 0;) {
				if(p_ti->GoodsID == ti.GoodsID && (p_ti->Flags & sign_mask) == (ti.Flags & sign_mask)) {
					const double sum_qtty = p_ti->Quantity_ + ti.Quantity_;
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

int PPViewBill::ShowPoolDetail(const PPBillPacket & rBillPack)
{
	int    ok = 1;
	if(rBillPack.Rec.ID && GetOpType(rBillPack.Rec.OpID) == PPOPT_POOL) {
		BillFilt   flt;
		PPViewBill bv;
		flt.Bbt = bbtUndef;
		flt.PoolBillID = rBillPack.Rec.ID;
		flt.AssocID    = PPASS_OPBILLPOOL;
		flt.Flags |= BillFilt::fEditPoolByType;
		if(bv.Init_(&flt))
			ok = bv.Browse(0);
		else
			ok = PPErrorZ();
	}
	return ok;
}

int PPViewBill::ShowDetails(PPID billID)
{
	int    ok = -1;
	int    is_lock = 0;
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
			ShowPoolDetail(pack);
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

static int SelectPrintPoolVerb(int * pVerb)
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

int PPViewBill::PrintBill(PPID billID)
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
					THROW(P_BObj->PrintCheck__(&pack, 0, /*addCashSummator*/1));
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
				if(ok > 0) {
					if(prn_verb == 1)
						bv.Print();
					else {
						PPBillPacket merged_pack;
						THROW(bv.CreateTempPoolPacket(&merged_pack));
						PrintGoodsBill(&merged_pack);
					}
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

int PPViewBill::PrintAllBills()
{
	int    ok = -1;
	int    is_packet = 0;
	const  PPID op_type_id = Filt.OpID ? GetOpType(Filt.OpID) : 0;
	SVector * p_rpt_ary = 0;
	long   out_prn_flags = 0;
	bool   skip = false;
	int    msg_id = 0;
	//PPTXT_PRNALLBILLS_NOFLTOP              "Печать всех первичных документов выборки доступна лишь при выборе вида операции в фильтре"
	//PPTXT_PRNALLBILLS_FLTOPISPOOL          "Печать всех первичных документов выборки не доступна для выборки пулов документов"
	//PPTXT_PRNALLBILLS_FLTOPISPAYMWOINVCOPT "Печать всех первичных документов выборки оплат доступна только если вид операции имеет флаг 'Печатать счет-фактуру'"
	//PPTXT_PRNALLBILLS_FLTOPGEN_NOCOMMOPTYP "Печать всех первичных документов выборки по обобщенной операции не доступна если в списке есть документы разных типов операции"
	//PPTXT_PRNALLBILLS_FLTOPGEN_NOCOMMACS   "Печать всех первичных документов выборки по обобщенной операции не доступна если в обобщении есть разные таблицы статей"
	if(!op_type_id) {
		msg_id = PPTXT_PRNALLBILLS_NOFLTOP;
		skip = true;
	}
	else if(op_type_id == PPOPT_POOL) {
		msg_id = PPTXT_PRNALLBILLS_FLTOPISPOOL;
		skip = true;
	}
	else if(op_type_id == PPOPT_PAYMENT) {
		if(!CheckOpPrnFlags(Filt.OpID, OPKF_PRT_INVOICE)) {
			msg_id = PPTXT_PRNALLBILLS_FLTOPISPAYMWOINVCOPT;
			skip = true;
		}
	}
	else if(op_type_id == PPOPT_GENERIC) {
		// @v12.2.6 @exploration {
		// @todo Добавить опцию в конфигурацию, которая позволяла бы такой трюк
		PPID   common_op_type_id = 0;
		PPID   common_acs_id = -1;
		BillViewItem item;
		PPIDArray op_list;
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			op_list.add(item.OpID);
		}
		if(op_list.getCount()) {
			op_list.sortAndUndup();
			{
				for(uint opi = 0; common_op_type_id >= 0 && opi < op_list.getCount(); opi++) {
					const PPID iter_op_id = op_list.get(opi);
					PPOprKind2 op_rec;
					const PPID iter_op_type_id = GetOpType(iter_op_id, &op_rec);
					if(iter_op_type_id <= 0)
						common_op_type_id = -1;
					else if(!common_op_type_id)
						common_op_type_id = iter_op_type_id;
					else if(common_op_type_id != iter_op_type_id)
						common_op_type_id = -1;
					//
					if(common_acs_id < 0) {
						common_acs_id = op_rec.AccSheetID;
					}
					else if(common_acs_id != op_rec.AccSheetID)
						common_op_type_id = -1;
				}
			}
		}
		if(common_op_type_id <= 0) {
			msg_id = PPTXT_PRNALLBILLS_FLTOPGEN_NOCOMMOPTYP;
			skip = true;
		}
		else if(common_acs_id < 0) {
			msg_id = PPTXT_PRNALLBILLS_FLTOPGEN_NOCOMMACS;
			skip = true;
		}
		// } @v12.2.6 
	}
	// @v12.2.6 if(!oneof3(op_type_id, 0, PPOPT_POOL, PPOPT_GENERIC) && (op_type_id != PPOPT_PAYMENT || CheckOpPrnFlags(Filt.OpID, OPKF_PRT_INVOICE))) {
	// @todo Если skip, то вывести сообщение о том, что нельзя массово отпечатать доки
	if(skip) {
		if(msg_id) {
			SString msg_buf;
			PPLoadText(msg_id, msg_buf);
			PPOutputMessage(msg_buf, mfInfo|mfOK);
		}
	}
	else { // @v12.2.6
		int    out_amt_type = 0;
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
		int    r = 1;
		for(InitIteration(order); r > 0 && NextIteration(&item) > 0;) {
			if(item.ID) {
				is_packet = 0;
				THROW(P_BObj->ExtractPacket(item.ID, &pack));
				is_packet = 1;
				// @v11.4.0 r = (count == 0) ? PrepareBillMultiPrint(&pack, &p_rpt_ary, &out_prn_flags) : 1;
				// @v11.4.0 {
				if(count == 0) {
					r = PrepareBillMultiPrint(&pack, &p_rpt_ary, &out_prn_flags);
					out_amt_type = pack.OutAmtType; // Сохраняем значение pack.OutAmtType выбранное в интерактивном режиме
				}
				else {
					r = 1;
					pack.OutAmtType = out_amt_type; // Используем вариант выбора цены, интерактивно выбранный на первой итерации
				}
				// } @v11.4.0 
				if(r > 0) {
					// @v11.4.0 @SevaSob pack.OutAmtType = out_amt_type; 
					// THROW(r = PrintGoodsBill(&pack, &p_rpt_ary, 1));
					THROW(r = MultiPrintGoodsBill(&pack, p_rpt_ary, out_prn_flags));
					// @v11.4.0 out_amt_type = pack.OutAmtType;
					count++;
				}
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

int PPViewBill::UpdateAttributes()
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
		PPWaitStart();
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
							// @v11.1.12 BillCore::SetCode(pack.Rec.Code, pack.Rec.Flags);
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
						// @v11.1.12 BillCore::SetCode(rec.Code, rec.Flags);
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
		PPWaitStop();
	}
	CATCH
		P_BObj->atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int PPViewBill::CalcBillVATax(BVATAccmArray * dest)
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

static void _SetTotal(TDialog * dlg, uint ctl, double s, double vatnf, double vat)
{
	SString buf;
	buf.Space().
		Cat(s, MKSFMTD(19, 2, NMBF_TRICOMMA|ALIGN_RIGHT)).CR().Space().
		Cat(vatnf, MKSFMTD(19, 4, NMBF_TRICOMMA|ALIGN_RIGHT)).CR().Space().
		Cat(vat,   MKSFMTD(19, 4, NMBF_TRICOMMA|ALIGN_RIGHT));
	dlg->setStaticText(ctl, buf);
}

int PPViewBill::ViewVATaxList()
{
	int    ok = 1;
	BVATAccmArray dest;
	uint   i;
	double vat_cost   = 0.0, vat_price   = 0.0, tmp;
	double vatnf_cost = 0.0, vatnf_price = 0.0; // Сумма НДС для тех, кто платит
	double s_cost     = 0.0, s_price     = 0.0;
	double snf_cost   = 0.0, snf_price   = 0.0; // Сумма по документам для тех, кто платит
	SString sub;
	BVATAccm * p_item, total;
	StrAssocArray * p_ary = 0;
	SmartListBox * p_list = 0;
	ListBoxDef   * p_def  = 0;
	TDialog * dlg    = 0;
	PPWaitStart();
	THROW(CalcBillVATax(&dest));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_VATAXLST))));
	p_list = static_cast<SmartListBox *>(dlg->getCtrlView(CTL_VATAXLST_LIST));
	MEMSZERO(total);
	THROW_MEM(p_ary = new StrAssocArray);
	for(i = 0; dest.enumItems(&i, (void **)&p_item);) {
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
	PPWaitStop();
	ExecView(dlg);
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

void PPViewBill::ViewTotal()
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
			if(event.isCmd(cmPrint)) {
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
	if(Filt.Flags & BillFilt::fInvOnly) {
		PPIDArray id_list;
		BillViewItem item;
		PPWaitStart();
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			id_list.add(item.ID);
		id_list.sortAndUndup();
		PPWaitStop();
		P_BObj->ViewInventoryTotal(id_list, 0);
	}
	else {
		BillTotal total;
		PPWaitStart();
		if(CalcTotal(&total)) {
			PPWaitStop();
			if(!P_BObj->CheckRights(BILLRT_ACCSCOST)) {
				total.Amounts.Put(PPAMT_BUYING, 0L/*@curID*/, 0, 0, 1);
				if(Filt.OpID && CheckOpFlags(Filt.OpID, OPKF_BUYING))
					total.Sum = 0.0;
			}
			total.Amounts.Remove(PPAMT_PCTDIS, 0L);
			BillTotalDialog * dlg = new BillTotalDialog(DLG_BILLTOTAL2, CTL_BILLTOTAL2_AMTLIST, &total, this);
			if(CheckDialogPtrErr(&dlg))
				ExecViewAndDestroy(dlg);
		}
		PPWaitStop();
	}
}

// @vmiller
struct PrvdrDllLink {
	PrvdrDllLink() : SessId(0), P_ExpDll(0)
	{
		PrvdrSymb[0] = 0;
	}
	int    SessId;
	char   PrvdrSymb[12]; // PPSupplExchangeCfg::PrvdrSymb
	ImpExpDll * P_ExpDll;
	ImpExpParamDllStruct ParamDll;
};

int WriteBill_ExportMarks(const PPBillImpExpParam & rParam, const PPBillPacket & rBp, const SString & rFileName, SString & rResultFileName); // @prototype

static bool IsByEmailAddrByContext(const SString & rBuf) { return (rBuf.IsEqiAscii("@bycontext") || rBuf == "@@"); }

int PPViewBill::ExportGoodsBill(const PPBillImpExpParam * pBillParam, const PPBillImpExpParam * pBRowParam)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    r = 0;
	int    dll_pos = 0;
	PPID   prev_bill_id = 0;
	PPBillExporter b_e;
	BillViewItem view_item;
	SString msg_buf;
	SString fmt_buf;
	SString temp_buf;
	STempBuffer result_str(1024);
	char   errmsg_[1024];
	PrvdrDllLink * p_prvd_dll_link = 0;
	TSCollection <PrvdrDllLink> exp_dll_coll;
	SString doc_type;
	PPObjTag tag_obj;
	PPLogger logger;
	StringSet result_file_list;
	PPIDArray bill_id_list;
	PPIDArray exported_bill_id_list; // @v11.5.6 Список идентификаторов документов, которые были в действительности экспортированы
	PPID   single_loc_id = -1;
	int    is_there_bnkpaym = 0;
	for(InitIteration(OrdByDefault); NextIteration(&view_item) > 0;) {
		if(IsGoodsDetailOp(view_item.OpID) || view_item.Flags & BILLF_BANKING) {
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
			//
			const  PPID inet_acc_id = b_e.Tp.InetAccID;
			const StrAssocArray inet_addr_list = b_e.Tp.AddrList;
			// @v11.8.6 PPID  fix_tag_id = 0; // @v11.5.6
			// @v11.8.6 PPObjectTag fix_tag_rec;
			bool   use_mail_addr_by_context = false;
			SString email_buf;
			SString mail_subj(b_e.Tp.Subject);
			mail_subj.SetIfEmpty("No Subject").Transf(CTRANSF_INNER_TO_UTF8);
			if(inet_acc_id && inet_addr_list.getCount()) {
				for(uint ai = 0; !use_mail_addr_by_context && ai < inet_addr_list.getCount(); ai++) {
					temp_buf = inet_addr_list.Get(ai).Txt;
					if(IsByEmailAddrByContext(temp_buf))
						use_mail_addr_by_context = true;
				}
			}
			//
			PPBillImpExpParam bill_param(b_e.BillParam);
			PPBillImpExpParam brow_param(b_e.BRowParam);
			if(b_e.GetIEBill())
				bill_param.FileName = b_e.GetIEBill()->GetPreservedOrgFileName();
			if(b_e.GetIEBRow())
				brow_param.FileName = b_e.GetIEBRow()->GetPreservedOrgFileName();
			// @v11.5.6 {
			/* @v11.8.6
			if(bill_param.FixTagID) {
				if(tag_obj.Search(bill_param.FixTagID, &fix_tag_rec) > 0 && fix_tag_rec.ObjTypeID == PPOBJ_BILL)
					fix_tag_id = fix_tag_rec.ID;	
			}*/
			// } @v11.5.6 
			// (не надо: сервисные функции PPBillImpExpBaseProcessBlock сами все сделают) fix_tag_id = b_e.GetFixTagID(0); // @v11.8.6
			if(b_e.BillParam.PredefFormat) {
				if(oneof7(b_e.BillParam.PredefFormat, piefNalogR_Invoice, piefNalogR_REZRUISP, piefNalogR_SCHFDOPPR, piefExport_Marks, 
					piefNalogR_ON_NSCHFDOPPRMARK, piefNalogR_ON_NSCHFDOPPR, piefNalogR_ON_NKORSCHFDOPPR)) { // @v11.2.1 piefNalogR_ON_NSCHFDOPPR // @v11.7.0 piefNalogR_ON_NKORSCHFDOPPR
					SString result_file_name_;
					PPWaitStart();
					for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
						const  PPID bill_id = bill_id_list.get(_idx);
						int    err = 0;
						// @v11.8.6 ObjTagItem fix_tag_item;
						// @v11.8.6 const  bool do_skip = (fix_tag_id && p_ref->Ot.GetTag(PPOBJ_BILL, bill_id, fix_tag_id, &fix_tag_item) > 0);
						const  bool do_skip = b_e.SkipExportBillBecauseFixTag(bill_id); // @v11.8.6
						if(!do_skip && P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
							int    r = 0;
							THROW(b_e.Init(&bill_param, &brow_param, &pack, 0 /*&result_file_list*/));
							{
								const SString nominal_file_name = b_e.BillParam.FileName;
								// @v11.2.2 {
								if(oneof2(b_e.BillParam.PredefFormat, piefNalogR_ON_NSCHFDOPPRMARK, piefNalogR_ON_NSCHFDOPPR)) {
									bool pack_has_marks = false;
									PPLotExtCodeContainer::MarkSet ext_codes_set;
									StringSet ss;
									Goods2Tbl::Rec goods_rec;
									PPGoodsType gt_rec;
									BarcodeArray bc_list; // @v11.8.3
									SString norm_code; // @v11.8.3
									for(uint tiidx = 0; !pack_has_marks && tiidx < pack.GetTCount(); tiidx++) {
										const PPTransferItem & r_ti = pack.ConstTI(tiidx);
										const  PPID goods_id = r_ti.GoodsID;
										if(pack.XcL.Get(tiidx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
											ext_codes_set.GetByBoxID(0, ss);
											for(uint ecsp = 0; !pack_has_marks && ss.get(&ecsp, temp_buf);) {
												if(temp_buf.NotEmptyS())
													pack_has_marks = true;
											}
										}
										// @v11.8.3 {
										if(GObj.Fetch(goods_id, &goods_rec) > 0 && goods_rec.GoodsTypeID) {
											// @v12.2.9 GTCHZNPT_SOFTDRINKS
											if(GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0 && oneof3(gt_rec.ChZnProdType, GTCHZNPT_MILK, GTCHZNPT_WATER, GTCHZNPT_SOFTDRINKS)) {
												GObj.P_Tbl->ReadBarcodes(goods_id, bc_list);
												temp_buf.Z();
												for(uint bcidx = 0; !pack_has_marks && bcidx < bc_list.getCount(); bcidx++) {
													const BarcodeTbl::Rec & r_bc_rec = bc_list.at(bcidx);
													int    diag = 0;
													int    std = 0;
													const  int dbcr = PPObjGoods::DiagBarcode(r_bc_rec.Code, &diag, &std, &norm_code);
													if(dbcr > 0 && oneof4(std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE)) {
														assert(norm_code.Len() < 14);
														if(norm_code.Len() < 14)
															pack_has_marks = true;
													}
												}
											}
										}
										// } @v11.5.9
									}
									const  bool is_exp_correction = (pack.OpTypeID == PPOPT_CORRECTION && pack.P_LinkPack->OpTypeID == PPOPT_GOODSEXPEND);
									if(is_exp_correction) {
										DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, "ON_NKORSCHFDOPPR", nominal_file_name);
										r = _blk.Do_CorrInvoice(result_file_name_);
									}
									else {
										DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, (pack_has_marks ? "ON_NSCHFDOPPRMARK" : "ON_NSCHFDOPPR"), nominal_file_name);
										r = _blk.Do_Invoice2(result_file_name_);
									}
								}
								else { // } @v11.2.2 
									switch(b_e.BillParam.PredefFormat) {
										case piefNalogR_Invoice:  
											{
												DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, "ON_SFAKT", nominal_file_name);
												r = _blk.IsValid() ? _blk.Do_Invoice(result_file_name_) : 0;
											}
											break;
										case piefNalogR_REZRUISP: 
											{
												DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, "DP_REZRUISP", nominal_file_name);
												r = _blk.IsValid() ? _blk.Do_Invoice(result_file_name_) : 0;
											}
											break;
										case piefNalogR_SCHFDOPPR: 
											{
												DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, "ON_NSCHFDOPPR", nominal_file_name);
												r = _blk.IsValid() ? _blk.Do_UPD(result_file_name_) : 0;
											}
											break;
										case piefExport_Marks: // @erik 
											r = WriteBill_ExportMarks(b_e.BillParam, pack, nominal_file_name, result_file_name_); 
											break; 
										case piefNalogR_ON_NKORSCHFDOPPR: 
											{
												DocNalogRu_WriteBillBlock _blk(b_e.BillParam, pack, "ON_NKORSCHFDOPPR", nominal_file_name);
												r = _blk.IsValid() ? _blk.Do_CorrInvoice(result_file_name_) : 0;
											}
											break;
									}
								}
								if(r > 0) {
									result_file_list.add(result_file_name_);
									exported_bill_id_list.add(bill_id); // @v11.5.6
								}
							}
						}
						PPWaitPercent(_idx+1, bill_id_list.getCount());
					}
					// @todo @v12.2.6 Отправить файлы на ftp (или еще куда)
					if(result_file_list.getCount()) {
						PPObjInternetAccount ia_obj;
						PPInternetAccount2 ia_pack;
						// @v12.2.6 {
						if(b_e.BillParam.InetAccID && ia_obj.Get(b_e.BillParam.InetAccID, &ia_pack) > 0 && ia_pack.Flags & PPInternetAccount::fFtpAccount) {
							SString ftp_path;
							SString naked_file_name;
							SString accs_name;
							for(uint rflp = 0; result_file_list.get(&rflp, temp_buf);) {
								ftp_path.Z();
								naked_file_name.Z();
								accs_name.Z();
								{
									SFsPath ps(temp_buf);
									ps.Merge(SFsPath::fNam|SFsPath::fExt, naked_file_name);
									ia_pack.GetExtField(FTPAEXSTR_HOST, ftp_path);
								}
								{
									SUniformFileTransmParam param;
									char   pwd[256];
									(param.SrcPath = temp_buf).Transf(CTRANSF_OUTER_TO_UTF8);
									SFsPath::NormalizePath(ftp_path, SFsPath::npfSlash|SFsPath::npfKeepCase, param.DestPath);
									param.Flags = 0;
									param.Format = SFileFormat::Unkn;
									ia_pack.GetExtField(FTPAEXSTR_USER, accs_name);
									ia_pack.GetPassword_(pwd, sizeof(pwd), FTPAEXSTR_PASSWORD);
									param.AccsName.EncodeUrl(accs_name, 0);
									param.AccsPassword.EncodeUrl(pwd, 0);
									memzero(pwd, sizeof(pwd));
									if(param.Run(0, 0)) {
										; // @todo @succ
									}
									else {
										logger.LogLastError();
									}
									accs_name.Obfuscate();
								}
							}
						}
						// } @v12.2.6 
						if(inet_acc_id && inet_addr_list.getCount()) {
							for(uint ai = 0; ai < inet_addr_list.getCount(); ai++) {
								(temp_buf = inet_addr_list.Get(ai).Txt).Strip();
								if(IsByEmailAddrByContext(temp_buf)) {
									;
								}
								else
									email_buf.CatDivIfNotEmpty(',', 0).Cat(temp_buf);
							}
							if(email_buf.NotEmptyS() && !PutFilesToEmail2(&result_file_list, inet_acc_id, email_buf, mail_subj, 0))
								logger.LogLastError();
						}
					}
					PPWaitStop();
				}
			}
			else if(b_e.Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
				StringSet local_result_file_list;
				PPObjSecur sec_obj(PPOBJ_USR, 0);
				email_buf.Z();
				THROW(Helper_ExportBnkOrder(b_e.CfgNameBill, &local_result_file_list, logger));
				if(inet_acc_id && inet_addr_list.getCount() && local_result_file_list.getCount()) {
					for(uint ai = 0; ai < inet_addr_list.getCount(); ai++) {
						(temp_buf = inet_addr_list.Get(ai).Txt).Strip();
						if(IsByEmailAddrByContext(temp_buf)) {
							const  PPID user_id = LConfig.UserID;
							PPSecur sec_rec;
							if(user_id && sec_obj.Fetch(user_id, &sec_rec) > 0 && sec_rec.PersonID) {
								StringSet ss_elink;
								PPELinkArray elink_list;
								PsnObj.P_Tbl->GetELinks(sec_rec.PersonID, elink_list);
								if(elink_list.GetListByType(ELNKRT_EMAIL, ss_elink) > 0) {
									for(uint sselp = 0; ss_elink.get(&sselp, temp_buf);) {
										if(temp_buf.NotEmptyS()) {
											email_buf.CatDivIfNotEmpty(',', 0).Cat(temp_buf);
											break;
										}
									}
								}
							}
						}
						else
							email_buf.CatDivIfNotEmpty(',', 0).Cat(temp_buf);
					}
					if(email_buf.NotEmptyS() && !PutFilesToEmail2(&local_result_file_list, inet_acc_id, email_buf, mail_subj, 0))
						logger.LogLastError();
				}
			}
			else if(b_e.Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp) {
				long   cflags = (b_e.Flags & PPBillImporter::fTestMode) ? PPEgaisProcessor::cfDebugMode : 0;
				if(b_e.Flags & PPBillImporter::fEgaisVer4) { // @11.0.12
					cflags |= PPEgaisProcessor::cfVer4;
					cflags &= ~PPEgaisProcessor::cfVer3;
				}
				else if(b_e.Flags & PPBillImporter::fEgaisVer3) {
					cflags |= PPEgaisProcessor::cfVer3;
					cflags &= ~PPEgaisProcessor::cfVer4;
				}
				PPEgaisProcessor ep(cflags, &logger, 0); // @instantiation(PPEgaisProcessor)
				THROW(ep);
				THROW(ep.CheckLic());
				{
					PPBillIterchangeFilt sbp;
					sbp.IdList = bill_id_list;
					sbp.LocID = (single_loc_id > 0) ? single_loc_id : Filt.LocList.GetSingle();
					TSVector <PPEgaisProcessor::UtmEntry> utm_list;
					THROW(ep.GetUtmList(sbp.LocID, utm_list));
					for(uint i = 0; i < utm_list.getCount(); i++) {
						ep.SetUtmEntry(sbp.LocID, &utm_list.at(i), &sbp.Period);
						ep.SendBillActs(sbp);
						ep.SendBillRepeals(sbp);
						ep.SendBills(sbp);
						ep.SetUtmEntry(0, 0, 0);
					}
				}
			}
			else {
				SString edi_prvdr_symb;
				PPWaitStart();
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
											PPVersionInfo vi;
											//vers_info.GetProductName(temp_buf.Z());
											vi.GetTextAttrib(vi.taiProductName, temp_buf);
											STRNSCPY(hdr.SrcSystemName, temp_buf);
											//vers_info.GetVersionText(hdr.SrcSystemVer, sizeof(hdr.SrcSystemVer));
											vi.GetTextAttrib(vi.taiVersionText, temp_buf);
											STRNSCPY(hdr.SrcSystemVer, temp_buf);
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
								PrvdrDllLink * p_prvdr_item = (dll_pos < static_cast<int>(exp_dll_coll.getCount())) ? exp_dll_coll.at(dll_pos) : 0;
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
					if(b_e.BillParam.Flags & PPBillImpExpParam::fExpOneByOne) {
						StringSet local_result_file_list;
						for(uint _idx = 0; _idx < bill_id_list.getCount(); _idx++) {
							const  PPID bill_id = bill_id_list.get(_idx);
							// @v11.8.6 ObjTagItem fix_tag_item;
							// @v11.8.6 const  bool do_skip = (fix_tag_id && p_ref->Ot.GetTag(PPOBJ_BILL, bill_id, fix_tag_id, &fix_tag_item) > 0); // @v11.5.6
							const  bool do_skip = b_e.SkipExportBillBecauseFixTag(bill_id); // @v11.8.6
							if(!do_skip && P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
								local_result_file_list.Z();
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
										if(!(b_e.BillParam.Flags & PPBillImpExpParam::fImpExpRowsOnly)) // @v11.1.10
											b_e.BillParam.DistributeFile(&logger);
										if(p_iebrow && fileExists(b_e.BRowParam.FileName) && b_e.BRowParam.FileName.CmpNC(b_e.BillParam.FileName) != 0)
											b_e.BRowParam.DistributeFile(&logger);
									}
								}
								if(use_mail_addr_by_context && pack.GetContextEmailAddr(temp_buf) > 0 && local_result_file_list.getCount()) {
									if(PutFilesToEmail2(&local_result_file_list, inet_acc_id, temp_buf, mail_subj, 0)) {
										exported_bill_id_list.add(bill_id); // @v11.5.6
									}
									else
										logger.LogLastError();
								}
								else
									exported_bill_id_list.add(bill_id); // @v11.5.6
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
							// @v11.8.6 ObjTagItem fix_tag_item;
							// @v11.8.6 const  bool do_skip = (fix_tag_id && p_ref->Ot.GetTag(PPOBJ_BILL, bill_id, fix_tag_id, &fix_tag_item) > 0); // @v11.5.6
							const  bool do_skip = b_e.SkipExportBillBecauseFixTag(bill_id); // @v11.8.6
							if(!do_skip) {
								if(P_BObj->ExtractPacketWithFlags(bill_id, &pack, BPLD_FORCESERIALS) > 0) {
									if(!b_e.PutPacket(&pack, 0, 0)) {
										logger.LogMsgCode(mfError, PPERR_IMPEXP_BILL, pack.Rec.Code);
									}
									else {
										PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
										PPFormatT(PPTXT_LOG_EXPBILL_ITEM, &msg_buf, temp_buf.cptr());
										logger.Log(msg_buf);
										exported_bill_id_list.add(bill_id); // @v11.5.6
									}
								}
								else
									logger.LogLastError();
							}
							PPWaitPercent(_idx+1, bill_id_list.getCount());
						}
						CALLPTRMEMB(p_iebrow, CloseFile());
						if(p_iebill) {
							p_iebill->CloseFile();
							if(exported_bill_id_list.getCount()) { // @v11.5.6
								b_e.BillParam.DistributeFile(&logger);
								if(p_iebrow && fileExists(b_e.BRowParam.FileName) && b_e.BRowParam.FileName.CmpNC(b_e.BillParam.FileName) != 0)
									b_e.BRowParam.DistributeFile(&logger);
							}
						}
						//THROW(b_e.SignBill());
						ok = 1;
					}
					if(inet_acc_id && inet_addr_list.getCount() && result_file_list.getCount()) {
						temp_buf.Z();
						for(uint ai = 0; !use_mail_addr_by_context && ai < inet_addr_list.getCount(); ai++) {
							temp_buf = inet_addr_list.Get(ai).Txt;
							if(IsByEmailAddrByContext(temp_buf))
								temp_buf.Z();
							else
								break;
						}
						if(temp_buf.NotEmptyS() && !PutFilesToEmail2(&result_file_list, inet_acc_id, temp_buf, mail_subj, 0))
							logger.LogLastError();
					}
				}
			}
			// @v11.5.6 {
			/* @v11.8.6 if(exported_bill_id_list.getCount()) {
				if(fix_tag_id && oneof6(fix_tag_rec.TagDataType, OTTYP_BOOL, OTTYP_NUMBER, OTTYP_INT, OTTYP_DATE, OTTYP_TIMESTAMP, OTTYP_STRING)) {
					exported_bill_id_list.sortAndUndup();
					PPTransaction tra(1);
					THROW(tra);
					for(uint bidx = 0; bidx < exported_bill_id_list.getCount(); bidx++) {
						const  PPID bill_id = exported_bill_id_list.get(bidx);
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(bill_id, &bill_rec) > 0) {
							ObjTagItem tag_item;
							switch(fix_tag_rec.TagDataType) {
								case OTTYP_BOOL: tag_item.SetInt(fix_tag_id, 1); break;
								case OTTYP_NUMBER: tag_item.SetReal(fix_tag_id, 1.0); break;
								case OTTYP_INT:  tag_item.SetInt(fix_tag_id, 1); break;
								case OTTYP_DATE: tag_item.SetDate(fix_tag_id, getcurdate_()); break;
								case OTTYP_TIMESTAMP: tag_item.SetTimestamp(fix_tag_id, getcurdatetime_()); break;
								case OTTYP_STRING: tag_item.SetStr(fix_tag_id, "done"); break;
							}
							THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_id, &tag_item, 0));
						}
					}
					THROW(tra.Commit());
				}
			}*/
			// } @v11.5.6 
			// @v11.8.6 {
			exported_bill_id_list.sortAndUndup();
			THROW(b_e.SetFixTagOnExportedBill(exported_bill_id_list, 1));
			// } @v11.8.6
		}
	}
	CATCH
		if(dll_pos < static_cast<int>(exp_dll_coll.getCount())) {
			ImpExpDll * p_ied = exp_dll_coll.at(dll_pos)->P_ExpDll;
			if(p_ied && p_ied->IsInited()) {
				errmsg_[0] = 0;
				p_ied->GetErrorMessage(errmsg_, sizeof(errmsg_));
				PPSetError(PPERR_IMPEXP_DLL, msg_buf.Z().Cat(errmsg_).Transf(CTRANSF_OUTER_TO_INNER));
			}
		}
		logger.LogLastError();
		ok = PPErrorZ();
	ENDCATCH
	for(uint i = 0; i < exp_dll_coll.getCount(); i++) {
		ImpExpDll * p_ied = exp_dll_coll.at(i)->P_ExpDll;
		b_e.BillParam.ImpExpParamDll = exp_dll_coll.at(i)->ParamDll;
		if(p_ied && p_ied->IsInited())
			b_e.CheckBillsWasExported(p_ied);
		ZDELETE(exp_dll_coll.at(i)->P_ExpDll);
	}
	PPWaitStop();
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	return ok;
}

int PPViewBill::Helper_ExportBnkOrder(const char * pSection, StringSet * pResultFileList, PPLogger & rLogger)
{
	int    ok = -1;
	SString section(pSection);
	if(section.NotEmptyS()) {
		SString temp_buf;
		SString fmt_buf;
		SString msg_buf;
		BillViewItem item;
		PPIDArray id_list;
		DateRange period;
		period.Set(MAXDATE, encodedate(1, 1, 1900));
		PPWaitStart();
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			if(item.Flags & BILLF_BANKING) {
				id_list.addUnique(item.ID);
				period.AdjustToDate(item.Dt);
			}
			else {
				temp_buf.Z().Cat(item.Dt);
				msg_buf.Printf(PPLoadTextS(PPTXT_BILLNOTBANKING, fmt_buf), item.Code, temp_buf.cptr());
				rLogger.Log(msg_buf);
			}
		}
		const uint cnt = id_list.getCount();
		if(cnt) {
			StringSet local_result_file_list;
			ClientBankExportDef cbed(&period);
			THROW(cbed.ReadDefinition(section));
			THROW(cbed.CreateOutputFile(&local_result_file_list));
			THROW(cbed.PutHeader());
			for(uint i = 0; i < cnt; i++) {
				PPBillPacket pack;
				const  PPID id = id_list.get(i);
				THROW(P_BObj->ExtractPacket(id, &pack) > 0);
				THROW(cbed.PutRecord(&pack, 0, &rLogger));
				PPWaitPercent(i+1, cnt);
			}
			THROW(cbed.PutEnd());
			if(pResultFileList) {
				for(uint sp = 0; local_result_file_list.get(&sp, temp_buf);) {
					pResultFileList->add(temp_buf);
				}
			}
			ok = 1;
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPViewBill::ExportBnkOrder()
{
	int    ok = -1;
	SString section;
	PPLogger logger;
	if(CliBnkSelectCfgDialog(1, section) > 0) {
		ok = Helper_ExportBnkOrder(section, 0, logger);
	}
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	return ok;
}

static int SCardNumDlg(PPSCardPacket & rScPack, CCheckTbl::Rec * pChkRec, int isDraft)
{
	int    ok = -1;
	char   sc_code[24];
	PPObjSCard sc_obj;
	CCheckTbl::Rec cc_rec;
	TDialog * p_dlg = new TDialog(isDraft ? DLG_SCARDCHK : DLG_SCARDNUM);
	LDATE  dt = ZERODATE;
	THROW(CheckDialogPtr(&p_dlg));
	if(isDraft) {
		p_dlg->SetupCalDate(CTLCAL_SCARDNUM_CHKDT, CTL_SCARDNUM_CHKDT);
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
			if(sc_obj.SearchCode(0, sc_code, &sc_rec_) != 1)
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
							dttm.Set(dt, ZEROTIME);
							if(sc_obj.P_CcTbl->GetListByCard(sc_rec_.ID, &dttm, &chk_ary) > 0) {
								for(uint i = 0; i < chk_ary.getCount(); i++) {
									THROW(sc_obj.P_CcTbl->Search(chk_ary.at(i), &tmp_cc_rec));
									if(tmp_cc_rec.Code == chk_no && (!cash_no || tmp_cc_rec.PosNodeID == cash_no) && (++chks_qtty == 1))
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

static int SCardInfoDlg(PPSCardPacket & rScPack, PPID * pOpID, long flags, int withoutPsn)
{
	int    without_person = 0;
	int    ok = -1;
	int    valid_data = 0;
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
				if(psn_pack.Kinds.lsearch(acs_rec.ObjGroup))
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

int PPViewBill::AddBySCard(PPID * pID)
{
	int    ok = -1, r = 0;
	const  int is_draft = BIN(Filt.Flags & BillFilt::fDraftOnly);
	int    sc_num_ret = 0;
	PPID   op_id = 0;
	PPObjSCard sc_obj;
	PPSCardPacket sc_pack;
	CCheckTbl::Rec cc_rec;
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

int PPViewBill::Browse(int modeless)
{
	int    ok = 1;
	const  PPConfig & r_cfg = LConfig;
	const  PPID save_loc   = r_cfg.Location;
	PPID   single_loc_id = LocList_.getSingle();
	Filt.Period.Actualize(ZERODATE);
	THROW((Filt.Flags & BillFilt::fDebtOnly) || AdjustPeriodToRights(Filt.Period, 0));
	if(single_loc_id && single_loc_id != r_cfg.Location)
		DS.SetLocation(single_loc_id);
	ok = PPView::Browse(modeless);
	CATCHZOK
	if(!modeless)
		DS.SetLocation(save_loc);
	return ok;
}

int PPViewBill::SelectBillListForm(uint * pForm, int * pIsExt, IterOrder * pOrder)
{
	// @todo добавить сортировка по дате и номеру документа 
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

int PPViewBill::Transmit(PPID id, int transmitKind)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(transmitKind == 0) {
		if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
			BillViewItem item;
			const PPIDArray & rary = param.DestDBDivList.Get();
			PPObjIDArray objid_ary;
			PPWaitStart();
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
	PPWaitStop();
	return ok;
}

/*void LogObjToTransmit(PPID objType, PPID objID, const char * pName)
{
	SString msg_buf;
}*/

// static
int PPViewBill::TransmitByFilt(const BillFilt * pFilt, const ObjTransmitParam * pParam)
{
	int    ok = -1, r = 1;
	uint   val = 0;
	BillFilt filt;
	ObjTransmitParam param;
	filt.SetupBrowseBillsType(filt.Bbt = bbtUndef);
	RVALUEPTR(filt, pFilt);
	RVALUEPTR(param, pParam);
	if(!pFilt && (r = SelectorDialog(DLG_BBTSEL, CTL_BBTSEL_TYPE, &val)) > 0)
		filt.SetupBrowseBillsType(filt.Bbt = static_cast<BrowseBillsType>(val));
	if((pFilt && pParam) || (r > 0 && ObjTransmDialogExt(DLG_OBJTRANSM, PPVIEW_BILL, &param, &filt) > 0)) {
		BillViewItem item;
		PPViewBill   view;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		THROW(view.InitLocal(&filt));
		for(view.InitIteration(OrdByDefault); view.NextIteration(&item) > 0; PPWaitPercent(view.GetCounter())) {
			objid_ary.Add(PPOBJ_BILL, item.ID);
		}
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}
//
// Descr: Блок данных для печати итогов выборки документов, передаваемый объекту PPALDD_BillTotal
//
struct BillTotalPrintData {
	const BillTotal * P_Total;
	const BillFilt  * P_Filt;
};

int PPViewBill::PrintTotal(const BillTotal * pTotal)
{
	BillTotalPrintData btpd;
	btpd.P_Total = pTotal;
	btpd.P_Filt = &Filt;
	return PPAlddPrint(REPORT_BILLTOTAL, PView(&btpd), 0);
}

// AHTOXA {
// @<<PPALDD_BillInfoList::NextIteration
int PPViewBill::GetPacket(PPID billID, PPBillPacket * pPack) const
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

int PPViewBill::PrintBillInfoList()
{
	BillInfoListPrintData bilpd(this, 0);
	PView  pv(&bilpd);
	int    ok = PPAlddPrint(REPORT_BILLINFOLIST, pv, 0);
	ZDELETE(static_cast<BillInfoListPrintData *>(pv.Ptr)->P_Pack);
	return ok;
}
// } AHTOXA

int PPViewBill::Print()
{
	int    ok = 1;
	int    reply;
	int    ext = 0;
	uint   form;
	IterOrder order = OrdByDefault;
	if(Filt.SortOrder == BillFilt::ordByDate)
		order = OrdByDate;
	else if(Filt.SortOrder == BillFilt::ordByCode)
		order = OrdByCode;
	else if(Filt.SortOrder == BillFilt::ordByDateCode) // @v11.0.11
		order = OrdByDateCode;
	else if(Filt.SortOrder == BillFilt::ordByObject)
		order = OrdByObjectName;
	THROW(reply = SelectBillListForm(&form, &ext, &order));
	if(reply > 0) {
		PPReportEnv env;
		env.Sort = order;
		PPAlddPrint(form, PView(this), &env);
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewBill::UpdateTempTable(PPID id)
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
	else {
		rec.Clear();
	}
	if(P_TempOrd && oneof3(Filt.SortOrder, BillFilt::ordByCode, BillFilt::ordByObject, BillFilt::ordByDateCode)) { // @v11.0.11 BillFilt::ordByDateCode
		BillTbl::Key0 k0;
		k0.ID = id;
		if(id) {
			if(id_found) {
				TempOrderTbl::Rec ord_rec;
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

int PPViewBill::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	int    ok = -1, update = 0;
	if(pEv) {
		if(kind == PPAdviseBlock::evBillChanged) {
			if(pEv->ObjID && oneof4(pEv->Action, PPACN_UPDBILL, PPACN_RMVBILL, PPACN_BILLSTATUSUPD, PPACN_UPDBILLFREIGHT))
				P_BObj->Dirty(pEv->ObjID);
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
				const  PPID bill_id = UpdateBillList.get(i);
				UpdateTempTable(bill_id);
			}
			pBrw->Refresh();
		}
		else
			pBrw->Update();
		UpdateBillList.clear();
	}
	return ok;
}

/*virtual*/int PPViewBill::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = (ppvCmd != PPVCMD_DETAIL && ppvCmd != PPVCMD_PRINT) ? PPView::ProcessCommand(ppvCmd, pHdr, pBrw) : -2;
	const  uint options = (Filt.Flags & BillFilt::fAsSelector) ? 0 : (OLW_CANEDIT|OLW_CANINSERT|OLW_CANDELETE);
	int    update = 0;
	PPID   id = 0;
	if(ok == -2) {
		const PPConfig & r_cfg = LConfig;
		BillTbl::Rec bill_rec;
		BrwHdr hdr;
		if(pHdr && ppvCmd != PPVCMD_INPUTCHAR && ppvCmd != PPVCMD_RECEIVEDFOCUS)
			hdr = *static_cast<const PPViewBill::BrwHdr *>(pHdr);
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
						if(CheckIDForFilt(new_bill_id, 0)) {
							// Если вновь созданный документ попадает в выборку, то
							// следующее присвоение обеспечит перевод курсора на этот документ.
							id = new_bill_id;
						}
						P_BObj->Dirty(hdr.ID); // @v12.2.1
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
				if(P_BObj->P_OpObj)
					ok = (P_BObj->Search(hdr.ID, &bill_rec) > 0 && P_BObj->P_OpObj->Edit(&bill_rec.OpID, 0) == cmOK) ? 1 : -1;
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
			case PPVCMD_WROFFINVENTORY:
				ok = -1;
				if(hdr.ID && PPMessage(mfConf|mfYesNo, PPCFM_INVWRITEOFF) == cmYes) {
					PPWaitStart();
					if(P_BObj->ConvertInventory(hdr.ID))
						ok = 1;
					else
						PPError();
					PPWaitStop();
				}
				break;
			case PPVCMD_INPUTCHAR:
				if(PTR8C(pHdr)[0] == kbCtrlX) {
					if(State & stCtrlX) {
						ok = UpdateAttributes();
						State &= ~stCtrlX;
					}
					else
						State |= stCtrlX;
				}
				/*else if(PTR8C(pHdr)[0] == '/') {
					if(State & stTagPreKey) {
						//
					}
					else
						State |= stTagPreKey;
				}*/
				else
					State &= ~stCtrlX;
				break;
			case PPVCMD_TRFRANLZ:
				ok = -1;
				{
					TrfrAnlzFilt filt;
					PPViewTrfrAnlz v_trfr;
					if(v_trfr.EditBaseFilt(&filt) > 0) {
						PPWaitStart();
						BillViewItem bitem;
						for(InitIteration(OrdByID); NextIteration(&bitem) > 0;)
							filt.RcptBillList.Add(bitem.ID);
						PPWaitStop();
						ok = ::ViewTrfrAnlz(&filt);
					}
				}
				break;
			case PPVCMD_PAYMENT:
				if((ok = ViewPayments(hdr.ID, LinkedBillFilt::lkPayments)) > 0)
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
			case PPVCMD_ATTACHBILLTOBILL:  ok = AttachBill(hdr.ID, pBrw); break;
			case PPVCMD_TRANSMIT:          ok = Transmit(hdr.ID, 0); break;
			case PPVCMD_TRANSMITCHARRY:    ok = Transmit(hdr.ID, 1); break;
			case PPVCMD_EXPORT:            ok = ExportGoodsBill(0, 0); break;
			case PPVCMD_PRINT:             ok = PrintBill(hdr.ID); break;
			case PPVCMD_PRINTLIST:         ok = Print(); break;
			case PPVCMD_PRINTINFOLIST:     ok = PrintBillInfoList(); break;
			case PPVCMD_PRINTALLBILLS:     ok = PrintAllBills(); break;
			case PPVCMD_POSPRINTBYBILL:    ok = P_BObj->PosPrintByBill(hdr.ID); break;
			case PPVCMD_CREATEMRPTAB:
				if(hdr.ID)
					ok = CreateMrpTab(hdr.ID);
				break;
			case PPVCMD_CHECKSTAT:
				if(r_cfg.Cash) {
					PPCashMachine * cm = PPCashMachine::CreateInstance(r_cfg.Cash);
					if(cm) {
						ok = cm->SyncViewSessionStat(0);
						delete cm;
					}
				}
				break;
			case PPVCMD_REFRESH:
				ok = update = 1;
				break;
			case PPVCMD_RECEIVEDFOCUS:
				if(!pHdr) {
					const  PPID single_loc_id = LocList_.getSingle();
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
			case PPVCMD_QUICKTAGEDIT: // @v11.2.8
				// В этой команде указатель pHdr занят под список идентификаторов тегов, соответствующих нажатой клавише
				// В связи с этим текущий элемент таблицы придется получить явным вызовом pBrw->getCurItem()
				//
				{
					const BrwHdr * p_row = static_cast<const BrwHdr *>(pBrw->getCurItem());
					ok = PPView::Helper_ProcessQuickTagEdit(PPObjID(PPOBJ_BILL, p_row ? p_row->ID : 0), pHdr/*(LongArray *)*/);
				}
				break;
			case PPVCMD_TAGS:
				if(hdr.ID)
					ok = EditObjTagValList(PPOBJ_BILL, hdr.ID, 0);
				break;
			case PPVCMD_TAGSALL:
				// @v11.1.9 {
				ok = -1;
				{
					const  PPID obj_type = PPOBJ_BILL;
					int    update_mode = ObjTagList::mumAdd;
					ObjTagList common_tag_list;
					common_tag_list.Oid.Obj = obj_type;
					if(EditObjTagValUpdateList(&common_tag_list, 0, &update_mode) > 0 && common_tag_list.GetCount()) {
						BillViewItem item;
						ObjTagCore & r_ot = PPRef->Ot;
						PPTransaction tra(1);
						THROW(tra);
						for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
							ObjTagList local_tag_list;
							THROW(r_ot.GetList(obj_type, item.ID, &local_tag_list));
							if(local_tag_list.Merge(common_tag_list, update_mode) > 0) {
								THROW(r_ot.PutList(obj_type, item.ID, &local_tag_list, 0));
							}
							PPWaitPercent(GetCounter());
						}
						THROW(tra.Commit());
					}
				}
				// } @v11.1.9 
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
								memos.ReplaceStr(PPConst::P_ObjMemoDelim, "\n", 0);
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
		if(IsTempTblNeeded()) {
			ok = UpdateTempTable(id);
			update = 2;
		}
		if(update == 2 && pBrw) {
			pBrw->Update();
			if(CheckIDForFilt(id, 0)) {
				pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
			}
		}
	}
	CATCHZOKPPERR
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}
//
//
//
int STDCALL ViewGoodsBills(BillFilt * pFilt, bool asModeless)
{
	int    ok = -1;
	int    r = 0;
	int    view_in_use = 0;
	bool   modeless = GetModelessStatus(asModeless);
	PPView * p_v = 0;
	PPBaseFilt * p_flt = 0;
    PPViewBrowser * p_prev_win = 0;
	THROW(PPCheckDatabaseChain());
	THROW(PPView::CreateInstance(PPVIEW_BILL, &p_v));
	THROW(p_flt = p_v->CreateFilt(PPView::GetDescriptionExtra(p_v->GetViewId())));
	if(modeless)
		p_prev_win = static_cast<PPViewBrowser *>(PPFindLastBrowser());
	if(pFilt) {
		THROW(p_flt->Copy(pFilt, 1));
	}
	else if(p_prev_win) {
		THROW(p_flt->Copy(p_prev_win->P_View->GetBaseFilt(), 1));
	}
	while(pFilt || p_v->EditBaseFilt(p_flt) > 0) {
		PPWaitStart();
		if(static_cast<const BillFilt *>(p_flt)->Flags & BillFilt::fAsSelector)
			modeless = false;
		THROW(p_v->Init_(p_flt));
		PPCloseBrowser(p_prev_win);
		THROW(r = p_v->Browse(modeless));
		if(modeless || pFilt) {
			view_in_use = 1;
			break;
		}
	}
	if(!modeless && r > 0) {
		pFilt->Sel = static_cast<const BillFilt *>(p_v->GetBaseFilt())->Sel;
		ok = 1;
	}
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use)
		delete p_v;
	delete p_flt;
	return ok;
}

int STDCALL ViewBillsByPool(PPID poolType, PPID poolOwnerID)
{
	int    ok = -1;
	const  bool pool_bytype = (poolOwnerID && oneof5(poolType, PPASS_OPBILLPOOL, PPASS_TODOBILLPOOL, PPASS_PRJBILLPOOL, PPASS_PRJPHASEBILLPOOL, PPASS_TSESSBILLPOOL));
	PPBaseFilt * p_flt = 0;
	PPView * p_v = 0;
	if(poolType && poolOwnerID) {
		THROW(PPView::CreateInstance(PPVIEW_BILL, &p_v));
		THROW(p_flt = p_v->CreateFilt(PPView::GetDescriptionExtra(p_v->GetViewId())));
		{
			BillFilt * p_bfilt = static_cast<BillFilt *>(p_flt);
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

int STDCALL BrowseBills(BrowseBillsType bbt)
{
	int    ok = -1;
	THROW(PPCheckDatabaseChain());
	{
		BillFilt::FiltExtraParam p(1, bbt);
		ok = PPView::Execute(PPVIEW_BILL, 0, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
	}
	CATCHZOKPPERR
	return ok;
}
//
// Implementation of PPALDD_GoodsBillBase
//
struct DlGoodsBillBaseBlock {
	DlGoodsBillBaseBlock(PPBillPacket * pPack) : P_Pack(pPack), Iter(pPack, 0, 0)
	{
		ResetRow();
	}
	SString & UnifySymb(const char * pSymb) const
	{
		SString & r_buf = SLS.AcquireRvlStr();
		(r_buf = pSymb).Strip().ToLower().ReplaceStr("-", 0, 0).ReplaceStr("_", 0, 0).ReplaceStr(" ", 0, 0);
		return r_buf;
	}
	double GetValueBySymb(const char * pSymb) const
	{
		SString & r_symb = UnifySymb(pSymb);
		if(r_symb == "qtty" || r_symb == "quantity" || r_symb == "qty") return Qtty;
		else if(r_symb == "oldqtty") return OldQtty;
		else if(r_symb == "cost") return Cost;
		else if(r_symb == "price") return Price;
		else if(r_symb == "discount") return Discount;
		else if(r_symb == "curprice") return CurPrice;
		else if(r_symb == "cursum") return CurSum;
		else if(r_symb == "mainprice") return MainPrice;
		else if(r_symb == "mainsum") return MainSum;
		else if(r_symb == "mainsumwovat") return MainSumWoVat;
		else if(r_symb == "extprice") return ExtPrice;
		else if(r_symb == "vatrate") return VATRate;
		else if(r_symb == "vatsum") return VATSum;
		else if(r_symb == "excrate") return ExcRate;
		else if(r_symb == "excsum") return ExcSum;
		else if(r_symb == "strate") return STRate;
		else if(r_symb == "stsum") return STSum;
		else if(r_symb == "newprice") return NewPrice;
		else if(r_symb == "newpricewovat") return NewPriceWoVat;
		else if(r_symb == "newcost") return NewCost;
		else if(r_symb == "newcostwovat") return NewCostWoVat;
		else if(r_symb == "newpricesum") return NewPriceSum;
		else if(r_symb == "newcostsum") return NewCostSum;
		else if(r_symb == "newpricesumwovat") return NewPriceSumWoVat;
		else if(r_symb == "newcostsumwovat") return NewCostSumWoVat;
		else if(r_symb == "oldprice") return OldPrice;
		else if(r_symb == "oldpricewovat") return OldPriceWoVat;
		else if(r_symb == "oldcost") return OldCost;
		else if(r_symb == "oldcostwovat") return OldCostWoVat;
		else if(r_symb == "oldpricesum") return OldPriceSum;
		else if(r_symb == "oldcostsum") return OldCostSum;
		else if(r_symb == "oldpricesumwovat") return OldPriceSumWoVat;
		else if(r_symb == "oldcostsumwovat") return OldCostSumWoVat;
		else if(r_symb == "vatsumoldcost") return VatSum_OldCost;
		else if(r_symb == "vatsumnewcost") return VatSum_NewCost;
		else if(r_symb == "vatsumoldprice") return VatSum_OldPrice;
		else if(r_symb == "vatsumnewprice") return VatSum_NewPrice;
		else if(r_symb == "oldpricevat") return OldPriceVat;
		else if(r_symb == "oldcostvat") return OldCostVat;
		else if(r_symb == "newpricevat") return NewPriceVat;
		else if(r_symb == "newcostvat") return NewCostVat;
		else return 0.0;
	}
	void ResetRow()
	{
		Qtty = 0.0;
		OldQtty = 0.0;
		Cost = 0.0;
		Price = 0.0;
		Discount = 0.0;
		CurPrice = 0.0;
		CurSum = 0.0;
		MainPrice = 0.0;
		MainSum = 0.0;
		MainSumWoVat = 0.0;
		ExtPrice = 0.0;
		VATRate = 0.0;
		VATSum = 0.0;
		ExcRate = 0.0;
		ExcSum = 0.0;
		STRate = 0.0;
		STSum = 0.0;
		NewPrice = 0.0;
		NewCost = 0.0;
		NewPriceWoVat = 0.0;
		NewCostWoVat = 0.0;
		NewPriceSum = 0.0;
		NewCostSum = 0.0;
		NewPriceSumWoVat = 0.0;
		NewCostSumWoVat = 0.0;
		OldPriceVat = 0.0;
		OldCostVat = 0.0;
		NewPriceVat = 0.0;
		NewCostVat = 0.0;
		OldPrice = 0.0;
		OldCost = 0.0;
		OldPriceWoVat = 0.0;
		OldCostWoVat = 0.0;
		OldPriceSum = 0.0;
		OldCostSum = 0.0;
		OldPriceSumWoVat = 0.0;
		OldCostSumWoVat = 0.0;
		VatSum_OldCost = 0.0;
		VatSum_NewCost = 0.0;
		VatSum_OldPrice = 0.0;
		VatSum_NewPrice = 0.0;
	}
	PPBillPacket * P_Pack; // @notowned
	TiIter Iter;
	PPBillPacket::TiItemExt Item; // Последняя сканированная итератором строка (используется функциями)

	double Qtty;
	double OldQtty;
	double Cost;
	double Price;
	double Discount;
	double CurPrice;
	double CurSum;
	double MainPrice;
	double MainSum;
	double MainSumWoVat;
	double ExtPrice;
	double VATRate;
	double VATSum;
	double ExcRate;
	double ExcSum;
	double STRate;
	double STSum;
	double NewPrice;
	double NewCost;
	double NewPriceWoVat;
	double NewCostWoVat;
	double NewPriceSum;
	double NewCostSum;
	double NewPriceSumWoVat;
	double NewCostSumWoVat;
	double OldPrice;
	double OldCost;
	double OldPriceVat;
	double OldCostVat;
	double NewPriceVat;
	double NewCostVat;
	double OldPriceWoVat;
	double OldCostWoVat;
	double OldPriceSum;
	double OldCostSum;
	double OldPriceSumWoVat;
	double OldCostSumWoVat;
	double VatSum_OldCost;
	double VatSum_NewCost;
	double VatSum_OldPrice;
	double VatSum_NewPrice;
};

PPALDD_CONSTRUCTOR(GoodsBillBase)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBillBase)
{
	DlGoodsBillBaseBlock * p_extra = static_cast<DlGoodsBillBaseBlock *>(Extra[0].Ptr);
	if(p_extra && p_extra->P_Pack)
		p_extra->P_Pack->RemoveVirtualTItems();
	Destroy();
}

int PPALDD_GoodsBillBase::InitData(PPFilt & rFilt, long rsrv)
{
	DlGoodsBillBaseBlock * p_extra = new DlGoodsBillBaseBlock(static_cast<PPBillPacket *>(rFilt.Ptr));
	Extra[0].Ptr = p_extra;
	PPBillPacket * p_pack = p_extra->P_Pack;
	PPOprKind op_rec;
	const  long bill_f = p_pack->Rec.Flags;
	const  PPID optype = p_pack->OpTypeID;
	PPID   main_org_id = 0;
	BillTotalData total_data;
	PPObjPerson psn_obj;
	int    exclude_vat = 0;
	GetOpData(p_pack->Rec.OpID, &op_rec);
	const  PPID object_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? p_pack->Rec.Object2 : p_pack->Rec.Object;
	const  PPID psn_id = ObjectToPerson(object_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(psn_id && psn_obj.GetRelPersonList(psn_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
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
	// @v11.1.12 strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	strip(STRNSCPY(H.Memo, p_pack->SMemo)); // @v11.1.12
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
				H.RcvrLocID = (bill_f & BILLF_FREIGHT && p_pack->P_Freight) ? p_pack->P_Freight->DlvrAddrID__ : 0;
				if(parent_person_id) {
					H.RcvrID = parent_person_id;
					H.ConsigneeReq = psn_id;
				}
				else {
					H.RcvrID = psn_id;
					H.ConsigneeReq = H.RcvrID;
				}
				H.RcvrReq = H.RcvrID;
			}
	}
	else if(optype == PPOPT_DRAFTRECEIPT || (bill_f & BILLF_GRECEIPT) ||
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
				H.ConsignorReq = psn_id;
			}
			else {
				H.DlvrID = psn_id;
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
		const  PPID suppl_person_id = (optype == PPOPT_GOODSRETURN) ? H.RcvrID : H.DlvrID;
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
		p_pack->CalcTotal(total_data, btc_flags);
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
	{
		PTR32(H.TxtManualDscnt)[0] = 0;
		if(!CheckOpPrnFlags(p_pack->Rec.OpID, OPKF_PRT_NDISCNT)) {
			int    isdis = 0;
			SString val;
			SString temp_buf;
			const  int    re = BIN(p_pack->Rec.Flags & BILLF_RMVEXCISE);
			const  int    ne = (CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re;
			const  double dis = p_pack->Amounts.Get(PPAMT_MANDIS, p_pack->Rec.CurID);
			const  double pctdis = p_pack->Amounts.Get(PPAMT_PCTDIS, 0L/*@curID*/);
			if(dis != 0 || pctdis != 0) {
				PPLoadText(PPTXT_INCLDIS, temp_buf);
				if(pctdis != 0.0)
					temp_buf.Cat(pctdis, MKSFMTD(0, 1, 0)).Strip().CatChar('%');
				else
					temp_buf.Cat(dis, SFMT_MONEY);
				isdis = 1;
			}
			if(!ne && /*enableSTaxText*/(total_data.STax > 0)) {
				PPGetSubStr(PPTXT_INCLEXCISE, isdis, val);
				if(temp_buf.NotEmpty())
					temp_buf.Space();
			}
			STRNSCPY(H.TxtManualDscnt, temp_buf);
		}
	}
	return (DlRtm::InitData(rFilt, rsrv) > 0) ? 1 : -1;
}

int PPALDD_GoodsBillBase::InitIteration(PPIterID iterId, int sortId, long)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.nn = 0;
	DlGoodsBillBaseBlock * p_extra = static_cast<DlGoodsBillBaseBlock *>(Extra[0].Ptr);
	PPBillPacket * p_pack = p_extra->P_Pack;
	long   f = H.fMergeSameGoods ? (ETIEF_UNITEBYGOODS|ETIEF_DIFFBYPACK|ETIEF_DIFFBYQCERT|ETIEF_DIFFBYNETPRICE) : 0;
	PPID   filt_grp_id = 0;
	if(p_pack->ProcessFlags & PPBillPacket::pfPrintTareSaldo) {
		PPObjGoods goods_obj;
		if(goods_obj.GetConfig().TareGrpID) {
			filt_grp_id = goods_obj.GetConfig().TareGrpID;
			f |= ETIEF_SALDOFILTGRP;
		}
	}
	p_extra->Iter.Init(p_pack, f, filt_grp_id, static_cast<TiIter::Order>(H.RowOrder));
	return 1;
}

int PPALDD_GoodsBillBase::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	DlGoodsBillBaseBlock * p_extra = static_cast<DlGoodsBillBaseBlock *>(Extra[0].Ptr);
	PPBillPacket * p_pack = p_extra ? p_extra->P_Pack : 0;
	CALLPTRMEMB(p_extra, ResetRow());
	//
	// Возможны два алгоритма расчета налогов по объединенным строкам документа:
	// 1. Объединенная строка обсчитывается сама по себе, как единая
	// 2. Каждая из строк, включенных в объединенную, обсчитываются порознь, а результат складывается.
	//
	int   merge_line_tax_alg = 1;
	const  PPCommConfig & r_ccfg = CConfig;
	if(checkdate(r_ccfg._InvcMergeTaxCalcAlg2Since) && p_pack->Rec.Dt >= r_ccfg._InvcMergeTaxCalcAlg2Since) {
		merge_line_tax_alg = 2;
	}
	//
	SString temp_buf;
	PPObjBill * p_bobj = BillObj;
	ReceiptCore * p_rcpt = (p_bobj && p_bobj->trfr) ? &p_bobj->trfr->Rcpt : 0;
	PPTransferItem * p_ti;
	PPTransferItem temp_ti;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	PPObjQuotKind qk_obj;
	PPQuotKindPacket qk_pack;
	double ext_price = 0.0;
	double upp = 0.0; // Емкость упаковки
	int    tiamt;
	int    price_chng = 1;
	uint   n = static_cast<uint>(I.nn);
	const  PPID qk_id = p_pack->Ext.ExtPriceQuotKindID;
	const  long exclude_tax_flags = H.fSupplIsVatExempt ? GTAXVF_VAT : 0L;
	const  bool extprice_by_base = (qk_obj.Fetch(qk_id, &qk_pack) > 0 && qk_pack.Rec.Flags & QUOTKF_EXTPRICEBYBASE);
	int    treat_as_unlim = 0;
	do {
		price_chng = 1; // Цена изменилась по отношению к предыдущему лоту. Если не установлен флаг pfPrintChangedPriceOnly, то игнорируется.
		treat_as_unlim = 0;
		if(p_pack->EnumTItemsExt(&p_extra->Iter, &temp_ti, &p_extra->Item/*tiie*/) > 0) {
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
		if(gto_assc.GetListByGoods(goods_rec.ID, loc_list) > 0) {
			for(uint j = 0; temp_buf.IsEmpty() && j < loc_list.getCount(); j++) {
				const  PPID loc_id = loc_list.get(j);
				if(loc_obj.BelongTo(loc_id, p_pack->Rec.LocID, &temp_buf))
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
		tiamt = /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(p_pack->Rec);
		if(tiamt == TIAMT_COST)
			I.MainPrice = p_ti->Cost;
		else
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
		ReceiptTbl::Rec lot_rec;
		if(goods_obj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0 && gse.Package > 0.0)
			upp = gse.Package;
		else if(p_pack->IsDraft() && p_rcpt && p_rcpt->GetLastLot(p_ti->GoodsID, p_pack->Rec.LocID, p_pack->Rec.Dt, &lot_rec) > 0)
			upp = lot_rec.UnitPerPack;
	}
	I.UnitsPerPack = upp;
	I.FullPack = 0;
	if(upp > 0.0)
		I.FullPack = floor(fabs(I.Qtty) / upp);
	{
		I.VATRate = 0.0;
		I.VATSum = 0.0;
		I.ExcRate = 0.0;
		I.ExcSum = 0.0;
		I.STRate = 0.0;
		I.STSum = 0.0;
		if(merge_line_tax_alg == 2 && p_extra->Item.MergePosList.getCount()) {
			double prev_vat_rate = 0.0;
			assert(p_extra->Item.MergePosList.getCount());
			for(uint i = 0; i < p_extra->Item.MergePosList.getCount(); i++) {
				uint pos_local = p_extra->Item.MergePosList.get(i);
				assert(pos_local < p_pack->GetTCount());
				const PPTransferItem & r_ti_local = p_pack->TI(pos_local);
				const double qtty_local = fabs(r_ti_local.Qtty());
				GTaxVect gtv;
				gtv.CalcBPTI(*p_pack, r_ti_local, tiamt, exclude_tax_flags);
				I.VATRate = gtv.GetTaxRate(GTAX_VAT, 0);
				assert(i == 0 || I.VATRate == prev_vat_rate);
				prev_vat_rate = I.VATRate;
				I.VATSum  += gtv.GetValue(GTAXVF_VAT);
				I.ExcRate = gtv.GetTaxRate(GTAX_EXCISE, 0);
				I.ExcSum  += gtv.GetValue(GTAXVF_EXCISE);
				I.STRate  = gtv.GetTaxRate(GTAX_SALES, 0);
				if(tiamt == TIAMT_COST) {
					if(r_ti_local.Flags & PPTFR_COSTWOVAT)
						I.MainPrice += I.VATSum / qtty_local;
				}
				else if(r_ti_local.Flags & PPTFR_PRICEWOTAXES) {
					const double _a = gtv.GetValue(GTAXVF_BEFORETAXES) / qtty_local;
					if(i == 0)
						I.MainPrice = _a;
					else
						I.MainPrice += _a;
				}
				if(!(p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel)) {
					I.STSum += gtv.GetValue(GTAXVF_SALESTAX);
				}
			}
		}
		else {
			const PPTransferItem & r_ti_local = *p_ti;
			const double qtty_local = fabs(r_ti_local.Qtty());
			GTaxVect gtv;
			gtv.CalcBPTI(*p_pack, r_ti_local, tiamt, exclude_tax_flags);
			I.VATRate = gtv.GetTaxRate(GTAX_VAT, 0);
			I.VATSum  = gtv.GetValue(GTAXVF_VAT);
			I.ExcRate = gtv.GetTaxRate(GTAX_EXCISE, 0);
			I.ExcSum  = gtv.GetValue(GTAXVF_EXCISE);
			I.STRate  = gtv.GetTaxRate(GTAX_SALES, 0);
			if(tiamt == TIAMT_COST) {
				if(r_ti_local.Flags & PPTFR_COSTWOVAT)
					I.MainPrice += I.VATSum / qtty_local;
			}
			else if(r_ti_local.Flags & PPTFR_PRICEWOTAXES) {
				const double _a = gtv.GetValue(GTAXVF_BEFORETAXES) / qtty_local;
				I.MainPrice = _a;
			}
			if(!(p_pack->ProcessFlags & PPBillPacket::pfPrintPLabel)) {
				I.STSum = gtv.GetValue(GTAXVF_SALESTAX);
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
			p_pack->GetNextPLU(&p_extra->Iter, &plu, obj_assc_name);
			STRNSCPY(I.CQtty, obj_assc_name);
			I.STSum = plu;
		}
		else {
			QttyToStr(p_ti->Quantity_, upp, ((LConfig.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_COMPLPACK|QTTYF_FRACTION) : QTTYF_FRACTION), I.CQtty);
		}
	}
	if(p_pack->P_PckgList) {
		while(p_extra->Item.Pckg.Len() > 1 && isalpha(p_extra->Item.Pckg.Last()))
			p_extra->Item.Pckg.TrimRight();
		p_extra->Item.Pckg.CopyTo(I.CLB, sizeof(I.CLB));
	}
	else
		p_extra->Item.Clb.CopyTo(I.CLB, sizeof(I.CLB));
	{
		p_extra->Qtty = I.Qtty;
		p_extra->Cost = I.Cost;
		p_extra->Price = I.Price;
		p_extra->Discount = I.Discount;
		p_extra->CurPrice = I.CurPrice;
		p_extra->CurSum = I.CurSum;
		p_extra->VATRate = I.VATRate;
		p_extra->VATSum = I.VATSum;
		p_extra->ExcRate = I.ExcRate;
		p_extra->ExcSum = I.ExcSum;
		p_extra->ExtPrice = I.ExtPrice;
	}
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsBillBase::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	DlGoodsBillBaseBlock * p_extra = static_cast<DlGoodsBillBaseBlock *>(Extra[0].Ptr);
	PPBillPacket * p_pack = p_extra ? p_extra->P_Pack : 0;
	if(pF->Name == "?CalcInSaldo") {
		double saldo = 0.0;
		if(p_pack) {
			const  PPID goods_id = _ARG_INT(1);
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
				const  PPID dlvr_loc_id = p_pack->GetDlvrAddrID();
				p_bobj->GetGoodsSaldo(goods_id, H.ObjectID, dlvr_loc_id, H.Dt, oprno, &saldo, 0);
			}
		}
		_RET_DBL = saldo;
	}
	else if(pF->Name == "?UnlimGoodsOnly") {
		_RET_INT = BIN(p_pack && p_pack->ProcessFlags & PPBillPacket::pfAllGoodsUnlim);
	}
	else if(pF->Name == "?GetRowAmount") { // iterator
		_RET_DBL = p_extra ? p_extra->GetValueBySymb(_ARG_STR(1)) : 0.0;
	}
}
//
// Implementation of PPALDD_GoodsBillDispose
//
struct DlGoodsBillDisposeBlock {
	DlGoodsBillDisposeBlock(void * ptr) : P_Pack(static_cast<PPBillPacket *>(ptr))
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
	DlGoodsBillDisposeBlock * p_blk = static_cast<DlGoodsBillDisposeBlock *>(Extra[0].Ptr);
	delete p_blk;
	Destroy();
}

int PPALDD_GoodsBillDispose::InitData(PPFilt & rFilt, long rsrv)
{
	DlGoodsBillDisposeBlock * p_blk = new DlGoodsBillDisposeBlock(rFilt.Ptr);
	Extra[0].Ptr = p_blk;
	PPOprKind op_rec;
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(p_blk->P_Pack);
	const  long bill_f = p_pack->Rec.Flags;
	const  PPID optype = p_pack->OpTypeID;
	PPID   main_org_id = 0;
	BillTotalData total_data;
	PPObjPerson psn_obj;
	int    exclude_vat = 0;
	GetOpData(p_pack->Rec.OpID, &op_rec);
	const  PPID ar_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? p_pack->Rec.Object2 : p_pack->Rec.Object;
	const  PPID psn_id = ObjectToPerson(ar_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(psn_id && psn_obj.GetRelPersonList(psn_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
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
	// @v11.1.12 strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	strip(STRNSCPY(H.Memo, p_pack->SMemo)); // @v11.1.12
	H.Dt = (bill_f & BILLF_PRINTINVOICE && p_pack->Ext.InvoiceDate) ? p_pack->Ext.InvoiceDate : p_pack->Rec.Dt;
	H.OprKindID  = p_pack->Rec.OpID;
	H.ObjectID   = ar_id;
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
		if(ar_id)
			if(PPObjLocation::ObjToWarehouse(ar_id)) {
				H.RcvrID    = main_org_id;
				H.RcvrReq   = main_org_id;
				H.RcvrLocID = PPObjLocation::ObjToWarehouse(ar_id);
				H.ConsigneeReq = main_org_id;
			}
			else {
				H.RcvrLocID = (bill_f & BILLF_FREIGHT && p_pack->P_Freight) ? p_pack->P_Freight->DlvrAddrID__ : 0;
				if(parent_person_id) {
					H.RcvrID = parent_person_id;
					H.ConsigneeReq = psn_id;
				}
				else {
					H.RcvrID = psn_id;
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
		if(ar_id) {
			if(IsIntrOp(p_pack->Rec.OpID) == INTRRCPT) {
				H.DlvrID = main_org_id;
				H.ConsignorReq = H.DlvrID;
			}
			else if(parent_person_id) {
				H.DlvrID = parent_person_id;
				H.ConsignorReq = psn_id;
			}
			else {
				H.DlvrID = psn_id;
				H.ConsignorReq = H.DlvrID;
			}
			H.DlvrReq = H.DlvrID;
		}
		if(H.DlvrID == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(ar_id);
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
	p_pack->CalcTotal(total_data, btc_flags);
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
	DlGoodsBillDisposeBlock * p_blk = static_cast<DlGoodsBillDisposeBlock *>(Extra[0].Ptr);
	PPBillPacket * p_pack = p_blk->P_Pack;
	p_pack->InitExtTIter(ETIEF_DISPOSE, 0, static_cast<TiIter::Order>(H.RowOrder));
	return 1;
}

int PPALDD_GoodsBillDispose::NextIteration(long iterId)
{
	IterProlog(iterId, 0);
	//
	DlGoodsBillDisposeBlock * p_blk = static_cast<DlGoodsBillDisposeBlock *>(Extra[0].Ptr);
	PPBillPacket * p_pack = p_blk->P_Pack;
	//
	const PPConfig & r_cfg = LConfig;
	PPTransferItem * p_ti, temp_ti;
	PPBillPacket::TiItemExt tiie;
	GTaxVect gtv;
	SString temp_buf;
	double ext_price = 0.0;
	double upp = 0.0; // Емкость упаковки
	long   exclude_tax_flags = H.fSupplIsVatExempt ? GTAXVF_VAT : 0L;
	int    tiamt;
	uint   n = static_cast<uint>(I.nn);
	PPObjGoods goods_obj;
	PPObjQuotKind qk_obj;
	PPQuotKindPacket qk_pack;
	const  PPID qk_id = p_pack->Ext.ExtPriceQuotKindID;
	const  bool extprice_by_base = (qk_obj.Fetch(qk_id, &qk_pack) > 0 && qk_pack.Rec.Flags & QUOTKF_EXTPRICEBYBASE);
	do {
		if(p_pack->EnumTItemsExt(0, &temp_ti, &tiie) > 0) {
			n++;
			p_ti = &temp_ti;
			if(qk_id) {
				const double base = extprice_by_base ? p_ti->Price : p_ti->NetPrice();
				const QuotIdent qi(p_ti->LocID, qk_id, p_ti->CurID, p_pack->Rec.Object);
				goods_obj.GetQuotExt(p_ti->GoodsID, qi, p_ti->Cost, base, &ext_price, 1);
			}
		}
		else
			return -1;
	} while(0);
	I.nn      = n;
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
		tiamt = /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(p_pack->Rec);
		I.MainPrice = p_ti->NetPrice();
	}
	I.ExtPrice     = ext_price;
	I.Qtty = p_ti->Qtty();
	upp = p_ti->UnitPerPack;
	if(upp <= 0.0) {
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
			upp = gse.Package;
	}
	if(upp <= 0.0 && p_pack->IsDraft()) {
		PPObjBill * p_bobj = BillObj;
		ReceiptCore * p_rcpt = (p_bobj && p_bobj->trfr) ? &p_bobj->trfr->Rcpt : 0;
		ReceiptTbl::Rec lot_rec;
		if(p_rcpt && p_rcpt->GetLastLot(p_ti->GoodsID, p_pack->Rec.LocID, p_pack->Rec.Dt, &lot_rec) > 0)
			upp = lot_rec.UnitPerPack;
	}
	I.UnitsPerPack = upp;
	I.FullPack = 0;
	if(upp > 0.0)
		I.FullPack = static_cast<long>(fabs(I.Qtty) / upp);
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
	{
		const long qtsf = ((r_cfg.Flags & CFGFLG_USEPACKAGE) ? MKSFMT(0, QTTYF_COMPLPACK | QTTYF_FRACTION) : QTTYF_FRACTION);
		QttyToStr(fabs(p_ti->Qtty()),     upp, qtsf, I.CQtty);
		QttyToStr(fabs(tiie.LctRec.Qtty), upp, qtsf, I.CDispQtty);
	}
	return DlRtm::NextIteration(iterId);
}
//
// Implementation of PPALDD_Bill
//
struct DL600_BillExt {
	DL600_BillExt(BillCore * pT) : P_Bill(pT), CrEventSurID(0)
	{
	}
	BillCore * P_Bill;
	long   CrEventSurID;
	BillTbl::Rec Rec;
	SString SMemo; // @v11.1.12
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
	delete static_cast<DL600_BillExt *>(Extra[0].Ptr);
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
		DL600_BillExt * p_ext = static_cast<DL600_BillExt *>(Extra[0].Ptr);
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
			H.ID = rec.ID;
			H.ArticleID  = rec.Object;
			H.Object2ID  = rec.Object2;
			H.LocID      = rec.LocID;
			H.OprKindID  = rec.OpID;
			H.LinkBillID = rec.LinkBillID;
			H.UserID     = rec.UserID;
			STRNSCPY(H.Code, rec.Code);
			p_billcore->GetItemMemo(rec.ID, p_ext->SMemo);
			STRNSCPY(H.Memo, p_ext->SMemo);
			H.Dt       = rec.Dt;
			H.CurID    = rec.CurID;
			H.StatusID = rec.StatusID;
			H.Flags    = rec.Flags;
			H.fNeedPayment = BIN(rec.Flags & BILLF_NEEDPAYMENT);
			H.fPayout      = BIN(rec.Flags & BILLF_PAYOUT);
			H.fWL  = BIN(rec.Flags & BILLF_WHITELABEL);
			H.DueDate      = rec.DueDate;
			H.Amount   = BR2(rec.Amount);
			amtt_obj.GetTaxAmountIDs(tai, 1);
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
				H.DlvrLocID      = freight.DlvrAddrID__;
				H.IssueDate      = freight.IssueDate;
				H.ArrivalDate    = freight.ArrivalDate;
				H.CaptainID      = freight.CaptainID;
				H.Captain2ID     = freight.Captain2ID;
				H.VesselsAgentID = freight.AgentID;
				H.NmbOrigsBsL    = freight.NmbOrigsBsL;
			}
			if(rec.Flags & BILLF_RENT && p_billcore->GetRentCondition(rec.ID, &rent) > 0) {
				H.RcStart  = rent.Period.low;
				H.RcFinish = rent.Period.upp;
				H.RcFlags  = rent.Flags;
				H.fRcPctCharge = BIN(rent.Flags & PPRentCondition::fPercent);
				H.fRcClosed    = BIN(rent.Flags & PPRentCondition::fClosed);
				H.RcCycle      = rent.Cycle;
				H.RcDayOffs    = static_cast<int16>(rent.ChargeDayOffs);
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
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))

	DL600_BillExt * p_ext = static_cast<DL600_BillExt *>(Extra[0].Ptr);
	BillCore * p_billcore = p_ext->P_Bill;
	if(pF->Name == "?GetAmount") {
		double amt = 0.0;
		if(p_billcore) {
			PPObjAmountType amt_obj;
			PPID   amt_id = 0;
			if(amt_obj.SearchSymb(&amt_id, _ARG_STR(1)) > 0)
				p_billcore->GetAmount(H.ID, amt_id, 0/*curID*/, &amt);
			else {
				;
			}
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
			if(p_billcore->GetListOfOrdersByLading(H.ID, ord_bill_list) > 0)
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
			_RET_INT = pack.GetTCountI();
		else
			_RET_INT = 0;
	}
	else if(pF->Name == "?GetCreationEvent") {
		long   sur_id = static_cast<DL600_BillExt *>(Extra[0].Ptr)->CrEventSurID;
		if(!sur_id) {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			SysJournalTbl::Rec sj_rec;
			if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, H.ID, &sj_rec) > 0) {
				DS.GetTLA().SurIdList.Add(&sur_id, &sj_rec, sizeof(sj_rec));
				static_cast<DL600_BillExt *>(Extra[0].Ptr)->CrEventSurID = sur_id;
			}
			else
				static_cast<DL600_BillExt *>(Extra[0].Ptr)->CrEventSurID = -1;
		}
		else if(sur_id < 0)
			sur_id = 0;
		_RET_INT = sur_id;
	}
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_BILL, H.ID, _ARG_STR(1));
	}
	else if(pF->Name == "?GetMemo") {
		if(p_ext->Rec.ID) {
			// @v11.1.12 _RET_STR = p_ext->Rec.Memo;
			_RET_STR = p_ext->SMemo; // @v11.1.12
		}
		else
			_RET_STR.Z();
	}
	else if(pF->Name == "?GetUedIdent") { // @v12.2.10
		uint64 ued = UED::SetRaw_Oid(SObjID(PPOBJ_BILL, H.ID));
		_RET_STR.Z().CatHexUpper(ued);
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
	H.AssocType = rFilt.Ptr ? *static_cast<const int32 *>(rFilt.Ptr) : PPASS_OPBILLPOOL;
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
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))

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
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
	MEMSZERO(H);
	if(!CheckOpPrnFlags(p_pack->Rec.OpID, OPKF_PRT_NBILLN))
		STRNSCPY(H.Code, p_pack->Rec.Code);
	// @v11.1.12 strip(STRNSCPY(H.Memo, p_pack->Rec.Memo));
	strip(STRNSCPY(H.Memo, p_pack->SMemo)); // @v11.1.12
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
	const  int  accs_cost = BillObj->CheckRights(BILLRT_ACCSCOST);
	PPTransferItem * p_ti;
	for(uint i = 0; p_pack->EnumTItems(&i, &p_ti);) {
		double amount = 0.0;
		double vatsum = 0.0;
		double excisesum = 0.0;
		GTaxVect gtv;
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
			tiamt = /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(p_pack->Rec);
			amount = fabs(p_ti->CalcAmount(0));
		}
		gtv.CalcBPTI(*p_pack, *p_ti, tiamt, GTAXVF_SALESTAX);
		vatsum    = gtv.GetValue(GTAXVF_VAT);
		excisesum = gtv.GetValue(GTAXVF_EXCISE);
		if(p_ti->Flags & PPTFR_COSTWOVAT)
			if(p_pack->OutAmtType == 1 || (p_pack->OutAmtType != 2 && !(p_ti->Flags & PPTFR_SELLING)))
				amount += gtv.GetValue(GTAXVF_VAT);
		if(p_ti->Flags & PPTFR_PLUS) {
			H.ReceiptQtty   += fabs(p_ti->Quantity_);
			if(p_ti->UnitPerPack > 0.0)
				H.ReceiptPacks += static_cast<long>(fabs(p_ti->Qtty()) / p_ti->UnitPerPack);
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
				H.ExpendPacks += static_cast<long>(fabs(p_ti->Qtty()) / p_ti->UnitPerPack);
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
		PPBillPacket   * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
		PPTransferItem * p_ti = 0;
		GTaxVect gtv;
		long   exclude_tax_flags = GTAXVF_SALESTAX;
		int    tiamt;
		uint   n = static_cast<uint>(I.Iter_NN);
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
			tiamt = /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(p_pack->Rec);
			I.MainPrice = p_ti->NetPrice();
		}
		I.Qtty = p_ti->Qtty();
		I.UnitsPerPack = p_ti->UnitPerPack;
		I.FullPack = 0;
		if(p_ti->UnitPerPack > 0.0)
			I.FullPack = ffloori(fabs(I.Qtty) / p_ti->UnitPerPack);
		gtv.CalcBPTI(*p_pack, *p_ti, tiamt, exclude_tax_flags);
		I.VATRate = gtv.GetTaxRate(GTAX_VAT, 0);
		I.VATSum  = gtv.GetValue(GTAXVF_VAT);
		I.ExcRate = gtv.GetTaxRate(GTAX_EXCISE, 0);
		I.ExcSum  = gtv.GetValue(GTAXVF_EXCISE);
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

PPALDD_DESTRUCTOR(GoodsReval)
{
	Destroy();
}

int PPALDD_GoodsReval::InitData(PPFilt & rFilt, long rsrv)
{
	DlGoodsBillBaseBlock * p_extra = new DlGoodsBillBaseBlock(static_cast<PPBillPacket *>(rFilt.Ptr));
	Extra[0].Ptr = p_extra;
	PPBillPacket * p_pack = p_extra->P_Pack;
	BillTbl::Rec rec = p_pack->Rec;
	BillTotalData total_data;
	PPObjPerson psn_obj;
	PPOprKind op_rec;
	GetOpData(rec.OpID, &op_rec);
	PPID   main_org_id = 0;
	const  PPID ar_id = (op_rec.PrnFlags & OPKF_PRT_EXTOBJ2OBJ) ? rec.Object2 : rec.Object;
	const  PPID psn_id = ObjectToPerson(ar_id);
	PPID   parent_person_id = 0;
	{
		PPIDArray rel_list;
		if(psn_id && psn_obj.GetRelPersonList(psn_id, PPPSNRELTYP_AFFIL, 0, &rel_list) > 0)
			parent_person_id = rel_list.at(0);
	}
	p_pack->GetMainOrgID_(&main_org_id);
	int    exclude_vat = 0;
	MEMSZERO(H);
	H.BillID     = rec.ID;
	H.LinkBillID = rec.LinkBillID;
	H.Dt = rec.Dt;
	H.ArticleID  = ar_id;
	H.LocID      = rec.LocID;
	H.ExpendFlag = 0; // Приход (под вопросом, но переоценка и корректировка в большинстве случаев трактуются как приход)
	STRNSCPY(H.Code, rec.Code);
	// @v11.1.12 STRNSCPY(H.Memo, rec.Memo);
	STRNSCPY(H.Memo, p_pack->SMemo); // @v11.1.12
	{
		H.RcvrID    = main_org_id;
		H.RcvrReq   = main_org_id;
		H.RcvrLocID = rec.LocID;
		H.ConsigneeReq = main_org_id;
		if(ar_id) {
			if(IsIntrOp(rec.OpID) == INTRRCPT) {
				H.DlvrID = main_org_id;
				H.ConsignorReq = H.DlvrID;
			}
			else if(parent_person_id) {
				H.DlvrID = parent_person_id;
				H.ConsignorReq = psn_id;
			}
			else {
				H.DlvrID = psn_id;
				H.ConsignorReq = H.DlvrID;
			}
			H.DlvrReq = H.DlvrID;
		}
		if(H.DlvrID == main_org_id)
			H.DlvrLocID = PPObjLocation::ObjToWarehouse(ar_id);
	}
	{
		PersonTbl::Rec psn_rec;
		const  PPID suppl_person_id = H.DlvrID;
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
		p_pack->CalcTotal(total_data, btc_flags);
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
		DlGoodsBillBaseBlock * p_extra = static_cast<DlGoodsBillBaseBlock *>(Extra[0].Ptr);
		const PPBillPacket * p_pack = p_extra ? p_extra->P_Pack : 0;
		CALLPTRMEMB(p_extra, ResetRow());
		PPTransferItem * p_ti;
		uint   nn = static_cast<uint>(I.nn);
		if(p_pack && p_pack->EnumTItems(&nn, &p_ti)) {
			long   org_rbybill = 0;
			const  int correction_type = p_ti->IsCorrectionExp() ? -1 : (p_ti->IsCorrectionRcpt() ? +1 : 0);
			int    reval_assets_wo_vat = 1;
			double new_price = (correction_type == -1) ? (p_ti->Price-p_ti->Discount) : p_ti->Price;
			double new_cost  = p_ti->Cost;
			double old_price = (correction_type == -1) ? p_ti->RevalCost : ((correction_type == 0) ? p_ti->Discount : 0.0);
			double old_cost  = (correction_type == +1) ? p_ti->RevalCost : ((correction_type == 0) ? p_ti->Cost : 0.0);
			const double qtty      = correction_type ? fabs(p_ti->Quantity_) : fabs(p_ti->Rest_);
			const double old_qtty  = correction_type ? fabs(p_ti->QuotPrice) : fabs(qtty);
			double vatsum_oldcost = 0.0;
			double vatsum_newcost = 0.0;
			double vatsum_oldprice = 0.0;
			double vatsum_newprice = 0.0;
			BillTbl::Rec link_bill_rec;
			PPObjGoods    gobj;
			PPGoodsTaxEntry gtx_cost;
			PPGoodsTaxEntry gtx_price;
			{
				LDATE tax_dt = p_ti->Date;
				if(p_pack->OpTypeID == PPOPT_CORRECTION && p_pack->Rec.LinkBillID && BillObj->Fetch(p_pack->Rec.LinkBillID, &link_bill_rec) > 0)
					tax_dt = link_bill_rec.Dt;
				gobj.FetchTaxEntry2(p_ti->GoodsID, 0/*lotID*/, 0/*taxPayerID*/, tax_dt, 0, &gtx_price);
			}
			if(p_ti->LotTaxGrpID) {
				gobj.GTxObj.Fetch(p_ti->LotTaxGrpID, p_ti->LotDate, 0, &gtx_cost);
			}
			else {
				gtx_cost = gtx_price;
			}
			double tax_new_qtty = qtty;
			double tax_old_qtty = old_qtty;
			gobj.MultTaxFactor(p_ti->GoodsID, &tax_old_qtty);
			gobj.MultTaxFactor(p_ti->GoodsID, &tax_new_qtty);
			// @v11.1.7 {
			{
				if(correction_type < 0) { // Корректировка расхода
					org_rbybill = p_ti->RByBill;
				}
				else if(correction_type > 0) { // Корректировка прихода
					uint lot_pos = 0;
					if(p_pack->P_LinkPack && p_pack->P_LinkPack->SearchLot(p_ti->LotID, &lot_pos))
						org_rbybill = p_pack->P_LinkPack->ConstTI(lot_pos).RByBill;
				}
				else { // Переоценка. Код эквивалентен корректировке прихода, но дублируем на случая специальных особенностей, которорые могут вылезти позже
					uint lot_pos = 0;
					if(p_pack->P_LinkPack && p_pack->P_LinkPack->SearchLot(p_ti->LotID, &lot_pos))
						org_rbybill = p_pack->P_LinkPack->ConstTI(lot_pos).RByBill;
				}
			}
			// } @v11.1.7 
			if(p_ti->Flags & PPTFR_CORRECTION) {
				GTaxVect gtv;
				{
					long   amt_flags  = ~0L;
					long   excl_flags = 0L;
					if(p_ti->Flags & PPTFR_COSTWOVAT)
						amt_flags &= ~GTAXVF_VAT;
					if(PPObjLocation::CheckWarehouseFlags(p_ti->LocID, LOCF_VATFREE) || IsSupplVATFree(p_ti->Suppl) > 0)
						excl_flags |= GTAXVF_VAT;
					if(gtx_cost.Flags & GTAXF_NOLOTEXCISE)
						excl_flags |= GTAXVF_EXCISE;
					gtv.Calc_(gtx_cost, old_cost, tax_old_qtty, amt_flags, excl_flags);
					vatsum_oldcost = gtv.GetValue(GTAXVF_VAT);
					gtv.Calc_(gtx_cost, new_cost,  tax_new_qtty, amt_flags, excl_flags);
					vatsum_newcost  = gtv.GetValue(GTAXVF_VAT);
				}
				{
					long   amt_flags  = ~0L;
					long   excl_flags = 0L;
					gtv.Calc_(gtx_price, old_price * old_qtty, tax_old_qtty, amt_flags, excl_flags);
					vatsum_oldprice = gtv.GetValue(GTAXVF_VAT);
					gtv.Calc_(gtx_price, new_price * qtty,  tax_new_qtty, amt_flags, excl_flags);
					vatsum_newprice = gtv.GetValue(GTAXVF_VAT);
				}
			}
			else if(p_ti->Flags & PPTFR_REVAL && reval_assets_wo_vat && gobj.IsAsset(labs(p_ti->GoodsID))) {
				GTaxVect gtv;
				long   amt_flags  = ~0L;
				long   excl_flags = 0L;
				if(p_ti->Flags & PPTFR_COSTWOVAT)
					amt_flags &= ~GTAXVF_VAT;
				if(PPObjLocation::CheckWarehouseFlags(p_ti->LocID, LOCF_VATFREE) || IsSupplVATFree(p_ti->Suppl) > 0)
					excl_flags |= GTAXVF_VAT;
				if(gtx_cost.Flags & GTAXF_NOLOTEXCISE)
					excl_flags |= GTAXVF_EXCISE;
				gtv.Calc_(gtx_cost, new_price, tax_new_qtty, amt_flags, excl_flags);
				new_price = gtv.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gtv.Calc_(gtx_cost, new_cost,  tax_new_qtty, amt_flags, excl_flags);
				new_cost  = gtv.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gtv.Calc_(gtx_cost, old_price, tax_old_qtty, amt_flags, excl_flags);
				old_price = gtv.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
				gtv.Calc_(gtx_cost, old_cost,  tax_old_qtty, amt_flags, excl_flags);
				old_cost  = gtv.GetValue(~GTAXVF_BEFORETAXES & ~GTAXVF_VAT);
			}
			{
				p_extra->OldCostVat = vatsum_oldcost;
				p_extra->NewCostVat = vatsum_newcost;
				p_extra->OldPriceVat = fdivnz(vatsum_oldprice, old_qtty);
				p_extra->NewPriceVat = fdivnz(vatsum_newprice, qtty);
				p_extra->VatSum_OldCost = vatsum_oldcost * old_qtty;
				p_extra->VatSum_NewCost = vatsum_newcost * qtty;
				p_extra->VatSum_OldPrice = vatsum_oldprice;
				p_extra->VatSum_NewPrice = vatsum_newprice;
				p_extra->Qtty = qtty;
				p_extra->OldQtty = old_qtty;
				p_extra->OldCost = old_cost;
				p_extra->OldCostWoVat = old_cost-vatsum_oldcost;
				p_extra->OldPrice = old_price;
				p_extra->OldPriceWoVat = old_price-p_extra->OldPriceVat;
				p_extra->NewCost = new_cost;
				p_extra->NewCostWoVat = new_cost-vatsum_newcost;
				p_extra->NewCostSum = new_cost * qtty;
				p_extra->OldCostSum = old_cost * p_extra->OldQtty;
				p_extra->NewPrice = new_price;
				p_extra->NewPriceWoVat = new_price-p_extra->NewPriceVat;
				p_extra->NewPriceSum = new_price * qtty;
				p_extra->OldPriceSum = old_price * p_extra->OldQtty;
				p_extra->VATRate = gtx_cost.GetVatRate();
				p_extra->NewCostSumWoVat = p_extra->NewCostSum - p_extra->VatSum_NewCost;
				p_extra->OldCostSumWoVat = p_extra->OldCostSum - p_extra->VatSum_OldCost;
				p_extra->NewPriceSumWoVat = p_extra->NewPriceSum - p_extra->VatSum_NewPrice;
				p_extra->OldPriceSumWoVat = p_extra->OldPriceSum - p_extra->VatSum_OldPrice;
				if(p_ti->IsCorrectionExp()) {
					p_extra->MainPrice = p_extra->NewPrice;
					p_extra->MainSum = p_extra->NewPriceSum - p_extra->OldPriceSum;
					p_extra->VATSum = p_extra->VatSum_NewPrice - p_extra->VatSum_OldPrice;
				}
				else if(p_ti->IsCorrectionRcpt()) {
					p_extra->MainPrice = p_extra->NewCost;
					p_extra->MainSum = p_extra->NewCostSum - p_extra->OldCostSum;
					p_extra->VATSum = p_extra->VatSum_NewCost - p_extra->VatSum_OldCost;
				}
				else {
					p_extra->MainPrice = p_extra->NewPrice;
					p_extra->MainSum = p_extra->NewPriceSum;
					p_extra->VATSum = p_extra->VatSum_NewPrice;
				}
				p_extra->MainSumWoVat = p_extra->MainSum - p_extra->VATSum;
			}
			{
				I.nn       = nn;
				I.RByBill  = p_ti->RByBill; // @v11.1.7
				I.OrgRByBill = org_rbybill; // @v11.1.7
				I.GoodsID  = p_ti->GoodsID;
				I.LotID    = p_ti->LotID;
				I.Quantity = p_extra->Qtty;
				I.OldQtty  = p_extra->OldQtty;
				I.NewPrice = new_price;
				I.NewCost  = new_cost;
				I.OldPrice = old_price;
				I.OldCost  = old_cost;
				I.VATRate  = gtx_cost.GetVatRate();
				I.VATSumOldCost = p_extra->VatSum_OldCost;
				I.VATSumNewCost = p_extra->VatSum_NewCost;
				I.VATSumOldPrice = p_extra->VatSum_OldPrice;
				I.VATSumNewPrice = p_extra->VatSum_NewPrice;
			}
		}
		else
			return -1;
	}
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsReval::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	const DlGoodsBillBaseBlock * p_extra = static_cast<const DlGoodsBillBaseBlock *>(Extra[0].Ptr);
	const PPBillPacket * p_pack = p_extra ? p_extra->P_Pack : 0;
	if(pF->Name == "?UnlimGoodsOnly") {
		_RET_INT = BIN(p_pack && p_pack->ProcessFlags & PPBillPacket::pfAllGoodsUnlim);
	}
	else if(pF->Name == "?GetRowAmount") {
		_RET_DBL = p_extra ? p_extra->GetValueBySymb(_ARG_STR(1)) : 0.0;
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
	const BillTbl::Rec * p_rec = &static_cast<const PPBillPacket *>(Extra[0].Ptr)->Rec;
	MEMSZERO(H);
	H.BillID = p_rec->ID;
	if(CheckOpFlags(p_rec->OpID, OPKF_PROFITABLE))
		H.fProfitable = 1;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_BillPayPlan::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(Extra[0].Ptr);
	I.nn = 0;
	I.Rest = p_pack->Rec.Amount;
	return 1;
}

int PPALDD_BillPayPlan::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
		uint   n = static_cast<uint>(I.nn);
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
	PPBillPacket * pack = static_cast<PPBillPacket *>(rFilt.Ptr);
	int    incstax = 0, val = 0;
	PPIniFile ini_file;
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CASHORDINCSTAX, &val))
		incstax = val;
	H.Dt = pack->Rec.Dt;
	STRNSCPY(H.Code, CheckOpPrnFlags(pack->Rec.OpID, OPKF_PRT_NBILLN) ? 0 : pack->Rec.Code);
	// @v11.1.12 STRNSCPY(H.Memo, pack->Rec.Memo);
	STRNSCPY(H.Memo, pack->SMemo); // @v11.1.12
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
		p_tax_pack->CalcTotal(total_data, btc_flags);
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
			amtt_obj.GetTaxAmountIDs(tais, 1);
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
		amtt_obj.GetTaxAmountIDs(tais, 1);
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
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
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
		if(p_pack->Rec.Object) {
			if(PPObjLocation::ObjToWarehouse(p_pack->Rec.Object)) {
				H.RcvrReq   = main_org_id;
				H.RcvrLocID = PPObjLocation::ObjToWarehouse(p_pack->Rec.Object);
			}
			else {
				H.RcvrReq   = ObjectToPerson(p_pack->Rec.Object);
				H.RcvrLocID = 0;
			}
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
		PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
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
		PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
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
	PPViewBill * p_v = static_cast<PPViewBill *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewBill::IterOrder>(SortIdx));
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
		p_v->CalcItemTotal(item.ID, total);
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
	PPViewBill * p_v = static_cast<PPViewBill *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewBill::IterOrder>(SortIdx));
	I.recNo = 0;
	I.grpNo = 0;
	return 1;
}

struct PPALDD_ContentBList_VIterState {
	PPALDD_ContentBList_VIterState(PPID billID, bool isDraft) : BillID(billID), IsDraft(isDraft), Rbb(0)
	{
	}
	const  PPID BillID;
	const  bool IsDraft;
	int    Rbb;
};

int PPALDD_ContentBList::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPViewBill * p_v = static_cast<PPViewBill *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	PPObjBill * p_bobj = BillObj;
	BillViewItem item;
	PPTransferItem ti;
	CpTrfrExt ext;
	while(I.recNo || p_v->NextIteration(&item) > 0) {
		if(!I.recNo) {
			I.BillID = item.ID;
			I.grpNo++;
			PPALDD_ContentBList_VIterState vis(I.BillID, IsDraftOp(item.OpID));
			p_v->SetIterState(&vis, sizeof(vis));
		}
		else {
			p_bobj->Fetch(I.BillID, &item); // @v12.3.2 @fix
		}
		int    r = 0;
		PPALDD_ContentBList_VIterState * p_vis = static_cast<PPALDD_ContentBList_VIterState *>(p_v->GetIterState());
		if(p_vis) {
			p_vis->Rbb = 0;
			if(p_vis->IsDraft) {
				if(p_bobj->P_CpTrfr) {
		 			r = p_bobj->P_CpTrfr->EnumItems(I.BillID, reinterpret_cast<int *>(&I.recNo), &ti, &ext);
				}
			}
			else {
				r = p_bobj->trfr->EnumItems(I.BillID, reinterpret_cast<int *>(&I.recNo), &ti);
			}
		}
		if(r > 0) {
			assert(p_vis != 0);
			if(p_vis) {
				p_vis->Rbb = ti.RByBill;
			}
			double cost = 0.0;
			double old_cost = 0.0;
			double price = 0.0;
			double nominal_price = 0.0;
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
			GTaxVect gtv;
			gtv.CalcTI(ti, item.OpID, /*TIAMT_AMOUNT*/GTaxVect::GetTaxNominalAmountType(item));
		   	I.VaTax    = gtv.GetTaxRate(GTAX_VAT, 0);
			I.VatSum   = gtv.GetValue(GTAXVF_VAT);
			I.ExTax    = gtv.GetTaxRate(GTAX_EXCISE, 0);
			I.ExtSum   = gtv.GetValue(GTAXVF_EXCISE);
			I.StTax    = gtv.GetTaxRate(GTAX_SALES, 0);
			I.StSum    = gtv.GetValue(GTAXVF_SALESTAX);
			I.NominalPrice = fdivnz(gtv.GetValue(GTAXVF_BEFORETAXES), qtty); // @v11.3.6
			if(ti.Flags & PPTFR_COSTWOVAT) {
				gtv.CalcTI(ti, item.OpID, TIAMT_COST);
				cost += gtv.GetValue(GTAXVF_VAT) / qtty;
				old_cost += gtv.GetValue(GTAXVF_VAT) / old_qtty;
			}
			if(ti.Flags & PPTFR_PRICEWOTAXES) {
				gtv.CalcTI(ti, item.OpID, TIAMT_PRICE);
				price = gtv.GetValue(GTAXVF_BEFORETAXES) / qtty;
				old_price = gtv.GetValue(GTAXVF_BEFORETAXES) / old_qtty;
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
			// @erik {
			if(GetOpType(item.OpID) == PPOPT_PAYMENT) {
				double cost = 0.0;
				double old_cost = 0.0;
				double price = 0.0;
				double old_price = 0.0;
				double qtty  = 0.0;
				double old_qtty = 0.0;
				double coefnt_cost_price = 0.0;
				I.GoodsID  = 0;
				I.LotID    = 0;
				BillTbl::Rec link_bill_rec;
				if(item.LinkBillID && p_bobj->Search(item.LinkBillID, &link_bill_rec) > 0 && link_bill_rec.Amount != 0.0) {
					PPBillPacket link_bpack;
					if(p_bobj->ExtractPacket(item.LinkBillID, &link_bpack) > 0) {
						qtty = item.Amount / link_bpack.Rec.Amount;
						cost = link_bpack.Amounts.Get(PPAMT_BUYING, link_bpack.Rec.CurID);
						price = link_bpack.Amounts.Get(PPAMT_SELLING, link_bpack.Rec.CurID);
						coefnt_cost_price = cost / price;
						I.Cost = item.Amount * coefnt_cost_price;
						I.OldCost = 0.0;
						I.Price = item.Amount;
						I.OldPrice = 0.0;
						I.CurPrice = qtty;
						I.Quantity = 1.0; //один раз оплата
						I.OldQtty = 0.0;
					}
				}
				else {
					I.Cost     = cost;
					I.OldCost  = old_cost;
					I.Price    = price;
					I.OldPrice = old_price;
					I.CurPrice = 0.0;
					I.Quantity = 0.0;
					I.OldQtty  = 0.0;
				}
				return DlRtm::NextIteration(iterId);
			}
			// } @erik
			else {
				I.recNo = 0;
				PPWaitPercent(p_v->GetCounter());
			}
		}
	}
	return -1;
}

void PPALDD_ContentBList::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS) // @v12.2.0
{
	#define _ARG_INT(n)  (*static_cast<const int *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	PPViewBill * p_v = static_cast<PPViewBill *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	if(pF->Name == "?GetRowAmount") { // iterator
		double result = 0.0;
		if(p_v) {
			PPALDD_ContentBList_VIterState * p_vis = static_cast<PPALDD_ContentBList_VIterState *>(p_v->GetIterState());
			if(p_vis && p_vis->BillID && p_vis->Rbb) {
				const PPBillPacket * p_bp = p_v->GetIterCachedBillPack(p_vis->BillID);
				uint tiidx = 0;
				if(p_bp && p_bp->SearchTI(p_vis->Rbb, &tiidx)) {
					const PPTransferItem & r_ti = p_bp->ConstTI(tiidx);
					SString & r_buf = SLS.AcquireRvlStr();
					(r_buf = _ARG_STR(1)).Strip().ToLower().ReplaceStr("-", 0, 0).ReplaceStr("_", 0, 0).ReplaceStr(" ", 0, 0);
					SString symb(r_buf);
					const double qtty = r_ti.Qtty();
					const double absqtty = fabs(r_ti.Qtty());
					if(symb == "qtty" || symb == "quantity" || symb == "qty") {
						result = absqtty;
					}
					else if(symb == "oldqtty") {
					}
					else if(symb == "cost") {
						result = fabs(r_ti.Cost);
					}
					else if(symb == "price") {
						result = fabs(r_ti.Price);
					}
					else if(symb == "discount") {
						result = r_ti.Discount;
					}
					else if(symb == "curprice") {
					}
					else if(symb == "cursum") {
					}
					else if(symb == "mainprice") {
					}
					else if(symb == "mainsum") {
					}
					else if(symb == "mainsumwovat") {
					}
					else if(symb == "extprice") {
					}
					else if(symb == "excrate") {
					}
					else if(symb == "excsum") {
					}
					else if(symb == "strate") {
					}
					else if(symb == "stsum") {
					}
					else if(symb == "newprice") {
					}
					else if(symb == "newpricewovat") {
					}
					else if(symb == "newcost") {
					}
					else if(symb == "newcostwovat") {
					}
					else if(symb == "newpricesum") {
					}
					else if(symb == "newcostsum") {
					}
					else if(symb == "newpricesumwovat") {
					}
					else if(symb == "newcostsumwovat") {
					}
					else if(symb == "oldprice") {
					}
					else if(symb == "oldpricewovat") {
					}
					else if(symb == "oldcost") {
					}
					else if(symb == "oldcostwovat") {
					}
					else if(symb == "oldpricesum") {
					}
					else if(symb == "oldcostsum") {
					}
					else if(symb == "oldpricesumwovat") {
					}
					else if(symb == "oldcostsumwovat") {
					}
					else if(symb == "vatsumoldcost") {
					}
					else if(symb == "vatsumnewcost") {
					}
					else if(symb == "vatsumoldprice") {
					}
					else if(symb == "vatsumnewprice") {
					}
					else if(symb == "oldpricevat") {
					}
					else if(symb == "oldcostvat") {
					}
					else if(symb == "newpricevat") {
					}
					else if(symb == "newcostvat") {
					}
					else if(symb == "vatrate") {
					}
					else if(symb == "vat") {
					}
					else if(symb == "vatsum") {
					}
					else if(symb == "costvatrate") { // ставка НДС в ценах поступления //
						GTaxVect gtv;
						gtv.CalcBPTI(*p_bp, r_ti, TIAMT_COST);
						result = gtv.GetTaxRate(GTAX_VAT, 0);
					}
					else if(symb == "costvat") { // величина НДС в цене поступления //
						if(absqtty != 0.0) {
							GTaxVect gtv;
							gtv.CalcBPTI(*p_bp, r_ti, TIAMT_COST);
							result = fabs(gtv.GetValue(GTAXVF_VAT)) / absqtty;
						}
					}
					else if(symb == "costvatsum") { // величина НДС в сумме поступления (в цене поступления, умноженной на количество)
						GTaxVect gtv;
						gtv.CalcBPTI(*p_bp, r_ti, TIAMT_COST);
						result = fabs(gtv.GetValue(GTAXVF_VAT));
					}
					else if(symb == "pricevatrate") {
						GTaxVect gtv;
						gtv.CalcBPTI(*p_bp, r_ti, TIAMT_PRICE);
						result = gtv.GetTaxRate(GTAX_VAT, 0);
					}
					else if(symb == "pricevat") {
						if(absqtty != 0.0) {
							GTaxVect gtv;
							gtv.CalcBPTI(*p_bp, r_ti, TIAMT_PRICE);
							result = fabs(gtv.GetValue(GTAX_VAT)) / absqtty;
						}
					}
					else if(symb == "pricevatsum") {
						GTaxVect gtv;
						gtv.CalcBPTI(*p_bp, r_ti, TIAMT_PRICE);
						result = fabs(gtv.GetValue(GTAX_VAT));
					}
				}
			}
		}
		_RET_DBL = result; // p_extra ? p_extra->GetValueBySymb(_ARG_STR(1)) : 0.0;
		
	}
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
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
	MEMSZERO(H);
	H.BillID = p_pack->Rec.ID;
	if(p_pack->P_AdvRep) {
		STRNSCPY(H.Rcp1Text, p_pack->P_AdvRep->Rcp[0].Text);
		H.Rcp1Dt  = p_pack->P_AdvRep->Rcp[0].Dt;
		H.Rcp1Amt = p_pack->P_AdvRep->Rcp[0].Amount;
		STRNSCPY(H.Rcp2Text, p_pack->P_AdvRep->Rcp[1].Text);
		H.Rcp2Dt  = p_pack->P_AdvRep->Rcp[1].Dt;
		H.Rcp2Amt = p_pack->P_AdvRep->Rcp[1].Amount;
		H.InRest = p_pack->P_AdvRep->InRest;
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
	const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(Extra[0].Ptr);
	if(H.nn < static_cast<int16>(p_pack->AdvList.GetCount())) {
		const PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(H.nn);
		SString temp_buf;
		STRNSCPY(I.AdvCode, r_item.AdvCode);
		I.AdvDt = r_item.AdvDt;
		I.AdvBillKindID = r_item.AdvBillKindID;
		GetObjectName(PPOBJ_ADVBILLKIND, r_item.AdvBillKindID, temp_buf.Z());
		STRNSCPY(I.AdvBillKindName, temp_buf);
		I.AdvBillID = r_item.AdvBillID;
		I.AccID     = r_item.AccID;
		I.ArID      = r_item.ArID;
		I.Flags     = r_item.Flags;
		I.Amt       = r_item.Amount;
		I.ExtAmt    = r_item.ExtAmt;
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
	PPObjPerson psn_obj;
	PPBillPacket * pack = static_cast<PPBillPacket *>(rFilt.Ptr);
	if(pack && pack->P_PaymOrder) {
		SString temp_buf;
		BnkAcctData bnk_data(BADIF_INITALLBR);
		H.PayerID = pack->P_PaymOrder->PayerID;
		H.RcvrID  = pack->P_PaymOrder->RcvrID;
		H.BillID  = pack->P_PaymOrder->BillID;
		H.LclDt   = pack->P_PaymOrder->Dt;
		STRNSCPY(H.LclCode, pack->P_PaymOrder->Code);
		// @v11.1.12 STRNSCPY(H.LclMemo, pack->Rec.Memo);
		STRNSCPY(H.LclMemo, pack->SMemo); // @v11.1.12
		H.PayerBnkAccID = pack->P_PaymOrder->PayerBnkAccID;
		psn_obj.GetBnkAcctData(H.PayerBnkAccID, 0, &bnk_data);
		H.PayerBnkID = bnk_data.Bnk.ID;
		//STRNSCPY(H.PayerBnkName, bnk_data.Bnk.Name);
		STRNSCPY(H.PayerBnkCity, bnk_data.Bnk.City);
		STRNSCPY(H.PayerBIC,     bnk_data.Bnk.BIC);
		STRNSCPY(H.PayerBnkCorAcc, bnk_data.Bnk.CorrAcc);
		STRNSCPY(H.PayerAcc,     bnk_data.Acct);
		H.RcvrBnkAccID  = pack->P_PaymOrder->RcvrBnkAccID;
		bnk_data.InitFlags = BADIF_INITALLBR;
		psn_obj.GetBnkAcctData(H.RcvrBnkAccID, 0, &bnk_data);
		H.RcvrBnkID = bnk_data.Bnk.ID;
		//STRNSCPY(H.RcvrBnkName, bnk_data.Bnk.Name);
		STRNSCPY(H.RcvrBnkCity,   bnk_data.Bnk.City);
		STRNSCPY(H.RcvrBIC,       bnk_data.Bnk.BIC);
		STRNSCPY(H.RcvrBnkCorAcc, bnk_data.Bnk.CorrAcc);
		STRNSCPY(H.RcvrAcc,       bnk_data.Acct);
		H.BnkPaymMethod = pack->P_PaymOrder->BnkPaymMethod;
		H.BnkQueueing   = pack->P_PaymOrder->BnkQueueing;
		H.FormalPurpose = pack->P_PaymOrder->FormalPurpose;
		H.PayerStatus   = pack->P_PaymOrder->PayerStatus;
		{
			temp_buf.Z();
			if(pack->P_PaymOrder->PayerStatus) {
				temp_buf.Z().CatLongZ(pack->P_PaymOrder->PayerStatus, 2);
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
		// @v11.1.12 STRNSCPY(H.LclMemo, pack->Rec.Memo);
		STRNSCPY(H.LclMemo, pack->SMemo); // @v11.1.12
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
	if(rsrv) {
		BillTotalPrintData * p_data = static_cast<BillTotalPrintData *>(rFilt.Ptr);
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
	BillTotalPrintData * p_data = static_cast<BillTotalPrintData *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	AmtEntry * p_item;
	uint   n = static_cast<uint>(I.LineNo);
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
	AssetCard * p_data = static_cast<AssetCard *>(rFilt.Ptr);
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
	AssetCard * p_data = static_cast<AssetCard *>(Extra[1].Ptr);
	AssetCard::MovItem * p_item;
	uint   n = static_cast<uint>(H.MovLineNo);
	if(p_data && p_data->P_MovList && p_data->P_MovList->enumItems(&n, (void **)&p_item) > 0) {
		H.MovLineNo = static_cast<long>(n);
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
	const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(Extra[0].Ptr);
	MEMSZERO(H);
	H.BillID = p_pack->Rec.ID;
	STRNSCPY(H.WarrantNo,  p_pack->Rec.Code);
	H.WarrantDt = p_pack->Rec.Dt;
	p_pack->GetLastPayDate(&dt);
	H.ExpiryDt = dt;
	if(p_pack->Rec.Object && p_pack->AccSheetID)
		H.PersonReqID = ObjectToPerson(p_pack->Rec.Object, 0);
	H.SupplID = p_pack->Rec.Object2;
	// @v11.1.12 STRNSCPY(H.Memo, p_pack->Rec.Memo);
	STRNSCPY(H.Memo, p_pack->SMemo); // @v11.1.12
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
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	if(I.LineNo < static_cast<int16>(p_pack->AdvList.GetCount())) {
		SString temp_buf;
		char   buf[128];
		const  PPAdvBillItemList::Item & r_item = p_pack->AdvList.Get(I.LineNo);
		STRNSCPY(I.GdsName, r_item.Memo);
		GetObjectName(PPOBJ_UNIT, r_item.ArID, temp_buf.Z());
		STRNSCPY(I.Unit, temp_buf);
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
	const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(rFilt.Ptr);
	const  long f = p_pack->Rec.Flags;
	H.BillID = p_pack->Rec.ID;
	H.fTotalDiscount     = BIN(f & BILLF_TOTALDISCOUNT);
	H.fGReceipt  = BIN(f & BILLF_GRECEIPT);
	H.fGExpend   = BIN(f & BILLF_GEXPEND);
	H.fGReval    = BIN(f & BILLF_GREVAL);
	H.fGModif    = BIN(f & BILLF_GMODIF);
	H.fClosedOrder       = BIN(f & BILLF_CLOSEDORDER);
	H.fCash      = BIN(f & BILLF_CASH);
	H.fCCheck    = BIN(f & BILLF_CHECK);
	H.fNoAturn   = BIN(f & BILLF_NOATURN);
	H.fRmvExcise = BIN(f & BILLF_RMVEXCISE);
	H.fFixedAmounts      = BIN(f & BILLF_FIXEDAMOUNTS);
	H.fFreight   = BIN(f & BILLF_FREIGHT);
	H.fRent      = BIN(f & BILLF_RENT);
	H.fRecon     = BIN(f & BILLF_RECKON);
	H.fBanking   = BIN(f & BILLF_BANKING);
	H.fShipped   = BIN(f & BILLF_SHIPPED);
	H.fWritedOff = BIN(f & BILLF_WRITEDOFF);
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
	uint   n = static_cast<uint>(I.nn);
	PPBillPacket * p_pack = static_cast<PPBillPacket *>(Extra[0].Ptr);
	if(n < p_pack->Amounts.getCount()) {
		const AmtEntry & r_amt = p_pack->Amounts.at(n);
		I.nn = n + 1;
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
	BillInfoListPrintData * p_bilpd = static_cast<BillInfoListPrintData *>(Extra[0].Ptr);
	I.nn = 0;
	ZDELETE(p_bilpd->P_Pack);
	return p_bilpd->P_V->InitIteration(PPViewBill::OrdByDefault);
}

int PPALDD_BillInfoList::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	int    ok = 1;
	uint   n = static_cast<uint>(I.nn);
	BillViewItem item;
	PPBillPacket * p_pack = 0;
	BillInfoListPrintData * p_bilpd = static_cast<BillInfoListPrintData *>(Extra[0].Ptr);
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
		I.fTotalDiscount = BIN(f & BILLF_TOTALDISCOUNT);
		I.fGReceipt  = BIN(f & BILLF_GRECEIPT);
		I.fGExpend   = BIN(f & BILLF_GEXPEND);
		I.fGReval    = BIN(f & BILLF_GREVAL);
		I.fGModif    = BIN(f & BILLF_GMODIF);
		I.fClosedOrder = BIN(f & BILLF_CLOSEDORDER);
		I.fCash      = BIN(f & BILLF_CASH);
		I.fCCheck    = BIN(f & BILLF_CHECK);
		I.fNoAturn   = BIN(f & BILLF_NOATURN);
		I.fRmvExcise = BIN(f & BILLF_RMVEXCISE);
		I.fFixedAmounts = BIN(f & BILLF_FIXEDAMOUNTS);
		I.fFreight   = BIN(f & BILLF_FREIGHT);
		I.fRent      = BIN(f & BILLF_RENT);
		I.fRecon     = BIN(f & BILLF_RECKON);
		I.fBanking   = BIN(f & BILLF_BANKING);
		I.fShipped   = BIN(f & BILLF_SHIPPED);
		I.fWritedOff = BIN(f & BILLF_WRITEDOFF);
		I.fCSessWrOff        = BIN(f & BILLF_CSESSWROFF);
		I.fAdvanceRep        = BIN(f & BILLF_ADVANCEREP);
		I.fTGGLexCsNPrice    = BIN(f & BILLF_TGGLEXCSNPRICE);
		I.fTSessWrOff        = BIN(f & BILLF_TSESSWROFF);
		if(n < p_bilpd->P_Pack->Amounts.getCount()) {
			AmtEntry amt = p_bilpd->P_Pack->Amounts.at(n);
			I.nn = n + 1;
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
	delete static_cast<UhttBillBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttBill::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	MEMSZERO(H);
	PPViewBill bv;
	PPBillExt  ext;
	SString temp_buf;
	UhttBillBlock & r_blk = *static_cast<UhttBillBlock *>(Extra[0].Ptr);
	if(bv.GetPacket(rFilt.ID, &r_blk.Pack) > 0) {
		PPObjBill * p_bobj = BillObj;
		H.ID = r_blk.Pack.Rec.ID;
		H.Dt = r_blk.Pack.Rec.Dt;
		H.OprKindID = r_blk.Pack.Rec.OpID;
		STRNSCPY(H.Code, r_blk.Pack.Rec.Code);
		H.LocID = r_blk.Pack.Rec.LocID;
		H.ArticleID = r_blk.Pack.Rec.Object;
		H.DlvrLocID = r_blk.Pack.GetDlvrAddrID();
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
	UhttBillBlock & r_blk = *static_cast<UhttBillBlock *>(Extra[0].Ptr);
	r_blk.ItemsCounter = 0;
	return -1;
}

int PPALDD_UhttBill::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	if(iterId == GetIterID("iter@Items")) {
		UhttBillBlock & r_blk = *static_cast<UhttBillBlock *>(Extra[0].Ptr);
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
	TSVector <UhttGoodsRestVal> * p_list = static_cast<TSVector <UhttGoodsRestVal> *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttDraftTransitGoodsRestList::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	TSVector <UhttGoodsRestVal> * p_list = static_cast<TSVector <UhttGoodsRestVal> *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_UhttDraftTransitGoodsRestList::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	TSVector <UhttGoodsRestVal> * p_list = static_cast<TSVector <UhttGoodsRestVal> *>(Extra[0].Ptr);
	if(p_list && p_list->testPointer()) {
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
