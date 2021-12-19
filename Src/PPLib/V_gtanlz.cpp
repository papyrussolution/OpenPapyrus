// V_GTANLZ.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2014, 2015, 2016, 2017, 2019, 2020, 2021
// @codepage UTF-8
// Налоговый анализ товарооборота
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(GoodsTaxAnalyze); GoodsTaxAnalyzeFilt::GoodsTaxAnalyzeFilt() : PPBaseFilt(PPFILT_GOODSTAXANALYZE, 0, 1)
{
	SetFlatChunk(offsetof(GoodsTaxAnalyzeFilt, ReserveStart),
		offsetof(GoodsTaxAnalyzeFilt, BillList)-offsetof(GoodsTaxAnalyzeFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(GoodsTaxAnalyzeFilt, BillList));
	Init(1, 0);
	if(CConfig.IncomeCalcMethod == INCM_BYPAYMENT)
		Flags |= GoodsTaxAnalyzeFilt::fByPayment;
}

int GoodsTaxAnalyzeFilt::HasCycleFlags() const
{
	return BIN(Flags & (fDayly|fMonthly));
}

PPViewGoodsTaxAnalyze::PPViewGoodsTaxAnalyze() : PPView(0, &Filt, PPVIEW_GOODSTAXANALYZE, 0, 0), P_TempTbl(0), IterIdx(0), P_GGIter(0), P_InOutVATList(0)
{
}

PPViewGoodsTaxAnalyze::~PPViewGoodsTaxAnalyze()
{
	delete P_TempTbl;
	delete P_GGIter;
	delete P_InOutVATList;
	PPObjGoodsGroup::RemoveTempAlt(Filt.GoodsGrpID, (long)this);
}

const BVATAccmArray * PPViewGoodsTaxAnalyze::GetInOutVATList() const
{
	return P_InOutVATList;
}
//
//
//
int PPViewGoodsTaxAnalyze::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class GoodsTaxAnalyzeFiltDialog : public WLDialog {
		DECL_DIALOG_DATA(GoodsTaxAnalyzeFilt);
	public:
		enum {
			ctlgroupGoodsFilt = 1
		};
		GoodsTaxAnalyzeFiltDialog() : WLDialog(DLG_GDSGRPRLZ, CTL_GDSGRPRLZ_LABEL)
		{
			addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(0, CTLSEL_FLTLOT_GGRP, cmGoodsFilt));
			SetupCalPeriod(CTLCAL_GDSGRPRLZ_PERIOD, CTL_GDSGRPRLZ_PERIOD);
			SetupCalPeriod(CTLCAL_GDSGRPRLZ_LOTSPRD, CTL_GDSGRPRLZ_LOTSPRD);
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			PPID   acc_sheet_id = 0;
			PPIDArray types;
			ushort v = 0;
			SetPeriodInput(this, CTL_GDSGRPRLZ_PERIOD,  &Data.Period);
			SetPeriodInput(this, CTL_GDSGRPRLZ_LOTSPRD, &Data.LotsPeriod);
			SetupPPObjCombo(this, CTLSEL_GDSGRPRLZ_LOC, PPOBJ_LOCATION, Data.LocID, 0, 0);
			types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF, PPOPT_PAYMENT, PPOPT_GENERIC, 0L);
			SetupOprKindCombo(this, CTLSEL_GDSGRPRLZ_OP, Data.OpID, 0, &types, 0);
			GetOpCommonAccSheet(Data.OpID, &(acc_sheet_id = 0), 0);
			setupAccSheet(acc_sheet_id);
			GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
			setGroupData(ctlgroupGoodsFilt, &gf_rec);
			AddClusterAssoc(CTL_GDSGRPRLZ_FLAGS, 0, GoodsTaxAnalyzeFilt::fNozeroExciseOnly);
			AddClusterAssoc(CTL_GDSGRPRLZ_FLAGS, 1, GoodsTaxAnalyzeFilt::fIgnoreVatFreeTag);
			AddClusterAssoc(CTL_GDSGRPRLZ_FLAGS, 2, GoodsTaxAnalyzeFilt::fByPayment);
			AddClusterAssoc(CTL_GDSGRPRLZ_FLAGS, 3, GoodsTaxAnalyzeFilt::fOldStyleLedger);
			SetClusterData(CTL_GDSGRPRLZ_FLAGS, Data.Flags);
			DisableClusterItem(CTL_GDSGRPRLZ_FLAGS, 3, !Data.HasCycleFlags());
			setWL(BIN(Data.Flags & GoodsTaxAnalyzeFilt::fLabelOnly));
			if(Data.Flags & GoodsTaxAnalyzeFilt::fDiffAll)
				v = 1;
			else if(Data.Flags & GoodsTaxAnalyzeFilt::fDiffByInVAT) {
				if(Data.Flags & GoodsTaxAnalyzeFilt::fDiffByOutVAT)
					v = 4;
				else
					v = 2;
			}
			else if(Data.Flags & GoodsTaxAnalyzeFilt::fDiffByOutVAT)
				v = 3;
			else
				v = 0;
			setCtrlData(CTL_GDSGRPRLZ_DIFFTAX, &v);
			if(Data.Flags & GoodsTaxAnalyzeFilt::fDayly)
				v = 1;
			else if(Data.Flags & GoodsTaxAnalyzeFilt::fMonthly)
				v = 2;
			else if(Data.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots)
				v = 3;
			else
				v = 0;
			setCtrlData(CTL_GDSGRPRLZ_CCL, &v);
			SetupArCombo(this, CTLSEL_GDSGRPRLZ_SUPPL,   Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
			SetupArCombo(this, CTLSEL_GDSGRPRLZ_SUPPLAG, Data.SupplAgentID, OLW_LOADDEFONOPEN|OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
			disableCtrls(Data.BillList.IsExists(), CTLSEL_GDSGRPRLZ_OP, CTLSEL_GDSGRPRLZ_LOC, 0);
			SetupSubstGoodsCombo(this, CTLSEL_GDSGRPRLZ_GSUBST, Data.Sgg);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			ushort v = 0;
			GoodsFiltCtrlGroup::Rec gf_rec;
			THROW(GetPeriodInput(this, sel = CTL_GDSGRPRLZ_PERIOD, &Data.Period));
			THROW(AdjustPeriodToRights(Data.Period, 1));
			THROW(GetPeriodInput(this, sel = CTL_GDSGRPRLZ_LOTSPRD, &Data.LotsPeriod));
			getCtrlData(CTLSEL_GDSGRPRLZ_LOC, &Data.LocID);
			getCtrlData(CTLSEL_GDSGRPRLZ_OP,  &Data.OpID);
			getCtrlData(CTLSEL_GDSGRPRLZ_OBJ, &Data.ObjectID);
			THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
			Data.GoodsGrpID = gf_rec.GoodsGrpID;
			GetClusterData(CTL_GDSGRPRLZ_FLAGS, &Data.Flags);
			SETFLAG(Data.Flags, GoodsTaxAnalyzeFilt::fLabelOnly, getWL());
			getCtrlData(CTL_GDSGRPRLZ_DIFFTAX, &(v = 0));
			Data.Flags &= ~(GoodsTaxAnalyzeFilt::fDiffAll|GoodsTaxAnalyzeFilt::fDiffByInVAT|GoodsTaxAnalyzeFilt::fDiffByOutVAT);
			switch(v) {
				case 1: Data.Flags |= GoodsTaxAnalyzeFilt::fDiffAll; break;
				case 2: Data.Flags |= GoodsTaxAnalyzeFilt::fDiffByInVAT; break;
				case 3: Data.Flags |= GoodsTaxAnalyzeFilt::fDiffByOutVAT; break;
				case 4: Data.Flags |= (GoodsTaxAnalyzeFilt::fDiffByInVAT|GoodsTaxAnalyzeFilt::fDiffByOutVAT); break;
			}
			getCtrlData(CTL_GDSGRPRLZ_CCL, &(v = 0));
			Data.Flags &= ~(GoodsTaxAnalyzeFilt::fDayly|GoodsTaxAnalyzeFilt::fMonthly|GoodsTaxAnalyzeFilt::fLedgerByLots);
			switch(v) {
				case 1: Data.Flags |= GoodsTaxAnalyzeFilt::fDayly; break;
				case 2: Data.Flags |= GoodsTaxAnalyzeFilt::fMonthly; break;
				case 3: Data.Flags |= GoodsTaxAnalyzeFilt::fLedgerByLots; break;
			}
			getCtrlData(CTLSEL_GDSGRPRLZ_SUPPL,   &Data.SupplID);
			getCtrlData(CTLSEL_GDSGRPRLZ_SUPPLAG, &Data.SupplAgentID);
			getCtrlData(CTLSEL_GDSGRPRLZ_GSUBST, &Data.Sgg);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			WLDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GDSGRPRLZ_OP)) {
				if(getCtrlView(CTLSEL_GDSGRPRLZ_OBJ)) {
					PPID   acc_sheet_id = 0;
					getCtrlData(CTLSEL_GDSGRPRLZ_OP, &Data.OpID);
					GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
					setupAccSheet(acc_sheet_id);
				}
			}
			else if(event.isClusterClk(CTL_GDSGRPRLZ_CCL))
				DisableClusterItem(CTL_GDSGRPRLZ_FLAGS, 3, getCtrlUInt16(CTL_GDSGRPRLZ_CCL) == 0);
			else
				return;
			clearEvent(event);
		}
		void   setupAccSheet(PPID accSheetID)
		{
			SETIFZ(accSheetID, GetSellAccSheet());
			if(getCtrlView(CTLSEL_GDSGRPRLZ_OBJ)) {
				SetupArCombo(this, CTLSEL_GDSGRPRLZ_OBJ, Data.ObjectID, OLW_LOADDEFONOPEN, accSheetID, sacfDisableIfZeroSheet);
				if(!accSheetID && isCurrCtlID(CTL_GDSGRPRLZ_OBJ))
					selectNext();
			}
		}
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	DIALOG_PROC_BODY(GoodsTaxAnalyzeFiltDialog, static_cast<GoodsTaxAnalyzeFilt *>(pBaseFilt));
}

