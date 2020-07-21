// V_GSTRUC.CPP
// Copyright (c) A.Starodub 2007, 2008, 2009, 2014, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Таблица просмотра товарных структур
//
#include <pp.h>
#pragma hdrstop

//static 
int FASTCALL PPViewGoodsStruc::Cmp_ItemEntry(const PPViewGoodsStruc * pView, int order, const void * i1, const void * i2)
{
	int    si = 0;
	if(pView && i1 && i2) {
		const ItemEntry * p1 = static_cast<const ItemEntry *>(i1);
		const ItemEntry * p2 = static_cast<const ItemEntry *>(i2);
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
	PPViewGoodsStruc * p_view = static_cast<PPViewGoodsStruc *>(pExtraData);
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
		AddClusterAssoc(CTL_GSFILT_FLAGS, 0, GoodsStrucFilt::fShowUnrefs);
		AddClusterAssoc(CTL_GSFILT_FLAGS, 1, GoodsStrucFilt::fSkipByPassiveOwner); // @v10.3.2
		SetClusterData(CTL_GSFILT_FLAGS, Data.Flags);
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
		GetClusterData(CTL_GSFILT_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		CATCHZOK
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
	GoodsStrucFilt * p_filt = static_cast<GoodsStrucFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(GoodsStrucFiltDialog, p_filt);
}

IMPLEMENT_PPFILT_FACTORY(GoodsStruc); SLAPI GoodsStrucFilt::GoodsStrucFilt() : PPBaseFilt(PPFILT_GOODSSTRUC, 0, 0)
{
	SetFlatChunk(offsetof(GoodsStrucFilt, ReserveStart),
		offsetof(GoodsStrucFilt, ReserveEnd) - offsetof(GoodsStrucFilt, ReserveStart));
	Init(1, 0);
}

SLAPI PPViewGoodsStruc::PPViewGoodsStruc() : PPView(0, &Filt, PPVIEW_GOODSSTRUC), CurrentViewOrder(OrdByDefault), IterIdx(0), P_DsList__(0)
{
	DefReportId = REPORT_GOODSSTRUCLIST;
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewGoodsStruc::~PPViewGoodsStruc()
{
	ZDELETE(P_DsList__);
}

int SLAPI PPViewGoodsStruc::CmpSortIndexItems(PPViewBrowser * pBrw, const PPViewGoodsStruc::ItemEntry * pItem1, const PPViewGoodsStruc::ItemEntry * pItem2)
{
	return Implement_CmpSortIndexItems_OnArray(pBrw, pItem1, pItem2);
}

static IMPL_CMPFUNC(ViewGoodsStruc_ItemEntry, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewGoodsStruc * p_view = static_cast<PPViewGoodsStruc *>(p_brw->P_View);
		if(p_view) {
			const PPViewGoodsStruc::ItemEntry * p_item1 = static_cast<const PPViewGoodsStruc::ItemEntry *>(i1);
			const PPViewGoodsStruc::ItemEntry * p_item2 = static_cast<const PPViewGoodsStruc::ItemEntry *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int SLAPI PPViewGoodsStruc::SortList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount()); // @v10.7.5
	if(is_sorting_needed) {
		ItemList.sort(PTR_CMPFUNC(ViewGoodsStruc_ItemEntry), pBrw);
	}
	return ok;
}

int SLAPI PPViewGoodsStruc::MakeList(PPViewBrowser * pBrw)
{
	Problems.freeAll();
	StrucList.clear();
	ItemList.clear();
	StrPool.ClearS();
	int    ok = 1;
	// @v10.8.2 const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount()); // @v10.7.5
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
			if(!(Filt.Flags & Filt.fSkipByPassiveOwner) || !(grec.Flags & GF_PASSIV)) { // @v10.3.2
				THROW(AddItem(grec.ID, grec.StrucID, 0));
			}
			PPWaitPercent(gi.GetIterCounter());
		}
		if(!Filt.PrmrGoodsGrpID && !Filt.PrmrGoodsID && Filt.Flags & Filt.fShowUnrefs) {
			long   t = 0;
			uint   p = 0;
			PPIDArray owner_list;
			PPGoodsStrucHeader gsh;
			for(SEnum en = GSObj.Enum(0); en.Next(&gsh) > 0;)
				t++;
			for(SEnum en = GSObj.Enum(0); en.Next(&gsh) > 0; p++) {
				const PPID gs_id = gsh.ID;
				owner_list.clear();
				GObj.P_Tbl->SearchGListByStruc(gs_id, &owner_list);
				if(!owner_list.getCount() && !(gsh.Flags & GSF_CHILD)) {
					uint   ex_spos = 0;
					if(!StrucList.lsearch(&gs_id, &ex_spos, CMPF_LONG, offsetof(StrucEntry, GStrucID))) {
						THROW(AddItem(0, gs_id, 0));
					}
				}
				PPWaitPercent(p, t);
			}
		}
	}
	ItemList.sort(PTR_CMPCFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
	// @v10.7.5 {
	if(pBrw) {
		pBrw->Helper_SetAllColumnsSortable();
		SortList(pBrw);
	}
	// } @v10.7.5 
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoodsStruc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PPWait(1);
	CurrentViewOrder = static_cast<IterOrder>(Filt.InitOrder);
	THROW(MakeList(0));
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
		const ItemEntry * p_item = static_cast<const ItemEntry *>(pBlk->P_SrcData);
		SString temp_buf;
		int    r = 0;
		switch(pBlk->ColumnN) {
			case 0: pBlk->Set(p_item->GStrucID); break; // @id
			case 1: // Primary goods name
				if(p_item->StrucEntryP < StrucList.getCount())
					pBlk->Set(GetGoodsName(StrucList.at(p_item->StrucEntryP).PrmrGoodsID, temp_buf));
				break;
			case 2: pBlk->Set(GetGoodsName(p_item->GoodsID, temp_buf)); break; // Secondary goods name
			case 3: // Simple representation of component qtty per one primary ware item
				{
					double qtty = 0.0;
					PPGoodsStrucItem::GetEffectiveQuantity(1.0, p_item->GoodsID, p_item->Median, p_item->Denom, p_item->Flags, &qtty);
					pBlk->Set(temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOZERO)));
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
			case 5: pBlk->Set(p_item->Netto); break; // Netto
			case 6: // Общий делитель структуры
				if(p_item->StrucEntryP < StrucList.getCount()) {
					const StrucEntry & r_entry = StrucList.at(p_item->StrucEntryP);
					pBlk->Set(r_entry.CommDenom);
				}
				else
					pBlk->Set(0.0);
				break;
			case 7: pBlk->Set(p_item->ItemNo); break; // Номер строки внутри стурктуры
			case 8: // @v10.7.5 Наименование структуры
				{
					temp_buf.Z();
					PPGoodsStrucHeader rec;
					if(GSObj.Fetch(p_item->GStrucID, &rec) > 0)
						temp_buf = rec.Name;
					pBlk->Set(temp_buf); break;
				}
				break;
		}
	}
	return ok;
}

