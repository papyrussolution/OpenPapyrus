// V_LOCTR.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2011, 2012, 2013, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewLocTransf)
//
IMPLEMENT_PPFILT_FACTORY(LocTransf); SLAPI LocTransfFilt::LocTransfFilt() : PPBaseFilt(PPFILT_LOCTRANSF, 0, 2)
{
	SetFlatChunk(offsetof(LocTransfFilt, ReserveStart),
		offsetof(LocTransfFilt, LocList) - offsetof(LocTransfFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(LocTransfFilt, LocList));
	SetBranchObjIdListFilt(offsetof(LocTransfFilt, BillList));
	Init(1, 0);
}
//
//
//
PalletCtrlGroup::PalletCtrlGroup(uint ctlselPalletType, uint ctlPallet, uint ctlPalletCount,
	uint ctlPckg, uint ctlPckgCount, uint ctlQtty, uint ctlselGoods)
{
	State = 0;
	CtlselPalletType = ctlselPalletType;
	CtlPallet = ctlPallet;
	CtlPalletCount = ctlPalletCount;
	CtlPckg = ctlPckg;
	CtlPckgCount = ctlPckgCount;
	CtlQtty = ctlQtty;
	CtlselGoods = ctlselGoods;
	MEMSZERO(Data);
}

int PalletCtrlGroup::RecalcQtty(TDialog * pDlg, int cargoUnit)
{
	double src_val = 0.0;
	if(cargoUnit == CARGOUNIT_PCKG)
		src_val = pDlg->getCtrlReal(CtlPckgCount);
	else if(cargoUnit == CARGOUNIT_PALLET)
		src_val = (double)pDlg->getCtrlUInt16(CtlPalletCount);
	else if(cargoUnit == CARGOUNIT_ITEM)
		src_val = pDlg->getCtrlReal(CtlQtty);
	if(src_val < 0.0)
		src_val = 0.0;
	PPID   pallet_type_id = pDlg->getCtrlLong(CtlselPalletType);
	double dest_val = 0.0;
	if(cargoUnit != CARGOUNIT_ITEM)
		if(Gse.ConvertCargoUnits(cargoUnit, CARGOUNIT_ITEM, pallet_type_id, src_val, &dest_val, 0) > 0) {
			pDlg->setCtrlReal(CtlQtty, dest_val);
		}
	if(cargoUnit != CARGOUNIT_PCKG)
		if(Gse.ConvertCargoUnits(cargoUnit, CARGOUNIT_PCKG, pallet_type_id, src_val, &dest_val, 0) > 0) {
			pDlg->setCtrlReal(CtlPckgCount, dest_val);
		}
	if(cargoUnit != CARGOUNIT_PALLET)
		if(Gse.ConvertCargoUnits(cargoUnit, CARGOUNIT_PALLET, pallet_type_id, src_val, &dest_val, 0) > 0) {
			pDlg->setCtrlUInt16(CtlPalletCount, (int16)dest_val);
		}
	return 1;
}

int PalletCtrlGroup::SetupPallet(TDialog * pDlg, int doSelect)
{
	GoodsStockExt::Pallet plt;
	if((Data.PalletTypeID || doSelect) && Gse.GetPalletEntry(Data.PalletTypeID, &plt) > 0) {
		pDlg->setCtrlLong(CtlselPalletType, plt.PalletTypeID);
		pDlg->setCtrlReal(CtlPallet, (double)plt.PacksPerLayer * plt.MaxLayers);
		pDlg->setCtrlUInt16(CtlPalletCount, 0);
		pDlg->disableCtrl(CtlPallet, 0);
		pDlg->disableCtrl(CtlPalletCount, 0);
	}
	else {
		pDlg->setCtrlLong(CtlselPalletType, Data.PalletTypeID = 0);
		pDlg->setCtrlReal(CtlPallet, 0.0);
		pDlg->setCtrlUInt16(CtlPalletCount, 0);
		pDlg->disableCtrl(CtlPallet, 1);
		pDlg->disableCtrl(CtlPalletCount, 1);
	}
	pDlg->disableCtrl(CtlselPalletType, Gse.PltList.getCount() < 2);
	pDlg->setCtrlReal(CtlPckg, (Gse.Package > 0.0) ? Gse.Package : 0.0);
	pDlg->disableCtrl(CtlPckg, (Gse.Package <= 0.0));
	pDlg->disableCtrl(CtlPckgCount, (Gse.Package <= 0.0));
	if(Gse.Package <= 0.0)
		pDlg->setCtrlReal(CtlPckgCount, 0.0);
	return 1;
}

int PalletCtrlGroup::SetupGoods(TDialog * pDlg, PPID goodsID)
{
	Data.GoodsID = goodsID;
	if(Data.GoodsID)
		GObj.GetStockExt(Data.GoodsID, &Gse, 1);
	else
		Gse.Init();
	Data.PalletTypeID = 0;
	SetupPallet(pDlg, 1);
	return 1;
}

void PalletCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(cmInputUpdated)) {
		if(!(State & stInputUpdLock)) {
			State |= stInputUpdLock;
			if(event.isCtlEvent(CtlQtty))
				RecalcQtty(pDlg, CARGOUNIT_ITEM);
			else if(event.isCtlEvent(CtlPckgCount))
				RecalcQtty(pDlg, CARGOUNIT_PCKG);
			else if(event.isCtlEvent(CtlPalletCount))
				RecalcQtty(pDlg, CARGOUNIT_PALLET);
			State &= ~stInputUpdLock;
		}
	}
	else if(event.isCbSelected(CtlselGoods)) {
		SetupGoods(pDlg, pDlg->getCtrlLong(CtlselGoods));
	}
	else if(event.isCbSelected(CtlselPalletType)) {
		Data.PalletTypeID = pDlg->getCtrlLong(CtlselPalletType);
		SetupPallet(pDlg, 0);
	}
}

int PalletCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *(Rec *)pData;
	SetupPPObjCombo(pDlg, CtlselPalletType, PPOBJ_PALLET, Data.PalletTypeID, 0, 0);
	SetupGoods(pDlg, Data.GoodsID);
	pDlg->setCtrlUInt16(CtlPalletCount, (uint16)Data.PalletCount);
	pDlg->setCtrlReal(CtlQtty, Data.Qtty);
	return 1;
}

int PalletCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	Data.PalletTypeID = pDlg->getCtrlLong(CtlselPalletType);
	Data.PalletCount = pDlg->getCtrlUInt16(CtlPalletCount);
	Data.Qtty = pDlg->getCtrlReal(CtlQtty);
	ASSIGN_PTR((Rec *)pData, Data);
	return 1;
}


class LocTransfDialog : public TDialog {
public:
	enum {
		grpLoc = 1,
		grpGoods,
		grpPallet
	};
	LocTransfDialog(const PPBillPacket * pPack) : TDialog(DLG_LOCTRANSF)
	{
		State = 0;
		P_Pack = pPack;
		WarehouseID = P_Pack ? P_Pack->Rec.LocID : 0;
		addGroup(grpLoc, new LocationCtrlGroup(CTLSEL_LOCTRANSF_LOC, CTL_LOCTRANSF_LOCCODE, 0, 0, 0, LocationCtrlGroup::fWarehouseCell, 0));
		addGroup(grpGoods, new GoodsCtrlGroup(CTLSEL_LOCTRANSF_GGRP, CTLSEL_LOCTRANSF_GOODS));
		addGroup(grpPallet, new PalletCtrlGroup(CTLSEL_LOCTRANSF_PLTTYPE, CTL_LOCTRANSF_PLT, CTL_LOCTRANSF_PLTC,
			CTL_LOCTRANSF_PCKG, CTL_LOCTRANSF_PCKGC, CTL_LOCTRANSF_QTTY, CTLSEL_LOCTRANSF_GOODS));
		Ptb.SetBrush(brushIllSerial, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
	}
	int    setDTS(const LocTransfTbl::Rec * pData);
	int    getDTS(LocTransfTbl::Rec * pData);
private:
	DECL_HANDLE_EVENT;
	int    SetupGoodsAndLot();

	LocTransfTbl::Rec Data;
	const PPBillPacket * P_Pack;
	PPID   WarehouseID;
	enum {
		dummyFirst = 1,
		brushIllSerial // Кисть для индикации не идентифицированного серийного номера
	};
	SPaintToolBox Ptb;
	enum {
		stSerialUndef  = 0x0001 // Введенный в поле CTL_LOCTRANSF_SERIAL серийный номер не идентифицирован
	};
	long   State;
};

IMPL_HANDLE_EVENT(LocTransfDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmLot)) {
		GoodsCtrlGroup::Rec goodscg_rec;
		getGroupData(grpGoods, &goodscg_rec);
		if(goodscg_rec.GoodsID)
			ViewLots(goodscg_rec.GoodsID, WarehouseID, 0, 0, 0);
	}
	else if(event.isCmd(cmCtlColor)) {
		TDrawCtrlData * p_dc = (TDrawCtrlData *)TVINFOPTR;
		if(p_dc && getCtrlHandle(CTL_LOCTRANSF_SERIAL) == p_dc->H_Ctl && State & stSerialUndef) {
			::SetBkMode(p_dc->H_DC, TRANSPARENT);
			::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
			p_dc->H_Br = (HBRUSH)Ptb.Get(brushIllSerial);
		}
		else
			return;
	}
	else if(TVBROADCAST) {
		if(TVCMD == cmChangedFocus) {
			if(event.isCtlEvent(CTL_LOCTRANSF_SERIAL)) {
				SString sn_buf;
				getCtrlString(CTL_LOCTRANSF_SERIAL, sn_buf);
				if(sn_buf.NotEmptyS()) {
					ReceiptTbl::Rec lot_rec;
					if(BillObj->SelectLotBySerial(sn_buf, 0, WarehouseID, &lot_rec) > 0) {
						Data.GoodsID = lot_rec.GoodsID;
						PalletCtrlGroup * p_plt_grp = (PalletCtrlGroup *)getGroup(grpPallet);
						CALLPTRMEMB(p_plt_grp, SetupGoods(this, Data.GoodsID));
						Data.LotID = lot_rec.ID;
						SetupGoodsAndLot();
					}
					else
						State |= stSerialUndef;
				}
			}
			else
				return;
		}
		else
			return;
	}
	else
		return;
	clearEvent(event);
}

int LocTransfDialog::SetupGoodsAndLot()
{
	SString temp_buf;
	GoodsCtrlGroup::Rec goodscg_rec(0, Data.GoodsID, WarehouseID, GoodsCtrlGroup::disableEmptyGoods);
	setGroupData(grpGoods, &goodscg_rec);
	if(Data.LotID) {
		PPObjBill * p_bobj = BillObj;
		ReceiptTbl::Rec lot_rec;
		if(p_bobj->trfr->Rcpt.Search(Data.LotID, &lot_rec) > 0) {
			p_bobj->GetSerialNumberByLot(Data.LotID, temp_buf, 0);
			setCtrlString(CTL_LOCTRANSF_SERIAL, temp_buf);
			ReceiptCore::MakeCodeString(&lot_rec, 0, temp_buf);
		}
	}
	setCtrlString(CTL_LOCTRANSF_LOTINFO, temp_buf);
	return 1;
}

