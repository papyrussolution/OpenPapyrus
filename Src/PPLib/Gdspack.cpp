// GDSPACK.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
// Пакет товара и сопутствующие объекты (PPGoodsPacket, PPGdsClsProp, PPGdsClsDim, PPGdsClsPacket, etc)
//
#include <pp.h>
#pragma hdrstop
//
// PPGoodsPacket
//
PPGoodsPacket::PPGoodsPacket() : P_Filt(0), P_Quots(0), P_Gled(0), UpdFlags(0), ClsDimZeroFlags(0), LinkFiles(PPOBJ_GOODS)
{
	// @v11.8.1 destroy();
}

PPGoodsPacket::~PPGoodsPacket()
{
	// @v11.8.1 destroy();
	// @v11.8.1 {
	ZDELETE(P_Filt);
	ZDELETE(P_Quots);
	ZDELETE(P_Gled);
	// } @v11.8.1 
}

PPGoodsPacket & PPGoodsPacket::Z()
{
	UpdFlags = 0;
	ClsDimZeroFlags = 0;
	MEMSZERO(Rec);
	MEMSZERO(ExtRec);
	Stock.Z();
	Codes.clear();
	ArCodes.clear();
	ExtString.Z();
	ZDELETE(P_Filt);
	ZDELETE(P_Quots);
	ZDELETE(P_Gled);
	LinkFiles.Clear();
	TagL.Z();
	GenericList.Set(0);
	GS.Init();
	return *this;
}

PPGoodsPacket & FASTCALL PPGoodsPacket::operator = (const PPGoodsPacket & rS)
{
	if(this != &rS) {
		// @v11.8.1 destroy();
		// @v11.8.1 {
		ZDELETE(P_Filt);
		ZDELETE(P_Quots);
		ZDELETE(P_Gled);
		// } @v11.8.1 
		UpdFlags = rS.UpdFlags;
		ClsDimZeroFlags = rS.ClsDimZeroFlags;
		Rec = rS.Rec;
		ExtRec = rS.ExtRec;
		Stock = rS.Stock;
		Codes.copy(rS.Codes);
		ArCodes.copy(rS.ArCodes);
		GS = rS.GS;
		ExtString = rS.ExtString;
		ExTitles = rS.ExTitles;
		if(rS.P_Filt)
			P_Filt = new GoodsFilt(*rS.P_Filt);
		if(rS.P_Quots)
			P_Quots = new PPQuotArray(*rS.P_Quots);
		if(rS.P_Gled)
			P_Gled = new GoodsLotExtData(*rS.P_Gled);
		LinkFiles = rS.LinkFiles;
		TagL = rS.TagL;
		GenericList = rS.GenericList;
	}
	return *this;
}

int PPGoodsPacket::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPGoodsPacket::PutExtStrData(int fldID, const SString & rBuf) { return PPPutExtStrData(fldID, ExtString, rBuf); }
GoodsPacketKind PPGoodsPacket::GetPacketKind() const { return PPObjGoods::GetRecKind(&Rec); }

int PPGoodsPacket::AddCode(const char * code, long codeType, double uPerP)
{
	if(code && code[0]) {
	   	BarcodeTbl::Rec bcrec;
		strip(STRNSCPY(bcrec.Code, code));
		bcrec.Qtty = uPerP;
		bcrec.BarcodeType = codeType;
		return Codes.insert(&bcrec) ? 1 : PPSetErrorSLib();
	}
	else
		return -1;
}

int FASTCALL PPGoodsPacket::GetGroupCode(SString & rBuf) const
{
	rBuf.Z();
	if(Codes.getCount()) {
		(rBuf = Codes.at(0).Code).Strip();
		rBuf.ShiftLeftChr('@');
	}
	return rBuf.NotEmptyS() ? 1 : -1;
}

