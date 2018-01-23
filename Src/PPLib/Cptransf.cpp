// CPTRANSF.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage UTF-8
// @Kernel
//
#include <pp.h>
#pragma hdrstop

SLAPI CpTransfCore::CpTransfCore() : CpTransfTbl()
{
}

// static
int FASTCALL CpTransfCore::PutExt__(CpTransfTbl::Rec & rRec, const CpTrfrExt * pExt)
{
	int    ok = 1;
	SString ext_buf, temp_buf;
    PPPutExtStrData(CPTFREXSTR_SERIAL, ext_buf, (temp_buf = pExt ? pExt->PartNo : 0).Strip());
    PPPutExtStrData(CPTFREXSTR_CLB, ext_buf, (temp_buf = pExt ? pExt->Clb : 0).Strip());
    STRNSCPY(rRec.Tail, ext_buf);
	return ok;
}

// static
int FASTCALL CpTransfCore::GetExt__(CpTransfTbl::Rec & rRec, CpTrfrExt * pExt)
{
	int    ok = 1;
	SString ext_buf, temp_buf;
	ext_buf = rRec.Tail;
	if(pExt) {
		pExt->PartNo[0] = 0;
		pExt->Clb[0] = 0;
	}
	PPGetExtStrData(CPTFREXSTR_SERIAL, ext_buf, temp_buf.Z());
    if(pExt) {
		STRNSCPY(pExt->PartNo, temp_buf);
    }
	PPGetExtStrData(CPTFREXSTR_CLB, ext_buf, temp_buf.Z());
    if(pExt) {
		STRNSCPY(pExt->Clb, temp_buf);
    }
    return ok;
}

int SLAPI CpTransfCore::Search(PPID billID, int rByBill, CpTransfTbl::Rec * pRec)
{
	CpTransfTbl::Key0 k;
	k.BillID  = billID;
	k.RByBill = rByBill;
	return SearchByKey(this, 0, &k, pRec);
}

int SLAPI CpTransfCore::LoadItems(PPID billID, PPBillPacket * pPack, const PPIDArray * pGoodsList)
{
	int    ok = 1;
	assert(!pGoodsList || pGoodsList->isSorted());
	if(!pGoodsList || pGoodsList->getCount()) {
		PROFILE_START
		CpTransfTbl::Key0 k;
		k.BillID = billID;
		k.RByBill = -MAXSHORT;
		BExtQuery q(this, 0, 128);
		DBQ * dbq = &(this->BillID == billID);
		if(pGoodsList) {
			if(pGoodsList->getCount() == 1) {
				dbq = &(*dbq && this->GoodsID == pGoodsList->get(0));
			}
			else if(pGoodsList->getCount() > 1) { // @paranoic (we have allready checked it above)
				dbq = &(*dbq && this->GoodsID >= pGoodsList->get(0) && this->GoodsID <= pGoodsList->getLast());
			}
		}
		q.select(this->BillID, this->RByBill, this->LocID, this->GoodsID, this->OrdLotID,
			this->CurID, this->UnitPerPack, this->Qtty, this->Cost, this->Price, this->Discount,
			this->CurPrice, this->Expiry, this->QCertID, this->InTaxGrpID, this->Flags, this->Tail, 0L).where(*dbq);
		for(q.initIteration(0, &k, spGt); q.nextIteration() > 0;) {
			if(!pGoodsList || pGoodsList->bsearch(data.GoodsID)) {
				PPTransferItem ti;
				ti.Date     = pPack->Rec.Dt;
				ti.BillID   = data.BillID;
				ti.RByBill  = data.RByBill;
				ti.LocID    = data.LocID;
				ti.GoodsID  = data.GoodsID;
				ti.OrdLotID = data.OrdLotID;
				ti.CurID    = (int16)data.CurID;
				ti.UnitPerPack = data.UnitPerPack;
				ti.Quantity_ = data.Qtty;
				ti.Cost     = data.Cost;
				ti.Price    = data.Price;
				ti.Discount = data.Discount;
				ti.CurPrice = data.CurPrice;
				ti.Expiry   = data.Expiry;
				ti.QCert    = data.QCertID;
				ti.LotTaxGrpID = data.InTaxGrpID;
				ti.Flags    = data.Flags;
				{
					CpTrfrExt cpext;
					MEMSZERO(cpext);
					CpTransfCore::GetExt__(data, &cpext);
					THROW(pPack->LoadTItem(&ti, cpext.Clb, cpext.PartNo));
				}
			}
		}
		PROFILE_END
	}
	CATCHZOK
	return ok;
}

