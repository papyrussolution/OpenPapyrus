// CURRATE.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2005, 2007, 2010, 2016
//
#include <pp.h>
#pragma hdrstop

SLAPI CurRateCore::CurRateCore() : CurrencyRateTbl()
{
}

int SLAPI CurRateCore::SearchRate(const CurRateIdent * pIdent, CurrencyRateTbl::Rec * pRec)
{
	CurrencyRateTbl::Key0 k;
	k.CurID      = pIdent->CurID;
	k.RateTypeID = pIdent->RateTypeID;
	k.RelCurID   = pIdent->BaseCurID;
	k.Dt         = pIdent->Dt;
	return SearchByKey(this, 0, &k, pRec);
}

int SLAPI CurRateCore::GetRate(PPID curID, PPID rateTypeID, PPID relCurID, LDATE * pDt, double * pRate)
{
	int    ok   = -1;
	double rate = 0.0;
	CurrencyRateTbl::Key1 k;
	LDATE  dt = pDt ? *pDt : MAXDATE;
	if(curID == LConfig.BaseCurID)
		rate = 1.0;
	else {
		k.CurID      = curID;
		k.RateTypeID = rateTypeID;
		k.RelCurID   = relCurID;
		k.Dt         = dt;
		if(search(1, &k, spLe) && k.CurID == curID && k.RateTypeID == rateTypeID && k.RelCurID == relCurID) {
			dt = k.Dt;
			rate = data.Rate;
		}
		else
			ok = PPDbSearchError();
	}
	ASSIGN_PTR(pRate, rate);
	ASSIGN_PTR(pDt,   dt);
	return ok;
}

int SLAPI CurRateCore::UpdateRate(const CurRateIdent * pIdent, double rate, int use_ta)
{
	int    ok = 1;
	CurrencyRateTbl::Key0 k;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		k.CurID      = pIdent->CurID;
		k.RateTypeID = pIdent->RateTypeID;
		k.RelCurID   = pIdent->BaseCurID;
		k.Dt         = pIdent->Dt;
		if(SearchByKey_ForUpdate(this, 0, &k, 0) > 0) {
			data.Rate = rate;
			THROW_DB(updateRec()); // @sfu
		}
		else {
			clearDataBuf();
			data.CurID      = pIdent->CurID;
			data.RateTypeID = pIdent->RateTypeID;
			data.RelCurID   = pIdent->BaseCurID;
			data.Dt         = pIdent->Dt;
			data.Rate       = rate;
			THROW_DB(insertRec());
		}
		THROW(tra.Commit())
	}
	CATCHZOK
	return ok;
}

int SLAPI CurRateCore::RemoveRate(const CurRateIdent * pIdent, int use_ta)
{
	return deleteFrom(this, use_ta, this->CurID == pIdent->CurID && this->RateTypeID == pIdent->RateTypeID &&
		this->RelCurID == pIdent->BaseCurID && this->Dt == pIdent->Dt) ? 1 : PPSetErrorDB();
}
