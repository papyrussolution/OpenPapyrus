// DL200PRS.CPP
// Copyrigh (c) A.Sobolev 2002, 2003, 2005, 2007, 2008, 2009, 2012, 2013, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

// Prototype (REPORTW.CPP)

DL2_Resolver::SPD::SPD(int kind, DL2_Resolver * pR) : Kind(kind), P_R(pR), Flags(0)
{
}

DL2_Resolver::SPD::~SPD()
{
}

int DL2_Resolver::SPD::ResolveWarehouseList_(int32 listId, ObjIdListFilt & rList)
{
	PPID   obj_type = 0;
	int    r = P_R ? P_R->Oc.Get(listId, &obj_type, rList) : 0;
	return (r && obj_type == PPOBJ_LOCATION) ? r : 0;
}

int DL2_Resolver::SPD::ResolveGoodsGroupList(int32 listId, ObjIdListFilt & rList)
{
	PPID   obj_type = 0;
	int    r = P_R ? P_R->Oc.Get(listId, &obj_type, rList) : 0;
	return (r && obj_type == PPOBJ_GOODSGROUP) ? r : 0;
}

class DL2SPD_Bill : public DL2_Resolver::SPD {
public:
	DL2SPD_Bill(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kBill, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			const DL2SPD_Bill * p_s = static_cast<const DL2SPD_Bill *>(pS);
			if(Period.IsEqual(p_s->Period) && OpID == p_s->OpID && LocList.IsEqual(p_s->LocList))
				return 1;
		}
		return 0;
	}
	virtual int Init(const DL2_Score & rS, LDATE actualDate)
	{
		int    ok = 1;
		Period = rS.Period;
		Period.Actualize(actualDate);
		PPObjOprKind op_obj;
		if(rS.OpCode[0]) {
			THROW(op_obj.SearchBySymb(rS.OpCode, &OpID) > 0);
		}
		else
			OpID = 0;
		{
			LocList.Set(0);
			THROW(ResolveWarehouseList_(rS.LocListID, LocList));
		}
		CATCHZOK
		return ok;
	}
	virtual DL2_CI * Resolve(const DL2_Score & rS)
	{
		DL2_CI * p_ci = 0;
		if(!(Flags & fResolved)) {
			PPViewBill view;
			BillFilt filt;
			filt.Period = Period;
			filt.OpID = OpID;
			filt.LocList = LocList;
			THROW(view.Init_(&filt));
			THROW(view.CalcTotal(&Total));
			Flags |= fResolved;
		}
		double cost = 0.0;
		double result = 0.0;
		double netprice = 0.0;
		switch(rS.Sub) {
			case DL2_Score::subAmount:
				result = Total.Sum;
				break;
			case DL2_Score::subCost:
				result = Total.Amounts.Get(PPAMT_BUYING, 0);
				break;
			case DL2_Score::subPrice:
				result = Total.Amounts.Get(PPAMT_SELLING, 0);
				break;
			case DL2_Score::subDiscount:
				result = Total.Amounts.Get(PPAMT_DISCOUNT, 0);
				break;
			case DL2_Score::subNetPrice:
				result = Total.Amounts.Get(PPAMT_SELLING, 0) - Total.Amounts.Get(PPAMT_DISCOUNT, 0);
				break;
			case DL2_Score::subMargin:
				cost = Total.Amounts.Get(PPAMT_BUYING, 0);
				netprice = Total.Amounts.Get(PPAMT_SELLING, 0) - Total.Amounts.Get(PPAMT_DISCOUNT, 0);
				result = netprice - cost;
				break;
			case DL2_Score::subPctIncome:
				cost = Total.Amounts.Get(PPAMT_BUYING, 0);
				netprice = Total.Amounts.Get(PPAMT_SELLING, 0) - Total.Amounts.Get(PPAMT_DISCOUNT, 0);
				result = fdivnz(netprice - cost, cost);
				break;
			case DL2_Score::subPctMargin:
				cost = Total.Amounts.Get(PPAMT_BUYING, 0);
				netprice = Total.Amounts.Get(PPAMT_SELLING, 0) - Total.Amounts.Get(PPAMT_DISCOUNT, 0);
				result = fdivnz(netprice - cost, netprice);
				break;
			case DL2_Score::subCount:
				result = Total.Count;
				break;
			default:
				result = Total.Sum;
				break;
		}
		p_ci = new DL2_CI;
		p_ci->Init(result);
		CATCH
			ZDELETE(p_ci);
		ENDCATCH
		return p_ci;
	}

	DateRange Period;
	PPID   OpID;
	ObjIdListFilt LocList;
	BillTotal Total;
protected:
	DL2SPD_Bill(int kind, DL2_Resolver * pR) : DL2_Resolver::SPD(kind, pR)
	{
	}
};

class DL2SPD_Paym : public DL2SPD_Bill {
public:
	DL2SPD_Paym(DL2_Resolver * pR) : DL2SPD_Bill(DL2_Score::kPaym, pR)
	{
	}
};

class DL2SPD_CCheck : public DL2_Resolver::SPD {
public:
	DL2SPD_CCheck(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kCCheck, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			const DL2SPD_CCheck * p_s = static_cast<const DL2SPD_CCheck *>(pS);
			if(Period.IsEqual(p_s->Period) && LocList.IsEqual(p_s->LocList) && GgList.IsEqual(p_s->GgList))
				return 1;
		}
		return 0;
	}
	virtual int Init(const DL2_Score & rS, LDATE actualDate)
	{
		int    ok = 1;
		Period = rS.Period;
		Period.Actualize(actualDate);
		{
			LocList.Set(0);
			THROW(ResolveWarehouseList_(rS.LocListID, LocList));
		}
		{
			GgList.Set(0);
			THROW(ResolveGoodsGroupList(rS.GoodsGrpListID, GgList));
		}
		CATCHZOK
		return ok;
	}
	virtual DL2_CI * Resolve(const DL2_Score & rS)
	{
		DL2_CI * p_ci = 0;
		if(!(Flags & fResolved)) {
			PPViewCCheck view;
			CCheckFilt filt;
			filt.Period = Period;
			filt.SetLocList(&LocList.Get());
			if(GgList.GetCount()) {
				filt.GoodsGrpID = GgList.Get(0);
			}
			THROW(view.Init_(&filt));
			THROW(view.CalcTotal(&Total));
			Flags |= fResolved;
		}
		double result = 0.0;
		switch(rS.Sub) {
			case DL2_Score::subAmount:    result = Total.Amount; break;
			case DL2_Score::subCost:      result = Total.Amount; break;
			case DL2_Score::subPrice:     result = Total.Amount + Total.Discount; break;
			case DL2_Score::subDiscount:  result = Total.Discount; break;
			case DL2_Score::subNetPrice:  result = Total.Amount; break;
			case DL2_Score::subMargin:    break;
			case DL2_Score::subPctIncome: break;
			case DL2_Score::subPctMargin: break;
			case DL2_Score::subCount:     result = Total.Count; break;
			default:                      result = Total.Amount; break;
		}
		p_ci = new DL2_CI;
		p_ci->Init(result);
		CATCH
			ZDELETE(p_ci);
		ENDCATCH
		return p_ci;
	}
	DateRange Period;
	ObjIdListFilt LocList;
	ObjIdListFilt GgList;
	CCheckTotal Total;
};

