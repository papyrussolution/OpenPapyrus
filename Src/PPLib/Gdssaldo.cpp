// GDSSALDO.CPP
// Copyright (c) V.Nasonov 2003, 2005, 2007, 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2019, 2020, 2021, 2024, 2025
// @codepage UTF-8
// Расчет сальдо по товарам
//
#include <pp.h>
#pragma hdrstop
//
// GoodsSaldoCore
//
GoodsSaldoCore::GoodsSaldoCore() : GoodsDebtTbl()
{
}

int GoodsSaldoCore::GetLastSaldo(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE * pDt, GoodsDebtTbl::Rec * pRec)
{
	int    ok = -1;
	LDATE  dt = ZERODATE;
	GoodsDebtTbl::Rec gd_rec;
	if(goodsID) {
		const LDATE base_date = (pDt == 0 || *pDt == ZERODATE) ? MAXDATE : *pDt;
		GoodsDebtTbl::Key0 k0;
		MEMSZERO(k0);
		k0.GoodsID = goodsID;
		k0.ArID    = arID;
		k0.DlvrLocID = dlvrLocID;
		k0.Dt      = base_date;
		if(search(0, &k0, spLt) && k0.GoodsID == goodsID && k0.ArID == arID && k0.DlvrLocID == dlvrLocID) {
			CopyBufTo(&gd_rec);
			dt = gd_rec.Dt;
			ok = 1;
		}
		else {
			gd_rec.GoodsID = goodsID;
			gd_rec.ArID    = arID;
			gd_rec.DlvrLocID = dlvrLocID;
		}
		//
		{
			if(arID) {
				LDATE   last_calc_date = ZERODATE;
				if(GetLastSaldo(goodsID, 0, 0, &last_calc_date, 0) > 0) {
					dt = last_calc_date;
					ok = 2;
				}
			}
		}
	}
	else
		ok = PPSetErrorInvParam();
	ASSIGN_PTR(pRec, gd_rec);
	ASSIGN_PTR(pDt, dt);
	return ok;
}

int GoodsSaldoCore::GetLastCalcDate(PPID goodsGrpID, PPID goodsID, PPID arID, PPID dlvrLocID, LDATE * pDt)
{
	int    ok = 1;
	LDATE  dt = ZERODATE;
	PPIDArray temp_goods_list;
	ObjIdListFilt goods_list;
	union {
		GoodsDebtTbl::Key0 k0;
		GoodsDebtTbl::Key1 k1;
	} k;
	int    idx  = 0;
	MEMSZERO(k);
	if(goodsGrpID || goodsID || arID) {
		if(goodsID) {
			temp_goods_list.add(goodsID);
			goods_list.Set(&temp_goods_list);
		}
		else if(goodsGrpID) {
			THROW(GoodsIterator::GetListByGroup(goodsGrpID, &temp_goods_list));
			temp_goods_list.sort();
			goods_list.Set(&temp_goods_list);
		}
		if(arID && !goodsID) {
			k.k1.ArID      = arID;
			k.k1.DlvrLocID = dlvrLocID;
			k.k1.GoodsID   = MAXLONG;
			k.k1.Dt        = MAXDATE;
			idx = 1;
		}
		else {
			k.k0.GoodsID = NZOR(goodsID, MAXLONG);
			k.k0.ArID    = NZOR(arID, MAXLONG);
			k.k0.DlvrLocID = dlvrLocID;
			k.k0.Dt = (goodsID && arID) ? MAXDATE : ZERODATE;
			idx = 0;
		}
		while(search(idx, &k, spLt)) {
			if(goods_list.CheckID(data.GoodsID) && (!arID || data.ArID == arID))
				SETMAX(dt, data.Dt);
		}
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pDt, dt);
	return ok;
}
//
//   CalcSaldoList
//
struct CalcSaldoEntry { // @flat
	CalcSaldoEntry() : GoodsID(0), ArticleID(0), Dt(ZERODATE)
	{
	}
	PPID   GoodsID;
	PPID   ArticleID;
	LDATE  Dt;
};

