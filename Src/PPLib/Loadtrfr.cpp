// LOADTRFR.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010, 2013, 2015, 2016, 2017, 2018, 2019, 2021
// @codepage UTF-8
// @Kernel
// Загрузка товарных строк документа
//
#include <pp.h>
#pragma hdrstop

int FASTCALL PPTransferItem::SetupByRec(const TransferTbl::Rec * tr)
{
	THISZERO();
	Date     = tr->Dt;
	BillID   = tr->BillID;
	RByBill  = tr->RByBill;
	LocID    = tr->LocID;
	GoodsID  = tr->GoodsID;
	LotID    = tr->LotID;
	CorrLoc  = tr->CorrLoc;
	Rest_    = tr->Rest;
	Flags    = tr->Flags;
	if(Flags & PPTFR_REVAL) {
		if(Flags & PPTFR_CORRECTION)
			Quantity_ = tr->Quantity;
		RevalCost = TR5(tr->Cost);
		Discount  = TR5(tr->Price);
	}
	else {
		Quantity_ = tr->Quantity;
		Cost     = TR5(tr->Cost);
		Price    = TR5(tr->Price);
		Discount = TR5(tr->Discount);
		QuotPrice = TR5(tr->QuotPrice);
	}
	WtQtty    = R6(tr->WtQtty);
	CurPrice  = TR5(tr->CurPrice);
	CurID     = static_cast<int16>(tr->CurID);
	//
	// Флаги PPTFR_PLUS и PPTFR_MINUS введены с v1.10.1
	// Для поддержки совместимости с документами, созданными
	// предыдущими версиями, при загрузки строк устанавливаем
	// эти флаги.
	//
	if(!(Flags & (PPTFR_PLUS|PPTFR_MINUS|PPTFR_REVAL))) {
		if(Quantity_ < 0.0)
			Flags |= PPTFR_MINUS;
		else if(Flags & PPTFR_RECEIPT)
			Flags |= PPTFR_PLUS;
		else if(Quantity_ > 0.0)
			Flags |= PPTFR_PLUS;
	}
	return 1;
}

int FASTCALL PPTransferItem::SetupByRec(const LocTransfTbl::Rec * pTr)
{
	THISZERO();
	Date     = pTr->Dt;
	LocTransfTm = pTr->Tm;
	BillID   = pTr->BillID;
	RByBill  = pTr->RByBill;
	LocID    = pTr->LocID;
	GoodsID  = pTr->GoodsID;
	LotID    = pTr->LotID;
	Flags    = pTr->Flags;
	Quantity_ = pTr->Qtty;
	TFlags  |= tfLocTransf;
	Flags &= ~(PPTFR_PLUS|PPTFR_MINUS);
	if(pTr->Op == LOCTRFROP_PUT)
		Flags |= PPTFR_PLUS;
	else if(pTr->Op == LOCTRFROP_GET)
		Flags |= PPTFR_MINUS;
	return 1;
}

int Transfer::SetupItemByLot(PPTransferItem * pItem, ReceiptTbl::Rec * pLotRec, int checkLotPrices, long oprno)
{
	if(!(pItem->Flags & PPTFR_UNLIM)) {
		pItem->UnitPerPack = pLotRec->UnitPerPack;
		pItem->Suppl       = pLotRec->SupplID;
		pItem->Expiry      = pLotRec->Expiry;
		pItem->LotTaxGrpID = pLotRec->InTaxGrpID;
		SETFLAG(pItem->Flags, PPTFR_COSTWOVAT,  pLotRec->Flags & LOTF_COSTWOVAT);
		SETFLAG(pItem->Flags, PPTFR_COSTWSTAX,  pLotRec->Flags & LOTF_COSTWSTAX);
		SETFLAG(pItem->TFlags, PPTransferItem::tfOrdReserve, pLotRec->Flags & LOTF_ORDRESERVE);
		if(pItem->TFlags & PPTransferItem::tfLocTransf) {
			pItem->Cost  = R5(pLotRec->Cost);
			pItem->Price = R5(pLotRec->Price);
			pItem->QCert = pLotRec->QCertID;
			pItem->ExtCost = pLotRec->ExtCost;
		}
		else {
			pItem->LotDate = pLotRec->Dt;
			if(pItem->Flags & PPTFR_REVAL) {
				if(checkLotPrices)
					if(!GetLotPrices(pLotRec, pItem->Date, oprno))
						return 0;
				pItem->Cost  = R5(pLotRec->Cost);
				pItem->Price = R5(pLotRec->Price);
				if(pItem->Flags & PPTFR_CORRECTION) {
					pItem->QuotPrice = pLotRec->Quantity;
					pItem->Quantity_ += pLotRec->Quantity;
					pItem->Flags &= ~(PPTFR_PLUS|PPTFR_MINUS);
				}
			}
			else {
				pItem->QCert = pLotRec->QCertID;
				pItem->ExtCost = pLotRec->ExtCost;
			}
		}
	}
	return 1;
}

