// V_GSTRUC.CPP
// Copyright (c) A.Starodub 2007, 2008, 2009, 2014, 2016, 2017, 2018
// @codepage UTF-8
// Таблица просмотра товарных структур
//
#include <pp.h>
#pragma hdrstop

//static 
int FASTCALL PPViewGoodsStruc::Cmp_ItemEntry(PPViewGoodsStruc * pView, int order, const void * i1, const void * i2)
{
	int    si = 0;
	if(pView && i1 && i2) {
		const ItemEntry * p1 = (const ItemEntry *)i1;
		const ItemEntry * p2 = (const ItemEntry *)i2;
		const uint slc = pView->StrucList.getCount();
		SString n1, n2;
		switch(order) {
			default: // @fallthrough
			case OrdByPrmrGoodsName:
				{
					if(p1->StrucEntryP < slc) {
						GetGoodsName(pView->StrucList.at(p1->StrucEntryP).PrmrGoodsID, n1);
					}
					if(p2->StrucEntryP < slc) {
						GetGoodsName(pView->StrucList.at(p2->StrucEntryP).PrmrGoodsID, n2);
					}
					si = n1.CmpNC(n2);
					SETIFZ(si, CMPSIGN(p1->ItemNo, p2->ItemNo));
				}
				break;
			case OrdByScndGoodsName:
				{
					GetGoodsName(p1->GoodsID, n1);
					GetGoodsName(p2->GoodsID, n2);
					si = n1.CmpNC(n2);
					if(!si) {
						n1.Z();
						n2.Z();
						if(p1->StrucEntryP < slc) {
							const StrucEntry & r_entry = pView->StrucList.at(p1->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n1);
						}
						if(p2->StrucEntryP < slc) {
							const StrucEntry & r_entry = pView->StrucList.at(p2->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n2);
						}
						si = n1.CmpNC(n2);
						SETIFZ(si, CMPSIGN(p1->ItemNo, p2->ItemNo));
					}
				}
				break;
			case OrdByStrucTypePrmrGoodsName:
				{
					const int k1 = PPGoodsStruc::GetStrucKind(p1->StrucFlags);
					const int k2 = PPGoodsStruc::GetStrucKind(p2->StrucFlags);
					si = CMPSIGN(k1, k2);
					if(!si) {
						if(p1->StrucEntryP < slc) {
							const StrucEntry & r_entry = pView->StrucList.at(p1->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n1);
						}
						if(p2->StrucEntryP < slc) {
							const StrucEntry & r_entry = pView->StrucList.at(p2->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n2);
						}
						si = n1.CmpNC(n2);
					}
				}
				break;
		}
	}
	return si;
}

static IMPL_CMPCFUNC(GoodsStrucView_ItemEntry_CurrentOrder, i1, i2)
{
	PPViewGoodsStruc * p_view = (PPViewGoodsStruc *)pExtraData;
	return p_view ? PPViewGoodsStruc::Cmp_ItemEntry(p_view, p_view->GetCurrentViewOrder(), i1, i2) : 0;
}

