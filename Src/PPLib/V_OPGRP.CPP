// V_OPGRP.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

IMPLEMENT_PPFILT_FACTORY(OpGrouping); SLAPI OpGroupingFilt::OpGroupingFilt() : PPBaseFilt(PPFILT_OPGROUPING, 0, 1)
{
	SetFlatChunk(offsetof(OpGroupingFilt, ReserveStart),
		offsetof(OpGroupingFilt, LocList)-offsetof(OpGroupingFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(OpGroupingFilt, LocList));
	Init(1, 0);
}

OpGroupingFilt & FASTCALL OpGroupingFilt::operator = (const OpGroupingFilt & s)
{
	Copy(&s, 1);
	return *this;
}

int SLAPI OpGroupingFilt::IsEqualExcept(const OpGroupingFilt & rS, long flags) const
{
	if(flags & eqxCycleStat) {
		OpGroupingFilt temp1, temp2;
		temp1 = *this;
		temp2 = rS;
		temp1.CycleStat = 0;
		temp2.CycleStat = 0;
		return temp1.IsEqual(&temp2, 1);
	}
	else
		return IsEqual(&rS, 1);
}

SLAPI PPViewOpGrouping::OpGroupingStatEntry::OpGroupingStatEntry() : OpID(0), Sign(0), S(0)
{
}

SLAPI PPViewOpGrouping::PPViewOpGrouping() : PPView(0, &Filt, PPVIEW_OPGROUPING), P_TempTbl(0), P_TempStatTbl(0), P_ViewTrfrAnlz(0), P_ViewBill(0)
{
}

SLAPI PPViewOpGrouping::~PPViewOpGrouping()
{
	delete P_TempTbl;
	delete P_TempStatTbl;
	delete P_ViewTrfrAnlz;
	delete P_ViewBill;
	DBRemoveTempFiles();
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempOpGrpng);

enum {
	siQtty = 0,
	siPhQtty,
	siCount,
	siAvgLn,
	siAmount,
	siCost,
	siPrice,
	siDiscount,
	siExtCost,
	siExtPrice,
	siIncome,
	siNumItems
};

int SLAPI PPViewOpGrouping::Init_(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1, done = 0, create_stat = 0;
	SString temp_buf;
	PPLogger logger;
	TempOpGrpngTbl * p_tbl = 0;
	OpGroupingFilt prev_filt = Filt;
	THROW(Helper_InitBaseFilt(pBaseFilt));
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_ViewTrfrAnlz);
	ZDELETE(P_ViewBill);
	Filt.Period.Actualize(ZERODATE);
	Filt.LotsPeriod.Actualize(ZERODATE);
	Filt.ShipmentPeriod.Actualize(ZERODATE);
	CycleList.init2(&Filt.Period, &Filt.Cycl);
	//
	// Если заданы группа товара или товар, то колонки
	// по дополнительному типу товара получить нельзя.
	//
	if(Filt.GoodsGrpID || Filt.GoodsID)
		Filt.ExtGoodsTypeID = 0;
	THROW(AdjustPeriodToRights(Filt.Period, 0));
	if(P_TempTbl) {
		if(Filt.IsEqual(&prev_filt, 1))
			done = 1;
		else if(Filt.IsEqualExcept(prev_filt, OpGroupingFilt::eqxCycleStat)) {
			create_stat = 1;
			done = 1;
		}
	}
	if(!done) {
		ZDELETE(P_TempTbl);
		MEMSZERO(GdsOpTotal);
		MEMSZERO(LastOutRest);
		GdsOpTotal.Dt = plusdate(NZOR(Filt.Period.upp, Filt.Period.low), 1);
		GdsOpTotal.OpID = OPGRPNG_TOTAL_ID;
		if(!PPLoadText(PPTXT_GOODSOPRTOTAL, temp_buf))
			PPGetWord(PPWORD_TOTAL, 0, temp_buf);
		STRNSCPY(GdsOpTotal.OpName, temp_buf);
		THROW(p_tbl = CreateTempFile());
		{
			uint   cycle_no = 0;
			BExtInsert bei(p_tbl);
			TSVector <DateRange> period_list; // @v9.8.4 TSArray-->TSVector
			if(!Filt.Cycl)
				period_list.insert(&Filt.Period);
			else
				for(cycle_no = 0; cycle_no < CycleList.getCount(); cycle_no++)
					period_list.insert(&CycleList.at(cycle_no));
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(cycle_no = 0; cycle_no < period_list.getCount(); cycle_no++) {
				const DateRange & r_period = period_list.at(cycle_no);
				const  int accs_cost = BillObj->CheckRights(BILLRT_ACCSCOST);
				GoodsGrpngEntry * p_entry = 0;
				GoodsGrpngArray ary(&logger);
				GCTFilt gct_flt;
				gct_flt.Period     = r_period;
				gct_flt.LotsPeriod = Filt.LotsPeriod;
				gct_flt.ShipmentPeriod = Filt.ShipmentPeriod;
				gct_flt.OpID       = Filt.OpID;
				gct_flt.LocList    = Filt.LocList;
				gct_flt.SupplID    = Filt.SupplID;
				gct_flt.SupplAgentID = Filt.SupplAgentID;
				gct_flt.ArList.Add(Filt.ArID);
				gct_flt.GoodsGrpID = Filt.GoodsGrpID;
				gct_flt.GoodsID    = Filt.GoodsID;
				gct_flt.ExtGoodsTypeID = Filt.ExtGoodsTypeID;
				SETFLAG(gct_flt.Flags, OPG_LABELONLY,    Filt.Flags & OpGroupingFilt::fLabelOnly);
				SETFLAG(gct_flt.Flags, OPG_CALCBILLROWS, Filt.Flags & OpGroupingFilt::fCalcAvgLn);
				SETFLAG(gct_flt.Flags, OPG_COSTBYPAYM,   Filt.Flags & OpGroupingFilt::fCostByPaym);
				SETFLAG(gct_flt.Flags, OPG_INCLACCOPS,   Filt.Flags & OpGroupingFilt::fInclAccOps);
				if(Filt.Flags & OpGroupingFilt::fCalcRest) {
					if(cycle_no == 0)
						gct_flt.Flags |= (OPG_CALCINREST | OPG_CALCOUTREST);
					else {
						gct_flt.Flags &= ~OPG_CALCINREST;
						gct_flt.Flags |=  OPG_CALCOUTREST;
					}
				}
				gct_flt.Flags |= (OPG_PROCESSRECKONING | OPG_PROCESSGENOP);
				{
					AdjGdsGrpng agg;
					THROW(agg.BeginGoodsGroupingProcess(&gct_flt));
					THROW(ary.ProcessGoodsGrouping(&gct_flt, &agg));
					agg.EndGoodsGroupingProcess();
				}
				ary.InitOpNames();
				for(uint i = 0; ary.enumItems(&i, (void **)&p_entry);) {
					int    skip = 0;
					if(Filt.Flags & OpGroupingFilt::fSkipNUpdLotRestOps && CheckOpFlags(p_entry->OpID, OPKF_NOUPDLOTREST, 0))
						skip = 1;
					if(!skip) {
						int    sign = 0;
						TempOpGrpngTbl::Rec rec;
						// @v10.6.4 MEMSZERO(rec);
						rec.Dt   = r_period.low;
						rec.OpID = p_entry->OpID;
						STRNSCPY(rec.OpName, p_entry->OpName);
						rec.GoodsTaxGrpID = p_entry->GoodsTaxGrpID;
						rec.LotTaxGrpID   = p_entry->LotTaxGrpID;
						SETFLAG(rec.TaxFlags, GGEF_VATFREE,     p_entry->Flags & GGEF_VATFREE);
						SETFLAG(rec.TaxFlags, GGEF_LOCVATFREE,  p_entry->Flags & GGEF_LOCVATFREE);
						SETFLAG(rec.TaxFlags, GGEF_TOGGLESTAX,  p_entry->Flags & GGEF_TOGGLESTAX);
						SETFLAG(rec.TaxFlags, GGEF_RECKONING,   p_entry->Flags & GGEF_RECKONING);
						SETFLAG(rec.TaxFlags, GGEF_INTRREVERSE, p_entry->Flags & GGEF_INTRREVERSE);
						SETFLAG(rec.TaxFlags, GGEF_PAYMBYPAYOUTLOT, p_entry->Flags & GGEF_PAYMBYPAYOUTLOT);
						if(p_entry->Sign)
							rec.Sign = p_entry->Sign;
						else if(GetRecSign(&rec, &sign) > 0)
							rec.Sign = sign;
						rec.Qtty   = p_entry->Quantity;
						rec.PhQtty = p_entry->Volume;
						rec.Cost   = accs_cost ? p_entry->Cost : 0.0;
						rec.Price  = p_entry->Price;
						rec.Discount = p_entry->Discount;
						rec.Income = accs_cost ? p_entry->Income() : 0.0;
						rec.Amount = (accs_cost || p_entry->Amount != p_entry->Cost) ? p_entry->Amount : 0.0;
						rec.ExtCost = accs_cost ? p_entry->ExtCost : 0.0;
						rec.ExtPrice = p_entry->ExtPrice;
						rec.Count  = p_entry->Count;
						rec.LnCount = p_entry->LnCount;
						rec.AvgLn   = p_entry->AvgLn;
						if(!(p_entry->Flags & GGEF_RECKONING) || rec.Income != 0.0) {
							RecalcGdsOpTotal(&rec);
							THROW_DB(bei.insert(&rec));
						}
					}
				}
			}
			THROW_DB(bei.insert(&GdsOpTotal));
			THROW_DB(bei.flash());
			THROW(tra.Commit());
		}
		P_TempTbl = p_tbl;
		p_tbl = 0;
		create_stat = 1;
		done = 1;
	}
	if(create_stat) {
		ZDELETE(P_TempStatTbl);
		if(CycleList.getCount() && Filt.CycleStat) {
			uint   i, j, k;
			LAssocArray op_list;
			TSCollection <OpGroupingStatEntry> stat_list[siNumItems];
			TSVector <OpGroupingViewItem> item_list;
			THROW(CreateStatList(&op_list, &item_list, stat_list));
			THROW(P_TempStatTbl = CreateTempFile());
			{
				PPTransaction tra(ppDbDependTransaction, 1);
				THROW(tra);
				for(i = 0; i < siNumItems; i++)
					for(j = 0; j < stat_list[i].getCount(); j++)
						stat_list[i].at(j)->S.Finish();
				op_list.Sort();
				for(i = 0; i < op_list.getCount(); i++) {
					const PPID op_id = op_list.at(i).Key;
					const int sign = op_list.at(i).Val;
					TempOpGrpngTbl::Rec rec;
					// @v10.6.4 MEMSZERO(rec);
					OpGroupingViewItem item;
					MEMSZERO(item);
					rec.OpID = op_id;
					rec.Sign = sign;
					rec.Dt = Filt.Period.low;
					for(k = 0; k < item_list.getCount(); k++)
						if(item_list.at(k).OpID == op_id && item_list.at(k).Sign == sign) {
							STRNSCPY(rec.OpName, item_list.at(k).OpName);
							break;
						}
					rec.Qtty     = GetStatItem(0, op_id, sign, siQtty, stat_list);
					rec.PhQtty   = GetStatItem(0, op_id, sign, siPhQtty, stat_list);
					rec.Count    = R0i(GetStatItem(0, op_id, sign, siCount, stat_list));
					rec.AvgLn    = R0i(GetStatItem(0, op_id, sign, siAvgLn, stat_list));
					rec.Amount   = GetStatItem(0, op_id, sign, siAmount, stat_list);
					rec.Cost     = GetStatItem(0, op_id, sign, siCost, stat_list);
					rec.Price    = GetStatItem(0, op_id, sign, siPrice, stat_list);
					rec.Discount = GetStatItem(0, op_id, sign, siDiscount, stat_list);
					rec.ExtCost  = GetStatItem(0, op_id, sign, siExtCost, stat_list);
					rec.ExtPrice = GetStatItem(0, op_id, sign, siExtPrice, stat_list);
					rec.Income   = GetStatItem(0, op_id, sign, siIncome, stat_list);
					THROW_DB(P_TempStatTbl->insertRecBuf(&rec));
				}
				THROW(tra.Commit());
			}
		}
	}
	CATCH
		ZDELETE(P_TempTbl);
		ZDELETE(P_TempStatTbl);
		ok = 0;
	ENDCATCH
	delete p_tbl;
	logger.Save(PPFILNAM_ERR_LOG, 0);
	return ok;
}