class CalcSaldoList : public SVector {
public:
	CalcSaldoList() : SVector(sizeof(CalcSaldoEntry))
	{
	}
	int   Search(PPID gdsID, PPID artID, uint * p = 0);
	int   Insert(const CalcSaldoEntry * pEntry, uint * p = 0);
	CalcSaldoEntry & FASTCALL at(uint p) const
	{
		return *static_cast<CalcSaldoEntry *>(SVector::at(p));
	}
};

IMPL_CMPFUNC(CalcSaldoEnKey, i1, i2)
	{ RET_CMPCASCADE2(static_cast<const CalcSaldoEntry *>(i1), static_cast<const CalcSaldoEntry *>(i2), GoodsID, ArticleID); }

int CalcSaldoList::Search(PPID gdsID, PPID artID, uint * p)
{
	CalcSaldoEntry dse;
	dse.GoodsID   = gdsID;
	dse.ArticleID = artID;
	return bsearch(&dse, p, PTR_CMPFUNC(CalcSaldoEnKey));
}

int CalcSaldoList::Insert(const CalcSaldoEntry * pEntry, uint * p)
{
	return ordInsert(pEntry, p, PTR_CMPFUNC(CalcSaldoEnKey)) ? 1 : PPSetErrorSLib();
}
//
// PrcssrGoodsSaldo
//
#define DEF_SALDO_PERIOD  7L

struct GArSEntry { // @flat
	PPID   GoodsID;
	PPID   ArID;
	PPID   DlvrLocID;
	LDATE  Dt;
	double Qtty;
	double Amt;
};

struct GArSLastEntry {
	PPID   GoodsID;
	PPID   ArID;
	PPID   DlvrLocID;
	uint   Pos;
};

class PrcssrGoodsSaldo {
public:
	struct Param {
		Param() : GoodsGrpID(0), GoodsID(0), ArID(0), Dt(ZERODATE), CalcPeriod(0), FullCalc(0)
		{
		}
		PPID  GoodsGrpID;
		PPID  GoodsID;
		PPID  ArID;
		LDATE Dt;
		int   CalcPeriod; // sgdXXX
		int   FullCalc;
	};

	PrcssrGoodsSaldo()
	{
	}
	int    InitParam(Param *);
	int    EditParam(Param *);
	int    Init(const Param *);
	int    Run();
	int    Test(PPID goodsID, PPID arID, PPID dlvrID, const DateRange * pPeriod);
private:
	int    SetUnworkingContragentsSaldo(LDATE dt, CalcSaldoList * pList);
	int    SetupItem(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, double qtty, double amt, TSVector <GArSEntry> & rList);

	GoodsSaldoCore GSCore;
	Param  Par;
};

