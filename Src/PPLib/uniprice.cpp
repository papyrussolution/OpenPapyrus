// UNIPRICE.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
// Унификация цен реализации товара
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(PrcssrUnifyPrice); PrcssrUnifyPriceFilt::PrcssrUnifyPriceFilt() : PPBaseFilt(PPFILT_PRCSSRUNIFYPRICEPARAM, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrUnifyPriceFilt, ReserveStart),
		offsetof(PrcssrUnifyPriceFilt, ReserveEnd)-offsetof(PrcssrUnifyPriceFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
	Mode = mLastLot;
}

int PrcssrUnifyPriceFilt::Setup(int _costReval, PPID _loc, PPID _suppl)
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

bool PrcssrUnifyPriceFilt::IsCostBase() const { return (Flags & fCostBase && PctVal != 0.0); }
//
//
//
static int GetNewPrice(PrcssrUnifyPriceFilt * pParam, const Goods2Tbl::Rec * pGoodsRec, double * pNewPrice)
{
	class NewPriceDialog : public TDialog {
	public:
		NewPriceDialog(PrcssrUnifyPriceFilt * aParam, PPID aGoods) : TDialog(DLG_NEWPRICE), Param(*aParam), GoodsID(aGoods)
		{
			disableCtrls(1, CTL_NEWPRICE_GOODS, CTL_NEWPRICE_LOC, 0);
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmLot)) {
				PPObjBill::SelectLotParam slp(GoodsID, Param.LocID, 0, PPObjBill::SelectLotParam::fFillLotRec);
				if(BillObj->SelectLot2(slp) > 0) {
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
		PPWaitStop();
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
		PPWaitStart();
	}
	return r;
}

PrcssrUnifyPrice::PrcssrUnifyPrice() : P_BObj(BillObj)
{
}

int PrcssrUnifyPrice::EditParam(PrcssrUnifyPriceFilt * pParam)
{
	class UnifyPriceDialog : public TDialog {
		DECL_DIALOG_DATA(PrcssrUnifyPriceFilt);
		enum {
			ctlgroupGoodsFilt = 1
		};
	public:
		explicit UnifyPriceDialog(uint dlgID) : TDialog(dlgID)
		{
			addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(0, CTLSEL_UNIPRICE_GGRP, cmGoodsFilt));
		}
		DECL_DIALOG_SETDTS()
		{
			// @v11.1.12 {
			bool allow_psrcGtPriceRestrLow = false;
			bool allow_psrcGtPriceRestrUpp = false;
			{
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				PPIDArray restr_id_list;
				for(SEnum en = gt_obj.Enum(0); en.Next(&gt_rec) > 0;) {
					restr_id_list.addnz(gt_rec.PriceRestrID);
				}
				if(restr_id_list.getCount()) {
					restr_id_list.sortAndUndup();
					PPObjGoodsValRestr gvr_obj;
					PPGoodsValRestrPacket gvr_pack;
					for(uint i = 0; i < restr_id_list.getCount(); i++) {
						if(gvr_obj.GetPacket(restr_id_list.get(i), &gvr_pack) > 0) {
							if(gvr_pack.LowBoundFormula.NotEmpty())
								allow_psrcGtPriceRestrLow = true;
							if(gvr_pack.UppBoundFormula.NotEmpty())
								allow_psrcGtPriceRestrUpp = true;
						}
					}
				}
			}
			// } @v11.1.12 
			RVALUEPTR(Data, pData);
			size_t spctval_len = 0;
			char   spctval[64];
			ushort v;
			GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
			setGroupData(ctlgroupGoodsFilt, &gf_rec);
			// @v11.3.2 @obsolete setCtrlOption(CTL_UNIPRICE_FRAME, ofFramed, 1);
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_OPRKIND, PPOBJ_OPRKIND,    Data.OpKindID,   0, reinterpret_cast<void *>(PPOPT_GOODSREVAL));
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_LOC,     PPOBJ_LOCATION,   Data.LocID,      0);
			SetupPPObjCombo(this, CTLSEL_UNIPRICE_QUOTK,   PPOBJ_QUOTKIND,   Data.QuotKindID, 0);
			AddClusterAssoc(CTL_UNIPRICE_EXCLGGRPF, 0, PrcssrUnifyPriceFilt::fExcludeGoodsGrp);
			SetClusterData(CTL_UNIPRICE_EXCLGGRPF, Data.Flags);
			// @v11.1.12 {
			AddClusterAssocDef(CTL_UNIPRICE_PSRC, 0, PrcssrUnifyPriceFilt::psrcImplicit);
			AddClusterAssoc(CTL_UNIPRICE_PSRC, 1, PrcssrUnifyPriceFilt::psrcQuot);
			AddClusterAssoc(CTL_UNIPRICE_PSRC, 2, PrcssrUnifyPriceFilt::psrcGtPriceRestrLow);
			AddClusterAssoc(CTL_UNIPRICE_PSRC, 3, PrcssrUnifyPriceFilt::psrcGtPriceRestrUpp);
			SetClusterData(CTL_UNIPRICE_PSRC, Data.PriceSource);
			DisableClusterItem(CTL_UNIPRICE_PSRC, 2, !allow_psrcGtPriceRestrLow);
			DisableClusterItem(CTL_UNIPRICE_PSRC, 3, !allow_psrcGtPriceRestrUpp);
			// } @v11.1.12 
			AddClusterAssoc(CTL_UNIPRICE_FLAGS, 0, PrcssrUnifyPriceFilt::fConfirm);
			SetClusterData(CTL_UNIPRICE_FLAGS, Data.Flags);

			AddClusterAssocDef(CTL_UNIPRICE_MODE, 0, PrcssrUnifyPriceFilt::mLastLot);
			AddClusterAssoc(CTL_UNIPRICE_MODE, 1, PrcssrUnifyPriceFilt::mUnify);
			AddClusterAssoc(CTL_UNIPRICE_MODE, 2, PrcssrUnifyPriceFilt::mEachLot);
			SetClusterData(CTL_UNIPRICE_MODE, Data.Mode);

			memzero(spctval, sizeof(spctval));
			if(Data.PctVal != 0) {
				realfmt(Data.PctVal, SFMT_MONEY, spctval);
				spctval_len = sstrlen(spctval);
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
			SetupPriceSource(); // @v11.1.12
			return 1;
		}
		DECL_DIALOG_GETDTS()
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
			THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
			Data.GoodsGrpID = gf_rec.GoodsGrpID;
			GetClusterData(CTL_UNIPRICE_PSRC, &Data.PriceSource); // @v11.1.12
			GetClusterData(CTL_UNIPRICE_EXCLGGRPF, &Data.Flags);
			GetClusterData(CTL_UNIPRICE_FLAGS,     &Data.Flags);
			GetClusterData(CTL_UNIPRICE_MODE,      &Data.Mode);
			getCtrlData(sel = CTL_UNIPRICE_PCT, spctval);
			spctval_len = sstrlen(spctval);
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
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_UNIPRICE_QUOTK)) {
				PPID   qk_id = getCtrlLong(CTLSEL_UNIPRICE_QUOTK);
				disableCtrl(CTL_UNIPRICE_PCT, BIN(qk_id));
			}
			else if(event.isClusterClk(CTL_UNIPRICE_PSRC)) {
				SetupPriceSource();
			}
			else
				return;
			clearEvent(event);
		}
		void SetupPriceSource() // @v11.1.12
		{
			GetClusterData(CTL_UNIPRICE_PSRC, &Data.PriceSource); 
			disableCtrl(CTLSEL_UNIPRICE_QUOTK, oneof2(Data.PriceSource, Data.psrcGtPriceRestrLow, Data.psrcGtPriceRestrUpp));
		}
	};
	DIALOG_PROC_BODY_P1(UnifyPriceDialog, pParam->CostReval ? DLG_UNICOST : DLG_UNIPRICE, pParam);
}

