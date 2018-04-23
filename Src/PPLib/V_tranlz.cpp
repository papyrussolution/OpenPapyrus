// V_TRANLZ.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
//#include <graph.h>

SLAPI TrfrAnlzViewItem::TrfrAnlzViewItem()
{
	Clear();
}

void SLAPI TrfrAnlzViewItem::Clear()
{
	memzero(this, offsetof(TrfrAnlzViewItem, BillCode_));
	BillCode_.Z();
	DtText_.Z();
	GoodsText_.Z();
	PersonText_.Z();
}
//
//
//
SLAPI TrfrAnlzTotal::TrfrAnlzTotal()
{
	destroy();
}

void SLAPI TrfrAnlzTotal::destroy()
{
	THISZERO();
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(TrfrAnlz); SLAPI TrfrAnlzFilt::TrfrAnlzFilt() : PPBaseFilt(PPFILT_TRFRANLZ, 0, 3) // @v7.9.11 ver 2-->3
{
	SetFlatChunk(offsetof(TrfrAnlzFilt, ReserveStart),
		offsetof(TrfrAnlzFilt, BillList)-offsetof(TrfrAnlzFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, BillList));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, RcptBillList));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, LocList));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, CtValList));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, ArList));
	SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt, AgentList));
	Init(1, 0);
}

int SLAPI TrfrAnlzFilt::HasCntragentGrouping() const
	{ return oneof6(Grp, gCntragent, gCntragentDate, gGoodsCntragent, gGoodsCntragentDate, gDateCntragentAgentGoods, gBillCntragent); }
int SLAPI TrfrAnlzFilt::HasGoodsGrouping() const
	{ return oneof6(Grp, gGoods, gGoodsCntragent, gGoodsCntragentDate, gGoodsBill, gDateCntragentAgentGoods, gGoodsDate); }
int SLAPI TrfrAnlzFilt::HasDateGrouping() const
	{ return oneof4(Grp, gCntragentDate, gGoodsCntragentDate, gDateCntragentAgentGoods, gGoodsDate); }
int SLAPI TrfrAnlzFilt::HasBillGrouping() const
	{ return oneof2(Grp, gGoodsBill, gBillCntragent); }

int SLAPI TrfrAnlzFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == 2) {
		class TrfrAnlzFilt_v2 : public PPBaseFilt {
		public:
			SLAPI  TrfrAnlzFilt_v2() : PPBaseFilt(PPFILT_TRFRANLZ, 0, 2)
			{
				SetFlatChunk(offsetof(TrfrAnlzFilt_v2, ReserveStart), offsetof(TrfrAnlzFilt_v2, BillList)-offsetof(TrfrAnlzFilt_v2, ReserveStart));
				SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt_v2, BillList));
				SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt_v2, RcptBillList));
				SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt_v2, LocList));
				SetBranchObjIdListFilt(offsetof(TrfrAnlzFilt_v2, CtValList));
				Init(1, 0);
			}

			uint8  ReserveStart[28]; // @anchor
			PPID   SupplAgentID;
			DateRange Period;
			DateRange LotsPeriod;
			PPID   OpID;
			PPID   SupplID;
			PPID   ArID_;
			PPID   DlvrAddrID;
			PPID   AgentID_;
			PPID   PsnCatID;
			PPID   CityID;
			PPID   GoodsGrpID;
			PPID   GoodsID;
			long   Flags;
			int    InitOrd;
			long   CtKind;
			PPID   BrandID;
			Grouping Grp;

			SubstGrpGoods   Sgg;
			SubstGrpPerson  Sgp;
			SubstGrpDate    Sgd;
			ObjIdListFilt   BillList;
			ObjIdListFilt   RcptBillList;
			ObjIdListFilt   LocList;
			ObjIdListFilt   CtValList;
		};
		TrfrAnlzFilt_v2 fv2;
		THROW(fv2.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
#define CPYFLD(f) f = fv2.f
		CPYFLD(SupplAgentID);
		CPYFLD(Period);
		CPYFLD(LotsPeriod);
		CPYFLD(OpID);
		CPYFLD(SupplID);
		CPYFLD(ArID_);
		CPYFLD(DlvrAddrID);
		CPYFLD(AgentID_);
		CPYFLD(PsnCatID);
		CPYFLD(CityID);
		CPYFLD(GoodsGrpID);
		CPYFLD(GoodsID);
		CPYFLD(Flags);
		CPYFLD(InitOrd);
		CPYFLD(CtKind);
		CPYFLD(BrandID);
		CPYFLD(Grp);

		CPYFLD(Sgg);
		CPYFLD(Sgp);
		CPYFLD(Sgd);
		CPYFLD(BillList);
		CPYFLD(RcptBillList);
		CPYFLD(LocList);
		CPYFLD(CtValList);
#undef CPYFLD
		ok = 1;
	}
	CATCHZOK
	return ok;
}

TrfrAnlzFilt & FASTCALL TrfrAnlzFilt::operator = (const TrfrAnlzFilt & rS)
{
	Copy(&rS, 1);
	return *this;
}

int SLAPI TrfrAnlzFilt::IsEqualExcept(const TrfrAnlzFilt & rS, long flags) const
{
#define NEQ_FLD(f) (f) != (rS.f)
	if(NEQ_FLD(Period.low))
		return 0;
	if(NEQ_FLD(Period.upp))
		return 0;
	if(NEQ_FLD(LotsPeriod.low))
		return 0;
	if(NEQ_FLD(LotsPeriod.upp))
		return 0;
	if(NEQ_FLD(OpID))
		return 0;
	if(NEQ_FLD(SupplID))
		return 0;
	if(NEQ_FLD(ArID_))
		return 0;
	if(NEQ_FLD(DlvrAddrID))
		return 0;
	if(NEQ_FLD(AgentID_))
		return 0;
	if(NEQ_FLD(SupplAgentID))
		return 0;
	if(NEQ_FLD(PsnCatID))
		return 0;
	if(NEQ_FLD(CityID))
		return 0;
	if(NEQ_FLD(GoodsGrpID))
		return 0;
	if(NEQ_FLD(GoodsID))
		return 0;
	if(NEQ_FLD(Flags))
		return 0;
	if(NEQ_FLD(Grp))
		return 0;
	if(NEQ_FLD(Sgg))
		return 0;
	if(NEQ_FLD(Sgp))
		return 0;
	if(NEQ_FLD(Sgd))
		return 0;
	if(!BillList.IsEqual(rS.BillList))
		return 0;
	if(!RcptBillList.IsEqual(rS.RcptBillList))
		return 0;
	if(!LocList.IsEqual(rS.LocList))
		return 0;
	if(!ArList.IsEqual(rS.ArList))
		return 0;
	if(!AgentList.IsEqual(rS.AgentList))
		return 0;
	if(!(flags & eqxCrosstab)) {
		if(NEQ_FLD(CtKind))
			return 0;
		if(!CtValList.IsEqual(rS.CtValList))
			return 0;
	}
	if(!(flags & eqxOrder)) {
		if(NEQ_FLD(InitOrd)) // !
			return 0;
	}
#undef NEQ_FLD
	return 1;
}

// virtual
int SLAPI TrfrAnlzFilt::Describe(long flags, SString & rBuf) const
{
	SString temp_buf;
	PutMembToBuf(&Period,     STRINGIZE(Period), rBuf);
	PutMembToBuf(&LotsPeriod, STRINGIZE(LotsPeriod), rBuf);

	PutObjMembToBuf(PPOBJ_OPRKIND,      OpID,        STRINGIZE(OpID),      rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,      SupplID,    STRINGIZE(SupplID),    rBuf);
	PutObjMembToBuf(PPOBJ_GOODS,        GoodsID,    STRINGIZE(GoodsID),    rBuf);
	PutObjMembToBuf(PPOBJ_GOODSGROUP,   GoodsGrpID, STRINGIZE(GoodsGrpID), rBuf);
	PutObjMembToBuf(PPOBJ_WORLD,        CityID,     STRINGIZE(CityID),     rBuf);
	PutObjMembToBuf(PPOBJ_PRSNCATEGORY, PsnCatID,   STRINGIZE(PsnCatID),   rBuf);
	PutObjMembToBuf(PPOBJ_LOCATION,     DlvrAddrID, STRINGIZE(DlvrAddrID), rBuf);
	{
		if(CtKind == ctNone)
			temp_buf = STRINGIZE(ctNone);
		else if(CtKind == ctDate)
			temp_buf = STRINGIZE(ctDate);
		else if(CtKind == ctCntragent)
			temp_buf = STRINGIZE(ctCntragent);
		else if(CtKind == ctLocation)
			temp_buf = STRINGIZE(ctLocation);
		else
			temp_buf.Z();
		PutMembToBuf(temp_buf, STRINGIZE(CtKind), rBuf);
	}
	{
		if(InitOrd == PPViewTrfrAnlz::OrdByDefault)
			temp_buf = STRINGIZE(OrdByDefault);
		else if(InitOrd == PPViewTrfrAnlz::OrdByDate)
			temp_buf = STRINGIZE(OrdByDate);
		else if(InitOrd == PPViewTrfrAnlz::OrdByGoods)
			temp_buf = STRINGIZE(OrdByGoods);
		else if(InitOrd == PPViewTrfrAnlz::OrdByArticle)
			temp_buf = STRINGIZE(OrdByArticle);
		else
			temp_buf.Z();
	 	PutMembToBuf(temp_buf, STRINGIZE(InitOrd), rBuf);
	}
	{
		switch(Grp) {
			case gNone:                    temp_buf = STRINGIZE(gNong); break;
			case gGoods:                   temp_buf = STRINGIZE(gGoods); break;
			case gCntragent:               temp_buf = STRINGIZE(gCntragent); break;
			case gCntragentDate:           temp_buf = STRINGIZE(gCntragentDate); break;
			case gGoodsCntragent:          temp_buf = STRINGIZE(gGoodsCntragent); break;
			case gGoodsCntragentDate:      temp_buf = STRINGIZE(gGoodsCntragentDate); break;
			case gGoodsBill:               temp_buf = STRINGIZE(gGoodsBill); break;
			case gDateCntragentAgentGoods: temp_buf = STRINGIZE(gDateCntragentAgentGoods); break;
			case gGoodsDate:               temp_buf = STRINGIZE(gGoodsDate); break;
			case gBillCntragent:           temp_buf = STRINGIZE(gBillCntragent); break;
			default: temp_buf.Z();
		}
		PutMembToBuf(temp_buf, STRINGIZE(Grp), rBuf);
	}
	{
		long id = 1;
		StrAssocArray flag_list;
		if(Flags & fLabelOnly)          flag_list.Add(id++, STRINGIZE(fLabelOnly));
		if(Flags & fGByDate)            flag_list.Add(id++, STRINGIZE(fGByDate));
		if(Flags & fGetRest)            flag_list.Add(id++, STRINGIZE(fGetRest));
		if(Flags & fSubstPersonRAddr)   flag_list.Add(id++, STRINGIZE(fSubstPersonRAddr));
		if(Flags & fSubstDlvrAddr)      flag_list.Add(id++, STRINGIZE(fSubstDlvrAddr));
		if(Flags & fDiffByDlvrAddr)     flag_list.Add(id++, STRINGIZE(fDiffByDlvrAddr));
		if(Flags & fDontInitSubstNames) flag_list.Add(id++, STRINGIZE(fDontInitSubstNames));
		if(Flags & fInitLocCount)       flag_list.Add(id++, STRINGIZE(fInitLocCount));
		if(Flags & fCalcRest)           flag_list.Add(id++, STRINGIZE(fCalcRest));
		if(Flags & fShowAllArticles)    flag_list.Add(id++, STRINGIZE(fShowAllArticles));
		if(Flags & fShowAllAgents)      flag_list.Add(id++, STRINGIZE(fShowAllAgents));
		if(Flags & fShowAllGoods)       flag_list.Add(id++, STRINGIZE(fShowAllGoods));
		if(Flags & fByZeroAgent)        flag_list.Add(id++, STRINGIZE(fByZeroAgent));
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}

	PutSggMembToBuf(Sgg, STRINGIZE(Sgg), rBuf);
	PutSgpMembToBuf(Sgp, STRINGIZE(Sgp), rBuf);
	PutSgdMembToBuf(Sgd, STRINGIZE(Sgd), rBuf);

	PutObjMembListToBuf(PPOBJ_BILL,     &BillList,     STRINGIZE(BillList),     rBuf);
	PutObjMembListToBuf(PPOBJ_BILL,     &RcptBillList, STRINGIZE(RcptBillList), rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION, &LocList,      STRINGIZE(LocList),      rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION, &ArList,       STRINGIZE(ArList),       rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION, &AgentList,    STRINGIZE(AgentList),    rBuf);
	return 1;
}
//
//
//
SLAPI TrfrAnlzViewItem_AlcRep::TrfrAnlzViewItem_AlcRep()
{
	Init();
}

void SLAPI TrfrAnlzViewItem_AlcRep::Init()
{
	Item.Clear();
	MEMSZERO(GoodsRec);
	GCPack.Init();
	GoodsStock.Init();
	MEMSZERO(GoodsExt);
	MEMSZERO(BillRec);
	MEMSZERO(OrgLotRec);
	PersonID = 0;
	OrgLot_Prsn_SupplID = 0;
	//IsImport = 0;
	//IsExport = 0;
	//IsManuf  = 0;
	//IsOptBuyer = 0;
	Flags = 0;
}
//
//
//
struct TagrCacheItem {
	// key: {Dt, GoodsID, PersonID, BillID, DlvrLocID}
	TagrCacheItem()
	{
		THISZERO();
	}
	long   ID__;
	long   Counter;
	LDATE  Dt;
	long   GoodsID;
	long   PersonID;
	long   ArticleID;
	long   BillID;
	long   DlvrLocID;
	long   LocCount;
	double Qtty;
	double PhQtty;
	double Cost;
	double Price;
	double Discount;
	double Income;
	double SaldoQtty;
	double SaldoAmt;
	double PVat;
	double Brutto;
	double LinkQtty;  // @v9.4.10
	double LinkCost;  // @v9.4.10
	double LinkPrice; // @v9.4.10
	double ExtVal1;   // @v9.3.5
	DBRowId DbPos;
};

IMPL_CMPFUNC(TagrCacheItem, i1, i2) { RET_CMPCASCADE5((const TagrCacheItem *)i1, (const TagrCacheItem *)i2, Dt, GoodsID, PersonID, BillID, DlvrLocID); }

TagrCacheItem & FASTCALL PPViewTrfrAnlz::GetCacheItem(uint pos) const
{
	return *(TagrCacheItem *)Cache.at(pos);
}

int SLAPI PPViewTrfrAnlz::FlashCacheItem(BExtInsert * pBei, const TagrCacheItem & rItem)
{
	int    ok = 1;
	if(!P_TrGrpngTbl)
		ok = -1;
	else if(rItem.DbPos) {
		TempTrfrGrpngTbl::Rec & r_rec = P_TrGrpngTbl->data;
		THROW_DB(P_TrGrpngTbl->getDirectForUpdate(-1, 0, rItem.DbPos));
#define CPY(f) r_rec.f = rItem.f
		CPY(ID__);
		CPY(Dt);
		CPY(GoodsID);
		CPY(PersonID);
		CPY(ArticleID);
		CPY(BillID);
		CPY(DlvrLocID);
		CPY(LocCount);
		CPY(Qtty);
		CPY(PhQtty);
		CPY(Cost);
		CPY(Price);
		CPY(Discount);
		CPY(Income);
		CPY(SaldoQtty);
		CPY(SaldoAmt);
		CPY(PVat);
		CPY(Brutto);
		CPY(LinkQtty);  // @v9.4.10
		CPY(LinkCost);  // @v9.4.10
		CPY(LinkPrice); // @v9.4.10
		CPY(ExtVal1);   // @v9.3.5
#undef CPY
		THROW_DB(P_TrGrpngTbl->updateRec()); // @sfu
	}
	else {
		TempTrfrGrpngTbl::Rec rec;
		MEMSZERO(rec);
#define CPY(f) rec.f = rItem.f
		CPY(ID__);  //
		CPY(Dt);
		CPY(GoodsID);
		CPY(PersonID);
		CPY(ArticleID);
		CPY(BillID);
		CPY(DlvrLocID);
		CPY(LocCount);
		CPY(Qtty);
		CPY(PhQtty);
		CPY(Cost);
		CPY(Price);
		CPY(Discount);
		CPY(Income);
		CPY(SaldoQtty);
		CPY(SaldoAmt);
		CPY(PVat);
		CPY(Brutto);
		CPY(LinkQtty);  // @v9.4.10
		CPY(LinkCost);  // @v9.4.10
		CPY(LinkPrice); // @v9.4.10
		CPY(ExtVal1);   // @v9.3.5
#undef CPY
		THROW_DB(pBei->insert(&rec));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTrfrAnlz::FlashCacheItems(uint count)
{
	int    ok = 1;
	if(!P_TrGrpngTbl)
		ok = -1;
	else {
		uint   i;
		BExtInsert bei(P_TrGrpngTbl);
		if(count) {
			PPIDArray lru_pos_array;
			LAssocArray lru_array;
			for(i = 0; i < Cache.getCount(); i++) {
				THROW_SL(lru_array.Add(GetCacheItem(i).Counter, i, 0));
			}
			lru_array.Sort();
			for(i = 0, count = MIN(count, lru_array.getCount()); i < count; i++) {
				uint pos = lru_array.at(i).Val;
				THROW(FlashCacheItem(&bei, GetCacheItem(pos)));
				THROW(lru_pos_array.add(pos));
			}
			lru_pos_array.sort();
			for(int rev_i = lru_pos_array.getCount() - 1; rev_i >= 0; rev_i--)
				Cache.atFree((uint)lru_pos_array.at(rev_i));
		}
		else {
			for(i = 0; i < Cache.getCount(); i++)
				THROW(FlashCacheItem(&bei, GetCacheItem(i)));
			Cache.clear();
		}
		THROW_DB(bei.flash());
	}
	CATCHZOK
	return ok;
}

SLAPI PPViewTrfrAnlz::PPViewTrfrAnlz() : PPView(0, &Filt, PPVIEW_TRFRANLZ), Cache(sizeof(TagrCacheItem), /*32,*/O_ARRAY),
	MaxCacheItems(DS.CheckExtFlag(ECF_SYSSERVICE) ? (128*1024) : (64*1024U)), CacheDelta(DS.CheckExtFlag(ECF_SYSSERVICE) ? 4096 : 2048),
	P_TrAnlzTbl(0), P_TrGrpngTbl(0), P_OrderTbl(0), P_IterOrderQuery(0), P_InnerIterItem(0), P_BObj(BillObj), Flags(0), GrpIdCounter(0)
{
	ImplementFlags |= (implDontSetupCtColumnsOnChgFilt|implUseServer);
	SETFLAG(Flags, fAccsCost, P_BObj->CheckRights(BILLRT_ACCSCOST));
}

SLAPI PPViewTrfrAnlz::~PPViewTrfrAnlz()
{
	delete P_IterOrderQuery;
	delete P_TrAnlzTbl;
	delete P_TrGrpngTbl;
	delete P_OrderTbl;
	delete P_InnerIterItem;
	if(!(BaseState & bsServerInst))
		DBRemoveTempFiles();
}

class TrfrAnlzCrosstab : public Crosstab {
public:
	SLAPI  TrfrAnlzCrosstab(PPViewTrfrAnlz * pV) : Crosstab(), P_V(pV)
	{
	}
	virtual BrowserWindow * SLAPI CreateBrowser(uint brwId, int dataOwner)
		{ return new PPViewBrowser(brwId, CreateBrowserQuery(), P_V, dataOwner); }
	virtual int  SLAPI GetTabTitle(const void * pVal, TYPEID typ, SString & rBuf) const
		{ return (pVal && P_V) ? P_V->GetTabTitle(*(const long *)pVal, rBuf) : 0; }
protected:
	PPViewTrfrAnlz * P_V;
};

int SLAPI PPViewTrfrAnlz::GetTabTitle(long tabID, SString & rBuf) const
{
	rBuf.Z();
	if(Filt.CtKind == TrfrAnlzFilt::ctDate) {
		LDATE dt = ZERODATE;
		dt.v = tabID;
		rBuf.Cat(dt);
	}
	else if(Filt.CtKind == TrfrAnlzFilt::ctCntragent)
		GetObjectName(PPOBJ_ARTICLE, tabID, rBuf);
	return 1;
}

int SLAPI PPViewTrfrAnlz::SerializeState(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	SString temp_buf;
	THROW(PPView::SerializeState(dir, rBuf, pCtx));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pCtx->Serialize(dir, InRest, rBuf));
	THROW_SL(pCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(CurViewOrd)), &CurViewOrd, 0, rBuf));
	THROW_SL(pCtx->Serialize(dir, &LocList, rBuf));
	// @v9.1.8 THROW_SL(pCtx->Serialize(dir, &GctRestList, rBuf)); // @v9.1.7
	THROW(GctRestList.Serialize(dir, rBuf, pCtx)); // @v9.1.8
	THROW(Gsl.Serialize(dir, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempTrfrAnlzTbl> (dir, &P_TrAnlzTbl, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempTrfrGrpngTbl> (dir, &P_TrGrpngTbl, rBuf, pCtx));
	THROW(SerializeDbTableByFileName <TempOrderTbl> (dir, &P_OrderTbl, rBuf, pCtx));
	if(dir > 0) {
		uint8 ind = P_Ct ? 0 : 1;
		THROW_SL(rBuf.Write(ind));
		if(P_Ct) {
			THROW(P_Ct->Write(rBuf, pCtx));
		}
	}
	else if(dir < 0) {
		uint8 ind = 0;
		ZDELETE(P_Ct);
		THROW_SL(rBuf.Read(ind));
		if(ind == 0 && P_TrGrpngTbl) {
			THROW_MEM(P_Ct = new TrfrAnlzCrosstab(this));
			THROW(P_Ct->Read(P_TrGrpngTbl, rBuf, pCtx));
		}
		Total.destroy();
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewTrfrAnlz::AllocInnerIterItem()
{
	SETIFZ(P_InnerIterItem, new TrfrAnlzViewItem);
	return P_InnerIterItem ? 1 : PPSetErrorNoMem();
}

const TrfrAnlzViewItem * PPViewTrfrAnlz::GetInnerIterItem() const
{
	return P_InnerIterItem;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempTrfrAnlzFile, TempTrfrAnlz);
PP_CREATE_TEMP_FILE_PROC(CreateTempTrfrGrpngFile, TempTrfrGrpng);

int SLAPI PPViewTrfrAnlz::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1, use_ta = 1;
	TrfrAnlzFilt prev_filt = Filt;
	BExtInsert * p_bei = 0;
	Flags &= ~(fAsGoodsCard | fShowSaldo);
	Cache.freeAll();
	PPWait(1);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	Filt.LotsPeriod.Actualize(ZERODATE);
	THROW(AdjustPeriodToRights(Filt.Period, 0));
	ZDELETE(P_InnerIterItem);
	if(!(Flags & fOnceInited) || !Filt.IsEqualExcept(prev_filt, TrfrAnlzFilt::eqxOrder|TrfrAnlzFilt::eqxCrosstab)) {
		// @v8.3.3 {
		PPUserFuncProfiler ufp(PPUPRF_VIEW_TRFRANLZ);
		double prf_measure = 0.0;
		// } @v8.3.3
		//
		// Инициализируем список складов, по которым следует поднимать выборку.
		// Учитываются доступные склады в правах доступа.
		//
		LocList.freeAll();
		if(Filt.LocList.GetCount() == 0) {
			PPObjLocation loc_obj;
			loc_obj.GetWarehouseList(&LocList);
		}
		else {
			const PPIDArray & r_loc_list = Filt.LocList.Get();
			for(uint i = 0; i < r_loc_list.getCount(); i++) {
				const PPID loc_id = r_loc_list.at(i);
				if(ObjRts.CheckLocID(loc_id, 0))
					LocList.add(loc_id);
			}
		}
		if(Filt.GoodsID && !Filt.OpID && oneof2(Filt.Grp, TrfrAnlzFilt::gNone, TrfrAnlzFilt::gGoodsBill))
			Flags |= fAsGoodsCard;
		else if(Filt.GoodsGrpID && ((Filt.Grp == TrfrAnlzFilt::gGoods && Filt.ArList.GetSingle()) ||
			(Filt.Grp == TrfrAnlzFilt::gCntragent && Filt.GoodsID)) && Filt.Sgg == sggNone && Filt.Sgp == sgpNone)
			Flags |= fShowSaldo;
		//
		if(Filt.Flags & TrfrAnlzFilt::fCalcAvgRest)
			Filt.Flags |= TrfrAnlzFilt::fCalcRest;
		if((Filt.Flags & TrfrAnlzFilt::fCalcRest) && ((Flags & fShowSaldo) || Filt.Sgg || !oneof2(Filt.Grp, TrfrAnlzFilt::gGoods, TrfrAnlzFilt::gGoodsDate)))
			Filt.Flags &= ~(TrfrAnlzFilt::fCalcRest|TrfrAnlzFilt::fCalcAvgRest);
		//
		if(Filt.CtKind == TrfrAnlzFilt::ctDate && !Filt.HasDateGrouping())
			Filt.CtKind = TrfrAnlzFilt::ctNone;
		else if(Filt.CtKind == TrfrAnlzFilt::ctCntragent && !Filt.HasCntragentGrouping())
			Filt.CtKind = TrfrAnlzFilt::ctNone;
		if(Filt.CtKind && Filt.CtValList.CheckID(TrfrAnlzFilt::ctvLocCount) > 0)
			Filt.Flags |= TrfrAnlzFilt::fInitLocCount;
		Total.destroy();
		GctRestList.freeAll(); // @v9.1.3
		ZDELETE(P_TrAnlzTbl);
		ZDELETE(P_TrGrpngTbl);
		ZDELETE(P_OrderTbl);
		LocCountList.freeAll();
		Gsl.Clear();
		Psp.Init(Filt.Sgp);
		GrpIdCounter = 0;
		Cf = PTR_CMPFUNC(TagrCacheItem);
		Cache.clear();
		LastCacheTouch = 0;
		SETFLAG(Psp.Flags, PPObjPerson::SubstParam::fSubstDlvrAddr, Filt.Flags & TrfrAnlzFilt::fSubstDlvrAddr);
		SETFLAG(Psp.Flags, PPObjPerson::SubstParam::fSubstPersonRAddr, Filt.Flags & TrfrAnlzFilt::fSubstPersonRAddr);
		CurViewOrd = CurIterOrd = (IterOrder)Filt.InitOrd;
		{
			SString msg_buf;
			TransferTbl::Rec rec;
			BillTbl::Rec bill_rec;
			GCTIterator::ItemExtension gct_ext;
			GCTFilt gct_filt;
			gct_filt.Period  = Filt.Period;
			gct_filt.LotsPeriod = Filt.LotsPeriod;
			gct_filt.LocList.Set(&LocList);
			gct_filt.BillList = Filt.BillList;
			gct_filt.ArList   = Filt.ArList;
			gct_filt.AgentList = Filt.AgentList;
			gct_filt.OpID    = Filt.OpID;
			gct_filt.SupplID = Filt.SupplID;
			gct_filt.ArID_   = Filt.ArID_;
			gct_filt.DlvrAddrID = Filt.DlvrAddrID;
			gct_filt.AgentID_   = Filt.AgentID_;
			gct_filt.GoodsGrpID = Filt.GoodsGrpID;
			gct_filt.GoodsID    = Filt.GoodsID;
			gct_filt.BrandID    = Filt.BrandID;
			gct_filt.SupplAgentID = Filt.SupplAgentID;
			SETFLAG(gct_filt.Flags, OPG_LABELONLY,   Filt.Flags & TrfrAnlzFilt::fLabelOnly);
			SETFLAG(gct_filt.Flags, OPG_BYZEROAGENT, Filt.Flags & TrfrAnlzFilt::fByZeroAgent);
			SETFLAG(gct_filt.Flags, OPG_FORCEBILLCACHE, 1);
			SETFLAG(gct_filt.Flags, OPG_BYZERODLVRADDR, Filt.Flags & TrfrAnlzFilt::fByZeroDlvrAddr);
			SETFLAG(gct_filt.Flags, OPG_SKIPNOUPDLOTREST, 1); // @v8.9.0
			SETFLAG(gct_filt.Flags, OPG_STOREDAYLYRESTS, Filt.Flags & TrfrAnlzFilt::fCalcRest); // @v9.1.3
			// @v9.4.10 {
			if(GCTIterator::AnalyzeOp(Filt.OpID, 0) & (GCTIterator::aorfThereAreDrafts|GCTIterator::aorfThereAreOrders)) {
				SETFLAG(gct_filt.Flags, OPG_COMPAREWROFF, Filt.Flags & TrfrAnlzFilt::fCmpWrOff);
			}
			// } @v9.4.10
			if(Filt.Flags & TrfrAnlzFilt::fShowAllArticles)
				gct_filt.SoftRestrict = srArticle;
			else if(Filt.Flags & TrfrAnlzFilt::fShowAllAgents)
				gct_filt.SoftRestrict = srAgent;
			else if(Filt.Flags & TrfrAnlzFilt::fShowAllGoods)
				gct_filt.SoftRestrict = srGoods;
			else
				gct_filt.SoftRestrict = srNone;
			GCTIterator gctiter(&gct_filt, &Filt.Period);
			ZDELETE(P_TrAnlzTbl);
			ZDELETE(P_TrGrpngTbl);
			if(Flags & fAsGoodsCard) {
				//
				// Рассчитываем входящий остаток
				//
				if((Filt.Flags & TrfrAnlzFilt::fLabelOnly)) {
					InRest = 0.0;
				}
				else if(Filt.ArList.GetSingle()) {
					LDATE  start_dt = Filt.Period.low;
					if(start_dt)
						start_dt = plusdate(start_dt, -1);
					GetSaldo(Filt.GoodsID, Filt.ArList.GetSingle(), Filt.DlvrAddrID, start_dt, &InRest, 0);
				}
				else
					THROW(CalcInRest(Filt.GoodsID, &InRest));
			}
			if(Filt.Grp) {
				THROW(P_TrGrpngTbl = CreateTempTrfrGrpngFile());
			}
			else {
				THROW(P_TrAnlzTbl = CreateTempTrfrAnlzFile());
				THROW_MEM(p_bei = new BExtInsert(P_TrAnlzTbl, 28*1024));
			}
			{
				PPTransaction tra(ppDbDependTransaction, use_ta);
				THROW(tra);
				if(gctiter.First(&rec, &bill_rec, &gct_ext) > 0) {
					LDATE  prev_dt = ZERODATE;
					TransferTbl::Rec temp_rec;
					BillTbl::Rec temp_bill_rec;
					double sales = 0.0, income = 0.0;
					double d_cost = 0.0;
					double d_price = 0.0;
					long   oprno = 0;
					do {
						int    skip = 0;
						THROW(PPCheckUserBreak());
						prf_measure += 1.0; // @v8.3.3
						if(Filt.PsnCatID || Filt.CityID) {
							PersonTbl::Rec p_rec;
							MEMSZERO(p_rec);
							const PPID psn_id = ObjectToPerson(bill_rec.Object);
							if(psn_id && PsnObj.Fetch(psn_id, &p_rec) > 0) {
								if(Filt.PsnCatID)
									skip = (p_rec.CatID == Filt.PsnCatID) ? 0 : 1;
								if(!skip && Filt.CityID) {
									int    opt = PSNGETADDRO_DEFAULT;
									PPID   addr_id = 0, dlvr_addr_id = 0;
									PPID   city_id = 0;
									skip = 1;
									GetDlvrLocID(&bill_rec, &dlvr_addr_id);
									if(Filt.Flags & TrfrAnlzFilt::fSubstDlvrAddr)
										opt = PSNGETADDRO_DLVRADDR;
									else if(Filt.Flags & TrfrAnlzFilt::fSubstPersonRAddr)
										opt = PSNGETADDRO_REALADDR;
									if(PsnObj.GetAddrID(p_rec.ID, dlvr_addr_id, opt, &addr_id) > 0)
										if(PsnObj.GetCityByAddr(addr_id, &city_id, 0, 1) > 0)
											skip = (city_id == Filt.CityID) ? 0 : 1;
								}
							}
							else
								skip = 1;
						}
						if(!skip && !Filt.RcptBillList.IsEmpty()) {
							skip = 1;
							if(rec.LotID) {
								ReceiptTbl::Rec org_lot_rec;
								THROW(P_BObj->trfr->Rcpt.SearchOrigin(rec.LotID, 0, 0, &org_lot_rec));
								skip = (Filt.RcptBillList.Search(org_lot_rec.BillID, 0) > 0) ? 0 : 1;
							}
						}
						if(!skip) {
							PPWaitMsg(PPObjBill::MakeCodeString(&bill_rec, 0, msg_buf));
							if(Filt.Flags & TrfrAnlzFilt::fGByDate && Flags & fAsGoodsCard) {
								double aqtty = fabs(rec.Quantity);
								LDATE  _date = rec.Dt;
								ShrinkSubstDate(Filt.Sgd, rec.Dt, &_date);
								if(prev_dt != _date) {
									if(prev_dt) {
										temp_rec.Dt = prev_dt;
										temp_rec.Quantity = income;
										temp_bill_rec.CRate = fabs(sales);
										temp_rec.Cost  = fabs(fdivnz(d_cost, income));
										temp_rec.Price = fabs(fdivnz(d_price, sales));
										THROW(Add(p_bei, &oprno, &temp_rec, &temp_bill_rec, &gct_ext));
									}
									prev_dt  = _date;
									temp_rec = rec;
									temp_bill_rec = bill_rec;
									d_cost = d_price = 0.0;
									if(rec.Quantity > 0.0) {
										sales = 0.0;
										income = aqtty;
										d_cost = aqtty * fabs(rec.Cost);
									}
									else {
										sales   = aqtty;
										income  = 0.0;
										d_price = aqtty * fabs(rec.Price - rec.Discount);
									}
								}
								else {
									if(rec.Quantity > 0.0) {
										income += aqtty;
										d_cost += aqtty * fabs(rec.Cost);
									}
									else {
										sales += aqtty;
										d_price += aqtty * fabs(rec.Price - rec.Discount);
									}
									temp_rec.Rest = rec.Rest;
								}
							}
							else {
								int    res;
								THROW(res = Add(p_bei, &oprno, &rec, &bill_rec, &gct_ext));
								if(res < 0)
									break;
							}
						}
					} while(gctiter.Next(&rec, &bill_rec, &gct_ext) > 0);
					if(Filt.Flags & TrfrAnlzFilt::fGByDate && Flags & fAsGoodsCard) {
						temp_rec.Quantity = income;
						temp_bill_rec.CRate = fabs(sales);
						temp_rec.Cost  = fabs(fdivnz(d_cost, income));
						temp_rec.Price = fabs(fdivnz(d_price, sales));
						THROW(Add(p_bei, &oprno, &temp_rec, &temp_bill_rec, 0));
					}
				}
				if(p_bei)
					THROW_DB(p_bei->flash());
				THROW(FlashCacheItems(0));
				if(Flags & fShowSaldo) {
					PPWait(1);
					THROW(AddAbsentSaldo());
				}
				if(!(Filt.Flags & TrfrAnlzFilt::fDontInitSubstNames)) {
					THROW(InitGrpngNames());
					THROW(CreateOrderTable((IterOrder)Filt.InitOrd));
				}
				THROW(tra.Commit());
			}
			if(Filt.Flags & TrfrAnlzFilt::fCalcRest) {
				const GCTIterator::GoodsRestArray * p_gr_list = gctiter.GetGoodsRestList();
				if(p_gr_list) {
					GctRestList = *p_gr_list;
					if(Filt.Period.low) {
						const LDATE beg_rest_date = plusdate(Filt.Period.low, -1);
						PPViewGoodsRest gr_view;
						GoodsRestFilt gr_filt;
						GoodsRestViewItem gr_item;
						//
						gr_filt.Date = beg_rest_date;
						gr_filt.LocList = Filt.LocList;
						gr_filt.GoodsGrpID = Filt.GoodsGrpID;
						gr_filt.BrandID = Filt.BrandID;
						gr_filt.Flags |= GoodsRestFilt::fEachLocation;
						THROW(gr_view.Init_(&gr_filt));
						for(gr_view.InitIteration(PPViewGoodsRest::OrdByDefault); gr_view.NextIteration(&gr_item) > 0;) {
                            GctRestList.SetInitRest(gr_item.GoodsID, gr_item.LocID, gr_item.Rest);
						}
						GctRestList.Finish();
					}
				}
			}
			Flags |= fOnceInited;
		}
		// @v8.3.3 {
		ufp.SetFactor(0, prf_measure);
		ufp.Commit();
		// } @v8.3.3
	}
	else {
		if(!(Filt.Flags & TrfrAnlzFilt::fDontInitSubstNames)) {
			PPTransaction tra(ppDbDependTransaction, use_ta);
			THROW(tra);
			THROW(CreateOrderTable((IterOrder)Filt.InitOrd));
			THROW(tra.Commit());
		}
	}
	{
		ZDELETE(P_Ct);
		TempTrfrGrpngTbl * p_tgt = P_TrGrpngTbl;
		if(p_tgt && Filt.CtKind != TrfrAnlzFilt::ctNone) {
			int    setup_total = 0;
			SString temp_buf;
			DBFieldList total_list;
			StringSet total_title_list;
			THROW_MEM(P_Ct = new TrfrAnlzCrosstab(this));
			if(Filt.CtKind == TrfrAnlzFilt::ctDate)
				P_Ct->SetTable(p_tgt, p_tgt->Dt);
			else if(Filt.CtKind == TrfrAnlzFilt::ctCntragent)
				P_Ct->SetTable(p_tgt, p_tgt->ArticleID);
			/*
			else if(Filt.CtKind == TrfrAnlzFilt::ctLocation)
				P_Ct->SetTable(p_tgt, p_tgt->Dt);
			*/
			P_Ct->AddIdxField(p_tgt->GoodsID);
			P_Ct->AddIdxField(p_tgt->PersonID);
			if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)
				P_Ct->AddIdxField(p_tgt->DlvrLocID);
			P_Ct->AddInheritedFixField(p_tgt->GoodsText);
			P_Ct->AddInheritedFixField(p_tgt->PersonText);
			if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvQtty) > 0) {
				P_Ct->AddAggrField(p_tgt->Qtty);
				total_list.Add(p_tgt->Qtty);
				total_title_list.add(GetCtColumnTitle(TrfrAnlzFilt::ctvQtty, temp_buf));
			}
			if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvCost) > 0) {
				P_Ct->AddAggrField(p_tgt->Cost);
				total_list.Add(p_tgt->Cost);
				total_title_list.add(GetCtColumnTitle(TrfrAnlzFilt::ctvCost, temp_buf));
			}
			if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvNetPrice) > 0) {
				P_Ct->AddAggrField(p_tgt->Price);
				total_list.Add(p_tgt->Price);
				total_title_list.add(GetCtColumnTitle(TrfrAnlzFilt::ctvNetPrice, temp_buf));
			}
			if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctMargin) > 0 || Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctIncome) > 0) {
				P_Ct->AddAggrField(p_tgt->Income);
			}
			else if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvIncome) > 0) {
				P_Ct->AddAggrField(p_tgt->Income);
				total_list.Add(p_tgt->Income);
				total_title_list.add(GetCtColumnTitle(TrfrAnlzFilt::ctvIncome, temp_buf));
			}
			if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvLocCount) > 0) {
				P_Ct->AddAggrField(p_tgt->LocCount);
				total_list.Add(p_tgt->LocCount);
				total_title_list.add(GetCtColumnTitle(TrfrAnlzFilt::ctvLocCount, temp_buf));
			}
			if(Filt.Grp == TrfrAnlzFilt::gGoodsDate)
				P_Ct->SetSortIdx("GoodsText", 0L);
			else if(Filt.Grp == TrfrAnlzFilt::gCntragentDate)
				P_Ct->SetSortIdx("PersonText", 0L);
			else if(Filt.InitOrd == PPViewTrfrAnlz::OrdByGoods)
				P_Ct->SetSortIdx("GoodsText", 0L);
			else if(Filt.InitOrd == PPViewTrfrAnlz::OrdByArticle)
				P_Ct->SetSortIdx("PersonText", 0L);
			else
				P_Ct->SetSortIdx("GoodsText", 0L);
			if(total_list.GetCount()) {
				PPGetWord(PPWORD_TOTAL, 0, temp_buf);
				P_Ct->AddTotalRow(total_list, 0, temp_buf);
				uint ss_pos = 0;
				for(uint i = 0; i < total_list.GetCount(); i++) {
					total_title_list.get(&ss_pos, temp_buf.Z());
					P_Ct->AddTotalColumn(total_list.Get(i), 0, temp_buf);
				}
			}
			THROW(P_Ct->Create(use_ta));
		}
	}
	CATCH
		delete p_bei;
		ZDELETE(P_Ct);
		ZDELETE(P_TrGrpngTbl);
		ZDELETE(P_TrAnlzTbl);
		ZDELETE(P_OrderTbl);
		ok = 0;
	ENDCATCH
	Cache.freeAll();
	PPWait(0);
	return ok;
}

