// EXTGSEL.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// Descr: Таблица подбора партии для вставки в расходный товарный документ
//
class PickLotBrowser : public BrowserWindow {
public:
	struct Entry {
		PPID   LotID;          // #0
		LDATE  Date;           // #4
		PPID   GoodsID;        // #8
		double Rest;           // #12
		double Price;          // #20
		LDATE  Expiry;         // #28
		char   GoodsName[128]; // #32
		char   Barcode[32];    // #160
		char   Serial[32];     // #192
	};
	static SArray * CreateArray();
	static int FASTCALL AddItemToArray(SArray * pAry, const ReceiptTbl::Rec * pRec, LDATE billDate, double rest);
	static int SortArray(SArray * pAry, int f);
	PickLotBrowser(SArray * pList);
private:
	DECL_HANDLE_EVENT;
	PPObjBill * P_BObj;
};
//
//
/*static*/SArray * PickLotBrowser::CreateArray()
{
	SArray * p_ary = new SArray(sizeof(Entry));
	if(!p_ary)
		PPSetErrorNoMem();
	return p_ary;
}

/*static*/int FASTCALL PickLotBrowser::AddItemToArray(SArray * pAry, const ReceiptTbl::Rec * pRec, LDATE billDate, double rest)
{
	int    ok = 1;
	if(!pAry->lsearch(&pRec->ID, 0, CMPF_LONG, offsetof(Entry, LotID))) {
		PPObjBill * p_bobj = BillObj;
		int    sr = 0;
		SString serial;
		ReceiptTbl::Rec lot_rec = *pRec;
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		Entry  entry;
		MEMSZERO(entry);
		entry.LotID = lot_rec.ID;
		entry.Date  = lot_rec.Dt;
		if(billDate)
			THROW(p_bobj->trfr->GetLotPrices(&lot_rec, billDate));
		entry.Price = lot_rec.Price;
		entry.Rest  = rest;
		entry.GoodsID = labs(lot_rec.GoodsID);
		entry.Expiry  = lot_rec.Expiry;
		if(goods_obj.Fetch(labs(lot_rec.GoodsID), &goods_rec) > 0) {
			STRNSCPY(entry.GoodsName, goods_rec.Name);
		}
		if(goods_obj.P_Tbl->FetchSingleBarcode(lot_rec.GoodsID, serial.Z()) > 0)
			serial.CopyTo(entry.Barcode, sizeof(entry.Barcode));
		sr = p_bobj->GetSerialNumberByLot(lot_rec.ID, serial = 0, 1);
		serial.CopyTo(entry.Serial, sizeof(entry.Serial));
		THROW_SL(pAry->insert(&entry));
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(PickLotBrowser_Entry_GoodsName, i1, i2)
{
	const PickLotBrowser::Entry * e1 = static_cast<const PickLotBrowser::Entry *>(i1);
	const PickLotBrowser::Entry * e2 = static_cast<const PickLotBrowser::Entry *>(i2);
	return stricmp866(e1->GoodsName, e2->GoodsName);
}

/*static*/int PickLotBrowser::SortArray(SArray * pAry, int f)
{
	return pAry ? (pAry->sort(PTR_CMPFUNC(PickLotBrowser_Entry_GoodsName)), 1) : 0;
}

PickLotBrowser::PickLotBrowser(SArray * pList) : BrowserWindow(BROWSER_PICKLOT, pList), P_BObj(BillObj)
{
}

IMPL_HANDLE_EVENT(PickLotBrowser)
{
	const PickLotBrowser::Entry * p_entry = 0;
	BrowserWindow::handleEvent(event);
	if(event.isCmd(cmaEdit)) {
		if(IsInState(sfModal))
			endModal(cmOK);
	}
	else if(event.isKeyDown(kbF3)) {
		if(P_BObj->CheckRights(BILLRT_ACCSCOST)) {
			p_entry = static_cast<const PickLotBrowser::Entry *>(getCurItem());
			if(p_entry && p_entry->LotID)
				ViewOpersByLot(p_entry->LotID, 0);
		}
	}
	else if(event.isKeyDown(kbF4)) {
		p_entry = static_cast<const PickLotBrowser::Entry *>(getCurItem());
		if(p_entry && p_entry->LotID)
			P_BObj->EditLotExtData(p_entry->LotID);
	}
	else if(event.isKeyDown(kbCtrlF6)) {
		p_entry = static_cast<const PickLotBrowser::Entry *>(getCurItem());
		if(p_entry && p_entry->LotID)
			EditObjTagValList(PPOBJ_LOT, p_entry->LotID, 0);
	}
	else if(event.isKeyDown(kbCtrlF3)) {
		p_entry = static_cast<const PickLotBrowser::Entry *>(getCurItem());
		if(p_entry)
			P_BObj->EditLotSystemInfo(p_entry->LotID);
	}
	else
		return;
	clearEvent(event);
}

PickLotParam::PickLotParam(int mode) : Mode(mode), LocID(0), Price(0.0)
{
}

int PickLot(PickLotParam & rParam, TIDlgInitData * pResult)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	PickLotBrowser * p_brw = 0;
	ReceiptCore & rcpt = p_bobj->trfr->Rcpt;
	SArray * p_list = PickLotBrowser::CreateArray();
	LotArray lot_list;
	THROW(p_list);
	if(rParam.Mode == rParam.mByGoodsList) {
		PPIDArray goods_id_list;
		for(uint i = 0; i < rParam.GoodsList.getCount(); i++) {
			goods_id_list.add(rParam.GoodsList.at_WithoutParent(i).Id);
		}
		goods_id_list.sortAndUndup();
		THROW(rcpt.GetListOfOpenedLots(goods_id_list, rParam.LocID, &lot_list));
	}
	else if(rParam.Mode == rParam.mBySerial) {
		if(rParam.Text.NotEmptyS()) {
			int    srch_substr = BIN(rParam.Text.C(0) == '*');
			size_t len = rParam.Text.ShiftLeftChr('*').Len();
			if(len >= 3) {
				const StrAssocArray * p_full_list = 0;
				p_full_list = p_bobj->GetFullSerialList();
				if(p_full_list) {
					const uint c = p_full_list->getCount();
					for(uint i = 0; i < c; i++) {
						StrAssocArray::Item item = p_full_list->at_WithoutParent(i);
						int    r = ExtStrSrch(item.Txt, rParam.Text, 0);
						if(r > 0) {
							ReceiptTbl::Rec lot_rec;
							if(rcpt.Search(item.Id, &lot_rec) > 0 && !lot_rec.Closed && (!rParam.LocID || lot_rec.LocID == rParam.LocID)) {
								THROW_SL(lot_list.insert(&lot_rec));
							}
						}
					}
					p_bobj->ReleaseFullSerialList(p_full_list);
				}
			}
		}
	}
	else if(rParam.Mode == rParam.mByPrice) {
		ReceiptTbl::Key3 k;
		BExtQuery q(&rcpt, 3, 128);
		DBQ * dbq = &(rcpt.Closed == 0L && rcpt.GoodsID > 0L);
		if(rParam.LocID)
			dbq = &(*dbq && rcpt.LocID == rParam.LocID);
		{
			double ri = 0.0;
			if(modf(fabs(rParam.Price), &ri) != 0.0)
				dbq = &(*dbq && rcpt.Price > R2(ri) && rcpt.Price < R2(ri+1));
			else
				dbq = &(*dbq && rcpt.Price == R2(rParam.Price));
		}
		q.selectAll().where(*dbq);
		MEMSZERO(k);
		for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
			if(R2(rcpt.data.Price) == R2(rParam.Price)) {
				THROW_SL(lot_list.insert(&rcpt.data));
			}
		}
	}
	if(lot_list.getCount()) {
		for(uint i = 0; i < lot_list.getCount(); i++) {
			const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
			THROW(PickLotBrowser::AddItemToArray(p_list, &r_lot_rec, ZERODATE, r_lot_rec.Rest));
		}
		PickLotBrowser::SortArray(p_list, 0);
		{
			THROW_MEM(p_brw = new PickLotBrowser(p_list));
			if(rParam.SubTitle.NotEmpty())
                p_brw->setSubTitle(rParam.SubTitle);
			if(ExecView(p_brw) == cmOK) {
				const PickLotBrowser::Entry * p_sel = static_cast<const PickLotBrowser::Entry *>(p_brw->getCurItem());
				if(p_sel) {
					if(pResult) {
						pResult->LotID = p_sel->LotID;
						pResult->GoodsID = p_sel->GoodsID;
						STRNSCPY(pResult->Serial, p_sel->Serial);
					}
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	if(p_brw)
		delete p_brw;
	else
		ZDELETE(p_list);
	return ok;
}
//
//
//
TIDlgInitData::TIDlgInitData()
{
	THISZERO();
}

void FASTCALL TIDlgInitData::SetTiSign(int tisign)
{
	switch(tisign) {
		case TISIGN_MINUS: ModifMode = mmExpend; break;
		case TISIGN_PLUS: ModifMode = mmIncoming; break;
		case TISIGN_RECOMPLETE: ModifMode = mmRecompl; break;
		default: ModifMode = mmUndef; break;
	}
}

int TIDlgInitData::GetTiSign() const
{
	switch(ModifMode) {
		case mmExpend: return TISIGN_MINUS;
		case mmIncoming: return TISIGN_PLUS;
		case mmRecompl: return TISIGN_RECOMPLETE;
		default: return TISIGN_UNDEF;
	}
}

#define MAX_GOODS_RECS   50000
#define MIN_WORDSEL_SYMB 4

/*static*/long ExtGoodsSelDialog::GetDefaultFlags()
{
	long   f = 0;
	// @v11.2.6 UserInterfaceSettings uiset;
	// @v11.2.6 uiset.Restore();
	const long uif = APPL->GetUiSettings().Flags;
	if(uif & UserInterfaceSettings::fExtGoodsSelHideGenerics)
		f |= fHideGeneric;
	if(uif & UserInterfaceSettings::fExtGoodsSelMainName)
		f |= ExtGoodsSelDialog::fByName;
	return f;
}

ExtGoodsSelDialog::ExtGoodsSelDialog(PPID _op, PPID initGoodsID, long flags) : TDialog(DLG_EXTGSEL), P_ObjTkn(0), Flags(0), OpID(_op),
	ArID(0), InnerSelectionID(0), Price(0.0)
{
	const  PPConfig & r_cfg = LConfig;
	SETFLAGBYSAMPLE(Flags, fByName, flags);
	SETFLAGBYSAMPLE(Flags, fExistsOnly, flags);
	SETFLAGBYSAMPLE(Flags, fForceExhausted, flags);
	SETFLAGBYSAMPLE(Flags, fHidePassive, flags);
	SETFLAGBYSAMPLE(Flags, fForcePassive, flags);
	SETFLAGBYSAMPLE(Flags, fSelectModifMode, flags);
	SETFLAGBYSAMPLE(Flags, fHideGeneric, flags);
	SETFLAGBYSAMPLE(Flags, fForceMatrixUsage, flags); // @v11.2.8
	LocID = r_cfg.Location;
	MEMSZERO(SerialData);
	P_List = static_cast<SmartListBox *>(getCtrlView(CTL_EXTGSEL_LIST));
	showCtrl(CTL_EXTGSEL_MTX_IND, false);
	showCtrl(CTL_EXTGSEL_INFO, false);
	//
	Ptb.SetBrush(brushModMode_Expend,   SPaintObj::bsSolid, GetColorRef(SClrOrangered), 0);
	Ptb.SetBrush(brushModMode_Incoming, SPaintObj::bsSolid, GetColorRef(SClrMediumblue), 0);
	Ptb.SetBrush(brushModMode_Recompl,  SPaintObj::bsSolid, GetColorRef(SClrSteelblue), 0);
	//
	if(Flags & fSelectModifMode) {
		SString temp_buf;
		showCtrl(CTLSEL_EXTGSEL_MODIFMODE, true);
		StrAssocArray mod_item_list;
		PPLoadString("expend", temp_buf);
		mod_item_list.AddFast(TIDlgInitData::mmExpend, temp_buf);
		PPLoadString("incoming", temp_buf);
		mod_item_list.AddFast(TIDlgInitData::mmIncoming, temp_buf);
		PPLoadString("recompl", temp_buf);
		mod_item_list.AddFast(TIDlgInitData::mmRecompl, temp_buf);
		PPID   init_mm = (Flags & fExistsOnly) ? TIDlgInitData::mmExpend : TIDlgInitData::mmIncoming;
		SetupStrAssocCombo(this, CTLSEL_EXTGSEL_MODIFMODE, mod_item_list, init_mm, 0, 0, 0);
	}
	else {
		showCtrl(CTLSEL_EXTGSEL_MODIFMODE, false);
		selectCtrl(CTL_EXTGSEL_QTTY);
	}
	{
		int    disable_group_selection = 0;
		if(!initGoodsID) {
			PPAccessRestriction accsr;
			initGoodsID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
			if(initGoodsID && accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp)
				disable_group_selection = 1;
		}
		SetupPPObjCombo(this, CTLSEL_EXTGSEL_GGRP, PPOBJ_GOODSGROUP, initGoodsID, OLW_CANINSERT|OLW_WORDSELECTOR, 0);
		if(disable_group_selection)
			disableCtrl(CTLSEL_EXTGSEL_GGRP, true);
	}
	{
		UseBrand = BIN(GObj.GetConfig().Flags & GCF_USEBRANDINGSELEXTDLG);
		if(UseBrand)
			SetupPPObjCombo(this, CTLSEL_EXTGSEL_BRAND, PPOBJ_BRAND, 0, OLW_LOADDEFONOPEN, 0);
		else {
			showCtrl(CTLSEL_EXTGSEL_BRAND, false);
			showCtrl(CTL_EXTGSEL_BRAND, false);
		}
	}
	if((Flags & fForceMatrixUsage) || ((r_cfg.Flags & CFGFLG_USEGOODSMATRIX) && OpID && CheckOpFlags(OpID, OPKF_RESTRICTBYMTX)))
		Flags |= fMatrix;
	if(initGoodsID)
		updateList(0);
	{
		GObj.P_Tbl->getNumRecs(&G2RecCount);
		/*
		if(G2RecCount < MAX_GOODS_RECS)
			SetupWordSelector(CTL_EXTGSEL_LIST, 0, initGoodsID, MIN_WORDSEL_SYMB, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
		*/
	}
}

ExtGoodsSelDialog::~ExtGoodsSelDialog()
{
	delete P_ObjTkn;
}

void ExtGoodsSelDialog::setLocation(PPID locID)
{
	LocID = locID;
}

void ExtGoodsSelDialog::setSelectionByPrice(double p)
{
	if(!(Flags & fSelByPrice) || p != Price) {
		Flags |= fSelByPrice;
		Price = p;
		setCtrlLong(CTLSEL_EXTGSEL_GGRP,  0);
		disableCtrl(CTLSEL_EXTGSEL_GGRP,  1);
		if(UseBrand) {
			setCtrlLong(CTLSEL_EXTGSEL_BRAND, 0);
			disableCtrl(CTLSEL_EXTGSEL_BRAND, true);
		}
		updateList(0);
		selectCtrl(CTL_EXTGSEL_LIST);
	}
}

void ExtGoodsSelDialog::setSelectionByGroup()
{
	if(Flags & (fSelByPrice|fSelByList|fSelByStrList)) {
		Flags &= ~(fSelByPrice|fSelByList|fSelByStrList);
		Price = 0.0;
		IdList.freeAll();
		StrIdList.Z();
		disableCtrl(CTLSEL_EXTGSEL_GGRP, false);
		if(UseBrand)
			disableCtrl(CTLSEL_EXTGSEL_BRAND, false);
		updateList(0);
		selectCtrl(UseBrand ? CTLSEL_EXTGSEL_BRAND : CTLSEL_EXTGSEL_GGRP);
	}
}

void ExtGoodsSelDialog::setSelectionByGoodsList(const PPIDArray * pList, const char * pInfoText)
{
	if(Flags & fSelByPrice) {
		Flags &= ~fSelByPrice;
		Price = 0.0;
		disableCtrl(CTLSEL_EXTGSEL_GGRP, false);
		if(UseBrand)
			disableCtrl(CTLSEL_EXTGSEL_BRAND, false);
		updateList(0);
		selectCtrl(UseBrand ? CTLSEL_EXTGSEL_BRAND : CTLSEL_EXTGSEL_GGRP);
	}
	if(!isempty(pInfoText)) {
		showCtrl(CTL_EXTGSEL_INFO, true);
		showCtrl(CTL_EXTGSEL_GGRP, false);
		showCtrl(CTLSEL_EXTGSEL_GGRP, false);
		showCtrl(CTL_EXTGSEL_BRAND, false);
		showCtrl(CTLSEL_EXTGSEL_BRAND, false);
		setStaticText(CTL_EXTGSEL_INFO, pInfoText);
	}
	else {
		showCtrl(CTL_EXTGSEL_INFO, false);
		showCtrl(CTL_EXTGSEL_GGRP, true);
		showCtrl(CTLSEL_EXTGSEL_GGRP, true);
		if(UseBrand) {
			showCtrl(CTL_EXTGSEL_BRAND, true);
			showCtrl(CTLSEL_EXTGSEL_BRAND, true);
		}
	}
	Flags |= fSelByList;
	Flags &= ~fSelByStrList;
	StrIdList.Z();
	if(!RVALUEPTR(IdList, pList))
		IdList.freeAll();
	disableCtrl(CTLSEL_EXTGSEL_GGRP, true);
	if(UseBrand)
		disableCtrl(CTLSEL_EXTGSEL_BRAND, true);
	updateList(0);
	selectCtrl(CTL_EXTGSEL_LIST);
}

void ExtGoodsSelDialog::setSelectionByGoodsList(const StrAssocArray * pList, const char * pInfoText)
{
	if(Flags & fSelByPrice) {
		Flags &= ~fSelByPrice;
		Price = 0.0;
		disableCtrl(CTLSEL_EXTGSEL_GGRP, false);
		if(UseBrand)
			disableCtrl(CTLSEL_EXTGSEL_BRAND, false);
		updateList(0);
		selectCtrl(UseBrand ? CTLSEL_EXTGSEL_BRAND : CTLSEL_EXTGSEL_GGRP);
	}
	if(!isempty(pInfoText)) {
		showCtrl(CTL_EXTGSEL_INFO, true);
		showCtrl(CTL_EXTGSEL_GGRP, false);
		showCtrl(CTLSEL_EXTGSEL_GGRP, false);
		showCtrl(CTL_EXTGSEL_BRAND, false);
		showCtrl(CTLSEL_EXTGSEL_BRAND, false);
		setStaticText(CTL_EXTGSEL_INFO, pInfoText);
	}
	else {
		showCtrl(CTL_EXTGSEL_INFO, false);
		showCtrl(CTL_EXTGSEL_GGRP, true);
		showCtrl(CTLSEL_EXTGSEL_GGRP, true);
		if(UseBrand) {
			showCtrl(CTL_EXTGSEL_BRAND, true);
			showCtrl(CTLSEL_EXTGSEL_BRAND, true);
		}
	}
	Flags &= ~fSelByList;
	Flags |= fSelByStrList;
	IdList.freeAll();
	if(!RVALUEPTR(StrIdList, pList))
		StrIdList.Z();
	disableCtrl(CTLSEL_EXTGSEL_GGRP, true);
	if(UseBrand)
		disableCtrl(CTLSEL_EXTGSEL_BRAND, true);
	updateList(0);
	selectCtrl(CTL_EXTGSEL_LIST);
}

void ExtGoodsSelDialog::updateList(int pos)
{
	showCtrl(CTL_EXTGSEL_MTX_IND, false);
	if(P_List) {
		ListBoxDef * p_def = 0;
		const  PPID op_type_id = GetOpType(OpID);
		const  long preserve_pos = P_List->P_Def ? P_List->P_Def->_curItem() : 0;
		PPID   grp   = getCtrlLong(CTLSEL_EXTGSEL_GGRP);
		PPID   brand = getCtrlLong(CTLSEL_EXTGSEL_BRAND);
		int    exists_only = 0;
        long   mod_mode = TIDlgInitData::mmUndef;
        if(Flags & fSelectModifMode)
			getCtrlData(CTLSEL_EXTGSEL_MODIFMODE, &mod_mode);
		if(Flags & fForceExhausted)
			exists_only = 0;
		else if(oneof2(mod_mode, TIDlgInitData::mmExpend, TIDlgInitData::mmRecompl))
			exists_only = 1;
		else if(mod_mode == TIDlgInitData::mmIncoming)
			exists_only = 0;
		else if((Flags & fExistsOnly) || IsExpendOp(OpID) > 0 || op_type_id == PPOPT_GOODSREVAL || (op_type_id == PPOPT_GOODSORDER && CheckOpFlags(OpID, OPKF_ORDEXSTONLY)))
			exists_only = 1;
		const  PPID g = exists_only ? (grp ? -grp : LONG_MIN) : grp;
		if(Flags & (fSelByPrice|fSelByList|fSelByStrList)) {
			StrAssocArray * p_list = 0;
			if(Flags & fSelByPrice)
				p_list = GObj.CreateListByPrice(LocID, Price);
			else if(Flags & fSelByList)
				p_list = GObj.CreateListByList(&IdList);
			else if(Flags & fSelByStrList) {
				p_list = new StrAssocArray;
				*p_list = StrIdList;
			}
			p_def = new StrAssocListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
		}
		else {
			PPID   mtx_loc_id = LocID;
			long   _sel_options = 0;
			SETFLAG(_sel_options, PPObjGoods::selfByName, Flags & fByName);
			SETFLAG(_sel_options, PPObjGoods::selfHidePassive, Flags & fHidePassive);
			SETFLAG(_sel_options, PPObjGoods::selfHideGeneric, Flags & fHideGeneric); // @v10.7.7
			SETFLAG(_sel_options, PPObjGoods::selfForcePassive, Flags & fForcePassive);
			if(Flags & fMatrix) {
				showCtrl(CTL_EXTGSEL_MTX_IND, true);
				SetCtrlBitmap(CTL_EXTGSEL_MTX_IND, BM_MATRIX);
				_sel_options |= PPObjGoods::selfUseMatrix;
				if(IsIntrExpndOp(OpID) && ArID)
					mtx_loc_id = NZOR(PPObjLocation::ObjToWarehouse(ArID), LocID);
			}
			SETFLAG(_sel_options, PPObjGoods::selfExtLongAsBrand, brand);
			p_def = GObj._Selector2(0, g, _sel_options, reinterpret_cast<void *>(brand), LocID, mtx_loc_id);
		}
		if(p_def) {
			if(p_def->GetCapability() & ListBoxDef::cFullInMem)
				SetupWordSelector(CTL_EXTGSEL_LIST, 0, 0, MIN_WORDSEL_SYMB, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
			else
				ResetWordSelector(CTL_EXTGSEL_LIST);
			P_List->setDef(p_def);
			if(pos < 0)
				P_List->focusItem(preserve_pos);
			P_List->Draw_();
			P_List->select();
		}
	}
}

int ExtGoodsSelDialog::selectGoods(PPID grp, PPID goodsID)
{
	int    ok = -1;
	if(P_List && (grp || goodsID)) {
		setCtrlLong(CTLSEL_EXTGSEL_GGRP, grp);
		updateList(0);
		if(goodsID) {
			P_List->TransmitData(+1, &goodsID);
			if(P_List->HasState(SmartListBox::stDataFounded)) {
				selectCtrl(CTL_EXTGSEL_LIST);
				P_List->Draw_();
			}
		}
		ok = 1;
	}
	return ok;
}

PPID ExtGoodsSelDialog::getSelectedItem()
{
	long   id = 0;
	return (P_List && P_List->getCurID(&id)) ? id : 0;
}

void ExtGoodsSelDialog::searchBarcode()
{
	int    r;
	Goods2Tbl::Rec rec;
	SString bcode;
	if((r = GObj.SelectGoodsByBarcode(0, ArID, &rec, 0, &bcode)) > 0) {
		if(OpID && !GObj.CheckMatrix(rec.ID, NZOR(LocID, LConfig.Location), OpID, ArID))
			PPError();
		else
			selectGoods(rec.ParentID, rec.ID);
	}
	else if(r == -2 && PPMessage(mfConf|mfYesNo, PPCFM_ADDNEWGOODS) == cmYes) {
		PPID   id = 0;
		PPID   grp_id = getCtrlLong(CTLSEL_EXTGSEL_GGRP);
		r = GObj.Edit(&id, gpkndGoods, grp_id, 0, bcode);
		if(r == cmOK) {
			grp_id = (GObj.Fetch(id, &rec) > 0) ? rec.ParentID : 0;
			selectGoods(grp_id, id);
		}
		else if(!r)
			PPError();
	}
}

IMPL_HANDLE_EVENT(ExtGoodsSelDialog)
{
	PPID   grp_id;
	PPID   brand = 0;
	if(TVCOMMAND && oneof2(TVCMD, cmOK, cmLBDblClk)) {
		long   c;
		if(!P_List || !P_List->getCurID(&(c = 0)) || !c)
			clearEvent(event);
		else if(TVCMD == cmLBDblClk && event.isCtlEvent(CTL_EXTGSEL_LIST))
			event.message.command = cmOK;
	}
	TDialog::handleEvent(event);
	if(event.isCmd(cmCtlColor)) {
		TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
		if(p_dc && getCtrlHandle(CTL_EXTGSEL_MODIFMODE) == p_dc->H_Ctl) {
			const long modif_mode = getCtrlLong(CTLSEL_EXTGSEL_MODIFMODE);
			if(modif_mode == TIDlgInitData::mmExpend) {
				::SetBkMode(p_dc->H_DC, TRANSPARENT);
				::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
				p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushModMode_Expend));
			}
			else if(modif_mode == TIDlgInitData::mmIncoming) {
				::SetBkMode(p_dc->H_DC, TRANSPARENT);
				::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
				p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushModMode_Incoming));
			}
			else if(modif_mode == TIDlgInitData::mmRecompl) {
				::SetBkMode(p_dc->H_DC, TRANSPARENT);
				::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
				p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushModMode_Recompl));
			}
			else
				return;
		}
		else
			return;
	}
	else if(event.isCbSelected(CTLSEL_EXTGSEL_GGRP) || event.isCbSelected(CTLSEL_EXTGSEL_BRAND) || event.isCbSelected(CTLSEL_EXTGSEL_MODIFMODE))
		updateList(0);
	else if(event.isCmd(cmaInsert)) {
		PPID   new_goods_id = 0;
		grp_id = getCtrlLong(CTLSEL_EXTGSEL_GGRP);
		if(GObj.Edit(&new_goods_id, reinterpret_cast<void *>(grp_id)) == cmOK)
			selectGoods(grp_id, new_goods_id);
	}
	else if(event.isCmd(cmSearchUHTT) || event.isKeyDown(kbCtrlF6)) {
		const  PPID goods_id = getSelectedItem();
		if(goods_id)
			GObj.ViewUhttGoodsRestList(goods_id);
	}
	else if(event.isKeyDown(kbF3)) {
		if(Flags & fSelectModifMode) {
			const long  cur_mode = getCtrlLong(CTLSEL_EXTGSEL_MODIFMODE);
			long  new_mode = 0;
			switch(cur_mode) {
				case TIDlgInitData::mmIncoming: new_mode = TIDlgInitData::mmExpend; break;
				case TIDlgInitData::mmExpend: new_mode = TIDlgInitData::mmIncoming; break;
				case TIDlgInitData::mmRecompl: new_mode = TIDlgInitData::mmExpend; break;
				default: new_mode = TIDlgInitData::mmExpend; break;
			}
			setCtrlLong(CTLSEL_EXTGSEL_MODIFMODE, new_mode);
			updateList(0);
		}
		else
			return;
	}
	else if(event.isKeyDown(kbCtrlF8)) {
		const  PPID goods_id = getSelectedItem();
		Goods2Tbl::Rec goods_rec;
		if(goods_id && GObj.Fetch(goods_id, &goods_rec) > 0) {
			PPIDArray analog_list;
			SString transit_component_buf;
			if(GObj.P_Tbl->SearchGoodsAnalogs(goods_id, analog_list, &transit_component_buf) > 0 && analog_list.getCount()) {
				TIDlgInitData inner_tidi;
				long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags(); // @v10.7.7
				ExtGoodsSelDialog * p_inner_dlg = new ExtGoodsSelDialog(0, 0, egsd_flags);
				if(CheckDialogPtrErr(&p_inner_dlg)) {
					SString info_buf, temp_buf;
					PPLoadString("analogs", temp_buf);
					info_buf.Z().Cat(goods_rec.Name).CR();
					info_buf.Cat(temp_buf);
					if(transit_component_buf.NotEmptyS())
						info_buf.Space().CatChar('(').Cat(transit_component_buf).CatChar(')');
					p_inner_dlg->setSelectionByGoodsList(&analog_list, info_buf);
					p_inner_dlg->ToCascade();
					if(ExecView(p_inner_dlg) == cmOK) {
						p_inner_dlg->getDTS(&inner_tidi);
						InnerSelectionID = inner_tidi.GoodsID;
					}
					else
						InnerSelectionID = 0;
				}
				ZDELETE(p_inner_dlg);
				if(InnerSelectionID && IsInState(sfModal)) {
					EndModalCmd = cmOK;
					clearEvent(event);
				}
			}
		}
	}
	else if(event.isCmd(cmSelSerial)) { // end dialog
		PPID   goods_id = getSelectedItem();
		if(goods_id) {
			PPObjTSession tses_obj;
			PPObjTSession::SerialByGoodsListItem si;
			if(tses_obj.SelectSerialByGoods(goods_id, LocID, &si) > 0) {
				SerialData = si;
				if(IsInState(sfModal)) {
					endModal(cmOK);
					return; // После endModal не следует обращаться к this
				}
				clearEvent(event);
			}
			else {
				PPObjBill::SelectLotParam slp(goods_id, LocID, 0, PPObjBill::SelectLotParam::fNotEmptySerial);
				slp.Period.Set(ZERODATE, getcurdate_());
				if(BillObj->SelectLot2(slp) > 0) {
					MEMSZERO(SerialData);
					SerialData.LotID = slp.RetLotID;
					STRNSCPY(SerialData.Serial, slp.RetLotSerial);
					if(IsInState(sfModal)) {
						endModal(cmOK);
						return; // После endModal не следует обращаться к this
					}
					clearEvent(event);
				}
			}
		}
	}
	else if(event.wasFocusChanged(CTL_EXTGSEL_GGRP) || event.wasFocusChanged(CTL_EXTGSEL_BRAND)) {
		if(P_List && !P_List->P_Def) {
			grp_id = getCtrlLong(CTLSEL_EXTGSEL_GGRP);
			brand  = getCtrlLong(CTLSEL_EXTGSEL_BRAND);
			if(grp_id || brand)
				updateList(0);
		}
		else
			return;
	}
	else if(event.isKeyDown(kbF2)) {
		searchBarcode();
	}
	else if(event.isKeyDown(kbAltF2)) {
		PPID   n = 0;
		PPID   s = getSelectedItem();
		if(s && GObj.AddBySample(&n, s) == cmOK)
			selectGoods(getCtrlLong(CTLSEL_EXTGSEL_GGRP), n);
	}
	else if(event.isKeyDown(kbF9)) {
		const  PPID goods_id = getSelectedItem();
		if(goods_id)
			PPGoodsCalculator(goods_id, 0, 0, 0, 0);
	}
	else if(event.isKeyDown(KB_CTRLENTER))
		if(IsCurrentView(P_List)) {
			PPID goods_id = getSelectedItem();
			if(goods_id && GObj.Edit(&goods_id, 0L) == cmOK)
				updateList(-1);
		}
		else
			return;
	else
		return;
	clearEvent(event);
}

