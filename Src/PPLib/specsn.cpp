// SPECSN.CPP
// Copyright (c) A.Starodub 2004, 2005, 2006, 2007, 2008, 2012, 2013, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

#define SPCSNEXSTR_GOODSNAME        1 // “екстовое наименование товара (может отличатьс€ от GoodsID.Name)
#define SPCSNEXSTR_MANUFNAME        2 // “екстовое наименование производител€ (может отличатьс€ от ManufID.Name)
#define SPCSNEXSTR_LABNAME          3 // “екстовое наименование лаборатории, осущетсвл€вшей анализ
#define SPCSNEXSTR_MANUFCOUNTRYNAME 4 // “екстовое наименование страны происхождени€ (может отличатьс€ от ManufCountryID.Name)
#define SPCSNEXSTR_DESCRIPTION      5 // “екстовое описание серии (характер дефекта и т.д.)

static int FASTCALL IsValidSpcSeriesExStrID(int id)
{
	return oneof5(id, SPCSNEXSTR_GOODSNAME, SPCSNEXSTR_MANUFNAME, SPCSNEXSTR_LABNAME, SPCSNEXSTR_MANUFCOUNTRYNAME, SPCSNEXSTR_DESCRIPTION);
}

//static
int SLAPI SpecSeriesCore::GetExField(const SpecSeries2Tbl::Rec * pRec, int fldId, SString & rBuf)
{
	int    ok = -1;
	rBuf.Z();
	if(IsValidSpcSeriesExStrID(fldId)) {
		SString temp_buf = pRec->Tail;
		ok = PPGetExtStrData(fldId, temp_buf, rBuf);
	}
	return ok;
}

//static
int SLAPI SpecSeriesCore::SetExField(SpecSeries2Tbl::Rec * pRec, int fldId, const char * pBuf)
{
	int    ok = -1;
	if(IsValidSpcSeriesExStrID(fldId)) {
		SString temp_buf = pRec->Tail;
		ok = PPPutExtStrData(fldId, temp_buf, pBuf);
		temp_buf.CopyTo(pRec->Tail, sizeof(pRec->Tail));
	}
	return ok;
}

// AHTOXA {
SLAPI SpecSeriesCore::SpecSeriesCore() : SpecSeries2Tbl()
{
}

int SLAPI SpecSeriesCore::Search(PPID id, SpecSeries2Tbl::Rec * pRec)
{
	return SearchByID(this, PPOBJ_SPECSERIES, id, pRec);
}

