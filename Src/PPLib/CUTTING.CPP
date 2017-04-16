// CUTTING.CPP
// Copyright (c) A.Sobolev 2000-2002, 2003, 2004, 2005, 2006, 2007, 2009, 2011, 2015, 2016
//
#include <pp.h>
#pragma hdrstop

int SLAPI Transfer::CuttingLotOperations(PPID lotID, LDATE endDate, double * pEndQtty)
{
	int    ok = 1, recalc_rests = 0;
	int    is_forward_op = 0; // Признак того, что по лоту есть операции после усекаемого периода.
		// Если "да", то лот с нулевым остатком не удаляется.
	LDATE  dt;
	long   oprno = 0;
	double end_rest, lot_qtty = 0.0;
	ReceiptTbl::Rec lot_rec;
	TransferTbl::Rec rec;
	//RECORDNUMBER lot_pos, first_rec_pos = 0;
	DBRowId lot_pos, first_rec_pos;
	first_rec_pos.SetZero();
	THROW(Rcpt.Search(lotID, &lot_rec) > 0);
	THROW_DB(Rcpt.getPosition(&lot_pos));
	GetRest(lot_rec.ID, endDate, &end_rest);
	GetLotPrices(&lot_rec, endDate, 0L);
	lot_qtty = lot_rec.Quantity;
	for(dt = ZERODATE; EnumByLot(lotID, &dt, &oprno, &rec) > 0 && dt <= endDate;) {
		//RECORDNUMBER pos;
		DBRowId pos;
		THROW_DB(getPosition(&pos));
		if(rec.Flags & PPTFR_RECEIPT && !((RECORDNUMBER)first_rec_pos))
			first_rec_pos = pos;
		else if(!rec.Reverse && SearchByBill(rec.BillID, 1, rec.RByBill, 0) > 0) {
			double child_end_qtty = 0.0;
			if(data.Flags & PPTFR_RECEIPT)
				THROW(CuttingLotOperations(data.LotID, endDate, &child_end_qtty));
			THROW_DB(getDirect(-1, 0, pos));
			child_end_qtty = R6(child_end_qtty);
			if(child_end_qtty > 0.0) {
				lot_qtty     += (data.Quantity + child_end_qtty);
				data.Quantity = -child_end_qtty;
				data.Cost  = TR5(lot_rec.Cost);
				data.Price = TR5(lot_rec.Price);
				THROW_DB(updateRec());
				recalc_rests = 1;
			}
			else {
				lot_qtty += data.Quantity;
				THROW_DB(deleteRec());
			}
		}
		else {
			THROW_DB(getDirect(-1, 0, pos));
			lot_qtty += data.Quantity;
			THROW_DB(deleteRec());
		}
	}
	lot_qtty = R6(lot_qtty);
	// @v5.3.11 {
	if(lot_qtty <= 0) {
		if(EnumByLot(lotID, &dt, &oprno, &rec) > 0)
			is_forward_op = 1;
	}
	// } @v5.3.11
	if(lot_qtty > 0 || is_forward_op) {
		THROW_DB(getDirect(-1, 0, first_rec_pos));
		data.Cost  = TR5(lot_rec.Cost);
		data.Price = TR5(lot_rec.Price);
		data.Quantity = data.Rest = lot_qtty;
		THROW_DB(updateRec());
		THROW_DB(Rcpt.getDirect(-1, 0, lot_pos));
		lot_rec.Quantity = lot_qtty;
		THROW_DB(Rcpt.updateRecBuf(&lot_rec));
		if(recalc_rests) {
			double temp_rest = 0.0;
			for(dt = ZERODATE, oprno = 0; EnumByLot(lotID, &dt, &oprno) > 0 && dt <= endDate;) {
				data.Rest = R6(temp_rest += data.Quantity);
				THROW_DB(updateRec());
			}
		}
	}
	else {
		if(first_rec_pos) {
			THROW_DB(getDirect(-1, 0, first_rec_pos));
			THROW_DB(deleteRec());
		}
		THROW_DB(Rcpt.getDirect(-1, 0, lot_pos));
		THROW_DB(Rcpt.deleteRec());
	}
	ASSIGN_PTR(pEndQtty, lot_qtty);
	CATCHZOK
	return ok;
}