class DL2SPD_GoodsRest : public DL2_Resolver::SPD {
public:
	DL2SPD_GoodsRest(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kGoodsRest, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			const DL2SPD_GoodsRest * p_s = static_cast<const DL2SPD_GoodsRest *>(pS);
			if(Period.IsEqual(p_s->Period) && LocList.IsEqual(p_s->LocList) && GgList.IsEqual(p_s->GgList))
				return 1;
		}
		return 0;
	}
	virtual int Init(const DL2_Score & rS, LDATE actualDate)
	{
		int    ok = 1;
		Period = rS.Period;
		Period.Actualize(actualDate);
		{
			LocList.Set(0);
			THROW(ResolveWarehouseList_(rS.LocListID, LocList));
		}
		{
			GgList.Set(0);
			THROW(ResolveGoodsGroupList(rS.GoodsGrpListID, GgList));
		}
		Flags &= ~fResolved;
		CATCHZOK
		return ok;
	}
	virtual DL2_CI * Resolve(const DL2_Score & rS)
	{
		DL2_CI * p_ci = 0;
		if(!(Flags & fResolved)) {
			PPViewGoodsRest view;
			GoodsRestFilt filt;
			filt.Date = Period.IsZero() ? ZERODATE : Period.upp;
			if(LocList.GetCount())
				filt.LocList.Set(&LocList.Get());
			if(GgList.GetCount())
				filt.GoodsGrpID = GgList.Get(0);
			filt.Flags |= GoodsRestFilt::fCalcTotalOnly;
			THROW(view.Init_(&filt));
			THROW(view.GetTotal(&Total));
			Flags |= fResolved;
		}
		double result = 0.0;
		switch(rS.Sub) {
			case DL2_Score::subCount:    result = Total.Count; break;
			case DL2_Score::subAmount:   result = Total.SumPrice; break;
			case DL2_Score::subCost:     result = Total.SumCost; break;
			case DL2_Score::subPrice:    result = Total.SumPrice; break;
			case DL2_Score::subDiscount: break;
			case DL2_Score::subNetPrice: break;
			case DL2_Score::subMargin:   result = Total.SumPrice - Total.SumCost; break;
			case DL2_Score::subPctIncome:
				result = 100.0 * fdivnz(Total.SumPrice - Total.SumCost, Total.SumCost);
				break;
			case DL2_Score::subPctMargin:
				result = 100.0 * fdivnz(Total.SumPrice - Total.SumCost, Total.SumPrice);
				break;
			default: result = Total.SumPrice; break;
		}
		p_ci = new DL2_CI;
		p_ci->Init(result);
		CATCH
			ZDELETE(p_ci);
		ENDCATCH
		return p_ci;
	}
	DateRange Period;
	ObjIdListFilt LocList;
	ObjIdListFilt GgList;
	GoodsRestTotal Total;
};

class DL2SPD_Debt : public DL2_Resolver::SPD {
public:
	DL2SPD_Debt(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kDebt, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			const DL2SPD_Debt * p_s = static_cast<const DL2SPD_Debt *>(pS);
			if(Period.IsEqual(p_s->Period) && AccSheetID == p_s->AccSheetID)
				return 1;
		}
		return 0;
	}
	virtual int Init(const DL2_Score & rS, LDATE actualDate)
	{
		int    ok = 1;
		Period = rS.Period;
		Period.Actualize(actualDate);
		if(rS.OpCode[0]) {
			PPObjAccSheet acs_obj;
			THROW(acs_obj.SearchBySymb(rS.OpCode, &AccSheetID, 0) > 0);
		}
		else {
			AccSheetID = GetSellAccSheet();
		}
		CATCHZOK
		return ok;
	}
	virtual DL2_CI * Resolve(const DL2_Score & rS)
	{
		DL2_CI * p_ci = 0;
		if(!(Flags & fResolved)) {
			PPViewDebtTrnovr view;
			DebtTrnovrFilt filt;
			filt.Period = Period;
			filt.PaymPeriod = Period;
			filt.AccSheetID = AccSheetID;
			filt.Flags |= DebtTrnovrFilt::fExtended;
			THROW(view.Init_(&filt));
			THROW(view.GetTotal(&Total));
			Flags |= fResolved;
		}
		double result = 0.0;
		switch(rS.Sub) {
			case DL2_Score::subNone:
			case DL2_Score::subAmount:
				result = Total.TDebt.Get(0, 0);
				break;
			case DL2_Score::subCount:
				result = Total.Count;
				break;
			default:
				result = Total.TDebt.Get(0, 0);
				break;
		}
		p_ci = new DL2_CI;
		p_ci->Init(result);
		CATCH
			ZDELETE(p_ci);
		ENDCATCH
		return p_ci;
	}

	DateRange Period;
	PPID   AccSheetID;
	DebtTrnovrTotal Total;
};

class DL2SPD_BizScore : public DL2_Resolver::SPD {
public:
	DL2SPD_BizScore(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kBizScore, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			const DL2SPD_BizScore * p_s = static_cast<const DL2SPD_BizScore *>(pS);
			if(Period.IsEqual(p_s->Period) && ScoreID == p_s->ScoreID)
				return 1;
		}
		return 0;
	}
	virtual int Init(const DL2_Score & rS, LDATE actualDate)
	{
		int    ok = 1;
		Period = rS.Period;
		Period.Actualize(actualDate);
		if(rS.OpCode[0]) {
			PPObjBizScore bs_obj;
			THROW(bs_obj.SearchBySymb(rS.OpCode, &ScoreID, 0) > 0);
		}
		else {
			ScoreID = 0;
		}
		CATCHZOK
		return ok;
	}
	virtual DL2_CI * Resolve(const DL2_Score & rS)
	{
		DL2_CI * p_ci = 0;
		if(!(Flags & fResolved)) {
			PPViewBizScoreVal view;
			BizScoreValFilt filt;
			filt.Period = Period;
			filt.BizScoreID = ScoreID;
			THROW(view.Init_(&filt));
			THROW(view.CalcTotal(&Total));
			Flags |= fResolved;
		}
		double result = 0.0;
		switch(rS.Sub) {
			case DL2_Score::subNone:
			case DL2_Score::subAmount: result = Total.Sum; break;
			case DL2_Score::subCount:  result = Total.Count; break;
			case DL2_Score::subAverage: result = fdivnz(Total.Sum, Total.Count); break;
			default: result = Total.Sum; break;
		}
		p_ci = new DL2_CI;
		p_ci->Init(result);
		CATCH
			ZDELETE(p_ci);
		ENDCATCH
		return p_ci;
	}
	DateRange Period;
	PPID   ScoreID;
	//
	//
	//
	BizScoreValTotal Total;
};