int PrcssrGoodsSaldo::InitParam(Param * pPar)
{
	if(pPar) {
		memzero(pPar, sizeof(*pPar));
		PPObjGoods goods_obj;
		pPar->GoodsGrpID = goods_obj.GetConfig().TareGrpID;
	}
	return 1;
}
//
// Implementation of GoodsSaldoParamDlg
//
int PrcssrGoodsSaldo::EditParam(Param * pPar)
{
	class GoodsSaldoParamDlg : public TDialog {
		DECL_DIALOG_DATA(PrcssrGoodsSaldo::Param);
		enum {
			ctlgroupGoods = 1
		};
	public:
		GoodsSaldoParamDlg() : TDialog(DLG_GDSSALDO)
		{
			addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_GDSSALDO_GGRP, CTLSEL_GDSSALDO_GOODS));
		}
		DECL_DIALOG_SETDTS()
		{
			ushort v;
			GoodsCtrlGroup::Rec rec;
			if(!RVALUEPTR(Data, pData))
				MEMSZERO(Data);
			rec.GoodsGrpID = Data.GoodsGrpID;
			rec.GoodsID = Data.GoodsID;
			rec.Flags   = GoodsCtrlGroup::enableSelUpLevel;
			setGroupData(ctlgroupGoods, &rec);
			{
				const  PPID acs_id = GetSellAccSheet();
				SetupArCombo(this, CTLSEL_GDSSALDO_CNTRAGNT, Data.ArID, 0, acs_id, sacfDisableIfZeroSheet);
				if(!acs_id && isCurrCtlID(CTLSEL_GDSSALDO_CNTRAGNT))
					selectNext();
			}
			SetupSubstDateCombo(this, CTLSEL_GDSSALDO_PERIOD, Data.CalcPeriod);
			ReplySelection();
			v = Data.FullCalc;
			setCtrlData(CTL_GDSSALDO_HOW, &v);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok  = 1;
			uint   sel = 0;
			ushort v;
			GoodsCtrlGroup::Rec rec;
			THROW(getGroupData(ctlgroupGoods, &rec));
			Data.GoodsGrpID = rec.GoodsGrpID;
			Data.GoodsID    = rec.GoodsID;
			sel = CTL_GDSSALDO_GGRP;
			THROW_PP_S(Data.GoodsGrpID, PPERR_GOODSGROUPNEEDED, GetGoodsName(rec.GoodsID, SLS.AcquireRvlStr())); // @v10.3.6 THROW_PP-->THROW_PP_S(..,GetGoodsName(rec.GoodsID))
			getCtrlData(CTLSEL_GDSSALDO_CNTRAGNT, &Data.ArID);
			getCtrlData(CTLSEL_GDSSALDO_PERIOD,   &Data.CalcPeriod);
			getCtrlData(CTL_GDSSALDO_HOW, &v);
			Data.FullCalc = v;
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmCBSelected)) {
				ReplySelection();
				clearEvent(event);
			}
		}
		void   ReplySelection()
		{
			LDATE  dt;
			char   last_calc_date[16];
			GoodsCtrlGroup::Rec rec;
			memzero(last_calc_date, sizeof(last_calc_date));
			if(getGroupData(ctlgroupGoods, &rec)) {
				PPID   ar_id = getCtrlLong(CTLSEL_GDSSALDO_CNTRAGNT);
				GSCore.GetLastCalcDate(rec.GoodsGrpID, rec.GoodsID, ar_id, 0 /*dlvrLocID*/, &dt);
				datefmt(&dt, DATF_DMY, last_calc_date);
				setCtrlData(CTL_GDSSALDO_LASTCALC, &last_calc_date);
			}
		}
		GoodsSaldoCore GSCore;
	};
	int    ok = -1, valid_data;
	GoodsSaldoParamDlg * dlg = 0;
	THROW_INVARG(pPar);
	THROW(CheckDialogPtr(&(dlg = new GoodsSaldoParamDlg())));
	dlg->setDTS(pPar);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
		if(dlg->getDTS(pPar))
			ok = valid_data = 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int PrcssrGoodsSaldo::Init(const Param * pPar)
{
	return RVALUEPTR(Par, pPar) ? 1 : PPSetErrorInvParam();
}

IMPL_CMPFUNC(GArSEntry, i1, i2)
	{ RET_CMPCASCADE4(static_cast<const GArSEntry *>(i1), static_cast<const GArSEntry *>(i2), GoodsID, ArID, DlvrLocID, Dt); }

int PrcssrGoodsSaldo::SetupItem(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, double qtty, double amt, TSVector <GArSEntry> & rList)
{
	int    ok = 1;
	if(checkdate(dt)) {
		uint pos = 0;
		GArSEntry key_entry;
		double prev_total_qtty = 0.0;
		double prev_total_amt = 0.0;
		//
		key_entry.GoodsID = goodsID;
		key_entry.ArID = arID;
		key_entry.DlvrLocID = dlvrLocID;
		key_entry.Dt = ZERODATE;
		if(rList.lsearch(&key_entry, &(pos = 0), PTR_CMPFUNC(GArSEntry))) {
			GArSEntry & r_entry = rList.at(pos);
			prev_total_qtty = r_entry.Qtty; // !
			prev_total_amt = r_entry.Amt;   // !
			r_entry.Qtty += qtty;
			r_entry.Amt += amt;
		}
		else {
			GArSEntry entry;
			MEMSZERO(entry);
			entry.GoodsID = goodsID;
			entry.ArID = arID;
			entry.DlvrLocID = dlvrLocID;
			entry.Dt = ZERODATE;
			entry.Qtty = qtty;
			entry.Amt = amt;
			THROW_SL(rList.insert(&entry));
		}
		//
		key_entry.GoodsID = goodsID;
		key_entry.ArID = arID;
		key_entry.DlvrLocID = dlvrLocID;
		key_entry.Dt = dt;
		if(rList.lsearch(&key_entry, &(pos = 0), PTR_CMPFUNC(GArSEntry))) {
			GArSEntry & r_entry = rList.at(pos);
			r_entry.Qtty += qtty;
			r_entry.Amt += amt;
		}
		else {
			GArSEntry entry;
			MEMSZERO(entry);
			entry.GoodsID = goodsID;
			entry.ArID = arID;
			entry.DlvrLocID = dlvrLocID;
			entry.Dt = dt;
			entry.Qtty = prev_total_qtty + qtty;
			entry.Amt = prev_total_amt + amt;
			THROW_SL(rList.insert(&entry));
		}
	}
	CATCHZOK
	return ok;
}

