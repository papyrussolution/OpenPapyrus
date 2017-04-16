// ASSTWROF.CPP
// Copyright (c) A.Sobolev 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2016
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
int SLAPI Transfer::EnumAssetOp(PPID lotID, DateIter * pIter, int * pOpCode, PPID * pDestLotID, TransferTbl::Rec * pRec)
{
	int    op_code = 0, r;
	TransferTbl::Rec rec;
	if((r = EnumByLot(lotID, pIter, &rec)) > 0) {
		if(rec.Flags & PPTFR_RECEIPT) {
			if(rec.Flags & PPTFR_ASSETEXPL)
				op_code = ASSTOPC_RCPTEXPL;
			else
				op_code = ASSTOPC_RCPT;
		}
		else if(rec.Flags & PPTFR_REVAL) {
			if(rec.Flags & PPTFR_ASSETEXPL)
				if(rec.Flags & PPTFR_MODIF)
					op_code = ASSTOPC_EXPLOUT;
				else
					op_code = ASSTOPC_EXPL;
			else
				op_code = ASSTOPC_DEPREC;
		}
		else if(rec.Flags & PPTFR_MINUS)
			if(SearchMirror(rec.Dt, rec.OprNo, &rec) > 0) {
				ASSIGN_PTR(pDestLotID, rec.LotID);
				op_code = ASSTOPC_MOV;
			}
			else
				op_code = ASSTOPC_EXPEND;
		ASSIGN_PTR(pOpCode, op_code);
		ASSIGN_PTR(pRec, rec);
		return 1;
	}
	return r ? -1 : 0;
}

int SLAPI Transfer::EnumAssetOp(PPID * pLotID, DateIter * pIter, int * pOpCode, TransferTbl::Rec * pRec)
{
	int    op_code = 0, r;
	PPID   dest_lot_id = 0;
	TransferTbl::Rec rec;
	if(!pIter->dt && !pIter->oprno) {
		PPID org_lot_id = 0;
		if(Rcpt.SearchOrigin(*pLotID, &org_lot_id, 0, 0) <= 0)
			return 0;
		*pLotID = org_lot_id;
	}
	if((r = EnumAssetOp(*pLotID, pIter, &op_code, &dest_lot_id, &rec)) > 0) {
		if(op_code == ASSTOPC_MOV) {
			*pLotID = dest_lot_id;
			pIter->dt = rec.Dt;
			pIter->oprno = rec.OprNo;
		}
		ASSIGN_PTR(pOpCode, op_code);
		ASSIGN_PTR(pRec, rec);
		return 1;
	}
	return r ? -1 : 0;
}
//
//
//
SLAPI PPObjAssetWrOffGrp::PPObjAssetWrOffGrp(void * extraPtr) : PPObjReference(PPOBJ_ASSTWROFFGRP, extraPtr)
{
}

int SLAPI PPObjAssetWrOffGrp::Edit(PPID * pID, void * extraPtr)
{
	int    ok = 1, r = cmCancel, valid_data = 0, is_new = 0;
	ushort v;
	PPAssetWrOffGrp rec;
	TDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_AWOG))));
	THROW(EditPrereq(pID, dlg, &is_new));
	if(!is_new) {
		THROW(Search(*pID, &rec) > 0);
	}
	else
		MEMSZERO(rec);
	dlg->setCtrlData(CTL_AWOG_NAME, rec.Name);
	dlg->setCtrlData(CTL_AWOG_ID,   &rec.ID);
	dlg->disableCtrl(CTL_AWOG_ID, (!PPMaster || rec.ID));
	dlg->setCtrlData(CTL_AWOG_CODE, rec.Code);
	if(rec.WrOffType == AWOGT_AMRTZ_LIN)
		v = 1;
	else if(rec.WrOffType == AWOGT_AMRTZ_ACCEL)
		v = 2;
	else if(rec.WrOffType == AWOGT_INDIRCOST)
		v = 3;
	else
		v = 0;
	dlg->setCtrlData(CTL_AWOG_SCHEME, &v);
	dlg->setCtrlData(CTL_AWOG_TERM,  &rec.WrOffTerm);
	dlg->setCtrlData(CTL_AWOG_LIMIT, &rec.Limit);
	while(!valid_data && (r = ExecView(dlg)) == cmOK) {
		dlg->getCtrlData(CTL_AWOG_NAME,  rec.Name);
		dlg->getCtrlData(CTL_AWOG_ID,    &rec.ID);
		dlg->getCtrlData(CTL_AWOG_CODE,  rec.Code);
		dlg->getCtrlData(CTL_AWOG_TERM,  &rec.WrOffTerm);
		dlg->getCtrlData(CTL_AWOG_LIMIT, &rec.Limit);
		dlg->getCtrlData(CTL_AWOG_SCHEME, &v);
		if(v == 1)
			rec.WrOffType = AWOGT_AMRTZ_LIN;
		else if(v == 2)
			rec.WrOffType = AWOGT_AMRTZ_ACCEL;
		else if(v == 3)
			rec.WrOffType = AWOGT_INDIRCOST;
		if(!CheckName(*pID, strip(rec.Name), 0))
			dlg->selectCtrl(CTL_AWOG_NAME);
		else {
			valid_data = 1;
			if(*pID)
				*pID = rec.ID;
			THROW(EditItem(Obj, *pID, &rec, 1));
		}
	}
	CATCH
		ok = PPErrorZ();
	ENDCATCH
	delete dlg;
	return ok ? r : 0;
}

