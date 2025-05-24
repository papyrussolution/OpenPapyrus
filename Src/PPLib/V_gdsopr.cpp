// V_GDSOPR.CPP
// Copyright (c) A.Sobolev 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <crtdbg.h>
//
//
//
#define GOA_MAX_CACHE_ITEMS 32*1024 // Максимальное количество элементов в кэше PPViewGoodsOpAnalyze::P_Cache
#define GOA_CACHE_DELTA           0 // Количество элементов, сбрасываемое из кэша в базу данных при переполнении кэша

struct GoaAddingBlock {
	GoaAddingBlock() 
	{ 
		THISZERO(); 
	}
	enum {
		fProfitable          = 0x0001, // CheckOpFlags(pPack->Rec.OpID, OPKF_PROFITABLE)
		fIncomeWithoutExcise = 0x0002, // (profitable && Filt.Flags & GoodsOpAnalyzeFilt::fPriceWithoutExcise)
		fTradePlan           = 0x0004  // Блок используется для вставки торгового плана
	};
	PPID   ArID;         // (Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval) ? pPack->Rec.Object : 0L;
	PPID   OpID;         //
	PPID   LocID;        // EachLoc ? pPack->Rec.LocID : Filt.LocList.GetSingle();
	int    Sign;         //
	int    BillSign;     // bill_sign = (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogSelected && sign) ? sign : 1; // AHTOXA
	long   Flags;        // GoaAddingBlock::fXXX
	double Part;
	Goods2Tbl::Rec GoodsRec;
	PPID   ParentGrpID;
	PPID   FinalGoodsID; // ИД товара после подстановки
	int    TiSign;       // _sign = (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogSelected) ? 0 : sign;
	double UnitPerPack;
	double Qtty;
	double PhQtty;       // При установленом флаге GoodsOpAnlyzeFilt::fCompareWithReceipt используется для хранения количества товара документа закупки
	double Cost;
	double Price;
	double OldCost;      // При установленом флаге GoodsOpAnlyzeFilt::fCompareWithReceipt используется для хранения сумм цен поступления товара документа закупки
	double OldPrice;     // При установленом флаге GoodsOpAnlyzeFilt::fCompareWithReceipt используется для хранения сумм цен реализации  товара документа закупки
	double Rest;         //
	double PhRest;       //
	double RestCostSum;  //
	double RestPriceSum; //
};

struct GoaCacheItem {      // size=156
	// Базовая комбинация идентификации структуры {Sign; GoodsID; LocID; ArID}
	// В некоторых ситуациях возможны дополнительные поля для дифференциации: {..Price} || {..Price; Cost}
	GoaCacheItem() 
	{ 
		THISZERO(); 
	}
	int16  Sign;
	int16  Reserve;        // @alignment
	long   Counter;
	PPID   GoodsID;
	PPID   LocID;
	PPID   ArID;
	double UnitPerPack;
	double Qtty;
	double PhQtty;
	double Cost;
	double Price;
	double SumCost;
	double SumPrice;
	double OldCost;
	double OldPrice;
	double Income;         // 6*4+10*8
	double Rest;           //
	double PhRest;         //
	double RestCostSum;    //
	double RestPriceSum;   //
	DBRowId DbPos;
};

struct GoaUniqItem {       // size=36
	long   Id;
	int16  Sign;
	int16  Reserve;        // @alignment
	PPID   GoodsID;
	PPID   LocID;
	PPID   ArID;
	double Cost;
	double Price;
};

IMPL_CMPFUNC(GoaCacheItem, i1, i2)
	{ RET_CMPCASCADE4(static_cast<const GoaCacheItem *>(i1), static_cast<const GoaCacheItem *>(i2), Sign, GoodsID, ArID, LocID); }

IMPL_CMPFUNC(GoaCacheItem_P, i1, i2)
{
	const GoaCacheItem * p1 = static_cast<const GoaCacheItem *>(i1);
	const GoaCacheItem * p2 = static_cast<const GoaCacheItem *>(i2);
	int    si;
	CMPCASCADE4(si, p1, p2, Sign, GoodsID, ArID, LocID);
	if(si == 0) {
		const double p = R6(p1->Price-p2->Price);
		if(p < 0)
			si = -1;
		else if(p > 0)
			si = 1;
	}
	return si;
}

IMPL_CMPFUNC(GoaCacheItem_CP, i1, i2)
{
	const GoaCacheItem * p1 = static_cast<const GoaCacheItem *>(i1);
	const GoaCacheItem * p2 = static_cast<const GoaCacheItem *>(i2);
	int    si;
	CMPCASCADE4(si, p1, p2, Sign, GoodsID, ArID, LocID);
	if(si == 0) {
		double p = R6(p1->Price-p2->Price);
		if(p == 0.0) {
			p = R6(p1->Cost-p2->Cost);
			if(p < 0.0)
				si = -1;
			else if(p > 0.0)
				si = 1;
		}
		else if(p < 0.0)
			si = -1;
		else
			si = 1;
	}
	return si;
}

IMPL_CMPFUNC(GoaUniqItem, i1, i2)
	{ RET_CMPCASCADE4(static_cast<const GoaUniqItem *>(i1), static_cast<const GoaUniqItem *>(i2), Sign, GoodsID, ArID, LocID); }

IMPL_CMPFUNC(GoaUniqItem_P, i1, i2)
{
	const GoaUniqItem * p1 = static_cast<const GoaUniqItem *>(i1);
	const GoaUniqItem * p2 = static_cast<const GoaUniqItem *>(i2);
	int    si;
	CMPCASCADE4(si, p1, p2, Sign, GoodsID, ArID, LocID);
	if(si == 0) {
		double p = R6(p1->Price-p2->Price);
		if(p < 0)
			si = -1;
		else if(p > 0)
			si = 1;
	}
	return si;
}

IMPL_CMPFUNC(GoaUniqItem_CP, i1, i2)
{
	const GoaUniqItem * p1 = static_cast<const GoaUniqItem *>(i1);
	const GoaUniqItem * p2 = static_cast<const GoaUniqItem *>(i2);
	int    si;
	CMPCASCADE4(si, p1, p2, Sign, GoodsID, ArID, LocID);
	if(si == 0) {
		double p = R6(p1->Price-p2->Price);
		if(p == 0) {
			p = R6(p1->Cost-p2->Cost);
			if(p < 0)
				si = -1;
			else if(p > 0)
				si = 1;
		}
		else if(p < 0)
			si = -1;
		else
			si = 1;
	}
	return si;
}
//
//
//
GoodsOpAnalyzeTotal::GoodsOpAnalyzeTotal()
{
	Init();
}

void GoodsOpAnalyzeTotal::Init()
{
	Count = 0;
	InCount = 0;
	PhQtty = Qtty = Cost = Price = Income = RestCost = RestPrice = 0.0;
	InPhQtty = InQtty = InCost = InPrice = InIncome = 0.0;
	PlanQtty = PlanSum = 0.0;
	Rest = 0.0;
	PhRest = 0.0;
	Amounts.clear(); // @v12.1.11 freeAll()-->clear()
	IndicatorVect.clear(); // @v12.1.11
}

GoodsOpAnalyzeTotal & FASTCALL GoodsOpAnalyzeTotal::operator = (const GoodsOpAnalyzeTotal & src)
{
	Count  = src.Count;
	Qtty   = src.Qtty;
	PhQtty = src.PhQtty;
	Cost   = src.Cost;
	Price  = src.Price;
	Income = src.Income;
	Rest = src.Rest;
	PhRest = src.PhRest;
	RestCost  = src.RestCost;
	RestPrice = src.RestPrice;
	InCount  = src.InCount;
	InQtty   = src.InQtty;
	InPhQtty = src.InPhQtty;
	InCost   = src.InCost;
	InPrice  = src.InPrice;
	InIncome = src.InIncome;
	Amounts.copy(src.Amounts);
	IndicatorVect = src.IndicatorVect; // @v12.1.11
	return *this;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(GoodsOpAnalyze); GoodsOpAnalyzeFilt::GoodsOpAnalyzeFilt() : PPBaseFilt(PPFILT_GOODSOPANALYZE, 0, 2)
{
	SetFlatChunk(offsetof(GoodsOpAnalyzeFilt, ReserveStart),
		offsetof(GoodsOpAnalyzeFilt, GoodsIdList) - offsetof(GoodsOpAnalyzeFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(GoodsOpAnalyzeFilt, GoodsIdList));
	SetBranchObjIdListFilt(offsetof(GoodsOpAnalyzeFilt, BillList));
	SetBranchObjIdListFilt(offsetof(GoodsOpAnalyzeFilt, LocList));
	SetBranchSVector(offsetof(GoodsOpAnalyzeFilt, CompareItems));
	Init(1, 0);
}

/*virtual*/int GoodsOpAnalyzeFilt::Describe(long flags, SString & rBuf) const
{
	PutObjMembToBuf(PPOBJ_QUOTKIND,   QuotKindID,   STRINGIZE(QuotKindID), rBuf);
	PutObjMembToBuf(PPOBJ_OPRKIND,    OpID,         STRINGIZE(OpID), rBuf);
	PutObjMembToBuf(PPOBJ_ACCSHEET,   AccSheetID,   STRINGIZE(AccSheetID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    ObjectID,     STRINGIZE(ObjectID), rBuf);
	PutObjMembToBuf(PPOBJ_WORLD,      ObjCityID,    STRINGIZE(ObjCityID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    SupplID,      STRINGIZE(SupplID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    AgentID,      STRINGIZE(AgentID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    SupplAgentID, STRINGIZE(SupplAgentID), rBuf);
	PutObjMembToBuf(PPOBJ_GOODSGROUP, GoodsGrpID,   STRINGIZE(GoodsGrpID), rBuf);
	{
		SString buf;
#define __OPGRP(f) if(OpGrpID == f) buf = #f;
		__OPGRP(ogSelected);
		__OPGRP(ogIncoming);
		__OPGRP(ogProfitable);
		__OPGRP(ogPayed);
		__OPGRP(ogInOutAnalyze);
#undef __OPGRP
		PutMembToBuf(buf, STRINGIZE(OpGrpID), rBuf);
	}
	PutSggMembToBuf(Sgg, STRINGIZE(Sgg), rBuf);

	PutMembToBuf((long)ABCAnlzGroup, STRINGIZE(ABCAnlzGroup),    rBuf);
	PutMembToBuf(RestCalcDate,       STRINGIZE(RestCalcDate),    rBuf);
	PutMembToBuf(CmpRestCalcDate,    STRINGIZE(CmpRestCalcDate), rBuf);
	PutMembToBuf(&CmpPeriod,         STRINGIZE(CmpPeriod),       rBuf);
	PutMembToBuf(&Period,            STRINGIZE(Period),          rBuf);

	PutObjMembListToBuf(PPOBJ_GOODS,    &GoodsIdList, STRINGIZE(GoodsIdList), rBuf);
	PutObjMembListToBuf(PPOBJ_BILL,     &BillList,    STRINGIZE(BIllList),    rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION, &GoodsIdList, STRINGIZE(LocList),     rBuf);
	{
		long   id = 1;
		StrAssocArray flag_list;
#define __FLG(f) if(Flags & f) flag_list.Add(id++, #f)
		__FLG(fLabelOnly);
		__FLG(fDiffByPrice);
		__FLG(fDiffByNetPrice);
		__FLG(fIntrReval);
		__FLG(fPriceWithoutExcise);
		__FLG(fUseABCAnlz);
		__FLG(fCalcRest);
		__FLG(fPriceDeviation);
		__FLG(fDisplayWoPacks);
		__FLG(fEachLocation);
		__FLG(fCalcOrder);
		__FLG(fShowSStatSales);
		__FLG(fCompareWithReceipt);
		__FLG(fUnprofitableGoods);
		__FLG(fBadSellingGoods);
		__FLG(fComparePctDiff);
		__FLG(fBillListAsTradePlan);
		__FLG(fTradePlanObjAsSuppl);
		__FLG(fTradePlanGoodsOnly);
		__FLG(fCrosstab);
#undef __FLG
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	return 1;
}

void FASTCALL GoodsOpAnalyzeFilt::AddTradePlanBillID(PPID billID)
{
	if(Flags & fBillListAsTradePlan)
		BillList.Add(billID);
	else {
		BillList.Set(0);
		BillList.Add(billID);
		Flags |= fBillListAsTradePlan;
	}
}

SString & FASTCALL GoodsOpAnalyzeFilt::GetOpName(SString & rName) const
{
	uint   str_id = 0;
	SString temp_buf;
	SString buf;
	uint   beg_str_id = PPTXT_GOODSOPRABC;
	switch(OpGrpID) {
		case GoodsOpAnalyzeFilt::ogSelected:
			if(Flags & GoodsOpAnalyzeFilt::fIntrReval) {
				beg_str_id = PPTXT_INTRREVABC;
				str_id = PPTXT_BYINTROPR;
			}
			else
				str_id = PPTXT_BYOPR;
			break;
		case GoodsOpAnalyzeFilt::ogIncoming:   str_id = PPTXT_BYINCOMINGOPR;   break;
		case GoodsOpAnalyzeFilt::ogProfitable: str_id = PPTXT_BYPROFITABLEOPR; break;
		case GoodsOpAnalyzeFilt::ogPayed:      str_id = PPTXT_BYPAYEDOPR;      break;
	}
	PPLoadText(str_id, temp_buf);
	if(ABCAnlzGroup > 0) {
		PPLoadText(beg_str_id, buf);
		buf.Space().CatChar('{').Cat(temp_buf);
		temp_buf = buf;
	}
	if(oneof2(OpGrpID, GoodsOpAnalyzeFilt::ogSelected, GoodsOpAnalyzeFilt::ogInOutAnalyze)) {
		::GetOpName(OpID, buf);
		temp_buf.Space().CatChar('\'').Cat(buf).CatChar('\'');
	}
	if(ABCAnlzGroup > 0)
		temp_buf.CatChar('}');
	return (rName = temp_buf);
}

bool FASTCALL GoodsOpAnalyzeFilt::IsValidABCGroup(short abcGroup) const
{
	return (!(Flags & fUseABCAnlz) || ((ABCAnlzGroup == 2 && abcGroup < 0) || (ABCAnlzGroup == 1 && abcGroup > 0) || (ABCAnlzGroup < 0 && ABCAnlzGroup == abcGroup)));
}

bool GoodsOpAnalyzeFilt::IsLeadedInOutAnalyze() const { return (OpGrpID == ogInOutAnalyze && Flags & fLeaderInOutGoods && GoodsIdList.GetCount()); }

void GoodsOpAnalyzeFilt::ZeroCompareItems()
{
	CmpPeriod.Z();
	CompareItems.freeAll();
	Flags &= ~GoodsOpAnalyzeFilt::fComparePctDiff;
	CmpRestCalcDate = ZERODATE;
}
//
//
//
PPViewGoodsOpAnalyze::PPViewGoodsOpAnalyze() : PPView(0, &Filt, PPVIEW_GOODSOPANALYZE, 0, 0), P_BObj(BillObj), P_Psc(0), State(0),
	P_TempTbl(0), P_TempOrd(0), IterIdx(0), P_GGIter(0), P_TradePlanPacket(0), P_TrfrFilt(0), P_Cache(0), P_CmpView(0), P_Uniq(0),
	P_GoodsList(0), CurrentViewOrder(OrdByDefault)
{
	if(P_BObj->CheckRights(BILLRT_ACCSCOST))
		State |= sAccsCost;
}

PPViewGoodsOpAnalyze::~PPViewGoodsOpAnalyze()
{
	delete P_TempTbl;
	delete P_TempOrd;
	delete P_GGIter;
	delete P_Psc;
	delete P_TrfrFilt;
	delete P_TradePlanPacket;
	delete P_Cache;
	delete P_CmpView;
	delete P_Uniq;
	delete P_GoodsList;
}

int PPViewGoodsOpAnalyze::InitUniq(const SArray * pUniq)
{
	int    ok = 1;
	ZDELETE(P_Uniq);
	THROW_MEM(P_Uniq = new SArray(sizeof(GoaUniqItem)));
	if(pUniq)
		P_Uniq->copy(*pUniq);
	CATCHZOK
	return ok;
}

void PPViewGoodsOpAnalyze::CopyUniq(SArray * pUniq) const
{
	if(P_Uniq && pUniq)
		pUniq->copy(*P_Uniq);
}

int PPViewGoodsOpAnalyze::GetByID(PPID id, TempGoodsOprTbl::Rec * pRec)
{
	int    ok = -1;
	if(P_TempTbl && P_TempTbl->search(0, &id, spEq) > 0) {
		ASSIGN_PTR(pRec, P_TempTbl->data);
		ok = 1;
	}
	return ok;
}
//
//
//
void ABCAnlzFilt::SortGrpFract()
{
	for(size_t i = 0; i < ABC_GRPSCOUNT - 1; i++) {
		for(size_t j = i + 1; j < ABC_GRPSCOUNT; j++) {
			if(GrpFract[i] < GrpFract[j]) {
				GrpFract[i] += GrpFract[j];
				GrpFract[j]  = GrpFract[i] - GrpFract[j];
				GrpFract[i] -= GrpFract[j];
			}
		}
	}
}

int ABCAnlzFilt::CheckFracts(double * pFractsSum)
{
	int    ok = 1;
	size_t i;
	double fract_sum = 0.0;
	for(i = 0; i < ABC_GRPSCOUNT; i++)
		fract_sum += GrpFract[i];
	if(fract_sum == 0) {
		fract_sum += (GrpFract[0] = 70.0);
		fract_sum += (GrpFract[1] = 20.0);
		fract_sum += (GrpFract[2] = 10.0);
	}
	THROW_PP(fract_sum <= 100.0, PPERR_INVPRCTSUM);
	if(fract_sum < 100.0) {
		for(i = 0; i < ABC_GRPSCOUNT && GrpFract[i] != 0; i++);
		THROW_PP(i != ABC_GRPSCOUNT, PPERR_INVPRCTSUM); // все  элементы != 0 и fract_sum < 100.0
		GrpFract[i] += (100.0 - fract_sum);
		fract_sum = 100.0;
	}
	CATCHZOK
	ASSIGN_PTR(pFractsSum, fract_sum);
	return ok;
}

int ABCAnlzFilt::GetGroupName(short abcGroup, char * pBuf, size_t bufLen)
{
	int    ok = -1;
	if(pBuf) {
		SString buf;
		buf.CatChar(65 + abcGroup).Space().CatChar('(').Cat(GrpFract[abcGroup], SFMT_MONEY).CatChar('%').CatChar(')');
		buf.CopyTo(pBuf, bufLen);
		ok = 1;
	}
	return ok;
}
//
//
//
class GoodsOpAnlzFiltDialog : public WLDialog {
	DECL_DIALOG_DATA(GoodsOpAnalyzeFilt);
	enum {
		ctlgroupGoodsFilt = 1,
		ctlgroupLoc       = 2,
	};
public:
	GoodsOpAnlzFiltDialog() : WLDialog(DLG_GOODSOPR, CTL_BILLFLT_LABEL)
	{
		addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(0, CTLSEL_BILLFLT_GGRP, cmGoodsFilt));
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_BILLFLT_LOC, 0, 0, cmLocList, 0, LocationCtrlGroup::fEnableSelUpLevel, 0)); // @v12.1.5 LocationCtrlGroup::fEnableSelUpLevel
		SetupCalPeriod(CTLCAL_BILLFLT_PERIOD, CTL_BILLFLT_PERIOD);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		PPID   acc_sheet_id = 0;
		PPID   parent_grp_id = 0;
		PPObjGoods goods_obj;
		SetPeriodInput(this, CTL_BILLFLT_PERIOD, &Data.Period);
		SetupOpCombo();
		GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
		SetupAccSheet(Data.AccSheetID ? Data.AccSheetID : acc_sheet_id);
		goods_obj.GetParentID(Data.GoodsGrpID, &parent_grp_id);
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, 0, 0, GoodsCtrlGroup::enableSelUpLevel, reinterpret_cast<void *>(parent_grp_id));
		setGroupData(ctlgroupGoodsFilt, &gf_rec);
		LocationCtrlGroup::Rec loc_rec(&Data.LocList);
		setGroupData(ctlgroupLoc, &loc_rec);
		SetupPPObjCombo(this, CTLSEL_BILLFLT_MPGUA, PPOBJ_GLOBALUSERACC, Data.MarketplaceGuaID, 0, /*reinterpret_cast<void *>(PPGLS_WILDBERRIES)*/0); // @v12.1.12
		setWL(BIN(Data.Flags & GoodsOpAnalyzeFilt::fLabelOnly));
		//
		// В диалоге флаги расчета сумм НДС в цп и цр устанавливаются (снимаются) одновременно
		//
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fCalcCVat, BIN(Data.Flags & (GoodsOpAnalyzeFilt::fCalcCVat|GoodsOpAnalyzeFilt::fCalcPVat)));
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fCalcPVat, BIN(Data.Flags & (GoodsOpAnalyzeFilt::fCalcCVat|GoodsOpAnalyzeFilt::fCalcPVat)));
		//
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  0, GoodsOpAnalyzeFilt::fDiffByPrice);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  1, GoodsOpAnalyzeFilt::fDiffByNetPrice);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  2, GoodsOpAnalyzeFilt::fPriceWithoutExcise);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  3, GoodsOpAnalyzeFilt::fCalcRest);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  4, GoodsOpAnalyzeFilt::fCalcOrder);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  5, GoodsOpAnalyzeFilt::fShowSStatSales);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  6, GoodsOpAnalyzeFilt::fUnprofitableGoods);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  7, GoodsOpAnalyzeFilt::fBadSellingGoods);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  8, GoodsOpAnalyzeFilt::fTradePlanGoodsOnly);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE,  9, GoodsOpAnalyzeFilt::fAddNzRestItems);
		AddClusterAssoc(CTL_BILLFLT_DIFFBYPRICE, 10, GoodsOpAnalyzeFilt::fCalcCVat);
		SetClusterData(CTL_BILLFLT_DIFFBYPRICE, Data.Flags);
		SetupFlags();
		// @v12.1.7 setCtrlData(CTL_BILLFLT_OPRSET, &(v = (ushort)Data.OpGrpID));
		// @v12.1.7 {
		AddClusterAssocDef(CTL_BILLFLT_OPRSET, 0, GoodsOpAnalyzeFilt::ogSelected);
		AddClusterAssoc(CTL_BILLFLT_OPRSET, 1, GoodsOpAnalyzeFilt::ogIncoming);
		AddClusterAssoc(CTL_BILLFLT_OPRSET, 2, GoodsOpAnalyzeFilt::ogProfitable);
		AddClusterAssoc(CTL_BILLFLT_OPRSET, 3, GoodsOpAnalyzeFilt::ogPayed);
		AddClusterAssoc(CTL_BILLFLT_OPRSET, 4, GoodsOpAnalyzeFilt::ogInOutAnalyze);
		AddClusterAssoc(CTL_BILLFLT_OPRSET, 5, GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze);
		SetClusterData(CTL_BILLFLT_OPRSET, Data.OpGrpID);
		// } @v12.1.7
		disableCtrls(!oneof2(Data.OpGrpID, GoodsOpAnalyzeFilt::ogSelected, GoodsOpAnalyzeFilt::ogInOutAnalyze), /*CTL_BILLFLT_PERIOD,*/ CTL_BILLFLT_OPRKIND, CTL_BILLFLT_AMOUNT, 0);
		AddClusterAssoc(CTL_BILLFLT_INTRREVAL, 0, GoodsOpAnalyzeFilt::fIntrReval);
		SetClusterData(CTL_BILLFLT_INTRREVAL, Data.Flags);
		AddClusterAssoc(CTL_BILLFLT_FLAGS, 0, GoodsOpAnalyzeFilt::fDisplayWoPacks);
		AddClusterAssoc(CTL_BILLFLT_FLAGS, 1, GoodsOpAnalyzeFilt::fCompareWithReceipt);
		SetClusterData(CTL_BILLFLT_FLAGS, Data.Flags);
		AddClusterAssoc(CTL_BILLFLT_EACHLOC, 0, GoodsOpAnalyzeFilt::fEachLocation);
		AddClusterAssoc(CTL_BILLFLT_EACHLOC, 1, GoodsOpAnalyzeFilt::fCrosstab);
		SetClusterData(CTL_BILLFLT_EACHLOC, Data.Flags);
		AddClusterAssoc(CTL_BILLFLT_USEABCANLZ, 0, GoodsOpAnalyzeFilt::fUseABCAnlz);
		AddClusterAssoc(CTL_BILLFLT_USEABCANLZ, 1, GoodsOpAnalyzeFilt::fABCAnlzByGGrps);
		SetupCtrls(Data.Flags);
		disableCtrls(Data.BillList.IsExists(), CTL_BILLFLT_OPRSET,
			CTLSEL_BILLFLT_OBJECT, CTLSEL_BILLFLT_OPRKIND, CTL_BILLFLT_PERIOD, CTLCAL_BILLFLT_PERIOD, 0);
		if(Data.BillList.IsExists()) {
			disableCtrl(CTLSEL_BILLFLT_LOC, 1);
			enableCommand(cmLocList, 0);
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ushort v;
		GoodsFiltCtrlGroup::Rec gf_rec;
		LocationCtrlGroup::Rec  loc_rec;
		THROW(GetPeriodInput(this, CTL_BILLFLT_PERIOD, &Data.Period));
		getCtrlData(CTLSEL_BILLFLT_OPRKIND, &Data.OpID);
		Data.ObjectID = (Data.OpID || Data.AccSheetID) ? getCtrlLong(CTLSEL_BILLFLT_OBJECT) : 0;
		//getCtrlData(CTLSEL_BILLFLT_SUPPL, &Data.SupplID);
		getCtrlData(CTLSEL_BILLFLT_GGRP,  &Data.GoodsGrpID);
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fLabelOnly, getWL());
		GetClusterData(CTL_BILLFLT_DIFFBYPRICE, &Data.Flags);
		//
		// В диалоге флаги расчета сумм НДС в цп и цр устанавливаются (снимаются) одновременно
		//
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fCalcCVat, BIN(Data.Flags & (GoodsOpAnalyzeFilt::fCalcCVat|GoodsOpAnalyzeFilt::fCalcPVat)));
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fCalcPVat, BIN(Data.Flags & (GoodsOpAnalyzeFilt::fCalcCVat|GoodsOpAnalyzeFilt::fCalcPVat)));
		GetClusterData(CTL_BILLFLT_INTRREVAL, &Data.Flags);
		// @v12.1.7 getCtrlData(CTL_BILLFLT_OPRSET, &(v = 0));
		// @v12.1.7 Data.OpGrpID = v;
		GetClusterData(CTL_BILLFLT_OPRSET, &Data.OpGrpID); // @v12.1.7
		GetClusterData(CTL_BILLFLT_USEABCANLZ, &Data.Flags);
		Data.ABCAnlzGroup = BIN(Data.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz);
		if(Data.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz)
			Data.ABCAnlz.CheckFracts(0);
		else
			MEMSZERO(Data.ABCAnlz);
		v = getCtrlUInt16(CTL_BILLFLT_PRICEDEV);
		SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fPriceDeviation, v & 0x01);
		GetClusterData(CTL_BILLFLT_FLAGS, &Data.Flags);
		GetClusterData(CTL_BILLFLT_EACHLOC,   &Data.Flags);
		THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
		Data.GoodsGrpID = gf_rec.GoodsGrpID;
		THROW(getGroupData(ctlgroupLoc, &loc_rec));
		Data.LocList = loc_rec.LocList;
		getCtrlData(CTLSEL_BILLFLT_MPGUA, &Data.MarketplaceGuaID);
		if(!(Data.Flags & GoodsOpAnalyzeFilt::fCrosstab) && Data.CmpPeriod.IsZero())
			Data.CompareItems.freeAll();
		if(!AdjustPeriodToRights(Data.Period, 1)) {
			selectCtrl(CTL_BILLFLT_PERIOD);
			CALLEXCEPT();
		}
		else {
			ASSIGN_PTR(pData, Data);
		}
		CATCHZOKPPERR
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	void   ReplyOprGrpChanged();
	void   SetupOpCombo();
	void   SetupAccSheet(PPID accSheetID);
	int    EditCompareItems();
	int    EditABCAnlzFilt(ABCAnlzFilt * pFilt);
	void   SetupCtrls(long prevFlags);
	void   SetupFlags();
};

void GoodsOpAnlzFiltDialog::SetupCtrls(long prevFlags)
{
	const  PPID op_type_id = GetOpType(Data.OpID);
	const  bool enbl = (Data.OpGrpID != GoodsOpAnalyzeFilt::ogInOutAnalyze && !(Data.Flags & GoodsOpAnalyzeFilt::fIntrReval) && op_type_id != PPOPT_GOODSREVAL);
	const  bool cmp_f = LOGIC(Data.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt);
	bool   disable_abc = false;
	ushort v = 0;
	showCtrl(CTLSEL_BILLFLT_MPGUA, Data.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze); // @v12.1.12
	DisableClusterItem(CTL_BILLFLT_EACHLOC, 1, BIN(!(Data.Flags & GoodsOpAnalyzeFilt::fEachLocation)));
	if(!(Data.Flags & GoodsOpAnalyzeFilt::fEachLocation)) {
		Data.Flags &= ~GoodsOpAnalyzeFilt::fCrosstab;
		SetClusterData(CTL_BILLFLT_EACHLOC, Data.Flags);
	}
	const  bool is_crosstab = LOGIC(Data.Flags & GoodsOpAnalyzeFilt::fCrosstab);
	SETFLAG(v, 0x01, (Data.Flags & GoodsOpAnalyzeFilt::fPriceDeviation) && enbl && (!cmp_f));
	SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fPriceDeviation, v & 0x01);
	Data.QuotKindID = (enbl && (!(Data.Flags & GoodsOpAnalyzeFilt::fPriceDeviation))) ? Data.QuotKindID : 0;
	disable_abc = (Data.QuotKindID > 0 || (Data.Flags & GoodsOpAnalyzeFilt::fPriceDeviation) || !enbl || cmp_f || is_crosstab);
	disableCtrl(CTL_BILLFLT_PRICEDEV, !enbl || cmp_f);
	setCtrlData(CTL_BILLFLT_PRICEDEV, &v);
	v = 0;
	disableCtrl(CTL_BILLFLT_USEABCANLZ, disable_abc);
	if(disable_abc)
		Data.Flags &= ~(GoodsOpAnalyzeFilt::fUseABCAnlz|GoodsOpAnalyzeFilt::fABCAnlzByGGrps);
	else {
		if(!(prevFlags & GoodsOpAnalyzeFilt::fUseABCAnlz) && (Data.Flags & GoodsOpAnalyzeFilt::fABCAnlzByGGrps))
			Data.Flags |= GoodsOpAnalyzeFilt::fUseABCAnlz;
		else if((prevFlags & GoodsOpAnalyzeFilt::fUseABCAnlz) && !(Data.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz))
			Data.Flags &= ~GoodsOpAnalyzeFilt::fABCAnlzByGGrps;
	}
	SetClusterData(CTL_BILLFLT_USEABCANLZ, Data.Flags);
	enableCommand(cmABCAnlzOptions, BIN(Data.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz));
	Data.ABCAnlzGroup = BIN(Data.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz);
	enableCommand(cmCompare, !Data.ABCAnlzGroup);
	if(Data.ABCAnlzGroup)
		Data.ZeroCompareItems();
	DisableClusterItem(CTL_BILLFLT_FLAGS, 1, (op_type_id != PPOPT_DRAFTRECEIPT));
	SetClusterData(CTL_BILLFLT_FLAGS, Data.Flags);
}