int Transfer::EnumItems(PPID billID, int * pRByBill, PPTransferItem * pTI)
{
	int    ok = -1;
	TransferTbl::Key0 k0;
	k0.BillID  = billID;
	k0.Reverse = 0;
	k0.RByBill = *pRByBill;
	if(search(0, &k0, spGt) && k0.BillID == billID && k0.Reverse == 0) {
		*pRByBill = k0.RByBill;
		if(pTI) {
			pTI->SetupByRec(&data);
			if(pTI->LotID) {
				ReceiptTbl::Rec lot_rec, org_lot_rec;
				PPID   org_lot_id = 0;
				THROW(Rcpt.SearchOrigin(pTI->LotID, &org_lot_id, &lot_rec, &org_lot_rec) > 0);
				THROW_PP(pTI->LotID == lot_rec.ID, PPERR_INT_TRFRLOADINGLOT);
				THROW(SetupItemByLot(pTI, &lot_rec, 1, data.OprNo));
				pTI->LotDate = org_lot_rec.Dt;
			}
			else
				pTI->LotDate = pTI->Date;
		}
		ok = 1;
	}
	else
		THROW_DB(BTROKORNFOUND);
	CATCH
		//
		// В журнале pperrmsg.log уточняем природу ошибки
		//
		{
			SString msg_buf, bill_code;
			GetObjectName(PPOBJ_BILL, billID, bill_code);
			PPGetMessage(mfError, PPERR_F_TRANSFER_ENUMITEMS, bill_code, 1, msg_buf);
			PPLogMessage(PPFILNAM_ERRMSG_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_DBINFO);
		}
		ok = 0;
	ENDCATCH
	return ok;
}