int SLAPI PPObjAssetWrOffGrp::Browse(void * extraPtr)
{
	return RefObjView(this, PPDS_CRRASSETWROFFGRP, 0);
}
//
//
//
double SLAPI CalcAssetDeprec(LDATE start, LDATE dt, long term)
{
	int    d, m, y;
	decodedate(&d, &m, &y, &start);
	long   beg = (y * 12 + m);
	decodedate(&d, &m, &y, &dt);
	long   end = (y * 12 + m);
	long   num_periods = ((end - beg) > 0) ? (end - beg) : 0;
	return ((double)num_periods) / (double)term;
}

int SLAPI CalcRestAssetPrice(LDATE start, LDATE dt, long term, double cost, double * pPrice)
{
	double deprec = CalcAssetDeprec(start, dt, term);
	*pPrice = cost - (cost * deprec);
	if(*pPrice < 0)
		*pPrice = 0;
	return 1;
}
//
//
//
class PrcssrAssetDeprec {
public:
	struct Param {
		LDATE  Dt;
		PPID   GoodsTypeID;
		PPID   GoodsGrpID;
		PPID   RevalOpID;
		PPID   LocID;
	};
	SLAPI  PrcssrAssetDeprec()
	{
		P_BObj = BillObj;
		MEMSZERO(P);
	}
	int    SLAPI InitParam(Param * pParam)
	{
		if(pParam) {
			memzero(pParam, sizeof(*pParam));
			PPObjGoods goods_obj;
			pParam->GoodsGrpID = goods_obj.GetConfig().AssetGrpID;
		}
		return 1;
	}
	int    SLAPI Init(const Param * pParam)
	{
		P = *pParam;
		return P.RevalOpID ? 1 : PPSetError(PPERR_OPRKINDNEEDED);
	}
	int    SLAPI EditParam(Param *);
	int    SLAPI Run();
private:
	int    SLAPI ProcessGoods(Goods2Tbl::Rec * pGoodsRec, PPID * pTurnedBillID);
	Param P;
	PPObjBill * P_BObj;
	GoodsIterator GI;
};