void GoodsOpAnlzFiltDialog::SetupFlags()
{
	if(!!Data.Sgb) {
		Data.Flags &= ~GoodsOpAnalyzeFilt::fCalcRest;
		Data.Flags |= GoodsOpAnalyzeFilt::fDisplayWoPacks;
	}
	long   flags = Data.Flags;
	if(!(flags & GoodsOpAnalyzeFilt::fCalcRest))
		flags &= ~(GoodsOpAnalyzeFilt::fCalcOrder|GoodsOpAnalyzeFilt::fShowSStatSales|
			GoodsOpAnalyzeFilt::fBadSellingGoods|GoodsOpAnalyzeFilt::fAddNzRestItems);
	if(flags & GoodsOpAnalyzeFilt::fBadSellingGoods) {
		flags &= ~(GoodsOpAnalyzeFilt::fUnprofitableGoods|GoodsOpAnalyzeFilt::fAddNzRestItems);
		Data.Sgg = sggNone;
	}
	if(Data.Sgg)
		Data.Flags |= GoodsOpAnalyzeFilt::fDisplayWoPacks;
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 3, !!Data.Sgb);
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 4, !(flags & GoodsOpAnalyzeFilt::fCalcRest));
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 5, !(flags & GoodsOpAnalyzeFilt::fCalcRest));
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 7, !(flags & GoodsOpAnalyzeFilt::fCalcRest));
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 9, !(flags & GoodsOpAnalyzeFilt::fCalcRest) || (flags & GoodsOpAnalyzeFilt::fBadSellingGoods));
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 6, flags & GoodsOpAnalyzeFilt::fBadSellingGoods);
	DisableClusterItem(CTL_BILLFLT_DIFFBYPRICE, 8, BIN(!(Data.Flags & GoodsOpAnalyzeFilt::fBillListAsTradePlan)));
	SetClusterData(CTL_BILLFLT_DIFFBYPRICE, flags);

	DisableClusterItem(CTL_BILLFLT_FLAGS, 0, !!Data.Sgb || Data.Sgg);
	SetClusterData(CTL_BILLFLT_FLAGS, flags);
}

void GoodsOpAnlzFiltDialog::SetupOpCombo()
{
	PPIDArray op_list;
	if(Data.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze) {
		op_list.addzlist(PPOPT_GOODSMODIF, PPOPT_GENERIC, 0L);
	}
	else /*if(Data.OpGrpID == GoodsOpAnalyzeFilt::ogSelected)*/ {
		op_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL,
			PPOPT_GOODSMODIF, PPOPT_GOODSORDER, PPOPT_PAYMENT, PPOPT_CHARGE, PPOPT_GOODSACK,
			PPOPT_GENERIC, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, PPOPT_DRAFTQUOTREQ, 0L);
	}
	SetupOprKindCombo(this, CTLSEL_BILLFLT_OPRKIND, Data.OpID, 0, &op_list, 0);
}

class GoodsOpAnlzCmpFiltDialog : public PPListDialog {
	DECL_DIALOG_DATA(GoodsOpAnalyzeFilt);
public:
	GoodsOpAnlzCmpFiltDialog() : PPListDialog(DLG_GOODSOPRE, CTL_GOODSOPRE_VALLIST), PrevID(0)
	{
		PPLoadText(PPTXT_GOODSOPRADDVALNAMES, Items);
		SetupCalPeriod(CTLCAL_GOODSOPRE_PERIOD, CTL_GOODSOPRE_PERIOD);
		setSmartListBoxOption(CTL_GOODSOPRE_VALLIST, lbtSelNotify);
		PPSetupCtrlMenu(this, CTL_GOODSOPRE_PERIOD, CTLMNU_GOODSOPRE_PERIOD, CTRLMENU_GOODSOPANLZADDPERIOD);
		SetupCalDate(CTLCAL_GOODSOPRE_RESTDT, CTL_GOODSOPRE_RESTDT);
		PPSetupCtrlMenu(this, CTL_GOODSOPRE_RESTDT, CTLMNU_GOODSOPRE_RESTDT, CTRLMENU_RESTDATE);
	}
	DECL_DIALOG_SETDTS()
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		const bool is_crosstab = LOGIC(Data.Flags & GoodsOpAnalyzeFilt::fCrosstab);
		disableCtrls(is_crosstab, CTL_GOODSOPRE_PERIOD, CTL_GOODSOPRE_RESTDT, CTL_GOODSOPRE_PCTDIFF, 0L);
		DisableClusterItem(CTL_GOODSOPRE_FLAGS, 1, is_crosstab);
		DisableClusterItem(CTL_GOODSOPRE_FLAGS, 2, is_crosstab);
		if(is_crosstab)
			Data.Flags &= ~GoodsOpAnalyzeFilt::fComparePctDiff;
		else if(Data.CmpPeriod.IsZero())
			Data.Flags |= GoodsOpAnalyzeFilt::fComparePctDiff;
		SetPeriodInput(this, CTL_GOODSOPRE_PERIOD, &Data.CmpPeriod);
		AddClusterAssoc(CTL_GOODSOPRE_FLAGS, 0, GoodsOpAnalyzeFilt::ffldMainPeriod);
		AddClusterAssoc(CTL_GOODSOPRE_FLAGS, 1, GoodsOpAnalyzeFilt::ffldCmpPeriod);
		AddClusterAssoc(CTL_GOODSOPRE_FLAGS, 2, GoodsOpAnalyzeFilt::ffldDiff);
		AddClusterAssoc(CTL_GOODSOPRE_PCTDIFF, 0, GoodsOpAnalyzeFilt::fComparePctDiff);
		SetClusterData(CTL_GOODSOPRE_PCTDIFF, Data.Flags);
		disableCtrl(CTL_GOODSOPRE_RESTDT, (Data.Flags & GoodsOpAnalyzeFilt::fCalcRest) ? 0 : 1);
		setCtrlData(CTL_GOODSOPRE_RESTDT, &Data.CmpRestCalcDate);
		updateList(-1);
		SetupFlags();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SetupFlags();
		THROW(GetPeriodInput(this, CTL_GOODSOPRE_PERIOD, &Data.CmpPeriod));
		GetClusterData(CTL_GOODSOPRE_PCTDIFF, &Data.Flags);
		getCtrlData(CTL_GOODSOPRE_RESTDT, &Data.CmpRestCalcDate);
		if(!(Data.Flags & GoodsOpAnalyzeFilt::fCalcRest))
			Data.CmpRestCalcDate = ZERODATE;
		if(Data.CmpPeriod.IsZero())
			Data.Flags &= ~GoodsOpAnalyzeFilt::fComparePctDiff;
		ASSIGN_PTR(pData, Data);
		CATCHZOK
		return ok;
	}
private:
	DECL_HANDLE_EVENT;
	virtual int setupList();
	virtual int editItem(long pos, long id);
	void   SetupFlags();

	uint   PrevID;
	SString Items;
};

IMPL_HANDLE_EVENT(GoodsOpAnlzCmpFiltDialog)
{
	int is_crosstab = BIN(Data.Flags & GoodsOpAnalyzeFilt::fCrosstab);
	PPListDialog::handleEvent(event);
	if(TVCOMMAND) {
		if(TVCMD == cmLBItemSelected) {
			SetupFlags();
			clearEvent(event);
		}
	}
	else if(TVKEYDOWN) {
		if(!is_crosstab) {
			if(oneof3(TVKEY, kbF6, kbF8, kbF10)) {
				if(TVKEY == kbF10) {
					if(Data.Period.low != ZERODATE && Data.Period.upp != ZERODATE) {
						long days = diffdate(Data.Period.upp, Data.Period.low);
						days++;
						plusperiod(&(Data.CmpPeriod.low = Data.Period.low), PRD_DAY, -days, 0);
						plusperiod(&(Data.CmpPeriod.upp = Data.Period.upp), PRD_DAY, -days, 0);
					}
				}
				else {
					int period = (TVKEY == kbF6) ? PRD_MONTH : PRD_ANNUAL;
					if(Data.Period.low != ZERODATE)
						plusperiod(&(Data.CmpPeriod.low = Data.Period.low), period, -1, 0);
					if(Data.Period.upp != ZERODATE)
						plusperiod(&(Data.CmpPeriod.upp = Data.Period.upp), period, -1, 0);
				}
				SetPeriodInput(this, CTL_GOODSOPRE_PERIOD, &Data.CmpPeriod);
				clearEvent(event);
			}
			else if(TVKEY == kbF3 || TVKEY == kbF4 || TVKEY == kbF5) {
				GetPeriodInput(this, CTL_GOODSOPRE_PERIOD, &Data.CmpPeriod);
				if(TVKEY == kbF3)
					setCtrlDate(CTL_GOODSOPRE_RESTDT, ZERODATE);
				else if(TVKEY == kbF4)
					setCtrlDate(CTL_GOODSOPRE_RESTDT, Data.CmpPeriod.low ? plusdate(Data.CmpPeriod.low, -1) : ZERODATE);
				else if(TVKEY == kbF5)
					setCtrlDate(CTL_GOODSOPRE_RESTDT, Data.CmpPeriod.upp);
				clearEvent(event);
			}
		}
	}
}

/*virtual*/int GoodsOpAnlzCmpFiltDialog::setupList()
{
	int    ok = 1;
	SString buf;
	StringSet ss(';', Items);
	for(uint p = 0, id = 1; ss.get(&p, buf); id++)
		if(!addStringToList(id, buf))
			ok = 0;
	return ok;
}

/*virtual*/int GoodsOpAnlzCmpFiltDialog::editItem(long pos, long id)
{
	long   set_flags = (Data.Flags & GoodsOpAnalyzeFilt::fCrosstab) ? GoodsOpAnalyzeFilt::ffldMainPeriod :
		(GoodsOpAnalyzeFilt::ffldMainPeriod|GoodsOpAnalyzeFilt::ffldCmpPeriod|GoodsOpAnalyzeFilt::ffldDiff);
	long   flags = 0;
	GetClusterData(CTL_GOODSOPRE_FLAGS, &flags);
	flags = (flags == 0) ? set_flags : 0;
	SetClusterData(CTL_GOODSOPRE_FLAGS, flags);
	SetupFlags();
	return 1;
}

void GoodsOpAnlzCmpFiltDialog::SetupFlags()
{
	uint   p = 0;
	ObjRestrictItem item(PrevID, 0);
	GetClusterData(CTL_GOODSOPRE_FLAGS, &item.Flags);
	if(item.Flags)
		Data.CompareItems.UpdateItemByID(item.ObjID, item.Flags);
	else
		Data.CompareItems.RemoveItemByID(item.ObjID);
	getSelection(&item.ObjID);
	item.Flags = 0;
	if(Data.CompareItems.SearchItemByID(item.ObjID, &p))
		item = Data.CompareItems.at(p);
	SetClusterData(CTL_GOODSOPRE_FLAGS, item.Flags);
	PrevID = item.ObjID;
}

int GoodsOpAnlzFiltDialog::EditCompareItems()
{
	GetClusterData(CTL_BILLFLT_EACHLOC, &Data.Flags);
	GetPeriodInput(this, CTL_BILLFLT_PERIOD, &Data.Period);
	DIALOG_PROC_BODYERR(GoodsOpAnlzCmpFiltDialog, &Data);
}

class GoodsOpAnlzExtFiltDialog : public TDialog {
	DECL_DIALOG_DATA(GoodsOpAnalyzeFilt);
	const PPID   SupplAcsID;
	const PPID   AgentAcsID;
public:
	GoodsOpAnlzExtFiltDialog() : TDialog(DLG_GOODSOPR2), SupplAcsID(GetSupplAccSheet()), AgentAcsID(GetAgentAccSheet())
	{
		PPSetupCtrlMenu(this, CTL_BILLFLT_RESTDATE, CTLMNU_BILLFLT_RESTDATE, CTRLMENU_RESTDATE);
		SetupCalDate(CTLCAL_BILLFLT_RESTDATE, CTL_BILLFLT_RESTDATE);
	}
	DECL_DIALOG_SETDTS()
	{
		PPID   object2_sheet_id = 0;
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		GetOpCommonAccSheet(Data.OpID, 0, &object2_sheet_id);
		SetupArCombo(this, CTLSEL_BILLFLT_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, SupplAcsID, sacfDisableIfZeroSheet);
		SetupArCombo(this, CTLSEL_BILLFLT_AGENT, Data.AgentID, OLW_LOADDEFONOPEN, AgentAcsID, sacfDisableIfZeroSheet);
		SetupArCombo(this, CTLSEL_BILLFLT_SUPPLAGNT, Data.SupplAgentID, OLW_LOADDEFONOPEN, AgentAcsID, sacfDisableIfZeroSheet);
		SetupArCombo(this, CTLSEL_BILLFLT_OBJ2, (object2_sheet_id) ? Data.Object2 : 0L, OLW_LOADDEFONOPEN, object2_sheet_id, sacfDisableIfZeroSheet);
		SetupPPObjCombo(this,  CTLSEL_BILLFLT_OBJCITY, PPOBJ_WORLD, Data.ObjCityID, OLW_LOADDEFONOPEN,
			PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
		SetupPPObjCombo(this, CTLSEL_BILLFLT_FRGHTAGNT, PPOBJ_PERSON, Data.FreightAgentID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(PPPRK_VESSELSAGENT));
		disableCtrl(CTL_BILLFLT_RESTDATE, (Data.Flags & GoodsOpAnalyzeFilt::fCalcRest) ? 0 : 1);
		setCtrlData(CTL_BILLFLT_RESTDATE, &Data.RestCalcDate);
		int enbl = (Data.OpGrpID != GoodsOpAnalyzeFilt::ogInOutAnalyze &&(!(Data.Flags & GoodsOpAnalyzeFilt::fIntrReval)) &&
			(!(Data.Flags & GoodsOpAnalyzeFilt::fPriceDeviation)) && GetOpType(Data.OpID) != PPOPT_GOODSREVAL);
		SetupPPObjCombo(this, CTLSEL_BILLFLT_QUOTKIND, PPOBJ_QUOTKIND, Data.QuotKindID, OLW_LOADDEFONOPEN);
		disableCtrl(CTLSEL_BILLFLT_QUOTKIND, !enbl);
		disableCtrl(CTLSEL_BILLFLT_OBJ2, !object2_sheet_id);
		SetupSubstBillCombo(this, CTLSEL_BILLFLT_SUBST, Data.Sgb);
		SetupSubst();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		if(SupplAcsID)
			getCtrlData(CTLSEL_BILLFLT_SUPPL, &Data.SupplID);
		if(AgentAcsID) {
			getCtrlData(CTLSEL_BILLFLT_AGENT, &Data.AgentID);
			getCtrlData(CTLSEL_BILLFLT_SUPPLAGNT, &Data.SupplAgentID);
		}
		getCtrlData(CTLSEL_BILLFLT_OBJCITY, &Data.ObjCityID);
		getCtrlData(CTLSEL_BILLFLT_FRGHTAGNT, &Data.FreightAgentID);
		Data.Sgb.S = (SubstGrpBill::_S)getCtrlLong(CTLSEL_BILLFLT_SUBST);
		if(!Data.Sgb) {
			Data.Sgg = (SubstGrpGoods)getCtrlLong(CTLSEL_BILLFLT_SUBSTGDS);
		}
		else {
			if(Data.Sgb.S == SubstGrpBill::sgbDate) {
				Data.Sgb.S2.Sgd = (SubstGrpDate)getCtrlLong(CTLSEL_BILLFLT_SUBSTGDS);
			}
			else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject,  SubstGrpBill::sgbObject2,
				SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer)) {
				Data.Sgb.S2.Sgp = (SubstGrpPerson)getCtrlLong(CTLSEL_BILLFLT_SUBSTGDS);
			}
			else
				Data.Sgb.S2.Sgp = sgpNone;
		}
		getCtrlData(CTL_BILLFLT_RESTDATE, &Data.RestCalcDate);
		getCtrlData(CTL_BILLFLT_QUOTKIND, &Data.QuotKindID); // AHTOXA
		getCtrlData(CTLSEL_BILLFLT_OBJ2, &Data.Object2);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_BILLFLT_SUBST)) {
			Data.Sgb.S = (SubstGrpBill::_S)getCtrlLong(CTLSEL_BILLFLT_SUBST);
			SetupSubst();
		}
		else if(event.isCbSelected(CTLSEL_BILLFLT_SUBSTGDS)) {
			if(!Data.Sgb) {
				Data.Sgg = (SubstGrpGoods)getCtrlLong(CTLSEL_BILLFLT_SUBSTGDS);
				SetupSubst();
			}
		}
		else if(event.isKeyDown(kbF3))
			setCtrlDate(CTL_BILLFLT_RESTDATE, ZERODATE);
		else if(event.isKeyDown(kbF4))
			setCtrlDate(CTL_BILLFLT_RESTDATE, Data.Period.low ? plusdate(Data.Period.low, -1) : ZERODATE);
		else if(event.isKeyDown(kbF5))
			setCtrlDate(CTL_BILLFLT_RESTDATE, Data.Period.upp);
		else
			return;
		clearEvent(event);
	}
	void SetupSubst()
	{
		int    dsbl_bill_subst = 0;
		int    dsbl_sec_subst = 0;
		SString label_buf;
		if(Data.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods) {
			Data.Sgg = sggNone;
			Data.Sgb.Reset();
			dsbl_bill_subst = 1;
			dsbl_sec_subst = 1;
		}
		if(Data.Flags & (GoodsOpAnalyzeFilt::fCalcRest|GoodsOpAnalyzeFilt::fShowSStatSales|GoodsOpAnalyzeFilt::fDiffByPrice|GoodsOpAnalyzeFilt::fDiffByNetPrice)) {
			Data.Sgb.Reset();
			dsbl_bill_subst = 1;
		}
		if(!!Data.Sgb) {
			Data.Sgg = sggNone;
			if(Data.Sgb.S == SubstGrpBill::sgbDate) {
				PPLoadString("date", label_buf);
				SetupSubstDateCombo(this, CTLSEL_BILLFLT_SUBSTGDS, (long)Data.Sgb.S2.Sgd);
				dsbl_sec_subst = 0;
			}
			else if(oneof4(Data.Sgb.S, SubstGrpBill::sgbObject,  SubstGrpBill::sgbObject2,
				SubstGrpBill::sgbAgent, SubstGrpBill::sgbPayer)) {
				PPLoadString("person", label_buf);
				SetupSubstPersonCombo(this, CTLSEL_BILLFLT_SUBSTGDS, Data.Sgb.S2.Sgp);
				dsbl_sec_subst = 0;
			}
			else {
				setLabelText(CTL_BILLFLT_SUBSTGDS, label_buf.Z());
				dsbl_sec_subst = 1;
			}
		}
		else {
			PPLoadString("ware", label_buf);
			SetupSubstGoodsCombo(this, CTLSEL_BILLFLT_SUBSTGDS, Data.Sgg);
			if(Data.Sgg) {
				Data.Sgb.Reset();
				dsbl_bill_subst = 1;
			}
		}
		setLabelText(CTL_BILLFLT_SUBSTGDS, label_buf);
		disableCtrl(CTLSEL_BILLFLT_SUBST, dsbl_bill_subst);
		disableCtrl(CTLSEL_BILLFLT_SUBSTGDS, dsbl_sec_subst);
	}
};

IMPL_HANDLE_EVENT(GoodsOpAnlzFiltDialog)
{
	WLDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_BILLFLT_OPRKIND)) {
		PPID prev_op_id = Data.OpID;
		getCtrlData(CTLSEL_BILLFLT_OPRKIND, &Data.OpID);
		Data.Object2 = (Data.OpID != prev_op_id) ? 0 : Data.Object2;
		if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
			PPID   acc_sheet_id = 0;
			GetOpCommonAccSheet(Data.OpID, &acc_sheet_id, 0);
			SetupAccSheet(acc_sheet_id);
			if(IsIntrExpndOp(Data.OpID))
				disableCtrl(CTL_BILLFLT_INTRREVAL, 0);
			else {
				disableCtrl(CTL_BILLFLT_INTRREVAL, 1);
				setCtrlUInt16(CTL_BILLFLT_INTRREVAL, 0);
			}
		}
		SetupCtrls(Data.Flags);
	}
	else if(event.isCmd(cmClusterClk)) {
		const uint ctl_id = event.getCtlID();
		if(ctl_id == CTL_BILLFLT_OPRSET)
			ReplyOprGrpChanged();
		else if(ctl_id == CTL_BILLFLT_DIFFBYPRICE) {
			GetClusterData(CTL_BILLFLT_DIFFBYPRICE, &Data.Flags);
			SetupFlags();
		}
		else if(oneof4(ctl_id, CTL_BILLFLT_USEABCANLZ, CTL_BILLFLT_INTRREVAL, CTL_BILLFLT_PRICEDEV, CTL_BILLFLT_FLAGS)) {
			ushort v = 0; // getCtrlUInt16(CTL_BILLFLT_USEABCANLZ);
			long prev_flags = Data.Flags;
			GetClusterData(CTL_BILLFLT_USEABCANLZ, &Data.Flags);
			// SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fUseABCAnlz, v & 1);
			v = getCtrlUInt16(CTL_BILLFLT_PRICEDEV);
			SETFLAG(Data.Flags, GoodsOpAnalyzeFilt::fPriceDeviation, v & 1);
			GetClusterData(CTL_BILLFLT_INTRREVAL, &Data.Flags);
			GetClusterData(CTL_BILLFLT_FLAGS, &Data.Flags);
			SetupCtrls(prev_flags);
		}
		else if(ctl_id == CTL_BILLFLT_EACHLOC) {
			GetClusterData(CTL_BILLFLT_EACHLOC, &Data.Flags);
			Data.ZeroCompareItems();
			SetupCtrls(Data.Flags);
		}
		else
			return;
	}
	else if(event.isCmd(cmBillExtra)) {
		int    valid_data = 0;
		const  PPID suppl_acs_id = GetSupplAccSheet();
		const  PPID agent_acs_id = GetAgentAccSheet();
		if(suppl_acs_id || agent_acs_id) {
			GoodsOpAnlzExtFiltDialog * p_dlg = new GoodsOpAnlzExtFiltDialog();
			GetClusterData(CTL_BILLFLT_DIFFBYPRICE, &Data.Flags);
			GetPeriodInput(this, CTL_BILLFLT_PERIOD, &Data.Period);
			p_dlg->setDTS(&Data);
			if(CheckDialogPtrErr(&p_dlg)) {
				while(!valid_data && ExecView(p_dlg) == cmOK) {
					if(p_dlg->getDTS(&Data) > 0) {
						valid_data = 1;
						SetupCtrls(Data.Flags);
						SetupFlags();
					}
					else
						PPError();
				}
				disableCtrl(CTLSEL_BILLFLT_QUOTKIND, 0);
			}
			delete p_dlg;
		}
	}
	else if(event.isCmd(cmCompare))
		EditCompareItems();
	else if(event.isCmd(cmABCAnlzOptions))
		EditABCAnlzFilt(&Data.ABCAnlz);
	else
		return;
	clearEvent(event);
}

void GoodsOpAnlzFiltDialog::ReplyOprGrpChanged()
{
	if(GetClusterData(CTL_BILLFLT_OPRSET, &Data.OpGrpID)) {
		SetupCtrls(Data.Flags);
		SetupOpCombo();
		if(!oneof2(Data.OpGrpID, GoodsOpAnalyzeFilt::ogSelected, GoodsOpAnalyzeFilt::ogInOutAnalyze)) {
			disableCtrl(CTLSEL_BILLFLT_OPRKIND, 1);
			Data.OpID = 0;
			if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
				PPID   last = 0;
				PPOprKind op_rec;
				for(SEnum en = PPRef->Enum(PPOBJ_OPRKIND, 0); en.Next(&op_rec) > 0;) {
					if(op_rec.Flags & OPKF_PROFITABLE && op_rec.AccSheetID)
						last = (last && op_rec.AccSheetID != last) ? -1 : op_rec.AccSheetID;
				}
				SetupAccSheet((last == -1) ? 0 : last);
			}
		}
		else
			disableCtrl(CTLSEL_BILLFLT_OPRKIND, 0);
	}
}

void GoodsOpAnlzFiltDialog::SetupAccSheet(PPID accSheetID)
{
	if(getCtrlView(CTLSEL_BILLFLT_OBJECT)) {
		Data.AccSheetID = accSheetID;
		SetupArCombo(this, CTLSEL_BILLFLT_OBJECT, Data.ObjectID, OLW_LOADDEFONOPEN, accSheetID, sacfDisableIfZeroSheet);
		if(!accSheetID && isCurrCtlID(CTL_BILLFLT_OBJECT))
			selectNext();
	}
}

int GoodsOpAnlzFiltDialog::EditABCAnlzFilt(ABCAnlzFilt * pFilt)
{
	class ABCAnlzFiltDialog : public TDialog {
		DECL_DIALOG_DATA(ABCAnlzFilt);
	public:
		ABCAnlzFiltDialog() : TDialog(DLG_ABCANLZ)
		{
			disableCtrl(CTL_ABCANLZ_FRSUM, 1);
		}
		DECL_DIALOG_SETDTS()
		{
			double fract_sum = 0.0;
			RVALUEPTR(Data, pData);
			AddClusterAssocDef(CTL_ABCANLZ_GROUPBY, 0, ABCAnlzFilt::GroupByCostSum);
			AddClusterAssoc(CTL_ABCANLZ_GROUPBY, 1, ABCAnlzFilt::GroupByPriceSum);
			AddClusterAssoc(CTL_ABCANLZ_GROUPBY, 2, ABCAnlzFilt::GroupByQtty);
			AddClusterAssoc(CTL_ABCANLZ_GROUPBY, 3, ABCAnlzFilt::GroupByIncome);
			SetClusterData(CTL_ABCANLZ_GROUPBY, Data.GroupBy);
			Data.CheckFracts(&fract_sum);
			setCtrlReal(CTL_ABCANLZ_FRSUM, fract_sum);
			for(uint i = 0; i < ABC_GRPSCOUNT; i++)
				setCtrlReal(grpFractCtl(i), Data.GrpFract[i]);
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			GetClusterData(CTL_ABCANLZ_GROUPBY, &Data.GroupBy);
			int    ok = checkGrpsFract(0);
			if(ok) {
				Data.SortGrpFract();
				ASSIGN_PTR(pData, Data);
			}
			else
				PPError();
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVBROADCAST && TVCMD == cmChangedFocus) {
				double fract_sum = 0.0;
				checkGrpsFract(&fract_sum);
				for(uint i = 0; i < ABC_GRPSCOUNT; i++)
					setCtrlReal(grpFractCtl(i), Data.GrpFract[i]);
				setCtrlReal(CTL_ABCANLZ_FRSUM, fract_sum);
				clearEvent(event);
			}
		}
		uint    grpFractCtl(uint i) const
		{
			return (3 + i + WINDOWS_ID_BIAS);
		}
		int     checkGrpsFract(double * pFractsSum)
		{
			for(uint i = 0; i < ABC_GRPSCOUNT; i++)
				Data.GrpFract[i] = fabs(getCtrlReal(grpFractCtl(i)));
			return Data.CheckFracts(pFractsSum);
		}
	};
	DIALOG_PROC_BODY(ABCAnlzFiltDialog, pFilt);
}

int PPViewGoodsOpAnalyze::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(!Filt.IsA(pBaseFilt))
		return 0;
	GoodsOpAnalyzeFilt * p_filt = static_cast<GoodsOpAnalyzeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(GoodsOpAnlzFiltDialog, p_filt);
}

PPViewGoodsOpAnalyze::IterOrder PPViewGoodsOpAnalyze::GetIterOrder() const
{
	IterOrder ord;
	if(Filt.ABCAnlzGroup < 0) {
		switch(Filt.ABCAnlz.GroupBy) {
			case ABCAnlzFilt::GroupByPriceSum: ord = PPViewGoodsOpAnalyze::OrdByPriceSum; break;
			case ABCAnlzFilt::GroupByQtty:     ord = PPViewGoodsOpAnalyze::OrdByQtty;     break;
			case ABCAnlzFilt::GroupByIncome:   ord = PPViewGoodsOpAnalyze::OrdByIncome;   break;
			case ABCAnlzFilt::GroupByCostSum:
			default: ord = PPViewGoodsOpAnalyze::OrdByCostSum; break;
		}
	}
	else
		ord = PPViewGoodsOpAnalyze::OrdByGoodsName;
	return ord;
}
//
//
//
PPBaseFilt * PPViewGoodsOpAnalyze::CreateFilt(const void * extraPtr) const
{
	GoodsOpAnalyzeFilt * p_filt = new GoodsOpAnalyzeFilt;
	PPAccessRestriction accsr;
	p_filt->GoodsGrpID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
	return p_filt;
}