int SLAPI PPViewTrfrAnlz::CalcInRest(PPID goodsID, double * pRest)
{
	int    ok = 1;
	GoodsRestParam gp;
	gp.CalcMethod = GoodsRestParam::pcmSum;
	Filt.LocList.CopyTo(&gp.LocList);
	gp.GoodsID    = goodsID;
	gp.SupplID    = Filt.SupplID;
	gp.Date       = plusdate(Filt.Period.low, -1);
	if(P_BObj->trfr->GetRest(&gp)) {
		ASSIGN_PTR(pRest, gp.Total.Rest);
	}
	else {
		ASSIGN_PTR(pRest, 0);
		ok = 0;
	}
	return ok;
}
//
//
//
int SLAPI PPViewTrfrAnlz::GetDlvrLocID(const BillTbl::Rec * pBillRec, PPID * pDlvrLocID)
{
	PPID   dlvr_loc_id = 0;
	int    intr = IsIntrOp(pBillRec->OpID);
	if(intr == INTREXPND)
		dlvr_loc_id = PPObjLocation::ObjToWarehouse(pBillRec->Object);
	else if(intr == INTRRCPT)
		dlvr_loc_id = pBillRec->LocID;
	else {
		PPFreight freight;
		if(pBillRec->Flags & BILLF_FREIGHT && P_BObj->P_Tbl->GetFreight(pBillRec->ID, &freight) > 0)
			dlvr_loc_id = freight.DlvrAddrID;
	}
	ASSIGN_PTR(pDlvrLocID, dlvr_loc_id);
	return dlvr_loc_id ? 1 : -1;
}

int SLAPI PPViewTrfrAnlz::InitDlvrLocID(const BillTbl::Rec * pBillRec, PPID * pDlvrLocID, TempTrfrGrpngTbl::Rec * pRec)
{
	if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) {
		if(*pDlvrLocID < 0)
			GetDlvrLocID(pBillRec, pDlvrLocID);
		pRec->DlvrLocID = *pDlvrLocID;
	}
	return (*pDlvrLocID > 0) ? 1 : -1;
}

int SLAPI PPViewTrfrAnlz::GetSaldo(PPID goodsID, PPID arID, PPID dlvrLocID, LDATE dt, double * pSaldoQtty, double * pSaldoAmt)
{
	return P_BObj->GetGoodsSaldo(goodsID, arID, dlvrLocID, dt, 0, pSaldoQtty, pSaldoAmt);
}

int SLAPI PPViewTrfrAnlz::Add(BExtInsert * pBei, long * pOprNo, TransferTbl::Rec * pTrfrRec, BillTbl::Rec * pBillRec, GCTIterator::ItemExtension * pExt)
{
	int    ok = 1;
	double phuperu = 0.0;
	PersonTbl::Rec psn_rec;
	PPID   bill_agent_id = 0;
	PPBillExt billext_rec;
	PPFreight freight;
	PPGoodsTaxEntry gtx;
	PROFILE_START
	if(Filt.Grp == TrfrAnlzFilt::gDateCntragentAgentGoods || Filt.Sgp == sgpBillAgent) {
		if(Filt.Flags & (TrfrAnlzFilt::fShowAllArticles|TrfrAnlzFilt::fShowAllGoods) && (Filt.Flags & TrfrAnlzFilt::fByZeroAgent || Filt.AgentList.GetSingle())) {
			//
			// При указанных условиях GCTIterator обработал ограничение Filt.AgentID как мягкое.
			// То есть, все записи, которые не соответствуют этому ограничению, имеют
			// нулевые величины, но присутствуют в выборке. Все, что осталось сделать -
			// это фиктивно обозначить в каждой из этих записей агента как Filt.AgentID.
			//
			bill_agent_id = (Filt.Flags & TrfrAnlzFilt::fByZeroAgent) ? 0 : Filt.AgentList.GetSingle();
		}
		else if(P_BObj->FetchExt(pBillRec->ID, &billext_rec) > 0)
			bill_agent_id = billext_rec.AgentID;
	}
	else if(Filt.Sgp == sgpVesselAgent && P_BObj->P_Tbl->GetFreight(pBillRec->ID, &freight) > 0) {
		bill_agent_id = freight.AgentID;
	}
	const  int  is_vessel_agent = BIN(Filt.Sgp == sgpVesselAgent);
	const  PPID goods_id = labs(pTrfrRec->GoodsID);
	// @v8.3.3 PPID   suppl_id = 0;
	const  PPID ar_id  = is_vessel_agent ? 0 : (Filt.Sgp == sgpBillAgent ? bill_agent_id : pBillRec->Object);
	const  PPID psn_id = is_vessel_agent ? bill_agent_id : ObjectToPerson(ar_id);
	PPID   dlvr_loc_id = -1;
	PPID   _psn_id = 0;
	PPID   _ar_id = ar_id;
	PPID   _goods_id = goods_id;
	LDATE  _dt = pBillRec->Dt;
	long   _oprno = pTrfrRec->OprNo;
	const  double qtty = (pTrfrRec->Flags & PPTFR_REVAL) ? pTrfrRec->Rest : fabs(pTrfrRec->Quantity);
	const  int    sign = (pTrfrRec->Quantity < 0) ? -1 : 1;
	double cost = (Flags & fAccsCost) ? (TR5(pTrfrRec->Cost) * qtty) : 0.0;
	double price;
	double discount;
	double pvat = 0.0;
	// @v9.5.8 {
	int    skip = 0;
	if(Filt.Flags & TrfrAnlzFilt::fCmpWrOff && Filt.Flags & TrfrAnlzFilt::fCmpWrOff_DiffOnly && pExt) {
		if(feqeps(fabs(pExt->LinkQtty), fabs(qtty), 1.0E-7))
			skip = 1;
	}
	if(!skip) {
	// } @v9.5.8
		// @v9.3.4 {
		double ext_val1 = 0.0;
		{
			const long ext_val_param = Filt.ExtValueParam[0];
			if(oneof2(ext_val_param, Filt.extvLinkOrderDiscount, Filt.extvLinkOrderPriceAbove)) {
				PPID   ord_lot_id = 0;
				if(P_BObj->GetOrderLotForTransfer(*pTrfrRec, &ord_lot_id) > 0) {
					assert(ord_lot_id);
					DateIter di;
					TransferTbl::Rec ord_rec;
					if(P_BObj->trfr->EnumByLot(ord_lot_id, &di, &ord_rec) > 0) {
						if(TESTMULTFLAG(ord_rec.Flags, (PPTFR_ORDER|PPTFR_RECEIPT))) {
							if(ext_val_param == Filt.extvLinkOrderDiscount) {
								if(ord_rec.Discount != 0.0) {
									const double ord_qtty = fabs(ord_rec.Quantity);
									const double ord_price = fabs(ord_rec.Price) * ord_qtty;
									const double ord_dis   = ord_rec.Discount * ord_qtty;
									double ord_pct_dis = (ord_price > 0.0 && ord_dis > 0.0) ? R6(ord_dis / ord_price) : 0.0;
									if(ord_pct_dis > 0.0)
										ext_val1 = (pTrfrRec->Price - pTrfrRec->Discount) * ord_pct_dis * qtty / (1.0 - ord_pct_dis);
								}
							}
							else if(ext_val_param == Filt.extvLinkOrderPriceAbove) {
								const double ord_price = fabs(ord_rec.Price - ord_rec.Discount);
								const double sale_price = fabs(pTrfrRec->Price - pTrfrRec->Discount);
								if(ord_price > sale_price) {
                                    ext_val1 = (ord_price - sale_price) * qtty;
								}
							}
						}
					}
				}
			}
			// @v9.5.0 {
			else if(ext_val_param > Filt.extvQuotBias) {
				const QuotIdent qi(pTrfrRec->Dt, pTrfrRec->LocID, ext_val_param-Filt.extvQuotBias, pTrfrRec->CurID, pBillRec->Object); // @v9.5.3 pTrfrRec->Dt
				double _ext_quot = 0.0;
				if(GObj.GetQuotExt(goods_id, qi, pTrfrRec->Cost, pTrfrRec->Price, &_ext_quot, 1) > 0) {
					ext_val1 = _ext_quot * qtty * sign;
				}
			}
			// } @v9.5.0
		}
		// } @v9.3.4
		if(Flags & fAsGoodsCard && Filt.Flags & TrfrAnlzFilt::fGByDate) {
			//
			// В случае расчета карточки товара с группировкой по дате price представляет
			// среднюю цену расхода (в ценах реализации).
			// При этом количество расхода установлено в поле pBillRec->CRate
			//
			price = TR5(pTrfrRec->Price) * fabs(pBillRec->CRate);
			discount = 0.0;
		}
		else {
			price = TR5(pTrfrRec->Price) * qtty;
			discount = TR5(pTrfrRec->Discount) * qtty;
		}
		//
		// Рассчитываем налоги либо поправляем цены, в случае, если они указаны без налогов
		//
		if((Filt.Flags & (TrfrAnlzFilt::fCalcVat|TrfrAnlzFilt::fCWoVat) || (pTrfrRec->Flags & (PPTFR_COSTWOVAT|PPTFR_PRICEWOTAXES)))) {
			Goods2Tbl::Rec grec;
			if(GObj.Fetch(goods_id, &grec) > 0) {
				double tax_factor = qtty;
				GObj.MultTaxFactor(goods_id, &tax_factor);
				if(((Filt.Flags & TrfrAnlzFilt::fCWoVat) || (pTrfrRec->Flags & PPTFR_COSTWOVAT)) && (Flags & fAccsCost)) {
					LDATE  lot_date = ZERODATE;
					PPID   in_tax_grp_id = 0;
					int    vat_free = 0;
					ReceiptTbl::Rec org_lot_rec;
					if(pTrfrRec->LotID) {
						if(P_BObj->trfr->Rcpt.SearchOrigin(pTrfrRec->LotID, 0, 0, &org_lot_rec) > 0) {
							lot_date = org_lot_rec.Dt;
							in_tax_grp_id = org_lot_rec.InTaxGrpID;
							vat_free = IsLotVATFree(org_lot_rec);
						}
					}
					if(pTrfrRec->Flags & PPTFR_COSTWOVAT)
						GObj.AdjCostToVat(in_tax_grp_id, grec.TaxGrpID, lot_date, tax_factor, &cost, 1, vat_free);
					if(Filt.Flags & TrfrAnlzFilt::fCWoVat) {
						//
						// Расчет цены поступления без НДС
						//
						if(GObj.GTxObj.Fetch(NZOR(in_tax_grp_id, grec.TaxGrpID), pTrfrRec->Dt, 0, &gtx) > 0) {
							GTaxVect vect;
							vect.Calc_(&gtx, cost, tax_factor, ~GTAXVF_SALESTAX, (vat_free ? GTAXVF_VAT : 0));
							cost -= vect.GetValue(GTAXVF_VAT);
						}
					}
				}
				if(Filt.Flags & (TrfrAnlzFilt::fCalcVat|TrfrAnlzFilt::fCWoVat) || pTrfrRec->Flags & PPTFR_PRICEWOTAXES) {
					if(GObj.GTxObj.Fetch(grec.TaxGrpID, pTrfrRec->Dt, pBillRec->OpID, &gtx) > 0) {
						if(pTrfrRec->Flags & PPTFR_PRICEWOTAXES) {
							price = price - discount;
							discount = 0.0;
							GObj.AdjPriceToTaxes(gtx.TaxGrpID, tax_factor, &price, 1);
						}
						if(Filt.Flags & (TrfrAnlzFilt::fCalcVat|TrfrAnlzFilt::fCWoVat)) {
							const long amt_fl = (CConfig.Flags & CCFLG_PRICEWOEXCISE) ? ~GTAXVF_SALESTAX : GTAXVF_BEFORETAXES;
							GTaxVect vect;
							vect.Calc_(&gtx, fabs(price - discount), tax_factor, amt_fl, 0);
							if(Filt.Flags & TrfrAnlzFilt::fCalcVat) {
								pvat = vect.GetValue(GTAXVF_VAT);
							}
							if(Filt.Flags & TrfrAnlzFilt::fCWoVat) {
								//
								// Расчет цены реализации без НДС
								//
								price = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE);
								discount = 0.0;
							}
						}
					}
				}
			}
		}
		cost     *= sign;
		price    *= sign;
		discount *= sign;
		pvat     *= sign;
		{
			PPObjGoods::SubstBlock sgg_blk; // @v8.3.3
			sgg_blk.ExclParentID = Filt.GoodsGrpID;
			sgg_blk.LocID = pTrfrRec->LocID;
			sgg_blk.LotID = pTrfrRec->LotID; // @v8.3.4
			/* @v8.3.4
			if(Filt.Sgg == sggSuppl) {
				ReceiptTbl::Rec lot_rec;
				if(P_BObj->trfr->Rcpt.Search(pTrfrRec->LotID, &lot_rec) > 0)
					sgg_blk.SupplID = lot_rec.SupplID;
			}
			else if(Filt.Sgg == sggSupplAgent)
				sgg_blk.SupplID = P_BObj->GetSupplAgent(pTrfrRec->LotID);
			*/
			THROW(GObj.SubstGoods(goods_id, &_goods_id, Filt.Sgg, &sgg_blk, &Gsl));
		}
		if(Filt.Flags & TrfrAnlzFilt::fInitLocCount && dlvr_loc_id < 0) {
			if(Filt.Flags & (TrfrAnlzFilt::fDiffByDlvrAddr|TrfrAnlzFilt::fSubstDlvrAddr))
				GetDlvrLocID(pBillRec, &dlvr_loc_id);
			if(dlvr_loc_id <= 0 && PsnObj.Fetch(psn_id, &psn_rec) > 0) {
				if(Filt.Flags & (TrfrAnlzFilt::fSubstPersonRAddr|TrfrAnlzFilt::fSubstDlvrAddr))
					dlvr_loc_id = psn_rec.RLoc;
				if(dlvr_loc_id <= 0)
					dlvr_loc_id = psn_rec.MainLoc;
			}
		}
		//
		// Если условия фильтра предполагают детализацию (Контрагент-Адрес доставки), то подстановка персоналии невозможна.
		//
		if(Filt.Sgp && !(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)) {
			if(Filt.Flags & TrfrAnlzFilt::fSubstDlvrAddr && dlvr_loc_id < 0)
				GetDlvrLocID(pBillRec, &dlvr_loc_id);
			PsnObj.Subst(ar_id ? (ar_id | sgpArticleMask) : psn_id, dlvr_loc_id, &Psp, PSNSUBSTF_LASTRELINHIERARH, &_psn_id);
			if(_psn_id && psn_id != _psn_id && ar_id) {
				ArticleTbl::Rec ar_rec;
				if(ArObj.Fetch(ar_id, &ar_rec) > 0 && ar_rec.AccSheetID) {
					PPID   ar2_id = 0;
					if(ArObj.P_Tbl->PersonToArticle(_psn_id, ar_rec.AccSheetID, &ar2_id) > 0)
						_ar_id = ar2_id;
				}
			}
		}
		else
			_psn_id = NZOR(psn_id, (ar_id | sgpArticleMask));
		if(ShrinkSubstDate(Filt.Sgd, _dt, &_dt) > 0)
			_oprno = ++(*pOprNo);
		if(Filt.Grp) {
			TempTrfrGrpngTbl::Key1 k;
			TempTrfrGrpngTbl::Rec tg_rec;
			MEMSZERO(tg_rec);
			double rest = 0.0;
			tg_rec.Qtty      = pTrfrRec->Quantity;
			if(GObj.GetPhUPerU(goods_id, 0, &phuperu) > 0)
				tg_rec.PhQtty = tg_rec.Qtty * phuperu;
			tg_rec.Cost      = cost;
			tg_rec.Price     = price;
			tg_rec.Discount  = discount;
			tg_rec.Price    -= tg_rec.Discount;
			tg_rec.PVat      = pvat;
			// @v9.4.10 {
			if(pExt) {
				tg_rec.LinkQtty  = pExt->LinkQtty;
				tg_rec.LinkCost  = pExt->LinkCost;
				tg_rec.LinkPrice = pExt->LinkPrice;
			}
			// } @v9.4.10
			tg_rec.ExtVal1   = ext_val1; // @v9.3.5
			tg_rec.ArticleID = _ar_id;
			tg_rec.GoodsID   = _goods_id;
			tg_rec.PersonID  = _psn_id;
			tg_rec.Dt        = _dt;
			MEMSZERO(k);
			if(Filt.Grp == TrfrAnlzFilt::gGoods) {
				k.GoodsID = _goods_id;
				tg_rec.PersonID = 0;
				tg_rec.Dt = ZERODATE;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gGoodsDate) {
				k.GoodsID = _goods_id;
				k.Dt      = _dt;
				tg_rec.PersonID = 0;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gCntragent) {
				k.PersonID = _psn_id;
				tg_rec.GoodsID = 0;
				tg_rec.Dt = ZERODATE;
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				k.DlvrLocID = tg_rec.DlvrLocID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gCntragentDate) {
				k.PersonID = _psn_id;
				tg_rec.GoodsID = 0;
				k.Dt = _dt;
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				k.DlvrLocID = tg_rec.DlvrLocID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gGoodsCntragent) {
				k.GoodsID  = _goods_id;
				k.PersonID = _psn_id;
				tg_rec.Dt = ZERODATE;
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				k.DlvrLocID = tg_rec.DlvrLocID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gGoodsCntragentDate) {
				k.GoodsID  = _goods_id;
				k.PersonID = _psn_id;
				k.Dt = _dt;
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				k.DlvrLocID = tg_rec.DlvrLocID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gGoodsBill) {
				tg_rec.BillID = pBillRec->ID;
				k.GoodsID = _goods_id;
				k.PersonID = _psn_id;
				k.Dt = _dt;
				k.BillID = pBillRec->ID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gDateCntragentAgentGoods) {
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				if(tg_rec.DlvrLocID == 0 && psn_id && PsnObj.Fetch(psn_id, &psn_rec) > 0)
					tg_rec.DlvrLocID = NZOR(psn_rec.RLoc, psn_rec.MainLoc);
				tg_rec.BillID = bill_agent_id;
				k.GoodsID  = _goods_id;
				k.PersonID = _psn_id;
				k.Dt       = _dt;
				k.BillID   = tg_rec.BillID;
				k.DlvrLocID = tg_rec.DlvrLocID;
			}
			else if(Filt.Grp == TrfrAnlzFilt::gBillCntragent) {
				k.PersonID = _psn_id;
				k.BillID   = pBillRec->ID;
				k.Dt       = pBillRec->Dt;
				InitDlvrLocID(pBillRec, &dlvr_loc_id, &tg_rec);
				k.DlvrLocID = tg_rec.DlvrLocID;
				tg_rec.BillID  = pBillRec->ID;
				tg_rec.GoodsID = 0;
				tg_rec.Dt      = pBillRec->Dt;
			}
			{
				TempTrfrGrpngTbl * p_tgt = P_TrGrpngTbl;
				TagrCacheItem ctest, citem;
				uint   pos = 0;
				int    r = 0;
				ctest.Dt = tg_rec.Dt;
				ctest.GoodsID = tg_rec.GoodsID;
				ctest.PersonID = tg_rec.PersonID;
				ctest.BillID = tg_rec.BillID;
				ctest.DlvrLocID = tg_rec.DlvrLocID;
				if(Cache.bsearch(&ctest, &pos, Cf)) {
					r = 1;
				}
				else if(p_tgt->search(1, &k, spEq)) {
	#define CPY(f) citem.f = p_tgt->data.f
					CPY(ID__);
					CPY(Dt);
					CPY(GoodsID);
					CPY(PersonID);
					CPY(ArticleID);
					CPY(BillID);
					CPY(DlvrLocID);
					CPY(LocCount);
					CPY(Qtty);
					CPY(PhQtty);
					CPY(Cost);
					CPY(Price);
					CPY(Discount);
					CPY(Income);
					CPY(SaldoQtty);
					CPY(SaldoAmt);
					CPY(PVat);
					CPY(Brutto);    // @v8.7.7
					CPY(LinkQtty);  // @v9.4.10
					CPY(LinkCost);  // @v9.4.10
					CPY(LinkPrice); // @v9.4.10
					CPY(ExtVal1);   // @v9.3.5
	#undef CPY
					citem.Counter = 0;
					p_tgt->getPosition(&citem.DbPos);
					if(Cache.getCount() >= MaxCacheItems)
						THROW(FlashCacheItems(CacheDelta));
					THROW_SL(Cache.ordInsert(&citem, &pos, Cf));
					r = 1;
				}
				if(r > 0) {
					TagrCacheItem & r_citem = GetCacheItem(pos);
					r_citem.Qtty    += tg_rec.Qtty;
					r_citem.PhQtty  += tg_rec.PhQtty;
					r_citem.Cost    += tg_rec.Cost;
					r_citem.Price   += tg_rec.Price;
					r_citem.PVat    += tg_rec.PVat;
					r_citem.Brutto  += tg_rec.Brutto; // @v8.7.7
					r_citem.LinkQtty  += tg_rec.LinkQtty;  // @v9.4.10
					r_citem.LinkCost  += tg_rec.LinkCost;  // @v9.4.10
					r_citem.LinkPrice += tg_rec.LinkPrice; // @v9.4.10
					r_citem.ExtVal1 += tg_rec.ExtVal1; // @v9.3.5
					if(Filt.Flags & TrfrAnlzFilt::fInitLocCount)
						if(!LocCountList.SearchPair(r_citem.ID__, dlvr_loc_id, 0)) {
							LocCountList.Add(r_citem.ID__, dlvr_loc_id, 0);
							r_citem.LocCount++;
						}
					if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctMargin) > 0)
						r_citem.Income = r_citem.Price ? (100.0 * (r_citem.Price - r_citem.Cost)) / r_citem.Price : 0.0;
					else if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctIncome) > 0)
						r_citem.Income = r_citem.Cost ? (100.0 * (r_citem.Price - r_citem.Cost)) / r_citem.Cost : 0.0;
					else
						r_citem.Income = r_citem.Price - r_citem.Cost;
					r_citem.Discount += tg_rec.Discount;
					if(!(Flags & fShowSaldo))
						r_citem.SaldoQtty = rest;
					r_citem.Counter = ++LastCacheTouch;
				}
				else {
					long __id = 0;
					if(Flags & fShowSaldo) {
						THROW(ok = GetSaldo(goods_id, pBillRec->Object, tg_rec.DlvrLocID, NZOR(Filt.Period.upp, getcurdate_()), &tg_rec.SaldoQtty, &tg_rec.SaldoAmt));
					}
					else
						tg_rec.SaldoQtty = rest;
					if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctMargin))
						tg_rec.Income = tg_rec.Price ? (100.0 * (tg_rec.Price - tg_rec.Cost)) / tg_rec.Price : 0.0;
					else if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctIncome))
						tg_rec.Income = tg_rec.Cost ? (100.0 * (tg_rec.Price - tg_rec.Cost)) / tg_rec.Cost : 0.0;
					else
						tg_rec.Income = tg_rec.Price - tg_rec.Cost;
					tg_rec.LocCount = 1;
					MEMSZERO(citem);
					citem.ID__ = ++GrpIdCounter;
	#define CPY(f) citem.f = tg_rec.f
					CPY(Dt);
					CPY(GoodsID);
					CPY(PersonID);
					CPY(ArticleID);
					CPY(BillID);
					CPY(DlvrLocID);
					CPY(LocCount);
					CPY(Qtty);
					CPY(PhQtty);
					CPY(Cost);
					CPY(Price);
					CPY(Discount);
					CPY(Income);
					CPY(SaldoQtty);
					CPY(SaldoAmt);
					CPY(PVat);
					CPY(Brutto);     // @v8.7.7
					CPY(LinkQtty);  // @v9.4.10
					CPY(LinkCost);  // @v9.4.10
					CPY(LinkPrice); // @v9.4.10
					CPY(ExtVal1);   // @v9.3.5
	#undef CPY
					citem.Counter = ++LastCacheTouch;
					if(Cache.getCount() >= MaxCacheItems)
						THROW(FlashCacheItems(CacheDelta));
					THROW_SL(Cache.ordInsert(&citem, &pos, Cf));
					if(Filt.Flags & TrfrAnlzFilt::fInitLocCount)
						LocCountList.Add(citem.ID__, dlvr_loc_id, 0);
				}
			}
		}
		else {
			TempTrfrAnlzTbl::Rec rec;
			MEMSZERO(rec);
			memcpy(rec.BillCode, pBillRec->Code, sizeof(rec.BillCode));
			// @debug {
			if(pBillRec->Dt != pTrfrRec->Dt) {
				SString msg_buf, fmt_buf, bill_buf;
				PPObjBill::MakeCodeString(pBillRec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, bill_buf);
				// @v9.5.5 GetGoodsName(pTrfrRec->GoodsID, fmt_buf);
				GObj.FetchNameR(pTrfrRec->GoodsID, fmt_buf); // @v9.5.5
				(msg_buf = bill_buf).CatDiv('-', 1).Cat(fmt_buf);
				bill_buf = msg_buf;
				msg_buf.Printf(PPLoadTextS(PPTXT_LOG_UNEQTRFRBILLDATE, fmt_buf), bill_buf.cptr());
				PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
			}
			// } @debug
			rec.Dt        = _dt;
			rec.OprNo     = _oprno;
			rec.BillID    = pBillRec->ID;
			rec.OpID      = pBillRec->OpID;
			rec.ArticleID = pBillRec->Object;
			rec.GoodsID   = _goods_id;
			rec.LotID     = pTrfrRec->LotID;
			rec.LocID     = pTrfrRec->LocID;
			rec.Qtty      = pTrfrRec->Quantity;
			// @v8.3.0 {
			if(Filt.Flags & TrfrAnlzFilt::fForceInitDlvrAddr) {
				GetDlvrLocID(pBillRec, &dlvr_loc_id);
				rec.DlvrLocID = dlvr_loc_id;
			}
			// } @v8.3.0
			if(Flags & fAsGoodsCard) {
				if(!(pTrfrRec->Flags & (PPTFR_UNLIM | PPTFR_ACK)) || Filt.ArList.GetSingle())
					InRest = faddwsign(InRest, pTrfrRec->Quantity, (Filt.Flags & TrfrAnlzFilt::fLabelOnly) ? -1 : +1);
				if(Filt.Flags & TrfrAnlzFilt::fGByDate) {
					InRest -= pBillRec->CRate;
					// as sales
					rec.Discount = pBillRec->CRate;
					rec.Cost = cost;
					rec.Price = price;
				}
				else {
					if(qtty == 0.0) {
						rec.Cost  = 0.0;
						rec.Price = 0.0;
					}
					else {
						rec.Cost  = R2(cost / (qtty * sign));
						rec.Price = R2((price - discount) / (qtty * sign));
					}
					// as amount
					rec.Discount = (pTrfrRec->Flags & PPTFR_SELLING) ? rec.Price : rec.Cost;
				}
				// as rest after operation
				rec.PhQtty   = InRest;
			}
			else {
				if(Filt.Flags & TrfrAnlzFilt::fGetRest)
					rec.PhQtty = pTrfrRec->Rest; // As Rest
				else if(GObj.GetPhUPerU(pTrfrRec->GoodsID, 0, &phuperu) > 0)
					rec.PhQtty = pTrfrRec->Quantity * phuperu;
				rec.Cost     = cost;
				rec.Price    = price;
				rec.Discount = discount;
				rec.Price   -= rec.Discount;
			}
			rec.PVat = pvat;
			// @v9.4.10 {
			if(pExt) {
				rec.LinkBillID = pExt->LinkBillID; // @v9.6.8
				rec.LinkQtty  = pExt->LinkQtty;
				rec.LinkCost  = pExt->LinkCost;
				rec.LinkPrice = pExt->LinkPrice;
			}
			// } @v9.4.10
			rec.ExtVal1 = ext_val1; // @v9.3.5
			THROW_DB(pBei->insert(&rec));
		}
	}
	PROFILE_END
	CATCHZOK
	return ok;
}