void PPViewGoodsTaxAnalyze::MakeTaxStr(GoodsGrpngEntry * pGGE, char * pBuf, size_t bufLen)
{
	PPGoodsTax gtx;
	PPGoodsTaxEntry gtx_entry;
	SString temp_buf;
	if(Filt.Flags & (GoodsTaxAnalyzeFilt::fDiffByInVAT | GoodsTaxAnalyzeFilt::fDiffByOutVAT | GoodsTaxAnalyzeFilt::fDiffAll)) {
		temp_buf.CatChar((pGGE->Flags & GGEF_TOGGLESTAX) ? 'A' : '.');
		temp_buf.CatChar((pGGE->Flags & GGEF_VATFREE)    ? '*' : '.');
		if(Filt.Flags & (GoodsTaxAnalyzeFilt::fDiffAll | GoodsTaxAnalyzeFilt::fDiffByOutVAT))
			if(pGGE->GoodsTaxGrpID) {
				if(GObj.GTxObj.FetchByID(pGGE->GoodsTaxGrpID, &gtx_entry) > 0)
					if(Filt.Flags & GoodsTaxAnalyzeFilt::fDiffAll)
						if(GObj.GTxObj.Search(gtx_entry.TaxGrpID & 0x00ffffffL, &gtx) > 0)
							temp_buf.Cat(strip(gtx.Name));
						else
							temp_buf.CatChar('#').Cat(gtx_entry.TaxGrpID & 0x00ffffffL);
					else
						temp_buf.Cat(gtx_entry.VAT, MKSFMTD(0, 1, NMBF_NOTRAILZ));
				else
					temp_buf.CatChar('#').Cat(pGGE->GoodsTaxGrpID);
			}
		if(Filt.Flags & (GoodsTaxAnalyzeFilt::fDiffAll | GoodsTaxAnalyzeFilt::fDiffByInVAT))
			if(pGGE->LotTaxGrpID && GObj.GTxObj.Search((pGGE->LotTaxGrpID & 0x00ffffffL), &gtx) > 0) {
				temp_buf.Space().CatChar('(');
				if(GObj.GTxObj.FetchByID(pGGE->LotTaxGrpID, &gtx_entry) > 0)
					if(Filt.Flags & GoodsTaxAnalyzeFilt::fDiffAll)
						if(GObj.GTxObj.Search(gtx_entry.TaxGrpID & 0x00ffffffL, &gtx) > 0)
							temp_buf.Cat(strip(gtx.Name));
						else
							temp_buf.CatChar('#').Cat(gtx_entry.TaxGrpID & 0x00ffffffL);
					else
						temp_buf.Cat(gtx.VAT, MKSFMTD(0, 1, NMBF_NOTRAILZ));
				else
					temp_buf.CatChar('#').Cat(pGGE->LotTaxGrpID);
				temp_buf.CatChar(')');
			}
	}
	temp_buf.CopyTo(pBuf, bufLen);
}

void PPViewGoodsTaxAnalyze::FormatCycle(LDATE dt, char * pBuf, size_t bufLen)
{
	Helper_FormatCycle(Filt.Cycl, CycleList, dt, pBuf, bufLen);
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsTaxAnlz);