int PPViewGoodsOpAnalyze::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	Goods2Tbl::Rec grp_rec;
	double ufp_factor[4];
	MEMSZERO(ufp_factor);
	PPUserFuncProfiler ufp(PPUPRF_VIEW_GOODSOPANLZ);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	Filt.CmpPeriod.Actualize(ZERODATE);
	ZDELETE(P_GoodsList);
	Counter.Init();
	Total.Init();
	LocList_.Set(0); // @v12.1.5
	IndicatorList.freeAll(); // @v12.1.8
	MpDBlk.Z(); // @v12.1.8
	State &= ~(sTotalInited | sFiltAltGrp | sFiltExclFolder | sReval);
	BExtQuery::ZDelete(&P_IterQuery);
	Bsp.Init(Filt.Sgb);
	if(!Filt.Sgb) {
		if(Filt.Sgg != sggNone)
			Filt.Flags |= GoodsOpAnalyzeFilt::fDisplayWoPacks;
	}
	else {
		Filt.Sgg = sggNone;
		Filt.Flags &= ~GoodsOpAnalyzeFilt::fCalcRest;
		Filt.Flags |= GoodsOpAnalyzeFilt::fDisplayWoPacks;
	}
	CurrentViewOrder = GetIterOrder();
	Gsl.Init(1, 0);
	if(GObj.Fetch(Filt.GoodsGrpID, &grp_rec) > 0) {
		SETFLAG(State, sFiltAltGrp, (grp_rec.Flags & GF_ALTGROUP));
		SETFLAG(State, sFiltExclFolder, (grp_rec.Flags & GF_EXCLALTFOLD));
		{
			PPIDArray temp_list;
			THROW_MEM(P_GoodsList = new UintHashTable);
			GoodsIterator::GetListByGroup(Filt.GoodsGrpID, &temp_list);
			for(uint i = 0; i < temp_list.getCount(); i++)
				P_GoodsList->Add(temp_list.get(i));
		}
	}
	ZDELETE(P_TradePlanPacket);
	if(Filt.Flags & GoodsOpAnalyzeFilt::fBillListAsTradePlan && Filt.BillList.GetCount()) {
		THROW_MEM(P_TradePlanPacket = new PPTrfrArray);
		for(uint i = 0; i < Filt.BillList.GetCount(); i++) {
			PPBillPacket pack;
			if(P_BObj->ExtractPacket(Filt.BillList.Get().get(i), &pack) > 0) {
				PPTransferItem * p_ti;
				for(uint j = 0; pack.EnumTItems(&j, &p_ti) > 0;) {
					if(Filt.Flags & GoodsOpAnalyzeFilt::fTradePlanObjAsSuppl)
						p_ti->Suppl = pack.Rec.Object;
					THROW_SL(P_TradePlanPacket->insert(p_ti));
				}
			}
		}
	}
	if(P_Cache == 0) {
		THROW_MEM(P_Cache = new SArray(sizeof(GoaCacheItem), O_ARRAY));
	}
	else
		P_Cache->freeAll();
	if(Filt.Flags & (GoodsOpAnalyzeFilt::fIntrReval|GoodsOpAnalyzeFilt::fDiffByPrice)) {
		Cf = PTR_CMPFUNC(GoaCacheItem_CP);
		Cf_UniqItem = PTR_CMPFUNC(GoaUniqItem_CP);
	}
	else if(Filt.Flags & GoodsOpAnalyzeFilt::fDiffByNetPrice) {
		Cf = PTR_CMPFUNC(GoaCacheItem_P);
		Cf_UniqItem = PTR_CMPFUNC(GoaUniqItem_P);
	}
	else {
		Cf = PTR_CMPFUNC(GoaCacheItem);
		Cf_UniqItem = PTR_CMPFUNC(GoaUniqItem);
	}
	// @v12.1.5 {
	{
		//
		// Инициализируем список складов, по которым следует поднимать выборку. Учитываются доступные склады в правах доступа.
		//
		LocList_.Set(0);
		PPIDArray temp_loc_list;
		if(Filt.LocList.GetCount() == 0) {
			bool is_loclist_restricted = false;
			LocObj.GetWarehouseList(&temp_loc_list, &is_loclist_restricted);
			if(is_loclist_restricted)
				LocList_.Set(&temp_loc_list);
		}
		else {
			const PPIDArray & r_loc_list = Filt.LocList.Get();
			LocObj.ResolveWarehouseList(&r_loc_list, temp_loc_list);
			LocList_.Set(&temp_loc_list);
		}
	}
	// } @v12.1.5 
	THROW(CreateTempTable(ufp_factor));
	CalcTotal(&Total);
	ufp.SetFactor(0, ufp_factor[0]);
	ufp.Commit();
	CATCHZOK
	CALLPTRMEMB(P_Cache, freeAll());
	return ok;
}

int PPViewGoodsOpAnalyze::InitIterQuery(PPID grpID)
{
	BtrDbKey k__;
	int    sp_mode = spFirst;
	void * k = k__;
	TempGoodsOprTbl::Key2 k2;
	delete P_IterQuery;
	P_IterQuery = new BExtQuery(P_TempTbl, IterIdx, 16);
	P_IterQuery->selectAll();
	if(grpID) {
		if(grpID == -1)
			grpID = 0;
		P_IterQuery->where(P_TempTbl->GoodsGrp == grpID);
		MEMSZERO(k2);
		if(Filt.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz)
			k2.InOutTag = -MAXSHORT;
		k2.GoodsGrp = grpID;
		k = &k2;
		sp_mode = spGe;
	}
	P_IterQuery->initIteration(0, k, sp_mode);
	return 1;
}

