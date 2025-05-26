// V_LOT.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewLot)
//
LotFilt::FiltExtraParam::FiltExtraParam(int kind) : Kind(kind) 
{ 
	assert(oneof2(Kind, kRegular, kOrders)); 
}

int LotFilt::InitInstance()
{
	P_TagF = 0;
	SetFlatChunk(offsetof(LotFilt, ReserveStart),
		offsetof(LotFilt, Reserve)-offsetof(LotFilt, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(LotFilt, P_TagF));
	SetBranchSString(offsetof(LotFilt, ExtString));
	SetBranchObjIdListFilt(offsetof(LotFilt, LocList));
	return Init(1, 0);
}

IMPLEMENT_PPFILT_FACTORY(Lot); LotFilt::LotFilt() : PPBaseFilt(PPFILT_LOT, 0, 3)
{
	InitInstance();
}

LotFilt::LotFilt(const LotFilt & rS) : PPBaseFilt(PPFILT_LOT, 0, 3)
{
	InitInstance();
	Copy(&rS, 1);
}

LotFilt & FASTCALL LotFilt::operator = (const LotFilt & s)
{
	Copy(&s, 0);
	return *this;
}

int LotFilt::GetExtssData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int LotFilt::PutExtssData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }

/*virtual*/int LotFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		struct LotFilt_v0 : public PPBaseFilt {
			LotFilt_v0() : PPBaseFilt(PPFILT_LOT, 0, 0)
			{
				SetFlatChunk(offsetof(LotFilt_v0, ReserveStart),
					offsetof(LotFilt_v0, Reserve)-offsetof(LotFilt_v0, ReserveStart)+sizeof(Reserve));
				Init(1, 0);
			}
			uint8  ReserveStart[24]; // @anchor
			int16  CostDevRestr;
			int16  PriceDevRestr;
			PPID   ParentLotID;
			DateRange Period;
			DateRange Operation;
			DateRange ExpiryPrd;
			DateRange QcExpiryPrd;
			PPID   LocID_Obsolete;
			PPID   SupplID;
			PPID   GoodsGrpID;
			PPID   GoodsID;
			PPID   QCertID;
			PPID   InTaxGrpID;
			long   Flags;
			uint   ClosedTag;
			char   Serial[32];
			RealRange CostRange;
			RealRange PriceRange;
			long   Reserve;          // @anchor
		};
		LotFilt_v0 fv0;
		THROW(fv0.Read(rBuf, 0));
#define CPYFLD(f) f = fv0.f
		CPYFLD(CostDevRestr);
		CPYFLD(PriceDevRestr);
		CPYFLD(ParentLotID);
		CPYFLD(Period);
		CPYFLD(Operation);
		CPYFLD(ExpiryPrd);
		CPYFLD(QcExpiryPrd);
		CPYFLD(LocID_Obsolete);
		CPYFLD(SupplID);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(QCertID);
		CPYFLD(InTaxGrpID);
		CPYFLD(Flags);
		CPYFLD(ClosedTag);
		CPYFLD(CostRange);
		CPYFLD(PriceRange);
#undef CPYFLD
		if(fv0.Serial[0]) {
			PutExtssData(extssSerialText, fv0.Serial);
		}
		ok = 1;
	}
	else if(ver == 1) {
		struct LotFilt_v1 : public PPBaseFilt {
			LotFilt_v1() : PPBaseFilt(PPFILT_LOT, 0, 1)
			{
				SetFlatChunk(offsetof(LotFilt_v1, ReserveStart), offsetof(LotFilt_v1, Reserve)-offsetof(LotFilt_v1, ReserveStart)+sizeof(Reserve));
				Init(1, 0);
			}
			uint8  ReserveStart[24];
			int16  CostDevRestr;
			int16  PriceDevRestr;
			PPID   ParentLotID;
			DateRange Period;
			DateRange Operation;
			DateRange ExpiryPrd;
			DateRange QcExpiryPrd;
			PPID   LocID_Obsolete;
			PPID   SupplID;
			PPID   GoodsGrpID;
			PPID   GoodsID;
			PPID   QCertID;
			PPID   InTaxGrpID;
			long   Flags;
			uint   ClosedTag;
			char   Serial[32];
			RealRange CostRange;
			RealRange PriceRange;
			long   Reserve;
			TagFilt * P_TagF;
		};
		LotFilt_v1 fv1;
		THROW(fv1.Read(rBuf, 0));
#define CPYFLD(f) f = fv1.f
		CPYFLD(CostDevRestr);
		CPYFLD(PriceDevRestr);
		CPYFLD(ParentLotID);
		CPYFLD(Period);
		CPYFLD(Operation);
		CPYFLD(ExpiryPrd);
		CPYFLD(QcExpiryPrd);
		CPYFLD(LocID_Obsolete);
		CPYFLD(SupplID);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(QCertID);
		CPYFLD(InTaxGrpID);
		CPYFLD(Flags);
		CPYFLD(ClosedTag);
		CPYFLD(CostRange);
		CPYFLD(PriceRange);
#undef CPYFLD
		if(fv1.Serial[0]) {
			PutExtssData(extssSerialText, fv1.Serial);
		}
		ZDELETE(P_TagF);
		if(fv1.P_TagF) {
			THROW_MEM(P_TagF = new TagFilt);
			*P_TagF = *fv1.P_TagF;
		}
		memzero(Reserve2, sizeof(Reserve2));
		ok = 1;
	}
	else if(ver == 2) {
		class LotFilt_v2 : public PPBaseFilt {
		public:
			LotFilt_v2() : PPBaseFilt(PPFILT_LOT, 0, 2), P_TagF(0)
			{
				SetFlatChunk(offsetof(LotFilt_v2, ReserveStart), offsetof(LotFilt_v2, Reserve)-offsetof(LotFilt_v2, ReserveStart)+sizeof(Reserve));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(LotFilt_v2, P_TagF));
				SetBranchSString(offsetof(LotFilt_v2, ExtString));
				Init(1, 0);
			}
			uint8  ReserveStart[20];
			long   ExtViewAttr;
			int16  CostDevRestr;
			int16  PriceDevRestr;
			PPID   ParentLotID;
			DateRange Period;
			DateRange Operation;
			DateRange ExpiryPrd;
			DateRange QcExpiryPrd;
			PPID   LocID_Obsolete;
			PPID   SupplID;
			PPID   GoodsGrpID;
			PPID   GoodsID;
			PPID   QCertID;
			PPID   InTaxGrpID;
			long   Flags;
			uint   ClosedTag;
			uint8  Reserve2[32];
			RealRange CostRange;
			RealRange PriceRange;
			long   Reserve;
			TagFilt * P_TagF;
			SString ExtString;
		};
		LotFilt_v2 fv2;
		THROW(fv2.Read(rBuf, 0));
#define CPYFLD(f) f = fv2.f
		CPYFLD(ExtViewAttr);
		CPYFLD(CostDevRestr);
		CPYFLD(PriceDevRestr);
		CPYFLD(ParentLotID);
		CPYFLD(Period);
		CPYFLD(Operation);
		CPYFLD(ExpiryPrd);
		CPYFLD(QcExpiryPrd);
		CPYFLD(LocID_Obsolete);
		CPYFLD(SupplID);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(QCertID);
		CPYFLD(InTaxGrpID);
		CPYFLD(Flags);
		CPYFLD(ClosedTag);
		CPYFLD(CostRange);
		CPYFLD(PriceRange);
		CPYFLD(ExtString);
#undef CPYFLD
		ZDELETE(P_TagF);
		if(fv2.P_TagF) {
			THROW_MEM(P_TagF = new TagFilt);
			*P_TagF = *fv2.P_TagF;
		}
		memzero(Reserve2, sizeof(Reserve2));
		LocList.Add(LocID_Obsolete);
	}
	CATCHZOK
	return ok;
}

/*virtual*/int LotFilt::Describe(long flags, SString & rBuf) const
{
	{
		SString buf;
		if(CostDevRestr == drNone)
			buf = STRINGIZE(drNone);
		else if(CostDevRestr == drBelow)
			buf = STRINGIZE(drBelow);
		else if(CostDevRestr == drAbove)
			buf = STRINGIZE(drAbove);
		else if(CostDevRestr == drAny)
			buf = STRINGIZE(drAny);
		PutMembToBuf(buf, STRINGIZE(CostDevRestr), rBuf);
		if(PriceDevRestr == drNone)
			buf = STRINGIZE(drNone);
		else if(PriceDevRestr == drBelow)
			buf = STRINGIZE(drBelow);
		else if(PriceDevRestr == drAbove)
			buf = STRINGIZE(drAbove);
		else if(PriceDevRestr == drAny)
			buf = STRINGIZE(drAny);
		PutMembToBuf(buf, STRINGIZE(PriceDevRestr), rBuf);
	}
	PutMembToBuf(&Period,         STRINGIZE(Period),      rBuf);
	PutMembToBuf(&Operation,      STRINGIZE(Operation),   rBuf);
	PutMembToBuf(&ExpiryPrd,      STRINGIZE(ExpiryPrd),   rBuf);
	PutMembToBuf(&QcExpiryPrd,    STRINGIZE(QcExpiryPrd), rBuf);
	PutMembToBuf((long)ClosedTag, STRINGIZE(ClosedTag),   rBuf);
	PutMembToBuf(&CostRange,      STRINGIZE(CostRange),   rBuf);
	PutMembToBuf(&PriceRange,     STRINGIZE(PriceRange),  rBuf);

	PutObjMembListToBuf(PPOBJ_LOCATION, &LocList,   STRINGIZE(LocList),   rBuf);
	PutObjMembToBuf(PPOBJ_PRSNCATEGORY,  SupplPsnCategoryID, STRINGIZE(SupplPsnCategoryID), rBuf); // @v11.4.4
	PutObjMembToBuf(PPOBJ_ARTICLE,    SupplID,     STRINGIZE(SupplID),     rBuf);
	PutObjMembToBuf(PPOBJ_GOODSGROUP, GoodsGrpID,  STRINGIZE(GoodsGrpID),  rBuf);
	PutObjMembToBuf(PPOBJ_GOODS,      GoodsID,     STRINGIZE(GoodsID),     rBuf);
	PutObjMembToBuf(PPOBJ_QCERT,      QCertID,     STRINGIZE(QCertID),     rBuf);
	PutObjMembToBuf(PPOBJ_LOT,        ParentLotID, STRINGIZE(ParentLotID), rBuf);
	PutObjMembToBuf(PPOBJ_GOODSTAX,   InTaxGrpID,  STRINGIZE(InTaxGrpID),  rBuf);
	{
		long id = 1;
		StrAssocArray flag_list;
		#define __ADD_FLAG(f) if(Flags & f) flag_list.Add(id++, STRINGIZE(f));
		__ADD_FLAG(fWithoutQCert);
		__ADD_FLAG(fOrders);
		__ADD_FLAG(fCostAbovePrice);
		__ADD_FLAG(fWithoutClb);
		__ADD_FLAG(fDeadLots);
		__ADD_FLAG(fWithoutExpiry);
		__ADD_FLAG(fOnlySpoilage);
		__ADD_FLAG(fShowSerialN);
		__ADD_FLAG(fSkipNoOp);
		__ADD_FLAG(fCheckOriginLotDate);
		__ADD_FLAG(fSkipClosedBeforeOp);
		__ADD_FLAG(fNoTempTable);
		__ADD_FLAG(fShowBillStatus);
		__ADD_FLAG(fShowPriceDev);
		__ADD_FLAG(fRestByPaym); // @v11.4.4
		__ADD_FLAG(fInitOrgLot); // @v11.4.4
		__ADD_FLAG(fLotfPrWoTaxes); // @v11.4.4
		#undef __ADD_FLAG
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	return 1;
}
//
// @v12.2.2 int ConvertLandQCertToLotTag() { return (PPError(PPERR_FUNCNOTMORESUPPORTED), 0); }
//
PPViewLot::IterData::IterData() : P_ByTagList(0), P_ByTagExclList(0)
{
}

PPViewLot::IterData::~IterData()
{
	delete P_ByTagList;
	delete P_ByTagExclList;
}

void PPViewLot::IterData::Reset()
{
	PsnNativeCntryList.freeAll();
	NativeCntryList.freeAll();
	IdBySerialList.freeAll();
	IdList.freeAll();
	ZDELETE(P_ByTagList);
	ZDELETE(P_ByTagExclList);
}
//
//
//
PPViewLot::PPViewLot() : PPView(0, &Filt, PPVIEW_LOT, implUseQuickTagEditFunc, 0), // @v11.2.8 implUseQuickTagEditFunc
	P_BObj(BillObj), State(0), P_Tbl(&P_BObj->trfr->Rcpt), P_TempTbl(0), P_SpoilTbl(0), P_PplBlkBeg(0), P_PplBlkEnd(0)
{
	SETFLAG(State, stAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
}

PPViewLot::~PPViewLot()
{
	delete P_TempTbl;
	delete P_SpoilTbl;
	delete P_PplBlkBeg;
	delete P_PplBlkEnd;
}

PPBaseFilt * PPViewLot::CreateFilt(const void * extraPtr) const
{
	LotFilt * p_filt = new LotFilt;
	if(p_filt) {
		if(reinterpret_cast<long>(extraPtr) == 1)
			p_filt->Flags |= LotFilt::fOrders;
		p_filt->Period.upp = getcurdate_();
	}
	return p_filt;
}
//
// LotFiltDialog
// PPViewLot::EditFilt with helpers
//
class LotFiltDialog : public TDialog {
	DECL_DIALOG_DATA(LotFilt);
public:
	enum {
		ctlgroupGoods     = 1,
		ctlgroupGoodsFilt = 2,
		ctrgroupLoc       = 3
	};
 	explicit LotFiltDialog(uint dlgID) : TDialog(dlgID)
	{
		addGroup(ctrgroupLoc, new LocationCtrlGroup(CTLSEL_FLTLOT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0));
		addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(CTLSEL_FLTLOT_GOODS, CTLSEL_FLTLOT_GGRP, cmGoodsFilt));
		SetupCalPeriod(CTLCAL_FLTLOT_PERIOD, CTL_FLTLOT_PERIOD);
		SetupCalPeriod(CTLCAL_FLTLOT_OPERAT, CTL_FLTLOT_OPERAT);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SetPeriodInput(this, CTL_FLTLOT_PERIOD, &Data.Period);
		SetPeriodInput(this, CTL_FLTLOT_OPERAT, &Data.Operation);
		const  PPID suppl_acs_id = (Data.Flags & LotFilt::fOrders) ? GetSellAccSheet() : GetSupplAccSheet();
		setCtrlData(CTL_FLTLOT_CLOSED, &Data.ClosedTag);
		{
			LocationCtrlGroup::Rec loc_rec(&Data.LocList);
			setGroupData(ctrgroupLoc, &loc_rec);
		}
		SetupPPObjCombo(this, CTLSEL_FLTLOT_SPPLPSNCAT, PPOBJ_PRSNCATEGORY, pData->SupplPsnCategoryID, 0); // @v11.4.4
		if((Data.Flags & LotFilt::fOrders) || BillObj->CheckRights(BILLOPRT_ACCSSUPPL, 1)) // @v11.4.4 @fix (Data.Flags & LotFilt::fOrders ||)
			SetupArCombo(this, CTLSEL_FLTLOT_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, suppl_acs_id, sacfDisableIfZeroSheet);
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(ctlgroupGoodsFilt, &gf_rec);
		if(Data.Flags & LotFilt::fOrders) {
			SetupArCombo(this, CTLSEL_FLTLOT_AGENT,  Data.AgentID,  OLW_LOADDEFONOPEN, GetAgentAccSheet(), sacfDisableIfZeroSheet); // @v12.1.6
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 0, LotFilt::fShowBillStatus);
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 1, LotFilt::fShowSerialN);
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 2, LotFilt::fShowAgent); // @v12.1.6
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 3, LotFilt::fCancelledOrdersOnly); // @v12.1.6
		}
		else {
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  0, LotFilt::fWithoutQCert);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  1, LotFilt::fCostAbovePrice);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  2, LotFilt::fWithoutClb);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  3, LotFilt::fDeadLots);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  4, LotFilt::fWithoutExpiry);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  5, LotFilt::fOnlySpoilage);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  6, LotFilt::fShowSerialN);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  7, LotFilt::fSkipNoOp);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  8, LotFilt::fSkipClosedBeforeOp);
			AddClusterAssoc(CTL_FLTLOT_FLAGS,  9, LotFilt::fCheckOriginLotDate);
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 10, LotFilt::fRestByPaym);
			AddClusterAssoc(CTL_FLTLOT_FLAGS, 11, LotFilt::fLotfPrWoTaxes);
		}
		SetClusterData(CTL_FLTLOT_FLAGS, Data.Flags);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GoodsFiltCtrlGroup::Rec gf_rec;
		THROW(GetPeriodInput(this, sel = CTL_FLTLOT_PERIOD, &Data.Period));
		THROW(AdjustPeriodToRights(Data.Period, 1));
		THROW(GetPeriodInput(this, sel = CTL_FLTLOT_OPERAT, &Data.Operation));
		if(!Data.Operation.IsZero())
			THROW(AdjustPeriodToRights(Data.Operation, 1));
		getCtrlData(CTL_FLTLOT_CLOSED,   &Data.ClosedTag);
		{
			LocationCtrlGroup::Rec loc_rec;
			getGroupData(ctrgroupLoc, &loc_rec);
			Data.LocList = loc_rec.LocList;
		}
		getCtrlData(CTLSEL_FLTLOT_SPPLPSNCAT, &Data.SupplPsnCategoryID); // @v11.4.4
		getCtrlData(CTLSEL_FLTLOT_SUPPL, &Data.SupplID);
		THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		Data.GoodsID    = gf_rec.GoodsID;
		// @v12.1.6 {
		if(Data.Flags & LotFilt::fOrders) {
			getCtrlData(CTLSEL_FLTLOT_AGENT, &Data.AgentID);
		}
		// } @v12.1.6 
		GetClusterData(CTL_FLTLOT_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	PPObjGoods GObj;
};