int SLAPI PPViewTrfrAnlz::GetArticlesBySaldo(PPID gdsID, PPIDArray * pArtAry)
{
	int    ok = 1;
	GoodsDebtTbl  gdt;
	GoodsDebtTbl::Key0 gdk0;
	if(pArtAry) {
		pArtAry->freeAll();
		MEMSZERO(gdk0);
		gdk0.GoodsID = gdsID;
		gdk0.ArID    = MAXLONG;
		while(gdt.search(0, &gdk0, spLt) && gdt.data.GoodsID == gdsID) {
			if(!pArtAry->bsearch(gdt.data.ArID))
				THROW(pArtAry->ordInsert(gdt.data.ArID, 0));
			gdk0.Dt = ZERODATE;
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPViewTrfrAnlz::AddAbsentSaldo()
{
	int    ok = -1;
	PPID   goods_id, art_id;
	PPIDArray ary;
	TempTrfrGrpngTbl::Key1 k;
	TempTrfrGrpngTbl::Rec  tg_rec;
	if(P_TrGrpngTbl) {
		if(Filt.Grp == TrfrAnlzFilt::gGoods) {
			if(Filt.ArList.GetSingle())
				art_id = Filt.ArList.GetSingle();
			if(Filt.GoodsID) {
				THROW(ary.add(Filt.GoodsID));
			}
			else {
				THROW(GoodsIterator::GetListByGroup(Filt.GoodsGrpID, &ary));
			}
		}
		else if(Filt.Grp == TrfrAnlzFilt::gCntragent) {
			goods_id = Filt.GoodsID;
			if(Filt.ArList.GetCount()) {
				THROW(ary.add(&Filt.ArList.Get()));
			}
			else {
				THROW(GetArticlesBySaldo(goods_id, &ary));
			}
		}
		for(uint i = 0; i < ary.getCount(); i++) {
			MEMSZERO(tg_rec);
			MEMSZERO(k);
			if(Filt.Grp == TrfrAnlzFilt::gGoods)
				k.GoodsID = tg_rec.GoodsID = goods_id = ary.at(i);
			else if(Filt.Grp == TrfrAnlzFilt::gCntragent) {
				tg_rec.ArticleID = art_id = ary.at(i);
				k.PersonID = tg_rec.PersonID = ObjectToPerson(art_id);
			}
			if(!P_TrGrpngTbl->search(1, &k, spEq)) {
				double qtty, amt;
				THROW(ok = GetSaldo(goods_id, art_id, tg_rec.DlvrLocID, Filt.Period.upp, &qtty, &amt));
				if(ok > 0) {
					tg_rec.SaldoQtty = R2(qtty);
					tg_rec.SaldoAmt  = R2(amt);
					if(tg_rec.SaldoQtty || tg_rec.SaldoAmt)
						THROW_DB(P_TrGrpngTbl->insertRecBuf(&tg_rec));
				}
				else
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

void SLAPI PPViewTrfrAnlz::InitDateText(LDATE dt, PPID billID, SString & rBuf)
{
	FormatSubstDate(Filt.Sgd, dt, rBuf, (P_TrGrpngTbl && Filt.CtKind == TrfrAnlzFilt::ctDate) ? DATF_YMD|DATF_CENTURY : 0);
	if(billID && Filt.Grp != TrfrAnlzFilt::gDateCntragentAgentGoods) {
		BillTbl::Rec bill_rec;
		if(P_BObj->Fetch(billID, &bill_rec) > 0 && bill_rec.Code[0])
			rBuf.Space().Space().Cat(bill_rec.Code);
	}
}

int SLAPI PPViewTrfrAnlz::InitGrpngNames()
{
	int    ok = 1;
	TempTrfrGrpngTbl * p_tgt = P_TrGrpngTbl;
	if(p_tgt) {
		SString msg_buf, temp_buf;
		IterCounter counter;
		PPInitIterCounter(counter, p_tgt);
		PPLoadText(PPTXT_TRFRANLZINITGRPNGTEXT, msg_buf);
		PPObjWorld w_obj;
		PROFILE_START
		//for(PPID id = 0; p_tgt->search(0, &id, spGt);) {
		PPID   id = 0;
		if(p_tgt->search(0, &id, spFirst)) do {
			TempTrfrGrpngTbl::Rec rec;
			p_tgt->copyBufTo(&rec);
			PROFILE(GObj.GetSubstText(rec.GoodsID, Filt.Sgg, &Gsl, temp_buf));
			STRNSCPY(rec.GoodsText, temp_buf);
			PsnObj.GetSubstText(rec.PersonID, (Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) ? rec.DlvrLocID : 0, &Psp, temp_buf);
			STRNSCPY(rec.PersonText, temp_buf);
			InitDateText(rec.Dt, rec.BillID, temp_buf);
			STRNSCPY(rec.DtText, temp_buf);
			THROW_DB(p_tgt->updateRecBuf(&rec));
			PPWaitPercent(counter.Increment(), msg_buf);
		} while(p_tgt->search(0, &id, spNext));
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

#pragma warn -stv

int SLAPI PPViewTrfrAnlz::CreateOrderTable(IterOrder ord)
{
	int    ok = 1;
	int    idx;
	int    skip_creating_temp_tbl = 0;
	PPID   prev_id = -1;
	char   prev_name[256];
	SString msg_buf, temp_buf;
	TempOrderTbl * p_ord_tbl = 0;
	ZDELETE(P_OrderTbl);
	BExtInsert * p_bei = 0;
	IterCounter cntr;
	PPLoadString("sorting", msg_buf);
	if(P_TrGrpngTbl) {
		TempTrfrGrpngTbl * p_t = P_TrGrpngTbl;
		char   gk[MAXKEYLEN];
		memzero(gk, sizeof(gk));
		BExtQuery q(p_t, 0);
		q.select(p_t->ID__, p_t->GoodsText, p_t->PersonText, p_t->DtText, p_t->Dt, 0L);
		THROW(p_ord_tbl = CreateTempOrderFile());
		THROW_MEM(p_bei = new BExtInsert(p_ord_tbl));
		PPInitIterCounter(cntr, p_t);
		for(q.initIteration(0, gk, spFirst); q.nextIteration() > 0; PPWaitPercent(cntr.Increment(), msg_buf)) {
			p_ord_tbl->clearDataBuf();
			p_ord_tbl->data.ID = P_TrGrpngTbl->data.ID__;
			if(ord == PPViewTrfrAnlz::OrdByGoods)
				STRNSCPY(p_ord_tbl->data.Name, p_t->data.GoodsText);
			else if(ord == PPViewTrfrAnlz::OrdByArticle)
				STRNSCPY(p_ord_tbl->data.Name, p_t->data.PersonText);
			else if(ord == PPViewTrfrAnlz::OrdByDate) {
				//STRNSCPY(p_ord_tbl->data.Name, P_TrGrpngTbl->data.DtText);
				datefmt(&p_t->data.Dt, DATF_YMD | DATF_CENTURY, p_ord_tbl->data.Name);
			}
			THROW_DB(p_bei->insert(&p_ord_tbl->data));
		}
	}
	else {
		PPID   obj_type = 0;
		if(!(Flags & fAsGoodsCard))
			if(ord == PPViewTrfrAnlz::OrdByGoods) {
				idx = 1;
				if(Filt.Sgg == sggManuf)
					obj_type = PPOBJ_PERSON;
				else
					obj_type = PPOBJ_GOODS;
			}
			else if(ord == PPViewTrfrAnlz::OrdByArticle) {
				idx = 2;
				obj_type = PPOBJ_ARTICLE;
			}
			else
				skip_creating_temp_tbl = 1;
		else
			skip_creating_temp_tbl = 1;
		if(!skip_creating_temp_tbl) {
			union {
				TempTrfrAnlzTbl::Key1 k1;
				TempTrfrAnlzTbl::Key2 k2;
			} k;
			TempTrfrAnlzTbl * p_tat = P_TrAnlzTbl;
			BExtQuery q(p_tat, idx);
			q.select(p_tat->ID__, p_tat->GoodsID, p_tat->ArticleID, 0L);
			MEMSZERO(k);
			memzero(prev_name, sizeof(prev_name));
			THROW(p_ord_tbl = CreateTempOrderFile());
			THROW_MEM(p_bei = new BExtInsert(p_ord_tbl));
			PPInitIterCounter(cntr, p_tat);
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0; PPWaitPercent(cntr.Increment(), msg_buf)) {
				PPID   id = 0;
				if(ord == PPViewTrfrAnlz::OrdByGoods)
					id = p_tat->data.GoodsID;
				else if(ord == PPViewTrfrAnlz::OrdByArticle)
					id = p_tat->data.ArticleID;
				if(id != prev_id) {
					switch(obj_type) {
						case PPOBJ_GOODS:
							// @v9.5.5 GetGoodsName(id, temp_buf);
							GObj.FetchNameR(id, temp_buf); // @v9.5.5
							break;
						case PPOBJ_ARTICLE: GetArticleName(id, temp_buf); break;
						case PPOBJ_PERSON:  GetPersonName(id, temp_buf); break;
						default: temp_buf.Z(); break;
					}
					temp_buf.CopyTo(prev_name, sizeof(prev_name));
				}
				p_ord_tbl->clearDataBuf();
				p_ord_tbl->data.ID = p_tat->data.ID__;
				STRNSCPY(p_ord_tbl->data.Name, prev_name);
				THROW_DB(p_bei->insert(&p_ord_tbl->data));
				prev_id = id;
			}
		}
	}
	if(p_bei)
		THROW_DB(p_bei->flash());
	P_OrderTbl = p_ord_tbl;
	p_ord_tbl = 0;
	CATCHZOK
	delete p_bei;
	delete p_ord_tbl;
	return ok;
}

int SLAPI PPViewTrfrAnlz::InitIteration(IterOrder ord)
{
	int    ok = 1;
	BExtQuery::ZDelete(&P_IterQuery);
	BExtQuery::ZDelete(&P_IterOrderQuery);
	PrevOuterID = -2;
	CurOuterID  = -1;
	CurIterOrd = ord;
	if(P_TrAnlzTbl)
		PPInitIterCounter(Counter, P_TrAnlzTbl);
	else if(P_TrGrpngTbl)
		PPInitIterCounter(Counter, P_TrGrpngTbl);
	else
		Counter.Init(0UL);
	if(P_OrderTbl && ord == Filt.InitOrd) {
		char   k[MAXKEYLEN];
		P_IterOrderQuery = new BExtQuery(P_OrderTbl, 1);
		P_IterOrderQuery->select(P_OrderTbl->ID, 0L);
		memzero(k, sizeof(k));
		P_IterOrderQuery->initIteration(0, k, spFirst);
		if(P_IterOrderQuery->nextIteration() > 0) {
			CurOuterID  = P_OrderTbl->data.ID;
			PrevOuterID = CurOuterID - 1;
		}
		else
			ok = -1;
	}
	return ok;
}

int SLAPI PPViewTrfrAnlz::NextOuterIteration()
{
	if(P_IterOrderQuery)
		if(P_IterOrderQuery->nextIteration() > 0) {
			CurOuterID = P_OrderTbl->data.ID;
			return 1;
		}
	return -1;
}

int SLAPI PPViewTrfrAnlz::NextInnerIteration(TrfrAnlzViewItem * pItem)
{
	int    r = -1;
	if(P_TrAnlzTbl) {
		if(P_IterOrderQuery) {
			if(PrevOuterID != CurOuterID) {
				PPID __id = CurOuterID;
				if(P_TrAnlzTbl->search(3, &__id, spEq)) {
					PrevOuterID = CurOuterID;
					InitAppData(pItem);
					return 1;
				}
			}
			else
				return -1;
		}
		else if(!P_IterQuery) {
			char   k0[MAXKEYLEN];
			P_IterQuery = new BExtQuery(P_TrAnlzTbl, 0);
			P_IterQuery->selectAll();
			memzero(k0, sizeof(k0));
			P_IterQuery->initIteration(0, k0, spFirst);
		}
		r = P_IterQuery->nextIteration();
		if(r > 0)
			InitAppData(pItem);
	}
	else if(P_TrGrpngTbl) {
		if(P_IterOrderQuery) {
			if(PrevOuterID != CurOuterID) {
				PPID __id = CurOuterID;
				if(P_TrGrpngTbl->search(0, &__id, spEq)) {
					PrevOuterID = CurOuterID;
					InitAppData(pItem);
					return 1;
				}
			}
		   	return -1;
		}
		else if(P_IterQuery == 0) {
			char k0[MAXKEYLEN];
			P_IterQuery = new BExtQuery(P_TrGrpngTbl, 1);
			P_IterQuery->selectAll();
			memzero(k0, sizeof(k0));
			P_IterQuery->initIteration(0, k0, spFirst);
		}
		r = P_IterQuery->nextIteration();
		if(r > 0)
			InitAppData(pItem);
	}
	return r;
}

int FASTCALL PPViewTrfrAnlz::NextIteration(TrfrAnlzViewItem * pItem)
{
	do {
		if(NextInnerIteration(pItem) > 0) {
			Counter.Increment();
			return 1;
		}
	} while(NextOuterIteration() > 0);
	return -1;
}

int SLAPI PPViewTrfrAnlz::GetBrwHdr(const void * pRow, BrwHdr * pHdr) const
{
	int    ok = 1;
	memzero(pHdr, sizeof(*pHdr));
	if(pRow) {
		if(P_Ct) {
			struct CtHdr {
				long   __ID;
				long   GoodsID;
				long   PersonID;
			};
			const CtHdr * p_ct_hdr = (CtHdr *)pRow;
			pHdr->__ID = p_ct_hdr->__ID;
			pHdr->GoodsID = p_ct_hdr->GoodsID;
			pHdr->ArID = p_ct_hdr->PersonID;
			if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)
				pHdr->DlvrAddrID = *(long*)((char*)pRow + sizeof(long) * 3);
		}
		else if(P_TrAnlzTbl) {
			if(Flags & fAsGoodsCard) {
				struct _H {
					PPID  OprNo;
					LDATE LDt;
					PPID  BillID;
				};
				pHdr->BillID  = (Filt.Flags & TrfrAnlzFilt::fGByDate) ? 0 : ((_H*)pRow)->BillID;
				pHdr->Dt      = ((_H*)pRow)->LDt;
				pHdr->OprNo   = ((_H*)pRow)->OprNo;
				pHdr->GoodsID = Filt.GoodsID;
			}
			else {
				struct _H {
					LDATE Dt;
					PPID  BillID;
				};
				pHdr->Dt      = ((_H*)pRow)->Dt;
				pHdr->BillID  = ((_H*)pRow)->BillID;
			}
		}
		else if(P_TrGrpngTbl)
			pHdr->__ID = *(PPID *)pRow;
	}
	else
		ok = 0;
	return ok;
}

int PPViewTrfrAnlz::DynFuncGetRest = 0;
int PPViewTrfrAnlz::DynFuncGetAvgRest = 0;
int PPViewTrfrAnlz::DynFuncGetTrnovr = 0;

static IMPL_DBE_PROC(dbqf_trfrnalz_getturnover_iidprr)
{
	double turnover = 0.0;
	double rest = 0.0;
	if(option != CALC_SIZE) {
		const  PPID   goods_id = params[0].lval;
		const  PPID   loc_id = params[1].lval;
		const  LDATE  dt = params[2].dval;
		const  double qtty = fabs(params[4].rval);
		double amount = fabs(params[5].rval);
		if(qtty != 0.0) {
			const  GCTIterator::GoodsRestArray * p_rest_list = (const  GCTIterator::GoodsRestArray *)params[3].ptrval;
			if(p_rest_list) {
				//rest = loc_id ? p_rest_list->GetRest(goods_id, loc_id, dt) : p_rest_list->GetRest(goods_id, dt);
				DateRange period;
				period.Set(MAXDATE, encodedate(1, 1, 1900));
				rest = p_rest_list->GetAverageRest(goods_id, loc_id, period);
			}
			if(rest != 0.0) {
				if(amount != 0.0) {
					const double price = amount / qtty;
					rest *= price;
				}
				else
					amount = qtty;
                turnover = amount / rest;
			}
		}
		result->init(turnover);
	}
}

static IMPL_DBE_PROC(dbqf_trfrnalz_getrest_iidp)
{
	double rest = 0.0;
	if(option != CALC_SIZE) {
		const PPID   goods_id = params[0].lval;
		const PPID   loc_id = params[1].lval;
		const LDATE  dt = params[2].dval;
		const GCTIterator::GoodsRestArray * p_rest_list = (const  GCTIterator::GoodsRestArray *)params[3].ptrval;
        if(p_rest_list) {
			rest = loc_id ? p_rest_list->GetRest(goods_id, loc_id, dt) : p_rest_list->GetRest(goods_id, dt);
        }
		result->init(rest);
	}
}

static IMPL_DBE_PROC(dbqf_trfrnalz_getavgrest_iidp)
{
	double rest = 0.0;
	if(option != CALC_SIZE) {
		PPID   goods_id = params[0].lval;
		PPID   loc_id = params[1].lval;
		LDATE  dt = params[2].dval;
		const  GCTIterator::GoodsRestArray * p_rest_list = (const  GCTIterator::GoodsRestArray *)params[3].ptrval;
        if(p_rest_list) {
			DateRange period;
			period.Set(MAXDATE, encodedate(1, 1, 1900));
			rest = p_rest_list->GetAverageRest(goods_id, loc_id, period);
        }
		result->init(rest);
	}
}

DBQuery * SLAPI PPViewTrfrAnlz::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncGetRest,    BTS_REAL, dbqf_trfrnalz_getrest_iidp,       4, BTS_INT, BTS_INT, BTS_DATE, BTS_PTR);
	DbqFuncTab::RegisterDyn(&DynFuncGetAvgRest, BTS_REAL, dbqf_trfrnalz_getavgrest_iidp,    4, BTS_INT, BTS_INT, BTS_DATE, BTS_PTR);
	DbqFuncTab::RegisterDyn(&DynFuncGetTrnovr,  BTS_REAL, dbqf_trfrnalz_getturnover_iidprr, 6, BTS_INT, BTS_INT, BTS_DATE, BTS_PTR, BTS_REAL, BTS_REAL);

	uint   brw_id = 0;
	TempTrfrAnlzTbl  * tat = 0;
	TempTrfrGrpngTbl * tgt = 0;
	TempOrderTbl     * tot = 0;
	ArticleTbl   * at2 = 0;
	DBE    dbe_oprkind;
	DBE    dbe_ar;
	DBE    dbe_goods;
	DBE    dbe_goodscode; // @v8.4.11
	DBE    dbe_serial;    // @v8.4.11
	DBE    dbe_rest;      // @v9.1.3
	DBE    dbe_trnovr;    // @v9.1.5
	DBE    dbe_linkdate;  // @v10.0.03
	DBQ  * dbq = 0, * dbq2 = 0;
	DBQuery * q = 0;
	if(P_Ct) {
		if(Filt.Grp == TrfrAnlzFilt::gCntragentDate)
			brw_id = (Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) ? BROWSER_TRFRGR_CT_PD : BROWSER_TRFRGR_CT_P;
		else if(Filt.Grp == TrfrAnlzFilt::gGoodsDate)
			brw_id = BROWSER_TRFRGR_CT_G;
		else //if(Filt.Grp == TrfrAnlzFilt::gGoodsCntragentDate)
			brw_id = (Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) ? BROWSER_TRFRGR_CT_GPD : BROWSER_TRFRGR_CT_GP;
		q = PPView::CrosstabDbQueryStub;
	}
	else {
		if(P_TrAnlzTbl) {
			THROW(CheckTblPtr(tat = new TempTrfrAnlzTbl(P_TrAnlzTbl->GetName())));
			PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, tat->ArticleID);
			PPDbqFuncPool::InitObjNameFunc(dbe_oprkind, PPDbqFuncPool::IdObjNameOprKind, tat->OpID);
			if(Flags & fAsGoodsCard) {
				brw_id = (Filt.Flags & TrfrAnlzFilt::fGByDate) ? BROWSER_TRFRANLZ_GC : BROWSER_GOODSCARD;
				DBE * p_mult = & (tat->Discount * tat->Qtty);
				q = & select(
					tat->OprNo,     // #0
					tat->Dt,        // #1
					tat->BillID,    // #2
					tat->BillCode,  // #3
					tat->Qtty,      // #4
					tat->Discount,  // #5
					*p_mult,        // #6
					tat->PhQtty,    // #7
					dbe_oprkind,    // #8
					dbe_ar,         // #9
					tat->PVat,      // #10
					tat->LinkQtty,  // #11 @v9.4.10
					tat->LinkCost,  // #12 @v9.4.10
					tat->LinkPrice, // #13 @v9.4.10
					tat->ExtVal1,   // #14 @v9.3.5 // @v9.4.10 11-->14
					0L).from(tat, 0L).orderBy(tat->Dt, tat->OprNo, 0L);
				delete p_mult;
				if(pSubTitle) {
					SString loc_name, goods_name;
					GetExtLocationName(Filt.LocList, 3, loc_name);
					*pSubTitle = 0;
					pSubTitle->Cat(loc_name).CatDiv('-', 1).Cat(GetGoodsName(Filt.GoodsID, goods_name));
				}
			}
			else {
				int    goods_as_ar = 0;
				DBE    dbe_cost, dbe_price;
				DBE    dbe_loc;
				PPDbqFuncPool::InitObjNameFunc(dbe_loc, PPDbqFuncPool::IdObjNameLoc, tat->LocID);
				brw_id = BROWSER_TRFRANLZ;
				if(P_OrderTbl)
					THROW(CheckTblPtr(tot = new TempOrderTbl(P_OrderTbl->GetName())));
				{
					dbe_cost.init();
					dbe_cost.push(tat->Qtty);
					dbe_cost.push(tat->Cost);
					dbe_cost.push((DBFunc)PPDbqFuncPool::IdTaCost);
				}
				{
					dbe_price.init();
					dbe_price.push(tat->Qtty);
					dbe_price.push(tat->Price);
					dbe_price.push(tat->Discount);
					dbe_price.push((DBFunc)PPDbqFuncPool::IdTaPrice);
				}
				if(oneof2(Filt.Sgg, sggSupplAgent, sggSuppl)) {
					goods_as_ar = 1;
					THROW(CheckTblPtr(at2 = new ArticleTbl));
					q = & select(
						tat->Dt,       // #0
						tat->BillID,   // #1
						tat->BillCode, // #2
						at2->Name,     // #3
						dbe_oprkind,   // #4
						dbe_ar,        // #5
						tat->Qtty,     // #6
						tat->Cost,     // #7   Сумма в ценах поступления //
						tat->Price,    // #8   Сумма в ценах реализации  //
						dbe_cost,      // #9   Цена поступления //
						dbe_price,     // #10  Цена реализации  //
						dbe_loc,       // #11
						tat->PVat,     // #12
						tat->LinkQtty, // #13 @v9.4.10
						tat->LinkCost, // #14 @v9.4.10
						tat->LinkPrice, // #15 @v9.4.10
						tat->ExtVal1,  // #16 @v9.3.5  // @v9.4.10 13-->16
						0L);
					dbq2 = &(at2->ID += (tat->GoodsID & ~GOODSSUBSTMASK));
				}
				else {
					PPDbqFuncPool::InitObjNameFunc(dbe_goods, PPDbqFuncPool::IdObjNameGoods, tat->GoodsID);
					q = & select(
						tat->Dt,       // #0
						tat->BillID,   // #1
						tat->BillCode, // #2
						dbe_goods,     // #3
						dbe_oprkind,   // #4
						dbe_ar,        // #5
						tat->Qtty,     // #6
						tat->Cost,     // #7   Сумма в ценах поступления //
						tat->Price,    // #8   Сумма в ценах реализации  //
						dbe_cost,      // #9   Цена поступления //
						dbe_price,     // #10  Цена реализации  //
						dbe_loc,       // #11
						tat->PVat,     // #12
						tat->LinkQtty,  // #13 @v9.4.10
						tat->LinkCost,  // #14 @v9.4.10
						tat->LinkPrice, // #15 @v9.4.10
						tat->ExtVal1,  // #16 @v9.3.5  // @v9.4.10 13-->16
						0L);
					// @v8.4.11 {
					if(Filt.Sgg == sggNone) {
                        if(Filt.Flags & TrfrAnlzFilt::fShowGoodsCode) {
							dbe_goodscode.init();
							dbe_goodscode.push(tat->GoodsID);
							dbe_goodscode.push((DBFunc)PPDbqFuncPool::IdGoodsSingleBarcode);
							q->addField(dbe_goodscode);  // #17 // @v9.3.5 #13-->#14 // @v9.4.10 14-->17
                        }
                        else {
							q->addField(tat->ID__);      // #17 @stub // @v9.3.5 #13-->#14 // @v9.4.10 14-->17
                        }
                        if(Filt.Flags & TrfrAnlzFilt::fShowSerial) {
                        	dbe_serial.init();
							dbe_serial.push(dbconst(PPTAG_LOT_SN));
							dbe_serial.push(tat->LotID);
							dbe_serial.push((DBFunc)PPDbqFuncPool::IdObjTagText);
							q->addField(dbe_serial);  // #18 // @v9.3.5 #14-->#15 // @v9.4.10 15-->18
                        }
                        else {
							q->addField(tat->ID__);   // #18 @stub // @v9.3.5 #14-->#15 // @v9.4.10 15-->18
                        }
						{
							dbe_linkdate.init();
							dbe_linkdate.push(tat->LinkBillID);
							dbe_linkdate.push((DBFunc)PPDbqFuncPool::IdBillDate);
							q->addField(dbe_linkdate); // #19 @v10.0.03
						}
					}
					else {
						q->addField(tat->ID__);  // #17 @stub // @v9.3.5 #13-->#14 // @v9.4.10 14-->17
						q->addField(tat->ID__);  // #18 @stub // @v9.3.5 #14-->#15 // @v9.4.10 15-->18
						q->addField(tat->ID__);  // #19 @stub
					}
					// } @v8.4.11
				}
				if(P_OrderTbl) {
					// if !goods_as_ar then at2 == 0
					q->from(tot, tat, at2, 0L);
					dbq = & (tat->ID__ == tot->ID);
					q->where(*dbq && *dbq2).orderBy(tot->Name, 0L);
				}
				else {
					// if !goods_as_ar then at2 == 0
					q->from(tat, at2, 0L).where(*dbq2).orderBy(tat->Dt, tat->OprNo, 0L);
				}
			}
		}
		else if(P_TrGrpngTbl) {
			switch(Filt.Grp) {
				case TrfrAnlzFilt::gGoods:
				case TrfrAnlzFilt::gGoodsDate: brw_id = (Flags & fShowSaldo) ? BROWSER_TRFRGR_GS : BROWSER_TRFRGR_G; break;
				case TrfrAnlzFilt::gCntragent: brw_id = (Flags & fShowSaldo) ? BROWSER_TRFRGR_PS : BROWSER_TRFRGR_P; break;
				case TrfrAnlzFilt::gCntragentDate: brw_id = BROWSER_TRFRGR_PD; break;
				case TrfrAnlzFilt::gGoodsCntragent: brw_id = BROWSER_TRFRGR_GP; break;
				case TrfrAnlzFilt::gGoodsCntragentDate: brw_id = BROWSER_TRFRGR_GPD; break;
				case TrfrAnlzFilt::gGoodsBill: brw_id = BROWSER_TRFRGR_GB; break;
				case TrfrAnlzFilt::gDateCntragentAgentGoods: brw_id = BROWSER_TRFRGR_DPAG; break;
				case TrfrAnlzFilt::gBillCntragent: brw_id = BROWSER_TRFRGR_BP; break;
			}
			DBFieldList fld_list;
			THROW(CheckTblPtr(tgt = new TempTrfrGrpngTbl(P_TrGrpngTbl->GetName())));
			fld_list.Add(tgt->ID__);       //  #0
			fld_list.Add(tgt->GoodsID);    //  #1
			fld_list.Add(tgt->PersonID);   //  #2
			fld_list.Add(tgt->Dt);         //  #3
			fld_list.Add(tgt->BillID);     //  #4
			fld_list.Add(tgt->ArticleID);  //  #5
			fld_list.Add(tgt->GoodsText);  //  #6
			fld_list.Add(tgt->PersonText); //  #7
			fld_list.Add(tgt->DtText);     //  #8
			fld_list.Add(tgt->Qtty);       //  #9
			fld_list.Add(tgt->PhQtty);     // #10
			fld_list.Add(tgt->Cost);       // #11
			fld_list.Add(tgt->Price);      // #12
			fld_list.Add(tgt->Discount);   // #13
			fld_list.Add(tgt->SaldoQtty);  // #14
			fld_list.Add(tgt->SaldoAmt);   // #15
			fld_list.Add(tgt->PVat);       // #16
			fld_list.Add(tgt->LinkQtty);   // #17 @v9.4.10
			fld_list.Add(tgt->LinkCost);   // #18 @v9.4.10
			fld_list.Add(tgt->LinkPrice);  // #19 @v9.4.10
			fld_list.Add(tgt->ExtVal1);    // #20 // @v9.3.5 // @v9.4.10 #17-->#20
			q = & select(fld_list);
			if(Filt.Grp == TrfrAnlzFilt::gDateCntragentAgentGoods) {
				PPDbqFuncPool::InitObjNameFunc(dbe_ar, PPDbqFuncPool::IdObjNameAr, tgt->BillID);
				q->addField(dbe_ar);       // #21 // @v9.3.5 #17-->18 // @v9.4.10 #18-->#21
			}
			else {
				q->addField(tgt->ID__);    // #21 @stub // @v9.3.5 #17-->18 // @v9.4.10 #18-->#21
			}
			// @v8.4.11 {
			if(Filt.Sgg == sggNone && Filt.Flags & TrfrAnlzFilt::fShowGoodsCode) {
				dbe_goodscode.init();
				dbe_goodscode.push(tgt->GoodsID);
				dbe_goodscode.push((DBFunc)PPDbqFuncPool::IdGoodsSingleBarcode);
				q->addField(dbe_goodscode);  // #22 // @v9.3.5 #18-->19 // @v9.4.10 #19-->#22
			}
			else {
				q->addField(tgt->ID__);      // #22 @stub // @v9.3.5 #18-->19 // @v9.4.10 #19-->#22
			}
			// } @v8.4.11
			if(Filt.Flags & TrfrAnlzFilt::fCalcRest) {
				dbe_rest.init();
				dbe_rest.push(tgt->GoodsID);
				dbe_rest.push(dbconst(0L));
				if(Filt.Flags & TrfrAnlzFilt::fCalcAvgRest) {
					if(Filt.Grp == TrfrAnlzFilt::gGoodsDate)
						dbe_rest.push(tgt->Dt);
					else
						dbe_rest.push(dbconst(NZOR(Filt.Period.upp, getcurdate_())));
					dbe_rest.push(dbconst((const void *)&GctRestList));
					dbe_rest.push((DBFunc)DynFuncGetAvgRest);
				}
				else {
					if(Filt.Grp == TrfrAnlzFilt::gGoodsDate)
						dbe_rest.push(tgt->Dt);
					else
						dbe_rest.push(dbconst(NZOR(Filt.Period.upp, getcurdate_())));
					dbe_rest.push(dbconst((const void *)&GctRestList));
					dbe_rest.push((DBFunc)DynFuncGetRest);
				}
				q->addField(dbe_rest);      // #23 // @v9.3.5 #19-->20 // @v9.4.10 #20-->#23
			}
			else {
				q->addField(tgt->ID__);     // #23 @stub // @v9.3.5 #19-->20 // @v9.4.10 #20-->#23
			}
			if(Filt.Flags & TrfrAnlzFilt::fCalcRest && Filt.RestAddendumValue & TrfrAnlzFilt::ravTurnoverRate) {
				dbe_trnovr.init();
				dbe_trnovr.push(tgt->GoodsID);
				dbe_trnovr.push(dbconst(0L));
				if(Filt.Grp == TrfrAnlzFilt::gGoodsDate)
					dbe_trnovr.push(tgt->Dt);
				else
					dbe_trnovr.push(dbconst(NZOR(Filt.Period.upp, getcurdate_())));
				dbe_trnovr.push(dbconst((const void *)&GctRestList));
				dbe_trnovr.push(tgt->Qtty);
				/* @v9.3.4 if(Filt.RestAddendumValue & TrfrAnlzFilt::ravPrice)
					dbe_trnovr.push(tgt->Price);
				else if(Filt.RestAddendumValue & TrfrAnlzFilt::ravCost)
					dbe_trnovr.push(tgt->Cost);
				else */
					dbe_trnovr.push(dbconst(0.0));
				dbe_trnovr.push((DBFunc)DynFuncGetTrnovr);
				q->addField(dbe_trnovr);     // #24 @stub // @v9.3.5 #20-->21 // @v9.4.10 #21-->#24
			}
			else {
				q->addField(tgt->ID__);      // #24 @stub // @v9.3.5 #20-->21 // @v9.4.10 #21-->#24
			}
			if(P_OrderTbl) {
				THROW(CheckTblPtr(tot = new TempOrderTbl(P_OrderTbl->GetName())));
				q->from(tot, tgt, 0L).where(tgt->ID__ == tot->ID).orderBy(tot->Name, 0L);
			}
			else
				q->from(tgt, 0L).where(*dbq).orderBy(tgt->Dt, 0L);
		}
		THROW(CheckQueryPtr(q));
	}
	CATCH
		ZDELETE(q);
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewTrfrAnlz::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
			case PPVCMD_GRAPH:
				ok = -1;
				ViewGraph();
				break;
			case PPVCMD_EDITGOODS:
			case PPVCMD_EDITPERSON:
				if(!P_Ct && P_TrGrpngTbl) {
					ok = -1;
					TempTrfrGrpngTbl::Rec rec;
					PPID   obj_type = 0;
					PPID   obj_id = 0;
					if(Filt.Grp == TrfrAnlzFilt::gCntragent && !Filt.Sgp)
						obj_type = (Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) ? PPOBJ_LOCATION : PPOBJ_PERSON;
					else if(Filt.Grp == TrfrAnlzFilt::gGoods && !Filt.Sgg)
						obj_type = PPOBJ_GOODS;
					if(obj_type) {
						BrwHdr hdr;
						if(GetBrwHdr(pHdr, &hdr) && SearchByID(P_TrGrpngTbl, 0, hdr.__ID, &rec) > 0) {
							if(obj_type == PPOBJ_LOCATION) {
								obj_id = rec.DlvrLocID;
                                if(obj_id && PsnObj.LocObj.Edit(&obj_id, 0) > 0) {
                                	ok = 1;
                                }
							}
							else if(obj_type == PPOBJ_PERSON) {
								obj_id = rec.PersonID;
								if(obj_id && PsnObj.Edit(&obj_id, 0) > 0) {
									ok = 1;
								}
							}
							else if(obj_type == PPOBJ_GOODS) {
								obj_id = labs(rec.GoodsID);
								if(obj_id && GObj.Edit(&obj_id, 0) > 0) {
									ok = 1;
								}
							}
						}
					}
				}
				break;
			case PPVCMD_TAGS:
			case PPVCMD_TAGSALL:
				if(!P_Ct && P_TrGrpngTbl) {
					ok = -1;
					TempTrfrGrpngTbl::Rec rec;
					PPID   obj_type = 0;
					PPID   obj_id = 0;
					if(Filt.Grp == TrfrAnlzFilt::gCntragent && !Filt.Sgp)
						obj_type = (Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) ? PPOBJ_LOCATION : PPOBJ_PERSON;
					else if(Filt.Grp == TrfrAnlzFilt::gGoods && !Filt.Sgg)
						obj_type = PPOBJ_GOODS;
					if(obj_type) {
						if(ppvCmd == PPVCMD_TAGS) {
							BrwHdr hdr;
							if(GetBrwHdr(pHdr, &hdr) && SearchByID(P_TrGrpngTbl, 0, hdr.__ID, &rec) > 0) {
								if(obj_type == PPOBJ_LOCATION)
									obj_id = rec.DlvrLocID;
								else if(obj_type == PPOBJ_PERSON)
									obj_id = rec.PersonID;
								else if(obj_type == PPOBJ_GOODS)
									obj_id = labs(rec.GoodsID);
								if(obj_id)
									ok = EditObjTagValList(obj_type, obj_id, 0);
							}
						}
						else if(ppvCmd == PPVCMD_TAGSALL) {
							int    update_mode = ObjTagList::mumAdd;
							ObjTagList common_tag_list;
							common_tag_list.ObjType = obj_type;
							PPIDArray id_list;
							TrfrAnlzViewItem item;
							Reference * p_ref = PPRef;
							for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
								if(obj_type == PPOBJ_LOCATION)
									obj_id = item.DlvrLocID;
								else if(obj_type == PPOBJ_PERSON)
									obj_id = item.PersonID;
								else if(obj_type == PPOBJ_GOODS)
									obj_id = labs(item.GoodsID);
								else
									obj_id = 0;
								id_list.addnz(obj_id);
							}
							id_list.sortAndUndup();
							if(id_list.getCount() && EditObjTagValUpdateList(&common_tag_list, 0, &update_mode) > 0 && common_tag_list.GetCount()) {
								PPWait(1);
								PPTransaction tra(1);
								THROW(tra);
								for(uint i = 0; i < id_list.getCount(); i++) {
									obj_id = id_list.get(i);
									ObjTagList local_tag_list;
									THROW(p_ref->Ot.GetList(obj_type, obj_id, &local_tag_list));
									if(local_tag_list.Merge(common_tag_list, update_mode) > 0) {
										THROW(p_ref->Ot.PutList(obj_type, obj_id, &local_tag_list, 0));
									}
									PPWaitPercent(GetCounter());
								}
								THROW(tra.Commit());
								PPWait(0);
							}
						}
					}
				}
				break;
		}
	}
	CATCHZOKPPERR
	return ok;
}

SString & SLAPI PPViewTrfrAnlz::GetCtColumnTitle(const int ct, SString & rBuf)
{
	rBuf.Z();
	SString temp_buf;
	switch(ct) {
		case TrfrAnlzFilt::ctvQtty: PPLoadString("qtty", rBuf); break;
		case TrfrAnlzFilt::ctvCost:
			PPGetWord(PPWORD_COST, 0, temp_buf);
			PPGetWord(PPWORD_TRNOVR_IN, 0, rBuf).Space().Cat(temp_buf);
			break;
		case TrfrAnlzFilt::ctvNetPrice:
			PPGetWord(PPWORD_PRICE_P, 0, temp_buf);
			PPGetWord(PPWORD_TRNOVR_IN, 0, rBuf).Space().Cat(temp_buf);
			break;
		case TrfrAnlzFilt::ctvPctMargin:
			PPGetWord(PPWORD_MARGIN, 0, rBuf).Space().CatChar('%');
			break;
		case TrfrAnlzFilt::ctvPctIncome:
			PPLoadString("income", rBuf);
			rBuf.Space().CatChar('%');
			break;
		case TrfrAnlzFilt::ctvIncome: PPLoadString("income", rBuf); break;
		case TrfrAnlzFilt::ctvLocCount: PPGetWord(PPWORD_LOCSCOUNT, 0, rBuf); break;
	}
	return rBuf;
}

void SLAPI PPViewTrfrAnlz::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		if(!P_Ct) {
			if(Filt.Grp == TrfrAnlzFilt::gGoodsDate) {
				pBrw->InsColumn(1, "@date", 3, 0, MKSFMT(0, DATF_DMY), 0);
			}
			if(Filt.Flags & TrfrAnlzFilt::fCalcRest) {
				pBrw->insertColumn(-1, "Rest", 23, 0, MKSFMTD(0, 3, NMBF_NOZERO), 0); // @v9.3.5 #19-->20 // @v9.4.10 #20-->23
				if(Filt.RestAddendumValue & TrfrAnlzFilt::ravTurnoverRate) {
					pBrw->insertColumn(-1, "Turnover", 24, 0, MKSFMTD(0, 6, NMBF_NOZERO), 0); // @v9.3.5 #20-->21 // @v9.4.10 #21-->24
				}
			}
			const DBQBrowserDef * p_def = (const DBQBrowserDef *)pBrw->getDef();
			if(p_def) {
				const DBQuery * p_q = p_def->getQuery();
				if(Filt.Flags & TrfrAnlzFilt::fCalcVat) {
					if(p_q) {
						uint pos = 0;
						if(p_q->getFieldPosByName("PVat", &pos)) {
							// @v9.0.2 pBrw->InsColumnWord(-1, PPWORD_VAT, pos, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
							pBrw->InsColumn(-1, "@vat", pos, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0); // @v9.0.2
						}
					}
				}
				// @v8.4.11 {
				if(!(Flags & fAsGoodsCard) && Filt.Sgg == sggNone) {
					int    col_pos = -1;
					if(P_TrAnlzTbl) {
						for(uint i = 0; col_pos < 0 && i < p_def->getCount(); i++)
							if(p_def->at(i).OrgOffs == 3) // #3
								col_pos = (int)i;
						if(col_pos >= 0) {
							if(Filt.Flags & TrfrAnlzFilt::fShowGoodsCode) {
								pBrw->InsColumn(++col_pos, "@barcode", 17, 0, 0, 0); // @v9.3.5 #13-->14 // @v9.4.10 #14-->17
							}
							if(Filt.Flags & TrfrAnlzFilt::fShowSerial) {
								pBrw->InsColumn(++col_pos, "@serial", 18, 0, 0, 0); // @v9.3.5 #14-->15 // @v9.4.10 #15-->18
							}
						}
					}
					else if(P_TrGrpngTbl) {
						if(Filt.Flags & TrfrAnlzFilt::fShowGoodsCode) {
							for(uint i = 0; col_pos < 0 && i < p_def->getCount(); i++)
								if(p_def->at(i).OrgOffs == 6) // #6
									col_pos = (int)i;
							if(col_pos >= 0)
								pBrw->InsColumn(++col_pos, "@barcode", 22, 0, 0, 0); // @v9.3.5 #18-->19 // @v9.4.10 #19-->22
						}
					}
				}
				// } @v8.4.11
				// @v9.4.10 {
				if(Filt.Flags & Filt.fCmpWrOff) {
					uint pos = 0;
					if(p_q) {
						// @v10.0.03 {
						if(!(Flags & fAsGoodsCard) && Filt.Sgg == sggNone) {
							pBrw->InsColumn(-1, "LinkDate", 19, 0, DATF_DMY, 0);
						}
						// } @v10.0.03 
						if(p_q->getFieldPosByName("LinkQtty", &pos))
							pBrw->InsColumn(-1, "LinkQtty", pos, 0, MKSFMTD(0, 3, NMBF_NOZERO), 0);
						if(p_q->getFieldPosByName("LinkCost", &pos))
							pBrw->InsColumn(-1, "LinkCost", pos, 0, MKSFMTD(0, 3, NMBF_NOZERO), 0);
						if(p_q->getFieldPosByName("LinkPrice", &pos))
							pBrw->InsColumn(-1, "LinkPrice", pos, 0, MKSFMTD(0, 3, NMBF_NOZERO), 0);
					}
				}
				// } @v9.4.10
				if(Filt.ExtValueParam[0]) {
					uint pos = 0;
					if(p_q && p_q->getFieldPosByName("ExtVal1", &pos))
						pBrw->InsColumn(-1, "ExtVal1", pos, 0, MKSFMTD(0, 2, NMBF_NOZERO), 0);
				}
			}
		}
		else {
			BroCrosstab ct_col;
			BrowserDef * p_def = pBrw->view->getDef();
			if(p_def) {
				uint col_width = 20;
				SString title, buf;
#define ADDCTCOLUMN(type, caption, format, options, width) \
			ct_col.P_Text    = newStr(caption); \
			ct_col.Type    = type; \
			ct_col.Format  = format; \
			ct_col.Options = options;  \
			ct_col.Width   = width; \
			p_def->AddCrosstab(&ct_col);

				p_def->FreeAllCrosstab();
				if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvQtty) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvQtty, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(col_width, 2, NMBF_NOZERO), 0, col_width);
				}
				if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvCost) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvCost, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(col_width, 2, NMBF_NOZERO), 0, col_width);
				}
				if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvNetPrice) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvNetPrice, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(col_width, 2, NMBF_NOZERO), 0, col_width);
				}
				if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctMargin) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvPctMargin, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(10, 2, NMBF_NOZERO), 0, 10);
				}
				else if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvPctIncome) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvPctIncome, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(10, 2, NMBF_NOZERO), 0, 10);
				}
				else if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvIncome) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvIncome, title);
					ADDCTCOLUMN(T_DOUBLE, title, MKSFMTD(10, 2, NMBF_NOZERO), 0, 10);
				}
				if(Filt.CtValList.CheckID(TrfrAnlzFilt::ctvLocCount) > 0) {
					GetCtColumnTitle(TrfrAnlzFilt::ctvLocCount, title);
					ADDCTCOLUMN(T_LONG, title, MKSFMT(col_width, NMBF_NOZERO), 0, col_width);
				}
			}
			P_Ct->SetupBrowserCtColumns(pBrw);
		}
		pBrw->SetTempGoodsGrp(Filt.GoodsGrpID);
	}
}

