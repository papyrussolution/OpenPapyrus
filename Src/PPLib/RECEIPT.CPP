// RECEIPT.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2013, 2015, 2016, 2017, 2018, 2019
// @Kernel
//
#include <pp.h>
#pragma hdrstop

int SLAPI ReceiptCore::LotDimensions::EditTag(const PPGdsClsPacket * pGcPack, ObjTagItem * pItem)
{
	int    ok = -1;
	TDialog * dlg = 0;
	SString temp_buf;
	if(CheckDialogPtrErr(&(dlg = new TDialog(DLG_TAGLOTDIM)))) {
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
			if(p_gc_pack->Rec.LotDimCount == 0)
				disable_flags = dfX|dfY|dfZ;
			else if(p_gc_pack->Rec.LotDimCount == 1)
				disable_flags = dfY|dfZ;
			else if(p_gc_pack->Rec.LotDimCount == 2)
				disable_flags = dfZ;
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_X, (disable_flags & dfX));
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_Y, (disable_flags & dfY));
			dlg->setCtrlReadOnly(CTL_TAGLOTDIM_Z, (disable_flags & dfZ));
		}
        while(ok < 0 && ExecView(dlg) == cmOK) {
			ld.X = dlg->getCtrlReal(CTL_TAGLOTDIM_X);
			ld.Y = dlg->getCtrlReal(CTL_TAGLOTDIM_Y);
			ld.Z = dlg->getCtrlReal(CTL_TAGLOTDIM_Z);
            ld.ToString(temp_buf);
            pItem->SetStr(pItem->TagID, temp_buf);
            ok = 1;
        }
	}
	delete dlg;
	return ok;
}

SLAPI ReceiptCore::LotDimensions::LotDimensions() : X(0.0), Y(0.0), Z(0.0)
{
	MEMSZERO(Reserve);
}

void SLAPI ReceiptCore::LotDimensions::Clear()
{
	X = 0.0;
	Y = 0.0;
	Z = 0.0;
	MEMSZERO(Reserve);
}

SString & FASTCALL ReceiptCore::LotDimensions::ToString(SString & rBuf) const
{
	const long fmt = MKSFMTD(0, 6, NMBF_NOTRAILZ);
    return rBuf.Z().CatEq("x", X, fmt).Space().CatEq("y", Y, fmt).Space().CatEq("z", Z, fmt);
}

int FASTCALL ReceiptCore::LotDimensions::FromString(const char * pBuf)
{
	int   ok = 1;
	SString src_buf = pBuf;
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

//static
SString & FASTCALL ReceiptCore::MakeCodeString(const ReceiptTbl::Rec * pRec, int options, SString & rBuf)
{
	// "@goods - @date - @int - @article"
	rBuf.Z();
	return PPFormatT(PPTXT_CODELOT, &rBuf, pRec->GoodsID, pRec->Dt, pRec->OprNo, pRec->SupplID);
}

SLAPI ReceiptCore::ReceiptCore() : ReceiptTbl(), IgnoreGpretMostRecentFlags(-1)
{
}

int SLAPI ReceiptCore::Search(PPID id, ReceiptTbl::Rec * pRec)
	{ return SearchByID(this, PPOBJ_LOT, id, pRec); }
int SLAPI ReceiptCore::Update(PPID id, ReceiptTbl::Rec * pRec, int use_ta)
	{ return UpdateByID(this, PPOBJ_LOT, id, pRec, use_ta); }
int SLAPI ReceiptCore::Add(PPID * pID, ReceiptTbl::Rec * pRec, int use_ta)
	{ return AddByID(this, pID, pRec, use_ta); }

int SLAPI ReceiptCore::Remove(PPID id, int use_ta)
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

int SLAPI ReceiptCore::SearchLoop(PPID * pLotID, PPID locID)
{
	for(PPID id = *pLotID; id; id = data.PrevLotID) {
		if(Search(id, 0) <= 0)
			return 0;
		else if(data.LocID == locID)
			return ((*pLotID = id), 1);
	}
	return ((*pLotID = 0), -1);
}

int SLAPI ReceiptCore::SearchOrigin(PPID lotID, PPID * pOrgLotID, ReceiptTbl::Rec * pThisRec, ReceiptTbl::Rec * pOrgRec)
{
	int    ok = 1;
	PPID   org_lot_id = 0;
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
		else
			ok = 0;
	}
	ASSIGN_PTR(pOrgRec, lot_rec);
	ASSIGN_PTR(pOrgLotID, org_lot_id);
	return ok;
}