IMPL_HANDLE_EVENT(LotFiltDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmTags)) {
		if(!SETIFZ(Data.P_TagF, new TagFilt()))
			PPError(PPERR_NOMEM);
		else if(!EditTagFilt(PPOBJ_LOT, Data.P_TagF))
			PPError();
		if(Data.P_TagF->IsEmpty())
			ZDELETE(Data.P_TagF);
	}
	else if(event.isCmd(cmaMore)) {
		int    ok = -1;
		SString temp_buf;
		GetClusterData(CTL_FLTLOT_FLAGS, &Data.Flags);
		LotFilt temp_data = Data;
		TDialog * dlg = new TDialog(DLG_FLTLOTEXT);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->disableCtrl(CTL_FLTLOT_EXPIRY, BIN(Data.Flags & LotFilt::fWithoutExpiry));
			if(Data.Flags & LotFilt::fWithoutExpiry)
				dlg->disableCtrl(CTLCAL_FLTLOT_EXPIRY, 1);
			else
				dlg->SetupCalPeriod(CTLCAL_FLTLOT_EXPIRY, CTL_FLTLOT_EXPIRY);
			dlg->SetupCalPeriod(CTLCAL_FLTLOT_QCEXPIRY, CTL_FLTLOT_QCEXPIRY);
			SetPeriodInput(dlg, CTL_FLTLOT_EXPIRY,   &temp_data.ExpiryPrd);
			SetPeriodInput(dlg, CTL_FLTLOT_QCEXPIRY, &temp_data.QcExpiryPrd);
			SetRealRangeInput(dlg, CTL_FLTLOT_COST,  &temp_data.CostRange);
			SetRealRangeInput(dlg, CTL_FLTLOT_PRICE, &temp_data.PriceRange);
			temp_data.GetExtssData(LotFilt::extssSerialText, temp_buf);
			dlg->setCtrlString(CTL_FLTLOT_SERIAL, temp_buf);
			SetupPPObjCombo(dlg, CTLSEL_FLTLOT_INTAXGRP, PPOBJ_GOODSTAX, temp_data.InTaxGrpID, 0, 0);
			dlg->AddClusterAssoc(CTL_FLTLOT_SHOWPRICEDEV, 0, LotFilt::fShowPriceDev);
			dlg->SetClusterData(CTL_FLTLOT_SHOWPRICEDEV, temp_data.Flags);

			dlg->AddClusterAssocDef(CTL_FLTLOT_CDEVRESTR, 0, LotFilt::drNone);
			dlg->AddClusterAssoc(CTL_FLTLOT_CDEVRESTR, 1, LotFilt::drBelow);
			dlg->AddClusterAssoc(CTL_FLTLOT_CDEVRESTR, 2, LotFilt::drAbove);
			dlg->AddClusterAssoc(CTL_FLTLOT_CDEVRESTR, 3, LotFilt::drAny);
			dlg->SetClusterData(CTL_FLTLOT_CDEVRESTR, temp_data.CostDevRestr);

			dlg->AddClusterAssocDef(CTL_FLTLOT_PDEVRESTR, 0, LotFilt::drNone);
			dlg->AddClusterAssoc(CTL_FLTLOT_PDEVRESTR, 1, LotFilt::drBelow);
			dlg->AddClusterAssoc(CTL_FLTLOT_PDEVRESTR, 2, LotFilt::drAbove);
			dlg->AddClusterAssoc(CTL_FLTLOT_PDEVRESTR, 3, LotFilt::drAny);
			dlg->SetClusterData(CTL_FLTLOT_PDEVRESTR, temp_data.PriceDevRestr);
			dlg->AddClusterAssoc(CTL_FLTLOT_EXTVIEWATTR, 0, LotFilt::exvaNone);
			dlg->AddClusterAssoc(CTL_FLTLOT_EXTVIEWATTR, 1, LotFilt::exvaEgaisTags);
			dlg->AddClusterAssoc(CTL_FLTLOT_EXTVIEWATTR, 2, LotFilt::exvaVetisTags);
			dlg->SetClusterData(CTL_FLTLOT_EXTVIEWATTR, temp_data.ExtViewAttr);
			while(ok < 0 && ExecView(dlg) == cmOK)
				if(!(temp_data.Flags & LotFilt::fOrders)) {
					if(!GetPeriodInput(dlg, CTL_FLTLOT_EXPIRY, &temp_data.ExpiryPrd))
						PPErrorByDialog(dlg, CTL_FLTLOT_EXPIRY);
					else if(!GetPeriodInput(dlg, CTL_FLTLOT_QCEXPIRY, &temp_data.QcExpiryPrd))
						PPErrorByDialog(dlg, CTL_FLTLOT_QCEXPIRY);
					else {
						GetRealRangeInput(dlg, CTL_FLTLOT_COST,  &temp_data.CostRange);
						GetRealRangeInput(dlg, CTL_FLTLOT_PRICE, &temp_data.PriceRange);
						dlg->getCtrlString(CTL_FLTLOT_SERIAL, temp_buf);
						temp_data.PutExtssData(LotFilt::extssSerialText, temp_buf);
						dlg->getCtrlData(CTL_FLTLOT_INTAXGRP, &temp_data.InTaxGrpID);

						dlg->GetClusterData(CTL_FLTLOT_SHOWPRICEDEV, &temp_data.Flags);
						dlg->GetClusterData(CTL_FLTLOT_CDEVRESTR, &temp_data.CostDevRestr);
						dlg->GetClusterData(CTL_FLTLOT_PDEVRESTR, &temp_data.PriceDevRestr);
						dlg->GetClusterData(CTL_FLTLOT_EXTVIEWATTR, &temp_data.ExtViewAttr);

						Data = temp_data;
						ok = 1;
					}
				}
		}
		else
			ok = 0;
		delete dlg;
		clearEvent(event);
	}
}

int PPViewLot::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	LotFilt * p_filt = static_cast<LotFilt *>(pBaseFilt);
	uint   dlg_id = (p_filt->Flags & LotFilt::fOrders) ? DLG_FLTLOTORD : DLG_FLTLOT;
	DIALOG_PROC_BODY_P1(LotFiltDialog, dlg_id, p_filt);
}
//
// PPViewLot::MovLotOps with helpers
//
class MovLotOpsDialog : public TDialog {
	DECL_DIALOG_DATA(long);
public:
	MovLotOpsDialog() : TDialog(DLG_MOVLOTOPS)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		AddClusterAssoc(CTL_MOVLOTOPS_MERGE, 0, TMLOF_ADDLOTS);
		SetClusterData(CTL_MOVLOTOPS_MERGE, Data);
		AddClusterAssoc(CTL_MOVLOTOPS_AVG, 0, TMLOF_AVGCOST);
		AddClusterAssoc(CTL_MOVLOTOPS_AVG, 1, TMLOF_AVGPRICE);
		SetClusterData(CTL_MOVLOTOPS_AVG, Data);
		setCtrlUInt16(CTL_MOVLOTOPS_RMVSRC, BIN(Data & TMLOF_RMVSRCLOT));
		setCtrlUInt16(CTL_MOVLOTOPS_RMVREVAL, BIN(Data & (TMLOF_RMVSRCLOT | TMLOF_RMVREVAL)));
		disableCtrl(CTL_MOVLOTOPS_AVG, !BIN(Data & TMLOF_ADDLOTS));
		disableCtrl(CTL_MOVLOTOPS_RMVREVAL, BIN(Data & TMLOF_RMVSRCLOT));
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		GetClusterData(CTL_MOVLOTOPS_MERGE, &Data);
		GetClusterData(CTL_MOVLOTOPS_AVG,   &Data);
		SETFLAG(Data, TMLOF_RMVSRCLOT, getCtrlUInt16(CTL_MOVLOTOPS_RMVSRC));
		if(Data & TMLOF_RMVSRCLOT)
			Data |= TMLOF_RMVREVAL;
		else {
			SETFLAG(Data, TMLOF_RMVREVAL, getCtrlUInt16(CTL_MOVLOTOPS_RMVREVAL));
		}
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_MOVLOTOPS_MERGE) || event.isClusterClk(CTL_MOVLOTOPS_RMVSRC)) {
			long   f = 0;
			getDTS(&f);
			setDTS(&f);
			clearEvent(event);
		}
	}
};

int PPViewLot::MovLotOps(PPID srcLotID)
{
	int    ok = -1;
	ReceiptTbl::Rec lot_rec;
	if(srcLotID && P_Tbl->Search(srcLotID, &lot_rec) > 0) {
		PPObjBill::SelectLotParam slp(lot_rec.GoodsID, lot_rec.LocID, srcLotID, 0);
		if(P_BObj->SelectLot2(slp) > 0) {
			long   mlo_flags = 0;
			if(PPDialogProcBody <MovLotOpsDialog, long>(&mlo_flags) > 0)
				ok = P_BObj->trfr->MoveLotOps(srcLotID, slp.RetLotID, mlo_flags, 1) ? 1 : PPErrorZ();
		}
	}
	return ok;
}
//
//
//
int FASTCALL PPViewLot::AddDerivedLotToTotal(const ReceiptTbl::Rec * pRec)
{
	const double rest = pRec->Rest;
	Total.DCount++;
	Total.DRest += rest;
	if(State & stAccsCost)
		Total.DCost += R5(pRec->Cost) * rest;
	Total.DPrice += R5(pRec->Price) * rest;
	return 1;
}

/*static*/int PPViewLot::CalcChildLots(const ReceiptTbl::Rec * pLotRec, void * extraPtr)
{
	PPViewLot * p_lv = static_cast<PPViewLot *>(extraPtr);
	return (p_lv && pLotRec->LocID != p_lv->Total.LocID) ? p_lv->AddDerivedLotToTotal(pLotRec) : 0;
}

int PPViewLot::CalcTotal(LotTotal::Status stat, LotTotal * pTotal)
{
	int    ok = 1;
	LotViewItem item;
	PPWaitStart();
	if(stat == LotTotal::Undef) {
		MEMSZERO(Total);
	}
	else if(stat == LotTotal::Base) {
		if(Total.Stat == LotTotal::Undef) {
			MEMSZERO(Total);
			for(InitIteration(); NextIteration(&item) > 0;) {
				Total.Count++;
				Total.Qtty  += item.Quantity;
				Total.Rest  += item.Rest;
				Total.OpRestBeg += item.BegRest;
				Total.OpRestEnd += item.EndRest;
				if(State & stAccsCost)
					Total.Cost  += R5(item.Cost) * item.Rest;
				Total.Price += R5(item.Price) * item.Rest;
			}
			Total.Stat = LotTotal::Base;
		}
	}
	else if(stat == LotTotal::Extended) {
		if(oneof2(Total.Stat, LotTotal::Base, LotTotal::Undef)) {
			MEMSZERO(Total);
			for(InitIteration(); NextIteration(&item) > 0;) {
				THROW(PPCheckUserBreak());
				Total.Count++;
				Total.Qtty  += item.Quantity;
				Total.Rest  += item.Rest;
				if(State & stAccsCost)
					Total.Cost += R5(item.Cost) * item.Rest;
				Total.Price += R5(item.Price) * item.Rest;
				THROW(P_BObj->trfr->GetLotPrices(&item, item.Dt, 1));
				if(State & stAccsCost)
					Total.InCost += R5(item.Cost) * item.Quantity;
				Total.InPrice += R5(item.Price) * item.Quantity;
				if(Total.LocID)
					THROW(P_Tbl->GatherChildren(item.ID, 0, PPViewLot::CalcChildLots, this));
			}
			Total.Stat = LotTotal::Extended;
		}
	}
	CATCH
		Total.Stat = LotTotal::Undef;
		ok = 0;
	ENDCATCH
	PPWaitStop();
	if(pTotal)
		memcpy(pTotal, &Total, sizeof(LotTotal));
	return ok;
}

void PPViewLot::ViewTotal()
{
	class LotTotalDialog : public TDialog {
	public:
		explicit LotTotalDialog(PPViewLot * lv) : TDialog(DLG_LOTTOTAL), LV(lv)
		{
		}
		int    setup()
		{
			if(LV->CalcTotal(LotTotal::Base, &Total)) {
				setCtrlLong(CTL_LOTTOTAL_COUNT, Total.Count);
				setCtrlReal(CTL_LOTTOTAL_QTTY,  Total.Qtty);
				setCtrlReal(CTL_LOTTOTAL_REST,  Total.Rest);
				setCtrlReal(CTL_LOTTOTAL_COST,  Total.Cost);
				setCtrlReal(CTL_LOTTOTAL_PRICE, Total.Price);
				setCtrlReal(CTL_LOTTOTAL_OPRESTBEG, Total.OpRestBeg);
				setCtrlReal(CTL_LOTTOTAL_OPRESTEND, Total.OpRestEnd);
				return 1;
			}
			else
				return PPErrorZ();
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmaMore)) {
				if(LV->CalcTotal(LotTotal::Extended, &Total)) {
					TDialog * dlg = 0;
					if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_LOTTOTALE)))) {
						dlg->setCtrlLong(CTL_LOTTOTAL_COUNT,   Total.Count);
						dlg->setCtrlLong(CTL_LOTTOTAL_DCOUNT,  Total.DCount);
						dlg->setCtrlReal(CTL_LOTTOTAL_QTTY,    Total.Qtty);
						dlg->setCtrlReal(CTL_LOTTOTAL_REST,    Total.Rest);
						dlg->setCtrlReal(CTL_LOTTOTAL_COST,    Total.Cost);
						dlg->setCtrlReal(CTL_LOTTOTAL_PRICE,   Total.Price);
						dlg->setCtrlReal(CTL_LOTTOTAL_INCOST,  Total.InCost);
						dlg->setCtrlReal(CTL_LOTTOTAL_INPRICE, Total.InPrice);
						dlg->setCtrlReal(CTL_LOTTOTAL_DREST,   Total.DRest);
						dlg->setCtrlReal(CTL_LOTTOTAL_DCOST,   Total.DCost);
						dlg->setCtrlReal(CTL_LOTTOTAL_DPRICE,  Total.DPrice);
						ExecViewAndDestroy(dlg);
					}
				}
				clearEvent(event);
			}
		}
		PPViewLot * LV;
		LotTotal Total;
	};
	LotTotalDialog * dlg = new LotTotalDialog(this);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setup();
		ExecViewAndDestroy(dlg);
	}
}
//
// PPViewLot::RecoverLots with helpers
//
struct LotRecoverParam {
	LotRecoverParam() : Flags(0), MinusCompensOpID(0)
	{
	}
	long   Flags;
	PPID   MinusCompensOpID;
	SString LogFileName;
};

static int RecoverLotsDialog(LotRecoverParam & rParam)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_CORLOTS);
	if(CheckDialogPtr(&dlg)) {
		SString temp_buf;
		PPLoadString("lotrecoverparam_minuscompensop_hint", temp_buf);
		dlg->setStaticText(CTL_CORLOTS_MCOP_HINT, temp_buf);
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORLOTS_LOG, CTL_CORLOTS_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		PPIDArray op_type_list;
		op_type_list.add(PPOPT_GOODSRECEIPT);
		SetupOprKindCombo(dlg, CTLSEL_CORLOTS_MCOP, rParam.MinusCompensOpID, 0, &op_type_list, 0);
		dlg->setCtrlString(CTL_CORLOTS_LOG, rParam.LogFileName);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  0, TLRF_REPAIR);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  1, TLRF_REPAIRPACK);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  2, TLRF_REPAIRPACKUNCOND);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  3, TLRF_REPAIRCOST);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  4, TLRF_REPAIRPRICE);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  5, TLRF_RMVLOST);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  6, TLRF_CHECKUNIQSERIAL);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  7, TLRF_ADJUNUQSERIAL);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  8, TLRF_INDEPHQTTY);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  9, TLRF_REPAIRWOTAXFLAGS);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 10, TLRF_SETALCCODETOGOODS);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 11, TLRF_SETALCCODETOLOTS);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 12, TLRF_SETINHQCERT);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 13, TLRF_SETALCOMANUF);
		if(!(LConfig.Flags & CFGFLG_USEPACKAGE)) {
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 1, 1);
			rParam.Flags &= ~(TLRF_REPAIRPACK);
		}
		dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 6, BIN(PPObjBill::VerifyUniqSerialSfx(BillObj->GetConfig().UniqSerialSfx) <= 0));
		if(!PPMaster) {
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 3, 1);
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 4, 1);
			rParam.Flags &= ~(TLRF_REPAIRCOST | TLRF_REPAIRPRICE);
		}
		dlg->SetClusterData(CTL_CORLOTS_FLAGS, rParam.Flags);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_CORLOTS_LOG, rParam.LogFileName);
			rParam.LogFileName.Strip();
			rParam.MinusCompensOpID = dlg->getCtrlLong(CTLSEL_CORLOTS_MCOP);
			dlg->GetClusterData(CTL_CORLOTS_FLAGS, &rParam.Flags);
			if(!PPMaster)
				rParam.Flags &= ~(TLRF_REPAIRCOST | TLRF_REPAIRPRICE);
			ok = 1;
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

