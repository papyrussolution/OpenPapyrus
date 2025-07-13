// V_GSTRUC.CPP
// Copyright (c) A.Starodub 2007, 2008, 2009, 2014, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// Таблица просмотра товарных структур
//
#include <pp.h>
#pragma hdrstop

GoodsStrucProcessingBlock::GoodsStrucProcessingBlock() : P_TecObj(0)
{
}

GoodsStrucProcessingBlock::GoodsStrucProcessingBlock(const GoodsStrucProcessingBlock & rS) : P_TecObj(0), StrucList(rS.StrucList), ItemList(rS.ItemList), StrPool(rS.StrPool)
{
}

GoodsStrucProcessingBlock::~GoodsStrucProcessingBlock()
{
	delete P_TecObj; // @v11.7.6
}

/*static*/int FASTCALL PPViewGoodsStruc::Cmp_ItemEntry(const PPViewGoodsStruc * pView, int order, const void * i1, const void * i2)
{
	int    si = 0;
	if(pView && i1 && i2) {
		const GoodsStrucProcessingBlock::ItemEntry * p1 = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(i1);
		const GoodsStrucProcessingBlock::ItemEntry * p2 = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(i2);
		const uint slc = pView->Cb.StrucList.getCount();
		SString n1, n2;
		switch(order) {
			default: // @fallthrough
			case OrdByPrmrGoodsName:
				{
					if(p1->StrucEntryP < slc) {
						GetGoodsName(pView->Cb.StrucList.at(p1->StrucEntryP).PrmrGoodsID, n1);
					}
					if(p2->StrucEntryP < slc) {
						GetGoodsName(pView->Cb.StrucList.at(p2->StrucEntryP).PrmrGoodsID, n2);
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
							const GoodsStrucProcessingBlock::StrucEntry & r_entry = pView->Cb.StrucList.at(p1->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n1);
						}
						if(p2->StrucEntryP < slc) {
							const GoodsStrucProcessingBlock::StrucEntry & r_entry = pView->Cb.StrucList.at(p2->StrucEntryP);
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
							const GoodsStrucProcessingBlock::StrucEntry & r_entry = pView->Cb.StrucList.at(p1->StrucEntryP);
							GetGoodsName(r_entry.PrmrGoodsID, n1);
						}
						if(p2->StrucEntryP < slc) {
							const GoodsStrucProcessingBlock::StrucEntry & r_entry = pView->Cb.StrucList.at(p2->StrucEntryP);
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

static IMPL_CMPFUNC(GoodsStrucView_ItemEntry_CurrentOrder, i1, i2)
{
	PPViewGoodsStruc * p_view = static_cast<PPViewGoodsStruc *>(pExtraData);
	return p_view ? PPViewGoodsStruc::Cmp_ItemEntry(p_view, p_view->GetCurrentViewOrder(), i1, i2) : 0;
}

class GoodsStrucFiltDialog : public TDialog {
	DECL_DIALOG_DATA(GoodsStrucFilt);
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
	DECL_DIALOG_SETDTS()
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
		AddClusterAssoc(CTL_GSFILT_FLAGS, 1, GoodsStrucFilt::fSkipByPassiveOwner);
		AddClusterAssoc(CTL_GSFILT_FLAGS, 2, GoodsStrucFilt::fShowTech); // @v11.7.6
		AddClusterAssoc(CTL_GSFILT_FLAGS, 3, GoodsStrucFilt::fShowEstValue); // @v11.8.2
		SetClusterData(CTL_GSFILT_FLAGS, Data.Flags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
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
};

PPBaseFilt * PPViewGoodsStruc::CreateFilt(const void * extraPtr) const
{
	PPBaseFilt * p_base_filt = 0;
	if(PPView::CreateFiltInstance(PPFILT_GOODSSTRUC, &p_base_filt))
		p_base_filt->Init(1, 0);
	else 
		PPSetError(PPERR_BASEFILTUNSUPPORTED);
	return p_base_filt;
}

int PPViewGoodsStruc::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	GoodsStrucFilt * p_filt = static_cast<GoodsStrucFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(GoodsStrucFiltDialog, p_filt);
}

IMPLEMENT_PPFILT_FACTORY(GoodsStruc); GoodsStrucFilt::GoodsStrucFilt() : PPBaseFilt(PPFILT_GOODSSTRUC, 0, 0)
{
	SetFlatChunk(offsetof(GoodsStrucFilt, ReserveStart),
		offsetof(GoodsStrucFilt, ReserveEnd) - offsetof(GoodsStrucFilt, ReserveStart));
	Init(1, 0);
}

PPViewGoodsStruc::PPViewGoodsStruc() : 
	PPView(0, &Filt, PPVIEW_GOODSSTRUC, implBrowseArray, REPORT_GOODSSTRUCLIST), CurrentViewOrder(OrdByDefault), IterIdx(0), P_DsList__(0),
	H_AsideListWindow(0)
{
}

PPViewGoodsStruc::~PPViewGoodsStruc()
{
	ZDELETE(P_DsList__);
}

int PPViewGoodsStruc::CmpSortIndexItems(PPViewBrowser * pBrw, const GoodsStrucProcessingBlock::ItemEntry * pItem1, const GoodsStrucProcessingBlock::ItemEntry * pItem2)
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
			const GoodsStrucProcessingBlock::ItemEntry * p_item1 = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(i1);
			const GoodsStrucProcessingBlock::ItemEntry * p_item2 = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

int PPViewGoodsStruc::SortList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	const  int is_sorting_needed = BIN(pBrw && pBrw->GetSettledOrderList().getCount());
	if(is_sorting_needed) {
		Cb.ItemList.sort(PTR_CMPFUNC(ViewGoodsStruc_ItemEntry), pBrw);
	}
	return ok;
}

int PPViewGoodsStruc::MakeList(PPViewBrowser * pBrw)
{
	Problems.freeAll();
	Cb.StrucList.clear();
	Cb.ItemList.clear();
	Cb.StrPool.ClearS();
	int    ok = 1;
	// @v11.7.6 {
	uint   add_item_flags = 0;
	if(Filt.Flags & Filt.fShowTech)
		add_item_flags = GoodsStrucProcessingBlock::addifInitTech;
	// } @v11.7.6 
	if(Filt.Flags & Filt.fShowEstValue)
		add_item_flags = GoodsStrucProcessingBlock::addifInitEstValue;
	Goods2Tbl::Rec grec;
	if(Filt.PrmrGoodsID) {
		if(Cb.GObj.Search(Filt.PrmrGoodsID, &grec) > 0) {
			THROW(Cb.AddItem(grec.ID, grec.StrucID, Filt.ScndGoodsGrpID, Filt.ScndGoodsID, add_item_flags));
		}
	}
	else {
		GoodsFilt goods_filt;
		goods_filt.GrpIDList.Add(Filt.PrmrGoodsGrpID);
		goods_filt.Flags |= GoodsFilt::fWithStrucOnly;
		for(GoodsIterator gi(&goods_filt, 0); gi.Next(&grec) > 0;) {
			if(!(Filt.Flags & Filt.fSkipByPassiveOwner) || !(grec.Flags & GF_PASSIV)) {
				THROW(Cb.AddItem(grec.ID, grec.StrucID, Filt.ScndGoodsGrpID, Filt.ScndGoodsID, add_item_flags));
			}
			PPWaitPercent(gi.GetIterCounter());
		}
		if(!Filt.PrmrGoodsGrpID && !Filt.PrmrGoodsID && Filt.Flags & Filt.fShowUnrefs) {
			uint   t = 0;
			uint   p = 0;
			PPIDArray owner_list;
			PPGoodsStrucHeader gsh;
			for(SEnum en = Cb.GSObj.Enum(0); en.Next(&gsh) > 0;)
				t++;
			for(SEnum en = Cb.GSObj.Enum(0); en.Next(&gsh) > 0; p++) {
				const  PPID gs_id = gsh.ID;
				Cb.GObj.SearchGListByStruc(gs_id, false/*expandGenerics*/, owner_list);
				if(!owner_list.getCount() && !(gsh.Flags & GSF_CHILD)) {
					uint   ex_spos = 0;
					if(!Cb.StrucList.lsearch(&gs_id, &ex_spos, CMPF_LONG, offsetof(GoodsStrucProcessingBlock::StrucEntry, GStrucID))) {
						THROW(Cb.AddItem(0, gs_id, Filt.ScndGoodsGrpID, Filt.ScndGoodsID, add_item_flags));
					}
				}
				PPWaitPercent(p, t);
			}
		}
	}
	Cb.ItemList.sort(PTR_CMPFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
	if(pBrw) {
		pBrw->Helper_SetAllColumnsSortable();
		SortList(pBrw);
	}
	CATCHZOK
	return ok;
}

int PPViewGoodsStruc::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	PPWaitStart();
	CurrentViewOrder = static_cast<IterOrder>(Filt.InitOrder);
	THROW(MakeList(0));
	CATCHZOK
	PPWaitStop();
	return ok;
}

int GoodsStrucProcessingBlock::AddItem(PPID goodsID, PPID strucID, PPID filtScndGroupID, PPID filtScndID, uint __flags/*bool checkExistance, bool recursive*/)
{
	int    ok = 1;
	PPGoodsStruc struc;
	const  PPID loc_id = LConfig.Location; // @v11.8.2
	if(__flags & addifCheckExistance) {
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
				const  PPID ex_goods_id = ex_goods_list.get(p);
				THROW(AddItem(ex_goods_id, strucID, filtScndGroupID, filtScndID, (__flags & addifRecursive))); // @recursion
			}
		}
	}
	if(GSObj.Get(strucID, &struc) > 0) {
		const  bool is_folder = LOGIC(struc.Rec.Flags & GSF_FOLDER);
		const  uint count  = is_folder ? struc.Children.getCount() : struc.Items.getCount();
		if(count) {
			SString temp_buf;
			SString struc_name;
			PPIDArray tec_id_list; // Список технологий, ассоциированных со структурой
			uint    struc_entry_pplus1 = 0;
			for(uint i = 0; i < count; i++) {
				if(is_folder) {
					const PPGoodsStruc * p_struc = struc.Children.at(i);
					THROW(AddItem(goodsID, p_struc->Rec.ID, filtScndGroupID, filtScndID, __flags)); // @recursion
				}
				else {
					const PPGoodsStrucItem & r_i = struc.Items.at(i);
					if((!filtScndID || r_i.GoodsID == filtScndID) && (!filtScndGroupID || GObj.BelongToGroup(r_i.GoodsID, filtScndGroupID, 0))) {
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
							// @v11.7.6 {
							if((__flags & addifInitTech) && SETIFZ(P_TecObj, new PPObjTech())) {
								P_TecObj->GetListByGoodsStruc(struc.Rec.ID, &tec_id_list);
								if(tec_id_list.getCount()) {
									new_entry.SingleAssociatedTechID = tec_id_list.get(0);
									if(tec_id_list.getCount() > 1)
										new_entry.InternalFlags |= intfMultAssociatedTech;
								}
							}
							// } @v11.7.6 
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
								if(struc_name.IsEmpty())
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
						// @v11.8.2 {
						if(__flags & addifInitEstValue) { 
							double price = 0.0;
							double sum = 0.0;
							if(struc.GetEstimationPrice(i, loc_id, &price, &sum, 0) > 0 && sum > 0.0)
								new_item.EstPrice = sum;
							else
								new_item.InternalFlags |= intfUncertEstPrice;
						}
						// } @v11.8.2 
						THROW_SL(ItemList.insert(&new_item));
						if(__flags & addifRecursive) {
							PPGoodsStruc inner_struc;
							if(GObj.LoadGoodsStruc(PPGoodsStruc::Ident(new_item.GoodsID, GSF_COMPL, GSF_PARTITIAL), &inner_struc) > 0) {
								THROW(AddItem(new_item.GoodsID, inner_struc.Rec.ID, filtScndGroupID, filtScndID, (/*addifCheckExistance|*/addifRecursive))); // @recusive
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewGoodsStruc::InitIteration()
{
	Counter.Init(Cb.ItemList.getCount());
	IterIdx = 0;
	return 1;
}

int FASTCALL PPViewGoodsStruc::NextIteration(GoodsStrucViewItem * pItem)
{
	int    ok = -1;
	if(pItem && IterIdx < Cb.ItemList.getCount()) {
		memzero(pItem, sizeof(*pItem));
		const GoodsStrucProcessingBlock::ItemEntry & r_item = Cb.ItemList.at(IterIdx);
		pItem->GStrucID = r_item.GStrucID;
		if(r_item.StrucEntryP < Cb.StrucList.getCount()) {
			SString temp_buf;
			const GoodsStrucProcessingBlock::StrucEntry & r_struc_entry = Cb.StrucList.at(r_item.StrucEntryP);
			pItem->PrmrGoodsID = r_struc_entry.PrmrGoodsID;
			pItem->CommDenom = r_struc_entry.CommDenom;
			pItem->GiftAmtRestrict = r_struc_entry.GiftAmtRestrict;
			pItem->GiftLimit = r_struc_entry.GiftLimit;
			pItem->GiftQuotKindID = r_struc_entry.GiftQuotKindID;
			pItem->ParentStrucID = r_struc_entry.ParentStrucID;
			pItem->Period = r_struc_entry.Period;
			pItem->StrucFlags = r_struc_entry.Flags;
			pItem->VariedPropObjType = r_struc_entry.VariedPropObjType;
			Cb.StrPool.GetS(r_struc_entry.NameP, temp_buf);
			STRNSCPY(pItem->StrucName, temp_buf);
			Cb.StrPool.GetS(r_struc_entry.SymbP, temp_buf);
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

int FASTCALL PPViewGoodsStruc::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	const GoodsStrucProcessingBlock::ItemEntry * p_item = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(pBlk->P_SrcData);
	SString temp_buf;
	int    r = 0;
	switch(pBlk->ColumnN) {
		case 0: pBlk->Set(p_item->GStrucID); break; // @id
		case 1: // Primary goods name
			if(p_item->StrucEntryP < Cb.StrucList.getCount())
				pBlk->Set(GetGoodsName(Cb.StrucList.at(p_item->StrucEntryP).PrmrGoodsID, temp_buf));
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
			if(p_item->StrucEntryP < Cb.StrucList.getCount()) {
				const GoodsStrucProcessingBlock::StrucEntry & r_entry = Cb.StrucList.at(p_item->StrucEntryP);
				PPGoodsStruc::MakeTypeString(r_entry.GStrucID, r_entry.Flags, r_entry.ParentStrucID, temp_buf);
			}
			else
				temp_buf.Z();
			pBlk->Set(temp_buf);
			break;
		case 5: pBlk->Set(p_item->Netto); break; // Netto
		case 6: // Общий делитель структуры
			if(p_item->StrucEntryP < Cb.StrucList.getCount()) {
				const GoodsStrucProcessingBlock::StrucEntry & r_entry = Cb.StrucList.at(p_item->StrucEntryP);
				pBlk->Set(r_entry.CommDenom);
			}
			else
				pBlk->Set(0.0);
			break;
		case 7: pBlk->Set(p_item->ItemNo); break; // Номер строки внутри стурктуры
		case 8: // Наименование структуры
			{
				temp_buf.Z();
				PPGoodsStrucHeader rec;
				if(Cb.GSObj.Fetch(p_item->GStrucID, &rec) > 0)
					temp_buf = rec.Name;
				pBlk->Set(temp_buf);
			}
			break;
		case 9: // @v11.7.6 Ассоциированная технология //
			{
				temp_buf.Z();
				if(p_item->StrucEntryP < Cb.StrucList.getCount()) {
					const GoodsStrucProcessingBlock::StrucEntry & r_se = Cb.StrucList.at(p_item->StrucEntryP);
					if(r_se.SingleAssociatedTechID) {
						if(SETIFZ(Cb.P_TecObj, new PPObjTech())) {
							TechTbl::Rec tec_rec;
							if(Cb.P_TecObj->Fetch(r_se.SingleAssociatedTechID, &tec_rec) > 0)
								temp_buf = tec_rec.Code;
							else
								ideqvalstr(r_se.SingleAssociatedTechID, temp_buf);
							if(r_se.InternalFlags & GoodsStrucProcessingBlock::intfMultAssociatedTech)
								temp_buf.CatCharN('.', 3);
							pBlk->Set(temp_buf);
						}
					}
				}
			}
			break;
		case 10: // @v11.9.2 Ожидаемая цена
			{
				temp_buf.Z();
				if(p_item->StrucEntryP < Cb.StrucList.getCount()) {
					if(p_item->InternalFlags & GoodsStrucProcessingBlock::intfUncertEstPrice) {
						temp_buf.CatChar('?');
					}
					else if(p_item->EstPrice > 0.0) {
						temp_buf.Cat(p_item->EstPrice, MKSFMTD_020);
					}
					pBlk->Set(temp_buf);
				}
			}
			break;
	}
	return ok;
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

int PPViewGoodsStruc::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, PPViewBrowser * pBrw)
{
	int    ok = -1;
	if(pBrw && pData && pStyle) {
		const  BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < static_cast<long>(p_def->getCount())) {
			const BroColumn & r_col = p_def->at(col);
			const GoodsStrucProcessingBlock::ItemEntry * p_item = static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(pData);
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
			else if(r_col.OrgOffs == 6) { // common denomitator
				if(Cb.GObj.GetConfig().Flags & GCF_BANSTRUCCDONDECOMPL) {
					if(p_item->GStrucID && p_item->StrucEntryP < Cb.StrucList.getCount()) {
						const GoodsStrucProcessingBlock::StrucEntry & r_struc_entry = Cb.StrucList.at(p_item->StrucEntryP);
						if((r_struc_entry.Flags & GSF_DECOMPL) && !(r_struc_entry.Flags & GSF_COMPL)) {
							if(r_struc_entry.CommDenom != 0.0 && r_struc_entry.CommDenom != 1.0)
								ok = pStyle->SetRightFigCircleColor(GetColorRef(SClrBrown));
						}
					}
				}
			}
		}
	}
	return ok;
}

void PPViewGoodsStruc::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewGoodsStruc *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		if(Filt.Flags & GoodsStrucFilt::fShowEstValue) { // @v11.8.2
			pBrw->InsColumn(-1, "@expectedprice", 10, MKSTYPE(S_ZSTRING, 64), MKSFMT(0, ALIGN_RIGHT), BCO_USERPROC); // #10 estimated price
		}
		if(Filt.Flags & GoodsStrucFilt::fShowTech) {
			pBrw->InsColumn(-1, "@tech", 9, MKSTYPE(S_ZSTRING, 64), MKSFMT(64, 0), BCO_USERPROC); // #9  tech
		}
		pBrw->Helper_SetAllColumnsSortable();
	}
}

SArray * PPViewGoodsStruc::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	ASSIGN_PTR(pBrwId, BROWSER_GOODSSTRUC2);
	return new SArray(Cb.ItemList);
}

int PPViewGoodsStruc::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		GoodsStrucViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPIDArray uniq_struc_list;
		PPObjIDArray objid_ary;
		PPWaitStart();
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
	PPWaitStop();
	return ok;
}

int PPViewGoodsStruc::Recover()
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
		PPWaitStart();
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			uniq_struc_list.addnz(item.GStrucID);
		}
		if(uniq_struc_list.getCount()) {
			PPIDArray goods_ids;
			uniq_struc_list.sortAndUndup();
			for(uint i = 0; i < uniq_struc_list.getCount(); i++) {
				PPID   id = uniq_struc_list.get(i);
				{
					PPGoodsStruc gs;
					PPIDArray struct_ids;
					Cb.GObj.SearchGListByStruc(id, false/*expandGenerics*/, goods_ids);
					THROW(Cb.GSObj.Get(id, &gs));
					THROW(Cb.GSObj.CheckStruct(&goods_ids, &struct_ids, &gs, &Problems, &logger));
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
						nna_gs_list.add(p_problem->LocIdent);
				}
				if(flags & cfSpcDetaching) {
					PPGoodsStrucHeader gs_rec;
					PPIDArray gs_id_list;
					const size_t spl = sstrlen(p_surrogate_prefix);
					for(SEnum en = Cb.GSObj.Enum(0); en.Next(&gs_rec) > 0;) {
						if(gs_rec.Flags & GSF_NAMED && strncmp(gs_rec.Name, p_surrogate_prefix, spl) == 0) {
							gs_id_list.add(gs_rec.ID);
						}
					}
					if(gs_id_list.getCount()) {
						PPTransaction tra(1);
						THROW(tra);
						gs_id_list.sortAndUndup();
						for(uint i = 0; i < gs_id_list.getCount(); i++) {
							const  PPID gs_id = gs_id_list.get(i);
							Cb.GObj.SearchGListByStruc(gs_id, false/*expandGenerics*/, owner_list);
							if(owner_list.getCount()) {
								for(uint oidx = 0; oidx < owner_list.getCount(); oidx++) {
									PPID goods_id = owner_list.get(oidx);
									PPGoodsPacket goods_pack;
									THROW(Cb.GObj.GetPacket(goods_id, &goods_pack, 0) > 0);
									if(goods_pack.Rec.StrucID == gs_id) {
										goods_pack.Rec.StrucID = 0;
										goods_pack.GS.Rec.ID = 0;
										THROW(Cb.GObj.PutPacket(&goods_id, &goods_pack, 0));
									}
								}
							}
						}
						THROW(tra.Commit());
					}
				}
				if(nna_gs_list.getCount()) {
					nna_gs_list.sortAndUndup();
					SString surrogate_name;
					PPTransaction tra(1);
					THROW(tra);
					for(uint k = 0; k < nna_gs_list.getCount(); k++) {
						const  PPID gs_id = nna_gs_list.get(k);
						Cb.GObj.SearchGListByStruc(gs_id, false/*expandGenerics*/, owner_list);
						if(owner_list.getCount() > 1) {
							PPGoodsStrucHeader gsh;
							if(Cb.GSObj.Search(gs_id, &gsh) > 0) {
								if(!(gsh.Flags & GSF_NAMED) && !(gsh.Flags & GSF_CHILD)) {
									PPGoodsStruc gs;
									THROW(Cb.GSObj.Get(gs_id, &gs) > 0);
									{
										long   nn = 0;
										PPID   n_gs_id = 0;
										surrogate_name = p_surrogate_prefix;
										STRNSCPY(gs.Rec.Name, surrogate_name);
										while(Cb.GSObj.SearchByName(gs.Rec.Name, &n_gs_id, 0) > 0 && n_gs_id != gs_id) {
											(surrogate_name = p_surrogate_prefix).Space().Cat(++nn);
											STRNSCPY(gs.Rec.Name, surrogate_name);
										}
									}
									gs.Rec.Flags |= GSF_NAMED;
									PPID   temp_id = gs_id;
									THROW(Cb.GSObj.Put(&temp_id, &gs, 0));
									assert(temp_id == gs_id);
									//
									// Снимаем ссылку на структуру со всех товаров // (@v10.0.12 отменено) кроме последнего (с наибольшим идентификатором)
									//
									for(uint oidx = 0; oidx < owner_list.getCount(); oidx++) {
										PPID   goods_id = owner_list.get(oidx);
										PPGoodsPacket goods_pack;
										THROW(Cb.GObj.GetPacket(goods_id, &goods_pack, 0) > 0);
										if(goods_pack.Rec.StrucID == gs_id) {
											goods_pack.Rec.StrucID = 0;
											goods_pack.GS.Rec.ID = 0;
											THROW(Cb.GObj.PutPacket(&goods_id, &goods_pack, 0));
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
	PPWaitStop();
	return ok;
}

int PPViewGoodsStruc::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	PPID   parent_struc_id = 0;
	GoodsStrucProcessingBlock::ItemEntry brw_hdr;
	if(ok == -2) {
		if(pHdr && ppvCmd != PPVCMD_FOREIGNFOCUCNOTIFICATION) // @v11.1.12 PPVCMD_FOREIGNFOCUCNOTIFICATION
			brw_hdr = *static_cast<const GoodsStrucProcessingBlock::ItemEntry *>(pHdr);
		else
			MEMSZERO(brw_hdr);
		switch(ppvCmd) {
			case PPVCMD_USERSORT: SortList(pBrw); ok = 1; break; // The rest will be done below
			case PPVCMD_EDITITEM:
			case PPVCMD_EDITGOODS:
			case PPVCMD_EDITITEMGOODS:
				if(brw_hdr.GStrucID && brw_hdr.StrucEntryP < Cb.StrucList.getCount()) {
					PPGoodsStruc gs;
					const GoodsStrucProcessingBlock::StrucEntry & r_struc_entry = Cb.StrucList.at(brw_hdr.StrucEntryP);
					if(Cb.GSObj.Get(brw_hdr.GStrucID, &gs) > 0) {
						int r = 0;
						if(gs.Rec.ParentID != 0) {
							parent_struc_id = gs.Rec.ParentID;
							r = Cb.GSObj.Get(parent_struc_id, &gs);
						}
						else
							r = 1;
						gs.GoodsID = r_struc_entry.PrmrGoodsID;
						if(oneof2(ppvCmd, PPVCMD_EDITGOODS, PPVCMD_EDITITEMGOODS)) {
							PPID   goods_id = (ppvCmd == PPVCMD_EDITGOODS) ? r_struc_entry.PrmrGoodsID : brw_hdr.GoodsID;
							ok = (Cb.GObj.Edit(&goods_id, 0) == cmOK) ? 1 : -1;
						}
						else if(r > 0 && Cb.GSObj.EditDialog(&gs) > 0) {
							PPID   struc_id = parent_struc_id ? parent_struc_id : brw_hdr.GStrucID;
							int r = Cb.GSObj.Put(&struc_id, &gs, 1);
							if(r > 0) {
								Cb.AddItem(r_struc_entry.PrmrGoodsID, struc_id, Filt.ScndGoodsGrpID, Filt.ScndGoodsID, GoodsStrucProcessingBlock::addifCheckExistance);
								Cb.ItemList.sort(PTR_CMPFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
								ok = 1;
							}
							else if(!r)
								ok = PPErrorZ();
						}
					}
				}
				break;
			case PPVCMD_VIEWTECH: // @v11.7.6
				ok = -1;
				if(brw_hdr.GStrucID) {
					TechFilt filt;
					filt.GStrucID = brw_hdr.GStrucID;
					::ViewTech(&filt);
				}
				break;
			case PPVCMD_VIEWGOODSOPANLZ:
				if(brw_hdr.GStrucID && brw_hdr.StrucEntryP < Cb.StrucList.getCount()) {
					PPGoodsStruc gs;
					const GoodsStrucProcessingBlock::StrucEntry & r_struc_entry = Cb.StrucList.at(brw_hdr.StrucEntryP);
					if(Cb.GSObj.Get(brw_hdr.GStrucID, &gs) > 0) {
						const  PPID  goods_id = r_struc_entry.PrmrGoodsID;
						GoodsOpAnalyzeFilt filt;
						filt.OpGrpID = GoodsOpAnalyzeFilt::ogInOutAnalyze;
						filt.Flags |= GoodsOpAnalyzeFilt::fLeaderInOutGoods;
						filt.GoodsIdList.Add(goods_id);
						filt.Period.low = getcurdate_();
						plusperiod(&filt.Period.low, PRD_ANNUAL, -1, 0);
						ViewGoodsOpAnalyze(&filt);
					}
				}
				break;
			case PPVCMD_SYSJ:
				if(brw_hdr.GStrucID) {
					ViewSysJournal(PPOBJ_GOODSSTRUC, brw_hdr.GStrucID, 0);
					ok = -1;
				}
				break;
			case PPVCMD_TRANSMIT:
				ok = -1;
				Transmit(0);
				break;
			case PPVCMD_DORECOVER:
				ok = -1;
				//GSObj.CheckStructs();
				Recover();
				break;
			case PPVCMD_TOTAL:
				ok = -1;
				ViewTotal();
				break;
			case PPVCMD_TREEVIEW: // @v11.1.12 @construction
				ok = -1;
				MakeTreeListView(pBrw);
				break;
			case PPVCMD_NEXTPROBLEM:
				if(brw_hdr.GStrucID && Problems.getCount()) {
					for(uint i = 0; i < Problems.getCount(); i++) {
						const PPObjGoodsStruc::CheckGsProblem * p_problem = Problems.at(i);
						if(static_cast<PPID>(p_problem->LocIdent) > brw_hdr.GStrucID) {
							if(pBrw->search2(&p_problem->LocIdent, CMPF_LONG, srchFirst, 0)) {
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
						if(static_cast<PPID>(p_problem->LocIdent) < brw_hdr.GStrucID) {
							if(pBrw->search2(&p_problem->LocIdent, CMPF_LONG, srchFirst, 0)) {
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
						SString buf(p_problem->Descr);
						PPTooltipMessage(buf, 0, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|SMessageWindow::fTextAlignLeft|
							SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
					}
				}
				break;
			case PPVCMD_FOREIGNFOCUCNOTIFICATION: // @v11.1.12
				{
					ok = -1;
					const ForeignFocusEvent * p_ev_data = static_cast<const ForeignFocusEvent *>(pHdr);
					if(p_ev_data->P_SrcView && p_ev_data->FocusedItemIdent) {
						if(p_ev_data->FocusedItemIdent & 0xff000000) {
							pBrw->go((p_ev_data->FocusedItemIdent & ~0xff000000));
						}
						else {
							long gstruc_id = p_ev_data->FocusedItemIdent;
							pBrw->search2(&gstruc_id, CMPF_LONG, srchFirst, 0);
						}
					}
				}
				break;
		}
	}
	if(ok > 0) {
		if(ppvCmd == PPVCMD_EDITITEM) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				p_def->setArray(new SArray(Cb.ItemList), 0, 1);
				pBrw->search2(&brw_hdr.GStrucID, CMPF_LONG, srchFirst, 0);
			}
		}
		else if(ppvCmd == PPVCMD_USERSORT) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(Cb.ItemList);
				p_def->setArray(p_array, 0, 1);
				if(p_array) {
					pBrw->setRange(p_array->getCount());
					uint   temp_pos = 0;
					long   update_pos = -1;
					if(pHdr) {
						for(uint i = 0; i < Cb.ItemList.getCount(); i++) {
							const GoodsStrucProcessingBlock::ItemEntry & r_entry = Cb.ItemList.at(i);
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

void PPViewGoodsStruc::ViewTotal()
{
	long   goods_count = 0, lines_count = 0;
	PPID   prev_goods = 0;
	TDialog * p_dlg = new TDialog(DLG_TGSTRUC);
	if(CheckDialogPtr(&p_dlg)) {
		GoodsStrucViewItem item;
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
	}
}
//
//
//
GoodsStrucTreeListViewBlock::GoodsStrucTreeListViewBlock(const GoodsStrucProcessingBlock & rCb) : Cb(rCb), ItemOffset(0x70000000L)
{
}

StrAssocTree * GoodsStrucTreeListViewBlock::MakeTree()
{
	StrAssocTree * p_list = new StrAssocTree;
	if(p_list) {
		const uint slc = Cb.StrucList.getCount();
		if(slc > 16)
			PPWait(1);
		LongArray absence_struc_item_list;
		LongArray recur_list;
		SString temp_buf;
		Goods2Tbl::Rec goods_rec;
		PPGoodsStrucHeader gs_rec;
		for(uint sidx = 0; sidx < slc; sidx++) {
			const GoodsStrucProcessingBlock::StrucEntry & r_se = Cb.StrucList.at(sidx);
			if(r_se.ParentStrucID == 0) {
				const  PPID gs_id = r_se.GStrucID;
				SPtrHandle _ph = p_list->Search(gs_id);
				if(!_ph) {
					SPtrHandle result = p_list->Add_Unsafe(SPtrHandle(), gs_id, MakeText(r_se, temp_buf));
					if(result) {
						for(uint iidx = 0; iidx < Cb.ItemList.getCount(); iidx++) {
							const GoodsStrucProcessingBlock::ItemEntry & r_ie = Cb.ItemList.at(iidx);
							if(r_ie.GStrucID == gs_id) {
								recur_list.Z(); // @v11.4.0
								AddEntry_TopDown(p_list, 0, iidx, r_ie, result, recur_list);
							}
						}
					}
				}
			}
			PPWaitPercent(sidx+1, Cb.StrucList.getCount());
		}
		p_list->SortByText();
		PPWait(0);
	}
	return p_list;
}
	
const GoodsStrucProcessingBlock::ItemEntry * GoodsStrucTreeListViewBlock::GetEntryByIdx(uint idx) const
{
	return (idx < Cb.ItemList.getCount()) ? &Cb.ItemList.at(idx) : 0;
}
	
const GoodsStrucProcessingBlock::StrucEntry * GoodsStrucTreeListViewBlock::GetStrucEntryByID(PPID id) const
{
	uint  pos = 0;
	return Cb.StrucList.lsearch(&id, &pos, CMPF_LONG) ? &Cb.StrucList.at(pos) : 0;
}

SString & GoodsStrucTreeListViewBlock::MakeText(const GoodsStrucProcessingBlock::StrucEntry & rSe, SString & rTitleBuf)
{
	rTitleBuf.Z();
	if(rSe.PrmrGoodsID) {
		Goods2Tbl::Rec goods_rec;
		if(Cb.GObj.Fetch(rSe.PrmrGoodsID, &goods_rec) > 0)
			rTitleBuf = goods_rec.Name;
		else
			ideqvalstr(rSe.PrmrGoodsID, rTitleBuf);
	}
	else
		rTitleBuf = "#npg";
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(rSe.GStrucID) {
		PPGoodsStrucHeader gs_rec;
		if(Cb.GSObj.Fetch(rSe.GStrucID, &gs_rec) > 0) {
			if(gs_rec.Name[0])
				r_temp_buf = gs_rec.Name;
			else
				(r_temp_buf = "gs").CatLongZ(rSe.GStrucID, 6);
		}
		else
			(r_temp_buf = "#gs").CatLongZ(rSe.GStrucID, 6);
	}
	rTitleBuf.Space().CatChar('[').Cat(r_temp_buf).CatChar(']');
	return rTitleBuf;
}
	
void GoodsStrucTreeListViewBlock::AddEntry_TopDown(StrAssocTree * pList, uint level, uint idx, const GoodsStrucProcessingBlock::ItemEntry & rEntry, SPtrHandle hParent, LongArray & rRecurList)
{
	assert(level <= 255);
	if(rEntry.GStrucID && !rRecurList.lsearch(rEntry.GoodsID)) {
		TitleBuf.Z();
		if(rEntry.GoodsID) {
			Goods2Tbl::Rec goods_rec;
			if(Cb.GObj.Fetch(rEntry.GoodsID, &goods_rec) > 0)
				TitleBuf = goods_rec.Name;
			else
				ideqvalstr(rEntry.GoodsID, TitleBuf);
		}
		else
			TitleBuf = "#ng";
		const long _ident = (idx | ((level + 1) << 24));
		SPtrHandle h_new = pList->Add_Unsafe(hParent, _ident, TitleBuf);
		if(h_new) {
			rRecurList.add(rEntry.GoodsID);
			for(uint inner_gl_idx = 0; Cb.StrucList.lsearch(&rEntry.GoodsID, &inner_gl_idx, CMPF_LONG, offsetof(GoodsStrucProcessingBlock::StrucEntry, PrmrGoodsID)); inner_gl_idx++) {
				const GoodsStrucProcessingBlock::StrucEntry & r_se = Cb.StrucList.at(inner_gl_idx);
				assert(r_se.PrmrGoodsID == rEntry.GoodsID);
				if(r_se.Flags & (GSF_COMPL|GSF_DECOMPL)) {
					for(uint i = 0; i < Cb.ItemList.getCount(); i++) {
						const GoodsStrucProcessingBlock::ItemEntry & r_ie = Cb.ItemList.at(i);
						if(r_ie.GStrucID == r_se.GStrucID) {
							AddEntry_TopDown(pList, level+1, i, r_ie, h_new, rRecurList); // @recursion
						}
					}
					break; // Пока добавим лишь одну найденную подструктуру
				}
			}
		}
	}
}

static int __MakeGoodsStrucTreeListView(/*PPViewBrowser * pBrw*/)
{
	class GStrucListWindow : public ListWindow {
	public:
		GStrucListWindow(GoodsStrucTreeListViewBlock * pBlk, void * hMainBrwWindow) : P_Blk(pBlk), H_MainBrwWindow(hMainBrwWindow), FfeRing(64),
			ListWindow(new StdTreeListBoxDef2_(pBlk ? pBlk->MakeTree() : 0, lbtDisposeData|lbtDblClkNotify|lbtSelNotify|lbtFocNotify, 0))
		{
		}
		~GStrucListWindow()
		{
			delete P_Blk;
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmLBDblClk))
				TVCMD = cmaEdit;
			else if(TVKEYDOWN && TVKEY == kbEnter) {
				event.what = TEvent::evCommand;
				TVCMD = cmaEdit;
			}
			ListWindow::handleEvent(event);
			if(event.isCmd(cmLBItemFocused)) {
				if(H_MainBrwWindow) {
					long   ident = 0;
					if(getResult(&ident)) {
						ForeignFocusEvent ev_data;
						ev_data.P_SrcView = this;
						ev_data.FocusedItemIdent = ident;
						ForeignFocusEvent & r_p = FfeRing.push(ev_data);
						::PostMessage(static_cast<HWND>(H_MainBrwWindow), WM_USER_NOTIFYOTHERWNDEVNT, cmNotifyForeignFocus, reinterpret_cast<LPARAM>(&r_p));
					}
				}
			}
			else if(event.isCmd(cmaEdit)) {
				long   ident = 0;
				if(getResult(&ident)) {
					if(ident & 0xff000000) {
						const GoodsStrucProcessingBlock::ItemEntry * p_entry = P_Blk->GetEntryByIdx(static_cast<uint>(ident & ~0xff000000));
						const GoodsStrucProcessingBlock::StrucEntry * p_struc_entry = p_entry ? P_Blk->GetStrucEntryByID(p_entry->GStrucID) : 0;
						if(p_struc_entry) {
							PPGoodsStruc gs;
							PPID  parent_struc_id = 0;
							if(GsObj.Get(p_entry->GStrucID, &gs) > 0) {
								int r = 0;
								if(gs.Rec.ParentID != 0) {
									parent_struc_id = gs.Rec.ParentID;
									r = GsObj.Get(parent_struc_id, &gs);
								}
								else
									r = 1;
								gs.GoodsID = p_struc_entry->PrmrGoodsID;
								if(r > 0 && GsObj.EditDialog(&gs) > 0) {
									PPID   struc_id = parent_struc_id ? parent_struc_id : p_entry->GStrucID;
									r = P_Blk->Cb.GSObj.Put(&struc_id, &gs, 1);
									if(r > 0) {
										P_Blk->Cb.AddItem(p_struc_entry->PrmrGoodsID, struc_id, 0/*Filt.ScndGoodsGrpID*/, /*Filt.ScndGoodsID*/0, 
											GoodsStrucProcessingBlock::addifCheckExistance);
										P_Blk->Cb.ItemList.sort(PTR_CMPFUNC(GoodsStrucView_ItemEntry_CurrentOrder), this);
									}
									else if(!r) {
										; // @err
									}
								}
							}
						}
					}
					else {
						PPID _id_to_edit = ident;
						if(GsObj.Edit(&_id_to_edit, 0) > 0) {
							;
						}
					}
				}
			}
		}
		TSRingStack <ForeignFocusEvent> FfeRing;
		GoodsStrucTreeListViewBlock * P_Blk;
		void * H_MainBrwWindow;
		PPObjGoodsStruc GsObj;
	};
	int    ok = 1;
	/*{
		GoodsStrucTreeListViewBlock * p_blk = new GoodsStrucTreeListViewBlock(Cb);
		GStrucListWindow * p_lw = new GStrucListWindow(p_blk, pBrw ? pBrw->H() : 0);
		if(p_lw) {
			SString temp_buf;
			p_lw->SetToolbar(TOOLBAR_LIST_GOODSSTRUC);
			PPLoadString("view_goodsstruc", temp_buf);
			if(APPL->AddListToTree(9459, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), p_lw) > 0) {
				H_AsideListWindow = p_lw->H();
				ok = 1;
			}
			else {
				ok = 0;
				ZDELETE(p_lw);
			}
		}
		else
			ok = 0;
	}*/
	return ok;
}

static int Helper_TreeListToSylk(const StrAssocTree * pData, SPtrHandle hEntry, uint & rRowN, uint levelN, SylkWriter & rSw)
{
	int    ok = 0;
	assert(pData);
	if(pData) {
		long   id = 0;
		long   parent_id = 0;
		SString text_buf;
		bool   do_process_list = false;
		if(!hEntry) {
			do_process_list = true;
		}
		else if(pData->Get_Unsafe(hEntry, &id, &parent_id, &text_buf)) {
			rSw.PutFormat("FG0L", 2, levelN+1, ++rRowN);
			rSw.PutVal(text_buf.cptr(), 1);
			do_process_list = true;
		}
		if(do_process_list) {
			TSVector <SPtrHandle> inner_list;
			if(pData->GetListByParent_Unsafe(hEntry, false, inner_list) > 0) {
				assert(inner_list.getCount());
				for(uint i = 0; i < inner_list.getCount(); i++) {
					Helper_TreeListToSylk(pData, inner_list.at(i), rRowN, levelN+1, rSw); // @recursion
				}
			}
		}
	}
	return ok;
}

static int CopyTreeListToClipboard(const StrAssocTree * pData)
{
	int    ok = -1;
	if(pData && pData->GetCount()) {
		const uint _depth = pData->GetDepth(SPtrHandle(0));
		SString val_buf;
		SString out_buf;
		if(_depth > 0) {
			const char * p_fontface_tnr = "Times New Roman";
			SString dec;
			SylkWriter sw(0);
			sw.PutRec("ID", "PPapyrus");
			{
				char   buf[64];
				::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, sizeof(buf));
				dec.Cat(buf);
			}
			sw.PutFont('F', p_fontface_tnr, 10, slkfsBold);
			sw.PutFont('F', p_fontface_tnr, 8,  0);
			sw.PutRec('F', "G");
			uint   rown = 0;
			uint   level = 0;
			Helper_TreeListToSylk(pData, SPtrHandle(0), rown, level, sw);
			sw.PutLine("E");
			{
				sw.GetBuf(&out_buf);
				SClipboard::Copy_SYLK(out_buf);
				ok = 1;
			}
		}
	}
	return ok;
}

int PPViewGoodsStruc::MakeTreeListView(PPViewBrowser * pBrw) // @v11.1.12 
{
	class GStrucListWindow : public ListWindow {
	public:
		GStrucListWindow(PPViewGoodsStruc * pOwner, GoodsStrucTreeListViewBlock * pBlk, void * hMainBrwWindow) : 
			P_Owner(pOwner), P_Blk(pBlk), H_MainBrwWindow(hMainBrwWindow), FfeRing(64),
			ListWindow(new StdTreeListBoxDef2_(pBlk ? pBlk->MakeTree() : 0, lbtDisposeData|lbtDblClkNotify|lbtSelNotify|lbtFocNotify, 0))
		{
			assert(P_Owner);
		}
		~GStrucListWindow()
		{
			delete P_Blk;
		}
	private:
		DECL_HANDLE_EVENT
		{
			if(event.isCmd(cmLBDblClk))
				TVCMD = cmaEdit;
			else {
				if(TVKEYDOWN) {
					if(TVKEY == kbEnter) {
						event.what = TEvent::evCommand;
						TVCMD = cmaEdit;
					}
				}
			}
			ListWindow::handleEvent(event);
			if(TVKEYDOWN) {
				if(event.isKeyDown(kbCtrlIns)) {
					if(P_Def) {
						StdTreeListBoxDef2_ * p_d = static_cast<StdTreeListBoxDef2_ *>(P_Def);
						CopyTreeListToClipboard(p_d->GetData());
						clearEvent(event);
					}
				}
			}
			else if(event.isCmd(cmLBItemFocused)) {
				if(H_MainBrwWindow) {
					long   ident = 0;
					if(getResult(&ident)) {
						ForeignFocusEvent ev_data;
						ev_data.P_SrcView = this;
						ev_data.FocusedItemIdent = ident;
						ForeignFocusEvent & r_p = FfeRing.push(ev_data);
						::PostMessage(static_cast<HWND>(H_MainBrwWindow), WM_USER_NOTIFYOTHERWNDEVNT, cmNotifyForeignFocus, reinterpret_cast<LPARAM>(&r_p));
					}
				}
			}
			else if(event.isCmd(cmCopyToClipboard)) {
				if(P_Def) {
					StdTreeListBoxDef2_ * p_d = static_cast<StdTreeListBoxDef2_ *>(P_Def);
					CopyTreeListToClipboard(p_d->GetData());
					clearEvent(event);
				}
			}
			else if(event.isCmd(cmaEdit)) {
				long   ident = 0;
				if(getResult(&ident)) {
					if(ident & 0xff000000) {
						const GoodsStrucProcessingBlock::ItemEntry * p_entry = P_Blk->GetEntryByIdx(static_cast<uint>(ident & ~0xff000000));
						const GoodsStrucProcessingBlock::StrucEntry * p_struc_entry = p_entry ? P_Blk->GetStrucEntryByID(p_entry->GStrucID) : 0;
						if(p_struc_entry) {
							PPGoodsStruc gs;
							PPID  parent_struc_id = 0;
							if(GsObj.Get(p_entry->GStrucID, &gs) > 0) {
								int r = 0;
								if(gs.Rec.ParentID != 0) {
									parent_struc_id = gs.Rec.ParentID;
									r = GsObj.Get(parent_struc_id, &gs);
								}
								else
									r = 1;
								gs.GoodsID = p_struc_entry->PrmrGoodsID;
								if(r > 0 && GsObj.EditDialog(&gs) > 0) {
									PPID   struc_id = parent_struc_id ? parent_struc_id : p_entry->GStrucID;
									r = P_Blk->Cb.GSObj.Put(&struc_id, &gs, 1);
									if(r > 0) {
										P_Blk->Cb.AddItem(p_struc_entry->PrmrGoodsID, struc_id, 0/*Filt.ScndGoodsGrpID*/, /*Filt.ScndGoodsID*/0, 
											GoodsStrucProcessingBlock::addifCheckExistance);
										P_Blk->Cb.ItemList.sort(PTR_CMPFUNC(GoodsStrucView_ItemEntry_CurrentOrder), P_Owner); // @v11.5.8 @fix this-->P_Owner
										//ok = 1;
									}
									else if(!r) {
										//ok = PPErrorZ();
									}
								}
							}
						}
					}
					else {
						PPID _id_to_edit = ident;
						if(GsObj.Edit(&_id_to_edit, 0) > 0) {
							;
						}
					}
				}
			}
		}
		PPViewGoodsStruc * P_Owner; // @v11.5.8
		TSRingStack <ForeignFocusEvent> FfeRing;
		GoodsStrucTreeListViewBlock * P_Blk;
		void * H_MainBrwWindow;
		PPObjGoodsStruc GsObj;
	};
	int    ok = 1;
	{
		GoodsStrucTreeListViewBlock * p_blk = new GoodsStrucTreeListViewBlock(Cb);
		GStrucListWindow * p_lw = new GStrucListWindow(this, p_blk, pBrw ? pBrw->H() : 0);
		if(p_lw) {
			SString temp_buf;
			p_lw->SetToolbar(TOOLBAR_LIST_GOODSSTRUC);
			PPLoadString("view_goodsstruc", temp_buf);
			if(APPL->AddListToTree(9459, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), p_lw) > 0) {
				H_AsideListWindow = p_lw->H();
				ok = 1;
			}
			else {
				ok = 0;
				ZDELETE(p_lw);
			}
		}
		else
			ok = 0;
	}
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

void PPALDD_GoodsStrucList::Destroy() { DESTROY_PPVIEW_ALDD(GoodsStruc); }
int  PPALDD_GoodsStrucList::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/) { INIT_PPVIEW_ALDD_ITER(GoodsStruc); }

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

