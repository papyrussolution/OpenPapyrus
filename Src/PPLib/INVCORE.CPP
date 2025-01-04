// INVCORE.CPP
// Copyright (c) A.Sobolev 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2014, 2021, 2025
// @Kernel
//
#include <pp.h>
#pragma hdrstop

InventoryTotal::InventoryTotal()
{
	THISZERO();
}

InventoryCore::InventoryCore(char * pFileName) : InventoryTbl(pFileName)
{
}

int InventoryCore::_Search(PPID id, long * oprno, int sp, void * b)
{
	InventoryTbl::Key0 k;
	k.BillID = id;
	k.OprNo  = *oprno;
	if(search(0, &k, sp) && id == k.BillID) {
		*oprno = k.OprNo;
		copyBufTo(b);
		return 1;
	}
	return PPDbSearchError();
}

int InventoryCore::Search(PPID id, long oprno, void * b) { return _Search(id, &oprno, spEq, b); }

int InventoryCore::InitEnum(PPID billID, long * pHandle)
{
	BExtQuery * q = new BExtQuery(this, 0);
	q->selectAll().where(this->BillID == billID);
	InventoryTbl::Key0 k0;
	k0.BillID = billID;
	k0.OprNo = 0;
	q->initIteration(false, &k0, spGe);
	return EnumList.RegisterIterHandler(q, pHandle);
}

int InventoryCore::NextEnum(long enumHandle, void * pRec) { return (EnumList.NextIter(enumHandle) > 0) ? (copyBufTo(pRec), 1) : -1; }
int InventoryCore::DestroyIter(long enumHandle) { return EnumList.DestroyIterHandler(enumHandle); }

SEnum::Imp * InventoryCore::Enum(PPID billID)
{
	long   h = -1;
	return InitEnum(billID, &h) ? new PPTblEnum <InventoryCore>(this, h) : 0;
}

int InventoryCore::SearchByGoods(PPID billID, PPID goodsID, InventoryArray * pList)
{
	int    ok = -1;
	InventoryTbl::Key1 k;
	MEMSZERO(k);
	k.BillID = billID;
	k.GoodsID = goodsID;
	CALLPTRMEMB(pList, clear());
	if(search(1, &k, spGe) && k.BillID == billID && k.GoodsID == goodsID) {
		do {
			ok = 1;
			if(pList) {
				pList->insert(&data);
			}
			else
				break;
		} while(search(1, &k, spNext) && k.BillID == billID && k.GoodsID == goodsID);
	}
	return (ok > 0) ? ok : PPDbSearchError();
}

int InventoryCore::SearchIdentical(PPID billID, PPID goodsID, const char * pSerial, InventoryTbl::Rec * pRec)
{
	InventoryTbl::Key1 k;
	MEMSZERO(k);
	k.BillID = billID;
	k.GoodsID = goodsID;
	STRNSCPY(k.Serial, pSerial);
	return SearchByKey(this, 1, &k, pRec);
}

int InventoryCore::Remove(PPID id, int use_ta) { return deleteFrom(this, use_ta, this->BillID == id) ? 1 : PPSetErrorDB(); }

int InventoryCore::CalcDiff(InventoryTbl::Rec * pRec)
{
	if(pRec == 0)
		return -1;
	double diff = pRec->Quantity - pRec->StockRest;
	double pct_diff = 0.0;
	int    sign = 0;
	if(diff != 0.0) {
		sign = (diff > 0.0) ? 1 : -1;
		pct_diff = 100.0 * diff / ((pRec->StockRest != 0.0) ? pRec->StockRest : pRec->Quantity);
	}
	INVENT_SETDIFFSIGN(pRec->Flags, sign);
	pRec->DiffQtty    = fabs(diff);
	pRec->DiffPctQtty = fabs(pct_diff);
	if(!(pRec->Flags & INVENTF_WRITEDOFF))
		pRec->UnwritedDiff = pRec->DiffQtty;
	return 1;
}