// static
int FASTCALL PPViewGoodsStruc::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewGoodsStruc * p_v = static_cast<PPViewGoodsStruc *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw) {
		PPViewGoodsStruc * p_view = static_cast<PPViewGoodsStruc *>(p_brw->P_View);
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewGoodsStruc::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			const ItemEntry * p_item = static_cast<const ItemEntry *>(pData);
			if(r_col.OrgOffs == 0) { // id
				if(Problems.getCount() && Problems.bsearch(&p_item->GStrucID, 0, CMPF_LONG))
					ok = pStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
			}
			else if(r_col.OrgOffs == 4) { // type of struc
				SColor clr;
				switch(PPGoodsStruc::GetStrucKind(p_item->StrucFlags)) {
					case PPGoodsStruc::kBOM: clr = GetColorRef(SClrGreen); break;
					case PPGoodsStruc::kPart: clr = GetColorRef(SClrLightgreen); break;
					case PPGoodsStruc::kSubst: clr = GetColorRef(SClrOrange); break;
					case PPGoodsStruc::kGift: clr = GetColorRef(SClrPink); break;
					case PPGoodsStruc::kComplex: clr = GetColorRef(SClrLightblue); break;
					default: clr = SClrGrey; break;
				}
				ok = pStyle->SetRightFigCircleColor(clr);
			}
			// @v10.4.6 {
			else if(r_col.OrgOffs == 6) { // common denomitator
				if(GObj.GetConfig().Flags & GCF_BANSTRUCCDONDECOMPL) {
					if(p_item->GStrucID && p_item->StrucEntryP < StrucList.getCount()) {
						const StrucEntry & r_struc_entry = StrucList.at(p_item->StrucEntryP);
						if((r_struc_entry.Flags & GSF_DECOMPL) && !(r_struc_entry.Flags & GSF_COMPL)) {
							if(r_struc_entry.CommDenom != 0.0 && r_struc_entry.CommDenom != 1.0)
								ok = pStyle->SetRightFigCircleColor(GetColorRef(SClrBrown));
						}
					}
				}
			}
			// } @v10.4.6 
		}
	}
	return ok;
}