int PPGoodsPacket::SetGroupCode(const char * pBuf)
{
	Codes.clear();
	if(!isempty(pBuf)) {
		char   temp[64];
		char * p;
		if(pBuf[0] != '@') {
			temp[0] = '@';
			p = temp+1;
		}
		else
			p = temp;
		strnzcpy(p, pBuf, sizeof(temp) - 1);
		return AddCode(temp, 0, 1L);
	}
	else
		return -1;
}

int PPGoodsPacket::GetArCode(PPID arID, SString & rCode) const
{
	int    ok = -1;
	uint   pos = 0;
	if(ArCodes.lsearch(&arID, &pos, CMPF_LONG, offsetof(ArGoodsCodeTbl::Rec, ArID))) {
		rCode = ArCodes.at(pos).Code;
		ok = 1;
	}
	else
		rCode.Z();
	return ok;
}

bool PPGoodsPacket::IsExtRecEmpty() const
{
	return !(ExtRec.KindID || ExtRec.GradeID || ExtRec.AddObjID || ExtRec.AddObj2ID || ExtRec.X || ExtRec.Y || ExtRec.Z || ExtRec.W);
}

/*static*/int PPGoodsPacket::ValidateAddedMsgSign(const char * pSign, size_t signBufSize)
{
	int    ok = 1;
	int    par_open = 0;
	uint   in_par_count = 0;
	const  size_t len = sstrlen(pSign);
	if(len) {
		if(signBufSize && len >= signBufSize)
			ok = 0;
		else {
			for(uint i = 0; ok && i < len; i++) {
				char c = pSign[i];
				if(c == ' ') {
					;
				}
				else if(c == '(') {
					if(par_open)
						ok = 0;
					else
						par_open = 1;
				}
				else if(c == ')') {
					if(!par_open)
						ok = 0;
					else if(in_par_count == 0)
						ok = 0;
					else {
						par_open = 0;
						in_par_count = 0;
					}
				}
				else {
					c = toupper(c);
					if(oneof6(c, 'A', 'B', 'C', 'D', 'E', 'M')) {
						if(par_open)
							in_par_count++;
					}
					else
						ok = 0;
				}
			}
			if(par_open)
				ok = 0;
		}
	}
	else
		ok = -1;
	return ok;
}

static SString & FASTCALL PreprocessDataStr(SString & rBuf)
{
	rBuf.ReplaceChar('\t', ' ').ReplaceChar('\x0D', ' ').ReplaceChar('\x0A', ' ');
	while(rBuf.ReplaceStrR("  ", " ", 0))
		;
	return rBuf;
}

int PPGoodsPacket::PrepareAddedMsgStrings(const char * pSign, long flags, const LDATETIME * pManufDtm, StringSet & rSet)
{
	// pamsfStrictOrder
	int    ok = 1;
	int    par_open = 0;
	SString temp_buf, cat_buf;
	rSet.Z();
	if(PPGoodsPacket::ValidateAddedMsgSign(pSign, 0) > 0) {
		const size_t sign_len = sstrlen(pSign);
		for(size_t i = 0; i < sign_len; i++) {
			int    fld_id = 0;
			char   c = toupper(pSign[i]);
			if(c == ' ') {
				;
			}
			else if(c == '(') {
				par_open = 1;
				cat_buf.Z();
			}
			else if(c == ')') {
				par_open = 0;
				if(cat_buf.NotEmptyS() || (flags & pamsfStrictOrder)) {
					rSet.add(cat_buf);
					ok = 1;
				}
			}
			else {
				switch(c) {
					case 'A': fld_id = GDSEXSTR_A; break;
					case 'B': fld_id = GDSEXSTR_B; break;
					case 'C': fld_id = GDSEXSTR_C; break;
					case 'D': fld_id = GDSEXSTR_D; break;
					case 'E': fld_id = GDSEXSTR_E; break;
				}
				if(fld_id) {
					GetExtStrData(fld_id, temp_buf.Z());
					PreprocessDataStr(temp_buf.Strip());
					if(par_open) {
						if(temp_buf.NotEmptyS())
							cat_buf.Space().Cat(temp_buf);
					}
					else if(temp_buf.NotEmptyS() || (flags & pamsfStrictOrder)) {
						rSet.add(temp_buf);
						ok = 1;
					}
				}
				else if(c == 'M') {
					if(pManufDtm && pManufDtm->d) {
						temp_buf.Z().Cat(*pManufDtm, DATF_DMY, TIMF_HMS|TIMF_NOZERO);
						if(par_open) {
							cat_buf.Space().Cat(temp_buf);
						}
						else {
							rSet.add(temp_buf);
							ok = 1;
						}
					}
				}
			}
		}
	}
	else /*if()*/ {
		static const uint8 gesidlist[] = {GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E};
		for(uint i = 0; i < SIZEOFARRAY(gesidlist); i++) {
			if(GetExtStrData(gesidlist[i], temp_buf) > 0) {
				rSet.add(PreprocessDataStr(temp_buf));
				ok = 2;
			}
		}
	}
	return ok;
}
//
//
//
PPGdsClsProp::PPGdsClsProp() : ItemsListID(0)
{
	Name[0] = 0;
}

