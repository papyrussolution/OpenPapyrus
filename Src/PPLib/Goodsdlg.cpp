// GOODSDLG.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Диалог редактирования товара
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>

struct UhttSearchGoodsParam {
	UhttSearchGoodsParam() : Criterion(critBarcode)
	{
	}
	enum {
		critBarcode = 1,
		critName
	};
	int    Criterion;
	SString Name;
	SString Barcode;
};

int PPObjGoods::SearchUhttInteractive(const SString & rName, const SString & rBarcode, UhttGoodsPacket * pResultItem)
{
	class UhttSearchGoodsDialog : public TDialog {
		DECL_DIALOG_DATA(UhttSearchGoodsParam);
	public:
		UhttSearchGoodsDialog() : TDialog(DLG_UHTTSGOODS)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_UHTTSGOODS_SEL, 0, UhttSearchGoodsParam::critBarcode);
			AddClusterAssoc(CTL_UHTTSGOODS_SEL, 1, UhttSearchGoodsParam::critName);
			SetClusterData(CTL_UHTTSGOODS_SEL, Data.Criterion);
			if(Data.Criterion == UhttSearchGoodsParam::critName) {
				setCtrlString(CTL_UHTTSGOODS_TEXT, Data.Name);
			}
			else {
				setCtrlString(CTL_UHTTSGOODS_TEXT, Data.Barcode);
			}
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			Data.Criterion = GetClusterData(CTL_UHTTSGOODS_SEL);
			if(Data.Criterion == UhttSearchGoodsParam::critName) {
				getCtrlString(CTL_UHTTSGOODS_TEXT, Data.Name);
			}
			else {
				getCtrlString(CTL_UHTTSGOODS_TEXT, Data.Barcode);
			}
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_UHTTSGOODS_SEL)) {
				//
				// Забрать предудущее значение строки
				//
				if(Data.Criterion == UhttSearchGoodsParam::critName) {
					getCtrlString(CTL_UHTTSGOODS_TEXT, Data.Name);
				}
				else {
					getCtrlString(CTL_UHTTSGOODS_TEXT, Data.Barcode);
				}
				Data.Criterion = GetClusterData(CTL_UHTTSGOODS_SEL);
				//
				// Установить текст в соответствии с критерием
				//
				if(Data.Criterion == UhttSearchGoodsParam::critName) {
					setCtrlString(CTL_UHTTSGOODS_TEXT, Data.Name);
				}
				else {
					setCtrlString(CTL_UHTTSGOODS_TEXT, Data.Barcode);
				}
				clearEvent(event);
			}
		}
	};
	class SelectDialog : public PPListDialog {
	public:
		SelectDialog(TSCollection <UhttGoodsPacket> & rList) : PPListDialog(DLG_SELBYBCODE, CTL_SELBYBCODE_LIST), R_List(rList)
		{
			updateList(-1);
		}
		int getDTS(uint * pSelPos)
		{
			long   sel = 0;
			int    ok = getCurItem(0, &sel);
			ASSIGN_PTR(pSelPos, static_cast<uint>(sel));
			return ok;
		}
	private:
		virtual int setupList()
		{
			int     ok = -1;
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < R_List.getCount(); i++) {
				UhttGoodsPacket * p_item = R_List.at(i);
				if(p_item) {
					ss.Z();
					ss.add(temp_buf = p_item->SingleBarcode);
					ss.add(temp_buf = p_item->Name);
					THROW(addStringToList(i+1, ss.getBuf()));
				}
			}
			CATCHZOK
			return ok;
		}
		TSCollection <UhttGoodsPacket> & R_List;
	};
	int    ok = -1;
	UhttSearchGoodsParam param;
	SelectDialog * sel_dlg = 0;
	UhttSearchGoodsDialog * dlg = new UhttSearchGoodsDialog;
	THROW(CheckDialogPtr(&dlg));
	(param.Name = rName).Strip();
	(param.Barcode = rBarcode).Strip();
	if(param.Name.NotEmpty()) {
		param.Criterion = UhttSearchGoodsParam::critName;
	}
	else {
		param.Criterion = UhttSearchGoodsParam::critBarcode;
	}
	dlg->setDTS(&param);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&param))
			ok = 1;
	}
	if(ok > 0) {
		if(param.Criterion == UhttSearchGoodsParam::critName && param.Name.IsEmpty())
			ok = -1;
		else if(param.Criterion == UhttSearchGoodsParam::critBarcode && param.Barcode.IsEmpty())
			ok = -1;
		else {
			PPUhttClient uc;
			TSCollection <UhttGoodsPacket> uhtt_goods_list;
			THROW(uc.Auth());
			if(param.Criterion == UhttSearchGoodsParam::critName) {
				THROW(uc.GetGoodsByName(param.Name, uhtt_goods_list));
			}
			else {
				THROW(uc.GetGoodsByCode(param.Barcode, uhtt_goods_list));
			}
			{
				sel_dlg = new SelectDialog(uhtt_goods_list);
				THROW(CheckDialogPtr(&sel_dlg));
				if(ExecView(sel_dlg) == cmOK) {
					if(pResultItem) {
						uint sel_pos = 0;
						sel_dlg->getDTS(&sel_pos);
						if(sel_pos <= uhtt_goods_list.getCount()) {
							*pResultItem = *uhtt_goods_list.at(sel_pos-1);
						}
					}
					ok = 1;
				}
			}
		}
	}
	CATCHZOKPPERR
	delete sel_dlg;
	delete dlg;
	return ok;
}

static int MergeUhttRestList(TSCollection <UhttGoodsRestListItem> & rDest, const TSCollection <UhttGoodsRestListItem> & rSrc)
{
	int    ok = 1;
	const  uint sc = rSrc.getCount();
    for(uint i = 0; i < sc; i++) {
    	const UhttGoodsRestListItem * p_src_item = rSrc.at(i);
		if(p_src_item) {
			const LDATETIME src_price_dtm = p_src_item->PriceDtm;
			int    found = 0;
			for(uint j = 0; !found && j < rDest.getCount(); j++) {
                UhttGoodsRestListItem * p_item = rDest.at(j);
				if(p_item && p_item->LocID == p_src_item->LocID) {
					if(cmp(p_src_item->RestDtm, p_item->RestDtm) > 0)
						p_item->RestDtm = p_src_item->RestDtm;
					p_item->Rest += p_src_item->Rest;
					const LDATETIME dest_price_dtm = p_item->PriceDtm;
					int r = cmp(src_price_dtm, dest_price_dtm);
					if(r > 0) {
                        p_item->Price = p_src_item->Price;
						p_item->PriceDtm = p_src_item->PriceDtm;
					}
					found = 1;
				}
			}
			if(!found) {
				UhttGoodsRestListItem * p_new_item = new UhttGoodsRestListItem;
				THROW_MEM(p_new_item);
				*p_new_item = *p_src_item;
                THROW_SL(rDest.insert(p_new_item));
			}
		}
    }
    CATCHZOK
    return ok;
}

int PPObjGoods::ViewUhttGoodsRestList(PPID goodsID)
{
	class UhttGoodsRestListDialog : public PPListDialog {
	public:
		UhttGoodsRestListDialog(TSCollection <UhttGoodsRestListItem> & rList) : PPListDialog(DLG_UHTTGRLIST, CTL_UHTTGRLIST_LIST), R_List(rList)
		{
			updateList(-1);
		}
	private:
		virtual int setupList()
		{
			int     ok = -1;
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < R_List.getCount(); i++) {
				const UhttGoodsRestListItem * p_item = R_List.at(i);
				if(p_item) {
					//@lbt_uhttgrlist        "20,L,Организация;20,L,Адрес;10,R,Остаток;8,L,Дата остатка;8,R,Цена;8,L,Дата цены"
					ss.Z();
					ss.add(temp_buf = p_item->Name);
					ss.add(temp_buf = p_item->LocAddr);
					ss.add(temp_buf.Z().Cat(p_item->Rest, MKSFMTD(0, 3, NMBF_NOZERO)));
					ss.add(temp_buf.Z().Cat((LDATE)p_item->RestDtm, DATF_DMY));
					ss.add(temp_buf.Z().Cat(p_item->Price, MKSFMTD(0, 2, NMBF_NOZERO)));
					ss.add(temp_buf.Z().Cat((LDATE)p_item->PriceDtm, DATF_DMY));
					THROW(addStringToList(i+1, ss.getBuf()));
				}
			}
			CATCHZOK
			return ok;
		}
		TSCollection <UhttGoodsRestListItem> & R_List;
	};
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(Fetch(goodsID, &goods_rec) > 0) {
		TSCollection <UhttGoodsRestListItem> uhtt_list;
		PPUhttClient uc;
		THROW(uc.Auth());
		{
			TSCollection <UhttGoodsPacket> uhtt_goods_list;
			THROW(uc.GetUhttGoodsList(goodsID, 0, uhtt_goods_list));
			for(uint i = 0; i < uhtt_goods_list.getCount(); i++) {
				const UhttGoodsPacket * p_uhtt_goods = uhtt_goods_list.at(i);
				if(p_uhtt_goods) {
					TSCollection <UhttGoodsRestListItem> temp_list;
					THROW(uc.GetGoodsRestList(p_uhtt_goods->ID, temp_list));
					THROW(MergeUhttRestList(uhtt_list, temp_list));
				}
			}
		}
		{
			SString goods_info_buf;
			UhttGoodsRestListDialog * dlg = new UhttGoodsRestListDialog(uhtt_list);
			THROW(CheckDialogPtr(&dlg));
			goods_info_buf = goods_rec.Name;
			dlg->setStaticText(CTL_UHTTGRLIST_ST_GOODS, goods_info_buf);
			ExecViewAndDestroy(dlg);
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPObjGoods::ViewGoodsRestByLocList(PPID goodsID)
{
	struct GoodsRestByLocEntry {
		PPID   LocID;
		double Rest;
		double Cost;
		double Price;
	};
	class GoodsRestByLocListDialog : public PPListDialog {
	public:
		GoodsRestByLocListDialog(const TSVector <GoodsRestByLocEntry> & rList) : PPListDialog(DLG_GRESTBYLOCLIST, CTL_GRESTBYLOCLIST_LIST), R_List(rList)
		{
			updateList(-1);
		}
	private:
		virtual int setupList()
		{
			int     ok = -1;
			SString temp_buf;
			LocationTbl::Rec loc_rec;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < R_List.getCount(); i++) {
				const GoodsRestByLocEntry & r_item = R_List.at(i);
				ss.Z();
				temp_buf.Z();
				if(LocObj.Fetch(r_item.LocID, &loc_rec) > 0) {
					temp_buf = loc_rec.Name;
				}
				ss.add(temp_buf);
				ss.add(temp_buf.Z().Cat(r_item.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
				ss.add(temp_buf.Z().Cat(r_item.Cost, MKSFMTD(0, 2, NMBF_NOZERO)));
				ss.add(temp_buf.Z().Cat(r_item.Price, MKSFMTD(0, 2, NMBF_NOZERO)));
				THROW(addStringToList(i+1, ss.getBuf()));
			}
			CATCHZOK
			return ok;
		}
		const TSVector <GoodsRestByLocEntry> & R_List;
		PPObjLocation LocObj;
	};
	int    ok = -1;
	GoodsRestByLocListDialog * dlg = 0;
	Goods2Tbl::Rec goods_rec;
	if(goodsID && Search(goodsID, &goods_rec) > 0) {
		TSVector <GoodsRestByLocEntry> list;
		PPObjBill * p_bobj = BillObj;
		GoodsRestParam gp;
		gp.GoodsID = goodsID;
		gp.DiffParam |= GoodsRestParam::_diffLoc;
		p_bobj->trfr->GetRest(gp);
		for(uint i = 0; i < gp.getCount(); i++) {
			const GoodsRestVal & r_v = gp.at(i);
			GoodsRestByLocEntry new_entry;
			MEMSZERO(new_entry);
			new_entry.LocID = r_v.LocID;
			new_entry.Rest = r_v.Rest;
			new_entry.Cost = r_v.Cost;
			new_entry.Price = r_v.Price;
			list.insert(&new_entry);
			
		}
		if(list.getCount()) {
			dlg = new GoodsRestByLocListDialog(list);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->setStaticText(CTL_GRESTBYLOCLIST_GOODS, goods_rec.Name);
				ExecView(dlg);
				ok = 1;
			}
		}
	}
	delete dlg;
	return ok;
}

// utility
int PPObjGoods::SelectBarcode(int kind, PPID parentID, SString & rBuf)
{
	int    ok = -1;
	PPID   id = 0;
	rBuf.Z();
	if(kind == PPGDSK_GOODS) {
		PPBarcodeStruc rec;
		PPObjBarCodeStruc bcs_obj;
		PPID   id = bcs_obj.GetSingle();
		if(id || PPSelectObject(PPOBJ_BCODESTRUC, &id, 0, 0) > 0) {
			if(bcs_obj.Search(id, &rec) > 0) {
				if(GetBarcodeByTemplate(parentID, rec.Templ, 0, rBuf))
					ok = 1;
			}
			else
				ok = PPErrorZ();
		}
	}
	else if(kind == PPGDSK_GROUP) {
		GenGroupCode(2, rBuf);
		ok = 1;
	}
	return ok;
}

GoodsFiltCtrlGroup::Rec::Rec(PPID grpID, PPID goodsID, PPID locID, long flags, void * extraPtr) :
	GoodsID(goodsID), GoodsGrpID(grpID), LocID(locID), Flags(flags), ExtraPtr(extraPtr)
{
}

GoodsFiltCtrlGroup::GoodsFiltCtrlGroup(uint ctlselGoods, uint ctlselGGrp, uint cm) :
	CtlselGoods(ctlselGoods), CtlselGoodsGrp(ctlselGGrp), Cm(cm), DisableGroupSelection(0)
{
}

int GoodsFiltCtrlGroup::IsGroupSelectionDisabled() const
{
	return DisableGroupSelection;
}

/*virtual*/int GoodsFiltCtrlGroup::setData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	if(pDlg && pData) {
		Data = *static_cast<const Rec *>(pData);
		const  long f = GF_DYNAMICTEMPALTGRP;
		if(GObj.CheckFlag(Data.GoodsGrpID, f)) {
			THROW(Filt.ReadFromProp(PPOBJ_GOODSGROUP, Data.GoodsGrpID, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_));
		}
		if(!(Data.Flags & GoodsCtrlGroup::ignoreRtOnlyGroup)) {
			PPAccessRestriction accsr;
			if(ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID) {
				Goods2Tbl::Rec gg_rec;
				if(GObj.Fetch(accsr.OnlyGoodsGrpID, &gg_rec) > 0 && gg_rec.Kind == PPGDSK_GROUP) {
					if(!Data.GoodsGrpID || Data.GoodsGrpID == accsr.OnlyGoodsGrpID) {
						Data.GoodsGrpID = accsr.OnlyGoodsGrpID;
						if(accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp)
							DisableGroupSelection = 1;
					}
				}
			}
		}
		if(CtlselGoodsGrp && CtlselGoods) {
			GoodsCtrlGroup::Rec rec(Data.GoodsGrpID, Data.GoodsID, Data.LocID, Data.Flags);
			GoodsCtrlGroup * p_grp = static_cast<GoodsCtrlGroup *>(pDlg->getGroup(CtlselGoodsGrp));
			if(!p_grp) {
				p_grp = new GoodsCtrlGroup(CtlselGoodsGrp, CtlselGoods);
				pDlg->addGroup(CtlselGoodsGrp, p_grp);
			}
			CALLPTRMEMB(p_grp, setData(pDlg, &rec));
		}
		else {
			PPID   g  = 0;
			long   fl = OLW_LOADDEFONOPEN;
			if(Data.Flags & GoodsCtrlGroup::enableInsertGoods)
				fl |= OLW_CANINSERT;
			SetupPPObjCombo(pDlg, CtlselGoodsGrp, PPOBJ_GOODSGROUP, Data.GoodsGrpID,
				((Data.Flags & GoodsCtrlGroup::enableSelUpLevel) ? (fl|OLW_CANSELUPLEVEL) : fl), Data.ExtraPtr);
			if(Data.Flags & GoodsCtrlGroup::existsGoodsOnly)
				g = (Data.GoodsGrpID) ? -labs(Data.GoodsGrpID) : LONG_MIN;
			else
				g = labs(Data.GoodsGrpID);
			fl = OLW_LOADDEFONOPEN;
			if(Data.Flags & GoodsCtrlGroup::enableInsertGoods)
				fl |= OLW_CANINSERT;
			SetupPPObjCombo(pDlg, CtlselGoods, PPOBJ_GOODS, Data.GoodsID, fl, reinterpret_cast<void *>(g));
		}
		if(DisableGroupSelection) {
			pDlg->disableCtrl(CtlselGoodsGrp, true);
			if(Cm)
				pDlg->enableCommand(Cm, 0);
		}
		SetupCtrls(pDlg);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*virtual*/int GoodsFiltCtrlGroup::getData(TDialog * pDlg, void * pData)
{
	int    ok = -1;
	THROW(PPObjGoodsGroup::RemoveDynamicAlt(Data.GoodsGrpID, 0, 1));
	if(pDlg && pData) {
		if(!Filt.IsEmpty()) {
			THROW(PPObjGoodsGroup::AddDynamicAltGroupByFilt(&Filt, &Data.GoodsGrpID, 0, 1));
		}
		else if(CtlselGoodsGrp && CtlselGoods) {
			GoodsCtrlGroup::Rec rec;
			THROW(pDlg->getGroupData(CtlselGoodsGrp, &rec));
			Data.GoodsGrpID = rec.GoodsGrpID;
			Data.GoodsID    = rec.GoodsID;
			Data.LocID      = rec.LocID;
		}
		else if(CtlselGoodsGrp)
			pDlg->getCtrlData(CtlselGoodsGrp, &Data.GoodsGrpID);
		if(CtlselGoods)
			pDlg->getCtrlData(CtlselGoods, &Data.GoodsID);
		*static_cast<Rec *>(pData) = Data;
		ok = 1;
	}
	CATCHZOK
	return ok;
}

void GoodsFiltCtrlGroup::SetupCtrls(TDialog * pDlg)
{
	const bool by_filt = !Filt.IsEmpty();
	pDlg->disableCtrl(CtlselGoods, by_filt);
	pDlg->disableCtrl(CtlselGoodsGrp, (DisableGroupSelection || by_filt));
	if(by_filt) {
		//
		// Группу нельзя блокировать вызовом pDlg->disableCtrl(CtlselGoodsGrp, by_filt)
		// по-скольку выбор группы мог быть заблокирован выше на основании прав доступа,
		// и в таком случае приведенный вариант разблокирует комбо-бокс.
		//
		// pDlg->disableCtrl(CtlselGoodsGrp, true);
		//
		SString buf;
		SetComboBoxLinkText(pDlg, CtlselGoodsGrp, PPGetWord(PPWORD_TEMPALTGRP, 0, buf));
	}
}

int GoodsFiltCtrlGroup::EditFilt(TDialog * pDlg)
{
	const bool prev_by_filt = !Filt.IsEmpty();
	if(GoodsFilterDialog(&Filt) > 0) {
		const bool by_filt = !Filt.IsEmpty();
		if(by_filt || prev_by_filt) {
			pDlg->setCtrlLong(CtlselGoods, 0);
			pDlg->setCtrlLong(CtlselGoodsGrp, 0);
			SetComboBoxLinkText(pDlg, CtlselGoodsGrp, "");
		}
		SetupCtrls(pDlg);
	}
	return 1;
}

void GoodsFiltCtrlGroup::handleEvent(TDialog * pDlg, TEvent & event)
{
	if(Filt.IsEmpty() && CtlselGoodsGrp && CtlselGoods) {
		CtrlGroup * p_grp = static_cast<CtrlGroup *>(pDlg->getGroup(CtlselGoodsGrp));
		CALLPTRMEMB(p_grp, handleEvent(pDlg, event));
	}
	if(event.isCmd(Cm)) {
		EditFilt(pDlg);
		pDlg->clearEvent(event);
	}
	else
		return;
}

static int _EditBarcodeItem(BarcodeTbl::Rec * pRec, PPID goodsGrpID)
{
	class BarcodeItemDialog : public TDialog {
		DECL_DIALOG_DATA(BarcodeTbl::Rec);
	public:
		explicit BarcodeItemDialog(PPID goodsGrpID) : TDialog(DLG_BARCODE), GoodsGrpID(goodsGrpID)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			SString goods_name;
			GetGoodsName(Data.GoodsID, goods_name);
			setCtrlString(CTL_BARCODE_GOODS, goods_name);
			setCtrlData(CTL_BARCODE_CODE,  Data.Code);
			setCtrlData(CTL_BARCODE_UPP,   &Data.Qtty);
			{
				long   bcf = 0;
				SETFLAG(bcf, 0x01, IsInnerBarcodeType(Data.BarcodeType, BARCODE_TYPE_PREFERRED));
				SETFLAG(bcf, 0x02, IsInnerBarcodeType(Data.BarcodeType, BARCODE_TYPE_MARKED));
				AddClusterAssoc(CTL_BARCODE_FLAGS, 0, 0x01);
				AddClusterAssoc(CTL_BARCODE_FLAGS, 1, 0x02);
				SetClusterData(CTL_BARCODE_FLAGS, bcf);
			}
			selectCtrl(CTL_BARCODE_CODE);
			if(!GObj.CheckRights(GOODSRT_PRIORCODE)) {
				DisableClusterItem(CTL_BARCODE_FLAGS, 0, 1);
				if(Data.BarcodeType == BARCODE_TYPE_PREFERRED)
					disableCtrls(1, CTL_BARCODE_CODE, CTL_BARCODE_UPP, 0);
			}
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			int    sel = 0;
			SString barcode;
			SString mark_buf;
			getCtrlString(sel = CTL_BARCODE_CODE, barcode); // Data.Code
			THROW_PP(barcode.NotEmptyS(), PPERR_BARCODENEEDED);
			if(PrcssrAlcReport::IsEgaisMark(barcode, &mark_buf)) {
				PrcssrAlcReport::EgaisMarkBlock emb;
				if(PrcssrAlcReport::ParseEgaisMark(mark_buf, emb)) {
					barcode = emb.EgaisCode;
					setCtrlString(CTL_BARCODE_CODE, barcode);
				}
			}
			STRNSCPY(Data.Code, barcode);
			getCtrlData(sel = CTL_BARCODE_UPP, &Data.Qtty);
			THROW_PP(Data.Qtty > 0.0, PPERR_INVUNITPPACK);
			{
				const long bcf = GetClusterData(CTL_BARCODE_FLAGS);
				if(bcf & 0x01)
					SetInnerBarcodeType(&Data.BarcodeType, BARCODE_TYPE_PREFERRED);
				else
					ResetInnerBarcodeType(&Data.BarcodeType, BARCODE_TYPE_PREFERRED);
				if(bcf & 0x02)
					SetInnerBarcodeType(&Data.BarcodeType, BARCODE_TYPE_MARKED);
				else
					ResetInnerBarcodeType(&Data.BarcodeType, BARCODE_TYPE_MARKED);
			}
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			SString code;
			if(event.isCmd(cmOK)) {
				BarcodeTbl::Rec  bc_rec;
				SString mark_buf;
				getCtrlString(CTL_BARCODE_CODE, code);
				if(PrcssrAlcReport::IsEgaisMark(code, &mark_buf)) {
					PrcssrAlcReport::EgaisMarkBlock emb;
					if(PrcssrAlcReport::ParseEgaisMark(mark_buf, emb)) {
						code = emb.EgaisCode;
						setCtrlString(CTL_BARCODE_CODE, code);
					}
				}
				if(GObj.SearchBy2dBarcode(code, &bc_rec, 0) > 0)
					setCtrlData(CTL_BARCODE_CODE, bc_rec.Code);
			}
			TDialog::handleEvent(event);
			if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_BARCODE_CODE) && !P_Current->IsInState(sfDisabled)) {
				if(GObj.SelectBarcode(PPGDSK_GOODS, GoodsGrpID, code) > 0)
					setCtrlString(CTL_BARCODE_CODE, code);
			}
			else if(event.isKeyDown(kbF8)) {
				getCtrlString(CTL_BARCODE_CODE, code);
				if(oneof2(code.Len(), 13, 8)) {
					code.TrimRight();
					PPBarcode::BarcodeImageParam bip;
					bip.Code = code;
					bip.Std = BARCSTD_EAN13;
					bip.OutputFormat = SFileFormat::Png;
					if(!PPBarcode::CreateImage(bip))
						PPError();
				}
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_BARCODE_CODE)) {
				getCtrlString(CTL_BARCODE_CODE, code);
				const uint len = static_cast<uint>(code.Len());
				setStaticText(CTL_BARCODE_LEN, code.Z().Cat(len));
			}
			else
				return;
			clearEvent(event);
		}
		PPID   GoodsGrpID;
		PPObjGoods GObj;
	};
	DIALOG_PROC_BODY_P1(BarcodeItemDialog, goodsGrpID, pRec);
}