int PPViewLot::RecoverLots()
{
	int    ok = -1;
	int    ta = 0;
	int    frrl_tag = 0, r;
	long   err_lot_count = 0;
	int    modified = 0;
	EgaisRefACore * p_refa_c = 0;
	LotRecoverParam param;
	PPLogger logger;
	UintHashTable goods_list;
	PPIDArray loc_list;
	SString temp_buf;
	SString msg_buf;
	PPGetFileName(PPFILNAM_LOTERR_LOG, param.LogFileName);
	THROW(r = RecoverLotsDialog(/*log_file_name, &c_flags*/param));
	if(r > 0) {
		PPBillPacket neg_rest_pack;
		PPWaitStart();
		PPLoadText(PPTXT_CHECKLOTS, msg_buf);
		logger.Log(msg_buf);
		{
			Transfer * p_trfr = P_BObj->trfr;
			Reference * p_ref = PPRef;
			SString ref_a;
			SString lot_str;
			SString psn_name_buf;
			TSVector <EgaisRefATbl::Rec> ref_a_list;
			PrcssrAlcReport::Config alc_cfg;
			PPIDArray psn_ref_list;
			PPIDArray lot_id_list;
			PPID   manuf_tag_id = 0;
			if(param.Flags & TLRF_SETALCOMANUF) {
				PrcssrAlcReport::ReadConfig(&alc_cfg);
				if(alc_cfg.ManufImpTagID)
					manuf_tag_id = alc_cfg.ManufImpTagID;
				else if(alc_cfg.LotManufTagList.getCount())
					manuf_tag_id = alc_cfg.LotManufTagList.get(0);
				if(manuf_tag_id)
					p_refa_c = new EgaisRefACore;
			}
			{
				LotViewItem  lv_item;
				for(InitIteration(); NextIteration(&lv_item) > 0;)
					lot_id_list.add(lv_item.ID);
			}
			if(param.Flags & (TLRF_REPAIR|TLRF_ADJUNUQSERIAL|TLRF_SETALCCODETOGOODS|TLRF_SETALCCODETOLOTS)) {
				THROW(PPStartTransaction(&ta, 1));
				THROW(P_BObj->atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
				if(param.MinusCompensOpID && /*Filt.LocID*/LocList.getSingle()) {
					THROW(neg_rest_pack.CreateBlank2(param.MinusCompensOpID, getcurdate_(), /*Filt.LocID*/LocList.getSingle(), 0));
				}
			}
			for(uint ididx = 0; ididx < lot_id_list.getCount(); ididx++) {
				const  PPID _lot_id = lot_id_list.get(ididx);
				ReceiptTbl::Rec lot_rec;
				if(p_trfr->Rcpt.Search(_lot_id, &lot_rec) > 0) {
					PPLotFaultArray ary(_lot_id, logger);
					{
						lot_str.Z().Cat(_lot_id).Space();
						lot_str.CatChar('[').Cat(lot_rec.Dt).CatDiv('-', 1);
						GetGoodsName(labs(lot_rec.GoodsID), temp_buf);
						lot_str.Cat(temp_buf);
						lot_str.CatChar(']');
					}
					if(param.Flags & TLRF_SETALCOMANUF && p_refa_c) {
						if(p_ref->Ot.GetTagStr(PPOBJ_LOT, _lot_id, PPTAG_LOT_FSRARINFA, ref_a) > 0) {
							char   manuf_rar_ident[32];
							char   importer_rar_ident[32];
							char   shipper_rar_ident[32];
							int    ambig_manuf = 0;
							int    ambig_imptr = 0;
							int    ambig_shipper = 0;
							int    is_foreign_country = 0; // !643
							manuf_rar_ident[0] = 0;
							importer_rar_ident[0] = 0;
							shipper_rar_ident[0] = 0;
							ObjTagItem ex_lot_manuf_tag_item;
							PPID   ex_lot_manuf_id = 0;
							if(p_ref->Ot.GetTag(PPOBJ_LOT, _lot_id, manuf_tag_id, &ex_lot_manuf_tag_item) > 0) {
								ex_lot_manuf_tag_item.GetInt(&ex_lot_manuf_id);
							}
							if(p_refa_c->SearchByCode(ref_a, ref_a_list) > 0) {
								for(uint rlidx = 0; rlidx < ref_a_list.getCount(); rlidx++) {
									const EgaisRefATbl::Rec & r_ref_a_rec = ref_a_list.at(rlidx);
									if(r_ref_a_rec.ManufRarIdent[0]) {
										if(!manuf_rar_ident[0])
											STRNSCPY(manuf_rar_ident, r_ref_a_rec.ManufRarIdent);
										else
											ambig_manuf = 1;
									}
									if(r_ref_a_rec.ImporterRarIdent[0]) {
										if(!importer_rar_ident[0])
											STRNSCPY(importer_rar_ident, r_ref_a_rec.ImporterRarIdent);
										else
											ambig_imptr = 1;
									}
									if(r_ref_a_rec.ShipperRarIdent[0]) {
										if(!shipper_rar_ident[0])
											STRNSCPY(shipper_rar_ident, r_ref_a_rec.ShipperRarIdent);
										else
											ambig_shipper = 1;
									}
									if(r_ref_a_rec.CountryCode && r_ref_a_rec.CountryCode != 643)
										is_foreign_country = 1;
								}
								PPID   manuf_id_to_set = 0;
								if(importer_rar_ident[0]) {
									if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, PPTAG_PERSON_FSRARID, importer_rar_ident, &psn_ref_list) > 0) {
										assert(psn_ref_list.getCount());
										if(psn_ref_list.getCount()) {
											if(!ex_lot_manuf_id || !psn_ref_list.lsearch(ex_lot_manuf_id)) {
												PPLoadString(PPSTR_ERROR, PPERR_ELOT_DIFEGAISIMPTR, temp_buf);
												msg_buf.Printf(temp_buf, lot_str.cptr(), importer_rar_ident);
												logger.Log(msg_buf);
												// PPERR_ELOT_DIFEGAISIMPTR
												// Справка А лота '%s' ссылается на импортера '%s', отличного от того, что установлен в лоте
												manuf_id_to_set = psn_ref_list.get(0);
											}
										}
									}
									else {
										PPLoadString(PPSTR_ERROR, PPERR_ELOT_EGAISIMPTRCODENOTFOUNT, temp_buf);
										msg_buf.Printf(temp_buf, lot_str.cptr(), importer_rar_ident);
										logger.Log(msg_buf);
										// PPERR_ELOT_EGAISIMPTRCODENOTFOUNT
										// Справка А лота '%s' ссылается на импортера с кодом '%s', но соответствующая персоналия не в базе данных не найдена
									}
								}
								else if(is_foreign_country && shipper_rar_ident[0]) {
									if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, PPTAG_PERSON_FSRARID, shipper_rar_ident, &psn_ref_list) > 0) {
										assert(psn_ref_list.getCount());
										if(psn_ref_list.getCount()) {
											if(!ex_lot_manuf_id || !psn_ref_list.lsearch(ex_lot_manuf_id)) {
												PPLoadString(PPSTR_ERROR, PPERR_ELOT_DIFEGAISIMPTR, temp_buf);
												msg_buf.Printf(temp_buf, lot_str.cptr(), shipper_rar_ident);
												logger.Log(msg_buf);
												// PPERR_ELOT_DIFEGAISIMPTR
												// Справка А лота '%s' ссылается на импортера '%s', отличного от того, что установлен в лоте
												manuf_id_to_set = psn_ref_list.get(0);
											}
										}
									}
									else {
										PPLoadString(PPSTR_ERROR, PPERR_ELOT_EGAISIMPTRCODENOTFOUNT, temp_buf);
										msg_buf.Printf(temp_buf, lot_str.cptr(), shipper_rar_ident);
										logger.Log(msg_buf);
										// PPERR_ELOT_EGAISIMPTRCODENOTFOUNT
										// Справка А лота '%s' ссылается на импортера с кодом '%s', но соответствующая персоналия не в базе данных не найдена
									}
								}
								else if(manuf_rar_ident[0]) {
									if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, PPTAG_PERSON_FSRARID, manuf_rar_ident, &psn_ref_list) > 0) {
										assert(psn_ref_list.getCount());
										if(psn_ref_list.getCount()) {
											if(!ex_lot_manuf_id || !psn_ref_list.lsearch(ex_lot_manuf_id)) {
												PPLoadString(PPSTR_ERROR, PPERR_ELOT_DIFEGAISMANIF, temp_buf);
												msg_buf.Printf(temp_buf, lot_str.cptr(), manuf_rar_ident);
												logger.Log(msg_buf);
												// PPERR_ELOT_DIFEGAISMANIF
												// Справка А лота '%s' ссылается на производителя '%s', отличного от того, что установлен в лоте
												manuf_id_to_set = psn_ref_list.get(0);
											}
										}
									}
									else {
										PPLoadString(PPSTR_ERROR, PPERR_ELOT_EGAISMANUFCODENOTFOUNT, temp_buf);
										msg_buf.Printf(temp_buf, lot_str.cptr(), manuf_rar_ident);
										logger.Log(msg_buf);
										// PPERR_ELOT_EGAISMANUFCODENOTFOUNT
										// Справка А лота '%s' ссылается на производителя с кодом '%s', но соответствующая персоналия не в базе данных не найдена
									}
								}
								if(param.Flags & TLRF_REPAIR) {
									if(manuf_id_to_set) {
										ObjTagItem lot_manuf_tag_item;
										if(!lot_manuf_tag_item.SetInt(manuf_tag_id, manuf_id_to_set)) {
											logger.LogLastError();
										}
										else if(!p_ref->Ot.PutTag(PPOBJ_LOT, _lot_id, &lot_manuf_tag_item, 0)) {
											logger.LogLastError();
										}
									}
								}
							}
						}
					}
					THROW(r = p_trfr->CheckLot(_lot_id, 0, param.Flags, ary));
					if(r < 0) {
						err_lot_count++;
						ary.AddMessage();
						if(param.Flags & (TLRF_REPAIR|TLRF_ADJUNUQSERIAL|TLRF_SETALCCODETOGOODS|TLRF_SETALCCODETOLOTS)) {
							THROW(p_trfr->RecoverLot(_lot_id, &ary, param.Flags, 0));
							modified = 1;
						}
					}
					{
						PPLotFault nf;
						uint   nf_pos = 0;
						if(neg_rest_pack.Rec.OpID && lot_rec.GoodsID > 0 && lot_rec.Rest < 0.0 && ary.HasFault(PPLotFault::NegativeRest, &nf, &nf_pos)) {
							if(ary.getCount() == 1) {
								PPTransferItem ti(&neg_rest_pack.Rec, TISIGN_UNDEF);
								THROW(ti.SetupGoods(lot_rec.GoodsID, 0));
								THROW(ti.SetupLot(_lot_id, &lot_rec, 0));
								ti.Flags &= ~PPTFR_RECEIPT;
								ti.Quantity_ = -lot_rec.Rest;
								ti.TFlags |= PPTransferItem::tfForceNoRcpt;
								THROW(neg_rest_pack.InsertRow(&ti, 0, 0));
							}
						}
					}
					goods_list.Add(static_cast<ulong>(labs(lot_rec.GoodsID)));
					loc_list.addUnique(lot_rec.LocID);
				}
				PPWaitPercent(ididx+1, lot_id_list.getCount());
			}
			if(neg_rest_pack.GetTCount()) {
				neg_rest_pack.InitAmounts();
				THROW(P_BObj->FillTurnList(&neg_rest_pack));
				THROW(P_BObj->TurnPacket(&neg_rest_pack, 0));
			}
			THROW(P_BObj->atobj->P_Tbl->LockingFRR(0, &frrl_tag, 0));
			THROW(PPCommitWork(&ta));
		}
		{
			//
			// Функция CorrectCurRest выполняет исправление в отдельной транзакции,
			// по этому, этот блок вынесен за пределы общей транзакции
			//
			IterCounter cntr;
			cntr.Init(static_cast<long>(goods_list.GetCount()));
			for(ulong goods_id = 0; goods_list.Enum(&goods_id);) {
				if(!P_BObj->trfr->CorrectCurRest(static_cast<PPID>(goods_id), &loc_list, &logger, BIN(param.Flags & TLRF_REPAIR)))
					logger.LogLastError();
				PPWaitPercent(cntr.Increment());
			}
		}
		logger.Save(param.LogFileName, 0);
		PPWaitStop();
	}
	if(modified) {
		CalcTotal(LotTotal::Undef, 0);
		ok = 1;
	}
	CATCH
		P_BObj->atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	delete p_refa_c;
	return ok;
}

int PPViewLot::EditLot(PPID id)
{
	int    ok = 0;
	int    valid = 0;
	uint   pos = 0;
	uint   del = 0;
	LotViewItem lvi;
	PPBillPacket billp;
	PPTransferItem * p_ti;
	double cost;
	double price;
	double qtty;
	double upp;
	TDialog * dlg = 0;
	SString goods_name, suppl_name;
	THROW(GetItem(id, &lvi) > 0)
	THROW(P_BObj->ExtractPacket(lvi.BillID, &billp) > 0)
	if(billp.SearchLot(id, &pos) == 0) {
		if(IsIntrExpndOp(billp.Rec.OpID)) {
			THROW_PP(billp.SearchLot(lvi.PrevLotID, &pos), PPERR_LOTNOTBELONGTOBILL);
		}
		else {
			CALLEXCEPT_PP(PPERR_LOTNOTBELONGTOBILL);
		}
	}
	//THROW_PP(billp.SearchLot(id, &pos) > 0, PPERR_LOTNOTBELONGTOBILL);
	p_ti  = &billp.TI(pos);
	qtty  = p_ti->Quantity_;
	price = p_ti->Price;
	cost  = (State & stAccsCost) ? p_ti->Cost : 0.0;
	upp   = p_ti->UnitPerPack;
	GetGoodsName(lvi.GoodsID, goods_name);
	GetArticleName(lvi.SupplID, suppl_name);
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_EDITLOT))))
	dlg->setCtrlData(CTL_EDITLOT_COST,        &cost);
	dlg->setCtrlData(CTL_EDITLOT_PRICE,       &price);
	dlg->setCtrlData(CTL_EDITLOT_QUANTITY,    &qtty);
	dlg->setCtrlData(CTL_EDITLOT_UNITPERPACK, &upp);
	dlg->setCtrlString(CTL_EDITLOT_NAME,      goods_name);
	dlg->setCtrlData(CTL_EDITLOT_BILLNO,      billp.Rec.Code);
	dlg->setCtrlString(CTL_EDITLOT_SUPPL,     suppl_name);
	dlg->setCtrlData(CTL_EDITLOT_LOTID,       &id);
	dlg->disableCtrl(CTL_EDITLOT_NAME,   1);
	dlg->disableCtrl(CTL_EDITLOT_BILLNO, 1);
	dlg->disableCtrl(CTL_EDITLOT_SUPPL,  1);
	dlg->disableCtrl(CTL_EDITLOT_LOTID,  1);
	do {
		if((ok = ExecView(dlg)) == cmOK && State & stAccsCost) {
			dlg->getCtrlData(CTL_EDITLOT_COST,        &cost);
			dlg->getCtrlData(CTL_EDITLOT_PRICE,       &price);
			dlg->getCtrlData(CTL_EDITLOT_QUANTITY,    &qtty);
			dlg->getCtrlData(CTL_EDITLOT_UNITPERPACK, &upp);
			dlg->getCtrlData(CTL_EDITLOT_DELETE,      &del);
			valid = (del || (cost != 0.0) && (price != 0.0) && (qtty != 0.0));
		}
		else
			valid = 1;
	} while(!valid);
	if(ok == cmOK && State & stAccsCost) {
		PPWaitStart();
		if(del) {
			THROW(billp.RemoveRow(pos));
			billp.InitAmounts();
			THROW(P_BObj->FillTurnList(&billp));
			THROW(P_BObj->UpdatePacket(&billp, 1))
		}
		else if(p_ti->Price != price || p_ti->Cost != cost || p_ti->Quantity_ != qtty || p_ti->UnitPerPack != upp) {
			p_ti->Quantity_ = qtty;
   			p_ti->Price = price;
			p_ti->Cost  = cost;
			p_ti->UnitPerPack = upp;
			billp.InitAmounts();
			THROW(P_BObj->FillTurnList(&billp));
			THROW(P_BObj->UpdatePacket(&billp, 1))
		}
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOKPPERR
	PPWaitStop();
	delete dlg;
	return ok;
}
//
//
//
int PPViewLot::GetItem(PPID lotID, LotViewItem * pItem)
{
	memzero(pItem, sizeof(*pItem));
	if(P_Tbl->Search(lotID, pItem) > 0) {
		if(P_TempTbl) {
			TempLotTbl::Key0 k0;
			k0.LotID = lotID;
			if(P_TempTbl->search(0, &k0, spEq)) {
				if(pItem) {
					STRNSCPY(pItem->Serial, P_TempTbl->data.Serial);
					pItem->BegRest = P_TempTbl->data.BegRest;
					pItem->EndRest = P_TempTbl->data.EndRest;
				}
			}
		}
		return 1;
	}
	else
		return -1;
}

int PPViewLot::ViewBillInfo(PPID billID)
{
	return P_BObj->ViewBillInfo(billID);
}

int PPViewLot::Init_(const PPBaseFilt * pFilt)
{
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	ZDELETE(P_PplBlkBeg);
	ZDELETE(P_PplBlkEnd);
	MEMSZERO(Total);
	Counter.Init();
	SupplList.Set(0);

	int    ok = 1;
	SString temp_buf;
	PPObjLocation loc_obj;
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	Filt.ExpiryPrd.Actualize(ZERODATE);
	Filt.QcExpiryPrd.Actualize(ZERODATE);
	SETFLAG(State, stNoTempTbl, BIN(Filt.Flags & LotFilt::fNoTempTable));
	Filt.GetExtssData(LotFilt::extssSerialText, temp_buf);
	SETFLAG(State, stFiltSerial, temp_buf.NotEmptyS());

	Itd.Reset();
	if(GObj.IsGeneric(Filt.GoodsID)) {
		Filt.GoodsGrpID = Filt.GoodsID;
		Filt.GoodsID = 0;
	}
	if(Filt.Flags & LotFilt::fOnlySpoilage)
		P_SpoilTbl = new SpecSeriesCore;
	//
	// Инициализируем список складов, по которым следует поднимать выборку.
	// Учитываются доступные склады в правах доступа.
	//
	LocList.freeAll();
	if(Filt.LocID_Obsolete && Filt.LocList.IsEmpty())
		Filt.LocList.Add(Filt.LocID_Obsolete);
	if(Filt.LocList.IsEmpty()) {
		loc_obj.GetWarehouseList(&LocList, 0);
	}
	else {
		const PPIDArray & r_loc_list = Filt.LocList.Get();
		loc_obj.ResolveWarehouseList(&r_loc_list, LocList); // @v12.1.5
		/*@v12.1.6 for(uint locidx = 0; locidx < Filt.LocList.GetCount(); locidx++) {
			const  PPID loc_id = Filt.LocList.Get(locidx);
			if(loc_id && ObjRts.CheckLocID(loc_id, 0))
				LocList.add(loc_id);
		}*/
	}
	if(Filt.ParentLotID) {
		P_Tbl->GatherChildren(Filt.ParentLotID, &Itd.IdList, 0, 0);
	}
	else {
		//
		// Если фильтр требует проверки на наличие ГТД у лотов, то инициализируем список
		// идентификаторов собственного государства так как для лотов, относящихся к товарам,
		// произведенным в собственном государстве не требуется ГТД.
		//
		if(Filt.Flags & LotFilt::fWithoutClb) {
			SString native_country_name;
			PPObjWorld::GetNativeCountryName(native_country_name);
			BtrDbKey k_;
			{
				PersonTbl * p_psn_tbl = PsnObj.P_Tbl;
				BExtQuery psn_cntr_q(p_psn_tbl, 0);
				psn_cntr_q.select(p_psn_tbl->ID, p_psn_tbl->Name, 0L).where(p_psn_tbl->Status == PPPRS_COUNTRY);
				for(psn_cntr_q.initIteration(0, k_, spGt); psn_cntr_q.nextIteration() > 0;)
					if(native_country_name.CmpNC(p_psn_tbl->data.Name) == 0)
						Itd.PsnNativeCntryList.add(p_psn_tbl->data.ID);
			}
			{
				PPObjWorld w_obj;
				SVector list(sizeof(WorldTbl::Rec));
				w_obj.GetListByName(WORLDOBJ_COUNTRY, native_country_name, &list);
				for(uint i = 0; i < list.getCount(); i++)
					Itd.NativeCntryList.add(static_cast<const WorldTbl::Rec *>(list.at(i))->ID);
			}
		}
		{
			Filt.GetExtssData(LotFilt::extssSerialText, temp_buf);
			if(temp_buf.NotEmptyS())
				P_BObj->SearchLotsBySerial(temp_buf, &Itd.IdBySerialList);
		}
		// @v7.7.8 {
		/* @construction
		if(Filt.P_TagF && !Filt.P_TagF->IsEmpty()) {
			PPObjTag tag_obj;
			UintHashTable _list, _excl_list;
			if(tag_obj.GetObjListByFilt(PPOBJ_LOT, Filt.P_TagF, _list, _excl_list) > 0) {
				if(_excl_list.GetCount()) {
					SETIFZ(p_excl_list, new PPIDArray);
					THROW_MEM(p_excl_list);
					for(ulong _v = 0; _excl_list.Enum(&_v);)
						p_excl_list->addUnique((PPID)_v);
				}
			}
		}
		@construction */
		// } @v7.7.8
		if(Filt.SupplID) {
			PPObjPersonRelType prt_obj;
			PPIDArray grp_prt_list;
			if(prt_obj.GetGroupingList(&grp_prt_list) > 0) {
				PPIDArray list;
				for(uint i = 0; i < grp_prt_list.getCount(); i++)
					ArObj.GetRelPersonList(Filt.SupplID, grp_prt_list.get(i), 1, &list);
				if(list.getCount())
					SupplList.Set(&list);
			}
			if(!SupplList.IsExists())
				SupplList.Add(Filt.SupplID);
		}
	}
	if(IsTempTblNeeded()) {
		THROW(CreateTempTable());
	}
	CATCHZOK
	return ok;
}

bool PPViewLot::IsTempTblNeeded() const
{
	bool   yes = false;
	if(!(State & stNoTempTbl)) {
		const long tf = (LotFilt::fOrders|LotFilt::fDeadLots|LotFilt::fWithoutClb|
			LotFilt::fOnlySpoilage|LotFilt::fCheckOriginLotDate|LotFilt::fShowPriceDev|LotFilt::fLotfPrWoTaxes);
		if((!Filt.Operation.IsZero() || (Filt.Flags & tf) || (!Filt.GoodsID && Filt.GoodsGrpID) || !Filt.QcExpiryPrd.IsZero()) ||
			SupplList.GetCount() > 1 || Filt.ParentLotID || (Filt.P_TagF && !Filt.P_TagF->IsEmpty()) || State & stFiltSerial)
			yes = true;
		else {
			SString temp_buf;
			Filt.GetExtssData(LotFilt::extssSerialText, temp_buf);
			if(temp_buf.NotEmptyS())
				yes = true;
		}
	}
	return yes;
}