#if 0 // {
class DL2SPD_PersonEvent : public DL2_Resolver::SPD {
public:
	DL2SPD_PersonEvent(DL2_Resolver * pR) : DL2_Resolver::SPD(DL2_Score::kPersonEvent, pR)
	{
	}
	virtual int IsEqHdr(const DL2_Resolver::SPD * pS) const
	{
		if(pS && pS->Kind == Kind) {
			DL2SPD_PersonEvent * p_s = (DL2SPD_PersonEvent *)pS;
			if(Period.IsEqual(p_s->Period) && OpSymb.CmpNC(p_s->OpSymb) == 0)
				return 1;
		}
		return 0;
	}

	DateRange Period;
	SString OpSymb;
	//
	//
	//
	PPIDArray EvIdList;
	StringSet EvStrList;
};
#endif // } 0
//
//
//
SLAPI DL2_Resolver::DL2_Resolver(long flags) : Flags(flags), ActualDate(ZERODATE), CurAr(-1)
{
	SString buf;
	PPLoadText(PPTXT_DL200_NAMEVARS, buf);
	StringSet ss(';', buf);
	for(uint i = 0, j = 0; ss.get(&i, buf) > 0; j++)
		NameVars.Add(j + 1, buf);
	NameVars.SortByText();
}

SLAPI DL2_Resolver::~DL2_Resolver()
{
}