int PrcssrGoodsSaldo::Test(PPID goodsID, PPID arID, PPID dlvrLocID, const DateRange * pPeriod)
{
	int    ok = 1;
	PPLogger logger;
    DateRange period;
    if(goodsID && arID) {
		PPObjBill * p_bobj = BillObj;
		SString msg_buf, fmt_buf;
		if(pPeriod) {
			if(!checkdate(pPeriod->upp))
				period.upp = getcurdate_();
			if(!checkdate(pPeriod->low))
				period.low = plusdate(period.upp, -180);
		}
		else {
			period.upp = getcurdate_();
			period.low = plusdate(period.upp, -180);
		}
		// PPTXT_TESTGS_LOG_HEADER "Тестирование расчета сальдо по товару '@goods' для контрагента '@article' (@locaddr) за период @period"
		PPFormatT(PPTXT_TESTGS_LOG_HEADER, &msg_buf, goodsID, arID, dlvrLocID, period);
		logger.Log(msg_buf);
		for(LDATE dt = period.low; dt <= period.upp; dt = plusdate(dt, 1)) {
			double saldo_qtty = 0.0;
			double saldo_amt = 0.0;
			double direct_saldo_qtty = 0.0;
			double direct_saldo_amt = 0.0;
			DateRange local_period;
			local_period.Set(ZERODATE, dt);
			THROW(p_bobj->GetGoodsSaldo(goodsID, arID, dlvrLocID, dt, MAXLONG, &saldo_qtty, &saldo_amt));
			THROW(p_bobj->CalcGoodsSaldo(goodsID, arID, dlvrLocID, &local_period, MAXLONG, &direct_saldo_qtty, &direct_saldo_amt));
			if(direct_saldo_qtty != saldo_qtty) {
                //PPTXT_TESTGS_LOG_ERROR        "Ошибка в расчета сальдо за @date: GetGoodsSaldo=@real, CalcGoodsSaldo=@real"
                PPFormatT(PPTXT_TESTGS_LOG_ERROR, &msg_buf, dt, saldo_qtty, direct_saldo_qtty);
                logger.Log(msg_buf);
			}
		}
    }
    else
		ok = -1;
	CATCHZOK
	return ok;
}