class BarcodeListDialog : public PPListDialog {
	DECL_DIALOG_DATA(BarcodeArray);
public:
	BarcodeListDialog(PPID goodsID, PPID goodsGroupID) : PPListDialog(DLG_BARCODELIST, CTL_BARCODELIST_LIST), GoodsID(goodsID), GoodsGrpID(goodsGroupID)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(GoodsID) {
			SString goods_name;
			setStaticText(CTL_BARCODELIST_GOODS, GetGoodsName(GoodsID, goods_name));
		}
		Data.copy(*pData);
		updateList(0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		return pData->copy(Data) ? 1 : PPSetErrorSLib();
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmCopyToClipboard)) {
			long  cur_pos = 0;
			long  cur_id = 0;
			SString text_buf;
			if(getCurItem(&cur_pos, &cur_id)) {
				if(cur_pos >= 0 && cur_pos < Data.getCountI()) {
					const BarcodeTbl::Rec & r_rec = Data.at(static_cast<uint>(cur_pos));
					text_buf.Cat(r_rec.Code).Tab().Cat(r_rec.Qtty).Tab();
					if(oneof2(r_rec.BarcodeType, BARCODE_TYPE_PREFERRED, BARCODE_TYPE_PREFMARK))
						text_buf.Cat("preferred");
					SClipboard::Copy_Text(text_buf, text_buf.Len());
				}
			}			
		}
		else if(event.isCmd(cmCopyToClipboardAll)) {
			if(Data.getCount()) {
				SString text_buf;
				for(uint i = 0; i < Data.getCount(); i++) {
					const BarcodeTbl::Rec & r_rec = Data.at(static_cast<uint>(i));
					text_buf.Cat(r_rec.Code).Tab().Cat(r_rec.Qtty).Tab();
					if(oneof2(r_rec.BarcodeType, BARCODE_TYPE_PREFERRED, BARCODE_TYPE_PREFMARK))
						text_buf.Cat("preferred");
					text_buf.CR();
				}
				SClipboard::Copy_Text(text_buf, text_buf.Len());
			}
		}
		else
			return;
		clearEvent(event);
	}
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	virtual int  moveItem(long pos, long id, int up);

	const  PPID GoodsID;
	const  PPID GoodsGrpID;
};

int BarcodeListDialog::moveItem(long pos, long id, int up)
{
	int    ok = 1;
	if(up && pos > 0)
		Data.swap(pos, pos-1);
	else if(!up && pos < (Data.getCountI()-1))
		Data.swap(pos, pos+1);
	else
		ok = -1;
	return ok;
}

int BarcodeListDialog::addItem(long * pPos, long *)
{
	int    ok = -1;
	BarcodeTbl::Rec rec;
	rec.GoodsID = GoodsID;
	rec.Qtty = 1.0;
	if(_EditBarcodeItem(&rec, GoodsGrpID) > 0) {
		if(Data.insert(&rec)) {
			if(rec.BarcodeType == BARCODE_TYPE_PREFERRED) {
				for(uint i = 0; i < Data.getCount()-1; i++)
					if(Data.at(i).BarcodeType == BARCODE_TYPE_PREFERRED)
						ResetInnerBarcodeType(&Data.at(i).BarcodeType, BARCODE_TYPE_PREFERRED);
			}
			ASSIGN_PTR(pPos, Data.getCount()-1);
			ok = 1;
		}
		else
			ok = PPSetErrorSLib();
	}
	return ok;
}

int BarcodeListDialog::editItem(long pos, long)
{
	if(pos >= 0 && pos < Data.getCountI()) {
		BarcodeTbl::Rec rec = Data.at(static_cast<uint>(pos));
		if(_EditBarcodeItem(&rec, GoodsGrpID) > 0) {
			Data.at(static_cast<uint>(pos)) = rec;
			if(IsInnerBarcodeType(rec.BarcodeType, BARCODE_TYPE_PREFERRED)) {
				for(uint i = 0; i < Data.getCount(); i++)
					if(i != (uint)pos && IsInnerBarcodeType(Data.at(i).BarcodeType, BARCODE_TYPE_PREFERRED))
						ResetInnerBarcodeType(&Data.at(i).BarcodeType, BARCODE_TYPE_PREFERRED);
			}
			return 1;
		}
	}
	return -1;
}

int BarcodeListDialog::delItem(long pos, long)
{
	return Data.atFree(static_cast<uint>(pos)) ? 1 : -1;
}

int BarcodeListDialog::setupList()
{
	SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_BARCODELIST_LIST));
	if(SmartListBox::IsValidS(p_list)) {
		BarcodeTbl::Rec * p_rec;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; Data.enumItems(&i, (void **)&p_rec);) {
			char   sub[128];
			ss.Z();
			ss.add(p_rec->Code);
			realfmt(p_rec->Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ), sub);
			ss.add(sub);
			if(!addStringToList(i, ss.getBuf()))
				return 0;
			if(IsInnerBarcodeType(p_rec->BarcodeType, BARCODE_TYPE_PREFERRED))
				p_list->P_Def->SetItemColor(i, SClrWhite, SClrGreen);
			else
				p_list->P_Def->ResetItemColor(i);
		}
	}
	return 1;
}
//
//
//
class ArGoodsCodeDialog : public TDialog {
	DECL_DIALOG_DATA(ArGoodsCodeTbl::Rec);
public:
	ArGoodsCodeDialog(int ownCode) : TDialog(DLG_ARGOODSCODE), AcsID(0), OwnCode(ownCode)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString goods_name;
		GetGoodsName(Data.GoodsID, goods_name);
		setCtrlString(CTL_ARGOODSCODE_GOODS, goods_name);
		ArticleTbl::Rec ar_rec;
		if(Data.ArID && ArObj.Fetch(Data.ArID, &ar_rec) > 0) {
			AcsID = ar_rec.AccSheetID;
			disableCtrl(CTLSEL_ARGOODSCODE_ACS, true);
		}
		else
			AcsID = GetSupplAccSheet();
		SetupPPObjCombo(this, CTLSEL_ARGOODSCODE_ACS, PPOBJ_ACCSHEET, AcsID, 0, 0);
		SetupArCombo(this, CTLSEL_ARGOODSCODE_AR, Data.ArID, OLW_CANINSERT|OLW_LOADDEFONOPEN, AcsID, sacfDisableIfZeroSheet|sacfNonGeneric);
		setCtrlData(CTL_ARGOODSCODE_CODE, Data.Code);
		setCtrlReal(CTL_ARGOODSCODE_UPP,  fdiv1000i(Data.Pack));
		uint   sel = 0;
		if(OwnCode)
			sel = CTL_ARGOODSCODE_CODE;
		else if(AcsID == 0)
			sel = CTL_ARGOODSCODE_ACS;
		else if(Data.ArID == 0)
			sel = CTL_ARGOODSCODE_AR;
		else
			sel = CTL_ARGOODSCODE_CODE;
		selectCtrl(sel);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		int    sel = 0;
		getCtrlData(sel = CTLSEL_ARGOODSCODE_AR, &Data.ArID);
		getCtrlData(sel = CTL_ARGOODSCODE_CODE, Data.Code);
		THROW_PP(strip(Data.Code)[0], PPERR_BARCODENEEDED);
		double upp = getCtrlReal(CTL_ARGOODSCODE_UPP);
		THROW_PP(upp > 0.0, PPERR_INVUNITPPACK);
		Data.Pack = (long)(upp * 1000.0);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	PPObjArticle ArObj;
	PPID   AcsID; // PPOBJ_ACCSHEET
	int    OwnCode;
};

IMPL_HANDLE_EVENT(ArGoodsCodeDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_ARGOODSCODE_CODE)) {
		SString temp_buf, code;
		getCtrlString(CTL_ARGOODSCODE_CODE, code);
		setStaticText(CTL_ARGOODSCODE_LEN, temp_buf.Cat((uint)code.Len()));
	}
	else if(event.isCbSelected(CTLSEL_ARGOODSCODE_ACS)) {
		if(!Data.ArID) {
			AcsID = getCtrlLong(CTLSEL_ARGOODSCODE_ACS);
			SetupArCombo(this, CTLSEL_ARGOODSCODE_AR, 0L, OLW_LOADDEFONOPEN, AcsID, sacfDisableIfZeroSheet|sacfNonGeneric);
		}
		else if(AcsID != getCtrlLong(CTLSEL_ARGOODSCODE_ACS))
			setCtrlLong(CTLSEL_ARGOODSCODE_ACS, AcsID);
	}
	else if(event.isKeyDown(kbF2) && isCurrCtlID(CTL_ARGOODSCODE_CODE)) {
		SString code;
		getCtrlString(CTL_ARGOODSCODE_CODE, code);
		if(!code.NotEmptyS()) {
			if(PPObjGoods::GenerateOwnArCode(code, 1) > 0)
				setCtrlString(CTL_ARGOODSCODE_CODE, code);
		}
	}
	else
		return;
	clearEvent(event);
}

static int _EditArGoodsCodeItem(ArGoodsCodeTbl::Rec * pRec, int ownCode) { DIALOG_PROC_BODY_P1(ArGoodsCodeDialog, ownCode, pRec); }

class ArGoodsCodeListDialog : public PPListDialog {
	DECL_DIALOG_DATA(ArGoodsCodeArray);
public:
	ArGoodsCodeListDialog(PPID goodsID, PPID defArID) : PPListDialog(DLG_ARGCODELIST, CTL_ARGCODELIST_LIST), GoodsID(goodsID), DefArID(defArID)
	{
		if(GoodsID) {
			SString goods_name;
			GetGoodsName(GoodsID, goods_name);
			setStaticText(CTL_ARGCODELIST_GOODS, goods_name);
		}
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		updateList(0);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);
	virtual int moveItem(long pos, long id, int up);

	PPID   GoodsID;
	PPID   DefArID;
};