int PPViewGoodsTaxAnalyze::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	int    by_comlex_paym = 0;
	SString temp_buf;
	const  bool is_price_wo_excise = LOGIC(CConfig.Flags & CCFLG_PRICEWOEXCISE);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	OpList.Z();
	if(Filt.OpID)
		if(IsGenericOp(Filt.OpID) > 0)
			GetGenericOpList(Filt.OpID, &OpList);
		else
			OpList.add(Filt.OpID);
	if(Filt.Cycl.Cycle && !Filt.HasCycleFlags()) {
		CycleList.init(&Filt.Period, Filt.Cycl);
		CycleList.getCycleParams(&Filt.Period, &Filt.Cycl);
	}
	else if(Filt.HasCycleFlags()) {
		Filt.Cycl.Cycle = (Filt.Flags & GoodsTaxAnalyzeFilt::fDayly) ? PRD_DAY : PRD_MONTH;
		Filt.Cycl.NumCycles = 0;
		CycleList.init(&Filt.Period, Filt.Cycl);
		CycleList.getCycleParams(&Filt.Period, &Filt.Cycl);
	}
	else
		CycleList.freeAll();
	THROW(AdjustPeriodToRights(Filt.Period, 0));
	//
	Gsl.Clear();
	ZDELETE(P_TempTbl);
	ZDELETE(P_InOutVATList);
	THROW_MEM(P_InOutVATList = new BVATAccmArray(BVATF_SUMZEROVAT));
	THROW(P_TempTbl = CreateTempFile());
	{
		uint   i;
		int    monthly = 0; // Признак того, что отчет рассчитывается по месяцам (фактически расчет осуществляется по дням, но группируется в месяцы).
		PPGoodsTaxEntry gtx;
		GoodsGrpngArray gga;
		PPIDArray goods_list, local_goods_list;
		Goods2Tbl::Rec goods_rec;
		TSVector <DateRange> period_list;
		if(!Filt.Cycl)
			period_list.insert(&Filt.Period);
		else if(Filt.Flags & GoodsTaxAnalyzeFilt::fMonthly) {
			PPCycleArray temp_cycle_list;
			Filt.Cycl.NumCycles = 0;
			temp_cycle_list.init(&Filt.Period, PRD_DAY, 0);
			for(uint cycle_no = 0; cycle_no < temp_cycle_list.getCount(); ++cycle_no)
				period_list.insert(&temp_cycle_list.at(cycle_no));
			monthly = 1;
		}
		else { // GoodsTaxAnalyzeFilt::fDayly
			for(uint cycle_no = 0; cycle_no < CycleList.getCount(); ++cycle_no)
				period_list.insert(&CycleList.at(cycle_no));
		}
		if(Filt.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots) {
			PPViewLot lot_view;
			LotFilt lot_filt;
			LotViewItem lot_item;
			const LDATE upp_date = NZOR(Filt.Period.upp, getcurdate_()); // @v10.8.10 LConfig.OperDate-->getcurdate_()
			lot_filt.Period.Set(ZERODATE, upp_date);
			lot_filt.LocList.Add(Filt.LocID);
			lot_filt.GoodsGrpID = Filt.GoodsGrpID;
			PPObjBill::PplBlock ppl_blk(lot_filt.Period, 0, 0);
			ppl_blk.Flags |= PPObjBill::PplBlock::fGatherPaym;
			ppl_blk.GatherPaymPeriod = Filt.Period;
			THROW(lot_view.Init_(&lot_filt));
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(lot_view.InitIteration(); lot_view.NextIteration(&lot_item) > 0; PPWaitPercent(lot_view.GetCounter())) {
				/*
					Paym - оплаченная часть отгрузки до конца периода
					PaymBefore - оплаченная часть отгрузки до начала периода
					LotPaym - оплаченная часть прихода до конца периода
					LotPaymBefore - оплаченная часть прихода до начала периода

					expend1 = (LotPaym - LotPaymBefore) * PaymBefore;
					expend2 = (Paym - PaymBefore) * LotPaym
					expend = expend1 + expend2

					expend_before = PaymBefore * LotPaymBefore
				*/
				if(upp_date && lot_item.Dt <= upp_date) {
					PPObjBill::EprBlock epr;
					const int r = p_bobj->GetPayoutPartOfLot(lot_item.ID, ppl_blk, 0);
					if(r == 1) {
						const double tolerance = 1.0e-7;
						//
						// Все рассчитываем только для оригинальных лотов чтобы не увеличивать
						// результат при обработке порожденных.
						//
						const double lot_paym_part = ppl_blk.Part;               // LotPaym
						const double lot_paym_part_before = ppl_blk.PartBefore;  // LotPaymBefore
						const double rest = ((lot_item.Cost * lot_item.Quantity) * (1.0 - lot_paym_part));
						double exp_part = 0.0;                                   // Paym
						double exp_part_before = 0.0;                            // PaymBefore
						{
							DateRange op_period;
							{
								op_period.Set(ZERODATE, Filt.Period.upp);
								p_bobj->GetExpendedPartOfReceipt(lot_item.ID, &op_period, 0, epr);
								const double _amt = epr.Amount;
								if(fabs(_amt) > tolerance && fabs(epr.Payout) <= (fabs(_amt)+tolerance))
									exp_part = fabs(fdivnz(epr.Payout, _amt));
							}
							//
							{
								op_period.Set(ZERODATE, plusdate(Filt.Period.low, -1));
								p_bobj->GetExpendedPartOfReceipt(lot_item.ID, &op_period, 0, epr);
								const double _amt = epr.Amount;
								if(fabs(_amt) > tolerance && fabs(epr.Payout) <= (fabs(_amt)+tolerance))
									exp_part_before = fabs(fdivnz(epr.Payout, _amt));
							}
						}
						double rest_part = (1.0 - exp_part) * lot_paym_part;
						if(fabs(rest_part) < tolerance)
							rest_part = 0.0;
						const double expend2 = (exp_part - exp_part_before) * lot_paym_part;
						const uint c = ppl_blk.PaymList.getCount();
						if(rest_part != 0.0 || expend2 != 0.0 || c) {
							TempGoodsTaxAnlzTbl::Rec rec;
							Goods2Tbl::Rec goods_rec;
							// @v10.6.4 MEMSZERO(rec);
							rec.LotID = lot_item.ID;
							rec.GoodsID = lot_item.GoodsID;
							const int _gfr = GObj.Fetch(lot_item.GoodsID, &goods_rec);
							if(_gfr > 0) {
								rec.GoodsGrpID = goods_rec.ParentID;
								STRNSCPY(rec.Name, goods_rec.Name);
							}
							double cost = lot_item.Cost;
							double cost_wo_tax = lot_item.Cost;
							double vat = 0.0;
							double stax = 0.0;
							{
								double tax_factor = 1.0;
								const int vat_free = IsLotVATFree(lot_item);
								const LDATE org_date = lot_item.Dt;
								if(lot_item.Flags & LOTF_COSTWOVAT) {
									GObj.AdjCostToVat(lot_item.InTaxGrpID, goods_rec.TaxGrpID, org_date, tax_factor, &cost, 1, vat_free);
								}
								{
									PPID   in_tax_grp_id = NZOR(lot_item.InTaxGrpID, goods_rec.TaxGrpID);
									long   amt_fl = 0;
									GTaxVect vect(5);
									PPGoodsTaxEntry gt;
									if(GObj.GTxObj.Fetch(in_tax_grp_id, org_date, 0L, &gt) > 0) {
										amt_fl = ~GTAXVF_SALESTAX;
										const long excl_fl = (vat_free > 0) ? GTAXVF_VAT : 0;
										vect.Calc_(&gt, cost, tax_factor, amt_fl, excl_fl);
										vat = vect.GetValue(GTAXVF_VAT);
										stax = vect.GetValue(GTAXVF_SALESTAX);
										cost_wo_tax -= (vat + stax);
									}
								}
							}
							rec.TrnovrCost = cost_wo_tax;
							rec.C_VATSum = cost - cost_wo_tax - stax; // vat
							rec.C_STaxSum = stax;
							//rec.Rest = lot_item.Quantity * (1.0 - lot_paym_part);
							rec.Rest = lot_item.Quantity * rest_part;
							if(rec.Rest != 0.0 || expend2 != 0.0) {
								rec.Dt = lot_item.Dt;
								BillTbl::Rec bill_lot_rec;
								if(p_bobj->Fetch(lot_item.BillID, &bill_lot_rec) > 0) {
									STRNSCPY(rec.BillNo, bill_lot_rec.Code);
								}
								rec.ExpQtty = expend2 * lot_item.Quantity;
								THROW_DB(P_TempTbl->insertRecBuf(&rec));
								rec.ExpQtty = 0.0;
								rec.Rest = 0.0;
							}
							for(uint i = 0; i < c; i++) {
								const RAssoc & r_paym_item = ppl_blk.PaymList.at(i);
								if(r_paym_item.Val != 0.0 || rec.Rest != 0.0) {
									BillTbl::Rec paym_rec;
									if(p_bobj->Fetch(r_paym_item.Key, &paym_rec) > 0) {
										const double factor = lot_item.Quantity * (r_paym_item.Val/ppl_blk.NominalAmount);
										rec.Dt = paym_rec.Dt;
										STRNSCPY(rec.BillNo, paym_rec.Code);
										rec.ExpQtty = factor * exp_part_before;
										rec.Qtty = factor;
										THROW_DB(P_TempTbl->insertRecBuf(&rec));
										rec.Rest = 0.0;
									}
								}
							}
						}
					}
				}
			}
			THROW(tra.Commit());
		}
		else {
			// @v10.7.7 {
			PPIDArray common_goods_list; 
			const  int  income_calc_method = (Filt.Flags & GoodsTaxAnalyzeFilt::fByPayment) ? INCM_BYPAYMENT : INCM_BYSHIPMENT;
			if(!Filt.BillList.IsExists()) {
				for(GoodsIterator giter(Filt.GoodsGrpID, GoodsIterator::ordByName); giter.Next(&goods_rec) > 0;)
					if(!(goods_rec.Flags & GF_GENERIC))
						common_goods_list.add(goods_rec.ID);
			}
			// } @v10.7.7
			PPObjGoods::SubstBlock sgg_blk;
			sgg_blk.ExclParentID = Filt.GoodsGrpID;
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(uint cycle_no = 0; cycle_no < period_list.getCount(); ++cycle_no) {
				GCTFilt gctf;
				AdjGdsGrpng agg;
				const  DateRange & r_period = period_list.at(cycle_no);
				LDATE  entry_date = r_period.low;
				if(monthly) {
					DateRange p_;
					if(CycleList.searchPeriodByDate(r_period.low, &p_) > 0)
						entry_date = p_.low;
				}
				gctf.Period = r_period;
				gctf.OpID   = Filt.OpID;
				gctf.LotsPeriod = Filt.LotsPeriod;
				gctf.LocList.Add(Filt.LocID);
				gctf.Flags |= (OPG_SETTAXES | OPG_DIFFBYTAX | OPG_PROCESSRECKONING | OPG_PROCESSGENOP | OPG_MERGECORRECTION); // @v10.7.5 OPG_MERGECORRECTION
				gctf.SupplID      = Filt.SupplID;
				gctf.SupplAgentID = Filt.SupplAgentID;
				if(Filt.BillList.IsExists()) {
					gctf.BillList = Filt.BillList;
					gctf.Flags &= ~(OPG_CALCINREST | OPG_CALCOUTREST);
				}
				if(Filt.HasCycleFlags() && Filt.Flags & GoodsTaxAnalyzeFilt::fByPayment && !(Filt.Flags & GoodsTaxAnalyzeFilt::fOldStyleLedger))
					gctf.Flags |= OPG_COSTBYPAYM;
				if(Filt.Flags & GoodsTaxAnalyzeFilt::fNozeroExciseOnly)
					gctf.Flags |= static_cast<uint>(OPG_NOZEROEXCISE);
				if(Filt.Flags & GoodsTaxAnalyzeFilt::fLabelOnly)
					gctf.Flags |= static_cast<uint>(OPG_LABELONLY);
				if(oneof2(Filt.Sgg, sggSuppl, sggSupplAgent))
					gctf.Flags |= OPG_PROCESSBYLOTS;
				THROW(agg.BeginGoodsGroupingProcess(gctf));
				goods_list.clear();
				if(gctf.BillList.IsExists()) {
					local_goods_list.clear();
					const PPIDArray & r_bill_list = gctf.BillList.Get();
					for(i = 0; i < r_bill_list.getCount(); i++)
						THROW(p_bobj->trfr->CalcBillTotal(r_bill_list.get(i), 0, &local_goods_list));
					local_goods_list.sortAndUndup();
					for(i = 0; i < local_goods_list.getCount(); i++) {
						const PPID goods_id = local_goods_list.get(i);
						if(GObj.Fetch(goods_id, &goods_rec) > 0 && !(goods_rec.Flags & GF_GENERIC) && GObj.BelongToGroup(goods_id, Filt.GoodsGrpID))
							goods_list.add(goods_id);
					}
				}
				else {
					/* @v10.7.7 for(GoodsIterator giter(Filt.GoodsGrpID, GoodsIterator::ordByName); giter.Next(&goods_rec) > 0;)
						if(!(goods_rec.Flags & GF_GENERIC))
							goods_list.add(goods_rec.ID);*/
					goods_list = common_goods_list; // @v10.7.7
				}
				const uint gc = goods_list.getCount();
				for(uint j = 0; j < gc; j++) {
					const PPID goods_id = goods_list.get(j);
					THROW(PPCheckUserBreak());
					const int gfr = GObj.Fetch(goods_id, &goods_rec);
					//assert(gfr > 0); // @debug
					if(gfr > 0) {
						GoodsGrpngEntry * p_gge;
						gctf.GoodsID = goods_id;
						gctf.GoodsGrpID = 0;
						gga.Reset();
						THROW(gga.ProcessGoodsGrouping(gctf, &agg));
						for(i = 0; gga.enumItems(&i, (void **)&p_gge);) {
							PPID   op_id = 0;
							if(!Filt.OpID) {
								op_id = p_gge->IsProfitable(BIN(income_calc_method == INCM_BYPAYMENT));
								if(!op_id && p_gge->Flags & GGEF_PAYMBYPAYOUTLOT) {
									p_gge->Price = 0.0;
									p_gge->Discount = 0.0;
									op_id = p_gge->OpID;
								}
							}
							else if(OpList.lsearch(p_gge->OpID))
								op_id = Filt.OpID;
							if(op_id && !(Filt.Flags & GoodsTaxAnalyzeFilt::fByPayment && GetOpType(op_id) == PPOPT_GOODSRETURN)) {
								PPID   final_goods_id = goods_id;
								bool   is_new_rec = false;
								int    r;
								TempGoodsTaxAnlzTbl::Rec rec;
								TempGoodsTaxAnlzTbl::Key0 k;
								GTaxVect vect;
								if(Filt.Sgg) {
									sgg_blk.LotID = p_gge->LotID;
									THROW(GObj.SubstGoods(goods_id, &final_goods_id, Filt.Sgg, &sgg_blk, &Gsl));
								}
								//
								// Ищем аналогичную запись
								//
								int32  _goods_tax_grp_id = 0;
								int32  _lot_tax_grp_id = 0;
								int32  _tax_flags = 0;
								MEMSZERO(k);
								k.Dt = entry_date;
								k.GoodsID = final_goods_id;
								if(Filt.Flags & GoodsTaxAnalyzeFilt::fDiffAll) {
									_goods_tax_grp_id = p_gge->GoodsTaxGrpID;
									_lot_tax_grp_id   = p_gge->LotTaxGrpID;
									_tax_flags        = (p_gge->Flags & (GGEF_VATFREE|GGEF_TOGGLESTAX|GGEF_LOCVATFREE|GGEF_PAYMBYPAYOUTLOT));
								}
								else {
									if((Filt.Flags & GoodsTaxAnalyzeFilt::fDiffByOutVAT) &&
										p_gge->GoodsTaxGrpID && GObj.GTxObj.Search((p_gge->GoodsTaxGrpID & 0x00ffffffL), &gtx) > 0)
										_goods_tax_grp_id = (long)(gtx.VAT * 100);
									if(Filt.Flags & GoodsTaxAnalyzeFilt::fDiffByInVAT) {
										if(p_gge->LotTaxGrpID && GObj.GTxObj.Search(p_gge->LotTaxGrpID, &gtx) > 0)
											_lot_tax_grp_id = (long)(gtx.VAT * 100);
										_tax_flags = (p_gge->Flags & GGEF_VATFREE);
									}
								}
								k.GoodsTaxGrpID = _goods_tax_grp_id;
								k.LotTaxGrpID   = _lot_tax_grp_id;
								k.TaxFlags      = _tax_flags;
								THROW(r = SearchByKey(P_TempTbl, 0, &k, &rec));
								if(r < 0) {
									is_new_rec = true;
									MEMSZERO(rec);
									rec.Dt = entry_date;
									rec.GoodsID = final_goods_id;
									rec.GoodsGrpID = Filt.Sgg ? 0 : goods_rec.ParentID;
									if(Filt.Sgg) {
										GObj.GetSubstText(final_goods_id, Filt.Sgg, &Gsl, temp_buf);
										STRNSCPY(rec.Name, temp_buf);
									}
									else
										STRNSCPY(rec.Name, goods_rec.Name);
									MakeTaxStr(p_gge, rec.TaxStr, sizeof(rec.TaxStr));
									rec.GoodsTaxGrpID = _goods_tax_grp_id;
									rec.LotTaxGrpID   = _lot_tax_grp_id;
									rec.TaxFlags      = _tax_flags;
								}
								int    is_exp = 0;
								double income = 0.0;
								if(p_gge->Flags & GGEF_PAYMBYPAYOUTLOT) {
									is_exp = 1;
									income = -p_gge->Cost;
								}
								else {
									is_exp = (IsGenericOp(Filt.OpID) > 0) ? BIN(p_gge->Sign < 0) : BIN(Filt.OpID || (p_gge->Sign < 0));
									income = p_gge->Income((Filt.Flags & GoodsTaxAnalyzeFilt::fByPayment) ? INCM_BYPAYMENT : INCM_BYSHIPMENT);
								}
								const double trnovr_p = is_exp ? p_gge->Price : -p_gge->Price;
								const double trnovr_c = is_exp ? p_gge->Cost  : -p_gge->Cost;
								const double discount = is_exp ? p_gge->Discount : -p_gge->Discount;
								const PPID   lot_tax_grp_id = p_gge->LotTaxGrpID;
								long   excl_flags = 0L;
								rec.Qtty        += (is_exp ? p_gge->Quantity : -p_gge->Quantity);
								rec.PhQtty      += (is_exp ? p_gge->Volume   : -p_gge->Volume);
								rec.TrnovrPrice += trnovr_p;
								rec.TrnovrCost  += trnovr_c;
								rec.Income      += income;
								if(R6(rec.Qtty) != 0.0) {
									BVATAccm in_item, out_item;
									if(PPObjGoodsTax::FetchByID(p_gge->GoodsTaxGrpID, &gtx) > 0) {
										const  long amt_flags = (p_gge->Flags & GGEF_PRICEWOTAXES) ? GTAXVF_AFTERTAXES : GTAXVF_BEFORETAXES;
										const  int  re = BIN(p_gge->Flags & GGEF_TOGGLESTAX);
										if(p_gge->Flags & GGEF_LOCVATFREE) {
											excl_flags |= GTAXVF_VAT;
											out_item.IsVatFree = 1;
										}
										if(is_price_wo_excise ? !re : re)
											excl_flags |= GTAXVF_SALESTAX;
										vect.Calc_(&gtx, trnovr_p, fabs(p_gge->TaxFactor), amt_flags, excl_flags);
										double excs = fabs(vect.GetValue(GTAXVF_EXCISE));
										rec.ExciseSum += is_exp ? excs : -excs;
										rec.STaxSum   += vect.GetValue(GTAXVF_SALESTAX);
										rec.VATSum    += vect.GetValue(GTAXVF_VAT);
										out_item.PRate    = out_item.CRate = vect.GetTaxRate(GTAX_VAT, 0);
										out_item.Price    = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT | GTAXVF_EXCISE);
										out_item.PVATSum  = vect.GetValue(GTAXVF_VAT);
										out_item.PTrnovr  = trnovr_p;
										out_item.Discount = discount;
										//
										// Рассчитываем НДС с дохода
										//
										double income_minus_stax = income - vect.GetValue(GTAXVF_SALESTAX);
										vect.Calc_(&gtx, income_minus_stax, fabs(p_gge->TaxFactor), GTAXVF_BEFORETAXES, GTAXVF_SALESTAX);
										rec.IncVATSum += vect.GetValue(GTAXVF_VAT);
									}
									//
									// Рассчитываем НДС в ценах поступления //
									//
									if(PPObjGoodsTax::FetchByID(lot_tax_grp_id, &gtx) > 0) {
										excl_flags = 0L;
										if(p_gge->LotTaxGrpID == 0)
											excl_flags |= GTAXVF_EXCISE;
										SETFLAG(excl_flags, GTAXVF_SALESTAX, !(p_gge->Flags & GGEF_COSTWSTAX));
										if(p_gge->Flags & GGEF_VATFREE)
											excl_flags |= GTAXVF_VAT;
										vect.Calc_(&gtx, trnovr_c, fabs(p_gge->TaxFactor), GTAXVF_BEFORETAXES, excl_flags);
										rec.C_VATSum  += vect.GetValue(GTAXVF_VAT);
										rec.C_STaxSum += vect.GetValue(GTAXVF_SALESTAX);
										in_item.PRate   = in_item.CRate   = vect.GetTaxRate(GTAX_VAT, 0);
										in_item.Cost    = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_VAT);
										in_item.CVATSum = vect.GetValue(GTAXVF_VAT);
									}
									THROW_DB(is_new_rec ? P_TempTbl->insertRecBuf(&rec) : P_TempTbl->updateRecBuf(&rec));
									// AHTOXA Calc in/out vat {
									if(P_InOutVATList) {
										THROW(P_InOutVATList->Add(&in_item));
										THROW(P_InOutVATList->Add(&out_item));
									}
									// } AHTOXA
								}
								else if(!is_new_rec)
									THROW_DB(P_TempTbl->deleteRec());
							}
						}
					}
					long   iters_count = gc;
					long   iter_no     = j;
					if(period_list.getCount()) {
						iter_no += (cycle_no * iters_count);
						iters_count *= period_list.getCount();
					}
					PPWaitPercent(iter_no, iters_count);
				}
				agg.EndGoodsGroupingProcess();
			}
			THROW(tra.Commit());
		}
	}
	PPObjGoodsGroup::SetOwner(Filt.GoodsGrpID, 0, (long)this);
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int PPViewGoodsTaxAnalyze::InitIterQuery(PPID grpID)
{
	// @v10.6.8 char   k_[MAXKEYLEN];
	BtrDbKey k__; // @v10.6.8
	TempGoodsTaxAnlzTbl::Key2 k2;
	int  sp_mode = spFirst;
   	void * k = k__; // memzero(k_, sizeof(k_));
	delete P_IterQuery;
	P_IterQuery = new BExtQuery(P_TempTbl, IterIdx, 14);
	P_IterQuery->selectAll();
	if(grpID) {
		P_IterQuery->where(P_TempTbl->GoodsGrpID == grpID);
		MEMSZERO(k2);
		k2.GoodsGrpID = grpID;
		k = &k2;
		sp_mode = spGe;
	}
	P_IterQuery->initIteration(0, k, sp_mode);
	return 1;
}

