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
	{ PPBZSI_ORDCANCELLATIONRATE,    "ordcancellationrate" }, // @v12.1.11
	{ PPBZSI_SALERETCOUNT,           "saleretcount" }, // @v12.1.11
	{ PPBZSI_SALERETQTTY,            "saleretqtty" }, // @v12.1.11
	{ PPBZSI_SALERETCOST,            "saleretcost" }, // @v12.1.11
	{ PPBZSI_MPPROMOTION,            "mppromotion" }, // @v12.1.12
};

static const SIntToSymbTabEntry BzsIDescrList[] = { // @v12.1.11
	{ PPBZSI_AMOUNT,                 "bzsi_amount" },
	{ PPBZSI_COST,                   "bzsi_cost" },
	{ PPBZSI_PRICE,                  "bzsi_price" },
	{ PPBZSI_DISCOUNT,               "bzsi_discount" },
	{ PPBZSI_NETPRICE,               "bzsi_netprice" },
	{ PPBZSI_MARGIN,                 "bzsi_margin" },
	{ PPBZSI_PCTINCOME,              "bzsi_pctincome" },
	{ PPBZSI_PCTMARGIN,              "bzsi_pctmargin" },
	{ PPBZSI_COUNT,                  "bzsi_count" },
	{ PPBZSI_AVERAGE,                "bzsi_average" },
	{ PPBZSI_MPACCEPTANCE,           "bzsi_mpacceptance" },
	{ PPBZSI_MPSTORAGE,              "bzsi_mpstorage" },
	{ PPBZSI_MPCOMMISSION,           "bzsi_mpcommission" },
	{ PPBZSI_MPCOMMISSIONPCT,        "bzsi_mpcommissionpct" },
	{ PPBZSI_MPSELLERSPART,          "bzsi_mpsellerspart" },
	{ PPBZSI_MPSELLERSPARTPCT,       "bzsi_mpsellerspartpct" },
	{ PPBZSI_MPACQUIRING,            "bzsi_mpacquiring" },
	{ PPBZSI_MPACQUIRINGPCT,         "bzsi_mpacquiringpct" },
	{ PPBZSI_ORDCOUNT,               "bzsi_ordcount" },
	{ PPBZSI_ORDQTTY,                "bzsi_ordqtty" },
	{ PPBZSI_SALECOUNT,              "bzsi_salecount" },
	{ PPBZSI_SALEQTTY,               "bzsi_saleqtty" },
	{ PPBZSI_ORDCANCELLEDCOUNT,      "bzsi_ordcancelledcount" },
	{ PPBZSI_ORDCANCELLEDQTTY,       "bzsi_ordcancelledqtty" },
	{ PPBZSI_ORDSHIPMDELAYDAYSAVG,   "bzsi_ordshipmdelaydaysavg" },
	{ PPBZSI_ORDSHIPMDELAYDAYSMIN,   "bzsi_ordshipmdelaydaysmin" },
	{ PPBZSI_ORDSHIPMDELAYDAYSMAX,   "bzsi_ordshipmdelaydaysmax" },
	{ PPBZSI_SUPPLSHIPMDELAYDAYSAVG, "bzsi_supplshipmdelaydaysavg" },
	{ PPBZSI_ORDSHIPMDELAYDAYS,      "bzsi_ordshipmdelaydays" },
	{ PPBZSI_SUPPLSHIPMDELAYDAYS,    "bzsi_supplshipmdelaydays" },
	{ PPBZSI_MPAMT_ORDPRICE,         "bzsi_mpamt_ordprice" },
	{ PPBZSI_MPAMT_ORDSELLERPRICE,   "bzsi_mpamt_ordsellerprice" },
	{ PPBZSI_MPAMT_SHIPMPRICE,       "bzsi_mpamt_shipmprice" },
	{ PPBZSI_MPAMT_SHIPMSELLERPRICE, "bzsi_mpamt_shipmsellerprice" },
	{ PPBZSI_SALECOST,               "bzsi_salecost" },
	{ PPBZSI_FREIGHT,                "bzsi_freight" },
	{ PPBZSI_ORDCANCELLATIONRATE,    "bzsi_ordcancellationrate" }, // @v12.1.11
	{ PPBZSI_SALERETCOUNT,           "bzsi_saleretcount" }, // @v12.1.11
	{ PPBZSI_SALERETQTTY,            "bzsi_saleretqtty" }, // @v12.1.11
	{ PPBZSI_SALERETCOST,            "bzsi_saleretcost" }, // @v12.1.11
	{ PPBZSI_MPPROMOTION,            "bzsi_mppromotion" }, // @v12.1.12
};

static const int BzsINonAdditiveList[] = {
	PPBZSI_PCTINCOME, PPBZSI_PCTMARGIN, PPBZSI_MPCOMMISSIONPCT, 
	PPBZSI_MPSELLERSPARTPCT, PPBZSI_MPACQUIRINGPCT, PPBZSI_ORDSHIPMDELAYDAYSAVG, PPBZSI_ORDSHIPMDELAYDAYSMIN, 
	PPBZSI_ORDSHIPMDELAYDAYSMAX, PPBZSI_SUPPLSHIPMDELAYDAYSAVG, PPBZSI_ORDCANCELLATIONRATE
};

/*static*/bool PPObjBizScore::IsBzsiAdditive(int bzsi)
{
	for(uint i = 0; i < SIZEOFARRAY(BzsINonAdditiveList); i++) {
		if(BzsINonAdditiveList[i] == bzsi)
			return false;
	}
	return true;
}

/*static*/const char * PPObjBizScore::GetBzsiSymb(int bzsi, SString & rBuf)
{
	const char * p_result = SIntToSymbTab_GetSymbPtr(BzsISymList, SIZEOFARRAY(BzsISymList), bzsi);
	(rBuf = p_result);
	return p_result;
}

/*static*/const char * PPObjBizScore::GetBzsiDescr(int bzsi, SString & rBuf)
{
	rBuf.Z();
	const char * p_result = SIntToSymbTab_GetSymbPtr(BzsIDescrList, SIZEOFARRAY(BzsIDescrList), bzsi);
	if(p_result) {
		PPLoadString(p_result, rBuf);
	}
	return rBuf;
}

/*static*/int PPObjBizScore::RecognizeBzsiSymb(const char * pSymb)
{
	return SIntToSymbTab_GetId(BzsISymList, SIZEOFARRAY(BzsISymList), pSymb);
}
