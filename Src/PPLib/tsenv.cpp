// TSENV.CPP
// Copyright (c) A.Sobolev 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
/*
ORDER_TYPE_BUY	0
ORDER_TYPE_SELL	1
ORDER_TYPE_BUY_LIMIT	2
ORDER_TYPE_SELL_LIMIT	3
ORDER_TYPE_BUY_STOP	4
ORDER_TYPE_SELL_STOP	5
ORDER_TYPE_BUY_STOP_LIMIT	6
ORDER_TYPE_SELL_STOP_LIMIT	7
ORDER_TYPE_CLOSE_BY	8
TRADE_ACTION_DEAL	1
TRADE_ACTION_PENDING	5
TRADE_ACTION_SLTP	6
TRADE_ACTION_MODIFY	7
TRADE_ACTION_REMOVE	8
TRADE_ACTION_CLOSE_BY	10
ORDER_FILLING_FOK	0
ORDER_FILLING_IOC	1
ORDER_FILLING_RETURN	2
ORDER_TIME_GTC	0
ORDER_TIME_DAY	1
ORDER_TIME_SPECIFIED	2
ORDER_TIME_SPECIFIED_DAY	3
TICK_FLAG_BID	2
TICK_FLAG_ASK	4
TICK_FLAG_LAST	8
TICK_FLAG_VOLUME	16
TICK_FLAG_BUY	32
TICK_FLAG_SELL	64
POSITION_TYPE_BUY	0
POSITION_TYPE_SELL	1
*/

static const int TsStakeEnvironment_CurrentVersion = 2; // @v10.5.4 1-->2

SLAPI TsStakeEnvironment::TerminalInfo::TerminalInfo() : GmtOffset(0)
{
	memzero(Reserve, sizeof(Reserve));
}

SLAPI TsStakeEnvironment::AccountInfo::AccountInfo() : ID(0), ActualDtm(ZERODATETIME), Balance(0.0), Profit(0.0), Margin(0.0), MarginFree(0.0)
{
}

SLAPI TsStakeEnvironment::Stake::Stake()
{
	THISZERO();
}

SLAPI TsStakeEnvironment::StakeRequestBlock::Req::Req()
{
	THISZERO();
}

SLAPI TsStakeEnvironment::StakeRequestBlock::Result::Result()
{
	THISZERO();
}

SLAPI TsStakeEnvironment::StakeRequestBlock::StakeRequestBlock(/*TsStakeEnvironment & rEnv*/) : /*R_Env(rEnv)*/SStrGroup(), Ver(TsStakeEnvironment_CurrentVersion)
{
}

int SLAPI TsStakeEnvironment::StakeRequestBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   ver = Ver;
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	if(dir < 0 && ver < Ver) 
		ok = -1;
	else {
		THROW_SL(pSCtx->Serialize(dir, &L, rBuf));
		THROW_SL(pSCtx->Serialize(dir, &RL, rBuf));
		THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	}
	CATCHZOK
	return ok;
}

SLAPI TsStakeEnvironment::TsStakeEnvironment() : SStrGroup(), Ver(TsStakeEnvironment_CurrentVersion)
{
}

SLAPI TsStakeEnvironment::TsStakeEnvironment(const TsStakeEnvironment & rS) : SStrGroup(), Ver(TsStakeEnvironment_CurrentVersion)
{
	Copy(rS);
}

TsStakeEnvironment & FASTCALL TsStakeEnvironment::operator = (const TsStakeEnvironment & rS)
{
	Copy(rS);
	return *this;
}

int SLAPI TsStakeEnvironment::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   ver = Ver;
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	if(dir < 0 && ver < Ver) {
		ok = -1;
	}
	else {
		THROW_SL(pSCtx->SerializeBlock(dir, sizeof(Term), &Term, rBuf, 1)); // @v10.5.4
		THROW_SL(pSCtx->Serialize(dir, Acc.ID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Acc.ActualDtm, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Acc.Balance, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Acc.Profit, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Acc.Margin, rBuf));
		THROW_SL(pSCtx->Serialize(dir, Acc.MarginFree, rBuf));
		THROW_SL(pSCtx->Serialize(dir, &TL, rBuf));
		THROW_SL(pSCtx->Serialize(dir, &SL, rBuf));
		THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	}
	CATCHZOK
	return ok;
}

int FASTCALL TsStakeEnvironment::Copy(const TsStakeEnvironment & rS)
{
	int    ok = 1;
	SStrGroup::CopyS(rS);
	Term = rS.Term; // @v10.5.4
	Acc = rS.Acc;
	TL = rS.TL;
	SL = rS.SL;
	return ok;
}

const TsStakeEnvironment::Tick * FASTCALL TsStakeEnvironment::SearchTickBySymb(const char * pSymb) const
{
	const Tick * p_result = 0;
	if(!isempty(pSymb)) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		for(uint i = 0; !p_result && i < TL.getCount(); i++) {
			const Tick & r_tk = TL.at(i);
			GetS(r_tk.SymbP, r_temp_buf);
			if(r_temp_buf.IsEqiAscii(pSymb))
				p_result = &r_tk;
		}
	}
	return p_result;
}
//
//
//
SLAPI TsStakeEnvironment::TransactionNotification::TransactionNotification() : SStrGroup(), Ver(TsStakeEnvironment_CurrentVersion)
{
}

int SLAPI TsStakeEnvironment::TransactionNotification::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   ver = Ver;
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	if(dir < 0 && ver < Ver) {
		ok = -1;
	}
	else {
		THROW_SL(pSCtx->Serialize(dir, &L, rBuf));
		THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	}
	CATCHZOK
	return ok;
}