int PPViewGoodsTaxAnalyze::InitIteration(IterOrder ord)
{
	int    ok = 1;
	IterIdx = 0;
	IterGrpName.Z();
	ZDELETE(P_GGIter);
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	PPInitIterCounter(Counter, P_TempTbl);
	if(ord == OrdByID)
		IterIdx = 0;
	else if(ord == OrdByName)
		IterIdx = 1;
	else if(ord == OrdByGrp_Name) {
		if(Filt.Sgg)
			IterIdx = 1;
		else {
			IterIdx = 2;
			THROW_MEM(P_GGIter = new GoodsGroupIterator(Filt.GoodsGrpID));
		}
	}
	else
		IterIdx = 1;
	if(IterIdx != 2)
		InitIterQuery(0);
	else
		NextOuterIteration();
	CATCHZOK
	return ok;
}

int PPViewGoodsTaxAnalyze::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName) > 0) {
		InitIterQuery(grp_id);
		return 1;
	}
	else
		return -1;
}

int FASTCALL PPViewGoodsTaxAnalyze::NextIteration(GoodsTaxAnalyzeViewItem * pItem)
{
	do {
		if(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			if(pItem) {
				const TempGoodsTaxAnlzTbl::Rec & r_rec = P_TempTbl->data;
				memzero(pItem, sizeof(GoodsTaxAnalyzeViewItem));
				pItem->Dt   = r_rec.Dt;
				pItem->GoodsID      = r_rec.GoodsID;
				pItem->GoodsGrpID   = r_rec.GoodsGrpID;
				pItem->P_GoodsGrpName = IterGrpName;
				pItem->Qtty = r_rec.Qtty;
				pItem->PhQtty       = r_rec.PhQtty;
				pItem->TrnovrCost   = r_rec.TrnovrCost;
				pItem->TrnovrPrice  = r_rec.TrnovrPrice;
				pItem->Income       = r_rec.Income;
				pItem->LotTaxGrpID  = r_rec.LotTaxGrpID;
				pItem->GoodsTaxGrpID = r_rec.GoodsTaxGrpID;
				pItem->TaxFlags     = r_rec.TaxFlags;
				pItem->ExciseSum    = r_rec.ExciseSum;
				pItem->C_VATSum     = r_rec.C_VATSum;
				pItem->VATSum       = r_rec.VATSum;
				pItem->IncVATSum    = r_rec.IncVATSum;
				pItem->STaxSum      = r_rec.STaxSum;
				pItem->ExpQtty      = r_rec.ExpQtty;
				pItem->Rest = r_rec.Rest;
				STRNSCPY(pItem->BillNo, r_rec.BillNo);
				STRNSCPY(pItem->Name, r_rec.Name);
				STRNSCPY(pItem->TaxStr, r_rec.TaxStr);
			}
			Counter.Increment();
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

DBQuery * PPViewGoodsTaxAnalyze::CreateBrowserQuery(uint * pBrwId, SString *)
{
	uint   brw_id = Filt.HasCycleFlags() ? BROWSER_GOODSTAXANLZ_D : BROWSER_GOODSTAXANLZ;
	TempGoodsTaxAnlzTbl * t = new TempGoodsTaxAnlzTbl(P_TempTbl->GetName());
	DBQuery * q = 0;
	THROW(CheckTblPtr(t));
	if(Filt.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots) {
		brw_id = BROWSER_GOODSTAXANLZ_PIL;
		q = & select(
			t->GoodsID,     // #0
			t->LotID,       // #1
			t->Dt,          // #2
			t->BillNo,      // #3
			t->Name,        // #4
			t->TrnovrCost,  // #5
			t->C_VATSum,    // #6
			t->C_STaxSum,   // #7
			(t->TrnovrCost + t->C_VATSum + t->C_STaxSum) * t->Qtty,     // #8
			(t->TrnovrCost + t->C_VATSum + t->C_STaxSum) * t->ExpQtty,  // #9
			(t->TrnovrCost + t->C_VATSum + t->C_STaxSum) * t->Rest,     // #10
			0L).from(t, 0L).orderBy(t->Dt, t->Name, 0L);
	}
	else {
		brw_id = Filt.HasCycleFlags() ? BROWSER_GOODSTAXANLZ_D : BROWSER_GOODSTAXANLZ;
		q = & select(
			t->GoodsID,     // #0
			t->Dt,          // #1
			t->Name,        // #2
			t->TaxStr,      // #3
			t->TrnovrPrice, // #4
			t->Income,      // #5
			t->VATSum,      // #6
			t->ExciseSum,   // #7
			t->STaxSum,     // #8
			t->TrnovrCost,  // #9
			t->ExpQtty,     // #10
			t->Rest,        // #11
			t->BillNo,      // #12
			0L).from(t, 0L).orderBy(t->Dt, t->Name, 0L);
	}
	THROW(CheckQueryPtr(q));
	CATCH
		if(q) {
			ZDELETE(q);
		}
		else
			delete t;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int PPViewGoodsTaxAnalyze::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int   ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		PPID   goods_id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		if(ppvCmd == PPVCMD_EDITGOODS) {
			ok = (goods_id && GObj.Edit(&goods_id, 0) == cmOK) ? 1 : -1;
		}
	}
	return ok;
}

int PPViewGoodsTaxAnalyze::Print(const void *)
{
	uint   rpt_id = 0;
	IterOrder order;
	if(Filt.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots) {
		rpt_id = REPORT_GTAXANLZ_LEDGBYLOTS;
	}
	else if(Filt.HasCycleFlags()) {
		TDialog * dlg = new TDialog(DLG_PRNPRP);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setCtrlUInt16(CTL_PRNPRP_WHAT, 0);
			if(ExecView(dlg) == cmOK) {
				ushort v = dlg->getCtrlUInt16(CTL_PRNPRP_WHAT);
				if(v == 0)
					rpt_id = REPORT_GTAXANLZ_PRPRCPT;
				else if(v == 1)
					rpt_id = REPORT_GTAXANLZ_PRPEXPND;
			}
			delete dlg;
		}
		order = OrdByName;
	}
	else {
		rpt_id = REPORT_GTAXANLZ;
		order = Filt.Sgg ? OrdByName : OrdByGrp_Name;
	}
	if(rpt_id) {
		//
		// При сортировке OrdByGrp_Name  InitIteration;NextIteration не находят товары для печати из-за того,
		// что товары привязываются к временной альт. группе через ассоциации, а не через свои родительские группы,
		// поэтому приходится при печати в фмльтре не указывать эту альт. группу.
		//
		PPID  cur_grp_id = 0;
		if(order == OrdByGrp_Name && Filt.GoodsGrpID && PPObjGoodsGroup::IsTempAlt(Filt.GoodsGrpID) > 0) {
			cur_grp_id = Filt.GoodsGrpID;
			Filt.GoodsGrpID = 0;
		}
		//
		PPReportEnv env;
		env.Sort = order;
		PPAlddPrint(rpt_id, PView(this), &env);
		if(cur_grp_id)
			Filt.GoodsGrpID = cur_grp_id;
		return 1;
	}
	else
		return -1;
}

int PPViewGoodsTaxAnalyze::PrintTotal(const GoodsTaxAnalyzeTotal * pTotal)
{
	GTaxAnlzTotalPrintData  gtatpd;
	gtatpd.P_Total   = pTotal;
	gtatpd.P_Filt    = &Filt;
	gtatpd.P_VATList = P_InOutVATList;
	return PPAlddPrint(REPORT_GTAXANLZTOTAL, PView(&gtatpd), 0);
}

int PPViewGoodsTaxAnalyze::Detail(const void * pHdr, PPViewBrowser *)
{
	struct BrwHdr {
		PPID   GoodsID;
		LDATE  Dt;
	};
	int    ok = -1;
	const  BrwHdr * p_hdr = static_cast<const BrwHdr *>(pHdr);
	PPID   id = p_hdr ? p_hdr->GoodsID : 0;
	LDATE  dt = p_hdr ? p_hdr->Dt : ZERODATE;
	if(id) {
		id &= ~GOODSSUBSTMASK;
		if(Filt.Sgg) {
			GoodsTaxAnalyzeFilt temp_filt;
			temp_filt = Filt;
			temp_filt.Sgg = sggNone;
			if(Filt.Sgg == sggGroup)
				temp_filt.GoodsGrpID = id;
			else if(Filt.Sgg == sggSuppl)
				temp_filt.SupplID = id;
			else if(Filt.Sgg == sggSupplAgent)
				temp_filt.SupplAgentID = id;
			ok = ViewGoodsTaxAnalyze(&temp_filt);
		}
		else {
			OpGroupingFilt op_grpng_flt;
			if(!Filt.Cycl.Cycle || !dt || CycleList.searchPeriodByDate(dt, &op_grpng_flt.Period) <= 0)
				op_grpng_flt.Period = Filt.Period;
			op_grpng_flt.LotsPeriod = Filt.LotsPeriod;
			op_grpng_flt.LocList.Add(Filt.LocID);
			op_grpng_flt.SupplID = Filt.SupplID;
			op_grpng_flt.GoodsID = id;
			if(Filt.HasCycleFlags() && Filt.Flags & GoodsTaxAnalyzeFilt::fByPayment && !(Filt.Flags & GoodsTaxAnalyzeFilt::fOldStyleLedger))
				op_grpng_flt.Flags |= OpGroupingFilt::fCostByPaym;
			ViewOpGrouping(&op_grpng_flt);
		}
	}
	return ok;
}
//
//
//
DECL_CMPFUNC(BVATAccm);

int PPViewGoodsTaxAnalyze::CalcTotal(GoodsTaxAnalyzeTotal * pTotal)
{
	int    ok = -1;
	GoodsTaxAnalyzeViewItem item;
	if(pTotal) {
		memzero(pTotal, sizeof(*pTotal));
		PPWaitStart();
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			pTotal->Count++;
			if(Filt.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots) {
				const double cost = (item.TrnovrCost + item.C_VATSum + item.C_STaxSum);
				pTotal->PilRcptSum += cost * item.Qtty;
				pTotal->PilExpSum  += cost * item.ExpQtty;
				pTotal->PilRestSum += cost * item.Rest;
			}
			else {
				pTotal->TrnovrCost  += item.TrnovrCost;
				pTotal->TrnovrPrice += item.TrnovrPrice;
				pTotal->Income      += item.Income;
				pTotal->ExciseSum   += item.ExciseSum;
				pTotal->C_VATSum    += item.C_VATSum;
				pTotal->IncVATSum   += item.IncVATSum;
				pTotal->VATSum      += item.VATSum;
				pTotal->STaxSum     += item.STaxSum;
			}
		}
		CALLPTRMEMB(P_InOutVATList, sort(PTR_CMPFUNC(BVATAccm)));
		PPWaitStop();
		ok = 1;
	}
	return ok;
}