int SLAPI Transfer::Cutting(LDATE endDate, int use_ta)
{
	int    ok = 1, ta = 0;
	LDATE  /*@v5.1.5 dt,*/ prev_date = ZERODATE;
	long   oprno = 0;
	SString msg_buf, msg_buf2;
	ReceiptTbl::Key1 rk;
	MEMSZERO(rk);
	PPGetSubStr(PPTXT_CUTTINGPROCESS, 2, msg_buf);
	PPWaitMsg(msg_buf);
	THROW(PPStartTransaction(&ta, use_ta));
	while(Rcpt.search(1, &rk, spGt) && rk.Dt <= endDate) {
		if(rk.Dt != prev_date) {
			PPWaitMsg((msg_buf2 = msg_buf).CatDiv('>', 1).Cat(rk.Dt));
			prev_date = rk.Dt;
		}
		if(Rcpt.data.PrevLotID == 0)
			THROW(CuttingLotOperations(Rcpt.data.ID, endDate, 0));
	}
	deleteFrom(this, 0, this->LotID == 0L && this->Dt <= endDate); // @v5.1.5
	{
		TransferTbl::Key1 k1;
		MEMSZERO(k1);
		while(search(1, &k1, spGt) && k1.Dt <= endDate)
			if(Rcpt.Search(data.LotID) <= 0)
				THROW_DB(deleteRec());
	}
	THROW(PPCommitWork(&ta));
	CATCH
		ok = 0;
		THROW(PPRollbackWork(&ta));
	ENDCATCH
	return ok;
}

int SLAPI PPObjBill::CuttingUntiedCLB(int use_ta)
{
	int    ok = 1, ta = 0;
	ObjTagTbl::Key1 k;
	ObjTagCore tagcore;
	k.TagID = PPTAG_LOT_CLB;
	k.ObjID = 0;
	THROW(PPStartTransaction(&ta, use_ta));
	if(tagcore.search(1, &k, spGe) && k.TagID == PPTAG_LOT_CLB)
		do {
			if(trfr->Rcpt.Search(k.ObjID) <= 0)
				THROW_DB(tagcore.deleteRec());
		} while(tagcore.search(1, &k, spNext) && k.TagID == PPTAG_LOT_CLB);
	THROW(PPCommitWork(&ta));
	CATCH
		ok = 0;
		PPRollbackWork(&ta);
	ENDCATCH
	return ok;
}