int SLAPI CpTransfCore::EnumItems(PPID billID, int * pRByBill, PPTransferItem * pTi, CpTrfrExt * pExt)
{
	int    ok = 1;
	CpTransfTbl::Key0 k;
	k.BillID = billID;
	k.RByBill = *pRByBill;
	if(search(0, &k, spGt) && k.BillID == billID) {
		*pRByBill = k.RByBill;
		if(pTi) {
			pTi->Init(0);
			pTi->BillID   = data.BillID;
			pTi->RByBill  = data.RByBill;
			pTi->LocID    = data.LocID;
			pTi->GoodsID  = data.GoodsID;
			pTi->OrdLotID = data.OrdLotID;
			pTi->CurID    = (int16)data.CurID;
			pTi->UnitPerPack = data.UnitPerPack;
			pTi->Quantity_ = data.Qtty;
			pTi->Cost     = data.Cost;
			pTi->Price    = data.Price;
			pTi->Discount = data.Discount;
			pTi->CurPrice = data.CurPrice;
			pTi->Expiry   = data.Expiry;
			pTi->QCert    = data.QCertID;
			pTi->LotTaxGrpID = data.InTaxGrpID;
			pTi->Flags    = data.Flags;
		}
		if(pExt) {
			CpTransfCore::GetExt__(data, pExt);
		}
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI CpTransfCore::PutItem(PPTransferItem * pTi, int16 forceRByBill, const CpTrfrExt * pExt, int use_ta)
{
	int    ok = 1;
	int    is_new = 0;
	CpTransfTbl::Rec rec;
	CpTransfTbl::Key0 k;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pTi->RByBill == 0 || Search(pTi->BillID, pTi->RByBill, &rec) <= 0) {
			is_new = 1;
			if(forceRByBill > 0) {
				pTi->RByBill = forceRByBill;
			}
			else {
				MEMSZERO(rec);
				k.BillID = pTi->BillID;
				k.RByBill = MAXSHORT;
				pTi->RByBill = (search(0, &k, spLt) && k.BillID == pTi->BillID) ? (k.RByBill+1) : 1;
			}
		}
		rec.BillID   = pTi->BillID;
		rec.RByBill  = pTi->RByBill;
		rec.GoodsID  = pTi->GoodsID;
		rec.LocID    = pTi->LocID;
		rec.OrdLotID = pTi->OrdLotID;
		rec.CurID    = pTi->CurID;
		rec.UnitPerPack = pTi->UnitPerPack;
		rec.Qtty     = pTi->Quantity_;
		rec.Cost     = pTi->Cost;
		rec.Price    = pTi->Price;
		rec.Discount   = pTi->Discount;
		rec.CurPrice   = pTi->CurPrice;
		rec.Expiry     = pTi->Expiry;
		rec.QCertID    = pTi->QCert;
		rec.InTaxGrpID = pTi->LotTaxGrpID;
		rec.Flags      = pTi->Flags;
		CpTransfCore::PutExt__(rec, pExt); // @v8.9.10
		/* @v8.9.10
		if(pExt) {
			STRNSCPY(rec.PartNo, pExt->PartNo);
			STRNSCPY(rec.Clb,    pExt->Clb);
		}
		*/
		if(is_new) {
			THROW_DB(insertRecBuf(&rec));
		}
		else {
			k.BillID  = pTi->BillID;
			k.RByBill = pTi->RByBill;
			if(searchForUpdate(0, &k, spEq)) {
				THROW_DB(updateRecBuf(&rec)); // @sfu
			}
			else {
				THROW_DB(insertRecBuf(&rec));
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI CpTransfCore::RemoveItem(PPID billID, int rByBill, int use_ta)
{
	int    ok = 1;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		CpTransfTbl::Key0 k;
		k.BillID  = billID;
		k.RByBill = rByBill;
		if(searchForUpdate(0, &k, spEq)) {
			THROW_DB(deleteRec()); // @sfu
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI CpTransfCore::SearchGoodsRef(PPID goodsID, CpTransfTbl::Rec * pRec)
{
	CpTransfTbl::Key1 k1;
	MEMSZERO(k1);
	k1.GoodsID = goodsID;
	return (search(1, &k1, spGe) && k1.GoodsID == goodsID) ? 1 : PPDbSearchError();
}

int SLAPI CpTransfCore::ReplaceGoods(PPID destGoodsID, PPID srcGoodsID, int use_ta)
{
	int    ok = 1;
	CpTransfTbl::Key1 k1;
	MEMSZERO(k1);
	k1.GoodsID = destGoodsID;
	int    sp = spGe;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		// @todo update_for
		while(searchForUpdate(1, &k1, sp) && k1.GoodsID == destGoodsID) {
			data.GoodsID = srcGoodsID;
			THROW_DB(updateRec()); // @sfu
			sp = spGe;
		}
		THROW_DB(BTROKORNFOUND);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
int SLAPI PPObjBill::InitDraftWrOffPacket(const PPDraftOpEx * pWrOffParam, const BillTbl::Rec * pDraftRec, PPBillPacket * pPack, int use_ta)
{
	int    ok = 1;
	PPOprKind wroff_op_rec;
	PPBillExt bill_ext;
	ArticleTbl::Rec ar_rec;
	THROW(pPack->CreateBlank_WithoutCode(pWrOffParam->WrOffOpID, 0, pDraftRec->LocID, use_ta)); // @v8.6.10 0-->use_ta // @v8.8.12 CreateBlank-->CreateBlank_WithoutCode
	THROW(GetOpData(pWrOffParam->WrOffOpID, &wroff_op_rec) > 0);
	STRNSCPY(pPack->Rec.Code, pDraftRec->Code);
	pPack->Rec.Dt = (pWrOffParam->Flags & DROXF_WROFFCURDATE) ? getcurdate_() : pDraftRec->Dt;
	if(P_Tbl->GetExtraData(pDraftRec->ID, &bill_ext) > 0)
		pPack->Ext.AgentID = bill_ext.AgentID;
	{
		PPID   ar_id = 0;
		if(pWrOffParam->WrOffObjID)
			ar_id = pWrOffParam->WrOffObjID;
		else if(ArObj.Fetch(pDraftRec->Object, &ar_rec) > 0 && ar_rec.AccSheetID == wroff_op_rec.AccSheetID)
			ar_id = pDraftRec->Object;
		if(ar_id) {
			PPBillPacket::SetupObjectBlock sob; // @v9.0.0
			// @v9.0.0 pPack->Rec.Object = ar_id;
			THROW(pPack->SetupObject(ar_id, sob)); // @v9.0.0
			if(sob.State & sob.stHasCliAgreement) {
				if(pPack->Rec.Flags & BILLF_NEEDPAYMENT) {
					pPack->SetupDefaultPayDate(sob.CliAgt.DefPayPeriod, sob.CliAgt.PaymDateBase);
				}
			}
			else if(sob.State & sob.stHasSupplAgreement) {
				if(pPack->Rec.Flags & BILLF_NEEDPAYMENT) {
					pPack->SetupDefaultPayDate(sob.SupplAgt.DefPayPeriod, 0);
				}
			}
		}
	}
	if(ArObj.Fetch(pDraftRec->Object2, &ar_rec) > 0 && ar_rec.AccSheetID == wroff_op_rec.AccSheet2ID)
		pPack->Rec.Object2 = pDraftRec->Object2;
	pPack->Rec.LocID = pDraftRec->LocID;
	pPack->Rec.CurID = pDraftRec->CurID;
	if(pPack->Rec.CurID) {
		LDATE  temp_dt = pPack->Rec.Dt;
		double rate = 1.0;
		GetCurRate(pPack->Rec.CurID, &temp_dt, &rate);
		pPack->Rec.CRate = rate;
	}
	pPack->Rec.LinkBillID = pDraftRec->ID;
	STRNSCPY(pPack->Rec.Memo, pDraftRec->Memo);
	if(pDraftRec->Flags & BILLF_FREIGHT && CheckOpFlags(pPack->Rec.OpID, OPKF_FREIGHT)) {
		PPFreight freight;
		if(P_Tbl->GetFreight(pDraftRec->ID, &freight) > 0) {
			freight.ID = 0;
			pPack->SetFreight(&freight);
		}
	}
#if 0 // @v9.0.0 { Функционал этого блока обслуживается вызовом pPack->SetupObject(pPack->Rec.Object, sob) выше
	//
	// Устанавливаем значения, согласно клиентским соглашениям
	//
	{
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		PPClientAgreement ca_rec;
		int    is_acc_sheet = BIN(acs_obj.Fetch(pPack->AccSheet, &acs_rec) > 0);
		if(is_acc_sheet) {
			if(((acs_rec.Flags & ACSHF_USECLIAGT) || pPack->AccSheet == GetSellAccSheet())) {
				if(ArObj.GetClientAgreement(pPack->Rec.Object, &ca_rec, 1) > 0) {
					if(!(ca_rec.Flags & AGTF_DEFAULT))
						if(GetAgentAccSheet() && ca_rec.DefAgentID && !pPack->Ext.AgentID)
							pPack->Ext.AgentID = ca_rec.DefAgentID;
					// @v8.4.2 {
					if(pPack->Rec.Flags & BILLF_NEEDPAYMENT) {
						pPack->SetupDefaultPayDate(ca_rec.DefPayPeriod, ca_rec.PaymDateBase);
					}
					// } @v8.4.2
					// @v8.4.2 if(ca_rec.DefPayPeriod >= 0 && pPack->Rec.Flags & BILLF_NEEDPAYMENT)
					// @v8.4.2 pPack->SetPayDate(plusdate(pPack->Rec.Dt, ca_rec.DefPayPeriod), 0);
				}
			}
		}
	}
#endif // } 0 @v9.0.0
	CATCHZOK
	return ok;
}

static int SLAPI InsertComplList(PPBillPacket * pPack, PPComplBlock & rList, int sign, SString * pSrcSerial, PUGL * pDfctList)
{
	int    ok = 1, incomplete = 0;
	PPObjGoods goods_obj;
	LongArray positions;
	SString src_serial;
	PUGL   deficit_list;
	PUGL * p_deficit_list = NZOR(pDfctList, &deficit_list);
	for(uint i = 0; i < rList.getCount(); i++) {
		ComplItem & r_item = rList.at(i);
		src_serial = 0;
		if(r_item.PartQty != 0 && r_item.NeedQty != 0) {
			long   convert_ilti_flags = CILTIF_OPTMZLOTS|CILTIF_USESUBST;
			ILTI   ilti;
			ilti.GoodsID = r_item.GoodsID;
			if(r_item.GoodsFlags & GF_UNLIM) {
				QuotIdent qi(QIDATE(pPack->Rec.Dt), pPack->Rec.LocID, PPQUOTK_BASE, pPack->Rec.CurID);
				if(goods_obj.GetQuot(ilti.GoodsID, qi, 0L, 0L, &ilti.Price) <= 0)
					ilti.Price = 0.0;
				// @v9.4.2 {
				if(ilti.Cost <= 0.0)
					ilti.Cost = ilti.Price;
				// } @v9.4.2 
			}
			if(sign > 0) {
				ilti.SetQtty(r_item.NeedQty, 0, PPTFR_RECEIPT | PPTFR_PLUS);
				if(!(r_item.GoodsFlags & GF_UNLIM))
					THROW(::GetCurGoodsPrice(ilti.GoodsID, pPack->Rec.LocID, GPRET_MOSTRECENT, &ilti.Price));
			}
			else {
				ilti.SetQtty(r_item.NeedQty, 0, PPTFR_MINUS);
				if(r_item.GsiFlags & GSIF_QUERYEXPLOT) {
					RVALUEPTR(src_serial, pSrcSerial);
					convert_ilti_flags = 0;
				}
			}
			THROW(BillObj->ConvertILTI(&ilti, pPack, &positions, convert_ilti_flags, src_serial, 0));
			if(src_serial.NotEmpty())
				ok = 2;
			r_item.FreeQty = r_item.NeedQty - fabs(ilti.Rest);
			// @v9.4.9 if(R6(ilti.Rest) != 0.0) {
			if(ilti.HasDeficit()) { // @v9.4.9
				THROW(p_deficit_list->Add(&ilti, pPack->Rec.LocID, i-1, pPack->Rec.Dt));
				incomplete = 1;
			}
		}
	}
	if(incomplete) {
		pPack->RemoveRows(&positions);
		ok = (PPSetError(PPERR_AUTOCOMPLREST), -1);
	}
	CATCH
		ok = 0;
		pPack->RemoveRows(&positions);
	ENDCATCH
	pPack->SetQuantitySign(-1);
	return ok;
}

int SLAPI PPObjBill::CreateModifByPUGL(PPID modifOpID, PPID * pID, PUGL * pPugl, PPID sessID, const GoodsReplacementArray * pGra)
{
	//
	// @>>PPBillPacket::InsertComplete >> PPObjBill::ConvertILTI
	//
	//
	int    ok = 1;
	uint   i;
	PUGL   compl_pugl;
	PPBillPacket pack;
	ASSIGN_PTR(pID, 0);
	THROW(pack.CreateBlank(modifOpID, 0, pPugl->LocID, 0));
	pack.Rec.Dt = pPugl->Dt;
	pack.Rec.LocID = pPugl->LocID;
	//
	// Если документ создается для комплектации только одного наименования товара,
	// то вносим это наименование в примечание к документу
	//
	if(pPugl->getCount() == 1) {
		SString memo_buf;
		GetGoodsName(((PUGI *)pPugl->at(0))->GoodsID, memo_buf);
		memo_buf.CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
	}
	for(i = 0; i < pPugl->getCount();) {
		PUGI   pugi = *(PUGI *)pPugl->at(i++);
		PPGoodsStruc gs;
		PPGoodsStruc::Ident gs_ident(pugi.GoodsID, GSF_COMPL, GSF_PARTITIAL, pack.Rec.Dt);
		uint   acpos = 0;
		const  int lgs_r = LoadGoodsStruc(&gs_ident, &gs);
		if(lgs_r > 0) {
			int    r = 0;
			pPugl->atFree(--i);

			ReceiptTbl::Rec lot_rec;
			PPTransferItem ti;

			THROW(ti.Init(&pack.Rec, 1, 1));
			THROW(ti.SetupGoods(pugi.GoodsID));
			ti.SetupLot(0, 0, 0);
			ti.Quantity_ = fabs(R6(pugi.DeficitQty));
			if(trfr->Rcpt.GetLastLot(ti.GoodsID, 0L, pack.Rec.Dt, &lot_rec) > 0) {
				ti.Price = R5(lot_rec.Price);
				ti.Cost  = R5(lot_rec.Cost);
				ti.QCert = lot_rec.QCertID;
				ti.UnitPerPack = lot_rec.UnitPerPack;
			}
			else {
				ti.Price = pugi.Price;
				ti.Cost  = pugi.Cost;
			}
			if(pugi.Price > 0.0)
				ti.Price = pugi.Price;
			acpos = pack.GetTCount();
			THROW(pack.InsertRow(&ti, 0));
			THROW(r = pack.InsertComplete(&gs, acpos, &compl_pugl, PCUG_CANCEL, pGra));
			THROW(pack.ShrinkTRows());
		}
	}
	for(i = 0; i < compl_pugl.getCount(); i++) {
		THROW(pPugl->Add((PUGI *)compl_pugl.at(i), pack.Rec.Dt));
	}
	if(pPugl->getCount() == 0) {
		pack.SetPoolMembership(PPBillPacket::bpkCSessDfct, sessID);
		pack.CalcModifCost();
		THROW(__TurnPacket(&pack, 0, 1, 0));
		ASSIGN_PTR(pID, pack.Rec.ID);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

SLAPI PPObjBill::WrOffDraftBlock::WrOffDraftBlock(const PPDraftOpEx * pWrOffParam, PUGL * pDfctList) : P_WrOffParam(pWrOffParam), P_DfctList(pDfctList)
{
}

int SLAPI PPObjBill::Helper_WrOffDrft_ExpModif(WrOffDraftBlock & rBlk, int use_ta)
{
	//
	// Списание драфт-документа в документ модификации.
	// Если задана операция компенсации дефицита посредством модификации,
	// то предпринимаем попытку сформировать комплектацию, способную покрыть
	// дефицит, после чего вновь пытаемся списать документ.
	//
	int    ok = -1, r;
	int    incomplete = 0;
	PPBillPacket * p_pack = 0;
	PPGoodsStruc gs;
	PPComplBlock compl_list;
	LongArray rows;
	THROW_MEM(p_pack = new PPBillPacket);
	THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
	for(uint i = 0; i < rBlk.SrcDraftPack.GetTCount(); i++) {
		const PPTransferItem & r_src_ti = rBlk.SrcDraftPack.ConstTI(i);
		ILTI ilti(&r_src_ti);
		ilti.Flags |= PPTFR_MINUS;
		rows.clear();
		THROW(ConvertILTI(&ilti, p_pack, &rows, CILTIF_DEFAULT, 0));
		// @v9.4.9 if(ilti.Rest == 0.0) {
		if(!ilti.HasDeficit()) { // @v9.4.9
			PPGoodsStruc::Ident gs_ident(r_src_ti.GoodsID, GSF_DECOMPL, GSF_PARTITIAL, p_pack->Rec.Dt);
			if(LoadGoodsStruc(&gs_ident, &gs) > 0) {
				for(uint j = rows.getCount()-1; !incomplete && j >= 0; j--) {
					const PPTransferItem & r_row_ti = p_pack->ConstTI(rows.at(j));
					THROW(gs.InitCompleteData2(r_row_ti.GoodsID, r_row_ti.Quantity_, compl_list));
				}
			}
		}
		else {
			if(rBlk.P_DfctList)
				THROW(rBlk.P_DfctList->Add(&ilti, rBlk.SrcDraftPack.Rec.LocID, i, p_pack->Rec.Dt));
			incomplete = 1;
		}
	}
	THROW(r = InsertComplList(p_pack, compl_list, +1, 0 /*pSrcSerial*/, rBlk.P_DfctList));
	p_pack->CalcModifCost();
	THROW_SL(rBlk.ResultList.insert(p_pack));
	p_pack = 0;
	ok = 1;
	CATCHZOK
	if(incomplete)
		ok *= 1000;
	delete p_pack; // Разрушается, если не был вставлен в rBlk.ResultList
	return ok;
}

int SLAPI PPObjBill::Helper_WrOffDrft_ExpExp(WrOffDraftBlock & rBlk, int use_ta)
{
	//
	// Списание драфт-документа в расходный документ.
	// Если задана операция компенсации дефицита посредством модификации,
	// то предпринимаем попытку сформировать комплектацию, способную покрыть
	// дефицит, после чего вновь пытаемся списать документ.
	//
	int    ok = -1, r;
	int    incomplete = 0;
	int    try_again = 0;
	PUGL   temp_pugl;
	PPID   compl_bill_id = 0;
	PPBillPacket * p_pack = 0;
	LongArray rows;
	SString serial_buf;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW_MEM(p_pack = new PPBillPacket);
		do {
			THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, 0));
			for(uint i = 0; i < rBlk.SrcDraftPack.GetTCount(); i++) {
				const PPTransferItem & r_src_ti = rBlk.SrcDraftPack.ConstTI(i);
				// @v9.8.11 rBlk.SrcDraftPack.SnL.GetNumber(i, &serial_buf);
				rBlk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial_buf); // @v9.8.11 
				ILTI ilti(&r_src_ti);
				uint ciltif = CILTIF_USESUBST|CILTIF_USESUBST_STRUCONLY|CILTIF_SUBSTSERIAL|CILTIF_ALLOWZPRICE;
					// @v8.4.8 CILTIF_SUBSTSERIAL // @v9.2.1 CILTIF_ALLOWZPRICE
				if(!(CcFlags & CCFLG_NOADJPRWROFFDRAFT)) // @v9.3.4
					ciltif |= CILTIF_CAREFULLYALIGNPRICE;
				THROW(ConvertILTI(&ilti, p_pack, &rows, ciltif, serial_buf));
				if(ilti.HasDeficit()) {
					THROW(temp_pugl.Add(&ilti, rBlk.SrcDraftPack.Rec.LocID, i, p_pack->Rec.Dt));
					incomplete = 1;
				}
			}
			if(incomplete && rBlk.P_WrOffParam->WrOffComplOpID && GetOpType(rBlk.P_WrOffParam->WrOffComplOpID) == PPOPT_GOODSMODIF) {
				if(try_again == 0) {
					int    r;
					temp_pugl.SetHeader(&rBlk.SrcDraftPack.Rec);
					THROW(r = CreateModifByPUGL(rBlk.P_WrOffParam->WrOffComplOpID, &compl_bill_id, &temp_pugl));
					if(r > 0) {
						try_again = 1;
						incomplete = 0;
					}
				}
				else {
					if(compl_bill_id) {
						THROW(RemovePacket(compl_bill_id, 0));
						compl_bill_id = 0;
					}
					break;
				}
			}
			else
				break;
		} while(try_again == 1);
		//
		// Если установлен флаг, допускающий досписание частичной структуры, то
		// добавляем в результирующий документ элементы частичных структур, ассоциированных
		// с первоначально добавленными товарами.
		//
		if(rBlk.P_WrOffParam->Flags & DROXF_USEPARTSTRUC) {
			PPComplBlock compl_list;
			for(uint i = 0; i < rows.getCount(); i++) {
				PPGoodsStruc gs;
				PPTransferItem & r_ti = p_pack->TI(rows.at(i));
				PPGoodsStruc::Ident gs_ident(r_ti.GoodsID, GSF_PARTITIAL, 0, p_pack->Rec.Dt);
				if(LoadGoodsStruc(&gs_ident, &gs) > 0) {
					THROW(gs.InitCompleteData2(r_ti.GoodsID, r_ti.Quantity_, compl_list));
				}
			}
			THROW(r = InsertComplList(p_pack, compl_list, -1, 0/*pSrcSerial*/, rBlk.P_DfctList));
		}
		if(rBlk.P_DfctList) {
			for(uint i = 0; i < temp_pugl.getCount(); i++)
				THROW(rBlk.P_DfctList->Add((PUGI *)temp_pugl.at(i), p_pack->Rec.Dt));
		}
		THROW(tra.Commit());
		rBlk.ResultList.insert(p_pack);
		p_pack = 0;
	}
	ok = 1;
	CATCHZOK
	if(incomplete)
		ok *= 1000;
	delete p_pack; // Разрушается, если не был вставлен в rBlk.ResultList
	return ok;
}


int SLAPI PPObjBill::Helper_WrOffDrft_Acct(WrOffDraftBlock & rBlk, int use_ta)
{
	//
	// Списание драфт-документа в бухгалтерский документ.
	// Очень простая процедура: формируем бух документ, номинальная сумма равна номинальной сумме драфт-документа.
	// Все суммы из драф-документа копируем в результирующий и фиксируем эти суммы.
	//
	int    ok = -1;
	const  PPID src_bill_id = rBlk.SrcDraftPack.Rec.ID;
	PPBillPacket * p_pack = 0;
	THROW_MEM(p_pack = new PPBillPacket);
	THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, use_ta));
	{
		p_pack->Rec.Memo[0] = 0;
		THROW(p_pack->InitAmounts(0));
		p_pack->Rec.Amount = rBlk.SrcDraftPack.Rec.Amount;
		p_pack->Amounts = rBlk.SrcDraftPack.Amounts;
		p_pack->Rec.Flags |= BILLF_FIXEDAMOUNTS;
		rBlk.ResultList.insert(p_pack);
		p_pack = 0;
		ok = 1;
	}
	CATCHZOK
	delete p_pack; // Разрушается, если не был вставлен в rBlk.ResultList
	return ok;
}

int SLAPI PPObjBill::Helper_WrOffDrft_ExpDrftRcp(WrOffDraftBlock & rBlk, int use_ta)
{
	//
	// Списание расходного драфт-документа в приходный драфт-документ.
	// Функция очень специфическая: если товарная позиция может быть разложена на компоненты (по структуре,
	// то в результирующий документ вставляются терминальные компоненты с финальной балансировкой цен для получения
	// сумм документа списания эквивалентных списываемому документу.
	//
	int    ok = -1;
	const  PPID src_bill_id = rBlk.SrcDraftPack.Rec.ID;
	PUGL   temp_pugl;
	PPID   compl_bill_id = 0;
	LongArray rows;
	SString serial_buf;
	PPObjMrpTab mrp_obj;
	MrpTabPacket mrp_pack;
	PPBillPacket * p_pack = 0;
	THROW_MEM(p_pack = new PPBillPacket);
	THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, use_ta));
	{
		LongArray rows;
		mrp_pack.Init(PPOBJ_BILL, src_bill_id, "temp-#");
		THROW(ok = Helper_PutBillToMrpTab(src_bill_id, &mrp_pack, rBlk.P_WrOffParam, use_ta));
		if(mrp_pack.getCount()) {
			THROW(mrp_obj.FinishPacket(&mrp_pack, PPObjMrpTab::cfIgnoreRest, use_ta));
			if(mrp_pack.getCount() == 1) {
				const MrpTabLeaf & r_leaf = mrp_pack.at(0);
				RAssocArray req_list;
				CMrpTab term_list;
				double _sum_price = 0.0;
				THROW(mrp_pack.GetCTerminalList(r_leaf.TabID, term_list));
				{
					for(uint i = 0; i < term_list.getCount(); i++) {
						const CMrpTab::Row & r_row = term_list.at(i);
						if(r_row.SrcID == MRPSRCV_TOTAL) {
							THROW_SL(req_list.Add(r_row.DestID, r_row.DestReq));
						}
					}
				}
				{
					for(uint i = 0; i < req_list.getCount(); i++) {
						const RAssoc & r_item = req_list.at(i);
						Goods2Tbl::Rec goods_rec;
						if(r_item.Key && GObj.Fetch(r_item.Key, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC)) {
							const double _qtty = r_item.Val;
							LDATE  _dt = p_pack->Rec.Dt;
							double _price = 0.0;
							PPTransferItem ti;
							ReceiptTbl::Rec lot_rec;
							THROW(ti.Init(&p_pack->Rec));
							ti.GoodsID = r_item.Key;
							ti.Quantity_ = _qtty;
							while(_price == 0.0) {
								if(trfr->Rcpt.GetLastLot(ti.GoodsID, p_pack->Rec.LocID, _dt, &lot_rec) > 0) {
									if(lot_rec.Price > 0.0)
										_price = lot_rec.Price;
									else
										_dt = plusdate(lot_rec.Dt, -1);
								}
								else if(trfr->Rcpt.GetLastLot(ti.GoodsID, -1, _dt, &lot_rec) > 0) {
									if(lot_rec.Price > 0.0)
										_price = lot_rec.Price;
									else
										_dt = plusdate(lot_rec.Dt, -1);
								}
								else
									break;
							}
							if(_price > 0.0) {
								ti.Cost = _price;
								ti.Price = _price;
								_sum_price += (_price * _qtty);
								rows.clear();
								THROW(p_pack->InsertRow(&ti, &rows));
								ok = 1;
							}
						}
					}
				}
				{
					const double _balance_amount = rBlk.SrcDraftPack.Rec.Amount;
					double running_total = 0.0;
					double min_qtty  = SMathConst::Max;
					double max_price = 0.0;
					uint   last_index = 0;
					for(uint i = 0; i < p_pack->GetTCount(); i++) {
						PPTransferItem & r_ti = p_pack->TI(i);
						const double _cost = R2(r_ti.Price * _balance_amount / _sum_price);
						r_ti.Cost = _cost;
						r_ti.Price = _cost;
						const double _qtty = r_ti.Quantity_;
						running_total += R2(_cost * _qtty);
						if(_qtty >= 1.0 && (_qtty < min_qtty || (_qtty == min_qtty && _cost > max_price))) {
							last_index = i+1;
							min_qtty = _qtty;
							max_price = _cost;
						}
					}
					if(fabs(running_total - _balance_amount) >= 0.01 && last_index) {
						PPTransferItem & r_ti = p_pack->TI(last_index-1);
						r_ti.Cost = R5(r_ti.Cost + (_balance_amount - running_total) / r_ti.Quantity_);
						r_ti.Price = r_ti.Cost;
					}
				}
			}
		}
	}
	if(ok > 0) {
		p_pack->Rec.Memo[0] = 0;
		THROW(p_pack->InitAmounts(0));
		p_pack->Amounts = rBlk.SrcDraftPack.Amounts;
		p_pack->Rec.Flags |= BILLF_FIXEDAMOUNTS;
		rBlk.ResultList.insert(p_pack);
		p_pack = 0;
	}
	/*
	for(uint i = 0; i < rSrcDraftPack.GetTCount(); i++) {
		const PPTransferItem & r_src_ti = rSrcDraftPack.ConstTI(i);
	}
	*/
	//ok = 1;
	CATCHZOK
	delete p_pack; // Разрушается, если не был вставлен в rBlk.ResultList
	return ok;
}

int SLAPI PPObjBill::Helper_WrOffDrft_DrftRcptModif(WrOffDraftBlock & rBlk, PPIDArray * pWrOffBills)
{
	int    ok = 1;
	int    incomplete = 0, processed = 0, j, r;
	PPGoodsStruc gs;
	LongArray rows;
	PPTransferItem ti;
	SString serial_buf/*, clb_buf*/;
	SString src_serial;
	PPBillPacket * p_pack = 0;
	if(rBlk.P_WrOffParam->Flags & DROXF_MULTDRFTWROFF) {
		for(uint i = 0; i < rBlk.SrcDraftPack.GetTCount(); i++) {
			const PPTransferItem & r_src_ti = rBlk.SrcDraftPack.ConstTI(i);
			PPComplBlock compl_list;
			rBlk.SrcDraftPack.LTagL.GetTagStr(i, PPTAG_LOT_SOURCESERIAL, src_serial); // @v9.4.1
			THROW_MEM(p_pack = new PPBillPacket);
			THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
			ti.Init(&p_pack->Rec, 0, +1);
			THROW(ti.SetupGoods(r_src_ti.GoodsID));
			ti.UnitPerPack = r_src_ti.UnitPerPack;
			ti.Quantity_ = r_src_ti.Quantity_;
			ti.Cost  = r_src_ti.Cost;
			ti.Price = r_src_ti.Price;
			ti.QCert = r_src_ti.QCert;
			ti.Flags = r_src_ti.Flags;
			ti.Flags &= ~PPTFR_DRAFT;
			ti.TFlags &= ~PPTransferItem::tfDirty;
			rows.clear();
			THROW(p_pack->InsertRow(&ti, &rows));
			// @v9.8.11 rBlk.SrcDraftPack.SnL.GetNumber(i, &serial_buf);
			rBlk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial_buf); // @v9.8.11
			for(j = rows.getCount()-1; !incomplete && j >= 0; j--) {
				PPGoodsStruc::Ident gs_ident(r_src_ti.GoodsID, GSF_COMPL, GSF_PARTITIAL, p_pack->Rec.Dt);
				const uint pos = rows.at(j);
				// @v9.8.11 THROW(p_pack->ClbL.AddNumber(pos, serial_buf));
				THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_CLB, pos, serial_buf)); // @v9.8.11
				if(LoadGoodsStruc(&gs_ident, &gs) > 0) {
					const PPTransferItem & r_row_ti = p_pack->ConstTI(pos);
					THROW(gs.InitCompleteData2(r_row_ti.GoodsID, r_row_ti.Quantity_, compl_list));
				}
			}
			//
			{
				uint   new_item_pos = 0;
				THROW(r = InsertComplList(p_pack, compl_list, -1, &src_serial, rBlk.P_DfctList));
				if(r > 0) {
					// @v9.4.1 {
					if(r == 2) {
						if(src_serial.NotEmpty()) {
							const uint lp = rows.at(0);
							// @v9.8.11 p_pack->SnL.GetNumber(lp, &serial_buf);
							p_pack->LTagL.GetNumber(PPTAG_LOT_SN, lp, serial_buf); // @v9.8.11 
							if(serial_buf.Empty()) {
								// @todo Следует формировать новую серию по какому-либо шаблону
								(serial_buf = src_serial).CatChar('-').Cat("???");
								// @v9.8.11 p_pack->SnL.AddNumber(lp, serial_buf);
								p_pack->LTagL.AddNumber(PPTAG_LOT_SN, lp, serial_buf); // @v9.8.11 
							}
						}
					}
					// } @v9.4.1
					p_pack->CalcModifCost();
				}
				else
					incomplete = 1;
				new_item_pos = rBlk.ResultList.getCount();
				THROW_SL(rBlk.ResultList.insert(p_pack));
				if(r > 0) {
					THROW(Helper_WriteOffTurnResultItem(rBlk, new_item_pos, pWrOffBills));
				}
				p_pack = 0;
				processed = 1;
			}
		}
	}
	else {
		PPComplBlock compl_list;
		THROW_MEM(p_pack = new PPBillPacket);
		THROW(InitDraftWrOffPacket(rBlk.P_WrOffParam, &rBlk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
		for(uint i = 0; i < rBlk.SrcDraftPack.GetTCount(); i++) {
			const PPTransferItem & r_src_ti = rBlk.SrcDraftPack.ConstTI(i);
			ti.Init(&p_pack->Rec, 0, +1);
			THROW(ti.SetupGoods(r_src_ti.GoodsID));
			ti.UnitPerPack = r_src_ti.UnitPerPack;
			ti.Quantity_ = r_src_ti.Quantity_;
			ti.Cost  = r_src_ti.Cost;
			ti.Price = r_src_ti.Price;
			ti.QCert = r_src_ti.QCert;
			ti.Flags = r_src_ti.Flags;
			ti.Flags &= ~PPTFR_DRAFT;
			ti.TFlags &= ~PPTransferItem::tfDirty;
			rows.clear();
			THROW(p_pack->InsertRow(&ti, &rows));
			// @v9.8.11 rBlk.SrcDraftPack.SnL.GetNumber(i, &serial_buf);
			rBlk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial_buf);
			for(j = rows.getCount()-1; !incomplete && j >= 0; j--) {
				PPGoodsStruc::Ident gs_ident(r_src_ti.GoodsID, GSF_COMPL, GSF_PARTITIAL, p_pack->Rec.Dt);
				const uint pos = rows.at(j);
				// @v9.8.11 THROW(p_pack->ClbL.AddNumber(pos, serial_buf));
				THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_CLB, pos, serial_buf)); // @v9.8.11
				if(LoadGoodsStruc(&gs_ident, &gs) > 0) {
					const PPTransferItem & r_row_ti = p_pack->ConstTI(pos);
					THROW(gs.InitCompleteData2(r_row_ti.GoodsID, r_row_ti.Quantity_, compl_list));
				}
			}
		}
		THROW(r = InsertComplList(p_pack, compl_list, -1, 0 /*pSrcSerial*/, rBlk.P_DfctList));
		if(r > 0)
			p_pack->CalcModifCost();
		else
			incomplete = 1;
		THROW_SL(rBlk.ResultList.insert(p_pack));
		p_pack = 0;
		processed = 1;
	}
	CATCHZOK
	if(incomplete)
		ok *= 1000;
	delete p_pack;
	return ok;
}