void PPViewGoodsTaxAnalyze::ViewTotal()
{
	class GTaxAnlzTotalDialog : public TDialog {
	public:
		GTaxAnlzTotalDialog(GoodsTaxAnalyzeTotal * pTotal, PPViewGoodsTaxAnalyze * pV) : TDialog(DLG_GTANLZTOTAL), P_V(pV)
		{
			if(!RVALUEPTR(Total, pTotal))
				MEMSZERO(Total);
		}
		int  setDTS()
		{
			int    ok = 1;
			setCtrlLong(CTL_GTANLZTOTAL_COUNT,   Total.Count);
			setCtrlReal(CTL_GTANLZTOTAL_TOCOST,  Total.TrnovrCost);
			setCtrlReal(CTL_GTANLZTOTAL_TOPRICE, Total.TrnovrPrice);
			setCtrlReal(CTL_GTANLZTOTAL_INCOME,  Total.Income);
			setCtrlReal(CTL_GTANLZTOTAL_EXCISE,  Total.ExciseSum);
			setCtrlReal(CTL_GTANLZTOTAL_CVAT,    Total.C_VATSum);
			setCtrlReal(CTL_GTANLZTOTAL_INCVAT,  Total.IncVATSum);
			setCtrlReal(CTL_GTANLZTOTAL_VAT,     Total.VATSum);
			setCtrlReal(CTL_GTANLZTOTAL_STAX,    Total.STaxSum);
			if(P_V) {
				const BVATAccmArray * p_inout_vatlist = P_V->GetInOutVATList();
				if(p_inout_vatlist) {
					SmartListBox * p_list = 0;
					BVATAccm * p_item = 0;
					SString sub;
					StringSet ss(SLBColumnDelim);
					THROW(SetupStrListBox(this, CTL_GTANLZTOTAL_INOUTVL));
					THROW(p_list = static_cast<SmartListBox *>(getCtrlView(CTL_GTANLZTOTAL_INOUTVL)));
					for(uint i = 0; p_inout_vatlist->enumItems(&i, (void **)&p_item) > 0;) {
						ss.clear();
						ss.add(sub.Z().Cat(p_item->PRate, MKSFMTD(0, 1, 0)).CatChar('%'));
						if(p_item->IsVatFree || p_item->PRate == 0.0) {
							ss.add(sub.Z().Cat(p_item->Cost,  MKSFMTD(0, 2, NMBF_TRICOMMA|ALIGN_RIGHT)));
							ss.add(sub.Z().Cat(p_item->Price, MKSFMTD(0, 2, NMBF_TRICOMMA|ALIGN_RIGHT)));
						}
						else {
							ss.add(sub.Z().Cat(p_item->CVATSum, MKSFMTD(0, 2, NMBF_TRICOMMA|ALIGN_RIGHT)));
							ss.add(sub.Z().Cat(p_item->PVATSum, MKSFMTD(0, 2, NMBF_TRICOMMA|ALIGN_RIGHT)));
						}
						THROW_SL(p_list->addItem(i, ss.getBuf()));
					}
					p_list->Draw_();
				}
			}
			CATCHZOK
			return ok;
		}
	protected:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(P_V && event.isCmd(cmPrint)) {
				P_V->PrintTotal(&Total);
				clearEvent(event);
			}
		}
		GoodsTaxAnalyzeTotal Total;
		PPViewGoodsTaxAnalyze * P_V;
	};
	GoodsTaxAnalyzeTotal  total;
	if(CalcTotal(&total) > 0) {
		if(Filt.Flags & GoodsTaxAnalyzeFilt::fLedgerByLots) {
			TDialog * dlg = new TDialog(DLG_GTANLZTOTAL_PIL);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->setCtrlLong(CTL_GTANLZTOTAL_COUNT, total.Count);
				dlg->setCtrlReal(CTL_GTANLZTOTAL_RCPT,  total.PilRcptSum);
				dlg->setCtrlReal(CTL_GTANLZTOTAL_EXP,   total.PilExpSum);
				dlg->setCtrlReal(CTL_GTANLZTOTAL_REST,  total.PilRestSum);
				ExecViewAndDestroy(dlg);
			}
		}
		else {
			GTaxAnlzTotalDialog * dlg = new GTaxAnlzTotalDialog(&total, this);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->setDTS();
				ExecViewAndDestroy(dlg);
			}
		}
	}
}