int ArGoodsCodeListDialog::setupList()
{
	StringSet ss(SLBColumnDelim);
	SString sub;
	ArGoodsCodeTbl::Rec * p_item;
	for(uint i = 0; Data.enumItems(&i, (void **)&p_item);) {
		ss.Z();
		GetArticleName(p_item->ArID, sub);
		ss.add(sub);
		ss.add(p_item->Code);
		sub.Z().Cat(fdiv1000i(p_item->Pack), MKSFMTD(0, 3, NMBF_NOTRAILZ));
		ss.add(sub);
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int ArGoodsCodeListDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
	ArGoodsCodeTbl::Rec item;
	item.GoodsID = GoodsID;
	item.ArID = DefArID;
	item.Pack = 1000;
	while(ok < 0 && _EditArGoodsCodeItem(&item, 0) > 0) {
		uint pos = 0;
		if(Data.lsearch(&item.ArID, &pos, CMPF_LONG, offsetof(ArGoodsCodeTbl::Rec, ArID)))
			PPError(PPERR_ARCODEDUPAR, 0);
		else if(Data.insert(&item)) {
			ASSIGN_PTR(pPos, Data.getCount()-1);
			ok = 1;
		}
		else
			PPError(PPERR_SLIB, 0);
	}
	return ok;
}

int ArGoodsCodeListDialog::editItem(long pos, long id)
{
	if(pos >= 0 && pos < Data.getCountI()) {
		ArGoodsCodeTbl::Rec item = Data.at(static_cast<uint>(pos));
		while(_EditArGoodsCodeItem(&item, BIN(item.ArID == 0)) > 0) {
			int    r = 1;
			uint   ext_pos = 0;
			while(r && Data.lsearch(&item.ArID, &ext_pos, CMPF_LONG, offsetof(ArGoodsCodeTbl::Rec, ArID)))
				if(ext_pos != (uint)pos)
					r = (PPError(PPERR_ARCODEDUPAR, 0), 0);
				else
					ext_pos++;
			if(r) {
				Data.at(static_cast<uint>(pos)) = item;
				return 1;
			}
		}
	}
	return -1;
}

int ArGoodsCodeListDialog::moveItem(long pos, long id, int up)
{
	int    ok = 1;
	if(up && pos > 0)
		Data.swap(pos, pos-1);
	else if(!up && pos < (Data.getCountI()-1))
		Data.swap(pos, pos+1);
	else
		ok = -1;
	return ok;
}

int ArGoodsCodeListDialog::delItem(long pos, long id) { return Data.atFree(static_cast<uint>(pos)) ? 1 : -1; }

int PPObjGoods::EditArCode(PPID goodsID, PPID arID, int ownCode)
{
	int    ok = -1;
	PPGoodsPacket pack;
	Goods2Tbl::Rec goods_rec;
	if(CConfig.Flags & CCFLG_USEARGOODSCODE && Fetch(goodsID, &goods_rec) > 0) {
		ArGoodsCodeArray code_list;
		if(arID || ownCode) {
			ArGoodsCodeTbl::Rec code_rec;
			uint   pos = 0;
			int    is_new = 0;
			THROW(P_Tbl->ReadArCodes(goodsID, &code_list));
			if(code_list.lsearch(&arID, &pos, CMPF_LONG, offsetof(ArGoodsCodeTbl::Rec, ArID))) {
				code_rec = code_list.at(pos);
			}
			else {
				code_rec.GoodsID = goodsID;
				code_rec.ArID = arID;
				code_rec.Pack = 1000;
				is_new = 1;
			}
			while(ok < 0 && _EditArGoodsCodeItem(&code_rec, ownCode) > 0) {
				if(is_new) {
					pos = code_list.getCount();
					code_list.insert(&code_rec);
					is_new = 0;
				}
				else
					code_list.at(pos) = code_rec;
				if(!P_Tbl->UpdateArCodes(goodsID, &code_list, 1))
					PPError();
				else
					ok = 1;
			}
		}
		else {
			ArGoodsCodeListDialog * dlg = 0;
			THROW(P_Tbl->ReadArCodes(goodsID, &code_list));
			if(CheckDialogPtrErr(&(dlg = new ArGoodsCodeListDialog(goodsID, arID)))) {
				dlg->setDTS(&code_list);
				while(ok < 0 && ExecView(dlg) == cmOK) {
					if(dlg->getDTS(&code_list))
						if(!P_Tbl->UpdateArCodes(goodsID, &code_list, 1))
							PPError();
						else
							ok = 1;
				}
				delete dlg;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
ClsdGoodsDialog::ClsdGoodsDialog(uint dlgID, PPGdsClsPacket * pGcPack, int modifyOnlyExtRec) : TDialog(dlgID),
	ZeroFlags(0), ModifyOnlyExtRec(modifyOnlyExtRec), GcPack(*pGcPack)
{
	if(ModifyOnlyExtRec) {
		showCtrl(CTL_SG_FULLDLGBUTTON, false);
		enableCommand(cmFullGoodsDialog, 0);
	}
	if(ModifyOnlyExtRec != 2) {
		showCtrl(CTL_SG_ZEROX, false);
		showCtrl(CTL_SG_ZEROY, false);
		showCtrl(CTL_SG_ZEROZ, false);
		showCtrl(CTL_SG_ZEROW, false);
	}
	disableCtrl(CTLSEL_SG_CLS, ModifyOnlyExtRec != 2);
}

void ClsdGoodsDialog::printLabel()
{
	if(getDTS(0)) {
		if(Data.Rec.ID)
			BarcodeLabelPrinter::PrintGoodsLabel(Data.Rec.ID);
		else {
			SString temp_buf;
			RetailGoodsInfo rgi;
			Data.Codes.GetSingle(0, temp_buf);
			temp_buf.CopyTo(rgi.BarCode, sizeof(rgi.BarCode));
			STRNSCPY(rgi.Name, Data.Rec.Name);
			BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, 0, 0);
		}
	}
}

IMPL_HANDLE_EVENT(ClsdGoodsDialog)
{
	TDialog::handleEvent(event);
	uint16 zero = 0;
	if(TVCOMMAND) {
		if(event.isCbSelected(CTLSEL_SG_CLS) && ModifyOnlyExtRec == 2) {
			PPID   new_cls_id = getCtrlLong(CTLSEL_SG_CLS);
			if(new_cls_id != GcPack.Rec.ID) {
				PPObjGoodsClass gc_obj;
				PPGdsClsPacket gc_pack;
				if(gc_obj.GetPacket(new_cls_id, &gc_pack) > 0) {
					GcPack = gc_pack;
					if(new_cls_id == Org.ExtRec.GoodsClsID) {
						Data = Org;
					}
					else {
						MEMSZERO(Data.ExtRec);
						Data.Rec.GdsClsID = new_cls_id;
						Data.ExtRec.GoodsClsID = new_cls_id;
					}
					setDTS(0);
				}
				else
					setCtrlLong(CTLSEL_SG_CLS, GcPack.Rec.ID);
			}
		}
		else if(event.isClusterClk(CTL_SG_ZEROX))
			setupZeroDim(CTL_SG_ZEROX, CTL_SG_DIMX, cmSgDimX);
		else if(event.isClusterClk(CTL_SG_ZEROY))
			setupZeroDim(CTL_SG_ZEROY, CTL_SG_DIMY, cmSgDimY);
		else if(event.isClusterClk(CTL_SG_ZEROZ))
			setupZeroDim(CTL_SG_ZEROZ, CTL_SG_DIMZ, cmSgDimZ);
		else if(event.isClusterClk(CTL_SG_ZEROW))
			setupZeroDim(CTL_SG_ZEROW, CTL_SG_DIMW, cmSgDimW);
		else if(TVCMD == cmSgDimX)
			selectDim(CTL_SG_DIMX, &GcPack.DimX);
		else if(TVCMD == cmSgDimY)
			selectDim(CTL_SG_DIMY, &GcPack.DimY);
		else if(TVCMD == cmSgDimZ)
			selectDim(CTL_SG_DIMZ, &GcPack.DimZ);
		else if(TVCMD == cmSgDimW)
			selectDim(CTL_SG_DIMW, &GcPack.DimW);
		else if(TVCMD == cmFullGoodsDialog) {
			if(IsInState(sfModal)) {
				endModal(TVCMD);
				return; // После endModal не следует обращаться к this
			}
		}
		else
			return;
	}
	else if(TVKEYDOWN)
		if(TVKEY == kbF2) {
			switch(GetCurrId()) {
				case CTL_SG_DIMX: selectDim(CTL_SG_DIMX, &GcPack.DimX); break;
				case CTL_SG_DIMY: selectDim(CTL_SG_DIMY, &GcPack.DimY); break;
				case CTL_SG_DIMZ: selectDim(CTL_SG_DIMZ, &GcPack.DimZ); break;
				case CTL_SG_DIMW: selectDim(CTL_SG_DIMW, &GcPack.DimW); break;
				default: return;
			}
		}
		else if(TVKEY == kbF7 || TVCHR == kbCtrlL)
			printLabel();
		else
			return;
	else
		return;
	clearEvent(event);
}

int ClsdGoodsDialog::selectDim(uint ctlID, PPGdsClsDim * pDim)
{
	int    ok = -1;
	int    scale = static_cast<int>(pDim->Scale);
	double rval = getCtrlReal(ctlID);
	long   lval = static_cast<long>(rval * fpow10i(scale));
	ListWindow * p_lw = CreateListWindow_Simple(lbtDblClkNotify);
	if(p_lw) {
		for(uint i = 0; i < pDim->ValList.getCount(); i++) {
			char   item_buf[64];
			long   tmp_lval = pDim->ValList.at(i);
			realfmt(static_cast<double>(tmp_lval) / fpow10i(scale), MKSFMTD(0, scale, NMBF_NOTRAILZ), item_buf);
			p_lw->listBox()->addItem(tmp_lval, item_buf);
		}
		p_lw->listBox()->TransmitData(+1, &lval);
		if(ExecView(p_lw) == cmOK) {
			p_lw->listBox()->TransmitData(-1, &lval);
			setCtrlReal(ctlID, static_cast<double>(lval) / fpow10i(scale));
			ok = 1;
		}
	}
	else
		ok = PPErrorZ();
	delete p_lw;
	return ok;
}

void ClsdGoodsDialog::setupZeroDim(uint zeroCtlId, uint inpId, uint selCmd)
{
	if(ModifyOnlyExtRec == 2) {
		const uint16 zero = getCtrlUInt16(zeroCtlId);
		disableCtrl(inpId, LOGIC(zero));
		enableCommand(selCmd, !zero);
	}
}

int ClsdGoodsDialog::setupGdsClsDim(uint inpID, uint selCmd, uint labelID, uint zeroID, uint flag, PPGdsClsDim * pDim, long val)
{
	const int x = BIN(GcPack.Rec.Flags & flag);
	disableCtrls(!x, inpID, labelID, zeroID, 0);
	enableCommand(selCmd, x);
	setStaticText(labelID, x ? pDim->Name : 0);
	setCtrlReal(inpID, x ? (((double)val) / fpow10i((int)pDim->Scale)) : 0);
	{
		const uint   boffs = PPGdsCls::UseFlagToE(flag);
		const uint16 zero = static_cast<uint16>((ModifyOnlyExtRec == 2 && zeroID && (boffs > 0)) ? (ZeroFlags & (1 < (boffs-1))) : 0);
		setCtrlUInt16(zeroID, zero);
		setupZeroDim(zeroID, inpID, selCmd);
	}
	return 1;
}

int ClsdGoodsDialog::setupGdsClsProp(uint selID, uint labelID, uint flag, PPGdsClsProp * pProp, PPID dataID)
{
	const int x = BIN(GcPack.Rec.Flags & flag);
	disableCtrls(!x, selID, labelID, 0);
	setStaticText(labelID, x ? pProp->Name : 0);
	if(x && pProp->ItemsListID)
		SetupPPObjCombo(this, selID, pProp->ItemsListID, dataID, OLW_CANINSERT | OLW_LOADDEFONOPEN, 0);
	return 1;
}

int ClsdGoodsDialog::setDTS(const PPGoodsPacket * pPack)
{
	int    ok = 1;
	if(pPack) {
		Data = *pPack;
		Org = *pPack;
	}
	setTitle(GcPack.Rec.Name);
	SetupPPObjCombo(this, CTLSEL_SG_CLS, PPOBJ_GOODSCLASS, Data.Rec.GdsClsID, 0, 0);
	setupGdsClsProp(CTLSEL_SG_KIND,     CTL_SG_L_KIND,     PPGdsCls::fUsePropKind,  &GcPack.PropKind,  Data.ExtRec.KindID);
	setupGdsClsProp(CTLSEL_SG_ADDPROP,  CTL_SG_L_ADDPROP,  PPGdsCls::fUsePropAdd,   &GcPack.PropAdd,   Data.ExtRec.AddObjID);
	setupGdsClsProp(CTLSEL_SG_GRADE,    CTL_SG_L_GRADE,    PPGdsCls::fUsePropGrade, &GcPack.PropGrade, Data.ExtRec.GradeID);
	setupGdsClsProp(CTLSEL_SG_ADD2PROP, CTL_SG_L_ADD2PROP, PPGdsCls::fUsePropAdd2,  &GcPack.PropAdd2,  Data.ExtRec.AddObj2ID);
	setupGdsClsDim(CTL_SG_DIMX, cmSgDimX, CTL_SG_L_DIMX, CTL_SG_ZEROX, PPGdsCls::fUseDimX, &GcPack.DimX, Data.ExtRec.X);
	setupGdsClsDim(CTL_SG_DIMY, cmSgDimY, CTL_SG_L_DIMY, CTL_SG_ZEROY, PPGdsCls::fUseDimY, &GcPack.DimY, Data.ExtRec.Y);
	setupGdsClsDim(CTL_SG_DIMZ, cmSgDimZ, CTL_SG_L_DIMZ, CTL_SG_ZEROZ, PPGdsCls::fUseDimZ, &GcPack.DimZ, Data.ExtRec.Z);
	setupGdsClsDim(CTL_SG_DIMW, cmSgDimW, CTL_SG_L_DIMW, CTL_SG_ZEROW, PPGdsCls::fUseDimW, &GcPack.DimW, Data.ExtRec.W);
	return ok;
}

int ClsdGoodsDialog::getDim(uint ctlID, uint zeroID, long f, /*long scale*/const PPGdsClsDim * pDim, long * pVal)
{
	int    ok = -1;
	if(GcPack.Rec.Flags & f) {
		uint16 zero = (ModifyOnlyExtRec == 2 && zeroID) ? getCtrlUInt16(zeroID) : 0;
		if(zero) {
			uint boffs = PPGdsCls::UseFlagToE(f);
			if(boffs > 0) {
				ZeroFlags |= (1 << (boffs-1));
			}
			else
				zero = 0;
		}
		if(zero) {
			*pVal = 0;
		}
		else {
			double rval = getCtrlReal(ctlID);
			*pVal = R0i(rval * fpow10i((int)pDim->Scale));
		}
		ok = 1;
		if(GcPack.Rec.Flags & PPGdsCls::fDisableFreeDim && pDim->ValList.getCount())
			if(!pDim->ValList.lsearch(*pVal))
				ok = PPSetError(PPERR_INVGDSCLSDIM, pDim->Name);
	}
	return ok;
}

int ClsdGoodsDialog::getDTS(PPGoodsPacket * pPack)
{
	int    ok = 1;
	if(GcPack.Rec.Flags & PPGdsCls::fUsePropKind)
		getCtrlData(CTLSEL_SG_KIND, &Data.ExtRec.KindID);
	if(GcPack.Rec.Flags & PPGdsCls::fUsePropAdd)
		getCtrlData(CTLSEL_SG_ADDPROP, &Data.ExtRec.AddObjID);
	if(GcPack.Rec.Flags & PPGdsCls::fUsePropGrade)
		getCtrlData(CTLSEL_SG_GRADE, &Data.ExtRec.GradeID);
	if(GcPack.Rec.Flags & PPGdsCls::fUsePropAdd2)
		getCtrlData(CTLSEL_SG_ADD2PROP, &Data.ExtRec.AddObj2ID);
	THROW(getDim(CTL_SG_DIMX, CTL_SG_ZEROX, PPGdsCls::fUseDimX, &GcPack.DimX, &Data.ExtRec.X));
	THROW(getDim(CTL_SG_DIMY, CTL_SG_ZEROY, PPGdsCls::fUseDimY, &GcPack.DimY, &Data.ExtRec.Y));
	THROW(getDim(CTL_SG_DIMZ, CTL_SG_ZEROZ, PPGdsCls::fUseDimZ, &GcPack.DimZ, &Data.ExtRec.Z));
	THROW(getDim(CTL_SG_DIMW, CTL_SG_ZEROW, PPGdsCls::fUseDimW, &GcPack.DimW, &Data.ExtRec.W));
	GcPack.CompleteGoodsPacket(&Data);
	if(ModifyOnlyExtRec == 2) {
		Data.ClsDimZeroFlags = ZeroFlags;
	}
	ASSIGN_PTR(pPack, Data);
	CATCHZOKPPERR
	return ok;
}
//
//
//
GoodsDialog::GoodsDialog(uint rezID) : TDialog(rezID), St(0), gpk(gpkndUndef)
{
	const  long ccflags = CConfig.Flags;
	const  bool use_gdscls = LOGIC(ccflags & CCFLG_USEGDSCLS);
	Ptb.SetBrush(brushPriorBarcode, SPaintObj::bsSolid, GetColorRef(SClrGreen), 0);
	enableCommand(cmGoodsExt, use_gdscls);
	enableCommand(cmArGoodsCodeList, (ccflags & CCFLG_USEARGOODSCODE));
	enableCommand(cmGoodsStruc, PPObjGoodsStruc().CheckRights(PPR_READ));
	disableCtrl(CTLSEL_GOODS_CLS, !use_gdscls);
	if(GObj.GetConfig().Flags & GCF_DISABLEWOTAXFLAG)
		St |= stWoTaxFlagDisabled;
	addGroup(ctlgroupIBG, new ImageBrowseCtrlGroup(/*PPTXT_PICFILESEXTS,*/CTL_GOODS_IMAGE, cmAddImage, cmDelImage, GObj.CheckRights(GOODSRT_UPDIMAGE)));
}

void GoodsDialog::setupInhTaxGrpName()
{
	Goods2Tbl::Rec grp_rec;
	PPID   parent_id = getCtrlLong(CTLSEL_GOODS_GROUP);
	if(GObj.Fetch(parent_id, &grp_rec) > 0) {
		SString temp_buf, inh_tax_name;
		getStaticText(CTL_GOODS_ST_INH_TAXGRP, temp_buf);
		GetObjectName(PPOBJ_GOODSTAX, grp_rec.TaxGrpID, inh_tax_name);
		temp_buf.Strip().Space().Cat(inh_tax_name);
		setStaticText(CTL_GOODS_ST_INH_TAXGRP, temp_buf);
	}
}

int PPObjGoods::__Helper_GetPriceRestrictions_ByFormula(SString & rFormula, const PPGoodsPacket * pPack, double & rBound)
{
	int    ok = -1;
	rBound = 0.0;
	if(pPack && rFormula.NotEmptyS()) {
		double bound = 0.0;
		PPGdsClsPacket gc_pack;
		PPGdsClsPacket * p_gc_pack = 0;
		if(pPack->Rec.GdsClsID) {
			PPObjGoodsClass gc_obj;
			if(gc_obj.Fetch(pPack->Rec.GdsClsID, &gc_pack) > 0)
				p_gc_pack = &gc_pack;
		}
		GdsClsCalcExprContext ctx(p_gc_pack, pPack);
		if(PPCalcExpression(rFormula, &bound, &ctx) && bound > 0.0) {
			rBound = bound;
			ok = 1;
		}
	}
	return ok;
}

void GoodsDialog::SetupAddedInfo()
{
	SString title_buf, temp_buf;
	const  PPID goods_id = Data.Rec.ID;
	if(goods_id) {
		PPObjGoods::ReadGoodsExTitles(Data.Rec.ParentID, title_buf);
		PPGetExtStrData(GDSEXSTR_INFOSYMB, title_buf, temp_buf);
		if(temp_buf == "lastselldate") {
			PPID    op_id = CConfig.RetailOp;
			if(op_id) {
				LDATE   last_sell_date = ZERODATE;
				TransferTbl & r_t = *BillObj->trfr;
				TransferTbl::Key3 k3;
				k3.GoodsID = goods_id;
				k3.Dt = MAXDATE;
				k3.OprNo = MAXLONG;
				if(r_t.search(3, &k3, spLt) && r_t.data.GoodsID == goods_id) do {
					BillTbl::Rec bill_rec;
					if(BillObj->Fetch(r_t.data.BillID, &bill_rec) > 0 && bill_rec.OpID == op_id) {
						last_sell_date = r_t.data.Dt;
					}
				} while(!last_sell_date && r_t.search(3, &k3, spLt) && r_t.data.GoodsID == goods_id);
				PPLoadString("lastselldate", title_buf);
                title_buf.CatDiv(':', 2);
                if(checkdate(last_sell_date))
					title_buf.Cat(last_sell_date, DATF_DMY|DATF_CENTURY);
                else {
					PPLoadString("no", temp_buf);
					title_buf.Cat(temp_buf);
                }
			}
			else
				title_buf.Z();
		}
		else if(temp_buf == "pricerestriction") {
			title_buf = "no price restriction";
			if(Data.Rec.ID && Data.Rec.GoodsTypeID) {
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				if(gt_obj.Fetch(Data.Rec.GoodsTypeID, &gt_rec) > 0 && gt_rec.PriceRestrID) {
					PPObjGoodsValRestr gvr_obj;
					PPGoodsValRestrPacket gvr_pack;
					RealRange range;
					int   pr = 0;
					if(gvr_obj.Fetch(gt_rec.PriceRestrID, &gvr_pack) > 0) {
						if(GObj.__Helper_GetPriceRestrictions_ByFormula(gvr_pack.LowBoundFormula, &Data, range.low) > 0)
							pr = 1;
						if(GObj.__Helper_GetPriceRestrictions_ByFormula(gvr_pack.UppBoundFormula, &Data, range.upp) > 0)
							pr = 1;
						(title_buf = "price restriction").CatDiv(':', 2);
						if(range.low > 0.0)
							title_buf.Cat(range.low, MKSFMTD_020);
						title_buf.Dot().Dot();
						if(range.upp > 0.0)
							title_buf.Cat(range.upp, MKSFMTD_020);
					}
				}
			}

		}
		else
			title_buf.Z();
	}
	setStaticText(CTL_GOODS_ST_INFO, title_buf);
}

int GoodsDialog::setDTS(const PPGoodsPacket * pPack)
{
	int    ok = 1;
	PPID   prev_grp_level = 0;
	RVALUEPTR(Data, pPack);
	showButton(cmSearchUHTT, Data.Rec.ID == 0);
	gpk  = Data.GetPacketKind();
	if(gpk == gpkndGoods && GObj.ValidateGoodsParent(Data.Rec.ParentID) <= 0) {
		if(GObj.ValidateGoodsParent(GObj.GetConfig().DefGroupID) > 0) {
			Data.Rec.ParentID = GObj.GetConfig().DefGroupID;
		}
	}
	if(Data.Rec.ParentID) {
		Goods2Tbl::Rec grp_rec;
		if(GObj.Fetch(Data.Rec.ParentID, &grp_rec) > 0)
			prev_grp_level = grp_rec.ParentID;
	}
	long   f = OLW_LOADDEFONOPEN | OLW_WORDSELECTOR | ((gpk == gpkndGoods) ? OLW_CANINSERT : OLW_CANSELUPLEVEL); // @v11.1.10 OLW_WORDSELECTOR
	long   selgrp_bias = GGRTYP_SEL_NORMAL;
	setCtrlLong(CTL_GOODS_ID, Data.Rec.ID);
	if(oneof2(gpk, gpkndGoods, gpkndOrdinaryGroup)) {
		if(CConfig.Flags & CCFLG_USEGDSCLS) {
			SetupPPObjCombo(this, CTLSEL_GOODS_CLS, PPOBJ_GOODSCLASS, Data.Rec.GdsClsID, OLW_LOADDEFONOPEN, 0);
			enableCommand(cmGoodsExt, BIN(Data.Rec.GdsClsID));
		}
		SetupPPObjCombo(this, CTLSEL_GOODS_MANUF,  PPOBJ_PERSON, Data.Rec.ManufID, OLW_CANINSERT|OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPPRK_MANUF));
		SetupPPObjCombo(this, CTLSEL_GOODS_BRAND,  PPOBJ_BRAND,  Data.Rec.BrandID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
		SetupPPObjCombo(this, CTLSEL_GOODS_UNIT,   PPOBJ_UNIT,   Data.Rec.UnitID, OLW_CANINSERT|OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPUnit::Trade));
		SetupPPObjCombo(this, CTLSEL_GOODS_PHUNIT, PPOBJ_UNIT,   Data.Rec.PhUnitID, OLW_CANINSERT|OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPUnit::Trade|PPUnit::Physical));
		SetupPPObjCombo(this, CTLSEL_GOODS_TYPE, PPOBJ_GOODSTYPE, Data.Rec.GoodsTypeID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
		SetupPPObjCombo(this, CTLSEL_GOODS_TAX, PPOBJ_GOODSTAX, Data.Rec.TaxGrpID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
		//
		// Флаги прав доступа GOODSRT_UPDTAXGRP и GOODSRT_UPDGTYPE определяются только для //
		// объекта PPOBJ_GOODS, но распространяется так же и на объект PPOBJ_GOODSGROUP    //
		//
		disableCtrl(CTLSEL_GOODS_TAX,  pPack->Rec.ID && !GObj.CheckRights(GOODSRT_UPDTAXGRP));
		disableCtrl(CTLSEL_GOODS_TYPE, pPack->Rec.ID && !GObj.CheckRights(GOODSRT_UPDGTYPE));
		setupInhTaxGrpName();
		if(gpk == gpkndGoods) {
			SString own_ar_code;
			Data.GetArCode(0, own_ar_code);
			setStaticText(CTL_GOODS_OWNARCODE, own_ar_code);
			SetupAddedInfo();
		}
		else { // gpk == gpkndOrdinaryGroup
			SetupPPObjCombo(this, CTLSEL_GOODS_BCODESTRUC, PPOBJ_BCODESTRUC, Data.Rec.DefBCodeStrucID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
			SetupPPObjCombo(this, CTLSEL_GOODS_DEFPRC, PPOBJ_PROCESSOR, Data.Rec.DefPrcID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN);
			SetupPPObjCombo(this, CTLSEL_GOODS_DEFGSTRUC, PPOBJ_GOODSSTRUC, Data.Rec.StrucID, 0);
			selgrp_bias = GGRTYP_SEL_FOLDER;
			setCtrlLong(CTL_GOODSGROUP_LIMIT, Data.Rec.GoodsLimit);
			//
			AddClusterAssoc(CTL_GOODS_GRPFLAGS, 0, GF_UNCLASSF);
			SetClusterData(CTL_GOODS_GRPFLAGS, Data.Rec.Flags);
		}
	}
	else if(gpk == gpkndFolderGroup) {
		SetupPPObjCombo(this, CTLSEL_GOODS_TYPE, PPOBJ_GOODSTYPE, Data.Rec.GoodsTypeID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
		setCtrlUInt16(CTL_GOODS_FLDFLAGS, BIN(Data.Rec.Flags & GF_EXCLALTFOLD));
		selgrp_bias = GGRTYP_SEL_FOLDER;
	}
	SetupPPObjCombo(this, CTLSEL_GOODS_GROUP, PPOBJ_GOODSGROUP, Data.Rec.ParentID, f, reinterpret_cast<void *>(prev_grp_level + selgrp_bias));
	if(Data.Rec.ID) {
		disableCtrl(CTL_GOODS_GENERIC, !PPMaster);
		//disableCtrl(CTLSEL_GOODS_CLS, Data.Rec.GdsClsID && !PPMaster);
	}
	enableCommand(cmEditObjAssoc, Data.Rec.ID != 0); // AHTOXA
	SetupPPObjCombo(this, CTLSEL_GOODS_WROFFGRP, PPOBJ_ASSTWROFFGRP, Data.Rec.WrOffGrpID, OLW_CANINSERT|OLW_LOADDEFONOPEN);
	setCtrlData(CTL_GOODS_NAME,      Data.Rec.Name);
	setCtrlData(CTL_GOODS_ABBR,      Data.Rec.Abbr);
	setCtrlData(CTL_GOODS_PHUPERU,   &Data.Rec.PhUPerU);
	SETFLAG(St, stPrevGenTag, (Data.Rec.Flags & GF_GENERIC));
	setCtrlUInt16(CTL_GOODS_GENERIC, BIN(Data.Rec.Flags & GF_GENERIC));
	SETFLAG(St, stOrgWoTaxFlagsIsTrue, (Data.Rec.Flags & GF_PRICEWOTAXES));
	AddClusterAssoc(CTL_GOODS_FLAGS, 0, GF_NODISCOUNT);
	AddClusterAssoc(CTL_GOODS_FLAGS, 1, GF_PRICEWOTAXES);
	AddClusterAssoc(CTL_GOODS_FLAGS, 2, GF_PASSIV);
	AddClusterAssoc(CTL_GOODS_FLAGS, 3, GF_USEINDEPWT);
	AddClusterAssoc(CTL_GOODS_FLAGS, 4, GF_WANTVETISCERT);
	SetClusterData(CTL_GOODS_FLAGS, Data.Rec.Flags);
	AddClusterAssoc(CTL_GOODS_ASSETFLAGS, 0, GF_WROFFBYPRICE);
	SetClusterData(CTL_GOODS_ASSETFLAGS, Data.Rec.Flags);
	AddClusterAssoc(CTL_GOODS_DYNAMIC, 0, GF_DYNAMIC);
	SetClusterData(CTL_GOODS_DYNAMIC, Data.Rec.Flags);
	if(GObj.IsAssetType(Data.Rec.GoodsTypeID)) {
		SString okof_buf;
		Data.GetExtStrData(GDSEXSTR_OKOF, okof_buf);
		setCtrlString(CTL_GOODS_OKOF, okof_buf);
	}
	setupBarcode();
	{
		ImageBrowseCtrlGroup::Rec rec;
		Data.LinkFiles.Init(PPOBJ_GOODS);
		if(Data.Rec.Flags & GF_HASIMAGES)
			Data.LinkFiles.Load(Data.Rec.ID, 0L);
		Data.LinkFiles.At(0, rec.Path);
		setGroupData(ctlgroupIBG, &rec);
	}
	return ok;
}

int GoodsDialog::getDTS(PPGoodsPacket * pPack)
{
	int    ok = 1;
	int    sel = 0;
	PPID   parent_id = 0;
	ushort v = 0;
	getCtrlData(sel = CTL_GOODS_NAME, Data.Rec.Name);
	strip(Data.Rec.Name);
	if(gpk == gpkndGoods) {
		getCtrlData(sel = CTL_GOODS_ABBR, Data.Rec.Abbr);
		strip(Data.Rec.Abbr);
	}
	getCtrlData(sel = CTLSEL_GOODS_GROUP, &parent_id);
	Data.Rec.ParentID = parent_id;
	// THROW(!(GObj.P_Tbl->Data.Flags & GF_FOLDER), PPERR_GGROUPHASBRANCHES);
	getCtrlData(sel = CTLSEL_GOODS_MANUF,  &Data.Rec.ManufID);
	getCtrlData(CTLSEL_GOODS_BRAND, &Data.Rec.BrandID);
	getCtrlData(CTLSEL_GOODS_TYPE, &Data.Rec.GoodsTypeID);
	getCtrlData(CTLSEL_GOODS_TAX,  &Data.Rec.TaxGrpID);
	getCtrlData(CTLSEL_GOODS_CLS,  &Data.Rec.GdsClsID);
	getCtrlData(CTLSEL_GOODS_UNIT, &Data.Rec.UnitID);
	sel = CTL_GOODS_UNIT;
	getCtrlData(CTLSEL_GOODS_PHUNIT, &Data.Rec.PhUnitID);
	if(gpk == gpkndGoods) {
		getCtrlData(sel = CTL_GOODS_PHUPERU, &Data.Rec.PhUPerU);
	}
	else if(gpk == gpkndOrdinaryGroup) {
		getCtrlData(CTLSEL_GOODS_BCODESTRUC, &Data.Rec.DefBCodeStrucID);
		getCtrlData(CTLSEL_GOODS_DEFPRC, &Data.Rec.DefPrcID);
		getCtrlData(CTLSEL_GOODS_DEFGSTRUC, &Data.Rec.StrucID);
		Data.Rec.GoodsLimit = getCtrlLong(CTL_GOODSGROUP_LIMIT);
		GetClusterData(CTL_GOODS_GRPFLAGS, &Data.Rec.Flags);
	}
	if(Data.Rec.PhUnitID == 0)
		Data.Rec.PhUPerU = 0.0;
	{
		const int gen = BIN(getCtrlUInt16(CTL_GOODS_GENERIC));
		// Добавлена проверка на существование лотов
		if(!Data.Rec.ID || (gen != BIN(St & stPrevGenTag) && !gen && (BillObj->trfr->Rcpt.EnumByGoods(Data.Rec.ID, 0, 0) < 0)))
			SETFLAG(Data.Rec.Flags, GF_GENERIC, gen);
	}
	getCtrlData(CTLSEL_GOODS_WROFFGRP, &Data.Rec.WrOffGrpID);
	GetClusterData(CTL_GOODS_FLAGS, &Data.Rec.Flags);
	if(St & stWoTaxFlagDisabled)
		SETFLAG(Data.Rec.Flags, GF_PRICEWOTAXES, St & stOrgWoTaxFlagsIsTrue);
	if(gpk == gpkndFolderGroup) {
		v = getCtrlUInt16(CTL_GOODS_FLDFLAGS);
		SETFLAG(Data.Rec.Flags, GF_EXCLALTFOLD, v & 1);
	}
	GetClusterData(CTL_GOODS_ASSETFLAGS, &Data.Rec.Flags);
	GetClusterData(CTL_GOODS_DYNAMIC,    &Data.Rec.Flags);
	if(GObj.IsAssetType(Data.Rec.GoodsTypeID)) {
		SString okof_buf;
		getCtrlString(CTL_GOODS_OKOF, okof_buf);
		Data.PutExtStrData(GDSEXSTR_OKOF, okof_buf);
	}
	sel = CTL_GOODS_BARCODE;
	THROW(getBarcode());
	if(gpk == gpkndGoods) {
		Data.Rec.StrucID = Data.GS.Rec.ID;
	}
	{
		ImageBrowseCtrlGroup::Rec rec;
		if(getGroupData(ctlgroupIBG, &rec))
			if(rec.Path.Len()) {
				THROW(Data.LinkFiles.Replace(0, rec.Path));
			}
			else
				Data.LinkFiles.Remove(0);
		SETFLAG(Data.Rec.Flags, GF_HASIMAGES, Data.LinkFiles.GetCount());
	}
	if(pPack) {
		//
		// GoodsDialog::getDTS может быть вызвана с нулевым значением pPack
		// для заполнения внутренних полей Data при вызове вложенных диалогов.
		// В этом случае не следует проверять корректность заполнения полей.
		//
		THROW(GObj.ValidatePacket(&Data));
	}
	ASSIGN_PTR(pPack, Data);
	CATCH
		switch(PPErrCode) {
			case PPERR_NAMENEEDED: sel = CTL_GOODS_NAME; break;
			case PPERR_ABBRNEEDED: sel = CTL_GOODS_ABBR; break;
			case PPERR_UNITNEEDED: sel = CTL_GOODS_UNIT; break;
			case PPERR_INVALIDPHUPERU: sel = CTL_GOODS_PHUPERU; break;
			case PPERR_GOODSGROUPNEEDED:
			case PPERR_GOODSGRPLOOP:
			case PPERR_GGRPFOLDERNEEDED: sel = CTL_GOODS_GROUP; break;
			case PPERR_INVBARCODE:
			case PPERR_INVBCODELEN:
			case PPERR_INVBCODEPRFX:
			case PPERR_DUPBARCODE:
			case PPERR_BARCODENEEDED:    sel = CTL_GOODS_BARCODE; break;
			default: sel = 0; break;
		}
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

void GoodsDialog::setupBarcode()
{
	SString barcode;
	if(Data.Rec.Kind == PPGDSK_GROUP)
		Data.GetGroupCode(barcode);
	else {
		const uint count = Data.Codes.getCount();
		if(!count)
			barcode.Z();
		else if(count == 1)
			barcode = Data.Codes.at(0).Code;
		else
			PPLoadString("list", barcode);
		disableCtrl(CTL_GOODS_BARCODE, count > 1);
	}
	setCtrlString(CTL_GOODS_BARCODE, barcode);
}

int GoodsDialog::getBarcode()
{
	uint   count;
	int    is_group_code = 0;
	SString barcode;
	SString mark_buf;
	BarcodeTbl::Rec rec;
	if(getCtrlString(CTL_GOODS_BARCODE, barcode)) {
		barcode.Strip();
		if(PrcssrAlcReport::IsEgaisMark(barcode, &mark_buf)) {
			PrcssrAlcReport::EgaisMarkBlock emb;
			if(PrcssrAlcReport::ParseEgaisMark(mark_buf, emb)) {
				barcode = emb.EgaisCode;
				setCtrlString(CTL_GOODS_BARCODE, barcode);
			}
		}
		if(GObj.SearchBy2dBarcode(barcode, &rec, 0) > 0) {
			barcode = rec.Code;
			setCtrlString(CTL_GOODS_BARCODE, barcode);
		}
		if(Data.Rec.Kind == PPGDSK_GROUP) {
			Data.SetGroupCode(barcode);
			is_group_code = 1;
		}
		else {
			count = Data.Codes.getCount();
			if(barcode.NotEmptyS()) {
				if(!count) {
					rec.Clear();
					rec.GoodsID = Data.Rec.ID;
					rec.Qtty    = 1.0;
					STRNSCPY(rec.Code, barcode);
					if(!Data.Codes.insert(&rec))
						return PPSetErrorSLib();
				}
				else if(count == 1)
					STRNSCPY(Data.Codes.at(0).Code, barcode);
			}
			else
				Data.Codes.freeAll();
		}
		return GObj.CheckBarcodeList(&Data);
	}
	return -1;
}

void GoodsDialog::editBarcodeList()
{
	BarcodeListDialog * dlg = 0;
	int    r = getBarcode();
	if(r >= 0) {
		if(!r)
			PPError();
		PPID   goods_grp_id = getCtrlLong(CTLSEL_GOODS_GROUP);
		if(CheckDialogPtrErr(&(dlg = new BarcodeListDialog(Data.Rec.ID, goods_grp_id)))) {
			dlg->setDTS(&Data.Codes);
			if(ExecView(dlg) == cmOK)
				dlg->getDTS(&Data.Codes);
			delete dlg;
			setupBarcode();
		}
	}
}

void GoodsDialog::printLabel()
{
	if(gpk == gpkndGoods) {
		if(Data.Rec.ID)
			BarcodeLabelPrinter::PrintGoodsLabel(Data.Rec.ID);
		else {
			RetailGoodsInfo rgi;
			const uint count = Data.Codes.getCount();
			if(count) {
				if(count == 1)
					getCtrlData(CTL_GOODS_BARCODE, rgi.BarCode);
				else
					for(uint i = 0; i < count; i++)
						if(Data.Codes.at(i).Qtty == 1.0)
							STRNSCPY(rgi.BarCode, Data.Codes.at(i).Code);
				if(rgi.BarCode[0] == 0)
					STRNSCPY(rgi.BarCode, Data.Codes.at(0).Code);
			}
			getCtrlData(CTL_GOODS_NAME, rgi.Name);
			BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, 0, 0);
		}
	}
}

void GoodsDialog::printInfoLabel()
{
	PPGoodsPacket pack;
	if(Data.Rec.ID)
		if(getDTS(&pack)) {
			int ok = -1, num_copies = 1;
			{
				TDialog * dlg = new TDialog(DLG_PRNGLABEL);
				if(CheckDialogPtr(&dlg)) {
					dlg->setCtrlData(CTL_PRNGLABEL_COUNT, &num_copies);
					for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
						dlg->getCtrlData(CTL_PRNGLABEL_COUNT, &num_copies);
						if(checkirange(num_copies, 1, 1000))
							ok = valid_data = 1;
						else
							PPError(PPERR_USERINPUT, 0);
					}
				}
				ZDELETE(dlg);
			}
			if(ok > 0) {
				uint   rpt_id = REPORT_GOODSLABEL;
				GoodsLabelAlddParam param;
				MEMSZERO(param);
				param.GoodsID = pack.Rec.ID;
				param.NumCopies = num_copies;
				PPAlddPrint(rpt_id, PView(&param), 0);
			}
		}
		else
			PPError();
}

//void GoodsDialog::setupWrOffGrpButton() {}
//
//
//
int EditGoodsExTitles(SString & rGoodsExTitles)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GOODSEXTITLES);
	if(CheckDialogPtrErr(&dlg)) {
		enum {
			adinfNone = 1,
			adinfLastSellDate = 2,
			adinfPriceRestrictions = 3
		};
		long   adinf = 0;
		SString buf;
		static const struct {
			int16  F;
			uint16 C;
		} fld_to_ctl_list[] = { {GDSEXSTR_STORAGE, CTL_GOODSEXTITLES_A}, {GDSEXSTR_STANDARD, CTL_GOODSEXTITLES_B}, 
			{ GDSEXSTR_INGRED, CTL_GOODSEXTITLES_C }, { GDSEXSTR_ENERGY, CTL_GOODSEXTITLES_D }, { GDSEXSTR_USAGE, CTL_GOODSEXTITLES_E } };
		{
			for(uint i = 0; i < SIZEOFARRAY(fld_to_ctl_list); i++) {
				PPGetExtStrData(fld_to_ctl_list[i].F, rGoodsExTitles, buf);
				dlg->setCtrlString(fld_to_ctl_list[i].C, buf);
			}
		}
		/*PPGetExtStrData(GDSEXSTR_STORAGE, rGoodsExTitles, buf);
		dlg->setCtrlString(CTL_GOODSEXTITLES_A, buf);
		PPGetExtStrData(GDSEXSTR_STANDARD, rGoodsExTitles, buf);
		dlg->setCtrlString(CTL_GOODSEXTITLES_B, buf);
		PPGetExtStrData(GDSEXSTR_INGRED, rGoodsExTitles, buf);
		dlg->setCtrlString(CTL_GOODSEXTITLES_C, buf);
		PPGetExtStrData(GDSEXSTR_ENERGY, rGoodsExTitles, buf);
		dlg->setCtrlString(CTL_GOODSEXTITLES_D, buf);
		PPGetExtStrData(GDSEXSTR_USAGE, rGoodsExTitles, buf);
		dlg->setCtrlString(CTL_GOODSEXTITLES_E, buf);*/
		PPGetExtStrData(GDSEXSTR_INFOSYMB, rGoodsExTitles, buf);
		if(buf == "lastselldate")
			adinf = adinfLastSellDate;
		else if(buf == "pricerestriction")
			adinf = adinfPriceRestrictions;
		dlg->AddClusterAssocDef(CTL_GOODSEXTITLES_AI, 0, adinfNone);
		dlg->AddClusterAssoc(CTL_GOODSEXTITLES_AI, 1, adinfLastSellDate);
		dlg->AddClusterAssoc(CTL_GOODSEXTITLES_AI, 2, adinfPriceRestrictions);
		dlg->SetClusterData(CTL_GOODSEXTITLES_AI, adinf);
		if(ExecView(dlg) == cmOK) {
			rGoodsExTitles.Z();
			{
				for(uint i = 0; i < SIZEOFARRAY(fld_to_ctl_list); i++) {
					dlg->getCtrlString(fld_to_ctl_list[i].C, buf);
					PPPutExtStrData(fld_to_ctl_list[i].F, rGoodsExTitles, buf);
				}
			}
			/*dlg->getCtrlString(CTL_GOODSEXTITLES_A, buf);
			PPPutExtStrData(GDSEXSTR_STORAGE, rGoodsExTitles, buf);
			dlg->getCtrlString(CTL_GOODSEXTITLES_B, buf);
			PPPutExtStrData(GDSEXSTR_STANDARD, rGoodsExTitles, buf);
			dlg->getCtrlString(CTL_GOODSEXTITLES_C, buf);
			PPPutExtStrData(GDSEXSTR_INGRED, rGoodsExTitles, buf);
			dlg->getCtrlString(CTL_GOODSEXTITLES_D, buf);
			PPPutExtStrData(GDSEXSTR_ENERGY, rGoodsExTitles, buf);
			dlg->getCtrlString(CTL_GOODSEXTITLES_E, buf);
			PPPutExtStrData(GDSEXSTR_USAGE, rGoodsExTitles, buf);*/
			dlg->GetClusterData(CTL_GOODSEXTITLES_AI, &adinf);
			if(adinf == adinfLastSellDate)
				buf = "lastselldate";
			else if(adinf == adinfPriceRestrictions)
				buf = "pricerestriction";
			else
				buf.Z();
			PPPutExtStrData(GDSEXSTR_INFOSYMB, rGoodsExTitles, buf);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

class GoodsVadDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPGoodsPacket);
public:
	GoodsVadDialog() : PPListDialog(DLG_GOODSVAD, CTL_GOODSVAD_MINSTOCKLI), MaxExtTextLen(4000), LastPalletTypeID(0)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString ex_titles;
		if(Data.Rec.Name[0]) {
			SString title_buf = getTitle();
			title_buf.CatDiv('-', 1).Cat(Data.Rec.Name);
			setTitle(title_buf);
		}
		PPObjGoods::ReadGoodsExTitles(Data.Rec.ParentID, ex_titles);
		double brutto = R6(fdiv1000i(Data.Stock.Brutto));
		setExtStrData(CTL_GOODSVAD_STORAGE,  ex_titles);
		setExtStrData(CTL_GOODSVAD_STANDARD, ex_titles);
		setExtStrData(CTL_GOODSVAD_LABELNAME, ex_titles);
		setExtStrData(CTL_GOODSVAD_INGRED, ex_titles);
		setExtStrData(CTL_GOODSVAD_ENERGY, ex_titles);
		setExtStrData(CTL_GOODSVAD_USAGE,  ex_titles);

		setCtrlData(CTL_GOODSVAD_BRUTTO, &brutto);
		setCtrlData(CTL_GOODSVAD_PACKAGE, &Data.Stock.Package);
		setDimentions();
		setCtrlReal(CTL_GOODSVAD_VOLUME, Data.Stock.CalcVolume(1.0));
		ushort v = BIN(Data.Rec.Flags & GF_VOLUMEVAL);
		setCtrlData(CTL_GOODSVAD_VOLUMEVAL, &v);
		if(v)
			disableCtrls(1, CTL_GOODSVAD_SZWD, CTL_GOODSVAD_SZLN, CTL_GOODSVAD_SZHT, 0);
		else
			disableCtrl(CTL_GOODSVAD_VOLUME, true);
		setCtrlData(CTL_GOODSVAD_EXPIRYPRD, &Data.Stock.ExpiryPeriod);
		setCtrlReal(CTL_GOODSVAD_MINSHQTTY,  Data.Stock.MinShippmQtty);
		setCtrlUInt16(CTL_GOODSVAD_MULTMINSH, BIN(Data.Stock.GseFlags & GoodsStockExt::fMultMinShipm));
		setCtrlData(CTL_GOODSVAD_NTBRTCOEF, &Data.Stock.NettBruttCoeff);
		SetPalletData(-1);
		updateList(-1);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		double brutto = 0.0;
		Data.ExtString.Z();
		getExtStrData(CTL_GOODSVAD_STORAGE);
		getExtStrData(CTL_GOODSVAD_STANDARD);
		getExtStrData(CTL_GOODSVAD_LABELNAME);
		getExtStrData(CTL_GOODSVAD_INGRED);
		getExtStrData(CTL_GOODSVAD_ENERGY);
		getExtStrData(CTL_GOODSVAD_USAGE);
		SETFLAG(Data.Rec.Flags, GF_EXTPROP, Data.ExtString.NotEmptyS());
		getCtrlData(CTL_GOODSVAD_BRUTTO,   &brutto);
		Data.Stock.Brutto = R0i(brutto * 1000.0);
		getCtrlData(CTL_GOODSVAD_PACKAGE, &Data.Stock.Package);
		getDimentions();
		ushort v = getCtrlUInt16(CTL_GOODSVAD_VOLUMEVAL);
		if(v)
			Data.Stock.SetVolume(getCtrlReal(CTL_GOODSVAD_VOLUME));
		SETFLAG(Data.Rec.Flags, GF_VOLUMEVAL, v);
		getCtrlData(CTL_GOODSVAD_EXPIRYPRD, &Data.Stock.ExpiryPeriod);
		getCtrlData(CTL_GOODSVAD_MINSHQTTY, &Data.Stock.MinShippmQtty);
		SETFLAG(Data.Stock.GseFlags, GoodsStockExt::fMultMinShipm, getCtrlUInt16(CTL_GOODSVAD_MULTMINSH));
		getCtrlData(sel = CTL_GOODSVAD_NTBRTCOEF, &Data.Stock.NettBruttCoeff);
		THROW_PP(Data.Stock.NettBruttCoeff >= 0.0f && Data.Stock.NettBruttCoeff <= 1.0f, PPERR_INVNETTOBRUTTOCOEF);
		GetPalletData(-1);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	struct ExtStrCtlEntry {
		uint   CtlId;
		uint   LabelId;
		uint   FldId;
	};
	static ExtStrCtlEntry ExtStrCtlList[];

	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int delItem(long pos, long id);
	virtual int editItem(long pos, long id);
	const  ExtStrCtlEntry * GetExtStrEntryByCtl(uint ctlId) const;
	void   setExtStrData(uint ctlID, SString & rTitleBuf);
	void   getExtStrData(uint ctlID);
	void   setDimentions()
	{
		setCtrlLong(CTL_GOODSVAD_SZWD, Data.Stock.PckgDim.Width);
		setCtrlLong(CTL_GOODSVAD_SZLN, Data.Stock.PckgDim.Length);
		setCtrlLong(CTL_GOODSVAD_SZHT, Data.Stock.PckgDim.Height);
	}
	void   getDimentions()
	{
		getCtrlData(CTL_GOODSVAD_SZWD, &Data.Stock.PckgDim.Width);
		getCtrlData(CTL_GOODSVAD_SZLN, &Data.Stock.PckgDim.Length);
		getCtrlData(CTL_GOODSVAD_SZHT, &Data.Stock.PckgDim.Height);
	}
	int    SetPalletData(PPID palletTypeID);
	int    GetPalletData(PPID palletTypeID);

	const uint MaxExtTextLen;
	PPID   LastPalletTypeID;
};

/*virtual*/int GoodsVadDialog::setupList()
{
	int    ok = 1;
	SString buf;
	LocationFilt loc_filt(LOCTYP_WAREHOUSE);
	StrAssocArray::Item item;
	StrAssocArray loc_list;
	PPObjLocation obj_loc(&loc_filt);
	StrAssocArray * p_loc_list = obj_loc.MakeList_(0, 0);
	if(p_loc_list) {
		p_loc_list->SortByText();
		PPLoadText(PPTXT_ALLWAREHOUSES, buf);
		loc_list.Add(0, buf);
		for(uint i = 0; i < p_loc_list->getCount(); i++) {
			item = p_loc_list->Get(i);
			loc_list.Add(item.Id, item.Txt);
		}
		for(uint i = 0; i < loc_list.getCount(); i++) {
			item = loc_list.Get(i);
			double qtty = Data.Stock.GetMinStock(item.Id, 0);
			StringSet ss(SLBColumnDelim);
			ss.add(item.Txt);
			ss.add(buf.Z().Cat(qtty, MKSFMTD(0, 3, NMBF_NOZERO|NMBF_NOTRAILZ)));
			if(!addStringToList(item.Id, ss.getBuf()))
				ok = 0;
		}
		ZDELETE(p_loc_list);
	}
	else
		ok = 0;
	return ok;
}

/*virtual*/int GoodsVadDialog::delItem(long pos, long id)
{
	int    ok = 1;
	if(id >= 0)
		Data.Stock.SetMinStock(id, 0);
	else
		ok = -1;
	return ok;
}

/*virtual*/int GoodsVadDialog::editItem(long pos, long id)
{
	int    ok = -1;
	if(id >= 0) {
		double qtty = 0.0;
		SString title, input_title;
		PPLoadString("qtty", input_title);
		PPLoadString("minstock", title);
		qtty = Data.Stock.GetMinStock(id, 0);
		if(InputQttyDialog(title, input_title, &qtty) > 0) {
			Data.Stock.SetMinStock(id, qtty);
			ok = 1;
		}
	}
	return ok;
}

int GoodsVadDialog::SetPalletData(PPID palletTypeID)
{
	int    use_ext_type = 0;
	if(palletTypeID < 0) {
		PPLocationConfig loc_cfg;
		PPObjLocation::FetchConfig(&loc_cfg);
		palletTypeID = loc_cfg.DefPalletID;
	}
	else if(palletTypeID)
		use_ext_type = 1;
	GoodsStockExt::Pallet def_item;
	const uint c = Data.Stock.PltList.getCount();
	if(c) {
		if(palletTypeID) {
			for(uint i = 0; i < c; i++) {
				const GoodsStockExt::Pallet & r_item = Data.Stock.PltList.at(i);
				if(r_item.PalletTypeID == palletTypeID) {
					def_item = r_item;
					break;
				}
			}
		}
		if(!def_item.PalletTypeID) {
			if(use_ext_type)
				def_item.PalletTypeID = palletTypeID;
			else
				def_item = Data.Stock.PltList.at(0);
		}
	}
	else
		def_item.PalletTypeID = palletTypeID;
	LastPalletTypeID = def_item.PalletTypeID;
	SetupPPObjCombo(this, CTLSEL_GOODSVAD_PLTTYPE, PPOBJ_PALLET, def_item.PalletTypeID, 0, 0);
	setCtrlData(CTL_GOODSVAD_PLTLAYER,  &def_item.PacksPerLayer);
	setCtrlData(CTL_GOODSVAD_PLTLAYERC, &def_item.MaxLayers);
	return 1;
}

int GoodsVadDialog::GetPalletData(PPID palletTypeID)
{
	GoodsStockExt::Pallet def_item;
	if(palletTypeID < 0)
		getCtrlData(CTLSEL_GOODSVAD_PLTTYPE, &def_item.PalletTypeID);
	else
		def_item.PalletTypeID = palletTypeID;
	getCtrlData(CTL_GOODSVAD_PLTLAYER,  &def_item.PacksPerLayer);
	getCtrlData(CTL_GOODSVAD_PLTLAYERC, &def_item.MaxLayers);
	{
		uint   c = Data.Stock.PltList.getCount();
		if(c) {
			PPObjPallet plt_obj;
			do {
				GoodsStockExt::Pallet & r_item = Data.Stock.PltList.at(--c);
				if(r_item.PalletTypeID == 0 || plt_obj.Search(r_item.PalletTypeID, 0) <= 0) {
					Data.Stock.PltList.atFree(c);
				}
			} while(c);
		}
	}
	if(def_item.PalletTypeID) {
		int    found = 0;
		uint   c = Data.Stock.PltList.getCount();
		if(c) do {
			GoodsStockExt::Pallet & r_item = Data.Stock.PltList.at(--c);
			if(r_item.PalletTypeID == def_item.PalletTypeID) {
				if(found) {
					Data.Stock.PltList.atFree(c);
				}
				else {
					if(def_item.PacksPerLayer > 0)
						Data.Stock.PltList.at(c) = def_item;
					else
						Data.Stock.PltList.atFree(c);
					found = 1;
				}
			}
		} while(c);
		if(!found && def_item.PacksPerLayer > 0)
			Data.Stock.PltList.insert(&def_item);
	}
	return 1;
}

GoodsVadDialog::ExtStrCtlEntry GoodsVadDialog::ExtStrCtlList[] = {
	{ CTL_GOODSVAD_STORAGE,  CTL_GOODSVAD_TITLE_A, GDSEXSTR_STORAGE },
	{ CTL_GOODSVAD_STANDARD, CTL_GOODSVAD_TITLE_B, GDSEXSTR_STANDARD },
	{ CTL_GOODSVAD_LABELNAME, 0, GDSEXSTR_LABELNAME },
	{ CTL_GOODSVAD_INGRED, CTL_GOODSVAD_TITLE_C, GDSEXSTR_INGRED },
	{ CTL_GOODSVAD_ENERGY, CTL_GOODSVAD_TITLE_D, GDSEXSTR_ENERGY },
	{ CTL_GOODSVAD_USAGE,  CTL_GOODSVAD_TITLE_E, GDSEXSTR_USAGE }
};

const GoodsVadDialog::ExtStrCtlEntry * GoodsVadDialog::GetExtStrEntryByCtl(uint ctlId) const
{
	for(uint i = 0; i < SIZEOFARRAY(ExtStrCtlList); i++)
		if(ExtStrCtlList[i].CtlId == ctlId)
			return (ExtStrCtlList+i);
	return 0;
}

void GoodsVadDialog::setExtStrData(uint ctlID, /*uint titleCtlID, uint strID,*/ SString & rTitleBuf)
{
	const ExtStrCtlEntry * p_entry = GetExtStrEntryByCtl(ctlID);
	if(p_entry) {
		SString temp_buf;
		if(p_entry->FldId) {
			if(p_entry->LabelId) {
				PPGetExtStrData(p_entry->FldId, rTitleBuf, temp_buf);
				setStaticText(p_entry->LabelId, temp_buf);
			}
			Data.GetExtStrData(p_entry->FldId, temp_buf);
		}
		//
		// Нельзя устанавливать длину поля больше или равной 4096 из-за того,
		// что максрос SFMTLEN предполагает, что значение длины имеет 24 бита.
		//
		SetupInputLine(ctlID, MKSTYPE(S_ZSTRING, MaxExtTextLen), MKSFMT(MaxExtTextLen, 0));
		setCtrlString(ctlID, temp_buf);
	}
}

void GoodsVadDialog::getExtStrData(uint ctlID)
{
	const ExtStrCtlEntry * p_entry = GetExtStrEntryByCtl(ctlID);
	if(p_entry && p_entry->FldId) {
		SString temp_buf;
		getCtrlString(ctlID, temp_buf);
		if(temp_buf.NotEmptyS())
			Data.PutExtStrData(p_entry->FldId, temp_buf);
	}
}

IMPL_HANDLE_EVENT(GoodsVadDialog)
{
	PPListDialog::handleEvent(event);
	SString temp_buf;
	if(event.isKeyDown(kbF2)) {
		getCtrlString(CTL_GOODSVAD_LABELNAME, temp_buf);
		if(!temp_buf.NotEmptyS())
			setCtrlString(CTL_GOODSVAD_LABELNAME, temp_buf = Data.Rec.Name);
	}
	else if(event.isKeyDown(kbF3)) {
		const uint curr_id = GetCurrId();
		const ExtStrCtlEntry * p_entry = GetExtStrEntryByCtl(curr_id);
		if(p_entry) {
			SString title_buf;
			SString subtitle_buf;
			title_buf = Data.Rec.Name;
            if(p_entry->LabelId) {
				SString ex_titles;
				PPObjGoods::ReadGoodsExTitles(Data.Rec.ParentID, ex_titles);
				PPGetExtStrData(p_entry->FldId, ex_titles, subtitle_buf);
            }
			getCtrlString(curr_id, temp_buf.Z());
			if(BigTextDialog(MaxExtTextLen, title_buf, subtitle_buf, temp_buf) > 0) {
                setCtrlString(curr_id, temp_buf);
			}
		}
	}
	else if(event.isCmd(cmInputUpdated)) {
		if(event.isCtlEvent(CTL_GOODSVAD_SZWD) || event.isCtlEvent(CTL_GOODSVAD_SZLN) || event.isCtlEvent(CTL_GOODSVAD_SZHT) || event.isCtlEvent(CTL_GOODSVAD_PACKAGE)) {
			if(!getCtrlUInt16(CTL_GOODSVAD_VOLUMEVAL)) {
				getCtrlData(CTL_GOODSVAD_PACKAGE, &Data.Stock.Package);
				getDimentions();
				setCtrlReal(CTL_GOODSVAD_VOLUME, Data.Stock.CalcVolume(1));
			}
		}
	}
	else if(event.wasFocusChanged3(CTL_GOODSVAD_SZWD, CTL_GOODSVAD_SZLN, CTL_GOODSVAD_SZHT)) {
		getDimentions();
		setCtrlReal(CTL_GOODSVAD_VOLUME, Data.Stock.CalcVolume(1));
	}
	else if(event.isClusterClk(CTL_GOODSVAD_VOLUMEVAL)) {
		if(getCtrlUInt16(CTL_GOODSVAD_VOLUMEVAL)) {
			getDimentions();
			setCtrlReal(CTL_GOODSVAD_VOLUME, Data.Stock.CalcVolume(1));
			disableCtrls(1, CTL_GOODSVAD_SZWD, CTL_GOODSVAD_SZLN, CTL_GOODSVAD_SZHT, 0);
			disableCtrl(CTL_GOODSVAD_VOLUME, false);
		}
		else {
			Data.Stock.SetVolume(getCtrlReal(CTL_GOODSVAD_VOLUME));
			setDimentions();
			disableCtrls(0, CTL_GOODSVAD_SZWD, CTL_GOODSVAD_SZLN, CTL_GOODSVAD_SZHT, 0);
			disableCtrl(CTL_GOODSVAD_VOLUME, true);
		}
	}
	else if(event.isCbSelected(CTLSEL_GOODSVAD_PLTTYPE)) {
		PPID   pallet_type_id = getCtrlLong(CTLSEL_GOODSVAD_PLTTYPE);
		if(pallet_type_id != LastPalletTypeID) {
			GetPalletData(LastPalletTypeID);
			SetPalletData(pallet_type_id);
		}
		LastPalletTypeID = pallet_type_id;
	}
	else
		return;
	clearEvent(event);
}

int PPObjGoods::EditVad(PPID goodsID)
{
	int    ok = -1;
	PPGoodsPacket pack;
	if(GetPacket(goodsID, &pack, 0) > 0) {
		GoodsVadDialog * dlg = new GoodsVadDialog;
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(&pack);
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
				if(dlg->getDTS(&pack))
					if(PutPacket(&goodsID, &pack, 1))
						ok = valid_data = 1;
					else
						PPError();
			delete dlg;
		}
		else
			ok = 0;
	}
	return ok;
}

void GoodsDialog::completeByClass()
{
	getCtrlData(CTLSEL_GOODS_CLS, &Data.Rec.GdsClsID);
	if(Data.Rec.GdsClsID) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(Data.Rec.GdsClsID, &gc_pack) > 0) {
			getCtrlData(CTL_GOODS_NAME, Data.Rec.Name);
			strip(Data.Rec.Name);
			if(gpk == gpkndGoods) {
				getCtrlData(CTL_GOODS_ABBR, Data.Rec.Abbr);
				strip(Data.Rec.Abbr);
			}
			gc_pack.CompleteGoodsPacket(&Data);
			setCtrlData(CTL_GOODS_NAME, Data.Rec.Name);
			setCtrlData(CTL_GOODS_ABBR, Data.Rec.Abbr);
			setCtrlData(CTLSEL_GOODS_UNIT,   &Data.Rec.UnitID);
			setCtrlData(CTLSEL_GOODS_PHUNIT, &Data.Rec.PhUnitID);
			setCtrlData(CTL_GOODS_PHUPERU,   &Data.Rec.PhUPerU);
		}
	}
}

IMPL_HANDLE_EVENT(GoodsDialog)
{
	if(event.isCmd(cmOK))
		getBarcode();
	TDialog::handleEvent(event);
	if((TVBROADCAST && TVCMD == cmChangedFocus && event.isCtlEvent(CTL_GOODS_NAME)) ||
		(TVKEYDOWN && TVKEY == kbF2 && isCurrCtlID(CTL_GOODS_NAME))) {
		SString abbr;
		if(getCtrlString(CTL_GOODS_ABBR, abbr) && abbr.Strip().IsEmpty()) {
			getCtrlString(CTL_GOODS_NAME, abbr);
			setCtrlString(CTL_GOODS_ABBR, abbr.Strip());
		}
		else
			return;
	}
	else if(event.isCbSelected(CTLSEL_GOODS_CLS)) {
		PPID   gc_id = getCtrlLong(CTLSEL_GOODS_CLS);
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(gc_id, &gc_pack) > 0) {
			Data.Rec.GdsClsID = gc_id;
			enableCommand(cmGoodsExt, BIN(gc_id));
			PPID temp_id = getCtrlLong(CTLSEL_GOODS_UNIT);
			if(temp_id == 0 && gc_pack.Rec.DefUnitID)
				setCtrlLong(CTLSEL_GOODS_UNIT, gc_pack.Rec.DefUnitID);
			temp_id = getCtrlLong(CTLSEL_GOODS_GROUP);
			if(temp_id == 0 && gc_pack.Rec.DefGrpID)
				setCtrlLong(CTLSEL_GOODS_GROUP, gc_pack.Rec.DefGrpID);
		}
		else
			setCtrlLong(CTLSEL_GOODS_CLS, 0);
	}
	else if(event.isCbSelected(CTLSEL_GOODS_BRAND)) {
		const  PPID prev_brand_id = Data.Rec.BrandID;
		Data.Rec.BrandID = getCtrlLong(CTLSEL_GOODS_BRAND);
		if(Data.Rec.BrandID != prev_brand_id)
			completeByClass();
	}
	else if(event.isCbSelected(CTLSEL_GOODS_GROUP))
		setupInhTaxGrpName();
	else if(event.isClusterClk(CTL_GOODS_DYNAMIC)) {
		if(Data.Rec.ID) {
			//
			// Нельзя устанавливать признак "Динамическая альт группа" для групп,
			// которые привязаны к электронным весам.
			//
			long   flags = 0;
			GetClusterData(CTL_GOODS_DYNAMIC, &flags);
			if(flags & GF_DYNAMIC) {
				PPScale item;
				PPObjScale obj_scale;
				for(SEnum en = obj_scale.P_Ref->Enum(PPOBJ_SCALE, 0); en.Next(&item) > 0;) {
					if(Data.Rec.ID == item.AltGoodsGrp) {
						SetClusterData(CTL_GOODS_DYNAMIC, flags & ~GF_DYNAMIC);
						break;
					}
				}
			}
		}
	}
	else if(event.isClusterClk(CTL_GOODS_FLAGS) && St & stWoTaxFlagDisabled) {
		ushort v = getCtrlUInt16(CTL_GOODS_FLAGS);
		SETFLAG(v, 0x0002, St & stOrgWoTaxFlagsIsTrue);
		setCtrlData(CTL_GOODS_FLAGS, &v);
	}
	else if(TVCOMMAND) {
		switch(TVCMD) {
			case cmCtlColor:
				{
					TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
					char   barcode[64];
					if(p_dc && Data.Rec.Kind == PPGDSK_GOODS) {
						if(getCtrlHandle(CTL_GOODS_BARCODE) == p_dc->H_Ctl && getCtrlData(CTL_GOODS_BARCODE, barcode)) {
							if(Data.Codes.getCount() == 1 && IsInnerBarcodeType(Data.Codes.at(0).BarcodeType, BARCODE_TYPE_PREFERRED)) {
								::SetBkMode(p_dc->H_DC, TRANSPARENT);
								::SetTextColor(p_dc->H_DC, GetColorRef(SClrWhite));
								p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(brushPriorBarcode));
								clearEvent(event);
							}
						}
					}
				}
				break;
			case cmaMore:
				editBarcodeList();
				break;
			case cmArGoodsCodeList:
				{
					ArGoodsCodeListDialog * dlg = 0;
					if(CheckDialogPtrErr(&(dlg = new ArGoodsCodeListDialog(Data.Rec.ID, 0)))) {
						dlg->setDTS(&Data.ArCodes);
						if(ExecView(dlg) == cmOK) {
							dlg->getDTS(&Data.ArCodes);
							SString own_ar_code;
							Data.GetArCode(0, own_ar_code);
							setStaticText(CTL_GOODS_OWNARCODE, own_ar_code);
						}
						delete dlg;
					}
				}
				break;
			case cmGoodsWrOffGrp:
				{
					TDialog * dlg = new TDialog(DLG_GOODSWOG);
					if(CheckDialogPtrErr(&dlg)) {
						SetupPPObjCombo(dlg, CTLSEL_GOODSWOG_WOG, PPOBJ_ASSTWROFFGRP, Data.Rec.WrOffGrpID, OLW_CANINSERT, 0);
						if(ExecView(dlg) == cmOK)
							dlg->getCtrlData(CTLSEL_GOODSWOG_WOG, &Data.Rec.WrOffGrpID);
					}
					delete dlg;
				}
				break;
			case cmGoodsFilt:
				if(gpk == gpkndAltGroup) {
					GoodsFilt flt;
					if(GoodsFilterDialog(Data.P_Filt ? Data.P_Filt : &flt) > 0) {
						if(!SETIFZ(Data.P_Filt, new GoodsFilt(flt)))
							PPError(PPERR_NOMEM, 0);
					}
				}
				break;
			case cmGoodsValueAddedData:
				{
					getCtrlData(CTL_GOODS_NAME, Data.Rec.Name);
					GoodsVadDialog * dlg = new GoodsVadDialog;
					if(CheckDialogPtrErr(&dlg)) {
						dlg->setDTS(&Data);
						for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
							if(dlg->getDTS(&Data))
								valid_data = 1;
						}
						delete dlg;
					}
				}
				break;
			case cmGoodsStruc:
				if(gpk == gpkndGoods) {
					int    disable_confirm = 0;
					SETIFZ(Data.GS.GoodsID, Data.Rec.ID);
					for(int valid_data = 0; !valid_data && GObj.GSObj.EditDialog(&Data.GS) > 0;) {
						valid_data = 1;
						if(Data.GS.IsNamed()) {
							if(disable_confirm || PPMessage(mfConf|mfYes|mfNo, PPCFM_MODIFYNAMEDGS) == cmYes) {
								disable_confirm = 1;
								if(!GObj.GSObj.Put(&Data.GS.Rec.ID, &Data.GS, 1))
									valid_data = PPErrorZ();
							}
						}
					}
				}
				break;
			case cmGoodsEditTax:
				if(oneof2(gpk, gpkndGoods, gpkndOrdinaryGroup)) {
					PPID   tax_id = getCtrlLong(CTLSEL_GOODS_TAX);
					if(tax_id)
						GObj.GTxObj.Edit(&tax_id, 0);
				}
				break;
			case cmGoodsExt:
				getDTS(0);
				GObj.EditClsdGoods(&Data, 1);
				setCtrlData(CTL_GOODS_NAME,    Data.Rec.Name);
				setCtrlData(CTL_GOODS_ABBR,    Data.Rec.Abbr);
				setCtrlData(CTLSEL_GOODS_UNIT, &Data.Rec.UnitID);
				setCtrlData(CTLSEL_GOODS_PHUNIT, &Data.Rec.PhUnitID);
				setCtrlData(CTL_GOODS_PHUPERU, &Data.Rec.PhUPerU);
				break;
			case cmGoodsExTitles:
				EditGoodsExTitles(Data.ExTitles);
				break;
			case cmEditObjAssoc:
				if(Data.Rec.ID && resourceID == DLG_GOODS)
					GObj.ShowGoodsAsscInfo(Data.Rec.ID);
				break;
			case cmViewQuots:
				if(Data.Rec.ID)
					GObj.EditQuotations(Data.Rec.ID, 0, -1L, 0, PPQuot::clsGeneral);
				break;
			case cmGoodsMatrix:
				if(Data.Rec.ID)
					GObj.EditQuotations(Data.Rec.ID, 0, -1L, 0, PPQuot::clsMtx);
				break;
			case cmSupplCost:
				if(Data.Rec.ID)
					GObj.EditQuotations(Data.Rec.ID, 0, -1L, 0, PPQuot::clsSupplDeal);
				break;
			case cmTags:
				Data.TagL.Oid.Obj = PPOBJ_GOODS;
				EditObjTagValList(&Data.TagL, 0);
				break;
			case cmSearchUHTT:
				{
					SString name_buf;
					SString barcode_buf;
					getCtrlString(CTL_GOODS_NAME, name_buf);
					getCtrlString(CTL_GOODS_BARCODE, barcode_buf);
					UhttGoodsPacket ugp;
					if(GObj.SearchUhttInteractive(name_buf, barcode_buf, &ugp) > 0) {
						getCtrlString(CTL_GOODS_NAME, name_buf);
						if(!name_buf.NotEmptyS())
							setCtrlString(CTL_GOODS_NAME, name_buf = ugp.Name);
						getCtrlString(CTL_GOODS_BARCODE, barcode_buf);
						if(!barcode_buf.NotEmptyS())
							setCtrlString(CTL_GOODS_BARCODE, barcode_buf = ugp.SingleBarcode);
					}
				}
				break;
			case cmGoodsStat:
				if(Data.Rec.ID) {
					OpGroupingFilt filt;
					filt.GoodsID = Data.Rec.ID;
                    filt.Period.FromStr("^2..", 0);
                    filt.Flags |= OpGroupingFilt::fCalcRest;
                    PPView::Execute(PPVIEW_OPGROUPING, &filt, 0, 0);
				}
				break;
			default:
				return;
		}
	}
	else if(TVKEYDOWN)
		if(TVKEY == kbF2 && isCurrCtlID(CTL_GOODS_BARCODE) && !P_Current->IsInState(sfDisabled)) {
			SString code;
			if(GObj.SelectBarcode(Data.Rec.Kind, getCtrlLong(CTLSEL_GOODS_GROUP), code) > 0)
				setCtrlString(CTL_GOODS_BARCODE, code);
		}
		else if(TVKEY == kbF7 || TVCHR == kbCtrlL)
			printLabel();
		else if(TVCHR == kbCtrlB)
			editBarcodeList();
		else if(TVKEY == kbAltF7)
			printInfoLabel();
		else
			return;
	else
		return;
	clearEvent(event);
}
//
// GoodsCtrlGroup
//
GoodsCtrlGroup::Rec::Rec(PPID grpID, PPID goodsID, PPID locID, uint flags) : GoodsGrpID(grpID), GoodsID(goodsID), LocID(locID), ArID(0), Flags(flags)
{
}