int PPViewGoodsOpAnalyze::InitIteration(IterOrder ord)
{
	int    ok = 1;
	IterIdx   = 0;
	IterGrpName = 0;
	ZDELETE(P_GGIter);
	ZDELETE(P_TempOrd);
	BExtQuery::ZDelete(&P_IterQuery);
	THROW_PP(P_TempTbl, PPERR_PPVIEWNOTINITED);
	PPInitIterCounter(Counter, P_TempTbl);
	THROW(CreateOrderTable(ord, &P_TempOrd));
	if(P_TempOrd) {
		TempOrderTbl::Key1 k;
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempOrd, 1, 64));
		P_IterQuery->selectAll();
		MEMSZERO(k);
		P_IterQuery->initIteration(false, &k, spFirst);
	}
	else {
		if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze) {
			IterIdx = 1;
			InitIterQuery(0L);
		}
		else {
			IterIdx = 2;
			Goods2Tbl::Rec grp_rec;
			if(GObj.Fetch(Filt.GoodsGrpID, &grp_rec) <= 0)
				MEMSZERO(grp_rec);
			if(!Filt.Sgg) {
				PPID   parent_grp_id = (grp_rec.Flags & GF_ALTGROUP) ? 0 : Filt.GoodsGrpID;
				THROW_MEM(P_GGIter = new GoodsGroupIterator(parent_grp_id, GoodsGroupIterator::fAddZeroGroup));
				NextOuterIteration();
			}
			else {
				IterGrpName = grp_rec.Name;
				InitIterQuery(0L);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPViewGoodsOpAnalyze::NextOuterIteration()
{
	PPID   grp_id = 0;
	if(P_GGIter && P_GGIter->Next(&grp_id, IterGrpName) > 0) {
		InitIterQuery(NZOR(grp_id, -1));
		return 1;
	}
	else
		return -1;
}

int FASTCALL PPViewGoodsOpAnalyze::NextIteration(GoodsOpAnalyzeViewItem * pItem)
{
	do {
		while(P_IterQuery && P_IterQuery->nextIteration() > 0) {
			if(pItem) {
				if(P_TempOrd) {
					if(SearchByID(P_TempTbl, 0, P_TempOrd->data.ID, 0) <= 0)
						continue;
				}
				if(!Filt.IsValidABCGroup(P_TempTbl->data.InOutTag))
					continue;
				TempGoodsOprTbl::Rec rec = P_TempTbl->data;
				memzero(pItem, sizeof(GoodsOpAnalyzeViewItem));
				pItem->ID__ = rec.ID__;
				pItem->InOutTag     = rec.InOutTag;
				pItem->GoodsID      = rec.GoodsID;
				if(Filt.ABCAnlzGroup == 2) { // GroupBy ABC group
					TempGoodsOprTbl::Key1 k1;
					MEMSZERO(k1);
					k1.InOutTag = -rec.InOutTag;
					if(P_TempTbl->search(1, &k1, spGe) && k1.InOutTag == (short)-rec.InOutTag) {
						pItem->GoodsGrpID   = P_TempTbl->data.GoodsGrp;
						IterGrpName = P_TempTbl->data.Text;
						pItem->OldPrice     = P_TempTbl->data.OldPrice; // position percent in group GoodsGrpID
						pItem->PctVal       = P_TempTbl->data.PctVal;
					}
					else {
						pItem->GoodsGrpID   = rec.GoodsGrp;
						pItem->OldPrice     = rec.OldPrice;
						pItem->PctVal       = rec.PctVal;
					}
				}
				else {
					pItem->GoodsGrpID   = rec.GoodsGrp;
					pItem->OldPrice     = rec.OldPrice;
					pItem->PctVal       = rec.PctVal;
				}
				pItem->P_GoodsGrpName = IterGrpName;
				pItem->UnitPerPack    = rec.UnitPerPack;
				pItem->Qtty   = rec.Quantity;
				pItem->PhQtty = rec.PhQtty;
				pItem->Cost   = rec.Cost;
				pItem->Price  = rec.Price;
				pItem->OldCost        = rec.OldCost;
				pItem->SumCost        = rec.SumCost;
				pItem->SumPrice       = rec.SumPrice;
				pItem->Income = rec.Income;
				pItem->Rest   = rec.Rest;
				pItem->PhRest = rec.PhRest;
				pItem->RestCostSum    = rec.RestCostSum;
				pItem->RestPriceSum   = rec.RestPriceSum;
				pItem->LocID  = rec.LocID;
				STRNSCPY(pItem->GoodsName, rec.Text);
				if(!!Filt.Sgb) {
					if(oneof3(Filt.Sgb.S, SubstGrpBill::sgbObject, SubstGrpBill::sgbObject2, SubstGrpBill::sgbAgent)) {
						pItem->SubstArID  = rec.GoodsID;
						pItem->SubstPsnID = ObjectToPerson(pItem->SubstArID, 0);
					}
					else if(oneof2(Filt.Sgb.S, SubstGrpBill::sgbDlvrLoc, SubstGrpBill::sgbLocation)) {
						pItem->SubstLocID = rec.GoodsID;
					}
				}
				else if(oneof2(Filt.Sgg, sggSuppl, sggSupplAgent)) {
					pItem->SubstArID  = (rec.GoodsID & ~GOODSSUBSTMASK);
					pItem->SubstPsnID = ObjectToPerson(pItem->SubstArID, 0);
				}
				else if(Filt.Sgg == sggLocation) {
					pItem->SubstLocID = (rec.GoodsID & ~GOODSSUBSTMASK);
				}
				else if(Filt.Sgg == sggManuf) {
					pItem->SubstPsnID = (rec.GoodsID & ~GOODSSUBSTMASK);
				}
				if(P_Ct == 0 && Filt.CmpPeriod.IsZero() == 0) {
					TempGoodsOprTbl::Rec rec2;
					P_CmpView->GetByID(rec.ID__, &rec2);
					pItem->Qtty.SetCm(rec2.Quantity);
					pItem->SumCost.SetCm(rec2.SumCost);
					pItem->SumPrice.SetCm(rec2.SumPrice);
					pItem->Income.SetCm(rec2.Income);
					pItem->Rest.SetCm(rec2.Rest);
					pItem->PhRest.SetCm(rec2.PhRest);
					pItem->RestCostSum.SetCm(rec2.RestCostSum);
					pItem->RestPriceSum.SetCm(rec2.RestPriceSum);
				}
			}
			Counter.Increment();
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

int PPViewGoodsOpAnalyze::CalcTotal(GoodsOpAnalyzeTotal * pTotal)
{
	TempGoodsOprTbl::Key0 k;
	PPObjGoodsType gtobj;
	TempGoodsOprTbl * p_t = P_TempTbl;
	BExtQuery q(P_TempTbl, 0, 16);
	q.select(p_t->ID__, p_t->InOutTag, p_t->GoodsID, p_t->Quantity, p_t->PhQtty,
		p_t->SumCost, p_t->SumPrice, p_t->Income, p_t->RestCostSum, p_t->RestPriceSum,
		p_t->OldCost, p_t->OldPrice, 0L);
	MEMSZERO(k);
	for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
		if(Filt.IsValidABCGroup(p_t->data.InOutTag)) {
			PPID amt_type_cost  = PPAMT_BUYING;
			PPID amt_type_price = PPAMT_SELLING;
			if(p_t->data.InOutTag < 0 && (!(Filt.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz))) {
				pTotal->InCount++;
				pTotal->InQtty   += p_t->data.Quantity;
				pTotal->InPhQtty += p_t->data.PhQtty;
				pTotal->InCost   += p_t->data.SumCost;
				pTotal->InPrice  += p_t->data.SumPrice;
				pTotal->InIncome += p_t->data.Income;
			}
			else {
				Goods2Tbl::Rec goods_rec;
				pTotal->Count++;
				pTotal->Qtty   += p_t->data.Quantity;
				pTotal->PhQtty += p_t->data.PhQtty;
				pTotal->Cost   += p_t->data.SumCost;
				pTotal->Price  += p_t->data.SumPrice;
				pTotal->Income += p_t->data.Income;
				pTotal->Rest   += p_t->data.Rest;
				pTotal->PhRest += p_t->data.PhRest;
				pTotal->RestCost  += p_t->data.RestCostSum;
				pTotal->RestPrice += p_t->data.RestPriceSum;
				if(GObj.Fetch(p_t->data.GoodsID, &goods_rec) > 0) {
					PPID goods_type_id = goods_rec.GoodsTypeID;
					if(goods_type_id && goods_type_id != PPGT_DEFAULT) {
						PPGoodsType gt;
						if(gtobj.Fetch(goods_type_id, &gt) > 0) {
							if(gt.AmtCost)
								amt_type_cost = gt.AmtCost;
							if(gt.AmtPrice)
								amt_type_price = gt.AmtPrice;
						}
					}
				}
				pTotal->Amounts.Add(amt_type_cost,  0L/*@curID*/, p_t->data.SumCost, 0);
				pTotal->Amounts.Add(amt_type_price, 0L/*@curID*/, p_t->data.SumPrice, 0);
			}
		}
		if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) { // @v12.1.11
			const IndicatorVector * p_ind = GetIndicatorEntryC(p_t->data.ID__);
			if(p_ind) {
				for(uint i = 0; i < p_ind->getCount(); i++) {
					const BzsVal & r_val = p_ind->at(i);
					if(PPObjBizScore::IsBzsiAdditive(r_val.Bzsi)) {
						Total.IndicatorVect.Add(r_val.Bzsi, r_val.Val);
					}
				}
			}
		}
	}
	if(P_TradePlanPacket) {
		PPTransferItem * p_plan_ti = 0;
		for(uint i = 0; P_TradePlanPacket->enumItems(&i, (void **)&p_plan_ti) > 0;) {
			pTotal->PlanQtty += fabs(p_plan_ti->Quantity_);
			pTotal->PlanSum  += p_plan_ti->NetPrice() * fabs(p_plan_ti->Quantity_);
		}
	}
	State |= sTotalInited;
	return 1;
}

int FASTCALL PPViewGoodsOpAnalyze::CheckBillRec(const BillTbl::Rec * pRec)
{
	PPID   psn_id, city_id;
	if((Filt.Flags & GoodsOpAnalyzeFilt::fLabelOnly) && !(pRec->Flags & BILLF_WHITELABEL))
		return 0;
	if(!CheckFiltID(Filt.ObjectID, pRec->Object))
		return 0;
	if(!CheckFiltID(Filt.Object2, pRec->Object2))
		return 0;
	if(!LocList_.CheckID(pRec->LocID))
		return 0;
	if(Filt.ObjCityID) {
		psn_id = ObjectToPerson(pRec->Object);
		if(psn_id) {
			city_id = 0;
			PPID   addr_id = 0;
			if(PsnObj.GetAddrID(psn_id, 0, PSNGETADDRO_DEFAULT, &addr_id) > 0) {
				PsnObj.GetCityByAddr(addr_id, &city_id, 0);
				if(city_id != Filt.ObjCityID)
					return 0;
			}
			else
				return 0;
		}
		else
			return 0;
	}
	if(Filt.FreightAgentID) {
		PPFreight freight;
		if(!(pRec->Flags & BILLF_FREIGHT) || (P_BObj->P_Tbl->GetFreight(pRec->ID, &freight) <= 0) || freight.AgentID != Filt.FreightAgentID)
			return 0;
	}
	return 1;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempGoodsOpr);

int PPViewGoodsOpAnalyze::InitGoodsRestView(PPViewGoodsRest * pGrView)
{
	assert(pGrView);
	int    ok = 1;
	GoodsRestFilt gr_filt;
	gr_filt.CalcMethod = GoodsRestParam::pcmAvg;
	gr_filt.Date = Filt.RestCalcDate;
	if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
		gr_filt.Flags |= GoodsRestFilt::fEachLocation;
	else if(LocList_.IsExists())
		gr_filt.LocList = LocList_;
	if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder)
		gr_filt.Flags |= GoodsRestFilt::fCalcOrder;
	if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales)
		gr_filt.Flags |= GoodsRestFilt::fCalcSStatSales;
	gr_filt.GoodsGrpID = Filt.GoodsGrpID;
	gr_filt.SupplID = Filt.SupplID;
	gr_filt.Sgg = Filt.Sgg;
	pGrView->SetGsl(&Gsl);
	gr_filt.Flags |= GoodsRestFilt::fOuterGsl;
	THROW(pGrView->Init_(&gr_filt));
	CATCHZOK
	return ok;
}

class ABCGroupingRecsStorage {
public:
	static double GetValueByGrouping(uint groupBy, const GoodsOpAnalyzeViewItem * pItem);

	ABCGroupingRecsStorage()
	{
		ABCGrpRecs = new SArray(sizeof(TempGoodsOprTbl::Rec));
		Init(0, 0, 0);
	}
	~ABCGroupingRecsStorage()
	{
		ZDELETE(ABCGrpRecs);
	}
	void   Init(const ABCAnlzFilt *, double totalSum, long totalRecs);
	int    CalcBelongToABCGrp(const GoodsOpAnalyzeViewItem *, short * pABCGrp, int finish = 0);
	int    EnumItems(short * pABCGrp, TempGoodsOprTbl::Rec * pRec);
private:
	/*
	enum ValueType {
		vTotalSum       = 0,
		vFraction       = 1,
		vTotalFraction  = 2,
		vMinFraction    = 3,
		vRecsCount      = 4,
		vMinRecsCount   = 5,
		vTotalRecsCount = 6,
		vABCGrp = 7,
		vMinABCGrp      = 8
	};
	*/
	double GetMaxFraction(short * pABCGrp);
	double GetMinFraction(short * pABCGrp);
	void   AccumulateVals(TempGoodsOprTbl::Rec *, const GoodsOpAnalyzeViewItem *);
	int    SaveValToMinABCGrp(double val, const GoodsOpAnalyzeViewItem * pItem);

	SArray * ABCGrpRecs;
	PPIDArray ExcludedGrps;
	ABCAnlzFilt Filt;
	TempGoodsOprTbl::Rec TempRec;
	double TotalSum;
	double Fraction;
	double TotalFraction;
	double MinFraction;
	long   RecsCount;
	long   MinRecsCount;
	long   TotalRecsCount;
	short  ABCGrp;
	short  MinABCGrp;
};

void ABCGroupingRecsStorage::Init(const ABCAnlzFilt * pFilt, double totalSum, long totalRecsCount)
{
	ABCGrpRecs->freeAll();
	ExcludedGrps.freeAll();
	if(!RVALUEPTR(Filt, pFilt))
		MEMSZERO(Filt);
	MinFraction = Fraction = 0.0;
	RecsCount = MinRecsCount = 0;
	TotalSum = NZOR(totalSum, 1.0);
	TotalRecsCount = (totalRecsCount > 0) ? totalRecsCount : 1;
	GetMinFraction(&MinABCGrp);
	if(MinABCGrp != -1) {
		MEMSZERO(TempRec);
		Filt.GetGroupName(MinABCGrp, TempRec.Text, sizeof(TempRec.Text));
		TempRec.InOutTag = MinABCGrp + 1;
		ABCGrpRecs->insert(&TempRec);
	}
	TotalFraction = GetMaxFraction(&ABCGrp);
	MEMSZERO(TempRec);
}

double ABCGroupingRecsStorage::GetMaxFraction(short * pABCGrp)
{
	int    idx = -1;
	double max_val = 0.0;
	for(int i = 0; i < ABC_GRPSCOUNT; i++)
		if(Filt.GrpFract[i] > max_val && !ExcludedGrps.lsearch(i)) {
			idx = i;
			max_val = Filt.GrpFract[i];
		}
	if(idx > -1)
		ExcludedGrps.add(idx);
	ASSIGN_PTR(pABCGrp, static_cast<short>(idx));
	return fdiv100r(max_val) * TotalSum;
}

double ABCGroupingRecsStorage::GetMinFraction(short * pABCGrp)
{
	int    idx = -1;
	double min_val = SMathConst::Max;
	for(int i = 0; i < ABC_GRPSCOUNT; i++)
		if(Filt.GrpFract[i] <= min_val && Filt.GrpFract[i] > 0) {
			idx = i;
			min_val = Filt.GrpFract[i];
		}
	if(idx > -1)
		ExcludedGrps.add(idx);
	ASSIGN_PTR(pABCGrp, static_cast<short>(idx));
	return fdiv100r(min_val) * TotalSum;
}

/*static*/double ABCGroupingRecsStorage::GetValueByGrouping(uint groupBy, const GoodsOpAnalyzeViewItem * pItem)
{
	double val = 0.0;
	if(pItem)
		switch(groupBy) {
			case ABCAnlzFilt::GroupByCostSum:  val = pItem->SumCost;  break;
			case ABCAnlzFilt::GroupByPriceSum: val = pItem->SumPrice; break;
			case ABCAnlzFilt::GroupByQtty:     val = pItem->Qtty;     break;
			case ABCAnlzFilt::GroupByIncome:   val = pItem->Income;   break;
		}
	return val;
}

int ABCGroupingRecsStorage::CalcBelongToABCGrp(const GoodsOpAnalyzeViewItem * pItem, short * pABCGrp, int finish)
{
	int    ok = 1;
	short  abc_grp = -1;
	double val = GetValueByGrouping(Filt.GroupBy, pItem);
	if(((Fraction + (val / 2.0)) > TotalFraction && Fraction != 0.0 || finish) && ABCGrp != -1) {
		TempRec.InOutTag    = ABCGrp + 1;
		TempRec.PctVal      = (Fraction / TotalSum) * 100.0;
		TempRec.OldPrice    = fdivi(RecsCount, TotalRecsCount) * 100.0;
		TempRec.OldCost     = (double)RecsCount;
		Filt.GetGroupName(ABCGrp, TempRec.Text, sizeof(TempRec.Text));
		THROW_SL(ABCGrpRecs->insert(&TempRec));
		RecsCount = 0;
		Fraction = 0.0;
		TotalFraction = GetMaxFraction(&ABCGrp);
		MEMSZERO(TempRec);
		abc_grp = ABCGrp;
	}
	if(val > 0 && (Fraction + val / 2.0 <= TotalFraction) && ABCGrp != -1 && !finish) {
		AccumulateVals(&TempRec, pItem);
		Fraction += val;
		abc_grp = ABCGrp;
		RecsCount++;
	}
	else {
		if(!finish)
			MinRecsCount++;
		THROW(ok = SaveValToMinABCGrp(val, pItem));
		abc_grp = MinABCGrp;
	}
	CATCH
		ok = 0;
		abc_grp = -1;
	ENDCATCH
	ASSIGN_PTR(pABCGrp, ((abc_grp < 0) ? 0 : abc_grp + 1));
	return ok;
}

int ABCGroupingRecsStorage::EnumItems(short * pABCGrp, TempGoodsOprTbl::Rec * pRec)
{
	int    ok = -1;
	if(pABCGrp && *pABCGrp <= (short)ABC_GRPSCOUNT && *pABCGrp > 0) {
		TempGoodsOprTbl::Rec rec;
		uint   pos = 0;
		if(Filt.GrpFract[*pABCGrp-1] > 0)
			if(ABCGrpRecs->lsearch(pABCGrp, &pos, PTR_CMPFUNC(int16), sizeof(long) * 2))
				rec = *static_cast<const TempGoodsOprTbl::Rec *>(ABCGrpRecs->at(pos));
			else {
				Filt.GetGroupName(*pABCGrp - 1, rec.Text, sizeof(rec.Text));
				rec.InOutTag = *pABCGrp;
			}
		(*pABCGrp)++;
		ASSIGN_PTR(pRec, rec);
		ok = 1;
	}
	return ok;
}

void ABCGroupingRecsStorage::AccumulateVals(TempGoodsOprTbl::Rec * pRec, const GoodsOpAnalyzeViewItem * pItem)
{
	if(pRec && pItem) {
		pRec->SumCost  += pItem->SumCost;
		pRec->SumPrice += pItem->SumPrice;
		pRec->Quantity += pItem->Qtty;
		pRec->Income   += pItem->Income;
		pRec->RestCostSum  += pItem->RestCostSum;
		pRec->RestPriceSum += pItem->RestPriceSum;
	}
}

int ABCGroupingRecsStorage::SaveValToMinABCGrp(double val, const GoodsOpAnalyzeViewItem * pItem)
{
	int    ok = -1;
	uint   pos = 0;
	if(MinABCGrp >= 0) {
		short  srch_val = MinABCGrp + 1;
		if(ABCGrpRecs->lsearch(&srch_val, &pos, PTR_CMPFUNC(int16), sizeof(long) * 2)) {
			TempGoodsOprTbl::Rec rec = *static_cast<TempGoodsOprTbl::Rec *>(ABCGrpRecs->at(pos));
			rec.OldPrice = fdivi(MinRecsCount, TotalRecsCount) * 100.0;
			rec.OldCost  = (double)MinRecsCount;
			MinFraction += val;
			rec.PctVal   = (MinFraction / TotalSum) * 100.0;
			AccumulateVals(&rec, pItem);
			ABCGrpRecs->atFree(pos);
			THROW_SL(ABCGrpRecs->insert(&rec));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

class ABCGrpStorageList {
public:
	struct TotalItem {
		TotalItem() : GoodsGrpID(0), RecsCount(0), TotalSum(0.0)
		{
		}
		long   GoodsGrpID;
		long   RecsCount;
		double TotalSum;
	};

	explicit ABCGrpStorageList(PPObjGoods * pGObj) : P_GObj(pGObj), P_TotalItems(0), P_StorageList(0), EnumIdx(0)
	{
	}
	ABCGrpStorageList()
	{
		delete[] P_StorageList;
		ZDELETE(P_TotalItems);
	}
	int    Init(const ABCAnlzFilt *);
	int    InitGoodsGrpList(GoodsOpAnalyzeFilt * pFilt);
	int    CalcBelongToABCGrp(const GoodsOpAnalyzeViewItem *, short * pABCGrp, int finish = 0);
	int    EnumItems(short * pABCGrp, TempGoodsOprTbl::Rec * pRec);
	int    IncTotalItem(uint groupBy, const GoodsOpAnalyzeViewItem * pItem);
private:
	long   GetGoodsGrpPos(PPID goodsID);
	ABCGroupingRecsStorage * GetStorage(PPID goodsID);

	uint   EnumIdx;
	SArray * P_TotalItems;
	PPIDArray GoodsGroupList;
	ABCGroupingRecsStorage * P_StorageList;
	PPObjGoods * P_GObj;
};

int ABCGrpStorageList::Init(const ABCAnlzFilt * pFilt)
{
	int    ok = -1;
	if(pFilt && P_TotalItems) {
		long count = P_TotalItems->getCount();
		if(count) {
			THROW_MEM(P_StorageList = new ABCGroupingRecsStorage[count]);
			for(long i = 0; i < count; i++) {
				TotalItem * p_item = (TotalItem*)P_TotalItems->at((uint)i);
				P_StorageList[i].Init(pFilt, p_item->TotalSum, p_item->RecsCount);
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int ABCGrpStorageList::InitGoodsGrpList(GoodsOpAnalyzeFilt * pFilt)
{
	int  ok = -1;
	int  abc_by_ggrps = BIN(pFilt && pFilt->Flags & GoodsOpAnalyzeFilt::fABCAnlzByGGrps);
	StrAssocArray * p_ggrp_list = 0;
	if(abc_by_ggrps) {
		if(pFilt->GoodsGrpID) {
			if(P_GObj->CheckFlag(pFilt->GoodsGrpID, GF_EXCLALTFOLD) || P_GObj->CheckFlag(pFilt->GoodsGrpID, GF_FOLDER)) {
				PPIDArray child_list;
				if(P_GObj->P_Tbl->GetGroupTerminalList(pFilt->GoodsGrpID, &child_list, 0) > 0 && child_list.getCount()) {
					THROW_MEM(p_ggrp_list = new StrAssocArray());
					for(uint i = 0; i < child_list.getCount(); i++) {
						PPID id = child_list.at(i);
						p_ggrp_list->Add(id, 0, 0);
					}
				}
			}
		}
		else {
			PPObjGoodsGroup ggobj;
			p_ggrp_list = ggobj.MakeStrAssocList((void *)GGRTYP_SEL_NORMAL);
			for(long i = p_ggrp_list->getCount(); i >= 0; i--) {
				PPID ggrp_id = p_ggrp_list->Get(i).Id;
				if(P_GObj->CheckFlag(ggrp_id, GF_FOLDER))
					p_ggrp_list->Remove(ggrp_id);
			}
		}
	}
	THROW_MEM(P_TotalItems = new SArray(sizeof(ABCGrpStorageList::TotalItem)));
	if(p_ggrp_list && p_ggrp_list->getCount()) {
		for(uint i = 0; i < p_ggrp_list->getCount(); i++) {
		 	ABCGrpStorageList::TotalItem item;
			item.GoodsGrpID = p_ggrp_list->Get(i).Id;
			P_TotalItems->insert(&item);
			GoodsGroupList.add(item.GoodsGrpID);
		}
		ok = 1;
	}
	else {
		ABCGrpStorageList::TotalItem item;
		P_TotalItems->insert(&item);
		GoodsGroupList.add(0L);
	}
	CATCHZOK
	ZDELETE(p_ggrp_list);
	return ok;
}

long ABCGrpStorageList::GetGoodsGrpPos(PPID goodsID)
{
	long pos = -1;
	if(P_TotalItems) {
		uint count = GoodsGroupList.getCount();
		if(count == 1 && GoodsGroupList.at(0) == 0)
			pos = 0;
		else
			for(uint i = 0; pos == -1 && i < count; i++)
				if(P_GObj->BelongToGroup(goodsID, GoodsGroupList.at(i)) > 0)
					pos = i;
	}
	return pos;
}

int ABCGrpStorageList::IncTotalItem(uint groupBy, const GoodsOpAnalyzeViewItem * pItem)
{
	int    ok = -1;
	if(pItem) {
		const long pos = GetGoodsGrpPos(pItem->GoodsID);
		if(pos >= 0) {
			double val = ABCGroupingRecsStorage::GetValueByGrouping(groupBy, pItem);
			ABCGrpStorageList::TotalItem * p_item = static_cast<ABCGrpStorageList::TotalItem *>(P_TotalItems->at(static_cast<uint>(pos)));
			p_item->TotalSum += (val >= 0) ? val : 0;
			p_item->RecsCount++;
		}
		ok = 1;
	}
	return ok;
}

ABCGroupingRecsStorage * ABCGrpStorageList::GetStorage(PPID goodsID)
{
	ABCGroupingRecsStorage * p_storage = 0;
	if(P_StorageList) {
		const long pos = GetGoodsGrpPos(goodsID);
		if(pos >= 0)
			p_storage = &P_StorageList[pos];
	}
	return p_storage;
}

int ABCGrpStorageList::CalcBelongToABCGrp(const GoodsOpAnalyzeViewItem * pItem, short * pABCGrp, int finish)
{
	int    ok = -1;
	if(finish) {
		const uint count = GoodsGroupList.getCount();
		ok = 1;
		for(uint i = 0; i < count; i++)
			if(P_StorageList[i].CalcBelongToABCGrp(pItem, pABCGrp, finish) <= 0)
				ok = -1;
	}
	else {
		const long pos = GetGoodsGrpPos(pItem->GoodsID);
		if(pos >= 0 && P_StorageList) {
			ABCGroupingRecsStorage * p_storage = &P_StorageList[pos];
			ok = p_storage->CalcBelongToABCGrp(pItem, pABCGrp, finish);
			if(pABCGrp)
				*pABCGrp += (short)(pos * ABC_GRPSCOUNT);
		}
	}
	return ok;
}

int ABCGrpStorageList::EnumItems(short * pABCGrp, TempGoodsOprTbl::Rec * pRec)
{
	uint   count = GoodsGroupList.getCount();
	if(*pABCGrp == 1)
		EnumIdx = 0;
	else
		*pABCGrp -= (short)EnumIdx * ABC_GRPSCOUNT;
	int    is_valid = 0;
	while(!is_valid && EnumIdx < count) {
		ABCGroupingRecsStorage * p_storage = &P_StorageList[EnumIdx];
		is_valid = BIN(p_storage->EnumItems(pABCGrp, pRec) > 0);
		if(!is_valid) {
			*pABCGrp = 1;
			EnumIdx++;
		}
	}
	if(is_valid && pRec) {
		if(GoodsGroupList.at(EnumIdx) != 0) {
			SString temp_buf, name;
			GetObjectName(PPOBJ_GOODSGROUP, GoodsGroupList.at(EnumIdx), name);
			(temp_buf = name).Space().Cat(pRec->Text);
			// (temp_buf = pRec->Text).Space().Cat(name);
			temp_buf.CopyTo(pRec->Text, sizeof(pRec->Text));
		}
		*pABCGrp += (short)EnumIdx * ABC_GRPSCOUNT;
		if(pRec->InOutTag)
			pRec->InOutTag = *pABCGrp - 1;

	}
	return is_valid;
}

int PPViewGoodsOpAnalyze::PutBillToTempTable(PPBillPacket * pPack, double part, int sign, const PPIDArray * pSupplBillList)
{
	int    ok = 1;
	PPBillPacket * p_link_pack = 0;
	if(Filt.IsLeadedInOutAnalyze()) {
		ok = -1;
		if(pPack->HasOneOfGoods(Filt.GoodsIdList)) {
			for(uint i = 0; ok < 0 && i < Filt.GoodsIdList.GetCount(); i++) {
				const  PPID goods_id = Filt.GoodsIdList.Get(i);
				uint gp = 0;
				if(goods_id && pPack->SearchGoods(goods_id, &gp)) {
					const PPTransferItem & r_ti = pPack->ConstTI(gp);
					if(r_ti.GetSign(pPack->Rec.OpID) == TISIGN_PLUS)
						ok = 1;
				}
			}
		}
	}
	if(ok > 0) {
		long   subst_bill_val = 0;
		PPIDArray goods_list;
		GoaAddingBlock blk;
		InitAddingBlock(pPack, part, sign, &blk);
		PPTransferItem * p_ti = 0;
		if(Filt.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt) {
			DateIter di;
			BillTbl::Rec link_rec;
			//
			// Объединяем строки из всех документов списания в один пакет p_link_pack
			//
			for(DateIter di; P_BObj->P_Tbl->EnumLinks(pPack->Rec.ID, &di, BLNK_ALL, &link_rec) > 0;) {
				PPTransferItem ti;
				PPBillPacket temp_pack;
				THROW_MEM(SETIFZ(p_link_pack, new PPBillPacket));
				THROW(P_BObj->ExtractPacket(link_rec.ID, &temp_pack));
				for(temp_pack.InitExtTIter(ETIEF_UNITEBYGOODS, 0); temp_pack.EnumTItemsExt(0, &ti) > 0;) {
					THROW(p_link_pack->LoadTItem(&ti, 0, 0));
				}
			}
			if(p_link_pack) {
				PPTransferItem ti;
				for(p_link_pack->InitExtTIter(ETIEF_UNITEBYGOODS, 0); p_link_pack->EnumTItemsExt(0, &ti) > 0;) {
					if(!pPack->SearchGoods(ti.GoodsID, 0)) {
						ti.Quantity_ = 0.0;
						ti.WtQtty = 0.0;
						THROW(pPack->LoadTItem(&ti, 0, 0));
					}
				}
			}
		}
		if(!!Filt.Sgb)
			P_BObj->Subst(pPack, &subst_bill_val, &Bsp);
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti);) {
			int    r;
			THROW(PPCheckUserBreak());
			THROW(r = PreprocessTi(p_ti, pSupplBillList, subst_bill_val, &blk));
			if(r > 0) {
				if(p_link_pack && !goods_list.lsearch(blk.GoodsRec.ID)) {
					blk.PhQtty   = 0.0;
					blk.OldCost  = 0.0;
					blk.OldPrice = 0.0;
					PPTransferItem ti;
					for(p_link_pack->InitExtTIter(ETIEF_UNITEBYGOODS, 0); p_link_pack->EnumTItemsExt(0, &ti) > 0;) {
						if(ti.GoodsID == blk.GoodsRec.ID) {
							blk.PhQtty   += ti.Quantity_;
							blk.OldCost  += ti.Quantity_ * ti.Cost;
							blk.OldPrice += ti.Quantity_ * ti.Price;
							goods_list.addUnique(ti.GoodsID);
							//break;
						}
					}
				}
				PROFILE_S(THROW(AddItem(blk)), "AddItem()");
			}
		}
	}
	CATCHZOK
	ZDELETE(p_link_pack);
	return ok;
}

void PPViewGoodsOpAnalyze::GetTabTitle(long tabID, SString & rBuf)
{
	if(tabID) {
		LocationTbl::Rec loc_rec;
		if(LocObj.Search(tabID, &loc_rec) > 0)
			rBuf = loc_rec.Name;
		else
			rBuf.Z().Cat(tabID);
	}
}

void PPViewGoodsOpAnalyze::GetEditIds(const void * pRow, PPID * pLocID, PPID * pGoodsID, long col)
{
	PPID   loc_id   = 0;
	PPID   goods_id = 0;
	if(pRow) {
		if(P_Ct) {
			int    crsst_flds = Filt.CompareItems.getCountI();
			col = (col > 0 && Filt.Sgg == sggNone && !Filt.Sgb) ? (col - 1) : col;
			crsst_flds = (crsst_flds > 0) ? crsst_flds : 1;
			uint   tab_idx = (col > 0) ? (col / (crsst_flds + 1)) + 1 : col;
			int    r = (tab_idx > 0) ? P_Ct->GetTab(tab_idx - 1, &loc_id) : 1;
			if(r > 0)
				P_Ct->GetIdxFieldVal(0, pRow, &goods_id, sizeof(goods_id));
		}
		else {
			loc_id   = static_cast<const BrwHdr *>(pRow)->LocID;
			goods_id = static_cast<const BrwHdr *>(pRow)->GoodsID;
		}
	}
	ASSIGN_PTR(pLocID, loc_id);
	ASSIGN_PTR(pGoodsID, goods_id);
}

class GoodsOpAnlzCrosstab : public Crosstab {
public:
	struct TotalItem {
		double Sum;
		long   MtxCount;
		long   NzMtxCount;
		double RelNzToMtx;
	};
	GoodsOpAnlzCrosstab(PPViewGoodsOpAnalyze * pV) : Crosstab(), P_V(pV)
	{
	}
	virtual BrowserWindow * CreateBrowser(uint brwId, int dataOwner)
	{
		PPViewBrowser * p_brw = new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner);
		SetupBrowserCtColumns(p_brw);
		return p_brw;
	}
private:
	virtual void GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
	{
		if(pVal && /*typ == MKSTYPE(S_INT, 4) &&*/ P_V) 
			P_V->GetTabTitle(*static_cast<const long *>(pVal), rBuf);
	}
	PPViewGoodsOpAnalyze * P_V;
};

struct __GoaBillEntry {
	PPID   BillID;
	int    IsPaym;
	double Payment;
	double Part;
};

int PPViewGoodsOpAnalyze::CreateTempTable(double * pUfpFactors)
{
	assert(pUfpFactors != 0);

	int    ok = 1;
	bool   debug_mark = false;
	PPIDArray op_list;
	PPIDArray neg_op_list;
	PPOprKind op_rec;
	PPID   id;
	TempGoodsOprTbl * p_prev_temp_tbl = 0;
	SString wait_msg;
	double payment = 0.0;
	PPBillPacket pack;
	PPObjOprKind op_obj;
	PPIDArray ext_bill_list;
	PPIDArray suppl_bill_list;
	PPIDArray * p_suppl_bill_list = 0;
	bool   use_ext_list = false;
	ZDELETE(P_CmpView);
	ZDELETE(P_TempTbl);
	if(!Filt.CmpPeriod.IsZero() || (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze)) {
		THROW(InitUniq(0));
	}
	THROW(P_TempTbl = CreateTempFile());
	if(P_TradePlanPacket) {
		GoaAddingBlock blk;
		//InitAddingBlock(P_TradePlanPacket, 1, 0, &blk);
		const  long f = Filt.Flags;
		long   subst_bill_val = 0;
		blk.ParentGrpID = 0;
		blk.Flags |= GoaAddingBlock::fTradePlan;
		blk.BillSign = 1;
		blk.Part = 1.0;
		/* @construction
		if(!!Filt.Sgb)
			P_BObj->Subst(P_TradePlanPacket, &subst_bill_val, &Bsp);
		*/
		PPTransferItem * p_ti;
		for(uint i = 0; P_TradePlanPacket->enumItems(&i, (void **)&p_ti);) {
			int r;
			blk.LocID = (f & GoodsOpAnalyzeFilt::fEachLocation) ? p_ti->LocID : LocList_.GetSingle();
			THROW(r = PreprocessTi(p_ti, 0, subst_bill_val, &blk));
			if(r > 0)
				THROW(AddItem(blk));
		}
		pUfpFactors[0] += static_cast<double>(P_TradePlanPacket->getCount());
	}
	if(Filt.AgentID) {
		// @todo Опционально использовать индивидуальную фильтрацию по расширению документа
		THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.AgentID, 0L, ext_bill_list));
		// Сортировку выполняет GetBillListByExt
		use_ext_list = true;
	}
	if(Filt.SupplAgentID) {
		// @todo Опционально использовать индивидуальную фильтрацию по расширению документа
		THROW(P_BObj->P_Tbl->GetBillListByExt(Filt.SupplAgentID, 0L, suppl_bill_list));
		p_suppl_bill_list = &suppl_bill_list;
	}
	if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) { // @v12.1.7
		PPMarketplaceConfig cfg;
		PrcssrMarketplaceInterchange::ReadConfig(&cfg);
		assert(P_Uniq);
		if(cfg.OrderOpID && cfg.SalesOpID) {
			// Лидирующая операция - продажа. То есть, мы просматриваем продажи за период, а заказы
			// перебираем следующим образом: все заказы за период + все заказы, к которым привязаны продажи.
			//
			BillTbl::Rec bill_rec;
			BillTbl::Key2 k2;
			BillCore * p_bt = P_BObj->P_Tbl;
			PPIDArray local_ord_list; // Заказы, к которым привязан конкретный док отгрузки
			PPIDArray seen_ord_lot_list; // Идентификаторы лотов заказов, которые мы уже учли в отчете
			PPIDArray temp_list;
			PPIDArray goods_list;
			PPIDArray wh_list;
			PPID   mp_acc_id = 0;
			PPID   mp_acs_id = 0;
			PrcssrMarketplaceInterchange * p_mp_prc = 0;
			MpDBlk.Z();
			if(Filt.MarketplaceGuaID) {
				MarketplaceInterchangeFilt mp_filt;
				mp_filt.GuaID = Filt.MarketplaceGuaID;
				p_mp_prc = new PrcssrMarketplaceInterchange();
				if(p_mp_prc->Init(&mp_filt)) {
					mp_acc_id = p_mp_prc->Helper_GetMarketplaceOpsAccount(false, 0);
					if(mp_acc_id) {
						mp_acs_id = p_mp_prc->Helper_GetMarketplaceOpsAccSheetID(false, false, 0);
					}
				}
				else {
					ZDELETE(p_mp_prc);
				}
			}
			{
				// Продажи
				BExtQuery q(p_bt, 2);
				q.select(p_bt->ID, p_bt->Code, p_bt->Dt, p_bt->LocID, p_bt->OpID, p_bt->Object, p_bt->Object2, p_bt->Flags,
					p_bt->LinkBillID, p_bt->Amount, 0).where(p_bt->OpID == cfg.SalesOpID && daterange(p_bt->Dt, &Filt.Period) && p_bt->EdiOp == PPEDIOP_MRKTPLC_SALE);
				MEMSZERO(k2);
				k2.OpID = cfg.SalesOpID;
				k2.Dt = Filt.Period.low;
				for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
					THROW(PPCheckUserBreak());
					p_bt->copyBufTo(&bill_rec);
					MpDBlk.ShipmList.add(bill_rec.ID);
					temp_list.Z();
					p_bt->GetListOfOrdersByLading(bill_rec.ID, temp_list);
					MpDBlk.OrdList.add(&temp_list);
				}
			}
			if(cfg.ReturnOpID) {
				// Возвраты
				BExtQuery q(p_bt, 2);
				q.select(p_bt->ID, p_bt->Code, p_bt->Dt, p_bt->LocID, p_bt->OpID, p_bt->Object, p_bt->Object2, p_bt->Flags,
					p_bt->LinkBillID, p_bt->Amount, 0).where(p_bt->OpID == cfg.ReturnOpID && daterange(p_bt->Dt, &Filt.Period) && p_bt->EdiOp == PPEDIOP_MRKTPLC_RETURN);
				MEMSZERO(k2);
				k2.OpID = cfg.ReturnOpID;
				k2.Dt = Filt.Period.low;
				for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
					THROW(PPCheckUserBreak());
					p_bt->copyBufTo(&bill_rec);
					MpDBlk.RetList.add(bill_rec.ID);
				}
			}
			{
				// Заказы
				BExtQuery q(p_bt, 2);
				q.select(p_bt->ID, p_bt->Code, p_bt->Dt, p_bt->LocID, p_bt->OpID, p_bt->Object, p_bt->Object2, p_bt->Flags,
					p_bt->LinkBillID, p_bt->Amount, 0).where(p_bt->OpID == cfg.OrderOpID && daterange(p_bt->Dt, &Filt.Period) && p_bt->EdiOp == PPEDIOP_MRKTPLC_ORDER);
				MEMSZERO(k2);
				k2.OpID = cfg.OrderOpID;
				k2.Dt = Filt.Period.low;
				for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
					THROW(PPCheckUserBreak());
					p_bt->copyBufTo(&bill_rec);
					MpDBlk.OrdList.add(bill_rec.ID);
				}
			}
			MpDBlk.ShipmList.sortAndUndup();
			MpDBlk.RetList.sortAndUndup(); // @v12.1.11
			MpDBlk.OrdList.sortAndUndup();
			for(uint slidx = 0; slidx < MpDBlk.ShipmList.getCount(); slidx++) {
				const PPID bill_id = MpDBlk.ShipmList.get(slidx);
				PPBillPacket bpack;
				if(P_BObj->ExtractPacket(bill_id, &bpack) > 0) {
					bpack.GetOrderList(local_ord_list);
					wh_list.add(bpack.Rec.LocID);
					{
						long   subst_bill_val = 0;
						GoaAddingBlock blk;
						double part = 1.0;
						int    sign = 0;
						InitAddingBlock(&bpack, part, sign, &blk);
						if(!!Filt.Sgb)
							P_BObj->Subst(&bpack, &subst_bill_val, &Bsp);
						for(uint tiidx = 0; tiidx < bpack.GetTCount(); tiidx++) {
							const  PPTransferItem & r_ti = bpack.ConstTI(tiidx);
							int    r;
							THROW(PPCheckUserBreak());
							goods_list.add(labs(r_ti.GoodsID));
							THROW(r = PreprocessTi(&r_ti, p_suppl_bill_list, subst_bill_val, &blk));
							if(r > 0) {
								const double ti_qtty = fabs(r_ti.Quantity_);
								PROFILE_S(THROW(AddItem(blk)), "AddItem()"); // ogMarketplaceSalesAnalyze
								{
									IndicatorVector * p_iv = GetIndicatorEntry(blk);
									if(!p_iv) {
										IndicatorVector * p_new_iv = IndicatorList.CreateNewItem();
										p_new_iv->GoodsID = blk.FinalGoodsID;
										p_new_iv->ArID = blk.ArID;
										p_new_iv->LocID = blk.LocID;
										p_new_iv->Sign = blk.Sign;
										p_iv = p_new_iv;
									}
									const double ti_part = r_ti.CalcAmount() / bpack.Rec.Amount;
									p_iv->Add(PPBZSI_SALECOUNT, 1.0);
									p_iv->Add(PPBZSI_SALEQTTY, ti_qtty);
									p_iv->Add(PPBZSI_SALECOST, ti_qtty * r_ti.Cost);
									{
										const double a = bpack.Amounts.Get(PPAMT_FREIGHT, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_FREIGHT, a * ti_part);
									}
									{
										const double a = bpack.Amounts.Get(PPAMT_MP_SELLERPART, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPSELLERSPART, a * ti_part);
									}
									{
										const double a = bpack.Amounts.Get(PPAMT_MP_COMMISSION, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPCOMMISSION, a * ti_part);
									}
									{
										const double a = bpack.Amounts.Get(PPAMT_MP_ACQUIRING, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPACQUIRING, a * ti_part);
									}
									{
										double sp = 0.0;
										if(bpack.LTagL.GetReal(PPTAG_LOT_MP_SELLERSPRICE, tiidx, &sp)) {
											;
										}
										else
											sp = r_ti.Price;
										p_iv->Add(PPBZSI_MPAMT_SHIPMPRICE, ti_qtty * r_ti.NetPrice());
										p_iv->Add(PPBZSI_MPAMT_SHIPMSELLERPRICE, ti_qtty * sp);
									}
									if(local_ord_list.getCount()) {
										if(r_ti.Flags & PPTFR_ONORDER && r_ti.OrdLotID && !seen_ord_lot_list.lsearch(r_ti.OrdLotID)) {
											uint sh_lot_pos = 0;
											if(bpack.SearchShLot(r_ti.OrdLotID, &sh_lot_pos)) {
												ReceiptTbl::Rec lot_rec;
												if(P_BObj->trfr->Rcpt.Search(r_ti.OrdLotID, &lot_rec) > 0) {
													PPBillPacket ord_bpack;
													if(P_BObj->ExtractPacket(lot_rec.BillID, &ord_bpack) > 0) {
														wh_list.add(ord_bpack.Rec.LocID);
														p_iv->Add(PPBZSI_ORDCOUNT, 1.0);
														p_iv->Add(PPBZSI_ORDQTTY, fabs(lot_rec.Quantity));
														//
														uint ord_rowidx = 0;
														if(ord_bpack.SearchLot(r_ti.OrdLotID, &ord_rowidx)) {
															const PPTransferItem & r_ord_ti = ord_bpack.ConstTI(ord_rowidx);
															p_iv->Add(PPBZSI_MPAMT_ORDPRICE, fabs(lot_rec.Quantity) * r_ord_ti.NetPrice());
															p_iv->Add(PPBZSI_MPAMT_ORDSELLERPRICE, fabs(lot_rec.Quantity) * r_ord_ti.Price);
														}
														//
														long ord_shipm_delay = diffdate(bpack.Rec.Dt, lot_rec.Dt);
														p_iv->Add(PPBZSI_ORDSHIPMDELAYDAYS, static_cast<double>(ord_shipm_delay));
														seen_ord_lot_list.add(lot_rec.ID);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			//
			// Цикл по возвратам от покупателей //
			//
			for(uint rlidx = 0; rlidx < MpDBlk.RetList.getCount(); rlidx++) {
				const PPID bill_id = MpDBlk.RetList.get(rlidx);
				PPBillPacket bpack;
				if(P_BObj->ExtractPacket(bill_id, &bpack) > 0) {
					long   subst_bill_val = 0;
					GoaAddingBlock blk;
					double part = 1.0;
					int    sign = 0;
					wh_list.add(bpack.Rec.LocID);
					InitAddingBlock(&bpack, part, sign, &blk);
					if(!!Filt.Sgb)
						P_BObj->Subst(&bpack, &subst_bill_val, &Bsp);
					for(uint tiidx = 0; tiidx < bpack.GetTCount(); tiidx++) {
						const  PPTransferItem & r_ti = bpack.ConstTI(tiidx);
						int    r;
						goods_list.add(labs(r_ti.GoodsID));
						{
							PPTransferItem _ret_ti(r_ti);
							_ret_ti.Quantity_ = 0.0;
							_ret_ti.Cost = 0.0;
							_ret_ti.Price = 0.0;
							_ret_ti.Discount = 0.0;
							THROW(PPCheckUserBreak());
							THROW(r = PreprocessTi(&_ret_ti, p_suppl_bill_list, subst_bill_val, &blk)); // blk.Sign вероятно требует уточнения //
							if(r > 0) {
								const double ti_qtty = fabs(r_ti.Quantity_);
								PROFILE_S(THROW(AddItem(blk)), "AddItem()"); // ogMarketplaceSalesAnalyze
								{
									IndicatorVector * p_iv = GetIndicatorEntry(blk);
									if(!p_iv) {
										IndicatorVector * p_new_iv = IndicatorList.CreateNewItem();
										p_new_iv->GoodsID = blk.FinalGoodsID;
										p_new_iv->ArID = blk.ArID;
										p_new_iv->LocID = blk.LocID;
										p_new_iv->Sign = blk.Sign;
										p_iv = p_new_iv;
									}
									const double ti_part = r_ti.CalcAmount() / bpack.Rec.Amount;
									p_iv->Add(PPBZSI_SALERETCOUNT, 1.0);
									p_iv->Add(PPBZSI_SALERETQTTY, ti_qtty);
									p_iv->Add(PPBZSI_SALERETCOST, ti_qtty * r_ti.Cost);
									{
										const double a = bpack.Amounts.Get(PPAMT_FREIGHT, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_FREIGHT, a * ti_part);
									}
									{
										const double a = -fabs(bpack.Amounts.Get(PPAMT_MP_SELLERPART, 0L));
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPSELLERSPART, a * ti_part);
									}
									{
										const double a = bpack.Amounts.Get(PPAMT_MP_COMMISSION, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPCOMMISSION, a * ti_part);
									}
									{
										const double a = bpack.Amounts.Get(PPAMT_MP_ACQUIRING, 0L);
										if(a != 0.0)
											p_iv->Add(PPBZSI_MPACQUIRING, a * ti_part);
									}
								}
							}
						}
					}
				}
			}
			//
			// Цикл по заказам (которые не были обработаны в цикле обзора продаж)
			//
			for(uint olidx = 0; olidx < MpDBlk.OrdList.getCount(); olidx++) {
				const PPID bill_id = MpDBlk.OrdList.get(olidx);
				PPBillPacket bpack;
				if(P_BObj->ExtractPacket(bill_id, &bpack) > 0) {
					long   subst_bill_val = 0;
					GoaAddingBlock blk;
					double part = 1.0;
					int    sign = 0;
					wh_list.add(bpack.Rec.LocID);
					InitAddingBlock(&bpack, part, sign, &blk);
					if(!!Filt.Sgb)
						P_BObj->Subst(&bpack, &subst_bill_val, &Bsp);
					for(uint tiidx = 0; tiidx < bpack.GetTCount(); tiidx++) {
						const  PPTransferItem & r_ti = bpack.ConstTI(tiidx);
						if(!seen_ord_lot_list.lsearch(r_ti.LotID)) {
							int    r;
							goods_list.add(labs(r_ti.GoodsID));
							PPTransferItem _ord_ti(r_ti);
							_ord_ti.Quantity_ = 0.0;
							_ord_ti.Cost = 0.0;
							_ord_ti.Price = 0.0;
							_ord_ti.Discount = 0.0;
							THROW(PPCheckUserBreak());
							THROW(r = PreprocessTi(&_ord_ti, p_suppl_bill_list, subst_bill_val, &blk));
							if(r > 0) {
								PROFILE_S(THROW(AddItem(blk)), "AddItem()"); // ogMarketplaceSalesAnalyze
								{
									IndicatorVector * p_iv = GetIndicatorEntry(blk);
									if(!p_iv) {
										IndicatorVector * p_new_iv = IndicatorList.CreateNewItem();
										p_new_iv->GoodsID = blk.FinalGoodsID;
										p_new_iv->ArID = blk.ArID;
										p_new_iv->LocID = blk.LocID;
										p_new_iv->Sign = blk.Sign;
										p_iv = p_new_iv;
									}
									const double ti_part = r_ti.CalcAmount() / bpack.Rec.Amount;
									p_iv->Add(PPBZSI_ORDCOUNT, 1.0);
									p_iv->Add(PPBZSI_ORDQTTY, fabs(r_ti.Quantity_));
									p_iv->Add(PPBZSI_MPAMT_ORDPRICE, fabs(fabs(r_ti.Quantity_)) * r_ti.NetPrice());
									p_iv->Add(PPBZSI_MPAMT_ORDSELLERPRICE, fabs(fabs(r_ti.Quantity_)) * r_ti.NetPrice());
									if(bpack.Rec.Flags2 & BILLF2_DECLINED) {
										p_iv->Add(PPBZSI_ORDCANCELLEDCOUNT, 1.0);
										p_iv->Add(PPBZSI_ORDCANCELLEDQTTY,  fabs(r_ti.Quantity_));
										MpDBlk.CancelledOrdList.add(bpack.Rec.ID);
									}
								}
							}
						}
					}
				}
			}
			if(goods_list.getCount()) {
				if(mp_acc_id && mp_acs_id) {
					goods_list.sortAndUndup();
					assert(wh_list.getCount());
					wh_list.sortAndUndup();
					DateRange period(Filt.Period);
					double total_storage = 0.0; // Общие расходы на хранение (без разбивки, ибо wildberries не дает таковой)
					double total_acceptance = 0.0; // Общие расходы на приемку товаров на складах маркетплейса (без разбивки, ибо wildberries не дает таковой)
					double total_deduction = 0.0; // Общие удержания со счета - чаще всего на маркетинг (без разбивки, ибо wildberries не дает таковой)
					bool   is_there_any_amount = false;
					struct ArToVarAssoc {
						PPID   ArNo;
						double * P_Var;
					};
					const ArToVarAssoc ar_to_var_assoc[] = {
						{ ARTN_MRKTPLCACC_STORAGE, &total_storage },
						{ ARTN_MRKTPLCACC_ACCEPTANCE, &total_acceptance },
						{ ARTN_MRKTPLCACC_DEDUCTION, &total_deduction }
					};
					{
						ArticleTbl::Rec ar_rec;
						for(uint avaidx = 0; avaidx < SIZEOFARRAY(ar_to_var_assoc); avaidx++) {
							const int ar_no = ar_to_var_assoc[avaidx].ArNo;
							double * p_var = ar_to_var_assoc[avaidx].P_Var;
							PPViewAccAnlz aa_view;
							AccAnlzFilt aa_filt;
							AccAnlzTotal aa_total;
							aa_filt.Period = Filt.Period;
							aa_filt.AcctId.ac = mp_acc_id;
							if(P_BObj->ArObj.P_Tbl->SearchNum(mp_acs_id, ar_no, &ar_rec) > 0) {
								aa_filt.AcctId.ar = ar_rec.ID;
								aa_filt.Aco = ACO_3;
								if(P_BObj->atobj->P_Tbl->AcctIDToRel(&aa_filt.AcctId, &aa_filt.AccID)) {
									aa_filt.Flags |= AccAnlzFilt::fTotalOnly;
									aa_filt.Flags &= ~AccAnlzFilt::fTrnovrBySheet;
									if(aa_view.Init_(&aa_filt)) {
										aa_view.GetTotal(&aa_total);
										double dbt = 0.0;
										double crd = 0.0;
										aa_total.DbtTrnovr.Get(0L, 0L, &dbt);
										aa_total.CrdTrnovr.Get(0L, 0L, &crd);
										*p_var = crd - dbt;
										if(*p_var != 0.0) {
											is_there_any_amount = true;
										}
									}
								}
							}								
						}
					}
					if(is_there_any_amount) {
						bool avg_rest_is_ok = false;
						RAssocArray avg_rest_result;
						if(total_storage > 0.0 || total_acceptance > 0.0) {
							if(P_BObj->trfr->EvaluateAverageRestByGoods(wh_list, goods_list, period, avg_rest_result) > 0) {
								avg_rest_is_ok = true;
							}
						}
						//
						const double avg_rest_total = avg_rest_result.GetTotal();
						double total_shipm_qtty = 0.0; // Общее заказанное количество 
						double total_ord_qtty = 0.0;   // Общее отгруженное количество 
						{
							for(uint i = 0; i < IndicatorList.getCount(); i++) {
								const IndicatorVector * p_iv = IndicatorList.at(i);
								if(p_iv) {
									double v = 0.0;
									if(p_iv->Get(PPBZSI_ORDQTTY, &v)) 
										total_ord_qtty += v;
									if(p_iv->Get(PPBZSI_SALEQTTY, &v)) 
										total_shipm_qtty += v;
								}
							}
						}
						for(uint gidx = 0; gidx < goods_list.getCount(); gidx++) {
							const PPID goods_id = goods_list.get(gidx);
							double ord_qtty   = 0; // Заказанное количество по товару goods_id
							double shipm_qtty = 0; // Отгруженное количество по товару goods_id
							uint   item_count = 0; // Количество элементов в IndicatorList имеющих общий ид товара
							{
								for(uint i = 0; i < IndicatorList.getCount(); i++) {
									const IndicatorVector * p_iv = IndicatorList.at(i);
									if(p_iv && p_iv->GoodsID == goods_id) {
										item_count++;
										double v = 0.0;
										if(p_iv->Get(PPBZSI_ORDQTTY, &v)) 
											ord_qtty += v;
										if(p_iv->Get(PPBZSI_SALEQTTY, &v)) 
											shipm_qtty += v;
									}
								}
							}
							if(item_count) {
								const double avg_rest = avg_rest_result.Get(goods_id, 0);
								for(uint i = 0; i < IndicatorList.getCount(); i++) {
									IndicatorVector * p_iv = IndicatorList.at(i);
									if(p_iv && p_iv->GoodsID == goods_id) {
										double coeff = 0.0; // Коэффициент для расчета удельной доли стоимости хранения и приемки
										double coeff2 = 0.0; // Коэффициент для расчета удельной доли прочих расходов (реклама, вестимо)
										double v = 0.0;
										if(avg_rest_total != 0.0) {
											const double avg_rest_part = avg_rest / avg_rest_total;
											{
												// Здесь приоритет у отгрузок перед заказами
												if(item_count == 1) { 
													coeff = avg_rest_part; // Единственный элемент для товара goods_id: коэффициент просто равен доле среднего остатка
												}
												else if(shipm_qtty > 0.0) {
													p_iv->Get(PPBZSI_SALEQTTY, &v);
													coeff = (v / shipm_qtty) * avg_rest_part;
												}
												else if(ord_qtty > 0.0) {
													p_iv->Get(PPBZSI_ORDQTTY, &v);
													coeff = (v / ord_qtty) * avg_rest_part;
												}
												else {
													coeff = (1.0 / static_cast<double>(item_count)) * avg_rest_part;
												}
											}
										}
										{
											// Здесь приоритет у заказов перед отгрузками
											if(total_ord_qtty > 0.0) {
												p_iv->Get(PPBZSI_ORDQTTY, &v);
												coeff2 = (v / total_ord_qtty);
											}
											else if(total_shipm_qtty > 0.0) {
												p_iv->Get(PPBZSI_SALEQTTY, &v);
												coeff2 = (v / total_shipm_qtty);
											}
											else {
												coeff2 = (1.0 / static_cast<double>(item_count));
											}
										}
										double storage_cost = coeff * total_storage;
										double acceptance_cost = coeff * total_acceptance;
										double deduction_cost = coeff2 * total_deduction;
										p_iv->Add(PPBZSI_MPSTORAGE, storage_cost);
										p_iv->Add(PPBZSI_MPACCEPTANCE, acceptance_cost);
										p_iv->Add(PPBZSI_MPPROMOTION, deduction_cost);
									}
								}
							}
						}
					}
				}
			}
		}
		THROW(FlashCacheItems(0));
		{
			TempGoodsOprTbl * p_t = P_TempTbl;
			TempGoodsOprTbl::Rec rec;
			BExtQuery q(p_t, 0, 64);
			{
				q.select(p_t->ID__, p_t->GoodsID, p_t->Object, p_t->LocID, p_t->InOutTag, 0L);
				TempGoodsOprTbl::Key0 k;
				MEMSZERO(k);
				for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
					p_t->copyBufTo(&rec);
					IndicatorVector * p_iv = GetIndicatorEntry(rec.GoodsID, rec.Object, rec.LocID, rec.InOutTag);
					if(p_iv) {
						p_iv->Ident = rec.ID__;
					}
				}
				IndicatorList.sort2(PTR_CMPFUNC(BzsValVector_Ident));
			}
		}
	}
	else if(Filt.BillList.IsExists() && !(Filt.Flags & GoodsOpAnalyzeFilt::fBillListAsTradePlan)) {
		for(uint i = 0; i < Filt.BillList.Get().getCount(); i++) {
			if(P_BObj->ExtractPacket(Filt.BillList.Get().at(i), &pack) > 0) {
				if(!use_ext_list || ext_bill_list.bsearch(pack.Rec.ID)) {
					const double part = (GetOpType(pack.Rec.OpID, &op_rec) == PPOPT_GOODSRETURN) ? -1.0 : 1.0;
					PPWaitMsg(wait_msg.Z().Cat(pack.Rec.Dt).Space().Cat(pack.Rec.Code));
					THROW(PPCheckUserBreak());
					THROW(PutBillToTempTable(&pack, part, 0, p_suppl_bill_list));
					pUfpFactors[0] += (1.0 + (double)pack.GetTCount());
				}
			}
		}
	}
	else {
		if(!Filt.OpID && Filt.IsLeadedInOutAnalyze()) {
			GetOpList(PPOPT_GOODSMODIF, &op_list);
		}
		else if(oneof2(Filt.OpGrpID, GoodsOpAnalyzeFilt::ogSelected, GoodsOpAnalyzeFilt::ogInOutAnalyze)) {
			int    are_all_reval = 1;
			if(IsGenericOp(Filt.OpID) > 0) {
				ObjRestrictArray or_list;
				op_obj.GetGenericList(Filt.OpID, &or_list);
				for(uint i = 0; i < or_list.getCount(); i++) {
					const ObjRestrictItem & r_or_item = or_list.at(i);
					const  PPID op_id = r_or_item.ObjID;
					op_list.add(op_id);
					if(r_or_item.Flags & GOIF_NEGATIVE)
						neg_op_list.add(op_id);
					if(are_all_reval && GetOpType(op_id) != PPOPT_GOODSREVAL)
						are_all_reval = 0;
				}
			}
			else {
				op_list.add(Filt.OpID);
				if(GetOpType(Filt.OpID) != PPOPT_GOODSREVAL)
					are_all_reval = 0;
			}
			if(are_all_reval) {
				if(!(Filt.Flags & (GoodsOpAnalyzeFilt::fCalcOrder | GoodsOpAnalyzeFilt::fShowSStatSales)))
					State |= sReval;
			}
		}
		else {
			for(id = 0; EnumOperations(0L, &id, &op_rec) > 0;) {
				const bool is_prft = (op_rec.Flags & OPKF_PROFITABLE && op_rec.OpTypeID != PPOPT_PAYMENT);
				if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogIncoming) {
					if(is_prft && op_rec.OpTypeID == PPOPT_GOODSEXPEND)
						THROW(op_list.add(id));
				}
				else if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogProfitable) {
					if(is_prft || (op_rec.OpTypeID == PPOPT_GOODSRETURN && CheckOpFlags(op_rec.LinkOpID, OPKF_PROFITABLE)))
						THROW(op_list.add(id));
				}
				else if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogPayed) {
					if(is_prft && !(op_rec.OpTypeID == PPOPT_GOODSRETURN)) {
						if(!(op_rec.Flags & OPKF_NEEDPAYMENT)) {
							THROW(op_list.add(id));
						}
						else {
							THROW(op_obj.GetPaymentOpList(id, &op_list));
						}
					}
				}
			}
		}
		{
			TSVector <__GoaBillEntry> bill_entry_list;
			if(Filt.IsLeadedInOutAnalyze()) {
				GCTFilt gct_filt;
				gct_filt.GoodsList = Filt.GoodsIdList;
				gct_filt.LocList = LocList_;
				gct_filt.Period = Filt.Period;
				gct_filt.Flags |= OPG_FORCEBILLCACHE;
				if(op_list.getCount() == 1) {
					gct_filt.OpID = op_list.get(0);
				}
				GCTIterator gctiter(&gct_filt, &gct_filt.Period);
				TransferTbl::Rec trfr_rec; // Используется при вызове методов GCTIterator
				BillTbl::Rec bill_rec;     // Используется при вызове методов GCTIterator
				if(gctiter.First(&trfr_rec, &bill_rec) > 0) {
					PPIDArray local_bill_list;
					do {
						int is_suite = 0;
						if(op_list.getCount()) {
							if(op_list.lsearch(bill_rec.OpID))
								is_suite = 1;
						}
						else {
							if(GetOpType(bill_rec.OpID) == PPOPT_GOODSMODIF)
								is_suite = 1;
						}
						if(is_suite && CheckBillRec(&bill_rec)) {
							local_bill_list.add(bill_rec.ID);
						}
					} while(gctiter.Next(&trfr_rec, &bill_rec) > 0);
					if(local_bill_list.getCount()) {
						local_bill_list.sortAndUndup();
						for(uint i = 0; i < local_bill_list.getCount(); i++) {
							__GoaBillEntry entry;
							entry.BillID = local_bill_list.get(i);
							entry.IsPaym = 0;
							entry.Payment = 0.0;
							entry.Part = 0.0;
							THROW_SL(bill_entry_list.insert(&entry));
						}
					}
				}
			}
			else {
				for(uint i = 0; i < op_list.getCount(); i++) {
					const PPID op_id = op_list.get(i);
					BillTbl::Rec bill_rec;
					int    is_paym = 0;
					double part    = 1.0;
					if(Filt.OpGrpID != GoodsOpAnalyzeFilt::ogSelected) {
						if(GetOpType(op_id, &op_rec) == PPOPT_GOODSRETURN)
							part = -1.0;
						else if(op_rec.OpTypeID == PPOPT_PAYMENT)
							is_paym = 1;
					}
					BillTbl::Key2 k2;
					BillTbl * p_bt = P_BObj->P_Tbl;
					BExtQuery q(p_bt, 2);
					q.select(p_bt->ID, p_bt->Code, p_bt->Dt, p_bt->LocID, p_bt->OpID, p_bt->Object, p_bt->Object2, p_bt->Flags,
						p_bt->LinkBillID, p_bt->Amount, 0).where(p_bt->OpID == op_id && daterange(p_bt->Dt, &Filt.Period));
					MEMSZERO(k2);
					k2.OpID = op_id;
					k2.Dt = Filt.Period.low;
					for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
						THROW(PPCheckUserBreak());
						p_bt->copyBufTo(&bill_rec);
						if(!is_paym && use_ext_list && !ext_bill_list.bsearch(bill_rec.ID))
							continue;
						if(is_paym || CheckBillRec(&bill_rec)) {
							int    sign = 0, r;
							if(is_paym) {
								id = bill_rec.LinkBillID;
								payment = BR2(bill_rec.Amount);
								if(payment == 0)
									continue;
								if(use_ext_list && !ext_bill_list.bsearch(id))
									continue;
								THROW(r = P_BObj->Search(id, &pack.Rec));
								if(r > 0) {
									if(!CheckBillRec(&pack.Rec))
										continue;
								}
								else {
									PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
									continue;
								}
							}
							else
								id = bill_rec.ID;
							{
								__GoaBillEntry entry;
								entry.BillID = id;
								entry.IsPaym = is_paym;
								entry.Payment = payment;
								entry.Part = part;
								THROW_SL(bill_entry_list.insert(&entry));
							}
						}
					}
				}
			}
			{
				//
				// Основной цикл формирования отчета
				//
				bill_entry_list.sort(CMPF_LONG);
				for(uint i = 0; i < bill_entry_list.getCount(); i++) {
					THROW(PPCheckUserBreak());
					const __GoaBillEntry & r_entry = bill_entry_list.at(i);
					int    sign = 0;
					double part = r_entry.Part;
					THROW(P_BObj->ExtractPacket(r_entry.BillID, &pack) > 0);
					if(r_entry.IsPaym)
						part = fdivnz(r_entry.Payment, pack.GetAmount());
					if(oneof2(Filt.OpGrpID, GoodsOpAnalyzeFilt::ogInOutAnalyze, GoodsOpAnalyzeFilt::ogSelected))
						sign = neg_op_list.lsearch(pack.Rec.OpID) ? -1 : 1;
					THROW(PutBillToTempTable(&pack, part, sign, p_suppl_bill_list));
					pUfpFactors[0] += (1.0 + (double)pack.GetTCount());
					PPWaitPercent(i+1, bill_entry_list.getCount(), PPObjBill::MakeCodeString(&pack.Rec, 1, wait_msg));
				}
			}
		}
	}
	THROW(FlashCacheItems(0));
	if(Filt.Flags & GoodsOpAnalyzeFilt::fUnprofitableGoods) {
		PPLoadText(PPTXT_CALCUNPROFITABLEGOODS, wait_msg);
		PPWaitMsg(wait_msg);
		THROW_DB(deleteFrom(P_TempTbl, 0, (P_TempTbl->SumPrice >= P_TempTbl->SumCost)));
	}
	if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) {
		//
		// Формирование таблицы позиций, которые плохо продаются.
		//
		if(Filt.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods) {
			PPID   prev_goods_id = 0;
			PPID   prev_loc_id = 0;
			PPViewGoodsRest gr_view;
			GoodsRestViewItem gr_item;
			CALLPTRMEMB(P_Cache, freeAll());
			p_prev_temp_tbl = P_TempTbl;
			THROW(P_TempTbl = CreateTempFile());
			THROW(InitGoodsRestView(&gr_view));
			PPLoadText(PPTXT_CALCBADSALESGOODS, wait_msg);
			for(gr_view.InitIteration(); gr_view.NextIteration(&gr_item) > 0;) {
				TempGoodsOprTbl::Key3 k3;
				MEMSZERO(k3);
				k3.GoodsID = gr_item.GoodsID;
				if(!p_prev_temp_tbl->search(3, &k3, spGe) || k3.GoodsID != gr_item.GoodsID) {
					GoaAddingBlock blk;
					THROW(GObj.Fetch(gr_item.GoodsID, &blk.GoodsRec));
					blk.FinalGoodsID = gr_item.GoodsID;
					blk.ArID = Filt.SupplID;
					blk.OpID = Filt.OpID;
					blk.LocID        = gr_item.LocID;
					blk.ParentGrpID  = gr_item.GoodsGrpID;
					blk.UnitPerPack  = gr_item.UnitPerPack;
					blk.Cost = gr_item.Cost;
					blk.Price        = gr_item.Price;
					if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder)
						blk.OldCost = gr_item.Order;
					if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales)
						blk.OldPrice = gr_item.SStatSales;
					THROW(AddItem(blk));
				}
				pUfpFactors[0] += 1.5;
				PPWaitPercent(gr_view.GetCounter(), wait_msg);
			}
			THROW(FlashCacheItems(0));
		}
		//
		// Формирование значений товарных остатков
		//
		{
			PPID   prev_goods_id = 0;
			PPID   prev_loc_id = 0;
			TempGoodsOprTbl::Key3 k3;
			IterCounter c;
			PPViewGoodsRest gr_view;
			LAssocArray goods_list; // Список пар {товар, склад}, присутсвующих в текущем отчете. Необходим
				// для идентификации товаров, по которым есть остаток, но которые в то же время отсутствуют в текущем отчете.
			THROW(InitGoodsRestView(&gr_view));
			MEMSZERO(k3);
			PPInitIterCounter(c, P_TempTbl);
			PPLoadText(PPTXT_CALCREST, wait_msg);
			if(P_TempTbl->searchForUpdate(3, &k3, spFirst)) do {
				TempGoodsOprTbl::Rec rec;
				P_TempTbl->copyBufTo(&rec);
				if(rec.GoodsID != prev_goods_id || rec.LocID != prev_loc_id) {
					goods_list.Add(rec.GoodsID, rec.LocID, 0);
					int    do_update = 0;
					GoodsRestViewItem gr_item;
					prev_goods_id = rec.GoodsID;
					prev_loc_id   = rec.LocID;
					int    r = 0;
					if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
						r = gr_view.GetItem(rec.GoodsID, rec.LocID, &gr_item);
					else if(LocList_.GetCount() == 1)
						r = gr_view.GetItem(rec.GoodsID, &LocList_, &gr_item);
					else
						r = gr_view.GetItem(rec.GoodsID, 0L, &gr_item);
					if(r > 0) {
						P_TempTbl->data.Rest = gr_item.Rest;
						P_TempTbl->data.PhRest = gr_item.PhRest;
						P_TempTbl->data.RestCostSum  = gr_item.SumCost;
						P_TempTbl->data.RestPriceSum = gr_item.SumPrice;
						if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder)
							P_TempTbl->data.OldCost = gr_item.Order;
						if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales)
							P_TempTbl->data.OldPrice = gr_item.SStatSales;
						do_update = 1;
					}
					else {
						if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales) {
							ObjIdListFilt loc_list;
							if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
								loc_list.Add(rec.LocID);
							else
								loc_list = LocList_;
							PredictSalesStat stat;
							if(gr_view.GetGoodsStat(rec.GoodsID, loc_list, &stat) > 0) {
								P_TempTbl->data.OldPrice = stat.GetAverage(PSSV_QTTY);
								do_update = 1;
							}
							pUfpFactors[0] += 1.5;
						}
						if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder) {
							GoodsRestParam ord_p;
							if(gr_view.CalcOrder(rec.GoodsID, &ord_p) > 0) {
								P_TempTbl->data.OldCost = ord_p.Total.Rest;
								do_update = 1;
							}
							pUfpFactors[0] += 1.3;
						}
					}
					if(do_update)
						THROW_DB(P_TempTbl->updateRec()); // @sfu
				}
				PPWaitPercent(c.Increment(), wait_msg);
			} while(P_TempTbl->searchForUpdate(3, &k3, spNext));
			if(!(Filt.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods) && (Filt.Flags & GoodsOpAnalyzeFilt::fAddNzRestItems)) {
				GoodsRestViewItem gr_item;
				CALLPTRMEMB(P_Cache, freeAll());
				for(gr_view.InitIteration(); gr_view.NextIteration(&gr_item) > 0;) {
					if(!goods_list.SearchPair(gr_item.GoodsID, gr_item.LocID, 0)) {
						GoaAddingBlock blk;
						THROW(GObj.Fetch(gr_item.GoodsID, &blk.GoodsRec));
						blk.FinalGoodsID = gr_item.GoodsID;
						blk.ArID = Filt.SupplID;
						blk.OpID = Filt.OpID;
						blk.LocID        = gr_item.LocID;
						blk.ParentGrpID  = gr_item.GoodsGrpID;
						blk.UnitPerPack  = gr_item.UnitPerPack;
						blk.Rest = gr_item.Rest;
						blk.PhRest       = gr_item.PhRest;
						blk.RestCostSum  = gr_item.SumCost;
						blk.RestPriceSum = gr_item.SumPrice;
						blk.Cost = gr_item.Cost;
						blk.Price        = gr_item.Price;
						if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder)
							blk.OldCost = gr_item.Order;
						if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales)
							blk.OldPrice = gr_item.SStatSales;
						THROW(AddItem(blk));
						pUfpFactors[0] += 0.11;
					}
					PPWaitPercent(gr_view.GetCounter(), wait_msg);
				}
				THROW(FlashCacheItems(0));
			}
		}
	}
	//
	// Формирование ABC-анализа
	//
	if((Filt.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz) && !(Filt.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods)) {
		short  abc_grp = 0;
		long   recs_count = 0;
		double total_sum = 0.0;
		GoodsOpAnalyzeFilt old_filt = Filt;
		TempGoodsOprTbl::Rec rec;
		GoodsOpAnalyzeViewItem item;
		ABCGrpStorageList abc_grp_stores(&GObj);

		Filt.Flags &= ~GoodsOpAnalyzeFilt::fUseABCAnlz;
		Filt.ABCAnlzGroup = -1;
		IterOrder ord = GetIterOrder(); // в данном случае вызывать только после выражения Filt.ABCAnlzGroup = -1;
		PPLoadText(PPTXT_ABCANLZPROCESSING, wait_msg);
		PPWaitStart();
		THROW(abc_grp_stores.InitGoodsGrpList(&Filt));
		for(InitIteration(ord); NextIteration(&item) > 0;) {
			THROW(abc_grp_stores.IncTotalItem(Filt.ABCAnlz.GroupBy, &item));
		}
		THROW(abc_grp_stores.Init(&Filt.ABCAnlz));
		for(InitIteration(ord); NextIteration(&item) > 0;) {
			THROW(abc_grp_stores.CalcBelongToABCGrp(&item, &abc_grp));
			if(abc_grp) {
				THROW_DB(updateFor(P_TempTbl, 0, (P_TempTbl->ID__ == item.ID__),
					set(P_TempTbl->InOutTag, dbconst((long)-abc_grp))));
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, (P_TempTbl->ID__ == item.ID__)));
			}
			pUfpFactors[0] += 0.09;
			PPWaitPercent(GetCounter(), wait_msg);
		}
		THROW(abc_grp_stores.CalcBelongToABCGrp(0, 0, 1));
		for(abc_grp = 1; abc_grp_stores.EnumItems(&abc_grp, &rec) > 0;) {
			if(rec.InOutTag != 0) { // ABC group != 0
				THROW_DB(P_TempTbl->insertRecBuf(&rec));
			}
		}
		Filt = old_filt;
	}
	//
	// Формирование сравнительных значений (сравнительный период Filt.CmpPeriod)
	//
	if(!(Filt.Flags & GoodsOpAnalyzeFilt::fCrosstab) && Filt.CmpPeriod.IsZero() == 0) {
		uint count = 0;
		uint i = P_Uniq->getCount();
		GoodsOpAnalyzeFilt filt;
		filt = Filt;
		filt.Period = filt.CmpPeriod;
		filt.RestCalcDate = filt.CmpRestCalcDate;
		filt.ZeroCompareItems();
		THROW_MEM(P_CmpView = new PPViewGoodsOpAnalyze);
		THROW(P_CmpView->InitUniq(P_Uniq));
		THROW(P_CmpView->Init_(&filt));
		P_CmpView->CopyUniq(P_Uniq);
		count = P_Uniq->getCount();
		CALLPTRMEMB(P_Cache, freeAll());
		for(; i < count; i++) {
			TempGoodsOprTbl::Key0 k0;
			const GoaUniqItem * p_id = static_cast<const GoaUniqItem *>(P_Uniq->at(i));
			MEMSZERO(k0);
			k0.ID__ = p_id->Id;
			if(P_TempTbl->search(0, &k0, spEq) <= 0) {
				GoaAddingBlock blk;
				THROW(GObj.Fetch(p_id->GoodsID, &blk.GoodsRec));
				blk.TiSign       = p_id->Sign;
				blk.FinalGoodsID = p_id->GoodsID;
				blk.ArID = p_id->ArID;
				blk.OpID = Filt.OpID;
				blk.LocID        = p_id->LocID;
				blk.ParentGrpID  = blk.GoodsRec.ParentID;
				THROW(AddItem(blk));
			}
		}
		THROW(FlashCacheItems(0));
	}
	Crosstab * p_prev_ct = P_Ct;
	P_Ct = 0;
	if(Filt.Flags & GoodsOpAnalyzeFilt::fCrosstab) {
		int aggr_fld_add = 0;
		SString temp_buf, c_title;
		DBFieldList total_row_list, total_col_list;
		THROW_MEM(P_Ct = new GoodsOpAnlzCrosstab(this)); // Crosstab
		P_Ct->SetTable(P_TempTbl, P_TempTbl->LocID);
		P_Ct->AddIdxField(P_TempTbl->GoodsID);
		P_Ct->SetSortIdx("Text", 0);
		P_Ct->AddInheritedFixField(P_TempTbl->Text);
		if(Filt.Sgg == sggNone && !Filt.Sgb)
			P_Ct->AddInheritedFixField(P_TempTbl->Barcode);
		for(uint i = 0; i < Filt.CompareItems.getCount(); i++) {
			long   id = Filt.CompareItems.at(i).ObjID;
			long   flags = Filt.CompareItems.at(i).Flags;
			c_title = 0;
			if(id == GoodsOpAnalyzeFilt::fldidQtty) {
				PPLoadString("qtty", c_title);
				P_Ct->AddAggrField(P_TempTbl->Quantity, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->Quantity);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidRest) {
				PPLoadString("rest", c_title);
				P_Ct->AddAggrField(P_TempTbl->Rest, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->Rest);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidCostRest) {
				PPGetWord(PPWORD_REST_IN, 0, c_title);
				PPGetWord(PPWORD_COST,    0, temp_buf);
				c_title.Space().Cat(temp_buf);
				P_Ct->AddAggrField(P_TempTbl->RestCostSum, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->RestCostSum);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidPriceRest) {
				PPGetWord(PPWORD_REST_IN, 0, c_title);
				PPGetWord(PPWORD_PRICE_P, 0, temp_buf);
				c_title.Space().Cat(temp_buf);
				P_Ct->AddAggrField(P_TempTbl->RestPriceSum, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->RestPriceSum);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidCostAmount) {
				PPGetWord(PPWORD_SUM_IN, 0, c_title);
				PPGetWord(PPWORD_COST,   0, temp_buf);
				c_title.Space().Cat(temp_buf);
				P_Ct->AddAggrField(P_TempTbl->SumCost, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->SumCost);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidPriceAmount) {
				PPGetWord(PPWORD_SUM_IN,  0, c_title);
				PPGetWord(PPWORD_PRICE_P, 0, temp_buf);
				c_title.Space().Cat(temp_buf);
				P_Ct->AddAggrField(P_TempTbl->SumPrice, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->SumPrice);
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidIncome) {
				PPLoadString("income", c_title);
				P_Ct->AddAggrField(P_TempTbl->Income, Crosstab::afSum, c_title);
				total_row_list.Add(P_TempTbl->Income);
				aggr_fld_add = 1;
			}
			/*
			else if(id == GoodsOpAnalyzeFilt::fldidPctProfitable) {
				PPLoadString("income", c_title);
				c_title.Space().CatChar('%');
				c  = 29;
				aggr_fld_add = 1;
			}
			else if(id == GoodsOpAnalyzeFilt::fldidPctMargin) {
				PPGetWord(PPWORD_MARGIN, 0, c_title);
				c_title.Space().CatChar('%');
				PPDbqFuncPool::InitPctFunc(dbe_pct_margin1, tbl->Income, tbl->SumPrice, 0);
				c  = 30;
				aggr_fld_add = 1;
			}
			*/
		}
		if(aggr_fld_add == 0) {
			P_Ct->AddAggrField(P_TempTbl->Quantity);
			total_row_list.Add(P_TempTbl->Quantity);
		}
		PPGetWord(PPWORD_TOTAL, 0, temp_buf.Z());
		P_Ct->AddTotalRow(total_row_list, 0, temp_buf);
		// P_Ct->AddTotalColumn(total_row_list, 0, temp_buf);
		THROW(P_Ct->Create(1));
		pUfpFactors[0] *= 1.5;
		ok = 1;
	}
	ZDELETE(p_prev_ct);
	CATCH
		ZDELETE(P_Ct);
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	ZDELETE(p_prev_temp_tbl);
	ZDELETE(P_Cache);
	if(!_CrtCheckMemory())
		ok = 0;
	return ok;
}

