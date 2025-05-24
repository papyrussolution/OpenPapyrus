// PP-IFM.CPP
// Copyright (c) A.Sobolev 2025
//
#pragma hdrstop
#include <pp-ifm.h>

CcFiscalCorrection::CcFiscalCorrection() : AmtCash(0.0), AmtBank(0.0), AmtPrepay(0.0), AmtPostpay(0.0), AmtReckon(0.0),
	AmtVat20(0.0), AmtVat18(0.0), AmtVat10(0.0), AmtVat07(0.0), AmtVat05(0.0), AmtVat00(0.0), AmtNoVat(0.0), VatRate(0.0), Dt(ZERODATE), Flags(0)
{
}