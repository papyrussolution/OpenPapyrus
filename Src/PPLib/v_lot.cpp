// V_LOT.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewLot)
//
int SLAPI LotFilt::InitInstance()
{
	P_TagF = 0;
	SetFlatChunk(offsetof(LotFilt, ReserveStart),
		offsetof(LotFilt, Reserve)-offsetof(LotFilt, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(LotFilt, P_TagF)); // @v7.4.5
	SetBranchSString(offsetof(LotFilt, ExtString)); // @v8.4.11
	return Init(1, 0);
}

IMPLEMENT_PPFILT_FACTORY(Lot); SLAPI LotFilt::LotFilt() : PPBaseFilt(PPFILT_LOT, 0, 2) // @v7.4.5 ver 0-->1 // @v8.4.11 1-->2
{
	InitInstance();
}

SLAPI LotFilt::LotFilt(const LotFilt & rS) : PPBaseFilt(PPFILT_LOT, 0, 1)
{
	InitInstance();
	Copy(&rS, 1);
}

LotFilt & FASTCALL LotFilt::operator = (const LotFilt & s)
{
	Copy(&s, 0);
	return *this;
}

int SLAPI LotFilt::GetExtssData(int fldID, SString & rBuf) const
{
	return PPGetExtStrData(fldID, ExtString, rBuf);
}

int SLAPI LotFilt::PutExtssData(int fldID, const char * pBuf)
{
	return PPPutExtStrData(fldID, ExtString, pBuf);
}

//virtual
int SLAPI LotFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 0) {
		struct LotFilt_v0 : public PPBaseFilt {
			SLAPI LotFilt_v0() : PPBaseFilt(PPFILT_LOT, 0, 0)
			{
				SetFlatChunk(offsetof(LotFilt, ReserveStart),
					offsetof(LotFilt, Reserve)-offsetof(LotFilt, ReserveStart)+sizeof(Reserve));
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
			PPID   LocID;
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
		CPYFLD(LocID);
		CPYFLD(SupplID);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(QCertID);
		CPYFLD(InTaxGrpID);
		CPYFLD(Flags);
		CPYFLD(ClosedTag);
		CPYFLD(CostRange);
		CPYFLD(PriceRange);
		// @v8.4.11 STRNSCPY(Serial, fv0.Serial);
#undef CPYFLD
		// @v8.4.11 {
		if(fv0.Serial[0]) {
			PutExtssData(extssSerialText, fv0.Serial);
		}
		// } @v8.4.11
		ok = 1;
	}
	else if(ver == 1) {
		struct LotFilt_v1 : public PPBaseFilt {
			SLAPI  LotFilt_v1() : PPBaseFilt(PPFILT_LOT, 0, 1)
			{
				SetFlatChunk(offsetof(LotFilt, ReserveStart),
					offsetof(LotFilt, Reserve)-offsetof(LotFilt, ReserveStart)+sizeof(Reserve));
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
			PPID   LocID;
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
		CPYFLD(LocID);
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
	CATCHZOK
	return ok;
}

// virtual
int SLAPI LotFilt::Describe(long flags, SString & rBuf) const
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
	// @v8.4.11 PutMembToBuf(Serial,          STRINGIZE(Serial),      rBuf);
	PutMembToBuf(&CostRange,      STRINGIZE(CostRange),   rBuf);
	PutMembToBuf(&PriceRange,     STRINGIZE(PriceRange),  rBuf);

	PutObjMembToBuf(PPOBJ_LOCATION,   LocID,       STRINGIZE(LocID),       rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    SupplID,     STRINGIZE(SupplID),     rBuf);
	PutObjMembToBuf(PPOBJ_GOODSGROUP, GoodsGrpID,  STRINGIZE(GoodsGrpID),  rBuf);
	PutObjMembToBuf(PPOBJ_GOODS,      GoodsID,     STRINGIZE(GoodsID),     rBuf);
	PutObjMembToBuf(PPOBJ_QCERT,      QCertID,     STRINGIZE(QCertID),     rBuf);
	PutObjMembToBuf(PPOBJ_LOT,        ParentLotID, STRINGIZE(ParentLotID), rBuf);
	PutObjMembToBuf(PPOBJ_GOODSTAX,   InTaxGrpID,  STRINGIZE(InTaxGrpID),  rBuf);
	{
		long id = 1;
		StrAssocArray flag_list;
		if(Flags & fEmptyPeriod)        flag_list.Add(id++, STRINGIZE(fEmptyPeriod));
		if(Flags & fWithoutQCert)       flag_list.Add(id++, STRINGIZE(fWithoutQCert));
		if(Flags & fOrders)             flag_list.Add(id++, STRINGIZE(fOrders));
		if(Flags & fCostAbovePrice)     flag_list.Add(id++, STRINGIZE(fCostAbovePrice));
		if(Flags & fWithoutClb)         flag_list.Add(id++, STRINGIZE(fWithoutClb));
		if(Flags & fDeadLots)           flag_list.Add(id++, STRINGIZE(fDeadLots));
		if(Flags & fWithoutExpiry)      flag_list.Add(id++, STRINGIZE(fWithoutExpiry));
		if(Flags & fOnlySpoilage)       flag_list.Add(id++, STRINGIZE(fOnlySpoilage));
		if(Flags & fShowSerialN)        flag_list.Add(id++, STRINGIZE(fShowSerialN));
		if(Flags & fSkipNoOp)           flag_list.Add(id++, STRINGIZE(fSkipNoOp));
		if(Flags & fCheckOriginLotDate) flag_list.Add(id++, STRINGIZE(fCheckOriginLotDate));
		if(Flags & fSkipClosedBeforeOp) flag_list.Add(id++, STRINGIZE(fSkipClosedBeforeOp));
		if(Flags & fNoTempTable)        flag_list.Add(id++, STRINGIZE(fNoTempTable));
		if(Flags & fShowBillStatus)     flag_list.Add(id++, STRINGIZE(fShowBillStatus));
		if(Flags & fShowPriceDev)       flag_list.Add(id++, STRINGIZE(fShowPriceDev));
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	return 1;
}
//
//
//
int SLAPI ConvertLandQCertToLotTag()
{
	return (PPError(PPERR_FUNCNOTMORESUPPORTED), 0); // @v8.1.12
#if 0 // @v8.1.12 {
	int    ok = -1, ta = 0;
	int    valid_data = 0;
	struct Filt {
		PPID      GGrp;
		DateRange Period;
		PPID      LotTag;
		PPID      PsnKind;
	} filt;
	PPLogger logger;
	PPObjTag obj_tag;
	TDialog * p_dlg = new TDialog(DLG_CVTQCERT);
	THROW(CheckDialogPtr(&p_dlg));

	MEMSZERO(filt);
	p_dlg->SetupCalPeriod(CTLCAL_CVTQCERT_PERIOD, CTL_CVTQCERT_PERIOD);
	SetupPPObjCombo(p_dlg, CTLSEL_CVTQCERT_PSNKIND, PPOBJ_PRSNKIND, 0, 0, 0);
	SetPeriodInput(p_dlg, CTL_CVTQCERT_PERIOD, 0, &filt.Period);
	{
		ObjTagFilt ot_filt(PPOBJ_LOT, ObjTagFilt::fOnlyTags);
		SetupObjTagCombo(p_dlg, CTLSEL_CVTQCERT_LOTTAG, 0, 0, &ot_filt);
	}
	SetupPPObjCombo(p_dlg, CTLSEL_CVTQCERT_GGRP, PPOBJ_GOODSGROUP, 0, 0, 0);
	/*
	{
		PPIDArray opt_list;
		opt_list.add(PPOPT_GOODSRECEIPT);
		SetupOprKindCombo(p_dlg, CTLSEL_CVTQCERT_OPRKIND, &opt_list, 0);
	}
	*/
	for(valid_data = 0; !valid_data && ExecView(p_dlg) == cmOK;) {
		p_dlg->getCtrlData(CTLSEL_CVTQCERT_PSNKIND, &filt.PsnKind);
		p_dlg->getCtrlData(CTLSEL_CVTQCERT_LOTTAG,  &filt.LotTag);
		p_dlg->getCtrlData(CTLSEL_CVTQCERT_GGRP,   &filt.GGrp);
		// getCtrlData(CTLSEL_CVTQCERT_OPRKIND, &filt.OprKind);

		if(GetPeriodInput(p_dlg, CTL_CVTQCERT_PERIOD, &filt.Period) <= 0);
		else if(!filt.PsnKind)
			PPError(PPERR_PSNKINDNEEDED);
		else if(!filt.LotTag)
			PPError(PPERR_TAGNEEDED);
		/*
		else if(!filt.OprKind)
			PPError(PPERR_INVOP);
		*/
		else {
			PPObjTagPacket tag_pack;
			if(obj_tag.GetPacket(filt.LotTag, &tag_pack) > 0) {
				if(tag_pack.Rec.TagEnumID != PPOBJ_PERSON || tag_pack.Rec.LinkObjGrp != filt.PsnKind)
					PPError(PPERR_CVTQCERT_INVTAG);
				else
					valid_data = 1;
			}
		}
	}
	delete p_dlg;
	p_dlg = 0;

	PPWait(1);
	if(valid_data == 1) {
		long            status = 0L;
		SString         msg_psn_crt, msg_qcert_cvtd;
		LotFilt         lot_filt;
		PPPersonKind    psnk_rec;
		LotViewItem     item;
		PPObjBill       bobj;
		PPObjPerson     psn_obj;
		PPObjPersonKind psnk_obj;
		PPObjQCert      qc_obj;
		PPViewLot       view;

		if(psnk_obj.Search(filt.PsnKind, &psnk_rec) > 0)
			status = psnk_rec.DefStatusID;

		PPLoadText(PPTXT_PERSON_CREATED, msg_psn_crt);
		PPLoadText(PPTXT_QCERT_CONVERTED, msg_qcert_cvtd);
		lot_filt.Period     = filt.Period;
		lot_filt.GoodsGrpID = filt.GGrp;
		THROW(PPStartTransaction(&ta, 1));
		THROW(view.Init_(&lot_filt));
		for(view.InitIteration(PPViewLot::OrdByDefault); view.NextIteration(&item) > 0;) {
			ObjTagList tag_list;
			THROW(bobj.GetTagListByLot(item.ID, 0, &tag_list)); // @v7.9.4 skipReserveTags 1-->0
			/*if(!tag_list.GetItem(filt.LotTag))*/ {
				 PPID  psn_id = 0L;
				 QualityCertTbl::Rec qc_rec;

				 MEMSZERO(qc_rec);
				 if(qc_obj.Search(item.QCertID, &qc_rec) > 0 && strlen(qc_rec.Code) > 0 && strlen(qc_rec.Manuf) > 0) {
					PPIDArray psn_list;
					if(psn_obj.GetListByRegNumber(PPREGT_TPID, filt.PsnKind, qc_rec.Code, psn_list) > 0 && psn_list.getCount()) // Поиск персоналии по ИНН
						psn_id = psn_list.at(0);
					else if(psn_obj.P_Tbl->SearchByName(qc_rec.Manuf, &psn_id) <= 0) { // Поиск персоналии по наименованию
						SString out_msg;
						PPPersonPacket psn_pack;

						STRNSCPY(psn_pack.Rec.Name, qc_rec.Manuf);
						psn_pack.Rec.Status = status;
						psn_pack.Kinds.add(filt.PsnKind);
						psn_pack.SetExtName(qc_rec.Manuf);
						psn_pack.AddRegister(PPREGT_TPID, qc_rec.Code);
						THROW(psn_obj.PutPacket(&psn_id, &psn_pack, 0));
						out_msg.Printf(msg_psn_crt, qc_rec.Manuf, qc_rec.Code);
						logger.Log(out_msg);
					}
				 }
				 if(psn_id) {
					ObjTagItem tag;
					tag.SetInt(filt.LotTag, psn_id);
				 	THROW(tag_list.PutItem(0, &tag));
				 	THROW(PPRef->Ot.PutList(PPOBJ_LOT, item.ID, &tag_list, 0));
					{
						SString out_msg, psn_name, goods_name, bill_dt;
						BillTbl::Rec bill_rec;

						MEMSZERO(bill_rec);
						GetObjectName(PPOBJ_PERSON, psn_id, psn_name);
						GetObjectName(PPOBJ_GOODS,  item.GoodsID, goods_name);
						bobj.Search(item.BillID, &bill_rec);
						bill_dt.Cat(bill_rec.Dt);

						out_msg.Printf(msg_qcert_cvtd, psn_name.cptr(), goods_name.cptr(), bill_rec.Code, bill_dt.cptr());
						logger.Log(out_msg);
					}
				 }
			}
			PPWaitPercent(view.GetCounter(), 0);
		}
		ok = 1;
		THROW(PPCommitWork(&ta));
	}
	CATCH
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	logger.Save(PPFILNAM_INFO_LOG, 1);
	delete p_dlg;
	PPWait(0);
	return ok;
#endif // } @v8.1.12
}
//
//
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
SLAPI PPViewLot::PPViewLot() : PPView(0, &Filt, PPVIEW_LOT), P_BObj(BillObj), State(0), P_Tbl(&P_BObj->trfr->Rcpt),
	P_TempTbl(0), P_SpoilTbl(0), P_PplBlkBeg(0), P_PplBlkEnd(0)
{
	SETFLAG(State, stAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
}

SLAPI PPViewLot::~PPViewLot()
{
	delete P_TempTbl;
	delete P_SpoilTbl;
	delete P_PplBlkBeg;
	delete P_PplBlkEnd;
}

PPBaseFilt * PPViewLot::CreateFilt(void * extraPtr) const
{
	LotFilt * p_filt = new LotFilt;
	if(p_filt) {
		if(((long)extraPtr) == 1)
			p_filt->Flags |= LotFilt::fOrders;
		p_filt->Period.upp = LConfig.OperDate;
	}
	return p_filt;
}
//
// LotFiltDialog
// PPViewLot::EditFilt with helpers
//
#define GRP_GOODS     1
#define GRP_GOODSFILT 2

class LotFiltDialog : public TDialog {
public:
 	LotFiltDialog(uint dlgID) : TDialog(dlgID)
	{
		addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(CTLSEL_FLTLOT_GOODS, CTLSEL_FLTLOT_GGRP, cmGoodsFilt));
		SetupCalPeriod(CTLCAL_FLTLOT_PERIOD, CTL_FLTLOT_PERIOD);
		SetupCalPeriod(CTLCAL_FLTLOT_OPERAT, CTL_FLTLOT_OPERAT);
	}
	int    setDTS(const LotFilt*);
	int    getDTS(LotFilt*);
private:
	DECL_HANDLE_EVENT;
	LotFilt Data;
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

int LotFiltDialog::setDTS(const LotFilt * pFilt)
{
	Data = *pFilt;
	SetPeriodInput(this, CTL_FLTLOT_PERIOD, &Data.Period);
	SetPeriodInput(this, CTL_FLTLOT_OPERAT, &Data.Operation);
	const PPID suppl_acs_id = (Data.Flags & LotFilt::fOrders) ? GetSellAccSheet() : GetSupplAccSheet();
	setCtrlData(CTL_FLTLOT_CLOSED, &Data.ClosedTag);
	SetupPPObjCombo(this, CTLSEL_FLTLOT_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
	if(BillObj->CheckRights(BILLOPRT_ACCSSUPPL, 1))
		SetupArCombo(this, CTLSEL_FLTLOT_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, suppl_acs_id, sacfDisableIfZeroSheet);
	GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
	setGroupData(GRP_GOODSFILT, &gf_rec);
	if(Data.Flags & LotFilt::fOrders) {
		AddClusterAssoc(CTL_FLTLOT_FLAGS, 0, LotFilt::fShowBillStatus);
		AddClusterAssoc(CTL_FLTLOT_FLAGS, 1, LotFilt::fShowSerialN);
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
		AddClusterAssoc(CTL_FLTLOT_FLAGS, 11, LotFilt::fLotfPrWoTaxes); // @v8.9.0
	}
	SetClusterData(CTL_FLTLOT_FLAGS, Data.Flags);
	return 1;
}

int LotFiltDialog::getDTS(LotFilt * pFilt)
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
	getCtrlData(CTLSEL_FLTLOT_LOC,   &Data.LocID);
	getCtrlData(CTLSEL_FLTLOT_SUPPL, &Data.SupplID);
	THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
	Data.GoodsGrpID = gf_rec.GoodsGrpID;
	Data.GoodsID    = gf_rec.GoodsID;
	GetClusterData(CTL_FLTLOT_FLAGS, &Data.Flags);
	ASSIGN_PTR(pFilt, Data);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewLot::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	LotFilt * p_filt = (LotFilt *)pBaseFilt;
	uint   dlg_id = (p_filt->Flags & LotFilt::fOrders) ? DLG_FLTLOTORD : DLG_FLTLOT;
	DIALOG_PROC_BODY_P1(LotFiltDialog, dlg_id, p_filt);
}
//
// PPViewLot::MovLotOps with helpers
//
class MovLotOpsDialog : public TDialog {
public:
	MovLotOpsDialog() : TDialog(DLG_MOVLOTOPS)
	{
	}
	int    setDTS(const long * fl)
	{
		long   f = *fl;
		AddClusterAssoc(CTL_MOVLOTOPS_MERGE, 0, TMLOF_ADDLOTS);
		SetClusterData(CTL_MOVLOTOPS_MERGE, f);
		AddClusterAssoc(CTL_MOVLOTOPS_AVG, 0, TMLOF_AVGCOST);
		AddClusterAssoc(CTL_MOVLOTOPS_AVG, 1, TMLOF_AVGPRICE);
		SetClusterData(CTL_MOVLOTOPS_AVG, f);
		setCtrlUInt16(CTL_MOVLOTOPS_RMVSRC, BIN(f & TMLOF_RMVSRCLOT));
		setCtrlUInt16(CTL_MOVLOTOPS_RMVREVAL, BIN(f & (TMLOF_RMVSRCLOT | TMLOF_RMVREVAL)));
		disableCtrl(CTL_MOVLOTOPS_AVG, !BIN(f & TMLOF_ADDLOTS));
		disableCtrl(CTL_MOVLOTOPS_RMVREVAL, BIN(f & TMLOF_RMVSRCLOT));
		return 1;
	}
	int    getDTS(long * fl)
	{
		long   f = 0;
		GetClusterData(CTL_MOVLOTOPS_MERGE, &f);
		GetClusterData(CTL_MOVLOTOPS_AVG,   &f);
		SETFLAG(f, TMLOF_RMVSRCLOT, getCtrlUInt16(CTL_MOVLOTOPS_RMVSRC));
		if(f & TMLOF_RMVSRCLOT)
			f |= TMLOF_RMVREVAL;
		else {
			SETFLAG(f, TMLOF_RMVREVAL, getCtrlUInt16(CTL_MOVLOTOPS_RMVREVAL));
		}
		ASSIGN_PTR(fl, f);
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

int SLAPI PPViewLot::MovLotOps(PPID srcLotID)
{
	int    ok = -1;
	//PPID   dest_id = 0;
	ReceiptTbl::Rec lot_rec;
	if(srcLotID && P_Tbl->Search(srcLotID, &lot_rec) > 0) {
		//if(SelectLot(rec.LocID, rec.GoodsID, srcLotID, &dest_id, 0) > 0 && dest_id) {
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
int FASTCALL PPViewLot::AddDerivedLotToTotal(ReceiptTbl::Rec * pRec)
{
	const double rest = pRec->Rest;
	Total.DCount++;
	Total.DRest += rest;
	if(State & stAccsCost)
		Total.DCost += R5(pRec->Cost) * rest;
	Total.DPrice += R5(pRec->Price) * rest;
	return 1;
}

//static
int PPViewLot::CalcChildLots(ReceiptTbl::Rec * pLotRec, void * extraPtr)
{
	PPViewLot * p_lv = (PPViewLot *)extraPtr;
	return (p_lv && pLotRec->LocID != p_lv->Total.LocID) ? p_lv->AddDerivedLotToTotal(pLotRec) : 0;
}

int SLAPI PPViewLot::CalcTotal(LotTotal::Status stat, LotTotal * pTotal)
{
	int    ok = 1;
	LotViewItem item;
	PPWait(1);
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
					THROW(P_Tbl->GatherChilds(item.ID, 0, PPViewLot::CalcChildLots, this));
			}
			Total.Stat = LotTotal::Extended;
		}
	}
	CATCH
		Total.Stat = LotTotal::Undef;
		ok = 0;
	ENDCATCH
	PPWait(0);
	if(pTotal)
		memcpy(pTotal, &Total, sizeof(LotTotal));
	return ok;
}

int SLAPI PPViewLot::ViewTotal()
{
	class LotTotalDialog : public TDialog {
	public:
		LotTotalDialog(PPViewLot * lv) : TDialog(DLG_LOTTOTAL)
		{
			LV = lv;
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
	return 1;
}
//
// PPViewLot::RecoverLots with helpers
//
struct LotRecoverParam {
	LotRecoverParam()
	{
		Flags = 0;
		MinusCompensOpID = 0;
	}
	long   Flags;
	PPID   MinusCompensOpID;
	SString LogFileName;
};

static int SLAPI RecoverLotsDialog(LotRecoverParam & rParam)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_CORLOTS);
	if(CheckDialogPtr(&dlg)) {
		SString temp_buf;
		PPLoadString("lotrecoverparam_minuscompensop_hint", temp_buf);
		dlg->setStaticText(CTL_CORLOTS_MCOP_HINT, temp_buf);
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORLOTS_LOG, CTL_CORLOTS_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		// @v8.0.9 {
		PPIDArray op_type_list;
		op_type_list.add(PPOPT_GOODSRECEIPT);
		SetupOprKindCombo(dlg, CTLSEL_CORLOTS_MCOP, rParam.MinusCompensOpID, 0, &op_type_list, 0);
		// } @v8.0.9
		dlg->setCtrlString(CTL_CORLOTS_LOG, rParam.LogFileName);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  0, TLRF_REPAIR);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  1, TLRF_REPAIRPACK);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  2, TLRF_REPAIRPACKUNCOND); // @v8.8.1
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  3, TLRF_REPAIRCOST);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  4, TLRF_REPAIRPRICE);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  5, TLRF_RMVLOST);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  6, TLRF_CHECKUNIQSERIAL);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  7, TLRF_ADJUNUQSERIAL);
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  8, TLRF_INDEPHQTTY); // @v8.3.9
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS,  9, TLRF_REPAIRWOTAXFLAGS); // @v8.9.0
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 10, TLRF_SETALCCODETOGOODS); // @v9.3.1
		dlg->AddClusterAssoc(CTL_CORLOTS_FLAGS, 11, TLRF_SETALCCODETOLOTS); // @v9.7.8
		if(!(LConfig.Flags & CFGFLG_USEPACKAGE)) {
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 1, 1);
			rParam.Flags &= ~(TLRF_REPAIRPACK);
		}
		dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 6, BIN(PPObjBill::VerifyUniqSerialSfx(BillObj->GetConfig().UniqSerialSfx) <= 0)); // @v7.3.0
		if(!PPMaster) {
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 2, 1);
			dlg->DisableClusterItem(CTL_CORLOTS_FLAGS, 3, 1);
			rParam.Flags &= ~(TLRF_REPAIRCOST | TLRF_REPAIRPRICE);
		}
		dlg->SetClusterData(CTL_CORLOTS_FLAGS, rParam.Flags);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_CORLOTS_LOG, rParam.LogFileName);
			rParam.LogFileName.Strip();
			rParam.MinusCompensOpID = dlg->getCtrlLong(CTLSEL_CORLOTS_MCOP); // @v8.0.9
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