int Transfer::GetOriginalValuesForCorrection(const PPTransferItem & rTi, const PPIDArray & rBillChain, long options, double * pOrgQtty, double * pOrgPrice)
{
	int    ok = -1;
	double org_qtty = 0.0;
	double org_price = 0.0;
	if(rTi.IsCorrectionExp()) {
		if(rBillChain.getCount()) {
			const PPID org_corr_bill_id = rBillChain.get(0);
			TransferTbl::Rec _rec;
			if(SearchByBill(org_corr_bill_id, 0, rTi.RByBill, &_rec) > 0) {
				if(!(options & govcoVerifyGoods) || _rec.GoodsID == rTi.GoodsID) {
					org_qtty = _rec.Quantity;
					org_price = _rec.Price - _rec.Discount;
					ok = 1;
				}
				for(uint j = 1; j < rBillChain.getCount(); j++) {
					const PPID chain_bill_id = rBillChain.get(j);
					if(SearchByBill(chain_bill_id, 0, rTi.RByBill, &_rec) > 0) {
						org_qtty += _rec.Quantity;
						org_price = _rec.Price - _rec.Discount;
						ok = 2;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pOrgQtty, org_qtty);
	ASSIGN_PTR(pOrgPrice, org_price);
	return ok;
}

int Transfer::LoadItems(PPBillPacket & rPack, const PPIDArray * pGoodsList)
{
	int    ok = 1;
	assert(!pGoodsList || pGoodsList->isSorted());
	if(!pGoodsList || pGoodsList->getCount()) {
		const  int is_debug = BIN(CConfig.Flags & CCFLG_DEBUG);
		BillTbl::Rec correction_org_bill_rec;
		PPIDArray correction_bill_chain;
		PROFILE_START;
		uint   i;
		PPTransferItem info, *p_ti;
		rPack.ProcessFlags &= ~PPBillPacket::pfErrOnTiLoading;
		correction_org_bill_rec.ID = 0;
		if(rPack.OpTypeID == PPOPT_CORRECTION) {
			BillObj->GetCorrectionBackChain(rPack.Rec.ID, correction_bill_chain);
		}
		{
			TransferTbl::Key0 k0;
			k0.BillID  = rPack.Rec.ID;
			k0.Reverse = 0;
			k0.RByBill = 0;
			BExtQuery q(this, 0, 128);
			DBQ * dbq = &(this->BillID == rPack.Rec.ID && this->Reverse == 0.0);
			if(pGoodsList) {
				if(pGoodsList->getCount() == 1) {
					dbq = &(*dbq && this->GoodsID == pGoodsList->get(0));
				}
				else if(pGoodsList->getCount() > 1) { // @paranoic (we have allready checked it above)
					dbq = &(*dbq && this->GoodsID >= pGoodsList->get(0) && this->GoodsID <= pGoodsList->getLast());
				}
			}
			q.select(RByBill, GoodsID, LotID, Rest, Flags, CorrLoc, Quantity, WtQtty, Cost,
				Price, Discount, QuotPrice, OprNo, CurID, CurPrice, 0L).where(*dbq);
			for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;) {
				if(!pGoodsList || pGoodsList->bsearch(data.GoodsID)) {
					//
					// Защита от недопустимых значений с плавающей точкой
					//
					if(is_debug) {
						int    fpok = 1;
						const  double big = 1.e9;
						SString fp_err_var;
#define _FZEROINV(v) if(!IsValidIEEE(v) || fabs(v) > big) { v = 0; fp_err_var.CatDivIfNotEmpty(';', 2).Cat(#v); fpok = 0; }
						_FZEROINV(data.Quantity);
						_FZEROINV(data.Rest);
						_FZEROINV(data.Cost);
						_FZEROINV(data.WtQtty);
						_FZEROINV(data.WtRest);
						_FZEROINV(data.Price);
						_FZEROINV(data.QuotPrice);
						_FZEROINV(data.Discount);
						_FZEROINV(data.CurPrice);
						if(!fpok) {
							PPSetError(PPERR_INVFPVAL, fp_err_var);
							PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
						}
						// @v9.5.1 if(data.CurID != rPack.Rec.CurID)
							data.CurID = rPack.Rec.CurID;
#undef _FZEROINV
					}
					info.SetupByRec(&data);
					if(info.Flags & PPTFR_REVAL)
						info.Suppl = data.OprNo; // Временно используем поле PPTransferItem::Suppl
					THROW(rPack.LoadTItem(&info, 0, 0));
				}
			}
			THROW_DB(BTROKORNFOUND);
		}
		for(i = 0; rPack.EnumTItems(&i, &p_ti);) {
			if(p_ti->IsCorrectionExp()) {
				double org_qtty = 0.0;
				double org_price = 0.0;
				if(GetOriginalValuesForCorrection(*p_ti, correction_bill_chain, 0, &org_qtty, &org_price) > 0) {
					p_ti->Quantity_ += org_qtty;
					p_ti->RevalCost = org_price;
					p_ti->QuotPrice = fabs(org_qtty);
				}
				else {
					// @todo Здесь надо как-то отреагировать (в лог что-то написать или что-то в этом роде)
				}
			}
			if(p_ti->LotID) {
				ReceiptTbl::Rec lot_rec, org_lot_rec;
				PPID   org_lot_id = 0;
				int    r = 1;
				//
				// Попытка "интеллигентно" обработать ошибку, возникающую в одной
				// из товарных строк документа, дабы документ все-таки был загружен.
				//
				if(Rcpt.SearchOrigin(p_ti->LotID, &org_lot_id, &lot_rec, &org_lot_rec) > 0) {
					if(p_ti->LotID != lot_rec.ID)
						r = PPSetError(PPERR_INT_TRFRLOADINGLOT);
					else {
						if(lot_rec.ID == lot_rec.PrevLotID)
							lot_rec.PrevLotID = 0;
						r = SetupItemByLot(p_ti, &lot_rec, 1, p_ti->Suppl);
					}
				}
				else
					r = 0;
				if(!r) {
					PPSaveErrContext();
					PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
					{
						SString fmt_buf, msg_buf, bill_buf, err_buf, goods_name;
						GetGoodsName(p_ti->GoodsID, goods_name);
						PPObjBill::MakeCodeString(&rPack.Rec, 1, bill_buf).CatDiv(';', 2).Cat(i).CatDiv(';', 2).Cat(goods_name);
						PPGetLastErrorMessage(1, err_buf);
						msg_buf.Printf(PPLoadTextS(PPTXT_TRFRLOADINGFAULT, fmt_buf), bill_buf.cptr(), err_buf.cptr());
						PPLogMessage(PPFILNAM_ERR_LOG, msg_buf, LOGMSGF_TIME|LOGMSGF_USER);
					}
					rPack.TiErrList.addUnique(i-1);
					rPack.ProcessFlags |= PPBillPacket::pfErrOnTiLoading;
					ok = 0;
				}
				p_ti->LotDate = org_lot_rec.Dt;
			}
			else
				p_ti->LotDate = p_ti->Date;
		}
		if(ok == 0)
			PPRestoreErrContext();
		PROFILE_END;
	}
	CATCHZOK
	return ok;
}

int Transfer::GetOrderFulfillmentStatus(PPID billID, int * pStatus)
{
	int    ok = -1;
	//   0 - документ не содержит ни одной строки заказа (возможно, это вообще не заказ и т.п.)
	//   1 - заказ полностью не выполнен
	//   2 - заказ полностью выполнен
	//   3 - заказ выполнен частично
	int    status = 0;
	if(billID) {
		PPIDArray ord_lot_list;
		{
			BExtQuery q(this, 0, 256);
			q.select(this->GoodsID, this->LotID, this->Flags, 0L).where(this->BillID == billID && Reverse == 0L);
			TransferTbl::Key0 k0;
			k0.BillID  = billID;
			k0.Reverse = 0;
			k0.RByBill = 0;
			for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;) {
				if(data.GoodsID < 0 && data.Flags & PPTFR_ORDER && data.LotID) {
					ord_lot_list.add(data.LotID);
				}
			}
		}
		const uint org_lot_list_count = ord_lot_list.getCount();
		if(org_lot_list_count) {
			ord_lot_list.sortAndUndup();
			assert(ord_lot_list.getCount() <= org_lot_list_count);
			if(ord_lot_list.getCount() < org_lot_list_count) {
				// Случилось нечто плохое: все идентификаторы лотов в документе заказа должны быть разными
				// а эта ситуация говорит об обратном. Я не знаю вероятна ли такая ситуация в существующих
				// данных, но на всякий случай делаю эту пометку.
				// ! Мы не будем здесь никак это обрабатывать и сделаем вид, что все в порядке.
			}
			bool is_there_completed = false;
			bool is_there_untouched = false;
			bool is_there_touched_and_uncompleted = false;
			bool is_there_anybody = false;
			ReceiptTbl::Rec lot_rec;
			for(uint i = 0; i < ord_lot_list.getCount(); i++) {
				double rest = 0.0;
				const PPID lot_id = ord_lot_list.get(i);
				if(Rcpt.Search(lot_id, &lot_rec) > 0 && lot_rec.Quantity > 0.0) {
					is_there_anybody = true;
					if(lot_rec.Rest >= lot_rec.Quantity)
						is_there_untouched = true;
					else if(lot_rec.Rest <= 0.0)
						is_there_completed = true;
					else if(lot_rec.Rest > 0.0 && lot_rec.Rest < lot_rec.Quantity)
						is_there_touched_and_uncompleted = true;
					else {
						assert(0); // unreachable
					}
				}
			}
			if(is_there_anybody) {
				if(is_there_untouched) {
					if(is_there_completed || is_there_touched_and_uncompleted)
						status = 3;
					else
						status = 1;
				}
				else {
					if(is_there_touched_and_uncompleted)
						status = 3;
					else
						status = 2;
				}
				ok = 1;
			}
			else {
				ok = -1;
			}
		}
	}
	ASSIGN_PTR(pStatus, status);
	return ok;
}

int Transfer::CalcBillTotal(PPID billID, BillTotal * pTotal, PPIDArray * pList)
{
	int    ok = 1;
	BExtQuery q(this, 0, 256);
	q.select(this->GoodsID, 0L).where(this->BillID == billID && Reverse == 0L);
	TransferTbl::Key0 k0;
	k0.BillID  = billID;
	k0.Reverse = 0;
	k0.RByBill = 0;
	for(q.initIteration(0, &k0, spGt); q.nextIteration() > 0;) {
		if(pTotal)
			pTotal->LineCount++;
		if(pList)
			THROW_SL(pList->add(data.GoodsID));
	}
	CATCHZOK
	return ok;
}
//
//
//
GoodsByTransferFilt::GoodsByTransferFilt(const GoodsFilt * pGoodsFilt) : SupplID(0), Flags(0)
{
	LotPeriod.Z();
	if(pGoodsFilt) {
		SupplID = pGoodsFilt->SupplID;
		LocList = pGoodsFilt->LocList;
		LotPeriod = pGoodsFilt->LotPeriod;
		Flags = pGoodsFilt->Flags;
	}
}

int GoodsByTransferFilt::IsEmpty() const
{
	return (SupplID || !LotPeriod.IsZero()) ? 0 : 1;
}

int Transfer::GetGoodsIdList(const GoodsByTransferFilt & rFilt, PPIDArray & rList)
{
	int    ok = 1, idx = 0;
	DBQ  * dbq = 0;
	rList.freeAll();
	if(!rFilt.IsEmpty()) {
		SString fmt_buf, msg_buf;
		if(!DS.IsThreadInteractive())
			PPLoadText(PPTXT_SELECTINGGOODSBYTRFR, fmt_buf);
		const int inc_nzero_rest = BIN(rFilt.Flags & GoodsFilt::fNoZeroRestOnLotPeriod);
		union {
			ReceiptTbl::Key1 k1;
			ReceiptTbl::Key5 k5;
		} k;
		MEMSZERO(k);
		if(rFilt.SupplID) {
			idx = 5;
			k.k5.SupplID = rFilt.SupplID;
		}
		else {
			idx = 1;
			k.k1.Dt = inc_nzero_rest ? ZERODATE : rFilt.LotPeriod.low;
		}
		BExtQuery q(&Rcpt, idx, 64);
		q.select(Rcpt.GoodsID, Rcpt.Dt, Rcpt.Closed, Rcpt.LocID, 0L);
		dbq = ppcheckfiltid(dbq, Rcpt.SupplID, rFilt.SupplID);
		if(!inc_nzero_rest)
			dbq = & (*dbq && daterange(Rcpt.Dt, &rFilt.LotPeriod));
		dbq = ppcheckfiltid(dbq, Rcpt.LocID, rFilt.LocList.GetSingle());
		if(!(rFilt.Flags & GoodsFilt::fIncludeIntr) && !inc_nzero_rest)
			dbq = & (*dbq && Rcpt.PrevLotID == 0L);
		dbq = & (*dbq && Rcpt.GoodsID > 0L);
		q.where(*dbq);
		for(q.initIteration(0, &k, spGe); q.nextIteration() > 0;) {
			if(!inc_nzero_rest || Rcpt.data.Dt >= rFilt.LotPeriod.low || !Rcpt.data.Closed) {
				if(rFilt.LocList.CheckID(Rcpt.data.LocID)) {
					THROW(rList.add(Rcpt.data.GoodsID));
					if(!DS.IsThreadInteractive()) {
						const uint _mc = rList.getCount();
                        if((_mc % 1000) == 0)
                            PPWaitMsg((msg_buf = fmt_buf).Space().Cat(_mc));
					}
				}
			}
		}
		rList.sortAndUndup();
		//
		// Если требуются только новые товары (лотов по которым не было до периода поступления //
		// то по каждому товару из списка просматриваем лоты за период 0..pFilt->LotPeriod.low-1
		// и удаляет из списка товары, для которых найден хотя бы один лот по такому условию.
		//
		if(!inc_nzero_rest && rFilt.Flags & GoodsFilt::fNewLots && rFilt.LotPeriod.low) {
			for(int i = rList.getCount()-1; i >= 0; i--) {
				DateIter di(0, plusdate(rFilt.LotPeriod.low, -1));
				ReceiptTbl::Rec lot_rec;
				while(Rcpt.EnumByGoods(rList.at(i), &di, &lot_rec) > 0) {
					if(rFilt.LocList.CheckID(lot_rec.LocID)) {
						rList.atFree(i);
						break;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
