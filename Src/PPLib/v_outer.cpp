// V_OUTER.CPP
// Copyright (c) A.Sobolev 2025
// @codepage UTF-8
// Реализация класса(-ов) PPView для данных, полученных из внешних источников
//
#include <pp.h>
#pragma hdrstop

PPViewWbPublicGoods::PPViewWbPublicGoods() : PPView(0, &Filt, PPVIEW_WBPUBLICGOODS, implBrowseArray, 0), P_DsList(0), TotalResultCountOnServer(0)
{
}

/*virtual*/PPViewWbPublicGoods::~PPViewWbPublicGoods()
{
	ZDELETE(P_DsList);
}

/*virtual*/int PPViewWbPublicGoods::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(!Filt.IsA(pBaseFilt))
		ok = 0;
	else {
		MarketplaceGoodsSelectionFilt * p_filt = static_cast<MarketplaceGoodsSelectionFilt *>(pBaseFilt);
		int    cat_pool_fetching_result = CatPool.GetCount() ? 1 : 0;
		if(!cat_pool_fetching_result) {
			if(PPMarketplaceInterface_Wildberries::LoadPublicGoodsCategoryList(CatPool, true) > 0) {
				cat_pool_fetching_result = 1;
			}
		}
		if(cat_pool_fetching_result) {
			ok = PPMarketplaceInterface_Wildberries::EditPublicGoodsSelectionFilt(CatPool, *p_filt);
		}
	}
	return ok;
}

/*virtual*/PPBaseFilt * PPViewWbPublicGoods::CreateFilt(const void * extraPtr) const
{
	return new MarketplaceGoodsSelectionFilt();
}