int SLAPI ReceiptCore::GetOriginDate(const ReceiptTbl::Rec * pLotRec, LDATE * pDate, PPID * pBillID)
{
	int    ok = 1;
	PPID   bill_id = 0;
	LDATE  lot_date = ZERODATE;
	if(pLotRec->PrevLotID) {
		ReceiptTbl::Rec org_lot_rec;
		if(SearchOrigin(pLotRec->PrevLotID, 0, 0, &org_lot_rec)) {
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

int SLAPI ReceiptCore::_SearchLot(int closed, PPID goodsID, PPID locID, LDATE dt, long oprno, int sp)
{
	ReceiptTbl::Key3 k;
	k.Closed   = closed;
	k.GoodsID  = goodsID;
	k.LocID    = locID;
	k.Dt       = dt;
	k.OprNo    = oprno;
	return search(3, &k, sp) ? 1 : PPDbSearchError();
}

int SLAPI ReceiptCore::GetLastOpenedLot(PPID goodsID, PPID locID, LDATE date, void * b)
{
	int    r = _SearchLot(0, goodsID, locID, NZOR(date, encodedate(31, 12, 2999)), MAXLONG, spLt);
	if(r > 0 && data.Closed == 0 && data.GoodsID == goodsID && data.LocID == locID) {
		copyBufTo(b);
		return 1;
	}
	else
		return r ? -1 : 0;
}

int SLAPI ReceiptCore::GetCurrentGoodsPrice(PPID goodsID, PPID locID, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
	{ return Helper_GetCurrentGoodsPrice(goodsID, locID, MAXDATE, flags, pPrice, pRec); }
int SLAPI ReceiptCore::GetGoodsPrice(PPID goodsID, PPID locID, LDATE date, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
	{ return Helper_GetCurrentGoodsPrice(goodsID, locID, date, flags, pPrice, pRec); }

int SLAPI ReceiptCore::Helper_GetCurrentGoodsPrice(PPID goodsID, PPID locID, LDATE date, uint flags, double * pPrice, ReceiptTbl::Rec * pRec)
{
	int    r  = GPRET_NOLOTS;
	PROFILE_START
	int    use_mostrecent = BIN(flags & GPRET_MOSTRECENT);
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
				use_mostrecent = 0;
		}
		ReceiptTbl::Rec lot_rec;
		if(use_mostrecent && GetLastLot(goodsID, locID, dt, &lot_rec) > 0)
			r = lot_rec.Closed ? GPRET_CLOSEDLOTS : GPRET_PRESENT;
		else if((r = GetLastOpenedLot(goodsID, locID, dt, &lot_rec)) > 0)
			r = GPRET_PRESENT;
		else if(!flags || (flags & (GPRET_OTHERLOC | GPRET_CLOSEDLOTS)))
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

int SLAPI ReceiptCore::GetPreviousLot(PPID goodsID, PPID locID, LDATE dt, long oprNo, ReceiptTbl::Rec * pRec)
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

int SLAPI ReceiptCore::GetReserve(PPID goodsID, PPID locID, double * pValue)
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
	for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;)
		if(data.Flags & LOTF_ORDRESERVE && !(data.Flags & LOTF_CLOSEDORDER))
			rest += data.Rest;
	rest = R6(rest);
	ASSIGN_PTR(pValue, rest);
	return (rest > 0) ? 1 : -1;
}

int SLAPI ReceiptCore::EnumLastLots(PPID goodsID, PPID locID, LDATE * pDt, long * pOprNo, ReceiptTbl::Rec * pRec)
{
	ReceiptTbl::Key2 k, prev_k;
	k.GoodsID = goodsID;
	k.Dt      = *pDt;
	k.OprNo   = *pOprNo;
	for(prev_k = k; search(2, &k, spLt) && data.GoodsID == goodsID; prev_k = k) {
		if(memcmp(&k, &prev_k, sizeof(k)) == 0) {
			PPSetError(PPERR_PROBDBINDEXFAULT, GetFileName());
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR | LOGMSGF_TIME | LOGMSGF_USER);
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

int SLAPI ReceiptCore::Helper_GetLastLot(PPID goodsID, PPID locID, LDATE dt, ReceiptTbl::Rec * pRec)
{
	int    ok = -1;
	ReceiptTbl::Key2 k;
	k.GoodsID = goodsID;
	k.Dt      = dt;
	k.OprNo   = MAXLONG;
	//
	// ѕредпринимаем попытку сходу найти нужный лот.
	//
	if(search(2, &k, spLt) && data.GoodsID == goodsID) {
		if(!locID || data.LocID == locID) {
			copyBufTo(pRec);
			ok = 1;
		}
		else {
			//
			// ¬ этом блоке точно (locID != 0)
			//

			//
			// @v6.3.14
			// ѕохоже, следующий участок кода все-таки дает значительную задержку при
			// большом количестве лотов.
			// ¬ св€зи с этим замен€ем его на блок ниже.
			//
#if 0 // {
			{
				//
				// ѕопытка быстро найти лот, соответствующий нашим требовани€м провалилась.
				// “ак как лоты по товару, соответствующие заданному складу могут быть разбросаны
				// очень хаотично, то отдадим серверу задачу по нахождению требуемой записи.
				// ќн с этим справитс€ намного быстрее чем мы, перебирающие запись за записью.
				//
				// ќбращаю внимание на параметр reverse = 1 в функции initIteration: нам надо
				// перебирать лоты в пор€дке, обратном к индексации.
				//
				BExtQuery q(this, 2, 1);
				q.selectAll().where(this->GoodsID == goodsID && this->Dt <= dt && this->LocID == locID);
				if(q.initIteration(1, 0, -1) && q.nextIteration() > 0) {
					copyBufTo(pRec);
					ok = 1;
				}
			}
#else // }{
			{
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
					// ¬торой вызов search переписал буфер данных таблицы
					// в результате, вызывающа€ функци€ может использовать не верное значение.
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
#endif // }
		}
	}
	return (BTROKORNFOUND) ? ok : PPSetErrorDB();
}

int SLAPI ReceiptCore::GetLastLot(PPID goodsID, PPID locID, LDATE date, ReceiptTbl::Rec * pLotRec)
{
	int    ok = -1;
	PROFILE_START
	if(locID < 0) {
		int    r = Helper_GetLastLot(goodsID, labs(locID), date, pLotRec);
		if(r >= 0)
			ok = r;
		else
			ok = Helper_GetLastLot(goodsID, 0, date, pLotRec);
	}
	else
		ok = Helper_GetLastLot(goodsID, locID, date, pLotRec);
	PROFILE_END
	return ok;
}

int SLAPI ReceiptCore::GetFirstLot(PPID goodsID, PPID locID, ReceiptTbl::Rec * pLotRec)
{
	int    ok = -1;
	if(locID == 0) {
		// GoodsID, Dt, OprNo (unique mod);                  // #2
		ReceiptTbl::Key2 k2;
		MEMSZERO(k2);
		k2.GoodsID = goodsID;
		if(search(2, &k2, spGt) && data.GoodsID == goodsID) { // Ћоты с нулевой датой не интересны - по 'тому spGt (не spGe)
			ASSIGN_PTR(pLotRec, data);
			ok = 1;
		}
		else if(pLotRec)
			memzero(pLotRec, sizeof(*pLotRec));
	}
	else {
		// Closed, GoodsID, LocID, Dt, OprNo (unique mod);   // #3
		ReceiptTbl::Rec rec_o, rec_c;
		int    s_o = 0, s_c = 0;
		ReceiptTbl::Key3 k3;
		MEMSZERO(k3);
		k3.Closed = 0;
		k3.GoodsID = goodsID;
		k3.LocID = locID;
		if(search(3, &k3, spGt) && data.Closed == 0 && data.GoodsID == goodsID && data.LocID == locID) { // Ћоты с нулевой датой не интересны - по 'тому spGt (не spGe)
			s_o = 1;
			copyBufTo(&rec_o);
		}
		MEMSZERO(k3);
		k3.Closed = 1;
		k3.GoodsID = goodsID;
		k3.LocID = locID;
		if(search(0, &k3, spGt) && data.Closed == 1 && data.GoodsID == goodsID && data.LocID == locID) { // Ћоты с нулевой датой не интересны - по 'тому spGt (не spGe)
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

int SLAPI ReceiptCore::EnumLots(PPID goods, PPID loc, DateIter * iter, void * b)
{
	int    r = _SearchLot(0, goods, loc, iter->dt, iter->oprno, spGt);
	if(r > 0 && !data.Closed && data.GoodsID == goods && data.LocID == loc && iter->Advance(data.Dt, data.OprNo) > 0) {
		copyBufTo(b);
		return 1;
	}
	return r ? -1 : 0;
}

int SLAPI ReceiptCore::EnumByGoods(PPID goodsID, DateIter * pIter, ReceiptTbl::Rec * pRec)
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

IMPL_CMPFUNC(Receipt_DtOprNo_Asc, i1, i2)  { RET_CMPCASCADE2((const ReceiptTbl::Rec *)i1, (const ReceiptTbl::Rec *)i2, Dt, OprNo); }
IMPL_CMPFUNC(Receipt_DtOprNo_Desc, i1, i2) { RET_CMPCASCADE2((const ReceiptTbl::Rec *)i2, (const ReceiptTbl::Rec *)i1, Dt, OprNo); }

IMPL_CMPFUNC(Receipt_DtOprNo_FEFO_Asc, i1, i2)
{
	int    si = 0;
	const  LDATE  expiry1 = NZOR(((ReceiptTbl::Rec *)i1)->Expiry, MAXDATE);
	const  LDATE  expiry2 = NZOR(((ReceiptTbl::Rec *)i2)->Expiry, MAXDATE);
	if(expiry1 == expiry2) {
		CMPCASCADE2(si, (const ReceiptTbl::Rec *)i1, (const ReceiptTbl::Rec *)i2, Dt, OprNo);
	}
	else
        si = cmp_ulong(expiry1, expiry2);
	return si;
}

IMPL_CMPFUNC(Receipt_DtOprNo_FEFO_Desc, i1, i2)
{
	int    si = 0;
	const  LDATE  expiry1 = NZOR(((ReceiptTbl::Rec *)i1)->Expiry, MAXDATE);
	const  LDATE  expiry2 = NZOR(((ReceiptTbl::Rec *)i2)->Expiry, MAXDATE);
	if(expiry1 == expiry2) {
		CMPCASCADE2(si, (const ReceiptTbl::Rec *)i2, (const ReceiptTbl::Rec *)i1, Dt, OprNo);
	}
	else
		si = cmp_ulong(expiry1, expiry2);
	return si;
}

int SLAPI ReceiptCore::GetListOfOpenedLots(const PPIDArray & rGoodsList, PPID locID, LotArray * pList)
{
	int    ok = -1;
	CALLPTRMEMB(pList, clear());
	const uint _c = rGoodsList.getCount();
	if(_c) {
		if(_c == 1) {
			ok = GetListOfOpenedLots(1, rGoodsList.get(0), locID, ZERODATE, pList);
		}
		else {
			const PPID first_goods_id = rGoodsList.get(0);
			const PPID last_goods_id = rGoodsList.get(_c-1);
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
			for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
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

int SLAPI ReceiptCore::GetListOfOpenedLots(int dir, PPID goodsID, PPID locID, LDATE beforeDt, LotArray * pList)
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
			for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
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

int SLAPI ReceiptCore::EnumRefs(PPID prevLotID, LDATE * pDt, long * pOprNo, void * b)
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

int SLAPI ReceiptCore::GatherChilds(PPID parent, PPIDArray * pList, int (*test)(ReceiptTbl::Rec *, void *), void * extraPtr)
{
	int    r;
	PPID   id;
	LDATE  dt;
	long   oprno;
	ReceiptTbl::Rec rr;
	if(parent) {
		for(oprno = dt.v = 0; (r = EnumRefs(parent, &dt, &oprno, &rr)) > 0;) {
			id = rr.ID;
			if((!test || test(&rr, extraPtr)) && pList)
				THROW(pList->add(id));
			THROW(GatherChilds(id, pList, test, extraPtr)); // @recursion
		}
		THROW(r);
	}
	r = 1;
	CATCH
		r = 0;
	ENDCATCH
	return r;
}

int SLAPI ReceiptCore::GetLastQCert(PPID goodsID, PPID * pQCertID)
{
	ASSIGN_PTR(pQCertID, 0);

	ReceiptTbl::Key2 k;
	BExtQuery q(this, 2);
	q.select(this->ID, this->QCertID, 0L).where(this->GoodsID == goodsID && this->QCertID > 0L);
	k.GoodsID = goodsID;
	k.Dt = MAXDATE;
	k.OprNo = MAXLONG;
	for(q.initIteration(1, &k, spLt); q.nextIteration() > 0;) {
		if(data.QCertID) {
			ASSIGN_PTR(pQCertID, data.QCertID);
			return 1;
		}
	}
	return (goodsID > 0) ? GetLastQCert(-goodsID, pQCertID) : -1;
}

int SLAPI ReceiptCore::IsThereOpenedLotForQCert(PPID qcertID)
{
	int    ok = -1;
	ReceiptTbl::Key6 k;
	MEMSZERO(k);
	k.QCertID = qcertID;
	BExtQuery q(this, 6, 1);
	q.select(this->ID, 0L).where(this->QCertID == qcertID && this->Rest > 0.0);
	q.initIteration(0, &k, spGe);
	if(q.nextIteration() > 0)
		ok = 1;
	return ok;
}

int SLAPI ReceiptCore::GetSupplList(PPID goodsID, const PPIDArray * pLocList, LDATE lastDate, PPIDArray * pSupplList)
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
	for(q.initIteration(1, &k, spLe); q.nextIteration() > 0;) {
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
SLAPI LotExtCodeCore::LotExtCodeCore() : LotExtCodeTbl(0)
{
}

int SLAPI LotExtCodeCore::RemoveAllByBill(PPID billID, int use_ta)
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

int SLAPI LotExtCodeCore::PutContainer(PPID billID, PPLotExtCodeContainer * pC, int use_ta)
{
	int    ok = -1;
	LongArray found_idx_list;
	//SString code_buf;
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
				//Item & r_item = *(Item *)at(i);
				// @v10.2.9 if(r_item.RowIdx == pTbl->data.RByBill && r_item.Sign == pTbl->data.Sign) {
				if(citem.RowIdx == data.RByBill && citem.Flags == data.Flags && citem.BoxId == data.BoxNo && citem.Num == data.Code) { // @v10.2.9
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
						//Item & r_item = *(Item *)at(i);
						//GetS(r_item.CodeP, code_buf);
						LotExtCodeTbl::Rec rec;
						MEMSZERO(rec);
						rec.BillID = billID;
						rec.RByBill = citem.RowIdx;
						// @v10.2.9 rec.Sign = r_item.Sign;
						rec.Flags = citem.Flags; // @v10.2.9
						rec.BoxNo = citem.BoxId; // @v10.2.9
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

int SLAPI LotExtCodeCore::GetContainer(PPID billID, PPLotExtCodeContainer & rC)
{
	int    ok = -1;
	rC.Release();
	SString code_buf;
	LotExtCodeTbl::Key2 k2;
	MEMSZERO(k2);
	k2.BillID = billID;
	BExtQuery q(this, 2);
	q.selectAll().where(this->BillID == billID);
	for(q.initIteration(0, &k2, spGe); q.nextIteration() > 0;) {
		LotExtCodeTbl::Rec rec;
		copyBufTo(&rec);
		code_buf = rec.Code;
		if(rec.BillID == billID && code_buf.NotEmptyS()) {
			THROW(rC.Add(rec.RByBill, rec.BoxNo, rec.Flags, code_buf, 0));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI LotExtCodeCore::GetMarkListByLot(PPID lotID, const StringSet * pExcludeList, StringSet & rSsExtCodes, uint * pExtCodeCount)
{
	rSsExtCodes.clear();
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

int SLAPI LotExtCodeCore::FindMarkToTransfer(const char * pCode, PPID goodsID, PPID lotID, PPLotExtCodeContainer::MarkSet & rResult)
{
	int    ok = -1;
	rResult.Clear();
	if(!isempty(pCode)) {
		PPObjBill * p_bobj = BillObj;
		BillTbl::Rec bill_rec;
		TransferTbl::Rec trfr_rec;
		LotExtCodeTbl::Key1 k1;
		int    is_box = 0;
		int    goods_detected = 0;
		int    lot_detected = 0;
		MEMSZERO(k1);
		STRNSCPY(k1.Code, pCode);
		if(search(1, &k1, spGe) && stricmp(data.Code, pCode) == 0) {
			int   parity = 0;
			PPID  first_bill_id = 0;
			int   first_rbb = 0;
			long  box_no = 0;
			if(data.Flags & PPLotExtCodeContainer::fBox)
				is_box = 1;
			do {
				if(data.BillID && p_bobj->Fetch(data.BillID, &bill_rec) > 0) {
					const PPID op_type_id = GetOpType(bill_rec.OpID);
					if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND)) {
						if(data.RByBill && p_bobj->trfr->SearchByBill(data.BillID, 0, data.RByBill, &trfr_rec) > 0) {
							if(trfr_rec.Flags & PPTFR_RECEIPT) {
								if(lotID && trfr_rec.LotID == lotID)
									lot_detected = 1;
								if(goodsID && trfr_rec.GoodsID == goodsID)
									goods_detected = 1;
							}
							if(trfr_rec.Quantity > 0.0) {
								if(!first_bill_id) {
									first_bill_id = data.BillID;
									first_rbb = data.RByBill;
									box_no = data.BoxNo;
								}
								parity++;
							}
							else if(trfr_rec.Quantity < 0.0)
								parity--;
						}
					}
				}
			} while(search(1, &k1, spNext) && stricmp(data.Code, pCode) == 0);
			THROW_PP_S(!goodsID || goods_detected, PPERR_EGAISMARKMISSGOODS, pCode);
			THROW_PP_S(!lotID || lot_detected, PPERR_EGAISMARKMISSLOT, pCode);
			THROW_PP_S(parity >= 0, PPERR_EGAISMARKISEXHAUSTED, pCode);
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
							THROW(rResult.AddBox(box_no, data.Code));
							box_count++;
						}
						else {
							THROW(rResult.AddNum(box_no, data.Code));
							mark_count++;
						}
					} while(search(2, &k2, spNext) && data.BillID == first_bill_id && data.RByBill == first_rbb && data.BoxNo == box_no);
				}
				THROW(box_count == 1);
				THROW(mark_count > 0);
			}
			else {
				THROW(rResult.AddNum(0, data.Code));
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