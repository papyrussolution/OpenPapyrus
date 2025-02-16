// INVENTRY.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2024, 2025
// @codepage UTF-8
//
// Инвентаризация //
//
#include <pp.h>
#pragma hdrstop
//
//
//
class InventoryDialog : public TDialog {
public:
	InventoryDialog(uint rezID, PPObjBill * pBObj, PPBillPacket *);
	int    getDTS(PPBillPacket *);
private:
	DECL_HANDLE_EVENT;
	void   editItems(int filtered);
	void   writeOff();
	void   rollbackWritingOff();
	int    showLinkFilesList();
	void   setupHiddenButton(long opflag, uint cm, uint ctlId);

	enum {
		fSetupObj2ByCliAgt = 0x0020
	};

	PPObjBill * P_BObj;
	PPBillPacket * P_Data;
	InventoryFilt InvFilt;
	ushort selExistsGoodsOnly;
	uint16 Reserve; // @alignment
	long   Flags;
	TRect  DefaultRect;
};

InventoryDialog::InventoryDialog(uint rezID, PPObjBill * pBObj, PPBillPacket * pData) : TDialog(rezID), P_BObj(pBObj), Flags(0)
{
	int    dsbl_object2 = 0;
	SString temp_buf;
	PPObjArticle ar_obj;
	PPObjOprKind op_obj;
	PPOprKindPacket op_pack;
	op_obj.GetPacket(pData->Rec.OpID, &op_pack);
	selExistsGoodsOnly = 0;
	P_Data = pData;
	setCtrlData(CTL_BILL_DOC,    P_Data->Rec.Code);
	setCtrlData(CTL_BILL_DATE,   &P_Data->Rec.Dt);
	setCtrlData(CTL_BILL_DUEDATE, &P_Data->Rec.DueDate);
	SetupArCombo(this, CTLSEL_BILL_OBJECT, P_Data->Rec.Object, OLW_LOADDEFONOPEN, op_pack.Rec.AccSheetID, sacfDisableIfZeroSheet);
	{
		if(op_pack.Rec.AccSheet2ID) {
			PPClientAgreement ca_rec;
			SETFLAG(Flags, fSetupObj2ByCliAgt, ar_obj.GetClientAgreement(0, ca_rec) > 0 && ca_rec.ExtObjectID == op_pack.Rec.AccSheet2ID);
			SetupArCombo(this, CTLSEL_BILL_OBJ2, P_Data->Rec.Object2, /*OLW_LOADDEFONOPEN|*/OLW_CANINSERT, op_pack.Rec.AccSheet2ID, 0);
			op_pack.GetExtStrData(OPKEXSTR_OBJ2NAME, temp_buf);
			setStaticText(CTL_BILL_OBJ2NAME, temp_buf);
		}
		else
			dsbl_object2 = 1;
		disableCtrls(dsbl_object2, CTL_BILL_OBJ2, CTLSEL_BILL_OBJ2, CTL_BILL_OBJ2NAME, 0);
		showCtrl(CTL_BILL_OBJ2,     !dsbl_object2);
		showCtrl(CTLSEL_BILL_OBJ2,  !dsbl_object2);
		showCtrl(CTL_BILL_OBJ2NAME, !dsbl_object2);
	}
	{
		const  PPID strg_loc_id = P_Data->P_Freight ? P_Data->P_Freight->StorageLocID : 0;
		LocationFilt loc_filt(LOCTYP_WAREPLACE, 0, P_Data->Rec.LocID);
		SetupLocationCombo(this, CTLSEL_BILL_STRGLOC, strg_loc_id, OLW_CANSELUPLEVEL, &loc_filt);
	}
	// @v11.1.12 setCtrlData(CTL_BILL_MEMO,   P_Data->Rec.Memo);
	setCtrlString(CTL_BILL_MEMO,   P_Data->SMemo); // @v11.1.12
	setCtrlReal(CTL_BILL_AMOUNT, P_Data->GetAmount());
	setCtrlData(CTL_BILL_SELEXSGOODS, &selExistsGoodsOnly);
	disableCtrls(1, CTL_BILL_AMOUNT, CTL_BILL_STOCKAMT, 0);
	SetupCalDate(CTLCAL_BILL_DATE, CTL_BILL_DATE);
	SetupCalDate(CTLCAL_BILL_DUEDATE, CTL_BILL_DUEDATE);

	PPInventoryOpEx  inv_op_ex;
	P_BObj->P_OpObj->FetchInventoryData(P_Data->Rec.OpID, &inv_op_ex);
	int    substr_idx = (inv_op_ex.Flags & INVOPF_ZERODEFAULT) ? 0 : 1;
	PPGetSubStr(PPTXT_INVENTEXTINFO, substr_idx, temp_buf);
	setStaticText(CTL_BILL_EXTINFO, temp_buf);
	{
		temp_buf.Z().Cat(op_pack.Rec.Name).CatDiv(';', 1);
		CatObjectName(PPOBJ_LOCATION, P_Data->Rec.LocID, temp_buf);
		setTitle(temp_buf);
	}
	{
		GetObjectName(PPOBJ_BILLSTATUS, P_Data->Rec.StatusID, temp_buf);
		setStaticText(CTL_BILL_STATUS, temp_buf);
	}
	enableCommand(cmInvWriteOff, !(inv_op_ex.Flags & INVOPF_INVBYCLIENT));
	enableCommand(cmInvRollback, !(inv_op_ex.Flags & INVOPF_INVBYCLIENT));
	//
	setupPosition();
	DefaultRect = getRect();
	showLinkFilesList();
	//
	setupHiddenButton(OPKF_ATTACHFILES, cmLinkFiles, CTL_BILL_LINKFILESBUTTON);
}

void InventoryDialog::setupHiddenButton(long opflag, uint cm, uint ctlId)
{
	const int s = BIN(CheckOpFlags(P_Data->Rec.OpID, opflag));
	enableCommand(cm, s);
	showCtrl(ctlId, s);
}

int InventoryDialog::getDTS(PPBillPacket * /*_data*/)
{
	getCtrlData(CTL_BILL_DOC,  P_Data->Rec.Code);
	getCtrlData(CTL_BILL_DATE, &P_Data->Rec.Dt);
	getCtrlData(CTL_BILL_DUEDATE, &P_Data->Rec.DueDate);
	getCtrlData(CTL_BILL_AMOUNT, &P_Data->Rec.Amount);
	getCtrlData(CTLSEL_BILL_OBJECT, &P_Data->Rec.Object);
	getCtrlData(CTLSEL_BILL_OBJ2,   &P_Data->Rec.Object2);
	{
		PPFreight freight;
		RVALUEPTR(freight, P_Data->P_Freight);
		getCtrlData(CTLSEL_BILL_STRGLOC, &freight.StorageLocID);
		P_Data->SetFreight(&freight);
	}
	// @v11.1.12 getCtrlData(CTL_BILL_MEMO, P_Data->Rec.Memo);
	getCtrlString(CTL_BILL_MEMO, P_Data->SMemo); // @v11.1.12
	return 1;
}

void InventoryDialog::editItems(int filtered)
{
	int    ok = -1;
	InventoryFilt temp;
	InventoryFilt * p_filt = filtered ? &InvFilt : &temp;
	ushort v = 0;
	getDTS(0);
	getCtrlData(CTL_BILL_SELEXSGOODS, &v);
	PPViewInventory * p_v = new PPViewInventory;
	THROW_MEM(p_v);
	if(P_Data->ProcessFlags & PPBillPacket::pfZombie) {
		p_v->SetOuterPack(P_Data);
		filtered = 0;
	}
	else {
		THROW(P_BObj->TurnInventory(P_Data, 1));
		p_filt->SetSingleBillID(P_Data->Rec.ID);
	}
	if(!filtered || p_v->EditBaseFilt(p_filt) > 0) {
		SETFLAG(p_filt->Flags, InventoryFilt::fSelExistsGoodsOnly, v);
		THROW(p_v->Init_(p_filt));
		THROW(p_v->Browse(0));
		if(!(P_Data->ProcessFlags & PPBillPacket::pfZombie) && p_v->GetUpdateStatus())
			THROW(p_v->UpdatePacket(P_Data->Rec.ID));
		{
			BillTbl::Rec bill_rec;
			const double amount = (P_BObj->Search(P_Data->Rec.ID, &bill_rec) > 0) ? bill_rec.Amount : 0.0;
			setCtrlReal(CTL_BILL_AMOUNT, amount);
		}
		ok = 1;
	}
	CATCHZOKPPERR
	delete p_v;
}

#define RESIZE_DELTA 50L

int InventoryDialog::showLinkFilesList()
{
	HWND   list = ::GetDlgItem(H(), CTL_BILL_LNKFILELIST);
	if(list) {
		const int show_cmd = (!P_Data || !P_Data->LnkFiles.getCount()) ? SW_HIDE : SW_NORMAL;
		TRect  rect = DefaultRect;
		if(show_cmd == SW_NORMAL)
			rect.b.y += RESIZE_DELTA;
		::MoveWindow(H(), rect.a.x, rect.a.y, rect.width(), rect.height(), 1);
		if(show_cmd == SW_NORMAL) {
			rect = getClientRect();
			::MoveWindow(list, rect.a.x, rect.b.y - RESIZE_DELTA, rect.width(), RESIZE_DELTA, 1);
			//updateList(-1);
		}
		::ShowWindow(list, show_cmd);
		setupPosition();
	}
	return 1;
}

void InventoryDialog::writeOff()
{
	if(PPMessage(mfConf|mfYesNo, PPCFM_INVWRITEOFF) == cmYes) {
		PPWaitStart();
		if(!P_BObj->ConvertInventory(P_Data->Rec.ID))
			PPError();
		PPWaitStop();
	}
}

void InventoryDialog::rollbackWritingOff()
{
	PPWaitStart();
	if(!P_BObj->RollbackInventoryWrOff(P_Data->Rec.ID))
		PPError();
	PPWaitStop();
}

