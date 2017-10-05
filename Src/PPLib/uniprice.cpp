// UNIPRICE.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016
// @codepage windows-1251
// Унификация цен реализации товара
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(PrcssrUnifyPrice); SLAPI PrcssrUnifyPriceFilt::PrcssrUnifyPriceFilt() : PPBaseFilt(PPFILT_PRCSSRUNIFYPRICEPARAM, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrUnifyPriceFilt, ReserveStart),
		offsetof(PrcssrUnifyPriceFilt, ReserveEnd)-offsetof(PrcssrUnifyPriceFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
	Mode = mLastLot;
}

int SLAPI PrcssrUnifyPriceFilt::Setup(int _costReval, PPID _loc, PPID _suppl)
{
	int    ok = 1;
	Init(1, 0);
	PPID   op_id = GetSingleOp(PPOPT_GOODSREVAL);
	CostReval = _costReval;
	LocID    = NZOR(_loc, LConfig.Location);
	SupplID  = _suppl;
	if(op_id != 0 && (!CostReval || (LocID && SupplID))) {
		if(op_id > 0)
			OpKindID = op_id;
		if(!CostReval) {
			Mode = PrcssrUnifyPriceFilt::mUnify;
		}
		RoundPrec = LConfig.RoundPrec;
		const long f = CheckXORFlags(LConfig.Flags, CFGFLG_ROUNDUP, CFGFLG_ROUNDDOWN);
		if(f == CFGFLG_ROUNDUP)
			RoundDir = 1;
		else if(f == CFGFLG_ROUNDDOWN)
			RoundDir = -1;
		OldPrice = 0.0;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PrcssrUnifyPriceFilt::IsCostBase() const
{
	return BIN(Flags & fCostBase && PctVal != 0.0);
}
//
//
//
static int SLAPI GetNewPrice(PrcssrUnifyPriceFilt * pParam, const Goods2Tbl::Rec * pGoodsRec, double * pNewPrice)
{
	class NewPriceDialog : public TDialog {
	public:
		NewPriceDialog(PrcssrUnifyPriceFilt * aParam, PPID aGoods) : TDialog(DLG_NEWPRICE)
		{
			Param = *aParam;
			GoodsID = aGoods;
			disableCtrls(1, CTL_NEWPRICE_GOODS, CTL_NEWPRICE_LOC, 0);
		}
	private:
		DECL_HANDLE_EVENT
		{
			//PPID   lot_id = 0;
			//ReceiptTbl::Rec rec;
			TDialog::handleEvent(event);
			if(event.isCmd(cmLot)) {
				PPObjBill::SelectLotParam slp(GoodsID, Param.LocID, 0, PPObjBill::SelectLotParam::fFillLotRec);
				if(BillObj->SelectLot2(slp) > 0) {
				//if(SelectLot(Param.LocID, GoodsID, 0, &lot_id, &rec) > 0) {
					if(Param.Mode == PrcssrUnifyPriceFilt::mUnify && !Param.QuotKindID)
						setCtrlReal(CTL_NEWPRICE_PRICE, Param.CalcPrice(slp.RetLotRec.Cost, slp.RetLotRec.Price));
				}
				clearEvent(event);
			}
		}
		PrcssrUnifyPriceFilt Param;
		PPID   GoodsID;
	};
	int    r = cmOK;
	if(pParam->Flags & PrcssrUnifyPriceFilt::fConfirm) {
		ushort v = 0;
		double price = *pNewPrice;
		PPWait(0);
		NewPriceDialog * dlg = new NewPriceDialog(pParam, pGoodsRec->ID);
		if(CheckDialogPtr(&dlg)) {
			SString temp_buf;
			GetLocationName(pParam->LocID, temp_buf);
			dlg->setCtrlString(CTL_NEWPRICE_LOC, temp_buf);
			temp_buf = pGoodsRec->Name;
			dlg->setCtrlString(CTL_NEWPRICE_GOODS, temp_buf);
			dlg->setCtrlData(CTL_NEWPRICE_PRICE, &price);
			dlg->setCtrlData(CTL_NEWPRICE_ALL,   &v);
			if((r = ExecView(dlg)) == cmOK) {
				dlg->getCtrlData(CTL_NEWPRICE_PRICE, &price);
				dlg->getCtrlData(CTL_NEWPRICE_ALL,   &v);
				*pNewPrice = price;
				if(v)
					pParam->Flags &= ~PrcssrUnifyPriceFilt::fConfirm;
			}
			delete dlg;
		}
		else
			r = 0;
		PPWait(1);
	}
	return r;
}

SLAPI PrcssrUnifyPrice::PrcssrUnifyPrice()
{
	P_BObj = BillObj;
}

int SLAPI PrcssrUnifyPrice::EditParam(PrcssrUnifyPriceFilt * pParam)
{
#define GRP_GOODSFILT 1L

	class UnifyPriceDialog : public TDialog {
	public:
		UnifyPriceDialog(uint dlgID) : TDialog(dlgID)
		{
			addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(0, CTLSEL_UNIPRICE_GGRP, cmGoodsFilt));
		}
		int    setDTS(const PrcssrUnifyPriceFilt * pData)
		{
			Data = *pData;
			size_t spctval_len = 0;
			char   spctval[64];
			ushort v;
			GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
			setGroupData(GRP_GOODSFILT, &gf_rec);
			setCtrlOption(CTL_UNIPRICE_FRAME, ofFramed, 1);
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_OPRKIND, PPOBJ_OPRKIND,    Data.OpKindID,   0, (void *)PPOPT_GOODSREVAL);
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_LOC,     PPOBJ_LOCATION,   Data.LocID,      0);
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_QUOTK,   PPOBJ_QUOTKIND,   Data.QuotKindID, 0);
			AddClusterAssoc(CTL_UNIPRICE_EXCLGGRPF, 0, PrcssrUnifyPriceFilt::fExcludeGoodsGrp);
			SetClusterData(CTL_UNIPRICE_EXCLGGRPF, Data.Flags);

			AddClusterAssoc(CTL_UNIPRICE_FLAGS, 0, PrcssrUnifyPriceFilt::fConfirm);
			SetClusterData(CTL_UNIPRICE_FLAGS, Data.Flags);

			AddClusterAssocDef(CTL_UNIPRICE_MODE, 0, PrcssrUnifyPriceFilt::mLastLot);
			AddClusterAssoc(CTL_UNIPRICE_MODE, 1, PrcssrUnifyPriceFilt::mUnify);
			AddClusterAssoc(CTL_UNIPRICE_MODE, 2, PrcssrUnifyPriceFilt::mEachLot);
			SetClusterData(CTL_UNIPRICE_MODE, Data.Mode);

			memzero(spctval, sizeof(spctval));
			if(Data.PctVal != 0) {
				realfmt(Data.PctVal, SFMT_MONEY, spctval);
				spctval_len = strlen(spctval);
				if(spctval_len < sizeof(spctval)) {
					if(Data.IsCostBase())
						spctval[spctval_len] = 'C';
					else if(Data.Flags & PrcssrUnifyPriceFilt::fAbsVal)
						spctval[spctval_len] = '$';
					else
						spctval[spctval_len] = '%';
				}
				setCtrlData(CTL_UNIPRICE_PCT,   spctval);
			}
			if(Data.RoundDir > 0)
				v = 1;
			else if(Data.RoundDir < 0)
				v = 2;
			else
				v = 0;
			setCtrlData(CTL_UNIPRICE_ROUNDDIR, &v);
			setCtrlData(CTL_UNIPRICE_PREC,     &Data.RoundPrec);
			setCtrlData(CTL_UNIPRICE_OLDPRICE, &Data.OldPrice);
			disableCtrl(CTL_UNIPRICE_PCT, BIN(Data.QuotKindID));
			return 1;
		}
		int    getDTS(PrcssrUnifyPriceFilt * pData)
		{
			int    ok = 1;
			uint   sel = 0;
			int    cost_base = 0, abs_val = 0;
			size_t spctval_len = 0;
			char   spctval[64];
			ushort v;
			GoodsFiltCtrlGroup::Rec gf_rec;

			getCtrlData(sel = CTLSEL_UNIPRICE_OPRKIND, &Data.OpKindID);
			THROW_PP(Data.OpKindID, PPERR_OPRKINDNEEDED);
			getCtrlData(sel = CTLSEL_UNIPRICE_LOC,     &Data.LocID);
			THROW_PP(Data.LocID, PPERR_LOCNEEDED);
			getCtrlData(CTLSEL_UNIPRICE_QUOTK,   &Data.QuotKindID);
			THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
			Data.GoodsGrpID = gf_rec.GoodsGrpID;
			GetClusterData(CTL_UNIPRICE_EXCLGGRPF, &Data.Flags);
			GetClusterData(CTL_UNIPRICE_FLAGS,     &Data.Flags);
			GetClusterData(CTL_UNIPRICE_MODE,      &Data.Mode);
			getCtrlData(sel = CTL_UNIPRICE_PCT, spctval);
			spctval_len = strlen(spctval);
			if(spctval_len) {
				char symb = spctval[spctval_len-1];
				if(toupper(symb) == 'C' /*lat*/ || toupper(symb) == 145 /*rus*/) {
					cost_base = 1;
					spctval[spctval_len-1] = '\0';
				}
				else if(symb == '%' || symb == '/')
					spctval[spctval_len-1] = '\0';
				else if(symb == '$') {
					abs_val = 1;
					spctval[spctval_len-1] = '\0';
				}
				else
					abs_val = 1;
				strtodoub(spctval, &Data.PctVal);
			}
			else
				Data.PctVal = 0;
			THROW_PP(!Data.CostReval || Data.PctVal != 0, PPERR_COSTREVALPCTNEEDED);
			SETFLAG(Data.Flags, PrcssrUnifyPriceFilt::fCostBase, Data.PctVal != 0 && cost_base);
			SETFLAG(Data.Flags, PrcssrUnifyPriceFilt::fAbsVal,   Data.PctVal != 0 && abs_val);
			getCtrlData(CTL_UNIPRICE_PREC,     &Data.RoundPrec);
			getCtrlData(CTL_UNIPRICE_OLDPRICE, &Data.OldPrice);
			getCtrlData(CTL_UNIPRICE_ROUNDDIR, &(v = 0));
			if(v == 1)
				Data.RoundDir = 1;
			else if(v == 2)
				Data.RoundDir = -1;
			else
				Data.RoundDir = 0;
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_UNIPRICE_QUOTK)) {
				PPID   qk_id = getCtrlLong(CTLSEL_UNIPRICE_QUOTK);
				disableCtrl(CTL_UNIPRICE_PCT, BIN(qk_id));
				clearEvent(event);
			}
		}
		PrcssrUnifyPriceFilt Data;
	};
	DIALOG_PROC_BODY_P1(UnifyPriceDialog, pParam->CostReval ? DLG_UNICOST : DLG_UNIPRICE, pParam);
}