int LocTransfDialog::setDTS(const LocTransfTbl::Rec * pData)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	double bill_qtty = 0.0;
	SString temp_buf;
	Data = *pData;
	Data.Qtty = fabs(Data.Qtty);
	if(P_Pack) {
		p_bobj->MakeCodeString(&P_Pack->Rec, PPObjBill::mcsAddLocName, temp_buf);
		if(Data.BillID && Data.RByBill) {
			uint tipos = 0;
			if(P_Pack->SearchTI(Data.RByBill, &tipos))
				bill_qtty = P_Pack->ConstTI(tipos).Qtty();
		}
	}
	else if(Data.BillID && Data.RByBill) {
		BillTbl::Rec bill_rec;
		if(p_bobj->Search(Data.BillID, &bill_rec) > 0) {
			p_bobj->MakeCodeString(&bill_rec, PPObjBill::mcsAddLocName, temp_buf);
			int    rbybill = Data.RByBill-1;
			PPTransferItem ti;
			if(p_bobj->trfr->EnumItems(Data.BillID, &rbybill, &ti) > 0)
				bill_qtty = ti.Qtty();
		}
		else
			temp_buf.Z();
	}
	else
		temp_buf.Z();
	setStaticText(CTL_LOCTRANSF_BILLTITLE, temp_buf);
	setCtrlReal(CTL_LOCTRANSF_BILLQTTY, bill_qtty);
	showCtrl(CTL_LOCTRANSF_BILLQTTY, bill_qtty != 0.0);
	temp_buf.Z().Cat(Data.Dt).Space().Cat(Data.Tm);
	setCtrlString(CTL_LOCTRANSF_TM, temp_buf);
	{
		ObjIdListFilt loc_list;
		if(Data.LocID)
			loc_list.Add(Data.LocID);
		LocationCtrlGroup::Rec loccg_rec(&loc_list, WarehouseID);
		setGroupData(grpLoc, &loccg_rec);
	}
	AddClusterAssocDef(CTL_LOCTRANSF_OP,  0, LOCTRFROP_PUT);
	AddClusterAssoc(CTL_LOCTRANSF_OP,  1, LOCTRFROP_GET);
	AddClusterAssoc(CTL_LOCTRANSF_OP,  2, LOCTRFROP_INVENT);
	SetClusterData(CTL_LOCTRANSF_OP, Data.Op);
	SetupGoodsAndLot();
	{
		PalletCtrlGroup::Rec plt_rec;
		plt_rec.PalletTypeID = Data.PalletTypeID;
		plt_rec.PalletCount = Data.PalletCount;
		plt_rec.Qtty = Data.Qtty;
		plt_rec.GoodsID = Data.GoodsID;
		setGroupData(grpPallet, &plt_rec);
	}
	disableCtrl(CTL_LOCTRANSF_OP, BIN(pData->RByLoc));
	disableCtrl(CTL_LOCTRANSF_LOCCODE, BIN(pData->RByLoc));
	disableCtrl(CTLSEL_LOCTRANSF_LOC, BIN(pData->RByLoc));
	return ok;
}

int LocTransfDialog::getDTS(LocTransfTbl::Rec * pData)
{
	int    ok = 1;
	{
		LocationCtrlGroup::Rec loccg_rec;
		THROW(getGroupData(grpLoc, &loccg_rec));
		THROW(Data.LocID = loccg_rec.LocList.GetSingle());
	}
	GetClusterData(CTL_LOCTRANSF_OP, &Data.Op);
	{
		GoodsCtrlGroup::Rec goodscg_rec;
		getGroupData(grpGoods, &goodscg_rec);
		Data.GoodsID = goodscg_rec.GoodsID;
	}
	{
		PalletCtrlGroup::Rec plt_rec;
		getGroupData(grpPallet, &plt_rec);
		Data.PalletTypeID = plt_rec.PalletTypeID;
		Data.PalletCount = plt_rec.PalletCount;
		Data.Qtty = plt_rec.Qtty;
	}
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERR
	return ok;
}

int SLAPI EditLocTransf(const PPBillPacket * pPack, LocTransfTbl::Rec * pData)
{
	DIALOG_PROC_BODY_P1(LocTransfDialog, pPack, pData);
}
//
//
//
int PPViewLocTransf::DynCheckCellParent = 0;

SLAPI PPViewLocTransf::PPViewLocTransf() : PPView(0, &Filt, PPVIEW_LOCTRANSF)
{
	P_TempTbl = 0;
}

SLAPI PPViewLocTransf::~PPViewLocTransf()
{
	delete P_TempTbl;
}

PPBaseFilt * SLAPI PPViewLocTransf::CreateFilt(void * extraPtr) const
{
	LocTransfFilt * p_filt = new LocTransfFilt;
	return p_filt;
}

class LocTransfFiltDialog : public TDialog {
public:
	enum {
		grpLoc = 1,
		grpGoods
	};
	LocTransfFiltDialog() : TDialog(DLG_LOCTRFILT)
	{
 		addGroup(grpLoc, new LocationCtrlGroup(CTLSEL_LOCTRFILT_LOC, 0, 0, 0, 0, LocationCtrlGroup::fWarehouseCell|LocationCtrlGroup::fEnableSelUpLevel, 0));
 		addGroup(grpGoods, new GoodsCtrlGroup(CTLSEL_LOCTRFILT_GGRP, CTLSEL_LOCTRFILT_GOODS));
	}
	int    setDTS(const LocTransfFilt * pData)
	{
		int    ok = 1;
		Data = *pData;

		LocationCtrlGroup::Rec loccg_rec(&Data.LocList, 0);
		setGroupData(grpLoc, &loccg_rec);
		GoodsCtrlGroup::Rec goodscg_rec(Data.GoodsGrpID, Data.GoodsID, 0, 0);
		setGroupData(grpGoods, &goodscg_rec);
		if(Data.Mode == LocTransfFilt::modeDisposition) {
			LocationCtrlGroup * p_loc_grp = (LocationCtrlGroup *)getGroup(grpLoc);
			if(p_loc_grp)
				p_loc_grp->SetWarehouseCellMode(this, 0);
		}
		AddClusterAssocDef(CTL_LOCTRFILT_MODE, 0, LocTransfFilt::modeGeneral);
		AddClusterAssoc(CTL_LOCTRFILT_MODE, 1, LocTransfFilt::modeCurrent);
		AddClusterAssoc(CTL_LOCTRFILT_MODE, 2, LocTransfFilt::modeEmpty);
		AddClusterAssoc(CTL_LOCTRFILT_MODE, 3, LocTransfFilt::modeDisposition);
		SetClusterData(CTL_LOCTRFILT_MODE, Data.Mode);
		return ok;
	}
	int    getDTS(LocTransfFilt * pData)
	{
		int    ok = 1;
		LocationCtrlGroup::Rec loccg_rec;
		getGroupData(grpLoc, &loccg_rec);
		Data.LocList = loccg_rec.LocList;
		GoodsCtrlGroup::Rec goodscg_rec;
		getGroupData(grpGoods, &goodscg_rec);
		Data.GoodsGrpID = goodscg_rec.GrpID;
		Data.GoodsID = goodscg_rec.GoodsID;
		GetClusterData(CTL_LOCTRFILT_MODE, &Data.Mode);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_LOCTRFILT_MODE)) {
			long   prev_mode = Data.Mode;
			GetClusterData(CTL_LOCTRFILT_MODE, &Data.Mode);
			if(Data.Mode != prev_mode) {
				LocationCtrlGroup * p_loc_grp = (LocationCtrlGroup *)getGroup(grpLoc);
				if(p_loc_grp)
					p_loc_grp->SetWarehouseCellMode(this, (Data.Mode == LocTransfFilt::modeDisposition) ? 0 : 1);
			}
			clearEvent(event);
		}
		else
			return;
	}

	LocTransfFilt Data;
};

