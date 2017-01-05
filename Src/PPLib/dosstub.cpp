// DOSSTUB.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2009
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop

int SLAPI DosStubMsg()
{
	PPMessage(mfInfo|mfOK, PPINF_DOSSTUB, 0);
	return -1;
}

#define DOSSTUBFUNC(func) func { return DosStubMsg(); }

DOSSTUBFUNC(int SLAPI ViewTrfrAnlz(const TrfrAnlzFilt *))
DOSSTUBFUNC(int SLAPI ViewPredictSales(PredictSalesFilt *))
DOSSTUBFUNC(int SLAPI FillPredictSales())
DOSSTUBFUNC(int SLAPI ViewShipmAnalyze(ShipmAnalyzeFilt *))
DOSSTUBFUNC(int SLAPI ViewGoodsTaxAnalyze(const GoodsTaxAnalyzeFilt *))
DOSSTUBFUNC(int SLAPI ViewSCard(const SCardFilt *, int))
DOSSTUBFUNC(int SLAPI ViewGoodsMov(const GoodsMovFilt *))
DOSSTUBFUNC(int SLAPI ViewGoodsTurnover(long))
DOSSTUBFUNC(int SLAPI ViewGoodsOpAnalyze(const GoodsOpAnalyzeFilt *))
DOSSTUBFUNC(int SLAPI EditPriceListConfig())
DOSSTUBFUNC(int SLAPI ViewPriceList(const PriceListFilt *))
DOSSTUBFUNC(int SLAPI ViewOpGrouping(const OpGroupingFilt *))
DOSSTUBFUNC(int SLAPI EditHolidays());
DOSSTUBFUNC(int SLAPI AnalyzeObjSyncCmp(int));
DOSSTUBFUNC(int SLAPI ViewBalance(const BalanceFilt *));

DOSSTUBFUNC(int SLAPI PPObjBHT::TransmitData());
DOSSTUBFUNC(int SLAPI PPObjBHT::TransmitProgram());
DOSSTUBFUNC(int SLAPI PPObjBHT::ReceiveData());