GoaCacheItem * FASTCALL PPViewGoodsOpAnalyze::GetCacheItem(uint pos) const { return static_cast<GoaCacheItem *>(P_Cache->at(pos)); }

int PPViewGoodsOpAnalyze::FlashCacheItems(uint count)
{
	int    ok = 1;
	if(!P_TempTbl)
		ok = -1;
	else {
		uint   i;
		BExtInsert bei(P_TempTbl);
		if(count) {
			PPIDArray lru_pos_array;
			LAssocArray lru_array;
			for(i = 0; i < P_Cache->getCount(); i++) {
				THROW_SL(lru_array.Add(GetCacheItem(i)->Counter, i, 0));
			}
			lru_array.Sort();
			for(i = 0, count = MIN(count, lru_array.getCount()); i < count; i++) {
				const uint pos = lru_array.at(i).Val;
				THROW(FlashCacheItem(&bei, GetCacheItem(pos)));
				THROW(lru_pos_array.add(pos));
			}
			lru_pos_array.sort();
			for(int rev_i = lru_pos_array.getCount() - 1; rev_i >= 0; rev_i--)
				P_Cache->atFree((uint)lru_pos_array.at(rev_i));
		}
		else {
			for(i = 0; i < P_Cache->getCount(); i++)
				THROW(FlashCacheItem(&bei, GetCacheItem(i)));
			P_Cache->clear();
		}
		THROW(bei.flash());
	}
	CATCHZOK
	return ok;
}

int PPViewGoodsOpAnalyze::FlashCacheItem(BExtInsert * pBei, const GoaCacheItem * pItem)
{
	int    ok = 1;
	long   id = 1;
	if(!P_TempTbl)
		ok = -1;
	else if(pItem->DbPos) {
		TempGoodsOprTbl::Rec & r_rec = P_TempTbl->data;
		THROW_DB(P_TempTbl->getDirect(-1, 0, pItem->DbPos));
		r_rec.Cost     = pItem->Cost;
		r_rec.Price    = pItem->Price;
		r_rec.OldCost  = pItem->OldCost;
		r_rec.OldPrice = pItem->OldPrice;
		r_rec.SumCost  = pItem->SumCost;
		r_rec.SumPrice = pItem->SumPrice;
		r_rec.Quantity = pItem->Qtty;
		r_rec.PhQtty   = pItem->PhQtty;
		r_rec.Income   = pItem->Income;
		r_rec.Rest     = pItem->Rest;
		r_rec.PhRest   = pItem->PhRest;
		r_rec.RestCostSum  = pItem->RestCostSum;
		r_rec.RestPriceSum = pItem->RestPriceSum;
		THROW_DB(P_TempTbl->updateRec());
	}
	else {
		SString temp_buf;
		TempGoodsOprTbl::Rec rec;
		rec.InOutTag = pItem->Sign;
		rec.GoodsID  = pItem->GoodsID;
		rec.Object   = pItem->ArID;
		rec.LocID    = pItem->LocID;
		if(!!Filt.Sgb) {
			P_BObj->GetSubstText(pItem->GoodsID, &Bsp, temp_buf);
			temp_buf.CopyTo(rec.Text, sizeof(rec.Text));
		}
		else {
			Goods2Tbl::Rec goods_rec;
			GObj.GetSubstText(pItem->GoodsID, Filt.Sgg, &Gsl, temp_buf);
			STRNSCPY(rec.Text, temp_buf);
			if(!(pItem->GoodsID & GOODSSUBSTMASK) && pItem->GoodsID) {
				if(State & sFiltExclFolder) {
					PPID   parent_id = 0;
					if(GObj.P_Tbl->GetExclusiveAltParent(pItem->GoodsID, Filt.GoodsGrpID, &parent_id) > 0)
						rec.GoodsGrp = parent_id;
				}
				else if(GObj.Fetch(pItem->GoodsID, &goods_rec) > 0)
					rec.GoodsGrp = goods_rec.ParentID;
			}
		}
		//
		// Вычисление уникального __ID для таблицы, если строится отчет со-сравнением
		//
		if(P_Uniq) {
			uint pos = 0;
			GoaUniqItem _id;
			MEMSZERO(_id);
			_id.Sign    = pItem->Sign;
			_id.GoodsID = pItem->GoodsID;
			_id.ArID    = pItem->ArID;
			_id.LocID   = pItem->LocID;
			_id.Cost    = pItem->Cost;
			_id.Price   = pItem->Price;
			if(P_Uniq->lsearch(&_id, &pos, Cf_UniqItem))
				id = static_cast<const GoaUniqItem *>(P_Uniq->at(pos))->Id;
			else {
				if(P_Uniq->getCount())
					id = static_cast<const GoaUniqItem *>(P_Uniq->at(P_Uniq->getCount()-1))->Id + 1;
				_id.Id = id;
				THROW_SL(P_Uniq->insert(&_id));
			}
			rec.ID__ = id;
		}
		rec.UnitPerPack = pItem->UnitPerPack;
		rec.Cost     = pItem->Cost;
		rec.Price    = pItem->Price;
		rec.OldCost  = pItem->OldCost;
		rec.OldPrice = pItem->OldPrice;
		rec.SumCost  = pItem->SumCost;
		rec.SumPrice = pItem->SumPrice;
		rec.Quantity = pItem->Qtty;
		rec.PhQtty   = pItem->PhQtty;
		rec.Income   = pItem->Income;
		rec.Rest     = pItem->Rest;
		rec.PhRest   = pItem->PhRest;
		rec.RestCostSum  = pItem->RestCostSum;
		rec.RestPriceSum = pItem->RestPriceSum;
		if(Filt.Sgg == sggNone && !Filt.Sgb) {
			GObj.FetchSingleBarcode(rec.GoodsID, temp_buf.Z());
			temp_buf.CopyTo(rec.Barcode, sizeof(rec.Barcode));
		}
		THROW_DB(pBei->insert(&rec));
	}
	CATCHZOK
	return ok;
}

