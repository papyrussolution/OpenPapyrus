// LOCTRANSF.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2020, 2021, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// @ModuleDef(LocTransfCore)
//
/*

Unit : Abstract/Common
	Common Unit: XYZ, Mass

Warehouse >>> Zone >>> Column x Row x Layer

*/
LocTransfOpBlock::LocTransfOpBlock(int domain, int op, PPID locID)
{
	Init(domain, op, locID);
}

LocTransfOpBlock::LocTransfOpBlock(const LocTransfOpBlock & rS)
{
	Domain = rS.Domain;
	LTOp = rS.LTOp;
	BillID = rS.BillID;
	RByBillLT = rS.RByBillLT;
	GoodsID = rS.GoodsID;
	LotID = rS.LotID;
	LocOwnerPersonID = rS.LocOwnerPersonID;
	LocID = rS.LocID;
	RByLoc = rS.RByLoc;
	PalletTypeID = rS.PalletTypeID;
	PalletCount = rS.PalletCount;
	Qtty = rS.Qtty;
	Flags = rS.Flags;
	UserID = rS.UserID;
	Dtm = rS.Dtm;
	OrderBillID = rS.OrderBillID;
	OrderRByBill = rS.OrderRByBill;
}

LocTransfOpBlock::LocTransfOpBlock(const LocTransfTbl::Rec & rRec)
{
	Domain = rRec.Domain;
	LTOp = rRec.LTOp;
	BillID = rRec.BillID;
	RByBillLT = rRec.RByBillLT;
	GoodsID = rRec.GoodsID;
	LotID = rRec.LotID;
	LocOwnerPersonID = rRec.LocOwnerPersonID;
	LocID = rRec.LocID;
	RByLoc = rRec.RByLoc;
	PalletTypeID = rRec.PalletTypeID;
	PalletCount = rRec.PalletCount;
	Qtty = rRec.Qtty;
	Flags = rRec.Flags;
	UserID = rRec.UserID;
	Dtm.Set(rRec.Dt, rRec.Tm);
	OrderBillID = rRec.OrderBillID;
	OrderRByBill = rRec.OrderRByBill;
}

LocTransfOpBlock & LocTransfOpBlock::Init(int domain, int op, PPID locID)
{
	Domain = domain; // @v12.4.1
	LTOp = op;
	BillID = 0;
	RByBillLT = 0;
	GoodsID = 0;
	LotID = 0;
	LocOwnerPersonID = 0; // @v12.4.1
	LocID = locID;
	RByLoc = 0;
	PalletTypeID = 0;
	PalletCount = 0;
	Qtty = 0.0;
	// @v12.4.1 {
	Flags = 0;            
	UserID = 0;
	Dtm = ZERODATETIME;
	OrderBillID = 0;
	OrderRByBill = 0;
	memzero(Reserve, sizeof(Reserve));
	// } @v12.4.1 
	return *this;
}

LocTransfOpBlock & FASTCALL LocTransfOpBlock::operator = (const LocTransfTbl::Rec * pRec)
{
	if(pRec) {
		Domain = pRec->Domain; // @v12.4.1
		LTOp = pRec->LTOp;
		BillID = pRec->BillID;
		RByBillLT = pRec->RByBillLT;
		GoodsID = pRec->GoodsID;
		LotID = pRec->LotID;
		LocOwnerPersonID = pRec->LocOwnerPersonID; // @v12.4.1
		LocID = pRec->LocID;
		RByLoc = pRec->RByLoc;
		PalletTypeID = pRec->PalletTypeID;
		PalletCount = pRec->PalletCount;
		Qtty = pRec->Qtty;
		// @v12.4.1 {
		Flags = pRec->Flags;         
		UserID = pRec->UserID;
		Dtm.Set(pRec->Dt, pRec->Tm);
		OrderBillID = pRec->OrderBillID;
		OrderRByBill = pRec->OrderRByBill;
		// } @v12.4.1 
	}
	return *this;
}

void FASTCALL LocTransfOpBlock::CopyTo(LocTransfTbl::Rec & rRec) const
{
	rRec.Domain = Domain;
	rRec.LTOp = LTOp;
	rRec.BillID = BillID;
	rRec.RByBillLT = RByBillLT;
	rRec.GoodsID = GoodsID;
	rRec.LotID = LotID;
	rRec.LocOwnerPersonID = LocOwnerPersonID;
	rRec.LocID = LocID;
	rRec.RByLoc = RByLoc;
	rRec.PalletTypeID = PalletTypeID;
	rRec.PalletCount = PalletCount;
	rRec.Qtty = Qtty;
	rRec.Flags = Flags;
	rRec.UserID = rRec.UserID;
	rRec.Dt = Dtm.d;
	rRec.Tm = Dtm.t;
	rRec.OrderBillID = OrderBillID;
	rRec.OrderRByBill = OrderRByBill;
}

bool FASTCALL LocTransfOpBlock::IsEq(const LocTransfOpBlock & rS) const
{
	return (Domain == rS.Domain && LTOp == rS.LTOp && BillID == rS.BillID && RByBillLT == rS.RByBillLT &&
		GoodsID == rS.GoodsID && LotID == rS.LotID && LocOwnerPersonID == rS.LocOwnerPersonID && LocID == rS.LocID &&
		RByLoc == rS.RByLoc && PalletTypeID == rS.PalletTypeID &&
		PalletCount == rS.PalletCount && fabs(Qtty) == fabs(rS.Qtty) &&
		Flags == rS.Flags && UserID == rS.UserID && Dtm == rS.Dtm && OrderBillID == rS.OrderBillID && OrderRByBill == rS.OrderRByBill);
}

bool FASTCALL LocTransfOpBlock::IsEq(const LocTransfTbl::Rec & rRec) const
{
	return (Domain == rRec.Domain && LTOp == rRec.LTOp && BillID == rRec.BillID && RByBillLT == rRec.RByBillLT &&
		GoodsID == rRec.GoodsID && LotID == rRec.LotID && LocOwnerPersonID == rRec.LocOwnerPersonID && LocID == rRec.LocID &&
		RByLoc == rRec.RByLoc && PalletTypeID == rRec.PalletTypeID &&
		PalletCount == rRec.PalletCount && fabs(Qtty) == fabs(rRec.Qtty) &&
		Flags == rRec.Flags && UserID == rRec.UserID && cmp(Dtm, rRec.Dt, rRec.Tm) == 0 && OrderBillID == rRec.OrderBillID && OrderRByBill == rRec.OrderRByBill);
}