IMPL_HANDLE_EVENT(InventoryDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND)
		switch(TVCMD) {
			case cmDetail:         editItems(0); break;
			case cmFilteredDetail: editItems(1); break;
			case cmInvWriteOff:    writeOff();   break;
			case cmInvRollback:    rollbackWritingOff(); break;
			case cmTags: // @erik
				P_Data->BTagL.Oid.Obj = PPOBJ_BILL;
				EditObjTagValList(&P_Data->BTagL, 0);
				break;
			default: return;
		}
	else if(TVKEYDOWN) {
		if(TVKEY == KB_CTRLENTER)
			editItems(0);
		else if(TVKEY == kbF2) {
			if(isCurrCtlID(CTL_BILL_DOC))
				P_BObj->UpdateOpCounter(P_Data);
		}
	}
	else
		return;
	clearEvent(event);
}
//
// InventoryOptionsDialog
//
class InventoryOptionsDialog : public TDialog {
	DECL_DIALOG_DATA(PPInventoryOpEx);
public:
	InventoryOptionsDialog() : TDialog(DLG_OPKINVE)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		ushort v;
		RVALUEPTR(Data, pData);
		SetupPPObjCombo(this, CTLSEL_OPKINVE_WRDNOP, PPOBJ_OPRKIND, Data.WrDnOp, 0, reinterpret_cast<void *>(PPOPT_GOODSEXPEND));
		setupAccSheet(CTLSEL_OPKINVE_WRDNOP, CTLSEL_OPKINVE_WRDNOBJ, Data.WrDnObj);
		SetupPPObjCombo(this, CTLSEL_OPKINVE_WRUPOP, PPOBJ_OPRKIND, Data.WrUpOp, 0, reinterpret_cast<void *>(PPOPT_GOODSRECEIPT));
		setupAccSheet(CTLSEL_OPKINVE_WRUPOP, CTLSEL_OPKINVE_WRUPOBJ, Data.WrUpObj);
		SetupPPObjCombo(this, CTLSEL_OPKINVE_ONWROFFST, PPOBJ_BILLSTATUS, Data.OnWrOffStatusID, 0); // @v10.5.9
		//setCtrlData(CTL_OPKINVE_NOMINAL,    &(v = Data.Nominal));
		//setCtrlData(CTL_OPKINVE_DEFREST,    &(v = Data.DefaultRest));
		setCtrlData(CTL_OPKINVE_AUTOMETHOD, &(v = Data.AutoFillMethod));
		setCtrlData(CTL_OPKINVE_CALCPRICE,  &(v = Data.AmountCalcMethod));
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 0, INVOPF_COSTNOMINAL);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 1, INVOPF_ZERODEFAULT);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 2, INVOPF_WROFFWODSCNT);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 3, INVOPF_USEPACKS);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 4, INVOPF_SELGOODSBYNAME);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 5, INVOPF_USEANOTERLOCLOTS);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 6, INVOPF_INVBYCLIENT);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 7, INVOPF_ASSET);
		AddClusterAssoc(CTL_OPKINVE_FLAGS, 8, INVOPF_USESERIAL);
		SetClusterData(CTL_OPKINVE_FLAGS, Data.Flags);
		{
			AddClusterAssocDef(CTL_OPKINVE_ACCSLMODE, 0, Data.accsliNo);
			AddClusterAssoc(CTL_OPKINVE_ACCSLMODE, 1, Data.accsliCode);
			AddClusterAssoc(CTL_OPKINVE_ACCSLMODE, 2, Data.accsliCodeAndQtty);
			SetClusterData(CTL_OPKINVE_ACCSLMODE, Data.GetAccelInputMode());
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlData(CTLSEL_OPKINVE_WRDNOP,  &Data.WrDnOp);
		getCtrlData(CTLSEL_OPKINVE_WRDNOBJ, &Data.WrDnObj);
		getCtrlData(CTLSEL_OPKINVE_WRUPOP,  &Data.WrUpOp);
		getCtrlData(CTLSEL_OPKINVE_WRUPOBJ, &Data.WrUpObj);
		getCtrlData(CTLSEL_OPKINVE_ONWROFFST, &Data.OnWrOffStatusID); // @v10.5.9
		getCtrlData(CTL_OPKINVE_AUTOMETHOD, &Data.AutoFillMethod);
		getCtrlData(CTL_OPKINVE_CALCPRICE,  &Data.AmountCalcMethod);
		GetClusterData(CTL_OPKINVE_FLAGS, &Data.Flags);
		Data.SetAccelInputMode(GetClusterData(CTL_OPKINVE_ACCSLMODE));
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID);
};

void InventoryOptionsDialog::setupAccSheet(uint opSelCtl, uint objSelCtl, PPID arID)
{
	PPOprKind op_rec;
	const  PPID op_id = getCtrlLong(opSelCtl);
	GetOpData(op_id, &op_rec);
	SetupArCombo(this, objSelCtl, arID, OLW_LOADDEFONOPEN | OLW_CANINSERT, op_rec.AccSheetID, sacfDisableIfZeroSheet);
}

IMPL_HANDLE_EVENT(InventoryOptionsDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_OPKINVE_WRDNOP))
		setupAccSheet(CTLSEL_OPKINVE_WRDNOP, CTLSEL_OPKINVE_WRDNOBJ, 0);
	else if(event.isCbSelected(CTLSEL_OPKINVE_WRUPOP))
		setupAccSheet(CTLSEL_OPKINVE_WRUPOP, CTLSEL_OPKINVE_WRUPOBJ, 0);
	else
		return;
	clearEvent(event);
}

int EditInventoryOptionsDialog(PPInventoryOpEx * pData) { DIALOG_PROC_BODY(InventoryOptionsDialog, pData); }
//
// PPObjBill inventory methods
//
void PPObjBill::InvItem::Init(PPID goodsID, const char * pSerial)
{
	THISZERO();
	GoodsID = goodsID;
	STRNSCPY(Serial, pSerial);
}

PPObjBill::InvBlock::InvBlock(long flags) : State(0), Flags(flags)
{
	// @v10.6.10 THISZERO();
	// @v10.6.10 Flags = flags;
}

int PPObjBill::InitInventoryBlock(PPID billID, InvBlock & rBlk)
{
	int    ok = 1;
	THROW(Fetch(billID, &rBlk.BillRec) > 0);
	THROW(P_OpObj->FetchInventoryData(rBlk.BillRec.OpID, &rBlk.Ioe));
	rBlk.State |= InvBlock::stInited;
	CATCHZOK
	return ok;
}

int PPObjBill::AcceptInventoryItem(const InvBlock & rBlk, InvItem * pItem, int use_ta)
{
	int    ok = 1;
	int    skip = 0;
	long   oprno = 0;
	SString fmt_buf, log_msg;
	ReceiptTbl::Rec lot_rec;
	GoodsRestParam p;
	InventoryTbl::Rec inv_rec, sg_rec;
	THROW(GetInventoryStockRest(rBlk, pItem, &p));
	if(rBlk.Flags & InvBlock::fPriceByLastLot &&
		trfr->Rcpt.GetGoodsPrice(pItem->GoodsID, rBlk.BillRec.LocID, rBlk.BillRec.Dt, GPRET_INDEF, 0, &lot_rec) > 0)
		pItem->FinalPrice = R5((rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) ? lot_rec.Cost : lot_rec.Price);
	else if(!(rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) && pItem->Price > 0.0)
		pItem->FinalPrice = pItem->Price;
	else if((rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) && pItem->Cost > 0.0)
		pItem->FinalPrice = pItem->Cost;
	else if(p.Total.Rest > 0.0)
		pItem->FinalPrice = (rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) ? p.Total.Cost : p.Total.Price;
	else if(trfr->Rcpt.GetGoodsPrice(pItem->GoodsID, rBlk.BillRec.LocID, rBlk.BillRec.Dt, GPRET_INDEF, 0, &lot_rec) > 0)
		pItem->FinalPrice = R5((rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) ? lot_rec.Cost : lot_rec.Price);
	else
		pItem->FinalPrice = 0.0;
	if(rBlk.Flags & InvBlock::fAutoLine) {
		if(p.Total.Rest != 0.0 || rBlk.Flags & InvBlock::fAutoLineAllowZero) {
			inv_rec.Flags |= INVENTF_GENAUTOLINE;
			pItem->FinalQtty = (rBlk.Flags & InvBlock::fAutoLineZero) ? 0.0 : p.Total.Rest;
		}
		else
			skip = 1;
	}
	else
		pItem->FinalQtty = pItem->Qtty;
	// @v10.5.6 {
	if(p.Total.Rest <= 0.0) {
		if(rBlk.Flags & InvBlock::fExcludeZeroRestPassiv && GObj.CheckFlag(pItem->GoodsID, GF_PASSIV)) { // @v11.1.2
			skip = 1;
		}
		else if(rBlk.Flags & InvBlock::fRestrictZeroRestWithMtx) {
			if(GObj.GetConfig().MtxQkID && !GObj.BelongToMatrix(pItem->GoodsID, rBlk.BillRec.LocID)) {
				skip = 1;
			}
		}
	}
	// } @v10.5.6 
	if(!skip) {
		pItem->State = 0;
		inv_rec.BillID      = rBlk.BillRec.ID;
		inv_rec.GoodsID     = pItem->GoodsID;
		inv_rec.Quantity    = pItem->FinalQtty;
		inv_rec.UnitPerPack = (pItem->UnitPerPack > 0.0) ? pItem->UnitPerPack : p.Total.UnitsPerPack;
		inv_rec.Price       = pItem->FinalPrice;
		inv_rec.StockRest   = p.Total.Rest;
		inv_rec.StockPrice  = pItem->FinalPrice;
		inv_rec.WrOffPrice  = inv_rec.StockPrice;
		STRNSCPY(inv_rec.Serial, pItem->Serial);
		if(GetInvT().SearchIdentical(rBlk.BillRec.ID, pItem->GoodsID, pItem->Serial, &sg_rec) > 0) {
			if(rBlk.Flags & InvBlock::fFailOnDup) {
				pItem->State |= InvItem::stFailOnDup;
				SString goods_name;
				GetGoodsName(inv_rec.GoodsID, goods_name);
				CALLEXCEPT_PP_S(PPERR_INVMOVFAILONSAMEGOODS, goods_name);
			}
			else {
				THROW_PP(!(sg_rec.Flags & INVENTF_WRITEDOFF), PPERR_INVTOOWRITEDOFF);
				if(sg_rec.Flags & INVENTF_AUTOLINE) { // Просто изменить строку
					pItem->State |= InvItem::stUpdatedAutoLine;
				}
				else {
					pItem->FinalQtty += sg_rec.Quantity;
					pItem->State |= InvItem::stAddedToExistLine;
					sg_rec.Quantity = pItem->FinalQtty;
					inv_rec = sg_rec;
				}
				oprno = sg_rec.OprNo;
			}
		}
		THROW(GetInvT().Set(rBlk.BillRec.ID, &oprno, &inv_rec, use_ta));
	}
	else {
		pItem->State |= InvItem::stSkip;
		ok = -1;
	}
	CATCHZOK
	return ok;
}