PPGdsClsProp & PPGdsClsProp::Z()
{
	Name[0] = 0;
	ItemsListID = 0;
	return *this;
}

PPGdsClsProp & FASTCALL PPGdsClsProp::operator = (const PPGdsClsProp & s)
{
	STRNSCPY(Name, s.Name);
	ItemsListID = s.ItemsListID;
	return *this;
}
//
//
//
PPGdsClsDim::PPGdsClsDim() : Scale(0)
{
	Name[0] = 0;
}

PPGdsClsDim & PPGdsClsDim::Z()
{
	Name[0] = 0;
	Scale = 0;
	ValList.Z(); // @v11.2.7 freeAll()-->Z()
	return *this;
}

PPGdsClsDim & FASTCALL PPGdsClsDim::operator = (const PPGdsClsDim & s)
{
	STRNSCPY(Name, s.Name);
	Scale = s.Scale;
	ValList.copy(s.ValList);
	return *this;
}

SString & PPGdsClsDim::ToStr(SString & rBuf) const
{
	rBuf.Z();
	for(uint i = 0; i < ValList.getCount(); i++) {
		if(i > 0)
			rBuf.Comma();
		double val = static_cast<double>(ValList.at(i));
		if(Scale != 0)
			val = val / fpow10i((int)Scale);
		rBuf.Cat(val, MKSFMTD(0, Scale, NMBF_NOTRAILZ));
	}
	return rBuf;
}

int PPGdsClsDim::FromStr(int, const char * pBuf)
{
	ValList.Z(); // @v11.2.7 freeAll()-->Z()
	char   tmp_buf[32];
	StringSet ss(',', pBuf);
	for(uint pos = 0; ss.get(&pos, tmp_buf, sizeof(tmp_buf));) {
		double val = satof(tmp_buf);
		if(Scale)
			val *= fpow10i((int)Scale);
		ValList.add(R0i(val));
	}
	return 1;
}
//
//
//
/*static*/int PPGdsCls::IsEqByDynGenMask(long mask, const GoodsExtTbl::Rec * p1, const GoodsExtTbl::Rec * p2)
{
#define M(f) (mask & (1 << (f-1)))
	if(M(eKind) && p1->KindID != p2->KindID)
		return 0;
	if(M(eGrade) && p1->GradeID != p2->GradeID)
		return 0;
	if(M(eAdd) && p1->AddObjID != p2->AddObjID)
		return 0;
	if(M(eAdd2) && p1->AddObj2ID != p2->AddObj2ID)
		return 0;
	if(M(eX) && p1->X != p2->X)
		return 0;
	if(M(eY) && p1->Y != p2->Y)
		return 0;
	if(M(eZ) && p1->Z != p2->Z)
		return 0;
	if(M(eW) && p1->W != p2->W)
		return 0;
	return 1;
#undef M
}

PPGdsCls2::PPGdsCls2()
{
	THISZERO();
}

