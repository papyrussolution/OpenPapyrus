// GDSSALDO.CPP
// Copyright (c) V.Nasonov 2003, 2005, 2007, 2009, 2010, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
// Расчет сальдо по товарам
//
#include <pp.h>
#pragma hdrstop
//
// GoodsSaldoCore
//
SLAPI GoodsSaldoCore::GoodsSaldoCore() : GoodsDebtTbl()
{
}

int SLAPI GoodsSaldoCore::GetLastSaldo(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE * pDt, GoodsDebtTbl::Rec * pRec)
{
	int    ok = -1;
	LDATE  dt = ZERODATE;
	GoodsDebtTbl::Rec gd_rec;
	MEMSZERO(gd_rec);
	if(goodsID) {
		const LDATE base_date = (pDt == 0 || *pDt == ZERODATE) ? MAXDATE : *pDt;
		GoodsDebtTbl::Key0 k0;
		MEMSZERO(k0);
		k0.GoodsID = goodsID;
		k0.ArID    = arID;
		k0.DlvrLocID = dlvrLocID;
		k0.Dt      = base_date;
		if(search(0, &k0, spLt) && k0.GoodsID == goodsID && k0.ArID == arID && k0.DlvrLocID == dlvrLocID) { // @v8.8.0 spLe-->spLt
			copyBufTo(&gd_rec);
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
		ok = PPSetError(PPERR_INVPARAM);
	ASSIGN_PTR(pRec, gd_rec);
	ASSIGN_PTR(pDt, dt);
	return ok;
}

int SLAPI GoodsSaldoCore::GetLastCalcDate(PPID goodsGrpID, PPID goodsID, PPID arID, PPID dlvrLocID, LDATE * pDt)
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
			THROW(temp_goods_list.sort());
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
struct CalcSaldoEntry {
	SLAPI  CalcSaldoEntry()
	{
		THISZERO();
	}
	PPID   GoodsID;
	PPID   ArticleID;
	LDATE  Dt;
};

class CalcSaldoList : public SArray {
public:
	SLAPI  CalcSaldoList() : SArray(sizeof(CalcSaldoEntry))
	{
	}
	int   SLAPI Search(PPID gdsID, PPID artID, uint * p = 0);
	int   SLAPI Insert(CalcSaldoEntry * e, uint * p = 0);
	CalcSaldoEntry & FASTCALL at(uint p) const
	{
		return *(CalcSaldoEntry*)SArray::at(p);
	}
};

IMPL_CMPFUNC(CalcSaldoEnKey, i1, i2)
{
	int    si = 0;
	CMPCASCADE2(si, (CalcSaldoEntry*)i1, (CalcSaldoEntry*)i2, GoodsID, ArticleID);
	return si;
}

int SLAPI CalcSaldoList::Search(PPID gdsID, PPID artID, uint * p)
{
	CalcSaldoEntry dse;
	dse.GoodsID   = gdsID;
	dse.ArticleID = artID;
	return bsearch(&dse, p, PTR_CMPFUNC(CalcSaldoEnKey));
}

int SLAPI CalcSaldoList::Insert(CalcSaldoEntry * e, uint * p)
{
	return ordInsert(e, p, PTR_CMPFUNC(CalcSaldoEnKey)) ? 1 : PPSetErrorSLib();
}
//
//   PrcssrGoodsSaldo
//
#define DEF_SALDO_PERIOD  7L

struct GArSEntry {
	PPID   GoodsID;
	PPID   ArID;
	PPID   DlvrLocID; // @v9.1.8
	LDATE  Dt;
	double Qtty;
	double Amt;
};

struct GArSLastEntry {
	PPID   GoodsID;
	PPID   ArID;
	PPID   DlvrLocID; // @v9.1.8
	uint   Pos;
};

class PrcssrGoodsSaldo {
public:
	struct Param {
		PPID  GoodsGrpID;
		PPID  GoodsID;
		PPID  ArID;
		LDATE Dt;
		int   CalcPeriod; // sgdXXX
		int   FullCalc;
	};

	SLAPI  PrcssrGoodsSaldo()
	{
	}
	int    SLAPI InitParam(Param *);
	int    SLAPI EditParam(Param *);
	int    SLAPI Init(const Param *);
	int    SLAPI Run();
	int    SLAPI Test(PPID goodsID, PPID arID, PPID dlvrID, const DateRange * pPeriod);
private:
	int    SLAPI SetUnworkingContragentsSaldo(LDATE dt, CalcSaldoList * pList);
	int    SLAPI SetupItem(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, double qtty, double amt, TSArray <GArSEntry> & rList);

	GoodsSaldoCore GSCore;
	Param  Par;
};

int SLAPI PrcssrGoodsSaldo::InitParam(Param * pPar)
{
	if(pPar) {
		memzero(pPar, sizeof(*pPar));
		PPObjGoods goods_obj;
		pPar->GoodsGrpID = goods_obj.GetConfig().TareGrpID;
	}
	return 1;
}
//
//   Implementation of GoodsSaldoParamDlg
//
#define GSGRP_GOODS 1

class GoodsSaldoParamDlg : public TDialog {
public:
	GoodsSaldoParamDlg() : TDialog(DLG_GDSSALDO)
	{
		addGroup(GSGRP_GOODS, new GoodsCtrlGroup(CTLSEL_GDSSALDO_GGRP, CTLSEL_GDSSALDO_GOODS));
	}
	int    setDTS(const PrcssrGoodsSaldo::Param *);
	int    getDTS(PrcssrGoodsSaldo::Param * pPar);
private:
	DECL_HANDLE_EVENT;
	void   ReplySelection();

	PrcssrGoodsSaldo::Param GSParam;
	GoodsSaldoCore GSCore;
};

void GoodsSaldoParamDlg::ReplySelection()
{
	LDATE  dt;
	char   last_calc_date[16];
	GoodsCtrlGroup::Rec rec;
	memzero(last_calc_date, sizeof(last_calc_date));
	if(getGroupData(GSGRP_GOODS, &rec)) {
		PPID   ar_id = getCtrlLong(CTLSEL_GDSSALDO_CNTRAGNT);
		GSCore.GetLastCalcDate(rec.GrpID, rec.GoodsID, ar_id, 0 /*dlvrLocID*/, &dt);
		datefmt(&dt, DATF_DMY, last_calc_date);
		setCtrlData(CTL_GDSSALDO_LASTCALC, &last_calc_date);
	}
}

IMPL_HANDLE_EVENT(GoodsSaldoParamDlg)
{
	TDialog::handleEvent(event);
	if(TVCOMMAND && TVCMD == cmCBSelected) {
		ReplySelection();
		clearEvent(event);
	}
}

int GoodsSaldoParamDlg::setDTS(const PrcssrGoodsSaldo::Param * pPar)
{
	ushort v;
	GoodsCtrlGroup::Rec rec;
	if(!RVALUEPTR(GSParam, pPar))
		MEMSZERO(GSParam);
	MEMSZERO(rec);
	rec.GrpID   = GSParam.GoodsGrpID;
	rec.GoodsID = GSParam.GoodsID;
	rec.Flags   = GoodsCtrlGroup::enableSelUpLevel;
	setGroupData(GSGRP_GOODS, &rec);
	{
		const PPID acs_id = GetSellAccSheet();
		SetupArCombo(this, CTLSEL_GDSSALDO_CNTRAGNT, GSParam.ArID, 0, acs_id, sacfDisableIfZeroSheet);
		if(!acs_id && isCurrCtlID(CTLSEL_GDSSALDO_CNTRAGNT))
			selectNext();
	}
	SetupSubstDateCombo(this, CTLSEL_GDSSALDO_PERIOD, GSParam.CalcPeriod);
	ReplySelection();
	v = GSParam.FullCalc;
	setCtrlData(CTL_GDSSALDO_HOW, &v);
	return 1;
}

int GoodsSaldoParamDlg::getDTS(PrcssrGoodsSaldo::Param * pPar)
{
	int    ok  = 1;
	uint   sel = 0;
	ushort v;
	GoodsCtrlGroup::Rec rec;
	THROW(getGroupData(GSGRP_GOODS, &rec));
	GSParam.GoodsGrpID = rec.GrpID;
	GSParam.GoodsID    = rec.GoodsID;
	sel = CTL_GDSSALDO_GGRP;
	THROW_PP(GSParam.GoodsGrpID, PPERR_GOODSGROUPNEEDED);
	getCtrlData(CTLSEL_GDSSALDO_CNTRAGNT, &GSParam.ArID);
	getCtrlData(CTLSEL_GDSSALDO_PERIOD,   &GSParam.CalcPeriod);
	getCtrlData(CTL_GDSSALDO_HOW, &v);
	GSParam.FullCalc = v;
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	ASSIGN_PTR(pPar, GSParam);
	return ok;
}
//
//
//
int SLAPI PrcssrGoodsSaldo::EditParam(Param * pPar)
{
	int    ok = -1, valid_data;
	GoodsSaldoParamDlg * dlg = 0;
	THROW_PP(pPar, PPERR_INVPARAM);
	THROW(CheckDialogPtr(&(dlg = new GoodsSaldoParamDlg())));
	dlg->setDTS(pPar);
	for(valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
		if(dlg->getDTS(pPar))
			ok = valid_data = 1;
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PrcssrGoodsSaldo::Init(const Param * pPar)
{
	return RVALUEPTR(Par, pPar) ? 1 : PPSetError(PPERR_INVPARAM);
}

IMPL_CMPFUNC(GArSEntry, i1, i2)
{
	int    si = 0;
	CMPCASCADE4(si, (const GArSEntry *)i1, (const GArSEntry *)i2, GoodsID, ArID, DlvrLocID, Dt);
	return si;
}

int SLAPI PrcssrGoodsSaldo::SetupItem(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, double qtty, double amt, TSArray <GArSEntry> & rList)
{
	int    ok = 1;
	if(checkdate(dt, 0)) {
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

int SLAPI PrcssrGoodsSaldo::Test(PPID goodsID, PPID arID, PPID dlvrLocID, const DateRange * pPeriod)
{
	int    ok = 1;
	PPLogger logger;
    DateRange period;
    if(goodsID && arID) {
		SString msg_buf, fmt_buf;
		if(pPeriod) {
			if(!checkdate(pPeriod->upp, 0))
				period.upp = getcurdate_();
			if(!checkdate(pPeriod->low, 0))
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
			THROW(BillObj->GetGoodsSaldo(goodsID, arID, dlvrLocID, dt, MAXLONG, &saldo_qtty, &saldo_amt));
			THROW(BillObj->CalcGoodsSaldo(goodsID, arID, dlvrLocID, &local_period, MAXLONG, &direct_saldo_qtty, &direct_saldo_amt));
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

int SLAPI PrcssrGoodsSaldo::Run()
{
	int    ok = 1;
	const  LDATE end_date = plusdate(NZOR(Par.Dt, getcurdate_()), -1);
	PPIDArray goods_list;
	PPIDArray actual_goods_list; // Список товаров, для которых надо удалить записи перед вставкой новых
	SString goods_name, temp_buf;
	IterCounter cntr;
	TSArray <GArSEntry> list;
	//TSArray <GArSLastEntry> idx_list;
	THROW_PP(Par.GoodsGrpID || Par.GoodsID, PPERR_INVPARAM);
	PPWait(1);
	if(!Par.GoodsID) {
		THROW(GoodsIterator::GetListByGroup(Par.GoodsGrpID, &goods_list));
		THROW(goods_list.sort());
	}
	else
		goods_list.add(Par.GoodsID);
	for(uint i = 0; i < goods_list.getCount(); i++) {
		const PPID goods_id = labs(goods_list.get(i));
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
				const PPID ar_id = bill_rec.Object;
				LDATE dt = trfr_rec.Dt;
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
						const PPID   dlvr_loc_id = (BillObj->P_Tbl->GetFreight(bill_rec.ID, &freight) > 0) ? freight.DlvrAddrID : 0;
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
		if(!checkdate(last_date, 0))
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
				const PPID goods_id = actual_goods_list.get(i);
				THROW_DB(deleteFrom(&GSCore, 0, (GSCore.GoodsID == goods_id)));
			}
		}
		{
			BExtInsert bei(&GSCore);
			for(uint i = 0; i < list.getCount(); i++) {
				const GArSEntry & r_entry = list.at(i);
				if(r_entry.Dt) {
					GoodsDebtTbl::Rec rec;
					MEMSZERO(rec);
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
	PPWait(0);
	return ok;
}

int SLAPI ProcessGoodsSaldo()
{
	int    ok = -1;
	PrcssrGoodsSaldo prcssr;
	PrcssrGoodsSaldo::Param param;
	prcssr.InitParam(&param);
	while(prcssr.EditParam(&param) > 0)
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else {
			ok = PPErrorZ();
			break;
		}
	return ok;
}