PPViewGoodsOpAnalyze::IndicatorVector::IndicatorVector() : BzsValVector(), Sign(0), GoodsID(0), ArID(0), LocID(0)
{
}

PPViewGoodsOpAnalyze::IndicatorVector * PPViewGoodsOpAnalyze::GetIndicatorEntry(PPID goodsID, PPID arID, PPID locID, int sign)
{
	IndicatorVector * p_result = 0;
	for(uint i = 0; !p_result && i < IndicatorList.getCount(); i++) {
		IndicatorVector * p_item = IndicatorList.at(i);
		if(p_item && p_item->GoodsID == goodsID && p_item->ArID == arID && p_item->LocID == locID && p_item->Sign == sign) {
			p_result = p_item;
		}
	}
	return p_result;
}

PPViewGoodsOpAnalyze::IndicatorVector * PPViewGoodsOpAnalyze::GetIndicatorEntry(const GoaAddingBlock & rBlk)
{
	return GetIndicatorEntry(rBlk.FinalGoodsID, rBlk.ArID, rBlk.LocID, rBlk.Sign);
}

const PPViewGoodsOpAnalyze::IndicatorVector * PPViewGoodsOpAnalyze::GetIndicatorEntryC(PPID ident) const
{
	const IndicatorVector * p_result = 0;
	uint idx = 0;
	IndicatorVector key;
	key.Ident = ident;
	if(IndicatorList.bsearch(&key, &idx, PTR_CMPFUNC(BzsValVector_Ident))) {
		p_result = IndicatorList.at(idx);
	}
	return p_result;
}

int PPViewGoodsOpAnalyze::AddItem(const GoaAddingBlock & rBlk)
{
	int    ok = 1;
	const  bool   is_profitable = (Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval) ? true : LOGIC(rBlk.Flags & GoaAddingBlock::fProfitable);
	const  double _q = rBlk.Qtty;
	double sum_cost  = rBlk.Cost  * _q;
	double sum_price = rBlk.Price * _q;
	uint   pos = 0;
	int    r = -1;
	{
		//
		// Ищем в кэше элемент, соответствующий тому, что добавляется (pBlk) //
		//
		GoaCacheItem key;
		GoaCacheItem item;
		key.Sign    = rBlk.TiSign;
		key.GoodsID = rBlk.FinalGoodsID;
		key.ArID    = rBlk.ArID;
		key.LocID   = rBlk.LocID;
		key.Cost    = rBlk.Cost;
		key.Price   = rBlk.Price;
		if(P_Cache->bsearch(&key, &pos, Cf))
			r = 1;
		else {
			TempGoodsOprTbl::Rec  rec;
			TempGoodsOprTbl::Key3 k;
			k.InOutTag = rBlk.TiSign;
			k.GoodsID = rBlk.FinalGoodsID;
			k.Object  = rBlk.ArID;
			k.LocID   = 0;
			if(P_TempTbl->search(3, &k, spGe) && k.InOutTag == rBlk.TiSign && k.GoodsID == rBlk.FinalGoodsID && (!rBlk.ArID || k.Object == rBlk.ArID))
				do {
					P_TempTbl->copyBufTo(&rec);
					item.Sign     = rec.InOutTag;
					item.GoodsID  = rec.GoodsID;
					item.ArID     = rec.Object;
					item.LocID    = rec.LocID;
					item.Cost     = rec.Cost;
					item.Price    = rec.Price;
					if(Cf(&item, &key, 0) == 0) {
						item.UnitPerPack = rec.UnitPerPack;
						item.OldCost  = rec.OldCost;
						item.OldPrice = rec.OldPrice;
						item.SumCost  = rec.SumCost;
						item.SumPrice = rec.SumPrice;
						item.Qtty     = rec.Quantity;
						item.PhQtty   = rec.PhQtty;
						item.Income   = rec.Income;
						item.Counter  = 1;
						THROW_DB(P_TempTbl->getPosition(&item.DbPos));
						if(P_Cache->getCount() >= GOA_MAX_CACHE_ITEMS)
							THROW(FlashCacheItems(GOA_CACHE_DELTA));
						THROW_SL(P_Cache->ordInsert(&item, &pos, Cf));
						r = 1;
					}
				} while(r < 0 && P_TempTbl->search(&k, spNext) && k.InOutTag == rBlk.TiSign &&
					k.GoodsID == rBlk.FinalGoodsID && (!rBlk.ArID || k.Object == rBlk.ArID));
		}
	}
	if(r > 0) {
		GoaCacheItem * p_item = GetCacheItem(pos);
		if(rBlk.Flags & GoaAddingBlock::fTradePlan) {
			p_item->OldCost  += _q;
			p_item->OldPrice += rBlk.Price * _q;
		}
		else {
			p_item->Qtty += _q;
			p_item->Rest += rBlk.Rest;
			p_item->PhRest += rBlk.PhRest;
			p_item->RestCostSum  += rBlk.RestCostSum;
			p_item->RestPriceSum += rBlk.RestPriceSum;
			const double sum_q = p_item->Qtty;
			p_item->PhQtty   += rBlk.PhQtty;
			p_item->SumCost  += sum_cost;
			p_item->SumPrice += sum_price;
			if(R6(sum_q) != 0.0) {
				if(Filt.Flags & GoodsOpAnalyzeFilt::fDiffByNetPrice)
					p_item->Cost = R2(p_item->SumCost / sum_q);
				else if(!(Filt.Flags & GoodsOpAnalyzeFilt::fDiffByPrice)) {
					p_item->Cost  = R2(p_item->SumCost  / sum_q);
					p_item->Price = R2(p_item->SumPrice / sum_q);
				}
				if(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation) {
					if(rBlk.OldCost)
						p_item->OldCost  = R2((p_item->OldCost * (sum_q - _q) + rBlk.OldCost * _q) / sum_q);
					if(rBlk.OldPrice)
						p_item->OldPrice = R2((p_item->OldPrice * (sum_q - _q) + rBlk.OldPrice * _q) / sum_q);
				}
				else if(Filt.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt) {
					p_item->OldCost  += rBlk.OldCost;
					p_item->OldPrice += rBlk.OldPrice;
				}
				else if(State & sReval) {
					p_item->OldCost  += (rBlk.OldCost * _q);
					p_item->OldPrice += (rBlk.OldPrice * _q);
				}
			}
			if(is_profitable && State & sAccsCost)
				p_item->Income += (sum_price - sum_cost);
		}
		p_item->Counter++;
	}
	else {
		GoaCacheItem item;
		item.Sign     = rBlk.TiSign;
		item.GoodsID  = rBlk.FinalGoodsID;
		item.ArID     = rBlk.ArID;
		item.LocID    = rBlk.LocID;
		item.UnitPerPack = rBlk.UnitPerPack;
		if(rBlk.Flags & GoaAddingBlock::fTradePlan) {
			item.OldCost  = _q;
			item.OldPrice = rBlk.Price * _q;
		}
		else {
			item.Qtty     = _q;
			item.Rest     = rBlk.Rest;
			item.PhRest   = rBlk.PhRest;
			item.RestCostSum  = rBlk.RestCostSum;
			item.RestPriceSum = rBlk.RestPriceSum;
			item.PhQtty   = rBlk.PhQtty;
			item.SumCost  = sum_cost;
			item.SumPrice = sum_price;
			item.OldCost  = rBlk.OldCost;  // non additive
			item.OldPrice = rBlk.OldPrice; // non additive
			if(R6(_q) != 0.0) {
				if(Filt.Flags & GoodsOpAnalyzeFilt::fDiffByNetPrice)
					item.Cost = R2(item.SumCost / _q);
				else if(!(Filt.Flags & GoodsOpAnalyzeFilt::fDiffByPrice)) {
					item.Cost  = R2(item.SumCost  / _q);
					item.Price = R2(item.SumPrice / _q);
				}
				else {
					item.Cost = rBlk.Cost;
					item.Price = rBlk.Price;
				}
				if(State & sReval) {
					item.OldCost  = (rBlk.OldCost * _q);
					item.OldPrice = (rBlk.OldPrice * _q);
				}
			}
			if(is_profitable && State & sAccsCost)
				item.Income = (sum_price - sum_cost);
		}
		item.Counter = 1;
		if(P_Cache->getCount() >= GOA_MAX_CACHE_ITEMS)
			THROW(FlashCacheItems(GOA_CACHE_DELTA));
		THROW_SL(P_Cache->ordInsert(&item, 0, Cf));
	}
	CATCHZOK
	return ok;
}

void PPViewGoodsOpAnalyze::InitAddingBlock(const PPBillPacket * pPack, double part, int sign, GoaAddingBlock * pBlk)
{
	const  long f = Filt.Flags;
	pBlk->ParentGrpID = 0;
	pBlk->Flags &= ~GoaAddingBlock::fTradePlan;
	SETFLAG(pBlk->Flags, GoaAddingBlock::fProfitable, CheckOpFlags(pPack->Rec.OpID, OPKF_PROFITABLE));
	SETFLAG(pBlk->Flags, GoaAddingBlock::fIncomeWithoutExcise, (pBlk->Flags & GoaAddingBlock::fProfitable && f & GoodsOpAnalyzeFilt::fPriceWithoutExcise));
	pBlk->ArID  = (f & GoodsOpAnalyzeFilt::fIntrReval) ? pPack->Rec.Object : 0L;
	pBlk->OpID  = pPack->Rec.OpID;
	pBlk->LocID = (f & GoodsOpAnalyzeFilt::fEachLocation) ? pPack->Rec.LocID : LocList_.GetSingle();
	pBlk->BillSign = (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogSelected && sign) ? sign : 1;
	pBlk->Sign = sign;
	pBlk->Part = part;
}

int PPViewGoodsOpAnalyze::PreprocessTi(const PPTransferItem * pTi, const PPIDArray * pSupplBillList, long substBillVal, GoaAddingBlock * pBlk)
{
	int    ok = 1;
	const  PPID goods_id = labs(pTi->GoodsID);
	if(Filt.SupplID && pTi->Suppl != Filt.SupplID)
		return -1;
	if(P_TradePlanPacket && !(pBlk->Flags & GoaAddingBlock::fTradePlan) && Filt.Flags & GoodsOpAnalyzeFilt::fTradePlanGoodsOnly) {
		if(!P_TradePlanPacket->lsearch(&goods_id, 0, CMPF_LONG, offsetof(PPTransferItem, GoodsID)))
			return -1;
	}
	if(GObj.Fetch(goods_id, &pBlk->GoodsRec) <= 0)
		return -1;
	if(Filt.GoodsIdList.IsExists()) {
		if(Filt.IsLeadedInOutAnalyze())
			;
		else if(!Filt.GoodsIdList.CheckID(goods_id))
			return -1;
	}
	else if(Filt.GoodsGrpID) {
		if(P_GoodsList) {
			if(!P_GoodsList->Has(goods_id))
				return -1;
		}
		else if(!GObj.BelongToGroup(goods_id, Filt.GoodsGrpID, &pBlk->ParentGrpID))
			return -1;
	}
	if(pSupplBillList) {
		ReceiptTbl::Rec lot_rec;
		if(P_BObj->trfr->Rcpt.SearchOrigin(labs(pTi->LotID), 0, 0, &lot_rec) > 0) {
			if(!pSupplBillList->bsearch(lot_rec.BillID))
				return -1;
		}
		else
			return -1;
	}
	pBlk->TiSign = (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogSelected) ? 0 : pBlk->Sign;
	if(pBlk->TiSign && pTi->Flags & PPTFR_MINUS)
		pBlk->TiSign *= -1;

	pBlk->Cost        = 0.0;
	pBlk->Price       = 0.0;
	pBlk->OldCost     = 0.0;
	pBlk->OldPrice    = 0.0;
	pBlk->Rest        = 0.0;
	pBlk->PhRest      = 0.0;
	pBlk->RestCostSum = 0.0;
	pBlk->RestPriceSum = 0.0;
	pBlk->UnitPerPack = pTi->UnitPerPack;
	pBlk->Qtty        = fabs(pTi->Qtty());
	if(pBlk->Part != 0.0 && pBlk->Part != 1.0)
		pBlk->Qtty *= pBlk->Part;
	double phuperu = 0.0;
	pBlk->PhQtty = (GObj.GetPhUPerU(goods_id, 0, &phuperu) > 0) ? (pBlk->Qtty * phuperu) : 0.0;
	if(Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval)
		pBlk->Cost = pTi->Price;
	else if(State & sAccsCost)
		if(pTi->Flags & PPTFR_REVAL) {
			pBlk->OldCost = pTi->RevalCost;
			pBlk->Cost = pTi->Cost;
		}
		else
			pBlk->Cost = pTi->Cost;
	if(Filt.QuotKindID > 0) {
		const QuotIdent qi(pBlk->LocID, Filt.QuotKindID);
		GObj.GetQuotExt(goods_id, qi, pTi->Cost, pTi->Price, &pBlk->Price, 1);
	}
	else if(pTi->Flags & PPTFR_REVAL) {
		pBlk->OldPrice = pTi->Discount;
		pBlk->Price    = pTi->Price;
	}
	else
		pBlk->Price = pTi->NetPrice();
	if(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation)
		if(pTi->Discount <= 0.0)
			pBlk->OldCost = -pTi->Discount;
		else
			pBlk->OldPrice = pTi->Discount;
	if((pTi->Flags & (PPTFR_COSTWOVAT|PPTFR_PRICEWOTAXES)) || (pBlk->Flags & GoaAddingBlock::fIncomeWithoutExcise) ||
		(Filt.Flags & (GoodsOpAnalyzeFilt::fCalcCVat|GoodsOpAnalyzeFilt::fCalcPVat))) {
		double tax_factor = 1.0;
		GObj.MultTaxFactor(goods_id, &tax_factor);
		const  PPID   lot_tg_id   = pTi->LotTaxGrpID;
		const  PPID   goods_tg_id = pBlk->GoodsRec.TaxGrpID;
		if(!(Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval) && pTi->Flags & PPTFR_COSTWOVAT) {
			if(pBlk->Cost != 0.0)
				GObj.AdjCostToVat(lot_tg_id, goods_tg_id, pTi->LotDate, tax_factor, &pBlk->Cost, 1);
			if(pBlk->OldCost != 0.0 && !(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation) && !P_TradePlanPacket)
				GObj.AdjCostToVat(lot_tg_id, goods_tg_id, pTi->LotDate, tax_factor, &pBlk->OldCost, 1);
		}
		if(pTi->Flags & PPTFR_PRICEWOTAXES || (pBlk->Flags & GoaAddingBlock::fIncomeWithoutExcise)) {
			PPGoodsTaxEntry gtx;
			const  bool re = LOGIC(pTi->Flags & PPTFR_RMVEXCISE);
			const  bool excl_stax = (CConfig.Flags & CCFLG_PRICEWOEXCISE) ? !re : re;
			if(GObj.FetchTaxEntry2(goods_id, 0/*lotID*/, 0/*taxPayerID*/, pTi->Date, pBlk->OpID, &gtx) > 0) {
				if(pTi->Flags & PPTFR_PRICEWOTAXES) {
					GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &pBlk->Price, excl_stax);
					if(pBlk->OldPrice != 0.0 && !(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation) && !P_TradePlanPacket)
						GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &pBlk->OldPrice, excl_stax);
				}
				//
				// Если требуется рассчитать доходность без акциза, то из цены реализации вычитаем величину акциза.
				//
				if(pBlk->Flags & GoaAddingBlock::fIncomeWithoutExcise) {
					GTaxVect gtv;
					gtv.Calc_(gtx, pBlk->Price, tax_factor, GTAXVF_BEFORETAXES, excl_stax ? GTAXVF_SALESTAX : 0);
					const double excise = gtv.GetValue(GTAXVF_EXCISE);
					pBlk->Price -= excise;
					if(pBlk->OldPrice != 0.0 && (!(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation)))
						pBlk->OldPrice -= excise;
				}
			}
		}
	}
	pBlk->Qtty   *= pBlk->BillSign;
	pBlk->PhQtty *= pBlk->BillSign;
	if(!!Filt.Sgb)
		pBlk->FinalGoodsID = substBillVal;
	else {
		PPObjGoods::SubstBlock sgg_blk;
		sgg_blk.ExclParentID = Filt.GoodsGrpID;
		sgg_blk.LocID = pTi->LocID;
		sgg_blk.LotID = pTi->LotID;
		THROW(GObj.SubstGoods(goods_id, &pBlk->FinalGoodsID, Filt.Sgg, &sgg_blk, &Gsl));
	}
	if(Filt.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt)
		pBlk->PhQtty = pBlk->OldPrice = pBlk->OldCost = 0.0;
	CATCHZOK
	return ok;
}

int PPViewGoodsOpAnalyze::CreateOrderTable(IterOrder ord, TempOrderTbl ** ppTbl)
{
	int    ok = 1;
	TempOrderTbl * p_o = 0;
	*ppTbl = 0;
	if(oneof5(ord, OrdByQtty, OrdByCostSum, OrdByPriceSum, OrdByIncome, OrdByRest)) {
		TempGoodsOprTbl * p_t = P_TempTbl;
		BExtQuery q(p_t, 0, 64);
		THROW(p_o = CreateTempOrderFile());
		{
			SString temp_buf;
			BExtInsert bei(p_o);
			q.select(p_t->ID__, p_t->GoodsID, p_t->Quantity, p_t->SumCost, p_t->SumPrice, p_t->Income, p_t->InOutTag, p_t->Rest, 0L);
			TempGoodsOprTbl::Key0 k;
			MEMSZERO(k);
			for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
				const double abc_idx = (double)((p_t->data.InOutTag < 0) ? -p_t->data.InOutTag : 0);
				const double large_val = 1e12;
				const char * p_fmt = "%04.0lf%030.8lf";
				TempOrderTbl::Rec ord_rec;
				ord_rec.ID = p_t->data.ID__;
				if(ord == OrdByQtty)
					temp_buf.Printf(p_fmt, abc_idx, large_val-p_t->data.Quantity);
				else if(ord == OrdByCostSum)
					temp_buf.Printf(p_fmt, abc_idx, large_val-p_t->data.SumCost);
				else if(ord == OrdByPriceSum)
					temp_buf.Printf(p_fmt, abc_idx, large_val-p_t->data.SumPrice);
				else if(ord == OrdByIncome)
					temp_buf.Printf(p_fmt, abc_idx, large_val-p_t->data.Income);
				else if(ord == OrdByRest)
					temp_buf.Printf(p_fmt, abc_idx, large_val-p_t->data.Rest);
				temp_buf.CopyTo(ord_rec.Name, sizeof(ord_rec.Name));
				THROW_DB(bei.insert(&ord_rec));
			}
			THROW_DB(bei.flash());
		}
		*ppTbl = p_o;
		p_o = 0;
	}
	else
		ok = -1;
	CATCHZOK
	delete p_o;
	return ok;
}

int PPViewGoodsOpAnalyze::ChangeOrder(BrowserWindow * pW)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GOPANLZORD);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->AddClusterAssocDef(CTL_GOPANLZORD_ORDER,  0, OrdByGoodsName);
		dlg->AddClusterAssoc(CTL_GOPANLZORD_ORDER,  1, OrdByQtty);
		dlg->AddClusterAssoc(CTL_GOPANLZORD_ORDER,  2, OrdByCostSum);
		dlg->AddClusterAssoc(CTL_GOPANLZORD_ORDER,  3, OrdByPriceSum);
		dlg->AddClusterAssoc(CTL_GOPANLZORD_ORDER,  4, OrdByIncome);
		dlg->AddClusterAssoc(CTL_GOPANLZORD_ORDER,  5, OrdByRest);
		dlg->SetClusterData(CTL_GOPANLZORD_ORDER, CurrentViewOrder);
		dlg->DisableClusterItem(CTL_GOPANLZORD_ORDER, 5, !(Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest));
		if(ExecView(dlg) == cmOK) {
			long   ord = 0;
			dlg->GetClusterData(CTL_GOPANLZORD_ORDER, &ord);
			if(ord != static_cast<long>(CurrentViewOrder)) {
				uint brw_id = 0;
				SString sub_title;
				CurrentViewOrder = static_cast<PPViewGoodsOpAnalyze::IterOrder>(ord);
				DBQuery * p_q = CreateBrowserQuery(&brw_id, &sub_title);
				if(p_q) {
					DBQBrowserDef * p_def = static_cast<DBQBrowserDef *>(pW->getDef());
					p_def->setQuery(*p_q);
					pW->setSubTitle(sub_title);
					pW->Refresh();
					ok = 1;
				}
				else
					ok = PPErrorZ();
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int PPViewGoodsOpAnalyze::ViewDetail(PPViewBrowser * pBrw, PPID rowIdent, PPID locID, PPID goodsID, short abcGroup, bool viewAllLots)
{
	int    ok = -1;
	if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) {
		if(pBrw) {
			const  BrowserDef * p_def = pBrw->getDefC();
			if(p_def) {
				const int cur_column_idx = pBrw->GetCurColumn();
				if(cur_column_idx >= 0 && cur_column_idx < p_def->getCountI()) {
					const BroColumn & r_col = p_def->at(cur_column_idx);
					switch(r_col.OrgOffs) {
						case 101: // PPBZSI_ORDCOUNT
						case 102: // PPBZSI_ORDQTTY
							if(MpDBlk.OrdList.getCount()) {
								bool skip = false;
								BillFilt bill_filt;
								if(goodsID) {
									PPIDArray list;
									P_BObj->LimitBillListByGoods(MpDBlk.OrdList, labs(goodsID), list);
									if(list.getCount())
										bill_filt.List.Set(&list);
									else
										skip = true;
								}
								else {
									bill_filt.List.Set(&MpDBlk.OrdList);
								}
								if(!skip) {
									bill_filt.Bbt = bbtOrderBills;
									BillFilt::FiltExtraParam p(0, bill_filt.Bbt);
									PPView::Execute(PPVIEW_BILL, &bill_filt, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
								}
							}
							break;
						case 103: // PPBZSI_SALECOUNT
						case 104: // PPBZSI_SALEQTTY
							if(MpDBlk.ShipmList.getCount()) {
								bool skip = false;
								BillFilt bill_filt;
								if(goodsID) {
									PPIDArray list;
									P_BObj->LimitBillListByGoods(MpDBlk.ShipmList, labs(goodsID), list);
									if(list.getCount())
										bill_filt.List.Set(&list);
									else
										skip = true;
								}
								else
									bill_filt.List.Set(&MpDBlk.ShipmList);
								if(!skip) {
									bill_filt.Bbt = bbtGoodsBills;
									BillFilt::FiltExtraParam p(0, bill_filt.Bbt);
									PPView::Execute(PPVIEW_BILL, &bill_filt, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
								}
							}
							break;
						case 105: // PPBZSI_ORDCANCELLEDCOUNT
						case 106: // PPBZSI_ORDCANCELLEDQTTY
						case 1105:
							if(MpDBlk.CancelledOrdList.getCount()) {
								bool skip = false;
								BillFilt bill_filt;
								if(goodsID) {
									PPIDArray list;
									P_BObj->LimitBillListByGoods(MpDBlk.CancelledOrdList, labs(goodsID), list);
									if(list.getCount())
										bill_filt.List.Set(&list);
									else
										skip = true;
								}
								else
									bill_filt.List.Set(&MpDBlk.CancelledOrdList);
								if(!skip) {
									bill_filt.Bbt = bbtOrderBills;
									BillFilt::FiltExtraParam p(0, bill_filt.Bbt);
									PPView::Execute(PPVIEW_BILL, &bill_filt, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
								}
							}
							break;
						case 107: // PPBZSI_MPSELLERSPART
							break;
						case 108: // PPBZSI_MPCOMMISSION
							break;
						case 109: // PPBZSI_MPACQUIRING
							break;
						case 1111: // PPBZSI_ORDSHIPMDELAYDAYSAVG
							break;
						case 121: // PPBZSI_SALERETCOUNT
						case 122: // PPBZSI_SALERETQTTY
						case 123: // PPBZSI_SALERETCOST
							if(MpDBlk.RetList.getCount()) {
								bool skip = false;
								BillFilt bill_filt;
								if(goodsID) {
									PPIDArray list;
									P_BObj->LimitBillListByGoods(MpDBlk.RetList, labs(goodsID), list);
									if(list.getCount())
										bill_filt.List.Set(&list);
									else
										skip = true;
								}
								else
									bill_filt.List.Set(&MpDBlk.RetList);
								if(!skip) {
									bill_filt.Bbt = bbtGoodsBills;
									BillFilt::FiltExtraParam p(0, bill_filt.Bbt);
									PPView::Execute(PPVIEW_BILL, &bill_filt, (GetModelessStatus() ? PPView::exefModeless : 0), &p);
								}
							}
							break;
					}
				}
			}
		}
	}
	else if(abcGroup != 0 && Filt.ABCAnlzGroup > 0) {
		GoodsOpAnalyzeFilt old_filt;
		old_filt = Filt;
		IterOrder old_order = CurrentViewOrder;
		Filt.ABCAnlzGroup = -abcGroup;
		CurrentViewOrder  = GetIterOrder(); // в данном случае вызывать после выражения Filt.ABCAnlzGroup = -abcGroup;
		State &= ~sTotalInited;
		Total.Init();
		ok = Browse(0);
		State &= ~sTotalInited;
		Total.Init();
		Filt = old_filt;
		CurrentViewOrder = old_order;
	}
	else if(Filt.Sgg) {
		GoodsOpAnalyzeFilt temp_filt;
		temp_filt = Filt;
		temp_filt.Sgg = sggNone;
		temp_filt.Flags &= ~GoodsOpAnalyzeFilt::fUseABCAnlz;
		if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation) {
			if(locID) {
				temp_filt.LocList.Set(0);
				temp_filt.LocList.Add(locID);
			}
		}
		int    view = 1;
		if(Filt.Sgg == sggSuppl)
			temp_filt.SupplID = (goodsID & ~GOODSSUBSTMASK);
		else if(Filt.Sgg == sggSupplAgent)
			temp_filt.SupplAgentID = (goodsID & ~GOODSSUBSTMASK);
		else if(Filt.Sgg == sggLocation)
			temp_filt.LocList.Add((goodsID & ~GOODSSUBSTMASK));
		else if(Gsl.GetGoodsBySubstID(goodsID, &temp_filt.GoodsIdList) > 0)
			; //ViewGoodsOpAnalyze(&temp_filt);
		else
			view = 0;
		if(view)
			ViewGoodsOpAnalyze(&temp_filt);
	}
	else if(!!Filt.Sgb) {
		PPIDArray list;
		GoodsOpAnalyzeFilt temp_filt;
		temp_filt = Filt;
		temp_filt.Sgg = sggNone;
		temp_filt.Sgb.Reset();
		temp_filt.Flags &= ~GoodsOpAnalyzeFilt::fUseABCAnlz;
		Bsp.AsscList.GetListByKey(goodsID, list);
		temp_filt.BillList.Set(&list);
		ViewGoodsOpAnalyze(&temp_filt);
	}
	else {
		LotFilt lot_flt;
		lot_flt.GoodsID = labs(goodsID);
		if(viewAllLots == 0) {
			lot_flt.Flags |= LotFilt::fOrders;
			lot_flt.ClosedTag = 1;
		}
		if(locID)
			lot_flt.LocList.Add(locID);
		else
			lot_flt.LocList = LocList_;
		if(!ViewLots(&lot_flt, 0, 1))
			ok = 0;
	}
	return ok;
}

void PPViewGoodsOpAnalyze::ViewTotal()
{
	if((State & sTotalInited) || CalcTotal(&Total)) {
		uint   dlg_id = 0;
		SString temp_buf;
		int    view_cmp_total = 0;
		if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze)
			dlg_id = DLG_INOUTANLZTOTAL;
		else if(P_TradePlanPacket)
			dlg_id = DLG_GDSOPRPLANTOTAL;
		else if(P_CmpView && ((P_CmpView->State & sTotalInited) || P_CmpView->CalcTotal(&P_CmpView->Total))) {
			view_cmp_total = 1;
			dlg_id = DLG_GDSOPRTOTALCMP;
		}
		else if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) // @v12.1.11 
			dlg_id = DLG_GDSOPRMPTOTAL;
		else
			dlg_id = DLG_GDSOPRTOTAL;
		AmtListDialog * dlg = new AmtListDialog(dlg_id, CTL_GDSOPRTOTAL_AMTLIST, 0, &Total.Amounts, 0, 0, 0);
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setCtrlData(CTL_GDSOPRTOTAL_COUNT,     &Total.Count);
			if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) { // @v12.1.11
				TView * v = dlg->getCtrlView(CTL_GDSOPRTOTAL_BZSILIST);
				if(TView::IsSubSign(v, TV_SUBSIGN_LISTBOX)) {
					SmartListBox * p_lbx = static_cast<SmartListBox *>(v);
					if(SetupStrListBox(p_lbx)) {
						StringSet ss(SLBColumnDelim);
						for(uint i = 0; i < Total.IndicatorVect.getCount(); i++) {
							const BzsVal & r_ind = Total.IndicatorVect.at(i);
							if(r_ind.Bzsi != PPBZSI_ORDSHIPMDELAYDAYS) {
								ss.Z();
								temp_buf.Z();
								PPObjBizScore::GetBzsiDescr(r_ind.Bzsi, temp_buf);
								if(temp_buf.IsEmpty())
									temp_buf.Cat("bzsi").CatChar('#').Cat(r_ind.Bzsi);
								ss.add(temp_buf);
								temp_buf.Z().Cat(r_ind.Val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
								ss.add(temp_buf);
								p_lbx->addItem(r_ind.Bzsi, ss.getBuf());
							}
						}
						{
							double value = 0.0;
							const int cind = PPBZSI_ORDSHIPMDELAYDAYSAVG;
							if(Total.IndicatorVect.GetCompound(cind, &value)) {
								ss.Z();
								temp_buf.Z();
								PPObjBizScore::GetBzsiDescr(cind, temp_buf);
								if(temp_buf.IsEmpty())
									temp_buf.Cat("bzsi").CatChar('#').Cat(cind);
								ss.add(temp_buf);
								temp_buf.Z().Cat(value, MKSFMTD(0, 1, NMBF_NOTRAILZ));
								ss.add(temp_buf);
								p_lbx->addItem(cind, ss.getBuf());
							}
						}
						{
							double value = 0.0;
							const int cind = PPBZSI_ORDCANCELLATIONRATE;
							if(Total.IndicatorVect.GetCompound(cind, &value)) {
								ss.Z();
								temp_buf.Z();
								PPObjBizScore::GetBzsiDescr(cind, temp_buf);
								if(temp_buf.IsEmpty())
									temp_buf.Cat("bzsi").CatChar('#').Cat(cind);
								ss.add(temp_buf);
								temp_buf.Z().Cat(value, MKSFMTD(0, 3, NMBF_NOTRAILZ));
								ss.add(temp_buf);
								p_lbx->addItem(cind, ss.getBuf());
							}
						}
						{
							static const long expenses_indicator_list[] = { PPBZSI_SALECOST, PPBZSI_FREIGHT, PPBZSI_MPPROMOTION, PPBZSI_MPACCEPTANCE, PPBZSI_MPSTORAGE };
							double value = 0.0;
							//
							double earnings = 0.0;
							double expenses = 0.0;
							double temp_val = 0.0;
							Total.IndicatorVect.Get(PPBZSI_MPSELLERSPART, &earnings);
							for(uint eiidx = 0; eiidx < SIZEOFARRAY(expenses_indicator_list); eiidx++) {
								Total.IndicatorVect.Get(expenses_indicator_list[eiidx], &temp_val);
								expenses += temp_val;
							}
							value = earnings - expenses;
							//
							{
								ss.Z();
								temp_buf.Z();
								ss.add(PPLoadStringS("profit", temp_buf));
								temp_buf.Z().Cat(value, MKSFMTD(0, 3, NMBF_NOTRAILZ));
								ss.add(temp_buf);
								p_lbx->addItem(PPBZSI_MARGIN, ss.getBuf());
							}
						}
						p_lbx->Draw_();
					}
				}
			}
			else {
				dlg->setCtrlData(CTL_GDSOPRTOTAL_QTTY,      &Total.Qtty);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_PHQTTY,    &Total.PhQtty);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_COST,      &Total.Cost);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_PRICE,     &Total.Price);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INCOME,    &Total.Income);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_RSTCOST,   &Total.RestCost);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_RSTPRICE,  &Total.RestPrice);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INCOUNT,   &Total.InCount);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INQTTY,    &Total.InQtty);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INPHQTTY,  &Total.InPhQtty);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INCOST,    &Total.InCost);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_INPRICE,   &Total.InPrice);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_PLANQTTY,  &Total.PlanQtty);
				dlg->setCtrlData(CTL_GDSOPRTOTAL_PLANSUM,   &Total.PlanSum);
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_DIFCOUNT,  Total.Count - Total.InCount); //{ turistti add
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_DIFQTTY,   Total.Qtty - Total.InQtty);
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_DIFPHQTT,  Total.PhQtty - Total.InPhQtty);
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_DIFCOST,   Total.Cost - Total.InCost);
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_DIFPRICE,  Total.Price - Total.InPrice); //} turistti
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_PCT_QTTY, fdivnz(100.0 * Total.Qtty,  Total.PlanQtty));
				dlg->setCtrlReal(CTL_GDSOPRTOTAL_PCT_SUM,  fdivnz(100.0 * Total.Price, Total.PlanSum));
				if(view_cmp_total) {
					dlg->setCtrlData(CTL_GDSOPRTOTAL_COUNT2,     &P_CmpView->Total.Count);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_QTTY2,      &P_CmpView->Total.Qtty);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_PHQTTY2,    &P_CmpView->Total.PhQtty);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_COST2,      &P_CmpView->Total.Cost);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_PRICE2,     &P_CmpView->Total.Price);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_INCOME2,    &P_CmpView->Total.Income);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_RSTCOST2,   &P_CmpView->Total.RestCost);
					dlg->setCtrlData(CTL_GDSOPRTOTAL_RSTPR2,     &P_CmpView->Total.RestPrice);
				}
			}
			ExecViewAndDestroy(dlg);
		}
	}
}

