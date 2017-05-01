// V_ASSET.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2006, 2007, 2009, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(PPViewAsset)
//
IMPLEMENT_PPFILT_FACTORY(Asset); SLAPI AssetFilt::AssetFilt() : PPBaseFilt(PPFILT_ASSET, 0, 1)
{
	SetFlatChunk(offsetof(AssetFilt, ReserveStart),
		offsetof(AssetFilt, Reserve)-offsetof(AssetFilt, ReserveStart)+sizeof(Reserve));
	Init(1, 0);
}

AssetFilt & FASTCALL AssetFilt::operator = (const AssetFilt & s)
{
	Copy(&s, 1);
	return *this;
}

SLAPI PPViewAsset::PPViewAsset() : PPView(0, &Filt, PPVIEW_ASSET)
{
	P_BObj = BillObj;
	P_TempTbl = 0;
	P_GGIter = 0;
}

SLAPI PPViewAsset::~PPViewAsset()
{
	delete P_TempTbl;
	delete P_GGIter;
}

PPBaseFilt * SLAPI PPViewAsset::CreateFilt(void * extraPtr) const
{
	AssetFilt * p_filt = new AssetFilt;
	if(p_filt) {
		p_filt->GoodsGrpID = GObj.GetConfig().AssetGrpID;
	}
	return p_filt;
}

#define GRP_GOODSFILT 1

