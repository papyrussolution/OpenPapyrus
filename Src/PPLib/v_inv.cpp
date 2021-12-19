// V_INV.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
// PPViewInventory: отображение строк инвентаризации
//
#include <pp.h>
#pragma hdrstop

/*static*/int PPViewInventory::DynFuncInvLnWrOff = 0;

IMPLEMENT_PPFILT_FACTORY(Inventory); InventoryFilt::InventoryFilt() : PPBaseFilt(PPFILT_INVENTORY, 0, 1)
{
	SetFlatChunk(offsetof(InventoryFilt, ReserveStart),
		offsetof(InventoryFilt, Reserve)-offsetof(InventoryFilt, ReserveStart)+sizeof(Reserve));
	SetBranchObjIdListFilt(offsetof(InventoryFilt, BillList));
	SetBranchObjIdListFilt(offsetof(InventoryFilt, GoodsList));
	Init(1, 0);
}

InventoryFilt & FASTCALL InventoryFilt::operator = (const InventoryFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

void FASTCALL InventoryFilt::SetSingleBillID(PPID billID) { BillList.SetSingle(billID); }
PPID InventoryFilt::GetSingleBillID() const { return BillList.GetSingle(); }
int  InventoryFilt::HasSubst() const { return BIN(!!Sgb || Sgg); }

void InventoryFilt::Setup(PPID billID)
{
	Init(1, 0);
	SetSingleBillID(billID);
}

PPViewInventory::ExtraEntry::ExtraEntry()
{
	THISZERO();
}

PPViewInventory::PPViewInventory() : PPView(0, &Filt, PPVIEW_INVENTORY, 0, 0), P_BObj(BillObj), P_TempTbl(0), P_TempOrd(0),
	P_TempSubstTbl(0), P_GgIter(0), P_GIter(0), Flags(0), CommonLocID(0), CommonDate(ZERODATE), LastSurrID(0), P_OuterPack(0)
{
}

PPViewInventory::~PPViewInventory()
{
	delete P_TempOrd;
	delete P_TempTbl;
	delete P_TempSubstTbl;
	delete P_GgIter;
	delete P_GIter;
}

void PPViewInventory::SetOuterPack(PPBillPacket * pPack) { P_OuterPack = pPack; }
int  PPViewInventory::GetZeroByDefaultStatus() const { return BIN(Flags & fIsZeroByDefault); }
int  PPViewInventory::GetUpdateStatus() const { return BIN(Flags & fWasUpdated); }

class InventoryFiltDialog : public TDialog {
	DECL_DIALOG_DATA(InventoryFilt);
	enum {
		ctlgroupGoods = 1
	};
public:
	InventoryFiltDialog() : TDialog(DLG_INVDIFFLT)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_INVDIFFLT_GRP, CTLSEL_INVDIFFLT_GOODS));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		ushort v;
		RVALUEPTR(Data, pData);
		{
			GoodsCtrlGroup::Rec rec;
			rec.GrpID   = Data.GoodsGrpID;
			rec.GoodsID = Data.GoodsID;
			rec.Flags   = GoodsCtrlGroup::enableSelUpLevel;
			setGroupData(ctlgroupGoods, &rec);
		}
		{
			const PPID strg_loc_id = Data.StorageLocID;
			LocationFilt loc_filt(LOCTYP_WAREPLACE, 0, 0);
			SetupLocationCombo(this, CTLSEL_INVDIFFLT_STRGLOC, strg_loc_id, OLW_CANSELUPLEVEL, &loc_filt);
		}
		long   f = Data.Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus);
		v = (f == 0) ? 0 : ((f == InventoryFilt::fLack) ? 1 : ((f == InventoryFilt::fSurplus) ? 2 : 3));
		setCtrlUInt16(CTL_INVDIFFLT_SIGN, v);
		setCtrlUInt16(CTL_INVDIFFLT_UNIT,   BIN(Data.Flags & InventoryFilt::fAmtVal));
		setCtrlUInt16(CTL_INVDIFFLT_PCTVAL, BIN(Data.Flags & InventoryFilt::fPctVal));
		SetRealRangeInput(this, CTL_INVDIFFLT_VAL, Data.MinVal, Data.MaxVal);
		f = CheckXORFlags(Data.Flags, InventoryFilt::fUnwrOff, InventoryFilt::fWrOff);
		v = (f == InventoryFilt::fUnwrOff) ? 1 : ((f == InventoryFilt::fWrOff) ? 2 : 0);
		setCtrlUInt16(CTL_INVDIFFLT_WROFF, v);
		AddClusterAssocDef(CTL_INVDIFFLT_ORD,  0, InventoryFilt::ordByDefault);
		AddClusterAssoc(CTL_INVDIFFLT_ORD,  1, InventoryFilt::ordByGoods);
		AddClusterAssoc(CTL_INVDIFFLT_ORD,  2, InventoryFilt::ordByDeficit);
		AddClusterAssoc(CTL_INVDIFFLT_ORD,  3, InventoryFilt::ordByDiff);
		SetClusterData(CTL_INVDIFFLT_ORD, (long)Data.SortOrder);
		SetupSubstBillCombo(this, CTLSEL_INVDIFFLT_SUBST, Data.Sgb);
		SetupSubst();
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort v = 0;
		long   f = 0;
		{
			GoodsCtrlGroup::Rec rec;
			getGroupData(ctlgroupGoods, &rec);
			Data.GoodsGrpID = rec.GrpID;
			Data.GoodsID    = rec.GoodsID;
			//getCtrlData(CTLSEL_INVDIFFLT_GRP, &Data.GoodsGrpID);
		}
		getCtrlData(CTLSEL_INVDIFFLT_STRGLOC, &Data.StorageLocID);
		getCtrlData(CTL_INVDIFFLT_SIGN, &(v = 0));
		if(v == 0)
			f = 0;
		else if(v == 1)
			f = InventoryFilt::fLack;
		else if(v == 2)
			f = InventoryFilt::fSurplus;
		else
			f = InventoryFilt::fLack | InventoryFilt::fSurplus;
		Data.Flags &= ~(InventoryFilt::fLack | InventoryFilt::fSurplus);
		Data.Flags |= f;
		getCtrlData(CTL_INVDIFFLT_UNIT, &(v = 0));
		SETFLAG(Data.Flags, InventoryFilt::fAmtVal, v == 1);
		getCtrlData(CTL_INVDIFFLT_PCTVAL, &(v = 0));
		SETFLAG(Data.Flags, InventoryFilt::fPctVal, v);
		GetRealRangeInput(this, CTL_INVDIFFLT_VAL, &Data.MinVal, &Data.MaxVal);
		getCtrlData(CTL_INVDIFFLT_WROFF, &(v = 0));
		if(v == 1)
			f = InventoryFilt::fUnwrOff;
		else if(v == 2)
			f = InventoryFilt::fWrOff;
		else
			f = 0;
		Data.Flags = SetXORFlags(Data.Flags, InventoryFilt::fUnwrOff, InventoryFilt::fWrOff, f);
		Data.SortOrder = static_cast<int16>(GetClusterData(CTL_INVDIFFLT_ORD));
		Data.Sgb.S = static_cast<SubstGrpBill::_S>(getCtrlLong(CTLSEL_INVDIFFLT_SUBST));
		if(!Data.Sgb)
			Data.Sgg = static_cast<SubstGrpGoods>(getCtrlLong(CTLSEL_INVDIFFLT_SUBSTG));
		else {
			if(Data.Sgb.S == SubstGrpBill::sgbDate) {
				Data.Sgb.S2.Sgd = static_cast<SubstGrpDate>(getCtrlLong(CTLSEL_INVDIFFLT_SUBSTG));
			}
			else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject,  SubstGrpBill::sgbObject2,
				SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer)) {
				Data.Sgb.S2.Sgp = static_cast<SubstGrpPerson>(getCtrlLong(CTLSEL_INVDIFFLT_SUBSTG));
			}
			else
				Data.Sgb.S2.Sgp = sgpNone;
		}
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_INVDIFFLT_SUBST)) {
			Data.Sgb.S = static_cast<SubstGrpBill::_S>(getCtrlLong(CTLSEL_INVDIFFLT_SUBST));
			SetupSubst();
		}
		else if(event.isCbSelected(CTLSEL_INVDIFFLT_SUBSTG)) {
			if(!Data.Sgb) {
				Data.Sgg = static_cast<SubstGrpGoods>(getCtrlLong(CTLSEL_INVDIFFLT_SUBSTG));
				SetupSubst();
			}
		}
		else
			return;
		clearEvent(event);
	}
	void SetupSubst()
	{
		int    dsbl_bill_subst = 0;
		int    dsbl_sec_subst = 0;
		SString label_buf;
		if(!!Data.Sgb) {
			Data.Sgg = sggNone;
			if(Data.Sgb.S == SubstGrpBill::sgbDate) {
				PPLoadString("date", label_buf);
				SetupSubstDateCombo(this, CTLSEL_INVDIFFLT_SUBSTG, (long)Data.Sgb.S2.Sgd);
				dsbl_sec_subst = 0;
			}
			else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject,  SubstGrpBill::sgbObject2, SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer)) {
				PPLoadString("person", label_buf);
				SetupSubstPersonCombo(this, CTLSEL_INVDIFFLT_SUBSTG, Data.Sgb.S2.Sgp);
				dsbl_sec_subst = 0;
			}
			else {
				setLabelText(CTL_INVDIFFLT_SUBSTG, label_buf.Z());
				dsbl_sec_subst = 1;
			}
		}
		else {
			PPLoadString("ware", label_buf);
			SetupSubstGoodsCombo(this, CTLSEL_INVDIFFLT_SUBSTG, Data.Sgg);
			if(Data.Sgg) {
				Data.Sgb.Reset();
				dsbl_bill_subst = 1;
			}
		}
		setLabelText(CTL_INVDIFFLT_SUBSTG, label_buf);
		disableCtrl(CTLSEL_INVDIFFLT_SUBST, dsbl_bill_subst);
		disableCtrl(CTLSEL_INVDIFFLT_SUBSTG, dsbl_sec_subst);
	}
};

int PPViewInventory::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1, valid_data = 0;
	InventoryFilt * p_filt = static_cast<InventoryFilt *>(pFilt);
	InventoryFiltDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new InventoryFiltDialog()))) {
		dlg->setDTS(p_filt);
		while(!valid_data && ExecView(dlg) == cmOK) {
			if(dlg->getDTS(p_filt)) {
				ok = valid_data = 1;
			}
		}
		delete dlg;
	}
	else
		ok = 0;
	return ok;
}

static int CheckInventoryLineForWrOff(long fltFlags, PPID billID, long lnFlags, double diffQtty)
{
	int    ok = 1;
	const  long   f = (fltFlags & (InventoryFilt::fWrOff | InventoryFilt::fUnwrOff));
	if(oneof2(f, InventoryFilt::fWrOff, InventoryFilt::fUnwrOff)) {
		BillTbl::Rec bill_rec;
		const int is_bill_wroff = BIN(BillObj->Fetch(billID, &bill_rec) && bill_rec.Flags & BILLF_CLOSEDORDER);
		if(f == InventoryFilt::fWrOff) {
			if(!(lnFlags & INVENTF_WRITEDOFF)) {
				if(!is_bill_wroff || /*diffQtty != 0.0*/(lnFlags & (INVENTF_LACK|INVENTF_SURPLUS))) // @v10.5.9
					ok = 0;
			}
		}
		else if(f == InventoryFilt::fUnwrOff) {
			if(lnFlags & INVENTF_WRITEDOFF || (is_bill_wroff && /*diffQtty == 0.0*/!(lnFlags & (INVENTF_LACK|INVENTF_SURPLUS))))
				ok = 0;
		}
	}
	return ok;
}