int PPViewGoodsOpAnalyze::ViewGraph()
{
	int    ok = -1;
	if(Filt.ABCAnlzGroup > 0) {
		GoodsOpAnalyzeTotal  goa_total;
		TSVector <TempGoodsOprTbl::Rec>  abc_ary;
		TempGoodsOprTbl::Key0 k;
		TempGoodsOprTbl * p_t = P_TempTbl;
		BExtQuery q(P_TempTbl, 0, 16);
		q.selectAll();
		MEMSZERO(k);
		for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
			if(Filt.IsValidABCGroup(p_t->data.InOutTag)) {
				goa_total.Qtty   += p_t->data.Quantity;
				goa_total.Cost   += p_t->data.SumCost;
				goa_total.Price  += p_t->data.SumPrice;
				goa_total.Income += p_t->data.Income;
				goa_total.Count  += (long)p_t->data.OldCost;
				abc_ary.insert(&p_t->data);
			}
		}
		if(abc_ary.getCount()) {
			uint  c;
			SString temp_buf;
			Generator_GnuPlot plot(0);
			Generator_GnuPlot::PlotParam param;
			Generator_GnuPlot::StyleTics xtics;
			plot.StartData(1);
			plot.PutData(PPLoadStringS("group", temp_buf).Transf(CTRANSF_INNER_TO_OUTER), 1);
			for(c = 0; c < abc_ary.getCount(); c++) {
				(temp_buf = abc_ary.at(c).Text).ReplaceChar(' ', '_').Transf(CTRANSF_INNER_TO_OUTER);
				plot.PutData(temp_buf, 1);
				param.Legend.Add(c + 2, temp_buf);
			}
			plot.PutEOR();
			if(goa_total.Qtty) {
				PPLoadStringS("qtty", temp_buf).Transf(CTRANSF_INNER_TO_OUTER);
				plot.PutData(temp_buf, 1);
				for(c = 0; c < abc_ary.getCount(); c++)
					plot.PutData(abc_ary.at(c).Quantity / goa_total.Qtty);
				plot.PutEOR();
			}
			if(goa_total.Cost) {
				PPLoadStringS("amount", temp_buf).Transf(CTRANSF_INNER_TO_OUTER).CatChar('_').Cat("Cost");
				plot.PutData(temp_buf, 1);
				for(c = 0; c < abc_ary.getCount(); c++)
					plot.PutData(abc_ary.at(c).SumCost / goa_total.Cost);
				plot.PutEOR();
			}
			if(goa_total.Price) {
				PPLoadStringS("amount", temp_buf).Transf(CTRANSF_INNER_TO_OUTER).CatChar('_').Cat("Price");
				plot.PutData(temp_buf, 1);
				for(c = 0; c < abc_ary.getCount(); c++)
					plot.PutData(abc_ary.at(c).SumPrice / goa_total.Price);
				plot.PutEOR();
			}
			if(goa_total.Income) {
				PPLoadString("income", temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				plot.PutData(temp_buf, 1);
				for(c = 0; c < abc_ary.getCount(); c++)
					plot.PutData(abc_ary.at(c).Income / goa_total.Income);
				plot.PutEOR();
			}
			if(goa_total.Count) {
				plot.PutData(PPGetWord(PPWORD_POS, 1, temp_buf), 1);
				for(c = 0; c < abc_ary.getCount(); c++)
					plot.PutData(abc_ary.at(c).OldCost / goa_total.Count);
				plot.PutEOR();
			}
			plot.PutEndOfData();

			plot.Preamble();
			plot.SetGrid();
			plot.SetStyleFill("solid");
			plot.SetAxisRange(Generator_GnuPlot::axY, 0.0, 1.0);
			xtics.Rotate = 90;
			xtics.Font.Size = 8;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, xtics.Font.Face);
			plot.SetTics(Generator_GnuPlot::axX, &xtics);
			param.Flags |= Generator_GnuPlot::PlotParam::fHistogram;
			plot.Plot(&param);
			ok = plot.Run();
		}
	}
	return ok;
}