int PPViewLot::PutAllToBasket()
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	param.SelPrice = 2;
	THROW(r = GetBasketByDialog(&param, GetSymb()));
	if(r > 0) {
		LotViewItem item;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0;) {
			ILTI   i_i;
			i_i.GoodsID     = labs(item.GoodsID);
			i_i.UnitPerPack = item.UnitPerPack;
			i_i.Cost        = R5(item.Cost);
			switch(param.SelPrice) {
				case 1:  i_i.Price = R5(item.Cost);  break;
				case 2:  i_i.Price = R5(item.Price); break;
				case 3:  i_i.Price = item.Price; break;
				default: i_i.Price = item.Price; break;
			}
			i_i.Price = (i_i.Price == 0.0) ? item.Price : i_i.Price;
			i_i.CurPrice = 0.0;
			i_i.Flags    = 0;
			i_i.Suppl    = item.SupplID;
			i_i.QCert    = item.QCertID;
			i_i.Expiry   = item.Expiry;
			i_i.Quantity = (param.Flags & SelBasketParam::fUseGoodsRestAsQtty) ? fabs(item.Rest) : fabs(item.Quantity);
			THROW(param.Pack.AddItem(&i_i, 0, param.SelReplace));
		}
		PPWaitStop();
		THROW(GoodsBasketDialog(param, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewLot::MakeLotListForEgaisRetReg2ToWh(PPEgaisProcessor & rEp, PPID opID, PPID locID, RAssocArray & rList)
{
	int    ok = -1;
	if(opID) {
		Reference * p_ref = PPRef;
		const LDATE _curdate = getcurdate_();
		PPViewBill bill_view;
		BillFilt bill_filt;
		bill_filt.OpID = opID;
		if(locID)
			bill_filt.LocList.Add(locID);
		if(!Filt.Operation.IsZero())
			bill_filt.Period = Filt.Operation;
		else
			bill_filt.Period.Set(plusdate(_curdate, -3), _curdate);
		if(bill_view.Init_(&bill_filt)) {
			BillViewItem bill_item;
			PPBillPacket bp;
			SString ref_b;
			SString egais_code;
			LotExtCodeCore * p_lec_t = P_BObj->P_LotXcT;
			for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&bill_item) > 0;) {
				if(P_BObj->ExtractPacket(bill_item.ID, &bp) > 0) {
					for(uint i = 0; i < bp.GetTCount(); i++) {
						const PPTransferItem & r_ti = bp.ConstTI(i);
						if(r_ti.GoodsID > 0 && r_ti.LotID && rEp.IsAlcGoods(r_ti.GoodsID)) {
							int    is_lot_in_3format = 0;
							if(p_lec_t) {
								int16 row_idx = 0;
								int   row_is_found = 0;
								for(int   rbb_iter = 0; !row_is_found && P_BObj->trfr->EnumItems(bp.Rec.ID, &rbb_iter, 0) > 0;) {
									row_idx++;
									if(rbb_iter == r_ti.RByBill)
										row_is_found = 1;
								}
								if(row_is_found) {
									LotExtCodeTbl::Key2 k2;
									MEMSZERO(k2);
									k2.BillID = bp.Rec.ID;
									k2.RByBill = row_idx;
									if(p_lec_t->search(2, &k2, spGe) && p_lec_t->data.BillID == bp.Rec.ID && p_lec_t->data.RByBill == row_idx) do {
										if(p_lec_t->data.Code[0])
											is_lot_in_3format = 1;
									} while(!is_lot_in_3format && p_lec_t->search(2, &k2, spGe) && p_lec_t->data.BillID == bp.Rec.ID && p_lec_t->data.RByBill == row_idx);
								}
							}
							if(!is_lot_in_3format) {
								p_ref->Ot.GetTagStr(PPOBJ_LOT, r_ti.LotID, PPTAG_LOT_FSRARINFB, ref_b);
								p_ref->Ot.GetTagStr(PPOBJ_LOT, r_ti.LotID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
								if(ref_b.NotEmpty() && egais_code.NotEmpty()) {
									rList.Add(r_ti.LotID, fabs(r_ti.Quantity_));
									ok = 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPViewLot::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		Reference * p_ref = PPRef;
		PPID   lot_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			// @debug {
			case PPVCMD_TEST:
				if(Filt.Operation.low && Filt.Operation.upp) {
					SString temp_buf;
					PPGetFilePath(PPPATH_OUT, "EvaluateAverageRestByLot-debug.txt", temp_buf);
					SFile f_out(temp_buf, SFile::mWrite);
					if(f_out.IsValid()) {
						LotViewItem view_item;
						for(InitIteration(); NextIteration(&view_item) > 0;) {
							double avg_rest = 0.0;
							P_BObj->trfr->EvaluateAverageRestByLot(view_item.ID, Filt.Operation, &avg_rest);
							if(view_item.BegRest != 0.0 || view_item.EndRest != 0.0 || avg_rest != 0.0) {
								temp_buf.Z().CatEq("id", view_item.ID).Space().CatEq("b", view_item.BegRest, MKSFMTD_030).Space().
									CatEq("e", view_item.EndRest, MKSFMTD_030).Space().CatEq("avg", avg_rest, MKSFMTD_030);
								f_out.WriteLine(temp_buf.CR());
							}
						}
					}
				}
				break;
			// } @debug 
			case PPVCMD_EDITGOODS:
				ok = -1;
				{
					LotViewItem item;
					GetItem(lot_id, &item);
					if(GObj.Edit(&item.GoodsID, 0) == cmOK)
						ok = 1;
				}
				break;
			case PPVCMD_VIEWCHILDS:
				ok = -1;
				if(lot_id) {
					LotFilt temp_filt;
					temp_filt.ParentLotID = lot_id;
					ViewLots(&temp_filt, 0, 1);
				}
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(lot_id) {
					LotViewItem item;
					if(GetItem(lot_id, &item) > 0) {
						LotFilt temp_filt;
						temp_filt.Copy(&Filt, 1);
						temp_filt.Period.Z();
						temp_filt.ExpiryPrd.Z();
						temp_filt.QcExpiryPrd.Z();
						temp_filt.GoodsID = item.GoodsID;
						temp_filt.LocList.Add(item.LocID);
						ViewLots(&temp_filt, 0, 1);
					}
				}
				break;
			case PPVCMD_ADDEDINFO:
				if(P_BObj->EditLotExtData(lot_id) > 0) {
					CalcTotal(LotTotal::Undef, 0);
					ok = 1;
				}
				else
					ok = -1;
				break;
			case PPVCMD_SYSTEMINFO: ok = P_BObj->EditLotSystemInfo(lot_id); break;
			case PPVCMD_DORETURN: ok = P_BObj->AddRetBillByLot(lot_id); break;
			case PPVCMD_MOVLOTOP: ok = MovLotOps(lot_id); break;
			case PPVCMD_DORECOVER: ok = RecoverLots(); break;
			case PPVCMD_QUICKTAGEDIT: // @v11.2.8
				// В этой команде указатель pHdr занят под список идентификаторов тегов, соответствующих нажатой клавише
				// В связи с этим текущий элемент таблицы придется получить явным вызовом pBrw->getCurItem()
				//
				{
					const BrwHdr * p_row = static_cast<const BrwHdr *>(pBrw->getCurItem());
					ok = PPView::Helper_ProcessQuickTagEdit(PPObjID(PPOBJ_LOT, p_row ? p_row->ID : 0), pHdr/*(LongArray *)*/);
				}
				break;
			case PPVCMD_TAGS: ok = EditObjTagValList(PPOBJ_LOT, lot_id, 0); break;
			case PPVCMD_EXTUPDATE:
				ok = -1;
				if(lot_id)
					ok = EditLot(lot_id);
				break;
			case PPVCMD_PUTTOBASKET:
				ok = -1;
				{
					LotViewItem lvi;
					if(lot_id && GetItem(lot_id, &lvi) > 0) {
						if(!(Filt.Flags & LotFilt::fOrders)) {
							lvi.Quantity = 0.0;
							lvi.Price    = 0.0;
						}
						AddGoodsToBasket(lvi.GoodsID, lvi.LocID, lvi.Quantity, lvi.Price);
					}
				}
				break;
			case PPVCMD_PUTTOBASKETALL:
				ok = -1;
				PutAllToBasket();
				break;
			case PPVCMD_VIEWCOMPLETE:
				ok = -1;
				if(lot_id)
					P_BObj->ViewLotComplete(lot_id, 0);
				break;
			case PPVCMD_VIEWSPOILTSER:
				ok = -1;
				if(lot_id && P_SpoilTbl) {
					SString serial;
					if(P_BObj->GetSerialNumberByLot(lot_id, serial, 0) > 0) {
						P_BObj->ReleaseSerialFromUniqSuffix(serial);
						ViewSpoilList(P_SpoilTbl, serial, 0);
					}
				}
				break;
			case PPVCMD_PRINTLABEL:
				ok = -1;
				if(lot_id)
					BarcodeLabelPrinter::PrintLotLabel(lot_id);
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_EXPGOODSLABEL:
				ok = -1;
				ExportGoodsLabelData();
				break;
			case PPVCMD_REVALCOST:
				ok = -1;
				RevalCostByLots();
				break;
			case PPVCMD_EDITPERSON:
				ok = -1;
				{
					ArticleTbl::Rec  ar_rec;
					LotViewItem  item;
					GetItem(lot_id, &item);
					if(item.SupplID && ArObj.Fetch(item.SupplID, &ar_rec) > 0 && ar_rec.ObjID) {
						PsnObj.Edit(&ar_rec.ObjID, 0);
					}
				}
				break;
			case PPVCMD_TAGSALL:
				ok = -1;
				{
					const  PPID obj_type = PPOBJ_LOT;
					ObjTagList common_tag_list;
					common_tag_list.Oid.Obj = obj_type;
					int   update_mode = ObjTagList::mumAdd;
					if(EditObjTagValUpdateList(&common_tag_list, 0, &update_mode) > 0 && common_tag_list.GetCount()) {
						LotViewItem item;
						PPTransaction tra(1);
						THROW(tra);
						for(InitIteration(PPViewLot::OrdByDefault); NextIteration(&item) > 0;) {
							ObjTagList local_tag_list;
							THROW(p_ref->Ot.GetList(obj_type, item.ID, &local_tag_list));
							if(local_tag_list.Merge(common_tag_list, update_mode) > 0) {
								THROW(p_ref->Ot.PutList(obj_type, item.ID, &local_tag_list, 0));
							}
							PPWaitPercent(GetCounter());
						}
						THROW(tra.Commit());
					}
				}
				break;
			case PPVCMD_CHANGESTATUS: // @v11.1.6
				ok = -1;
				if(lot_id) {
					LotViewItem  item;
					GetItem(lot_id, &item);
					if(item.BillID)
						ok = P_BObj->EditBillStatus(item.BillID);
				}
				break;
			case PPVCMD_LOTEXTCODE:
				if(lot_id) {
					PPView::Execute(PPVIEW_LOTEXTCODE, 0, 0 /* modal */, reinterpret_cast<void *>(lot_id));
				}
				break;
			case PPVCMD_CHANGECLOSEPAR:
				Filt.ClosedTag = (Filt.ClosedTag == 1) ? 0 : 1;
				ok = ChangeFilt(1, pBrw);
				break;
            case PPVCMD_CREATESPCREST:
				{
					long   selection = 0; // PPEDIOP_EGAIS_ACTCHARGEON - по справкам Б, PPEDIOP_EGAIS_ACTCHARGEONSHOP - торговый зал (регистр 2) по кодам ЕГАИС
					{
						TDialog * dlg = new TDialog(DLG_SELEGAISCHRGON);
						if(CheckDialogPtrErr(&dlg)) {
							dlg->AddClusterAssocDef(CTL_SELEGAISCHRGON_WHAT,  0, PPEDIOP_EGAIS_ACTCHARGEON);
							dlg->AddClusterAssoc(CTL_SELEGAISCHRGON_WHAT,  1, PPEDIOP_EGAIS_ACTCHARGEONSHOP);
							dlg->AddClusterAssoc(CTL_SELEGAISCHRGON_WHAT,  2, PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000);
							dlg->SetClusterData(CTL_SELEGAISCHRGON_WHAT, selection);
							if(ExecView(dlg) == cmOK)
								selection = dlg->GetClusterData(CTL_SELEGAISCHRGON_WHAT);
							else
								selection = 0;
						}
						ZDELETE(dlg);
					}
					if(oneof3(selection, PPEDIOP_EGAIS_ACTCHARGEON, PPEDIOP_EGAIS_ACTCHARGEONSHOP, (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000))) {
						const LDATE _curdate = getcurdate_();
						PPEgaisProcessor ep(PPEgaisProcessor::cfUseVerByConfig, 0, 0);
						SString temp_buf;
						SString egais_code;
						SString ref_a;
						SString ref_b;
						LotViewItem item;
						PPIDArray lot_list;
						RAssocArray ret_bill_lot_list;
						const  PPID loc_id = NZOR(LocList.getSingle(), LConfig.Location);
						THROW(ep);
						PPWaitStart();
						if(selection == (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000)) {
							// Защита на случай если ep.GetConfig().SupplRetOpID == ep.GetConfig().IntrExpndOpID
							const  PPID suppl_ret_op_id = ep.GetConfig().SupplRetOpID;
							const  PPID intr_expnd_op_id = ep.GetConfig().IntrExpndOpID;
							if(suppl_ret_op_id)
								MakeLotListForEgaisRetReg2ToWh(ep, suppl_ret_op_id,  loc_id, ret_bill_lot_list);
							if(intr_expnd_op_id && intr_expnd_op_id != suppl_ret_op_id)
								MakeLotListForEgaisRetReg2ToWh(ep, intr_expnd_op_id, loc_id, ret_bill_lot_list);
						}
						else {
							for(InitIteration(); NextIteration(&item) > 0;) {
								if(item.GoodsID > 0 && ep.IsAlcGoods(item.GoodsID)) {
									if(selection == PPEDIOP_EGAIS_ACTCHARGEON) {
										THROW_MEM(lot_list.add(item.ID));
									}
									else if(oneof2(selection, PPEDIOP_EGAIS_ACTCHARGEONSHOP, (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000))) {
										p_ref->Ot.GetTagStr(PPOBJ_LOT, item.ID, PPTAG_LOT_FSRARINFB, temp_buf);
										if(temp_buf.IsEmpty()) {
											if(p_ref->Ot.GetTagStr(PPOBJ_LOT, item.ID, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0) {
												if(selection == PPEDIOP_EGAIS_ACTCHARGEONSHOP) {
													THROW_MEM(lot_list.add(item.ID));
												}
												/*else if(selection == (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000)) {
													double pre_qtty = 0.0;
													if(preliminary_ret_bill_lot_list.Search(item.ID, &pre_qtty, 0, 0)) {
														ret_bill_lot_list.Add(item.ID, pre_qtty);
														THROW_MEM(lot_list.add(item.ID));
													}
												}*/
											}
										}
									}
								}
							}
						}
						if(selection == (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000)) {
							if(ret_bill_lot_list.getCount()) {
								PPID   sco_op_id = 0;
								PPObjOprKind op_obj;
								if(op_obj.GetEdiShopChargeOnOp(&sco_op_id, 1)) {
									PPBillPacket new_bp;
									THROW(new_bp.CreateBlank2(sco_op_id, _curdate, loc_id, 0));
									for(uint i = 0; i < ret_bill_lot_list.getCount(); i++) {
										const RAssoc & r_assc = ret_bill_lot_list.at(i);
										ReceiptTbl::Rec lot_rec;
										if(P_BObj->trfr->Rcpt.Search(r_assc.Key, &lot_rec) > 0) {
											p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARINFA, ref_a);
											p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARINFB, ref_b);
											p_ref->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
											if(ref_b.NotEmpty() && egais_code.NotEmpty()) {
												double ret_qtty = R6(r_assc.Val);
												PPTransferItem ti;
												uint   new_pos = new_bp.GetTCount();
												THROW(ti.Init(&new_bp.Rec, 1));
												THROW(ti.SetupGoods(lot_rec.GoodsID, 0));
												ti.Quantity_ = -fabs(ret_qtty);
												ti.Cost = lot_rec.Cost;
												ti.Price = lot_rec.Price;
												THROW(new_bp.LoadTItem(&ti, 0, 0));
												{
													ObjTagList tag_list;
													tag_list.PutItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
													tag_list.PutItemStr(PPTAG_LOT_FSRARINFB, ref_b);
													tag_list.PutItemStrNE(PPTAG_LOT_FSRARINFA, ref_a);
													THROW(new_bp.LTagL.Set(new_pos, &tag_list));
												}
											}
										}
									}
									if(new_bp.GetTCount()) {
										new_bp.InitAmounts();
										THROW(P_BObj->TurnPacket(&new_bp, 1));
										PPObjBill::MakeCodeString(&new_bp.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
										PPMessage(mfInfo, PPINF_EGAISSHOPRETURNCR, temp_buf);
									}
								}
							}
						}
						else if(lot_list.getCount()) {
							lot_list.sortAndUndup();
							PPIDArray local_list;
							const LDATE rest_date = NZOR(Filt.Period.upp, _curdate);
							for(uint i = 0; i < lot_list.getCount(); i++) {
								local_list.add(lot_list.get(i));
							}
							{
								PPID   new_bill_id = 0;
								BillTbl::Rec new_bill_rec;
								const int cbr = ep.CreateActChargeOnBill(&new_bill_id, selection, loc_id, rest_date, local_list, 1);
								THROW(cbr);
								if(cbr > 0 && P_BObj->Search(new_bill_id, &new_bill_rec) > 0) {
									PPObjBill::MakeCodeString(&new_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
									PPMessage(mfInfo, PPINF_ACTCHARGEONCR, temp_buf);
								}
								else
									PPMessage(mfInfo, PPINF_ACTCHARGEONDONTCR);
							}
						}
						PPWaitStop();
					}
				}
				break;
		}
		if(ok > 0 && lot_id)
			UpdateTempTable(lot_id);
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewLot::Debug()
{
	LotViewItem item;
	SString buf;
	PPWaitStart();
	PPLogMessage(PPFILNAM_DEBUG_LOG, PPSTR_TEXT, PPTXT_LOG_LOTPRICEROUNDTEST_BEG, LOGMSGF_TIME|LOGMSGF_USER);
	for(InitIteration(); NextIteration(&item) > 0;) {
		buf.Z().CR().Cat(item.Cost, MKSFMTD(0, 12, 0)).Space().CatCharN('-', 2).Space().Cat(item.Price, MKSFMTD(0, 12, 0));
		PPLogMessage(PPFILNAM_DEBUG_LOG, buf, 0);
		for(int i = 2; i <= 6; i++) {
			const double c = round(item.Cost, i);
			const double p = round(item.Price, i);
			buf.Z().Cat(i).CatDiv(':', 2).Cat(c, MKSFMTD(0, 12, 0)).Space().CatCharN('-', 2).Space().Cat(p, MKSFMTD(0, 12, 0));
			PPLogMessage(PPFILNAM_DEBUG_LOG, buf, 0);
		}
		PPWaitPercent(Counter.Increment());
	}
	PPLogMessage(PPFILNAM_DEBUG_LOG, PPSTR_TEXT, PPTXT_LOG_LOTPRICEROUNDTEST_END, LOGMSGF_TIME|LOGMSGF_USER);
	PPWaitStop();
	return -1;
}

int PPViewLot::UpdateTempTable(PPID lotID)
{
	int    ok = -1;
	TempLotTbl::Rec rec;
	SString temp_buf;
	if(P_TempTbl) {
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(SearchByKey_ForUpdate(P_TempTbl, 0, &lotID, &rec) > 0) {
			ReceiptTbl::Rec lot_rec;
			LotViewItem item;
			int    r = -1;
			if(P_Tbl->Search(lotID, &lot_rec) > 0 && (r = AcceptViewItem(lot_rec, &item)) > 0) {
				rec.Clear();
				rec.LotID   = lot_rec.ID;
				rec.Dt      = lot_rec.Dt;
				rec.OrgID   = item.OrgLotID;
				rec.OrgDt   = item.OrgLotDt;
				rec.OprNo   = lot_rec.OprNo;
				rec.GoodsID = lot_rec.GoodsID;
				STRNSCPY(rec.GoodsName, GetGoodsName(lot_rec.GoodsID, temp_buf));
				STRNSCPY(rec.Serial, item.Serial);
				// @v11.1.6
				if((Filt.Flags & LotFilt::fShowBillStatus) && P_BObj) {
					BillTbl::Rec bill_rec;
					if(P_BObj->Search(item.BillID, &bill_rec) > 0 && bill_rec.StatusID) {
						PPObjBillStatus bs_obj;
						PPBillStatus bs_rec;
						if(bs_obj.Fetch(bill_rec.StatusID, &bs_rec) > 0)
							STRNSCPY(rec.BillStatus, bs_rec.Name);
					}
				}
				// } @v11.1.6
				rec.BegRest   = item.BegRest;
				rec.EndRest   = item.EndRest;
				rec.QttyPlus  = item.QttyPlus;
				rec.QttyMinus = item.QttyMinus;
				THROW_DB(P_TempTbl->updateRecBuf(&rec)); // @sfu
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->LotID == lotID));
				ok = 1;
			}
			THROW(r);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPViewLot::InsertTempRecsByIter(BExtInsert * pBei, long * pCounter, UintHashTable * pHt, int showPercentage)
{
	int    ok = 1, r;
	long   nr = DEREFPTRORZ(pCounter);
	PPObjBillStatus bs_obj;
	SString temp_buf;
	LotViewItem item;
	PPBillStatus bs_rec;
	BillTbl::Rec brec;
	if(InitIteration(PPViewLot::OrdByDefault) > 0) {
		while(NextIteration(&item) > 0) {
			if(!pHt || !pHt->Has(item.ID)) {
				int    skip = 0;
				TempLotTbl::Rec rec;
				rec.LotID   = item.ID;
				rec.Dt      = item.Dt;
				rec.OrgDt   = item.OrgLotDt;
				rec.OprNo   = item.OprNo;
				rec.GoodsID = item.GoodsID;
				GObj.FetchNameR(item.GoodsID, temp_buf);
				STRNSCPY(rec.GoodsName, temp_buf);
				STRNSCPY(rec.Serial, item.Serial);
				rec.BegRest   = item.BegRest;
				rec.EndRest   = item.EndRest;
				rec.QttyPlus  = item.QttyPlus;
				rec.QttyMinus = item.QttyMinus;
				if((Filt.Flags & LotFilt::fShowBillStatus) && P_BObj && P_BObj->Search(item.BillID, &brec) > 0 && brec.StatusID && bs_obj.Fetch(brec.StatusID, &bs_rec) > 0)
					STRNSCPY(rec.BillStatus, bs_rec.Name);
				if(Filt.Flags & LotFilt::fShowPriceDev) {
					ReceiptTbl::Rec prev_rec;
					THROW(r = P_Tbl->GetPreviousLot(item.GoodsID, item.LocID, item.Dt, item.OprNo, &prev_rec));
					if(r > 0) {
						if(item.Cost > prev_rec.Cost)
							rec.SFlags |= LOTSF_COSTUP;
						else if(item.Cost < prev_rec.Cost)
							rec.SFlags |= LOTSF_COSTDOWN;
						if(item.Price > prev_rec.Price)
							rec.SFlags |= LOTSF_PRICEUP;
						else if(item.Price < prev_rec.Price)
							rec.SFlags |= LOTSF_PRICEDOWN;
					}
					else
						rec.SFlags |= LOTSF_FIRST;
					switch(Filt.CostDevRestr) {
						case LotFilt::drBelow:
							if(!(rec.SFlags & LOTSF_COSTDOWN))
								skip = 1;
							break;
						case LotFilt::drAbove:
							if(!(rec.SFlags & LOTSF_COSTUP))
								skip = 1;
							break;
						case LotFilt::drAny:
							if(!(rec.SFlags & LOTSF_COSTDOWN) && !(rec.SFlags & LOTSF_COSTUP))
								skip = 1;
							break;
					}
					//
					switch(Filt.PriceDevRestr) {
						case LotFilt::drBelow:
							if(!(rec.SFlags & LOTSF_PRICEDOWN))
								skip = 1;
							break;
						case LotFilt::drAbove:
							if(!(rec.SFlags & LOTSF_PRICEUP))
								skip = 1;
							break;
						case LotFilt::drAny:
							if(!(rec.SFlags & LOTSF_PRICEDOWN) && !(rec.SFlags & LOTSF_PRICEUP))
								skip = 1;
							break;
					}
				}
				if(!skip) {
					THROW_DB(pBei->insert(&rec));
					CALLPTRMEMB(pHt, Add(item.ID));
					nr++;
				}
			}
			if(showPercentage)
				PPWaitPercent(Counter);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pCounter, nr);
	return ok;
}

int PPViewLot::CreateTempTable()
{
	ZDELETE(P_TempTbl);
	int    ok = 1;
	TempLotTbl * p_temp_tbl = 0;
	int    done = 0;
	long   nr = 0;
	THROW(p_temp_tbl = CreateTempFile <TempLotTbl> ());
	{
		const long period_threshould = 180;
		BExtInsert bei(p_temp_tbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if((!Filt.GoodsID && Filt.GoodsGrpID) && !SupplList.GetSingle() && Filt.Period.GetLength() > period_threshould) {
			PPIDArray group_goods_list;
			RECORDNUMBER num_goods = 0;
			uint   grp_count = 0;
			long   grp_calc_threshold = CConfig.GRestCalcThreshold;
			if(grp_calc_threshold <= 0 || grp_calc_threshold > 1000)
				grp_calc_threshold = 100;
			//
			GObj.P_Tbl->getNumRecs(&num_goods);
			GoodsFilt goods_flt;
			goods_flt.GrpID = Filt.GoodsGrpID;
			THROW(GoodsIterator::GetListByFilt(&goods_flt, &group_goods_list));
			grp_count = group_goods_list.getCount();
			if(num_goods && (((1000 * grp_count) / num_goods) < (ulong)grp_calc_threshold)) {
				//
				// Если задан одиночный склад и признак закрытого лота, то не следует использовать
				// внутренние PPViewLot, по скольку при этом выборка идет по отдельному индексу и быстрее будет
				// отобрать нужные товары из выборки лотов, чем для каждого товара из группы отбирать нужные лоты.
				//
				if(!(LocList.getSingle() && Filt.ClosedTag)) {
					PPViewLot temp_view;
					LotFilt temp_filt = Filt;
					temp_filt.Flags |= LotFilt::fNoTempTable;
					UintHashTable ht;
					for(uint i = 0; i < grp_count; i++) {
						temp_filt.GoodsID = group_goods_list.at(i);
						THROW(temp_view.Init_(&temp_filt));
						THROW(temp_view.InsertTempRecsByIter(&bei, &nr, &ht, 0));
						PPWaitPercent(i+1, grp_count);
					}
					done = 1;
				}
			}
		}
		if(!done) {
			uint   sc = SupplList.GetCount();
			if(sc > 1) {
				PPViewLot temp_view;
				LotFilt temp_filt = Filt;
				temp_filt.Flags |= LotFilt::fNoTempTable;
				UintHashTable ht;
				for(uint i = 0; i < sc; i++) {
					temp_filt.SupplID = SupplList.Get().get(i);
					THROW(temp_view.Init_(&temp_filt));
					THROW(temp_view.InsertTempRecsByIter(&bei, &nr, &ht, 0));
					PPWaitPercent(i+1, sc);
				}
				done = 1;
			}
		}
		if(!done) {
			THROW(InsertTempRecsByIter(&bei, &nr, 0, 1));
			done = 1;
		}
		THROW_DB(bei.flash());
		THROW(tra.Commit());
	}
	Counter.Init(nr);
	P_TempTbl = p_temp_tbl;
	p_temp_tbl = 0;
	CATCH
		ZDELETE(p_temp_tbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewLot::InitIteration(IterOrder order)
{
	int    ok = 1;
	bool   no_recs = false;
	DBQ  * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	if(order == OrdByGoodsName && !P_TempTbl) {
		THROW(CreateTempTable());
	}
	if(P_TempTbl) {
		BtrDbKey key_;
		BtrDbKey key__;
		int    idx = 0;
		if(oneof2(order, OrdByDefault, OrdByID))
			idx = 0;
		else if(order == OrdByDate)
			idx = 1;
		else if(order == OrdByGoodsName)
			idx = 2;
		else
			idx = 0;
		P_IterQuery = new BExtQuery(P_TempTbl, idx, 16);
		P_IterQuery->selectAll();
		Counter.Init(P_IterQuery->countIterations(0, key__, spGe));
		P_IterQuery->initIteration(0, key_, spGe);
	}
	else if(Filt.ParentLotID) {
		Itd.IdList.setPointer(0);
		Counter.SetTotal(Itd.IdList.getCount());
	}
	else {
		int    idx = 0;
		union {
			ReceiptTbl::Key1 k1;
			ReceiptTbl::Key2 k2;
			ReceiptTbl::Key3 k3;
			ReceiptTbl::Key5 k5;
			ReceiptTbl::Key6 k6;
			ReceiptTbl::Key7 k7;
		} k, k_;
		PPID   abs_goods_id = labs(Filt.GoodsID);
		PPID   q_goods_id = (Filt.Flags & LotFilt::fOrders) ? -abs_goods_id : abs_goods_id;
		const  PPID   single_loc_id = LocList.getSingle();
		const  PPID   single_suppl_id = SupplList.GetSingle();
		LDATE  expr_beg;
		MEMSZERO(k);
		if(Filt.Flags & LotFilt::fCheckOriginLotDate) {
			DateRange period;
			dbq = & (*dbq && daterange(P_Tbl->Dt, &period.Set(Filt.Period.low, ZERODATE)));
		}
		else
			dbq = & (*dbq && daterange(P_Tbl->Dt, &Filt.Period));
		dbq = ppcheckfiltid(dbq, P_Tbl->GoodsID, q_goods_id);
		dbq = ppcheckfiltid(dbq, P_Tbl->LocID,   single_loc_id);
		dbq = ppcheckfiltid(dbq, P_Tbl->SupplID, single_suppl_id);
		if(Filt.QCertID || (Filt.Flags & LotFilt::fWithoutQCert))
			dbq = &(*dbq && P_Tbl->QCertID == Filt.QCertID);
		if(oneof2(Filt.ClosedTag, 1, 2))
			dbq = &(*dbq && P_Tbl->Closed == ((Filt.ClosedTag == 1) ? 0L : 1L));
		dbq = &(*dbq && realrange(P_Tbl->Cost, Filt.CostRange.low, Filt.CostRange.upp) && realrange(P_Tbl->Price, Filt.PriceRange.low, Filt.PriceRange.upp));
		if(Filt.Flags & LotFilt::fWithoutExpiry) {
			//
			// @v4.6.11
			// Почему-то конструкция P_Tbl->Expiry < encodedate(1,1,1900) работает
			// надежнее, чем P_Tbl->Expiry > 0L. Надо бы разобраться.
			//
			dbq = & (*dbq && P_Tbl->Expiry < encodedate(1,1,1900));
		}
		else {
			if(Filt.ExpiryPrd.upp && !Filt.ExpiryPrd.low)
				encodedate(1, 1, 1900, &expr_beg);
			else
				expr_beg = Filt.ExpiryPrd.low;
			dbq = & (*dbq && daterange(P_Tbl->Expiry, expr_beg, Filt.ExpiryPrd.upp));
		}
		if(Filt.Flags & LotFilt::fOrders)
			dbq = & (*dbq && P_Tbl->GoodsID < 0L && P_Tbl->BillID > 0L);
		else
			dbq = & (*dbq && P_Tbl->GoodsID > 0L && P_Tbl->BillID > 0L);
		dbq = ppcheckfiltid(dbq, P_Tbl->InTaxGrpID, Filt.InTaxGrpID);
		if(Filt.Operation.low && Filt.Flags & LotFilt::fSkipClosedBeforeOp)
			dbq = &(*dbq && P_Tbl->CloseDate >= Filt.Operation.low);
		if(single_loc_id && Filt.ClosedTag) {
			if(Filt.GoodsID) {
				idx  = 3;
				k.k3.Closed  = (Filt.ClosedTag == 1) ? 0 : 1;
				k.k3.GoodsID = q_goods_id;
				k.k3.LocID   = single_loc_id;
				k.k3.Dt      = Filt.Period.low;
				k_ = k;
				if(P_Tbl->search(idx, &k_, spGe)) {
					if(k_.k3.Closed != k.k3.Closed || k_.k3.GoodsID != k.k3.GoodsID || k_.k3.LocID != k.k3.LocID)
						no_recs = true;
					else if(Filt.Period.upp && k_.k3.Dt > Filt.Period.upp)
						no_recs = true;
				}
				else {
					THROW_DB(BTROKORNFOUND);
					no_recs = true;
				}
			}
			else {
				idx  = 7;
				k.k7.LocID   = single_loc_id;
				k.k7.Closed  = (Filt.ClosedTag == 1) ? 0 : 1;
				k.k7.Dt      = Filt.Period.low;
			}
		}
		else if(Filt.GoodsID) {
			idx        = 2;
			k.k2.GoodsID = q_goods_id;
			k.k2.Dt    = Filt.Period.low;
		}
		else if(single_suppl_id) {
			idx        = 5;
			k.k5.SupplID = single_suppl_id;
			k.k5.Dt    = Filt.Period.low;
		}
		else if(Filt.QCertID || (Filt.Flags & LotFilt::fWithoutQCert)) {
			idx  = 6;
			k.k6.QCertID = Filt.QCertID;
			k.k6.Dt      = Filt.Period.low;
		}
		else {
			idx        = 1;
			k.k1.Dt    = Filt.Period.low;
		}
		THROW_MEM(P_IterQuery = new BExtQuery(P_Tbl, idx));
		P_IterQuery->selectAll().where(*dbq);
		k_ = k;
		if(no_recs)
			ok = -1;
		else
			Counter.Init(P_IterQuery->countIterations(0, &k_, spGe));
		P_IterQuery->initIteration(false, &k, spGe);
	}
	CATCH
		if(P_IterQuery == 0)
			delete dbq;
		else
			BExtQuery::ZDelete(&P_IterQuery);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewLot::CheckForFilt(const ReceiptTbl::Rec & rRec)
{
	int    ok = 1;
	SString temp_buf;
	THROW(!Filt.GoodsID || labs(rRec.GoodsID) == labs(Filt.GoodsID));
	THROW(!(Filt.Flags & LotFilt::fOrders && Filt.ClosedTag == 1) || !(rRec.Flags & LOTF_CLOSEDORDER));
	THROW(!(Filt.Flags & LotFilt::fLotfPrWoTaxes) || (rRec.Flags & LOTF_PRICEWOTAXES));
	THROW(!(State & stFiltSerial) || Itd.IdBySerialList.lsearch(rRec.ID));
	THROW(!(Filt.Flags & LotFilt::fCostAbovePrice) || dbl_cmp(rRec.Cost, rRec.Price) > 0);
	THROW(!SupplList.GetCount() || SupplList.CheckID(rRec.SupplID));
	THROW(!LocList.getCount() || LocList.lsearch(rRec.LocID));
	if(Filt.Flags & LotFilt::fCancelledOrdersOnly) {
		BillTbl::Rec bill_rec;
		THROW(P_BObj->Fetch(rRec.BillID, &bill_rec) > 0 && (bill_rec.Flags2 & BILLF2_DECLINED));
	}
	if(Filt.AgentID) {
		PPBillExt bill_ext;
		THROW(P_BObj->FetchExt(rRec.BillID, &bill_ext) > 0 && bill_ext.AgentID == Filt.AgentID);	
	}
	if(Filt.SupplPsnCategoryID) {
		THROW(rRec.SupplID);
		{
			const  PPID psn_id = ObjectToPerson(rRec.SupplID, 0);
			PersonTbl::Rec psn_rec;
			THROW(psn_id && PsnObj.Fetch(psn_id, &psn_rec) > 0 && psn_rec.CatID == Filt.SupplPsnCategoryID);
		}
	}
	if(!Filt.QcExpiryPrd.IsZero() && rRec.QCertID) {
		QualityCertTbl::Rec qc_rec;
		if(QcObj.Search(rRec.QCertID, &qc_rec) > 0) {
			THROW(Filt.QcExpiryPrd.CheckDate(qc_rec.Expiry));
		}
	}
	if(Filt.Flags & LotFilt::fWithoutClb) {
		const int ret_value = P_BObj->GetClbNumberByLot(rRec.ID, 0, temp_buf);
		THROW(ret_value < 0);
		{
			PPID   country_id = 0;
			PPCountryBlock cb;
			int    r = GObj.GetManufCountry(rRec.GoodsID, 0, &country_id, &cb);
			if(cb.IsNative || (r == 1 && Itd.NativeCntryList.lsearch(country_id)) || (r == 2 && Itd.PsnNativeCntryList.lsearch(country_id))) {
				// Если у лота нет ГТД, но товар произведен в родной стране (NativeCountry), то ГТД и не нужен, потому считаем, что такой лот 
				// не должен попадать в выборку 'лоты без ГТД'
				CALLEXCEPT();
			}
		}
	}
	THROW(PPObjTag::CheckForTagFilt(PPOBJ_LOT, rRec.ID, Filt.P_TagF) > 0);
	THROW(!(Filt.Flags & LotFilt::fDeadLots) || P_BObj->trfr->IsDeadLot(rRec.ID) > 0);
	if(Filt.Flags & LotFilt::fOnlySpoilage) {
		//
		// На нужны только те лоты, которые имеют серийный номер и этот серийный номер перечислен в списке негодных.
		// Кроме того, если найденная запись списка 'негодных' серий имеет ссылку на товар, а наш лот относится к другому товару, то уходим.
		// @todo Пока перестраивал и писал комментарии заметил, что тут ошибка: надо извлечь из P_SpoilTbl все записи с заданной серией 
		// и если среди них есть хоть одна с пустым ид товара или с ид товара равным rRec.GoodsID то считать, что наша серия 'негодная'
		//
		P_BObj->GetSerialNumberByLot(rRec.ID, temp_buf, 1);
		THROW(temp_buf.NotEmpty());
		{
			SpecSeries2Tbl::Rec ss_rec;
			if(P_SpoilTbl->SearchBySerial(SPCSERIK_SPOILAGE, temp_buf, &ss_rec) > 0 && (!ss_rec.GoodsID || labs(ss_rec.GoodsID) == labs(rRec.GoodsID))) {
				;
			}
			else {
				CALLEXCEPT();
			}
		}
	}
	if(Filt.GoodsGrpID) {
		THROW(GObj.BelongToGroup(rRec.GoodsID, Filt.GoodsGrpID, 0) > 0);
	}
	CATCHZOK
	return ok;
}

int PPViewLot::AcceptViewItem(const ReceiptTbl::Rec & rLotRec, LotViewItem * pItem)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	LotViewItem item;
	MEMSZERO(item);
	SString temp_buf;
	if(CheckForFilt(rLotRec)) {
		bool   do_skip = false;
		*static_cast<ReceiptTbl::Rec *>(&item) = rLotRec;
		if(Filt.Flags & LotFilt::fShowSerialN) {
			P_BObj->GetSerialNumberByLot(rLotRec.ID, temp_buf, 1);
			temp_buf.CopyTo(item.Serial, sizeof(item.Serial));
		}
		if(!Filt.Operation.IsZero()) {
			const  LDATE low_date = Filt.Operation.low ? plusdate(Filt.Operation.low, -1) : ZERODATE;
			if(Filt.Flags & LotFilt::fRestByPaym) {
				{
					//
					// Расчет неоплаченных поставщикам остатков по лотам
					//
					double part;
					if(low_date && rLotRec.Dt <= low_date) {
						if(!P_PplBlkBeg) {
							DateRange prd;
							prd.Set(ZERODATE, low_date);
							THROW_MEM(P_PplBlkBeg = new PPObjBill::PplBlock(prd, 0, 0));
						}
						int r = P_BObj->GetPayoutPartOfLot(rLotRec.ID, *P_PplBlkBeg, &part);
						item.BegRest = (r == 1) ? ((item.Cost * item.Quantity) * (1.0 - part)) : 0.0;
					}
					if(Filt.Operation.upp && rLotRec.Dt <= Filt.Operation.upp) {
						if(!P_PplBlkEnd) {
							DateRange prd;
							prd.Set(ZERODATE, Filt.Operation.upp);
							THROW_MEM(P_PplBlkEnd = new PPObjBill::PplBlock(prd, 0, 0));
						}
						int r = P_BObj->GetPayoutPartOfLot(rLotRec.ID, *P_PplBlkEnd, &part);
						item.EndRest = (r == 1) ? ((item.Cost * item.Quantity) * (1.0 - part)) : 0.0;
					}
					if(R6(item.BegRest - item.EndRest) == 0.0) {
						do_skip = true;
					}
					else {
						const double diff = item.EndRest - item.BegRest;
						item.QttyPlus  = (diff > 0.0) ? diff : 0.0;
						item.QttyMinus = (diff < 0.0) ? -diff : 0.0;
					}
				}
			}
			else {
				int    is_empty = 1;
				DateIter di(&Filt.Operation);
				TransferTbl::Rec trfr_rec;
				while(P_BObj->trfr->EnumByLot(rLotRec.ID, &di, &trfr_rec) > 0) {
					if(trfr_rec.Flags & PPTFR_PLUS)
						item.QttyPlus  += fabs(trfr_rec.Quantity);
					else if(trfr_rec.Flags & PPTFR_MINUS)
						item.QttyMinus += fabs(trfr_rec.Quantity);
					is_empty = 0;
				}
				if(is_empty && Filt.Flags & LotFilt::fSkipNoOp) {
					do_skip = true;
				}
				else {
					LDATE  tmpdt = low_date;
					if(tmpdt >= rLotRec.Dt)
						P_BObj->trfr->GetRest(rLotRec.ID, tmpdt, &item.BegRest);
					tmpdt = Filt.Operation.upp;
					if(tmpdt == 0)
						item.EndRest = rLotRec.Rest;
					else if(tmpdt < rLotRec.CloseDate)
						P_BObj->trfr->GetRest(rLotRec.ID, tmpdt, &item.EndRest);
				}
			}
		}
		if(Filt.Flags & (LotFilt::fInitOrgLot|LotFilt::fCheckOriginLotDate)) {
			ReceiptTbl::Rec org_rec;
			if(item.PrevLotID)
				P_Tbl->SearchOrigin(item.ID, 0, 0, &org_rec);
			else {
				org_rec.ID = item.ID;
				org_rec.Dt = item.Dt;
			}
			item.OrgLotID = org_rec.ID;
			item.OrgLotDt = org_rec.Dt;
			if(Filt.Flags & LotFilt::fCheckOriginLotDate) {
				if(!org_rec.ID || !Filt.Period.CheckDate(org_rec.Dt))
					do_skip = true;
				else
					item.OrgLotDt = org_rec.Dt;
			}
		}
		if(!do_skip)
			ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pItem, item);
	return ok;
}

int FASTCALL PPViewLot::NextIteration(LotViewItem * pItem)
{
	LotViewItem item;
	if(P_IterQuery) {
		while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			MEMSZERO(item);
			Counter.Increment();
			if(P_TempTbl) {
				TempLotTbl::Rec & r_temp_rec = P_TempTbl->data;
				PPID   lot_id = r_temp_rec.LotID;
				if(P_Tbl->Search(lot_id) > 0) {
					*static_cast<ReceiptTbl::Rec *>(&item) = P_Tbl->data;
					STRNSCPY(item.Serial, r_temp_rec.Serial);
					item.BegRest   = r_temp_rec.BegRest;
					item.EndRest   = r_temp_rec.EndRest;
					item.QttyPlus  = r_temp_rec.QttyPlus;
					item.QttyMinus = r_temp_rec.QttyMinus;
					if(!(State & stAccsCost))
						item.Cost = 0.0;
					ASSIGN_PTR(pItem, item);
					return 1;
				}
			}
			else {
				ReceiptTbl::Rec lot_rec;
				P_Tbl->copyBufTo(&lot_rec);
				int    r = AcceptViewItem(lot_rec, pItem);
				THROW(r);
				if(r > 0)
					return 1;
				else
					continue;
			}
		}
	}
	else if(Filt.ParentLotID) {
		if(Itd.IdList.testPointer()) {
			MEMSZERO(item);
			Counter.Increment();
			const PPID lot_id = Itd.IdList.get(Itd.IdList.incPointer());
			if(P_Tbl->Search(lot_id) > 0) {
				*static_cast<ReceiptTbl::Rec *>(&item) = P_Tbl->data;
				if(!(State & stAccsCost))
					item.Cost = 0.0;
			}
			else {
				item.ID = lot_id;
			}
			ASSIGN_PTR(pItem, item);
			return 1;
		}
	}
	CATCH
		return 0;
	ENDCATCH
	return -1;
}
//
//
//
/*static*/int PPViewLot::CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw && pData && pStyle) {
		BrowserDef * p_def = p_brw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			PPViewLot * p_view = static_cast<PPViewLot *>(p_brw->P_View);
			const LotFilt * p_filt = static_cast<const LotFilt *>(p_view->GetBaseFilt());
			const PPViewLot::BrwHdr * p_hdr = static_cast<const PPViewLot::BrwHdr *>(pData);
			const BroColumn & r_col = p_def->at(col);
			if(r_col.OrgOffs == 0) { // ID
				const TagFilt & r_tag_filt = p_view->P_BObj->GetConfig().LotTagIndFilt;
				if(!r_tag_filt.IsEmpty()) {
					SColor clr;
					if(r_tag_filt.SelectIndicator(p_hdr->ID, clr))
						ok = pStyle->SetLeftBottomCornerColor(static_cast<COLORREF>(clr));
				}
			}
			else if(r_col.OrgOffs == 10) { // Expiry
				const PPBillConfig & r_bcfg = p_view->P_BObj->GetConfig();
				if(r_bcfg.WarnLotExpirFlags & r_bcfg.wlefIndicator) {
					TYPEID typ = 0;
					union {
						LDATE  Expiry;
						uint8  Pad[512];
					} dest_data;
					if(p_def->GetCellData(pData, col, &typ, &dest_data, sizeof(dest_data))) {
						if(checkdate(dest_data.Expiry) && diffdate(getcurdate_(), dest_data.Expiry) >= r_bcfg.WarnLotExpirDays) {
							pStyle->Color = GetColorRef(SClrOrange);
							ok = 1;
						}
					}
				}
			}
			else {
				const long qtty_col  = 4;
				const long cost_col  = 8;
				const long price_col = 9;
				if(p_hdr->SFlags && oneof3(col, qtty_col, cost_col, price_col)) {
					if(col == qtty_col && p_hdr->SFlags & LOTSF_FIRST)
						ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrBlue));
					else if(col == cost_col) {
						if(p_hdr->SFlags & LOTSF_COSTUP)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrGreen));
						else if(p_hdr->SFlags & LOTSF_COSTDOWN)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
					}
					else if(col == price_col) {
						if(p_hdr->SFlags & LOTSF_PRICEUP)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrGreen));
						else if(p_hdr->SFlags & LOTSF_PRICEDOWN)
							ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
					}
				}
			}
		}
	}
	return ok;
}

void PPViewLot::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & LotFilt::fOrders) {
			SString word;
			pBrw->LoadToolbarResource(TOOLBAR_ORDLOTS);
			pBrw->SetColumnTitle(6, PPLoadStringS("ordered", word)); // @v11.4.4 @fix 3-->6
			pBrw->SetColumnTitle(5, PPLoadStringS("orderer", word)); // @v11.4.4 @fix 4-->5
			/* @v12.1.6 (see below) if(Filt.Flags & LotFilt::fShowBillStatus) {
				pBrw->InsColumn(-1, "@status", 16, 0, MKSFMT(10, 0), BCO_CAPLEFT); // @v11.1.6 #15-->#16
			}
			if(Filt.Flags & LotFilt::fShowAgent) {
				pBrw->InsColumn(-1, "@agent", 17, 0, MKSFMT(48, 0), BCO_CAPLEFT); // @v12.1.6
			}*/
		}
		{
			DBQBrowserDef * p_def = static_cast<DBQBrowserDef *>(pBrw->getDef());
			const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
			if(p_q) {
				if(Filt.Flags & LotFilt::fShowSerialN) {
					uint fld_no = 15;
					pBrw->InsColumn(-1, "@serial", fld_no, 0, MKSFMT(32, ALIGN_LEFT), BCO_CAPLEFT);
				}
				// @v12.1.6 {
				if(Filt.Flags & LotFilt::fShowBillStatus) {
					pBrw->InsColumn(-1, "@status", 16, 0, MKSFMT(10, 0), BCO_CAPLEFT);
				}
				if(Filt.Flags & LotFilt::fShowAgent) {
					pBrw->InsColumn(-1, "@agent", 17, 0, MKSFMT(48, 0), BCO_CAPLEFT);
				}
				// } @v12.1.6 
				if(Filt.ExtViewAttr == LotFilt::exvaEgaisTags) {
					uint fld_no = 18;
					pBrw->InsColumn(-1, "@rtag_fsrarinfalotcode",  fld_no++, 0, MKSFMT(32, ALIGN_LEFT), BCO_CAPLEFT);
					pBrw->InsColumn(-1, "@rtag_fsrarinfblotcode",  fld_no++, 0, MKSFMT(32, ALIGN_LEFT), BCO_CAPLEFT);
					pBrw->InsColumn(-1, "@rtag_fsrarlotgoodscode", fld_no++, 0, MKSFMT(32, ALIGN_LEFT), BCO_CAPLEFT);
				}
				else if(Filt.ExtViewAttr == LotFilt::exvaVetisTags) {
					uint fld_no = 18;
					pBrw->InsColumn(-1, "@rtag_lotvetisuuid", fld_no++, 0, MKSFMT(40, ALIGN_LEFT), BCO_CAPLEFT);
				}
			}
		}
		pBrw->SetTempGoodsGrp(Filt.GoodsGrpID);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

DBQuery * PPViewLot::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBDataCell fld_list[20];
	int    c = 0;
	LDATE  expr_beg;
	TempLotTbl * tt  = 0;
	ReceiptTbl * rcp = 0;
	DBE    dbe_ar;
	DBE    dbe_loc;
	DBE    dbe_goods;
	DBE    dbe_closedate;
	DBE    dbe_serial;
	DBE    dbe_egais_ref_a;
	DBE    dbe_egais_ref_b;
	DBE    dbe_egais_prodcode;
	DBE    dbe_vetis_vdocuuid;
	DBE    dbe_bill_code; // @v11.1.6
	DBE    dbe_chkpsncat; // @v11.4.4
	DBE    dbe_agentname; // @v12.1.6
	DBE    dbe_empty; // @v12.1.6
	DBE    dbe_billstatus; // @v12.1.6
	DBQ  * dbq = 0;
	DBQuery * q = 0;
	{
		dbe_empty.init();
		dbe_empty.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	if(!P_TempTbl && IsTempTblNeeded())
		THROW(CreateTempTable());
	THROW(CheckTblPtr(rcp = new ReceiptTbl));
	if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1) || (Filt.Flags & LotFilt::fOrders))
		PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, rcp->SupplID);
	else {
		dbe_ar.init();
		dbe_ar.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc, rcp->LocID);
	PPDbqFuncPool::InitObjNameFunc(dbe_bill_code, PPDbqFuncPool::IdObjCodeBill, rcp->BillID); // @v11.1.6
	{
		dbe_closedate.init();
		dbe_closedate.push(rcp->CloseDate);
		dbe_closedate.push(static_cast<DBFunc>(PPDbqFuncPool::IdLotCloseDate));
	}
	// @v12.1.6 {
	{
		dbe_billstatus.init();
		if(Filt.Flags & LotFilt::fShowBillStatus) {
			dbe_billstatus.push(rcp->BillID);
			dbe_billstatus.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillStatusName));
		}
		else
			dbe_billstatus.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	{
		dbe_agentname.init();
		if(Filt.Flags & LotFilt::fShowAgent) {
			dbe_agentname.push(rcp->BillID);
			dbe_agentname.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillAgentName));
		}
		else
			dbe_agentname.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	if(Filt.Flags & LotFilt::fShowSerialN) {
		PPDbqFuncPool::InitObjTagTextFunc(dbe_serial, PPTAG_LOT_SN, rcp->ID);
	}
	else {
		dbe_serial.init();
		dbe_serial.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	// } @v12.1.6 
	if(P_TempTbl) {
		THROW(CheckTblPtr(tt = new TempLotTbl(P_TempTbl->GetName())));
		dbq = &(rcp->ID == tt->LotID);

		fld_list[c++].F = rcp->ID;        // #00
		fld_list[c++].F = (Filt.Flags & LotFilt::fCheckOriginLotDate) ? tt->OrgDt : rcp->Dt; // #01
		fld_list[c++].F = tt->SFlags;     // #02
		fld_list[c++].E = dbe_loc;        // #03
		fld_list[c++].F = tt->GoodsName;  // #04
		fld_list[c++].E = dbe_ar;         // #05
		fld_list[c++].F = rcp->Quantity;  // #06
		fld_list[c++].F = rcp->Rest;      // #07
		if(State & stAccsCost)
			fld_list[c++].F = rcp->Cost;  // #08
		else
			fld_list[c++].C.init(0.0);    // #08
		fld_list[c++].F = rcp->Price;     // #09
		fld_list[c++].F = rcp->Expiry;    // #10
		fld_list[c++].E = dbe_closedate;  // #11
		fld_list[c++].E = dbe_bill_code;  // #12 // @v11.1.6
		fld_list[c++].F = tt->BegRest;    // #13 // @v11.1.6 #+1
		fld_list[c++].F = tt->EndRest;    // #14 // @v11.1.6 #+1
		fld_list[c++].E = dbe_serial;     // #15 // @v11.1.6 #+1
		// @v12.1.6 fld_list[c++].F = tt->BillStatus; // #16 // @v11.1.6 #+1
		fld_list[c++].E = dbe_billstatus;             // #16 // @v11.1.6 #+1 // @v12.1.6
		fld_list[c++].E = dbe_agentname;              // #17 // @v12.1.6
		if(Filt.ExtViewAttr == LotFilt::exvaEgaisTags) {
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_ref_a, PPTAG_LOT_FSRARINFA, tt->LotID); // #18 // @v11.1.6 #+1 // @v12.1.6 #+1
			fld_list[c++].E = dbe_egais_ref_a;
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_ref_b, PPTAG_LOT_FSRARINFB, tt->LotID); // #19 // @v11.1.6 #+1 // @v12.1.6 #+1
			fld_list[c++].E = dbe_egais_ref_b;
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_prodcode, PPTAG_LOT_FSRARLOTGOODSCODE, tt->LotID); // #20 // @v11.1.6 #+1 // @v12.1.6 #+1
			fld_list[c++].E = dbe_egais_prodcode;
			//DBE    dbe_egais_manuf;
			//DBE    dbe_egais_prodtypecode;
		}
		else if(Filt.ExtViewAttr == LotFilt::exvaVetisTags) {
			PPDbqFuncPool::InitObjTagTextFunc(dbe_vetis_vdocuuid, PPTAG_LOT_VETIS_UUID, rcp->ID); // #18 // @v11.1.6 #+1 // @v12.1.6 #+1
			fld_list[c++].E = dbe_vetis_vdocuuid;
		}
		q = &selectbycell(c, fld_list);
		q->from(tt, rcp, 0L).where(*dbq).orderBy(tt->Dt, tt->OprNo, 0L);
	}
	else {
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, rcp->GoodsID);
		fld_list[c++].F = rcp->ID;        // #00
		fld_list[c++].F = rcp->Dt;        // #01
		fld_list[c++].C.init(0L);         // #02
		fld_list[c++].E = dbe_loc;        // #03
		fld_list[c++].E = dbe_goods;      // #04
		if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1) || (Filt.Flags & LotFilt::fOrders))
			fld_list[c++].E = dbe_ar;     // #05
		else
			fld_list[c++].C.init(static_cast<const char *>(0)); // #05 DBConst
		fld_list[c++].F = rcp->Quantity;  // #06
		fld_list[c++].F = rcp->Rest;      // #07
		if(State & stAccsCost)
			fld_list[c++].F = rcp->Cost;  // #08
		else
			fld_list[c++].C.init(0.0);    // #08
		fld_list[c++].F = rcp->Price;     // #09
		fld_list[c++].F = rcp->Expiry;    // #10
		fld_list[c++].E = dbe_closedate;  // #11
		fld_list[c++].E = dbe_bill_code;  // #12 // @v11.1.6 
		//fld_list[c++].E = dbe_empty;      // #13 // @v12.1.6 Для выравнивания нумерации полей между вариантами с P_TempTbl и без оной
		//fld_list[c++].E = dbe_empty;      // #14 // @v12.1.6 Для выравнивания нумерации полей между вариантами с P_TempTbl и без оной
		fld_list[c++].C.init(0.0);        // #13 // @v12.1.6 Для выравнивания нумерации полей между вариантами с P_TempTbl и без оной
		fld_list[c++].C.init(0.0);        // #14 // @v12.1.6 Для выравнивания нумерации полей между вариантами с P_TempTbl и без оной
		fld_list[c++].E = dbe_serial;     // #15 // @v11.1.6 #+1 // @v12.1.6 #+2
		fld_list[c++].E = dbe_billstatus; // #16 @v12.1.6
		fld_list[c++].E = dbe_agentname;  // #17 // @v12.1.6
		if(Filt.ExtViewAttr == LotFilt::exvaEgaisTags) {
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_ref_a, PPTAG_LOT_FSRARINFA, rcp->ID); // #18 // @v11.1.6 #+1 // @v12.1.6 #+4
			fld_list[c++].E = dbe_egais_ref_a;
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_ref_b, PPTAG_LOT_FSRARINFB, rcp->ID); // #19 // @v11.1.6 #+1 // @v12.1.6 #+4
			fld_list[c++].E = dbe_egais_ref_b;
			PPDbqFuncPool::InitObjTagTextFunc(dbe_egais_prodcode, PPTAG_LOT_FSRARLOTGOODSCODE, rcp->ID); // #20 // @v11.1.6 #+1 // @v12.1.6 #+4
			fld_list[c++].E = dbe_egais_prodcode;
			//DBE    dbe_egais_manuf;
			//DBE    dbe_egais_prodtypecode;
		}
		else if(Filt.ExtViewAttr == LotFilt::exvaVetisTags) {
			PPDbqFuncPool::InitObjTagTextFunc(dbe_vetis_vdocuuid, PPTAG_LOT_VETIS_UUID, rcp->ID, 0/*dontUseCache*/); // #18 // @v11.1.6 #+1 // @v12.1.6 #+4
			fld_list[c++].E = dbe_vetis_vdocuuid;
		}
		if(Filt.QCertID || (Filt.Flags & LotFilt::fWithoutQCert))
			dbq = &(rcp->QCertID == Filt.QCertID);
		dbq = & (*dbq && daterange(rcp->Dt, &Filt.Period));
		if(oneof2(Filt.ClosedTag, 1, 2))
			dbq = &(*dbq && rcp->Closed == ((Filt.ClosedTag == 1) ? 0L : 1L));
		if(LocList.getCount())
			dbq = & (*dbq && ppidlist(rcp->LocID, &LocList));
		dbq = ppcheckfiltid(dbq, rcp->SupplID, SupplList.GetSingle());
		dbq = & (*dbq && realrange(rcp->Cost, Filt.CostRange.low, Filt.CostRange.upp) && realrange(rcp->Price, Filt.PriceRange.low, Filt.PriceRange.upp));
		if(Filt.Flags & LotFilt::fCostAbovePrice)
			dbq = & (*dbq && rcp->Cost > rcp->Price);
		if(Filt.Flags & LotFilt::fCancelledOrdersOnly) { // @v12.1.6
			DBE dbe_iscancelled_bill;
			dbe_iscancelled_bill.init();
			dbe_iscancelled_bill.push(rcp->BillID);
			dbe_iscancelled_bill.push(dbconst(BILLF2_DECLINED));
			dbe_iscancelled_bill.push(static_cast<DBFunc>(PPDbqFuncPool::IdCheckBillFlag2));
			dbq = & (*dbq && dbe_iscancelled_bill == 1L);
		}
		if(Filt.Flags & LotFilt::fWithoutExpiry)
			dbq = & (*dbq && rcp->Expiry == 0L);
		else {
			if(Filt.ExpiryPrd.upp && !Filt.ExpiryPrd.low)
				encodedate(1, 1, 1900, &expr_beg);
			else
				expr_beg = Filt.ExpiryPrd.low;
			dbq = & (*dbq && daterange(rcp->Expiry, expr_beg, Filt.ExpiryPrd.upp));
		}
		if(Filt.Flags & LotFilt::fOrders) {
			if(Filt.GoodsID)
				dbq = &(*dbq && rcp->GoodsID == -labs(Filt.GoodsID));
			else
				dbq = &(*dbq && rcp->GoodsID < 0L);
		}
		else {
			if(Filt.GoodsID)
				dbq = &(*dbq && rcp->GoodsID == labs(Filt.GoodsID));
			else
				dbq = &(*dbq && rcp->GoodsID > 0L);
		}
		dbq = ppcheckfiltid(dbq, rcp->InTaxGrpID, Filt.InTaxGrpID);
		// @v11.4.4 {
		if(Filt.SupplPsnCategoryID) {
			dbe_chkpsncat.init();
			dbe_chkpsncat.push(rcp->SupplID);
			DBConst dbc_long;
			dbc_long.init(Filt.SupplPsnCategoryID);
			dbe_chkpsncat.push(dbc_long);
			dbe_chkpsncat.push(static_cast<DBFunc>(PPDbqFuncPool::IdArIsCatPerson));
			dbq = & (*dbq && dbe_chkpsncat == 1L);
		}
		// } @v11.4.4 
		q = &selectbycell(c, fld_list);
		q->from(rcp, 0L).where(*dbq);
		if(LocList.getSingle() && Filt.GoodsID && Filt.ClosedTag)
			q->orderBy(rcp->Closed, rcp->GoodsID, rcp->LocID, rcp->Dt, rcp->OprNo, 0L);
		else if(LocList.getSingle() && Filt.ClosedTag)
			q->orderBy(rcp->LocID, rcp->Closed, rcp->Dt, rcp->OprNo, 0L);
		else if(Filt.QCertID)
			q->orderBy(rcp->QCertID, rcp->Dt, rcp->OprNo, 0L);
		else if(Filt.GoodsID)
			q->orderBy(rcp->GoodsID, rcp->Dt, rcp->OprNo, 0L);
		else if(SupplList.GetSingle())
			q->orderBy(rcp->SupplID, rcp->Dt, rcp->OprNo, 0L);
		else if(Filt.Flags & LotFilt::fWithoutQCert)
			q->orderBy(rcp->QCertID, rcp->Dt, rcp->OprNo, 0L);
		else
			q->orderBy(rcp->Dt, rcp->OprNo, 0L);
	}
	THROW(CheckQueryPtr(q));
	{
		SString sub_title, temp_buf;
		const bool ord = LOGIC(Filt.Flags & LotFilt::fOrders);
		if(ord) {
			PPLoadString("orders", temp_buf);
			sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
		}
		if(Filt.ParentLotID) {
			SString code;
			ReceiptTbl::Rec lot_rec;
			if(P_Tbl->Search(Filt.ParentLotID, &lot_rec) > 0)
				ReceiptCore::MakeCodeString(&lot_rec, 0, code);
			else
				ideqvalstr(Filt.ParentLotID, code);
			sub_title.Printf(PPLoadTextS(PPTXT_LOTSTITLE_BYPARENT, temp_buf), code.cptr());
		}
		else {
			if(LocList.getSingle()) {
				GetLocationName(LocList.getSingle(), temp_buf);
				sub_title.CatDivIfNotEmpty('-', 1).Cat(temp_buf);
			}
			if(Filt.GoodsID) {
				sub_title.CatDivIfNotEmpty('-', 1).Cat(GetGoodsName(Filt.GoodsID, temp_buf));
			}
			{
				const uint sc = SupplList.GetCount();
				if(sc > 0) {
					SString ar_buf;
					for(uint i = 0; i < sc; i++) {
						GetArticleName(SupplList.Get().get(i), temp_buf);
						ar_buf.CatDivIfNotEmpty(';', 2).Cat(temp_buf);
						if(ar_buf.Len() > 64 && i < sc-1) {
							ar_buf.CatCharN('.', 2);
							break;
						}
					}
					sub_title.CatDivIfNotEmpty('-', 1).Cat(ar_buf);
				}
			}
			if(Filt.QCertID) {
				sub_title.CatDivIfNotEmpty('-', 1);
				CatObjectName(PPOBJ_QCERT, Filt.QCertID, sub_title);
			}
		}
		if(sub_title.IsEmpty())
			PPFormatPeriod(&Filt.Period, sub_title);
		if(Filt.ClosedTag == 1)	{
			SString word;
			PPGetWord(PPWORD_ONLYOPEN, 0, word);
			sub_title.Space().Cat(word);
		}
		if(!Filt.Operation.IsZero())
			brw_id = BROWSER_LOTOPER2;
		else
			brw_id = BROWSER_LOT2;
		ASSIGN_PTR(pSubTitle, sub_title);
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete tt;
			delete rcp;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}
