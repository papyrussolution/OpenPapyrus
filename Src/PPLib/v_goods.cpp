// V_GOODS.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
// Фильтр по товарам
//
int SLAPI GoodsFilt::InitInstance()
{
	P_SjF = 0;
	P_TagF = 0;
	SetFlatChunk(offsetof(GoodsFilt, ReserveStart), offsetof(GoodsFilt, Ep) + offsetof(ClsdGoodsFilt, KindList) - offsetof(GoodsFilt, ReserveStart));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, Ep) + offsetof(ClsdGoodsFilt, KindList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, Ep) + offsetof(ClsdGoodsFilt, GradeList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, Ep) + offsetof(ClsdGoodsFilt, AddObjList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, Ep) + offsetof(ClsdGoodsFilt, AddObj2List));
	SetBranchSString(offsetof(GoodsFilt, SrchStr_));
	SetBranchSString(offsetof(GoodsFilt, BarcodeLen));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, GrpIDList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, ManufList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, LocList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, BrandList));
	SetBranchObjIdListFilt(offsetof(GoodsFilt, BrandOwnerList));
	SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(GoodsFilt, P_SjF));
	SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(GoodsFilt, P_TagF));
	return Init(1, 0);
}

#define GOODSFILT_VERSION -25
	// @v6.0.2 -14-->-19
	// @v6.4.2 -19-->-21
	// @v7.2.0 -21-->-22
	// @v7.7.9 -22-->-23
	// @v8.6.4 -23-->-24
	// @v10.0.12 -24-->-25

IMPLEMENT_PPFILT_FACTORY(Goods); SLAPI GoodsFilt::GoodsFilt(PPID goodsGroupID) : PPBaseFilt(PPFILT_GOODS, 0, GOODSFILT_VERSION)
{
	InitInstance();
	GrpID = goodsGroupID;
}

SLAPI GoodsFilt::GoodsFilt(const GoodsFilt & s) : PPBaseFilt(PPFILT_GOODS, 0, GOODSFILT_VERSION)
{
	InitInstance();
	Copy(&s, 1);
	ResultBrandList = s.ResultBrandList;
}

GoodsFilt & FASTCALL GoodsFilt::operator = (const GoodsFilt & s)
{
	Copy(&s, 0);
	ResultBrandList = s.ResultBrandList;
	return *this;
}

struct ExtParams_Before24 {
	PPID   GdsClsID;
	PPID   KindID;
	PPID   AddObjID;
	PPID   GradeID;
	PPID   AddObj2ID;
	RealRange DimX;
	RealRange DimY;
	RealRange DimZ;
	RealRange DimW;
};

/*virtual*/int SLAPI GoodsFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
{
	int    ok = -1;
	if(ver == -24) {
		class GoodsFilt_v24 : public PPBaseFilt {
		public:
			GoodsFilt_v24() : PPBaseFilt(PPFILT_GOODS, 0, -24), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(GoodsFilt_v24, ReserveStart), offsetof(GoodsFilt_v24, Ep) + offsetof(ClsdGoodsFilt, KindList) - offsetof(GoodsFilt_v24, ReserveStart));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, Ep) + offsetof(ClsdGoodsFilt, KindList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, Ep) + offsetof(ClsdGoodsFilt, GradeList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, Ep) + offsetof(ClsdGoodsFilt, AddObjList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, Ep) + offsetof(ClsdGoodsFilt, AddObj2List));
				SetBranchSString(offsetof(GoodsFilt_v24, SrchStr_));
				SetBranchSString(offsetof(GoodsFilt_v24, BarcodeLen));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, GrpIDList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, ManufList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, LocList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, BrandList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v24, BrandOwnerList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(GoodsFilt_v24, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(GoodsFilt_v24, P_TagF));
				Init(1, 0);
			}
			char   ReserveStart[4];    // @anchor Проецируется на __GoodsFilt::Reserve
			PPID   UhttStoreID;
			PPID   RestrictQuotKindID;
			int32  InitOrder;
			PPID   MtxLocID;
			PPID   BrandOwnerID;
			PPID   CodeArID;
			PPID   GrpID;
			PPID   ManufID;
			PPID   ManufCountryID;
			PPID   UnitID;
			PPID   PhUnitID;
			PPID   SupplID;
			PPID   GoodsTypeID;
			PPID   TaxGrpID;
			PPID   LocID_;
			DateRange LotPeriod;
			long   Flags;
			long   VatRate;
			LDATE  VatDate;
			PPID   BrandID_;
			ClsdGoodsFilt Ep;          // @anchor
			SString SrchStr_;
			SString BarcodeLen;
			ObjIdListFilt GrpIDList;
			ObjIdListFilt ManufList;
			ObjIdListFilt LocList;
			ObjIdListFilt BrandList;
			ObjIdListFilt BrandOwnerList;
			SysJournalFilt * P_SjF;
			TagFilt * P_TagF;
		};
		GoodsFilt_v24 fv24;
		THROW(fv24.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
#define CPYFLD(f) f = fv24.f
		CPYFLD(InitOrder);
		CPYFLD(MtxLocID);
		CPYFLD(BrandOwnerID);
		CPYFLD(CodeArID);
		CPYFLD(GrpID);
		CPYFLD(ManufID);
		CPYFLD(ManufCountryID);
		CPYFLD(UnitID);
		CPYFLD(PhUnitID);
		CPYFLD(SupplID);
		CPYFLD(GoodsTypeID);
		CPYFLD(TaxGrpID);
		CPYFLD(LocID_);
		CPYFLD(LotPeriod);
		CPYFLD(Flags);
		CPYFLD(VatRate);
		CPYFLD(VatDate);
		CPYFLD(BrandID_);
		CPYFLD(Ep);
		CPYFLD(SrchStr_);
		CPYFLD(BarcodeLen);
		CPYFLD(GrpIDList);
		CPYFLD(ManufList);
		CPYFLD(LocList);
		CPYFLD(BrandList);
#undef CPYFLD
		THROW_SL(TSDupPtr <SysJournalFilt> (&P_SjF, fv24.P_SjF));
		THROW_SL(TSDupPtr <TagFilt> (&P_TagF, fv24.P_TagF));
		GoodsStrucID = 0;
		ok = 1;
	}
	if(ver == -23) {
		class GoodsFilt_v23 : public PPBaseFilt {
		public:
			GoodsFilt_v23() : PPBaseFilt(PPFILT_GOODS, 0, -23), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(GoodsFilt_v23, ReserveStart), offsetof(GoodsFilt_v23, SrchStr_) - offsetof(GoodsFilt_v23, ReserveStart));
				SetBranchSString(offsetof(GoodsFilt_v23, SrchStr_));
				SetBranchSString(offsetof(GoodsFilt_v23, BarcodeLen));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v23, GrpIDList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v23, ManufList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v23, LocList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v23, BrandList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v23, BrandOwnerList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(GoodsFilt_v23, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(GoodsFilt_v23, P_TagF));
				Init(1, 0);
			}
			char   ReserveStart[4];
			PPID   UhttStoreID;
			PPID   RestrictQuotKindID;
			int32  InitOrder;
			PPID   MtxLocID;
			PPID   BrandOwnerID;
			PPID   CodeArID;
			PPID   GrpID;
			PPID   ManufID;
			PPID   ManufCountryID;
			PPID   UnitID;
			PPID   PhUnitID;
			PPID   SupplID;
			PPID   GoodsTypeID;
			PPID   TaxGrpID;
			PPID   LocID_;
			DateRange LotPeriod;
			long   Flags;
			long   VatRate;
			LDATE  VatDate;
			PPID   BrandID_;
			ExtParams_Before24 Ep;
			SString SrchStr_;
			SString BarcodeLen;
			ObjIdListFilt GrpIDList;
			ObjIdListFilt ManufList;
			ObjIdListFilt LocList;
			ObjIdListFilt BrandList;
			ObjIdListFilt BrandOwnerList;
			SysJournalFilt * P_SjF;
			TagFilt * P_TagF;
		};
		GoodsFilt_v23 fv23;
		THROW(fv23.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
#define CPYFLD(f) f = fv23.f
		CPYFLD(InitOrder);
		CPYFLD(MtxLocID);
		CPYFLD(BrandOwnerID);
		CPYFLD(CodeArID);
		CPYFLD(GrpID);
		CPYFLD(ManufID);
		CPYFLD(ManufCountryID);
		CPYFLD(UnitID);
		CPYFLD(PhUnitID);
		CPYFLD(SupplID);
		CPYFLD(GoodsTypeID);
		CPYFLD(TaxGrpID);
		CPYFLD(LocID_);
		CPYFLD(LotPeriod);
		CPYFLD(Flags);
		CPYFLD(VatRate);
		CPYFLD(VatDate);
		CPYFLD(BrandID_);
		CPYFLD(SrchStr_);
		CPYFLD(BarcodeLen);
		CPYFLD(GrpIDList);
		CPYFLD(ManufList);
		CPYFLD(LocList);
		CPYFLD(BrandList);
#undef CPYFLD
		{
			Ep.GdsClsID    = fv23.Ep.GdsClsID;
			Ep.KindList    = fv23.Ep.KindID;
			Ep.AddObjList  = fv23.Ep.AddObjID;
			Ep.GradeList   = fv23.Ep.GradeID;
			Ep.AddObj2List = fv23.Ep.AddObj2ID;
			Ep.DimX_Rng = fv23.Ep.DimX;
			Ep.DimY_Rng = fv23.Ep.DimY;
			Ep.DimZ_Rng = fv23.Ep.DimZ;
			Ep.DimW_Rng = fv23.Ep.DimW;
		}
		THROW_SL(TSDupPtr <SysJournalFilt> (&P_SjF, fv23.P_SjF));
		THROW_SL(TSDupPtr <TagFilt> (&P_TagF, fv23.P_TagF));
		ok = 1;
	}
	else if(ver == -22) {
		class GoodsFilt_v22 : public PPBaseFilt {
		public:
			GoodsFilt_v22() : PPBaseFilt(PPFILT_GOODS, 0, -22), P_SjF(0), P_TagF(0)
			{
				SetFlatChunk(offsetof(GoodsFilt_v22, ReserveStart), offsetof(GoodsFilt_v22, SrchStr_) - offsetof(GoodsFilt_v22, ReserveStart));
				SetBranchSString(offsetof(GoodsFilt_v22, SrchStr_));
				SetBranchSString(offsetof(GoodsFilt_v22, BarcodeLen));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v22, GrpIDList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v22, ManufList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v22, LocList));
				SetBranchObjIdListFilt(offsetof(GoodsFilt_v22, BrandList));
				SetBranchBaseFiltPtr(PPFILT_SYSJOURNAL, offsetof(GoodsFilt_v22, P_SjF));
				SetBranchBaseFiltPtr(PPFILT_TAG, offsetof(GoodsFilt_v22, P_TagF));
				Init(1, 0);
			}
			char   ReserveStart[12];
			int32  InitOrder;
			PPID   MtxLocID;
			PPID   BrandOwnerID;
			PPID   CodeArID;
			PPID   GrpID;
			PPID   ManufID;
			PPID   ManufCountryID;
			PPID   UnitID;
			PPID   PhUnitID;
			PPID   SupplID;
			PPID   GoodsTypeID;
			PPID   TaxGrpID;
			PPID   LocID_;
			DateRange LotPeriod;
			long   Flags;
			long   VatRate;
			LDATE  VatDate;
			PPID   BrandID_;
			ExtParams_Before24 Ep;
			SString SrchStr_;
			SString BarcodeLen;
			ObjIdListFilt GrpIDList;
			ObjIdListFilt ManufList;
			ObjIdListFilt LocList;
			ObjIdListFilt BrandList;
			SysJournalFilt * P_SjF;
			TagFilt * P_TagF;
		};
		GoodsFilt_v22 fv22;
		THROW(fv22.Read(rBuf, 0));
		memzero(ReserveStart, sizeof(ReserveStart));
#define CPYFLD(f) f = fv22.f
		CPYFLD(InitOrder);
		CPYFLD(MtxLocID);
		CPYFLD(BrandOwnerID);
		CPYFLD(CodeArID);
		CPYFLD(GrpID);
		CPYFLD(ManufID);
		CPYFLD(ManufCountryID);
		CPYFLD(UnitID);
		CPYFLD(PhUnitID);
		CPYFLD(SupplID);
		CPYFLD(GoodsTypeID);
		CPYFLD(TaxGrpID);
		CPYFLD(LocID_);
		CPYFLD(LotPeriod);
		CPYFLD(Flags);
		CPYFLD(VatRate);
		CPYFLD(VatDate);
		CPYFLD(BrandID_);
		// @v8.6.4 CPYFLD(Ep);
		CPYFLD(SrchStr_);
		CPYFLD(BarcodeLen);
		CPYFLD(GrpIDList);
		CPYFLD(ManufList);
		CPYFLD(LocList);
		CPYFLD(BrandList);
#undef CPYFLD
		{
			Ep.GdsClsID    = fv22.Ep.GdsClsID;
			Ep.KindList    = fv22.Ep.KindID;
			Ep.AddObjList  = fv22.Ep.AddObjID;
			Ep.GradeList   = fv22.Ep.GradeID;
			Ep.AddObj2List = fv22.Ep.AddObj2ID;
			Ep.DimX_Rng = fv22.Ep.DimX;
			Ep.DimY_Rng = fv22.Ep.DimY;
			Ep.DimZ_Rng = fv22.Ep.DimZ;
			Ep.DimW_Rng = fv22.Ep.DimW;
		}
		THROW_SL(TSDupPtr <SysJournalFilt> (&P_SjF, fv22.P_SjF));
		THROW_SL(TSDupPtr <TagFilt> (&P_TagF, fv22.P_TagF));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

/*virtual*/int SLAPI GoodsFilt::Describe(long flags, SString & rBuf) const
{
	PutObjMembToBuf(PPOBJ_PERSON,     BrandOwnerID,   STRINGIZE(BrandOwnerID),   rBuf);
	PutObjMembToBuf(PPOBJ_UNIT,       UnitID,         STRINGIZE(UnitID),         rBuf);
	PutObjMembToBuf(PPOBJ_UNIT,       PhUnitID,       STRINGIZE(PhUnitID),       rBuf);
	PutObjMembToBuf(PPOBJ_WORLD,      ManufCountryID, STRINGIZE(ManufCountryID), rBuf);
	PutObjMembToBuf(PPOBJ_ARTICLE,    SupplID,        STRINGIZE(SupplID),        rBuf);
	PutObjMembToBuf(PPOBJ_GOODSTYPE,  GoodsTypeID,    STRINGIZE(GoodsTypeID),    rBuf);
	PutObjMembToBuf(PPOBJ_GOODSTAX,   TaxGrpID,       STRINGIZE(TaxGrpID),       rBuf);
	PutMembToBuf(&LotPeriod, STRINGIZE(LotPeriod),    rBuf);
	PutMembToBuf(VatRate,    STRINGIZE(VatRate),   rBuf);
	PutMembToBuf(VatDate,    STRINGIZE(VatDate),   rBuf);
	PutMembToBuf(SrchStr_, STRINGIZE(SrchStr_), rBuf);
	PutMembToBuf(BarcodeLen, STRINGIZE(BarcodeLen), rBuf);
	PutObjMembListToBuf(PPOBJ_GOODSGROUP, &GrpIDList, STRINGIZE(GrpIDList), rBuf);
	PutObjMembListToBuf(PPOBJ_PERSON,     &ManufList, STRINGIZE(ManufList), rBuf);
	PutObjMembListToBuf(PPOBJ_LOCATION,   &LocList,   STRINGIZE(LocList),   rBuf);
	PutObjMembListToBuf(PPOBJ_BRAND,      &BrandList, STRINGIZE(BrandList), rBuf);
	{
		long id = 1;
		StrAssocArray flag_list;
		// @v9.9.3 if(Flags & fUseGrpList)            flag_list.Add(id++, STRINGIZE(fUseGrpList));
		// @v9.9.3 if(Flags & fUseUnitMask)           flag_list.Add(id++, STRINGIZE(fIntUnitOnly));
		if(Flags & fWoBrand)               flag_list.Add(id++, STRINGIZE(fWoBrand)); // @v10.6.8
		if(Flags & fIntUnitOnly)           flag_list.Add(id++, STRINGIZE(fIntUnitOnly));
		if(Flags & fFloatUnitOnly)         flag_list.Add(id++, STRINGIZE(fFloatUnitOnly));
		if(Flags & fNegation)              flag_list.Add(id++, STRINGIZE(fNegation));
		if(Flags & fGenGoods)              flag_list.Add(id++, STRINGIZE(fGenGoods));
		if(Flags & fGroupGenGoods)         flag_list.Add(id++, STRINGIZE(fGroupGenGoods));
		if(Flags & fUndefType)             flag_list.Add(id++, STRINGIZE(fUndefType));
		if(Flags & fNewLots)               flag_list.Add(id++, STRINGIZE(fNewLots));
		if(Flags & fExcludeAsset)          flag_list.Add(id++, STRINGIZE(fExcludeAsset));
		if(Flags & fIncludeIntr)           flag_list.Add(id++, STRINGIZE(fIncludeIntr));
		if(Flags & fShowBarcode)           flag_list.Add(id++, STRINGIZE(fShowBarcode));
		if(Flags & fShowCargo)             flag_list.Add(id++, STRINGIZE(fShowCargo));
		if(Flags & fHidePassive)           flag_list.Add(id++, STRINGIZE(fHidePassive));
		if(Flags & fPassiveOnly)           flag_list.Add(id++, STRINGIZE(fPassiveOnly));
		if(Flags & fHideGeneric)           flag_list.Add(id++, STRINGIZE(fHideGeneric)); // @v10.7.7
		if(Flags & fGenGoodsOnly)          flag_list.Add(id++, STRINGIZE(fGenGoodsOnly));
		if(Flags & fWOTaxGdsOnly)          flag_list.Add(id++, STRINGIZE(fWOTaxGdsOnly));
		if(Flags & fNoZeroRestOnLotPeriod) flag_list.Add(id++, STRINGIZE(fNoZeroRestOnLotPeriod));
		if(Flags & fNoDisOnly)             flag_list.Add(id++, STRINGIZE(fNoDisOnly));
		if(Flags & fShowStrucType)         flag_list.Add(id++, STRINGIZE(fShowStrucType));
		if(Flags & fNotUseViewOptions)     flag_list.Add(id++, STRINGIZE(fNotUseViewOptions));
		if(Flags & fShowGoodsWOStruc)      flag_list.Add(id++, STRINGIZE(fShowGoodsWOStruc));
		if(Flags & fWoTaxGrp)              flag_list.Add(id++, STRINGIZE(fWoTaxGrp));
		if(Flags & fRestrictByMatrix)      flag_list.Add(id++, STRINGIZE(fRestrictByMatrix));
		if(Flags & fShowArCode)            flag_list.Add(id++, STRINGIZE(fShowArCode));
		if(Flags & fShowOwnArCode)         flag_list.Add(id++, STRINGIZE(fShowOwnArCode));
		if(Flags & fShowWoArCode)          flag_list.Add(id++, STRINGIZE(fShowWoArCode));
		PutFlagsMembToBuf(&flag_list, STRINGIZE(Flags), rBuf);
	}
	CALLPTRMEMB(P_SjF, Describe(flags, rBuf));
	return 1;
}

int SLAPI GoodsFilt::Setup()
{
	if(GrpIDList.GetCount() > 1)
		GrpID = 0;
	else {
		if(GrpIDList.GetCount() == 1)
			GrpID = GrpIDList.Get().at(0);
		GrpIDList.Set(0);
	}
	SrchStr_.Strip();
	BarcodeLen.Strip();
	if(Flags & fIntUnitOnly && Flags & fFloatUnitOnly)
		Flags &= ~(fIntUnitOnly | fFloatUnitOnly);
	else if(Flags & (fIntUnitOnly | fFloatUnitOnly))
		UnitID = 0;
	if(LocID_) {
		LocList.Add(LocID_);
		LocID_ = 0;
	}
	// @v10.6.8 {
	if(Flags & fWoBrand) {
		ResultBrandList.Set(0);
	}
	else { // } @v10.6.8 
		if(BrandID_) {
			BrandList.Add(BrandID_);
			BrandID_ = 0;
		}
		BrandList.Sort();
		if(BrandOwnerID) {
			BrandOwnerList.Add(BrandOwnerID);
			BrandOwnerID = 0;
		}
		BrandOwnerList.Sort();
		CalcResultBrandList(ResultBrandList);
	}
	if(P_SjF && P_SjF->IsEmpty())
		ZDELETE(P_SjF);
	if(oneof2(InitOrder, PPViewGoods::OrdByBarcode, PPViewGoods::OrdByBarcode_Name))
		Flags |= fShowBarcode;
	return 1;
}

int SLAPI GoodsFilt::IsEmpty() const
{
	const long nemp_fl = (fWithStrucOnly|fIntUnitOnly|fFloatUnitOnly|fHidePassive|
		fPassiveOnly|fHideGeneric|fGenGoodsOnly|fWOTaxGdsOnly|fNoDisOnly|fRestrictByMatrix|fOutOfMatrix|fActualOnly|fHasImages|fUseIndepWtOnly|fWoBrand); 
		// @v10.6.8 fWoBrand // @v10.7.7 fHideGeneric
	// Setup();
	/*
	return !(GrpID || ManufID || UnitID || SupplID || GoodsTypeID || BrandID || PhUnitID ||
		TaxGrpID || VatRate || ManufCountryID || !LotPeriod.IsZero() || LocID ||
		(Flags & nemp_fl) || SrchStr.NotEmpty() || !GrpIDList.IsEmpty() || BarcodeLen.NotEmpty() || Ep.GdsClsID);
	*/
	if(GrpID)
		return 0;
	else if(ManufID)
		return 0;
	else if(UnitID)
		return 0;
	else if(SupplID)
		return 0;
	else if(GoodsTypeID)
		return 0;
	else if(BrandID_)
		return 0;
	else if(GoodsStrucID) // @v10.0.12
		return 0;
	else if(BrandOwnerID)
		return 0;
	else if(PhUnitID)
		return 0;
	else if(TaxGrpID)
		return 0;
	else if(VatRate)
		return 0;
	else if(ManufCountryID)
		return 0;
	else if(!LotPeriod.IsZero())
		return 0;
	else if(LocID_)
		return 0;
	else if(UhttStoreID)
		return 0;
	else if(RestrictQuotKindID)
		return 0;
	else if(Flags & nemp_fl)
		return 0;
	else if(Flags & fShowArCode && !(Flags & fShowWoArCode))
		return 0;
	else if(SrchStr_.NotEmpty())
		return 0;
	else if(!GrpIDList.IsEmpty())
		return 0;
	else if(BarcodeLen.NotEmpty())
		return 0;
	else if(Ep.GdsClsID)
		return 0;
	else if(!LocList.IsEmpty())
		return 0;
	else if(!BrandList.IsEmpty())
		return 0;
	else if(!BrandOwnerList.IsEmpty())
		return 0;
	else if(P_SjF && !P_SjF->IsEmpty())
		return 0;
	else if(P_TagF && !P_TagF->IsEmpty())
		return 0;
	else
		return 1;
}

int SLAPI GoodsFilt::GetExtssData(int fldID, SString & rBuf) const { return PPGetExtStrData_def(fldID, extssNameText, SrchStr_, rBuf); }
int SLAPI GoodsFilt::PutExtssData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, SrchStr_, pBuf); }
const ObjIdListFilt & GoodsFilt::GetResultBrandList() const { return ResultBrandList; }
int SLAPI GoodsFilt::IsRestrictedByAr() const { return BIN(Flags & fShowArCode && !(Flags & fShowWoArCode) && (CodeArID || Flags & GoodsFilt::fShowOwnArCode)); }

