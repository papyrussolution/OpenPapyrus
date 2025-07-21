// GOODS.CPP
// Copyright (c) A.Sobolev 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

ClsdGoodsFilt::ClsdGoodsFilt()
{
	Z();
}

ClsdGoodsFilt & ClsdGoodsFilt::Z()
{
	GdsClsID = 0;
	Flags = 0;
	DimX_Rng.Z();
	DimY_Rng.Z();
	DimZ_Rng.Z();
	DimW_Rng.Z();
	return *this;
}

int FASTCALL ClsdGoodsFilt::IsEq(const ClsdGoodsFilt & rS) const
{
#define CMP_FLD(f) if((f) != (rS.f)) return 0
	CMP_FLD(GdsClsID);
	CMP_FLD(Flags);
	CMP_FLD(DimX_Rng);
	CMP_FLD(DimY_Rng);
	CMP_FLD(DimZ_Rng);
	CMP_FLD(DimW_Rng);
#undef CMP_FLD
#define CMP_LST(f) if(!f.IsEq(rS.f)) return 0
	CMP_LST(KindList);
	CMP_LST(GradeList);
	CMP_LST(AddObjList);
	CMP_LST(AddObj2List);
	return 1;
}

int ClsdGoodsFilt::HasAttrRestrictions() const
{
	return (KindList.GetCount() || GradeList.GetCount() || AddObjList.GetCount() || AddObj2List.GetCount() ||
		!DimX_Rng.IsZero() || !DimY_Rng.IsZero() || !DimZ_Rng.IsZero() || !DimW_Rng.IsZero());
}

int ClsdGoodsFilt::SetDimRange(int dim, double low, double upp)
{
	int    ok = 1;
	switch(dim) {
		case PPGdsCls::eX: DimX_Rng.Set(low, upp); break;
		case PPGdsCls::eY: DimY_Rng.Set(low, upp); break;
		case PPGdsCls::eZ: DimZ_Rng.Set(low, upp); break;
		case PPGdsCls::eW: DimW_Rng.Set(low, upp); break;
		default: ok = 0; break;
	}
	return ok;
}

BarcodeArrangeConfig::BarcodeArrangeConfig()
{
	THISZERO();
}

int BarcodeArrangeConfig::Load()
{
	THISZERO();
	int    ok = -1;
	PPIniFile ini_file;
	SString param, prefix, length;
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BARCODELOWPRIORITY, param) > 0) {
		param.Divide(',', prefix, length);
		prefix.Strip().CopyTo(LowPriorPrefix, sizeof(LowPriorPrefix));
		LowPriorLen = (int16)length.Strip().ToLong();
		ok = 1;
	}
	return ok;
}

int BarcodeArrangeConfig::Save()
{
	return -1;
}

int BarcodeArrangeConfig::IsLowPrior(const char * pBarcode) const
{
	const size_t lpp_len = sstrlen(LowPriorPrefix);
	if(lpp_len) {
		if(strnicmp(pBarcode, LowPriorPrefix, lpp_len) == 0)
			if(!LowPriorLen || sstrlen(pBarcode) == LowPriorLen)
				return 1;
	}
	else if(LowPriorLen && sstrlen(pBarcode) == LowPriorLen)
		return 1;
	return 0;
}
//
//
//
int32 FASTCALL MakeInnerBarcodeType(int bt)
{
	if(bt == BARCODE_TYPE_COMMON)
		return BARCODE_TYPE_COMMON;
	else if(bt == BARCODE_TYPE_PREFERRED)
		return BARCODE_TYPE_PREFERRED;
	else if(bt == BARCODE_TYPE_MARKED)
		return BARCODE_TYPE_MARKED;
	else if(bt == BARCODE_TYPE_PREFMARK)
		return BARCODE_TYPE_PREFMARK;
	else if(bt == BARCODE_TYPE_UNDEF)
		return BARCODE_TYPE_UNDEF;
	else
		return BARCODE_TYPE_COMMON;
}

void FASTCALL SetInnerBarcodeType(int32 * pBarcodeType, int bt)
{
	if(pBarcodeType) {
		int32 obct = *pBarcodeType;
		if(bt == BARCODE_TYPE_MARKED) {
			if(obct == BARCODE_TYPE_PREFERRED)
				obct = BARCODE_TYPE_PREFMARK;
			else if(obct != BARCODE_TYPE_PREFMARK)
				obct = BARCODE_TYPE_MARKED;
		}
		else if(bt == BARCODE_TYPE_PREFERRED) {
			if(obct == BARCODE_TYPE_MARKED)
				obct = BARCODE_TYPE_PREFMARK;
			else if(obct != BARCODE_TYPE_PREFMARK)
				obct = BARCODE_TYPE_PREFERRED;
		}
		else if(bt == BARCODE_TYPE_COMMON) {
			if(!oneof4(obct, BARCODE_TYPE_PREFERRED, BARCODE_TYPE_MARKED, BARCODE_TYPE_PREFMARK, BARCODE_TYPE_COMMON))
				obct = BARCODE_TYPE_COMMON;
		}
		else if(bt == BARCODE_TYPE_UNDEF)
			obct = BARCODE_TYPE_UNDEF;
		*pBarcodeType = obct;
	}
}

void FASTCALL ResetInnerBarcodeType(int32 * pBarcodeType, int bt)
{
	if(pBarcodeType) {
		int32 obct = *pBarcodeType;
		if(bt == BARCODE_TYPE_MARKED) {
			if(obct == BARCODE_TYPE_PREFMARK)
				obct = BARCODE_TYPE_PREFERRED;
			else if(obct == BARCODE_TYPE_MARKED)
				obct = BARCODE_TYPE_COMMON;
		}
		else if(bt == BARCODE_TYPE_PREFERRED) {
			if(obct == BARCODE_TYPE_PREFMARK)
				obct = BARCODE_TYPE_MARKED;
			else if(obct == BARCODE_TYPE_PREFERRED)
				obct = BARCODE_TYPE_COMMON;
		}
		else if(bt == BARCODE_TYPE_UNDEF)
			obct = BARCODE_TYPE_COMMON;
		*pBarcodeType = obct;
	}
}

int FASTCALL IsInnerBarcodeType(int32 barcodeType, int bt)
{
	if(bt == BARCODE_TYPE_PREFERRED)
		return oneof2(barcodeType, BARCODE_TYPE_PREFERRED, BARCODE_TYPE_PREFMARK);
	else if(bt == BARCODE_TYPE_MARKED)
		return oneof2(barcodeType, BARCODE_TYPE_MARKED, BARCODE_TYPE_PREFMARK);
	else if(bt == BARCODE_TYPE_PREFMARK)
		return (barcodeType == BARCODE_TYPE_PREFMARK);
	else if(bt == BARCODE_TYPE_UNDEF)
		return (barcodeType == BARCODE_TYPE_UNDEF);
	else if(bt == BARCODE_TYPE_COMMON)
		return oneof3(barcodeType, BARCODE_TYPE_COMMON, BARCODE_TYPE_PREFERRED, BARCODE_TYPE_PREFMARK);
	else
		return 0;
}
//
//
//
int BarcodeArray::Add(const char * pCode, long codeType, double qtty)
{
	BarcodeTbl::Rec item;
	STRNSCPY(item.Code, pCode);
	item.BarcodeType = codeType;
	item.Qtty = qtty;
	return insert(&item) ? 1 : PPSetErrorSLib();
}

void BarcodeArray::Arrange()
{
	const  BarcodeArrangeConfig & bac = DS.GetConstTLA().Bac;
	uint   last = getCount();
	if(getCount() > 1)
		for(int i = getCount()-2; i >= 0; i--) {
			const BarcodeTbl::Rec rec = at(i);
			if(bac.IsLowPrior(rec.Code)) {
				atInsert(last, &rec);
				last--;
				atFree(i);
			}
		}
}

int BarcodeArray::SearchCode(const char * pCode, uint * pPos) const
{
	int    ok = 0;
	if(!isempty(pCode)) {
		for(uint i = 0; !ok && i < getCount(); i++) {
			if(sstreq(at(i).Code, pCode)) {
				ASSIGN_PTR(pPos, i);
				ok = 1;
			}
		}
	}
	return ok;
}

int BarcodeArray::GetSingle(uint sifFlags/*BarcodeArray::sifXXX*/, SString & rBuf) const
{
	int    ok = -1;
	const BarcodeTbl::Rec * p_single_item = GetSingleItem(0, sifFlags);
	if(p_single_item) {
		if(IsInnerBarcodeType(p_single_item->BarcodeType, BARCODE_TYPE_PREFERRED))
			ok = (p_single_item->Qtty == 1.0) ? 2 : 3;
		else
			ok = 1;
		rBuf = p_single_item->Code;
	}
	else
		rBuf.Z();
	return ok;
}

/*
const BarcodeTbl::Rec * FASTCALL BarcodeArray::GetSingleItem(uint * pPos) const
{
	const  BarcodeTbl::Rec * p_ret = 0;
	const  uint c = getCount();
	if(c) {
		uint   i, p = 0;
		if(c > 1) {
			const BarcodeTbl::Rec * p_pref_item = GetPreferredItem(&i);
			if(p_pref_item)
				p = i;
			else {
				int    done = 0;
				for(int get_any = 0; !done && get_any <= 1; get_any++) {
					for(i = 0; !done && i < c; i++) {
						const BarcodeTbl::Rec & r_item = at(i);
						if((r_item.Qtty == 1.0 && sstrlen(r_item.Code) <= 13) || get_any) {
							p = i;
							done = 1;
						}
					}
				}
			}
		}
		p_ret = &at(p);
		ASSIGN_PTR(pPos, p);
	}
	return p_ret;
}
*/

const BarcodeTbl::Rec * BarcodeArray::GetSingleItem(uint * pPos, uint sifFlags/*BarcodeArray::sifXXX*/) const
{
	const  BarcodeTbl::Rec * p_ret = 0;
	const  uint c = getCount();
	if(c) {
		uint   i;
		int    diag = 0;
		int    std = 0;
		SString norm_code;
		if(c > 1) {
			const BarcodeTbl::Rec * p_pref_item = GetPreferredItem(&i);
			bool  is_pref_suitable = false;
			uint   p = 0; // result index +1 (0 - suitable entry is not found)
			if(p_pref_item) {
				if(sifFlags & sifValidEanUpcOnly) {
					const  int dbcr = PPObjGoods::DiagBarcode(p_pref_item->Code, &diag, &std, &norm_code);
					if(dbcr > 0 && oneof4(std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE))
						is_pref_suitable = true;
				}
				else
					is_pref_suitable = true;
			}
			if(is_pref_suitable) {
				p = i+1;
			}
			else {
				for(int get_any = 0; !p && get_any <= 1; get_any++) {
					for(i = 0; !p && i < c; i++) {
						const BarcodeTbl::Rec & r_item = at(i);
						if((r_item.Qtty == 1.0 && sstrlen(r_item.Code) <= 13) || get_any) {
							if(sifFlags & sifValidEanUpcOnly) {
								const  int dbcr = PPObjGoods::DiagBarcode(r_item.Code, &diag, &std, &norm_code);
								if(dbcr > 0 && oneof4(std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE)) {
									p = i+1;
								}
							}
							else {
								p = i+1;
							}
						}
					}
				}
			}
			if(p) {
				p_ret = &at(p-1);
				ASSIGN_PTR(pPos, p-1);
			}
		}
		else {
			const BarcodeTbl::Rec & r_item = at(0);
			if(sifFlags & sifValidEanUpcOnly) {
				const  int dbcr = PPObjGoods::DiagBarcode(r_item.Code, &diag, &std, &norm_code);
				if(dbcr > 0 && oneof4(std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE)) {
					p_ret = &r_item;
					ASSIGN_PTR(pPos, 0);
				}
			}
			else {
				p_ret = &r_item;
				ASSIGN_PTR(pPos, 0);
			}
		}
	}
	return p_ret;
}

int FASTCALL BarcodeArray::SetPreferredItem(uint pos)
{
	int    ok = 1;
	if(pos < getCount()) {
		for(uint i = 0; i < getCount(); i++) {
			if(i != pos && IsInnerBarcodeType(at(i).BarcodeType, BARCODE_TYPE_PREFERRED)) {
				ResetInnerBarcodeType(&at(i).BarcodeType, BARCODE_TYPE_PREFERRED);
				ok = 2;
			}
		}
		SetInnerBarcodeType(&at(pos).BarcodeType, BARCODE_TYPE_PREFERRED);
	}
	else
		ok = -1;
	return ok;
}

const BarcodeTbl::Rec * FASTCALL BarcodeArray::GetPreferredItem(uint * pPos) const
{
	for(uint i = 0; i < getCount(); i++) {
		if(IsInnerBarcodeType(at(i).BarcodeType, BARCODE_TYPE_PREFERRED)) {
			ASSIGN_PTR(pPos, i);
			return &at(i);
		}
	}
	ASSIGN_PTR(pPos, 0);
	return 0;
}

int BarcodeArray::Replace(const char * pSearchCode, const char * pReplaceCode)
{
	int    ok = -1;
	uint   c = getCount();
	if(c)
		do {
			BarcodeTbl::Rec & r_rec = at(--c);
			if(sstreq(r_rec.Code, pSearchCode)) {
				if(pReplaceCode)
					STRNSCPY(r_rec.Code, pReplaceCode);
				else
					atFree(c);
				ok = 1;
			}
		} while(c);
	return ok;
}
//
// TwoDimBarcodeFormatArray
//
struct TwoDimBarcodeFormatEntry {
	long   MinBarcodeLen;
	long   FmtDescrOffset;
	char   FmtDescr[16];
	long   BarcodeOffset;
	long   BarcodeLen;
};

class TwoDimBarcodeFormatArray : public  TSArray <TwoDimBarcodeFormatEntry> {
public:
	TwoDimBarcodeFormatArray() : TSArray <TwoDimBarcodeFormatEntry>()
	{
		Init();
	}
	int    Init();
	int    Search(GoodsCore * pGoodsTbl, const char * pCodeLine, BarcodeTbl::Rec * pRec, Goods2Tbl::Rec * pGoodsRec) const;
};

int TwoDimBarcodeFormatArray::Init()
{
	int    ok = -1;
	SString buf;
	PPIniFile ini_file;
	if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_TWOSIZE_BARCODE_FORMAT, buf) > 0) {
		StringSet ss(';', buf);
		for(uint pos = 0; ss.get(&pos, buf);) {
			uint  p = 0;
			TwoDimBarcodeFormatEntry  tbf_entry;
			StringSet  descr(',', buf);
			descr.get(&p, buf);
			strtolong(buf, &tbf_entry.MinBarcodeLen);
			descr.get(&p, buf);
			strtolong(buf, &tbf_entry.FmtDescrOffset);
			descr.get(&p, buf);
			STRNSCPY(tbf_entry.FmtDescr, buf);
			descr.get(&p, buf);
			strtolong(buf, &tbf_entry.BarcodeOffset);
			descr.get(&p, buf);
			strtolong(buf, &tbf_entry.BarcodeLen);
			THROW_SL(insert(&tbf_entry));
		}
	}
	CATCH
		freeAll();
		ok = 0;
	ENDCATCH
	return ok;
}

int TwoDimBarcodeFormatArray::Search(GoodsCore * pGoodsTbl, const char * pCodeLine, BarcodeTbl::Rec * pRec, Goods2Tbl::Rec * pGoodsRec) const
{
	int    ok = -1;
	size_t len = sstrlen(pCodeLine);
	BarcodeTbl::Rec bc_rec;
	SString code_buf;
	if(len) {
		if(PrcssrAlcReport::IsEgaisMark(pCodeLine, &code_buf)) {
			PrcssrAlcReport::EgaisMarkBlock emb;
			if(PrcssrAlcReport::ParseEgaisMark(code_buf, emb)) {
				if(pGoodsTbl->SearchByBarcode(emb.EgaisCode, &bc_rec, 0) > 0)
					ok = 1;
			}
		}
		else {
			for(uint i = 0; ok < 0 && i < getCount(); i++) {
				const TwoDimBarcodeFormatEntry & tbf_entry = at(i);
				const size_t descr_len = sstrlen(tbf_entry.FmtDescr);
				if(len >= (size_t)tbf_entry.MinBarcodeLen && strnicmp(pCodeLine + tbf_entry.FmtDescrOffset, tbf_entry.FmtDescr, descr_len) == 0) {
					(code_buf = (pCodeLine + tbf_entry.BarcodeOffset)).Trim(tbf_entry.BarcodeLen);
					//
					// @note: В версии 6.1.0 вызов PPObjGoods::SearchByBarcode с признаком adoptSearching
					// заменен на вызов GoodsCore::SearchByBarcode (то есть адаптивный поиск теперь отключен).
					// По логике здесь адаптивный поиск и не требуется. Тем не менее, следует иметь
					// в виду указанное изменение при разборе вероятных проблем с поиском по двухмерному коду.
					//
					if(pGoodsTbl->SearchByBarcode(code_buf, &bc_rec, 0) > 0)
						ok = 1;
				}
			}
		}
	}
	ASSIGN_PTR(pRec, bc_rec);
	return ok;
}
//
//
//
void GoodsCore::InitQc()
{
	if(!P_Qc && !P_Qc2) {
		if(CConfig.Flags2 & CCFLG2_QUOT2)
			P_Qc2 = new Quotation2Core;
		else
			P_Qc = new QuotationCore;
	}
	assert(BIN(P_Qc) != BIN(P_Qc2));
}

GoodsCore::GoodsCore() : P_Ref(PPRef), P_Qc(0), P_Qc2(0)
{
}

GoodsCore::~GoodsCore()
{
	delete P_Qc;
	delete P_Qc2;
}