int PrcssrGoodsSaldo::Run()
{
	int    ok = 1;
	const  LDATE end_date = plusdate(NZOR(Par.Dt, getcurdate_()), -1);
	PPIDArray goods_list;
	PPIDArray actual_goods_list; // Список товаров, для которых надо удалить записи перед вставкой новых
	SString goods_name, temp_buf;
	IterCounter cntr;
	TSVector <GArSEntry> list;
	THROW_INVARG(Par.GoodsGrpID || Par.GoodsID);
	PPWaitStart();
	if(!Par.GoodsID) {
		THROW(GoodsIterator::GetListByGroup(Par.GoodsGrpID, &goods_list));
		goods_list.sort();
	}
	else
		goods_list.add(Par.GoodsID);
	for(uint i = 0; i < goods_list.getCount(); i++) {
		const  PPID goods_id = labs(goods_list.get(i));
		GetGoodsName(goods_id, goods_name);
		PPWaitMsg(goods_name);
		LDATE  last_date = ZERODATE;
		TransferTbl::Rec trfr_rec;
		BillTbl::Rec bill_rec;
		GCTFilt gct_filt;
		gct_filt.Period.Set(ZERODATE, end_date);
		gct_filt.GoodsID = goods_id;
		gct_filt.Flags |= OPG_FORCEBILLCACHE;
		GCTIterator gctiter(&gct_filt, &gct_filt.Period);
		if(gctiter.First(&trfr_rec, &bill_rec) > 0) {
			do {
				const  PPID  ar_id = bill_rec.Object;
				const LDATE dt = trfr_rec.Dt;
				PPWaitMsg((temp_buf = goods_name).CatDiv('-', 1).Cat(dt));
				if(ar_id != 0 && GetOpType(bill_rec.OpID) != PPOPT_GOODSREVAL) {
					const double qtty = trfr_rec.Quantity;
					const double amount = qtty * (trfr_rec.Price - trfr_rec.Discount);
					/*
					if(Par.CalcPeriod == sgdWeek)
						dt = plusdate(dt, 7-dayofweek(&dt, 1));
					else if(Par.CalcPeriod == sgdMonth)
						dt = plusdate(dt, dt.dayspermonth()-dt.day());
					else if(Par.CalcPeriod == sgdQuart)
						dt = encodedate(1, ((dt.month() + 2) / 3) * 3, dt.year());
					else if(Par.CalcPeriod == sgdYear)
						dt = encodedate(1, 12, dt.year());
					*/
					//
					// Если скорректированная с учетом периодичности дата превышает заданную в параметрах,
					// то не заполняем соответствующие элементы
					//
					if(dt <= end_date) {
						PPFreight freight;
						const  PPID   dlvr_loc_id = (BillObj->P_Tbl->GetFreight(bill_rec.ID, &freight) > 0) ? freight.DlvrAddrID__ : 0;
						THROW(SetupItem(goods_id, ar_id, dlvr_loc_id, dt, qtty, amount, list));
						if(dlvr_loc_id) {
							THROW(SetupItem(goods_id, ar_id, 0, dt, qtty, amount, list));
						}
						SETMAX(last_date, dt);
					}
				}
			} while(gctiter.Next(&trfr_rec, &bill_rec) > 0);
		}
		//
		// Устанавливаем последнюю дату периода расчета для товара.
		// Это необходимо для того, чтобы при вычислении сальдо по клиенту, для которого было
		// мало операций не перечислять все с начала времен, а считать от этой самой даты.
		//
		if(!checkdate(last_date))
			last_date = end_date;
		THROW(SetupItem(goods_id, 0, 0, last_date, 0.0, 0.0, list));
		//
		actual_goods_list.add(goods_id);
	}
	{
		PPTransaction tra(1);
		THROW(tra);
		{
			actual_goods_list.sortAndUndup();
			for(uint i = 0; i < actual_goods_list.getCount(); i++) {
				const  PPID goods_id = actual_goods_list.get(i);
				THROW_DB(deleteFrom(&GSCore, 0, (GSCore.GoodsID == goods_id)));
			}
		}
		{
			BExtInsert bei(&GSCore);
			for(uint i = 0; i < list.getCount(); i++) {
				const GArSEntry & r_entry = list.at(i);
				if(r_entry.Dt) {
					GoodsDebtTbl::Rec rec;
					rec.GoodsID = r_entry.GoodsID;
					rec.ArID = r_entry.ArID;
					rec.DlvrLocID = r_entry.DlvrLocID;
					rec.Dt = r_entry.Dt;
					rec.SaldoQtty = r_entry.Qtty;
					rec.SaldoAmount = r_entry.Amt;
					THROW_DB(bei.insert(&rec));
				}
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int ProcessGoodsSaldo()
{
	int    ok = -1;
	PrcssrGoodsSaldo prcssr;
	PrcssrGoodsSaldo::Param param;
	prcssr.InitParam(&param);
	while(prcssr.EditParam(&param) > 0) {
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else {
			ok = PPErrorZ();
			break;
		}
	}
	return ok;
}