int FASTCALL GoodsFilt::GetBarcodeLenList(PPIDArray & rList) const
{
	int    ok = -1;
	rList.clear();
	if(BarcodeLen.NotEmpty()) {
		StringSet ss(',', BarcodeLen);
		SString len;
		for(uint ssp = 0; ss.get(&ssp, len);) {
			rList.addUnique(len.Strip().ToLong());
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL GoodsFilt::GetBarcodePrefixList(StringSet & rSet) const
{
	int    ok = -1;
	rSet.clear();
	return ok;
}

int SLAPI GoodsFilt::CalcResultBrandList(ObjIdListFilt & rResult) const
{
	rResult.Set(0);
	rResult.Add(&BrandList.Get());
	if(BrandOwnerList.GetCount()) {
		BrandFilt bf;
		bf.OwnerList = BrandOwnerList;
		PPObjBrand brobj;
		PPIDArray local_brand_list;
		brobj.GetListByFilt(&bf, &local_brand_list);
		rResult.Add(&local_brand_list);
	}
	return rResult.GetCount() ? 1 : -1;
}
//
// Storing format for GoodsFilt
//
struct __GoodsFilt {
	int32  ObjType;
	int32  ObjID;
	int32  PropID;
	/*
		Номер версии записи. Предназначен для идентификации формата
		записи при чтении ее из БД.
		При добавлении новых полей в структуру следует уменьшать номер
		версии (-11, -12, -13, -14 и т.д.) и в функции GoodsFilt::ReadFromProp
		следить за номером версии считываемой записи и соответственно ее
		обрабатывать.
	*/
	int32  VerTag; // -14 // @v3.11.3

	PPID   GrpID;
	PPID   ManufID;
	PPID   UnitID;
	PPID   PhUnitID;
	PPID   SupplID;
	PPID   GoodsTypeID;
	PPID   TaxGrpID;
	long   Flags;
	PPID   ManufCountryID; // v-12
	PPID   LocID;
	DateRange   LotPeriod; // v-13
	long   VatRate;        // v-15 // Ставка НДС (0.01), которой облагается товар
	LDATE  VatDate;        // v-15 // Дата, на которую рассчитывается VatRate
	PPID   BrandID;        // v-15 // Брэнд. (Просранство - за счет Reserve)
	PPID   CodeArID;       // v-18
	PPID   BrandOwnerID;   // v-20
	int32  MtxLocID;       //
	int32  InitOrder;      //
	char   Reserve[12];    // v-15
	//SrchStr[]
	//uint32 GrpIDListCount;
	//GrpIDList[]
	//BarcodeLenStr[]      // v-14
	//GoodsFilt::ExtParams // v-16, v-17
	//LocList              // v-19
	//BrandList            // v-19
	//SysJournalFilt       // v-21
	//TagFilt              // v-22
	//BrandOwnerList       // v-23
};

char * SLAPI GoodsFilt::WriteObjIdListFilt(char * p, const ObjIdListFilt & rList) const
{
	if(!rList.IsEmpty()) {
		*reinterpret_cast<uint32 *>(p) = rList.GetCount();
		p += sizeof(uint32);
		PPID * p_id = reinterpret_cast<PPID *>(p);
		for(uint i = 0; i < rList.GetCount(); i++)
			*p_id++ = rList.Get().get(i);
		p = reinterpret_cast<char *>(p_id);
	}
	else {
		*reinterpret_cast<uint32 *>(p) = 0;
		p += sizeof(uint32);
	}
	return p;
}

const void * SLAPI GoodsFilt::ReadObjIdListFilt(const /*char*/void * p, ObjIdListFilt & rList)
{
	const  uint8 * _ptr = static_cast<const uint8 *>(p);
	uint   list_count = (uint)*reinterpret_cast<const uint32 *>(_ptr);
	_ptr += sizeof(uint32);
	rList.Set(0);
	for(uint i = 0; i < list_count; i++) {
		PPID   id = *reinterpret_cast<const PPID *>(_ptr);
		if(id > 0)
			rList.Add(id);
		_ptr += sizeof(PPID);
	}
	return _ptr;
}

#if 0 // {
int SLAPI GoodsFilt::WriteToProp(PPID obj, PPID id, PPID prop)
{
	int    ok = 1;
	__GoodsFilt * p_buf = 0;
	SBuffer temp_ser_buf;
	Setup();
	if(IsEmpty())
		ok = PPRef->RemoveProp(obj, id, prop, 0);
	else {
		SSerializeContext sctx;
        THROW(Serialize(+1, temp_ser_buf, &sctx));
		PutPropSBuffer

		char * p = 0;
		size_t rec_size = sizeof(__GoodsFilt) + 2 + sizeof(uint32) * 4;
		rec_size += SrchStr_.Len();
		rec_size += sizeof(PPID) * GrpIDList.GetCount();
		rec_size += BarcodeLen.Len();
		if(Ep.GdsClsID)
			rec_size += sizeof(Ep);
		else
			rec_size += sizeof(Ep.GdsClsID);
		// @v6.0.2 {
		rec_size += sizeof(PPID) * LocList.GetCount();
		rec_size += sizeof(PPID) * BrandList.GetCount();
		// } @v6.0.2
		// @v6.4.2 (-21) {
		rec_size += sizeof(int32); // Версия P_SjF (если 0 то P_SjF пустой)
		if(P_SjF && !P_SjF->IsEmpty()) {
			rec_size += sizeof(P_SjF->Period);
			rec_size += sizeof(P_SjF->BegTm);
			rec_size += sizeof(P_SjF->UserID);
			rec_size += sizeof(P_SjF->Flags);
			rec_size += sizeof(uint16); // count of P_SjF->ActionIDList
			rec_size += sizeof(P_SjF->ActionIDList.getCount() * sizeof(int32));
		}
		// } @v6.4.2 (-21)
		// @v7.2.0 (-22) {
		rec_size += sizeof(int32); // Версия P_TagF (если 0 то P_TagF пустой)
		if(P_TagF && !P_TagF->IsEmpty()) {
			rec_size += sizeof(P_TagF->Flags);
			temp_ser_buf.Clear();
			P_TagF->TagsRestrict.Write(temp_ser_buf, 0);
			rec_size += sizeof(uint32); // Пространство под размер буфера
			rec_size += temp_ser_buf.GetAvailableSize();
		}
		// } @v7.2.0 (-22)
		rec_size += sizeof(PPID) * BrandOwnerList.GetCount(); // @v7.7.9
		rec_size = MAX(rec_size, PROPRECFIXSIZE);
		p_buf = (__GoodsFilt*)SAlloc::C(1, rec_size);

		p_buf->ObjType = obj;
		p_buf->ObjID   = id;
		p_buf->PropID  = prop;
		p_buf->VerTag  = -23; // @v5.6.11 // @v6.0.2 -18-->-19 // @v6.0.7 -19-->-20 // @v6.4.2 -20-->-21 @v7.3.10 -21-->-22 @v7.7.9 -22-->-23
		p_buf->GrpID   = GrpID;
		p_buf->ManufID = ManufID;
		p_buf->ManufCountryID = ManufCountryID;
		p_buf->UnitID   = UnitID;
		p_buf->PhUnitID = PhUnitID;
		p_buf->SupplID  = SupplID;
		p_buf->GoodsTypeID = GoodsTypeID;
		p_buf->TaxGrpID    = TaxGrpID;
		p_buf->LocID     = LocID_;
		p_buf->LotPeriod = LotPeriod;
		p_buf->Flags     = Flags;
		// @v4.3.9 @v-15 {
		p_buf->VatRate = VatRate;
		p_buf->VatDate = VatDate;
		// }
		p_buf->BrandID  = BrandID_;
		p_buf->CodeArID = CodeArID;
		p_buf->BrandOwnerID = BrandOwnerID;
		p_buf->MtxLocID = MtxLocID;
		p_buf->InitOrder = InitOrder;
		p = (char *)(p_buf + 1);
		strnzcpy(p, SrchStr_, 0);
		p += sstrlen(p) + 1;
		p = WriteObjIdListFilt(p, GrpIDList);
		// @v-14 {
		strnzcpy(p, BarcodeLen, 0);
		p += sstrlen(p) + 1;
		// }
		// @v4.5.5 @v-16, @v-17 {
		if(Ep.GdsClsID) {
			memcpy(p, &Ep, sizeof(Ep));
			p += sizeof(Ep);
		}
		else {
			memcpy(p, &Ep.GdsClsID, sizeof(Ep.GdsClsID));
			p += sizeof(Ep.GdsClsID);
		}
		// }
		// @v6.0.2 (-19) {
		p = WriteObjIdListFilt(p, LocList);
		p = WriteObjIdListFilt(p, BrandList);
		// } (-19) @v6.0.2
		// @v6.4.2 (-21) {
		if(P_SjF && !P_SjF->IsEmpty()) {
			*(int32 *)p = P_SjF->GetVer();
			p += sizeof(int32);
#define CPY(f,t) *(t *)p = P_SjF->f; p += sizeof(t)
			CPY(Period, DateRange);
			CPY(BegTm, LTIME);
			CPY(UserID, PPID);
			CPY(Flags, long);
#undef CPY
			uint16 c = (uint16)P_SjF->ActionIDList.getCount();
			PTR16(p)[0] = c;
			p += sizeof(uint16);
			for(uint16 i = 0; i < c; i++) {
				*(int32 *)p = P_SjF->ActionIDList.get(i);
				p += sizeof(int32);
			}
		}
		else {
			*(int32 *)p = 0;
			p += sizeof(int32);
		}
		// } @v6.4.2 (-21)
		// @v7.2.0 (-22) {
		if(P_TagF && !P_TagF->IsEmpty()) {
			size_t s = 0;
			*(int32 *)p = P_TagF->GetVer();
			p += sizeof(int32);
			*(int32 *)p = P_TagF->Flags;
			p += sizeof(int32);
			{
				temp_ser_buf.Clear(); // SBuffer
				P_TagF->TagsRestrict.Write(temp_ser_buf, 0);
				s = temp_ser_buf.GetAvailableSize();
				*(uint32 *)p = s;
				p += sizeof(uint32);
				memcpy(p, temp_ser_buf, s);
				p += s;
			}
		}
		else {
			*(int32 *)p = 0;
			p += sizeof(int32);
		}
		// } @v7.2.0 (-22)
		p = WriteObjIdListFilt(p, BrandOwnerList); // @v7.7.9
		ok = PPRef->PutProp(obj, id, prop, p_buf, rec_size);
		SAlloc::F(p_buf);
	}
	return ok;
}
#endif // } 0

int SLAPI GoodsFilt::WriteToProp(PPID obj, PPID id, PPID prop, PPID propBefore8604)
{
	int    ok = 1;
	Setup();
	if(IsEmpty()) {
		THROW(PPRef->RemoveProperty(obj, id, prop, -1));
	}
	else {
		SBuffer buffer;
		SSerializeContext sctx;
        THROW(Serialize(+1, buffer, &sctx));
		THROW(PPRef->PutPropSBuffer(obj, id, prop, buffer, -1));
	}
	CATCHZOK
	return ok;
}

int SLAPI GoodsFilt::ReadFromProp(PPID obj, PPID id, PPID prop, PPID propBefore8604)
{
	int    ok = -1;
	SBuffer buffer;
	if(PPRef->GetPropSBuffer(obj, id, prop, buffer) > 0) {
		SSerializeContext sctx;
        THROW(Serialize(-1, buffer, &sctx));
        ok = 1;
	}
	else if(propBefore8604) {
		THROW(ok = ReadFromProp_Before8604(obj, id, propBefore8604));
	}
	CATCHZOK
	return ok;
}

int SLAPI GoodsFilt::ReadFromProp_Before8604(PPID obj, PPID id, PPID prop)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	uint   i, c;
	__GoodsFilt * p_buf = 0;
	const uint8 * p = 0;
	size_t prop_size = 0;
	if(p_ref->GetPropActualSize(obj, id, prop, &prop_size) > 0) {
		THROW_MEM(p_buf = static_cast<__GoodsFilt *>(SAlloc::C(1, prop_size)));
		THROW(p_ref->GetProperty(obj, id, prop, p_buf, prop_size) > 0);
		if(p_buf->VerTag <= -11 && p_buf->VerTag > -100) {
			GrpID       = p_buf->GrpID;
			ManufID     = p_buf->ManufID;
			UnitID      = p_buf->UnitID;
			PhUnitID    = p_buf->PhUnitID;
			SupplID     = p_buf->SupplID;
			GoodsTypeID = p_buf->GoodsTypeID;
			TaxGrpID    = p_buf->TaxGrpID;
			Flags       = p_buf->Flags;
			if(p_buf->VerTag <= -12) {
				ManufCountryID = p_buf->ManufCountryID;
				if(p_buf->VerTag <= -13) {
					LotPeriod = p_buf->LotPeriod;
					LocID_ = p_buf->LocID;
					if(p_buf->VerTag <= -15) {
						VatRate  = p_buf->VatRate;
						VatDate  = p_buf->VatDate;
						BrandID_ = p_buf->BrandID;
						if(p_buf->VerTag <= -18)
							CodeArID = p_buf->CodeArID;
						if(p_buf->VerTag <= -20)
							BrandOwnerID = p_buf->BrandOwnerID;
						if(p_buf->VerTag <= -22) {
							MtxLocID = p_buf->MtxLocID;
							InitOrder = p_buf->InitOrder;
						}
						p = PTR8C(p_buf+1);
					}
					else {
						p = PTR8C(p_buf+1) - sizeof(p_buf->VatRate) - sizeof(p_buf->VatDate)  - sizeof(p_buf->BrandID) -
							sizeof(p_buf->CodeArID) - sizeof(p_buf->BrandOwnerID) - 20;
					}
				}
				else
					p = PTR8C(p_buf+1) - sizeof(p_buf->LotPeriod) - sizeof(p_buf->LocID);
			}
			else
				p = PTR8C(p_buf+1) - sizeof(p_buf->ManufCountryID) - sizeof(p_buf->LotPeriod) - sizeof(p_buf->LocID);
			SrchStr_ = reinterpret_cast<const char *>(p);
			p += sstrlen(p) + 1;
			p = PTR8C(ReadObjIdListFilt(p, GrpIDList));
			if(p_buf->VerTag <= -14) {
				BarcodeLen = reinterpret_cast<const char *>(p);
				p += sstrlen(p) + 1;
			}
			if(p_buf->VerTag <= -17) {
				const PPID gds_cls_id = *reinterpret_cast<const PPID *>(p);
				Ep.Z();
				if(gds_cls_id) {
					ExtParams_Before24 _ep;
					memcpy(&_ep, p, sizeof(_ep));
					Ep.GdsClsID   = _ep.GdsClsID;
					Ep.KindList   = _ep.KindID;
					Ep.AddObjList = _ep.AddObjID;
					Ep.AddObj2List = _ep.AddObj2ID;
					Ep.GradeList  = _ep.GradeID;
					Ep.DimX_Rng = _ep.DimX;
					Ep.DimY_Rng = _ep.DimY;
					Ep.DimZ_Rng = _ep.DimZ;
					Ep.DimW_Rng = _ep.DimW;
					p += sizeof(_ep);
				}
				else {
					p += sizeof(gds_cls_id);
				}
			}
			else if(p_buf->VerTag <= -16) {
				const PPID gds_cls_id = *reinterpret_cast<const PPID *>(p);
				Ep.Z();
				if(gds_cls_id) {
					//
					// В более поздних версиях добавились поля сверх перечисленных
					//
					struct ExtParams_16 {
						PPID   GdsClsID;
						PPID   KindID;
						PPID   AddObjID;
						PPID   GradeID;
						RealRange DimX;
						RealRange DimY;
						RealRange DimZ;
					};
					const ExtParams_16 * p_ep16 = reinterpret_cast<const ExtParams_16 *>(p);
					Ep.GdsClsID   = p_ep16->GdsClsID;
					Ep.KindList   = p_ep16->KindID;
					Ep.AddObjList = p_ep16->AddObjID;
					Ep.GradeList  = p_ep16->GradeID;
					Ep.DimX_Rng = p_ep16->DimX;
					Ep.DimY_Rng = p_ep16->DimY;
					Ep.DimZ_Rng = p_ep16->DimZ;
					p += sizeof(ExtParams_16);
				}
				else
					p += sizeof(Ep.GdsClsID);
			}
			if(p_buf->VerTag <= -19) {
				p = PTR8C(ReadObjIdListFilt(p, LocList));
				p = PTR8C(ReadObjIdListFilt(p, BrandList));
			}
			ZDELETE(P_SjF);
			if(p_buf->VerTag <= -21) {
				int32  sjf_ver = *reinterpret_cast<const int32 *>(p);
				p += sizeof(int32);
				if(sjf_ver) {
					P_SjF = new SysJournalFilt;
#define CPY(f,t) P_SjF->f = *reinterpret_cast<const t *>(p); p += sizeof(t)
					CPY(Period, DateRange);
					CPY(BegTm, LTIME);
					CPY(UserID, PPID);
					CPY(Flags, long);
#undef CPY
					uint16 c = *reinterpret_cast<const uint16 *>(p);
					p += sizeof(uint16);
					for(uint16 i = 0; i < c; i++) {
						P_SjF->ActionIDList.add(*reinterpret_cast<const int32 *>(p));
						p += sizeof(int32);
					}
				}
			}
			if(p_buf->VerTag <= -22) {
				int32  tf_ver = *reinterpret_cast<const int32 *>(p);
				p += sizeof(int32);
				if(tf_ver) {
					P_TagF = new TagFilt;
					P_TagF->Flags = *reinterpret_cast<const int32 *>(p);
					p += sizeof(int32);
					uint32 s = *reinterpret_cast<const uint32 *>(p);
					p += sizeof(uint32);

					SBuffer temp_ser_buf;
					temp_ser_buf.Write(p, s);
					p += s;
					P_TagF->TagsRestrict.Read(temp_ser_buf, 0);
				}
			}
			if(p_buf->VerTag <= -23) {
				p = PTR8C(ReadObjIdListFilt(p, BrandOwnerList));
			}
		}
		else {
			const struct GoodsFilt_0 {
				PPID   GrpID;       //
				PPID   ManufID;     //
				PPID   UnitID;      //
				long   Flags;       //
				PPID   SupplID;     // 20 bytes
				PPID   GoodsTypeID; //
				PPID   PhUnitID;    // 28 bytes
			} * p_old_data = reinterpret_cast<const GoodsFilt_0 *>(p_buf);

			GrpID       = p_old_data->GrpID;
			ManufID     = p_old_data->ManufID;
			UnitID      = p_old_data->UnitID;
			PhUnitID    = p_old_data->PhUnitID;
			SupplID     = p_old_data->SupplID;
			GoodsTypeID = p_old_data->GoodsTypeID;
			TaxGrpID    = 0;
			Flags       = p_old_data->Flags;
			GrpIDList.Set(0);
   	    	c = (uint)(reinterpret_cast<const PropertyTbl::Rec *>(p_buf)->Val2);
			for(i = 0; i < c; i++)
				GrpIDList.Add(*reinterpret_cast<const PPID *>(PTR8C(p_buf) + PROPRECFIXSIZE + i * sizeof(PPID)));
		}
	   	Setup();
	}
	else
		ok = -1;
	CATCHZOK
	SAlloc::F(p_buf);
	Setup();
	return ok;
}
//
//
//
class GoodsListDialog : public TDialog {
public:
	explicit GoodsListDialog(int disableAutoFill) : TDialog(DLG_GDSLST)
	{
		P_List = static_cast<SmartListBox *>(getCtrlView(CTL_GDSLST_LIST));
		setCtrlOption(CTL_GDSLST_LIST, ofFramed, 1);
		showCtrl(CTL_GDSLST_AFBUTTON, !disableAutoFill);
		SetupPPObjCombo(this, CTLSEL_GDSLST_GGRP, PPOBJ_GOODSGROUP, 0, OLW_LOADDEFONOPEN, 0);
		updateList();
	}
	int    setSelectedItem(PPID);
	PPID   getSelectedItem();
private:
	DECL_HANDLE_EVENT;
	void   searchBarcode();
	void   selectGoods(PPID grp, PPID goodsID);
	void   updateList();

	PPObjGoods GObj;
	SmartListBox * P_List;
};

PPID GoodsListDialog::getSelectedItem()
{
	PPID   id = 0;
	return (P_List && P_List->getCurID(&id)) ? id : 0;
}

int SLAPI SelectGoods(PPID & rGoodsID)
{
	int    ok = -1;
	PPID   goods_id = rGoodsID;
	GoodsListDialog * dlg = new GoodsListDialog(1);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->selectCtrl(CTLSEL_GDSLST_GGRP);
		dlg->setSelectedItem(goods_id);
		if(ExecView(dlg) == cmOK) {
			goods_id = dlg->getSelectedItem();
			ok = 1;
		}
		delete dlg;
	}
	else
		ok = 0;
	rGoodsID = goods_id;
	return ok;
}

int GoodsListDialog::setSelectedItem(PPID goodsID)
{
	int    ok = -1;
	if(P_List) {
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(goodsID, &goods_rec) > 0)
			setCtrlLong(CTLSEL_GDSLST_GGRP, goods_rec.ParentID);
		if(goodsID) {
			P_List->TransmitData(+1, &goodsID);
			if(P_List->HasState(SmartListBox::stDataFounded)) {
				P_List->Draw_();
				ok = 1;
			}
		}
	}
	return ok;
}

IMPL_HANDLE_EVENT(GoodsListDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_GDSLST_GGRP)) {
		updateList();
		selectCtrl(CTL_GDSLST_LIST); // @v9.1.1
	}
	else if(event.isCmd(cmaInsert) && IsCurrentView(P_List)) {
		PPID   c = 0L;
		PPID   grp_id = getCtrlLong(CTLSEL_GDSLST_GGRP);
		if(GObj.Edit(&c, reinterpret_cast<void *>(grp_id)) == cmOK)
			selectGoods(grp_id, c);
	}
	else if(event.isKeyDown(kbF2))
		searchBarcode();
	else if(event.isKeyDown(kbAltF2)) {
		PPID   n = 0;
		PPID   c = getSelectedItem();
		if(c && GObj.AddBySample(&n, c) == cmOK) {
			Goods2Tbl::Rec new_goods_rec;
			if(GObj.Fetch(n, &new_goods_rec) > 0) {
				selectGoods(new_goods_rec.ParentID, n);
			}
		}
	}
	else if(event.isKeyDown(KB_CTRLENTER) && IsCurrentView(P_List)) {
		PPID c = getSelectedItem();
		if(c && GObj.Edit(&c, 0L) == cmOK)
			updateList();
	}
	else if(IsInState(sfModal)) {
		if(event.isCmd(cmLBDblClk))
			endModal(cmOK);
		else if(event.isCmd(cmAutoFill))
			endModal(event.message.command);
	}
	else
		return;
	clearEvent(event);
}

void GoodsListDialog::selectGoods(PPID grp, PPID goodsID)
{
	if(P_List && goodsID) {
		setCtrlData(CTLSEL_GDSLST_GGRP, &grp);
		updateList();
		P_List->TransmitData(+1, &goodsID);
		if(P_List->HasState(SmartListBox::stDataFounded)) {
			selectCtrl(CTL_GDSLST_LIST);
			P_List->Draw_();
		}
	}
}

void GoodsListDialog::searchBarcode()
{
	int    r;
	Goods2Tbl::Rec rec;
	SString bcode;
	if((r = GObj.SelectGoodsByBarcode(0, 0, &rec, 0, &bcode)) > 0)
		selectGoods(rec.ParentID, rec.ID);
	else if(r == -2 && PPMessage(mfConf|mfYesNo, PPCFM_ADDNEWGOODS) == cmYes) {
		PPID   id = 0;
		Goods2Tbl::Rec goods_rec;
		PPID   grp_id = getCtrlLong(CTLSEL_GDSLST_GGRP);
		r = GObj.Edit(&id, gpkndGoods, grp_id, 0, bcode);
		if(r == cmOK && GObj.Search(id, &goods_rec) > 0) {
			grp_id = goods_rec.ParentID;
			selectGoods(grp_id, id);
		}
		else if(r == 0)
			PPError();
	}
}

void GoodsListDialog::updateList()
{
	if(P_List) {
		ListBoxDef * p_def = GObj.Selector(reinterpret_cast<void *>(getCtrlLong(CTLSEL_GDSLST_GGRP)));
		if(p_def) {
			if(p_def->GetCapability() & ListBoxDef::cFullInMem)
				SetupWordSelector(CTL_GDSLST_LIST, 0, 0, /*MIN_WORDSEL_SYMB*/4, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
			else
				ResetWordSelector(CTL_GDSLST_LIST);
			P_List->setDef(p_def);
			P_List->Draw_();
		}
	}
}
//
//
//
SLAPI PPViewGoods::PPViewGoods() : PPView(0, &Filt, PPVIEW_GOODS), P_TempTbl(0), P_Iter(0)
{
	Filt.GrpID = 0;
	//CurrentViewOrder = OrdByDefault;
}

SLAPI PPViewGoods::~PPViewGoods()
{
	RemoveTempAltGroup();
	delete P_TempTbl;
	delete P_Iter;
	DBRemoveTempFiles();
}
//
// Похоже, необходимости в этой функции нет, поскольку технология создания //
// временных альтернативных групп больше не используется //
//
void SLAPI PPViewGoods::RemoveTempAltGroup()
{
	if(Filt.GrpID < 0) {
		PPRef->Assc.Remove(PPASS_ALTGOODSGRP, Filt.GrpID, 0, 1);
		Filt.GrpID = 0;
	}
}

PPObjGoods * SLAPI PPViewGoods::GetObj() { return &GObj; }
int SLAPI PPViewGoods::IsAltFltGroup() { return (Filt.GrpID > 0 && PPObjGoodsGroup::IsAlt(Filt.GrpID) > 0); }
int SLAPI PPViewGoods::IsGenGoodsFlt() { return (Filt.GrpID > 0 && Filt.Flags & GoodsFilt::fGenGoods); }

PPBaseFilt * SLAPI PPViewGoods::CreateFilt(void * extraPtr) const
{
	GoodsFilt * p_filt = new GoodsFilt;
	p_filt->Flags |= GoodsFilt::fHidePassive;
	PPAccessRestriction accsr;
	p_filt->GrpID = ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID;
	return p_filt;
}

int SLAPI PPViewGoods::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	return Filt.IsA(pBaseFilt) ? GoodsFilterDialog(static_cast<GoodsFilt *>(pBaseFilt)) : 0;
}

int SLAPI PPViewGoods::OnExecBrowser(PPViewBrowser * pBrw)
{
	int    disable_group_selection = 0;
	PPAccessRestriction accsr;
	if(ObjRts.GetAccessRestriction(accsr).OnlyGoodsGrpID && accsr.CFlags & PPAccessRestriction::cfStrictOnlyGoodsGrp) {
		disable_group_selection = 1;
	}
	if(!disable_group_selection)
		pBrw->SetupToolbarCombo(PPOBJ_GOODSGROUP, Filt.GrpID, OLW_CANSELUPLEVEL|OLW_LOADDEFONOPEN, 0);
	return -1;
}

static int FASTCALL HasImages(const void * pData)
{
	struct Goods_ {
		long   ID;
		long   Flags;
	};
	return pData ? BIN(static_cast<const Goods_ *>(pData)->Flags & GF_HASIMAGES) : 0;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr); // @v9.5.5
	if(p_brw) {
		PPViewGoods * p_view = static_cast<PPViewGoods *>(p_brw->P_View); // @v9.5.5
		ok = p_view ? p_view->CellStyleFunc_(pData, col, paintAction, pStyle, p_brw) : -1;
	}
	return ok;
}

int SLAPI PPViewGoods::CellStyleFunc_(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pCellStyle, PPViewBrowser * pBrw)
{
	struct Goods_ {
		long   ID;
		long   Flags;
	};
	int    ok = -1;
	if(pBrw && pData && pCellStyle && col >= 0) {
		BrowserDef * p_def = pBrw->getDef();
		if(col >= 0 && col < p_def->getCountI()) {
			const BroColumn & r_col = p_def->at(col);
			if(col == 0) { // id
				if(static_cast<const Goods_ *>(pData)->Flags & GF_HASIMAGES)
					ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrGreen));
			}
			else if(col == 1) { // name
				if(static_cast<const Goods_ *>(pData)->Flags & GF_GENERIC)
					ok = pCellStyle->SetRightFigCircleColor(GetColorRef(SClrOrange));
				{
					const TagFilt & r_tag_filt = GObj.GetConfig().TagIndFilt;
					if(!r_tag_filt.IsEmpty()) {
						SColor clr;
						if(r_tag_filt.SelectIndicator(static_cast<const Goods_ *>(pData)->ID, clr) > 0)
							ok = pCellStyle->SetLeftBottomCornerColor(static_cast<COLORREF>(clr));
					}
				}
			}
			else if(r_col.OrgOffs == 7) { // barcode
				PROFILE_START
				SString barcode;
				p_def->getFullText(pData, col, barcode);
				if(barcode.NotEmptyS()) {
					if(barcode.Len() == 3)
						ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrOrange));
					else if(barcode.Len() == 19)
						ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrLightblue));
					else {
						int    diag = 0, std = 0;
						int    r = PPObjGoods::DiagBarcode(barcode, &diag, &std, 0);
						if(r > 0) {
							//pCellStyle->Flags |= BrowserWindow::CellStyle::fCorner;
							//pCellStyle->Color = GetColorRef(SClrGreen);
							//ok = 1;
						}
						else if(r < 0)
							ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrYellow));
						else
							ok = pCellStyle->SetLeftTopCornerColor(GetColorRef(SClrRed));
					}
				}
				PROFILE_END
			}
		}
	}
	return ok;
}

void SLAPI PPViewGoods::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(!GObj.CheckFlag(Filt.GrpID, GF_DYNAMICALTGRP) && PPObjGoodsGroup::IsAlt(Filt.GrpID) > 0 && !(Filt.Flags & GoodsFilt::fNegation)) { // @v8.3.12 && !(Filt.Flags & GoodsFilt::fNegation)
		// @v10.6.4 pBrw->InsColumnWord(-1, PPWORD_PLU, 17, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0);
		pBrw->InsColumn(-1, "@plu", 17, 0, MKSFMTD(0, 0, NMBF_NOZERO), 0); // @v10.6.4
	}
	/*
	if(Filt.Flags & GoodsFilt::fShowStrucType && Filt.Flags & GoodsFilt::fShowBarcode)
		// @v9.0.2 pBrw->InsColumnWord(-1, PPWORD_BARCODE, 6, 0, 0, 0);
		pBrw->InsColumn(-1, "@barcode", 6, 0, 0, 0); // @v9.0.2
	*/
	if(Filt.Flags & GoodsFilt::fShowBarcode) {
		// @v9.0.2 pBrw->InsColumnWord(-1, PPWORD_BARCODE, 7, 0, 0, 0);
		pBrw->InsColumn(-1, "@barcode", 7, 0, 0, 0); // @v9.0.2
	}
	if(!(Filt.Flags & GoodsFilt::fShowStrucType) && Filt.Flags & GoodsFilt::fShowCargo) {
		pBrw->insertColumn(-1, "@cargobrutto",  10, 0, ALIGN_RIGHT|NMBF_NOZERO, 0);
		pBrw->insertColumn(-1, "@cargodim",    11, 0, ALIGN_RIGHT|NMBF_NOZERO, 0);
		pBrw->insertColumn(-1, "@stockths",    12, 0, ALIGN_RIGHT|NMBF_NOZERO, 0);
		pBrw->insertColumn(-1, "@cargopckg",   13, 0, ALIGN_RIGHT|NMBF_NOZERO, 0);
	}
	if(Filt.Flags & GoodsFilt::fShowArCode) {
		// @v9.0.2 pBrw->InsColumnWord(-1, PPWORD_AR,   8, 0, 0, 0);
		// @v9.0.2 pBrw->InsColumnWord(-1, PPWORD_CODE, 9, 0, 0, 0);
		pBrw->InsColumn(-1, "@article", 8, 0, 0, 0); // @v9.0.2
		pBrw->InsColumn(-1, "@code", 9, 0, 0, 0); // @v9.0.2
	}
	CALLPTRMEMB(pBrw, SetCellStyleFunc(CellStyleFunc, /*this*/pBrw)); // @v9.5.5 this-->pBrw
}

static void FASTCALL SetGsChr(SString & rS, char c)
{
	if(!rS.HasChr(c))
		rS.CatChar(c);
}

static int FASTCALL SetGoodsStrucSymb(PPGoodsStrucHeader & rGsRec, SString & rBuf)
{
	int    r = -1;
	SetGsChr(rBuf, 'E');
	if(rGsRec.Flags & GSF_COMPL)
		SetGsChr(rBuf, 'C');
	if(rGsRec.Flags & GSF_DECOMPL)
		SetGsChr(rBuf, 'D');
	if(rGsRec.Flags & GSF_PARTITIAL)
		SetGsChr(rBuf, 'P');
	if(rGsRec.Flags & GSF_SUBST)
		SetGsChr(rBuf, 'S');
	if(rGsRec.Flags & GSF_PRESENT)
		SetGsChr(rBuf, 'G');
	/*
	if(gs_rec.ParentID)
		rBuf.CatChar('F');
	*/
	if(rGsRec.Flags & GSF_FOLDER) {
		SetGsChr(rBuf, 'F');
		r = 1;
	}
	return r;
}

static int GetStrucTypeString(PPID strucID, SString & rBuf)
{
	int    ok = -1;
	if(strucID) {
		PPObjGoodsStruc gs_obj;
		PPGoodsStrucHeader gs_rec;
		if(gs_obj.Fetch(strucID, &gs_rec) > 0) {
			if(SetGoodsStrucSymb(gs_rec, rBuf) > 0) {
				PPIDArray child_list;
				gs_obj.GetChildIDList(strucID, &child_list);
				for(uint i = 0; i < child_list.getCount(); i++)
					GetStrucTypeString(child_list.get(i), rBuf); // @recursion
			}
			ok = 1;
		}
	}
	return ok;
}