void SLAPI PPViewGoodsStruc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewGoodsStruc::GetDataForBrowser, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		pBrw->Helper_SetAllColumnsSortable(); // @v10.7.5
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
	enum {
		cfCorrection   = 0x0001,
		cfSpcDetaching = 0x0002
	};
	int    ok = 1;
	PPLogger logger;
	SString log_fname;
	long   flags = 0;
	GoodsStrucViewItem item;
	PPIDArray uniq_struc_list;
	PPObjIDArray objid_ary;
	int   do_cancel = 1;
	{
		TDialog * dlg = new TDialog(DLG_CORGSTRUC);
		THROW(CheckDialogPtr(&dlg));
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_CORGSTRUC_LOG, CTL_CORGSTRUC_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		PPGetFileName(PPFILNAM_GSTRUCERR_LOG, log_fname);
		dlg->setCtrlString(CTL_CORGSTRUC_LOG, log_fname);
		dlg->AddClusterAssoc(CTL_CORGSTRUC_FLAGS, 0, cfCorrection);
		dlg->AddClusterAssoc(CTL_CORGSTRUC_FLAGS, 1, cfSpcDetaching);
		dlg->SetClusterData(CTL_CORGSTRUC_FLAGS, flags);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_CORGSTRUC_LOG, log_fname);
			dlg->GetClusterData(CTL_CORGSTRUC_FLAGS, &flags);
			do_cancel = 0;
		}
		delete dlg;
	}
	if(!do_cancel) {
		Problems.freeAll();
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			uniq_struc_list.addnz(item.GStrucID);
		}
		if(uniq_struc_list.getCount()) {
			uniq_struc_list.sortAndUndup();
			for(uint i = 0; i < uniq_struc_list.getCount(); i++) {
				PPID   id = uniq_struc_list.get(i);
				{
					PPGoodsStruc gs;
					PPIDArray struct_ids;
					PPIDArray goods_ids;
					GObj.P_Tbl->SearchGListByStruc(id, &goods_ids);
					THROW(GSObj.Get(id, &gs));
					THROW(GSObj.CheckStruct(&goods_ids, &struct_ids, &gs, &Problems, &logger));
				}
				PPWaitPercent(i+1, uniq_struc_list.getCount());
			}
			if(Problems.getCount())
				Problems.sort(CMPF_LONG); // Сортируем по идентификатору структуры для быстрого поиска
			if(flags & cfCorrection && (Problems.getCount() || flags & cfSpcDetaching)) {
				const char * p_surrogate_prefix = "$named-struct";
				PPIDArray owner_list;
				PPIDArray nna_gs_list;
				for(uint j = 0; j < Problems.getCount(); j++) {
					const PPObjGoodsStruc::CheckGsProblem * p_problem = Problems.at(j);
					if(p_problem->Code == PPObjGoodsStruc::CheckGsProblem::errNoNameAmbig)
						nna_gs_list.add(p_problem->GsID);
				}
				// @v10.0.12 {
				if(flags & cfSpcDetaching) {
					PPGoodsStrucHeader gs_rec;
					PPIDArray gs_id_list;
					const size_t spl = sstrlen(p_surrogate_prefix);
					for(SEnum en = GSObj.Enum(0); en.Next(&gs_rec) > 0;) {
						if(gs_rec.Flags & GSF_NAMED && strncmp(gs_rec.Name, p_surrogate_prefix, spl) == 0) {
							gs_id_list.add(gs_rec.ID);
						}
					}
					if(gs_id_list.getCount()) {
						PPTransaction tra(1);
						THROW(tra);
						gs_id_list.sortAndUndup();
						for(uint i = 0; i < gs_id_list.getCount(); i++) {
							const PPID gs_id = gs_id_list.get(i);
							owner_list.clear();
							GObj.P_Tbl->SearchGListByStruc(gs_id, &owner_list);
							if(owner_list.getCount()) {
								for(uint oidx = 0; oidx < owner_list.getCount(); oidx++) {
									PPID goods_id = owner_list.get(oidx);
									PPGoodsPacket goods_pack;
									THROW(GObj.GetPacket(goods_id, &goods_pack, 0) > 0);
									if(goods_pack.Rec.StrucID == gs_id) {
										goods_pack.Rec.StrucID = 0;
										goods_pack.GS.Rec.ID = 0;
										THROW(GObj.PutPacket(&goods_id, &goods_pack, 0));
									}
								}
							}
						}
						THROW(tra.Commit());
					}
				}
				// } @v10.0.12 
				if(nna_gs_list.getCount()) {
					nna_gs_list.sortAndUndup();
					SString surrogate_name;
					PPTransaction tra(1);
					THROW(tra);
					for(uint k = 0; k < nna_gs_list.getCount(); k++) {
						const PPID gs_id = nna_gs_list.get(k);
						{
							owner_list.clear();
							GObj.P_Tbl->SearchGListByStruc(gs_id, &owner_list);
							if(owner_list.getCount() > 1) {
								PPGoodsStrucHeader gsh;
								if(GSObj.Search(gs_id, &gsh) > 0) {
									if(!(gsh.Flags & GSF_NAMED) && !(gsh.Flags & GSF_CHILD)) {
										PPGoodsStruc gs;
										THROW(GSObj.Get(gs_id, &gs) > 0);
										{
											long   nn = 0;
											PPID   n_gs_id = 0;
											surrogate_name = p_surrogate_prefix;
											STRNSCPY(gs.Rec.Name, surrogate_name);
											while(GSObj.SearchByName(gs.Rec.Name, &n_gs_id, 0) > 0 && n_gs_id != gs_id) {
												(surrogate_name = p_surrogate_prefix).Space().Cat(++nn);
												STRNSCPY(gs.Rec.Name, surrogate_name);
											}
										}
										gs.Rec.Flags |= GSF_NAMED;
										PPID   temp_id = gs_id;
										THROW(GSObj.Put(&temp_id, &gs, 0));
										assert(temp_id == gs_id);
										//
										// Снимаем ссылку на структуру со всех товаров // (@v10.0.12 отменено) кроме последнего (с наибольшим идентификатором)
										//
										for(uint oidx = 0; oidx < owner_list.getCount(); oidx++) {
											PPID goods_id = owner_list.get(oidx);
											PPGoodsPacket goods_pack;
											THROW(GObj.GetPacket(goods_id, &goods_pack, 0) > 0);
											if(goods_pack.Rec.StrucID == gs_id) {
												goods_pack.Rec.StrucID = 0;
												goods_pack.GS.Rec.ID = 0;
												THROW(GObj.PutPacket(&goods_id, &goods_pack, 0));
											}
										}
									}
								}
								
							}
						}
					}
					THROW(tra.Commit());
				}
			}
			logger.Save(log_fname, 0);
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
			brw_hdr = *static_cast<const ItemEntry *>(pHdr);
		else
			MEMSZERO(brw_hdr);
		switch(ppvCmd) {
			case PPVCMD_USERSORT: // @v10.7.5
				SortList(pBrw);
				ok = 1; // The rest will be done below
				break;
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
			case PPVCMD_VIEWGOODSOPANLZ:
				if(brw_hdr.GStrucID && brw_hdr.StrucEntryP < StrucList.getCount()) {
					PPGoodsStruc gs;
					const StrucEntry & r_struc_entry = StrucList.at(brw_hdr.StrucEntryP);
					if(GSObj.Get(brw_hdr.GStrucID, &gs) > 0) {
						const PPID  goods_id = r_struc_entry.PrmrGoodsID;
						GoodsOpAnalyzeFilt filt;
						filt.OpGrpID = GoodsOpAnalyzeFilt::ogInOutAnalyze;
						filt.Flags |= GoodsOpAnalyzeFilt::fLeaderInOutGoods;
						filt.GoodsIdList.Add(goods_id);
						filt.Period.low = getcurdate_(); // @v10.2.3
						plusperiod(&filt.Period.low, PRD_ANNUAL, -1, 0); // @v10.2.3
						ViewGoodsOpAnalyze(&filt);
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
				//GSObj.CheckStructs();
				Recover();
				break;
			case PPVCMD_TOTAL:
				ok = ViewTotal();
				break;
			case PPVCMD_NEXTPROBLEM:
				if(brw_hdr.GStrucID && Problems.getCount()) {
					for(uint i = 0; i < Problems.getCount(); i++) {
						const PPObjGoodsStruc::CheckGsProblem * p_problem = Problems.at(i);
						if(p_problem->GsID > brw_hdr.GStrucID) {
							if(pBrw->search2(&p_problem->GsID, CMPF_LONG, srchFirst, 0)) {
								ok = 1;
								break;
							}
						}
					}
				}
				break;
			case PPVCMD_PREVPROBLEM:
				if(brw_hdr.GStrucID && Problems.getCount()) {
					uint i = Problems.getCount();
					if(i) do {
						const PPObjGoodsStruc::CheckGsProblem * p_problem = Problems.at(--i);
						if(p_problem->GsID < brw_hdr.GStrucID) {
							if(pBrw->search2(&p_problem->GsID, CMPF_LONG, srchFirst, 0)) {
								ok = 1;
								break;
							}
						}
					} while(i);
				}
				break;
			case PPVCMD_MOUSEHOVER:
				if(brw_hdr.GStrucID) {
					uint    pp = 0;
					if(Problems.getCount() && Problems.bsearch(&brw_hdr.GStrucID, &pp, CMPF_LONG)) {
						const PPObjGoodsStruc::CheckGsProblem * p_problem = Problems.at(pp);
						SString buf = p_problem->Text;
						PPTooltipMessage(buf, 0, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|SMessageWindow::fTextAlignLeft|
							SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
					}
				}
				break;
		}
	}
	if(ok > 0) {
		if(ppvCmd == PPVCMD_EDITITEM) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				p_def->setArray(new SArray(ItemList), 0, 1);
				pBrw->search2(&brw_hdr.GStrucID, CMPF_LONG, srchFirst, 0);
			}
		}
		else if(ppvCmd == PPVCMD_USERSORT) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(ItemList);
				p_def->setArray(p_array, 0, 1);
				if(p_array) {
					pBrw->setRange(p_array->getCount());
					uint   temp_pos = 0;
					long   update_pos = -1;
					if(pHdr) {
						for(uint i = 0; i < ItemList.getCount(); i++) {
							const ItemEntry & r_entry = ItemList.at(i);
							if(r_entry.GStrucID == brw_hdr.GStrucID && r_entry.ItemNo == brw_hdr.ItemNo) {
								update_pos = static_cast<long>(i);
								break;
							}
						}
					}
					if(update_pos >= 0)
						pBrw->go(update_pos);
					else if(update_pos == MAXLONG)
						pBrw->go(p_array->getCount()-1);
				}
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
