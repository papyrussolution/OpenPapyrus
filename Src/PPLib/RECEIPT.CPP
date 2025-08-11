// RECEIPT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2022, 2023, 2024, 2025
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

int ReceiptCore::LotDimensions::EditTag(const PPGdsClsPacket * pGcPack, ObjTagItem * pItem)
{
	int    ok = -1;
	SString temp_buf;
	TDialog * dlg = new TDialog(DLG_TAGLOTDIM);
	if(CheckDialogPtrErr(&dlg)) {
		const PPGdsClsPacket * p_gc_pack = pGcPack;
		ReceiptCore::LotDimensions ld;
		pItem->GetStr(temp_buf);
		ld.FromString(temp_buf);
        dlg->setCtrlReal(CTL_TAGLOTDIM_X, ld.X);
        dlg->setCtrlReal(CTL_TAGLOTDIM_Y, ld.Y);
        dlg->setCtrlReal(CTL_TAGLOTDIM_Z, ld.Z);
		if(p_gc_pack) {
			enum {
				dfX = 0x0001,
				dfY = 0x0002,
				dfZ = 0x0004
			};
			long   disable_flags = 0;
			switch(p_gc_pack->Rec.LotDimCount) {
				case 0: disable_flags = dfX|dfY|dfZ; break;
				case 1: disable_flags = dfY|dfZ; break;
				case 2: disable_flags = dfZ; break;
			}
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_X, LOGIC(disable_flags & dfX));
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_Y, LOGIC(disable_flags & dfY));
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_Z, LOGIC(disable_flags & dfZ));
		}
        while(ok < 0 && ExecView(dlg) == cmOK) {
			ld.X = dlg->getCtrlReal(CTL_TAGLOTDIM_X);
			ld.Y = dlg->getCtrlReal(CTL_TAGLOTDIM_Y);
			ld.Z = dlg->getCtrlReal(CTL_TAGLOTDIM_Z);
            ld.ToStr(temp_buf);
            pItem->SetStr(pItem->TagID, temp_buf);
            ok = 1;
        }
	}
	delete dlg;
	return ok;
}

ReceiptCore::LotDimensions::LotDimensions() : X(0.0), Y(0.0), Z(0.0)
{
	MEMSZERO(Reserve);
}

void ReceiptCore::LotDimensions::Clear()
{
	X = 0.0;
	Y = 0.0;
	Z = 0.0;
	MEMSZERO(Reserve);
}

SString & FASTCALL ReceiptCore::LotDimensions::ToStr(SString & rBuf) const
{
	const long fmt = MKSFMTD(0, 6, NMBF_NOTRAILZ);
    return rBuf.Z().CatEq("x", X, fmt).Space().CatEq("y", Y, fmt).Space().CatEq("z", Z, fmt);
}

int FASTCALL ReceiptCore::LotDimensions::FromString(const char * pBuf)
{
	int   ok = 1;
	SString src_buf(pBuf);
	Clear();
	if(src_buf.NotEmptyS()) {
		SString key_buf;
		double value;
        SStrScan scan(src_buf);
        while(scan.Skip().GetEqN(key_buf, value)) {
			if(key_buf.IsEqiAscii("x"))
				X = value;
			else if(key_buf.IsEqiAscii("y"))
				Y = value;
			else if(key_buf.IsEqiAscii("z"))
				Z = value;
        }
	}
	return ok;
}

/*static*/SString & FASTCALL ReceiptCore::MakeCodeString(const ReceiptTbl::Rec * pRec, int options, SString & rBuf)
{
	// "@goods - @date - @int - @article"
	rBuf.Z();
	return PPFormatT(PPTXT_CODELOT, &rBuf, pRec->GoodsID, pRec->Dt, pRec->OprNo, pRec->SupplID);
}

ReceiptCore::ReceiptCore() : ReceiptTbl(), IgnoreGpretMostRecentFlags(-1)
{
}

int ReceiptCore::Search(PPID id, ReceiptTbl::Rec * pRec) { return SearchByID(this, PPOBJ_LOT, id, pRec); }
int ReceiptCore::Update(PPID id, ReceiptTbl::Rec * pRec, int use_ta) { return UpdateByID(this, PPOBJ_LOT, id, pRec, use_ta); }
int ReceiptCore::Add(PPID * pID, ReceiptTbl::Rec * pRec, int use_ta) { return AddByID(this, pID, pRec, use_ta); }