int SLAPI PPViewAsset::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	int    ok = -1;
	ushort v;
	AssetFilt filt;
	TDialog * dlg = new TDialog(DLG_ASSETFLT);
	THROW(Filt.IsA(pBaseFilt));
	filt = *(AssetFilt *)pBaseFilt;
	THROW(CheckDialogPtrErr(&dlg));
	SetupCalCtrl(CTLCAL_ASSETFLT_PERIOD, dlg, CTL_ASSETFLT_PERIOD, 1);
	SetupCalCtrl(CTLCAL_ASSETFLT_OPERAT, dlg, CTL_ASSETFLT_OPERAT, 2);
	SetPeriodInput(dlg, CTL_ASSETFLT_PERIOD, &filt.Period);
	SetPeriodInput(dlg, CTL_ASSETFLT_OPERAT, &filt.OperPeriod);
	SetupPPObjCombo(dlg, CTLSEL_ASSETFLT_LOC, PPOBJ_LOCATION, filt.LocID, 0, 0);
	dlg->addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_ASSETFLT_GGRP, cmGoodsFilt));
	{
		GoodsFiltCtrlGroup::Rec gf_rec(filt.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
		dlg->setGroupData(GRP_GOODSFILT, &gf_rec);
		if(filt.Flags & AssetFilt::fUnwritedOffOnly)
			v = 0;
		else if(filt.Flags & AssetFilt::fWritedOffOnly)
			v = 2;
		else
			v = 1;
		dlg->setCtrlData(CTL_ASSETFLT_CLOSED, &v);
		dlg->AddClusterAssoc(CTL_ASSETFLT_FLAGS, 0, AssetFilt::fShowClosed);
		dlg->SetClusterData(CTL_ASSETFLT_FLAGS, filt.Flags);
		dlg->AddClusterAssoc(CTL_ASSETFLT_INEXPL,  0,  0);
		dlg->AddClusterAssoc(CTL_ASSETFLT_INEXPL, -1,  0);
		dlg->AddClusterAssoc(CTL_ASSETFLT_INEXPL,  1, +1);
		dlg->AddClusterAssoc(CTL_ASSETFLT_INEXPL,  2, -1);
		dlg->SetClusterData(CTL_ASSETFLT_INEXPL,   filt.Ft_InExpl);
		if(ExecView(dlg) == cmOK) {
			long   temp_long = 0;
			GetPeriodInput(dlg, CTL_ASSETFLT_PERIOD, &filt.Period);
			GetPeriodInput(dlg, CTL_ASSETFLT_OPERAT, &filt.OperPeriod);
			dlg->getCtrlData(CTLSEL_ASSETFLT_LOC, &filt.LocID);
			dlg->getGroupData(GRP_GOODSFILT, &gf_rec);
			filt.GoodsGrpID = gf_rec.GoodsGrpID;
			dlg->getCtrlData(CTL_ASSETFLT_CLOSED, &(v = 0));
			filt.Flags &= ~(AssetFilt::fWritedOffOnly | AssetFilt::fUnwritedOffOnly);
			if(v == 0)
				filt.Flags |= AssetFilt::fUnwritedOffOnly;
			else if(v == 2)
				filt.Flags |= AssetFilt::fWritedOffOnly;
			dlg->GetClusterData(CTL_ASSETFLT_FLAGS, &filt.Flags);
			dlg->GetClusterData(CTL_ASSETFLT_INEXPL, &temp_long);
			filt.Ft_InExpl = (int16)temp_long;
			*(AssetFilt *)pBaseFilt = filt;
			ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempAsset);

int SLAPI PPViewAsset::MakeItem(PPID lotID, BExtInsert * pBei, int use_ta)
{
	int    ok = 1;
	PPObjAssetWrOffGrp awog_obj;
	PPAssetWrOffGrp    awog_rec;
	ReceiptTbl::Rec lot_rec, org_lot_rec;
	Goods2Tbl::Rec goods_rec;
	TempAssetTbl::Rec asset_rec;
	double qtty = 1.0;
	double rest = 0.0;
	double start_cost = 0.0;  // Балансовая стоимость после ввода в эксплуатацию
	double deprec_beg = 0.0;  // Амортизация до операционного периода
	double deprec_cont = 0.0; // Амортизация в течении операционного периода
	LDATE  dt = ZERODATE;
	LDATE  lot_date = ZERODATE;
	SString temp_buf;
	DateRange period;
	period.SetZero();
	THROW(P_BObj->trfr->Rcpt.Search(lotID, &org_lot_rec) > 0);
	qtty = org_lot_rec.Quantity;
	rest = org_lot_rec.Rest;
	lot_rec = org_lot_rec;
	start_cost = R5(lot_rec.Cost) * qtty;
	THROW(GObj.Search(labs(lot_rec.GoodsID), &goods_rec) > 0);
	MEMSZERO(asset_rec);
	asset_rec.GoodsID = goods_rec.ID;
	asset_rec.LotID   = lot_rec.ID;
	asset_rec.GrpID   = goods_rec.ParentID;
	STRNSCPY(asset_rec.Name, goods_rec.Name);
	asset_rec.Dt = lot_rec.Dt;
	P_BObj->GetSerialNumberByLot(lot_rec.ID, temp_buf, 1);
	temp_buf.CopyTo(asset_rec.Serial, sizeof(asset_rec.Serial));
	if(goods_rec.WrOffGrpID && awog_obj.Search(goods_rec.WrOffGrpID, &awog_rec) > 0)
		asset_rec.WrOffTerm = (int16)awog_rec.WrOffTerm;
	{
		DateIter iter;
		PPID   lot_id = lot_rec.ID;
		int    op_code = 0;
		int    in_expl = 0;
		TransferTbl::Rec trfr_rec;
		while(P_BObj->trfr->EnumAssetOp(&lot_id, &iter, &op_code, &trfr_rec) > 0) {
			if(oneof2(op_code, ASSTOPC_RCPT, ASSTOPC_RCPTEXPL))
				lot_date = iter.dt;
			if(oneof2(op_code, ASSTOPC_RCPTEXPL, ASSTOPC_EXPL)) {
				if(!in_expl) {
					asset_rec.ExplDt = iter.dt;
					lot_rec = org_lot_rec;
					P_BObj->trfr->GetLotPrices(&lot_rec, iter.dt, 0);
					start_cost = R5(lot_rec.Cost) * qtty;
					in_expl = 1;
				}
				//break;
			}
			else if(op_code == ASSTOPC_DEPREC) {
				if(iter.dt < Filt.OperPeriod.low || Filt.OperPeriod.CheckDate(iter.dt)) {
					lot_rec = org_lot_rec;
					P_BObj->trfr->GetLotPrices(&lot_rec, iter.dt, trfr_rec.OprNo);
					double d = trfr_rec.Price - R5(lot_rec.Price);
					if(iter.dt < Filt.OperPeriod.low)
						deprec_beg += d;
					else if(Filt.OperPeriod.CheckDate(iter.dt))
						deprec_cont += d;
				}
			}
		}
	}
	if(Filt.Ft_InExpl > 0) {
		if(asset_rec.ExplDt) {
			if(!Filt.OperPeriod.CheckDate(asset_rec.ExplDt))
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(Filt.Ft_InExpl < 0) {
		if(asset_rec.ExplDt && Filt.OperPeriod.CheckDate(asset_rec.ExplDt))
			ok = -1;
	}
	if(ok > 0) {
		PPTransaction tra(ppDbDependTransaction, use_ta);
		THROW(tra);
		dt = MAX(Filt.OperPeriod.low, asset_rec.ExplDt);
		lot_rec = org_lot_rec;
		P_BObj->trfr->GetLotPrices(&lot_rec, dt, 0);
		asset_rec.Cost  = R5(lot_rec.Cost)  * qtty;
		asset_rec.Price = R5(lot_rec.Price) * qtty;
		dt = NZOR(Filt.OperPeriod.upp, MAXDATE);
		lot_rec = org_lot_rec;
		P_BObj->trfr->GetLotPrices(&lot_rec, dt, 0);
		asset_rec.Cost2  = R5(lot_rec.Cost) * rest;  // @v6.1.12 qtty -> rest
		asset_rec.Price2 = R5(lot_rec.Price) * rest; // @v6.1.12 qtty -> rest
		{
			double tax_factor = 1.0;
			PPID   tax_grp_id = lot_rec.InTaxGrpID;
			int    vat_free = IsLotVATFree(lot_rec);
			if(tax_grp_id == 0) {
				Goods2Tbl::Rec temp_goods_rec;
				if(GObj.Fetch(goods_rec.ID, &temp_goods_rec) > 0)
					tax_grp_id = temp_goods_rec.TaxGrpID;
			}
			GObj.MultTaxFactor(goods_rec.ID, &tax_factor);
			GTaxVect vect;
			PPGoodsTaxEntry gtx;

			// Calculating cost without VAT
			if(GObj.GTxObj.Fetch(tax_grp_id, lot_date, 0L, &gtx) > 0) {
				if(lot_rec.Flags & LOTF_COSTWOVAT) {
					GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &start_cost, 1, vat_free);
					GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &asset_rec.Cost, 1, vat_free);
					GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &asset_rec.Price, 1, vat_free);
					GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &asset_rec.Cost2, 1, vat_free);
					GObj.AdjCostToVat(0, tax_grp_id, lot_date, tax_factor, &asset_rec.Price2, 1, vat_free);
				}
				long   amt_fl = ~GTAXVF_SALESTAX;
				long   excl_fl = (vat_free > 0) ? GTAXVF_VAT : 0;
				vect.Calc_(&gtx, start_cost, tax_factor, amt_fl, excl_fl);
				start_cost -= vect.GetValue(GTAXVF_VAT);
				vect.Calc_(&gtx, asset_rec.Cost, tax_factor, amt_fl, excl_fl);
				asset_rec.Cost -= vect.GetValue(GTAXVF_VAT);
				vect.Calc_(&gtx, asset_rec.Price, tax_factor, amt_fl, excl_fl);
				asset_rec.Price -= vect.GetValue(GTAXVF_VAT);
				vect.Calc_(&gtx, asset_rec.Cost2, tax_factor, amt_fl, excl_fl);
				asset_rec.Cost2 -= vect.GetValue(GTAXVF_VAT);
				vect.Calc_(&gtx, asset_rec.Price2, tax_factor, amt_fl, excl_fl);
				asset_rec.Price2 -= vect.GetValue(GTAXVF_VAT);
				//
				vect.Calc_(&gtx, deprec_beg, tax_factor, amt_fl, excl_fl);
				deprec_beg -= vect.GetValue(GTAXVF_VAT);
				vect.Calc_(&gtx, deprec_cont, tax_factor, amt_fl, excl_fl);
				deprec_cont -= vect.GetValue(GTAXVF_VAT);
				//
			}
		}
		/*
		asset_rec.Deprec  = start_cost - asset_rec.Price;
		asset_rec.Deprec2 = start_cost - asset_rec.Price2;
		asset_rec.DiffCost = asset_rec.Cost2 - asset_rec.Cost;
		asset_rec.DiffDeprec = asset_rec.Deprec2 - asset_rec.Deprec;
		*/
		asset_rec.Deprec  = deprec_beg;
		asset_rec.Deprec2 = (rest != 0.0) ? (deprec_beg + deprec_cont) : 0.0;
		asset_rec.DiffDeprec = deprec_cont;
		asset_rec.DiffCost = asset_rec.Cost2 - asset_rec.Cost;
		if(pBei) {
			THROW_DB(pBei->insert(&asset_rec));
		}
		else {
			TempAssetTbl::Key0 k0;
			k0.LotID = lotID;
			if(P_TempTbl->searchForUpdate(0, &k0, spEq)) {
				THROW_DB(P_TempTbl->updateRecBuf(&asset_rec)); // @sfu
			}
			else {
				THROW_DB(P_TempTbl->insertRecBuf(&asset_rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewAsset::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	Filt.Period.Actualize(ZERODATE);
	THROW(AdjustPeriodToRights(Filt.Period, 0));
	Filt.OperPeriod.Actualize(ZERODATE);
	THROW(AdjustPeriodToRights(Filt.OperPeriod, 0));
	ZDELETE(P_TempTbl);
	{
		ReceiptCore * p_rcpt = & P_BObj->trfr->Rcpt;
		Goods2Tbl::Rec goods_rec;
		ReceiptTbl::Rec lot_rec;
		GoodsIterator giter;
		GoodsFilt     flt;
		PPObjAssetWrOffGrp awog_obj;
		PPAssetWrOffGrp    awog_rec;
		THROW(P_TempTbl = CreateTempFile());
		{
			BExtInsert bei(P_TempTbl);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			flt.GrpID = Filt.GoodsGrpID;
			for(giter.Init(&flt, 0); giter.Next(&goods_rec) > 0; PPWaitPercent(giter.GetIterCounter())) {
				if(goods_rec.WrOffGrpID && awog_obj.Search(goods_rec.WrOffGrpID, &awog_rec) > 0) {
					for(DateIter diter(&Filt.Period); p_rcpt->EnumByGoods(goods_rec.ID, &diter, &lot_rec) > 0;) {
						if(Filt.LocID && lot_rec.LocID != Filt.LocID)
							continue;
						if(Filt.Flags & AssetFilt::fUnwritedOffOnly && R5(lot_rec.Price) <= 0.0)
							continue;
						if(Filt.Flags & AssetFilt::fWritedOffOnly && R5(lot_rec.Price) > 0.0)
							continue;
						if(!(Filt.Flags & AssetFilt::fShowClosed)) {
							if(Filt.OperPeriod.upp) {
								double rest = 0.0;
								P_BObj->trfr->GetRest(lot_rec.ID, Filt.OperPeriod.upp, &rest);
								if(rest <= 0.0)
									continue;
							}
							else if(lot_rec.Closed)
								continue;
						}
						THROW(MakeItem(lot_rec.ID, &bei, 0));
					}
				}
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewAsset::InitIterQuery(PPID grpID)
{
	int    sp_mode = spFirst;
	union {
		TempAssetTbl::Key1 k1;
		TempAssetTbl::Key2 k2;
	} k;
	delete P_IterQuery;
	P_IterQuery = new BExtQuery(P_TempTbl, IterIdx, 16);
	P_IterQuery->selectAll();
	MEMSZERO(k);
	if(grpID) {
		if(grpID == -1)
			grpID = 0;
		P_IterQuery->where(P_TempTbl->GrpID == grpID);
		k.k1.GrpID = grpID;
		sp_mode = spGe;
	}
	else
		sp_mode = spFirst;
	P_IterQuery->initIteration(0, &k, sp_mode);
	return 1;
}

int SLAPI PPViewAsset::InitIteration(IterOrder ord)
{
	int    ok = 1;
	IterIdx   = 0;
	IterGrpName = 0;
	ZDELETE(P_GGIter);
	ZDELETE(P_IterQuery);
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	Counter.Init(P_TempTbl);
	if(GObj.IsAltGroup(Filt.GoodsGrpID) <= 0 && ord == OrdByGrp_GoodsName) {
		IterIdx = 1;
		THROW_MEM(P_GGIter = new GoodsGroupIterator(Filt.GoodsGrpID, GoodsGroupIterator::fAddZeroGroup));
		NextOuterIteration();
	}
	else {
		IterIdx = 2;
		Goods2Tbl::Rec grp_rec;
		if(GObj.Fetch(Filt.GoodsGrpID, &grp_rec) > 0)
			IterGrpName = grp_rec.Name;
		InitIterQuery(0L);
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewAsset::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName) > 0) {
		SETIFZ(grp_id, -1);
		InitIterQuery(grp_id);
		return 1;
	}
	else
		return -1;
}

int SLAPI PPViewAsset::NextIteration(AssetViewItem * pItem)
{
	do {
		if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			if(pItem) {
				memzero(pItem, sizeof(AssetViewItem));
				pItem->LotID   = P_TempTbl->data.LotID;
				pItem->GoodsID = P_TempTbl->data.GoodsID;
				STRNSCPY(pItem->GoodsName, P_TempTbl->data.Name);
				STRNSCPY(pItem->Serial, P_TempTbl->data.Serial);
				pItem->P_GoodsGrpName = IterGrpName;
				pItem->Dt         = P_TempTbl->data.Dt;
				pItem->ExplDt     = P_TempTbl->data.ExplDt;
				pItem->WrOffTerm  = P_TempTbl->data.WrOffTerm;
				pItem->Cost       = P_TempTbl->data.Cost;
				pItem->Price      = P_TempTbl->data.Price;
				pItem->Deprec     = P_TempTbl->data.Deprec;
				pItem->Cost2      = P_TempTbl->data.Cost2;
				pItem->Price2     = P_TempTbl->data.Price2;
				pItem->Deprec2    = P_TempTbl->data.Deprec2;
				pItem->DiffCost   = P_TempTbl->data.DiffCost;
				pItem->DiffDeprec = P_TempTbl->data.DiffDeprec;
			}
			Counter.Increment();
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

int SLAPI PPViewAsset::GetItem(PPID lotID, AssetViewItem * pItem)
{
	if(P_TempTbl) {
		TempAssetTbl::Key0 k;
		k.LotID = lotID;
		if(P_TempTbl->search(0, &k, spEq)) {
			memzero(pItem, sizeof(AssetViewItem));
			pItem->LotID = P_TempTbl->data.LotID;
			pItem->GoodsID = P_TempTbl->data.GoodsID;
			STRNSCPY(pItem->GoodsName, P_TempTbl->data.Name);
			STRNSCPY(pItem->Serial, P_TempTbl->data.Serial);
			pItem->Dt = P_TempTbl->data.Dt;
			pItem->ExplDt = P_TempTbl->data.ExplDt;
			pItem->WrOffTerm = P_TempTbl->data.WrOffTerm;
			pItem->Cost = P_TempTbl->data.Cost;
			pItem->Price = P_TempTbl->data.Price;
			pItem->Deprec = P_TempTbl->data.Deprec;
			pItem->Cost2 = P_TempTbl->data.Cost2;
			pItem->Price2 = P_TempTbl->data.Price2;
			pItem->Deprec2 = P_TempTbl->data.Deprec2;
			pItem->DiffCost = P_TempTbl->data.DiffCost;
			pItem->DiffDeprec = P_TempTbl->data.DiffDeprec;
			return 1;
		}
	}
	return -1;
}

int SLAPI PPViewAsset::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   lot_id = pHdr ? *(PPID *)pHdr : 0;
	return ::ViewOpersByLot(lot_id, 0);
}

int SLAPI PPViewAsset::ViewTotal()
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(P_TempTbl) {
		AssetTotal total;
		MEMSZERO(total);
		TempAssetTbl::Key0 k0;
		MEMSZERO(k0);
		BExtQuery q(P_TempTbl, 0, 16);
		q.selectAll();
		for(q.initIteration(0, &k0, spFirst); q.nextIteration() > 0;) {
			total.Count++;
			total.Cost    += P_TempTbl->data.Cost;
			total.Price   += P_TempTbl->data.Price;
			total.Deprec  += P_TempTbl->data.Deprec;
			total.Cost2   += P_TempTbl->data.Cost2;
			total.Price2  += P_TempTbl->data.Price2;
			total.Deprec2 += P_TempTbl->data.Deprec2;
		}
		dlg = new TDialog(DLG_ASSETTOTAL);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setCtrlLong(CTL_ASSETTOTAL_COUNT,  total.Count);
			dlg->setCtrlReal(CTL_ASSETTOTAL_COST,   total.Cost2);
			dlg->setCtrlReal(CTL_ASSETTOTAL_PRICE,  total.Price2);
			dlg->setCtrlReal(CTL_ASSETTOTAL_DEPREC, total.Deprec2);
			ExecViewAndDestroy(dlg);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI PPViewAsset::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   id = pHdr ? *(PPID *)pHdr : 0;
		AssetViewItem item;
		switch(ppvCmd) {
			case PPVCMD_EDITGOODS:
				ok = -1;
				if(GetItem(id, &item) && GObj.Edit(&item.GoodsID, 0) == cmOK)
					ok = MakeItem(id, 0, 1) ? 1 : PPErrorZ();
				break;
			case PPVCMD_VIEWCOMPLETE:
				ok = -1;
				P_BObj->ViewLotComplete(id, 0);
				break;
			case PPVCMD_ADDEDINFO:
				ok = -1;
				if(P_BObj->EditLotExtData(id) > 0)
					ok = MakeItem(id, 0, 1) ? 1 : PPErrorZ();
				break;
			case PPVCMD_PRINTLIST:
				ok = -1;
				PrintList();
				break;
		}
	}
	return ok;
}

DBQuery * SLAPI PPViewAsset::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = Filt.OperPeriod.IsZero() ? BROWSER_ASSET : BROWSER_ASSETOPER;
	DBQuery * q = 0;
	TempAssetTbl * tbl = new TempAssetTbl(P_TempTbl->fileName);
	q = & select(
		tbl->LotID,
		tbl->GoodsID,
		tbl->Name,
		tbl->Serial,
		tbl->Dt,
		tbl->ExplDt,
		tbl->WrOffTerm,
		tbl->Cost2,
		tbl->Price2,
		tbl->Deprec2,
		tbl->Cost,
		tbl->Price,
		tbl->Deprec,
		tbl->DiffCost,
		tbl->DiffDeprec, 0L);
	q->from(tbl, 0L).orderBy(tbl->Name, tbl->Dt, 0L);
	THROW(CheckQueryPtr(q));
	if(pSubTitle)
		PPFormatPeriod(&Filt.OperPeriod, *pSubTitle);
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete tbl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewAsset::PrintList()
{
	return Helper_Print(Filt.OperPeriod.IsZero() ? REPORT_ASSETVIEW : REPORT_ASSETOPERVIEW, OrdByGrp_GoodsName);
}

int SLAPI PPViewAsset::Print(const void * pHdr)
{
	int    ok = 1;
	PPID   lot_id = pHdr ? *(PPID *)pHdr : 0;
	AssetCard card;
	MEMSZERO(card);
	if(P_BObj->IsAssetLot(lot_id) > 0) {
		int    r;
		if((r = P_BObj->MakeAssetCard(lot_id, &card)) > 0) {
			PPFilt pf;
			pf.ID = 0;
			pf.Ptr = &card;
			ok = PPAlddPrint(REPORT_ASSETCARD, &pf);
		}
		else if(r == 0)
			ok = PPErrorZ();
		else
			ok = -1;
	}
	else
		ok = -2;
	if(card.P_MovList)
		ZDELETE(card.P_MovList);
	return ok;
}

int SLAPI PPViewAsset::PreprocessBrowser(PPViewBrowser * pBrw)
{
	return (pBrw && pBrw->SetTempGoodsGrp(Filt.GoodsGrpID) > 0) ? 1 : -1;
}
//
// Implementation of PPALDD_AssetView
//
PPALDD_CONSTRUCTOR(AssetView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(AssetView)
{
	Destroy();
}

int PPALDD_AssetView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Asset, rsrv);
	H.FltLocID   = p_filt->LocID;
	H.FltGrpID   = p_filt->GoodsGrpID;
	H.FltBeg     = p_filt->Period.low;
	H.FltEnd     = p_filt->Period.upp;
	H.FltOperBeg = p_filt->OperPeriod.low;
	H.FltOperEnd = p_filt->OperPeriod.upp;
	H.Ft_InExpl  = p_filt->Ft_InExpl;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_AssetView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER_ORD(Asset, (PPViewAsset::IterOrder)sortId);
}

int PPALDD_AssetView::NextIteration(PPIterID iterId, long rsrv)
{
	START_PPVIEW_ALDD_ITER(Asset);
	I.LotID   = item.LotID;
	I.GoodsID = item.GoodsID;
	STRNSCPY(I.GoodsGrpName, item.P_GoodsGrpName);
	I.Dt      = item.Dt;
	I.ExplDt  = item.ExplDt;
	I.Cost    = item.Cost;
	I.Price   = item.Price;
	I.Deprec  = item.Deprec;
	I.Cost2   = item.Cost2;
	I.Price2  = item.Price2;
	I.Deprec2 = item.Deprec2;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_AssetView::Destroy()
{
	DESTROY_PPVIEW_ALDD(Asset);
}

