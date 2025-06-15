// OBJBIZSC2.CPP
// Copyright (c) A.Sobolev 2024, 2025
// @codepage UTF-8
// „асть модул€ PPObjBizScore выделена в отдельный файл дл€ компил€ции инструменальных процессов без полной линковки с PapyrusLib
//
#include <pp.h>
#pragma hdrstop

struct BzsIEntry {
	PPID   ID;
	const  char * P_Symb;
	const  char * P_DescrSymb;
	uint   AggrFunc; // AGGRFUNC_XXX 
};

static const BzsIEntry BzsIList[] = {
	{ PPBZSI_AMOUNT,                 "amount"                  , "bzsi_amount",                 AGGRFUNC_SUM },
	{ PPBZSI_AMT_COST,               "cost"                    , "bzsi_cost",                   AGGRFUNC_SUM },
	{ PPBZSI_AMT_PRICE,              "price"                   , "bzsi_price",                  AGGRFUNC_SUM },
	{ PPBZSI_AMT_DISCOUNT,           "discount"                , "bzsi_discount",               AGGRFUNC_SUM },
	{ PPBZSI_AMT_NETPRICE,           "netprice"                , "bzsi_netprice",               AGGRFUNC_SUM },
	{ PPBZSI_AMT_MARGIN,             "margin"                  , "bzsi_margin",                 AGGRFUNC_SUM },
	{ PPBZSI_PCTINCOME,              "pctincome"               , "bzsi_pctincome",              AGGRFUNC_NONE },
	{ PPBZSI_PCTMARGIN,              "pctmargin"               , "bzsi_pctmargin",              AGGRFUNC_NONE },
	{ PPBZSI_COUNT,                  "count"                   , "bzsi_count",                  AGGRFUNC_SUM },
	{ PPBZSI_AVERAGE,                "average"                 , "bzsi_average",                AGGRFUNC_NONE },
	{ PPBZSI_MPACCEPTANCE,           "mpacceptance"            , "bzsi_mpacceptance",           AGGRFUNC_SUM },
	{ PPBZSI_MPSTORAGE,              "mpstorage"               , "bzsi_mpstorage",              AGGRFUNC_SUM },
	{ PPBZSI_MPCOMMISSION,           "mpcommission"            , "bzsi_mpcommission",           AGGRFUNC_SUM },
	{ PPBZSI_MPCOMMISSIONPCT,        "mpcommissionpct"         , "bzsi_mpcommissionpct",        AGGRFUNC_NONE },
	{ PPBZSI_MPSELLERSPART,          "mpsellerspart"           , "bzsi_mpsellerspart",          AGGRFUNC_SUM },
	{ PPBZSI_MPSELLERSPARTPCT,       "mpsellerspartpct"        , "bzsi_mpsellerspartpct",       AGGRFUNC_NONE },
	{ PPBZSI_MPACQUIRING,            "mpacquiring"             , "bzsi_mpacquiring",            AGGRFUNC_SUM },
	{ PPBZSI_MPACQUIRINGPCT,         "mpacquiringpct"          , "bzsi_mpacquiringpct",         AGGRFUNC_NONE },
	{ PPBZSI_ORDCOUNT,               "ordcount"                , "bzsi_ordcount",               AGGRFUNC_SUM },
	{ PPBZSI_ORDQTTY,                "ordqtty"                 , "bzsi_ordqtty",                AGGRFUNC_SUM },
	{ PPBZSI_SALECOUNT,              "salecount"               , "bzsi_salecount",              AGGRFUNC_SUM },
	{ PPBZSI_SALEQTTY,               "saleqtty"                , "bzsi_saleqtty",               AGGRFUNC_SUM },
	{ PPBZSI_ORDCANCELLEDCOUNT,      "ordcancelledcount"       , "bzsi_ordcancelledcount",      AGGRFUNC_SUM },
	{ PPBZSI_ORDCANCELLEDQTTY,       "ordcancelledqtty"        , "bzsi_ordcancelledqtty",       AGGRFUNC_SUM },
	{ PPBZSI_ORDSHIPMDELAYDAYSAVG,   "ordshipmdelaydaysavg"    , "bzsi_ordshipmdelaydaysavg",   AGGRFUNC_NONE },
	{ PPBZSI_ORDSHIPMDELAYDAYSMIN,   "ordshipmdelaydaysmin"    , "bzsi_ordshipmdelaydaysmin",   AGGRFUNC_NONE },
	{ PPBZSI_ORDSHIPMDELAYDAYSMAX,   "ordshipmdelaydaysmax"    , "bzsi_ordshipmdelaydaysmax",   AGGRFUNC_NONE },
	{ PPBZSI_SUPPLSHIPMDELAYDAYSAVG, "supplshipmdelaydaysavg"  , "bzsi_supplshipmdelaydaysavg", AGGRFUNC_NONE },
	{ PPBZSI_ORDSHIPMDELAYDAYS,      "ordshipmdelaydays"       , "bzsi_ordshipmdelaydays",      AGGRFUNC_SUM },
	{ PPBZSI_SUPPLSHIPMDELAYDAYS,    "supplshipmdelaydays"     , "bzsi_supplshipmdelaydays",    AGGRFUNC_SUM },
	{ PPBZSI_MPAMT_ORDPRICE,         "mpamtordprice"           , "bzsi_mpamt_ordprice",         AGGRFUNC_SUM },
	{ PPBZSI_MPAMT_ORDSELLERPRICE,   "mpamtordsellerprice"     , "bzsi_mpamt_ordsellerprice",   AGGRFUNC_SUM },
	{ PPBZSI_MPAMT_SHIPMPRICE,       "mpamtshipmprice"         , "bzsi_mpamt_shipmprice",       AGGRFUNC_SUM },
	{ PPBZSI_MPAMT_SHIPMSELLERPRICE, "mpamtshipmsellerprice"   , "bzsi_mpamt_shipmsellerprice", AGGRFUNC_SUM },
	{ PPBZSI_SALECOST,               "salecost"                , "bzsi_salecost",               AGGRFUNC_SUM },
	{ PPBZSI_FREIGHT,                "freight"                 , "bzsi_freight",                AGGRFUNC_SUM },
	{ PPBZSI_ORDCANCELLATIONRATE,    "ordcancellationrate"     , "bzsi_ordcancellationrate",    AGGRFUNC_NONE }, // @v12.1.11
	{ PPBZSI_SALERETCOUNT,           "saleretcount"            , "bzsi_saleretcount",           AGGRFUNC_SUM }, // @v12.1.11
	{ PPBZSI_SALERETQTTY,            "saleretqtty"             , "bzsi_saleretqtty",            AGGRFUNC_SUM }, // @v12.1.11
	{ PPBZSI_SALERETCOST,            "saleretcost"             , "bzsi_saleretcost",            AGGRFUNC_SUM }, // @v12.1.11
	{ PPBZSI_MPPROMOTION,            "mppromotion"             , "bzsi_mppromotion",            AGGRFUNC_SUM }, // @v12.1.12
};