static IMPL_DBE_PROC(dbqf_goodsstructype_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init(sizeof(buf));
	}
	else {
		SString temp_buf;
		GetStrucTypeString(params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

int PPViewGoods::DynFuncStrucType = 0; // static

DBQuery * SLAPI PPViewGoods::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	DbqFuncTab::RegisterDyn(&DynFuncStrucType, BTS_STRING, dbqf_goodsstructype_i, 1, BTS_INT);

	DBQuery * q = 0;
	DBE    dbe_unit;
	DBE    dbe_phunit;
	DBE    dbe_manuf;
	DBE    dbe_code_ar;
	DBE    dbe_dim;
	DBE    dbe_brutto;
	DBE    dbe_min;
	DBE    dbe_pack;
	DBE    dbe_structype;
	DBE    dbe_brand;
	DBE    dbe_group;
	DBQ  * dbq = 0;
	ObjAssocTbl * oa = 0;
	Goods2Tbl   * g  = 0;
	BarcodeTbl  * p_bc_t = 0;
	ArGoodsCodeTbl * p_ac_t = 0;
	TempOrderTbl   * tmp_t  = 0;
	uint   brw_id = 0;
	PPID   grp_id = Filt.GrpID;
	int    alt = 0, gen = 0, dyn = GObj.CheckFlag(grp_id, GF_DYNAMICALTGRP);
	if(grp_id == 0)
		alt = 0;
	else if(grp_id < 0)
		alt = 1;
	else if(Filt.Flags & GoodsFilt::fGenGoods)
		gen = 1;
	else if((alt = PPObjGoodsGroup::IsAlt(grp_id)) < 0) {
		grp_id = 0;
		alt = 0;
	}
	THROW(CheckTblPtr(g  = new Goods2Tbl));
	PPDbqFuncPool::InitObjNameFunc(dbe_unit,   PPDbqFuncPool::IdObjNameUnit,       g->UnitID);
	PPDbqFuncPool::InitObjNameFunc(dbe_phunit, PPDbqFuncPool::IdObjNameUnit,       g->PhUnitID);
	PPDbqFuncPool::InitObjNameFunc(dbe_manuf,  PPDbqFuncPool::IdObjNamePerson,     g->ManufID);
	PPDbqFuncPool::InitLongFunc(dbe_dim,       PPDbqFuncPool::IdGoodsStockDim,     g->ID);
	PPDbqFuncPool::InitLongFunc(dbe_brutto,    PPDbqFuncPool::IdGoodsStockBrutto,  g->ID);
	PPDbqFuncPool::InitLongFunc(dbe_min,       PPDbqFuncPool::IdGoodsStockMin,     g->ID);
	PPDbqFuncPool::InitLongFunc(dbe_pack,      PPDbqFuncPool::IdGoodsStockPackage, g->ID);
	PPDbqFuncPool::InitLongFunc(dbe_structype, PPViewGoods::DynFuncStrucType,      g->StrucID);
	PPDbqFuncPool::InitLongFunc(dbe_brand,     PPDbqFuncPool::IdObjNameBrand,      g->BrandID);
	PPDbqFuncPool::InitLongFunc(dbe_group,     PPDbqFuncPool::IdObjNameGoods,      g->ParentID);
	if(P_TempTbl)
		THROW(CheckTblPtr(tmp_t = new TempOrderTbl(P_TempTbl->GetName())));
	q = & select(g->ID, 0L);                           // #00
	q->addField(g->Flags);                             // #01
	q->addField(g->Name);                              // #02
	q->addField(dbe_manuf);                            // #03
	q->addField(dbe_unit);                             // #04
	q->addField(dbe_phunit);                           // #05
	q->addField(g->PhUPerU);                           // #06
	if(Filt.Flags & GoodsFilt::fShowBarcode) {
		THROW(CheckTblPtr(p_bc_t = new BarcodeTbl));
		q->addField(p_bc_t->Code);                     // #07
	}
	else
		q->addField(g->ID); // stub                    // #07
	if(Filt.Flags & GoodsFilt::fShowArCode) {
		THROW(CheckTblPtr(p_ac_t = new ArGoodsCodeTbl));
		PPDbqFuncPool::InitObjNameFunc(dbe_code_ar, PPDbqFuncPool::IdObjNameAr, p_ac_t->ArID);
		q->addField(dbe_code_ar);                      // #08
		q->addField(p_ac_t->Code);                     // #09
	}
	else {
		q->addField(g->ID); // stub                    // #08
		q->addField(g->ID); // stub                    // #09
	}
	q->addField(dbe_brutto);                           // #10
	q->addField(dbe_dim);                              // #11
	q->addField(dbe_min);                              // #12
	q->addField(dbe_pack);                             // #13
	q->addField(dbe_structype);                        // #14
	q->addField(dbe_brand);                            // #15
	q->addField(dbe_group);                            // #16

	if(Filt.Flags & GoodsFilt::fShowStrucType)
		brw_id = BROWSER_GOODSWITHSTRUC;
	else
		brw_id = BROWSER_GOODS;
	/*
	if(Filt.Flags & GoodsFilt::fShowCargo)
		brw_id = (Filt.Flags & GoodsFilt::fShowBarcode) ? BROWSER_GOODSCARGOBARCODE : BROWSER_GOODSCARGO;
	else if(Filt.Flags & GoodsFilt::fShowStrucType)
		brw_id = BROWSER_GOODSWITHSTRUC;
	else
		brw_id = (Filt.Flags & GoodsFilt::fShowBarcode) ? BROWSER_GOODSBARCODE : BROWSER_GOODS;
	*/
	if(tmp_t) {
		dbq = & (g->ID == tmp_t->ID);
		if(p_bc_t)
			dbq = & (*dbq && ((g->ID == p_bc_t->GoodsID && p_bc_t->BarcodeType >= 0L)));
		if(p_ac_t) {
			if(Filt.CodeArID || Filt.Flags & GoodsFilt::fShowOwnArCode) {
				if(Filt.Flags & GoodsFilt::fShowWoArCode)
					dbq = & (*dbq && (g->ID += p_ac_t->GoodsID) && p_ac_t->ArID == Filt.CodeArID);
				else
					dbq = & (*dbq && (g->ID == p_ac_t->GoodsID) && p_ac_t->ArID == Filt.CodeArID);
			}
			else {
				if(Filt.Flags & GoodsFilt::fShowWoArCode)
					dbq = & (*dbq && (p_ac_t->GoodsID += g->ID));
				else
					dbq = & (*dbq && (p_ac_t->GoodsID == g->ID));
			}
		}
		if(!dyn && alt && !(Filt.Flags & GoodsFilt::fNegation)) {
			THROW(CheckTblPtr(oa = new ObjAssocTbl));
			if(alt)
				q->addField(oa->InnerNum);             // #17
			dbq = & (*dbq && (tmp_t->ID == oa->ScndObjID &&
				oa->PrmrObjID == grp_id && oa->AsscType == (alt ? PPASS_ALTGOODSGRP : PPASS_GENGOODS)));
		}
		if(Filt.Flags & GoodsFilt::fShowStrucType && !(Filt.Flags & GoodsFilt::fShowGoodsWOStruc))
			dbq = &(*dbq && g->StrucID > 0L);
		q->addTable(tmp_t);
		q->addTable(g);
		q->addTable(p_bc_t);
		q->addTable(p_ac_t);
		q->addTable(oa);
		q->where(*dbq).orderBy(tmp_t->Name, 0L);
	}
	else {
		if(alt || gen) {
			THROW(CheckTblPtr(oa = new ObjAssocTbl));
			if(alt)
				q->addField(oa->InnerNum);             // #17
			dbq = & (oa->AsscType == (alt ? PPASS_ALTGOODSGRP : PPASS_GENGOODS) && (oa->PrmrObjID == grp_id && g->ID == oa->ScndObjID));
		}
		else {
			dbq = & (g->Kind == PPGDSK_GOODS);
			dbq = ppcheckfiltid(dbq, g->ParentID, grp_id);
		}
		q->addTable(oa);
		q->addTable(g);
		q->addTable(p_bc_t);
		q->addTable(p_ac_t);
		if(grp_id >= 0) {
			dbq = ppcheckfiltid(dbq, g->UnitID, Filt.UnitID);
			dbq = ppcheckfiltid(dbq, g->PhUnitID, Filt.PhUnitID);
			if(Filt.Flags & GoodsFilt::fUndefType)
				Filt.GoodsTypeID = 0;
			if(Filt.Flags & GoodsFilt::fWoTaxGrp)
				Filt.TaxGrpID = 0;
			if(Filt.Flags & GoodsFilt::fUndefType || Filt.GoodsTypeID)
				dbq = & (*dbq && g->GoodsTypeID == Filt.GoodsTypeID);
			if(Filt.Flags & GoodsFilt::fWoTaxGrp || Filt.TaxGrpID)
				dbq = & (*dbq && g->TaxGrpID == Filt.TaxGrpID);
		}
		dbq = ppcheckflag(dbq, g->Flags, GF_PASSIV,  (Filt.Flags & GoodsFilt::fHidePassive) ? -1 : BIN(Filt.Flags & GoodsFilt::fPassiveOnly));
		dbq = ppcheckflag(dbq, g->Flags, GF_GENERIC, (Filt.Flags & GoodsFilt::fHideGeneric) ? -1 : BIN(Filt.Flags & GoodsFilt::fGenGoodsOnly)); // @v10.7.7 
		// @v10.7.7 dbq = ppcheckflag(dbq, g->Flags, GF_GENERIC,      BIN(Filt.Flags & GoodsFilt::fGenGoodsOnly));
		dbq = ppcheckflag(dbq, g->Flags, GF_PRICEWOTAXES, BIN(Filt.Flags & GoodsFilt::fWOTaxGdsOnly));
		dbq = ppcheckflag(dbq, g->Flags, GF_NODISCOUNT,   BIN(Filt.Flags & GoodsFilt::fNoDisOnly));
		dbq = ppcheckflag(dbq, g->Flags, GF_HASIMAGES,    BIN(Filt.Flags & GoodsFilt::fHasImages));
		dbq = ppcheckflag(dbq, g->Flags, GF_USEINDEPWT,   BIN(Filt.Flags & GoodsFilt::fUseIndepWtOnly));
		// @v10.6.8 {
		if(Filt.Flags & GoodsFilt::fWoBrand)
			dbq = &(*dbq && g->BrandID == 0L);
		// } @v10.6.8 
		if(Filt.ManufID && grp_id >= 0)
			dbq = & (*dbq && g->ManufID == Filt.ManufID);
		if(p_bc_t)
			dbq = & (*dbq && ((g->ID == p_bc_t->GoodsID && p_bc_t->BarcodeType >= 0L)));
		if(p_ac_t) {
			if(Filt.CodeArID || Filt.Flags & GoodsFilt::fShowOwnArCode) {
				if(Filt.Flags & GoodsFilt::fShowWoArCode)
					dbq = & (*dbq && (g->ID += p_ac_t->GoodsID) && p_ac_t->ArID == Filt.CodeArID);
				else
					dbq = & (*dbq && (g->ID == p_ac_t->GoodsID) && p_ac_t->ArID == Filt.CodeArID);
			}
			else {
				if(Filt.Flags & GoodsFilt::fShowWoArCode)
					dbq = & (*dbq && (p_ac_t->GoodsID += g->ID));
				else
					dbq = & (*dbq && (p_ac_t->GoodsID == g->ID));
			}
		}
		if(Filt.GoodsStrucID) {
			dbq = &(*dbq && g->StrucID == Filt.GoodsStrucID);
		}
		else if(Filt.Flags & GoodsFilt::fShowStrucType && !(Filt.Flags & GoodsFilt::fShowGoodsWOStruc))
			dbq = &(*dbq && g->StrucID > 0L);
		q->where(*dbq);
		if(Filt.GoodsStrucID)
			q->orderBy(g->StrucID, 0L);
		else if(Filt.GrpID)
			q->orderBy(g->Kind, g->ParentID, g->Name, 0L);
		else
			q->orderBy(g->Kind, g->Name, 0L);
	}
	q->options |= DBQuery::correct_search_more_problem;
	THROW(CheckQueryPtr(q));
	if(pSubTitle) {
		*pSubTitle = 0;
		SString temp_buf;
		if(Filt.Ep.GdsClsID) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(Filt.Ep.GdsClsID, &gc_pack) > 0) {
				PPLoadString("class", temp_buf);
				pSubTitle->Cat(temp_buf).CatDiv(':', 2).Cat(gc_pack.Rec.Name);
			}
		}
		if(pSubTitle->Empty()) {
			if(Filt.GrpID)
				GetGoodsName(Filt.GrpID, *pSubTitle);
			else if(Filt.GrpIDList.GetCount()) {
				for(uint i = 0; i < Filt.GrpIDList.GetCount(); i++) {
					if(GetGoodsNameR(Filt.GrpIDList.Get().get(i), temp_buf) > 0)
						pSubTitle->CatDivIfNotEmpty(';', 2).Cat(temp_buf);
					if(pSubTitle->Len() > 64 && i < Filt.GrpIDList.GetCount()-1) {
						pSubTitle->CatCharN('.', 2);
						break;
					}
				}
			}
		}
	}
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete g;
			delete p_bc_t;
			delete p_ac_t;
			delete oa;
			delete tmp_t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempCargoFile, TempGoodsCargo);

void SLAPI PPViewGoods::MakeTempRec(const Goods2Tbl::Rec * pGoodsRec, TempOrderTbl::Rec * pOrdRec)
{
	if(pOrdRec) {
		const size_t max_prefix_len = 48;
		Goods2Tbl::Rec temp_rec;
		IterOrder ord = static_cast<IterOrder>(Filt.InitOrder);
		SString buf;
		SString temp_buf;
		SString last_alphabet_symb;
		memzero(pOrdRec, sizeof(*pOrdRec));
		pOrdRec->ID = pGoodsRec->ID;
		if(oneof2(ord, OrdByBarcode, OrdByBarcode_Name)) {
			GObj.FetchSingleBarcode(pGoodsRec->ID, temp_buf);
			buf.Printf("%032s", temp_buf.cptr());
			if(ord == OrdByBarcode_Name)
				buf.Cat(pGoodsRec->Name);
		}
		else if(ord == OrdByBrand_Name) {
			if(GObj.Fetch(pGoodsRec->BrandID, &temp_rec) > 0)
				temp_buf = temp_rec.Name;
			else
				temp_buf.Z();
			const size_t len = temp_buf.Trim(max_prefix_len).Len();
			if(len == 0) {
				last_alphabet_symb.Z().CatChar('я').Transf(CTRANSF_UTF8_TO_INNER);
				temp_buf.CatCharN(last_alphabet_symb.C(0), max_prefix_len);
			}
			else {
				const size_t tail = (len >= max_prefix_len) ? 0 : max_prefix_len - len;
				temp_buf.CatCharN((char)0, tail);
			}
			(buf = temp_buf).Cat(pGoodsRec->Name);
		}
		else if(ord == OrdByAbbr) {
			buf = pGoodsRec->Abbr;
		}
		else if(oneof2(ord, OrdByGrp_Name, OrdByGrp_Abbr)) {
			if(GObj.Fetch(pGoodsRec->ParentID, &temp_rec) > 0)
				temp_buf = temp_rec.Name;
			else
				temp_buf.Z();
			const size_t len = temp_buf.Trim(max_prefix_len).Len();
			if(len == 0) {
				last_alphabet_symb.Z().CatChar('я').Transf(CTRANSF_UTF8_TO_INNER);
				temp_buf.CatCharN(last_alphabet_symb.C(0), max_prefix_len);
			}
			else {
				const size_t tail = (len >= max_prefix_len) ? 0 : max_prefix_len - len;
				temp_buf.CatCharN((char)0, tail);
			}
			(buf = temp_buf).Cat((ord == OrdByGrp_Abbr) ? pGoodsRec->Abbr : pGoodsRec->Name);
		}
		else
			buf = pGoodsRec->Name;
		buf.CopyTo(pOrdRec->Name, sizeof(pOrdRec->Name));
	}
}

