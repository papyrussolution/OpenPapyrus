// PREDICT.CPP
// Copyright (c) A.Starodub 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013, 2014, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
struct LocValEntry {
	LocValEntry(PPID locID, LDATE lastOpDate, double rest)
	{
		THISZERO();
		LocID = locID;
		LastOpDate = lastOpDate;
		Rest = rest;
	}
	int    IsFirstIter() const
	{
		return BIN(FirstIterTag == 0);
	}
	PPID   LocID;
	LDATE  LastOpDate;
	double Rest;
	double Sell;
	double SellAmt;
	int16  FirstIterTag; // 0 - first iteration, 1 - not first iteration
	int16  Reserve;      // @alignment
};

class LocValList : public SArray {
public:
	SLAPI  LocValList();
	int    SLAPI Setup(LDATE startDate, const GoodsRestParam *);
	int    SLAPI Setup(LDATE startDate, PPID goodsID, const TSArray <PrcssrPrediction::_GoodsLocRestItem> * pList);
	LocValEntry * FASTCALL GetEntry(PPID locID);
private:
	int    FASTCALL AddEntry(PPID locID, double rest)
	{
		LocValEntry entry(locID, StartDate, rest);
		return insert(&entry) ? 1 : PPSetErrorSLib();
	}
	LDATE  StartDate;
};

SLAPI LocValList::LocValList() : SArray(sizeof(LocValEntry), 32, O_ARRAY)
{
}

int SLAPI LocValList::Setup(LDATE startDate, PPID goodsID, const TSArray <PrcssrPrediction::_GoodsLocRestItem> * pList)
{
	assert(pList);
	StartDate = startDate;
	uint   pos = 0;
	if(pList->bsearch(&goodsID, &pos, CMPF_LONG)) {
		uint   _p = pos;
		//
		// @paranoic: функция bsearch и так находит самую первую позицию {
		//
		while(_p) {
			if(pList->at(_p-1).GoodsID == goodsID)
				_p--;
			else
				break;
		}
		// }
		while(_p < pList->getCount()) {
			const PrcssrPrediction::_GoodsLocRestItem & r_item = pList->at(_p++);
			if(r_item.GoodsID == goodsID) {
				if(r_item.LocID)
					AddEntry(r_item.LocID, r_item.Rest);
			}
			else
				break;
		}
	}
	return 1;
}

int SLAPI LocValList::Setup(LDATE startDate, const GoodsRestParam * pGrp)
{
	StartDate = startDate;
	for(uint i = 0; i < pGrp->getCount(); i++) {
		const GoodsRestVal & r_item = pGrp->at(i);
		if(r_item.LocID)
			AddEntry(r_item.LocID, r_item.Rest);
		else {
			SString msg_buf, fmt_buf, goods_name, date_buf;
			msg_buf.Printf(PPLoadTextS(PPTXT_LOG_PSALES_UNDEFLOC_REST, fmt_buf), GetGoodsName(pGrp->GoodsID, goods_name).cptr(), date_buf.Cat(pGrp->Date).cptr());
			PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
		}
	}
	return 1;
}

LocValEntry * FASTCALL LocValList::GetEntry(PPID locID)
{
	LocValEntry * p_entry = 0;
	for(uint i = 0; enumItems(&i, (void **)&p_entry);)
		if(p_entry->LocID == locID)
			return p_entry;
	AddEntry(locID, 0);
	p_entry = (LocValEntry *)at(getCount()-1);
	return p_entry;
}
//
//
//
// static
int SLAPI Predictor::GetPredictCfg(PPPredictConfig * pCfg)
{
	PPPredictConfig cfg;
	int    ok = PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PREDICTCFG, &cfg, sizeof(cfg));
	if(ok <= 0)
		MEMSZERO(cfg);
	ASSIGN_PTR(pCfg, cfg);
	return ok;
}

// static
int SLAPI Predictor::PutPredictCfg(const PPPredictConfig * pCfg, int use_ta)
{
	int    ok = 1, is_new = 1;
	PPPredictConfig prev_cfg;
	PPPredictConfig cfg = *pCfg;
	PPTransaction tra(use_ta);
	THROW(tra);
	if(PPRef->GetProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PREDICTCFG, &prev_cfg, sizeof(prev_cfg)) > 0)
		is_new = 0;
	THROW(PPRef->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_PREDICTCFG, &cfg, sizeof(cfg), 0));
	DS.LogAction(is_new ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, PPCFGOBJ_PREDICTSALES, 0, 0, 0);
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

SLAPI Predictor::Predictor()
{
	GetPredictCfg(&Cfg);
}

int SLAPI Predictor::IsWorkDay(const ObjIdListFilt * pLocList, LDATE dt)
{
	return !T.IsHoliday(pLocList, dt);
}

Predictor::EvalParam::EvalParam(SStatFilt * pFilt)
{
	THISZERO();
	if(pFilt && pFilt->Flags & SStatFilt::fOverrideCfgParams) {
		Method = pFilt->_Method;
		P = pFilt->_P;
		MinP = pFilt->_MinP; // @v6.4.0
		TrustCriterion = pFilt->_TrustCriterion;
	}
	else {
		Method = -1;
		P = -1;
		MinP = -1; // @v6.4.0
		TrustCriterion = -1;
	}
}