int FASTCALL DL2_Resolver::Log(const char * pMsg)
{
	int    ok = -1;
	if(Flags & fDoLogging && pMsg) {
		ok = PPLogMessage(PPFILNAM_DEBUG_LOG, pMsg, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
	}
	return ok;
}

int SLAPI DL2_Resolver::SetActualDate(LDATE actualDate)
{
	if(checkdate(actualDate, 1)) {
		ActualDate = actualDate;
		return 1;
	}
	else
		return PPSetErrorSLib();
}

LDATE SLAPI DL2_Resolver::GetActualDate() const
{
	return ActualDate;
}

int SLAPI DL2_Resolver::SetPeriod(DateRange period)
{
	CurPeriod = period;
	return 1;
}

int SLAPI DL2_Resolver::SetCurArticle(long ar)
{
	CurAr = ar;
	return 1;
}

long SLAPI DL2_Resolver::GetCurArticle() const
{
	return CurAr;
}

int SLAPI DL2_Resolver::ResolveName(const char * pExpression, SString & rName)
{
	int    ok = 1;
	const  PPCommConfig & r_ccfg = CConfig;
	uint   pos = 0;
	long   id = -1;
	SString buf;
	rName.Z();
	if(NameVars.SearchByText(pExpression, 1, &pos) > 0)
		id = NameVars.Get(pos).Id - 1;
	switch(id) {
		case PPDL200_NAMEVAR_MAINORGNAME:
			rName = GetMainOrgName(rName);
			break;
		case PPDL200_NAMEVAR_MAINORGADDR:
			PsnObj.GetAddress(r_ccfg.MainOrgID, rName);
			break;
		case PPDL200_NAMEVAR_MAINORGINN:
		case PPDL200_NAMEVAR_MAINORGOKPO:
			{
				PersonReq psn_req;
				PsnObj.GetPersonReq(r_ccfg.MainOrgID, &psn_req);
				if(id == PPDL200_NAMEVAR_MAINORGINN)
					rName.CopyFrom(psn_req.TPID);
				else
					rName.CopyFrom(psn_req.OKPO);
			}
			break;
		case PPDL200_NAMEVAR_DIRECTOR:
			{
				DS.GetTLA().InitMainOrgData(0);
				GetPersonName(r_ccfg.MainOrgDirector_, rName);
			}
			break;
		case PPDL200_NAMEVAR_ACCOUNTANT:
			{
				DS.GetTLA().InitMainOrgData(0);
				GetPersonName(r_ccfg.MainOrgAccountant_, rName);
			}
			break;
		case PPDL200_NAMEVAR_DBNAME:
			CurDict->GetDbName(rName);
			break;
		case PPDL200_NAMEVAR_DBSYMB:
			CurDict->GetDbSymb(rName);
			break;
		default:
			PPSetError(PPERR_DL200_SYMBNFOUND, pExpression);
			ok = 0;
			break;
	}
	return ok;
}

DL2_CI * SLAPI DL2_Resolver::ResolveScore(const DL2_Score & rSc)
{
	DL2_CI * p_ci = 0;
	SPD  * p_spd = 0;
	switch(rSc.Kind) {
		case DL2_Score::kBill:
			THROW_MEM(p_spd = new DL2SPD_Bill(this));
			// BizScore: создан контекст Bill
			break;
		case DL2_Score::kPaym:
			THROW_MEM(p_spd = new DL2SPD_Paym(this));
			// BizScore: создан контекст Paym
			break;
		case DL2_Score::kCCheck:
			THROW_MEM(p_spd = new DL2SPD_CCheck(this));
			// BizScore: создан контекст CCheck
			break;
		case DL2_Score::kGoodsRest:
			THROW_MEM(p_spd = new DL2SPD_GoodsRest(this));
			// BizScore: создан контекст GoodsRest
			break;
		case DL2_Score::kDebt:
			THROW_MEM(p_spd = new DL2SPD_Debt(this));
			// BizScore: создан контекст Debt
			break;
		case DL2_Score::kBizScore:
			THROW_MEM(p_spd = new DL2SPD_BizScore(this));
			// BizScore: создан контекст BizScore
			break;
		// case DL2_Score::kPersonEvent: p_spd = new DL2SPD_PersonEvent(this); break;
	}
	if(p_spd) {
		int    found = 0;
		p_spd->Init(rSc, ActualDate);
		for(uint i = 0; !found && i < SpdList.getCount(); i++) {
			SPD * p_item = SpdList.at(i);
			if(p_item && p_spd->IsEqHdr(p_item)) {
				found = 1;
				ZDELETE(p_spd);
				p_spd = p_item;
			}
		}
		p_ci = p_spd->Resolve(rSc);
		if(!found) {
			SpdList.insert(p_spd);
		}
	}
	CATCH
		ZDELETE(p_ci);
	ENDCATCH
	return p_ci;
}

/*virtual*/DL2_CI * SLAPI DL2_Resolver::Resolve(int exprNo, const DL2_CI * pCi) { return Helper_Resolve(0, pCi); }
DL2_CI * SLAPI DL2_Resolver::Resolve(const DL2_CI * pItem) { return Helper_Resolve(0, pItem); }

DL2_CI * SLAPI DL2_Resolver::Helper_Resolve(const DL2_Column * pCol, const DL2_CI * pItem)
{
	DL2_CI * p_result = 0;
	if(pItem->CiType == DL2CIT_ACC) {
		double val = 0.0;
		AcctID acctid;
		PPID   acc_sheet_id = 0;
		int    aco;
		long   mask = 0;
		DL2_Acc dl2ac = pItem->A;
		const DL2_Acc * p_col_ac = (pCol && pCol->CiType == DL2CIT_ACC) ? (&pCol->CiAc) : 0;
		if(dl2ac.Flags & DL2_Acc::fAco1) {
			dl2ac.Acc.sb = 0;
			dl2ac.Acc.ar = 0;
			aco = ACO_1;
		}
		else if(dl2ac.Flags & DL2_Acc::fAco2) {
			dl2ac.Acc.ar = 0;
			aco = ACO_2;
		}
		else if(dl2ac.Flags & DL2_Acc::fAco3)
			aco = ACO_3;
		else
			if(dl2ac.Acc.ar)
				aco = ACO_3;
			else if(dl2ac.Acc.sb)
				aco = ACO_2;
			else
				aco = ACO_1;
		mask = (DL2_Acc::fInRest | DL2_Acc::fRest | DL2_Acc::fTurnover);
		if(!(dl2ac.Flags & mask)) {
			if(p_col_ac)
				dl2ac.Flags |= (p_col_ac->Flags & mask);
			if(!(dl2ac.Flags & mask))
				dl2ac.Flags |= DL2_Acc::fRest;
		}
		mask = (DL2_Acc::fDebit | DL2_Acc::fCredit);
		if(!(dl2ac.Flags & mask) && p_col_ac && p_col_ac->Flags & mask)
			dl2ac.Flags |= (p_col_ac->Flags & mask);
		if(dl2ac.PrdOfs == 0 && p_col_ac && p_col_ac->PrdOfs) {
			dl2ac.PrdOfs = p_col_ac->PrdOfs;
			dl2ac.NumPrdOfs = p_col_ac->NumPrdOfs;
		}
		int    r = AtObj.ConvertAcct(&dl2ac.Acc, 0L, &acctid, &acc_sheet_id);
		THROW(r);
		THROW_MEM(p_result = new DL2_CI);
		if(r > 0) {
			DateRange period = CurPeriod;
			if(dl2ac.PrdOfs) {
				plusperiod(&period.low, dl2ac.PrdOfs, dl2ac.NumPrdOfs, 0);
				plusperiod(&period.upp, dl2ac.PrdOfs, dl2ac.NumPrdOfs, 0);
			}
			PPID   acc_id = 0;
			long   f = (DL2_Acc::fDebit | DL2_Acc::fCredit);
			int    is_net_trnovr = BIN(dl2ac.Flags & DL2_Acc::fTurnover && (!(dl2ac.Flags & f) || (dl2ac.Flags & f) == f));
			if(oneof2(aco, ACO_1, ACO_2))
				acc_id = acctid.ac;
			else
				THROW(AtObj.P_Tbl->AcctIDToRel(&acctid, &acc_id));
			if(dl2ac.Flags & DL2_Acc::fTurnover && dl2ac.CorrAcc.ac) {
				//
				// Обороты в корреспонденции с заданным счетом
				//
				AccAnlzFilt flt;
				flt.Period = period;
				flt.Aco = dl2ac.GetAco();
				flt.AccID = acc_id;
				flt.CorAco = dl2ac.GetCorrAco();
				flt.CorAcc = dl2ac.CorrAcc;
				flt.Flags |= AccAnlzFilt::fTotalOnly;
				PPViewAccAnlz v_accanlz;
				AccAnlzTotal t_accanlz;
				v_accanlz.Init_(&flt);
				v_accanlz.GetTotal(&t_accanlz);
				double dbt = t_accanlz.DbtTrnovr.Get(0, 0L /* curID */);
				double crd = t_accanlz.CrdTrnovr.Get(0, 0L /* curID */);
				if(is_net_trnovr)
					val = dbt-crd;
				else if(dl2ac.Flags & DL2_Acc::fDebit)
					val = dbt;
				else if(dl2ac.Flags & DL2_Acc::fCredit)
					val = crd;
			}
			else if(dl2ac.Flags & (DL2_Acc::fRest | DL2_Acc::fInRest) || is_net_trnovr) {
				//
				// Развернутый остаток.
				// Рассчитывается тогда, когда запрашивается входящий либо исходящий остаток
				// конкретно по дебету либо по кредиту счета (субсчета). В остальных случаях
				// опция fSpread игнорируется.
				//
				if((dl2ac.Flags & DL2_Acc::fSpread) && (dl2ac.Flags & (DL2_Acc::fDebit | DL2_Acc::fCredit)) &&
					oneof2(aco, ACO_1, ACO_2)) {
					uint   brf = BALRESTF_SPREADBYSUBACC;
					if(aco == ACO_1)
						brf |= BALRESTF_ACO1GROUPING;
					double dbt = 0.0, crd = 0.0;
					if(dl2ac.Flags & DL2_Acc::fRest) {
						THROW(AtObj.P_Tbl->GetBalRest(period.upp, acc_id, &dbt, &crd, brf));
					}
					else if(dl2ac.Flags & DL2_Acc::fInRest) {
						THROW(AtObj.P_Tbl->GetBalRest(period.low, acc_id, &dbt, &crd, brf | BALRESTF_INCOMING));
					}
					if(dl2ac.Flags & DL2_Acc::fDebit)
						val = dbt;
					else if(dl2ac.Flags & DL2_Acc::fCredit)
						val = crd;
				}
				else {
					AmtList al_in, al_out;
					THROW(AtObj.P_Tbl->CalcComplexRest(aco, acc_id, 0, 0, &period, &al_in, &al_out));
					if(dl2ac.Flags & DL2_Acc::fRest) {
						val = al_out.Get(0, 0);
						if(dl2ac.Flags & DL2_Acc::fCredit)
							val = (val < 0) ? -val : 0.0;
						else if(dl2ac.Flags & DL2_Acc::fDebit) {
							if(val < 0)
								val = 0;
						}
					}
					else if(dl2ac.Flags & DL2_Acc::fInRest) {
						val = al_in.Get(0, 0);
						if(dl2ac.Flags & DL2_Acc::fCredit)
							val = (val < 0) ? -val : 0.0;
						else if(dl2ac.Flags & DL2_Acc::fDebit) {
							if(val < 0)
								val = 0;
						}
					}
					else // if(is_net_trnovr)
						val = al_out.Get(0, 0) - al_in.Get(0, 0);
				}
			}
			else { // if(dl2ac.Flags & DL2_Acc::fTurnover) {
				double in_dbt = 0.0, in_crd = 0.0;
				double out_dbt = 0.0, out_crd = 0.0;
				if(oneof2(aco, ACO_1, ACO_2)) {
					uint   i;
					long   _aco = aco;
					PPID   _acc_id = acc_id;
					PPIDArray acc_list;
					THROW(AtObj.P_Tbl->IdentifyAcc(&_aco, &_acc_id, 0, 0, &acc_list));
					if(acc_list.getCount() == 0)
						acc_list.add(_acc_id);
					for(i = 0; i < acc_list.getCount(); i++) {
						_acc_id = acc_list.at(i);
						THROW(AtObj.P_Tbl->GetBalRest(period.low, _acc_id, &in_dbt, &in_crd, BALRESTF_INCOMING));
						THROW(AtObj.P_Tbl->GetBalRest(period.upp, _acc_id, &out_dbt, &out_crd, 0));
						if(dl2ac.Flags & DL2_Acc::fDebit)
							val += (out_dbt - in_dbt);
						else
							val += (out_crd - in_crd);
					}
				}
				else {
					val = -1.0;          // @todo
				}
			}
		}
		p_result->Init(R2(val));
	}
	else if(pItem->CiType == DL2CIT_SCORE) {
		p_result = ResolveScore(pItem->Score);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}
//
//
//
SLAPI DL200_Context::DL200_Context(DL2_Resolver * pResolver, BillContext * pBillCtx) : ExprEvalContext(),
	P_Resolver(pResolver), P_BillCtx(pBillCtx), LastFuncId(EXRP_EVAL_FIRST_FUNC)
{
	ImplementFlags |= fSelfScanArgList;
}

int SLAPI DL200_Context::IsFunc(const char * pSymb, int * pFuncId)
{
	int    func_id = 0;
	DL2_Score sc;
	int    ok = sc.GetFromStr(pSymb, 0);
	if(ok > 0) {
		SString symb(pSymb);
		symb.Strip();
		uint   pos = 0;
		if(FuncList.SearchByText(symb, 1, &pos)) {
			func_id = FuncList.Get(pos).Id;
		}
		else {
			func_id = ++LastFuncId;
			FuncList.Add(func_id, symb);
		}
		ok = 1;
	}
	ASSIGN_PTR(pFuncId, func_id);
	return ok;
}

int SLAPI DL200_Context::ResolveFunc(int funcId, FC & rFc)
{
	int    ok = 1;
	SString func_name;
	if(FuncList.GetText(funcId, func_name)) {
		func_name.Cat(rFc.StrArg);
		DL2_Score sc;
		sc.Init(P_Resolver);
		sc.GetFromStr(func_name, 0);
		DL2_CI * p_ci = P_Resolver->ResolveScore(sc);
		if(p_ci) {
			if(p_ci->CiType == DL2CIT_REAL)
				rFc.RetReal = p_ci->R;
			else if(p_ci->CiType == DL2CIT_INT)
				rFc.RetReal = p_ci->I;
			else if(p_ci->CiType == DL2CIT_STRING)
				rFc.RetStr = p_ci->GetStr();
		}
		delete p_ci;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI DL200_Context::Resolve(const char * pSymb, double * pVal)
{
	int    ok = 1;
	double v = 0.0;
	DL2_Acc acc;
	DL2_CI ci;
	DL2_Formula formula;
	if(pSymb) {
		if(pSymb[0] == '[') {
			acc.Init();
			acc.GetFromStr(pSymb, P_Resolver->GetCurArticle());
			ci.Init(&acc);
			formula.AddItem(&ci);
			const DL2_CI * p_res = formula.Calc(0, P_Resolver);
			if(p_res)
				v = p_res->R;
		}
		else if(pSymb[0] == '$') {
			size_t offs = 0;
			DL2_Score sc;
			sc.Init(P_Resolver);
			sc.GetFromStr(pSymb, &offs);
			ci.Init(&sc);
			formula.AddItem(&ci);
			const DL2_CI * p_res = formula.Calc(0, P_Resolver);
			if(p_res)
				v = p_res->R;
		}
		else if(P_BillCtx)
			ok = P_BillCtx->Resolve(pSymb, &v);
	}
	ASSIGN_PTR(pVal, v);
	return ok;
}

double SLAPI DL2_Resolver::Resolve(const char * pExpression)
{
	double v = 0.0;
	DL200_Context ctx(this);
	PPExprParser::CalcExpression(pExpression, &v, 0/*const PPCalcFuncList * pCFL*/, &ctx);
	return v;
}

int SLAPI DL2_Resolver::ReverseFormula(const char * pFormula, SString & rResult)
{
	int    ok = 1;
	SString inp_buf(pFormula);
	DL2_Score sc;
	sc.Init(this);
	sc.GetFromStr(inp_buf.Strip(), 0);
	sc.PutToStr(rResult);
	return ok;
}
//
//
//
SLAPI PrcssrDL200::PrcssrDL200() : P_Dict(0), P_HdrTbl(0), P_IterTbl(0)
{
	InitParam(&P);
}

SLAPI PrcssrDL200::~PrcssrDL200()
{
	delete P_HdrTbl;
	delete P_IterTbl;
	delete P_Dict;
}

DL2_CI * SLAPI PrcssrDL200::Resolve(int exprNo, const DL2_CI * pCi)
{
	const DL2_Column * p_col = D.GetColumn(exprNo);
	return Helper_Resolve(p_col, pCi);
}

struct _DL200_OutpHdr {
	int32  HdrID;
	char   Title[36];
	int32  MainOrgID;
	char   MainOrgName[48];
	char   MainOrgAddr[64];
	char   MainOrgBusiness[48];
	char   MainOrgINN[24];
	char   MainOrgOKPO[24];
	char   MainOrgOKONH[32];
	DateRange Period;
	char   PeriodTxt[32];
	int16  Cycle;
	int16  NumCycles;
	char   CycleTxt[24];
	//char   Descript[...];
	//char   ColN_Name[36]; // NumColumns times
};

struct _DL200_OutpIter {
	int32  IterID;
	int16  CycleNo;
	DateRange Period;
	char   PeriodTxt[32];
	char   CycleTxt[32];
	//char   GroupN_Descr[...];  ...
	//????   ColN_Value;         ...
};

DBTable * SLAPI PrcssrDL200::CreateHeaderDBTable()
{
	uint   i;
	BNKey  bnkey;
	size_t max_col_name = 16;
	DBTable * p_tbl = new DBTable;
	THROW_MEM(p_tbl);
	p_tbl->AddField(/*"HdrID"*/"__ID",       MKSTYPE(S_AUTOINC, 4));
	p_tbl->AddField("Title",       MKSTYPE(S_ZSTRING, 36));
	p_tbl->AddField("MainOrgID",   MKSTYPE(S_INT, 4));
	p_tbl->AddField("MainOrgName", MKSTYPE(S_ZSTRING, 48));
	p_tbl->AddField("MainOrgAddr", MKSTYPE(S_ZSTRING, 64));
	p_tbl->AddField("MainOrgBusiness", MKSTYPE(S_ZSTRING, 48));
	p_tbl->AddField("MainOrgINN",      MKSTYPE(S_ZSTRING, 24));
	p_tbl->AddField("MainOrgOKPO",     MKSTYPE(S_ZSTRING, 24));
	p_tbl->AddField("MainOrgOKONH",    MKSTYPE(S_ZSTRING, 32));
	p_tbl->AddField("Beg",       MKSTYPE(S_DATE, 4));
	p_tbl->AddField("End",       MKSTYPE(S_DATE, 4));
	p_tbl->AddField("PeriodTxt", MKSTYPE(S_ZSTRING, 32));
	p_tbl->AddField("Cycle",     MKSTYPE(S_INT, 2));
	p_tbl->AddField("NumCycles", MKSTYPE(S_INT, 2));
	p_tbl->AddField("CycleTxt",  MKSTYPE(S_ZSTRING, 24));
	if(D.P_Descript)
		max_col_name = sstrlen(D.P_Descript)+1;
	else
		max_col_name = 36;
	p_tbl->AddField("Descript", MKSTYPE(S_ZSTRING, max_col_name));
	for(i = 0; i < D.GetColumnsCount(); i++) {
		const DL2_Column * p_c = D.GetColumn(i);
		char fld_title[48];
		sprintf(fld_title, "Col%02u_Name", i+1);
		size_t col_name_len = 16;
		if(p_c->P_Title)
			col_name_len = sstrlen(p_c->P_Title)+1;
		p_tbl->AddField(fld_title, MKSTYPE(S_ZSTRING, col_name_len));
	}
	bnkey.addSegment(0, XIF_EXT);
	p_tbl->AddKey(bnkey);
	CATCH
		ZDELETE(p_tbl);
	ENDCATCH
	return p_tbl;
}

DBTable * SLAPI PrcssrDL200::CreateIterDBTable()
{
	/*
		int32 IterID;
		int16 CycleNo;
		date  Beg;
		date  End;
		char  PeriodTxt[32];
		char  CycleTxt[32];

		char  GroupN_Descr[];

		????  ColN_Value;
	*/
	uint   i, num_groups = 0;
	BNKey  bnkey;
	char   fld_name[36];
	DBTable * p_tbl = new DBTable;
	THROW_MEM(p_tbl);
	p_tbl->AddField(/*"IterID"*/"__ID",    MKSTYPE(S_AUTOINC, 4));
	p_tbl->AddField("CycleNo",   MKSTYPE(S_INT, 2));
	p_tbl->AddField("Beg",       MKSTYPE(S_DATE, 4));
	p_tbl->AddField("End",       MKSTYPE(S_DATE, 4));
	p_tbl->AddField("PeriodTxt", MKSTYPE(S_ZSTRING, 32));
	p_tbl->AddField("CycleTxt",  MKSTYPE(S_ZSTRING, 32));

	num_groups = D.GetMaxNesting();
	for(i = 0; i < num_groups; i++) {
		size_t max_descr_len = D.GetMaxDescriptionSize(i+1);
		sprintf(fld_name, "Group%02u_Descr", i+1);
		p_tbl->AddField(fld_name, MKSTYPE(S_ZSTRING, max_descr_len));
	}
	for(i = 0; i < D.GetColumnsCount(); i++) {
		const DL2_Column * p_c = D.GetColumn(i);
		sprintf(fld_name, "Col%02u_Value", i+1);
		TYPEID typ;
		if(oneof2(p_c->CiType, DL2CIT_REAL, DL2CIT_ACC))
			typ = MKSTYPE(S_FLOAT, 8);
		else if(p_c->CiType == DL2CIT_INT)
			typ = MKSTYPE(S_INT, 4);
		else if(p_c->CiType == DL2CIT_DATE)
			typ = MKSTYPE(S_DATE, 4);
		else if(p_c->CiType == DL2CIT_TIME)
			typ = MKSTYPE(S_TIME, 4);
		else if(p_c->CiType == DL2CIT_STRING)
			typ = MKSTYPE(S_ZSTRING, p_c->MaxOutSize);
		else
			typ = MKSTYPE(S_ZSTRING, 36);
		p_tbl->AddField(fld_name, typ);
	}
	bnkey.addSegment(0, XIF_EXT);
	p_tbl->AddKey(bnkey);
	CATCH
		ZDELETE(p_tbl);
	ENDCATCH
	return p_tbl;
}

int FASTCALL __CopyFileByPath(const char * pSrcPath, const char * pDestPath, const char * pFileName); // Prototype (pputil.cpp)

int	SLAPI PrcssrDL200::FillHeader()
{
	int    ok = 1;
	P_HdrTbl->clearDataBuf();
	size_t p = 0, max_col_name;
	uint   i;
	char * p_buf = static_cast<char *>(P_HdrTbl->getDataBuf());
	_DL200_OutpHdr * p_hdr_buf = reinterpret_cast<_DL200_OutpHdr *>(p_buf);
	PPID   main_org_id = 0;
	SString temp_buf;
	PPObjPerson psn_obj;
	PersonReq pr;
	GetMainOrgID(&main_org_id);
	psn_obj.GetPersonReq(main_org_id, &pr);
	STRNSCPY(p_hdr_buf->Title, D.Name);
	p_hdr_buf->MainOrgID = main_org_id;
	(temp_buf = pr.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(p_hdr_buf->MainOrgName, sizeof(p_hdr_buf->MainOrgName));
	(temp_buf = pr.Addr).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(p_hdr_buf->MainOrgAddr, sizeof(p_hdr_buf->MainOrgAddr));
	(temp_buf = pr.TPID).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(p_hdr_buf->MainOrgINN, sizeof(p_hdr_buf->MainOrgINN));
	(temp_buf = pr.OKPO).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(p_hdr_buf->MainOrgOKPO, sizeof(p_hdr_buf->MainOrgOKPO));
	(temp_buf = pr.OKONF).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(p_hdr_buf->MainOrgOKONH, sizeof(p_hdr_buf->MainOrgOKONH));
	p_hdr_buf->Period = P.Period;
	p_hdr_buf->Cycle  = P.Cycl.Cycle;
	p_hdr_buf->NumCycles = P.Cycl.NumCycles;
	p += sizeof(_DL200_OutpHdr);

	if(D.P_Descript) {
		max_col_name = sstrlen(D.P_Descript)+1;
		strnzcpy(p_buf+p, D.P_Descript, max_col_name);
	}
	else
		max_col_name = 36;
	p += max_col_name;
	for(i = 0; i < D.GetColumnsCount(); i++) {
		const DL2_Column * p_c = D.GetColumn(i);
		size_t col_name_len = 16;
		if(p_c->P_Title) {
			col_name_len = sstrlen(p_c->P_Title)+1;
			strnzcpy(p_buf+p, p_c->P_Title, col_name_len);
		}
		else {
			col_name_len = 16;
			sprintf(p_buf+p, "Column #%02u", i+1);
		}
		p += col_name_len;
	}
	THROW_DB(P_HdrTbl->insertRec());
	CATCHZOK
	return ok;
}

int SLAPI PrcssrDL200::InitOutput()
{
	int    ok = 1;
	SString iter_fname;
	SString head_fname;
	SString path;
	SString packpath;
	SString temp_buf;

	ZDELETE(P_Dict);
	ZDELETE(P_HdrTbl);
	ZDELETE(P_IterTbl);

	PPGetFileName(PPFILNAM_HEAD_BTR, head_fname);
	PPGetFileName(PPFILNAM_ITER_BTR, iter_fname);

	PPGetPath(PPPATH_TEMP, path);
	PPGetPath(PPPATH_PACK, packpath);
	THROW_PP(path.NotEmptyS(), PPERR_UNDEFTEMPPATH);
	THROW_PP(packpath.NotEmptyS(), PPERR_UNDEFPACKPATH);
	path.RmvLastSlash();
	packpath.RmvLastSlash();
	THROW_SL(fileExists(path));
	THROW_SL(fileExists(packpath));
	path.SetLastSlash().CatLongZ(DS.GetTLA().PrnDirId, 8);
	OutPath = path;
	if(::access(path, 0) != 0) {
		THROW_SL(createDir(path));
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfTableFileName));
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfFieldFileName));
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfIndexFileName));
	}
	THROW_MEM(P_Dict = BDictionary::CreateBtrDictInstance(path));
	THROW_DB(P_Dict->IsValid());
	THROW(P_HdrTbl = CreateHeaderDBTable());
	THROW(P_IterTbl = CreateIterDBTable());
	//
	//
	//
	P_HdrTbl->SetTableName((temp_buf = D.Name).Cat("_Head"));
	P_HdrTbl->SetName((temp_buf = path).SetLastSlash().Cat(head_fname));
	THROW(P_Dict->CreateTableAndFileBySpec(&P_HdrTbl));
	//
	P_IterTbl->SetTableName((temp_buf = D.Name).Cat("_Iter"));
	P_IterTbl->SetName((temp_buf = path).SetLastSlash().Cat(iter_fname));
	THROW(P_Dict->CreateTableAndFileBySpec(&P_IterTbl));
	//
	THROW(FillHeader());
	CATCHZOKPPERR
	return ok;
}