int SLAPI PrcssrAssetDeprec::EditParam(Param * pData)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_ASSTREVAL)))) {
		dlg->SetupCalDate(CTLCAL_ASSTREVAL_DT, CTL_ASSTREVAL_DT);
		dlg->setCtrlData(CTL_ASSTREVAL_DT, &pData->Dt);
		SetupPPObjCombo(dlg, CTLSEL_ASSTREVAL_OPRKIND, PPOBJ_OPRKIND,    pData->RevalOpID,   0, (void *)PPOPT_GOODSREVAL);
		SetupPPObjCombo(dlg, CTLSEL_ASSTREVAL_LOC,     PPOBJ_LOCATION,   pData->LocID,       0);
		SetupPPObjCombo(dlg, CTLSEL_ASSTREVAL_GT,      PPOBJ_GOODSTYPE,  pData->GoodsTypeID, 0);
		SetupPPObjCombo(dlg, CTLSEL_ASSTREVAL_GGRP,    PPOBJ_GOODSGROUP, pData->GoodsGrpID,  OLW_CANSELUPLEVEL);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
			dlg->getCtrlData(CTL_ASSTREVAL_DT, &pData->Dt);
			if(!checkdate(pData->Dt, 0))
				PPErrorByDialog(dlg, CTL_ASSTREVAL_DT, PPERR_SLIB);
			else {
				dlg->getCtrlData(CTL_ASSTREVAL_OPRKIND, &pData->RevalOpID);
				dlg->getCtrlData(CTL_ASSTREVAL_LOC,     &pData->LocID);
				dlg->getCtrlData(CTL_ASSTREVAL_GT,      &pData->GoodsTypeID);
				dlg->getCtrlData(CTL_ASSTREVAL_GGRP,    &pData->GoodsGrpID);
				if(pData->RevalOpID == 0)
					PPErrorByDialog(dlg, CTL_ASSTREVAL_OPRKIND, PPERR_OPRKINDNEEDED);
				else
					ok = valid_data = 1;
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PrcssrAssetDeprec::Run()
{
	int    ok = -1;
	ReceiptCore * p_rcpt = & P_BObj->trfr->Rcpt;
	Goods2Tbl::Rec goods_rec;
	ReceiptTbl::Rec lot_rec;
	PPObjGoods    goods_obj;
	GoodsIterator giter;
	GoodsFilt     flt;
	PPBillPacket  pack;
	PPObjAssetWrOffGrp awog_obj;
	PPAssetWrOffGrp    awog_rec;
	SString msg_buf, fmt_buf;
	PPLogger logger;
	SString serial;

	flt.GoodsTypeID = P.GoodsTypeID;
	flt.GrpID = P.GoodsGrpID;

	PPWait(1);
	{
		PPTransaction tra(1);
		THROW(tra);
		THROW(pack.CreateBlank(P.RevalOpID, 0, P.LocID, 0))
		pack.Rec.Dt    = P.Dt;
		pack.Rec.LocID = P.LocID;
		for(giter.Init(&flt, 0); giter.Next(&goods_rec) > 0; PPWaitPercent(giter.GetIterCounter())) {
			if(!goods_obj.IsAsset(goods_rec.ID))
				continue;
			if(goods_rec.WrOffGrpID && awog_obj.Search(goods_rec.WrOffGrpID, &awog_rec) > 0) {
				if(awog_rec.WrOffTerm > 0) {
					for(DateIter diter; p_rcpt->EnumLots(goods_rec.ID, P.LocID, &diter, &lot_rec) > 0;) {
						if(R6(lot_rec.Rest) > 0.0) {
							int    calc_by_price = BIN(goods_rec.Flags & GF_WROFFBYPRICE);
							int    in_expl = 0;
							double new_price = 0.0;
							double deprec = 0.0;
							P_BObj->trfr->GetLotPrices(&lot_rec, P.Dt, 0);
							double price = R5(lot_rec.Price);
							double cost  = R5(lot_rec.Cost);
							LDATE  start = ZERODATE, last_deprec_date = ZERODATE;
							PPID   lot_id = lot_rec.ID;
							int    op_code = 0;
							DateIter iter(0, P.Dt);
							while(P_BObj->trfr->EnumAssetOp(&lot_id, &iter, &op_code, 0) > 0)
								if(oneof2(op_code, ASSTOPC_RCPTEXPL, ASSTOPC_EXPL)) {
									start = iter.dt;
									in_expl = 1;
								}
								else if(op_code == ASSTOPC_EXPLOUT) {
									if(start) {
										deprec += CalcAssetDeprec(start, iter.dt, awog_rec.WrOffTerm);
										start = ZERODATE;
									}
								}
								else if(op_code == ASSTOPC_DEPREC) {
									last_deprec_date = iter.dt;
								}
							if(start)
								if(calc_by_price && last_deprec_date)
									deprec = CalcAssetDeprec(last_deprec_date, P.Dt, awog_rec.WrOffTerm);
								else
									deprec += CalcAssetDeprec(start, P.Dt, awog_rec.WrOffTerm);
							if(calc_by_price && last_deprec_date)
								if(start)
									new_price = R2(price - cost * deprec);
								else
									new_price = price;
							else
								new_price = R2(cost - cost * deprec);
							if(new_price < 0.0)
								new_price = 0.0;
							if(in_expl) {
								if(new_price != price) {
									PPTransferItem ti;
									THROW(ti.Init(&pack.Rec));
									THROW(ti.SetupGoods(lot_rec.GoodsID));
									THROW(ti.SetupLot(lot_rec.ID, 0, 0));
									ti.Suppl    = lot_rec.SupplID;
									ti.Cost     = ti.RevalCost = R5(lot_rec.Cost);
									ti.Price    = new_price;
									ti.Discount = price;
									ti.Rest_    = lot_rec.Rest;
									THROW(pack.InsertRow(&ti, 0));
								}
							}
							else {
								if(P_BObj->GetSerialNumberByLot(lot_rec.ID, serial, 1) <= 0)
									(serial = 0).CatChar('#').Cat(lot_rec.ID);
								PPLoadText(PPTXT_LOG_ASSTWROF_OUTOFEXPL, fmt_buf);
								logger.Log(msg_buf.Printf(fmt_buf, goods_rec.Name, (const char *)serial));
							}
						}
					}
				}
			}
			else
				logger.LogString(PPTXT_LOG_ASSTWROF_NOWROFFGRP, goods_rec.Name);
		}
		if(pack.GetTCount()) {
			pack.InitAmounts();
			THROW(P_BObj->FillTurnList(&pack));
			if(!P_BObj->TurnPacket(&pack, 0)) {
				P_BObj->DiagGoodsTurnError(&pack);
				CALLEXCEPT();
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWait(0);
	return ok;
}

int SLAPI RevalAssets()
{
	int    ok = -1;
	PrcssrAssetDeprec prcssr;
	PrcssrAssetDeprec::Param param;
	prcssr.InitParam(&param);

	param.Dt    = LConfig.OperDate;
	param.LocID = LConfig.Location;
	PPID op_id  = GetSingleOp(PPOPT_GOODSREVAL);
	if(op_id) {
		if(op_id > 0)
			param.RevalOpID = op_id;
		while(prcssr.EditParam(&param) > 0) {
			if(!prcssr.Init(&param) || !prcssr.Run())
				PPError();
			else
				break;
		}
		ok = 1;
	}
	return ok;
}