int SLAPI PPObjBill::Cutting(LDATE endDate, int use_ta)
{
	trfr->__DontCheckQttyInUpdateTransferItem__ = 1;

	int    ok = 1, ta = 0, frrl_tag = 0;
	SString balance_fname;
	SString msg_buf;
	BillTbl::Key1 bk;
	bk.Dt = endDate;
	bk.BillNo = MAXLONG;
	int    rbybill = 0;
	PPTransferItem ti;

	MakeTempFileName(0, 0, 0, 0, balance_fname);
	THROW(SaveBalance(endDate, balance_fname));

	THROW(PPStartTransaction(&ta, use_ta));
	THROW(atobj->P_Tbl->LockingFRR(1, &frrl_tag, 0));
	PPGetSubStr(PPTXT_CUTTINGPROCESS, 2, msg_buf);
	PPWaitMsg(msg_buf);
	THROW(trfr->Cutting(endDate, 0));

	PPGetSubStr(PPTXT_CUTTINGPROCESS, 3, msg_buf);
	PPWaitMsg(msg_buf);
	while(P_Tbl->search(1, &bk, spLt)) {
		double fix_paym = 0.0, paym = 0.0, debt;
		int    has_actual_link = 0;
		PPID   reckon_id;
		PPBillPacket pack;
		BillTbl::Rec bill_rec, link_bill_rec;
		DateIter di;

		P_Tbl->copyBufTo(&bill_rec);
		PPWaitDate(bill_rec.Dt);

		if(bill_rec.OpID == PPOPK_UNASSIGNED)
			continue;
		if(IsOpPaymOrRetn(bill_rec.OpID) && bill_rec.LinkBillID)
			continue;
		THROW(P_Tbl->GetAmount(bill_rec.ID, PPAMT_FIXPAYMENT, bill_rec.CurID, &fix_paym));
		for(di.Init(); P_Tbl->EnumLinks(bill_rec.ID, &di, BLNK_PAYMRETN, &link_bill_rec) > 0;) {
			double lpaym = BR2(link_bill_rec.Amount);
			if(checkdrange(link_bill_rec.Dt, ZERODATE, endDate)) {
				reckon_id = 0;
				if(IsMemberOfPool(link_bill_rec.ID, PPASS_PAYMBILLPOOL, &reckon_id) <= 0)
					reckon_id = 0;
				if(RemovePacket(link_bill_rec.ID, 0)) {
					if(link_bill_rec.CurID == bill_rec.CurID)
						fix_paym += lpaym;
					if(reckon_id && ExtractPacket(reckon_id, &pack) > 0) {
						pack.Amounts.Add(PPAMT_FIXPAYMENT, link_bill_rec.CurID, lpaym, 1);
						UpdatePacket(&pack, 0);
					}
				}
				else if(link_bill_rec.CurID == bill_rec.CurID) {
					has_actual_link = 1;
					paym += lpaym;
				}
			}
			else {
				has_actual_link = 1;
				if(link_bill_rec.CurID == bill_rec.CurID)
					paym += lpaym;
			}
		}
		for(reckon_id = 0; EnumMembersOfPool(PPASS_PAYMBILLPOOL, bill_rec.ID, &reckon_id) > 0;) {
			if(Search(reckon_id, &link_bill_rec) > 0) {
				double lpaym = BR2(link_bill_rec.Amount);
				if(checkdrange(link_bill_rec.Dt, ZERODATE, endDate)) {
					PPID   debt_id = 0;
					if(Search(link_bill_rec.LinkBillID) > 0)
						debt_id = link_bill_rec.LinkBillID;
					if(RemovePacket(link_bill_rec.ID, 0)) {
					   	if(link_bill_rec.CurID == bill_rec.CurID)
							fix_paym += lpaym;
						if(debt_id && ExtractPacket(debt_id, &pack) > 0) {
							pack.Amounts.Add(PPAMT_FIXPAYMENT, link_bill_rec.CurID, lpaym, 1);
							UpdatePacket(&pack, 0);
						}
					}
					else if(link_bill_rec.CurID == bill_rec.CurID) {
						has_actual_link = 1;
						paym += lpaym;
					}
				}
				else {
				   	has_actual_link = 1;
					if(link_bill_rec.CurID == bill_rec.CurID)
						paym += lpaym;
				}
			}
		}
		fix_paym = R2(fix_paym);
		paym = R2(paym + fix_paym);
		debt = BR2(bill_rec.Amount - paym);
		if(has_actual_link || ((bill_rec.Flags & BILLF_NEEDPAYMENT || CheckOpFlags(bill_rec.OpID, OPKF_RECKON)) && debt > 0.0)) {
			THROW(ExtractPacket(bill_rec.ID, &pack) > 0);
			pack.Rec.Flags |= (BILLF_FIXEDAMOUNTS | BILLF_NOATURN);
			pack.Rec.PaymAmount = paym; // @v8.5.8
			pack.Amounts.Put(PPAMT_PAYMENT,    bill_rec.CurID, paym, 0, 1);
			pack.Amounts.Put(PPAMT_FIXPAYMENT, bill_rec.CurID, fix_paym, 0, 1);
			/*THROW(*/UpdatePacket(&pack, 0)/*)*/;
		}
		else if(trfr->EnumItems(bill_rec.ID, &(rbybill = 0), &ti) > 0) {
			THROW(ExtractPacket(bill_rec.ID, &pack) > 0);
			pack.Rec.Flags |= (BILLF_FIXEDAMOUNTS | BILLF_NOATURN);
			pack.Rec.PaymAmount = paym; // @v8.5.8
			pack.Amounts.Put(PPAMT_PAYMENT,    bill_rec.CurID, paym, 0, 1);
			pack.Amounts.Put(PPAMT_FIXPAYMENT, bill_rec.CurID, fix_paym, 0, 1);
			/*THROW(*/UpdatePacket(&pack, 0)/*)*/;
		}
		else {
			if(!RemovePacket(bill_rec.ID, 0)) {
				THROW(ExtractPacket(bill_rec.ID, &pack) > 0);
				pack.Rec.Flags |= (BILLF_FIXEDAMOUNTS | BILLF_NOATURN);
				pack.Rec.PaymAmount = paym; // @v8.5.8
				pack.Amounts.Put(PPAMT_PAYMENT,    bill_rec.CurID, paym, 0, 1);
				pack.Amounts.Put(PPAMT_FIXPAYMENT, bill_rec.CurID, fix_paym, 0, 1);
				/*THROW(*/UpdatePacket(&pack, 0)/*)*/;
			}
		}
	}
	THROW(atobj->P_Tbl->LockingFRR(0, &frrl_tag, 0));
	THROW(CuttingUntiedCLB(0));
	THROW(PPCommitWork(&ta));
	THROW(LoadBalance(endDate, balance_fname, 1));
	CATCH
		ok = 0;
		atobj->P_Tbl->LockingFRR(-1, &frrl_tag, 0);
		PPRollbackWork(&ta);
	ENDCATCH
	trfr->__DontCheckQttyInUpdateTransferItem__ = 0;
	return ok;
}