int SLAPI PrcssrDL200::FinishOutput()
{
	ZDELETE(P_HdrTbl);
	ZDELETE(P_IterTbl);
	ZDELETE(P_Dict);
	return 1;
}

int SLAPI PrcssrDL200::InitParam(Param * p)
{
	memzero(p, sizeof(Param));
	return 1;
}

class DL200_ParamDialog : public TDialog {
	DECL_DIALOG_DATA(PrcssrDL200::Param);
	enum {
		ctlgroupCycle = 1
	};
public:
	DL200_ParamDialog() : TDialog(DLG_DL200P)
	{
		CycleCtrlGroup * grp = new CycleCtrlGroup(CTLSEL_DL200P_CYCLE, CTL_DL200P_NUMCYCLES, CTL_DL200P_PERIOD);
		addGroup(ctlgroupCycle, grp);
		SetupCalCtrl(CTLCAL_DL200P_PERIOD, this, CTL_DL200P_PERIOD, 1);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		CycleCtrlGroup::Rec cycle_rec;
		//setCtrlData(CTL_DL200P_FNAME, Data.FileName);
		//setCtrlData(CTL_DL200P_DATANAME, Data.DataName);
		setupFileCombo();
		setupFormCombo();
		SetPeriodInput(this, CTL_DL200P_PERIOD, &Data.Period);
		cycle_rec.C = Data.Cycl;
		setGroupData(ctlgroupCycle, &cycle_rec);
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		CycleCtrlGroup::Rec cycle_rec;
		getSelectedFileName(Data.FileName, sizeof(Data.FileName));
		ComboBox * p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_DL200P_DATANAME));
		if(p_cb)
			p_cb->getInputLineText(Data.DataName, sizeof(Data.DataName));
		else
			Data.DataName[0] = 0;
		//getCtrlData(CTL_DL200P_FNAME, Data.FileName);
		//getCtrlData(CTL_DL200P_DATANAME, Data.DataName);
		GetPeriodInput(this, CTL_DL200P_PERIOD, &Data.Period);
		getGroupData(ctlgroupCycle, &cycle_rec);
		Data.Cycl = cycle_rec.C;
		ASSIGN_PTR(pData, Data);
		return 1;
	}