GoodsCtrlGroup::GoodsCtrlGroup(uint _ctlsel_grp, uint _ctlsel_goods) : CtrlGroup(), P_Filt(0),
	CtlselGrp(_ctlsel_grp), CtlselGoods(_ctlsel_goods), CtlGrp(0), CtlGoods(0), Flags(0), LocID(0), TempAltGrpID(0), ArID(0)
{
}

GoodsCtrlGroup::~GoodsCtrlGroup()
{
	delete P_Filt;
	PPObjGoodsGroup::RemoveDynamicAlt(TempAltGrpID, 0, 1, 1);
}

void GoodsCtrlGroup::SetupCtrls(TDialog * dlg)
{
	if(!CtlGrp && !CtlGoods) {
		CtlGrp   = GetComboBoxLinkID(dlg, CtlselGrp);
		CtlGoods = GetComboBoxLinkID(dlg, CtlselGoods);
	}
}

int GoodsCtrlGroup::setFilt(TDialog * pDlg, const GoodsFilt * pFilt)
{
	int    ok = 1;
	if(pFilt && !pFilt->IsEmpty()) {
		if(P_Filt && P_Filt->IsEq(pFilt, 0)) {
			ok = -1;
		}
		else {
			THROW_MEM(SETIFZ(P_Filt, new GoodsFilt));
			*P_Filt = *pFilt;
			{
				PPTransaction tra(1);
				THROW(tra);
				THROW(PPObjGoodsGroup::RemoveDynamicAlt(TempAltGrpID, 0, 1, 0));
				THROW(PPObjGoodsGroup::AddDynamicAltGroupByFilt(P_Filt, &TempAltGrpID, 0, 0));
				THROW(tra.Commit());
			}
			ok = 1;
		}
	}
	else {
		if(P_Filt == 0)
			ok = -1;
		else {
			ZDELETE(P_Filt);
			THROW(PPObjGoodsGroup::RemoveDynamicAlt(TempAltGrpID, 0, 1, 1));
			ok = 1;
		}
	}
	if(ok > 0) {
		Rec rec;
		getData(pDlg, &rec);
		rec.GoodsGrpID = TempAltGrpID;
		setData(pDlg, &rec);
	}
	CATCHZOK
	return ok;
}