int SLAPI PPViewTrfrAnlz::Print(const void *)
{
	int    ok = 1/*, order = 0*/;
	int    disable_grpng = 0;
	uint   rpt_id = 0;
	if(P_Ct)
		rpt_id = REPORT_TRFRANLZ_CT;
	else if(Flags & fAsGoodsCard) {
		if(Filt.Flags & TrfrAnlzFilt::fGByDate)
			rpt_id = REPORT_GOODSCARDCOMPR;
		else if(Filt.LocList.GetSingle())
			rpt_id = REPORT_GOODSCARD;
		else
			rpt_id = REPORT_GOODSCARDL;
	}
	else {
		switch(Filt.Grp) {
			case TrfrAnlzFilt::gGoods:
			case TrfrAnlzFilt::gCntragent:
				rpt_id = (Flags & fShowSaldo) ? REPORT_TRFRANLZ_G1S : REPORT_TRFRANLZ_G1;
				break;
			case TrfrAnlzFilt::gNone:                    rpt_id = REPORT_TRFRANLZ; break;
			case TrfrAnlzFilt::gGoodsCntragent:
			case TrfrAnlzFilt::gCntragentDate:
			case TrfrAnlzFilt::gGoodsBill:
			case TrfrAnlzFilt::gGoodsDate:               rpt_id = REPORT_TRFRANLZ_G2;   break;
			case TrfrAnlzFilt::gGoodsCntragentDate:      rpt_id = REPORT_TRFRANLZ_G3;   break;
			case TrfrAnlzFilt::gDateCntragentAgentGoods: rpt_id = REPORT_TRFRANLZ_DPAG; break;
			default: rpt_id = REPORT_TRFRANLZ; break;
		}
	}
	PPReportEnv env;
	PView pv(this);
	env.Sort = CurViewOrd;
	env.PrnFlags = disable_grpng ? SReport::DisableGrouping : 0;
	return PPAlddPrint(rpt_id, &pv, &env);
}