int SLAPI PPViewGoods::UpdateTempTable(PPID goodsID, PPViewBrowser * pBrw)
{
	int    ok = 1;
	if(P_TempTbl) {
		if(goodsID) {
			Goods2Tbl::Rec goods_rec;
			if(GObj.Search(goodsID, &goods_rec) > 0 && GObj.CheckForFilt(&Filt, goodsID, 0)) {
				TempOrderTbl::Rec temp_rec;
				MakeTempRec(&goods_rec, &temp_rec);
				TempOrderTbl::Key0 k0;
				MEMSZERO(k0);
				k0.ID = goodsID;
				if(P_TempTbl->search(0, &k0, spEq)) {
					THROW_DB(P_TempTbl->updateRecBuf(&temp_rec));
				}
				else {
					THROW_DB(P_TempTbl->insertRecBuf(&temp_rec));
				}
			}
			else {
				THROW_DB(deleteFrom(P_TempTbl, 0, P_TempTbl->ID == goodsID));
			}
		}
		else if(pBrw)
			ok = ChangeFilt(1, pBrw);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewGoods::IsTempTblNeeded()
{
	if(Filt.GrpIDList.IsExists())
		return 1;
	else {
		SString temp_buf;
		Filt.GetExtssData(Filt.extssNameText, temp_buf.Z());
		if(temp_buf.NotEmptyS())
			return 1;
		else {
			Filt.GetExtssData(Filt.extssBarcodeText, temp_buf.Z());
			if(temp_buf.NotEmptyS())
				return 1;
			else {
				/* @v8.2.12 else if(Filt.SrchStr_.NotEmpty())
					return 1; */
				if(Filt.BarcodeLen.NotEmpty())
					return 1;
				if(Filt.ManufID) // @v10.6.8
					return 1;
				if(Filt.ManufCountryID)
					return 1;
				else if(!Filt.GetResultBrandList().IsEmpty())
					return 1;
				else if(!Filt.LotPeriod.IsZero())
					return 1;
				else if(Filt.SupplID || Filt.VatRate)
					return 1;
				else if(Filt.Flags & GoodsFilt::fNegation)
					return 1;
				else if(Filt.Ep.GdsClsID)
					return 1;
				else if(Filt.Flags & (GoodsFilt::fWoTaxGrp|GoodsFilt::fRestrictByMatrix|GoodsFilt::fOutOfMatrix|GoodsFilt::fActualOnly))
					return 1;
				else if(Filt.Flags & GoodsFilt::fShowArCode && !(Filt.Flags & GoodsFilt::fShowWoArCode) &&
					(Filt.CodeArID || Filt.Flags & GoodsFilt::fShowOwnArCode)) {
					return 1;
				}
				if(Filt.P_SjF && !Filt.P_SjF->IsEmpty())
					return 1;
				if(Filt.P_TagF && !Filt.P_TagF->IsEmpty())
					return 1;
				else if(Filt.GrpID) {
					Goods2Tbl::Rec grp_rec;
					if(GObj.Fetch(Filt.GrpID, &grp_rec) > 0)
						if(grp_rec.Flags & (GF_FOLDER|GF_GENERIC|GF_DYNAMICALTGRP))
							return 1;
				}
				else if(!oneof2(Filt.InitOrder, OrdByDefault, OrdByName))
					return 1;
			}
		}
	}
	return 0;
}

int SLAPI PPViewGoods::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	RemoveTempAltGroup();
	ZDELETE(P_Iter);
	BExtQuery::ZDelete(&P_IterQuery);
	ZDELETE(P_TempTbl);
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Setup();
	if(Filt.P_SjF)
		Filt.P_SjF->Period.Actualize(ZERODATE);
	THROW(CreateTempTable(static_cast<IterOrder>(Filt.InitOrder), &P_TempTbl));
	CATCH
		ZDELETE(P_TempTbl);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI PPViewGoods::InitIteration(int aOrder)
{
	int    ok  = 1;
	Counter.Init();
	ZDELETE(P_Iter);
	BExtQuery::ZDelete(&P_IterQuery);
	BrcIdx = 0;
	BarcodeAry.freeAll();
	MEMSZERO(IterCurItem);
	ZDELETE(P_TempTbl);
	CreateTempTable(static_cast<PPViewGoods::IterOrder>(aOrder), &P_TempTbl);
	if(P_TempTbl) {
		TempOrderTbl::Key1 k1;
		PPInitIterCounter(Counter, P_TempTbl);
		THROW_MEM(P_IterQuery = new BExtQuery(P_TempTbl, 1));
		P_IterQuery->select(P_TempTbl->ID, 0);
		MEMSZERO(k1);
		P_IterQuery->initIteration(0, &k1, spFirst);
	}
	else {
		int    iter_order;
		if(aOrder == OrdByDefault)
			iter_order = GoodsIterator::ordByDefault;
		else if(aOrder == OrdByName || aOrder == OrdByGrp_Name)
			iter_order = GoodsIterator::ordByName;
		else if(aOrder == OrdByAbbr || aOrder == OrdByGrp_Abbr)
			iter_order = GoodsIterator::ordByAbbr;
		THROW_MEM(P_Iter = new GoodsIterator(&Filt, iter_order));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPViewGoods::NextInnerIteration(int initList, GoodsViewItem * pItem)
{
	int    ok = -1;
	if(Filt.Flags & GoodsFilt::fShowBarcode) {
		if(initList) {
			BrcIdx = 0;
			GObj.ReadBarcodes(pItem->ID, BarcodeAry);
		}
		if(BrcIdx < BarcodeAry.getCount()) {
			STRNSCPY(pItem->Barcode, BarcodeAry.at(BrcIdx).Code);
			BrcIdx++;
			ok = 1;
		}
	}
	else if(initList && (Filt.Flags & GoodsFilt::fShowStrucType))
		ok = (pItem && (pItem->StrucID || Filt.Flags & GoodsFilt::fShowGoodsWOStruc)) ? 1 : -1;
	else if(initList)
		ok = 1;
	return ok;
}

int FASTCALL PPViewGoods::NextIteration(GoodsViewItem * pItem)
{
	int    ok = -1;
	if(NextInnerIteration(0, &IterCurItem) > 0) {
		ASSIGN_PTR(pItem, IterCurItem);
		ok = 1;
	}
	else {
		if(P_IterQuery) {
			while(ok < 0 && P_IterQuery->nextIteration() > 0) {
				PPID   goods_id = P_TempTbl->data.ID;
				MEMSZERO(IterCurItem);
				if(GObj.Search(goods_id, &IterCurItem) > 0) {
					if(Filt.Flags & GoodsFilt::fShowCargo) {
						GoodsStockExt gse;
						if(GObj.GetStockExt(goods_id, &gse, 1) > 0) {
							IterCurItem.Brutto   = gse.Brutto;
							IterCurItem.PckgDim  = gse.PckgDim;
							IterCurItem.RtlDim   = gse.RtlDim;
							IterCurItem.ExpiryPeriod = gse.ExpiryPeriod;
							IterCurItem.GseFlags = gse.GseFlags;
							IterCurItem.MinStock = gse.GetMinStock(0);
							IterCurItem.Package  = gse.Package;
							IterCurItem.MinShippmQtty = gse.MinShippmQtty;
						}
					}
					if(Filt.Flags & GoodsFilt::fShowStrucType) {
						SString temp_buf;
						GetStrucTypeString(IterCurItem.StrucID, temp_buf);
						temp_buf.CopyTo(IterCurItem.StrucType, sizeof(IterCurItem.StrucType));
					}
					if(NextInnerIteration(1, &IterCurItem) > 0) {
						ASSIGN_PTR(pItem, IterCurItem);
						ok = 1;
					}
				}
				Counter.Increment();
			}
		}
		else if(P_Iter) {
			while(ok < 0 && P_Iter->Next(&IterCurItem) > 0) {
				if(Filt.Flags & GoodsFilt::fShowCargo) {
					GoodsStockExt gse;
					if(GObj.GetStockExt(IterCurItem.ID, &gse, 1) > 0) {
						IterCurItem.Brutto   = gse.Brutto;
						IterCurItem.PckgDim  = gse.PckgDim;
						IterCurItem.RtlDim   = gse.RtlDim;
						IterCurItem.ExpiryPeriod = gse.ExpiryPeriod;
						IterCurItem.GseFlags = gse.GseFlags;
						IterCurItem.MinStock = gse.GetMinStock(0);
						IterCurItem.Package  = gse.Package;
						IterCurItem.MinShippmQtty = gse.MinShippmQtty;
					}
				}
				if(Filt.Flags & GoodsFilt::fShowStrucType) {
					SString temp_buf;
					GetStrucTypeString(IterCurItem.StrucID, temp_buf);
					temp_buf.CopyTo(IterCurItem.StrucType, sizeof(IterCurItem.StrucType));
				}
				if(NextInnerIteration(1, &IterCurItem) > 0) {
					ASSIGN_PTR(pItem, IterCurItem);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

const IterCounter & SLAPI PPViewGoods::GetCounter() const
{
	return P_Iter ? P_Iter->GetIterCounter() : Counter;
}

int SLAPI PPViewGoods::ViewTotal()
{
	TDialog * dlg = new TDialog(DLG_GOODSTOTAL);
	if(CheckDialogPtrErr(&dlg)) {
		long count = 0;
		GoodsViewItem item;
		PPWait(1);
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			count++;
		PPWait(0);
		dlg->setCtrlLong(CTL_GOODSTOTAL_COUNT, count);
		ExecViewAndDestroy(dlg);
		return -1;
	}
	else
		return 0;
}

int SLAPI PPViewGoods::DeleteItem(PPID id)
{
	int    ok = -1;
	if(id) {
		if(IsGenGoodsFlt()) {
			if(CONFIRM(PPCFM_DELETE)) {
				ok = GObj.RemoveGoodsFromGen(id, Filt.GrpID, 1);
				if(ok == 0)
					PPError();
			}
		}
		else if(IsAltFltGroup()) {
			if(!GObj.CheckFlag(Filt.GrpID, GF_DYNAMIC) && CONFIRM(PPCFM_DELETE)) {
				ok = PPRef->Assc.Remove(PPASS_ALTGOODSGRP, Filt.GrpID, id, 1);
				if(ok == 0)
					PPError();
			}
		}
		else {
			PPID grp_id = (Filt.GrpID > 0 && !(Filt.Flags & GoodsFilt::fGenGoods)) ? Filt.GrpID : 0;
			ok = GObj.RemoveObjV(id, 0, PPObject::rmv_default, reinterpret_cast<void *>(grp_id));
		}
		if(P_TempTbl && ok > 0) {
			ok = deleteFrom(P_TempTbl, 0, P_TempTbl->ID == id);
			if(ok == 0)
				PPError();
		}
	}
	return ok;
}
//
//
//
SLAPI PPViewGoods::GoodsMoveParam::GoodsMoveParam() : Action(aMoveToGroup), DestGrpID(0), Flags(0), ClsDimZeroFlags(0), RValue(0.0)
{
	// @v10.6.8 @ctr MEMSZERO(Clssfr);
}

int SLAPI PPViewGoods::RemoveAll()
{
	class GoodsMoveDialog : public TDialog {
		DECL_DIALOG_DATA(GoodsMoveParam);
	public:
		enum {
			ctlgroupLoc = 1
		};
		GoodsMoveDialog::GoodsMoveDialog() : TDialog(DLG_GOODSRMVALL)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 0, GoodsMoveParam::aMoveToGroup);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 1, GoodsMoveParam::aChgClssfr);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 2, GoodsMoveParam::aChgTaxGroup); // @v9.5.0
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 3, GoodsMoveParam::aChgGoodsType); // @v9.8.12
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 4, GoodsMoveParam::aRemoveAll);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 5, GoodsMoveParam::aActionByFlags);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 6, GoodsMoveParam::aActionByRule);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 7, GoodsMoveParam::aChgMinStock);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 8, GoodsMoveParam::aSplitBarcodeItems);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 9, GoodsMoveParam::aMergeDiezNames);
			AddClusterAssoc(CTL_REMOVEALL_WHAT, 10, GoodsMoveParam::aAssignCodeByTemplate); // @v10.7.6
			SetClusterData(CTL_REMOVEALL_WHAT, Data.Action);
			AddClusterAssoc(CTL_REMOVEALL_FLAGS, 0, GoodsMoveParam::fRemoveExtTextA);
			AddClusterAssoc(CTL_REMOVEALL_FLAGS, 1, GoodsMoveParam::fRemoveExtTextB);
			AddClusterAssoc(CTL_REMOVEALL_FLAGS, 2, GoodsMoveParam::fRemoveExtTextC);
			AddClusterAssoc(CTL_REMOVEALL_FLAGS, 3, GoodsMoveParam::fRemoveExtTextD);
			AddClusterAssoc(CTL_REMOVEALL_FLAGS, 4, GoodsMoveParam::fRemoveExtTextE);
			SetClusterData(CTL_REMOVEALL_FLAGS, Data.Flags);
			SetupExtCombo();
			if(Data.Action == GoodsMoveParam::aChgMinStock) {
				Data.Rule.Z().Cat(Data.RValue);
			}
			else
				setCtrlString(CTL_REMOVEALL_RULE, Data.Rule);
			disableCtrl(CTL_REMOVEALL_FLAGS, Data.Action != GoodsMoveParam::aActionByFlags);
			enableCommand(cmGoodsChgClssfr, Data.Action == GoodsMoveParam::aChgClssfr);
			DisableClusterItem(CTL_REMOVEALL_WHAT, 0, !(Data.Flags & GoodsMoveParam::fMassOpAllowed));
			DisableClusterItem(CTL_REMOVEALL_WHAT, 1, !(Data.Flags & GoodsMoveParam::fMassOpAllowed));
			DisableClusterItem(CTL_REMOVEALL_WHAT, 5, !(Data.Flags & GoodsMoveParam::fMassOpAllowed));
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			GetClusterData(CTL_REMOVEALL_WHAT, &Data.Action);
			THROW_PP(oneof11(Data.Action, GoodsMoveParam::aMoveToGroup, GoodsMoveParam::aChgClssfr, GoodsMoveParam::aRemoveAll,
				GoodsMoveParam::aActionByFlags, GoodsMoveParam::aActionByRule, GoodsMoveParam::aChgMinStock,
				GoodsMoveParam::aSplitBarcodeItems, GoodsMoveParam::aMergeDiezNames, GoodsMoveParam::aChgTaxGroup,
				GoodsMoveParam::aChgGoodsType, GoodsMoveParam::aAssignCodeByTemplate), PPERR_USERINPUT);
			GetClusterData(CTL_REMOVEALL_FLAGS, &Data.Flags);
			getCtrlString(CTL_REMOVEALL_RULE, Data.Rule);
			if(Data.Action == GoodsMoveParam::aChgClssfr) {
				sel = CTL_REMOVEALL_GRP;
				THROW_PP(Data.Clssfr.GoodsClsID, PPERR_GDSCLSNEEDED);
			}
			else if(Data.Action == GoodsMoveParam::aActionByFlags) {
				sel = CTL_REMOVEALL_FLAGS;
				THROW_PP(Data.Flags & (GoodsMoveParam::fRemoveExtTextA|GoodsMoveParam::fRemoveExtTextB), PPERR_USERINPUT);
			}
			else if(Data.Action == GoodsMoveParam::aActionByRule) {
				sel = CTL_REMOVEALL_RULE;
				THROW_PP(Data.Rule.NotEmptyS(), PPERR_RULENEEDED);
			}
			else if(Data.Action == GoodsMoveParam::aMoveToGroup) {
				getCtrlData(sel = CTLSEL_REMOVEALL_GRP, &Data.DestGrpID);
				THROW_PP(Data.DestGrpID, PPERR_GOODSGROUPNEEDED);
			}
			// @v9.5.0 {
			else if(Data.Action == GoodsMoveParam::aChgTaxGroup) {
				getCtrlData(sel = CTLSEL_REMOVEALL_GRP, &Data.DestGrpID);
				THROW_PP(Data.DestGrpID, PPERR_TAXGROUPNEEDED);
			}
			// } @v9.5.0
			// @v9.8.12 {
			else if(Data.Action == GoodsMoveParam::aChgGoodsType) {
				getCtrlData(sel = CTLSEL_REMOVEALL_GRP, &Data.DestGrpID);
				THROW_PP(Data.DestGrpID, PPERR_GOODSTYPENEEDED);
			}
			// } @v9.8.12
			else if(Data.Action == GoodsMoveParam::aChgMinStock) {
				LocationCtrlGroup::Rec loc_rec;
				getGroupData(ctlgroupLoc, &loc_rec);
				Data.LocList = loc_rec.LocList;
				if(Data.LocList.IsExists() && Data.LocList.IsEmpty())
					Data.LocList.Set(0);
				sel = CTL_REMOVEALL_RULE;
                Data.RValue = Data.Rule.ToReal();
                THROW_PP_S(Data.RValue >= 0.0, PPERR_INVMINSTOCKINPUT, Data.Rule);
                if(Data.RValue == 0.0 && Data.Rule.NotEmptyS()) {
                	SStrScan scan(Data.Rule);
					scan.Skip();
					THROW_PP_S(scan.IsNumber(), PPERR_INVMINSTOCKINPUT, Data.Rule);
                }
			}
			// @v10.7.6 {
			else if(Data.Action == GoodsMoveParam::aAssignCodeByTemplate) {
				getCtrlData(sel = CTLSEL_REMOVEALL_GRP, &Data.DestGrpID);
				THROW_PP(Data.DestGrpID, PPERR_BARCODESTRUCNEEDED);
			}
			// } @v10.7.6 
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_REMOVEALL_WHAT)) {
				GetClusterData(CTL_REMOVEALL_WHAT, &Data.Action);
				SetupExtCombo();
				disableCtrl(CTL_REMOVEALL_FLAGS, Data.Action != GoodsMoveParam::aActionByFlags);
				enableCommand(cmGoodsChgClssfr, Data.Action == GoodsMoveParam::aChgClssfr);
			}
			else if(event.isCmd(cmGoodsChgClssfr)) {
				PPObjGoods goods_obj;
				PPGoodsPacket temp_pack;
				temp_pack.ExtRec = Data.Clssfr;
				if(goods_obj.EditClsdGoods(&temp_pack, 2) > 0) {
					Data.Clssfr = temp_pack.ExtRec;
					Data.ClsDimZeroFlags = temp_pack.ClsDimZeroFlags;
				}
			}
			else
				return;
			clearEvent(event);
		}
		void    SetupExtCombo()
		{
			SString rule_label_buf;
			const char * p_title_meta = 0;
			int    disable_grp_combo = 0;
			if(Data.Action == GoodsMoveParam::aChgMinStock) {
				showButton(cmLocList, 1);
				//disableCtrl(CTLSEL_REMOVEALL_GRP, 0);
				p_title_meta = "warehouse";
				PPLoadString("minrest", rule_label_buf);
				addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_REMOVEALL_GRP, 0, 0, cmLocList, 0, 0, 0));
				LocationCtrlGroup::Rec loc_rec(&Data.LocList);
				setGroupData(ctlgroupLoc, &loc_rec);
			}
			else {
				PPLoadString("rule", rule_label_buf);
				addGroup(ctlgroupLoc, 0);
				showButton(cmLocList, 0);
				if(Data.Action == GoodsMoveParam::aMoveToGroup) {
					p_title_meta = "goodsgroup";
					SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_GOODSGROUP, Data.DestGrpID, 0, reinterpret_cast<void *>(GGRTYP_SEL_NORMAL));
				}
				// @v9.5.0 {
				else if(Data.Action == GoodsMoveParam::aChgTaxGroup) {
					p_title_meta = "taxgroup";
					SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_GOODSTAX, Data.DestGrpID, 0, 0);
				}
				// } @v9.5.0
				// @v9.8.12 {
				else if(Data.Action == GoodsMoveParam::aChgGoodsType) {
					p_title_meta = "goods_type";
					SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_GOODSTYPE, Data.DestGrpID, 0, 0);
				}
				// } @v9.8.12
				// @v10.7.6 {
				else if(Data.Action == GoodsMoveParam::aAssignCodeByTemplate) {
					p_title_meta = "barcodestruc";
					SetupPPObjCombo(this, CTLSEL_REMOVEALL_GRP, PPOBJ_BCODESTRUC, Data.DestGrpID, 0, 0);
				}
				else 
					disable_grp_combo = 1;
				// } @v10.7.6 
			}
			disableCtrl(CTLSEL_REMOVEALL_GRP, disable_grp_combo);
			if(p_title_meta) {
				SString combo_label_buf;
				PPLoadString(p_title_meta, combo_label_buf);
				setLabelText(CTL_REMOVEALL_GRP, combo_label_buf);
			}
			setLabelText(CTL_REMOVEALL_RULE, rule_label_buf);
			disableCtrl(CTL_REMOVEALL_RULE, !oneof2(Data.Action, GoodsMoveParam::aActionByRule, GoodsMoveParam::aChgMinStock));
		}
	};
	const  size_t max_nm_len = sizeof(static_cast<const Goods2Tbl::Rec *>(0)->Name)-1;
	int    ok = -1;
	GoodsMoveDialog * dlg = 0;
	int    valid_data = 0;
	SString temp_buf;
	if(!(GmParam.Flags & GmParam.fInit)) {
		GmParam.Action = GoodsMoveParam::aMoveToGroup;
		GmParam.DestGrpID = 0;
		SETFLAG(GmParam.Flags, GoodsMoveParam::fAltGroup, IsAltFltGroup());
		SETFLAG(GmParam.Flags, GoodsMoveParam::fMassOpAllowed, GObj.CheckRights(GOODSRT_MULTUPD));
		THROW((GmParam.Flags & GoodsMoveParam::fMassOpAllowed) || (GmParam.Flags & GoodsMoveParam::fAltGroup));
		GmParam.Flags |= GmParam.fInit;
	}
	THROW(CheckDialogPtr(&(dlg = new GoodsMoveDialog)));
	dlg->setDTS(&GmParam);
	while(!valid_data && ExecView(dlg) == cmOK)
		if(dlg->getDTS(&GmParam))
			valid_data = 1;
	if(valid_data == 1) {
		long   success_count = 0, skip_count = 0;
		PPWait(1);
		GoodsViewItem item;
		PPGoodsPacket pack;
		PPIDArray id_list;
		PPLogger logger;
		const long _flags = GmParam.Flags;
		if(oneof4(GmParam.Action, GoodsMoveParam::aMoveToGroup, GoodsMoveParam::aChgTaxGroup, GoodsMoveParam::aChgGoodsType, GoodsMoveParam::aChgMinStock)) {
			for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
				if(GObj.GetPacket(item.ID, &pack, 0) > 0) {
					if(GmParam.Action == GoodsMoveParam::aMoveToGroup)
						pack.Rec.ParentID = GmParam.DestGrpID;
					else if(GmParam.Action == GoodsMoveParam::aChgTaxGroup)
						pack.Rec.TaxGrpID = GmParam.DestGrpID;
					else if(GmParam.Action == GoodsMoveParam::aChgGoodsType)
						pack.Rec.GoodsTypeID = GmParam.DestGrpID;
					else if(GmParam.Action == GoodsMoveParam::aChgMinStock) {
						const uint loc_count = GmParam.LocList.GetCount();
						if(loc_count) {
							for(uint i = 0; i < loc_count; i++)
								pack.Stock.SetMinStock(GmParam.LocList.Get(i), GmParam.RValue);
						}
						else
							pack.Stock.SetMinStock(0, GmParam.RValue);
					}
					else {
						assert(0); // something went wrong!
					}
					if(GObj.PutPacket(&item.ID, &pack, 1))
						success_count++;
					else {
						logger.LogLastError();
						skip_count++;
					}
				}
				PPWaitPercent(GetCounter());
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aActionByRule) {
			// A=ABC
			SString dest_fld, src_fld_set;
			int    rule_is_ok = 0;
			int    dest_fld_id = 0;
			LongArray src_fld_id_list;
            if(GmParam.Rule.Divide('=', dest_fld, src_fld_set) > 0) {
            	dest_fld.Strip().ToUpper();
				if(dest_fld.Len() == 1) {
					switch(dest_fld.C(0)) {
						case 'A': dest_fld_id = GDSEXSTR_A; break;
						case 'B': dest_fld_id = GDSEXSTR_B; break;
						case 'C': dest_fld_id = GDSEXSTR_C; break;
						case 'D': dest_fld_id = GDSEXSTR_D; break;
						case 'E': dest_fld_id = GDSEXSTR_E; break;
					}
					if(dest_fld_id > 0) {
						src_fld_set.Strip().ToUpper();
						if(src_fld_set.Len()) {
							rule_is_ok = 1;
							for(uint i = 0; rule_is_ok && i < src_fld_set.Len(); i++) {
								switch(src_fld_set.C(i)) {
									case 'A': src_fld_id_list.addUnique(GDSEXSTR_A); break;
									case 'B': src_fld_id_list.addUnique(GDSEXSTR_B); break;
									case 'C': src_fld_id_list.addUnique(GDSEXSTR_C); break;
									case 'D': src_fld_id_list.addUnique(GDSEXSTR_D); break;
									case 'E': src_fld_id_list.addUnique(GDSEXSTR_E); break;
									default: rule_is_ok = 0; break;
								}
							}
						}
					}
				}
            }
            if(rule_is_ok) {
				SString ext_fld_buf;
            	for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
					if(GObj.GetPacket(item.ID, &pack, 0) > 0) {
						ext_fld_buf.Z();
                        for(uint i = 0; i < src_fld_id_list.getCount(); i++) {
							const int src_fld_id = src_fld_id_list.get(i);
							pack.GetExtStrData(src_fld_id, temp_buf.Z());
							if(temp_buf.NotEmptyS()) {
								if(ext_fld_buf.NotEmpty())
									ext_fld_buf.CRB();
								ext_fld_buf.Cat(temp_buf);
							}
                        }
                        ext_fld_buf.Trim(4000-1);
                        pack.GetExtStrData(dest_fld_id, temp_buf.Z());
                        if(temp_buf != ext_fld_buf) {
                        	pack.PutExtStrData(dest_fld_id, ext_fld_buf);
							if(GObj.PutPacket(&item.ID, &pack, 1))
								success_count++;
							else {
								logger.LogLastError();
								skip_count++;
							}
                        }
					}
					PPWaitPercent(GetCounter());
            	}
            }
		}
		else if(GmParam.Action == GoodsMoveParam::aActionByFlags) {
			if(_flags & (GoodsMoveParam::fRemoveExtTextA|GoodsMoveParam::fRemoveExtTextB|GoodsMoveParam::fRemoveExtTextC|GoodsMoveParam::fRemoveExtTextD|GoodsMoveParam::fRemoveExtTextE)) {
				static const struct { int16 RmvF; int8 ExtF; } _rmv_f_ext_f_assoc_list[] = {
					{ GoodsMoveParam::fRemoveExtTextA, GDSEXSTR_A },
					{ GoodsMoveParam::fRemoveExtTextB, GDSEXSTR_B },
					{ GoodsMoveParam::fRemoveExtTextC, GDSEXSTR_C },
					{ GoodsMoveParam::fRemoveExtTextD, GDSEXSTR_D },
					{ GoodsMoveParam::fRemoveExtTextE, GDSEXSTR_E },
				};
				for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
					if(GObj.GetPacket(item.ID, &pack, 0) > 0) {
						int do_update = 0;
						for(uint i = 0; i < SIZEOFARRAY(_rmv_f_ext_f_assoc_list); i++) {
							const int ext_f = _rmv_f_ext_f_assoc_list[i].ExtF;
							if(_flags & _rmv_f_ext_f_assoc_list[i].RmvF && pack.GetExtStrData(ext_f, temp_buf) && temp_buf.NotEmpty()) {
								pack.PutExtStrData(ext_f, temp_buf.Z());
								do_update = 1;
							}
						}
						if(do_update) {
							if(GObj.PutPacket(&item.ID, &pack, 1))
								success_count++;
							else {
								logger.LogLastError();
								skip_count++;
							}
						}
						else
							skip_count++;
					}
					PPWaitPercent(GetCounter());
				}
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aChgClssfr) {
			if(GmParam.Clssfr.GoodsClsID) {
				IterCounter cntr;
				for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
					if(GmParam.Clssfr.GoodsClsID == item.GdsClsID || item.GdsClsID == 0)
						id_list.add(item.ID); // @v10.7.6 addUnique-->add
					else
						skip_count++;
					PPWaitPercent(GetCounter());
				}
				id_list.sortAndUndup(); // @v10.7.6
				cntr.Init(id_list.getCount());
				for(uint i = 0; i < id_list.getCount(); i++) {
					PPID goods_id = id_list.get(i);
					if(GObj.GetPacket(goods_id, &pack, 0) > 0) {
						int do_update = 0;
						GoodsExtTbl::Rec org_ext = pack.ExtRec;
						if(pack.ExtRec.GoodsClsID == GmParam.Clssfr.GoodsClsID || pack.ExtRec.GoodsClsID == 0) {
							pack.ExtRec.GoodsClsID = GmParam.Clssfr.GoodsClsID;
							pack.Rec.GdsClsID = GmParam.Clssfr.GoodsClsID;
							if(GmParam.Clssfr.KindID)    pack.ExtRec.KindID    = GmParam.Clssfr.KindID;
							if(GmParam.Clssfr.GradeID)   pack.ExtRec.GradeID   = GmParam.Clssfr.GradeID;
							if(GmParam.Clssfr.AddObjID)  pack.ExtRec.AddObjID  = GmParam.Clssfr.AddObjID;
							if(GmParam.Clssfr.AddObj2ID) pack.ExtRec.AddObj2ID = GmParam.Clssfr.AddObj2ID;
							if(GmParam.ClsDimZeroFlags & (1 << (PPGdsCls::eX-1)))
								pack.ExtRec.X = 0;
							else if(GmParam.Clssfr.X)
								pack.ExtRec.X = GmParam.Clssfr.X;
							if(GmParam.ClsDimZeroFlags & (1 << (PPGdsCls::eY-1)))
								pack.ExtRec.Y = 0;
							else if(GmParam.Clssfr.Y)
								pack.ExtRec.Y = GmParam.Clssfr.Y;
							if(GmParam.ClsDimZeroFlags & (1 << (PPGdsCls::eZ-1)))
								pack.ExtRec.Z = 0;
							else if(GmParam.Clssfr.Z)
								pack.ExtRec.Z = GmParam.Clssfr.Z;
							if(GmParam.ClsDimZeroFlags & (1 << (PPGdsCls::eW-1)))
								pack.ExtRec.W = 0;
							else if(GmParam.Clssfr.W)
								pack.ExtRec.W = GmParam.Clssfr.W;
						}
						if(memcmp(&pack.ExtRec, &org_ext, sizeof(org_ext)) != 0)
							if(GObj.PutPacket(&goods_id, &pack, 1))
								success_count++;
							else {
								logger.LogLastError();
								skip_count++;
							}
						else
							skip_count++;
					}
					else
						skip_count++;
					PPWaitPercent(cntr.Increment());
				}
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aRemoveAll) {
			PPID   assc_type = 0;
			PPID   assc_owner_id = 0;
			for(InitIteration(OrdByDefault); NextIteration(&item) > 0;)
				id_list.add(item.ID); // @v10.7.6 addUnique-->add
			if(id_list.getCount()) {
				id_list.sortAndUndup(); // @v10.7.6
				ObjCollection oc;
				oc.CreateFullList(gotlfExcludeDyn|gotlfExcludeObjBill|gotlfExcludeObsolete);
				PPTransaction tra(1);
				THROW(tra);
				if(Filt.GrpID > 0 && GObj.IsGeneric(Filt.GrpID)) {
					assc_type = PPASS_GENGOODS;
					assc_owner_id = Filt.GrpID;
				}
				else if(IsAltFltGroup()) {
					if(!GObj.CheckFlag(Filt.GrpID, GF_DYNAMIC)/* && CONFIRM(PPCFM_REMOVEALTGGRPMEMBS)*/) {
						assc_type = PPASS_ALTGOODSGRP;
						assc_owner_id = Filt.GrpID;
					}
				}
				else {
					for(uint i = 0; i < id_list.getCount(); i++) {
						PPID goods_id = id_list.get(i);
						if(GObj.RemoveObjV(goods_id, &oc, PPObject::not_addtolog|PPObject::no_wait_indicator, 0)) {
							if(P_TempTbl)
								deleteFrom(P_TempTbl, 0, P_TempTbl->ID == goods_id);
							success_count++;
						}
						else {
							logger.LogLastError();
							skip_count++;
						}
						PPWaitPercent(i+1, id_list.getCount());
					}
				}
				if(assc_type && assc_owner_id) {
					for(uint i = 0; i < id_list.getCount(); i++) {
						const PPID goods_id = id_list.get(i);
						THROW(PPRef->Assc.Remove(assc_type, assc_owner_id, goods_id, 0));
						if(P_TempTbl)
							deleteFrom(P_TempTbl, 0, P_TempTbl->ID == goods_id);
						success_count++;
					}
					if(assc_type == PPASS_ALTGOODSGRP && success_count) {
						DS.LogAction(PPACN_RMVALLGDSALTGRP, PPOBJ_GOODSGROUP, Filt.GrpID, 0, 0);
					}
				}
				THROW(tra.Commit());
				if(success_count)
					ok = 1;
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aAssignCodeByTemplate) { // @v10.7.6
			if(GmParam.DestGrpID) {
				PPObjBarCodeStruc bcs_obj;
				PPBarcodeStruc bcs_rec;
				if(bcs_obj.Search(GmParam.DestGrpID, &bcs_rec) > 0 && bcs_rec.Templ[0]) {
					IterCounter cntr;
					BarcodeArray current_code_list;
					for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
						id_list.add(item.ID);
						PPWaitPercent(GetCounter());
					}
					if(id_list.getCount()) {
						id_list.sortAndUndup();
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; i < id_list.getCount(); i++) {
							const PPID goods_id = id_list.get(i);
							if(GObj.GetPacket(goods_id, &pack, 0) > 0) {
								if(GObj.GetBarcodeByTemplate(pack.Rec.ParentID, bcs_rec.Templ, &pack.Codes, temp_buf) > 0) {
									BarcodeTbl::Rec new_code_rec;
									new_code_rec.GoodsID = goods_id;
									new_code_rec.BarcodeType = 0;
									new_code_rec.Qtty = 1.0;
									STRNSCPY(new_code_rec.Code, temp_buf);
									pack.Codes.insert(&new_code_rec);
									PPID   temp_id = goods_id;
									THROW(GObj.PutPacket(&temp_id, &pack, 0));
									success_count++;
								}
								else
									skip_count++;
							}
						}
						THROW(tra.Commit());
					}
				}
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aMergeDiezNames) {
			LAssocArray to_process_list;
			THROW(GObj.CheckRights(GOODSRT_UNITE));
			for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
				Goods2Tbl::Key2 k2;
				temp_buf = item.Name;
				uint   undup_pos = 0;
				if(PPObjGoods::HasUndupNameSuffix(temp_buf, &undup_pos)) {
					assert(undup_pos > 3);
					temp_buf.Trim(undup_pos).Strip();
                    k2.Kind = PPGDSK_GOODS;
                    STRNSCPY(k2.Name, temp_buf);
                    if(GObj.P_Tbl->search(2, &k2, spEq)) {
						const PPID src_id = GObj.P_Tbl->data.ID;
						if(!to_process_list.SearchPair(src_id, item.ID, 0))
							to_process_list.Add(src_id, item.ID, 0);
                    }
				}
				else {
					const size_t inl = sstrlen(item.Name);
                    k2.Kind = PPGDSK_GOODS;
                    STRNSCPY(k2.Name, item.Name);
                    if(GObj.P_Tbl->search(2, &k2, spGe) && GObj.P_Tbl->data.Kind == PPGDSK_GOODS && strncmp(GObj.P_Tbl->data.Name, item.Name, inl) == 0) do {
						temp_buf = GObj.P_Tbl->data.Name;
						if(PPObjGoods::HasUndupNameSuffix(temp_buf, &undup_pos)) {
							temp_buf.Trim(undup_pos).Strip();
							if(temp_buf.CmpNC(item.Name) == 0) {
								const PPID dest_id = GObj.P_Tbl->data.ID;
								if(!to_process_list.SearchPair(item.ID, dest_id, 0))
									to_process_list.Add(item.ID, dest_id, 0);
							}
						}
                    } while(GObj.P_Tbl->search(2, &k2, spNext) && GObj.P_Tbl->data.Kind == PPGDSK_GOODS && strncmp(GObj.P_Tbl->data.Name, item.Name, inl) == 0);
				}
			}
			to_process_list.Sort();
			{
				//PPTransaction tra(1);
				//THROW(tra);
				for(uint i = 0; i < to_process_list.getCount(); i++) {
					PPID   src_id = to_process_list.at(i).Key;
					PPID   dest_id = to_process_list.at(i).Val;
					THROW(PPObject::ReplaceObj(PPOBJ_GOODS, dest_id, src_id, PPObject::use_transaction));
					PPWaitPercent(i+1, to_process_list.getCount());
				}
				//THROW(tra.Commit());
			}
		}
		else if(GmParam.Action == GoodsMoveParam::aSplitBarcodeItems) {
			LAssocArray to_process_list;
			PPID   prev_id = 0;
			SString result_barcode;
			SString goods_name;
			SString suffix;
			BarcodeArray bc_list;
			LongArray valid_bc_pos_list;
			PPGoodsPacket org_pack;
			PPGoodsPacket new_pack;
			THROW(GObj.CheckRights(GOODSRT_UNITE));
			for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
				if(item.ID != prev_id) {
					GObj.ReadBarcodes(item.ID, bc_list);
					if(bc_list.getCount() > 1) {
						valid_bc_pos_list.clear();
						for(uint i = 0; i < bc_list.getCount(); i++) {
							const BarcodeTbl::Rec & r_bc_item = bc_list.at(i);
							temp_buf = r_bc_item.Code;
							int    diag = 0, std = 0;
							int    dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
							if(dbr > 0) {
								assert(temp_buf == result_barcode);
								valid_bc_pos_list.add(i);
							}
						}
						if(valid_bc_pos_list.getCount() > 1) {
							id_list.add(item.ID);
						}
					}
					//
					prev_id = item.ID;
				}
				PPWaitPercent(GetCounter());
			}
			if(id_list.getCount()) {
				PPTransaction tra(1);
				THROW(tra);
				id_list.sortAndUndup();
				for(uint idlidx = 0; idlidx < id_list.getCount(); idlidx++) {
					const PPID goods_id = id_list.get(idlidx);
					if(GObj.GetPacket(goods_id, &org_pack, 0) > 0) {
						valid_bc_pos_list.clear();
						bc_list = org_pack.Codes;
						for(uint i = 0; i < org_pack.Codes.getCount(); i++) {
							const BarcodeTbl::Rec & r_bc_item = org_pack.Codes.at(i);
							temp_buf = r_bc_item.Code;
							int    diag = 0, std = 0;
							int    dbr = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &result_barcode);
							if(dbr > 0) {
								assert(temp_buf == result_barcode);
								valid_bc_pos_list.add(i);
							}
						}
						if(valid_bc_pos_list.getCount() > 1) {
							uint j;
							for(j = valid_bc_pos_list.getCount()-1; j > 0; j--) { // (j > 0) первый код оставляем в оригинальном пакете
								const uint bc_pos = valid_bc_pos_list.get(j);
								const BarcodeTbl::Rec & r_bc_item = org_pack.Codes.at(bc_pos);
								BarcodeTbl::Rec & r_org_bc_item = org_pack.Codes.at(bc_pos);
								assert(strcmp(r_bc_item.Code, r_org_bc_item.Code) == 0);
								org_pack.Codes.atFree(bc_pos);
							}
							{
								PPID   temp_id = org_pack.Rec.ID;
								THROW(GObj.PutPacket(&temp_id, &org_pack, 0));
								success_count++;
							}
							for(j = valid_bc_pos_list.getCount()-1; j > 0; j--) { // (j > 0) первый код оставляем в оригинальном пакете
								const uint bc_pos = valid_bc_pos_list.get(j);
								const BarcodeTbl::Rec & r_bc_item = bc_list.at(bc_pos);
								BarcodeTbl::Rec & r_org_bc_item = bc_list.at(bc_pos);
								assert(strcmp(r_bc_item.Code, r_org_bc_item.Code) == 0);
								new_pack = org_pack;
								new_pack.Rec.ID = 0;
								new_pack.ArCodes.freeAll();
								new_pack.Codes.freeAll();
								new_pack.Codes.insert(&r_bc_item);
								new_pack.TagL.PutItem(PPTAG_GOODS_UUID, 0);
								new_pack.TagL.PutItem(PPTAG_GOODS_OUTERPOSCODE, 0);
								{
									PPID   temp_nm_goods_id = 0;
									long   uc = 1;
									goods_name = new_pack.Rec.Name;
									temp_buf = goods_name;
									while(GObj.SearchByName(temp_buf, &temp_nm_goods_id, 0) > 0) {
										suffix.Z().Space().CatChar('#').Cat(++uc);
										temp_buf = goods_name;
										size_t sum_len = temp_buf.Len() + suffix.Len();
										if(sum_len > max_nm_len)
											temp_buf.Trim(max_nm_len-suffix.Len());
										temp_buf.Cat(suffix);
									}
									goods_name = temp_buf;
									STRNSCPY(new_pack.Rec.Name, goods_name);
								}
								{
									PPID   temp_id = 0;
									THROW(GObj.PutPacket(&temp_id, &new_pack, 0));
									success_count++;
								}
							}
						}
					}
					PPWaitPercent(idlidx+1, id_list.getCount());
				}
				THROW(tra.Commit());
			}
		}
		PPWait(0);
		{
			SString fmt_buf, msg_buf;
			PPLoadText(PPTXT_GOODSRMVALL, fmt_buf);
			PPOutputMessage(msg_buf.Printf(fmt_buf, success_count, skip_count), mfInfo | mfOK);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewGoods::AddGoodsFromBasket()
{
	int    ok = -1;
	if(IsAltFltGroup()) {
		PPID   gb_id = 0;
		if(ListBoxSelDialog(PPOBJ_GOODSBASKET, &gb_id, 0) > 0 && gb_id > 0) {
			int    r = 0;
			PPBasketPacket pack;
			PPObjGoodsBasket gb_obj;
			THROW(r = gb_obj.GetPacket(gb_id, &pack));
			if(r > 0) {
				uint   count = pack.Lots.getCount();
				ILTI * p_item = 0;
				PPWait(1);
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; pack.Lots.enumItems(&i, (void **)&p_item) > 0;) {
					THROW(GObj.AssignGoodsToAltGrp(p_item->GoodsID, Filt.GrpID, 0, 0));
					PPWaitPercent(i-1, count);
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
	}
	else if(IsGenGoodsFlt()) {
		PPID   gb_id = 0;
		if(ListBoxSelDialog(PPOBJ_GOODSBASKET, &gb_id, 0) > 0 && gb_id > 0) {
			int    r = 0;
			PPBasketPacket pack;
			PPObjGoodsBasket gb_obj;
			THROW(r = gb_obj.GetPacket(gb_id, &pack));
			if(r > 0) {
				uint   count = pack.Lots.getCount();
				ILTI * p_item = 0;
				PPWait(1);
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; pack.Lots.enumItems(&i, (void **)&p_item) > 0;) {
					THROW(GObj.AssignGoodsToGen(p_item->GoodsID, Filt.GrpID, 0, 0));
					PPWaitPercent(i-1, count);
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoods::AddItem(GoodsListDialog ** ppDlgPtr, PPViewBrowser * pBrw, PPID * pID)
{
	int    ok = -1, ta = 0, r;
	PPID   id = 0;
	ASSIGN_PTR(pID, 0);
	GoodsListDialog * dlg = 0;
	if(IsAltFltGroup() || IsGenGoodsFlt()) {
		if(!GObj.CheckFlag(Filt.GrpID, GF_DYNAMIC)) {
			if(ppDlgPtr == 0 || *ppDlgPtr == 0) {
				THROW(CheckDialogPtrErr(&(dlg = new GoodsListDialog(0))));
			}
			else
				dlg = *ppDlgPtr;
			ASSIGN_PTR(ppDlgPtr, 0);
			if((r = ExecView(dlg)) == cmOK || r == cmAutoFill) {
				if(r == cmAutoFill) {
					ASSIGN_PTR(ppDlgPtr, dlg);
					if(IsAltFltGroup()) {
						PPObjGoodsGroup ggobj;
						GoodsFilt tmpf;
						if(ggobj.ReadGoodsFilt(Filt.GrpID, &tmpf) > 0 && !tmpf.IsEmpty()) {
							int  is_autofill = 0;
							Goods2Tbl::Rec grec;
							PPWait(1);
							for(GoodsIterator giter(&tmpf, 0); giter.Next(&grec) > 0;) {
								THROW(PPCheckUserBreak());
								if(!GObj.AssignGoodsToAltGrp(grec.ID, Filt.GrpID, 0, 1))
									PPError();
								else
									is_autofill = 1;
								PPWaitPercent(giter.GetIterCounter());
							}
							if(is_autofill)
								DS.LogAction(PPACN_AUTOFILLALTGRP, PPOBJ_GOODSGROUP, Filt.GrpID, 0, 1);
							THROW(UpdateTempTable(0, pBrw));
							PPWait(0);
							ASSIGN_PTR(ppDlgPtr, 0);
							ok = 1;
						}
					}
					else if(IsGenGoodsFlt()) {
					}
				}
				else if((id = dlg->getSelectedItem()) != 0) {
					ASSIGN_PTR(ppDlgPtr, dlg);
					if(IsAltFltGroup()) {
						SString grp_name;
						int    reply = 1; // 1 - standard, 2 - generic members to group, other - cancel
						if(GObj.IsGeneric(id)) {
							int   cf = PPMessage(mfConf|mfYes|mfNo|mfCancel, PPCFM_GENRCGOODSTOALTGRP);
							if(cf == cmYes)
								reply = 2;
							else if(cf == cmNo)
								reply = 1;
							else
								reply = -1;
						}
						if(reply == 1) {
							int    s = GObj.AssignGoodsToAltGrp(id, Filt.GrpID, 0, 1);
							while(s < 0 && PPErrCode == PPERR_DUPEXCLALTGRPMEMBER) {
								PPID   miss_grp_id = DS.GetConstTLA().LastErrObj.Id;
								if(PPMessage(mfConf|mfYesNo, PPCFM_DUPEXCLALTGRPMEMBER, GetGoodsName(miss_grp_id, grp_name)) == cmYes) {
									THROW(PPStartTransaction(&ta, 1));
									THROW(PPRef->Assc.Remove(PPASS_ALTGOODSGRP, miss_grp_id, id, 0));
									if((s = GObj.AssignGoodsToAltGrp(id, Filt.GrpID, 0, 0)) > 0) {
										THROW(PPCommitWork(&ta));
										ok = 1;
									}
									else {
										const int err_code = PPErrCode;
										THROW(PPRollbackWork(&ta));
										ok = PPSetError(err_code);
									}
								}
								else
									break;
							}
							if(s > 0) {
								THROW(UpdateTempTable(id, pBrw));
								ASSIGN_PTR(pID, id);
								ok = 1;
							}
							else
								THROW(PPErrCode == PPERR_DUPEXCLALTGRPMEMBER);
						}
						else if(reply == 2) {
							PPIDArray gen_member_list;
							PPIDArray to_update_list;
							GObj.GetGenericList(id, &gen_member_list);
							PPTransaction tra(1);
							THROW(tra);
							for(uint i = 0; i < gen_member_list.getCount(); i++) {
								const PPID member_goods_id = gen_member_list.get(i);
								if(GObj.AssignGoodsToAltGrp(member_goods_id, Filt.GrpID, 0, 0) > 0)
									to_update_list.add(member_goods_id);
							}
							THROW(tra.Commit());
							if(to_update_list.getCount()) {
								to_update_list.sortAndUndup();
								for(uint j = 0; j < to_update_list.getCount(); j++)
									UpdateTempTable(to_update_list.get(j), pBrw);
								ok = 1;
							}
						}
					}
					else if(IsGenGoodsFlt()) {
						THROW(GObj.AssignGoodsToGen(id, Filt.GrpID, 0, 1));
						THROW(UpdateTempTable(id, pBrw));
						ASSIGN_PTR(pID, id);
						ok = 1;
					}
				}
			}
		}
	}
	else {
		PPID   grp_id = (Filt.GrpID > 0 && !(Filt.Flags & GoodsFilt::fGenGoods)) ? Filt.GrpID : 0;
		PPID   cls_id = Filt.Ep.GdsClsID;
		if(GObj.Edit(&(id = 0), gpkndGoods, grp_id, cls_id, 0) == cmOK) {
			THROW(UpdateTempTable(id, pBrw));
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	CATCH
		PPRollbackWork(&ta);
		ok = PPErrorZ();
	ENDCATCH
	if(*ppDlgPtr == 0) {
		ZDELETE(dlg);
	}
	return ok;
}

void SLAPI PPViewGoods::ViewGenMembers(PPID id)
{
	if(id && GObj.IsGeneric(id)) {
		GoodsFilt flt;
		flt.GrpID = id;
		flt.Flags |= GoodsFilt::fGenGoods;
		PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
	}
}

int PPViewGoods::Detail(const void * pHdr, PPViewBrowser * pBrw)
{
	ViewGenMembers(pHdr ? *static_cast<const PPID *>(pHdr) : 0);
	return -1;
}

static int RecoverGoodsExtPropRef(PPID goodsID, PPID * pRefID, int isCls, const PPGdsClsProp & rProp, PPLogger * pLogger)
{
	int    ok = 1;
	uint   msg_id = 0;
	if(*pRefID) {
		if(isCls > 0)
			if(rProp.ItemsListID) {
				PPObject * p_obj = GetPPObject(rProp.ItemsListID, 0);
				if(!p_obj || p_obj->Search(*pRefID, 0) < 0) {
					msg_id = PPTXT_LOG_GOODSEXTHANGPROP;
					ok = 0;
				}
				delete p_obj;
			}
			else {
				msg_id = PPTXT_LOG_GOODSEXTUNDEFPROP;
				ok = 0;
			}
		else {
			msg_id = PPTXT_LOG_GOODSEXTUNCLSPROP;
			ok = 0;
		}
		if(!ok) {
			*pRefID = 0;
			if(pLogger && msg_id) {
				SString goods_name;
				pLogger->LogString(msg_id, GetGoodsName(goodsID, goods_name));
			}
		}
	}
	else
		ok = -1;
	return ok;
}

struct GoodsRecoverParam {
	GoodsRecoverParam() : Flags(0)
	{
	}
	enum {
		fCorrect          = 0x0001, // Исправлять ошибки
		fCheckAlcoAttribs = 0x0002, // Проверять алкогольные атрибуты
		fBarcode          = 0x0004  // @v10.8.5 Проверять валидность штрихкодов. Если fCorrect, то добавлять или исправлять контрольную цифру 
	};
	SString LogFileName;  // Имя файла журнала, в который заносится информация об ошибках
	long   Flags;
};

static int SLAPI EditGoodsRecoverParam(GoodsRecoverParam * pData)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_RCVRGOODS);
	if(CheckDialogPtrErr(&dlg)) {
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_RCVRGOODS_LOG, CTL_RCVRGOODS_LOG, 1, 0, 0, FileBrowseCtrlGroup::fbcgfLogFile);
		dlg->setCtrlString(CTL_RCVRGOODS_LOG, pData->LogFileName);
		dlg->AddClusterAssoc(CTL_RCVRGOODS_FLAGS, 0, GoodsRecoverParam::fCorrect);
		dlg->AddClusterAssoc(CTL_RCVRGOODS_FLAGS, 1, GoodsRecoverParam::fCheckAlcoAttribs);
		dlg->AddClusterAssoc(CTL_RCVRGOODS_FLAGS, 2, GoodsRecoverParam::fBarcode);
		dlg->SetClusterData(CTL_RCVRGOODS_FLAGS, pData->Flags);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_RCVRGOODS_LOG, pData->LogFileName);
			dlg->GetClusterData(CTL_RCVRGOODS_FLAGS, &pData->Flags);
			ok = 1;
		}
	}
	else
		ok = 1;
	delete dlg;
	return ok;
}

int SLAPI PPViewGoods::Repair(PPID /*id*/)
{
	int    ok = 1;
	PPLogger logger;
	PPEgaisProcessor * p_eg_prc = 0;
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket  gc_pack;
	GoodsViewItem item;
	GoodsRecoverParam param;
	SString temp_buf;
	SString valid_code;
	SString fmt_buf, msg_buf;
	if(EditGoodsRecoverParam(&param) > 0) {
		if(param.Flags & param.fCheckAlcoAttribs) {
			THROW_MEM(p_eg_prc = new PPEgaisProcessor(PPEgaisProcessor::cfUseVerByConfig, &logger, 0));
		}
		PPWait(1);
		// @v10.7.9 {
		{
			//
			// Проверка на наличие висячих записей штрихкодов
			//
			BarcodeTbl & r_bctbl = GObj.P_Tbl->GetBcTbl_();
			BarcodeTbl::Key0 bck0;
			PPIDArray hanged_bc_goods_list;
			MEMSZERO(bck0);
			if(r_bctbl.search(0, &bck0, spFirst)) do {
				PPID   bc_goods_id = r_bctbl.data.GoodsID;
				int    r = GObj.Search(bc_goods_id, 0);
				if(r > 0) {
					; // ok
				}
				else if(r < 0) {
					// Висячая запись штрихкода %s
					// PPTXT_LOG_HANGEDBARCODEREC
					hanged_bc_goods_list.add(bc_goods_id);
					msg_buf.Z().Cat(r_bctbl.data.Code).Space().CatChar('#').Cat(bc_goods_id);
					logger.LogString(PPTXT_LOG_HANGEDBARCODEREC, msg_buf);
				}
				else {
					logger.LogLastError();
				}
			} while(r_bctbl.search(0, &bck0, spNext));
			if(param.Flags & GoodsRecoverParam::fCorrect && hanged_bc_goods_list.getCount()) {
				hanged_bc_goods_list.sortAndUndup();
				PPTransaction tra(1);
				THROW(tra);
				for(uint ididx = 0; ididx < hanged_bc_goods_list.getCount(); ididx++) {
					THROW(GObj.P_Tbl->UpdateBarcodes(hanged_bc_goods_list.get(ididx), 0, 0));
				}
				THROW(tra.Commit());
			}
		}
		// } @v10.7.9 
		PPID   prev_id = 0;
		for(InitIteration(); NextIteration(&item) > 0;) {
			if(item.ID != prev_id) {
				PPGoodsPacket pack;
				int    err = 0;
				int    to_turn_packet = 0;
				if(GObj.GetPacket(item.ID, &pack, 0) > 0) {
					const int is_cls = BIN(pack.Rec.GdsClsID && gc_obj.GetPacket(pack.Rec.GdsClsID, &gc_pack) > 0);
					if(!RecoverGoodsExtPropRef(pack.Rec.ID, &pack.ExtRec.KindID, is_cls, gc_pack.PropKind, &logger))
						err = 1;
					if(!RecoverGoodsExtPropRef(pack.Rec.ID, &pack.ExtRec.GradeID, is_cls, gc_pack.PropGrade, &logger))
						err = 1;
					if(!RecoverGoodsExtPropRef(pack.Rec.ID, &pack.ExtRec.AddObjID, is_cls, gc_pack.PropAdd, &logger))
						err = 1;
					if(!RecoverGoodsExtPropRef(pack.Rec.ID, &pack.ExtRec.AddObj2ID, is_cls, gc_pack.PropAdd2, &logger))
						err = 1;
					//
					if(is_cls > 0) {
						THROW(gc_pack.CompleteGoodsPacket(&pack));
						to_turn_packet = 1;
					}
					else if(!pack.Stock.IsEmpty()) {
						to_turn_packet = 1;
					}
					{ // Проверка на флаги
						const long f = (pack.Rec.Flags & GF_DB_FLAGS_GOODS);
						if(f != pack.Rec.Flags) {
							pack.Rec.Flags = f;
							logger.LogString(PPTXT_LOG_GOODSFLAGSMISSMATCH, pack.Rec.Name);
							err = 1;
						}
					}
					{ // Проверка на обнуление зарезервированного пространства
						for(size_t i = 0; i < sizeof(pack.Rec.Reserve); i++)
							if(pack.Rec.Reserve[i] != 0) {
								pack.Rec.Reserve[i] = 0;
								to_turn_packet = 1;
								break;
							}
					}
					{
						if(pack.Rec.WrOffGrpID && SearchObject(PPOBJ_ASSTWROFFGRP, pack.Rec.WrOffGrpID, 0) <= 0) {
							pack.Rec.WrOffGrpID = 0;
							logger.LogString(PPTXT_LOG_GOODSHANGWROFFGRP, pack.Rec.Name);
							err = 1;
						}
					}
					if(pack.Rec.Flags & GF_VOLUMEVAL && pack.Stock.IsEmpty()) {
						pack.Rec.Flags &= ~GF_VOLUMEVAL;
						to_turn_packet = 1;
					}
					if(param.Flags & GoodsRecoverParam::fBarcode) {
						for(uint i = 0; i < pack.Codes.getCount(); i++) {
							temp_buf = pack.Codes.at(i).Code;
							if(temp_buf.NotEmptyS()) {
								if(!oneof2(temp_buf.Len(), 3, 19) && !GObj.GetConfig().IsWghtPrefix(temp_buf)) {
									int    diag = 0, std = 0;
									int    r = PPObjGoods::DiagBarcode(temp_buf, &diag, &std, &valid_code);
									if(r <= 0) { // @v10.8.5 (r == 0)-->(r <= 0)
										PPObjGoods::GetBarcodeDiagText(diag, fmt_buf);
										PPBarcode::GetStdName(std, temp_buf);
										(msg_buf = pack.Rec.Name).CatDiv(':', 2).Cat(temp_buf).
											Cat(pack.Codes.at(i).Code).Space().Cat("->").Space().Cat(valid_code).CatDiv('-', 1).Cat(fmt_buf);
										logger.Log(msg_buf);
										// @v10.8.5 {
										if(param.Flags & GoodsRecoverParam::fCorrect) {
											if(diag == PPObjGoods::cddInvCheckDigEan13) {
												STRNSCPY(pack.Codes.at(i).Code, valid_code);
												to_turn_packet = 1;
											}
											else if(GObj.GetConfig().Flags & GCF_BCCHKDIG) {
												if(diag == PPObjGoods::cdd_Ean13WoCheckDig) {
													STRNSCPY(pack.Codes.at(i).Code, valid_code);
													to_turn_packet = 1;
												}
											}
										}
										// } @v10.8.5 
									}
								}
							}
						}
					}
					if(p_eg_prc) {
						const int agr = p_eg_prc->IsAlcGoods(pack.Rec.ID);
						// 402
						if(agr > 0) {
							PPEgaisProcessor::GoodsItem agi;
							const int pgir = p_eg_prc->PreprocessGoodsItem(pack.Rec.ID, 0, 0, PPEgaisProcessor::pgifForceUsingInnerDb, agi);
							if(!pgir) {
								StringSet msg_ss('\t', agi.MsgPool);
								for(uint ssp = 0; msg_ss.get(&ssp, temp_buf);) {
									logger.Log(temp_buf);
								}
							}
							if(agi.StatusFlags & agi.stEgaisCodeByGoods) {
							}
							if(agi.CategoryCode.Empty())
								logger.LogString(PPTXT_LOG_ALCGOODSHASNTCAT, pack.Rec.Name); // Для алкогольного товара '%s' не определен вид алкогольной продукции
							if(agi.Proof <= 0.0)
								logger.LogString(PPTXT_LOG_ALCGOODSHASNTPROOF, pack.Rec.Name); // Для алкогольного товара '%s' не определено содержание спирта
							if(agi.Volume <= 0.0)
								logger.LogString(PPTXT_LOG_ALCGOODSHASNTVOL, pack.Rec.Name); // Для алкогольного товара '%s' не определен объем
						}
					}
					if(err || to_turn_packet) {
						if(param.Flags & GoodsRecoverParam::fCorrect)
							THROW(GObj.PutPacket(&item.ID, &pack, 1));
					}
				}
			}
			prev_id = item.ID;
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	logger.Save(param.LogFileName, 0);
	delete p_eg_prc;
	return ok;
}

int SLAPI PPViewGoods::AddBarcodeCheckDigit()
{
	int    ok = -1;
	GoodsViewItem item;
	if(PPMaster) {
		PPWait(1);
		PPTransaction tra(1);
		THROW(tra);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
			int    r = 0;
			uint   i;
			BarcodeArray bc_list;
			BarcodeTbl::Rec * p_bc;
			THROW(GObj.ReadBarcodes(item.ID, bc_list));
			for(i = 0; bc_list.enumItems(&i, (void **)&p_bc);) {
				const size_t bc_len = sstrlen(p_bc->Code);
				if(oneof3(bc_len, 11, 12, 7)) {
					::AddBarcodeCheckDigit(p_bc->Code);
					r = 1;
				}
			}
			if(r) {
				THROW(GObj.P_Tbl->UpdateBarcodes(item.ID, &bc_list, 0));
				ok = 1;
			}
		}
		THROW(tra.Commit());
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewGoods::Transmit(PPID /*id*/)
{
	int    ok = -1;
	ObjTransmitParam param;
	if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
		GoodsViewItem item;
		const PPIDArray & rary = param.DestDBDivList.Get();
		PPObjIDArray objid_ary;
		PPWait(1);
		for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter()))
			objid_ary.Add(PPOBJ_GOODS, item.ID);
		THROW(PPObjectTransmit::Transmit(&rary, &objid_ary, &param));
		ok = 1;
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

int SLAPI PPViewGoods::ReplaceNames()
{
	class SrGoodsDialog : public TDialog {
	public:
		SrGoodsDialog() : TDialog(DLG_SRGOODS)
		{
		}
		int setDTS(const PPGoodsReplaceNameParam * pData)
		{
			int    ok = 1;
			ushort v = 0;
			Data = *pData;
			setCtrlString(CTL_SR_SEARCH, Data.SrchPattern);
			setCtrlString(CTL_SR_REPLACE, Data.RplcPattern);
			AddClusterAssoc(CTL_SRGOODS_DOREPLACE, 0, PPGoodsReplaceNameParam::fDoReplace);
			SetClusterData(CTL_SRGOODS_DOREPLACE, Data.Flags);
			disableCtrl(CTL_SR_REPLACE, !BIN(Data.Flags & PPGoodsReplaceNameParam::fDoReplace));
			if(Data.Flags & PPGoodsReplaceNameParam::fAbbr)
				v = 1;
			else if(Data.Flags & PPGoodsReplaceNameParam::fNameAndAbbr)
				v = 2;
			else
				v = 0;
			setCtrlData(CTL_SR_WHAT, &v);
			SetupPPObjCombo(this, CTLSEL_SRGOODS_BRAND, PPOBJ_BRAND, Data.BrandID, OLW_CANINSERT);
			SetupPPObjCombo(this, CTLSEL_SRGOODS_GRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANINSERT, reinterpret_cast<void *>(GGRTYP_SEL_NORMAL));
			SetupPersonCombo(this, CTLSEL_SRGOODS_MANUF, Data.ManufID, OLW_CANINSERT, PPPRK_MANUF, 1);
			AddClusterAssoc(CTL_SRGOODS_SPCFLAGS, 0, PPGoodsReplaceNameParam::fRestoreLastHistoryName);
			SetClusterData(CTL_SRGOODS_SPCFLAGS, Data.Flags);
			return ok;
		}
		int getDTS(PPGoodsReplaceNameParam * pData)
		{
			int    ok = 1;
			getCtrlString(CTL_SR_SEARCH, Data.SrchPattern);
			getCtrlString(CTL_SR_REPLACE, Data.RplcPattern);
			ushort v = getCtrlUInt16(CTL_SR_WHAT);
			SETFLAG(Data.Flags, PPGoodsReplaceNameParam::fAbbr, v == 1);
			SETFLAG(Data.Flags, PPGoodsReplaceNameParam::fNameAndAbbr, v == 2);
			getCtrlData(CTLSEL_SRGOODS_BRAND, &Data.BrandID);
			getCtrlData(CTLSEL_SRGOODS_GRP,   &Data.GoodsGrpID);
			getCtrlData(CTLSEL_SRGOODS_MANUF, &Data.ManufID);
			GetClusterData(CTL_SRGOODS_SPCFLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isClusterClk(CTL_SRGOODS_DOREPLACE)) {
				GetClusterData(CTL_SRGOODS_DOREPLACE, &Data.Flags);
				disableCtrl(CTL_SR_REPLACE, !BIN(Data.Flags & PPGoodsReplaceNameParam::fDoReplace));
			}
			else
				return;
			clearEvent(event);
		}
		PPGoodsReplaceNameParam Data;

	};
	int    ok = -1;
	if(!GObj.CheckRights(PPR_MOD) || !GObj.CheckRights(GOODSRT_MULTUPD))
		return PPErrorZ();
	SrGoodsDialog * dlg = new SrGoodsDialog;
	TDialog * dlg_cfm = new TDialog(DLG_SRGOODSCFM);
	if(CheckDialogPtrErr(&dlg) && CheckDialogPtrErr(&dlg_cfm)) {
		PPGoodsReplaceNameParam param;
		dlg->setDTS(&param);
		if(ExecView(dlg) == cmOK) {
			int    yes_for_all = 0;
			dlg->getDTS(&param);
			PPWait(1);
			GoodsViewItem item;
			SString old_name;
			PPIDArray goods_list;
			for(InitIteration(); NextIteration(&item) > 0; PPWaitPercent(GetCounter())) {
				goods_list.add(item.ID);
			}
			goods_list.sortAndUndup();
			for(uint i = 0; i < goods_list.getCount(); i++) {
				PPID goods_id = goods_list.get(i);
				PPGoodsPacket pack;
				if(GObj.GetPacket(goods_id, &pack, 0) > 0) {
					old_name = (param.Flags & PPGoodsReplaceNameParam::fAbbr) ? pack.Rec.Abbr : pack.Rec.Name;
					if(GObj.ReplaceName(&pack, &param) > 0) {
						dlg_cfm->setCtrlString(CTL_SR_SEARCH, old_name);
						if(param.Flags & PPGoodsReplaceNameParam::fAbbr)
							dlg_cfm->setCtrlData(CTL_SR_REPLACE, pack.Rec.Abbr);
						else
							dlg_cfm->setCtrlData(CTL_SR_REPLACE, pack.Rec.Name);
						int cfm_cmd = yes_for_all ? cmYes : ExecView(dlg_cfm);
						if(cfm_cmd == cmYes || cfm_cmd == cmaAll) {
							if(cfm_cmd == cmaAll)
								yes_for_all = 1;
							if(param.BrandID)
								pack.Rec.BrandID = param.BrandID;
							if(param.GoodsGrpID)
								pack.Rec.ParentID = param.GoodsGrpID;
							if(param.ManufID)
								pack.Rec.ManufID = param.ManufID;
							if(!GObj.PutPacket(&goods_id, &pack, 1)) {
								PPError();
								PPWait(1);
							}
							else
								ok = 1;
						}
						else if(cfm_cmd == cmCancel)
							break;
					}
				}
				PPWaitPercent(i+1, goods_list.getCount());
			}
			PPWait(0);
		}
	}
	else
		ok = 0;
	delete dlg;
	delete dlg_cfm;
	return ok;
}

int SLAPI PPViewGoods::UpdateFlags()
{
	int    ok = -1;
	long   setf = 0, resetf = 0;
	TDialog * dlg = 0;
	THROW(GObj.CheckRights(PPR_MOD) && GObj.CheckRights(GOODSRT_MULTUPD));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_UPDGOODSFLAGS))));
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_SET, 0, GF_NODISCOUNT);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_SET, 1, GF_PASSIV);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_SET, 2, GF_PRICEWOTAXES);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_SET, 3, GF_WANTVETISCERT); // @v10.1.12
	dlg->SetClusterData(CTL_UPDGOODSFLAGS_SET, setf);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_RESET, 0, GF_NODISCOUNT);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_RESET, 1, GF_PASSIV);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_RESET, 2, GF_PRICEWOTAXES);
	dlg->AddClusterAssoc(CTL_UPDGOODSFLAGS_RESET, 3, GF_WANTVETISCERT); // @v10.1.12
	dlg->SetClusterData(CTL_UPDGOODSFLAGS_RESET, resetf);
	if(ExecView(dlg) == cmOK) {
		setf = resetf = 0;
		dlg->GetClusterData(CTL_UPDGOODSFLAGS_SET, &setf);
		dlg->GetClusterData(CTL_UPDGOODSFLAGS_RESET, &resetf);
		if(setf || resetf) {
			GoodsViewItem item;
			PPWait(1);
			PPTransaction tra(1);
			THROW(tra);
			for(InitIteration(); NextIteration(&item) > 0;) {
				int    r;
				THROW(r = GObj.UpdateFlags(item.ID, setf, resetf, 0));
				if(r > 0)
					ok = 1;
				PPWaitPercent(GetCounter());
			}
			THROW(tra.Commit());
			PPWait(0);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int SLAPI PPViewGoods::Print(const void *)
{
	uint   rpt_id = 0;
	if(Filt.Flags & GoodsFilt::fShowCargo)
		rpt_id = REPORT_GOODSCARGOVIEW;
	else if(Filt.Flags & GoodsFilt::fShowStrucType)
		rpt_id = REPORT_GOODSWITHSTRUC;
	else
		rpt_id = REPORT_GOODSVIEW;
	PView  pf(this);
	PPReportEnv env;
	env.Sort = Filt.InitOrder;
	// env.PrnFlags = SReport::DisableGrouping;
	PPAlddPrint(rpt_id, &pf, &env);
	return 1;
}

int SLAPI PPViewGoods::PrintPLabel(PPID goodsID)
{
	uint   rpt_id = 0;
	ushort v = 0;
	int    r = -1;
	TDialog * p_dlg = 0;
	PriceListFilt flt;
	PPViewPriceList v_price;
	PView  pf(&v_price); // @v9.0.4 (&v_price)
	PPReportEnv env;
	env.Sort = OrdByName;
	env.PrnFlags = SReport::DisableGrouping;
	if(goodsID) {
		if(CheckDialogPtrErr(&(p_dlg = new TDialog(DLG_GPLABEL))) > 0) {
			p_dlg->setCtrlUInt16(CTL_GPLABEL_WHAT, 0);
			if(ExecView(p_dlg) == cmOK) {
				v = p_dlg->getCtrlUInt16(CTL_GPLABEL_WHAT);
				rpt_id = (v == 1) ? REPORT_PLABELBIG : REPORT_PLABELSMALL;
				r = 1;
			}
			ZDELETE(p_dlg);
			if(r > 0) {
				flt.GoodsIDList.Add(goodsID);
				flt.LocID = NZOR(Filt.LocList.GetSingle(), LConfig.Location);
				flt.GoodsGrpID = Filt.GrpID;
				v_price.Init_(&flt);
				// @v9.0.4 pf.Ptr = &v_price;
			}
		}
		else
			r = 0;
	}
	return (r > 0) ? PPAlddPrint(rpt_id, &pf, &env) : r;
}

int SLAPI PPViewGoods::Export(const PPGoodsImpExpParam * pExpCfg)
{
	int    ok = 1, r;
	PPGoodsExporter g_e;
	THROW(r = g_e.Init(pExpCfg));
	if(r > 0) {
		PPWait(1);
		GoodsViewItem item;
		for(InitIteration(OrdByDefault); NextIteration(&item) > 0;) {
			PPGoodsPacket gpack;
			THROW(GObj.GetPacket(item.ID, &gpack, 0));
			strip(item.Barcode);
			THROW(g_e.ExportPacket(&gpack, item.Barcode));
			PPWaitPercent(GetCounter());
		}
		PPWait(0);
	}
	CATCHZOKPPERR
	return ok;
}

struct UhttExpGoodsParam {
	enum {
		fOnlyUnassocItems = 0x0001
	};
	enum {
		coGoodsGrpName = 0,
		coTag
	};

	UhttExpGoodsParam() : Flags(0), CategoryObject(coGoodsGrpName), CategoryTagID(0)
	{
	}
	long   Flags;
	long   CategoryObject; // Сущность, которую следует выгружать в качестве наименования категории товара
	PPID   CategoryTagID;
};

int SLAPI PPViewGoods::ExportUhtt()
{
	class UhttExportGoodsDialog : public TDialog {
		DECL_DIALOG_DATA(UhttExpGoodsParam);
	public:
		explicit UhttExportGoodsDialog(PPViewGoods * pView) : TDialog(DLG_UHTTEXPGOODS), P_View(pView)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			AddClusterAssoc(CTL_UHTTEXPGOODS_CO, 0, UhttExpGoodsParam::coGoodsGrpName);
			AddClusterAssoc(CTL_UHTTEXPGOODS_CO, 1, UhttExpGoodsParam::coTag);
			SetClusterData(CTL_UHTTEXPGOODS_CO, Data.CategoryObject);
			ObjTagFilt tag_filt(PPOBJ_GOODS);
			SetupObjTagCombo(this, CTLSEL_UHTTEXPGOODS_COT, Data.CategoryTagID, 0, &tag_filt);
			disableCtrl(CTLSEL_UHTTEXPGOODS_COT, (Data.CategoryObject != UhttExpGoodsParam::coTag));
			AddClusterAssoc(CTL_UHTTEXPGOODS_FLAGS, 0, UhttExpGoodsParam::fOnlyUnassocItems);
			SetClusterData(CTL_UHTTEXPGOODS_FLAGS, Data.Flags);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
            GetClusterData(CTL_UHTTEXPGOODS_CO, &Data.CategoryObject);
            if(Data.CategoryObject == UhttExpGoodsParam::coTag) {
            	getCtrlData(sel = CTLSEL_UHTTEXPGOODS_COT, &Data.CategoryTagID);
            	THROW_PP(Data.CategoryTagID, PPERR_TAGNEEDED);
            }
            else
				Data.CategoryTagID = 0;
			GetClusterData(CTL_UHTTEXPGOODS_FLAGS, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmCountUhttRefs)) {
				GetUhttRef();
				clearEvent(event);
			}
			else if(event.isClusterClk(CTL_UHTTEXPGOODS_CO)) {
				GetClusterData(CTL_UHTTEXPGOODS_CO, &Data.CategoryObject);
				disableCtrl(CTLSEL_UHTTEXPGOODS_COT, (Data.CategoryObject != UhttExpGoodsParam::coTag));
			}
		}
		int GetUhttRef()
		{
			int    ok = -1, r;
			SString msg_buf, fmt_buf;
			if(P_View) {
				GoodsViewItem item;
				LongArray uniq_id_list;
				LAssocArray list;
				StrAssocArray code_ref_list;
				PPWait(1);
				for(P_View->InitIteration(P_View->OrdByDefault); P_View->NextIteration(&item) > 0;) {
					uniq_id_list.add(item.ID);
				}
				uniq_id_list.sortAndUndup();
				if(uniq_id_list.getCount()) {
					PPUhttClient uc;
					for(uint i = 0; i < uniq_id_list.getCount(); i++) {
						list.Add(uniq_id_list.get(i), 0, 0);
					}
					THROW(uc.Auth());
					THROW(r = uc.GetUhttGoodsRefList(list, &code_ref_list));
					list.Sort();
					{
						uint ref_count = 0;
						for(uint i = 0; i < list.getCount(); i++) {
							const PPID goods_id = list.at(i).Key;
							const PPID uhtt_id = list.at(i).Val;
							if(uhtt_id && (!i || list.at(i-1).Key != goods_id))
								ref_count++;
						}
						PPFormatT(PPTXT_UHTTEXPGOODS_REFS, &msg_buf, uniq_id_list.getCountI(), static_cast<long>(ref_count));
						setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
						ok = 1;
					}
				}
				else {
					PPLoadText(PPTXT_UHTTEXPGOODS_EMPTY, msg_buf);
					setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
				}
			}
			CATCH
				PPGetLastErrorMessage(1, msg_buf);
				setStaticText(CTL_UHTTEXPGOODS_INFO, msg_buf);
				ok = 0;
			ENDCATCH
			PPWait(0);
			return ok;
		}
		PPViewGoods * P_View; // @notowned
	};
	int    ok = -1;
	Reference * p_ref = PPRef;
	TSCollection <UhttGoodsPacket> uhtt_goods_list;
	BarcodeArray bc_list;
	PPUhttClient uc;
	PPLogger logger;
	UhttExpGoodsParam param;
	UhttExportGoodsDialog * dlg = new UhttExportGoodsDialog(this);
	THROW(CheckDialogPtr(&dlg));
	dlg->setDTS(&param);
	while(ExecView(dlg) == cmOK) {
		if(dlg->getDTS(&param)) {
			LongArray uniq_id_list;
			PPObjBrand brand_obj;
			SString img_path;
			SString temp_buf;
			SString barcode;
			SString msg_buf;
			SString fmt_buf;
			StringSet ss_uhttsync_bcode_template;
			{
				PPBarcodeStruc bcs_rec;
				for(SEnum en = p_ref->Enum(PPOBJ_BCODESTRUC, 0); en.Next(&bcs_rec) > 0;) {
					if(bcs_rec.Speciality == PPBarcodeStruc::spcUhttSync && bcs_rec.Templ[0]) {
						ss_uhttsync_bcode_template.add((temp_buf = bcs_rec.Templ).Strip());
					}
				}
			}
			ObjLinkFiles lf(PPOBJ_GOODS);
			{
				GoodsViewItem item;
				for(InitIteration(OrdByDefault); NextIteration(&item) > 0;)
					uniq_id_list.add(item.ID);
			}
			uniq_id_list.sortAndUndup();
			if(uniq_id_list.getCount()) {
				uint i;
				SString code_buf;
				SString template_buf;
				LAssocArray ref_list;
				LAssocArray brand_ref_list;
				LAssocArray manuf_ref_list;
				StrAssocArray code_ref_list;
				LongArray templated_code_pos_list; // Список позиций кодов, соответствующих Universe-HTT-шаблонам в bc_list
				PPIDArray uhtt_id_list;

				PPWait(1);
				THROW(uc.Auth());

				for(i = 0; i < uniq_id_list.getCount(); i++) {
					ref_list.Add(uniq_id_list.get(i), 0, 0);
				}
				THROW(uc.Auth());
				THROW(uc.GetUhttGoodsRefList(ref_list, &code_ref_list));
				for(i = 0; i < uniq_id_list.getCount(); i++) {
					const PPID goods_id = uniq_id_list.get(i);
					PPID   uhtt_goods_id = 0;
					int    is_private_code = 0;
					uint   private_code_pos = 0;
					Goods2Tbl::Rec goods_rec;
					PersonTbl::Rec psn_rec;
					PPBrand brand_rec;
					templated_code_pos_list.clear();
					THROW(GObj.Search(goods_id, &goods_rec) > 0);
					GObj.ReadBarcodes(goods_id, bc_list);
					// @v10.0.0 Убираем из списка кодов коды алкогольной продукции ЕГАИС {
					{
						uint bcidx = bc_list.getCount();
						if(bcidx) do {
							const BarcodeTbl::Rec & r_bc_rec = bc_list.at(--bcidx); // @v10.0.02 @fix --i-->--bcidx
							if(sstrlen(r_bc_rec.Code) == 19)
								bc_list.atFree(bcidx);
							else {
								for(uint ssp = 0; ss_uhttsync_bcode_template.get(&ssp, template_buf);) {
									if(GObj.P_Tbl->IsTemplatedBarcode(goods_rec.ParentID, GObj.GetConfig(), template_buf, r_bc_rec.Code)) {
										templated_code_pos_list.add(bcidx);
									}
								}
							}
						} while(bcidx);
						if(templated_code_pos_list.getCount() > 1) {
							//
							// Если мы нашли более одного шаблонизированного кода, то используем только первый из них
							// (не забываем, что оригинальный список bc_list мы просматривали в обратном порядке - потому
							// реверсируем templated_code_pos_list дабы извлекать первый элемент по индексу 0).
							//
							templated_code_pos_list.reverse(0, templated_code_pos_list.getCount());
						}
					}
					// } @v10.0.0
					if(bc_list.getCount()) { // @v10.0.0
						uhtt_id_list.clear();
						ref_list.GetListByKey(goods_id, uhtt_id_list);
						for(uint j = 0; j < uhtt_id_list.getCount(); j++) {
							const PPID temp_uhtt_id = uhtt_id_list.get(j);
							if(temp_uhtt_id) {
								if(code_ref_list.GetText(temp_uhtt_id, code_buf) > 0) {
									if(code_buf[0] > '9') {
										uint   cp = 0;
										int    ccr = bc_list.SearchCode(code_buf, &cp);
										assert(ccr); // Не может быть чтобы код не был найден в списке -
											// мы же его туда положили когда запрашивали соответствие через GetUhttGoodsRefList()
										if(ccr) {
											uhtt_goods_id = temp_uhtt_id;
											private_code_pos = cp;
											is_private_code = 1;
										}
									}
									else
										SETIFZ(uhtt_goods_id, temp_uhtt_id);
								}
								else {
									; // Неожиданная ситуация - код обязательно должен быть
								}
							}
						}
						if(uhtt_goods_id || (!(param.Flags & param.fOnlyUnassocItems) && bc_list.getCount())) {
							if(!uhtt_goods_id || is_private_code) {
								PPID   uhtt_brand_id = 0;
								PPID   uhtt_manuf_id = 0;
								if(goods_rec.BrandID) {
									PPID   temp_uhtt_id = 0;
									if(brand_ref_list.Search(goods_rec.BrandID, &temp_uhtt_id, 0) > 0) {
										uhtt_brand_id = temp_uhtt_id;
									}
									else if(brand_obj.Fetch(goods_rec.BrandID, &brand_rec) > 0 && brand_rec.Name[0]) {
										TSCollection <UhttBrandPacket> brand_list;
										if(uc.GetBrandByName(brand_rec.Name, brand_list) > 0 && brand_list.getCount())
											uhtt_brand_id = brand_list.at(0)->ID;
										brand_ref_list.Add(goods_rec.BrandID, uhtt_brand_id, 0);
									}
								}
								if(goods_rec.ManufID) {
									PPID   temp_uhtt_id = 0;
									if(manuf_ref_list.Search(goods_rec.ManufID, &temp_uhtt_id, 0) > 0) {
										uhtt_manuf_id = temp_uhtt_id;
									}
									else if(PsnObj.Fetch(goods_rec.ManufID, &psn_rec) > 0 && psn_rec.Name[0]) {
										TSCollection <UhttPersonPacket> person_list;
										if(uc.GetPersonByName(psn_rec.Name, person_list) > 0 && person_list.getCount()) {
											for(uint k = 0; !uhtt_manuf_id && k < person_list.getCount(); k++) {
												const UhttPersonPacket * p_pack = person_list.at(k);
												for(uint n = 0; !uhtt_manuf_id && n < p_pack->KindList.getCount(); n++) {
													if(sstreqi_ascii(p_pack->KindList.at(n)->Code, "MANUF"))
														uhtt_manuf_id = p_pack->ID;
												}
											}
										}
										manuf_ref_list.Add(goods_rec.ManufID, uhtt_manuf_id, 0);
									}
								}
								if(!uhtt_goods_id) {
									logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSNFOUND, &msg_buf, goods_id));
								}
								else if(is_private_code) {
									logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSPRVCODEUPD, &msg_buf, goods_id));
								}
								{
									for(uint m = 0; (!uhtt_goods_id || is_private_code) && m < bc_list.getCount(); m++) {
										int    one_iter = 0;
										if(!uhtt_goods_id || is_private_code) {
											assert(m < bc_list.getCount());
											assert(private_code_pos < bc_list.getCount());
											if(!is_private_code && templated_code_pos_list.getCount()) // @v10.7.6
												m = templated_code_pos_list.get(0);
											else
												m = private_code_pos;
											one_iter = 1;
										}
										(barcode = bc_list.at(m).Code).Strip();
										UhttGoodsPacket new_pack;
										Goods2Tbl::Rec grp_rec;
										new_pack.SetName(goods_rec.Name);
										new_pack.ID = uhtt_goods_id;
										new_pack.BrandID = uhtt_brand_id;
										new_pack.ManufID = uhtt_manuf_id;
										if(param.CategoryObject == UhttExpGoodsParam::coTag) {
											if(param.CategoryTagID) {
												// @v10.7.9 {
												{
													UhttTagItem * p_new_item = uc.GetUhttTagText(PPOBJ_GOODS, goods_rec.ID, param.CategoryTagID, "OuterGroup");
													if(p_new_item)
														THROW_SL(new_pack.TagList.insert(p_new_item));
												}
												// } @v10.7.9 
												/* @v10.7.9 ObjTagItem tag_item;
												if(p_ref->Ot.GetTag(PPOBJ_GOODS, goods_rec.ID, param.CategoryTagID, &tag_item) > 0) {
													tag_item.GetStr(temp_buf);
													if(temp_buf.NotEmptyS()) {
														UhttTagItem * p_new_item = new UhttTagItem("OuterGroup", temp_buf);
														THROW_MEM(p_new_item);
														THROW_SL(new_pack.TagList.insert(p_new_item));
													}
												}*/
											}
										}
										else if(param.CategoryObject == UhttExpGoodsParam::coGoodsGrpName) {
											if(GObj.Fetch(goods_rec.ParentID, &grp_rec) > 0) {
												UhttTagItem * p_new_item = new UhttTagItem("OuterGroup", temp_buf = grp_rec.Name);
												THROW_MEM(p_new_item);
												THROW_SL(new_pack.TagList.insert(p_new_item));
											}
										}
										{
											PPGoodsPacket pack;
											pack.Rec.ID = goods_id; // @trick
											pack.Rec.Flags |= GF_EXTPROP; // @trick
											if(GObj.GetValueAddedData(goods_id, &pack) > 0) {
												if(pack.GetExtStrData(GDSEXSTR_A, temp_buf) > 0)
													new_pack.SetExt(GDSEXSTR_A, temp_buf);
												if(pack.GetExtStrData(GDSEXSTR_B, temp_buf) > 0)
													new_pack.SetExt(GDSEXSTR_B, temp_buf);
												if(pack.GetExtStrData(GDSEXSTR_C, temp_buf) > 0)
													new_pack.SetExt(GDSEXSTR_C, temp_buf);
												if(pack.GetExtStrData(GDSEXSTR_D, temp_buf) > 0)
													new_pack.SetExt(GDSEXSTR_D, temp_buf);
												if(pack.GetExtStrData(GDSEXSTR_E, temp_buf) > 0)
													new_pack.SetExt(GDSEXSTR_E, temp_buf);
											}
										}
										barcode.CopyTo(new_pack.SingleBarcode, sizeof(new_pack.SingleBarcode));
										{
											GoodsStockExt gse;
											if(GObj.GetStockExt(goods_id, &gse, 1) > 0)
												new_pack.Package = gse.Package;
										}
										long   new_goods_id = 0;
										if(uc.CreateGoods(&new_goods_id, new_pack) > 0) {
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSCR, &msg_buf, goods_id, barcode.cptr()));
											uc.GetGoodsByCode(bc_list.at(m).Code, uhtt_goods_list);
											if(uhtt_goods_list.getCount() == 1)
												uhtt_goods_id = uhtt_goods_list.at(0)->ID;
											else if(uhtt_goods_list.getCount() > 1)
												uhtt_goods_id = uhtt_goods_list.at(0)->ID;
										}
										else
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSCRFAULT, &msg_buf, goods_id, barcode.cptr(), uc.GetLastMessage().cptr()));
										if(one_iter)
											break;
									}
								}
							}
							if(uhtt_goods_id) {
								if(goods_rec.Flags & GF_HASIMAGES) {
									lf.Load(goods_id, 0L);
									lf.At(0, img_path.Z());
									if(img_path.NotEmptyS()) {
										if(uc.SetObjImage("GOODS", uhtt_goods_id, img_path)) {
											// PPTXT_LOG_UHTT_GOODSSETIMG "Для товара @goods экспортировано изображение"
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSSETIMG, &msg_buf, goods_id));
										}
										else {
											// PPTXT_LOG_UHTT_GOODSSETIMGFAULT "Ошибка экспорта изображения для товара @goods: @zstr"
											logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSSETIMGFAULT, &msg_buf, goods_id, uc.GetLastMessage().cptr()));
										}
									}
								}
								else {
									// PPTXT_LOG_UHTT_GOODSNOIMG "Для товара @goods нет изображения"
									logger.Log(PPFormatT(PPTXT_LOG_UHTT_GOODSNOIMG, &msg_buf, goods_id));
								}
							}
						}
					}
					PPWaitPercent(i, uniq_id_list.getCount());
				}
				PPWait(0);
				break;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

