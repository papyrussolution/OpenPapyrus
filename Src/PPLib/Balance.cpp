// BALANCE.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000-2002, 2003, 2004, 2007, 2008, 2009, 2012, 2014, 2015, 2016
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SLAPI Balance::Balance(char * fName) : BalanceTbl(fName)
{
}

double SLAPI Balance::_Rest() const
{
	return (MONEYTOLDBL(data.DbtRest) - MONEYTOLDBL(data.CrdRest));
}

double SLAPI Balance::_Rest(int side) const
{
	return ((side == PPDEBIT) ? MONEYTOLDBL(data.DbtRest) : MONEYTOLDBL(data.CrdRest));
}

void SLAPI Balance::_SetRest(int side, double val)
{
	if(side == PPDEBIT)
		LDBLTOMONEY(val, data.DbtRest);
	else
		LDBLTOMONEY(val, data.CrdRest);
}
//
// Флаги для функции Balance::_Turn
//
#define USE_TRANSACTION 0x0001
#define TURN_ROLLBACK   0x0002

int SLAPI Balance::Turn(PPID bal, LDATE date, AccTurnParam * p, int use_ta)
{
	return _Turn(bal, date, p, use_ta ? USE_TRANSACTION : 0);
}

int SLAPI Balance::RollbackTurn(PPID bal, LDATE date, AccTurnParam * p, int use_ta)
{
	return _Turn(bal, date, p, (use_ta ? USE_TRANSACTION : 0) | TURN_ROLLBACK);
}

int SLAPI Balance::Search(PPID * pAccID, LDATE * pDt, int spMode)
{
	BalanceTbl::Key1 k1;
	k1.AccID = *pAccID;
	k1.Dt    = *pDt;
	if(search(1, &k1, spMode)) {
		*pAccID  = k1.AccID;
		*pDt = k1.Dt;
		return 1;
	}
	else
		return PPDbSearchError();
}

int SLAPI Balance::_Turn(PPID bal, LDATE date, AccTurnParam * param, uint flags)
{
	int    ok = 1;
	int    r, rollback = BIN(flags & TURN_ROLLBACK);
	// Получаем оригинальную сумму проводки
	double abs_amt = (param->Side == PPCREDIT) ? -param->Amt : param->Amt;
	double new_rest;
	BalanceTbl::Key1 k1;
	{
		PPTransaction tra(BIN(flags & USE_TRANSACTION));
		THROW(tra);
		k1.AccID = bal;
		k1.Dt = date;
		THROW(r = search(1, &k1, spEq) ? 1 : PPDbSearchError()); // @v8.1.4 searchForUpdate-->search
		if(r > 0) {
			if(abs_amt != 0.0) {
				// @v8.1.4 {
				DBRowId _dbpos;
				THROW_DB(getPosition(&_dbpos));
				THROW_DB(getDirectForUpdate(1, &k1, _dbpos));
				// } @v8.1.4
				new_rest = _Rest()+param->Amt;
				if(LConfig.Flags & CFGFLG_CHECKTURNREST) {
					THROW_PP(new_rest >= param->Low, PPERR_BALLOWBOUND);
					THROW_PP(new_rest <= param->Upp, PPERR_BALUPPBOUND);
					if(rollback) {
						THROW_PP((_Rest(param->Side) + abs_amt) >= 0, PPERR_BALROLLBACKBOUND);
					}
				}
				_SetRest(param->Side, _Rest(param->Side) + abs_amt);
				THROW_DB(updateRec()); // @sfu
			}
		}
		else {
			THROW_PP(rollback == 0, PPERR_DATEBALNFOUND);
			k1.AccID = bal;
			k1.Dt = date;
			r = search(1, &k1, spLt) ? 1 : (BTROKORNFOUND ? -1 : 0);
			THROW_DB(r);
			if(r < 0 || k1.AccID != bal) {
				_SetRest(PPCREDIT, (param->Side == PPDEBIT)  ? 0.0 : abs_amt);
				_SetRest(PPDEBIT,  (param->Side == PPCREDIT) ? 0.0 : abs_amt);
			}
			else
				_SetRest(param->Side, _Rest(param->Side) + abs_amt);
			data.Dt  = date;
			data.AccID = bal;
			THROW_DB(insertRec());
		}
		if(!BillObj->atobj->P_Tbl->IsFRRLocked() && abs_amt != 0.0) {
			k1.AccID = bal;
			k1.Dt = date;
			while((r = search(1, &k1, spGt)) != 0 && k1.AccID == bal) { // @v8.2.0 searchForUpdate-->search
				new_rest = _Rest()+param->Amt;
				if(LConfig.Flags & CFGFLG_CHECKTURNREST) {
					THROW_PP(new_rest >= param->Low, PPERR_FWBALLOWBOUND);
					THROW_PP(new_rest <= param->Upp, PPERR_FWBALUPPBOUND);
					if(rollback)
						THROW_PP((_Rest(param->Side) + abs_amt) >= 0, PPERR_BALROLLBACKBOUND);
				}
				THROW_DB(rereadForUpdate(1, &k1)); // @v8.2.0
				_SetRest(param->Side, _Rest(param->Side) + abs_amt);
				THROW_DB(updateRec()); // @sfu
			}
			THROW_DB(BTROKORNFOUND);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI Balance::GetRest(PPID accID, LDATE dt, double * pDbt, double * pCrd)
{
	PPID acc_id = accID;
	int  r = Search(&acc_id, &dt, spLe);
	if(r > 0 && accID == acc_id) {
		*pDbt = _Rest(PPDEBIT);
		*pCrd = _Rest(PPCREDIT);
	}
	else
		*pDbt = *pCrd = 0;
	return BIN(r);
}

int SLAPI Balance::GetTurnover(PPID bal, LDATE beg, LDATE end, double * dbt, double * crd)
{
	int    ok = 1;
	double d, c;
	THROW(GetRest(bal, end, &d, &c));
	THROW(GetRest(bal, plusdate(beg, -1), dbt, crd));
	*dbt -= d;
	*crd -= c;
	CATCHZOK
	return ok;
}

int SLAPI Balance::GetBalance(PPID bal, LDATE beg, LDATE end, double row[])
{
	double d, c;
	SETIFZ(end, encodedate(31, 12, 3000));
	if(!GetRest(bal, end, &row[2], &row[3]))
		return 0;
	if(!GetRest(bal, beg ? plusdate(beg, -1) : ZERODATE, &d, &c))
		return 0;
	row[0] = row[2] - d;
	row[1] = row[3] - c;
	return 1;
}