int ReceiptCore::Remove(PPID id, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(RemoveByID(this, id, 0));
		if(LConfig.DBDiv) {
			ObjSyncCore & r_sync = *DS.GetTLA().P_ObjSync;
			THROW(r_sync.RemoveByPrivateID(PPOBJ_LOT, id, 0));
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ReceiptCore::SearchLoop(PPID * pLotID, PPID locID)
{
	for(PPID id = *pLotID; id; id = data.PrevLotID) {
		if(Search(id, 0) <= 0)
			return 0;
		else if(data.LocID == locID)
			return ((*pLotID = id), 1);
	}
	return ((*pLotID = 0), -1);
}

int ReceiptCore::Helper_SearchOrigin(const ReceiptTbl::Rec * pInitLotRec, PPID lotID, PPID * pOrgLotID, ReceiptTbl::Rec * pThisRec, ReceiptTbl::Rec * pOrgRec)
{
	int    ok = 1;
	PPID   org_lot_id = pInitLotRec ? pInitLotRec->ID : 0;
	PPIDArray looked;
	SString msg_buf;
	ReceiptTbl::Rec lot_rec;
	for(PPID id = lotID; ok == 1 && id; id = lot_rec.PrevLotID) {
		if(Search(id, &lot_rec) > 0) {
			if(id == lotID)
				ASSIGN_PTR(pThisRec, lot_rec);
			//
			// Prevent loop
			//
			if(lot_rec.PrevLotID == lot_rec.ID) {
				org_lot_id = lot_rec.ID;
				PPSetError(PPERR_LOTLOOP, msg_buf.Cat(org_lot_id).CatChar('-').CatChar('>').Cat(id));
				lot_rec.PrevLotID = 0;
				ok = 2;
			}
			else if(looked.lsearch(id))
				ok = PPSetError(PPERR_LOTLOOP, msg_buf.Cat(org_lot_id).CatChar('-').CatChar('>').Cat(id));
			looked.add(id);
			org_lot_id = id;
		}
		else {
			const ReceiptTbl::Rec * p_init_lot_rec = (id == lotID) ? pInitLotRec : &lot_rec;
			msg_buf.Z();
			if(p_init_lot_rec)
				msg_buf.CatEq("goods_id", p_init_lot_rec->GoodsID).Space().CatEq("lot_id", p_init_lot_rec->ID);
			else
				msg_buf.Cat("???");
			msg_buf.Space().Cat("-->").Space().Cat(id);
			ok = PPSetError(PPERR_ORGLOTNFOUND, msg_buf);
		}
	}
	ASSIGN_PTR(pOrgRec, lot_rec);
	ASSIGN_PTR(pOrgLotID, org_lot_id);
	return ok;
}

int ReceiptCore::SearchOrigin(PPID lotID, PPID * pOrgLotID, ReceiptTbl::Rec * pThisRec, ReceiptTbl::Rec * pOrgRec)
{
	return Helper_SearchOrigin(0, lotID, pOrgLotID, pThisRec, pOrgRec);
}

int ReceiptCore::GetOriginDate(const ReceiptTbl::Rec * pLotRec, LDATE * pDate, PPID * pBillID)
{
	int    ok = 1;
	PPID   bill_id = 0;
	LDATE  lot_date = ZERODATE;
	if(pLotRec->PrevLotID) {
		ReceiptTbl::Rec org_lot_rec;
		if(Helper_SearchOrigin(pLotRec, pLotRec->PrevLotID, 0, 0, &org_lot_rec)) {
			lot_date = org_lot_rec.Dt;
			bill_id = org_lot_rec.BillID;
		}
		else
			ok = 0;
	}
	else {
		lot_date = pLotRec->Dt;
		bill_id = pLotRec->BillID;
	}
	ASSIGN_PTR(pDate, lot_date);
	ASSIGN_PTR(pBillID, bill_id);
	return ok;
}

int ReceiptCore::_SearchLot(int closed, PPID goodsID, PPID locID, LDATE dt, long oprno, int sp)
{
	ReceiptTbl::Key3 k;
	k.Closed   = closed;
	k.GoodsID  = goodsID;
	k.LocID    = locID;
	k.Dt       = dt;
	k.OprNo    = oprno;
	return search(3, &k, sp) ? 1 : PPDbSearchError();
}

int ReceiptCore::GetLastOpenedLot(PPID goodsID, const PPIDArray * pLocList, LDATE date, ReceiptTbl::Rec * pLotRec/*=0*/)
{
	int    ok = -1;
	const  LDATE _date = checkdate(date) ? date : encodedate(31, 12, 2999);
	ReceiptTbl::Rec last_lot_rec;
	ReceiptTbl::Key3 k;
	if(!pLocList) {
		MEMSZERO(k);
		k.GoodsID = goodsID;
		BExtQuery q(this, 3);
		DBQ * dbq = &(this->Closed == 0L && this->GoodsID == goodsID && this->Dt <= _date);
		q.selectAll().where(*dbq);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			if(data.Dt > last_lot_rec.Dt || (data.Dt == last_lot_rec.Dt && data.OprNo > last_lot_rec.OprNo)) {
				last_lot_rec = data;
				ok = 1;
			}
		}
	}
	else {
		const PPID single_loc_id = pLocList->getSingle();
		if(single_loc_id) {
			MEMSZERO(k);
			k.GoodsID  = goodsID;
			k.LocID    = single_loc_id;
			k.Dt       = _date;
			k.OprNo    = MAXLONG;
			int    r = search(3, &k, spLt) ? 1 : PPDbSearchError();
			if(r > 0 && data.Closed == 0 && data.GoodsID == goodsID && data.LocID == single_loc_id) {
				last_lot_rec = data;
				ok = 1;
			}
			else
				ok = r ? -1 : 0;
		}
		else {
			for(uint i = 0; i < pLocList->getCount(); i++) {
				const PPID loc_id = pLocList->get(i);
				MEMSZERO(k);
				k.GoodsID  = goodsID;
				k.LocID    = loc_id;
				k.Dt       = _date;
				k.OprNo    = MAXLONG;
				int    r = search(3, &k, spLt) ? 1 : PPDbSearchError();
				if(r > 0 && data.Closed == 0 && data.GoodsID == goodsID && data.LocID == loc_id) {
					if(data.Dt > last_lot_rec.Dt || (data.Dt == last_lot_rec.Dt && data.OprNo > last_lot_rec.OprNo)) {
						last_lot_rec = data;
						ok = 1;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pLotRec, last_lot_rec);
	return ok;
}

int ReceiptCore::GetCurrentGoodsPrice(PPID goodsID, PPID locID, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
	{ return Helper_GetCurrentGoodsPrice(goodsID, locID, MAXDATE, flags, pPrice, pRec); }
int ReceiptCore::GetGoodsPrice(PPID goodsID, PPID locID, LDATE date, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
	{ return Helper_GetCurrentGoodsPrice(goodsID, locID, date, flags, pPrice, pRec); }

int ReceiptCore::GetCurrentGoodsPrice_ByLocList(PPID goodsID, const PPIDArray * pLocList, uint flags, double * pPrice, ReceiptTbl::Rec * pRec/*=0*/) // @v12.3.10
	{ return Helper_GetCurrentGoodsPrice_ByLocList(goodsID, pLocList, MAXDATE, flags, pPrice, pRec); }

int ReceiptCore::Helper_GetCurrentGoodsPrice(PPID goodsID, PPID locID, LDATE date, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
{
	int    r  = GPRET_NOLOTS;
	PROFILE_START
	bool   use_mostrecent = LOGIC(flags & GPRET_MOSTRECENT);
	flags  &= ~GPRET_MOSTRECENT;
	LDATE  dt = date;
	long   o  = 0;
	ASSIGN_PTR(pPrice, 0.0);
	if(flags & GPRET_FORCELOTID) {
		if(pRec && pRec->ID) {
			ReceiptTbl::Rec lot_rec;
			if(Search(pRec->ID, &lot_rec) > 0) {
				if(labs(lot_rec.GoodsID) == labs(goodsID)) {
					if(!locID || lot_rec.LocID == locID || flags & GPRET_OTHERLOC) {
						if(lot_rec.Rest > 0.0 || flags & GPRET_CLOSEDLOTS) {
							ASSIGN_PTR(pPrice, R5(lot_rec.Price));
							ASSIGN_PTR(pRec, lot_rec);
							r = GPRET_FORCELOTID;
						}
					}
				}
			}
		}
	}
	if(r == GPRET_NOLOTS) {
		if(use_mostrecent) {
			if(IgnoreGpretMostRecentFlags < 0) {
				PPObjGoods goods_obj;
				IgnoreGpretMostRecentFlags = (goods_obj.GetConfig().Flags & GCF_RETAILPRICEBYMOSTRECENTLOT) ? 0 : 1;
			}
			if(IgnoreGpretMostRecentFlags > 0)
				use_mostrecent = false;
		}
		ReceiptTbl::Rec lot_rec;
		if(use_mostrecent && GetLastLot(goodsID, locID, dt, &lot_rec) > 0)
			r = lot_rec.Closed ? GPRET_CLOSEDLOTS : GPRET_PRESENT;
		else {
			PPIDArray loc_list;
			PPIDArray * p_loc_list = (loc_list.addnz(locID) > 0) ? &loc_list : 0;
			if((r = GetLastOpenedLot(goodsID, p_loc_list, dt, &lot_rec)) > 0)
				r = GPRET_PRESENT;
			else if(!flags || (flags & (GPRET_OTHERLOC | GPRET_CLOSEDLOTS))) {
				if((r = GetLastLot(goodsID, locID, MAXDATE, &lot_rec)) > 0) {
					if(locID == 0 && !lot_rec.Closed) {
						if(!flags || (flags & GPRET_OTHERLOC))
							r = GPRET_OTHERLOC;
					}
					else if(!flags || (flags & GPRET_CLOSEDLOTS))
						r = GPRET_CLOSEDLOTS;
				}
				else if(!flags || (flags & GPRET_OTHERLOC)) {
					if((r = GetLastLot(goodsID, 0L, MAXDATE, &lot_rec)) > 0)
						r = GPRET_OTHERLOC;
				}
			}
		}
		if(r > 0) {
			ASSIGN_PTR(pPrice, R5(lot_rec.Price));
			ASSIGN_PTR(pRec, lot_rec);
		}
		else {
			memzero(pRec, sizeof(*pRec));
			r = (r < 0) ? GPRET_NOLOTS : GPRET_ERROR;
		}
	}
	PROFILE_END
	return r;
}

int ReceiptCore::Helper_GetCurrentGoodsPrice_ByLocList(PPID goodsID, const PPIDArray * pLocList, LDATE date, uint flags, double * pPrice, ReceiptTbl::Rec * pRec) // @v12.3.10
{
	int    r  = GPRET_NOLOTS;
	PROFILE_START
	bool   use_mostrecent = LOGIC(flags & GPRET_MOSTRECENT);
	flags  &= ~GPRET_MOSTRECENT;
	LDATE  dt = date;
	long   o  = 0;
	ASSIGN_PTR(pPrice, 0.0);
	if(flags & GPRET_FORCELOTID) {
		if(pRec && pRec->ID) {
			ReceiptTbl::Rec lot_rec;
			if(Search(pRec->ID, &lot_rec) > 0) {
				if(labs(lot_rec.GoodsID) == labs(goodsID)) {
					if(!pLocList || pLocList->lsearch(lot_rec.LocID) || flags & GPRET_OTHERLOC) {
						if(lot_rec.Rest > 0.0 || flags & GPRET_CLOSEDLOTS) {
							ASSIGN_PTR(pPrice, R5(lot_rec.Price));
							ASSIGN_PTR(pRec, lot_rec);
							r = GPRET_FORCELOTID;
						}
					}
				}
			}
		}
	}
	if(r == GPRET_NOLOTS) {
		if(use_mostrecent) {
			if(IgnoreGpretMostRecentFlags < 0) {
				PPObjGoods goods_obj;
				IgnoreGpretMostRecentFlags = (goods_obj.GetConfig().Flags & GCF_RETAILPRICEBYMOSTRECENTLOT) ? 0 : 1;
			}
			if(IgnoreGpretMostRecentFlags > 0)
				use_mostrecent = false;
		}
		ReceiptTbl::Rec lot_rec;
		if(use_mostrecent && GetLastLot_ByLocList(goodsID, pLocList, dt, &lot_rec) > 0)
			r = lot_rec.Closed ? GPRET_CLOSEDLOTS : GPRET_PRESENT;
		else {
			if((r = GetLastOpenedLot(goodsID, pLocList, dt, &lot_rec)) > 0)
				r = GPRET_PRESENT;
			else if(!flags || (flags & (GPRET_OTHERLOC | GPRET_CLOSEDLOTS))) {
				if((r = GetLastLot_ByLocList(goodsID, pLocList, MAXDATE, &lot_rec)) > 0) {
					if(pLocList == 0 && !lot_rec.Closed) {
						if(!flags || (flags & GPRET_OTHERLOC))
							r = GPRET_OTHERLOC;
					}
					else if(!flags || (flags & GPRET_CLOSEDLOTS))
						r = GPRET_CLOSEDLOTS;
				}
				else if(!flags || (flags & GPRET_OTHERLOC)) {
					if((r = GetLastLot_ByLocList(goodsID, 0, MAXDATE, &lot_rec)) > 0)
						r = GPRET_OTHERLOC;
				}
			}
		}
		if(r > 0) {
			ASSIGN_PTR(pPrice, R5(lot_rec.Price));
			ASSIGN_PTR(pRec, lot_rec);
		}
		else {
			memzero(pRec, sizeof(*pRec));
			r = (r < 0) ? GPRET_NOLOTS : GPRET_ERROR;
		}
	}
	PROFILE_END
	return r;
}

int ReceiptCore::GetPreviousLot(PPID goodsID, PPID locID, LDATE dt, long oprNo, ReceiptTbl::Rec * pRec)
{
	int    ok = -1, r;
	ReceiptTbl::Key2 k2;
	k2.GoodsID = goodsID;
	k2.Dt = dt;
	k2.OprNo = oprNo;
	while(ok < 0 && (r = search(2, &k2, spLt)) != 0 && data.GoodsID == goodsID) {
		if(!locID || data.LocID == locID) {
			copyBufTo(pRec);
			ok = 1;
		}
	}
	if(!r)
		ok = BTROKORNFOUND ? -1 : PPSetErrorDB();
	return ok;
}

int ReceiptCore::GetReserve(PPID goodsID, PPID locID, double * pValue)
{
	PPID   goods_id = -labs(goodsID);
	double rest = 0.0;
	ReceiptTbl::Key3 k;
	MEMSZERO(k);
	k.GoodsID = goods_id;
	k.LocID   = locID;
	BExtQuery q(this, 3);
	DBQ * dbq = &(this->Closed == 0L && this->GoodsID == goods_id);
	dbq = ppcheckfiltid(dbq, this->LocID, locID);
	q.select(this->Flags, this->Rest, 0L).where(*dbq);
	for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;)
		if(data.Flags & LOTF_ORDRESERVE && !(data.Flags & LOTF_CLOSEDORDER))
			rest += data.Rest;
	rest = R6(rest);
	ASSIGN_PTR(pValue, rest);
	return (rest > 0) ? 1 : -1;
}

int ReceiptCore::EnumLastLots(PPID goodsID, PPID locID, LDATE * pDt, long * pOprNo, ReceiptTbl::Rec * pRec)
{
	ReceiptTbl::Key2 k, prev_k;
	k.GoodsID = goodsID;
	k.Dt      = *pDt;
	k.OprNo   = *pOprNo;
	for(prev_k = k; search(2, &k, spLt) && data.GoodsID == goodsID; prev_k = k) {
		if(memcmp(&k, &prev_k, sizeof(k)) == 0) {
			PPSetError(PPERR_PROBDBINDEXFAULT, GetFileName());
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR_TIME_USER);
			return 0;
		}
		else if(!locID || data.LocID == locID) {
			*pDt    = k.Dt;
			*pOprNo = k.OprNo;
			copyBufTo(pRec);
			return 1;
		}
	}
	return PPDbSearchError();
}

int ReceiptCore::Helper_GetLastLot_ByLocList(PPID goodsID, const PPIDArray * pLocList, LDATE dt, ReceiptTbl::Rec * pRec) // @v12.3.10
{
	int    ok = -1;
	ReceiptTbl::Rec last_rec;
	if(!pLocList || pLocList->getCount()) {
		ReceiptTbl::Key2 k;
		k.GoodsID = goodsID;
		k.Dt      = dt;
		k.OprNo   = MAXLONG;
		if(search(2, &k, spLt) && data.GoodsID == goodsID) {
			if(!pLocList || pLocList->lsearch(data.LocID)) {
				last_rec = data;
				ok = 1;
			}
			else {
				assert(last_rec.ID == 0); // @ctr
				assert(pLocList != 0); // В этом блоке точно (locID != 0)
				for(uint loc_idx = 0; loc_idx < pLocList->getCount(); loc_idx++) {
					const  PPID loc_id = pLocList->get(loc_idx);
					uint   r = 0;
					ReceiptTbl::Rec rec[2];
					ReceiptTbl::Key3 k3;
					{
						MEMSZERO(k3);
						k3.Closed   = 0;
						k3.GoodsID  = goodsID;
						k3.LocID    = loc_id;
						k3.Dt       = dt;
						k3.OprNo    = MAXLONG;
						if(search(3, &k3, spLt) && data.GoodsID == goodsID && data.LocID == loc_id) {
							copyBufTo(&rec[0]);
							r |= 1;
						}
					}
					{
						MEMSZERO(k3);
						k3.Closed   = 1;
						k3.GoodsID  = goodsID;
						k3.LocID    = loc_id;
						k3.Dt       = dt;
						k3.OprNo    = MAXLONG;
						if(search(3, &k3, spLt) && data.GoodsID == goodsID && data.LocID == loc_id) {
							copyBufTo(&rec[1]);
							r |= 2;
						}
					}
					if(r == (1|2)) {
						if(rec[0].Dt > rec[1].Dt)
							r = 1;
						else if(rec[0].Dt < rec[1].Dt)
							r = 2;
						else if(rec[0].OprNo > rec[1].OprNo)
							r = 1;
						else if(rec[0].OprNo < rec[1].OprNo)
							r = 2;
						else
							r = 1;
					}
					if(r == 1) {
						//
						// Второй вызов search переписал буфер данных таблицы в результате, вызывающая функция может использовать не верное значение.
						//
						if(rec[0].Dt > last_rec.Dt || (rec[0].Dt == last_rec.Dt && rec[0].OprNo > last_rec.OprNo)) {
							last_rec = rec[0];
						}
						//copyBufFrom(&rec[0]);
						//ASSIGN_PTR(pRec, rec[0]);
						ok = 1;
					}
					else if(r == 2) {
						if(rec[1].Dt > last_rec.Dt || (rec[1].Dt == last_rec.Dt && rec[1].OprNo > last_rec.OprNo)) {
							last_rec = rec[1];
						}
						//ASSIGN_PTR(pRec, rec[1]);
						ok = 1;
					}
				}
			}
		}
	}
	assert(ok <= 0 || last_rec.ID);
	ASSIGN_PTR(pRec, last_rec);
	return ok;
}

int ReceiptCore::Helper_GetLastLot(PPID goodsID, PPID locID, LDATE dt, ReceiptTbl::Rec * pRec)
{
	int    ok = -1;
	ReceiptTbl::Key2 k;
	k.GoodsID = goodsID;
	k.Dt      = dt;
	k.OprNo   = MAXLONG;
	//
	// Предпринимаем попытку сходу найти нужный лот.
	//
	if(search(2, &k, spLt) && data.GoodsID == goodsID) {
		if(!locID || data.LocID == locID) {
			copyBufTo(pRec);
			ok = 1;
		}
		else {
			assert(locID != 0); // В этом блоке точно (locID != 0)
			uint   r = 0;
			ReceiptTbl::Rec rec[2];
			ReceiptTbl::Key3 k3;
			{
				MEMSZERO(k3);
				k3.Closed   = 0;
				k3.GoodsID  = goodsID;
				k3.LocID    = locID;
				k3.Dt       = dt;
				k3.OprNo    = MAXLONG;
				if(search(3, &k3, spLt) && data.GoodsID == goodsID && data.LocID == locID) {
					copyBufTo(&rec[0]);
					r |= 1;
				}
			}
			{
				MEMSZERO(k3);
				k3.Closed   = 1;
				k3.GoodsID  = goodsID;
				k3.LocID    = locID;
				k3.Dt       = dt;
				k3.OprNo    = MAXLONG;
				if(search(3, &k3, spLt) && data.GoodsID == goodsID && data.LocID == locID) {
					copyBufTo(&rec[1]);
					r |= 2;
				}
			}
			if(r == (1|2)) {
				if(rec[0].Dt > rec[1].Dt)
					r = 1;
				else if(rec[0].Dt < rec[1].Dt)
					r = 2;
				else if(rec[0].OprNo > rec[1].OprNo)
					r = 1;
				else if(rec[0].OprNo < rec[1].OprNo)
					r = 2;
				else
					r = 1;
			}
			if(r == 1) {
				//
				// Второй вызов search переписал буфер данных таблицы в результате, вызывающая функция может использовать не верное значение.
				//
				copyBufFrom(&rec[0]);
				ASSIGN_PTR(pRec, rec[0]);
				ok = 1;
			}
			else if(r == 2) {
				ASSIGN_PTR(pRec, rec[1]);
				ok = 1;
			}
		}
	}
	return (BTROKORNFOUND) ? ok : PPSetErrorDB();
}

int ReceiptCore::GetLastLot(PPID goodsID, PPID locID, LDATE date, ReceiptTbl::Rec * pLotRec)
{
	int    ok = -1;
	PROFILE_START
	if(locID < 0) {
		const int r = Helper_GetLastLot(goodsID, labs(locID), date, pLotRec);
		if(r >= 0)
			ok = r;
		else
			ok = Helper_GetLastLot(goodsID, 0, date, pLotRec);
	}
	else {
		ok = Helper_GetLastLot(goodsID, locID, date, pLotRec);
	}
	PROFILE_END
	return ok;
}

int ReceiptCore::GetLastLot_ByLocList(PPID goodsID, const PPIDArray * pLocList, LDATE date, ReceiptTbl::Rec * pLotRec) // @v12.3.10
{
	int    ok = -1;
	if(!pLocList || pLocList->getCount()) {
		PROFILE_START
		ok = Helper_GetLastLot_ByLocList(goodsID, pLocList, date, pLotRec);
		PROFILE_END
	}
	return ok;
}

int ReceiptCore::GetFirstLot(PPID goodsID, PPID locID, ReceiptTbl::Rec * pLotRec)
{
	int    ok = -1;
	if(locID == 0) {
		// GoodsID, Dt, OprNo (unique mod);                  // #2
		ReceiptTbl::Key2 k2;
		MEMSZERO(k2);
		k2.GoodsID = goodsID;
		if(search(2, &k2, spGt) && data.GoodsID == goodsID) { // Лоты с нулевой датой не интересны - по 'тому spGt (не spGe)
			ASSIGN_PTR(pLotRec, data);
			ok = 1;
		}
		else if(pLotRec)
			memzero(pLotRec, sizeof(*pLotRec));
	}
	else {
		// Closed, GoodsID, LocID, Dt, OprNo (unique mod);   // #3
		ReceiptTbl::Rec rec_o;
		ReceiptTbl::Rec rec_c;
		int    s_o = 0;
		int    s_c = 0;
		ReceiptTbl::Key3 k3;
		MEMSZERO(k3);
		k3.Closed = 0;
		k3.GoodsID = goodsID;
		k3.LocID = locID;
		if(search(3, &k3, spGt) && data.Closed == 0 && data.GoodsID == goodsID && data.LocID == locID) { // Лоты с нулевой датой не интересны - по 'тому spGt (не spGe)
			s_o = 1;
			copyBufTo(&rec_o);
		}
		MEMSZERO(k3);
		k3.Closed = 1;
		k3.GoodsID = goodsID;
		k3.LocID = locID;
		if(search(0, &k3, spGt) && data.Closed == 1 && data.GoodsID == goodsID && data.LocID == locID) { // Лоты с нулевой датой не интересны - по 'тому spGt (не spGe)
			s_c = 1;
			copyBufTo(&rec_c);
		}
		if(s_o && s_c) {
			if(rec_o.Dt > rec_c.Dt || (rec_o.Dt == rec_c.Dt && rec_o.OprNo > rec_c.OprNo))
				s_o = 0;
			else
				s_c = 0;
			ok = 1;
		}
		else if(s_o || s_c)
			ok = 1;
		if(s_c) {
			ASSIGN_PTR(pLotRec, rec_c);
		}
		else if(s_o) {
			ASSIGN_PTR(pLotRec, rec_o);
		}
		else if(pLotRec)
			memzero(pLotRec, sizeof(*pLotRec));
	}
	return ok;
}

int ReceiptCore::EnumLots(PPID goods, PPID locID, DateIter * pIter, void * b)
{
	int    r = _SearchLot(0, goods, locID, pIter->dt, pIter->oprno, spGt);
	if(r > 0 && !data.Closed && data.GoodsID == goods && data.LocID == locID && pIter->Advance(data.Dt, data.OprNo) > 0) {
		copyBufTo(b);
		return 1;
	}
	else
		return r ? -1 : 0;
}

int ReceiptCore::EnumByGoods(PPID goodsID, DateIter * pIter, ReceiptTbl::Rec * pRec)
{
	ReceiptTbl::Key2 k2;
	k2.GoodsID = goodsID;
	k2.Dt = pIter ? pIter->dt : ZERODATE;
	k2.OprNo = pIter ? pIter->oprno : 0;
	if(search(2, &k2, spGt) && data.GoodsID == goodsID) {
		copyBufTo(pRec);
		return pIter ? pIter->Advance(k2.Dt, k2.OprNo) : 1;
	}
	else
		return PPDbSearchError();
}

IMPL_CMPFUNC(Receipt_DtOprNo_Asc, i1, i2) { RET_CMPCASCADE2(static_cast<const ReceiptTbl::Rec *>(i1), static_cast<const ReceiptTbl::Rec *>(i2), Dt, OprNo); }
IMPL_CMPFUNC(Receipt_DtOprNo_Desc, i1, i2) { RET_CMPCASCADE2(static_cast<const ReceiptTbl::Rec *>(i2), static_cast<const ReceiptTbl::Rec *>(i1), Dt, OprNo); }

IMPL_CMPFUNC(Receipt_DtOprNo_FEFO_Asc, i1, i2)
{
	int    si = 0;
	const  LDATE  expiry1 = NZOR(static_cast<const ReceiptTbl::Rec *>(i1)->Expiry, MAXDATE);
	const  LDATE  expiry2 = NZOR(static_cast<const ReceiptTbl::Rec *>(i2)->Expiry, MAXDATE);
	if(expiry1 == expiry2) {
		CMPCASCADE2(si, static_cast<const ReceiptTbl::Rec *>(i1), static_cast<const ReceiptTbl::Rec *>(i2), Dt, OprNo);
	}
	else
        si = cmp_ulong(expiry1, expiry2);
	return si;
}

IMPL_CMPFUNC(Receipt_DtOprNo_FEFO_Desc, i1, i2)
{
	int    si = 0;
	const  LDATE  expiry1 = NZOR(static_cast<const ReceiptTbl::Rec *>(i1)->Expiry, MAXDATE);
	const  LDATE  expiry2 = NZOR(static_cast<const ReceiptTbl::Rec *>(i2)->Expiry, MAXDATE);
	if(expiry1 == expiry2) {
		CMPCASCADE2(si, static_cast<const ReceiptTbl::Rec *>(i2), static_cast<const ReceiptTbl::Rec *>(i1), Dt, OprNo);
	}
	else
		si = cmp_ulong(expiry1, expiry2);
	return si;
}

int ReceiptCore::GetMostCriticalExpiryDate(PPID goodsID, PPID locID, PPID * pLotID, LDATE * pExpiryDate) // @v11.6.2 
{
	int    ok = -1;
	LDATE min_expiry = ZERODATE;
	PPID  min_expiry_lot_id = 0;
	LotArray lot_list;
	if(GetListOfOpenedLots(1, goodsID, locID, ZERODATE, &lot_list) > 0) {
		assert(lot_list.getCount());
		uint  min_expiry_idx = 0; // [1..] (0 - undef)
		for(uint i = 0; i < lot_list.getCount(); i++) {
			const ReceiptTbl::Rec & r_rec = lot_list.at(i);
			if(checkdate(r_rec.Expiry) && (!min_expiry || r_rec.Expiry < min_expiry)) {
				min_expiry = r_rec.Expiry;
				min_expiry_idx = i+1;
			}
		}
		if(min_expiry_idx) {
			assert(min_expiry_idx <= lot_list.getCount());
			assert(checkdate(min_expiry));
			min_expiry_lot_id = lot_list.at(min_expiry_idx-1).ID;
			ok = 1;
		}
	}
	else {
		assert(!lot_list.getCount());
	}
	ASSIGN_PTR(pLotID, min_expiry_lot_id);
	ASSIGN_PTR(pExpiryDate, min_expiry);
	return ok;
}

int ReceiptCore::GetListOfLotsExpiredSince(LDATE expiredSince, bool openedOnly, LDATE enumSince, PPIDArray * pLotIdList, PPIDArray * pGoodsIdList) // @v11.6.2
{
	CALLPTRMEMB(pLotIdList, clear());
	CALLPTRMEMB(pGoodsIdList, clear());
	int    ok = -1;
	if(checkdate(expiredSince)) {
		union {
			ReceiptTbl::Key1 k1;
			ReceiptTbl::Key3 k3;
		} k;
		int    idx = 0;
		DBQ  * dbq = 0;
		MEMSZERO(k);
		if(openedOnly) {
			idx = 3;
			k.k3.Closed = 0;
			dbq = &(*dbq && this->Closed == 0L && this->Expiry <= expiredSince && this->GoodsID > 0L);
		}
		else {
			idx = 1;
			k.k1.Dt = checkdate(enumSince) ? enumSince : ZERODATE;
			if(checkdate(enumSince))
				dbq = &(*dbq && this->Dt >= enumSince);
			dbq = &(*dbq && this->Expiry <= expiredSince && this->GoodsID > 0L);
		}
		BExtQuery q(this, idx);
		q.selectAll().where(*dbq);
		for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
			if(data.Expiry <= expiredSince && data.GoodsID > 0) {
				CALLPTRMEMB(pLotIdList, add(data.ID));
				CALLPTRMEMB(pGoodsIdList, add(data.GoodsID));
				ok = 1;
			}
		}
		CALLPTRMEMB(pGoodsIdList, sortAndUndup());
	}
	return ok;
}

int ReceiptCore::GetListOfOpenedLots(const PPIDArray & rGoodsList, PPID locID, LotArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	const uint _c = rGoodsList.getCount();
	if(_c) {
		if(_c == 1) {
			ok = GetListOfOpenedLots(1, rGoodsList.get(0), locID, ZERODATE, pList);
		}
		else {
			const  PPID first_goods_id = rGoodsList.get(0);
			const  PPID last_goods_id = rGoodsList.get(_c-1);
			ReceiptTbl::Key3 k;
			MEMSZERO(k);
			k.Closed   = 0;
			k.GoodsID  = first_goods_id;
			DBQ * dbq = 0;
			BExtQuery q(this, 3);
			dbq = &(this->Closed == 0L && this->GoodsID >= first_goods_id && this->GoodsID <= last_goods_id);
			if(locID)
                dbq = &(*dbq && this->LocID == locID);
			q.selectAll().where(*dbq);
			for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
				if(rGoodsList.bsearch(data.GoodsID)) {
					if(pList)
						THROW_SL(pList->insert(&data));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int ReceiptCore::GetListOfOpenedLots(int dir, PPID goodsID, PPID locID, LDATE beforeDt, LotArray * pList)
{
	const int use_bextq = 0;
	int    ok = -1;
	const PPConfig & r_cfg = LConfig;
	{
		PPUserFuncProfiler ufp(PPUPRF_GETOPENEDLOTS);
		uint   ufp_factor_i = 0;
		ReceiptTbl::Key3 k;
		MEMSZERO(k);
		if(dir < 0) {
			if(r_cfg.RealizeOrder == RLZORD_LIFO)
				dir = 0; // Enumerate lots backward
			else
				dir = 1; // Enumerate lots forward
		}
		k.Closed   = 0;
		k.GoodsID  = goodsID;
		k.LocID    = locID;
		if(use_bextq) {
			DBQ * dbq = 0;
			BExtQuery q(this, 3);
			dbq = &(this->Closed == 0L && this->GoodsID == goodsID && this->LocID == locID);
			if(beforeDt)
				dbq = &(*dbq && this->Dt <= beforeDt);
			q.selectAll().where(*dbq);
			for(q.initIteration(false, &k, spGt); q.nextIteration() > 0;) {
				ufp_factor_i++;
				if((goodsID >= 0) || !(data.Flags & LOTF_CLOSEDORDER)) {
					CALLPTRMEMB(pList, insert(&data));
					ok = 1;
				}
			}
		}
		else {
			if(search(3, &k, spGe) && !data.Closed && data.GoodsID == goodsID && data.LocID == locID && (!beforeDt || data.Dt <= beforeDt)) do {
				ufp_factor_i++;
				if((goodsID >= 0) || !(data.Flags & LOTF_CLOSEDORDER)) {
					CALLPTRMEMB(pList, insert(&data));
					ok = 1;
				}
			} while(search(3, &k, spNext) && !data.Closed && data.GoodsID == goodsID && data.LocID == locID && (!beforeDt || data.Dt <= beforeDt));
		}
		if(ok > 0 && pList) {
			if(r_cfg.Flags & CFGFLG_FEFO) {
				if(dir == 0) {
					pList->sort(PTR_CMPFUNC(Receipt_DtOprNo_FEFO_Desc));
				}
				else {
					pList->sort(PTR_CMPFUNC(Receipt_DtOprNo_FEFO_Asc));
				}
			}
			else if(dir == 0)
				pList->sort(PTR_CMPFUNC(Receipt_DtOprNo_Desc));
		}
		ufp.SetFactor(1, (double)ufp_factor_i);
		ufp.Commit();
	}
	return ok;
}

int ReceiptCore::EnumRefs(PPID prevLotID, LDATE * pDt, long * pOprNo, void * b)
{
	int    ok = -1;
	ReceiptTbl::Key4 k;
	k.PrevLotID = prevLotID;
	k.Dt      = *pDt;
	k.OprNo   = *pOprNo;
	if(search(4, &k, spGt) && data.PrevLotID == prevLotID) {
		*pDt = k.Dt;
		*pOprNo = k.OprNo;
		copyBufTo(b);
		ok = 1;
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int ReceiptCore::GatherChildren(PPID parentID, PPIDArray * pList, int (*test)(const ReceiptTbl::Rec *, void *), void * extraPtr)
{
	int    r = 1;
	PPID   id = 0; // @v12.2.4 @fix @initializing
	ReceiptTbl::Rec rr;
	if(parentID) {
		LDATE  dt = ZERODATE;
		long   oprno = 0;
		while((r = EnumRefs(parentID, &dt, &oprno, &rr)) > 0) {
			if(id == parentID) { // @v12.1.9
				// @todo Здесь что-то в лог надо вывести - тяжелая ошибка: лот ссылается сам на себя //
			}
			else {
				id = rr.ID;
				if((!test || test(&rr, extraPtr)) && pList)
					THROW(pList->add(id));
				THROW(GatherChildren(id, pList, test, extraPtr)); // @recursion
			}
		}
		THROW(r);
	}
	r = 1;
	CATCH
		r = 0;
	ENDCATCH
	return r;
}

int ReceiptCore::GetLastQCert(PPID goodsID, LDATE beforeDate, PPID locID, PPID * pQCertID, PPID * pLotID)
{
	int    ok = -1;
	ASSIGN_PTR(pQCertID, 0);
	ReceiptTbl::Key2 k;
	BExtQuery q(this, 2);
	DBQ * dbq = &(this->GoodsID == goodsID && this->QCertID > 0L);
	if(checkdate(beforeDate))
		dbq = &(*dbq && this->Dt <= beforeDate);
	dbq = ppcheckfiltid(dbq, this->LocID, locID); // @v11.9.10
	q.select(this->ID, this->Dt, this->QCertID, 0L).where(*dbq);
	k.GoodsID = goodsID;
	k.Dt = checkdate(beforeDate) ? beforeDate : MAXDATE;
	k.OprNo = MAXLONG;
	for(q.initIteration(true, &k, spLe); ok < 0 && q.nextIteration() > 0;) {
		if(data.QCertID) {
			ASSIGN_PTR(pQCertID, data.QCertID);
			ASSIGN_PTR(pLotID, data.ID);
			ok = 1;
		}
	}
	if(ok < 0 && goodsID > 0) {
		ok = GetLastQCert(-goodsID /*по лотам заказов*/, beforeDate, locID, pQCertID, pLotID); // @recursion
	}
	if(ok < 0 && locID) {
		ok = GetLastQCert(goodsID, beforeDate, 0 /*по любому складу*/, pQCertID, pLotID); // @recursion
	}
	return ok;
}

int ReceiptCore::IsThereOpenedLotForQCert(PPID qcertID)
{
	int    ok = -1;
	ReceiptTbl::Key6 k;
	MEMSZERO(k);
	k.QCertID = qcertID;
	BExtQuery q(this, 6, 1);
	q.select(this->ID, 0L).where(this->QCertID == qcertID && this->Rest > 0.0);
	q.initIteration(false, &k, spGe);
	if(q.nextIteration() > 0)
		ok = 1;
	return ok;
}

int ReceiptCore::GetSupplList(PPID goodsID, const PPIDArray * pLocList, LDATE lastDate, PPIDArray * pSupplList)
{
	int    ok = -1;
	LDATE  upp_date = NZOR(lastDate, getcurdate_());
	LDATE  first_date = ZERODATE;
	const  long   max_days = 90;
	const  long   min_lots = 3;
	long   count = 0;
	DBQ  * dbq = 0;
	BExtQuery q(this, 2, 128);
	ReceiptTbl::Key2 k;
	MEMSZERO(k);
	k.GoodsID = goodsID;
	k.Dt = upp_date;
	dbq = &(this->GoodsID == goodsID && this->Dt <= upp_date);
	dbq = ppcheckfiltidlist(dbq, this->LocID, pLocList);
	q.select(this->SupplID, this->Dt, 0L).where(*dbq);
	for(q.initIteration(true, &k, spLe); q.nextIteration() > 0;) {
		SETIFZ(first_date, data.Dt);
		CALLPTRMEMB(pSupplList, addUnique(data.SupplID));
		if(++count > min_lots && diffdate(first_date, data.Dt) > max_days)
			break;
	}
	CALLPTRMEMB(pSupplList, sort());
	ok = count ? 1 : -1;
	return ok;
}

int FASTCALL GetCurGoodsPrice(PPID goodsID, PPID locID, uint flags, double * pPrice, ReceiptTbl::Rec * pLotRec)
	{ return BillObj->trfr->Rcpt.GetCurrentGoodsPrice(goodsID, locID, flags, pPrice, pLotRec); }
//
//
//
LotExtCodeCore::LotExtCodeCore() : LotExtCodeTbl(0)
{
}

int LotExtCodeCore::RemoveAllByBill(PPID billID, int use_ta)
{
	int    ok = -1;
	LotExtCodeTbl::Key2 k2;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k2);
		k2.BillID = billID;
		if(search(2, &k2, spGe) && data.BillID == billID) do {
			THROW_DB(rereadForUpdate(2, &k2));
			THROW_DB(deleteRec());
			ok = 1;
		} while(search(2, &k2, spNext) && data.BillID == billID);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int LotExtCodeCore::PutContainer(PPID billID, const PPLotExtCodeContainer * pC, int use_ta)
{
	int    ok = -1;
	LongArray found_idx_list;
	PPLotExtCodeContainer::Item2 citem;
	LotExtCodeTbl::Key2 k2;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		MEMSZERO(k2);
		k2.BillID = billID;
		if(search(2, &k2, spGe) && data.BillID == billID) do {
			int    is_found = 0;
			for(uint i = 0; !is_found && i < pC->GetCount(); i++) {
				pC->GetByIdx(i, citem);
				if(citem.RowIdx == data.RByBill && citem.Flags == data.Flags && citem.BoxId == data.BoxNo && citem.Num == data.Code) {
					is_found = 1;
					found_idx_list.add(i+1);
				}
			}
			if(!is_found) {
				THROW_DB(rereadForUpdate(2, &k2));
				THROW_DB(deleteRec());
				ok = 1;
			}
		} while(search(2, &k2, spNext) && data.BillID == billID);
		{
			found_idx_list.sortAndUndup();
			if(found_idx_list.getCount() < pC->GetCount()) {
				BExtInsert bei(this);
				for(uint i = 0; i < pC->GetCount(); i++) {
					if(!found_idx_list.bsearch(i+1)) {
						pC->GetByIdx(i, citem);
						LotExtCodeTbl::Rec rec;
						rec.BillID = billID;
						rec.RByBill = citem.RowIdx;
						rec.Flags = citem.Flags;
						rec.BoxNo = citem.BoxId;
						STRNSCPY(rec.Code, citem.Num);
						THROW_DB(bei.insert(&rec));
						ok = 1;
					}
				}
				THROW_DB(bei.flash());
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int LotExtCodeCore::GetContainer(PPID billID, PPLotExtCodeContainer & rC)
{
	int    ok = -1;
	rC.Release();
	SString code_buf;
	LotExtCodeTbl::Rec rec;
	LotExtCodeTbl::Key2 k2;
	MEMSZERO(k2);
	k2.BillID = billID;
	BExtQuery q(this, 2);
	q.selectAll().where(this->BillID == billID);
	for(q.initIteration(false, &k2, spGe); q.nextIteration() > 0;) {
		copyBufTo(&rec);
		code_buf = rec.Code;
		if(rec.BillID == billID && code_buf.NotEmptyS()) {
			THROW(rC.Helper_Add(rec.RByBill, rec.BoxNo, rec.Flags, code_buf, 0, 0));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int LotExtCodeCore::GetMarkListByLot(PPID lotID, const StringSet * pExcludeList, StringSet & rSsExtCodes, uint * pExtCodeCount)
{
	rSsExtCodes.Z();
	int    ok = -1;
	uint   ext_code_count = 0;
	if(lotID) {
		LotExtCodeTbl::Key0 k0; 
		MEMSZERO(k0);
		k0.LotID = lotID;
		if(search(0, &k0, spGe) && data.LotID == lotID && data.BillID == 0) {
			ok = 1;
			SString temp_buf;
			do {
				temp_buf = data.Code;
				if(!pExcludeList || !pExcludeList->search(temp_buf, 0, 1)) {
					ext_code_count++;
					rSsExtCodes.add(temp_buf);
				}
			} while(search(0, &k0, spNext) && data.LotID == lotID && data.BillID == 0);
		}
	}
	ASSIGN_PTR(pExtCodeCount, ext_code_count);
	return ok;
}

int LotExtCodeCore::GetRecListByMark(const char * pCode, TSVector <LotExtCodeTbl::Rec> & rList)
{
	int    ok = -1;
	rList.clear();
	LotExtCodeTbl::Key1 k1;
	LotExtCodeTbl::Rec rec;
	MEMSZERO(k1);
	STRNSCPY(k1.Code, pCode);
	if(search(1, &k1, spGe) && stricmp(data.Code, pCode) == 0) {
		do {
			copyBufTo(&rec);
			THROW_SL(rList.insert(&rec));
			ok = 1;
		} while(search(1, &k1, spNext) && stricmp(data.Code, pCode) == 0);
	}
	CATCHZOK
	return ok;
}

int LotExtCodeCore::GetListByBillRow(PPID billID, int rbb, bool firstOnly, StringSet & rSs, uint * pCount)
{
	rSs.Z();
	int    ok = -1;
	uint   count = 0;
	LotExtCodeTbl::Key2 k2;
	MEMSZERO(k2);
	k2.BillID = billID;
	k2.RByBill = rbb;
	if(search(2, &k2, spGe) && data.BillID == billID && data.RByBill == rbb) do {
		if(data.Code[0] && !(data.Flags & PPLotExtCodeContainer::fBox)) {
			ok = 1;
			count++;
			rSs.add(data.Code);
			if(firstOnly)
				break;
		}
	} while(search(2, &k2, spNext) && data.BillID == billID && data.RByBill == rbb);
	ASSIGN_PTR(pCount, count);
	return ok;
}

int LotExtCodeCore::FindMarkToTransfer(const char * pCode, PPID goodsID, PPID lotID, PPLotExtCodeContainer::MarkSet & rResult)
{
	int    ok = -1;
	rResult.Z();
	if(!isempty(pCode)) {
		PPObjBill * p_bobj = BillObj;
		BillTbl::Rec bill_rec;
		TransferTbl::Rec trfr_rec;
		int    goods_detected = 0;
		int    lot_detected = 0;
		TSVector <LotExtCodeTbl::Rec> rec_list;
		int   parity = 0;
		PPID  first_bill_id = 0;
		int   first_rbb = 0;
		long  box_no = 0;
		if(GetRecListByMark(pCode, rec_list) > 0) {
			assert(rec_list.getCount());
			const int is_box = BIN(rec_list.at(0).Flags & PPLotExtCodeContainer::fBox);
			for(uint i = 0; i < rec_list.getCount(); i++) {
				const LotExtCodeTbl::Rec & r_rec = rec_list.at(i);
				if(r_rec.BillID && p_bobj->Fetch(r_rec.BillID, &bill_rec) > 0) {
					const  PPID op_type_id = GetOpType(bill_rec.OpID);
					if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND)) {
						if(r_rec.RByBill) {
							int16 row_idx = 0;
							int   rbb_target = 0;
							PPTransferItem ti_iter;
							for(int rbb_iter = 0; !rbb_target && p_bobj->trfr->EnumItems(r_rec.BillID, &rbb_iter, &ti_iter) > 0;) {
								row_idx++;
								if(row_idx == r_rec.RByBill)
									rbb_target = ti_iter.RByBill;
							}
							if(rbb_target && p_bobj->trfr->SearchByBill(r_rec.BillID, 0, rbb_target, &trfr_rec) > 0) {
								if(trfr_rec.Flags & PPTFR_RECEIPT) {
									if(lotID && trfr_rec.LotID == lotID)
										lot_detected = 1;
									if(goodsID && trfr_rec.GoodsID == goodsID)
										goods_detected = 1;
								}
								else if(trfr_rec.CorrLoc && p_bobj->trfr->SearchByBill(r_rec.BillID, 1, rbb_target, &trfr_rec) > 0) {
									if(lotID && trfr_rec.LotID == lotID)
										lot_detected = 1;
									if(goodsID && trfr_rec.GoodsID == goodsID)
										goods_detected = 1;
								}
								if(trfr_rec.Quantity > 0.0) {
									if(!first_bill_id) {
										first_bill_id = r_rec.BillID;
										first_rbb = r_rec.RByBill;
										box_no = r_rec.BoxNo;
									}
									parity++;
								}
								else if(trfr_rec.Quantity < 0.0)
									parity--;
							}
						}
					}
				}
			}
			THROW_PP_S(!goodsID || goods_detected, PPERR_EGAISMARKMISSGOODS, pCode);
			THROW_PP_S(!lotID || lot_detected, PPERR_EGAISMARKMISSLOT, pCode);
			THROW_PP_S(parity > 0, PPERR_EGAISMARKISEXHAUSTED, pCode);
			if(is_box) {
				LotExtCodeTbl::Key2 k2;
				MEMSZERO(k2);
				k2.BillID = first_bill_id;
				k2.RByBill = first_rbb;
				k2.BoxNo = box_no;
				int    box_count = 0;
				uint   mark_count = 0;
				if(search(2, &k2, spGe) && data.BillID == first_bill_id && data.RByBill == first_rbb && data.BoxNo == box_no) {
					do {
						if(data.Flags & PPLotExtCodeContainer::fBox) {
							THROW(rResult.AddBox(box_no, data.Code, 1));
							box_count++;
						}
						else {
							THROW(rResult.AddNum(box_no, data.Code, 1));
							mark_count++;
						}
					} while(search(2, &k2, spNext) && data.BillID == first_bill_id && data.RByBill == first_rbb && data.BoxNo == box_no);
				}
				THROW(box_count == 1);
				THROW(mark_count > 0);
			}
			else {
				THROW(rResult.AddNum(0, pCode, 1));
			}
			ok = 1;
		}
		else {
			CALLEXCEPT_PP_S(PPERR_EGAISMARKNOTFOUND, pCode);
		}
	}
	CATCHZOK
	return ok;
}