int SLAPI PPObjBill::SaveBalance(LDATE endDate, const char * pFileName)
{
	int    ok = 1;
	char   msg_buf[64], format[64];
	IterCounter cntr;
	FILE * f_out = 0;
	AcctRelTbl::Key3 k;
	MEMSZERO(k);
	f_out = fopen(pFileName, onecstr('w'));
	THROW_PP(f_out, PPERR_EXPFOPENFAULT);
	PPGetSubStr(PPTXT_CUTTINGPROCESS, 0, msg_buf, sizeof(msg_buf));
	PPGetSubStr(PPTXT_SAVBAL_STRINGS, 0, format, sizeof(format));
	for(cntr.Init(&atobj->P_Tbl->AccRel); atobj->P_Tbl->AccRel.search(3, &k, spGt);) {
		AcctRelTbl::Rec accrel_rec;
		atobj->P_Tbl->AccRel.copyBufTo(&accrel_rec);
		if(!accrel_rec.Closed) {
			double rest = 0.0;
			atobj->P_Tbl->GetAcctRest(endDate, accrel_rec.ID, &rest, 0);
			fprintf(f_out, format, accrel_rec.ID, accrel_rec.AccID,
				accrel_rec.ArticleID, accrel_rec.Ac, accrel_rec.Sb,
				accrel_rec.Ar, accrel_rec.CurID, rest);
		}
		PPWaitPercent(cntr.Increment(), msg_buf);
	}
	CATCHZOK
	SFile::ZClose(&f_out);
	return ok;
}

int SLAPI PPObjBill::LoadBalance(LDATE endDate, const char * pFileName, int use_ta)
{
	int    ok = 1, ta = 0;
	char   buf[256], msg_buf[64], format[64];
	RECORDNUMBER numrecs = 0, recno = 0;
	CurRateCore cr_core;
	FILE * f_in = fopen(pFileName, onecstr('r'));
	THROW_PP(f_in, PPERR_OPENIMPORTFILE);
	PPGetSubStr(PPTXT_CUTTINGPROCESS, 1, msg_buf, sizeof(msg_buf));
	PPGetSubStr(PPTXT_SAVBAL_STRINGS, 1, format, sizeof(format));
	THROW(PPStartTransaction(&ta, use_ta));
	while(fgets(buf, sizeof(buf), f_in))
		numrecs++;
	rewind(f_in);
	while(fgets(buf, sizeof(buf), f_in)) {
		PPBillPacket pack;
		PPID   accrel_id, acc_id, ar_id, cur_id;
		Acct   acct, beg_rest_acct;
		double rest, actual_rest;
		PPWaitPercent(recno++, numrecs, msg_buf);
		sscanf(buf, format, &accrel_id, &acc_id, &ar_id, &acct.ac, &acct.sb, &acct.ar, &cur_id, &rest);
		atobj->P_Tbl->GetAcctRest(endDate, accrel_id, &actual_rest, 0);
		beg_rest_acct.ac = 100;
		beg_rest_acct.sb = 0;
		beg_rest_acct.ar = 0;
		if(rest != actual_rest) {
			long   flags = 0;
			AcctID acctid;
			PPID   as_id = 0;
			PPAccount acc_rec;
			PPAccTurn * p_at = 0;
			THROW(atobj->CreateBlankAccTurn(PPOPK_GENERICACCTURN, &pack, &flags, 0));
			p_at = & pack.Turns.at(0);
			pack.Rec.CurID = p_at->CurID = cur_id;
			pack.Rec.Dt    = p_at->Date  = endDate;
			if(cur_id) {
				LDATE  crate_date = endDate;
				cr_core.GetRate(cur_id, LConfig.BaseRateTypeID, LConfig.BaseCurID, &crate_date, &p_at->CRate);
			}
			atobj->ConvertAcct(&acct, cur_id, &acctid, &as_id);
			if(atobj->P_Tbl->AccObj.Search(acctid.ac, &acc_rec) > 0) {
				int    is_outbal = 0, is_reg = 0;
				if(oneof2(acc_rec.Type, ACY_OBAL, ACY_REGISTER)) {
					p_at->DbtID = acctid;
					p_at->DbtSheet = as_id;
					p_at->Amount = rest - actual_rest;
					if(acc_rec.Type == ACY_REGISTER)
						p_at->Flags |= PPAF_REGISTER;
					else
						p_at->Flags |= PPAF_OUTBAL;
				}
				else {
					if(rest > actual_rest) {
						atobj->ConvertAcct(&acct, cur_id, &p_at->DbtID, &p_at->DbtSheet);
						atobj->ConvertAcct(&beg_rest_acct, cur_id, &p_at->CrdID, &p_at->CrdSheet);
						p_at->Amount = rest - actual_rest;
					}
					else {
						atobj->ConvertAcct(&acct, cur_id, &p_at->CrdID, &p_at->CrdSheet);
						atobj->ConvertAcct(&beg_rest_acct, cur_id, &p_at->DbtID, &p_at->DbtSheet);
						p_at->Amount = actual_rest - rest;
					}
					//p_at->Amount = fabs(rest - actual_rest);
				}
				pack.Rec.Amount = BR2(p_at->Amount);
				pack.Rec.CRate  = p_at->CRate;
				if(cur_id)
					pack.Amounts.Put(PPAMT_CRATE, cur_id, p_at->CRate, 0, 1);
				THROW(TurnPacket(&pack, 0));
			}
		}
	}
	THROW(PPCommitWork(&ta));
	CATCH
		ok = 0;
		PPRollbackWork(&ta);
	ENDCATCH
	return ok;
}