int SLAPI PPViewTrfrAnlz::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = -1;
	BrwHdr hdr;
	if(GetBrwHdr(pHdr, &hdr)) {
		long   ct_cntrag_id = 0;
		DateRange ct_period;
		int    is_ct = 0;
		if(P_Ct) {
			const  uint tab_idx = pBrw ? pBrw->GetCurColumn() : 0;
			PPID   tab_id = 0;
			// @v9.8.7 DBFieldList fld_list; // realy const, do not modify
			int    r = 0;
			//
			// Расчет смещения осуществляется в соответствии с установкой полей
			// в функции  PPViewTrfrAnlz::CreateBrowserQuery
			//
			if(oneof2(Filt.Grp, TrfrAnlzFilt::gCntragentDate, TrfrAnlzFilt::gGoodsDate)) {
				if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)
					r = (tab_idx > 1) ? P_Ct->GetTab((tab_idx-2) / P_Ct->GetAggrCount(), &tab_id) : 1;
				else
					r = (tab_idx > 0) ? P_Ct->GetTab((tab_idx-1) / P_Ct->GetAggrCount(), &tab_id) : 1;
			}
			else { //if(Filt.Grp == TrfrAnlzFilt::gGoodsCntragentDate)
				if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)
					r = (tab_idx > 2) ? P_Ct->GetTab((tab_idx-3) / P_Ct->GetAggrCount(), &tab_id) : 1;
				else
					r = (tab_idx > 1) ? P_Ct->GetTab((tab_idx-2) / P_Ct->GetAggrCount(), &tab_id) : 1;
			}
			ct_period = Filt.Period;
			if(r > 0) {
				if(Filt.CtKind == TrfrAnlzFilt::ctDate) {
					LDATE dt;
					dt.v = (ulong)tab_id;
					if(!checkdate(dt, 0) || ExpandSubstDate(Filt.Sgd, dt, &ct_period) <= 0)
						ct_period = Filt.Period;
				}
				else if(Filt.CtKind == TrfrAnlzFilt::ctCntragent)
					ct_cntrag_id = tab_id;
			}
			is_ct = 1;
		}
		if(Filt.Flags & TrfrAnlzFilt::fGByDate) {
			TrfrAnlzFilt flt;
			flt = Filt;
			if(is_ct) {
				flt.Period = ct_period;
				flt.ArList.Set(0);
				flt.ArList.Add(ct_cntrag_id);
			}
			else if(hdr.Dt)
				ExpandSubstDate(Filt.Sgd, hdr.Dt, &flt.Period);
			flt.Flags = 0;
			ok = ViewTrfrAnlz(&flt);
		}
		else if(hdr.BillID)
			ok = P_BObj->Edit(&hdr.BillID, 0);
		else if(!(Flags & fAsGoodsCard) && Filt.Grp != TrfrAnlzFilt::gNone) {
			TempTrfrGrpngTbl::Rec rec;
			TrfrAnlzFilt flt;
			flt = Filt;
			PPID   goods_id = 0;
			PPID   psn_id = 0;
			PPID   ar_id = 0;
			if(P_Ct) {
				psn_id = hdr.ArID;
				goods_id = hdr.GoodsID;
				if(is_ct) {
					flt.Period = ct_period;
					flt.ArList.Set(0);
					flt.ArList.Add(ct_cntrag_id);
				}
				if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) {
					if(!hdr.DlvrAddrID)
						flt.Flags |= flt.fByZeroDlvrAddr;
					else
						flt.DlvrAddrID = hdr.DlvrAddrID;
				}
				ok = 1;
			}
			else if(SearchByID(P_TrGrpngTbl, 0, hdr.__ID, &rec) > 0) {
				psn_id   = rec.PersonID/* & (~ARTICLE_MASK)*/;
				goods_id = rec.GoodsID;
				if(!oneof2(Filt.Sgp, sgpBillAgent, sgpVesselAgent)) {
					if(Filt.HasCntragentGrouping()) {
						// @v9.0.9 {
						if(Filt.Sgp == sgpCity) {
							flt.CityID = psn_id;
							SETFLAGBYSAMPLE(flt.Flags, TrfrAnlzFilt::fSubstDlvrAddr, Filt.Flags);
							SETFLAGBYSAMPLE(flt.Flags, TrfrAnlzFilt::fSubstPersonRAddr, Filt.Flags);
						} // } @v9.0.9
						else {
							flt.ArList.Set(0);
							flt.ArList.Add(rec.ArticleID);
							if(Filt.Flags & TrfrAnlzFilt::fDiffByDlvrAddr) {
								if(!rec.DlvrLocID)
									flt.Flags |= flt.fByZeroDlvrAddr;
								else
									flt.DlvrAddrID = rec.DlvrLocID;
							}
						}
					}
				}
				if(Filt.HasBillGrouping())
					flt.BillList.Add(rec.BillID);
				if(rec.Dt)
					ExpandSubstDate(Filt.Sgd, rec.Dt, &flt.Period);
				ok = 1;
			}
			if(ok > 0) {
				flt.Flags = (flt.Flags & (TrfrAnlzFilt::fCalcVat|TrfrAnlzFilt::fByZeroDlvrAddr|
					TrfrAnlzFilt::fSubstDlvrAddr|TrfrAnlzFilt::fSubstPersonRAddr|TrfrAnlzFilt::fCmpWrOff));
				flt.Grp = TrfrAnlzFilt::gNone;
				flt.Sgd = sgdNone;
				flt.Sgg = sggNone;
				flt.Sgp = sgpNone;
				if(psn_id) {
					if(Filt.Sgp == sgpBillAgent) {
						const PPID acc_sheet_id = GetAgentAccSheet();
						if(acc_sheet_id) {
							PPID   agent_ar_id = 0;
							flt.AgentList.Set(0);
							flt.AgentList.Add((ArObj.P_Tbl->PersonToArticle(psn_id, acc_sheet_id, &agent_ar_id) > 0) ? agent_ar_id : rec.ArticleID);
						}
					}
					// @ не реализовано
					else if(Filt.Sgp == sgpVesselAgent) {
					}
					else if(Filt.Sgp == sgpNone) {
						//flt.ArID = ObjectToPerson
					}
				}
				else if(Filt.Sgp == sgpBillAgent) {
					flt.AgentList.Set(0);
					flt.Flags |= TrfrAnlzFilt::fByZeroAgent;
				}
				if(Filt.HasGoodsGrouping()) {
					switch(Filt.Sgg) {
						case sggGroup: flt.GoodsGrpID = goods_id; break;
						case sggBrand: flt.BrandID = goods_id; break;
						case sggSuppl: flt.SupplID = (goods_id & ~GOODSSUBSTMASK); break;
						case sggLocation: flt.LocList.SetSingle(goods_id & ~GOODSSUBSTMASK); break;
						case sggSupplAgent:
							;// (goods_id & ~GOODSSUBSTMASK)
							break;
						default: flt.GoodsID = goods_id; break;
					}
				}
				ok = ViewTrfrAnlz(&flt);
			}
		}
	}
	return ok;
}

int SLAPI PPViewTrfrAnlz::ViewGraph()
{
	int    ok = -1;
#if 0 // {
	double * p_ary = 0;
	if(Filt.OpID != 0 && Filt.Grp != TrfrAnlzFilt::gNone && (Filt.Sgg != sggNone ||
		Filt.Sgp != sgpNone || Filt.Sgd != sgdNone)) {
		TrfrAnlzViewItem item;
		PPGraphParam g_param;
		PPGraph graph;
		InitIteration(PPViewTrfrAnlz::OrdByDefault);
		for(g_param.NumRows; NextIteration(&item) > 0; g_param.NumRows++);
		if(g_param.NumRows > 0) {
			char   buf[48];
			p_ary = new double[g_param.NumRows];
			g_param.Kind = PPGraphParam::gKindPie;
			g_param.AllocMem();
			InitIteration(PPViewTrfrAnlz::OrdByDefault);
			for(uint i = 0; NextIteration(&item) > 0; i++) {
				p_ary[i] = -item.Price;
				if(sstrlen(item.GoodsText) > 0)
					STRNSCPY(buf, item.GoodsText);
				else
					STRNSCPY(buf, item.PersonText);
				SOemToChar(buf);
				g_param.Names.insert(newStr(buf));
			}
			STRNSCPY(g_param.Title, "Diagram");
			g_param.P_Rows->insert(p_ary);
			graph.Init(&g_param);
			graph.Browse();
		}
	}
	delete p_ary;
#endif // } 0
	return ok;
}

void FASTCALL PPViewTrfrAnlz::InitAppData(TrfrAnlzViewItem * pItem)
{
	pItem->Clear();
	GoodsSubstItem gsi;
	if(P_TrAnlzTbl) {
		PersonTbl::Rec psn_rec;
		const TempTrfrAnlzTbl::Rec & r_rec = P_TrAnlzTbl->data;
		pItem->Dt         = r_rec.Dt;
		pItem->OprNo      = r_rec.OprNo;
		pItem->BillID     = r_rec.BillID;
		pItem->BillCode_  = r_rec.BillCode;
		pItem->LocID      = r_rec.LocID;
		pItem->ArticleID  = r_rec.ArticleID;
		pItem->PersonID   = ObjectToPerson(r_rec.ArticleID);
		pItem->OpID       = r_rec.OpID;
		pItem->GoodsID    = r_rec.GoodsID;
		pItem->LotID      = r_rec.LotID;
		pItem->LinkBillID = r_rec.LinkBillID; // @v9.6.8
		pItem->DlvrLocID  = r_rec.DlvrLocID; // @v8.3.2

		GObj.GetSubstText(r_rec.GoodsID, Filt.Sgg, &Gsl, pItem->GoodsText_);
		pItem->SubGoodsClsID = (IS_SGG_CLSSUBST(Filt.Sgg) && Gsl.GetItem(r_rec.GoodsID, &gsi) > 0) ? gsi.ClsID : 0;
		InitDateText(r_rec.Dt, 0L, pItem->DtText_);
		if(pItem->PersonID && PsnObj.Fetch(pItem->PersonID, &psn_rec) > 0)
			pItem->PersonText_ = psn_rec.Name;

		pItem->Qtty      = r_rec.Qtty;
		pItem->PhQtty    = r_rec.PhQtty;
		pItem->Cost      = r_rec.Cost;
		pItem->Price     = r_rec.Price;
		pItem->Discount  = r_rec.Discount;
		pItem->PVat      = r_rec.PVat;
		pItem->Brutto    = r_rec.Brutto;    // @v9.6.8
		pItem->LinkQtty  = r_rec.LinkQtty;  // @v9.6.8
		pItem->LinkCost  = r_rec.LinkCost;  // @v9.6.8
		pItem->LinkPrice = r_rec.LinkPrice; // @v9.6.8
		pItem->ExtValue[0] = r_rec.ExtVal1; // @v9.3.5
		if((Flags & fAsGoodsCard) || (Filt.Flags & TrfrAnlzFilt::fGetRest)) { // AHTOXA
			pItem->Rest   = r_rec.PhQtty;
			pItem->Amount = r_rec.Discount;
		}
		else {
			pItem->PhQtty = r_rec.PhQtty;
			pItem->Discount = r_rec.Discount;
		}
	}
	else if(P_TrGrpngTbl) {
		const TempTrfrGrpngTbl::Rec & r_tg_rec = P_TrGrpngTbl->data;
		BillTbl::Rec bill_rec;
		pItem->Dt        = r_tg_rec.Dt;
		pItem->GoodsID   = r_tg_rec.GoodsID;
		pItem->PersonID  = r_tg_rec.PersonID;
		pItem->ArticleID = r_tg_rec.ArticleID;
		pItem->BillID    = r_tg_rec.BillID;
		pItem->DlvrLocID = r_tg_rec.DlvrLocID;
		if(r_tg_rec.BillID && P_BObj->Fetch(r_tg_rec.BillID, &bill_rec) > 0) {
			pItem->BillCode_ = bill_rec.Code;
			pItem->OpID = bill_rec.OpID;
		}
		pItem->GoodsText_  = r_tg_rec.GoodsText;
		pItem->PersonText_ = r_tg_rec.PersonText;
		pItem->DtText_     = r_tg_rec.DtText;
		pItem->SubGoodsClsID = (IS_SGG_CLSSUBST(Filt.Sgg) && Gsl.GetItem(r_tg_rec.GoodsID, &gsi) > 0) ? gsi.ClsID : 0;
		pItem->Qtty      = r_tg_rec.Qtty;
		pItem->PhQtty    = r_tg_rec.PhQtty;
		pItem->Cost      = r_tg_rec.Cost;
		pItem->Price     = r_tg_rec.Price;
		pItem->Discount  = r_tg_rec.Discount;
		pItem->PVat      = r_tg_rec.PVat;
		pItem->Brutto    = r_tg_rec.Brutto;    // @v9.6.8
		pItem->LinkQtty  = r_tg_rec.LinkQtty;  // @v9.6.8
		pItem->LinkCost  = r_tg_rec.LinkCost;  // @v9.6.8
		pItem->LinkPrice = r_tg_rec.LinkPrice; // @v9.6.8
		pItem->ExtValue[0] = r_tg_rec.ExtVal1; // @v9.3.5
		pItem->SaldoQtty = r_tg_rec.SaldoQtty;
		pItem->SaldoAmt  = r_tg_rec.SaldoAmt;
		pItem->LocCount  = r_tg_rec.LocCount;
	}
	ASSIGN_PTR(P_InnerIterItem, *pItem); // @v9.3.5
}
//
//
//
class TrfrAnlzFiltDialog : public WLDialog {
public:
	enum {
		ctlgroupGoodsFilt = 1,
		ctlgroupLoc       = 2,
		ctlgroupArticle   = 3,
		ctlgroupAgent     = 4
	};
	TrfrAnlzFiltDialog() : WLDialog(DLG_TRFRANLZ, CTL_GTO_LABEL)
	{
		addGroup(ctlgroupGoodsFilt, new GoodsFiltCtrlGroup(CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, cmGoodsFilt));
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_GTO_LOC, 0, 0, cmLocList, 0, 0, 0));
		addGroup(ctlgroupArticle, new ArticleCtrlGroup(CTLSEL_GTO_ACS, CTLSEL_GTO_OPR, CTLSEL_GTO_OBJECT, cmArList, 0)); // @v8.0.9 GetSellAccSheet()-->0
		addGroup(ctlgroupAgent, new ArticleCtrlGroup(0, 0, CTLSEL_GTO_AGENT,  cmAgentList, GetAgentAccSheet()));
		SetupCalPeriod(CTLCAL_GTO_PERIOD, CTL_GTO_PERIOD);
		SetupCalPeriod(CTLCAL_GTO_LOTPERIOD, CTL_GTO_LOTPERIOD);
	}
	int    setDTS(const TrfrAnlzFilt * pData);
	int    getDTS(TrfrAnlzFilt * pData);
private:
	DECL_HANDLE_EVENT;
	// @v9.2.5 (@unused) void   setupAccSheet(PPID accSheetID);
	void   SetSaldoInfo();
	void   SetupCtrls();

	TrfrAnlzFilt Data;
};

void TrfrAnlzFiltDialog::SetupCtrls()
{
	long   flags = 0;
	long   id = 0;
	//
	const ushort v = getCtrlUInt16(CTL_GTO_GRPDATE);
	disableCtrl(CTL_GTO_ORDER, v);
	// @v9.4.10 {
	const PPID op_id = getCtrlLong(CTLSEL_GTO_OPR);
	const int  enable_cmp_wroff = BIN(op_id && (GCTIterator::AnalyzeOp(op_id, 0) & (GCTIterator::aorfThereAreDrafts|GCTIterator::aorfThereAreOrders)));
	DisableClusterItem(CTL_GTO_FLAGS, 4, !enable_cmp_wroff);
	// } @v9.4.10
	DisableClusterItem(CTL_GTO_FLAGS, 5, (!enable_cmp_wroff || !(Data.Flags & TrfrAnlzFilt::fCmpWrOff))); // @v9.5.8
	if(v) {
		setCtrlUInt16(CTL_GTO_ORDER, 0);
		Data.Grp = TrfrAnlzFilt::gNone;
		Data.Sgg = sggNone;
		Data.Sgp = sgpNone;
	}
	//
	GetClusterData(CTL_GTO_SHOWALL, &flags);
	if(flags & TrfrAnlzFilt::fShowAllArticles) {
		DisableClusterItem(CTL_GTO_SHOWALL, 1, 1);
		DisableClusterItem(CTL_GTO_SHOWALL, 2, 1);
		setCtrlData(CTLSEL_GTO_OBJECT, &id);
	}
	else if(flags & TrfrAnlzFilt::fShowAllAgents) {
		DisableClusterItem(CTL_GTO_SHOWALL, 0, 1);
		DisableClusterItem(CTL_GTO_SHOWALL, 2, 1);
		setCtrlData(CTLSEL_GTO_AGENT, &id);
	}
	else if(flags & TrfrAnlzFilt::fShowAllGoods) {
		GoodsFiltCtrlGroup::Rec gf_rec(0, 0, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(ctlgroupGoodsFilt, &gf_rec);
		DisableClusterItem(CTL_GTO_SHOWALL, 0, 1);
		DisableClusterItem(CTL_GTO_SHOWALL, 1, 1);
		enableCommand(cmGoodsFilt, 0);
	}
	else {
		DisableClusterItem(CTL_GTO_SHOWALL, 0, 0);
		DisableClusterItem(CTL_GTO_SHOWALL, 1, 0);
		DisableClusterItem(CTL_GTO_SHOWALL, 2, 0);
		const GoodsFiltCtrlGroup * p_grp = (const GoodsFiltCtrlGroup *)getGroup(ctlgroupGoodsFilt);
		if(!p_grp || !p_grp->IsGroupSelectionDisabled())
			enableCommand(cmGoodsFilt, 1);
	}
	disableCtrl(CTLSEL_GTO_OBJECT, flags & TrfrAnlzFilt::fShowAllArticles);
	disableCtrl(CTLSEL_GTO_AGENT,  flags & TrfrAnlzFilt::fShowAllAgents);
	if(flags & TrfrAnlzFilt::fShowAllGoods)
		disableCtrls(1, CTLSEL_GTO_GOODS, CTLSEL_GTO_GGRP, 0L);
}

int TrfrAnlzFiltDialog::setDTS(const TrfrAnlzFilt * pData)
{
	RVALUEPTR(Data, pData);

	ushort v;
	PPIDArray types;
	PPID   acc_sheet_id = 0;

	SetPeriodInput(this, CTL_GTO_PERIOD, &Data.Period);
	SetPeriodInput(this, CTL_GTO_LOTPERIOD, &Data.LotsPeriod);
	{
		LocationCtrlGroup::Rec loc_rec(&Data.LocList);
		setGroupData(ctlgroupLoc, &loc_rec);
	}
	SetupArCombo(this, CTLSEL_GTO_SUPPL, Data.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
	SetupArCombo(this, CTLSEL_GTO_SUPPLAG, Data.SupplAgentID, OLW_CANINSERT, GetAgentAccSheet(), sacfDisableIfZeroSheet); // @v8.4.1
	SetupPPObjCombo(this, CTLSEL_GTO_PSNCAT, PPOBJ_PRSNCATEGORY, Data.PsnCatID, OLW_LOADDEFONOPEN|OLW_CANINSERT);
	SetupPPObjCombo(this, CTLSEL_GTO_CITY, PPOBJ_WORLD, Data.CityID, OLW_LOADDEFONOPEN, PPObjWorld::MakeExtraParam(WORLDOBJ_CITY, 0, 0));
	{
		types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_GOODSRETURN, PPOPT_GOODSREVAL, PPOPT_GOODSMODIF,
			PPOPT_GOODSORDER, PPOPT_GENERIC, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT, 0L);
		SetupOprKindCombo(this, CTLSEL_GTO_OPR, Data.OpID, 0, &types, 0);
		ArticleCtrlGroup::Rec grp_rec(Data.AcsID, Data.OpID, &Data.ArList); // @v8.4.1 Data.AcsID
		setGroupData(ctlgroupArticle, &grp_rec);
	}
	{
		ArticleCtrlGroup::Rec grp_rec(GetAgentAccSheet(), 0, &Data.AgentList);
		setGroupData(ctlgroupAgent, &grp_rec);
	}
	{
		GoodsFiltCtrlGroup::Rec gf_rec(Data.GoodsGrpID, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel);
		setGroupData(ctlgroupGoodsFilt, &gf_rec);
	}
	setWL(BIN(Data.Flags & TrfrAnlzFilt::fLabelOnly));
	setCtrlUInt16(CTL_GTO_GRPDATE, v = BIN(Data.Flags & TrfrAnlzFilt::fGByDate));
	v = 0;
	if(getCtrlView(CTL_GTO_ORDER)) {
		switch(Data.InitOrd) {
			case PPViewTrfrAnlz::OrdByDate:    v = 0; break;
			case PPViewTrfrAnlz::OrdByGoods:   v = 1; break;
			case PPViewTrfrAnlz::OrdByArticle: v = 2; break;
			default: v = 0; break;
		}
		if(Data.Flags & TrfrAnlzFilt::fGByDate) {
			disableCtrl(CTL_GTO_ORDER, 1);
			v = 0;
		}
		setCtrlData(CTL_GTO_ORDER, &v);
	}
	if(getCtrlView(CTL_GTO_GENGGRP))
		setCtrlUInt16(CTL_GTO_GENGGRP, BIN(Data.Flags & OPG_GRPBYGENGOODS));
	AddClusterAssoc(CTL_GTO_SHOWALL, 0, TrfrAnlzFilt::fShowAllArticles);
	AddClusterAssoc(CTL_GTO_SHOWALL, 1, TrfrAnlzFilt::fShowAllAgents);
	AddClusterAssoc(CTL_GTO_SHOWALL, 2, TrfrAnlzFilt::fShowAllGoods);
	SetClusterData(CTL_GTO_SHOWALL, Data.Flags);
	AddClusterAssoc(CTL_GTO_FLAGS, 0, TrfrAnlzFilt::fCalcVat);
	AddClusterAssoc(CTL_GTO_FLAGS, 1, TrfrAnlzFilt::fCWoVat);
	AddClusterAssoc(CTL_GTO_FLAGS, 2, TrfrAnlzFilt::fShowGoodsCode);
	AddClusterAssoc(CTL_GTO_FLAGS, 3, TrfrAnlzFilt::fShowSerial);
	AddClusterAssoc(CTL_GTO_FLAGS, 4, TrfrAnlzFilt::fCmpWrOff); // @v9.4.10
	AddClusterAssoc(CTL_GTO_FLAGS, 5, TrfrAnlzFilt::fCmpWrOff_DiffOnly); // @v9.5.8
	SetClusterData(CTL_GTO_FLAGS, Data.Flags);
	SetupCtrls();
	SetSaldoInfo();
	return 1;
}

int TrfrAnlzFiltDialog::getDTS(TrfrAnlzFilt * pData)
{
	int    ok = 1;
	ushort v;
	GoodsFiltCtrlGroup::Rec gf_rec;
	THROW(GetPeriodInput(this, CTL_GTO_PERIOD,    &Data.Period));
	THROW(GetPeriodInput(this, CTL_GTO_LOTPERIOD, &Data.LotsPeriod));
	{
		LocationCtrlGroup::Rec loc_rec;
		getGroupData(ctlgroupLoc, &loc_rec);
		Data.LocList = loc_rec.LocList;
	}
	getCtrlData(CTLSEL_GTO_SUPPL, &Data.SupplID);
	getCtrlData(CTLSEL_GTO_SUPPLAG, &Data.SupplAgentID); // @v8.4.1
	{
		ArticleCtrlGroup::Rec grp_rec;
		getGroupData(ctlgroupArticle, &grp_rec);
		Data.OpID = grp_rec.OpID;
		Data.AcsID = grp_rec.AcsID; // @v8.4.1
		Data.ArList = grp_rec.ArList;
	}
	{
		ArticleCtrlGroup::Rec grp_rec;
		getGroupData(ctlgroupAgent, &grp_rec);
		Data.AgentList = grp_rec.ArList;
	}
	getCtrlData(CTLSEL_GTO_CITY,   &Data.CityID);
	getCtrlData(CTLSEL_GTO_PSNCAT, &Data.PsnCatID);
	THROW(getGroupData(ctlgroupGoodsFilt, &gf_rec));
	Data.GoodsGrpID = gf_rec.GoodsGrpID;
	Data.GoodsID    = gf_rec.GoodsID;
	SETFLAG(Data.Flags, TrfrAnlzFilt::fLabelOnly, getWL());
	v = getCtrlUInt16(CTL_GTO_GRPDATE);
	SETFLAG(Data.Flags, TrfrAnlzFilt::fGByDate, v);
	THROW_PP(v != 1 || Data.GoodsID, PPERR_GOODSNEEDED);
	if(getCtrlView(CTL_GTO_ORDER)) {
		switch(getCtrlUInt16(CTL_GTO_ORDER)) {
			case 0: Data.InitOrd = PPViewTrfrAnlz::OrdByDate;    break;
			case 1: Data.InitOrd = PPViewTrfrAnlz::OrdByGoods;   break;
			case 2: Data.InitOrd = PPViewTrfrAnlz::OrdByArticle; break;
		}
	}
	SETFLAG(Data.Flags, OPG_GRPBYGENGOODS, getCtrlUInt16(CTL_GTO_GENGGRP));
	GetClusterData(CTL_GTO_SHOWALL, &Data.Flags);
	GetClusterData(CTL_GTO_FLAGS, &Data.Flags);
	ASSIGN_PTR(pData, Data);
	CATCHZOKPPERR
	return ok;
}

void TrfrAnlzFiltDialog::SetSaldoInfo()
{
	int   show_saldo_info = 0;
	PPID  ar_id = 0;
	GoodsFiltCtrlGroup::Rec  rec;
	getCtrlData(CTLSEL_GTO_OBJECT, &ar_id);
	if(getGroupData(ctlgroupGoodsFilt, &rec) && rec.GoodsGrpID &&
		((Data.Grp == TrfrAnlzFilt::gGoods && ar_id) || (Data.Grp == TrfrAnlzFilt::gCntragent && rec.GoodsID))
		&& Data.Sgg == sggNone && Data.Sgp == sgpNone) {
		SString temp_buf, txt_buf;
		LDATE  dt = ZERODATE;
		GoodsSaldoCore GSCore;
		GSCore.GetLastCalcDate(rec.GoodsGrpID, rec.GoodsID, ar_id, 0/*dlvrLocID*/, &dt);
		PPGetSubStr(PPTXT_TRFRANLZ_SALDOINFO, 0, txt_buf);
		temp_buf.Cat(txt_buf).CatDiv(':', 2);
		if(dt)
			temp_buf.Cat(dt);
		else {
			PPGetSubStr(PPTXT_TRFRANLZ_SALDOINFO, 1, txt_buf); // нет расчета
			temp_buf.Cat(txt_buf);
		}
		setStaticText(CTL_GTO_SALDOINFO, temp_buf);
		show_saldo_info = 1;
	}
	showCtrl(CTL_GTO_SALDOINFO, show_saldo_info);
}
//
//
//
class TrfrAnlzGrpngDialog : public TDialog {
public:
	TrfrAnlzGrpngDialog() : TDialog(DLG_TAGRPNG)
	{
	}
	int    setDTS(const TrfrAnlzFilt * pData);
	int    getDTS(TrfrAnlzFilt * pData);
private:
	DECL_HANDLE_EVENT;
	void   SetupCtrls();
	void   GetRestParams();
	TrfrAnlzFilt Data;
};

#define SHOW_CTVAL 0x00000001L

class TrfrAnlzCtDialog : public PPListDialog {
public:
	TrfrAnlzCtDialog() : PPListDialog(DLG_TAC, CTL_TAC_VALLIST)
	{
		PPLoadText(PPTXT_TRFRANLZCTVALNAMES, CtValNames);
		setSmartListBoxOption(CTL_TAC_VALLIST, lbtSelNotify);
	}
	int    setDTS(const TrfrAnlzFilt * pData)
	{
		if(!RVALUEPTR(Data, pData))
			Data.Init(1, 0);
		if(!Data.HasDateGrouping()) {
			DisableClusterItem(CTL_TAC_KIND, 1, 1);
			Data.CtKind = (Data.CtKind == TrfrAnlzFilt::ctDate) ? TrfrAnlzFilt::ctNone : Data.CtKind;
		}
		if(!Data.HasCntragentGrouping()) {
			DisableClusterItem(CTL_TAC_KIND, 2, 1);
			Data.CtKind = (Data.CtKind == TrfrAnlzFilt::ctCntragent) ? TrfrAnlzFilt::ctNone : Data.CtKind;
		}
		DisableClusterItem(CTL_TAC_KIND, 3, 1);

		AddClusterAssocDef(CTL_TAC_KIND, 0, TrfrAnlzFilt::ctNone);
		AddClusterAssoc(CTL_TAC_KIND, 1, TrfrAnlzFilt::ctDate);
		AddClusterAssoc(CTL_TAC_KIND, 2, TrfrAnlzFilt::ctCntragent);
		AddClusterAssoc(CTL_TAC_KIND, 3, TrfrAnlzFilt::ctLocation);
		SetClusterData(CTL_TAC_KIND, Data.CtKind);

		if(!Data.CtValList.IsExists())
			Data.CtValList.InitEmpty();
		updateList(-1);
		return 1;
	}
	int    getDTS(TrfrAnlzFilt * pData)
	{
		int    ok = 1;
		GetClusterData(CTL_TAC_KIND, &Data.CtKind);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isKeyDown(kbSpace)) {
			long sel_id = 0;
			if(getSelection(&sel_id) && sel_id) {
				ToggleFlag(sel_id);
				updateList(-1);
			}
			clearEvent(event);
		}
	}
	virtual int setupList();
	virtual int editItem(long pos, long id)
	{
		return ToggleFlag(id);
	}
	int    ToggleFlag(long itemId);
	SString CtValNames;
	TrfrAnlzFilt Data;
};