//
//
//
int PPViewLot::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	const  PPID lot_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
	return lot_id ? ::ViewOpersByLot(lot_id, 0) : -1;
}

int PPViewLot::Print(const void *)
{
	int    ok = 1;
	PPReportEnv env;
	TDialog * dlg = new TDialog(DLG_PRNLOTS);
	if(CheckDialogPtrErr(&dlg)) {
		int    reply = ExecView(dlg);
		ushort v;
		dlg->getCtrlData(CTL_PRNLOTS_ORDER, &v);
		if(v == 1)
			env.Sort = OrdByGoodsName;
		else
			env.Sort = OrdByDate;
		dlg->getCtrlData(CTL_PRNLOTS_FLAGS, &v);
		SETFLAG(Filt.Flags, 0x0001, v & 1);
		delete dlg;
		if(reply == cmOK) {
			uint   rpt_id = 0;
			const  PPID single_suppl_id = SupplList.GetSingle();
			if(Filt.Flags & LotFilt::fOrders)
				rpt_id = single_suppl_id ? REPORT_ORDERLOTSS : REPORT_ORDERLOTS;
			else if(Filt.Operation.IsZero())
				rpt_id = single_suppl_id ? REPORT_LOTSS : REPORT_LOTS;
			else
				rpt_id = single_suppl_id ? REPORT_LOTSSOPER : REPORT_LOTSOPER;
			ok = PPAlddPrint(rpt_id, PView(this), &env);
			Filt.Flags &= ~0x0001;
		}
		else
			ok = -1;
	}
	return ok;
}