int InventoryCore::Set(PPID id, long * pOprNo, InventoryTbl::Rec * pRec, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		CalcDiff(pRec);
		if(*pOprNo) {
			if(pRec) {
				InventoryTbl::Key0 k0;
				k0.BillID = id;
				k0.OprNo  = *pOprNo;
				if(searchForUpdate(0, &k0, spEq)) {
					pRec->BillID = id;
					pRec->OprNo  = *pOprNo;
					THROW_DB(updateRecBuf(pRec)); // @sfu
				}
				else {
					PPDbSearchError();
					CALLEXCEPT();
				}
			}
			else {
				THROW_DB(deleteFrom(this, 0, this->BillID == id && this->OprNo == *pOprNo));
			}
		}
		else {
			long   oprno = MAXLONG;
			THROW_INVARG(pRec);
			*pOprNo = (_Search(id, &oprno, spLe) > 0) ? (oprno + 1) : 1;
			pRec->BillID = id;
			pRec->OprNo  = *pOprNo;
			THROW_DB(insertRecBuf(pRec));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int InventoryCore::CheckFlags(const InventoryFilt * pFilt, const InventoryTbl::Rec * pRec)
{
	int    ok = 0;
	long   db_f  = pRec->Flags;
	long   f_f   = pFilt->Flags;
	if(f_f & (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
		if((f_f & (InventoryFilt::fLack | InventoryFilt::fSurplus)) == (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
			if(pRec->DiffQtty > 0)
				ok = 1;
		}
		else if(f_f & InventoryFilt::fLack)
			ok = (db_f & INVENTF_LACK) ? 1 : 0;
		else if(f_f & InventoryFilt::fSurplus)
			ok = (db_f & INVENTF_SURPLUS) ? 1 : 0;
	}
	else
		ok = 1;
	if(ok) {
		f_f = (f_f & (InventoryFilt::fWrOff | InventoryFilt::fUnwrOff));
		if(f_f == InventoryFilt::fWrOff)
			ok = (db_f & INVENTF_WRITEDOFF) ? 1 : 0;
		else if(f_f == InventoryFilt::fUnwrOff)
			ok = (db_f & INVENTF_WRITEDOFF) ? 0 : 1;
	}
	return ok;
}

int InventoryCore::CalcTotal(const InventoryFilt * pFilt, InventoryTotal * pTotal, PPIDArray * pGoodsList)
{
	int    ok = 1;
	InventoryTbl::Key0 k;
	PPObjGoods gobj;
	DBQ  * dbq = 0;
	PPIDArray bill_list;
	pFilt->BillList.Get(bill_list);
	const uint bcount = bill_list.getCount();
	if(pTotal) {
		if(pFilt->Flags & InventoryFilt::fMultipleTotal) {
			pTotal->BillCount += bcount;
		}
		else {
			memzero(pTotal, sizeof(*pTotal));
			pTotal->ID = pFilt->GetSingleBillID();
		}
	}
	for(uint i = 0; i < bcount; i++) {
		const  PPID bill_id = bill_list.get(i);
		int   do_skip = 0;
		if(pFilt->StorageLocID) {
			PPFreight freight;
			const  PPID storage_loc_id = (BillObj->FetchFreight(bill_id, &freight) > 0) ? freight.StorageLocID : 0;
			if(storage_loc_id != pFilt->StorageLocID)
				do_skip = 1;
		}
		if(!do_skip) {
			BExtQuery q(this, 0);
			q.select(this->GoodsID, this->Quantity, this->StockRest, this->Price, this->StockPrice,
				this->WrOffPrice, this->DiffQtty, this->CSesDfctQtty, this->CSesDfctPrice, this->Flags, 0L);
			dbq = & (this->BillID == bill_id);
			if(pFilt->Flags & (InventoryFilt::fLack | InventoryFilt::fSurplus)) {
				double minv = pFilt->MinVal;
				double maxv = pFilt->MaxVal;
				if(maxv >= minv && maxv > 0.0) {
					if(minv < 0.0)
						minv = 0.0;
					if(pFilt->Flags & InventoryFilt::fPctVal) {
						if(minv == maxv) {
							minv = MAX(0, minv - 1.0);
							maxv = maxv + 1.0;
						}
						dbq = & (*dbq && realrange(this->DiffPctQtty, minv, maxv));
					}
					else
						dbq = & (*dbq && realrange(this->DiffQtty, minv, maxv));
				}
			}
			q.where(*dbq);
			k.BillID = bill_id;
			k.OprNo = 0;
			for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
				if(CheckFlags(pFilt, &data) && gobj.BelongToGroup(data.GoodsID, pFilt->GoodsGrpID, 0)) {
					CALLPTRMEMB(pGoodsList, add(labs(data.GoodsID)));
					if(pTotal) {
						pTotal->ItemsCount++;
						pTotal->Quantity    += data.Quantity;
						pTotal->Amount      += R2(data.Quantity * data.Price);
						pTotal->StockRest   += data.StockRest;
						pTotal->StockAmount += R2(data.StockRest * data.StockPrice);
						if(data.Flags & INVENTF_SURPLUS)
							pTotal->Surplus += (data.WrOffPrice * data.DiffQtty);
						else if(data.Flags & INVENTF_LACK)
							pTotal->Lack    += (data.WrOffPrice * data.DiffQtty);
						pTotal->DfctQtty   += data.CSesDfctQtty;
						pTotal->DfctAmount += data.CSesDfctQtty * data.CSesDfctPrice;
					}
				}
			}
		}
	}
	CALLPTRMEMB(pGoodsList, sortAndUndup());
	return ok;
}