class GoodsStrucFiltDialog : public TDialog {
public:
	enum {
		ctlgroupGoodsFiltPrmr = 1,
		ctlgroupGoodsFiltScnd = 2,
	};
	GoodsStrucFiltDialog() : TDialog(DLG_GSFILT)
	{
		addGroup(ctlgroupGoodsFiltPrmr, new GoodsFiltCtrlGroup(CTLSEL_GSFILT_PGOODS, CTLSEL_GSFILT_PGGRP, cmPGoodsFilt));
		addGroup(ctlgroupGoodsFiltScnd, new GoodsFiltCtrlGroup(CTLSEL_GSFILT_SGOODS, CTLSEL_GSFILT_SGGRP, cmSGoodsFilt));
	}
	int    setDTS(const GoodsStrucFilt * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//
		{
			GoodsFiltCtrlGroup::Rec gf_rec(Data.PrmrGoodsGrpID, Data.PrmrGoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
			setGroupData(ctlgroupGoodsFiltPrmr, &gf_rec);
		}
		{
			GoodsFiltCtrlGroup::Rec gf_rec(Data.ScndGoodsGrpID, Data.ScndGoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
			setGroupData(ctlgroupGoodsFiltScnd, &gf_rec);
		}
		AddClusterAssocDef(CTL_GSFILT_ORDER, 0, PPViewGoodsStruc::OrdByDefault);
		AddClusterAssocDef(CTL_GSFILT_ORDER, 1, PPViewGoodsStruc::OrdByPrmrGoodsName);
		AddClusterAssocDef(CTL_GSFILT_ORDER, 2, PPViewGoodsStruc::OrdByScndGoodsName);
		AddClusterAssocDef(CTL_GSFILT_ORDER, 3, PPViewGoodsStruc::OrdByStrucTypePrmrGoodsName);
		SetClusterData(CTL_GSFILT_ORDER, Data.InitOrder);
		return ok;
	}
	int    getDTS(GoodsStrucFilt * pData)
	{
		int    ok = 1;
		{
			GoodsFiltCtrlGroup::Rec gf_rec;
			THROW(getGroupData(ctlgroupGoodsFiltPrmr, &gf_rec));
			Data.PrmrGoodsGrpID = gf_rec.GoodsGrpID;
			Data.PrmrGoodsID    = gf_rec.GoodsID;
		}
		{
			GoodsFiltCtrlGroup::Rec gf_rec;
			THROW(getGroupData(ctlgroupGoodsFiltScnd, &gf_rec));
			Data.ScndGoodsGrpID = gf_rec.GoodsGrpID;
			Data.ScndGoodsID    = gf_rec.GoodsID;
		}
		GetClusterData(CTL_GSFILT_ORDER, &Data.InitOrder);
		//
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = 0;
		ENDCATCH
		return ok;
	}
private:
	GoodsStrucFilt Data;
};

PPBaseFilt * SLAPI PPViewGoodsStruc::CreateFilt(void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_GOODSSTRUC, &p_base_filt))
		p_base_filt->Init(1, 0);
	else 
		PPSetError(PPERR_BASEFILTUNSUPPORTED);
	return p_base_filt;
}

int SLAPI PPViewGoodsStruc::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	GoodsStrucFilt * p_filt = (GoodsStrucFilt *)pBaseFilt;
	DIALOG_PROC_BODY(GoodsStrucFiltDialog, p_filt);
}

IMPLEMENT_PPFILT_FACTORY(GoodsStruc); SLAPI GoodsStrucFilt::GoodsStrucFilt() : PPBaseFilt(PPFILT_GOODSSTRUC, 0, 0)
{
	SetFlatChunk(offsetof(GoodsStrucFilt, ReserveStart),
		offsetof(GoodsStrucFilt, ReserveEnd) - offsetof(GoodsStrucFilt, ReserveStart));
	Init(1, 0);
}