int ViewGoodsTaxAnalyze(const GoodsTaxAnalyzeFilt * pFilt) { return PPView::Execute(PPVIEW_GOODSTAXANALYZE, pFilt, PPView::exefModeless, 0); }
//
// Implementation of PPALDD_GoodsTaxAnlz
//
PPALDD_CONSTRUCTOR(GoodsTaxAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsTaxAnlz) { Destroy(); }

// AHTOXA {
int SetInOutVAT(double * pVATRate, double * pInVATVal, double * pOutVATVal, double * pPTrnovr, double * pDscnt,
	const BVATAccmArray * pInOutList, uint i)
{
	double in_vat_val = 0.0, out_vat_val = 0.0, vat_rate = 0.0, trnovr = 0.0, dscnt = 0.0;
	if(pInOutList && i >= 0 && pInOutList->getCount() > i) {
		BVATAccm item = pInOutList->at(i);
		vat_rate    = item.PRate;
		if(pInOutList->at(i).IsVatFree || item.PRate == 0.0) {
			in_vat_val  = item.Cost;
			out_vat_val = item.Price;
		}
		else {
			in_vat_val  = item.CVATSum;
			out_vat_val = item.PVATSum;
		}
		trnovr = item.PTrnovr;
		dscnt  = item.Discount;
	}
	ASSIGN_PTR(pInVATVal,  in_vat_val);
	ASSIGN_PTR(pOutVATVal, out_vat_val);
	ASSIGN_PTR(pVATRate,   vat_rate);
	ASSIGN_PTR(pPTrnovr,   trnovr);
	ASSIGN_PTR(pDscnt,     dscnt);
	return 1;
}
// } ATHOXA