PPViewGoods::IterOrder SLAPI PPViewGoods::GetIterOrder() const
{
	return OrdByName;
}

int SLAPI PPViewGoods::CreateTempTable(IterOrder ord, TempOrderTbl ** ppTbl)
{
	int    ok = 1;
	TempOrderTbl * p_o = 0;
	*ppTbl = 0;
	if(IsTempTblNeeded() || ord != OrdByDefault) {
		THROW(p_o = CreateTempOrderFile());
		{
			GoodsFilt flt = Filt;
			GoodsIterator gi;
			Goods2Tbl::Rec goods_rec;
			BExtInsert bei(p_o);
			PPTransaction tra(ppDbDependTransaction, 1);
			THROW(tra);
			for(gi.Init(&flt, 0); gi.Next(&goods_rec) > 0;) {
				TempOrderTbl::Rec rec;
				MakeTempRec(&goods_rec, &rec);
				THROW_DB(bei.insert(&rec));
				PPWaitPercent(gi.GetIterCounter());
			}
			THROW_DB(bei.flash());
			THROW(tra.Commit());
			*ppTbl = p_o;
			p_o = 0;
		}
	}
	else
		ok = -1;
	CATCHZOK
	delete p_o;
	return ok;
}

int SLAPI PPViewGoods::ChangeOrder(PPViewBrowser * pW)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GOODSORD);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->AddClusterAssocDef(CTL_GOODSORD_ORDER,  0, OrdByName);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  1, OrdByAbbr);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  2, OrdByGrp_Name);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  3, OrdByGrp_Abbr);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  4, OrdByBarcode);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  5, OrdByBrand_Name);
		dlg->AddClusterAssoc(CTL_GOODSORD_ORDER,  6, OrdByBarcode_Name);
		dlg->SetClusterData(CTL_GOODSORD_ORDER, Filt.InitOrder);
		if(ExecView(dlg) == cmOK) {
			long   ord = 0;
			dlg->GetClusterData(CTL_GOODSORD_ORDER, &ord);
			if(ord != Filt.InitOrder) {
				Filt.InitOrder = ord;
				ChangeFilt(1, pW);
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
//
//
//
int SLAPI PPViewGoods::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2) {
		const  PPConfig & r_cfg = LConfig;
		PPID   id = pHdr ? *static_cast<const PPID *>(pHdr) : 0;
		PPID   temp_id;
		switch(ppvCmd) {
			case PPVCMD_INPUTCHAR:
				{
					char c = *static_cast<const char *>(pHdr);
					if(isdec(c)) {
						int    r = 0;
						Goods2Tbl::Rec goods_rec;
						double qtty = 0.0;
						if((r = GObj.SelectGoodsByBarcode(c, NZOR(Filt.CodeArID, Filt.SupplID), &goods_rec, &qtty, 0)) > 0) {
							CALLPTRMEMB(pBrw, search2(&goods_rec.ID, CMPF_LONG, srchFirst, 0));
						}
						else if(r != -1 && pBrw)
							pBrw->bottom();
						ok = -1;
					}
					else
						ok = -2;
				}
				break;
			case PPVCMD_EDITITEM:
				temp_id = (Filt.GrpID > 0 && !(Filt.Flags & GoodsFilt::fGenGoods)) ? Filt.GrpID : 0;
				ok = (id && GObj.Edit(&id, reinterpret_cast<void *>(temp_id)) == cmOK) ? 1 : -1;
				if(ok > 0)
					UpdateTempTable(id, pBrw);
				break;
			case PPVCMD_ADDITEM:
				ok = -1;
				{
					GoodsListDialog * p_dlg = 0;
					do {
						if(AddItem(&p_dlg, pBrw, &(temp_id = 0)) > 0) {
							UpdateTempTable(temp_id, pBrw);
							pBrw->Update();
							pBrw->search2(&temp_id, CMPF_LONG, srchFirst, 0);
							ok = 1;
						}
					} while(p_dlg != 0);
				}
				break;
			case PPVCMD_ADDBYSAMPLE:
				ok = -1;
				if(id && GObj.AddBySample(&(temp_id = 0), id) == cmOK) {
					UpdateTempTable(temp_id, pBrw);
					pBrw->Update();
					pBrw->search2(&temp_id, CMPF_LONG, srchFirst, 0);
					ok = 1;
				}
				break;
			case PPVCMD_DELETEITEM:
				ok = DeleteItem(id);
				if(ok > 0)
					UpdateTempTable(id, pBrw);
				break;
			case PPVCMD_DELETEALL:
				ok = RemoveAll();
				if(ok > 0)
					UpdateTempTable(0, pBrw);
				break;
			case PPVCMD_UNITEOBJ:
				{
					PPObjGoods::ExtUniteBlock eub;
					eub.ResultID = id;
					if(Filt.Flags & GoodsFilt::fGenGoods && Filt.GrpID) {
						Goods2Tbl::Rec gen_rec;
						if(GObj.Fetch(Filt.GrpID, &gen_rec) > 0 && gen_rec.Flags & GF_GENERIC && gen_rec.Kind == PPGDSK_GOODS) {
							PPIDArray gen_list;
							GObj.GetGenericList(gen_rec.ID, &gen_list);
							if(gen_list.getCount() > 2 && gen_list.lsearch(id)) {
								gen_list.freeByKey(id, 0);
								eub.DestList = gen_list;
							}
						}
					}
					ok = GObj.ReplaceGoods(eub);
					if(ok > 0)
						UpdateTempTable(0, pBrw);
				}
				break;
			case PPVCMD_TAGS:
				ok = id ? EditObjTagValList(PPOBJ_GOODS, id, 0) : -1;
				break;
			case PPVCMD_TAGSALL:
				ok = -1;
				{
					const PPID obj_type = PPOBJ_GOODS;
					int    update_mode = ObjTagList::mumAdd;
					ObjTagList common_tag_list;
					common_tag_list.ObjType = obj_type;
					if(EditObjTagValUpdateList(&common_tag_list, 0, &update_mode) > 0 && common_tag_list.GetCount()) {
						GoodsViewItem item;
						ObjTagCore & r_ot = PPRef->Ot;
						PPTransaction tra(1);
						THROW(tra);
						for(InitIteration(PPViewGoods::OrdByDefault); NextIteration(&item) > 0;) {
							ObjTagList local_tag_list;
							THROW(r_ot.GetList(obj_type, item.ID, &local_tag_list));
							if(local_tag_list.Merge(common_tag_list, update_mode) > 0) {
								THROW(r_ot.PutList(obj_type, item.ID, &local_tag_list, 0));
							}
							PPWaitPercent(GetCounter());
						}
						THROW(tra.Commit());
					}
				}
				break;
			case PPVCMD_ADDTOBASKET:
				ok = -1;
				AddGoodsToBasket(id, 0, 1, 0);
				break;
			case PPVCMD_ADDFROMBASKET:
				ok = AddGoodsFromBasket();
				if(ok > 0)
					UpdateTempTable(0, pBrw);
				break;
			case PPVCMD_VIEWLOTS:
				ok = -1;
				if(id)
					if(GObj.CheckFlag(id, GF_UNLIM))
						GObj.EditQuotations(id, 0, -1L/*@curID*/, 0, PPQuot::clsGeneral);
					else
						::ViewLots(id, 0, Filt.SupplID, 0, 1);
				break;
			case PPVCMD_GOODSSTRUCT:
				ok = -1;
				if(id)
					GObj.EditGoodsStruc(id);
				break;
			// @v10.6.8 {
			case PPVCMD_SIMILARITY:
				ok = -1;
				if(id && pBrw && pBrw->getDefC()) {
					int col_idx = pBrw->GetCurColumn();
					if(col_idx >= 0 && col_idx < pBrw->getDefC()->getCountI()) {
						const BroColumn & r_col = pBrw->getDefC()->at(col_idx);
						Goods2Tbl::Rec goods_rec;
						if(r_col.OrgOffs == 15) { // brand
							if(GObj.Fetch(id, &goods_rec) > 0 && goods_rec.BrandID) {
								GoodsFilt temp_filt;
								temp_filt.BrandList.Add(goods_rec.BrandID);
								SETFLAGBYSAMPLE(temp_filt.Flags, GoodsFilt::fShowBarcode, Filt.Flags);
								return PPView::Execute(PPVIEW_GOODS, &temp_filt, 0, 0);
							}
						}
						else if(r_col.OrgOffs == 16) { // group
							if(GObj.Fetch(id, &goods_rec) > 0 && goods_rec.ParentID) {
								GoodsFilt temp_filt;
								temp_filt.GrpIDList.Add(goods_rec.ParentID);
								SETFLAGBYSAMPLE(temp_filt.Flags, GoodsFilt::fShowBarcode, Filt.Flags);
								return PPView::Execute(PPVIEW_GOODS, &temp_filt, 0, 0);
							}
						}
						else if(r_col.OrgOffs == 3) { // manuf
							if(GObj.Fetch(id, &goods_rec) > 0 && goods_rec.ManufID) {
								GoodsFilt temp_filt;
								temp_filt.ManufID = goods_rec.ManufID;
								SETFLAGBYSAMPLE(temp_filt.Flags, GoodsFilt::fShowBarcode, Filt.Flags);
								return PPView::Execute(PPVIEW_GOODS, &temp_filt, 0, 0);
							}
						}
						else if(r_col.OrgOffs == 7) { // code
							SString barcode;
							pBrw->getDef()->getFullText(pHdr, col_idx, barcode);
							if(barcode.NotEmptyS()) {
								if(barcode.Len() >= 12)
									barcode.Trim(8);
								else if(barcode.Len() >= 8)
									barcode.Trim(5);
								else
									barcode.Z();
								if(barcode.NotEmpty()) {
									GoodsFilt temp_filt;
									temp_filt.PutExtssData(GoodsFilt::extssBarcodeText, barcode);
									SETFLAGBYSAMPLE(temp_filt.Flags, GoodsFilt::fShowBarcode, Filt.Flags);
									return PPView::Execute(PPVIEW_GOODS, &temp_filt, 0, 0);									
								}
							}
						}
					}
				}
				break;
			// } @v10.6.8
			case PPVCMD_ADDEDFIELDS:
				ok = GObj.EditVad(id);
				break;
			case PPVCMD_EDITARCODES:
				ok = GObj.EditArCode(id, Filt.CodeArID, BIN(Filt.Flags & GoodsFilt::fShowOwnArCode));
				break;
			case PPVCMD_MOVARCODES:
				ok = -1;
				if(Filt.Flags & GoodsFilt::fShowArCode && Filt.CodeArID) {
					if(GObj.CheckRights(GOODSRT_MULTUPD)) {
						PPID   src_ar_id = Filt.CodeArID; // GoodsFilt
						PPID   grp_id = NZOR(Filt.GrpID, Filt.GrpIDList.GetSingle());
						PPObjArticle ar_obj;
						ArticleTbl::Rec ar_rec;
						// @v10.6.4 MEMSZERO(ar_rec);
						const PPID acs_id = (ar_obj.Fetch(src_ar_id, &ar_rec) > 0) ? ar_rec.AccSheetID : GetSupplAccSheet();
						if(acs_id) {
							TDialog * dlg = new TDialog(DLG_MOVARCODE);
							if(CheckDialogPtrErr(&dlg)) {
								SString temp_buf;
								PPID   dest_ar_id = 0;
								SetupArCombo(dlg, CTLSEL_MOVARCODE_DESTAR, dest_ar_id, 0, acs_id, sacfDisableIfZeroSheet);
								dlg->setCtrlData(CTL_MOVARCODE_SRCAR, ar_rec.Name);
								dlg->setCtrlString(CTL_MOVARCODE_GGRP, GetGoodsName(grp_id, temp_buf));
								if(ExecView(dlg) == cmOK) {
									dest_ar_id = dlg->getCtrlLong(CTLSEL_MOVARCODE_DESTAR);
									if(dest_ar_id) {
										PPLogger logger;
										if(!GObj.P_Tbl->MoveArCodes(dest_ar_id, src_ar_id, grp_id, &logger, 1))
											PPError();
										else {
											ok = 1;
										}
									}
								}
							}
							delete dlg;
						}
					}
					else
						PPError();
				}
				break;
			case PPVCMD_VIEWQUOT:
				ok = -1;
				GObj.EditQuotations(id, r_cfg.Location, -1L/*@curID*/, 0, PPQuot::clsGeneral);
				break;
			case PPVCMD_VIEWSUPPLQUOT:
				GObj.EditQuotations(id, r_cfg.Location, -1L/*@curID*/, 0, PPQuot::clsSupplDeal);
				break;
			case PPVCMD_VIEWGOODSMATRIX:
				GObj.EditQuotations(id, r_cfg.Location, -1L/*@curID*/, 0, PPQuot::clsMtx);
				break;
			case PPVCMD_VIEWGOODSAGGR:
				ok = (GObj.ShowGoodsAsscInfo(id) > 0) ? 1 : -1;
				break;
			case PPVCMD_VIEWTECH:
				ok = -1;
				if(id) {
					TechFilt filt;
					filt.GoodsID = id;
					::ViewTech(&filt);
				}
				break;
			case PPVCMD_VIEWTSESS:
				ok = -1;
				if(id) {
					TSessLineFilt filt(0, id, 0);
					::ViewTSessLine(&filt);
				}
				break;
			case PPVCMD_POSPRINT:
				ok = -1;
				PrintPLabel(id);
				break;
			case PPVCMD_PRINTLABEL:
				ok = -1;
				if(id)
					BarcodeLabelPrinter::PrintGoodsLabel(id);
				break;
			case PPVCMD_EXPORT:
				ok = -1;
				Export(0);
				break;
			case PPVCMD_EXPORTUHTT:
				ok = -1;
				ExportUhtt();
				break;
			case PPVCMD_TRANSMIT:
				ok = -1;
				Transmit(id);
				break;
			case PPVCMD_DORECOVER:
				ok = Repair(id);
				break;
			case PPVCMD_CHNGFLAGS:
				ok = UpdateFlags();
				break;
			case PPVCMD_REPLACEMENT:
				ok = ReplaceNames();
				if(ok > 0)
					UpdateTempTable(0, pBrw);
				break;
			case PPVCMD_SYSJ:
				if(id) {
					ViewSysJournal(PPOBJ_GOODS, id, 0);
					ok = -1;
				}
				break;
			case PPVCMD_SORT:
				ok = ChangeOrder(pBrw);
				break;
			case PPVCMD_TB_CBX_SELECTED:
				{
					ok = -1;
					PPID   ggrp_id = 0;
					if(pBrw && pBrw->GetToolbarComboData(&ggrp_id) && Filt.GrpID != ggrp_id) {
						Filt.GrpID = ggrp_id;
						Filt.GrpIDList.Z();
						ok = ChangeFilt(1, pBrw);
					}
				}
				break;
			case PPVCMD_MOUSEHOVER:
				if(HasImages(pHdr)) {
					SString img_path;
					ObjLinkFiles link_files(PPOBJ_GOODS);
					link_files.Load(id, 0L);
					link_files.At(0, img_path);
					PPTooltipMessage(0, img_path, pBrw->H(), 10000, 0, SMessageWindow::fShowOnCursor|SMessageWindow::fCloseOnMouseLeave|
						SMessageWindow::fOpaque|SMessageWindow::fSizeByText|SMessageWindow::fChildWindow);
				}
				break;
			/*
			@todo реализовать вызов AddBarcodeCheckDigit()
			Ранее вызывалась по <Ctrl-X Ctrl-A>
				AddBarcodeCheckDigit();
			*/
			//case PPVCMD_DETAIL:
			//case PPVCMD_PRINT         :
			//case PPVCMD_SYSJ          :
			//case PPVCMD_TOTAL         :
		}
	}
	CATCHZOKPPERR
	return ok;
}