/* @v12.3.6 
static const SIntToSymbTabEntry BzsISymList[] = { // @v12.1.6
	//{ PPBZSI_NONE,                   "" },
	{ PPBZSI_AMOUNT,                 "amount" },
	{ PPBZSI_AMT_COST,                   "cost" },
	{ PPBZSI_AMT_PRICE,                  "price" },
	{ PPBZSI_AMT_DISCOUNT,               "discount" },
	{ PPBZSI_AMT_NETPRICE,               "netprice" },
	{ PPBZSI_AMT_MARGIN,                 "margin" },
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
	{ PPBZSI_AMT_COST,                   "bzsi_cost" },
	{ PPBZSI_AMT_PRICE,                  "bzsi_price" },
	{ PPBZSI_AMT_DISCOUNT,               "bzsi_discount" },
	{ PPBZSI_AMT_NETPRICE,               "bzsi_netprice" },
	{ PPBZSI_AMT_MARGIN,                 "bzsi_margin" },
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

bool PPObjBizScore::IsBzsiAdditive(int bzsi)
{
	for(uint i = 0; i < SIZEOFARRAY(BzsINonAdditiveList); i++) {
		if(BzsINonAdditiveList[i] == bzsi)
			return false;
	}
	return true;
}*/

/*static*/uint PPObjBizScore::GetBzsiAggrFunc(int bzsi) // @v12.3.6
{
	uint   result = 0;
	for(uint i = 0; !result && i < SIZEOFARRAY(BzsIList); i++) {
		const BzsIEntry & r_entry = BzsIList[i];
		if(r_entry.ID == bzsi)
			result = r_entry.AggrFunc;
	}
	return result;
}

/*static*/const char * PPObjBizScore::GetBzsiSymb(int bzsi, SString & rBuf)
{
	const char * p_result = 0;
	for(uint i = 0; !p_result && i < SIZEOFARRAY(BzsIList); i++) {
		const BzsIEntry & r_entry = BzsIList[i];
		if(r_entry.ID == bzsi)
			p_result = r_entry.P_Symb;
	}
	//const char * p_result = SIntToSymbTab_GetSymbPtr(BzsISymList, SIZEOFARRAY(BzsISymList), bzsi);
	(rBuf = p_result);
	return p_result;
}

/*static*/const char * PPObjBizScore::GetBzsiDescr(int bzsi, SString & rBuf)
{
	rBuf.Z();
	//const char * p_result = SIntToSymbTab_GetSymbPtr(BzsIDescrList, SIZEOFARRAY(BzsIDescrList), bzsi);
	const char * p_result = 0;
	for(uint i = 0; !p_result && i < SIZEOFARRAY(BzsIList); i++) {
		const BzsIEntry & r_entry = BzsIList[i];
		if(r_entry.ID == bzsi)
			p_result = r_entry.P_DescrSymb;
	}
	if(p_result) {
#ifdef DL200C
		rBuf.Z(); // @todo
#else
		PPLoadString(p_result, rBuf);
#endif
	}
	return rBuf;
}

/*static*/int PPObjBizScore::RecognizeBzsiSymb(const char * pSymb)
{
	int    result = 0;
	for(uint i = 0; !result && i < SIZEOFARRAY(BzsIList); i++) {
		const BzsIEntry & r_entry = BzsIList[i];
		if(sstreqi_ascii(pSymb, r_entry.P_Symb))
			result = r_entry.ID;
	}
	return result;
	//return SIntToSymbTab_GetId(BzsISymList, SIZEOFARRAY(BzsISymList), pSymb);
}