/*static*/int FASTCALL PPViewGoodsOpAnalyze::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewGoodsOpAnalyze * p_v = static_cast<PPViewGoodsOpAnalyze *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int PPViewGoodsOpAnalyze::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) {
			const BrwHdr * p_item = static_cast<const BrwHdr *>(pBlk->P_SrcData);
			const IndicatorVector * p_ind = GetIndicatorEntryC(p_item->RowId);
			double value = 0.0;
			switch(pBlk->ColumnN) {
				case 101: // PPBZSI_ORDCOUNT
					if(p_ind) {
						p_ind->Get(PPBZSI_ORDCOUNT, &value);
						pBlk->Set(static_cast<int32>(value));
						ok = 1;
					}
					break; 
				case 102: // PPBZSI_ORDQTTY
					if(p_ind) {
						p_ind->Get(PPBZSI_ORDQTTY, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 103: // PPBZSI_SALECOUNT
					if(p_ind) {
						p_ind->Get(PPBZSI_SALECOUNT, &value);
						pBlk->Set(static_cast<int32>(value));
						ok = 1;
					}
					break;
				case 104: // PPBZSI_SALEQTTY
					if(p_ind) {
						p_ind->Get(PPBZSI_SALEQTTY, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 105: // PPBZSI_ORDCANCELLEDCOUNT
					if(p_ind) {
						p_ind->Get(PPBZSI_ORDCANCELLEDCOUNT, &value);
						pBlk->Set(static_cast<int32>(value));
						ok = 1;
					}
					break;
				case 106: // PPBZSI_ORDCANCELLEDQTTY
					if(p_ind) {
						p_ind->Get(PPBZSI_ORDCANCELLEDQTTY, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 1105: //PPBZSI_ORDCANCELLATIONRATE
					//pBrw->insertColumn(-1, "CancellationRate",  1105, T_DOUBLE, MKSFMTD(0, 4, 0), BCO_CAPRIGHT|BCO_USERPROC);
					if(p_ind) {
						p_ind->GetCompound(PPBZSI_ORDCANCELLATIONRATE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 107: // PPBZSI_MPSELLERSPART
					if(p_ind) {
						p_ind->Get(PPBZSI_MPSELLERSPART, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 108: // PPBZSI_MPCOMMISSION
					if(p_ind) {
						p_ind->Get(PPBZSI_MPCOMMISSION, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 109: // PPBZSI_MPACQUIRING
					if(p_ind) {
						p_ind->Get(PPBZSI_MPACQUIRING, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 1111: // PPBZSI_ORDSHIPMDELAYDAYSAVG
					if(p_ind) {
						p_ind->GetCompound(PPBZSI_ORDSHIPMDELAYDAYSAVG, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 112: // PPBZSI_MPAMT_ORDPRICE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPAMT_ORDPRICE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 113: // PPBZSI_MPAMT_ORDSELLERPRICE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPAMT_ORDSELLERPRICE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 114: // PPBZSI_MPAMT_SHIPMPRICE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPAMT_SHIPMPRICE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 115: // PPBZSI_MPAMT_SHIPMSELLERPRICE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPAMT_SHIPMSELLERPRICE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 116: // PPBZSI_SALECOST
					if(p_ind) {
						p_ind->Get(PPBZSI_SALECOST, &value);
						pBlk->Set(value);
						ok = 1;
					}					
					break;
				case 1116: // Income
					if(p_ind) {
						double earnings = 0.0;
						double expenses = 0.0;
						double temp_val = 0.0;
						static const long expenses_indicator_list[] = { PPBZSI_SALECOST, PPBZSI_FREIGHT, PPBZSI_MPPROMOTION, PPBZSI_MPACCEPTANCE, PPBZSI_MPSTORAGE };
						p_ind->Get(PPBZSI_MPSELLERSPART, &earnings);
						for(uint eiidx = 0; eiidx < SIZEOFARRAY(expenses_indicator_list); eiidx++) {
							p_ind->Get(expenses_indicator_list[eiidx], &temp_val);
							expenses += temp_val;
						}
						value = earnings - expenses;
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 117: // PPBZSI_FREIGHT
					if(p_ind) {
						p_ind->Get(PPBZSI_FREIGHT, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 121: // PPBZSI_SALERETCOUNT
					if(p_ind) {
						p_ind->Get(PPBZSI_SALERETCOUNT, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 122: // PPBZSI_SALERETQTTY
					if(p_ind) {
						p_ind->Get(PPBZSI_SALERETQTTY, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 123: // PPBZSI_SALERETCOST
					if(p_ind) {
						p_ind->Get(PPBZSI_SALERETCOST, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 124: // PPBZSI_MPSTORAGE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPSTORAGE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 125: // PPBZSI_MPACCEPTANCE
					if(p_ind) {
						p_ind->Get(PPBZSI_MPACCEPTANCE, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
				case 126: // PPBZSI_MPPROMOTION
					if(p_ind) {
						p_ind->Get(PPBZSI_MPPROMOTION, &value);
						pBlk->Set(value);
						ok = 1;
					}
					break;
			}
		}
	}
	return ok;
}

void PPViewGoodsOpAnalyze::PreprocessBrowser(PPViewBrowser * pBrw)
{
	DBQBrowserDef * p_def = pBrw ? static_cast<DBQBrowserDef *>(pBrw->getDef()) : 0;
	const DBQuery * p_q = p_def ? p_def->getQuery() : 0;
	if(p_q) {
		int    loc_ins_col = (Filt.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze) ? 2 : 1;
		uint   brw_id = pBrw->GetResID();
		SString temp_buf;
		if(brw_id == BROWSER_GOODSOPER_MP) { // @v12.1.7
			assert(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze);
			int    mp_col = -1;
			uint   column_idx = 1;
			if(MpDBlk.OrdList.getCount()) {
				//PPLoadString("ware", temp_buf);
				//pBrw->insertColumn(-1, "OrdCount",           101, T_INT32,  0, BCO_USERPROC);
				pBrw->InsColumn(-1, "@qtty", 102, T_DOUBLE, MKSFMTD(0, 3, NMBF_NOTRAILZ), BCO_USERPROC);
				//pBrw->insertColumn(-1, "SaleCount",          103, T_INT32,  0, BCO_USERPROC);
				pBrw->InsColumn(-1, "@sum_clientprice", 112, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				pBrw->InsColumn(-1, "@sum_sellerprice", 113, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = 3;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("booking_pl", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
			if(MpDBlk.CancelledOrdList.getCount()) {
				pBrw->insertColumn(-1, "@qtty", 106, T_DOUBLE, MKSFMTD(0, 3, NMBF_NOTRAILZ), BCO_USERPROC);
				pBrw->insertColumn(-1, "Rate",  1105, T_DOUBLE, MKSFMTD(0, 4, 0), BCO_USERPROC);
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = 2;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("ordercancellation_pl", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
			if(MpDBlk.ShipmList.getCount()) {
				pBrw->InsColumn(-1, "@qtty", 104, T_DOUBLE, MKSFMTD(0, 3, NMBF_NOTRAILZ), BCO_USERPROC);
				pBrw->InsColumn(-1, "@sumcost", 116, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				pBrw->InsColumn(-1, "@sum_clientprice", 114, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				pBrw->InsColumn(-1, "@sum_sellerprice", 115, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = 4;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("shipment_pl", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
			if(MpDBlk.RetList.getCount()) { // @v12.1.11
				pBrw->InsColumn(-1, "@qtty", 122, T_DOUBLE, MKSFMTD(0, 3, NMBF_NOTRAILZ), BCO_USERPROC);
				pBrw->InsColumn(-1, "@sumcost", 123, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = 2;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("returnfrombuyer_pl", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
			pBrw->insertColumn(-1, "@sellersincome",     107, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
			column_idx += 1;
			{
				uint grp_count = 0;
				pBrw->insertColumn(-1, "@commission_s",  108, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				grp_count++;
				pBrw->insertColumn(-1, "@bankacquiring", 109, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				grp_count++;
				pBrw->insertColumn(-1, "@freight", 117, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				grp_count++;
				// @v12.1.12 {
				pBrw->insertColumn(-1, "@storage", 124, T_DOUBLE, SFMT_MONEY, BCO_USERPROC); 
				grp_count++;
				pBrw->insertColumn(-1, "@warehouseacceptance", 125, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				grp_count++;
				pBrw->insertColumn(-1, "@advpromotion", 126, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
				grp_count++;
				// } @v12.1.12
				{
					BroGroup grp;
					grp.First = column_idx;
					grp.Count = grp_count;
					grp.Height = 1;
					grp.P_Text = newStr(PPLoadStringS("expense_pl", temp_buf));
					p_def->AddColumnGroup(&grp);
					column_idx += grp.Count;
				}
			}
			pBrw->insertColumn(-1, "@profit", 1116, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
			pBrw->insertColumn(-1, "@bzsi_ordshipmdelaydaysavg", 1111, T_DOUBLE, SFMT_MONEY, BCO_USERPROC);
			CALLPTRMEMB(pBrw, SetDefUserProc(PPViewGoodsOpAnalyze::GetDataForBrowser, this));
		}
		else if(brw_id == BROWSER_GOODSOPER && Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) {
			pBrw->insertColumn(-1, "@rest", 16, 0, MKSFMTD(10, 6, NMBF_NOTRAILZ|NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
			if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcOrder) {
				pBrw->InsColumn(-1, "@booking", 14, 0, MKSFMTD(10, 3, NMBF_NOTRAILZ|NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
			}
			if(Filt.Flags & GoodsOpAnalyzeFilt::fShowSStatSales) {
				pBrw->InsColumn(-1, "@avgdaylysales", 11, 0, MKSFMTD(10, 3, NMBF_NOTRAILZ|NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
			}
		}
		else if(brw_id == BROWSER_ABCANLZ) {
			if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) {
				BroGroup grp;
				grp.First = 7;
				grp.Count = 2;
				grp.Height = 1;
				PPGetWord(PPWORD_REST_IN, 0, temp_buf);
				grp.P_Text = newStr(temp_buf);
				pBrw->getDef()->AddColumnGroup(&grp);
				pBrw->InsColumnWord(-1, PPWORD_COST,    17, 0, MKSFMTD(11, 2, NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
				pBrw->InsColumnWord(-1, PPWORD_PRICE_P, 18, 0, MKSFMTD(10, 2, NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
			}
			pBrw->InsColumnWord(6, PPWORD_POS, 14, 0, MKSFMTD(8, 0, NMBF_NOZERO|ALIGN_RIGHT), BCO_CAPRIGHT); // @v12.1.7 +1
		}
		//
		// Сравнение товарного отчета по операции за 2 разных периода
		//
		else if(brw_id == BROWSER_GOODSOPER_COMPARE) {
			SString main_title, add_title, diff_title;
			PPLoadString("main", main_title);
			PPGetWord(PPWORD_ADDITIONAL, 0, add_title);
			PPLoadString("daterange", temp_buf);
			PPLoadString("difference", diff_title);
			main_title.Space().Cat(temp_buf);
			add_title.Space().Cat(temp_buf);
			if(Filt.Flags & GoodsOpAnalyzeFilt::fComparePctDiff)
				diff_title.Space().CatChar('%');
			for(uint i = 0, first_c = 1; i < Filt.CompareItems.getCount(); i++) {
				uint c = 0;
				uint c2 = 0;
				uint c3 = 0;
				uint c4 = 0;
				uint cols_count = 0;
				const long id = Filt.CompareItems.at(i).ObjID;
				const long flags = Filt.CompareItems.at(i).Flags;
				const bool is_main_prd = LOGIC(flags & GoodsOpAnalyzeFilt::ffldMainPeriod);
				const bool is_add_prd  = LOGIC(flags & GoodsOpAnalyzeFilt::ffldCmpPeriod);
				const bool is_diff     = LOGIC(flags & GoodsOpAnalyzeFilt::ffldDiff);
				SString c_title;
				switch(id) {
					case GoodsOpAnalyzeFilt::fldidQtty:
						PPLoadString("qtty", c_title);
						c  = 4; // @v12.1.7 +1
						c2 = 23; // @v12.1.7 +1
						c3 = 34; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidRest:
						PPLoadString("rest", c_title);
						c  = 12; // @v12.1.7 +1
						c2 = 24; // @v12.1.7 +1
						c3 = 35; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidCostRest:
						PPGetWord(PPWORD_REST_IN, 0, c_title);
						PPGetWord(PPWORD_COST,    0, temp_buf);
						c_title.Space().Cat(temp_buf);
						c  = 17; // @v12.1.7 +1
						c2 = 25; // @v12.1.7 +1
						c3 = 36; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidPriceRest:
						PPGetWord(PPWORD_REST_IN, 0, c_title);
						PPGetWord(PPWORD_PRICE_P, 0, temp_buf);
						c_title.Space().Cat(temp_buf);
						c  = 18; // @v12.1.7 +1
						c2 = 26; // @v12.1.7 +1
						c3 = 37; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidCostAmount:
						PPGetWord(PPWORD_SUM_IN, 0, c_title);
						PPGetWord(PPWORD_COST,   0, temp_buf);
						c_title.Space().Cat(temp_buf);
						c  =  6; // @v12.1.7 +1
						c2 = 27; // @v12.1.7 +1
						c3 = 38; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidPriceAmount:
						PPGetWord(PPWORD_SUM_IN,  0, c_title);
						PPGetWord(PPWORD_PRICE_P, 0, temp_buf);
						c_title.Space().Cat(temp_buf);
						c  =  7; // @v12.1.7 +1
						c2 = 28; // @v12.1.7 +1
						c3 = 39; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidIncome:
						PPLoadString("income", c_title);
						c  =  8; // @v12.1.7 +1
						c2 = 29; // @v12.1.7 +1
						c3 = 40; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidPctProfitable:
						PPLoadString("income", c_title);
						c_title.Space().CatChar('%');
						c  = 30; // @v12.1.7 +1
						c2 = 32; // @v12.1.7 +1
						c3 = 41; // @v12.1.7 +1
						break;
					case GoodsOpAnalyzeFilt::fldidPctMargin:
						PPGetWord(PPWORD_MARGIN, 0, c_title);
						c_title.Space().CatChar('%');
						c  = 31; // @v12.1.7 +1
						c2 = 33; // @v12.1.7 +1
						c3 = 42; // @v12.1.7 +1
						break;
				}
				if(is_main_prd) {
					pBrw->insertColumn(-1, main_title, c, 0L, MKSFMTD(10, 2, NMBF_NOZERO), 0);
					cols_count++;
				}
				if(is_add_prd) {
					pBrw->insertColumn(-1, add_title, c2, 0L, MKSFMTD(10, 2, NMBF_NOZERO), 0);
					cols_count++;
				}
				if(is_diff) {
					pBrw->insertColumn(-1, diff_title, c3, 0L, MKSFMTD(10, 2, NMBF_NOZERO), 0);
					cols_count++;
				}
				if(is_main_prd || is_add_prd || is_diff) {
					BrowserDef * p_def = pBrw->getDef();
					if(p_def) {
						BroGroup grp;
						grp.First = first_c;
						grp.Count = cols_count;
						grp.Height = 1;
						grp.P_Text = newStr(c_title);
						c_title.CopyTo(grp.P_Text, c_title.Len() + 1);
						p_def->AddColumnGroup(&grp);
					}
				}
				first_c += cols_count;
			}
		}
		if(P_Ct == 0 && Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation && Filt.ABCAnlzGroup <= 0) {
			pBrw->InsColumn(loc_ins_col, "@warehouse", 19, 0, 0, 0); // @v12.1.7 +1
		}
		if(Filt.Sgg == sggNone && !Filt.Sgb) {
			pBrw->InsColumn(1, "@barcode", (P_Ct ? 3 : 20), 0, 0, 0); // @v12.1.7 19-->20
		}
		pBrw->SetTempGoodsGrp(Filt.GoodsGrpID);
	}
}

DBQuery * PPViewGoodsOpAnalyze::CreateBrowserQuery(uint * pBrwID, SString * pSubTitle)
{
	uint   brw_id = 0;
	long   ff = 0;
	SString sub_title;
	TempGoodsOprTbl * tbl = 0;
	TempGoodsOprTbl * p_add_ttbl = 0;
	TempOrderTbl    * p_ot = 0;
	DBQ  * dbq = 0;
	DBE    dbe_loc;
	DBE    dbe_pct_qtty;
	DBE    dbe_pct_sum;
	DBE    dbe_cqtty;
	DBE    dbe_crest;
	DBE    dbe_pct_income1;
	DBE    dbe_pct_income2;
	DBE    dbe_pct_margin1;
	DBE    dbe_pct_margin2;
	DBE    dbe_diffpct_qtty;
	DBE    dbe_diffpct_rest;
	DBE    dbe_diffpct_restcost;
	DBE    dbe_diffpct_restprice;
	DBE    dbe_diffpct_sumcost;
	DBE    dbe_diffpct_sumprice;
	DBE    dbe_diffpct_income;
	DBE    dbe_diffpct_profit;
	DBE    dbe_diffpct_margin;
	DBQuery * q = 0;
	if(P_Ct == 0) {
		if(P_CmpView) {
			SString tbl_name;
			P_CmpView->GetTempTableName(tbl_name);
			THROW_MEM(p_add_ttbl = new TempGoodsOprTbl(tbl_name));
		}
		if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogMarketplaceSalesAnalyze) // @v12.1.7
			brw_id = BROWSER_GOODSOPER_MP;
		else if(p_add_ttbl)
			brw_id = BROWSER_GOODSOPER_COMPARE;
		else if(Filt.ABCAnlzGroup > 0)
			brw_id = BROWSER_ABCANLZ;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval)
			brw_id = BROWSER_INTRREVAL;
		else if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze)
			brw_id = BROWSER_INOUTANLZ;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation)
			brw_id = BROWSER_GOODSOPER_PRICEDEV;
		else if(P_TradePlanPacket)
			brw_id = BROWSER_GOODSOPER_PLAN;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt)
			brw_id = BROWSER_GOODSOPER_CMPWITHRECEIPT;
		else if(State & sReval)
			brw_id = BROWSER_GOODSOPER_REVAL;
		else
			brw_id = BROWSER_GOODSOPER;
		Filt.GetOpName(sub_title);
		if(Filt.Flags & GoodsOpAnalyzeFilt::fDisplayWoPacks)
			ff = QTTYF_FRACTION | NMBF_NOZERO;
		else if(LConfig.Flags & CFGFLG_USEPACKAGE)
			ff = QTTYF_COMPLPACK | QTTYF_FRACTION | NMBF_NOZERO;
		else
			ff = QTTYF_FRACTION | NMBF_NOZERO;
		if(oneof5(CurrentViewOrder, OrdByQtty, OrdByCostSum, OrdByPriceSum, OrdByIncome, OrdByRest))
			THROW(CreateOrderTable(CurrentViewOrder, &p_ot));
		tbl = new TempGoodsOprTbl(P_TempTbl->GetName());
		{
			dbe_cqtty.init();
			dbe_cqtty.push(tbl->Quantity);
			dbe_cqtty.push(tbl->UnitPerPack);
			DBConst dbc_long;
			dbc_long.init(1L);
			dbe_cqtty.push(dbc_long); // Trust
			dbc_long.init(ff);
			dbe_cqtty.push(dbc_long); // Formatting flags
			dbe_cqtty.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
		}
		{
			dbe_crest.init();
			dbe_crest.push(tbl->Rest);
			dbe_crest.push(tbl->UnitPerPack);
			DBConst dbc_long;
			dbc_long.init(1L);
			dbe_crest.push(dbc_long); // Trust
			dbc_long.init(ff);
			dbe_crest.push(dbc_long); // Formatting flags
			dbe_crest.push(static_cast<DBFunc>(PPDbqFuncPool::IdCQtty));
		}
		q = & select(
			tbl->ID__,           // #00 // @v12.1.7  
    
			tbl->LocID,          // #01 // @v12.1.7 +1
			tbl->GoodsID,        // #02 // @v12.1.7 +1
			tbl->Text,           // #03 // @v12.1.7 +1
			tbl->Quantity,       // #04 // @v12.1.7 +1
			tbl->PhQtty,         // #05 // @v12.1.7 +1
			tbl->SumCost,        // #06 // @v12.1.7 +1
			tbl->SumPrice,       // #07 // @v12.1.7 +1
			tbl->Income,         // #08 // @v12.1.7 +1
			tbl->InOutTag,       // #09 // @v12.1.7 +1
			tbl->PctVal,         // #10 // @v12.1.7 +1
			tbl->OldPrice,       // #11 // @v12.1.7 +1
			tbl->Rest,           // #12 // @v12.1.7 +1
			tbl->Price,          // #13 // @v12.1.7 +1
			tbl->OldCost,        // #14 // @v12.1.7 +1
			0L);
		q->addField(dbe_cqtty);         // #15 // @v12.1.7 +1
		q->addField(dbe_crest);         // #16 // @v12.1.7 +1
		q->addField(tbl->RestCostSum);  // #17 // @v12.1.7 +1
		q->addField(tbl->RestPriceSum); // #18 // @v12.1.7 +1
		{
			PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc, tbl->LocID);
			q->addField(dbe_loc);       // #19 // @v12.1.7 +1
		}
		q->addField(tbl->Barcode);      // #20 // @v12.1.7 +1
		if(P_TradePlanPacket) {
			PPDbqFuncPool::InitPctFunc(dbe_pct_qtty, tbl->Quantity, tbl->OldCost, 0);
			PPDbqFuncPool::InitPctFunc(dbe_pct_sum,  tbl->SumPrice, tbl->OldPrice, 0);
			q->addField(dbe_pct_qtty);  // #21 // @v12.1.7 +1
			q->addField(dbe_pct_sum);   // #22 // @v12.1.7 +1
		}
		else {
			q->addField(tbl->ID__);     // #21 // @v12.1.7 +1
			q->addField(tbl->ID__);     // #22 // @v12.1.7 +1
		}
		//
		// Сравнение товарного отчета по операции за 2 разных периода
		//
		if(p_add_ttbl) {
			PPDbqFuncPool::InitPctFunc(dbe_pct_income1, tbl->Income, tbl->SumCost, 0);
			PPDbqFuncPool::InitPctFunc(dbe_pct_income2, p_add_ttbl->Income, p_add_ttbl->SumCost, 0);
			PPDbqFuncPool::InitPctFunc(dbe_pct_margin1, tbl->Income, tbl->SumPrice, 0);
			PPDbqFuncPool::InitPctFunc(dbe_pct_margin2, p_add_ttbl->Income, p_add_ttbl->SumPrice, 0);
			//
			// Значения для дополнительного периода
			//
			q->addField(p_add_ttbl->Quantity);       // #23 // @v12.1.7 +1
			q->addField(p_add_ttbl->Rest);           // #24 // @v12.1.7 +1
			q->addField(p_add_ttbl->RestCostSum);    // #25 // @v12.1.7 +1
			q->addField(p_add_ttbl->RestPriceSum);   // #26 // @v12.1.7 +1
			q->addField(p_add_ttbl->SumCost);        // #27 // @v12.1.7 +1
			q->addField(p_add_ttbl->SumPrice);       // #28 // @v12.1.7 +1
			q->addField(p_add_ttbl->Income);         // #29 // @v12.1.7 +1
			q->addField(dbe_pct_income1);            // #30 // @v12.1.7 +1
			q->addField(dbe_pct_margin1);            // #31 // @v12.1.7 +1
			q->addField(dbe_pct_income2);            // #32 // @v12.1.7 +1
			q->addField(dbe_pct_margin2);            // #33 // @v12.1.7 +1
			//
			// Разница
			//
			if(!(Filt.Flags & GoodsOpAnalyzeFilt::fComparePctDiff)) {
				q->addField(tbl->Quantity     - p_add_ttbl->Quantity);       // #34 // @v12.1.7 +1
				q->addField(tbl->Rest         - p_add_ttbl->Rest);           // #35 // @v12.1.7 +1
				q->addField(tbl->RestCostSum  - p_add_ttbl->RestCostSum);    // #36 // @v12.1.7 +1
				q->addField(tbl->RestPriceSum - p_add_ttbl->RestPriceSum);   // #37 // @v12.1.7 +1
				q->addField(tbl->SumCost      - p_add_ttbl->SumCost);        // #38 // @v12.1.7 +1
				q->addField(tbl->SumPrice     - p_add_ttbl->SumPrice);       // #39 // @v12.1.7 +1
				q->addField(tbl->Income       - p_add_ttbl->Income);         // #40 // @v12.1.7 +1
				dbe_pct_income1.DontDestroy = 1;
				dbe_pct_income2.DontDestroy = 1;
				dbe_pct_margin1.DontDestroy = 1;
				dbe_pct_margin2.DontDestroy = 1;
				q->addField(dbe_pct_income1 - dbe_pct_income2);              // #41 // @v12.1.7 +1
				q->addField(dbe_pct_margin1 - dbe_pct_margin2);              // #42 // @v12.1.7 +1
			}
			//
			// Разница в %
			//
			else {
				//
				// Разница в % кол-во
				//
				dbe_diffpct_qtty.init();
				dbe_diffpct_qtty.push(tbl->Quantity - p_add_ttbl->Quantity);
				dbe_diffpct_qtty.push(p_add_ttbl->Quantity);
				dbe_diffpct_qtty.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % остатки
				//
				dbe_diffpct_rest.init();
				dbe_diffpct_rest.push(tbl->Rest - p_add_ttbl->Rest);
				dbe_diffpct_rest.push(p_add_ttbl->Rest);
				dbe_diffpct_rest.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % остатки в ценах поступлени
				//
				dbe_diffpct_restcost.init();
				dbe_diffpct_restcost.push(tbl->RestCostSum - p_add_ttbl->RestCostSum);
				dbe_diffpct_restcost.push(p_add_ttbl->RestCostSum);
				dbe_diffpct_restcost.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % остатки в ценах реализации
				//
				dbe_diffpct_restprice.init();
				dbe_diffpct_restprice.push(tbl->RestPriceSum - p_add_ttbl->RestPriceSum);
				dbe_diffpct_restprice.push(p_add_ttbl->RestPriceSum);
				dbe_diffpct_restprice.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % сумма в ценах поступления //
				//
				dbe_diffpct_sumcost.init();
				dbe_diffpct_sumcost.push(tbl->SumCost - p_add_ttbl->SumCost);
				dbe_diffpct_sumcost.push(p_add_ttbl->SumCost);
				dbe_diffpct_sumcost.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % сумма в ценах реализации
				//
				dbe_diffpct_sumprice.init();
				dbe_diffpct_sumprice.push(tbl->SumPrice - p_add_ttbl->SumPrice);
				dbe_diffpct_sumprice.push(p_add_ttbl->SumPrice);
				dbe_diffpct_sumprice.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
				//
				// Разница в % доход
				//
				dbe_diffpct_income.init();
				dbe_diffpct_income.push(tbl->Income - p_add_ttbl->Income);
				dbe_diffpct_income.push(p_add_ttbl->Income);
				dbe_diffpct_income.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));

				dbe_pct_income1.DontDestroy = 1;
				dbe_pct_income2.DontDestroy = 1;
				dbe_pct_margin1.DontDestroy = 1;
				dbe_pct_margin2.DontDestroy = 1;

				dbe_diffpct_profit.init();
				dbe_diffpct_profit.push(dbe_pct_income1 - dbe_pct_income2);
				dbe_diffpct_profit.push(dbe_pct_income2);
				dbe_diffpct_profit.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));

				dbe_diffpct_margin.init();
				dbe_diffpct_margin.push(dbe_pct_margin1 - dbe_pct_margin2);
				dbe_diffpct_margin.push(dbe_pct_margin2);
				dbe_diffpct_margin.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));

				q->addField(dbe_diffpct_qtty);      // #34 // @v12.1.7 +1
				q->addField(dbe_diffpct_rest);      // #35 // @v12.1.7 +1
				q->addField(dbe_diffpct_restcost);  // #36 // @v12.1.7 +1
				q->addField(dbe_diffpct_restprice); // #37 // @v12.1.7 +1
				q->addField(dbe_diffpct_sumcost);   // #38 // @v12.1.7 +1
				q->addField(dbe_diffpct_sumprice);  // #39 // @v12.1.7 +1
				q->addField(dbe_diffpct_income);    // #40 // @v12.1.7 +1
				q->addField(dbe_diffpct_profit);    // #41 // @v12.1.7 +1
				q->addField(dbe_diffpct_margin);    // #42 // @v12.1.7 +1
			}
		}
		if(p_add_ttbl) {
			if(p_ot)
				q->from(p_ot, tbl, p_add_ttbl, 0L);
			else
				q->from(tbl, p_add_ttbl, 0L);
		}
		else {
			if(p_ot)
				q->from(p_ot, tbl, 0L);
			else
				q->from(tbl, 0L);
		}
		if(p_add_ttbl)
			dbq = & (*dbq && (tbl->ID__ += p_add_ttbl->ID__));
		if(p_ot || (Filt.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz)) {
			if(Filt.Flags & GoodsOpAnalyzeFilt::fUseABCAnlz)
				if(Filt.ABCAnlzGroup > 0)
					dbq = & (*dbq && tbl->InOutTag > 0L);
				else if(Filt.ABCAnlzGroup < 0)
					dbq = & (*dbq && tbl->InOutTag == (long)Filt.ABCAnlzGroup);
			if(p_ot)
				dbq = & (*dbq && p_ot->ID == tbl->ID__);
			q->where(*dbq);
			if(p_ot)
				q->orderBy(p_ot->Name, 0L);
			else
				q->orderBy(tbl->InOutTag, tbl->Text, 0L);
		}
		else if(!!Filt.Sgb && Filt.Sgb.S == SubstGrpBill::sgbDate) {
			q->orderBy(tbl->InOutTag, tbl->GoodsID, 0L);
		}
		else {
			if(p_add_ttbl)
				q->where(*dbq);
			q->orderBy(tbl->InOutTag, tbl->Text, 0L);
		}
		THROW(CheckQueryPtr(q));
	}
	else {
		brw_id = BROWSER_GOODSOPER_CROSSTAB;
		q = PPView::CrosstabDbQueryStub;
		Filt.GetOpName(sub_title);
	}
	ASSIGN_PTR(pSubTitle, sub_title);
	ASSIGN_PTR(pBrwID, brw_id);
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete tbl;
			delete p_add_ttbl;
			delete p_ot;
		}
	ENDCATCH
	return q;
}

int PPViewGoodsOpAnalyze::ConvertLinesToBasket()
{
	int    ok = -1, r = 0;
	int    convert = (Filt.Sgg == sggNone && !(Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval) &&
		Filt.OpGrpID != GoodsOpAnalyzeFilt::ogInOutAnalyze && Filt.ABCAnlzGroup <= 0);
	if(convert) {
		SelBasketParam param;
		param.SelPrice = 1;
		THROW(r = GetBasketByDialog(&param, GetSymb()));
		if(r > 0) {
			GoodsOpAnalyzeViewItem item;
			PPWaitStart();
			for(InitIteration(PPViewGoodsOpAnalyze::OrdByGoodsName); NextIteration(&item) > 0;) {
				const  PPID goods_id = labs(item.GoodsID);
				if(!(goods_id & GOODSSUBSTMASK)) {
					double qtty = 0.0;
					ILTI   i_i;
					ReceiptTbl::Rec lot_rec;
					THROW(::GetCurGoodsPrice(goods_id, item.LocID, GPRET_MOSTRECENT, 0, &lot_rec) != GPRET_ERROR);
					i_i.GoodsID     = goods_id;
					i_i.UnitPerPack = item.UnitPerPack;
					i_i.Cost     = R5(lot_rec.Cost);
					i_i.Price    = R5((param.SelPrice == 1) ? item.Cost : item.Price);
					i_i.CurPrice = 0.0;
					i_i.Flags    = 0;
					i_i.Suppl    = lot_rec.SupplID;
					i_i.QCert    = lot_rec.QCertID;
					i_i.Expiry   = lot_rec.Expiry;
					if(param.Flags & SelBasketParam::fUseGoodsRestAsQtty) {
						if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) {
							qtty = item.Rest;
						}
						else {
							GoodsRestParam  gr_param;
							gr_param.Date    = ZERODATE; // Текущий остаток
							gr_param.LocID   = item.LocID;
							gr_param.GoodsID = goods_id;
							THROW(P_BObj->trfr->GetCurRest(gr_param));
							qtty = gr_param.Total.Rest;
						}
					}
					else
						qtty = fabs((double)item.Qtty);
					i_i.Quantity = qtty;
					i_i.Rest     = i_i.Quantity;
					THROW(param.Pack.AddItem(&i_i, 0, param.SelReplace));
				}
				PPWaitPercent(GetCounter());
			}
			PPWaitStop();
			THROW(GoodsBasketDialog(param, 1));
			ok = 1;
		}
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

void PPViewGoodsOpAnalyze::GetTempTableName(SString & rBuf) const
{
	if(P_TempTbl)
		rBuf = P_TempTbl->GetName();
	else
		rBuf.Z();
}

int PPViewGoodsOpAnalyze::ABCGrpToAltGrp(short abcGroup)
{
	int    ok = -1;
	TDialog * p_dlg = 0;
	GoodsOpAnalyzeFilt old_filt = Filt;
	if(abcGroup > 0 && Filt.ABCAnlzGroup > 0) {
		int    valid_data = 0;
		PPID   grp_id = 0;
		p_dlg = new TDialog(DLG_GRPSEL);
		THROW(CheckDialogPtr(&p_dlg));
		SetupPPObjCombo(p_dlg, CTLSEL_GRPSEL_GROUP, PPOBJ_GOODSGROUP, 0, OLW_CANINSERT|OLW_LOADDEFONOPEN, reinterpret_cast<void *>(GGRTYP_SEL_ALT));
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			grp_id = p_dlg->getCtrlLong(CTLSEL_GRPSEL_GROUP);
			if(!GObj.IsAltGroup(grp_id)) {
				grp_id = 0;
				PPError(PPERR_NOTALTGRP);
			}
			else
				valid_data = 1;
		}
		if(grp_id && valid_data) {
			GoodsOpAnalyzeViewItem item;
			Filt.ABCAnlzGroup = -abcGroup; // all goods belong to this group
			PPWaitStart();
			for(InitIteration(PPViewGoodsOpAnalyze::OrdByDefault); NextIteration(&item) > 0;) {
				THROW(GObj.AssignGoodsToAltGrp(item.GoodsID, grp_id, 0, 1));
				PPWaitPercent(GetCounter());
			}
			PPWaitStop();
			ok = GObj.Browse(reinterpret_cast<void *>(grp_id));
		}
	}
	CATCHZOKPPERR
	delete p_dlg;
	Filt = old_filt;
	return ok;
}

int PPViewGoodsOpAnalyze::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	BrwHdr hdr;
	MEMSZERO(hdr);
	if(pHdr)
		hdr = *static_cast<const BrwHdr *>(pHdr);
	ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const int cur_column_idx = pBrw ? pBrw->GetCurColumn() : 0;
		GetEditIds(pHdr, &hdr.LocID, &hdr.GoodsID, cur_column_idx);
		switch(ppvCmd) {
			case PPVCMD_EDITITEM:
				ViewDetail(pBrw, hdr.RowId, hdr.LocID, hdr.GoodsID, hdr.InOutTag, true);
				break;
			case PPVCMD_GOODS:
				ok = -1;
				if(hdr.GoodsID) {
					if(oneof2(Filt.Sgg, sggSuppl, sggSupplAgent)) {
						PPID   _id_to_edit = (hdr.GoodsID & ~GOODSSUBSTMASK);
						if(P_BObj->ArObj.Edit(&_id_to_edit, 0) == cmOK)
							ok = 1;
					}
					else if(Filt.Sgg == sggLocation) {
						PPID   _id_to_edit = (hdr.GoodsID & ~GOODSSUBSTMASK);
						if(LocObj.Edit(&_id_to_edit, 0) == cmOK)
							ok = 1;
					}
					else if(Filt.Sgg == sggManuf) {
						PPID   _id_to_edit = (hdr.GoodsID & ~GOODSSUBSTMASK);
						if(PsnObj.Edit(&_id_to_edit, 0) == cmOK)
							ok = 1;
					}
					else {
						if(GObj.Edit(&hdr.GoodsID, 0) == cmOK)
							ok = 1;
					}
				}
				break;
			case PPVCMD_TRFRANLZ:
				ok = -1;
				{
					Goods2Tbl::Rec goods_rec;
					SETIFZ(P_TrfrFilt, new TrfrAnlzFilt());
					if(P_TrfrFilt && GObj.Fetch(labs(hdr.GoodsID), &goods_rec) > 0) {
						PPViewTrfrAnlz trfr_anlz;
						// @v12.1.7 {
						if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogSelected) {
							P_TrfrFilt->OpID = Filt.OpID;
						}
						// } @v12.1.7
						P_TrfrFilt->Period     = Filt.Period;
						P_TrfrFilt->GoodsID    = goods_rec.ID;
						P_TrfrFilt->GoodsGrpID = goods_rec.ParentID;
						if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
							P_TrfrFilt->LocList.Z().Add(hdr.LocID);
						else
							P_TrfrFilt->LocList = LocList_;
						P_TrfrFilt->SupplID = Filt.SupplID;
						P_TrfrFilt->AgentList.Set(0);
						P_TrfrFilt->AgentList.Add(Filt.AgentID);
						if(trfr_anlz.EditBaseFilt(P_TrfrFilt) > 0)
							::ViewTrfrAnlz(P_TrfrFilt);
					}
				}
				break;
			case PPVCMD_ORDLOTS:
				ViewDetail(pBrw, hdr.RowId, hdr.LocID, hdr.GoodsID, 0, false);
				break;
			case PPVCMD_OPGRPNG:
				ok = -1;
				if(hdr.GoodsID) {
					OpGroupingFilt op_grpng_flt;
					op_grpng_flt.Period  = Filt.Period;
					if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
            			op_grpng_flt.LocList.Add(hdr.LocID);
					else
						op_grpng_flt.LocList = LocList_;
					op_grpng_flt.SupplID = Filt.SupplID;
					op_grpng_flt.GoodsID = hdr.GoodsID;
					ViewOpGrouping(&op_grpng_flt);
				}
				break;
			case PPVCMD_ADDTOBASKET:
				ok = -1;
				if(!Filt.Sgg && !Filt.Sgb)
					AddGoodsToBasket(hdr.GoodsID, LocList_.GetSingle());
				break;
			case PPVCMD_ADDALLTOBASKET:
				ok = -1;
				if(!Filt.Sgg && !Filt.Sgb)
					ConvertLinesToBasket();
				break;
			case PPVCMD_SORT:
				ok = ChangeOrder(pBrw);
				break;
			case PPVCMD_ADDTOALTGRP:
				ok = -1;
				ABCGrpToAltGrp(hdr.InOutTag);
				break;
			case PPVCMD_GRAPH:
				ok = -1;
				ViewGraph();
				break;
		}
	}
	return ok;
}

int PPViewGoodsOpAnalyze::Print(const void *)
{
	int    ok = 1;
	IterOrder ord = OrdByDefault;
	int    no_cost = (State & sAccsCost) ? 0 : 1;
	int    disable_grouping = 0;
	uint   rpt_id = 0;
	TDialog * dlg = 0;
	GoodsOpAnalyzeFilt old_filt = Filt;
	if(Filt.ABCAnlzGroup <= 0) {
		if(!Filt.CmpPeriod.IsZero())
			rpt_id = REPORT_GOODSOPANLZCMP;
		else if(P_TradePlanPacket)
			rpt_id = REPORT_GOODSOPRTRADEPLAN;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fCompareWithReceipt)
			rpt_id = REPORT_GOODSOPANLZDRAFTWROFF;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fEachLocation)
			rpt_id = REPORT_GOODSOPREL;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fIntrReval)
			rpt_id = REPORT_GOODSINTRREVAL;
		else if(Filt.OpGrpID == GoodsOpAnalyzeFilt::ogInOutAnalyze)
			rpt_id = REPORT_GOODSOPRINOUT;
		else if(Filt.Flags & GoodsOpAnalyzeFilt::fPriceDeviation)
			rpt_id = REPORT_GOODSOPRWITHDEV;
		else {
			if(GetOpType(Filt.OpID) == PPOPT_GOODSREVAL)
				rpt_id = REPORT_GOODSOPRREVAL;
			else if(Filt.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods)
				rpt_id = REPORT_GOA_ZEROOP;
			else if(Filt.Flags & GoodsOpAnalyzeFilt::fUnprofitableGoods)
				rpt_id = REPORT_GOA_LOSS;
			else if(Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest)
				rpt_id = REPORT_GOODSOPRPRICEREST;
			else
				rpt_id = REPORT_GOODSOPRPRICE;
			dlg = new TDialog(DLG_GOPANLZPRN);
			if(CheckDialogPtrErr(&dlg)) {
				ushort v = 0;
				dlg->AddClusterAssocDef(CTL_GOPANLZPRN_ORDER, 0, OrdByGoodsName);
				dlg->AddClusterAssoc(CTL_GOPANLZPRN_ORDER, 1, OrdByQtty);
				dlg->AddClusterAssoc(CTL_GOPANLZPRN_ORDER, 2, OrdByCostSum);
				dlg->AddClusterAssoc(CTL_GOPANLZPRN_ORDER, 3, OrdByPriceSum);
				dlg->AddClusterAssoc(CTL_GOPANLZPRN_ORDER, 4, OrdByIncome);
				dlg->SetClusterData(CTL_GOPANLZPRN_ORDER, CurrentViewOrder);
				SETFLAG(v, 0x01, BIN(no_cost));
				dlg->setCtrlData(CTL_GOPANLZPRN_FLAGS, &v);
				dlg->setCtrlUInt16(CTL_GOPANLZPRN_SPC, 0);
				if(ExecView(dlg) == cmOK) {
					long temp_long = 0;
					dlg->GetClusterData(CTL_GOPANLZPRN_ORDER, &temp_long);
					CurrentViewOrder = static_cast<IterOrder>(temp_long);
					ord = CurrentViewOrder;
					v = dlg->getCtrlUInt16(CTL_GOPANLZPRN_SPC);
					if(GetOpType(Filt.OpID) != PPOPT_GOODSREVAL) {
						if(v == 1)
							rpt_id = REPORT_SPCGOODSOPR;
						else {
							v = dlg->getCtrlUInt16(CTL_GOPANLZPRN_FLAGS);
							no_cost = BIN(v & 0x01);
							if(Filt.Flags & GoodsOpAnalyzeFilt::fBadSellingGoods)
								rpt_id = REPORT_GOA_ZEROOP;
							else if(Filt.Flags & GoodsOpAnalyzeFilt::fUnprofitableGoods)
								rpt_id = REPORT_GOA_LOSS;
							else if(no_cost)
								rpt_id = (Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) ? REPORT_GOODSOPRONEPRICEREST : REPORT_GOODSOPRONEPRICE;
							else
								rpt_id = (Filt.Flags & GoodsOpAnalyzeFilt::fCalcRest) ? REPORT_GOODSOPRPRICEREST : REPORT_GOODSOPRPRICE;
						}
					}
					if(oneof4(ord, OrdByQtty, OrdByCostSum, OrdByPriceSum, OrdByIncome) || Filt.Sgg)
						disable_grouping = 1;
				}
				else
					ok = -1;
			}
			else
				ok = 0;
		}
	}
	else {
		uint   sel = 0;
		ok = SelectorDialog(DLG_ABCANLZPRN, CTL_ABCANLZPRN_WHAT, &sel);
		if(ok > 0) {
			if(sel == 1) {
				Filt.ABCAnlzGroup = 2;
				rpt_id = REPORT_GOODSOPRABC;
			}
			else {
				Filt.ABCAnlzGroup = 1;
				rpt_id = REPORT_GOODSOPRABCGRP;
			}
		}
		else
			ok = -1;
	}
	if(ok > 0) {
		PPReportEnv env;
		env.Sort = ord;
		env.PrnFlags = disable_grouping ? SReport::DisableGrouping : 0;
		PPAlddPrint(rpt_id, PView(this), &env);
	}
	Filt = old_filt;
	delete dlg;
	return ok;
}

int ViewGoodsOpAnalyze(const GoodsOpAnalyzeFilt * pFilt) { return PPView::Execute(PPVIEW_GOODSOPANALYZE, pFilt, 1, 0); }
//
// Implementation of PPALDD_GoodsOpAnlz
//
PPALDD_CONSTRUCTOR(GoodsOpAnlz)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsOpAnlz) { Destroy(); }

int PPALDD_GoodsOpAnlz::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsOpAnalyze, rsrv);
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltLocID      = p_filt->LocList.GetSingle();
	H.FltOpID       = p_filt->OpID;
	H.FltOpGrpID    = p_filt->OpGrpID;
	H.FltObjectID   = p_filt->ObjectID;
	H.FltAgentID    = p_filt->AgentID;
	H.FltSupplAgentID = p_filt->SupplAgentID;
	H.FltSupplID    = p_filt->SupplID;
	H.FltBeg        = p_filt->Period.low;
	H.FltEnd        = p_filt->Period.upp;
	H.FltFlags        = p_filt->Flags;
	H.fDiffByPrice    = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fDiffByPrice);
	H.fDiffByNetPrice = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fDiffByNetPrice);
	H.fIntrReval      = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fIntrReval);
	H.fLabelOnly      = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fLabelOnly);
	H.fPriceWithoutExcise = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fPriceWithoutExcise);
	H.fCalcRest     = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fCalcRest);
	H.fEachLocation = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fEachLocation);
	H.RestCalcDate  = p_filt->RestCalcDate;
	SString op_name;
	p_filt->GetOpName(op_name).CopyTo(H.FltOpTxt, sizeof(H.FltOpTxt));
	H.Sgg = p_filt->Sgg;
	if(p_filt->Sgg)
		PPGetSubStr(PPTXT_SUBSTGOODSLIST, p_filt->Sgg-1, H.SggTxt, sizeof(H.SggTxt));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsOpAnlz::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	//INIT_PPVIEW_ALDD_ITER(GoodsOpAnalyze);
	PPViewGoodsOpAnalyze * p_v = static_cast<PPViewGoodsOpAnalyze *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewGoodsOpAnalyze::IterOrder>(SortIdx));
	return 1;
}

int PPALDD_GoodsOpAnlz::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsOpAnalyze);
	PPWaitPercent(p_v->GetCounter());
	I.InOutTag = item.InOutTag;
	I.GoodsID  = item.GoodsID;
	I.GoodsGrpID  = item.GoodsGrpID;
	I.SubstArID   = item.SubstArID;
	I.SubstPsnID  = item.SubstPsnID;
	I.SubstLocID  = item.SubstLocID;
	STRNSCPY(I.GoodsName, item.GoodsName);
	STRNSCPY(I.GrpName,   item.P_GoodsGrpName);
	I.UnitPerPack = item.UnitPerPack;
	I.Qtty     = item.Qtty;
	I.PhQtty   = item.PhQtty;
	I.Cost     = item.Cost;
	I.Price    = item.Price;
	I.OldCost  = item.OldCost;
	I.OldPrice = item.OldPrice;
	I.SumCost  = item.SumCost;
	I.SumPrice = item.SumPrice;
	I.Income   = item.Income;
	I.PctVal   = item.PctVal;
	I.Rest     = item.Rest;
	I.PhRest   = item.PhRest;
	I.RestCost = item.RestCostSum;
	I.RestPrice = item.RestPriceSum;
	const GoodsOpAnalyzeFilt * p_filt = static_cast<const GoodsOpAnalyzeFilt *>(p_v->GetBaseFilt());
	I.LocID = (p_filt->Flags & GoodsOpAnalyzeFilt::fEachLocation) ? item.LocID : 0;
	long   f = (LConfig.Flags & CFGFLG_USEPACKAGE && !p_filt->Sgg && !(p_filt->Flags & GoodsOpAnalyzeFilt::fDisplayWoPacks)) ?
		MKSFMTD(0, 6, QTTYF_COMPLPACK | NMBF_NOTRAILZ) : MKSFMTD(0, 6, NMBF_NOTRAILZ);
	QttyToStr(item.Qtty, item.UnitPerPack, f, I.CQtty);
	QttyToStr(item.Rest, item.UnitPerPack, f, I.CRest);
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsOpAnlz::Destroy() { DESTROY_PPVIEW_ALDD(GoodsOpAnalyze); }
//
// Implementation of PPALDD_GoodsOpAnlzCmp
//
PPALDD_CONSTRUCTOR(GoodsOpAnlzCmp)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsOpAnlzCmp) { Destroy(); }

int PPALDD_GoodsOpAnlzCmp::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(GoodsOpAnalyze, rsrv);
	H.FltGoodsGrpID   = p_filt->GoodsGrpID;
	H.FltLocID        = p_filt->LocList.GetSingle();
	H.FltOpID = p_filt->OpID;
	H.FltOpGrpID      = p_filt->OpGrpID;
	H.FltObjectID     = p_filt->ObjectID;
	H.FltAgentID      = p_filt->AgentID;
	H.FltSupplAgentID = p_filt->SupplAgentID;
	H.FltSupplID      = p_filt->SupplID;
	H.FltBeg  = p_filt->Period.low;
	H.FltEnd  = p_filt->Period.upp;
	H.CmpFltBeg       = p_filt->CmpPeriod.low;
	H.CmpFltEnd       = p_filt->CmpPeriod.upp;
	H.FltFlags        = p_filt->Flags;
	H.fDiffByPrice    = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fDiffByPrice);
	H.fDiffByNetPrice = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fDiffByNetPrice);
	H.fIntrReval      = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fIntrReval);
	H.fLabelOnly      = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fLabelOnly);
	H.fPriceWithoutExcise = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fPriceWithoutExcise);
	H.fCalcRest       = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fCalcRest);
	H.fEachLocation   = BIN(p_filt->Flags & GoodsOpAnalyzeFilt::fEachLocation);
	H.RestCalcDate    = p_filt->RestCalcDate;
	H.CmpRestCalcDate = p_filt->CmpRestCalcDate;
	SString op_name;
	p_filt->GetOpName(op_name).CopyTo(H.FltOpTxt, sizeof(H.FltOpTxt));
	H.Sgg = p_filt->Sgg;
	if(p_filt->Sgg)
		PPGetSubStr(PPTXT_SUBSTGOODSLIST, p_filt->Sgg-1, H.SggTxt, sizeof(H.SggTxt));
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsOpAnlzCmp::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	//INIT_PPVIEW_ALDD_ITER(GoodsOpAnalyze);
	PPViewGoodsOpAnalyze * p_v = static_cast<PPViewGoodsOpAnalyze *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->InitIteration(static_cast<PPViewGoodsOpAnalyze::IterOrder>(SortIdx));
	return 1;
}

int PPALDD_GoodsOpAnlzCmp::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(GoodsOpAnalyze);
	PPWaitPercent(p_v->GetCounter());
	I.GoodsID  = item.GoodsID;
	I.GoodsGrpID  = item.GoodsGrpID;
	I.SubstArID   = item.SubstArID;
	I.SubstPsnID  = item.SubstPsnID;
	I.SubstLocID  = item.SubstLocID;
	STRNSCPY(I.GoodsName, item.GoodsName);
	STRNSCPY(I.GrpName,   item.P_GoodsGrpName);
	I.Qtty    = item.Qtty;
	I.SumCost = item.SumCost;
	I.SumPrice        = item.SumPrice;
	I.Income  = item.Income;
	I.Rest    = item.Rest;
	I.RestCostSum     = item.RestCostSum;
	I.RestPriceSum    = item.RestPriceSum;
	I.CmpQtty = item.Qtty.Cm;
	I.CmpSumCost      = item.SumCost.Cm;
	I.CmpSumPrice     = item.SumPrice.Cm;
	I.CmpIncome       = item.Income.Cm;
	I.CmpRest = item.Rest.Cm;
	I.CmpRestCostSum  = item.RestCostSum.Cm;
	I.CmpRestPriceSum = item.RestPriceSum.Cm;
	const GoodsOpAnalyzeFilt * p_filt = static_cast<const GoodsOpAnalyzeFilt *>(p_v->GetBaseFilt());
	I.LocID = (p_filt->Flags & GoodsOpAnalyzeFilt::fEachLocation) ? item.LocID : 0;
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsOpAnlzCmp::Destroy() { DESTROY_PPVIEW_ALDD(GoodsOpAnalyze); }