int PPViewInventory::CheckLineForFilt(const InventoryTbl::Rec * pRec) const
{
	if(!Filt.BillList.CheckID(pRec->BillID))
		return 0;
	if(!CheckInventoryLineForWrOff(Filt.Flags, pRec->BillID, pRec->Flags, pRec->DiffQtty))
		return 0;
	else if(Filt.Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
		if(!(Filt.Flags & InventoryFilt::fLack)) {
			if(INVENT_DIFFSIGN(pRec->Flags) <= 0)
				return 0;
		}
		else if(!(Filt.Flags & InventoryFilt::fSurplus)) {
			if(INVENT_DIFFSIGN(pRec->Flags) >= 0)
				return 0;
		}
		else if(pRec->DiffQtty == 0)
			return 0;
		double minv = Filt.MinVal;
		double maxv = Filt.MaxVal;
		if(maxv >= minv && maxv > 0) {
			if(minv < 0.0)
				minv = 0.0;
			if(Filt.Flags & InventoryFilt::fPctVal) {
				if(minv == maxv) {
					minv = MAX(0.0, minv - 1.0);
					maxv = maxv + 1.0;
				}
				if(pRec->DiffPctQtty < minv || pRec->DiffPctQtty > maxv)
					return 0;
			}
			else if(pRec->DiffQtty < minv || pRec->DiffQtty > maxv)
				return 0;
		}
	}
	return 1;
}

int PPViewInventory::UpdateTempTable(PPID billID, long oprno)
{
	int    ok = 1;
	{
		InventoryTbl::Rec rec;
		PPTransaction tra(ppDbDependTransaction, BIN(P_TempTbl || P_TempOrd));
		THROW(tra);
		const PPID bill_id = billID;
		if(InvTbl.Search(bill_id, oprno, &rec) > 0) {
			{
				int    is_extra_entry_found = 0;
				for(uint i = 0; i < ExtraList.getCount(); i++) {
					ExtraEntry & r_entry = ExtraList.at(i);
					if(r_entry.BillID == billID && r_entry.OprNo == oprno) {
						r_entry.GoodsID = rec.GoodsID;
						is_extra_entry_found = 1;
					}
				}
				if(!is_extra_entry_found) {
					ExtraEntry new_entry;
					PPFreight freight;
					// @v10.9.12 @ctr MEMSZERO(new_entry);
					new_entry.SurrID = ++LastSurrID;
					new_entry.BillID = rec.BillID;
					new_entry.OprNo = rec.OprNo;
					new_entry.GoodsID = rec.GoodsID;
					new_entry.StorageLocID = (P_BObj->FetchFreight(rec.BillID, &freight) > 0) ? freight.StorageLocID : 0;
					THROW_SL(ExtraList.insert(&new_entry));
				}
			}
			if(P_TempTbl) {
				InventoryTbl::Key0 k0;
				k0.BillID = bill_id;
				k0.OprNo = oprno;
				if(P_TempTbl->searchForUpdate(0, &k0, spEq)) {
					THROW_DB(P_TempTbl->updateRecBuf(&rec)); // @sfu
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&rec));
				}
			}
			if(P_TempOrd) {
				TempDoubleIDTbl::Rec ord_rec;
				TempDoubleIDTbl::Key0 ko0;
				// @v10.6.4 MEMSZERO(ord_rec);
				MEMSZERO(ko0);
				MakeTempOrdRec(&rec, &ord_rec);
				ko0.PrmrID = ord_rec.PrmrID;
				ko0.ScndID = ord_rec.ScndID;
				if(P_TempOrd->searchForUpdate(0, &ko0, spEq)) {
					THROW_DB(P_TempOrd->updateRecBuf(&ord_rec));
				}
				else {
					THROW_DB(P_TempOrd->insertRecBuf(&ord_rec));
				}
			}
		}
		else {
			uint i = ExtraList.getCount();
			if(i) do {
				ExtraEntry & r_entry = ExtraList.at(--i);
				if(r_entry.BillID == billID && r_entry.OprNo == oprno)
					ExtraList.atFree(i);
			} while(i);
			if(P_TempTbl) {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->BillID == bill_id && P_TempTbl->OprNo == oprno));
			}
			if(P_TempOrd) {
				THROW_DB(deleteFrom(P_TempOrd, 0, P_TempOrd->PrmrID == bill_id && P_TempOrd->ScndID == oprno));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOKPPERR
	return ok;
}

//int PPViewInventory::

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, Inventory);
PP_CREATE_TEMP_FILE_PROC(CreateTempSubstFile, TempInventorySubst);
PP_CREATE_TEMP_FILE_PROC(CreateTempOrder2IDFile, TempDoubleID);

int PPViewInventory::MakeTempOrdRec(const InventoryTbl::Rec * pRec, TempDoubleIDTbl::Rec * pOutRec)
{
	SString temp_buf;
	TempDoubleIDTbl::Rec rec;
	// @v10.6.4 MEMSZERO(rec);
	if(pRec) {
		rec.PrmrID = pRec->BillID;
		rec.ScndID = pRec->OprNo;
		if(Filt.SortOrder == InventoryFilt::ordByGoods)
			GetObjectName(PPOBJ_GOODS, pRec->GoodsID, temp_buf);
		else {
			double big = 1.e9;
			double val = (Filt.SortOrder == InventoryFilt::ordByDeficit) ? (pRec->CSesDfctQtty * pRec->CSesDfctPrice) : (INVENT_DIFFSIGN(pRec->Flags) * pRec->DiffQtty);
			val += big;
			temp_buf.Printf("%055.8lf", val);
		}
		temp_buf.CopyTo(rec.Name, sizeof(rec.Name));
	}
	ASSIGN_PTR(pOutRec, rec);
	return 1;
}