int TrfrAnlzCtDialog::setupList()
{
	int    ok = 1;
	SString buf;
	StringSet text_list(';', CtValNames);
	StringSet ss(SLBColumnDelim);
	for(uint i = 0, j = 1; ok && text_list.get(&i, buf) > 0; j++) {
		ss.clear();
		ss.add(buf);
		buf.Z().CatChar(Data.CtValList.CheckID(j) ? 'v' : ' ');
		ss.add(buf);
		ok = addStringToList(j, ss.getBuf());
	}
	return ok;
}

int TrfrAnlzCtDialog::ToggleFlag(long itemId)
{
	if(itemId) {
		if(Data.CtValList.CheckID(itemId)) {
			Data.CtValList.Remove(itemId);
		}
		else {
			Data.CtValList.Add(itemId);
			if(oneof3(itemId, TrfrAnlzFilt::ctvIncome, TrfrAnlzFilt::ctvPctIncome, TrfrAnlzFilt::ctvPctMargin)) {
				if(itemId != TrfrAnlzFilt::ctvIncome)
					Data.CtValList.Remove(TrfrAnlzFilt::ctvIncome);
				if(itemId != TrfrAnlzFilt::ctvPctIncome)
					Data.CtValList.Remove(TrfrAnlzFilt::ctvPctIncome);
				if(itemId != TrfrAnlzFilt::ctvPctMargin)
					Data.CtValList.Remove(TrfrAnlzFilt::ctvPctMargin);
			}
		}
		return 1;
	}
	else
		return -1;
}

void TrfrAnlzGrpngDialog::SetupCtrls()
{
	getCtrlData(CTLSEL_TAGRPNG_GOODS, &Data.Sgg);
	getCtrlData(CTLSEL_TAGRPNG_CNTRAGENT, &Data.Sgp);
	getCtrlData(CTLSEL_TAGRPNG_GGRPNG,    &Data.Grp);
	GetRestParams();
	disableCtrls(Data.Sgp == sgpCategory, CTL_TAGRPNG_DIFFDLVRADDR, CTL_TAGRPNG_SUBSTRADDR, 0);
	if(Data.Sgp == sgpCategory)
		setCtrlUInt16(CTL_TAGRPNG_DIFFDLVRADDR, 0);
	{
		const int disable_calc_rest = BIN(Data.Sgg || !oneof2(Data.Grp, TrfrAnlzFilt::gGoods, TrfrAnlzFilt::gGoodsDate));
		disableCtrl(CTL_TAGRPNG_CALCREST, disable_calc_rest);
		const int disable_trnovr = BIN(disable_calc_rest || !(Data.Flags & TrfrAnlzFilt::fCalcRest));
		disableCtrl(CTL_TAGRPNG_TURNOVER, disable_trnovr);
	}
}

IMPL_HANDLE_EVENT(TrfrAnlzGrpngDialog)
{
	TDialog::handleEvent(event);
	if(event.isCmd(cmEditCtParam)) {
		getDTS(/*&Data*/0);
		if(!PPDialogProcBody <TrfrAnlzCtDialog, TrfrAnlzFilt> (&Data))
			PPError();
	}
	else if(event.isClusterClk(CTL_TAGRPNG_DIFFDLVRADDR)) {
		disableCtrls(getCtrlUInt16(CTL_TAGRPNG_DIFFDLVRADDR), CTLSEL_TAGRPNG_CNTRAGENT, CTL_TAGRPNG_SUBSTRADDR, 0);
	}
	else if(event.isCbSelected(CTLSEL_TAGRPNG_CNTRAGENT) ||
		event.isCbSelected(CTLSEL_TAGRPNG_GOODS) || event.isCbSelected(CTLSEL_TAGRPNG_GGRPNG) ||
		event.isClusterClk(CTL_TAGRPNG_CALCREST))
		SetupCtrls();
	else
		return;
	clearEvent(event);
}

int TrfrAnlzGrpngDialog::setDTS(const TrfrAnlzFilt * pData)
{
	Data = *pData;
	disableCtrls(BIN(Data.Flags & TrfrAnlzFilt::fDiffByDlvrAddr), CTLSEL_TAGRPNG_CNTRAGENT, CTL_TAGRPNG_SUBSTRADDR, 0);
	ushort v = 0;
	setCtrlOption(CTL_TAGRPNG_FRAME, ofFramed, 1);
	AddClusterAssoc(CTL_TAGRPNG_DIFFDLVRADDR, 0, TrfrAnlzFilt::fDiffByDlvrAddr);
	SetClusterData(CTL_TAGRPNG_DIFFDLVRADDR, Data.Flags);
	SetupStringCombo(this, CTLSEL_TAGRPNG_GGRPNG, PPTXT_GOODSGRPNGLIST, (long)Data.Grp);
	SetupSubstGoodsCombo(this, CTLSEL_TAGRPNG_GOODS, Data.Sgg);
	SetupSubstPersonCombo(this, CTLSEL_TAGRPNG_CNTRAGENT, Data.Sgp);
	SetupSubstDateCombo(this, CTLSEL_TAGRPNG_DATE, Data.Sgd);
	if(Data.Flags & TrfrAnlzFilt::fSubstPersonRAddr)
		v = 1;
	else if(Data.Flags & TrfrAnlzFilt::fSubstDlvrAddr)
		v = 2;
	else
		v = 0;
	setCtrlData(CTL_TAGRPNG_SUBSTRADDR, &v);
	// @v9.1.3 {
	{
		long   calc_rest_param = 0;
		if(Data.Flags & TrfrAnlzFilt::fCalcRest)
			calc_rest_param |= 0x01;
		if(Data.Flags & TrfrAnlzFilt::fCalcAvgRest)
			calc_rest_param |= 0x02;
		AddClusterAssoc(CTL_TAGRPNG_CALCREST, 0, 0);
		AddClusterAssoc(CTL_TAGRPNG_CALCREST, 1, 0x01);
		AddClusterAssoc(CTL_TAGRPNG_CALCREST, 2, 0x01|0x02);
		SetClusterData(CTL_TAGRPNG_CALCREST, calc_rest_param);
	}
	// } @v9.1.3
	// @v9.1.5 {
	{
		AddClusterAssoc(CTL_TAGRPNG_TURNOVER, 0, TrfrAnlzFilt::ravTurnoverRate);
		SetClusterData(CTL_TAGRPNG_TURNOVER, Data.RestAddendumValue);
	}
	{
		StrAssocArray qk_list_addendum;
		PPObjQuotKind qk_obj;
		PPQuotKind qk_rec;
        for(SEnum en = qk_obj.Enum(0); en.Next(&qk_rec) > 0;) {
			qk_list_addendum.AddFast(qk_rec.ID + TrfrAnlzFilt::extvQuotBias, qk_rec.Name);
        }
		qk_list_addendum.SortByText();
		//
		SetupStringComboWithAddendum(this, CTLSEL_TAGRPNG_EXTVAL1, "trfranlz_enum_extval", &qk_list_addendum, Data.ExtValueParam[0]);
	}
	// } @v9.1.5
	SetupCtrls();
	return 1;
}

void TrfrAnlzGrpngDialog::GetRestParams()
{
	long   calc_rest_param = 0;
	GetClusterData(CTL_TAGRPNG_CALCREST, &calc_rest_param);
	if(calc_rest_param == (0x01|0x02)) {
		Data.Flags |= (TrfrAnlzFilt::fCalcRest|TrfrAnlzFilt::fCalcAvgRest);
	}
	else if(calc_rest_param == 0x01) {
		Data.Flags |= TrfrAnlzFilt::fCalcRest;
		Data.Flags &= ~TrfrAnlzFilt::fCalcAvgRest;
	}
	else {
		Data.Flags &= ~(TrfrAnlzFilt::fCalcRest|TrfrAnlzFilt::fCalcAvgRest);
	}
}

int TrfrAnlzGrpngDialog::getDTS(TrfrAnlzFilt * pData)
{
	ushort v;
	getCtrlData(CTLSEL_TAGRPNG_GGRPNG,    &Data.Grp);
	getCtrlData(CTLSEL_TAGRPNG_GOODS,     &Data.Sgg);
	getCtrlData(CTLSEL_TAGRPNG_CNTRAGENT, &Data.Sgp);
	getCtrlData(CTLSEL_TAGRPNG_DATE,      &Data.Sgd);
	getCtrlData(CTL_TAGRPNG_SUBSTRADDR, &(v = 0));
	Data.Flags &= ~(TrfrAnlzFilt::fSubstPersonRAddr | TrfrAnlzFilt::fSubstDlvrAddr);
	if(v == 1)
		Data.Flags |= TrfrAnlzFilt::fSubstPersonRAddr;
	else if(v == 2)
		Data.Flags |= TrfrAnlzFilt::fSubstDlvrAddr;
	GetClusterData(CTL_TAGRPNG_DIFFDLVRADDR, &Data.Flags);
	if(Data.Flags & TrfrAnlzFilt::fDiffByDlvrAddr)
		Data.Sgp = sgpNone;
	GetRestParams();
	GetClusterData(CTL_TAGRPNG_TURNOVER, &Data.RestAddendumValue); // @v9.1.5
	getCtrlData(CTLSEL_TAGRPNG_EXTVAL1, &Data.ExtValueParam[0]); // @v9.1.5
	ASSIGN_PTR(pData, Data);
	return 1;
}

IMPL_HANDLE_EVENT(TrfrAnlzFiltDialog)
{
	WLDialog::handleEvent(event);
	if(event.isCmd(cmTaGrpngOptions)) {
		PPDialogProcBody <TrfrAnlzGrpngDialog, TrfrAnlzFilt> (&Data);
		SetSaldoInfo();
	}
	else if(event.isCmd(cmCBSelected)) {
		SetSaldoInfo();
		// @v9.4.10 {
		if(event.isCtlEvent(CTLSEL_GTO_OPR))
			SetupCtrls();
		// } @v9.4.10
	}
	else if(event.isClusterClk(CTL_GTO_GRPDATE)) {
		SetupCtrls();
		SetSaldoInfo();
	}
	else if(event.isClusterClk(CTL_GTO_SHOWALL))
		SetupCtrls();
	// @v9.5.8 {
	else if(event.isClusterClk(CTL_GTO_FLAGS)) {
		GetClusterData(CTL_GTO_FLAGS, &Data.Flags);
		SetupCtrls();
	}
	// } @v9.5.8
	else
		return;
	clearEvent(event);
}

int SLAPI PPViewTrfrAnlz::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	if(Filt.IsA(pBaseFilt)) {
		DIALOG_PROC_BODY(TrfrAnlzFiltDialog, (TrfrAnlzFilt *)pBaseFilt);
	}
	else
		return 0;
}

int SLAPI PPViewTrfrAnlz::Export()
{
	int    ok = 1;
	SString path;
	TrfrAnlzViewItem item;
	DbfTable * p_out_tbl = 0;
	PPWait(1);
	PPGetFilePath(PPPATH_OUT, PPFILNAM_TRFRANLZ_DBF, path);
	THROW(p_out_tbl = CreateDbfTable(DBFS_TRFRANLZLIC, path, 1));
	for(InitIteration(CurViewOrd); NextIteration(&item) > 0;) {
		double amount = (IsSellingOp(item.OpID) > 0) ? item.Price : item.Cost;
		DbfRecord dbfr(p_out_tbl);
		dbfr.empty();
		dbfr.put( 1, item.Dt);
		dbfr.put( 2, item.OprNo);
		dbfr.put( 3, item.BillID);
		dbfr.put( 4, item.BillCode_);
		dbfr.put( 5, item.GoodsID);
		dbfr.put( 6, item.ArticleID);
		dbfr.put( 7, item.GoodsText_);
		dbfr.put( 8, item.PersonText_);
		dbfr.put( 9, item.Qtty);
		dbfr.put(10, item.PhQtty);
		dbfr.put(11, item.Cost);
		dbfr.put(12, item.Price);
		dbfr.put(13, item.Discount);
		dbfr.put(14, amount);
		dbfr.put(15, item.OpID);
		THROW_PP(p_out_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
		PPWaitPercent(GetCounter());
	}
	PPWait(0);
	CATCHZOKPPERR
	delete p_out_tbl;
	return ok;
}

int SLAPI PPViewTrfrAnlz::CalcTotal(TrfrAnlzTotal * pTotal)
{
	if(pTotal) {
		pTotal->destroy();
		TrfrAnlzViewItem item;
		PPWait(1);
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			pTotal->Count++;
			pTotal->Qtty     += item.Qtty;
			pTotal->PhQtty   += item.PhQtty;
			if(Flags & fAsGoodsCard) {
				pTotal->Cost     += (item.Cost * item.Qtty);
				pTotal->Price    += (item.Price * item.Qtty);
				pTotal->Amount   += (item.Amount * item.Qtty);
			}
			else {
				pTotal->Cost     += item.Cost;
				pTotal->Price    += item.Price;
				pTotal->Discount += item.Discount;
				pTotal->Amount   += item.Amount;
			}
			pTotal->SaldoQtty += item.SaldoQtty;
			pTotal->SaldoAmt  += item.SaldoAmt;
			pTotal->PVat      += item.PVat;
			pTotal->ExtValue[0] += item.ExtValue[0]; // @v9.7.5
			pTotal->ExtValue[1] += item.ExtValue[1]; // @v9.7.5
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
		return 1;
	}
	else
		return -1;
}

int SLAPI PPViewTrfrAnlz::ViewTotal()
{
	TDialog * dlg = 0;
	if(Total.Count > 0 || CalcTotal(&Total) > 0) {
		if(CheckDialogPtrErr(&(dlg = new TDialog((Flags & fShowSaldo) ? DLG_TRFRANLZTOTAL_S : DLG_TRFRANLZTOTAL)))) {
			dlg->setCtrlLong(CTL_TRFRANLZTOTAL_COUNT,  Total.Count);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_QTTY,   Total.Qtty);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_PHQTTY, Total.PhQtty);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_COST,   Total.Cost);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_PRICE,  Total.Price);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_DIS,    Total.Discount);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_AMOUNT, Total.Amount);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_SQTTY,  Total.SaldoQtty);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_SAMT,   Total.SaldoAmt);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_PVAT,   Total.PVat);
			dlg->setCtrlReal(CTL_TRFRANLZTOTAL_EXTV1,  Total.ExtValue[0]); // @v9.7.5
			ExecViewAndDestroy(dlg);
		}
	}
	return -1;
}

int SLAPI PPViewTrfrAnlz::SetAlcRepParam(AlcReportParam * pParam)
{
	if(!RVALUEPTR(AlcRepParam, pParam))
		MEMSZERO(AlcRepParam);
	return 1;
}