bool GoodsCore::IsRecValid(const Goods2Tbl::Rec * pRec)
{
	bool   ok = true;
	const  long  k = pRec->Kind;
	Goods2Tbl::Rec parent_rec;
	if(k == PPGDSK_GOODS) {
		THROW_PP_S(pRec->ParentID, PPERR_GOODSGROUPNEEDED, pRec->Name); // @v10.3.6 THROW_PP-->THROW_PP_S(..,pRec->Name)
		THROW_PP_S(Fetch(pRec->ParentID, &parent_rec) > 0, PPERR_GOODSGROUPNEEDED, pRec->Name); // @v10.3.6 THROW_PP-->THROW_PP_S(..,pRec->Name)
		THROW_PP_S(!(parent_rec.Flags & (GF_ALTGROUP|GF_FOLDER)), PPERR_INVGOODSPARENT, pRec->Name)
	}
	else if(k == PPGDSK_GROUP) {
		if(pRec->ParentID) {
			THROW_PP_S(Fetch(pRec->ParentID, &parent_rec) > 0, PPERR_GOODSGROUPNEEDED, pRec->Name); // @v10.3.6 THROW_PP-->THROW_PP_S(..,pRec->Name)
			THROW_PP_S(parent_rec.Flags & GF_FOLDER, PPERR_INVGOODSPARENT, pRec->Name)
		}
	}
	else {
		//THROW_PP(oneof3(k, PPGDSK_PCKGTYPE, PPGDSK_TRANSPORT, PPGDSK_BRAND), PPERR_INVGOODSKIND);
		THROW_PP_S(oneof6(k, PPGDSK_PCKGTYPE, PPGDSK_TRANSPORT, PPGDSK_BRAND, PPGDSK_SUPRWARE, PPGDSK_SWPROGRAM, PPGDSK_COMPUTER), PPERR_INVGOODSKIND, pRec->Name);
	}
	CATCHZOK
	return ok;
}

int GoodsCore::Update(PPID * pGoodsID, Goods2Tbl::Rec * pRec, int use_ta)
{
	int    ok = 1, r;
	PPID   id = *pGoodsID;
	PPID   obj_type = 0;
	if(pRec)
		switch(pRec->Kind) {
			case PPGDSK_GOODS:     obj_type = PPOBJ_GOODS;      break;
			case PPGDSK_GROUP:     obj_type = PPOBJ_GOODSGROUP; break;
			case PPGDSK_TRANSPORT: obj_type = PPOBJ_TRANSPORT;  break;
			case PPGDSK_BRAND:     obj_type = PPOBJ_BRAND;      break;
			case PPGDSK_PCKGTYPE:  obj_type = PPOBJ_PCKGTYPE;   break;
			case PPGDSK_SUPRWARE:  obj_type = PPOBJ_COMPGOODS;  break; // @vmiller
			case PPGDSK_SWPROGRAM: obj_type = PPOBJ_SWPROGRAM;  break; // @v11.9.12
			case PPGDSK_COMPUTER:  obj_type = PPOBJ_COMPUTER;  break; // @v11.9.12
		}
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(id) {
			if(pRec) {
				pRec->Flags &= ~(GF_TRANSGLED|GF_TRANSQUOT);
				THROW(IsRecValid(pRec));
				THROW(SearchByID_ForUpdate(this, obj_type, id, 0));
				if(!updateRecBuf(pRec)) { // @sfu
					if(BtrError == BE_DUP) {
						SString msg_buf;
						CALLEXCEPT_PP_S(PPERR_DUPGOODS, msg_buf.Cat(pRec->ID).Quot('[', ']').Space().Cat(pRec->Name));
					}
					else
						CALLEXCEPT_PP(PPERR_DBENGINE);
				}
			}
			else {
				PPID   ref_id = 0;
				THROW(Search(id) > 0);
				THROW(r = SearchAnyRef(obj_type, id, &ref_id));
				PPObject::SetLastErrObj(obj_type, ref_id);
				THROW_PP(r < 0, PPERR_REFSEXISTS);
				THROW(UpdateBarcodes(id, 0, 0));
				THROW(UpdateArCodes(id, 0, 0));
				THROW(P_Ref->Assc.Remove(PPASS_ALTGOODSGRP, 0, id, 0));
				THROW(P_Ref->Assc.Remove(PPASS_ALTGOODSGRP, id, 0, 0));
				THROW(RemoveGoodsFromGen(id, 0, 0));
				THROW(P_Ref->Assc.Remove(PPASS_GENGOODS, id, 0, 0));
				THROW_DB(deleteFrom(this, 0, this->ID == id));
			}
			Dirty(id);
		}
		else if(pRec) {
			int    r;
			pRec->Flags &= ~(GF_TRANSGLED|GF_TRANSQUOT);
			THROW(IsRecValid(pRec));
			r = AddObjRecByID(this, obj_type, pGoodsID, pRec, 0);
			if(!r && BtrError == BE_DUP) {
				SString msg_buf;
				CALLEXCEPT_PP_S(PPERR_DUPGOODS, msg_buf.Cat(pRec->ID).Quot('[', ']').Space().Cat(pRec->Name));
			}
			THROW(r);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::AddBarcode(PPID goodsID, const char * pBarcode, double qtty, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec goods_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Fetch(goodsID, &goods_rec) > 0) {
			BarcodeTbl::Rec rec;
			rec.GoodsID = goodsID;
			STRNSCPY(rec.Code, pBarcode);
			rec.Qtty = (qtty > 0.0) ? qtty : 1.0;
			THROW_DB(BCTbl.insertRecBuf(&rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::UpdateBarcodes(PPID goodsID, const BarcodeArray * pCodeList, int use_ta)
{
	int    ok = 1;
	if(goodsID) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(&BCTbl, 0, BCTbl.GoodsID == goodsID));
		if(pCodeList) {
			BarcodeTbl::Rec * p_rec;
			for(uint i = 0; pCodeList->enumItems(&i, (void **)&p_rec);) {
		   	    p_rec->GoodsID = goodsID;
				THROW_DB(BCTbl.insertRecBuf(p_rec));
	   	    }
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::UpdateArCodes(PPID goodsID, const ArGoodsCodeArray * pCodeList, int use_ta)
{
	int    ok = 1;
	if(goodsID) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(&ACodT, 0, ACodT.GoodsID == goodsID));
		if(pCodeList && Search(goodsID) > 0) {
			ArGoodsCodeTbl::Rec * p_rec;
			for(uint i = 0; pCodeList->enumItems(&i, (void **)&p_rec);) {
		   	    p_rec->GoodsID = goodsID;
				THROW_DB(ACodT.insertRecBuf(p_rec));
	   	    }
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SetArCode(PPID goodsID, PPID arID, const char * pCode, int32 pack, int use_ta)
{
	int    ok = 1;
	if(goodsID) {
		PPID   goods_id = labs(goodsID);
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_DB(deleteFrom(&ACodT, 0, ACodT.GoodsID == goods_id && ACodT.ArID == arID));
		if(!isempty(pCode) && Search(goodsID) > 0) {
			ArGoodsCodeTbl::Rec rec;
			rec.GoodsID = goods_id;
			rec.ArID    = arID;
			rec.Pack    = pack;
			STRNSCPY(rec.Code, pCode);
			THROW_DB(ACodT.insertRecBuf(&rec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SetArCode(PPID goodsID, PPID arID, const char * pCode, int use_ta)
{
	return SetArCode(goodsID, arID, pCode, 1000, use_ta);
}

int GoodsCore::MoveArCodes(PPID destArID, PPID srcArID, PPID grpID, uint flags, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	SString code_buf;
	SString fmt_buf;
	SString msg_buf;
	if(srcArID >= 0) {
		ArGoodsCodeArray list;
		if(ReadArCodesByAr(0, srcArID, &list) > 0) {
			PPTransaction tra(use_ta);
			THROW(tra);
			for(uint i = 0; i < list.getCount(); i++) {
				ArGoodsCodeTbl::Rec & r_rec = list.at(i);
				if(BelongToGroup(r_rec.GoodsID, grpID, 0) > 0) {
					ArGoodsCodeTbl::Rec found_rec;
					if(SearchByArCode(destArID, r_rec.Code, &found_rec, 0) > 0) {
						if(PPObjGoods::GenerateOwnArCode(code_buf, 0) > 0) {
							ArGoodsCodeTbl::Rec new_rec;
							new_rec = r_rec;
							new_rec.ArID = destArID;
							code_buf.CopyTo(new_rec.Code, sizeof(new_rec.Code));
							THROW(ACodT.insertRecBuf(&new_rec));
							CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_EXISTSGEN, &msg_buf, destArID, r_rec.Code, found_rec.GoodsID, new_rec.Code)));
						}
						else {
							CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_EXISTSNGEN, &msg_buf, destArID, r_rec.Code, found_rec.GoodsID)));
						}
					}
					// @v11.7.5 {
					else if(flags & movarcodfCopyOnly) {
						ArGoodsCodeTbl::Rec new_rec;
						new_rec = r_rec;
						new_rec.ArID = destArID;
						STRNSCPY(new_rec.Code, r_rec.Code);
						THROW(ACodT.insertRecBuf(&new_rec));
						CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_COPIED, &msg_buf, r_rec.Code, r_rec.GoodsID, srcArID, destArID)));
					}
					// } @v11.7.5 
					else {
						THROW_DB(updateFor(&ACodT, 0, (ACodT.ArID == srcArID && ACodT.Code == r_rec.Code),
							set(ACodT.ArID, dbconst(destArID))));
						ok = 1;
						CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_MOVED, &msg_buf, r_rec.Code, r_rec.GoodsID, srcArID, destArID)));
					}
				}
			}
			THROW(tra.Commit());
		}
	}
	else if(grpID && destArID >= 0) {
		PPIDArray goods_list;
		ArGoodsCodeArray code_list;
		GoodsIterator::GetListByGroup(grpID, &goods_list);
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < goods_list.getCount(); i++) {
			const  PPID goods_id = goods_list.get(i);
			int r = ReadArCodesByAr(goods_id, destArID, &code_list);
			THROW(r);
			if(r > 0) {
				CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_EXISTS, &msg_buf, goods_id, destArID, code_list.at(0).Code)));
			}
			else if(PPObjGoods::GenerateOwnArCode(code_buf, 0) > 0) {
				ArGoodsCodeTbl::Rec new_rec;
				new_rec.GoodsID = goods_id;
				new_rec.ArID = destArID;
				code_buf.CopyTo(new_rec.Code, sizeof(new_rec.Code));
				THROW(ACodT.insertRecBuf(&new_rec));
				CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_MOVARCOD_ADDED, &msg_buf, goods_id, destArID, new_rec.Code)));
			}
			else {
				CALLPTRMEMB(pLogger, Log(PPLoadTextS(PPTXT_MOVARCOD_NGEN, msg_buf)));
				break;
			}
		}
		THROW(tra.Commit());
	}
	CATCH
		ok = 0;
		CALLPTRMEMB(pLogger, LogLastError());
	ENDCATCH
	return ok;
}
//
//
//
struct __GoodsStockExt {  // @persistent @store(PropertyTbl)
	struct PalletList {
		uint32 Count;
		GoodsStockExt::Pallet Item[1];
	};
	long    ObjType;      // const PPOBJ_GOODS
	long    ObjID;        // ->Goods2.ID
	long    Prop;         // const GDSPRP_STOCKDATA
	long    Brutto;       // Масса брутто, г (Если Package != 0, то масса упаковки)
	PPDimention PckgDim;  // Габаритные размеры упаковки поставки
	uint32  Reserve3;     //
	float   NettBruttCoeff; //
	double  Package;      // Емкость упаковки при поставке (торговых единиц)
	int16   ExpiryPeriod;
	int16   GseFlags;     //
	double  MinShippmQtty; //
	char    Reserve2[12];
	PalletList PltList;   // @last
};

int GoodsCore::PutStockExt(PPID id, const GoodsStockExt * pData, int use_ta)
{
	int    ok = 1;
	__GoodsStockExt * p_strg = 0;
	const RAssocArray * p_min_stock_list = 0;
	size_t sz = 0;
	if(pData) {
		sz = sizeof(__GoodsStockExt);
		const uint plt_c = pData->PltList.getCount();
		if(plt_c > 1)
			sz += sizeof(GoodsStockExt::Pallet) * (plt_c - 1);
		THROW_MEM(p_strg = static_cast<__GoodsStockExt *>(SAlloc::M(sz)));
		memzero(p_strg, sizeof(*p_strg));
		p_strg->ObjType  = PPOBJ_GOODS;
		p_strg->ObjID    = id;
		p_strg->Prop     = GDSPRP_STOCKDATA;
		p_strg->Brutto   = pData->Brutto;
		p_strg->PckgDim  = pData->PckgDim;
		p_strg->NettBruttCoeff = pData->NettBruttCoeff;
		p_strg->Package  = pData->Package;
		p_strg->ExpiryPeriod = pData->ExpiryPeriod;
		p_strg->GseFlags = pData->GseFlags;
		p_strg->MinShippmQtty = pData->MinShippmQtty;
		p_strg->PltList.Count = plt_c;
		for(uint i = 0; i < plt_c; i++) {
			p_strg->PltList.Item[i] = pData->PltList.at(i);
		}
		p_min_stock_list = &pData->MinStockList;
	}
	THROW(P_Ref->PutProp(PPOBJ_GOODS, id, GDSPRP_STOCKDATA, p_strg, sz, use_ta));
	THROW(P_Ref->PutPropArray(PPOBJ_GOODS, id, GDSPRP_MINSTOCKBYLOC, p_min_stock_list, use_ta));
	CATCHZOK
	SAlloc::F(p_strg);
	return ok;
}

int GoodsCore::GetStockExt(PPID id, GoodsStockExt * pData, int useCache /*=0*/)
{
	int    ok = -1;
	__GoodsStockExt * p_strg = 0;
	THROW_INVARG(pData);
	if(useCache) {
		ok = FetchStockExt(id, pData);
	}
	else {
		size_t sz = 0;
		uint   init_plt_c = 4;
		pData->Z();
		sz = sizeof(__GoodsStockExt) + (sizeof(GoodsStockExt::Pallet) * init_plt_c);
		THROW_MEM(p_strg = static_cast<__GoodsStockExt *>(SAlloc::M(sz)));
		memzero(p_strg, sizeof(*p_strg));
		THROW(ok = P_Ref->GetProperty(PPOBJ_GOODS, id, GDSPRP_STOCKDATA, p_strg, sz));
		if(ok > 0) {
			if(p_strg->PltList.Count > init_plt_c && p_strg->PltList.Count <= 8) {
				init_plt_c = p_strg->PltList.Count;
				sz = sizeof(__GoodsStockExt) + (sizeof(GoodsStockExt::Pallet) * init_plt_c);
				THROW_MEM(p_strg = static_cast<__GoodsStockExt *>(SAlloc::R(p_strg, sz)));
				THROW(P_Ref->GetProperty(PPOBJ_GOODS, id, GDSPRP_STOCKDATA, p_strg, sz) > 0);
			}
			// Защита от 'грязных' значений в базе данных {
			SETMAX(p_strg->Brutto, 0);
			SETMAX(p_strg->PckgDim.Length, 0);
			SETMAX(p_strg->PckgDim.Width, 0);
			SETMAX(p_strg->PckgDim.Height, 0);
			// }
			pData->Brutto   = p_strg->Brutto;
			pData->PckgDim  = p_strg->PckgDim;
			pData->NettBruttCoeff = p_strg->NettBruttCoeff;
			pData->Package  = (SIEEE754::IsValid(p_strg->Package) && p_strg->Package > 0) ? R6(p_strg->Package) : 0;
			pData->ExpiryPeriod = p_strg->ExpiryPeriod;
			pData->GseFlags      = p_strg->GseFlags;
			pData->MinShippmQtty = p_strg->MinShippmQtty;
			for(uint i = 0; i < p_strg->PltList.Count; i++) {
				THROW_SL(pData->PltList.insert(&p_strg->PltList.Item[i]));
			}
			if(P_Ref->GetPropArray(PPOBJ_GOODS, id, GDSPRP_MINSTOCKBYLOC, &pData->MinStockList) > 0) {
				uint min_stock_count = pData->MinStockList.getCount();
				for(uint i = 0; i < min_stock_count; i++) {
					double min_stock = pData->MinStockList.at(i).Val;
					pData->MinStockList.at(i).Val = (SIEEE754::IsValid(min_stock) && min_stock > 0) ? R6(min_stock) : 0;
				}
			}
		}
	}
	CATCHZOK
	SAlloc::F(p_strg);
	return ok;
}

int GoodsCore::GetListByBarcodeLen(const PPIDArray * pLens, PPIDArray & rList)
{
	rList.clear();

	int    ok = -1;
	PPIDArray lens(*pLens), temp_list;
	lens.sort();
	int    has_zero_len = BIN(lens.bsearch(0));
	if(lens.getCount()) {
		PROFILE_START
		BExtQuery q(&BCTbl, 0);
		q.selectAll();
		BarcodeTbl::Key0 k0;
		MEMSZERO(k0);
		for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
			// С символа '@' начинаются коды товарных групп
			// @v5.9.9 VADIM - с символа '$' начинаются артикулы товаров (ИД импортированных товаров)
			if(BCTbl.data.Code[0] != '@' && BCTbl.data.Code[0] != '$' && lens.bsearch(sstrlen(BCTbl.data.Code)))
				THROW(rList.add(BCTbl.data.GoodsID));
			if(has_zero_len)
				THROW(temp_list.add(BCTbl.data.GoodsID));
		}
		temp_list.sortAndUndup();
		ok = 1;
		PROFILE_END
	}
	if(has_zero_len) {
		PROFILE_START
		BExtQuery q(this, 2, 256);
		q.select(this->ID, 0L).where(this->Kind == PPGDSK_GOODS);
		Goods2Tbl::Key2 k2;
		MEMSZERO(k2);
		k2.Kind = PPGDSK_GOODS;
		for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
			if(!temp_list.bsearch(data.ID)) {
				THROW(rList.add(data.ID));
			}
		}
		ok = 1;
		PROFILE_END
	}
	CATCHZOK
	rList.sortAndUndup();
	return ok;
}

int GoodsCore::GetListBySubstring(const char * pSubstr, PPIDArray * pList, int skipPassive, int srchByExtStr)
{
	long flags = 0;
	if(skipPassive < 0)
		flags |= glsfDefPassive;
	else if(skipPassive)
		flags |= glsfSkipPassive;
	if(srchByExtStr)
		flags |= glsfByExtStr;
	return Helper_GetListBySubstring(pSubstr, pList, flags);
}

int GoodsCore::GetListBySubstring(const char * pSubstr, StrAssocArray * pList, int skipPassive, int srchByExtStr)
{
	long flags = glsfStrList;
	if(skipPassive < 0)
		flags |= glsfDefPassive;
	else if(skipPassive)
		flags |= glsfSkipPassive;
	if(srchByExtStr)
		flags |= glsfByExtStr;
	int    ok = Helper_GetListBySubstring(pSubstr, pList, flags);
	pList->SortByText();
	return ok;
}

struct PPTextSrchPatternEntry {
	PPTextSrchPatternEntry() : P_Sp(0), P_ApxP(0), Kind(kText)
	{
	}
	~PPTextSrchPatternEntry()
	{
		delete P_Sp;
		delete P_ApxP;
	}
	enum {
		kText    = 0,
		kLinkOr  = 1,
		kLinkAnd = 2,
		kApproxText = 0x0100,
		kFromStart  = 0x0200,
	};
	int    Kind;
	SString Text;
	SSrchPattern * P_Sp;
	ApproxStrSrchParam * P_ApxP;
};