SLAPI PPViewGoodsStruc::PPViewGoodsStruc() : PPView(0, &Filt, PPVIEW_GOODSSTRUC), CurrentViewOrder(OrdByDefault)
{
	DefReportId = REPORT_GOODSSTRUCLIST;
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewGoodsStruc::~PPViewGoodsStruc()
{
}

int SLAPI PPViewGoodsStruc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PPWait(1);
	StrucList.clear();
	ItemList.clear();
	StrPool.ClearS();
	CurrentViewOrder = (IterOrder)Filt.InitOrder;
	{
		Goods2Tbl::Rec grec;
		if(Filt.PrmrGoodsID) {
			if(GObj.Search(Filt.PrmrGoodsID, &grec) > 0) {
				THROW(AddItem(grec.ID, grec.StrucID, 0));
			}
		}
		else {
			GoodsFilt goods_filt;
			goods_filt.GrpIDList.Add(Filt.PrmrGoodsGrpID);
			goods_filt.Flags |= GoodsFilt::fWithStrucOnly;
			for(GoodsIterator gi(&goods_filt, 0); gi.Next(&grec) > 0;) {
				THROW(AddItem(grec.ID, grec.StrucID, 0));
				PPWaitPercent(gi.GetIterCounter());
			}
		}
	}
	ItemList.sort(PTR_CMPCFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsStruc::AddItem(PPID goodsID, PPID strucID, int checkExistance)
{
	int    ok = 1;
	PPGoodsStruc struc;
	if(checkExistance) {
		PPIDArray ex_goods_list; // Список товаров, с которыми была связана структура strucID в списках
		for(uint ex_spos = 0; StrucList.lsearch(&strucID, &ex_spos, CMPF_LONG, offsetof(StrucEntry, GStrucID)); ex_spos++) {
			const StrucEntry & r_entry = StrucList.at(ex_spos);
			uint  item_pos = ItemList.getCount();		
			if(item_pos) do {
				ItemEntry * p_item = &ItemList.at(--item_pos);
				if(p_item->StrucEntryP == ex_spos) {
					ex_goods_list.add(r_entry.PrmrGoodsID);
					ItemList.atFree(item_pos);
				}
			} while(item_pos);
		}
		{
			ex_goods_list.sortAndUndup();
			uint p = 0;
			if(ex_goods_list.bsearch(goodsID, &p))
				ex_goods_list.atFree(p);
			for(p = 0; p < ex_goods_list.getCount(); p++) {
				const PPID ex_goods_id = ex_goods_list.get(p);
				THROW(AddItem(ex_goods_id, strucID, 0)); // @recursion
			}
		}
	}
	if(GSObj.Get(strucID, &struc) > 0) {
		const  int  folder = BIN(struc.Rec.Flags & GSF_FOLDER);
		const  uint count  = folder ? struc.Childs.getCount() : struc.Items.getCount();
		if(count) {
			SString temp_buf;
			SString struc_name;
			uint    struc_entry_pplus1 = 0;
			for(uint i = 0; i < count; i++) {
				if(folder) {
					PPGoodsStruc * p_struc = struc.Childs.at(i);
					THROW(AddItem(goodsID, p_struc->Rec.ID, checkExistance)); // @recursion
				}
				else {
					const PPGoodsStrucItem & r_i = struc.Items.at(i);
					if((!Filt.ScndGoodsID || r_i.GoodsID == Filt.ScndGoodsID) && (!Filt.ScndGoodsGrpID || GObj.BelongToGroup(r_i.GoodsID, Filt.ScndGoodsGrpID, 0))) {
						if(!struc_entry_pplus1) {
							StrucEntry new_entry;
							MEMSZERO(new_entry);
							new_entry.PrmrGoodsID = goodsID;
							new_entry.GStrucID = struc.Rec.ID;
							new_entry.ParentStrucID = struc.Rec.ParentID;
							new_entry.Flags = struc.Rec.Flags;
							new_entry.CommDenom = struc.Rec.CommDenom;
							new_entry.GiftQuotKindID = struc.Rec.GiftQuotKindID;
							new_entry.Period = struc.Rec.Period;
							new_entry.GiftAmtRestrict = struc.Rec.GiftAmtRestrict;
							new_entry.GiftLimit = struc.Rec.GiftLimit;
							new_entry.VariedPropObjType = struc.Rec.VariedPropObjType;
							{
								if(struc.Rec.ParentID) {
									PPGoodsStrucHeader hdr_rec;
									if(GSObj.Fetch(struc.Rec.ParentID, &hdr_rec) > 0)
										struc_name = hdr_rec.Name;
									else
										ideqvalstr(struc.Rec.ParentID, struc_name.Z());
								}
								else
									struc_name = struc.Rec.Name;
								if(struc_name.Empty())
									ideqvalstr(struc.Rec.ID, struc_name).Quot('[', ']');
								StrPool.AddS(struc_name, &new_entry.NameP);
							}
							StrPool.AddS(struc.Rec.Symb, &new_entry.SymbP);
							THROW_SL(StrucList.insert(&new_entry));
							struc_entry_pplus1 = StrucList.getCount();
						}
						assert(struc_entry_pplus1 > 0 && struc_entry_pplus1 <= StrucList.getCount());
						ItemEntry new_item;
						MEMSZERO(new_item);
						new_item.StrucEntryP = struc_entry_pplus1-1;
						new_item.ItemNo   = i+1;
						new_item.GStrucID = struc.Rec.ID;
						new_item.GoodsID = r_i.GoodsID;
						new_item.Flags = r_i.Flags;
						new_item.StrucFlags = struc.Rec.Flags;
						new_item.Median = r_i.Median;
						new_item.Denom = r_i.Denom;
						new_item.Netto = r_i.Netto;
						THROW_SL(ItemList.insert(&new_item));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsStruc::InitIteration()
{
	Counter.Init(ItemList.getCount());
	IterIdx = 0;
	return 1;
}

int FASTCALL PPViewGoodsStruc::NextIteration(GoodsStrucViewItem * pItem)
{
	int    ok = -1;
	if(pItem && IterIdx < ItemList.getCount()) {
		memzero(pItem, sizeof(*pItem));
		const ItemEntry & r_item = ItemList.at(IterIdx);
		if(pItem) {
			pItem->GStrucID = r_item.GStrucID;
			if(r_item.StrucEntryP < StrucList.getCount()) {
				SString temp_buf;
				const StrucEntry & r_struc_entry = StrucList.at(r_item.StrucEntryP);
				pItem->PrmrGoodsID = r_struc_entry.PrmrGoodsID;
				pItem->CommDenom = r_struc_entry.CommDenom;
				pItem->GiftAmtRestrict = r_struc_entry.GiftAmtRestrict;
				pItem->GiftLimit = r_struc_entry.GiftLimit;
				pItem->GiftQuotKindID = r_struc_entry.GiftQuotKindID;
				pItem->ParentStrucID = r_struc_entry.ParentStrucID;
				pItem->Period = r_struc_entry.Period;
				pItem->StrucFlags = r_struc_entry.Flags;
				pItem->VariedPropObjType = r_struc_entry.VariedPropObjType;
				StrPool.GetS(r_struc_entry.NameP, temp_buf);
				STRNSCPY(pItem->StrucName, temp_buf);
				StrPool.GetS(r_struc_entry.SymbP, temp_buf);
				STRNSCPY(pItem->StrucSymb, temp_buf);
			}
			pItem->GoodsID = r_item.GoodsID;
			pItem->Median = r_item.Median;
			pItem->Denom = r_item.Denom;
			pItem->Netto = r_item.Netto;
			pItem->ItemFlags = r_item.Flags;
		}
		IterIdx++;
		Counter.Increment();
		ok = 1;
	}
	return ok;
}

int SLAPI FASTCALL PPViewGoodsStruc::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		const ItemEntry * p_item = (ItemEntry *)pBlk->P_SrcData;
		SString temp_buf;
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: // @id
				pBlk->Set(p_item->GStrucID);
				break;
			case 1: // Primary goods name
				if(p_item->StrucEntryP < StrucList.getCount()) {
					GetGoodsName(StrucList.at(p_item->StrucEntryP).PrmrGoodsID, temp_buf);
					pBlk->Set(temp_buf);
				}
				break;
			case 2: // Secondary goods name
				{
					GetGoodsName(p_item->GoodsID, temp_buf);
					pBlk->Set(temp_buf);
				}
				break;
			case 3: // Simple representation of component qtty per one primary ware item
				{
					double qtty = 0.0;
					PPGoodsStrucItem::GetEffectiveQuantity(1.0, p_item->GoodsID, p_item->Median, p_item->Denom, p_item->Flags, &qtty);
					temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOZERO));
					pBlk->Set(temp_buf);
				}
				break;
			case 4: // Text representation of flags
				if(p_item->StrucEntryP < StrucList.getCount()) {
					const StrucEntry & r_entry = StrucList.at(p_item->StrucEntryP);
					PPGoodsStruc::MakeTypeString(r_entry.GStrucID, r_entry.Flags, r_entry.ParentStrucID, temp_buf);
				}
				else
					temp_buf.Z();
				pBlk->Set(temp_buf);
				break;
			case 5: // Netto
				pBlk->Set(p_item->Netto);
				break;
			case 6: // Общий делитель структуры
				if(p_item->StrucEntryP < StrucList.getCount()) {
					const StrucEntry & r_entry = StrucList.at(p_item->StrucEntryP);
					pBlk->Set(r_entry.CommDenom);
				}
				else
					pBlk->Set(0.0);
				break;
			case 7: // Номер строки внутри стурктуры
				pBlk->Set(p_item->ItemNo);
				break;
		}
	}
	return ok;
}

// static
int PPViewGoodsStruc::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewGoodsStruc * p_v = (PPViewGoodsStruc *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw) {
		PPViewGoodsStruc * p_view = (PPViewGoodsStruc *)p_brw->P_View;
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewGoodsStruc::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < (long)p_def->getCount()) {
			const BroColumn & r_col = p_def->at(col);
			ItemEntry * p_item = (ItemEntry *)pData;
			if(r_col.OrgOffs == 4) { // type of struc
				SColor clr;
				switch(PPGoodsStruc::GetStrucKind(p_item->StrucFlags)) {
					case PPGoodsStruc::kBOM: clr = SClrGreen; break;
					case PPGoodsStruc::kPart: clr = SClrLightgreen; break;
					case PPGoodsStruc::kSubst: clr = SClrOrange; break;
					case PPGoodsStruc::kGift: clr = SClrPink; break;
					case PPGoodsStruc::kComplex: clr = SClrLightblue; break;
					default: clr = SClrGrey; break;
				}
				pStyle->RightFigColor = clr;
				pStyle->Flags |= BrowserWindow::CellStyle::fRightFigCircle;
				ok = 1;
			}
		}
	}
	return ok;
}

void SLAPI PPViewGoodsStruc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewGoodsStruc::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

SArray * SLAPI PPViewGoodsStruc::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(ItemList);
	uint   brw_id = BROWSER_GOODSSTRUC2;
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewGoodsStruc::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		GoodsStrucViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPIDArray uniq_struc_list;
		PPObjIDArray objid_ary;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			uniq_struc_list.addnz(item.GStrucID);
		}
		if(uniq_struc_list.getCount()) {
			uniq_struc_list.sortAndUndup();
			objid_ary.Add(PPOBJ_GOODSSTRUC, uniq_struc_list);
			THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsStruc::Recover()
{
	int    ok = 1;
	long   p = 0, t = 0;
	PPLogger logger;
	GoodsStrucViewItem item;
	PPIDArray uniq_struc_list;
	PPObjIDArray objid_ary;
	PPWait(1);
	for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
		uniq_struc_list.addnz(item.GStrucID);
	}
	if(uniq_struc_list.getCount()) {
		uniq_struc_list.sortAndUndup();
		for(uint i = 0; i < uniq_struc_list.getCount(); i++) {
			PPID   id = uniq_struc_list.get(i);
			THROW(GSObj.CheckStruc(id, &logger));
			PPWaitPercent(p, t);
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoodsStruc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   parent_struc_id = 0;
	ItemEntry brw_hdr;
	if(ok == -2) {
		if(pHdr)
			brw_hdr = *(ItemEntry *)pHdr;
		else
			MEMSZERO(brw_hdr);
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
			case PPVCMD_EDITGOODS:
			case PPVCMD_EDITITEMGOODS:
				if(brw_hdr.GStrucID && brw_hdr.StrucEntryP < StrucList.getCount()) {
					PPGoodsStruc gs;
					const StrucEntry & r_struc_entry = StrucList.at(brw_hdr.StrucEntryP);
					if(GSObj.Get(brw_hdr.GStrucID, &gs) > 0) {
						int r = 0;
						if(gs.Rec.ParentID != 0) {
							parent_struc_id = gs.Rec.ParentID;
							r = GSObj.Get(parent_struc_id, &gs);
						}
						else
							r = 1;
						gs.GoodsID = r_struc_entry.PrmrGoodsID;
						if(oneof2(ppvCmd, PPVCMD_EDITGOODS, PPVCMD_EDITITEMGOODS)) {
							PPID   goods_id = (ppvCmd == PPVCMD_EDITGOODS) ? r_struc_entry.PrmrGoodsID : brw_hdr.GoodsID;
							ok = (GObj.Edit(&goods_id, 0) == cmOK) ? 1 : -1;
						}
						else if(r > 0 && GSObj.EditDialog(&gs) > 0) {
							PPID   struc_id = parent_struc_id ? parent_struc_id : brw_hdr.GStrucID;
							int r = GSObj.Put(&struc_id, &gs, 1);
							if(r > 0) {
								AddItem(r_struc_entry.PrmrGoodsID, struc_id, 1);
								ItemList.sort(PTR_CMPCFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
								ok = 1;
							}
							else if(r == 0)
								ok = PPErrorZ();
						}
					}
				}
				break;
			case PPVCMD_SYSJ: // @v10.0.09
				if(brw_hdr.GStrucID) {
					ViewSysJournal(PPOBJ_GOODSSTRUC, brw_hdr.GStrucID, 0);
					ok = -1;
				}
				break;
			case PPVCMD_TRANSMIT: // @v10.0.09
				ok = -1;
				Transmit(0);
				break;
			case PPVCMD_DORECOVER: // @v10.0.09
				ok = -1;
				GSObj.CheckStructs();
				break;
			case PPVCMD_TOTAL:
				ok = ViewTotal();
				break;
		}
	}
	if(ok > 0) {
		if(ppvCmd == PPVCMD_EDITITEM) {
			AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
			if(p_def) {
				p_def->setArray(new SArray(ItemList), 0, 1);
				pBrw->search2(&brw_hdr.GStrucID, CMPF_LONG, srchFirst, 0);
			}
		}
		else if(oneof2(ppvCmd, PPVCMD_EDITGOODS, PPVCMD_EDITITEMGOODS)) {
			CALLPTRMEMB(pBrw, Update());
		}
	}
	return ok;
}

int SLAPI PPViewGoodsStruc::ViewTotal()
{
	int    ok = 1;
	long   goods_count = 0, lines_count = 0;
	PPID   prev_goods = 0;
	GoodsStrucViewItem item;
	TDialog * p_dlg = 0;
	THROW(CheckDialogPtr(&(p_dlg = new TDialog(DLG_TGSTRUC))));
	for(InitIteration(); NextIteration(&item) > 0;) {
		if(item.GoodsID != prev_goods) {
			goods_count++;
			prev_goods = item.GoodsID;
		}
		lines_count++;
		PPWaitPercent(Counter);
	}
	p_dlg->setCtrlLong(CTL_TGSTRUC_LINES, lines_count);
	p_dlg->setCtrlLong(CTL_TGSTRUC_GOODS, goods_count);
	ExecViewAndDestroy(p_dlg);
	CATCHZOKPPERR
	return ok;
}
//
// Implementation of PPALDD_GoodsStrucList
//
PPALDD_CONSTRUCTOR(GoodsStrucList)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsStrucList)
{
	Destroy();
}

//typedef GoodsFilt GoodsStrucFilt;

int PPALDD_GoodsStrucList::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsStruc, rsrv);
	H.FltFlags = p_filt->Flags;
	H.FltPrmrGroupID = p_filt->PrmrGoodsGrpID;
	H.FltPrmrGoodsID = p_filt->PrmrGoodsID;
	H.FltScndGroupID = p_filt->ScndGoodsGrpID;
	H.FltScndGoodsID = p_filt->ScndGoodsID;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsStrucList::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	INIT_PPVIEW_ALDD_ITER(GoodsStruc);
}

int PPALDD_GoodsStrucList::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsStruc); // PPViewGoodsStruc
	I.GsID = item.GStrucID;
	I.StrucFlags = item.StrucFlags;
	I.ParentID = item.ParentStrucID;
	I.GoodsID = item.PrmrGoodsID;
	I.CommDenom = item.CommDenom;
	I.GiftQuotKindID = item.GiftQuotKindID;
	I.GiftAmtRestrict = item.GiftAmtRestrict;
	I.GiftLimit = item.GiftLimit;
	
	I.ItemID = item.GoodsID;
	I.ItemFlags = item.ItemFlags;
	I.Median = item.Median;
	I.Denom = item.Denom;
	I.Netto = item.Netto;
	{
		SString & r_temp_buf = SLS.AcquireRvlStr();
		PPGoodsStrucItem::MakeEstimationString(item.Median, item.Denom, r_temp_buf, MKSFMTD(0, 3, ALIGN_RIGHT));
		STRNSCPY(I.SQtty, r_temp_buf);
	}
	/*
	STRNSCPY(I.Type,      item.Type);
	*/
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsStrucList::Destroy()
{
	DESTROY_PPVIEW_ALDD(GoodsStruc);
}