int SLAPI PrcssrUnifyPrice::InitBillPack()
{
	return BPack.CreateBlank(P.OpKindID, 0, P.LocID, 1) ? ((BPack.Rec.LocID = P.LocID), 1) : 0;
}

int SLAPI PrcssrUnifyPrice::TurnBillPack()
{
	int    ok = 1;
	if(BPack.GetTCount()) {
		BPack.InitAmounts();
		THROW(P_BObj->FillTurnList(&BPack));
		if(!P_BObj->TurnPacket(&BPack, 1)) {
			P_BObj->DiagGoodsTurnError(&BPack);
			CALLEXCEPT();
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

#if 0 // {

int SLAPI PrcssrUnifyPrice::ProcessGoods(const Goods2Tbl::Rec * pGoodsRec, PPID * pTurnedBillID)
{
	int    ok = 1, r;
	uint   i;
	LotArray lot_list, unify_lot_list;
	PPIDArray pos_to_del;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	ReceiptTbl::Rec * p_lot_rec;
	PPTransferItem    ti;
	int    count = 0, diff = 0;
	double last_price = 0.0, price = 0.0, cost = 0.0;
	for(DateIter diter; (r = rcpt.EnumLots(pGoodsRec->ID, P.LocID, &diter)) > 0;)
		if(R6(rcpt.data.Rest) > 0.0) {
			price = R5(rcpt.data.Price);
			cost  = R5(rcpt.data.Cost);
			if(count && last_price != price)
				diff = 1;
			last_price = price;
			count++;
			if(!P.OldPrice || price == P.OldPrice)
				THROW_SL(lot_list.insert(&rcpt.data));
			if(P.OldPrice)
				THROW_SL(unify_lot_list.insert(&rcpt.data));
		}
	THROW(r);
	if(lot_list.getCount()) {
		int    skip = 0;
		int    last_lot_only = 0;
		double new_price = 0.0;
		if(P.QuotKindID) {
			double quot = 0.0;
			QuotIdent qi(P.LocID, P.QuotKindID, 0 /* CurID */, 0 /* ArID */);
			if(GObj.GetQuotExt(pGoodsRec->ID, qi, cost, price, &quot, 1) > 0) {
				new_price = (P.RoundPrec != 0.0) ? Round(quot, P.RoundPrec, P.RoundDir) : quot;
				if(!(P.Flags & PrcssrUnifyPriceFilt::fUnify))
					last_lot_only = 1;
			}
			else
				skip = 1;
		}
		else if(P.PctVal || P.IsCostBase()) {
			new_price = P.CalcPrice(cost, price);
			if(!(P.Flags & PrcssrUnifyPriceFilt::fUnify))
				last_lot_only = 1;
		}
		else if(diff && P.Flags & PrcssrUnifyPriceFilt::fUnify) {
			new_price = P.CalcPrice(cost, price);
			if(P.OldPrice)
				lot_list.copy(unify_lot_list);
		}
		else
			skip = 1;
		if(!skip && (r = GetNewPrice(&P, pGoodsRec, &new_price)) == cmOK) {
			for(i = 0; lot_list.enumItems(&i, (void**)&p_lot_rec);) {
				double lot_price = R5(p_lot_rec->Price);
				if((!last_lot_only || i == lot_list.getCount()) && dbl_cmp(lot_price, new_price) != 0) {
					MEMSZERO(ti);
					THROW(ti.Init(&BPack.Rec));
					THROW(ti.SetupGoods(pGoodsRec->ID));
					THROW(ti.SetupLot(p_lot_rec->ID, 0, 0));
					ti.Suppl    = p_lot_rec->SupplID;
					ti.Cost     = ti.RevalCost = R5(p_lot_rec->Cost);
					ti.Price    = new_price;
					ti.Discount = lot_price;
					ti.Rest_    = p_lot_rec->Rest;
					THROW(BPack.InsertRow(&ti, 0));
				}
			}
		}
		THROW(r);
		if(BPack.CheckLargeBill(0)) {
			THROW(TurnBillPack());
			ASSIGN_PTR(pTurnedBillID, BPack.Rec.ID);
			THROW(InitBillPack());
		}
	}
	CATCHZOK
	return ok;
}

#endif // } 0

double SLAPI PrcssrUnifyPriceFilt::CalcPrice(double cost, double price) const
{
	double base_price = price;
	if(!QuotKindID) {
		if(IsCostBase())
			base_price = cost;
		if(base_price != 0.0 && PctVal != 0.0) {
			double p = (Flags & fAbsVal) ? (base_price + PctVal) : (base_price * (1.0 + fdiv100r(PctVal)));
			base_price = Round(p, RoundPrec, RoundDir);
		}
	}
	return base_price;
}

int SLAPI PrcssrUnifyPrice::CalcNewPrice(const ReceiptTbl::Rec & rLotRec, double * pPrice)
{
	int    ok = 1;
	double new_price = 0.0;
	const double price = R5(rLotRec.Price);
	const double cost  = R5(rLotRec.Cost);
	if(P.QuotKindID) {
		double quot = 0.0;
		QuotIdent qi(P.LocID, P.QuotKindID, 0 /* CurID */, 0 /* ArID */);
		if(GObj.GetQuotExt(rLotRec.GoodsID, qi, cost, price, &quot, 1) > 0) {
			new_price = (P.RoundPrec != 0.0) ? Round(quot, P.RoundPrec, P.RoundDir) : quot;
		}
		else
			ok = 0;
	}
	else {
		new_price = P.IsCostBase() ? cost : price;
		if(new_price != 0.0 && P.PctVal != 0.0) {
			double p = (P.Flags & PrcssrUnifyPriceFilt::fAbsVal) ? (new_price + P.PctVal) : (new_price * (1.0 + fdiv100r(P.PctVal)));
			new_price = Round(p, P.RoundPrec, P.RoundDir);
		}
	}
	ASSIGN_PTR(pPrice, new_price);
	return ok ? (dbl_cmp(price, new_price) ? 1 : -1) : 0;
}

int SLAPI PrcssrUnifyPrice::ProcessGoods2(const Goods2Tbl::Rec * pGoodsRec, PPID * pTurnedBillID)
{
	int    ok = 1, r;
	LotArray lot_list, unify_lot_list;
	PPIDArray pos_to_del;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	PPTransferItem    ti;
	int    count = 0, diff = 0;
	double last_price = 0.0, price = 0.0, cost = 0.0;
	for(DateIter diter; (r = rcpt.EnumLots(pGoodsRec->ID, P.LocID, &diter)) > 0;) {
		if(R6(rcpt.data.Rest) > 0.0) {
			price = R5(rcpt.data.Price);
			cost  = R5(rcpt.data.Cost);
			if(count && last_price != price)
				diff = 1;
			last_price = price;
			count++;
			if(!P.OldPrice || price == P.OldPrice)
				THROW_SL(lot_list.insert(&rcpt.data));
			if(P.OldPrice)
				THROW_SL(unify_lot_list.insert(&rcpt.data));
		}
	}
	THROW(r);
	{
		const uint lc = lot_list.getCount();
		if(lc) {
			int    ir = 0;
			int    last_lot_only = 0;
			double new_last_price = 0.0;
			double new_price = 0.0;
			ReceiptTbl::Rec & r_last_lot_rec = lot_list.at(lc-1);
			const int lr = CalcNewPrice(r_last_lot_rec, &new_last_price);
			if(lr) {
                if(P.Mode == PrcssrUnifyPriceFilt::mUnify) {
					for(uint i = 0; i < lc; i++) {
						const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
						const double lot_price = R5(r_lot_rec.Price);
						new_price = new_last_price;
						if(dbl_cmp(lot_price, new_price) != 0) {
							THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
							if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
								MEMSZERO(ti);
								THROW(ti.Init(&BPack.Rec));
								THROW(ti.SetupGoods(pGoodsRec->ID));
								THROW(ti.SetupLot(r_lot_rec.ID, 0, 0));
								ti.Suppl    = r_lot_rec.SupplID;
								ti.Cost     = ti.RevalCost = R5(r_lot_rec.Cost);
								ti.Price    = new_price;
								ti.Discount = lot_price;
								ti.Rest_    = r_lot_rec.Rest;
								THROW(BPack.InsertRow(&ti, 0));
							}
						}
					}
                }
                else if(P.Mode == PrcssrUnifyPriceFilt::mEachLot) {
					for(uint i = 0; i < lc; i++) {
						const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
						const double lot_price = R5(r_lot_rec.Price);
						if(CalcNewPrice(r_lot_rec, &new_price) > 0) {
							THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
							if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
								MEMSZERO(ti);
								THROW(ti.Init(&BPack.Rec));
								THROW(ti.SetupGoods(pGoodsRec->ID));
								THROW(ti.SetupLot(r_lot_rec.ID, 0, 0));
								ti.Suppl    = r_lot_rec.SupplID;
								ti.Cost     = ti.RevalCost = R5(r_lot_rec.Cost);
								ti.Price    = new_price;
								ti.Discount = lot_price;
								ti.Rest_    = r_lot_rec.Rest;
								THROW(BPack.InsertRow(&ti, 0));
							}
						}
					}
                }
                else { // P.Mode == PrcssrUnifyPriceFilt::mLastLot
					if(lr > 0) {
						const ReceiptTbl::Rec & r_lot_rec = r_last_lot_rec;
						const double lot_price = R5(r_lot_rec.Price);
						new_price = new_last_price;
						THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
						if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
							MEMSZERO(ti);
							THROW(ti.Init(&BPack.Rec));
							THROW(ti.SetupGoods(pGoodsRec->ID));
							THROW(ti.SetupLot(r_lot_rec.ID, 0, 0));
							ti.Suppl    = r_lot_rec.SupplID;
							ti.Cost     = ti.RevalCost = R5(r_lot_rec.Cost);
							ti.Price    = new_price;
							ti.Discount = lot_price;
							ti.Rest_    = r_lot_rec.Rest;
							THROW(BPack.InsertRow(&ti, 0));
						}
					}
                }
			}
			if(BPack.CheckLargeBill(0)) {
				THROW(TurnBillPack());
				ASSIGN_PTR(pTurnedBillID, BPack.Rec.ID);
				THROW(InitBillPack());
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrUnifyPrice::Process(const PrcssrUnifyPriceFilt * pParam)
{
	int    ok = 1;
	uint   i;
	PPID   turned_bill_id;
	PPID   goodsgrp_id = 0, excl_goodsgrp_id = 0;
	PPIDArray    turned_bill_list;
	PPObjGoods   gobj;
	Goods2Tbl::Rec grec;
	GoodsIterator  iter;
	P = *pParam;
	THROW(InitBillPack());
	PPWait(1);
	if(P.GoodsGrpID && !(P.Flags & PrcssrUnifyPriceFilt::fExcludeGoodsGrp))
		goodsgrp_id = P.GoodsGrpID;
	else
		excl_goodsgrp_id = P.GoodsGrpID;
	//
	// @todo В фильтр по товарам добавить опцию "!GF_UNLIM"
	//
	for(iter.Init(goodsgrp_id, GoodsIterator::ordByName); iter.Next(&grec) > 0;) {
		Goods2Tbl::Rec temp_grec;
		THROW(PPCheckUserBreak());
		if(gobj.Fetch(grec.ID, &temp_grec) > 0 && !(temp_grec.Flags & GF_UNLIM) && !gobj.IsAsset(grec.ID))
			if(!excl_goodsgrp_id || gobj.BelongToGroup(grec.ID, excl_goodsgrp_id) <= 0) {
				THROW(ProcessGoods2(&grec, &(turned_bill_id = 0)));
				if(turned_bill_id)
					turned_bill_list.add(turned_bill_id);
			}
		PPWaitPercent(iter.GetIterCounter());
	}
	THROW(TurnBillPack());
	PPWait(0);
	CATCH
		ok = 0;
		PPSaveErrContext();
		for(i = 0; i < turned_bill_list.getCount(); i++)
			P_BObj->RemovePacket(turned_bill_list.at(i), 1);
		PPRestoreErrContext();
	ENDCATCH
	return ok;
}
//
//
//
int SLAPI UnifyGoodsPrice()
{
	int    ok = -1;
	PrcssrUnifyPriceFilt param;
	PrcssrUnifyPrice upb;
	if(param.Setup(0, LConfig.Location, 0) > 0 && upb.EditParam(&param) > 0) {
		PPObjGoodsGroup::SetDynamicOwner(param.GoodsGrpID, 0, (long)0);
		ok = upb.Process(&param) ? 1 : PPErrorZ();
		PPObjGoodsGroup::RemoveDynamicAlt(param.GoodsGrpID, (long)0);
	}
	return ok;
}