int PPALDD_GoodsTaxAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsTaxAnalyze, rsrv);
	const BVATAccmArray * p_inout_vat_list = p_v->GetInOutVATList();
	SString temp_buf;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltLocID   = p_filt->LocID;
	H.FltOpID    = p_filt->OpID;
	H.FltSupplID = p_filt->SupplID;
	H.FltBeg     = p_filt->Period.low;
	H.FltEnd     = p_filt->Period.upp;
	H.FltLotsBeg = p_filt->LotsPeriod.low;
	H.FltLotsEnd = p_filt->LotsPeriod.upp;
	H.IncomeMethod = (p_filt->Flags & GoodsTaxAnalyzeFilt::fByPayment) ? INCM_BYPAYMENT : INCM_BYSHIPMENT;
	PPFormatPeriod(&p_filt->Period, temp_buf).CopyTo(H.Prd, sizeof(H.Prd));
	PPFormatPeriod(&p_filt->LotsPeriod, temp_buf).CopyTo(H.LotsPrd, sizeof(H.LotsPrd));
	H.FltCycle   = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.FltFlags   = p_filt->Flags;
	H.fNozeroExciseOnly = BIN(p_filt->Flags & GoodsTaxAnalyzeFilt::fNozeroExciseOnly);
	H.fLabelOnly        = BIN(p_filt->Flags & GoodsTaxAnalyzeFilt::fLabelOnly);
	H.fByGroups = BIN(p_filt->Sgg & sggGroup);
	// AHTOXA {
	SetInOutVAT(&H.VATRate1, &H.VATSumIn1, &H.VATSumOut1, &H.PTrnovr1, &H.Discount1, p_inout_vat_list, 0);
	SetInOutVAT(&H.VATRate2, &H.VATSumIn2, &H.VATSumOut2, &H.PTrnovr2, &H.Discount2, p_inout_vat_list, 1);
	SetInOutVAT(&H.VATRate3, &H.VATSumIn3, &H.VATSumOut3, &H.PTrnovr3, &H.Discount3, p_inout_vat_list, 2);
	SetInOutVAT(&H.VATRate4, &H.VATSumIn4, &H.VATSumOut4, &H.PTrnovr4, &H.Discount4, p_inout_vat_list, 3);
	SetInOutVAT(&H.VATRate5, &H.VATSumIn5, &H.VATSumOut5, &H.PTrnovr5, &H.Discount5, p_inout_vat_list, 4);
	// } AHTOXA
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsTaxAnlz::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewGoodsTaxAnalyze * p_v = static_cast<PPViewGoodsTaxAnalyze *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewGoodsTaxAnalyze::IterOrder>(SortIdx));
	return 1;
}