int SLAPI PPObjBill::Helper_WriteOffTurnResultItem(WrOffDraftBlock & rBlk, uint pos, PPIDArray * pWrOffBills) // @notransaction
{
	int    ok = -1;
	if(pos < rBlk.ResultList.getCount()) {
		PPBillPacket * p_bp = rBlk.ResultList.at(pos);
		if(p_bp && p_bp->Rec.ID == 0) {
			if(CheckOpFlags(p_bp->Rec.OpID, OPKF_ONORDER)) {
				BillTbl::Rec link_rec;
				const PPID link_bill_id = rBlk.SrcDraftPack.Rec.LinkBillID;
				if(Search(link_bill_id, &link_rec) > 0 && GetOpType(link_rec.OpID) == PPOPT_GOODSORDER) {
					PPBillPacket ord_pack;
					THROW(ExtractPacket(link_bill_id, &ord_pack) > 0);
					THROW(p_bp->AttachToOrder(&ord_pack));
				}
			}
			if(p_bp->Rec.OpID) {
				THROW(__TurnPacket(p_bp, pWrOffBills, 0, 0));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::Helper_WriteOffDraft(PPID billID, const PPDraftOpEx * pWrOffParam, PPIDArray * pWrOffBills, PUGL * pDfctList, int use_ta)
{
	int    ok = -1;
	int    incomplete = 0, processed = 0, r;
	// @v9.4.0 int    j;
	PPID   compl_bill_id = 0;
	PPTransferItem ti;
	SString serial_buf, clb_buf;
	PPBillPacket * p_pack = 0;
	PPGoodsStruc gs;
	// @v9.4.0 PPComplBlock compl_list;
	LongArray rows;
	WrOffDraftBlock blk(pWrOffParam, pDfctList);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(ExtractPacket(billID, &blk.SrcDraftPack) > 0); // @v8.5.8
		if(blk.P_WrOffParam->WrOffOpID == 0) {
			// Если операция списания не определена, то, естественно, никаких документов
			// формировать не требуется. Все, что мы должны сделать - пометить документ как списанный
			processed = 1;
		}
		else {
			const PPID wroff_op_type_id = GetOpType(blk.P_WrOffParam->WrOffOpID);
			if(blk.SrcDraftPack.OpTypeID == PPOPT_DRAFTEXPEND) {
				switch(wroff_op_type_id) {
					case PPOPT_ACCTURN:
						THROW(r = Helper_WrOffDrft_Acct(blk, 0));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
						break;
					case PPOPT_DRAFTRECEIPT:
						THROW(r = Helper_WrOffDrft_ExpDrftRcp(blk, 0));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
						break;
					case PPOPT_GOODSEXPEND:
						THROW(r = Helper_WrOffDrft_ExpExp(blk, 0));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
						break;
					case PPOPT_GOODSMODIF:
						THROW(r = Helper_WrOffDrft_ExpModif(blk, 0));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
#if 0 // @v9.4.0 {
						THROW_MEM(p_pack = new PPBillPacket);
						THROW(InitDraftWrOffPacket(blk.P_WrOffParam, &blk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
						for(uint i = 0; i < blk.SrcDraftPack.GetTCount(); i++) {
							const PPTransferItem & r_src_ti = blk.SrcDraftPack.ConstTI(i);
							ILTI ilti(&r_src_ti);
							ilti.Flags |= PPTFR_MINUS;
							rows.clear();
							THROW(ConvertILTI(&ilti, p_pack, &rows, CILTIF_DEFAULT, 0));
							if(ilti.Rest == 0.0) {
								PPGoodsStruc::Ident gs_ident(r_src_ti.GoodsID, GSF_DECOMPL, GSF_PARTITIAL, p_pack->Rec.Dt);
								if(LoadGoodsStruc(&gs_ident, &gs) > 0)
									for(j = rows.getCount()-1; !incomplete && j >= 0; j--)
										THROW(InitCompleteData2(&gs, p_pack->ConstTI(rows.at(j)), &compl_list));
							}
							else {
								if(blk.P_DfctList)
									THROW(blk.P_DfctList->Add(&ilti, blk.SrcDraftPack.Rec.LocID, i, p_pack->Rec.Dt));
								incomplete = 1;
							}
						}
						THROW(r = InsertComplList(p_pack, compl_list, +1, blk.P_DfctList));
						p_pack->CalcModifCost();
						THROW_SL(blk.ResultList.insert(p_pack));
						p_pack = 0;
						processed = 1;
#endif // } 0 @v9.4.0
						break;
					case PPOPT_GOODSORDER:
						THROW_MEM(p_pack = new PPBillPacket);
						THROW(InitDraftWrOffPacket(blk.P_WrOffParam, &blk.SrcDraftPack.Rec, p_pack, 0));
						for(uint i = 0; i < blk.SrcDraftPack.GetTCount(); i++) {
							const PPTransferItem & r_src_ti = blk.SrcDraftPack.ConstTI(i);
							ILTI ilti(&r_src_ti);
							rows.clear();
							THROW(ConvertILTI(&ilti, p_pack, &rows, CILTIF_DEFAULT|CILTIF_ABSQTTY, 0));
						}
						THROW_SL(blk.ResultList.insert(p_pack));
						p_pack = 0;
						processed = 1;
						break;
				}
			}
			else if(blk.SrcDraftPack.OpTypeID == PPOPT_DRAFTRECEIPT) {
				switch(wroff_op_type_id) {
					case PPOPT_ACCTURN:
						THROW(r = Helper_WrOffDrft_Acct(blk, 0));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
						break;
					case PPOPT_DRAFTRECEIPT:
						THROW_MEM(p_pack = new PPBillPacket);
						THROW(InitDraftWrOffPacket(blk.P_WrOffParam, &blk.SrcDraftPack.Rec, p_pack, 0 /*use_ta*/));
						if(!(blk.P_WrOffParam->Flags & DROXF_CREMPTYBILL)) {
							for(uint i = 0; i < blk.SrcDraftPack.GetTCount(); i++) {
								const PPTransferItem & r_src_ti = blk.SrcDraftPack.ConstTI(i);
								THROW(ti.Init(&p_pack->Rec));
								THROW(ti.SetupGoods(r_src_ti.GoodsID));
								ti.UnitPerPack = r_src_ti.UnitPerPack;
								ti.Quantity_ = r_src_ti.Quantity_;
								ti.Cost  = r_src_ti.Cost;
								ti.Price = r_src_ti.Price;
								if(ti.CurID) {
									ti.CurPrice = r_src_ti.CurPrice;
									double base_price = TR5(ti.CurPrice * p_pack->Rec.CRate);
									if(ti.Flags & PPTFR_SELLING)
										ti.Price = base_price;
									else
										ti.Cost = base_price;
								}
								ti.QCert = r_src_ti.QCert;
								if(!(blk.P_WrOffParam->Flags & DROXF_DONTINHEXPIRY))
									ti.Expiry = r_src_ti.Expiry;
								ti.Flags = r_src_ti.Flags;
								ti.Flags &= ~PPTFR_DRAFT;
								ti.TFlags &= ~PPTransferItem::tfDirty;
								THROW(p_pack->InsertRow(&ti, 0));
								{
									const uint dest_pos = p_pack->GetTCount()-1;
									// @v9.8.11 blk.SrcDraftPack.ClbL.GetNumber(i, &clb_buf);
									// @v9.8.11 blk.SrcDraftPack.SnL.GetNumber(i, &serial_buf);
									// @v9.8.11 THROW(p_pack->ClbL.AddNumber(dest_pos, clb_buf));
									// @v9.8.11 THROW(p_pack->SnL.AddNumber(dest_pos, serial_buf));
									blk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_CLB, i, clb_buf); // @v9.8.11
									THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_CLB, dest_pos, clb_buf)); // @v9.8.11 
									blk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial_buf); // @v9.8.11
									THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_SN, dest_pos, serial_buf)); // @v9.8.11 
									{
										const ObjTagList * p_org_lot_tag_list = blk.SrcDraftPack.LTagL.Get(i);
										THROW(p_pack->LTagL.Set(dest_pos, p_org_lot_tag_list));
									}
								}
							}
						}
						THROW_SL(blk.ResultList.insert(p_pack));
						p_pack = 0;
						processed = 1;
						break;
					case PPOPT_GOODSRECEIPT:
						THROW_MEM(p_pack = new PPBillPacket);
						THROW(InitDraftWrOffPacket(blk.P_WrOffParam, &blk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
						if(!(blk.P_WrOffParam->Flags & DROXF_CREMPTYBILL)) {
							for(uint i = 0; i < blk.SrcDraftPack.GetTCount(); i++) {
								const PPTransferItem & r_src_ti = blk.SrcDraftPack.ConstTI(i);
								THROW(ti.Init(&p_pack->Rec));
								THROW(ti.SetupGoods(r_src_ti.GoodsID));
								ti.RByBill = r_src_ti.RByBill; // @v8.9.8
								ti.UnitPerPack = r_src_ti.UnitPerPack;
								ti.Quantity_ = r_src_ti.Quantity_;
								ti.Cost  = r_src_ti.Cost;
								ti.Price = r_src_ti.Price;
								if(ti.CurID) {
									ti.CurPrice = r_src_ti.CurPrice;
									double base_price = TR5(ti.CurPrice * p_pack->Rec.CRate);
									if(ti.Flags & PPTFR_SELLING)
										ti.Price = base_price;
									else
										ti.Cost = base_price;
								}
								ti.QCert = r_src_ti.QCert;
								if(!(blk.P_WrOffParam->Flags & DROXF_DONTINHEXPIRY))
									ti.Expiry = r_src_ti.Expiry;
								ti.Flags = r_src_ti.Flags;
								ti.Flags &= ~PPTFR_DRAFT;
								ti.TFlags |= PPTransferItem::tfForceNew; // @v8.9.8
								ti.TFlags &= ~PPTransferItem::tfDirty;
								THROW(p_pack->InsertRow(&ti, 0));
								{
									const uint dest_pos = p_pack->GetTCount()-1;
									// @v9.8.11 blk.SrcDraftPack.ClbL.GetNumber(i, &clb_buf);
									// @v9.8.11 blk.SrcDraftPack.SnL.GetNumber(i, &serial_buf);
									// @v9.8.11 THROW(p_pack->ClbL.AddNumber(dest_pos, clb_buf));
									// @v9.8.11 THROW(p_pack->SnL.AddNumber(dest_pos, serial_buf));
									blk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_CLB, i, clb_buf); // @v9.8.11 
									THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_CLB, dest_pos, clb_buf)); // @v9.8.11 
									blk.SrcDraftPack.LTagL.GetNumber(PPTAG_LOT_SN, i, serial_buf); // @v9.8.11 
									THROW(p_pack->LTagL.AddNumber(PPTAG_LOT_SN, dest_pos, serial_buf)); // @v9.8.11 
									{
										const ObjTagList * p_org_lot_tag_list = blk.SrcDraftPack.LTagL.Get(i);
										THROW(p_pack->LTagL.Set(dest_pos, p_org_lot_tag_list));
									}
								}
							}
							p_pack->ProcessFlags |= PPBillPacket::pfForceRByBill; // @v8.9.8
						}
						THROW_SL(blk.ResultList.insert(p_pack));
						p_pack = 0;
						processed = 1;
						break;
					case PPOPT_GOODSMODIF:
						THROW(r = Helper_WrOffDrft_DrftRcptModif(blk, pWrOffBills));
						if(r > 0) {
							processed = 1;
							if(r == 1000)
								incomplete = 1;
						}
						break;
					case PPOPT_GOODSORDER:
						THROW_MEM(p_pack = new PPBillPacket);
						THROW(InitDraftWrOffPacket(blk.P_WrOffParam, &blk.SrcDraftPack.Rec, p_pack, 0/*use_ta*/));
						//for(rbybill = 0; P_CpTrfr->EnumItems(billID, &rbybill, &src_ti, &cpext) > 0;) {
						for(uint i = 0; i < blk.SrcDraftPack.GetTCount(); i++) {
							const PPTransferItem & r_src_ti = blk.SrcDraftPack.ConstTI(i);
							ILTI ilti(&r_src_ti);
							rows.clear();
							THROW(ConvertILTI(&ilti, p_pack, &rows, CILTIF_DEFAULT|CILTIF_ABSQTTY, 0));
						}
						THROW_SL(blk.ResultList.insert(p_pack));
						p_pack = 0;
						processed = 1;
						break;
				}
			}
		}
		if(processed) {
			if(incomplete)
				ok = -2;
			else {
				const uint rlc = blk.ResultList.getCount();
				if(rlc) {
					for(uint i = 0; i < rlc; i++) {
						/*
						PPBillPacket * p_bp = blk.ResultList.at(i);
						if(p_bp) {
							if(CheckOpFlags(p_bp->Rec.OpID, OPKF_ONORDER)) {
								BillTbl::Rec link_rec;
								const PPID link_bill_id = blk.SrcDraftPack.Rec.LinkBillID;
								if(Search(link_bill_id, &link_rec) > 0 && GetOpType(link_rec.OpID) == PPOPT_GOODSORDER) {
									PPBillPacket ord_pack;
									THROW(ExtractPacket(link_bill_id, &ord_pack) > 0);
									THROW(p_bp->AttachToOrder(&ord_pack));
								}
							}
							if(p_bp->Rec.OpID) {
								THROW(__TurnPacket(p_bp, pWrOffBills, 0, 0));
							}
						}
						*/
						THROW(Helper_WriteOffTurnResultItem(blk, i, pWrOffBills));
					}
				}
				else {
					//
					// @v6.9.0..6.9.8
					// Если нет документа списания (p_pack->Rec.OpID == 0), то необходимо
					// "в ручную" установить признак того, что draft списан.
					// В противном случае это сделает функция PPObjBill::TurnPacket
					//
					THROW(P_Tbl->SetRecFlag(billID, BILLF_WRITEDOFF, 1, 0));
				}
				ok = 1;
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_pack;
	return ok;
}

int SLAPI PPObjBill::WriteOffDraft(PPID billID, PPIDArray * pResultList, PUGL * pDfctList, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec bill_rec;
	if(Search(billID, &bill_rec) > 0 && !(bill_rec.Flags & BILLF_WRITEDOFF) && IsDraftOp(bill_rec.OpID)) {
		PPObjSecur::Exclusion ose(PPEXCLRT_DRAFTWROFF); // @v8.6.4
		PPOprKindPacket op_pack;
		THROW(P_OpObj->GetPacket(bill_rec.OpID, &op_pack) > 0);
		THROW_PP(op_pack.P_DraftData, PPERR_UNDEFWROFFOP);
		THROW(ok = Helper_WriteOffDraft(billID, op_pack.P_DraftData, pResultList, pDfctList, use_ta));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::RollbackWrOffDraft(PPID billID, int use_ta)
{
	int    ok = -1;
	BillTbl::Rec bill_rec, link_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(Search(billID, &bill_rec) > 0 && IsDraftOp(bill_rec.OpID) && bill_rec.Flags & BILLF_WRITEDOFF) {
			PPObjSecur::Exclusion ose(PPEXCLRT_DRAFTWROFFROLLBACK); // @v8.6.4
			for(DateIter di; P_Tbl->EnumLinks(billID, &di, BLNK_ALL, &link_rec) > 0;)
				THROW(RemovePacket(link_rec.ID, 0));
			// @v8.8.3 bill_rec.Flags &= ~BILLF_WRITEDOFF;
			// @v8.8.3 THROW(tbl->EditRec(&billID, &bill_rec, 0));
			THROW(P_Tbl->SetRecFlag(billID, BILLF_WRITEDOFF, 0, 0)); // @v8.8.3
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::Helper_CreateDeficitTi(PPBillPacket & rPack, const PUGL * pPugl, const PUGI * pItem, PUGL::SupplSubstItem * pSupplSubstItem, PPID & rComplArID)
{
	int    ok = 1;
	double cost, price;
	PPTransferItem ti;
	ReceiptTbl::Rec lot_rec;
	THROW(ti.Init(&rPack.Rec, 1, +1));
	THROW(ti.SetupGoods(pItem->GoodsID));
	ti.SetupLot(0, 0, 0);
	if(pSupplSubstItem) {
		ti.Quantity_ = pSupplSubstItem->Qtty;
		ti.Suppl = pSupplSubstItem->SupplID;
		ti.Flags |= PPTFR_FORCESUPPL;
	}
	else {
		ti.Quantity_ = R6(pItem->DeficitQty);
	}
	if(trfr->Rcpt.GetGoodsPrice(ti.GoodsID, rPack.Rec.LocID, rPack.Rec.Dt, GPRET_MOSTRECENT, &price, &lot_rec) > 0) {
		ti.Price = R5(lot_rec.Price);
		cost = lot_rec.Cost;
		SETIFZ(rComplArID, lot_rec.SupplID);
	}
	else {
		ti.Price = pItem->Price;
		cost = pItem->Cost;
	}
	ti.Cost = R5(pPugl->CostByCalc ? ti.Price / (1 + fdiv100r(pPugl->CalcCostPct)) : cost);
	THROW(rPack.InsertRow(&ti, 0));
	CATCHZOK
	return ok;
}

int SLAPI PPObjBill::ProcessDeficit(PPID compOpID, PPID compArID, const PUGL * pPugl, PPLogger * pLogger, int use_ta)
{
	int    ok = -1;
	PPOprKind op_rec;
	THROW_PP(compOpID, PPERR_UNDEFDWODFCTOP);
	THROW_PP(GetOpData(compOpID, &op_rec) > 0, PPERR_UNDEFDWODFCTOP);
	if(pPugl) {
		uint   i, j;
		PPBillPacket pack;
		PPID   comp_ar_id = compArID;
		PPIDArray loc_list;
		TSVector <PUGL::SupplSubstItem> suppl_subst_list; // @v9.8.6 TSArray-->TSVector
		pPugl->GetItemsLocList(loc_list);
		if(loc_list.getCount() == 0)
			loc_list.addUnique(0L);
		{
			PPTransaction tra(use_ta);
			THROW(tra);
			for(j = 0; j < loc_list.getCount(); j++) {
				PPID   loc_id = loc_list.at(j);
				SETIFZ(loc_id, NZOR(pPugl->LocID, LConfig.Location));
				if(loc_id) {
					int    r;
					PUGI * p_item;
					THROW(pack.CreateBlank2(compOpID, pPugl->Dt, loc_id, 0));
					pack.Rec.Object = comp_ar_id;
					PPGetWord(PPWORD_AT_AUTO, 0, pack.Rec.Memo, sizeof(pack.Rec.Memo));
					for(i = 0; pPugl->enumItems(&i, (void **)&p_item);) {
						if(p_item->DeficitQty > 0.0 && p_item->LocID == loc_id) {
							int do_suppl_subst = BIN(op_rec.OpTypeID == PPOPT_GOODSRECEIPT && pPugl->GetSupplSubstList(i, suppl_subst_list) > 0);
							if(do_suppl_subst && PUGL::BalanceSupplSubstList(suppl_subst_list, p_item->DeficitQty) > 0) {
								for(uint j = 0; j < suppl_subst_list.getCount(); j++) {
									THROW(Helper_CreateDeficitTi(pack, pPugl, p_item, &suppl_subst_list.at(j), comp_ar_id));
								}
							}
							else {
								THROW(Helper_CreateDeficitTi(pack, pPugl, p_item, 0, comp_ar_id));
							}
						}
					}
					SETIFZ(pack.Rec.Object, comp_ar_id);
					THROW(r = __TurnPacket(&pack, 0, 1, 0));
					if(r > 0 && pLogger)
						pLogger->LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
				}
			}
			THROW(tra.Commit());
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}