int GoodsCtrlGroup::setData(TDialog * dlg, void * pData)
{
	const  PPID save_loc_id = LConfig.Location;
	Rec  * p_rec = static_cast<Rec *>(pData);
	PPID   grp_id = 0;
	PPID   prev_grp_level = 0;
	int    disable_group_selection = 0;
	PPObjGoods gobj;
	Flags = p_rec->Flags;
	LocID = p_rec->LocID;
	ArID  = p_rec->ArID;
	if(LocID)
		DS.SetLocation(LocID); // Текущий склад используется для определения списка товаров, которые есть на остатке //
	if(p_rec->GoodsGrpID)
		grp_id = labs(p_rec->GoodsGrpID);
	else if(p_rec->GoodsID) {
		Goods2Tbl::Rec goods_rec;
		if(gobj.Fetch(p_rec->GoodsID, &goods_rec) > 0) {
			if(goods_rec.Kind == PPGDSK_GROUP) {
				p_rec->GoodsGrpID = p_rec->GoodsID;
				p_rec->GoodsID = 0;
			}
			else
				p_rec->GoodsGrpID = goods_rec.ParentID;
		}
		else
			p_rec->GoodsID = 0;
		grp_id = p_rec->GoodsGrpID;
	}
	if(!grp_id && !(Flags & ignoreRtOnlyGroup)) {
		PPAccessRestriction accsr;
		if(ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID) {
			Goods2Tbl::Rec gg_rec;
			if(gobj.Fetch(accsr.OnlyGoodsGrpID, &gg_rec) > 0 && gg_rec.Kind == PPGDSK_GROUP) {
				grp_id = p_rec->GoodsGrpID = accsr.OnlyGoodsGrpID;
				if(accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp)
					disable_group_selection = 1;
			}
		}
	}
	long   fl = 0; // @v9.4.0 OLW_LOADDEFONOPEN-->0
	gobj.GetParentID(grp_id, &prev_grp_level);
	if(Flags & enableInsertGoods)
		fl |= OLW_CANINSERT;
	fl |= OLW_WORDSELECTOR;
	SetupPPObjCombo(dlg, CtlselGrp, PPOBJ_GOODSGROUP, p_rec->GoodsGrpID, ((Flags & enableSelUpLevel) ? (fl|OLW_CANSELUPLEVEL) : fl), reinterpret_cast<void *>(prev_grp_level));
	if(disable_group_selection)
		dlg->disableCtrl(CtlselGrp, true);
	{
		const  PPID ext_id = (Flags & existsGoodsOnly) ? (p_rec->GoodsGrpID ? -labs(p_rec->GoodsGrpID) : LONG_MIN) : labs(p_rec->GoodsGrpID);
		fl = OLW_LOADDEFONOPEN;
		if(Flags & enableInsertGoods)
			fl |= OLW_CANINSERT;
		SetupPPObjCombo(dlg, CtlselGoods, PPOBJ_GOODS, p_rec->GoodsID, fl, reinterpret_cast<void *>(ext_id));
	}
	DS.SetLocation(save_loc_id);
	return 1;
}

