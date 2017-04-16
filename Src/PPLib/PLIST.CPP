// PLIST.CPP
// Copyright (c) A.Sobolev 2001, 2003, 2005, 2006, 2007, 2009, 2013, 2014, 2015, 2016
//
#include <pp.h>
#pragma hdrstop

SLAPI PriceLineIdent::PriceLineIdent()
{
	THISZERO();
}
//
// PriceListCore
//
SLAPI PriceListCore::PriceListCore() : PriceListTbl()
{
}

int SLAPI PriceListCore::Search(PPID id, void * b)
{
	return SearchByID(this, PPOBJ_PRICELIST, id, b);
}

int SLAPI PriceListCore::Add(PPID * pID, void * b, int use_ta)
{
	return AddByID(this, pID, b, use_ta);
}

int SLAPI PriceListCore::Update(PPID id, void * b, int use_ta)
{
	return UpdateByID(this, 0, id, b, use_ta);
}

int SLAPI PriceListCore::Remove(PPID id, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(deleteFrom(&Lines, 0, Lines.ListID == id));
		THROW(deleteFrom(this, 0, this->ID == id));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

PriceLineTbl::Key0 * SLAPI PriceListCore::IdentToKey0(const PriceLineIdent * pIdent, PriceLineTbl::Key0 * pKey)
{
	pKey->ListID  = pIdent->PListID;
	pKey->GoodsID = pIdent->GoodsID;
	pKey->QuotKindID = pIdent->QuotKindID;
	pKey->LineNo = pIdent->LineNo;
	return pKey;
}

int SLAPI PriceListCore::SearchLine(const PriceLineIdent * pIdent, void * b)
{
	PriceLineTbl::Key0 k;
	return SearchByKey(&Lines, 0, IdentToKey0(pIdent, &k), b);
}

int SLAPI PriceListCore::SearchGoodsLine(PriceLineIdent * pIdent, void * b)
{
	PriceLineTbl::Key0 k;
	IdentToKey0(pIdent, &k);
	if(Lines.search(0, &k, spGe)) {
		if(k.ListID == pIdent->PListID && k.GoodsID == pIdent->GoodsID && k.QuotKindID == pIdent->QuotKindID) {
			pIdent->PListID = k.ListID;
			pIdent->GoodsID = k.GoodsID;
			pIdent->QuotKindID = k.QuotKindID;
			pIdent->LineNo = k.LineNo;
			Lines.copyBufTo(b);
			return 1;
		}
	}
	return PPDbSearchError();
}

int SLAPI PriceListCore::EnumLines(const PriceLineIdent * pIdent, PriceLineIdent * pIter, void * b)
{
	PriceLineTbl::Key0 k;
	IdentToKey0(pIter, &k);
	if(Lines.search(0, &k, spGt)) {
		if((!pIdent->PListID || pIdent->PListID == k.ListID) &&
			(!pIdent->GoodsID || pIdent->GoodsID == k.GoodsID) &&
			(!pIdent->QuotKindID || pIdent->QuotKindID == k.QuotKindID)) {
			pIter->PListID = k.ListID;
			pIter->GoodsID = k.GoodsID;
			pIter->QuotKindID = k.QuotKindID;
			pIter->LineNo = k.LineNo;
			Lines.copyBufTo(b);
			return 1;
		}
	}
	return PPDbSearchError();
}

int SLAPI PriceListCore::GetLineNo(const PriceLineIdent * pIdent, short * pLineNo)
{
	int    ok = 1;
	PriceLineTbl::Key0 k;
	IdentToKey0(pIdent, &k);
	if(Lines.search(0, &k, spLt) && k.ListID == pIdent->PListID &&
		k.GoodsID == pIdent->GoodsID && k.QuotKindID == pIdent->QuotKindID) {
		ASSIGN_PTR(pLineNo, (k.LineNo + 1));
	}
	else if(BTROKORNFOUND) {
		ASSIGN_PTR(pLineNo, 1);
	}
	else
		ok = PPSetErrorDB();
	return ok;
}

int SLAPI PriceListCore::AddLine(PPID id, void * b, PriceLineIdent * pIdent, int useSubst, int use_ta)
{
	int    ok = 1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	PriceLineIdent ident;
	PriceLineTbl::Rec * p_rec = (PriceLineTbl::Rec*)b;
	if(!useSubst)
		THROW(goods_obj.Fetch(p_rec->GoodsID, &goods_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		ident.PListID = id;
		ident.GoodsID = p_rec->GoodsID;
		ident.QuotKindID = p_rec->QuotKindID;
		THROW(GetLineNo(&ident, &p_rec->LineNo));
		ident.LineNo = p_rec->LineNo;
		p_rec->ListID = id;
		if(!useSubst) {
			STRNSCPY(p_rec->Name, goods_rec.Name);
			p_rec->GoodsGrpID = goods_rec.ParentID;
			p_rec->ManufID    = goods_rec.ManufID;
			p_rec->UnitID     = goods_rec.UnitID;
		}
		THROW_DB(Lines.insertRecBuf(p_rec));
		THROW(tra.Commit());
		ASSIGN_PTR(pIdent, ident);
	}
	CATCHZOK
	return ok;
}

int SLAPI PriceListCore::UpdateLine(const PriceLineIdent * pIdent, PriceLineTbl::Rec * pRec, int useSubst, int use_ta)
{
	int    ok = 1;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	if(!useSubst)
		THROW(goods_obj.Fetch(pRec->GoodsID, &goods_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchLine(pIdent) > 0);
		if(!useSubst) {
			pRec->GoodsGrpID = goods_rec.ParentID;
			pRec->ManufID    = goods_rec.ManufID;
			pRec->UnitID     = goods_rec.UnitID;
			// @v7.9.6 STRNSCPY(pRec->Name, goods_rec.Name);
		}
		THROW_DB(Lines.updateRecBuf(pRec));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PriceListCore::RemoveLine(const PriceLineIdent * pIdent, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(SearchLine(pIdent) > 0);
		THROW_DB(Lines.deleteRec());
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
