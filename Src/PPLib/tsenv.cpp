// TSENV.CPP
// Copyright (c) A.Sobolev 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI TsStakeEnvironment::AccountInfo::AccountInfo() : ID(0), ActualDtm(ZERODATETIME), Balance(0.0), Profit(0.0), MarginFree(0.0)
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

SLAPI TsStakeEnvironment::StakeRequestBlock::StakeRequestBlock(TsStakeEnvironment & rEnv) : R_Env(rEnv)
{
}

int SLAPI TsStakeEnvironment::StakeRequestBlock::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	long   ver = R_Env.Ver;
	THROW_SL(pSCtx->Serialize(dir, ver, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &L, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &RL, rBuf));
	THROW_SL(dynamic_cast<SStrGroup &>(R_Env).SerializeS(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

static const int TsStakeEnvironment_CurrentVersion = 1;

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
	THROW_SL(pSCtx->Serialize(dir, Acc.ID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Acc.ActualDtm, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Acc.Balance, rBuf));
	THROW_SL(pSCtx->Serialize(dir, Acc.Profit, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &TL, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &SL, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int FASTCALL TsStakeEnvironment::Copy(const TsStakeEnvironment & rS)
{
	int    ok = 1;
	SStrGroup::CopyS(rS);
	Acc = rS.Acc;
	TL = rS.TL;
	SL = rS.SL;
	return ok;
}