int PPViewInventory::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Goods2Tbl::Rec gg_rec;
	SString name_buf;
	THROW(Helper_InitBaseFilt(pFilt) > 0);
	ZDELETE(P_TempTbl);
	ZDELETE(P_TempOrd);
	ZDELETE(P_TempSubstTbl);
	CommonLocID = 0;
	CommonDate = ZERODATE;
	CommonIoeFlags = 0;
	int    is_subst = 0;
	const  uint bill_count = Filt.BillList.GetCount();
	ExtraList.clear();
	TextPool.Z();
	Bsp.Init(Filt.Sgb);
	if(!!Filt.Sgb) {
		Filt.Sgg = sggNone;
		is_subst = 1;
	}
	else if(Filt.Sgg) {
		is_subst = 1;
	}
	Gsl.Init(1, 0);
	if(P_OuterPack) {
		const PPID bill_id = P_OuterPack->Rec.ID;
		const long subst_bill_val = bill_id;
		const PPID storage_loc_id = P_OuterPack->P_Freight ? P_OuterPack->P_Freight->StorageLocID : 0;
		THROW(P_TempTbl = CreateTempFile());
		if(Filt.SortOrder != InventoryFilt::ordByDefault)
			THROW(P_TempOrd = CreateTempOrder2IDFile());
		{
			BExtInsert bei(is_subst ? 0 : P_TempTbl);
			BExtInsert ord_bei(is_subst ? 0 : P_TempOrd);
			PPTransaction tra(ppDbDependTransaction, BIN(P_TempTbl || P_TempOrd));
			THROW(tra);
			for(uint i = 0; i < P_OuterPack->InvList.getCount(); i++) {
				InventoryTbl::Rec & r_inv_rec = P_OuterPack->InvList.at(i);
				const PPID org_goods_id = r_inv_rec.GoodsID;
				int    do_skip = 0;
				if(Filt.GoodsList.IsExists()) {
					if(!Filt.GoodsList.CheckID(org_goods_id))
						do_skip = 1;
				}
				else {
					if(Filt.GoodsID && org_goods_id != Filt.GoodsID)
						do_skip = 1;
					else if(Filt.GoodsGrpID && !GObj.BelongToGroup(org_goods_id, Filt.GoodsGrpID, 0))
						do_skip = 1;
				}
				if(!do_skip) {
					ExtraEntry new_entry;
					// @v10.9.12 @ctr MEMSZERO(new_entry);
					PPID   final_goods_id = 0;
					if(!!Filt.Sgb)
						final_goods_id = subst_bill_val;
					else {
						PPObjGoods::SubstBlock sgg_blk;
						sgg_blk.ExclParentID = Filt.GoodsGrpID;
						sgg_blk.LocID = P_OuterPack->Rec.LocID;
						sgg_blk.LotID = 0;
						THROW(GObj.SubstGoods(org_goods_id, &final_goods_id, Filt.Sgg, &sgg_blk, &Gsl));
					}
					if(is_subst) {
						const double diff = (r_inv_rec.Quantity - r_inv_rec.StockRest);
						uint idx = 0;
						TempInventorySubstTbl::Key0 k0;
						MEMSZERO(k0);
						k0.GoodsID = final_goods_id;
						if(P_TempSubstTbl->search(0, &k0, spEq)) {
							const int sr = ExtraList.lsearch(&final_goods_id, &idx, CMPF_LONG, offsetof(ExtraEntry, GoodsID));
							assert(sr);
							P_TempSubstTbl->data.Quantity += r_inv_rec.Quantity;
							P_TempSubstTbl->data.StockRest += r_inv_rec.StockRest;
							P_TempSubstTbl->data.DiffQtty += diff;
							P_TempSubstTbl->data.DiffPrice += diff * r_inv_rec.Price;
							P_TempSubstTbl->data.DiffPctQtty = 100.0 * P_TempSubstTbl->data.DiffQtty / P_TempSubstTbl->data.StockRest;
							P_TempSubstTbl->data.SumPrice += r_inv_rec.Quantity * r_inv_rec.Price;
							P_TempSubstTbl->data.SumStockPrice += r_inv_rec.StockRest * r_inv_rec.StockPrice;
							P_TempSubstTbl->data.SumWrOffPrice += diff * r_inv_rec.WrOffPrice;
							THROW(P_TempSubstTbl->updateRec());
						}
						else {
							new_entry.SurrID = ++LastSurrID;
							new_entry.BillID = subst_bill_val;
							new_entry.OprNo = r_inv_rec.OprNo;
							new_entry.GoodsID = final_goods_id;
							new_entry.StorageLocID = storage_loc_id;
							THROW_SL(ExtraList.insert(&new_entry));
							{
								TempInventorySubstTbl::Rec temp_rec;
								// @v10.6.4 MEMSZERO(temp_rec);
								temp_rec.GoodsID = final_goods_id;
								temp_rec.Quantity = r_inv_rec.Quantity;
								temp_rec.StockRest = r_inv_rec.StockRest;
								temp_rec.DiffQtty = diff;
								temp_rec.DiffPrice = diff * r_inv_rec.Price;
								temp_rec.DiffPctQtty = 100.0 * temp_rec.DiffQtty / temp_rec.StockRest;
								temp_rec.SumPrice = r_inv_rec.Quantity * r_inv_rec.Price;
								temp_rec.SumStockPrice = r_inv_rec.StockRest * r_inv_rec.StockPrice;
								temp_rec.SumWrOffPrice = diff * r_inv_rec.WrOffPrice;
								if(!!Filt.Sgb) {
									P_BObj->GetSubstText(final_goods_id, &Bsp, name_buf.Z());
								}
								else {
									GObj.GetSubstText(final_goods_id, Filt.Sgg, &Gsl, name_buf.Z());
								}
								name_buf.CopyTo(temp_rec.Name, sizeof(temp_rec.Name));
								THROW(P_TempSubstTbl->insertRecBuf(&temp_rec));
							}
						}
					}
					else {
						new_entry.SurrID = ++LastSurrID;
						new_entry.BillID = subst_bill_val;
						new_entry.OprNo = r_inv_rec.OprNo;
						new_entry.GoodsID = final_goods_id;
						new_entry.StorageLocID = storage_loc_id;
						THROW_SL(ExtraList.insert(&new_entry));
						if(P_TempTbl) {
							THROW_DB(bei.insert(&r_inv_rec));
						}
						if(P_TempOrd) {
							TempDoubleIDTbl::Rec rec;
							MakeTempOrdRec(&r_inv_rec, &rec);
							THROW_DB(ord_bei.insert(&rec));
						}
					}
				}
			}
			if(!is_subst) {
				if(P_TempTbl)
					THROW_DB(bei.flash());
				if(P_TempOrd)
					THROW_DB(ord_bei.flash());
			}
			THROW(tra.Commit());
		}
	}
	else if(bill_count) {
		PPWaitStart();
		if(is_subst) {
			THROW(P_TempSubstTbl = CreateTempSubstFile());
		}
		else if(bill_count > 1 || Filt.SortOrder != InventoryFilt::ordByDefault || GObj.Fetch(Filt.GoodsGrpID, &gg_rec) > 0) {
			THROW(P_TempTbl = CreateTempFile());
			if(Filt.SortOrder != InventoryFilt::ordByDefault)
				THROW(P_TempOrd = CreateTempOrder2IDFile());
		}
		for(uint i = 0; i < bill_count; i++) {
			const PPID bill_id = Filt.BillList.Get().get(i);
			BillTbl::Rec bill_rec;
			PPInventoryOpEx ioe;
			if(P_BObj->Search(bill_id, &bill_rec) > 0) {
				PPFreight freight;
				const PPID storage_loc_id = (P_BObj->FetchFreight(bill_id, &freight) > 0) ? freight.StorageLocID : 0;
				if(!Filt.StorageLocID || storage_loc_id == Filt.StorageLocID) {
					long   subst_bill_val = bill_rec.ID;
					if(!!Filt.Sgb) {
						PPBill _pack;
						_pack.Rec = bill_rec;
						P_BObj->LoadForSubst(&Bsp, &_pack);
						P_BObj->Subst(&_pack, &subst_bill_val, &Bsp);
					}
					THROW(P_BObj->P_OpObj->FetchInventoryData(bill_rec.OpID, &ioe));
					if(i == 0) {//bpack.Rec.LocID == CommonLocID)
						CommonLocID = bill_rec.LocID;
						CommonDate = bill_rec.Dt;
						CommonIoeFlags = ioe.Flags;
					}
					else {
						if(bill_rec.LocID != CommonLocID)
							CommonLocID = 0;
						if(bill_rec.Dt != CommonDate)
							CommonDate = ZERODATE;
						CommonIoeFlags &= ioe.Flags;
					}
					{
						InventoryTbl::Rec inv_rec;
						BExtInsert bei(is_subst ? 0 : P_TempTbl);
						BExtInsert ord_bei(is_subst ? 0 : P_TempOrd);
						PPTransaction tra(ppDbDependTransaction, BIN(P_TempTbl || P_TempOrd));
						THROW(tra);
						{
							for(SEnum en = InvTbl.Enum(bill_id); en.Next(&inv_rec) > 0;) {
								int    do_skip = 0;
								const PPID org_goods_id = inv_rec.GoodsID;
								if(Filt.GoodsList.IsExists()) {
									if(!Filt.GoodsList.CheckID(org_goods_id))
										do_skip = 1;
								}
								else {
									if(Filt.GoodsID && org_goods_id != Filt.GoodsID)
										do_skip = 1;
									else if(Filt.GoodsGrpID && !GObj.BelongToGroup(org_goods_id, Filt.GoodsGrpID, 0))
										do_skip = 1;
								}
								//@erik @v10.5.4
								if(!CheckInventoryLineForWrOff(Filt.Flags, inv_rec.BillID, inv_rec.Flags, inv_rec.DiffQtty))
									do_skip = 1;
								else if(Filt.Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus)){
									if((Filt.Flags & InventoryFilt::fLack) && !(Filt.Flags & InventoryFilt::fSurplus)) {
										if(!(inv_rec.Flags & INVENTF_LACK))
											do_skip = 1;
									}
									else if((Filt.Flags & InventoryFilt::fSurplus) && !(Filt.Flags & InventoryFilt::fLack)) {
										if(!(inv_rec.Flags & INVENTF_SURPLUS))
											do_skip = 1;
									}
									else {
										if(!(inv_rec.Flags & INVENTF_SURPLUS) && !(inv_rec.Flags & INVENTF_LACK))
											do_skip = 1;
									}
								}
								// } @erik
								if(!do_skip) {
									ExtraEntry new_entry;
									// @v10.9.12 @ctr MEMSZERO(new_entry);
									PPID   final_goods_id = 0;
									if(!!Filt.Sgb)
										final_goods_id = subst_bill_val;
									else {
										PPObjGoods::SubstBlock sgg_blk;
										sgg_blk.ExclParentID = Filt.GoodsGrpID;
										sgg_blk.LocID = bill_rec.LocID;
										sgg_blk.LotID = 0;
										THROW(GObj.SubstGoods(org_goods_id, &final_goods_id, Filt.Sgg, &sgg_blk, &Gsl));
									}
									if(is_subst) {
										const double diff = (inv_rec.Quantity - inv_rec.StockRest);
										uint idx = 0;
										TempInventorySubstTbl::Key0 k0;
										MEMSZERO(k0);
										k0.GoodsID = final_goods_id;
										if(P_TempSubstTbl->search(0, &k0, spEq)) {
											const int sr = ExtraList.lsearch(&final_goods_id, &idx, CMPF_LONG, offsetof(ExtraEntry, GoodsID));
											assert(sr);
											{
												TempInventorySubstTbl::Rec & r_st_rec = P_TempSubstTbl->data;
												r_st_rec.Quantity += inv_rec.Quantity;
												r_st_rec.StockRest += inv_rec.StockRest;
												r_st_rec.DiffQtty += diff;
												r_st_rec.DiffPrice += diff * inv_rec.Price;
												r_st_rec.DiffPctQtty = 100.0 * r_st_rec.DiffQtty / r_st_rec.StockRest;
												r_st_rec.SumPrice += inv_rec.Quantity * inv_rec.Price;
												r_st_rec.SumStockPrice += inv_rec.StockRest * inv_rec.StockPrice;
												r_st_rec.SumWrOffPrice += diff * inv_rec.WrOffPrice;
												THROW(P_TempSubstTbl->updateRec());
											}
										}
										else {
											new_entry.SurrID = ++LastSurrID;
											new_entry.BillID = subst_bill_val;
											new_entry.OprNo = inv_rec.OprNo;
											new_entry.GoodsID = final_goods_id;
											new_entry.StorageLocID = storage_loc_id;
											THROW_SL(ExtraList.insert(&new_entry));
											{
												TempInventorySubstTbl::Rec temp_rec;
												// @v10.6.4 MEMSZERO(temp_rec);
												temp_rec.GoodsID = final_goods_id;
												temp_rec.Quantity = inv_rec.Quantity;
												temp_rec.StockRest = inv_rec.StockRest;
												temp_rec.DiffQtty = diff;
												temp_rec.DiffPrice = diff * inv_rec.Price;
												temp_rec.DiffPctQtty = 100.0 * temp_rec.DiffQtty / temp_rec.StockRest;
												temp_rec.SumPrice = inv_rec.Quantity * inv_rec.Price;
												temp_rec.SumStockPrice = inv_rec.StockRest * inv_rec.StockPrice;
												temp_rec.SumWrOffPrice = diff * inv_rec.WrOffPrice;
												if(!!Filt.Sgb) {
													P_BObj->GetSubstText(final_goods_id, &Bsp, name_buf.Z());
												}
												else {
													GObj.GetSubstText(final_goods_id, Filt.Sgg, &Gsl, name_buf.Z());
												}
												name_buf.CopyTo(temp_rec.Name, sizeof(temp_rec.Name));
												THROW(P_TempSubstTbl->insertRecBuf(&temp_rec));
											}
										}
									}
									else {
										new_entry.SurrID = ++LastSurrID;
										new_entry.BillID = subst_bill_val;
										new_entry.OprNo = inv_rec.OprNo;
										new_entry.GoodsID = final_goods_id;
										new_entry.StorageLocID = storage_loc_id;
										THROW_SL(ExtraList.insert(&new_entry));
										if(P_TempTbl) {
											THROW_DB(bei.insert(&inv_rec));
										}
										if(P_TempOrd) {
											TempDoubleIDTbl::Rec rec;
											MakeTempOrdRec(&inv_rec, &rec);
											THROW_DB(ord_bei.insert(&rec));
										}
									}
								}
							}
						}
						if(!is_subst) {
							if(P_TempTbl)
								THROW_DB(bei.flash());
							if(P_TempOrd)
								THROW_DB(ord_bei.flash());
						}
						THROW(tra.Commit());
					}
				}
			}
		}
		SETFLAG(Flags, fIsZeroByDefault, (CommonIoeFlags & INVOPF_ZERODEFAULT));
		SETFLAG(Flags, fSelGoodsByName, (CommonIoeFlags & INVOPF_SELGOODSBYNAME));
	}
	CATCH
		ZDELETE(P_TempTbl);
		ZDELETE(P_TempOrd);
		ZDELETE(P_TempSubstTbl);
		ok = 0;
	ENDCATCH
	PPWaitStop();
	return ok;
}

int PPViewInventory::InitIteration()
{
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_GgIter);
	ZDELETE(P_GIter);
	IterByGoods.clear();
	ExtraList.setPointer(0);
	int    ok = 1;
	DBQ  * dbq = 0;
	THROW_MEM(P_GgIter = new GoodsGroupIterator(Filt.GoodsGrpID, GoodsGroupIterator::fAddZeroGroup));
	if(Flags & fIsZeroByDefault && Filt.Flags & InventoryFilt::fShowAbsenseGoods) {
		THROW_MEM(P_GIter = new GoodsIterator(Filt.GoodsGrpID, GoodsIterator::ordByName));
		Counter.Init(P_GIter->GetIterCounter().GetTotal());
	}
	else {
		const PPID bill_id = Filt.GetSingleBillID();
		THROW_MEM(P_IterQuery = new BExtQuery(&InvTbl, 0));
		P_IterQuery->selectAll();
		dbq = & (InvTbl.BillID == bill_id);
		if(Filt.Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
			double minv = Filt.MinVal;
			double maxv = Filt.MaxVal;
			if(maxv >= minv && maxv > 0) {
				if(minv < 0)
					minv = 0;
				if(Filt.Flags & InventoryFilt::fPctVal) {
					if(minv == maxv) {
						minv = MAX(0, minv - 1L);
						maxv = maxv + 1L;
					}
					dbq = & (*dbq && realrange(InvTbl.DiffPctQtty, minv, maxv));
				}
				else
					dbq = & (*dbq && realrange(InvTbl.DiffQtty, minv, maxv));
			}
		}
		P_IterQuery->where(*dbq);
		InventoryTbl::Key0 k, k_;
		k.BillID = bill_id;
		k.OprNo = 0;
		k_ = k;
		Counter.Init(P_IterQuery->countIterations(0, &k_, spGt));
		P_IterQuery->initIteration(0, &k, spGt);
	}
	CATCH
		BExtQuery::ZDelete(&P_IterQuery);
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL PPViewInventory::NextIteration(InventoryViewItem * pItem)
{
	int    ok = -1;
	PROFILE_START;
	Goods2Tbl::Rec goods_rec;
	InventoryTbl::Rec inv_rec;
	const  PPID single_bill_id = Filt.GetSingleBillID();
	// @v10.6.4 MEMSZERO(inv_rec);
	if(P_GIter) {
		while(ok <= 0 && P_GIter->Next(&goods_rec) > 0) {
			Counter.Increment();
			while(ok < 0 && IterByGoods.getPointer() < IterByGoods.getCount()) {
				uint pos = IterByGoods.incPointer();
				if(CheckLineForFilt(&IterByGoods.at(pos))) {
					inv_rec = IterByGoods.at(pos);
					ok = 1;
				}
			}
			if(ok < 0) {
				IterByGoods.clear();
				InventoryArray local_inv_list;
				for(uint i = 0; i < Filt.BillList.GetCount(); i++) {
					const PPID bill_id = Filt.BillList.Get().get(i);
					if(InvTbl.SearchByGoods(bill_id, goods_rec.ID, &local_inv_list) > 0) {
						for(uint j = 0; j < local_inv_list.getCount(); j++) {
							THROW_SL(IterByGoods.insert(&local_inv_list.at(j)));
						}
					}
				}
				IterByGoods.setPointer(0);
				if(IterByGoods.getCount()) {
					while(ok < 0 && IterByGoods.getPointer() < IterByGoods.getCount()) {
						uint pos = IterByGoods.incPointer();
						if(CheckLineForFilt(&IterByGoods.at(pos))) {
							inv_rec = IterByGoods.at(pos);
							ok = 1;
						}
					}
				}
				else if(!(goods_rec.Flags & GF_GENERIC)) {
					const int is_asst = GObj.IsAsset(goods_rec.ID);
					if((CommonIoeFlags & INVOPF_ASSET && is_asst) || (!(CommonIoeFlags & INVOPF_ASSET) && !is_asst)) {
						GoodsRestParam p;
						p.LocID   = CommonLocID;
						p.GoodsID = goods_rec.ID;
						p.Date    = CommonDate;
						if(P_BObj->trfr->GetRest(p) > 0 && p.Total.Rest > 0.0) {
							MEMSZERO(inv_rec);
							inv_rec.BillID  = single_bill_id;
							inv_rec.GoodsID = goods_rec.ID;
							inv_rec.UnitPerPack = goods_rec.PhUPerU;
							inv_rec.DiffQtty = inv_rec.StockRest = p.Total.Rest;
							inv_rec.Price = p.Total.Price;
							memcpy(&inv_rec.StockPrice, &inv_rec.Price, sizeof(inv_rec.Price));
							INVENT_SETDIFFSIGN(inv_rec.Flags, -1);
							inv_rec.DiffPctQtty = -100;
							ok = CheckLineForFilt(&inv_rec) ? 1 : -1;
						}
					}
				}
			}
		}
	}
	else {
		while(ok <= 0 && ExtraList.getPointer() < ExtraList.getCount()) {
			const ExtraEntry & r_entry = ExtraList.at(ExtraList.getPointer());
			Counter.Increment();
			ExtraList.incPointer();
			if(InvTbl.Search(r_entry.BillID, r_entry.OprNo, &inv_rec) > 0 && InvTbl.CheckFlags(&Filt, &inv_rec)) {
				ok = 1;
				if(Filt.Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
					double minv = Filt.MinVal;
					double maxv = Filt.MaxVal;
					if(maxv >= minv && maxv > 0) {
						if(minv < 0)
							minv = 0;
						if(Filt.Flags & InventoryFilt::fPctVal) {
							if(minv == maxv) {
								minv = MAX(0, minv - 1L);
								maxv = maxv + 1L;
							}
							if(inv_rec.DiffPctQtty < minv || inv_rec.DiffPctQtty > maxv)
								ok = -1;
						}
						else {
							if(inv_rec.DiffQtty < minv || inv_rec.DiffQtty > maxv)
								ok = -1;
						}
					}
				}
				if(ok > 0 && GObj.BelongToGroup(inv_rec.GoodsID, Filt.GoodsGrpID, 0) && GObj.Fetch(inv_rec.GoodsID, &goods_rec) <= 0)
					ok = -1;
			}
		}
	}
	if(ok > 0) {
		if(pItem) {
			*static_cast<InventoryTbl::Rec *>(pItem) = inv_rec;
			P_GgIter->Get(goods_rec.ParentID, pItem->FullGrpName);
		}
	}
	CATCHZOK
	PROFILE_END;
	return ok;
}