/*virtual*/int PPViewWbPublicGoods::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	if(Helper_InitBaseFilt(pBaseFilt)) {
		if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
			PPMarketplaceInterface_Wildberries::LoadPublicPickUpPointList(PupPool, true/*useCache*/); 
			ok = 1;
		}
		else {
			TotalResultCountOnServer = 0;
			if(Filt.SearchPatternUtf8.NotEmptyS()) {
				PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(CatPool, Filt, GoodsPool);
				ok = 1;
			}
			else if(Filt.CatID) {
				if(!CatPool.GetCount())
					PPMarketplaceInterface_Wildberries::LoadPublicGoodsCategoryList(CatPool, true);
				if(CatPool.GetCount()) {
					PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(CatPool, Filt, GoodsPool);
					ok = 1;
				}
			}
			else if(Filt.BrandID) {
				PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(CatPool, Filt, GoodsPool);
				ok = 1;
			}
			else if(Filt.SupplID) {
				PPMarketplaceInterface_Wildberries::LoadPublicGoodsList(CatPool, Filt, GoodsPool);
				ok = 1;
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int PPViewWbPublicGoods::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 1;
	assert(pBlk->P_SrcData && pBlk->P_DestData); // Функция вызывается только из одной локации и эти members != 0 равно как и pBlk != 0
	SString temp_buf;
	const uint * p_idx = static_cast<const uint *>(pBlk->P_SrcData);
	if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
		const auto * p_entry = PupPool.GetEntryC((*p_idx)-1); // Индексы заданы в диапазоне [1..PupPool.GetCount()]
		assert(p_entry);
		switch(pBlk->ColumnN) {
			case 1: // id
				pBlk->Set(p_entry->ID);
				break;
			case 2: // country
				{
					temp_buf.Z();
					if(p_entry->UedCountry) {
						const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
						if(p_uedc) {
							p_uedc->GetSymb(p_entry->UedCountry, temp_buf);
						}
					}
					pBlk->Set(temp_buf);
				}
				break;
			case 3: // appellation
				PupPool.GetS(p_entry->NameP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 4: // address
				PupPool.GetS(p_entry->AddrP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 5: // workingtime
				PupPool.GetS(p_entry->WorkTimeP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 6: // geocoord
				{
					SGeoPosLL geopos;
					UED::GetRaw_GeoLoc(p_entry->UedGeoLoc, geopos);
					pBlk->Set(geopos.ToStr(temp_buf));
				}
				break;
			case 7: // rating
				pBlk->Set(static_cast<double>(p_entry->Rate));
				break;
			case 8: // fittingroomcount
				pBlk->Set(static_cast<long>(p_entry->FittingRoomCount));
				break;
			case 9: // dest
				pBlk->Set(p_entry->Dest);
				break;
			case 10: // dest3
				pBlk->Set(p_entry->Dest3);
				break;
		}
	}
	else {
		const auto * p_entry = GoodsPool.GetEntryC((*p_idx)-1); // Индексы заданы в диапазоне [1..GoodsPool.GetCount()]
		assert(p_entry);
		switch(pBlk->ColumnN) {
			case 1: // Ид товара
				pBlk->Set(p_entry->ID);
				break;
			case 2: // Наименование товара
				GoodsPool.GetS(p_entry->NameP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 3: // Наименование бренда
				GoodsPool.GetS(p_entry->BrandP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 4: // Имя поставщика
				GoodsPool.GetS(p_entry->SupplP, temp_buf);
				pBlk->Set(temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				break;
			case 5: // Цена
				{
					double price = 0.0;
					if(p_entry->SizeL.getCount()) {
						price = static_cast<double>(p_entry->SizeL.at(0).Price100_Product) / 100.0;
					}
					pBlk->Set(price);
				}
				break;
			case 6: // Рейтинг
				pBlk->Set(static_cast<double>(p_entry->ReviewRating));
				break;
			case 7: // Количество оценок
				pBlk->Set(static_cast<long>(p_entry->FeedbackCount));
				break;
		}
	}
	return ok;
}

int PPViewWbPublicGoods::CmpSortIndexItems(PPViewBrowser * pBrw, const uint * pItem1, const uint * pItem2)
{
	return Implement_CmpSortIndexItems_OnArray(pBrw, pItem1, pItem2);
}

static IMPL_CMPFUNC(PPViewWbPublicGoods, i1, i2)
{
	int    si = 0;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(pExtraData);
	if(p_brw) {
		PPViewWbPublicGoods * p_view = static_cast<PPViewWbPublicGoods *>(p_brw->P_View);
		if(p_view) {
			const uint * p_item1 = static_cast<const uint *>(i1);
			const uint * p_item2 = static_cast<const uint *>(i2);
			si = p_view->CmpSortIndexItems(p_brw, p_item1, p_item2);
		}
	}
	return si;
}

void PPViewWbPublicGoods::PreprocessBrowser(PPViewBrowser * pBrw)
{
	SString temp_buf;
	if(pBrw) {
		pBrw->SetDefUserProc([](SBrowserDataProcBlock * pBlk) -> int
			{
				return (pBlk && pBlk->ExtraPtr) ? static_cast<PPViewWbPublicGoods *>(pBlk->ExtraPtr)->_GetDataForBrowser(pBlk) : 0;				
			}, this);
		//pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
		if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
			pBrw->InsColumn(-1, "@id",            1, T_INT32, ALIGN_RIGHT, BCO_USERPROC);
			pBrw->InsColumn(-1, "@country",       2, MKSTYPE(S_ZSTRING, 32),  0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@appellation",   3, MKSTYPE(S_ZSTRING, 64),  0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@address",       4, MKSTYPE(S_ZSTRING, 200), 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@workingtime",   5, MKSTYPE(S_ZSTRING, 200), 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@geocoord",      6, MKSTYPE(S_ZSTRING, 64),  0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@rating",        7, MKSTYPE(S_FLOAT, 8),     MKSFMTD(0, 1, NMBF_NOZERO), BCO_USERPROC);
			pBrw->InsColumn(-1, "@fittingroomcount", 8, T_INT32,              0, BCO_USERPROC);
			pBrw->InsColumn(-1, "Destination",       9, T_INT64,                ALIGN_RIGHT, BCO_USERPROC);
			pBrw->InsColumn(-1, "Destination3",     10, T_INT64,                ALIGN_RIGHT, BCO_USERPROC);
		}
		else {
			pBrw->InsColumn(-1, "@id",            1, T_INT64, 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@appellation",   2, MKSTYPE(S_ZSTRING, 200), 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@brand",         3, MKSTYPE(S_ZSTRING, 200), 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@supplier",      4, MKSTYPE(S_ZSTRING, 200), 0, BCO_USERPROC);
			pBrw->InsColumn(-1, "@price",         5, T_DOUBLE, MKSFMTD_020, BCO_USERPROC);
			pBrw->InsColumn(-1, "@rating",        6, T_DOUBLE, MKSFMTD(0, 1, 0), BCO_USERPROC);
			pBrw->InsColumn(-1, "@feedbackcount", 7, T_INT32, 0, BCO_USERPROC);
		}
		pBrw->Helper_SetAllColumnsSortable();
	}
}

int PPViewWbPublicGoods::MakeList(PPViewBrowser * pBrw)
{
	int    ok = 1;
	const  bool is_sorting_needed = (pBrw && pBrw->GetSettledOrderList().getCount());
	if(P_DsList)
		P_DsList->clear();
	else
		P_DsList = new TSArray <uint>();
	if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
		for(uint i = 0; i < PupPool.GetCount(); i++) {
			const PPMarketplaceInterface_Wildberries::PickUpPointPool::Entry * p_entry = PupPool.GetEntryC(i);
			if(p_entry) {
				uint   idx = i+1;
				P_DsList->insert(&idx);
			}
		}
	}
	else {
		for(uint i = 0; i < GoodsPool.GetCount(); i++) {
			const PPMarketplaceInterface_Wildberries::PublicWarePool::Entry * p_entry = GoodsPool.GetEntryC(i);
			if(p_entry) {
				uint   idx = i+1;
				P_DsList->insert(&idx);
			}
		}
	}
	if(pBrw) {
		pBrw->Helper_SetAllColumnsSortable();
		if(is_sorting_needed)
			P_DsList->sort(PTR_CMPFUNC(PPViewWbPublicGoods), pBrw);
	}
	//CATCHZOK
	return ok;
}

/*virtual*/SArray * PPViewWbPublicGoods::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new TSArray <uint>();
	THROW(MakeList(0));
	p_array = new SArray(*P_DsList);
	SetExtToolbar(TOOLBAR_WBPUBLICGOODS);
	ASSIGN_PTR(pBrwId, BROWSER_EMPTY);
	if(pSubTitle) {
		PPLoadString("marketplaceexplorer", *pSubTitle);
	}
	CATCH
		ZDELETE(P_DsList);
	ENDCATCH
	return p_array;
}

void PPViewWbPublicGoods::CalcTotal(WbPublicGoodsTotal * pTotal)
{
	WbPublicGoodsTotal total;
	Int64Array brand_list;
	Int64Array vendor_list;
	if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
	}
	else {
		total.CountOnServer = GoodsPool.TotalCountOnServer;
		total.Count = GoodsPool.GetCount();
		for(uint i = 0; i < GoodsPool.GetCount(); i++) {
			const auto * p_entry = GoodsPool.GetEntryC(i);
			if(p_entry) {
				if(p_entry->BrandID)
					brand_list.add(p_entry->BrandID);
				if(p_entry->SupplID)
					vendor_list.add(p_entry->SupplID);
			}
		}
		brand_list.sortAndUndup();
		vendor_list.sortAndUndup();
		total.BrandCount = brand_list.getCount();
		total.VendorCount = vendor_list.getCount();
	}
	ASSIGN_PTR(pTotal, total);
}

/*virtual*/void PPViewWbPublicGoods::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_WBGOODSLISTTOTAL);
	WbPublicGoodsTotal total;
	if(CheckDialogPtrErr(&dlg)) {
		CalcTotal(&total);
		dlg->setCtrlData(CTL_WBGOODSLISTTOTAL_COUNT, &total.Count);
		dlg->setCtrlData(CTL_WBGOODSLISTTOTAL_SCOUNT, &total.CountOnServer);
		dlg->setCtrlData(CTL_WBGOODSLISTTOTAL_BCOUNT, &total.BrandCount);
		dlg->setCtrlData(CTL_WBGOODSLISTTOTAL_VCOUNT, &total.VendorCount);
		ExecViewAndDestroy(dlg);
	}
}

/*virtual*/int PPViewWbPublicGoods::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	uint   idx = pHdr ? *static_cast<const uint *>(pHdr) : 0;
	if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
		;
	}
	else {
		auto * p_entry = GoodsPool.GetEntry(idx-1); // Индексы заданы в диапазоне [1..GoodsPool.GetCount()]
		if(p_entry) {
			SString temp_buf;
			TDialog * dlg = new TDialog(DLG_WBGOODSLISTDETAIL);
			if(CheckDialogPtrErr(&dlg)) {
				// @debug {
				//uint   pic_count = p_entry->PicsCount;
				//TSCollection <SImageBuffer> img_list;
				//PPMarketplaceInterface_Wildberries::LoadPublicGoodsImageList(p_entry->ID, &pic_count/*[INOUT]*/, img_list); 
				// } @debug
				dlg->setCtrlData(CTL_WBGOODSLISTDETAIL_ID, &p_entry->ID);
				GoodsPool.GetS(p_entry->NameP, temp_buf);
				dlg->setCtrlString(CTL_WBGOODSLISTDETAIL_NAME, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				dlg->setCtrlData(CTL_WBGOODSLISTDETAIL_BRANDID, &p_entry->BrandID);
				GoodsPool.GetS(p_entry->BrandP, temp_buf);
				dlg->setCtrlString(CTL_WBGOODSLISTDETAIL_BRAND, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				dlg->setCtrlData(CTL_WBGOODSLISTDETAIL_SUPPLID, &p_entry->SupplID);
				GoodsPool.GetS(p_entry->SupplP, temp_buf);
				dlg->setCtrlString(CTL_WBGOODSLISTDETAIL_SUPPL, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				GoodsPool.GetS(p_entry->EntityP, temp_buf);
				dlg->setCtrlString(CTL_WBGOODSLISTDETAIL_ENT, temp_buf.Transf(CTRANSF_UTF8_TO_INNER));
				ExecViewAndDestroy(dlg);
			}
		}
	}
	return -1;
}

/*virtual*/int PPViewWbPublicGoods::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	uint   idx = pHdr ? *static_cast<const uint *>(pHdr) : 0;
	const  PPID preserve_idx = idx;
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_USERSORT: ok = 1; break; // The rest will be done below
			case PPVCMD_WWWLINK: 
				if(Filt.ViewKind == MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
					const auto * p_entry = PupPool.GetEntryC(idx-1); // Индексы заданы в диапазоне [1..PupPool.GetCount()]
					if(p_entry) {
						// https://www.google.com/maps/place/54.9333108,37.4352159
						if(p_entry->UedGeoLoc) {
							SString temp_buf;
							SGeoPosLL geopos;
							UED::GetRaw_GeoLoc(p_entry->UedGeoLoc, geopos);
							temp_buf.Z().Cat("maps/place").SetLastDSlash().Cat(geopos.Lat, MKSFMTD(0, 7, 0)).Comma().Cat(geopos.Lon, MKSFMTD(0, 7, 0));
							SString url_buf(InetUrl::MkHttps("www.google.com", temp_buf));
							SStringU url_u;
							url_u.CopyFromMb(cpANSI, url_buf, url_buf.Len());
							if(url_u.NotEmpty())
								::ShellExecuteW(0, L"open", url_u, NULL, NULL, SW_SHOWNORMAL);
						}
					}
				}
				else {
					auto * p_entry = GoodsPool.GetEntry(idx-1); // Индексы заданы в диапазоне [1..GoodsPool.GetCount()]
					if(p_entry) {
						//https://www.wildberries.ru/catalog/9020554/detail.aspx
						SString temp_buf;
						(temp_buf = "catalog").SetLastDSlash().Cat(p_entry->ID).SetLastDSlash().Cat("detail").Dot().Cat("aspx");
						SString url_buf(InetUrl::MkHttps("www.wildberries.ru", temp_buf));
						SStringU url_u;
						url_u.CopyFromMb(cpANSI, url_buf, url_buf.Len());
						if(url_u.NotEmpty())
							::ShellExecuteW(0, L"open", url_u, NULL, NULL, SW_SHOWNORMAL);
					}
				}
				break;
			case PPVCMD_SIMILARITY:
				ok = -1;
				if(Filt.ViewKind != MarketplaceGoodsSelectionFilt::vkPickUpPoints) {
					auto * p_entry = GoodsPool.GetEntry(idx-1); // Индексы заданы в диапазоне [1..GoodsPool.GetCount()]
					if(p_entry && pBrw && pBrw->getDefC()) {
						int col_idx = pBrw->GetCurColumn();
						if(col_idx >= 0 && col_idx < pBrw->getDefC()->getCountI()) {
							const BroColumn & r_col = pBrw->getDefC()->at(col_idx);
							Goods2Tbl::Rec goods_rec;
							if(r_col.OrgOffs == 3) { // brand
								if(p_entry->BrandID) {
									MarketplaceGoodsSelectionFilt temp_filt;
									temp_filt.BrandID = p_entry->BrandID;
									return PPView::Execute(PPVIEW_WBPUBLICGOODS, &temp_filt, 0, 0);
								}
							}
							else if(r_col.OrgOffs == 4) { // supplier
								if(p_entry->SupplID) {
									MarketplaceGoodsSelectionFilt temp_filt;
									temp_filt.SupplID = p_entry->SupplID;
									return PPView::Execute(PPVIEW_WBPUBLICGOODS, &temp_filt, 0, 0);
								}
							}
						}
					}
				}
				break;
		}
	}
	if(ok > 0) {
		MakeList(pBrw);
		if(pBrw) {
			AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
			if(p_def) {
				SArray * p_array = new SArray(*P_DsList);
				p_def->setArray(p_array, 0, 1);
				pBrw->setRange(p_array->getCount());
				uint   temp_pos = 0;
				long   update_pos = -1;
				if(preserve_idx > 0 && P_DsList->lsearch(&preserve_idx, &temp_pos, CMPF_LONG))
					update_pos = temp_pos;
				if(update_pos >= 0)
					pBrw->go(update_pos);
				else if(update_pos == MAXLONG)
					pBrw->go(p_array->getCount()-1);
			}
			pBrw->Update();
		}
	}
	return ok;
}

/*
		class Entry { 
		public:
			Entry();
			Entry & Z();
			bool   FromJsonObj(PublicWarePool & rPool, const SJson * pJs);

			int64  ID;
			int32  Wh;
			int64  BrandID;
			int32  SupplID;
			int32  SubjId;
			int32  SubjParentId;
			float  SupplRating;
			uint32 SupplFlags;
			uint   PicsCount;
			float  Rating;
			float  ReviewRating;
			float  NmReviewRating;
			uint   FeedbackCount;
			uint   NmFeedbackCount;
			uint   FeedbackPointCount;
			double TotalStock;
			uint   BrandP;
			uint   NameP;
			uint   EntityP;
			uint   SupplP;

			TSVector <Size> SizeL;
		};
*/ 