int GoodsCtrlGroup::getData(TDialog * dlg, void * pData)
{
	Rec * p_rec = static_cast<Rec *>(pData);
	dlg->getCtrlData(CtlselGrp,   &p_rec->GoodsGrpID);
	dlg->getCtrlData(CtlselGoods, &p_rec->GoodsID);
	return (p_rec->GoodsID == 0 && Flags & disableEmptyGoods) ? PPSetError(PPERR_GOODSNEEDED) : 1;
}

int GoodsCtrlGroup::setFlag(TDialog * dlg, long f, int on)
{
	int    ok = -1;
	if(oneof2(f, disableEmptyGoods, existsGoodsOnly)) {
		if((on && !(Flags & f)) || !on && Flags & f) {
			if(f == existsGoodsOnly) {
				setFlagExistsOnly(dlg, on);
			}
			else {
				SETFLAG(Flags, f, on);
			}
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int GoodsCtrlGroup::setFlagExistsOnly(TDialog * dlg, int on)
{
	if((on && !(Flags & existsGoodsOnly)) || !on && Flags & existsGoodsOnly) {
		SETFLAG(Flags, existsGoodsOnly, on);
		PPID   grp_id = 0, goods_id = 0;
		dlg->getCtrlData(CtlselGrp, &grp_id);
		dlg->getCtrlData(CtlselGoods, &goods_id);
		if(Flags & existsGoodsOnly)
			grp_id = grp_id ? -labs(grp_id) : LONG_MIN;
		else
			grp_id = labs(grp_id);
		long   fl = OLW_LOADDEFONOPEN;
		if(Flags & enableInsertGoods)
			fl |= OLW_CANINSERT;
		SetupPPObjCombo(dlg, CtlselGoods, PPOBJ_GOODS, goods_id, fl, reinterpret_cast<void *>(grp_id));
		return !on;
	}
	return on;
}

static bool isComboCurrent(TDialog * dlg, TView * cb)
{
	return (cb && (dlg->IsCurrentView(cb) || dlg->IsCurrentView(static_cast<ComboBox *>(cb)->GetLink())) && !cb->IsInState(sfDisabled));
}

void GoodsCtrlGroup::handleEvent(TDialog * dlg, TEvent & event)
{
	PPID   grp;
	if(TVCOMMAND) {
		if(event.isCbSelected(CtlselGrp)) {
			dlg->getCtrlData(CtlselGrp, &grp);
			if(Flags & existsGoodsOnly)
				grp = grp ? -grp : LONG_MIN;
			long   fl = OLW_LOADDEFONOPEN;
			if(Flags & enableInsertGoods)
				fl |= OLW_CANINSERT;
			SetupPPObjCombo(dlg, CtlselGoods, PPOBJ_GOODS, 0, fl, reinterpret_cast<void *>(grp));
			TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselGoods));
			if(Flags & activateGoodsListOnGroupSelection) {
				SetupCtrls(dlg);
				dlg->selectCtrl(CtlGoods);
				dlg->messageToCtrl(CtlselGoods, cmCBActivate, 0);
			}
			dlg->clearEvent(event);
		}
	}
	else if(TVKEYDOWN && TVKEY == kbF2 && dlg->GetCurrentView()) {
		SetupCtrls(dlg);
		if(isComboCurrent(dlg, dlg->getCtrlView(CtlGrp)) || isComboCurrent(dlg, dlg->getCtrlView(CtlGoods))) {
			PPObjGoods gobj;
			Goods2Tbl::Rec rec;
			if(gobj.SelectGoodsByBarcode(0, ArID, &rec, 0, 0) > 0) {
				if(rec.Kind == PPGDSK_GOODS) {
					{
						ComboBox * p_combo = static_cast<ComboBox *>(dlg->getCtrlView(CtlselGrp));
						if(p_combo) {
							TView::messageCommand(p_combo->getListWindow(), cmLBLoadDef);
							dlg->setCtrlLong(CtlselGrp, rec.ParentID);
						}
					}
					long   fl = OLW_LOADDEFONOPEN;
					if(Flags & enableInsertGoods)
						fl |= OLW_CANINSERT;
					SetupPPObjCombo(dlg, CtlselGoods, PPOBJ_GOODS, rec.ID, fl, reinterpret_cast<void *>(rec.ParentID));
					TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselGoods));
				}
				else if(rec.Kind == PPGDSK_GROUP) {
					PPID prev_grp_id = dlg->getCtrlLong(CtlselGrp);
					if(prev_grp_id != rec.ID) {
						dlg->setCtrlLong(CtlselGrp, rec.ID);
						long   fl = OLW_LOADDEFONOPEN;
						if(Flags & enableInsertGoods)
							fl |= OLW_CANINSERT;
						SetupPPObjCombo(dlg, CtlselGoods, PPOBJ_GOODS, 0, fl, reinterpret_cast<void *>(rec.ID));
						TView::messageCommand(dlg, cmCBSelected, dlg->getCtrlView(CtlselGoods));
					}
				}
			}
			dlg->clearEvent(event);
		}
	}
}
//
// Объединение товаров
//
class ReplGoodsDialog : public TDialog {
	DECL_DIALOG_DATA(PPObjGoods::ExtUniteBlock);
	enum {
		ctlgroupGoods1 = 1,
		ctlgroupGoods2 = 2,
	};
public:
	enum {
		sfCommon = 0,
		sfEgais,
		//sfAllToOne // @v9.8.6 Объединение всех товаров из выборки в один
	};
	enum {
		selfByClass   = 0x0001,
		selfByVolume  = 0x0002,
		selfByAlcKind = 0x0004,
		selfByManuf   = 0x0008,
		selfBySuppl   = 0x0010
	};
	ReplGoodsDialog(int specialForm) : TDialog((specialForm == sfEgais) ? DLG_REPLGOODSALC : DLG_REPLGOODS), SelectionFlags(0), SpecialForm(specialForm)
	{
		if(specialForm == sfEgais)
			PrcssrAlcReport::ReadConfig(&AlcrCfg);
		// @v11.3.2 @obsolete setCtrlOption(CTL_REPLGOODS_PANE1, ofFramed, 1);
		// @v11.3.2 @obsolete setCtrlOption(CTL_REPLGOODS_PANE2, ofFramed, 1);
		addGroup(ctlgroupGoods1, new GoodsCtrlGroup(CTLSEL_REPLGOODS_GRP1, CTLSEL_REPLGOODS_GOODS1));
		addGroup(ctlgroupGoods2, new GoodsCtrlGroup(CTLSEL_REPLGOODS_GRP2, CTLSEL_REPLGOODS_GOODS2));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		GoodsCtrlGroup::Rec src_rec(0, Data.ResultID);
		setGroupData(ctlgroupGoods1, &src_rec);
		Goods2Tbl::Rec goods_rec;
		PPID   grp_id = 0;
		if(GObj.Fetch(Data.ResultID, &goods_rec) > 0)
			grp_id = goods_rec.ParentID;
		GoodsCtrlGroup::Rec dest_rec(grp_id, Data.DestList.getSingle());
		setGroupData(ctlgroupGoods2, &dest_rec);
		if(SpecialForm == sfEgais) {
			AddClusterAssoc(CTL_REPLGOODS_FLTF, 0, selfByClass);
			AddClusterAssoc(CTL_REPLGOODS_FLTF, 1, selfByVolume);
			AddClusterAssoc(CTL_REPLGOODS_FLTF, 2, selfByAlcKind);
			AddClusterAssoc(CTL_REPLGOODS_FLTF, 3, selfByManuf);
			AddClusterAssoc(CTL_REPLGOODS_FLTF, 4, selfBySuppl);
			SetClusterData(CTL_REPLGOODS_FLTF, SelectionFlags);
			showCtrl(CTL_REPLGOODS_ALLDEST, false);
		}
		else {
			if(Data.DestList.getCount() > 1) {
				showCtrl(CTL_REPLGOODS_ALLDEST, true);
				/* не следует по-умолчанию ставить эту галку - она опасная
				setCtrlUInt16(CTL_REPLGOODS_ALLDEST, 1);
				disableCtrl(CTLSEL_REPLGOODS_GRP2, true);
				disableCtrl(CTLSEL_REPLGOODS_GOODS2, true);
				enableCommand(cmExchange, 0);
				*/
			}
			else
				showCtrl(CTL_REPLGOODS_ALLDEST, false);
		}
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		PPID   dest_id;
		GoodsCtrlGroup::Rec rec;
		THROW(getGroupData(ctlgroupGoods1, &rec));
		Data.ResultID = rec.GoodsID;
		{
			uint16 v = getCtrlUInt16(CTL_REPLGOODS_ALLDEST);
			SETFLAG(Data.Flags, Data.fAllToOne, (v & 1));
			THROW_PP(Data.ResultID != 0, PPERR_REPLZEROOBJ);
		}
		if(!(Data.Flags & Data.fAllToOne)) {
			THROW(getGroupData(ctlgroupGoods2, &rec));
			dest_id = rec.GoodsID;
			THROW_PP(dest_id != 0 && Data.ResultID != 0, PPERR_REPLZEROOBJ);
			THROW_PP(dest_id != Data.ResultID, PPERR_REPLSAMEOBJ);
			Data.DestList.clear();
			Data.DestList.add(dest_id);
		}
		ASSIGN_PTR(pData, Data);
		CATCH
			selectCtrl(Data.ResultID == 0 ? CTL_REPLGOODS_GOODS1 : CTL_REPLGOODS_GOODS2);
			ok = 0;
		ENDCATCH
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_REPLGOODS_GOODS1)) {
			PPID   goods_id = getCtrlLong(CTLSEL_REPLGOODS_GOODS1);
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(goods_id, &goods_rec) > 0 && !getCtrlLong(CTLSEL_REPLGOODS_GRP2))
				setCtrlData(CTLSEL_REPLGOODS_GRP2, &goods_rec.ParentID);
		}
		else if(event.isClusterClk(CTL_REPLGOODS_ALLDEST)) {
			if(Data.DestList.getCount() > 1) {
				uint16 v = getCtrlUInt16(CTL_REPLGOODS_ALLDEST);
				disableCtrl(CTLSEL_REPLGOODS_GRP2, LOGIC(v & 1));
				disableCtrl(CTLSEL_REPLGOODS_GOODS2, LOGIC(v & 1));
				enableCommand(cmExchange, !(v & 1));
				{
					SString info_buf;
					if(v & 1) {
						info_buf.CatEq("count", Data.DestList.getCount());
					}
					setStaticText(CTL_REPLGOODS_ST_INFO, info_buf);
				}
			}
		}
		else if(event.isClusterClk(CTL_REPLGOODS_FLTF)) {
            long   f = GetClusterData(CTL_REPLGOODS_FLTF);
            GoodsCtrlGroup::Rec rec;
			if(getGroupData(ctlgroupGoods2, &rec)) {
				PPID   dest_id = rec.GoodsID;
				Goods2Tbl::Rec goods_rec;
				if(GObj.Fetch(dest_id, &goods_rec) > 0) {
					GoodsExtTbl::Rec goods_ext;
					if(GObj.P_Tbl->GetExt(dest_id, &goods_ext) <= 0)
						goods_ext.GoodsID = 0;
					GoodsFilt filt;
					PPGdsClsPacket gc_pack;
					PPObjGoodsClass gc_obj;
					if(goods_rec.GdsClsID && gc_obj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0) {
						if(f & selfByClass) {
							filt.Ep.GdsClsID = goods_rec.GdsClsID;
						}
						if(f & selfByAlcKind) {
							if(goods_ext.GoodsID && oneof4(AlcrCfg.CategoryClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW)) {
								double org_val = 0.0;
								gc_pack.GetExtDim(&goods_ext, AlcrCfg.CategoryClsDim, &org_val);
								if(org_val > 0.0)
									filt.Ep.SetDimRange(AlcrCfg.CategoryClsDim, org_val, org_val);
							}
						}
						if(f & selfByVolume) {
							if(goods_ext.GoodsID && oneof4(AlcrCfg.VolumeClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW)) {
								double org_val = 0.0;
								gc_pack.GetExtDim(&goods_ext, AlcrCfg.VolumeClsDim, &org_val);
								if(org_val > 0.0)
									filt.Ep.SetDimRange(AlcrCfg.VolumeClsDim, org_val, org_val);
							}
						}
					}
					if(f & selfByManuf && goods_rec.ManufID) {
						filt.ManufID = goods_rec.ManufID;
					}
					{
						GoodsCtrlGroup * p_grp = static_cast<GoodsCtrlGroup *>(getGroup(ctlgroupGoods1));
						p_grp->setFilt(this, &filt);
					}
				}
			}
		}
		else if(event.isCmd(cmLots1))
			viewLots(ctlgroupGoods1);
		else if(event.isCmd(cmLots2))
			viewLots(ctlgroupGoods2);
		else if(event.isCmd(cmExchange))
			Exchange();
		else
			return;
		clearEvent(event);
	}
	void   viewLots(int n)
	{
		GoodsCtrlGroup::Rec rec;
		getGroupData(n, &rec);
		if(rec.GoodsID)
			ViewLots(rec.GoodsID, 0, 0, 0, 0);
	}
	int    Exchange()
	{
		int    ok = 1;
		PPID   src_id = 0;
		PPID   dest_id = 0;
		{
			GoodsCtrlGroup::Rec rec;
			THROW(getGroupData(ctlgroupGoods1, &rec));
			src_id = rec.GoodsID;
			THROW(getGroupData(ctlgroupGoods2, &rec));
			dest_id = rec.GoodsID;
		}
		{
			GoodsCtrlGroup::Rec rec(0, dest_id);
			setGroupData(ctlgroupGoods1, &rec);
		}
		{
			GoodsCtrlGroup::Rec rec(0, src_id);
			setGroupData(ctlgroupGoods2, &rec);
		}
		CATCHZOKPPERR
		return ok;
	}
	long   SelectionFlags;
	int    SpecialForm;
	PrcssrAlcReport::Config AlcrCfg;
	PPObjGoods GObj;
};

PPObjGoods::ExtUniteBlock::ExtUniteBlock() : Flags(0), ResultID(0), DestList()
{
}

/*static*/int PPObjGoods::ReplaceGoods(/*PPID srcID, PPObjGoods::ExtUniteBlock * pEub*/ExtUniteBlock & rEub)
{
	int    ok = -1;
	PPIDArray total_dest_list;
	const  PPIDArray org_dest_list = rEub.DestList; // Диалог параметров объединения может затереть оригинальный список
	PPObjGoods goods_obj;
	ReplGoodsDialog * dlg = 0;
	int    special_form = ReplGoodsDialog::sfCommon;
	if(rEub.Flags & ExtUniteBlock::fUseSpcFormEgais)
		special_form = ReplGoodsDialog::sfEgais;
	THROW(goods_obj.CheckRights(GOODSRT_UNITE));
	THROW(CheckDialogPtr(&(dlg = new ReplGoodsDialog(special_form))));
	dlg->setDTS(/*dest_id, src_id*/&rEub);
	while(ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&rEub)) {
			int    local_ok = 1;
			const  PPID src_id = rEub.ResultID;
			const  int is_single = BIN(rEub.DestList.getCount() == 1);
			PPTransaction tra(1);
			THROW(tra);
			for(uint i = 0; local_ok && i < rEub.DestList.getCount(); i++) {
				PPID   dest_id = rEub.DestList.get(i);
				long   rof = 0;
				if(is_single)
					rof |= PPObject::user_request;
				if(dest_id != src_id) {
					if(PPObject::ReplaceObj(PPOBJ_GOODS, dest_id, src_id, rof/*PPObject::use_transaction|PPObject::user_request*/)) {
						total_dest_list.add(dest_id);
					}
					else {
						local_ok = 0;
					}
				}
			}
			if(local_ok) {
				THROW(tra.Commit());
				//
				dlg->setCtrlLong(CTLSEL_REPLGOODS_GOODS2, 0L);
				ok = 1;
				rEub.DestList.clear();
				rEub.ResultID = src_id;
				if(rEub.Flags & PPObjGoods::ExtUniteBlock::fOnce || rEub.DestList.getCount() > 1) {
					break;
				}
			}
			else {
				tra.Rollback();
				PPError();
			}
		}
	}
	CATCHZOKPPERR
	rEub.DestList = total_dest_list;
	delete dlg;
	return ok;
}
//
// Ассоциации товаров
//
class GoodsAsscDialog : public PPListDialog {
public:
	GoodsAsscDialog(PPID goodsID, PPObjGoods * pObj) : PPListDialog(DLG_GDSASSCINFO, CTL_GDSASSCINFO_LIST), GoodsID(goodsID), LastItemId(0), P_GObj(pObj)
	{
		SString name;
		setCtrlData(CTL_GDSASSCINFO_ID, &GoodsID);
		GetGoodsName(goodsID, name);
		setCtrlString(CTL_GDSASSCINFO_GOODS, name);
		updateList(-1);
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int editItem(long pos, long id);
	virtual int addItem(long * pPos, long * pID); // AHTOXA
	int    setupAssoc(PPID asscType, uint asscText, PPID objType);
	int    EditPLU(PPID goodsGrpID, long * pPLU);

	PPID   GoodsID;
	long   LastItemId;
	PPObjGoods * P_GObj;
	LAssocArray AsscList;
};

int GoodsAsscDialog::EditPLU(PPID goodsGrpID, long * pPLU)
{
	class EditPLUDialog : public TDialog {
	public:
		EditPLUDialog(PPID goodsID, PPID goodsGrpID) : TDialog(DLG_EDITPLU), GoodsID(goodsID), GoodsGrpID(goodsGrpID)
		{
		}
		int    setDTS(const long * pPlu)
		{
			setCtrlLong(CTL_EDITPLU_PLU, DEREFPTRORZ(pPlu));
			return 1;
		}
		int    getDTS(long * pPLU)
		{
			int    ok = 1;
			ObjAssocTbl::Rec rec;
			long   plu = getCtrlLong(CTL_EDITPLU_PLU);
			THROW_PP(PPRef->Assc.SearchNum(PPASS_ALTGOODSGRP, GoodsGrpID, plu, &rec) <= 0 || rec.ScndObjID == GoodsID, PPERR_DUPLPLU);
			ASSIGN_PTR(pPLU, plu);
			CATCH
				ok = PPErrorByDialog(this, CTL_EDITPLU_PLU);
			ENDCATCH
			return ok;
		}
	private:
		PPID   GoodsID;
		PPID   GoodsGrpID;
	};
	DIALOG_PROC_BODY_P2(EditPLUDialog, GoodsID, goodsGrpID, pPLU);
}

IMPL_HANDLE_EVENT(GoodsAsscDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmEditPLU)) {
		if(SmartListBox::IsValidS(P_Box)) {
			Reference * p_ref = PPRef;
			const uint pos = P_Box->P_Def->_curItem();
			if(pos < AsscList.getCount()) {
				LAssoc item = AsscList.at(pos);
				if(item.Val && item.Key == PPASS_ALTGOODSGRP) {
					ObjAssocTbl::Rec rec;
					if(p_ref->Assc.Search(PPASS_ALTGOODSGRP, item.Val, GoodsID, &rec) > 0) {
						int  old_inner_num = rec.InnerNum;
						EditPLU(item.Val, &rec.InnerNum);
						// @todo Операцию изменения номера принадлежности товара группе перенести
						// в PPObjGoods и реализовать в одной транзакции с LogAction
						if(!p_ref->Assc.Update(rec.ID, &rec, 1))
							PPError();
						else {
							if(rec.InnerNum != old_inner_num)
								DS.LogAction(PPACN_CHNGPLUALTGRP, PPOBJ_GOODSGROUP, item.Val, rec.InnerNum, 1);
							updateList(-1);
						}
					}
				}
			}
		}
		clearEvent(event);
	}
}
// } AHTOXA

int GoodsAsscDialog::setupAssoc(PPID asscType, uint asscText, PPID objType)
{
	int    ok = 1;
	PPObjGoodsStruc gs_obj;
	SString name, assc_name;
	char   temp_buf[128];
	StringSet ss(SLBColumnDelim);
	ObjAssocTbl::Rec assc_rec;
	PPGetSubStr(PPTXT_GOODS_ASSC_NAME, asscText, assc_name);
	for(SEnum en = PPRef->Assc.Enum(asscType, GoodsID, 1); en.Next(&assc_rec) > 0;) {
		const  PPID assc_id = assc_rec.PrmrObjID;
		name.Z();
		if(asscType == PPASS_GOODSSTRUC) {
			PPID   goods_id = 0;
			PPGoodsStrucHeader gs_hdr;
			if(gs_obj.Fetch(assc_id, &gs_hdr) > 0) {
				if(gs_hdr.Flags & GSF_NAMED)
					name = gs_hdr.Name;
				else if(P_GObj->P_Tbl->SearchAnyRef(objType, assc_id, &goods_id) > 0)
					GetGoodsName(goods_id, name);
			}
		}
		else
			GetObjectName(objType, assc_id, name);
		ss.Z();
		ss.add(assc_name);
		ss.add(name);
		ss.add(ltoa(assc_rec.InnerNum, temp_buf, 10));
		THROW(addStringToList(++LastItemId, ss.getBuf()));
		THROW_SL(AsscList.Add(asscType, assc_id, 0));
	}
	CATCHZOK
	return ok;
}