#if 0 // {
	// Test of function CreatePrintableBarcode
	{
		GoodsViewItem item;
		PPObjGoods g_obj;
		FILE * f_out = fopen("barcode.", "w");
		PPWait(1);
		if(f_out) {
			for(P_View->InitIteration(); P_View->NextIteration(&item) > 0;) {
				char bc[32], pbc[32];
				if(g_obj.GetSingleBarcode(item.ID, bc, sizeof(bc)) > 0) {
					if(!CreatePrintableBarcode(bc, pbc, sizeof(pbc)))
						strcpy(pbc, "XXXXXXXXXXXXX");
					fprintf(f_out, "%s %s %s\n", pbc, bc, item.Name);
				}
				PPWaitPercent(P_View->GetCounter());
			}
			fclose(f_out);
		}
		PPWait(0);
	}
#endif // }
//
// Implementation of PPALDD_GoodsBasket
//
PPALDD_CONSTRUCTOR(GoodsBasket)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsBasket) { Destroy(); }

int PPALDD_GoodsBasket::InitData(PPFilt & rFilt, long rsrv)
{
	//int uncertainty = 0;
	PPViewGoodsBasket * p_v = 0;
	if(rsrv) {
		p_v = static_cast<PPViewGoodsBasket *>(rFilt.Ptr);
		Extra[1].Ptr = p_v;
	}
	else {
		PPObjGoodsBasket gb_obj;
		PPBasketPacket packet;
		gb_obj.GetPacket(rFilt.ID, &packet);
		p_v = new PPViewGoodsBasket(&packet);
		Extra[0].Ptr = p_v;
	}
	const PPBasketPacket * p_packet = p_v->GetPacket();
	H.ID = p_packet->Head.ID;
	STRNSCPY(H.Name, p_packet->Head.Name);
	H.Flags = p_packet->Head.Flags;
	H.ItemsCount = p_packet->Lots.getCount();
	ILTI * p_item = 0;
	double sum_price = 0.0;
	for(uint i = 0; p_packet->Lots.enumItems(&i, (void **)&p_item);)
		sum_price += (p_item->Price * p_item->Quantity);
	H.EstPrice = sum_price;
	H.FltSupplID = p_packet->Head.SupplID;
	{
		SString str_phones;
		PPObjPerson    psn_obj;
		PPPersonPacket psn_pack;
		PPID psn_id = ObjectToPerson(p_packet->Head.SupplID);
		if(psn_obj.GetPacket(psn_id, &psn_pack, 0) > 0 && psn_pack.GetPhones(2, str_phones) > 0)
			str_phones.CopyTo(H.Phones, sizeof(H.Phones));
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsBasket::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	PPViewGoodsBasket * p_v = static_cast<PPViewGoodsBasket *>(NZOR(Extra[1].Ptr, Extra[0].Ptr));
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	p_v->Init(SortIdx);
	p_v->InitIteration();
	I.LineNo = 0;
	return 1;
}

int PPALDD_GoodsBasket::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PPViewGoodsBasket * p_v = static_cast<PPViewGoodsBasket *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);
	uint   n = (uint)I.LineNo;
	ILTI   item;
	if(p_v->NextIteration(&item) > 0) {
		I.LineNo = n;
		I.ItemID = item.GoodsID;
		I.Flags  = item.Flags;
		I.Price  = item.Price;
		I.Quantity = item.Quantity;
		QttyToStr(item.Quantity, item.UnitPerPack, ((LConfig.Flags & CFGFLG_USEPACKAGE) ?
			MKSFMT(0, QTTYF_COMPLPACK | QTTYF_FRACTION) : QTTYF_FRACTION), I.CQtty);
		I.SumPerUnit = item.Price * item.Quantity;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_GoodsBasket::Destroy() { DESTROY_PPVIEW_ALDD(GoodsBasket); }
//
// Implementation of PPALDD_GoodsView
//
PPALDD_CONSTRUCTOR(GoodsView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsView) { Destroy(); }

int PPALDD_GoodsView::InitData(PPFilt & rFilt, long rsrv)
{
	INIT_PPVIEW_ALDD_DATA_U(Goods, rsrv);
	H.FltGrpID    = p_filt->GrpID;
	H.FltManufID  = p_filt->ManufID;
	H.FltUnitID   = p_filt->UnitID;
	H.FltPhUnitID = p_filt->PhUnitID;
	H.FltGoodsTypeID = p_filt->GoodsTypeID;
	H.FltTaxGrpID = p_filt->TaxGrpID;
	H.FltBrandID  = p_filt->GetResultBrandList().GetSingle();
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsView::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	INIT_PPVIEW_ALDD_ITER(Goods);
}

int PPALDD_GoodsView::NextIteration(PPIterID iterId)
{
	START_PPVIEW_ALDD_ITER(Goods);
	I.GoodsID = item.ID;
	STRNSCPY(I.BarCode, item.Barcode);
	STRNSCPY(I.StrucType, item.StrucType);
	PPWaitPercent(p_v->GetCounter());
	FINISH_PPVIEW_ALDD_ITER();
}

void PPALDD_GoodsView::Destroy() { DESTROY_PPVIEW_ALDD(Goods); }
//
// Implementation of PPALDD_GoodsStruc
//
PPALDD_CONSTRUCTOR(GoodsStruc)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsStruc) { Destroy(); }