void   PPGdsCls2::SetDynGenMask(int fld, int val) { DynGenMask |= (1 << (fld-1)); }
bool   FASTCALL PPGdsCls2::GetDynGenMask(int fld) const { return LOGIC(DynGenMask & (1 << (fld-1))); }

/*static*/long FASTCALL PPGdsCls2::UseFlagToE(long useFlag)
{
	switch(useFlag) {
		case fUsePropKind: return eKind;
		case fUsePropGrade: return eGrade;
		case fUsePropAdd: return eAdd;
		case fUsePropAdd2: return eAdd2;
		case fUseDimX: return eX;
		case fUseDimY: return eY;
		case fUseDimZ: return eZ;
		case fUseDimW: return eW;
	}
	return 0;
}

/*static*/long FASTCALL PPGdsCls2::EToUseFlag(long e)
{
	switch(e) {
		case eKind: return fUsePropKind;
		case eGrade: return fUsePropGrade;
		case eAdd: return fUsePropAdd;
		case eAdd2: return fUsePropAdd2;
		case eX: return fUseDimX;
		case eY: return fUseDimY;
		case eZ: return fUseDimZ;
		case eW: return fUseDimW;
	}
	return 0;
}
//
//
//
PPGdsClsPacket::PPGdsClsPacket()
{
	// @v11.2.7 Init();
}

PPGdsClsPacket::PPGdsClsPacket(const PPGdsClsPacket & rS)
{
	Copy(rS);
}

PPGdsClsPacket & FASTCALL PPGdsClsPacket::operator = (const PPGdsClsPacket & rS)
{
	Copy(rS);
	return *this;
}

PPGdsClsPacket & PPGdsClsPacket::Z()
{
	MEMSZERO(Rec);
	NameConv.Z();
	AbbrConv.Z();
	PhUPerU_Formula.Z();
	TaxMult_Formula.Z();
	Package_Formula.Z();
	LotDimQtty_Formula.Z();
	Brutto_Formula.Z(); // @v12.0.10
	PropKind.Z();
	PropGrade.Z();
	PropAdd.Z();
	PropAdd2.Z();
	DimX.Z();
	DimY.Z();
	DimZ.Z();
	DimW.Z();
	FormulaList.freeAll();
	return *this;
}

int FASTCALL PPGdsClsPacket::Copy(const PPGdsClsPacket & rS)
{
	Rec = rS.Rec;
	NameConv = rS.NameConv;
	AbbrConv = rS.AbbrConv;
	PhUPerU_Formula = rS.PhUPerU_Formula;
	TaxMult_Formula = rS.TaxMult_Formula;
	Package_Formula = rS.Package_Formula;
	LotDimQtty_Formula = rS.LotDimQtty_Formula;
	Brutto_Formula = rS.Brutto_Formula; // @v12.0.10
	PropKind  = rS.PropKind;
	PropGrade = rS.PropGrade;
	PropAdd   = rS.PropAdd;
	PropAdd2  = rS.PropAdd2;
	DimX = rS.DimX;
	DimY = rS.DimY;
	DimZ = rS.DimZ;
	DimW = rS.DimW;
	FormulaList.freeAll();
	for(uint i = 0; i < rS.FormulaList.getCount(); i++)
		FormulaList.insert(newStr(rS.FormulaList.at(i)));
	return 1;
}