int SLAPI PPViewLocTransf::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	enum {
		grpLoc = 1,
		grpGoods
	};
	int    ok = -1;
	THROW(Filt.IsA(pBaseFilt));
	ok = PPDialogProcBody <LocTransfFiltDialog, LocTransfFilt> ((LocTransfFilt *)pBaseFilt);
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewLocTransf::UpdateTempRec(PPID tempRecID, PPID locID, int rByLoc, int use_ta)
{
	int    ok = 1;
	if(P_TempTbl) {
		TempLocTransfTbl::Rec temp_rec;
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		if(SearchByID_ForUpdate(P_TempTbl, 0, tempRecID, &temp_rec) > 0) {
			LocTransfTbl::Rec rec;
			if(Tbl.Search(locID, rByLoc, &rec) > 0) {
				MakeTempRec(rec, temp_rec);
			}
			else {
				temp_rec.LocID = 0;
				temp_rec.RByLoc = 0;
				temp_rec.DispQtty = 0.0;
				temp_rec.PalletTypeID = 0;
				temp_rec.PalletCount = 0;
				temp_rec.RestByGoods = 0.0;
				temp_rec.RestByLot = 0.0;
				temp_rec.Dt = ZERODATE;
				temp_rec.Tm = ZEROTIME;
				temp_rec.UserID = 0;
				temp_rec.Flags = 0;
			}
			THROW_DB(P_TempTbl->updateRecBuf(&temp_rec)); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

void SLAPI PPViewLocTransf::MakeTempRec(const LocTransfTbl::Rec & rRec, TempLocTransfTbl::Rec & rTempRec)
{
	rTempRec.Op = rRec.Op;
	rTempRec.LocID = rRec.LocID;
	rTempRec.RByLoc = rRec.RByLoc;
	rTempRec.GoodsID = rRec.GoodsID;
	rTempRec.DispQtty = rRec.Qtty;
	rTempRec.PalletTypeID = rRec.PalletTypeID;
	rTempRec.PalletCount = rRec.PalletCount;
	rTempRec.RestByGoods = rRec.RestByGoods;
	rTempRec.RestByLot = rRec.RestByLot;
	rTempRec.Dt = rRec.Dt;
	rTempRec.Tm = rRec.Tm;
	rTempRec.UserID = rRec.UserID;
	rTempRec.Flags = rRec.Flags;
}

int SLAPI PPViewLocTransf::ProcessDispBill(PPID billID, BExtInsert * pBei, int use_ta)
{
	int    ok = 1;
	BExtInsert * p_local_bei = 0;
	if(Filt.Mode == LocTransfFilt::modeDisposition && P_TempTbl) {
		if(pBei == 0) {
			THROW_MEM(p_local_bei = new BExtInsert(P_TempTbl));
			pBei = p_local_bei;
		}
		{
			PPObjBill * p_bobj = BillObj;
			Transfer * p_tr = p_bobj->trfr;
			TempLocTransfTbl::Rec rec;
			BillTbl::Rec bill_rec;
			LongArray seen_list;
			TSArray <LocTransfTbl::Rec> disp_list;
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			THROW(Tbl.GetDisposition(billID, disp_list));
			if(p_bobj->Search(billID, &bill_rec) > 0) {
				PPTransferItem ti;
				for(int rbb = 0; p_tr->EnumItems(billID, &rbb, &ti) > 0;) {
					int    disposed = 0;
					MEMSZERO(rec);
					rec.BillID = billID;
					rec.RByBill = rbb;
					if(ti.GetSign(bill_rec.OpID) == TISIGN_PLUS)
						rec.Op = LOCTRFROP_PUT;
					else if(ti.GetSign(bill_rec.OpID) == TISIGN_MINUS)
						rec.Op = LOCTRFROP_GET;
					rec.GoodsID = labs(ti.GoodsID);
					rec.BillQtty = ti.Qtty();
					for(uint j = 0; j < disp_list.getCount(); j++) {
						LocTransfTbl::Rec & r_lt_rec = disp_list.at(j);
						if(r_lt_rec.RByBill == rbb) {
							TempLocTransfTbl::Rec rec2;
							rec2 = rec;
							MakeTempRec(r_lt_rec, rec2);
							THROW_DB(pBei->insert(&rec2));
							THROW_SL(seen_list.add((long)j));
							disposed = 1;
						}
					}
					if(!disposed) {
						THROW_DB(pBei->insert(&rec));
					}
				}
			}
			//
			// Записи размещения, у которых не оказалось соответствия со строкой документа вставляем все равно.
			//
			for(uint j = 0; j < disp_list.getCount(); j++) {
				if(!seen_list.lsearch((long)j)) {
					LocTransfTbl::Rec & r_lt_rec = disp_list.at(j);
					MEMSZERO(rec);
					rec.BillID = billID;
					rec.RByBill = 0;
					MakeTempRec(r_lt_rec, rec);
					THROW_DB(pBei->insert(&rec));
				}
			}
			THROW_DB(pBei->flash());
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_local_bei;
	return ok;
}

int SLAPI PPViewLocTransf::Helper_BuildDispTable(int clearBefore, int use_ta)
{
	int    ok = -1;
	if(P_TempTbl) {
		BExtInsert bei(P_TempTbl);
		PPTransaction tra(ppDbDependTransaction, 1);
		THROW(tra);
		if(clearBefore)
			THROW_DB(deleteFrom(P_TempTbl, 0, *(DBQ *)0));
		for(uint i = 0; i < DispBillList.getCount(); i++) {
			THROW(ProcessDispBill(DispBillList.get(i), &bei, 0));
		}
		THROW(tra.Commit());
		ok = 1;
	}
	CATCHZOK
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempLocTransf);

int SLAPI PPViewLocTransf::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	ZDELETE(P_TempTbl);
	{
		if(Filt.LocList.IsExists()) {
			PPIDArray domain;
			LocObj.ResolveWhCellList(&Filt.LocList.Get(), 0, domain);
			Domain.Set(&domain);
		}
		else
			Domain.Set(0);
	}
	if(Filt.Mode == LocTransfFilt::modeDisposition) {
		DispBillList = Filt.BillList.Get();
		THROW(P_TempTbl = CreateTempFile());
		THROW(Helper_BuildDispTable(0, 1));
	}
	else if(Filt.Mode == LocTransfFilt::modeEmpty) {
		THROW(P_TempTbl = CreateTempFile());
		{
			TempLocTransfTbl::Rec temp_rec;
			PPIDArray list;
			Tbl.GetEmptyCellList(&Domain.Get(), &list);
			BExtInsert bei(P_TempTbl);
			{
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				for(uint i = 0; i < list.getCount(); i++) {
					MEMSZERO(temp_rec);
					temp_rec.LocID = list.get(i);
					THROW_DB(bei.insert(&temp_rec));
				}
				THROW_DB(bei.flash());
				THROW(tra.Commit());
			}
		}
	}
	else
		DispBillList.freeAll();
	CATCHZOK
	return ok;
}

int SLAPI PPViewLocTransf::InitIteration()
{
	int    ok = 1;
	int    idx = 0, sp = spFirst;
	union {
		TempLocTransfTbl::Key2 tk2;
		LocTransfTbl::Key0 k0;
		LocTransfTbl::Key1 k1;
		LocTransfTbl::Key2 k2;
		LocTransfTbl::Key5 k5;
	} k, k_;
	MEMSZERO(k);
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_TempTbl) {
		idx = 2;
		sp = spFirst;
		P_IterQuery = new BExtQuery(P_TempTbl, idx);
		P_IterQuery->selectAll();
	}
	else {
		const  PPID single_loc_id = Filt.LocList.GetSingle();
		DBQ * dbq = 0;
		if(Filt.LotID) {
			idx = 1;
			sp = spGe;
			k.k1.LotID = Filt.LotID;
			dbq = &(*dbq && Tbl.LotID == Filt.LotID);
		}
		else if(Filt.GoodsID) {
			idx = 2;
			sp = spGe;
			k.k2.GoodsID = Filt.GoodsID;
			dbq = &(*dbq && Tbl.GoodsID == Filt.GoodsID);
		}
		else if(Filt.Mode == LocTransfFilt::modeCurrent) {
			idx = 5;
			sp = spGe;
			k.k5.Op = 0;
		}
		else {
			idx = 0;
			sp = spFirst;
		}
		if(Filt.Mode = LocTransfFilt::modeCurrent)
			dbq	 = &(*dbq && Tbl.Op == 0L && Tbl.RestByGoods > 0.0);
		else
			dbq	 = &(*dbq && Tbl.Op > 0L);
		if(single_loc_id) {
			LocationTbl::Rec loc_rec;
			if(LocObj.Fetch(single_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WHCELL)
				dbq = &(*dbq && Tbl.LocID == single_loc_id);
		}
		P_IterQuery = new BExtQuery(&Tbl, idx);
		P_IterQuery->selectAll().where(*dbq);
	}
	k_ = k;
	Counter.Init(P_IterQuery->countIterations(0, &k_, sp));
	P_IterQuery->initIteration(0, &k, sp);
	return ok;
}

int FASTCALL PPViewLocTransf::NextIteration(LocTransfViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			Counter.Increment();
			if(Filt.Mode != Filt.modeDisposition) {
				const PPID cell_id = P_TempTbl ? P_TempTbl->data.LocID : Tbl.data.LocID;
				if(Domain.CheckID(cell_id)) {
					ok = 1;
				}
			}
			else {
				ok = 1;
			}
			if(ok > 0) {
				if(pItem) {
					memzero(pItem, sizeof(*pItem));
					if(P_TempTbl) {
						#define FLD(f) pItem->f = P_TempTbl->data.f
						FLD(LocID);
						FLD(RByLoc);
						FLD(Dt);
						FLD(Tm);
						FLD(UserID);
						FLD(BillID);
						FLD(RByBill);
						FLD(Op);
						FLD(Flags);
						FLD(GoodsID);
						//FLD(LotID);
						FLD(RestByGoods);
						FLD(RestByLot);
						FLD(PalletTypeID);
						FLD(PalletCount);
						pItem->Qtty = P_TempTbl->data.DispQtty;
						pItem->BillQtty = P_TempTbl->data.BillQtty;
						pItem->TempID__ = P_TempTbl->data.ID__;
						#undef FLD
					}
					else {
						*((LocTransfTbl::Rec *)pItem) = Tbl.data;
					}
				}
			}
		}
	}
	return ok;
}

static void SLAPI dbqf_checkcellparent_ii(int option, DBConst * result, DBConst * params)
{
	long   r = 0;
	const PPIDArray * p_cell_list = (const PPIDArray *)params[1].lval;
	if(!p_cell_list || p_cell_list->bsearch(params[0].lval))
		r = 1;
	result->init(r);
}

DBQuery * SLAPI PPViewLocTransf::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	// LocTransfFilt
	static DbqStringSubst optype_subst(3);  // @global @threadsafe

	uint   brw_id = BROWSER_LOCTRANSF;
	DBQuery * p_q = 0;
	DBE    dbe_loc;      // Наименование ячейки
	DBE    dbe_goods;    // Наименование товара
	DBE    dbe_user;     // Наименование пользователя, создавшего запись
	DBE    dbe_barcode;  // Штрихкод товара
	DBE  * p_dbe_op = 0; // Наименование операции
	DBE    dbe_bill;     // Номер и дата документа
	DBE    dbe_chkloc;   // Проверка критерия Filt.LocID
	DBQ  * dbq = 0;
	int    single_loc_crit = 0;
	LocTransfTbl * t = 0;
	TempLocTransfTbl * tmpt = 0;
	if(P_TempTbl) {
		if(Filt.Mode == LocTransfFilt::modeDisposition)
			brw_id = BROWSER_LOCTRANSF_DISP;
		else if(Filt.Mode == LocTransfFilt::modeEmpty)
			brw_id = BROWSER_LOCTRANSF_EMPTY;
		tmpt = new TempLocTransfTbl(P_TempTbl->fileName);
		PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc,   tmpt->LocID);
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, tmpt->GoodsID);
		PPDbqFuncPool::InitObjNameFunc(dbe_user,  PPDbqFuncPool::IdObjNameUser,  tmpt->UserID);
		PPDbqFuncPool::InitObjNameFunc(dbe_bill,  PPDbqFuncPool::IdObjCodeBillCmplx, tmpt->BillID);
		PPDbqFuncPool::InitObjNameFunc(dbe_barcode, PPDbqFuncPool::IdGoodsSingleBarcode, tmpt->GoodsID);
		p_dbe_op = & enumtoa(tmpt->Op, 3, optype_subst.Get(PPTXT_LOCTRANSF_OPTEXT));
		p_q = &select(
			tmpt->ID__,         // #00
			tmpt->LocID,        // #01
			tmpt->RByLoc,       // #02
			tmpt->Op,           // #03
			tmpt->DispQtty,     // #04
			tmpt->RestByGoods,  // #05
			tmpt->RestByLot,    // #06
			tmpt->Dt,           // #07
			tmpt->Tm,           // #08
			*p_dbe_op,          // #09
			dbe_loc,            // #10
			dbe_goods,          // #11
			dbe_bill,           // #12
			dbe_user,           // #13
			dbe_barcode,        // #14
			tmpt->BillQtty,     // #15
			0).from(tmpt, 0L).where(*dbq);
	}
	else {
		t = new LocTransfTbl;
		PPDbqFuncPool::InitObjNameFunc(dbe_loc,   PPDbqFuncPool::IdObjNameLoc,   t->LocID);
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, t->GoodsID);
		PPDbqFuncPool::InitObjNameFunc(dbe_user,  PPDbqFuncPool::IdObjNameUser,  t->UserID);
		PPDbqFuncPool::InitObjNameFunc(dbe_bill,  PPDbqFuncPool::IdObjCodeBillCmplx, t->BillID);
		PPDbqFuncPool::InitObjNameFunc(dbe_barcode, PPDbqFuncPool::IdGoodsSingleBarcode, t->GoodsID);
		p_dbe_op = & enumtoa(t->Op, 3, optype_subst.Get(PPTXT_LOCTRANSF_OPTEXT));
		{
			const  PPID single_loc_id = Filt.LocList.GetSingle();
			LocationTbl::Rec loc_rec;
			if(single_loc_id && LocObj.Fetch(single_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WHCELL) {
				dbq = &(*dbq && t->LocID == single_loc_id);
				single_loc_crit = 1;
			}
			else if(Filt.LocList.GetCount() > 0) {
				if(DbqFuncTab::RegisterDyn(&DynCheckCellParent, 0, BTS_INT, dbqf_checkcellparent_ii, 2, BTS_INT, BTS_INT)) {
					CellList.clear();
					PPIDArray temp_cell_list;
					for(uint i = 0; i < Filt.LocList.GetCount(); i++) {
						const PPID loc_id = Filt.LocList.Get().get(i);
						LocObj.ResolveWhCell(loc_id, temp_cell_list, 0, 1);
						CellList.addUnique(&temp_cell_list);
					}
					CellList.sort();
					dbe_chkloc.init();
					dbe_chkloc.push(t->LocID);
					DBConst dbc_long;
					dbc_long.init((long)&CellList);
					dbe_chkloc.push(dbc_long);
					dbe_chkloc.push((DBFunc)DynCheckCellParent);
					dbq = & (*dbq && dbe_chkloc == (long)1);
				}
			}
		}
		if(Filt.LotID)
			dbq = &(*dbq && t->LotID == Filt.LotID);
		else {
			dbq = ppcheckfiltid(dbq, t->GoodsID, Filt.GoodsID);
		}
		if(Filt.Mode == LocTransfFilt::modeCurrent)
			dbq	 = &(*dbq && t->Op == 0L && t->RestByGoods > 0.0);
		else
			dbq	 = &(*dbq && t->Op > 0L);
		p_q = &select(
			t->LocID,        // #00 @stub
			t->LocID,        // #01
			t->RByLoc,       // #02
			t->Op,           // #03
			t->Qtty,         // #04
			t->RestByGoods,  // #05
			t->RestByLot,    // #06
			t->Dt,           // #07
			t->Tm,           // #08
			*p_dbe_op,       // #09
			dbe_loc,         // #10
			dbe_goods,       // #11
			dbe_bill,        // #12
			dbe_user,        // #13
			dbe_barcode,     // #14
			t->Qtty,         // #15 @stub
			0).from(t, 0L).where(*dbq);
		if(single_loc_crit)
			p_q->orderBy(t->LocID, 0);
		else if(Filt.LotID)
			p_q->orderBy(t->LotID, 0);
	}
	ASSIGN_PTR(pBrwId, brw_id);
	return p_q;
}