Predictor::EvalParam & Predictor::EvalParam::Set(const ObjIdListFilt * pLocList, PPID goodsID, const DateRange & rPeriod)
{
	P_LocList = pLocList;
	GoodsID = goodsID;
	Period = rPeriod;
	return *this;
}
//
// Период pPeriod может состоять из трех типов отрезков:
// 1. Отрезок, стоящий за верхней датой периода модели (predict_chunk)
//    Данные по этому отрезку заполняются непосредственно прогнозированием.
// 2. Отрезок, пересекающий период модели (model_chunk)
//    Заполняется известными данными о продажах из таблицы PredictSales
// 3. Отрезок, предшествующий периоду модели (proto_chunk)
//    Заполняется значениями среднедневных продаж
//
// rParam.Period должен быть закрытым с обеих сторон (rParam.Period->low != 0 && rParam.Period->up != 0).
//
int SLAPI Predictor::Predict_(const EvalParam & rParam, double * pVal, PredictSalesStat * pStat, int * pCanTrust)
{
	int    ok = 1, result = 0, r, can_trust = 0;
	double predict_qtty = 0.0, model_qtty = 0.0, proto_qtty = 0.0, total_qtty = 0.0;
	DateRange period, model_period;
	PredictSalesStat pss(0, 0);
	//
	ObjIdListFilt inner_loc_list;
	{
		PPIDArray temp_loc_list;
		if(rParam.P_LocList == 0 || !rParam.P_LocList->IsExists())
			T.GetLocList(temp_loc_list);
		else
			temp_loc_list = rParam.P_LocList->Get();
		inner_loc_list.Set(&temp_loc_list);
	}
	//
	THROW_INVARG(rParam.Period.low && rParam.Period.upp);
	period = rParam.Period;
	const PPID goods_id = rParam.GoodsID;
	THROW(r = T.GetStat(goods_id, inner_loc_list, &pss));
	if(r > 0) {
		ASSIGN_PTR(pStat, pss);
		if(T.GetPeriod(&inner_loc_list, goods_id, &model_period) > 0) {
			double final_coeff = 1.0;
			PPQuotArray coeff_qlist;
			const PPQuotArray * p_coeff_qlist = 0;
			if(Cfg.CorrectKoeff) {
				const  PPID q_loc_id = inner_loc_list.GetSingle();
				PPObjGoods goods_obj;
				goods_obj.P_Tbl->FetchQuotList(goods_id, Cfg.CorrectKoeff, 0, coeff_qlist);
				int    is_there_prd_coeff = 0;
				uint   i = coeff_qlist.getCount();
				if(i) do {
					const PPQuot & r_q = coeff_qlist.at(--i);
					if(!r_q.Period.IsZero() && r_q.Period.upp >= r_q.Period.low) {
						is_there_prd_coeff = 1;
					}
					else
						coeff_qlist.atFree(i);
				} while(i);
				if(is_there_prd_coeff) {
					p_coeff_qlist = &coeff_qlist;
				}
				{
					QuotIdent qi(q_loc_id, Cfg.CorrectKoeff);
					double coeff = 0.0;
					goods_obj.GetQuot(goods_id, qi, 1.0, 1.0, &coeff, 1);
					if(coeff > 0.0 && coeff < 10.0)
						final_coeff = coeff;
				}
			}
			DateRange predict_chunk, model_chunk, proto_chunk;
			predict_chunk.SetZero();
			model_chunk.SetZero();
			proto_chunk.SetZero();
			if(period.upp > model_period.upp) {
				predict_chunk.upp = period.upp;
				predict_chunk.low = MAX(period.low, plusdate(model_period.upp, 1));
				result |= 0x01;
			}
			if(period.low < model_period.low) {
				proto_chunk.low = period.low;
				proto_chunk.upp = MIN(period.upp, plusdate(model_period.low, -1));
				result |= 0x04;
			}
			if(period.low <= model_period.upp) {
				model_chunk.low = MAX(period.low, model_period.low);
				model_chunk.upp = MIN(period.upp, model_period.upp);
				result |= 0x02;
			}
			if(!predict_chunk.IsZero()) {
				int    method = 0;
				double average = 0.0;
				DateRange load_period;
				load_period.Set(ZERODATE, (rParam.LoadUpDate && rParam.LoadUpDate < model_period.upp) ? rParam.LoadUpDate : model_period.upp);
				PredictSalesStat stat(Cfg.CorrectKoeff, p_coeff_qlist);
				can_trust = 1;
				const int np = (rParam.P == -1) ? Cfg.P : rParam.P;
				const int min_p = (rParam.MinP == -1) ? Cfg.MinP : rParam.MinP;
				const int method_ = (rParam.Method == -1) ? Cfg.Method : rParam.Method;
				const int trust_crit = (rParam.TrustCriterion == -1) ? Cfg.TrustCriterion : rParam.TrustCriterion;
				if(method_ == PRMTHD_LSLIN && (np == 0 || np >= 3)) {
					stat.Flags |= PSSF_USELSSLIN;
					T.CalcStat(goods_id, inner_loc_list, &load_period, -np, &stat);
					method = PRMTHD_LSLIN;
				}
				else { // Simple average
					//
					// Если в конфигурации указано количество значений меньшее
					// или равное 0, то средние продажи извлекаем из статистики.
					// В противном случае вычисляем средние продажи по np последним точкам.
					//
					if(np > 0)
						T.CalcStat(goods_id, inner_loc_list, &load_period, -np, &stat);
					average = stat.GetAverage(PSSV_QTTY);
					method = PRMTHD_SIMPLEAVERAGE;
				}
				if((min_p > 0 && min_p <= np) && stat.Count < min_p) {
					can_trust = 0;
				}
				else {
					long   diff = 0;
					LDATE  end_dt = MAX(model_period.upp, pss.LastDate);
					for(LDATE dt = stat.FirstPointDate; dt <= end_dt; dt = plusdate(dt, 1)) {
						if(!T.IsHoliday(&inner_loc_list, dt))
							diff++;
					}
					double crit = fdiv100i(trust_crit);
					if(diff != 0 && crit > (double)stat.Count / diff)
						can_trust = 0;
				}
				{
					long   num_days = 0;
					for(LDATE dt = plusdate(model_period.upp, 1); dt <= period.upp; dt = plusdate(dt, 1))
						if(!T.IsHoliday(&inner_loc_list, dt) && period.CheckDate(dt)) {
							num_days++;
							if(method == PRMTHD_LSLIN)
								predict_qtty += stat.Predict(PSSV_QTTY, dt, 0);
						}
					if(method == PRMTHD_SIMPLEAVERAGE && stat.Count)
						predict_qtty = num_days * average;
				}
			}
			if(!model_chunk.IsZero()) {
				PredictSalesStat stat(0, 0);
				THROW(T.CalcStat(goods_id, inner_loc_list, &model_chunk, 0, &stat));
				model_qtty = stat.QttySum;
			}
			if(!proto_chunk.IsZero())
				if(pss.Count) {
					long   num_days = 0;
					for(LDATE dt = proto_chunk.low; dt <= proto_chunk.upp; dt = plusdate(dt, 1))
						if(!T.IsHoliday(&inner_loc_list, dt))
							num_days++;
					proto_qtty = num_days * pss.GetAverage(PSSV_QTTY);
				}
			total_qtty = (proto_qtty + model_qtty + predict_qtty) * final_coeff;
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	CATCHZOK
	ASSIGN_PTR(pVal, total_qtty);
	ASSIGN_PTR(pCanTrust, can_trust);
	return ok;
}

int SLAPI Predictor::GetStat(PPID goodsID, const ObjIdListFilt & rLocList, PredictSalesStat * pStat)
{
	return T.GetStat(goodsID, rLocList, pStat);
}

int SLAPI GetEstimatedSales(const ObjIdListFilt * pLocList, PPID goodsID, const DateRange * pPeriod, double * pQtty)
{
	int    ok = 1;
	Predictor predictor;
	Predictor::EvalParam p;
	ok = predictor.Predict_(p.Set(pLocList, goodsID, *pPeriod), pQtty, 0);
	return ok;
}
//
//
//
int PrcssrPrediction::Param::SetPeriod(DateRange period)
{
	Period = period;
	return 1;
}

DateRange PrcssrPrediction::Param::GetPeriod() const
{
	return Period;
}

DateRange PrcssrPrediction::Param::GetNormPeriod() const
{
	DateRange norm = Period;
	norm.Actualize(ZERODATE);
	return norm;
}

SLAPI PPPredictConfig::PPPredictConfig()
{
	THISZERO();
}

// static
int FASTCALL PPPredictConfig::_GetPckgUse(long f)
{
	const long t = CHKXORFLAGS(f, fPrefStockPckg, fPrefLotPckg);
	return (t == fPrefStockPckg) ? pckgPrefStock : ((t == fPrefLotPckg) ? pckgPrefLot : pckgDontUse);
}

// static
int FASTCALL PPPredictConfig::_GetPckgRounding(long f)
{
	const long t = CHKXORFLAGS(f, fRoundPckgUp, fRoundPckgDn);
	return (t == fRoundPckgUp) ? pckgRoundUp : ((t == fRoundPckgDn) ? pckgRoundDn : pckgRoundNear);
}

// static
long FASTCALL PPPredictConfig::_SetPckgUse(long f, int t)
{
	f &= ~(fPrefStockPckg | fPrefLotPckg);
	if(t == pckgPrefStock)
		f |= fPrefStockPckg;
	else if(t == pckgPrefLot)
		f |= fPrefLotPckg;
	return f;
}

// static
long FASTCALL PPPredictConfig::_SetPckgRounding(long f, int t)
{
	f &= ~(fRoundPckgUp | fRoundPckgDn);
	if(t == pckgRoundUp)
		f |= fRoundPckgUp;
	else if(t == pckgRoundDn)
		f |= fRoundPckgDn;
	return f;
}

//
// static
int SLAPI PrcssrPrediction::EditPredictCfg()
{
	int    ok = -1, is_new = 0;
	ushort v = 0;
	PPIDArray op_list;
	TDialog * dlg = new TDialog(DLG_PREDICTCFG);
	PPPredictConfig cfg;

	THROW(CheckCfgRights(PPCFGOBJ_PREDICTSALES, PPR_READ, 0));
	MEMSZERO(cfg);
	is_new = PrcssrPrediction::GetPredictCfg(&cfg);

	THROW(CheckDialogPtr(&dlg));
	op_list.add(PPOPT_DRAFTRECEIPT);
	SetupOprKindCombo(dlg, CTLSEL_PREDICTCFG_OPRPCH, cfg.PurchaseOpID, 0, &op_list, 0);
	SetupPPObjCombo(dlg, CTLSEL_PREDICTCFG_OPRS, PPOBJ_OPRKIND, cfg.OpID, 0, 0);
	dlg->SetupCalDate(CTLCAL_PREDICTCFG_DTBEGSELL, CTL_PREDICTCFG_DTBEGSELL);
	dlg->setCtrlData(CTL_PREDICTCFG_DTBEGSELL, &cfg.StartDate);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_METHOD,  0, PRMTHD_SIMPLEAVERAGE);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_METHOD, -1, PRMTHD_SIMPLEAVERAGE);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_METHOD,  1, PRMTHD_LSLIN);
	dlg->SetClusterData(CTL_PREDICTCFG_METHOD, cfg.Method);

	dlg->AddClusterAssoc(CTL_PREDICTCFG_USEPCKG,  0, PPPredictConfig::pckgDontUse);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_USEPCKG, -1, PPPredictConfig::pckgDontUse);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_USEPCKG,  1, PPPredictConfig::pckgPrefStock);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_USEPCKG,  2, PPPredictConfig::pckgPrefLot);
	dlg->SetClusterData(CTL_PREDICTCFG_USEPCKG, cfg.GetPckgUse());

	dlg->AddClusterAssoc(CTL_PREDICTCFG_ROUNDPCKG,  0, PPPredictConfig::pckgRoundUp);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_ROUNDPCKG, -1, PPPredictConfig::pckgRoundUp);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_ROUNDPCKG,  1, PPPredictConfig::pckgRoundDn);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_ROUNDPCKG,  2, PPPredictConfig::pckgRoundNear);
	dlg->SetClusterData(CTL_PREDICTCFG_ROUNDPCKG, cfg.GetPckgRounding());

	dlg->setCtrlData(CTL_PREDICTCFG_TRUST,  &cfg.TrustCriterion);
	dlg->setCtrlData(CTL_PREDICTCFG_P, &cfg.P);
	dlg->setCtrlData(CTL_PREDICTCFG_PMIN, &cfg.MinP);
	dlg->setCtrlData(CTL_PREDICTCFG_Q, &cfg.Q);
	dlg->setCtrlData(CTL_PREDICTCFG_DINSTOCK, &cfg.DefInsurStock);
	// @v6.2.4 dlg->AddClusterAssoc(CTL_PREDICTCFG_FLAGS, 0, PPPredictConfig::fAddMinStockToOrder);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_FLAGS, 0, PPPredictConfig::fZeroPckgUp);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_FLAGS, 1, PPPredictConfig::fUseInsurStock);
	dlg->AddClusterAssoc(CTL_PREDICTCFG_FLAGS, 2, PPPredictConfig::fMinStockAsMinOrder);
	dlg->SetClusterData(CTL_PREDICTCFG_FLAGS, cfg.Flags);
	dlg->setCtrlData(CTL_PREDICTCFG_DTENDCALC, &cfg.EndCalcDate);
	if(cfg.Flags & PPPredictConfig::fContinueBuilding) {
		SString msg_buf;
		PPLoadText(PPTXT_PSALES_CONTINUEMODE, msg_buf);
		dlg->setStaticText(CTL_PREDICTCFG_ST_CONT, msg_buf);
	}
	//
	// Фиксирование артикулов по поставщику, для Рассчета заказа по поставщику
	//
	{
		dlg->AddClusterAssoc(CTL_PREDICTCFG_FIXARCODE, -1, 0);
		dlg->AddClusterAssoc(CTL_PREDICTCFG_FIXARCODE,  0, 0);
		dlg->AddClusterAssoc(CTL_PREDICTCFG_FIXARCODE,  1, SStatFilt::fExtByArCode);
		dlg->AddClusterAssoc(CTL_PREDICTCFG_FIXARCODE,  2, SStatFilt::fRestrictByArCode);
		if(!(CConfig.Flags & CCFLG_USEARGOODSCODE)) {
			cfg.FixArCodes = 0;
			dlg->disableCtrl(CTL_PREDICTCFG_FIXARCODE, 1);
		}
		dlg->SetClusterData(CTL_PREDICTCFG_FIXARCODE, (long)cfg.FixArCodes);
	}
	SetupPPObjCombo(dlg, CTLSEL_PREDICTCFG_CKOEFF,  PPOBJ_QUOTKIND, cfg.CorrectKoeff, OLW_CANINSERT, (void *)1);
	for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
		long   temp_long;
		THROW(CheckCfgRights(PPCFGOBJ_PREDICTSALES, PPR_MOD, 0));
		dlg->getCtrlData(CTL_PREDICTCFG_OPRS, &cfg.OpID);
		dlg->getCtrlData(CTL_PREDICTCFG_OPRPCH, &cfg.PurchaseOpID);
		dlg->getCtrlData(CTL_PREDICTCFG_DTBEGSELL, &cfg.StartDate);
		dlg->GetClusterData(CTL_PREDICTCFG_METHOD, &cfg.Method);
		dlg->GetClusterData(CTL_PREDICTCFG_USEPCKG, &(temp_long = 0));
		cfg.SetPckgUse(temp_long);
		dlg->GetClusterData(CTL_PREDICTCFG_ROUNDPCKG, &(temp_long = 0));
		cfg.SetPckgRounding(temp_long);

		dlg->getCtrlData(CTL_PREDICTCFG_TRUST,  &cfg.TrustCriterion);
		dlg->getCtrlData(CTL_PREDICTCFG_P, &cfg.P);
		dlg->getCtrlData(CTL_PREDICTCFG_PMIN, &cfg.MinP);
		dlg->getCtrlData(CTL_PREDICTCFG_Q, &cfg.Q);
		dlg->getCtrlData(CTL_PREDICTCFG_DINSTOCK, &cfg.DefInsurStock);
		dlg->GetClusterData(CTL_PREDICTCFG_FLAGS, &cfg.Flags);
		dlg->getCtrlData(CTLSEL_PREDICTCFG_CKOEFF, &cfg.CorrectKoeff);
		{
			long fix_ar_codes = 0;
			dlg->GetClusterData(CTL_PREDICTCFG_FIXARCODE, &fix_ar_codes);
			cfg.FixArCodes = (int16)fix_ar_codes;
		}
		if(cfg.TrustCriterion < 0 || cfg.TrustCriterion > 100)
			PPErrorByDialog(dlg, CTL_PREDICTCFG_TRUST, PPERR_PERCENTINPUT);
		else if(cfg.P < 0 || cfg.Q < 0) {
			PPError(PPERR_INVMODELPARAM);
			if(cfg.P < 0)
				dlg->selectCtrl(CTL_PREDICTCFG_P);
			else
				dlg->selectCtrl(CTL_PREDICTCFG_Q);
		}
		else if(cfg.MinP < 0 || cfg.MinP > cfg.P)
			PPErrorByDialog(dlg, CTL_PREDICTCFG_PMIN, PPERR_INVPREDICT_PMIN);
		else if(cfg.StartDate && !checkdate(&cfg.StartDate))
			PPErrorByDialog(dlg, CTL_PREDICTCFG_DTBEGSELL, PPERR_SLIB);
		else {
			dlg->getCtrlData(CTL_PREDICTCFG_DTENDCALC, &cfg.EndCalcDate);
			if(!checkdate(cfg.EndCalcDate, 1))
				PPErrorByDialog(dlg, CTL_PREDICTCFG_DTENDCALC, PPERR_SLIB);
			else if(PutPredictCfg(&cfg, 1))
				ok = valid_data = 1;
			else
				ok = PPErrorZ();
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

SLAPI PrcssrPrediction::PrcssrPrediction() : Predictor()
{
	P_BObj = BillObj;
	P_IterQuery = 0;
	MaxTime = 0;
	if(!Cfg.StartDate)
		P_BObj->P_Tbl->GetFirstDate(Cfg.OpID, &Cfg.StartDate);
}

SLAPI PrcssrPrediction::~PrcssrPrediction()
{
	delete P_IterQuery;
}

int SLAPI PrcssrPrediction::InitParam(Param * pParam)
{
	memzero(pParam, sizeof(Param));
	DateRange p;
	pParam->SetPeriod(p.Set(Cfg.StartDate, ZERODATE));
	pParam->Process = (Param::prcsFillSales | Param::prcsFillHoles | Param::prcsFillModel);
	return 1;
}

int SLAPI PrcssrPrediction::GetLastUpdate(PPID goodsID, const ObjIdListFilt & rLocList, LDATE * pLastDate)
{
	return T.GetLastUpdate(goodsID, rLocList, pLastDate);
}
//
//
//
#define GRP_GOODS 1

class PSalesTestParamDialog : public TDialog {
public:
	PSalesTestParamDialog(PPPredictConfig * pCfg) : TDialog(DLG_PSALESTEST)
	{
		if(pCfg)
			Cfg = *pCfg;
		else
			MEMSZERO(Cfg);
		addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_PSALTST_GGROUP, CTLSEL_PSALTST_GOODS));
	}
	int    setDTS(const PrcssrPrediction::Param * pData)
	{
		int    ok = 1;
		long   mode = 0;
    	if(pData)
			Data = *pData;
		else
			MEMSZERO(Data);
		Data.Process = PrcssrPrediction::Param::prcsTest;
		SetupCalPeriod(CTLCAL_PSALTST_PERIOD, CTL_PSALTST_PERIOD);
		GoodsCtrlGroup::Rec rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODS, &rec);
		mode = (Data.Flags & PrcssrPrediction::Param::fTestUpdatedItems) ? 1 : 0;
		AddClusterAssoc(CTL_PSALTST_GOODSSELKIND, 0, 0);
		AddClusterAssoc(CTL_PSALTST_GOODSSELKIND, -1, 0);
		AddClusterAssoc(CTL_PSALTST_GOODSSELKIND, 1, 1);
		SetClusterData(CTL_PSALTST_GOODSSELKIND, mode);
		{
			DateRange period = Data.GetPeriod();
			SetPeriodInput(this, CTL_PSALTST_PERIOD, &period);
		}
		return ok;
	}
	int    getDTS(PrcssrPrediction::Param * pData)
	{
		int    ok = 1;
		long   mode = 0;
		Data.Process = PrcssrPrediction::Param::prcsTest;
		GoodsCtrlGroup::Rec rec;
		getGroupData(GRP_GOODS, &rec);
		Data.GoodsID = rec.GoodsID;
		Data.GoodsGrpID = rec.GrpID;
		GetClusterData(CTL_PSALTST_GOODSSELKIND, &mode);
		SETFLAG(Data.Flags, PrcssrPrediction::Param::fTestUpdatedItems, mode == 1);
		{
			DateRange period = Data.GetPeriod();
			GetPeriodInput(this, CTL_PSALTST_PERIOD, &period);
			Data.SetPeriod(period);
		}
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	PrcssrPrediction::Param Data;
	PPPredictConfig Cfg;
};