int PPViewLot::Export()
{
	int    ok = 1, r;
	PPLotExporter l_e;
	THROW(r = l_e.Init(0));
	if(r > 0) {
		PPWaitStart();
		LotViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			THROW(l_e.Export(&item));
			PPWaitPercent(GetCounter());
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
//
// PPViewLot::ExportGoodsLabelData
//
int PPViewLot::ExportGoodsLabelData()
{
	int    ok = 1;
	SString path;
	PPID   goods_id = 0;
	PPID   prev_goods_id = 0;
	LotViewItem lv_item;
	PPObjWorld  w_obj;
	DbfTable   * out_tbl = 0;
	DbfTable   * out_tblh = 0;
	DbfRecord  * p_tblh_rec = 0;
	SString main_org_name;
	PPWaitStart();
	PPGetFilePath(PPPATH_OUT, PPFILNAM_GLABELH_DBF, path);
	THROW(out_tblh = CreateDbfTable(DBFS_RETAILGOODSHDR, path, 1));
	THROW_MEM(p_tblh_rec = new DbfRecord(out_tblh));
	p_tblh_rec->put(1, GetMainOrgName(main_org_name));
	if(/*Filt.LocID*/LocList.getSingle()) {
		SString loc_name;
		GetLocationName(/*Filt.LocID*/LocList.getSingle(), loc_name);
		p_tblh_rec->put(2, loc_name);
	}
	out_tblh->appendRec(p_tblh_rec);
	ZDELETE(out_tblh);
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_GLABEL_DBF, path));
	THROW(out_tbl = CreateDbfTable(DBFS_RETAILGOODS, path, 1));
	for(InitIteration(OrdByGoodsName); NextIteration(&lv_item) > 0;) {
		goods_id = labs(lv_item.GoodsID);
		if(goods_id && (!prev_goods_id || goods_id != prev_goods_id)) {
			RetailGoodsInfo rgi;
			if(GObj.GetRetailGoodsInfo(goods_id, lv_item.LocID, &rgi) > 0) {
				DbfRecord dbfr(out_tbl);
				dbfr.put(1, rgi.ID);
				dbfr.put(2, rgi.Name);
				dbfr.put(3, rgi.BarCode);
				dbfr.put(4, rgi.Price);
				dbfr.put(6, rgi.UnitName);
				dbfr.put(7, rgi.Manuf);
				dbfr.put(8, rgi.ManufCountry);
				dbfr.put(9, lv_item.Expiry);
				THROW_PP(out_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
			}
			prev_goods_id = goods_id;
		}
		PPWaitPercent(GetCounter());
	}
	PPWaitStop();
	CATCHZOKPPERR
	delete p_tblh_rec;
	delete out_tbl;
	return ok;
}
//
//
//
//int EditPrcssrUnifyPriceFiltDialog(PrcssrUnifyPriceFilt *);

int PPViewLot::RevalCostByLots()
{
	int    ok = 1;
	PrcssrUnifyPriceFilt param;
	LotViewItem lv_item;
	PrcssrUnifyPrice upb;
	if(param.Setup(1, /*Filt.LocID*/LocList.getSingle(), SupplList.GetSingle()) > 0 && upb.EditParam(&param) > 0) {
		PPBillPacket pack;
		PPWaitStart();
		THROW(pack.CreateBlank(param.OpKindID, 0, 0, 1));
		pack.Rec.Object = SupplList.GetSingle();
		for(InitIteration(OrdByGoodsName); NextIteration(&lv_item) > 0;) {
			if(!lv_item.Closed) {
				PPTransferItem ti;
				THROW(ti.Init(&pack.Rec));
				THROW(ti.SetupGoods(lv_item.GoodsID));
				THROW(ti.SetupLot(lv_item.ID, &lv_item, 0));
				ti.RevalCost = ti.Cost;
				ti.Cost     = param.CalcPrice(ti.Cost, ti.Price);
				ti.Discount = ti.Price;
				ti.Rest_    = lv_item.Rest;
				THROW(pack.InsertRow(&ti, 0));
			}
			PPWaitPercent(GetCounter());
		}
		pack.InitAmounts();
		THROW(P_BObj->FillTurnList(&pack));
		if(!P_BObj->TurnPacket(&pack, 1)) {
			P_BObj->DiagGoodsTurnError(&pack);
			CALLEXCEPT();
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}
//
// Global standalone functions
//
int STDCALL ViewLots(PPID goods, PPID locID, PPID suppl, PPID qcert, int modeless)
{
	LotFilt flt;
	flt.GoodsID = goods;
	flt.LocList.Add(locID);
	flt.SupplID = suppl;
	flt.QCertID = qcert;
	return ::ViewLots(&flt, 0, modeless);
}

int STDCALL ViewLots(const LotFilt * pFilt, int asOrders, int asModeless) { return PPView::Execute(PPVIEW_LOT, pFilt, asModeless, reinterpret_cast<void *>(BIN(asOrders))); }
//
// Implementation of PPALDD_Lot
//
PPALDD_CONSTRUCTOR(Lot)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(Lot) { Destroy(); }

int PPALDD_Lot::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		PPObjBill * p_bobj = BillObj;
		MEMSZERO(H);
		H.ID = rFilt.ID;
		ReceiptTbl::Rec rec;
		if(p_bobj->trfr->Rcpt.Search(rFilt.ID, &rec) > 0) {
			SString temp_buf;
			H.ID = rec.ID;
			H.GoodsID = rec.GoodsID;
			H.LocID   = rec.LocID;
			H.SupplID = p_bobj->CheckRights(BILLOPRT_ACCSSUPPL, 1) ? rec.SupplID : 0;
			H.BillID  = rec.BillID;
			H.QCertID = rec.QCertID;
			H.InTaxGrpID = rec.InTaxGrpID;
			H.Dt      = rec.Dt;
			H.Expiry  = rec.Expiry;
			H.CloseDt = rec.CloseDate;
			H.Closed  = rec.Closed;
			H.Flags   = rec.Flags;
			H.UnitPerPack = rec.UnitPerPack;
			H.Qtty    = rec.Quantity;
			H.Cost    = p_bobj->CheckRights(BILLRT_ACCSCOST) ? rec.Cost : 0.0;
			H.Price   = rec.Price;
			H.Rest    = rec.Rest;
			p_bobj->GetClbNumberByLot(rec.ID, 0, temp_buf);
			temp_buf.CopyTo(H.CLB, sizeof(H.CLB));
			p_bobj->GetSerialNumberByLot(rec.ID, temp_buf, 1);
			temp_buf.CopyTo(H.Serial, sizeof(H.Serial));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_Lot::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_LONG(n) (*static_cast<const long *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_INT     (*static_cast<int *>(rS.GetPtr(pApl->Get(0))))
	_RET_INT = 0;
	if(pF->Name == "?GetOrgLotID") {
		PPID   org_lot_id = 0;
		BillObj->trfr->Rcpt.SearchOrigin(H.ID, &org_lot_id, 0, 0);
		SETIFZ(org_lot_id, H.ID);
		_RET_INT = org_lot_id;
	}
	else if(pF->Name == "?GetTag") {
		_RET_INT = PPObjTag::Helper_GetTag(PPOBJ_LOT, H.ID, _ARG_STR(1));
	}
	else if(pF->Name == "?GetTagByID") {
		_RET_INT = PPObjTag::Helper_GetTagByID(PPOBJ_LOT, H.ID, _ARG_LONG(1));
	}
}
//
// Implementation of PPALDD_Lots
//
PPALDD_CONSTRUCTOR(Lots)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Lots) { Destroy(); }

int PPALDD_Lots::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Lot, rsrv);
	H.FltLocID      = /*p_filt->LocID*/p_filt->LocList.GetSingle();
	H.FltSupplID    = p_filt->SupplID;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltGoodsID    = p_filt->GoodsID;
	H.FltBeg        = p_filt->Period.low;
	H.FltEnd        = p_filt->Period.upp;
	H.FltExpiryBeg  = p_filt->ExpiryPrd.low;
	H.FltExpiryEnd  = p_filt->ExpiryPrd.upp;
	H.OperLow       = p_filt->Operation.low;
	H.OperUpp       = p_filt->Operation.upp;
	H.IsOper        = (H.OperLow || H.OperUpp);
	H.FltFlags      = p_filt->Flags;
	H.fWithoutQCert = BIN(p_filt->Flags & LotFilt::fWithoutQCert);
	H.fOrders       = BIN(p_filt->Flags & LotFilt::fOrders);
	H.fCostAbovePrice = BIN(p_filt->Flags & LotFilt::fCostAbovePrice);
	H.FltClosedTag  = p_filt->ClosedTag;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Lots::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	//INIT_PPVIEW_ALDD_ITER(Lot);
	PPViewLot * p_v = static_cast<PPViewLot *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return BIN(p_v->InitIteration(static_cast<PPViewLot::IterOrder>(SortIdx)));
}

int PPALDD_Lots::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Lot);
	long   qtty_to_str_fmt = (H.FltFlags & 1) ? MKSFMT(0, QTTYF_SIMPLPACK|QTTYF_FRACTION) : QTTYF_FRACTION;
	I.LotID     = item.ID;
	I.BegRest   = item.BegRest;
	I.EndRest   = item.EndRest;
	I.QttyPlus  = item.QttyPlus;
	I.QttyMinus = item.QttyMinus;
	I.Sales = I.BegRest - I.EndRest;
	QttyToStr(item.Quantity, item.UnitPerPack, qtty_to_str_fmt, I.CQtty);
	QttyToStr(item.Rest,     item.UnitPerPack, qtty_to_str_fmt, I.CRest);
	if(H.IsOper) {
		QttyToStr(I.BegRest, item.UnitPerPack, qtty_to_str_fmt, I.CBegRest);
		QttyToStr(I.EndRest, item.UnitPerPack, qtty_to_str_fmt, I.CEndRest);
		QttyToStr(I.Sales,   item.UnitPerPack, qtty_to_str_fmt, I.CSales);
	}
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_Lots::Destroy() { DESTROY_PPVIEW_ALDD(Lot); }
//
// Import/Export
//
IMPLEMENT_IMPEXP_HDL_FACTORY(LOT, PPLotImpExpParam);