struct Dl6_GoodsStruc_Support {
	explicit SLAPI  Dl6_GoodsStruc_Support(PPFilt & rFilt) : P_Iter(0), IsOwnPtr(0)
	{
		if(rFilt.Ptr)
			P_Iter = static_cast<GStrucIterator *>(rFilt.Ptr);
		else if(rFilt.ID) {
			PPObjGoodsStruc gs_obj;
			PPGoodsStruc gs;
			if(gs_obj.Get(rFilt.ID, &gs) > 0) {
				P_Iter = new GStrucIterator;
				if(P_Iter) {
					IsOwnPtr = 1;
					P_Iter->Init(&gs, 0);
				}
			}
		}
	}
	SLAPI ~Dl6_GoodsStruc_Support()
	{
		if(IsOwnPtr)
			delete P_Iter;
	}
	GStrucIterator * P_Iter;
	int    IsOwnPtr;
};

int PPALDD_GoodsStruc::InitData(PPFilt & rFilt, long rsrv)
{
	int    uncertainty = 0;
	Dl6_GoodsStruc_Support * p_supp = new Dl6_GoodsStruc_Support(rFilt);
	if(p_supp) {
		Extra[1].Ptr = p_supp;
		const PPGoodsStruc * p_gs = p_supp->P_Iter ? p_supp->P_Iter->GetStruc() : 0;
		if(p_gs) {
			H.ID      = p_gs->Rec.ID;
			H.GoodsID = p_gs->GoodsID;
			STRNSCPY(H.Name, p_gs->Rec.Name);
			H.Flags  = p_gs->Rec.Flags;
			H.fAllowCompl   = BIN(p_gs->Rec.Flags & GSF_COMPL);
			H.fAllowDecompl = BIN(p_gs->Rec.Flags & GSF_DECOMPL);
			H.fPartitial    = BIN(p_gs->Rec.Flags & GSF_PARTITIAL);
			H.fNamed        = BIN(p_gs->Rec.Flags & GSF_NAMED);
			H.LocID = LConfig.Location;
			H.VariedPropObjType = p_gs->Rec.VariedPropObjType;
			H.CommDenom = p_gs->Rec.CommDenom;
			p_gs->CalcEstimationPrice(&H.EstPrice, &uncertainty, 1);
			H.fUncertainPrice = uncertainty;
			H.ItemsCount = p_gs->Items.getCount();
		}
	}
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsStruc::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	int    ok = -1;
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	I.LineNo = 0;
	Dl6_GoodsStruc_Support * p_supp = static_cast<Dl6_GoodsStruc_Support *>(Extra[1].Ptr);
	GStrucIterator * p_gs_iter = p_supp ? p_supp->P_Iter : 0;
	if(p_gs_iter) {
		p_gs_iter->InitIteration();
		ok = 1;
	}
	return ok;
}

int PPALDD_GoodsStruc::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	{
		GStrucRecurItem gsr_item;
		Dl6_GoodsStruc_Support * p_supp = static_cast<Dl6_GoodsStruc_Support *>(Extra[1].Ptr);
		GStrucIterator * p_gs_iter = p_supp ? p_supp->P_Iter : 0;
		if(p_gs_iter && p_gs_iter->NextIteration(&gsr_item) > 0) {
			PPGoodsStrucItem item = gsr_item.Item;
			SString temp_buf;
			MEMSZERO(I);
			I.LineNo  = gsr_item.Level;
			I.ItemID  = item.GoodsID;
			I.Flags   = item.Flags;
			I.Median  = item.Median;
			I.PerUnit = item.Denom;
			I.Netto   = item.Netto;

			I.fPercentVal = BIN(item.Flags & GSIF_PCTVAL);
			I.fRoundDown  = BIN(item.Flags & GSIF_ROUNDDOWN);
			I.fPhUnitVal  = BIN(item.Flags & GSIF_PHUVAL);
			I.fUncertainPrice = BIN(item.Flags & GSIF_UNCERTPRICE);
			I.LotID = gsr_item.LastLotID;
			I.Price = gsr_item.Price;
			I.SumPerUnit = gsr_item.Sum;
			item.GetEstimationString(temp_buf);
			if(item.Flags & GSIF_PCTVAL)
				temp_buf.CatChar('%');
			else if(item.Flags & GSIF_PHUVAL)
				temp_buf.CatChar('p').CatChar('h');
			STRNSCPY(I.CMedian, temp_buf);
		}
		else
			return -1;
	}
	return DlRtm::NextIteration(iterId);
}

void PPALDD_GoodsStruc::Destroy()
{
	Dl6_GoodsStruc_Support * p_supp = static_cast<Dl6_GoodsStruc_Support *>(Extra[1].Ptr);
	delete p_supp;
	Extra[0].Ptr = Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_QUOTKIND
//
PPALDD_CONSTRUCTOR(QuotKind)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(QuotKind) { Destroy(); }

int PPALDD_QuotKind::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPQuotKind rec;
		if(SearchObject(PPOBJ_QUOTKIND, rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			H.OpID = rec.OpID;
			H.Discount = rec.Discount;
			H.fAbsDiscount = BIN(rec.Flags & QUOTKF_ABSDIS);
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Symb, rec.Symb);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_GoodsGroup
//
PPALDD_CONSTRUCTOR(GoodsGroup)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjGoods;
	}
}

PPALDD_DESTRUCTOR(GoodsGroup)
{
	Destroy();
	delete static_cast<PPObjGoods *>(Extra[0].Ptr);
}

int PPALDD_GoodsGroup::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		Goods2Tbl::Rec rec;
		PPObjGoods * p_goods_obj = static_cast<PPObjGoods *>(Extra[0].Ptr);
		if(p_goods_obj->Fetch(rFilt.ID, &rec) > 0) {
			H.ID       = rec.ID;
			H.UnitID   = rec.UnitID;
			H.PhUnitID = rec.PhUnitID;
			H.TypeID   = rec.GoodsTypeID;
			H.TaxGrpID = rec.TaxGrpID;
			H.ParentID = rec.ParentID;
			H.ClsID    = rec.GdsClsID;
			H.Flags    = rec.Flags;
			STRNSCPY(H.Name, rec.Name);
			if(rec.ParentID) {
				SString full_name;
				p_goods_obj->P_Tbl->MakeFullName(rec.ID, 0, full_name);
				full_name.CopyTo(H.FullName, sizeof(H.FullName));
			}
			else
				STRNSCPY(H.FullName, rec.Name);
			p_goods_obj->GetSingleBarcode(rec.ID, H.Code, sizeof(H.Code));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_GoodsGroup::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	#define _RET_LONG    (*static_cast<long *>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetFilt") {
		long   sur_id = 0;
		GoodsFilt filt;
		PPObjGoodsGroup gg_obj;
		if(gg_obj.ReadGoodsFilt(H.ID, &filt) > 0) {
			SBuffer buffer;
			SSerializeContext sctx;
			if(filt.Serialize(+1, buffer, &sctx)) {
				DS.GetTLA().SurIdList.Add(&sur_id, buffer.GetBuf(0), buffer.GetAvailableSize());
			}
		}
		_RET_LONG = sur_id;
	}
	// @v10.2.0 {
	else if(pF->Name == "?GetFullName") {
		PPObjGoods goods_obj;
		goods_obj.P_Tbl->MakeFullName(H.ID, 0, _RET_STR);
	}
	// } @v10.2.0
}
//
// Implementation of PPALDD_Goods
//
struct DL600_GoodsBlock {
    PPObjGoods Obj;
    PPGoodsPacket Pack;
};

PPALDD_CONSTRUCTOR(Goods)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new DL600_GoodsBlock;
	}
}

PPALDD_DESTRUCTOR(Goods)
{
	Destroy();
	delete static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
}

int PPALDD_Goods::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk->Obj.GetPacket(rFilt.ID, &p_blk->Pack, PPObjGoods::gpoSkipQuot) > 0) { // @v8.3.7 PPObjGoods::gpoSkipQuot
			SString temp_buf;
			H.ID        = p_blk->Pack.Rec.ID;
			H.UnitID    = p_blk->Pack.Rec.UnitID;
			H.PhUnitID  = p_blk->Pack.Rec.PhUnitID;
			H.PhPerUnit = p_blk->Pack.Rec.PhUPerU;
			H.GroupID   = p_blk->Pack.Rec.ParentID;
			H.TypeID    = p_blk->Pack.Rec.GoodsTypeID;
			H.ClsID     = p_blk->Pack.Rec.GdsClsID;
			H.ManufID   = p_blk->Pack.Rec.ManufID;
			H.TaxGrpID  = p_blk->Pack.Rec.TaxGrpID;
			H.WrOffGrpID = p_blk->Pack.Rec.WrOffGrpID;
			H.BrandID   = p_blk->Pack.Rec.BrandID;
			H.Snl       = p_blk->Pack.Rec.ID;
			H.Flags     = p_blk->Pack.Rec.Flags;
			H.Kind      = p_blk->Pack.Rec.Kind;
			p_blk->Pack.Codes.GetSingle(temp_buf); temp_buf.CopyTo(H.SingleBarCode, sizeof(H.SingleBarCode));
			STRNSCPY(H.Name, p_blk->Pack.Rec.Name);

			H.Brutto = p_blk->Pack.Stock.Brutto;
			H.Length = p_blk->Pack.Stock.PckgDim.Length;   // Габаритная длина упаковки поставки,  мм
			H.Width  = p_blk->Pack.Stock.PckgDim.Width;    // Габаритная ширина упаковки поставки, мм
			H.Height = p_blk->Pack.Stock.PckgDim.Height;   // Габаритная высота упаковки поставки, мм
			H.MinStock = p_blk->Pack.Stock.GetMinStock(0); // Минимальный запас товара
			H.Package  = p_blk->Pack.Stock.Package;        // Емкость упаковки поставки (торговых единиц)
			H.ExpiryPeriod = p_blk->Pack.Stock.ExpiryPeriod; // Срок годности товара (дней).
			H.GseFlags = p_blk->Pack.Stock.GseFlags;
			H.MinShippmQtty = p_blk->Pack.Stock.MinShippmQtty;

			p_blk->Pack.GetExtStrData(GDSEXSTR_STORAGE,   temp_buf); temp_buf.CopyTo(H.Storage,   sizeof(H.Storage));
			p_blk->Pack.GetExtStrData(GDSEXSTR_STANDARD,  temp_buf); temp_buf.CopyTo(H.Standard,  sizeof(H.Standard));
			p_blk->Pack.GetExtStrData(GDSEXSTR_LABELNAME, temp_buf); temp_buf.CopyTo(H.LabelName, sizeof(H.LabelName));
			p_blk->Pack.GetExtStrData(GDSEXSTR_INGRED,    temp_buf); temp_buf.CopyTo(H.Ingred, sizeof(H.Ingred));
			p_blk->Pack.GetExtStrData(GDSEXSTR_ENERGY,    temp_buf); temp_buf.CopyTo(H.Energy, sizeof(H.Energy));
			p_blk->Pack.GetExtStrData(GDSEXSTR_USAGE,     temp_buf); temp_buf.CopyTo(H.Usage,  sizeof(H.Usage));
			p_blk->Pack.GetExtStrData(GDSEXSTR_OKOF,      temp_buf); temp_buf.CopyTo(H.OKOF,   sizeof(H.OKOF));
			if(p_blk->Pack.Rec.GdsClsID) {
				PPObjGoodsClass gc_obj;
				PPGdsClsPacket gc_pack;
				if(gc_obj.Fetch(p_blk->Pack.Rec.GdsClsID, &gc_pack) > 0) {
					gc_pack.GetExtDim(&p_blk->Pack.ExtRec, PPGdsCls::eX, &H.DimX);
					gc_pack.GetExtDim(&p_blk->Pack.ExtRec, PPGdsCls::eY, &H.DimY);
					gc_pack.GetExtDim(&p_blk->Pack.ExtRec, PPGdsCls::eZ, &H.DimZ);
					gc_pack.GetExtDim(&p_blk->Pack.ExtRec, PPGdsCls::eW, &H.DimW);
					gc_pack.GetExtProp(&p_blk->Pack.ExtRec, PPGdsCls::eKind, &H.KindID, temp_buf);    temp_buf.CopyTo(H.KindText, sizeof(H.KindText));
					gc_pack.GetExtProp(&p_blk->Pack.ExtRec, PPGdsCls::eGrade, &H.GradeID, temp_buf);  temp_buf.CopyTo(H.GradeText, sizeof(H.GradeText));
					gc_pack.GetExtProp(&p_blk->Pack.ExtRec, PPGdsCls::eAdd, &H.AddObjID, temp_buf);   temp_buf.CopyTo(H.AddObjText, sizeof(H.AddObjText));
					gc_pack.GetExtProp(&p_blk->Pack.ExtRec, PPGdsCls::eAdd2, &H.AddObj2ID, temp_buf); temp_buf.CopyTo(H.AddObj2Text, sizeof(H.AddObj2Text));
				}
			}
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

void PPALDD_Goods::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_STR(n)  (**static_cast<const SString **>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_LONG(n) (*static_cast<const long *>(rS.GetPtr(pApl->Get(n))))
	#define _ARG_DT(n)   (*static_cast<const LDATE *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	#define _RET_LONG    (*static_cast<long *>(rS.GetPtr(pApl->Get(0))))
	#define _RET_DBL     (*static_cast<double *>(rS.GetPtr(pApl->Get(0))))

	PPObjBill * p_bobj = BillObj;
	ReceiptTbl::Rec lot_rec;
	if(pF->Name == "?GetArCode") {
		_RET_STR.Z();
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		PPObjGoods * p_obj = static_cast<PPObjGoods *>(Extra[0].Ptr);
		if(p_blk)
			p_blk->Obj.P_Tbl->GetArCode(_ARG_LONG(1), H.ID, _RET_STR, 0);
	}
	else if(pF->Name == "?GetAbbr") {
		_RET_STR.Z();
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk && p_blk->Pack.Rec.ID) {
			_RET_STR = p_blk->Pack.Rec.Abbr;
		}
	}
	else if(pF->Name == "?GetLastLot") {
		if(p_bobj->trfr->Rcpt.GetLastLot(H.ID, _ARG_LONG(1), MAXDATE, &lot_rec) > 0)
			_RET_LONG = lot_rec.ID;
		else
			_RET_LONG = 0;
	}
	else if(pF->Name == "?GetLastLotForDate") {
		if(p_bobj->trfr->Rcpt.GetLastLot(H.ID, _ARG_LONG(1), _ARG_DT(2), &lot_rec) > 0)
			_RET_LONG = lot_rec.ID;
		else
			_RET_LONG = 0;
	}
	else if(pF->Name == "?GetAverageCost") { // @v10.2.6
		//double GetAverageCost(date dt, long locID, string serial);
		LDATE  dt = _ARG_DT(1);
		PPID   loc_id = _ARG_LONG(2);
		const  SString & r_serial = _ARG_STR(3);
		LotArray lot_list;
		p_bobj->trfr->Rcpt.GetList(H.ID, loc_id, 0, dt, 0/*opened only*/, 0/*non-zero rest only*/, &lot_list);
		double sum_cost = 0.0;
		double sum_rest = 0.0;
		SString temp_buf;
		for(uint i = 0; i < lot_list.getCount(); i++) {
			const ReceiptTbl::Rec & r_lot_rec = lot_list.at(i);
			if(r_serial.Empty() || (p_bobj->GetSerialNumberByLot(r_lot_rec.ID, temp_buf, 1) > 0 && temp_buf == r_serial)) {
				double rest = 0.0;
				p_bobj->trfr->GetRest(r_lot_rec.ID, dt, MAXLONG, &rest, 0);
				if(rest > 0.0) {
					sum_rest += rest;
					sum_cost += rest * r_lot_rec.Cost;
				}
				// @v10.4.0 {
				else {
					const double qtty = fabs(r_lot_rec.Quantity);
					sum_rest += qtty;
					sum_cost += qtty * r_lot_rec.Cost;
				}
				// } @v10.4.0
			}
		}
		_RET_DBL = (sum_rest > 0.0) ? (sum_cost / sum_rest) : 0.0;
	}
	else if(pF->Name == "?GetQuot") {
		//double GetQuot(string quotKindSymb[20], long locID, long arID, long curID);
		double quot = 0.0;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk) {
			PPObjQuotKind qk_obj;
			PPID   qk_id = 0;
			if(qk_obj.SearchSymb(&qk_id, _ARG_STR(1)) > 0) {
				PPID   loc_id = _ARG_LONG(2);
				const  QuotIdent qi(loc_id, qk_id, _ARG_LONG(4), _ARG_LONG(3));
				double cost = 0.0;
				double price = 0.0;
				if(p_bobj->trfr->Rcpt.GetLastLot(H.ID, loc_id, MAXDATE, &lot_rec) > 0) {
					cost = lot_rec.Cost;
					price = lot_rec.Price;
				}
				p_blk->Obj.GetQuotExt(H.ID, qi, cost, price, &quot, 1);
			}
		}
		_RET_DBL = quot;
	}
	else if(pF->Name == "?GetExclAltGroupMembership") {
		_RET_LONG = 0;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk) {
			PPObjGoodsGroup gg_obj;
			BarcodeTbl::Rec bc_rec;
			if(gg_obj.SearchCode(_ARG_STR(1), &bc_rec) > 0) {
				PPIDArray _grp_list;
				p_blk->Obj.P_Tbl->GetGroupTerminalList(bc_rec.GoodsID, &_grp_list, 0);
				for(uint i = 0; i < _grp_list.getCount(); i++) {
					if(p_blk->Obj.BelongToGroup(H.ID, _grp_list.get(i), 0) > 0) {
						_RET_LONG = _grp_list.get(i);
						break;
					}
				}
			}
		}
	}
	else if(pF->Name == "?GetAddedText") {
		_RET_STR.Z();
		const char * p_symb = _ARG_STR(1);
		if(!isempty(p_symb)) {
			DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
			if(p_blk && p_blk->Pack.Rec.ID) {
				switch(toupper(p_symb[0])) {
					case 'A': p_blk->Pack.GetExtStrData(GDSEXSTR_A, _RET_STR); break;
					case 'B': p_blk->Pack.GetExtStrData(GDSEXSTR_B, _RET_STR); break;
					case 'C': p_blk->Pack.GetExtStrData(GDSEXSTR_C, _RET_STR); break;
					case 'D': p_blk->Pack.GetExtStrData(GDSEXSTR_D, _RET_STR); break;
					case 'E': p_blk->Pack.GetExtStrData(GDSEXSTR_E, _RET_STR); break;
				}
			}
		}
	}
	else if(pF->Name == "?GetMinStock") {
		double val = 0.0;
		const PPID loc_id = _ARG_LONG(1);
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk && p_blk->Pack.Rec.ID) {
			val = p_blk->Pack.Stock.GetMinStock(loc_id, 1);
		}
		_RET_DBL = val;
	}
	else if(pF->Name == "?GetTag") {
		_RET_LONG = PPObjTag::Helper_GetTag(PPOBJ_GOODS, H.ID, _ARG_STR(1));
	}
	else if(pF->Name == "?GetManufCountryText") {
		_RET_STR.Z();
		long   k = _ARG_LONG(1);
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk) {
			PPID   country_id = 0;
			PPCountryBlock cblk;
			p_blk->Obj.GetManufCountry(H.ID, 0, &country_id, &cblk);
			if(k == 1)
				_RET_STR = cblk.Abbr;
			else if(k == 2)
				_RET_STR = cblk.Code;
			else
				_RET_STR = cblk.Name;
		}
	}
	else if(pF->Name == "?GetBarcodeList") {
		SString delim = _ARG_STR(1);
		SString temp_buf;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk && p_blk->Pack.Rec.ID) {
			const uint n = p_blk->Pack.Codes.getCount();
			for(uint i = 0; i < n; i++) {
				BarcodeTbl::Rec & r_bc_rec = p_blk->Pack.Codes.at(i);
				temp_buf.Cat(r_bc_rec.Code);
				if(i < n - 1)
					temp_buf.Cat(delim);
			}
		}
		_RET_STR = temp_buf;
	}
	// @v9.4.10 {
	else if(pF->Name == "?GetSingleEgaisCode") {
		SString temp_buf;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		int    found = 0;
		if(p_blk && p_blk->Pack.Rec.ID) {
			const uint n = p_blk->Pack.Codes.getCount();
			for(uint i = 0; !found && i < n; i++) {
				BarcodeTbl::Rec & r_bc_rec = p_blk->Pack.Codes.at(i);
				temp_buf = r_bc_rec.Code;
				if(temp_buf.Len() == 19)
					found = 1;
			}
		}
		if(!found)
			temp_buf.Z();
		_RET_STR = temp_buf;
	}
	// } @v9.4.10
	else if(pF->Name == "?GetImagePath") {
		_RET_STR.Z();
		//string GetImagePath[256](int rel, string stub[256]);
		const int rel = _ARG_LONG(1);
		SString stub = _ARG_STR(2);
		ObjLinkFiles lf(PPOBJ_GOODS);
		lf.Load(H.ID, 0L);
		SString org_file_path;
		if(lf.At(0, org_file_path) && fileExists(org_file_path)) {
		}
		else if(stub.NotEmptyS() && fileExists(stub))
			org_file_path = stub;
		else
			org_file_path = 0;
		if(org_file_path.NotEmpty()) {
            SString dest_file_path;
            SString dest_file_name;
            if(P_Ep && P_Ep->DestPath.NotEmpty())
                dest_file_path = P_Ep->DestPath;
            else
				PPGetPath(PPPATH_TEMP, dest_file_path);
            dest_file_path.SetLastSlash().Cat("img");
            ::createDir(dest_file_path);
            SPathStruc ps_dest(dest_file_path);
            SPathStruc ps_src(org_file_path);
            ps_dest.Nam = ps_src.Nam;
            /* @v9.1.1
            if(P_Ep && P_Ep->OutputFormat == SFileFormat::Latex)
				ps_dest.Ext.Z();
			else
				ps_dest.Ext = ps_src.Ext;
			*/
			ps_dest.Ext = ps_src.Ext; // @v9.1.1
            ps_dest.Merge(dest_file_name);
            SCopyFile(org_file_path, dest_file_name, 0, 0, 0);
            //
            // @v9.1.1 {
            // Для latex результирующее имя файла должно быть без расширения
            // (в то время как сам файл расширение иметь должен)
            //
            if(P_Ep && P_Ep->OutputFormat == SFileFormat::Latex) {
				ps_dest.Ext.Z();
				ps_dest.Merge(dest_file_name);
            }
            // }
            if(rel) {
				SPathStruc::GetRelativePath(dest_file_path, 0, dest_file_name, 0, stub);
            }
            else
				stub = dest_file_name;
            SPathStruc::NormalizePath(stub, SPathStruc::npfSlash, _RET_STR);
		}
	}
	else if(pF->Name == "?GetCodeInAltGroup") {
		const long grp_id = _ARG_LONG(1);
		long   num = 0;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(p_blk) {
			p_blk->Obj.P_Tbl->GetGoodsCodeInAltGrp(p_blk->Pack.Rec.ID, grp_id, &num);
		}
		_RET_LONG = num;
	}
	else if(pF->Name == "?GetExtDim") {
		const int dim_id = _ARG_LONG(1);
		double val = 0.0;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(dim_id && p_blk && p_blk->Pack.Rec.ID && p_blk->Pack.Rec.GdsClsID) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(p_blk->Pack.Rec.GdsClsID, &gc_pack) > 0)
				gc_pack.GetExtDim(&p_blk->Pack.ExtRec, dim_id, &val);
		}
		_RET_DBL = val;
	}
	else if(pF->Name == "?GetExtDimI") {
		const int dim_id = _ARG_LONG(1);
		double val = 0.0;
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(dim_id && p_blk && p_blk->Pack.Rec.ID && p_blk->Pack.Rec.GdsClsID) {
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(p_blk->Pack.Rec.GdsClsID, &gc_pack) > 0)
				gc_pack.GetExtDim(&p_blk->Pack.ExtRec, dim_id, &val);
		}
		_RET_LONG = (int)val;
	}
	else if(pF->Name == "?GetExtProp") {
		_RET_STR.Z();
		const int prop_id = _ARG_LONG(1);
		DL600_GoodsBlock * p_blk = static_cast<DL600_GoodsBlock *>(Extra[0].Ptr);
		if(prop_id && p_blk && p_blk->Pack.Rec.ID) {
			PPID   _i = 0;
			PPObjGoodsClass gc_obj;
			PPGdsClsPacket gc_pack;
			if(gc_obj.Fetch(p_blk->Pack.Rec.GdsClsID, &gc_pack) > 0)
				gc_pack.GetExtProp(&p_blk->Pack.ExtRec, prop_id, &_i, _RET_STR);
		}
	}
}
//
// Implementation of PPALDD_GoodsFilt
//
PPALDD_CONSTRUCTOR(GoodsFilt) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(GoodsFilt) { Destroy(); }

int PPALDD_GoodsFilt::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.SurID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		size_t data_len = 0;
		MEMSZERO(H);
		const void * p_sur_data = DS.GetTLA().SurIdList.Get(rFilt.ID, &data_len);
        SBuffer buffer;
        buffer.Write(p_sur_data, data_len);
        GoodsFilt filt;
        SSerializeContext sctx;
        if(filt.Serialize(-1, buffer, &sctx)) {
			SString temp_buf;
			H.SurID = rFilt.ID;
			H.RestrictQuotKindID = filt.RestrictQuotKindID;
			H.MtxLocID = filt.MtxLocID;
			H.BrandOwnerID = filt.BrandOwnerID;
			H.CodeArID = filt.CodeArID;
			H.GrpID = filt.GrpIDList.GetSingle();
			H.BrandID_ = filt.BrandList.GetSingle();
			H.ManufID = filt.ManufList.GetSingle();
			H.ManufCountryID = filt.ManufCountryID;
			H.UnitID = filt.UnitID;
			H.PhUnitID = filt.PhUnitID;
			H.SupplID = filt.SupplID;
			H.GoodsTypeID = filt.GoodsTypeID;
			H.TaxGrpID = filt.TaxGrpID;
			H.LocID_ = filt.LocList.GetSingle();
			H.UhttStoreID = filt.UhttStoreID;
			H.LotPeriodLow = filt.LotPeriod.low;
			H.LotPeriodUpp = filt.LotPeriod.upp;
			H.Flags = filt.Flags;
			H.InitOrder = filt.InitOrder;
			H.VatRate = filt.VatRate;
			H.VatDate = filt.VatDate;
			filt.GetExtssData(GoodsFilt::extssNameText, temp_buf.Z());
			STRNSCPY(H.SrchName, temp_buf);
			filt.GetExtssData(GoodsFilt::extssBarcodeText, temp_buf.Z());
			STRNSCPY(H.SrchBarcode, temp_buf);
			STRNSCPY(H.BarcodeLen, filt.BarcodeLen);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_UhttGoods
//
struct UhttGoodsBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttGoodsBlock() : BarcodeIterCounter(0), TagIterCounter(0), State(stFetch), PrivateGoodsGroupTagID(0)
	{
		DefUnitID = GObj.GetConfig().DefUnitID;
	}
	void Clear()
	{
		Pack.destroy();
		BarcodeIterCounter = 0;
		TagIterCounter = 0;
		State = stFetch;
		PrivateGoodsGroupTagID = 0;
		PrivateGoodsGroup.Z();
	}
	PPObjGoods GObj;
	PPObjTag   TagObj;
	PPGoodsPacket Pack;
	uint   BarcodeIterCounter;
	uint   TagIterCounter;
	int    State;
	PPID   PrivateGoodsGroupTagID;
	PPID   DefUnitID;
	SString PrivateGoodsGroup;
};

PPALDD_CONSTRUCTOR(UhttGoods)
{
	if(Valid) {
		Extra[0].Ptr = new UhttGoodsBlock();
		InitFixData(rscDefHdr, &H, sizeof(H));
		InitFixData("iter@BarcodeList", &I_BarcodeList, sizeof(I_BarcodeList));
		InitFixData("iter@TagList", &I_TagList, sizeof(I_TagList));
	}
}

PPALDD_DESTRUCTOR(UhttGoods)
{
	Destroy();
	delete static_cast<UhttGoodsBlock *>(Extra[0].Ptr);
}