int PPGdsClsPacket::GetPropName(int prop, SString & rBuf) const
{
	int    ok = 0;
	rBuf.Z();
	const long uf = PPGdsCls::EToUseFlag(prop);
	switch(prop) {
		case PPGdsCls::eKind:  if(Rec.Flags & uf) { (rBuf = PropKind.Name).SetIfEmpty("gckind"); ok = 1; } break;
		case PPGdsCls::eGrade: if(Rec.Flags & uf) { (rBuf = PropGrade.Name).SetIfEmpty("gcgrade"); ok = 1; } break;
		case PPGdsCls::eAdd:   if(Rec.Flags & uf) { (rBuf = PropAdd.Name).SetIfEmpty("gcadd"); ok = 1; } break;
		case PPGdsCls::eAdd2:  if(Rec.Flags & uf) { (rBuf = PropAdd2.Name).SetIfEmpty("gcadd2"); ok = 1; } break;
		case PPGdsCls::eX:     if(Rec.Flags & uf) { (rBuf = DimX.Name).SetIfEmpty("gcdimx"); ok = 1; } break;
		case PPGdsCls::eY:     if(Rec.Flags & uf) { (rBuf = DimY.Name).SetIfEmpty("gcdimy"); ok = 1; } break;
		case PPGdsCls::eZ:     if(Rec.Flags & uf) { (rBuf = DimZ.Name).SetIfEmpty("gcdimz"); ok = 1; } break;
		case PPGdsCls::eW:     if(Rec.Flags & uf) { (rBuf = DimW.Name).SetIfEmpty("gcdimw"); ok = 1; } break;
	}
	return ok;
}

int PPGdsClsPacket::FormatProp(const PPGdsClsProp * pProp, PPID propVal, SString & rBuf) const
{
	rBuf.Z();
	if(propVal && pProp->ItemsListID)
		if(GetObjectName(pProp->ItemsListID, propVal, rBuf) > 0)
			return 1;
	return -1;
}

void PPGdsClsPacket::FormatDim(const PPGdsClsDim * pDim, long dimVal, SString & rBuf) const
{
	const int scale = (int)pDim->Scale;
	double v = round(((double)dimVal) / fpow10i(scale), scale);
	rBuf.Z().Cat(v, MKSFMTD(0, scale, NMBF_NOTRAILZ));
}

int PPGdsClsPacket::GetExtDim(const GoodsExtTbl::Rec * pRec, int dim, double * pVal) const
{
	int    ok = -1;
	const  long * p_s = 0;
	double v = 0.0;
	if(pRec->GoodsClsID == Rec.ID) {
		switch(dim) {
			case PPGdsCls::eX: v = pRec->X; p_s = &DimX.Scale; break;
			case PPGdsCls::eY: v = pRec->Y; p_s = &DimY.Scale; break;
			case PPGdsCls::eZ: v = pRec->Z; p_s = &DimZ.Scale; break;
			case PPGdsCls::eW: v = pRec->W; p_s = &DimW.Scale; break;
			default: ok = 0; break;
		}
		if(p_s) {
			v /= fpow10i(*p_s);
			ok = 1;
		}
	}
	ASSIGN_PTR(pVal, v);
	return ok;
}

int PPGdsClsPacket::RealToExtDim(double realVal, int dim, long * pLongVal) const
{
	int    ok = -1;
	long   p  = 0;
	long   lv = 0L;
	switch(dim) {
		case PPGdsCls::eX: p = DimX.Scale; break;
		case PPGdsCls::eY: p = DimY.Scale; break;
		case PPGdsCls::eZ: p = DimZ.Scale; break;
		case PPGdsCls::eW: p = DimW.Scale; break;
		default: ok = 0; break;
	}
	if(ok) {
		lv = static_cast<long>(realVal * fpow10i(p));
		ok = 1;
	}
	ASSIGN_PTR(pLongVal, lv);
	return ok;
}

int PPGdsClsPacket::RealToExtDim(double realVal, int dim, GoodsExtTbl::Rec & rExtRec) const
{
	int    ok = 1;
	switch(dim) {
		case PPGdsCls::eX: rExtRec.X = static_cast<long>(realVal * fpow10i(DimX.Scale)); break;
		case PPGdsCls::eY: rExtRec.Y = static_cast<long>(realVal * fpow10i(DimY.Scale)); break;
		case PPGdsCls::eZ: rExtRec.Z = static_cast<long>(realVal * fpow10i(DimZ.Scale)); break;
		case PPGdsCls::eW: rExtRec.W = static_cast<long>(realVal * fpow10i(DimW.Scale)); break;
		default: ok = 0; break;
	}
	return ok;
}