int SLAPI PPViewOpGrouping::InitIteration()
{
	char   k[MAXKEYLEN];
	P_IterQuery = new BExtQuery(P_TempTbl, 0, 16);
	P_IterQuery->selectAll();
	memzero(k, sizeof(k));
	P_IterQuery->initIteration(0, k, spFirst);
	return 1;
}

int FASTCALL PPViewOpGrouping::NextIteration(OpGroupingViewItem * pItem)
{
	if(P_IterQuery && P_IterQuery->nextIteration() > 0 && P_TempTbl->data.OpID != OPGRPNG_TOTAL_ID) {
		TempOpGrpngTbl::Rec rec;
		P_TempTbl->copyBufTo(&rec);
		memzero(pItem, sizeof(*pItem));
		pItem->Dt   = rec.Dt;
		pItem->OpID = rec.OpID;
		STRNSCPY(pItem->OpName, rec.OpName);
		pItem->GoodsTaxGrpID = rec.GoodsTaxGrpID;
		pItem->LotTaxGrpID   = rec.LotTaxGrpID;
		pItem->Sign      = rec.Sign;
		pItem->Qtty      = rec.Qtty;
		pItem->PhQtty    = rec.PhQtty;
		pItem->Flags     = rec.TaxFlags;
		pItem->Count     = rec.Count;
		pItem->LnCount   = rec.LnCount;
		pItem->AvgLn     = rec.AvgLn;
		pItem->Amount    = rec.Amount;
		pItem->Cost      = rec.Cost;
		pItem->Price     = rec.Price;
		pItem->Discount  = rec.Discount;
		pItem->Income    = rec.Income;
		pItem->ExtCost   = rec.ExtCost;
		pItem->ExtPrice  = rec.ExtPrice;
		pItem->VatSum    = rec.VatSum;
		pItem->ExciseSum = rec.ExciseSum;
		pItem->STaxSum   = rec.STaxSum;
		return 1;
	}
	return -1;
}