int PrcssrUnifyPrice::InitBillPack()
{
	return BPack.CreateBlank(P.OpKindID, 0, P.LocID, 1) ? ((BPack.Rec.LocID = P.LocID), 1) : 0;
}

int PrcssrUnifyPrice::TurnBillPack()
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

int PrcssrUnifyPrice::ProcessGoods(const Goods2Tbl::Rec * pGoodsRec, PPID * pTurnedBillID)
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
			for(i = 0; lot_list.enumItems(&i, (void **)&p_lot_rec);) {
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

double PrcssrUnifyPriceFilt::CalcPrice(double cost, double price) const
{
	double base_price = price;
	if(!QuotKindID) {
		if(IsCostBase())
			base_price = cost;
		if(base_price != 0.0 && PctVal != 0.0) {
			double p = (Flags & fAbsVal) ? (base_price + PctVal) : (base_price * (1.0 + fdiv100r(PctVal)));
			base_price = PPRound(p, RoundPrec, RoundDir);
		}
	}
	return base_price;
}

int PrcssrUnifyPrice::CalcNewPrice(const ReceiptTbl::Rec & rLotRec, double * pPrice)
{
	int    ok = 1;
	double new_price = 0.0;
	const double price = R5(rLotRec.Price);
	const double cost  = R5(rLotRec.Cost);
	if(P.QuotKindID) {
		double quot = 0.0;
		QuotIdent qi(P.LocID, P.QuotKindID, 0 /* CurID */, 0 /* ArID */);
		if(GObj.GetQuotExt(rLotRec.GoodsID, qi, cost, price, &quot, 1) > 0) {
			new_price = (P.RoundPrec != 0.0) ? PPRound(quot, P.RoundPrec, P.RoundDir) : quot;
		}
		else
			ok = 0;
	}
	else {
		new_price = P.IsCostBase() ? cost : price;
		if(new_price != 0.0 && P.PctVal != 0.0) {
			double p = (P.Flags & PrcssrUnifyPriceFilt::fAbsVal) ? (new_price + P.PctVal) : (new_price * (1.0 + fdiv100r(P.PctVal)));
			new_price = PPRound(p, P.RoundPrec, P.RoundDir);
		}
	}
	ASSIGN_PTR(pPrice, new_price);
	return ok ? (dbl_cmp(price, new_price) ? 1 : -1) : 0;
}

int PrcssrUnifyPrice::Helper_GetPriceRestrictions_ByFormula(SString & rFormula, const PPGoodsPacket * pPack, const ReceiptTbl::Rec & rLotRec, double & rBound)
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
		ctx.P_LotRec = &rLotRec;
		if(PPCalcExpression(rFormula, &bound, &ctx) && bound > 0.0) {
			rBound = bound;
			ok = 1;
		}
	}
	return ok;
}