private:
	DECL_HANDLE_EVENT;
	int    setupFileCombo();
	int    setupFormCombo();
	int    getSelectedFileName(char * pBuf, size_t bufLen);
	char   DdPath[MAXPATH];
};

IMPL_HANDLE_EVENT(DL200_ParamDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_DL200P_FNAME)) {
		setupFormCombo();
		clearEvent(event);
	}
}

int DL200_ParamDialog::getSelectedFileName(char * pBuf, size_t bufLen)
{
	char   fname[MAXPATH];
	ComboBox   * p_cb = 0;
	fname[0] = 0;
	ASSIGN_PTR(pBuf, 0);
	if((p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_DL200P_FNAME))) != 0) {
		p_cb->getInputLineText(fname, sizeof(fname));
		if(fname[0]) {
			SString path;
			PPGetPath(PPPATH_DD, path);
			SPathStruc::ReplaceExt(path.SetLastSlash().Cat(fname), "BIN", 0);
			path.CopyTo(pBuf, bufLen);
			return 1;
		}
	}
	return 0;
}

int DL200_ParamDialog::setupFormCombo()
{
	int    ok = 1;
	char   path[MAXPATH];
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_DL200P_DATANAME))) != 0) {
		SStrCollection data_list;
		long   sel = 0;
		THROW(p_lw = CreateListWindow(48, lbtDisposeData | lbtDblClkNotify));
		getSelectedFileName(path, sizeof(path));
		if(path[0]) {
			uint   i;
			char * p_dataname;
			DL2_Storage strg;
			THROW(strg.Open(path, 1));
			THROW(strg.GetDataEntriesList(&data_list));
			for(i = 0; data_list.enumItems(&i, (void **)&p_dataname);) {
				if(stricmp(p_dataname, Data.DataName) == 0)
					sel = i;
				p_lw->listBox()->addItem(i, p_dataname);
			}
			if(sel == 0 && data_list.getCount() == 1)
				sel = 1;
		}
		p_cb->setListWindow(p_lw, sel);
	}
	CATCHZOK
	return ok;
}