int SLAPI PPViewOpGrouping::CalcStat(BillStatArray * pList)
{
	int    ok = -1;
	if(pList) {
		OpGroupingViewItem item;
		for(InitIteration(); NextIteration(&item) > 0;) {
			BillStatFunc bill_stat;
			bill_stat.OpID = item.OpID;
			bill_stat.Count = item.Count;
			bill_stat.LnCount = item.LnCount;
			bill_stat.AvgLines = item.AvgLn;
			pList->insert(&bill_stat);
		}
		ok = 1;
	}
	return ok;
}

void SLAPI PPViewOpGrouping::FormatCycle(LDATE dt, char * pBuf, size_t bufLen)
{
	Helper_FormatCycle(Filt.Cycl, CycleList, dt, pBuf, bufLen);
}

int SLAPI PPViewOpGrouping::GetRecSign(const TempOpGrpngTbl::Rec * pRec, int * pSign) const
{
	int    ok = -1;
	int    sign = 0;
	const  PPID op_id = pRec->OpID;
	if(op_id == -1)
		sign = 1;
	else if(op_id == 10000)
		sign = -1;
	else {
		int    op_type = GetOpType(op_id);
		if(op_type == PPOPT_GOODSMODIF)
			sign = NZOR(pRec->Sign, +1);
		else {
			int    intr = IsIntrOp(op_id);
			if(oneof4(op_type, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL) || intr) {
				sign = (IsExpendOp(op_id) > 0 || intr == INTREXPND) ? -1 : 1;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pSign, sign);
	return ok;
}

int SLAPI PPViewOpGrouping::RecalcGdsOpTotal(TempOpGrpngTbl::Rec * pRec)
{
	int    sign = 0;
	int    r = GetRecSign(pRec, &sign);
	pRec->TotalSign = static_cast<short>(sign);
	TempOpGrpngTbl::Rec rec = *pRec;
	//   Корректировка учета исходящих остатков в GdsOpTotal при использовании циклов в фильтре
	if(rec.OpID == 10000) {
		rec.Qtty     -= LastOutRest.Qtty;
		rec.PhQtty   -= LastOutRest.PhQtty;
		rec.Cost     -= LastOutRest.Cost;
		rec.Price    -= LastOutRest.Price;
		rec.Amount   -= LastOutRest.Amount;
		rec.ExtCost  -= LastOutRest.ExtCost;
		rec.ExtPrice -= LastOutRest.ExtPrice;
		rec.Income   -= LastOutRest.Income;
		LastOutRest   = *pRec;
	}
	if(GetOpType(rec.OpID) != PPOPT_GOODSREVAL) {
		GdsOpTotal.Qtty     += rec.Qtty   * sign;
		GdsOpTotal.PhQtty   += rec.PhQtty * sign;
	}
	GdsOpTotal.Cost     += rec.Cost * sign;
	GdsOpTotal.Price    += (rec.Price+rec.Discount) * sign;
	GdsOpTotal.Amount   += rec.Amount;
	GdsOpTotal.ExtCost  += rec.ExtCost  * sign;
	GdsOpTotal.ExtPrice += rec.ExtPrice * sign;
	if(rec.OpID != -1 && rec.OpID != 10000)
		GdsOpTotal.Count += rec.Count;
	GdsOpTotal.Income += rec.Income;
	return 1;
}

void SLAPI PPViewOpGrouping::GetGdsOpTotal(OpGroupingViewItem * pItem)
{
	memzero(pItem, sizeof(OpGroupingViewItem));
	pItem->Dt       = GdsOpTotal.Dt;
	pItem->OpID     = GdsOpTotal.OpID;
	STRNSCPY(pItem->OpName, GdsOpTotal.OpName);
	pItem->Qtty     = GdsOpTotal.Qtty;
	pItem->PhQtty   = GdsOpTotal.PhQtty;
	pItem->Count    = GdsOpTotal.Count;
	pItem->Amount   = GdsOpTotal.Amount;
	pItem->Cost     = GdsOpTotal.Cost;
	pItem->Price    = GdsOpTotal.Price;
	pItem->ExtCost  = GdsOpTotal.ExtCost;
	pItem->ExtPrice = GdsOpTotal.ExtPrice;
	pItem->Income   = GdsOpTotal.Income;
}

int SLAPI PPViewOpGrouping::AddStatItem(PPID opID, int sign, double val, uint si, TSCollection <OpGroupingStatEntry> * pList)
{
	int    ok = 1, found = 0;
	assert(si < siNumItems);
	for(uint i = 0; i < pList[si].getCount(); i++) {
		OpGroupingStatEntry * p_entry = pList[si].at(i);
		if(p_entry->OpID == opID && p_entry->Sign == sign) {
			p_entry->S.Step(val);
			found = 1;
		}
	}
	if(!found) {
		OpGroupingStatEntry * p_new_entry = pList[si].CreateNewItem();
		p_new_entry->OpID = opID;
		p_new_entry->Sign = sign;
		p_new_entry->S.Init(StatBase::fStoreVals);
		p_new_entry->S.Step(val);
	}
	return ok;
}

double SLAPI PPViewOpGrouping::GetStatItem(int stat, PPID opID, int sign, uint si, const TSCollection <OpGroupingStatEntry> * pList)
{
	int    ok = 1;
	assert(si < siNumItems);
	SETIFZ(stat, Filt.CycleStat);
	for(uint i = 0; i < pList[si].getCount(); i++) {
		const OpGroupingStatEntry * p_entry = pList[si].at(i);
		if(p_entry->OpID == opID && p_entry->Sign == sign) {
			if(stat == CYCLESTAT_AVERAGE) {
				const long c = p_entry->S.GetCount();
				return fdivnz(p_entry->S.GetSum(), p_entry->S.GetCount());
			}
			else if(stat == CYCLESTAT_MIN)
				return p_entry->S.GetMin();
			else if(stat == CYCLESTAT_MAX)
				return p_entry->S.GetMax();
			else if(oneof3(stat, CYCLESTAT_TREND, CYCLESTAT_TRENDBASE, CYCLESTAT_TRENDREL)) { // @v6.2.6 AHTOXA
				long count = p_entry->S.GetCount();
				LVect vect_x, vect_y;
				LssLin prcssr;
				vect_x.init(count, 0);
				vect_y.init(count, 0);
				for(long i = 0; i < count; i++) {
					double val = 0.0;
					p_entry->S.GetValue(i, &val);
					vect_x.set(i, i);
					vect_y.set(i, val);
				}
				prcssr.Solve(vect_x, vect_y);
				if(stat == CYCLESTAT_TREND)
					return round(prcssr.B, 3);
				else if(stat == CYCLESTAT_TRENDBASE)
					return round(prcssr.A, 3);
				else if(stat == CYCLESTAT_TRENDREL)
					return round(fdivnz(prcssr.B, prcssr.A), 6);
			}
			else
				return 0.0;
		}
	}
	return 0.0;
}

IMPL_CMPFUNC(OpGroupingViewItem, i1, i2) 
	{ RET_CMPCASCADE3(static_cast<const OpGroupingViewItem *>(i1), static_cast<const OpGroupingViewItem *>(i2), Dt, OpID, Sign); }

int SLAPI PPViewOpGrouping::CreateStatList(LAssocArray * pOpList, TSVector <OpGroupingViewItem> * pItemList, TSCollection <OpGroupingStatEntry> * pStatList) // @v9.8.4 TSArray-->TSVector
{
	int    ok = -1;
	if(CycleList.getCount()) {
		uint   i, j, k;
		PPIDArray date_list;
		OpGroupingViewItem item;
		LAssocArray proxi_op_list;
		TSVector <OpGroupingViewItem> proxi_item_list; // @v9.8.4 TSArray-->TSVector
		SETIFZ(pOpList, &proxi_op_list);
		SETIFZ(pItemList, &proxi_item_list);
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(!oneof2(item.OpID, OPGRPNG_TOTAL_ID, OPGRPNG_INREST)) {
				if(!pOpList->SearchPair(item.OpID, item.Sign, 0))
					pOpList->Add(item.OpID, item.Sign, 0);
				date_list.addUnique((long)item.Dt.v);
				THROW_SL(pItemList->insert(&item));
			}
		}
		for(i = 0; i < date_list.getCount(); i++) {
			LDATE dt;
			dt.v = date_list.at(i);
			for(j = 0; j < pOpList->getCount(); j++) {
				int found = 0;
				const LAssoc & r_op = pOpList->at(j);
				for(k = 0; !found && k < pItemList->getCount(); k++) {
					const OpGroupingViewItem & r_item = pItemList->at(k);
					if(r_item.Dt == dt && r_item.OpID == r_op.Key && r_item.Sign == r_op.Val)
						found = 1;
				}
				if(!found) {
					OpGroupingViewItem new_item;
					MEMSZERO(new_item);
					new_item.Dt = dt;
					new_item.OpID = r_op.Key;
					new_item.Sign = (short)r_op.Val;
					for(k = 0; k < pItemList->getCount(); k++)
						if(pItemList->at(k).OpID == new_item.OpID && pItemList->at(k).Sign == new_item.Sign) {
							STRNSCPY(new_item.OpName, pItemList->at(k).OpName);
							break;
						}
					THROW_SL(pItemList->insert(&new_item));
				}
			}
		}
		pItemList->sort(PTR_CMPFUNC(OpGroupingViewItem));
		if(pStatList) {
			for(i = 0; i < pItemList->getCount(); i++) {
				const OpGroupingViewItem & r_item = pItemList->at(i);
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Qtty,     siQtty, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.PhQtty,   siPhQtty, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Count,    siCount, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.AvgLn,    siAvgLn, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Amount,   siAmount, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Cost,     siCost, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Price,    siPrice, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Discount, siDiscount, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.ExtCost,  siExtCost, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.ExtPrice, siExtPrice, pStatList));
				THROW(AddStatItem(r_item.OpID, r_item.Sign, r_item.Income,   siIncome, pStatList));
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
#define GRP_GOODSFILT 1
#define GRP_CYCLE     2
#define GRP_LOC       3

class OpGroupingFiltDialog : public WLDialog {
	DECL_DIALOG_DATA(OpGroupingFilt);
public:
	OpGroupingFiltDialog() : WLDialog(DLG_OPGRPNGFLT, CTL_GTO_LABEL)
	{
		addGroup(GRP_GOODSFILT, new GoodsFiltCtrlGroup(CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, cmGoodsFilt));
		addGroup(GRP_CYCLE, new CycleCtrlGroup(CTLSEL_GTO_CYCLE, CTL_GTO_NUMCYCLES, CTL_GTO_PERIOD));
		SetupCalCtrl(CTLCAL_GTO_PERIOD, this, CTL_GTO_PERIOD, 1);
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_GTO_LOC, 0, 0, cmLocList, 0, 0, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		PPIDArray types;
		CycleCtrlGroup::Rec cycle_cg_rec;
		RVALUEPTR(Data, pData);
		LocationCtrlGroup::Rec l_rec(&Data.LocList);
		setGroupData(GRP_LOC, &l_rec);
		SetPeriodInput(this, CTL_GTO_PERIOD, &Data.Period);
		const PPID acs_id = GetSupplAccSheet();
		SetupArCombo(this, CTLSEL_GTO_SUPPL, Data.SupplID, 0, acs_id, sacfDisableIfZeroSheet);
		types.add(PPOPT_GENERIC);
		SetupOprKindCombo(this, CTLSEL_GTO_OPR, Data.OpID, 0, &types, 0);
		ReplyOprKindSelection();
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODSFILT, &gf_rec);
		SetupPPObjCombo(this, CTLSEL_GTO_EXTGOODSTYPE, PPOBJ_GOODSTYPE, Data.ExtGoodsTypeID, 0, 0);
		cycle_cg_rec.C = Data.Cycl;
		setGroupData(GRP_CYCLE, &cycle_cg_rec);
		SetupStringCombo(this, CTLSEL_GTO_CYCLESTAT, PPTXT_CYCLESTATLIST, Data.CycleStat);
		setWL(BIN(Data.Flags & OpGroupingFilt::fLabelOnly));
		AddClusterAssoc(CTL_GTO_FLAGS, 0, OpGroupingFilt::fCalcRest);
		AddClusterAssoc(CTL_GTO_FLAGS, 1, OpGroupingFilt::fCalcAvgLn);
		AddClusterAssoc(CTL_GTO_FLAGS, 2, OpGroupingFilt::fInclAccOps);
		SetClusterData(CTL_GTO_FLAGS, Data.Flags);
		SetupArCombo(this, CTLSEL_GTO_SUPPLAG, Data.SupplAgentID, OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet);
		disableCtrl(CTLSEL_GTO_EXTGOODSTYPE, Data.GoodsGrpID || Data.GoodsID);
		DisableClusterItem(CTL_GTO_FLAGS, 2, Data.GoodsGrpID || Data.GoodsID);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		GoodsFiltCtrlGroup::Rec gf_rec;
		CycleCtrlGroup::Rec cycle_cg_rec;
		LocationCtrlGroup::Rec l_rec;
		getGroupData(GRP_LOC, &l_rec);
		Data.LocList = l_rec.LocList;
		THROW(GetPeriodInput(this, CTL_GTO_PERIOD, &Data.Period));
		THROW(AdjustPeriodToRights(Data.Period, 1));
		getCtrlData(CTLSEL_GTO_SUPPL,  &Data.SupplID);
		getCtrlData(CTLSEL_GTO_OPR,    &Data.OpID);
		getCtrlData(CTLSEL_GTO_OBJECT, &Data.ArID);
		MEMSZERO(gf_rec);
		THROW(getGroupData(GRP_GOODSFILT, &gf_rec));
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		Data.GoodsID    = gf_rec.GoodsID;
		getCtrlData(CTLSEL_GTO_EXTGOODSTYPE, &Data.ExtGoodsTypeID);
		MEMSZERO(cycle_cg_rec);
		THROW(getGroupData(GRP_CYCLE, &cycle_cg_rec));
		Data.Cycl = cycle_cg_rec.C;
		Data.CycleStat = getCtrlLong(CTLSEL_GTO_CYCLESTAT);
		SETFLAG(Data.Flags, OpGroupingFilt::fLabelOnly, getWL());
		GetClusterData(CTL_GTO_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_GTO_SUPPLAG, &Data.SupplAgentID); // AHTOXA
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   ReplyOprKindSelection();
	int    EditAdvOptions();
};

void OpGroupingFiltDialog::ReplyOprKindSelection()
{
	PPID   acc_sheet_id = 0;
	getCtrlData(CTLSEL_GTO_OPR, &Data.OpID);
	DisableClusterItem(CTL_GTO_FLAGS, 2, Data.GoodsGrpID || Data.GoodsID || Data.OpID); // @v8.6.1
	GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
	SetupArCombo(this, CTLSEL_GTO_OBJECT, Data.ArID, 0, acc_sheet_id, sacfDisableIfZeroSheet);
	if(!acc_sheet_id && isCurrCtlID(CTL_GTO_OBJECT))
		selectNext();
}

int OpGroupingFiltDialog::EditAdvOptions()
{
	int    valid_data = 0, ok = -1;
	TDialog * p_dlg = new TDialog(DLG_OPGRPNGFLTA);
	THROW(CheckDialogPtr(&p_dlg));
	SetupCalCtrl(CTLCAL_OPGRPNGFLTA_PRD, p_dlg, CTL_OPGRPNGFLTA_PRD, 1);
	SetPeriodInput(p_dlg, CTL_OPGRPNGFLTA_PRD, &Data.ShipmentPeriod);
	while(!valid_data && ExecView(p_dlg) == cmOK) {
		if(GetPeriodInput(p_dlg, CTL_OPGRPNGFLTA_PRD, &Data.ShipmentPeriod) <= 0)
			PPError();
		else
			ok = valid_data = 1;
	}
	CATCHZOK
	delete p_dlg;
	return ok;
}

IMPL_HANDLE_EVENT(OpGroupingFiltDialog)
{
	WLDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_GTO_GGRP) || event.isCbSelected(CTLSEL_GTO_GOODS)) {
		GoodsFiltCtrlGroup::Rec gf_rec;
		getGroupData(GRP_GOODSFILT, &gf_rec);
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		Data.GoodsID    = gf_rec.GoodsID;
		disableCtrl(CTLSEL_GTO_EXTGOODSTYPE, Data.GoodsGrpID || Data.GoodsID);
		DisableClusterItem(CTL_GTO_FLAGS, 2, Data.GoodsGrpID || Data.GoodsID || Data.OpID); // @v8.6.1
	}
	else if(event.isCbSelected(CTLSEL_GTO_OPR))
		ReplyOprKindSelection();
	else if(event.isCmd(cmAdvOptions))
		EditAdvOptions();
	else
		return;
	clearEvent(event);
}

int SLAPI PPViewOpGrouping::EditBaseFilt(PPBaseFilt * pFilt)
{
	if(!Filt.IsA(pFilt))
		return 0;
	OpGroupingFilt * p_filt = static_cast<OpGroupingFilt *>(pFilt);
	DIALOG_PROC_BODY(OpGroupingFiltDialog, p_filt);
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	/*
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw && pData && pStyle) {
		const  DBQBrowserDef * p_def = (const DBQBrowserDef *)p_brw->getDef();
		const DBQuery * q = p_def ? (const DBQuery *)p_def->getQuery() : 0;
		if(q && col >= 0 && col < (long)p_def->getCount()) {
			const BroColumn & r_col = p_def->at(col);
			if(oneof2(r_col.OrgOffs, 6, 7)) { // Cost || Price
				uint fp = 0;
				if(q->getFieldPosByName("TotalSign", &fp)) {
					size_t offs = 0;
					for(uint i = 0; i < fp; i++)
						offs += stsize(q->flds[i].type);
					long total_sign = 0;
					stcast(q->flds[fp].type, T_INT32, PTR8(pData)+offs, &total_sign, 0);
					if(total_sign > 0) {
						pStyle->Color = LightenColor(GetColorRef(SClrGreen), 0.7f);
						pStyle->Flags = 0;
						ok = 1;
					}
					else {
						pStyle->Color = LightenColor(GetColorRef(SClrRed), 0.7f);
						pStyle->Flags = 0;
						ok = 1;
					}
				}
			}
		}
	}
	*/
	return ok;
}

void SLAPI PPViewOpGrouping::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(Filt.Flags & OpGroupingFilt::fCalcAvgLn) {
			pBrw->InsColumnWord(-1, PPWORD_LINES,    13, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0);
			pBrw->InsColumnWord(-1, PPWORD_AVGLINES, 14, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0);
		}
		pBrw->SetTempGoodsGrp(Filt.GoodsGrpID);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

DBQuery * SLAPI PPViewOpGrouping::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	uint   brw_id = 0;
	DBQuery * q = 0;
	TempOpGrpngTbl * p_tbl = 0;
	if(P_TempStatTbl) {
		if(Filt.GoodsGrpID || Filt.GoodsID || Filt.SupplAgentID)
			brw_id = BROWSER_OPGRPNG;
		else if(Filt.ExtGoodsTypeID)
			brw_id = BROWSER_BILLOPGRPNGEXT;
		else
			brw_id = BROWSER_BILLOPGRPNG;
		p_tbl = new TempOpGrpngTbl(P_TempStatTbl->GetName());
	}
	else {
		if(Filt.GoodsGrpID || Filt.GoodsID || Filt.SupplAgentID)
			brw_id = !Filt.Cycl ? BROWSER_OPGRPNG : BROWSER_OPGRPNGCYCLE;
		else if(Filt.ExtGoodsTypeID)
			brw_id = !Filt.Cycl ? BROWSER_BILLOPGRPNGEXT : BROWSER_BILLOPGRPNGCYCLEEXT;
		else
			brw_id = !Filt.Cycl ? BROWSER_BILLOPGRPNG : BROWSER_BILLOPGRPNGCYCLE;
		p_tbl = new TempOpGrpngTbl(P_TempTbl->GetName());
	}
	THROW(CheckTblPtr(p_tbl));
	q = & select(
		p_tbl->ID__,      // #0
		p_tbl->Dt,        // #1
		p_tbl->OpID,      // #2
		p_tbl->OpName,    // #3
		p_tbl->Qtty,      // #4
		p_tbl->PhQtty,    // #5
		p_tbl->Cost,      // #6
		p_tbl->Price,     // #7
		p_tbl->Income,    // #8
		p_tbl->Count,     // #9
		p_tbl->Amount,    // #10
		p_tbl->ExtCost,   // #11
		p_tbl->ExtPrice,  // #12
		p_tbl->LnCount,   // #13
		p_tbl->AvgLn,     // #14
		p_tbl->Discount,  // #15
		p_tbl->TotalSign, // #16
		0L).from(p_tbl, 0L); //turistti change
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		SString temp_buf;
		if(!Filt.Period.IsZero()) {
			pSubTitle->CatDivIfNotEmpty('-', 1).Cat(Filt.Period, 1);
		}
		pSubTitle->CatDivIfNotEmpty('-', 1).Cat(GetExtLocationName(Filt.LocList, 2, temp_buf));
		if(Filt.GoodsID) {
			pSubTitle->CatDivIfNotEmpty(':', 1).Cat(GetGoodsName(Filt.GoodsID, temp_buf));
		}
		else if(Filt.GoodsGrpID) {
			pSubTitle->CatDivIfNotEmpty(':', 1).Cat(GetGoodsName(Filt.GoodsGrpID, temp_buf));
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else
			delete p_tbl;
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewOpGrouping::Print(const void * pHdr)
{
	int    ok = 1;
	uint   rpt_id = 0, sel = 0;
	PPReportEnv env;
	if(!Filt.Cycl && !Filt.ExtGoodsTypeID)
		ok = SelectorDialog(DLG_SELOGPRN, CTL_SELOGPRN_SEL, &sel);
	if(ok > 0) {
		SETFLAG(Filt.Flags, OpGroupingFilt::fPrnBillList, sel);
		if(Filt.GoodsGrpID || Filt.GoodsID)
			rpt_id = sel ? REPORT_OPGRPNGBLIST : REPORT_OPGRPNG;
		else if(Filt.ExtGoodsTypeID)
			rpt_id = REPORT_BILLOPGRPNGEXT;
		else
			rpt_id = sel ? REPORT_BILLOPGRPNGBLIST : REPORT_BILLOPGRPNG;
		if(!sel && !Filt.Cycl)
			env.PrnFlags |= SReport::DisableGrouping;
		PView  pv(this);
		PPAlddPrint(rpt_id, &pv, &env);
	}
	return ok;
}

int SLAPI PPViewOpGrouping::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	PPID   __id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
	TempOpGrpngTbl * p_tbl = NZOR(P_TempStatTbl, P_TempTbl);
	if(__id && p_tbl) {
		PPID   k = __id;
		if(p_tbl->search(0, &k, spEq) && p_tbl->data.OpID != OPGRPNG_TOTAL_ID) {
			DateRange detail_period;
			if(!CycleList.getCount() || P_TempStatTbl)
				detail_period = Filt.Period;
			else if(CycleList.searchPeriodByDate(p_tbl->data.Dt, &detail_period) <= 0)
				return -1;
			if(oneof2(p_tbl->data.OpID, OPGRPNG_INREST, OPGRPNG_OUTREST)) {
				if(Filt.GoodsID) {
					LotFilt  lot_flt;
					lot_flt.LocID      = Filt.LocList.GetSingle();
					lot_flt.SupplID    = Filt.SupplID;
					lot_flt.GoodsGrpID = Filt.GoodsGrpID;
					lot_flt.GoodsID    = Filt.GoodsID;
					ViewLots(&lot_flt, 0, 1);
				}
				else {
					GoodsRestFilt  gr_flt;
					gr_flt.Date = (p_tbl->data.OpID == OPGRPNG_INREST) ?
						(detail_period.low ? plusdate(detail_period.low, -1) : encodedate(1, 1, 1900)) : detail_period.upp;
					gr_flt.LocList     = Filt.LocList;
					gr_flt.SupplID     = Filt.SupplID;
					gr_flt.GoodsGrpID  = Filt.GoodsGrpID;
					gr_flt.AgentID     = Filt.SupplAgentID;
					ViewGoodsRest(&gr_flt, 0);
				}
			}
			else if(!Filt.GoodsGrpID && !Filt.GoodsID && !Filt.SupplAgentID) {
				BillFilt bill_flt;
				bill_flt.Period   = detail_period;
				bill_flt.LocList  = Filt.LocList;
				bill_flt.ObjectID = p_tbl->data.ObjectID;
				bill_flt.OpID     = p_tbl->data.OpID;
				if(bill_flt.OpID == PPOPK_INTRRECEIPT) {
					bill_flt.LocList.Set(0);
					bill_flt.ObjectID = PPObjLocation::WarehouseToObj(Filt.LocList.GetSingle());
				}
				SETFLAG(bill_flt.Flags, BillFilt::fLabelOnly, Filt.Flags & OpGroupingFilt::fLabelOnly);
				bill_flt.Flags |= BillFilt::fAllCurrencies;
				ViewGoodsBills(&bill_flt, 1);
				return 1;
			}
			else /*if(Filt.GoodsID)*/ {
				TrfrAnlzFilt tra_flt;
				tra_flt.Period     = detail_period;
				tra_flt.LocList    = Filt.LocList;
				tra_flt.OpID       = p_tbl->data.OpID;
				tra_flt.SupplID    = Filt.SupplID;
				tra_flt.GoodsGrpID = Filt.GoodsGrpID;
				tra_flt.GoodsID    = Filt.GoodsID;
				tra_flt.SupplAgentID = Filt.SupplAgentID;
				if(tra_flt.OpID == PPOPK_INTRRECEIPT) {
					tra_flt.LocList.FreeAll();
					tra_flt.ArList.Add(PPObjLocation::WarehouseToObj(Filt.LocList.GetSingle()));
					PPIDArray op_list;
					for(PPID op_id = 0; EnumOperations(0L, &op_id, 0) > 0;) {
						const int dir = IsIntrOp(op_id);
						if(oneof2(dir, INTRRCPT, INTREXPND))
							op_list.add(op_id);
					}
					tra_flt.OpID = op_list.getSingle();
				}
				SETFLAG(tra_flt.Flags, TrfrAnlzFilt::fLabelOnly, Filt.Flags & OpGroupingFilt::fLabelOnly);
				ViewTrfrAnlz(&tra_flt);
			}
		}
	}
	return -1;
}

int SLAPI PPViewOpGrouping::InitBillList(PPID opID)
{
	int    ok = 1;
	if(Filt.GoodsGrpID || Filt.GoodsID) {
		TrfrAnlzFilt  tra_flt;
		tra_flt.Period     = Filt.Period;
		tra_flt.LocList    = Filt.LocList;
		tra_flt.OpID       = opID;
		tra_flt.GoodsGrpID = Filt.GoodsGrpID;
		tra_flt.GoodsID = Filt.GoodsID;
		if(tra_flt.OpID == PPOPK_INTRRECEIPT) {
			tra_flt.LocList.FreeAll();
			tra_flt.ArList.Add(PPObjLocation::WarehouseToObj(Filt.LocList.GetSingle()));
			PPIDArray op_list;
			for(PPID op_id = 0; EnumOperations(0L, &op_id, 0) > 0;) {
				const int dir = IsIntrOp(op_id);
				if(oneof2(dir, INTRRCPT, INTREXPND))
					op_list.add(op_id);
			}
			tra_flt.OpID = op_list.getSingle();
		}
		SETFLAG(tra_flt.Flags, TrfrAnlzFilt::fLabelOnly, Filt.Flags & OpGroupingFilt::fLabelOnly);
		ZDELETE(P_ViewTrfrAnlz);
		THROW_MEM(P_ViewTrfrAnlz = new PPViewTrfrAnlz);
		THROW(P_ViewTrfrAnlz->Init_(&tra_flt));
		PPWait(1);
		THROW(P_ViewTrfrAnlz->InitIteration(PPViewTrfrAnlz::OrdByDate));
	}
	else {
		BillFilt  bill_flt;
		bill_flt.Period   = Filt.Period;
		bill_flt.LocList  = Filt.LocList;
		bill_flt.ObjectID = Filt.ArID;
		bill_flt.OpID     = opID;
		if(bill_flt.OpID == PPOPK_INTRRECEIPT) {
			bill_flt.LocList.Set(0);
			bill_flt.ObjectID = PPObjLocation::WarehouseToObj(Filt.LocList.GetSingle());
		}
		SETFLAG(bill_flt.Flags, BillFilt::fLabelOnly, Filt.Flags & OpGroupingFilt::fLabelOnly);
		SETFLAG(bill_flt.Flags, BillFilt::fAllCurrencies, 1);
		ZDELETE(P_ViewBill);
		THROW_MEM(P_ViewBill = new PPViewBill);
		THROW(P_ViewBill->Init_(&bill_flt));
		THROW(P_ViewBill->InitIteration(PPViewBill::OrdByDate));
	}
	CATCH
		ZDELETE(P_ViewTrfrAnlz);
		ZDELETE(P_ViewBill);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewOpGrouping::EnumBillList(uint * pPos, OpGroupingViewItem * pItem)
{
	int    ok = -1;
	uint   pos = DEREFPTRORZ(pPos);
	OpGroupingViewItem  item;
	MEMSZERO(item);
	if(Filt.GoodsGrpID || Filt.GoodsID) {
		TrfrAnlzViewItem  trf_item;
		if(P_ViewTrfrAnlz && P_ViewTrfrAnlz->NextIteration(&trf_item) > 0) {
			item.Dt            = trf_item.Dt;
			item.GoodsTaxGrpID = trf_item.GoodsID;
			item.LotTaxGrpID   = trf_item.BillID;
			item.Qtty          = fabs(trf_item.Qtty);
			item.PhQtty        = fabs(trf_item.PhQtty);
			item.Cost          = fabs(trf_item.Cost);
			item.Price         = fabs(trf_item.Price);
			item.Discount      = fabs(trf_item.Discount);
			ok = 1;
		}
	}
	else {
		BillViewItem  bill_item;
		if(P_ViewBill && P_ViewBill->NextIteration(&bill_item) > 0) {
			AmtList   amt_list;
			item.Dt            = bill_item.Dt;
			item.LotTaxGrpID   = bill_item.ID;
			item.Amount        = BR2(bill_item.Amount);
			if(bill_item.ID) {
				BillObj->P_Tbl->GetAmountList(bill_item.ID, &amt_list);
				item.Cost      = amt_list.Get(PPAMT_BUYING, 0L);
				item.Discount  = amt_list.Get(PPAMT_DISCOUNT, 0L);
				item.Price     = amt_list.Get(PPAMT_SELLING, 0L) - item.Discount;
				item.VatSum    = amt_list.Get(PPAMT_VATAX, 0L);
				//
				BillTotalData btd;
				P_ViewBill->CalcItemTotal(bill_item.ID, &btd);
				item.LnCount   = btd.LinesCount;
				item.Qtty      = fabs(btd.UnitsCount);
				item.PhQtty    = fabs(btd.PhUnitsCount);
			}
			ok = 1;
		}
	}
	if(ok > 0)
		pos++;
	else
		pos = 0;
	ASSIGN_PTR(pPos, pos);
	ASSIGN_PTR(pItem, item);
	return ok;
}

int SLAPI PPViewOpGrouping::ViewGraph(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	const  int col = pBrw ? pBrw->GetCurColumn() : -1;
	SString line_text, temp_buf, op_name;
	Generator_GnuPlot plot(0);
	Generator_GnuPlot::PlotParam param;
	if(CycleList.getCount()) {
		uint   i;
		PPID   __id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		PPID   cur_op_id = 0;
		long   cur_sign = 0;
		TempOpGrpngTbl * p_tbl = NZOR(P_TempStatTbl, P_TempTbl);
		if(__id && p_tbl) {
			PPID   k = __id;
			if(p_tbl->search(0, &k, spEq) && p_tbl->data.OpID != OPGRPNG_TOTAL_ID) {
				cur_op_id = p_tbl->data.OpID;
				cur_sign = p_tbl->data.Sign;
				op_name = p_tbl->data.OpName;
			}
		}

		plot.Preamble();

		int    si = siAmount;
		if(col >= 0) {
			const int offs = pBrw->getDef()->at(col).OrgOffs;
			switch(offs) {
				case  4:
					si = siQtty;
					PPLoadString("qtty", line_text);
					break;
				case  5:
					si = siPhQtty;
					PPLoadString("phqtty", line_text);
					break;
				case  6:
					si = siCost;
					PPLoadString("sumcost", line_text);
					break;
				case  7:
					si = siPrice;
					PPLoadString("sumprice", line_text);
					break;
				case  8:
					si = siIncome;
					PPLoadString("profitableness", line_text);
					break;
				case  9:
					si = siCount;
					PPLoadString("linecount", line_text); // @v9.3.1
					// @v9.3.1 (line_text = "Количество строк").ToOem();
					break;
				case 10:
					si = siAmount;
					PPLoadString("nominalamount", line_text);
					break;
				case 11:
					si = siExtCost;
					PPLoadString("sumcost", line_text);
					line_text.Space().CatParStr("extra");
					break;
				case 12:
					si = siExtPrice;
					PPLoadString("sumprice", line_text);
					line_text.Space().CatParStr("extra");
					break;
				case 13:
					si = siAvgLn;
					PPGetWord(PPWORD_LINES, 0, line_text);
					break;
				case 14:
					si = siAvgLn;
					PPGetWord(PPWORD_AVGLINES, 0, line_text);
					break;
				case 15:
					si = siDiscount;
					PPLoadString("discount", line_text);
					break;
				default:
					si = siAmount;
					PPLoadString("nominalamount", line_text);
					break;
			}
		}
		size_t p = 0;
		{
			PPGpPlotItem item(plot.GetDataFileName(), line_text.Transf(CTRANSF_INNER_TO_OUTER), PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrBlue), 3);
			item.AddDataIndex(1);
			item.AddDataIndex(2);
			plot.AddPlotItem(item);
		}
		{
			PPGetSubStr(PPTXT_CYCLESTATLIST, CYCLESTAT_MIN-1, temp_buf);
			if(temp_buf.SearchChar(',', &(p = 0)))
				temp_buf.Excise(0, p+1);
			PPGpPlotItem item(0, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrGreen), 0.5f);
			item.AddDataIndex(1);
			item.AddDataIndex(3);
			plot.AddPlotItem(item);
		}
		{
			PPGetSubStr(PPTXT_CYCLESTATLIST, CYCLESTAT_MAX-1, temp_buf);
			if(temp_buf.SearchChar(',', &(p = 0)))
				temp_buf.Excise(0, p+1);
			PPGpPlotItem item(0, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrGreen), 0.5f);
			item.AddDataIndex(1);
			item.AddDataIndex(4);
			plot.AddPlotItem(item);
		}
		{
			PPGetSubStr(PPTXT_CYCLESTATLIST, CYCLESTAT_AVERAGE-1, temp_buf);
			if(temp_buf.SearchChar(',', &(p = 0)))
				temp_buf.Excise(0, p+1);
			PPGpPlotItem item(0, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrGreen), 0.5f);
			item.AddDataIndex(1);
			item.AddDataIndex(5);
			plot.AddPlotItem(item);
		}
		{
			PPGetSubStr(PPTXT_CYCLESTATLIST, CYCLESTAT_TREND-1, temp_buf);
			if(temp_buf.SearchChar(',', &(p = 0)))
				temp_buf.Excise(0, p+1);
			PPGpPlotItem item(0, temp_buf.Transf(CTRANSF_INNER_TO_OUTER), PPGpPlotItem::sLines);
			item.Style.SetLine(GetColorRef(SClrRed), 1);
			item.AddDataIndex(1);
			item.AddDataIndex(6);
			plot.AddPlotItem(item);
		}

		(temp_buf = pBrw->getTitle()).Transf(CTRANSF_INNER_TO_OUTER);
		if(op_name.NotEmptyS())
			temp_buf.Space().CatParStr(op_name.Transf(CTRANSF_INNER_TO_OUTER));
		plot.SetTitle(temp_buf);

		plot.SetDateTimeFormat(Generator_GnuPlot::axX, 0);
		plot.SetGrid();
		plot.Plot(&param);

		LAssocArray op_list;
		TSCollection <OpGroupingStatEntry> stat_list[siNumItems];
		TSVector <OpGroupingViewItem> item_list; // @v9.8.4 TSArray-->TSVector
		THROW(CreateStatList(&op_list, &item_list, stat_list));

		plot.StartData(1);

		double _min = GetStatItem(CYCLESTAT_MIN, cur_op_id, cur_sign, si, stat_list);
		double _max = GetStatItem(CYCLESTAT_MAX, cur_op_id, cur_sign, si, stat_list);
		double _avg = GetStatItem(CYCLESTAT_AVERAGE, cur_op_id, cur_sign, si, stat_list);
		double _trend = GetStatItem(CYCLESTAT_TREND, cur_op_id, cur_sign, si, stat_list);
		double _trendbase = GetStatItem(CYCLESTAT_TRENDBASE, cur_op_id, cur_sign, si, stat_list);
		int    s = 0;
		for(i = 0; i < item_list.getCount(); i++) {
			const OpGroupingViewItem & r_item = item_list.at(i);
			if(r_item.OpID == cur_op_id && r_item.Sign == cur_sign) {
				double val = 0.0;
				switch(si) {
					case siQtty: val = r_item.Qtty; break;
					case siPhQtty: val = r_item.PhQtty; break;
					case siCost: val = r_item.Cost; break;
					case siPrice: val = r_item.Price; break;
					case siDiscount: val = r_item.Discount; break;
					case siIncome: val = r_item.Income; break;
					case siCount: val = r_item.Count; break;
					case siAmount: val = r_item.Amount; break;
					case siExtCost: val = r_item.ExtCost; break;
					case siExtPrice: val = r_item.ExtPrice; break;
					case siAvgLn: val = r_item.AvgLn; break;
				}
				plot.PutData(r_item.Dt); // #1
				plot.PutData(val);       // #2
				plot.PutData(_min);      // #3
				plot.PutData(_max);      // #4
				plot.PutData(_avg);      // #5
				plot.PutData(_trend * s + _trendbase); // #6
				s++;

				plot.PutEOR();
			}
		}
		plot.PutEndOfData();
		ok = plot.Run();
	}
	CATCHZOK
	return ok;
}