int PPViewInventory::UpdatePacket(PPID billID)
{
	int    ok = -1;
	if(billID) {
		InventoryTotal total;
		InventoryFilt filt;
		filt.SetSingleBillID(billID);
		PPBillPacket bpack;
		THROW(P_BObj->ExtractPacket(billID, &bpack) > 0);
		THROW(InvTbl.CalcTotal(&filt, &total));
		bpack.Rec.Amount = BR2(total.Amount);
		bpack.Amounts.Put(PPAMT_MAIN, bpack.Rec.CurID, total.Amount, 0, 1);
		THROW(P_BObj->TurnInventory(&bpack, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
// InventoryItemDialog
//
QuantityCtrlGroup::Rec::Rec()
{
	THISZERO();
}

bool FASTCALL QuantityCtrlGroup::Rec::operator == (const Rec & rS) const
{
	const double epsilon = 1E-8;
	return (feqeps(UnitPerPack, rS.UnitPerPack, epsilon) && feqeps(Packs, rS.Packs, epsilon) &&
		feqeps(PackTail, rS.PackTail, epsilon) && feqeps(Qtty, rS.Qtty, epsilon));
}

QuantityCtrlGroup::QuantityCtrlGroup(uint ctlUpp, uint ctlPacks, uint ctlQtty) : CtrlGroup(), CtlUpp(ctlUpp), CtlPacks(ctlPacks), CtlQtty(ctlQtty), LockUpdByInput(0)
{
}

void QuantityCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(event.isCmd(cmInputUpdated)) {
		const uint ctl = event.getCtlID();
		if(oneof3(ctl, CtlUpp, CtlPacks, CtlQtty)) {
			SetupQuantity(pDlg, ctl, 1);
			//pDlg->clearEvent(event);
		}
	}
}

int QuantityCtrlGroup::ReadFld(TDialog * pDlg, uint master, uint ctl)
{
	int    nothing = 0;
	Rec    preserve_data = Data;
	if(ctl == CtlPacks) {
		SString packs_buf;
		pDlg->getCtrlString(ctl, packs_buf);
		if(packs_buf.NotEmptyS()) {
			SString left_buf, right_buf;
			if(packs_buf.Divide('+', left_buf, right_buf) > 0) {
				Data.Packs = left_buf.ToReal();
				Data.PackTail = right_buf.ToReal();
			}
			else {
				Data.Packs = packs_buf.ToReal();
				Data.PackTail = 0.0;
			}
		}
		else {
			Data.Packs = 0.0;
			Data.PackTail = 0.0;
		}
	}
	else if(ctl == CtlUpp) {
		Data.UnitPerPack = pDlg->getCtrlReal(ctl);
	}
	else if(ctl == CtlQtty) {
		Data.Qtty = pDlg->getCtrlReal(ctl);
	}
	else
		nothing = 1;
	return (nothing || (Data == preserve_data && master == ctl)) ? 0 : 1;
}

void QuantityCtrlGroup::SetupQuantity(TDialog * pDlg, uint master, int readFlds)
{
	if(!LockUpdByInput) {
		LockUpdByInput = 1;
		if(!readFlds || (ReadFld(pDlg, master, CtlUpp) && ReadFld(pDlg, master, CtlPacks) && ReadFld(pDlg, master, CtlQtty))) {
			if(Data.UnitPerPack > 0.0) {
				if(master == CtlPacks || master == CtlUpp)
					Data.Qtty = Data.UnitPerPack * Data.Packs + Data.PackTail;
				else if(master == 0 || master == CtlQtty) {
					Data.Packs = fint(Data.Qtty / Data.UnitPerPack);
					Data.PackTail = fint(fmod(Data.Qtty, Data.UnitPerPack));
				}
			}
			else {
				Data.UnitPerPack = 0.0;
				Data.Packs = 0.0;
				Data.PackTail = 0.0;
			}
			SString packs_buf;
			pDlg->setCtrlReal(CtlUpp, Data.UnitPerPack);
			packs_buf.Cat(Data.Packs, MKSFMTD(0, 6, NMBF_NOTRAILZ|NMBF_NOZERO));
			if(Data.PackTail > 0.0)
				packs_buf.CatChar('+').Cat(Data.PackTail, MKSFMTD(0, 6, NMBF_NOTRAILZ));
			pDlg->setCtrlString(CtlPacks, packs_buf);
			pDlg->setCtrlReal(CtlQtty, Data.Qtty);
			if(master) {
				TInputLine * p_il = static_cast<TInputLine *>(pDlg->getCtrlView(master));
				if(p_il && p_il->IsSubSign(TV_SUBSIGN_INPUTLINE))
					p_il->selectAll(0);
			}
		}
		LockUpdByInput = 0;
	}
}

int QuantityCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	Data = *static_cast<Rec *>(pData);
	SetupQuantity(pDlg, 0, 0);
	return 1;
}

int QuantityCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	if(!LockUpdByInput) {
		ReadFld(pDlg, 0, CtlUpp);
		ReadFld(pDlg, 0, CtlPacks);
		ReadFld(pDlg, 0, CtlQtty);
	}
	*static_cast<Rec *>(pData) = Data;
	return 1;
}

class InventoryItemDialog : public TDialog {
	DECL_DIALOG_DATA(InventoryTbl::Rec);
	enum {
		ctlgroupGoods = 1,
		ctlgroupQtty  = 2
	};
public:
	InventoryItemDialog(PPObjBill * pBObj, const PPBillPacket * pPack, const PPInventoryOpEx * pInvOpEx, int existsGoodsOnly) :
		TDialog(DLG_INVITEM), P_BObj(pBObj), P_Pack(pPack), StockRest(0.0), StockPrice(0.0), St(0), Packs(0.0), Price(0.0)
	{
		InvOpEx = *pInvOpEx;
		SETFLAG(St, stExistsGoodsOnly, existsGoodsOnly);
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_INVITEM_GOODSGRP, CTLSEL_INVITEM_GOODS));
		addGroup(ctlgroupQtty,  new QuantityCtrlGroup(CTL_INVITEM_UNITPERPACK, CTL_INVITEM_PACKS, CTL_INVITEM_QUANTITY));
		disableCtrls(1, CTL_INVITEM_STOCKREST, CTL_INVITEM_DIFFREST, 0);
		SETFLAG(St, stUseSerial, InvOpEx.Flags & INVOPF_USESERIAL);
		{
			SString temp_buf;
			if(InvOpEx.Flags & INVOPF_COSTNOMINAL)
				PPLoadString("inventory_cost", temp_buf);
			else
				PPLoadString("inventory_price", temp_buf);
			setLabelText(CTL_INVITEM_PRICE, temp_buf);
		}
		// @v10.9.12 {
		if(CConfig.Flags2 & CCFLG2_HIDEINVENTORYSTOCK) {
			showCtrl(CTL_INVITEM_STOCKREST, 0);
			showCtrl(CTL_INVITEM_DIFFREST, 0);
			enableCommand(cmLot, 0);
		}
		// } @v10.9.12 
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		Serial = (St & stUseSerial) ? Data.Serial : 0;
		int    ok = 1;
		disableCtrl(CTL_INVITEM_SERIAL, !(St & stUseSerial));
		GoodsCtrlGroup::Rec gcg_rec(0, Data.GoodsID, P_Pack->Rec.LocID, GoodsCtrlGroup::disableEmptyGoods);
		if(St & stExistsGoodsOnly)
			gcg_rec.Flags |= GoodsCtrlGroup::existsGoodsOnly;
		setGroupData(ctlgroupGoods, &gcg_rec);
		if(Data.GoodsID) {
			disableCtrls(1, CTLSEL_INVITEM_GOODS, CTLSEL_INVITEM_GOODSGRP, 0);
			replyGoodsSelection();
		}
		else {
			setupQuantity();
			setCtrlData(CTL_INVITEM_PRICE, &Data.Price);
		}
		setCtrlString(CTL_INVITEM_SERIAL, Serial);
		setCtrlData(CTL_INVITEM_WROFFPRICE, &Data.WrOffPrice);
		disableCtrl(CTL_INVITEM_WROFFPRICE, 1);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GoodsCtrlGroup::Rec gcg_rec;
		QuantityCtrlGroup::Rec qcg_rec;
		THROW(getGroupData(ctlgroupGoods, &gcg_rec));
		THROW(getGroupData(ctlgroupQtty, &qcg_rec));
		Data.GoodsID = gcg_rec.GoodsID;
		getCtrlString(CTL_INVITEM_SERIAL, Serial);
		Data.UnitPerPack = qcg_rec.UnitPerPack;
		Data.Quantity = qcg_rec.Qtty;
		getCtrlData(CTL_INVITEM_PRICE, &Data.Price);
		INVENT_SETAUTOLINE(Data.Flags, 0);
		if(!(Data.Flags & INVENTF_WRITEDOFF))
			Data.WrOffPrice = Data.StockPrice;
		if(P_Pack && P_Pack->Rec.DueDate != ZERODATE) {
			CSessDfctList dfct_list;
			DateRange dfct_prd;
			dfct_prd.low = P_Pack->Rec.DueDate;
			THROW(GL.GetDfctList(0, 0, Data.GoodsID, &dfct_prd, 1, &dfct_list, 0));
			if(dfct_list.getCount()) {
				Data.CSesDfctQtty  = dfct_list.at(0).Dfct;
				Data.CSesDfctPrice = fabs(fdivnz(dfct_list.at(0).Sum, dfct_list.at(0).Qtty));
			}
		}
		if(St & stUseSerial)
			Serial.CopyTo(Data.Serial, sizeof(Data.Serial));
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	int    readQttyFld(uint master, uint ctl, double * val);
	int    replyGoodsSelection();
	void   setupQuantity();
	void   setupQttyFldPrec();
	void   recalcUnitsToPhUnits(); // kbF4

	PPObjBill * P_BObj;
	PPObjGoods GObj;
	CGoodsLine GL;
	const  PPBillPacket * P_Pack;
	PPInventoryOpEx  InvOpEx;
	enum {
		stExistsGoodsOnly = 0x0001,
		stGoodsFixed      = 0x0002,  // Фиксированный товар (выбран до входа в диалог)
		stUseSerial       = 0x0004,  //
		stLockUpdateByInp = 0x0008   //
	};
	long   St;
	double StockRest;
	double StockPrice;
	double Packs;
	double Price;
	SString Serial;
};

