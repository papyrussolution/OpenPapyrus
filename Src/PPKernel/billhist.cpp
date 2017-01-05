// BHISTCOR.CPP
// Copyright (c) A.Starodub 2004, 2006, 2007, 2008, 2009, 2010, 2015, 2016
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop
//
//
//
SLAPI HistBillCore::HistBillCore() : HistBillTbl()
{
}

int SLAPI HistBillCore::Search(PPID id, HistBillTbl::Rec * pRec)
{
	HistBillTbl::Key0 k;
	MEMSZERO(k);
	k.ID = id;
	return SearchByKey(this, 0, &k, pRec);
}

int SLAPI HistBillCore::SearchOpenBill(PPID billID, HistBillTbl::Rec * pRec)
{
	int    ok = -1;
	HistBillTbl::Key1 k;
	MEMSZERO(k);
	k.BillID  = billID;
	k.Ver     = HISTBILL_MAXVER;
	k.InnerID = MAXLONG;
	if(search(1, &k, spLe)) {
		copyBufTo(pRec);
		if(k.BillID == billID && k.Ver != HISTBILL_MAXVER)
			ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

//static
int SLAPI HistBillCore::HBRecToBRec(HistBillTbl::Rec * pHBRec, BillTbl::Rec * pBRec)
{
	if(pHBRec && pBRec) {
		pBRec->ID   = pHBRec->BillID;
		STRNSCPY(pBRec->Code, pHBRec->Code);
		pBRec->Dt      = pHBRec->Dt;
		pBRec->LocID   = pHBRec->LocID;
		pBRec->Object  = pHBRec->Object;
		pBRec->Object2 = pHBRec->Object2;
		pBRec->CurID   = pHBRec->CurID;
		pBRec->CRate   = pHBRec->CRate;
		pBRec->Amount  = BR2(pHBRec->Amount);
		pBRec->Flags   = pHBRec->Flags;
		pBRec->SCardID = pHBRec->SCardID;
	}
	else
		return (PPErrCode = PPERR_INVPARAM, -1);
	return 1;
}

int SLAPI HistBillCore::GetIdx(PPID billID, PPID * pVer, PPID * pInnerID)
{
	int    r = 0;
	PPID   ver = 1, id = 1;
	HistBillTbl::Rec rec;
	MEMSZERO(rec);
	if((r = SearchOpenBill(billID, &rec)) > 0) {
		ver = data.Ver + 1;
		id  = rec.InnerID;
	}
	else if(r <= 0) {
		id = rec.InnerID + 1;
		ver = 1;
	}
	ASSIGN_PTR(pVer, ver);
	ASSIGN_PTR(pInnerID, id);
	return 1;
}

int SLAPI HistBillCore::PutPacket(PPID * pID, PPHistBillPacket * pPack, int close, int use_ta)
{
	int    ok = 1;
	PPID   id = 0;
	HistTrfrTbl::Rec * p_rec;
	THROW_PP(pPack, PPERR_INVPARAM);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pPack->Head.InnerID == 0 || pPack->Head.Ver == 0)
			GetIdx(pPack->Head.BillID, &pPack->Head.Ver, &pPack->Head.InnerID);
		pPack->Head.Ver = close ? HISTBILL_MAXVER : pPack->Head.Ver;
		THROW_DB(insertRecBuf(&pPack->Head));
		id = data.ID;
		{
			BExtInsert bei(&ItemsTbl);
			for(uint i = 0; pPack->EnumItems(&i, &p_rec) > 0;) {
				p_rec->HistBillID = id;
				THROW_DB(bei.insert(p_rec));
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	ASSIGN_PTR(pID, id);
	CATCHZOK
	return ok;
}

int SLAPI HistBillCore::GetPacket(PPID id, PPHistBillPacket * pPack)
{
	int    ok = -1;
	char   str_id[20];
	THROW_PP(pPack, PPERR_INVPARAM);
	PPSetAddedMsgString(ltoa(id, str_id, 10));
	if(Search(id, &pPack->Head) > 0) {
		HistTrfrTbl::Key0 k;
		MEMSZERO(k);
		k.HistBillID = id;
		k.OprNo = 0;
		BExtQuery q(&ItemsTbl, 0, 48);
		q.selectAll().where(ItemsTbl.HistBillID == pPack->Head.ID);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;)
			THROW(pPack->InsertRow(&ItemsTbl.data));
		ok = 1;
	}
	else
		PPErrCode = PPERR_HISTBILLNOTFOUND;
	CATCHZOK
	return ok;
}

int SLAPI HistBillCore::DoMaintain(LDATE toDt, int recover, PPLogger * pLogger)
{
	int    ok = 1;
	SString msg, buf, fname;
	IterCounter counter;
	HistBillTbl::Key0 k;
	if(recover) {
		k.ID = MAXLONG;
		PPID last_id = 0;
		if(search(0, &k, spLe)) {
			last_id = data.ID;
			HistTrfrTbl::Key0 tk0;
			tk0.HistBillID = last_id+1;
			tk0.OprNo = 0;
			if(ItemsTbl.search(0, &tk0, spGe)) {
				PPLoadText(PPTXT_HISTBILL_HANGEDLINES, msg);
				if(pLogger) {
					pLogger->Log(msg);
				}
				else {
					PPLogMessage(PPFILNAM_ERR_LOG, msg, LOGMSGF_TIME|LOGMSGF_USER);
				}
				THROW_DB(deleteFrom(&ItemsTbl, 1, (ItemsTbl.HistBillID > last_id)));
			}
		}
	}
	if(toDt) {
		{
			SPathStruc ps;
			PPLoadText(PPTXT_DBMAINTAIN, buf);
			ps.Split(fileName);
			ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, fname);
			msg.Printf(buf, (const char *)fname);
		}
		PPTransaction tra(1);
		THROW(tra);
		counter.Init(this);
		MEMSZERO(k);
		while(search(0, &k, spGt)) {
			if(data.Dt <= toDt)
				THROW(Remove(data.ID, 0));
			PPWaitPercent(counter.Increment(), msg);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI HistBillCore::Remove(PPID id, int useTa)
{
	int    ok = 1;
	{
		PPTransaction tra(useTa);
		THROW(tra);
		THROW_DB(deleteFrom(&ItemsTbl, 0, (ItemsTbl.HistBillID == id)));
		THROW_DB(deleteFrom(this, 0, (this->ID == id)));
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI PPHistBillPacket::PPHistBillPacket()
{
	MEMSZERO(Head);
}

SLAPI PPHistBillPacket::~PPHistBillPacket()
{
	destroy();
}

void SLAPI PPHistBillPacket::destroy()
{
	MEMSZERO(Head);
	RemoveRows(0);
}

int SLAPI PPHistBillPacket::Init(PPBillPacket * pPack)
{
	int    ok = 1;
	if(pPack) {
		PPTransferItem * p_ti = 0;
		Head.BillID     = pPack->Rec.ID;
		STRNSCPY(Head.Code, pPack->Rec.Code);
		Head.Dt         = pPack->Rec.Dt;
		Head.OpID       = pPack->Rec.OpID;
		Head.LocID      = pPack->Rec.LocID;
		Head.Object     = pPack->Rec.Object;
		Head.Object2    = pPack->Rec.Object2;
		Head.CurID      = pPack->Rec.CurID;
		Head.CRate      = pPack->Rec.CRate;
		Head.Amount     = pPack->GetAmount();
		Head.LinkBillID = pPack->Rec.LinkBillID;
		Head.Flags      = pPack->Rec.Flags;
		Head.SCardID    = pPack->Rec.SCardID;
		Head.PayerID    = pPack->Ext.PayerID;
		Head.AgentID    = pPack->Ext.AgentID;
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti) > 0; ) {
			HistTrfrTbl::Rec h_item;
			MEMSZERO(h_item);
			h_item.OprNo      = (long)i;
			h_item.GoodsID    = p_ti->GoodsID;
			h_item.Quantity   = p_ti->Quantity_;
			h_item.Cost       = p_ti->Cost;
			h_item.Price      = p_ti->Price;
			h_item.Discount   = p_ti->Discount;
			h_item.QCertID    = p_ti->QCert;
			h_item.Expiry     = p_ti->Expiry;
			h_item.Flags      = p_ti->Flags;
			THROW(InsertRow(&h_item) > 0);
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPHistBillPacket::ConvertToBillPack(PPBillPacket * pPack)
{
	int    ok = 1;
	HistTrfrTbl::Rec * p_h_item;
	THROW_PP(pPack, PPERR_INVPARAM);
	THROW(pPack->CreateBlank_WithoutCode(Head.OpID, Head.LinkBillID, Head.LocID, 1));
	pPack->Rec.ID   = Head.BillID;
	STRNSCPY(pPack->Rec.Code, Head.Code);
	pPack->Rec.Dt         = Head.Dt;
	pPack->Rec.LocID      = Head.LocID;
	pPack->Rec.Object     = Head.Object;
	pPack->Rec.Object2    = Head.Object2;
	pPack->Rec.CurID      = Head.CurID;
	pPack->Rec.CRate      = Head.CRate;
	pPack->Rec.Amount     = BR2(Head.Amount);
	pPack->Rec.Flags      = Head.Flags;
	pPack->Rec.SCardID    = Head.SCardID;
	pPack->Ext.PayerID    = Head.PayerID;
	pPack->Ext.AgentID    = Head.AgentID;
	for(uint i = 0; EnumItems(&i, &p_h_item); ) {
		PPTransferItem ti(&pPack->Rec, TISIGN_UNDEF);
		ti.SetupGoods(p_h_item->GoodsID);
		ti.BillID   = pPack->Rec.ID;
		ti.GoodsID  = p_h_item->GoodsID;
		ti.Quantity_ = p_h_item->Quantity;
		ti.Cost     = p_h_item->Cost;
		ti.Price    = p_h_item->Price;
		ti.Discount = p_h_item->Discount;
		ti.QCert    = p_h_item->QCertID;
		ti.Expiry   = p_h_item->Expiry;
		ti.Flags    = p_h_item->Flags;
		THROW(pPack->InsertRow(&ti, 0));
	}
	CATCHZOK
	return ok;
}

PPHistBillPacket & FASTCALL PPHistBillPacket::operator = (const PPHistBillPacket & aPack)
{
	Head = aPack.Head;
	Items.copy(aPack.Items);
	return * this;
}

int SLAPI PPHistBillPacket::EnumItems(uint * pI, HistTrfrTbl::Rec** ppItem) const
{
	return Items.enumItems(pI, (void**)ppItem);
}

uint SLAPI PPHistBillPacket::GetCount() const
{
	return Items.getCount();
}

HistTrfrTbl::Rec & FASTCALL PPHistBillPacket::Item(uint p)
{
	return Items.at(p);
}

int SLAPI PPHistBillPacket::InsertRow(HistTrfrTbl::Rec * pItem)
{
	return (Items.insert(pItem)) ? 1 : PPSetErrorSLib();
}

int SLAPI PPHistBillPacket::RemoveRow(uint rowIdx)
{
	return Items.atFree(rowIdx) ? 1 : -1;
}

int SLAPI PPHistBillPacket::RemoveRows(IntArray * pPositions)
{
	if(pPositions) {
		for(int p = pPositions->getCount() - 1; p >= 0; p--) {
			RemoveRow(pPositions->at(p));
			pPositions->atFree(p);
		}
	}
	else
		Items.freeAll();
	return 1;
}