// virtual
int SLAPI PPViewOpGrouping::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -2;
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_GRAPH:
				ok = ViewGraph(pHdr, pBrw);
				break;
		}
	}
	return ok;
}

int SLAPI ViewOpGrouping(const OpGroupingFilt * pFilt) { return PPView::Execute(PPVIEW_OPGROUPING, pFilt, 1, 0); }
//
// Implementation of PPALDD_OpGrouping
//
PPALDD_CONSTRUCTOR(OpGrouping)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(OpGrouping) { Destroy(); }

int PPALDD_OpGrouping::InitData(PPFilt & rFilt, long rsrv)
{
	OpGroupingViewItem  total;
	INIT_PPVIEW_ALDD_DATA_U(OpGrouping, rsrv);
	H.FltOpID    = p_filt->OpID;
	H.FltLocID   = p_filt->LocList.GetSingle();
	H.FltSupplID = p_filt->SupplID;
	H.FltObjectID   = p_filt->ArID;
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltGoodsID    = p_filt->GoodsID;
	H.ExtGoodsTypeID = p_filt->ExtGoodsTypeID;
	H.FltBeg       = p_filt->Period.low;
	H.FltEnd       = p_filt->Period.upp;
	H.FltLotBeg    = p_filt->LotsPeriod.low;
	H.FltLotEnd    = p_filt->LotsPeriod.upp;
	H.FltCycle     = p_filt->Cycl.Cycle;
	H.FltNumCycles = p_filt->Cycl.NumCycles;
	H.Flags        = p_filt->Flags;
	H.fWlOnly      = BIN(p_filt->Flags & OpGroupingFilt::fLabelOnly);
	p_v->GetGdsOpTotal(&total);
	H.TotalQtty    = total.Qtty;
	H.TotalCost    = total.Cost;
	H.TotalPhQtty  = total.PhQtty;
	H.TotalPrice   = total.Price;
	H.TotalAmount  = total.Amount;
	H.TotalExtCost = total.ExtCost;
	H.TotalExtPrice = total.ExtPrice;
	H.TotalCount   = total.Count;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_OpGrouping::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	I.RecNo = 0;
	I.GrpNo = 0;
	INIT_PPVIEW_ALDD_ITER(OpGrouping);
}