int SLAPI PPViewLocTransf::AddItem(PPID curLocID, long curRByLoc)
{
	int    ok = -1;
	if(Filt.Mode == LocTransfFilt::modeGeneral) {
		LocTransfTbl::Rec rec;
		MEMSZERO(rec);
		rec.LocID = NZOR(curLocID, Filt.LocList.GetSingle());
		ok = EditLocTransf(0, &rec);
		if(ok > 0) {
			LocTransfOpBlock blk(rec.Op, rec.LocID);
			blk.GoodsID = rec.GoodsID;
			blk.LotID = rec.LotID;
			blk.PalletTypeID = rec.PalletTypeID;
			blk.PalletCount = rec.PalletCount;
			blk.Qtty = fabs(rec.Qtty);
			ok = Tbl.PutOp(blk, 0, 1) ? 1 : PPErrorZ();
		}
 	}
	else if(Filt.Mode == LocTransfFilt::modeDisposition) {
		BillFilt bill_filt;
		LDATE cd = getcurdate_();
		bill_filt.Period.Set(plusdate(cd, -365), plusdate(cd, 2));
		bill_filt.LocList = Filt.LocList;
		bill_filt.Flags |= BillFilt::fAsSelector;
		PPViewBill bill_view;
		if(bill_view.Init_(&bill_filt)) {
			while(ok < 0 && bill_view.Browse(0) > 0) {
				PPID bill_id = ((BillFilt*)bill_view.GetBaseFilt())->Sel;
				if(DispBillList.addUnique(bill_id) > 0) {
					if(ProcessDispBill(bill_id, 0, 1))
						ok = 1;
					else
						ok = PPErrorZ();
				}
			}
		}
	}
	else if(Filt.Mode == LocTransfFilt::modeCurrent) {
		LocTransfTbl::Rec rec;
		MEMSZERO(rec);
		rec.LocID = NZOR(curLocID, Filt.LocList.GetSingle());
		rec.Op = LOCTRFROP_GET;
		if(curLocID && curRByLoc) {
			LocTransfTbl::Rec cur_rec;
			if(Tbl.Search(curLocID, curRByLoc, &cur_rec) > 0)
				rec.GoodsID = cur_rec.GoodsID;
		}
		ok = EditLocTransf(0, &rec);
		if(ok > 0) {
			LocTransfOpBlock blk(rec.Op, rec.LocID);
			blk.GoodsID = rec.GoodsID;
			blk.LotID = rec.LotID;
			blk.PalletTypeID = rec.PalletTypeID;
			blk.PalletCount = rec.PalletCount;
			blk.Qtty = fabs(rec.Qtty);
			ok = Tbl.PutOp(blk, 0, 1) ? 1 : PPErrorZ();
		}
	}
	else if(Filt.Mode == LocTransfFilt::modeEmpty) {
		LocTransfTbl::Rec rec;
		MEMSZERO(rec);
		rec.LocID = NZOR(curLocID, Filt.LocList.GetSingle());
		rec.Op = LOCTRFROP_PUT;
		ok = EditLocTransf(0, &rec);
		if(ok > 0) {
			LocTransfOpBlock blk(rec.Op, rec.LocID);
			blk.GoodsID = rec.GoodsID;
			blk.LotID = rec.LotID;
			blk.PalletTypeID = rec.PalletTypeID;
			blk.PalletCount = rec.PalletCount;
			blk.Qtty = fabs(rec.Qtty);
			ok = Tbl.PutOp(blk, 0, 1) ? 1 : PPErrorZ();
		}
	}
	return ok;
}

