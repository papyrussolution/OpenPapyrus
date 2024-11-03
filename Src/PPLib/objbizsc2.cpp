// OBJBIZSC2.CPP
// Copyright (c) A.Sobolev 2024
// @codepage UTF-8
// „асть модул€ PPObjBizScore выделена в отдельный файл дл€ компил€ции инструменальных процессов без полной линковки с PapyrusLib
//
#include <pp.h>
#pragma hdrstop

static const SIntToSymbTabEntry BzsISymList[] = { // @v12.1.6
	//{ PPBZSI_NONE,                   "" },
	{ PPBZSI_AMOUNT,                 "amount" },
	{ PPBZSI_COST,                   "cost" },
	{ PPBZSI_PRICE,                  "price" },
	{ PPBZSI_DISCOUNT,               "discount" },
	{ PPBZSI_NETPRICE,               "netprice" },
	{ PPBZSI_MARGIN,                 "margin" },
	{ PPBZSI_PCTINCOME,              "pctincome" },
	{ PPBZSI_PCTMARGIN,              "pctmargin" },
	{ PPBZSI_COUNT,                  "count" },
	{ PPBZSI_AVERAGE,                "average" },
	{ PPBZSI_MPACCEPTANCE,           "mpacceptance" },
	{ PPBZSI_MPSTORAGE,              "mpstorage" },
	{ PPBZSI_MPCOMMISSION,           "mpcommission" },
	{ PPBZSI_MPCOMMISSIONPCT,        "mpcommissionpct" },
	{ PPBZSI_MPSELLERSPART,          "mpsellerspart" },
	{ PPBZSI_MPSELLERSPARTPCT,       "mpsellerspartpct" },
	{ PPBZSI_MPACQUIRING,            "mpacquiring" },
	{ PPBZSI_MPACQUIRINGPCT,         "mpacquiringpct" },
	{ PPBZSI_ORDCOUNT,               "ordcount" },
	{ PPBZSI_ORDQTTY,                "ordqtty" },
	{ PPBZSI_SALECOUNT,              "salecount" },
	{ PPBZSI_SALEQTTY,               "saleqtty" },
	{ PPBZSI_ORDCANCELLEDCOUNT,      "ordcancelledcount" },
	{ PPBZSI_ORDCANCELLEDQTTY,       "ordcancelledqtty" },
	{ PPBZSI_ORDSHIPMDELAYDAYSAVG,   "ordshipmdelaydaysavg" },
	{ PPBZSI_ORDSHIPMDELAYDAYSMIN,   "ordshipmdelaydaysmin" },
	{ PPBZSI_ORDSHIPMDELAYDAYSMAX,   "ordshipmdelaydaysmax" },
	{ PPBZSI_SUPPLSHIPMDELAYDAYSAVG, "supplshipmdelaydaysavg" },
	{ PPBZSI_ORDSHIPMDELAYDAYS,      "ordshipmdelaydays" },
	{ PPBZSI_SUPPLSHIPMDELAYDAYS,    "supplshipmdelaydays" },
	{ PPBZSI_MPAMT_ORDPRICE,         "mpamtordprice" },
	{ PPBZSI_MPAMT_ORDSELLERPRICE,   "mpamtordsellerprice" },
	{ PPBZSI_MPAMT_SHIPMPRICE,       "mpamtshipmprice" },
	{ PPBZSI_MPAMT_SHIPMSELLERPRICE, "mpamtshipmsellerprice" },
	{ PPBZSI_SALECOST,               "salecost" },
	{ PPBZSI_FREIGHT,                "freight" },
};

/*static*/const char * PPObjBizScore::GetBzsiSymb(int bzsi, SString & rBuf)
{
	const char * p_result = SIntToSymbTab_GetSymbPtr(BzsISymList, SIZEOFARRAY(BzsISymList), bzsi);
	(rBuf = p_result);
	return p_result;
}

/*static*/int PPObjBizScore::RecognizeBzsiSymb(const char * pSymb)
{
	return SIntToSymbTab_GetId(BzsISymList, SIZEOFARRAY(BzsISymList), pSymb);
}