int GoodsAsscDialog::setupList()
{
	int    ok = 1;
	LastItemId = 0;
	AsscList.freeAll();
	THROW(setupAssoc(PPASS_ALTGOODSGRP, PPGDSASSC_ALTGOODSGRP, PPOBJ_GOODSGROUP));
	THROW(setupAssoc(PPASS_GOODSSTRUC,  PPGDSASSC_GOODSSTRUC,  PPOBJ_GOODSSTRUC));
	THROW(setupAssoc(PPASS_GENGOODS,    PPGDSASSC_GENGOODS,    PPOBJ_GOODS));
	CATCHZOK
	return ok;
}

int GoodsAsscDialog::editItem(long pos, long /*id*/)
{
	if(pos >= 0 && pos < AsscList.getCountI()) {
		LAssoc & item = AsscList.at(static_cast<uint>(pos));
		if(item.Val) {
			int    r = -1;
			if(item.Key == PPASS_ALTGOODSGRP)
				r = P_GObj->Edit(&item.Val, gpkndUndef, 0, 0, 0);
			else if(item.Key == PPASS_GOODSSTRUC)
				r = P_GObj->GSObj.Edit(&item.Val, 0);
			else if(item.Key == PPASS_GENGOODS)
				r = P_GObj->Edit(&item.Val, 0);
			if(r == cmOK)
				return 1;
			else if(!r)
				return 0;
		}
	}
	return -1;
}

int GoodsAsscDialog::addItem(long * pPos, long * pID)
{
	int    r = -1;
	int    valid_data = 0;
	PPID   alt_grp = 0;
	while(!valid_data && ListBoxSelDialog::Run(PPOBJ_GOODSGROUP, &alt_grp, reinterpret_cast<void *>(GGRTYP_SEL_ALT)) > 0)
		if(P_GObj && P_GObj->IsAltGroup(alt_grp) > 0) {
			r = P_GObj->AssignGoodsToAltGrp(GoodsID, alt_grp, 0, 1);
			valid_data = 1;
		}
		else
			PPError(PPERR_NOTALTGRP);
	return r;
}

int PPObjGoods::ShowGoodsAsscInfo(PPID goodsID)
{
	int    ok = -1;
	if(goodsID) {
		GoodsAsscDialog * dlg = new GoodsAsscDialog(goodsID, this);
		if(CheckDialogPtrErr(&dlg)) {
			ExecViewAndDestroy(dlg);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}
//
// Диалог фильтра по товарам
//
class GoodsFiltDialog : public TDialog {
	DECL_DIALOG_DATA(GoodsFilt);
public:
	enum {
		ctlgroupBrand      = 1,
		ctlgroupBrandOwner = 2
	};
	GoodsFiltDialog() : TDialog(DLG_GOODSFLT)
	{
		addGroup(ctlgroupBrand, new BrandCtrlGroup(CTLSEL_GOODSFLT_BRAND, cmBrandList));
		addGroup(ctlgroupBrandOwner, new PersonListCtrlGroup(CTLSEL_GOODSFLT_BROWNER, 0, cmBrandOwnerList, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		SString temp_buf;
		BrandCtrlGroup::Rec brand_grp_rec(Data.BrandList.IsExists() ? &Data.BrandList.Get() : 0);
		PersonListCtrlGroup::Rec brandowner_grp_rec(PPPRK_MANUF, Data.BrandOwnerList.IsExists() ? &Data.BrandOwnerList.Get() : 0);
		setGroupData(ctlgroupBrand, &brand_grp_rec);
		setCtrlUInt16(CTL_GOODSFLT_NOBRAND, BIN(Data.Flags & GoodsFilt::fWoBrand));
		setGroupData(ctlgroupBrandOwner, &brandowner_grp_rec);
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_GRP,     PPOBJ_GOODSGROUP, Data.GrpID,       OLW_CANSELUPLEVEL|/*OLW_LOADDEFONOPEN|*/OLW_WORDSELECTOR); // @v11.1.10 OLW_WORDSELECTOR
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_MANUF,   PPOBJ_PERSON,     Data.ManufID,     OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPPRK_MANUF));
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_COUNTRY, PPOBJ_COUNTRY,    Data.ManufCountryID, OLW_LOADDEFONOPEN, 0);
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_UNIT,    PPOBJ_UNIT,       Data.UnitID,      0, 0);
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_PHUNIT,  PPOBJ_UNIT,       Data.PhUnitID,    0, 0);
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_KIND,    PPOBJ_GOODSTYPE,  Data.GoodsTypeID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_GOODSFLT_TAXGRP,  PPOBJ_GOODSTAX,   Data.TaxGrpID,    0, 0);
		setupVat();
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 0, GoodsFilt::fHidePassive);
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 1, GoodsFilt::fRestrictByMatrix);
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 2, GoodsFilt::fOutOfMatrix);
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 3, GoodsFilt::fHasImages);
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 4, GoodsFilt::fUseIndepWtOnly);
		AddClusterAssoc(CTL_GOODSFLT_FLAGS, 5, GoodsFilt::fHideGeneric);
		SetClusterData(CTL_GOODSFLT_FLAGS, Data.Flags);
		setCtrlUInt16(CTL_GOODSFLT_NOT, BIN(Data.Flags & GoodsFilt::fNegation));
		setCtrlUInt16(CTL_GOODSFLT_WOTAXGRP, BIN(Data.Flags & GoodsFilt::fWoTaxGrp));
		AddClusterAssoc(CTL_GOODSFLT_UNITF, 0, GoodsFilt::fIntUnitOnly);
		AddClusterAssoc(CTL_GOODSFLT_UNITF, 1, GoodsFilt::fFloatUnitOnly);
		SetClusterData(CTL_GOODSFLT_UNITF, Data.Flags);
		setCtrlUInt16(CTL_GOODSFLT_NOKIND, BIN(Data.Flags & GoodsFilt::fUndefType));
		if(Data.GrpIDList.IsExists()) {
			SetComboBoxListText(this, CTLSEL_GOODSFLT_GRP);
			disableCtrl(CTLSEL_GOODSFLT_GRP, true);
		}
		{
			Data.GetExtssData(Data.extssNameText, temp_buf.Z());
			setCtrlString(CTL_GOODSFLT_NAMESTR, temp_buf);
			SetupWordSelector(CTL_GOODSFLT_NAMESTR, new TextHistorySelExtra("goodsfilt-nametext-common"), 0, 2, WordSel_ExtraBlock::fFreeText);
		}
		{
			Data.GetExtssData(Data.extssBarcodeText, temp_buf.Z());
			setCtrlString(CTL_GOODSFLT_BARCODESTR, temp_buf);
			SetupWordSelector(CTL_GOODSFLT_BARCODESTR, new TextHistorySelExtra("goodsfilt-barcodetext-common"), 0, 2, WordSel_ExtraBlock::fFreeText);
		}
		setCtrlString(CTL_GOODSFLT_BCLEN, Data.BarcodeLen);
		if(Data.Flags & GoodsFilt::fNotUseViewOptions) {
			Data.Flags &= ~(GoodsFilt::fShowBarcode|GoodsFilt::fShowCargo|GoodsFilt::fShowStrucType);
			showCtrl(STDCTL_VIEWOPTBUTTON, false);
		}
		SetupStringCombo(this, CTLSEL_GOODSFLT_ORDER, PPTXT_GOODSORDER, Data.InitOrder);
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		SString temp_buf;
		getCtrlData(CTLSEL_GOODSFLT_GRP,    &Data.GrpID);
		getCtrlData(CTLSEL_GOODSFLT_MANUF,  &Data.ManufID);
		getCtrlData(CTLSEL_GOODSFLT_COUNTRY, &Data.ManufCountryID);
		getCtrlData(CTLSEL_GOODSFLT_UNIT,   &Data.UnitID);
		getCtrlData(CTLSEL_GOODSFLT_PHUNIT, &Data.PhUnitID);
		getCtrlData(CTLSEL_GOODSFLT_KIND,   &Data.GoodsTypeID);
		getCtrlData(CTLSEL_GOODSFLT_TAXGRP, &Data.TaxGrpID);
		GetClusterData(CTL_GOODSFLT_FLAGS,  &Data.Flags);
		SETFLAG(Data.Flags, GoodsFilt::fNegation, getCtrlUInt16(CTL_GOODSFLT_NOT));
		SETFLAG(Data.Flags, GoodsFilt::fWoTaxGrp, getCtrlUInt16(CTL_GOODSFLT_WOTAXGRP));
		GetClusterData(CTL_GOODSFLT_UNITF, &Data.Flags);
		SETFLAG(Data.Flags, GoodsFilt::fUndefType, getCtrlUInt16(CTL_GOODSFLT_NOKIND));
		{
			getCtrlString(CTL_GOODSFLT_NAMESTR, temp_buf);
			Data.PutExtssData(Data.extssNameText, temp_buf);
		}
		{
			getCtrlString(CTL_GOODSFLT_BARCODESTR, temp_buf);
			Data.PutExtssData(Data.extssBarcodeText, temp_buf);
		}
		getCtrlString(CTL_GOODSFLT_BCLEN, Data.BarcodeLen);
		{
			BrandCtrlGroup::Rec brand_grp_rec;
			getGroupData(ctlgroupBrand, &brand_grp_rec);
			Data.BrandList.Set(&brand_grp_rec.List);
			SETFLAG(Data.Flags, GoodsFilt::fWoBrand, getCtrlUInt16(CTL_GOODSFLT_NOBRAND));
		}
		{
			PersonListCtrlGroup::Rec brandowner_grp_rec;
			getGroupData(ctlgroupBrandOwner, &brandowner_grp_rec);
			Data.BrandOwnerList.Set(&brandowner_grp_rec.List);
		}
		getCtrlData(CTLSEL_GOODSFLT_ORDER, &Data.InitOrder);
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	int    editGoodsViewOptions();
	int    EditExtParams();
	int    editSysjFilt();
	void   GroupList();
	int    vatFilt();
	void   setupVat();
	void   SetupCtrls();
};
//
// Диалог дополнительных опций фильтра по товарам
//
int GoodsFilterAdvDialog(GoodsFilt * pFilt, int disable)
{
	class GoodsFiltAdvDialog : public TDialog {
		DECL_DIALOG_DATA(GoodsFilt);
	public:
		enum {
			ctlgroupLoc = 3
		};
		GoodsFiltAdvDialog(GoodsFilt * pF, int doDisable) : TDialog(DLG_GFLTADVOPT), CtlDisableSuppl(doDisable)
		{
			addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_GFLTADVOPT_LOC, 0, 0, cmLocList, 0, 0, 0));
			setDTS(pF);
			SetupCalPeriod(CTLCAL_GFLTADVOPT_LOTPERIOD, CTL_GFLTADVOPT_LOTPERIOD);
		}
		DECL_DIALOG_SETDTS()
		{
			ushort v = 0;
			RVALUEPTR(Data, pData);
			SetPeriodInput(this, CTL_GFLTADVOPT_LOTPERIOD, Data.LotPeriod);
			{
				LocationCtrlGroup::Rec l_rec(&Data.LocList);
				setGroupData(ctlgroupLoc, &l_rec);
			}
			if(CtlDisableSuppl)
				disableCtrl(CTLSEL_GFLTADVOPT_SUPPL, true);
			else
				SetupArCombo(this, CTLSEL_GFLTADVOPT_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
			AddClusterAssoc(CTL_GFLTADVOPT_FLAGS, 0, GoodsFilt::fNewLots);
			AddClusterAssoc(CTL_GFLTADVOPT_FLAGS, 1, GoodsFilt::fNoZeroRestOnLotPeriod);
			AddClusterAssoc(CTL_GFLTADVOPT_FLAGS, 2, GoodsFilt::fActualOnly);
			SetClusterData(CTL_GFLTADVOPT_FLAGS, Data.Flags);
			if(Data.Flags & GoodsFilt::fPassiveOnly)
				v = 1;
			else if(Data.Flags & GoodsFilt::fGenGoodsOnly)
				v = 2;
			else if(Data.Flags & GoodsFilt::fWOTaxGdsOnly)
				v = 3;
			else if(Data.Flags & GoodsFilt::fNoDisOnly)
				v = 4;
			else
				v = 0;
			setCtrlData(CTL_GFLTADVOPT_EXTFLT, &v);
			// @v11.6.2 {
			AddClusterAssoc(CTL_GFLTADVOPT_SNEXP, 0, GoodsFilt::f2SoonExpiredOnly); 
			SetClusterData(CTL_GFLTADVOPT_SNEXP, Data.Flags2);
			setCtrlData(CTL_GFLTADVOPT_AHEXPD, &Data.AheadExpiryDays);
			// } @v11.6.2 
			// @v11.8.3 {
			SetIntRangeInput(this, CTL_GFLTADVOPT_IDRNG, &Data.IdRange);
			SetIntRangeInput(this, CTL_GFLTADVOPT_GRPCRNG, &Data.GrpCountRange);
			// } @v11.8.3 
			SetupCtrls();
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 0;
			ushort v = 0;
			GetClusterData(CTL_GFLTADVOPT_FLAGS, &Data.Flags);
			if(!GetPeriodInput(this, CTL_GFLTADVOPT_LOTPERIOD, &Data.LotPeriod))
				PPErrorByDialog(this, CTL_GFLTADVOPT_LOTPERIOD);
			else if((Data.Flags & (GoodsFilt::fNewLots | GoodsFilt::fNoZeroRestOnLotPeriod)) && !Data.LotPeriod.low)
				PPErrorByDialog(this, CTL_GFLTADVOPT_PLISTFNEW, PPERR_CHKWPER);
			else {
				if(!CtlDisableSuppl)
					getCtrlData(CTLSEL_GFLTADVOPT_SUPPL, &Data.SupplID);
				{
					LocationCtrlGroup::Rec l_rec;
					getGroupData(ctlgroupLoc, &l_rec);
					Data.LocList = l_rec.LocList;
				}
				getCtrlData(CTL_GFLTADVOPT_EXTFLT, &(v = 0));
				SETFLAG(Data.Flags, GoodsFilt::fPassiveOnly,  v == 1);
				SETFLAG(Data.Flags, GoodsFilt::fGenGoodsOnly, v == 2);
				SETFLAG(Data.Flags, GoodsFilt::fWOTaxGdsOnly, v == 3);
				SETFLAG(Data.Flags, GoodsFilt::fNoDisOnly,    v == 4);
				if(Data.Flags & GoodsFilt::fPassiveOnly)
					Data.Flags &= ~GoodsFilt::fHidePassive;
				if(Data.Flags & GoodsFilt::fGenGoodsOnly)
					Data.Flags &= ~GoodsFilt::fHideGeneric;
				// @v11.6.2 {
				GetClusterData(CTL_GFLTADVOPT_SNEXP, &Data.Flags2);
				getCtrlData(CTL_GFLTADVOPT_AHEXPD, &Data.AheadExpiryDays);
				// } @v11.6.2 
				// @v11.8.3 {
				GetIntRangeInput(this, CTL_GFLTADVOPT_IDRNG, &Data.IdRange);
				GetIntRangeInput(this, CTL_GFLTADVOPT_GRPCRNG, &Data.GrpCountRange);
				// } @v11.8.3 
				ASSIGN_PTR(pData, Data);
				ok = 1;
			}
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_GFLTADVOPT_PLISTFNEW) || event.isClusterClk(CTL_GFLTADVOPT_PLUSNREST) || event.isClusterClk(CTL_GFLTADVOPT_SNEXP)) {
				SetupCtrls();
				clearEvent(event);
			}
		}
		void   SetupCtrls()
		{
			GetClusterData(CTL_GFLTADVOPT_FLAGS, &Data.Flags);
			DisableClusterItem(CTL_GFLTADVOPT_FLAGS, 1, Data.Flags & GoodsFilt::fNewLots);
			DisableClusterItem(CTL_GFLTADVOPT_FLAGS, 0, Data.Flags & GoodsFilt::fNoZeroRestOnLotPeriod);
			// @v11.6.2 {
			GetClusterData(CTL_GFLTADVOPT_SNEXP, &Data.Flags2);
			disableCtrl(CTL_GFLTADVOPT_AHEXPD, !(Data.Flags2 & GoodsFilt::f2SoonExpiredOnly)); 
			// } @v11.6.2 
		}
		int	   CtlDisableSuppl;
	};
	int    ok = -1;
	GoodsFiltAdvDialog * dlg = new GoodsFiltAdvDialog(pFilt, disable);
	if(CheckDialogPtrErr(&dlg)) {
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(pFilt))
			   	ok = valid_data = 1;
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int GoodsFiltDialog::vatFilt()
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GOODSFLTVAT);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->SetupCalDate(CTLCAL_GOODSFLTVAT_DATE, CTL_GOODSFLTVAT_DATE);
		double rate = fdiv100i(Data.VatRate);
		dlg->setCtrlData(CTL_GOODSFLTVAT_RATE, &rate);
		dlg->setCtrlData(CTL_GOODSFLTVAT_DATE, &Data.VatDate);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTL_GOODSFLTVAT_RATE, &rate);
			Data.VatRate = (long)(rate * 100.0);
			dlg->getCtrlData(CTL_GOODSFLTVAT_DATE, &Data.VatDate);
			setupVat();
			ok = valid_data = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int GoodsFiltDialog::editSysjFilt()
{
	SysJournalFilt sj_filt;
	RVALUEPTR(sj_filt, Data.P_SjF);
	sj_filt.ObjType = PPOBJ_GOODS;
	if(EditSysjFilt2(&sj_filt) > 0) {
		SETIFZ(Data.P_SjF, new SysJournalFilt);
		ASSIGN_PTR(Data.P_SjF, sj_filt);
	}
	if(Data.P_SjF) {
		//
		// Функция SysJournalFilt::IsEmpty считает фильтр, в котором установлен ObjType не пустым. В данном случае это - не верно.
		//
		Data.P_SjF->ObjType = 0;
		if(Data.P_SjF->IsEmpty()) {
			ZDELETE(Data.P_SjF);
		}
		else
			Data.P_SjF->ObjType = PPOBJ_GOODS;
	}
	return 1;
}

IMPL_HANDLE_EVENT(GoodsFiltDialog)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND) {
		switch(TVCMD) {
			case cmGrpList:      GroupList();            break;
			case cmViewOptions:  editGoodsViewOptions(); break;
			case cmGoodsFiltVat: vatFilt();              break;
			case cmExtParams:    EditExtParams();        break;
			case cmSysjFilt2:    editSysjFilt();         break;
			case cmAdvOptions:
				GetClusterData(CTL_GOODSFLT_FLAGS, &Data.Flags);
				if(GoodsFilterAdvDialog(&Data, 0) > 0)
					SetClusterData(CTL_GOODSFLT_FLAGS, Data.Flags);
				break;
			case cmClusterClk:
				{
					const uint ctl_id = event.getCtlID();
					if(oneof4(ctl_id, CTL_GOODSFLT_NOKIND, CTL_GOODSFLT_FLAGS, CTL_GOODSFLT_WOTAXGRP, CTL_GOODSFLT_NOBRAND))
						SetupCtrls();
					else
						return;
				}
				break;
			case cmTags:
				if(SETIFZ(Data.P_TagF, new TagFilt)) {
					if(!EditTagFilt(PPOBJ_GOODS, Data.P_TagF))
						PPError();
					if(Data.P_TagF->IsEmpty())
						ZDELETE(Data.P_TagF);
				}
				else
					PPError(PPERR_NOMEM);
				break;
			default:
				return;
		}
		clearEvent(event);
	}
}