int PPObjBill::LoadInventoryArray(PPID billID, InventoryArray & rList)
{
	int    ok = -1;
	rList.clear();
	InventoryTbl::Rec rec;
	for(SEnum en = GetInvT().Enum(billID); en.Next(&rec) > 0;) {
		THROW_SL(rList.insert(&rec));
	}
	if(rList.getCount())
		ok = 1;
	CATCHZOK
	return ok;
}

int PPObjBill::GetInventoryStockRest(const InvBlock & rBlk, InvItem * pItem, GoodsRestParam * pRestParam)
{
	int    ok = 1;
	ReceiptTbl::Rec lot_rec;
	PPUserFuncProfiler ufp(PPUPRF_GETINVSTOCKREST);
	switch(rBlk.Ioe.AmountCalcMethod) {
		case PPInventoryOpEx::acmFIFO: pRestParam->CalcMethod = GoodsRestParam::pcmFirstLot; break;
		case PPInventoryOpEx::acmLIFO: pRestParam->CalcMethod = GoodsRestParam::pcmLastLot; break;
		case PPInventoryOpEx::acmAVG:
		default: pRestParam->CalcMethod = GoodsRestParam::pcmAvg; break;
	}
	int    undef_item = 1;
	double rest = 0.0;
	LDATE _dt = ZERODATE;
	long  _oprno = MAXLONG;
	if(!(rBlk.Flags & InvBlock::fUseCurrent)) {
		if(pItem->RestDt) {
			_dt = pItem->RestDt;
			if(pItem->RestOprNo > 0)
				_oprno = pItem->RestOprNo;
		}
		else
			_dt = rBlk.BillRec.Dt;
	}
	if(pItem->Serial[0]) {
		if(SelectLotBySerial(pItem->Serial, pItem->GoodsID, rBlk.BillRec.LocID, &lot_rec) > 0) {
			trfr->GetRest(lot_rec.ID, NZOR(_dt, MAXDATE), _oprno, &rest, 0);
			pRestParam->Total.Init(&lot_rec, rest);
			undef_item = 0;
		}
	}
	else {
		pRestParam->GoodsID = pItem->GoodsID;
		pRestParam->Flags_  = 0;
		pRestParam->Date    = _dt;
		pRestParam->OprNo   = _oprno;
		pRestParam->LocID   = rBlk.BillRec.LocID;
		pRestParam->DiffParam |= GoodsRestParam::_diffSerial;
		THROW(trfr->GetRest(*pRestParam));
		rest = pRestParam->Total.Rest;
		if(rBlk.Ioe.Flags & INVOPF_USESERIAL) {
			const GoodsRestVal * p_val;
			InventoryTbl::Rec ident_rec;
			for(pRestParam->setPointer(0); (p_val = static_cast<const GoodsRestVal *>(pRestParam->next())) != 0;) {
				if(p_val->Serial[0] && GetInvT().SearchIdentical(rBlk.BillRec.ID, pItem->GoodsID, p_val->Serial, &ident_rec) > 0)
					rest -= p_val->Rest;
			}
		}
		if(pRestParam->Total.Rest > 0.0)
			undef_item = 0;
	}
	if(undef_item) {
		const  PPID loc_id = (rBlk.Ioe.Flags & INVOPF_USEANOTERLOCLOTS) ? -rBlk.BillRec.LocID : rBlk.BillRec.LocID;
		if(trfr->Rcpt.GetLastLot(pItem->GoodsID, loc_id, rBlk.BillRec.Dt, &lot_rec) > 0)
			pRestParam->Total.Init(&lot_rec, rest);
		else
			pRestParam->Total.Init(0, rest);
	}
	pItem->FinalRest = rest;
	pItem->StockPrice = (rBlk.Ioe.Flags & INVOPF_COSTNOMINAL) ? pRestParam->Total.Cost : pRestParam->Total.Price;
	ufp.Commit();
	CATCHZOK
	return ok;
}

int PPObjBill::EditInventory(PPBillPacket * pPack, long)
{
	int    r = cmCancel;
	int    valid_data = 0;
	InventoryDialog * dlg = new InventoryDialog(DLG_INVENTORY, this, pPack);
	if(CheckDialogPtrErr(&dlg)) {
		LDATE  old_date = pPack->Rec.Dt;
		PPSetupCtrlMenu(dlg, CTL_BILL_DOC, CTLMNU_BILL_DOC, CTRLMENU_BILLNUMBER);
		PPSetupCtrlMenu(dlg, CTL_BILL_MEMO, CTLMNU_BILL_MEMO, CTRLMENU_BILLMEMO);
		while(!valid_data && (r = ExecView(dlg)) == cmOK) {
			valid_data = dlg->getDTS(pPack);
			if(valid_data && !(pPack->ProcessFlags & PPBillPacket::pfZombie)) {
				if(pPack->Rec.Dt != old_date) {
					if(pPack->Rec.ID && P_Tbl->EnumLinks(pPack->Rec.ID, 0, BLNK_ALL) > 0)
						valid_data = (PPError(PPERR_INVTOOWRITEDOFF, 0), 0);
					else if(pPack->Rec.ID) {
						if(PPMessage(mfConf|mfYesNo, PPCFM_INVDATEUPD) != cmYes)
							valid_data = 0;
						else if(!RecalcInventoryStockRests(pPack->Rec.ID, 0, 1))
							valid_data = PPErrorZ();
						else
							pPack->Rec.BillNo = 0;
					}
					else
						pPack->Rec.BillNo = 0;
				}
				if(valid_data)
					r = TurnInventory(pPack, 1) ? cmOK : PPErrorZ();
			}
		}
		delete dlg;
	}
	else
		r = 0;
	return r;
}