int InventoryItemDialog::replyGoodsSelection()
{
	int    ok = 1;
	GoodsRestParam p;
	getCtrlData(CTLSEL_INVITEM_GOODS, &Data.GoodsID);
	if(Data.GoodsID) {
		PPObjBill::InvBlock blk;
		PPObjBill::InvItem inv_item;
		THROW(P_BObj->InitInventoryBlock(P_Pack->Rec.ID, blk));
		inv_item.Init(Data.GoodsID, Data.Serial);
		THROW(P_BObj->GetInventoryStockRest(blk, &inv_item, &p));
		StockRest = inv_item.FinalRest;
		StockPrice = inv_item.StockPrice;
		if(Data.OprNo == 0) {
			if(Data.Quantity <= 0.0)
				Data.Quantity = StockRest;
			Data.UnitPerPack = p.Total.UnitsPerPack;
			Data.Price = StockPrice;
		}
		Data.StockRest  = StockRest;
		Data.StockPrice = StockPrice;
		setupQuantity();
		setCtrlReal(CTL_INVITEM_PRICE, Data.Price);
		setCtrlReal(CTL_INVITEM_STOCKREST, StockRest);
		selectCtrl((InvOpEx.Flags & INVOPF_USEPACKS) ? CTL_INVITEM_UNITPERPACK : CTL_INVITEM_QUANTITY);
	}
	else {
		selectCtrl(CTL_INVITEM_GOODS);
		messageToCtrl(CTLSEL_INVITEM_GOODS, cmCBActivate, 0);
	}
	CATCHZOKPPERR
	return ok;
}

void InventoryItemDialog::recalcUnitsToPhUnits()
{
	Goods2Tbl::Rec goods_rec;
	const PPID goods_id = getCtrlLong(CTLSEL_INVITEM_GOODS);
	if(goods_id && GObj.Fetch(goods_id, &goods_rec) > 0) {
		const double qtty = getCtrlReal(CTL_INVITEM_QUANTITY);
		if(goods_rec.PhUPerU > 0.0) {
			Data.Quantity = R6(qtty / goods_rec.PhUPerU);
			setupQuantity();
		}
	}
}

IMPL_HANDLE_EVENT(InventoryItemDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmLot)) {
		if(Data.GoodsID) {
			LotFilt filt;
			filt.GoodsID = Data.GoodsID;
			filt.LocList.Add(P_Pack ? P_Pack->Rec.LocID : 0);
			if(St & stUseSerial)
				filt.Flags |= LotFilt::fShowSerialN;
			ViewLots(&filt, 0, 0);
		}
	}
	else if(event.isCbSelected(CTLSEL_INVITEM_GOODS)) {
		replyGoodsSelection();
	}
	else if(event.isKeyDown(kbF2)) {
		if(isCurrCtlID(CTL_INVITEM_PRICE))
		 	setCtrlData(CTL_INVITEM_PRICE, &StockPrice);
	}
	else if(event.isKeyDown(kbF4)) {
		if(isCurrCtlID(CTL_INVITEM_QUANTITY))
			recalcUnitsToPhUnits();
	}
	else if(event.isCmd(cmInputUpdated)) {
		uint i = TVINFOVIEW->GetId();
		if(oneof3(i, CTL_INVITEM_UNITPERPACK, CTL_INVITEM_PACKS, CTL_INVITEM_QUANTITY)) {
			if(!(St & stLockUpdateByInp)) {
				St |= stLockUpdateByInp;
				QuantityCtrlGroup::Rec qcg_rec;
				getGroupData(ctlgroupQtty, &qcg_rec);
				setCtrlReal(CTL_INVITEM_DIFFREST, qcg_rec.Qtty - StockRest);
				St &= ~stLockUpdateByInp;
			}
		}
	}
	else
		return;
	clearEvent(event);
}

void InventoryItemDialog::setupQuantity()
{
	QuantityCtrlGroup::Rec qcg_rec;
	qcg_rec.UnitPerPack = Data.UnitPerPack;
	qcg_rec.Qtty = Data.Quantity;
	setGroupData(ctlgroupQtty, &qcg_rec);
}