int PPALDD_OpGrouping::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	PPViewOpGrouping * p_v = static_cast<PPViewOpGrouping *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	OpGroupingViewItem item;
	while(I.RecNo || p_v->NextIteration(&item) > 0) {
		const OpGroupingFilt * p_filt = (const OpGroupingFilt *)p_v->GetBaseFilt();
		if(!I.RecNo) {
			I.GrpNo++;
			I.Dt = item.Dt;
			p_v->FormatCycle(item.Dt, I.CycleText, sizeof(I.CycleText));
			I.ObjectID = item.ObjectID;
			I.OpID = item.OpID;
			I.GoodsTaxGrpID = item.GoodsTaxGrpID;
			I.LotTaxGrpID = item.LotTaxGrpID;
			if(oneof2(item.OpID, -1, 10000) && p_filt->Flags & OpGroupingFilt::fPrnBillList)
				I.OpName[0] = 0;
			else
				STRNSCPY(I.OpName, item.OpName);
			I.Sign     = item.Sign;
			I.Qtty     = item.Qtty;
			I.PhQtty   = item.PhQtty;
			I.SumCost  = item.Cost;
			I.SumPrice = item.Price;
			I.SumDiscount = item.Discount;
			I.Income   = item.Income;
			I.Amount   = item.Amount;
			I.ExtCost  = item.ExtCost;
			I.ExtPrice = item.ExtPrice;
			I.Count    = item.Count;
			I.SumVat      = item.VatSum;
			I.SumExcise   = item.ExciseSum;
			I.SumSalesTax = item.STaxSum;
		}
		if(!oneof2(I.OpID, -1, 10000) && p_filt->Flags & OpGroupingFilt::fPrnBillList) {
			if(!I.RecNo) {
				if(item.Flags & GGEF_INTRREVERSE)
					continue;
				else
					p_v->InitBillList(I.OpID);
			}
			p_v->EnumBillList((uint *)&I.RecNo, &item);
			I.BillID       = item.LotTaxGrpID;
			I.GoodsID      = item.GoodsTaxGrpID;
			I.BillDt       = item.Dt;
			I.BillQtty     = item.Qtty;
			I.BillPhQtty   = item.PhQtty;
			I.BillAmount   = item.Amount;
			I.BillCostSum  = item.Cost;
			I.BillPriceSum = item.Price;
			I.BillDscntSum = item.Discount;
			I.SumVat      = item.VatSum;
			I.SumExcise   = item.ExciseSum;
			I.SumSalesTax = item.STaxSum;
		}
		else {
			I.BillID       = 0;
			I.GoodsID      = 0;
			I.BillDt       = ZERODATE;
			I.BillQtty     = 0;
			I.BillPhQtty   = 0;
			I.BillAmount   = 0;
			I.BillCostSum  = 0;
			I.BillPriceSum = 0;
			I.BillDscntSum = 0;
		}
		return DlRtm::NextIteration(iterId);
	}
	return -1;
}

void PPALDD_OpGrouping::Destroy()
{
	DESTROY_PPVIEW_ALDD(OpGrouping);
}