int SLAPI PPViewLot::RecoverLots()
{
	int    ok = -1, ta = 0;
	int    frrl_tag = 0, r;
	long   err_lot_count = 0;
	int    modified = 0;
	LotRecoverParam param;
	LotViewItem  lv_item;
	PPLogger logger;
	UintHashTable goods_list;
	PPIDArray loc_list;
	SString msg_buf;
	PPGetFileName(PPFILNAM_LOTERR_LOG, param.LogFileName);
	THROW(r = RecoverLotsDialog(/*log_file_name, &c_flags*/param));
	if(r > 0) {
		PPBillPacket neg_rest_pack;
		PPWait(1);
		PPLoadText(PPTXT_CHECKLOTS, msg_buf);
		logger.Log(msg_buf);
		{
			if(param.Flags) {
				THROW(PPStartTransaction(&ta, 1));
				THROW(P_BObj->atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
				if(param.MinusCompensOpID && Filt.LocID) {
					THROW(neg_rest_pack.CreateBlank2(param.MinusCompensOpID, getcurdate_(), Filt.LocID, 0));
				}
			}
			for(InitIteration(); NextIteration(&lv_item) > 0;) {
				PPLotFaultArray ary(lv_item.ID, logger);
				THROW(r = P_BObj->trfr->CheckLot(lv_item.ID, 0, param.Flags, &ary));
				if(r < 0) {
					err_lot_count++;
					ary.AddMessage();
					if(param.Flags & (TLRF_REPAIR|TLRF_ADJUNUQSERIAL|TLRF_SETALCCODETOGOODS|TLRF_SETALCCODETOLOTS)) {
						THROW(P_BObj->trfr->RecoverLot(lv_item.ID, &ary, param.Flags, 0));
						modified = 1;
					}
				}
				{
					PPLotFault nf;
					uint   nf_pos = 0;
					if(neg_rest_pack.Rec.OpID && lv_item.GoodsID > 0 && lv_item.Rest < 0.0 && ary.HasFault(PPLotFault::NegativeRest, &nf, &nf_pos)) {
						if(ary.getCount() == 1) {
							PPTransferItem ti(&neg_rest_pack.Rec, TISIGN_UNDEF);
							THROW(ti.SetupGoods(lv_item.GoodsID, 0));
							THROW(ti.SetupLot(lv_item.ID, &lv_item, 0));
							ti.Flags &= ~PPTFR_RECEIPT;
							ti.Quantity_ = -lv_item.Rest;
							ti.TFlags |= PPTransferItem::tfForceNoRcpt;
							THROW(neg_rest_pack.InsertRow(&ti, 0, 0));
						}
					}
				}
				goods_list.Add((ulong)labs(lv_item.GoodsID));
				loc_list.addUnique(lv_item.LocID);
				PPWaitPercent(GetCounter());
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
			cntr.Init((long)goods_list.GetCount());
			for(ulong goods_id = 0; goods_list.Enum(&goods_id);) {
				if(!P_BObj->trfr->CorrectCurRest((PPID)goods_id, &loc_list, &logger, BIN(param.Flags & TLRF_REPAIR)))
					logger.LogLastError();
				PPWaitPercent(cntr.Increment());
			}
		}
		logger.Save(param.LogFileName, 0);
		PPWait(0);
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
	return ok;
}

int SLAPI PPViewLot::EditLot(PPID id)
{
	int    ok = 0, valid = 0;
	uint   pos = 0, del = 0;
	LotViewItem lvi;
	PPBillPacket billp;
	PPTransferItem * p_ti;
	double cost, price, qtty, upp;
	TDialog * dlg = 0;
	SString goods_name, suppl_name;
	THROW(GetItem(id, &lvi) > 0)
	THROW(P_BObj->ExtractPacket(lvi.BillID, &billp) > 0)
	if(billp.SearchLot(id, &pos) == 0)
		if(IsIntrExpndOp(billp.Rec.OpID)) {
			THROW_PP(billp.SearchLot(lvi.PrevLotID, &pos) > 0, PPERR_LOTNOTBELONGTOBILL);
		}
		else {
			CALLEXCEPT_PP(PPERR_LOTNOTBELONGTOBILL);
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
		PPWait(1);
		if(del) {
			THROW(billp.RemoveRow(pos));
			THROW(billp.InitAmounts());
			THROW(P_BObj->FillTurnList(&billp));
			THROW(P_BObj->UpdatePacket(&billp, 1))
		}
		else if(p_ti->Price != price || p_ti->Cost != cost || p_ti->Quantity_ != qtty || p_ti->UnitPerPack != upp) {
			p_ti->Quantity_ = qtty;
   			p_ti->Price = price;
			p_ti->Cost  = cost;
			p_ti->UnitPerPack = upp;
			THROW(billp.InitAmounts());
			THROW(P_BObj->FillTurnList(&billp));
			THROW(P_BObj->UpdatePacket(&billp, 1))
		}
		ok = 1;
	}
	else
		ok = -1;
	CATCHZOKPPERR
	PPWait(0);
	delete dlg;
	return ok;
}
//
//
//
int SLAPI PPViewLot::GetItem(PPID lotID, LotViewItem * pItem)
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

int SLAPI PPViewLot::ViewBillInfo(PPID billID)
{
	return P_BObj->ViewBillInfo(billID);
}

int SLAPI PPViewLot::Init_(const PPBaseFilt * pFilt)
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
	if(Filt.LocID == 0) {
		PPObjLocation loc_obj;
		loc_obj.GetWarehouseList(&LocList);
	}
	else if(ObjRts.CheckLocID(Filt.LocID, 0))
		LocList.add(Filt.LocID);
	if(Filt.ParentLotID) {
		P_Tbl->GatherChilds(Filt.ParentLotID, &Itd.IdList, 0, 0);
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
			char   k[MAXKEYLEN];
			memzero(k, sizeof(k));
			{
				PersonTbl * p_psn_tbl = PsnObj.P_Tbl;
				BExtQuery psn_cntr_q(p_psn_tbl, 0);
				psn_cntr_q.select(p_psn_tbl->ID, p_psn_tbl->Name, 0L).where(p_psn_tbl->Status == PPPRS_COUNTRY);
				for(psn_cntr_q.initIteration(0, k, spGt); psn_cntr_q.nextIteration() > 0;)
					if(native_country_name.CmpNC(p_psn_tbl->data.Name) == 0)
						Itd.PsnNativeCntryList.add(p_psn_tbl->data.ID);
			}
			{
				PPObjWorld w_obj;
				SArray list(sizeof(WorldTbl::Rec));
				w_obj.GetListByName(WORLDOBJ_COUNTRY, native_country_name, &list);
				for(uint i = 0; i < list.getCount(); i++)
					Itd.NativeCntryList.add(((WorldTbl::Rec *)list.at(i))->ID);
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

int SLAPI PPViewLot::IsTempTblNeeded() const
{
	int    yes = 0;
	if(!(State & stNoTempTbl)) {
		const long tf = (LotFilt::fOrders|LotFilt::fDeadLots|LotFilt::fWithoutClb|
			LotFilt::fOnlySpoilage|LotFilt::fCheckOriginLotDate|LotFilt::fShowPriceDev|LotFilt::fLotfPrWoTaxes);
		if((!Filt.Operation.IsZero() || (Filt.Flags & tf) || (!Filt.GoodsID && Filt.GoodsGrpID) || !Filt.QcExpiryPrd.IsZero()) ||
			SupplList.GetCount() > 1 || Filt.ParentLotID || (Filt.P_TagF && !Filt.P_TagF->IsEmpty()) || State & stFiltSerial)
			yes = 1;
		else {
			SString temp_buf;
			Filt.GetExtssData(LotFilt::extssSerialText, temp_buf);
			if(temp_buf.NotEmptyS())
				yes = 1;
		}
	}
	return yes;
}

int SLAPI PPViewLot::PutAllToBasket()
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	param.SelPrice = 2;
	THROW(r = GetBasketByDialog(&param, GetSymb()));
	if(r > 0) {
		LotViewItem item;
		PPWait(1);
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
		PPWait(0);
		THROW(GoodsBasketDialog(param, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewLot::MakeLotListForEgaisRetReg2ToWh(PPEgaisProcessor & rEp, PPID opID, PPID locID, RAssocArray & rList)
{
	int    ok = -1;
	if(opID) {
		const LDATE _curdate = getcurdate_();
		PPViewBill bill_view;
		BillFilt bill_filt;
		bill_filt.OpID = opID;
		if(locID)
			bill_filt.LocList.Add(locID);
		if(!Filt.Operation.IsZero()) {
			bill_filt.Period = Filt.Operation;
		}
		else {
			bill_filt.Period.Set(plusdate(_curdate, -3), _curdate);
		}
		if(bill_view.Init_(&bill_filt)) {
			BillViewItem bill_item;
			PPBillPacket bp;
			SString ref_b;
			SString egais_code;
			for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&bill_item) > 0;) {
				if(P_BObj->ExtractPacket(bill_item.ID, &bp) > 0) {
					for(uint i = 0; i < bp.GetTCount(); i++) {
						const PPTransferItem & r_ti = bp.ConstTI(i);
						if(r_ti.GoodsID > 0 && r_ti.LotID && rEp.IsAlcGoods(r_ti.GoodsID)) {
							PPRef->Ot.GetTagStr(PPOBJ_LOT, r_ti.LotID, PPTAG_LOT_FSRARINFB, ref_b);
							PPRef->Ot.GetTagStr(PPOBJ_LOT, r_ti.LotID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
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
	return ok;
}

int SLAPI PPViewLot::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   lot_id = pHdr ? *(PPID *)pHdr : 0;
		switch(ppvCmd) {
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
						temp_filt.Period.SetZero();
						temp_filt.ExpiryPrd.SetZero();
						temp_filt.QcExpiryPrd.SetZero();
						temp_filt.GoodsID = item.GoodsID;
						temp_filt.LocID = item.LocID;
						ViewLots(&temp_filt, 0, 1);
					}
				}
				break;
			case PPVCMD_SYSTEMINFO:
				ok = P_BObj->EditLotSystemInfo(lot_id);
				break;
			case PPVCMD_ADDEDINFO:
				if(P_BObj->EditLotExtData(lot_id) > 0) {
					CalcTotal(LotTotal::Undef, 0);
					ok = 1;
				}
				else
					ok = -1;
				break;
			case PPVCMD_DORETURN:
				ok = P_BObj->AddRetBillByLot(lot_id);
				break;
			case PPVCMD_MOVLOTOP:
				ok = MovLotOps(lot_id);
				break;
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
			case PPVCMD_DORECOVER:
				ok = RecoverLots();
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
						P_BObj->ReleaseSerialFromUniqSuffix(serial); // @v7.8.6
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
			case PPVCMD_TAGS:
				ok = EditObjTagValList(PPOBJ_LOT, lot_id, 0);
				break;
			case PPVCMD_TAGSALL:
				ok = -1;
				{
					const PPID obj_type = PPOBJ_LOT;
					ObjTagList common_tag_list;
					common_tag_list.ObjType = obj_type;
					int   update_mode = ObjTagList::mumAdd;
					if(EditObjTagValUpdateList(&common_tag_list, 0, &update_mode) > 0 && common_tag_list.GetCount()) {
						LotViewItem item;
						PPTransaction tra(1);
						THROW(tra);
						for(InitIteration(PPViewLot::OrdByDefault); NextIteration(&item) > 0;) {
							ObjTagList local_tag_list;
							THROW(PPRef->Ot.GetList(obj_type, item.ID, &local_tag_list));
							if(local_tag_list.Merge(common_tag_list, update_mode) > 0) {
								THROW(PPRef->Ot.PutList(obj_type, item.ID, &local_tag_list, 0));
							}
							PPWaitPercent(GetCounter());
						}
						THROW(tra.Commit());
					}
				}
				break;
			case PPVCMD_LOTEXTCODE:
				if(lot_id) {
					PPView::Execute(PPVIEW_LOTEXTCODE, 0, 0 /* modal */, (void *)lot_id);
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
						PPEgaisProcessor ep(0, 0);
						SString temp_buf;
						SString egais_code;
						SString ref_a;
						SString ref_b;
						LotViewItem item;
						PPIDArray lot_list;
						RAssocArray ret_bill_lot_list;
						const PPID loc_id = NZOR(Filt.LocID, LConfig.Location);
						THROW(ep);
						PPWait(1);
						if(selection == (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000)) {
							MakeLotListForEgaisRetReg2ToWh(ep, ep.GetConfig().SupplRetOpID,  loc_id, ret_bill_lot_list);
							MakeLotListForEgaisRetReg2ToWh(ep, ep.GetConfig().IntrExpndOpID, loc_id, ret_bill_lot_list);
						}
						else {
							for(InitIteration(); NextIteration(&item) > 0;) {
								if(item.GoodsID > 0 && ep.IsAlcGoods(item.GoodsID)) {
									if(selection == PPEDIOP_EGAIS_ACTCHARGEON) {
										THROW_MEM(lot_list.add(item.ID));
									}
									else if(oneof2(selection, PPEDIOP_EGAIS_ACTCHARGEONSHOP, (PPEDIOP_EGAIS_ACTCHARGEONSHOP+1000))) {
										PPRef->Ot.GetTagStr(PPOBJ_LOT, item.ID, PPTAG_LOT_FSRARINFB, temp_buf);
										if(temp_buf.Empty()) {
											if(PPRef->Ot.GetTagStr(PPOBJ_LOT, item.ID, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0) {
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
											PPRef->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARINFA, ref_a);
											PPRef->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARINFB, ref_b);
											PPRef->Ot.GetTagStr(PPOBJ_LOT, lot_rec.ID, PPTAG_LOT_FSRARLOTGOODSCODE, egais_code);
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
								// @v9.2.11 local_list.clear();
								local_list.add(lot_list.get(i));
								// @v9.2.11 THROW(ep.CreateActChargeOnBill(0, loc_id, rest_date, local_list, 1));
							}
							{
								PPID   new_bill_id = 0;
								BillTbl::Rec new_bill_rec;
								const int cbr = ep.CreateActChargeOnBill(&new_bill_id, selection, loc_id, rest_date, local_list, 1);
								THROW(cbr); // @v9.2.11
								if(cbr > 0 && P_BObj->Search(new_bill_id, &new_bill_rec) > 0) {
									PPObjBill::MakeCodeString(&new_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
									PPMessage(mfInfo, PPINF_ACTCHARGEONCR, temp_buf);
								}
								else {
									PPMessage(mfInfo, PPINF_ACTCHARGEONDONTCR);
								}
							}
						}
						PPWait(0);
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

int SLAPI PPViewLot::Debug()
{
	LotViewItem item;
	SString buf;
	PPWait(1);
	PPLogMessage(PPFILNAM_DEBUG_LOG, PPSTR_TEXT, PPTXT_LOG_LOTPRICEROUNDTEST_BEG, LOGMSGF_TIME|LOGMSGF_USER);
	for(InitIteration(); NextIteration(&item) > 0;) {
		buf.Z().CR().Cat(item.Cost, MKSFMTD(0, 20, 0)).Space().CatCharN('-', 2).Space().Cat(item.Price, MKSFMTD(0, 20, 0));
		PPLogMessage(PPFILNAM_DEBUG_LOG, buf, 0);
		for(int i = 2; i <= 6; i++) {
			double c = round(item.Cost, i);
			double p = round(item.Price, i);
			buf.Z().Cat(i).CatDiv(':', 2).Cat(c, MKSFMTD(0, 20, 0)).
				Space().CatCharN('-', 2).Space().Cat(p, MKSFMTD(0, 20, 0));
			PPLogMessage(PPFILNAM_DEBUG_LOG, buf, 0);
		}
		PPWaitPercent(Counter.Increment());
	}
	PPLogMessage(PPFILNAM_DEBUG_LOG, PPSTR_TEXT, PPTXT_LOG_LOTPRICEROUNDTEST_END, LOGMSGF_TIME|LOGMSGF_USER);
	PPWait(0);
	return -1;
}

int SLAPI PPViewLot::UpdateTempTable(PPID lotID)
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
				MEMSZERO(rec);
				rec.LotID   = lot_rec.ID;
				rec.Dt      = lot_rec.Dt;
				rec.OrgDt   = item.OrgLotDt;
				rec.OprNo   = lot_rec.OprNo;
				rec.GoodsID = lot_rec.GoodsID;
				STRNSCPY(rec.GoodsName, GetGoodsName(lot_rec.GoodsID, temp_buf));
				STRNSCPY(rec.Serial, item.Serial);
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

int SLAPI PPViewLot::InsertTempRecsByIter(BExtInsert * pBei, long * pCounter, UintHashTable * pHt, int showPercentage)
{
	int    ok = 1, r;
	long   nr = DEREFPTRORZ(pCounter);
	PPObjBillStatus bs_obj;
	SString temp_buf;
	TempLotTbl::Rec rec;
	LotViewItem item;
	if(InitIteration(PPViewLot::OrdByDefault) > 0)
		while(NextIteration(&item) > 0) {
			if(!pHt || !pHt->Has(item.ID)) {
				int    skip = 0;
				MEMSZERO(rec);
				rec.LotID   = item.ID;
				rec.Dt      = item.Dt;
				rec.OrgDt   = item.OrgLotDt;
				rec.OprNo   = item.OprNo;
				rec.GoodsID = item.GoodsID;
				// @v9.5.5 GetGoodsName(item.GoodsID, temp_buf);
				GObj.FetchNameR(item.GoodsID, temp_buf); // @v9.5.5
				STRNSCPY(rec.GoodsName, temp_buf);
				STRNSCPY(rec.Serial, item.Serial);
				rec.BegRest   = item.BegRest;
				rec.EndRest   = item.EndRest;
				rec.QttyPlus  = item.QttyPlus;
				rec.QttyMinus = item.QttyMinus;
				if((Filt.Flags & LotFilt::fShowBillStatus) && P_BObj) {
					PPBillStatus bs_rec;
					BillTbl::Rec brec;
					if(P_BObj->Search(item.BillID, &brec) > 0 && brec.StatusID && bs_obj.Fetch(brec.StatusID, &bs_rec) > 0)
						STRNSCPY(rec.BillStatus, bs_rec.Name);
				}
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

					if(Filt.CostDevRestr == LotFilt::drBelow) {
						if(!(rec.SFlags & LOTSF_COSTDOWN))
							skip = 1;
					}
					else if(Filt.CostDevRestr == LotFilt::drAbove) {
						if(!(rec.SFlags & LOTSF_COSTUP))
							skip = 1;
					}
					else if(Filt.CostDevRestr == LotFilt::drAny) {
						if(!(rec.SFlags & LOTSF_COSTDOWN) && !(rec.SFlags & LOTSF_COSTUP))
							skip = 1;
					}
					//
					if(Filt.PriceDevRestr == LotFilt::drBelow) {
						if(!(rec.SFlags & LOTSF_PRICEDOWN))
							skip = 1;
					}
					else if(Filt.PriceDevRestr == LotFilt::drAbove) {
						if(!(rec.SFlags & LOTSF_PRICEUP))
							skip = 1;
					}
					else if(Filt.PriceDevRestr == LotFilt::drAny) {
						if(!(rec.SFlags & LOTSF_PRICEDOWN) && !(rec.SFlags & LOTSF_PRICEUP))
							skip = 1;
					}
				}
				if(!skip) {
					THROW_DB(pBei->insert(&rec));
					if(pHt)
						pHt->Add(item.ID);
					nr++;
				}
			}
			if(showPercentage)
				PPWaitPercent(Counter);
		}
	//THROW_DB(pBei->flash());
	CATCHZOK
	ASSIGN_PTR(pCounter, nr);
	return ok;
}

// @v8.6.6 PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempLot);

int SLAPI PPViewLot::CreateTempTable()
{
	ZDELETE(P_TempTbl);
	int    ok = 1;
	TempLotTbl * p_temp_tbl = 0;
	int    done = 0;
	long   nr = 0;
	THROW(p_temp_tbl = CreateTempFile <TempLotTbl> ());
	{
		const long period_threshould = 180; // @v9.0.0
		BExtInsert bei(p_temp_tbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if((!Filt.GoodsID && Filt.GoodsGrpID) && !SupplList.GetSingle() && Filt.Period.GetLength() > period_threshould) {
			PPIDArray group_goods_list;
			RECORDNUMBER num_goods = 0, grp_count = 0;
			long   grp_calc_threshold = CConfig.GRestCalcThreshold;
			if(grp_calc_threshold <= 0 || grp_calc_threshold > 1000)
				grp_calc_threshold = 100;
			//
			GObj.P_Tbl->getNumRecs(&num_goods);
			GoodsFilt goods_flt;
			goods_flt.GrpID = Filt.GoodsGrpID;
			THROW(GoodsIterator::GetListByFilt(&goods_flt, &group_goods_list));
			grp_count = group_goods_list.getCount();
			if(num_goods && (((1000L * grp_count) / num_goods) < (ulong)grp_calc_threshold)) {
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

int SLAPI PPViewLot::InitIteration(IterOrder order)
{
	int    ok = 1, no_recs = 0;
	DBQ  * dbq = 0;
	BExtQuery::ZDelete(&P_IterQuery);
	Counter.Init();
	if(order == OrdByGoodsName && !P_TempTbl) {
		THROW(CreateTempTable());
	}
	if(P_TempTbl) {
		char   key[MAXKEYLEN], key_[MAXKEYLEN];
		memzero(key, sizeof(key));
		memzero(key_, sizeof(key_));
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
		Counter.Init(P_IterQuery->countIterations(0, key_, spGe));
		P_IterQuery->initIteration(0, key, spGe);
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
		dbq = &(*dbq && realrange(P_Tbl->Cost, Filt.CostRange.low, Filt.CostRange.upp) &&
			realrange(P_Tbl->Price, Filt.PriceRange.low, Filt.PriceRange.upp));
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
				idx          = 3;
				k.k3.Closed  = (Filt.ClosedTag == 1) ? 0 : 1;
				k.k3.GoodsID = q_goods_id;
				k.k3.LocID   = single_loc_id;
				k.k3.Dt      = Filt.Period.low;
				k_ = k;
				if(P_Tbl->search(idx, &k_, spGe)) {
					if(k_.k3.Closed != k.k3.Closed || k_.k3.GoodsID != k.k3.GoodsID || k_.k3.LocID != k.k3.LocID)
						no_recs = 1;
					else if(Filt.Period.upp && k_.k3.Dt > Filt.Period.upp)
						no_recs = 1;
				}
				else {
					THROW_DB(BTROKORNFOUND);
					no_recs = 1;
				}
			}
			else {
				idx          = 7;
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
			idx          = 6;
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
		P_IterQuery->initIteration(0, &k, spGe);
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

int SLAPI PPViewLot::AcceptViewItem(const ReceiptTbl::Rec & rLotRec, LotViewItem * pItem)
{
	int    ok = -1;
	LotViewItem item;
	MEMSZERO(item);
	SString temp_buf;
	if(Filt.Flags & LotFilt::fOrders && Filt.ClosedTag == 1 && rLotRec.Flags & LOTF_CLOSEDORDER)
		return -1;
	// @v8.9.0 {
	if(Filt.Flags & LotFilt::fLotfPrWoTaxes && !(rLotRec.Flags & LOTF_PRICEWOTAXES))
		return -1;
	// } @v8.9.0
	if((State & stFiltSerial) && !Itd.IdBySerialList.lsearch(rLotRec.ID))
		return -1;
	else if(Filt.Flags & LotFilt::fCostAbovePrice && dbl_cmp(rLotRec.Cost, rLotRec.Price) <= 0)
		return -1;
	else if(SupplList.GetCount() > 1 && !SupplList.CheckID(rLotRec.SupplID))
		return -1;
	else if(!Filt.QcExpiryPrd.IsZero()) {
		if(rLotRec.QCertID && QcObj.Search(rLotRec.QCertID) > 0 && !Filt.QcExpiryPrd.CheckDate(QcObj.P_Tbl->data.Expiry))
			return -1;
	}
	if(Filt.Flags & LotFilt::fWithoutClb) {
		int    ret_value = P_BObj->GetClbNumberByLot(rLotRec.ID, 0, temp_buf);
		if(ret_value > 0)
			return -1;
		else if(ret_value < 0) {
			PPID   country_id = 0;
			int    r = GObj.GetManufCountry(rLotRec.GoodsID, 0, &country_id, 0);
			THROW(r);
			if((r == 1 && Itd.NativeCntryList.lsearch(country_id)) || (r == 2 && Itd.PsnNativeCntryList.lsearch(country_id)))
				return -1;
		}
	}
	if(Filt.Flags & LotFilt::fDeadLots && P_BObj->trfr->IsDeadLot(rLotRec.ID) < 0)
		return -1;
	if(Filt.Flags & LotFilt::fOnlySpoilage || Filt.Flags & LotFilt::fShowSerialN) {
		int    skip = 1;
		SpecSeries2Tbl::Rec ss_rec;
		THROW(P_BObj->GetSerialNumberByLot(rLotRec.ID, temp_buf, 1));
		if(!(Filt.Flags & LotFilt::fOnlySpoilage) || P_SpoilTbl->SearchBySerial(SPCSERIK_SPOILAGE, temp_buf, &ss_rec) > 0) {
			skip = 0;
			if(Filt.Flags & LotFilt::fOnlySpoilage) {
				if(ss_rec.GoodsID && labs(ss_rec.GoodsID) != labs(rLotRec.GoodsID))
					skip = 1;
			}
		}
		if(skip)
			return -1;
		else
			temp_buf.CopyTo(item.Serial, sizeof(item.Serial));
	}
	if(Filt.GoodsID || GObj.BelongToGroup(rLotRec.GoodsID, Filt.GoodsGrpID, 0) > 0) {
		*(ReceiptTbl::Rec *)&item = rLotRec;
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
					if(R6(item.BegRest - item.EndRest) == 0.0)
						return -1;
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
				if(is_empty && Filt.Flags & LotFilt::fSkipNoOp)
					return -1;

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
		if(Filt.Flags & LotFilt::fCheckOriginLotDate) {
			ReceiptTbl::Rec org_rec;
			MEMSZERO(org_rec);
			if(item.PrevLotID)
				P_Tbl->SearchOrigin(item.ID, 0, 0, &org_rec);
			else {
				org_rec.ID = item.ID;
				org_rec.Dt = item.Dt;
			}
			if(!org_rec.ID || !Filt.Period.CheckDate(org_rec.Dt))
				return -1;
			else
				item.OrgLotDt = org_rec.Dt;
		}
		if(PPObjTag::CheckForTagFilt(PPOBJ_LOT, rLotRec.ID, Filt.P_TagF) <= 0)
			return -1;
		ASSIGN_PTR(pItem, item);
		ok = 1;
	}
	CATCHZOK
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
					*(ReceiptTbl::Rec *)&item = P_Tbl->data;
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
		if(Itd.IdList.getPointer() < Itd.IdList.getCount()) {
			MEMSZERO(item);
			Counter.Increment();
			PPID   lot_id = Itd.IdList.get(Itd.IdList.incPointer());
			if(P_Tbl->Search(lot_id) > 0) {
				*(ReceiptTbl::Rec *)&item = P_Tbl->data;
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
static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw && pData && pStyle) {
		PPViewLot * p_view = (PPViewLot *)p_brw->P_View;
		const LotFilt * p_filt = (const LotFilt *)p_view->GetBaseFilt();
		PPViewLot::BrwHdr * p_hdr = (PPViewLot::BrwHdr *)pData;
		const long qtty_col = 4;
		const long cost_col = p_filt->Operation.IsZero() ? 7 : 8;
		const long price_col = p_filt->Operation.IsZero() ? 8 : 9;
		if(p_hdr->SFlags && oneof3(col, qtty_col, cost_col, price_col)) {
			if(col == qtty_col && p_hdr->SFlags & LOTSF_FIRST) {
				pStyle->Color = GetColorRef(SClrBlue);
				pStyle->Flags = BrowserWindow::CellStyle::fCorner;
				ok = 1;
			}
			else if(col == cost_col) {
				if(p_hdr->SFlags & LOTSF_COSTUP) {
					pStyle->Color = GetColorRef(SClrGreen);
					pStyle->Flags = BrowserWindow::CellStyle::fCorner;
					ok = 1;
				}
				else if(p_hdr->SFlags & LOTSF_COSTDOWN) {
					pStyle->Color = GetColorRef(SClrRed);
					pStyle->Flags = BrowserWindow::CellStyle::fCorner;
					ok = 1;
				}
			}
			else if(col == price_col) {
				if(p_hdr->SFlags & LOTSF_PRICEUP) {
					pStyle->Color = GetColorRef(SClrGreen);
					pStyle->Flags = BrowserWindow::CellStyle::fCorner;
					ok = 1;
				}
				else if(p_hdr->SFlags & LOTSF_PRICEDOWN) {
					pStyle->Color = GetColorRef(SClrRed);
					pStyle->Flags = BrowserWindow::CellStyle::fCorner;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI PPViewLot::PreprocessBrowser(PPViewBrowser * pBrw)
{
	int   ok = -1;
	if(pBrw) {
		if(Filt.Flags & LotFilt::fOrders) {
			SString word;
			pBrw->LoadToolbar(TOOLBAR_ORDLOTS);
			//PPGetWord(PPWORD_ORDERER, 0, word);
			PPLoadString("orderer", word);
			pBrw->SetColumnTitle(3, word);
			// @v9.1.8 PPGetWord(PPWORD_ORDERED, 0, word);
			PPLoadString("ordered", word); // @v9.1.8
			pBrw->SetColumnTitle(4, word);
			if(Filt.Flags & LotFilt::fShowBillStatus) {
				// @v9.1.11 pBrw->InsColumnWord(-1, PPWORD_STATUS, 15, 0, MKSFMT(10, 0), BCO_CAPLEFT);
				pBrw->InsColumn(-1, "@status", 15, 0, MKSFMT(10, 0), BCO_CAPLEFT); // @v9.1.11
			}
			ok = 1;
		}
		if(Filt.Flags & LotFilt::fShowSerialN) {
			DBQBrowserDef * p_def = (DBQBrowserDef *)pBrw->getDef();
			const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
			if(p_q) {
				uint fld_no = P_TempTbl ? 14 : 12;
				pBrw->InsColumn(-1, "@serial", fld_no, 0, MKSFMT(32, ALIGN_LEFT), BCO_CAPRIGHT);
				ok = 1;
			}
		}
		if(pBrw->SetTempGoodsGrp(Filt.GoodsGrpID) > 0)
			ok = 1;
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
	return ok;
}

DBQuery * SLAPI PPViewLot::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
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
	DBQ  * dbq = 0;
	DBQuery * q = 0;
	if(!P_TempTbl && IsTempTblNeeded())
		THROW(CreateTempTable());
	THROW(CheckTblPtr(rcp = new ReceiptTbl));
	if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1) || (Filt.Flags & LotFilt::fOrders)) // @v9.5.3 || (Filt.Flags & LotFilt::fOrders)
		PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, rcp->SupplID);
	else {
		dbe_ar.init();
		dbe_ar.push((DBFunc)PPDbqFuncPool::IdEmpty); // @v9.5.3
	}
	PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc, rcp->LocID);
	{
		dbe_closedate.init();
		dbe_closedate.push(rcp->CloseDate);
		dbe_closedate.push((DBFunc)PPDbqFuncPool::IdLotCloseDate);
	}
	if(P_TempTbl) {
		THROW(CheckTblPtr(tt = new TempLotTbl(P_TempTbl->GetName())));
		dbq = &(rcp->ID == tt->LotID);

		fld_list[c++].f = rcp->ID;        // #00
		fld_list[c++].f = (Filt.Flags & LotFilt::fCheckOriginLotDate) ? tt->OrgDt : rcp->Dt; // #01
		fld_list[c++].f = tt->SFlags;     // #02
		fld_list[c++].e = dbe_loc;        // #03
		fld_list[c++].f = tt->GoodsName;  // #04
		fld_list[c++].e = dbe_ar;         // #05
		fld_list[c++].f = rcp->Quantity;  // #06
		fld_list[c++].f = rcp->Rest;      // #07
		if(State & stAccsCost)
			fld_list[c++].f = rcp->Cost;  // #08
		else
			fld_list[c++].c.init(0.0);    // #08
		fld_list[c++].f = rcp->Price;     // #09
		fld_list[c++].f = rcp->Expiry;    // #10
		fld_list[c++].e = dbe_closedate;  // #11
		fld_list[c++].f = tt->BegRest;    // #12
		fld_list[c++].f = tt->EndRest;    // #13
		// @v8.4.11 {
		if(Filt.Flags & LotFilt::fShowSerialN) {
			dbe_serial.init();
			dbe_serial.push(dbconst(PPTAG_LOT_SN));
			dbe_serial.push(tt->LotID);
			dbe_serial.push((DBFunc)PPDbqFuncPool::IdObjTagText);
			fld_list[c++].e = dbe_serial; // #14
		}
		else {
			fld_list[c++].c.init((const char *)0); // #14 @stub
		}
		// } @v8.4.11
		// @v8.4.11 fld_list[c++].f = tt->Serial;     // #14
		fld_list[c++].f = tt->BillStatus; // #15

		q = &selectbycell(c, fld_list);
		q->from(tt, rcp, 0L).where(*dbq).orderBy(tt->Dt, tt->OprNo, 0L);
	}
	else {
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, rcp->GoodsID);
		fld_list[c++].f = rcp->ID;        // #00
		fld_list[c++].f = rcp->Dt;        // #01

		fld_list[c++].c.init(0L);         // #02

		fld_list[c++].e = dbe_loc;        // #03
		fld_list[c++].e = dbe_goods;      // #04
		if(P_BObj->CheckRights(BILLOPRT_ACCSSUPPL, 1) || (Filt.Flags & LotFilt::fOrders)) // @v9.5.3 (|| (Filt.Flags & LotFilt::fOrders))
			fld_list[c++].e = dbe_ar;     // #05
		else
			fld_list[c++].c.init((const char *)0); // #05 DBConst
		fld_list[c++].f = rcp->Quantity;  // #06
		fld_list[c++].f = rcp->Rest;      // #07
		if(State & stAccsCost)
			fld_list[c++].f = rcp->Cost;  // #08
		else
			fld_list[c++].c.init(0.0);    // #08
		fld_list[c++].f = rcp->Price;     // #09
		fld_list[c++].f = rcp->Expiry;    // #10
		fld_list[c++].e = dbe_closedate;  // #11
		// @v8.4.11 {
		if(Filt.Flags & LotFilt::fShowSerialN) {
			dbe_serial.init();
			dbe_serial.push(dbconst(PPTAG_LOT_SN));
			dbe_serial.push(rcp->ID);
			dbe_serial.push((DBFunc)PPDbqFuncPool::IdObjTagText);
			fld_list[c++].e = dbe_serial; // #12
		}
		else {
			fld_list[c++].c.init((const char *)0); // #12 @stub
		}
		// } @v8.4.11
		if(Filt.QCertID || (Filt.Flags & LotFilt::fWithoutQCert))
			dbq = &(rcp->QCertID == Filt.QCertID);
		dbq = & (*dbq && daterange(rcp->Dt, &Filt.Period));
		if(oneof2(Filt.ClosedTag, 1, 2))
			dbq = &(*dbq && rcp->Closed == ((Filt.ClosedTag == 1) ? 0L : 1L));
		if(Filt.LocID)
			dbq = ppcheckfiltid(dbq, rcp->LocID, Filt.LocID);
		else if(LocList.getCount())
			dbq = & (*dbq && ppidlist(rcp->LocID, &LocList));
		dbq = ppcheckfiltid(dbq, rcp->SupplID, SupplList.GetSingle());
		dbq = & (*dbq && realrange(rcp->Cost, Filt.CostRange.low, Filt.CostRange.upp) &&
			realrange(rcp->Price, Filt.PriceRange.low, Filt.PriceRange.upp));
		if(Filt.Flags & LotFilt::fCostAbovePrice)
			dbq = & (*dbq && rcp->Cost > rcp->Price);
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
		q = &selectbycell(c, fld_list);
		q->from(rcp, 0L).where(*dbq);
		if(Filt.LocID && Filt.GoodsID && Filt.ClosedTag)
			q->orderBy(rcp->Closed, rcp->GoodsID, rcp->LocID, rcp->Dt, rcp->OprNo, 0L);
		else if(Filt.LocID && Filt.ClosedTag)
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
		int    ord = BIN(Filt.Flags & LotFilt::fOrders);
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
				GetObjectName(PPOBJ_QCERT, Filt.QCertID, sub_title, 1);
			}
		}
		if(sub_title.Empty()) {
			PPFormatPeriod(&Filt.Period, sub_title);
		}
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
int SLAPI PPViewLot::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   lot_id = pHdr ? *(PPID *)pHdr : 0;
	return lot_id ? ::ViewOpersByLot(lot_id, 0) : -1;
}

int SLAPI PPViewLot::Print(const void *)
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
			PView  pf(this);
			ok = PPAlddPrint(rpt_id, &pf, &env);
			Filt.Flags &= ~0x0001;
		}
		else
			ok = -1;
	}
	return ok;
}

int SLAPI PPViewLot::Export()
{
	int    ok = 1, r;
	PPLotExporter l_e;
	THROW(r = l_e.Init(0));
	if(r > 0) {
		PPWait(1);
		LotViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			THROW(l_e.Export(&item));
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}
//
// PPViewLot::ExportGoodsLabelData
//
int SLAPI PPViewLot::ExportGoodsLabelData()
{
	int    ok = 1;
	SString path;
	PPID   goods_id = 0, prev_goods_id = 0;
	LotViewItem lv_item;
	PPObjWorld  w_obj;
	DbfTable   * out_tbl = 0, * out_tblh = 0;
	DbfRecord  * p_tblh_rec = 0;
	SString main_org_name;
	PPWait(1);
	PPGetFilePath(PPPATH_OUT, PPFILNAM_GLABELH_DBF, path);
	THROW(out_tblh = CreateDbfTable(DBFS_RETAILGOODSHDR, path, 1));
	THROW_MEM(p_tblh_rec = new DbfRecord(out_tblh));
	p_tblh_rec->empty();
	p_tblh_rec->put(1, GetMainOrgName(main_org_name));
	if(Filt.LocID) {
		SString loc_name;
		GetLocationName(Filt.LocID, loc_name);
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
				dbfr.empty();
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
	PPWait(0);
	CATCHZOKPPERR
	delete p_tblh_rec;
	delete out_tbl;
	return ok;
}
//
//
//
//int SLAPI EditPrcssrUnifyPriceFiltDialog(PrcssrUnifyPriceFilt *);

int SLAPI PPViewLot::RevalCostByLots()
{
	int    ok = 1;
	PrcssrUnifyPriceFilt param;
	LotViewItem lv_item;
	PrcssrUnifyPrice upb;
	if(param.Setup(1, Filt.LocID, SupplList.GetSingle()) > 0 && upb.EditParam(&param) > 0) {
		PPBillPacket pack;
		PPWait(1);
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
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}
//
// Global standalone functions
//
int SLAPI ViewLots(PPID goods, PPID loc, PPID suppl, PPID qcert, int modeless)
{
	LotFilt flt;
	flt.GoodsID = goods;
	flt.LocID   = loc;
	flt.SupplID = suppl;
	flt.QCertID = qcert;
	return ViewLots(&flt, 0, modeless);
}

int SLAPI ViewLots(const LotFilt * pFilt, int asOrders, int asModeless)
{
	return PPView::Execute(PPVIEW_LOT, pFilt, asModeless, (void *)BIN(asOrders));
}
//
// Implementation of PPALDD_Lot
//
PPALDD_CONSTRUCTOR(Lot)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(Lot)
{
	Destroy();
}

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
			H.SupplID = p_bobj->CheckRights(BILLOPRT_ACCSSUPPL, 1) ? rec.SupplID : 0; // @v7.2.6 CheckRights
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
			H.Cost    = p_bobj->CheckRights(BILLRT_ACCSCOST) ? rec.Cost : 0.0; // @v7.2.6 CheckRights
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
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _ARG_LONG(n) (*(long *)rS.GetPtr(pApl->Get(n)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))

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
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Lots)
{
	Destroy();
}

int PPALDD_Lots::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Lot, rsrv);
	H.FltLocID      = p_filt->LocID;
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
	PPViewLot * p_v = (PPViewLot *)(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	return BIN(p_v->InitIteration((PPViewLot::IterOrder)SortIdx));
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

void PPALDD_Lots::Destroy()
{
	DESTROY_PPVIEW_ALDD(Lot);
}
//
//
//
#define USE_IMPL_DL6ICLS_PPFiltLot
#define USE_IMPL_DL6ICLS_PPViewLot
#include "..\rsrc\dl600\ppifc_auto.cpp"

DL6_IC_CONSTRUCTION_EXTRA(PPFiltLot, DL6ICLS_PPFiltLot_VTab, LotFilt);
//
// Interface IPpyFilt_Lot implementation
//
void DL6ICLS_PPFiltLot::SetPeriod(LDATE low, LDATE upp)
	{ ((LotFilt *)ExtraPtr)->Period.Set(low, upp); }
void DL6ICLS_PPFiltLot::SetOperationPeriod(LDATE low, LDATE upp)
	{ ((LotFilt *)ExtraPtr)->Operation.Set(low, upp); }
void DL6ICLS_PPFiltLot::SetExpiryPeriod(LDATE low, LDATE upp)
	{ ((LotFilt *)ExtraPtr)->ExpiryPrd.Set(low, upp); }
void DL6ICLS_PPFiltLot::SetQcExpiryPeriod(LDATE low, LDATE upp)
	{ ((LotFilt *)ExtraPtr)->QcExpiryPrd.Set(low, upp); }
SDateRange DL6ICLS_PPFiltLot::get_Period()
	{ return DateRangeToOleDateRange(((LotFilt *)ExtraPtr)->Period); }
SDateRange DL6ICLS_PPFiltLot::get_OperationPeriod()
	{ return DateRangeToOleDateRange(((LotFilt *)ExtraPtr)->Operation); }
SDateRange DL6ICLS_PPFiltLot::get_ExpiryPeriod()
	{ return DateRangeToOleDateRange(((LotFilt *)ExtraPtr)->ExpiryPrd); }
SDateRange DL6ICLS_PPFiltLot::get_QcExpiryPeriod()
	{ return DateRangeToOleDateRange(((LotFilt *)ExtraPtr)->QcExpiryPrd); }

int32 DL6ICLS_PPFiltLot::get_LocID()                 { IMPL_PPIFC_GETPROP(LotFilt, LocID); }
void  DL6ICLS_PPFiltLot::put_LocID(int32 value)      { IMPL_PPIFC_PUTPROP(LotFilt, LocID); }
int32 DL6ICLS_PPFiltLot::get_SupplID()               { IMPL_PPIFC_GETPROP(LotFilt, SupplID); }
void  DL6ICLS_PPFiltLot::put_SupplID(int32 value)    { IMPL_PPIFC_PUTPROP(LotFilt, SupplID); }
int32 DL6ICLS_PPFiltLot::get_GoodsGrpID()            { IMPL_PPIFC_GETPROP(LotFilt, GoodsGrpID); }
void  DL6ICLS_PPFiltLot::put_GoodsGrpID(int32 value) { IMPL_PPIFC_PUTPROP(LotFilt, GoodsGrpID); }
int32 DL6ICLS_PPFiltLot::get_GoodsID()               { IMPL_PPIFC_GETPROP(LotFilt, GoodsID); }
void  DL6ICLS_PPFiltLot::put_GoodsID(int32 value)    { IMPL_PPIFC_PUTPROP(LotFilt, GoodsID); }
int32 DL6ICLS_PPFiltLot::get_QCertID()               { IMPL_PPIFC_GETPROP(LotFilt, QCertID); }
void  DL6ICLS_PPFiltLot::put_QCertID(int32 value)    { IMPL_PPIFC_PUTPROP(LotFilt, QCertID); }
int32 DL6ICLS_PPFiltLot::get_InTaxGrpID()            { IMPL_PPIFC_GETPROP(LotFilt, InTaxGrpID); }
void  DL6ICLS_PPFiltLot::put_InTaxGrpID(int32 value) { IMPL_PPIFC_PUTPROP(LotFilt, InTaxGrpID); }

PpyVLotFlags DL6ICLS_PPFiltLot::get_Flags()           { IMPL_PPIFC_GETPROP_CAST(LotFilt, Flags, PpyVLotFlags); }
void DL6ICLS_PPFiltLot::put_Flags(PpyVLotFlags value) { IMPL_PPIFC_PUTPROP(LotFilt, Flags); }
uint32 DL6ICLS_PPFiltLot::get_ClosedTag()             { IMPL_PPIFC_GETPROP(LotFilt, ClosedTag); }
void DL6ICLS_PPFiltLot::put_ClosedTag(uint32 value)   { IMPL_PPIFC_PUTPROP(LotFilt, ClosedTag); }

double DL6ICLS_PPFiltLot::get_CostLow()              { return ((LotFilt *)ExtraPtr)->CostRange.low; }
void   DL6ICLS_PPFiltLot::put_CostLow(double value)  { ((LotFilt *)ExtraPtr)->CostRange.low = value; }
double DL6ICLS_PPFiltLot::get_CostUpp()              { return ((LotFilt *)ExtraPtr)->CostRange.upp; }
void   DL6ICLS_PPFiltLot::put_CostUpp(double value)  { ((LotFilt *)ExtraPtr)->CostRange.upp = value; }
double DL6ICLS_PPFiltLot::get_PriceLow()             { return ((LotFilt *)ExtraPtr)->PriceRange.low; }
void   DL6ICLS_PPFiltLot::put_PriceLow(double value) { ((LotFilt *)ExtraPtr)->PriceRange.low = value; }
double DL6ICLS_PPFiltLot::get_PriceUpp()             { return ((LotFilt *)ExtraPtr)->PriceRange.upp; }
void   DL6ICLS_PPFiltLot::put_PriceUpp(double value) { ((LotFilt *)ExtraPtr)->PriceRange.upp = value; }

SString & DL6ICLS_PPFiltLot::get_Serial()
{
	((LotFilt *)ExtraPtr)->GetExtssData(LotFilt::extssSerialText, RetStrBuf);
	return RetStrBuf;
}

void DL6ICLS_PPFiltLot::put_Serial(SString & value)
{
	((LotFilt *)ExtraPtr)->PutExtssData(LotFilt::extssSerialText, value);
}
//
//
//
DL6_IC_CONSTRUCTION_EXTRA(PPViewLot, DL6ICLS_PPViewLot_VTab, PPViewLot);
//
// Interface IPapyrusView implementation
//
IUnknown* DL6ICLS_PPViewLot::CreateFilt(int32 param)
{
	IUnknown * p_filt = 0;
	return CreateInnerInstance("PPFiltLot", 0, (void **)&p_filt) ? p_filt : (AppError = 1, 0);
}

int32 DL6ICLS_PPViewLot::Init(IUnknown* pFilt)
{
	IMPL_PPIFC_PPVIEWINIT(Lot);
}

int32 DL6ICLS_PPViewLot::InitIteration(int32 order)
{
	return ((PPViewLot*)ExtraPtr)->InitIteration((PPViewLot::IterOrder)order);
}

int32 DL6ICLS_PPViewLot::NextIteration(PPYVIEWITEM item)
{
	int    ok = -1;
	SPpyVI_Lot * p_item = (SPpyVI_Lot *)item;
	LotViewItem inner_item;
	if(((PPViewLot *)ExtraPtr)->NextIteration(&inner_item) > 0) {
		SString temp_buf;
		p_item->RecTag = PPVIEWITEM_LOT;
		#define FLD(f) p_item->f = inner_item.f
		FLD(ID);
		FLD(BillID);
		FLD(LocID);
		p_item->Dt = (OleDate)inner_item.Dt;
		FLD(OprNo);
		FLD(Closed);
		FLD(GoodsID);
		FLD(QCertID);
		FLD(UnitPerPack);
		FLD(Quantity);
		FLD(WtQtty);
		FLD(WtRest);
		FLD(Cost);
		FLD(ExtCost);
		FLD(Price);
		FLD(Rest);
		FLD(PrevLotID);
		FLD(SupplID);
		p_item->CloseDate = (OleDate)inner_item.CloseDate;
		p_item->Expiry    = (OleDate)inner_item.Expiry;
		FLD(InTaxGrpID);
		FLD(Flags);
		FLD(BegRest);
		FLD(EndRest);
		FLD(QttyPlus);
		FLD(QttyMinus);
		p_item->OrgLotDt = (OleDate)inner_item.OrgLotDt;
		(temp_buf = inner_item.Serial).CopyToOleStr(&p_item->Serial);
		#undef FLD
		ok = 1;
	}
	return ok;
}

SIterCounter DL6ICLS_PPViewLot::GetIterCounter()
{
	return GetPPViewIterCounter(ExtraPtr, &AppError);
}

int32 DL6ICLS_PPViewLot::GetTotal(PPYVIEWTOTAL total)
{
	PPViewLot * p_v = (PPViewLot *)ExtraPtr;
	if(p_v && total) {
		LotTotal inner_total;
		SPpyVT_Lot * p_total = (SPpyVT_Lot *)total;
		if(p_v->CalcTotal(LotTotal::Extended, &inner_total)) {
			p_total->RecTag = 0;
			#define FLD(f) p_total->f = inner_total.f
			FLD(Count);
			FLD(Qtty);
			FLD(Rest);
			FLD(Cost);
			FLD(Price);
			FLD(DCount);
			FLD(InCost);
			FLD(InPrice);
			FLD(DRest);
			FLD(DCost);
			FLD(DPrice);
		}
		else
			AppError = 1;
	}
	return !AppError;
}
//
// Import/Export
//
IMPLEMENT_IMPEXP_HDL_FACTORY(LOT, PPLotImpExpParam);

SLAPI PPLotImpExpParam::PPLotImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
	Flags = 0;
	UhttGoodsCodeArID = 0;
}

//virtual
int PPLotImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
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

//virtual
int PPLotImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
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

//virtual
int PPLotImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
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
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI EditLotImpExpParam(const char * pIniSection)
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

int SLAPI SelectLotImpExpCfgs(PPLotImpExpParam * pParam, int import)
{
	int    ok = -1, valid_data = 0;
	uint   p = 0;
	long   id = 0;
	SString ini_file_name;
	StrAssocArray list;
	PPLotImpExpParam param;
	TDialog * p_dlg = 0;
	THROW_INVARG(pParam);
	pParam->Direction = BIN(import);
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_LOT, &param, &list, import ? 2 : 1));
	id = (list.SearchByText(pParam->Name, 1, &p) > 0) ? (uint)list.at(p).Id : 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		SString sect;
		//В режиме тестирования - начало
		#if SLTEST_RUNNING
			for(int i = 1; i < (int)list.getCount(); i++) {
				list.Get(i, sect);
				if(strstr(sect, pParam->Name)) {
					pParam->ProcessName(1, sect);
					pParam->ReadIni(&ini_file, sect, 0);
					ok = 1;
					return ok;
				}
			}
		#endif
		// конец
		while(!valid_data && ListBoxSelDialog(&list, import ? PPTXT_TITLE_LOTIMPCFG : PPTXT_TITLE_LOTEXPCFG, &id, 0) > 0) {
			SString sect;
			if(id) {
				list.Get(id, sect);
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

SLAPI PPLotExporter::PPLotExporter() : Param(0, 0)
{
	P_IE = 0;
}

SLAPI PPLotExporter::~PPLotExporter()
{
	ZDELETE(P_IE);
}

int SLAPI PPLotExporter::Init(const PPLotImpExpParam * pParam)
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

int SLAPI PPLotExporter::Export(const LotViewItem * pItem)
{
	int    ok = 1;
	SString temp_buf;
	Sdr_Lot  sdr_lot;
	MEMSZERO(sdr_lot);
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
	temp_buf = pItem->Serial;
	BillObj->ReleaseSerialFromUniqSuffix(temp_buf);
	temp_buf.CopyTo(sdr_lot.Serial, sizeof(sdr_lot.Serial));
	if(UhttCli.HasAccount() && Param.UhttGoodsCodeArID) {
		ArticleTbl::Rec ar_rec;
		if(ArObj.Fetch(Param.UhttGoodsCodeArID, &ar_rec) > 0) {
			PPID   acs_id = 0;
			PPID   psn_id = ObjectToPerson(ar_rec.ID, &acs_id);
			if(psn_id) {
				SString inn;
				PsnObj.GetRegNumber(psn_id, PPREGT_TPID, inn);
				if(inn.NotEmptyS()) {
					const int _cd = BIN(GObj.GetConfig().Flags & GCF_BCCHKDIG);
					SString org_code, adj_code;
					BarcodeArray bc_list;
					GObj.ReadBarcodes(pItem->GoodsID, bc_list);
					if(bc_list.getCount()) {
						for(uint i = 0; i < bc_list.getCount(); i++) {
							if(UhttCli.IsAuth() || UhttCli.Auth()) {
								org_code = bc_list.at(i).Code;
								if(UhttCli.GetGoodsArCode(org_code, inn, temp_buf) && temp_buf.NotEmptyS()) {
									temp_buf.CopyTo(sdr_lot.UhttArCode, sizeof(sdr_lot.UhttArCode));
									break;
								}
								// @v8.3.11 {
								else if(!_cd && UhttCli.GetGoodsArCode(AddBarcodeCheckDigit(adj_code = org_code), inn, temp_buf) && temp_buf.NotEmptyS()) {
									temp_buf.CopyTo(sdr_lot.UhttArCode, sizeof(sdr_lot.UhttArCode));
									break;
								}
								// } @v8.3.11
								else {
									int d = 0, std = 0;
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
	Param.InrRec.ConvertDataFields(CTRANSF_INNER_TO_OUTER, &sdr_lot);
	THROW(P_IE->AppendRecord(&sdr_lot, sizeof(sdr_lot)));
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI EditLotExtCode(LotExtCodeTbl::Rec & rRec, char firstChar)
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
		TInputLine * il = (TInputLine*)dlg->getCtrlView(CTL_LOTEXTCODE_CODE);
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

IMPLEMENT_PPFILT_FACTORY(LotExtCode); SLAPI LotExtCodeFilt::LotExtCodeFilt() : PPBaseFilt(PPFILT_LOTEXTCODE, 0, 1)
{
	SetFlatChunk(offsetof(LotExtCodeFilt, ReserveStart),
		offsetof(LotExtCodeFilt, Reserve)-offsetof(LotExtCodeFilt, ReserveStart)+sizeof(Reserve));
	SetBranchSString(offsetof(LotExtCodeFilt, SrchStr));
	Init(1, 0);
}

SLAPI PPViewLotExtCode::PPViewLotExtCode() : PPView(0, &Filt, PPVIEW_LOTEXTCODE)
{
	P_BObj = BillObj;
	ImplementFlags |= implDontEditNullFilter;
}

SLAPI PPViewLotExtCode::~PPViewLotExtCode()
{
}

int SLAPI PPViewLotExtCode::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	CATCHZOK
	return ok;
}

int SLAPI PPViewLotExtCode::InitIteration()
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
	P_IterQuery->initIteration(0, &k0, spGe);
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

PPBaseFilt * SLAPI PPViewLotExtCode::CreateFilt(void * extraPtr) const
{
	LotExtCodeFilt * p_filt = new LotExtCodeFilt;
	if(p_filt) {
		p_filt->LotID = ((long)extraPtr);
	}
	return p_filt;
}

int SLAPI PPViewLotExtCode::EditBaseFilt(PPBaseFilt * pFilt)
{
	return -1;
}

int SLAPI PPViewLotExtCode::GetRec(const void * pHdr, LotExtCodeTbl::Rec & rRec)
{
	int    ok = 0;
	if(pHdr) {
		LotExtCodeTbl::Key0 k0;
		k0.LotID = *(long *)pHdr;
		STRNSCPY(k0.Code, (const char *)(PTR8(pHdr)+sizeof(long)));
		if(Tbl.search(0, &k0, spEq)) {
			Tbl.copyBufTo(&rRec);
			ok = 1;
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewLotExtCode::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
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

int SLAPI PPViewLotExtCode::CheckDupCode(const LotExtCodeTbl::Rec & rRec)
{
	int    ok = 1;
	LotExtCodeTbl::Key1 k1;
	MEMSZERO(k1);
	STRNSCPY(k1.Code, rRec.Code);
	if(Tbl.search(1, &k1, spEq)) {
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

int SLAPI PPViewLotExtCode::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_LOTEXTCTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long count = 0;
		LotExtCodeViewItem item;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWait(0);
		dlg->setCtrlLong(CTL_LOTEXTCTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
		return -1;
	}
	else
		return 0;
}

int SLAPI PPViewLotExtCode::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   lot_id = pHdr ? *(PPID *)pHdr : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				if(Filt.LotID) {
					LotExtCodeTbl::Rec rec;
					MEMSZERO(rec);
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
					char c = *(const char *)pHdr;
					const char uc = toupper(c);
					if(isdec(c) || (uc >= 'A' && uc <= 'Z')) {
						LotExtCodeTbl::Rec rec;
						MEMSZERO(rec);
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
						if(EditLotExtCode(rec, 0) && strcmp(rec.Code, org_rec.Code) != 0) {
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