int SLAPI PPViewTrfrAnlz::NextIteration_AlcRep(TrfrAnlzViewItem_AlcRep * pItem)
{
	int    ok = -1;
	TrfrAnlzViewItem item;
	if(NextIteration(&item) > 0) {
		PPID   psn_id = 0, org_lot_id = 0;
		SString temp_buf;
		ReceiptTbl::Rec lot;
		THROW(GObj.Fetch(item.GoodsID, &pItem->GoodsRec));
		if(pItem->GoodsRec.GdsClsID)
			THROW(GdsClsObj.Fetch(pItem->GoodsRec.GdsClsID, &pItem->GCPack));
		THROW(GObj.GetStockExt(pItem->GoodsRec.ID, &pItem->GoodsStock, 1));
		THROW(GObj.P_Tbl->GetExt(pItem->GoodsRec.ID, &pItem->GoodsExt));
		pItem->Item = item;
		// не будем извлекать пока что, так как billrec = bill packet pItem->BillRec;
		THROW(P_BObj->trfr->Rcpt.SearchOrigin(item.LotID, &org_lot_id, &lot, &pItem->OrgLotRec));
		// @v9.1.12 {
		if(item.LotID == org_lot_id && GetOpType(item.OpID) == PPOPT_GOODSRECEIPT) {
			BillTbl::Rec cor_bill_rec;
			PPIDArray cor_bill_list;
			for(DateIter diter; P_BObj->P_Tbl->EnumLinks(item.BillID, &diter, BLNK_CORRECTION, &cor_bill_rec) > 0;) {
				cor_bill_list.add(cor_bill_rec.ID);
			}
			if(cor_bill_list.getCount()) {
				PPBillPacket cor_bp;
				for(uint i = 0; i < cor_bill_list.getCount(); i++) {
					const PPID cor_bill_id = cor_bill_list.get(i);
					THROW(P_BObj->ExtractPacket(cor_bill_id, &cor_bp) > 0);
					for(uint j = 0; j < cor_bp.GetTCount(); j++) {
						const PPTransferItem & r_other = cor_bp.ConstTI(j);
						if(r_other.LotID == item.LotID) {
							pItem->Item.Qtty = r_other.Quantity_;
							// @todo Здесь необходимо так же пересчитать физическое количество и суммы.
							// Сразу это не сделано из-за того, что надо было быстро, а для алкогольной декларации это - не обязательно.
							break;
						}
					}
				}
			}
		}
		// } @v9.1.12
		{
			ObjTagItem tag;
			//pItem->IsImport   = 0;
			//pItem->IsExport   = 0;
			//pItem->IsManuf    = 0;
			//pItem->IsOptBuyer = 0;
			pItem->Flags = 0;
			psn_id = ObjectToPerson(item.ArticleID);
			pItem->PersonID = psn_id;
			pItem->OrgLot_Prsn_SupplID = ObjectToPerson(pItem->OrgLotRec.SupplID);
			if(AlcRepParam.ImpExpTag) {
				if(TagObj.FetchTag(psn_id, AlcRepParam.ImpExpTag, &tag) > 0) {
					tag.GetStr(temp_buf);
					const long val = temp_buf.ToLong();
					//pItem->IsOptBuyer = BIN(val == 1);
					//pItem->IsExport   = BIN(val == 2);
					SETFLAG(pItem->Flags, pItem->fIsOptBuyer, (val == 1));
					SETFLAG(pItem->Flags, pItem->fIsExport,   (val == 2));
				}
			}
			if(AlcRepParam.ManufOptBuyerTag) {
				if(TagObj.FetchTag(psn_id, AlcRepParam.ManufOptBuyerTag, &tag) > 0) {
					tag.GetStr(temp_buf);
					const long val = temp_buf.ToLong();
					//pItem->IsManuf  = BIN(val == 1);
					//pItem->IsImport = BIN(val == 2);
					SETFLAG(pItem->Flags, pItem->fIsManuf,  (val == 1));
					SETFLAG(pItem->Flags, pItem->fIsImport, (val == 2));
				}
			}
			else {
				if(AlcRepParam.ImportKindID) {
        			SETFLAG(pItem->Flags, pItem->fIsImport, PsnObj.P_Tbl->IsBelongToKind(psn_id, AlcRepParam.ImportKindID));
				}
				if(AlcRepParam.ManufKindID) {
					SETFLAG(pItem->Flags, pItem->fIsManuf, PsnObj.P_Tbl->IsBelongToKind(psn_id, AlcRepParam.ManufKindID));
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI ViewTrfrAnlz(const TrfrAnlzFilt * pFilt) { return PPView::Execute(PPVIEW_TRFRANLZ, pFilt, PPView::exefModeless, 0); }
//
// Implementation of PPALDD_TrfrAnlzBase
//
PPALDD_CONSTRUCTOR(TrfrAnlzBase)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TrfrAnlzBase) { Destroy(); }

int PPALDD_TrfrAnlzBase::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(TrfrAnlz, rsrv);
	p_v->AllocInnerIterItem(); // @v9.3.5
	H.FltBeg        = p_filt->Period.low;
	H.FltEnd        = p_filt->Period.upp;
	H.FltOpID       = p_filt->OpID;
	H.FltLocID      = p_filt->LocList.GetSingle();
	H.FltSupplID    = p_filt->SupplID;
	H.FltObjectID   = p_filt->ArList.GetSingle();
	H.FltGoodsGrpID = p_filt->GoodsGrpID;
	H.FltGoodsID    = p_filt->GoodsID;
	H.Flags         = p_filt->Flags;
	H.fLabelOnly    = BIN(p_filt->Flags & TrfrAnlzFilt::fLabelOnly);
	H.Grp = p_filt->Grp;
	H.Sgg = p_filt->Sgg;
	H.Sgp = p_filt->Sgp;
	H.Sgd = p_filt->Sgd;
	if(p_filt->CtValList.GetCount()) {
		PPIDArray ctval_list;
		p_filt->CtValList.CopyTo(&ctval_list);
		H.CtValKind = (int16)ctval_list.at(0);
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TrfrAnlzBase::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewTrfrAnlz * p_v = (PPViewTrfrAnlz*)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	H.Ord = (int16)sortId;
	return BIN(p_v->InitIteration((PPViewTrfrAnlz::IterOrder)sortId));
}

int PPALDD_TrfrAnlzBase::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(TrfrAnlz);
	if(H.Grp == TrfrAnlzFilt::gDateCntragentAgentGoods)
		I._AgentID = item.BillID;
	else
		I.BillID   = item.BillID;
	I.LotID = item.LotID;
	I.DlvrLocID = item.DlvrLocID;
	I.GoodsID  = item.GoodsID;
	I.OpID     = item.OpID;
	I.LocID    = item.LocID;
	I.ObjectID = item.ArticleID;
	I.ContragentID = item.PersonID;
	I.Dt = item.Dt;
	STRNSCPY(I.GoodsText, item.GoodsText_);
	STRNSCPY(I.PersonText, item.PersonText_);
	STRNSCPY(I.DtText, item.DtText_);
	I.Qtty     = item.Qtty;
	I.PhQtty   = item.PhQtty;
	I.Rest     = item.Rest;
	I.Cost     = item.Cost;
	I.Price    = item.Price;
	I.Discount = item.Discount;
	I.PVat     = item.PVat;
	I.Amount   = item.Amount;
	I.SalQtty  = item.SaldoQtty;
	I.SalAmt   = item.SaldoAmt;
	const TrfrAnlzFilt * p_filt = (const TrfrAnlzFilt *)p_v->GetBaseFilt();
	long ctval = 0;
	I.LocCount = item.LocCount;
	if(p_filt->CtValList.GetCount()) {
		PPIDArray ctval_list;
		p_filt->CtValList.CopyTo(&ctval_list);
		ctval = ctval_list.at(0);
	}
	if(ctval == TrfrAnlzFilt::ctvCost)
		I.CtVal = item.Cost;
	else if(ctval == TrfrAnlzFilt::ctvNetPrice)
		I.CtVal = item.Price;
	else if(ctval == TrfrAnlzFilt::ctvPctMargin)
		I.CtVal = fdivnz(100.0 * (I.Price - I.Cost), I.Price);
	else if(ctval == TrfrAnlzFilt::ctvPctIncome)
		I.CtVal = fdivnz(100.0 * (I.Price - I.Cost), I.Cost);
	else if(ctval == TrfrAnlzFilt::ctvIncome)
		I.CtVal = (I.Price - I.Cost);
	else if(ctval == TrfrAnlzFilt::ctvQtty)
		I.CtVal = I.Qtty;
	else if(ctval == TrfrAnlzFilt::ctvLocCount)
		I.CtVal = item.LocCount;
	if(H.Ord == PPViewTrfrAnlz::OrdByDate)
		I.RptGrpngVal = item.Dt.v;
	else if(H.Ord == PPViewTrfrAnlz::OrdByGoods)
		I.RptGrpngVal = item.GoodsID;
	else if(H.Ord == PPViewTrfrAnlz::OrdByArticle)
		I.RptGrpngVal = item.PersonID;
	else
		I.RptGrpngVal = 0;
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_TrfrAnlzBase::Destroy()
{
	DESTROY_PPVIEW_ALDD(TrfrAnlz);
}

void PPALDD_TrfrAnlzBase::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**(SString **)rS.GetPtr(pApl->Get(n)))
	#define _ARG_INT(n)  (*(int *)rS.GetPtr(pApl->Get(n)))
	#define _RET_DBL     (*(double *)rS.GetPtr(pApl->Get(0)))
	#define _RET_INT     (*(int *)rS.GetPtr(pApl->Get(0)))
	#define _RET_LONG    (*(long *)rS.GetPtr(pApl->Get(0)))
	PPViewTrfrAnlz * p_v = (PPViewTrfrAnlz*)NZOR(Extra[1].Ptr, Extra[0].Ptr);
	if(pF->Name == "?GetExtVal") {
		double ext_val = 0.0;
		const TrfrAnlzViewItem * p_item = p_v ? p_v->GetInnerIterItem() : 0;
		if(p_item) {
			if(_ARG_INT(1) == 1)
				ext_val = p_item->ExtValue[0];
		}
		_RET_DBL = ext_val;
	}
	else if(pF->Name == "?GetLinkBillID") {
		const TrfrAnlzViewItem * p_item = p_v ? p_v->GetInnerIterItem() : 0;
		_RET_LONG = p_item ? p_item->LinkBillID : 0;
	}
	else if(pF->Name == "?GetLinkValue") {
		double val = 0.0;
		const TrfrAnlzViewItem * p_item = p_v ? p_v->GetInnerIterItem() : 0;
		if(p_item) {
			switch(_ARG_INT(1)) {
				case 1: val = p_item->LinkQtty;  break;
				case 2: val = p_item->LinkCost;  break;
				case 3: val = p_item->LinkPrice; break;
			}
		}
		_RET_DBL = val;
	}
}
//
//
//
PrcssrAlcReport::Config::Config() : P_CcFilt(0)
{
	Clear();
}

SLAPI PrcssrAlcReport::Config::Config(const Config & rS) : P_CcFilt(0)
{
	Copy(rS);
}

PrcssrAlcReport::Config & FASTCALL PrcssrAlcReport::Config::operator = (const Config & rS)
{
    Copy(rS);
    return *this;
}

PrcssrAlcReport::Config::~Config()
{
	ZDELETE(P_CcFilt);
}

int FASTCALL PrcssrAlcReport::Config::Copy(const Config & rS)
{
	int    ok = 1;
	memmove(this, &rS, offsetof(PrcssrAlcReport::Config, StorageLocList));
	StorageLocList = rS.StorageLocList;
	LotManufTagList = rS.LotManufTagList;
	if(rS.P_CcFilt) {
		THROW_MEM(SETIFZ(P_CcFilt, new CCheckFilt));
		*P_CcFilt = *rS.P_CcFilt;
	}
	else {
		ZDELETE(P_CcFilt);
	}
	CATCHZOK
	return ok;
}

PrcssrAlcReport::Config & SLAPI PrcssrAlcReport::Config::Clear()
{
	memzero(this, offsetof(PrcssrAlcReport::Config, StorageLocList));
	StorageLocList.clear();
	LotManufTagList.clear();
	ZDELETE(P_CcFilt);
	return *this;
}

int SLAPI PrcssrAlcReport::Config::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	if(dir > 0) {
		Ver = DS.GetVersion();
	}
	THROW_SL(Ver.Serialize(dir, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, RcptOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, SaleRetOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, RcptEtcOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ExpndOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, SupplRetOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ExpndEtcOpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, IntrExpndOpID, rBuf));

	THROW_SL(pSCtx->Serialize(dir, AlcGoodsGrpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, BeerGoodsGrpID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, CategoryTagID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, CategoryClsDim, rBuf));
	THROW_SL(pSCtx->Serialize(dir, VolumeClsDim, rBuf));
	THROW_SL(pSCtx->Serialize(dir, AlcLicRegTypeID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, KppRegTypeID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, WhsExpTagID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, ManufImpTagID, rBuf));
	THROW_SL(pSCtx->Serialize(dir, SubstCategoryCode, sizeof(SubstCategoryCode), rBuf));
	THROW_SL(pSCtx->SerializeBlock(dir, sizeof(E), &E, rBuf, 0));
	THROW_SL(pSCtx->Serialize(dir, &StorageLocList, rBuf));
	THROW_SL(pSCtx->Serialize(dir, &LotManufTagList, rBuf));
	// @v9.4.0 {
	if(dir > 0) {
		if(!Ver.IsLt(9, 4, 0)) {
			THROW(PPView::WriteFiltPtr(rBuf, P_CcFilt));
		}
	}
	else if(dir < 0) {
		if(Ver.IsLt(9, 4, 0)) {
            ZDELETE(P_CcFilt);
		}
		else {
			THROW(PPView::ReadFiltPtr(rBuf, (PPBaseFilt **)&P_CcFilt));
		}
	}
	// } @v9.4.0
	CATCHZOK
	return ok;
}

PrcssrAlcReport::GoodsItem::GoodsItem()
{
	Clear();
}

PrcssrAlcReport::GoodsItem & PrcssrAlcReport::GoodsItem::Clear()
{
	StatusFlags = 0;
	GoodsID = 0;
	LotID = 0;
	MnfOrImpPsnID = 0;
	Volume = 0.0;
	Brutto = 0.0;
	Proof = 0.0;
	BottlingDate = ZERODATE;
	CountryCode = 0;
	OuterUnpackedTag = 0; // @v9.5.10
	UnpackedVolume = 0.0;
	CategoryCodePos = 0;
	CategoryCode.Z();
	CategoryName.Z();
	MsgPool.Z();
	EgaisCode.Z();
	InformA.Z();
	InformB.Z();

	RefcInfA_ActualDate = ZERODATE;
	RefcPr_ActualDate = ZERODATE;
	RefcProductID = 0;
	RefcManufID = 0;
	RefcImporterID = 0;
	RefcInfAID = 0;
	RefcVolume = 0.0;
	RefcProof = 0.0;
	RefcCategoryCode.Z();
	RefcEgaisCode.Z();
	RefcManufCode.Z();
	RefcImporterCode.Z();

	return *this;
}

SLAPI PrcssrAlcReport::PrcssrAlcReport() : P_RefC(0), P_BObj(BillObj)
{
	DS.FetchAlbatrosConfig(&ACfg);
}

SLAPI PrcssrAlcReport::~PrcssrAlcReport()
{
	P_BObj = 0;
	ZDELETE(P_RefC);
}

int SLAPI PrcssrAlcReport::Init()
{
	int    ok = 1;
	AlcGoodsList.clear();
	BeerGoodsList.clear();
	{
		CategoryNameList.Clear();
		SString file_name;
		PPGetFilePath(PPPATH_DD, "RAR-AlcoholCategory.txt", file_name);
		THROW_SL(fileExists(file_name));
		{
			SFile f_in(file_name, SFile::mRead);
			SString line_buf, code_buf, text_buf;
			while(f_in.ReadLine(line_buf)) {
				if(line_buf.Chomp().Divide('\t', code_buf, text_buf) > 0) {
					THROW_SL(CategoryNameList.Add(code_buf, text_buf, 1));
				}
			}
		}
	}
	if(Cfg.E.Flags & Config::fDetectAlcByClass && Cfg.E.AlcGoodsClsID) { // @v9.0.10
		; // Принадлежность товара алкоголю идентифицируем по классу, потому инициализироват
		// список алкогольных и пивных товаров нет смысла
	}
	else {
		if(Cfg.AlcGoodsGrpID) {
			GoodsIterator::GetListByGroup(Cfg.AlcGoodsGrpID, &AlcGoodsList);
			AlcGoodsList.sortAndUndup();
		}
		if(Cfg.BeerGoodsGrpID) {
			GoodsIterator::GetListByGroup(Cfg.BeerGoodsGrpID, &BeerGoodsList);
			BeerGoodsList.sortAndUndup();
		}
	}
	CATCHZOK
	return ok;
}

//static
int SLAPI PrcssrAlcReport::AutoConfigure(long flags)
{
	int    ok = -1;
	SString name_buf;
	SString temp_buf;
	PrcssrAlcReport::Config config;
	int    rcr = ReadConfig(&config);
	THROW(rcr);
	if(!config.E.AlcGoodsClsID) {
		PPGdsClsPacket gc_pack;
		PPObjGoodsClass gc_obj;
		THROW(PPLoadString("alcohol", temp_buf));
		STRNSCPY(gc_pack.Rec.Name, temp_buf);
		STRNSCPY(gc_pack.Rec.Symb, "ALCOHOL");
		gc_pack.Rec.Flags = PPGdsCls::fDupCombine|PPGdsCls::fStdEditDlg;
		// dim-x Крепость %об
		// dim-y Емкость Л
		PPLoadString("meas_alcostrength", name_buf);
		PPLoadString("munit_pctvol", temp_buf);
		name_buf.CatDiv(',', 2).Cat(temp_buf);
		STRNSCPY(gc_pack.DimX.Name, name_buf);
		gc_pack.DimX.Scale = 1;
		gc_pack.Rec.Flags |= PPGdsCls::fUseDimX;
		//
		PPLoadString("meas_volume", name_buf);
		PPLoadString("munit_l", temp_buf);
		name_buf.CatDiv(',', 2).Cat(temp_buf);
		STRNSCPY(gc_pack.DimY.Name, name_buf);
		gc_pack.DimY.Scale = 3;
		gc_pack.Rec.Flags |= PPGdsCls::fUseDimY;
		//
		PPLoadString("goodsalcocode", name_buf);
		STRNSCPY(gc_pack.DimZ.Name, name_buf);
		gc_pack.DimZ.Scale = 0;
		gc_pack.Rec.Flags |= PPGdsCls::fUseDimZ;
		{
			PPID   ex_gc_id = 0;
			PPTransaction tra(1);
			THROW(tra);
			if(gc_obj.SearchBySymb(gc_pack.Rec.Symb, &ex_gc_id, 0) > 0) {
				config.E.AlcGoodsClsID = ex_gc_id;
			}
			else {
				THROW(gc_obj.PutPacket(&config.E.AlcGoodsClsID, &gc_pack, 0));
				config.E.ProofClsDim = PPGdsCls::eX;
				config.VolumeClsDim = PPGdsCls::eY;
				config.CategoryClsDim = PPGdsCls::eZ;
			}
			config.E.Flags |= (config.fDetectAlcByClass|config.fEgaisVer2Fmt);
			THROW(WriteConfig(&config, 0));
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

// static
int FASTCALL PrcssrAlcReport::ReadConfig(PrcssrAlcReport::Config * pCfg)
{
	int    ok = -1;
	SBuffer buffer;
	pCfg->Clear();
	SSerializeContext sctx;
	if(PPRef->GetPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALCREPORTCFG, buffer) > 0) {
		if(pCfg->Serialize(-1, buffer, &sctx))
			ok = 1;
		else {
			pCfg->Clear();
			ok = 0;
		}
	}
	return ok;
}

// static
int FASTCALL PrcssrAlcReport::WriteConfig(PrcssrAlcReport::Config * pCfg, int use_ta)
{
	int    ok = 1;
	SBuffer buffer;
	SSerializeContext sctx;
	THROW(pCfg->Serialize(1, buffer, &sctx));
	THROW(PPRef->PutPropSBuffer(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_ALCREPORTCFG, buffer, use_ta));
	CATCHZOK
	return ok;
}

int SLAPI PrcssrAlcReport::SetConfig(const PrcssrAlcReport::Config * pCfg)
{
	int    ok = 1;
	if(!RVALUEPTR(Cfg, pCfg)) {
		ok = ReadConfig(&Cfg);
	}
	return ok;
}

int SLAPI PrcssrAlcReport::ValidateConfig(const Config & rCfg, long flags)
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket gc_pack;
	THROW_PP(rCfg.AlcGoodsGrpID && GObj.Fetch(rCfg.AlcGoodsGrpID, &goods_rec) > 0, PPERR_ALCRCFG_INVALCGOODSGRP);
	THROW_PP(rCfg.E.AlcGoodsClsID && gc_obj.Fetch(rCfg.E.AlcGoodsClsID, &gc_pack) > 0, PPERR_ALCRCFG_INVALCGOODSCLS);
    THROW_PP(rCfg.CategoryClsDim || rCfg.CategoryTagID, PPERR_ALCRCFG_INVCATTAG);
    THROW_PP(oneof4(rCfg.VolumeClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW), PPERR_ALCRCFG_INVVOLDIM);
    THROW_PP(oneof4(rCfg.E.ProofClsDim, PPGdsCls::eX, PPGdsCls::eY, PPGdsCls::eZ, PPGdsCls::eW), PPERR_ALCRCFG_INVPROOFDIM);
	THROW_PP(!rCfg.E.ProofClsDim || rCfg.E.ProofClsDim != rCfg.VolumeClsDim, PPERR_ALCRCFG_EQCLSDIM);
	THROW_PP(!rCfg.CategoryClsDim || (rCfg.CategoryClsDim != rCfg.VolumeClsDim && rCfg.CategoryClsDim != rCfg.E.ProofClsDim), PPERR_ALCRCFG_EQCLSDIM);
	CATCHZOK
	return ok;
}

class AlcReportConfigDialog : public TDialog {
public:
	AlcReportConfigDialog() : TDialog(DLG_ALCREPCFG)
	{
	}
	int    setDTS(const PrcssrAlcReport::Config * pData)
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		PPIDArray op_list;
		{
			op_list.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_RCP, Data.RcptOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSRETURN, PPOPT_GOODSEXPEND, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_RCR, Data.SupplRetOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSRECEIPT, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_RCE, Data.RcptEtcOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSEXPEND, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_EXP, Data.ExpndOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSEXPEND, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_EXE, Data.ExpndEtcOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSRETURN, PPOPT_GOODSRECEIPT, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_EXR, Data.SaleRetOpID, 0, &op_list, 0);
		}
		{
			op_list.Z().addzlist(PPOPT_GOODSEXPEND, PPOPT_GENERIC, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_INT, Data.IntrExpndOpID, 0, &op_list, 0);
		}
		// @v9.3.12 {
		{
			op_list.Z().addzlist(PPOPT_INVENTORY, 0);
			SetupOprKindCombo(this, CTLSEL_ALCREPCFG_OP_INV, Data.E.EgaisInvOpID, 0, &op_list, 0);
		}
		// } @v9.3.12
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_ALCGRP,  PPOBJ_GOODSGROUP, Data.AlcGoodsGrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN, 0);
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_BEERGRP, PPOBJ_GOODSGROUP, Data.BeerGoodsGrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN, 0);
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_ALCCLS,  PPOBJ_GOODSCLASS, Data.E.AlcGoodsClsID, 0, 0); // @v8.8.0
		{
			ObjTagFilt ot_filt;
			ot_filt.ObjTypeID = PPOBJ_LOT; // @v8.5.6 @fix PPOBJ_GOODS-->PPOBJ_LOT
			SetupObjTagCombo(this, CTLSEL_ALCREPCFG_CATTAG, Data.CategoryTagID, 0, &ot_filt);
		}
		// @v8.9.0 {
		{
			ObjTagFilt ot_filt;
			ot_filt.ObjTypeID = PPOBJ_PERSON;
			SetupObjTagCombo(this, CTLSEL_ALCREPCFG_MITAG, Data.ManufImpTagID, 0, &ot_filt);
		}
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_IMPPSNK, PPOBJ_PRSNKIND, Data.E.ImporterPersonKindID, 0);
		// } @v8.9.0
		if(Data.LotManufTagList.getCount() > 1) {
			disableCtrl(CTLSEL_ALCREPCFG_MNFTAG, 1);
		}
		else {
			disableCtrl(CTLSEL_ALCREPCFG_MNFTAG, 0);
			ObjTagFilt ot_filt;
			ot_filt.ObjTypeID = PPOBJ_LOT;
			SetupObjTagCombo(this, CTLSEL_ALCREPCFG_MNFTAG, Data.LotManufTagList.getSingle(), 0, &ot_filt);
		}
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_LICREG, PPOBJ_REGISTERTYPE, Data.AlcLicRegTypeID, 0, 0);
		SetupPPObjCombo(this, CTLSEL_ALCREPCFG_KPPREG, PPOBJ_REGISTERTYPE, Data.KppRegTypeID, 0, 0);

		SetupStringCombo(this, CTLSEL_ALCREPCFG_CATDIM, PPTXT_GCDIMLIST, Data.CategoryClsDim);
		SetupStringCombo(this, CTLSEL_ALCREPCFG_VOLDIM, PPTXT_GCDIMLIST, Data.VolumeClsDim);
		SetupStringCombo(this, CTLSEL_ALCREPCFG_PRFDIM, PPTXT_GCDIMLIST, Data.E.ProofClsDim); // @v8.8.0
		// @v9.0.10 {
		AddClusterAssoc(CTL_ALCREPCFG_FLAGS, 0, PrcssrAlcReport::Config::fDetectAlcByClass);
		AddClusterAssoc(CTL_ALCREPCFG_FLAGS, 1, PrcssrAlcReport::Config::fWhToReg2ByLacks); // @v9.3.8
		AddClusterAssoc(CTL_ALCREPCFG_FLAGS, 2, PrcssrAlcReport::Config::fEgaisVer2Fmt); // @v9.6.12
		AddClusterAssoc(CTL_ALCREPCFG_FLAGS, 3, PrcssrAlcReport::Config::fEgaisVer3Fmt); // @v9.9.5
		SetClusterData(CTL_ALCREPCFG_FLAGS, Data.E.Flags);
		// } @v9.0.10
		// @v9.3.10 {
		AddClusterAssocDef(CTL_ALCREPCFG_WOSW,  0, PrcssrAlcReport::Config::woswNone);
		AddClusterAssoc(CTL_ALCREPCFG_WOSW,  1, PrcssrAlcReport::Config::woswBalanceWithLots);
		AddClusterAssoc(CTL_ALCREPCFG_WOSW,  2, PrcssrAlcReport::Config::woswByCChecks);
		AddClusterAssoc(CTL_ALCREPCFG_WOSW,  3, PrcssrAlcReport::Config::woswByBills);
		SetClusterData(CTL_ALCREPCFG_WOSW, Data.E.WrOffShopWay);
		// } @v9.3.10
		enableCommand(cmCCheckFilt, Data.E.WrOffShopWay == PrcssrAlcReport::Config::woswByCChecks); // @v9.4.0
		return ok;
	}
	int    getDTS(PrcssrAlcReport::Config * pData)
	{
		int    ok = 1;
		getCtrlData(CTLSEL_ALCREPCFG_OP_RCP, &Data.RcptOpID);
		getCtrlData(CTLSEL_ALCREPCFG_OP_RCR, &Data.SupplRetOpID);
		getCtrlData(CTLSEL_ALCREPCFG_OP_RCE, &Data.RcptEtcOpID);

		getCtrlData(CTLSEL_ALCREPCFG_OP_EXP, &Data.ExpndOpID);
		getCtrlData(CTLSEL_ALCREPCFG_OP_EXE, &Data.ExpndEtcOpID);
		getCtrlData(CTLSEL_ALCREPCFG_OP_EXR, &Data.SaleRetOpID);

		getCtrlData(CTLSEL_ALCREPCFG_OP_INT, &Data.IntrExpndOpID);
		getCtrlData(CTLSEL_ALCREPCFG_OP_INV, &Data.E.EgaisInvOpID); // @v9.3.12

		getCtrlData(CTLSEL_ALCREPCFG_ALCGRP, &Data.AlcGoodsGrpID);
		getCtrlData(CTLSEL_ALCREPCFG_BEERGRP, &Data.BeerGoodsGrpID);
		getCtrlData(CTLSEL_ALCREPCFG_ALCCLS, &Data.E.AlcGoodsClsID); // @v8.8.0

		getCtrlData(CTLSEL_ALCREPCFG_CATTAG, &Data.CategoryTagID);
		getCtrlData(CTLSEL_ALCREPCFG_MITAG, &Data.ManufImpTagID); // @v8.9.0
		getCtrlData(CTLSEL_ALCREPCFG_IMPPSNK, &Data.E.ImporterPersonKindID); // @v8.9.0
		if(Data.LotManufTagList.getCount() <= 1) {
			Data.LotManufTagList.Z().addnz(getCtrlLong(CTLSEL_ALCREPCFG_MNFTAG));
		}
		getCtrlData(CTLSEL_ALCREPCFG_LICREG, &Data.AlcLicRegTypeID);
		getCtrlData(CTLSEL_ALCREPCFG_KPPREG, &Data.KppRegTypeID);

		Data.CategoryClsDim = (int16)getCtrlLong(CTLSEL_ALCREPCFG_CATDIM);
		Data.VolumeClsDim = (int16)getCtrlLong(CTLSEL_ALCREPCFG_VOLDIM);
		Data.E.ProofClsDim = (int16)getCtrlLong(CTLSEL_ALCREPCFG_PRFDIM); // @v8.8.0

		GetClusterData(CTL_ALCREPCFG_FLAGS, &Data.E.Flags); // @v9.0.10
		GetClusterData(CTL_ALCREPCFG_WOSW, &Data.E.WrOffShopWay); // @v9.3.10

		ASSIGN_PTR(pData, Data);
		return ok;
	}
public:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_ALCREPCFG_WOSW)) {
			GetClusterData(CTL_ALCREPCFG_WOSW, &Data.E.WrOffShopWay);
			enableCommand(cmCCheckFilt, Data.E.WrOffShopWay == PrcssrAlcReport::Config::woswByCChecks);
		}
		else if(event.isCmd(cmCCheckFilt)) {
            CCheckFilt cc_filt;
			RVALUEPTR(cc_filt, Data.P_CcFilt);
            PPViewCCheck cc_view;
            if(cc_view.EditBaseFilt(&cc_filt) > 0) {
				if(cc_filt.IsEmpty()) {
					ZDELETE(Data.P_CcFilt);
				}
				else {
					SETIFZ(Data.P_CcFilt, new CCheckFilt);
					ASSIGN_PTR(Data.P_CcFilt, cc_filt);
				}
            }
		}
		else
			return;
		clearEvent(event);
	}
	PrcssrAlcReport::Config Data;
};

// static
int SLAPI PrcssrAlcReport::EditConfig()
{
	int    ok = -1;
	AlcReportConfigDialog * dlg = 0;
	Config cfg;
	THROW(CheckCfgRights(PPCFGOBJ_ALCREPORT, PPR_READ, 0));
	THROW(PrcssrAlcReport::ReadConfig(&cfg));
	THROW(CheckDialogPtr(&(dlg = new AlcReportConfigDialog)));
	dlg->setDTS(&cfg);
	while(ok < 0 && ExecView(dlg) == cmOK) {
		THROW(CheckCfgRights(PPCFGOBJ_ALCREPORT, PPR_MOD, 0));
		if(dlg->getDTS(&cfg) > 0 && PrcssrAlcReport::WriteConfig(&cfg, 1)) {
			ok = 1;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PrcssrAlcReport::GetCategoryNameByCodePos(uint codePos, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	if(codePos > 0 && codePos <= CategoryNameList.getCount()) {
		StrStrAssocArray::Item item = CategoryNameList.at(codePos-1);
		rBuf = item.Val;
		ok = 1;
	}
	return ok;
}

int SLAPI PrcssrAlcReport::GetCategoryNameByCode(const char * pCode, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	SString code = pCode;
	if(code.NotEmptyS()) {
		long  ncode = code.ToLong();
		if(ncode > 0 && ncode < 100) {
			code.Z().CatLongZ(ncode, 3);
		}
		uint   cnp = 0;
		if(CategoryNameList.Search(code, &rBuf, &cnp))
			ok = 1;
	}
	return ok;
}

int SLAPI PrcssrAlcReport::SearchPersonByRarCode(const char * pCode, PPID * pPsnID, PPID * pLocID)
{
    int    ok = -1;
	int    _loc_ok = -1;
	int    _psn_ok = -1;
    PPID   psn_id = 0;
    PPID   loc_id = 0;
    if(!isempty(pCode)) {
		Reference * p_ref = PPRef;
		PPIDArray psn_id_list;
		PPIDArray loc_id_list;
		p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_PERSON, PPTAG_PERSON_FSRARID, pCode, &psn_id_list);
		p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_LOCATION, PPTAG_LOC_FSRARID, pCode, &loc_id_list);
		if(loc_id_list.getCount()) {
            for(uint i = 0; i < loc_id_list.getCount(); i++) {
                const PPID _id = loc_id_list.get(i);
                LocationTbl::Rec loc_rec;
                PersonTbl::Rec psn_rec;
                if(PsnObj.LocObj.Fetch(_id, &loc_rec) > 0) {
                	if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &psn_rec) > 0) {
						if(_loc_ok < 0) {
							psn_id = psn_rec.ID;
							loc_id = _id;
							_loc_ok = 1;
						}
						else if(_loc_ok > 0) {
							_loc_ok = 2; // Есть неоднозначность - найдено более одной локации с одним и тем же кодом
							break;
						}
                	}
				}
            }
		}
		if(_loc_ok < 0) {
			for(uint i = 0; i < psn_id_list.getCount(); i++) {
                const PPID _id = psn_id_list.get(i);
                PersonTbl::Rec psn_rec;
                if(PsnObj.Fetch(_id, &psn_rec) > 0) {
					if(_psn_ok < 0) {
						psn_id = psn_rec.ID;
						_psn_ok = 1;
					}
					else if(_psn_ok > 0) {
						_psn_ok = 2; // Есть неоднозначность - найдено более одной персоналии с одним и тем же кодом
						break;
					}
                }
			}
		}
		else if(psn_id_list.getCount()) {
			for(uint i = 0; i < psn_id_list.getCount(); i++) {
                const PPID _id = psn_id_list.get(i);
                PersonTbl::Rec psn_rec;
                if(PsnObj.Fetch(_id, &psn_rec) > 0) {
					_psn_ok = 3; // Неоднозначность: по коду найден и подходящая локация и персоналия.
						// Предпочтение отдаем локации.
					break;
                }
			}
		}
    }
    ASSIGN_PTR(pPsnID, psn_id);
    ASSIGN_PTR(pLocID, loc_id);
    ok = MAX(_psn_ok, _loc_ok);
    return ok;
}

