// prc_billautocreate.cpp
// Copyright (c) A.Sobolev 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI PPBillAutoCreateParam::PPBillAutoCreateParam() : PPBaseFilt(PPFILT_BILLAUTOCREATEPARAM, 0, 0)
{
	P_TaF = 0;
    P_SsF = 0;
    P_CcF = 0;
    P_CsF = 0;
	SetFlatChunk(offsetof(PPBillAutoCreateParam, ReserveStart),
		offsetof(PPBillAutoCreateParam, Reserve)-offsetof(PPBillAutoCreateParam, ReserveStart)+sizeof(Reserve));
	SetBranchBaseFiltPtr(PPFILT_TRFRANLZ, offsetof(PPBillAutoCreateParam, P_TaF));
	SetBranchBaseFiltPtr(PPFILT_SSTAT,    offsetof(PPBillAutoCreateParam, P_SsF));
	SetBranchBaseFiltPtr(PPFILT_CCHECK,   offsetof(PPBillAutoCreateParam, P_CcF));
	SetBranchBaseFiltPtr(PPFILT_CSESS,    offsetof(PPBillAutoCreateParam, P_CsF));
	Init(1, 0);
}

SLAPI PrcssrBillAutoCreate::PrcssrBillAutoCreate()
{
    P_TaV = 0;
    P_SsV = 0;
    P_CcV = 0;
    P_CsV = 0;
}

SLAPI PrcssrBillAutoCreate::~PrcssrBillAutoCreate()
{
    ZDELETE(P_TaV);
    ZDELETE(P_SsV);
    ZDELETE(P_CcV);
    ZDELETE(P_CsV);
}

int SLAPI PrcssrBillAutoCreate::InitParam(PPBillAutoCreateParam *)
{
	int    ok = -1;
	return ok;
}

int SLAPI PrcssrBillAutoCreate::EditParam(PPBillAutoCreateParam *)
{
	int    ok = -1;
	return ok;
}

int SLAPI PrcssrBillAutoCreate::Init(const PPBillAutoCreateParam *)
{
	int    ok = -1;
	return ok;
}

int SLAPI PrcssrBillAutoCreate::Run()
{
	int    ok = -1;
	return ok;
}