int SLAPI PPViewLocTransf::EditItem(PPID tempRecID, PPID curLocID, long curRByLoc)
{
	int    ok = -1;
	LocTransfTbl::Rec rec;
	if(Filt.Mode == LocTransfFilt::modeGeneral) {
		if(Tbl.Search(curLocID, curRByLoc, &rec) > 0) {
			ok = EditLocTransf(0, &rec);
			if(ok > 0) {
				LocTransfOpBlock blk(rec.Op, rec.LocID);
				blk.GoodsID = rec.GoodsID;
				blk.LotID = rec.LotID;
				blk.LocID  = curLocID;
				blk.RByLoc = curRByLoc;
				blk.PalletTypeID = rec.PalletTypeID;
				blk.PalletCount = rec.PalletCount;
				blk.Qtty = fabs(rec.Qtty);
				THROW(Tbl.PutOp(blk, 0, 1));
				ok = 1;
			}
		}
	}
	else if(Filt.Mode == LocTransfFilt::modeDisposition) {
		if(P_TempTbl) {
			TempLocTransfTbl::Rec temp_rec;
			if(SearchByID(P_TempTbl, 0, tempRecID, &temp_rec) > 0) {
				PPBillPacket pack, * p_pack = 0;
				if(temp_rec.BillID) {
					THROW(BillObj->ExtractPacket(temp_rec.BillID, &pack) > 0);
					p_pack = &pack;
				}
				if(temp_rec.LocID && Tbl.Search(temp_rec.LocID, temp_rec.RByLoc, &rec) > 0) {
					ok = EditLocTransf(p_pack, &rec);
					if(ok > 0) {
						LocTransfOpBlock blk(rec.Op, rec.LocID);
						PPTransaction tra(1);
						THROW(tra);
						blk.BillID = rec.BillID;
						blk.RByBill = rec.RByBill;
						blk.GoodsID = rec.GoodsID;
						blk.LotID = rec.LotID;
						blk.LocID  = temp_rec.LocID;
						blk.RByLoc = temp_rec.RByLoc;
						blk.PalletTypeID = rec.PalletTypeID;
						blk.PalletCount = rec.PalletCount;
						blk.Qtty = fabs(rec.Qtty);
						THROW(Tbl.PutOp(blk, 0, 0));
						THROW(UpdateTempRec(tempRecID, blk.LocID, blk.RByLoc, 0));
						THROW(tra.Commit());
						ok = 1;
					}
				}
				else {
					MEMSZERO(rec);
					rec.BillID = temp_rec.BillID;
					rec.RByBill = temp_rec.RByBill;
					rec.Op = temp_rec.Op;
					if(p_pack) {
						uint tipos = 0;
						if(p_pack->SearchTI(rec.RByBill, &tipos)) {
							const PPTransferItem & r_ti = p_pack->ConstTI(tipos);
							rec.GoodsID = labs(r_ti.GoodsID);
							if(rec.Op == LOCTRFROP_PUT)
								rec.LotID = r_ti.LotID;
						}
					}
					ok = EditLocTransf(p_pack, &rec);
					if(ok > 0) {
						LocTransfOpBlock blk(rec.Op, rec.LocID);
						PPTransaction tra(1);
						THROW(tra);
						blk.BillID = rec.BillID;
						blk.RByBill = rec.RByBill;
						blk.GoodsID = rec.GoodsID;
						blk.LotID = rec.LotID;
						blk.PalletTypeID = rec.PalletTypeID;
						blk.PalletCount = rec.PalletCount;
						blk.Qtty = fabs(rec.Qtty);
						int    rbyloc = 0;
						THROW(Tbl.PutOp(blk, &rbyloc, 0));
						THROW(UpdateTempRec(tempRecID, blk.LocID, rbyloc, 0));
						THROW(tra.Commit());
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewLocTransf::DeleteItem(PPID tempRecID, PPID curLocID, long curRByLoc)
{
	int    ok = -1;
	if(Filt.Mode == LocTransfFilt::modeGeneral) {
		if(curLocID && curRByLoc) {
			THROW(Tbl.RemoveOp(curLocID, curRByLoc, 1));
			ok = 1;
		}
	}
	else if(Filt.Mode == LocTransfFilt::modeDisposition) {
		if(P_TempTbl && tempRecID) {
			TempLocTransfTbl::Rec temp_rec;
			if(SearchByID(P_TempTbl, 0, tempRecID, &temp_rec) > 0) {
				if(temp_rec.LocID && temp_rec.RByLoc) {
					PPTransaction tra(1);
					THROW(tra);
					THROW(Tbl.RemoveOp(temp_rec.LocID, temp_rec.RByLoc, 0));
					THROW(UpdateTempRec(tempRecID, temp_rec.LocID, temp_rec.RByLoc, 0));
					THROW(tra.Commit());
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewLocTransf::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	Hdr    hdr;
	if(pHdr)
		hdr = *(Hdr *)pHdr;
	else
		MEMSZERO(hdr);
	if(hdr.LocID) {
		LocTransfFilt filt;
		filt.Mode = LocTransfFilt::modeGeneral;
		filt.LocList.Add(hdr.LocID);
		if(!PPView::Execute(PPVIEW_LOCTRANSF, &filt, 1, 0))
			ok = 0;
	}
	return ok;
}

int SLAPI PPViewLocTransf::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		Hdr    hdr;
		if(pHdr)
			hdr = *(Hdr *)pHdr;
		else
			MEMSZERO(hdr);
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = AddItem(hdr.LocID, hdr.RByLoc);
				break;
			case PPVCMD_EDITITEM:
				ok = EditItem(hdr.ID__, hdr.LocID, hdr.RByLoc);
				break;
			case PPVCMD_DELETEITEM:
				ok = DeleteItem(hdr.ID__, hdr.LocID, hdr.RByLoc);
				break;
			case PPVCMD_EDITWHCELL:
				ok = -1;
				{
					PPID   loc_id = hdr.LocID;
					if(loc_id) {
						if(LocObj.Edit(&loc_id, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_DISPOSE:
				ok = -1;
				if(Filt.Mode == LocTransfFilt::modeDisposition) {
					LocTransfDisposer disposer;
					PPLogger logger;
					int r = disposer.Dispose(DispBillList, &logger, 1);
					if(r > 0)
						ok = Helper_BuildDispTable(1, 1) ? 1 : PPErrorZ();
					else if(r == 0)
						ok = PPErrorZ();
				}
				break;
			case PPVCMD_EDITGOODS:
				ok = -1;
				{
					PPID goods_id = 0;
					if(P_TempTbl) {
						TempLocTransfTbl::Rec temp_rec;
						if(SearchByID(P_TempTbl, 0, hdr.ID__, &temp_rec) > 0)
							goods_id = temp_rec.GoodsID;
					}
					else {
						LocTransfTbl::Rec rec;
						if(Tbl.Search(hdr.LocID, hdr.RByLoc, &rec) > 0)
							goods_id = rec.GoodsID;
					}
					if(goods_id) {
						PPObjGoods goods_obj;
						if(goods_obj.Edit(&goods_id, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_EDITBILL:
				ok = -1;
				{
					PPID   bill_id = 0;
					if(P_TempTbl) {
						TempLocTransfTbl::Rec temp_rec;
						if(SearchByID(P_TempTbl, 0, hdr.ID__, &temp_rec) > 0)
							bill_id = temp_rec.BillID;
					}
					else {
						LocTransfTbl::Rec rec;
						if(Tbl.Search(hdr.LocID, hdr.RByLoc, &rec) > 0)
							bill_id = rec.BillID;
					}
					if(bill_id) {
						int    r = BillObj->Edit(&bill_id, 0);
						if(r == 0)
							ok = PPErrorZ();
						else if(r > 0)
							ok = 1;
					}
				}
				break;
		}
	}
	return ok;
}
//
// Implementation of PPALDD_LocTransfView
//
PPALDD_CONSTRUCTOR(LocTransfView)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(LocTransfView)
{
	Destroy();
}

int PPALDD_LocTransfView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(LocTransf, rsrv);
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltGoodsID = p_filt->GoodsID;
	H.FltLotID = p_filt->LotID;
	H.Mode = p_filt->Mode;
	H.MoveOp = p_filt->MoveOp;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_LocTransfView::InitIteration(long iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(LocTransf);
}

int PPALDD_LocTransfView::NextIteration(long iterId)
{
	START_PPVIEW_ALDD_ITER(LocTransf);
	I.TempID__ = item.TempID__;
	I.CellID = item.LocID;
	I.RByLoc = item.RByLoc;
	I.Dt = item.Dt;
	I.Tm = item.Tm;
	I.UserID = item.UserID;
	I.BillID = item.BillID;
	I.RByBill = item.RByBill;
	I.Op = item.Op;
	I.Flags = item.Flags;
	I.GoodsID = item.GoodsID;
	I.LotID = item.LotID;
	I.PalletTypeID = item.PalletTypeID;
	I.PalletCount = item.PalletCount;
	I.RestByGoods = item.RestByGoods;
	I.RestByLot = item.RestByLot;
	I.Qtty = item.Qtty;
	I.BillQtty = item.BillQtty;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_LocTransfView::Destroy()
{
	DESTROY_PPVIEW_ALDD(LocTransf);
}