#define GRP_GOODS 1

class PredictionParamDialog : public TDialog {
public:
	PredictionParamDialog(PPPredictConfig * pCfg, PredictSalesCore * pSalesTbl) : TDialog(DLG_FILLSALESTBL)
	{
		if(!RVALUEPTR(Cfg, pCfg))
			MEMSZERO(Cfg);
		P_SalesTbl = pSalesTbl;
		PrevContinueMode = BIN(Cfg.Flags & PPPredictConfig::fContinueBuilding);
		SetCtrlBitmap(CTL_FILLSALESTBL_IMG, BM_FILLSALESTBL);
		addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_FILLSALESTBL_GRP, CTLSEL_FILLSALESTBL_GDS));
	}
	int    setDTS(const PrcssrPrediction::Param *);
	int    getDTS(PrcssrPrediction::Param *);
private:
	DECL_HANDLE_EVENT;
	void   DisableRecalcBegDt();

	PrcssrPrediction::Param Data;
	PPPredictConfig Cfg;
	PredictSalesCore * P_SalesTbl;
	int    PrevContinueMode;
};

IMPL_HANDLE_EVENT(PredictionParamDialog)
{
	TDialog::handleEvent(event);
	if(event.isClusterClk(CTL_FILLSALESTBL_UPDATE)) {
		GetClusterData(CTL_FILLSALESTBL_UPDATE, &Data.Replace);
		DisableRecalcBegDt();
	}
	else if(event.isCbSelected(CTLSEL_FILLSALESTBL_GDS)) {
		Data.GoodsID = getCtrlLong(CTLSEL_FILLSALESTBL_GDS);
		if(Data.GoodsID) {
			GetClusterData(CTL_FILLSALESTBL_UPDATE, &Data.Replace);
			if(Data.Replace == PrcssrPrediction::Param::rsRemoveAndReplaceAll) {
				Data.Replace = PrcssrPrediction::Param::rsUpdateOnly;
				SetClusterData(CTL_FILLSALESTBL_UPDATE, Data.Replace);
			}
			DisableRecalcBegDt();
		}
		DisableClusterItem(CTL_FILLSALESTBL_UPDATE, 1, Data.GoodsID);
	}
	else
		return;
	clearEvent(event);
}

void PredictionParamDialog::DisableRecalcBegDt()
{
	if(Data.Replace != PrcssrPrediction::Param::rsUpdateOnly)
		setCtrlDate(CTL_FILLSALESTBL_RCALBDT, ZERODATE);
	disableCtrl(CTL_FILLSALESTBL_RCALBDT, Data.Replace != PrcssrPrediction::Param::rsUpdateOnly);
}

int PredictionParamDialog::setDTS(const PrcssrPrediction::Param * pData)
{
	int    ok = 1;
	SString op_name;
	LDATE last_dt = ZERODATE;
	if(!RVALUEPTR(Data, pData))
		MEMSZERO(Data);
	DateRange period = Data.GetPeriod();
	THROW(GetOpName(Cfg.OpID, op_name) > 0);
	THROW(P_SalesTbl);
	P_SalesTbl->GetTblUpdateDt(&last_dt);

	setCtrlData(CTL_FILLSALESTBL_BEGDT,  &Cfg.StartDate);
	setCtrlData(CTL_FILLSALESTBL_LASTDT, &last_dt);
	setCtrlString(CTL_FILLSALESTBL_OPNAME, op_name);
	SETIFZ(period.upp, NZOR(Cfg.EndCalcDate, LConfig.OperDate));
	setCtrlData(CTL_FILLSALESTBL_RCALBDT, &period.low);
	setCtrlData(CTL_FILLSALESTBL_ENDDT, &period.upp);
	// @v7.7.2 SetupPPObjCombo(this, CTLSEL_FILLSALESTBL_GRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN, 0);
	// @v7.7.2 {
	{
		GoodsCtrlGroup::Rec ggrp_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(GRP_GOODS, &ggrp_rec);
	}
	{
		if(Data.GoodsID) {
			if(Data.Replace == PrcssrPrediction::Param::rsRemoveAndReplaceAll) {
				Data.Replace = PrcssrPrediction::Param::rsUpdateOnly;
				SetClusterData(CTL_FILLSALESTBL_UPDATE, Data.Replace);
			}
		}
		DisableClusterItem(CTL_FILLSALESTBL_UPDATE, 1, Data.GoodsID);
	}
	// } @v7.7.2
	AddClusterAssoc(CTL_FILLSALESTBL_UPDATE, 0, PrcssrPrediction::Param::rsUpdateOnly);
	AddClusterAssoc(CTL_FILLSALESTBL_UPDATE, -1, PrcssrPrediction::Param::rsUpdateOnly);
	AddClusterAssoc(CTL_FILLSALESTBL_UPDATE, 1, PrcssrPrediction::Param::rsRemoveAndReplaceAll);
	SetClusterData(CTL_FILLSALESTBL_UPDATE, Data.Replace);
	AddClusterAssoc(CTL_FILLSALESTBL_CONT, 0, PrcssrPrediction::Param::fRemoveContinueMode);
	SetClusterData(CTL_FILLSALESTBL_CONT, Data.Flags);
	// @v8.0.10 {
	AddClusterAssoc(CTL_FILLSALESTBL_FLAGS, 0, PrcssrPrediction::Param::fUsePPViewGoodsRest);
	SetClusterData(CTL_FILLSALESTBL_FLAGS, Data.Flags);
	// } @v8.0.10
	if(PrevContinueMode) {
		SString msg_buf;
		PPLoadText(PPTXT_PSALES_CONTINUEMODE, msg_buf);
		setStaticText(CTL_FILLSALESTBL_ST_CONT, msg_buf);
	}
	disableCtrls(1, CTL_FILLSALESTBL_BEGDT, CTL_FILLSALESTBL_LASTDT, CTL_FILLSALESTBL_OPNAME, 0);
	SETIFZ(Data.Replace, PrcssrPrediction::Param::rsUpdateOnly);
	DisableRecalcBegDt();
	CATCHZOK
	return ok;
}