int PPViewInventory::EditLine(PPID billID, long * pOprNo, PPID goodsID, const char * pSerial, double initQtty, int accelMode)
{
	int    ok = -1;
	int    valid_data = 0;
	int    accel_mode = 0;
	int    skip_dialog = 0;
	int    was_goods_replaced = 0;
	SString temp_buf;
	InventoryTbl::Rec rec, duprec;
	InventoryItemDialog * dlg = 0;
	if(billID) {
		PPBillPacket bpack;
		THROW(P_BObj->ExtractPacket(billID, &bpack) > 0);
		THROW_PP_S(!bpack.Rec.StatusID || !P_BObj->CheckStatusFlag(bpack.Rec.StatusID, BILSTF_DENY_MOD), PPERR_BILLST_DENY_MOD, PPObjBill::MakeCodeString(&bpack.Rec, 1, temp_buf));
		if(*pOprNo == 0) {
			MEMSZERO(rec);
			rec.GoodsID = goodsID;
			rec.Quantity = initQtty;
			STRNSCPY(rec.Serial, pSerial);
			if(accelMode && rec.GoodsID && rec.Quantity > 0.0)
				accel_mode = 1;
		}
		else {
			Goods2Tbl::Rec goods_rec;
			THROW(InvTbl.Search(billID, *pOprNo, &rec) > 0);
			if(GObj.Fetch(rec.GoodsID, &goods_rec) < 0) {
				PPID   subst_id = 0;
				for(valid_data = 0; !valid_data && GObj.SelectGoodsInPlaceOfRemoved(rec.GoodsID, subst_id, &subst_id) > 0;) {
					if(InvTbl.SearchIdentical(billID, subst_id, 0, 0) > 0) {
						PPError(PPERR_INVMOVFAILONSAMEGOODS, GetGoodsName(subst_id, temp_buf));
					}
					else {
						rec.GoodsID = subst_id;
						THROW(InvTbl.Set(billID, pOprNo, &rec, 1));
						valid_data = 1;
						was_goods_replaced = 1;
					}
				}
				if(!valid_data)
					skip_dialog = 1;
			}
		}
		if(!skip_dialog) {
			PPInventoryOpEx ioe;
			THROW(P_BObj->P_OpObj->FetchInventoryData(bpack.Rec.OpID, &ioe));
			THROW(CheckDialogPtr(&(dlg = new InventoryItemDialog(P_BObj, &bpack, &ioe, BIN(Filt.Flags & InventoryFilt::fSelExistsGoodsOnly)))));
			THROW(dlg->setDTS(&rec));
			for(valid_data = 0; !valid_data && (accel_mode || ExecView(dlg) == cmOK);) {
				if((valid_data = dlg->getDTS(&rec)) != 0) {
					if(rec.Flags & INVENTF_WRITEDOFF)
						ok = (PPError(PPERR_UPDWROFFINV, 0), -1);
					else {
						int    reply = cmYes;
						if(*pOprNo == 0 && InvTbl.SearchIdentical(billID, rec.GoodsID, strip(rec.Serial), &duprec) > 0) {
							if(duprec.Flags & INVENTF_WRITEDOFF) {
								reply = cmCancel;
								PPError(PPERR_INVTOOWRITEDOFF, 0);
							}
							else if(duprec.Flags & INVENTF_AUTOLINE) // Просто изменить строку
								*pOprNo = duprec.OprNo;
							else {
								if(accelMode)
									reply = cmYes;
								else
									reply = PPMessage(mfConf | mfYesNoCancel, PPCFM_INVDUPGOODS);
								if(reply == cmYes) {
									duprec.Quantity += rec.Quantity;
									rec = duprec;
									*pOprNo = duprec.OprNo;
								}
								else if(reply == cmNo)
									*pOprNo = duprec.OprNo;
							}
						}
						if(reply != cmCancel) {
							THROW(InvTbl.Set(billID, pOprNo, &rec, 1));
							ok = 1;
						}
						else
							ok = -1;
					}
				}
				else
					accel_mode = 0;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	if(ok < 0 && was_goods_replaced)
		ok = -2;
	return ok;
}
//
//
//
int PPViewInventory::AddItem(TIDlgInitData * pInitData)
{
	int    ok = -1;
	const  int accel_mode = pInitData ? BIN(pInitData->Flags & TIDIF_AUTOQTTY) : 0;
	long   oprno = 0;
	PPID   goods_id = pInitData ? pInitData->GoodsID : 0;
	double qtty = pInitData ? pInitData->Quantity : 0.0;
	const  char * p_serial = pInitData ? pInitData->Serial : 0;
	PPID   bill_id = Filt.GetSingleBillID();
	if(bill_id) {
		int    r = EditLine(bill_id, &oprno, goods_id, p_serial, qtty, accel_mode);
		if(r > 0) {
			UpdateTempTable(bill_id, oprno);
			Flags |= fWasUpdated;
			ok = 1;
		}
	}
	return ok;
}

#define INVDBQ_GOODSIDOFFS (sizeof(long) * 2)

/*static int InvItemQuickInputDialog(int initChar, SString & rBuf, double & rQtty)
{
	int    ok = -1;
	SString code;
	QuickSearchDialog * dlg = new QuickSearchDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(isalnum(initChar))
			code.CatChar(initChar);
		else
			code = rBuf;
		dlg->setCtrlString(CTL_INVITEM_CODE, code);
		dlg->setCtrlReal(CTL_INVITEM_QUANTITY, rQtty);
		if(code.NotEmpty() && isalnum(initChar)) {
			TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_INVITEM_CODE));
			CALLPTRMEMB(il, disableDeleteSelection(1));
		}
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_INVITEM_CODE, code);
			rQtty = dlg->getCtrlReal(CTL_INVITEM_QUANTITY);
			if(code.NotEmptyS())
				ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	rBuf = code;
	return ok;
}*/

int PPViewInventory::SelectGoodsByBarcode(int initChar, PPID arID, Goods2Tbl::Rec * pRec, double * pQtty, SString * pRetCode)
{
	class QuickSearchDialog : public TDialog {
	public:
		QuickSearchDialog() : TDialog(DLG_INVITEMQ)
		{
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmOK) && isCurrCtlID(CTL_INVITEM_CODE)) {
				BarcodeTbl::Rec  bc_rec;
				SString code;
				getCtrlString(CTL_INVITEM_CODE, code);
				if(GObj.SearchBy2dBarcode(code, &bc_rec, 0) > 0)
					setCtrlData(CTL_INVITEM_CODE, bc_rec.Code);
				{
					double qtty = getCtrlReal(CTL_INVITEM_QUANTITY);
					Goods2Tbl::Rec goods_rec;
					SString ret_code;
					if(code.NotEmptyS() && GObj.GetGoodsByBarcode(code, 0, &goods_rec, &qtty, &ret_code) > 0) {
						setCtrlReal(CTL_INVITEM_QUANTITY, qtty);
						selectCtrl(CTL_INVITEM_QUANTITY);
					}
				}
				clearEvent(event);
			}
			else {
				TDialog::handleEvent(event);
			}
		}
		PPObjGoods GObj;
	};
	int    ok = -1;
	double qtty = DEREFPTROR(pQtty, 1.0);
	SString code(DS.GetTLA().Lid.Barcode);
	QuickSearchDialog * dlg = new QuickSearchDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(isalnum(initChar))
			code.CatChar(initChar);
		dlg->setCtrlString(CTL_INVITEM_CODE, code);
		dlg->setCtrlReal(CTL_INVITEM_QUANTITY, qtty);
		if(code.NotEmpty() && isalnum(initChar)) {
			TInputLine * il = static_cast<TInputLine *>(dlg->getCtrlView(CTL_INVITEM_CODE));
			CALLPTRMEMB(il, disableDeleteSelection(1));
		}
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_INVITEM_CODE, code);
			qtty = dlg->getCtrlReal(CTL_INVITEM_QUANTITY);
			if(code.NotEmptyS() && GObj.GetGoodsByBarcode(code, arID, pRec, /*&qtty*/0, pRetCode)) {
				ASSIGN_PTR(pQtty, qtty);
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewInventory::SelectByBarcode(int initChar, PPViewBrowser * pBrw)
{
	const int  skip_dlg = BIN(P_BObj->GetConfig().Flags & BCF_ADDAUTOQTTYBYBRCODE);
	//const int  is_accel = BIN(CommonIoeFlags & INVOPF_ACCELADDITEMS);
	const PPID loc_id = NZOR(CommonLocID, LConfig.Location);
	const int  accel_mode = PPInventoryOpEx::Helper_GetAccelInputMode(CommonIoeFlags);

	int    ok = -1;
	int    r = 0;
	SString code;
	Goods2Tbl::Rec goods_rec;
	ReceiptTbl::Rec lot_rec;
	PPIDArray lot_list;
	double qtty = 0.0;
	const  PPID single_bill_id = Filt.GetSingleBillID();
	if(accel_mode == PPInventoryOpEx::accsliCodeAndQtty) {
		qtty = 1.0;
		r = PPViewInventory::SelectGoodsByBarcode(initChar, 0, &goods_rec, &qtty, &code);
	}
	else {
		qtty = 1.0;
		r = GObj.SelectGoodsByBarcode(initChar, 0, &goods_rec, &qtty, &code);
	}
	if(r > 0) {
		if(single_bill_id && (accel_mode || InvTbl.SearchByGoods(single_bill_id, goods_rec.ID, 0) < 0)) {
			TIDlgInitData tidi;
			tidi.GoodsID  = goods_rec.ID;
			tidi.Quantity = accel_mode ? qtty : 0.0;
			tidi.Flags    = 0;
			SETFLAG(tidi.Flags, TIDIF_AUTOQTTY, accel_mode);
			ok = AddItem(&tidi);
		}
		else if(pBrw) {
			pBrw->search2(&goods_rec.ID, CMPF_LONG, srchFirst, INVDBQ_GOODSIDOFFS);
			ok = 1;
		}
	}
	else if(P_BObj->SearchLotsBySerial(code, &lot_list) > 0) {
		const uint c = lot_list.getCount();
		PPID   lot_id = 0;
		if(c == 1) {
			lot_id = lot_list.get(0);
		}
		else {
			SArray * p_ary = SelLotBrowser::CreateArray();
			for(uint i = 0; i < c; i++) {
				lot_id = lot_list.get(i);
				if(P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
					if(!loc_id || lot_rec.LocID == loc_id) {
						double rest = 0.0;
						P_BObj->trfr->GetRest(lot_id, CommonDate, MAXLONG, &rest, 0);
						SelLotBrowser::AddItemToArray(p_ary, &lot_rec, CommonDate, rest, 1);
					}
				}
			}
			lot_id = 0;
			if(p_ary->getCount()) {
				SelLotBrowser::Entry * p_sel = 0;
				SelLotBrowser * p_brw = new SelLotBrowser(P_BObj, p_ary, 0, 0);
				if(ExecView(p_brw) == cmOK && (p_sel = (SelLotBrowser::Entry *)p_brw->getCurItem()) != 0)
					lot_id = p_sel->LotID;
				delete p_brw;
			}
		}
		if(lot_id && P_BObj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0) {
			if(single_bill_id && (accel_mode || InvTbl.SearchIdentical(single_bill_id, lot_rec.GoodsID, code, 0) < 0)) {
				BillTbl::Rec bill_rec;
				if(P_BObj->Fetch(single_bill_id, &bill_rec) > 0) {
					TIDlgInitData tidi;
					tidi.GoodsID  = lot_rec.GoodsID;
					P_BObj->trfr->GetRest(lot_rec.ID, bill_rec.Dt, MAXLONG, &tidi.Quantity, 0);
					tidi.Flags = 0;
					STRNSCPY(tidi.Serial, code);
					ok = AddItem(&tidi);
				}
			}
			else {
				pBrw->search2(&goods_rec.ID, CMPF_LONG, srchFirst, INVDBQ_GOODSIDOFFS);
				ok = 1;
			}
		}
	}
	else if(r != -1 && pBrw) {
		pBrw->bottom();
		ok = 1;
	}
	return ok;
}

int PPViewInventory::ConvertBillToBasket()
{
	int    ok = -1, r = 0;
	SelBasketParam param;
	param.SelPrice = 2;
	THROW(r = GetBasketByDialog(&param, "Inventory", DLG_GBDATAINV));
	if(r > 0) {
		if(P_OuterPack) {
			for(uint i = 0; i < P_OuterPack->InvList.getCount(); i++) {
				InventoryTbl::Rec & r_item = P_OuterPack->InvList.at(i);
				ILTI  ilti;
				ilti.GoodsID     = labs(r_item.GoodsID);
				ilti.UnitPerPack = r_item.UnitPerPack;
				ilti.Quantity    = fabs(r_item.Quantity);
				ilti.Price       = r_item.Price;
				THROW(param.Pack.AddItem(&ilti, 0, param.SelReplace));
			}
		}
		else {
			InventoryViewItem  item;
			PPWaitStart();
			for(InitIteration(); NextIteration(&item) > 0;) {
				ILTI  ilti;
				ilti.GoodsID     = labs(item.GoodsID);
				ilti.UnitPerPack = item.UnitPerPack;
				ilti.Quantity    = fabs(item.Quantity);
				ilti.Price       = item.Price;
				THROW(param.Pack.AddItem(&ilti, 0, param.SelReplace));
			}
			PPWaitStop();
		}
		THROW(GoodsBasketDialog(param, 1));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

int PPViewInventory::ConvertBasket(const PPBasketPacket & rPack, int sgoption, int priceByLastLot, int use_ta)
{
	int    ok = 1;
	const  PPID   bill_id = Filt.GetSingleBillID();
	if(bill_id) {
		SString temp_buf;
		uint   i;
		//ILTI * p_item = 0;
		InventoryTbl::Rec inv_rec;
		PPWaitStart();
		IterCounter cntr;
		PPBillPacket bpack;
		THROW(P_BObj->ExtractPacket(bill_id, &bpack) > 0);
		cntr.Init(rPack.Lots.getCount());
		THROW_PP_S(!bpack.Rec.StatusID || !P_BObj->CheckStatusFlag(bpack.Rec.StatusID, BILSTF_DENY_MOD), PPERR_BILLST_DENY_MOD, PPObjBill::MakeCodeString(&bpack.Rec, 1, temp_buf));
		for(SEnum en = InvTbl.Enum(bill_id); en.Next(&inv_rec) > 0;)
			THROW_PP(!(inv_rec.Flags & INVENTF_WRITEDOFF), PPERR_UPDWROFFINV);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			{
				long   ib_flags = PPObjBill::InvBlock::fPriceByLastLot;
				if(sgoption == PPObjBill::imsgoFail)
					ib_flags |= PPObjBill::InvBlock::fFailOnDup;
				PPObjBill::InvBlock blk(ib_flags);
				PPObjBill::InvItem inv_item;
				THROW(P_BObj->InitInventoryBlock(bill_id, blk));
				for(i = 0; i < rPack.Lots.getCount(); i++) {
					const ILTI & r_item = rPack.Lots.at(i);
					inv_item.Init(r_item.GoodsID, 0);
					inv_item.Qtty = r_item.Quantity;
					inv_item.Cost = r_item.Price;
					inv_item.Price = r_item.Price;
					inv_item.UnitPerPack = r_item.UnitPerPack;
					THROW(P_BObj->AcceptInventoryItem(blk, &inv_item, 0));
					PPWaitPercent(cntr.Increment());
				}
				THROW(P_BObj->RecalcInventoryStockRests(bpack.Rec.ID, 0, 0));
				THROW(P_BObj->RecalcInventoryDeficit(&bpack.Rec, 0));
				{
					InventoryTotal total;
					InventoryFilt  flt;
					flt.SetSingleBillID(bill_id);
					THROW(InvTbl.CalcTotal(&flt, &total));
					bpack.Rec.Amount = BR2(total.Amount);
					THROW(P_BObj->TurnInventory(&bpack, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int PPViewInventory::ConvertBasketToBill()
{
	int    ok = -1, r = 0;
	PPObjGoodsBasket gb_obj;
	PPBasketCombine basket;
	TDialog * p_dlg = 0;
	THROW(r = gb_obj.SelectBasket(basket));
	if(r > 0) {
		PPObjBill::InvMovSgo sgo;
		THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_BSKTTOINV))));
		p_dlg->setCtrlUInt16(CTL_BSKTTOINV_SGOPTION,  0);
		p_dlg->setCtrlUInt16(CTL_BSKTTOINV_RULEPRICE, 0);
		if(ExecView(p_dlg) == cmOK) {
			ushort v = p_dlg->getCtrlUInt16(CTL_BSKTTOINV_SGOPTION);
			switch(v) {
				case 0: sgo = PPObjBill::imsgoAdd; break;
				case 1: sgo = PPObjBill::imsgoSkip; break;
				case 2: sgo = PPObjBill::imsgoFail; break;
				default: sgo = PPObjBill::imsgoAdd; break;
			}
			v = p_dlg->getCtrlUInt16(CTL_BSKTTOINV_RULEPRICE);
			THROW(ok = ConvertBasket(basket.Pack, sgo, (int)v, 1));
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	return ok;
}
//
//
//
static int AutoFillInventryDlg(AutoFillInvFilt * pFilt)
{
	class AutoFillInventryFiltDialog : public TDialog {
		DECL_DIALOG_DATA(AutoFillInvFilt);
	public:
		AutoFillInventryFiltDialog() : TDialog(DLG_FLTAFINV)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			if(Data.BillID) {
				SString info_buf;
				BillTbl::Rec bill_rec;
				if(BillObj->Search(Data.BillID, &bill_rec) > 0) {
					PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName, info_buf);
				}
				else
					ideqvalstr(Data.BillID, info_buf);
			}
			SetupPPObjCombo(this, CTLSEL_FLTAFINV_GGRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
			AddClusterAssocDef(CTL_FLTAFINV_METHOD,  0, PPInventoryOpEx::afmPresents);
			AddClusterAssoc(CTL_FLTAFINV_METHOD,  1, PPInventoryOpEx::afmAll);
			AddClusterAssoc(CTL_FLTAFINV_METHOD,  2, PPInventoryOpEx::afmPrev);
			AddClusterAssoc(CTL_FLTAFINV_METHOD,  3, PPInventoryOpEx::afmByCurLotRest);
			SetClusterData(CTL_FLTAFINV_METHOD, Data.Method);
			AddClusterAssoc(CTL_FLTAFINV_FLAGS, 0, AutoFillInvFilt::fFillWithZeroQtty);
			AddClusterAssoc(CTL_FLTAFINV_FLAGS, 1, AutoFillInvFilt::fRestrictZeroRestWithMtx); // @v10.5.6
			AddClusterAssoc(CTL_FLTAFINV_FLAGS, 2, AutoFillInvFilt::fExcludeZeroRestPassiv); // @v11.1.2
			SetClusterData(CTL_FLTAFINV_FLAGS, Data.Flags);
			{
				PPObjGoods goods_obj;
				DisableClusterItem(CTL_FLTAFINV_FLAGS, 1, !goods_obj.GetConfig().MtxQkID); // @v10.5.6
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			getCtrlData(CTLSEL_FLTAFINV_GGRP, &Data.GoodsGrpID);
			GetClusterData(CTL_FLTAFINV_METHOD, &Data.Method);
			GetClusterData(CTL_FLTAFINV_FLAGS,  &Data.Flags);
			ASSIGN_PTR(pData, Data);
			return 1;
		}
	};
	DIALOG_PROC_BODY(AutoFillInventryFiltDialog, pFilt);
}

AutoFillInvFilt::AutoFillInvFilt() : BillID(0), GoodsGrpID(0), Method(0), Flags(0), DueDate(ZERODATE)
{
}

int PPViewInventory::Build()
{
	int    ok = -1;
	const  PPID bill_id = Filt.GetSingleBillID();
	if(P_BObj && bill_id) {
		int    r = 0;
		SString temp_buf;
		BillTbl::Rec bill_rec;
		AutoFillInvFilt filt;
		PPInventoryOpEx ioe;
		THROW(P_BObj->Search(bill_id, &bill_rec) > 0);
		THROW(P_BObj->P_OpObj->FetchInventoryData(bill_rec.OpID, &ioe));
		THROW_PP_S(!bill_rec.StatusID || !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_DENY_MOD), PPERR_BILLST_DENY_MOD, PPObjBill::MakeCodeString(&bill_rec, 1, temp_buf));
		filt.BillID = bill_id;
		filt.Method = ioe.AutoFillMethod;
		THROW((r = AutoFillInventryDlg(&filt)));
		if(r > 0) {
			PPWaitStart();
			filt.DueDate = bill_rec.DueDate;
			THROW(P_BObj->AutoFillInventory(&filt));
			UpdateTempTable(bill_id, 0);
			PPWaitStop();
			Flags |= fWasUpdated;
			ok = 1;
		}
	}
	CATCHZOKPPERR
	return ok;
}

void PPViewInventory::ViewTotal()
{
	PPIDArray id_list;
	Filt.BillList.Get(id_list);
	BillObj->ViewInventoryTotal(id_list, &Filt);
}

int PPViewInventory::Print(const void *)
{
	Filt.Flags |= InventoryFilt::fShowAbsenseGoods;
	int    ok = Helper_Print(REPORT_INVENT);
	Filt.Flags &= ~InventoryFilt::fShowAbsenseGoods;
	return ok;
}

int PPViewInventory::Correct()
{
	int    ok = -1;
	long   rif_flags = 0;
	TDialog * dlg = 0;
	const  PPID bill_id = Filt.GetSingleBillID();
	if(bill_id && P_BObj) {
		THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_SELINVCORR))));
		dlg->AddClusterAssoc(CTL_SELINVCORR_FLAGS, 0, PPObjBill::rifRest);
		dlg->AddClusterAssoc(CTL_SELINVCORR_FLAGS, 1, PPObjBill::rifPrice);
		dlg->AddClusterAssoc(CTL_SELINVCORR_FLAGS, 2, PPObjBill::rifAverage);
		dlg->DisableClusterItem(CTL_SELINVCORR_FLAGS, 2, !(DS.CheckExtFlag(ECF_AVERAGE) && PPMaster));
		dlg->SetClusterData(CTL_SELINVCORR_FLAGS, rif_flags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			dlg->GetClusterData(CTL_SELINVCORR_FLAGS, &rif_flags);
			if(!(rif_flags & (PPObjBill::rifRest|PPObjBill::rifPrice|PPObjBill::rifAverage))) {
				PPError();
			}
			else
				ok = 1;
		}
		if(ok > 0) {
			THROW(P_BObj->RecalcInventoryStockRests(bill_id, rif_flags, 1));
			Flags |= fWasUpdated;
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PPViewInventory::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	BrwHdr hdr;
	if(pHdr)
		hdr = *static_cast<const BrwHdr *>(pHdr);
	else
		MEMSZERO(hdr);
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				ok = -1;
				if(pHdr)
					ok = SelectByBarcode(*static_cast<const char *>(pHdr), pBrw);
				break;
			case PPVCMD_SELECTBYCODE:
				ok = SelectByBarcode(0, pBrw);
				break;
			case PPVCMD_ADDITEM:
				ok = AddItem(0);
				break;
			case PPVCMD_SELECTBYNAME:
			case PPVCMD_ADDITEMEXT:
				ok = -1;
				{
					const PPID bill_id = Filt.GetSingleBillID();
					if(bill_id) {
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(bill_id, &bill_rec) > 0) {
							int    sel_by_name = BIN(ppvCmd == PPVCMD_SELECTBYNAME);
							StrAssocArray goods_list;
							if(sel_by_name) {
								SString sub;
								PPInputStringDialogParam isd_param;
								isd_param.P_Wse = new TextHistorySelExtra("goodsnamefragment-common"); // @v10.7.8
								PPLoadText(PPTXT_SELGOODSBYNAME, isd_param.Title);
								if(InputStringDialog(&isd_param, sub) > 0 && sub.NotEmptyS()) {
									if(!GObj.P_Tbl->GetListBySubstring(sub, &goods_list, -1)) {
										PPError();
										sel_by_name = 0;
									}
								}
								else
									sel_by_name = 0;
							}
							long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags(); // @v10.7.7
							SETFLAG(egsd_flags, ExtGoodsSelDialog::fExistsOnly, (Filt.Flags & InventoryFilt::fSelExistsGoodsOnly));
							SETFLAG(egsd_flags, ExtGoodsSelDialog::fByName, Flags & fSelGoodsByName); // В диалоге будут полные наименования //
							ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(bill_rec.OpID, 0, egsd_flags);
							if(CheckDialogPtrErr(&dlg)) {
								if(sel_by_name)
									dlg->setSelectionByGoodsList(&goods_list);
								while(ExecView(dlg) == cmOK) {
									TIDlgInitData tidi;
									if(dlg->getDTS(&tidi) > 0) {
										ok = AddItem(&tidi);
										if(ok > 0)
											pBrw->Update();
									}
								}
								delete dlg;
							}
						}
					}
				}
				break;
			case PPVCMD_EDITITEM:
				ok = -1;
				if(Filt.HasSubst()) {
					ok = Detail(pHdr, pBrw);
				}
				else if(hdr.BillID && hdr.OprNo) {
					int r = EditLine(hdr.BillID, &hdr.OprNo, 0, 0, 0.0, 0);
					if(r > 0 || r == -2) { // -2 - был заменен ид товара
						Flags |= fWasUpdated;
						ok = 1;
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = -1;
				if(hdr.BillID && hdr.OprNo) {
					BillTbl::Rec bill_rec;
					if(P_BObj->Search(hdr.BillID, &bill_rec) > 0) {
						if(bill_rec.StatusID && P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_DENY_MOD)) {
							SString temp_buf;
							ok = (PPError(PPERR_BILLST_DENY_MOD, PPObjBill::MakeCodeString(&bill_rec, 1, temp_buf)), 0);
						}
						else if(P_BObj->P_Tbl->EnumLinks(hdr.BillID, 0, BLNK_ALL) > 0)
							ok = (PPError(PPERR_UPDWROFFINV, 0), 0);
						else if(InvTbl.Set(hdr.BillID, &hdr.OprNo, 0, 1) > 0) {
							Flags |= fWasUpdated;
							ok = 1;
						}
					}
				}
				break;
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(hdr.BillID && hdr.OprNo) {
					InventoryTbl::Rec inv_rec;
					if(InvTbl.Search(hdr.BillID, hdr.OprNo, &inv_rec) > 0 && inv_rec.GoodsID) {
						if(GObj.Edit(&inv_rec.GoodsID, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_BUILD:
				ok = Build();
				break;
			case PPVCMD_RECALCREST:
			case PPVCMD_RECALCPRICE:
				ok = Correct();
				break;
			case PPVCMD_RECALCDFCT:
				if(Filt.BillList.GetCount() && CONFIRM(PPCFM_INVRECALCCSESSDFCT)) {
					for(uint i = 0; ok && i < Filt.BillList.GetCount(); i++) {
						const PPID bill_id = Filt.BillList.Get().get(i);
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(bill_id, &bill_rec) > 0) {
							if(P_BObj->RecalcInventoryDeficit(&bill_rec, 1)) {
								Flags |= fWasUpdated;
								ok = 1;
							}
							else
								ok = PPErrorZ();
						}
					}
				}
				break;
			case PPVCMD_ADDFROMBASKET:
				ok = -1;
				if(ConvertBasketToBill() > 0) {
					Flags |= fWasUpdated;
					ok = 1;
				}
				break;
			case PPVCMD_ADDTOBASKET:
				ok = -1;
				ConvertBillToBasket();
				break;
		}
	}
	return ok;
}

int PPViewInventory::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
    int    ok = -1;
	if(Filt.HasSubst()) {
		const PPID subst_id = pHdr ? *static_cast<const long *>(pHdr) : 0;
		if(subst_id) {
			InventoryFilt detail_filt;
			detail_filt = Filt;
			detail_filt.Sgb.S = SubstGrpBill::sgbNone;
			detail_filt.Sgg = sggNone;
			PPIDArray _list;
			if(!!Filt.Sgb) {
                Bsp.AsscList.GetListByKey(subst_id, _list);
				detail_filt.BillList.Set(&_list);
			}
			else {
				Gsl.GetGoodsBySubstID(subst_id, &detail_filt.GoodsList);
			}
            PPView::Execute(PPVIEW_INVENTORY, &detail_filt, PPView::exefModeless, 0);
		}
	}
    return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewInventory * p_view = static_cast<PPViewInventory *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int PPViewInventory::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  int is_subst = Filt.HasSubst();
		if(!is_subst) {
			const  BrowserDef * p_def = pBrw->getDef();
			if(col >= 0 && col < p_def->getCountI()) {
				const BroColumn & r_col = p_def->at(col);
				const PPViewInventory::BrwHdr * p_hdr = static_cast<const PPViewInventory::BrwHdr *>(pData);
				if(r_col.OrgOffs == 1) { // ID
					if(p_hdr->Flags & INVENTF_WRITEDOFF) {
						pStyle->Color = GetColorRef(SClrDodgerblue);
						pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
					else if(p_hdr->Flags & (INVENTF_LACK|INVENTF_SURPLUS)) {
						pStyle->Color = GetColorRef(SClrOrange);
						pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
				else if(r_col.OrgOffs == 5) { // Rest
					if(p_hdr->Flags & INVENTF_GENAUTOLINE) {
						pStyle->Color = GetColorRef(SClrBlue);
						pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
						ok = 1;
					}
				}
				else if(r_col.OrgOffs == 10) { // Difference
					if(!(CConfig.Flags2 & CCFLG2_HIDEINVENTORYSTOCK)) { // @v10.9.12
						if(p_hdr->Flags & INVENTF_LACK) {
							pStyle->Color = GetColorRef(SClrRed);
							pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
							ok = 1;
						}
						else if(p_hdr->Flags & INVENTF_SURPLUS) {
							pStyle->Color = GetColorRef(SClrGreen);
							pStyle->Flags |= BrowserWindow::CellStyle::fCorner;
							ok = 1;
						}
					}
				}
				// @v10.5.8 {
				else if(r_col.OrgOffs == 14) { // Status
					//if(p_hdr->Flags & INVENTF_WRITEDOFF) {
					if(CheckInventoryLineForWrOff(Filt.fWrOff, p_hdr->BillID, p_hdr->Flags, /*diffQtty*/0.0)) {
						pStyle->Color = GetColorRef(SClrLightblue);
						ok = 1;
					}
					else if(p_hdr->Flags & (INVENTF_LACK|INVENTF_SURPLUS)) {
						pStyle->Color = GetColorRef(SClrYellow);
						ok = 1;
					}
					else {
						pStyle->Color = GetColorRef(SClrIvory);
						ok = 1;
					}
				}
				// } @v10.5.8
			}
		}
	}
	return ok;
}

/*virtual*/void PPViewInventory::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		// @v11.1.8 {
		if(!Filt.GetSingleBillID() && !Filt.HasSubst()) {
			pBrw->InsColumn(0, "@billno", 15, 0, 0, 0);
			pBrw->InsColumn(1, "@billdate", 16, 0, 0, 0);
		}
		// } @v11.1.8 
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

static IMPL_DBE_PROC(dbqf_invlnwroff_iiir)
{
	long   flt_flags = params[0].lval;
	long   bill_id   = params[1].lval;
	long   ln_flags  = params[2].lval;
	double diff_qtty = params[3].rval;
	long   r = CheckInventoryLineForWrOff(flt_flags, bill_id, ln_flags, diff_qtty);
	result->init(r);
}

DBQuery * PPViewInventory::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncInvLnWrOff, BTS_INT, dbqf_invlnwroff_iiir, 4, BTS_INT, BTS_INT, BTS_INT, BTS_REAL);
	DBQuery * q  = 0;
	InventoryTbl * it = 0;
	TempInventorySubstTbl * st = 0;
	TempDoubleIDTbl * p_tord = 0;
	size_t tbl_count = 0;
	DBTable * tbl_l[2];
	DBQ  * dbq = 0;
	DBE  * dbe_tmp1 = 0;
	DBE  * dbe_tmp2 = 0;
	DBE    dbe_diffqtty;
	DBE    dbe_goods;
	DBE    dbe_barcode;
	DBE    dbe_strgloc;
	DBE    dbe_status;
	DBE    dbe_wroff;
	DBE    dbe_bill_code; // @v11.1.8
	DBE    dbe_bill_date; // @v11.1.8
	DBE    dbe_empty;
	uint   brw_id = 0;
	const  PPID single_bill_id = Filt.GetSingleBillID();
	const  int is_subst = Filt.HasSubst();
	{
		dbe_empty.init();
		dbe_empty.push(static_cast<DBFunc>(PPDbqFuncPool::IdEmpty));
	}
	if(is_subst) {
		assert(P_TempSubstTbl);
		brw_id = BROWSER_INVNTRYLINESSUBST;
		THROW(CheckTblPtr(st = new TempInventorySubstTbl(P_TempSubstTbl->GetName())));
		if(CConfig.Flags2 & CCFLG2_HIDEINVENTORYSTOCK) {
			q = & select(
				st->GoodsID,       // #00
				st->Name,          // #01
				st->Quantity,      // #02
				st->SumPrice,      // #03
				dbe_empty,         // #04
				dbe_empty,         // #05
				dbe_empty,         // #06
				dbe_empty,         // #07
				0L);
		}
		else {
			q = & select(
				st->GoodsID,       // #00
				st->Name,          // #01
				st->Quantity,      // #02
				st->SumPrice,      // #03
				st->StockRest,     // #04
				st->SumStockPrice, // #05
				st->DiffQtty,      // #06
				st->DiffPrice,     // #07
				0L);
		}
		q->from(st, 0L);
	}
	else {
		brw_id = BROWSER_INVNTRYLINES;
		tbl_l[0] = 0;
		tbl_l[1] = 0;
		THROW(CheckTblPtr(it = new InventoryTbl(P_TempTbl ? P_TempTbl->GetName().cptr() : 0)));
		if(P_TempOrd)
			THROW(CheckTblPtr(p_tord = new TempDoubleIDTbl(P_TempOrd->GetName())));
		dbe_tmp1 = & (it->Quantity * it->Price);
		dbe_tmp2 = & (it->CSesDfctQtty * it->CSesDfctPrice);
		{
			dbe_diffqtty.init();
			dbe_diffqtty.push(it->Flags);
			dbe_diffqtty.push(it->DiffQtty);
			dbe_diffqtty.push(static_cast<DBFunc>(PPDbqFuncPool::IdInventDiffQtty));
		}
		PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, it->GoodsID);
		PPDbqFuncPool::InitObjNameFunc(dbe_barcode, PPDbqFuncPool::IdGoodsSingleBarcode, it->GoodsID);
		{
			dbe_strgloc.init();
			dbe_strgloc.push(it->BillID);
			dbe_strgloc.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillFrghtStrgLoc));
		}
		{
			dbe_status.init();
			dbe_status.push(it->Flags);
			dbe_status.push(it->BillID);
			dbe_status.push(static_cast<DBFunc>(PPDbqFuncPool::IdInventLnStatus));
		}
		// @v11.1.8 {
		PPDbqFuncPool::InitObjNameFunc(dbe_bill_code, PPDbqFuncPool::IdObjCodeBill, it->BillID);
		{
			dbe_bill_date.init();
			dbe_bill_date.push(it->BillID);
			dbe_bill_date.push(static_cast<DBFunc>(PPDbqFuncPool::IdBillDate));
		}		
		// } @v11.1.8
		if(p_tord)
			tbl_l[tbl_count++] = p_tord;
		tbl_l[tbl_count++] = it;
		q = & (select(
			it->BillID,     // #00
			it->OprNo,      // #01
			it->GoodsID,    // #02
			it->Flags,      // #03 
			dbe_goods,      // #04 
			it->Quantity,   // #05 
			it->Price,      // #06 
			it->WrOffPrice, // #07 
			*dbe_tmp1,      // #08 
			*dbe_tmp2,      // #09 
			(CConfig.Flags2 & CCFLG2_HIDEINVENTORYSTOCK) ? dbe_empty : dbe_diffqtty, // #10
			//dbe_diffqtty,   // #10 
			it->Serial,     // #11 
			dbe_barcode,    // #12 
			dbe_strgloc,    // #13 
			dbe_status,     // #14 // @v10.5.8
			dbe_bill_code,  // #15 // @v11.1.8
			dbe_bill_date,  // #16 // @v11.1.8
			0L).from(tbl_l[0], tbl_l[1], 0L));
		ZDELETE(dbe_tmp1);
		ZDELETE(dbe_tmp2);
		if(!P_TempTbl) {
			double minv = Filt.MinVal;
			double maxv = Filt.MaxVal;
			dbq = & (it->BillID == single_bill_id);
			if(CheckXORFlags(Filt.Flags, InventoryFilt::fWrOff, InventoryFilt::fUnwrOff)) {
				//dbq = ppcheckflag(dbq, it->Flags, INVENTF_WRITEDOFF, (Filt.Flags & InventoryFilt::fWrOff) ? +1 : -1);  //v10.5.6 BIN(Filt.Flags & InventoryFilt::fWrOff) => (Filt.Flags & InventoryFilt::fWrOff) ? +1 : -1
				// @v10.5.9 {
				dbe_wroff.init();
				dbe_wroff.push(dbconst(Filt.Flags));
				dbe_wroff.push(it->BillID);
				dbe_wroff.push(it->Flags);
				dbe_wroff.push(it->DiffQtty);
				dbe_wroff.push(static_cast<DBFunc>(PPViewInventory::DynFuncInvLnWrOff));
				dbq = &(*dbq && dbe_wroff == 1L);
				// } @v10.5.9
			}
			if(Filt.Flags & (InventoryFilt::fLack|InventoryFilt::fSurplus))
				if(Filt.Flags & InventoryFilt::fLack && Filt.Flags & InventoryFilt::fSurplus)
					dbq = & (*dbq && it->DiffQtty > 0.0);
				else {
					dbq = ppcheckflag(dbq, it->Flags, INVENTF_SURPLUS, BIN(Filt.Flags & InventoryFilt::fSurplus));
					dbq = ppcheckflag(dbq, it->Flags, INVENTF_LACK, BIN(Filt.Flags & InventoryFilt::fLack));
				}
			if(maxv >= minv && maxv > 0.0) {
				if(minv < 0.0)
					minv = 0.0;
				if(Filt.Flags & InventoryFilt::fPctVal) {
					if(minv == maxv) {
						minv = MAX(0.0, minv - 1.0);
						maxv = maxv + 1.0;
					}
					dbq = & (*dbq && realrange(it->DiffPctQtty, minv, maxv));
				}
				else
					dbq = & (*dbq && realrange(it->DiffQtty, minv, maxv));
			}
		}
		if(P_TempOrd) {
			dbq = & (*dbq && it->BillID == p_tord->PrmrID && it->OprNo == p_tord->ScndID);
			q->where(*dbq).orderBy(p_tord->Name, 0L);
		}
		else
			q->where(*dbq).orderBy(it->BillID, it->OprNo, 0L);
	}
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		BillTbl::Rec bill_rec;
		if(single_bill_id && P_BObj->Fetch(single_bill_id, &bill_rec) > 0) {
			PPObjBill::MakeCodeString(&bill_rec, 1, *pSubTitle);
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete it;
			delete p_tord;
			delete st;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}
//
// Implementation of PPALDD_Invent
//
PPALDD_CONSTRUCTOR(Invent)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(Invent)
{
	Destroy();
}

int PPALDD_Invent::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Inventory, rsrv);
	H.BillID   = p_filt->GetSingleBillID();
	H.Flags    = p_filt->Flags;
	H.fZeroByDefault = p_v->GetZeroByDefaultStatus();
	H.fLack    = BIN(p_filt->Flags & InventoryFilt::fLack);
	H.fSurplus = BIN(p_filt->Flags & InventoryFilt::fSurplus);
	H.fAmtVal  = BIN(p_filt->Flags & InventoryFilt::fAmtVal);
	H.fPctVal  = BIN(p_filt->Flags & InventoryFilt::fPctVal);
	H.fWrOff   = BIN(p_filt->Flags & InventoryFilt::fWrOff);
	H.fUnwrOff = BIN(p_filt->Flags & InventoryFilt::fUnwrOff);
	H.FltGrpID = p_filt->GoodsGrpID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_Invent::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Inventory);
}