class PPTextSrchPattern : public TSCollection <PPTextSrchPatternEntry> {
public:
	PPTextSrchPattern() : TSCollection <PPTextSrchPatternEntry>()
	{
	}
	int    Init(const char * pPattern)
	{
		freeAll();
		return Helper_Init(pPattern);
	}
	int    Detect(const char * pText)
	{
		return getCount() ? Helper_Detect(pText, sstrlen(pText), 0) : 0;
	}
private:
	int    Helper_Detect(const char * pText, size_t textLen, uint entryIdx)
	{
		int    ok = 0;
		const uint _c = getCount();
		if(textLen && entryIdx < _c) {
			const PPTextSrchPatternEntry * p_entry = at(entryIdx);
			if(p_entry->Kind & PPTextSrchPatternEntry::kFromStart) {
				if(strncmp(p_entry->Text, pText, p_entry->Text.Len()) == 0)
					ok = 1;
			}
			else if(p_entry->Kind & PPTextSrchPatternEntry::kApproxText) {
				if(ApproxStrSrch(p_entry->Text, pText, p_entry->P_ApxP) > 0)
					ok = 1;
			}
			else if(p_entry->Kind == PPTextSrchPatternEntry::kText) {
				size_t fp = 0;
                if(p_entry->P_Sp->Search(pText, 0, textLen, &fp))
					ok = 1;
			}
			if((entryIdx+1) < _c) {
				const PPTextSrchPatternEntry * p_next_entry = at(entryIdx+1);
				if(p_next_entry->Kind == PPTextSrchPatternEntry::kLinkAnd) {
					if(ok) {
						ok = Helper_Detect(pText, textLen, entryIdx+2); // @recursion
					}
				}
				else if(p_next_entry->Kind == PPTextSrchPatternEntry::kLinkOr) {
					if(!ok) {
						ok = Helper_Detect(pText, textLen, entryIdx+2); // @recursion
					}
				}
			}
		}
		return ok;
	}
    int    Helper_Init(const char * pPattern)
    {
		int    ok = 1;
		SString temp_buf;
		uint   p = 0;
		int    done = 0;
		// div: 1 - OR, 2 - AND
		for(int div = 0; !div && pPattern[p]; p++) {
			const  char c = pPattern[p];
			uint   inc = 0;
			if(c == '|' && pPattern[p+1] == '|') {
				if(p && pPattern[p-1] == '\\') {
					temp_buf.TrimRight().CatCharN('|', 2);
				}
				else {
					inc = 2;
					div = 1;
				}
			}
			else if(c == '&' && pPattern[p+1] == '&') {
				if(p && pPattern[p-1] == '\\') {
					temp_buf.TrimRight().CatCharN('&', 2);
				}
				else {
					inc = 2;
					div = 2;
				}
			}
			if(div) {
                THROW(Helper_Init(temp_buf)); // @recursion
				if(div == 1) {
					PPTextSrchPatternEntry * p_new_entry = CreateNewItem();
					THROW_SL(p_new_entry);
					p_new_entry->Kind = PPTextSrchPatternEntry::kLinkOr;
				}
				else if(div == 2) {
					PPTextSrchPatternEntry * p_new_entry = CreateNewItem();
					THROW_SL(p_new_entry);
					p_new_entry->Kind = PPTextSrchPatternEntry::kLinkAnd;
				}
				THROW(Helper_Init(pPattern+p+inc)); // @recursion
				done = 1;
			}
			else
				temp_buf.CatChar(c);
		}
		if(!done) {
			const char * p_srch_str = temp_buf;
			PPTextSrchPatternEntry * p_new_entry = CreateNewItem();
			THROW_SL(p_new_entry);
			if(p_srch_str[0] == '%' && p_srch_str[1] == '^') {
				p_new_entry->Kind |= PPTextSrchPatternEntry::kFromStart;
				p_new_entry->Text = (p_srch_str+2);
			}
			else if(*p_srch_str == '!' && *++p_srch_str != '!') {
				p_new_entry->Kind |= PPTextSrchPatternEntry::kApproxText;
				THROW_SL(p_new_entry->P_ApxP = new ApproxStrSrchParam);
				p_new_entry->P_ApxP->weight = 1;
				p_new_entry->P_ApxP->method = 1;
				p_new_entry->P_ApxP->no_case = 1;
				if(*p_srch_str == '(') {
					int i, j;
					for(i = j = 0, p_srch_str++; i < 2 && *p_srch_str != ')'; i++, p_srch_str++) {
						THROW(isdec(*p_srch_str));
						j = j * 10 + *p_srch_str - '0';
					}
					THROW(*p_srch_str == ')');
					p_srch_str++;
					p_new_entry->P_ApxP->umin = fdiv100i(j);
				}
				else
					p_new_entry->P_ApxP->umin = 0.75;
				p_new_entry->Text = p_srch_str;
			}
			else {
				p_new_entry->Kind = PPTextSrchPatternEntry::kText;
				p_new_entry->Text = p_srch_str;
				THROW_SL(p_new_entry->P_Sp = new SSrchPattern(p_srch_str));
			}
		}
		CATCHZOK
		return ok;
    }
};