PPLotImpExpParam::PPLotImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags), Flags(0), UhttGoodsCodeArID(0)
{
}

/*virtual*/int PPLotImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(Flags)
			param_list.Add(PPLOTPAR_FLAGS, temp_buf.Z().Cat(Flags));
		if(UhttGoodsCodeArID)
			param_list.Add(PPLOTPAR_UHTTGOODSCODEAR, temp_buf.Z().Cat(UhttGoodsCodeArID));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Flags = 0;
		UhttGoodsCodeArID = 0;
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case PPLOTPAR_FLAGS:
					Flags = temp_buf.ToLong();
					break;
				case PPLOTPAR_UHTTGOODSCODEAR:
					UhttGoodsCodeArID = temp_buf.ToLong();
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPLotImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	long   flags = 0;
	SString params, fld_name;
	THROW(PPLoadText(PPTXT_LOTPARAMS, params));
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	if(Flags) {
		PPGetSubStr(params, PPLOTPAR_FLAGS, fld_name);
		pFile->AppendParam(pSect, fld_name, params.Z().Cat(Flags), 1);
	}
	if(UhttGoodsCodeArID) {
		PPGetSubStr(params, PPLOTPAR_UHTTGOODSCODEAR, fld_name);
		pFile->AppendParam(pSect, fld_name, params.Z().Cat(UhttGoodsCodeArID), 1);
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPLotImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	Flags = 0;
	UhttGoodsCodeArID = 0;
	THROW(PPLoadText(PPTXT_LOTPARAMS, params));
	if(PPGetSubStr(params, PPLOTPAR_FLAGS, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Flags = param_val.ToLong();
	}
	if(PPGetSubStr(params, PPLOTPAR_UHTTGOODSCODEAR, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			UhttGoodsCodeArID = param_val.ToLong();
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}

LotImpExpDialog::LotImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPLOT, 0)
{
}

int LotImpExpDialog::setDTS(const PPLotImpExpParam * pData)
{
	int    ok = 1;
	Data = *pData;
	ImpExpParamDialog::setDTS(&Data);
	SetupArCombo(this, CTLSEL_IMPEXPLOT_AR, Data.UhttGoodsCodeArID, 0, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	return ok;
}

int LotImpExpDialog::getDTS(PPLotImpExpParam * pData)
{
	int    ok = 1;
	uint   sel = 0;
	THROW(ImpExpParamDialog::getDTS(&Data));
	Data.UhttGoodsCodeArID = getCtrlLong(CTLSEL_IMPEXPLOT_AR);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERRBYDLG
	return ok;
}

int EditLotImpExpParam(const char * pIniSection)
{
	int    ok = -1;
	LotImpExpDialog * dlg = 0;
	PPLotImpExpParam param;
	SString ini_file_name, sect;
   	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
   	{
   		int    direction = 0;
   		PPIniFile ini_file(ini_file_name, 0, 1, 1);
   		THROW(CheckDialogPtr(&(dlg = new LotImpExpDialog())));
   		THROW(LoadSdRecord(PPREC_LOT, &param.InrRec));
   		direction = param.Direction;
   		if(!isempty(pIniSection))
   			THROW(param.ReadIni(&ini_file, pIniSection, 0));
   		dlg->setDTS(&param);
   		while(ok <= 0 && ExecView(dlg) == cmOK) {
   			if(dlg->getDTS(&param)) {
   				int    is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
   				if(!isempty(pIniSection))
   					if(is_new)
   						ini_file.RemoveSection(pIniSection);
   					else
   						ini_file.ClearSection(pIniSection);
   				PPErrCode = PPERR_DUPOBJNAME;
   				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
   					ok = 1;
   				else
   					PPError();
   			}
   			else
   				PPError();
		}
   	}
	CATCHZOKPPERR
   	delete dlg;
   	return ok;
}

int SelectLotImpExpCfgs(PPLotImpExpParam * pParam, int import)
{
	int    ok = -1;
	int    valid_data = 0;
	uint   p = 0;
	long   id = 0;
	SString ini_file_name;
	StrAssocArray list;
	PPLotImpExpParam param;
	TDialog * p_dlg = 0;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_LOT, &param, &list, import ? 2 : 1));
	id = (list.SearchByTextNc(pParam->Name, &p) > 0) ? (uint)list.Get(p).Id : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		SString sect;
		//В режиме тестирования - начало
		#if SLTEST_RUNNING
			for(int i = 1; i < (int)list.getCount(); i++) {
				list.GetText(i, sect);
				if(strstr(sect, pParam->Name)) {
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					ok = 1;
					return ok;
				}
			}
		#endif
		// конец
		while(!valid_data && ListBoxSelDialog::Run(&list, import ? PPTXT_TITLE_LOTIMPCFG : PPTXT_TITLE_LOTEXPCFG, &id) > 0) {
			SString sect;
			if(id) {
				list.GetText(id, sect);
				pParam->ProcessName(1, sect);
				pParam->ReadIni(&ini_file, sect, 0);
				valid_data = ok = 1;
			}
			else
				PPError(PPERR_INVGOODSIMPEXPCFG);
		}
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

PPLotExporter::PPLotExporter() : Param(0, 0), P_IE(0)
{
}

PPLotExporter::~PPLotExporter()
{
	ZDELETE(P_IE);
}

int PPLotExporter::Init(const PPLotImpExpParam * pParam)
{
	int    ok = 1;
	RVALUEPTR(Param, pParam);
	if(!pParam) {
		THROW(LoadSdRecord(PPREC_LOT, &Param.InrRec));
		ok = SelectLotImpExpCfgs(&Param, 0);
	}
	if(ok > 0) {
		THROW_MEM(P_IE = new PPImpExp(&Param, 0));
		THROW(P_IE->OpenFileForWriting(0, 1));
	}
	CATCHZOK
	return ok;
}

int PPLotExporter::Export(const LotViewItem * pItem)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	Sdr_Lot  sdr_lot;
	THROW_INVARG(pItem && P_IE);
	sdr_lot.ID = pItem->ID;
	sdr_lot.BillID = pItem->BillID;
	sdr_lot.LocID = pItem->LocID;
	sdr_lot.Dt = pItem->Dt;
	sdr_lot.OprNo = pItem->OprNo;
	sdr_lot.Closed = pItem->Closed;
	sdr_lot.GoodsID = pItem->GoodsID;
	sdr_lot.QCertID = pItem->QCertID;
	sdr_lot.UnitPerPack = pItem->UnitPerPack;
	sdr_lot.Quantity = pItem->Quantity;
	sdr_lot.WtQtty = pItem->WtQtty;
	sdr_lot.WtRest = pItem->WtRest;
	sdr_lot.Cost = pItem->Cost;
	sdr_lot.ExtCost = pItem->ExtCost;
	sdr_lot.Price = pItem->Price;
	sdr_lot.Rest = pItem->Rest;
	sdr_lot.PrevLotID = pItem->PrevLotID;
	sdr_lot.SupplID = pItem->SupplID;
	sdr_lot.CloseDate = pItem->CloseDate;
	sdr_lot.Expiry = pItem->Expiry;
	sdr_lot.InTaxGrpID = pItem->InTaxGrpID;
	sdr_lot.BegRest = pItem->BegRest;
	sdr_lot.QttyPlus = pItem->QttyPlus;
	sdr_lot.QttyMinus = pItem->QttyMinus;
	sdr_lot.OrgLotDt = pItem->OrgLotDt;
	{
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(pItem->GoodsID, &goods_rec) > 0) {
			STRNSCPY(sdr_lot.GoodsName, goods_rec.Name);
			if(GObj.Fetch(goods_rec.ParentID, &goods_rec) > 0) {
				STRNSCPY(sdr_lot.GoodsGroup, goods_rec.Name);
			}
			if(goods_rec.UnitID) {
				PPUnit unit_rec;
				if(GObj.FetchUnit(goods_rec.UnitID, &unit_rec) > 0) {
					STRNSCPY(sdr_lot.UnitName, unit_rec.Name);
				}
			}
		}
	}
	temp_buf = pItem->Serial;
	p_bobj->ReleaseSerialFromUniqSuffix(temp_buf);
	temp_buf.CopyTo(sdr_lot.Serial, sizeof(sdr_lot.Serial));
	if(UhttCli.GetState() & PPUhttClient::stHasAccount && Param.UhttGoodsCodeArID) {
		ArticleTbl::Rec ar_rec;
		if(ArObj.Fetch(Param.UhttGoodsCodeArID, &ar_rec) > 0) {
			PPID   acs_id = 0;
			const  PPID psn_id = ObjectToPerson(ar_rec.ID, &acs_id);
			if(psn_id) {
				SString inn;
				PsnObj.GetRegNumber(psn_id, PPREGT_TPID, inn);
				if(inn.NotEmptyS()) {
					const bool _cd = LOGIC(GObj.GetConfig().Flags & GCF_BCCHKDIG);
					SString org_code, adj_code;
					BarcodeArray bc_list;
					GObj.ReadBarcodes(pItem->GoodsID, bc_list);
					if(bc_list.getCount()) {
						for(uint i = 0; i < bc_list.getCount(); i++) {
							if(UhttCli.GetState() & PPUhttClient::stAuth || UhttCli.Auth()) {
								org_code = bc_list.at(i).Code;
								if(UhttCli.GetGoodsArCode(org_code, inn, temp_buf) && temp_buf.NotEmptyS()) {
									temp_buf.CopyTo(sdr_lot.UhttArCode, sizeof(sdr_lot.UhttArCode));
									break;
								}
								else if(!_cd && UhttCli.GetGoodsArCode(AddBarcodeCheckDigit(adj_code = org_code), inn, temp_buf) && temp_buf.NotEmptyS()) {
									temp_buf.CopyTo(sdr_lot.UhttArCode, sizeof(sdr_lot.UhttArCode));
									break;
								}
								else {
									int d = 0;
									int std = 0;
									int r = GObj.DiagBarcode(org_code, &d, &std, &adj_code);
									if(r < 0 && oneof4(d, PPObjGoods::cdd_UpcaWoCheckDig, PPObjGoods::cdd_Ean13WoCheckDig, PPObjGoods::cdd_Ean8WoCheckDig, PPObjGoods::cdd_UpceWoCheckDig)) {
										if(UhttCli.GetGoodsArCode(adj_code, inn, temp_buf.Z()) && temp_buf.NotEmptyS()) {
											temp_buf.CopyTo(sdr_lot.UhttArCode, sizeof(sdr_lot.UhttArCode));
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	{
		SdbField _f;
		StringSet ss_ext_codes;
		uint   ext_code_count = 0;
		LotExtCodeCore * p_lec = p_bobj->P_LotXcT;
		if(p_lec && Param.OtrRec.GetFieldByName_Fast("EgaisMark", &_f)) {
			p_lec->GetMarkListByLot(pItem->ID, 0, ss_ext_codes, &ext_code_count);
			assert(ext_code_count == ss_ext_codes.getCount());
			const  PPID lot_bill_id = pItem->BillID;
			TransferTbl::Rec trfr_rec;
			for(DateIter di; p_bobj->trfr->EnumByLot(pItem->ID, &di, &trfr_rec) > 0;) {
				if(trfr_rec.BillID == lot_bill_id) {
					int16 row_idx = 0;
					int   row_is_found = 0;
					for(int   rbb_iter = 0; !row_is_found && p_bobj->trfr->EnumItems(lot_bill_id, &rbb_iter, 0) > 0;) {
						row_idx++;
						if(rbb_iter == trfr_rec.RByBill)
							row_is_found = 1;
					}
					if(row_is_found) {
						StringSet local_ss;
						uint local_count = 0;
						p_lec->GetListByBillRow(lot_bill_id, row_idx, false, local_ss, &local_count);
						ss_ext_codes.add(local_ss);
						ext_code_count += local_count;
						break;
					}
				}
			}
		}
		if(Param.OtrRec.GetFieldByName_Fast("EgaisRefA", &_f)) {
			if(p_ref->Ot.GetTagStr(PPOBJ_LOT, pItem->ID, PPTAG_LOT_FSRARINFA, temp_buf) > 0)
				STRNSCPY(sdr_lot.EgaisRefA, temp_buf);
		}
		if(Param.OtrRec.GetFieldByName_Fast("EgaisRefB", &_f)) {
			if(p_ref->Ot.GetTagStr(PPOBJ_LOT, pItem->ID, PPTAG_LOT_FSRARINFB, temp_buf) > 0)
				STRNSCPY(sdr_lot.EgaisRefB, temp_buf);
		}
		if(Param.OtrRec.GetFieldByName_Fast("EgaisCode", &_f)) {
			if(p_ref->Ot.GetTagStr(PPOBJ_LOT, pItem->ID, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0)
				STRNSCPY(sdr_lot.EgaisCode, temp_buf);
		}
		if(Param.OtrRec.GetFieldByName_Fast("VetisCertGUID", &_f)) {
			S_GUID uuid;
			if(p_ref->Ot.GetTagGuid(PPOBJ_LOT, pItem->ID, PPTAG_LOT_VETIS_UUID, uuid) > 0) {
				uuid.ToStr(S_GUID::fmtIDL, temp_buf);
				STRNSCPY(sdr_lot.VetisCertGUID, temp_buf);
			}
		}
		if(ext_code_count) {
			const Sdr_Lot org_sdr_lot = sdr_lot;
			for(uint ssp = 0; ss_ext_codes.get(&ssp, temp_buf);) {
				STRNSCPY(sdr_lot.EgaisMark, temp_buf);
				Param.InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr_lot);
				THROW(P_IE->AppendRecord(&sdr_lot, sizeof(sdr_lot)));
				sdr_lot = org_sdr_lot;
			}
		}
		else {
			Param.InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr_lot);
			THROW(P_IE->AppendRecord(&sdr_lot, sizeof(sdr_lot)));
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
int EditLotExtCode(LotExtCodeTbl::Rec & rRec, char firstChar)
{
	int    ok = -1;
	uint   sel = 0;
	TDialog * dlg = new TDialog(DLG_LOTEXTCODE);
	SString temp_buf, info_buf;
	SString mark_buf;
	ReceiptTbl::Rec lot_rec;
	ReceiptCore & r_rcpt = BillObj->trfr->Rcpt;
	THROW(CheckDialogPtr(&dlg));
	THROW(r_rcpt.Search(rRec.LotID, &lot_rec) > 0);
	if(firstChar) {
		temp_buf.Z().CatChar(firstChar);
	}
	else
		(temp_buf = rRec.Code).Strip();
	dlg->setCtrlString(CTL_LOTEXTCODE_CODE, temp_buf);
	if(firstChar) {
		TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_LOTEXTCODE_CODE));
		CALLPTRMEMB(il, disableDeleteSelection(1));
	}
	GetGoodsName(lot_rec.GoodsID, temp_buf);
	info_buf.Z().CatEq("LotID", lot_rec.ID).Space().Cat(lot_rec.Dt, DATF_DMY|DATF_CENTURY).CR().Cat(temp_buf);
	dlg->setStaticText(CTL_LOTEXTCODE_INFO, info_buf);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		dlg->getCtrlString(sel = CTL_LOTEXTCODE_CODE, temp_buf);
		if(!temp_buf.NotEmptyS())
			PPErrorByDialog(dlg, sel, PPERR_CODENEEDED);
		else if(temp_buf.Len() >= sizeof(rRec.Code)) {
			PPSetError(PPERR_CODETOOLONG, (long)(sizeof(rRec.Code)-1));
			PPErrorByDialog(dlg, sel);
		}
		else if(!PrcssrAlcReport::IsEgaisMark(temp_buf, &mark_buf)) {
			PPSetError(PPERR_TEXTISNTEGAISMARK, temp_buf);
			PPErrorByDialog(dlg, sel);
		}
		else {
			STRNSCPY(rRec.Code, mark_buf);
			// PPBarcode::CreateImage(temp_buf, BARCSTD_PDF417, SFileFormat::Png, 0); // @debug
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

IMPLEMENT_PPFILT_FACTORY(LotExtCode); LotExtCodeFilt::LotExtCodeFilt() : PPBaseFilt(PPFILT_LOTEXTCODE, 0, 1)
{
	SetFlatChunk(offsetof(LotExtCodeFilt, ReserveStart),
		offsetof(LotExtCodeFilt, Reserve)-offsetof(LotExtCodeFilt, ReserveStart)+sizeof(Reserve));
	SetBranchSString(offsetof(LotExtCodeFilt, SrchStr));
	Init(1, 0);
}

PPViewLotExtCode::PPViewLotExtCode() : PPView(0, &Filt, PPVIEW_LOTEXTCODE, implDontEditNullFilter, 0), P_BObj(BillObj)
{
}

PPViewLotExtCode::~PPViewLotExtCode()
{
}

int PPViewLotExtCode::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	CATCHZOK
	return ok;
}

int PPViewLotExtCode::InitIteration()
{
	int    ok = 1;
	LotExtCodeTbl::Key0 k0, k0_;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	MEMSZERO(k0);
	k0.LotID = Filt.LotID;
	THROW_MEM(P_IterQuery = new BExtQuery(&Tbl, 0));
	P_IterQuery->selectAll().where(Tbl.LotID == Filt.LotID);
	k0_ = k0;
	Counter.Init(P_IterQuery->countIterations(0, &k0_, spGe));
	P_IterQuery->initIteration(false, &k0, spGe);
	CATCHZOK
	return ok;
}

int FASTCALL PPViewLotExtCode::NextIteration(LotExtCodeViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
		if(pItem) {
            pItem->LotID = Tbl.data.LotID;
            STRNSCPY(pItem->Code, Tbl.data.Code);
		}
		ok = 1;
	}
	return ok;
}

PPBaseFilt * PPViewLotExtCode::CreateFilt(const void * extraPtr) const
{
	LotExtCodeFilt * p_filt = new LotExtCodeFilt;
	if(p_filt)
		p_filt->LotID = reinterpret_cast<long>(extraPtr);
	return p_filt;
}

int PPViewLotExtCode::EditBaseFilt(PPBaseFilt * pFilt) { return -1; }

int PPViewLotExtCode::GetRec(const void * pHdr, LotExtCodeTbl::Rec & rRec)
{
	int    ok = 0;
	if(pHdr) {
		LotExtCodeTbl::Key0 k0;
		MEMSZERO(k0);
		k0.LotID = *static_cast<const long *>(pHdr);
		STRNSCPY(k0.Code, reinterpret_cast<const char *>(PTR8C(pHdr)+sizeof(long)));
		if(Tbl.search(0, &k0, spEq)) {
			Tbl.copyBufTo(&rRec);
			ok = 1;
		}
	}
	return ok;
}

DBQuery * PPViewLotExtCode::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = BROWSER_LOTEXTCODE;
	DBDataCell fld_list[20];
	int    c = 0;
	LotExtCodeTbl * t = 0;
	ReceiptTbl * rcp = 0;
	DBE    dbe_goods;
	DBQ  * dbq = 0;
	DBQuery * q = 0;
	THROW(CheckTblPtr(t = new LotExtCodeTbl));
	THROW(CheckTblPtr(rcp = new ReceiptTbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, rcp->GoodsID);
	fld_list[c++] = t->LotID;  // #0
	fld_list[c++] = t->Code;   // #1
	fld_list[c++] = rcp->Dt;   // #2
	fld_list[c++] = dbe_goods; // #3
	dbq = &(t->LotID == Filt.LotID && t->LotID == rcp->ID);
	q = &selectbycell(c, fld_list).from(t, rcp, 0L).where(*dbq);
	THROW(CheckQueryPtr(q));
	{
		SString sub_title;
		ReceiptTbl::Rec lot_rec;
		if(Filt.LotID && P_BObj->trfr->Rcpt.Search(Filt.LotID, &lot_rec) > 0)
			P_BObj->MakeLotText(&lot_rec, PPObjBill::ltfGoodsName, sub_title);
		ASSIGN_PTR(pSubTitle, sub_title);
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete t;
			delete rcp;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewLotExtCode::CheckDupCode(const LotExtCodeTbl::Rec & rRec)
{
	int    ok = 1;
	LotExtCodeTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.Code, rRec.Code);
	k1.BillID = rRec.BillID;
	if(Tbl.search(1, &k1, spGe) && sstreqi_ascii(Tbl.data.Code, rRec.Code) && Tbl.data.BillID == rRec.BillID) {
		SString msg_buf, temp_buf;
		ReceiptTbl::Rec lot_rec;
		if(P_BObj->trfr->Rcpt.Search(Tbl.data.LotID, &lot_rec) > 0)
			P_BObj->MakeLotText(&lot_rec, PPObjBill::ltfGoodsName, temp_buf);
		else
			ideqvalstr(Tbl.data.LotID, temp_buf);
		(msg_buf = rRec.Code).CatDiv('-', 1).Cat(temp_buf);
		ok = PPSetError(PPERR_DUPEXTLOTCODE, msg_buf);
	}
	return ok;
}

void PPViewLotExtCode::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_LOTEXTCTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long count = 0;
		LotExtCodeViewItem item;
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWaitStop();
		dlg->setCtrlLong(CTL_LOTEXTCTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
	}
}

int PPViewLotExtCode::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   lot_id = pHdr ? *static_cast<const  PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				if(Filt.LotID) {
					LotExtCodeTbl::Rec rec;
					rec.LotID = Filt.LotID;
                	if(EditLotExtCode(rec, 0) > 0) {
						THROW(CheckDupCode(rec));
						{
							PPTransaction tra(1);
							THROW(tra);
							THROW_DB(Tbl.insertRecBuf(&rec));
							THROW(tra.Commit());
							ok = 1;
						}
                	}
                }
				break;
			case PPVCMD_INPUTCHAR:
				if(pHdr) {
					char c = *static_cast<const char *>(pHdr);
					if(isasciialnum(c)) {
						LotExtCodeTbl::Rec rec;
						rec.LotID = Filt.LotID;
                		if(EditLotExtCode(rec, c) > 0) {
							THROW(CheckDupCode(rec));
							{
								PPTransaction tra(1);
								THROW(tra);
								THROW_DB(Tbl.insertRecBuf(&rec));
								THROW(tra.Commit());
								ok = 1;
							}
                		}
					}
					else
						ok = -2;
				}
				break;
			case PPVCMD_EDITITEM:
				{
					LotExtCodeTbl::Rec rec, org_rec;
					if(GetRec(pHdr, org_rec) > 0) {
						rec = org_rec;
						DBRowId _dbpos;
						THROW_DB(Tbl.getPosition(&_dbpos));
						if(EditLotExtCode(rec, 0) && !sstreq(rec.Code, org_rec.Code)) {
							PPTransaction tra(1);
							THROW(tra);
							THROW_DB(Tbl.getDirectForUpdate(0, 0, _dbpos));
							THROW_DB(Tbl.updateRecBuf(&rec));
							THROW(tra.Commit());
							ok = 1;
						}
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				{
					LotExtCodeTbl::Rec rec, org_rec;
					if(GetRec(pHdr, org_rec) > 0) {
						rec = org_rec;
						DBRowId _dbpos;
						THROW_DB(Tbl.getPosition(&_dbpos));
						if(CONFIRM(PPCFM_DELETE)) {
							PPTransaction tra(1);
							THROW(tra);
							THROW_DB(Tbl.getDirectForUpdate(0, 0, _dbpos));
							THROW_DB(Tbl.deleteRec());
							THROW(tra.Commit());
							ok = 1;
						}
					}
				}
				break;
		}
	}
	CATCHZOKPPERR
	return ok;
}
