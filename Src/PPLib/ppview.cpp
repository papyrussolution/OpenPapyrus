// PPVIEW.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8 // @v11.0.4
//
#include <pp.h>
#pragma hdrstop

PPView::Rc::Rc() : Id(0), P_ExtraParam(0)
{
}

PPView::Rc & PPView::Rc::Z()
{
	Id = 0;
	Symb.Z();
	Descr.Z();
	P_ExtraParam = 0;
	return *this;
}

/*static*/const PPView::DescriptionList * PPView::P_DL;
/*static*/const PPView::DescriptionList * PPView::P_FL;

/*static*/int PPView::InitializeDescriptionList()
{
	P_DL = new PPView::DescriptionList(PPView::DescriptionList::kView);
	P_FL = new PPView::DescriptionList(PPView::DescriptionList::kFilt);
	if(P_DL && P_DL->IsValid() && P_FL && P_FL->IsValid()) {
		return 1;
	}
	else {
		ZDELETE(P_DL);
		assert(0);
		return 0;
	}
}

/*static*/DBQuery * PPView::CrosstabDbQueryStub = reinterpret_cast<DBQuery *>(0x0001);
//
//
//
PPView::DescriptionList::DescriptionList(int kind) : Kind(kind), Fail(0)
{
	assert(oneof2(Kind, kView, kFilt));
	TVRez * p_rez = P_SlRez;
	THROW(oneof2(Kind, kView, kFilt));
	THROW_PP(p_rez, PPERR_RESFAULT);
	{
		ulong pos = 0;
		Rc rc;
		if(Kind == kFilt) {
			for(uint rsc_id = 0; p_rez->enumResources(PP_RCDECLFILT, &rsc_id, &pos) > 0;) {
				rc.Z();
				if(p_rez->findResource(rsc_id, PP_RCDECLFILT)) {
					rc.Id = rsc_id;
					p_rez->getString(rc.Symb, 0);
					p_rez->getString(rc.Descr, 0);
					InnerEntry ie;
					ie.Id = rc.Id;
					AddS(rc.Symb, &ie.SymbP);
					AddS(rc.Descr, &ie.DescrP);
					ie.P_ExtraParam = 0;
					THROW_SL(L.insert(&ie));
				}				
			}
		}
		else if(Kind == kView) {
			for(uint rsc_id = 0; p_rez->enumResources(PP_RCDECLVIEW, &rsc_id, &pos) > 0;) {
				if(!oneof4(rsc_id, PPVIEW_GOODSGROUP, PPVIEW_REGISTER, PPVIEW_TAG, PPVIEW_BBOARD)) { // Исключаем фиктивные PPView
					rc.Z();
					if(p_rez->findResource(rsc_id, PP_RCDECLVIEW)) {
						rc.Id = rsc_id;
						p_rez->getString(rc.Symb, 0);
						p_rez->getString(rc.Descr, 0);
						InnerEntry ie;
						ie.Id = rc.Id;
						AddS(rc.Symb, &ie.SymbP);
						AddS(rc.Descr, &ie.DescrP);
						ie.P_ExtraParam = 0;
						THROW_SL(L.insert(&ie));
					}
				}
			}
			{
				//
				// Специальные элементы для различных категорий документов
				//
				struct RSVD {
					const BillFilt::FiltExtraParam Ep;
					const char * P_Symb;
					const char * P_Name;
				};
				static const RSVD ep[] = {
					//{ BillFilt::FiltExtraParam(0, bbtGoodsBills), "Bill_Goods", "@{commoditydocument_pl}" },
					{ BillFilt::FiltExtraParam(0, bbtOrderBills), "Bill_Ord", "@{cmd_goodsorder}" },
					{ BillFilt::FiltExtraParam(0, bbtAccturnBills), "Bill_Acc", "@{accountdocument_pl}" },
					{ BillFilt::FiltExtraParam(0, bbtInventoryBills), "Bill_Inv", "@{inventorydocument_pl}" },
					{ BillFilt::FiltExtraParam(0, bbtPoolBills), "Bill_Pool", "@{documentpool_pl}" },
					//{ BillFilt::FiltExtraParam(0, bbtClientDebt), "Bill_CliDebt", "bbtClientDebt" },
					//{ BillFilt::FiltExtraParam(0, bbtClientRPayment), "Bill_CliRPaym", "bbtClientRPayment" },
					{ BillFilt::FiltExtraParam(0, bbtDraftBills), "Bill_Draft", "@{draftdocument_pl}" },
					//{ BillFilt::FiltExtraParam(0, bbtRealTypes), "Bill_Real", "bbtRealTypes" },
					{ BillFilt::FiltExtraParam(0, bbtWmsBills), "Bill_Wms", "@{cmd_wmsbilllist}" },
				};
				for(uint i = 0; i < SIZEOFARRAY(ep); i++) {
					InnerEntry ie;
					ie.Id = (PPVIEW_BILL | (ep[i].Ep.Bbt << 16));
					AddS(ep[i].P_Symb, &ie.SymbP);
					AddS(ep[i].P_Name, &ie.DescrP);
					ie.P_ExtraParam = &ep[i].Ep;
					THROW_SL(L.insert(&ie));
				}
			}
			{
				//
				// @v11.5.0 {
				// Специальные элементы для различный категорий лотов
				//
				struct RSVD {
					const void * P_ExtraParam;
					const char * P_Symb;
					const char * P_Name;
				};
				static const RSVD ep[] = {
					{ 0, "Lot_Regular", "@{lots}" },
					{ reinterpret_cast<const void *>(1), "Lot_Order", "@{orderlots}" },
				};
				for(uint i = 0; i < SIZEOFARRAY(ep); i++) {
					InnerEntry ie;
					ie.Id = (PPVIEW_LOT | (i << 16));
					AddS(ep[i].P_Symb, &ie.SymbP);
					AddS(ep[i].P_Name, &ie.DescrP);
					ie.P_ExtraParam = ep[i].P_ExtraParam; // @attention not address (&ep[i].P_ExtraParam) - strictrly value (ep[i].P_ExtraParam)
					THROW_SL(L.insert(&ie));
				}
				// } @v11.5.0 
			}
		}
		L.sort2(CMPF_LONG, 0);
	}
	CATCH
		Fail = 1;
	ENDCATCH
}

int PPView::DescriptionList::GetById(long id, PPView::Rc & rEntry) const
{
	int    ok = 0;
	rEntry.Z();
	if(!Fail) {
		uint   idx = 0;
		if(L.bsearch(&id, &idx, CMPF_LONG)) {
			const InnerEntry & r_ie = L.at(idx);
			assert(r_ie.Id == id);
			rEntry.Id = r_ie.Id;
			GetS(r_ie.SymbP, rEntry.Symb);
			GetS(r_ie.DescrP, rEntry.Descr);
			rEntry.P_ExtraParam = r_ie.P_ExtraParam;
			ok = 1;
		}
	}
	return ok;
}

int PPView::DescriptionList::GetBySymb(const char * pSymb, PPView::Rc & rEntry) const
{
	int    ok = 0;
	rEntry.Z();
	if(!Fail && !isempty(pSymb)) {
		SString symb;
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const InnerEntry & r_ie = L.at(i);
			if(GetS(r_ie.SymbP, symb) && symb.IsEqiAscii(pSymb)) {
				rEntry.Id = r_ie.Id;
				rEntry.Symb = symb;
				GetS(r_ie.DescrP, rEntry.Descr);
				rEntry.P_ExtraParam = r_ie.P_ExtraParam;
				ok = 1;
			}
		}
	}
	return ok;
}