int GoodsCore::Helper_GetListBySubstring(const char * pSubstr, void * pList, long flags)
{
	int    ok = 1;
	bool   skip_passive = false;
	bool   skip_generic = false;
	PPIDArray * p_list = 0;
	StrAssocArray * p_str_list = 0;
	SString pattern;
	(pattern = pSubstr).ToLower();
	if(flags & glsfStrList)
		p_str_list = static_cast<StrAssocArray *>(pList);
	else
		p_list = static_cast<PPIDArray *>(pList);
	if(flags & glsfDefPassive) {
		PPObjGoods goods_obj;
		skip_passive = LOGIC(goods_obj.GetConfig().Flags & GCF_DONTSELPASSIVE);
	}
	else if(flags & glsfSkipPassive)
		skip_passive = true;
	if(flags & glsfDefGeneric) {
		//PPObjGoods goods_obj;
		//skip_generic = BIN(goods_obj.GetConfig().Flags & GCF_DONTSELPASSIVE);
	}
	else if(flags & glsfSkipGeneric)
		skip_generic = true;
	const StrAssocArray * p_full_list = GetFullList();
	if(p_full_list) {
		PPUserFuncProfiler ufp(PPUPRF_SRCHINGOODSFL);
		Goods2Tbl::Rec goods_rec;
		uint   result_count = 0;
		const uint c = p_full_list->getCount();
		PPTextSrchPattern tsp;
		tsp.Init(pattern);
		for(uint i = 0; ok && i < c; i++) {
			StrAssocArray::Item item = p_full_list->at_WithoutParent(i);
			int r;
			PROFILE_START
			//r = ExtStrSrch(item.Txt, pattern, essfCaseSensitive); // @v10.1.4 essfCaseSensitive
			r = tsp.Detect(item.Txt);
			PROFILE_END
			PROFILE_START
			if(r > 0 && (!(skip_passive || skip_generic) || Fetch(item.Id, &goods_rec) > 0)) {
				if((!skip_passive || !(goods_rec.Flags & GF_PASSIV)) && (!skip_generic || !(goods_rec.Flags & GF_GENERIC))) {
					result_count++;
					if(p_list) {
						if(!p_list->add(item.Id)) { // @v10.1.4 addUnique-->add
							//
							// Здесь THROW не годится из-за того, что сразу после завершения цикла необходимо быстро сделать ReleaseFullList
							//
							ok = PPSetErrorSLib();
						}
					}
					else if(p_str_list) {
						p_str_list->Add(item.Id, item.Txt);
					}
				}
			}
			PROFILE_END
		}
		ReleaseFullList(p_full_list);
		p_full_list = 0;
		ufp.SetFactor(0, c);
		ufp.SetFactor(1, result_count);
		ufp.Commit();
	}
	else {
		int    r = 0;
		PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
		if(p_cli) {
			SString q;
			q.Cat("SELECT").Space().Cat("GOODS").Space().Cat("BY").Space().Cat("SUBNAME").CatParStr(pSubstr).Space();
			if(skip_passive)
				q.Cat("PASSIVE").CatParStr("no").Space();
			q.Cat("FORMAT").DotCat("BIN").CatParStr(static_cast<const char *>(0));
			PPJobSrvReply reply;
			if(p_cli->ExecSrvCmd(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				if(p_list) {
					StrAssocArray temp_str_list;
					temp_str_list.Read(reply, 0);
					for(uint i = 0; i < temp_str_list.getCount(); i++) {
						StrAssocArray::Item item = temp_str_list.at_WithoutParent(i);
						THROW_SL(p_list->add(item.Id)); // @v10.1.4 addUnique-->add
					}
				}
				else if(p_str_list) {
					p_str_list->Read(reply, 0);
					p_str_list->ClearParents();
				}
				r = 1;
			}
		}
		if(r <= 0) {
			BExtQuery q(this, 2);
			q.select(this->ID, this->Name, this->Flags, 0L).where(this->Kind == PPGDSK_GOODS);
			Goods2Tbl::Key2 k2;
			MEMSZERO(k2);
			k2.Kind = PPGDSK_GOODS;
			PPTextSrchPattern tsp;
			tsp.Init(pattern);
			SString text_buf;
			for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
				if(!(skip_passive && data.Flags & GF_PASSIV)) {
					(text_buf = data.Name).ToLower();
					//int r = ExtStrSrch(text_buf, pSubstr, 0);
					int r = tsp.Detect(text_buf);
					if(r > 0) {
						if(p_list) {
							THROW_SL(p_list->add(data.ID)); // @v10.1.4 addUnique-->add
						}
						else if(p_str_list) {
							THROW_SL(p_str_list->Add(data.ID, data.Name));
						}
					}
				}
			}
		}
	}
	if(flags & glsfByExtStr) {
		SString ext_str;
		PropertyTbl::Key0 k;
		BExtQuery q(&P_Ref->Prop, 0);
		k.ObjType = PPOBJ_GOODS;
		k.ObjID   = 0;
		k.Prop    = GDSPRP_EXTSTRDATA;
		q.select(P_Ref->Prop.ObjID, 0L).where(P_Ref->Prop.ObjType == PPOBJ_GOODS && P_Ref->Prop.Prop == GDSPRP_EXTSTRDATA);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			PPID   goods_id = P_Ref->Prop.data.ObjID;
			Goods2Tbl::Rec goods_rec;
			if(Fetch(goods_id, &goods_rec) > 0 && !(skip_passive && data.Flags & GF_PASSIV)) {
				if(P_Ref->GetPropVlrString(PPOBJ_GOODS, goods_id, GDSPRP_EXTSTRDATA, ext_str) > 0) {
					if(ExtStrSrch(ext_str, pSubstr, 0)) {
						if(p_list) {
							THROW_SL(p_list->add(goods_id)); // @v10.1.4 addUnique-->add
						}
						else if(p_str_list) {
							THROW_SL(p_str_list->Add(goods_id, goods_rec.Name));
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::GetListByBrandList(const PPIDArray & rBrandList, PPIDArray & rGoodsList)
{
	rGoodsList.clear();
	int    ok = 1;
	int    idx = 2;
	union {
		Goods2Tbl::Key2 k2;
		Goods2Tbl::Key3 k3;
	} k;
	const  PPID single_brand_id = (rBrandList.getCount() == 1) ? rBrandList.get(0) : 0;
	DBQ * dbq = &(this->Kind == PPGDSK_GOODS);
	MEMSZERO(k);
	if(single_brand_id) {
		dbq = &(*dbq && this->BrandID == single_brand_id);
		idx = 3;
		k.k3.Kind = PPGDSK_GOODS;
		k.k3.BrandID = single_brand_id;
	}
	else {
		idx = 2;
		k.k2.Kind = PPGDSK_GOODS;
	}
	BExtQuery q(this, idx, 48);
	q.select(this->ID, this->BrandID, 0L).where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
		if(single_brand_id && data.BrandID == single_brand_id) {
			THROW_SL(rGoodsList.add(data.ID));
		}
		else if(rBrandList.bsearch(data.BrandID, 0)) {
			THROW_SL(rGoodsList.add(data.ID));
		}
	}
	CATCHZOK
	rGoodsList.sortAndUndup();
	return ok;
}

int GoodsCore::Search(PPID id, void * b) { return SearchByID(this, PPOBJ_GOODS, id, b); }

int GoodsCore::GetExt(PPID id, GoodsExtTbl::Rec * pRec)
{
	int    r = SearchByID(&GeT, PPOBJ_GOODS, id, pRec);
	if(r <= 0)
		memzero(pRec, sizeof(*pRec));
	return r;
}

int GoodsCore::PutExt(PPID id, GoodsExtTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	PPID   k = id;
	long   cls_flags = 0;
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket gc_pack;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pRec && pRec->GoodsClsID) {
			if(gc_obj.Fetch(pRec->GoodsClsID, &gc_pack) > 0)
				cls_flags = gc_pack.Rec.Flags;
			pRec->GoodsID = id;
			if(cls_flags & PPGdsCls::fDupCombine) {
				long   c = pRec->UniqCntr;
				GoodsExtTbl::Key1 k1;
				k1.GoodsClsID = pRec->GoodsClsID;
   				k1.KindID  = pRec->KindID;
	   			k1.GradeID = pRec->GradeID;
				k1.AddObjID  = pRec->AddObjID;
				k1.AddObj2ID = pRec->AddObj2ID;
		   		k1.X = pRec->X;
				k1.Y = pRec->Y;
				k1.Z = pRec->Z;
				k1.W = pRec->W;
				do {
					k1.UniqCntr = c++;
				} while(GeT.search(1, &k1, spEq) && GeT.data.GoodsID != pRec->GoodsID);
				pRec->UniqCntr = c-1;
			}
		}
		if(GeT.searchForUpdate(0, &k, spEq)) {
			if(pRec) {
				THROW_DB(GeT.updateRecBuf(pRec)); // @sfu
			}
			else
				THROW_DB(GeT.deleteRec()); // @sfu
		}
		else if(pRec) {
			pRec->GoodsID = id;
			THROW_DB(GeT.insertRecBuf(pRec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::ReplaceExtDimScale(PPID clsID, int gcDim, long oldScale, long newScale, int use_ta)
{
	int    ok = -1;
	GoodsExtTbl::Key1 k1;
	GoodsExtTbl::Rec rec;
	PPIDArray goods_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			MEMSZERO(k1);
			k1.GoodsClsID = clsID;
			if(GeT.search(1, &k1, spGt) && GeT.data.GoodsClsID == clsID) do {
				int    do_update = 0;
				GeT.copyBufTo(&rec);
				if(gcDim == PPGdsCls::eX && rec.X)
					do_update = 1;
				else if(gcDim == PPGdsCls::eY && rec.Y)
					do_update = 1;
				else if(gcDim == PPGdsCls::eZ && rec.Z)
					do_update = 1;
				else if(gcDim == PPGdsCls::eW && rec.W)
					do_update = 1;
				if(do_update)
					goods_list.add(GeT.data.GoodsID);
			} while(GeT.search(1, &k1, spNext) && GeT.data.GoodsClsID == clsID);
		}
		if(goods_list.getCount()) {
			goods_list.sortAndUndup();
			for(uint i = 0; i < goods_list.getCount(); i++) {
				const  PPID goods_id = goods_list.get(i);
				GoodsExtTbl::Key0 k0;
				MEMSZERO(k0);
				k0.GoodsID = goods_id;
				if(GeT.searchForUpdate(0, &k0, spEq)) {
					int    do_update = 0;
					GeT.copyBufTo(&rec);
					if(gcDim == PPGdsCls::eX && rec.X) {
						double val =  static_cast<double>(rec.X) / fpow10i((int)oldScale);
						rec.X = static_cast<long>(val * fpow10i((int)newScale));
						do_update = 1;
					}
					else if(gcDim == PPGdsCls::eY && rec.Y) {
						double val =  static_cast<double>(rec.Y) / fpow10i((int)oldScale);
						rec.Y = static_cast<long>(val * fpow10i((int)newScale));
						do_update = 1;
					}
					else if(gcDim == PPGdsCls::eZ && rec.Z) {
						double val =  static_cast<double>(rec.Z) / fpow10i((int)oldScale);
						rec.Z = static_cast<long>(val * fpow10i((int)newScale));
						do_update = 1;
					}
					else if(gcDim == PPGdsCls::eW && rec.W) {
						double val =  static_cast<double>(rec.W) / fpow10i((int)oldScale);
						rec.W = static_cast<long>(val * fpow10i((int)newScale));
						do_update = 1;
					}
					if(do_update) {
						int r = PutExt(goods_id, &rec, 0);
						THROW_DB(r);
						ok = 1;
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::ReplaceExtObjRefs(PPID clsID, int gcProp, const LAssocArray * pSubstList, int use_ta)
{
	int    ok = 1;
	SArray temp_list(sizeof(GoodsExtTbl::Rec));
	GoodsExtTbl::Key1 k1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k1);
		k1.GoodsClsID = clsID;
		if(GeT.search(1, &k1, spGt) && GeT.data.GoodsClsID == clsID) do {
			int32 * p_prop_id = 0;
			if(gcProp == PPGdsCls::eKind)
				p_prop_id = &GeT.data.KindID;
			else if(gcProp == PPGdsCls::eGrade)
				p_prop_id = &GeT.data.GradeID;
			else if(gcProp == PPGdsCls::eAdd)
				p_prop_id = &GeT.data.AddObjID;
			else if(gcProp == PPGdsCls::eAdd2)
				p_prop_id = &GeT.data.AddObj2ID;
			if(p_prop_id && *p_prop_id) {
				long   new_id = 0;
				*p_prop_id = (pSubstList->Search(GeT.data.GoodsID, &new_id, 0)) ? new_id : 0;
				THROW_SL(temp_list.insert(&GeT.data));
				THROW_DB(GeT.deleteRec());
				//THROW_DB(GeT.updateRec());
			}
		} while(GeT.search(1, &k1, spNext) && GeT.data.GoodsClsID == clsID);
		{
			GoodsExtTbl::Rec * p_rec;
			for(uint i = 0; temp_list.enumItems(&i, (void **)&p_rec);)
				THROW_DB(GeT.insertRecBuf(p_rec));
			/*
			BExtInsert bei(&GeT);
			for(uint i = 0; temp_list.enumItems(&i, (void **)&p_rec);) {
				THROW_DB(bei.insert(p_rec));
			}
			THROW_DB(bei.flash());
			*/
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::GetExtPropRefList(PPID clsID, int gcProp, PPID propVal, LAssocArray * pGoodsList)
{
	int    ok = -1;
	BExtQuery q(&GeT, 1);
	DBQ * dbq = 0;
	GoodsExtTbl::Key1 k1;
	MEMSZERO(k1);
	k1.GoodsClsID = clsID;
	q.select(GeT.GoodsID, GeT.KindID, GeT.GradeID, GeT.AddObjID, GeT.AddObj2ID, 0L);
	dbq = &(*dbq && GeT.GoodsClsID == clsID);
	if(propVal)
		if(gcProp == PPGdsCls::eKind)
			dbq = &(*dbq && GeT.KindID == propVal);
		else if(gcProp == PPGdsCls::eGrade)
			dbq = &(*dbq && GeT.GradeID == propVal);
		else if(gcProp == PPGdsCls::eAdd)
			dbq = &(*dbq && GeT.AddObjID == propVal);
		else if(gcProp == PPGdsCls::eAdd2)
			dbq = &(*dbq && GeT.AddObj2ID == propVal);
	q.where(*dbq);
	for(q.initIteration(false, &k1, spGe); q.nextIteration() > 0;) {
		if(gcProp == PPGdsCls::eKind) {
			if(GeT.data.KindID && (!propVal || GeT.data.KindID == propVal)) {
				ok = 1;
				if(pGoodsList)
					pGoodsList->Add(GeT.data.GoodsID, GeT.data.KindID, 0);
				else
					break;
			}
		}
		else if(gcProp == PPGdsCls::eGrade) {
			if(GeT.data.GradeID && (!propVal || GeT.data.GradeID == propVal)) {
				ok = 1;
				if(pGoodsList)
					pGoodsList->Add(GeT.data.GoodsID, GeT.data.GradeID, 0);
				else
					break;
			}
		}
		else if(gcProp == PPGdsCls::eAdd) {
			if(GeT.data.AddObjID && (!propVal || GeT.data.AddObjID == propVal)) {
				ok = 1;
				if(pGoodsList)
					pGoodsList->Add(GeT.data.GoodsID, GeT.data.AddObjID, 0);
				else
					break;
			}
		}
		else if(gcProp == PPGdsCls::eAdd2) {
			if(GeT.data.AddObj2ID && (!propVal || GeT.data.AddObj2ID == propVal)) {
				ok = 1;
				if(pGoodsList)
					pGoodsList->Add(GeT.data.GoodsID, GeT.data.AddObj2ID, 0);
				else
					break;
			}
		}
	}
	return ok;
}

int GoodsCore::SearchAnyDynObjRef(PPID objType, PPID objID, PPID * pID)
{
	PPObjGoodsClass gc_obj;
	PPGdsClsPacket gc_pack;
	PPGdsCls gc_rec;
	PPID   cls_id = 0;
	while(gc_obj.EnumItems(&cls_id, &gc_rec) > 0) {
		if(gc_obj.Fetch(cls_id, &gc_pack) > 0) {
			if(gc_pack.PropKind.ItemsListID == objType) {
				LAssocArray list;
				if(GetExtPropRefList(cls_id, PPGdsCls::eKind, objID, &list) > 0) {
					if(list.getCount())
						ASSIGN_PTR(pID, list.at(0).Key);
					return 1;
				}
			}
			if(gc_pack.PropGrade.ItemsListID == objType) {
				LAssocArray list;
				if(GetExtPropRefList(cls_id, PPGdsCls::eGrade, objID, &list) > 0) {
					if(list.getCount())
						ASSIGN_PTR(pID, list.at(0).Key);
					return 1;
				}
			}
			if(gc_pack.PropAdd.ItemsListID == objType) {
				LAssocArray list;
				if(GetExtPropRefList(cls_id, PPGdsCls::eAdd, objID, &list) > 0) {
					if(list.getCount())
						ASSIGN_PTR(pID, list.at(0).Key);
					return 1;
				}
			}
			if(gc_pack.PropAdd2.ItemsListID == objType) {
				LAssocArray list;
				if(GetExtPropRefList(cls_id, PPGdsCls::eAdd2, objID, &list) > 0) {
					if(list.getCount())
						ASSIGN_PTR(pID, list.at(0).Key);
					return 1;
				}
			}
		}
	}
	return -1;
}

int GoodsCore::SearchAnyRef(PPID objType, PPID objID, PPID *pID)
{
	if(oneof2(objType, PPOBJ_GOODS, PPOBJ_GOODSGROUP)) {
		Goods2Tbl::Key1 k;
		MEMSZERO(k);
		k.Kind = PPGDSK_GROUP;
		k.ParentID = objID;
		if(search(1, &k, spGe) && k.Kind == PPGDSK_GROUP && k.ParentID == objID) {
			ASSIGN_PTR(pID, data.ID);
			return 1;
		}
		else {
			MEMSZERO(k);
			k.Kind = PPGDSK_GOODS;
			k.ParentID = objID;
			if(search(1, &k, spGe) && k.Kind == PPGDSK_GOODS && k.ParentID == objID) {
				ASSIGN_PTR(pID, data.ID);
				return 1;
			}
		}
	}
	else if(objType == PPOBJ_GOODSSTRUC) {
		Goods2Tbl::Key5 k5;
		MEMSZERO(k5);
		k5.StrucID = objID;
		if(search(5, &k5, spEq)) {
			ASSIGN_PTR(pID, data.ID);
			return 1;
		}
	}
	else if(IS_DYN_OBJTYPE(objType)) {
		return SearchAnyDynObjRef(objType, objID, pID);
	}
	else {
		DBQ * dbq = 0;
		if(objType == PPOBJ_UNIT)
			dbq = & (this->UnitID == objID || this->PhUnitID == objID);
		else if(objType == PPOBJ_PERSON)
			dbq = & (this->ManufID == objID);
		else if(objType == PPOBJ_GOODSTAX)
			dbq = & (this->TaxGrpID == objID);
		else if(objType == PPOBJ_GOODSTYPE)
			dbq = & (this->GoodsTypeID == objID);
		else if(objType == PPOBJ_GOODSSTRUC)
			dbq = & (this->StrucID == objID);
		else if(objType == PPOBJ_GOODSCLASS)
			dbq = & (this->GdsClsID == objID);
		else if(objType == PPOBJ_BRAND)
			dbq = & (this->BrandID == objID);
		else
			return -1;
		{
			Goods2Tbl::Key0 k0;
			BExtQuery q(this, 0);
			q.select(this->ID, 0L).where(*dbq);
			k0.ID = 0;
			if(q.fetchFirst(&k0, spGt) > 0) {
				ASSIGN_PTR(pID, data.ID);
				return 1;
			}
		}
	}
	return -1;
}

int GoodsCore::SearchByName(long kind, const char * pName, PPID * pID, Goods2Tbl::Rec * pRec)
{
	Goods2Tbl::Key2 k;
	MEMSZERO(k);
	k.Kind = kind;
	strip(STRNSCPY(k.Name, pName));
	int    ok = SearchByKey(this, 2, &k, pRec);
	ASSIGN_PTR(pID, (ok > 0) ? data.ID : 0);
	return ok;
}

int GoodsCore::ReadBarcodes(PPID id, BarcodeArray & rCodeList)
{
	int    ok = 1;
	rCodeList.clear();
	if(id) {
		id = labs(id);
		SString barcode;
		BarcodeTbl::Key1 k;
		BarcodeTbl::Rec rec;
		k.GoodsID = id;
		if(BCTbl.search(1, &k, spEq))
			do {
				BCTbl.copyBufTo(&rec);
				int    is_gds_article = 0;
				if(rec.Code[0] == '$' && (barcode = (rec.Code + 1)).IsDec() && IsInnerBarcodeType(rec.BarcodeType, BARCODE_TYPE_UNDEF))
					is_gds_article = 1;
				if(!is_gds_article && !rCodeList.insert(&rec))
					ok = PPSetErrorSLib();
			} while(ok && BCTbl.search(1, &k, spNext) && k.GoodsID == id);
		if(!BTROKORNFOUND)
			ok = PPSetErrorDB();
		rCodeList.Arrange();
	}
	return ok;
}

int GoodsCore::Helper_ReadArCodes(PPID goodsID, PPID arID, ArGoodsCodeArray * pCodeList, PPIDArray * pIdList)
{
	int    ok = -1;
	CALLPTRMEMB(pCodeList, clear());
	CALLPTRMEMB(pIdList, clear());
	if(goodsID == 0 && arID >= 0) {
		ArGoodsCodeTbl::Key0 k0;
		MEMSZERO(k0);
		k0.ArID = arID;
		BExtQuery q(&ACodT, 0);
		if(pCodeList == 0)
			q.select(ACodT.GoodsID, 0);
		else
			q.selectAll();
		q.where(ACodT.ArID == arID);
		for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
			THROW_SL(!pCodeList || pCodeList->insert(&ACodT.data));
			THROW_SL(!pIdList || pIdList->add(ACodT.data.GoodsID));
			ok = 1;
		}
	}
	else if(goodsID) {
		ArGoodsCodeTbl::Key2 k2;
		MEMSZERO(k2);
		k2.GoodsID = goodsID;
		if(arID >= 0)
			k2.ArID = arID;
		if(ACodT.search(2, &k2, spGe) && k2.GoodsID == goodsID) do {
			if(arID < 0 || k2.ArID == arID) {
				if(pCodeList)
					THROW_SL(pCodeList->insert(&ACodT.data));
				if(pIdList)
					THROW_SL(pIdList->add(ACodT.data.ArID));
				ok = 1;
			}
		} while(ACodT.search(2, &k2, spNext) && k2.GoodsID == goodsID);
		THROW_DB(BTROKORNFOUND);
	}
	CATCHZOK
	CALLPTRMEMB(pIdList, sortAndUndup());
	return ok;
}

int GoodsCore::ReadArCodes(PPID goodsID, ArGoodsCodeArray * pCodeList)
	{ return Helper_ReadArCodes(goodsID, -1, pCodeList, 0); }
int GoodsCore::ReadArCodesByAr(PPID goodsID, PPID arID, ArGoodsCodeArray * pCodeList)
	{ return Helper_ReadArCodes(goodsID, arID, pCodeList, 0); }
int GoodsCore::GetListByAr(PPID codeArID, PPIDArray * pList)
	{ return Helper_ReadArCodes(0, codeArID, 0, pList); }

int GoodsCore::GetSingleBarcode(PPID goodsID, uint sifFlags/*BarcodeArray::sifXXX*/, SString & rBuf)
{
	rBuf.Z();
	BarcodeArray codes;
	return ReadBarcodes(goodsID, codes) ? codes.GetSingle(sifFlags, rBuf) : 0;
}

int GoodsCore::SearchBarcode(const char * pCode, BarcodeTbl::Rec * pBcRec)
{
	BarcodeTbl::Key0 k;
	MEMSZERO(k);
	strip(STRNSCPY(k.Code, pCode));
	int    ok = SearchByKey(&BCTbl, 0, &k, pBcRec);
	if(ok < 0)
		PPSetError(PPERR_BARCODENFOUND, pCode);
	return ok;
}

int GoodsCore::SearchByBarcode(const char * pBarcode, BarcodeTbl::Rec * pBcRec, Goods2Tbl::Rec * pGoodsRec)
{
	BarcodeTbl::Rec bc_rec;
	int    ok = SearchBarcode(pBarcode, &bc_rec);
	if(ok > 0) {
		int    r = Search(bc_rec.GoodsID, pGoodsRec);
		if(r < 0)
			ok = PPSetError(PPERR_BL_BARCODE2GOODS);
		else if(!r)
			ok = 0;
		else {
			SString msg_buf;
			PPSetError(PPERR_BARCODEEXISTS, msg_buf.Cat(pBarcode).CatDiv('-', 1).Cat(data.Name));
		}
		ASSIGN_PTR(pBcRec, bc_rec);
	}
	return ok;
}

int GoodsCore::SearchBarcodeSubstr(const char * substr, BarcodeArray * codes)
{
	int    ok = -1;
	BarcodeTbl::Key0 k;
	BExtQuery q(&BCTbl, 0, 64);
	q.selectAll();
	MEMSZERO(k);
	for(q.initIteration(false, &k, spFirst); q.nextIteration() > 0;) {
		if(strstr(BCTbl.data.Code, substr)) {
			THROW_SL(codes->insert(&BCTbl.data));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SearchBarcodeSubstrExt(const char * pText, BarcodeArray * pCodes)
{
	int    ok = -1;
	SString text_buf(pText);
	text_buf.Strip();
	if(text_buf[0] == '*') {
		text_buf.ShiftLeft(1).Strip();
		ok = SearchBarcodeSubstr(text_buf, pCodes);
	}
	else {
		BarcodeTbl::Key0 k0;
		MEMSZERO(k0);
		text_buf.CopyTo(k0.Code, sizeof(k0.Code));
		BExtQuery q(&BCTbl, 0, 64);
		q.selectAll();
		for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
			if(text_buf.CmpL(BCTbl.data.Code, 0) == 0) {
				THROW_SL(pCodes->insert(&BCTbl.data));
				ok = 1;
			}
			else
				break;
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SearchArCodeSubstr(PPID arID, const char * substr, BarcodeArray * codes)
{
	int    ok = -1;
	union {
		ArGoodsCodeTbl::Key0 k0;
		ArGoodsCodeTbl::Key1 k1;
	} k;
	int    idx = 0, sp;
	DBQ  * dbq = 0;
	BExtQuery q(&ACodT, 0, 64);
	q.selectAll();
	MEMSZERO(k);
	if(arID > 0 || arID == -1) {
		idx = 0;
		sp = spGe;
		k.k0.ArID = arID;
		if(arID == -1)
			dbq = &(ACodT.ArID == 0L);
		else
			dbq = &(ACodT.ArID == arID);
	}
	else {
		idx = 1;
		sp = spFirst;
	}
	for(q.initIteration(false, &k, sp); q.nextIteration() > 0;) {
		if(strstr(ACodT.data.Code, substr)) {
			BarcodeTbl::Rec bc_rec;
			bc_rec.GoodsID = ACodT.data.GoodsID;
			bc_rec.Qtty = ACodT.data.Pack;
			bc_rec.BarcodeType = NZOR(ACodT.data.ArID, -1);
			STRNSCPY(bc_rec.Code, ACodT.data.Code);
			THROW_SL(codes->insert(&bc_rec));
			if(ok < 0)
				ok = 1;
			else
				ok++;
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SearchByArCode(PPID arID, const char * pBarcode, ArGoodsCodeTbl::Rec * pArCodeRec, Goods2Tbl::Rec * pGoodsRec)
{
	int    ok = -1;
	SString msg_buf;
	ArGoodsCodeTbl::Key0 k0;
	ArGoodsCodeTbl::Rec arcode_rec;
	k0.ArID = arID;
	STRNSCPY(k0.Code, pBarcode);
	ok = ACodT.search(0, &k0, spEq);
	if(ok > 0) {
		int    r = 0;
		arcode_rec = ACodT.data;
		r = Search(arcode_rec.GoodsID, pGoodsRec);
		if(r < 0)
			ok = PPSetError(PPERR_BL_ARCODE2GOODS, msg_buf.Cat(arID));
		else if(!r)
			ok = 0;
		else
			PPSetError(PPERR_BARCODEEXISTS, msg_buf.Cat(pBarcode).CatDiv('-', 1).Cat(data.Name));
		ASSIGN_PTR(pArCodeRec, arcode_rec);
	}
	return ok;
}

int GoodsCore::GetArCode(PPID arID, PPID goodsID, SString & rCode, int32 * pPack)
{
	int    ok = -1;
	rCode.Z();
	ArGoodsCodeTbl::Key2 k2;
	k2.ArID    = arID;
	k2.GoodsID = goodsID;
	if(ACodT.search(2, &k2, spEq) > 0) {
		rCode = ACodT.data.Code;
		ASSIGN_PTR(pPack, ACodT.data.Pack);
		ok = 1;
	}
	return ok;
}

struct BarcodeTemplateBlock {
	BarcodeTemplateBlock() : Len(0), Low(0), Upp(0), AddCheckDig(0)
	{
		PTR32(Prefix)[0] = 0;
		PTR32(Suffix)[0] = 0;
	}
	SString & FormatCode(int64 n, SString & rBuf) const
	{
		return rBuf.Z().Cat(Prefix).CatLongZ(n, Len).Cat(Suffix);
	}
	uint   Len;
	int64  Low;
	int64  Upp;
	int    AddCheckDig;
	char   Prefix[32];
	char   Suffix[32];
};

//static
int GoodsCore::Helper_SearchTemplatedBarcode(const BarcodeArray & rList, const void * pBlk, SString & rBarcode)
{
	const BarcodeTemplateBlock * p_btblk = static_cast<const BarcodeTemplateBlock *>(pBlk);
	int    ok = 0;
	rBarcode.Z();
	if(rList.getCount()) {
		SString buffer;
		SString bound_buf, val_str;
		const  size_t code_len = sstrlen(p_btblk->Prefix) + sstrlen(p_btblk->Suffix) + p_btblk->Len;
		int64  prev_val = (p_btblk->Low > 0) ? (p_btblk->Low-1) : 0;
		int64  using_val = 0;
		BarcodeTbl::Key0 k_low, k_upp;
		p_btblk->FormatCode(p_btblk->Low, bound_buf);
		if(p_btblk->AddCheckDig)
			bound_buf.CatChar('0');
		STRNSCPY(k_low.Code, bound_buf);
		p_btblk->FormatCode(p_btblk->Upp, bound_buf);
		if(p_btblk->AddCheckDig)
			bound_buf.CatChar('9');
		STRNSCPY(k_upp.Code, bound_buf);
		for(uint i = 0; !ok && i < rList.getCount(); i++) {
			const BarcodeTbl::Rec & r_item = rList.at(i);
			int r1 = stricmp(r_item.Code, k_low.Code);
			int r2 = stricmp(r_item.Code, k_upp.Code);
			if(r1 >= 0 && r2 <= 0 && sstrlen(r_item.Code) == (code_len+BIN(p_btblk->AddCheckDig))) {
				rBarcode = r_item.Code;
				ok = 1;
			}
		}
	}
	return ok;
}

//int GoodsCore::Helper_GetBarcodeByTempl(const char * pPrfx, const char * pSfx, uint len, int64 low, int64 upp, int addChkDig, SString & rBarcode)
int GoodsCore::Helper_GetBarcodeByTempl(const void * pBlk, SString & rBarcode)
{
	const BarcodeTemplateBlock * p_btblk = static_cast<const BarcodeTemplateBlock *>(pBlk);
	int    ok = 0;
	SString buffer;
	if(DS.CheckExtFlag(ECF_433OLDGENBARCODEMETHOD)) {
		for(int64 n = p_btblk->Low; !ok && n <= p_btblk->Upp; n++) {
			p_btblk->FormatCode(n, buffer);
			if(p_btblk->AddCheckDig)
				AddBarcodeCheckDigit(buffer);
			if(SearchBarcode(buffer, 0) <= 0)
				ok = 1;
		}
	}
	else {
		SString bound_buf, val_str;
		const  size_t code_len = sstrlen(p_btblk->Prefix) + sstrlen(p_btblk->Suffix) + p_btblk->Len;
		int64  prev_val = (p_btblk->Low > 0) ? (p_btblk->Low-1) : 0;
		int64  using_val = 0;
		BarcodeTbl::Key0 k0, k_low, k_upp;
		p_btblk->FormatCode(p_btblk->Low, bound_buf);
		if(p_btblk->AddCheckDig)
			bound_buf.CatChar('0');
		STRNSCPY(k_low.Code, bound_buf);
		p_btblk->FormatCode(p_btblk->Upp, bound_buf);
		if(p_btblk->AddCheckDig)
			bound_buf.CatChar('9');
		STRNSCPY(k_upp.Code, bound_buf);
		BExtQuery q(&BCTbl, 0, 128);
		q.select(BCTbl.Code, 0).where(BCTbl.Code >= k_low.Code && BCTbl.Code <= k_upp.Code);
		k0 = k_low;
		for(q.initIteration(false, &k0, spGe); !ok && q.nextIteration() > 0;) {
			char   temp_buf[32];
			STRNSCPY(temp_buf, BCTbl.data.Code);
			if(sstrlen(temp_buf) == (code_len+BIN(p_btblk->AddCheckDig))) {
				val_str = temp_buf + sstrlen(p_btblk->Prefix);
				val_str.Trim(p_btblk->Len);
				int64  cur_val = val_str.ToInt64();
			  	if(prev_val && cur_val > (prev_val+1)) {
					using_val = prev_val+1;
					ok = 1;
				}
				prev_val = cur_val;
			}
		}
		if(!ok && prev_val < p_btblk->Upp) {
			using_val = prev_val+1;
			ok = 1;
		}
		if(ok && using_val) {
			p_btblk->FormatCode(using_val, buffer);
			if(p_btblk->AddCheckDig)
				AddBarcodeCheckDigit(buffer);
			if(SearchBarcode(buffer, 0) > 0)
				ok = 0;
			else
				ok = 1;
		}
	}
	rBarcode = buffer;
	return ok;
}

int GoodsCore::ParseBarcodeTemplate(PPID grpID, const PPGoodsConfig & rCfg, const char * pTempl, void * pBlk)
{
	const char * p_meta_grp = "GRP";
	const char * p_meta_gr  = "@GR";
	const char * p_meta_wp  = "@WP";
	const char * p_meta_cp  = "@CP";
	int    ok = 0;
	BarcodeTemplateBlock * p_btblk = static_cast<BarcodeTemplateBlock *>(pBlk);
	char * c = p_btblk->Prefix;
	char * p_dup_templ = newStr(pTempl);
	SString temp_buf;
	strip(p_dup_templ);
	size_t templ_len = sstrlen(p_dup_templ);
	if(p_dup_templ[templ_len-1] == '^') {
		p_dup_templ[--templ_len] = 0;
		p_btblk->AddCheckDig = 1;
	}
	char * p = strip(p_dup_templ);
	if(*p) {
		memzero(p_btblk->Prefix, sizeof(p_btblk->Prefix));
		while(*p) {
			if(isdec(*p))
				*c++ = *p++;
			else if(strnicmp(p, p_meta_grp, 3) == 0 || strnicmp(p, p_meta_gr, 3) == 0) {
				if(GetSingleBarcode(grpID, 0, temp_buf) > 0) {
					temp_buf.ShiftLeftChr('@').Strip();
					c += sstrlen(strcpy(c, temp_buf));
				}
				p += 3;
			}
			else if(strnicmp(p, p_meta_wp, 3) == 0) {
				if(rCfg.WghtPrefix[0])
					c += sstrlen(strip(strcpy(c, rCfg.WghtPrefix)));
			   	p += 3;
			}
			else if(strnicmp(p, p_meta_cp, 3) == 0) {
				if(rCfg.WghtCntPrefix[0])
					c += sstrlen(strip(strcpy(c, rCfg.WghtCntPrefix)));
			   	p += 3;
			}
			else if(isasciiupr(*p))
				*c++ = *p++;
			else if(*p == '%') {
				SString sfx;
				double _rlo = 0.0;
				double _rup = 0.0;
				char   t[32];
				char * x;
				for(++p, x = t; isdec(*p);)
					*x++ = *p++;
				*x = 0;
				p_btblk->Len = satoi(t);
				if(*p == '[') {
					for(++p, x = t; *p && *p != ']';)
						*x++ = *p++;
					if(*p == ']')
						p++;
					*x = 0;
					strtorrng(t, &_rlo, &_rup);
				}
				for(; *p != 0; p++)
					if(*p != '\\')
						sfx.CatChar(*p);
				if(_rlo <= 0)
					_rlo = 1.0;
				if(_rup <= 0 || _rup > (fpow10i(p_btblk->Len) - 1))
					_rup = fpow10i(p_btblk->Len) - 1;
				p_btblk->Low = static_cast<int64>(_rlo);
				p_btblk->Upp = static_cast<int64>(_rup);
				STRNSCPY(p_btblk->Suffix, sfx);
				ok = 1;
				break;
			}
			else
				break;
		}
	}
	delete p_dup_templ;
	return ok;
}

int GoodsCore::IsTemplatedBarcode(PPID grpID, const PPGoodsConfig & rCfg, const char * pTempl, const char * pCode)
{
	int    ok = 0;
	BarcodeTemplateBlock btblk;
	if(!isempty(pCode) && ParseBarcodeTemplate(grpID, rCfg, pTempl, &btblk)) {
		SString buffer;
		SString bound_buf, val_str;
		const  size_t code_len = sstrlen(btblk.Prefix) + sstrlen(btblk.Suffix) + btblk.Len;
		int64  prev_val = (btblk.Low > 0) ? (btblk.Low-1) : 0;
		int64  using_val = 0;
		BarcodeTbl::Key0 k_low, k_upp;
		btblk.FormatCode(btblk.Low, bound_buf);
		if(btblk.AddCheckDig)
			bound_buf.CatChar('0');
		STRNSCPY(k_low.Code, bound_buf);
		btblk.FormatCode(btblk.Upp, bound_buf);
		if(btblk.AddCheckDig)
			bound_buf.CatChar('9');
		STRNSCPY(k_upp.Code, bound_buf);
		{
			int r1 = stricmp(pCode, k_low.Code);
			int r2 = stricmp(pCode, k_upp.Code);
			if(r1 >= 0 && r2 <= 0 && sstrlen(pCode) == (code_len+BIN(btblk.AddCheckDig))) {
				ok = 1;
			}
		}
	}
	return ok;
}

int GoodsCore::GetBarcodeByTemplate(PPID grpID, const PPGoodsConfig & rCfg, const char * pTempl, const BarcodeArray * pCurrentList, SString & rBuf)
{
	rBuf.Z();
	int    ok = 0;
	BarcodeTemplateBlock btblk;
	if(ParseBarcodeTemplate(grpID, rCfg, pTempl, &btblk)) {
		SString temp_buf;
		if(SVectorBase::GetCount(pCurrentList) && Helper_SearchTemplatedBarcode(*pCurrentList, &btblk, temp_buf) > 0) {
			rBuf = temp_buf;
			ok = -1;
		}
		else if(Helper_GetBarcodeByTempl(&btblk, temp_buf)) {
			rBuf = temp_buf;
			ok = 1;
		}
	}
	else
		rBuf.CatCharN('0', sstrlen(btblk.Prefix) + btblk.Len);
	return ok;
}

int GoodsCore::RemoveDupBarcode(PPID goodsID, const char * pCode, int use_ta)
{
	int    ok = -1;
	if(!isempty(pCode)) {
		BarcodeTbl::Rec bc_rec;
		BarcodeTbl::Key0 k;
		MEMSZERO(k);
		strip(STRNSCPY(k.Code, pCode));
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			if(SearchByKey(&BCTbl, 0, &k, &bc_rec) > 0 && bc_rec.GoodsID != goodsID) {
				THROW_DB(BCTbl.rereadForUpdate(0, &k));
				THROW_DB(BCTbl.deleteRec()); // @sfu
				ok = 1;
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::RemoveBarcodeLeadingZeros(int use_ta)
{
	int    ok = -1;
	long   item_id = 0;
	BarcodeTbl::Key0 k0;
	StrAssocArray code_list_to_update;
	BExtQuery q(&BCTbl, 0);
	q.select(BCTbl.Code, 0L);
	MEMSZERO(k0);
	for(q.initIteration(false, &k0, spFirst); q.nextIteration() > 0;) {
		char   code[32], old_code[32];
		STRNSCPY(code, BCTbl.data.Code);
		STRNSCPY(old_code, BCTbl.data.Code);
		char * p = strip(code);
		if(*p == '0') {
			do {
				p++;
			} while(*p == '0');
			STRNSCPY(code, p);
		}
		if(stricmp(old_code, code) != 0) {
			THROW_SL(code_list_to_update.Add(++item_id, old_code));
		}
	}
	if(code_list_to_update.getCount()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		for(uint i = 0; i < code_list_to_update.getCount(); i++) {
			STRNSCPY(k0.Code, code_list_to_update.Get(i).Txt);
			if(BCTbl.searchForUpdate(0, &k0, spEq)) {
				char * p = strip(BCTbl.data.Code);
				if(*p == '0') {
					do {
						p++;
					} while(*p == '0');
					STRNSCPY(BCTbl.data.Code, p);
				}
				THROW_DB(BCTbl.updateRec()); // @sfu
				ok = 1;
			}
			THROW_DB(BTROKORNFOUND);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::MoveGrpToGrp(PPID srcGrp, PPID destGrp, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Key1 k;
	THROW_PP(Search(destGrp) > 0 && data.Kind == PPGDSK_GROUP && !(data.Flags & (GF_ALTGROUP|GF_FOLDER)), PPERR_DESTGGRPNEXISTS);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k);
		k.Kind = PPGDSK_GOODS;
		k.ParentID = srcGrp;
		if(search(1, &k, spGe) && k.ParentID == srcGrp) {
			do {
				Goods2Tbl::Rec rec;
				copyBufTo(&rec);
				rec.ParentID = destGrp;
				THROW(Update(&rec.ID, &rec, 0));
			} while(search(&k, spGt) && k.ParentID == srcGrp);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::IsAltGroup(PPID grpID)
{
	Goods2Tbl::Rec rec;
	return (Fetch(grpID, &rec) > 0) ? BIN(rec.Flags & GF_ALTGROUP) : -1;
}

int GoodsCore::IsTempAltGroup(PPID grpID)
{
	Goods2Tbl::Rec rec;
	return (Fetch(grpID, &rec) > 0) ? BIN((rec.Flags & GF_TEMPALTGROUP) == GF_TEMPALTGROUP) : -1;
}

int GoodsCore::IsDynamicAltGroup(PPID grpID)
{
	Goods2Tbl::Rec rec;
	return (Fetch(grpID, &rec) > 0) ? BIN((rec.Flags & GF_DYNAMICALTGRP) == GF_DYNAMICALTGRP) : -1;
}

int GoodsCore::GetGoodsCodeInAltGrp(PPID goodsID, PPID grpID, long * pInnerNum)
{
	int    ok = -1;
	long   num = 0;
	if(goodsID && grpID) {
		Goods2Tbl::Rec grp_rec;
		if(Fetch(grpID, &grp_rec) > 0 && (grp_rec.Flags & GF_ALTGROUP) && !(grp_rec.Flags & GF_DYNAMIC)) {
			ObjAssocTbl::Rec oa_rec;
			if(P_Ref->Assc.Search(PPASS_ALTGOODSGRP, grpID, goodsID, &oa_rec) > 0) {
				num = oa_rec.InnerNum;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pInnerNum, num);
	return ok;
}

int GoodsCore::GetAltGroupsForGoods(PPID goodsID, PPIDArray * pGrpIDList)
{
	ObjAssocTbl::Rec assc_rec;
	for(SEnum en = P_Ref->Assc.Enum(PPASS_ALTGOODSGRP, goodsID, 1); en.Next(&assc_rec) > 0;) {
		if(!pGrpIDList->add(assc_rec.PrmrObjID))
			return PPSetErrorSLib();
	}
	return 1;
}

int GoodsCore::IsExclusiveAltGrp(PPID altGrpID, PPID * pParentID)
{
	int    ok = 0;
	PPID   parent_id = 0;
	Goods2Tbl::Rec grp_rec;
	if(Fetch(altGrpID, &grp_rec) > 0) {
		if(!(grp_rec.Flags & GF_ALTGROUP))
			ok = PPSetError(PPERR_NOTALTGRP);
		else {
			parent_id = grp_rec.ParentID;
			if(parent_id && Fetch(parent_id, &grp_rec) > 0 && (grp_rec.Flags & GF_EXCLALTFOLD)) {
				ASSIGN_PTR(pParentID, parent_id);
				ok = 1;
			}
		}
	}
	return ok;
}

int GoodsCore::GetExclusiveAltParent(PPID goodsID, PPID parentID, PPID * pAltGrpID)
{
	int    ok = -1;
	PPID   alt_grp_id = 0;
	PPIDArray alt_grp_list;
	PPIDArray term_grp_list;
	GetAltGroupsForGoods(goodsID, &alt_grp_list);
	GetGroupTerminalList(parentID, &term_grp_list, 0);
	alt_grp_list.intersect(&term_grp_list);
	for(uint i = 0; i < term_grp_list.getCount(); i++) {
		PPID grp_id = term_grp_list.get(i);
		if(!alt_grp_list.lsearch(grp_id) && IsDynamicAltGroup(grp_id) > 0) {
			if(BelongToGroup(goodsID, grp_id, 0) > 0)
				alt_grp_list.add(grp_id);
		}
	}
	if(alt_grp_list.getCount()) {
		alt_grp_id = alt_grp_list.get(0);
		ok = (alt_grp_list.getCount() == 1) ? 1 : 2;
	}
	ASSIGN_PTR(pAltGrpID, alt_grp_id);
	return ok;
}

int GoodsCore::CheckGoodsForExclusiveAltGrp(PPID goodsID, PPID altGrpID)
{
	int    ok = 1;
	PPID   parent_id = 0;
	if(IsExclusiveAltGrp(altGrpID, &parent_id) > 0) {
		PPIDArray alt_grp_list;
		PPIDArray term_grp_list;
		GetAltGroupsForGoods(goodsID, &alt_grp_list);
		alt_grp_list.freeByKey(altGrpID, 0);
		GetGroupTerminalList(parent_id, &term_grp_list, 0);
		alt_grp_list.intersect(&term_grp_list);
		if(alt_grp_list.getCount()) {
			ok = PPSetObjError(PPERR_DUPEXCLALTGRPMEMBER, PPOBJ_GOODS, alt_grp_list.at(0));
		}
	}
	return ok;
}

int GoodsCore::AssignGoodsToAltGrp(PPID goodsID, PPID altGrpID, long innerNum, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	Goods2Tbl::Rec goods_rec;
	if(!CheckGoodsForExclusiveAltGrp(goodsID, altGrpID)) {
		ok = -1;
	}
	else {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Fetch(goodsID, &goods_rec) > 0);
		if(P_Ref->Assc.Search(PPASS_ALTGOODSGRP, altGrpID, goodsID) < 0) {
			ObjAssocTbl::Rec rec;
			rec.AsscType  = PPASS_ALTGOODSGRP;
			rec.PrmrObjID = altGrpID;
			rec.ScndObjID = goodsID;
			if(innerNum) {
				int    r = P_Ref->Assc.SearchNum(PPASS_ALTGOODSGRP, altGrpID, innerNum, 0);
				THROW(r != 0);
				THROW_PP_S(r < 0, PPERR_ALTGOODSGRPNUMBUSY, innerNum);
				rec.InnerNum = innerNum;
			}
			else {
				THROW(P_Ref->Assc.SearchFreeNum(PPASS_ALTGOODSGRP, altGrpID, &rec.InnerNum));
			}
			THROW(P_Ref->Assc.Add(&id, &rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SetAltGrpList(PPID grpID, const PPIDArray & rList, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec grp_rec;
	THROW(Search(grpID, &grp_rec) > 0);
	THROW_PP_S(grp_rec.Kind == PPGDSK_GROUP && grp_rec.Flags & GF_ALTGROUP && !(grp_rec.Flags & GF_DYNAMIC), PPERR_OBJNOTSIMPLEALTGGRP, grp_rec.Name);
	{
		PPIDArray new_member_list, rmv_member_list;
		PPTransaction tra(use_ta);
		THROW(tra);
		P_Ref->Assc.GetListByPrmr(PPASS_ALTGOODSGRP, grpID, &rmv_member_list);
		rmv_member_list.sortAndUndup();
		uint   i = rmv_member_list.getCount();
		if(i) do {
			const  PPID item_id = rmv_member_list.get(--i);
			if(rList.lsearch(item_id))
				rmv_member_list.atFree(i);
		} while(i);
		for(i = 0; i < rList.getCount(); i++) {
			const  PPID item_id = rList.get(i);
			if(!rmv_member_list.bsearch(item_id)) {
				new_member_list.add(item_id);
			}
		}
		for(i = 0; i < rmv_member_list.getCount(); i++) {
			const  PPID item_id = rmv_member_list.get(i);
			THROW(P_Ref->Assc.Remove(PPASS_ALTGOODSGRP, grpID, item_id, 0));
		}
		for(i = 0; i < new_member_list.getCount(); i++) {
			const  PPID item_id = new_member_list.get(i);
			THROW(AssignGoodsToAltGrp(item_id, grpID, 0, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::IsCompatibleByUnit(PPID id1, PPID id2, double * pRatio)
{
	double _ratio = 0.0;
	Goods2Tbl::Rec rec1, rec2;
	if(Fetch(id1, &rec1) > 0 && Fetch(id2, &rec2) > 0) {
		if(rec1.UnitID == rec2.UnitID)
			_ratio = 1;
		else if(rec1.PhUnitID && rec1.PhUnitID == rec2.PhUnitID) {
			if(rec1.PhUPerU > 0 && rec2.PhUPerU > 0)
				_ratio = rec2.PhUPerU / rec1.PhUPerU;
		}
		else if(rec1.PhUnitID && rec1.PhUnitID == rec2.UnitID) {
			if(rec1.PhUPerU > 0.0)
				_ratio = 1.0 / rec1.PhUPerU;
		}
		else if(rec2.PhUnitID && rec1.UnitID == rec2.PhUnitID) {
			if(rec2.PhUPerU > 0.0)
				_ratio = rec2.PhUPerU;
		}
	}
	ASSIGN_PTR(pRatio, R6(_ratio));
	return (_ratio == 0.0) ? 0 : ((!(rec1.Flags & GF_INTVAL) || ffrac(_ratio) == 0.0) ? 1 : -1);
}

bool GoodsCore::IsChildOf(PPID id, PPID parent)
{
	Goods2Tbl::Rec rec;
	if(Fetch(id, &rec) > 0) {
		PPID p = rec.ParentID;
		if(p == parent)
			return true;
		else if(p == 0)
			return false;
		else
			return IsChildOf(p, parent); // @recursion
	}
	return false;
}

int GoodsCore::GetGroupFilt(PPID grpID, GoodsFilt * pFilt)
{
	int    ok = (grpID && pFilt) ? GetAltGroupFilt(grpID, pFilt) : -1;
	if(ok > 0) {
		if(pFilt && pFilt->P_SjF)
			pFilt->P_SjF->Period.Actualize(ZERODATE);
		pFilt->Setup();
	}
	return ok;
}

int GoodsCore::Helper_BelongToGroup(PPID id, PPID grp, PPID * pSubGrpID, PPIDArray * pDynGrpList)
{
	int    r = 0;
    PPID   sub_grp_id = 0;
	Goods2Tbl::Rec grp_rec;
	if(grp == 0)
		r = 1;
	else if(Fetch(grp, &grp_rec) > 0) {
		PPIDArray local_dyn_grp_list;
		SETIFZ(pDynGrpList, &local_dyn_grp_list);
		id = labs(id);
		if(grp_rec.Flags & GF_ALTGROUP) {
			if(grp_rec.Flags & GF_DYNAMIC) {
				GoodsFilt grp_filt;
				if(GetGroupFilt(grp, &grp_filt) > 0) {
					if(pDynGrpList->addUnique(grp) > 0) {
						PPObjGoods goods_obj;
						r = goods_obj.CheckForFilt(&grp_filt, id);
					}
					else
						r = 0;
				}
			}
			else
				r = BIN(P_Ref->Assc.Search(PPASS_ALTGOODSGRP, grp, id) > 0);
		}
		else if(grp_rec.Flags & GF_GENERIC)
			r = BIN(BelongToGen(id, &grp, 0) > 0);
		else if(grp_rec.Flags & GF_FOLDER && grp_rec.Flags & GF_EXCLALTFOLD) {
			PPIDArray term_list;
			GetGroupTerminalList(grp, &term_list, 0);
			for(uint i = 0; r == 0 && i < term_list.getCount(); i++) {
				const  PPID term_id = term_list.get(i);
				if(id == term_id || Helper_BelongToGroup(id, term_id, 0, pDynGrpList)) { // @recursion
			   		sub_grp_id = term_id;
			   		r = 1;
				}
			}
		}
		else if(IsChildOf(id, grp))
			r = 1;
	}
	ASSIGN_PTR(pSubGrpID, sub_grp_id);
	return r;
}

int GoodsCore::BelongToGroup(PPID id, PPID grp, PPID * pSubGrpID) { return Helper_BelongToGroup(id, grp, pSubGrpID, 0); }

int GoodsCore::BelongToGen(PPID goodsID, PPID * pGenID, ObjAssocTbl::Rec * b)
{
	int    ok = -1;
	if(pGenID) {
		if(*pGenID) {
			if(P_Ref->Assc.Search(PPASS_GENGOODS, *pGenID, goodsID, b) > 0)
				ok = 1;
		}
		else {
			ObjAssocTbl::Rec assc_rec;
			SEnum en = P_Ref->Assc.Enum(PPASS_GENGOODS, goodsID, 1);
			if(en.Next(&assc_rec) > 0) {
				ASSIGN_PTR(pGenID, assc_rec.PrmrObjID);
				ASSIGN_PTR(b, assc_rec);
				ok = 1;
			}
		}
		if(ok < 0)
			ok = BelongToDynGen(goodsID, pGenID, 0);
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}

int GoodsCore::GetGenericList(PPID genID, PPIDArray * pList)
{
	int    ok = -1;
	Goods2Tbl::Rec gen_rec;
	if(Fetch(genID, &gen_rec) > 0) {
		PPIDArray temp_list;
		THROW_PP(gen_rec.Flags & GF_GENERIC, PPERR_NOTGENGOODS);
		THROW(P_Ref->Assc.GetListByPrmr(PPASS_GENGOODS, genID, &temp_list));
		if(gen_rec.GdsClsID)
			THROW(GetDynGenericList(genID, &temp_list));
		THROW(pList->addUnique(&temp_list));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int GoodsCore::AssignGoodsToGen(PPID goodsID, PPID genID, int abbr, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	Goods2Tbl::Rec rec, gen_rec;
	const  char * p = abbr ? rec.Abbr : rec.Name;
	ObjAssocTbl::Rec assc_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(goodsID, &rec) > 0);
		THROW(Fetch(genID, &gen_rec) > 0);
		THROW_PP(gen_rec.Flags & GF_GENERIC, PPERR_NOTGENGOODS);
		if(BelongToGen(goodsID, &(id = 0), &assc_rec) > 0) {
			Goods2Tbl::Rec gen2_rec;
			if(Fetch(id, &gen2_rec) > 0)
				PPSetAddedMsgString(gen2_rec.Name);
			THROW_PP(id == genID, PPERR_TOOGENGOODS);
		}
		else {
			assc_rec.Clear();
			assc_rec.AsscType  = PPASS_GENGOODS;
			assc_rec.PrmrObjID = genID;
			assc_rec.ScndObjID = goodsID;
			THROW(P_Ref->Assc.SearchFreeNum(PPASS_GENGOODS, genID, &assc_rec.InnerNum));
			THROW(P_Ref->Assc.Add(&(id = 0), &assc_rec, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::RemoveGoodsFromGen(PPID goodsID, PPID genID, int use_ta)
{
	return P_Ref->Assc.Remove(PPASS_GENGOODS, genID, goodsID, use_ta);
}

int GoodsCore::SetGenericList(PPID goodsID, const PPIDArray & rList, int use_ta)
{
	int    ok = 1;
	Goods2Tbl::Rec gen_rec, goods_rec;
	TSVector <ObjAssocTbl::Rec> assc_list;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(Search(goodsID, &gen_rec) > 0);
		THROW_PP(gen_rec.Flags & GF_GENERIC, PPERR_NOTGENGOODS);
		{
			PPIDArray temp_list;
			temp_list.addUnique(&rList);
			temp_list.sort();
			for(uint i = 0; i < temp_list.getCount(); i++) {
				PPID   _id = temp_list.get(i);
				if(_id != goodsID && Fetch(_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
					ObjAssocTbl::Rec assc_rec;
					assc_rec.AsscType  = PPASS_GENGOODS;
					assc_rec.PrmrObjID = goodsID;
					assc_rec.ScndObjID = _id;
					THROW_SL(assc_list.insert(&assc_rec));
				}
			}
			THROW(P_Ref->Assc.AddArray(PPASS_GENGOODS, goodsID, &assc_list, 1, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
// GoodsCache
//
class GoodsCache : public ObjCacheHash {
public:
	struct Data : public ObjCacheEntry { // size=48+16
		PPID   ParentID;
		PPID   UnitID;
		PPID   PhUnitID;
		double PhUPerU;
		PPID   ClsID;
		PPID   BrandID;
		PPID   ManufID;
		PPID   StrucID;
		long   Flags;
		int16  Kind;
		int16  TypeID;
		int16  TaxGrpID;
		int16  Pad;        // @alignment
	};

	GoodsCache();
	~GoodsCache();
	virtual void FASTCALL Dirty(PPID id); // @sync_w
	int    GetGtl(PPID grpID, PPIDArray * pList, PPIDArray * pUntermList); // @sync_r
	int    GetAltGrpFilt(PPID grpID, GoodsFilt * pFilt); // @sync_r
	int    PutAltGrpFilt(PPID grpID, const GoodsFilt * pFilt); // @sync_w
	int    GetConfig(PPGoodsConfig * pCfg, int enforce); // @sync_w
	int    GetStockExt(PPID goodsID, GoodsStockExt * pExt); // @sync_w
	int    GetSingleBarcode(PPID goodsID, SString & rBuf); // @sync_w
	const  TwoDimBarcodeFormatArray * GetBc2dSpec();
	const  StrAssocArray * GetFullList(); // @sync_w
	void   ReleaseFullList(const StrAssocArray * pList);
	//
	// Descr: Сбрасывает содержимое кэша наименований товаров
	//
	void   ResetFullList(); // @sync_w
	int    SearchGoodsAnalogs(PPID goodsID, PPIDArray & rList, SString * pTransitComponentBuf); // @sync_w
private:
	virtual int  FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;

	struct GroupTermList {
		PPID   GrpID;
		PPIDArray List;       // Список отсортирован. За это отвечает функция GoodsCache::GetGtl()
		PPIDArray UntermList; // Список отсортирован. За это отвечает функция GoodsCache::GetGtl()
	};
	struct AltGrpFiltItem {
		PPID   GrpID;
		GoodsFilt Filt;
	};
	struct StockExt {
		PPID   GoodsID;
		long   Brutto;
		PPDimention PckgDim;
		double Package;
		double MinShippmQtty;
		int16  ExpiryPeriod;
		int16  GseFlags;
		RAssocArray MinStockList;
		TSVector <GoodsStockExt::Pallet> PltList;
	};
	class GslArray : public TSArray <GoodsCache::StockExt> {
	public:
		GslArray() : TSArray <GoodsCache::StockExt>(aryDataOwner|aryEachItem)
		{
		}
	private:
		virtual void FASTCALL freeItem(void * pItem)
		{
			static_cast<StockExt *>(pItem)->MinStockList.freeAll();
			static_cast<StockExt *>(pItem)->PltList.freeAll();
		}
	};
	TSCollection <GroupTermList> Gtl;
	TSCollection <AltGrpFiltItem> Agfl;
	GslArray      Gsl;    // Массив складских характеристик товаров
	UintHashTable ExcGsl; // Список товаров, которые не имеют складского расширения //
	//
	class FglArray : public StrAssocArray {
	public:
		FglArray(bool use) : StrAssocArray(), Use(use), Inited(false)
		{
		}
		void   FASTCALL Dirty(PPID goodsID)
		{
			DirtyTable.Add((uint32)labs(goodsID));
		}
		void   Reset()
		{
			DirtyTable.Clear();
			StrAssocArray::Z();
			Inited = false;
		}

		const  bool Use;
		bool   Inited;
		UintHashTable DirtyTable;
	};
	FglArray FullGoodsList;
	//
	class SingleBarcodeArray : public StrAssocArray {
	public:
		SingleBarcodeArray() : StrAssocArray()
		{
		}
		void   FASTCALL Dirty(PPID goodsID)
		{
			SRWLOCKER(Lck, SReadWriteLocker::Write);
			DirtyTable.Add((uint32)labs(goodsID));
		}
		UintHashTable ExcTable;   // Список товаров, не имеющих штрихкодов
		UintHashTable DirtyTable;
		ReadWriteLock Lck;
	};
	SingleBarcodeArray SbcList;
	//
	TwoDimBarcodeFormatArray * P_Bc2dSpec;
	PPObjectTokenizer * P_ObjTkn;
	PPGoodsConfig Cfg;
	ReadWriteLock GtlLock;
	ReadWriteLock AgflLock;
	ReadWriteLock CfgLock;
	ReadWriteLock GslLock;
	ReadWriteLock FglLock;
	ReadWriteLock TknLock;

	static  void FASTCALL AssignGoodsStockExtCacheRec(const GoodsCache::StockExt & rSrc, GoodsStockExt * pExt);
};

GoodsCache::GoodsCache() : ObjCacheHash(PPOBJ_GOODS, sizeof(Data),
	// @v11.0.4 (DS.CheckExtFlag(ECF_SYSSERVICE) ? (8*1024*1024) : (2*1024U*1024U)),
	// @v11.0.4 (DS.CheckExtFlag(ECF_SYSSERVICE) ? 16 : 12)),
	SMEGABYTE(8), 16), // @v11.0.4
	FullGoodsList(DS.CheckExtFlag(ECF_FULLGOODSCACHE)), P_Bc2dSpec(0), P_ObjTkn(0)
{
}

GoodsCache::~GoodsCache()
{
	ZDELETE(P_Bc2dSpec);
	ZDELETE(P_ObjTkn);
}

int GoodsCache::SearchGoodsAnalogs(PPID goodsID, PPIDArray & rList, SString * pTransitComponentBuf) // @sync_w
{
	int    ok = -1;
	if(goodsID) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		if(goods_obj.Fetch(goodsID, &goods_rec) > 0) {
			SRWLOCKER(TknLock, SReadWriteLocker::Write);
			if(!P_ObjTkn) {
				P_ObjTkn = new PPObjectTokenizer;
				if(P_ObjTkn) {
					PROFILE(P_ObjTkn->ProcessSuprWare(/*SUPRWARETYPE_GOODS*/0, SUPRWARECLASS_MEDICAL));
					PROFILE(P_ObjTkn->ProcessGoods(0));
					PROFILE(P_ObjTkn->IndexResources(0));
				}
			}
			ok = P_ObjTkn ? P_ObjTkn->SearchGoodsAnalogs(goodsID, rList, pTransitComponentBuf) : 0;
		}
	}
	return ok;
}

const TwoDimBarcodeFormatArray * GoodsCache::GetBc2dSpec()
{
	if(!P_Bc2dSpec) {
		ENTER_CRITICAL_SECTION
		SETIFZ(P_Bc2dSpec, new TwoDimBarcodeFormatArray);
		LEAVE_CRITICAL_SECTION
	}
	return P_Bc2dSpec;
}

int GoodsCache::GetConfig(PPGoodsConfig * pCfg, int enforce)
{
	{
		SRWLOCKER(CfgLock, SReadWriteLocker::Read);
		if(!(Cfg.Flags & GCF_VALID) || enforce) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(!(Cfg.Flags & GCF_VALID) || enforce) {
				PPObjGoods::ReadConfig(&Cfg);
				Cfg.Flags |= GCF_VALID;
			}
		}
		ASSIGN_PTR(pCfg, Cfg);
	}
	return 1;
}

//static
void FASTCALL GoodsCache::AssignGoodsStockExtCacheRec(const GoodsCache::StockExt & rSrc, GoodsStockExt * pExt)
{
	pExt->Brutto = rSrc.Brutto;
	pExt->PckgDim = rSrc.PckgDim;
	pExt->Package  = rSrc.Package;
	pExt->MinShippmQtty = rSrc.MinShippmQtty;
	pExt->ExpiryPeriod = rSrc.ExpiryPeriod;
	pExt->GseFlags     = rSrc.GseFlags;
	pExt->MinStockList = rSrc.MinStockList;
	pExt->PltList = rSrc.PltList;
}

int GoodsCache::GetStockExt(PPID goodsID, GoodsStockExt * pExt)
{
	int    ok = 0;
	pExt->Z();
	{
		SRWLOCKER(GslLock, SReadWriteLocker::Read);
		uint   pos = 0;
		goodsID = labs(goodsID);
		if(goodsID == 0 || ExcGsl.Has(goodsID))
			ok = -1;
		else if(Gsl.lsearch(&goodsID, &pos, CMPF_LONG)) {
			AssignGoodsStockExtCacheRec(Gsl.at(pos), pExt);
			ok = 1;
		}
		if(!ok) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			//
			// Пока ждали блокировку нашу работу мог сделать другой поток
			//
			if(goodsID == 0 || ExcGsl.Has(goodsID))
				ok = -1;
			else if(Gsl.lsearch(&goodsID, &pos, CMPF_LONG)) {
				AssignGoodsStockExtCacheRec(Gsl.at(pos), pExt);
				ok = 1;
			}
			if(!ok) {
				PPObjGoods goods_obj(SConstructorLite); // @v10.0.0 SConstructorLite
				GoodsStockExt temp;
				int    r = goods_obj.P_Tbl->GetStockExt(goodsID, &temp, 0 /* not using cache! */);
				if(r > 0) {
					StockExt ext;
					ext.GoodsID = goodsID;
					ext.Brutto = temp.Brutto;
					ext.PckgDim = temp.PckgDim;
					ext.Package  = temp.Package;
					ext.MinShippmQtty = temp.MinShippmQtty;
					ext.ExpiryPeriod = temp.ExpiryPeriod;
					ext.GseFlags = temp.GseFlags;
					ext.MinStockList = temp.MinStockList;
					ext.PltList = temp.PltList;
					long   zero = 0;
					if(Gsl.lsearch(&zero, &(pos = 0), CMPF_LONG))
						Gsl.at(pos) = ext;
					else {
						//
						// Здесь позаботимся о том, чтобы не раздвоить указатель в GoodsStockExt::MinStockList
						//
						StockExt dummy;
						Gsl.insert(&dummy);
						Gsl.at(Gsl.getCount()-1) = ext;
					}
					*pExt = temp;
					ok = 1;
				}
				else {
					ExcGsl.Add(goodsID);
					ok = -1;
				}
			}
		}
	}
	return ok;
}

int GoodsCache::GetSingleBarcode(PPID goodsID, SString & rBuf)
{
	int    ok = 0;
	rBuf.Z();
	if(goodsID == 0)
		ok = -1;
	else {
		SRWLOCKER(SbcList.Lck, SReadWriteLocker::Read);
		goodsID = labs(goodsID);
		if(!SbcList.DirtyTable.Has(goodsID)) {
			if(SbcList.ExcTable.Has(goodsID))
				ok = -1;
			else if(SbcList.GetText(goodsID, rBuf))
				ok = 1;
		}
		if(!ok) {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			//
			// Еще раз попытаемся получить то, что хотим (пока ждали блокировки другой поток мог сделать нашу работу)
			//
			if(!SbcList.DirtyTable.Has(goodsID)) {
				if(SbcList.ExcTable.Has(goodsID))
					ok = -1;
				else if(SbcList.GetText(goodsID, rBuf))
					ok = 1;
			}
			if(!ok) {
				PPObjGoods goods_obj(SConstructorLite); // @v10.0.0 SConstructorLite
				SString temp_buf; // Рискованно пользоваться буфером rBuf: маловероятно, но он может быть
					// одновременно использован другими потоками.
				int    r = goods_obj.GetSingleBarcode(goodsID, 0, temp_buf);
				const  ulong ugoodsid = static_cast<ulong>(goodsID);
				if(r > 0) {
					SbcList.Add(goodsID, temp_buf, 1);
					SbcList.ExcTable.Remove(ugoodsid);
					ok = 1;
				}
				else {
					SbcList.Remove(goodsID);
					SbcList.ExcTable.Add(ugoodsid);
					ok = -1;
				}
				SbcList.DirtyTable.Remove(ugoodsid);
				rBuf = temp_buf;
			}
		}
	}
	return ok;
}

void GoodsCache::ResetFullList()
{
	{
		SRWLOCKER(FglLock, SReadWriteLocker::Write);
		FullGoodsList.Inited = false;
		FullGoodsList.DirtyTable.Clear();
	}
}

const StrAssocArray * GoodsCache::GetFullList()
{
	int    err = 0;
	const  StrAssocArray * p_result = 0;
	if(FullGoodsList.Use) {
		if(!FullGoodsList.Inited || FullGoodsList.DirtyTable.GetCount()) {
			SString temp_buf;
			SRWLOCKER(FglLock, SReadWriteLocker::Write);
			if(!FullGoodsList.Inited || FullGoodsList.DirtyTable.GetCount()) {
				PPObjGoods goods_obj(SConstructorLite);
				if(!FullGoodsList.Inited) {
					PPUserFuncProfiler ufp(PPUPRF_BUILDGOODSFL); // @v10.1.4
					SString fmt_buf;
					uint   _mc = 0;
					if(!DS.IsThreadInteractive())
						PPLoadText(PPTXT_GETTINGFULLTEXTLIST, fmt_buf);
					PROFILE_START
					Goods2Tbl * p_tbl = goods_obj.P_Tbl;
					BExtQuery q(p_tbl, 0, 24);
					q.select(p_tbl->ID, p_tbl->ParentID, p_tbl->Name, 0L).where(p_tbl->Kind == PPGDSK_GOODS);
					FullGoodsList.Z();
					Goods2Tbl::Key0 k0;
					for(q.initIteration(false, &k0, spFirst); !err && q.nextIteration() > 0;) {
						_mc++;
						(temp_buf = p_tbl->data.Name).ToLower();
						if(!FullGoodsList.AddFast(p_tbl->data.ID, temp_buf)) {
							PPSetErrorSLib();
							err = 1;
						}
						else if(!DS.IsThreadInteractive()) {
							if((_mc % 1000) == 0)
								PPWaitMsg((temp_buf = fmt_buf).Space().Cat(_mc));
						}
					}
					PROFILE_END
					FullGoodsList.SortByText(); // @v10.5.11
					ufp.SetFactor(0, _mc); // @v10.1.4
					ufp.Commit(); // @v10.1.4
				}
				else {
					PROFILE_START
					Goods2Tbl::Rec goods_rec;
					for(ulong id = 0; !err && FullGoodsList.DirtyTable.Enum(&id);) {
						if(Get((long)id, &goods_rec) > 0) { // Извлекаем наименование из кэша (из самого себя): так быстрее.
							if(goods_rec.Kind == PPGDSK_GOODS) {
								(temp_buf = goods_rec.Name).ToLower();
								if(!FullGoodsList.Add(id, temp_buf, 1)) {
									PPSetErrorSLib();
									err = 1;
								}
							}
						}
					}
					PROFILE_END
				}
				if(!err) {
					FullGoodsList.DirtyTable.Clear();
					FullGoodsList.Inited = true;
				}
			}
		}
		if(!err) {
			#if SLTRACELOCKSTACK
			SLS.LockPush(SLockStack::ltRW_R, __FILE__, __LINE__);
			#endif
			FglLock.ReadLock_();
			p_result = &FullGoodsList;
		}
	}
	return p_result;
}

void GoodsCache::ReleaseFullList(const StrAssocArray * pList)
{
	if(pList && pList == &FullGoodsList) {
		FglLock.Unlock_();
		#if SLTRACELOCKSTACK
		SLS.LockPop();
		#endif
	}
}

void FASTCALL GoodsCache::Dirty(PPID id)
{
	uint   pos = 0;
	ObjCacheHash::Dirty(id);
	{
		//
		// Очистка элемента кэша складского расширения товара
		//
		SRWLOCKER(GslLock, SReadWriteLocker::Write);
		PPID   abs_id = labs(id);
		uint   gsl_pos = 0;
		ExcGsl.Remove(abs_id);
		if(Gsl.lsearch(&abs_id, &gsl_pos, CMPF_LONG))
			Gsl.at(gsl_pos).GoodsID = 0;
	}
	SbcList.Dirty(id);
	{
		SRWLOCKER(GtlLock, SReadWriteLocker::Write);
		//
		// Если измененная группа находится хотя бы в одном терминальном списке, то придется //
		// полностью очистить кэш терминальных групп (мы не можем быстро в условиях блокировки, требующей
		// моментального исполнения, тратить время на определение того, какая группа из какой и в какую
		// была перенесена
		//
		uint   gtl_pos = 0;
		for(uint i = 0; i < Gtl.getCount(); i++) {
			GroupTermList * p_item = Gtl.at(i);
			if(p_item->List.bsearch(id) || p_item->UntermList.bsearch(id)) {
				Gtl.freeAll();
				gtl_pos = 0;
				break;
			}
			else if(p_item->GrpID == id)
				gtl_pos = i+1;
		}
		if(gtl_pos)
			Gtl.atFree(gtl_pos-1);
	}
	{
		SRWLOCKER(AgflLock, SReadWriteLocker::Write);
		if(Agfl.lsearch(&id, &pos, CMPF_LONG))
			Agfl.atFree(pos);
	}
	//
	//
	//
	{
		SRWLOCKER(FglLock, SReadWriteLocker::Write);
		FullGoodsList.Dirty(id);
	}
}

int GoodsCache::GetAltGrpFilt(PPID grpID, GoodsFilt * pFilt)
{
	int    ok = 1;
	uint   pos = 0;
	{
		SRWLOCKER(AgflLock, SReadWriteLocker::Read);
		if(Agfl.lsearch(&grpID, &pos, CMPF_LONG)) {
			ASSIGN_PTR(pFilt, Agfl.at(pos)->Filt);
		}
		else
			ok = 0;
	}
	return ok;
}

int GoodsCache::PutAltGrpFilt(PPID grpID, const GoodsFilt * pFilt)
{
	int    ok = 1;
	uint   pos = 0;
	{
		SRWLOCKER(AgflLock, SReadWriteLocker::Write);
		if(Agfl.lsearch(&grpID, &pos, CMPF_LONG)) {
			if(pFilt) {
				AltGrpFiltItem * p_item = Agfl.at(pos);
				p_item->Filt = *pFilt;
			}
			else
				Agfl.atFree(pos);
		}
		else if(pFilt) {
			AltGrpFiltItem * p_item = new AltGrpFiltItem;
			THROW_MEM(p_item);
			p_item->GrpID = grpID;
			p_item->Filt = *pFilt;
			THROW_SL(Agfl.insert(p_item));
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCache::GetGtl(PPID grpID, PPIDArray * pList, PPIDArray * pUntermList)
{
	int    ok = 1;
	uint   pos = 0;
	{
		SRWLOCKER(GtlLock, SReadWriteLocker::Read);
		if(Gtl.lsearch(&grpID, &pos, CMPF_LONG)) {
			ASSIGN_PTR(pList, Gtl.at(pos)->List);
			ASSIGN_PTR(pUntermList, Gtl.at(pos)->UntermList);
		}
		else {
			SRWLOCKER_TOGGLE(SReadWriteLocker::Write);
			if(Gtl.lsearch(&grpID, &(pos = 0), CMPF_LONG)) {
				//
				// Возможно, пока мы ждали блокировку, работу сделал какой-то иной поток
				//
				ASSIGN_PTR(pList, Gtl.at(pos)->List);
				ASSIGN_PTR(pUntermList, Gtl.at(pos)->UntermList);
			}
			else {
				PROFILE_START
				PPObjGoods goods_obj(SConstructorLite);
				PPIDArray local_list, local_unterm_list;
				int    r = goods_obj.P_Tbl->Helper_GetGroupTerminalList(grpID, local_list, local_unterm_list);
				local_list.sortAndUndup();
				local_unterm_list.sortAndUndup();
				{
					assert(Gtl.lsearch(&grpID, &(pos = 0), CMPF_LONG) == 0);
					GroupTermList * p_item = new GroupTermList;
					if(p_item) {
						p_item->GrpID = grpID;
						p_item->List = local_list;
						p_item->UntermList = local_unterm_list;
						Gtl.insert(p_item);
					}
				}
				ASSIGN_PTR(pList, local_list);
				ASSIGN_PTR(pUntermList, local_unterm_list);
				PROFILE_END
			}
		}
	}
	return ok;
}

int GoodsCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, void * /*extraData*/)
{
	int    ok = 1;
	Data * p_cache_rec = static_cast<Data *>(pEntry);
	PPObjGoods  goods_obj(SConstructorLite);
	Goods2Tbl::Rec rec;
	if(id && goods_obj.Search(id, &rec) > 0) {
		p_cache_rec->ParentID = rec.ParentID;
		p_cache_rec->UnitID   = rec.UnitID;
		p_cache_rec->PhUnitID = rec.PhUnitID;
		p_cache_rec->PhUPerU  = rec.PhUPerU;
		p_cache_rec->Kind     = static_cast<int16>(rec.Kind);
		p_cache_rec->TypeID   = static_cast<int16>(rec.GoodsTypeID);
		p_cache_rec->TaxGrpID = static_cast<int16>(rec.TaxGrpID);
		p_cache_rec->ClsID    = rec.GdsClsID;
		p_cache_rec->BrandID  = rec.BrandID;
		p_cache_rec->ManufID  = rec.ManufID;
		p_cache_rec->StrucID  = rec.StrucID;
		p_cache_rec->Flags    = rec.Flags;
		if(rec.Kind == PPGDSK_GOODS) {
			Goods2Tbl::Rec grp_rec;
			PPUnit unit_rec;
			if(p_cache_rec->TaxGrpID == 0 || p_cache_rec->TypeID == 0) {
				//
				// Здесь необходимо извлечь запись родительской группы товара, но одна трудность
				// заставляет усложнить процедуру: так как сейчас мы изменяем кэш товаров, то
				// штатные средства извлечения из него записей заблокированы.
				// Следовательно, мы вынуждены прибегать к низкоуровневому извлечению записи.
				//
				const Data * p_parent_entry = 0;
				uint   c_pos = 0;
				if(Search(p_cache_rec->ParentID, &c_pos) > 0)
					p_parent_entry = static_cast<const Data *>(SearchByPos(c_pos, 1));
				if(p_parent_entry) {
					SETIFZ(p_cache_rec->TaxGrpID, p_parent_entry->TaxGrpID);
					SETIFZ(p_cache_rec->TypeID, p_parent_entry->TypeID);
				}
				else {
					// @v10.4.2 THROW(goods_obj.Search(p_cache_rec->ParentID, &grp_rec) > 0);
					if(goods_obj.Search(p_cache_rec->ParentID, &grp_rec) > 0) { // @v10.4.2
						SETIFZ(p_cache_rec->TaxGrpID, static_cast<short>(grp_rec.TaxGrpID));
						SETIFZ(p_cache_rec->TypeID, static_cast<short>(grp_rec.GoodsTypeID));
					}
				}
			}
			if(p_cache_rec->UnitID && goods_obj.FetchUnit(p_cache_rec->UnitID, &unit_rec) > 0 && unit_rec.Flags & PPUnit::IntVal)
				p_cache_rec->Flags |= GF_INTVAL;
			if(sstreq(rec.Name, rec.Abbr))
				p_cache_rec->Flags |= GF_ABBREQNAME;
		}
		if(p_cache_rec->TypeID && oneof2(rec.Kind, PPGDSK_GOODS, PPGDSK_GROUP)) {
			PPObjGoodsType gt_obj;
			PPGoodsType gt_rec;
			if(gt_obj.Fetch(p_cache_rec->TypeID, &gt_rec) > 0) {
				if(p_cache_rec->TypeID != PPGT_DEFAULT)
					p_cache_rec->Flags |= GF_ODD;
				if(gt_rec.Flags & GTF_UNLIMITED)
					p_cache_rec->Flags |= GF_UNLIM;
				if(gt_rec.Flags & GTF_AUTOCOMPL)
					p_cache_rec->Flags |= GF_AUTOCOMPL;
				if(gt_rec.Flags & GTF_ASSETS)
					p_cache_rec->Flags |= GF_ASSETS;
				if(gt_rec.Flags & GTF_EXCLVAT)
					p_cache_rec->Flags |= GF_EXCLVAT;
			}
		}
		PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void GoodsCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	Goods2Tbl::Rec * p_data_rec = static_cast<Goods2Tbl::Rec *>(pDataRec);
	if(p_data_rec) {
		const Data * p_cache_rec = static_cast<const Data *>(pEntry);
		memzero(p_data_rec, sizeof(*p_data_rec));
		p_data_rec->ID       = p_cache_rec->ID;
		p_data_rec->ParentID = p_cache_rec->ParentID;
		p_data_rec->UnitID   = p_cache_rec->UnitID;
		p_data_rec->PhUnitID = p_cache_rec->PhUnitID;
		p_data_rec->PhUPerU  = p_cache_rec->PhUPerU;
		p_data_rec->Kind     = p_cache_rec->Kind;
		p_data_rec->GoodsTypeID = p_cache_rec->TypeID;
		p_data_rec->TaxGrpID = p_cache_rec->TaxGrpID;
		p_data_rec->GdsClsID = p_cache_rec->ClsID;
		p_data_rec->BrandID  = p_cache_rec->BrandID;
		p_data_rec->ManufID  = p_cache_rec->ManufID;
		p_data_rec->StrucID  = p_cache_rec->StrucID;
		p_data_rec->Flags    = p_cache_rec->Flags;
		GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
		if(p_data_rec->Name[0] == 0) {
			if(oneof5(p_data_rec->Kind, PPGDSK_GOODS, PPGDSK_GROUP, PPGDSK_BRAND, PPGDSK_BRANDGROUP, PPGDSK_SUPRWARE)) {
				EntryToDataFailed = 1;
			}
		}
	}
}
//
//
//
const StrAssocArray * GoodsCore::GetFullList()
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	return p_cache ? p_cache->GetFullList() : 0;
}

void GoodsCore::ReleaseFullList(const StrAssocArray * pList)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	CALLPTRMEMB(p_cache, ReleaseFullList(pList));
}

void GoodsCore::ResetFullList()
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	CALLPTRMEMB(p_cache, ResetFullList());
}

int FASTCALL GoodsCore::Fetch(PPID id, Goods2Tbl::Rec * pRec)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	id = labs(id);
	return p_cache ? p_cache->Get(id, pRec) : Search(id, pRec);
}

int GoodsCore::SearchBy2dBarcode(const char * pCodeLine, BarcodeTbl::Rec * pRec, Goods2Tbl::Rec * pGoodsRec)
{
	int    ok = -1;
	memzero(pRec, sizeof(*pRec));
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	if(p_cache) {
		const TwoDimBarcodeFormatArray * p_format = p_cache->GetBc2dSpec();
		if(p_format)
			ok = p_format->Search(this, pCodeLine, pRec, pGoodsRec);
	}
	return ok;
}

void FASTCALL GoodsCore::Dirty(PPID id)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 1);
	CALLPTRMEMB(p_cache, Dirty(id));
}

int GoodsCore::FetchStockExt(PPID id, GoodsStockExt * pExt)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 1);
	return p_cache ? p_cache->GetStockExt(id, pExt) : GetStockExt(id, pExt, 0);
}

int GoodsCore::FetchSingleBarcode(PPID id, SString & rBuf)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 1);
	return p_cache ? p_cache->GetSingleBarcode(id, rBuf) : GetSingleBarcode(id, 0, rBuf);
}

int GoodsCore::SearchGoodsAnalogs(PPID id, PPIDArray & rList, SString * pTransitComponentBuf)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 1);
	return p_cache ? p_cache->SearchGoodsAnalogs(id, rList, pTransitComponentBuf) : 0;
}

int FASTCALL GoodsCore::FetchConfig(PPGoodsConfig * pCfg)
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 1);
	if(p_cache) {
		return p_cache->GetConfig(pCfg, 0);
	}
	else {
		pCfg->Z();
		return 0;
	}
}

int GoodsCore::DirtyConfig()
{
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS, 0);
	return p_cache ? p_cache->GetConfig(0, 1) : 0;
}

int GoodsCore::Helper_GetGroupTerminalList(PPID parentID, PPIDArray & rList, PPIDArray & rUntermList)
{
	int    ok = 1;
	int    is_there_branches = 0;
	Goods2Tbl::Key1 k;
	MEMSZERO(k);
	k.Kind = PPGDSK_GROUP;
	k.ParentID = parentID;
	if(search(1, &k, spGe) && data.Kind == PPGDSK_GROUP && data.ParentID == parentID) {
		PPIDArray local_list;
		{
			MEMSZERO(k);
			k.Kind = PPGDSK_GROUP;
			k.ParentID = parentID;
			BExtQuery q(this, 1);
			q.select(this->ID, this->Flags, 0L).where(this->Kind == PPGDSK_GROUP && this->ParentID == parentID);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				local_list.add(this->data.ID);
			}
		}
		for(uint i = 0; i < local_list.getCount(); i++) {
			THROW(Helper_GetGroupTerminalList(local_list.get(i), rList, rUntermList)); // @recursion
			is_there_branches = 1;
		}
	}
	if(!is_there_branches) {
		THROW(rList.add(parentID));
	}
	THROW(rUntermList.add(parentID));
	CATCHZOK
	return ok;
}

int GoodsCore::GetGroupTerminalList(PPID parentID, PPIDArray * pList, PPIDArray * pUntermList)
{
	int    ok = 1;
	GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
	if(p_cache) {
		THROW(p_cache->GetGtl(parentID, pList, pUntermList));
	}
	else {
		PPIDArray local_list, local_unterm_list;
		THROW(Helper_GetGroupTerminalList(parentID, local_list, local_unterm_list));
		local_list.sortAndUndup();
		local_unterm_list.sortAndUndup();
		ASSIGN_PTR(pList, local_list);
		ASSIGN_PTR(pUntermList, local_unterm_list);
	}
	CATCHZOK
	return ok;
}

int GoodsCore::GetAltGroupFilt(PPID grpID, GoodsFilt * pFilt)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(Fetch(grpID, &goods_rec) > 0 && goods_rec.Flags & GF_ALTGROUP) {
		GoodsCache * p_cache = GetDbLocalCachePtr <GoodsCache> (PPOBJ_GOODS);
		if((goods_rec.Flags & GF_TEMPALTGROUP) || !p_cache || !p_cache->GetAltGrpFilt(grpID, pFilt)) {
			GoodsFilt flt;
			if(flt.ReadFromProp(PPOBJ_GOODSGROUP, grpID, GGPRP_GOODSFILT2, GGPRP_GOODSFLT_) > 0)
				ok = 1;
			*pFilt = flt;
			if(p_cache)
				THROW(p_cache->PutAltGrpFilt(grpID, pFilt));
		}
		else
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int GoodsCore::SearchByExt(const GoodsExtTbl::Rec * pExtRec, PPID * pGoodsID, Goods2Tbl::Rec * pRec)
{
	int    ok = -1;
	PPID   goods_id = 0;
	GoodsExtTbl::Key1 k1;
	MEMSZERO(k1);
	// {GoodsClsID, KindID, GradeID, X, Y, Z, AddObjID} (unique mod);
	k1.GoodsClsID = pExtRec->GoodsClsID;
	k1.KindID = pExtRec->KindID;
	k1.GradeID = pExtRec->GradeID;
	k1.X = pExtRec->X;
	k1.Y = pExtRec->Y;
	k1.Z = pExtRec->Z;
	k1.AddObjID = pExtRec->AddObjID;
	if(GeT.search(1, &k1, spEq)) {
		goods_id = GeT.data.GoodsID;
		if(Search(goods_id, pRec) > 0) {
			ASSIGN_PTR(pGoodsID, goods_id);
			ok = 1;
		}
		else {
			ok = PPSetObjError(PPERR_BL_EXT2GOODS, PPOBJ_GOODS, goods_id);
		}
	}
	else
		ok = -1;
	return ok;
}

DBQ & GoodsCore::SetupDimDBQ(const PPGdsClsPacket * pPack, int dim, const RealRange * pRng)
{
	DBQ * dbq = 0;
	DBItem * p_dbi = 0;
	switch(dim) {
		case PPGdsCls::eX: p_dbi = &GeT.X; break;
		case PPGdsCls::eY: p_dbi = &GeT.Y; break;
		case PPGdsCls::eZ: p_dbi = &GeT.Z; break;
		case PPGdsCls::eW: p_dbi = &GeT.W; break;
	}
	if(p_dbi && !pRng->IsZero()) {
		long   low_val = 0, upp_val = 0;
		pPack->RealToExtDim(pRng->low, dim, &low_val);
		pPack->RealToExtDim(pRng->upp, dim, &upp_val);
		dbq = & realrange(*p_dbi, low_val, upp_val);
	}
	return *dbq;
}

int GoodsCore::GetListByExtFilt(const ClsdGoodsFilt * pFilt, PPIDArray * pList)
{
	int    ok = -1;
	if(pFilt->GdsClsID == 0)
		ok = -2;
	else {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		GoodsExtTbl::Key1 k;
		BExtQuery q(&GeT, 1);
		DBQ * dbq = & (GeT.GoodsClsID == pFilt->GdsClsID);
		if(pFilt->GdsClsID && gc_obj.Fetch(pFilt->GdsClsID, &gc_pack) > 0) {
			dbq = ppcheckfiltid(dbq, GeT.KindID,    pFilt->KindList.GetSingle());
			dbq = ppcheckfiltid(dbq, GeT.GradeID,   pFilt->GradeList.GetSingle());
			dbq = ppcheckfiltid(dbq, GeT.AddObjID,  pFilt->AddObjList.GetSingle());
			dbq = ppcheckfiltid(dbq, GeT.AddObj2ID, pFilt->AddObj2List.GetSingle());
			dbq = & (*dbq && SetupDimDBQ(&gc_pack, PPGdsCls::eX, &pFilt->DimX_Rng));
			dbq = & (*dbq && SetupDimDBQ(&gc_pack, PPGdsCls::eY, &pFilt->DimY_Rng));
			dbq = & (*dbq && SetupDimDBQ(&gc_pack, PPGdsCls::eZ, &pFilt->DimZ_Rng));
			dbq = & (*dbq && SetupDimDBQ(&gc_pack, PPGdsCls::eW, &pFilt->DimW_Rng));
		}
		q.select(GeT.GoodsID, 0L).where(*dbq);
		MEMSZERO(k);
		k.GoodsClsID = pFilt->GdsClsID;
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			int    add_and_exit = 0;
			if(pFilt->Flags & ClsdGoodsFilt::fFirstGenGoods) {
				Goods2Tbl::Rec rec;
				if(Fetch(GeT.data.GoodsID, &rec) > 0)
					if(rec.Flags & GF_GENERIC)
						add_and_exit = 1;
					else
						continue;
			}
			ok = 1;
			if(pList) {
				if(pList->add(GeT.data.GoodsID)) {
					if(add_and_exit)
						break;
				}
				else {
					ok = 0;
					break;
				}
			}
		}
	}
	return ok;
}

int GoodsCore::BelongToDynGen(PPID goodsID, PPID * pGenID, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray list;
	Goods2Tbl::Rec rec;
	if(Fetch(goodsID, &rec) > 0 && rec.GdsClsID) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(rec.GdsClsID, &gc_pack) > 0 && gc_pack.Rec.DynGenMask) {
			GoodsExtTbl::Rec ext;
			if(GetExt(goodsID, &ext) > 0) {
				ClsdGoodsFilt flt;
				gc_pack.GetDynGenFilt(&ext, &flt);
				THROW(GetListByExtFilt(&flt, &list));
				if(list.getCount()) {
					if(pGenID) {
						PPID generic_id = 0;
						for(uint i = 0; !generic_id && i < list.getCount(); i++) {
							if(Fetch(list.get(i), &rec) > 0 && rec.Flags & GF_GENERIC)
								generic_id = rec.ID;
						}
						if(*pGenID) {
							if(*pGenID == generic_id)
								ok = 2;
						}
						else {
							*pGenID = generic_id;
							ok = 2;
						}
					}
					else
						ok = 2;
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pList, list);
	return ok;
}

int GoodsCore::GetDynGenericList(PPID genGoodsID, PPIDArray * pList)
{
	int    ok = -1;
	PPIDArray list;
	Goods2Tbl::Rec gen_rec;
	if(Fetch(genGoodsID, &gen_rec) > 0 && gen_rec.Flags & GF_GENERIC && gen_rec.GdsClsID) {
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		if(gc_obj.Fetch(gen_rec.GdsClsID, &gc_pack) > 0 && gc_pack.Rec.DynGenMask) {
			GoodsExtTbl::Rec gen_ext;
			if(GetExt(genGoodsID, &gen_ext) > 0) {
				ClsdGoodsFilt flt;
				gc_pack.GetDynGenFilt(&gen_ext, &flt);
				THROW(GetListByExtFilt(&flt, &list));
				for(uint i = 0; i < list.getCount();) {
					i++;
					Goods2Tbl::Rec rec;
					if(Fetch(list.at(i-1), &rec) > 0) {
						if(rec.Flags & GF_GENERIC)
							list.atFree(--i);
					}
					else
						list.atFree(--i);
				}
			}
		}
	}
	ok = list.getCount() ? 1 : -1;
	CATCHZOK
	CALLPTRMEMB(pList, addUnique(&list));
	return ok;
}
//
//
//
int GoodsCore::ReplaceArticleRefs(PPID replacedID, PPID newID, int use_ta)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->ReplaceArticleRefs(replacedID, newID, use_ta);
	else if(P_Qc2)
		ok = P_Qc2->ReplaceObj(PPOBJ_ARTICLE, replacedID, newID, use_ta);
	return ok;
}

int GoodsCore::GetListByQuotKind(PPID qkID, PPID locID, PPIDArray & rList)
{
	rList.Z();
	int    ok = -1;
	if(qkID) { 
		InitQc();
		if(P_Qc2) {
			//P_Qc2->Get
			PPIDArray temp_list;
			PPIDArray rel_list;
			QuotFilt qfilt;
			qfilt.QuotKindID = qkID;
			if(locID)
				qfilt.LocID = locID;
			else
				qfilt.Flags |= QuotFilt::fAllLocations;
			P_Qc2->GetRelListByFilt(&qfilt, rel_list);
			for(uint i = 0; i < rel_list.getCount(); i++) {
				const  PPID rel_id = rel_list.get(i);
				P_Qc2->GetGoodsList(rel_id, 1, temp_list.Z());
				rList.add(&temp_list);
			}
			if(rList.getCount()) {
				rList.sortAndUndup();
				ok = 1;
			}
		}
	}
	return ok;
}

int GoodsCore::GetQuot(PPID goodsID, const QuotIdent & rQi, double cost, double price, double * pVal, int useCache)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->GetCurr(goodsID, rQi, cost, price, pVal, useCache);
	else if(P_Qc2)
		ok = P_Qc2->GetCurr(goodsID, rQi, cost, price, pVal, useCache);
	return ok;
}

int GoodsCore::GetQuotNearest(PPID goodsID, const QuotIdent & rQi, PPQuot * pQuot, int useCache)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->GetNearest(goodsID, rQi, pQuot, useCache);
	else if(P_Qc2)
		ok = P_Qc2->GetNearest(goodsID, rQi, pQuot, useCache);
	return ok;
}

int GoodsCore::GetQuotList(PPID goodsID, PPID locID, PPQuotArray & rList)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->GetCurrList(goodsID, 0, locID, rList);
	else if(P_Qc2)
		ok = P_Qc2->GetCurrList(goodsID, 0, locID, rList);
	return ok;
}

int GoodsCore::RemoveAllQuotForQuotKind(PPID qkID, int use_ta)
{
	int    ok = 1;
	InitQc();
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(P_Qc) {
			THROW(deleteFrom(P_Qc, 0, (P_Qc->Kind == qkID)));
		}
		else if(P_Qc2) {
			THROW(P_Qc2->RemoveAllForQuotKind(qkID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int GoodsCore::ClearQuotCache()
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->ClearCache();
	else if(P_Qc2)
		ok = P_Qc2->ClearCache();
	return ok;
}

int GoodsCore::FetchQuotList(PPID goodsID, PPID qkID, PPID locID, PPQuotArray & rList)
{
	int    ok = 0;
	rList.clear();
	InitQc();
	if(P_Qc)
		ok = P_Qc->FetchList(goodsID, rList);
	else if(P_Qc2)
		ok = P_Qc2->FetchList(goodsID, rList);
	if(ok > 0) {
		if(qkID || locID) {
			uint c = rList.getCount();
			if(c) do {
				const PPQuot & r_q = rList.at(--c);
				if((qkID && r_q.Kind != qkID) || (locID && r_q.LocID != locID)) {
					rList.atFree(c);
				}
			} while(c);
		}
	}
	return ok;
}

int GoodsCore::GetMatrix(PPID locID, PPIDArray * pResult)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->GetMatrix(locID, pResult);
	else if(P_Qc2)
		ok = P_Qc2->GetMatrix(locID, pResult);
	return ok;
}

int GoodsCore::DirtyMatrix(const PPIDArray * pGoodsList, PPIDArray * pMtxLocList)
{
	int    ok = 0;
	InitQc();
	const int deferred = DS.IsDbCacheDeferredState(DBS.GetDbPathID());
	if(P_Qc)
		ok = P_Qc->DirtyMatrix(pGoodsList, pMtxLocList, deferred);
	else if(P_Qc2)
		ok = P_Qc2->DirtyMatrix(pGoodsList, pMtxLocList, deferred);
	return ok;
}

int GoodsCore::SetQuotList(const PPQuotArray & rQList, bool updByTime, int use_ta)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->SetCurrList(rQList, 0, false, use_ta);
	else if(P_Qc2)
		ok = P_Qc2->Set_(rQList, 0, 0, false, updByTime, use_ta);
	return ok;
}

int GoodsCore::SetQuotListQ(const PPQuotArray & rQList, const PPQuotArray * pTemplate, bool noRmv, int use_ta)
{
	int    ok = 0;
	assert(!(pTemplate && use_ta));
	InitQc();
	if(P_Qc)
		ok = P_Qc->SetCurrList(rQList, pTemplate, noRmv, use_ta);
	else if(P_Qc2)
		ok = P_Qc2->Set_(rQList, 0, pTemplate, noRmv, false, use_ta);
	return ok;
}

int GoodsCore::SetQuot(const PPQuot & rQuot, int use_ta)
{
	int    ok = 0;
	InitQc();
	if(P_Qc) {
		PPID   qid = 0;
		ok = P_Qc->SetCurr(&qid, &rQuot, 1, use_ta);
	}
	else if(P_Qc2) {
		ok = P_Qc2->Set(rQuot, 0, 1, use_ta);
	}
	return ok;
}

int  GoodsCore::GetMatrixRestrict(PPID mtxRestrQkID, PPID goodsGrpID, PPID locID, int srchNearest, long * pResult)
{
	int    ok = 0;
	InitQc();
	if(P_Qc)
		ok = P_Qc->GetMatrixRestrict(mtxRestrQkID, goodsGrpID, locID, srchNearest, pResult);
	else if(P_Qc2)
		ok = P_Qc2->GetMatrixRestrict(mtxRestrQkID, goodsGrpID, locID, srchNearest, pResult);
	return ok;
}

int GoodsCore::Helper_GetMtxByLoc(PPID locID, PPIDArray & rResult)
{
	int    ok = 1, r = 0;
	rResult.clear();
#if 0 // возникли проблемы {
	PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
	if(p_cli && !(CConfig.Flags2 & CCFLG2_DONTUSE3TIERGMTX)) {
		SString q;
		q.Cat("GETGOODSMATRIX").Space().Cat("LOCATION").CatChar('(').Cat(locID).CatChar(')').Space();
		q.Cat("FORMAT").DotCat("BIN").CatParStr(static_cast<const char *>(0));
		PPJobSrvReply reply;
		if(p_cli->Exec(q, reply)) {
			THROW(reply.StartReading(0));
			THROW(reply.CheckRepError());
			THROW_SL(reply.Read(&rResult, 0));
			r = 1;
		}
	}
#endif // } 0
	if(!r) {
		if(P_Qc) {
			THROW(P_Qc->GetMatrix(locID, &rResult));
		}
		else if(P_Qc2) {
			THROW(P_Qc2->GetMatrix(locID, &rResult));
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::GetMatrix(const ObjIdListFilt & rLocList, int orRule, PPIDArray * pResult)
{
	int    ok = 1;
	if(pResult) {
		InitQc();
		if(rLocList.IsEmpty() || rLocList.GetSingle()) {
			const  PPID loc_id = rLocList.GetSingle();
			THROW(Helper_GetMtxByLoc(loc_id, *pResult));
		}
		else {
			pResult->clear();
			PPIDArray temp_list;
			const PPIDArray & r_loc_list = rLocList.Get();
			for(uint i = 0; i < r_loc_list.getCount(); i++) {
				const  PPID loc_id = r_loc_list.get(i);
				THROW(Helper_GetMtxByLoc(loc_id, temp_list));
				if(orRule || i == 0) {
					THROW_SL(pResult->add(&temp_list));
					pResult->sortAndUndup();
				}
				else {
					pResult->intersect(&temp_list, 1);
					pResult->sortAndUndup();
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int GoodsCore::BelongToMatrix(PPID goodsID, PPID locID)
{
	int    ok = -1;
	InitQc();
	if(P_Qc)
		ok = P_Qc->BelongToMatrix(goodsID, locID);
	else if(P_Qc2)
		ok = P_Qc2->BelongToMatrix(goodsID, locID);
	if(!ok) {
		SString goods_name;
		PPSetError(PPERR_GOODSISNOTINMATRIX, GetGoodsName(goodsID, goods_name));
	}
	return ok;
}

int GoodsCore::LoadNameList(const PPIDArray * pIdList, long flags, StrAssocArray * pNameList)
{
	int    ok = 1;
	const  uint max_query_items = 32;
	uint   pos = 0;
	PPIDArray temp_src_list;
	THROW(temp_src_list.add(pIdList));
	temp_src_list.sortAndUndup();
	if(temp_src_list.getCount()) {
		SString temp_buf;
		const  PPID min_id = temp_src_list.at(0);
		const  PPID max_id = temp_src_list.getLast();
		if(min_id == max_id) {
			if(Search(min_id, 0) > 0) {
				temp_buf.Z();
				if(data.Kind == PPGDSK_GROUP)
					temp_buf.CatDiv('@', 2);
				temp_buf.Cat(data.Name);
				THROW_SL(pNameList->Add(data.ID, temp_buf));
			}
		}
		else {
			Goods2Tbl::Key0 k0;
			k0.ID = min_id;
			BExtQuery q(this, 0);
			q.select(this->ID, this->Kind, this->Name, 0L).where(this->ID >= min_id && this->ID <= max_id);
			for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;)
				if(temp_src_list.bsearch(data.ID)) {
					temp_buf.Z();
					if(data.Kind == PPGDSK_GROUP)
						temp_buf.CatDiv('@', 2);
					temp_buf.Cat(data.Name);
					THROW_SL(pNameList->Add(data.ID, temp_buf));
				}
		}
	}
	CATCHZOK
	return ok;
}

int GoodsCore::CorrectCycleLink(PPID id, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	Goods2Tbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(id, &rec) > 0 && rec.Kind == PPGDSK_GROUP) {
			int    r;
			PPID   parent_id = 0;
			SString added_msg;
			PPIDArray cycle_list;
			cycle_list.add(id);
			do {
				parent_id = rec.ParentID;
				int    f = 0;
				if(parent_id && rec.ID == parent_id) {
					f = 1;
				}
				else if(cycle_list.lsearch(parent_id)) {
					f = 2;
				}
				if(f) {
					added_msg = rec.Name;
					rec.ParentID = 0;
					r = Update(&rec.ID, &rec, 0);
					if(r)
						added_msg.Space().Cat("CORRECTED");
					if(f == 1)
						PPSetError(PPERR_GOODSGRPSELFPAR, added_msg);
					else
						PPSetError(PPERR_CYCLELINKGOODSGRP, added_msg);
					if(pLogger)
						pLogger->LogMsgCode(PPMSG_ERROR, PPErrCode, added_msg);
					else
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
					THROW(r);
					ok = 1;
					break;
				}
				else
					cycle_list.add(parent_id);
			} while(parent_id && Search(parent_id, &rec) > 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*
ushort __cdecl CalcCRC16(const char ptr[], uint sz)
{
	return 0;
}
*/