int PPALDD_UhttGoods::InitData(PPFilt & rFilt, long rsrv)
{
	int   ok = -1;
	UhttGoodsBlock & r_blk = *static_cast<UhttGoodsBlock *>(Extra[0].Ptr);
	r_blk.Clear();
	MEMSZERO(H);
	if(r_blk.GObj.GetPacket(rFilt.ID, &r_blk.Pack, PPObjGoods::gpoSkipQuot) > 0) { // @v8.3.7 PPObjGoods::gpoSkipQuot
		SString temp_buf;
		H.ID        = r_blk.Pack.Rec.ID;
		H.UnitID    = r_blk.Pack.Rec.UnitID;
		H.PhUnitID  = r_blk.Pack.Rec.PhUnitID;
		H.GroupID   = r_blk.Pack.Rec.ParentID;
		H.TypeID    = r_blk.Pack.Rec.GoodsTypeID;
		H.ClsID     = r_blk.Pack.Rec.GdsClsID;
		H.ManufID   = r_blk.Pack.Rec.ManufID;
		H.TaxGrpID  = r_blk.Pack.Rec.TaxGrpID;
		H.BrandID   = r_blk.Pack.Rec.BrandID;
		H.Flags     = r_blk.Pack.Rec.Flags;
		H.PhPerUnit = r_blk.Pack.Rec.PhUPerU;
		H.Brutto    = r_blk.Pack.Stock.Brutto;
		H.Length    = r_blk.Pack.Stock.PckgDim.Length;
		H.Width     = r_blk.Pack.Stock.PckgDim.Width;
		H.Height    = r_blk.Pack.Stock.PckgDim.Height;
		H.Package   = r_blk.Pack.Stock.Package;
		H.ExpiryPeriod = r_blk.Pack.Stock.ExpiryPeriod;
		STRNSCPY(H.Name, r_blk.Pack.Rec.Name);
		r_blk.Pack.GetExtStrData(GDSEXSTR_STORAGE,  temp_buf); temp_buf.CopyTo(H.Storage,  sizeof(H.Storage));
		r_blk.Pack.GetExtStrData(GDSEXSTR_STANDARD, temp_buf); temp_buf.CopyTo(H.Standard, sizeof(H.Standard));
		r_blk.Pack.GetExtStrData(GDSEXSTR_INGRED,   temp_buf); temp_buf.CopyTo(H.Ingred,   sizeof(H.Ingred));
		r_blk.Pack.GetExtStrData(GDSEXSTR_ENERGY,   temp_buf); temp_buf.CopyTo(H.Energy,   sizeof(H.Energy));
		r_blk.Pack.GetExtStrData(GDSEXSTR_USAGE,    temp_buf); temp_buf.CopyTo(H.Usage,    sizeof(H.Usage));
		r_blk.Pack.GetExtStrData(GDSEXSTR_OKOF,     temp_buf); temp_buf.CopyTo(H.OKOF,     sizeof(H.OKOF));
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttGoods::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	UhttGoodsBlock & r_blk = *static_cast<UhttGoodsBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@BarcodeList"))
		r_blk.BarcodeIterCounter = 0;
	else if(iterId == GetIterID("iter@TagList"))
		r_blk.TagIterCounter = 0;
	return -1;
}

int PPALDD_UhttGoods::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	UhttGoodsBlock & r_blk = *static_cast<UhttGoodsBlock *>(Extra[0].Ptr);
	if(iterId == GetIterID("iter@BarcodeList")) {
		if(r_blk.BarcodeIterCounter < r_blk.Pack.Codes.getCount()) {
			STRNSCPY(I_BarcodeList.Code, r_blk.Pack.Codes.at(r_blk.BarcodeIterCounter).Code);
			I_BarcodeList.Package = r_blk.Pack.Codes.at(r_blk.BarcodeIterCounter).Qtty;
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.BarcodeIterCounter++;
	}
	else if(iterId == GetIterID("iter@TagList")) {
		if(r_blk.TagIterCounter < r_blk.Pack.TagL.GetCount()) {
			MEMSZERO(I_TagList);
			const ObjTagItem * p_item = r_blk.Pack.TagL.GetItemByPos(r_blk.TagIterCounter);
			I_TagList.TagTypeID = p_item->TagDataType;
			{
				PPObjectTag rec;
				if(r_blk.TagObj.Fetch(p_item->TagID, &rec) > 0)
					STRNSCPY(I_TagList.TagSymb, rec.Symb);
			}
			switch(p_item->TagDataType) {
				case OTTYP_STRING:
				case OTTYP_GUID:
					p_item->GetStr(temp_buf.Z());
					temp_buf.CopyTo(I_TagList.StrVal, sizeof(I_TagList.StrVal));
					break;
				case OTTYP_ENUM:
					p_item->GetInt(&I_TagList.IntVal);
					p_item->GetStr(temp_buf.Z());
					temp_buf.CopyTo(I_TagList.StrVal, sizeof(I_TagList.StrVal));
					break;
				case OTTYP_NUMBER: p_item->GetReal(&I_TagList.RealVal); break;
				case OTTYP_BOOL:
				case OTTYP_INT: p_item->GetInt(&I_TagList.IntVal); break;
				case OTTYP_DATE: p_item->GetDate(&I_TagList.DateVal); break;
			}
			ok = DlRtm::NextIteration(iterId);
		}
		r_blk.TagIterCounter++;
	}
	return ok;
}

static int SLAPI SetOuterGoodsTag(PPID tagID, const SString & rTagStrValue, PPGoodsPacket & rP)
{
	int    ok = -1;
	PPObjTag tag_obj;
	PPObjectTag tag_rec;
	if(tagID && rTagStrValue.NotEmpty() && tag_obj.Fetch(tagID, &tag_rec) > 0) {
		Reference * p_ref = PPRef;
		ObjTagItem pgg_tag_value;
		if(rTagStrValue.HasPrefix("/h|")) {
			StringSet ss("/h>");
			SString value_buf;
			ss.setBuf(rTagStrValue+3, rTagStrValue.Len()-3+1);
			if(tag_rec.TagDataType == OTTYP_ENUM && tag_rec.TagEnumID) {
				ss.reverse();
				PPID   prev_level_id = 0;
				SString temp_buf;
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					value_buf = temp_buf;
					if(value_buf.NotEmptyS()) {
						ReferenceTbl::Rec ref_rec;
						PPID   ref_id = 0;
						int    r = p_ref->SearchName(tag_rec.TagEnumID, &ref_id, value_buf, &ref_rec);
						THROW(r);
						if(r > 0) {
						}
						else if(r < 0) {
							MEMSZERO(ref_rec);
							value_buf.CopyTo(ref_rec.ObjName, sizeof(ref_rec.ObjName));
							ref_rec.Val2 = prev_level_id;
							PPTransaction tra(-1);
							THROW(tra);
							THROW(p_ref->AddItem(tag_rec.TagEnumID, &ref_id, &ref_rec, 0));
							THROW(tra.Commit());
						}
						prev_level_id = ref_id;
					}
				}
				if(prev_level_id) {
					if(pgg_tag_value.SetInt(tagID, prev_level_id)) {
						rP.TagL.PutItem(tagID, &pgg_tag_value);
						ok = 1;
					}
				}
			}
			else {
				uint ssp = 0;
				if(ss.get(&ssp, value_buf) && value_buf.NotEmptyS()) {
					if(pgg_tag_value.SetStr(tagID, value_buf)) {
						rP.TagL.PutItem(tagID, &pgg_tag_value);
						ok = 1;
					}
				}
			}
		}
		else if(pgg_tag_value.SetStr(tagID, rTagStrValue)) {
			rP.TagL.PutItem(tagID, &pgg_tag_value);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPALDD_UhttGoods::Set(long iterId, int commit)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	UhttGoodsBlock & r_blk = *static_cast<UhttGoodsBlock *>(Extra[0].Ptr);
	const PPID glob_acc_id = DS.GetConstTLA().GlobAccID;
	if(r_blk.State != UhttGoodsBlock::stSet) {
		r_blk.Clear();
		r_blk.State = r_blk.stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			Goods2Tbl::Rec parent_rec;
			r_blk.Pack.Rec.ID = H.ID;
			PPID   parent_id = H.GroupID;
			if(r_blk.GObj.ValidateGoodsParent(parent_id) > 0) {
				r_blk.Pack.Rec.ParentID = parent_id;
			}
			else {
				parent_id = r_blk.GObj.GetConfig().DefGroupID;
				if(r_blk.GObj.ValidateGoodsParent(parent_id) > 0) {
					r_blk.Pack.Rec.ParentID = parent_id;
				}
				else {
					PPObjGoodsGroup grp_obj;
					THROW(grp_obj.AddSimple(&(parent_id = 0), gpkndOrdinaryGroup, 0, "default", 0, 0, 1));
					r_blk.Pack.Rec.ParentID = parent_id;
				}
			}
			r_blk.GObj.Fetch(r_blk.Pack.Rec.ParentID, &parent_rec);

			(temp_buf = H.Name).Strip().RevertSpecSymb(SFileFormat::Html);
			STRNSCPY(r_blk.Pack.Rec.Name, temp_buf);
			STRNSCPY(r_blk.Pack.Rec.Abbr, temp_buf);
			r_blk.Pack.PutExtStrData(GDSEXSTR_STORAGE,  (temp_buf = H.Storage).Strip().RevertSpecSymb(SFileFormat::Html));
			r_blk.Pack.PutExtStrData(GDSEXSTR_STANDARD, (temp_buf = H.Standard).Strip().RevertSpecSymb(SFileFormat::Html));
			r_blk.Pack.PutExtStrData(GDSEXSTR_INGRED,   (temp_buf = H.Ingred).Strip().RevertSpecSymb(SFileFormat::Html));
			r_blk.Pack.PutExtStrData(GDSEXSTR_ENERGY,   (temp_buf = H.Energy).Strip().RevertSpecSymb(SFileFormat::Html));
			r_blk.Pack.PutExtStrData(GDSEXSTR_USAGE,    (temp_buf = H.Usage).Strip().RevertSpecSymb(SFileFormat::Html));
			r_blk.Pack.PutExtStrData(GDSEXSTR_OKOF,     (temp_buf = H.OKOF).Strip().RevertSpecSymb(SFileFormat::Html));
			{
				PPObjUnit u_obj;
				PPUnit u_rec;
				if(H.UnitID || H.PhUnitID) {
					if(H.UnitID) {
						if(u_obj.Fetch(H.UnitID, &u_rec) && u_rec.Flags & u_rec.Trade)
							r_blk.Pack.Rec.UnitID = H.UnitID;
					}
					SETIFZ(r_blk.Pack.Rec.UnitID, parent_rec.UnitID);
					SETIFZ(r_blk.Pack.Rec.UnitID, r_blk.GObj.GetConfig().DefUnitID);
					if(H.PhUnitID) {
						if(u_obj.Fetch(H.PhUnitID, &u_rec) && u_rec.Flags & u_rec.Physical)
							r_blk.Pack.Rec.PhUnitID = H.PhUnitID;
						r_blk.Pack.Rec.PhUPerU = H.PhPerUnit;
					}
				}
				if(!r_blk.Pack.Rec.UnitID && r_blk.DefUnitID) {
					if(u_obj.Fetch(r_blk.DefUnitID, &u_rec) > 0 && u_rec.Flags & u_rec.Trade)
						r_blk.Pack.Rec.UnitID = r_blk.DefUnitID;
				}
			}
			if(H.ManufID) {
				PPObjPerson psn_obj;
				PersonTbl::Rec psn_rec;
				if(psn_obj.Fetch(H.ManufID, &psn_rec) > 0 && psn_obj.P_Tbl->IsBelongToKind(H.ManufID, PPPRK_MANUF))
					r_blk.Pack.Rec.ManufID = H.ManufID;
			}
			if(H.BrandID) {
				PPObjBrand br_obj;
				PPBrand br_rec;
				if(br_obj.Fetch(H.BrandID, &br_rec) > 0)
					r_blk.Pack.Rec.BrandID = H.BrandID;
			}
			if(H.TaxGrpID) {
				PPObjGoodsTax gt_obj;
				PPGoodsTax gt_rec;
				if(gt_obj.Search(H.TaxGrpID, &gt_rec) > 0)
					r_blk.Pack.Rec.TaxGrpID = H.TaxGrpID;
			}
			r_blk.Pack.Stock.Package = H.Package;
			r_blk.Pack.Stock.Brutto = static_cast<long>(H.Brutto);
			r_blk.Pack.Stock.PckgDim.Width = H.Width;
			r_blk.Pack.Stock.PckgDim.Length = H.Length;
			r_blk.Pack.Stock.PckgDim.Height = H.Height;
			r_blk.Pack.Stock.ExpiryPeriod = static_cast<int16>(H.ExpiryPeriod);
			r_blk.Pack.Rec.Kind = PPGDSK_GOODS;
		}
		else if(iterId == GetIterID("iter@BarcodeList")) {
			SString norm_code, err_msg_buf, msg_buf;
			int   diag = 0, std = 0;
			int   r = PPObjGoods::DiagBarcode(strip(I_BarcodeList.Code), &diag, &std, &norm_code);
			if(r == 0 || (r < 0 && diag == PPObjGoods::cddFreePrefixEan13)) {
				PPObjGoods::GetBarcodeDiagText(diag, err_msg_buf);
				msg_buf.Z().Cat(std).Semicol().
					Cat(norm_code).Semicol().Cat(I_BarcodeList.Code).Semicol().Cat(err_msg_buf).CR();
				CALLEXCEPT_PP_S(PPERR_UNSTRICTBARCODE, msg_buf);
			}
			if(norm_code.NotEmpty())
				r_blk.Pack.Codes.Add(norm_code, 0, (I_BarcodeList.Package > 0) ? I_BarcodeList.Package : 1);
		}
		else if(iterId == GetIterID("iter@TagList")) {
			if(glob_acc_id && sstreqi_ascii(I_TagList.TagSymb, "OuterGroup")) {
				ObjTagItem pgg_tag;
				if(p_ref->Ot.GetTag(PPOBJ_GLOBALUSERACC, glob_acc_id, PPTAG_GUA_PGGTAG, &pgg_tag) > 0) {
					if(pgg_tag.GetStr(temp_buf) > 0) {
						PPID   pgg_tag_id = 0;
						if(r_blk.TagObj.FetchBySymb(temp_buf, &pgg_tag_id) > 0) {
							r_blk.PrivateGoodsGroupTagID = pgg_tag_id;
							r_blk.PrivateGoodsGroup = I_TagList.StrVal;
							THROW(SetOuterGoodsTag(pgg_tag_id, r_blk.PrivateGoodsGroup, r_blk.Pack));
						}
					}
				}
			}
		}
	}
	else {
		PPGlobalAccRights rights_blk(PPTAG_GUA_GOODSRIGHTS);
		int    has_rights = (r_blk.Pack.Rec.ID == 0) ? rights_blk.IsAllow(PPGlobalAccRights::fCreate) : rights_blk.IsAllow(PPGlobalAccRights::fEdit);
		if(has_rights) {
			PPID   id = r_blk.Pack.Rec.ID;
			if(id == 0) {
				THROW(r_blk.GObj.PutPacket(&id, &r_blk.Pack, 1));
			}
			else {
				//
				// Ограниченное изменение товара (только под определенной учетной записью и только некоторые атрибуты)
				//
				if(glob_acc_id) {
					int    any_for_upd = 0;
					SString ext_a, ext_b, ext_c, ext_d, ext_e;
					r_blk.Pack.GetExtStrData(GDSEXSTR_A, ext_a);
					r_blk.Pack.GetExtStrData(GDSEXSTR_B, ext_b);
					r_blk.Pack.GetExtStrData(GDSEXSTR_C, ext_c);
					r_blk.Pack.GetExtStrData(GDSEXSTR_D, ext_d);
					r_blk.Pack.GetExtStrData(GDSEXSTR_E, ext_e);
					if(ext_a.NotEmptyS() || ext_b.NotEmptyS() || ext_c.NotEmptyS() || ext_d.NotEmptyS() || ext_e.NotEmptyS())
						any_for_upd = 1;
					if(r_blk.PrivateGoodsGroup.NotEmptyS() || r_blk.Pack.Rec.BrandID || r_blk.Pack.Rec.ManufID)
						any_for_upd = 1;
					if(any_for_upd) {
						PPGoodsPacket org_pack;
						THROW(r_blk.GObj.GetPacket(id, &org_pack, 0) > 0);
						org_pack.PutExtStrData(GDSEXSTR_A, ext_a);
						org_pack.PutExtStrData(GDSEXSTR_B, ext_b);
						org_pack.PutExtStrData(GDSEXSTR_C, ext_c);
						org_pack.PutExtStrData(GDSEXSTR_D, ext_d);
						org_pack.PutExtStrData(GDSEXSTR_E, ext_e);
						if(r_blk.Pack.Rec.BrandID)
							org_pack.Rec.BrandID = r_blk.Pack.Rec.BrandID;
						if(r_blk.Pack.Rec.ManufID) {
							org_pack.Rec.ManufID = r_blk.Pack.Rec.ManufID;
						}
						if(r_blk.PrivateGoodsGroupTagID) {
							THROW(SetOuterGoodsTag(r_blk.PrivateGoodsGroupTagID, r_blk.PrivateGoodsGroup, org_pack));
						}
						THROW(r_blk.GObj.PutPacket(&id, &org_pack, 1));
					}
				}
			}
			Extra[4].Ptr = reinterpret_cast<void *>(id);
		}
		else {
			PPSetError(PPERR_NORIGHTS);
			DS.GetTLA().AddedMsgStrNoRights = DS.GetTLA().GlobAccName;
			ok = 0;
		}
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
//
// Implementation of PPALDD_UhttGoodsArCode
//
PPALDD_CONSTRUCTOR(UhttGoodsArCode) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(UhttGoodsArCode) { Destroy(); }

int PPALDD_UhttGoodsArCode::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	MEMSZERO(H);
	if(rFilt.Ptr) {
		const UhttGoodsArCodeIdent * p_ident = static_cast<const UhttGoodsArCodeIdent *>(rFilt.Ptr);
		PPObjGoods * p_goods_obj = 0;
		Goods2Tbl::Rec goods_rec;
		H.GoodsID = p_ident->GoodsID;
		if(!isempty(p_ident->Name))
			STRNSCPY(H.Name, p_ident->Name);
		else {
			SETIFZ(p_goods_obj, new PPObjGoods);
			if(p_goods_obj->Fetch(H.GoodsID, &goods_rec) > 0)
				STRNSCPY(H.Name, goods_rec.Name);
		}
		if(!isempty(p_ident->Code)) {
			STRNSCPY(H.ArCode, p_ident->Code);
			ok = 1;
		}
		else {
			SString code;
			SETIFZ(p_goods_obj, new PPObjGoods);
			if(p_goods_obj->P_Tbl->GetArCode(p_ident->ArID, H.GoodsID, code, 0) > 0) {
				code.CopyTo(H.ArCode, sizeof(H.ArCode));
				ok = 1;
			}
		}
		if(ok > 0)
			ok = DlRtm::InitData(rFilt, rsrv);
		ZDELETE(p_goods_obj);
	}
	return ok;
}
//
// Implementation of PPALDD_Transport
//
PPALDD_CONSTRUCTOR(Transport)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjTransport;
	}
}

PPALDD_DESTRUCTOR(Transport)
{
	Destroy();
	delete static_cast<PPObjTransport *>(Extra[0].Ptr);
}

int PPALDD_Transport::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjTransport * p_obj = static_cast<PPObjTransport *>(Extra[0].Ptr);
		PPTransport rec;
		if(p_obj->Get(rFilt.ID, &rec) > 0) {
			H.ID        = rec.ID;
			H.TrType    = rec.TrType;
			H.Capacity  = rec.Capacity;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Code, rec.Code);
			STRNSCPY(H.TrailerCode, rec.TrailerCode);
			GetObjectName(PPOBJ_TRANSPMODEL, rec.TrModelID, H.ModelName, sizeof(H.ModelName));
			H.OwnerID = rec.OwnerID;
			H.CountryID = rec.CountryID;
			H.CaptainID = rec.CaptainID;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_Brand
//
PPALDD_CONSTRUCTOR(Brand)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		Extra[0].Ptr = new PPObjBrand;
	}
}

PPALDD_DESTRUCTOR(Brand)
{
	Destroy();
	delete static_cast<PPObjBrand *>(Extra[0].Ptr);
}

int PPALDD_Brand::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjBrand * p_obj = static_cast<PPObjBrand *>(Extra[0].Ptr);
		PPBrandPacket pack;
		if(p_obj->Get(rFilt.ID, &pack) > 0) {
			H.ID        = pack.Rec.ID;
			STRNSCPY(H.Name, pack.Rec.Name);
			H.OwnerID = pack.Rec.OwnerID;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}

// @Muxa {
int PPALDD_Brand::Set(long iterId, int commit)
{
	int    ok = 1;
	SETIFZ(Extra[3].Ptr, new PPBrand());
	PPBrand * p_brand = static_cast<PPBrand *>(Extra[3].Ptr);
	if(commit == 0) {
		if(iterId == 0) {
			SString temp_buf;
			p_brand->ID = H.ID;
			(temp_buf = strip(H.Name)).RevertSpecSymb(SFileFormat::Html);
			STRNSCPY(p_brand->Name, temp_buf);
			p_brand->OwnerID = H.OwnerID;
		}
		else {
		}
	}
	else {
		PPObjBrand brand_obj;
		PPID  id = p_brand->ID;
		PPBrandPacket pack;
		pack.Rec = *p_brand;
		THROW(brand_obj.Put(&id, &pack, 1));
		Extra[4].Ptr = reinterpret_cast<void *>(id);
	}
	CATCHZOK
	if(commit) {
		delete p_brand;
		Extra[3].Ptr = 0;
	}
	return ok;
}
// } @Muxa
//
// Implementation of PPALDD_GoodsType
//
PPALDD_CONSTRUCTOR(GoodsType)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(GoodsType) { Destroy(); }

int PPALDD_GoodsType::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPObjGoodsType ot_obj;
		PPGoodsType ot_rec;
		if(ot_obj.Fetch(rFilt.ID, &ot_rec) > 0) {
			H.ID = ot_rec.ID;
			STRNSCPY(H.Name, ot_rec.Name);
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_GoodsTaxGrp
//
PPALDD_CONSTRUCTOR(GoodsTaxGrp)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(GoodsTaxGrp) { Destroy(); }

int PPALDD_GoodsTaxGrp::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPGoodsTax rec;
		PPObjGoodsTax gtobj;
		if(gtobj.Search(rFilt.ID, &rec) > 0) {
			H.ID         = rec.ID;
			H.VAT        = rec.VAT;
			H.Excise     = rec.Excise;
			H.SalesTax   = rec.SalesTax;
			H.fAbsExcise = BIN(rec.Flags & GTAXF_ABSEXCISE);
			STRNSCPY(H.Name, rec.Name);
			gtobj.FormatOrder(rec.Order, rec.UnionVect, H.OrderStr, sizeof(H.OrderStr));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_GoodsClass
//
PPALDD_CONSTRUCTOR(GoodsClass)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(GoodsClass) { Destroy(); }

int PPALDD_GoodsClass::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPGdsClsPacket gc_pack;
		PPObjGoodsClass gc_obj;
		if(gc_obj.GetPacket(rFilt.ID, &gc_pack) > 0) {
			SString temp_buf;
			H.ID             = gc_pack.Rec.ID;
			STRNSCPY(H.Name, gc_pack.Rec.Name);
			H.DefGrpID       = gc_pack.Rec.DefGrpID;
			H.DefUnitID      = gc_pack.Rec.DefUnitID;
			H.DefPhUnitID    = gc_pack.Rec.DefPhUnitID;
			H.DefTaxGrpID    = gc_pack.Rec.DefTaxGrpID;
			H.DefGoodsTypeID = gc_pack.Rec.DefGoodsTypeID;
			H.Flags          = gc_pack.Rec.Flags;
			H.DynGenMask     = gc_pack.Rec.DynGenMask;

			STRNSCPY(H.PropKindNam, gc_pack.PropKind.Name);
			H.PropKindObj = gc_pack.PropKind.ItemsListID;
			STRNSCPY(H.PropKindObjN, GetObjectTitle(H.PropKindObj, temp_buf));

			STRNSCPY(H.PropGradNam, gc_pack.PropGrade.Name);
			H.PropGradObj = gc_pack.PropGrade.ItemsListID;
			STRNSCPY(H.PropGradObjN, GetObjectTitle(H.PropGradObj, temp_buf));

			STRNSCPY(H.PropAddNam, gc_pack.PropAdd.Name);
			H.PropAddObj = gc_pack.PropAdd.ItemsListID;
			STRNSCPY(H.PropAddObjN, GetObjectTitle(H.PropAddObj, temp_buf));

			STRNSCPY(H.DimXName, gc_pack.DimX.Name);
			H.DimXScale = (int16)gc_pack.DimX.Scale;
			gc_pack.DimX.ToStr(0, H.DimXValues, sizeof(H.DimXValues));

			STRNSCPY(H.DimYName, gc_pack.DimY.Name);
			H.DimYScale = (int16)gc_pack.DimY.Scale;
			gc_pack.DimY.ToStr(0, H.DimYValues, sizeof(H.DimYValues));

			STRNSCPY(H.DimZName, gc_pack.DimZ.Name);
			H.DimZScale = (int16)gc_pack.DimZ.Scale;
			gc_pack.DimZ.ToStr(0, H.DimZValues, sizeof(H.DimZValues));

			STRNSCPY(H.NameConv, gc_pack.NameConv);
			STRNSCPY(H.PhUPerU_Form, gc_pack.PhUPerU_Formula);
			STRNSCPY(H.TaxMult_Form, gc_pack.TaxMult_Formula);
			STRNSCPY(H.Package_Form, gc_pack.Package_Formula);

			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_AssetWrOffGrp
//
PPALDD_CONSTRUCTOR(AssetWrOffGrp)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(AssetWrOffGrp) { Destroy(); }

int PPALDD_AssetWrOffGrp::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		PPAssetWrOffGrp rec;
		PPObjAssetWrOffGrp awog_obj;
		if(awog_obj.Search(rFilt.ID, &rec) > 0) {
			H.ID = rec.ID;
			STRNSCPY(H.Name, rec.Name);
			STRNSCPY(H.Code, rec.Code);
			H.WrOffType = rec.WrOffType;
			H.WrOffTerm = rec.WrOffTerm;
			H.Flags     = rec.Flags;
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPALDD_GoodsClassView
//
PPALDD_CONSTRUCTOR(GoodsClassView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsClassView) { Destroy(); }

int PPALDD_GoodsClassView::InitData(PPFilt & rFilt, long rsrv)
{
	PPObjGoodsClass gc_obj;
	Extra[0].Ptr = gc_obj.MakeStrAssocList(0);
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsClassView::InitIteration(PPIterID iterId, int sortId, long rsrv)
{
	StrAssocArray * p_list = static_cast<StrAssocArray *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	IterProlog(iterId, 1);
	return 1;
}

int PPALDD_GoodsClassView::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	StrAssocArray * p_list = static_cast<StrAssocArray *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		I.GcID = p_list->Get(p_list->incPointer()).Id;
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

void PPALDD_GoodsClassView::Destroy()
{
	StrAssocArray * p_list = static_cast<StrAssocArray *>(Extra[0].Ptr);
	delete p_list;
	Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_GoodsGroupView
//
PPALDD_CONSTRUCTOR(GoodsGroupView)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsGroupView)
{
	Destroy();
}

int PPALDD_GoodsGroupView::InitData(PPFilt & rFilt, long rsrv)
{
	GoodsGroupView * p_v = static_cast<GoodsGroupView *>(rFilt.Ptr);
	Extra[1].Ptr = p_v;
	H.GrpType = p_v->GetGrpType();
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_GoodsGroupView::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	GoodsGroupView * p_v = static_cast<GoodsGroupView *>(Extra[1].Ptr);
	return BIN(p_v && p_v->InitIteration());
}

int PPALDD_GoodsGroupView::NextIteration(PPIterID iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	GoodsGroupView * p_v = static_cast<GoodsGroupView *>(Extra[1].Ptr);
	GoodsGroupItem item;
	while(ok < 0 && p_v && p_v->NextIteration(&item) > 0) {
		if(PPObjGoodsGroup::IsTempAlt(item.ID) <= 0) {
			I.GrpID     = item.ID;
			I.Level     = item.Level;
			STRNSCPY(I.Code, item.Code);
			I.fAlt      = BIN(PPObjGoodsGroup::IsAlt(item.ID) > 0);
			I.fTempAlt  = BIN(PPObjGoodsGroup::IsTempAlt(item.ID) > 0);
			I.fDynAlt   = BIN(PPObjGoodsGroup::IsDynamicAlt(item.ID) > 0);
			ok = DlRtm::NextIteration(iterId);
			break;
		}
	}
	return ok;
}

void PPALDD_GoodsGroupView::Destroy()
{
	Extra[1].Ptr = 0;
}
//
// Implementation of PPALDD_GoodsLabel
//
PPALDD_CONSTRUCTOR(GoodsLabel)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignDefIterData(&I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(GoodsLabel) { Destroy(); }

int PPALDD_GoodsLabel::InitData(PPFilt & rFilt, long rsrv)
{
	const GoodsLabelAlddParam * p_param = static_cast<const GoodsLabelAlddParam *>(rFilt.Ptr);
	MEMSZERO(H);
	if(p_param) {
		H.HdrGoodsID = p_param->GoodsID;
		H.LocID      = p_param->LocID;
		H.NumCopies  = p_param->NumCopies;
		H.nn = 0;
		return DlRtm::InitData(rFilt, rsrv);
	}
	else
		return -1;
}

int PPALDD_GoodsLabel::InitIteration(PPIterID iterId, int sortId, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	if(sortId >= 0)
		SortIdx = sortId;
	H.nn = 0;
	return 1;
}

int PPALDD_GoodsLabel::NextIteration(PPIterID iterId)
{
	IterProlog(iterId, 0);
	if(H.nn < H.NumCopies) {
		I.GoodsID = H.HdrGoodsID;
		PPObjGoods gobj;
		RetailGoodsInfo rgi;
		if(gobj.GetRetailGoodsInfo(I.GoodsID, H.LocID, &rgi) > 0) {
			char   bc[32];
			STRNSCPY(bc, rgi.BarCode);
			const  size_t check_dig  = BIN(gobj.GetConfig().Flags & GCF_BCCHKDIG);
			const  int    add_chkdig = BIN(!check_dig);
			const  size_t bclen = sstrlen(bc);
			if(bclen) {
				if(bclen != 3 && bclen != (7+check_dig) && bclen < (12+check_dig))
					padleft(bc, '0', (12+check_dig) - bclen);
				const size_t len = sstrlen(bc);
				if(add_chkdig && len > 3 && !gobj.GetConfig().IsWghtPrefix(bc))
					AddBarcodeCheckDigit(bc);
			}
			STRNSCPY(I.PrnBarcode, bc);
			I.Expiry = rgi.Expiry;
			I.Price  = rgi.Price;
		}
		H.nn++;
	}
	else
		return -1;
	return DlRtm::NextIteration(iterId);
}