int PPGdsClsPacket::GetExtProp(const GoodsExtTbl::Rec * pRec, int prop, long * pID, SString & rBuf) const
{
	int    ok = -1;
	PPID   id = 0;
	PPID   ot = 0;
	rBuf.Z();
	if(pRec->GoodsClsID == Rec.ID) {
		switch(prop) {
			case PPGdsCls::eKind : id = pRec->KindID;    ot = PropKind.ItemsListID;   break;
			case PPGdsCls::eGrade: id = pRec->GradeID;   ot = PropGrade.ItemsListID;  break;
			case PPGdsCls::eAdd  : id = pRec->AddObjID;  ot = PropAdd.ItemsListID;    break;
			case PPGdsCls::eAdd2 : id = pRec->AddObj2ID; ot = PropAdd2.ItemsListID;   break;
			default: ok = 0; break;
		}
		if(ot) {
			if(id)
				GetObjectName(ot, id, rBuf);
			ok = 1;
		}
	}
	ASSIGN_PTR(pID, id);
	return ok;
}

int PPGdsClsPacket::GetDynGenFilt(const GoodsExtTbl::Rec * pRec, ClsdGoodsFilt * pFilt) const
{
	int    ok = -1;
	double val;
	ClsdGoodsFilt flt;
	MEMSZERO(flt);
	flt.GdsClsID = pRec->GoodsClsID;
	if(Rec.GetDynGenMask(PPGdsCls::eKind)) {
		flt.KindList = pRec->KindID;
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eGrade)) {
		flt.GradeList = pRec->GradeID;
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eAdd)) {
		flt.AddObjList = pRec->AddObjID;
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eAdd2)) {
		flt.AddObj2List = pRec->AddObj2ID;
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eX)) {
		GetExtDim(pRec, PPGdsCls::eX, &val);
		flt.DimX_Rng.SetVal(val);
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eY)) {
		GetExtDim(pRec, PPGdsCls::eY, &val);
		flt.DimY_Rng.SetVal(val);
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eZ)) {
		GetExtDim(pRec, PPGdsCls::eZ, &val);
		flt.DimZ_Rng.SetVal(val);
		ok = 1;
	}
	if(Rec.GetDynGenMask(PPGdsCls::eW)) {
		GetExtDim(pRec, PPGdsCls::eW, &val);
		flt.DimW_Rng.SetVal(val);
		ok = 1;
	}
	ASSIGN_PTR(pFilt, flt);
	return ok;
}

// @<<PPGdsClsPacket::GetNameByTemplate
int PPGdsClsPacket::CheckForSgg(long sym, SubstGrpGoods sgg) const
{
	int    r = 1;
	if(sgg != sggNone && sym != PPSYM_GC_NAME) {
		long   sym1 = 0;
		long   sym2 = 0;
		long   sym3 = 0;
		if(sgg == sggDimX)
			sym1 = PPSYM_GC_DIMX;
		else if(sgg == sggDimY)
			sym1 = PPSYM_GC_DIMY;
		else if(sgg == sggDimZ)
			sym1 = PPSYM_GC_DIMZ;
		else if(sgg == sggDimW)
			sym1 = PPSYM_GC_DIMW;
		else if(sgg == sggBrand)
			sym1 = PPSYM_BRAND;
		if(oneof4(sgg, sggClsKind, sggClsKind_Grade, sggClsKind_Grade_AddObj, sggClsKind_AddObj_Grade))
			sym1 = PPSYM_GC_KIND;
        if(oneof4(sgg, sggClsGrade, sggClsKind_Grade, sggClsKind_Grade_AddObj, sggClsKind_AddObj_Grade))
			sym2 = (sym1) ? PPSYM_GC_GRADE : (sym1 = PPSYM_GC_GRADE, 0);
		if(oneof3(sgg, sggClsAddObj, sggClsKind_Grade_AddObj, sggClsKind_AddObj_Grade))
			sym3 = (sym2) ? PPSYM_GC_ADDPROP : ((sym1) ? (sym2 = PPSYM_GC_ADDPROP, 0) : (sym1 = PPSYM_GC_ADDPROP, 0));
		r = (sym == sym1 || sym == sym2 || sym == sym3) ? 1 : 0;
	}
	return r;
}