int SLAPI SpecSeriesCore::Put(PPID * pID, SpecSeries2Tbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	SpecSeries2Tbl::Rec prev_rec;
	{
		PPTransaction tra(1);
		THROW(tra);
		if(pRec) {
			if(*pID) {
				THROW(SearchByID_ForUpdate(this, PPOBJ_SPECSERIES, *pID, &prev_rec) > 0);
				if(memcmp(pRec, &prev_rec, sizeof(prev_rec)) == 0)
					ok = -1;
				else {
					THROW_DB(updateRecBuf(pRec)); // @sfu
				}
			}
			else {
				THROW(AddByID(this, pID, pRec, 0));
			}
		}
		else if(*pID) {
			THROW(RemoveByID(this, *pID, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI SpecSeriesCore::ClearAll()
{
	return CurDict->RenewFile(*this, 0, 0);
}

int SLAPI SpecSeriesCore::SearchBySerial(PPID infoKind, const char * pBuf, SpecSeries2Tbl::Rec * pRec)
{
	SpecSeries2Tbl::Key1 k1;
	MEMSZERO(k1);
	k1.InfoKind = infoKind;
	STRNSCPY(k1.Serial, pBuf);
	return SearchByKey(this, 1, &k1, pRec);
}

int SLAPI SpecSeriesCore::GetListBySerial(PPID kind, const char * pSerial, StrAssocArray * pList)
{
	int    ok = -1;
	long   c = 0;
	SString temp_buf;
	SpecSeries2Tbl::Key1 k1;
	MEMSZERO(k1);
	k1.InfoKind = kind;
	STRNSCPY(k1.Serial, pSerial);
	BExtQuery q(this, 1);
	q.select(this->ID, this->Serial, this->Tail, 0).where(this->Serial == pSerial);
	for(q.initIteration(0, &k1, spEq); q.nextIteration() > 0;) {
		SpecSeriesCore::GetExField(&data, SPCSNEXSTR_GOODSNAME, temp_buf);
		CALLPTRMEMB(pList, Add(data.ID, temp_buf));
		ok = 1;
	}
	return ok;
}
//
// Implementation of PPALDD_UhttSpecSeries
//
struct UhttSpecSeriesBlock {
	enum {
		stFetch = 0,
		stSet
	};
	UhttSpecSeriesBlock()
	{
		Clear();
	}
	void Clear()
	{
		MEMSZERO(Rec);
		State = stFetch;
	}
	SpecSeriesCore      SsCore;
	SpecSeries2Tbl::Rec Rec;
	int    State;
};

PPALDD_CONSTRUCTOR(UhttSpecSeries)
{
	if(Valid) {
		Extra[0].Ptr = new UhttSpecSeriesBlock;
		InitFixData(rscDefHdr, &H, sizeof(H));
	}
}

PPALDD_DESTRUCTOR(UhttSpecSeries)
{
	Destroy();
	delete (UhttSpecSeriesBlock *)Extra[0].Ptr;
}

int PPALDD_UhttSpecSeries::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	UhttSpecSeriesBlock & r_blk = *(UhttSpecSeriesBlock *)Extra[0].Ptr;
	r_blk.Clear();
	MEMSZERO(H);
	if(r_blk.SsCore.Search(rFilt.ID, &r_blk.Rec) > 0) {
		SString temp_buf;
		H.ID = r_blk.Rec.ID;
		H.GoodsID = r_blk.Rec.GoodsID;
		SpecSeriesCore::GetExField(&r_blk.Rec, SPCSNEXSTR_GOODSNAME, temp_buf.Z());
		STRNSCPY(H.GoodsName, temp_buf);
		SpecSeriesCore::GetExField(&r_blk.Rec, SPCSNEXSTR_MANUFNAME, temp_buf.Z());
		STRNSCPY(H.ManufName, temp_buf);
		H.LabID = r_blk.Rec.LabID;
		H.ManufID = r_blk.Rec.ManufID;
		H.ManufCountryID = r_blk.Rec.ManufCountryID;
		STRNSCPY(H.Serial, r_blk.Rec.Serial);
		STRNSCPY(H.Barcode, r_blk.Rec.Barcode);
		H.InfoDate = r_blk.Rec.InfoDate;
		H.InfoKind = r_blk.Rec.InfoKind;
		STRNSCPY(H.InfoIdent, r_blk.Rec.InfoIdent);
		H.AllowDate = r_blk.Rec.AllowDate;
		STRNSCPY(H.AllowNumber, r_blk.Rec.AllowNumber);
		STRNSCPY(H.LetterType, r_blk.Rec.LetterType);
		H.Flags = r_blk.Rec.Flags;
		ok = DlRtm::InitData(rFilt, rsrv);
	}
	return ok;
}

int PPALDD_UhttSpecSeries::Set(long iterId, int commit)
{
	int    ok = 1;
	UhttSpecSeriesBlock & r_blk = *(UhttSpecSeriesBlock *)Extra[0].Ptr;
	if(r_blk.State != UhttSpecSeriesBlock::stSet) {
		r_blk.Clear();
		r_blk.State = UhttSpecSeriesBlock::stSet;
	}
	if(commit == 0) {
		if(iterId == 0) {
			r_blk.Rec.ID = H.ID;
			r_blk.Rec.GoodsID = H.GoodsID;
			if(sstrlen(H.GoodsName))
				SpecSeriesCore::SetExField(&r_blk.Rec, SPCSNEXSTR_GOODSNAME, H.GoodsName);
			r_blk.Rec.ManufID = H.ManufID;
			if(sstrlen(H.ManufName))
				SpecSeriesCore::SetExField(&r_blk.Rec, SPCSNEXSTR_MANUFNAME, H.ManufName);
			r_blk.Rec.LabID = H.LabID;
			r_blk.Rec.ManufCountryID = H.ManufCountryID;
			STRNSCPY(r_blk.Rec.Serial, H.Serial);
			STRNSCPY(r_blk.Rec.Barcode, H.Barcode);
			r_blk.Rec.InfoDate = H.InfoDate;
			r_blk.Rec.InfoKind = SPCSERIK_SPOILAGE;   // H.InfoKind;
			STRNSCPY(r_blk.Rec.InfoIdent, H.InfoIdent);
			r_blk.Rec.AllowDate = H.AllowDate;
			STRNSCPY(r_blk.Rec.AllowNumber, H.AllowNumber);
			STRNSCPY(r_blk.Rec.LetterType, H.LetterType);
			r_blk.Rec.Flags = H.Flags;
		}
	}
	else {
		PPID   id = r_blk.Rec.ID;
		THROW(r_blk.SsCore.Put(&id, &r_blk.Rec, 1));
		Extra[4].Ptr = (void *)id;
	}
	CATCHZOK
	if(commit || !ok)
		r_blk.Clear();
	return ok;
}