int ExtGoodsSelDialog::setDTS(TIDlgInitData * pTidi)
{
	int    ok = -1;
	MEMSZERO(SerialData);
	if(pTidi) {
		if(Flags & fSelectModifMode && pTidi->ModifMode)
			setCtrlLong(CTLSEL_EXTGSEL_MODIFMODE, pTidi->ModifMode);
		ArID = pTidi->ArID;
		selectGoods(pTidi->GoodsGrpID, pTidi->GoodsID);
		setCtrlData(CTL_EXTGSEL_QTTY,  &pTidi->Quantity);
		setCtrlUInt16(CTL_EXTGSEL_PACKF, 0);
		ok = 1;
	}
	return ok;
}

int ExtGoodsSelDialog::getDTS(TIDlgInitData * pTidi)
{
	int   ok = 1;
	if(pTidi) {
		pTidi->GoodsID  = 0;
		pTidi->Quantity = 0.0;
		pTidi->Flags    = 0;
		if(Flags & fSelectModifMode)
			getCtrlData(CTLSEL_EXTGSEL_MODIFMODE, &pTidi->ModifMode);
		if(InnerSelectionID) {
			pTidi->GoodsGrpID = 0;
			pTidi->GoodsID = InnerSelectionID;
		}
		else {
			long   goods_id = 0;
			if(P_List && P_List->getCurID(&goods_id) && goods_id) {
				pTidi->GoodsID = goods_id;
				getCtrlData(CTLSEL_EXTGSEL_GGRP, &pTidi->GoodsGrpID);
				if(SerialData.Serial[0]) {
					STRNSCPY(pTidi->Serial, SerialData.Serial);
					pTidi->Quantity = SerialData.Qtty;
					pTidi->LotID    = SerialData.LotID;
				}
				else {
					getCtrlData(CTL_EXTGSEL_QTTY,  &pTidi->Quantity);
					if(getCtrlUInt16(CTL_EXTGSEL_PACKF))
						pTidi->Flags |= TIDIF_PACKS;
				}
			}
			else
				ok = -1;
		}
	}
	return ok;
}