int PPView::DescriptionList::GetList(bool includeSpecialItems, TSCollection <Rc> & rResult) const
{
	int    ok = 1;
	rResult.freeAll();
	THROW(!Fail);
	for(uint i = 0; i < L.getCount(); i++) {
		const InnerEntry & r_ie = L.at(i);
		if(includeSpecialItems || !(r_ie.Id & 0xffff0000)) {
			Rc * p_new_entry = rResult.CreateNewItem();
			THROW_SL(p_new_entry);
			p_new_entry->Id = r_ie.Id;
			p_new_entry->P_ExtraParam = r_ie.P_ExtraParam;
			GetS(r_ie.SymbP, p_new_entry->Symb);
			GetS(r_ie.DescrP, p_new_entry->Descr);
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPView::GetFilterById(long id, PPView::Rc & rEntry) { return P_FL ? P_FL->GetById(id, rEntry) : 0; }
/*static*/int PPView::GetDescriptionById(long id, PPView::Rc & rEntry) { return P_DL ? P_DL->GetById(id, rEntry) : 0; }
/*static*/const void * PPView::GetDescriptionExtra(long id) 
{ 
	const void * p_result = 0;
	if(P_DL) {
		PPView::Rc entry;
		if(P_DL->GetById(id, entry))
			p_result = entry.P_ExtraParam;
	}
	return p_result;
}
/*static*/int PPView::GetDescriptionBySymb(const char * pSymb, PPView::Rc & rEntry) { return P_DL ? P_DL->GetBySymb(pSymb, rEntry) : 0; }
/*static*/int PPView::GetDescriptionList(bool includeSpecialItems, TSCollection <Rc> & rResult) { return P_DL ? P_DL->GetList(includeSpecialItems, rResult) : 0; }

/*static*/int PPView::GetDescriptionList(bool includeSpecialItems, StrAssocArray * pSymbList, StrAssocArray * pTextList)
{
	int    ok = 1;
	SString temp_buf;
	CALLPTRMEMB(pSymbList, Z());
	CALLPTRMEMB(pTextList, Z());
	TSCollection <Rc> rclist;
	THROW(GetDescriptionList(includeSpecialItems, rclist));
	for(uint i = 0; i < rclist.getCount(); i++) {
		const Rc * p_rc = rclist.at(i);
		if(p_rc) {
			if(pSymbList) {
				THROW_SL(pSymbList->Add(p_rc->Id, p_rc->Symb));
			}
			if(pTextList) {
				temp_buf = p_rc->Descr;
				PPExpandString(temp_buf, CTRANSF_UTF8_TO_INNER); // @v10.1.6
				THROW_SL(pTextList->Add(p_rc->Id, temp_buf));
			}			
		}
	}
	CALLPTRMEMB(pTextList, SortByText());
	CATCHZOK
	return ok;
}

#if 0 // @v11.4.4 {
/*static*/int PPView::LoadResource(int kind, int id, PPView::Rc & rRc)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	rRc.Z();
	rRc.Id = id;
	THROW_PP(p_rez, PPERR_RESFAULT);
	if(kind == 0) {
		THROW(p_rez->findResource(id, PP_RCDECLVIEW));
		p_rez->getString(rRc.Symb, 0);
		p_rez->getString(rRc.Descr, 0);
		PPExpandString(rRc.Descr, CTRANSF_UTF8_TO_OUTER);
	}
	else if(kind == 1) {
		THROW(p_rez->findResource(id, PP_RCDECLFILT));
		p_rez->getString(rRc.Symb, 0);
	}
	else {
		CALLEXCEPT_PP(PPERR_INVPARAM);
	}
	CATCHZOK
	return ok;
}

/*static*/int PPView::GetResourceLists(bool includeSpecialItems, StrAssocArray * pSymbList, StrAssocArray * pTextList)
{
	int    ok = 1;
	CALLPTRMEMB(pSymbList, Z());
	CALLPTRMEMB(pTextList, Z());
	TVRez * p_rez = P_SlRez;
	THROW_PP(p_rez, PPERR_RESFAULT);
	{
		ulong pos = 0;
		Rc rc;
		for(uint rsc_id = 0; p_rez->enumResources(PP_RCDECLVIEW, &rsc_id, &pos) > 0;) {
			if(!oneof4(rsc_id, PPVIEW_GOODSGROUP, PPVIEW_REGISTER, PPVIEW_TAG, PPVIEW_BBOARD)) { // Исключаем фиктивные PPView
				const void * p_extra_init_ptr = 0;
				THROW(LoadResource(0, rsc_id, rc));
				if(pSymbList) {
					THROW_SL(pSymbList->Add(rsc_id, rc.Symb));
				}
				if(pTextList) {
					PPExpandString(rc.Descr, CTRANSF_UTF8_TO_INNER); // @v10.1.6
					THROW_SL(pTextList->Add(rsc_id, rc.Descr));
				}
			}
		}
		// @v11.4.4 {
		if(includeSpecialItems) {
		}
		// } @v11.4.4 
		CALLPTRMEMB(pTextList, SortByText());
	}
	CATCHZOK
	return ok;
}
#endif // } 0 @v11.4.4

/*static*/int FASTCALL PPView::CreateFiltInstance(int filtID, PPBaseFilt ** ppF)
{
	ASSIGN_PTR(ppF, 0);
	Rc     rc;
	// @v11.4.4 return LoadResource(1, filtID, rc) ? CreateFiltInstanceBySymb(rc.Symb, ppF) : 0;
	return GetFilterById(filtID, rc) ? CreateFiltInstanceBySymb(rc.Symb, ppF) : 0; // @v11.4.4
}

/*static*/int FASTCALL PPView::CreateFiltInstanceBySymb(const char * pSymb, PPBaseFilt ** ppF)
{
	int    ok = 0;
	PPBaseFilt * p_filt = 0;
	if(pSymb && pSymb[0]) {
		FN_PPFILT_FACTORY f = reinterpret_cast<FN_PPFILT_FACTORY>(GetProcAddress(SLS.GetHInst(), SLS.AcquireRvlStr().Cat("BFF").CatChar('_').Cat(pSymb)));
		if(f) {
			p_filt = f();
			if(p_filt)
				ok = 1;
		}
		else
			PPSetError(PPERR_PPFILTUNIMPL, pSymb);
	}
	else
		PPSetError(PPERR_VIEWSYMBUNDEF);
	ASSIGN_PTR(ppF, p_filt);
	return ok;
}

struct ViewFiltObjMappingEntry {
	PPID   ObjType;
	int    ViewID;
	int    FiltID;
};

static const ViewFiltObjMappingEntry ViewFiltObjMapping[] = {
	{ PPOBJ_BILL, PPVIEW_BILL, PPFILT_BILL },
	{ PPOBJ_OPRKIND, PPVIEW_OPRKIND, PPFILT_OPRKIND },
	{ PPOBJ_DBDIV, PPVIEW_DBDIV, PPFILT_DBDIV },
	{ PPOBJ_BILL, PPVIEW_BILL, PPFILT_BILL },
	{ PPOBJ_PERSON, PPVIEW_PERSON,  PPFILT_PERSON },
	{ PPOBJ_SCARD, PPVIEW_SCARD,  PPFILT_SCARD },
	{ PPOBJ_ARTICLE, PPVIEW_ARTICLE,  PPFILT_ARTICLE },
	{ PPOBJ_BRAND, PPVIEW_BRAND, PPFILT_BRAND },
	{ PPOBJ_GOODS, PPVIEW_GOODS, PPFILT_GOODS },
	{ PPOBJ_PRJTASK, PPVIEW_PRJTASK, PPFILT_PRJTASK },
	{ PPOBJ_PROJECT, PPVIEW_PROJECT, PPFILT_PROJECT },
	{ PPOBJ_PERSONEVENT, PPVIEW_PERSONEVENT, PPFILT_PERSONEVENT },
	{ PPOBJ_CASHNODE, PPVIEW_CASHNODE, PPFILT_CASHNODE },
	{ PPOBJ_CSESSION, PPVIEW_CSESS, PPFILT_CSESS },
	{ PPOBJ_PROCESSOR, PPVIEW_PROCESSOR, PPFILT_PROCESSOR },
	{ PPOBJ_TECH, PPVIEW_TECH, PPFILT_TECH },
	{ PPOBJ_TSESSION, PPVIEW_TSESSION, PPFILT_TSESSION },
	{ PPOBJ_TIMESERIES, PPVIEW_TIMESERIES, PPFILT_TIMESERIES },
	{ PPOBJ_CCHECK, PPVIEW_CCHECK, PPFILT_CCHECK },
	{ PPOBJ_LOT, PPVIEW_LOT, PPFILT_LOT },
};

int PPGetObjViewFiltMapping_Obj(PPID objType, int * pViewId, int * pFiltId)
{
	for(uint i = 0; i < SIZEOFARRAY(ViewFiltObjMapping); i++) {
		if(ViewFiltObjMapping[i].ObjType == objType) {
			ASSIGN_PTR(pViewId, ViewFiltObjMapping[i].ViewID);
			ASSIGN_PTR(pFiltId, ViewFiltObjMapping[i].FiltID);
			return 1;
		}
	}
	return 0;
}

int PPGetObjViewFiltMapping_View(int viewId, PPID * pObjType, int * pFiltId)
{
	for(uint i = 0; i < SIZEOFARRAY(ViewFiltObjMapping); i++) {
		if(ViewFiltObjMapping[i].ViewID == viewId) {
			ASSIGN_PTR(pObjType, ViewFiltObjMapping[i].ObjType);
			ASSIGN_PTR(pFiltId, ViewFiltObjMapping[i].FiltID);
			return 1;
		}
	}
	return 0;
}

int PPGetObjViewFiltMapping_Filt(int filtId, PPID * pObjType, int * pViewId)
{
	for(uint i = 0; i < SIZEOFARRAY(ViewFiltObjMapping); i++) {
		if(ViewFiltObjMapping[i].FiltID == filtId) {
			ASSIGN_PTR(pObjType, ViewFiltObjMapping[i].ObjType);
			ASSIGN_PTR(pViewId, ViewFiltObjMapping[i].ViewID);
			return 1;
		}
	}
	return 0;
}

/*static*/int FASTCALL PPView::CreateInstance(int viewID, PPView ** ppV) { return CreateInstance(viewID, 0, ppV); }

/*static*/int FASTCALL PPView::CreateInstance(int viewID_, int32 * pSrvInstId, PPView ** ppV)
{
	int    ok = 1;
	const  int base_view_id = (viewID_ & 0x0000ffff); // @v11.4.4
	PPView * p_v = 0;
	int32  srv_inst_id = DEREFPTRORZ(pSrvInstId);
	PPThreadLocalArea & tla = DS.GetTLA();
	if(srv_inst_id) {
		p_v = tla.GetPPViewPtr(srv_inst_id);
		if(!p_v || !p_v->IsConsistent() || p_v->ViewId != viewID_) {
			p_v = 0;
			srv_inst_id = 0;
		}
	}
	if(!p_v) {
		switch(base_view_id) {
			case PPVIEW_TRFRANLZ:        p_v = new PPViewTrfrAnlz;       break;
			case PPVIEW_CCHECK:          p_v = new PPViewCCheck;         break;
			case PPVIEW_STAFFLIST:       p_v = new PPViewStaffList;      break;
			case PPVIEW_STAFFPOST:       p_v = new PPViewStaffPost;      break;
			case PPVIEW_SALARY:          p_v = new PPViewSalary;         break;
			case PPVIEW_LOT:             p_v = new PPViewLot;            break;
			case PPVIEW_PERSONEVENT:     p_v = new PPViewPersonEvent;    break;
			case PPVIEW_PROCESSOR:       p_v = new PPViewProcessor;      break;
			case PPVIEW_GOODSOPANALYZE:  p_v = new PPViewGoodsOpAnalyze; break;
			case PPVIEW_QUOT:            p_v = new PPViewQuot;           break;
			case PPVIEW_PRCBUSY:         p_v = new PPViewPrcBusy;        break;
			case PPVIEW_STAFFCAL:        p_v = new PPViewStaffCal;       break;
			case PPVIEW_SYSJOURNAL:      p_v = new PPViewSysJournal;     break;
			case PPVIEW_GOODS:           p_v = new PPViewGoods;          break;
			case PPVIEW_GOODSREST:       p_v = new PPViewGoodsRest;      break;
			case PPVIEW_BILL:            p_v = new PPViewBill;           break;
			case PPVIEW_PRJTASK:         p_v = new PPViewPrjTask;        break;
			case PPVIEW_OBJSYNC:         p_v = new PPViewObjSync;        break;
			case PPVIEW_PERSON:          p_v = new PPViewPerson;         break;
			case PPVIEW_OBJSYNCCMP:      p_v = new PPViewObjSyncCmp;     break;
			case PPVIEW_OBJSYNCQUEUE:    p_v = new PPViewObjSyncQueue;   break;
			case PPVIEW_SSTAT:           p_v = new PPViewSStat;          break;
			case PPVIEW_SCARD:           p_v = new PPViewSCard;          break;
			case PPVIEW_SCARDOP:         p_v = new PPViewSCardOp;        break;
			case PPVIEW_FREIGHT:         p_v = new PPViewFreight;        break;
			case PPVIEW_OPGROUPING:      p_v = new PPViewOpGrouping;     break;
			case PPVIEW_PERSONREL:       p_v = new PPViewPersonRel;      break;
			case PPVIEW_CSESSEXC:        p_v = new PPViewCSessExc;       break;
			case PPVIEW_DEBTTRNOVR:      p_v = new PPViewDebtTrnovr;     break;
			case PPVIEW_PAYMPLAN:        p_v = new PPViewPaymPlan;       break;
			case PPVIEW_PROJECT:         p_v = new PPViewProject;        break;
			case PPVIEW_GOODSSTRUC:      p_v = new PPViewGoodsStruc;     break;
			case PPVIEW_TSESSION:        p_v = new PPViewTSession;       break;
			case PPVIEW_TSESSANLZ:       p_v = new PPViewTSessAnlz;      break;
			case PPVIEW_CURRATE:         p_v = new PPViewCurRate;        break;
			case PPVIEW_OBJLIKENESS:     p_v = new PPViewObjLikeness;    break;
			case PPVIEW_GOODSTOOBJASSOC: p_v = new PPViewGoodsToObjAssoc; break;
			case PPVIEW_QCERT:           p_v = new PPViewQCert;          break;
			case PPVIEW_ARTICLE:         p_v = new PPViewArticle;        break;
			case PPVIEW_ACCANLZ:         p_v = new PPViewAccAnlz;        break;
			case PPVIEW_PRICELIST:       p_v = new PPViewPriceList;      break;
			case PPVIEW_VATBOOK:         p_v = new PPViewVatBook;        break;
			case PPVIEW_CSESS:           p_v = new PPViewCSess;          break;
			case PPVIEW_PRICEANLZ:       p_v = new PPViewPriceAnlz;      break;
			case PPVIEW_SCALE:           p_v = new PPViewScale;          break;
			case PPVIEW_CASHNODE:        p_v = new PPViewCashNode;       break;
			case PPVIEW_REPORT:          p_v = new PPViewReport;         break;
			case PPVIEW_LOGSMONITOR:	 p_v = new PPViewLogsMonitor;	break;
			case PPVIEW_BIZSCORE:        p_v = new PPViewBizScore;       break;
			case PPVIEW_BIZSCOREVAL:     p_v = new PPViewBizScoreVal;    break;
			case PPVIEW_PREDICTSALES:    p_v = new PPViewPredictSales;   break;
			case PPVIEW_GOODSTAXANALYZE: p_v = new PPViewGoodsTaxAnalyze; break;
			case PPVIEW_DEBTORSTAT:      p_v = new PPViewDebtorStat;     break;
			case PPVIEW_ACCTURN:         p_v = new PPViewAccturn;        break;
			case PPVIEW_ACCOUNT:         p_v = new PPViewAccount;        break;
			case PPVIEW_LOCTRANSF:       p_v = new PPViewLocTransf;      break;
			case PPVIEW_PALM:            p_v = new PPViewPalm;           break;
			case PPVIEW_SERVERSTAT:      p_v = new PPViewServerStat;     break;
			case PPVIEW_ASSET:           p_v = new PPViewAsset;          break;
			case PPVIEW_SHIPMANALYZE:    p_v = new PPViewShipmAnalyze;   break;
			case PPVIEW_TRANSPORT:       p_v = new PPViewTransport;      break;
			case PPVIEW_GOODSBILLCMP:    p_v = new PPViewGoodsBillCmp;   break;
			case PPVIEW_AMOUNTTYPE:      p_v = new PPViewAmountType;     break;
			case PPVIEW_REGISTERTYPE:    p_v = new PPViewRegisterType;   break;
			case PPVIEW_GOODSMOV:        p_v = new PPViewGoodsMov;       break;
			case PPVIEW_LOTOP:           p_v = new PPViewLotOp;          break;
			case PPVIEW_LOTEXTCODE:      p_v = new PPViewLotExtCode;     break;
			case PPVIEW_BUDGET:          p_v = new PPViewBudget;         break;
			case PPVIEW_BIZSCTEMPL:      p_v = new PPViewBizScTempl;     break;
			case PPVIEW_BIZSCVALBYTEMPL: p_v = new PPViewBizScValByTempl; break;
			case PPVIEW_CHECKOPJRNL:     p_v = new PPViewCheckOpJrnl;    break;
			case PPVIEW_GOODSMOV2:       p_v = new PPViewGoodsMov2;      break;
			case PPVIEW_INVENTORY:       p_v = new PPViewInventory;      break;
			case PPVIEW_STOCKOPT:        p_v = new PPViewStockOpt;       break;
			case PPVIEW_MRPTAB:          p_v = new PPViewMrpTab;         break;
			case PPVIEW_MRPLINE:         p_v = new PPViewMrpLine;        break;
			case PPVIEW_LINKEDBILL:      p_v = new PPViewLinkedBill;     break;
			case PPVIEW_BALANCE:         p_v = new PPViewBalance;        break;
			case PPVIEW_DIALOG:          p_v = new PPViewDialog;         break;
			case PPVIEW_DVCLOADINGSTAT:  p_v = new PPViewDvcLoadingStat; break;
			case PPVIEW_SPECSERIES:      p_v = new PPViewSpecSeries;     break;
			case PPVIEW_GTAJOURNAL:      p_v = new PPViewGtaJournal;     break;
			case PPVIEW_GLOBALUSERACC:   p_v = new PPViewGlobalUserAcc;  break;
			case PPVIEW_TECH:            p_v = new PPViewTech;           break;
			case PPVIEW_DBDIV:           p_v = new PPViewDBDiv;          break;
			case PPVIEW_SUPRWARE:        p_v = new PPViewSuprWare;       break;
			// case PPVIEW_DBMONITOR:      p_v = new PPViewDBMonitor;      break; // @v7.x.x
			case PPVIEW_USERPROFILE:     p_v = new PPViewUserProfile();  break;
			case PPVIEW_JOB:             p_v = new PPViewJob();          break;
			case PPVIEW_GEOTRACKING:     p_v = new PPViewGeoTracking();  break;
			case PPVIEW_OPRKIND:         p_v = new PPViewOprKind();      break;
			case PPVIEW_PHNSVCMONITOR:   p_v = new PPViewPhnSvcMonitor(); break;
			case PPVIEW_VETISDOCUMENT:   p_v = new PPViewVetisDocument(); break; // @v10.0.12
			case PPVIEW_TIMESERIES:      p_v = new PPViewTimeSeries();    break; // @v10.4.4
			case PPVIEW_TIMSERDETAIL:    p_v = new PPViewTimSerDetail();  break; // @v10.5.4
			case PPVIEW_BRAND:           p_v = new PPViewBrand();         break; // @v10.4.7
			case PPVIEW_QUOTEREQANALYZE: p_v = new PPViewQuoteReqAnalyze(); break; // @v10.5.7
			case PPVIEW_EVENT:           p_v = new PPViewEvent(); break; // @v10.8.9
			case PPVIEW_USERMENU:        p_v = new PPViewUserMenu(); break; // @v10.9.3 
			case PPVIEW_ALCODECLRU:      p_v = new PPViewAlcoDeclRu(); break; // @v11.0.5
			case PPVIEW_STYLOQBINDERY:   p_v = new PPViewStyloQBindery(); break; // @v11.1.7
			case PPVIEW_STYLOQCOMMAND:   p_v = new PPViewStyloQCommand(); break; // @v11.1.9
			default: ok = PPSetError(PPERR_UNDEFVIEWID);
		}
		if(p_v) {
			p_v->ViewId = viewID_; // @v11.4.4
			if(p_v->Symb.IsEmpty()) {
				Rc rc;
				// @v11.4.4 if(LoadResource(0, viewID, rc))
				if(GetDescriptionById(viewID_, rc)) // @v11.4.4
					p_v->Symb = rc.Symb;
			}
		}
	}
	if(p_v && pSrvInstId) {
		SETIFZ(srv_inst_id, tla.CreatePPViewPtr(p_v));
	}
	ASSIGN_PTR(pSrvInstId, srv_inst_id);
	ASSIGN_PTR(ppV, p_v);
	return ok;
}

/*static*/int FASTCALL PPView::Helper_Execute(int viewID, const PPBaseFilt * pFilt, int flags, PPView ** ppResult, void * extraPtr)
{
	int    ok = 1;
	int    view_in_use = 0;
	const  int modeless = GetModelessStatus(BIN(flags & exefModeless));
	PPViewBrowser * p_prev_win = 0;
	PPBaseFilt * p_flt = 0;
	PPView * p_v = 0;
	THROW(PPView::CreateInstance(viewID, &p_v));
	THROW(p_flt = p_v->CreateFilt(extraPtr));
	if(modeless)
		p_prev_win = static_cast<PPViewBrowser *>(PPFindLastBrowser());
	if(pFilt) {
		THROW(p_flt->Copy(pFilt, 1));
	}
	else if(p_prev_win) {
		THROW(p_flt->Copy(p_prev_win->P_View->GetBaseFilt(), 1));
	}
	else if(p_v->ImplementFlags & PPView::implDontEditNullFilter)
		pFilt = p_flt;
	while(pFilt || p_v->EditBaseFilt(p_flt) > 0) {
		PPWaitStart();
		THROW(p_v->Helper_Init(p_flt, flags));
		PPCloseBrowser(p_prev_win);
		THROW(p_v->Browse(modeless));
		if(modeless || pFilt) {
			view_in_use = 1;
			break;
		}
	}
	CATCHZOKPPERR
	if(!modeless || !ok || !view_in_use) {
		ZDELETE(p_v);
	}
	delete p_flt;
	ASSIGN_PTR(ppResult, p_v);
	return ok;
}

/*static*/int FASTCALL PPView::Execute(int viewID, const PPBaseFilt * pFilt, int flags /* exefXXX */, PPView ** ppResult, void * extraPtr)
	{ return Helper_Execute(viewID, pFilt, flags, ppResult, extraPtr); }
/*static*/int FASTCALL PPView::Execute(int viewID, const PPBaseFilt * pFilt, int flags, void * extraPtr)
	{ return Helper_Execute(viewID, pFilt, flags, 0, extraPtr); }

PPObject * PPView::GetObj() const { return P_Obj; }
//
//
//
PPViewDisplayExtList::PPViewDisplayExtList() : SStrGroup()
{
}

PPViewDisplayExtList & PPViewDisplayExtList::Z()
{
	L.clear();
	SStrGroup::ClearS();
	return *this;
}

int FASTCALL PPViewDisplayExtList::IsEq(const PPViewDisplayExtList & rS) const
{
	int    ok = 0;
	uint   c = L.getCount();
	if(c == rS.L.getCount()) {
		ok = 1;
		SString t1, t2;
		for(uint i = 0; ok && i < c; i++) {
			const InnerItem & r1 = L.at(i);
			const InnerItem & r2 = rS.L.at(i);
			if(r1.DataId != r2.DataId)
				ok = 0;
			else if(r1.Position != r2.Position)
				ok = 0;
			else {
				GetS(r1.TitleP, t1);
				GetS(r2.TitleP, t2);
				if(t1 != t2)
					ok = 0;
			}
		}
	}
	return ok;
}

int PPViewDisplayExtList::SearchItem(int dataId, uint * pPos) const
{
	uint   pos = 0;
	int    ok = L.lsearch(&dataId, &pos, CMPF_LONG);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int PPViewDisplayExtList::SetItem(int dataId, int position, const char * pTitle)
{
	int    ok = 0;
	uint   pos = 0;
	if(SearchItem(dataId, &pos)) {
		InnerItem & r_item = L.at(pos);
		r_item.Position = position;
		AddS(pTitle, &r_item.TitleP);
		ok = 2;
	}
	else {
		InnerItem new_item;
		MEMSZERO(new_item);
		new_item.DataId = dataId;
		new_item.Position = position;
		AddS(pTitle, &new_item.TitleP);
		L.insert(&new_item);
		ok = 1;
	}
	return ok;
}

int PPViewDisplayExtList::GetItemByPos(uint pos, PPViewDisplayExtItem * pItem) const
{
	int    ok = 0;
	if(pos < L.getCount()) {
		const InnerItem & r_item = L.at(pos);
		if(pItem) {
			pItem->DataId = r_item.DataId;
			pItem->Position = r_item.Position;
			GetS(r_item.TitleP, pItem->Title);
		}
		ok = 1;
	}
	return ok;
}

int PPViewDisplayExtList::GetItemByDataId(int dataId, PPViewDisplayExtItem * pItem) const
{
	int    ok = 0;
	uint   pos = 0;
	if(SearchItem(dataId, &pos)) {
		const InnerItem & r_item = L.at(pos);
		if(pItem) {
			pItem->DataId = r_item.DataId;
			pItem->Position = r_item.Position;
			GetS(r_item.TitleP, pItem->Title);
		}
		ok = 1;
	}
	return ok;
}

int PPViewDisplayExtList::RemoveItem(int dataId)
{
	int    ok = 0;
	uint   pos = 0;
	if(SearchItem(dataId, &pos)) {
		L.atFree(pos);
		ok = 1;
	}
	return ok;
}

int PPViewDisplayExtList::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		if(p_pack_handle) {
			const uint c = L.getCount();
			for(uint i = 0; i < c; i++) {
				InnerItem & r_item = L.at(i);
				Pack_Replace(p_pack_handle, r_item.TitleP);
			}
			Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int PPViewDisplayExtList::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Pack();
	}
	THROW_SL(pCtx->Serialize(dir, &L, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

int FASTCALL PPViewDisplayExtList::Write(SBuffer & rBuf) const
{
	int    ok = 1;
	// (Вызов не константный - придется обойтись без упаковки или вызывать ее как-то заранее) Pack();
	THROW_SL(rBuf.Write(&L, SBuffer::ffAryCount32));
	THROW_SL(SStrGroup::WriteS(rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL PPViewDisplayExtList::Read(SBuffer & rBuf)
{
	int    ok = 1;
	THROW_SL(rBuf.Read(&L, SBuffer::ffAryCount32));
	THROW_SL(SStrGroup::ReadS(rBuf));
	CATCHZOK
	return ok;
}
//
//
//
IMPL_INVARIANT_C(PPBaseFilt)
{
	S_INVARIANT_PROLOG(pInvP);
	S_ASSERT_P(*reinterpret_cast<const long *>(this) != 0, pInvP); // Проверка на целостность указателя на таблицу виртуальных функций
	S_ASSERT_P(Signature > 0, pInvP);
	//
	// Порожденный класс не имеет права устанавливать смещение "плоского" участка в пределах базового класса
	//
	S_ASSERT_P(FlatSize == 0 || FlatOffs >= sizeof(*this), pInvP);
	S_INVARIANT_EPILOG(pInvP);
}

/*static*/const char * PPBaseFilt::P_TextSignature = "#TFD";

PPBaseFilt::PPBaseFilt(long signature, long capability, int32 ver) : BranchList(sizeof(Branch)),
	Signature(signature), Capability(capability), Ver(ver), FlatOffs(0), FlatSize(0)
{
}

PPBaseFilt::~PPBaseFilt()
{
	const uint blc = BranchList.getCount();
	if(blc) {
		for(uint i = 0; i < blc; i++) {
			const Branch * p_b = static_cast<const Branch *>(BranchList.at(i));
			if(p_b->Type == Branch::tBaseFiltPtr) {
				PPBaseFilt ** pp_filt = reinterpret_cast<PPBaseFilt **>(PTR8(this) + p_b->Offs);
				ZDELETEFAST(*pp_filt);
			}
		}
	}
	Signature = 0; // Очищаем сигнатуру для того, чтобы можно было идентифицировать указатель на разрущенный объект
}

int PPBaseFilt::Describe(long flags, SString & rBuf) const
{
	rBuf.Z();
	return 1;
}

/*static*/void STDCALL PPBaseFilt::PutObjMembToBuf(PPID objType, PPID objID, const char * pMembName, SString & rBuf)
{
	if(objID) {
		SString & r_obj_name = SLS.AcquireRvlStr(); // @v10.5.4 SLS.AcquireRvlStr()
		if(GetObjectName(objType, objID, r_obj_name) > 0)
			PutMembToBuf(r_obj_name, pMembName, rBuf);
	}
}

/*static*/void STDCALL PPBaseFilt::PutObjMembListToBuf(PPID objType, const ObjIdListFilt * pList, const char * pMembName, SString & rBuf)
{
	if(pList && !pList->IsEmpty()) {
		const PPIDArray & r_list = pList->Get();
		const uint _count = r_list.getCount();
		if(_count) {
			SString buf, obj_name;
			for(uint i = 0; i < _count; i++) {
				PPID obj_id = r_list.at(i);
				obj_name.Z();
				if(obj_id && GetObjectName(objType, obj_id, obj_name) > 0)
					buf.Cat(obj_name);
				if(i != _count-1)
					buf.Comma();
			}
			PutMembToBuf(buf, pMembName, rBuf);
		}
	}
}

/*static*/void STDCALL PPBaseFilt::PutFlagsMembToBuf(const StrAssocArray * pFlagList, const char * pMembName, SString & rBuf)
{
	const uint count = pFlagList ? pFlagList->getCount() : 0;
	if(count) {
		SString buf;
		for(uint i = 0; i < count; i++) {
			StrAssocArray::Item item = pFlagList->Get(i);
			buf.Cat(item.Txt);
			if(i != count - 1)
				buf.Comma();
		}
		PutMembToBuf(buf, pMembName, rBuf);
	}
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(const SString & rParam, const char * pMembName, SString & rBuf)
{
	if(rParam.Len() && !isempty(pMembName))
		rBuf.Cat(pMembName).Eq().Cat(rParam).Semicol();
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(const char * pParam, const char * pMembName, SString & rBuf)
{
	if(!isempty(pMembName) && !isempty(pParam))
		rBuf.Cat(pMembName).Eq().Cat(pParam).Semicol();
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(LDATE param, const char * pMembName, SString & rBuf)
{
	if(param != ZERODATE) {
		SString & r_buf = SLS.AcquireRvlStr();
		PutMembToBuf(r_buf.Cat(param), pMembName, rBuf);
	}
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(const DateRange * pParam, const char * pMembName, SString & rBuf)
{
	if(pParam && !pParam->IsZero()) {
		SString & r_buf = SLS.AcquireRvlStr();
		PutMembToBuf(r_buf.Cat(*pParam), pMembName, rBuf);
	}
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(const RealRange * pParam,  const char * pMembName, SString & rBuf)
{
	if(pParam && !pParam->IsZero()) {
		SString & r_buf = SLS.AcquireRvlStr();
		PutMembToBuf(r_buf.Cat(pParam->low).Dot().Dot().Cat(pParam->upp), pMembName, rBuf);
	}
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(double param, const char * pMembName, SString & rBuf)
{
	SString & r_buf = SLS.AcquireRvlStr();
	PutMembToBuf(r_buf.Cat(param), pMembName, rBuf);
}

/*static*/void STDCALL PPBaseFilt::PutMembToBuf(long param, const char * pMembName, SString & rBuf)
{
	SString & r_buf = SLS.AcquireRvlStr();
	PutMembToBuf(r_buf.Cat(param), pMembName, rBuf);
}

void PPBaseFilt::PutSggMembToBuf(SubstGrpGoods sgg, const char * pMembName, SString & rBuf) const
{
	/* @v11.3.1 struct SggStruc { uint32 SggID; const char * P_Name; };*/
#define __ITEM(f) {f, #f}
	static const /*SggStruc*/SIntToSymbTabEntry SggStrucList[] = {
		__ITEM(sggNone),
		__ITEM(sggGeneric),
		__ITEM(sggGroup),
		__ITEM(sggManuf),
		__ITEM(sggDimX),
		__ITEM(sggDimY),
		__ITEM(sggDimZ),
		__ITEM(sggDimW),
		__ITEM(sggClsKind),
		__ITEM(sggClsGrade),
		__ITEM(sggClsAddObj),
		__ITEM(sggClsKind_Grade),
		__ITEM(sggSuppl),
		__ITEM(sggBrand),
		__ITEM(sggType),
		__ITEM(sggClsKind_Grade_AddObj),
		__ITEM(sggClsKind_AddObj_Grade),
		__ITEM(sggSupplAgent),
		__ITEM(sggLocation),
		__ITEM(sggBrandOwner),
		__ITEM(sggGroupSecondLvl)
	};
#undef __ITEM
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(SIntToSymbTab_GetSymb(SggStrucList, SIZEOFARRAY(SggStrucList), sgg, r_temp_buf) > 0)
		PutMembToBuf(r_temp_buf, pMembName, rBuf);
	/* @v11.3.1 for(uint i = 0; i < SIZEOFARRAY(SggStrucList); i++)
		if(SggStrucList[i].SggID == sgg) {
			PutMembToBuf(SggStrucList[i].P_Name, pMembName, rBuf);
			break;
		}*/
}

void PPBaseFilt::PutSgpMembToBuf(SubstGrpPerson sgp, const char * pMembName, SString & rBuf) const
{
	/* @v11.3.1 struct SgpStruc { uint32 SgpID; const char * P_Name; };*/
#define __ITEM(f) {f, #f}
	static const /*SgpStruc*/SIntToSymbTabEntry SgpStrucList[] = {
		__ITEM(sgpNone),
		__ITEM(sgpCity),
		__ITEM(sgpRegion),
		__ITEM(sgpCountry),
		__ITEM(sgpCategory),
		__ITEM(sgpBillAgent),
		__ITEM(sgpVesselAgent),
		__ITEM(sgpAccSheet),
		__ITEM(sgpFirstRelation),
		__ITEM(sgpArticleMask)
	};
#undef __ITEM
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(SIntToSymbTab_GetSymb(SgpStrucList, SIZEOFARRAY(SgpStrucList), sgp, r_temp_buf) > 0)
		PutMembToBuf(r_temp_buf, pMembName, rBuf);
	/* @v11.3.1 for(uint i = 0; i < SIZEOFARRAY(SgpStrucList); i++)
		if(SgpStrucList[i].SgpID == sgp) {
			PutMembToBuf(SgpStrucList[i].P_Name, pMembName, rBuf);
			break;
		}*/
}

void PPBaseFilt::PutSgdMembToBuf(SubstGrpDate sgd, const char * pMembName, SString & rBuf) const
{
	/* @v11.3.1 struct SgdStruc { uint32 SgpID; const char * P_Name; }; */
#define __ITEM(f) {f, #f}
	static const /*SgdStruc*/SIntToSymbTabEntry SgdStrucList[] = {
		__ITEM(sgdNone),
		__ITEM(sgdWeek),
		__ITEM(sgdMonth),
		__ITEM(sgdQuart),
		__ITEM(sgdYear),
		__ITEM(sgdHour), // @v11.3.1
		__ITEM(sgdWeekDay) // @v11.3.1
	};
#undef __ITEM
	SString & r_temp_buf = SLS.AcquireRvlStr();
	if(SIntToSymbTab_GetSymb(SgdStrucList, SIZEOFARRAY(SgdStrucList), sgd, r_temp_buf) > 0)
		PutMembToBuf(r_temp_buf, pMembName, rBuf);
	/* @v11.3.1 for(uint i = 0; i < SIZEOFARRAY(SgdStrucList); i++)
		if(SgdStrucList[i].SgpID == sgd) {
			PutMembToBuf(SgdStrucList[i].P_Name, pMembName, rBuf);
			break;
		}*/
}

PPBaseFilt & FASTCALL PPBaseFilt::operator = (const PPBaseFilt & s)
{
	Copy(&s, 0);
	return *this;
}

void PPBaseFilt::SetFlatChunk(size_t offs, size_t size)
{
	FlatOffs = static_cast<uint16>(offs);
	FlatSize = static_cast<uint16>(size);
}

int FASTCALL PPBaseFilt::CheckBranchOffs(size_t offs)
{
	assert(offs >= sizeof(*this));
	assert(!FlatOffs || offs < FlatOffs || offs >= static_cast<size_t>(FlatOffs + FlatSize));
	return 1;
}

int PPBaseFilt::AddBranch(uint type, size_t offs, int32 extraId)
{
	Branch b;
	b.Type = type;
	b.Offs = static_cast<uint16>(offs);
	b.ExtraId = extraId;
	return BranchList.insert(&b) ? 1 : PPSetErrorSLib();
}

int FASTCALL PPBaseFilt::SetBranchSString(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tSString, offs, 0) : 0; }
int FASTCALL PPBaseFilt::SetBranchSArray(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tSArray, offs, 0) : 0; }
int FASTCALL PPBaseFilt::SetBranchSVector(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tSVector, offs, 0) : 0; }
int FASTCALL PPBaseFilt::SetBranchObjIdListFilt(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tObjIdListFilt, offs, 0) : 0; }
int FASTCALL PPBaseFilt::SetBranchStrAssocArray(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tStrAssocArray, offs, 0) : 0; }
int PPBaseFilt::SetBranchBaseFiltPtr(int filtID, size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tBaseFiltPtr, offs, filtID) : 0; }
int FASTCALL PPBaseFilt::SetBranchDisplayExtList(size_t offs)
	{ return CheckBranchOffs(offs) ? AddBranch(Branch::tDisplayExtList, offs, 0) : 0; }

int FASTCALL PPBaseFilt::IsA(const PPBaseFilt * pS) const
{
	return (pS && pS->Signature == Signature) ? 1 : PPSetError(PPERR_FILTNEQTYPE);
}

long  PPBaseFilt::GetSignature() const { return Signature; }
int32 PPBaseFilt::GetVer() const { return Ver; }

int PPBaseFilt::Init(int fullyDestroy, long extraData)
{
	if(FlatSize)
		memzero(PTR8(this) + FlatOffs, FlatSize);
	for(uint i = 0; i < BranchList.getCount(); i++) {
		const Branch * p_b = static_cast<Branch *>(BranchList.at(i));
		if(p_b->Type == Branch::tSString)
			reinterpret_cast<SString *>(PTR8(this) + p_b->Offs)->Z();
		else if(p_b->Type == Branch::tSArray)
			reinterpret_cast<SArray *>(PTR8(this) + p_b->Offs)->freeAll();
		else if(p_b->Type == Branch::tSVector)
			reinterpret_cast<SVector *>(PTR8(this) + p_b->Offs)->freeAll();
		else if(p_b->Type == Branch::tObjIdListFilt)
			reinterpret_cast<ObjIdListFilt *>(PTR8(this) + p_b->Offs)->Set(0);
		else if(p_b->Type == Branch::tStrAssocArray)
			reinterpret_cast<StrAssocArray *>(PTR8(this) + p_b->Offs)->Z();
		else if(p_b->Type == Branch::tDisplayExtList)
			reinterpret_cast<PPViewDisplayExtList *>(PTR8(this) + p_b->Offs)->Z();
		else if(p_b->Type == Branch::tBaseFiltPtr) {
			PPBaseFilt ** pp_filt = reinterpret_cast<PPBaseFilt **>(PTR8(this) + p_b->Offs);
			if(*pp_filt) {
				if(fullyDestroy) {
					ZDELETE(*pp_filt);
				}
				else {
					(*pp_filt)->Init(fullyDestroy, 0);
				}
			}
		}
	}
	return -1;
}

static const char * P_FiltTag = "PPFILT";

/*static*/int FASTCALL PPView::WriteFiltPtr(SBuffer & rBuf, const PPBaseFilt * pFilt)
{
	int    ok = 1;
	SString temp_buf;
	rBuf.Write(temp_buf.CatTagBrace(P_FiltTag, 0));
	if(pFilt) {
		long   filt_id = pFilt->GetSignature();
		Rc     rc;
		// @v11.4.4 if(LoadResource(1, filt_id, rc))
		if(GetFilterById(filt_id, rc)) // @v11.4.4
			rBuf.Write(rc.Symb);
		else
			rBuf.Write(temp_buf.Z().Cat(filt_id));
		pFilt->Write(rBuf, 0);
	}
	rBuf.Write(temp_buf.Z().CatTagBrace(P_FiltTag, 1));
	return ok;
}

/*static*/int FASTCALL PPView::ReadFiltPtr(SBuffer & rBuf, PPBaseFilt ** ppFilt)
{
	int    ok = -1;
	int    tag_kind = 0;
	SString temp_buf, tag;
	if(rBuf.GetAvailableSize()) {
		rBuf.Read(temp_buf);
		SStrScan scan(temp_buf);
		THROW_PP(scan.GetTagBrace(tag, &tag_kind) && tag_kind == 0 && tag.Cmp(P_FiltTag, 0) == 0, PPERR_PPFILTREADFAULT);
		rBuf.Read(temp_buf);
		if(scan.GetTagBrace(tag, &tag_kind)) {
			//
			// empty filter
			//
			THROW_PP(tag_kind == 1 && tag.Cmp(P_FiltTag, 0) == 0, PPERR_PPFILTREADFAULT);
			ZDELETE(*ppFilt);
		}
		else {
			if((*ppFilt) == 0) {
				//
				// В буфере temp_buf находится либо символ фильтра либо его цифровая сигнатура
				//
				long   filt_id = temp_buf.ToLong();
				if(filt_id) {
					THROW(PPView::CreateFiltInstance(filt_id, ppFilt));
				}
				else {
					THROW(PPView::CreateFiltInstanceBySymb(temp_buf, ppFilt));
				}
			}
			THROW((*ppFilt)->Read(rBuf, 0));
			rBuf.Read(temp_buf);
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPBaseFilt::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = -1;
	if(dir > 0) {
		ok = Write(rBuf, 0);
	}
	else if(dir < 0) {
		ok = Read(rBuf, 0);
	}
	return ok;
}

int PPBaseFilt::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	SString temp_buf;
	THROW_SL(rBuf.Write(Signature));
	THROW_SL(rBuf.Write(Capability));
	THROW_SL(rBuf.Write(Ver));
	THROW(!FlatSize || rBuf.Write(PTR8C(this) + FlatOffs, FlatSize));
	for(uint i = 0; i < BranchList.getCount(); i++) {
		const Branch * p_b = reinterpret_cast<const Branch *>(BranchList.at(i));
		if(p_b->Type == Branch::tSString) {
			const SString * p_str = reinterpret_cast<const SString *>(PTR8C(this) + p_b->Offs);
			THROW_SL(rBuf.Write(*p_str));
		}
		else if(p_b->Type == Branch::tSArray) {
			const SArray * p_ary = reinterpret_cast<const SArray *>(PTR8C(this) + p_b->Offs);
			THROW_SL(rBuf.Write(p_ary, 0));
		}
		else if(p_b->Type == Branch::tSVector) {
			const SVector * p_ary = reinterpret_cast<const SVector *>(PTR8C(this) + p_b->Offs);
			THROW_SL(rBuf.Write(p_ary, 0));
		}
		else if(p_b->Type == Branch::tObjIdListFilt) {
			const ObjIdListFilt * p_list = reinterpret_cast<const ObjIdListFilt *>(PTR8C(this) + p_b->Offs);
			THROW_SL(p_list->Write(rBuf));
		}
		else if(p_b->Type == Branch::tStrAssocArray) {
			const StrAssocArray * p_ary = reinterpret_cast<const StrAssocArray *>(PTR8C(this) + p_b->Offs);
			THROW_SL(p_ary->Write(rBuf, 0));
		}
		else if(p_b->Type == Branch::tDisplayExtList) {
			const PPViewDisplayExtList * p_list = reinterpret_cast<const PPViewDisplayExtList *>(PTR8C(this) + p_b->Offs);
			THROW(p_list->Write(rBuf));
		}
		else if(p_b->Type == Branch::tBaseFiltPtr) {
			const PPBaseFilt ** pp_filt = (const PPBaseFilt **)(PTR8C(this) + p_b->Offs);
			THROW(PPView::WriteFiltPtr(rBuf, *pp_filt));
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPBaseFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
	return -1;
}

int PPBaseFilt::Read(SBuffer & rBuf, long extraParam)
{
	const uint16 flat_offs = FlatOffs;
	const uint16 flat_size = FlatSize;
	int    ok = 1;
	//
	// В случае ошибки считывания необходимо будет восстановить оригинальные значения Ver, Signature и Capability
	// из-за того, что эти факторы инициализируются терминальным классом и не могут быть реставрированы
	// методами базового класса.
	//
	const  int32  preserve_ver = Ver;
	const  long   preserve_signature = Signature;
	const  long   preserve_capability = Capability;
	SString temp_buf;
	union {
		char   text[4];
		long   bin;
	} _signature;
	const  size_t preserve_rd_offs = rBuf.GetRdOffs();
	THROW_SL(rBuf.Read(_signature.bin));
	if(strncmp(_signature.text, P_TextSignature, sizeof(Signature)) == 0) {
		THROW_SL(rBuf.ReadTermStr(0, temp_buf));
		int    r = ReadText(temp_buf, extraParam);
		THROW(r);
		THROW_PP(r > 0, PPERR_PPFILTTEXTUNSUPP);
	}
	else {
		int    inv_sign_result = -1;
		if(_signature.bin != preserve_signature) {
			const  size_t preserve_rd_offs_inner = rBuf.GetRdOffs();
			rBuf.SetRdOffs(preserve_rd_offs);
			inv_sign_result = ReadPreviousVer(rBuf, RpvInvSignValue);
			THROW(inv_sign_result);
			//rBuf.SetRdOffs(preserve_rd_offs_inner);
			_signature.bin = preserve_signature;
		}
		Signature = _signature.bin;
		if(inv_sign_result < 0) {
			THROW_SL(rBuf.Read(Capability));
			THROW_SL(rBuf.Read(Ver));
			if(labs(Ver) < labs(preserve_ver)) { // (GoodsFilt имеет метку версии с минусом)
				rBuf.SetRdOffs(preserve_rd_offs);
				int r = ReadPreviousVer(rBuf, Ver);
				THROW(r);
				THROW_PP(r > 0, PPERR_INVFILTVERSION);
				Ver = preserve_ver;
			}
			else {
				THROW_PP(preserve_ver == Ver, PPERR_INVFILTVERSION);
				THROW_SL(!FlatSize || rBuf.Read(PTR8(this) + FlatOffs, FlatSize));
				for(uint i = 0; i < BranchList.getCount(); i++) {
					const Branch * p_b = static_cast<Branch *>(BranchList.at(i));
					if(p_b->Type == Branch::tSString) {
						SString * p_str = reinterpret_cast<SString *>(PTR8(this) + p_b->Offs);
						THROW_SL(rBuf.Read(*p_str));
					}
					else if(p_b->Type == Branch::tSArray) {
						SArray * p_ary = reinterpret_cast<SArray *>(PTR8(this) + p_b->Offs);
						THROW_SL(rBuf.Read(p_ary, 0));
					}
					else if(p_b->Type == Branch::tSVector) {
						SVector * p_ary = reinterpret_cast<SVector *>(PTR8(this) + p_b->Offs);
						THROW_SL(rBuf.Read(p_ary, 0));
					}
					else if(p_b->Type == Branch::tObjIdListFilt) {
						ObjIdListFilt * p_list = reinterpret_cast<ObjIdListFilt *>(PTR8(this) + p_b->Offs);
						THROW_SL(p_list->Read(rBuf));
					}
					else if(p_b->Type == Branch::tStrAssocArray) {
						StrAssocArray * p_ary = reinterpret_cast<StrAssocArray *>(PTR8(this) + p_b->Offs);
						THROW_SL(p_ary->Read(rBuf, 0));
					}
					else if(p_b->Type == Branch::tDisplayExtList) {
						PPViewDisplayExtList * p_list = reinterpret_cast<PPViewDisplayExtList *>(PTR8(this) + p_b->Offs);
						THROW(p_list->Read(rBuf));
					}
					else if(p_b->Type == Branch::tBaseFiltPtr) {
						PPBaseFilt ** pp_filt = reinterpret_cast<PPBaseFilt **>(PTR8(this) + p_b->Offs);
						THROW(PPView::ReadFiltPtr(rBuf, pp_filt));
					}
				}
			}
		}
	}
	CATCH
		Ver = preserve_ver;
		Signature = preserve_signature;
		Capability = preserve_capability;
		if(PPErrCode == PPERR_INVFILTVERSION) {
			PPView::Rc rc;
			// @v11.4.4 if(PPView::LoadResource(1, Signature, rc) > 0)
			if(PPView::GetFilterById(Signature, rc)) // @v11.4.4
				PPSetAddedMsgString(rc.Symb);
			else
				PPSetAddedMsgString(temp_buf.Z().Cat(Signature));
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int PPBaseFilt::ReadText(const char * pText, long)
{
	return -1;
}

/*static*/int PPBaseFilt::CopyBaseFiltPtr(int filtId, const PPBaseFilt * pSrcFilt, PPBaseFilt ** ppDestFilt)
{
	int    ok = 1;
	if(pSrcFilt) {
		if((*ppDestFilt) == 0)
			THROW(PPView::CreateFiltInstance(filtId, ppDestFilt));
		THROW((*ppDestFilt)->Copy(pSrcFilt, 0));
	}
	else
		ZDELETE(*ppDestFilt);
	CATCHZOK
	return ok;
}

int PPBaseFilt::Copy(const PPBaseFilt * pS, int)
{
	int    ok = 1;
	THROW(IsA(pS));
	Signature = pS->Signature;
	Capability = pS->Capability;
	FlatOffs = pS->FlatOffs;
	FlatSize = pS->FlatSize;
	if(FlatSize)
		memcpy(PTR8(this) + FlatOffs, PTR8C(pS) + FlatOffs, FlatSize);
	for(uint i = 0; i < BranchList.getCount(); i++) {
		const Branch * p_b = static_cast<Branch *>(BranchList.at(i));
		if(p_b->Type == Branch::tSString) {
			SString * p_str = reinterpret_cast<SString *>(PTR8(this) + p_b->Offs);
			const SString * p_src_str = reinterpret_cast<const SString *>(PTR8C(pS) + p_b->Offs);
			*p_str = *p_src_str;
		}
		else if(p_b->Type == Branch::tSArray) {
			SArray * p_ary = reinterpret_cast<SArray *>(PTR8(this) + p_b->Offs);
			const SArray * p_src_ary = reinterpret_cast<const SArray *>(PTR8C(pS) + p_b->Offs);
			*p_ary = *p_src_ary;
		}
		else if(p_b->Type == Branch::tSVector) {
			SVector * p_ary = reinterpret_cast<SVector *>(PTR8(this) + p_b->Offs);
			const SVector * p_src_ary = reinterpret_cast<const SVector *>(PTR8C(pS) + p_b->Offs);
			*p_ary = *p_src_ary;
		}
		else if(p_b->Type == Branch::tObjIdListFilt) {
			ObjIdListFilt * p_list = reinterpret_cast<ObjIdListFilt *>(PTR8(this) + p_b->Offs);
			const ObjIdListFilt * p_src_list = reinterpret_cast<const ObjIdListFilt *>(PTR8C(pS) + p_b->Offs);
			*p_list = *p_src_list;
		}
		else if(p_b->Type == Branch::tStrAssocArray) {
			StrAssocArray * p_ary = reinterpret_cast<StrAssocArray *>(PTR8(this) + p_b->Offs);
			const StrAssocArray * p_src_ary = reinterpret_cast<const StrAssocArray *>(PTR8C(pS) + p_b->Offs);
			*p_ary = *p_src_ary;
		}
		else if(p_b->Type == Branch::tDisplayExtList) {
			PPViewDisplayExtList * p_list = reinterpret_cast<PPViewDisplayExtList *>(PTR8(this) + p_b->Offs);
			const PPViewDisplayExtList * p_src_list = reinterpret_cast<const PPViewDisplayExtList *>(PTR8C(pS) + p_b->Offs);
			*p_list = *p_src_list;
		}
		else if(p_b->Type == Branch::tBaseFiltPtr) {
			PPBaseFilt ** pp_filt = reinterpret_cast<PPBaseFilt **>(PTR8(this) + p_b->Offs);
			const PPBaseFilt * p_src_filt = *reinterpret_cast<PPBaseFilt * const *>(PTR8C(pS) + p_b->Offs);
			THROW(CopyBaseFiltPtr(p_b->ExtraId, p_src_filt, pp_filt));
		}
	}
	CATCHZOK
	return ok;
}

bool PPBaseFilt::IsEq(const PPBaseFilt * pS, int) const
{
	bool   ok = false;
	if(IsA(pS)) {
		if(Signature == pS->Signature && Capability == pS->Capability) {
			if(!FlatSize || memcmp(PTR8C(this) + FlatOffs, PTR8C(pS) + FlatOffs, FlatSize) == 0) {
				ok = true;
				for(uint i = 0; ok && i < BranchList.getCount(); i++) {
					const Branch * p_b = static_cast<const Branch *>(BranchList.at(i));
					if(p_b->Type == Branch::tSString) {
						const SString * p_str = reinterpret_cast<const SString *>(PTR8C(this) + p_b->Offs);
						const SString * p_src_str = reinterpret_cast<const SString *>(PTR8C(pS) + p_b->Offs);
						if(p_str->Cmp(*p_src_str, 0) != 0)
							ok = false;
					}
					else if(p_b->Type == Branch::tSArray) {
						const SArray * p_ary = reinterpret_cast<const SArray *>(PTR8C(this) + p_b->Offs);
						const SArray * p_src_ary = reinterpret_cast<const SArray *>(PTR8C(pS) + p_b->Offs);
						if(!p_ary->IsEq(*p_src_ary))
							ok = false;
					}
					else if(p_b->Type == Branch::tSVector) {
						const SVector * p_ary = reinterpret_cast<const SVector *>(PTR8C(this) + p_b->Offs);
						const SVector * p_src_ary = reinterpret_cast<const SVector *>(PTR8C(pS) + p_b->Offs);
						if(!p_ary->IsEq(*p_src_ary))
							ok = false;
					}
					else if(p_b->Type == Branch::tObjIdListFilt) {
						const ObjIdListFilt * p_list = reinterpret_cast<const ObjIdListFilt *>(PTR8C(this) + p_b->Offs);
						const ObjIdListFilt * p_src_list = reinterpret_cast<const ObjIdListFilt *>(PTR8C(pS) + p_b->Offs);
						if(!p_list->IsEq(*p_src_list))
							ok = false;
					}
					else if(p_b->Type == Branch::tStrAssocArray) {
						const StrAssocArray * p_ary = reinterpret_cast<const StrAssocArray *>(PTR8C(this) + p_b->Offs);
						const StrAssocArray * p_src_ary = reinterpret_cast<const StrAssocArray *>(PTR8C(pS) + p_b->Offs);
						if(!p_ary->IsEq(*p_src_ary))
							ok = false;
					}
					else if(p_b->Type == Branch::tDisplayExtList) {
						const PPViewDisplayExtList * p_list = reinterpret_cast<const PPViewDisplayExtList *>(PTR8C(this) + p_b->Offs);
						const PPViewDisplayExtList * p_src_list = reinterpret_cast<const PPViewDisplayExtList *>(PTR8C(pS) + p_b->Offs);
						if(!p_list->IsEq(*p_src_list))
							ok = false;
					}
					else if(p_b->Type == Branch::tBaseFiltPtr) {
						const PPBaseFilt * p_filt = *reinterpret_cast<const PPBaseFilt * const *>(PTR8C(this) + p_b->Offs);
						const PPBaseFilt * p_src_filt = *reinterpret_cast<const PPBaseFilt * const *>(PTR8C(pS) + p_b->Offs);
						if(p_filt && p_src_filt) {
							if(!p_filt->IsEq(p_src_filt, 0))
								ok = false;
						}
						else if(!(!p_filt && !p_src_filt))
							ok = false;
					}
				}
			}
		}
	}
	return ok;
}

/*virtual*/bool PPBaseFilt::IsEmpty() const
{
	PPBaseFilt * p_filt = 0;
	PPView::CreateFiltInstance(Signature, &p_filt);
	bool   r = (p_filt && IsEq(p_filt, 0));
	ZDELETE(p_filt);
	return r;
}
//
//
//
// @v10.9.1 (replaced with _PPConst.Signature_PPView) #define SIGN_PPVIEW 0x099A099BUL

PPView::PPView(PPObject * pObj, PPBaseFilt * pBaseFilt, int viewId, long implFlags, long defReportId) : 
	P_Obj(pObj), Sign(_PPConst.Signature_PPView), ExecFlags(0), ServerInstId(0),
	ViewId(viewId), BaseState(0), ExtToolbarId(0), P_IterQuery(0), P_Ct(0), P_F(pBaseFilt), ImplementFlags(implFlags), 
	DefReportId(defReportId), P_LastUpdatedObjects(0)
{
	if(ViewId) {
		Rc rc;
		// @v11.4.4 if(PPView::LoadResource(0, ViewId, rc)) {
		if(GetDescriptionById(ViewId, rc)) {
			Symb = rc.Symb;
			Descr = rc.Descr;
		}
	}
}

PPView::~PPView()
{
	if(ServerInstId) {
		if(BaseState & bsServerInst) {
			DS.GetTLA().ReleasePPViewPtr(ServerInstId);
		}
		else {
			PPJobSrvClient * p_cli = DS.GetClientSession(0);
			if(p_cli) {
				PPJobSrvCmd cmd;
				PPJobSrvReply reply;
				if(cmd.StartWriting(PPSCMD_DESTROYVIEW) && cmd.Write(ServerInstId)) {
					cmd.FinishWriting();
					if(p_cli->Exec(cmd, reply)) {
						reply.StartReading(0);
						if(reply.CheckRepError()) {
							BaseState |= bsServerInstDestr;
						}
					}
				}
			}
		}
	}
	Sign = 0;
	delete P_LastUpdatedObjects;
	delete P_IterQuery;
	delete P_Ct;
}

int    PPView::IsConsistent() const { return BIN(Sign == _PPConst.Signature_PPView); }
const  PPBaseFilt * PPView::GetBaseFilt() const { return P_F ? P_F : (PPSetError(PPERR_BASEFILTUNSUPPORTED), 0); }

int FASTCALL PPView::Helper_InitBaseFilt(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	if(P_F) {
		if(pFilt) {
			if(P_F->IsA(pFilt))
				P_F->Copy(pFilt, 1);
			else
				ok = 0;
		}
		else if(!P_F->Init(1, 0))
			ok = 0;
	}
	else
		ok = PPSetError(PPERR_UNDEFINNERBASEFILT);
	return ok;
}

PPBaseFilt * PPView::CreateFilt(const void * extraPtr) const
{
	if(P_F) {
		PPBaseFilt * p_filt = 0;
		PPView::CreateFiltInstanceBySymb(Symb, &p_filt);
		return p_filt;
	}
	else
		return (PPSetError(PPERR_BASEFILTUNSUPPORTED), 0);
}

#if 0 // @construction {

class DBTWrapper {
public:
	virtual DBTable * Create(const char * pFileName) = 0;
};

template <class T> class TDBTWrapper {
public:
	virtual DBTable * Create(const char * pFileName)
	{
		return new T(pFileName);
	}
};

/*static*/int PPView::SerializeTableSpec(int dir, DBTable ** ppTbl, TDBTWrapper & rCw, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	uint8  ind = 0;
#define T (*ppTbl)
	if(dir > 0) {
		if(T) {
			THROW(rBuf.Write(ind = 0));
			THROW(T->SerializeSpec(dir, rBuf, pCtx));
		}
		else {
			THROW(rBuf.Write(ind = 1));
		}
	}
	else if(dir < 0) {
		ZDELETE(T);
		THROW(rBuf.Read(ind));
		if(ind == 1) {
			// spec is empty
			ok = +100;
		}
		else if(ind == 0) {
			T = rCw.Create();
			THROW(T->SerializeSpec(dir, rBuf, pCtx));
			THROW(T->open(0, pTbl->fileName));
		}
	}
#undef T
	CATCHZOK
	return ok;
}

#endif // } 0 @construction

/*static*/int PPView::Destroy(PPJobSrvCmd & rCmd, PPJobSrvReply & rReply)
{
	int    ok = 1;
	int32  inst_id = 0;
	THROW_SL(rCmd.Read(inst_id));
	delete DS.GetTLA().GetPPViewPtr(inst_id);
	rReply.SetAck();
	CATCHZOK
	return ok;
}

/*static*/int PPView::Refresh(PPJobSrvCmd & rCmd, PPJobSrvReply & rReply)
{
	int    ok = -2;
	int32  inst_id = 0;
	THROW_SL(rCmd.Read(inst_id));
	PPView * p_v = DS.GetTLA().GetPPViewPtr(inst_id);
	if(p_v) {
		ok = p_v->ProcessCommand(PPVCMD_REFRESH, 0, 0);
	}
	{
		SString reply_buf;
		rReply.SetString(reply_buf.Cat(ok));
	}
	CATCHZOK
	return ok;
}

static int PublishNfViewToMqb(const PPNamedFilt * pNf, const char * pFileName)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	if(pNf && pNf->DestGuaList.GetCount()) {
		int    is_there_acceptable_gua = 0;
		{
			for(uint dgidx = 0; !is_there_acceptable_gua && dgidx < pNf->DestGuaList.GetCount(); dgidx++) {
				const PPID gua_id = pNf->DestGuaList.Get(dgidx);
				S_GUID gua_guid;
				if(gua_id && p_ref->Ot.GetTagGuid(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_GUID, gua_guid) > 0)
					is_there_acceptable_gua = 1;
			}
		}
		if(is_there_acceptable_gua) {
			PPMqbClient mqc;
			SString data_domain;
			if(PPMqbClient::InitClient(mqc, &data_domain)) {
				int64 _fsize = 0;
				SFile f_in(pFileName, SFile::mRead|SFile::mBinary);
				THROW_SL(f_in.IsValid());
				f_in.CalcSize(&_fsize);
				if(_fsize > 0) {
					STempBuffer data_buf(static_cast<size_t>(_fsize)+1024); // +1024 - insurance
					size_t actual_rd_size = 0;
					THROW_SL(data_buf.IsValid());
					THROW_SL(f_in.Read(data_buf, data_buf.GetSize(), &actual_rd_size));
					for(uint dgidx = 0; dgidx < pNf->DestGuaList.GetCount(); dgidx++) {
						const PPID gua_id = pNf->DestGuaList.Get(dgidx);
						S_GUID gua_guid;
						if(gua_id && p_ref->Ot.GetTagGuid(PPOBJ_GLOBALUSERACC, gua_id, PPTAG_GUA_GUID, gua_guid) > 0) {
							PPMqbClient::MessageProperties props;
							props.ContentType = SFileFormat::Xml;
							props.Encoding = SEncodingFormat::Unkn;
							props.Priority = 5;
							props.TimeStamp = getcurdatetime_();
							SPathStruc ps(pFileName);
							ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
							props.Headers.Add("filename", temp_buf.Transf(CTRANSF_OUTER_TO_UTF8));
							props.Headers.Add("namedfilt-name", (temp_buf = pNf->Name).Transf(CTRANSF_INNER_TO_UTF8));
							props.Headers.Add("namedfilt-symb", (temp_buf = pNf->Symb).Transf(CTRANSF_INNER_TO_UTF8));
							props.Headers.Add("namedfilt-viewsymb", (temp_buf = pNf->ViewSymb).Transf(CTRANSF_INNER_TO_UTF8));
							props.Headers.Add("namedfilt-dbsymb", (temp_buf = pNf->DbSymb).Transf(CTRANSF_INNER_TO_UTF8));
							{
								PPMqbClient::RoutingParamEntry rpe;
								THROW(rpe.SetupReserved(PPMqbClient::rtrsrvStyloView, data_domain, &gua_guid, 0, 0));
								THROW(mqc.ApplyRoutingParamEntry(rpe));
								THROW(mqc.Publish(rpe.ExchangeName, rpe.RoutingKey, &props, data_buf, actual_rd_size));
								ok = 1;
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

/*static*/int PPView::ExecuteNF(const PPNamedFilt * pNf, const char * pDl600Name, SString & rResultFileName)
{
	rResultFileName.Z();
	int    ok = 1;
	SString result_json_file_name;
	PPView * p_view = 0;
	PPBaseFilt * p_filt = 0;
	DlRtm  * p_rtm = 0;
	if(!pNf)
		ok = -1;
	else {
		SString dl600_name(pDl600Name);
		SBuffer filt_buf;
		THROW_PP_S(pNf->ViewID, PPERR_NAMEDFILTUNDEFVIEWID, pNf->Symb);
		THROW(PPView::CreateInstance(pNf->ViewID, &p_view));
		filt_buf = pNf->Param;
		if(filt_buf.GetAvailableSize()) {
			THROW(PPView::ReadFiltPtr(filt_buf, &p_filt));
		}
		else {
			THROW(p_filt = p_view->CreateFilt(PPView::GetDescriptionExtra(p_view->GetViewId())));
		}
		THROW(p_view->Init_(p_filt));
		if(!dl600_name.NotEmptyS()) {
			dl600_name = pNf->VD.GetStrucSymb();
		}
		if(!dl600_name.NotEmptyS()) {
			if(p_view->DefReportId) {
				SReport rpt(p_view->DefReportId, 0);
				THROW(rpt.IsValid());
				dl600_name = rpt.getDataName();
			}
		}
		{
			DlContext ctx;
			PPFilt f(p_view);
			DlRtm::ExportParam ep;
			THROW(ctx.InitSpecial(DlContext::ispcExpData));
			THROW(ctx.CreateDlRtmInstance(dl600_name, &p_rtm));
			ep.P_F = &f;
			ep.Sort = 0;
			ep.Flags |= (DlRtm::ExportParam::fIsView|DlRtm::ExportParam::fInheritedTblNames);
			ep.Flags &= ~DlRtm::ExportParam::fDiff_ID_ByScope;
			SETFLAG(ep.Flags, DlRtm::ExportParam::fCompressXml, pNf->Flags & PPNamedFilt::fCompressXml);
			if(pNf->Flags & PPNamedFilt::fDontWriteXmlDTD)
				ep.Flags |= (DlRtm::ExportParam::fDontWriteXmlDTD|DlRtm::ExportParam::fDontWriteXmlTypes);
			if(pNf->VD.GetCount() > 0)
				ep.P_ViewDef = &pNf->VD;
			ep.Cp = cpUTF8;
			THROW(p_rtm->ExportXML(ep, rResultFileName));
			// @debug {
			ep.Flags |= DlRtm::ExportParam::fJsonStQStyle; 
			THROW(p_rtm->ExportJson(ep, result_json_file_name)); 
			// } @debug 
			if(!PublishNfViewToMqb(pNf, rResultFileName)) {
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
			}
		}
	}
	CATCHZOK
	delete p_rtm;
	delete p_filt;
	delete p_view;
	return ok;
}

/*static*/int PPView::ExecuteNF(const PPNamedFilt * pNf, const char * pDl600Name, int format, SBuffer & rResult)
{
	assert(oneof3(format, SFileFormat::Unkn, SFileFormat::Xml, SFileFormat::Json));
	rResult.Z();
	int    ok = 1;
	SJson * p_js = 0;
	xmlBuffer * p_xml_buf = 0;
	xmlTextWriter * p_xml_writer = 0;
	PPView * p_view = 0;
	PPBaseFilt * p_filt = 0;
	DlRtm  * p_rtm = 0;
	if(!pNf)
		ok = -1;
	else {
		SString dl600_name(pDl600Name);
		SBuffer filt_buf;
		THROW_PP_S(pNf->ViewID, PPERR_NAMEDFILTUNDEFVIEWID, pNf->Symb);
		THROW(PPView::CreateInstance(pNf->ViewID, &p_view));
		filt_buf = pNf->Param;
		if(filt_buf.GetAvailableSize()) {
			THROW(PPView::ReadFiltPtr(filt_buf, &p_filt));
		}
		else {
			THROW(p_filt = p_view->CreateFilt(PPView::GetDescriptionExtra(p_view->GetViewId())));
		}
		THROW(p_view->Init_(p_filt));
		if(!dl600_name.NotEmptyS()) {
			dl600_name = pNf->VD.GetStrucSymb();
		}
		if(!dl600_name.NotEmptyS()) {
			if(p_view->DefReportId) {
				SReport rpt(p_view->DefReportId, 0);
				THROW(rpt.IsValid());
				dl600_name = rpt.getDataName();
			}
		}
		{
			DlContext ctx;
			PPFilt f(p_view);
			DlRtm::ExportParam ep;
			THROW(ctx.InitSpecial(DlContext::ispcExpData));
			THROW(ctx.CreateDlRtmInstance(dl600_name, &p_rtm));
			ep.P_F = &f;
			ep.Sort = 0;
			ep.Flags |= (DlRtm::ExportParam::fIsView|DlRtm::ExportParam::fInheritedTblNames);
			ep.Flags &= ~DlRtm::ExportParam::fDiff_ID_ByScope;
			SETFLAG(ep.Flags, DlRtm::ExportParam::fCompressXml, pNf->Flags & PPNamedFilt::fCompressXml);
			if(pNf->Flags & PPNamedFilt::fDontWriteXmlDTD)
				ep.Flags |= (DlRtm::ExportParam::fDontWriteXmlDTD|DlRtm::ExportParam::fDontWriteXmlTypes);
			if(pNf->VD.GetCount() > 0)
				ep.P_ViewDef = &pNf->VD;
			ep.Cp = cpUTF8;
			if(format == SFileFormat::Xml || format == SFileFormat::Unkn) {
				THROW(p_xml_buf = xmlBufferCreate());
				THROW(p_xml_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
				THROW(p_rtm->Helper_WriteXML(ep, p_xml_writer));
				xmlTextWriterFlush(p_xml_writer);
				rResult.Write(reinterpret_cast<const char *>(p_xml_buf->content), p_xml_buf->use);
			}
			else if(format == SFileFormat::Json) {
				SString json_buf;
				ep.Flags |= DlRtm::ExportParam::fJsonStQStyle; 
				THROW(p_js = p_rtm->ExportJson(ep));
				THROW_SL(p_js->ToStr(json_buf));
				rResult.Write(json_buf, json_buf.Len());
			}
		}
	}
	CATCHZOK
	delete p_js;
	xmlFreeTextWriter(p_xml_writer);
	xmlBufferFree(p_xml_buf);
	delete p_rtm;
	delete p_filt;
	delete p_view;
	return ok;
}

/*static*/int PPView::ExecuteNF(const char * pNamedFiltSymb, const char * pDl600Name, SString & rResultFileName)
{
	rResultFileName.Z();
	int    ok = 1;
	SString db_symb;
	SString filt_symb(pNamedFiltSymb);
	SString dl600_name(pDl600Name);
	THROW_PP(CurDict->GetDbSymb(db_symb), PPERR_DBSYMBUNDEF);
	{
		PPNamedFiltMngr mgr;
		PPNamedFiltPool pool(0, 1);
		THROW(mgr.LoadPool2(db_symb, &pool, true));
		const PPNamedFilt * p_nf = pool.GetBySymb(filt_symb, 0);
		THROW(p_nf);
		THROW(ExecuteNF(p_nf, pDl600Name, rResultFileName));
	}
	CATCHZOK
	return ok;
}

PPView::ExecNfViewParam::ExecNfViewParam() : Flags(0)
{
	memzero(ReserveStart, sizeof(ReserveStart));
}

int PPView::ExecNfViewParam::Write(SBuffer & rBuf, long)
{
	int    ok = 1;
	THROW_SL(rBuf.Write(ReserveStart, sizeof(ReserveStart)));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(NfSymb));
	THROW_SL(rBuf.Write(Dl600_Name));
	THROW_SL(rBuf.Write(FileName));
	CATCHZOK
	return ok;
}

int PPView::ExecNfViewParam::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	THROW_SL(rBuf.ReadV(ReserveStart, sizeof(ReserveStart)));
	THROW_SL(rBuf.Read(Flags));
	THROW_SL(rBuf.Read(NfSymb));
	THROW_SL(rBuf.Read(Dl600_Name));
	THROW_SL(rBuf.Read(FileName));
	CATCHZOK
	return ok;
}

/*static*/int PPView::EditExecNfViewParam(ExecNfViewParam & rData)
{
	int    ok = -1;
	TDialog * dlg = 0;
	PPNamedFiltMngr nf_mngr;
	SString db_symb;
	THROW_PP(CurDict->GetDbSymb(db_symb), PPERR_DBSYMBUNDEF);
	dlg = new TDialog(DLG_JOB_EXPVIEW);
	THROW(CheckDialogPtrErr(&dlg));
	{
		PPNamedFiltPool nf_pool(0, 1);
		PPNamedFilt nf;
		PPID   nf_id = 0, sel_nf_id = 0;
		StrAssocArray nf_list;
		THROW(nf_mngr.LoadPool2(db_symb, &nf_pool, false)); //@erik v10.7.5 LoadPool-->LoadPool2
		for(nf_id = 0; nf_pool.Enum(&nf_id, &nf) > 0;) {
			if(rData.NfSymb.NotEmpty() && rData.NfSymb.CmpNC(nf.Symb) == 0)
				sel_nf_id = nf.ID;
			nf_list.Add(nf.ID, nf.Name);
		}
		SetupStrAssocCombo(dlg, CTLSEL_JOB_EXPVIEW_FILT, nf_list, sel_nf_id, 0, 0, 0);
		dlg->setCtrlString(CTL_JOB_EXPVIEW_DL6STRUC, rData.Dl600_Name);
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_JOB_EXPVIEW_OUT, CTL_JOB_EXPVIEW_OUT, 1, 0, 0,
			FileBrowseCtrlGroup::fbcgfFile|FileBrowseCtrlGroup::fbcgfAllowNExists);
		dlg->setCtrlString(CTL_JOB_EXPVIEW_OUT, rData.FileName);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			nf_id = dlg->getCtrlLong(CTLSEL_JOB_EXPVIEW_FILT);
			const  PPNamedFilt * p_nf = nf_id ? nf_pool.GetByID(nf_id) : 0;
			if(p_nf && p_nf->Symb.NotEmpty()) {
				rData.NfSymb = p_nf->Symb;
				dlg->getCtrlString(CTL_JOB_EXPVIEW_DL6STRUC, rData.Dl600_Name);
				dlg->getCtrlString(CTL_JOB_EXPVIEW_OUT, rData.FileName);
				ok = 1;
			}
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

/*static*/int PPView::ExecuteServer(PPJobSrvCmd & rCmd, PPJobSrvReply & rReply)
{
	int    ok = 1;
	PPView * p_view = 0;
	PPBaseFilt * p_filt = 0;
	long   view_id = 0;
	long   cookie = 0;
	int32  inst_id = 0;
	{
		struct AdviseProcWrapper {
			static int Proc(int kind, const PPNotifyEvent * pEv, void * procExtPtr)
			{
				PPJobSrvReply * p_reply = static_cast<PPJobSrvReply *>(procExtPtr);
				if(p_reply) {
					SString temp_buf;
					pEv->GetExtStrData(PPNotifyEvent::extssMessage, temp_buf);
					p_reply->SendInformer(temp_buf);
				}
				return 1;
			}
		};
		PPAdviseBlock adv_blk;
		adv_blk.Kind = PPAdviseBlock::evWaitMsg;
		adv_blk.TId  = DS.GetConstTLA().GetThreadID();
		adv_blk.Proc = AdviseProcWrapper::Proc;
		adv_blk.ProcExtPtr = &rReply;
		DS.Advise(&cookie, &adv_blk);
	}
	THROW_SL(rCmd.Read(view_id));
	THROW_SL(rCmd.Read(inst_id));
	THROW(PPView::CreateInstance(view_id, &inst_id, &p_view));
	p_view->ServerInstId = inst_id;
	p_view->BaseState |= bsServerInst;
	THROW(p_filt = p_view->CreateFilt(PPView::GetDescriptionExtra(p_view->GetViewId())));
	{
		SBuffer temp_sbuf;
		SSerializeContext ctx;
		ctx.Init(0, getcurdate_());
		THROW(p_filt->Serialize(-1, rCmd, &ctx));
		THROW(p_view->Init_(p_filt));
		//
		ctx.Init(0, getcurdate_());
		THROW(p_view->SerializeState(+1, temp_sbuf, &ctx));
		THROW_SL(ctx.SerializeStateOfContext(+1, rReply));
		THROW_SL(rReply.Write(temp_sbuf.GetBuf(temp_sbuf.GetRdOffs()), temp_sbuf.GetAvailableSize()));
		p_view = 0; // Указатель p_view не разрушается ибо перешел во владение PPThreadLocalArea
	}
	CATCH
		ok = 0;
		delete p_view;
	ENDCATCH
	DS.Unadvise(cookie);
	delete p_filt;
	return ok;
}

int PPView::Helper_Init(const PPBaseFilt * pFilt, int flags)
{
	int    ok = 1, do_local = 1;
	int    try_reconnect = 2;
	PPJobSrvClient * p_cli = 0;
	PPBaseFilt * p_filt = 0;
	OuterTitle.Z();
	ExecFlags = flags;
	if(ImplementFlags & implUseServer && !(flags & exefDisable3Tier) && (p_cli = DS.GetClientSession(0)) != 0) {
		while(try_reconnect) {
			SSerializeContext ctx;
			PPJobSrvCmd cmd;
			PPJobSrvReply reply;
			//
			// Так как Serialize - non-const функция, придется создать копию фильтра
			// и уже ее передавать серверу.
			//
			THROW(p_filt = CreateFilt(PPView::GetDescriptionExtra(GetViewId())));
			THROW(p_filt->Copy(pFilt, 1));
			THROW(cmd.StartWriting(PPSCMD_CREATEVIEW));
			THROW_SL(cmd.Write(ViewId));
			THROW_SL(cmd.Write(ServerInstId));
			THROW(p_filt->Serialize(+1, cmd, &ctx));
			cmd.FinishWriting();
			if(!p_cli->Exec(cmd, reply)) {
				const SlThreadLocalArea & r_sltla = SLS.GetConstTLA();
				if(try_reconnect && PPErrCode == PPERR_SLIB && r_sltla.LastErr == SLERR_SOCK_WINSOCK && r_sltla.LastSockErr == WSAECONNRESET) {
					if(--try_reconnect) {
						p_cli = DS.GetClientSession(0);
						if(!p_cli)
							break;
					}
				}
				else
					try_reconnect = 0;
			}
			else {
				reply.StartReading(0);
				THROW(reply.CheckRepError());
				ctx.SerializeStateOfContext(-1, reply);
				SerializeState(-1, reply, &ctx);
				do_local = 0;
				try_reconnect = 0;
			}
		}
	}
	if(do_local) {
		THROW(InitLocal(pFilt)); // @v10.4.0
		/* @v10.4.0 SString filt_desc;
		pFilt->Describe(0, filt_desc);
		Profile profile;
		profile.Start(PPFILNAM_USERPROFILE_LOG, GetSymb(), filt_desc);
		THROW(Init_(pFilt));
		profile.Finish(PPFILNAM_USERPROFILE_LOG, GetSymb(), filt_desc); */
	}
	CATCHZOK
	delete p_filt;
	return ok;
}

int PPView::InitLocal(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SString filt_desc;
	pBaseFilt->Describe(0, filt_desc);
	Profile profile;
	profile.Start(PPFILNAM_USERPROFILE_LOG, GetSymb(), filt_desc);
	THROW(Init_(pBaseFilt));
	profile.Finish(PPFILNAM_USERPROFILE_LOG, GetSymb(), filt_desc);
	CATCHZOK
	return ok;
}

void PPView::SetOuterTitle(const char * pOuterTitle)
{
	(OuterTitle = pOuterTitle).Strip();
}

int PPView::GetOuterTitle(SString * pBuf) const
{
	ASSIGN_PTR(pBuf, OuterTitle);
	return BIN(OuterTitle.NotEmpty());
}

int PPView::EditBaseFilt(PPBaseFilt * pFilt) { return PPSetError(PPERR_BASEFILTUNSUPPORTED); }
int PPView::Init_(const PPBaseFilt * pFilt) { return PPSetError(PPERR_BASEFILTUNSUPPORTED); }
int PPView::GetOuterChangesStatus() const { return BIN(BaseState & bsOuterChangesStatus); }
const char * PPView::GetSymb() const { return Symb.cptr(); }
const char * PPView::GetDescr() const { return Descr.cptr(); }
int PPView::IsCrosstab() const { return BIN(P_Ct);}
const IterCounter & PPView::GetCounter() const { return Counter; }
void * PPView::GetEditExtraParam() { return 0; }
DBQuery * PPView::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle) { return 0; }
SArray * PPView::CreateBrowserArray(uint * pBrwId, SString * pSubTitle) { return 0; }
void PPView::PreprocessBrowser(PPViewBrowser * pBrw) {}
int PPView::OnExecBrowser(PPViewBrowser *) { return -1; }
int PPView::Detail(const void *, PPViewBrowser * pBrw) { return -1; }
void PPView::ViewTotal() {}

void PPView::Helper_FormatCycle(const PPCycleFilt & rCf, const PPCycleArray & rCa, LDATE dt, char * pBuf, size_t bufLen)
{
	if(rCf.Cycle)
		rCa.formatCycle(dt, pBuf, bufLen);
	else
		ASSIGN_PTR(pBuf, 0);
}

int PPView::Helper_ProcessQuickTagEdit(PPObjID oid, const void * pHdrPtr /*(LongArray *)*/)
{
	int    ok = -1;
	if(pHdrPtr && oid.Obj && oid.Id) {
		const LongArray * p_tag_id_list = static_cast<const LongArray *>(pHdrPtr);
		const uint lc = p_tag_id_list->getCount();
		if(lc > 0 && lc < 20) { // Ограничение 20 частично страхует от случая, когда вместо указателя на LongArray нам подсунули что-то иное
			PPID tag_id = 0;
			PPObjTag tag_obj;
			PPObjectTag tag_rec;
			for(uint i = 0; !tag_id && i < lc; i++) {
				if(tag_obj.Fetch(p_tag_id_list->get(i), &tag_rec) > 0 && tag_rec.ObjTypeID == oid.Obj)
					tag_id = tag_rec.ID;
			}
			if(tag_id) {
				ObjTagItem tag_item;
				ObjTagCore & r_ot = PPRef->Ot;
				if(r_ot.GetTag(oid.Obj, oid.Id, tag_id, &tag_item) > 0 || tag_item.Init(tag_id)) {
					if(EditObjTagItem(oid.Obj, oid.Id, &tag_item, 0) > 0) {
						if(r_ot.PutTag(oid.Obj, oid.Id, &tag_item, 1))
							ok = 1;
						else
							PPError();
					}
				}
			}
		}
	}
	return ok;
}

int PPView::Implement_CmpSortIndexItems_OnArray(PPViewBrowser * pBrw, const void * pItem1, const void * pItem2)
{
	int    sn = 0;
	AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
	if(p_def) {
		uint8  dest_data1[1024];
		uint8  dest_data2[1024];
		for(uint i = 0; !sn && i < pBrw->GetSettledOrderList().getCount(); i++) {
			int    col = pBrw->GetSettledOrderList().get(i);
			TYPEID typ1 = 0;
			TYPEID typ2 = 0;
			if(p_def->GetCellData(pItem1, labs(col)-1, &typ1, &dest_data1, sizeof(dest_data1)) && p_def->GetCellData(pItem2, labs(col)-1, &typ2, &dest_data2, sizeof(dest_data2))) {
				assert(typ1 == typ2);
				if(typ1 == typ2) {
					sn = stcomp(typ1, dest_data1, dest_data2);
					if(sn && col < 0)
						sn = -sn;
				}
			}
		}
	}
	return sn;
}

int PPView::Print(const void *)
{
	return DefReportId ? Helper_Print(DefReportId, 0) : -1;
}

int FASTCALL PPView::Helper_Print(uint rptId, int ord)
{
	int    ok = 1;
	if(rptId) {
		PPReportEnv env;
		env.Sort = ord;
		ok = PPAlddPrint(rptId, PView(this), &env);
	}
	else
		ok = -1;
	return ok;
}

int PPView::ExportXml(uint rptId, int ord)
{
	int    ok = 1;
	if(rptId) {
		PPReportEnv env;
		env.Sort = ord;
		env.PrnFlags = SReport::XmlExport|SReport::PrintingNoAsk;
		ok = PPAlddPrint(rptId, PView(this), &env);
	}
	else
		ok = -1;
	return ok;
}

int PPView::ChangeFilt(int refreshOnly, PPViewBrowser * pW)
{
	int    ok = -1;
	const  uint prev_rez_id = pW ? pW->GetResID() : 0;
	PPBaseFilt * p_filt = CreateFilt(PPView::GetDescriptionExtra(GetViewId()));
	const PPBaseFilt * p_src_filt = GetBaseFilt();
	DBQuery * p_q = 0;
	SArray * p_array = 0;
	THROW(p_filt);
	if(p_src_filt)
		p_filt->Copy(p_src_filt, 1);
	if(refreshOnly || EditBaseFilt(p_filt) > 0) {
		uint   brw_id = 0;
		int    was_ct = BIN(P_Ct);
		PPWaitStart();
		SString sub_title;
		THROW(Helper_Init(p_filt, ExecFlags));
		if(ImplementFlags & implBrowseArray) {
			THROW(p_array = CreateBrowserArray(&brw_id, &sub_title));
			if(pW->ChangeResource(brw_id, p_array, 1) == 2) {
				if(DS.CheckExtFlag(ECF_PREPROCBRWONCHGFILT)) {
					PreprocessBrowser(pW);
					pW->LoadToolbarResource(ExtToolbarId);
				}
				OnExecBrowser(pW);
				ok = 2;
			}
			else
				ok = 1;
		}
		else {
			THROW(p_q = CreateBrowserQuery(&brw_id, &sub_title));
			if(P_Ct) {
				int    r;
				THROW_PP(p_q == PPView::CrosstabDbQueryStub, PPERR_INVPPVIEWQUERY);
				THROW(p_q = P_Ct->CreateBrowserQuery());
				//
				// Если до этого у нас был кросстаб, то изменяем ресурс броузера с признаком force,
				// то есть, броузер будет загружен заново даже если его ID совпадает с предыдущим.
				//
				r = pW->ChangeResource(brw_id, p_q, 1/*was_ct*/);
				if(r > 0) {
					if(r == 2) {
						if(DS.CheckExtFlag(ECF_PREPROCBRWONCHGFILT)) {
							if(!(ImplementFlags & implDontSetupCtColumnsOnChgFilt))
								P_Ct->SetupBrowserCtColumns(pW);
							PreprocessBrowser(pW);
							pW->LoadToolbarResource(ExtToolbarId);
						}
						else
							P_Ct->SetupBrowserCtColumns(pW);
						OnExecBrowser(pW);
						ok = 2;
					}
					else {
						P_Ct->SetupBrowserCtColumns(pW);
						ok = 1;
					}
				}
			}
			else {
				THROW_PP(p_q != PPView::CrosstabDbQueryStub, PPERR_INVPPVIEWQUERY);
				if(pW->ChangeResource(brw_id, p_q, 1/*force*/) == 2) {
					if(DS.CheckExtFlag(ECF_PREPROCBRWONCHGFILT)) {
						PreprocessBrowser(pW);
						pW->LoadToolbarResource(ExtToolbarId);
					}
					OnExecBrowser(pW);
					ok = 2;
				}
				else
					ok = 1;
			}
		}
		pW->SetResID(prev_rez_id);
		pW->setSubTitle(sub_title);
		for(uint i = 0; i < pW->getDef()->getCount(); i++) {
			pW->SetupColumnWidth(i);
		}
		pW->Refresh();
		APPL->NotifyFrame(0);
		PPWaitStop();
	}
	CATCHZOKPPERR
	delete p_filt;
	return ok;
}

int PPView::GetLastUpdatedObjects(long extId, LongArray & rList) const
{
	rList.clear();
	if(extId)
		rList.add(extId);
	if(P_LastUpdatedObjects && P_LastUpdatedObjects->getCount()) {
        for(uint i = 0; i < P_LastUpdatedObjects->getCount(); i++) {
			rList.addnz(P_LastUpdatedObjects->get(i));
        }
		rList.sortAndUndup();
	}
	return rList.getCount() ? +1 : -1;
}

int PPView::UpdateTimeBrowser(const STimeChunkGrid * pGrid, const char * pTitle, int destroy)
{
	int    ok = -1;
	STimeChunkBrowser * p_brw = PPFindLastTimeChunkBrowser();
	if(p_brw && p_brw->IsKeepingData(pGrid)) {
		if(destroy)
			PPCloseBrowser(p_brw);
		else {
			if(pTitle) {
				p_brw->setTitle(pTitle);
				APPL->NotifyFrame(0);
			}
			p_brw->UpdateData();
		}
		ok = 1;
	}
	return ok;
}

void PPView::SetExtToolbar(uint toolbarId)
{
	ExtToolbarId = toolbarId;
}

int PPView::Browse(int modeless)
{
	int    ok = 1;
	DBQuery * q = 0;
	SArray * p_array = 0;
	PPViewBrowser * brw = 0;
	uint   brw_id = 0;
	SString sub_title, title;
	PPWaitStart();
	THROW(!P_Obj || P_Obj->CheckRights(PPR_READ));
	if(ImplementFlags & implBrowseArray) {
		THROW(p_array = CreateBrowserArray(&brw_id, &sub_title));
		THROW_MEM(brw = new PPViewBrowser(brw_id, p_array, this, modeless));
	}
	else {
		THROW(q = CreateBrowserQuery(&brw_id, &sub_title));
		if(P_Ct) {
			THROW_PP(q == PPView::CrosstabDbQueryStub, PPERR_INVPPVIEWQUERY);
			THROW(brw = static_cast<PPViewBrowser *>(P_Ct->CreateBrowser(brw_id, modeless)));
		}
		else {
			THROW_PP(q != PPView::CrosstabDbQueryStub, PPERR_INVPPVIEWQUERY);
			THROW_MEM(brw = new PPViewBrowser(brw_id, q, this, modeless));
		}
	}
	if(GetOuterTitle(&title))
		brw->setTitle(title);
	else
		brw->setSubTitle(sub_title);
	PreprocessBrowser(brw);
	brw->LoadToolbarResource(ExtToolbarId);
	PPWaitStop();
	// { Почти повторяет код PPOpenBrowser() за исключением вызова OnExecBrowser
	if(modeless) {
		brw->SetResID(static_cast<PPApp *>(APPL)->LastCmd);
		int    r = InsertView(brw);
		if(r < 0 && brw->IsConsistent())
			OnExecBrowser(brw);
	}
	else
		ok = (ExecView(brw) == cmOK) ? 1 : -1;
	// }
	CATCH
		ok = (PPWaitStop(), 0);
		if(brw == 0) {
			if(p_array)
				delete p_array;
			else if(q != PPView::CrosstabDbQueryStub)
				delete q;
		}
	ENDCATCH
	if(!modeless || !ok)
		delete brw;
	return ok;
}

int PPView::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	return DefaultCmdProcessor(ppvCmd, pHdr, pBrw);
}

int PPView::DefaultCmdProcessor(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	ZDELETEFAST(P_LastUpdatedObjects);
	if(ppvCmd == PPVCMD_PRINT) {
		Print(pHdr);
		ok = -1;
	}
	else if(ppvCmd == PPVCMD_TOTAL) {
		ViewTotal();
		ok = -1;
	}
	else if(ppvCmd == PPVCMD_DETAIL)
		ok = (Detail(pHdr, pBrw) > 0) ? 1 : -1;
	else if(ppvCmd == PPVCMD_CHANGEFILT)
		ok = (ImplementFlags & implChangeFilt || !P_F) ? -2 : ChangeFilt(0, pBrw);
	else if(P_Obj) {
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		switch(ppvCmd) {
			case PPVCMD_ADDITEM:
				ok = (P_Obj->Edit(&(id = 0), GetEditExtraParam()) == cmOK) ? 1 : -1;
				if(ok > 0 && id) {
					{
						SETIFZ(P_LastUpdatedObjects, new LongArray);
						if(P_LastUpdatedObjects) {
							P_LastUpdatedObjects->clear();
							P_LastUpdatedObjects->add(id);
						}
					}
					if(ImplementFlags & implOnAddSetupPos && pBrw) {
						pBrw->Update();
						pBrw->search2(&id, CMPF_LONG, srchFirst, 0);
						ok = -1; // pBrw не должен теперь обновлять содержимое таблицы
					}
				}
				break;
			case PPVCMD_DEFAULT:
			case PPVCMD_EDITITEM:
				if(pHdr) {
					ok = (id && P_Obj->Edit(&id, GetEditExtraParam()) == cmOK) ? 1 : -1;
					if(ok > 0 && id) {
						SETIFZ(P_LastUpdatedObjects, new LongArray);
						if(P_LastUpdatedObjects) {
							P_LastUpdatedObjects->clear();
							P_LastUpdatedObjects->add(id);
						}
					}
				}
				break;
			case PPVCMD_DELETEITEM:
				if(pHdr) {
					ok = (id && P_Obj->RemoveObjV(id, 0, PPObject::rmv_default, 0) > 0) ? 1 : -1;
					if(ok > 0 && id) {
						SETIFZ(P_LastUpdatedObjects, new LongArray);
						if(P_LastUpdatedObjects) {
							P_LastUpdatedObjects->clear();
							P_LastUpdatedObjects->add(id);
						}
					}
				}
				break;
			case PPVCMD_SYSJ:
				if(pHdr && id) {
					ViewSysJournal(P_Obj->Obj, id, 0);
					ok = -1;
				}
				break;
		}
	}
	else if(ppvCmd == PPVCMD_DEFAULT) // P_Obj == 0
		ok = (Detail(pHdr, pBrw) > 0) ? 1 : -1;
	return ok;
}

/*virtual*/int PPView::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, PPViewBrowser * pBrw, void * extraProcPtr)
{
	return -1;
}

/*virtual*/int PPView::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, ServerInstId, rBuf));
	THROW(P_F->Serialize(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}
//
//
//
PPViewBrowser::PPViewBrowser(uint brwId, DBQuery * pQ, PPView * pV, int dataOwner) : BrowserWindow(brwId, pQ), RefreshTimer(0),
	P_View(pV), VbState(dataOwner ? vbsDataOwner : 0), P_ComboBox(0), P_InputLine(0), H_ComboFont(0)
{
	Advise();
}

PPViewBrowser::PPViewBrowser(uint brwId, SArray * pQ, PPView * pV, int dataOwner) : BrowserWindow(brwId, pQ), RefreshTimer(0),
	P_View(pV), VbState(dataOwner ? vbsDataOwner : 0), P_ComboBox(0), P_InputLine(0), H_ComboFont(0)
{
	Advise();
}

PPViewBrowser::~PPViewBrowser()
{
	if(VbState & vbsDataOwner) {
		assert(P_View->IsConsistent());
		delete P_View;
	}
	if(H_ComboFont)
		::DeleteObject(H_ComboFont);
	ZDELETE(P_ComboBox);
	ZDELETE(P_InputLine);
	for(uint i = 0; i < TempGoodsGrpList.getCount(); i++) {
		const PPID grp_id = TempGoodsGrpList.at(i);
		PPObjGoodsGroup::RemoveDynamicAlt(grp_id, (long)this);
	}
	Unadvise();
}

int PPViewBrowser::ResetDataOwnership()
{
	if(VbState & vbsDataOwner) {
		VbState &= ~vbsDataOwner;
		return 1;
	}
	else
		return -1;
}

int PPViewBrowser::InsColumnWord(int atPos, uint wordId, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	SString temp_buf;
	PPGetWord(wordId, 0, temp_buf);
	return insertColumn(atPos, temp_buf, fldNo, typ, fmt, opt);
}

int PPViewBrowser::InsColumn(int atPos, const char * pText, uint fldNo, TYPEID typ, long fmt, uint opt)
{
	SString temp_buf;
	if(pText && pText[0] == '@')
		PPLoadString(pText+1, temp_buf);
	else
		temp_buf = pText;
	return insertColumn(atPos, temp_buf, fldNo, typ, fmt, opt);
}

int PPViewBrowser::Advise(int kind, PPID action, PPID objType, long flags)
{
	int    ok = -1;
	long   cookie = 0;
	PPAdviseBlock adv_blk;
	adv_blk.Kind = kind;
	adv_blk.TId = DS.GetConstTLA().GetThreadID();
	adv_blk.Action = action; // @v10.8.12
	adv_blk.ObjType = objType;
	adv_blk.Proc = PPViewBrowser::HandleNotifyEvent;
	adv_blk.ProcExtPtr = this;
	ok = DS.Advise(&cookie, &adv_blk);
	if(ok > 0)
		Cookies.addUnique(cookie);
	return ok;
}

int PPViewBrowser::Advise()
{
	int    ok = 1;
	PPObject * p_obj = P_View ? P_View->GetObj() : 0;
	if(p_obj)
		ok = Advise(PPAdviseBlock::evDirtyCacheBySysJ, 0, p_obj->Obj, 0);
	return ok;
}

void PPViewBrowser::Unadvise()
{
	for(uint i = 0; i < Cookies.getCount(); i++)
		DS.Unadvise(Cookies.at(i));
}

/*static*/int PPViewBrowser::HandleNotifyEvent(int kind, const PPNotifyEvent * pEv, void * extraProcPtr)
{
	int    ok = -1;
	if(pEv) {
		PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraProcPtr);
		if(p_brw && p_brw->P_View)
			ok = p_brw->P_View->HandleNotifyEvent(kind, pEv, p_brw, extraProcPtr);
	}
	return ok;
}

void PPViewBrowser::Update() { updateView(); }
void PPViewBrowser::updateView() { Refresh(); }

int PPViewBrowser::SetRefreshPeriod(long period)
{
	if(period >= 0 && period < 86400L) {
		RefreshTimer.Restart((uint32)period * 1000);
		return 1;
	}
	else
		return 0;
}

int PPViewBrowser::SetTempGoodsGrp(PPID grpID)
{
	int    ok = -1;
	if(PPObjGoodsGroup::SetDynamicOwner(grpID, 0, (long)this) > 0) {
		TempGoodsGrpList.addUnique(grpID);
		ok = 1;
	}
	return ok;
}

void PPViewBrowser::Helper_SetAllColumnsSortable()
{
	BrowserDef * p_def = getDef();
	if(p_def)
		for(uint cidx = 0; cidx < p_def->getCount(); cidx++)
			p_def->at(cidx).Options |= BCO_SORTABLE;
}

int PPViewBrowser::getCurHdr(void * pHdr)
{
	const void * p_row = getCurItem();
	if(p_row) {
		if(pHdr)
			*static_cast<long *>(pHdr) = *static_cast<const long *>(p_row);
		return 1;
	}
	return 0;
}

int PPViewBrowser::Export()
{
	int    ok = 1;
	SString name, path;
	BrowserDef * p_def = getDef();
	ComExcelApp * p_app = 0;
	ComExcelWorksheet  * p_sheet  = 0;
	ComExcelWorksheets * p_sheets = 0;
	ComExcelWorkbook   * p_wkbook = 0;
	PPWaitStart();
	if(p_def) {
		long   sheets_count = 0, beg_row = 1, cn_count = p_def->getCount();
		long   i = 0;
		SString dec;
		SString fmt, fmt_rus;
		SString temp_buf;
		PPIDArray width_ary;
		{
			TCHAR  li_buf[64];
			::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, li_buf, SIZEOFARRAY(li_buf));
			dec.Cat(SUcSwitch(li_buf));
		}
		THROW_MEM(p_app = new ComExcelApp);
		THROW(p_app->Init() > 0);
		THROW(p_wkbook = p_app->AddWkbook());
		THROW(p_sheets = p_wkbook->Get());
		(name = getTitle()).Transf(CTRANSF_INNER_TO_OUTER);
		name.ReplaceChar('*', '#'); // Замена запрещенного символа в названии, если таковой имеется
		sheets_count = p_sheets->GetCount();
		for(i = sheets_count; i > 1; i--)
			THROW(p_sheets->Delete(i) > 0);
		THROW(p_sheet = p_sheets->Get(1L));
		THROW(p_sheet->SetName(name));
		// Выводим название групп столбцов
		for(i = 0; i < (long)p_def->GetGroupCount(); i++) {
			const BroGroup * p_grp = p_def->GetGroup(i);
			(temp_buf = p_grp->P_Text).Transf(CTRANSF_INNER_TO_OUTER);
			THROW(p_sheet->SetValue(beg_row, p_grp->First + 1, temp_buf) > 0);
			THROW(p_sheet->SetBold(beg_row, p_grp->First + 1, 1) > 0);
			/*
			if(beg_row == 1)
				beg_row++;
			*/
		}
		if(p_def->GetGroupCount())
			beg_row++;
		// Выводим название столбцов
		for(i = 0; i < cn_count; i++) {
			const BroColumn & r_c = p_def->at(i);
			const long type = GETSTYPE(r_c.T);
			(temp_buf = r_c.text).Transf(CTRANSF_INNER_TO_OUTER);
			width_ary.add((PPID)temp_buf.Len());
			fmt.Z();
			fmt_rus.Z();
			if(type == S_DATE) {
				// @v11.0.4 fmt.Cat("DD/MM/YY;@");
				// @v11.0.4 fmt_rus.Cat("ДД/ММ/ГГ;@");
				PPLoadString("fmt_excel_date", fmt); // @v11.0.4 
				PPLoadString("fmt_excel_date_ru", fmt_rus); // @v11.0.4 
				fmt_rus.Transf(CTRANSF_INNER_TO_OUTER); // @v11.0.4
			}
			else if(oneof3(type, S_INT, S_UINT, S_AUTOINC))
				fmt.CatChar('0');
			else if(type == S_FLOAT) {
				size_t prec = SFMTPRC(r_c.format);
				fmt_rus = fmt.CatChar('0').CatChar(dec.C(0)).CatCharN('0', prec);
			}
			else
				fmt_rus = fmt.CatChar('@');
			if(p_sheet->SetColumnFormat(i + 1, fmt) <= 0)
				p_sheet->SetColumnFormat(i + 1, fmt_rus);
			THROW(p_sheet->SetCellFormat(beg_row, i + 1, "@") > 0);
			THROW(p_sheet->SetValue(beg_row, i + 1, temp_buf) > 0);
			THROW(p_sheet->SetBold(beg_row, i + 1, 1) > 0);
		}
		beg_row++;
		if(p_def->top() > 0) {
			long row = 0;
			SString val_buf;
			do {
				PROFILE_START
				for(long cn = 0; cn < cn_count; cn++) {
					COLORREF color;
					p_def->getFullText(p_def->_curItem(), cn, val_buf);
					val_buf.Strip().Transf(CTRANSF_INNER_TO_OUTER);
					if(GETSTYPE(p_def->at(cn).T) == S_FLOAT) {
						val_buf.ReplaceChar('.', dec.C(0));
					}
					THROW(p_sheet->SetValue(row + beg_row + 1, cn + 1, val_buf) > 0);
					if(GetCellColor(p_def->_curItem(), cn, &color) > 0)
						THROW(p_sheet->SetColor(row + beg_row + 1, cn + 1, color) > 0);
					if(width_ary.at(cn) < (long)val_buf.Len())
						width_ary.at(cn) = (PPID)val_buf.Len();
				}
				row++;
				PROFILE_END
			} while(p_def->step(1) > 0 && row < (USHRT_MAX-beg_row));
			for(i = 0; i < (long)width_ary.getCount(); i++)
				THROW(p_sheet->SetColumnWidth(i + 1, width_ary.at(i) + 2) > 0);
		}
		WMHScroll(SB_VERT, SB_BOTTOM, 0);
	}
	CATCHZOKPPERR
	if(p_wkbook) {
		PPGetPath(PPPATH_LOCAL, path);
		path.SetLastSlash();
		createDir(path);
		path.Cat(name.ReplaceChar('/', ' ')).Cat(".xls");
		SFile::Remove(path);
		p_wkbook->_SaveAs(path);
		p_wkbook->_Close();
		ZDELETE(p_wkbook);
	}
	PPWaitStop();
	ZDELETE(p_sheets);
	ZDELETE(p_sheet);
	ZDELETE(p_app);
	if(ok > 0)
		::ShellExecute(0, _T("open"), SUcSwitch(path), NULL, NULL, SW_SHOWNORMAL);
	return ok;
}

int PPViewBrowser::GetToolbarComboData(PPID * pID)
{
	return P_ComboBox ? (P_ComboBox->TransmitData(-1, pID), 1) : 0;
}

int PPViewBrowser::GetToolbarComboRect(RECT * pRect)
{
	int    ok = -1;
	HWND   hwnd = P_Toolbar ? P_Toolbar->GetToolbarHWND() : 0;
	RECT   rect;
	MEMSZERO(rect);
	if(hwnd) {
		DWORD  btns_sz    = static_cast<DWORD>(::SendMessage(hwnd, TB_GETBUTTONSIZE, 0, 0));
		DWORD  btns_count = static_cast<DWORD>(::SendMessage(hwnd, TB_BUTTONCOUNT,   0, 0));
		DWORD  padding    = static_cast<DWORD>(::SendMessage(hwnd, TB_GETPADDING,    0, 0));
		RECT   tb_rect;
		MEMSZERO(tb_rect);
		rect.top    = 2;
		rect.left = LOWORD(btns_sz) * btns_count + LOWORD(padding);
		rect.right  = 300;
		rect.bottom = 20;
		GetClientRect(hwnd, &tb_rect);
		rect.right = (rect.left + rect.right + rect.bottom > tb_rect.left + tb_rect.right) ?
			(tb_rect.right - rect.left - rect.bottom - 1) : rect.right;
		ok = 1;
	}
	ASSIGN_PTR(pRect, rect);
	return ok;
}

/*static int CreateComboBoxComplex(uint cbOptions, uint maxTextLen, TGroup * pOwner, TInputLine ** ppInp, ComboBox ** pCb)
{
	int    ok = 1;
	TRect  r;
	ComboBox * p_cb  = new ComboBox(r, cbOptions); //cbxAllowEmpty|cbxDisposeData|cbxListOnly
	TInputLine * p_inp = new TInputLine(r, S_ZSTRING, MKSFMT(maxTextLen, 0));
	p_cb->P_Owner = pOwner;
	p_cb->setState(sfMsgToParent, false);
	p_inp->setState(sfMsgToParent, false);
	p_inp->Parent = parent;
	p_inp->SetId(CTL_TOOLBAR_INPUTLI);
	p_cb->SetId(CTL_TOOLBAR_BTN);
	p_cb->Parent  = parent;
	p_inp->setupCombo(P_ComboBox);
	p_cb->handleWindowsMessage(WM_INITDIALOG,  0, 0);
	p_inp->handleWindowsMessage(WM_INITDIALOG, 0, 0);
	return ok;
}*/

void * PPViewBrowser::Helper_InitToolbarCombo()
{
#define CTL_TOOLBAR_BTN     1000
#define CTL_TOOLBAR_INPUTLI (WINDOWS_ID_BIAS + 1)
	HWND   parent = P_Toolbar ? P_Toolbar->GetToolbarHWND() : 0;
	if(parent) {
		TRect  r;
		DWORD  style = WS_VISIBLE|WS_CHILD|WS_BORDER|WS_CLIPSIBLINGS;
		RECT   rect;
		HWND   hwnd_li = 0;
		SString font_face;
		GetToolbarComboRect(&rect);
		if(P_InputLine) {
			::DestroyWindow(P_InputLine->getHandle());
			ZDELETE(P_InputLine);
		}
		if(P_ComboBox) {
			::DestroyWindow(P_ComboBox->getHandle());
			ZDELETE(P_ComboBox);
		}
		hwnd_li = ::CreateWindowEx(0, _T("EDIT"), 0, style|ES_AUTOHSCROLL|ES_READONLY,
			rect.left, rect.top, rect.right, rect.bottom, parent, (HMENU)CTL_TOOLBAR_INPUTLI, TProgram::GetInst(), 0);
		::CreateWindowEx(0, _T("BUTTON"), 0, style|BS_PUSHBUTTON|BS_BITMAP|BS_FLAT, rect.left + rect.right - 1,
			rect.top, rect.bottom, rect.bottom, parent, (HMENU)CTL_TOOLBAR_BTN, TProgram::GetInst(), 0);
		PPGetSubStr(PPTXT_FONTFACE, PPFONT_MSSANSSERIF, font_face);
		ZDeleteWinGdiObject(&H_ComboFont);
		H_ComboFont = TView::setFont(hwnd_li, font_face, 8);
		P_InputLine = new TInputLine(r, S_ZSTRING, MKSFMT(128, 0));
		P_ComboBox  = new ComboBox(r, cbxAllowEmpty|cbxDisposeData|cbxListOnly, P_InputLine);
		P_ComboBox->P_Owner = this;
		P_ComboBox->setState(sfMsgToParent, false);
		P_InputLine->setState(sfMsgToParent, false);
		P_InputLine->Parent = parent;
		P_InputLine->SetId(CTL_TOOLBAR_INPUTLI);
		P_ComboBox->SetId(CTL_TOOLBAR_BTN);
		P_ComboBox->Parent  = parent;
		P_ComboBox->handleWindowsMessage(WM_INITDIALOG,  0, 0);
		P_InputLine->handleWindowsMessage(WM_INITDIALOG, 0, 0);
	}
	return parent;
#undef CTL_TOOLBAR_BTN
#undef CTL_TOOLBAR_INPUTLI
}

int PPViewBrowser::Helper_SetupToolbarStringCombo(uint strID, PPID id)
{
	int    ok = -1;
	HWND   hw_parent = static_cast<HWND>(Helper_InitToolbarCombo());
	if(hw_parent) {
		SString item_buf, id_buf, txt_buf;
		SString line_buf;
		PPLoadText(strID, line_buf);
		StrAssocArray * p_list = new StrAssocArray();
		if(p_list) {
			for(int idx = 0; PPGetSubStr(line_buf, idx, item_buf) > 0; idx++) {
				long   id = 0;
				if(item_buf.Divide(',', id_buf, txt_buf) > 0)
					id = id_buf.ToLong();
				else {
					id = (idx+1);
					txt_buf = item_buf;
				}
				p_list->Add(id, txt_buf);
			}
			P_ComboBox->setListWindow(CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify), id);
		}
		::UpdateWindow(hw_parent);
		ok = 1;
	}
	return ok;
}

int PPViewBrowser::Helper_SetupToolbarCombo(PPID objType, PPID id, uint flags, void * extraPtr, const PPIDArray * pObjList)
{
	int    ok = -1;
	HWND   hw_parent = static_cast<HWND>(Helper_InitToolbarCombo());
	if(hw_parent) {
		if(pObjList) {
			StrAssocArray * p_list = new StrAssocArray;
			if(p_list) {
				SString name_buf;
				for(uint i = 0; i < pObjList->getCount(); i++) {
					const PPID obj_id = pObjList->get(i);
					if(obj_id) {
						switch(objType) {
							case PPOBJ_PERSON: GetPersonName(obj_id, name_buf); break;
							case PPOBJ_ARTICLE: GetArticleName(obj_id, name_buf); break;
							case PPOBJ_GOODS: GetGoodsName(obj_id, name_buf); break;
							default: GetObjectName(objType, obj_id, name_buf, 0); break;
						}
						p_list->Add(obj_id, name_buf);
					}
				}
				ListWindow * p_lw = CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify);
				if(p_lw) {
					P_ComboBox->setListWindow(p_lw);
					if(id)
						P_ComboBox->TransmitData(+1, &id);
				}
				else {
					ZDELETE(p_list);
					ok = 0;
				}
			}
			else
				ok = 0;
		}
		else
			SetupPPObjCombo(P_ComboBox, objType, id, flags, extraPtr);
		::UpdateWindow(hw_parent);
		ok = 1;
	}
	return ok;
}

int PPViewBrowser::SetupToolbarCombo(PPID objType, PPID id, uint flags, void * extraPtr)
	{ return Helper_SetupToolbarCombo(objType, id, flags, extraPtr, 0); }
int PPViewBrowser::SetupToolbarCombo(PPID objType, PPID id, uint flags, const PPIDArray & rObjList)
	{ return Helper_SetupToolbarCombo(objType, id, flags, 0, &rObjList); }
int PPViewBrowser::SetupToolbarStringCombo(uint strId, PPID id)
	{ return Helper_SetupToolbarStringCombo(strId, id); }

IMPL_HANDLE_EVENT(PPViewBrowser)
{
	if(this && this->IsConsistent()) { // @v10.3.9
		int    r;
		int    skip_inherited_processing = 0;
		if(TVKEYDOWN) {
			if(oneof3(TVKEY, kbEnter, kbIns, kbDel))
				skip_inherited_processing = 1;
			else if(P_View && VbState & vbsTagPreKey) {
				const void * p_row = getCurItem();
				if(p_row) { // Если текущий элемент не выделен то нечего и редактировать будет - пропускаем
					PPObject * p_obj = P_View->GetObj();
					const PPID obj_type = p_obj ? p_obj->Obj : 0;
					PPObjTag tag_obj;
					LongArray tag_list;
					KeyDownCommand kc;
					kc.Code  = LoWord(event.keyDown.keyCode);
					kc.State = HiWord(event.keyDown.keyCode);
					if(oneof2(kc.State, 0, KeyDownCommand::stateShift) && isasciialpha(static_cast<char>(kc.Code))) {
						kc.Code = toupper(kc.Code);
					}
					uint32 final_code = MakeLong(kc.Code, kc.State);
					if(tag_obj.GetListByHotKey(/*event.keyDown.keyCode*/final_code, obj_type, tag_list) > 0) {
						assert(tag_list.getCount());
						P_View->ProcessCommand(PPVCMD_QUICKTAGEDIT, &tag_list, this);
					}
				}
				VbState &= ~vbsTagPreKey;
				clearEvent(event);
			}
			else if(TVKEY == kbF10 && !(VbState & vbsKbF10)) { // @v11.2.8 !(VbState & vbsKbF10)
				VbState |= vbsKbF10;
				VbState &= ~vbsTagPreKey;
			}
			else {
				const  int c = TVCHR;
				if(c) {
					if(c == '/' && P_View && P_View->GetImplementFlags() & PPView::implUseQuickTagEditFunc && !(VbState & vbsTagPreKey)) { // @v11.2.8
						VbState |= vbsTagPreKey;
					}
					else {
						if(isalnum(c) || c == '*') {
							if(VbState & vbsKbF10 && oneof2(c, 'x', 'X')) {
								Export();
								clearEvent(event);
							}
							else if(P_View) {
								char   temp_buf[32];
								temp_buf[0] = c;
								temp_buf[1] = 0;
								r = P_View->ProcessCommand(PPVCMD_INPUTCHAR, temp_buf, this);
								if(r != -2) {
									if(r > 0)
										updateView();
									clearEvent(event);
								}
							}
						}
						else {
							KeyDownCommand kd;
							char   b[4];
							b[0] = c;
							b[1] = 0;
							SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
							r_temp_buf_u.CopyFromMb_OUTER(b, 1);
							uint   tc = kd.SetCharU(r_temp_buf_u.C(0)) ? kd.GetChar() : 0; 
							if(VbState & vbsKbF10 && oneof2(tc, 'x', 'X')) {
								Export();
								clearEvent(event);
							}
						}
						VbState &= ~vbsTagPreKey;
					}
					VbState &= ~vbsKbF10;
				}
			}
		}
		if(!skip_inherited_processing) {
			/* @v10.3.1
			if(event.isCmd(cmExecute)) {
				const int  r = P_View ? P_View->OnExecBrowser(this) : -1;
				if(r >= 0) {
					clearEvent(event);
					event.message.infoLong = r;
					return;
				}
				else {
					; // Управление передается базовому классу
				}
			}
			*/
			BrowserWindow::handleEvent(event);
		}
		if(!P_View || !P_View->IsConsistent() || (!this || !this->IsConsistent()))
			return;
		if(TVBROADCAST) {
			if(TVCMD == cmIdle) {
				if(RefreshTimer.Check(0)) {
					const int r = P_View->ProcessCommand(PPVCMD_REFRESHBYPERIOD, 0, this);
					if(r == -2 || r > 0)
						Update();
				}
				return; // @v10.5.9 в конце функции стоит clearEvent(event) который препятствует дальнейшей обработке cmIdle
			}
			else if(TVCMD == cmReceivedFocus)
				P_View->ProcessCommand(PPVCMD_RECEIVEDFOCUS, TVINFOVIEW, this);
			else if(TVCMD == cmMouseHover) {
				long   h = 0, v = 0;
				SPoint2S point = *static_cast<const SPoint2S *>(event.message.infoPtr);
				if(ItemByPoint(point, &h, &v)) {
					const void * p_row = getItemByPos(v);
					if(P_View->ProcessCommand(PPVCMD_MOUSEHOVER, p_row, this) > 0)
						updateView();
				}
			}
			else
				return;
		}
		else if(TVCOMMAND) {
			const void * p_row = getCurItem();
			switch(TVCMD) {
				case cmModalPostCreate: // @v10.3.1
					if(P_View->OnExecBrowser(this) == cmCancel)
						BbState |= bbsCancel; // @v10.3.3
					break;
				case cmSort: // @v10.6.4
					if(P_View->ProcessCommand(PPVCMD_USERSORT, p_row, this) > 0)
						updateView();
					break;
				case cmaInsert:
					if(P_View->ProcessCommand(PPVCMD_ADDITEM, p_row, this) > 0)
						updateView();
					break;
				case cmaDelete:
					if(P_View->ProcessCommand(PPVCMD_DELETEITEM, p_row, this) > 0)
						updateView();
					break;
				case cmaEdit:
					r = P_View->ProcessCommand(PPVCMD_EDITITEM, p_row, this);
					if(r > 0)
						updateView();
					else if(r == -2) {
						if(P_View->ProcessCommand(PPVCMD_DETAIL, p_row, this) > 0)
							updateView();
					}
					break;
				case cmCBSelected:
					r = P_View->ProcessCommand(PPVCMD_TB_CBX_SELECTED, p_row, this);
					break;
				case cmResize:
					if(P_InputLine && P_ComboBox) {
						RECT rect;
						if(GetToolbarComboRect(&rect) > 0) {
							SetWindowPos(P_InputLine->getHandle(), 0, rect.left, rect.top, rect.right, rect.bottom, 0);
							SetWindowPos(P_ComboBox->getHandle(), 0, rect.left + rect.right - 1, rect.top, rect.bottom, rect.bottom, 0);
						}
					}
					break;
				case cmNotifyForeignFocus: // @v11.1.12
					r = P_View->ProcessCommand(PPVCMD_FOREIGNFOCUCNOTIFICATION, event.message.infoPtr, this);
					break;
				default:
					return;
			}
		}
		else if(TVKEYDOWN) {
			uint   cmd = 0;
			if(translateKeyCode(TVKEY, &cmd) > 0) {
				const void * p_row = getCurItem();
				if(P_View->ProcessCommand(cmd, p_row, this) > 0)
					updateView();
			}
			else {
				char   temp_buf[32];
				temp_buf[0] = static_cast<char>(TVKEY);
				temp_buf[1] = 0;
				if(temp_buf[0] && P_View->ProcessCommand(PPVCMD_INPUTCHAR, temp_buf, this) > 0) // @v10.8.10 @fix (temp_buf[0] &&)
					updateView();
			}
		}
		else
			return;
		clearEvent(event);
	}
}
//
//
//
PPTimeChunkBrowser::PPTimeChunkBrowser() : STimeChunkBrowser()
{
}

/*virtual*/int PPTimeChunkBrowser::ExportToExcel()
{
	int    ok = -1;
	ComExcelApp * p_app = 0;
	ComExcelWorksheet  * p_sheet  = 0;
	ComExcelWorksheets * p_sheets = 0;
	ComExcelWorkbook   * p_wkbook = 0;
	SString name; // Name of the excel workbook
	SString path; // Path to the created excel workbook
	if(P.ViewType == P.vHourDay) {
		long   sheets_count = 0;
		long   beg_row = 1;
		long   cn_count = 0; // p_def->getCount();
		long   i = 0;
		//
		const char * p_fontface_tnr = "Times New Roman";
		long   column = 0;
		long   row = 0;
		SString temp_buf;
		SString fmt;
		SString out_buf;
		SString dow_buf;
		SString dec;
		THROW(p_app = new ComExcelApp);
		THROW(p_app->Init() > 0);
		THROW(p_wkbook = p_app->AddWkbook());
		THROW(p_sheets = p_wkbook->Get());
		name = "timetable";
		sheets_count = p_sheets->GetCount();
		for(i = sheets_count; i > 1; i--)
			THROW(p_sheets->Delete(i) > 0);
		THROW(p_sheet = p_sheets->Get(1L));
		THROW(p_sheet->SetName(name));
		{
			char   buf[64];
			::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buf, sizeof(buf));
			dec.Cat(buf);
		}
		{
			row = 1;
			column = 2;
			for(long quant = 0; ; quant++) {
				const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
				if(dt <= St.Bounds.Finish.d) {
					if(IsQuantVisible(quant)) {
						GetDayOfWeekText(dowtRuFull, dayofweek(&dt, 1), dow_buf);
						temp_buf.Z().Cat(dt, DATF_DMY).Space().Cat(dow_buf);
						p_sheet->SetBgColor(row, column, Ptb.GetColor(colorHeader));
						THROW(p_sheet->SetCellFormat(row, column, "@") > 0);
						THROW(p_sheet->SetValue(row, column, temp_buf) > 0);
						THROW(p_sheet->SetBold(row, column, 1) > 0);
						column++;
					}
				}
				else
					break;
			}
		}
		{
			uint   time_quant = 15 * 60; // 15 minuts
			STimeChunkAssocArray chunk_list(0);
			SString cell_buf;
			column = 1;
			for(long quant = 0; ; quant++, column++) {
				const  LDATE  dt = plusdate(St.Bounds.Start.d, quant);
				const long _col = column+1; // Первый столбец (column) - с заголовками, _col - столбец с данными
				if(dt <= St.Bounds.Finish.d) {
					row = 2;
					STimeChunkGrid::Color clr;
					STimeChunkAssoc __last_chunk;
					long   last_chunk_row = 0;
					long   last_chunk_finish_row = 0;
					for(uint time_band = 0; time_band < 24 * 3600; time_band += time_quant, row++) {
						LTIME   tm_start;
						LTIME   tm_end;
						tm_start.settotalsec(time_band);
						tm_end.settotalsec(time_band+time_quant-1);
						if(column == 1) {
							temp_buf.Z().Cat(tm_start, TIMF_HM);
							p_sheet->SetBgColor(row, column, Ptb.GetColor(colorHeader));
							THROW(p_sheet->SetCellFormat(row, column, "@") > 0);
							THROW(p_sheet->SetValue(row, column, temp_buf) > 0);
							THROW(p_sheet->SetBold(row, column, 1) > 0);
						}
						{
							STimeChunk range;
							range.Start.Set(dt, tm_start);
							range.Finish.Set(dt, tm_end);
							chunk_list.clear();
							if(P_Data->GetChunksByTime(range, chunk_list) > 0 && chunk_list.getCount()) {
								cell_buf.Z();
								const uint _chunk_count = chunk_list.getCount();
								if(_chunk_count > 1)
									chunk_list.Sort(0);
								for(uint i = 0; i < _chunk_count; i++) {
									const STimeChunkAssoc * p_chunk = static_cast<const STimeChunkAssoc *>(chunk_list.at(i));
									if(p_chunk) {
										if(i == 0 && __last_chunk.Id && __last_chunk.Id != p_chunk->Id) {
											GetChunkText(__last_chunk.Id, temp_buf);
											ComExcelRange * p_range = 0;
											THROW(p_sheet->SetValue(last_chunk_row, _col, temp_buf) > 0);
											if(row > (last_chunk_row+1) && (p_range = p_sheet->GetRange(last_chunk_row, _col, /*row-1*/last_chunk_finish_row, _col)) != 0) {
												p_range->DoMerge();
												p_range->SetBgColor(GetChunkColor(&__last_chunk, &clr) ? clr.C : Ptb.GetColor(colorMain));
												//p_range->SetValue(temp_buf);
											}
											else {
												p_sheet->SetBgColor(last_chunk_row, _col, GetChunkColor(&__last_chunk, &clr) ? clr.C : Ptb.GetColor(colorMain));
												//THROW(p_sheet->SetValue(last_chunk_row, _col, temp_buf) > 0);
											}
										}
										if(i == (_chunk_count-1)) {
											if(__last_chunk.Id != p_chunk->Id) {
												__last_chunk = *p_chunk;
												last_chunk_row = row;
											}
											last_chunk_finish_row = row;
										}
									}
								}
							}
						}
					}
					if(__last_chunk.Id) {
						GetChunkText(__last_chunk.Id, temp_buf);
						ComExcelRange * p_range = 0;
						THROW(p_sheet->SetValue(last_chunk_row, _col, temp_buf) > 0);
						if(row > (last_chunk_row+1) && (p_range = p_sheet->GetRange(last_chunk_row, _col, /*row-1*/last_chunk_finish_row, _col)) != 0) {
							p_range->DoMerge();
							p_range->SetBgColor(GetChunkColor(&__last_chunk, &clr) ? clr.C : Ptb.GetColor(colorMain));
							//p_range->SetValue(temp_buf);
						}
						else {
							p_sheet->SetBgColor(last_chunk_row, _col, GetChunkColor(&__last_chunk, &clr) ? clr.C : Ptb.GetColor(colorMain));
							//THROW(p_sheet->SetValue(last_chunk_row, _col, temp_buf) > 0);
						}
					}
				}
				else
					break;
			}
		}
		SLS.QueryPath("local", path);
		path.SetLastSlash();
		createDir(path);
		path.Cat(name.ReplaceChar('/', ' '));//.Cat(".xls");
		SFile::Remove(path);
		p_wkbook->_SaveAs(path);
		p_wkbook->_Close();
		ZDELETE(p_wkbook);
		ok = 1;
	}
	CATCHZOK
	ZDELETE(p_sheets);
	ZDELETE(p_sheet);
	ZDELETE(p_app);
	if(ok > 0 && fileExists(path))
		::ShellExecute(0, _T("open"), SUcSwitch(path), NULL, NULL, SW_SHOWNORMAL); // @unicodeproblem
	return ok;
}