int PredictionParamDialog::getDTS(PrcssrPrediction::Param * pData)
{
	ushort sel = 0;
	int    ok = 1;
	DateRange period = Data.GetPeriod();
	DateRange norm_period;
	norm_period.SetZero();
	LDATE  last_dt = ZERODATE;
	THROW(P_SalesTbl);
	P_SalesTbl->GetTblUpdateDt(&last_dt);
	// @v7.7.2 Data.GoodsGrpID = getCtrlLong(CTLSEL_FILLSALESTBL_GRP);
	// @v7.7.2 {
	{
		GoodsCtrlGroup::Rec ggrp_rec;
		getGroupData(GRP_GOODS, &ggrp_rec);
		Data.GoodsGrpID = ggrp_rec.GrpID;
		Data.GoodsID    = ggrp_rec.GoodsID;
	}
	// } @v7.7.2
	GetClusterData(CTL_FILLSALESTBL_UPDATE, &Data.Replace);
	{
		LDATE first_date = period.low;
		getCtrlData(sel = CTL_FILLSALESTBL_RCALBDT, &first_date);
		THROW_SL(checkdate(first_date, 1));
		if(first_date)
			period.low = first_date;
	}
	norm_period.low = period.low.getactual(ZERODATE);
	THROW_SL(checkdate(norm_period.low, 1));

	getCtrlData(sel = CTL_FILLSALESTBL_ENDDT, &period.upp);
	THROW_SL(checkdate(period.upp, 1));
	norm_period.upp = period.upp.getactual(ZERODATE);
	THROW_SL(checkdate(norm_period.upp, 1));

	THROW_SL(norm_period.InvariantC(0));
	THROW_PP(norm_period.upp > norm_period.low, PPERR_PSBUILDDATELTLAST);
	SETFLAG(Data.Flags, PrcssrPrediction::Param::fRecalcByPeriod, period.low);
	Data.SetPeriod(period);
	GetClusterData(CTL_FILLSALESTBL_CONT, &Data.Flags);
	GetClusterData(CTL_FILLSALESTBL_FLAGS, &Data.Flags); // @v8.0.10
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int SLAPI PrcssrPrediction::EditParam(Param * pParam)
{
	int    ok = -1;
	TDialog * p_dlg_ = 0;
	PPPredictConfig cfg;
	THROW_PP(PrcssrPrediction::GetPredictCfg(&cfg) > 0 && cfg.OpID > 0, PPERR_INVPREDICTOP);
	if(pParam && pParam->Process & pParam->prcsTest) {
		PSalesTestParamDialog * dlg = 0;
		THROW(CheckDialogPtr(&(dlg = new PSalesTestParamDialog(&cfg))));
		p_dlg_ = dlg;
		THROW(dlg->setDTS(pParam));
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(pParam))
				ok = 1;
	}
	else {
		PredictionParamDialog * dlg = 0;
		THROW(CheckDialogPtr(&(dlg = new PredictionParamDialog(&cfg, &T))));
		p_dlg_ = dlg;
		THROW(dlg->setDTS(pParam));
		while(ok < 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(pParam))
				ok = 1;
	}
	CATCHZOKPPERR
	delete p_dlg_;
	return ok;
}

// static
int SLAPI PrcssrPrediction::Lock(int unlock)
{
	int    ok = 1;
	if(!unlock) {
		PPID   mutex_id = 0;
		PPSyncItem sync_item;
		int    r = DS.GetSync().CreateMutex(LConfig.SessionID, PPOBJ_TSALESBUILD, 1L, &mutex_id, &sync_item);
		if(r < 0)
			ok = PPSetError(PPERR_PSALESBUILDISLOCKED, sync_item.Name);
		else if(r == 0)
			ok = 0;
	}
	else
		ok = DS.GetSync().ReleaseMutex(PPOBJ_TSALESBUILD, 1L);
	return ok;
}

// static
int SLAPI PrcssrPrediction::IsLocked()
{
	int    ok = -1, r;
	PPSync & r_sync = DS.GetSync();
	PPID   mutex_id = 0;
	PPSyncItem sync_item;
	if((r = r_sync.CreateMutex(LConfig.SessionID, PPOBJ_TSALESBUILD, 1L, &mutex_id, &sync_item)) > 0) {
		r_sync.ReleaseMutex(PPOBJ_TSALESBUILD, 1L);
		ok = 0;
	}
	else if(r < 0) {
		ok = (PPSetError(PPERR_PSALESBUILDISLOCKED, sync_item.Name), 1);
	}
	return ok;
}