int PrcssrUnifyPrice::ProcessGoods2(const Goods2Tbl::Rec * pGoodsRec, PPID * pTurnedBillID)
{
	int    ok = 1;
	int    r;
	LotArray lot_list;
	LotArray unify_lot_list;
	PPIDArray pos_to_del;
	ReceiptCore & rcpt = P_BObj->trfr->Rcpt;
	int    count = 0;
	int    diff = 0;
	double last_price = 0.0;
	// @v11.1.12 {
	bool   skip_this_goods = false;
	bool   use_price_restr_limit = false;
	SString price_restr_formula;
	PPGoodsPacket goods_pack;
	//double price_restr_limit = 0.0;
	if(oneof2(P.PriceSource, P.psrcGtPriceRestrLow, P.psrcGtPriceRestrUpp)) {
		skip_this_goods = true;
		PPGoodsType gt_rec;
		if(pGoodsRec->GoodsTypeID && GObj.FetchGoodsType(pGoodsRec->GoodsTypeID, &gt_rec) > 0) {
			if(gt_rec.PriceRestrID) {
				PPObjGoodsValRestr gvr_obj;
				PPGoodsValRestrPacket gvr_pack;
				if(gvr_obj.GetPacket(gt_rec.PriceRestrID, &gvr_pack) > 0) {
					if(GObj.GetPacket(pGoodsRec->ID, &goods_pack, 0) > 0) {
						if(P.PriceSource == P.psrcGtPriceRestrLow && gvr_pack.LowBoundFormula.NotEmpty()) {
							skip_this_goods = false;
							use_price_restr_limit = true;
							price_restr_formula = gvr_pack.LowBoundFormula;
							/*
							if(Helper_GetPriceRestrictions_ByFormula(gvr_pack.LowBoundFormula, &goods_pack, price_restr_limit) > 0) {
								assert(price_restr_limit > 0.0); // __Helper_GetPriceRestrictions_ByFormula guarantees!
								if(price_restr_limit > 0.0)
									skip_this_goods = false;
							}
							*/
						}
						else if(P.PriceSource == P.psrcGtPriceRestrUpp && gvr_pack.UppBoundFormula.NotEmpty()) {
							skip_this_goods = false;
							use_price_restr_limit = true;
							price_restr_formula = gvr_pack.UppBoundFormula;
							/*
							if(Helper_GetPriceRestrictions_ByFormula(gvr_pack.UppBoundFormula, &goods_pack, price_restr_limit) > 0) {
								assert(price_restr_limit > 0.0); // __Helper_GetPriceRestrictions_ByFormula guarantees!
								if(price_restr_limit > 0.0)
									skip_this_goods = false;
							}
							*/
						}
					}
				}
			}
		}
	}
	// } @v11.1.12 
	if(!skip_this_goods) {
		for(DateIter diter; (r = rcpt.EnumLots(pGoodsRec->ID, P.LocID, &diter)) > 0;) {
			if(R6(rcpt.data.Rest) > 0.0) {
				double price = R5(rcpt.data.Price);
				double cost  = R5(rcpt.data.Cost);
				if(count && last_price != price)
					diff = 1;
				last_price = price;
				count++;
				if(P.OldPrice == 0.0 || feqeps(price, P.OldPrice, 1e-6))
					THROW_SL(lot_list.insert(&rcpt.data));
				if(P.OldPrice != 0.0)
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
				int lr = 0;
				if(use_price_restr_limit) {
					lr = 1;
				}
				else {
					lr = CalcNewPrice(r_last_lot_rec, &new_last_price);
				}
				if(lr) {
					if(P.Mode == PrcssrUnifyPriceFilt::mUnify) {
						for(uint i = 0; i < lc; i++) {
							const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
							const double lot_price = R5(r_lot_rec.Price);
							new_price = new_last_price;
							double price_restr_limit = 0.0;
							if(use_price_restr_limit) {
								assert(price_restr_formula.NotEmpty());
								assert(goods_pack.Rec.ID == pGoodsRec->ID);
								if(price_restr_formula.NotEmpty() && goods_pack.Rec.ID == pGoodsRec->ID &&
									Helper_GetPriceRestrictions_ByFormula(price_restr_formula, &goods_pack, r_lot_rec, price_restr_limit) > 0) {
									assert(price_restr_limit > 0.0); // Helper_GetPriceRestrictions_ByFormula guarantees!
									new_price = price_restr_limit;
								}
								else
									new_price = 0.0;
							}
							if(new_price > 0.0 && dbl_cmp(lot_price, new_price) != 0) {
								THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
								if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
									PPTransferItem ti(&BPack.Rec, TISIGN_UNDEF);
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
							int   cnpr = 0;
							if(use_price_restr_limit) {
								double price_restr_limit = 0.0;
								assert(price_restr_formula.NotEmpty());
								assert(goods_pack.Rec.ID == pGoodsRec->ID);
								if(price_restr_formula.NotEmpty() && goods_pack.Rec.ID == pGoodsRec->ID &&
									Helper_GetPriceRestrictions_ByFormula(price_restr_formula, &goods_pack, r_lot_rec, price_restr_limit) > 0) {
									assert(price_restr_limit > 0.0); // Helper_GetPriceRestrictions_ByFormula guarantees!
									new_price = price_restr_limit;
									cnpr = 1;
								}
								else
									new_price = 0.0;
							}
							else {
								cnpr = CalcNewPrice(r_lot_rec, &new_price);
							}
							if(cnpr > 0) {
								THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
								if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
									PPTransferItem ti(&BPack.Rec, TISIGN_UNDEF);
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
							if(use_price_restr_limit) {
								double price_restr_limit = 0.0;
								assert(price_restr_formula.NotEmpty());
								assert(goods_pack.Rec.ID == pGoodsRec->ID);
								if(price_restr_formula.NotEmpty() && goods_pack.Rec.ID == pGoodsRec->ID &&
									Helper_GetPriceRestrictions_ByFormula(price_restr_formula, &goods_pack, r_lot_rec, price_restr_limit) > 0) {
									assert(price_restr_limit > 0.0); // Helper_GetPriceRestrictions_ByFormula guarantees!
									new_price = price_restr_limit;
									//cnpr = 1;
								}
								else
									new_price = 0.0;
							}
							else
								new_price = new_last_price;
							if(new_price > 0.0) {
								THROW(ir = GetNewPrice(&P, pGoodsRec, &new_price));
								if(ir == cmOK && dbl_cmp(lot_price, new_price) != 0) {
									PPTransferItem ti(&BPack.Rec, TISIGN_UNDEF);
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
				}
				if(BPack.CheckLargeBill(0)) {
					THROW(TurnBillPack());
					ASSIGN_PTR(pTurnedBillID, BPack.Rec.ID);
					THROW(InitBillPack());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrUnifyPrice::Process(const PrcssrUnifyPriceFilt * pParam)
{
	int    ok = 1;
	uint   i;
	PPID   turned_bill_id;
	PPID   goodsgrp_id = 0;
	PPID   excl_goodsgrp_id = 0;
	PPIDArray    turned_bill_list;
	PPObjGoods   gobj;
	Goods2Tbl::Rec grec;
	GoodsIterator  iter;
	P = *pParam;
	THROW(InitBillPack());
	PPWaitStart();
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
		if(gobj.Fetch(grec.ID, &temp_grec) > 0 && !(temp_grec.Flags & GF_UNLIM) && !gobj.IsAsset(grec.ID)) {
			if(!excl_goodsgrp_id || gobj.BelongToGroup(grec.ID, excl_goodsgrp_id) <= 0) {
				THROW(ProcessGoods2(&grec, &(turned_bill_id = 0)));
				turned_bill_list.addnz(turned_bill_id);
			}
		}
		PPWaitPercent(iter.GetIterCounter());
	}
	THROW(TurnBillPack());
	PPWaitStop();
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
int UnifyGoodsPrice()
{
	int    ok = -1;
	PrcssrUnifyPriceFilt param;
	PrcssrUnifyPrice upb;
	if(param.Setup(0, LConfig.Location, 0) > 0 && upb.EditParam(&param) > 0) {
		PPObjGoodsGroup::SetDynamicOwner(param.GoodsGrpID, 0, 0L);
		ok = upb.Process(&param) ? 1 : PPErrorZ();
		PPObjGoodsGroup::RemoveDynamicAlt(param.GoodsGrpID, 0L);
	}
	return ok;
}