int PPALDD_GoodsTaxAnlz::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		PPViewGoodsTaxAnalyze * p_v = static_cast<PPViewGoodsTaxAnalyze *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
		GoodsTaxAnalyzeViewItem item;
		if(p_v->NextIteration(&item) > 0) {
			I.Dt = item.Dt;
			p_v->FormatCycle(item.Dt, I.CycleText, sizeof(I.CycleText));
			I.GoodsID  = item.GoodsID;
			I.GoodsGrpID  = item.GoodsGrpID;
			I.GTaxGrpID   = item.GoodsTaxGrpID;
			I.LTaxGrpID   = item.LotTaxGrpID;
			STRNSCPY(I.BillNo, item.BillNo);
			STRNSCPY(I.Name, item.Name);
			STRNSCPY(I.GrpName, item.P_GoodsGrpName);
			STRNSCPY(I.TaxStr,  item.TaxStr);
			I.Qtty        = item.Qtty;
			I.PhQtty      = item.PhQtty;
			I.TrnovrCost  = item.TrnovrCost;
			I.TrnovrPrice = item.TrnovrPrice;
			I.Income      = item.Income;
			I.fIsVatFree  = BIN(item.TaxFlags & GGEF_VATFREE);
			I.fToggleSTax = BIN(item.TaxFlags & GGEF_TOGGLESTAX);
			I.ExciseSum  = item.ExciseSum;
			I.C_VATSum   = item.C_VATSum;
			I.C_STaxSum  = item.C_STaxSum;
			I.IncVATSum  = item.IncVATSum;
			I.VATSum     = item.VATSum;
			I.STaxSum    = item.STaxSum;
			I.ExpQtty    = item.ExpQtty;
			I.Rest       = item.Rest;
		}
		else
			return -1;
	}
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsTaxAnlz::Destroy() { DESTROY_PPVIEW_ALDD(GoodsTaxAnalyze); }
//
// Implementation of PPALDD_GTaxAnlzTotal
//
PPALDD_CONSTRUCTOR(GTaxAnlzTotal)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GTaxAnlzTotal)
{
	Destroy();
}

int PPALDD_GTaxAnlzTotal::InitData(PPFilt & rFilt, long rsrv)
{
	GTaxAnlzTotalPrintData * p_data = 0;
	if(rsrv) {
		p_data = (GTaxAnlzTotalPrintData *)rFilt.Ptr;
		Extra[1].Ptr = p_data;
	}
	else
		return 0;
	H.Count    = p_data->P_Total->Count;
	H.TrnovrCost       = p_data->P_Total->TrnovrCost;
	H.TrnovrPrice      = p_data->P_Total->TrnovrPrice;
	H.Income   = p_data->P_Total->Income;
	H.ExciseSum        = p_data->P_Total->ExciseSum;
	H.C_VATSum = p_data->P_Total->C_VATSum;
	H.VATSum   = p_data->P_Total->VATSum;
	H.IncVATSum        = p_data->P_Total->IncVATSum;
	H.STaxSum  = p_data->P_Total->STaxSum;

	H.FltBeg   = p_data->P_Filt->Period.low;
	H.FltEnd   = p_data->P_Filt->Period.upp;
	H.FltLotsBeg       = p_data->P_Filt->LotsPeriod.low;
	H.FltLotsEnd       = p_data->P_Filt->LotsPeriod.upp;
	H.FltLocID = p_data->P_Filt->LocID;
	H.FltOpID  = p_data->P_Filt->OpID;
	H.FltGoodsGrpID    = p_data->P_Filt->GoodsGrpID;
	H.FltSupplID       = p_data->P_Filt->SupplID;
	H.FltSupplAgentID  = p_data->P_Filt->SupplAgentID;
	H.IncomeMethod     = (p_data->P_Filt->Flags & GoodsTaxAnalyzeFilt::fByPayment) ? INCM_BYPAYMENT : INCM_BYSHIPMENT;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GTaxAnlzTotal::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.LineNo = 0;
	return 1;
}

int PPALDD_GTaxAnlzTotal::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	GTaxAnlzTotalPrintData * p_data = static_cast<GTaxAnlzTotalPrintData *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	BVATAccm * p_item;
	uint   n = static_cast<uint>(I.LineNo);
	if(p_data->P_VATList->enumItems(&n, (void **)&p_item) > 0) {
		I.LineNo     = n;
		I.VatRate    = p_item->PRate;
		if(p_item->IsVatFree || p_item->PRate == 0) {
			I.InVat  = p_item->Cost;
			I.OutVat = p_item->Price;
		}
		else {
			I.InVat  = p_item->CVATSum;
			I.OutVat = p_item->PVATSum;
		}
	}
	else if(n == 0) {
		I.LineNo  = 1;
		I.VatRate = 0;
		I.InVat   = p_data->P_Total->C_VATSum;
		I.OutVat  = p_data->P_Total->VATSum;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GTaxAnlzTotal::Destroy()
{
	Extra[1].Ptr = Extra[0].Ptr = 0;
}