int SLAPI PrcssrPrediction::Init(const Param * pParam)
{
	int    ok = 1;
	int    goods_quant = 0;
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_PSALES_GOODSQUANT, &goods_quant);
	for(int i = 15; i >= 0; i--) {
		if(goods_quant >= (1L << i)) {
			goods_quant = (1L << i);
			break;
		}
	}
	if(goods_quant <= 0)
		goods_quant = 512;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_PSALES_MAXTIME, &MaxTime);
	if(MaxTime < 0 || MaxTime > 24*60*60*7)
		MaxTime = 0;
	PPGetFilePath(PPPATH_BIN, "psalesstop.", StopFileName);
	P = *pParam;
	// @v7.6.9 {
	if(!P.GetPeriod().low) {
		DateRange temp_period = P.GetPeriod();
		temp_period.low = Cfg.StartDate;
		P.SetPeriod(temp_period);
	}
	// } @v7.6.9
	P.GoodsQuant = goods_quant;
	THROW_PP(Cfg.OpID > 0, PPERR_INVPREDICTOP);
	//
	// Определение списка видов операций продажи
	//
	OpList.freeAll();
	if(GetGenericOpList(Cfg.OpID, &OpList) <= 0)
		OpList.add(Cfg.OpID);
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPrediction::CheckInterruption()
{
	int    ok = 0;
	if(SLS.CheckStopFlag()) {
		PPLogMessage(PPFILNAM_PSALES_LOG, "Process interrupted by stop event", LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		ok = 1;
	}
	else {
		if(MaxTime > 0 && diffdatetimesec(getcurdatetime_(), TimerStart) >= MaxTime) {
			PPLogMessage(PPFILNAM_PSALES_LOG, "Process interrupted by time limit", LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			ok = 1;
		}
		else if(fileExists(StopFileName)) {
			PPLogMessage(PPFILNAM_PSALES_LOG, "Process interrupted by stop file", LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			SFile::Remove(StopFileName);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PrcssrPrediction::Run()
{
	const  int use_ta = 1;
	const  long preserve_replace_param = P.Replace;

	int    ok = 1, r;
	int    is_locked = 0;
	int    break_process = 0;
	int    do_update_common_last_date = 0;
	long   goods_count = 0, goods_processed = 0;
	long   last_goods_id = 0; // Товар, на котором было оборвано последнее построение
	MainIterCount = 0;
	PPIDArray goods_list;
	//TSArray <_GoodsLocRestItem> * p_rest_list = 0;
	_MassGoodsRestBlock * p_rest_blk = 0;
	SString msg_buf, fmt_buf, rest_time_buf, process_fmt_buf, ta_msg_buf;
	Goods2Tbl::Rec goods_rec;
	//
	// Создаем объект PPObjGoods для того, чтобы исключить дорогостоящие
	// открытия таблицы goods2 в итерациях
	//
	PPObjGoods goods_obj;
	MEMSZERO(Stat);
	TimerStart = getcurdatetime_();
	PROFILE_START
	THROW(Lock(0));
	is_locked = 1;
	PPLoadText(PPTXT_PSALESPROCESS, msg_buf);
	PPWaitMsg(msg_buf);
	if(P.Process == Param::prcsTest) {
		LDATE last_dt = ZERODATE;
		DateRange period = P.GetPeriod();
		THROW(T.GetTblUpdateDt(&last_dt));
		period.Actualize(ZERODATE);
		if(!period.upp || period.upp > last_dt)
			period.upp = last_dt;
		if(!period.low)
			period.low = plusdate(period.upp, -1);
		THROW_PP_S(period.upp >= period.low, PPERR_INVPERIOD, (msg_buf = 0).Cat(period, 0));
		P.SetPeriod(period);
		if(P.Flags & Param::fTestUpdatedItems) {
			LDATETIME since;
			since.d = period.low;
			since.t = ZEROTIME;
			THROW(BillObj->GetGoodsListByUpdatedBills(0, since, goods_list));
			if(P.GoodsGrpID) {
				PPIDArray temp_goods_list;
				THROW(GoodsIterator::GetListByGroup(P.GoodsGrpID, &temp_goods_list));
				goods_list.intersect(&temp_goods_list, 0);
			}
		}
		else {
			if(P.GoodsID)
				goods_list.add(P.GoodsID);
			else if(P.GoodsGrpID) {
				THROW(GoodsIterator::GetListByGroup(P.GoodsGrpID, &goods_list));
			}
			else {
				CALLEXCEPT_PP(PPERR_UNABLETESTPSALESBYALLGOODS);
			}
		}
		{
			PPWaitMsg(PPSTR_TEXT, PPTXT_SCANHOLIDAYS, 0);
			T.SaveHolidays();
			DateRange init_period;
			P_BObj->P_Tbl->ScanHolidays(0, Cfg.OpID, &init_period.Set(period.low, period.upp), &T);
		}
		THROW(ProcessGoodsList(goods_list, p_rest_blk, 0, use_ta));
	}
	else {
		if(P.Replace == Param::rsRemoveAndReplaceAll) {
			PPLoadText(PPTXT_LOG_PSALES_REBUILD, fmt_buf);
			PPFormat(fmt_buf, &msg_buf, P.GetNormPeriod());
			PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			THROW(DeleteAllItems());
		}
		else {
			THROW(T.CheckTableStruct());
			if(!P.GoodsID) {
				if(!(P.Flags & Param::fRemoveContinueMode) && GetContinueMode(0, &last_goods_id) > 0) {
					PPLoadText(PPTXT_LOG_PSALES_BUILDCONT, fmt_buf);
					PPFormat(fmt_buf, &msg_buf, P.GetNormPeriod(), last_goods_id);
				}
				else {
					PPLoadText(PPTXT_LOG_PSALES_BUILD, fmt_buf);
					PPFormat(fmt_buf, &msg_buf, P.GetNormPeriod());
				}
				PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
		{
			PPWaitMsg(PPSTR_TEXT, PPTXT_SCANHOLIDAYS, 0);
			T.SaveHolidays();
			DateRange init_period;
			P_BObj->P_Tbl->ScanHolidays(0, Cfg.OpID, &init_period.Set(Cfg.StartDate, P.GetNormPeriod().upp), &T);
		}
		PPLoadText(PPTXT_LOG_PSALESPROCESS, process_fmt_buf);
		PPLoadText(PPTXT_LOG_PSALESRETRANSACT, ta_msg_buf);
		if(P.GoodsID) {
			if(goods_obj.Fetch(P.GoodsID, &goods_rec) > 0 && !(goods_rec.Flags & GF_GENERIC)) {
				P.Replace = Param::rsReplaceExistance; // @v8.1.2
				goods_list.addUnique(P.GoodsID);
				goods_count = 1;
				THROW(ProcessGoodsList(goods_list, p_rest_blk, 1, use_ta));
				goods_processed = 1;
				Stat.GoodsCount++;
			}
		}
		else {
			const uint goods_chunk_size = P.GoodsQuant;
			uint  chunk_count = 0;
			Stat.GoodsQuant = goods_chunk_size;
			PPIDArray total_goods_list;
			//
			// Важно, чтобы товары перебирались в порядке следования идентификаторов
			//
			for(GoodsIterator giter(P.GoodsGrpID, 0); giter.Next(&goods_rec) > 0;) {
				if(!(goods_rec.Flags & GF_GENERIC)) {
					const PPID _goods_id = goods_rec.ID;
					if(_goods_id > last_goods_id)
						total_goods_list.add(_goods_id);
				}
			}
			goods_count = total_goods_list.getCount();
			if(P.Flags & Param::fUsePPViewGoodsRest) {
				const ObjIdListFilt empty_loc_list;
				THROW_MEM(p_rest_blk = new _MassGoodsRestBlock);
				p_rest_blk->Period = P.GetNormPeriod();
				if(P.Replace == Param::rsUpdateOnly) {
					for(uint j = 0; j < total_goods_list.getCount(); j++) {
						const  PPID goods_id = total_goods_list.get(j);
						LDATE  last_date = ZERODATE;
						if(T.GetLastUpdate(goods_id, empty_loc_list, &last_date) > 0 && last_date) {
							last_date = plusdate(last_date, 1);
							SETMIN(p_rest_blk->Period.low, last_date);
						}
					}
				}
				const LDATE rest_date = p_rest_blk->Period.low ? plusdate(p_rest_blk->Period.low, -1) : encodedate(1, 1, 1900);

				GoodsRestFilt gr_filt;
				PPViewGoodsRest gr_view;
				gr_filt.GoodsGrpID = P.GoodsGrpID;
				gr_filt.Date = rest_date;
				gr_filt.Flags |= GoodsRestFilt::fEachLocation;
				THROW(gr_view.Init_(&gr_filt));
				{
					GoodsRestViewItem gr_item;
					for(gr_view.InitIteration(); gr_view.NextIteration(&gr_item) > 0;) {
						_GoodsLocRestItem item;
						item.GoodsID = gr_item.GoodsID;
						item.LocID = gr_item.LocID;
						item.Rest = gr_item.Rest;
						THROW_SL(p_rest_blk->List.insert(&item));
					}
					p_rest_blk->List.sort(PTR_CMPFUNC(_2long));
				}
			}
			for(uint gi = 0; !break_process && gi < total_goods_list.getCount(); gi++) {
				++goods_processed;
				const PPID _goods_id = total_goods_list.get(gi);
				goods_list.add(_goods_id); // @v8.0.10 addUnique-->add
				if(goods_list.getCount() >= goods_chunk_size) {
					THROW(r = ProcessGoodsList(goods_list, p_rest_blk, 0, use_ta));
					if(r > 0) {
						Stat.GoodsCount += goods_list.getCount();
						goods_list.clear();
						++chunk_count;
						Stat.Time = diffdatetimesec(getcurdatetime_(), TimerStart);
						long   total = (long)((double)goods_count * (double)Stat.Time / (double)goods_processed);
						long   pct   = goods_count ? ((100L * goods_processed) / goods_count) : 100L;
						LTIME  rest_time;
						rest_time.settotalsec(total - Stat.Time);
						(rest_time_buf = 0).Cat(rest_time);
						(msg_buf = 0).Cat(pct).CatChar('%').Space().CatParStr(rest_time_buf);
						msg_buf.Printf(process_fmt_buf, pct, rest_time_buf.cptr());
						PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
						PPWaitMsg(msg_buf);
						if(CheckInterruption())
							break_process = 1;
					}
					else
						break_process = 1;
				}
			}
			if(!break_process) {
				THROW(r = ProcessGoodsList(goods_list, p_rest_blk, 0, use_ta));
				if(r > 0) {
					Stat.GoodsCount += goods_list.getCount();
					if(!P.GoodsGrpID)
						do_update_common_last_date = 1;
					THROW(RecalcStat(P.GetNormPeriod().upp, &Stat.Sse, 1)); // @v8.2.6
				}
				else
					break_process = 1;
			}
		}
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(!break_process) {
				if(do_update_common_last_date) {
					LDATE  last_dt = ZERODATE;
					THROW(T.GetTblUpdateDt(&last_dt));
					THROW(T.SetTblUpdateDt((P.GetNormPeriod().upp > last_dt) ? P.GetNormPeriod().upp : last_dt));
				}
				T.RestoreHolidays();
				THROW(T.Finish(0, 0));
				//
				// Процесс успешно завершен - снимаем признак режима продолжения построения в конфигурации.
				//
				THROW(SetContinueMode(1, 0, 0));
			}
			else {
				T.RestoreHolidays();
				THROW(T.Finish(0, 0));
			}
			THROW(tra.Commit());
		}
		if(goods_processed) {
			Stat.Time  = diffdatetimesec(getcurdatetime_(), TimerStart);
			long   total = (long)((double)goods_count * (double)Stat.Time / (double)goods_processed);
			long   pct   = goods_count ? ((100L * goods_processed) / goods_count) : 100L;
			LTIME  rest_time;
			rest_time.settotalsec(total - Stat.Time);
			(rest_time_buf = 0).Cat(rest_time);
			(msg_buf = 0).Cat(pct).CatChar('%').Space().CatParStr(rest_time_buf);
			msg_buf.Printf(process_fmt_buf, pct, rest_time_buf.cptr());
			PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			PPWaitMsg(msg_buf);
			{
				PPIDArray loc_list;
				T.GetLocList(loc_list);
				Stat.LocCount = loc_list.getCount();
				Stat.DayCount = diffdate(P.GetNormPeriod().upp, P.GetNormPeriod().low);
				Stat.AvgTimeG = fdivnz(Stat.Time, Stat.GoodsCount);
				Stat.AvgTimeGD = fdivnz(Stat.Time, Stat.GoodsCount * Stat.DayCount);
				Stat.AvgTimeGL = fdivnz(Stat.Time, Stat.GoodsCount * Stat.LocCount);
				Stat.AvgTimeGDL = fdivnz(Stat.Time, Stat.GoodsCount * Stat.DayCount * Stat.LocCount);
				(msg_buf = 0).Cat("Stat").CatDiv(':', 2).
					CatEq("Time", Stat.Time).CatDiv(';', 0).
					CatEq("DayCount", Stat.DayCount).CatDiv(';', 0).
					CatEq("LocCount", Stat.LocCount).CatDiv(';', 0).
					CatEq("GoodsCount", Stat.GoodsCount).CatDiv(';', 0).
					CatEq("GoodsQuant", Stat.GoodsQuant).CatDiv(';', 0).
					CatEq("AvgTimeG", Stat.AvgTimeG, MKSFMTD(0, 8, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("AvgTimeGD", Stat.AvgTimeGD, MKSFMTD(0, 8, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("AvgTimeGL", Stat.AvgTimeGL, MKSFMTD(0, 8, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("AvgTimeGDL", Stat.AvgTimeGDL, MKSFMTD(0, 8, NMBF_NOTRAILZ));
				PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
				(msg_buf = 0).Cat("StoreErr").CatDiv(':', 2).
					CatEq("Count", Stat.Sse.Count).CatDiv(';', 0).
					CatEq("QttySum", Stat.Sse.QttySum, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("QttySqSum", Stat.Sse.QttySqSum, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("AmtSum", Stat.Sse.AmtSum, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CatDiv(';', 0).
					CatEq("AmtSqSum", Stat.Sse.AmtSqSum, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				PPLogMessage(PPFILNAM_PSALES_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
			}
		}
	}
	CATCHZOK
	ZDELETE(p_rest_blk);
	if(is_locked)
		Lock(1);
	PROFILE_END
	return ok;
}

int SLAPI PrcssrPrediction::StoreStatByGoodsList(const PPIDArray & rGoodsList, LDATE commonLastDate, PredictSalesCore::StatStoreErr * pErr, int use_ta)
{
	//
	// Расчет и сохранение статистики по товарам разнесено для ускорения //
	// за счет разделения операций чтения из одной таблицы и записи в другую
	//
	int    ok = 1;
	uint   i, j;
	int    break_process = 0;
	int    show_wait_msg = CS_SERVER ? 0 : 1;
	PredictSalesStat * p_stat_list = 0;
	//
	// Рассчитываем статистику по товарам
	//
	PPIDArray loc_list;
	PROFILE_START
	T.GetLocList(loc_list);
	const uint loc_count = loc_list.getCount();
	const uint goods_count = rGoodsList.getCount();
	const uint c = loc_count * goods_count;
	THROW_MEM(p_stat_list = new PredictSalesStat[loc_count * goods_count]);
#if 1 // {
	//
	// Не смотря на старания этот вариант расчета получился крайне медленным
	// Видимо, тормозит выборка записей по условию (>= low_goods_id && <= upp_goods_id)
	//
	// @v8.1.8 Все таки, 'тот вариант оказался более производительным. Возможно, дело в том,
	// что теперь мы его вывели из под действия транзакции.
	//
	for(i = 0; i < loc_count; i++) {
		const  PPID  loc_id  = loc_list.at(i);
		const  PPID  low_goods_id = rGoodsList.get(0);
		const  PPID  upp_goods_id = rGoodsList.getLast();
		const  uint  loc_idx_in_stat_list = i * goods_count;
		int16  loc_idx = 0;
		T.ShrinkLoc(loc_id, &loc_idx);
		ObjIdListFilt loc_list2;
		loc_list2.Add(loc_id, 0);
		PredictSalesTbl::Key0 k_init, k;
		T.SetKey(&k_init, PSRECTYPE_DAY, loc_id, low_goods_id, ZERODATE);
		k_init.Dt = 0;
		//
		// Инициализируем статистику по товарам
		//
		for(j = 0; j < goods_count; j++) {
			PredictSalesStat * p_pss = &(p_stat_list[loc_idx_in_stat_list + j]);
			p_pss->Init();
			p_pss->LocID = loc_id;
			p_pss->GoodsID = rGoodsList.get(j);
		}
		//
		// Заполняем статистику по товарам
		//
		if(loc_id && goods_count) {
			BExtQuery q(&T, 0, 1024); // @v8.1.7 128-->1024
			DBQ * dbq = & (T.RType == (long)PSRECTYPE_DAY && T.Loc == (long)loc_idx &&
				T.GoodsID >= low_goods_id && T.GoodsID <= upp_goods_id+1);
			q.select(T.GoodsID, T.Dt, T.Quantity, T.Amount, 0L).where(*dbq);
			PPID   prev_goods_id = 0;
			uint   prev_goods_pos = 0;
			for(q.initIteration(0, &(k = k_init), spGe); q.nextIteration() > 0;) {
				PredictSalesStat * p_stat_entry = 0;
				if(prev_goods_id && T.data.GoodsID == prev_goods_id)
					p_stat_entry = &(p_stat_list[prev_goods_pos]);
				else if(T.data.GoodsID > upp_goods_id)
					break;
				else {
					for(j = loc_idx_in_stat_list; j < loc_idx_in_stat_list+goods_count; j++) {
						if(p_stat_list[j].GoodsID == T.data.GoodsID) {
							prev_goods_id = T.data.GoodsID;
							prev_goods_pos = j;
							p_stat_entry = &(p_stat_list[prev_goods_pos]);
							if(show_wait_msg)
								PPWaitPercent(prev_goods_pos + 1, c, "Calculating of total statistic");
							break;
						}
					}
				}
				if(p_stat_entry) {
					PredictSalesItem item;
					MEMSZERO(item);
					T.ExpandDate(T.data.Dt, &item.Dt);
					item.Qtty = T.data.Quantity;
					item.Amount = T.data.Amount;
					THROW(p_stat_entry->Step(&item));
				}
			}
		}
		//
		// Финишируем статистику по товарам
		//
		for(j = 0; j < goods_count; j++) {
			LDATE  last_dt = ZERODATE;
			PredictSalesStat * p_pss = &(p_stat_list[loc_idx_in_stat_list + j]);
			p_pss->Finish();
			THROW(GetLastUpdate(rGoodsList.get(j), loc_list2, &last_dt));
			p_pss->LastDate = MAX(commonLastDate, last_dt);
		}
	}
#else // }{
	for(i = 0; !break_process && i < loc_count; i++) {
		PPID   loc_id  = loc_list.at(i);
		ObjIdListFilt loc_list2;
		loc_list2.Add(loc_id, 0);
		for(j = 0; !break_process && j < goods_count; j++) {
			const  PPID goods_id = rGoodsList.get(j);
			LDATE  last_dt = ZERODATE;
			PredictSalesStat * p_pss = &(p_stat_list[i * goods_count + j]);
			THROW(T.CalcStat(goods_id, loc_list2, 0, 0, p_pss));
			THROW(T.GetLastUpdate(goods_id, loc_list2, &last_dt));
			p_pss->LastDate = MAX(commonLastDate, last_dt);
			if((j % 100) == 0) {
				if(show_wait_msg)
					PPWaitPercent(i * goods_count + j + 1, c, "Calculating of total statistic");
				if(CheckInterruption())
					break_process = 1;
			}
		}
	}
#endif // }
	if(!break_process) {
		PPTransaction tra(use_ta);
		THROW(tra);
		//
		// Массово удаляем всю статистику по списку товаров
		// для того, чтобы потом можно было спокойно использовать
		// массовую вставку.
		//
		for(j = 0; !break_process && j < goods_count; j++) {
			const PPID goods_id = rGoodsList.get(j);
			// @v8.0.11 {
			{
				GoodsStatTbl::Key1 sk1;
				sk1.GoodsID = goods_id;
				sk1.Loc = 0;
				if(T.StT.search(1, &sk1, spGe) && T.StT.data.GoodsID == goods_id) do {
					THROW_DB(T.StT.deleteRec());
				} while(T.StT.search(1, &sk1, spNext) && T.StT.data.GoodsID == goods_id);
			}
			// } @v8.0.11
			// @v8.0.11 THROW_DB(deleteFrom(&T.StT, 0, T.StT.GoodsID == goods_id));
			if(show_wait_msg)
				PPWaitPercent(j+1, goods_count, "Removing of old total statistic");
			if(CheckInterruption())
				break_process = 1;
		}
		if(!break_process) {
			//
			// Теперь массово добавляем всю статистику по товарам в таблицу StT
			//
			const uint c = loc_count * goods_count;
			BExtInsert bei(&T.StT);
			for(i = 0; !break_process && i < c; i++) {
				const PredictSalesStat * p_pss = &(p_stat_list[i]);
				GoodsStatTbl::Rec new_rec;
				MEMSZERO(new_rec);
				T.ShrinkLoc(p_pss->LocID, &new_rec.Loc);
				T.ShrinkDate(p_pss->LastDate, &new_rec.LastDate);
				new_rec.GoodsID   = p_pss->GoodsID;
				new_rec.Count     = (short)p_pss->Count;
				new_rec.QttySum   = (float)p_pss->QttySum;
				new_rec.QttySqSum = (float)p_pss->QttySqSum;
				new_rec.AmtSum    = (float)p_pss->AmtSum;
				new_rec.AmtSqSum  = (float)p_pss->AmtSqSum;
				if(pErr) {
					SETMAX(pErr->Count,     labs((long)new_rec.Count      - p_pss->Count));
					SETMAX(pErr->QttySum,   fabs((double)new_rec.QttySum  - p_pss->QttySum));
					SETMAX(pErr->QttySqSum, fabs((double)new_rec.QttySum  - p_pss->QttySum));
					SETMAX(pErr->AmtSum,    fabs((double)new_rec.AmtSum   - p_pss->AmtSum));
					SETMAX(pErr->AmtSqSum,  fabs((double)new_rec.AmtSqSum - p_pss->AmtSqSum));
				}
				THROW_DB(bei.insert(&new_rec));
				if(show_wait_msg)
					PPWaitPercent(i+1, c, "Inserting of new total statistic");
				if((i % 1000) == 0 && CheckInterruption())
					break_process = 1;
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	if(break_process)
		ok = -1;
	CATCHZOK
	PROFILE_END
	delete [] p_stat_list;
	return ok;
}

#if 1 // @construction {

int SLAPI PrcssrPrediction::FlashStat(PredictSalesStat * pList, uint count, PredictSalesCore::StatStoreErr * pErr, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < count; i++) {
			const  PredictSalesStat * p_pss = pList+i;
			int16  loc_idx = 0;
			T.ShrinkLoc(p_pss->LocID, &loc_idx);
			GoodsStatTbl::Key1 sk1;
			sk1.GoodsID = p_pss->GoodsID;
			sk1.Loc = loc_idx;
			if(T.StT.searchForUpdate(1, &sk1, spEq)) {
				assert(T.StT.data.Loc == loc_idx);
				assert(T.StT.data.GoodsID == p_pss->GoodsID);
				T.ShrinkDate(p_pss->LastDate, &T.StT.data.LastDate);
				T.StT.data.Count     = (short)p_pss->Count;
				T.StT.data.QttySum   = (float)p_pss->QttySum;
				T.StT.data.QttySqSum = (float)p_pss->QttySqSum;
				T.StT.data.AmtSum    = (float)p_pss->AmtSum;
				T.StT.data.AmtSqSum  = (float)p_pss->AmtSqSum;
				if(pErr) {
					SETMAX(pErr->Count,     labs((long)T.StT.data.Count      - p_pss->Count));
					SETMAX(pErr->QttySum,   fabs((double)T.StT.data.QttySum  - p_pss->QttySum));
					SETMAX(pErr->QttySqSum, fabs((double)T.StT.data.QttySum  - p_pss->QttySum));
					SETMAX(pErr->AmtSum,    fabs((double)T.StT.data.AmtSum   - p_pss->AmtSum));
					SETMAX(pErr->AmtSqSum,  fabs((double)T.StT.data.AmtSqSum - p_pss->AmtSqSum));
				}
				THROW_DB(T.StT.updateRec());
			}
			else {
				GoodsStatTbl::Rec new_rec;
				MEMSZERO(new_rec);
				new_rec.Loc = loc_idx;
				T.ShrinkDate(p_pss->LastDate, &new_rec.LastDate);
				new_rec.GoodsID   = p_pss->GoodsID;
				new_rec.Count     = (short)p_pss->Count;
				new_rec.QttySum   = (float)p_pss->QttySum;
				new_rec.QttySqSum = (float)p_pss->QttySqSum;
				new_rec.AmtSum    = (float)p_pss->AmtSum;
				new_rec.AmtSqSum  = (float)p_pss->AmtSqSum;
				if(pErr) {
					SETMAX(pErr->Count,     labs((long)new_rec.Count      - p_pss->Count));
					SETMAX(pErr->QttySum,   fabs((double)new_rec.QttySum  - p_pss->QttySum));
					SETMAX(pErr->QttySqSum, fabs((double)new_rec.QttySum  - p_pss->QttySum));
					SETMAX(pErr->AmtSum,    fabs((double)new_rec.AmtSum   - p_pss->AmtSum));
					SETMAX(pErr->AmtSqSum,  fabs((double)new_rec.AmtSqSum - p_pss->AmtSqSum));
				}
				THROW_DB(T.StT.insertRecBuf(&new_rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPrediction::RecalcStat(LDATE commonLastDate, PredictSalesCore::StatStoreErr * pErr, int use_ta)
{
	const uint max_stat_entries = 128 * 1024;
	int    ok = 1, ta = 0;
	int    break_process = 0;
	int    show_wait_msg = CS_SERVER ? 0 : 1;
	int    stat_entry_idx = -1;
	PredictSalesStat * p_stat_list = 0;
	PROFILE_START
	THROW_MEM(p_stat_list = new PredictSalesStat[max_stat_entries]);
	{
		//
		// Инициализируем статистику по товарам
		//
		for(uint j = 0; j < max_stat_entries; j++) {
			PredictSalesStat * p_pss = p_stat_list+j;
			p_pss->Init();
		}
		//
		// Заполняем статистику по товарам
		//
		{
			PredictSalesTbl::Key0 k_init, k;
			MEMSZERO(k_init);
			k_init.RType = PSRECTYPE_DAY;
			BExtQuery q(&T, 0, 1024);
			DBQ * dbq = & (T.RType == (long)PSRECTYPE_DAY);
			q.select(T.Loc, T.GoodsID, T.Dt, T.Quantity, T.Amount, 0L).where(*dbq);
			int16  prev_loc_idx = 0;
			PPID   loc_id = 0;
			PPID   prev_goods_id = 0;
			uint   prev_goods_pos = 0;
			for(q.initIteration(0, &(k = k_init), spGe); q.nextIteration() > 0;) {
				const int16 loc_idx = T.data.Loc;
				const PPID  goods_id = T.data.GoodsID;
				LDATE  item_dt;
				const double item_qtty = T.data.Quantity;
				const double item_amt  = T.data.Amount;
				T.ExpandDate(T.data.Dt, &item_dt);
				if(prev_loc_idx != loc_idx || prev_goods_id != goods_id) {
					if(stat_entry_idx >= 0) {
						p_stat_list[stat_entry_idx].Finish();
						p_stat_list[stat_entry_idx].LastDate = commonLastDate;
					}
					stat_entry_idx++;
					if(stat_entry_idx >= max_stat_entries) {
						THROW(FlashStat(p_stat_list, stat_entry_idx, pErr, 1));
						stat_entry_idx = 0;
					}
					p_stat_list[stat_entry_idx].Init();
					if(prev_loc_idx != loc_idx)
						T.ExpandLoc(loc_idx, &loc_id);
					p_stat_list[stat_entry_idx].LocID = loc_id;
					p_stat_list[stat_entry_idx].GoodsID = goods_id;
				}
				{
					PredictSalesItem item;
					MEMSZERO(item);
					item.Dt = item_dt;
					item.Qtty = item_qtty;
					item.Amount = item_amt;
					THROW(p_stat_list[stat_entry_idx].Step(&item));
				}
				prev_loc_idx = loc_idx;
				prev_goods_id = goods_id;
			}
			if(stat_entry_idx >= 0) {
				p_stat_list[stat_entry_idx].Finish();
				p_stat_list[stat_entry_idx].LastDate = commonLastDate;
				THROW(FlashStat(p_stat_list, stat_entry_idx+1, pErr, 1));
			}
		}
	}
	CATCHZOK
	PROFILE_END
	delete [] p_stat_list;
	return ok;
}

#endif // } 0 @construction

struct __CI {
	PPID   GoodsID;
	LDATE  LastDate; // Дата, до которой была рассчитана статистика до текущего расчета
	LocValList Lvl;
};

struct __HI {
	int16  LocIdx;
	LDATE  Dt;
};

static IMPL_CMPFUNC(__HI, i1, i2)
{
	int    si = 0;
	CMPCASCADE2(si, (__HI*)i1, (__HI*)i2, LocIdx, Dt);
	return si;
}

class __HolidayArray : public TSArray <__HI> {
public:
	SLAPI  __HolidayArray(const PredictSalesCore & rT, const DateRange & rPeriod) : TSArray <__HI>(), T(rT)
	{
		setDelta(32);
		Period = rPeriod;
	}
	int    SLAPI Is(int16 locIdx, LDATE dt);
private:
	DateRange Period;
	const PredictSalesCore & T;
	BitArray LocIdxList;
};

int SLAPI __HolidayArray::Is(int16 locIdx, LDATE dt)
{
	while((uint)locIdx >= LocIdxList.getCount())
		LocIdxList.insertN(0, 1024);
	if(!LocIdxList[locIdx]) {
		for(LDATE d = Period.low; d <= Period.upp; d = plusdate(d, 1)) {
			if(T.IsHolidayByLocIdx(locIdx, d)) {
				__HI item;
				item.LocIdx = locIdx;
				item.Dt = d;
				insert(&item);
			}
		}
		LocIdxList.set(locIdx, 1);
		sort(PTR_CMPFUNC(__HI));
	}
	__HI key;
	key.LocIdx = locIdx;
	key.Dt = dt;
	return bsearch(&key, 0, PTR_CMPFUNC(__HI));
}

int SLAPI PrcssrPrediction::ProcessGoodsList(PPIDArray & rGoodsList, const _MassGoodsRestBlock * pRestBlk, int calcStat, int use_ta)
{
	PPUserFuncProfiler ufp((P.Process == P.prcsTest) ? PPUPRF_PSALBLDGOODSTEST : PPUPRF_PSALBLDGOODS); // @v8.1.2

	int    ok = 1, /*ta = 0,*/ r;
	int    break_process = 0;
	uint   j;
	SArray * p_vect = 0;
	TSCollection <__CI> lvl_list;
	DateRange comm_period = P.GetNormPeriod();
	SString msg_fmt, msg, goods_name, period_buf, temp_buf;
	int    show_wait_msg = CS_SERVER ? 0 : 1;
	PPLogger * p_logger = 0;
	//
	// В дальнейшем мы закладываемся на то, что список товаров отсортирован
	//
	rGoodsList.sortAndUndup();
	PROFILE_START
	const ObjIdListFilt empty_loc_list;
	THROW_MEM(p_vect = new SArray(sizeof(PredictSalesTbl::Rec), 1024, O_ARRAY));
	//
	// Формируем список lvl_list, содержащий структуры, необходимые для заполнения таблицы
	//
	const uint c = rGoodsList.getCount();
	ufp.SetFactor(0, (double)c); // @v8.1.2
	if(pRestBlk) {
		comm_period = pRestBlk->Period;
	}
	for(j = 0; j < c; j++) {
		const  PPID goods_id = rGoodsList.get(j);
		__CI * p_ci = new __CI;
		p_ci->GoodsID = goods_id;
		if(P.Replace == Param::rsUpdateOnly) {
			LDATE  last_date = ZERODATE;
			T.GetLastUpdate(goods_id, empty_loc_list, &last_date);
			p_ci->LastDate = last_date;
			if(last_date && !pRestBlk) {
				last_date = plusdate(last_date, 1);
				SETMIN(comm_period.low, last_date);
			}
		}
		else
			p_ci->LastDate = ZERODATE;
		lvl_list.insert(p_ci);
		if(show_wait_msg)
			PPWaitPercent(j+1, c, "Initializing goods chunk");
	}
	PROFILE_END
	{
		PROFILE_START
		//
		// Рассчитываем начальные остатки
		//
		LDATE rest_date = comm_period.low ? plusdate(comm_period.low, -1) : encodedate(1, 1, 1900);
		const uint lvl_count = lvl_list.getCount();
		for(j = 0; !break_process && j < lvl_count; j++) {
			const  PPID goods_id = lvl_list.at(j)->GoodsID;
			if(pRestBlk) {
				lvl_list.at(j)->Lvl.Setup(comm_period.low, goods_id, &pRestBlk->List);
			}
			else {
				GoodsRestParam gp;
				gp.CalcMethod = GoodsRestParam::pcmSum;
				gp.GoodsID    = goods_id;
				gp.Date       = rest_date;
				gp.DiffParam |= GoodsRestParam::_diffLoc;
				THROW(P_BObj->trfr->GetRest(&gp));
				lvl_list.at(j)->Lvl.Setup(comm_period.low, &gp);
			}
			if(show_wait_msg)
				PPWaitPercent(j+1, lvl_count, "Calculating initial rest");
			if(CheckInterruption())
				break_process = 1;
		}
		PROFILE_END
	}
	if(!break_process) {
		PROFILE_START
		TransferTbl::Rec trfr_rec;
		BillTbl::Rec bill_rec;
		GCTFilt gct_filt;
		gct_filt.Period  = comm_period;
		gct_filt.GoodsList.Set(&rGoodsList);
		gct_filt.Flags |= (OPG_FORCEBILLCACHE | OPG_OPTIMIZEFORPSALES);
		GCTIterator gctiter(&gct_filt, &gct_filt.Period);
		PPID   last_goods_id = 0;
		uint   last_goods_pos = 0;
		__HolidayArray ha(T, comm_period);
		//
		// Основной цикл
		//
		const long _num_days = diffdate(comm_period.upp, comm_period.low)+1;
		ufp.SetFactor(1, (double)_num_days); // @v8.1.2
		int    _gctr = 0;
		PROFILE(_gctr = gctiter.First(&trfr_rec, &bill_rec));
		if(_gctr > 0) {
			do {
				PROFILE_START
				if(!(trfr_rec.Flags & (PPTFR_UNLIM|PPTFR_ACK))) { // @v9.5.8 @fix не учитываемые в остатке операции надо пропускать
					const PPID loc_id   = trfr_rec.LocID;
					const PPID goods_id = trfr_rec.GoodsID;
					if(!loc_id) {
						PPLoadText(PPTXT_LOG_PSALES_UNDEFLOC, msg_fmt);
						PPObjBill::MakeCodeString(&bill_rec, 1, temp_buf);
						msg.Printf(msg_fmt, GetGoodsName(goods_id, goods_name).cptr(), temp_buf.cptr());
						PPLogMessage(PPFILNAM_ERR_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
					}
					else {
						LocValList * p_lvl = 0;
						if(last_goods_id && goods_id == last_goods_id) {
							p_lvl = &lvl_list.at(last_goods_pos)->Lvl;
						}
						else if(lvl_list.bsearch(&goods_id, &(j = 0), CMPF_LONG)) {
							last_goods_id = goods_id;
							last_goods_pos = j;
							p_lvl = &lvl_list.at(last_goods_pos)->Lvl;
						}
						if(p_lvl) {
							LocValEntry & r_entry = *p_lvl->GetEntry(loc_id);
							const double qtty = trfr_rec.Quantity;
							if(trfr_rec.Dt != r_entry.LastOpDate && r_entry.LastOpDate)
								THROW(SetupGoodsSaleByLoc(r_entry, goods_id, trfr_rec.Dt, ha, p_vect));
							if(qtty < 0.0 && OpList.lsearch(bill_rec.OpID)) {
								r_entry.Sell += fabs(qtty);
								r_entry.SellAmt += fabs(qtty * (TR5(trfr_rec.Price)-TR5(trfr_rec.Discount)));
							}
							r_entry.LastOpDate = trfr_rec.Dt;
							r_entry.Rest      += qtty;
							r_entry.FirstIterTag = 1; // 0 - первая итерация, 1 - НЕ первая итерация //
						}
					}
				}
				++MainIterCount;
				if(show_wait_msg && (MainIterCount % 1024) == 0) {
					PPWaitPercent(diffdate(trfr_rec.Dt, comm_period.low)+1, _num_days, "Main loop of gathering data");
					//PPWaitPercent(last_goods_pos+1, lvl_list.getCount(), "Main loop of gathering data");
				}
				if((MainIterCount % 8*1024) == 0 && CheckInterruption())
					break_process = 1;
				PROFILE_END
				PROFILE(_gctr = gctiter.Next(&trfr_rec, &bill_rec));
			} while(!break_process && _gctr > 0);
		}
		{
			PROFILE_START
			const LDATE last_date = plusdate(comm_period.upp, 1);
			const uint lvl_count = lvl_list.getCount();
			for(j = 0; !break_process && j < lvl_count; j++) {
				__CI * p_ci = lvl_list.at(j);
				LocValEntry * p_entry;
				for(uint k = 0; p_ci->Lvl.enumItems(&k, (void **)&p_entry);)
					if(p_entry->LocID)
						THROW(SetupGoodsSaleByLoc(*p_entry, p_ci->GoodsID, last_date, ha, p_vect));
				if(show_wait_msg)
					PPWaitPercent(j+1, lvl_count, "Finalization main loop");
				if(CheckInterruption())
					break_process = 1;
			}
			PROFILE_END
		}
		PROFILE_END
	}
	if(!break_process) {
		PROFILE_START
		if(P.Process == P.prcsTest) {
			const uint vc = p_vect->getCount();
			enum {
				errQtty    = 0x0001,
				errAmt     = 0x0002,
				errFlags   = 0x0004,
				errAbsence = 0x8000
			};
			SString added_msg_buf;
			PPLoadText(PPTXT_PSALESTESTMSG, msg_fmt);
			for(j = 0; j < vc; j++) {
				const PredictSalesTbl::Rec * p_rec = (const PredictSalesTbl::Rec *)p_vect->at(j);
				PredictSalesTbl::Key0 k0;
				k0.RType = p_rec->RType;
				k0.Loc = p_rec->Loc;
				k0.GoodsID = p_rec->GoodsID;
				k0.Dt = p_rec->Dt;
				long   err_flags = 0;
				if(T.search(0, &k0, spEq)) {
					const double amt_tolerance = 1.0E-5;
					if(p_rec->Quantity != T.data.Quantity)
						err_flags |= errQtty;
					if(fabs(p_rec->Amount - T.data.Amount) > amt_tolerance)
						err_flags |= errAmt;
					if(p_rec->Flags != T.data.Flags)
						err_flags |= errFlags;
				}
				else
					err_flags |= errAbsence;
				if(err_flags) {
					//
					// "Ошибка в таблице продаж [@goods; @loc; @date]: @zstr"
					//PPTXT_PSALESTESTMSG             "Ошибка в таблице продаж [@goods; @loc; @date]: @zstr"
					//PPTXT_PSALESTESTMSG_QTTY        "неверное количество @real/@real"
					//PPTXT_PSALESTESTMSG_AMT         "неверная сумма @real/@real"
					//PPTXT_PSALESTESTMSG_FLAGS       "неверные флаги @hex/@hex"
					//PPTXT_PSALESTESTMSG_ABSENCE     "запись отсутствует"
					//
					PPID   loc_id = 0;
					LDATE  dt = ZERODATE;
					T.ExpandDate(p_rec->Dt, &dt);
					T.ExpandLoc(p_rec->Loc, &loc_id);
					msg = 0;
					if(err_flags & errAbsence) {
						PPLoadText(PPTXT_PSALESTESTMSG_ABSENCE, temp_buf);
						PPFormat(msg_fmt, &msg, p_rec->GoodsID, loc_id, dt, temp_buf.cptr());
					}
					if(err_flags & errQtty) {
						PPLoadText(PPTXT_PSALESTESTMSG_QTTY, temp_buf);
						PPFormat(temp_buf, &added_msg_buf, p_rec->Quantity, T.data.Quantity);
						PPFormat(msg_fmt, &msg, p_rec->GoodsID, loc_id, dt, added_msg_buf.cptr());
					}
					if(err_flags & errAmt) {
						PPLoadText(PPTXT_PSALESTESTMSG_AMT, temp_buf);
						PPFormat(temp_buf, &added_msg_buf, p_rec->Amount, T.data.Amount);
						PPFormat(msg_fmt, &msg, p_rec->GoodsID, loc_id, dt, added_msg_buf.cptr());
					}
					if(err_flags & errFlags) {
						PPLoadText(PPTXT_PSALESTESTMSG_FLAGS, temp_buf);
						PPFormat(temp_buf, &added_msg_buf, (long)p_rec->Flags, (long)T.data.Flags);
						PPFormat(msg_fmt, &msg, p_rec->GoodsID, loc_id, dt, added_msg_buf.cptr());
					}
					if(msg.NotEmpty()) {
						SETIFZ(p_logger, new PPLogger);
						CALLPTRMEMB(p_logger, Log(msg));
					}
				}
			}
		}
		else {
			//
			// Транзакцию начинаем только сейчас после того, как все необходимые данные были извлечены
			// (дабы это извлечение не торомзило под действием транзакции).
			//
			PPTransaction tra(use_ta);
			THROW(tra);
			if(p_vect) {
				PredictSalesTbl::Rec * p_rec = 0;
				//
				// Разносим операции поиска и удаления дубликатов с операцией
				// массированной вставки записей: это должно обеспечить некоторое ускорение
				// по сравнению с выполнением этих операций поочередно в одном цикле.
				//

				//
				// Удаляем данные в таблице, пересекающиеся с периодом заполнения //
				//
				__CI * p_ci = 0;
				const uint lvl_count = lvl_list.getCount();
				for(j = 0; !break_process && lvl_list.enumItems(&j, (void **)&p_ci);) {
					PROFILE_START
					THROW(T.RemovePeriod(-1, p_ci->GoodsID, &comm_period, 0));
					PROFILE_END
					if(show_wait_msg)
						PPWaitPercent(j, lvl_count, "Removing old sales data");
					if((j % 64) == 0 && CheckInterruption())
						break_process = 1;
				}
				if(!break_process) {
					PROFILE_START
					BExtInsert bei(&T);
					const uint vc = p_vect->getCount();
					for(j = 0; !break_process && p_vect->enumItems(&j, (void **)&p_rec);) {
						THROW_DB(bei.insert(p_rec));
						if(show_wait_msg)
							PPWaitPercent(j, vc, "Storing new sales data");
						if((j % 1024) == 0 && CheckInterruption())
							break_process = 1;
					}
					THROW_DB(bei.flash());
					PROFILE_END
				}
				ZDELETE(p_vect);
			}
			if(!break_process) {
				//
				// Сохраняем таблицу складов
				//
				THROW(T.Finish(1, 0));
				THROW(tra.Commit());
				{
					if(calcStat) {
						//
						// Разрываем транзакцию чтобы сканирование таблицы продаж для подстчета статистики
						// проходило вне транзации - так, вероятно, быстрее.
						//
						THROW(r = StoreStatByGoodsList(rGoodsList, P.GetNormPeriod().upp, &Stat.Sse, 1));
					}
					else
						r = 1;
					if(r > 0) {
						//
						// В конфигурации отмечаем факт завершения транзакции по очередному блоку товаров.
						// Если при обработке следующего блока произойдет обрыв процесса, то
						// при следующем запуска работа будет возобновлена, начиная с товара rGoodsList.getLast()+1
						//
						THROW(SetContinueMode(0, rGoodsList.getLast(), 1));
						if(CheckInterruption())
							break_process = 1;
					}
				}
			}
		}
		PROFILE_END
	}
	if(break_process)
		ok = -1;
	CATCHZOK
	ZDELETE(p_vect);
	CALLPTRMEMB(p_logger, Save(PPFILNAM_ERR_LOG, 0));
	ufp.Commit(); // @v8.1.2
	return ok;
}

int SLAPI PrcssrPrediction::SetupGoodsSaleByLoc(LocValEntry & rLvEntry, PPID goodsID, LDATE date, __HolidayArray & rHa, SArray * pVect)
{
	int    ok = 1;
	LDATE  last_date = rLvEntry.LastOpDate;
	if(last_date) {
		const  PPID loc_id = rLvEntry.LocID;
		int16  loc_idx = 0;
		const double rest = rLvEntry.Rest;
		const double sell = rLvEntry.Sell;
		const double amt  = rLvEntry.SellAmt;
		PredictSalesTbl::Rec rec;
		T.AddLocEntry(loc_id, &loc_idx);
		//
		// Ограничение (sell > 0.0 || rest > 0.0) препятствует занесению нулевых
		// продаж при отсутствии остатка. Это актуально при обработке первой
		// итерации основного цикла в функции ProcessSales.
		//
		if(sell > 0.0 || rest > 0.0) {
			if(!rHa.Is(loc_idx, last_date)) {
				int16  dt_idx = 0;
				MEMSZERO(rec);
				T.ShrinkDate(last_date, &dt_idx);
				rec.RType = PSRECTYPE_DAY;
				rec.GoodsID = goodsID;
				rec.Loc = loc_idx;
				rec.Dt = dt_idx;
				rec.Quantity = (float)sell;
				rec.Amount = (float)amt;
				rec.Flags = (sell == 0.0) ? PRSALF_ZERO : 0;
				THROW_SL(pVect->insert(&rec));
			}
		}
		//
		// Если в перерыве между продажами был остаток товара,
		// то считаем продажи в эти дни нулевыми.
		//
		if(rest > 0.0) {
			for(LDATE dt = plusdate(last_date, 1); dt < date; dt = plusdate(dt, 1)) {
				if(!rHa.Is(loc_idx, dt)) {
					int16  dt_idx = 0;
					MEMSZERO(rec);
					T.ShrinkDate(dt, &dt_idx);
					rec.RType = PSRECTYPE_DAY;
					rec.GoodsID = goodsID;
					rec.Loc = loc_idx;
					rec.Dt = dt_idx;
					// qtty and amount are zero
					rec.Flags = PRSALF_ZERO;
					THROW_SL(pVect->insert(&rec));
				}
			}
		}
	}
	rLvEntry.Sell = 0.0;
	rLvEntry.SellAmt = 0.0;
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPrediction::DeleteAllItems()
{
	return T.ClearAll();
}

int SLAPI PrcssrPrediction::DeleteItemsByPeriod(DateRange period, PPID gGrpID, int use_ta)
{
	return T.ClearByPeriod(period, gGrpID, use_ta);
}

int SLAPI PrcssrPrediction::SetContinueMode(int reset, PPID lastGoodsID, int use_ta)
{
	int    ok = 1;
	PPPredictConfig cfg;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Predictor::GetPredictCfg(&cfg));
		if(reset) {
			cfg.CbPeriod.SetZero();
			cfg.CbLastGoodsID = 0;
			cfg.CbProcess = 0;
			cfg.CbFlags   = 0;
			cfg.Flags &= ~PPPredictConfig::fContinueBuilding;
		}
		else {
			cfg.CbPeriod = P.GetNormPeriod();
			cfg.CbLastGoodsID = lastGoodsID;
			cfg.CbProcess = P.Process;
			cfg.CbFlags   = P.Flags;
			cfg.Flags |= PPPredictConfig::fContinueBuilding;
		}
		THROW(Predictor::PutPredictCfg(&cfg, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrPrediction::GetContinueMode(int checkOnly, PPID * pLastGoodsID)
{
	int    ok = -1;
	PPPredictConfig cfg;
	if(Predictor::GetPredictCfg(&cfg) > 0) {
		if(cfg.Flags & PPPredictConfig::fContinueBuilding) {
			ok = 1;
			if(!checkOnly) {
				P.SetPeriod(cfg.CbPeriod);
				P.Process = cfg.CbProcess;
				P.Flags = cfg.CbFlags;
				ASSIGN_PTR(pLastGoodsID, cfg.CbLastGoodsID);
			}
		}
	}
	return ok;
}