LocTransfCore::LocTransfCore() : LocTransfTbl()
{
}

LocTransfCore::~LocTransfCore()
{
}

int LocTransfCore::Search(PPID locID, long rByLoc, LocTransfTbl::Rec * pRec)
{
	LocTransfTbl::Key0 k0;
	k0.LocID = locID;
	k0.RByLoc = rByLoc;
	return SearchByKey(this, 0, &k0, pRec);
}

int LocTransfCore::SearchRestByGoods(int domain, PPID goodsID, PPID locID, long rByLoc, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	if(rByLoc == 0) {
		LocTransfTbl::Key5 k5;
		MEMSZERO(k5);
		k5.Domain = domain; // @v12.4.1
		k5.LTOp = 0;
		k5.LocID = locID;
		k5.GoodsID = goodsID;
		if(search(5, &k5, spGe) && data.LTOp == 0 && data.LocID == locID && data.GoodsID == goodsID && data.Domain == domain) {
			do {
				if(data.RestByGoods > 0.0) {
					CopyBufTo(pRec);
					ok = 2;
				}
			} while(ok < 0 && search(5, &k5, spNext) && data.LTOp == 0 && data.LocID == locID && data.GoodsID == goodsID && data.Domain == domain);
		}
	}
	else {
		LocTransfTbl::Key2 k2;
		k2.GoodsID = goodsID;
		k2.LocID = locID;
		k2.RByLoc = rByLoc;
		while(ok < 0 && search(2, &k2, spLt) && data.GoodsID == goodsID && data.LocID == locID && data.Domain == domain) {
			if(data.LTOp != 0) {
				CopyBufTo(pRec);
				ok = 1;
			}
		}
	}
	return ok;
}