class GoodsAdvOptDialog : public TDialog {
	DECL_DIALOG_DATA(GoodsFilt);
public:
	GoodsAdvOptDialog() : TDialog(DLG_GDSFVOPT), AcsID(0)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		SetShowFlags();
		if(!(CConfig.Flags & CCFLG_USEARGOODSCODE)) {
			DisableClusterItem(CTL_GDSFVOPT_FLAGS, 4, 1);
			disableCtrls(1, CTL_GDSFVOPT_OWNCODES, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
			Data.Flags &= ~GoodsFilt::fShowOwnArCode;
		}
		else {
			ArticleTbl::Rec ar_rec;
			if(Data.CodeArID && ArObj.Fetch(Data.CodeArID, &ar_rec) > 0) {
				AcsID = ar_rec.AccSheetID;
				disableCtrl(CTLSEL_GDSFVOPT_ACS, true);
			}
			else
				AcsID = GetSupplAccSheet();
			SetupPPObjCombo(this, CTLSEL_GDSFVOPT_ACS, PPOBJ_ACCSHEET, AcsID, 0, 0);
			SetupArCombo(this, CTLSEL_GDSFVOPT_AR, Data.CodeArID, OLW_LOADDEFONOPEN, AcsID, sacfDisableIfZeroSheet);
			AddClusterAssoc(CTL_GDSFVOPT_OWNCODES, 0, GoodsFilt::fShowOwnArCode);
			AddClusterAssoc(CTL_GDSFVOPT_OWNCODES, 1, GoodsFilt::fShowWoArCode);
			SetClusterData(CTL_GDSFVOPT_OWNCODES, Data.Flags);
		}
		// @v11.5.9 {
		{
			StrAssocArray assc_list;
			PPObjNamedObjAssoc::MakeGoodsToWarehouseAssocList(assc_list);
			SetupStrAssocCombo(this, CTLSEL_GDSFVOPT_ASSOC, assc_list, Data.GoodsLocAssocID, 0);
		}
		// } @v11.5.9 
		SetupCtrls();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		GetShowFlags();
		if(Data.Flags & GoodsFilt::fShowArCode)
			GetClusterData(CTL_GDSFVOPT_OWNCODES, &Data.Flags);
		else
			Data.Flags &= ~GoodsFilt::fShowOwnArCode;
		Data.CodeArID = (Data.Flags & GoodsFilt::fShowArCode) ? getCtrlLong(CTLSEL_GDSFVOPT_AR) : 0;
		Data.GoodsLocAssocID = (Data.Flags2 & GoodsFilt::f2ShowWhPlace) ? getCtrlLong(CTLSEL_GDSFVOPT_ASSOC) : 0; // @v11.5.9
		if(Data.Flags & GoodsFilt::fShowOwnArCode)
			Data.CodeArID = 0;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_GDSFVOPT_FLAGS))
			SetupCtrls();
		else if(event.isClusterClk(CTL_GDSFVOPT_FASSOC))
			SetupCtrls();
		else if(event.isClusterClk(CTL_GDSFVOPT_OWNCODES)) {
			if(getCtrlUInt16(CTL_GDSFVOPT_OWNCODES) & 0x01) {
				setCtrlLong(CTLSEL_GDSFVOPT_AR, 0);
				disableCtrls(1, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
			}
			else
				disableCtrls(0, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
		}
		else if(event.isCbSelected(CTLSEL_GDSFVOPT_ACS)) {
			if(!Data.CodeArID) {
				AcsID = getCtrlLong(CTLSEL_GDSFVOPT_ACS);
				SetupArCombo(this, CTLSEL_GDSFVOPT_AR, 0L, OLW_LOADDEFONOPEN, AcsID, sacfDisableIfZeroSheet);
			}
			else if(AcsID != getCtrlLong(CTLSEL_GDSFVOPT_ACS))
				setCtrlLong(CTLSEL_GDSFVOPT_ACS, AcsID);
		}
		else
			return;
		clearEvent(event);
	}
	void   SetShowFlags()
	{
		// В кластер CTL_GDSFVOPT_FLAGS начиная с релиза @v11.5.8 передаются флаги из полей Flags и Flags2
		// В связи с этим управление кластером чуть усложняется.
		long   f_ = 0;
		SETFLAG(f_, 0x0001, Data.Flags & GoodsFilt::fShowBarcode);
		SETFLAG(f_, 0x0002, Data.Flags & GoodsFilt::fShowCargo);
		SETFLAG(f_, 0x0004, Data.Flags & GoodsFilt::fShowStrucType);
		SETFLAG(f_, 0x0008, Data.Flags & GoodsFilt::fShowGoodsWOStruc);
		SETFLAG(f_, 0x0010, Data.Flags & GoodsFilt::fShowArCode);
		// @v11.5.9 SETFLAG(f_, 0x0020, Data.Flags2 & GoodsFilt::f2ShowWhPlace);
		AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 0, /*GoodsFilt::fShowBarcode*/0x0001);
		AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 1, /*GoodsFilt::fShowCargo*/0x0002);
		AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 2, /*GoodsFilt::fShowStrucType*/0x0004);
		AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 3, /*GoodsFilt::fShowGoodsWOStruc*/0x0008);
		AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 4, /*GoodsFilt::fShowArCode*/0x0010);
		// @v11.5.9 AddClusterAssoc(CTL_GDSFVOPT_FLAGS, 5, /*GoodsFilt::f2ShowWhPlace*/0x0020);
		SetClusterData(CTL_GDSFVOPT_FLAGS, /*Data.Flags*/f_);
		// @v11.5.9  {
		AddClusterAssoc(CTL_GDSFVOPT_FASSOC, 0, GoodsFilt::f2ShowWhPlace);
		SetClusterData(CTL_GDSFVOPT_FASSOC, Data.Flags2);
		// } @v11.5.9 
	}
	void   GetShowFlags()
	{
		long   f_ = GetClusterData(CTL_GDSFVOPT_FLAGS);
		SETFLAG(Data.Flags, GoodsFilt::fShowBarcode, f_ & 0x0001);
		SETFLAG(Data.Flags, GoodsFilt::fShowCargo, f_ & 0x0002);
		SETFLAG(Data.Flags, GoodsFilt::fShowStrucType, f_ & 0x0004);
		SETFLAG(Data.Flags, GoodsFilt::fShowGoodsWOStruc, f_ & 0x0008);
		SETFLAG(Data.Flags, GoodsFilt::fShowArCode, f_ & 0x0010);
		// @v11.5.9 SETFLAG(Data.Flags2, GoodsFilt::f2ShowWhPlace, f_ & 0x0020);
		GetClusterData(CTL_GDSFVOPT_FASSOC, &Data.Flags2); // @v11.5.9
	}
	void   SetupCtrls()
	{
		int    disable_struc = 0;
		int    disable_cargo = 0;
		GetShowFlags();
		if(!(Data.Flags & GoodsFilt::fShowStrucType))
			Data.Flags &= ~(GoodsFilt::fShowGoodsWOStruc);
		SetShowFlags();
		disableCtrl(CTLSEL_GDSFVOPT_ASSOC, LOGIC(!(Data.Flags2 & GoodsFilt::f2ShowWhPlace))); // @v11.5.9 
		DisableClusterItem(CTL_GDSFVOPT_FLAGS, 1, disable_cargo);
		DisableClusterItem(CTL_GDSFVOPT_FLAGS, 2, disable_struc);
		DisableClusterItem(CTL_GDSFVOPT_FLAGS, 3, /*!disable_cargo ||*/ disable_struc);
		if(Data.Flags & GoodsFilt::fShowArCode) {
			disableCtrl(CTL_GDSFVOPT_OWNCODES, false);
			disableCtrl(CTLSEL_GDSFVOPT_AR, false);
			disableCtrl(CTLSEL_GDSFVOPT_ACS, LOGIC(getCtrlLong(CTLSEL_GDSFVOPT_AR)));
			if(getCtrlUInt16(CTL_GDSFVOPT_OWNCODES) & 0x01) {
				setCtrlLong(CTLSEL_GDSFVOPT_AR, 0);
				disableCtrls(1, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
			}
			else
				disableCtrls(0, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
		}
		else
			disableCtrls(1, CTL_GDSFVOPT_OWNCODES, CTLSEL_GDSFVOPT_ACS, CTLSEL_GDSFVOPT_AR, 0);
	}
	PPID   AcsID;        // PPOBJ_ACCSHEET
	PPObjArticle ArObj;
};

int GoodsFiltDialog::editGoodsViewOptions() { DIALOG_PROC_BODY(GoodsAdvOptDialog, &Data); }

class EditExtParamsDlg : public TDialog {
	DECL_DIALOG_DATA(ClsdGoodsFilt);
public:
	EditExtParamsDlg() : TDialog(DLG_GFEXTOPT)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		GCObj.GetPacket(pData->GdsClsID, &GcPack);
		setTitle(GcPack.Rec.Name);
		setupGdsClsProp(CTLSEL_GFEXTOPT_KIND,     CTL_GFEXTOPT_L_KIND,     PPGdsCls::fUsePropKind,  &GcPack.PropKind,  Data.KindList);
		setupGdsClsProp(CTLSEL_GFEXTOPT_ADDPROP,  CTL_GFEXTOPT_L_ADDPROP,  PPGdsCls::fUsePropAdd,   &GcPack.PropAdd,   Data.AddObjList);
		setupGdsClsProp(CTLSEL_GFEXTOPT_GRADE,    CTL_GFEXTOPT_L_GRADE,    PPGdsCls::fUsePropGrade, &GcPack.PropGrade, Data.GradeList);
		setupGdsClsProp(CTLSEL_GFEXTOPT_ADD2PROP, CTL_GFEXTOPT_L_ADD2PROP, PPGdsCls::fUsePropAdd2,  &GcPack.PropAdd2,  Data.AddObj2List);
		setupGdsClsDim(CTL_GFEXTOPT_DIMX, CTL_GFEXTOPT_L_DIMX, PPGdsCls::fUseDimX, GcPack.DimX, Data.DimX_Rng);
		setupGdsClsDim(CTL_GFEXTOPT_DIMY, CTL_GFEXTOPT_L_DIMY, PPGdsCls::fUseDimY, GcPack.DimY, Data.DimY_Rng);
		setupGdsClsDim(CTL_GFEXTOPT_DIMZ, CTL_GFEXTOPT_L_DIMZ, PPGdsCls::fUseDimZ, GcPack.DimZ, Data.DimZ_Rng);
		setupGdsClsDim(CTL_GFEXTOPT_DIMW, CTL_GFEXTOPT_L_DIMW, PPGdsCls::fUseDimW, GcPack.DimW, Data.DimW_Rng);
		SetupPPObjCombo(this, CTLSEL_GFEXTOPT_GDSCLS, PPOBJ_GOODSCLASS, Data.GdsClsID, 0, 0);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getRange(CTL_GFEXTOPT_DIMX, &Data.DimX_Rng, GcPack.DimX.Scale);
		getRange(CTL_GFEXTOPT_DIMY, &Data.DimY_Rng, GcPack.DimY.Scale);
		getRange(CTL_GFEXTOPT_DIMZ, &Data.DimZ_Rng, GcPack.DimZ.Scale);
		getRange(CTL_GFEXTOPT_DIMW, &Data.DimW_Rng, GcPack.DimW.Scale);
		if(GcPack.Rec.Flags & PPGdsCls::fUsePropKind)
			Data.KindList = getCtrlLong(CTLSEL_GFEXTOPT_KIND);
		if(GcPack.Rec.Flags & PPGdsCls::fUsePropAdd)
			Data.AddObjList = getCtrlLong(CTLSEL_GFEXTOPT_ADDPROP);
		if(GcPack.Rec.Flags & PPGdsCls::fUsePropGrade)
			Data.GradeList = getCtrlLong(CTLSEL_GFEXTOPT_GRADE);
		if(GcPack.Rec.Flags & PPGdsCls::fUsePropAdd2)
			Data.AddObj2List = getCtrlLong(CTLSEL_GFEXTOPT_ADD2PROP);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   setupGdsClsProp(uint selID, uint labelID, uint flag, const PPGdsClsProp * pProp, const ObjIdListFilt & rList);
	void   setupGdsClsDim(uint inpID, uint labelID, uint flag, const PPGdsClsDim & rDim, const RealRange & rV);
	void   getRange(uint ctlID, RealRange * pRng, long scale);
	PPGdsClsPacket GcPack;
	PPObjGoodsClass GCObj;
};

IMPL_HANDLE_EVENT(EditExtParamsDlg)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_GFEXTOPT_GDSCLS)) {
		getCtrlData(CTLSEL_GFEXTOPT_GDSCLS, &GcPack.Rec.ID);
		if(Data.GdsClsID != GcPack.Rec.ID) {
			MEMSZERO(Data);
			Data.GdsClsID = GcPack.Rec.ID;
			setDTS(&Data);
		}
		clearEvent(event);
	}
}

void EditExtParamsDlg::setupGdsClsProp(uint selID, uint labelID, uint flag, const PPGdsClsProp * pProp, const ObjIdListFilt & rList)
{
	const char * p_name = (GcPack.Rec.Flags & flag) ? pProp->Name : 0;
	disableCtrls(p_name ? 0 : 1, selID, labelID, 0);
	setStaticText(labelID, p_name);
	if(p_name && pProp->ItemsListID)
		SetupPPObjCombo(this, selID, pProp->ItemsListID, rList.GetSingle(), OLW_CANINSERT, 0);
}

void EditExtParamsDlg::setupGdsClsDim(uint inpID, uint labelID, uint flag, const PPGdsClsDim & rDim, const RealRange & rV)
{
	const char * p_name = (GcPack.Rec.Flags & flag) ? rDim.Name : 0;
	disableCtrls(p_name ? 0 : 1, inpID, labelID, 0);
	setStaticText(labelID, p_name);
	if(p_name)
		SetRealRangeInput(this, inpID, rV.low, rV.upp, static_cast<int>(rDim.Scale));
}

void EditExtParamsDlg::getRange(uint ctlID, RealRange * pRng, long scale)
{
	GetRealRangeInput(this, ctlID, pRng);
	if(scale < 0) {
		const double _pw = fpow10i(scale);
		pRng->Set(pRng->low * _pw, pRng->upp * _pw);
	}
	else if(scale > 0)
		pRng->Set(round(pRng->low, scale), round(pRng->upp, scale));
}

int GoodsFiltDialog::EditExtParams() { DIALOG_PROC_BODY(EditExtParamsDlg, &Data.Ep); }

void GoodsFiltDialog::GroupList()
{
	getCtrlData(CTLSEL_GOODSFLT_GRP, &Data.GrpID);
	PPID   prev_grp_id = Data.GrpID;
	Data.GrpIDList.InitEmpty();
	Data.GrpIDList.Add(prev_grp_id);
	PPIDArray temp_list = Data.GrpIDList.Get();
	ListToListData data(PPOBJ_GOODSGROUP, reinterpret_cast<void *>(GGRTYP_SEL_NORMAL), &temp_list);
	data.Flags |= ListToListData::fIsTreeList;
	data.TitleStrID = PPTXT_SELGOODSGRPS;
	if(!ListToListDialog(&data))
		PPError();
	Data.GrpIDList.Set(&temp_list);
	Data.Setup();
	if(Data.GrpID != prev_grp_id)
		setCtrlData(CTLSEL_GOODSFLT_GRP, &Data.GrpID);
	if(Data.GrpIDList.GetCount() > 1)
		SetComboBoxListText(this, CTLSEL_GOODSFLT_GRP);
	disableCtrl(CTLSEL_GOODSFLT_GRP, (Data.GrpIDList.GetCount() > 1));
}

void GoodsFiltDialog::setupVat()
{
	SString temp_buf;
	if(Data.VatRate) {
		PPLoadStringS("vat", temp_buf).CatDiv('-', 1).Cat(fdiv100i(Data.VatRate), MKSFMTD(0, 2, NMBF_NOTRAILZ));
		if(Data.VatDate)
			temp_buf.CatDiv('-', 1).Cat(Data.VatDate, MKSFMT(0, DATF_DMY));
		SetComboBoxLinkText(this, CTLSEL_GOODSFLT_TAXGRP, temp_buf);
		disableCtrl(CTLSEL_GOODSFLT_TAXGRP, true);
	}
	else {
		SetComboBoxLinkText(this, CTLSEL_GOODSFLT_TAXGRP, temp_buf);
		disableCtrl(CTLSEL_GOODSFLT_TAXGRP, false);
		getCtrlData(CTLSEL_GOODSFLT_TAXGRP, &Data.TaxGrpID);
		setCtrlData(CTLSEL_GOODSFLT_TAXGRP, &Data.TaxGrpID);
	}
}

void GoodsFiltDialog::SetupCtrls()
{
	ushort v = getCtrlUInt16(CTL_GOODSFLT_NOKIND);
	if(!v) {
		disableCtrl(CTLSEL_GOODSFLT_KIND, false);
		setCtrlData(CTLSEL_GOODSFLT_KIND, &Data.GoodsTypeID);
	}
	else {
		setCtrlUInt16(CTLSEL_GOODSFLT_KIND, 0);
		disableCtrl(CTLSEL_GOODSFLT_KIND, true);
	}
	long prev_flags = Data.Flags;
	GetClusterData(CTL_GOODSFLT_FLAGS, &Data.Flags);
	long post_flags = Data.Flags;
	if(Data.Flags & GoodsFilt::fHidePassive)
		Data.Flags &= ~GoodsFilt::fPassiveOnly;
	if(Data.Flags & GoodsFilt::fHideGeneric)
		Data.Flags &= ~GoodsFilt::fGenGoodsOnly;
	if(Data.Flags & GoodsFilt::fRestrictByMatrix && !(prev_flags & GoodsFilt::fRestrictByMatrix))
		Data.Flags &= ~GoodsFilt::fOutOfMatrix;
	else if(Data.Flags & GoodsFilt::fOutOfMatrix && !(prev_flags & GoodsFilt::fOutOfMatrix))
		Data.Flags &= ~GoodsFilt::fRestrictByMatrix;
	//
	// Завершающая проверка, призванная исключить одновременную установку флагов
	// GoodsFilt::fRestrictByMatrix GoodsFilt::fOutOfMatrix
	// (флаг GoodsFilt::fRestrictByMatrix имеет приоритет).
	//
	if(Data.Flags & GoodsFilt::fRestrictByMatrix)
		Data.Flags &= ~GoodsFilt::fOutOfMatrix;
	if(post_flags != Data.Flags)
		SetClusterData(CTL_GOODSFLT_FLAGS, Data.Flags);
	v = getCtrlUInt16(CTL_GOODSFLT_WOTAXGRP);
	disableCtrl(CTLSEL_GOODSFLT_TAXGRP, v);
	enableCommand(cmGoodsFiltVat, !v);
	if(v) {
		setCtrlLong(CTLSEL_GOODSFLT_TAXGRP, 0);
		Data.VatDate = ZERODATE;
		Data.VatRate = 0;
	}
	v = getCtrlUInt16(CTL_GOODSFLT_NOBRAND);
	disableCtrls(BIN(v), CTLSEL_GOODSFLT_BRAND, CTLSEL_GOODSFLT_BROWNER, 0);
	enableCommand(cmBrandList, !BIN(v));
	enableCommand(cmBrandOwnerList, !BIN(v));
}

int GoodsFilterDialog(GoodsFilt * pFilt) { DIALOG_PROC_BODY(GoodsFiltDialog, pFilt); }
//
//
//
class ExportGoodsToGlbSvcDialog : public TDialog {
	DECL_DIALOG_DATA(PPObjGoods::ExportToGlbSvcParam);
public:
	explicit ExportGoodsToGlbSvcDialog(/*PPViewGoods * pView*/) : TDialog(DLG_UHTTEXPGOODS)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		AddClusterAssoc(CTL_UHTTEXPGOODS_GS, 0, PPGLS_UNIVERSEHTT);
		AddClusterAssoc(CTL_UHTTEXPGOODS_GS, 1, PPGLS_VK);
		AddClusterAssoc(CTL_UHTTEXPGOODS_GS, 2, PPGLS_UDS);
		SetClusterData(CTL_UHTTEXPGOODS_GS, Data.GlobalService);
		SetupPPObjCombo(this, CTLSEL_UHTTEXPGOODS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_SETUPSINGLE, reinterpret_cast<void *>(Data.GlobalService));
		AddClusterAssoc(CTL_UHTTEXPGOODS_CO, 0, DlgDataType::coGoodsGrpName);
		AddClusterAssoc(CTL_UHTTEXPGOODS_CO, 1, DlgDataType::coTag);
		SetClusterData(CTL_UHTTEXPGOODS_CO, Data.CategoryObject);
		ObjTagFilt tag_filt(PPOBJ_GOODS);
		SetupObjTagCombo(this, CTLSEL_UHTTEXPGOODS_COT, Data.CategoryTagID, 0, &tag_filt);
		disableCtrl(CTLSEL_UHTTEXPGOODS_COT, (Data.CategoryObject != DlgDataType::coTag));
		AddClusterAssoc(CTL_UHTTEXPGOODS_FLAGS, 0, DlgDataType::fExportPrice);
		AddClusterAssoc(CTL_UHTTEXPGOODS_FLAGS, 1, DlgDataType::fExportRest);
		AddClusterAssoc(CTL_UHTTEXPGOODS_FLAGS, 2, DlgDataType::fOnlyUnassocItems);
		AddClusterAssoc(CTL_UHTTEXPGOODS_FLAGS, 3, DlgDataType::fBlockAbsenceItems);
		SetClusterData(CTL_UHTTEXPGOODS_FLAGS, Data.Flags);

		AddClusterAssoc(CTL_UHTTEXPGOODS_DEXT, 0, GDSEXSTR_A);
		AddClusterAssoc(CTL_UHTTEXPGOODS_DEXT, 1, GDSEXSTR_B);
		AddClusterAssoc(CTL_UHTTEXPGOODS_DEXT, 2, GDSEXSTR_C);
		AddClusterAssoc(CTL_UHTTEXPGOODS_DEXT, 3, GDSEXSTR_D);
		AddClusterAssoc(CTL_UHTTEXPGOODS_DEXT, 4, GDSEXSTR_E);
		SetClusterData(CTL_UHTTEXPGOODS_DEXT, Data.DescrExtStrId);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		GetClusterData(CTL_UHTTEXPGOODS_GS, &Data.GlobalService);
		getCtrlData(CTLSEL_UHTTEXPGOODS_GUA, &Data.GuaID);
        GetClusterData(CTL_UHTTEXPGOODS_CO, &Data.CategoryObject);
        if(Data.CategoryObject == DlgDataType::coTag) {
            getCtrlData(sel = CTLSEL_UHTTEXPGOODS_COT, &Data.CategoryTagID);
            THROW_PP(Data.CategoryTagID, PPERR_TAGNEEDED);
        }
        else
			Data.CategoryTagID = 0;
		GetClusterData(CTL_UHTTEXPGOODS_FLAGS, &Data.Flags);
		GetClusterData(CTL_UHTTEXPGOODS_DEXT, &Data.DescrExtStrId);
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmCountUhttRefs)) {
			GetUhttRef();
		}
		else if(event.isClusterClk(CTL_UHTTEXPGOODS_CO)) {
			GetClusterData(CTL_UHTTEXPGOODS_CO, &Data.CategoryObject);
			disableCtrl(CTLSEL_UHTTEXPGOODS_COT, (Data.CategoryObject != PPObjGoods::ExportToGlbSvcParam::coTag));
		}
		else if(event.isClusterClk(CTL_UHTTEXPGOODS_GS)) {
			const long preserve_global_service = Data.GlobalService;
			GetClusterData(CTL_UHTTEXPGOODS_GS, &Data.GlobalService);
			if(Data.GlobalService != preserve_global_service) {
				Data.GuaID = 0;
				SetupPPObjCombo(this, CTLSEL_UHTTEXPGOODS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_SETUPSINGLE, reinterpret_cast<void *>(Data.GlobalService));
			}
		}
		else
			return;
		clearEvent(event);
	}
	int GetUhttRef()
	{
		int    ok = -1;
		/*
		int    r;
		SString msg_buf, fmt_buf;
		if(P_View) {
			GoodsViewItem item;
			LongArray uniq_id_list;
			LAssocArray list;
			StrAssocArray code_ref_list;
			PPWaitStart();
			for(P_View->InitIteration(P_View->OrdByDefault); P_View->NextIteration(&item) > 0;) {
				uniq_id_list.add(item.ID);
			}
			uniq_id_list.sortAndUndup();
			if(uniq_id_list.getCount()) {
				PPUhttClient uc;
				for(uint i = 0; i < uniq_id_list.getCount(); i++) {
					list.Add(uniq_id_list.get(i), 0, 0);
				}
				THROW(uc.Auth());
				THROW(r = uc.GetUhttGoodsRefList(list, &code_ref_list));
				list.Sort();
				{
					uint ref_count = 0;
					for(uint i = 0; i < list.getCount(); i++) {
						const  PPID goods_id = list.at(i).Key;
						const  PPID uhtt_id = list.at(i).Val;
						if(uhtt_id && (!i || list.at(i-1).Key != goods_id))
							ref_count++;
					}
					PPFormatT(PPTXT_UHTTEXPGOODS_REFS, &msg_buf, uniq_id_list.getCountI(), static_cast<long>(ref_count));
					setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
					ok = 1;
				}
			}
			else {
				PPLoadText(PPTXT_UHTTEXPGOODS_EMPTY, msg_buf);
				setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
			}
		}
		CATCH
			PPGetLastErrorMessage(1, msg_buf);
			setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
			ok = 0;
		ENDCATCH
		PPWaitStop();
		*/
		return ok;
	}
	//PPViewGoods * P_View; // @notowned
};

int PPObjGoods::EditExportToGlbSvcParam(ExportToGlbSvcParam * pData)
{
	DIALOG_PROC_BODY(ExportGoodsToGlbSvcDialog, pData);
}