int SLAPI PrcssrAlcReport::SearchGoodsByRarCode(const char * pCode, PPID * pGoodsID)
{
	ASSIGN_PTR(pGoodsID, 0);
	int    ok = -1;
	if(!isempty(pCode)) {
		BarcodeTbl::Rec bc_rec;
		Goods2Tbl::Rec goods_rec;
		if(GObj.P_Tbl->SearchByBarcode(pCode, &bc_rec, &goods_rec) > 0) {
			ASSIGN_PTR(pGoodsID, goods_rec.ID);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PrcssrAlcReport::GetEgaisCodeList(PPID goodsID, BarcodeArray & rList)
{
	int    ok = -1;
	rList.clear();
	BarcodeArray bc_list;
	GObj.P_Tbl->ReadBarcodes(goodsID, bc_list);
	if(bc_list.getCount()) {
		for(uint i = 0; i < bc_list.getCount(); i++) {
			const BarcodeTbl::Rec & r_bc_rec = bc_list.at(i);
			if(sstrlen(r_bc_rec.Code) == 19) {
				rList.insert(&r_bc_rec);
				ok = 1;
			}
		}
	}
	return ok;
}

// 01 2 34567 89012345678 9 012345678901234567890123456789012345678901234567
// 22 N 00000 0IXAIM093V1 0 1HE41114002092910TPD01BK4BOQV4QIKXJGU687IYO7TEWZ

static int FASTCALL Base36ToAlcoCode(const SString & rS, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	uint64 result = 0;
	const uint len = rS.Len();
	for(uint i = 0; ok && i < len; i++) {
		const  char c = toupper(rS.C(i));
		uint64 v = 0;
		if(c >= '0' && c <= '9')
			v = c - '0';
		else if(c >= 'A' && c <= 'Z')
			v = c - 'A' + 10;
		else
			ok = 0;
		const uint64 m = ui64pow(36, len-i-1);
		result += v * m;
	}
	if(ok) {
		rBuf.Cat(result);
		if(rBuf.Len() < 19)
			rBuf.PadLeft(19-rBuf.Len(), '0');
	}
	return ok;
}

// static
int FASTCALL PrcssrAlcReport::IsEgaisMark(const char * pMark, SString * pProcessedMark)
{
	int    yes = 0;
	const  size_t len = sstrlen(pMark);
	ASSIGN_PTR(pProcessedMark, 0);
	if(len == 68) {
		yes = 1;
		SString temp_buf;
		for(size_t i = 0; yes && i < len; i++) {
			const char c = pMark[i];
			if(!isdec(c) && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z')) {
				if(pProcessedMark) {
					temp_buf.Z().CatChar(c).Transf(CTRANSF_INNER_TO_OUTER);
					KeyDownCommand kd;
					uint   tc = kd.SetChar((uchar)temp_buf.C(0)) ? kd.GetChar() : 0; // Попытка транслировать латинский символ из локальной раскладки клавиатуры
					if((tc >= 'A' && tc <= 'Z') || (tc >= 'a' && tc <= 'z')) {
                        pProcessedMark->CatChar((char)tc);
					}
					else
						yes = 0;
				}
				else
					yes = 0;
			}
			else if(pProcessedMark)
				pProcessedMark->CatChar(c);
		}
	}
	if(pProcessedMark) {
		if(!yes)
			*pProcessedMark = 0;
		else
			assert(PrcssrAlcReport::IsEgaisMark(*pProcessedMark, 0)); // @recursion: sure this works right!
	}
	return yes;
}

// static
int SLAPI PrcssrAlcReport::ParseEgaisMark(const char * pMark, PrcssrAlcReport::EgaisMarkBlock & rMb)
{
	/*
	Если СтрДлина(Значение) <> 68 Тогда
		Сообщить("Некорректный номер марки")
	Иначе
		ВерсияПО = Лев(Значение, 2);
		если сред(Значение,4,5) = "00000" тогда
			НачальнаяПозицияАП = 9;
			КонецнаяПозицияАП = 2;
			КоличествоСимволовАП = 11;
		Иначе
			НачальнаяпозицияАП = 8;
			КонецнаяПозицияАП = 2;
			КоличествоСимволовАП = 12;
		КонецЕсли;
		строкаАП = Сред(Значение,НачальнаяПозицияАП,КоличествоСимволовАП);
		Результат = ПреобразованиеИЗBase36(строкаАП);
		Результат = СтрЗаменить(Результат, Символ(160), "");
		Алкокод = Формат(Результат, "ЧЦ=19; ЧВН=; ЧГ=0");
		Пока СтрДлина(Алкокод) < 19 Цикл
			Алкокод = "0" + Алкокод;
		КонецЦикла;


	Функция ПреобразованиеИЗBase36(строкаРазбора)
		Результат=0;
		н = СтрДлина(строкаРазбора);
		Для Х=1 По Н Цикл
			М=1;
			Для У=1 По Н-Х Цикл
				М=М*36
			КонецЦикла;
			Результат=Результат+(Найти("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",Сред(строкаРазбора,Х,1))-1)*М;
		КонецЦикла;
		Возврат Результат;
	КонецФункции
	*/
	rMb.Ver = 0;
	rMb.EgaisCode.Z();

	int    ok = 0;
	EgaisMarkBlock mb;
	SString temp_buf;
	SString mark = pMark;
	mark.Strip();
	THROW(PrcssrAlcReport::IsEgaisMark(mark, 0));
    mark.Sub(0, 2, temp_buf);
	rMb.Ver = (int16)temp_buf.ToLong();
	{
		mark.Sub(3, 5, temp_buf);
		size_t ap_start = 0;
		size_t ap_len = 0;
		if(temp_buf == "00000") {
			ap_start = 8;
			ap_len = 11;
		}
		else {
			ap_start = 7;
			ap_len = 12;
		}
		mark.Sub(ap_start, ap_len, temp_buf);
		Base36ToAlcoCode(temp_buf, rMb.EgaisCode);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrAlcReport::PreprocessGoodsItem(PPID goodsID, PPID lotID, const ObjTagList * pTags, long flags, PrcssrAlcReport::GoodsItem & rItem)
{
	rItem.Clear();

	int    ok = 1;
	Reference * p_ref = PPRef;
	SString fmt_buf, msg_buf, temp_buf;
	GoodsExtTbl::Rec goods_ext_rec;
	Goods2Tbl::Rec goods_rec;
	ObjTagItem tag_item;
	const char * p_egais_code = 0; // Указатель на наиболее надежную версию кода товара ЕГАИС
		// Может ссылаться либо на rItem.EgaisCode, либо на rItem.RefcEgaisCode.
	goods_ext_rec.GoodsID = 0;
	rItem.LotID = lotID;
	if(GObj.Fetch(goodsID, &goods_rec) > 0) {
		rItem.GoodsID = goods_rec.ID;
		if(Cfg.CategoryTagID) {
			if(pTags && pTags->GetItemStr(Cfg.CategoryTagID, temp_buf) > 0) {
				rItem.CategoryCode = temp_buf;
				rItem.StatusFlags |= GoodsItem::stCategoryCodeByLotTag;
			}
			else if(lotID && p_ref->Ot.GetTagStr(PPOBJ_LOT, lotID, Cfg.CategoryTagID, temp_buf) > 0) {
				rItem.CategoryCode = temp_buf;
				rItem.StatusFlags |= GoodsItem::stCategoryCodeByLotTag;
			}
		}
		if(goods_rec.GdsClsID) {
			PPGdsClsPacket gc_pack;
			rItem.StatusFlags |= GoodsItem::stClass;
			if(rItem.CategoryCode.Empty()) {
				if(Cfg.CategoryClsDim && (goods_ext_rec.GoodsID || GObj.P_Tbl->GetExt(goodsID, &goods_ext_rec) > 0)) {
					if(gc_pack.Rec.ID || GcObj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0) {
						double v = 0.0;
						if(gc_pack.GetExtDim(&goods_ext_rec, Cfg.CategoryClsDim, &v) > 0 && v > 0.0) {
							rItem.CategoryCode.Z().Cat((long)v);
							rItem.StatusFlags |= GoodsItem::stCategoryCodeByClsDim;
						}
					}
				}
			}
			if(Cfg.VolumeClsDim && (goods_ext_rec.GoodsID || GObj.P_Tbl->GetExt(goodsID, &goods_ext_rec) > 0)) {
				if(gc_pack.Rec.ID || GcObj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0) {
					double v = 0.0;
					if(gc_pack.GetExtDim(&goods_ext_rec, Cfg.VolumeClsDim, &v) > 0)
						rItem.Volume = v;
				}
			}
			if(Cfg.E.ProofClsDim && (goods_ext_rec.GoodsID || GObj.P_Tbl->GetExt(goodsID, &goods_ext_rec) > 0)) {
				if(gc_pack.Rec.ID || GcObj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0) {
					double v = 0.0;
					if(gc_pack.GetExtDim(&goods_ext_rec, Cfg.E.ProofClsDim, &v) > 0)
						rItem.Proof = v;
				}
			}
		}
		if(rItem.Volume == 0.0) {
			rItem.Volume = (goods_rec.PhUPerU > 0.0) ? goods_rec.PhUPerU : 1.0;
		}
		// @v8.9.0 {
		if(goods_rec.UnitID) {
			if(goods_rec.UnitID == PPUNT_LITER)
				rItem.UnpackedVolume = 1.0;
			else {
				PPObjUnit u_obj;
				PPUnit u_rec;
				if(u_obj.Fetch(goods_rec.UnitID, &u_rec) > 0 && u_rec.BaseUnitID == PPUNT_LITER)
					rItem.UnpackedVolume = u_rec.BaseRatio;
			}
		}
		// } @v8.9.0
		{
			GoodsStockExt gse;
			if(GObj.P_Tbl->GetStockExt(goodsID, &gse, 1) > 0) {
				rItem.Brutto = gse.CalcBrutto(1.0);
			}
		}
		if(rItem.CategoryCode.NotEmptyS()) {
			if(flags & pgifUseSubstCode && Cfg.SubstCategoryCode[0]) {
				rItem.CategoryCode = Cfg.SubstCategoryCode;
			}
			long  ncode = rItem.CategoryCode.ToLong();
			if(ncode > 0 && ncode < 100) {
				temp_buf.Z().CatLongZ(ncode, 3);
				rItem.CategoryCode = temp_buf;
			}
			if(ncode > 0 && ncode < 500 && !oneof3(ncode, /*260,*/ 261, 262, 263)) {
				rItem.StatusFlags |= GoodsItem::stMarkWanted;
			}
			uint   cnp = 0;
			if(CategoryNameList.Search(rItem.CategoryCode, &rItem.CategoryName, &cnp)) {
				rItem.StatusFlags |= GoodsItem::stCategoryName;
				rItem.CategoryCodePos = cnp+1;
			}
		}
		//
		// PPTXT_ALCREP_CATNDEF        "Для товара %s не определен код алкогольной продукции"
		// PPTXT_ALCREP_CATNAMNDEF     "Не удалось идентифицировать наименование вида алкогольной продукции: %s"
		// PPTXT_ALCREP_GOODSCLSNDEF   "Не определен класс у товара %s"
		//
		if(!(rItem.StatusFlags & GoodsItem::stClass)) {
            temp_buf.Z().CatChar('[').CatEq("id", goods_rec.ID).Space().CatEq("name", goods_rec.Name).CatChar(']');
            msg_buf.Z().Printf(PPLoadTextS(PPTXT_ALCREP_GOODSCLSNDEF, fmt_buf), temp_buf.cptr());
            if(rItem.MsgPool.NotEmptyS())
				rItem.MsgPool.Tab();
			rItem.MsgPool.Cat(msg_buf);
		}
		if(!(rItem.StatusFlags & (GoodsItem::stCategoryCodeByLotTag|GoodsItem::stCategoryCodeByClsDim))) {
            temp_buf.Z().CatChar('[').CatEq("id", goods_rec.ID).Space().CatEq("name", goods_rec.Name).CatChar(']');
            msg_buf.Z().Printf(PPLoadTextS(PPTXT_ALCREP_CATNDEF, fmt_buf), temp_buf.cptr());
            if(rItem.MsgPool.NotEmptyS())
				rItem.MsgPool.Tab();
			rItem.MsgPool.Cat(msg_buf);
			ok = 0;
		}
		if(!(rItem.StatusFlags & GoodsItem::stCategoryName)) {
            temp_buf.Z().CatChar('[').CatEq("id", goods_rec.ID).Space().CatEq("name", goods_rec.Name).Space().CatEq("code", rItem.CategoryCode).CatChar(']');
			msg_buf.Z().Printf(PPLoadTextS(PPTXT_ALCREP_CATNAMNDEF, fmt_buf), temp_buf.cptr());
            if(rItem.MsgPool.NotEmptyS())
				rItem.MsgPool.Tab();
			rItem.MsgPool.Cat(msg_buf);
			ok = 0;
		}
		// @v8.8.0 {
		if(pTags && pTags->GetItemStr(PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0) {
			rItem.EgaisCode = temp_buf;
			rItem.StatusFlags |= rItem.stEgaisCodeByLotTag;
			p_egais_code = rItem.EgaisCode;
		}
		else if(lotID && p_ref->Ot.GetTagStr(PPOBJ_LOT, lotID, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf) > 0) {
			rItem.EgaisCode = temp_buf;
			rItem.StatusFlags |= rItem.stEgaisCodeByLotTag;
			p_egais_code = rItem.EgaisCode;
		}
		else {
			BarcodeArray bc_list;
			if(GetEgaisCodeList(goodsID, bc_list) > 0) {
				assert(bc_list.getCount());
				rItem.EgaisCode = bc_list.at(0).Code;
				rItem.StatusFlags |= rItem.stEgaisCodeByGoods;
				p_egais_code = rItem.EgaisCode;
			}
		}
		// } @v8.8.0
		if(ACfg.Hdr.Flags & ACfg.Hdr.fUseOwnEgaisObjects || (flags & pgifForceUsingInnerDb)) {
            SETIFZ(P_RefC, new RefCollection);
            if(P_RefC) {
				int   _is_ref_a = 0;
				if(pTags && pTags->GetItemStr(PPTAG_LOT_FSRARINFA, temp_buf) > 0) {
					_is_ref_a = 1;
				}
				else if(lotID && p_ref->Ot.GetTagStr(PPOBJ_LOT, lotID, PPTAG_LOT_FSRARINFA, temp_buf) > 0) {
					_is_ref_a = 1;
				}
				if(_is_ref_a) {
					rItem.InformA = temp_buf;
					TSVector <EgaisRefATbl::Rec> refa_list; // @v9.8.4 TSArray-->TSVector
					const int actual_pos = P_RefC->RaC.SearchByCode(rItem.InformA, refa_list);
					if(actual_pos > 0) {
						const EgaisRefATbl::Rec & r_item = refa_list.at(actual_pos-1);
						if(r_item.Flags & EgaisRefACore::fVerified) {
							rItem.RefcInfA_ActualDate = r_item.ActualDate;
							rItem.RefcInfAID = r_item.ID;
							rItem.RefcEgaisCode = r_item.AlcCode;
							rItem.RefcImporterCode = r_item.ImporterRarIdent;
							rItem.RefcManufCode = r_item.ManufRarIdent;
							rItem.RefcVolume = ((double)r_item.Volume) / 100000.0;
							rItem.BottlingDate = r_item.BottlingDate;
							rItem.CountryCode = r_item.CountryCode;
							rItem.StatusFlags |= GoodsItem::stRefcUsedByRefA;
						}
					}
				}
				{
					if(isempty(p_egais_code) || rItem.StatusFlags & rItem.stEgaisCodeByGoods)
						p_egais_code = rItem.RefcEgaisCode;
                    if(!isempty(p_egais_code)) {
						TSVector <EgaisProductTbl::Rec> pr_list; // @v9.8.4 TSArray-->TSVector
                        const int actual_pos = P_RefC->PrC.SearchByCode(p_egais_code, pr_list);
                        if(actual_pos > 0) {
							EgaisProductCore::Item pr_item;
							P_RefC->PrC.RecToItem(pr_list.at(actual_pos-1), pr_item);
							int    what_is_more_actual = 0; // 1 - infa, 2 - product
							rItem.RefcPr_ActualDate = pr_item.ActualDate;
							if(rItem.RefcPr_ActualDate > rItem.RefcInfA_ActualDate)
								what_is_more_actual = 2;
							else if(rItem.RefcInfA_ActualDate > rItem.RefcPr_ActualDate)
								what_is_more_actual = 1;
							rItem.RefcProductID = pr_item.ID;
							assert(strcmp(pr_item.AlcoCode, p_egais_code) == 0);
							if(pr_item.ManufRarIdent[0]) {
								if(rItem.RefcManufCode.Empty())
									rItem.RefcManufCode = pr_item.ManufRarIdent;
								else if(rItem.RefcManufCode != pr_item.ManufRarIdent) {
									rItem.StatusFlags |= rItem.stRefcInfAPrManufConfl;
									if(what_is_more_actual == 2)
										rItem.RefcManufCode = pr_item.ManufRarIdent;
								}
							}
							if(pr_item.ImporterRarIdent[0]) {
								if(rItem.RefcImporterCode.Empty())
									rItem.RefcImporterCode = pr_item.ImporterRarIdent;
								else if(rItem.RefcImporterCode != pr_item.ImporterRarIdent) {
									rItem.StatusFlags |= rItem.stRefcInfAPrImprtConfl;
									if(what_is_more_actual == 2)
										rItem.RefcImporterCode = pr_item.ImporterRarIdent;
								}
							}
							rItem.RefcProof = pr_item.Proof;
							rItem.RefcVolume = pr_item.Volume;
							rItem.RefcCategoryCode = pr_item.CategoryCode;
							if(rItem.RefcCategoryCode.NotEmpty() && rItem.RefcCategoryCode != rItem.CategoryCode)
								rItem.StatusFlags |= rItem.stRefcPrDbCategConfl;
                        }
                    }
				}
            }
		}
	}
	else {
		PPGetLastErrorMessage(0, msg_buf);
		if(rItem.MsgPool.NotEmptyS())
			rItem.MsgPool.Tab();
		rItem.MsgPool.Cat(msg_buf);
		ok = 0;
	}
	return ok;
}

int FASTCALL PrcssrAlcReport::IsStorageLoc(PPID locID) const
{
	return Cfg.StorageLocList.bsearch(locID);
}

int FASTCALL PrcssrAlcReport::IsStorageBillLoc(PPID billID)
{
	int    yes = 0;
	BillTbl::Rec bill_rec;
    if(P_BObj->Fetch(billID, &bill_rec) > 0) {
    	yes = Cfg.StorageLocList.bsearch(bill_rec.LocID);
    }
    return yes;
}

int FASTCALL PrcssrAlcReport::GetAlcGoodsList(PPIDArray & rList)
{
	rList.clear();
	int    result = 0;
	if(Cfg.E.Flags & Config::fDetectAlcByClass && Cfg.E.AlcGoodsClsID) {
		GoodsFilt filt;
		filt.Ep.GdsClsID = Cfg.E.AlcGoodsClsID;
        GoodsIterator::GetListByFilt(&filt, &rList, 0);
		result = 0x04;
	}
	else {
        rList = AlcGoodsList;
        rList.add(&BeerGoodsList);
        result = 0x01 | 0x02;
	}
	rList.sortAndUndup();
	if(rList.getCount() == 0)
		result = 0;
	return result;
}

int FASTCALL PrcssrAlcReport::IsAlcGoods(PPID goodsID)
{
	int    result = 0;
	const  long goods_id = labs(goodsID);
	if(Cfg.E.Flags & Config::fDetectAlcByClass && Cfg.E.AlcGoodsClsID) {
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(goods_id, &goods_rec) > 0 && goods_rec.GdsClsID == Cfg.E.AlcGoodsClsID) {
			result = 4;
		}
	}
	else {
		if(AlcGoodsList.bsearch(goods_id))
			result |= 0x01;
		if(BeerGoodsList.bsearch(goods_id))
			result |= 0x02;
	}
	return result;
}

PPID FASTCALL PrcssrAlcReport::GetWkrRegisterTypeID(int wkr) const
{
	switch(wkr) {
		case wkrAlcLic: return Cfg.AlcLicRegTypeID;
		case wkrKPP: return NZOR(Cfg.KppRegTypeID, PPREGT_KPP);
		case wkrTransportLic: return Cfg.E.TransportLicRegTypeID;
	}
	return 0;
}

int SLAPI PrcssrAlcReport::GetWkrRegisterListByPeriod(int wkr, PPID psnID, PPID locID, const DateRange & rPeriod, RegisterArray * pList)
{
	int    ok = -1;
	const  PPID reg_type_id = GetWkrRegisterTypeID(wkr);
	PPID   psn_id = psnID;
	SString temp_buf;
	RegisterTbl::Rec reg_rec;
	MEMSZERO(reg_rec);
	RegisterArray full_reg_list;
	RegisterArray _reg_list;
	LocationTbl::Rec loc_rec;
	if(reg_type_id) {
		int    by_loc = 0;
		if(locID && PsnObj.LocObj.Fetch(locID, &loc_rec) > 0) {
			PsnObj.RegObj.P_Tbl->GetByLocation(locID, &full_reg_list);
			if(full_reg_list.GetRegister(reg_type_id, 0, 0) > 0)
				by_loc = 2;
			else if(wkr == wkrKPP && Cfg.KppDlvrExt && LocationCore::GetExField(&loc_rec, Cfg.KppDlvrExt, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				//
				// До ввода регистров по локациям для ввода КПП локации иногда использовалось специальное
				// дополнительное поле локации.
				//
				MEMSZERO(reg_rec);
				reg_rec.ObjType = PPOBJ_LOCATION;
				reg_rec.ObjID = locID;
				temp_buf.CopyTo(reg_rec.Num, sizeof(reg_rec.Num));
				_reg_list.insert(&reg_rec);
				by_loc = 4;
				ok = 4;
			}
			else if(!psn_id)
				psn_id = loc_rec.OwnerID;
		}
		if(ok <= 0) {
			if(!by_loc && psn_id) {
				// @v8.3.9 PsnObj.RegObj.P_Tbl->GetByPerson(psn_id, &full_reg_list);
				PsnObj.GetRegList(psn_id, &full_reg_list, 1); // @v8.3.9
			}
			full_reg_list.Sort();
			uint   reg_pos = 0;
			if(full_reg_list.GetListByPeriod(reg_type_id, rPeriod, &_reg_list) > 0) {
				ok = by_loc ? 2 : 1;
			}
		}
	}
	{
		for(uint i = 0; i < _reg_list.getCount(); i++) {
			ProcessRegisterRec(&_reg_list.at(i), psnID, locID);
		}
	}
	ASSIGN_PTR(pList, _reg_list);
	return ok;
}

int SLAPI PrcssrAlcReport::GetWkrRegisterList(int wkr, PPID psnID, PPID locID, LDATE dt, RegisterArray * pList)
{
	int    ok = -1;
	const  PPID reg_type_id = GetWkrRegisterTypeID(wkr);
	PPID   psn_id = psnID;
	SString temp_buf;
	RegisterTbl::Rec reg_rec;
	MEMSZERO(reg_rec);
	RegisterArray full_reg_list;
	RegisterArray _reg_list;
	LocationTbl::Rec loc_rec;
	if(reg_type_id) {
		int    by_loc = 0;
		if(locID && PsnObj.LocObj.Fetch(locID, &loc_rec) > 0) {
			PsnObj.RegObj.P_Tbl->GetByLocation(locID, &full_reg_list);
			if(full_reg_list.GetRegister(reg_type_id, 0, 0) > 0)
				by_loc = 2;
			else if(wkr == wkrKPP && Cfg.KppDlvrExt && LocationCore::GetExField(&loc_rec, Cfg.KppDlvrExt, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				//
				// До ввода регистров по локациям для ввода КПП локации иногда использовалось специальное
				// дополнительное поле локации.
				//
				MEMSZERO(reg_rec);
				reg_rec.ObjType = PPOBJ_LOCATION;
				reg_rec.ObjID = locID;
				temp_buf.CopyTo(reg_rec.Num, sizeof(reg_rec.Num));
				_reg_list.insert(&reg_rec);
				by_loc = 4;
				ok = 4;
			}
			else if(!psn_id)
				psn_id = loc_rec.OwnerID;
		}
		if(ok <= 0) {
			if(!by_loc && psn_id) {
				// @v8.3.9 PsnObj.RegObj.P_Tbl->GetByPerson(psn_id, &full_reg_list);
				PsnObj.GetRegList(psn_id, &full_reg_list, 1); // @v8.3.9
			}
			full_reg_list.Sort();
			uint   reg_pos = 0;
			if(full_reg_list.GetListByType(reg_type_id, dt, &_reg_list) > 0) {
				ok = by_loc ? 2 : 1;
			}
		}
	}
	{
		for(uint i = 0; i < _reg_list.getCount(); i++) {
			ProcessRegisterRec(&_reg_list.at(i), psnID, locID);
		}
	}
	ASSIGN_PTR(pList, _reg_list);
	return ok;
}

int SLAPI PrcssrAlcReport::GetWkrRegister(int wkr, PPID psnID, PPID locID, LDATE dt, RegisterTbl::Rec * pRec)
{
	int    ok = -1;
	const  PPID reg_type_id = GetWkrRegisterTypeID(wkr);
	PPID   psn_id = psnID;
	SString temp_buf;
	RegisterTbl::Rec reg_rec;
	MEMSZERO(reg_rec);
	RegisterArray reg_list;
	LocationTbl::Rec loc_rec;
	if(reg_type_id) {
		int    by_loc = 0;
		if(locID && PsnObj.LocObj.Fetch(locID, &loc_rec) > 0) {
			PsnObj.RegObj.P_Tbl->GetByLocation(locID, &reg_list);
			if(reg_list.GetRegister(reg_type_id, 0, 0) > 0)
				by_loc = 2;
			else if(wkr == wkrKPP && Cfg.KppDlvrExt && LocationCore::GetExField(&loc_rec, Cfg.KppDlvrExt, temp_buf) > 0 && temp_buf.NotEmptyS()) {
				//
				// До ввода регистров по локациям для ввода КПП локации иногда использовалось специальное
				// дополнительное поле локации.
				//
				MEMSZERO(reg_rec);
				reg_rec.ObjType = PPOBJ_LOCATION;
				reg_rec.ObjID = locID;
				temp_buf.CopyTo(reg_rec.Num, sizeof(reg_rec.Num));
				by_loc = 4;
				ok = 4;
			}
			else if(!psn_id)
				psn_id = loc_rec.OwnerID;
		}
		if(ok <= 0) {
			if(!by_loc && psn_id) {
				// @v8.4.9 PsnObj.RegObj.P_Tbl->GetByPerson(psn_id, &reg_list);
				PsnObj.GetRegList(psn_id, &reg_list, 1); // @v8.4.9
			}
			reg_list.Sort();
			uint   reg_pos = 0;
			// @v8.4.4 {
			int    sr = reg_list.SelectRegister(reg_type_id, dt, 0, &reg_rec);
			if(sr > 0) {
				ok = by_loc ? 2 : 1;
			}
			// } @v8.4.4
			/* @v8.4.4
			if(reg_list.GetRegister(reg_type_id, dt, &reg_pos, &reg_rec) > 0) {
				ok = by_loc ? 2 : 1;
			}
			*/
		}
	}
	if(ok > 0) {
		ProcessRegisterRec(&reg_rec, psnID, locID);
	}
	ASSIGN_PTR(pRec, reg_rec);
	return ok;
}

int FASTCALL PrcssrAlcReport::GetManufPersonType(PPID personID)
{
	int    result = -1;
	if(Cfg.ManufImpTagID) {
		ObjTagItem tag;
		if(TagObj.FetchTag(personID, Cfg.ManufImpTagID, &tag) > 0) {
			SString temp_buf;
			tag.GetStr(temp_buf);
			const long val = temp_buf.ToLong();
			if(val == 1)
				result = 1;
			else if(val == 2)
				result = 2;
		}
	}
	else {
		if(Cfg.E.ImporterPersonKindID && PsnObj.P_Tbl->IsBelongToKind(personID, Cfg.E.ImporterPersonKindID))
			result = 2;
		else if(PsnObj.P_Tbl->IsBelongToKind(personID, PPPRK_MANUF))
			result = 1;
	}
	return result;
}

int SLAPI PrcssrAlcReport::GetLotManufID(PPID lotID, PPID * pManufID, SString * pMsgBuf)
{
    int    ok = 0;
    PPID   manuf_id = 0;
    enum {
    	_stOK = 0,
    	_stLotHasntTag = 1,
    	_stHangedTagLink,
    	_stZeroTagValue,
    	_stHangedLot,
    	_stHangedLotBill
    };
    int    status = _stLotHasntTag;
    SString msg_buf, fmt_buf, temp_buf;
    if(lotID) {
		ObjTagItem tag_item;
		for(uint i = 0; !ok && i < Cfg.LotManufTagList.getCount(); i++) {
			const PPID tag_id = Cfg.LotManufTagList.at(i);
			if(tag_id && TagObj.FetchTag(lotID, tag_id, &tag_item) > 0) {
				tag_item.GetInt(&manuf_id);
				PersonTbl::Rec psn_rec;
				if(manuf_id) {
					if(PsnObj.Fetch(manuf_id, &psn_rec) > 0) {
						ok = 1;
						status = _stOK;
					}
					else {
						manuf_id = 0;
						status = _stHangedTagLink;
						// Лот имеет тег, ссылающийся на несуществующую персоналию
					}
				}
				else {
					status = _stZeroTagValue;
				}
			}
		}
		if(!manuf_id) {
			ReceiptTbl::Rec lot_rec;
            if(P_BObj->trfr->Rcpt.Search(lotID, &lot_rec) > 0) {
				ReceiptCore::MakeCodeString(&lot_rec, 0, temp_buf);
				if(status == _stHangedTagLink) {
					//PPTXT_ALCREP_MANUFLOTTAGHLINK "Тег производитель/импортер по лоту '%s' ссылается на не существующую персоналию"
					msg_buf.Printf(PPLoadTextS(PPTXT_ALCREP_MANUFLOTTAGHLINK, fmt_buf), temp_buf.cptr());
				}
				else if(status == _stZeroTagValue) {
					//PPTXT_ALCREP_MANUFLOTTAGZLINK "Тег производитель/импортер по лоту '%s' имеет нулевое или иное не верное значение"
					msg_buf.Printf(PPLoadTextS(PPTXT_ALCREP_MANUFLOTTAGZLINK, fmt_buf), temp_buf.cptr());
				}
				else {
					// PPTXT_ALCREP_MANUFLOTTAGNDEF  "Не определен тег производитель/импортер для лота '%s'"
					msg_buf.Printf(PPLoadTextS(PPTXT_ALCREP_MANUFLOTTAGNDEF, fmt_buf), temp_buf.cptr());
				}
            }
			else {
				status = _stHangedLot;
				//PPTXT_ALCREP_HANGEDLOT        "Обнаружен висячий идентификатор лота '%s'"
				temp_buf.Z().Cat(lotID);
				msg_buf.Printf(PPLoadTextS(PPTXT_ALCREP_HANGEDLOT, fmt_buf), temp_buf.cptr());
			}
		}
	}
	ASSIGN_PTR(pManufID, manuf_id);
	ASSIGN_PTR(pMsgBuf, msg_buf);
    return ok;
}

int SLAPI PrcssrAlcReport::ProcessRegisterRec(RegisterTbl::Rec * pRegRec, PPID psnID, PPID locID)
{
    struct SurRegIdBlock {
    	PPID RegID;
    	PPID PsnID;
    	PPID LocID;
    };
    int    ok = 1;
    SurRegIdBlock sib;
    sib.RegID = pRegRec->ID;
    sib.PsnID = psnID;
    sib.LocID = locID;
	uint32 sur_id = BobJencHash(&sib, sizeof(sib));
    pRegRec->ExtID = labs((long)sur_id); // @note Для банковских счетов поле RegisterTbl::Rec::ExtID значимо
    {
		SString serial, left, right;
		(serial = pRegRec->Serial).Strip();
		if(serial.Divide('#', left, right) > 0) {
            size_t pos = 0;
			if(right.Strip().StrChr(' ', &pos))
				right.Trim(pos).Strip();
			serial = right;
		}
		serial.CopyTo(pRegRec->Serial, sizeof(pRegRec->Serial));
    }
    return ok;
}

int SLAPI PrcssrAlcReport::FetchRegister(PPID regID, PPID psnID, PPID locID, RegisterTbl::Rec * pRegRec)
{
	RegisterTbl::Rec rec;
    int    ok = PsnObj.RegObj.Fetch(regID, &rec);
    if(ok > 0) {
		ProcessRegisterRec(&rec, psnID, locID);
    }
    else
		MEMSZERO(rec);
	ASSIGN_PTR(pRegRec, rec);
	return ok;
}

int SLAPI PrcssrAlcReport::GetBillLic(PPID billID, PPID * pRegID, RegisterTbl::Rec * pRegRec)
{
	int    ret = 0;
	PPID   lic_reg_id = 0;
	RegisterTbl::Rec lic_reg_rec;
	BillTbl::Rec bill_rec;
    if(P_BObj->Fetch(billID, &bill_rec) > 0) {
		PPID   bill_lic_id = 0;
		if(bill_lic_id) {
			ret = licsrcBillExt;
		}
		else {
			PPID   psn_id = ObjectToPerson(bill_rec.Object);
			PPID   loc_id = 0;
			if(psn_id) {
				PPFreight freight;
				if(P_BObj->FetchFreight(billID, &freight) > 0 && freight.DlvrAddrID)
					loc_id = freight.DlvrAddrID;
				int    r = GetWkrRegister(wkrAlcLic, psn_id, loc_id, bill_rec.Dt, &lic_reg_rec);
				if(r == 2)
					ret = licsrcDlvrLoc;
				else if(r == 1)
					ret = licsrcObject;
			}
		}
    }
    ASSIGN_PTR(pRegID, lic_reg_rec.ID);
    ASSIGN_PTR(pRegRec, lic_reg_rec);
    return ret;
}
//
// Implementation of PPALDD_AlcoRepConfig
//
PPALDD_CONSTRUCTOR(AlcoRepConfig)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(AlcoRepConfig) { Destroy(); }

int PPALDD_AlcoRepConfig::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = 1;
	if(H.ID == 0) {
		PrcssrAlcReport::Config cfg;
		PrcssrAlcReport::ReadConfig(&cfg);
		H.ID = 1;
#define CPYFLD(f) H.f = cfg.f
		CPYFLD(RcptOpID);
		CPYFLD(SaleRetOpID);
		CPYFLD(RcptEtcOpID);
		CPYFLD(ExpndOpID);
		CPYFLD(SupplRetOpID);
		CPYFLD(ExpndEtcOpID);
		CPYFLD(IntrExpndOpID);
		CPYFLD(AlcGoodsGrpID);
		CPYFLD(BeerGoodsGrpID);
		CPYFLD(CategoryTagID);
		CPYFLD(AlcLicRegTypeID);
		CPYFLD(KppRegTypeID);
		CPYFLD(WhsExpTagID);
		CPYFLD(ManufImpTagID);
		CPYFLD(CategoryClsDim);
		CPYFLD(VolumeClsDim);
		H.TransportLicRegTypeID = cfg.E.TransportLicRegTypeID;
		H.AlcGoodsClsID = cfg.E.AlcGoodsClsID;
		H.ProofClsDim = cfg.E.ProofClsDim;
#undef CPYFLD
		STRNSCPY(H.SubstCategoryCode, cfg.SubstCategoryCode);
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}