int PPGdsClsPacket::GetNameByTemplate(PPGoodsPacket * pPack, const char * pTemplate, char * pBuf, size_t bufLen, SubstGrpGoods sgg) const
{
	int    ok = 1;
	SString buf, temp_buf;
	PPSymbTranslator st;
	for(const char * p = pTemplate; p && *p;)
		if(*p == '@') {
			size_t next = 1;
			long   sym  = st.Translate(p, &next);
			temp_buf.Z();
			if(CheckForSgg(sym, sgg)) { // AHTOXA
				switch(sym) {
					case PPSYM_GC_NAME:
						temp_buf = Rec.Name;
						break;
					case PPSYM_GC_KIND:
						if(Rec.Flags & PPGdsCls::fUsePropKind)
							FormatProp(&PropKind, pPack->ExtRec.KindID, temp_buf);
						break;
					case PPSYM_GC_GRADE:
						if(Rec.Flags & PPGdsCls::fUsePropGrade)
							FormatProp(&PropGrade, pPack->ExtRec.GradeID, temp_buf);
						break;
					case PPSYM_GC_ADDPROP:
						if(Rec.Flags & PPGdsCls::fUsePropAdd)
							FormatProp(&PropAdd, pPack->ExtRec.AddObjID, temp_buf);
						break;
					case PPSYM_GC_ADD2PROP:
						if(Rec.Flags & PPGdsCls::fUsePropAdd2)
							FormatProp(&PropAdd2, pPack->ExtRec.AddObj2ID, temp_buf);
						break;
					case PPSYM_GC_DIMX:
						if(Rec.Flags & PPGdsCls::fUseDimX)
							FormatDim(&DimX, pPack->ExtRec.X, temp_buf);
						break;
					case PPSYM_GC_DIMY:
						if(Rec.Flags & PPGdsCls::fUseDimY)
							FormatDim(&DimY, pPack->ExtRec.Y, temp_buf);
						break;
					case PPSYM_GC_DIMZ:
						if(Rec.Flags & PPGdsCls::fUseDimZ)
							FormatDim(&DimZ, pPack->ExtRec.Z, temp_buf);
						break;
					case PPSYM_GC_DIMW:
						if(Rec.Flags & PPGdsCls::fUseDimW)
							FormatDim(&DimW, pPack->ExtRec.W, temp_buf);
						break;
					case PPSYM_BRAND:
						if(pPack->Rec.BrandID)
							GetObjectName(PPOBJ_BRAND, pPack->Rec.BrandID, temp_buf);
						break;
					case PPSYM_GRNAME:
						if(pPack->Rec.ParentID)
							GetGoodsName(pPack->Rec.ParentID, temp_buf);
						break;
				}
			}
			buf.Cat(temp_buf);
			p += next;
		}
		else
			buf.CatChar(*p++);
	if(buf.NotEmptyS()) {
		buf.CopyTo(pBuf, bufLen);
		ok = 1;
	}
	else
		ok = -1;
	return ok;
}