int PPALDD_Invent::NextIteration(PPIterID iterId)
{
	long   n = I.LineNo+1;
	START_PPVIEW_ALDD_ITER(Inventory);
	I.ItemBillID    = item.BillID; // @v10.7.1
	I.LineNo        = item.OprNo;
	I.WritedOff     = BIN(item.Flags & INVENTF_WRITEDOFF);
	I.ItemID        = item.GoodsID;
	I.ItemFlags     = item.Flags; // @v10.7.3
	I.AutoLine      = (item.Flags & INVENTF_AUTOLINE) ? ((item.Flags & INVENTF_GENAUTOLINE) ? 1 : 2) : 0;
	I.UnitPerPack   = item.UnitPerPack;
	I.Quantity      = item.Quantity;
	I.StockRest     = item.StockRest;
	I.Price = item.Price;
	I.StockPrice    = item.StockPrice;
	I.WrOffPrice    = item.WrOffPrice;
	I.DiffSign      = (item.Flags & INVENTF_SURPLUS) ? 1 : ((item.Flags & INVENTF_LACK) ? -1 : 0);
	I.DiffQtty      = INVENT_DIFFSIGN(item.Flags) ? (INVENT_DIFFSIGN(item.Flags)*item.DiffQtty) : item.DiffQtty;
	I.DiffPctQtty   = item.DiffPctQtty;
	I.UnwritedDiff  = item.UnwritedDiff;
	I.CSesDfctQtty  = item.CSesDfctQtty;
	I.CSesDfctPrice = item.CSesDfctPrice;
	item.FullGrpName.CopyTo(I.ExtGrpName, sizeof(I.ExtGrpName));
	STRNSCPY(I.Serial, item.Serial);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_Invent::Destroy() { DESTROY_PPVIEW_ALDD(Inventory); }