int LocTransfCore::SearchRestByLot(int domain, PPID lotID, PPID locID, long rByLoc, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	if(lotID) {
		LocTransfTbl::Key1 k1;
		MEMSZERO(k1);
		k1.LotID = lotID;
		k1.LocID = locID;
		if(rByLoc == 0) {
			while(ok < 0 && search(1, &k1, spGt) && data.LotID == lotID && data.LocID == locID && data.Domain == domain) {
				if(data.LTOp == 0) {
					CopyBufTo(pRec);
					ok = 2;
				}
			}
		}
		else {
			k1.RByLoc = rByLoc;
			while(ok < 0 && search(1, &k1, spLt) && data.LotID == lotID && data.LocID == locID && data.Domain == domain) {
				if(data.LTOp != 0) {
					CopyBufTo(pRec);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int LocTransfCore::GetTransByBill(PPID billID, int16 rByBill, TSVector <LocTransfTbl::Rec> * pList)
{
	LocTransfTbl::Key3 k3;
	BExtQuery q(this, 3, 128);
	q.selectAll().where(this->BillID == billID && this->RByBillLT == (long)rByBill);
	MEMSZERO(k3);
	k3.BillID  = billID;
	k3.RByBillLT = rByBill;
	for(q.initIteration(false, &k3, spEq); q.nextIteration() > 0;)
		pList->insert(&data);
	return 1;
}

int LocTransfCore::EnumByBill(PPID billID, int16 * pRByBill, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	LocTransfTbl::Key3 k3;
	k3.BillID = billID;
	k3.RByBillLT = *pRByBill;
	if(search(3, &k3, spGt) && data.BillID == billID) {
		*pRByBill = data.RByBillLT;
		CopyBufTo(pRec);
		ok = 1;
	}
	else if(!BTROKORNFOUND)
		ok = PPSetErrorDB();
	return ok;
}

int LocTransfCore::MakeOpBlockByOrder(const PPBillPacket & rOrderBPack, uint srcItemIdx/*[0..*/, LocTransfOpBlock & rBlk)
{
	int    ok = 0;
	if(srcItemIdx < SVectorBase::GetCount(rOrderBPack.P_LocTrfrList)) {
		const LocTransfOpBlock & r_src_blk = rOrderBPack.P_LocTrfrList->at(srcItemIdx);
		if(!(r_src_blk.Flags & LOCTRF_ORDER)) {
			; // @todo @err
		}
		else if(r_src_blk.BillID != rOrderBPack.Rec.ID) { // Если так, то это - внутренняя ошибка!
			; // @todo @err
		}
		else {
			rBlk.Init(r_src_blk.Domain, r_src_blk.LTOp, r_src_blk.LocID);
			rBlk.GoodsID = r_src_blk.GoodsID;
			rBlk.LotID = r_src_blk.LotID;
			rBlk.LocOwnerPersonID = r_src_blk.LocOwnerPersonID;
			rBlk.OrderBillID = r_src_blk.BillID;
			rBlk.OrderRByBill = r_src_blk.RByBillLT;
			// @todo Необходимо посчитать сколько уже было выполненно по этому заказу и скорректировать количество
			rBlk.Qtty = r_src_blk.Qtty;
			ok = 1;
		}
	}
	return ok;
}

int LocTransfCore::ValidateOpBlock(const LocTransfOpBlock & rBlk)
{
	int    ok = 1;
	LocationTbl::Rec loc_rec;
	THROW_PP(oneof2(rBlk.Domain, LOCTRFRDOMAIN_WMS, LOCTRFRDOMAIN_BAILMENT), PPERR_LOCTRFR_INVDOMAIN); // @v12.4.1
	THROW_PP(oneof3(rBlk.LTOp, LOCTRFROP_PUT, LOCTRFROP_GET, LOCTRFROP_INVENT), PPERR_LOCTRFR_INVOP);
	THROW_PP(rBlk.LocID, PPERR_LOCTRFR_ZEROLOC);
	THROW_PP(LocObj.Fetch(rBlk.LocID, &loc_rec) > 0, PPERR_LOCTRFR_UNEXLOC);
	if(rBlk.Domain == LOCTRFRDOMAIN_BAILMENT) {
		THROW_PP(loc_rec.Type == LOCTYP_ADDRESS, PPERR_LOCTRFR_INCLOCTYPE);
		THROW_PP(rBlk.LocOwnerPersonID, PPERR_LOCTRFR_BAILMENT_UNDEFLOCOWNER);
		THROW_PP(loc_rec.OwnerID == rBlk.LocOwnerPersonID, PPERR_LOCTRFR_BAILMENT_LOCNOTOFOWNER);
	}
	else {
		THROW_PP(loc_rec.Type == LOCTYP_WHCELL, PPERR_LOCTRFR_INCLOCTYPE);
	}
	THROW_PP(rBlk.GoodsID, PPERR_LOCTRFR_ZEROGOODS);
	THROW_PP(rBlk.Qtty > 0.0, PPERR_LOCTRFR_INVQTTY);
	CATCHZOK
	return ok;
}

int LocTransfCore::GetLastOpByLoc(PPID locID, long * pRByLoc, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	LocTransfTbl::Key0 k0;
	k0.LocID = locID;
	k0.RByLoc = MAXLONG;
	if(search(0, &k0, spLe) && k0.LocID == locID) {
		ASSIGN_PTR(pRByLoc, data.RByLoc);
		CopyBufTo(pRec);
		ok = 1;
	}
	else {
		ASSIGN_PTR(pRByLoc, 0);
		if(!BTROKORNFOUND)
			ok = PPSetErrorDB();
	}
	return ok;
}

int LocTransfCore::GetLastOpByLot(PPID locID, PPID lotID, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	if(lotID) {
		LocTransfTbl::Key1 k1;
		k1.LotID = lotID;
		k1.LocID = locID;
		k1.RByLoc = MAXLONG;
		if(search(1, &k1, spLe) && k1.LotID == lotID && k1.LocID == locID) {
			CopyBufTo(pRec);
			ok = 1;
		}
		else if(!BTROKORNFOUND)
			ok = PPSetErrorDB();
	}
	return ok;
}

int LocTransfCore::GetLastOpByGoods(PPID locID, PPID goodsID, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	LocTransfTbl::Key2 k2;
	k2.GoodsID = goodsID;
	k2.LocID = locID;
	k2.RByLoc = MAXLONG;
	if(search(2, &k2, spLe) && k2.GoodsID == goodsID && k2.LocID == locID) {
		CopyBufTo(pRec);
		ok = 1;
	}
	else if(!BTROKORNFOUND)
		ok = PPSetErrorDB();
	return ok;
}

int LocTransfCore::GetLastOpByBill(PPID billID, int16 * pRByBill, LocTransfTbl::Rec * pRec)
{
	int    ok = -1;
	if(billID) {
		LocTransfTbl::Key3 k3;
		k3.BillID = billID;
		k3.RByBillLT = MAXSHORT;
		if(search(3, &k3, spLe) && k3.BillID == billID) {
			ASSIGN_PTR(pRByBill, data.RByBillLT);
			CopyBufTo(pRec);
			ok = 1;
		}
		else {
			ASSIGN_PTR(pRByBill, 0);
			if(!BTROKORNFOUND)
				ok = PPSetErrorDB();
		}
	}
	return ok;
}

int LocTransfCore::PrepareRec(PPID locID, const LocTransfOpBlock * pBlk, LocTransfTbl::Rec * pRec)
{
	int    ok = 1;
	uint   i = 0;
	long   rbyloc = 0;
	int16  rbybill = 0;
	memzero(pRec, sizeof(*pRec));
	pRec->LocID = locID;
	THROW(GetLastOpByLoc(locID, &rbyloc, 0));
	pRec->RByLoc = rbyloc+1;
	// @v12.4.2 {
	if(pBlk && pBlk->Flags & LOCTRF_OWNEDBYBILL && pBlk->BillID) {
		pRec->BillID = pBlk->BillID;
		if(pBlk->RByBillLT <= 0) {
			THROW(GetLastOpByBill(pBlk->BillID, &rbybill, 0));
			pRec->RByBillLT = rbybill+1;
		}
		//
		/* @construction {
			SysJournal * p_sj = DS.GetTLA().P_SysJ;
			SysJournalTbl::Rec cr_sj_rec;
			if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, billID, &cr_sj_rec) > 0) {
				
			}
		}*/
		getcurdatetime(&pRec->Dt, &pRec->Tm); // @todo Заменить на {дату документа; время создания документа}
	}
	else {
		getcurdatetime(&pRec->Dt, &pRec->Tm);
	}
	if(pBlk)
		pRec->Flags |= (pBlk->Flags & (LOCTRF_OWNEDBYBILL|LOCTRF_ORDER));
	// } @v12.4.2 
	/* @v7.2.6 Ошибочный участок кода: RByBill должен соответствовать значению RByBill из BillTbl
	if(billID) {
		int16  rbybill = 0;
		pRec->BillID = billID;
		THROW(GetLastOpByBill(billID, &rbybill, 0));
		pRec->RByBill = rbybill+1;
	}
	*/
	pRec->UserID = LConfig.UserID;
	CATCHZOK
	return ok;
}

int LocTransfCore::RemoveOp(PPID locID, long rByLoc, int use_ta)
{
	int    ok = 1;
	DBRowId pos;
	LocTransfTbl::Rec rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			THROW(Search(locID, rByLoc, &rec) > 0);
			THROW_PP(rec.LinkLocID == 0, PPERR_LOCTRFR_RMVSHADOWREC);
			THROW_DB(getPosition(&pos));
			{
				if(!(rec.Flags & LOCTRF_ORDER)) { // @v12.4.2 (condition)
					double addendum = -rec.Qtty;
					THROW(UpdateForward(locID, rByLoc, rec.GoodsID, rec.LotID, 0, &addendum));
					THROW(UpdateCurrent(rec.Domain, locID, rec.GoodsID, rec.LotID, addendum));
				}
				THROW_DB(getDirectForUpdate(0, 0, pos));
				THROW_DB(deleteRec()); // @sfu
				{
					//
					// Теперь надо удалить связанные с этой записи
					//
					THROW_DB(deleteFrom(this, 0, this->LinkLocID == locID && this->LinkRByLoc == rByLoc));
				}
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int LocTransfCore::UpdateCurrent(int domain, PPID locID, PPID goodsID, PPID lotID, double addendum)
{
	int    ok = 1;
	const  LDATETIME now_dtm = getcurdatetime_();
	int    r;
	const  PPID user_id = LConfig.UserID;
	LocTransfTbl::Rec rec;
	if(lotID) {
		r = SearchRestByLot(domain, lotID, locID, 0, &rec);
		if(r > 0) {
			rec.UserID = user_id;
			rec.Dt = now_dtm.d;
			rec.Tm = now_dtm.t;
			rec.RestByLot += addendum;
			THROW_PP(rec.RestByLot >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_LOT);
			THROW_DB(updateRecBuf(&rec));
		}
		else {
			THROW(PrepareRec(locID, 0, &rec));
			rec.Domain = domain;
			if(domain == LOCTRFRDOMAIN_BAILMENT) {
				//rec.LocOwnerPersonID = 
			}
			rec.LotID = lotID;
			rec.RestByLot = addendum;
			THROW_PP(rec.RestByLot >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_LOT);
			THROW_DB(insertRecBuf(&rec));
		}
	}
	if(goodsID) {
		r = SearchRestByGoods(domain, goodsID, locID, 0, &rec);
		if(r > 0) {
			rec.UserID = user_id;
			rec.Dt = now_dtm.d;
			rec.Tm = now_dtm.t;
			rec.RestByGoods += addendum;
			THROW_PP_S(rec.RestByGoods >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_GOODS, GetGoodsName(goodsID, SLS.AcquireRvlStr()));
			THROW_DB(updateRecBuf(&rec));
		}
		else {
			THROW(PrepareRec(locID, 0, &rec));
			rec.Domain = domain;
			rec.GoodsID = goodsID;
			rec.RestByGoods = addendum;
			THROW_PP_S(rec.RestByGoods >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_GOODS, GetGoodsName(goodsID, SLS.AcquireRvlStr()));
			THROW_DB(insertRecBuf(&rec));
		}
	}
	CATCHZOK
	return ok;
}

int LocTransfCore::UpdateForward(PPID locID, long rByLoc, PPID goodsID, PPID lotID, int check, double * pAddendum)
{
	int    ok = 1;
	int    valid = 1;
	double neck = fabs(*pAddendum);
	if(check || neck != 0.0) {
		if(lotID) {
			LocTransfTbl::Key1 k1;
			k1.LotID = lotID;
			k1.LocID = locID;
			k1.RByLoc = rByLoc;
			if(check) {
				while(search(1, &k1, spGt) && data.LotID == lotID && data.LocID == locID) {
					if(data.LTOp != 0) {
						SETMIN(neck, data.RestByLot);
						data.RestByLot = R6(data.RestByLot + *pAddendum);
						if(data.RestByLot < 0.0)
							valid = 0;
					}
				}
			}
			else {
				while(searchForUpdate(1, &k1, spGt) && data.LotID == lotID && data.LocID == locID) {
					if(data.LTOp != 0) {
						data.RestByLot = R6(data.RestByLot + *pAddendum);
						THROW_PP(data.RestByLot >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_LOT);
						THROW_DB(updateRec()); // @sfu
					}
				}
			}
		}
		{
			LocTransfTbl::Key2 k2;
			k2.GoodsID = goodsID;
			k2.LocID = locID;
			k2.RByLoc = rByLoc;
			if(check) {
				while(search(2, &k2, spGt) && data.GoodsID == goodsID && data.LocID == locID) {
					if(data.LTOp != 0) {
						SETMIN(neck, data.RestByGoods);
						data.RestByGoods = R6(data.RestByGoods + *pAddendum);
						if(data.RestByGoods < 0.0)
							valid = 0;
					}
				}
			}
			else {
				while(searchForUpdate(2, &k2, spGt) && data.GoodsID == goodsID && data.LocID == locID) {
					if(data.LTOp != 0) {
						data.RestByGoods = R6(data.RestByGoods + *pAddendum);
						THROW_PP(data.RestByGoods >= 0.0, PPERR_LOCTRFR_FWLOTRESTBOUND_GOODS);
						THROW_DB(updateRec()); // @sfu
					}
				}
			}
		}
	}
	ok = valid ? 1 : -1;
	CATCHZOK
	return ok;
}

int LocTransfCore::PutOp(const LocTransfOpBlock & rBlk, int * pRByLoc, int * pRByBill, int use_ta)
{
	int    ok = 1;
	int    r;
	int    rbyloc = rBlk.RByLoc;
	int    rbybill = rBlk.RByBillLT; // @v12.4.2
	LocTransfTbl::Rec rec;
	LocTransfTbl::Rec ex_rec;
	THROW(ValidateOpBlock(rBlk));
	{
		const  PPID loc_id = rBlk.LocID;
		double rest_by_lot = 0.0;
		double rest_by_goods = 0.0;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(rbyloc) {
			int    recalc_forward = 0;
			THROW(Search(loc_id, rbyloc, &ex_rec) > 0);
			const  double prev_qtty = ex_rec.Qtty;
			THROW(ex_rec.BillID == rBlk.BillID && ex_rec.RByBillLT == rbybill);
			if(!(ex_rec.Flags & LOCTRF_ORDER)) {
				if(rBlk.GoodsID != ex_rec.GoodsID || rBlk.LotID != ex_rec.LotID) {
					double addendum = -prev_qtty;
					recalc_forward = 1;
					THROW(UpdateForward(ex_rec.LocID, ex_rec.RByLoc, ex_rec.GoodsID, ex_rec.LotID, 0, &addendum));
				}
			}
			{
				LocTransfTbl::Key0 k0;
				//
				// Находим остаток на момент операции (если не заказ)
				//
				if(!(rBlk.Flags & LOCTRF_ORDER)) {
					if(rBlk.LotID) {
						THROW(r = SearchRestByLot(rBlk.Domain, rBlk.LotID, loc_id, rbyloc, &ex_rec));
						if(r > 0)
							rest_by_lot = ex_rec.RestByLot;
					}
					THROW(r = SearchRestByGoods(rBlk.Domain, rBlk.GoodsID, loc_id, rbyloc, &ex_rec));
					if(r > 0)
						rest_by_goods = ex_rec.RestByGoods;
				}
				//
				k0.LocID = loc_id;
				k0.RByLoc = rbyloc;
				THROW(SearchByKey_ForUpdate(this, 0, &k0, &rec) > 0);
				rec.GoodsID = rBlk.GoodsID;
				rec.LotID   = rBlk.LotID;
				rec.PalletTypeID = rBlk.PalletTypeID;
				rec.PalletCount = rBlk.PalletCount;
				if(rec.LTOp == LOCTRFROP_PUT)
					rec.Qtty = +fabs(rBlk.Qtty);
				else if(rec.LTOp == LOCTRFROP_GET)
					rec.Qtty = -fabs(rBlk.Qtty);
				else if(rec.LTOp == LOCTRFROP_INVENT)
					rec.Qtty = rBlk.LotID ? (fabs(rBlk.Qtty) - rest_by_lot) : (fabs(rBlk.Qtty) - rest_by_goods);
				else {
					CALLEXCEPT(); // invalid op
				}
				double addendum = recalc_forward ? rec.Qtty : (rec.Qtty - prev_qtty);
				if(!(rec.Flags & LOCTRF_ORDER)) {
					rec.RestByLot = rBlk.LotID ? (rest_by_lot + rec.Qtty) : 0.0;
					rec.RestByGoods = rest_by_goods + rec.Qtty;
				}
				//
				THROW_DB(updateRecBuf(&rec));
				if(!(rec.Flags & LOCTRF_ORDER)) { // @v12.4.2 (condition)
					THROW(UpdateForward(rec.LocID, rec.RByLoc, rec.GoodsID, rec.LotID, 0, &addendum));
					THROW(UpdateCurrent(rec.Domain, rec.LocID, rec.GoodsID, rec.LotID, addendum));
				}
			}
		}
		else {
			THROW(PrepareRec(loc_id, &rBlk, &rec));
			rbyloc   = rec.RByLoc;
			// @v12.4.1 {
			rec.Domain = rBlk.Domain; 
			if(rBlk.Domain == LOCTRFRDOMAIN_BAILMENT) {
				rec.LocOwnerPersonID = rBlk.LocOwnerPersonID;
			}
			// } @v12.4.1 
			rec.LTOp   = rBlk.LTOp;
			rec.GoodsID      = rBlk.GoodsID;
			rec.LotID        = rBlk.LotID;
			rec.PalletTypeID = rBlk.PalletTypeID;
			rec.PalletCount  = rBlk.PalletCount;
			rec.Qtty = rBlk.Qtty;
			if(!(rBlk.Flags & LOCTRF_ORDER)) {
				if(rBlk.LotID) {
					THROW(r = SearchRestByLot(rBlk.Domain, rBlk.LotID, loc_id, 0, &ex_rec));
					if(r > 0)
						rest_by_lot = ex_rec.RestByLot;
				}
				THROW(r = SearchRestByGoods(rBlk.Domain, rBlk.GoodsID, loc_id, 0, &ex_rec));
				if(r > 0)
					rest_by_goods = ex_rec.RestByGoods;
			}
			if(rec.LTOp == LOCTRFROP_PUT)
				rec.Qtty = +fabs(rBlk.Qtty);
			else if(rec.LTOp == LOCTRFROP_GET)
				rec.Qtty = -fabs(rBlk.Qtty);
			else if(rec.LTOp == LOCTRFROP_INVENT)
				rec.Qtty = rBlk.LotID ? (fabs(rBlk.Qtty) - rest_by_lot) : (fabs(rBlk.Qtty) - rest_by_goods);
			else {
				CALLEXCEPT(); // invalid op
			}
			if(!(rec.Flags & LOCTRF_ORDER)) {
				if(rBlk.LotID)
					rec.RestByLot = rest_by_lot + rec.Qtty;
				rec.RestByGoods = rest_by_goods + rec.Qtty;
			}
			THROW_PP(rec.RestByLot >= 0.0, PPERR_WHCELLRESTLOT);
			THROW_PP(rec.RestByGoods >= 0.0, PPERR_WHCELLRESTGOODS);
			THROW_DB(insertRecBuf(&rec));
			if(!(rec.Flags & LOCTRF_ORDER)) { // @v12.4.2 (condition)
				THROW(UpdateCurrent(rec.Domain, rec.LocID, rec.GoodsID, rec.LotID, rec.Qtty));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pRByLoc, rbyloc);
	return ok;
}

int LocTransfCore::GetNonEmptyCellList(const PPIDArray * pCellList, PPIDArray * pList)
{
	int    ok = 1;
	PPIDArray cell_list;
	PPIDArray result_cell_list;
	BExtQuery q(this, 5);
	LocTransfTbl::Key5 k5;
	MEMSZERO(k5);
	k5.Domain = LOCTRFRDOMAIN_WMS; // @v12.4.1
	k5.LTOp = 0;
	q.select(this->LocID, this->RestByGoods, 0L).where(this->LTOp == 0L && this->RestByGoods > 0.0);
	for(q.initIteration(false, &k5, spGe); q.nextIteration() > 0;) {
		if(!pCellList || pCellList->lsearch(data.LocID)) {
			THROW_SL(result_cell_list.addUnique(data.LocID));
		}
	}
	ASSIGN_PTR(pList, result_cell_list);
	CATCHZOK
	return ok;
}

int LocTransfCore::GetEmptyCellList(const PPIDArray * pCellList, PPIDArray * pList)
{
	int    ok = 1;
	PPIDArray result_list;
	PPIDArray cell_list;
	PPIDArray non_empty_cell_list;
	if(!pCellList) {
		THROW(LocObj.ResolveWhCellList(0, 0, cell_list));
		pCellList = &cell_list;
	}
	THROW(GetNonEmptyCellList(pCellList, &non_empty_cell_list));
	THROW_SL(result_list.addUniqueExclusive(pCellList, &non_empty_cell_list));
	CATCHZOK
	ASSIGN_PTR(pList, result_list);
	return ok;
}

int LocTransfCore::GetCellListForGoods(PPID goodsID, const PPIDArray * pCellList, RAssocArray * pList)
{
	int    ok = -1;
	BExtQuery q(this, 2);
	LocTransfTbl::Key2 k2;
	MEMSZERO(k2);
	k2.GoodsID = goodsID;
	q.select(this->LocID, this->RestByGoods, 0L).where(this->GoodsID == goodsID && this->LTOp == 0L && this->RestByGoods > 0.0);
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		if(!pCellList || pCellList->lsearch(data.LocID)) {
			CALLPTRMEMB(pList, Add(data.LocID, data.RestByGoods));
			ok = 1;
		}
	}
	return ok;
}

int LocTransfCore::GetLocCellList(PPID goodsID, PPID parentLocID, RAssocArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, freeAll());
	if(goodsID) {
		BExtQuery q(this, 2);
		LocTransfTbl::Key2 k2;
		MEMSZERO(k2);
		k2.GoodsID = goodsID;
		q.select(this->LocID, this->RestByGoods, 0L).where(this->GoodsID == goodsID && this->LTOp == 0L && this->RestByGoods > 0.0);
		for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
			PPID parent_loc_id = 0;
			if(!parentLocID || LocObj.GetParentWarehouse(data.LocID, &parent_loc_id) > 0 && parentLocID == parent_loc_id) {
				CALLPTRMEMB(pList, Add(data.LocID, data.RestByGoods));
				ok = 1;
			}
		}
	}
	return ok;
}

int LocTransfCore::GetGoodsList(int domain, PPID locID, RAssocArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, freeAll());
	if(locID) {
		BExtQuery q(this, 5);
		LocTransfTbl::Key5 k5;
		MEMSZERO(k5);
		k5.Domain = domain;
		k5.LocID = locID;
		q.select(LocID, GoodsID, RestByGoods, 0L).where(Domain == domain && LocID == locID && LTOp == 0L);
		for(q.initIteration(false, &k5, spGe); q.nextIteration() > 0;) {
			CALLPTRMEMB(pList, Add(data.GoodsID, data.RestByGoods));
			ok = 1;
		}
	}
	return ok;
}

int LocTransfCore::GetDisposition(PPID billID, int rByBill, TSVector <LocTransfTbl::Rec> & rDispositionList)
{
	int    ok = -1;
	LocTransfTbl::Key3 k3;
	k3.BillID = billID;
	k3.RByBillLT = rByBill;
	if(search(3, &k3, spEq))
		do {
			THROW_SL(rDispositionList.insert(&data));
			ok = 1;
		} while(search(3, &k3, spNext) && data.BillID == billID && data.RByBillLT == rByBill);
	THROW(BTROKORNFOUND);
	CATCHZOK
	return ok;
}

int LocTransfCore::GetDisposition(PPID billID, TSVector <LocTransfTbl::Rec> & rDispositionList)
{
	int    ok = -1;
	LocTransfTbl::Key3 k3;
	k3.BillID = billID;
	k3.RByBillLT = 0;
	if(search(3, &k3, spGe) && data.BillID == billID)
		do {
			THROW_SL(rDispositionList.insert(&data));
			ok = 1;
		} while(search(3, &k3, spNext) && data.BillID == billID);
	THROW(BTROKORNFOUND);
	CATCHZOK
	return ok;
}
//
//
//
LocTransfDisposeItem::LocTransfDisposeItem()
{
	THISZERO();
}

LocTransfDisposer::LocTransfDisposer() : GtoAssc(PPASS_GOODS2WAREPLACE, PPOBJ_LOCATION, 1), State(0)
{
}

LocTransfDisposer::~LocTransfDisposer()
{
}

int LocTransfDisposer::SetupOpBlock(LocTransfDisposeItem & rItem, PPID whCellID, double * pQtty, LocTransfOpBlock & rBlk)
{
	const  int _domain = LOCTRFRDOMAIN_WMS;
	int    ok = 1;
	double pallet_qtty = 0.0;
	GoodsStockExt gse;
	GoodsStockExt::Pallet plt;
	rItem.LocID = whCellID;
	rBlk.Init(_domain, rItem.Op, whCellID);
	rBlk.GoodsID = rItem.GoodsID;
	rBlk.BillID = rItem.BillID;
	rBlk.RByBillLT = rItem.BillTiIdx;
	//rBlk.LotID = rItem.LotID;
	GObj.GetStockExt(rItem.GoodsID, &gse, 1);
	if(gse.GetSinglePalletEntry(&plt) > 0 && plt.IsValid()) {
		if(gse.Package > 0.0)
			pallet_qtty = gse.Package * plt.PacksPerLayer * plt.MaxLayers;
		else
			pallet_qtty = plt.PacksPerLayer * plt.MaxLayers;
	}
	if(rItem.Op == LOCTRFROP_PUT) {
		if(pallet_qtty > 0.0) {
			if(*pQtty > pallet_qtty)
				rBlk.Qtty = pallet_qtty;
			else
				rBlk.Qtty = *pQtty;
			rBlk.PalletTypeID = plt.PalletTypeID;
			rBlk.PalletCount = 1;
		}
		else
			rBlk.Qtty = *pQtty;
	}
	else if(rItem.Op == LOCTRFROP_GET) {
		LocTransfTbl::Rec rest_rec;
		if(LtT.SearchRestByGoods(_domain, rItem.GoodsID, whCellID, 0, &rest_rec) > 0 && rest_rec.RestByGoods > 0.0) {
			rBlk.Qtty = smin(*pQtty, rest_rec.RestByGoods);
			if(pallet_qtty > 0.0 && rBlk.Qtty >= pallet_qtty) {
				double rem = fmod(rBlk.Qtty, pallet_qtty);
				if(rem == 0.0) {
					rBlk.PalletTypeID = plt.PalletTypeID;
					rBlk.PalletCount = (int16)(rBlk.Qtty / pallet_qtty);
				}
			}
		}
	}
	if(rBlk.Qtty > 0.0) {
		*pQtty -= rBlk.Qtty;
	}
	else
		ok = -1;
	return ok;
}

int LocTransfDisposer::Dispose(const PPIDArray & rBillList, PPLogger * pLogger, int use_ta)
{
	int    ok = -1, r;
	PPObjBill * p_bobj = BillObj;
	uint   i;
	SString fmt_buf, msg_buf;
	PPIDArray out_bill_list;
	LocTransfDisposer disposer;
	LocTransfDisposeArray dispose_list_in, dispose_list_out;
	for(i = 0; i < rBillList.getCount(); i++) {
		const  PPID bill_id = rBillList.get(i);
		Transfer * p_tr = p_bobj->trfr;
		BillTbl::Rec bill_rec;
		TSVector <LocTransfTbl::Rec> disp_list;
		THROW(LtT.GetDisposition(bill_id, disp_list));
		if(p_bobj->Search(bill_id, &bill_rec) > 0) {
			PPTransferItem ti;
			for(int rbb = 0; p_tr->EnumItems(bill_id, &rbb, &ti) > 0;) {
				double disposed_qtty = 0.0;
				for(uint j = 0; j < disp_list.getCount(); j++) {
					const LocTransfTbl::Rec & r_lt_rec = disp_list.at(j);
					if(r_lt_rec.RByBillLT == rbb)
						disposed_qtty += fabs(r_lt_rec.Qtty);
				}
				double delta = fabs(ti.Qtty()) - disposed_qtty;
				if(delta > 0.0) {
					LocTransfDisposeItem dispose_item;
					if(ti.GetSign(bill_rec.OpID) == TISIGN_PLUS)
						dispose_item.Op = LOCTRFROP_PUT;
					else if(ti.GetSign(bill_rec.OpID) == TISIGN_MINUS)
						dispose_item.Op = LOCTRFROP_GET;
					if(dispose_item.Op) {
						dispose_item.GoodsID = labs(ti.GoodsID);
						dispose_item.WhLocID = bill_rec.LocID;
						dispose_item.BillID = bill_rec.ID;
						dispose_item.BillTiIdx = rbb;
						dispose_item.LotID = ti.LotID;
						dispose_item.Qtty = delta;
						THROW_SL(dispose_list_in.insert(&dispose_item));
					}
				}
			}
		}
	}
	if(dispose_list_in.getCount()) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(r = Dispose(dispose_list_in, dispose_list_out, 0));
		if(r > 0) {
			if(pLogger)
				PPLoadText(PPTXT_LOG_BILLDISPOSITION, fmt_buf);
			for(i = 0; i < dispose_list_out.getCount(); i++) {
				const LocTransfDisposeItem & r_disp_item = dispose_list_out.at(i);
				out_bill_list.addUnique(r_disp_item.BillID);
				if(pLogger && fmt_buf.NotEmpty()) {
					// PPTXT_LOG_BILLDISPOSITION "Товар '@goods' размещен в ячейке '@loc' количество =@real"
					pLogger->Log(PPFormat(fmt_buf, &msg_buf, r_disp_item.GoodsID, r_disp_item.LocID, r_disp_item.Qtty));
				}
			}
			for(i = 0; i < out_bill_list.getCount(); i++)
				DS.LogAction(PPACN_DISPOSEBILL, PPOBJ_BILL, out_bill_list.get(i), 0, 0);
			ok = 1;
		}
		THROW(tra.Commit());
	}
	if(pLogger) {
		PPLoadText(PPTXT_LOG_BILLDISPEMPTY, fmt_buf);
		for(i = 0; i < rBillList.getCount(); i++) {
			PPID bill_id = rBillList.get(i);
			if(!out_bill_list.lsearch(bill_id)) {
				// PPTXT_LOG_BILLDISPEMPTY   "Ни одна из строк документа '@bill' не была размещена по ячекам"
				pLogger->Log(PPFormat(fmt_buf, &msg_buf, bill_id));
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int LocTransfDisposer::Dispose(const LocTransfDisposeItem & rItem, LocTransfDisposeArray & rOutList, int use_ta)
{
	int    ok = 1;
	PPIDArray cell_list;
	PPIDArray temp_list;
	const PPIDArray * p_cell_list = 0;
	double req_qtty = rItem.Qtty;
	LocTransfOpBlock blk(LOCTRFRDOMAIN_WMS, rItem.Op, 0);
	PPTransaction tra(use_ta);
	THROW(tra);
	if(rItem.WhLocID) {
		temp_list.clear();
		temp_list.add(rItem.WhLocID);
		THROW(LocObj.ResolveWhCellList(&temp_list, 0, cell_list));
		p_cell_list = &cell_list;
	}
	if(rItem.Op == LOCTRFROP_PUT) {
		if(!(State & stGtoaLoaded)) {
			THROW(GtoAssc.Load());
			State |= stGtoaLoaded;
		}
		PPIDArray assc_loc_list;
		PPIDArray empty_cell_list;
		PPIDArray used_loc_list;
		GtoAssc.GetListByGoods(rItem.GoodsID, temp_list);
		if(temp_list.getCount()) {
			LocObj.ResolveWhCellList(&temp_list, 0, assc_loc_list);
			assc_loc_list = temp_list;
		}
		THROW(LtT.GetEmptyCellList(p_cell_list, &empty_cell_list));
		//
		// Сначала просматриваем АССОЦИИРОВАННЫЕ ПУСТЫЕ ячейки
		//
		{
			(temp_list = assc_loc_list).intersect(&empty_cell_list, 0);
			THROW(ArrangeCellList(rItem, temp_list));
			for(uint i = 0; req_qtty > 0.0 && i < temp_list.getCount(); i++) {
				const  PPID cell_id = temp_list.get(i);
				LocTransfDisposeItem item;
				item = rItem;
				item.Tag |= (item.ctInAssoc | item.ctEmpty);
				THROW(SetupOpBlock(item, cell_id, &req_qtty, blk));
				THROW(LtT.PutOp(blk, 0/*pRByLoc*/, 0/*pRByBill*/, 0));
				used_loc_list.add(cell_id);
				THROW_SL(rOutList.insert(&item));
			}
		}
		if(req_qtty > 0.0) {
			//
			// Теперь просматриваем НЕАССОЦИИРОВАННЫЕ ПУСТЫЕ ячейки
			//
			temp_list.clear();
			temp_list.addUniqueExclusive(&empty_cell_list, &used_loc_list);
			THROW(ArrangeCellList(rItem, temp_list));
			for(uint i = 0; req_qtty > 0.0 && i < temp_list.getCount(); i++) {
				const  PPID cell_id = temp_list.get(i);
				LocTransfDisposeItem item;
				item = rItem;
				item.Tag |= item.ctEmpty;
				if(assc_loc_list.getCount())
					item.Tag |= item.ctOutOfAssoc;
				THROW(SetupOpBlock(item, cell_id, &req_qtty, blk));
				THROW(LtT.PutOp(blk, 0/*pRByLoc*/, 0/*pRByBill*/, 0));
				used_loc_list.add(cell_id);
				THROW_SL(rOutList.insert(&item));
			}
		}
	}
	else if(rItem.Op == LOCTRFROP_GET) {
		RAssocArray cell_list_for_goods;
		THROW(LtT.GetCellListForGoods(rItem.GoodsID, p_cell_list, &cell_list_for_goods));
		temp_list.clear();
		THROW_SL(cell_list_for_goods.GetList(temp_list, 1));
		THROW(ArrangeCellList(rItem, temp_list));
		while(req_qtty > 0.0) {
			int    min_delta_idx = -1;
			double min_delta = SMathConst::Max;
			for(uint i = 0; i < temp_list.getCount(); i++) {
				const  PPID cell_id = temp_list.get(i);
				double rest = cell_list_for_goods.Get(cell_id);
				if(rest > 0.0) {
					const double delta = rest - req_qtty;
					if(delta < min_delta) {
						min_delta = delta;
						min_delta_idx = static_cast<int>(i);
					}
				}
			}
			if(min_delta_idx >= 0) {
				const  PPID cell_id = temp_list.get(min_delta_idx);
				const  double rest = cell_list_for_goods.Get(cell_id);
				LocTransfDisposeItem item;
				item = rItem;
				THROW(SetupOpBlock(item, cell_id, &req_qtty, blk));
				THROW(LtT.PutOp(blk, 0/*pRByLoc*/, 0/*pRByBill*/, 0));
				THROW_SL(rOutList.insert(&item));
				if(item.Qtty >= rest) {
					THROW_SL(cell_list_for_goods.Remove(cell_id));
					THROW_SL(temp_list.freeByKey(cell_id, 0));
				}
			}
			else
				break;
		}
	}
	THROW(tra.Commit());
	if(req_qtty > 0.0)
		ok = 2;
	CATCHZOK
	return ok;
}

int LocTransfDisposer::Dispose(const LocTransfDisposeArray & rInList, LocTransfDisposeArray & rOutList, int use_ta)
{
	int    ok = 1;
	PPTransaction tra(use_ta);
	THROW(tra);
	for(uint i = 0; i < rInList.getCount(); i++) {
		THROW(Dispose(rInList.at(i), rOutList, 0));
	}
	THROW(tra.Commit());
	CATCHZOK
	return ok;
}

struct ArrangeCellItem {
	PPID   CellID;
	PPID   ColumnID;
	int16  ColumnOrder;
	int16  Row;
	int16  Layer;
	int16  Depth;
	long   Counter;
};

struct ArrangeCellParam {
	int    Op;
};

IMPL_CMPFUNC(ArrangeCellItem, i1, i2)
{
	const ArrangeCellParam * p_param = static_cast<const ArrangeCellParam *>(pExtraData);
	const ArrangeCellItem * p1 = static_cast<const ArrangeCellItem *>(i1);
	const ArrangeCellItem * p2 = static_cast<const ArrangeCellItem *>(i2);
	const int op = p_param ? p_param->Op : 0; //LOCTRFROP_GET
	if(p1->ColumnOrder < p2->ColumnOrder)
		return -1;
	else if(p1->ColumnOrder > p2->ColumnOrder)
		return +1;
	else {
		if(p1->Row < p2->Row)
			return -1;
		else if(p1->Row > p2->Row)
			return +1;
		else {
			if(op == LOCTRFROP_PUT) {
				if(p1->Layer < p2->Layer)
					return -1;
				else if(p1->Layer > p2->Layer)
					return +1;
				else if(p1->Depth < p2->Depth) // @desc (Сначала заполняем более глубокие ячейки)
					return +1;
				else if(p1->Depth < p2->Depth)
					return -1;
			}
			else {
				if(p1->Layer < p2->Layer)
					return -1;
				if(p1->Layer > p2->Layer)
					return +1;
				else if(p1->Depth < p2->Depth) // @asc (Сначала берем из менее глубоких ячеек)
					return -1;
				else if(p1->Depth < p2->Depth)
					return +1;
			}
		}
		return CMPSIGN(p1->Counter, p2->Counter);
	}
}

int LocTransfDisposer::ArrangeColumnList(PPIDArray & rColumnList)
{
	int    ok = -1;
	return ok;
}

int LocTransfDisposer::ArrangeCellList(const LocTransfDisposeItem & rItem, PPIDArray & rLocList)
{
	int    ok = -1;
	uint   i;
	PPIDArray result;
	PPIDArray column_list;
	TSArray <ArrangeCellItem> list;
	for(i = 0; i < rLocList.getCount(); i++) {
		const  PPID cell_id = rLocList.get(i);
		LocationTbl::Rec loc_rec;
		if(LocObj.Fetch(cell_id, &loc_rec) > 0) {
			if(loc_rec.Type == LOCTYP_WHCELL) {
				ArrangeCellItem item;
				MEMSZERO(item);
				item.CellID = cell_id;
				item.ColumnID = loc_rec.ParentID;
				item.ColumnOrder = 0;
				item.Row = loc_rec.NumRows;
				item.Layer = loc_rec.NumLayers;
				item.Depth = loc_rec.Depth;
				item.Counter = loc_rec.Counter;
				THROW_SL(column_list.addUnique(loc_rec.ParentID));
				THROW_SL(list.insert(&item));
			}
		}
	}
	THROW(ArrangeColumnList(column_list));
	column_list.sort();
	for(i = 0; i < list.getCount(); i++) {
		ArrangeCellItem & r_item = list.at(i);
		if(r_item.ColumnID) {
			uint column_order = 0;
			if(column_list.bsearch(r_item.ColumnID, &column_order))
				r_item.ColumnOrder = column_order+1;
		}
	}
	{
		ArrangeCellParam param;
		param.Op = rItem.Op;
		list.sort(PTR_CMPFUNC(ArrangeCellItem), &param);
	}
	rLocList.clear();
	for(i = 0; i < list.getCount(); i++) {
		THROW_SL(rLocList.add(list.at(i).CellID));
	}
	ok = 1;
	CATCHZOK
	return ok;
}

int LocTransfDisposer::GetDistance(PPID loc1ID, PPID loc2ID, double * pDistance)
{
	ASSIGN_PTR(pDistance, 0.0);
	return -1;
}

int LocTransfDisposer::CheckLocRestictions(const LocTransfDisposeItem & rItem)
{
	return 1;
}