int PPObjBill::RollbackInventoryWrOff(PPID id)
{
	int    ok = 1, r;
	uint   i;
	SString serial;
	PPIDArray lock_list;
	InventoryCore & r_inv_tbl = GetInvT();
	InventoryTbl::Rec inv_rec;
	PPTransferItem  * p_ti;
	PPBillPacket link_pack;
	{
		PPObjSecur::Exclusion ose(PPEXCLRT_INVWROFFROLLBACK);
		PPTransaction tra(1);
		for(DateIter diter; (r = P_Tbl->EnumLinks(id, &diter, BLNK_ALL)) > 0;) {
			const  PPID link_id = P_Tbl->data.ID;
			THROW(ExtractPacketWithFlags(link_id, &link_pack, BPLD_LOCK|BPLD_FORCESERIALS) > 0);
			lock_list.add(link_id);
			for(i = 0; link_pack.EnumTItems(&i, &p_ti);) {
				link_pack.LTagL.GetString(PPTAG_LOT_SN, i-1, serial);
				if(r_inv_tbl.SearchIdentical(id, p_ti->GoodsID, serial, &inv_rec) > 0) {
					inv_rec.WrOffPrice = inv_rec.StockPrice;
					if(inv_rec.Flags & (INVENTF_WRITEDOFF|INVENTF_GENWROFFLINE)) {
						long oprno = inv_rec.OprNo;
						if(inv_rec.Flags & INVENTF_GENWROFFLINE) {
							THROW(r_inv_tbl.Set(id, &oprno, 0, 0));
						}
						else {
							inv_rec.Flags &= ~INVENTF_WRITEDOFF;
							THROW(r_inv_tbl.Set(id, &oprno, &inv_rec, 0));
						}
					}
				}
			}
			THROW(RemovePacket(link_id, 0));
		}
		THROW(r);
		{
			LongArray to_remove_oprno_list;
			for(SEnum en = r_inv_tbl.Enum(id); en.Next(&inv_rec) > 0;) {
				if(inv_rec.Flags & (INVENTF_WRITEDOFF|INVENTF_GENWROFFLINE)) {
					long oprno = inv_rec.OprNo;
					if(inv_rec.Flags & INVENTF_GENWROFFLINE) {
						//
						// При удалении записей внутри цикла может сбойнуть
						// перечисление записей en.Next(). Поэтому запоминаем
						// список номеров записей, которые надо удалить и
						// делаем это после цикла.
						//
						to_remove_oprno_list.add(oprno);
					}
					else {
						inv_rec.Flags &= ~INVENTF_WRITEDOFF;
						THROW(r_inv_tbl.Set(id, &oprno, &inv_rec, 0));
					}
				}
			}
			//
			// Непосредственное удаление записей с флагом INVENTF_GENWROFFLINE
			//
			for(i = 0; i < to_remove_oprno_list.getCount(); i++) {
				long oprno = to_remove_oprno_list.get(i);
				THROW(r_inv_tbl.Set(id, &oprno, 0, 0));
			}
		}
		DS.LogAction(PPACN_INVENTROLLBACK, PPOBJ_BILL, id, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	{
		const int save_err_code = PPErrCode;
		for(i = 0; i < lock_list.getCount(); i++)
			Unlock(lock_list.at(i));
		PPErrCode = save_err_code;
	}
	return ok;
}

int PPObjBill::RecalcInventoryDeficit(const BillTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	InventoryTbl::Rec rec;
	InventoryCore & r_inv_tbl = GetInvT();
	CSessDfctGoodsList dfct_list;
	PPWaitStart();
	if(pRec->DueDate) {
		{
			CGoodsLine gl;
			DateRange dfct_prd;
			dfct_prd.low = pRec->DueDate;
			dfct_prd.upp = pRec->Dt;
			THROW(gl.GetDfctGoodsList(0, 0, &dfct_prd, &dfct_list));
		}
		if(dfct_list.getCount()) {
			PPWaitStop();
			PPWaitStart();
			PPTransaction tra(use_ta);
			THROW(tra);
			for(SEnum en = r_inv_tbl.Enum(pRec->ID); en.Next(&rec) > 0;) {
				double price = 0.0, qtty = 0.0;
				CSessDfctGoodsItem item;
				if(dfct_list.Search(rec.GoodsID, &item) > 0) {
					qtty = item.Qtty;
					price = item.GetPrice();
				}
				if(rec.CSesDfctQtty != qtty || rec.CSesDfctPrice != price) {
					rec.CSesDfctQtty = qtty;
					rec.CSesDfctPrice = price;
					long oprno = rec.OprNo;
					THROW(r_inv_tbl.Set(pRec->ID, &oprno, &rec, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPObjBill::RecalcInventoryStockRests(PPID billID, /*int recalcPrices*/long flags, int use_ta)
{
	int    ok = 1, r;
	IterCounter cntr;
	InventoryCore & r_inv_tbl = GetInvT();
	InventoryTbl::Rec rec;
	InvBlock blk;
	InvItem inv_item;
	PPBillPacket link_pack;
	const  PPID bill_id = billID;
	struct WrOffPriceBlock { // @flat
		WrOffPriceBlock() : InvR(0), Dt(ZERODATE), OprNo(0), Qtty(0.0), Sum(0.0)
		{
		}
		long   InvR;
		//
		// Дата и порядковый номер операции, до которых следует принимать в расчет остаток по лотам.
		// Эти поля инициализируются для аварийного режима rifAverage с целью исключить из
		// рассмотрения операции уже сделанного списания.
		//
		LDATE  Dt;
		long   OprNo;
		//
		double Qtty;
		double Sum;
	};
	SVector wroff_price_list(sizeof(WrOffPriceBlock));
	PPWaitStart();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(InitInventoryBlock(bill_id, blk));
		if(flags & rifAverage) {
			SString serial;
			for(DateIter diter; (r = P_Tbl->EnumLinks(bill_id, &diter, BLNK_ALL)) > 0;) {
				const  PPID link_id = P_Tbl->data.ID;
				PPTransferItem * p_ti;
				THROW(ExtractPacketWithFlags(link_id, &link_pack, BPLD_LOCK|BPLD_FORCESERIALS) > 0);
				for(uint i = 0; link_pack.EnumTItems(&i, &p_ti);) {
					link_pack.LTagL.GetString(PPTAG_LOT_SN, i-1, serial);
					if(r_inv_tbl.SearchIdentical(bill_id, p_ti->GoodsID, serial, &rec) > 0) {
						uint   pos = 0;
						double t_qtty = fabs(p_ti->Qtty());
						double t_sum = 0;
						//
						// Для определения номера операции за день (Transfer::OprNo) придется извлечь запись из БД
						// поскольку в PPTransferItem это значение не заносится.
						//
						TransferTbl::Rec trfr_rec;
						THROW(trfr->SearchByBill(p_ti->BillID, 0, p_ti->RByBill, &trfr_rec) > 0);
						if(p_ti->Flags & PPTFR_PLUS) {
							GoodsRestParam p;
							inv_item.Init(rec.GoodsID, rec.Serial);
							inv_item.RestDt = p_ti->Date;
							inv_item.RestOprNo = trfr_rec.OprNo;
							THROW(GetInventoryStockRest(blk, &inv_item, &p));
							t_sum = fabs(inv_item.StockPrice * p_ti->Qtty());
						}
						else {
							t_sum = fabs(((blk.Ioe.Flags & INVOPF_COSTNOMINAL) ? p_ti->Cost : p_ti->Price) * p_ti->Qtty());
						}
						if(wroff_price_list.lsearch(&rec.OprNo, &pos, CMPF_LONG)) {
							WrOffPriceBlock * p_wopb = static_cast<WrOffPriceBlock *>(wroff_price_list.at(pos));
							if(p_ti->Date < p_wopb->Dt) {
								p_wopb->Dt = p_ti->Date;
								p_wopb->OprNo = trfr_rec.OprNo;
							}
							else if(p_ti->Date == p_wopb->Dt && trfr_rec.OprNo < p_wopb->OprNo)
								p_wopb->OprNo = trfr_rec.OprNo;
							p_wopb->Qtty += t_qtty;
							p_wopb->Sum += t_sum;
						}
						else {
							WrOffPriceBlock wopb;
							wopb.Dt = p_ti->Date;
							wopb.OprNo = trfr_rec.OprNo;
							wopb.InvR = rec.OprNo;
							wopb.Qtty = t_qtty;
							wopb.Sum = t_sum;
							THROW_SL(wroff_price_list.insert(&wopb));
						}
					}
				}
			}
		}
		{
			long   c = 0;
			for(SEnum en = r_inv_tbl.Enum(bill_id); en.Next(&rec) > 0;) {
				c++;
			}
			cntr.Init(c);
		}
		{
			for(SEnum en = r_inv_tbl.Enum(bill_id); en.Next(&rec) > 0;) {
				if(!(rec.Flags & INVENTF_WRITEDOFF) || (flags & rifAverage)) {
					GoodsRestParam p;
					inv_item.Init(rec.GoodsID, rec.Serial);
					uint   wopb_pos = 0;
					const  WrOffPriceBlock * p_wopb = 0;
					if(wroff_price_list.lsearch(&rec.OprNo, &wopb_pos, CMPF_LONG)) {
						p_wopb = static_cast<const WrOffPriceBlock *>(wroff_price_list.at(wopb_pos));
						inv_item.RestDt = p_wopb->Dt;
						inv_item.RestOprNo = p_wopb->OprNo;
					}
					THROW(GetInventoryStockRest(blk, &inv_item, &p));
					if(flags & rifRest)
						rec.StockRest = p.Total.Rest;
					if(flags & rifPrice) {
						rec.Price = inv_item.StockPrice;
						rec.StockPrice = inv_item.StockPrice;
						rec.WrOffPrice = inv_item.StockPrice;
					}
					if(flags & rifAverage && rec.Flags & INVENTF_WRITEDOFF) {
						if(p_wopb && p_wopb->Qtty > 0.0 && p_wopb->Sum > 0.0) {
							rec.WrOffPrice = R5(p_wopb->Sum / p_wopb->Qtty);
						}
					}
					long oprno = rec.OprNo;
					THROW(r_inv_tbl.Set(bill_id, &oprno, &rec, 0));
				}
				PPWaitPercent(cntr.Increment());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPObjBill::ViewInventoryTotal(const PPIDArray & rIdList, const InventoryFilt * pFilt)
{
	int    ok = -1;
	InventoryTotal total;
	TDialog * dlg = 0;
	{
		InventoryCore & r_inv_core = GetInvT();
		PPIDArray goods_list;
		InventoryFilt filt;
		RVALUEPTR(filt, pFilt);
		filt.Flags |= InventoryFilt::fMultipleTotal;
		filt.BillList.Set(&rIdList);
		THROW(r_inv_core.CalcTotal(&filt, &total, &goods_list));
		total.GoodsCount = goods_list.getCountI();
	}
	{
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_INVLINETOTAL))));
		if(total.BillCount <= 1) {
			dlg->showCtrl(CTL_INVLINETOTAL_BILLC, 0);
		}
		dlg->setCtrlLong(CTL_INVLINETOTAL_BILLC,   total.BillCount);
		dlg->setCtrlLong(CTL_INVLINETOTAL_GOODSC,  total.GoodsCount);
		dlg->setCtrlLong(CTL_INVLINETOTAL_COUNT,   total.ItemsCount);
		dlg->setCtrlReal(CTL_INVLINETOTAL_SQTTY,   total.StockRest);
		dlg->setCtrlReal(CTL_INVLINETOTAL_IQTTY,   total.Quantity);
		dlg->setCtrlReal(CTL_INVLINETOTAL_DQTTY,   total.Quantity - total.StockRest);
		dlg->setCtrlReal(CTL_INVLINETOTAL_SAMT,    total.StockAmount);
		dlg->setCtrlReal(CTL_INVLINETOTAL_IAMT,    total.Amount);
		dlg->setCtrlReal(CTL_INVLINETOTAL_SURPLUS, total.Surplus);
		dlg->setCtrlReal(CTL_INVLINETOTAL_LACK,    total.Lack);
		dlg->setCtrlReal(CTL_INVLINETOTAL_DAMT,    total.Surplus - total.Lack);
		dlg->setCtrlReal(CTL_INVLINETOTAL_DFCTQTY, total.DfctQtty);
		dlg->setCtrlReal(CTL_INVLINETOTAL_DFCTAMT, total.DfctAmount);
		ExecViewAndDestroy(dlg);
	}
	CATCHZOKPPERR
	return ok;
}

InventoryCore & PPObjBill::GetInvT()
{
	SETIFZ(P_InvT, new InventoryCore);
	return *P_InvT;
}

int PPObjBill::TurnInventory(PPBillPacket * pPack, int use_ta)
{
	int    ok = -1;
	PPID   id = pPack->Rec.ID;
	const  bool is_new = (id == 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if((ok = P_Tbl->Edit(&id, pPack, 0)) > 0) {
			pPack->Rec.ID = id;
			THROW(SetTagList(pPack->Rec.ID, &pPack->BTagL, 0));
			if(is_new)
				DS.LogAction(PPACN_INVENTTURN, PPOBJ_BILL, id, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::UniteInventory(PPID destBillID, PPIDArray * pSrcBillList, InvMovSgo sgoption, int rmvSrc, int use_ta)
{
	int    ok = 1;
	long   num_iters = 0, iter_count = 0;
	uint   i;
	SString goods_name, added_msg_buf;
	PPBillPacket dest_pack;
	InventoryCore & r_inv_tbl = GetInvT();
	InventoryTbl::Rec inv_rec, sg_rec;
	THROW(ExtractPacket(destBillID, &dest_pack) > 0);
	{
		for(SEnum en = r_inv_tbl.Enum(destBillID); en.Next(&inv_rec) > 0;) {
			GetGoodsName(inv_rec.GoodsID, goods_name);
			added_msg_buf.Z().Cat("Line").CatDiv(':', 0).Cat(inv_rec.OprNo).Space().Cat(goods_name);
			THROW_PP_S(!(inv_rec.Flags & INVENTF_WRITEDOFF), PPERR_UPDWROFFINV, added_msg_buf);
		}
	}
	for(i = 0; i < pSrcBillList->getCount(); i++) {
		for(SEnum en = r_inv_tbl.Enum(pSrcBillList->get(i)); en.Next(&inv_rec) > 0;) {
			THROW_PP(!(inv_rec.Flags & INVENTF_WRITEDOFF), PPERR_UPDWROFFINV);
			num_iters++;
		}
	}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(i = 0; i < pSrcBillList->getCount(); i++) {
			PPID   src_bill_id = pSrcBillList->at(i);
			for(SEnum en = r_inv_tbl.Enum(src_bill_id); en.Next(&inv_rec) > 0;) {
				if(r_inv_tbl.SearchIdentical(destBillID, inv_rec.GoodsID, inv_rec.Serial, &sg_rec) > 0) {
					if(sgoption == imsgoAdd) {
						sg_rec.Quantity += inv_rec.Quantity;
						THROW(r_inv_tbl.Set(destBillID, &sg_rec.OprNo, &sg_rec, 0));
					}
					else if(sgoption == imsgoSkip) {
						;
					}
					else if(sgoption == imsgoFail) {
						GetGoodsName(inv_rec.GoodsID, goods_name);
						CALLEXCEPT_PP_S(PPERR_INVMOVFAILONSAMEGOODS, goods_name);
					}
				}
				else {
					THROW(r_inv_tbl.Set(destBillID, &(inv_rec.OprNo = 0), &inv_rec, 0));
				}
				PPWaitPercent(iter_count++, num_iters);
			}
			if(rmvSrc)
				THROW(RemovePacket(src_bill_id, 0));
		}
		THROW(RecalcInventoryStockRests(dest_pack.Rec.ID, 0, 0));
		THROW(RecalcInventoryDeficit(&dest_pack.Rec, 0));
		{
			InventoryTotal total;
			InventoryFilt flt;
			flt.SetSingleBillID(dest_pack.Rec.ID);
			THROW(r_inv_tbl.CalcTotal(&flt, &total));
			dest_pack.Rec.Amount = BR2(total.Amount);
			THROW(TurnInventory(&dest_pack, 0));
		}
		DS.LogAction(PPACN_INVENTUNITE, PPOBJ_BILL, destBillID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjBill::AutoFillInventory(const AutoFillInvFilt * pFilt)
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	UintHashTable sn_lot_list; // Список лотов, которые имеют серийные номера
	SString serial;
	PPObjGoods goods_obj;
	PPIDArray goods_list;
	GetInvT(); // Гарантируем существование открытого экземпляра PPObjBill::P_InvT
	CGoodsLine gl;
	long   ib_flags = InvBlock::fAutoLine;
	SETFLAG(ib_flags, InvBlock::fUseCurrent, (pFilt->Method == PPInventoryOpEx::afmByCurLotRest));
	SETFLAG(ib_flags, InvBlock::fAutoLineAllowZero, (pFilt->Method == PPInventoryOpEx::afmAll));
	SETFLAG(ib_flags, InvBlock::fAutoLineZero, (pFilt->Flags & AutoFillInvFilt::fFillWithZeroQtty));
	SETFLAG(ib_flags, InvBlock::fRestrictZeroRestWithMtx, (pFilt->Flags & AutoFillInvFilt::fRestrictZeroRestWithMtx)); // @v10.5.6
	SETFLAG(ib_flags, InvBlock::fExcludeZeroRestPassiv, (pFilt->Flags & AutoFillInvFilt::fExcludeZeroRestPassiv)); // @v11.1.2
	InvBlock blk(ib_flags);
	InvItem inv_item;
	THROW(InitInventoryBlock(pFilt->BillID, blk));
	if(blk.Ioe.Flags & INVOPF_USESERIAL) {
		PPRef->Ot.GetObjectList(PPOBJ_LOT, PPTAG_LOT_SN, sn_lot_list);
	}
	{
		GoodsIterator iter;
		for(iter.Init(pFilt->GoodsGrpID, GoodsIterator::ordByName); iter.Next(&goods_rec) > 0;) {
			if(!(goods_rec.Flags & GF_GENERIC)) {
				const int is_asst = GObj.IsAsset(goods_rec.ID);
				if((blk.Ioe.Flags & INVOPF_ASSET && is_asst) || (!(blk.Ioe.Flags & INVOPF_ASSET) && !is_asst)) {
					THROW_SL(goods_list.add(goods_rec.ID));
				}
			}
		}
	}
	const uint gc = goods_list.getCount();
	if(gc) {
		const int ta_per_line = 1; // Если !0, то каждая строка формируется отдельной транзакцией, иначе - все в общей транзакции.
		// Каждая строка в отдельной траназкции значительно снижает общую нагрузку на остальных пользователей.
		double prf_measure = 0.0;
		PPUserFuncProfiler ufp(PPUPRF_INVENTAUTOBUILD);
		PPTransaction tra(BIN(!ta_per_line));
		THROW(tra);
		for(uint i = 0; i < gc; i++) {
			const  PPID goods_id = goods_list.get(i);
			THROW(PPCheckUserBreak());
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
				if(blk.Ioe.Flags & INVOPF_USESERIAL) {
					ReceiptTbl::Rec lot_rec;
					for(DateIter di; trfr->Rcpt.EnumByGoods(goods_id, &di, &lot_rec) > 0;) {
						if(sn_lot_list.Has((uint)lot_rec.ID) && GetSerialNumberByLot(lot_rec.ID, serial, 0) > 0) {
							inv_item.Init(goods_id, serial);
							THROW(AcceptInventoryItem(blk, &inv_item, BIN(ta_per_line)));
							prf_measure	+= 1.0;
						}
					}
				}
				else {
					inv_item.Init(goods_id, 0);
					THROW(AcceptInventoryItem(blk, &inv_item, BIN(ta_per_line)));
					prf_measure	+= 1.0;
				}
			}
			else {
				goods_rec.Name[0] = 0;
			}
			PPWaitPercent(i+1, gc, goods_rec.Name);
		}
		THROW(tra.Commit());
		if(!(pFilt->Method == PPInventoryOpEx::afmByCurLotRest))
			THROW(RecalcInventoryDeficit(&blk.BillRec, 1));
		ufp.SetFactor(0, prf_measure);
		ufp.Commit();
	}
	CATCHZOK
	return ok;
}
//
//
//
class InventoryConversion {
public:
	explicit InventoryConversion(PPObjBill * pBObj) : R_Tbl(pBObj->GetInvT()), invID(0), P_BObj(pBObj), transaction(0)
	{
	}
	~InventoryConversion()
	{
		// На всякий случай проверим, чтобы не оставалось активной транзакции
		PPRollbackWork(&transaction);
	}
	int    Run(PPID);
private:
	int    Init(PPID);
	int    InitPackets();
	int    TurnPackets(int force, int initNewPackets);
	int    SearchLot(PPID goodsID, ReceiptTbl::Rec * pRec);

	PPObjBill * P_BObj;
	PPObjGoods  GObj;
	InventoryCore & R_Tbl;
	PPInventoryOpEx invOpEx;
	PPID          invID;
	PPBillPacket  invPack;
	PPBillPacket  wrDnPack;
	PPBillPacket  wrUpPack;
	int transaction;
};

int InventoryConversion::Init(PPID billID)
{
	int    ok = 1;
	invID = billID;
	THROW(P_BObj->P_Tbl->Extract(invID, &invPack) > 0);
	THROW(P_BObj->P_OpObj->FetchInventoryData(invPack.Rec.OpID, &invOpEx));
	//
	// Инвентаризацию остатков по клиентам не списываем
	//
	if(!(invOpEx.Flags & INVOPF_INVBYCLIENT)) {
		THROW_PP(invOpEx.WrDnOp || invOpEx.WrUpOp, PPERR_UNDEFINVWROPS);
		if(!invOpEx.WrDnOp && PPMessage(mfConf|mfYesNo, PPCFM_UNDEFINVWRDNOP) != cmYes)
			ok = -1;
		if(!invOpEx.WrUpOp && PPMessage(mfConf|mfYesNo, PPCFM_UNDEFINVWRUPOP) != cmYes)
			ok = -1;
	}
	else
		ok =-1;
	CATCHZOK
	return ok;
}

int InventoryConversion::InitPackets()
{
	int    ok = 1;
	if(invOpEx.WrDnOp) {
		THROW(wrDnPack.CreateBlank(invOpEx.WrDnOp, 0, invPack.Rec.LocID, !transaction));
		wrDnPack.Rec.LinkBillID = invPack.Rec.ID;
		wrDnPack.Rec.Dt = invPack.Rec.Dt;
		wrDnPack.Rec.LocID      = invPack.Rec.LocID;
		wrDnPack.Rec.Object     = invOpEx.WrDnObj;
	}
	if(invOpEx.WrUpOp) {
		THROW(wrUpPack.CreateBlank(invOpEx.WrUpOp, 0, invPack.Rec.LocID, !transaction));
		wrUpPack.Rec.LinkBillID = invPack.Rec.ID;
		wrUpPack.Rec.Dt = invPack.Rec.Dt;
		wrUpPack.Rec.LocID      = invPack.Rec.LocID;
		wrUpPack.Rec.Object     = invOpEx.WrUpObj;
	}
	THROW(PPStartTransaction(&transaction, 1));
	CATCHZOK
	return ok;
}

int InventoryConversion::TurnPackets(int force, int initNewPackets)
{
	int    ok = 1;
	if(force || wrDnPack.CheckLargeBill(0) || wrUpPack.CheckLargeBill(0)) {
		wrDnPack.SetQuantitySign(-1);
		wrDnPack.ProcessFlags |= PPBillPacket::pfViewPercentOnTurn;
		THROW(P_BObj->__TurnPacket(&wrDnPack, 0, 1, 0));
		wrUpPack.SetQuantitySign(-1);
		wrUpPack.ProcessFlags |= PPBillPacket::pfViewPercentOnTurn;
		THROW(P_BObj->__TurnPacket(&wrUpPack, 0, 1, 0));
		THROW(PPCommitWork(&transaction));
		if(initNewPackets)
			THROW(InitPackets());
	}
	CATCHZOK
	return ok;
}

int InventoryConversion::Run(PPID billID)
{
	int    ok = 1;
	PPLogger logger;
	THROW(ok = Init(billID));
	if(ok > 0) {
		Goods2Tbl::Rec grec;
		PPIDArray goods_id_list;
		for(GoodsIterator iter(GoodsIterator::ordByName); iter.Next(&grec) > 0;) {
			THROW_SL(goods_id_list.add(grec.ID));
		}
		{
			PPObjSecur::Exclusion ose(PPEXCLRT_INVWROFF);
			PPObjArticle ar_obj;
			PPObjBill::InvBlock blk;
			PPObjBill::InvItem inv_item;
			InventoryArray inv_list;
			LongArray rows;
			SString clb;
			SString temp_buf;
			SString msg, goods_name;
			StringSet excl_serial(';', 0);
			THROW(P_BObj->InitInventoryBlock(invID, blk));
			THROW(InitPackets());
			const uint c = goods_id_list.getCount();
			for(uint i = 0; i < c; i++) {
				const  PPID goods_id = goods_id_list.get(i);
				int    r;
				int    is_inv_exists = 0;
				int    is_asset = 0;
				GoodsRestParam p;
				THROW(r = R_Tbl.SearchByGoods(invID, goods_id, &inv_list));
				is_inv_exists = BIN(r > 0);
				if(!is_inv_exists) {
					InventoryTbl::Rec ir;
					inv_list.insert(&ir);
					if(GObj.IsAsset(goods_id))
						is_asset = 1;
				}
				if(is_inv_exists || (!is_asset && invOpEx.Flags & INVOPF_ZERODEFAULT)) {
					uint j;
					excl_serial.Z();
					for(j = 0; j < inv_list.getCount(); j++) {
						if(inv_list.at(j).Serial[0] != 0)
							excl_serial.add(inv_list.at(j).Serial);
					}
					for(j = 0; j < inv_list.getCount(); j++) {
						double wroff_price = 0.0;
						const InventoryTbl::Rec & r_ir = inv_list.at(j);
						if(!(r_ir.Flags & INVENTF_WRITEDOFF) || r_ir.UnwritedDiff != 0.0) {
							ILTI   ilti;
							long   oprno;
							double diff;
							int    is_writed_off = 0;
							ReceiptTbl::Rec lotr;
							inv_item.Init(goods_id, r_ir.Serial);
							THROW(P_BObj->GetInventoryStockRest(blk, &inv_item, &p));
							double inv_rest  = R6(r_ir.Quantity);
							double inv_price = r_ir.Price;
							double stock_rest  = R6(inv_item.FinalRest);
							double stock_price = R2(inv_item.StockPrice);
							if(r_ir.Flags & INVENTF_WRITEDOFF)
								diff = R6(r_ir.UnwritedDiff) * INVENT_DIFFSIGN(r_ir.Flags);
							else
								diff = R6(inv_rest - stock_rest);
							if(diff < 0.0) {
								if(invOpEx.WrDnOp) {
									const char * p_cvt_serial = 0;
									uint  cvt_flags = CILTIF_DEFAULT;
									// @v10.7.4 {
									if(inv_rest == 0.0)
										cvt_flags |= CILTIF_CUTRESTTOZERO;
									// } @v10.7.4 
									if(invOpEx.Flags & INVOPF_WROFFWODSCNT)
										cvt_flags |= CILTIF_ZERODSCNT;
									if(r_ir.Serial[0] == 0 && excl_serial.getDataLen()) {
										p_cvt_serial = excl_serial.getBuf();
										cvt_flags |= CILTIF_EXCLUDESERIAL;
									}
									else
										p_cvt_serial = r_ir.Serial;
									ilti.GoodsID = goods_id;
			   						ilti.Price   = 0.0;
									ilti.SetQtty(diff);
									rows.clear();
									THROW(P_BObj->ConvertILTI(&ilti, &wrDnPack, &rows, cvt_flags, p_cvt_serial));
									{
										double amt = 0.0, t_qtty = 0.0;
										for(uint rp = 0; rp < rows.getCount(); rp++) {
											const PPTransferItem & r_ti = wrDnPack.ConstTI(rows.at(rp));
											t_qtty += r_ti.Quantity_;
											amt    += r_ti.Quantity_ * ((invOpEx.Flags & INVOPF_COSTNOMINAL) ? r_ti.Cost : r_ti.Price);
										}
										wroff_price = fdivnz(amt, t_qtty);
									}
									is_writed_off = 1;
								}
							}
							else if(diff > 0.0) {
								if(invOpEx.WrUpOp) {
									ilti.Flags |= PPTFR_RECEIPT;
									ilti.GoodsID  = goods_id;
									ilti.Cost     = (invOpEx.Flags & INVOPF_COSTNOMINAL) ? inv_price : p.Total.Cost;
									ilti.Price    = (invOpEx.Flags & INVOPF_COSTNOMINAL) ? p.Total.Price : inv_price;
									clb.Z();
									if(SearchLot(goods_id, &lotr) > 0) {
										if(ilti.Cost <= 0.0)
											ilti.Cost = R5(lotr.Cost);
										if(ilti.Price <= 0.0)
											ilti.Price = R5(lotr.Price);
										ilti.UnitPerPack = lotr.UnitPerPack;
										ilti.Suppl  = lotr.SupplID;
										ilti.QCert  = lotr.QCertID;
										ilti.Flags |= PPTFR_FORCESUPPL;
										THROW(P_BObj->GetClbNumberByLot(lotr.ID, 0, clb));
									}
									else
										ilti.Suppl = invOpEx.WrUpObj;
									if(ilti.Suppl == 0) {
										THROW(ar_obj.GetMainOrgAsSuppl(&ilti.Suppl, 1, !transaction));
										if(ilti.Suppl)
											ilti.Flags |= PPTFR_FORCESUPPL;
									}
									if(ilti.Suppl) {
										if(ilti.Price <= 0.0 && ilti.Cost > 0.0)
											ilti.Price = ilti.Cost;
										if(ilti.Cost <= 0.0 && ilti.Price > 0.0)
											ilti.Cost = ilti.Price;
										ilti.SetQtty(diff);
										rows.clear();
										THROW(P_BObj->ConvertILTI(&ilti, &wrUpPack, &rows, CILTIF_DEFAULT|CILTIF_INHLOTTAGS, r_ir.Serial));
										THROW(wrUpPack.LTagL.SetString(PPTAG_LOT_CLB, &rows, clb));
										{
											double amt = 0.0, t_qtty = 0.0;
											for(uint rp = 0; rp < rows.getCount(); rp++) {
												const PPTransferItem & r_ti = wrUpPack.ConstTI(rows.at(rp));
												t_qtty += r_ti.Quantity_;
												amt    += r_ti.Quantity_ * ((invOpEx.Flags & INVOPF_COSTNOMINAL) ? r_ti.Cost : r_ti.Price);
											}
											wroff_price = fdivnz(amt, t_qtty);
										}
										is_writed_off = 1;
									}
								}
							}
							{
								InventoryTbl::Rec ir;
								if(is_inv_exists)
									ir = r_ir;
								if(is_writed_off) {
									if(!is_inv_exists) {
										ir.BillID   = invID;
										ir.GoodsID  = goods_id;
										ir.Flags   |= INVENTF_GENWROFFLINE;
										ir.Quantity = 0.0;
										ir.Price    = stock_price;
										oprno       = 0;
									}
									else
										oprno = ir.OprNo;
									ir.StockRest  = stock_rest;
									ir.StockPrice = stock_price;
									ir.Flags     |= INVENTF_WRITEDOFF;
									ir.UnwritedDiff = fabs(ilti.Rest);
									ir.WrOffPrice   = wroff_price;
									THROW(R_Tbl.Set(invID, &oprno, &ir, 0));
									THROW(TurnPackets(0, 1));
									if(ir.UnwritedDiff) {
										PPLoadText(PPTXT_WROFFGOODS, temp_buf);
										GetGoodsName(ir.GoodsID, goods_name);
										msg.Printf(temp_buf, goods_name.cptr(), inv_rest - fabs(ir.UnwritedDiff), inv_rest);
										logger.Log(msg);
									}
								}
								else if(is_inv_exists) {
									ir.StockRest  = stock_rest;
									ir.StockPrice = stock_price;
									ir.Flags &= ~INVENTF_WRITEDOFF;
									oprno = ir.OprNo;
									ir.WrOffPrice = ir.StockPrice;
									THROW(R_Tbl.Set(invID, &oprno, &ir, 0));
								}
							}
						}
					}
				}
				PPWaitPercent(i+1, c);
			}
			THROW(TurnPackets(1, 0));
			{
				PPTransaction tra(1); // use_ta = 1 (TurnPackets закрывает свою транзакцию).
				THROW(tra);
				// @v10.5.9 {
				if(invOpEx.OnWrOffStatusID) {
					if(P_BObj->SetStatus(billID, invOpEx.OnWrOffStatusID, 0)) {
						BillTbl::Rec bill_rec;
						PPID   temp_bill_id = billID;
						THROW(P_BObj->Search(billID, &bill_rec) > 0);
						if(!(bill_rec.Flags & BILLF_WRITEDOFF)) {
							bill_rec.Flags |= BILLF_WRITEDOFF;
							THROW(P_BObj->P_Tbl->EditRec(&temp_bill_id, &bill_rec, 0));
						}
					}
					else {
						logger.LogLastError();
					}
				}
				// } @v10.5.9 
				DS.LogAction(PPACN_INVENTWROFF, PPOBJ_BILL, invID, 0, 0); 
				THROW(tra.Commit());
			}
		}
	}
	else
		ok = -1;
	CATCH
		PPRollbackWork(&transaction);
		ok = 0;
	ENDCATCH
	return ok;
}

int InventoryConversion::SearchLot(PPID goodsID, ReceiptTbl::Rec * pRec)
{
	PPID   loc_id = invPack.Rec.LocID;
	if(invOpEx.Flags & INVOPF_USEANOTERLOCLOTS)
		loc_id = -loc_id;
	return P_BObj->trfr->Rcpt.GetLastLot(goodsID, loc_id, invPack.Rec.Dt, pRec);
}

int PPObjBill::ConvertInventory(PPID invID)
{
	InventoryConversion ic(this);
	return ic.Run(invID);
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(INVENTORYITEM, PPInventoryImpExpParam);

PPInventoryImpExpParam::PPInventoryImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags)
{
}

class PrcssrInvImport {
public:
	struct Param {
		Param() : OpID(0), LocID(0), Dt(ZERODATE)
		{
		}
		PPID   OpID;
		PPID   LocID;
		LDATE  Dt;
		SString CfgName;
	};
	PrcssrInvImport();
	int    InitParam(Param *);
	int    EditParam(Param *);
	int    Init(const Param *);
	int    Run();
private:
	int    IdentifyBySerial(const char * pSerial, PPObjBill::InvItem * pInvItem, PPLogger & rLogger);

	PPObjBill * P_BObj;
	PPObjGoods GObj;
	Param  P;
	PPInventoryImpExpParam IeParam;
};

PrcssrInvImport::PrcssrInvImport() : P_BObj(BillObj)
{
}

int PrcssrInvImport::InitParam(Param * pParam)
{
	int    ok = -1;
	if(pParam) {
		pParam->LocID = LConfig.Location;
		pParam->OpID = GetSingleOp(PPOPT_INVENTORY);
		pParam->Dt = getcurdate_();
		pParam->CfgName = 0;
		ok = 1;
	}
	return ok;
}

int PrcssrInvImport::EditParam(Param * pParam)
{
	class InvImportDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrInvImport::Param);
	public:
		InvImportDialog() : TDialog(DLG_IEINV)
		{
			PPInventoryImpExpParam param;
			SetupCalDate(CTLCAL_IEINV_DATE, CTL_IEINV_DATE);
			GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_INVENTORYITEM, &param, &CfgList, 2 /* import */);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			StringSet ss;
			PPIDArray op_type_list;
			op_type_list.add(PPOPT_INVENTORY);
			const long cfg_id = (CfgList.getCount() == 1) ? CfgList.Get(0).Id : 0;
			SetupStrAssocCombo(this, CTLSEL_IEINV_CFG, CfgList, cfg_id, 0, 0, 0);
			SetupOprKindCombo(this, CTLSEL_IEINV_OP, Data.OpID, 0, &op_type_list, 0);
			SetupPPObjCombo(this, CTLSEL_IEINV_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			setCtrlDate(CTL_IEINV_DATE, Data.Dt);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			const  long cfg_id = getCtrlLong(sel = CTLSEL_IEINV_CFG);
			SString temp_buf;
			THROW_PP(cfg_id, PPERR_CFGNEEDED);
			THROW_PP(CfgList.GetText(cfg_id, Data.CfgName) > 0, PPERR_CFGNEEDED);
			Data.OpID = getCtrlLong(sel = CTLSEL_IEINV_OP);
			THROW_PP(Data.OpID, PPERR_OPRKINDNEEDED);
			Data.LocID = getCtrlLong(sel = CTLSEL_IEINV_LOC);
			THROW_PP(Data.LocID, PPERR_LOCNEEDED);
			getCtrlData(sel = CTL_IEINV_DATE, &Data.Dt);
			SETIFZ(Data.Dt, getcurdate_());
			THROW_SL(checkdate(Data.Dt));
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		StrAssocArray CfgList;
	};
	DIALOG_PROC_BODY(InvImportDialog, pParam);
}

int PrcssrInvImport::Init(const Param * pParam)
{
	RVALUEPTR(P, pParam);
	int    ok = 1;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(LoadSdRecord(PPREC_INVENTORYITEM, &IeParam.InrRec));
		IeParam.Direction = 1; // import
		THROW(IeParam.ProcessName(1, P.CfgName));
		THROW(IeParam.ReadIni(&ini_file, P.CfgName, 0));
	}
	CATCHZOK
	return ok;
}

int PrcssrInvImport::IdentifyBySerial(const char * pSerial, PPObjBill::InvItem * pInvItem, PPLogger & rLogger)
{
	int    ok = -1;
	SString serial;
	(serial = pSerial).Strip();
	if(serial.NotEmpty()) {
		PPIDArray lot_list;
		if(P_BObj->SearchLotsBySerialExactly(serial, &lot_list) > 0) {
			ReceiptTbl::Rec lot_rec;
			int r = P_BObj->SelectLotFromSerialList(&lot_list, P.LocID, 0, &lot_rec);
			if(r > 0 || r == -3) { // -3 - закрытый лот
				Goods2Tbl::Rec goods_rec;
				if(GObj.Fetch(labs(lot_rec.GoodsID), &goods_rec) > 0) {
					SString fmt_buf, log_msg;
					// @log Товар '%s' идентифицирован по серийному номеру '%s'
					PPLoadText((r == -3) ? PPTXT_LOG_IMPINV_GOODSIDBYSERIALCL : PPTXT_LOG_IMPINV_GOODSIDBYSERIAL, fmt_buf);
					rLogger.Log(log_msg.Printf(fmt_buf, goods_rec.Name, pSerial));
					pInvItem->GoodsID = goods_rec.ID;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PrcssrInvImport::Run()
{
	int    ok = -1;
	long   numrecs = 0;
	SString log_msg, fmt_buf, temp_buf, temp_buf2;
	SString filename;
	StringSet ss_files;
	SString serial;
	PPLogger logger;
	PPImpExp ie(&IeParam, 0);
	InventoryCore & r_inv_tbl = BillObj->GetInvT();
	PPWaitStart();
	THROW(IeParam.PreprocessImportFileSpec(ss_files)); // @v10.9.1
	ss_files.sortAndUndup();
	for(uint ssp = 0; ss_files.get(&ssp, filename);) {
		THROW(ie.OpenFileForReading(filename));
		THROW(ie.GetNumRecs(&numrecs));
		if(numrecs) {
			int    r;
			IterCounter cntr;
			PPBillPacket pack;
			Goods2Tbl::Rec goods_rec;
			BarcodeTbl::Rec bc_rec;
			PPTransaction tra(1);
			THROW(tra);
			THROW(pack.CreateBlank2(P.OpID, P.Dt, P.LocID, 0));
			THROW(P_BObj->TurnInventory(&pack, 0));
			{
				PPObjBill::InvBlock blk;
				Sdr_InventoryItem rec;
				THROW(P_BObj->InitInventoryBlock(pack.Rec.ID, blk));
				cntr.Init(numrecs);
				while((r = ie.ReadRecord(&rec, sizeof(rec))) > 0) {
					serial.Z();
					int    r2 = 0;
					int    is_wght_good = 0;
					PPObjBill::InvItem inv_item;
					inv_item.Init(0, 0);
					inv_item.Cost = rec.Cost;
					inv_item.Price = rec.Price;
					(temp_buf = rec.Barcode).Strip().Transf(CTRANSF_OUTER_TO_INNER).CopyTo(rec.Barcode, sizeof(rec.Barcode));
					(temp_buf = rec.GoodsName).Strip().Transf(CTRANSF_OUTER_TO_INNER).CopyTo(rec.GoodsName, sizeof(rec.GoodsName));
					(temp_buf = rec.Serial).Strip().Transf(CTRANSF_OUTER_TO_INNER).CopyTo(rec.Serial, sizeof(rec.Serial));
					//
					// Идентификация товара {
					//
					if(IdentifyBySerial(rec.Serial, &inv_item, logger) > 0) {
						STRNSCPY(inv_item.Serial, rec.Serial);
						r2 = 1;
					}
					else if(rec.GoodsID && GObj.Fetch(rec.GoodsID, &goods_rec) > 0) {
						// @log Товар '%s' идентифицирован по идентификатору '%s'
						PPLoadText(PPTXT_LOG_IMPINV_GOODSIDBYID, fmt_buf);
						logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name, temp_buf.Z().Cat(rec.GoodsID).cptr()));
						inv_item.GoodsID = rec.GoodsID;
						r2 = 1;
					}
					else if(rec.Barcode[0]) {
						size_t code_len = sstrlen(rec.Barcode);
						const int wp = GObj.GetConfig().IsWghtPrefix(rec.Barcode);
						if(wp && oneof2(code_len, 12, 13)) {
							is_wght_good = 1;
							rec.Barcode[12] = 0;
							inv_item.Qtty = (wp == 2) ? (double)atol(rec.Barcode+7) : fdiv1000i(atol(rec.Barcode+7));
							rec.Barcode[7] = 0;
							if(GObj.GetConfig().Flags & GCF_LOADTOSCALEGID)
								strtolong(rec.Barcode + 2, &inv_item.GoodsID);
						}
						if(is_wght_good && GObj.GetConfig().Flags & GCF_LOADTOSCALEGID) {
							if(GObj.Fetch(inv_item.GoodsID, 0) > 0) {
								// @log Товар '%s' идентифицирован по идентификатору '%s'
								PPLoadText(PPTXT_LOG_IMPINV_GOODSIDBYID, fmt_buf);
								logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name, temp_buf.Z().Cat(inv_item.GoodsID).cptr()));
								r2 = 1;
							}
						}
						else if(GObj.SearchByBarcode(rec.Barcode, &bc_rec, &goods_rec, 1) > 0) {
							inv_item.GoodsID = bc_rec.GoodsID;
							if(!is_wght_good) {
								inv_item.Qtty = bc_rec.Qtty;
								// @log Товар '%s' идентифицирован по штрихкоду '%s'
								PPLoadText(PPTXT_LOG_IMPINV_GOODSIDBYBARCODE, fmt_buf);
								logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name, rec.Barcode));
							}
							else {
								// @log Товар (весовой) '%s' идентифицирован по штрихкоду '%s'
								PPLoadText(PPTXT_LOG_IMPINV_GOODSIDBYBARCODEW, fmt_buf);
								logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name, rec.Barcode));
							}
							r2 = 1;
						}
						else if(IdentifyBySerial(rec.Barcode, &inv_item, logger) > 0)
							r2 = 1;
					}
					if(!r2 && rec.GoodsName[0]) {
						//
						// Если не удалось идентифицировтаь товар ни по идентификатору, ни по штрихкоду, ни по серии,
						// то предпринимаем последнюю попытку - по имени. Шансы на эту попытку не велики, но
						// общую надежность импорта это повышает.
						//
						if(GObj.SearchByName(rec.GoodsName, &inv_item.GoodsID, &goods_rec) > 0) {
							// @log Товар '%s' идентифицирован по имени
							PPLoadText(PPTXT_LOG_IMPINV_GOODSIDBYNAME, fmt_buf);
							logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name));
							r2 = 1;
						}
					}
					//
					// } Закончена идентификация товара
					//

					//
					// Если штриход весовой, то в коде могло быть "зашито" количество товара.
					// Поле Quantity имеет приоритет перед этим значением, однако, если rec.Quantity нулевое
					// или отрицательное, то мы воспользуемся величиной из штрихкода.
					//
					if(rec.Quantity > 0.0)
						inv_item.Qtty = rec.Quantity;
					if(r2 == 0) {
						// @log Строка не проведена: не удалось идентифицировать товар '%s'
						PPLoadText(PPTXT_LOG_IMPINV_GOODSNOTIDD, fmt_buf);
						temp_buf.Z().CatEq("recno", cntr+1);
						if(rec.GoodsName[0])
							temp_buf.CatDiv(':', 1).Cat(rec.GoodsName);
						if(rec.GoodsID)
							temp_buf.CatDiv(':', 1).Cat(rec.GoodsID);
						if(rec.Barcode[0])
							temp_buf.CatDiv(':', 1).Cat(rec.Barcode);
						if(rec.Serial[0])
							temp_buf.CatDiv(':', 1).Cat(rec.Serial);
						temp_buf.CatDiv(':', 1).CatEq("qtty", rec.Quantity);
						logger.Log(log_msg.Printf(fmt_buf, temp_buf.cptr()));
					}
					else if(inv_item.Qtty < 0.0 || inv_item.Qtty > 1000000.) {
						// @log Строка не проведена: для товара '%s' задано недопустимое значение количества '%s'
						PPLoadText(PPTXT_LOG_IMPINV_INVQTTY, fmt_buf);
						temp_buf.Z().Cat(rec.Quantity, SFMT_QTTY);
						log_msg.Printf(fmt_buf, goods_rec.Name, temp_buf.cptr());
						logger.Log(log_msg);
					}
					else {
						THROW(P_BObj->AcceptInventoryItem(blk, &inv_item, 0));
						if(inv_item.State & PPObjBill::InvItem::stAddedToExistLine) {
							// @log Товар '%s' уже встречался в файле - количество суммировано
							PPLoadText(PPTXT_LOG_IMPINV_DUPGOODS, fmt_buf);
							logger.Log(log_msg.Printf(fmt_buf, goods_rec.Name));
						}
						// @log Строка '%s' проведена
						PPLoadText(PPTXT_LOG_IMPINV_LINETURNED, fmt_buf);
						temp_buf.Z().Cat(goods_rec.Name).CatDiv('-', 1).Cat(inv_item.FinalQtty, SFMT_QTTY).
							CatDiv('-', 1).Cat(inv_item.FinalPrice, SFMT_MONEY);
						logger.Log(log_msg.Printf(fmt_buf, temp_buf.cptr()));
						ok = 1;
					}
					MEMSZERO(rec);
					PPWaitPercent(cntr.Increment());
				}
				THROW(r);
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	logger.Save(PPFILNAM_IMPEXP_LOG, 0);
	PPWaitStop();
	return ok;
}

int ImportInventory()
{
	int    ok = -1;
	PrcssrInvImport prcssr;
	PrcssrInvImport::Param param;
	prcssr.InitParam(&param);
	while(ok < 0 && prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			PPError();
	return ok;
}

int EditInventoryImpExpParams()
{
	int    ok = -1;
	PPInventoryImpExpParam param;
	ImpExpParamDialog * dlg = new ImpExpParamDialog(DLG_IMPEXP, 0);
	THROW(CheckDialogPtr(&dlg));
	THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_INVENTORYITEM, &param, dlg));
	CATCHZOK
	delete dlg;
	return ok;
}
//
//
//
class GeneratorGoods {
public:
	struct Param {
		Param() : LocID(0), GoodsGrpID(0), Part(0), Flags(0)
		{
		}
		enum {
			fOnStockOnly = 0x0001,
			fAllowDup    = 0x0002
		};
		PPID   LocID;
		PPID   GoodsGrpID;
		ulong  Part;       // Часть общего справочника (в промилле), которая должна принимать участие в выборке
		long   Flags;
	};
	GeneratorGoods();
	~GeneratorGoods();
	int    InitParam(Param * pParam);
	int    Init(const Param *);
	int    Next(PPID * pGoodsID, Goods2Tbl::Rec * pRec);
private:
	Param  P;
	PPObjGoods GObj;
	PPIDArray List;
	PPIDArray SeenList; // Список уже просмотренных значений (если (P.Flags & fAllowDup), то список не используется)
	SRng * P_Rng;
};

GeneratorGoods::GeneratorGoods() : P_Rng(0)
{
}

GeneratorGoods::~GeneratorGoods()
{
	delete P_Rng;
}

int GeneratorGoods::InitParam(Param * pParam)
{
	if(pParam) {
		pParam->LocID = 0;
		pParam->GoodsGrpID = 0;
		pParam->Part = 0;
		pParam->Flags = 0;
	}
	return 1;
}

int GeneratorGoods::Init(const Param * pParam)
{
	P = *pParam;
	int    ok = 1;
	//
	// Получаем "случайную" величину до перебора товаров, так как потом нам снова потребуется взять
	// случайную величину от времени.
	//
	const  ulong s = getcurtime_().v;
	SRng * p_rng = 0;
	Goods2Tbl::Rec goods_rec;
	GoodsIterator iter(P.GoodsGrpID, 0, 0);
	List.clear();
	SeenList.clear();
	ZDELETE(P_Rng);
	while(iter.Next(&goods_rec) > 0) {
		THROW_SL(List.add(goods_rec.ID));
	}
	if(P.Part > 0 && P.Part < 1000) {
		PPIDArray temp_list;
		const uint c = List.getCount();
		THROW(p_rng = SRng::CreateInstance(SRng::algMT, 0));
		p_rng->Set(s);
		while(temp_list.getCount() < ((c * P.Part) / 1000)) {
			uint pos = (uint)p_rng->GetUniformInt(c);
			if(pos < c) {
				THROW_SL(temp_list.addUnique(List.at(pos)));
			}
		}
		List = temp_list;
	}
	THROW(P_Rng = SRng::CreateInstance(SRng::algMT, 0));
	P_Rng->Set(getcurtime_().v);
	CATCHZOK
	delete p_rng;
	return ok;
}

int GeneratorGoods::Next(PPID * pGoodsID, Goods2Tbl::Rec * pRec)
{
	int    ok = -1;
	PPID   goods_id = 0;
	if(P_Rng) {
		uint c = List.getCount();
		if(c) {
			while(goods_id == 0 && SeenList.getCount() < c) {
				uint pos = (uint)P_Rng->GetUniformInt(c);
				if(pos < c) {
					goods_id = List.get(pos);
					if(!(P.Flags & Param::fAllowDup)) {
						if(SeenList.lsearch(goods_id))
							goods_id = 0;
						else
							THROW_SL(SeenList.add(goods_id));
					}
				}
			}
		}
	}
	if(goods_id) {
		if(pRec)
			GObj.Fetch(goods_id, pRec);
		ok = 1;
	}
	CATCHZOK
	ASSIGN_PTR(pGoodsID, goods_id);
	return ok;
}

#if SLTEST_RUNNING // {

int TestGenerateInventory()
{
	int    ok = 1;
	StrAssocArray cfg_list;
	PPInventoryImpExpParam param;
	GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_INVENTORYITEM, &param, &cfg_list, 1 /* export */);
	PPWaitStart();
	if(cfg_list.getCount()) {
		SString temp_buf;
		PPObjGoods goods_obj;
		GeneratorGoods gen;
		GeneratorGoods::Param gp;
		GoodsRestParam grp;
		grp.LocID = LConfig.Location;
		gen.InitParam(&gp);
		gp.Flags |= GeneratorGoods::Param::fAllowDup;
		gp.Part = 300; // 30% от общей номенклатуры
		PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, temp_buf);
		PPIniFile ini_file(temp_buf, 0, 1, 1);
		THROW(gen.Init(&gp));
		for(uint i = 0; i < cfg_list.getCount(); i++) {
			IterCounter cntr;
			cntr.Init(10000);
			PPInventoryImpExpParam ie_param;
			THROW(LoadSdRecord(PPREC_INVENTORYITEM, &ie_param.InrRec));
			{
				temp_buf = cfg_list.Get(i).Txt;
				ie_param.ProcessName(1, temp_buf);
				THROW(ie_param.ReadIni(&ini_file, temp_buf, 0));
				{
					PPImpExp ie(&ie_param, 0);
					PPID   goods_id = 0;
					Goods2Tbl::Rec goods_rec;
					THROW(ie.OpenFileForWriting(0, 1));
					while((ulong)cntr < cntr.GetTotal() && gen.Next(&goods_id, &goods_rec) > 0) {
						Sdr_InventoryItem item;
						item.GoodsID = goods_rec.ID;
						STRNSCPY(item.GoodsName, goods_rec.Name);
						SOemToChar(item.GoodsName);
						goods_obj.GetSingleBarcode(goods_rec.ID, item.Barcode, sizeof(item.Barcode));
						grp.GoodsID = goods_rec.ID;
						BillObj->trfr->GetRest(grp);
						double diff = fabs(SLS.GetTLA().Rg.GetGaussian(sqrt(0.2)));
						item.Quantity = grp.Total.Rest * diff;
						//
						// Изредка вставляем нулевые значения //
						//
						if(item.Quantity <= 0.0 && ((ulong)cntr % 10) == 0)
							item.Quantity = 10 * diff;
						if(item.Quantity > 0.0) {
							item.Quantity = (!(goods_rec.Flags & GF_INTVAL)) ?
								R3(item.Quantity) : ((item.Quantity > 1.0) ? fint(item.Quantity) : 1.0);
							THROW(ie.AppendRecord(&item, sizeof(item)));
							PPWaitPercent(cntr.Increment());
						}
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	PPWaitStop();
	return ok;
}

SLTEST_R(GenerateInventory)
{
	SLCHECK_NZ(TestGenerateInventory());
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