int PPGdsClsPacket::CompleteGoodsPacket(PPGoodsPacket * pPack)
{
	int    r = GetNameByTemplate(pPack, NameConv, pPack->Rec.Name, sizeof(pPack->Rec.Name));
	if(AbbrConv.NotEmptyS())
		GetNameByTemplate(pPack, AbbrConv, pPack->Rec.Abbr, sizeof(pPack->Rec.Abbr));
	else if(r > 0 || strip(pPack->Rec.Abbr)[0] == 0)
		STRNSCPY(pPack->Rec.Abbr, pPack->Rec.Name);
	if(pPack->Rec.UnitID == 0) {
		if(Rec.DefUnitID)
			pPack->Rec.UnitID = Rec.DefUnitID;
		else {
			PPObjGoods goods_obj;
			pPack->Rec.UnitID = goods_obj.GetConfig().DefUnitID;
		}
	}
	if(Rec.DefPhUnitID) {
		if(PhUPerU_Formula.NotEmptyS()) {
			double phuperu = 0.0;
			GdsClsCalcExprContext ctx(this, pPack);
			PPCalcExpression(PhUPerU_Formula, &phuperu, &ctx);
			if(phuperu > 0) {
				pPack->Rec.PhUnitID = Rec.DefPhUnitID;
				pPack->Rec.PhUPerU = phuperu;
			}
		}
	}
	if(TaxMult_Formula.NotEmptyS()) {
		double taxfactor = 0.0;
		GdsClsCalcExprContext ctx(this, pPack);
		PPCalcExpression(TaxMult_Formula, &taxfactor, &ctx);
		pPack->ExtRec.TaxFactor = taxfactor;
	}
	else
		pPack->ExtRec.TaxFactor = 0;
	SETFLAG(pPack->Rec.Flags, GF_TAXFACTOR, pPack->ExtRec.TaxFactor != 0);
	if(Package_Formula.NotEmptyS()) {
		double package = 0.0;
		GdsClsCalcExprContext ctx(this, pPack);
		PPCalcExpression(Package_Formula, &package, &ctx);
		pPack->Stock.Package = R6(package);
	}
	if(Brutto_Formula.NotEmptyS()) { // @v12.0.10
		double brutto = 0.0;
		GdsClsCalcExprContext ctx(this, pPack);
		PPCalcExpression(Brutto_Formula, &brutto, &ctx);
		pPack->Stock.Brutto = R0i(brutto * 1000.0);
	}
	SETIFZ(pPack->Rec.TaxGrpID,    Rec.DefTaxGrpID);
	SETIFZ(pPack->Rec.GoodsTypeID, Rec.DefGoodsTypeID);
	SETIFZ(pPack->Rec.ParentID,    Rec.DefGrpID);
	pPack->Rec.GdsClsID = Rec.ID;
	return 1;
}

int PPGdsClsPacket::PropSymbToID(int prop, const char * pSymb, PPID * pID)
{
	int    ok = -1;
	PPID   obj_type = 0;
	switch(prop) {
		case PPGdsCls::eKind: obj_type = PropKind.ItemsListID; break;
		case PPGdsCls::eGrade: obj_type = PropGrade.ItemsListID; break;
		case PPGdsCls::eAdd: obj_type = PropAdd.ItemsListID; break;
		case PPGdsCls::eAdd2: obj_type = PropAdd2.ItemsListID; break;
	}
	if(obj_type && IS_REF_OBJTYPE(obj_type)) {
		PPID   id = 0;
		if(PPRef->SearchSymb(obj_type, &id, pSymb, 0) > 0) {
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	return ok;
}

int PPGdsClsPacket::PropNameToID(int prop, const char * pName, PPID * pID, int forceCreate, int use_ta)
{
	int    ok = -1;
	PPID   obj_type = 0;
	switch(prop) {
		case PPGdsCls::eKind: obj_type = PropKind.ItemsListID; break;
		case PPGdsCls::eGrade: obj_type = PropGrade.ItemsListID; break;
		case PPGdsCls::eAdd: obj_type = PropAdd.ItemsListID; break;
		case PPGdsCls::eAdd2: obj_type = PropAdd2.ItemsListID; break;
	}
	if(obj_type && IS_REF_OBJTYPE(obj_type)) {
		PPID   id = 0;
		Reference * p_ref = PPRef;
		if(p_ref->SearchName(obj_type, &id, pName, 0) > 0) {
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
		else if(forceCreate) {
			ReferenceTbl::Rec rec;
			STRNSCPY(rec.ObjName, pName);
			THROW(p_ref->AddItem(obj_type, &(id = 0), &rec, use_ta));
			ASSIGN_PTR(pID, id);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