static int SLAPI EditSaveBalanceDialog(LDATE * pDt, char * pFileName)
{
	int    ok = -1;
	LDATE  dt = *pDt;
	char   fname[MAXPATH];
	STRNSCPY(fname, pFileName);
	TDialog * dlg = new TDialog(DLG_SAVEBALANCE);
	if(CheckDialogPtrErr(&dlg)) {
		FileBrowseCtrlGroup::Setup(dlg, CTLBRW_SAVEBALANCE_FNAME, CTL_SAVEBALANCE_FNAME, 1, 0, 0, FileBrowseCtrlGroup::fbcgfFile);
		dlg->SetupCalDate(CTLCAL_SAVEBALANCE_DATE, CTL_SAVEBALANCE_DATE);
		dlg->setCtrlData(CTL_SAVEBALANCE_DATE, &dt);
		dlg->setCtrlData(CTL_SAVEBALANCE_FNAME, fname);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_SAVEBALANCE_DATE, &dt);
			dlg->getCtrlData(CTL_SAVEBALANCE_FNAME, fname);
			*pDt = dt;
			strcpy(pFileName, fname);
			ok = 1;
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

int SLAPI SaveBalance()
{
	int    ok = -1;
	LDATE  dt = LConfig.OperDate;
	char   fname[MAXPATH];
	memzero(fname, sizeof(fname));
	if(EditSaveBalanceDialog(&dt, fname) > 0) {
		PPWait(1);
		if(BillObj->SaveBalance(dt, fname))
			ok = 1;
		else
			ok = PPErrorZ();
		PPWait(0);
	}
	return ok;
}

int SLAPI RestoreBalance()
{
	int    ok = -1;
	LDATE  dt = LConfig.OperDate;
	char   fname[MAXPATH];
	memzero(fname, sizeof(fname));
	if(EditSaveBalanceDialog(&dt, fname) > 0) {
		PPWait(1);
		if(BillObj->LoadBalance(dt, fname, 1))
			ok = 1;
		else
			ok = PPErrorZ();
		PPWait(0);
	}
	return ok;
}

int SLAPI DatabaseCutting()
{
	int    ok = -1, valid_data = 0;
	TDialog * dlg = new TDialog(DLG_CUTTING);
	if(CheckDialogPtrErr(&dlg)) {
		LDATE  end_date = ZERODATE;
		SetupCalCtrl(CTLCAL_CUTTING_DATE, dlg, CTL_CUTTING_DATE, 4);
		dlg->setCtrlData(CTL_CUTTING_DATE, &end_date);
		while(!valid_data && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_CUTTING_DATE, &end_date);
			if(checkdate(&end_date))
				valid_data = 1;
			else
				PPError(PPERR_SLIB, 0);
		}
		delete dlg;
		dlg = 0;
		if(valid_data) {
			PPWait(1);
			ok = BillObj->Cutting(end_date, 1) ? 1 : PPErrorZ();
			PPWait(0);
		}
	}
	return ok;
}