int DL200_ParamDialog::setupFileCombo()
{
	int    ok = 1;
	ComboBox   * p_cb = 0;
	ListWindow * p_lw = 0;
	if((p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_DL200P_FNAME))) != 0) {
		long sel = 0;
		p_lw = CreateListWindow(48, lbtDisposeData | lbtDblClkNotify);
		THROW(p_lw);
		{
			uint   p, i = 0;
			// @v10.3.0 SDirEntry sde;
			SString path, file_path;
			//PPFileNameArray fary;
			SFileEntryPool fep;
			SFileEntryPool::Entry fe;
			THROW(PPGetPath(PPPATH_DD, path));
			//THROW(fary.Scan(path.SetLastSlash(), "*" ".BIN"));
			THROW(fep.Scan(path.SetLastSlash(), "*.BIN", 0));
			//for(p = 0; fary.Enum(&p, &sde, &file_path);)
			for(p = 0; p < fep.GetCount(); p++) {
				if(fep.Get(p, &fe, &file_path)) {
					if(DL2_Storage::IsDL200File(file_path) > 0) {
						if(stricmp(Data.FileName, file_path) == 0)
							sel = i+1;
						p_lw->listBox()->addItem(++i, fe.Name); // @v10.3.0 @fix sde.FileName-->fe.Name
					}
				}
			}
		}
		p_cb->setListWindow(p_lw, sel);
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrDL200::EditParam(Param * pData)
{
	int    ok = -1;
	DL200_ParamDialog * dlg = 0;
	if(CheckDialogPtrErr(&(dlg = new DL200_ParamDialog()))) {
		dlg->setDTS(pData);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(pData)) {
				valid_data = 1;
				ok = 1;
			}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI PrcssrDL200::Init(const Param * pParam)
{
	int    ok = 1;
	P = *pParam;
	D.destroy();
	GStack.freeAll();
	Strg.Close();
	THROW(Strg.Open(P.FileName, 1));
	THROW(Strg.ReadEntry(DL2ENT_DATA, P.DataName, &D));
	CATCHZOK
	return ok;
}

int SLAPI PrcssrDL200::ProcessRow(const DL2_Row * pRow)
{
	int    ok = 1;
	uint   i;
	P_IterTbl->clearDataBuf();
	char * p_buf = static_cast<char *>(P_IterTbl->getDataBuf());
	_DL200_OutpIter * p_iterbuf = reinterpret_cast<_DL200_OutpIter *>(p_buf);
	size_t p = 0;
	p_iterbuf->Period = CurPeriod;
	periodfmt(&CurPeriod, p_iterbuf->PeriodTxt);
	p += sizeof(_DL200_OutpIter);
	uint num_groups = D.GetMaxNesting();
	for(i = 0; i < num_groups; i++) {
		size_t max_descr_len = D.GetMaxDescriptionSize(i+1);
		if(i < GStack.getPointer()) {
			const DL2_Group * p_group = static_cast<const DL2_Group *>(GStack.at(i));
			if(p_group) {
				if(p_group->P_Descript)
					strnzcpy(p_buf+p, p_group->P_Descript, max_descr_len);
				else
					strnzcpy(p_buf+p, p_group->Name, max_descr_len);
			}
		}
		p += max_descr_len;
	}
	for(i = 0; i < D.GetColumnsCount(); i++) {
		const DL2_Column * p_c = D.GetColumn(i);
		TYPEID typ;
		DL2_CI * p_result = 0;
		if(pRow->P_F) {
			THROW(p_result = pRow->P_F->Calc((int)i, this));
		}
		else {
			THROW_MEM(p_result = new DL2_CI);
		}
		if(oneof2(p_c->CiType, DL2CIT_REAL, DL2CIT_ACC)) {
			typ = MKSTYPE(S_FLOAT, 8);
			if(p_result->CiType == DL2CIT_REAL)
				*(double *)(p_buf+p) = p_result->R;
			else if(p_result->CiType == DL2CIT_INT)
				*(double *)(p_buf+p) = p_result->I;
		}
		else if(p_c->CiType == DL2CIT_INT) {
			typ = MKSTYPE(S_INT, 4);
			if(p_result->CiType == DL2CIT_REAL)
				*(long *)(p_buf+p) = (long)p_result->R;
			else if(p_result->CiType == DL2CIT_INT)
				*(long *)(p_buf+p) = p_result->I;
		}
		else if(p_c->CiType == DL2CIT_DATE) {
			typ = MKSTYPE(S_DATE, 4);
			if(p_result->CiType == DL2CIT_DATE)
				*(LDATE *)(p_buf+p) = p_result->D;
		}
		else if(p_c->CiType == DL2CIT_TIME) {
			typ = MKSTYPE(S_TIME, 4);
			if(p_result->CiType == DL2CIT_TIME)
				*(LTIME *)(p_buf+p) = p_result->T;
		}
		else if(p_c->CiType == DL2CIT_STRING) {
			typ = MKSTYPE(S_ZSTRING, p_c->MaxOutSize);
			p_result->ToString(p_buf+p, p_c->MaxOutSize);
		}
		else {
			typ = MKSTYPE(S_ZSTRING, 36);
			p_result->ToString(p_buf+p, 36);
		}
		p += GETSSIZE(typ);
		delete p_result;
	}
	THROW_DB(P_IterTbl->insertRec());
	CATCHZOK
	return ok;
}

int SLAPI PrcssrDL200::ProcessGroup(const DL2_Group * pGrp)
{
	int    ok = 1;
	if(pGrp)
		for(uint i = 0; i < pGrp->GetCount(); i++) {
			DL2_Entry * p_entry = pGrp->GetItem(i);
			if(p_entry->EntryType == DL2ENT_ROW) {
				THROW(ProcessRow((DL2_Row *)p_entry));
			}
			else if(p_entry->EntryType == DL2ENT_GROUP) {
				GStack.push((DL2_Group *)p_entry);
				THROW(ProcessGroup((DL2_Group *)p_entry));
				GStack.pop();
			}
			else
				CALLEXCEPT_PP(PPERR_DL200_INVENTRYTYPE);
		}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrDL200::Print()
{
	int    ok = -1;
	int    reply = 0;
	short  hJob = 0;
	long   fl = 0;
	//char * p_loc_fname = 0;
	//char   report_name[128];
	//char   fn[MAXPATH+1];
	SString fn;
	SString report_name;
	//strupr(strcat(strcpy(report_name, "DL2_"), D.Name));
	(report_name = "DL2_").Cat(D.Name).ToUpper();
	PrnDlgAns pans(report_name);
	if(EditPrintParam(&pans) > 0) {
		//STRNSCPY(fn, pans.Entries.at(pans.Selection)->ReportPath_);
		fn = pans.Entries.at(pans.Selection)->ReportPath_;
		switch(pans.Dest) {
			case PrnDlgAns::aPrint:
				ok = CrystalReportPrint(fn, OutPath, pans.Printer, pans.NumCopies, SPRN_DONTRENAMEFILES, 0);
				break;
			case PrnDlgAns::aPreview:
				ok = CrystalReportPrint(fn, OutPath, pans.Printer, 1, SPRN_PREVIEW|SPRN_DONTRENAMEFILES, 0);
				break;
			case PrnDlgAns::aExport:
				ok = CrystalReportExport(fn, OutPath, pans.ReportName, 0, 0);
				break;
			case -1:
				ok = -1;
				break;
		}
	}
	/*
	if(ok == 0 && SaveDataStruct(report_name, OutPath, fn) < 0)
		PPError(PPERR_CRYSTAL_REPORT);
	*/
	return ok;
}

int SLAPI PrcssrDL200::Run()
{
	int    ok = 1;
	PPCycleArray cycle_list;
	PPWait(1);
	THROW_PP(D.EntryType == DL2ENT_DATA, PPERR_DL200_INVENTRYTYPE);
	if(!P.Cycl)
		cycle_list.freeAll();
	else {
		cycle_list.init(&P.Period, P.Cycl);
		cycle_list.getCycleParams(&P.Period, &P.Cycl);
	}
	THROW(InitOutput());
	{
		PPTransaction tra(1);
		THROW(tra);
		if(cycle_list.getCount()) {
			for(uint i = 0; i < cycle_list.getCount(); i++) {
				cycle_list.getPeriod(i, &CurPeriod);
				THROW(ProcessGroup(&D));
			}
		}
		else {
			CurPeriod = P.Period;
			THROW(ProcessGroup(&D));
		}
		THROW(tra.Commit());
	}
	PPWait(0);
	THROW(Print());
	CATCHZOK
	FinishOutput();
	return ok;
}

int SLAPI ProcessDL200()
{
	PrcssrDL200 prcssr;
	PrcssrDL200::Param param;
	prcssr.InitParam(&param);
	while(prcssr.EditParam(&param) > 0) {
		if(!prcssr.Init(&param) || !prcssr.Run())
			PPError();
	}
	return 1;
}
